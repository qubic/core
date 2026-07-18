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
    static constexpr unsigned long long numberOfEvolutionNeurons =
        populationThreshold - numberOfNeurons; // P - K - L
    static constexpr unsigned long long maxNumberOfSynapses =
        populationThreshold * maxNumberOfNeighbors;
    static constexpr unsigned long long trainingSetSize = 1ULL << numberOfInputNeurons; // 2^K
    static constexpr unsigned long long paddingNumberOfSynapses =
        (maxNumberOfSynapses + 31) / 32 * 32; // padding to multiple of 32
    // Packed 2-bit synapse storage: 4 weights per byte. Encoding:
    //   00 -> 0, 01 -> +1, 10 -> -1, 11 -> 0
    static constexpr unsigned long long packedSynapsesBytes = (maxNumberOfSynapses + 3) / 4;

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
        PADDED_SAMPLES % BATCH_SIZE == 0,
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
        unsigned char synapsesPacked[packedSynapsesBytes];
        unsigned long long population;
    };

    // Decoded outgoing synapse buffer (derived from currentANN.synapsesPacked).
    Synapse synapses[maxNumberOfSynapses];

    // Intermediate data
    struct InitValue
    {
        unsigned long long outputNeuronPositions[numberOfOutputNeurons];
        unsigned long long evolutionNeuronPositions[numberOfEvolutionNeurons];
        unsigned long long synapseWeight[paddingNumberOfSynapses / 32]; // each 64bits elements will decide value of 32 synapses
        unsigned long long synapseMutation[numberOfMutations];
    };
    static constexpr unsigned long long paddingInitValueSizeInBytes = (sizeof(InitValue) + 64 - 1) / 64 * 64;

    unsigned char paddingInitValue[paddingInitValueSizeInBytes];

    // RDecoded to synapses buffer from currentANN.synapsesPacked
    void decodeSynapses()
    {
        const unsigned char *packedBytes = currentANN.synapsesPacked;
        int32_t *synOut = (int32_t *)synapses;
        // maxNumberOfSynapses is always a multiple of 4 (populationThreshold * maxNumberOfNeighbors,
        // and maxNumberOfNeighbors is required to be even by the static_assert)
        static_assert(maxNumberOfSynapses % 4 == 0, "maxNumberOfSynapses must be divisible by 4 for LUT unpacking");
        const unsigned long long count = maxNumberOfSynapses / 4;
        for (unsigned long long i = 0; i < count; i++)
        {
            synOut[i] = synapseWeightLUT[packedBytes[i]];
        }
    }

    // Training set
    alignas(64) char trainingInputs[numberOfInputNeurons * PADDED_SAMPLES];
    alignas(64) char trainingOutputs[numberOfOutputNeurons * PADDED_SAMPLES];

    // For accessing neuron values of multiple samples
    char *neuronValues;
    char *prevNeuronValues;
    alignas(64) char neuronValuesBuffer0[maxNumberOfNeurons * PADDED_SAMPLES];
    alignas(64) char neuronValuesBuffer1[maxNumberOfNeurons * PADDED_SAMPLES];

    // Incoming synapses split by sign
    alignas(64) unsigned int incomingPositiveSource[maxNumberOfNeurons * maxNumberOfNeighbors];
    alignas(64) unsigned int incomingNegativeSource[maxNumberOfNeurons * maxNumberOfNeighbors];
    unsigned int incomingPositiveCount[maxNumberOfNeurons];
    unsigned int incomingNegativeCount[maxNumberOfNeurons];

    // Dense incoming-weight matrix indexed as incomingSynapseWeight[target * pop + src].
    // Values in {-1, 0, +1}. Kept in sync with currentANN.synapsesPacked by mutate();
    // bestIncomingSynapseWeight is the shadow snapshotted/restored by copyANN.
    alignas(64) char incomingSynapseWeight[maxNumberOfNeurons * maxNumberOfNeurons];
    alignas(64) char bestIncomingSynapseWeight[maxNumberOfNeurons * maxNumberOfNeurons];

    // For tracking/compacting sample after each tick
    alignas(64) unsigned int sampleMapping[PADDED_SAMPLES];
    alignas(64) unsigned int sampleScores[PADDED_SAMPLES];
    unsigned long long activeCount;

    // Indices caching look up
    unsigned long long neuronIndices[maxNumberOfNeurons];
    unsigned long long outputNeuronIndices[numberOfOutputNeurons];
    unsigned long long outputNeuronIdxCache[numberOfOutputNeurons];
    unsigned long long numCachedOutputs;
    unsigned long long evolutionNeuronIdxCache[numberOfEvolutionNeurons];
    unsigned long long numCachedEvolution;

    // Unified output+evolution target list for K-block iteration in processTick.
    // outputEvoNeuronIdxCache[0..numCachedOutputs-1] = output neurons,
    // [numCachedOutputs..numCachedOutputs+numCachedEvolution-1] = evolution neurons.
    unsigned long long outputEvoNeuronIdxCache[numberOfOutputNeurons + numberOfEvolutionNeurons];
    unsigned long long numCachedOutputEvo;

    unsigned long long inputNeuronIdxCache[numberOfInputNeurons];
    unsigned long long numCachedInputs;

    // processNeuronOffsetCache is laid out as [output, input, evolution]
    unsigned long long processNeuronOffsetCache[maxNumberOfNeurons];
    unsigned long long numProcessNeuron;
    unsigned long long numCurrCompactRows;

    // 256-entry LUT: maps a packed byte (4 x 2-bit weights) to 4 char weights stored
    // in one int32_t. Used by decodeSynapses() to expand packed bytes via a single
    // 32-bit store per byte instead of 4 scalar mask/shift operations.
    alignas(64) int32_t synapseWeightLUT[256];

    // Buffers
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

        // Build synapseWeightLUT: each input byte holds 4 packed 2-bit weights;
        // table value is 4 char weights laid out for a single 32-bit store.
        // Encoding: 00 -> 0, 01 -> +1, 10 -> -1, 11 -> 0
        for (int b = 0; b < 256; b++)
        {
            char w[4];
            for (int j = 0; j < 4; j++)
            {
                unsigned char ev = (unsigned char)((b >> (j * 2)) & 3);
                w[j] = (ev == 1) ? (char)+1 : (ev == 2) ? (char)-1
                                                        : (char)0;
            }
            copyMem(&synapseWeightLUT[b], w, 4);
        }
    }

    void initialize(unsigned char miningSeed[32])
    {
    }

    unsigned long long getActualNeighborCount() const
    {
        unsigned long long population = currentANN.population;
        unsigned long long maxNeighbors = population - 1; // Exclude self
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
            return (long long)bufferIdx - synapseBufferCenter; // Negative (left)
        }
        else
        {
            return (long long)bufferIdx - synapseBufferCenter + 1; // Positive (right), skip 0
        }
    }

    void cacheOutputEvoNeuronIndices()
    {
        numCachedOutputs = 0;
        numCachedEvolution = 0;
        numCachedInputs = 0;
        numProcessNeuron = currentANN.population;
        for (unsigned long long i = 0; i < currentANN.population; i++)
        {
            if (currentANN.neuronTypes[i] == OUTPUT_NEURON_TYPE)
            {
                outputNeuronIdxCache[numCachedOutputs++] = i;
            }
            else if (currentANN.neuronTypes[i] == EVOLUTION_NEURON_TYPE)
            {
                evolutionNeuronIdxCache[numCachedEvolution++] = i;
            }
            else if (currentANN.neuronTypes[i] == INPUT_NEURON_TYPE)
            {
                // Cache input source indices for the tick-zero fast path.
                inputNeuronIdxCache[numCachedInputs++] = i;
            }
        }

        // Build processNeuronOffsetCache in [output, input, evolution] order so that
        // compactActive can compact the first numCurrCompactRows entries with curr+prev
        // and the rest with prev-only (when outputsOnlyExitCheck=true).
        unsigned long long idx = 0;
        for (unsigned long long i = 0; i < numCachedOutputs; i++)
            processNeuronOffsetCache[idx++] = outputNeuronIdxCache[i] * PADDED_SAMPLES;
        for (unsigned long long i = 0; i < numCachedInputs; i++)
            processNeuronOffsetCache[idx++] = inputNeuronIdxCache[i] * PADDED_SAMPLES;
        numCurrCompactRows = idx;
        for (unsigned long long i = 0; i < numCachedEvolution; i++)
            processNeuronOffsetCache[idx++] = evolutionNeuronIdxCache[i] * PADDED_SAMPLES;

        // Build the unified output+evolution cache (outputs first, then evolutions).
        // This is the iteration order used by processTick's K-block tiled path.
        numCachedOutputEvo = 0;
        for (unsigned long long i = 0; i < numCachedOutputs; i++)
            outputEvoNeuronIdxCache[numCachedOutputEvo++] = outputNeuronIdxCache[i];
        for (unsigned long long i = 0; i < numCachedEvolution; i++)
            outputEvoNeuronIdxCache[numCachedOutputEvo++] = evolutionNeuronIdxCache[i];
    }

    // Bit-flip mutation on the 2-bit packed weight encoding.
    //   +1 (01) flipped on either bit -> 0 (00 or 11), never -1
    //   -1 (10) flipped on either bit -> 0 (11 or 00), never +1
    //   0  (00) -> +1 or -1 depending on which bit is flipped
    //   0  (11) -> +1 or -1 depending on which bit is flipped (opposite of 00)
    // Returns true if target neuron is non-input
    // Returns false if target is an input neuron, so the mutation cannot change R.
    bool mutate(unsigned long long synapseMutation)
    {
        // Seed split: bit 0 -> which of the 2 bits to flip; bits 1..63 -> synapse pick
        unsigned long long population = currentANN.population;
        unsigned long long actualNeighbors = getActualNeighborCount();

        unsigned long long totalValidSynapses = population * actualNeighbors;
        unsigned long long flatIdx = (synapseMutation >> 1) % totalValidSynapses;

        unsigned long long srcNeuronIdx = flatIdx / actualNeighbors;
        unsigned long long localSynapseIdx = flatIdx % actualNeighbors;

        unsigned long long synapseIndex = localSynapseIdx + getSynapseStartIndex();
        unsigned long long synapseFullBufferIdx = srcNeuronIdx * maxNumberOfNeighbors + synapseIndex;

        // which of the 2 bits will be flipped
        unsigned long long bitOffset = synapseMutation & 1ULL;
        // byte index = synapseFullBufferIdx / 4 (4 weights per byte)
        unsigned long long byteIdx = synapseFullBufferIdx >> 2;
        // slot in byte = synapseFullBufferIdx % 4
        unsigned long long nibblePos = synapseFullBufferIdx & 3ULL;
        // get mask to the set bit
        unsigned char mask = 1u << (nibblePos * 2 + bitOffset);
        // flip the bit, others untouched
        currentANN.synapsesPacked[byteIdx] ^= mask;

        long long offset = bufferIndexToOffset(synapseIndex);
        unsigned long long tgtNeuronIdx = clampNeuronIndex((long long)srcNeuronIdx, offset);
        bool nonInputTarget = currentANN.neuronTypes[tgtNeuronIdx] != INPUT_NEURON_TYPE;

        // Update signed matrix. Decode the new weight after the XOR and
        // write it directly into incomingSynapseWeight[target][src]. Encoding: 00->0, 01->+1, 10->-1, 11->0.
        static constexpr char weightFromNibble[4] = {0, +1, -1, 0};
        unsigned char newNibble =
            (currentANN.synapsesPacked[byteIdx] >> (nibblePos * 2)) & 0x3u;
        incomingSynapseWeight[tgtNeuronIdx * maxNumberOfNeurons + srcNeuronIdx] =
            weightFromNibble[newNibble];

        return nonInputTarget;
    }

    // Calculate the new neuron index that is reached by moving from the given `neuronIdx` `value`
    // neurons to the right or left. Negative `value` moves to the left, positive `value` moves to
    // the right. The return value is clamped in a ring buffer fashion, i.e. moving right of the
    // rightmost neuron continues at the leftmost neuron.
    unsigned long long clampNeuronIndex(long long neuronIdx, long long value)
    {
        return clampCirculatingIndex((long long)currentANN.population, neuronIdx, value);
    }

    // Get the pointer to all outgoing synapses of a neuron.
    Synapse *getSynapses(unsigned long long neuronIndex)
    {
        return &synapses[neuronIndex * maxNumberOfNeighbors];
    }

