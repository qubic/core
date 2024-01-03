#pragma once

#include "common_def.h"



struct RequestContractIPO
{
    unsigned int contractIndex;

    enum {
        type = 33,
    };
};


struct RespondContractIPO
{
    unsigned int contractIndex;
    unsigned int tick;
    m256i publicKeys[NUMBER_OF_COMPUTORS];
    long long prices[NUMBER_OF_COMPUTORS];

    enum {
        type = 34,
    };
};

static_assert(sizeof(RespondContractIPO) == 4 + 4 + 32 * NUMBER_OF_COMPUTORS + 8 * NUMBER_OF_COMPUTORS, "Something is wrong with the struct size.");


struct RequestContractFunction // Invokes contract function
{
    unsigned int contractIndex;
    unsigned short inputType;
    unsigned short inputSize;
    // Variable-size input

    enum {
        type = 42,
    };
};


struct RespondContractFunction // Returns result of contract function invocation
{
    // Variable-size output; the size must be 0 if the invocation has failed for whatever reason (e.g. no a function registered for [inputType], or the function has timed out)

    enum {
        type = 43,
    };
};
