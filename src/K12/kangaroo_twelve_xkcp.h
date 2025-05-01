#pragma once

#include <lib/platform_common/qintrin.h>

#include "platform/memory.h"

namespace XKCP{

typedef unsigned char uint8_t;
typedef unsigned long long uint64_t;

#ifdef ALIGN
#undef ALIGN
#endif

#if defined(__GNUC__)
#define ALIGN(x) __attribute__ ((aligned(x)))
#elif defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#elif defined(__ARMCC_VERSION)
#define ALIGN(x) __align(x)
#else
#define ALIGN(x)
#endif

#define KeccakP1600_stateSizeInBytes    200
#define KeccakP1600_stateAlignment      8

// Forward declarations
namespace K12xkcp
{
    void KeccakP1600_Initialize(void *state);
    void KeccakP1600_AddByte(void *state, unsigned char data, unsigned int offset);
    void KeccakP1600_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
    void KeccakP1600_Permute_12rounds(void *state);
    void KeccakP1600_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
    size_t KeccakP1600_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);
}


// TurboSHAKE Code

typedef struct TurboSHAKE_InstanceStruct {
    uint8_t state[KeccakP1600_stateSizeInBytes];
    unsigned int rate;
    uint8_t byteIOIndex;
    uint8_t squeezing;
} TurboSHAKE_Instance;


typedef struct KangarooTwelve_InstanceStruct {
    ALIGN(KeccakP1600_stateAlignment) TurboSHAKE_Instance queueNode;
    ALIGN(KeccakP1600_stateAlignment) TurboSHAKE_Instance finalNode;
    size_t fixedOutputLength;
    size_t blockNumber;
    unsigned int queueAbsorbedLen;
    int phase;
    int securityLevel;
} KangarooTwelve_Instance;

static void TurboSHAKE_Initialize(TurboSHAKE_Instance* instance, int securityLevel)
{
    K12xkcp::KeccakP1600_Initialize(instance->state);
    instance->rate = (1600 - (2 * securityLevel));
    instance->byteIOIndex = 0;
    instance->squeezing = 0;
}

static void TurboSHAKE_Absorb(TurboSHAKE_Instance* instance, const unsigned char* data, size_t dataByteLen)
{
    size_t i, j;
    uint8_t partialBlock;
    const unsigned char* curData;

    const uint8_t rateInBytes = instance->rate / 8;

    // assert(instance->squeezing == 0);

    i = 0;
    curData = data;
    while (i < dataByteLen)
    {
        if ((instance->byteIOIndex == 0) && (dataByteLen - i >= rateInBytes))
        {
#ifdef KeccakP1600_12rounds_FastLoop_supported
            /* processing full blocks first */
            j = KeccakP1600_12rounds_FastLoop_Absorb(instance->state, instance->rate / 64, curData, dataByteLen - i);
            i += j;
            curData += j;
#endif
            for (j = dataByteLen - i; j >= rateInBytes; j -= rateInBytes)
            {
                K12xkcp::KeccakP1600_AddBytes(instance->state, curData, 0, rateInBytes);
                K12xkcp::KeccakP1600_Permute_12rounds(instance->state);
                curData += rateInBytes;
            }
            i = dataByteLen - j;
        }
        else
        {
            /* normal lane: using the message queue */
            if (dataByteLen - i > (size_t)rateInBytes - instance->byteIOIndex)
            {
                partialBlock = rateInBytes - instance->byteIOIndex;
            }
            else
            {
                partialBlock = (uint8_t)(dataByteLen - i);
            }
            i += partialBlock;

            K12xkcp::KeccakP1600_AddBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
            curData += partialBlock;
            instance->byteIOIndex += partialBlock;
            if (instance->byteIOIndex == rateInBytes)
            {
                K12xkcp::KeccakP1600_Permute_12rounds(instance->state);
                instance->byteIOIndex = 0;
            }
        }
    }
}

static void TurboSHAKE_AbsorbDomainSeparationByte(TurboSHAKE_Instance* instance, unsigned char D)
{
    const unsigned int rateInBytes = instance->rate / 8;

    // assert(D != 0);
    // assert(instance->squeezing == 0);

    /* Last few bits, whose delimiter coincides with first bit of padding */
    K12xkcp::KeccakP1600_AddByte(instance->state, D, instance->byteIOIndex);
    /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */
    if ((D >= 0x80) && (instance->byteIOIndex == (rateInBytes - 1)))
        K12xkcp::KeccakP1600_Permute_12rounds(instance->state);
    /* Second bit of padding */
    K12xkcp::KeccakP1600_AddByte(instance->state, 0x80, rateInBytes - 1);
    K12xkcp::KeccakP1600_Permute_12rounds(instance->state);
    instance->byteIOIndex = 0;
    instance->squeezing = 1;
}

static void TurboSHAKE_Squeeze(TurboSHAKE_Instance* instance, unsigned char* data, size_t dataByteLen)
{
    size_t i, j;
    unsigned int partialBlock;
    const unsigned int rateInBytes = instance->rate / 8;
    unsigned char* curData;

    if (!instance->squeezing)
        TurboSHAKE_AbsorbDomainSeparationByte(instance, 0x01);

    i = 0;
    curData = data;
    while (i < dataByteLen)
    {
        if ((instance->byteIOIndex == rateInBytes) && (dataByteLen - i >= rateInBytes))
        {
            for (j = dataByteLen - i; j >= rateInBytes; j -= rateInBytes)
            {
                K12xkcp::KeccakP1600_Permute_12rounds(instance->state);
                K12xkcp::KeccakP1600_ExtractBytes(instance->state, curData, 0, rateInBytes);
                curData += rateInBytes;
            }
            i = dataByteLen - j;
        }
        else
        {
            /* normal lane: using the message queue */
            if (instance->byteIOIndex == rateInBytes)
            {
                K12xkcp::KeccakP1600_Permute_12rounds(instance->state);
                instance->byteIOIndex = 0;
            }
            if (dataByteLen - i > rateInBytes - instance->byteIOIndex)
                partialBlock = rateInBytes - instance->byteIOIndex;
            else
                partialBlock = (unsigned int)(dataByteLen - i);
            i += partialBlock;

            K12xkcp::KeccakP1600_ExtractBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
            curData += partialBlock;
            instance->byteIOIndex += partialBlock;
        }
    }
}

typedef enum
{
    NOT_INITIALIZED,
    ABSORBING,
    FINAL,
    SQUEEZING
} KCP_Phases;
typedef KCP_Phases KangarooTwelve_Phases;

#define K12_chunkSize         8192
#define K12_suffixLeaf        0x0B /* '110': message hop, simple padding, inner node */
#define KT128_capacityInBytes 32
#define KT256_capacityInBytes 64

#define maxCapacityInBytes 64

