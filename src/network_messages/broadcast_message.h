#pragma once

#include "common_def.h"

#define MESSAGE_TYPE_SOLUTION 0

// TODO: documentation needed:
// "A General Message type used to send/receive messages from/to peers." -> right?
// What the the gammingNonce about exactly?
// Which other message types are planned next to MESSAGE_TYPE_SOLUTION?
//
// MESSAGE_TYPE_SOLUTION
//  sourcePublicKey Must not be NULL_ID. It can be a signing public key or a computor/candidate public key.
//  destinationPublicKey Public key of a computor/candidate controlled by a node.
//  gammingNonce There are two cases:
//  - If sourcePublicKey is just a signing pubkey: the first 32 bytes are zeros, and the last 32 bytes are taken from the message.
//  - If sourcePublicKey is a public key of a computor: the message is encrypted.
struct BroadcastMessage
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    m256i gammingNonce;

    enum {
        type = 1,
    };
};

static_assert(sizeof(BroadcastMessage) == 32 + 32 + 32, "Something is wrong with the struct size.");

struct CustomMiningTaskMessage
{
    m256i sourcePublicKey;
    m256i zero;
    m256i gammingNonce;

    m256i codeFileTrailerDigest;
    m256i dataFileTrailerDigest;

    // Task payload
};

struct CustomMiningSolutionMessage
{
    m256i sourcePublicKey;
    m256i zero;
    m256i gammingNonce;

    m256i codeFileTrailerDigest;
    m256i dataFileTrailerDigest;

    // Solution payload
};