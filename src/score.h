#pragma once
#ifdef NO_UEFI
unsigned long long top_of_stack;
#endif
#include "platform/memory_util.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/profiling.h"
#include "public_settings.h"
#include "score_cache.h"


////////// Scoring algorithm \\\\\\\\\\

#define NOT_CALCULATED -127 //not yet calculated
#define NULL_INDEX -2

#if !(defined (__AVX512F__) || defined(__AVX2__))
static_assert(false, "Either AVX2 or AVX512 is required.");
#endif

static const char LUT3States[3] = { 0, 1, -1 };

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
struct ScoreFunction
{
    static constexpr unsigned long long numberOfNeurons = numberOfInputNeurons + numberOfOutputNeurons;
    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long maxNumberOfSynapses = populationThreshold * numberOfNeighbors;

    static_assert(numberOfNeighbors % 2 == 0, "NUMBER_OF_NEIGHBOR_NEURONS must divided by 2");
    static_assert(populationThreshold > numberOfNeurons, "populationThreshold must be greater than numberOfNeurons");

    // This check for the clamp neuron index function
    static_assert(numberOfNeurons > numberOfNeighbors, "Number of neurons must be greater than the number of neighbors");


    // Clamp the neuron value
    template  <typename T>
    static T clampNeuron(T val)
    {
        if (val > NEURON_VALUE_LIMIT)
        {
            return NEURON_VALUE_LIMIT;
        }
        else if (val < -NEURON_VALUE_LIMIT)
        {
            return -NEURON_VALUE_LIMIT;
        }
        return val;
    }

    struct K12EngineX1
    {
        unsigned long long Aba, Abe, Abi, Abo, Abu;
        unsigned long long Aga, Age, Agi, Ago, Agu;
        unsigned long long Aka, Ake, Aki, Ako, Aku;
        unsigned long long Ama, Ame, Ami, Amo, Amu;
        unsigned long long Asa, Ase, Asi, Aso, Asu;
        unsigned long long scatteredStates[25];
        int leftByteInCurrentState;
        unsigned char* _pPoolBuffer;
        unsigned int _x;
    private:
        void _scatterFromVector()
        {
            copyToStateScalar(scatteredStates)
        }
        void hashNewChunk()
        {
            declareBCDEScalar
                rounds12Scalar
        }
        void hashNewChunkAndSaveToState()
        {
            hashNewChunk();
            _scatterFromVector();
            leftByteInCurrentState = 200;
        }
    public:
        K12EngineX1() {}
        void initState(const unsigned long long* comp_u64, const unsigned long long* nonce_u64, unsigned char* pPoolBuffer)
        {
            Aba = comp_u64[0];
            Abe = comp_u64[1];
            Abi = comp_u64[2];
            Abo = comp_u64[3];
            Abu = nonce_u64[0];
            Aga = nonce_u64[1];
            Age = nonce_u64[2];
            Agi = nonce_u64[3];
            Ago = Agu = Aka = Ake = Aki = Ako = Aku = Ama = Ame = Ami = Amo = Amu = Asa = Ase = Asi = Aso = Asu = 0;
            leftByteInCurrentState = 0;

            _x = 0;
            _pPoolBuffer = pPoolBuffer;
            write(_pPoolBuffer, RANDOM2_POOL_SIZE);
        }

        void write(unsigned char* out0, int size)
        {
            unsigned char* s0 = (unsigned char*)scatteredStates;
            if (leftByteInCurrentState)
            {
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                copyMem(out0, s0 + 200 - leftByteInCurrentState, copySize);
                size -= copySize;
                leftByteInCurrentState -= copySize;
                out0 += copySize;
            }
            while (size)
            {
                if (!leftByteInCurrentState)
                {
                    hashNewChunkAndSaveToState();
                }
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                copyMem(out0, s0 + 200 - leftByteInCurrentState, copySize);
                size -= copySize;
                leftByteInCurrentState -= copySize;
                out0 += copySize;
            }
        }

        unsigned int random2FromPrecomputedPool(unsigned char* output, unsigned long long outputSize)
        {
            for (unsigned long long i = 0; i < outputSize; i += 8)
            {
                *((unsigned long long*) & output[i]) = *((unsigned long long*) & _pPoolBuffer[_x & (RANDOM2_POOL_ACTUAL_SIZE - 1)]);
                _x = _x * 1664525 + 1013904223;// https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
            }
            return _x;
        }

        void scatterFromVector()
        {
            _scatterFromVector();
        }
        void hashWithoutWrite(int size)
        {
            if (leftByteInCurrentState)
            {
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                size -= copySize;
                leftByteInCurrentState -= copySize;
            }
            while (size)
            {
                if (!leftByteInCurrentState)
                {
                    hashNewChunk();
                    leftByteInCurrentState = 200;
                }
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                size -= copySize;
                leftByteInCurrentState -= copySize;
            }
        }
    };

    struct computeBuffer
    {
        K12EngineX1 k12;
        m256i currentRandomSeed;
        unsigned char computorPublicKey[32];
        unsigned char randomPoolBuffer[RANDOM2_POOL_SIZE];

