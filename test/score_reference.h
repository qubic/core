#pragma once

#include "../src/platform/memory_util.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template <
    unsigned long long numberOfInputNeurons, // K
    unsigned long long numberOfOutputNeurons,// L
    unsigned long long numberOfTicks,        // N
    unsigned long long numberOfNeighbors,    // 2M
    unsigned long long populationThreshold,  // P
    unsigned long long numberOfMutations,    // S
    unsigned int solutionThreshold,
    unsigned long long solutionBufferCount
>
struct ScoreReferenceImplementation
{
    static constexpr unsigned long long numberOfNeurons = numberOfInputNeurons + numberOfOutputNeurons;
    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long maxNumberOfSynapses = populationThreshold * numberOfNeighbors;

    static_assert(numberOfNeighbors % 2 == 0, "NUMBER_OF_NEIGHBOR_NEURONS must divided by 2");
    static_assert(populationThreshold > numberOfNeurons, "populationThreshold must be greater than numberOfNeurons");
    static_assert(numberOfNeurons > numberOfNeighbors, "Number of neurons must be greater than the number of neighbors");

    // Clamp the neuron value
    template <typename T>
    static T clampNeuron(T neuronValue)
    {
        if (neuronValue > 1)
        {
            return 1;
        }

        if (neuronValue < -1)
        {
            return -1;
        }
        return neuronValue;
    }

    void initMemory()
    {
        allocPoolWithErrorLog(L"ComputeBuffer", sizeof(ComputeInstance) * solutionBufferCount, (void**)&(_computeBuffer), __LINE__);
    }

    void freeMemory()
    {
        freePool(_computeBuffer);
    }

    ~ScoreReferenceImplementation()
    {
        freeMemory();
    }

    void initMiningData(m256i randomSeed)
    {
        
    }

    void initMiningData(unsigned char* seed)
    {
        for (int i = 0; i < solutionBufferCount; i++)
        {
            _computeBuffer[i].initMiningData(seed);
        }
    }

    struct ComputeInstance
    {
        void initMiningData(unsigned char miningSeed[32])
        {
            memcpy(currentRandomSeed, miningSeed, sizeof(this->currentRandomSeed));
        }

        unsigned char computorPublicKey[32];
        unsigned char currentRandomSeed[32];
        unsigned char randomPoolBuffer[RANDOM2_POOL_SIZE];

        struct Synapse
        {
            char weight;
        };

        enum NeuronNeighborSide
        {
            kLeft,
            kRight,
            kMid,
            kOutOfBound
        };

        // Data for running the ANN
        struct Neuron
        {
            enum Type
            {
                kInput,
                kOutput,
                kEvolution,
            };
            Type type;
            char value;
            bool markForRemoval;
        };

        // Data for roll back
        struct ANN
        {
            Neuron neurons[maxNumberOfNeurons];
            Synapse synapses[maxNumberOfSynapses];
            unsigned long long population;
        };
        ANN bestANN;
        ANN currentANN;

        // Intermediate data
        struct InitValue
        {
            unsigned long long outputNeuronPositions[numberOfOutputNeurons];
            unsigned long long synapseWeight[maxNumberOfSynapses];
            unsigned long long synpaseMutation[numberOfMutations];
        } initValue;

        struct MiningData
        {
            unsigned char inputNeuronRandomNumber[numberOfInputNeurons];
            unsigned char outputNeuronRandomNumber[numberOfOutputNeurons];
        } miningData;

        unsigned long long neuronIndices[numberOfNeurons];
        char previousNeuronValue[maxNumberOfNeurons];

        unsigned long long outputNeuronIndices[numberOfOutputNeurons];
        char outputNeuronExpectedValue[numberOfOutputNeurons];

        long long neuronValueBuffer[maxNumberOfNeurons];