static unsigned int right_encode(unsigned char* encbuf, size_t value)
{
    unsigned int n, i;
    size_t v;

    for (v = value, n = 0; v && (n < sizeof(size_t)); ++n, v >>= 8)
        ; /* empty */
    for (i = 1; i <= n; ++i)
    {
        encbuf[i - 1] = (unsigned char)(value >> (8 * (n - i)));
    }
    encbuf[n] = (unsigned char)n;
    return n + 1;
}

// K12 Code

/**
  * Function to initialize a KangarooTwelve instance.
  * @param  ktInstance      Pointer to the instance to be initialized.
  * @param  securityLevel   The desired security strength level (128 bits or 256 bits).
  * @param  outputByteLen   The desired number of output bytes,
  *                         or 0 for an arbitrarily-long output.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Initialize(KangarooTwelve_Instance* ktInstance, int securityLevel, size_t outputByteLen)
{
    ktInstance->fixedOutputLength = outputByteLen;
    ktInstance->queueAbsorbedLen = 0;
    ktInstance->blockNumber = 0;
    ktInstance->phase = ABSORBING;
    ktInstance->securityLevel = securityLevel;
    TurboSHAKE_Initialize(&ktInstance->finalNode, securityLevel);
    return 0;
}


/**
  * Function to give input data to be absorbed.
  * @param  ktInstance      Pointer to the instance initialized by KangarooTwelve_Initialize().
  * @param  input           Pointer to the input message data (M).
  * @param  inputByteLen    The number of bytes provided in the input message data.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Update(KangarooTwelve_Instance* ktInstance, const unsigned char* input, size_t inputByteLen)
{
    if (ktInstance->phase != ABSORBING)
        return 1;

    if (ktInstance->blockNumber == 0)
    {
        /* First block, absorb in final node */
        unsigned int len = (inputByteLen < (K12_chunkSize - ktInstance->queueAbsorbedLen)) ? (unsigned int)inputByteLen : (K12_chunkSize - ktInstance->queueAbsorbedLen);
        TurboSHAKE_Absorb(&ktInstance->finalNode, input, len);
        input += len;
        inputByteLen -= len;
        ktInstance->queueAbsorbedLen += len;
        if ((ktInstance->queueAbsorbedLen == K12_chunkSize) && (inputByteLen != 0))
        {
            /* First block complete and more input data available, finalize it */
            const unsigned char padding = 0x03; /* '110^6': message hop, simple padding */
            ktInstance->queueAbsorbedLen = 0;
            ktInstance->blockNumber = 1;
            TurboSHAKE_Absorb(&ktInstance->finalNode, &padding, 1);
            ktInstance->finalNode.byteIOIndex = (ktInstance->finalNode.byteIOIndex + 7) & ~7; /* Zero padding up to 64 bits */
        }
    }
    else if (ktInstance->queueAbsorbedLen != 0)
    {
        /* There is data in the queue, absorb further in queue until block complete */
        unsigned int len = (inputByteLen < (K12_chunkSize - ktInstance->queueAbsorbedLen)) ? (unsigned int)inputByteLen : (K12_chunkSize - ktInstance->queueAbsorbedLen);
        TurboSHAKE_Absorb(&ktInstance->queueNode, input, len);
        input += len;
        inputByteLen -= len;
        ktInstance->queueAbsorbedLen += len;
        if (ktInstance->queueAbsorbedLen == K12_chunkSize)
        {
            int capacityInBytes = 2 * (ktInstance->securityLevel) / 8;
            unsigned char intermediate[maxCapacityInBytes];
            // assert(capacityInBytes <= maxCapacityInBytes);
            ktInstance->queueAbsorbedLen = 0;
            ++ktInstance->blockNumber;
            TurboSHAKE_AbsorbDomainSeparationByte(&ktInstance->queueNode, K12_suffixLeaf);
            TurboSHAKE_Squeeze(&ktInstance->queueNode, intermediate, capacityInBytes);
            TurboSHAKE_Absorb(&ktInstance->finalNode, intermediate, capacityInBytes);
        }
    }

    while (inputByteLen > 0)
    {
        unsigned int len = (inputByteLen < K12_chunkSize) ? (unsigned int)inputByteLen : K12_chunkSize;
        TurboSHAKE_Initialize(&ktInstance->queueNode, ktInstance->securityLevel);
        TurboSHAKE_Absorb(&ktInstance->queueNode, input, len);
        input += len;
        inputByteLen -= len;
        if (len == K12_chunkSize)
        {
            int capacityInBytes = 2 * (ktInstance->securityLevel) / 8;
            unsigned char intermediate[maxCapacityInBytes];
            // assert(capacityInBytes <= maxCapacityInBytes);
            ++ktInstance->blockNumber;
            TurboSHAKE_AbsorbDomainSeparationByte(&ktInstance->queueNode, K12_suffixLeaf);
            TurboSHAKE_Squeeze(&ktInstance->queueNode, intermediate, capacityInBytes);
            TurboSHAKE_Absorb(&ktInstance->finalNode, intermediate, capacityInBytes);
        }
        else
        {
            ktInstance->queueAbsorbedLen = len;
        }
    }

    return 0;
}

/**
  * Function to call after all the input message has been input, and to get
  * output bytes if the length was specified when calling KangarooTwelve_Initialize().
  * @param  ktInstance      Pointer to the hash instance initialized by KangarooTwelve_Initialize().
  * If @a outputByteLen was not 0 in the call to KangarooTwelve_Initialize(), the number of
  *     output bytes is equal to @a outputByteLen.
  * If @a outputByteLen was 0 in the call to KangarooTwelve_Initialize(), the output bytes
  *     must be extracted using the KangarooTwelve_Squeeze() function.
  * @param  output          Pointer to the buffer where to store the output data.
  * @param  customization   Pointer to the customization string (C).
  * @param  customByteLen   The length of the customization string in bytes.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Final(KangarooTwelve_Instance* ktInstance, unsigned char* output, const unsigned char* customization, size_t customByteLen)
{
    unsigned char encbuf[sizeof(size_t) + 1 + 2];
    unsigned char padding;

    if (ktInstance->phase != ABSORBING)
        return 1;

    /* Absorb customization | right_encode(customByteLen) */
    if ((customByteLen != 0) && (KangarooTwelve_Update(ktInstance, customization, customByteLen) != 0))
        return 1;
    if (KangarooTwelve_Update(ktInstance, encbuf, right_encode(encbuf, customByteLen)) != 0)
        return 1;

    if (ktInstance->blockNumber == 0)
    {
        /* Non complete first block in final node, pad it */
        padding = 0x07; /*  '11': message hop, final node */
    }
    else
    {
        unsigned int n;

        if (ktInstance->queueAbsorbedLen != 0)
        {
            /* There is data in the queue node */
            int capacityInBytes = 2 * (ktInstance->securityLevel) / 8;
            unsigned char intermediate[maxCapacityInBytes];
            // assert(capacityInBytes <= maxCapacityInBytes);
            ++ktInstance->blockNumber;
            TurboSHAKE_AbsorbDomainSeparationByte(&ktInstance->queueNode, K12_suffixLeaf);
            TurboSHAKE_Squeeze(&ktInstance->queueNode, intermediate, capacityInBytes);
            TurboSHAKE_Absorb(&ktInstance->finalNode, intermediate, capacityInBytes);
        }
        --ktInstance->blockNumber; /* Absorb right_encode(number of Chaining Values) || 0xFF || 0xFF */
        n = right_encode(encbuf, ktInstance->blockNumber);
        encbuf[n++] = 0xFF;
        encbuf[n++] = 0xFF;
        TurboSHAKE_Absorb(&ktInstance->finalNode, encbuf, n);
        padding = 0x06; /* '01': chaining hop, final node */
    }
    TurboSHAKE_AbsorbDomainSeparationByte(&ktInstance->finalNode, padding);
    if (ktInstance->fixedOutputLength != 0)
    {
        ktInstance->phase = FINAL;
        TurboSHAKE_Squeeze(&ktInstance->finalNode, output, ktInstance->fixedOutputLength);
        return 0;
    }
    ktInstance->phase = SQUEEZING;
    return 0;
}

