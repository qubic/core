#pragma once

#include "common_def.h"

#define REQUEST_SYSTEM_INFO 46


#define RESPOND_SYSTEM_INFO 47

#pragma pack(push, 1)
struct RespondSystemInfo
{
    short version;
    unsigned short epoch;
    unsigned int tick;
    unsigned int initialTick;
    unsigned int latestCreatedTick;

    unsigned short initialMillisecond;
    unsigned char initialSecond;
    unsigned char initialMinute;
    unsigned char initialHour;
    unsigned char initialDay;
    unsigned char initialMonth;
    unsigned char initialYear;

    unsigned int numberOfEntities;
    unsigned int numberOfTransactions;

    m256i randomMiningSeed;
    int solutionThreshold;

    unsigned long long totalSpectrumAmount;

    // Entity balances less or euqal this value will be burned if number of entites rises to 75% of spectrum capacity.
    // Starts to be meaningful if >50% of spectrum is filled but may still change after that.
    unsigned long long currentEntityBalanceDustThreshold;

    unsigned int targetTickVoteSignature;
    unsigned long long _reserve0;
    unsigned long long _reserve1;
    unsigned long long _reserve2;
    unsigned long long _reserve3;
    unsigned long long _reserve4;
};
#pragma pack(pop)

static_assert(sizeof(RespondSystemInfo) == 128, "Something is wrong with the struct size of RespondSystemInfo.");
