#pragma once

struct RequestedCustomMiningVerification
{
    enum
    {
        type = 55,
    };
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned long long taskIndex;
    unsigned int nonce;
    unsigned int padding; // use later
};

