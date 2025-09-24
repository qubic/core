#pragma once
#ifdef NO_UEFI
static unsigned long long top_of_stack;
#endif
#include "platform/memory_util.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/profiling.h"
#include "public_settings.h"
#include "score_cache.h"

#if defined(_MSC_VER)

#define popcnt32(x)  static_cast<int>(__popcnt(static_cast<unsigned int>(x)))
#define popcnt64(x)  static_cast<int>(__popcnt64(static_cast<unsigned long long>(x)))

#else

#define popcnt32(x)  __builtin_popcount  (static_cast<unsigned int>(x))
#define popcnt64(x)  __builtin_popcountll(static_cast<unsigned long long>(x))

#endif


////////// Scoring algorithm \\\\\\\\\\

constexpr unsigned char INPUT_NEURON_TYPE = 0;
constexpr unsigned char OUTPUT_NEURON_TYPE = 1;
constexpr unsigned char EVOLUTION_NEURON_TYPE = 2;

#if !(defined (__AVX512F__) || defined(__AVX2__))
static_assert(false, "Either AVX2 or AVX512 is required.");
#endif

#if defined (__AVX512F__)
static constexpr int BATCH_SIZE = 64;
static constexpr int BATCH_SIZE_X8 = BATCH_SIZE * 8;
static inline int popcnt512(__m512i v)
{
    __m512i pc = _mm512_popcnt_epi64(v);
    return (int)_mm512_reduce_add_epi64(pc);
}
#elif defined(__AVX2__)
static constexpr int BATCH_SIZE = 32;
static constexpr int BATCH_SIZE_X8 = BATCH_SIZE * 8;
static inline unsigned popcnt256(__m256i v)
{
    return  popcnt64(_mm256_extract_epi64(v, 0)) +
        popcnt64(_mm256_extract_epi64(v, 1)) +
        popcnt64(_mm256_extract_epi64(v, 2)) +
        popcnt64(_mm256_extract_epi64(v, 3));
}

#endif

constexpr unsigned long long POOL_VEC_SIZE = (((1ULL << 32) + 64)) >> 3; // 2^32+64 bits ~ 512MB
constexpr unsigned long long POOL_VEC_PADDING_SIZE = (POOL_VEC_SIZE + 200 - 1) / 200 * 200; // padding for multiple of 200
constexpr unsigned long long STATE_SIZE = 200;

static void generateRandom2Pool(const unsigned char* miningSeed, unsigned char* state, unsigned char* pool)
{
    // same pool to be used by all computors/candidates and pool content changing each phase
    copyMem(&state[0], miningSeed, 32);
    setMem(&state[32], STATE_SIZE - 32, 0);

    for (unsigned int i = 0; i < POOL_VEC_PADDING_SIZE; i += STATE_SIZE)
    {
        KeccakP1600_Permute_12rounds(state);
        copyMem(&pool[i], state, STATE_SIZE);
    }
}

static void random2(
    unsigned char* seed,                // 32 bytes
    const unsigned char* pool,
    unsigned char* output,
    unsigned long long outputSizeInByte // must be divided by 64
)
{
    ASSERT(outputSizeInByte % 64 == 0);

    unsigned long long segments = outputSizeInByte / 64;
    unsigned int x[8] = { 0 };
    for (int i = 0; i < 8; i++)
    {
        x[i] = ((unsigned int*)seed)[i];
    }

    for (int j = 0; j < segments; j++)
    {
        // Each segment will have 8 elements. Each element have 8 bytes
        for (int i = 0; i < 8; i++)
        {
            unsigned int base = (x[i] >> 3) >> 3;
            unsigned int m = x[i] & 63;

            unsigned long long u64_0 = ((unsigned long long*)pool)[base];
            unsigned long long u64_1 = ((unsigned long long*)pool)[base + 1];

            // Move 8 * 8 * j to the current segment. 8 * i to current 8 bytes element
            if (m == 0)
            {
                // some compiler doesn't work with bit shift 64
                *((unsigned long long*) & output[j * 8 * 8 + i * 8]) = u64_0;
            }
            else
            {
                *((unsigned long long*) & output[j * 8 * 8 + i * 8]) = (u64_0 >> m) | (u64_1 << (64 - m));
            }

            // Increase the positions in the pool for each element.
            x[i] = x[i] * 1664525 + 1013904223; // https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
        }
    }

}

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