        void initMiningData(m256i miningSeed)
        {
            // Change of mining seed. re-generate mining data
            if (!isZero(miningSeed) && currentRandomSeed != miningSeed)
            {
                currentRandomSeed = miningSeed;

                // Generate data using mining seed
                generateMiningData(currentRandomSeed.m256i_u8);
            }
        }
        void initMemory()
        {
            currentRandomSeed = m256i::zero();
        }
        void deInitMemory()
        {

        }

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
            PROFILE_NAMED_SCOPE("Score::mutate");

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
            long long nnIndex = neuronIdx + value;
            if (nnIndex >= (long long)population)
            {
                nnIndex -= population;
            }
            else if (nnIndex < 0)
            {
                nnIndex += population;
            }

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
                copyMem(getSynapses(shiftIdx), getSynapses(shiftIdx + 1), numberOfNeighbors * sizeof(Synapse));
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
            PROFILE_NAMED_SCOPE("Score::processTick");

            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;

            // Memset value of current one
            setMem(neuronValueBuffer, sizeof(neuronValueBuffer), 0);

            // Loop though all neurons
            for (long long n = 0; n < population; ++n)
            {
                const Synapse* kSynapses = getSynapses(n);
                long long neuronValue = neurons[n].value;
                // Scan through all neighbor neurons and sum all connected neurons.
                // The synapses are arranged as neuronIndex * numberOfNeighbors
                for (long long m = 0; m < numberOfNeighbors / 2; m++)
                {
                    char synapseWeight = kSynapses[m].weight;
                    unsigned long long nnIndex = clampNeuronIndex(n + m, -(long long)numberOfNeighbors / 2);

                    // Weight-sum
                    neuronValueBuffer[nnIndex] += synapseWeight * neuronValue;
                }

                for (long long m = numberOfNeighbors / 2; m < numberOfNeighbors; m++)
                {
                    char synapseWeight = kSynapses[m].weight;
                    unsigned long long nnIndex =  clampNeuronIndex(n + m + 1, -(long long)numberOfNeighbors / 2);

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
            PROFILE_NAMED_SCOPE("Score::runTickSimulation");

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

                    neuronValue = LUT3States[randomValue % 3];

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
                neuronValue = LUT3States[randomNumber % 3];
                outputNeuronExpectedValue[i] = neuronValue;
            }
        }

        unsigned int initializeANN(const unsigned char nonce[32])
        {
            PROFILE_NAMED_SCOPE("Score::initializeANN");
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
                synapses[i].weight = LUT3States[initValue.synapseWeight[i] % 3];
            }

            // Init input neuron value and output neuron
            initInputNeuron();
            initOutputNeuron();

            // Init expected output neuron
            initExpectedOutputNeuron();

            // Ticks simulation
            runTickSimulation();

            // Copy the state for rollback later
            copyMem(&bestANN, &currentANN, sizeof(ANN));

            // Compute R and roll back if neccessary
            unsigned int R = computeNonMatchingOutput();

            return R;
        }
        // Main function for mining
        unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce)
        {
            PROFILE_NAMED_SCOPE("Score::ComputeScore");

            copyMem(computorPublicKey, publicKey, sizeof(computorPublicKey));

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
                    copyMem(&currentANN, &bestANN, sizeof(bestANN));
                }
                else
                {
                    bestR = R;

                    // Better R. Save the state
                    copyMem(&bestANN, &currentANN, sizeof(bestANN));
                }

                ASSERT(bestANN.population <= populationThreshold);
            }

            // Check score
            ASSERT(bestR <= numberOfOutputNeurons);
            unsigned int score = numberOfOutputNeurons - bestR;
            return score;
        }

    } *_computeBuffer = nullptr;
    m256i currentRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    volatile char scoreCacheLock;
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        currentRandomSeed = randomSeed; // persist the initial random seed to be able to send it back on system info response
        if (!isZero(currentRandomSeed))
        {
            for (unsigned int i = 0; i < solutionBufferCount; i++)
            {
                _computeBuffer[i].initMiningData(currentRandomSeed);
            }
        }
    }

    ~ScoreFunction()
    {
        freeMemory();
    }

    void freeMemory()
    {
        if (_computeBuffer)
        {
            for (unsigned int i = 0; i < solutionBufferCount; i++)
            {
                _computeBuffer[i].deInitMemory();
            }

            freePool(_computeBuffer);
            _computeBuffer = nullptr;
        }
    }

    bool initMemory()
    {
        if (_computeBuffer == nullptr)
        {
            if (!allocPoolWithErrorLog(L"computeBuffer (score solution buffer)", sizeof(computeBuffer) * solutionBufferCount, (void**)&_computeBuffer, __LINE__))
            {
                return false;
            }

            for (int bufId = 0; bufId < solutionBufferCount; bufId++)
            {
                auto& cb = _computeBuffer[bufId];
                cb.initMemory();
            }
        }

        for (int i = 0; i < solutionBufferCount; i++)
        {
            solutionEngineLock[i] = 0;
        }

#if USE_SCORE_CACHE
        scoreCacheLock = 0;
        setMem(&scoreCache, sizeof(scoreCache), 0);
#endif

        return true;
    }

    // Save score cache to SCORE_CACHE_FILE_NAME
    void saveScoreCache(int epoch, CHAR16* directory = NULL)
    {
#if USE_SCORE_CACHE
        ACQUIRE(scoreCacheLock);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        scoreCache.save(SCORE_CACHE_FILE_NAME, directory);
        RELEASE(scoreCacheLock);
#endif
    }

    // Update score cache filename with epoch and try to load file
    bool loadScoreCache(int epoch)
    {
        bool success = true;
#if USE_SCORE_CACHE
        ACQUIRE(scoreCacheLock);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        success = scoreCache.load(SCORE_CACHE_FILE_NAME);
        RELEASE(scoreCacheLock);
#endif
        return success;
    }

    bool isValidScore(unsigned int solutionScore)
    {
        return (solutionScore >= 0 && solutionScore <= numberOfOutputNeurons);
    }
    bool isGoodScore(unsigned int solutionScore, int threshold)
    {
        return (threshold <= numberOfOutputNeurons) && (solutionScore >= (unsigned int)threshold);
    }

    unsigned int computeScore(const int solutionBufIdx, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        _computeBuffer[solutionBufIdx].initMiningData(miningSeed.m256i_u8);
        return _computeBuffer[solutionBufIdx].computeScore(publicKey.m256i_u8, nonce.m256i_u8);
    }

    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        PROFILE_SCOPE();

        if (isZero(miningSeed) || miningSeed != currentRandomSeed)
        {
            return numberOfOutputNeurons + 1; // return invalid score
        }

        int score = 0;
