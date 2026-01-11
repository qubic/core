#pragma once

#include "platform/profiling.h"
#include "platform/memory_util.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "kangaroo_twelve.h"

namespace score_engine
{

enum AlgoType
{
    HyperIdentity = 0,
    Addition = 1,
    MaxAlgoCount // for counting current supported ago
};

// =============================================================================
// Algorithm 0: HyperIdentity Parameters
// =============================================================================
template<
    unsigned long long inputNeurons,   // numberOfInputNeurons
    unsigned long long outputNeurons,   // numberOfOutputNeurons
    unsigned long long ticks,   // numberOfTicks
    unsigned long long neighbor,  // numberOfNeighbors
    unsigned long long population,   // populationThreshold
    unsigned long long mutations,   // numberOfMutations
    unsigned int threshold          // solutionThreshold
>
struct HyperIdentityParams
{
    static constexpr unsigned long long numberOfInputNeurons = inputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = outputNeurons;
    static constexpr unsigned long long numberOfTicks = ticks;
    static constexpr unsigned long long numberOfNeighbors = neighbor;
    static constexpr unsigned long long populationThreshold = population;
    static constexpr unsigned long long numberOfMutations = mutations;
    static constexpr unsigned int solutionThreshold = threshold;

    static constexpr AlgoType algoType = AlgoType::HyperIdentity;
    static constexpr unsigned int paramsCount = 7;
};

// =============================================================================
// Algorithm 1: Addition Parameters
// =============================================================================
template<
    unsigned long long inputNeurons,   // numberOfInputNeurons
    unsigned long long outputNeurons,   // numberOfOutputNeurons
    unsigned long long ticks,   // numberOfTicks
    unsigned long long neighbor,  // maxNumberOfNeigbor
    unsigned long long population,   // populationThreshold
    unsigned long long mutations,   // numberOfMutations
    unsigned int threshold          // solutionThreshold
>
struct AdditionParams
{
    static constexpr unsigned long long numberOfInputNeurons = inputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = outputNeurons;
    static constexpr unsigned long long numberOfTicks = ticks;
    static constexpr unsigned long long numberOfNeighbors = neighbor;
    static constexpr unsigned long long populationThreshold = population;
    static constexpr unsigned long long numberOfMutations = mutations;
    static constexpr unsigned int solutionThreshold = threshold;