static void extract64Bits(unsigned long long number, char* output)
{
    int count = 0;
    for (int i = 0; i < 64; ++i)
    {
        output[i] = ((number >> i) & 1);
    }
}

static void setBitValue(unsigned char* data, unsigned long long bitIdx, unsigned char bitValue)
{
    // (data[bitIdx >> 3] & ~(1u << (bitIdx & 7u))). Set the bit at data[bitIdx >> 3] byte become zeros
    // then set it with the bit value
    data[bitIdx >> 3] = (data[bitIdx >> 3] & ~(1u << (bitIdx & 7u))) |
        (bitValue << (bitIdx & 7u));
}

static unsigned char getBitValue(const unsigned char* data, unsigned  long long  bitIdx)
{
    // data[bitIdx >> 3]: get the byte idx
    // (bitIdx & 7u) get the bit index in byte
    // (data[bitIdx >> 3] >> (bitIdx & 7u)) move the required bits to the end
    return ((data[bitIdx >> 3] >> (bitIdx & 7u)) & 1u);
}

template<unsigned long long paddedSizeInBits>
static void paddingDatabits(
    unsigned char* data,
    unsigned long long dataSizeInBits)
{
    const unsigned long long head = paddedSizeInBits;
    const unsigned long long tail = dataSizeInBits - paddedSizeInBits;

    for (unsigned r = 0; r < paddedSizeInBits; ++r)
    {
        unsigned long long src1 = tail + r + paddedSizeInBits;
        unsigned long long dst1 = (unsigned long long)r;

        unsigned char bit;

        // Copy the end to the head
        bit = getBitValue(data, src1);
        setBitValue(data, dst1, bit);

        // copy the head to the end
        unsigned long long src2 = head + r;
        unsigned long long dst2 = tail + 2 * paddedSizeInBits + r;
        bit = getBitValue(data, src2);
        setBitValue(data, dst2, bit);
    }
}

static void orShiftedMask64(unsigned long long* dst, unsigned int idx, unsigned int shift, unsigned long long mask)
{
    if (shift == 0)
    {
        dst[idx] |= mask;
    }
    else
    {
        dst[idx] |= mask << shift;
        dst[idx + 1] |= mask >> (64 - shift);
    }
}

static void orShiftedMask32(unsigned int* dst, unsigned int idx, unsigned int shift, unsigned int mask)
{
    if (shift == 0)
    {
        dst[idx] |= mask;
    }
    else
    {
        dst[idx] |= mask << shift;
        dst[idx + 1] |= mask >> (32 - shift);
    }
}