        void mutate(int mutateStep)
        {
            // Mutation
            unsigned long long population = currentANN.population;
            unsigned long long synapseCount = population * numberOfNeighbors;
            Synapse* synapses = currentANN.synapses;


            // Randomly pick a synapse, randomly increase or decrease its weight by 1 or -1
            unsigned long long synapseMutation = initValue.synpaseMutation[mutateStep];
            unsigned long long synapseIdx = (synapseMutation >> 1) % synapseCount;
            // Randomly increase or decrease its value
            char weightChange = 0;
            if ((synapseMutation & 1ULL) == 0)
            {
                weightChange = -1;
            }
            else
            {
                weightChange = 1;
            }

            char newWeight = synapses[synapseIdx].weight + weightChange;

            // Valid weight. Update it
            if (newWeight >= -1 && newWeight <= 1)
            {
                synapses[synapseIdx].weight = newWeight;
            }
            else // Invalid weight. Insert a neuron
            {
                // Insert the neuron
                insertNeuron(synapseIdx);
            }

            // Clean the ANN
            while (scanRedundantNeurons() > 0)
            {
                cleanANN();
            }
        }

        // Get the pointer to all outgoing synapse of a neurons
        Synapse* getSynapses(unsigned long long neuronIndex)
        {
            return &currentANN.synapses[neuronIndex * numberOfNeighbors];
        }

        // Circulate the neuron index
        unsigned long long clampNeuronIndex(long long neuronIdx, long long value)
        {
            unsigned long long population = currentANN.population;
            long long nnIndex = 0;
            // Calculate the neuron index (ring structure)
            if (value >= 0)
            {
                nnIndex = neuronIdx + value;
            }
            else
            {
                nnIndex = neuronIdx + population + value;
            }
            nnIndex = nnIndex % population;
            return (unsigned long long)nnIndex;
        }


        // Remove a neuron and all synapses relate to it
        void removeNeuron(unsigned long long neuronIdx)
        {
            // Scan all its neigbor to remove their outgoing synapse point to the neuron
            for (long long i = 0; i < numberOfNeighbors; ++i)
            {
                long long delta = i - (long long)numberOfNeighbors / 2;
                unsigned long long neigborNeuronIdx = clampNeuronIndex(neuronIdx, delta);
                Synapse* pNNSynapses = getSynapses(neigborNeuronIdx);

                // Get the index of synapse point to current neuron and mark it as invalid synapse
                long long synapseIndexOfNN = numberOfNeighbors - i;

                // The synapse array need to be shifted regard to the remove neuron
                // Also neuron need to have 2M neighbors, the addtional synapse will be set as zero weight
                // Case1 [S0 S1 S2 - SR S5 S6]. SR is removed, [S0 S1 S2 S5 S6 0]
                // Case2 [S0 S1 SR - S3 S4 S5]. SR is removed, [0 S0 S1 S3 S4 S5]
                if (synapseIndexOfNN >= numberOfNeighbors / 2)
                {
                    for (long long k = synapseIndexOfNN; k < numberOfNeighbors - 1; ++k)
                    {
                        pNNSynapses[k] = pNNSynapses[k + 1];
                    }
                    pNNSynapses[numberOfNeighbors - 1].weight = 0;
                }
                else
                {
                    for (long long k = synapseIndexOfNN; k > 0; --k)
                    {
                        pNNSynapses[k] = pNNSynapses[k - 1];
                    }
                    pNNSynapses[0].weight = 0;
                }
            }

            // Shift the synapse array and the neuron array
            for (unsigned long long shiftIdx = neuronIdx; shiftIdx < currentANN.population; shiftIdx++)
            {
                currentANN.neurons[shiftIdx] = currentANN.neurons[shiftIdx + 1];

                // Also shift the synapses
                memcpy(getSynapses(shiftIdx), getSynapses(shiftIdx + 1), numberOfNeighbors * sizeof(Synapse));
            }
            currentANN.population--;
        }

