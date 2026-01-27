#pragma once

#include "score_common.h"

namespace score_engine
{
template <typename Params>
struct ScoreAddition
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
        (maxNumberOfSynapses + 31) / 32 * 32; // padding to multiple of 32

#if defined(__AVX512F__)
    static constexpr unsigned long long BATCH_SIZE = 64;
#else // AVX2 path
    static constexpr unsigned long long BATCH_SIZE = 32;
#endif
    static constexpr unsigned long long PADDED_SAMPLES =
        ((trainingSetSize + BATCH_SIZE - 1) / BATCH_SIZE) * BATCH_SIZE;

    static_assert(
        maxNumberOfSynapses <= (0xFFFFFFFFFFFFFFFF << 1ULL),
        "maxNumberOfSynapses must less than or equal MAX_UINT64/2");
    static_assert(maxNumberOfNeighbors % 2 == 0, "maxNumberOfNeighbors must divided by 2");
    static_assert(
        populationThreshold > numberOfNeurons,
        "populationThreshold must be greater than numberOfNeurons");
    static_assert(
        PADDED_SAMPLES% BATCH_SIZE == 0,
        "PADDED_SAMPLES must be a multiple of BATCH_SIZE");
    static_assert(
        trainingSetSize <= 0xFFFFFFFF,
        "trainingSetSize must fit in unsigned int for sampleMapping");

    typedef char Synapse;
    typedef char Neuron;

    // Data for roll back
    struct ANN
    {
        unsigned char neuronTypes[maxNumberOfNeurons];
        Synapse synapses[maxNumberOfSynapses];
        unsigned long long population;
    };

    // Intermediate data
    struct InitValue
    {
        unsigned long long outputNeuronPositions[numberOfOutputNeurons];
        unsigned long long synapseWeight[paddingNumberOfSynapses / 32]; // each 64bits elements will decide value of 32 synapses
        unsigned long long synpaseMutation[numberOfMutations];
    };
    static constexpr unsigned long long paddingInitValueSizeInBytes = (sizeof(InitValue) + 64 - 1) / 64 * 64;

    unsigned char paddingInitValue[paddingInitValueSizeInBytes];

    // Training set
    alignas(64) char trainingInputs[numberOfInputNeurons * PADDED_SAMPLES];
    alignas(64) char trainingOutputs[numberOfOutputNeurons * PADDED_SAMPLES];

    // For accessing neuron values of multiple samples
    char* neuronValues;
    char* prevNeuronValues;
    alignas(64) char neuronValuesBuffer0[maxNumberOfNeurons * PADDED_SAMPLES];
    alignas(64) char neuronValuesBuffer1[maxNumberOfNeurons * PADDED_SAMPLES];

    // Incoming synapses
    alignas(64) unsigned int incomingSource[maxNumberOfNeurons * maxNumberOfNeighbors];
    alignas(64) Synapse incomingSynapses[maxNumberOfNeurons * maxNumberOfNeighbors];
    unsigned int incomingCount[maxNumberOfNeurons];

    // For tracking/compacting sample after each tick
    alignas(64) unsigned int sampleMapping[PADDED_SAMPLES];
    alignas(64) unsigned int sampleScores[PADDED_SAMPLES];
    unsigned long long activeCount;

    // Indices caching look up
    unsigned long long neuronIndices[numberOfNeurons];
    unsigned long long outputNeuronIndices[numberOfOutputNeurons];
    unsigned long long outputNeuronIdxCache[numberOfOutputNeurons];
    unsigned long long numCachedOutputs;

    // Buffers for cleaning up
    unsigned long long removalNeurons[maxNumberOfNeurons];
    unsigned long long numberOfRedundantNeurons;

    // ANN structure
    ANN bestANN;
    ANN currentANN;

    // Temp buffers
    char inputBits[numberOfInputNeurons];
    char outputBits[numberOfOutputNeurons];

    void initMemory()
    {
        generateTrainingSet();
    }


    void initialize(unsigned char miningSeed[32])
    {
    }