#if USE_SCORE_CACHE
        unsigned int scoreCacheIndex = scoreCache.getCacheIndex(publicKey, miningSeed, nonce);
        score = scoreCache.tryFetching(publicKey, miningSeed, nonce, scoreCacheIndex);
        if (score >= scoreCache.MIN_VALID_SCORE)
        {
            return score;
        }
        score = 0;
#endif

        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);
        ACQUIRE(solutionEngineLock[solutionBufIdx]);

        score = computeScore(processor_Number, publicKey, miningSeed, nonce);

        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        scoreCache.addEntry(publicKey, miningSeed, nonce, scoreCacheIndex, score);
#endif
#ifdef NO_UEFI
        int y = 2 + score;
        stackSize = top_of_stack - ((unsigned long long)(&y));
#endif
        return score;
    }

#ifdef NO_UEFI
    unsigned long long stackSize = 0;
#endif

    // Multithreaded solutions verification:
    // This module mainly serve tick processor in qubic core node, thus the queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    // for future use for somewhere else, you can only increase the size.

    volatile char taskQueueLock = 0;
    struct
    {
        m256i publicKey[NUMBER_OF_TRANSACTIONS_PER_TICK];
        m256i miningSeed[NUMBER_OF_TRANSACTIONS_PER_TICK];
        m256i nonce[NUMBER_OF_TRANSACTIONS_PER_TICK];
    } taskQueue;
    unsigned int _nTask;
    unsigned int _nProcessing;
    unsigned int _nFinished;
    bool _nIsTaskQueueReady;

    void resetTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nTask = 0;
        _nProcessing = 0;
        _nFinished = 0;
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // add task to the queue
    // queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    void addTask(m256i publicKey, m256i miningSeed, m256i nonce)
    {
        ACQUIRE(taskQueueLock);
        if (_nTask < NUMBER_OF_TRANSACTIONS_PER_TICK)
        {
            unsigned int index = _nTask++;
            taskQueue.publicKey[index] = publicKey;
            taskQueue.miningSeed[index] = miningSeed;
            taskQueue.nonce[index] = nonce;
        }
        RELEASE(taskQueueLock);
    }

    void startProcessTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = true;
        RELEASE(taskQueueLock);
    }

    void stopProcessTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // get a task, can call on any thread
    bool getTask(m256i* publicKey, m256i* miningSeed, m256i* nonce)
    {
        if (!_nIsTaskQueueReady)
        {
            return false;
        }
        bool result = false;
        ACQUIRE(taskQueueLock);
        if (_nProcessing < _nTask)
        {
            unsigned int index = _nProcessing++;
            *publicKey = taskQueue.publicKey[index];
            *miningSeed = taskQueue.miningSeed[index];
            *nonce = taskQueue.nonce[index];
            result = true;
        }
        else
        {
            result = false;
        }
        RELEASE(taskQueueLock);
        return result;
    }
    void finishTask()
    {
        ACQUIRE(taskQueueLock);
        _nFinished++;
        RELEASE(taskQueueLock);
    }

    bool isTaskQueueProcessed()
    {
        return _nFinished == _nTask;
    }

    void tryProcessSolution(unsigned long long processorNumber)
    {
        m256i publicKey;
        m256i miningSeed;
        m256i nonce;
        bool res = this->getTask(&publicKey, &miningSeed, &nonce);
        if (res)
        {
            (*this)(processorNumber, publicKey, miningSeed, nonce);
            this->finishTask();
        }
    }
};
