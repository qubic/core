#pragma once

#include "network_messages/common_def.h"
#include "contracts/qpi.h"

#include "system.h"

#include "oracle_transactions.h"
#include "core_om_network_messages.h"

#include "platform/memory_util.h"

struct OracleQueryMetadata
{
    uint64_t queryId;
    uint8_t type;             ///< contract query, user query, subscription (may be by multiple contracts)
    uint8_t status;           ///< overall status (pending -> success or timeout)
    uint16_t errorFlags;      ///< error flags (especially as returned by oracle machine connected to this node)
    uint32_t interfaceIndex;
    uint32_t queryTick;
    uint32_t revealTick;
    uint32_t revealTxIndex;
    QPI::DateAndTime timeout;

    union
    {
        struct
        {
            m256i queryingEntity;
            uint32_t queryTxIndex; // query tx index in tick

        } user;

        struct
        {
            uint64_t queryStorageOffset;
            uint16_t queryingContract;
        } contract;

        struct
        {
            uint64_t queryStorageOffset;
            uint32_t subscriptionId; // needs value reserved for "no subscription"
        } subscription;
    } varStruct;



};

// One per contract
struct OracleContractStatus
{
    uint32_t tick;
    uint16_t queryIndexInTick;
};

struct OracleSubscriptionContractStatus
{
    uint32_t subscriptionId;
    QPI::DateAndTime nextNotification;
    uint16_t notificationPeriodMinutes;
    uint16_t contractIndex;
};

struct OracleSubscriptionMetadata
{
    // Number of minutes between consecutive queries, scheduler triggers query in processTick()
    uint16_t queryPeriodMinutes;

    // Offset of DateAndTime timestamp in oracle query data
    uint16_t queryTimestampOffset;

    // ID of last query
    uint64_t lastQueryId;

    // ID of last revealed, for notifying with cached value on new subscription?
    uint64_t lastRevealId;

    // Timestamp for triggering next query and notifying about timeout if no query was received
    QPI::DateAndTime nextQueryTimestamp;
};

// State of received commits for a single oracle query
struct OracleReplyCommitState
{
    uint64_t queryId;
    uint16_t totalCommits;
    m256i replyDigests[NUMBER_OF_COMPUTORS];
    m256i replyKnowledgeProofs[NUMBER_OF_COMPUTORS];
    uint32_t replyTicks[NUMBER_OF_COMPUTORS];
    m256i mostCommitsDigest;
    uint16_t mostCommitsCount;
};

// 
struct OracleRevenueCounter
{
    uint32_t computorRevPoints[NUMBER_OF_COMPUTORS];
};

constexpr uint32_t MAX_ORACLE_QUERIES = (1 << 18);
constexpr uint64_t ORACLE_QUERY_STORAGE_SIZE = MAX_ORACLE_QUERIES * 512;

// TODO: locking, implement hash function for queryIdToIndex based on tick
class OracleEngine
{
    /// array of all oracle queries of the epoch with capacity for MAX_ORACLE_QUERIES elements
    OracleQueryMetadata* oracleQueries;

    /// number of elements used in oracleQueries
    uint32_t oracleQueryCount;

    // buffer continuously filled with variable-size query data (similar to tx storage)
    uint8_t* queryStorage;

    // how many bytes of queryStorage array are already in use / offset for adding new data
    uint64_t queryStorageBytesUsed;

    // status of each contract for assigning IDs
    OracleContractStatus contractStatus[MAX_NUMBER_OF_CONTRACTS];



    /// fast lookup of oracle query index (sequential position in oracleQueries array) from oracle query ID (composed of query tick and other info)
    QPI::HashMap<uint64_t, uint32_t, MAX_ORACLE_QUERIES> queryIdToIndex;

public:
    bool init()
    {
        // alloc arrays and set to 0
        if (!allocPoolWithErrorLog(L"OracleEngine::oracleQueries", MAX_ORACLE_QUERIES * sizeof(*oracleQueries), (void**)&oracleQueries, __LINE__)
            || !!allocPoolWithErrorLog(L"OracleEngine::queryStorage", ORACLE_QUERY_STORAGE_SIZE, (void**)&queryStorage, __LINE__))
        {
            return false;
        }

        oracleQueryCount = 0;
        queryStorageBytesUsed = 1; // reserve offset 0 for "no data"
        setMem(contractStatus, sizeof(contractStatus), 0);

        return true;
    }