    unsigned long long getActualNeighborCount() const
    {
        unsigned long long population = currentANN.population;
        unsigned long long maxNeighbors = population - 1;  // Exclude self
        unsigned long long actual = maxNumberOfNeighbors > maxNeighbors ? maxNeighbors : maxNumberOfNeighbors;

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

    void cacheOutputNeuronIndices()
    {
        numCachedOutputs = 0;
        for (unsigned long long i = 0; i < currentANN.population; i++)
        {
            if (currentANN.neuronTypes[i] == OUTPUT_NEURON_TYPE)
            {
                outputNeuronIdxCache[numCachedOutputs++] = i;
            }
        }
    }

    void mutate(unsigned long long mutateStep)
    {
        // Mutation
        unsigned long long population = currentANN.population;
        unsigned long long actualNeighbors = getActualNeighborCount();
        Synapse* synapses = currentANN.synapses;
        InitValue* initValue = (InitValue*)paddingInitValue;

        // Randomly pick a synapse, randomly increase or decrease its weight by 1 or -1
        unsigned long long synapseMutation = initValue->synpaseMutation[mutateStep];
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

        char newWeight = synapses[synapseFullBufferIdx] + weightChange;

        // Valid weight. Update it
        if (newWeight >= -1 && newWeight <= 1)
        {
            synapses[synapseFullBufferIdx] = newWeight;
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
        return clampCirculatingIndex((long long)currentANN.population, neuronIdx, value);
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
                pNNSynapses[endSynapseBufferIdx - 1] = 0;
            }
            else
            {
                for (long long k = synapseIndexOfNN; k > (long long)startSynapseBufferIdx; --k)
                {
                    pNNSynapses[k] = pNNSynapses[k - 1];
                }
                pNNSynapses[startSynapseBufferIdx] = 0;
            }
        }

        // Shift the synapse array and the neuron array
        for (unsigned long long shiftIdx = neuronIdx; shiftIdx < currentANN.population - 1; shiftIdx++)
        {
            currentANN.neuronTypes[shiftIdx] = currentANN.neuronTypes[shiftIdx + 1];

            // Also shift the synapses
            copyMem(
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
        ASSERT(synapseIndex >= oldStartSynapseBufferIdx && synapseIndex < oldEndSynapseBufferIdx);

        Synapse* synapses = currentANN.synapses;
        unsigned char* neuronTypes = currentANN.neuronTypes;
        unsigned long long& population = currentANN.population;

        // Copy original neuron to the inserted one and set it as  Neuron::kEvolution type
        unsigned long long insertedNeuronIdx = neuronIndex + 1;

        char originalWeight = synapses[synapseFullBufferIdx];

        // Insert the neuron into array, population increased one, all neurons next to original one
        // need to shift right
        for (unsigned long long i = population; i > neuronIndex; --i)
        {
            neuronTypes[i] = neuronTypes[i - 1];

            // Also shift the synapses to the right
            copyMem(getSynapses(i), getSynapses(i - 1), maxNumberOfNeighbors * sizeof(Synapse));
        }
        neuronTypes[insertedNeuronIdx] = EVOLUTION_NEURON_TYPE;
        population++;

        // Recalculate after population change
        unsigned long long newActualNeighbors = getActualNeighborCount();
        unsigned long long newStartSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long newEndSynapseBufferIdx = getSynapseEndIndex();

        // Try to update the synapse of inserted neuron. All outgoing synapse is init as zero weight
        Synapse* pInsertNeuronSynapse = getSynapses(insertedNeuronIdx);
        for (unsigned long long synIdx = 0; synIdx < maxNumberOfNeighbors; ++synIdx)
        {
            pInsertNeuronSynapse[synIdx] = 0;
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
                pInsertNeuronSynapse[synapseIndex - 1] = originalWeight;
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
            pInsertNeuronSynapse[synapseIndex] = originalWeight;
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

            ASSERT(insertedNeuronIdxInNeigborList >= 0);

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
                    pUpdatedSynapses[insertedNeuronIdxInNeigborList] = 0;
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
        unsigned char* neuronTypes = currentANN.neuronTypes;

        unsigned long long startSynapseBufferIdx = getSynapseStartIndex();
        unsigned long long endSynapseBufferIdx = getSynapseEndIndex();
        long long leftCount = (long long)getLeftNeighborCount();
        long long rightCount = (long long)getRightNeighborCount();

        numberOfRedundantNeurons = 0;
        // After each mutation, we must verify if there are neurons that do not affect the ANN
        // output. These are neurons that either have all incoming synapse weights as 0, or all
        // outgoing synapse weights as 0. Such neurons must be removed.
        for (unsigned long long i = 0; i < population; i++)
        {
            if (neuronTypes[i] == EVOLUTION_NEURON_TYPE)
            {
                bool allOutGoingZeros = true;
                bool allIncommingZeros = true;

                // Loop though its synapses for checkout outgoing synapses
                for (unsigned long long m = startSynapseBufferIdx; m < endSynapseBufferIdx; m++)
                {
                    char synapseW = synapses[i * maxNumberOfNeighbors + m];
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
                    char synapseW = getSynapses(nnIdx)[synapseIdx];

                    if (synapseW != 0)
                    {
                        allIncommingZeros = false;
                        break;
                    }
                }
                if (allOutGoingZeros || allIncommingZeros)
                {
                    removalNeurons[numberOfRedundantNeurons] = i;
                    numberOfRedundantNeurons++;
                }
            }
        }
        return numberOfRedundantNeurons;
    }

    // Remove neurons and synapses that do not affect the ANN
    void cleanANN()
    {
        unsigned long long& population = currentANN.population;

        // Scan and remove neurons/synapses
        for (unsigned long long i = 0; i < numberOfRedundantNeurons; i++)
        {
            // Remove it from the neuron list. Overwrite data
            // Remove its synapses in the synapses array
            removeNeuron(removalNeurons[i]);
            // The index is sorted, just reduce the index
            for (unsigned long long j = i + 1; j < numberOfRedundantNeurons; j++)
            {
                removalNeurons[j]--;
            }
        }
    }

    void processTick()
    {

        unsigned long long population = currentANN.population;
        unsigned char* neuronTypes = currentANN.neuronTypes;

        unsigned long long activeSamplePad = ((activeCount + BATCH_SIZE - 1) / BATCH_SIZE) * BATCH_SIZE;

#if defined(__AVX512F__)

        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        for (unsigned long long targetNeuron = 0; targetNeuron < population; targetNeuron++)
        {
            if (neuronTypes[targetNeuron] == INPUT_NEURON_TYPE)
                continue;

            unsigned int numIncoming = incomingCount[targetNeuron];
            const unsigned long long targetNeuronOffset = targetNeuron * maxNumberOfNeighbors;
            for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
            {
                __m512i acc0 = _mm512_setzero_si512();  // samples 0-31 (int16)
                __m512i acc1 = _mm512_setzero_si512();  // samples 32-63 (int16)
                for (unsigned int i = 0; i < numIncoming; i++)
                {
                    unsigned int srcNeuron = incomingSource[targetNeuronOffset + i];
                    __m512i weight512 = _mm512_set1_epi8(incomingSynapses[targetNeuronOffset + i]);

                    // Load 64 source neuron values
                    //__m512i srcValues = _mm512_load_si512((__m512i*)&neuronValues[srcNeuron][s]);
                    __m512i srcValues = _mm512_loadu_si512((__m512i*)&prevNeuronValues[srcNeuron * PADDED_SAMPLES + s]);

                    __mmask64 negMask = _mm512_movepi8_mask(weight512);
                    __m512i negated = _mm512_sub_epi8(_mm512_setzero_si512(), srcValues);
                    __m512i product = _mm512_mask_blend_epi8(negMask, srcValues, negated);

                    // Sign-extend char to int16 and accumulate
                    __m256i lo = _mm512_extracti64x4_epi64(product, 0);
                    __m256i hi = _mm512_extracti64x4_epi64(product, 1);

                    acc0 = _mm512_add_epi16(acc0, _mm512_cvtepi8_epi16(lo));
                    acc1 = _mm512_add_epi16(acc1, _mm512_cvtepi8_epi16(hi));
                }

                // Clamp to [-1, 1]
                acc0 = _mm512_max_epi16(acc0, negOne16);
                acc0 = _mm512_min_epi16(acc0, one16);
                acc1 = _mm512_max_epi16(acc1, negOne16);
                acc1 = _mm512_min_epi16(acc1, one16);

                // Pack int16 back to int8
                __m512i packed = _mm512_packs_epi16(acc0, acc1);

                // Fix lane ordering after packs
                packed = _mm512_permutexvar_epi64(packLoc, packed);

                _mm512_storeu_si512((__m512i*)&neuronValues[targetNeuron * PADDED_SAMPLES + s], packed);
            }
        }

#else

        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        for (unsigned long long targetNeuron = 0; targetNeuron < population; targetNeuron++)
        {
            if (neuronTypes[targetNeuron] == INPUT_NEURON_TYPE)
                continue;

            unsigned int numIncoming = incomingCount[targetNeuron];
            const unsigned long long targetNeuronOffset = targetNeuron * maxNumberOfNeighbors;
            for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
            {
                __m256i acc0 = _mm256_setzero_si256();  // samples 0-15 (int16)
                __m256i acc1 = _mm256_setzero_si256();  // samples 16-31 (int16)
                for (unsigned int i = 0; i < numIncoming; i++)
                {
                    unsigned int srcNeuron = incomingSource[targetNeuronOffset + i];
                    __m256i weight256 = _mm256_set1_epi8(incomingSynapses[targetNeuronOffset + i]);

                    // Load 32 source neuron values
                    __m256i srcValues = _mm256_loadu_si256((__m256i*)&prevNeuronValues[srcNeuron * PADDED_SAMPLES + s]);

                    __m256i negated = _mm256_sub_epi8(_mm256_setzero_si256(), srcValues);
                    __m256i product = _mm256_blendv_epi8(srcValues, negated, weight256);

                    // Sign-extend char to int16 and accumulate
                    __m128i lo = _mm256_castsi256_si128(product);
                    __m128i hi = _mm256_extracti128_si256(product, 1);

                    acc0 = _mm256_add_epi16(acc0, _mm256_cvtepi8_epi16(lo));
                    acc1 = _mm256_add_epi16(acc1, _mm256_cvtepi8_epi16(hi));
                }

                // Clamp to [-1, 1]
                acc0 = _mm256_max_epi16(acc0, negOne16);
                acc0 = _mm256_min_epi16(acc0, one16);
                acc1 = _mm256_max_epi16(acc1, negOne16);
                acc1 = _mm256_min_epi16(acc1, one16);

                // Pack int16 back to int8
                __m256i packed = _mm256_packs_epi16(acc0, acc1);

                // Fix lane ordering after packs. (3, 1, 2, 0)
                packed = _mm256_permute4x64_epi64(packed, 0xD8); // 0xD8: (3 << 6) | (1 << 4) | (2 << 2) | 0

                _mm256_storeu_si256((__m256i*)&neuronValues[targetNeuron * PADDED_SAMPLES + s], packed);
            }
        }
#endif

    }
    void loadTrainingData()
    {
        unsigned long long population = currentANN.population;

        // Load input neuron values from training data
        unsigned long long inputIdx = 0;
        for (unsigned long long n = 0; n < population; n++)
        {
            if (currentANN.neuronTypes[n] == INPUT_NEURON_TYPE)
            {
                copyMem(&neuronValues[n * PADDED_SAMPLES], &trainingInputs[inputIdx * PADDED_SAMPLES], PADDED_SAMPLES);
                inputIdx++;
            }
            else
            {
                // Non-input neurons: clear to zero
                setMem(&neuronValues[n * PADDED_SAMPLES], PADDED_SAMPLES, 0);
            }
        }

    }

    // Convert the outgoing synapse to incomming style
    void convertToIncomingSynapses()
    {
        unsigned long long population = currentANN.population;
        Synapse* synapses = currentANN.synapses;

        unsigned long long startIdx = getSynapseStartIndex();
        unsigned long long endIdx = getSynapseEndIndex();

        // Clear counts
        setMem(incomingCount, sizeof(incomingCount), 0);

        // Convert outgoing synapses to incoming ones
        for (unsigned long long n = 0; n < population; n++)
        {
            const Synapse* kSynapses = getSynapses(n);
            for (unsigned long long synIdx = startIdx; synIdx < endIdx; synIdx++)
            {
                char weight = kSynapses[synIdx];
                if (weight == 0) continue;

                long long offset = bufferIndexToOffset(synIdx);
                unsigned long long nnIndex = clampNeuronIndex((long long)n, offset);

                unsigned int idx = incomingCount[nnIndex]++;
                incomingSynapses[nnIndex * maxNumberOfNeighbors + idx] = weight;

                // Cache the incomming neuron
                incomingSource[nnIndex * maxNumberOfNeighbors + idx] = (unsigned int)n;
            }
        }
    }

    // Compute the score of a sample
    void computeSampleScore(unsigned long long sampleLocation)
    {
        unsigned int origSample = sampleMapping[sampleLocation];
        unsigned int score = 0;

        for (unsigned long long i = 0; i < numCachedOutputs; i++)
        {
            unsigned long long neuronIdx = outputNeuronIdxCache[i];
            char actual = neuronValues[neuronIdx * PADDED_SAMPLES + sampleLocation];
            char expected = trainingOutputs[i * PADDED_SAMPLES + origSample];

            if (actual == expected)
            {
                score++;
            }
        }

        sampleScores[origSample] = score;
    }

    void compactActiveSamplesWithScoring(unsigned long long& s, unsigned long long& writePos, unsigned long long population)
    {
        while (s < activeCount)
        {
            bool allUnchanged = true;
            for (unsigned long long n = 0; n < population && allUnchanged; n++)
            {
                if (neuronValues[n * PADDED_SAMPLES + s] != prevNeuronValues[n * PADDED_SAMPLES + s])
                {
                    allUnchanged = false;
                }
            }

            bool allOutputsNonZero = true;
            for (unsigned long long i = 0; i < numCachedOutputs && allOutputsNonZero; i++)
            {
                unsigned long long n = outputNeuronIdxCache[i];
                if (neuronValues[n * PADDED_SAMPLES + s] == 0)
                {
                    allOutputsNonZero = false;
                }
            }

            bool isDone = allUnchanged || allOutputsNonZero;

            if (isDone)
            {
                // Score this sample
                unsigned int origSample = sampleMapping[s];
                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    if (neuronValues[n * PADDED_SAMPLES + s] == trainingOutputs[i * PADDED_SAMPLES + origSample])
                    {
                        sampleScores[origSample]++;
                    }
                }
            }
            else
            {
                // Compact
                if (writePos != s)
                {
                    sampleMapping[writePos] = sampleMapping[s];
                    for (unsigned long long n = 0; n < population; n++)
                    {
                        neuronValues[n * PADDED_SAMPLES + writePos] = neuronValues[n * PADDED_SAMPLES + s];
                        prevNeuronValues[n * PADDED_SAMPLES + writePos] = prevNeuronValues[n * PADDED_SAMPLES + s];
                    }
                }
                writePos++;
            }
            s++;
        }
    }

    bool compactActiveSamplesWithScoringSIMD()
    {
        unsigned long long population = currentANN.population;
        unsigned long long writePos = 0;
        unsigned long long s = 0;
#if defined(__AVX512F__)
        const __m512i allOnes = _mm512_set1_epi8(-1);
        while (s + BATCH_SIZE <= activeCount)
        {
            // Check 1: All neurons unchanged (curr == prev)
            // unchangedMask = 1 if unchanged
            __m512i unchangedVec = allOnes;
            for (unsigned long long n = 0; n < population; n++)
            {
                __m512i curr = _mm512_loadu_si512((__m512i*)&neuronValues[n * PADDED_SAMPLES + s]);
                __m512i prev = _mm512_loadu_si512((__m512i*)&prevNeuronValues[n * PADDED_SAMPLES + s]);
                __m512i diff = _mm512_xor_si512(curr, prev);
                unchangedVec = _mm512_andnot_si512(diff, unchangedVec);
            }
            unsigned long long unchangedMask = (unsigned long long)_mm512_cmpeq_epi8_mask(unchangedVec, allOnes);

            // Check 2: All output neurons non-zero
            unsigned long long allOutputsNonZeroMask = ~0ULL;
            for (unsigned long long i = 0; i < numCachedOutputs; i++)
            {
                unsigned long long n = outputNeuronIdxCache[i];
                __m512i val = _mm512_loadu_si512((__m512i*)&neuronValues[n * PADDED_SAMPLES + s]);
                __mmask64 nonZeroMask = _mm512_test_epi8_mask(val, val);
                allOutputsNonZeroMask &= (unsigned long long)nonZeroMask;
            }

            // exitMask: 1 = sample is done, 0 = sample still need to continue ticking
            unsigned long long exitMask = unchangedMask | allOutputsNonZeroMask;
            // activeMask: 1 = sample still active
            unsigned long long activeMask = ~exitMask;

            // There is sample should be stopped and score
            if (exitMask != 0)
            {
                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    // Score each exiting sample
                    unsigned long long tempExitMask = exitMask;
                    while (tempExitMask != 0)
                    {
                        unsigned long long bitPos = countTrailingZerosAssumeNonZero64(tempExitMask);
                        unsigned int origSample = sampleMapping[s + bitPos];
                        char actualVal = neuronValues[n * PADDED_SAMPLES + s + bitPos];
                        char expectedVal = trainingOutputs[i * PADDED_SAMPLES + origSample];
                        if (actualVal == expectedVal)
                        {
                            sampleScores[origSample]++;
                        }
                        tempExitMask = _blsr_u64(tempExitMask); // Clear processed bit
                    }
                }
            }

            // Compact samples, move the unscore/active to the front
            unsigned int activeInBatch = popcnt64(activeMask);
            // All samples are active, just copy if needed
            if (activeInBatch == BATCH_SIZE)
            {
                // Positions are adjusted
                if (writePos != s)
                {
                    // Each copy 16 samples, need 4 times to copy full 64 samples
                    static_assert(BATCH_SIZE / 16 == 4, "Compress loop assumes BATCH_SIZE / 16 == 4");
                    __m512i map0 = _mm512_loadu_si512((__m512i*)&sampleMapping[s + 0]);
                    __m512i map1 = _mm512_loadu_si512((__m512i*)&sampleMapping[s + 16]);
                    __m512i map2 = _mm512_loadu_si512((__m512i*)&sampleMapping[s + 32]);
                    __m512i map3 = _mm512_loadu_si512((__m512i*)&sampleMapping[s + 48]);
                    _mm512_storeu_si512((__m512i*)&sampleMapping[writePos + 0], map0);
                    _mm512_storeu_si512((__m512i*)&sampleMapping[writePos + 16], map1);
                    _mm512_storeu_si512((__m512i*)&sampleMapping[writePos + 32], map2);
                    _mm512_storeu_si512((__m512i*)&sampleMapping[writePos + 48], map3);

                    // Neuron is char, 1 copy is enough
                    for (unsigned long long n = 0; n < population; n++)
                    {
                        unsigned long long baseIdx = n * PADDED_SAMPLES;

                        __m512i curr = _mm512_loadu_si512((__m512i*)&neuronValues[baseIdx + s]);
                        __m512i prev = _mm512_loadu_si512((__m512i*)&prevNeuronValues[baseIdx + s]);

                        _mm512_storeu_si512((__m512i*)&neuronValues[baseIdx + writePos], curr);
                        _mm512_storeu_si512((__m512i*)&prevNeuronValues[baseIdx + writePos], prev);
                    }
                }
                writePos += BATCH_SIZE;
            }
            else if (activeInBatch > 0) // mixing inactive an avtive
            {
                __mmask64 kActive = (__mmask64)activeMask;

                // Load 4 blocks of 16 samples to adjust the sample index
                static_assert(BATCH_SIZE / 16 == 4, "Compress loop assumes BATCH_SIZE / 16 == 4");
                int offset = 0;
                for (unsigned long long blockId = 0; blockId < BATCH_SIZE; blockId +=16)
                {
                    __mmask16 k16 = (__mmask16)((activeMask >> blockId) & 0xFFFF);
                    __m512i map = _mm512_loadu_si512((__m512i*)&sampleMapping[s + blockId]);
                    __m512i compressed = _mm512_maskz_compress_epi32(k16, map);
                    _mm512_storeu_si512((__m512i*)&sampleMapping[writePos + offset], compressed);
                    offset += popcnt32(k16);
                }


                // Adjust the neurons and previous neuron values
                unsigned long long baseIdx = 0;
                for (unsigned long long n = 0; n < population; n++, baseIdx += PADDED_SAMPLES)
                {
                    __m512i curr = _mm512_loadu_si512((__m512i*)&neuronValues[baseIdx + s]);
                    __m512i prev = _mm512_loadu_si512((__m512i*)&prevNeuronValues[baseIdx + s]);

                    __m512i compCurr = _mm512_maskz_compress_epi8(kActive, curr);
                    __m512i compPrev = _mm512_maskz_compress_epi8(kActive, prev);

                    _mm512_storeu_si512((__m512i*)&neuronValues[baseIdx + writePos], compCurr);
                    _mm512_storeu_si512((__m512i*)&prevNeuronValues[baseIdx + writePos], compPrev);
                }

                writePos += activeInBatch;
            }
            // activeInBatch == 0, all samples done, no compaction needed
            s += BATCH_SIZE;
        }
#else
        const __m256i allOnes = _mm256_set1_epi8(-1);
        const __m256i allZeros = _mm256_setzero_si256();
        while (s + BATCH_SIZE <= activeCount)
        {
            // Check 1: All neurons unchanged (curr == prev)
            // unchangedMask = 1 if unchanged
            __m256i unchangedVec = allOnes;
            for (unsigned long long n = 0; n < population; n++)
            {
                __m256i curr = _mm256_loadu_si256((__m256i*)&neuronValues[n * PADDED_SAMPLES + s]);
                __m256i prev = _mm256_loadu_si256((__m256i*)&prevNeuronValues[n * PADDED_SAMPLES + s]);
                __m256i diff = _mm256_xor_si256(curr, prev);
                unchangedVec = _mm256_andnot_si256(diff, unchangedVec);
            }
            unsigned int unchangedMask = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(unchangedVec, allOnes));

            // Check 2: All output neurons non-zero
            unsigned int allOutputsNonZeroMask = ~0U;
            for (unsigned long long i = 0; i < numCachedOutputs; i++)
            {
                unsigned long long n = outputNeuronIdxCache[i];
                __m256i val = _mm256_loadu_si256((__m256i*) & neuronValues[n * PADDED_SAMPLES + s]);
                __m256i isZero = _mm256_cmpeq_epi8(val, allZeros);
                unsigned int zeroMask = (unsigned int)_mm256_movemask_epi8(isZero);
                allOutputsNonZeroMask &= ~zeroMask;
            }

            // exitMask: 1 = sample is done, 0 = sample still need to continue ticking
            unsigned int exitMask = unchangedMask | allOutputsNonZeroMask;
            // activeMask: 1 = sample still active
            unsigned int activeMask = ~exitMask;

            // There is sample should be stopped and score
            if (exitMask != 0)
            {
                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    // Score each exiting sample
                    unsigned int tempExitMask = exitMask;
                    while (tempExitMask != 0)
                    {
                        unsigned int bitPos = countTrailingZerosAssumeNonZero32(tempExitMask);
                        unsigned int origSample = sampleMapping[s + bitPos];
                        char actualVal = neuronValues[n * PADDED_SAMPLES + s + bitPos];
                        char expectedVal = trainingOutputs[i * PADDED_SAMPLES + origSample];
                        if (actualVal == expectedVal)
                        {
                            sampleScores[origSample]++;
                        }
                        tempExitMask = _blsr_u32(tempExitMask); // Clear processed bit
                    }
                }
            }

            // Compact samples, move the unscore/active to the front
            int activeInBatch = popcnt32(activeMask);
            // All samples are active, just copy if needed
            if (activeInBatch == BATCH_SIZE)
            {
                // Positions are adjusted
                if (writePos != s)
                {
                    // Each copy 8 samples, need 4 times to copy full 32 samples
                    static_assert(BATCH_SIZE / 8 == 4, "Compress loop assumes BATCH_SIZE / 8 == 4");
                    __m256i map0 = _mm256_loadu_si256((__m256i*) & sampleMapping[s + 0]);
                    __m256i map1 = _mm256_loadu_si256((__m256i*) & sampleMapping[s + 8]);
                    __m256i map2 = _mm256_loadu_si256((__m256i*) & sampleMapping[s + 16]);
                    __m256i map3 = _mm256_loadu_si256((__m256i*) & sampleMapping[s + 24]);
                    _mm256_storeu_si256((__m256i*)& sampleMapping[writePos + 0], map0);
                    _mm256_storeu_si256((__m256i*)& sampleMapping[writePos + 8], map1);
                    _mm256_storeu_si256((__m256i*)& sampleMapping[writePos + 16], map2);
                    _mm256_storeu_si256((__m256i*)& sampleMapping[writePos + 24], map3);

                    // Neuron is char, 1 copy is enough
                    for (unsigned long long n = 0; n < population; n++)
                    {
                        unsigned long long baseIdx = n * PADDED_SAMPLES;

                        __m256i curr = _mm256_loadu_si256((__m256i*) & neuronValues[baseIdx + s]);
                        __m256i prev = _mm256_loadu_si256((__m256i*) & prevNeuronValues[baseIdx + s]);

                        _mm256_storeu_si256((__m256i*) & neuronValues[baseIdx + writePos], curr);
                        _mm256_storeu_si256((__m256i*) & prevNeuronValues[baseIdx + writePos], prev);
                    }

                }
                writePos += BATCH_SIZE;
            }
            else if (activeInBatch > 0) // mixing inactive an avtive
            {
                unsigned int mask = (unsigned int)activeMask;
                unsigned int writeOffset = 0;
                while (mask != 0)
                {
                    uint32_t i = countTrailingZerosAssumeNonZero32(mask);
                    sampleMapping[writePos + writeOffset] = sampleMapping[s + i];
                    writeOffset++;
                    mask = _blsr_u32(mask); // clear processed bit
                }

                // Separate update for better caching
                for (unsigned long long n = 0; n < population; n++)
                {
                    unsigned long long baseIdx = n * PADDED_SAMPLES;
                    mask = (unsigned int)activeMask;
                    writeOffset = 0;
                    while (mask != 0)
                    {
                        unsigned int i = countTrailingZerosAssumeNonZero32(mask);
                        neuronValues[baseIdx + writePos + writeOffset] = neuronValues[baseIdx + s + i];
                        prevNeuronValues[baseIdx + writePos + writeOffset] = prevNeuronValues[baseIdx + s + i];
                        writeOffset++;
                        mask = _blsr_u32(mask);
                    }
                }

                writePos += activeInBatch;
            }
            // activeInBatch == 0, all samples done, no compaction needed
            s += BATCH_SIZE;
        }
#endif