/**
  * Function to squeeze output data.
  * @param  ktInstance     Pointer to the hash instance initialized by KangarooTwelve_Initialize().
  * @param  data           Pointer to the buffer where to store the output data.
  * @param  outputByteLen  The number of output bytes desired.
  * @pre    KangarooTwelve_Final() must have been already called.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Squeeze(KangarooTwelve_Instance* ktInstance, unsigned char* output, size_t outputByteLen)
{
    if (ktInstance->phase != SQUEEZING)
        return 1;
    TurboSHAKE_Squeeze(&ktInstance->finalNode, output, outputByteLen);
    return 0;
}

/** Extendable ouput function KangarooTwelve.
* @param  input           Pointer to the input message (M).
* @param  inputByteLen    The length of the input message in bytes.
* @param  output          Pointer to the output buffer.
* @param  outputByteLen   The desired number of output bytes.
* @param  customization   Pointer to the customization string (C).
* @param  customByteLen   The length of the customization string in bytes.
* @param  securityLevel   The desired security strength level (128 bits or 256 bits).
* @return 0 if successful, 1 otherwise.
*/

int KangarooTwelveXKPC(int securityLevel, const unsigned char* input, size_t inputByteLen,
                       unsigned char* output, size_t outputByteLen,
                       const unsigned char* customization, size_t customByteLen)
{
    KangarooTwelve_Instance ktInstance;

    if (outputByteLen == 0)
        return 1;
    KangarooTwelve_Initialize(&ktInstance, securityLevel, outputByteLen);
    if (KangarooTwelve_Update(&ktInstance, input, inputByteLen) != 0)
        return 1;
    return KangarooTwelve_Final(&ktInstance, output, customization, customByteLen);
}

/**
 * Wrapper around `KangarooTwelve` to use the 128-bit security level and no customization String.
 */
static void KangarooTwelve(const unsigned char* input, unsigned int inputByteLen, unsigned char* output, unsigned int outputByteLen)
{
    int sts = KangarooTwelveXKPC(128, input, (size_t)inputByteLen, output, (size_t)outputByteLen, nullptr, 0);
}

static void random(const unsigned char* publicKey, const unsigned char* nonce, unsigned char* output, unsigned long long outputSize)
{
    unsigned char state[200];
    *((__m256i*)&state[0]) = *((__m256i*)publicKey);
    *((__m256i*)&state[32]) = *((__m256i*)nonce);
    setMem(&state[64], sizeof(state) - 64, 0);

    for (unsigned long long i = 0; i < outputSize / sizeof(state); i++)
    {
        K12xkcp::KeccakP1600_Permute_12rounds(state);
        copyMem(output, state, sizeof(state));
        output += sizeof(state);
    }
    if (outputSize % sizeof(state))
    {
        K12xkcp::KeccakP1600_Permute_12rounds(state);
        copyMem(output, state, outputSize % sizeof(state));
    }
}

#ifdef __AVX512F__

/*
This code is adopted from
https://github.com/XKCP/K12/blob/master/lib/Optimized64/KeccakP-1600-AVX512-plainC.c

K12 based on the eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP

The Keccak-p permutations, designed by Guido Bertoni, Joan Daemen, Micha�l Peeters and Gilles Van Assche.

Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/

---

We would like to thank Vladimir Sedach, we have used parts of his Keccak AVX-512 C++ code.
 */

typedef __m512i     V512;

#define XOR(a,b)                    _mm512_xor_si512(a,b)
#define XOR3(a,b,c)                 _mm512_ternarylogic_epi64(a,b,c,0x96)
#define XOR5(a,b,c,d,e)             XOR3(XOR3(a,b,c),d,e)
#define ROL(a,offset)               _mm512_rol_epi64(a,offset)
#define Chi(a,b,c)                  _mm512_ternarylogic_epi64(a,b,c,0xD2)

    /* ---------------------------------------------------------------- */

#define LOAD_Lanes(m,a)             _mm512_maskz_loadu_epi64(m,a)
#define LOAD_Lane(a)                LOAD_Lanes(0x01,a)
#define LOAD_Plane(a)               LOAD_Lanes(0x1F,a)
#define LOAD_8Lanes(a)              LOAD_Lanes(0xFF,a)
#define STORE_Lanes(a,m,v)          _mm512_mask_storeu_epi64(a,m,v)
#define STORE_Lane(a,v)             STORE_Lanes(a,0x01,v)
#define STORE_Plane(a,v)            STORE_Lanes(a,0x1F,v)
#define STORE_8Lanes(a,v)           STORE_Lanes(a,0xFF,v)

    /* ---------------------------------------------------------------- */

