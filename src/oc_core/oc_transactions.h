#pragma once

#include "network_messages/transactions.h"


// 13-byte ASCII domain separator that prefixes every signed OC authorization message.
// Prevents signature reuse across other Qubic message types.
constexpr char OC_AUTH_DOMAIN_SEPARATOR[13] = { 'Q', 'U', 'B', 'I', 'C', '_', 'O', 'C', '_', 'A', 'U', 'T', 'H' };
constexpr unsigned int OC_AUTH_DOMAIN_SEPARATOR_SIZE = 13;


// Canonical layout of bytes hashed into authMessage:
//
//     "QUBIC_OC_AUTH"               13 bytes, ASCII, no terminator
//     epoch                          uint16 LE,  2 bytes
//     interfaceIndex                 uint16 LE,  2 bytes
//     invocationId                   sint64 LE,  8 bytes
//     paramsDigest                   m256i,     32 bytes
//
// MUST be the SOLE serializer for the auth message. Both signing (per-computor
// emission) and verification (engine signature processing) consume this layout.
#pragma pack(push, 1)
struct OcAuthMessageBytes
{
    unsigned char domainSeparator[OC_AUTH_DOMAIN_SEPARATOR_SIZE];
    unsigned short epoch;
    unsigned short interfaceIndex;
    long long invocationId;
    m256i paramsDigest;
};
#pragma pack(pop)

static_assert(sizeof(OcAuthMessageBytes) == 13 + 2 + 2 + 8 + 32, "OcAuthMessageBytes must be exactly 57 bytes.");


// Single authorization signature for one invocation.
// Carried inside OcAuthSignatureTransaction's input payload, one or more per tx.
struct OcAuthSignatureItem
{
    long long invocationId;                          // 8 bytes
    unsigned short interfaceIndex;                   // 2 bytes
    unsigned short epoch;                            // 2 bytes
    unsigned char _padding[4];                       // 4 bytes, MUST be zero
    m256i paramsDigest;                              // 32 bytes; K12 over pinned OcRequest bytes
    unsigned char signature[SIGNATURE_SIZE];         // 64 bytes; SchnorrQ over authMessage
};

static_assert(sizeof(OcAuthSignatureItem) == 112, "OcAuthSignatureItem must be exactly 112 bytes.");


// Transaction emitted by a computor to authorize one or more OC invocations.
// The prefix is followed by the input payload (itemCount, padding, items), then the outer tx signature.
//
// Payload layout:
//     unsigned short itemCount
//     unsigned short _padding (= 0)
//     OcAuthSignatureItem items[itemCount]
//
// itemCount in [1, (MAX_INPUT_SIZE - 4) / sizeof(OcAuthSignatureItem)] = [1, 9].
struct OcAuthSignatureTransactionPrefix : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return 13;
    }

    static constexpr unsigned short minInputSize()
    {
        return 2 * sizeof(unsigned short) + sizeof(OcAuthSignatureItem); // header + 1 item
    }

    static constexpr unsigned short maxItemCount()
    {
        return (MAX_INPUT_SIZE - 2 * sizeof(unsigned short)) / sizeof(OcAuthSignatureItem);
    }
};

static_assert(OcAuthSignatureTransactionPrefix::maxItemCount() == 9, "Expected up to 9 items per OcAuthSignatureTransaction.");