static void packNegPosWithPadding(const char* data,
    unsigned long long dataSizeInBits,
    unsigned long long paddedSizeInBits,
    unsigned char* negMask,
    unsigned char* posMask)
{
    const unsigned long long totalBits = dataSizeInBits + 2ULL * paddedSizeInBits;
    const unsigned long long totalBytes = (totalBits + 8 - 1) >> 3;
    setMem(negMask, totalBytes, 0);
    setMem(posMask, totalBytes, 0);

#if defined (__AVX512F__)
    auto* neg64 = reinterpret_cast<unsigned long long*>(negMask);
    auto* pos64 = reinterpret_cast<unsigned long long*>(posMask);
    const __m512i vMinus1 = _mm512_set1_epi8(-1);
    const __m512i vPlus1 = _mm512_set1_epi8(+1);
    unsigned long long k = 0;
    for (; k + BATCH_SIZE <= dataSizeInBits; k += BATCH_SIZE)
    {
        __m512i v = _mm512_loadu_si512(reinterpret_cast<const void*>(data + k));
        __mmask64 mNeg = _mm512_cmpeq_epi8_mask(v, vMinus1);
        __mmask64 mPos = _mm512_cmpeq_epi8_mask(v, vPlus1);

        // Start to fill data from the offset
        unsigned long long bitPos = paddedSizeInBits + k;
        unsigned int wordIdx = static_cast<unsigned int>(bitPos >> 6);  // /64 
        unsigned int offset = static_cast<unsigned int>(bitPos & 63);  // %64 
        orShiftedMask64(neg64, wordIdx, offset, static_cast<unsigned long long>(mNeg));
        orShiftedMask64(pos64, wordIdx, offset, static_cast<unsigned long long>(mPos));
    }
#else
    auto* neg32 = reinterpret_cast<unsigned int*>(negMask);
    auto* pos32 = reinterpret_cast<unsigned int*>(posMask);
    const __m256i vMinus1 = _mm256_set1_epi8(-1);
    const __m256i vPlus1 = _mm256_set1_epi8(+1);
    unsigned long long k = 0;
    for (; k + BATCH_SIZE <= dataSizeInBits; k += BATCH_SIZE)
    {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + k));

        // Compare for -1 and +1
        __m256i isNeg = _mm256_cmpeq_epi8(v, vMinus1);
        __m256i isPos = _mm256_cmpeq_epi8(v, vPlus1);

        unsigned int mNeg = static_cast<unsigned int>(_mm256_movemask_epi8(isNeg));
        unsigned int mPos = static_cast<unsigned int>(_mm256_movemask_epi8(isPos));

        // Start to fill data from the offset
        unsigned long long bitPos = paddedSizeInBits + k;
        unsigned int wordIdx = static_cast<unsigned int>(bitPos >> 5);     // / 32
        unsigned int offset = static_cast<unsigned int>(bitPos & 31);     // % 32

        orShiftedMask32(neg32, wordIdx, offset, static_cast<unsigned int>(mNeg));
        orShiftedMask32(pos32, wordIdx, offset, static_cast<unsigned int>(mPos));

    }
#endif
    // Process the remained data
    for (; k < dataSizeInBits; ++k)
    {
        char v = data[k];
        if (v == 0) continue;                        /* nothing to set */

        unsigned long long bitPos = paddedSizeInBits + k; /* logical bit index */
        unsigned long long byteIdx = bitPos >> 3;          /* byte containing it */
        unsigned shift = bitPos & 7U;          /* bit %8 */

        unsigned char mask = (unsigned char)(1U << shift);

        if (v == -1)
            negMask[byteIdx] |= mask;
        else /* v == +1 */
            posMask[byteIdx] |= mask;
    }
}