#define KeccakP_DeclareVars \
    V512    b0, b1, b2, b3, b4; \
    V512    Baeiou, Gaeiou, Kaeiou, Maeiou, Saeiou; \
    V512    moveThetaPrev = _mm512_setr_epi64(4, 0, 1, 2, 3, 5, 6, 7); \
    V512    moveThetaNext = _mm512_setr_epi64(1, 2, 3, 4, 0, 5, 6, 7); \
    V512    rhoB = _mm512_setr_epi64( 0,  1, 62, 28, 27, 0, 0, 0); \
    V512    rhoG = _mm512_setr_epi64(36, 44,  6, 55, 20, 0, 0, 0); \
    V512    rhoK = _mm512_setr_epi64( 3, 10, 43, 25, 39, 0, 0, 0); \
    V512    rhoM = _mm512_setr_epi64(41, 45, 15, 21,  8, 0, 0, 0); \
    V512    rhoS = _mm512_setr_epi64(18,  2, 61, 56, 14, 0, 0, 0); \
    V512    pi1B = _mm512_setr_epi64(0, 3, 1, 4, 2, 5, 6, 7); \
    V512    pi1G = _mm512_setr_epi64(1, 4, 2, 0, 3, 5, 6, 7); \
    V512    pi1K = _mm512_setr_epi64(2, 0, 3, 1, 4, 5, 6, 7); \
    V512    pi1M = _mm512_setr_epi64(3, 1, 4, 2, 0, 5, 6, 7); \
    V512    pi1S = _mm512_setr_epi64(4, 2, 0, 3, 1, 5, 6, 7); \
    V512    pi2S1 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 0+8, 2+8); \
    V512    pi2S2 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 1+8, 3+8); \
    V512    pi2BG = _mm512_setr_epi64(0, 1, 0+8, 1+8, 6, 5, 6, 7); \
    V512    pi2KM = _mm512_setr_epi64(2, 3, 2+8, 3+8, 7, 5, 6, 7); \
    V512    pi2S3 = _mm512_setr_epi64(4, 5, 4+8, 5+8, 4, 5, 6, 7);

    /* ---------------------------------------------------------------- */

#define KeccakP_DeclareVars2 \
    V512    b0, b1, b2, b3, b4; \
    V512    moveThetaPrev = _mm512_setr_epi64(4, 0, 1, 2, 3, 5, 6, 7); \
    V512    moveThetaNext = _mm512_setr_epi64(1, 2, 3, 4, 0, 5, 6, 7); \
    V512    rhoB = _mm512_setr_epi64( 0,  1, 62, 28, 27, 0, 0, 0); \
    V512    rhoG = _mm512_setr_epi64(36, 44,  6, 55, 20, 0, 0, 0); \
    V512    rhoK = _mm512_setr_epi64( 3, 10, 43, 25, 39, 0, 0, 0); \
    V512    rhoM = _mm512_setr_epi64(41, 45, 15, 21,  8, 0, 0, 0); \
    V512    rhoS = _mm512_setr_epi64(18,  2, 61, 56, 14, 0, 0, 0); \
    V512    pi1B = _mm512_setr_epi64(0, 3, 1, 4, 2, 5, 6, 7); \
    V512    pi1G = _mm512_setr_epi64(1, 4, 2, 0, 3, 5, 6, 7); \
    V512    pi1K = _mm512_setr_epi64(2, 0, 3, 1, 4, 5, 6, 7); \
    V512    pi1M = _mm512_setr_epi64(3, 1, 4, 2, 0, 5, 6, 7); \
    V512    pi1S = _mm512_setr_epi64(4, 2, 0, 3, 1, 5, 6, 7); \
    V512    pi2S1 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 0+8, 2+8); \
    V512    pi2S2 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 1+8, 3+8); \
    V512    pi2BG = _mm512_setr_epi64(0, 1, 0+8, 1+8, 6, 5, 6, 7); \
    V512    pi2KM = _mm512_setr_epi64(2, 3, 2+8, 3+8, 7, 5, 6, 7); \
    V512    pi2S3 = _mm512_setr_epi64(4, 5, 4+8, 5+8, 4, 5, 6, 7);

    /* ---------------------------------------------------------------- */

#define XKCPAVX512copyFromState(pState) \
    Baeiou = LOAD_Plane(pState+ 0); \
    Gaeiou = LOAD_Plane(pState+ 5); \
    Kaeiou = LOAD_Plane(pState+10); \
    Maeiou = LOAD_Plane(pState+15); \
    Saeiou = LOAD_Plane(pState+20);

    /* ---------------------------------------------------------------- */

#define XKCPAVX512copyToState(pState) \
    STORE_Plane(pState+ 0, Baeiou); \
    STORE_Plane(pState+ 5, Gaeiou); \
    STORE_Plane(pState+10, Kaeiou); \
    STORE_Plane(pState+15, Maeiou); \
    STORE_Plane(pState+20, Saeiou);

    /* ---------------------------------------------------------------- */

#define KeccakP_Round(i) \
    /* Theta */ \
    b0 = XOR5( Baeiou, Gaeiou, Kaeiou, Maeiou, Saeiou ); \
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0); \
    b0 = _mm512_permutexvar_epi64(moveThetaNext, b0); \
    b0 = _mm512_rol_epi64(b0, 1); \
    Baeiou = XOR3( Baeiou, b0, b1 ); \
    Gaeiou = XOR3( Gaeiou, b0, b1 ); \
    Kaeiou = XOR3( Kaeiou, b0, b1 ); \
    Maeiou = XOR3( Maeiou, b0, b1 ); \
    Saeiou = XOR3( Saeiou, b0, b1 ); \
    /* Rho */ \
    Baeiou = _mm512_rolv_epi64(Baeiou, rhoB); \
    Gaeiou = _mm512_rolv_epi64(Gaeiou, rhoG); \
    Kaeiou = _mm512_rolv_epi64(Kaeiou, rhoK); \
    Maeiou = _mm512_rolv_epi64(Maeiou, rhoM); \
    Saeiou = _mm512_rolv_epi64(Saeiou, rhoS); \
    /* Pi 1 */ \
    b0 = _mm512_permutexvar_epi64(pi1B, Baeiou); \
    b1 = _mm512_permutexvar_epi64(pi1G, Gaeiou); \
    b2 = _mm512_permutexvar_epi64(pi1K, Kaeiou); \
    b3 = _mm512_permutexvar_epi64(pi1M, Maeiou); \
    b4 = _mm512_permutexvar_epi64(pi1S, Saeiou); \
    /* Chi */ \
    Baeiou = Chi(b0, b1, b2); \
    Gaeiou = Chi(b1, b2, b3); \
    Kaeiou = Chi(b2, b3, b4); \
    Maeiou = Chi(b3, b4, b0); \
    Saeiou = Chi(b4, b0, b1); \
    /* Iota */ \
    Baeiou = XOR(Baeiou, LOAD_Lane(K12xkcp::KeccakP1600RoundConstants+i)); \
    /* Pi 2 */ \
    b0 = _mm512_unpacklo_epi64(Baeiou, Gaeiou); \
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou); \
    b0 = _mm512_permutex2var_epi64(b0, pi2S1, Saeiou); \
    b2 = _mm512_unpackhi_epi64(Baeiou, Gaeiou); \
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou); \
    b2 = _mm512_permutex2var_epi64(b2, pi2S2, Saeiou); \
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1); \
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3); \
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1); \
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3); \
    b0 = _mm512_permutex2var_epi64(b0, pi2S3, b1); \
    Saeiou = _mm512_mask_blend_epi64(0x10, b0, Saeiou)

    /* ---------------------------------------------------------------- */