        // Check if the target is on the left side of base
        int checkNeighborSide(unsigned long long base, unsigned long long target)
        {
            unsigned long long population = currentANN.population;

            if (base == target)
            {
                return kMid;
            }

            NeuronNeighborSide side = kMid;
            unsigned long long distance = 0;
            unsigned long long diff = 0;
            unsigned long long circularDiff = 0;
            if (target > base)
            {
                diff = target - base;
                circularDiff = population + base - target;
                if (diff < circularDiff)
                {
                    distance = diff;
                    side = kRight;
                }
                else
                {
                    distance = circularDiff;
                    side = kLeft;
                }
            }
            else
            {
                diff = base - target;
                circularDiff = population + target - base;
                if (diff < circularDiff)
                {
                    distance = diff;
                    side = kLeft;
                }
                else
                {
                    distance = circularDiff;
                    side = kRight;
                }
            }

            if (distance > numberOfNeighbors / 2)
            {
                return kOutOfBound;
            }
            return side;
        }

        void insertNeuron(unsigned long long synapseIdx)
        {
            // A synapse have incomingNeighbor and outgoingNeuron, direction incomingNeuron -> outgoingNeuron
            unsigned long long incomingNeighborSynapseIdx = synapseIdx % numberOfNeighbors;
            unsigned long long outgoingNeuron = synapseIdx / numberOfNeighbors;
            
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            unsigned long long& population = currentANN.population;

            // Copy original neuron to the inserted one and set it as  Neuron::kEvolution type
            Neuron insertNeuron;
            insertNeuron = neurons[outgoingNeuron];
            insertNeuron.type = Neuron::kEvolution;
            unsigned long long insertedNeuronIdx = outgoingNeuron + 1;

            char originalWeight = synapses[synapseIdx].weight;

            // Insert the neuron into array, population increased one, all neurons next to original one need to shift right
            for (unsigned long long i = population; i > outgoingNeuron; --i)
            {
                neurons[i] = neurons[i - 1];

                // Also shift the synapses to the right
                memcpy(getSynapses(i), getSynapses(i - 1), numberOfNeighbors * sizeof(Synapse));
            }
            neurons[insertedNeuronIdx] = insertNeuron;
            population++;

            // Try to update the synapse of inserted neuron. All outgoing synapse is init as zero weight
            Synapse* pInsertNeuronSynapse = getSynapses(insertedNeuronIdx);
            for (unsigned long long synIdx = 0; synIdx < numberOfNeighbors; ++synIdx)
            {
                pInsertNeuronSynapse[synIdx].weight = 0;
            }

            // Copy the outgoing synapse of original neuron
            // Outgoing points to the left
            if (incomingNeighborSynapseIdx < numberOfNeighbors / 2)
            {
                if (incomingNeighborSynapseIdx > 0)
                {
                    // Decrease by one because the new neuron is next to the original one
                    pInsertNeuronSynapse[incomingNeighborSynapseIdx - 1].weight = originalWeight;
                }
                // Incase of the outgoing synapse point too far, don't add the synapse
            }
            else
            {
                // No need to adjust the added neuron but need to remove the synapse of the original neuron
                pInsertNeuronSynapse[incomingNeighborSynapseIdx].weight = originalWeight;
            }

            // The change of synapse only impact neuron in [originalNeuronIdx - numberOfNeighbors / 2 + 1, originalNeuronIdx +  numberOfNeighbors / 2]
            // In the new index, it will be  [originalNeuronIdx + 1 - numberOfNeighbors / 2, originalNeuronIdx + 1 + numberOfNeighbors / 2]
            // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
            for (long long delta = -(long long)numberOfNeighbors / 2; delta <= numberOfNeighbors / 2; ++delta)
            {
                unsigned long long updatedNeuronIdx = clampNeuronIndex(insertedNeuronIdx, delta);
                Synapse* pUpdatedSynapses = getSynapses(updatedNeuronIdx);

                // Generate a list of neighbor index of current updated neuron NN
                // Find the location of the inserted neuron in the list of neighbors
                long long insertedNeuronIdxInNeigborList = -1;
                for (long long k = 0; k < numberOfNeighbors; k++)
                {
                    unsigned long long nnIndex = 0;
                    if (k < (numberOfNeighbors / 2))
                    {
                        nnIndex = clampNeuronIndex(updatedNeuronIdx + k, -(long long)numberOfNeighbors / 2);
                    }
                    else
                    {
                        nnIndex = clampNeuronIndex(updatedNeuronIdx + k + 1, -(long long)numberOfNeighbors / 2);
                    }

                    if (nnIndex == insertedNeuronIdx)
                    {
                        insertedNeuronIdxInNeigborList = k;
                    }
                }

                // Something is wrong here if we can not find the
                assert(insertedNeuronIdxInNeigborList >= 0);

                // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
                // Case: neurons in range [N0 N1 N2 original], right synapses will be affected
                if (delta < 0)
                {
                    // Left side is kept as it is, only need to shift to the right side
                    for (unsigned long long k = numberOfNeighbors - 1; k >= insertedNeuronIdxInNeigborList; --k)
                    {
                        // Updated synapse
                        pUpdatedSynapses[k] = pUpdatedSynapses[k - 1];
                    }
                }
                else // Case: neurons in range [inserted N4 N5 N6], left synapses will be affected
                {
                    // Right side is kept as it is, only need to shift to the left side
                    for (unsigned long long k = 0; k <= insertedNeuronIdxInNeigborList; ++k)
                    {
                        // Updated synapse
                        pUpdatedSynapses[k] = pUpdatedSynapses[k + 1];
                    }
                }
            }
        }