#if defined(__AVX512F__)
    // K = 1
    void processNeuronTick512(
        unsigned long long targetNeuronBase,
        const unsigned int *positiveSources, unsigned int numPos,
        const unsigned int *negativeSources, unsigned int numNeg,
        unsigned long long activeSamplePad)
    {
        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        unsigned long long s = 0;
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m512i acc16_0a = _mm512_setzero_si512();
            __m512i acc16_1a = _mm512_setzero_si512();
            __m512i acc16_0b = _mm512_setzero_si512();
            __m512i acc16_1b = _mm512_setzero_si512();
            __m512i acc8a = _mm512_setzero_si512();
            __m512i acc8b = _mm512_setzero_si512();
            unsigned int count = 0;

            for (unsigned int i = 0; i < numPos; i++)
            {
                const unsigned long long srcBase = (unsigned long long)positiveSources[i] * PADDED_SAMPLES;
                acc8a = _mm512_add_epi8(acc8a, _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]));
                acc8b = _mm512_add_epi8(acc8b, _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]));
                count++;
                if (count == 127)
                {
                    acc16_0a = _mm512_add_epi16(acc16_0a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 0)));
                    acc16_1a = _mm512_add_epi16(acc16_1a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 1)));
                    acc16_0b = _mm512_add_epi16(acc16_0b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 0)));
                    acc16_1b = _mm512_add_epi16(acc16_1b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 1)));
                    acc8a = _mm512_setzero_si512();
                    acc8b = _mm512_setzero_si512();
                    count = 0;
                }
            }
            acc16_0a = _mm512_add_epi16(acc16_0a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 0)));
            acc16_1a = _mm512_add_epi16(acc16_1a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 1)));
            acc16_0b = _mm512_add_epi16(acc16_0b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 0)));
            acc16_1b = _mm512_add_epi16(acc16_1b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 1)));

            acc8a = _mm512_setzero_si512();
            acc8b = _mm512_setzero_si512();
            count = 0;
            for (unsigned int i = 0; i < numNeg; i++)
            {
                const unsigned long long srcBase = (unsigned long long)negativeSources[i] * PADDED_SAMPLES;
                acc8a = _mm512_add_epi8(acc8a, _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]));
                acc8b = _mm512_add_epi8(acc8b, _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]));
                count++;
                if (count == 127)
                {
                    acc16_0a = _mm512_sub_epi16(acc16_0a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 0)));
                    acc16_1a = _mm512_sub_epi16(acc16_1a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 1)));
                    acc16_0b = _mm512_sub_epi16(acc16_0b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 0)));
                    acc16_1b = _mm512_sub_epi16(acc16_1b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 1)));
                    acc8a = _mm512_setzero_si512();
                    acc8b = _mm512_setzero_si512();
                    count = 0;
                }
            }
            acc16_0a = _mm512_sub_epi16(acc16_0a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 0)));
            acc16_1a = _mm512_sub_epi16(acc16_1a, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8a, 1)));
            acc16_0b = _mm512_sub_epi16(acc16_0b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 0)));
            acc16_1b = _mm512_sub_epi16(acc16_1b, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8b, 1)));

            acc16_0a = _mm512_max_epi16(_mm512_min_epi16(acc16_0a, one16), negOne16);
            acc16_1a = _mm512_max_epi16(_mm512_min_epi16(acc16_1a, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[targetNeuronBase + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc16_0a, acc16_1a)));
            acc16_0b = _mm512_max_epi16(_mm512_min_epi16(acc16_0b, one16), negOne16);
            acc16_1b = _mm512_max_epi16(_mm512_min_epi16(acc16_1b, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[targetNeuronBase + s + BATCH_SIZE],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc16_0b, acc16_1b)));
        }

        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m512i acc16_0 = _mm512_setzero_si512();
            __m512i acc16_1 = _mm512_setzero_si512();
            __m512i acc8 = _mm512_setzero_si512();
            unsigned int count = 0;

            for (unsigned int i = 0; i < numPos; i++)
            {
                const unsigned long long srcBase = (unsigned long long)positiveSources[i] * PADDED_SAMPLES;
                acc8 = _mm512_add_epi8(acc8, _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]));
                count++;
                if (count == 127)
                {
                    acc16_0 = _mm512_add_epi16(acc16_0, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 0)));
                    acc16_1 = _mm512_add_epi16(acc16_1, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 1)));
                    acc8 = _mm512_setzero_si512();
                    count = 0;
                }
            }
            acc16_0 = _mm512_add_epi16(acc16_0, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 0)));
            acc16_1 = _mm512_add_epi16(acc16_1, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 1)));

            acc8 = _mm512_setzero_si512();
            count = 0;
            for (unsigned int i = 0; i < numNeg; i++)
            {
                const unsigned long long srcBase = (unsigned long long)negativeSources[i] * PADDED_SAMPLES;
                acc8 = _mm512_add_epi8(acc8, _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]));
                count++;
                if (count == 127)
                {
                    acc16_0 = _mm512_sub_epi16(acc16_0, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 0)));
                    acc16_1 = _mm512_sub_epi16(acc16_1, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 1)));
                    acc8 = _mm512_setzero_si512();
                    count = 0;
                }
            }
            acc16_0 = _mm512_sub_epi16(acc16_0, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 0)));
            acc16_1 = _mm512_sub_epi16(acc16_1, _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(acc8, 1)));

            acc16_0 = _mm512_max_epi16(_mm512_min_epi16(acc16_0, one16), negOne16);
            acc16_1 = _mm512_max_epi16(_mm512_min_epi16(acc16_1, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[targetNeuronBase + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc16_0, acc16_1)));
        }
    }

    // K=2
    // Computes two target neurons once
    void processNeuronTickBlock2_512(
        unsigned long long target0Base,
        unsigned long long target1Base,
        unsigned long long target0NeuronIdx,
        unsigned long long target1NeuronIdx,
        unsigned long long population,
        unsigned long long activeSamplePad)
    {
        const char *sign0 = &incomingSynapseWeight[target0NeuronIdx * maxNumberOfNeurons];
        const char *sign1 = &incomingSynapseWeight[target1NeuronIdx * maxNumberOfNeurons];

        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        unsigned long long s = 0;

        // Dual-batch fast path: process two BATCH_SIZE chunks per source load.
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m512i acc0_0a = _mm512_setzero_si512();
            __m512i acc0_1a = _mm512_setzero_si512();
            __m512i acc0_0b = _mm512_setzero_si512();
            __m512i acc0_1b = _mm512_setzero_si512();
            __m512i acc1_0a = _mm512_setzero_si512();
            __m512i acc1_1a = _mm512_setzero_si512();
            __m512i acc1_0b = _mm512_setzero_si512();
            __m512i acc1_1b = _mm512_setzero_si512();

            for (unsigned long long src = 0; src < population; src++)
            {
                char sg0 = sign0[src];
                char sg1 = sign1[src];
                if ((sg0 | sg1) == 0)
                    continue;

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i veca = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i vecb = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]);

                __m512i va_0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 0));
                __m512i va_1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 1));
                __m512i vb_0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 0));
                __m512i vb_1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 1));

                if (sg0 == 1)
                {
                    acc0_0a = _mm512_add_epi16(acc0_0a, va_0);
                    acc0_1a = _mm512_add_epi16(acc0_1a, va_1);
                    acc0_0b = _mm512_add_epi16(acc0_0b, vb_0);
                    acc0_1b = _mm512_add_epi16(acc0_1b, vb_1);
                }
                else if (sg0 == -1)
                {
                    acc0_0a = _mm512_sub_epi16(acc0_0a, va_0);
                    acc0_1a = _mm512_sub_epi16(acc0_1a, va_1);
                    acc0_0b = _mm512_sub_epi16(acc0_0b, vb_0);
                    acc0_1b = _mm512_sub_epi16(acc0_1b, vb_1);
                }

                if (sg1 == 1)
                {
                    acc1_0a = _mm512_add_epi16(acc1_0a, va_0);
                    acc1_1a = _mm512_add_epi16(acc1_1a, va_1);
                    acc1_0b = _mm512_add_epi16(acc1_0b, vb_0);
                    acc1_1b = _mm512_add_epi16(acc1_1b, vb_1);
                }
                else if (sg1 == -1)
                {
                    acc1_0a = _mm512_sub_epi16(acc1_0a, va_0);
                    acc1_1a = _mm512_sub_epi16(acc1_1a, va_1);
                    acc1_0b = _mm512_sub_epi16(acc1_0b, vb_0);
                    acc1_1b = _mm512_sub_epi16(acc1_1b, vb_1);
                }
            }

            // Clamp + pack + store target 0
            acc0_0a = _mm512_max_epi16(_mm512_min_epi16(acc0_0a, one16), negOne16);
            acc0_1a = _mm512_max_epi16(_mm512_min_epi16(acc0_1a, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target0Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc0_0a, acc0_1a)));
            acc0_0b = _mm512_max_epi16(_mm512_min_epi16(acc0_0b, one16), negOne16);
            acc0_1b = _mm512_max_epi16(_mm512_min_epi16(acc0_1b, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target0Base + s + BATCH_SIZE],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc0_0b, acc0_1b)));

            // Clamp + pack + store target 1
            acc1_0a = _mm512_max_epi16(_mm512_min_epi16(acc1_0a, one16), negOne16);
            acc1_1a = _mm512_max_epi16(_mm512_min_epi16(acc1_1a, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target1Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc1_0a, acc1_1a)));
            acc1_0b = _mm512_max_epi16(_mm512_min_epi16(acc1_0b, one16), negOne16);
            acc1_1b = _mm512_max_epi16(_mm512_min_epi16(acc1_1b, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target1Base + s + BATCH_SIZE],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc1_0b, acc1_1b)));
        }

        // Single-batch tail.
        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m512i acc0_0 = _mm512_setzero_si512();
            __m512i acc0_1 = _mm512_setzero_si512();
            __m512i acc1_0 = _mm512_setzero_si512();
            __m512i acc1_1 = _mm512_setzero_si512();

            for (unsigned long long src = 0; src < population; src++)
            {
                char sg0 = sign0[src];
                char sg1 = sign1[src];
                if ((sg0 | sg1) == 0)
                    continue;

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i vec = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i v0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 0));
                __m512i v1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 1));

                if (sg0 == 1)
                {
                    acc0_0 = _mm512_add_epi16(acc0_0, v0);
                    acc0_1 = _mm512_add_epi16(acc0_1, v1);
                }
                else if (sg0 == -1)
                {
                    acc0_0 = _mm512_sub_epi16(acc0_0, v0);
                    acc0_1 = _mm512_sub_epi16(acc0_1, v1);
                }

                if (sg1 == 1)
                {
                    acc1_0 = _mm512_add_epi16(acc1_0, v0);
                    acc1_1 = _mm512_add_epi16(acc1_1, v1);
                }
                else if (sg1 == -1)
                {
                    acc1_0 = _mm512_sub_epi16(acc1_0, v0);
                    acc1_1 = _mm512_sub_epi16(acc1_1, v1);
                }
            }

            acc0_0 = _mm512_max_epi16(_mm512_min_epi16(acc0_0, one16), negOne16);
            acc0_1 = _mm512_max_epi16(_mm512_min_epi16(acc0_1, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target0Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc0_0, acc0_1)));

            acc1_0 = _mm512_max_epi16(_mm512_min_epi16(acc1_0, one16), negOne16);
            acc1_1 = _mm512_max_epi16(_mm512_min_epi16(acc1_1, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target1Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc1_0, acc1_1)));
        }
    }

    // K=4 multi-target
    void processNeuronTickBlock4_512(
        const unsigned long long *targetBases,     // length 4
        const unsigned long long *targetNeuronIdx, // length 4 (for incomingSynapseWeight lookup)
        unsigned long long population,
        unsigned long long activeSamplePad)
    {
        const char *sg0 = &incomingSynapseWeight[targetNeuronIdx[0] * maxNumberOfNeurons];
        const char *sg1 = &incomingSynapseWeight[targetNeuronIdx[1] * maxNumberOfNeurons];
        const char *sg2 = &incomingSynapseWeight[targetNeuronIdx[2] * maxNumberOfNeurons];
        const char *sg3 = &incomingSynapseWeight[targetNeuronIdx[3] * maxNumberOfNeurons];

        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        unsigned long long s = 0;

        // Dual-batch fast path: process two BATCH_SIZE chunks per source load.
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m512i a0_la = _mm512_setzero_si512(), a0_ha = _mm512_setzero_si512();
            __m512i a0_lb = _mm512_setzero_si512(), a0_hb = _mm512_setzero_si512();
            __m512i a1_la = _mm512_setzero_si512(), a1_ha = _mm512_setzero_si512();
            __m512i a1_lb = _mm512_setzero_si512(), a1_hb = _mm512_setzero_si512();
            __m512i a2_la = _mm512_setzero_si512(), a2_ha = _mm512_setzero_si512();
            __m512i a2_lb = _mm512_setzero_si512(), a2_hb = _mm512_setzero_si512();
            __m512i a3_la = _mm512_setzero_si512(), a3_ha = _mm512_setzero_si512();
            __m512i a3_lb = _mm512_setzero_si512(), a3_hb = _mm512_setzero_si512();

            for (unsigned long long src = 0; src < population; src++)
            {
                char s0 = sg0[src];
                char s1 = sg1[src];
                char s2 = sg2[src];
                char s3 = sg3[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i veca = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i vecb = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]);

                __m512i va_lo = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 0));
                __m512i va_hi = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 1));
                __m512i vb_lo = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 0));
                __m512i vb_hi = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 1));

                if (s0 == 1)
                {
                    a0_la = _mm512_add_epi16(a0_la, va_lo);
                    a0_ha = _mm512_add_epi16(a0_ha, va_hi);
                    a0_lb = _mm512_add_epi16(a0_lb, vb_lo);
                    a0_hb = _mm512_add_epi16(a0_hb, vb_hi);
                }
                else if (s0 == -1)
                {
                    a0_la = _mm512_sub_epi16(a0_la, va_lo);
                    a0_ha = _mm512_sub_epi16(a0_ha, va_hi);
                    a0_lb = _mm512_sub_epi16(a0_lb, vb_lo);
                    a0_hb = _mm512_sub_epi16(a0_hb, vb_hi);
                }

                if (s1 == 1)
                {
                    a1_la = _mm512_add_epi16(a1_la, va_lo);
                    a1_ha = _mm512_add_epi16(a1_ha, va_hi);
                    a1_lb = _mm512_add_epi16(a1_lb, vb_lo);
                    a1_hb = _mm512_add_epi16(a1_hb, vb_hi);
                }
                else if (s1 == -1)
                {
                    a1_la = _mm512_sub_epi16(a1_la, va_lo);
                    a1_ha = _mm512_sub_epi16(a1_ha, va_hi);
                    a1_lb = _mm512_sub_epi16(a1_lb, vb_lo);
                    a1_hb = _mm512_sub_epi16(a1_hb, vb_hi);
                }

                if (s2 == 1)
                {
                    a2_la = _mm512_add_epi16(a2_la, va_lo);
                    a2_ha = _mm512_add_epi16(a2_ha, va_hi);
                    a2_lb = _mm512_add_epi16(a2_lb, vb_lo);
                    a2_hb = _mm512_add_epi16(a2_hb, vb_hi);
                }
                else if (s2 == -1)
                {
                    a2_la = _mm512_sub_epi16(a2_la, va_lo);
                    a2_ha = _mm512_sub_epi16(a2_ha, va_hi);
                    a2_lb = _mm512_sub_epi16(a2_lb, vb_lo);
                    a2_hb = _mm512_sub_epi16(a2_hb, vb_hi);
                }

                if (s3 == 1)
                {
                    a3_la = _mm512_add_epi16(a3_la, va_lo);
                    a3_ha = _mm512_add_epi16(a3_ha, va_hi);
                    a3_lb = _mm512_add_epi16(a3_lb, vb_lo);
                    a3_hb = _mm512_add_epi16(a3_hb, vb_hi);
                }
                else if (s3 == -1)
                {
                    a3_la = _mm512_sub_epi16(a3_la, va_lo);
                    a3_ha = _mm512_sub_epi16(a3_ha, va_hi);
                    a3_lb = _mm512_sub_epi16(a3_lb, vb_lo);
                    a3_hb = _mm512_sub_epi16(a3_hb, vb_hi);
                }
            }

            // Clamp + pack + store for 4 targets × 2 batches.
            #define STORE_K4_DUAL(idx)                                                                                \
                a##idx##_la = _mm512_max_epi16(_mm512_min_epi16(a##idx##_la, one16), negOne16);                       \
                a##idx##_ha = _mm512_max_epi16(_mm512_min_epi16(a##idx##_ha, one16), negOne16);                       \
                _mm512_storeu_si512((__m512i *)&neuronValues[targetBases[idx] + s],                                   \
                                    _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(a##idx##_la, a##idx##_ha))); \
                a##idx##_lb = _mm512_max_epi16(_mm512_min_epi16(a##idx##_lb, one16), negOne16);                       \
                a##idx##_hb = _mm512_max_epi16(_mm512_min_epi16(a##idx##_hb, one16), negOne16);                       \
                _mm512_storeu_si512((__m512i *)&neuronValues[targetBases[idx] + s + BATCH_SIZE],                      \
                                    _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(a##idx##_lb, a##idx##_hb)))

            STORE_K4_DUAL(0);
            STORE_K4_DUAL(1);
            STORE_K4_DUAL(2);
            STORE_K4_DUAL(3);
            #undef STORE_K4_DUAL
        }

        // Single-batch tail for the final BATCH_SIZE chunk (if activeSamplePad % 128 != 0).
        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m512i a_lo0 = _mm512_setzero_si512(), a_hi0 = _mm512_setzero_si512();
            __m512i a_lo1 = _mm512_setzero_si512(), a_hi1 = _mm512_setzero_si512();
            __m512i a_lo2 = _mm512_setzero_si512(), a_hi2 = _mm512_setzero_si512();
            __m512i a_lo3 = _mm512_setzero_si512(), a_hi3 = _mm512_setzero_si512();

            for (unsigned long long src = 0; src < population; src++)
            {
                char s0 = sg0[src];
                char s1 = sg1[src];
                char s2 = sg2[src];
                char s3 = sg3[src];
                // All-zero-skip dropped: 1.2 % hit rate; the check overhead outweighed
                // the rare savings, and removing it lets the sign-decode pipeline.

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i vec = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i v0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 0));
                __m512i v1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 1));

                if (s0 == 1)
                {
                    a_lo0 = _mm512_add_epi16(a_lo0, v0);
                    a_hi0 = _mm512_add_epi16(a_hi0, v1);
                }
                else if (s0 == -1)
                {
                    a_lo0 = _mm512_sub_epi16(a_lo0, v0);
                    a_hi0 = _mm512_sub_epi16(a_hi0, v1);
                }

                if (s1 == 1)
                {
                    a_lo1 = _mm512_add_epi16(a_lo1, v0);
                    a_hi1 = _mm512_add_epi16(a_hi1, v1);
                }
                else if (s1 == -1)
                {
                    a_lo1 = _mm512_sub_epi16(a_lo1, v0);
                    a_hi1 = _mm512_sub_epi16(a_hi1, v1);
                }

                if (s2 == 1)
                {
                    a_lo2 = _mm512_add_epi16(a_lo2, v0);
                    a_hi2 = _mm512_add_epi16(a_hi2, v1);
                }
                else if (s2 == -1)
                {
                    a_lo2 = _mm512_sub_epi16(a_lo2, v0);
                    a_hi2 = _mm512_sub_epi16(a_hi2, v1);
                }

                if (s3 == 1)
                {
                    a_lo3 = _mm512_add_epi16(a_lo3, v0);
                    a_hi3 = _mm512_add_epi16(a_hi3, v1);
                }
                else if (s3 == -1)
                {
                    a_lo3 = _mm512_sub_epi16(a_lo3, v0);
                    a_hi3 = _mm512_sub_epi16(a_hi3, v1);
                }
            }

            #define STORE_K4_TAIL(idx)                                                      \
                a_lo##idx = _mm512_max_epi16(_mm512_min_epi16(a_lo##idx, one16), negOne16); \
                a_hi##idx = _mm512_max_epi16(_mm512_min_epi16(a_hi##idx, one16), negOne16); \
                _mm512_storeu_si512((__m512i *)&neuronValues[targetBases[idx] + s],         \
                                    _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(a_lo##idx, a_hi##idx)))

            STORE_K4_TAIL(0);
            STORE_K4_TAIL(1);
            STORE_K4_TAIL(2);
            STORE_K4_TAIL(3);
            #undef STORE_K4_TAIL
        }
    }

    // K=4 tick-zero
    void processNeuronTickBlock4Zero_512(
        const unsigned long long *targetBases,
        const unsigned long long *targetNeuronIdx,
        unsigned long long activeSamplePad)
    {
        const char *sg0 = &incomingSynapseWeight[targetNeuronIdx[0] * maxNumberOfNeurons];
        const char *sg1 = &incomingSynapseWeight[targetNeuronIdx[1] * maxNumberOfNeurons];
        const char *sg2 = &incomingSynapseWeight[targetNeuronIdx[2] * maxNumberOfNeurons];
        const char *sg3 = &incomingSynapseWeight[targetNeuronIdx[3] * maxNumberOfNeurons];

        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        unsigned long long s = 0;

        // Dual-batch fast path
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m512i a0_la = _mm512_setzero_si512(), a0_ha = _mm512_setzero_si512();
            __m512i a0_lb = _mm512_setzero_si512(), a0_hb = _mm512_setzero_si512();
            __m512i a1_la = _mm512_setzero_si512(), a1_ha = _mm512_setzero_si512();
            __m512i a1_lb = _mm512_setzero_si512(), a1_hb = _mm512_setzero_si512();
            __m512i a2_la = _mm512_setzero_si512(), a2_ha = _mm512_setzero_si512();
            __m512i a2_lb = _mm512_setzero_si512(), a2_hb = _mm512_setzero_si512();
            __m512i a3_la = _mm512_setzero_si512(), a3_ha = _mm512_setzero_si512();
            __m512i a3_lb = _mm512_setzero_si512(), a3_hb = _mm512_setzero_si512();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char s0 = sg0[src];
                char s1 = sg1[src];
                char s2 = sg2[src];
                char s3 = sg3[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i veca = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i vecb = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]);

                __m512i va_lo = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 0));
                __m512i va_hi = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 1));
                __m512i vb_lo = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 0));
                __m512i vb_hi = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 1));

                if (s0 == 1)
                {
                    a0_la = _mm512_add_epi16(a0_la, va_lo);
                    a0_ha = _mm512_add_epi16(a0_ha, va_hi);
                    a0_lb = _mm512_add_epi16(a0_lb, vb_lo);
                    a0_hb = _mm512_add_epi16(a0_hb, vb_hi);
                }
                else if (s0 == -1)
                {
                    a0_la = _mm512_sub_epi16(a0_la, va_lo);
                    a0_ha = _mm512_sub_epi16(a0_ha, va_hi);
                    a0_lb = _mm512_sub_epi16(a0_lb, vb_lo);
                    a0_hb = _mm512_sub_epi16(a0_hb, vb_hi);
                }

                if (s1 == 1)
                {
                    a1_la = _mm512_add_epi16(a1_la, va_lo);
                    a1_ha = _mm512_add_epi16(a1_ha, va_hi);
                    a1_lb = _mm512_add_epi16(a1_lb, vb_lo);
                    a1_hb = _mm512_add_epi16(a1_hb, vb_hi);
                }
                else if (s1 == -1)
                {
                    a1_la = _mm512_sub_epi16(a1_la, va_lo);
                    a1_ha = _mm512_sub_epi16(a1_ha, va_hi);
                    a1_lb = _mm512_sub_epi16(a1_lb, vb_lo);
                    a1_hb = _mm512_sub_epi16(a1_hb, vb_hi);
                }

                if (s2 == 1)
                {
                    a2_la = _mm512_add_epi16(a2_la, va_lo);
                    a2_ha = _mm512_add_epi16(a2_ha, va_hi);
                    a2_lb = _mm512_add_epi16(a2_lb, vb_lo);
                    a2_hb = _mm512_add_epi16(a2_hb, vb_hi);
                }
                else if (s2 == -1)
                {
                    a2_la = _mm512_sub_epi16(a2_la, va_lo);
                    a2_ha = _mm512_sub_epi16(a2_ha, va_hi);
                    a2_lb = _mm512_sub_epi16(a2_lb, vb_lo);
                    a2_hb = _mm512_sub_epi16(a2_hb, vb_hi);
                }

                if (s3 == 1)
                {
                    a3_la = _mm512_add_epi16(a3_la, va_lo);
                    a3_ha = _mm512_add_epi16(a3_ha, va_hi);
                    a3_lb = _mm512_add_epi16(a3_lb, vb_lo);
                    a3_hb = _mm512_add_epi16(a3_hb, vb_hi);
                }
                else if (s3 == -1)
                {
                    a3_la = _mm512_sub_epi16(a3_la, va_lo);
                    a3_ha = _mm512_sub_epi16(a3_ha, va_hi);
                    a3_lb = _mm512_sub_epi16(a3_lb, vb_lo);
                    a3_hb = _mm512_sub_epi16(a3_hb, vb_hi);
                }
            }

            #define STORE_K4_ZERO_DUAL(idx)                                                                           \
                a##idx##_la = _mm512_max_epi16(_mm512_min_epi16(a##idx##_la, one16), negOne16);                       \
                a##idx##_ha = _mm512_max_epi16(_mm512_min_epi16(a##idx##_ha, one16), negOne16);                       \
                _mm512_storeu_si512((__m512i *)&neuronValues[targetBases[idx] + s],                                   \
                                    _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(a##idx##_la, a##idx##_ha))); \
                a##idx##_lb = _mm512_max_epi16(_mm512_min_epi16(a##idx##_lb, one16), negOne16);                       \
                a##idx##_hb = _mm512_max_epi16(_mm512_min_epi16(a##idx##_hb, one16), negOne16);                       \
                _mm512_storeu_si512((__m512i *)&neuronValues[targetBases[idx] + s + BATCH_SIZE],                      \
                                    _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(a##idx##_lb, a##idx##_hb)))

            STORE_K4_ZERO_DUAL(0);
            STORE_K4_ZERO_DUAL(1);
            STORE_K4_ZERO_DUAL(2);
            STORE_K4_ZERO_DUAL(3);
            #undef STORE_K4_ZERO_DUAL
        }

        // Single-batch tail
        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m512i a_lo0 = _mm512_setzero_si512(), a_hi0 = _mm512_setzero_si512();
            __m512i a_lo1 = _mm512_setzero_si512(), a_hi1 = _mm512_setzero_si512();
            __m512i a_lo2 = _mm512_setzero_si512(), a_hi2 = _mm512_setzero_si512();
            __m512i a_lo3 = _mm512_setzero_si512(), a_hi3 = _mm512_setzero_si512();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char s0 = sg0[src];
                char s1 = sg1[src];
                char s2 = sg2[src];
                char s3 = sg3[src];
                
                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i vec = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i v0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 0));
                __m512i v1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 1));

                if (s0 == 1)
                {
                    a_lo0 = _mm512_add_epi16(a_lo0, v0);
                    a_hi0 = _mm512_add_epi16(a_hi0, v1);
                }
                else if (s0 == -1)
                {
                    a_lo0 = _mm512_sub_epi16(a_lo0, v0);
                    a_hi0 = _mm512_sub_epi16(a_hi0, v1);
                }

                if (s1 == 1)
                {
                    a_lo1 = _mm512_add_epi16(a_lo1, v0);
                    a_hi1 = _mm512_add_epi16(a_hi1, v1);
                }
                else if (s1 == -1)
                {
                    a_lo1 = _mm512_sub_epi16(a_lo1, v0);
                    a_hi1 = _mm512_sub_epi16(a_hi1, v1);
                }

                if (s2 == 1)
                {
                    a_lo2 = _mm512_add_epi16(a_lo2, v0);
                    a_hi2 = _mm512_add_epi16(a_hi2, v1);
                }
                else if (s2 == -1)
                {
                    a_lo2 = _mm512_sub_epi16(a_lo2, v0);
                    a_hi2 = _mm512_sub_epi16(a_hi2, v1);
                }

                if (s3 == 1)
                {
                    a_lo3 = _mm512_add_epi16(a_lo3, v0);
                    a_hi3 = _mm512_add_epi16(a_hi3, v1);
                }
                else if (s3 == -1)
                {
                    a_lo3 = _mm512_sub_epi16(a_lo3, v0);
                    a_hi3 = _mm512_sub_epi16(a_hi3, v1);
                }
            }

            #define STORE_K4_ZERO_TAIL(idx)                                                 \
                a_lo##idx = _mm512_max_epi16(_mm512_min_epi16(a_lo##idx, one16), negOne16); \
                a_hi##idx = _mm512_max_epi16(_mm512_min_epi16(a_hi##idx, one16), negOne16); \
                _mm512_storeu_si512((__m512i *)&neuronValues[targetBases[idx] + s],         \
                                    _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(a_lo##idx, a_hi##idx)))

            STORE_K4_ZERO_TAIL(0);
            STORE_K4_ZERO_TAIL(1);
            STORE_K4_ZERO_TAIL(2);
            STORE_K4_ZERO_TAIL(3);
            #undef STORE_K4_ZERO_TAIL
        }
    }

    // K=2 tick-zero kernel 
    void processNeuronTickBlock2Zero_512(
        unsigned long long target0Base,
        unsigned long long target1Base,
        unsigned long long target0NeuronIdx,
        unsigned long long target1NeuronIdx,
        unsigned long long activeSamplePad)
    {
        const char *sign0 = &incomingSynapseWeight[target0NeuronIdx * maxNumberOfNeurons];
        const char *sign1 = &incomingSynapseWeight[target1NeuronIdx * maxNumberOfNeurons];

        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        unsigned long long s = 0;

        // Dual-batch fast path
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m512i acc0_0a = _mm512_setzero_si512();
            __m512i acc0_1a = _mm512_setzero_si512();
            __m512i acc0_0b = _mm512_setzero_si512();
            __m512i acc0_1b = _mm512_setzero_si512();
            __m512i acc1_0a = _mm512_setzero_si512();
            __m512i acc1_1a = _mm512_setzero_si512();
            __m512i acc1_0b = _mm512_setzero_si512();
            __m512i acc1_1b = _mm512_setzero_si512();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char sg0 = sign0[src];
                char sg1 = sign1[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i veca = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i vecb = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]);

                __m512i va_0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 0));
                __m512i va_1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 1));
                __m512i vb_0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 0));
                __m512i vb_1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 1));

                if (sg0 == 1)
                {
                    acc0_0a = _mm512_add_epi16(acc0_0a, va_0);
                    acc0_1a = _mm512_add_epi16(acc0_1a, va_1);
                    acc0_0b = _mm512_add_epi16(acc0_0b, vb_0);
                    acc0_1b = _mm512_add_epi16(acc0_1b, vb_1);
                }
                else if (sg0 == -1)
                {
                    acc0_0a = _mm512_sub_epi16(acc0_0a, va_0);
                    acc0_1a = _mm512_sub_epi16(acc0_1a, va_1);
                    acc0_0b = _mm512_sub_epi16(acc0_0b, vb_0);
                    acc0_1b = _mm512_sub_epi16(acc0_1b, vb_1);
                }

                if (sg1 == 1)
                {
                    acc1_0a = _mm512_add_epi16(acc1_0a, va_0);
                    acc1_1a = _mm512_add_epi16(acc1_1a, va_1);
                    acc1_0b = _mm512_add_epi16(acc1_0b, vb_0);
                    acc1_1b = _mm512_add_epi16(acc1_1b, vb_1);
                }
                else if (sg1 == -1)
                {
                    acc1_0a = _mm512_sub_epi16(acc1_0a, va_0);
                    acc1_1a = _mm512_sub_epi16(acc1_1a, va_1);
                    acc1_0b = _mm512_sub_epi16(acc1_0b, vb_0);
                    acc1_1b = _mm512_sub_epi16(acc1_1b, vb_1);
                }
            }

            // Clamp + pack + store target 0
            acc0_0a = _mm512_max_epi16(_mm512_min_epi16(acc0_0a, one16), negOne16);
            acc0_1a = _mm512_max_epi16(_mm512_min_epi16(acc0_1a, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target0Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc0_0a, acc0_1a)));
            acc0_0b = _mm512_max_epi16(_mm512_min_epi16(acc0_0b, one16), negOne16);
            acc0_1b = _mm512_max_epi16(_mm512_min_epi16(acc0_1b, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target0Base + s + BATCH_SIZE],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc0_0b, acc0_1b)));

            // Clamp + pack + store target 1
            acc1_0a = _mm512_max_epi16(_mm512_min_epi16(acc1_0a, one16), negOne16);
            acc1_1a = _mm512_max_epi16(_mm512_min_epi16(acc1_1a, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target1Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc1_0a, acc1_1a)));
            acc1_0b = _mm512_max_epi16(_mm512_min_epi16(acc1_0b, one16), negOne16);
            acc1_1b = _mm512_max_epi16(_mm512_min_epi16(acc1_1b, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target1Base + s + BATCH_SIZE],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc1_0b, acc1_1b)));
        }

        // Single-batch tail
        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m512i acc0_0 = _mm512_setzero_si512();
            __m512i acc0_1 = _mm512_setzero_si512();
            __m512i acc1_0 = _mm512_setzero_si512();
            __m512i acc1_1 = _mm512_setzero_si512();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char sg0 = sign0[src];
                char sg1 = sign1[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i vec = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i v0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 0));
                __m512i v1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 1));

                if (sg0 == 1)
                {
                    acc0_0 = _mm512_add_epi16(acc0_0, v0);
                    acc0_1 = _mm512_add_epi16(acc0_1, v1);
                }
                else if (sg0 == -1)
                {
                    acc0_0 = _mm512_sub_epi16(acc0_0, v0);
                    acc0_1 = _mm512_sub_epi16(acc0_1, v1);
                }

                if (sg1 == 1)
                {
                    acc1_0 = _mm512_add_epi16(acc1_0, v0);
                    acc1_1 = _mm512_add_epi16(acc1_1, v1);
                }
                else if (sg1 == -1)
                {
                    acc1_0 = _mm512_sub_epi16(acc1_0, v0);
                    acc1_1 = _mm512_sub_epi16(acc1_1, v1);
                }
            }

            acc0_0 = _mm512_max_epi16(_mm512_min_epi16(acc0_0, one16), negOne16);
            acc0_1 = _mm512_max_epi16(_mm512_min_epi16(acc0_1, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target0Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc0_0, acc0_1)));

            acc1_0 = _mm512_max_epi16(_mm512_min_epi16(acc1_0, one16), negOne16);
            acc1_1 = _mm512_max_epi16(_mm512_min_epi16(acc1_1, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[target1Base + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc1_0, acc1_1)));
        }
    }

    // K=1 tick-zero kernel
    void processNeuronTickZero_512(
        unsigned long long targetNeuronBase,
        unsigned long long targetNeuronIdx,
        unsigned long long activeSamplePad)
    {
        const char *sign = &incomingSynapseWeight[targetNeuronIdx * maxNumberOfNeurons];

        const __m512i one16 = _mm512_set1_epi16(1);
        const __m512i negOne16 = _mm512_set1_epi16(-1);
        const __m512i packLoc = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

        unsigned long long s = 0;

        // Dual-batch fast path
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m512i acc_0a = _mm512_setzero_si512();
            __m512i acc_1a = _mm512_setzero_si512();
            __m512i acc_0b = _mm512_setzero_si512();
            __m512i acc_1b = _mm512_setzero_si512();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char sg = sign[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i veca = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i vecb = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]);

                __m512i va_0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 0));
                __m512i va_1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(veca, 1));
                __m512i vb_0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 0));
                __m512i vb_1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vecb, 1));

                if (sg == 1)
                {
                    acc_0a = _mm512_add_epi16(acc_0a, va_0);
                    acc_1a = _mm512_add_epi16(acc_1a, va_1);
                    acc_0b = _mm512_add_epi16(acc_0b, vb_0);
                    acc_1b = _mm512_add_epi16(acc_1b, vb_1);
                }
                else if (sg == -1)
                {
                    acc_0a = _mm512_sub_epi16(acc_0a, va_0);
                    acc_1a = _mm512_sub_epi16(acc_1a, va_1);
                    acc_0b = _mm512_sub_epi16(acc_0b, vb_0);
                    acc_1b = _mm512_sub_epi16(acc_1b, vb_1);
                }
            }

            acc_0a = _mm512_max_epi16(_mm512_min_epi16(acc_0a, one16), negOne16);
            acc_1a = _mm512_max_epi16(_mm512_min_epi16(acc_1a, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[targetNeuronBase + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc_0a, acc_1a)));
            acc_0b = _mm512_max_epi16(_mm512_min_epi16(acc_0b, one16), negOne16);
            acc_1b = _mm512_max_epi16(_mm512_min_epi16(acc_1b, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[targetNeuronBase + s + BATCH_SIZE],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc_0b, acc_1b)));
        }

        // Single-batch tail
        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m512i acc_0 = _mm512_setzero_si512();
            __m512i acc_1 = _mm512_setzero_si512();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char sg = sign[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m512i vec = _mm512_loadu_si512((__m512i *)&prevNeuronValues[srcBase + s]);
                __m512i v0 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 0));
                __m512i v1 = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(vec, 1));

                if (sg == 1)
                {
                    acc_0 = _mm512_add_epi16(acc_0, v0);
                    acc_1 = _mm512_add_epi16(acc_1, v1);
                }
                else if (sg == -1)
                {
                    acc_0 = _mm512_sub_epi16(acc_0, v0);
                    acc_1 = _mm512_sub_epi16(acc_1, v1);
                }
            }

            acc_0 = _mm512_max_epi16(_mm512_min_epi16(acc_0, one16), negOne16);
            acc_1 = _mm512_max_epi16(_mm512_min_epi16(acc_1, one16), negOne16);
            _mm512_storeu_si512((__m512i *)&neuronValues[targetNeuronBase + s],
                                _mm512_permutexvar_epi64(packLoc, _mm512_packs_epi16(acc_0, acc_1)));
        }
    }

