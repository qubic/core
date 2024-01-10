#pragma once

#include "common_def.h"

struct ExchangePublicPeers
{
    IPv4Address peers[NUMBER_OF_EXCHANGED_PEERS];

    enum {
        type = 0,
    };
};

static_assert(sizeof(ExchangePublicPeers) == 4 * NUMBER_OF_EXCHANGED_PEERS, "Unexpected size!");