        // Process remained samples with scalar version
        compactActiveSamplesWithScoring(s, writePos, population);

        activeCount = writePos;
        return (activeCount == 0);
    }

    // Tick simulation only runs on one ANN
    void runTickSimulation()
    {
        // PROFILE_NAMED_SCOPE("runTickSimulation");

        unsigned long long population = currentANN.population;
        //Neuron* neurons = currentANN.neurons;
        unsigned char* neuronTypes = currentANN.neuronTypes;

        {
            // PROFILE_NAMED_SCOPE("runTickSimulation:PrepareData");
            for (unsigned long long i = 0; i < trainingSetSize; i++)
            {
                sampleMapping[i] = (unsigned int)i;
                sampleScores[i] = 0;
            }
            activeCount = trainingSetSize;

            // Load the training set and fill ANN value
            loadTrainingData();
            copyMem(prevNeuronValues, neuronValues, population * PADDED_SAMPLES);

            // Cache the location of ouput neurons
            cacheOutputNeuronIndices();

            // Transpose the synpase
            convertToIncomingSynapses();
        }

        {
            // PROFILE_NAMED_SCOPE("runTickSimulation:Ticking");
            for (unsigned long long tick = 0; tick < numberOfTicks; ++tick)
            {
                // No more ANN to infer aka stop condition of all samples are hit
                if (activeCount == 0)
                {
                    break;
                }

                swapCurrentPreviousNeuronPointers();

                {
                    // PROFILE_NAMED_SCOPE("Ticking:processTick");
                    processTick();
                }

                // Move samples that not exit to suitable consition
                {
                    // PROFILE_NAMED_SCOPE("Ticking:compactActiveSamplesWithScoring");
                    if (compactActiveSamplesWithScoringSIMD())
                    {
                        break;
                    }
                }
            }
        }

        // Full tick simulation finish, compute score of each remained samples
        for (unsigned long long i = 0; i < activeCount; i++)
        {
            computeSampleScore(i);
        }
    }

    // Generate all 2^K possible (A, B, C) pairs
    void generateTrainingSet()
    {
        static constexpr long long boundValue = (1LL << (numberOfInputNeurons / 2)) / 2;
        setMem(trainingInputs, sizeof(trainingInputs), 0);
        setMem(trainingOutputs, sizeof(trainingOutputs), 0);
        unsigned long long sampleIdx = 0;
        for (long long A = -boundValue; A < boundValue; A++)
        {
            for (long long B = -boundValue; B < boundValue; B++)
            {
                long long C = A + B;

                toTenaryBits<numberOfInputNeurons / 2>(A, inputBits);
                toTenaryBits<numberOfInputNeurons / 2>(B, inputBits + numberOfInputNeurons / 2);
                toTenaryBits<numberOfOutputNeurons>(C, outputBits);

                for (unsigned long long n = 0; n < numberOfInputNeurons; n++)
                {
                    trainingInputs[n * PADDED_SAMPLES + sampleIdx] = inputBits[n];
                }

                for (unsigned long long n = 0; n < numberOfOutputNeurons; n++)
                {
                    trainingOutputs[n * PADDED_SAMPLES + sampleIdx] = outputBits[n];
                }
                sampleIdx++;
            }
        }

    }

    unsigned int getTotalSamplesScore()
    {
        unsigned int total = 0;
        for (unsigned long long i = 0; i < trainingSetSize; i++)
        {
            total += sampleScores[i];
        }
        return total;
    }

    unsigned int inferANN()
    {
        unsigned int score = 0;
        runTickSimulation();
        score = getTotalSamplesScore();

        return score;
    }

    void swapCurrentPreviousNeuronPointers()
    {
        char* tmp = neuronValues;
        neuronValues = prevNeuronValues;
        prevNeuronValues = tmp;
    }

    unsigned int initializeANN(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        unsigned char hash[32];
        unsigned char combined[64];
        copyMem(combined, publicKey, 32);
        copyMem(combined + 32, nonce, 32);
        KangarooTwelve(combined, 64, hash, 32);

        unsigned long long& population = currentANN.population;
        Synapse* synapses = currentANN.synapses;
        unsigned char* neuronTypes = currentANN.neuronTypes;

        // Initialization
        population = numberOfNeurons;
        neuronValues = neuronValuesBuffer0;
        prevNeuronValues = neuronValuesBuffer1;

        // Generate all 2^K possible (A, B, C) pairs
        //generateTrainingSet();

        // Initalize with nonce and public key
        random2(hash, randomPool, (unsigned char*)&paddingInitValue, sizeof(paddingInitValue));

        // Randomly choose the positions of neurons types
        for (unsigned long long i = 0; i < population; ++i)
        {
            neuronIndices[i] = i;
            neuronTypes[i] = INPUT_NEURON_TYPE;
        }

        InitValue* initValue = (InitValue*)paddingInitValue;
        unsigned long long neuronCount = population;
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            unsigned long long outputNeuronIdx = initValue->outputNeuronPositions[i] % neuronCount;

            // Fill the neuron type
            neuronTypes[neuronIndices[outputNeuronIdx]] = OUTPUT_NEURON_TYPE;
            outputNeuronIndices[i] = neuronIndices[outputNeuronIdx];

            // This index is used, copy the end of indices array to current position and decrease
            // the number of picking neurons
            neuronCount = neuronCount - 1;
            neuronIndices[outputNeuronIdx] = neuronIndices[neuronCount];
        }

        // Synapse weight initialization
        auto extractWeight = [](unsigned long long packedValue, unsigned long long position) -> char
            {
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
                synapses[32 * i + j] = extractWeight(initValue->synapseWeight[i], j);
            }
        }

        // Handle remaining synapses (if maxNumberOfSynapses not divisible by 32)
        unsigned long long remainder = maxNumberOfSynapses % 32;
        if (remainder > 0)
        {
            unsigned long long lastBlock = maxNumberOfSynapses / 32;
            for (unsigned long long j = 0; j < remainder; ++j)
            {
                synapses[32 * lastBlock + j] = extractWeight(initValue->synapseWeight[lastBlock], j);
            }
        }

        // Run the first inference to get starting point before mutation
        unsigned int score = inferANN();

        return score;
    }

    // Main function for mining
    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        // Initialize
        unsigned int bestR = initializeANN(publicKey, nonce, randomPool);
        copyMem(&bestANN, &currentANN, sizeof(bestANN));

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
                copyMem(&bestANN, &currentANN, sizeof(bestANN));
            }
            else
            {
                // Roll back
                copyMem(&currentANN, &bestANN, sizeof(bestANN));
            }

            ASSERT(bestANN.population <= populationThreshold);
        }
        return bestR;
    }
};

}