#else // AVX2
    void processNeuronTick256(
        unsigned long long targetNeuronBase,
        const unsigned int *positiveSources, unsigned int numPos,
        const unsigned int *negativeSources, unsigned int numNeg,
        unsigned long long activeSamplePad)
    {
        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        unsigned long long s = 0;
        for (; s + 2 * BATCH_SIZE <= activeSamplePad; s += 2 * BATCH_SIZE)
        {
            __m256i acc16_0a = _mm256_setzero_si256();
            __m256i acc16_1a = _mm256_setzero_si256();
            __m256i acc16_0b = _mm256_setzero_si256();
            __m256i acc16_1b = _mm256_setzero_si256();
            __m256i acc8a = _mm256_setzero_si256();
            __m256i acc8b = _mm256_setzero_si256();
            unsigned int count = 0;

            for (unsigned int i = 0; i < numPos; i++)
            {
                const unsigned long long srcBase = (unsigned long long)positiveSources[i] * PADDED_SAMPLES;
                acc8a = _mm256_add_epi8(acc8a, _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]));
                acc8b = _mm256_add_epi8(acc8b, _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]));
                count++;
                if (count == 127)
                {
                    acc16_0a = _mm256_add_epi16(acc16_0a, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8a)));
                    acc16_1a = _mm256_add_epi16(acc16_1a, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8a, 1)));
                    acc16_0b = _mm256_add_epi16(acc16_0b, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8b)));
                    acc16_1b = _mm256_add_epi16(acc16_1b, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8b, 1)));
                    acc8a = _mm256_setzero_si256();
                    acc8b = _mm256_setzero_si256();
                    count = 0;
                }
            }
            acc16_0a = _mm256_add_epi16(acc16_0a, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8a)));
            acc16_1a = _mm256_add_epi16(acc16_1a, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8a, 1)));
            acc16_0b = _mm256_add_epi16(acc16_0b, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8b)));
            acc16_1b = _mm256_add_epi16(acc16_1b, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8b, 1)));

            acc8a = _mm256_setzero_si256();
            acc8b = _mm256_setzero_si256();
            count = 0;
            for (unsigned int i = 0; i < numNeg; i++)
            {
                const unsigned long long srcBase = (unsigned long long)negativeSources[i] * PADDED_SAMPLES;
                acc8a = _mm256_add_epi8(acc8a, _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]));
                acc8b = _mm256_add_epi8(acc8b, _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s + BATCH_SIZE]));
                count++;
                if (count == 127)
                {
                    acc16_0a = _mm256_sub_epi16(acc16_0a, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8a)));
                    acc16_1a = _mm256_sub_epi16(acc16_1a, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8a, 1)));
                    acc16_0b = _mm256_sub_epi16(acc16_0b, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8b)));
                    acc16_1b = _mm256_sub_epi16(acc16_1b, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8b, 1)));
                    acc8a = _mm256_setzero_si256();
                    acc8b = _mm256_setzero_si256();
                    count = 0;
                }
            }
            acc16_0a = _mm256_sub_epi16(acc16_0a, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8a)));
            acc16_1a = _mm256_sub_epi16(acc16_1a, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8a, 1)));
            acc16_0b = _mm256_sub_epi16(acc16_0b, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8b)));
            acc16_1b = _mm256_sub_epi16(acc16_1b, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8b, 1)));

            acc16_0a = _mm256_max_epi16(_mm256_min_epi16(acc16_0a, one16), negOne16);
            acc16_1a = _mm256_max_epi16(_mm256_min_epi16(acc16_1a, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[targetNeuronBase + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc16_0a, acc16_1a), 0xD8));
            acc16_0b = _mm256_max_epi16(_mm256_min_epi16(acc16_0b, one16), negOne16);
            acc16_1b = _mm256_max_epi16(_mm256_min_epi16(acc16_1b, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[targetNeuronBase + s + BATCH_SIZE],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc16_0b, acc16_1b), 0xD8));
        }

        for (; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m256i acc16_0 = _mm256_setzero_si256();
            __m256i acc16_1 = _mm256_setzero_si256();
            __m256i acc8 = _mm256_setzero_si256();
            unsigned int count = 0;

            for (unsigned int i = 0; i < numPos; i++)
            {
                const unsigned long long srcBase = (unsigned long long)positiveSources[i] * PADDED_SAMPLES;
                acc8 = _mm256_add_epi8(acc8, _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]));
                count++;
                if (count == 127)
                {
                    acc16_0 = _mm256_add_epi16(acc16_0, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8)));
                    acc16_1 = _mm256_add_epi16(acc16_1, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8, 1)));
                    acc8 = _mm256_setzero_si256();
                    count = 0;
                }
            }
            acc16_0 = _mm256_add_epi16(acc16_0, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8)));
            acc16_1 = _mm256_add_epi16(acc16_1, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8, 1)));

            acc8 = _mm256_setzero_si256();
            count = 0;
            for (unsigned int i = 0; i < numNeg; i++)
            {
                const unsigned long long srcBase = (unsigned long long)negativeSources[i] * PADDED_SAMPLES;
                acc8 = _mm256_add_epi8(acc8, _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]));
                count++;
                if (count == 127)
                {
                    acc16_0 = _mm256_sub_epi16(acc16_0, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8)));
                    acc16_1 = _mm256_sub_epi16(acc16_1, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8, 1)));
                    acc8 = _mm256_setzero_si256();
                    count = 0;
                }
            }
            acc16_0 = _mm256_sub_epi16(acc16_0, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(acc8)));
            acc16_1 = _mm256_sub_epi16(acc16_1, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(acc8, 1)));

            acc16_0 = _mm256_max_epi16(_mm256_min_epi16(acc16_0, one16), negOne16);
            acc16_1 = _mm256_max_epi16(_mm256_min_epi16(acc16_1, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[targetNeuronBase + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc16_0, acc16_1), 0xD8));
        }
    }

    // K=2 multi-target
    void processNeuronTickBlock2_256(
        unsigned long long target0Base,
        unsigned long long target1Base,
        unsigned long long target0NeuronIdx,
        unsigned long long target1NeuronIdx,
        unsigned long long population,
        unsigned long long activeSamplePad)
    {
        const char *sign0 = &incomingSynapseWeight[target0NeuronIdx * maxNumberOfNeurons];
        const char *sign1 = &incomingSynapseWeight[target1NeuronIdx * maxNumberOfNeurons];

        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m256i acc0_0 = _mm256_setzero_si256();
            __m256i acc0_1 = _mm256_setzero_si256();
            __m256i acc1_0 = _mm256_setzero_si256();
            __m256i acc1_1 = _mm256_setzero_si256();

            for (unsigned long long src = 0; src < population; src++)
            {
                char sg0 = sign0[src];
                char sg1 = sign1[src];
                if ((sg0 | sg1) == 0)
                    continue;

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m256i vec = _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]);
                __m256i v0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(vec));
                __m256i v1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vec, 1));

                if (sg0 == 1)
                {
                    acc0_0 = _mm256_add_epi16(acc0_0, v0);
                    acc0_1 = _mm256_add_epi16(acc0_1, v1);
                }
                else if (sg0 == -1)
                {
                    acc0_0 = _mm256_sub_epi16(acc0_0, v0);
                    acc0_1 = _mm256_sub_epi16(acc0_1, v1);
                }

                if (sg1 == 1)
                {
                    acc1_0 = _mm256_add_epi16(acc1_0, v0);
                    acc1_1 = _mm256_add_epi16(acc1_1, v1);
                }
                else if (sg1 == -1)
                {
                    acc1_0 = _mm256_sub_epi16(acc1_0, v0);
                    acc1_1 = _mm256_sub_epi16(acc1_1, v1);
                }
            }

            acc0_0 = _mm256_max_epi16(_mm256_min_epi16(acc0_0, one16), negOne16);
            acc0_1 = _mm256_max_epi16(_mm256_min_epi16(acc0_1, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[target0Base + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc0_0, acc0_1), 0xD8));

            acc1_0 = _mm256_max_epi16(_mm256_min_epi16(acc1_0, one16), negOne16);
            acc1_1 = _mm256_max_epi16(_mm256_min_epi16(acc1_1, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[target1Base + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc1_0, acc1_1), 0xD8));
        }
    }

    // K=4 multi-target 
    void processNeuronTickBlock4_256(
        const unsigned long long *targetBases,
        const unsigned long long *targetNeuronIdx,
        unsigned long long population,
        unsigned long long activeSamplePad)
    {
        const char *sg0 = &incomingSynapseWeight[targetNeuronIdx[0] * maxNumberOfNeurons];
        const char *sg1 = &incomingSynapseWeight[targetNeuronIdx[1] * maxNumberOfNeurons];
        const char *sg2 = &incomingSynapseWeight[targetNeuronIdx[2] * maxNumberOfNeurons];
        const char *sg3 = &incomingSynapseWeight[targetNeuronIdx[3] * maxNumberOfNeurons];

        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m256i a_lo0 = _mm256_setzero_si256(), a_hi0 = _mm256_setzero_si256();
            __m256i a_lo1 = _mm256_setzero_si256(), a_hi1 = _mm256_setzero_si256();
            __m256i a_lo2 = _mm256_setzero_si256(), a_hi2 = _mm256_setzero_si256();
            __m256i a_lo3 = _mm256_setzero_si256(), a_hi3 = _mm256_setzero_si256();

            for (unsigned long long src = 0; src < population; src++)
            {
                char s0 = sg0[src];
                char s1 = sg1[src];
                char s2 = sg2[src];
                char s3 = sg3[src];
                // All-zero-skip dropped: 1.2 % hit rate; the check overhead outweighed
                // the rare savings, and removing it lets the sign-decode pipeline.

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m256i vec = _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]);
                __m256i v0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(vec));
                __m256i v1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vec, 1));

                if (s0 == 1)
                {
                    a_lo0 = _mm256_add_epi16(a_lo0, v0);
                    a_hi0 = _mm256_add_epi16(a_hi0, v1);
                }
                else if (s0 == -1)
                {
                    a_lo0 = _mm256_sub_epi16(a_lo0, v0);
                    a_hi0 = _mm256_sub_epi16(a_hi0, v1);
                }

                if (s1 == 1)
                {
                    a_lo1 = _mm256_add_epi16(a_lo1, v0);
                    a_hi1 = _mm256_add_epi16(a_hi1, v1);
                }
                else if (s1 == -1)
                {
                    a_lo1 = _mm256_sub_epi16(a_lo1, v0);
                    a_hi1 = _mm256_sub_epi16(a_hi1, v1);
                }

                if (s2 == 1)
                {
                    a_lo2 = _mm256_add_epi16(a_lo2, v0);
                    a_hi2 = _mm256_add_epi16(a_hi2, v1);
                }
                else if (s2 == -1)
                {
                    a_lo2 = _mm256_sub_epi16(a_lo2, v0);
                    a_hi2 = _mm256_sub_epi16(a_hi2, v1);
                }

                if (s3 == 1)
                {
                    a_lo3 = _mm256_add_epi16(a_lo3, v0);
                    a_hi3 = _mm256_add_epi16(a_hi3, v1);
                }
                else if (s3 == -1)
                {
                    a_lo3 = _mm256_sub_epi16(a_lo3, v0);
                    a_hi3 = _mm256_sub_epi16(a_hi3, v1);
                }
            }

            #define STORE_K4_256(idx)                                                       \
                a_lo##idx = _mm256_max_epi16(_mm256_min_epi16(a_lo##idx, one16), negOne16); \
                a_hi##idx = _mm256_max_epi16(_mm256_min_epi16(a_hi##idx, one16), negOne16); \
                _mm256_storeu_si256((__m256i *)&neuronValues[targetBases[idx] + s],         \
                                    _mm256_permute4x64_epi64(_mm256_packs_epi16(a_lo##idx, a_hi##idx), 0xD8))

            STORE_K4_256(0);
            STORE_K4_256(1);
            STORE_K4_256(2);
            STORE_K4_256(3);
            #undef STORE_K4_256
        }
    }

    // K=4 tick-zero kernel
    void processNeuronTickBlock4Zero_256(
        const unsigned long long *targetBases,
        const unsigned long long *targetNeuronIdx,
        unsigned long long activeSamplePad)
    {
        const char *sg0 = &incomingSynapseWeight[targetNeuronIdx[0] * maxNumberOfNeurons];
        const char *sg1 = &incomingSynapseWeight[targetNeuronIdx[1] * maxNumberOfNeurons];
        const char *sg2 = &incomingSynapseWeight[targetNeuronIdx[2] * maxNumberOfNeurons];
        const char *sg3 = &incomingSynapseWeight[targetNeuronIdx[3] * maxNumberOfNeurons];

        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m256i a_lo0 = _mm256_setzero_si256(), a_hi0 = _mm256_setzero_si256();
            __m256i a_lo1 = _mm256_setzero_si256(), a_hi1 = _mm256_setzero_si256();
            __m256i a_lo2 = _mm256_setzero_si256(), a_hi2 = _mm256_setzero_si256();
            __m256i a_lo3 = _mm256_setzero_si256(), a_hi3 = _mm256_setzero_si256();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char s0 = sg0[src];
                char s1 = sg1[src];
                char s2 = sg2[src];
                char s3 = sg3[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m256i vec = _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]);
                __m256i v0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(vec));
                __m256i v1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vec, 1));

                if (s0 == 1)
                {
                    a_lo0 = _mm256_add_epi16(a_lo0, v0);
                    a_hi0 = _mm256_add_epi16(a_hi0, v1);
                }
                else if (s0 == -1)
                {
                    a_lo0 = _mm256_sub_epi16(a_lo0, v0);
                    a_hi0 = _mm256_sub_epi16(a_hi0, v1);
                }

                if (s1 == 1)
                {
                    a_lo1 = _mm256_add_epi16(a_lo1, v0);
                    a_hi1 = _mm256_add_epi16(a_hi1, v1);
                }
                else if (s1 == -1)
                {
                    a_lo1 = _mm256_sub_epi16(a_lo1, v0);
                    a_hi1 = _mm256_sub_epi16(a_hi1, v1);
                }

                if (s2 == 1)
                {
                    a_lo2 = _mm256_add_epi16(a_lo2, v0);
                    a_hi2 = _mm256_add_epi16(a_hi2, v1);
                }
                else if (s2 == -1)
                {
                    a_lo2 = _mm256_sub_epi16(a_lo2, v0);
                    a_hi2 = _mm256_sub_epi16(a_hi2, v1);
                }

                if (s3 == 1)
                {
                    a_lo3 = _mm256_add_epi16(a_lo3, v0);
                    a_hi3 = _mm256_add_epi16(a_hi3, v1);
                }
                else if (s3 == -1)
                {
                    a_lo3 = _mm256_sub_epi16(a_lo3, v0);
                    a_hi3 = _mm256_sub_epi16(a_hi3, v1);
                }
            }

            #define STORE_K4_ZERO_256(idx)                                                  \
                a_lo##idx = _mm256_max_epi16(_mm256_min_epi16(a_lo##idx, one16), negOne16); \
                a_hi##idx = _mm256_max_epi16(_mm256_min_epi16(a_hi##idx, one16), negOne16); \
                _mm256_storeu_si256((__m256i *)&neuronValues[targetBases[idx] + s],         \
                                    _mm256_permute4x64_epi64(_mm256_packs_epi16(a_lo##idx, a_hi##idx), 0xD8))

            STORE_K4_ZERO_256(0);
            STORE_K4_ZERO_256(1);
            STORE_K4_ZERO_256(2);
            STORE_K4_ZERO_256(3);
            #undef STORE_K4_ZERO_256
        }
    }

    // K=2 tick-zero kernel
    void processNeuronTickBlock2Zero_256(
        unsigned long long target0Base,
        unsigned long long target1Base,
        unsigned long long target0NeuronIdx,
        unsigned long long target1NeuronIdx,
        unsigned long long activeSamplePad)
    {
        const char *sign0 = &incomingSynapseWeight[target0NeuronIdx * maxNumberOfNeurons];
        const char *sign1 = &incomingSynapseWeight[target1NeuronIdx * maxNumberOfNeurons];

        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m256i acc0_0 = _mm256_setzero_si256();
            __m256i acc0_1 = _mm256_setzero_si256();
            __m256i acc1_0 = _mm256_setzero_si256();
            __m256i acc1_1 = _mm256_setzero_si256();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char sg0 = sign0[src];
                char sg1 = sign1[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m256i vec = _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]);
                __m256i v0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(vec));
                __m256i v1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vec, 1));

                if (sg0 == 1)
                {
                    acc0_0 = _mm256_add_epi16(acc0_0, v0);
                    acc0_1 = _mm256_add_epi16(acc0_1, v1);
                }
                else if (sg0 == -1)
                {
                    acc0_0 = _mm256_sub_epi16(acc0_0, v0);
                    acc0_1 = _mm256_sub_epi16(acc0_1, v1);
                }

                if (sg1 == 1)
                {
                    acc1_0 = _mm256_add_epi16(acc1_0, v0);
                    acc1_1 = _mm256_add_epi16(acc1_1, v1);
                }
                else if (sg1 == -1)
                {
                    acc1_0 = _mm256_sub_epi16(acc1_0, v0);
                    acc1_1 = _mm256_sub_epi16(acc1_1, v1);
                }
            }

            acc0_0 = _mm256_max_epi16(_mm256_min_epi16(acc0_0, one16), negOne16);
            acc0_1 = _mm256_max_epi16(_mm256_min_epi16(acc0_1, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[target0Base + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc0_0, acc0_1), 0xD8));

            acc1_0 = _mm256_max_epi16(_mm256_min_epi16(acc1_0, one16), negOne16);
            acc1_1 = _mm256_max_epi16(_mm256_min_epi16(acc1_1, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[target1Base + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc1_0, acc1_1), 0xD8));
        }
    }

    // K=1 tick-zero kernel
    void processNeuronTickZero_256(
        unsigned long long targetNeuronBase,
        unsigned long long targetNeuronIdx,
        unsigned long long activeSamplePad)
    {
        const char *sign = &incomingSynapseWeight[targetNeuronIdx * maxNumberOfNeurons];

        const __m256i one16 = _mm256_set1_epi16(1);
        const __m256i negOne16 = _mm256_set1_epi16(-1);

        for (unsigned long long s = 0; s < activeSamplePad; s += BATCH_SIZE)
        {
            __m256i acc_0 = _mm256_setzero_si256();
            __m256i acc_1 = _mm256_setzero_si256();

            for (unsigned long long i = 0; i < numCachedInputs; i++)
            {
                const unsigned long long src = inputNeuronIdxCache[i];
                char sg = sign[src];

                const unsigned long long srcBase = src * PADDED_SAMPLES;
                __m256i vec = _mm256_loadu_si256((__m256i *)&prevNeuronValues[srcBase + s]);
                __m256i v0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(vec));
                __m256i v1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(vec, 1));

                if (sg == 1)
                {
                    acc_0 = _mm256_add_epi16(acc_0, v0);
                    acc_1 = _mm256_add_epi16(acc_1, v1);
                }
                else if (sg == -1)
                {
                    acc_0 = _mm256_sub_epi16(acc_0, v0);
                    acc_1 = _mm256_sub_epi16(acc_1, v1);
                }
            }

            acc_0 = _mm256_max_epi16(_mm256_min_epi16(acc_0, one16), negOne16);
            acc_1 = _mm256_max_epi16(_mm256_min_epi16(acc_1, one16), negOne16);
            _mm256_storeu_si256((__m256i *)&neuronValues[targetNeuronBase + s],
                                _mm256_permute4x64_epi64(_mm256_packs_epi16(acc_0, acc_1), 0xD8));
        }
    }

