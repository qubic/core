#pragma once

#include "score_common_reference.h"

#include <cassert>
#include <vector>

namespace score_addition_reference
{

template <typename Params>
struct Miner
{
    // Convert params for easier usage
    static constexpr unsigned long long numberOfInputNeurons = Params::numberOfInputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = Params::numberOfOutputNeurons;
    static constexpr unsigned long long numberOfTicks = Params::numberOfTicks;
    static constexpr unsigned long long maxNumberOfNeighbors = Params::numberOfNeighbors;
    static constexpr unsigned long long populationThreshold = Params::populationThreshold;
    static constexpr unsigned long long numberOfMutations = Params::numberOfMutations;
    static constexpr unsigned int solutionThreshold = Params::solutionThreshold;

    static constexpr unsigned long long numberOfNeurons =
        numberOfInputNeurons + numberOfOutputNeurons;
    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long maxNumberOfSynapses =
        populationThreshold * maxNumberOfNeighbors;
    static constexpr unsigned long long trainingSetSize = 1ULL << numberOfInputNeurons; // 2^K
    static constexpr unsigned long long paddingNumberOfSynapses =
        (maxNumberOfSynapses + 31 ) / 32 * 32; // padding to multiple of 32

    static_assert(
        maxNumberOfSynapses <= (0xFFFFFFFFFFFFFFFF << 1ULL),
        "maxNumberOfSynapses must less than or equal MAX_UINT64/2");
    static_assert(maxNumberOfNeighbors % 2 == 0, "maxNumberOfNeighbors must divided by 2");
    static_assert(
        populationThreshold > numberOfNeurons,
        "populationThreshold must be greater than numberOfNeurons");

    std::vector<unsigned char> poolVec;

    void initialize(const unsigned char miningSeed[32])
    {
        // Init random2 pool with mining seed
        poolVec.resize(score_reference::POOL_VEC_PADDING_SIZE);
        score_reference::generateRandom2Pool(miningSeed, poolVec.data());
    }

    // Training set
    struct TraningPair
    {
        char input[numberOfInputNeurons]; // numberOfInputNeurons / 2 bits of A , and B (values: -1 or +1)
        char output[numberOfOutputNeurons];  // numberOfOutputNeurons bits of C (values: -1 or +1)
    } trainingSet[trainingSetSize];       // training set size: 2^K

    struct Synapse
    {
        char weight;
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
        unsigned long long synapseWeight[paddingNumberOfSynapses / 32]; // each 64bits elements will
                                                                        // decide value of 32 synapses
        unsigned long long synpaseMutation[numberOfMutations];
    } initValue;

    unsigned long long neuronIndices[numberOfNeurons];
    char previousNeuronValue[maxNumberOfNeurons];

    unsigned long long outputNeuronIndices[numberOfOutputNeurons];
    char outputNeuronExpectedValue[numberOfOutputNeurons];

    long long neuronValueBuffer[maxNumberOfNeurons];

    unsigned long long getActualNeighborCount() const
    {
        unsigned long long population = currentANN.population;
        unsigned long long maxNeighbors = population - 1;  // Exclude self
        unsigned long long actual = std::min(maxNumberOfNeighbors, maxNeighbors);
        
        return actual;
    }

    unsigned long long getLeftNeighborCount() const
    {
        unsigned long long actual = getActualNeighborCount();
        // For odd number, we add extra for the left
        return (actual + 1) / 2;
    }

    unsigned long long getRightNeighborCount() const
    {
        return getActualNeighborCount() - getLeftNeighborCount();
    }

    // Get the starting index in synapse buffer (left side start)
    unsigned long long getSynapseStartIndex() const
    {
        constexpr unsigned long long synapseBufferCenter = maxNumberOfNeighbors / 2;
        return synapseBufferCenter - getLeftNeighborCount();
    }

    // Get the ending index in synapse buffer (exclusive)
    unsigned long long getSynapseEndIndex() const
    {
        constexpr unsigned long long synapseBufferCenter = maxNumberOfNeighbors / 2;
        return synapseBufferCenter + getRightNeighborCount();
    }