// Load 256/512 values start from a bit index into a m512 or m256 register
#if defined (__AVX512F__)
static inline __m512i load512Bits(const unsigned char* array, unsigned long long bitLocation)
{
    const unsigned long long byteIndex = bitLocation >> 3;   // /8
    const unsigned int bitOffset = (unsigned)(bitLocation & 7ULL);  // %8

    __m512i v0 = _mm512_loadu_si512((const void*)(array + byteIndex));
    if (bitOffset == 0)
        return v0;

    __m512i v1 = _mm512_loadu_si512((const void*)(array + byteIndex + 1));
    __m512i right = _mm512_srli_epi64(v0, bitOffset);       // low part
    __m512i left = _mm512_slli_epi64(v1, 8u - bitOffset);  // carry bits

    return _mm512_or_si512(right, left);
}
#else
static inline __m256i load256Bits(const unsigned char* array, unsigned long long bitLocation)
{
    const unsigned long long byteIndex = bitLocation >> 3;
    const int bitOffset = (int)(bitLocation & 7ULL);

    // Load a 256-bit (32-byte) vector starting at the byte index.
    const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(array + byteIndex));

    if (bitOffset == 0)
    {
        return v;
    }

    // Perform the right shift within each 64-bit lane.
    const __m256i right_shifted = _mm256_srli_epi64(v, bitOffset);

    //  Left-shift the +1 byte vector to align the carry bits.
    const __m256i v_shifted_by_one_byte = _mm256_loadu_si256(
        reinterpret_cast<const __m256i*>(array + byteIndex + 1)
    );
    const __m256i left_shifted_carry = _mm256_slli_epi64(v_shifted_by_one_byte, 8 - bitOffset);

    // Combine the two parts with a bitwise OR to get the final result.
    return _mm256_or_si256(right_shifted, left_shifted_carry);

}
#endif

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

    volatile char random2PoolLock;
    unsigned char state[STATE_SIZE];
    unsigned char externalPoolVec[POOL_VEC_PADDING_SIZE];
    unsigned char poolVec[POOL_VEC_PADDING_SIZE];

    void initPool(const unsigned char* miningSeed)
    {
        // Init random2 pool with mining seed
        generateRandom2Pool(miningSeed, state, externalPoolVec);
    }

    struct computeBuffer
    {
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
        unsigned long long scanRedundantNeurons()
        {
            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;

            removalNeuronsCount = 0;
            // After each mutation, we must verify if there are neurons that do not affect the ANN output.
            // These are neurons that either have all incoming synapse weights as 0,
            // or all outgoing synapse weights as 0. Such neurons must be removed.
            for (unsigned long long i = 0; i < population; i++)
            {
                if (neuronTypes[i] == EVOLUTION_NEURON_TYPE)
                {
                    if (isAllOutgoingSynapsesZeros(i) || isAllIncomingSynapsesZeros(i))
                    {
                        neuronIndices[removalNeuronsCount] = i;
                        removalNeuronsCount++;
                    }
                }
            }
            return removalNeuronsCount;
        }

        // Remove neurons and synapses that do not affect the ANN
        void cleanANN()
        {
            // Scan and remove neurons/synapses
            for (unsigned long long i = 0; i < removalNeuronsCount; i++)
            {
                unsigned long long neuronIdx = neuronIndices[i];
                // Remove it from the neuron list. Overwrite data
                // Remove its synapses in the synapses array
                removeNeuron(neuronIdx);
            }
            removalNeuronsCount = 0;
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

        // returns last computed output neurons, only returns 256 non-zero neurons, neuron values are compressed to bit
        m256i getLastOutput()
        {
            unsigned long long population = bestANN.population;
            Neuron* neurons = bestANN.neurons;
            NeuronType* neuronTypes = bestANN.neuronTypes;
            int count = 0;
            int byteCount = 0;
            uint8_t A = 0;
            m256i result;
            result = m256i::zero();

            for (unsigned long long i = 0; i < population; i++)
            {
                if (neuronTypes[i] == OUTPUT_NEURON_TYPE)
                {
                    if (neurons[i])
                    {
                        uint8_t v = (neurons[i] > 0);
                        v = v << (7 - count);
                        A |= v;
                        if (++count == 8)
                        {
                            result.m256i_u8[byteCount++] = A;
                            A = 0;
                            count = 0;
                            if (byteCount >= 32)
                            {
                                break;
                            }
                        }
                    }
                }
            }

            return result;
        }

    } _computeBuffer[solutionBufferCount];
    m256i currentRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    volatile char scoreCacheLock;
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        // Below assume when a new mining seed is provided, we need to re-calculate the random2 pool
        // Check if random pool need to be re-generated
        if (!isZero(randomSeed))
        {
            initPool(randomSeed.m256i_u8);
        }
        currentRandomSeed = randomSeed; // persist the initial random seed to be able to send it back on system info response

        ACQUIRE(random2PoolLock);
        copyMem(poolVec, externalPoolVec, POOL_VEC_PADDING_SIZE);
        RELEASE(random2PoolLock);
    }

    ~ScoreFunction()
    {
        freeMemory();
    }

    void freeMemory()
    {
    }

    bool initMemory()
    {
        random2PoolLock = 0;

        // Make sure all padding data is set as zeros
        setMem(_computeBuffer, sizeof(_computeBuffer), 0);

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

    unsigned int computeScore(const unsigned long long solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        return _computeBuffer[solutionBufIdx].computeScore(publicKey.m256i_u8, nonce.m256i_u8, poolVec);
    }

    m256i getLastOutput(const unsigned long long processor_Number)
    {
        ACQUIRE(solutionEngineLock[processor_Number]);

        m256i result = _computeBuffer[processor_Number].getLastOutput();

        RELEASE(solutionEngineLock[processor_Number]);
        return result;
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

        score = computeScore(solutionBufIdx, publicKey, nonce);

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
