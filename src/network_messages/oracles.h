#pragma once

#include "common_def.h"


// Options to request oracle queries/replies/subscriptions:
// - all oracle query IDs of a given tick
// - all oracle query IDs of user oracle queries of a given tick (subset of 1)
// - all oracle query IDs of contract oracle queries of a given tick (subset of 1)
// - all oracle query IDs of queries of a given tick triggered by subscription (subset of 1)
// - all oracle query IDs that are pending (status is neither success nor failure)
// - for given oracle query ID: metadata, query, and response if available
// - subscription info for given oracle subscription ID
// - subscription IDs of all active subscriptions
// - subscription IDs of active subscriptions of a given contract
// - query statistics of this node (counts and durations)
// - query oracle revenue points
struct RequestOracleData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_ORACLE_DATA;
    }

    // type of oracle request
    static constexpr unsigned int requestAllQueryIdsByTick = 0;
    static constexpr unsigned int requestUserQueryIdsByTick = 1;
    static constexpr unsigned int requestContractDirectQueryIdsByTick = 2;
    static constexpr unsigned int requestContractSubscriptionQueryIdsByTick = 3;
    static constexpr unsigned int requestPendingQueryIds = 4;
    static constexpr unsigned int requestQueryAndResponse = 5;
    static constexpr unsigned int requestSubscription = 6;
    static constexpr unsigned int requestQueryStatistics = 7;
    static constexpr unsigned int requestOracleRevenuePoints = 8;
    static constexpr unsigned int requestActiveSubscriptions = 9;
    static constexpr unsigned int requestActiveContractSubscriptions = 10;
    unsigned int reqType;

    unsigned int _padding;

    // tick, query ID, subscription ID, or contract index (depending on reqType)
    long long reqTickOrId;
};

static_assert(sizeof(RequestOracleData) == 16, "Something is wrong with the struct size.");

// Response to RequestOracleData. This is just the header, the payload following depends on resType and is described below.
// The size of the payload can be calculated from the RequestResponseHeader size.
// One request may be followed by multiple responses, finished by END_RESPONSE.
struct RespondOracleData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_ORACLE_DATA;
    }

    // The payload is an array of 8-byte query IDs.
    static constexpr unsigned int respondQueryIds = 0;

    // The payload is RespondOracleDataQueryMetadata.
    static constexpr unsigned int respondQueryMetadata = 1;

    // The payload is the OracleQuery data as defined by the oracle interface type.
    static constexpr unsigned int respondQueryData = 2;

    // The payload is the OracleReply data as defined by the oracle interface type.
    static constexpr unsigned int respondReplyData = 3;

    // The payload is an array of 2-byte contract indices.
    static constexpr unsigned int respondNotifiedSubscriberContracts = 4;

    // The payload is RespondOracleDataSubscription.
    static constexpr unsigned int respondSubscription = 5;

    // The payload is RespondOracleDataSubscriber.
    static constexpr unsigned int respondSubscriber = 6;

    // The payload is RespondOracleDataQueryStatistics.
    static constexpr unsigned int respondQueryStatistics = 7;

    // The payload is an array of 676 revenue points (one per computer, each 8 bytes).
    static constexpr unsigned int respondOracleRevenuePoints = 8;

    // The payload is RespondOracleDataValidTickRange.
    static constexpr unsigned int respondTickRange = 9;

    // The payload is an array of 4-byte subscription IDs.
    static constexpr unsigned int respondSubscriptionIds = 10;

    // type of oracle response
    unsigned int resType;
};

static_assert(sizeof(RespondOracleData) == 4, "Something is wrong with the struct size.");

struct RespondOracleDataQueryMetadata
{
    int64_t queryId;
    uint8_t type;               ///< contract query, user query, subscription (may be by multiple contracts)
    uint8_t status;             ///< overall status (pending -> success or timeout)
    uint16_t statusFlags;       ///< status and error flags (especially as returned by oracle machine connected to this node)
    uint32_t queryTick;
    m256i queryingEntity;
    uint64_t timeout;           ///< Timeout in QPI::DateAndTime format
    uint32_t interfaceIndex;
    int32_t subscriptionId;     ///< -1 is reserved for "no subscription"
    uint32_t revealTick;        ///< Tick of reveal tx. Only available if status is success.
    uint16_t totalCommits;      ///< Total number of commit tx. Only available if status isn't success.
    uint16_t agreeingCommits;   ///< Number of agreeing commit tx (biggest group with same digest). Only available if status isn't success.
};

static_assert(sizeof(RespondOracleDataQueryMetadata) == 72, "Unexpected struct size");

struct RespondOracleDataSubscription
{
    int32_t subscriptionId;
    uint32_t interfaceIndex;
    int64_t lastPendingQueryId;
    int64_t lastRevealedQueryId;
    uint32_t generatedQueriesCount;
};

struct RespondOracleDataSubscriber
{
    int32_t subscriptionId;
    uint16_t contractIndex;
    uint16_t notificationPeriodMinutes;
    uint64_t nextQueryTimestamp;    ///< timestamp of next query in QPI::DateAndTime format
};

struct RespondOracleDataQueryStatistics
{
    uint64_t pendingCount;
    uint64_t pendingOracleMachineCount;
    uint64_t pendingCommitCount;
    uint64_t pendingRevealCount;

    uint64_t successfulCount;
    uint64_t revealTxCount;

    uint64_t unresolvableCount;

    uint64_t timeoutCount;
    uint64_t timeoutNoReplyCount;
    uint64_t timeoutNoCommitCount;
    uint64_t timeoutNoRevealCount;

    /// For how many queries multiple OM nodes connected to this Core node sent differing replies
    uint64_t oracleMachineRepliesDisagreeCount;

    /// How many thousandth ticks it takes on average until the OM reply is received
    uint64_t oracleMachineReplyAvgMilliTicksPerQuery;

    /// How many thousandth ticks it takes on average until the commit status is reached (until 451 commit tx got executed)
    uint64_t commitAvgMilliTicksPerQuery;

    /// How many thousandth ticks it takes on average until until success (ticks until 1 reveal tx got executed)
    uint64_t successAvgMilliTicksPerQuery;

    /// How many thousandth ticks it takes on average until until timeout (only considering cases in which timeout happened)
    uint64_t timeoutAvgMilliTicksPerQuery;

    /// Total number of commit with correct digest but wrong knowledge proof
    uint64_t wrongKnowledgeProofCount;

    /// Total number of queries by users
    unsigned long long userQueries;

    /// Total number of one-time queries by contracts
    unsigned long long contractQueries;

    /// Total number of subscription queries
    unsigned long long subscriptionQueries;
};

// Core node responds with this if the requested query tick is outside the range of ticks with available query info.
struct RespondOracleDataValidTickRange
{
    uint32_t firstTick;
    uint32_t currentTick;
};
