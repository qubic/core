#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"

#include "network_messages/special_command.h"

#define MAX_NUMBER_OF_SOLUTIONS 65536 // Must be 2^N



struct System
{
    short version;
    unsigned short epoch;
    unsigned int tick;
    unsigned int initialTick;
    unsigned int latestCreatedTick;
    unsigned int latestLedTick; // contains latest tick t in which TickData for tick (t + TICK_TRANSACTIONS_PUBLICATION_OFFSET) was broadcasted as tick leader

    unsigned short initialMillisecond;
    unsigned char initialSecond;
    unsigned char initialMinute;
    unsigned char initialHour;
    unsigned char initialDay;
    unsigned char initialMonth;
    unsigned char initialYear;

    unsigned long long latestOperatorNonce;

    unsigned int numberOfSolutions;
    struct Solution
    {
        m256i computorPublicKey;
        m256i miningSeed;
        m256i nonce;
    } solutions[MAX_NUMBER_OF_SOLUTIONS];

    m256i futureComputors[NUMBER_OF_COMPUTORS];
};
static_assert(sizeof(System) == 20 + 8 + 8 + 8 + 4 + 96 * MAX_NUMBER_OF_SOLUTIONS + 32 * NUMBER_OF_COMPUTORS, "Unexpected size");

GLOBAL_VAR_DECL System system;
