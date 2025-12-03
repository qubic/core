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

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_LOG;
    }
};


struct RespondLog
{
    // Variable-size log;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_LOG;
    }
};


// Request logid ranges from tx hash
struct RequestLogIdRangeFromTx
{
    unsigned long long passcode[4];
    unsigned int tick;
    unsigned int txId;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_LOG_ID_RANGE_FROM_TX;
    }
};


// Response logid ranges from tx hash
struct RespondLogIdRangeFromTx
{
    long long fromLogId;
    long long length;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_LOG_ID_RANGE_FROM_TX;
    }
};

// Request logId ranges (fromLogId, length) of all txs from a tick
struct RequestAllLogIdRangesFromTick
{
    unsigned long long passcode[4];
    unsigned int tick;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_ALL_LOG_ID_RANGES_FROM_TX;
    }
};


// Response logId ranges (fromLogId, length) of all txs from a tick
struct RespondAllLogIdRangesFromTick
{
    long long fromLogId[LOG_TX_PER_TICK];
    long long length[LOG_TX_PER_TICK];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_ALL_LOG_ID_RANGES_FROM_TX;
    }
};

// Request the node to prune logs (to save disk)
struct RequestPruningLog
{
    unsigned long long passcode[4];
    unsigned long long fromLogId;
    unsigned long long toLogId;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_PRUNING_LOG;
    }
};

// Response to above request, 0 if success, otherwise error code will be returned
struct RespondPruningLog
{
    long long success;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_PRUNING_LOG;
    }
};

// Request the digest of log event state, given requestedTick
struct RequestLogStateDigest
{
    unsigned long long passcode[4];
    unsigned int requestedTick;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_LOG_STATE_DIGEST;
    }
};

// Response above request, a 32 bytes digest
struct RespondLogStateDigest
{
    m256i digest;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_LOG_STATE_DIGEST;
    }
};