    void deinit()
    {
        // free arrays
    }

    void save() const
    {
        // save state (excluding queryIdToIndex and unused parts of large buffers)
    }

    void load()
    {
        // load state (excluding queryIdToIndex and unused parts of large buffers)
        // init queryIdToIndex
    }

    /**
    * Check and start user query based on transaction (should be called from tick processor).
    * @param tx Transaction, whose validity and signature has been checked before.
    * @param txIndex Index of tx in tick data, for referencing in tick storage.
    * @return Query ID or zero on error.
    */
    uint64_t startUserQuery(const OracleUserQueryTransactionPrefix* tx, uint32_t txIndex)
    {
        // ASSERT that tx is in tick storage at tx->tick, txIndex.
        // check interface index
        // check size of payload vs expected query of given interface
        // add to query storage
        // send query to oracle machine node
    }

    uint64_t startContractQuery(uint16_t contractIndex, uint32_t interfaceIndex, const void* queryData, uint16_t querySize, uint16_t timeoutSeconds)
    {
        // TODO: check that querySize matches registered size

        if (oracleQueryCount >= MAX_ORACLE_QUERIES || queryStorageBytesUsed + querySize > ORACLE_QUERY_STORAGE_SIZE || contractIndex >= MAX_NUMBER_OF_CONTRACTS)
            return 0;

        DateAndTime timeout = DateAndTime::now();
        if (!timeout.add(0, 0, 0, 0, 0, timeoutSeconds))
            return 0;

        // get sequential query index of contract in tick
        auto& cs = contractStatus[contractIndex];
        if (cs.tick < system.tick)
        {
            cs.tick = system.tick;
            cs.queryIndexInTick = 0;
        }
        else
        {
            if (cs.queryIndexInTick == 0xffff)
                return 0;
            ++cs.queryIndexInTick;
        }

        // compose query ID
        uint64_t queryId = ((uint64_t)system.tick << 32) | (contractIndex << 16) | cs.queryIndexInTick;

        ASSERT(!queryIdToIndex.contains(queryId));
        if (queryIdToIndex.set(queryId, oracleQueryCount) == NULL_INDEX)
            return false;

        auto& queryMetadata = oracleQueries[oracleQueryCount++];
        queryMetadata.queryId = queryId;
        queryMetadata.type = ORACLE_QUERY_TYPE_CONTRACT_QUERY;
        queryMetadata.status = ORACLE_QUERY_STATUS_PENDING;
        queryMetadata.errorFlags = ORACLE_FLAG_REPLY_PENDING;
        queryMetadata.interfaceIndex = interfaceIndex;
        queryMetadata.queryTick = system.tick;
        queryMetadata.revealTick = 0;
        queryMetadata.revealTxIndex = 0;
        queryMetadata.timeout = timeout;
        queryMetadata.varStruct.contract.queryingContract = contractIndex;
        queryMetadata.varStruct.contract.queryStorageOffset = queryStorageBytesUsed;

        copyMem(queryStorage + queryStorageBytesUsed, queryData, querySize);
        queryStorageBytesUsed += querySize;

        enqueuOracleQuery(queryId, interfaceIndex, timeoutSeconds, queryData, querySize);

        return 0;
    }

    // Enqueue oracle machine query message. May be called from tick processor or contract processor only (uses reorgBuffer).
    void enqueuOracleQuery(uint64_t queryId, uint32_t interfaceIdx, uint16_t timeoutInSeconds, const void* queryData, uint16 querySize)
    {
        // Preapre message payload
        OracleMachineQuery* omq = reinterpret_cast<OracleMachineQuery*>(reorgBuffer);
        omq->oracleQueryId = queryId;
        omq->oracleInterfaceIndex = interfaceIdx;
        omq->timeoutInSeconds = timeoutInSeconds;
        copyMem(omq + 1, queryData, querySize);

        // Enqueue for sending to all oracle machine peers (peer pointer address 0x1 is reserved for that)
        enqueueResponse((Peer*)0x1, sizeof(*omq) + querySize, OracleMachineQuery::type, 0, omq);
    }
};

OracleEngine oracleEngine;

/*
- Implement one-time-query by contract in TESTEXD
- build 

- Handle seamless transitions? Keep state?

*/
