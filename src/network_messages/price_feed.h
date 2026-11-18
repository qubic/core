#pragma once

#include "common_def.h"

// Read-only view of an hourly price series a node holds.

static constexpr unsigned int PRICE_SERIES_MAX_POINTS_PER_RESPONSE = 256;

struct RequestPriceSeries
{
    unsigned int fromHour;
    unsigned int count;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_PRICE_SERIES;
    }
};

#pragma pack(push, 1)
struct RespondPriceSeriesPoint
{
    unsigned int hour;
    unsigned int appendTick;
    unsigned int flags;
    long long priceE8;
};

// Only the first `count` points are sent; hours the node does not hold are left out.
struct RespondPriceSeries
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_PRICE_SERIES;
    }

    unsigned int tick;
    unsigned short epoch;
    unsigned char synced;
    unsigned char bound;
    unsigned int source;
    unsigned int nextHour;
    unsigned int failedAttempts;
    long long firstFailMs;
    long long lastFailMs;
    unsigned char seriesDigest[32];
    unsigned long long numSuccess;
    unsigned long long numTimeout;
    unsigned long long numUnresolvable;
    unsigned long long numInvalidReply;
    unsigned long long numStartFailed;
    unsigned long long numFilled;
    // Last hour known when the epoch began; target t of the epoch belongs to hour baseHour - 8759 + t.
    unsigned int baseHour;
    unsigned int count;
    RespondPriceSeriesPoint points[PRICE_SERIES_MAX_POINTS_PER_RESPONSE];
};
#pragma pack(pop)

static constexpr unsigned int RESPOND_PRICE_SERIES_HEADER_SIZE = 124;

// Task data a solution anchored at anchorTick is graded on.
struct RequestBpp9000TaskData
{
    unsigned int anchorTick;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_BPP9000_TASK_DATA;
    }
};

static constexpr unsigned int BPP9000_TASK_DATA_MAX_TARGETS = 24 * 365 + 24 * 7;

#pragma pack(push, 1)
// Only the known targets are sent: one frame plus shiftLimit, none if not valid.
struct RespondBpp9000TaskData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_BPP9000_TASK_DATA;
    }

    unsigned int tick;
    unsigned int anchorTick;
    unsigned short epoch;
    // 1: the node holds the series and has processed every tick before anchorTick. 0: ignore the rest.
    unsigned char valid;
    unsigned char reserved;
    unsigned int baseHour;
    // Last frame a solution with this anchor tick may reach.
    unsigned int shiftLimit;
    // 0 or 1 per hour.
    unsigned char targets[BPP9000_TASK_DATA_MAX_TARGETS];
};
#pragma pack(pop)

static constexpr unsigned int RESPOND_BPP9000_TASK_DATA_HEADER_SIZE = 20;

static_assert(sizeof(RequestBpp9000TaskData) == 4, "unexpected request size");
static_assert(sizeof(RespondBpp9000TaskData) == RESPOND_BPP9000_TASK_DATA_HEADER_SIZE + BPP9000_TASK_DATA_MAX_TARGETS, "unexpected response size");

static_assert(sizeof(RequestPriceSeries) == 8, "unexpected request size");
static_assert(sizeof(RespondPriceSeriesPoint) == 20, "unexpected point size");
static_assert(sizeof(RespondPriceSeries) == RESPOND_PRICE_SERIES_HEADER_SIZE + PRICE_SERIES_MAX_POINTS_PER_RESPONSE * sizeof(RespondPriceSeriesPoint),
    "unexpected response size");
