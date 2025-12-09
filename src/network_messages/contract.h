#pragma once

#include "common_def.h"


struct RequestActiveIPOs
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_ACTIVE_IPOS;
    }
};


struct RespondActiveIPO
{
    unsigned int contractIndex;
    char assetName[8];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_ACTIVE_IPO;
    }
};

struct RequestContractIPO
{
    unsigned int contractIndex;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_CONTRACT_IPO;
    }
};


struct RespondContractIPO
{
    unsigned int contractIndex;
    unsigned int tick;
    m256i publicKeys[NUMBER_OF_COMPUTORS];
    long long prices[NUMBER_OF_COMPUTORS];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_CONTRACT_IPO;
    }
};

static_assert(sizeof(RespondContractIPO) == 4 + 4 + 32 * NUMBER_OF_COMPUTORS + 8 * NUMBER_OF_COMPUTORS, "Something is wrong with the struct size.");


struct RequestContractFunction // Invokes contract function
{
    unsigned int contractIndex;
    unsigned short inputType;
    unsigned short inputSize;
    // Variable-size input

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_CONTRACT_FUNCTION;
    }
};


struct RespondContractFunction // Returns result of contract function invocation
{
    // Variable-size output; the size must be 0 if the invocation has failed for whatever reason (e.g. no a function registered for [inputType], or the function has timed out)

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_CONTRACT_FUNCTION;
    }
};
