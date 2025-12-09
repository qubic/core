#pragma once

#define SIGNATURE_SIZE 64
#define NUMBER_OF_TRANSACTIONS_PER_TICK 1024 // Must be 2^N
#define MAX_NUMBER_OF_CONTRACTS 1024 // Must be 1024
#define NUMBER_OF_COMPUTORS 676
#define QUORUM (NUMBER_OF_COMPUTORS * 2 / 3 + 1)
#define NUMBER_OF_EXCHANGED_PEERS 4

#define SPECTRUM_DEPTH 24 // Defines SPECTRUM_CAPACITY (1 << SPECTRUM_DEPTH)
#define SPECTRUM_CAPACITY (1ULL << SPECTRUM_DEPTH) // Must be 2^N

#define ASSETS_CAPACITY 0x1000000ULL // Must be 2^N
#define ASSETS_DEPTH 24 // Is derived from ASSETS_CAPACITY (=N)

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

constexpr uint8_t ORACLE_QUERY_TYPE_CONTRACT_QUERY = 0;
constexpr uint8_t ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION = 1;
constexpr uint8_t ORACLE_QUERY_TYPE_USER_QUERY = 2;

constexpr uint8_t ORACLE_QUERY_STATUS_UNKNOWN = 0;     ///< Query not found / valid.
constexpr uint8_t ORACLE_QUERY_STATUS_PENDING = 1;     ///< Query is being processed.
constexpr uint8_t ORACLE_QUERY_STATUS_COMMITTED = 2;   ///< The quorum has commited to a oracle reply, but it has not been revealed yet.
constexpr uint8_t ORACLE_QUERY_STATUS_SUCCESS = 3;     ///< The oracle reply has been confirmed and is available.
constexpr uint8_t ORACLE_QUERY_STATUS_DISAGREE = 5;    ///< No valid oracle reply is available, because computors disagreed about the value.
constexpr uint8_t ORACLE_QUERY_STATUS_TIMEOUT = 4;     ///< No valid oracle reply is available and timeout has hit.

// Fine-grained status flags returned by oracle machine nodes
constexpr uint16_t ORACLE_FLAG_REPLY_PENDING = 0x0;    ///< Oracle machine hasn't replied yet to the query. In OracleMachineReply::oracleMachineErrorFlags this means "no error".
constexpr uint16_t ORACLE_FLAG_INVALID_ORACLE = 0x1;   ///< Oracle machine reported that oracle (data source) in query was invalid.
constexpr uint16_t ORACLE_FLAG_ORACLE_UNAVAIL = 0x2;   ///< Oracle machine reported that oracle isn't available at the moment.
constexpr uint16_t ORACLE_FLAG_INVALID_TIME = 0x4;     ///< Oracle machine reported that time in query was invalid.
constexpr uint16_t ORACLE_FLAG_INVALID_PLACE = 0x8;    ///< Oracle machine reported that place in query was invalid.
constexpr uint16_t ORACLE_FLAG_INVALID_ARG = 0x10;     ///< Oracle machine reported that an argument in query was invalid.
constexpr uint16_t ORACLE_FLAG_OM_ERROR_FLAGS = 0xff;  ///< Mask of all error flags that may be returned by oracle machine response.
constexpr uint16_t ORACLE_FLAG_REPLY_RECEIVED = 0x100; ///< Oracle engine got valid reply from the oracle machine.
constexpr uint16_t ORACLE_FLAG_BAD_SIZE_REPLY = 0x200; ///< Oracle engine got reply of wrong size from the oracle machine.
constexpr uint16_t ORACLE_FLAG_OM_DISAGREE = 0x400;    ///< Oracle engine got different replies from oracle machines.
constexpr uint16_t ORACLE_FLAG_COMP_DISAGREE = 0x800;  ///< The number of reply commits is sufficient (>= 451 computors), but they disagree about the reply value.
constexpr uint16_t ORACLE_FLAG_TIMEOUT = 0x1000;       ///< The weren't enough reply commit tx before timeout (< 451).

typedef union IPv4Address
{
    uint8_t     u8[4];
    uint32_t    u32;
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