    // Convert buffer index to neighbor offset
    long long bufferIndexToOffset(unsigned long long bufferIdx) const
    {
        constexpr long long synapseBufferCenter = maxNumberOfNeighbors / 2;
        if (bufferIdx < synapseBufferCenter)
        {
            return (long long)bufferIdx - synapseBufferCenter;  // Negative (left)
        }
        else
        {
            return (long long)bufferIdx - synapseBufferCenter + 1;  // Positive (right), skip 0
        }
    }

    // Convert neighbor offset to buffer index
    long long offsetToBufferIndex(long long offset) const
    {
        constexpr long long synapseBufferCenter = maxNumberOfNeighbors / 2;
        if (offset == 0)
        {
            return -1;  // Invalid, exclude self
        }
        else if (offset < 0)
        {
            return synapseBufferCenter + offset;
        }
        else
        {
            return synapseBufferCenter + offset - 1;
        }
    }

    long long getIndexInSynapsesBuffer(long long neighborOffset) const
    {
        long long leftCount = (long long)getLeftNeighborCount();
        long long rightCount = (long long)getRightNeighborCount();
        
        if (neighborOffset == 0 || 
            neighborOffset < -leftCount || 
            neighborOffset > rightCount)
        {
            return -1;
        }

        return offsetToBufferIndex(neighborOffset);
    }