#endif

    void processNeuronTick(unsigned long long targetNeuron, unsigned long long activeSamplePad)
    {
        const unsigned long long offset = targetNeuron * maxNumberOfNeighbors;
        const unsigned long long base = targetNeuron * PADDED_SAMPLES;
#if defined(__AVX512F__)
        processNeuronTick512(base,
                                &incomingPositiveSource[offset], incomingPositiveCount[targetNeuron],
                                &incomingNegativeSource[offset], incomingNegativeCount[targetNeuron],
                                activeSamplePad);
#else
        processNeuronTick256(base,
                                &incomingPositiveSource[offset], incomingPositiveCount[targetNeuron],
                                &incomingNegativeSource[offset], incomingNegativeCount[targetNeuron],
                                activeSamplePad);
#endif
    }

    // K=2  dispatcher
    void processNeuronTickBlock2(
        unsigned long long target0NeuronIdx,
        unsigned long long target1NeuronIdx,
        unsigned long long population,
        unsigned long long activeSamplePad)
    {
        const unsigned long long base0 = target0NeuronIdx * PADDED_SAMPLES;
        const unsigned long long base1 = target1NeuronIdx * PADDED_SAMPLES;
#if defined(__AVX512F__)
        processNeuronTickBlock2_512(base0, base1, target0NeuronIdx, target1NeuronIdx,
                                    population, activeSamplePad);
#else
        processNeuronTickBlock2_256(base0, base1, target0NeuronIdx, target1NeuronIdx,
                                    population, activeSamplePad);
#endif
    }

    // K=4 dispatcher
    void processNeuronTickBlock4(
        const unsigned long long *targetNeuronIdx, // length 4
        unsigned long long population,
        unsigned long long activeSamplePad)
    {
        unsigned long long bases[4];
        for (int k = 0; k < 4; k++)
            bases[k] = targetNeuronIdx[k] * PADDED_SAMPLES;
#if defined(__AVX512F__)
        processNeuronTickBlock4_512(bases, targetNeuronIdx, population, activeSamplePad);
#else
        processNeuronTickBlock4_256(bases, targetNeuronIdx, population, activeSamplePad);
#endif
    }

    // Compute target subset [startIdx, endIdx) of outputEvoNeuronIdxCache = [[outputs], [evolutions]]
    // K-block dispatch: K=4 primary, K=2 tail, K=1 last. Identical math to a single-pass
    // processTick — the split into two subset calls (outputs first, evolutions after the
    // exit check) lives in runTickSimulation.
    void processTickSubset(unsigned long long startIdx, unsigned long long endIdx)
    {
        unsigned long long activeSamplePad = ((activeCount + BATCH_SIZE - 1) / BATCH_SIZE) * BATCH_SIZE;
        const unsigned long long population = currentANN.population;

        {
            // PROFILE_NAMED_SCOPE("processTick:EvolutionLoop");
            unsigned long long idx = startIdx;

            // Primary path: K=4 blocks.
            for (; idx + 4 <= endIdx; idx += 4)
            {
                processNeuronTickBlock4(
                    &outputEvoNeuronIdxCache[idx],
                    population, activeSamplePad);
            }

            // Tail: K=2 block for remainder of size 2-3.
            for (; idx + 2 <= endIdx; idx += 2)
            {
                processNeuronTickBlock2(
                    outputEvoNeuronIdxCache[idx],
                    outputEvoNeuronIdxCache[idx + 1],
                    population, activeSamplePad);
            }

            // Final odd target: K=1 tail.
            for (; idx < endIdx; ++idx)
            {
                processNeuronTick(outputEvoNeuronIdxCache[idx], activeSamplePad);
            }
        }
    }

    // K=4 tick-zero dispatcher
    void processNeuronTickBlock4Zero(
        const unsigned long long *targetNeuronIdx, // length 4
        unsigned long long activeSamplePad)
    {
        unsigned long long bases[4];
        for (int k = 0; k < 4; k++)
        {
            bases[k] = targetNeuronIdx[k] * PADDED_SAMPLES;
        }
#if defined(__AVX512F__)
        processNeuronTickBlock4Zero_512(bases, targetNeuronIdx, activeSamplePad);
#else
        processNeuronTickBlock4Zero_256(bases, targetNeuronIdx, activeSamplePad);
#endif
    }

    // K=2 tick-zero dispatcher.
    void processNeuronTickBlock2Zero(
        unsigned long long target0NeuronIdx,
        unsigned long long target1NeuronIdx,
        unsigned long long activeSamplePad)
    {
        const unsigned long long base0 = target0NeuronIdx * PADDED_SAMPLES;
        const unsigned long long base1 = target1NeuronIdx * PADDED_SAMPLES;
#if defined(__AVX512F__)
        processNeuronTickBlock2Zero_512(base0, base1, target0NeuronIdx, target1NeuronIdx,
                                        activeSamplePad);
#else
        processNeuronTickBlock2Zero_256(base0, base1, target0NeuronIdx, target1NeuronIdx,
                                        activeSamplePad);
#endif
    }

    // K = 1 tick-zero dispatcher.
    void processNeuronTickZero(unsigned long long targetNeuronIdx,
                                unsigned long long activeSamplePad)
    {
        const unsigned long long base = targetNeuronIdx * PADDED_SAMPLES;
#if defined(__AVX512F__)
        processNeuronTickZero_512(base, targetNeuronIdx, activeSamplePad);
#else
        processNeuronTickZero_256(base, targetNeuronIdx, activeSamplePad);
#endif
    }

    // At tick 0, prev[output/evolution] = 0, so the accumulator for
    // every target depends ONLY on input sources
    void processTickZeroSubset(unsigned long long startIdx, unsigned long long endIdx)
    {
        unsigned long long activeSamplePad = ((activeCount + BATCH_SIZE - 1) / BATCH_SIZE) * BATCH_SIZE;
        const unsigned long long population = currentANN.population;

        {
            // PROFILE_NAMED_SCOPE("processTick:EvolutionLoop");
            unsigned long long idx = startIdx;

            // K=4 blocks with tick-zero kernel (input sources only).
            for (; idx + 4 <= endIdx; idx += 4)
            {
                processNeuronTickBlock4Zero(
                    &outputEvoNeuronIdxCache[idx],
                    activeSamplePad);
            }

            // K = 2 tail
            for (; idx + 2 <= endIdx; idx += 2)
            {
                processNeuronTickBlock2Zero(
                    outputEvoNeuronIdxCache[idx],
                    outputEvoNeuronIdxCache[idx + 1],
                    activeSamplePad);
            }

            // K = 1 tail
            for (; idx < endIdx; ++idx)
            {
                processNeuronTickZero(outputEvoNeuronIdxCache[idx], activeSamplePad);
            }
        }
    }

    void loadTrainingData()
    {
        unsigned long long population = currentANN.population;

        // Non-input are no longer zero-filled, they are handled in processNeuronTickZero
        unsigned long long inputIdx = 0;
        for (unsigned long long n = 0; n < population; n++)
        {
            if (currentANN.neuronTypes[n] == INPUT_NEURON_TYPE)
            {
                const unsigned long long offset = n * PADDED_SAMPLES;
                copyMem(&neuronValues[offset], &trainingInputs[inputIdx * PADDED_SAMPLES], PADDED_SAMPLES);
                copyMem(&prevNeuronValues[offset], &trainingInputs[inputIdx * PADDED_SAMPLES], PADDED_SAMPLES);
                inputIdx++;
            }
        }
    }

    // Convert the outgoing synapse to incomming style
    void convertToIncomingSynapses()
    {
        unsigned long long population = currentANN.population;

        unsigned long long startIdx = getSynapseStartIndex();
        unsigned long long endIdx = getSynapseEndIndex();

        setMem(incomingPositiveCount, sizeof(incomingPositiveCount), 0);
        setMem(incomingNegativeCount, sizeof(incomingNegativeCount), 0);
        setMem(incomingSynapseWeight, sizeof(incomingSynapseWeight), 0);

        for (unsigned long long n = 0; n < population; n++)
        {
            const Synapse *kSynapses = getSynapses(n);
            for (unsigned long long synIdx = startIdx; synIdx < endIdx; synIdx++)
            {
                char weight = kSynapses[synIdx];
                if (weight == 0)
                    continue;

                long long offset = bufferIndexToOffset(synIdx);
                unsigned long long nnIndex = clampNeuronIndex((long long)n, offset);
                incomingSynapseWeight[nnIndex * maxNumberOfNeurons + n] = weight;

                if (weight > 0)
                {
                    unsigned int idx = incomingPositiveCount[nnIndex]++;
                    incomingPositiveSource[nnIndex * maxNumberOfNeighbors + idx] = (unsigned int)n;
                }
                else
                {
                    unsigned int idx = incomingNegativeCount[nnIndex]++;
                    incomingNegativeSource[nnIndex * maxNumberOfNeighbors + idx] = (unsigned int)n;
                }
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

    void compactActiveSamplesWithScoring(unsigned long long &s, unsigned long long &writePos, unsigned long long population,
                                            bool outputsOnlyExitCheck = false,
                                            bool tick0Compact = false)
    {
        while (s < activeCount)
        {
            bool allUnchanged = true;
            if (outputsOnlyExitCheck)
            {
                allUnchanged = false;
            }
            else
            {
                for (unsigned long long i = 0; i < numCachedOutputs && allUnchanged; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    if (neuronValues[n * PADDED_SAMPLES + s] != prevNeuronValues[n * PADDED_SAMPLES + s])
                    {
                        allUnchanged = false;
                    }
                }
                for (unsigned long long i = 0; i < numCachedEvolution && allUnchanged; i++)
                {
                    unsigned long long n = evolutionNeuronIdxCache[i];
                    if (neuronValues[n * PADDED_SAMPLES + s] != prevNeuronValues[n * PADDED_SAMPLES + s])
                    {
                        allUnchanged = false;
                    }
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
                    // When outputsOnlyExitCheck is true, curr's evolution rows are stale
                    // and will be overwritten by the next evolution pass — skip moving them.
                    unsigned long long currEnd = outputsOnlyExitCheck ? numCurrCompactRows : numProcessNeuron;
                    unsigned long long bothStart = tick0Compact ? numCachedOutputs : 0;
                    unsigned long long prevTailEnd = tick0Compact ? 0 : numProcessNeuron;
                    for (unsigned long long n = 0; n < bothStart; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        neuronValues[baseIdx + writePos] = neuronValues[baseIdx + s];
                    }
                    for (unsigned long long n = bothStart; n < currEnd; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        neuronValues[baseIdx + writePos] = neuronValues[baseIdx + s];
                        prevNeuronValues[baseIdx + writePos] = prevNeuronValues[baseIdx + s];
                    }
                    for (unsigned long long n = currEnd; n < prevTailEnd; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        prevNeuronValues[baseIdx + writePos] = prevNeuronValues[baseIdx + s];
                    }
                }
                writePos++;
            }
            s++;
        }
    }

    bool compactActiveSamplesWithScoringSIMD(bool outputsOnlyExitCheck = false,
                                                bool tick0Compact = false)
    {
        unsigned long long population = currentANN.population;
        unsigned long long writePos = 0;
        unsigned long long s = 0;
#if defined(__AVX512F__)
        const __m512i allOnes = _mm512_set1_epi8(-1);
        while (s + BATCH_SIZE <= activeCount)
        {
            // Check 1: All neurons unchanged (curr == prev)
            // Only check output + evolution neurons (input neurons never change)
            // When called after the output-only pass, evolution values for this tick
            // haven't been computed yet — skip the unchanged check.
            unsigned long long unchangedMask = 0;
            if (!outputsOnlyExitCheck)
            {
                __m512i unchangedVec = allOnes;
                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    __m512i curr = _mm512_loadu_si512((__m512i *)&neuronValues[n * PADDED_SAMPLES + s]);
                    __m512i prev = _mm512_loadu_si512((__m512i *)&prevNeuronValues[n * PADDED_SAMPLES + s]);
                    __m512i diff = _mm512_xor_si512(curr, prev);
                    unchangedVec = _mm512_andnot_si512(diff, unchangedVec);
                }
                for (unsigned long long i = 0; i < numCachedEvolution; i++)
                {
                    unsigned long long n = evolutionNeuronIdxCache[i];
                    __m512i curr = _mm512_loadu_si512((__m512i *)&neuronValues[n * PADDED_SAMPLES + s]);
                    __m512i prev = _mm512_loadu_si512((__m512i *)&prevNeuronValues[n * PADDED_SAMPLES + s]);
                    __m512i diff = _mm512_xor_si512(curr, prev);
                    unchangedVec = _mm512_andnot_si512(diff, unchangedVec);
                }
                unchangedMask = (unsigned long long)_mm512_cmpeq_epi8_mask(unchangedVec, allOnes);
            }

            // Check 2: All output neurons non-zero
            unsigned long long allOutputsNonZeroMask = ~0ULL;
            for (unsigned long long i = 0; i < numCachedOutputs; i++)
            {
                unsigned long long n = outputNeuronIdxCache[i];
                __m512i val = _mm512_loadu_si512((__m512i *)&neuronValues[n * PADDED_SAMPLES + s]);
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
                unsigned int origSamples[BATCH_SIZE];
                unsigned char exitBitPos[BATCH_SIZE];
                unsigned int numExits = 0;
                {
                    unsigned long long tempExitMask = exitMask;
                    while (tempExitMask != 0)
                    {
                        unsigned long long bitPos = countTrailingZerosAssumeNonZero64(tempExitMask);
                        exitBitPos[numExits] = (unsigned char)bitPos;
                        origSamples[numExits] = sampleMapping[s + bitPos];
                        numExits++;
                        tempExitMask = _blsr_u64(tempExitMask);
                    }
                }

                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    unsigned long long outBase = n * PADDED_SAMPLES + s;
                    unsigned long long expBase = i * PADDED_SAMPLES;
                    for (unsigned int e = 0; e < numExits; e++)
                    {
                        unsigned int origSample = origSamples[e];
                        char actualVal = neuronValues[outBase + exitBitPos[e]];
                        char expectedVal = trainingOutputs[expBase + origSample];
                        if (actualVal == expectedVal)
                        {
                            sampleScores[origSample]++;
                        }
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
                    __m512i map0 = _mm512_loadu_si512((__m512i *)&sampleMapping[s + 0]);
                    __m512i map1 = _mm512_loadu_si512((__m512i *)&sampleMapping[s + 16]);
                    __m512i map2 = _mm512_loadu_si512((__m512i *)&sampleMapping[s + 32]);
                    __m512i map3 = _mm512_loadu_si512((__m512i *)&sampleMapping[s + 48]);
                    _mm512_storeu_si512((__m512i *)&sampleMapping[writePos + 0], map0);
                    _mm512_storeu_si512((__m512i *)&sampleMapping[writePos + 16], map1);
                    _mm512_storeu_si512((__m512i *)&sampleMapping[writePos + 32], map2);
                    _mm512_storeu_si512((__m512i *)&sampleMapping[writePos + 48], map3);

                    // Skip curr's evolution rows when outputsOnlyExitCheck=true.
                    unsigned long long currEnd = outputsOnlyExitCheck ? numCurrCompactRows : numProcessNeuron;
                    unsigned long long bothStart = tick0Compact ? numCachedOutputs : 0;
                    unsigned long long prevTailEnd = tick0Compact ? 0 : numProcessNeuron;
                    for (unsigned long long n = 0; n < bothStart; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        __m512i curr = _mm512_loadu_si512((__m512i *)&neuronValues[baseIdx + s]);
                        _mm512_storeu_si512((__m512i *)&neuronValues[baseIdx + writePos], curr);
                    }
                    for (unsigned long long n = bothStart; n < currEnd; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];

                        __m512i curr = _mm512_loadu_si512((__m512i *)&neuronValues[baseIdx + s]);
                        __m512i prev = _mm512_loadu_si512((__m512i *)&prevNeuronValues[baseIdx + s]);

                        _mm512_storeu_si512((__m512i *)&neuronValues[baseIdx + writePos], curr);
                        _mm512_storeu_si512((__m512i *)&prevNeuronValues[baseIdx + writePos], prev);
                    }
                    for (unsigned long long n = currEnd; n < prevTailEnd; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        __m512i prev = _mm512_loadu_si512((__m512i *)&prevNeuronValues[baseIdx + s]);
                        _mm512_storeu_si512((__m512i *)&prevNeuronValues[baseIdx + writePos], prev);
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
                for (unsigned long long blockId = 0; blockId < BATCH_SIZE; blockId += 16)
                {
                    __mmask16 k16 = (__mmask16)((activeMask >> blockId) & 0xFFFF);
                    __m512i map = _mm512_loadu_si512((__m512i *)&sampleMapping[s + blockId]);
                    __m512i compressed = _mm512_maskz_compress_epi32(k16, map);
                    _mm512_storeu_si512((__m512i *)&sampleMapping[writePos + offset], compressed);
                    offset += popcnt32(k16);
                }

                // Adjust the neurons and previous neuron values
                unsigned long long currEnd = outputsOnlyExitCheck ? numCurrCompactRows : numProcessNeuron;
                unsigned long long bothStart = tick0Compact ? numCachedOutputs : 0;
                unsigned long long prevTailEnd = tick0Compact ? 0 : numProcessNeuron;
                for (unsigned long long n = 0; n < bothStart; n++)
                {
                    unsigned long long baseIdx = processNeuronOffsetCache[n];
                    __m512i curr = _mm512_loadu_si512((__m512i *)&neuronValues[baseIdx + s]);
                    __m512i compCurr = _mm512_maskz_compress_epi8(kActive, curr);
                    _mm512_storeu_si512((__m512i *)&neuronValues[baseIdx + writePos], compCurr);
                }
                for (unsigned long long n = bothStart; n < currEnd; n++)
                {
                    unsigned long long baseIdx = processNeuronOffsetCache[n];
                    __m512i curr = _mm512_loadu_si512((__m512i *)&neuronValues[baseIdx + s]);
                    __m512i prev = _mm512_loadu_si512((__m512i *)&prevNeuronValues[baseIdx + s]);

                    __m512i compCurr = _mm512_maskz_compress_epi8(kActive, curr);
                    __m512i compPrev = _mm512_maskz_compress_epi8(kActive, prev);

                    _mm512_storeu_si512((__m512i *)&neuronValues[baseIdx + writePos], compCurr);
                    _mm512_storeu_si512((__m512i *)&prevNeuronValues[baseIdx + writePos], compPrev);
                }
                for (unsigned long long n = currEnd; n < prevTailEnd; n++)
                {
                    unsigned long long baseIdx = processNeuronOffsetCache[n];
                    __m512i prev = _mm512_loadu_si512((__m512i *)&prevNeuronValues[baseIdx + s]);
                    __m512i compPrev = _mm512_maskz_compress_epi8(kActive, prev);
                    _mm512_storeu_si512((__m512i *)&prevNeuronValues[baseIdx + writePos], compPrev);
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
            // Only check output + evolution neurons (input neurons never change)
            unsigned int unchangedMask = 0;
            if (!outputsOnlyExitCheck)
            {
                __m256i unchangedVec = allOnes;
                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    __m256i curr = _mm256_loadu_si256((__m256i *)&neuronValues[n * PADDED_SAMPLES + s]);
                    __m256i prev = _mm256_loadu_si256((__m256i *)&prevNeuronValues[n * PADDED_SAMPLES + s]);
                    __m256i diff = _mm256_xor_si256(curr, prev);
                    unchangedVec = _mm256_andnot_si256(diff, unchangedVec);
                }
                for (unsigned long long i = 0; i < numCachedEvolution; i++)
                {
                    unsigned long long n = evolutionNeuronIdxCache[i];
                    __m256i curr = _mm256_loadu_si256((__m256i *)&neuronValues[n * PADDED_SAMPLES + s]);
                    __m256i prev = _mm256_loadu_si256((__m256i *)&prevNeuronValues[n * PADDED_SAMPLES + s]);
                    __m256i diff = _mm256_xor_si256(curr, prev);
                    unchangedVec = _mm256_andnot_si256(diff, unchangedVec);
                }
                unchangedMask = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(unchangedVec, allOnes));
            }

            // Check 2: All output neurons non-zero
            unsigned int allOutputsNonZeroMask = ~0U;
            for (unsigned long long i = 0; i < numCachedOutputs; i++)
            {
                unsigned long long n = outputNeuronIdxCache[i];
                __m256i val = _mm256_loadu_si256((__m256i *)&neuronValues[n * PADDED_SAMPLES + s]);
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
                // Hoist sampleMapping out of the inner loop (same as the AVX-512 path).
                unsigned int origSamples[BATCH_SIZE];
                unsigned char exitBitPos[BATCH_SIZE];
                unsigned int numExits = 0;
                {
                    unsigned int tempExitMask = exitMask;
                    while (tempExitMask != 0)
                    {
                        unsigned int bitPos = countTrailingZerosAssumeNonZero32(tempExitMask);
                        exitBitPos[numExits] = (unsigned char)bitPos;
                        origSamples[numExits] = sampleMapping[s + bitPos];
                        numExits++;
                        tempExitMask = _blsr_u32(tempExitMask);
                    }
                }

                for (unsigned long long i = 0; i < numCachedOutputs; i++)
                {
                    unsigned long long n = outputNeuronIdxCache[i];
                    unsigned long long outBase = n * PADDED_SAMPLES + s;
                    unsigned long long expBase = i * PADDED_SAMPLES;
                    for (unsigned int e = 0; e < numExits; e++)
                    {
                        unsigned int origSample = origSamples[e];
                        char actualVal = neuronValues[outBase + exitBitPos[e]];
                        char expectedVal = trainingOutputs[expBase + origSample];
                        if (actualVal == expectedVal)
                        {
                            sampleScores[origSample]++;
                        }
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
                    __m256i map0 = _mm256_loadu_si256((__m256i *)&sampleMapping[s + 0]);
                    __m256i map1 = _mm256_loadu_si256((__m256i *)&sampleMapping[s + 8]);
                    __m256i map2 = _mm256_loadu_si256((__m256i *)&sampleMapping[s + 16]);
                    __m256i map3 = _mm256_loadu_si256((__m256i *)&sampleMapping[s + 24]);
                    _mm256_storeu_si256((__m256i *)&sampleMapping[writePos + 0], map0);
                    _mm256_storeu_si256((__m256i *)&sampleMapping[writePos + 8], map1);
                    _mm256_storeu_si256((__m256i *)&sampleMapping[writePos + 16], map2);
                    _mm256_storeu_si256((__m256i *)&sampleMapping[writePos + 24], map3);

                    // Skip curr's evolution rows when outputsOnlyExitCheck=true
                    unsigned long long currEnd = outputsOnlyExitCheck ? numCurrCompactRows : numProcessNeuron;
                    unsigned long long bothStart = tick0Compact ? numCachedOutputs : 0;
                    unsigned long long prevTailEnd = tick0Compact ? 0 : numProcessNeuron;
                    for (unsigned long long n = 0; n < bothStart; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        __m256i curr = _mm256_loadu_si256((__m256i *)&neuronValues[baseIdx + s]);
                        _mm256_storeu_si256((__m256i *)&neuronValues[baseIdx + writePos], curr);
                    }
                    for (unsigned long long n = bothStart; n < currEnd; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];

                        __m256i curr = _mm256_loadu_si256((__m256i *)&neuronValues[baseIdx + s]);
                        __m256i prev = _mm256_loadu_si256((__m256i *)&prevNeuronValues[baseIdx + s]);

                        _mm256_storeu_si256((__m256i *)&neuronValues[baseIdx + writePos], curr);
                        _mm256_storeu_si256((__m256i *)&prevNeuronValues[baseIdx + writePos], prev);
                    }
                    for (unsigned long long n = currEnd; n < prevTailEnd; n++)
                    {
                        unsigned long long baseIdx = processNeuronOffsetCache[n];
                        __m256i prev = _mm256_loadu_si256((__m256i *)&prevNeuronValues[baseIdx + s]);
                        _mm256_storeu_si256((__m256i *)&prevNeuronValues[baseIdx + writePos], prev);
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
                // Skip curr's evolution rows when outputsOnlyExitCheck=true
                unsigned long long currEnd = outputsOnlyExitCheck ? numCurrCompactRows : numProcessNeuron;
                unsigned long long bothStart = tick0Compact ? numCachedOutputs : 0;
                unsigned long long prevTailEnd = tick0Compact ? 0 : numProcessNeuron;
                for (unsigned long long n = 0; n < bothStart; n++)
                {
                    unsigned long long baseIdx = processNeuronOffsetCache[n];
                    mask = (unsigned int)activeMask;
                    writeOffset = 0;
                    while (mask != 0)
                    {
                        unsigned int i = countTrailingZerosAssumeNonZero32(mask);
                        neuronValues[baseIdx + writePos + writeOffset] = neuronValues[baseIdx + s + i];
                        writeOffset++;
                        mask = _blsr_u32(mask);
                    }
                }
                for (unsigned long long n = bothStart; n < currEnd; n++)
                {
                    unsigned long long baseIdx = processNeuronOffsetCache[n];
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
                for (unsigned long long n = currEnd; n < prevTailEnd; n++)
                {
                    unsigned long long baseIdx = processNeuronOffsetCache[n];
                    mask = (unsigned int)activeMask;
                    writeOffset = 0;
                    while (mask != 0)
                    {
                        unsigned int i = countTrailingZerosAssumeNonZero32(mask);
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
        compactActiveSamplesWithScoring(s, writePos, population, outputsOnlyExitCheck, tick0Compact);

        activeCount = writePos;
        return (activeCount == 0);
    }

    // Tick simulation only runs on one ANN
    void runTickSimulation()
    {
        // PROFILE_NAMED_SCOPE("runTickSimulation");

        unsigned long long population = currentANN.population;
        unsigned char *neuronTypes = currentANN.neuronTypes;
        {
            // PROFILE_NAMED_SCOPE("runTickSimulation:PrepareData");
            for (unsigned long long i = 0; i < trainingSetSize; i++)
            {
                sampleMapping[i] = (unsigned int)i;
                sampleScores[i] = 0;
            }
            activeCount = trainingSetSize;

            loadTrainingData();

            const bool k1KernelMayRun =
                (numCachedOutputs & 1ULL) || (numCachedEvolution & 1ULL);
            if (k1KernelMayRun)
            {
                decodeSynapses();
                convertToIncomingSynapses();
            }
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

                // Process output first
                {
                    // PROFILE_NAMED_SCOPE("Ticking:processTick");
                    if (tick == 0)
                    {
                        processTickZeroSubset(0, numCachedOutputs);
                    }
                    else
                    {
                        processTickSubset(0, numCachedOutputs);
                    }
                }

                // Compact ouput
                {
                    // PROFILE_NAMED_SCOPE("Ticking:compactActiveSamplesWithScoring");
                    if (compactActiveSamplesWithScoringSIMD(true, (tick == 0)))
                    {
                        break;
                    }
                }

                // Process evolution, if ouput already satisfy the exit condition, we can skip evo processing
                if (activeCount == 0)
                {
                    break;
                }
                {
                    // PROFILE_NAMED_SCOPE("Ticking:processTick");
                    if (tick == 0)
                    {
                        processTickZeroSubset(numCachedOutputs, numCachedOutputEvo);
                    }
                    else
                    {
                        processTickSubset(numCachedOutputs, numCachedOutputEvo);
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
        char *tmp = neuronValues;
        neuronValues = prevNeuronValues;
        prevNeuronValues = tmp;
    }

    unsigned int initializeANN(const unsigned char *publicKey, const unsigned char *nonce, const unsigned char *randomPool)
    {
        unsigned char hash[32];
        unsigned char combined[64];
        copyMem(combined, publicKey, 32);
        copyMem(combined + 32, nonce, 32);
        KangarooTwelve(combined, 64, hash, 32);

        unsigned long long &population = currentANN.population;
        unsigned char *neuronTypes = currentANN.neuronTypes;

        // Initialization -- fixed-topology: population is N total, set once.
        population = populationThreshold;
        neuronValues = neuronValuesBuffer0;
        prevNeuronValues = neuronValuesBuffer1;

        // Generate all 2^K possible (A, B, C) pairs
        // generateTrainingSet();

        // Initalize with nonce and public key
        random2(hash, randomPool, (unsigned char *)&paddingInitValue, sizeof(paddingInitValue));

        // Default = Input.
        for (unsigned long long i = 0; i < population; ++i)
        {
            neuronIndices[i] = i;
            neuronTypes[i] = INPUT_NEURON_TYPE;
        }

        InitValue *initValue = (InitValue *)paddingInitValue;
        unsigned long long neuronCount = population;
        // Output positions from the remaining pool
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

        // Evolution positions from the remaining pool
        for (unsigned long long i = 0; i < numberOfEvolutionNeurons; ++i)
        {
            unsigned long long evolutionNeuronIdx = initValue->evolutionNeuronPositions[i] % neuronCount;

            neuronTypes[neuronIndices[evolutionNeuronIdx]] = EVOLUTION_NEURON_TYPE;

            neuronCount = neuronCount - 1;
            neuronIndices[evolutionNeuronIdx] = neuronIndices[neuronCount];
        }

        // Synapse weight initialization, already in the 2-bit packed, just copy them
        copyMem(currentANN.synapsesPacked,
                initValue->synapseWeight,
                sizeof(currentANN.synapsesPacked));

        // Cache output / evolution neuron index lookups. Fixed-topology means neuronTypes
        // and population are set once and never change.
        cacheOutputEvoNeuronIndices();

        // Build the incoming-weight matrix and snapshot it as bestIncomingSynapseWeight.
        decodeSynapses();
        convertToIncomingSynapses();
        copyMem(bestIncomingSynapseWeight, incomingSynapseWeight, sizeof(incomingSynapseWeight));

        // Run the first inference to get starting point before mutation
        unsigned int score = inferANN();

        return score;
    }

    void copyANN(ANN &dst, const ANN &src)
    {
        copyMem(dst.neuronTypes, src.neuronTypes, maxNumberOfNeurons);
        copyMem(dst.synapsesPacked, src.synapsesPacked, sizeof(src.synapsesPacked));
        dst.population = src.population;

        // Keep incomingSynapseWeight and bestIncomingSynapseWeight in sync with the synapses they shadow.
        //   copyANN(bestANN, currentANN)  -> "accept": snapshot incomingSynapseWeight into bestIncomingSynapseWeight.
        //   copyANN(currentANN, bestANN)  -> "rollback": restore incomingSynapseWeight from bestIncomingSynapseWeight.
        if (&dst == &bestANN)
        {
            copyMem(bestIncomingSynapseWeight, incomingSynapseWeight, sizeof(incomingSynapseWeight));
        }
        else if (&dst == &currentANN)
        {
            copyMem(incomingSynapseWeight, bestIncomingSynapseWeight, sizeof(incomingSynapseWeight));
        }
    }

    // Main function for mining
    unsigned int computeScore(const unsigned char *publicKey, const unsigned char *nonce, const unsigned char *randomPool)
    {
        // PROFILE_NAMED_SCOPE("computeScore");

        // Initialize
        unsigned int bestR = initializeANN(publicKey, nonce, randomPool);
        copyANN(bestANN, currentANN);

        InitValue *initValue = (InitValue *)paddingInitValue;
        for (unsigned long long s = 0; s < numberOfMutations; ++s)
        {
            if (mutate(initValue->synapseMutation[s]))
            {
                // Ticks simulation
                unsigned int R = inferANN();

                // Roll back if neccessary
                if (R >= bestR)
                {
                    bestR = R;
                    // Better R. Save the state
                    copyANN(bestANN, currentANN);
                }
                else
                {
                    // Roll back
                    copyANN(currentANN, bestANN);
                }
            }
            else
            {
                // Input-target mutation so R is unchanged from the previous iteration.
                // copy the new currentANN into bestANN.
                copyANN(bestANN, currentANN);
            }
        }
        return bestR;
    }
};

}