        // Check which neurons/synapse need to be removed after mutation
        unsigned long long scanRedundantNeurons()
        {
            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;

            unsigned long long numberOfRedundantNeurons = 0;
            // After each mutation, we must verify if there are neurons that do not affect the ANN output.
            // These are neurons that either have all incoming synapse weights as 0,
            // or all outgoing synapse weights as 0. Such neurons must be removed.
            for (unsigned long long i = 0; i < population; i++)
            {
                neurons[i].markForRemoval = false;
                if (neurons[i].type == Neuron::kEvolution)
                {
                    bool allOutGoingZeros = true;
                    bool allIncommingZeros = true;

                    // Loop though its synapses for checkout outgoing synapses
                    for (unsigned long long n = 0; n < numberOfNeighbors; n++)
                    {
                        char synapseW = synapses[i * numberOfNeighbors + n].weight;
                        if (synapseW != 0)
                        {
                            allOutGoingZeros = false;
                            break;
                        }
                    }

                    // Loop through the neighbor neurons to check all incoming synapses
                    for (unsigned long long n = 0; n <= numberOfNeighbors; n++)
                    {
                        if (n == numberOfNeighbors / 2)
                        {
                            continue;
                        }
                        unsigned long long nnIdx = clampNeuronIndex(i + n, -(long long)numberOfNeighbors / 2);
                        char synapseW = synapses[nnIdx * numberOfNeighbors].weight;
                        if (synapseW != 0)
                        {
                            allIncommingZeros = false;
                            break;
                        }
                    }
                    if (allOutGoingZeros || allIncommingZeros)
                    {
                        neurons[i].markForRemoval = true;
                        numberOfRedundantNeurons++;
                    }
                }
            }
            return numberOfRedundantNeurons;
        }

        // Remove neurons and synapses that do not affect the ANN
        void cleanANN()
        {
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            unsigned long long& population = currentANN.population;

            // Scan and remove neurons/synapses
            unsigned long long neuronIdx = 0;
            while (neuronIdx < population)
            {
                if (neurons[neuronIdx].markForRemoval)
                {
                    // Remove it from the neuron list. Overwrite data
                    // Remove its synapses in the synapses array
                    removeNeuron(neuronIdx);
                }
                else
                {
                    neuronIdx++;
                }
            }
        }

        void processTick()
        {
            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;

            // Memset value of current one
            memset(neuronValueBuffer, 0, sizeof(neuronValueBuffer));

            // Loop though all neurons
            for (long long n = 0; n < population; ++n)
            {
                const Synapse* kSynapses = getSynapses(n);
                long long neuronValue = neurons[n].value;
                // Scan through all neighbor neurons and sum all connected neurons.
                // The synapses are arranged as neuronIndex * numberOfNeighbors
                for (long long m = 0; m < numberOfNeighbors; m++)
                {
                    char synapseWeight = kSynapses[m].weight;
                    unsigned long long nnIndex = 0;
                    if (m < numberOfNeighbors / 2)
                    {
                        nnIndex = clampNeuronIndex(n + m, -(long long)numberOfNeighbors / 2);
                    }
                    else
                    {
                        nnIndex = clampNeuronIndex(n + m + 1, -(long long)numberOfNeighbors / 2);
                    }

                    // Weight-sum
                    neuronValueBuffer[nnIndex] += synapseWeight * neuronValue;
                }
            }

            // Clamp the neuron value
            for (long long n = 0; n < population; ++n)
            {
                long long neuronValue = clampNeuron(neuronValueBuffer[n]);
                neurons[n].value = neuronValue;
            }
        }