#define XKCPAVX512rounds12 \
    KeccakP_Round( 12 ); \
    KeccakP_Round( 13 ); \
    KeccakP_Round( 14 ); \
    KeccakP_Round( 15 ); \
    KeccakP_Round( 16 ); \
    KeccakP_Round( 17 ); \
    KeccakP_Round( 18 ); \
    KeccakP_Round( 19 ); \
    KeccakP_Round( 20 ); \
    KeccakP_Round( 21 ); \
    KeccakP_Round( 22 ); \
    KeccakP_Round( 23 )

    /* ---------------------------------------------------------------- */

namespace K12xkcp
{
    const unsigned long long KeccakP1600RoundConstants[24] = {
        0x0000000000000001ULL,
        0x0000000000008082ULL,
        0x800000000000808aULL,
        0x8000000080008000ULL,
        0x000000000000808bULL,
        0x0000000080000001ULL,
        0x8000000080008081ULL,
        0x8000000000008009ULL,
        0x000000000000008aULL,
        0x0000000000000088ULL,
        0x0000000080008009ULL,
        0x000000008000000aULL,
        0x000000008000808bULL,
        0x800000000000008bULL,
        0x8000000000008089ULL,
        0x8000000000008003ULL,
        0x8000000000008002ULL,
        0x8000000000000080ULL,
        0x000000000000800aULL,
        0x800000008000000aULL,
        0x8000000080008081ULL,
        0x8000000000008080ULL,
        0x0000000080000001ULL,
        0x8000000080008008ULL };

    /* ---------------------------------------------------------------- */

    void KeccakP1600_Initialize(void* state)
    {
        setMem(state, 200, 0);
    }

    void KeccakP1600_AddBytes(void* state, const unsigned char* data, unsigned int offset, unsigned int length)
    {
        unsigned char* stateAsBytes;
        unsigned long long* stateAsLanes;

        for (stateAsBytes = (unsigned char*)state; ((offset % 8) != 0) && (length != 0); ++offset, --length)
            stateAsBytes[offset] ^= *(data++);
        for (stateAsLanes = (unsigned long long*)(stateAsBytes + offset); length >= 8 * 8; stateAsLanes += 8, data += 8 * 8, length -= 8 * 8)
            STORE_8Lanes(stateAsLanes, XOR(LOAD_8Lanes(stateAsLanes), LOAD_8Lanes((const unsigned long long*)data)));
        for (/* empty */; length >= 8; ++stateAsLanes, data += 8, length -= 8)
            STORE_Lane(stateAsLanes, XOR(LOAD_Lane(stateAsLanes), LOAD_Lane((const unsigned long long*)data)));
        for (stateAsBytes = (unsigned char*)stateAsLanes; length != 0; --length)
            *(stateAsBytes++) ^= *(data++);
    }

    void KeccakP1600_ExtractBytes(const void* state, unsigned char* data, unsigned int offset, unsigned int length)
    {
        copyMem(data, (unsigned char*)state + offset, length);
    }

    void KeccakP1600_AddByte(void* state, unsigned char data, unsigned int offset)
    {
        ((unsigned char*)(state))[offset] ^= data;
    }


    void KeccakP1600_Permute_12rounds(void* state)
    {
        KeccakP_DeclareVars
        unsigned long long* stateAsLanes = (unsigned long long*)state;

        XKCPAVX512copyFromState(stateAsLanes);
        XKCPAVX512rounds12;
        XKCPAVX512copyToState(stateAsLanes);
    }

    size_t KeccakP1600_12rounds_FastLoop_Absorb(void* state, unsigned int laneCount, const unsigned char* data, size_t dataByteLen)
    {
        size_t originalDataByteLen = dataByteLen;

        KeccakP_DeclareVars
        unsigned long long* stateAsLanes = (unsigned long long*)state;
        unsigned long long* inDataAsLanes = (unsigned long long*)data;

        if (laneCount == 21)
        {
#define laneCount 21
            XKCPAVX512copyToState(stateAsLanes);
            while (dataByteLen >= 21 * 8)
            {
                Baeiou = XOR(Baeiou, LOAD_Plane(inDataAsLanes + 0));
                Gaeiou = XOR(Gaeiou, LOAD_Plane(inDataAsLanes + 5));
                Kaeiou = XOR(Kaeiou, LOAD_Plane(inDataAsLanes + 10));
                Maeiou = XOR(Maeiou, LOAD_Plane(inDataAsLanes + 15));
                Saeiou = XOR(Saeiou, LOAD_Lane(inDataAsLanes + 20));
                XKCPAVX512rounds12;
                inDataAsLanes += 21;
                dataByteLen -= 21 * 8;
            }
#undef laneCount
            XKCPAVX512copyToState(stateAsLanes);
        }
        else if (laneCount == 17)
        {
            // TODO: further optimization needed for this case, laneCount == 17.
            while (dataByteLen >= laneCount * 8)
            {
                KeccakP1600_AddBytes(state, data, 0, laneCount * 8);
                KeccakP1600_Permute_12rounds(state);
                data += laneCount * 8;
                dataByteLen -= laneCount * 8;
            }
        }

        return originalDataByteLen - dataByteLen;
    }
} // namespace K12xkcp

#elif defined(__AVX2__)
// XKCP AVX2 Implementation uses ASM Code in GCC syntax hence it is not applicable with MSVC at the moment hence we have to use the opt64 version

#include "brg_endian.h"

#define KeccakP1600_opt64_implementation_config "all rounds unrolled"
#define KeccakP1600_opt64_fullUnrolling

    /* ---------------------------------------------------------------- */

#define ROL64(a, offset) _rotl64(a, offset)

    /* ---------------------------------------------------------------- */

#define declareABCDE \
    unsigned long long Aba, Abe, Abi, Abo, Abu; \
    unsigned long long Aga, Age, Agi, Ago, Agu; \
    unsigned long long Aka, Ake, Aki, Ako, Aku; \
    unsigned long long Ama, Ame, Ami, Amo, Amu; \
    unsigned long long Asa, Ase, Asi, Aso, Asu; \
    unsigned long long Bba, Bbe, Bbi, Bbo, Bbu; \
    unsigned long long Bga, Bge, Bgi, Bgo, Bgu; \
    unsigned long long Bka, Bke, Bki, Bko, Bku; \
    unsigned long long Bma, Bme, Bmi, Bmo, Bmu; \
    unsigned long long Bsa, Bse, Bsi, Bso, Bsu; \
    unsigned long long Ca, Ce, Ci, Co, Cu; \
    unsigned long long Da, De, Di, Do, Du; \
    unsigned long long Eba, Ebe, Ebi, Ebo, Ebu; \
    unsigned long long Ega, Ege, Egi, Ego, Egu; \
    unsigned long long Eka, Eke, Eki, Eko, Eku; \
    unsigned long long Ema, Eme, Emi, Emo, Emu; \
    unsigned long long Esa, Ese, Esi, Eso, Esu; \

    /* ---------------------------------------------------------------- */

