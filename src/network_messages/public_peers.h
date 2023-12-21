#pragma once

#include "common_def.h"

struct ExchangePublicPeers
{
    unsigned char peers[NUMBER_OF_EXCHANGED_PEERS][4];

    enum {
        type = 0,
    };
};

