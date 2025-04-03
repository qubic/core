#pragma once

#include "common_def.h"

#define LOG_TX_NUMBER_OF_SPECIAL_EVENT 5
#define LOG_TX_PER_TICK (NUMBER_OF_TRANSACTIONS_PER_TICK + LOG_TX_NUMBER_OF_SPECIAL_EVENT)// +5 special events

// Fetches log
struct RequestLog
{
    unsigned long long passcode[4];
    unsigned long long fromID;
    unsigned long long toID; // inclusive

    enum {
        type = 44,
    };
};


struct RespondLog
{
    // Variable-size log;

    enum {
        type = 45,
    };
};


// Request logid ranges from tx hash
struct RequestLogIdRangeFromTx
{
    unsigned long long passcode[4];
    unsigned int tick;
    unsigned int txId;

    enum {
        type = 48,
    };
};


// Response logid ranges from tx hash
struct ResponseLogIdRangeFromTx
{
    long long fromLogId;
    long long length;

    enum {
        type = 49,
    };
};

// Request logId ranges (fromLogId, length) of all txs from a tick
struct RequestAllLogIdRangesFromTick
{
    unsigned long long passcode[4];
    unsigned int tick;

    enum {
        type = 50,
    };
};


// Response logId ranges (fromLogId, length) of all txs from a tick
struct ResponseAllLogIdRangesFromTick
{
    long long fromLogId[LOG_TX_PER_TICK];
    long long length[LOG_TX_PER_TICK];

    enum {
        type = 51,
    };
};

// Request the node to prune logs (to save disk)
struct RequestPruningLog
{
    unsigned long long passcode[4];
    unsigned long long fromLogId;
    unsigned long long toLogId;

    enum {
        type = 56,
    };
};

// Response to above request, 0 if success, otherwise error code will be returned
struct ResponsePruningLog
{
    long long success;
    enum {
        type = 57,
    };
};

// Request the digest of log event state, given requestedTick
struct RequestLogStateDigest
{
    unsigned long long passcode[4];
    unsigned int requestedTick;

    enum {
        type = 58,
    };
};

// Response above request, a 32 bytes digest
struct ResponseLogStateDigest
{
    m256i digest;
    enum {
        type = 59,
    };
};