#pragma once

#include "common_def.h"

struct ExchangePublicPeers
{
    IPv4Address peers[NUMBER_OF_EXCHANGED_PEERS];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::EXCHANGE_PUBLIC_PEERS;
    }
};

static_assert(sizeof(ExchangePublicPeers) == 4 * NUMBER_OF_EXCHANGED_PEERS, "Unexpected size!");