    void mutate(unsigned long long mutateStep)
    {
        // Mutation
        unsigned long long population = currentANN.population;
        unsigned long long actualNeighbors = getActualNeighborCount();
        Synapse* synapses = currentANN.synapses;

        // Randomly pick a synapse, randomly increase or decrease its weight by 1 or -1
        unsigned long long synapseMutation = initValue.synpaseMutation[mutateStep];
        unsigned long long totalValidSynapses = population * actualNeighbors;
        unsigned long long flatIdx = (synapseMutation >> 1) % totalValidSynapses;

        // Convert flat index to (neuronIdx, local synapse index within valid range)
        unsigned long long neuronIdx = flatIdx / actualNeighbors;
        unsigned long long localSynapseIdx = flatIdx % actualNeighbors;

        // Convert to synapse buffer index that have bigger range
        unsigned long long synapseIndex = localSynapseIdx + getSynapseStartIndex();
        unsigned long long synapseFullBufferIdx = neuronIdx * maxNumberOfNeighbors + synapseIndex;

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

        char newWeight = synapses[synapseFullBufferIdx].weight + weightChange;

        // Valid weight. Update it
        if (newWeight >= -1 && newWeight <= 1)
        {
            synapses[synapseFullBufferIdx].weight = newWeight;
        }
        else // Invalid weight. Insert a neuron
        {
            // Insert the neuron
            insertNeuron(neuronIdx, synapseIndex);
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
        return &currentANN.synapses[neuronIndex * maxNumberOfNeighbors];
    }

    // Calculate the new neuron index that is reached by moving from the given `neuronIdx` `value`
    // neurons to the right or left. Negative `value` moves to the left, positive `value` moves to
    // the right. The return value is clamped in a ring buffer fashion, i.e. moving right of the
    // rightmost neuron continues at the leftmost neuron.
    unsigned long long clampNeuronIndex(long long neuronIdx, long long value)
    {
        unsigned long long population = currentANN.population;
        assert(value > -(long long)population && value < (long long)population 
           && "clampNeuronIndex: |value| must be less than population");

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
        long long leftCount = (long long)getLeftNeighborCount();
        long long rightCount = (long long)getRightNeighborCount();
        unsigned long long startSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long endSynapseBufferIdx = getSynapseEndIndex();

        // Scan all its neighbor to remove their outgoing synapse point to the neuron
        for (long long neighborOffset = -leftCount; neighborOffset <= rightCount; neighborOffset++)
        {
            if (neighborOffset == 0) continue;

            unsigned long long nnIdx = clampNeuronIndex(neuronIdx, neighborOffset);
            Synapse* pNNSynapses = getSynapses(nnIdx);

            long long synapseIndexOfNN = getIndexInSynapsesBuffer(-neighborOffset);
            if (synapseIndexOfNN < 0)
            {
                continue;
            }

            // The synapse array need to be shifted regard to the remove neuron
            // Also neuron need to have 2M neighbors, the addtional synapse will be set as zero
            // weight Case1 [S0 S1 S2 - SR S5 S6]. SR is removed, [S0 S1 S2 S5 S6 0] Case2 [S0 S1 SR
            // - S3 S4 S5]. SR is removed, [0 S0 S1 S3 S4 S5]
            constexpr unsigned long long halfMax = maxNumberOfNeighbors / 2;
            if (synapseIndexOfNN >= (long long)halfMax)
            {
                for (long long k = synapseIndexOfNN; k < (long long)endSynapseBufferIdx - 1; ++k)
                {
                    pNNSynapses[k] = pNNSynapses[k + 1];
                }
                pNNSynapses[endSynapseBufferIdx - 1].weight = 0;
            }
            else
            {
                for (long long k = synapseIndexOfNN; k > (long long)startSynapseBufferIdx; --k)
                {
                    pNNSynapses[k] = pNNSynapses[k - 1];
                }
                pNNSynapses[startSynapseBufferIdx].weight = 0;
            }
        }

        // Shift the synapse array and the neuron array
        for (unsigned long long shiftIdx = neuronIdx; shiftIdx < currentANN.population - 1; shiftIdx++)
        {
            currentANN.neurons[shiftIdx] = currentANN.neurons[shiftIdx + 1];

            // Also shift the synapses
            memcpy(
                getSynapses(shiftIdx),
                getSynapses(shiftIdx + 1),
                maxNumberOfNeighbors * sizeof(Synapse));
        }
        currentANN.population--;
    }

    unsigned long long
    getNeighborNeuronIndex(unsigned long long neuronIndex, unsigned long long neighborOffset)
    {
        const unsigned long long leftNeighbors = getLeftNeighborCount();
        unsigned long long nnIndex = 0;
        if (neighborOffset < leftNeighbors)
        {
            nnIndex = clampNeuronIndex(
                neuronIndex + neighborOffset, -(long long)leftNeighbors);
        }
        else
        {
            nnIndex = clampNeuronIndex(
                neuronIndex + neighborOffset + 1, -(long long)leftNeighbors);
        }
        return nnIndex;
    }

    void insertNeuron(unsigned long long neuronIndex, unsigned long long synapseIndex)
    {
        unsigned long long synapseFullBufferIdx = neuronIndex * maxNumberOfNeighbors + synapseIndex;
        // Old value before insert neuron
        unsigned long long oldStartSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long oldEndSynapseBufferIdx = getSynapseEndIndex();
        unsigned long long oldActualNeighbors = getActualNeighborCount();
        long long oldLeftCount = (long long)getLeftNeighborCount();
        long long oldRightCount = (long long)getRightNeighborCount();

        constexpr unsigned long long halfMax = maxNumberOfNeighbors / 2;

        // Validate synapse index is within valid range
        assert(synapseIndex >= oldStartSynapseBufferIdx && synapseIndex < oldEndSynapseBufferIdx);

        Synapse* synapses = currentANN.synapses;
        Neuron* neurons = currentANN.neurons;
        unsigned long long& population = currentANN.population;

        // Copy original neuron to the inserted one and set it as  Neuron::kEvolution type
        Neuron insertNeuron;
        insertNeuron = neurons[neuronIndex];
        insertNeuron.type = Neuron::kEvolution;
        unsigned long long insertedNeuronIdx = neuronIndex + 1;

        char originalWeight = synapses[synapseFullBufferIdx].weight;

        // Insert the neuron into array, population increased one, all neurons next to original one
        // need to shift right
        for (unsigned long long i = population; i > neuronIndex; --i)
        {
            neurons[i] = neurons[i - 1];

            // Also shift the synapses to the right
            memcpy(getSynapses(i), getSynapses(i - 1), maxNumberOfNeighbors * sizeof(Synapse));
        }
        neurons[insertedNeuronIdx] = insertNeuron;
        population++;

        // Recalculate after population change
        unsigned long long newActualNeighbors = getActualNeighborCount();
        unsigned long long newStartSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long newEndSynapseBufferIdx = getSynapseEndIndex();

        // Try to update the synapse of inserted neuron. All outgoing synapse is init as zero weight
        Synapse* pInsertNeuronSynapse = getSynapses(insertedNeuronIdx);
        for (unsigned long long synIdx = 0; synIdx < maxNumberOfNeighbors; ++synIdx)
        {
            pInsertNeuronSynapse[synIdx].weight = 0;
        }

        // Copy the outgoing synapse of original neuron
        if (synapseIndex < halfMax)
        {
            // The synapse is going to a neuron to the left of the original neuron.
            // Check if the incoming neuron is still contained in the neighbors of the inserted
            // neuron. This is the case if the original `synapseIndex` is > 0, i.e.
            // the original synapse if not going to the leftmost neighbor of the original neuron.
            if (synapseIndex > newStartSynapseBufferIdx)
            {
                // Decrease idx by one because the new neuron is inserted directly to the right of
                // the original one.
                pInsertNeuronSynapse[synapseIndex - 1].weight = originalWeight;
            }
            // If the incoming neuron of the original synapse if not contained in the neighbors of
            // the inserted neuron, don't add the synapse.
        }
        else
        {
            // The synapse is going to a neuron to the right of the original neuron.
            // In this case, the incoming neuron of the synapse is for sure contained in the
            // neighbors of the inserted neuron and has the same idx (right side neighbors of
            // inserted neuron = right side neighbors of original neuron before insertion).
            pInsertNeuronSynapse[synapseIndex].weight = originalWeight;
        }

        // The change of synapse only impact neuron in [originalNeuronIdx - actualNeighbors / 2
        // + 1, originalNeuronIdx +  actualNeighbors / 2] In the new index, it will be
        // [originalNeuronIdx + 1 - actualNeighbors / 2, originalNeuronIdx + 1 +
        // actualNeighbors / 2] [N0 N1 N2 original inserted N4 N5 N6], M = 2.
        for (long long delta = -oldLeftCount; delta <= oldRightCount; ++delta)
        {
            // Only process the neighbors
            if (delta == 0)
            {
                continue;
            }
            unsigned long long updatedNeuronIdx = clampNeuronIndex(insertedNeuronIdx, delta);

            // Generate a list of neighbor index of current updated neuron NN
            // Find the location of the inserted neuron in the list of neighbors
            long long insertedNeuronIdxInNeigborList = -1;
            for (unsigned long long k = 0; k < newActualNeighbors; k++)
            {
                unsigned long long nnIndex = getNeighborNeuronIndex(updatedNeuronIdx, k);
                if (nnIndex == insertedNeuronIdx)
                {
                    insertedNeuronIdxInNeigborList = (long long)(newStartSynapseBufferIdx + k);
                }
            }

            assert(insertedNeuronIdxInNeigborList >= 0);

            Synapse* pUpdatedSynapses = getSynapses(updatedNeuronIdx);
            // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
            // Case: neurons in range [N0 N1 N2 original], right synapses will be affected
            if (delta < 0)
            {
                // Left side is kept as it is, only need to shift to the right side
                for (long long k = (long long)newEndSynapseBufferIdx - 1; k >= insertedNeuronIdxInNeigborList; --k)
                {
                    // Updated synapse
                    pUpdatedSynapses[k] = pUpdatedSynapses[k - 1];
                }

                // Incomming synapse from original neuron -> inserted neuron must be zero
                if (delta == -1)
                {
                    pUpdatedSynapses[insertedNeuronIdxInNeigborList].weight = 0;
                }
            }
            else // Case: neurons in range [inserted N4 N5 N6], left synapses will be affected
            {
                // Right side is kept as it is, only need to shift to the left side
                for (long long k = (long long)newStartSynapseBufferIdx; k < insertedNeuronIdxInNeigborList; ++k)
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

        unsigned long long startSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long endSynapseBufferIdx = getSynapseEndIndex();
        long long leftCount = (long long)getLeftNeighborCount();
        long long rightCount = (long long)getRightNeighborCount();

        unsigned long long numberOfRedundantNeurons = 0;
        // After each mutation, we must verify if there are neurons that do not affect the ANN
        // output. These are neurons that either have all incoming synapse weights as 0, or all
        // outgoing synapse weights as 0. Such neurons must be removed.
        for (unsigned long long i = 0; i < population; i++)
        {
            neurons[i].markForRemoval = false;
            if (neurons[i].type == Neuron::kEvolution)
            {
                bool allOutGoingZeros = true;
                bool allIncommingZeros = true;

                // Loop though its synapses for checkout outgoing synapses
                for (unsigned long long m = startSynapseBufferIdx; m < endSynapseBufferIdx; m++)
                {
                    char synapseW = synapses[i * maxNumberOfNeighbors + m].weight;
                    if (synapseW != 0)
                    {
                        allOutGoingZeros = false;
                        break;
                    }
                }

                // Loop through the neighbor neurons to check all incoming synapses
                for (long long offset = -leftCount; offset <= rightCount; offset++)
                {
                    if (offset == 0) continue;

                    unsigned long long nnIdx = clampNeuronIndex(i, offset);
                    long long synapseIdx = getIndexInSynapsesBuffer(-offset);
                    if (synapseIdx < 0)
                    {
                        continue;
                    }
                    char synapseW = getSynapses(nnIdx)[synapseIdx].weight;

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
        Neuron* neurons = currentANN.neurons;

        // Memset value of current one
        memset(neuronValueBuffer, 0, sizeof(neuronValueBuffer));

        // Loop though all neurons
        unsigned long long startSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long endSynapseBufferIdx = getSynapseEndIndex();

        for (unsigned long long n = 0; n < population; ++n)
        {
            const Synapse* kSynapses = getSynapses(n);
            long long neuronValue = neurons[n].value;
            // Scan through all neighbor neurons and sum all connected neurons.
            for (unsigned long long m = startSynapseBufferIdx; m < endSynapseBufferIdx; m++)
            {
                char synapseWeight = kSynapses[m].weight;
                long long offset = bufferIndexToOffset(m);
                unsigned long long nnIndex = clampNeuronIndex(static_cast<long long>(n), offset);

                // Weight-sum
                neuronValueBuffer[nnIndex] += synapseWeight * neuronValue;
            }
        }

        // Clamp the neuron value
        for (unsigned long long n = 0; n < population; ++n)
        {
            // Only non input neurons are updated
            if (Neuron::kInput != neurons[n].type)
            {
                char neuronValue = score_reference::clampNeuron(neuronValueBuffer[n]);
                neurons[n].value = neuronValue;
            }
        }
    }
    void loadTrainingData(unsigned long long trainingIndex)
    {
        unsigned long long population = currentANN.population;
        Neuron* neurons = currentANN.neurons;

        const auto& data = trainingSet[trainingIndex];
        // Load the input neuron value
        unsigned long long inputIndex = 0;
        for (unsigned long long n = 0; n < population; ++n)
        {
            // Init as zeros
            neurons[n].value = 0;
            if (Neuron::kInput == neurons[n].type)
            {
                neurons[n].value = data.input[inputIndex];
                inputIndex++;
            }
        }

        // Load the expected output value
        memcpy(outputNeuronExpectedValue, data.output, sizeof(outputNeuronExpectedValue[0]) * numberOfOutputNeurons);
    }
    // Tick simulation only runs on one ANN
    void runTickSimulation(unsigned long long trainingIndex)
    {
        unsigned long long population = currentANN.population;
        Neuron* neurons = currentANN.neurons;

        // Load the training set and fill ANN value
        loadTrainingData(trainingIndex);

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
            bool allNeuronsUnchanged = true;
            bool allOutputNeuronsIsNonZeros = true;
            for (unsigned long long n = 0; n < population; ++n)
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
            for (unsigned long long n = 0; n < population; ++n)
            {
                previousNeuronValue[n] = neurons[n].value;
            }
        }
    }

    unsigned int computeMatchingOutput()
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
                if (neurons[i].value == outputNeuronExpectedValue[outputIdx])
                {
                    R++;
                }
                outputIdx++;
            }
        }
        return R;
    }

    // Generate all 2^K possible (A, B, C) pairs
    void generateTrainingSet()
    {
        static constexpr long long boundValue = (1LL << (numberOfInputNeurons / 2)) / 2;
        unsigned long long index = 0;
        for (long long A = -boundValue; A < boundValue; A++)
        {
            for (long long B = -boundValue; B < boundValue; B++)
            {
                long long C = A + B;

                score_reference::toTenaryBits<numberOfInputNeurons / 2>(A, trainingSet[index].input);
                score_reference::toTenaryBits<numberOfInputNeurons / 2>(
                    B, trainingSet[index].input + numberOfInputNeurons / 2);
                score_reference::toTenaryBits<numberOfOutputNeurons>(C, trainingSet[index].output);
                index++;
            }
        }
    }

    unsigned int inferANN()
    {
        unsigned int score = 0;
        for (unsigned long long i = 0; i < trainingSetSize; ++i)
        {
            // Ticks simulation
            runTickSimulation(i);

            // Compute R
            unsigned int R = computeMatchingOutput();
            score += R;
        }
        return score;
    }

    unsigned int initializeANN(const unsigned char* publicKey, const unsigned char* nonce)
    {
        unsigned char hash[32];
        unsigned char combined[64];
        memcpy(combined, publicKey, 32);
        memcpy(combined + 32, nonce, 32);
        KangarooTwelve(combined, 64, hash, 32);

        unsigned long long& population = currentANN.population;
        Synapse* synapses = currentANN.synapses;
        Neuron* neurons = currentANN.neurons;

        // Initialization
        population = numberOfNeurons;

        // Generate all 2^K possible (A, B, C) pairs
        generateTrainingSet();

        // Initalize with nonce and public key
        score_reference::random2(hash, poolVec.data(), (unsigned char*)&initValue, sizeof(InitValue));

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

            // This index is used, copy the end of indices array to current position and decrease
            // the number of picking neurons
            neuronCount = neuronCount - 1;
            neuronIndices[outputNeuronIdx] = neuronIndices[neuronCount];
        }

        // Synapse weight initialization
        auto extractWeight = [](unsigned long long packedValue, unsigned long long position) -> char {
            unsigned char extractValue = static_cast<unsigned char>((packedValue >> (position * 2)) & 0b11);
            switch (extractValue)
            {
                case 2:
                    return -1;
                case 3:
                    return 1;
                default:
                    return 0;
            }
        };
        for (unsigned long long i = 0; i < (maxNumberOfSynapses / 32); ++i)
        {
            for (unsigned long long j = 0; j < 32; ++j)
            {
                synapses[32 * i + j].weight = extractWeight(initValue.synapseWeight[i], j);
            }
        }

        // Handle remaining synapses (if maxNumberOfSynapses not divisible by 32)
        unsigned long long remainder = maxNumberOfSynapses % 32;
        if (remainder > 0)
        {
            unsigned long long lastBlock = maxNumberOfSynapses / 32;
            for (unsigned long long j = 0; j < remainder; ++j)
            {
                synapses[32 * lastBlock + j].weight = extractWeight(initValue.synapseWeight[lastBlock], j);
            }
        }

        // Run the first inference to get starting point before mutation
        unsigned int score = inferANN();

        return score;
    }

    // Main function for mining
    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce)
    {
        // Initialize
        unsigned int bestR = initializeANN(publicKey, nonce);
        memcpy(&bestANN, &currentANN, sizeof(bestANN));

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
            unsigned int R = inferANN();

            // Roll back if neccessary
            if (R >= bestR)
            {
                bestR = R;
                // Better R. Save the state
                memcpy(&bestANN, &currentANN, sizeof(bestANN));
            }
            else
            {
                // Roll back
                memcpy(&currentANN, &bestANN, sizeof(bestANN));
            }

            assert(bestANN.population <= populationThreshold);
        }
        return bestR;
    }

    bool findSolution(const unsigned char* publicKey, const unsigned char* nonce)
    {
        unsigned int score = computeScore(publicKey, nonce);
        if (score >= solutionThreshold)
        {
            return true;
        }

        return false;
    }
};

} // namespace score_addition
