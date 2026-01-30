#pragma once

#include "score_common.h"

namespace score_engine
{

template <typename Params>
struct ScoreHyperIdentity
{
    // Convert params for easier usage
    static constexpr unsigned long long numberOfInputNeurons = Params::numberOfInputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = Params::numberOfOutputNeurons;
    static constexpr unsigned long long numberOfTicks = Params::numberOfTicks;
    static constexpr unsigned long long numberOfNeighbors = Params::numberOfNeighbors;
    static constexpr unsigned long long populationThreshold = Params::populationThreshold;
    static constexpr unsigned long long numberOfMutations = Params::numberOfMutations;
    static constexpr unsigned int solutionThreshold = Params::solutionThreshold;

    // Computation params
    static constexpr unsigned long long numberOfNeurons = numberOfInputNeurons + numberOfOutputNeurons;
    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long maxNumberOfSynapses = populationThreshold * numberOfNeighbors;
    static constexpr unsigned long long initNumberOfSynapses = numberOfNeurons * numberOfNeighbors;
    static constexpr long long radius = (long long)numberOfNeighbors / 2;
    static constexpr long long paddingNeuronsCount = (maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE;
    static constexpr unsigned long long incommingSynapsesPitch = ((numberOfNeighbors + 1) + BATCH_SIZE_X8 - 1) / BATCH_SIZE_X8 * BATCH_SIZE_X8;
    static constexpr unsigned long long incommingSynapseBatchSize = incommingSynapsesPitch >> 3;

    static_assert(numberOfInputNeurons % 64 == 0, "numberOfInputNeurons must be divided by 64");
    static_assert(numberOfOutputNeurons % 64 == 0, "numberOfOutputNeurons must be divided by 64");
    static_assert(maxNumberOfSynapses <= (0xFFFFFFFFFFFFFFFF << 1ULL), "maxNumberOfSynapses must less than or equal MAX_UINT64/2");
    static_assert(initNumberOfSynapses % 32 == 0, "initNumberOfSynapses must be divided by 32");
    static_assert(numberOfNeighbors % 2 == 0, "numberOfNeighbors must be divided by 2");
    static_assert(populationThreshold > numberOfNeurons, "populationThreshold must be greater than numberOfNeurons");
    static_assert(numberOfNeurons > numberOfNeighbors, "Number of neurons must be greater than the number of neighbors");
    static_assert(numberOfNeighbors < ((1ULL << 63) - 1), "numberOfNeighbors must be in long long range");
    static_assert(BATCH_SIZE_X8 % 8 == 0, "BATCH_SIZE must be be divided by 8");


    // Intermediate data
    struct InitValue
    {
        unsigned long long outputNeuronPositions[numberOfOutputNeurons];
        unsigned long long synapseWeight[initNumberOfSynapses / 32]; // each 64bits elements will decide value of 32 synapses
        unsigned long long synpaseMutation[numberOfMutations];
    };

    struct MiningData
    {
        unsigned long long inputNeuronRandomNumber[numberOfInputNeurons / 64];  // each bit will use for generate input neuron value
        unsigned long long outputNeuronRandomNumber[numberOfOutputNeurons / 64]; // each bit will use for generate expected output neuron value
    };
    static constexpr unsigned long long paddingInitValueSizeInBytes = (sizeof(InitValue) + 64 - 1) / 64 * 64;

    void initMemory() {}

    typedef char Synapse;
    typedef char Neuron;
    typedef unsigned char NeuronType;

    // Data for roll back
    struct ANN
    {
        void init()
        {
            neurons = paddingNeurons + radius;
        }
        void prepareData()
        {
            // Padding start and end of neuron array
            neurons = paddingNeurons + radius;
        }

        void copyDataTo(ANN& rOther)
        {
            copyMem(rOther.neurons, neurons, population * sizeof(Neuron));
            copyMem(rOther.neuronTypes, neuronTypes, population * sizeof(NeuronType));
            copyMem(rOther.synapses, synapses, maxNumberOfSynapses * sizeof(Synapse));
            rOther.population = population;
        }

        Neuron* neurons;
        // Padding start and end of neurons so that we can reduce the condition checking
        Neuron paddingNeurons[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE];
        NeuronType neuronTypes[(maxNumberOfNeurons + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE];
        Synapse synapses[maxNumberOfSynapses];

        // Encoded data
        unsigned char neuronPlus1s[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE + BATCH_SIZE_X8];
        unsigned char neuronMinus1s[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE + BATCH_SIZE_X8];

        unsigned char nextNeuronPlus1s[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE + BATCH_SIZE_X8];
        unsigned char nextneuronMinus1s[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE + BATCH_SIZE_X8];

        unsigned char synapsePlus1s[incommingSynapsesPitch * populationThreshold];
        unsigned char synapseMinus1s[incommingSynapsesPitch * populationThreshold];


        unsigned long long population;
    };
    ANN bestANN;
    ANN currentANN;

    // Intermediate data
    unsigned char paddingInitValue[paddingInitValueSizeInBytes];
    MiningData miningData;

    unsigned long long neuronIndices[numberOfNeurons];
    Neuron previousNeuronValue[maxNumberOfNeurons];

    Neuron outputNeuronExpectedValue[numberOfOutputNeurons];

    Neuron neuronValueBuffer[maxNumberOfNeurons];
    unsigned char hash[32];
    unsigned char combined[64];

    unsigned long long removalNeuronsCount;

    // Contain incomming synapse of neurons. The center one will be zeros
    Synapse incommingSynapses[maxNumberOfNeurons * incommingSynapsesPitch];

    // Padding to fix bytes for each row
    Synapse paddingIncommingSynapses[populationThreshold * incommingSynapsesPitch];

    unsigned char nextNeuronPlus1s[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE];
    unsigned char nextNeuronMinus1s[(maxNumberOfNeurons + numberOfNeighbors + BATCH_SIZE - 1) / BATCH_SIZE * BATCH_SIZE];

    unsigned char keptNeurons[maxNumberOfNeurons];
    unsigned long long affectedNeurons[maxNumberOfNeurons];
    unsigned long long nextAffectedNeurons[maxNumberOfNeurons];

    void mutate(unsigned long long mutateStep)
    {
        // Mutation
        unsigned long long population = currentANN.population;
        unsigned long long synapseCount = population * numberOfNeighbors;
        Synapse* synapses = currentANN.synapses;
        InitValue* initValue = (InitValue*)paddingInitValue;

        // Randomly pick a synapse, randomly increase or decrease its weight by 1 or -1
        unsigned long long synapseMutation = initValue->synpaseMutation[mutateStep];
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

        char newWeight = synapses[synapseIdx] + weightChange;

        // Valid weight. Update it
        if (newWeight >= -1 && newWeight <= 1)
        {
            synapses[synapseIdx] = newWeight;
        }
        else // Invalid weight. Insert a neuron
        {
            // Insert the neuron
            insertNeuron(synapseIdx);
        }
        // Clean the ANN
        cleanANN();
    }

    // Get the pointer to all outgoing synapse of a neurons
    Synapse* getSynapses(unsigned long long neuronIndex)
    {
        return &currentANN.synapses[neuronIndex * numberOfNeighbors];
    }

    // Circulate the neuron index
    unsigned long long clampNeuronIndex(long long neuronIdx, long long value)
    {
        const long long population = (long long)currentANN.population;
        long long nnIndex = neuronIdx + value;

        // Get the signed bit and decide if we should increase population
        nnIndex += (population & (nnIndex >> 63));

        // Subtract population if idx >= population
        long long over = nnIndex - population;
        nnIndex -= (population & ~(over >> 63));
        return (unsigned long long)nnIndex;
    }

    // Remove a neuron and all synapses relate to it
    void removeNeuron(unsigned long long neuronIdx)
    {
        // Scan all its neigbor to remove their outgoing synapse point to the neuron
        for (long long neighborOffset = -(long long)numberOfNeighbors / 2; neighborOffset <= (long long)numberOfNeighbors / 2; neighborOffset++)
        {
            unsigned long long nnIdx = clampNeuronIndex(neuronIdx, neighborOffset);
            Synapse* pNNSynapses = getSynapses(nnIdx);

            long long synapseIndexOfNN = getIndexInSynapsesBuffer(nnIdx, -neighborOffset);
            if (synapseIndexOfNN < 0)
            {
                continue;
            }

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
                pNNSynapses[numberOfNeighbors - 1] = 0;
            }
            else
            {
                for (long long k = synapseIndexOfNN; k > 0; --k)
                {
                    pNNSynapses[k] = pNNSynapses[k - 1];
                }
                pNNSynapses[0] = 0;
            }
        }

        // Shift the synapse array and the neuron array, also reduce the current ANN population
        currentANN.population--;
        for (unsigned long long shiftIdx = neuronIdx; shiftIdx < currentANN.population; shiftIdx++)
        {
            currentANN.neurons[shiftIdx] = currentANN.neurons[shiftIdx + 1];
            currentANN.neuronTypes[shiftIdx] = currentANN.neuronTypes[shiftIdx + 1];
            keptNeurons[shiftIdx] = keptNeurons[shiftIdx + 1];

            // Also shift the synapses
            copyMem(getSynapses(shiftIdx), getSynapses(shiftIdx + 1), numberOfNeighbors * sizeof(Synapse));
        }
    }

    unsigned long long getNeighborNeuronIndex(unsigned long long neuronIndex, unsigned long long neighborOffset)
    {
        unsigned long long nnIndex = 0;
        if (neighborOffset < (numberOfNeighbors / 2))
        {
            nnIndex = clampNeuronIndex(neuronIndex + neighborOffset, -(long long)numberOfNeighbors / 2);
        }
        else
        {
            nnIndex = clampNeuronIndex(neuronIndex + neighborOffset + 1, -(long long)numberOfNeighbors / 2);
        }
        return nnIndex;
    }

    void updateSynapseOfInsertedNN(unsigned long long insertedNeuronIdx)
    {
        // The change of synapse only impact neuron in [originalNeuronIdx - numberOfNeighbors / 2 + 1, originalNeuronIdx +  numberOfNeighbors / 2]
        // In the new index, it will be  [originalNeuronIdx + 1 - numberOfNeighbors / 2, originalNeuronIdx + 1 + numberOfNeighbors / 2]
        // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
        for (long long delta = -(long long)numberOfNeighbors / 2; delta <= (long long)numberOfNeighbors / 2; ++delta)
        {
            // Only process the neigbors
            if (delta == 0)
            {
                continue;
            }
            unsigned long long updatedNeuronIdx = clampNeuronIndex(insertedNeuronIdx, delta);

            // Generate a list of neighbor index of current updated neuron NN
            // Find the location of the inserted neuron in the list of neighbors
            long long insertedNeuronIdxInNeigborList = -1;
            for (long long k = 0; k < numberOfNeighbors; k++)
            {
                unsigned long long nnIndex = getNeighborNeuronIndex(updatedNeuronIdx, k);
                if (nnIndex == insertedNeuronIdx)
                {
                    insertedNeuronIdxInNeigborList = k;
                }
            }

            ASSERT(insertedNeuronIdxInNeigborList >= 0);

            Synapse* pUpdatedSynapses = getSynapses(updatedNeuronIdx);
            // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
            // Case: neurons in range [N0 N1 N2 original], right synapses will be affected
            if (delta < 0)
            {
                // Left side is kept as it is, only need to shift to the right side
                for (long long k = numberOfNeighbors - 1; k >= insertedNeuronIdxInNeigborList; --k)
                {
                    // Updated synapse
                    pUpdatedSynapses[k] = pUpdatedSynapses[k - 1];
                }

                // Incomming synapse from original neuron -> inserted neuron must be zero
                if (delta == -1)
                {
                    pUpdatedSynapses[insertedNeuronIdxInNeigborList] = 0;
                }
            }
            else // Case: neurons in range [inserted N4 N5 N6], left synapses will be affected
            {
                // Right side is kept as it is, only need to shift to the left side
                for (long long k = 0; k < insertedNeuronIdxInNeigborList; ++k)
                {
                    // Updated synapse
                    pUpdatedSynapses[k] = pUpdatedSynapses[k + 1];
                }
            }

        }
    }

    void insertNeuron(unsigned long long synapseIdx)
    {
        // A synapse have incomingNeighbor and outgoingNeuron, direction incomingNeuron -> outgoingNeuron
        unsigned long long incomingNeighborSynapseIdx = synapseIdx % numberOfNeighbors;
        unsigned long long outgoingNeuron = synapseIdx / numberOfNeighbors;

        Synapse* synapses = currentANN.synapses;
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;
        unsigned long long& population = currentANN.population;

        // Copy original neuron to the inserted one and set it as  EVOLUTION_NEURON_TYPE type
        Neuron insertNeuron = neurons[outgoingNeuron];
        unsigned long long insertedNeuronIdx = outgoingNeuron + 1;

        Synapse originalWeight = synapses[synapseIdx];

        // Insert the neuron into array, population increased one, all neurons next to original one need to shift right
        for (unsigned long long i = population; i > outgoingNeuron; --i)
        {
            neurons[i] = neurons[i - 1];
            neuronTypes[i] = neuronTypes[i - 1];

            // Also shift the synapses to the right
            copyMem(getSynapses(i), getSynapses(i - 1), numberOfNeighbors * sizeof(Synapse));
        }
        neurons[insertedNeuronIdx] = insertNeuron;
        neuronTypes[insertedNeuronIdx] = EVOLUTION_NEURON_TYPE;
        population++;

        // Try to update the synapse of inserted neuron. All outgoing synapse is init as zero weight
        Synapse* pInsertNeuronSynapse = getSynapses(insertedNeuronIdx);
        for (unsigned long long synIdx = 0; synIdx < numberOfNeighbors; ++synIdx)
        {
            pInsertNeuronSynapse[synIdx] = 0;
        }

        // Copy the outgoing synapse of original neuron
        // Outgoing points to the left
        if (incomingNeighborSynapseIdx < numberOfNeighbors / 2)
        {
            if (incomingNeighborSynapseIdx > 0)
            {
                // Decrease by one because the new neuron is next to the original one
                pInsertNeuronSynapse[incomingNeighborSynapseIdx - 1] = originalWeight;
            }
            // Incase of the outgoing synapse point too far, don't add the synapse
        }
        else
        {
            // No need to adjust the added neuron but need to remove the synapse of the original neuron
            pInsertNeuronSynapse[incomingNeighborSynapseIdx] = originalWeight;
        }

        updateSynapseOfInsertedNN(insertedNeuronIdx);
    }

    long long getIndexInSynapsesBuffer(unsigned long long neuronIdx, long long neighborOffset)
    {
        // Skip the case neuron point to itself and too far neighbor
        if (neighborOffset == 0
            || neighborOffset < -(long long)numberOfNeighbors / 2
            || neighborOffset >(long long)numberOfNeighbors / 2)
        {
            return -1;
        }

        long long synapseIdx = (long long)numberOfNeighbors / 2 + neighborOffset;
        if (neighborOffset >= 0)
        {
            synapseIdx = synapseIdx - 1;
        }

        return synapseIdx;
    }

    bool isAllOutgoingSynapsesZeros(unsigned long long neuronIdx)
    {
        Synapse* synapse = getSynapses(neuronIdx);
        for (unsigned long long n = 0; n < numberOfNeighbors; n++)
        {
            Synapse synapseW = synapse[n];
            if (synapseW != 0)
            {
                return false;
            }
        }
        return true;
    }

    bool isAllIncomingSynapsesZeros(unsigned long long neuronIdx)
    {
        // Loop through the neighbor neurons to check all incoming synapses
        for (long long neighborOffset = -(long long)numberOfNeighbors / 2; neighborOffset <= (long long)numberOfNeighbors / 2; neighborOffset++)
        {
            unsigned long long nnIdx = clampNeuronIndex(neuronIdx, neighborOffset);
            Synapse* nnSynapses = getSynapses(nnIdx);

            long long synapseIdx = getIndexInSynapsesBuffer(nnIdx, -neighborOffset);
            if (synapseIdx < 0)
            {
                continue;
            }
            Synapse synapseW = nnSynapses[synapseIdx];

            if (synapseW != 0)
            {
                return false;
            }
        }
        return true;
    }

    // Check which neurons/synapse need to be removed after mutation
    bool scanRedundantNeurons()
    {
        bool isStructureChanged = false;
        unsigned long long population = currentANN.population;
        Synapse* synapses = currentANN.synapses;
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;

        unsigned long long affectedCount = 0;
        unsigned long long nextCount = 0;
        setMem(keptNeurons, sizeof(keptNeurons), 255);

        // First scan
        for (unsigned long long i = 0; i < population; i++)
        {
            if (neuronTypes[i] != EVOLUTION_NEURON_TYPE)
            {
                continue;
            }

            if (isAllOutgoingSynapsesZeros(i) || isAllIncomingSynapsesZeros(i))
            {
                keptNeurons[i] = 0;
                affectedNeurons[affectedCount++] = i;
                isStructureChanged = true;
            }
        }

        while (affectedCount > 0)
        {
            nextCount = 0;
            for (unsigned long long affectedIndex = 0; affectedIndex < affectedCount; affectedIndex++)
            {
                unsigned long long i = affectedNeurons[affectedIndex];
                // Mark the neuron for removal and set all its incoming and outgoing synapse weights to zero.
                // This action isolates the neuron, allowing adjacent neurons to be considered for removal in the next iteration.

                // Remove outgoing synapse
                setMem(getSynapses(i), numberOfNeighbors * sizeof(Synapse), 0);

                // Scan all its neigbor to remove their outgoing synapse point to the neuron aka incomming synapses of this neuron
                for (long long neighborOffset = -(long long)numberOfNeighbors / 2; neighborOffset <= (long long)numberOfNeighbors / 2; neighborOffset++)
                {
                    unsigned long long nnIdx = clampNeuronIndex(i, neighborOffset);
                    Synapse* pNNSynapses = getSynapses(nnIdx);

                    long long synapseIndexOfNN = getIndexInSynapsesBuffer(nnIdx, -neighborOffset);
                    if (synapseIndexOfNN < 0)
                    {
                        continue;
                    }

                    // Synapse to this i neurons is marked as zero/aka disconnected
                    if (pNNSynapses[synapseIndexOfNN] != 0)
                    {
                        pNNSynapses[synapseIndexOfNN] = 0;

                        // This neuron is not marked as removal yet, record it
                        if (keptNeurons[nnIdx])
                        {
                            nextAffectedNeurons[nextCount++] = nnIdx;
                        }
                    }
                }

                // Mark the neurons as removal
                affectedCount = 0;
                for (unsigned long long k = 0; k < nextCount; ++k)
                {
                    unsigned long long idx = nextAffectedNeurons[k];

                    // Skip already removed or non-evolution neurons
                    if (!keptNeurons[idx] || neuronTypes[idx] != EVOLUTION_NEURON_TYPE)
                        continue;

                    // Check if these neurons are needed to be removed
                    if (isAllOutgoingSynapsesZeros(idx) || isAllIncomingSynapsesZeros(idx))
                    {
                        keptNeurons[idx] = 0;
                        affectedNeurons[affectedCount++] = idx;
                        isStructureChanged = true;
                    }
                }
            }
        }

        return isStructureChanged;
    }

    // Remove neurons and synapses that do not affect the ANN
    void cleanANN()
    {
        // No removal. First scan and probagate to be removed neurons
        if (!scanRedundantNeurons())
        {
            return;
        }

        // Remove neurons
        unsigned long long neuronIdx = 0;
        while (neuronIdx < currentANN.population)
        {
            if (keptNeurons[neuronIdx] == 0)
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

        unsigned char* pPaddingNeuronMinus = currentANN.neuronMinus1s;
        unsigned char* pPaddingNeuronPlus = currentANN.neuronPlus1s;

        unsigned char* pPaddingSynapseMinus = currentANN.synapseMinus1s;
        unsigned char* pPaddingSynapsePlus = currentANN.synapsePlus1s;

        paddingDatabits<radius>(pPaddingNeuronMinus, population);
        paddingDatabits<radius>(pPaddingNeuronPlus, population);


#if defined (__AVX512F__)
        constexpr unsigned long long chunks = incommingSynapsesPitch >> 9;
        __m512i minusBlock[chunks];
        __m512i minusNext[chunks];
        __m512i plusBlock[chunks];
        __m512i plusNext[chunks];

        constexpr unsigned long long blockSizeNeurons = 64ULL;
        constexpr unsigned long long bytesPerWord = 8ULL;

        unsigned long long n = 0;
        const unsigned long long lastBlock = (population / blockSizeNeurons) * blockSizeNeurons;
        for (; n < lastBlock; n += blockSizeNeurons)
        {
            // byteIndex = start byte for word containing neuron n
            unsigned long long byteIndex = ((n >> 6) << 3); // (n / 64) * 8
            unsigned long long curIdx = byteIndex;
            unsigned long long nextIdx = byteIndex + bytesPerWord; // +8 bytes

            // Load the neuron windows once per block for all chunks
            unsigned long long loadCur = curIdx;
            unsigned long long loadNext = nextIdx;
            for (unsigned blk = 0; blk < chunks; ++blk, loadCur += BATCH_SIZE, loadNext += BATCH_SIZE)
            {
                plusBlock[blk] = _mm512_loadu_si512((const void*)(pPaddingNeuronPlus + loadCur));
                plusNext[blk] = _mm512_loadu_si512((const void*)(pPaddingNeuronPlus + loadNext));
                minusBlock[blk] = _mm512_loadu_si512((const void*)(pPaddingNeuronMinus + loadCur));
                minusNext[blk] = _mm512_loadu_si512((const void*)(pPaddingNeuronMinus + loadNext));
            }

            __m512i sh = _mm512_setzero_si512();
            __m512i sh64 = _mm512_set1_epi64(64);
            const __m512i ones512 = _mm512_set1_epi64(1);

            // For each neuron inside this 64-neuron block
            for (unsigned int lane = 0; lane < 64; ++lane)
            {
                const unsigned long long current_n = n + lane;
                // synapse pointers for this neuron
                unsigned char* pSynapsePlus = pPaddingSynapsePlus + current_n * incommingSynapseBatchSize;
                unsigned char* pSynapseMinus = pPaddingSynapseMinus + current_n * incommingSynapseBatchSize;

                __m512i plusPopulation = _mm512_setzero_si512();
                __m512i minusPopulation = _mm512_setzero_si512();

                for (unsigned blk = 0; blk < chunks; ++blk)
                {
                    const __m512i synP = _mm512_loadu_si512((const void*)(pSynapsePlus + blk * BATCH_SIZE));
                    const __m512i synM = _mm512_loadu_si512((const void*)(pSynapseMinus + blk * BATCH_SIZE));

                    // stitch 64-bit lanes: cur >> s | next << (64 - s)
                    __m512i neuronPlus = _mm512_or_si512(_mm512_srlv_epi64(plusBlock[blk], sh), _mm512_sllv_epi64(plusNext[blk], sh64));
                    __m512i neuronMinus = _mm512_or_si512(_mm512_srlv_epi64(minusBlock[blk], sh), _mm512_sllv_epi64(minusNext[blk], sh64));

                    __m512i tmpP = _mm512_and_si512(neuronMinus, synM);
                    const __m512i plus = _mm512_ternarylogic_epi64(neuronPlus, synP, tmpP, 234);

                    __m512i tmpM = _mm512_and_si512(neuronMinus, synP);
                    const __m512i minus = _mm512_ternarylogic_epi64(neuronPlus, synM, tmpM, 234);

                    plusPopulation = _mm512_add_epi64(plusPopulation, _mm512_popcnt_epi64(plus));
                    minusPopulation = _mm512_add_epi64(minusPopulation, _mm512_popcnt_epi64(minus));
                }
                sh = _mm512_add_epi64(sh, ones512);
                sh64 = _mm512_sub_epi64(sh64, ones512);

                // Reduce to scalar and compute neuron value
                int score = (int)_mm512_reduce_add_epi64(_mm512_sub_epi64(plusPopulation, minusPopulation));
                char neuronValue = (score > 0) - (score < 0);
                neuronValueBuffer[current_n] = neuronValue;

                // Update the neuron positive and negative bitmaps
                unsigned char nNextNeg = neuronValue < 0 ? 1 : 0;
                unsigned char nNextPos = neuronValue > 0 ? 1 : 0;
                setBitValue(currentANN.nextneuronMinus1s, current_n + radius, nNextNeg);
                setBitValue(currentANN.nextNeuronPlus1s, current_n + radius, nNextPos);
            }
        }

        for (; n < population; ++n)
        {
            char neuronValue = 0;
            int score = 0;
            unsigned char* pSynapsePlus = pPaddingSynapsePlus + n * incommingSynapseBatchSize;
            unsigned char* pSynapseMinus = pPaddingSynapseMinus + n * incommingSynapseBatchSize;

            const unsigned long long byteIndex = n >> 3;
            const unsigned int bitOffset = (n & 7U);
            const unsigned int bitOffset_8 = (8u - bitOffset);
            __m512i sh = _mm512_set1_epi64((long long)bitOffset);
            __m512i sh8 = _mm512_set1_epi64((long long)bitOffset_8);

            __m512i plusPopulation = _mm512_setzero_si512();
            __m512i minusPopulation = _mm512_setzero_si512();

            for (unsigned blk = 0; blk < chunks; ++blk, pSynapsePlus += BATCH_SIZE, pSynapseMinus += BATCH_SIZE)
            {
                const __m512i synapsePlus = _mm512_loadu_si512((const void*)(pSynapsePlus));
                const __m512i synapseMinus = _mm512_loadu_si512((const void*)(pSynapseMinus));

                __m512i neuronPlus = _mm512_loadu_si512((const void*)(pPaddingNeuronPlus + byteIndex + blk * BATCH_SIZE));
                __m512i neuronPlusNext = _mm512_loadu_si512((const void*)(pPaddingNeuronPlus + byteIndex + blk * BATCH_SIZE + 1));
                __m512i neuronMinus = _mm512_loadu_si512((const void*)(pPaddingNeuronMinus + byteIndex + blk * BATCH_SIZE));
                __m512i neuronMinusNext = _mm512_loadu_si512((const void*)(pPaddingNeuronMinus + byteIndex + blk * BATCH_SIZE + 1));

                neuronPlus = _mm512_or_si512(_mm512_srlv_epi64(neuronPlus, sh), _mm512_sllv_epi64(neuronPlusNext, sh8));
                neuronMinus = _mm512_or_si512(_mm512_srlv_epi64(neuronMinus, sh), _mm512_sllv_epi64(neuronMinusNext, sh8));

                __m512i tempP = _mm512_and_si512(neuronMinus, synapseMinus);
                const __m512i plus = _mm512_ternarylogic_epi64(neuronPlus, synapsePlus, tempP, 234);

                __m512i tempM = _mm512_and_si512(neuronMinus, synapsePlus);
                const __m512i minus = _mm512_ternarylogic_epi64(neuronPlus, synapseMinus, tempM, 234);

                tempP = _mm512_popcnt_epi64(plus);
                tempM = _mm512_popcnt_epi64(minus);
                plusPopulation = _mm512_add_epi64(tempP, plusPopulation);
                minusPopulation = _mm512_add_epi64(tempM, minusPopulation);
            }

            score = (int)_mm512_reduce_add_epi64(_mm512_sub_epi64(plusPopulation, minusPopulation));
            neuronValue = (score > 0) - (score < 0);
            neuronValueBuffer[n] = neuronValue;

            unsigned char nNextNeg = neuronValue < 0 ? 1 : 0;
            unsigned char nNextPos = neuronValue > 0 ? 1 : 0;
            setBitValue(currentANN.nextneuronMinus1s, n + radius, nNextNeg);
            setBitValue(currentANN.nextNeuronPlus1s, n + radius, nNextPos);
        }
#else
        constexpr unsigned long long chunks = incommingSynapsesPitch >> 8;
        for (unsigned long long n = 0; n < population; ++n, pPaddingSynapsePlus += incommingSynapseBatchSize, pPaddingSynapseMinus += incommingSynapseBatchSize)
        {
            char neuronValue = 0;
            int score = 0;
            unsigned char* pSynapsePlus = pPaddingSynapsePlus;
            unsigned char* pSynapseMinus = pPaddingSynapseMinus;

            int synapseBlkIdx = 0; // blk index of synapse
            int neuronBlkIdx = 0;
            for (unsigned blk = 0; blk < chunks; ++blk, synapseBlkIdx += BATCH_SIZE, neuronBlkIdx += BATCH_SIZE_X8)
            {
                // Process 256bits at once, neigbor shilf 64 bytes = 256 bits
                const __m256i synapsePlus = _mm256_loadu_si256((const __m256i*)(pPaddingSynapsePlus + synapseBlkIdx));
                const __m256i synapseMinus = _mm256_loadu_si256((const __m256i*)(pPaddingSynapseMinus + synapseBlkIdx));

                __m256i neuronPlus = load256Bits(pPaddingNeuronPlus, n + neuronBlkIdx);
                __m256i neuronMinus = load256Bits(pPaddingNeuronMinus, n + neuronBlkIdx);

                // Compare the negative and possitive parts
                __m256i plus = _mm256_or_si256(_mm256_and_si256(neuronPlus, synapsePlus),
                    _mm256_and_si256(neuronMinus, synapseMinus));
                __m256i minus = _mm256_or_si256(_mm256_and_si256(neuronPlus, synapseMinus),
                    _mm256_and_si256(neuronMinus, synapsePlus));

                score += popcnt256(plus) - popcnt256(minus);
            }

            neuronValue = (score > 0) - (score < 0);
            neuronValueBuffer[n] = neuronValue;

            // Update the neuron positive and negative
            unsigned char nNextNeg = neuronValue < 0 ? 1 : 0;
            unsigned char nNextPos = neuronValue > 0 ? 1 : 0;
            setBitValue(currentANN.nextneuronMinus1s, n + radius, nNextNeg);
            setBitValue(currentANN.nextNeuronPlus1s, n + radius, nNextPos);
        }
#endif



        copyMem(currentANN.neurons, neuronValueBuffer, population * sizeof(Neuron));
        copyMem(currentANN.neuronMinus1s, currentANN.nextneuronMinus1s, sizeof(currentANN.neuronMinus1s));
        copyMem(currentANN.neuronPlus1s, currentANN.nextNeuronPlus1s, sizeof(currentANN.neuronPlus1s));
    }

    void runTickSimulation()
    {
        unsigned long long population = currentANN.population;
        Synapse* synapses = currentANN.synapses;
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;

        // Save the neuron value for comparison
        copyMem(previousNeuronValue, neurons, population * sizeof(Neuron));
        {
            //PROFILE_NAMED_SCOPE("convertSynapse");
            // Compute the incomming synapse of each neurons
            setMem(paddingIncommingSynapses, sizeof(paddingIncommingSynapses), 0);
            for (unsigned long long n = 0; n < population; ++n)
            {
                const Synapse* kSynapses = getSynapses(n);
                // Scan through all neighbor neurons and sum all connected neurons.
                // The synapses are arranged as neuronIndex * numberOfNeighbors
                for (long long m = 0; m < radius; m++)
                {
                    Synapse synapseWeight = kSynapses[m];
                    unsigned long long nnIndex = clampNeuronIndex(n + m, -radius);
                    paddingIncommingSynapses[nnIndex * incommingSynapsesPitch + (numberOfNeighbors - m)] = synapseWeight;
                }

                //paddingIncommingSynapses[n * incommingSynapsesPitch + radius] = 0;

                for (long long m = radius; m < numberOfNeighbors; m++)
                {
                    Synapse synapseWeight = kSynapses[m];
                    unsigned long long nnIndex = clampNeuronIndex(n + m + 1, -radius);
                    paddingIncommingSynapses[nnIndex * incommingSynapsesPitch + (numberOfNeighbors - m - 1)] = synapseWeight;
                }
            }
        }

        // Prepare masks
        {
            //PROFILE_NAMED_SCOPE("prepareMask");
            packNegPosWithPadding(currentANN.neurons,
                population,
                radius,
                currentANN.neuronMinus1s,
                currentANN.neuronPlus1s);

            packNegPosWithPadding(paddingIncommingSynapses,
                incommingSynapsesPitch * population,
                0,
                currentANN.synapseMinus1s,
                currentANN.synapsePlus1s);
        }

        {
            //PROFILE_NAMED_SCOPE("processTickLoop");
            for (unsigned long long tick = 0; tick < numberOfTicks; ++tick)
            {
                processTick();
                // Check exit conditions:
                // - N ticks have passed (already in for loop)
                // - All neuron values are unchanged
                // - All output neurons have non-zero values

                if (areAllNeuronsUnchanged((const char*)previousNeuronValue, (const char*)neurons, population)
                    || areAllNeuronsZeros((const char*)neurons, (const char*)neuronTypes, population))
                {
                    break;
                }

                // Copy the neuron value
                copyMem(previousNeuronValue, neurons, population * sizeof(Neuron));
            }
        }
    }

    bool areAllNeuronsZeros(
        const char* neurons,
        const char* neuronTypes,
        unsigned long long population)
    {

#if defined (__AVX512F__)
        const __m512i zero = _mm512_setzero_si512();
        const __m512i typeOutput = _mm512_set1_epi8(OUTPUT_NEURON_TYPE);

        unsigned long long i = 0;
        for (; i + BATCH_SIZE <= population; i += BATCH_SIZE)
        {
            __m512i cur = _mm512_loadu_si512((const void*)(neurons + i));
            __m512i types = _mm512_loadu_si512((const void*)(neuronTypes + i));

            __mmask64 type_mask = _mm512_cmpeq_epi8_mask(types, typeOutput);
            __mmask64 zero_mask = _mm512_cmpeq_epi8_mask(cur, zero);

            if (type_mask & zero_mask)
                return false;
        }
#else
        const __m256i zero = _mm256_setzero_si256();
        const __m256i typeOutput = _mm256_set1_epi8(OUTPUT_NEURON_TYPE);

        unsigned long long i = 0;
        for (; i + BATCH_SIZE <= population; i += BATCH_SIZE)
        {
            __m256i cur = _mm256_loadu_si256((const __m256i*)(neurons + i));
            __m256i types = _mm256_loadu_si256((const __m256i*)(neuronTypes + i));

            // Compare for type == OUTPUT
            __m256i type_cmp = _mm256_cmpeq_epi8(types, typeOutput);
            int type_mask = _mm256_movemask_epi8(type_cmp);

            // Compare for neuron == 0
            __m256i zero_cmp = _mm256_cmpeq_epi8(cur, zero);
            int zero_mask = _mm256_movemask_epi8(zero_cmp);

            // If both masks overlap → some output neuron is zero
            if (type_mask & zero_mask)
            {
                return false;
            }
        }

#endif
        for (; i < population; i++)
        {
            // Neuron unchanged check
            if (neuronTypes[i] == OUTPUT_NEURON_TYPE && neurons[i] == 0)
            {
                return false;
            }
        }

        return true;
    }

    bool areAllNeuronsUnchanged(
        const char* previousNeuronValue,
        const char* neurons,
        unsigned long long population)
    {
        unsigned long long i = 0;
        for (; i + BATCH_SIZE <= population; i += BATCH_SIZE)
        {

#if defined (__AVX512F__)
            __m512i prev = _mm512_loadu_si512((const void*)(previousNeuronValue + i));
            __m512i cur = _mm512_loadu_si512((const void*)(neurons + i));

            __mmask64 neq_mask = _mm512_cmpneq_epi8_mask(prev, cur);
            if (neq_mask)
            {
                return false;
            }
#else
            __m256i v_prev = _mm256_loadu_si256((const __m256i*)(previousNeuronValue + i));
            __m256i v_curr = _mm256_loadu_si256((const __m256i*)(neurons + i));
            __m256i cmp = _mm256_cmpeq_epi8(v_prev, v_curr);

            int mask = _mm256_movemask_epi8(cmp);

            // -1 means all bytes equal
            if (mask != -1)
            {
                return false;
            }
#endif
        }

        for (; i < population; i++)
        {
            // Neuron unchanged check
            if (previousNeuronValue[i] != neurons[i])
            {
                return false;
            }
        }

        return true;
    }

    unsigned int computeNonMatchingOutput()
    {
        unsigned long long population = currentANN.population;
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;

        // Compute the non-matching value R between output neuron value and initial value
        // Because the output neuron order never changes, the order is preserved
        unsigned int R = 0;
        unsigned long long outputIdx = 0;
        unsigned long long i = 0;
#if defined (__AVX512F__)
        const __m512i typeOutputAVX = _mm512_set1_epi8(OUTPUT_NEURON_TYPE);
        for (; i + BATCH_SIZE <= population; i += BATCH_SIZE)
        {
            // Load 64 neuron types and compare with OUTPUT_NEURON_TYPE
            __m512i types = _mm512_loadu_si512((const void*)(neuronTypes + i));
            __mmask64 type_mask = _mm512_cmpeq_epi8_mask(types, typeOutputAVX);

            if (type_mask == 0)
            {
                continue; // no output neurons in this 64-wide block, just skip
            }

            // Output neuron existed in this block
            for (int k = 0; k < BATCH_SIZE; ++k)
            {
                if (type_mask & (1ULL << k))
                {
                    char neuronVal = neurons[i + k];
                    if (neuronVal != outputNeuronExpectedValue[outputIdx])
                    {
                        R++;
                    }
                    outputIdx++;
                }
            }
        }
#else
        const __m256i typeOutputAVX = _mm256_set1_epi8(OUTPUT_NEURON_TYPE);
        for (; i + BATCH_SIZE <= population; i += BATCH_SIZE)
        {
            __m256i types_vec = _mm256_loadu_si256((const __m256i*)(neuronTypes + i));
            __m256i cmp_vec = _mm256_cmpeq_epi8(types_vec, typeOutputAVX);
            unsigned int type_mask = _mm256_movemask_epi8(cmp_vec);

            if (type_mask == 0)
            {
                continue; // no output neurons in this 32-wide block, just skip
            }
            for (int k = 0; k < BATCH_SIZE; ++k)
            {
                if (type_mask & (1U << k))
                {
                    char neuronVal = neurons[i + k];
                    if (neuronVal != outputNeuronExpectedValue[outputIdx])
                        R++;
                    outputIdx++;
                }
            }
        }
#endif

        // remainder loop
        for (; i < population; i++)
        {
            if (neuronTypes[i] == OUTPUT_NEURON_TYPE)
            {
                if (neurons[i] != outputNeuronExpectedValue[outputIdx])
                {
                    R++;
                }
                outputIdx++;
            }
        }

        return R;
    }

    void initInputNeuron()
    {
        unsigned long long population = currentANN.population;
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;
        Neuron neuronArray[64] = { 0 };
        unsigned long long inputNeuronInitIndex = 0;
        for (unsigned long long i = 0; i < population; ++i)
        {
            // Input will use the init value
            if (neuronTypes[i] == INPUT_NEURON_TYPE)
            {
                // Prepare new pack
                if (inputNeuronInitIndex % 64 == 0)
                {
                    extract64Bits(miningData.inputNeuronRandomNumber[inputNeuronInitIndex / 64], neuronArray);
                }
                char neuronValue = neuronArray[inputNeuronInitIndex % 64];

                // Convert value of neuron to trits (keeping 1 as 1, and changing 0 to -1.).
                neurons[i] = (neuronValue == 0) ? -1 : neuronValue;

                inputNeuronInitIndex++;
            }
        }
    }

    void initNeuronValue()
    {
        initInputNeuron();

        // Starting value of output neuron is zero
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;
        unsigned long long population = currentANN.population;
        for (unsigned long long i = 0; i < population; ++i)
        {
            if (neuronTypes[i] == OUTPUT_NEURON_TYPE)
            {
                neurons[i] = 0;
            }
        }
    }

    void initNeuronType()
    {
        unsigned long long population = currentANN.population;
        Neuron* neurons = currentANN.neurons;
        NeuronType* neuronTypes = currentANN.neuronTypes;
        InitValue* initValue = (InitValue*)paddingInitValue;

        // Randomly choose the positions of neurons types
        for (unsigned long long i = 0; i < population; ++i)
        {
            neuronIndices[i] = i;
            neuronTypes[i] = INPUT_NEURON_TYPE;
        }
        unsigned long long neuronCount = population;
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            unsigned long long outputNeuronIdx = initValue->outputNeuronPositions[i] % neuronCount;

            // Fill the neuron type
            neuronTypes[neuronIndices[outputNeuronIdx]] = OUTPUT_NEURON_TYPE;

            // This index is used, copy the end of indices array to current position and decrease the number of picking neurons
            neuronCount = neuronCount - 1;
            neuronIndices[outputNeuronIdx] = neuronIndices[neuronCount];
        }
    }

    void initExpectedOutputNeuron()
    {
        Neuron neuronArray[64] = { 0 };
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            // Prepare new pack
            if (i % 64 == 0)
            {
                extract64Bits(miningData.outputNeuronRandomNumber[i / 64], neuronArray);
            }
            char neuronValue = neuronArray[i % 64];
            // Convert value of neuron (keeping 1 as 1, and changing 0 to -1.).
            outputNeuronExpectedValue[i] = (neuronValue == 0) ? -1 : neuronValue;
        }
    }