    static constexpr AlgoType algoType = AlgoType::Addition;
    static constexpr unsigned int paramsCount = 7;
};

//=================================================================================================
// Defines and constants
static constexpr unsigned int DEFAUL_SOLUTION_THRESHOLD[AlgoType::MaxAlgoCount] = { 
    HYPERIDENTITY_SOLUTION_THRESHOLD_DEFAULT, 
    ADDITION_SOLUTION_THRESHOLD_DEFAULT};
static constexpr unsigned int INVALID_SCORE_VALUE = 0xFFFFFFFFU;
static constexpr long long NEURON_VALUE_LIMIT = 1LL;

constexpr unsigned char INPUT_NEURON_TYPE = 0;
constexpr unsigned char OUTPUT_NEURON_TYPE = 1;
constexpr unsigned char EVOLUTION_NEURON_TYPE = 2;

constexpr unsigned long long POOL_VEC_SIZE = (((1ULL << 32) + 64)) >> 3; // 2^32+64 bits ~ 512MB
constexpr unsigned long long POOL_VEC_PADDING_SIZE = (POOL_VEC_SIZE + 200 - 1) / 200 * 200; // padding for multiple of 200
constexpr unsigned long long STATE_SIZE = 200;

//=================================================================================================
// Helper functions



#if defined(_MSC_VER)

#define popcnt32(x)  static_cast<int>(__popcnt(static_cast<unsigned int>(x)))
#define popcnt64(x)  static_cast<int>(__popcnt64(static_cast<unsigned long long>(x)))

// Does not handle the x = 0. Expect check zeros before usage
static inline unsigned long long countTrailingZerosAssumeNonZero64(unsigned long long x)
{
    unsigned long index;
    _BitScanForward64(&index, x);
    return index;
}

// Does not handle the x = 0. Expect check zeros before usage
static inline unsigned int countTrailingZerosAssumeNonZero32(unsigned int x)
{
    unsigned long index;
    _BitScanForward(&index, x);
    return index;
}

#else

#define popcnt32(x)  __builtin_popcount  (static_cast<unsigned int>(x))
#define popcnt64(x)  __builtin_popcountll(static_cast<unsigned long long>(x))

// Does not handle the x = 0. Expect check zeros before usage
static inline unsigned long long countTrailingZerosAssumeNonZero64(unsigned long long x)
{
    return __builtin_ctzll(x);
}

// Does not handle the x = 0. Expect check zeros before usage
static inline unsigned int countTrailingZerosAssumeNonZero32(unsigned int x)
{
    return __builtin_ctz(x);
}

#endif

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
static char clampNeuron(T val)
{
    if (val > 1)
    {
        return 1;
    }
    else if (val < -1)
    {
        return -1;
    }
    return static_cast<char>(val);
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

template <unsigned long long bitCount>
static void toTenaryBits(long long A, char* bits)
{
    for (unsigned long long i = 0; i < bitCount; ++i)
    {
        char bitValue = static_cast<char>((A >> i) & 1);
        bits[i] = (bitValue == 0) ? -1 : bitValue;
    }
}

static int extractLastOutput(
    const unsigned char* data,
    const unsigned char* dataType,
    const unsigned long long dataSize,
    unsigned char* requestedOutput,
    int requestedSizeInBytes)
{
    setMem(requestedOutput, requestedSizeInBytes, 0);

    int byteCount = 0;
    int bitCount = 0;
    unsigned char currentByte = 0;

    for (unsigned long long i = 0; i < dataSize && byteCount < requestedSizeInBytes; i++)
    {
        if (dataType[i] == OUTPUT_NEURON_TYPE)
        {
            // Skip zero data
            if (data[i] == 0)
            {
                continue;
            }

            // Pack sign bit: 1 if positive, 0 if negative
            unsigned char bit = (data[i] > 0) ? 1 : 0;
            currentByte |= (bit << (7 - bitCount));
            bitCount++;

            // Byte complete
            if (bitCount == 8)
            {
                requestedOutput[byteCount++] = currentByte;
                currentByte = 0;
                bitCount = 0;
            }
        }
    }

    // Write final partial byte if any bits were set
    if (bitCount > 0 && byteCount < requestedSizeInBytes)
    {
        requestedOutput[byteCount++] = currentByte;
    }

    return byteCount;
}

// Clamp the index in cirulate
// This only work with |value| < population
static inline unsigned long long clampCirculatingIndex(
    long long population,
    long long neuronIdx,
    long long value)
{
    long long nnIndex = neuronIdx + value;

    // Get the signed bit and decide if we should increase population
    nnIndex += (population & (nnIndex >> 63));

    // Subtract population if idx >= population
    long long over = nnIndex - population;
    nnIndex -= (population & ~(over >> 63));
    return (unsigned long long)nnIndex;
}

// Get the solution threshold depend on nonce
// In case of not provided threholdBuffer, just return the default value
static AlgoType getAlgoType(const unsigned char* nonce)
{
    AlgoType selectedAgo = ((nonce[0] & 1) == 0) ? AlgoType::HyperIdentity : AlgoType::Addition;
    return selectedAgo;
}

// Verify if the solution threshold is valid
static bool checkAlgoThreshold(int threshold, AlgoType algo)
{
    if (threshold <= 0)
    {
        return false;
    }

    switch (algo)
    {
    case AlgoType::HyperIdentity:
        if (threshold > HYPERIDENTITY_NUMBER_OF_OUTPUT_NEURONS)
        {
            return false;
        }
        break;
    case AlgoType::Addition:
        if (threshold > ADDITION_NUMBER_OF_OUTPUT_NEURONS * (1U << ADDITION_NUMBER_OF_INPUT_NEURONS))
        {
            return false;
        }
        break;
    default:
        return false;
    }
    return true;
}

}
