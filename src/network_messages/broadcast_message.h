#pragma once

#include "common_def.h"

#define MESSAGE_TYPE_SOLUTION 0

// TODO: documentation needed:
// "A General Message type used to send/receive messages from/to peers." -> right?
// What the the gammingNonce about exactly?
// Which other message types are planned next to MESSAGE_TYPE_SOLUTION?
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