#define prepareTheta \
    Ca = Aba^Aga^Aka^Ama^Asa; \
    Ce = Abe^Age^Ake^Ame^Ase; \
    Ci = Abi^Agi^Aki^Ami^Asi; \
    Co = Abo^Ago^Ako^Amo^Aso; \
    Cu = Abu^Agu^Aku^Amu^Asu; \

    /* ---------------------------------------------------------------- */

#define XKCPthetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= K12xkcp::KeccakF1600RoundConstants[i];  \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
    Cu = E##bu; \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    Ci ^= E##gi; \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
    Cu ^= E##gu; \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^((~Bka)&  Bke ); \
    Cu ^= E##ku; \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    Ci ^= E##mi; \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
    Cu ^= E##mu; \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
    Cu ^= E##su; \
\

/* --- Code for round */
/* --- 64-bit lanes mapped to 64-bit words */
#define XKCPthetaRhoPiChiIota(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= K12xkcp::KeccakF1600RoundConstants[i]; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    E##ku =   Bku ^((~Bka)&  Bke ); \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
\

    /* ---------------------------------------------------------------- */

#define XKCPcopyFromState(X, state) \
    X##ba = state[ 0]; \
    X##be = state[ 1]; \
    X##bi = state[ 2]; \
    X##bo = state[ 3]; \
    X##bu = state[ 4]; \
    X##ga = state[ 5]; \
    X##ge = state[ 6]; \
    X##gi = state[ 7]; \
    X##go = state[ 8]; \
    X##gu = state[ 9]; \
    X##ka = state[10]; \
    X##ke = state[11]; \
    X##ki = state[12]; \
    X##ko = state[13]; \
    X##ku = state[14]; \
    X##ma = state[15]; \
    X##me = state[16]; \
    X##mi = state[17]; \
    X##mo = state[18]; \
    X##mu = state[19]; \
    X##sa = state[20]; \
    X##se = state[21]; \
    X##si = state[22]; \
    X##so = state[23]; \
    X##su = state[24]; \

    /* ---------------------------------------------------------------- */

#define XKCPcopyToState(state, X) \
    state[ 0] = X##ba; \
    state[ 1] = X##be; \
    state[ 2] = X##bi; \
    state[ 3] = X##bo; \
    state[ 4] = X##bu; \
    state[ 5] = X##ga; \
    state[ 6] = X##ge; \
    state[ 7] = X##gi; \
    state[ 8] = X##go; \
    state[ 9] = X##gu; \
    state[10] = X##ka; \
    state[11] = X##ke; \
    state[12] = X##ki; \
    state[13] = X##ko; \
    state[14] = X##ku; \
    state[15] = X##ma; \
    state[16] = X##me; \
    state[17] = X##mi; \
    state[18] = X##mo; \
    state[19] = X##mu; \
    state[20] = X##sa; \
    state[21] = X##se; \
    state[22] = X##si; \
    state[23] = X##so; \
    state[24] = X##su; \

    /* ---------------------------------------------------------------- */

#define copyStateVariables(X, Y) \
    X##ba = Y##ba; \
    X##be = Y##be; \
    X##bi = Y##bi; \
    X##bo = Y##bo; \
    X##bu = Y##bu; \
    X##ga = Y##ga; \
    X##ge = Y##ge; \
    X##gi = Y##gi; \
    X##go = Y##go; \
    X##gu = Y##gu; \
    X##ka = Y##ka; \
    X##ke = Y##ke; \
    X##ki = Y##ki; \
    X##ko = Y##ko; \
    X##ku = Y##ku; \
    X##ma = Y##ma; \
    X##me = Y##me; \
    X##mi = Y##mi; \
    X##mo = Y##mo; \
    X##mu = Y##mu; \
    X##sa = Y##sa; \
    X##se = Y##se; \
    X##si = Y##si; \
    X##so = Y##so; \
    X##su = Y##su; \

    /* ---------------------------------------------------------------- */

#define XKCProunds12 \
    prepareTheta \
    XKCPthetaRhoPiChiIotaPrepareTheta(12, A, E) \
    XKCPthetaRhoPiChiIotaPrepareTheta(13, E, A) \
    XKCPthetaRhoPiChiIotaPrepareTheta(14, A, E) \
    XKCPthetaRhoPiChiIotaPrepareTheta(15, E, A) \
    XKCPthetaRhoPiChiIotaPrepareTheta(16, A, E) \
    XKCPthetaRhoPiChiIotaPrepareTheta(17, E, A) \
    XKCPthetaRhoPiChiIotaPrepareTheta(18, A, E) \
    XKCPthetaRhoPiChiIotaPrepareTheta(19, E, A) \
    XKCPthetaRhoPiChiIotaPrepareTheta(20, A, E) \
    XKCPthetaRhoPiChiIotaPrepareTheta(21, E, A) \
    XKCPthetaRhoPiChiIotaPrepareTheta(22, A, E) \
    XKCPthetaRhoPiChiIota(23, E, A) \

    /* ---------------------------------------------------------------- */

namespace K12xkcp
{
    static const unsigned long long KeccakF1600RoundConstants[24] = {
        0x0000000000000001ULL,
        0x0000000000008082ULL,
        0x800000000000808aULL,
        0x8000000080008000ULL,
        0x000000000000808bULL,
        0x0000000080000001ULL,
        0x8000000080008081ULL,
        0x8000000000008009ULL,
        0x000000000000008aULL,
        0x0000000000000088ULL,
        0x0000000080008009ULL,
        0x000000008000000aULL,
        0x000000008000808bULL,
        0x800000000000008bULL,
        0x8000000000008089ULL,
        0x8000000000008003ULL,
        0x8000000000008002ULL,
        0x8000000000000080ULL,
        0x000000000000800aULL,
        0x800000008000000aULL,
        0x8000000080008081ULL,
        0x8000000000008080ULL,
        0x0000000080000001ULL,
        0x8000000080008008ULL};

    /* ---------------------------------------------------------------- */

    #define SnP_AddBytes(state, data, offset, length, SnP_AddLanes, SnP_AddBytesInLane, SnP_laneLengthInBytes) \
        { \
            if ((offset) == 0) { \
                SnP_AddLanes(state, data, (length)/SnP_laneLengthInBytes); \
                SnP_AddBytesInLane(state, \
                    (length)/SnP_laneLengthInBytes, \
                    (data)+((length)/SnP_laneLengthInBytes)*SnP_laneLengthInBytes, \
                    0, \
                    (length)%SnP_laneLengthInBytes); \
            } \
            else { \
                unsigned int _sizeLeft = (length); \
                unsigned int _lanePosition = (offset)/SnP_laneLengthInBytes; \
                unsigned int _offsetInLane = (offset)%SnP_laneLengthInBytes; \
                const unsigned char *_curData = (data); \
                while(_sizeLeft > 0) { \
                    unsigned int _bytesInLane = SnP_laneLengthInBytes - _offsetInLane; \
                    if (_bytesInLane > _sizeLeft) \
                        _bytesInLane = _sizeLeft; \
                    SnP_AddBytesInLane(state, _lanePosition, _curData, _offsetInLane, _bytesInLane); \
                    _sizeLeft -= _bytesInLane; \
                    _lanePosition++; \
                    _offsetInLane = 0; \
                    _curData += _bytesInLane; \
                } \
            } \
        }