    void initializeRandom2(
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* pRandom2Pool)
    {
        copyMem(combined, publicKey, 32);
        copyMem(combined + 32, nonce, 32);
        KangarooTwelve(combined, 64, hash, 32);

        // Initalize with nonce and public key
        {
            random2(hash, pRandom2Pool, paddingInitValue, paddingInitValueSizeInBytes);

            copyMem((unsigned char*)&miningData, pRandom2Pool, sizeof(MiningData));
        }

    }

    unsigned int initializeANN()
    {
        currentANN.init();
        currentANN.population = numberOfNeurons;
        bestANN.init();
        bestANN.population = numberOfNeurons;

        unsigned long long& population = currentANN.population;
        Synapse* synapses = currentANN.synapses;
        Neuron* neurons = currentANN.neurons;
        InitValue* initValue = (InitValue*)paddingInitValue;

        // Initialization
        population = numberOfNeurons;
        removalNeuronsCount = 0;

        // Synapse weight initialization
        for (unsigned long long i = 0; i < (initNumberOfSynapses / 32); ++i)
        {
            const unsigned long long mask = 0b11;
            for (int j = 0; j < 32; ++j)
            {
                int shiftVal = j * 2;
                unsigned char extractValue = (unsigned char)((initValue->synapseWeight[i] >> shiftVal) & mask);
                switch (extractValue)
                {
                case 2: synapses[32 * i + j] = -1; break;
                case 3: synapses[32 * i + j] = 1; break;
                default: synapses[32 * i + j] = 0;
                }
            }
        }

        // Init the neuron type positions in ANN
        initNeuronType();

        // Init input neuron value and output neuron
        initNeuronValue();

        // Init expected output neuron
        initExpectedOutputNeuron();

        // Ticks simulation
        runTickSimulation();

        // Copy the state for rollback later
        currentANN.copyDataTo(bestANN);

        // Compute R and roll back if neccessary
        unsigned int R = computeNonMatchingOutput();

        return R;

    }

    // Main function for mining
    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* pRandom2Pool)
    {
        // Setup the random starting point 
        initializeRandom2(publicKey, nonce, pRandom2Pool);

        // Initialize
        unsigned int bestR = initializeANN();

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
                //copyMem(&currentANN, &bestANN, sizeof(ANN));
                bestANN.copyDataTo(currentANN);
            }
            else
            {
                bestR = R;

                // Better R. Save the state
                //copyMem(&bestANN, &currentANN, sizeof(ANN));
                currentANN.copyDataTo(bestANN);
            }

            //ASSERT(bestANN.population <= populationThreshold);
        }

        unsigned int score = numberOfOutputNeurons - bestR;
        return score;
    }

    // returns last computed output neurons, only returns non-zero neurons,
    // non-zero neurons will be packed in bits until requestedSizeInBytes is hitted or no more output neurons
    int getLastOutput(unsigned char* requestedOutput, int requestedSizeInBytes)
    {
        return extractLastOutput(
            (const unsigned char*)bestANN.neurons,
            bestANN.neuronTypes,
            bestANN.population,
            requestedOutput,
            requestedSizeInBytes);
    }

};

}
