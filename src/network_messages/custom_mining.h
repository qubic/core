#pragma once

#include "lib/platform_common/qstdint.h"

#include "network_message_type.h"

enum CustomMiningType : uint8_t
{
    DOGE,
    TOTAL_NUM_TYPES // always keep this as last element
};

#pragma pack(push, 1) // pack all following structs tightly


// A generic custom mining struct that can contain mining task descriptions for different types.
struct CustomQubicMiningTask
{
    uint64_t jobId; // millisecond timestamp as dispatcher job id
    uint8_t customMiningType;

    // Followed by the specific task struct, e.g. QubicDogeMiningTask for CustomMiningType::DOGE.

    // Followed by the dispatcher signature (SIGNATURE_SIZE bytes).

    static constexpr unsigned char type()
    {
        return BROADCAST_CUSTOM_MINING_TASK;
    }
};


// A generic custom mining struct that can contain mining solutions for different types.
struct CustomQubicMiningSolution
{
    uint8_t sourcePublicKey[32]; // public key of the sender (miner), used for signature verification
    uint64_t jobId; // millisecond timestamp as dispatcher job id
    uint8_t customMiningType;

    // Followed by the specific solution struct, e.g. QubicDogeMiningSolution for CustomMiningType::DOGE.

    // Followed by the sender's signature (SIGNATURE_SIZE bytes).

    static constexpr unsigned char type()
    {
        return BROADCAST_CUSTOM_MINING_SOLUTION;
    }
};


// A struct for sending a mining task to the Qubic network.
struct QubicDogeMiningTask
{
    uint8_t cleanJobQueue; // flag indicating whether previous jobs should be dropped
    uint8_t dispatcherDifficulty[4]; // dispatcher difficulty, usually lower than pool and network difficulty, same compact format

    // The Dispatcher always expects a size of 8 bytes for the extraNonce2, 4 bytes for comp id, 4 bytes for miner to iterate.
    static constexpr unsigned int extraNonce2NumBytes = 8;

    // Data for building the block header, the byte arrays are in the
    // correct order for copying into the header directly.
    uint8_t version[4]; // version, little endian
    uint8_t nTime[4]; // timestamp, little endian
    uint8_t nBits[4]; // network difficulty, little endian
    uint8_t prevHash[32]; // previous hash, little endian
    unsigned int extraNonce1NumBytes;
    unsigned int coinbase1NumBytes;
    unsigned int coinbase2NumBytes;
    unsigned int numMerkleBranches;
    // Followed by the payload in the order
    // - extraNonce1
    // - coinbase1
    // - coinbase2
    // - merkleBranch1NumBytes (unsigned int), ... , merkleBranchNNumBytes (unsigned int)
    // - merkleBranch1, ... , merkleBranchN
    // Note: extraNonce1, coinbase1/2, and merkle branches have the same byte order as sent via stratum,
    // which should be correct for constructing the merkle root.
};


// A struct for receiving mining solutions from the Qubic network.
struct QubicDogeMiningSolution
{
    uint8_t nTime[4]; // the miner's rolling timestamp, little endian (same byte order as used in the block header)
    uint8_t nonce[4]; // little endian (same byte order as used in the block header)
    uint8_t merkleRoot[32]; // to avoid dispatcher having to calculate the root again, same byte order as used in the header
    uint8_t extraNonce2[8]; // same byte order as it was used to create the merkle root
};

#pragma pack(pop) // restore original alignment
