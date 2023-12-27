#pragma once

#include "common_def.h"

// Use "#pragma pack" keep the binary struct compatibility after changing from unsigned char [32] to m256i
#pragma pack(push,1)
struct Computors
{
    // TODO: Padding
    unsigned short epoch;
    m256i publicKeys[NUMBER_OF_COMPUTORS];
    unsigned char signature[SIGNATURE_SIZE];
};
#pragma pack(pop)

static_assert(sizeof(Computors) == 2 + 32 * NUMBER_OF_COMPUTORS + SIGNATURE_SIZE, "Something is wrong with the struct size.");

struct BroadcastComputors
{
    Computors computors;

    enum {
        type = 2,
    };
};


struct RequestComputors
{
    enum {
        type = 11,
    };
};