        void runTickSimulation()
        {
            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;

            // Save the neuron value for comparison
            for (unsigned long long i = 0; i < population; ++i)
            {
                // Backup the neuron value
                previousNeuronValue[i] = neurons[i].value;
            }

            for (unsigned long long tick = 0; tick < numberOfTicks; ++tick)
            {
                processTick();
                // Check exit conditions:
                // - N ticks have passed (already in for loop)
                // - All neuron values are unchanged
                // - All output neurons have non-zero values
                bool shouldExit = true;
                bool allNeuronsUnchanged = true;
                bool allOutputNeuronsIsNonZeros = true;
                for (long long n = 0; n < population; ++n)
                {
                    // Neuron unchanged check
                    if (previousNeuronValue[n] != neurons[n].value)
                    {
                        allNeuronsUnchanged = false;
                    }

                    // Ouput neuron value check
                    if (neurons[n].type == Neuron::kOutput && neurons[n].value == 0)
                    {
                        allOutputNeuronsIsNonZeros = false;
                    }
                }

                if (allOutputNeuronsIsNonZeros || allNeuronsUnchanged)
                {
                    break;
                }

                // Copy the neuron value
                for (long long n = 0; n < population; ++n)
                {
                    previousNeuronValue[n] = neurons[n].value;
                }
            }
        }

        unsigned int computeNonMatchingOutput()
        {
            unsigned long long population = currentANN.population;
            Neuron* neurons = currentANN.neurons;

            // Compute the non-matching value R between output neuron value and initial value
            // Because the output neuron order never changes, the order is preserved
            unsigned int R = 0;
            unsigned long long outputIdx = 0;
            for (unsigned long long i = 0; i < population; i++)
            {
                if (neurons[i].type == Neuron::kOutput)
                {
                    if (neurons[i].value != outputNeuronExpectedValue[outputIdx])
                    {
                        R++;
                    }
                    outputIdx++;
                }
            }
            return R;
        }

        void generateMiningData(unsigned char miningSeed[32])
        {
            // Init the neuron input and expected output value
            random2(miningSeed, (unsigned char*)&miningData, sizeof(miningData), randomPoolBuffer);
        }

        void initInputNeuron()
        {
            unsigned long long population = currentANN.population;
            Neuron* neurons = currentANN.neurons;
            unsigned long long inputNeuronInitIndex = 0;
            for (unsigned long long i = 0; i < population; ++i)
            {
                // Input will use the init value
                if (neurons[i].type == Neuron::kInput)
                {
                    char neuronValue = 0;
                    unsigned char randomValue = miningData.inputNeuronRandomNumber[inputNeuronInitIndex];
                    inputNeuronInitIndex++;
                    if (randomValue % 3 == 0)
                    {
                        neuronValue = 0;
                    }
                    else if (randomValue % 3 == 1)
                    {
                        neuronValue = 1;
                    }
                    else
                    {
                        neuronValue = -1;
                    }

                    // Convert value of neuron to trits (keeping 1 as 1, and changing 0 to -1.).
                    neurons[i].value = (neuronValue == 0) ? -1 : neuronValue;
                }
            }
        }

        void initOutputNeuron()
        {
            unsigned long long population = currentANN.population;
            Neuron* neurons = currentANN.neurons;
            for (unsigned long long i = 0; i < population; ++i)
            {
                if (neurons[i].type == Neuron::kOutput)
                {
                    neurons[i].value = 0;
                }
            }
        }

