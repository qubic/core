#pragma once

#include "common_def.h"


struct Tick
{
    unsigned short computorIndex;
    unsigned short epoch;
    unsigned int tick;

    unsigned short millisecond;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;

    unsigned long long prevResourceTestingDigest;
    unsigned long long saltedResourceTestingDigest;

    m256i prevSpectrumDigest;
    m256i prevUniverseDigest;
    m256i prevComputerDigest;
    m256i saltedSpectrumDigest;
    m256i saltedUniverseDigest;
    m256i saltedComputerDigest;

    m256i transactionDigest;
    m256i expectedNextTickTransactionDigest;

    unsigned char signature[SIGNATURE_SIZE];
};

static_assert(sizeof(Tick) == 8 + 8 + 16 + 6 * 32 + 2 * 32 + SIGNATURE_SIZE, "Something is wrong with the struct size.");


struct BroadcastTick
{
    Tick tick;

    enum {
        type = 3,
    };
};



struct TickData
{
    unsigned short computorIndex;
    unsigned short epoch;
    unsigned int tick;

    unsigned short millisecond;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;

    m256i timelock;
    m256i transactionDigests[NUMBER_OF_TRANSACTIONS_PER_TICK];
    long long contractFees[MAX_NUMBER_OF_CONTRACTS];

    unsigned char signature[SIGNATURE_SIZE];
};

static_assert(sizeof(TickData) == 8 + 8 + 32 + NUMBER_OF_TRANSACTIONS_PER_TICK * 32 + 8 * MAX_NUMBER_OF_CONTRACTS + SIGNATURE_SIZE, "Something is wrong with the struct size.");


struct BroadcastFutureTickData
{
    TickData tickData;

    enum {
        type = 8,
    };
};


struct RequestedQuorumTick
{
    unsigned int tick;
    unsigned char voteFlags[(NUMBER_OF_COMPUTORS + 7) / 8];
};


struct RequestQuorumTick
{
    RequestedQuorumTick quorumTick;

    enum {
        type = 14,
    };
};


struct RequestedTickData
{
    unsigned int tick;
};


struct RequestTickData
{
    RequestedTickData requestedTickData;

    enum {
        type = 16,
    };
};


#define REQUEST_CURRENT_TICK_INFO 27

#define RESPOND_CURRENT_TICK_INFO 28

struct CurrentTickInfo
{
    unsigned short tickDuration;
    unsigned short epoch;
    unsigned int tick;
    unsigned short numberOfAlignedVotes;
    unsigned short numberOfMisalignedVotes;
    unsigned int initialTick;
};

