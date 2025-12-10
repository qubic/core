#pragma once

#define SIGNATURE_SIZE 64
#ifdef TESTNET
#define NUMBER_OF_TRANSACTIONS_PER_TICK 1024 // Must be 2^N
#else
#define NUMBER_OF_TRANSACTIONS_PER_TICK 1024 // Must be 2^N
#endif
#define MAX_NUMBER_OF_CONTRACTS 1024 // Must be 1024
#define NUMBER_OF_COMPUTORS 676
#define QUORUM (NUMBER_OF_COMPUTORS * 2 / 3 + 1)
#define NUMBER_OF_EXCHANGED_PEERS 4

#ifdef TESTNET
#define SPECTRUM_DEPTH 24 // Defines SPECTRUM_CAPACITY (1 << SPECTRUM_DEPTH)
#else
#define SPECTRUM_DEPTH 24 // Defines SPECTRUM_CAPACITY (1 << SPECTRUM_DEPTH)
#endif
#define SPECTRUM_CAPACITY (1ULL << SPECTRUM_DEPTH) // Must be 2^N

#ifdef TESTNET
#define ASSETS_DEPTH 24 // Is derived from ASSETS_CAPACITY (=N)
#else
#define ASSETS_DEPTH 24 // Is derived from ASSETS_CAPACITY (=N)
#endif
#define ASSETS_CAPACITY (1ULL << ASSETS_DEPTH) // Must be 2^N

#define MAX_INPUT_SIZE 1024ULL
#define ISSUANCE_RATE 1000000000000LL
#define MAX_AMOUNT (ISSUANCE_RATE * 1000LL)
#define MAX_SUPPLY (ISSUANCE_RATE * 200ULL)

#include "network_message_type.h"


// If you want to use the network_meassges directory in your project without dependencies to other code,
// you may define NETWORK_MESSAGES_WITHOUT_CORE_DEPENDENCIES before including any header or change the
// following line to "#if 1".
#if defined(NETWORK_MESSAGES_WITHOUT_CORE_DEPENDENCIES)

#include <lib/platform_common/qintrin.h>
#include <lib/platform_common/qstdint.h>

typedef union m256i
{
    int8_t              m256i_i8[32];
    int16_t             m256i_i16[16];
    int32_t             m256i_i32[8];
    int64_t             m256i_i64[4];
    uint8_t             m256i_u8[32];
    uint16_t            m256i_u16[16];
    uint32_t            m256i_u32[8];
    uint64_t            m256i_u64[4];
} m256i;

#else

#include "../platform/m256.h"

#endif

typedef union IPv4Address
{
    uint8_t     u8[4];
    uint32_t    u32;

    void fromString(std::string str) {
        size_t pos = 0;
        for (int i = 0; i < 4; i++) {
            size_t nextPos = str.find('.', pos);
            std::string byteStr = (nextPos == std::string::npos) ? str.substr(pos) : str.substr(pos, nextPos - pos);
            u8[i] = static_cast<uint8_t>(std::stoi(byteStr));
            if (nextPos == std::string::npos) break;
            pos = nextPos + 1;
        }
    }
} IPv4Address;

static_assert(sizeof(IPv4Address) == 4, "Unexpected size!");

static inline bool operator==(const IPv4Address& a, const IPv4Address& b)
{
    return a.u32 == b.u32;
}

static inline bool operator!=(const IPv4Address& a, const IPv4Address& b)
{
    return a.u32 != b.u32;
}

// Compute the siblings array of each level of tree. This function is not thread safe
// make sure resource protection is handled outside
template <unsigned int depth>
static void getSiblings(int digestIndex, const m256i* digests, m256i siblings[depth])
{
    const unsigned int capacity = (1ULL << depth);
    int siblingIndex = digestIndex;
    unsigned int digestOffset = 0;
    for (unsigned int j = 0; j < depth; j++)
    {
        siblings[j] = digests[digestOffset + (siblingIndex ^ 1)];
        digestOffset += (capacity >> j);
        siblingIndex >>= 1;
    }
}