    /* ---------------------------------------------------------------- */

    #define SnP_ExtractBytes(state, data, offset, length, SnP_ExtractLanes, SnP_ExtractBytesInLane, SnP_laneLengthInBytes) \
        {                                                                                                                  \
            if ((offset) == 0)                                                                                             \
            {                                                                                                              \
                SnP_ExtractLanes(state, data, (length) / SnP_laneLengthInBytes);                                           \
                SnP_ExtractBytesInLane(state,                                                                              \
                                    (length) / SnP_laneLengthInBytes,                                                      \
                                    (data) + ((length) / SnP_laneLengthInBytes) * SnP_laneLengthInBytes,                   \
                                    0,                                                                                     \
                                    (length) % SnP_laneLengthInBytes);                                                     \
            }                                                                                                              \
            else                                                                                                           \
            {                                                                                                              \
                unsigned int _sizeLeft = (length);                                                                         \
                unsigned int _lanePosition = (offset) / SnP_laneLengthInBytes;                                             \
                unsigned int _offsetInLane = (offset) % SnP_laneLengthInBytes;                                             \
                unsigned char* _curData = (data);                                                                          \
                while (_sizeLeft > 0)                                                                                      \
                {                                                                                                          \
                    unsigned int _bytesInLane = SnP_laneLengthInBytes - _offsetInLane;                                     \
                    if (_bytesInLane > _sizeLeft)                                                                          \
                        _bytesInLane = _sizeLeft;                                                                          \
                    SnP_ExtractBytesInLane(state, _lanePosition, _curData, _offsetInLane, _bytesInLane);                   \
                    _sizeLeft -= _bytesInLane;                                                                             \
                    _lanePosition++;                                                                                       \
                    _offsetInLane = 0;                                                                                     \
                    _curData += _bytesInLane;                                                                              \
                }                                                                                                          \
            }                                                                                                              \
        }

    /* ---------------------------------------------------------------- */

    #if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    #define HTOLE64(x) (x)
    #else
    #define HTOLE64(x) (\
    ((x & 0xff00000000000000ull) >> 56) | \
    ((x & 0x00ff000000000000ull) >> 40) | \
    ((x & 0x0000ff0000000000ull) >> 24) | \
    ((x & 0x000000ff00000000ull) >> 8)  | \
    ((x & 0x00000000ff000000ull) << 8)  | \
    ((x & 0x0000000000ff0000ull) << 24) | \
    ((x & 0x000000000000ff00ull) << 40) | \
    ((x & 0x00000000000000ffull) << 56))
    #endif

    /* ---------------------------------------------------------------- */

    #define addInput21(X, input, laneCount) \
        if (laneCount == 21) { \
            X##ba ^= HTOLE64(input[ 0]); \
            X##be ^= HTOLE64(input[ 1]); \
            X##bi ^= HTOLE64(input[ 2]); \
            X##bo ^= HTOLE64(input[ 3]); \
            X##bu ^= HTOLE64(input[ 4]); \
            X##ga ^= HTOLE64(input[ 5]); \
            X##ge ^= HTOLE64(input[ 6]); \
            X##gi ^= HTOLE64(input[ 7]); \
            X##go ^= HTOLE64(input[ 8]); \
            X##gu ^= HTOLE64(input[ 9]); \
            X##ka ^= HTOLE64(input[10]); \
            X##ke ^= HTOLE64(input[11]); \
            X##ki ^= HTOLE64(input[12]); \
            X##ko ^= HTOLE64(input[13]); \
            X##ku ^= HTOLE64(input[14]); \
            X##ma ^= HTOLE64(input[15]); \
            X##me ^= HTOLE64(input[16]); \
            X##mi ^= HTOLE64(input[17]); \
            X##mo ^= HTOLE64(input[18]); \
            X##mu ^= HTOLE64(input[19]); \
            X##sa ^= HTOLE64(input[20]); \
        } \

    /* ---------------------------------------------------------------- */

    #define addInput17(X, input, laneCount) \
        if (laneCount == 17) { \
            X##ba ^= HTOLE64(input[ 0]); \
            X##be ^= HTOLE64(input[ 1]); \
            X##bi ^= HTOLE64(input[ 2]); \
            X##bo ^= HTOLE64(input[ 3]); \
            X##bu ^= HTOLE64(input[ 4]); \
            X##ga ^= HTOLE64(input[ 5]); \
            X##ge ^= HTOLE64(input[ 6]); \
            X##gi ^= HTOLE64(input[ 7]); \
            X##go ^= HTOLE64(input[ 8]); \
            X##gu ^= HTOLE64(input[ 9]); \
            X##ka ^= HTOLE64(input[10]); \
            X##ke ^= HTOLE64(input[11]); \
            X##ki ^= HTOLE64(input[12]); \
            X##ko ^= HTOLE64(input[13]); \
            X##ku ^= HTOLE64(input[14]); \
            X##ma ^= HTOLE64(input[15]); \
            X##me ^= HTOLE64(input[16]); \
        } \

    /* ---------------------------------------------------------------- */

    void KeccakP1600_Initialize(void* state)
    {
        setMem(state, 200, 0);
    }

    void KeccakP1600_AddBytesInLane(void* state, unsigned int lanePosition, const unsigned char* data, unsigned int offset, unsigned int length)
    {
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
        uint64_t lane;
        if (length == 0)
            return;
        if (length == 1)
            lane = data[0];
        else
        {
            lane = 0;
            copyMem(&lane, data, length);
        }
        lane <<= offset * 8;
#else
        uint64_t lane = 0;
        unsigned int i;
        for (i = 0; i < length; i++)
            lane |= ((uint64_t)data[i]) << ((i + offset) * 8);
#endif
        ((uint64_t*)state)[lanePosition] ^= lane;
    }

    static void KeccakP1600_AddLanes(void* state, const unsigned char* data, unsigned int laneCount)
    {
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
        unsigned int i = 0;
#ifdef NO_MISALIGNED_ACCESSES
    /* If either pointer is misaligned, fall back to byte-wise xor. */
        if (((((uintptr_t)state) & 7) != 0) || ((((uintptr_t)data) & 7) != 0))
        {
            for (i = 0; i < laneCount * 8; i++)
            {
                ((unsigned char*)state)[i] ^= data[i];
            }
        }
        else
#endif
        {
            /* Otherwise... */
            for (; (i + 8) <= laneCount; i += 8)
            {
                ((uint64_t*)state)[i + 0] ^= ((uint64_t*)data)[i + 0];
                ((uint64_t*)state)[i + 1] ^= ((uint64_t*)data)[i + 1];
                ((uint64_t*)state)[i + 2] ^= ((uint64_t*)data)[i + 2];
                ((uint64_t*)state)[i + 3] ^= ((uint64_t*)data)[i + 3];
                ((uint64_t*)state)[i + 4] ^= ((uint64_t*)data)[i + 4];
                ((uint64_t*)state)[i + 5] ^= ((uint64_t*)data)[i + 5];
                ((uint64_t*)state)[i + 6] ^= ((uint64_t*)data)[i + 6];
                ((uint64_t*)state)[i + 7] ^= ((uint64_t*)data)[i + 7];
            }
            for (; (i + 4) <= laneCount; i += 4)
            {
                ((uint64_t*)state)[i + 0] ^= ((uint64_t*)data)[i + 0];
                ((uint64_t*)state)[i + 1] ^= ((uint64_t*)data)[i + 1];
                ((uint64_t*)state)[i + 2] ^= ((uint64_t*)data)[i + 2];
                ((uint64_t*)state)[i + 3] ^= ((uint64_t*)data)[i + 3];
            }
            for (; (i + 2) <= laneCount; i += 2)
            {
                ((uint64_t*)state)[i + 0] ^= ((uint64_t*)data)[i + 0];
                ((uint64_t*)state)[i + 1] ^= ((uint64_t*)data)[i + 1];
            }
            if (i < laneCount)
            {
                ((uint64_t*)state)[i + 0] ^= ((uint64_t*)data)[i + 0];
            }
        }
#else
        unsigned int i;
        const uint8_t *curData = data;
        for(i=0; i<laneCount; i++, curData+=8)
        {
            uint64_t lane = (uint64_t)curData[0]
                | ((uint64_t)curData[1] <<  8)
                | ((uint64_t)curData[2] << 16)
                | ((uint64_t)curData[3] << 24)
                | ((uint64_t)curData[4] << 32)
                | ((uint64_t)curData[5] << 40)
                | ((uint64_t)curData[6] << 48)
                | ((uint64_t)curData[7] << 56);
            ((uint64_t*)state)[i] ^= lane;
        }
#endif
    }


    void KeccakP1600_AddByte(void *state, unsigned char byte, unsigned int offset)
    {
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
        ((unsigned char*)(state))[offset] ^= byte;
#else
        uint64_t lane = byte;
        lane <<= (offset%8)*8;
        ((uint64_t*)state)[offset/8] ^= lane;
#endif
    }


    void KeccakP1600_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
    {
        SnP_AddBytes(state, data, offset, length, KeccakP1600_AddLanes, KeccakP1600_AddBytesInLane, 8);
    }


    void KeccakP1600_Permute_12rounds(void *state)
    {
        declareABCDE
        /* #ifndef KeccakP1600_fullUnrolling */
        /* unsigned int i; */
        /* #endif */
        uint64_t *stateAsLanes = (uint64_t*)state;

        XKCPcopyFromState(A, stateAsLanes)
        XKCProunds12
        XKCPcopyToState(stateAsLanes, A)
    }


    void KeccakP1600_ExtractBytesInLane(const void *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length)
    {
        uint64_t lane = ((uint64_t*)state)[lanePosition];
#ifdef KeccakP1600_useLaneComplementing
        if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
            lane = ~lane;
#endif
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
        {
            uint64_t lane1[1];
            lane1[0] = lane;
            copyMem(data, (uint8_t*)lane1 + offset, length);
        }
#else
        unsigned int i;
        lane >>= offset * 8;
        for (i = 0; i < length; i++)
        {
            data[i] = lane & 0xFF;
            lane >>= 8;
        }
#endif
    }

