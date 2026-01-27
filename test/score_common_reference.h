#pragma once

#include "kangaroo_twelve.h"

#include <vector>

namespace score_reference
{

constexpr unsigned long long POOL_VEC_SIZE = (((1ULL << 32) + 64)) >> 3; // 2^32+64 bits ~ 512MB
constexpr unsigned long long POOL_VEC_PADDING_SIZE = (POOL_VEC_SIZE + 200 - 1) / 200 * 200; // padding for multiple of 200

void generateRandom2Pool(const unsigned char miningSeed[32], unsigned char* pool)
{
    unsigned char state[200];
    // same pool to be used by all computors/candidates and pool content changing each phase
    memcpy(&state[0], miningSeed, 32);
    memset(&state[32], 0, sizeof(state) - 32);

    for (unsigned int i = 0; i < POOL_VEC_PADDING_SIZE; i += sizeof(state))
    {
        KeccakP1600_Permute_12rounds(state);
        memcpy(&pool[i], state, sizeof(state));
    }
}

void random2(
    unsigned char seed[32],
    const unsigned char* pool,
    unsigned char* output,
    unsigned long long outputSizeInByte)
{
    unsigned long long paddingOutputSize = (outputSizeInByte + 64 - 1) / 64;
    paddingOutputSize = paddingOutputSize * 64;
    std::vector<unsigned char> paddingOutputVec(paddingOutputSize);
    unsigned char* paddingOutput = paddingOutputVec.data();

    unsigned long long segments = paddingOutputSize / 64;
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
                *((unsigned long long*) & paddingOutput[j * 8 * 8 + i * 8]) = u64_0;
            }
            else
            {
                *((unsigned long long*) & paddingOutput[j * 8 * 8 + i * 8]) = (u64_0 >> m) | (u64_1 << (64 - m));
            }

            // Increase the positions in the pool for each element.
            x[i] = x[i] * 1664525 + 1013904223; // https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
        }
    }

    memcpy(output, paddingOutput, outputSizeInByte);
}

// Clamp the neuron value
template <typename T>
char clampNeuron(T neuronValue)
{
    if (neuronValue > 1)
    {
        return 1;
    }

    if (neuronValue < -1)
    {
        return -1;
    }
    return static_cast<char>(neuronValue);
}

void extract64Bits(unsigned long long number, char* output)
{
    for (int i = 0; i < 64; ++i)
    {
        output[i] = ((number >> i) & 1);
    }
}

template <unsigned long long bitCount>
void toTenaryBits(long long A, char* bits)
{
    for (unsigned long long i = 0; i < bitCount; ++i)
    {
        char bitValue = static_cast<char>((A >> i) & 1);
        bits[i] = (bitValue == 0) ? -1 : bitValue;
    }
}
}
