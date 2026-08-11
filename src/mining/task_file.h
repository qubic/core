#pragma once

#include "platform/memory.h"

// Unified binary task file format: [ header 96B ][ topology block ][ data block ].
// Fixed-width fields use plain types (unsigned int = 32-bit, unsigned long long = 64-bit).
//
// Topology block (all unsigned int, native little-endian):
//   inputNeuronIndices[N], outputNeuronIndices[M], signalNeuronIndex, neighborIndices[P*K].
// Wiring is explicit per neuron: neuron n's k-th neighbour is neighborIndices[n*K + k], any neuron in
// [0, P). Each neuron may choose its own arbitrary set of K neighbours.
// Data block: numPairs rows, each row = ceil(N/5) packed input bytes + ceil(M/5) packed output bytes.
// Trits are packed five per byte: t0 + 3*t1 + 9*t2 + 27*t3 + 81*t4, so a valid byte is 0..242.
namespace score_task_file
{

// Magic/version for the Generic LUT task; callers pass these to write/load.
static constexpr unsigned int MAGIC = 0x5454554CU;
static constexpr unsigned int VERSION = 1;

// Store each value as a trit (3 states {0,1,2}) so the format can carry a third state in future tasks.
// TRIT_BASE is how many states one value can take; TRITS_PER_BYTE is how many trits pack into a byte
// (5 because 3^5 = 243 <= 256 is the tightest fit).
static constexpr unsigned int TRIT_BASE = 3;
static constexpr unsigned int TRITS_PER_BYTE = 5;

// exp-th power of base, evaluated at compile time.
constexpr unsigned long long ipow(unsigned long long base, unsigned int exp)
{
    return exp == 0 ? 1 : base * ipow(base, exp - 1);
}

// A packed byte holds values 0 .. BYTE_VALUE_LIMIT - 1 (243 = 3^5); anything at or above is invalid.
static constexpr unsigned int BYTE_VALUE_LIMIT = (unsigned int)ipow(TRIT_BASE, TRITS_PER_BYTE);

// Byte length of each hash carried in the header.
static constexpr unsigned int DATA_HASH_SIZE = 32;

#pragma pack(push, 1)
struct TaskFileHeader
{
    unsigned int magic;
    unsigned int version;
    unsigned int numInputTrits;      // N, input trits per sample (= input neurons)
    unsigned int numOutputTrits;     // M, output trits per sample (= output neurons)
    unsigned long long numPairs;     // T, number of samples
    unsigned int population;         // P, ANN neuron count
    unsigned int numNeighbors;       // K, LUT fan-in (neighbours per neuron)
    unsigned char topologyHash[DATA_HASH_SIZE];  // hash of the topology block
    unsigned char dataHash[DATA_HASH_SIZE];      // hash of the (packed) data block
};
#pragma pack(pop)

static_assert(sizeof(TaskFileHeader) == 96, "TaskFileHeader must be exactly 96 bytes");

// ceil(tritCount / TRITS_PER_BYTE): bytes needed to hold that many trits at five trits per byte.
inline unsigned long long packedBytes(unsigned long long tritCount)
{
    return (tritCount + (TRITS_PER_BYTE - 1)) / TRITS_PER_BYTE;
}

// Byte length of the topology block: inputNeuronIndices[N], outputNeuronIndices[M], one signal index,
// and neighborIndices[population*K] - all unsigned int.
inline unsigned long long topologyBytes(unsigned int numInputTrits, unsigned int numOutputTrits, unsigned int population, unsigned int numNeighbors)
{
    return ((unsigned long long)numInputTrits + numOutputTrits + 1 + (unsigned long long)population * numNeighbors) * sizeof(unsigned int);
}

// Byte length of the packed data block.
inline unsigned long long dataBytes(unsigned int numInputTrits, unsigned int numOutputTrits, unsigned long long numPairs)
{
    return numPairs * (packedBytes(numInputTrits) + packedBytes(numOutputTrits));
}

// Pack count trits {0,1,2} into out[], five per byte. Unused positions in the last byte are zero.
inline void packTrits(const unsigned char* trits, unsigned long long count, unsigned char* out)
{
    unsigned long long byteIndex = 0;
    for (unsigned long long i = 0; i < count; i += TRITS_PER_BYTE)
    {
        unsigned int packed = 0;
        unsigned int weight = 1;
        for (unsigned int k = 0; k < TRITS_PER_BYTE; ++k)
        {
            const unsigned char t = (i + k < count) ? trits[i + k] : 0;
            packed += (unsigned int)t * weight;
            weight *= TRIT_BASE;
        }
        out[byteIndex] = (unsigned char)packed;
        byteIndex++;
    }
}

// Unpack count trits from bytes[] (five per byte). Returns false if any byte is not a valid base-243
// group (>= BYTE_VALUE_LIMIT). Only the first count trits are written; padding is skipped.
inline bool unpackTrits(const unsigned char* bytes, unsigned long long count, unsigned char* trits)
{
    unsigned long long byteIndex = 0;
    for (unsigned long long i = 0; i < count; i += TRITS_PER_BYTE)
    {
        unsigned int packed = bytes[byteIndex];
        if (packed >= BYTE_VALUE_LIMIT)
        {
            return false;
        }
        for (unsigned int k = 0; k < TRITS_PER_BYTE; ++k)
        {
            if (i + k < count)
            {
                trits[i + k] = (unsigned char)(packed % TRIT_BASE);
            }
            packed /= TRIT_BASE;
        }
        byteIndex++;
    }
    return true;
}

// Serialise the topology into outBlock (topologyBytes long): input, output, signal, then the explicit
// per-neuron neighbour indices (population * K).
inline void serializeTopologyBlock(unsigned int numInputTrits, unsigned int numOutputTrits, unsigned int population, unsigned int numNeighbors,
                                   const unsigned int* inputNeuronIndices, const unsigned int* outputNeuronIndices,
                                   unsigned int signalNeuronIndex, const unsigned int* neighborIndices,
                                   unsigned char* outBlock)
{
    unsigned char* p = outBlock;
    copyMem(p, (const unsigned char*)inputNeuronIndices, (unsigned long long)numInputTrits * sizeof(unsigned int));
    p += (unsigned long long)numInputTrits * sizeof(unsigned int);
    copyMem(p, (const unsigned char*)outputNeuronIndices, (unsigned long long)numOutputTrits * sizeof(unsigned int));
    p += (unsigned long long)numOutputTrits * sizeof(unsigned int);
    copyMem(p, (const unsigned char*)&signalNeuronIndex, sizeof(unsigned int));
    p += sizeof(unsigned int);
    copyMem(p, (const unsigned char*)neighborIndices, (unsigned long long)population * numNeighbors * sizeof(unsigned int));
}

// Parse a topology block (as read from the file) into the caller's arrays. Inverse of serialize.
inline void parseTopologyBlock(const unsigned char* block, unsigned int numInputTrits, unsigned int numOutputTrits, unsigned int population, unsigned int numNeighbors,
                               unsigned int* outInputNeuronIndices, unsigned int* outOutputNeuronIndices,
                               unsigned int* outSignalNeuronIndex, unsigned int* outNeighborIndices)
{
    const unsigned char* p = block;
    copyMem((unsigned char*)outInputNeuronIndices, p, (unsigned long long)numInputTrits * sizeof(unsigned int));
    p += (unsigned long long)numInputTrits * sizeof(unsigned int);
    copyMem((unsigned char*)outOutputNeuronIndices, p, (unsigned long long)numOutputTrits * sizeof(unsigned int));
    p += (unsigned long long)numOutputTrits * sizeof(unsigned int);
    copyMem((unsigned char*)outSignalNeuronIndex, p, sizeof(unsigned int));
    p += sizeof(unsigned int);
    copyMem((unsigned char*)outNeighborIndices, p, (unsigned long long)population * numNeighbors * sizeof(unsigned int));
}

// Pack numPairs input/output trit rows into outBlock (dataBytes long), row-major.
inline void packDataBlock(unsigned int numInputTrits, unsigned int numOutputTrits, unsigned long long numPairs,
                          const unsigned char* inputsTrits, const unsigned char* outputsTrits,
                          unsigned char* outBlock)
{
    const unsigned long long inBytes = packedBytes(numInputTrits);
    const unsigned long long outBytes = packedBytes(numOutputTrits);
    const unsigned long long rowBytes = inBytes + outBytes;
    for (unsigned long long p = 0; p < numPairs; ++p)
    {
        unsigned char* row = outBlock + p * rowBytes;
        packTrits(inputsTrits + p * numInputTrits, numInputTrits, row);
        packTrits(outputsTrits + p * numOutputTrits, numOutputTrits, row + inBytes);
    }
}

// Unpack a data block into the caller's input/output trit arrays. Returns false on a malformed byte.
inline bool unpackDataBlock(unsigned int numInputTrits, unsigned int numOutputTrits, unsigned long long numPairs,
                            const unsigned char* block, unsigned char* outInputs, unsigned char* outOutputs)
{
    const unsigned long long inBytes = packedBytes(numInputTrits);
    const unsigned long long outBytes = packedBytes(numOutputTrits);
    const unsigned long long rowBytes = inBytes + outBytes;
    for (unsigned long long p = 0; p < numPairs; ++p)
    {
        const unsigned char* row = block + p * rowBytes;
        if (!unpackTrits(row, numInputTrits, outInputs + p * numInputTrits))
        {
            return false;
        }
        if (!unpackTrits(row + inBytes, numOutputTrits, outOutputs + p * numOutputTrits))
        {
            return false;
        }
    }
    return true;
}

} // namespace score_task_file