#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
    static void fromWordToBytes(uint8_t *bytes, const uint64_t word)
    {
        unsigned int i;

        for (i = 0; i < (64 / 8); i++)
            bytes[i] = (word >> (8 * i)) & 0xFF;
    }
#endif

    void KeccakP1600_ExtractLanes(const void *state, unsigned char *data, unsigned int laneCount)
    {
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
        copyMem(data, state, laneCount*8);
#else
        unsigned int i;

        for(i=0; i<laneCount; i++)
            fromWordToBytes(data+(i*8), ((const uint64_t*)state)[i]);
#endif
#ifdef KeccakP1600_useLaneComplementing
        if (laneCount > 1)
        {
            ((uint64_t*)data)[1] = ~((uint64_t*)data)[1];
            if (laneCount > 2)
            {
                ((uint64_t*)data)[2] = ~((uint64_t*)data)[2];
                if (laneCount > 8)
                {
                    ((uint64_t*)data)[8] = ~((uint64_t*)data)[8];
                    if (laneCount > 12)
                    {
                        ((uint64_t*)data)[12] = ~((uint64_t*)data)[12];
                        if (laneCount > 17)
                        {
                            ((uint64_t*)data)[17] = ~((uint64_t*)data)[17];
                            if (laneCount > 20)
                            {
                                ((uint64_t*)data)[20] = ~((uint64_t*)data)[20];
                            }
                        }
                    }
                }
            }
        }
#endif
    }

    void KeccakP1600_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length)
    {
        SnP_ExtractBytes(state, data, offset, length, KeccakP1600_ExtractLanes, KeccakP1600_ExtractBytesInLane, 8);
    }

    /* #include <assert.h>  */

    size_t KeccakP1600_12rounds_FastLoop_Absorb(void* state, unsigned int laneCount, const unsigned char* data, size_t dataByteLen)
    {
        size_t originalDataByteLen = dataByteLen;

        declareABCDE
            /* #ifndef KeccakP1600_fullUnrolling */
            /* unsigned int i; */
            /* #endif */
            uint64_t* stateAsLanes = (uint64_t*)state;
        uint64_t* inDataAsLanes = (uint64_t*)data;

        /* ASSERT(laneCount == 21 || laneCount == 17); */

        if (laneCount == 21)
        {
            XKCPcopyFromState(A, stateAsLanes) while (dataByteLen >= laneCount * 8)
            {
                addInput21(A, inDataAsLanes, laneCount)
                    XKCProunds12
                        inDataAsLanes += laneCount;
                dataByteLen -= laneCount * 8;
            }
            XKCPcopyToState(stateAsLanes, A)
        }
        else if (laneCount == 17)
        {
            XKCPcopyFromState(A, stateAsLanes) while (dataByteLen >= laneCount * 8)
            {
                addInput17(A, inDataAsLanes, laneCount)
                    XKCProunds12
                        inDataAsLanes += laneCount;
                dataByteLen -= laneCount * 8;
            }
            XKCPcopyToState(stateAsLanes, A)
        }

        return originalDataByteLen - dataByteLen;
    }
} // namespace K12xkcp
#endif
} // namespace XKPC