        void initExpectedOutputNeuron()
        {
            for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
            {
                char neuronValue = 0;
                unsigned char randomNumber = miningData.outputNeuronRandomNumber[i];
                if (randomNumber % 3 == 0)
                {
                    neuronValue = 0;
                }
                else if (randomNumber % 3 == 1)
                {
                    neuronValue = 1;
                }
                else
                {
                    neuronValue = -1;
                }

                outputNeuronExpectedValue[i] = neuronValue;
            }
        }

        unsigned int initializeANN(const unsigned char nonce[32])
        {
            unsigned long long& population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;

            // Initialization
            population = numberOfNeurons;

            // Initalize with nonce and public key
            static_assert(sizeof(InitValue) % 8 == 0, "InitValue size must divided by 8");
            random2(computorPublicKey, nonce, (unsigned char*)&initValue, sizeof(InitValue), randomPoolBuffer);

            // Randomly choose the positions of neurons types
            for (unsigned long long i = 0; i < population; ++i)
            {
                neuronIndices[i] = i;
                neurons[i].type = Neuron::kInput;
            }
            unsigned long long neuronCount = population;
            for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
            {
                unsigned long long outputNeuronIdx = initValue.outputNeuronPositions[i] % neuronCount;

                // Fill the neuron type
                neurons[neuronIndices[outputNeuronIdx]].type = Neuron::kOutput;
                outputNeuronIndices[i] = neuronIndices[outputNeuronIdx];

                // This index is used, copy the end of indices array to current position and decrease the number of picking neurons
                neuronCount = neuronCount - 1;
                neuronIndices[outputNeuronIdx] = neuronIndices[neuronCount];
            }

            // Synapse weight initialization
            const unsigned long long initNumberOfSynapses = population * numberOfNeighbors;
            for (unsigned long long i = 0; i < initNumberOfSynapses; ++i)
            {
                if (initValue.synapseWeight[i] % 3 == 0)
                {
                    synapses[i].weight = 0;
                }
                else if (initValue.synapseWeight[i] % 3 == 1)
                {
                    synapses[i].weight = 1;
                }
                else if (initValue.synapseWeight[i] % 3 == 2)
                {
                    synapses[i].weight = -1;
                }
            }

            // Generate data using mining seed
            generateMiningData(currentRandomSeed);

            // Init input neuron value and output neuron
            initInputNeuron();
            initOutputNeuron();

            // Init expected output neuron
            initExpectedOutputNeuron();

            // Ticks simulation
            runTickSimulation();

            // Copy the state for rollback later
            memcpy(&bestANN, &currentANN, sizeof(ANN));

            // Compute R and roll back if neccessary
            unsigned int R = computeNonMatchingOutput();

            return R;
        }
        // Main function for mining
        unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce)
        {
            memcpy(computorPublicKey, publicKey, sizeof(computorPublicKey));

            // Initialize
            unsigned int bestR = initializeANN(nonce);

            for (unsigned long long s = 0; s < numberOfMutations; ++s)
            {
                // Do the mutation
                mutate(s);

                // Exit if the number of population reaches the maximum allowed
                if (currentANN.population >= populationThreshold)
                {
                    break;
                }

                // Ticks simulation
                runTickSimulation();

                // Compute R and roll back if neccessary
                unsigned int R = computeNonMatchingOutput();
                if (R > bestR)
                {
                    // Roll back
                    memcpy(&currentANN, &bestANN, sizeof(bestANN));
                }
                else
                {
                    bestR = R;

                    // Better R. Save the state
                    memcpy(&bestANN, &currentANN, sizeof(bestANN));
                }

                assert(bestANN.population <= populationThreshold);
            }

            // Check score
            assert(bestR <= numberOfOutputNeurons);
            unsigned int score = numberOfOutputNeurons - bestR;
            return score;
        }
    };

    ComputeInstance* _computeBuffer;

    unsigned int operator()(unsigned long long processorNumber, unsigned char* publicKey, unsigned char* nonce)
    {
        processorNumber %= solutionBufferCount;
        return _computeBuffer[processorNumber].computeScore(publicKey, nonce);
    }

};
