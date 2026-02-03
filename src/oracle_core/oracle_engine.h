#pragma once

#include "contract_core/pre_qpi_def.h"
#include "contracts/qpi.h"
#include "oracle_core/oracle_interfaces_def.h"

#include "system.h"
#include "spectrum/special_entities.h"
#include "spectrum/spectrum.h"
#include "ticking/tick_storage.h"
#include "logging/logging.h"
#include "text_output.h"

#include "oracle_transactions.h"
#include "core_om_network_messages.h"

#include "platform/memory_util.h"


void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);

constexpr uint32_t MAX_ORACLE_QUERIES = (1 << 18);
constexpr uint64_t ORACLE_QUERY_STORAGE_SIZE = MAX_ORACLE_QUERIES * 512;
constexpr uint32_t MAX_SIMULTANEOUS_ORACLE_QUERIES = 1024;

#pragma pack(push, 4)
struct OracleQueryMetadata
{
    int64_t queryId;          ///< bits 31-62 encode index in tick, bits 0-30 are index in tick, negative values indicate error
    uint8_t type;             ///< contract query, user query, subscription (may be by multiple contracts)
    uint8_t status;           ///< overall status (pending -> success or timeout)
    uint16_t statusFlags;     ///< status and error flags (especially as returned by oracle machine connected to this node)
    uint32_t queryTick;
    QPI::DateAndTime timeout;
    uint32_t interfaceIndex;

    union
    {
        struct
        {
            uint32_t queryTxIndex; // query tx index in tick
            m256i queryingEntity;
        } user;

        struct
        {
            uint32_t notificationProcId;
            uint64_t queryStorageOffset;
            uint16_t queryingContract;
        } contract;

        struct
        {
            uint32_t subscriptionId; ///< 0 is reserved for "no subscription"
            uint64_t queryStorageOffset;
        } subscription;
    } typeVar;

    union
    {
        /// used before notification
        struct
        {
            uint32_t replyStateIndex;
        } pending;

        /// used after after reveal and notification
        struct
        {
            uint32_t revealTick;    ///< tick of first reveal tx (and notification)
            uint32_t revealTxIndex; ///< tx of reveal tx in tick
        } success;

        /// used after after failure
        struct
        {
            uint16_t totalCommits;
            uint16_t agreeingCommits;
        } failure;
    } statusVar;
};
#pragma pack(pop)

static_assert(sizeof(OracleQueryMetadata) == 72, "Unexpected struct size");


struct OracleSubscriptionContractStatus
{
    int32_t subscriptionId;
    uint16_t contractIndex;
    uint16_t notificationPeriodMinutes;
    QPI::DateAndTime nextNotification;
    USER_PROCEDURE notificationProcedure;
    uint32_t notificationLocalsSize;
};

struct OracleSubscriptionMetadata
{
    uint64_t initialQueryStorageOffset;

    // Number of minutes between consecutive queries, scheduler triggers query in processTick()
    uint16_t queryIntervalMinutes;

    // Offset of DateAndTime timestamp member variable in oracle query struct
    uint16_t queryTimestampOffset;

    // Query ID of last query (pending)
    int64_t lastQueryId;

    // Query ID of last revealed query, for notifying with cached value on new subscription?
    int64_t lastRevealId;

    // Timestamp for triggering next query and notifying about timeout if no query was received
    QPI::DateAndTime nextQueryTimestamp;
};

// State of received OM reply and computor commits for a single oracle query
template <uint16_t ownComputorSeedsCount>
struct OracleReplyState
{
    int64_t queryId;

    m256i ownReplyDigest;
    uint16_t ownReplySize;
    uint8_t ownReplyData[MAX_ORACLE_REPLY_SIZE + 2];

    // track state of own reply commits (when they are scheduled and when actually got executed)
    uint16_t ownReplyCommitExecCount;
    uint32_t ownReplyCommitComputorTxTick[ownComputorSeedsCount];
    uint32_t ownReplyCommitComputorTxExecuted[ownComputorSeedsCount];

    m256i replyCommitDigests[NUMBER_OF_COMPUTORS];
    m256i replyCommitKnowledgeProofs[NUMBER_OF_COMPUTORS];
    uint32_t replyCommitTicks[NUMBER_OF_COMPUTORS];

    uint16_t replyCommitHistogramIdx[NUMBER_OF_COMPUTORS];
    uint16_t replyCommitHistogramCount[NUMBER_OF_COMPUTORS];
    uint16_t mostCommitsHistIdx;
    uint16_t totalCommits;

    uint32_t expectedRevealTxTick;
    uint32_t revealTick;
    uint32_t revealTxIndex;
};

// 
struct OracleRevenueCounter
{
    uint32_t computorRevPoints[NUMBER_OF_COMPUTORS];
};

struct OracleNotificationData
{
    uint32_t procedureId;
    uint16_t contractIndex;
    uint16_t inputSize;
    uint8_t inputBuffer[16 + MAX_ORACLE_REPLY_SIZE];
};

// Array with fast insert and remove, for which order of entries does not matter. Entry duplicates are possible.
template <typename T, unsigned int N>
struct UnsortedMultiset
{
    T values[N];
    unsigned int numValues;

    bool add(const T& v)
    {
        ASSERT(numValues <= N);
        if (numValues >= N)
            return false;
        values[numValues++] = v;
        return true;
    }

    bool removeByIndex(unsigned int idx)
    {
        ASSERT(numValues <= N);
        if (idx >= numValues || numValues == 0)
            return false;
        --numValues;
        if (idx != numValues)
        {
            values[idx] = values[numValues];
        }
        return true;
    }

    bool removeByValue(const T& v)
    {
        unsigned int idx = 0;
        bool removedAny = false;
        while (idx < numValues)
        {
            if (values[idx] == v)
                removedAny = removedAny || removeByIndex(idx);
            else
                ++idx;
        }
        return removedAny;
    }
};

struct OracleEngineStatistics
{
    /// total number of successful oracle queries
    unsigned long long successCount;

    /// sum of ticks that were required to reach the success state
    unsigned long long successTicksSum;

    /// total number of timeout oracle queries without reply from oracle machine
    unsigned long long timeoutNoReplyCount;

    /// total number of timeout oracle queries without commit quorum
    unsigned long long timeoutNoCommitCount;

    /// total number of timeout oracle queries without reveal
    unsigned long long timeoutNoRevealCount;

    /// sum of ticks until timeout of all timeout cases
    unsigned long long timeoutTicksSum;

    /// total number of unresolvable oracle queries
    unsigned long long unresolvableCount;

    /// total number of oracle queries that got oracle machine reply locally
    unsigned long long oracleMachineReplyCount;

    /// sum of ticks that were required to get oracle machine reply locally
    unsigned long long oracleMachineReplyTicksSum;

    /// total number of oracle queries that reached commit state
    unsigned long long commitCount;

    /// sum of ticks that were required to reach the commit state
    unsigned long long commitTicksSum;

    /// total number of oracle machine replies that disagree with the first reply received for a query
    unsigned long long oracleMachineRepliesDisagreeCount;

    /// total number of reply reveal transactions
    unsigned long long revealTxCount;
};


template <uint16_t ownComputorSeedsCount>
class OracleEngine
{
protected:
    /// array of all oracle queries of the epoch with capacity for MAX_ORACLE_QUERIES elements
    OracleQueryMetadata* queries;

    /// number of elements used in queries
    uint32_t oracleQueryCount;

    // buffer continuously filled with variable-size query data (similar to tx storage)
    uint8_t* queryStorage;

    // how many bytes of queryStorage array are already in use / offset for adding new data
    uint64_t queryStorageBytesUsed;

    // state for assigning contract query IDs
    struct
    {
        uint32_t tick;
        uint32_t queryIndexInTick;
    } contractQueryIdState;

    // data type of state of received OM reply and computor commits for single oracle query (used before reveal)
    typedef OracleReplyState<ownComputorSeedsCount> ReplyState;

    // state of received OM reply and computor commits for each oracle query (used before reveal)
    ReplyState* replyStates;

    // index in replyStates to check next for empty slot (cyclic buffer)
    int32_t replyStatesIndex;

    /// fast lookup of query indices for which oracles are in pending state
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingQueryIndices;

    /// fast lookup of reply state indices for which commit tx is pending
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingCommitReplyStateIndices;

    /// fast lookup of reply state indices for which reveal tx is pending
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingRevealReplyStateIndices;

    // fast lookup of query indices for which the contract should be notified
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> notificationQueryIndicies;

    OracleEngineStatistics stats;

#if ENABLE_ORACLE_STATS_RECORD
    struct
    {
        unsigned long long queryCount;
        unsigned long long replyCount;
        unsigned long long validCount;

        // Extra data buffer
        unsigned long long extraData[4];

    } oracleStats[OI::oracleInterfacesCount]; // stats per oracle. 
#endif

    /// fast lookup of oracle query index (sequential position in queries array) from oracle query ID (composed of query tick and other info)
    QPI::HashMap<int64_t, uint32_t, MAX_ORACLE_QUERIES>* queryIdToIndex;

    /// array of ownComputorSeedsCount public keys (mainly for testing, in EFI core this points to computorPublicKeys from special_entities.h)
    const m256i* ownComputorPublicKeys;

    /// buffer used to store output of getNotification()
    OracleNotificationData notificationOutputBuffer;

    /// buffer used by enqueueOracleQuery()
    uint8_t enqueueOracleQueryBuffer[sizeof(OracleMachineQuery) + MAX_ORACLE_QUERY_SIZE];

    /// lock for preventing race conditions in concurrent execution
    mutable volatile char lock;

    /// Return empty reply state slot or max uint32 value on error
    uint32_t getEmptyReplyStateSlot()
    {
        ASSERT(replyStatesIndex < MAX_SIMULTANEOUS_ORACLE_QUERIES);
        for (uint32_t i = 0; i < MAX_SIMULTANEOUS_ORACLE_QUERIES; ++i)
        {
            if (replyStates[replyStatesIndex].queryId <= 0)
                return replyStatesIndex;

            ++replyStatesIndex;
            if (replyStatesIndex >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
                replyStatesIndex = 0;
        }
        return 0xffffffff;
    }

    void freeReplyStateSlot(uint32_t replyStateIdx)
    {
        ASSERT(replyStatesIndex < MAX_SIMULTANEOUS_ORACLE_QUERIES);
        setMem(&replyStates[replyStateIdx], sizeof(*replyStates), 0);
    }

    uint32_t findFirstQueryIndexOfTick(uint32_t tick) const
    {
        // precondition: queries is sorted by tick
#if !defined(NDEBUG)
        for (uint32_t t = 1; t < oracleQueryCount; ++t)
            ASSERT(queries[t].queryTick >= queries[t - 1].queryTick);
#endif

        if (!oracleQueryCount || tick < queries[0].queryTick || tick > queries[oracleQueryCount - 1].queryTick)
            return UINT32_MAX;

        uint32_t lower = 0;
        uint32_t upper = oracleQueryCount - 1;

        // invariants:
        // 1. lower <= upper
        // 2. queries[lower].queryTick <= tick
        // 2. tick <= queries[upper].queryTick
        while (upper - lower > 1)
        {
            uint32_t mid = (lower + upper) / 2;
            if (queries[mid].queryTick < tick)
                lower = mid;
            else
                upper = mid;
        }

        if (queries[lower].queryTick == tick)
            return lower;
        else if (queries[upper].queryTick == tick)
            return upper;
        else
            return UINT32_MAX;
    }

public:
    /// Initialize object, passing array of own computor public keys (with number of elements given by template param ownComputorSeedsCount).
    bool init(const m256i* ownComputorPublicKeys)
    {
        this->ownComputorPublicKeys = ownComputorPublicKeys;
        lock = 0;

        // alloc arrays and set to 0
        if (!allocPoolWithErrorLog(L"OracleEngine::queries", MAX_ORACLE_QUERIES * sizeof(*queries), (void**)&queries, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::queryStorage", ORACLE_QUERY_STORAGE_SIZE, (void**)&queryStorage, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::replyStates", MAX_SIMULTANEOUS_ORACLE_QUERIES * sizeof(*replyStates), (void**)&replyStates, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::queryIdToIndex", sizeof(*queryIdToIndex), (void**)&queryIdToIndex, __LINE__))
        {
            return false;
        }

        reset();

        return true;
    }

    /// Delete all queries, reply states, statistics etc.
    void reset()
    {
        ASSERT(queries && queryStorage && replyStates && queryIdToIndex);
        if (oracleQueryCount || queryStorageBytesUsed > 8 || queryIdToIndex->population())
        {
            setMem(queries, MAX_ORACLE_QUERIES * sizeof(*queries), 0);
            setMem(queryStorage, ORACLE_QUERY_STORAGE_SIZE, 0);
            setMem(replyStates, MAX_SIMULTANEOUS_ORACLE_QUERIES * sizeof(*replyStates), 0);
            queryIdToIndex->reset();
        }

        oracleQueryCount = 0;
        queryStorageBytesUsed = 8; // reserve offset 0 for "no data"
        setMem(&contractQueryIdState, sizeof(contractQueryIdState), 0);
        replyStatesIndex = 0;
        pendingQueryIndices.numValues = 0;
        pendingCommitReplyStateIndices.numValues = 0;
        pendingRevealReplyStateIndices.numValues = 0;
        notificationQueryIndicies.numValues = 0;
        setMem(&stats, sizeof(stats), 0);

#if ENABLE_ORACLE_STATS_RECORD
        setMem(&oracleStats, sizeof(oracleStats), 0);
#endif
    }

    void deinit()
    {
        // free arrays
        if (queries)
            freePool(queries);
        if (queryStorage)
            freePool(queryStorage);
        if (replyStates)
            freePool(replyStates);
    }

    /// Save current state to snapshot files. Can only be called from main processor!
    bool saveSnapshot(unsigned short epoch, CHAR16* directory) const;

    /// Load state from snapshot files. Can only be called from main processor!
    bool loadSnapshot(unsigned short epoch, CHAR16* directory);

    /**
    * Check and start user query based on transaction (should be called from tick processor).
    * @param tx Transaction, whose validity and signature has been checked before.
    * @param txIndex Index of tx in tick data, for referencing in tick storage.
    * @return Query ID or zero on error.
    */
    int64_t startUserQuery(const OracleUserQueryTransactionPrefix* tx, uint32_t txIndex)
    {
        // ASSERT that tx is in tick storage at tx->tick, txIndex.
        // check interface index
        // check size of payload vs expected query of given interface

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // add to query storage
        // send query to oracle machine node
    }

    int64_t startContractQuery(uint16_t contractIndex, uint32_t interfaceIndex,
        const void* queryData, uint16_t querySize, uint32_t timeoutMillisec,
        unsigned int notificationProcId)
    {
        // check inputs
        if (contractIndex >= MAX_NUMBER_OF_CONTRACTS || interfaceIndex >= OI::oracleInterfacesCount || querySize != OI::oracleInterfaces[interfaceIndex].querySize)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start contract oracle query due to contractIndex / oracleInterface issue!");
#endif
            return -1;
        }

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // check that still have free capacity for the query
        if (oracleQueryCount >= MAX_ORACLE_QUERIES || pendingQueryIndices.numValues >= MAX_SIMULTANEOUS_ORACLE_QUERIES || queryStorageBytesUsed + querySize > ORACLE_QUERY_STORAGE_SIZE)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start contract oracle query due to lack of space!");
#endif
            return -1;
        }

        // find slot storing temporary reply state
        uint32_t replyStateSlotIdx = getEmptyReplyStateSlot();
        if (replyStateSlotIdx >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start contract oracle query due to lack of free reply slot!");
#endif
            return -1;
        }

        // compute timeout as absolute point in time
        auto timeout = QPI::DateAndTime::now();
        if (!timeout.addMillisec(timeoutMillisec))
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start contract oracle query due to timeout timestamp issue!");
#endif
            return -1;
        }

        // get sequential query index of contract in tick
        auto& cs = contractQueryIdState;
        if (cs.tick < system.tick)
        {
            cs.tick = system.tick;
            cs.queryIndexInTick = NUMBER_OF_TRANSACTIONS_PER_TICK;
        }
        else
        {
            if (cs.queryIndexInTick >= 0x7FFFFFFF)
            {
#if !defined(NDEBUG) && !defined(NO_UEFI)
                addDebugMessage(L"Cannot start contract oracle query due to queryId issue!");
#endif
                return -1;
            }
            ++cs.queryIndexInTick;
        }

        // compose query ID
        int64_t queryId = ((int64_t)system.tick << 31) | cs.queryIndexInTick;
        static_assert(((0xFFFFFFFFll << 31) & 0x7FFFFFFFll) == 0ll && ((0xFFFFFFFFll << 31) | 0x7FFFFFFFll) > 0);

        // map ID to index
        ASSERT(!queryIdToIndex->contains(queryId));
        if (queryIdToIndex->set(queryId, oracleQueryCount) == QPI::NULL_INDEX)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start contract oracle query due to queryIdToIndex issue!");
#endif
            return -1;
        }

        // register index of pending query
        pendingQueryIndices.add(oracleQueryCount);

        // init query metadata (persistent)
        auto& queryMetadata = queries[oracleQueryCount++];
        queryMetadata.queryId = queryId;
        queryMetadata.type = ORACLE_QUERY_TYPE_CONTRACT_QUERY;
        queryMetadata.status = ORACLE_QUERY_STATUS_PENDING;
        queryMetadata.statusFlags = ORACLE_FLAG_REPLY_PENDING;
        queryMetadata.interfaceIndex = interfaceIndex;
        queryMetadata.queryTick = system.tick;
        queryMetadata.timeout = timeout;
        queryMetadata.typeVar.contract.queryingContract = contractIndex;
        queryMetadata.typeVar.contract.queryStorageOffset = queryStorageBytesUsed;
        queryMetadata.typeVar.contract.notificationProcId = notificationProcId;
        queryMetadata.statusVar.pending.replyStateIndex = replyStateSlotIdx;

        // init reply state (temporary until reply is revealed)
        ReplyState& replyState = replyStates[replyStateSlotIdx];
        setMem(&replyState, sizeof(replyState), 0);
        replyState.queryId = queryId;

        // copy oracle query data to permanent storage
        copyMem(queryStorage + queryStorageBytesUsed, queryData, querySize);
        queryStorageBytesUsed += querySize;

        // enqueue query message to oracle machine node
        enqueueOracleQuery(queryId, interfaceIndex, timeoutMillisec, queryData, querySize);

        // log status change
        OracleQueryStatusChange logEvent{ m256i(contractIndex, 0, 0, 0), queryId, interfaceIndex, queryMetadata.type, queryMetadata.status };
        logger.logOracleQueryStatusChange(logEvent);

        // Debug logging
#if ENABLE_ORACLE_STATS_RECORD
        oracleStats[queryMetadata.interfaceIndex].queryCount++;
#endif

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsg[200];
        setText(dbgMsg, L"oracleEngine.startContractQuery(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, ", queryId ");
        appendNumber(dbgMsg, queryId, FALSE);
        appendText(dbgMsg, ", interfaceIndex ");
        appendNumber(dbgMsg, interfaceIndex, FALSE);
        appendText(dbgMsg, ", timeout ");
        appendDateAndTime(dbgMsg, timeout);
        appendText(dbgMsg, ", now ");
        appendDateAndTime(dbgMsg, QPI::DateAndTime::now());
        addDebugMessage(dbgMsg);
#endif

        return queryId;
    }

    // Refund fees if an error occurred while trying to start an oracle query
    static void refundFees(const m256i& sourcePublicKey, int64_t refundAmount)
    {
        ASSERT(refundAmount >= 0);
        increaseEnergy(sourcePublicKey, refundAmount);
        const QuTransfer quTransfer = { m256i::zero(), sourcePublicKey, refundAmount };
        logger.logQuTransfer(quTransfer);
    }

protected:
    // Enqueue oracle machine query message. Cannot be run concurrently. Caller must acquire engine lock!
    void enqueueOracleQuery(int64_t queryId, uint32_t interfaceIdx, uint32_t timeoutMillisec, const void* queryData, uint16_t querySize)
    {
        // Check input size and compute total payload size
        ASSERT(querySize <= MAX_ORACLE_QUERY_SIZE);
        const unsigned int payloadSize = sizeof(OracleMachineQuery) + querySize;

        // Prepare message payload
        OracleMachineQuery* omq = reinterpret_cast<OracleMachineQuery*>(enqueueOracleQueryBuffer);
        omq->oracleQueryId = queryId;
        omq->oracleInterfaceIndex = interfaceIdx;
        omq->timeoutInMilliseconds = timeoutMillisec;
        copyMem(omq + 1, queryData, querySize);

        // Enqueue for sending to all oracle machine peers (peer pointer address 0x1 is reserved for that)
        enqueueResponse((Peer*)0x1, payloadSize, OracleMachineQuery::type(), 0, omq);
    }

    void logQueryStatusChange(const OracleQueryMetadata& oqm) const
    {
        m256i queryingEntity = m256i::zero();
        if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY)
            queryingEntity.u64._0 = oqm.typeVar.contract.queryingContract;
        else if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION)
            queryingEntity.u64._0 = oqm.typeVar.subscription.subscriptionId;
        else if (oqm.type == ORACLE_QUERY_TYPE_USER_QUERY)
            queryingEntity = oqm.typeVar.user.queryingEntity;
        OracleQueryStatusChange logEvent{ queryingEntity, oqm.queryId, oqm.interfaceIndex, oqm.type, oqm.status };
        logger.logOracleQueryStatusChange(logEvent);
    }

public:
    // CAUTION: Called from request processor, requires locking!
    void processOracleMachineReply(const OracleMachineReply* replyMessage, uint32_t replyMessageSize)
    {
        // check input params
        ASSERT(replyMessage);
        if (replyMessageSize < sizeof(OracleMachineReply))
            return;

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // get query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(replyMessage->oracleQueryId, queryIndex) || queryIndex >= oracleQueryCount)
            return;

        // get query metadata
        OracleQueryMetadata& oqm = queries[queryIndex];
        if (oqm.status != ORACLE_QUERY_STATUS_PENDING)
            return;

        // check error flags
        uint16_t errorFlags = (replyMessage->oracleMachineErrorFlags & ORACLE_FLAG_OM_ERROR_FLAGS);
        if (errorFlags != 0)
        {
            oqm.statusFlags |= errorFlags;
            return;
        }

        // check reply size vs size expected by interface
        ASSERT(oqm.interfaceIndex < OI::oracleInterfacesCount);
        uint32_t replySize = replyMessageSize - sizeof(OracleMachineReply);
        if (replySize > MAX_ORACLE_REPLY_SIZE || replySize != OI::oracleInterfaces[oqm.interfaceIndex].replySize)
        {
            oqm.statusFlags |= ORACLE_FLAG_BAD_SIZE_REPLY;
            return;
        }

        // compute digest of reply data
        const void* replyData = replyMessage + 1;
        m256i replyDigest;
        KangarooTwelve(replyData, replySize, replyDigest.m256i_u8, 32);

        // get reply state
        const auto replyStateIdx = oqm.statusVar.pending.replyStateIndex;
        ASSERT(replyStateIdx < MAX_SIMULTANEOUS_ORACLE_QUERIES);
        ReplyState& replyState = replyStates[replyStateIdx];
        ASSERT(replyState.queryId == replyMessage->oracleQueryId);

        // return if we already got a reply
        if (replyState.ownReplySize)
        {
            // if the digests don't match, set an error flag
            if (replyDigest != replyState.ownReplyDigest)
            {
                oqm.statusFlags |= ORACLE_FLAG_OM_DISAGREE;
                ++stats.oracleMachineRepliesDisagreeCount;
            }
            return;
        }

        // copy reply from message to state struct and set status flag
        copyMem(replyState.ownReplyData, replyData, replySize);
        replyState.ownReplyDigest = replyDigest;
        replyState.ownReplySize = replySize;
        oqm.statusFlags |= ORACLE_FLAG_REPLY_RECEIVED;

        // update statistics
        ++stats.oracleMachineReplyCount;
        stats.oracleMachineReplyTicksSum += (system.tick - oqm.queryTick);

        // add reply state to set of indices with pending commit tx
        pendingCommitReplyStateIndices.add(replyStateIdx);

#if ENABLE_ORACLE_STATS_RECORD
        // Update the stats for each type of oracles
        const uint32_t ifaceIdx = oqm.interfaceIndex;
        oracleStats[ifaceIdx].replyCount++;
        // Now only record contract query
        if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY)
        {
            const void* queryData = queryStorage + oqm.typeVar.contract.queryStorageOffset;

            bool valid = false;
            switch (ifaceIdx)
            {
            case 0:  // Price
                valid = OI::Price::replyIsValid(*(const OI::Price::OracleReply*)replyData);
                oracleStats[ifaceIdx].extraData[0] = ((const OI::Price::OracleReply*)replyData)->denominator;
                oracleStats[ifaceIdx].extraData[1] = ((const OI::Price::OracleReply*)replyData)->numerator;
                break;
            case 1:  // Mock
                valid = OI::Mock::replyIsValid(
                    *(const OI::Mock::OracleQuery*)queryData,
                    *(const OI::Mock::OracleReply*)replyData);
                // Get some extra data
                oracleStats[ifaceIdx].extraData[0] = ((const OI::Mock::OracleQuery*)queryData)->value;
                oracleStats[ifaceIdx].extraData[1] = ((const OI::Mock::OracleReply*)replyData)->echoedValue;
                oracleStats[ifaceIdx].extraData[2] = ((const OI::Mock::OracleReply*)replyData)->doubledValue;
                break;
            default: // unknown
                break;
            }
            if (valid)
            {
                oracleStats[ifaceIdx].validCount++;
            }
        }
#endif

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsg[200];
        setText(dbgMsg, L"oracleEngine.processOracleMachineReply(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, ", queryId ");
        appendNumber(dbgMsg, oqm.queryId, FALSE);
        appendText(dbgMsg, ", interfaceIndex ");
        appendNumber(dbgMsg, oqm.interfaceIndex, FALSE);
        addDebugMessage(dbgMsg);
#endif
    }

    /**
    * Prepare OracleReplyCommitTransaction in txBuffer, setting all except signature.
    * 
    * @param txBuffer Buffer for constructing the transaction. Size must be at least MAX_TRANSACTION_SIZE bytes.
    * @param computorIdx Index of computor list of computors broadcasted by arbitrator.
    * @param ownComputorIdx Index of computor in local array computorSeeds.
    * @param txScheduleTick Tick, in which the transaction is supposed to be scheduled.
    * @param startIdx Index returned by the previous call of this function if more than one tx is required.
    * @return 0 if no tx needs to be sent; UINT32_MAX if all pending commits are included in the created tx;
    *         any value in between indicates that another tx needs to be created and should be passed as the start
    *         index for the next call of this function
    *
    * Called from tick processor.
    */
    uint32_t getReplyCommitTransaction(
        void* txBuffer, uint16_t computorIdx, uint16_t ownComputorIdx,
        uint32_t txScheduleTick, uint32_t startIdx = 0)
    {
        // check inputs
        ASSERT(txBuffer);
        if (ownComputorIdx >= ownComputorSeedsCount || computorIdx >= NUMBER_OF_COMPUTORS || txScheduleTick <= system.tick)
            return 0;

        // init data pointers and reply commit counter
        auto* tx = reinterpret_cast<OracleReplyCommitTransactionPrefix*>(txBuffer);
        auto* commits = reinterpret_cast<OracleReplyCommitTransactionItem*>(tx->inputPtr());
        uint16_t commitsCount = 0;
        constexpr uint16_t maxCommitsCount = MAX_INPUT_SIZE / sizeof(OracleReplyCommitTransactionItem);

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // consider queries with pending commit tx, specifically the reply data indices of those
        const unsigned int replyIdxCount = pendingCommitReplyStateIndices.numValues;
        const unsigned int* replyIndices = pendingCommitReplyStateIndices.values;
        unsigned int idx = startIdx;
        for (; idx < replyIdxCount; ++idx)
        {
            // get reply state and check that oracle reply has been received
            const unsigned int replyIdx = replyIndices[idx];
            if (replyIdx >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
                continue;
            ReplyState& replyState = replyStates[replyIdx];
            if (replyState.queryId <= 0 || replyState.ownReplySize == 0)
                continue;

            // tx already executed or scheduled?
            if (replyState.ownReplyCommitComputorTxExecuted[ownComputorIdx] ||
                replyState.ownReplyCommitComputorTxTick[ownComputorIdx] >= system.tick) // TODO: > or >= ?
                continue;

            // additional commit required -> leave loop early to finish tx
            if (commitsCount == maxCommitsCount)
                break;

            // set known data of commit tx part
            commits[commitsCount].queryId = replyState.queryId;
            commits[commitsCount].replyDigest = replyState.ownReplyDigest;

            // compute knowledge proof of commit = K12(oracle reply + computor index)
            ASSERT(replyState.ownReplySize <= MAX_ORACLE_REPLY_SIZE);
            *(uint16_t*)(replyState.ownReplyData + replyState.ownReplySize) = computorIdx;
            KangarooTwelve(replyState.ownReplyData, replyState.ownReplySize + 2, &commits[commitsCount].replyKnowledgeProof, 32);

            // signal to schedule tx for given tick
            replyState.ownReplyCommitComputorTxTick[ownComputorIdx] = txScheduleTick;

            // we have completed adding this commit
            ++commitsCount;
        }

        // no reply commits needed? -> signal to skip tx
        if (!commitsCount)
            return 0;

        // finish all of tx except for signature
        tx->sourcePublicKey = ownComputorPublicKeys[ownComputorIdx];
        tx->destinationPublicKey = m256i::zero();
        tx->amount = 0;
        tx->tick = txScheduleTick;
        tx->inputType = OracleReplyCommitTransactionPrefix::transactionType();
        tx->inputSize = commitsCount * sizeof(OracleReplyCommitTransactionItem);

#if !defined(NDEBUG) && !defined(NO_UEFI) && 0
        CHAR16 dbgMsg[200];
        setText(dbgMsg, L"oracleEngine.getReplyCommitTransaction(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, ", txScheduleTick ");
        appendNumber(dbgMsg, txScheduleTick, FALSE);
        appendText(dbgMsg, ", computorIdx ");
        appendNumber(dbgMsg, computorIdx, FALSE);
        appendText(dbgMsg, ", commitsCount ");
        appendNumber(dbgMsg, commitsCount, FALSE);
        appendText(dbgMsg, ", queryId");
        for (int i = 0; i < commitsCount; ++i)
        {
            appendText(dbgMsg, " ");
            appendNumber(dbgMsg, commits[i].queryId, FALSE);
        }
        addDebugMessage(dbgMsg);
#endif

        // if we had to break from the loop early, return and signal to call this again for creating another
        // tx with the start index we return here
        if (idx < replyIdxCount)
            return idx;

        // signal that the tx is ready and the function doesn't need to be called again for more commits
        return UINT32_MAX;
    }

    // Called from tick processor.
    bool processOracleReplyCommitTransaction(const OracleReplyCommitTransactionPrefix* transaction)
    {
        // check precondition for calling with ASSERTs
        ASSERT(transaction != nullptr);
        ASSERT(transaction->checkValidity());
        ASSERT(isZero(transaction->destinationPublicKey));
        ASSERT(transaction->inputType == OracleReplyCommitTransactionPrefix::transactionType());

        // check size of tx
        if (transaction->inputSize < OracleReplyCommitTransactionPrefix::minInputSize())
            return false;

        // get computor index
        const int compIdx = computorIndex(transaction->sourcePublicKey);
        if (compIdx < 0)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            CHAR16 dbgMsg[120];
            setText(dbgMsg, L"oracleEngine.processOracleReplyCommitTransaction(), tick ");
            appendNumber(dbgMsg, system.tick, FALSE);
            appendText(dbgMsg, ", source is no computor!");
            addDebugMessage(dbgMsg);
#endif
            return false;
        }

#if !defined(NDEBUG) && !defined(NO_UEFI) && 0
        CHAR16 dbgMsg[800];
        setText(dbgMsg, L"oracleEngine.processOracleReplyCommitTransaction(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, ", computorIdx ");
        appendNumber(dbgMsg, compIdx, FALSE);
        appendText(dbgMsg, ", queryId ");
#endif

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // process the N commits in this tx
        const OracleReplyCommitTransactionItem* item = (const OracleReplyCommitTransactionItem*)transaction->inputPtr();
        uint32_t size = sizeof(OracleReplyCommitTransactionItem);
        for (; size <= transaction->inputSize; size += sizeof(OracleReplyCommitTransactionItem), ++item)
        {
            // get and check query index
            uint32_t queryIndex;
            if (!queryIdToIndex->get(item->queryId, queryIndex) || queryIndex >= oracleQueryCount)
                continue;

            // get query metadata and check state
            OracleQueryMetadata& oqm = queries[queryIndex];
            if (oqm.status != ORACLE_QUERY_STATUS_PENDING && oqm.status != ORACLE_QUERY_STATUS_COMMITTED)
                continue;

            // get reply state
            const auto replyStateIdx = oqm.statusVar.pending.replyStateIndex;
            ASSERT(replyStateIdx < MAX_SIMULTANEOUS_ORACLE_QUERIES);
            ReplyState& replyState = replyStates[replyStateIdx];
            ASSERT(replyState.queryId == item->queryId);

            // ignore commit if we already have processed a commit by this computor
            if (replyState.replyCommitTicks[compIdx] != 0)
                continue;

            // save reply commit of computor
            replyState.replyCommitDigests[compIdx] = item->replyDigest;
            replyState.replyCommitKnowledgeProofs[compIdx] = item->replyKnowledgeProof;
            replyState.replyCommitTicks[compIdx] = transaction->tick;

            // if tx is from own computor, prevent rescheduling of commit tx
            for (auto i = 0ull; replyState.ownReplyCommitExecCount < ownComputorSeedsCount && i < ownComputorSeedsCount; ++i)
            {
                if (!replyState.ownReplyCommitComputorTxExecuted[i] && ownComputorPublicKeys[i] == transaction->sourcePublicKey)
                {
                    replyState.ownReplyCommitComputorTxExecuted[i] = transaction->tick;
                    ++replyState.ownReplyCommitExecCount;
                    break;
                }
            }

            // update reply commit histogram
            // 1. search existing or free slot of digest in histogram array
            uint16_t histIdx = 0;
            while (replyState.replyCommitHistogramCount[histIdx] != 0 &&
                item->replyDigest != replyState.replyCommitDigests[replyState.replyCommitHistogramIdx[histIdx]])
            {
                ASSERT(histIdx < NUMBER_OF_COMPUTORS);
                ++histIdx;
            }
            // 2. update slot
            if (replyState.replyCommitHistogramCount[histIdx] == 0)
            {
                // first time we see this commit digest
                replyState.replyCommitHistogramIdx[histIdx] = compIdx;
            }
            ++replyState.replyCommitHistogramCount[histIdx];
            // 3. update variables that trigger reveal
            ++replyState.totalCommits;
            if (replyState.replyCommitHistogramCount[histIdx] > replyState.replyCommitHistogramCount[replyState.mostCommitsHistIdx])
                replyState.mostCommitsHistIdx = histIdx;

            // check if there are enough computor commits for decision
            const auto mostCommitsCount = replyState.replyCommitHistogramCount[replyState.mostCommitsHistIdx];
            if (mostCommitsCount >= QUORUM)
            {
                // enough commits for the reply reveal transaction
                if (oqm.status != ORACLE_QUERY_STATUS_COMMITTED)
                {
                    // -> switch to status COMMITTED
                    oqm.status = ORACLE_QUERY_STATUS_COMMITTED;
                    pendingCommitReplyStateIndices.removeByValue(replyStateIdx);
                    pendingRevealReplyStateIndices.add(replyStateIdx);
                    ++stats.commitCount;
                    stats.commitTicksSum += (system.tick - oqm.queryTick);

                    // log status change
                    logQueryStatusChange(oqm);

#if !defined(NDEBUG) && !defined(NO_UEFI) && 1
                    CHAR16 dbgMsg1[200];
                    setText(dbgMsg1, L"oracleEngine.processOracleReplyCommitTransaction(), tick ");
                    appendNumber(dbgMsg1, system.tick, FALSE);
                    appendText(dbgMsg1, ", queryId ");
                    appendNumber(dbgMsg1, oqm.queryId, FALSE);
                    appendText(dbgMsg1, " (");
                    appendNumber(dbgMsg1, mostCommitsCount, FALSE);
                    appendText(dbgMsg1, ":");
                    appendNumber(dbgMsg1, replyState.totalCommits - mostCommitsCount, FALSE);
                    appendText(dbgMsg1, ") -> COMMITTED");
                    addDebugMessage(dbgMsg1);
#endif
                }
            }
            else if (replyState.totalCommits - mostCommitsCount > NUMBER_OF_COMPUTORS - QUORUM)
            {
                // more than 1/3 of commits don't vote for most voted digest -> getting quorum isn't possible
                // -> switch to status UNRESOLVABLE
                oqm.status = ORACLE_QUERY_STATUS_UNRESOLVABLE;
                oqm.statusVar.failure.agreeingCommits = mostCommitsCount;
                oqm.statusVar.failure.totalCommits = replyState.totalCommits;
                pendingQueryIndices.removeByValue(queryIndex);
                ++stats.unresolvableCount;

                // cleanup data of pending reply immediately (no info for revenue required)
                pendingCommitReplyStateIndices.removeByValue(replyStateIdx);
                freeReplyStateSlot(replyStateIdx);

                // schedule contract notification(s) if needed
                if (oqm.type != ORACLE_QUERY_TYPE_USER_QUERY)
                    notificationQueryIndicies.add(queryIndex);

                // log status change
                logQueryStatusChange(oqm);

#if !defined(NDEBUG) && !defined(NO_UEFI) && 1
                CHAR16 dbgMsg1[200];
                setText(dbgMsg1, L"oracleEngine.processOracleReplyCommitTransaction(), tick ");
                appendNumber(dbgMsg1, system.tick, FALSE);
                appendText(dbgMsg1, ", queryId ");
                appendNumber(dbgMsg1, oqm.queryId, FALSE);
                appendText(dbgMsg1, " (");
                appendNumber(dbgMsg1, mostCommitsCount, FALSE);
                appendText(dbgMsg1, ":");
                appendNumber(dbgMsg1, replyState.totalCommits - mostCommitsCount, FALSE);
                appendText(dbgMsg1, ") -> UNRESOLVABLE");
                addDebugMessage(dbgMsg1);
#endif
            }

#if !defined(NDEBUG) && !defined(NO_UEFI) && 0
            appendNumber(dbgMsg, item->queryId, FALSE);
            appendText(dbgMsg, " (");
            appendNumber(dbgMsg, mostCommitsCount, FALSE);
            appendText(dbgMsg, ":");
            appendNumber(dbgMsg, replyState.totalCommits - mostCommitsCount, FALSE);
            appendText(dbgMsg, ")");
#endif
        }

#if !defined(NDEBUG) && !defined(NO_UEFI) && 0
        addDebugMessage(dbgMsg);
#endif

        return true;
    }

    /**
    * Prepare OracleReplyRevealTransaction in txBuffer, setting all except signature.
    *
    * @param txBuffer Buffer for constructing the transaction. Size must be at least MAX_TRANSACTION_SIZE bytes.
    * @param ownComputorIdx Index of computor in local array computorSeeds.
    * @param txScheduleTick Tick, in which the transaction is supposed to be scheduled.
    * @param startIdx Index returned by the previous call of this function if more than one tx is required.
    * @return 0 if no tx needs to be sent; any other value indicates that another tx needs to be created and it
    *         should be passed as the start index for the next call of this function
    *
    * Called from tick processor.
    */
    uint32_t getReplyRevealTransaction(void* txBuffer, uint16_t ownComputorIdx, uint32_t txScheduleTick, uint32_t startIdx = 0)
    {
        // check inputs
        ASSERT(txBuffer);
        if (ownComputorIdx >= ownComputorSeedsCount || txScheduleTick <= system.tick)
            return 0;

        // init data pointer
        auto* tx = reinterpret_cast<OracleReplyRevealTransactionPrefix*>(txBuffer);
        void* txReplyData = tx + 1;

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // consider queries with pending reveal tx, specifically the reply data indices of those
        const unsigned int replyIdxCount = pendingRevealReplyStateIndices.numValues;
        const unsigned int* replyIndices = pendingRevealReplyStateIndices.values;
        unsigned int idx = startIdx;
        for (; idx < replyIdxCount; ++idx)
        {
            // get reply state and check that oracle reply has been received and a quorum has formed about the value
            const unsigned int replyIdx = replyIndices[idx];
            if (replyIdx >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
                continue;
            ReplyState& replyState = replyStates[replyIdx];
            if (replyState.queryId <= 0 || replyState.ownReplySize == 0)
                continue;
            const uint16_t mostCommitsCount = replyState.replyCommitHistogramCount[replyState.mostCommitsHistIdx];
            ASSERT(replyState.mostCommitsHistIdx < NUMBER_OF_COMPUTORS && mostCommitsCount <= NUMBER_OF_COMPUTORS);
            if (mostCommitsCount < QUORUM)
                continue;

            // tx already scheduled or seen?
            if (replyState.expectedRevealTxTick >= system.tick) // TODO: > or >= ?
                continue;

            // check if local view is the quorum view
            const m256i quorumCommitDigest = replyState.replyCommitDigests[replyState.replyCommitHistogramIdx[replyState.mostCommitsHistIdx]];
            if (quorumCommitDigest != replyState.ownReplyDigest)
                continue;

            // set all of tx except for signature
            tx->sourcePublicKey = ownComputorPublicKeys[ownComputorIdx];
            tx->destinationPublicKey = m256i::zero();
            tx->amount = 0;
            tx->tick = txScheduleTick;
            tx->inputType = OracleReplyRevealTransactionPrefix::transactionType();
            tx->inputSize = sizeof(tx->queryId) + replyState.ownReplySize;
            tx->queryId = replyState.queryId;
            copyMem(txReplyData, replyState.ownReplyData, replyState.ownReplySize);

            // remember that we have scheduled reveal of this reply
            replyState.expectedRevealTxTick = txScheduleTick;

#if !defined(NDEBUG) && !defined(NO_UEFI)
            CHAR16 dbgMsg[200];
            setText(dbgMsg, L"oracleEngine.getReplyRevealTransaction(), tick ");
            appendNumber(dbgMsg, system.tick, FALSE);
            appendText(dbgMsg, ", txScheduleTick ");
            appendNumber(dbgMsg, txScheduleTick, FALSE);
            appendText(dbgMsg, ", queryId ");
            appendNumber(dbgMsg, replyState.queryId, FALSE);
            appendText(dbgMsg, ", computorIdx ");
            appendNumber(dbgMsg, computorIndex(tx->sourcePublicKey), FALSE);
            addDebugMessage(dbgMsg);
#endif

            // return non-zero in order instruct caller to call this function again with the returned startIdx
            return idx + 1;
        }

        // currently no reply reveal needed -> signal to skip tx
        return 0;
    }

protected:
    // Check oracle reply reveal transaction. Returns reply state if okay or NULL otherwise. Also sets output param queryIndexOutput.
    // Caller is responsible for locking.
    ReplyState* checkReplyRevealTransaction(const OracleReplyRevealTransactionPrefix* transaction, uint32_t* queryIndexOutput = nullptr) const
    {
        // check precondition for calling with ASSERTs
        ASSERT(transaction != nullptr);
        ASSERT(transaction->checkValidity());
        ASSERT(transaction->inputType == OracleReplyRevealTransactionPrefix::transactionType());
        ASSERT(isZero(transaction->destinationPublicKey));

        // check size of tx
        if (transaction->inputSize < OracleReplyRevealTransactionPrefix::minInputSize())
            return nullptr;

        // check that tx source is computor
        if (computorIndex(transaction->sourcePublicKey) < 0)
            return nullptr;

        // get and check query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(transaction->queryId, queryIndex) || queryIndex >= oracleQueryCount)
            return nullptr;

        // get query metadata and check state
        OracleQueryMetadata& oqm = queries[queryIndex];
        if (oqm.status != ORACLE_QUERY_STATUS_COMMITTED)
            return nullptr;

        // check reply size vs size expected by interface
        ASSERT(oqm.interfaceIndex < OI::oracleInterfacesCount);
        const uint16_t replySize = transaction->inputSize - sizeof(transaction->queryId);
        if (replySize != OI::oracleInterfaces[oqm.interfaceIndex].replySize)
        {
            oqm.statusFlags |= ORACLE_FLAG_BAD_SIZE_REVEAL;
            return nullptr;
        }

        // get reply state
        const auto replyStateIdx = oqm.statusVar.pending.replyStateIndex;
        ASSERT(replyStateIdx < MAX_SIMULTANEOUS_ORACLE_QUERIES);
        ReplyState& replyState = replyStates[replyStateIdx];
        ASSERT(replyState.queryId == transaction->queryId);

        // compute digest of reply in reveal tx
        const void* replyData = transaction + 1;
        m256i revealDigest;
        KangarooTwelve(replyData, replySize, revealDigest.m256i_u8, 32);

        // check that revealed reply matches the quorum digest
        const m256i quorumCommitDigest = replyState.replyCommitDigests[replyState.replyCommitHistogramIdx[replyState.mostCommitsHistIdx]];
        ASSERT(!isZero(quorumCommitDigest));
        if (revealDigest != quorumCommitDigest)
            return nullptr;

        // set output param
        if (queryIndexOutput)
            *queryIndexOutput = queryIndex;

        return &replyState;
    }

    // Caller is responsible for locking.
    const void* getReplyDataFromTickTransactionStorage(const OracleQueryMetadata& queryMetadata) const
    {
        const uint32_t tick = queryMetadata.statusVar.success.revealTick;
        const uint32_t txSlotInTickData = queryMetadata.statusVar.success.revealTxIndex;
        ASSERT(txSlotInTickData < NUMBER_OF_TRANSACTIONS_PER_TICK);
        ASSERT(ts.tickInCurrentEpochStorage(tick));
        const unsigned long long* tsTickTransactionOffsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(tick);
        const auto* transaction = (OracleReplyRevealTransactionPrefix*)ts.tickTransactions.ptr(tsTickTransactionOffsets[txSlotInTickData]);
        ASSERT(queryMetadata.queryId == transaction->queryId);
        ASSERT(queryMetadata.interfaceIndex < OI::oracleInterfacesCount);
        ASSERT(transaction->inputSize - sizeof(transaction->queryId) == OI::oracleInterfaces[queryMetadata.interfaceIndex].replySize);
        return transaction + 1;
    }

public:
    // Called by request processor when a tx is received in order to minimize sending of reveal tx.
    void announceExpectedRevealTransaction(const OracleReplyRevealTransactionPrefix* transaction)
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // check tx and get reply state
        ReplyState* replyState = checkReplyRevealTransaction(transaction);
        if (!replyState)
            return;

        // update tick when reveal is expected
        if (!replyState->expectedRevealTxTick || replyState->expectedRevealTxTick > transaction->tick)
            replyState->expectedRevealTxTick = transaction->tick;

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsg[200];
        setText(dbgMsg, L"oracleEngine.announceExpectedRevealTransaction(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, ", queryId ");
        appendNumber(dbgMsg, replyState->queryId, FALSE);
        appendText(dbgMsg, ", computorIdx ");
        appendNumber(dbgMsg, computorIndex(transaction->sourcePublicKey), FALSE);
        appendText(dbgMsg, ", expectedRevealTxTick ");
        appendNumber(dbgMsg, replyState->expectedRevealTxTick, FALSE);
        addDebugMessage(dbgMsg);
#endif
    }

    // Called from tick processor.
    bool processOracleReplyRevealTransaction(const OracleReplyRevealTransactionPrefix* transaction, uint32_t txSlotInTickData)
    {
        ASSERT(txSlotInTickData < NUMBER_OF_TRANSACTIONS_PER_TICK);
        ASSERT(transaction->tick == system.tick);
        {
            // check that tick storage contains tx at expected position
            ASSERT(ts.tickInCurrentEpochStorage(transaction->tick));
            const uint64_t* tsTickTransactionOffsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(transaction->tick);
            const auto* tsTx = ts.tickTransactions.ptr(tsTickTransactionOffsets[txSlotInTickData]);
            ASSERT(compareMem(transaction, tsTx, transaction->totalSize()) == 0);
        }

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // track how many reveal tx are processed in total for later optimizing code to achieve minimal overall
        // oracle reply latency with low number of reveal tx
        ++stats.revealTxCount;

        // check tx and get reply state + query metadata
        uint32_t queryIndex;
        ReplyState* replyState = checkReplyRevealTransaction(transaction, &queryIndex);
        if (!replyState)
            return false;
        OracleQueryMetadata& oqm = queries[queryIndex];
        const auto replyStateIdx = oqm.statusVar.pending.replyStateIndex;

        // TODO: check knowledge proofs of all computors and add revenue points for computors who sent correct commit tx fastest

        // update state to SUCCESS
        oqm.statusVar.success.revealTick = transaction->tick;
        oqm.statusVar.success.revealTxIndex = txSlotInTickData;
        oqm.status = ORACLE_QUERY_STATUS_SUCCESS;
        pendingQueryIndices.removeByValue(queryIndex);
        ++stats.successCount;
        stats.successTicksSum += (oqm.statusVar.success.revealTick - oqm.queryTick);

        // cleanup reply state
        pendingRevealReplyStateIndices.removeByValue(replyStateIdx);
        freeReplyStateSlot(replyStateIdx);

        // schedule contract notification(s) if needed
        if (oqm.type != ORACLE_QUERY_TYPE_USER_QUERY)
            notificationQueryIndicies.add(queryIndex);

        // log status change
        logQueryStatusChange(oqm);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsg[200];
        setText(dbgMsg, L"oracleEngine.processOracleReplyRevealTransaction(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, ", queryId ");
        appendNumber(dbgMsg, oqm.queryId, FALSE);
        appendText(dbgMsg, ", computorIdx ");
        appendNumber(dbgMsg, computorIndex(transaction->sourcePublicKey), FALSE);
        addDebugMessage(dbgMsg);
#endif

        return true;
    }

    // Called once per tick from the tick processor.
    void processTimeouts()
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // consider pending queries
        const uint32_t queryIdxCount = pendingQueryIndices.numValues;
        const uint32_t* queryIndices = pendingQueryIndices.values;
        const QPI::DateAndTime now = QPI::DateAndTime::now();
        for (uint32_t i = 0; i < queryIdxCount; ++i)
        {
            // get query data
            const uint32_t queryIndex = queryIndices[i];
            ASSERT(queryIndex < oracleQueryCount);
            OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(oqm.status == ORACLE_QUERY_STATUS_PENDING || oqm.status == ORACLE_QUERY_STATUS_COMMITTED);

            // check for timeout
            if (oqm.timeout < now)
            {
                // get reply state
                const auto replyStateIdx = oqm.statusVar.pending.replyStateIndex;
                ASSERT(replyStateIdx < MAX_SIMULTANEOUS_ORACLE_QUERIES);
                ReplyState& replyState = replyStates[replyStateIdx];
                ASSERT(replyState.queryId == oqm.queryId);
                const uint16_t mostCommitsCount = replyState.replyCommitHistogramCount[replyState.mostCommitsHistIdx];
                ASSERT(replyState.mostCommitsHistIdx < NUMBER_OF_COMPUTORS && mostCommitsCount <= NUMBER_OF_COMPUTORS);

                // update statistics
                stats.timeoutTicksSum += (system.tick - oqm.queryTick);
                if (oqm.status == ORACLE_QUERY_STATUS_COMMITTED)
                    ++stats.timeoutNoRevealCount;
                else if (oqm.statusFlags & ORACLE_FLAG_REPLY_RECEIVED)
                    ++stats.timeoutNoCommitCount;
                else
                    ++stats.timeoutNoReplyCount;

                // update state to TIMEOUT
                oqm.status = ORACLE_QUERY_STATUS_TIMEOUT;
                oqm.statusVar.failure.agreeingCommits = mostCommitsCount;
                oqm.statusVar.failure.totalCommits = replyState.totalCommits;
                pendingQueryIndices.removeByValue(queryIndex);

                // cleanup reply state
                pendingCommitReplyStateIndices.removeByValue(replyStateIdx);
                pendingRevealReplyStateIndices.removeByValue(replyStateIdx);
                freeReplyStateSlot(replyStateIdx);

                // schedule contract notification(s) if needed
                if (oqm.type != ORACLE_QUERY_TYPE_USER_QUERY)
                    notificationQueryIndicies.add(queryIndex);

                // log status change
                logQueryStatusChange(oqm);

#if !defined(NDEBUG) && !defined(NO_UEFI)
                CHAR16 dbgMsg[200];
                setText(dbgMsg, L"oracleEngine.processTimeouts(), tick ");
                appendNumber(dbgMsg, system.tick, FALSE);
                appendText(dbgMsg, ", queryId ");
                appendNumber(dbgMsg, oqm.queryId, FALSE);
                appendText(dbgMsg, ", timeout ");
                appendDateAndTime(dbgMsg, oqm.timeout);
                appendText(dbgMsg, ", now ");
                appendDateAndTime(dbgMsg, now);
                addDebugMessage(dbgMsg);
#endif
            }
        }
    }

    /**
    * @brief Get info for notifying contracts. Call until nullptr is returned.
    * @return Pointer to notification info or nullptr if no notifications are needed.
    * 
    * Only to be used in tick processor! No concurrent use supported. Uses one internal buffer for returned data.
    */
    const OracleNotificationData* getNotification()
    {
        // currently no notifications needed?
        if (!notificationQueryIndicies.numValues)
            return nullptr;

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // get index and update list
        const uint32_t queryIndex = notificationQueryIndicies.values[0];
        notificationQueryIndicies.removeByIndex(0);

        // get query metadata
        const OracleQueryMetadata& oqm = queries[queryIndex];
        ASSERT(oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY);

        const auto replySize = OI::oracleInterfaces[oqm.interfaceIndex].replySize;
        ASSERT(16 + replySize < 0xffff);

        if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY)
        {
            // setup notification
            notificationOutputBuffer.contractIndex = oqm.typeVar.contract.queryingContract;
            notificationOutputBuffer.procedureId = oqm.typeVar.contract.notificationProcId;
            notificationOutputBuffer.inputSize = (uint16_t)(16 + replySize);
            setMem(notificationOutputBuffer.inputBuffer, notificationOutputBuffer.inputSize, 0);
            *(int64_t*)(notificationOutputBuffer.inputBuffer + 0) = oqm.queryId;
            *(uint32_t*)(notificationOutputBuffer.inputBuffer + 8) = 0;
            *(uint8_t*)(notificationOutputBuffer.inputBuffer + 12) = oqm.status;
            if (oqm.status == ORACLE_QUERY_STATUS_SUCCESS)
            {
                const void* replySrcPtr = getReplyDataFromTickTransactionStorage(oqm);
                copyMem(notificationOutputBuffer.inputBuffer + 16, replySrcPtr, replySize);
            }
        }

        // TODO: handle subscriptions

        return &notificationOutputBuffer;
    }

    // Drop all queries of the previous epoch.
    void beginEpoch()
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        reset();

        // TODO
        // clean all subscriptions
        // Issue: For some queries at the end of the epoch, the notification is never called, possible solutions:
        // - notify timeout or another error at end of epoch
        // - save state of oracle engine also for start from scratch
        // in the future we may:
        // - keep all non-pending queries of the last n ticks in case of seamless transition
        // - keep info of all pending queries as well
    }

protected:
    // Caller is responsible for locking.
    bool getOracleQueryWithoutLocking(int64_t queryId, void* queryData, uint16_t querySize) const
    {
        // get query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(queryId, queryIndex) || queryIndex >= oracleQueryCount)
            return false;

        // check query size
        const auto& queryMetadata = queries[queryIndex];
        ASSERT(queryMetadata.interfaceIndex < OI::oracleInterfacesCount);
        if (querySize != OI::oracleInterfaces[queryMetadata.interfaceIndex].querySize)
            return false;

        void* querySrcPtr = nullptr;
        switch (queryMetadata.type)
        {
            case ORACLE_QUERY_TYPE_CONTRACT_QUERY:
            {
                const auto offset = queryMetadata.typeVar.contract.queryStorageOffset;
                ASSERT(offset > 0 && offset < queryStorageBytesUsed && queryStorageBytesUsed <= ORACLE_QUERY_STORAGE_SIZE);
                querySrcPtr = queryStorage + offset;
                break;
            }
            // TODO: support other types
            default:
                return false;
        }

        // Return query data
        copyMem(queryData, querySrcPtr, querySize);
        return true;
    }

public:
    bool getOracleQuery(int64_t queryId, void* queryData, uint16_t querySize) const
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        return getOracleQueryWithoutLocking(queryId, queryData, querySize);
    }

    bool getOracleReply(int64_t queryId, void* replyData, uint16_t replySize) const
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // get query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(queryId, queryIndex) || queryIndex >= oracleQueryCount)
            return false;

        // check status
        const auto& queryMetadata = queries[queryIndex];
        if (queryMetadata.status != ORACLE_QUERY_STATUS_SUCCESS)
            return false;

        // check query size
        ASSERT(queryMetadata.interfaceIndex < OI::oracleInterfacesCount);
        if (replySize != OI::oracleInterfaces[queryMetadata.interfaceIndex].replySize)
            return false;

        // get reply data from tick transaction storage
        const void* replySrcPtr = getReplyDataFromTickTransactionStorage(queryMetadata);

        // return reply data
        copyMem(replyData, replySrcPtr, replySize);
        return true;
    }

    uint8_t getOracleQueryStatus(int64_t queryId) const
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // get query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(queryId, queryIndex) || queryIndex >= oracleQueryCount)
            return ORACLE_QUERY_STATUS_UNKNOWN;

        // return status
        const auto& queryMetadata = queries[queryIndex];
        return queryMetadata.status;
    }


    // Useful for debugging, but expensive: check that everything is as expected.
    void checkStateConsistencyWithAssert() const
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin oracleEngine.checkStateConsistencyWithAssert()");
        forceLogToConsoleAsAddDebugMessage = true;
        logStatus();
        forceLogToConsoleAsAddDebugMessage = false;
        __ScopedScratchpad scratchpad(1000, /*initZero=*/true);
        CHAR16* dbgMsgBuf = (CHAR16*)scratchpad.ptr;
#endif

        ASSERT(queries);
        ASSERT(queryStorage);
        ASSERT(queryIdToIndex);
        ASSERT(replyStates);
        ASSERT(oracleQueryCount <= MAX_ORACLE_QUERIES);
        ASSERT(queryStorageBytesUsed <= ORACLE_QUERY_STORAGE_SIZE);
        ASSERT(oracleQueryCount == queryIdToIndex->population());
        ASSERT(contractQueryIdState.tick <= system.tick);
        ASSERT(contractQueryIdState.queryIndexInTick < 0x7FFFFFFF);

        uint64_t successCount = 0;
        uint64_t timeoutCount = 0;
        uint64_t unresolvableCount = 0;
        uint64_t pendingCount = 0;
        uint64_t committedCount = 0;
        uint64_t storageBytesUsed = 8;
        for (uint32_t queryIndex = 0; queryIndex < oracleQueryCount; ++queryIndex)
        {
            const OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(oqm.interfaceIndex < OI::oracleInterfacesCount);
            ASSERT(oqm.queryId);
            uint32_t idx = 0;
            ASSERT(queryIdToIndex->get(oqm.queryId, idx) && idx == queryIndex);
            ASSERT(oqm.queryTick <= system.tick);
            ASSERT(oqm.queryId >> 31 == oqm.queryTick);
            ASSERT(oqm.timeout.isValid());
            switch (oqm.type)
            {
            case ORACLE_QUERY_TYPE_CONTRACT_QUERY:
                ASSERT(oqm.typeVar.contract.queryingContract > 0 && oqm.typeVar.contract.queryingContract < MAX_NUMBER_OF_CONTRACTS);
                ASSERT(oqm.typeVar.contract.queryStorageOffset < queryStorageBytesUsed && oqm.typeVar.contract.queryStorageOffset > 0);
                storageBytesUsed += OI::oracleInterfaces[oqm.interfaceIndex].querySize;
                break;
            case ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION:
                // TODO
                break;
            case ORACLE_QUERY_TYPE_USER_QUERY:
                ASSERT(!isZero(oqm.typeVar.user.queryingEntity));
                ASSERT(oqm.typeVar.user.queryTxIndex < NUMBER_OF_TRANSACTIONS_PER_TICK);
                // TODO: check vs tick storage?
                break;
            default:
#if !defined(NDEBUG) && !defined(NO_UEFI)
                setText(dbgMsgBuf, L"Unexpected oracle query type ");
                appendNumber(dbgMsgBuf, oqm.status, FALSE);
                addDebugMessage(dbgMsgBuf);
#endif
                break;
            }

            // shared status checks
            const ReplyState* replyState = nullptr;
            uint16_t agreeingCommits = 0;
            switch (oqm.status)
            {
            case ORACLE_QUERY_STATUS_PENDING:
            case ORACLE_QUERY_STATUS_COMMITTED:
                ASSERT(oqm.statusVar.pending.replyStateIndex < MAX_SIMULTANEOUS_ORACLE_QUERIES);
                replyState = replyStates + oqm.statusVar.pending.replyStateIndex;
                ASSERT(replyState->queryId == oqm.queryId);
                ASSERT(replyState->ownReplySize == 0 || replyState->ownReplySize == OI::oracleInterfaces[oqm.interfaceIndex].replySize);
                ASSERT(replyState->mostCommitsHistIdx < NUMBER_OF_COMPUTORS);
                agreeingCommits = replyState->replyCommitHistogramCount[replyState->mostCommitsHistIdx];
                ASSERT(agreeingCommits <= replyState->totalCommits);
                ASSERT(replyState->totalCommits <= NUMBER_OF_COMPUTORS);
                break;
            case ORACLE_QUERY_STATUS_UNRESOLVABLE:
            case ORACLE_QUERY_STATUS_TIMEOUT:
                ASSERT(oqm.statusVar.failure.agreeingCommits < QUORUM);
                ASSERT(oqm.statusVar.failure.totalCommits <= NUMBER_OF_COMPUTORS);
                ASSERT(oqm.statusVar.failure.agreeingCommits <= oqm.statusVar.failure.totalCommits);
                break;
            }

            // specific status checks
            switch (oqm.status)
            {
            case ORACLE_QUERY_STATUS_PENDING:
                ++pendingCount;
                ASSERT(agreeingCommits < QUORUM);
                break;
            case ORACLE_QUERY_STATUS_COMMITTED:
                ++committedCount;
                ASSERT(agreeingCommits >= QUORUM);
                break;
            case ORACLE_QUERY_STATUS_SUCCESS:
                ++successCount;
                ASSERT(oqm.statusVar.success.revealTick <= system.tick);
                ASSERT(oqm.statusVar.success.revealTxIndex < NUMBER_OF_TRANSACTIONS_PER_TICK);
                break;
            case ORACLE_QUERY_STATUS_UNRESOLVABLE:
                ++unresolvableCount;
                ASSERT(oqm.statusVar.failure.totalCommits - oqm.statusVar.failure.agreeingCommits > NUMBER_OF_COMPUTORS - QUORUM);
                break;
            case ORACLE_QUERY_STATUS_TIMEOUT:
                ++timeoutCount;
                break;
            default:
#if !defined(NDEBUG) && !defined(NO_UEFI)
                setText(dbgMsgBuf, L"Unexpected oracle query state ");
                appendNumber(dbgMsgBuf, oqm.status, FALSE);
                addDebugMessage(dbgMsgBuf);
#endif
                break;
            }
        }
        for (uint32_t queryIndex = oracleQueryCount; queryIndex < MAX_ORACLE_QUERIES; ++queryIndex)
        {
            const OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(!oqm.queryId);
            ASSERT(!oqm.status);
        }

        ASSERT(queryStorageBytesUsed == storageBytesUsed);
        ASSERT(oracleQueryCount == pendingCount + committedCount + successCount + timeoutCount + unresolvableCount);
        ASSERT(committedCount == pendingRevealReplyStateIndices.numValues); // currently in committed state
        ASSERT(pendingCount + committedCount == pendingQueryIndices.numValues); // not finished (no success / failure)
        ASSERT(successCount == stats.successCount);
        ASSERT(timeoutCount == stats.timeoutNoReplyCount + stats.timeoutNoCommitCount + stats.timeoutNoRevealCount);
        ASSERT(unresolvableCount == stats.unresolvableCount);
        ASSERT(committedCount + successCount + stats.timeoutNoRevealCount == stats.commitCount);

        // check pending queries index
        uint32_t queryIdxCount = pendingQueryIndices.numValues;
        const uint32_t* queryIndices = pendingQueryIndices.values;
        for (uint32_t i = 0; i < queryIdxCount; ++i)
        {
            const uint32_t queryIndex = queryIndices[i];
            ASSERT(queryIndex < oracleQueryCount);
            const OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(oqm.status == ORACLE_QUERY_STATUS_PENDING || oqm.status == ORACLE_QUERY_STATUS_COMMITTED);
        }

        // check index of reply states with pending reply commit quorum
        uint32_t replyIdxCount = pendingCommitReplyStateIndices.numValues;
        const uint32_t* replyIndices = pendingCommitReplyStateIndices.values;
        for (uint32_t idx = 0; idx < replyIdxCount; ++idx)
        {
            const uint32_t replyIdx = replyIndices[idx];
            ASSERT(replyIdx < MAX_SIMULTANEOUS_ORACLE_QUERIES);
            const ReplyState& replyState = replyStates[replyIdx];
            ASSERT(replyState.queryId);
            uint32_t queryIndex = 0;
            ASSERT(queryIdToIndex->get(replyState.queryId, queryIndex) && queryIndex < oracleQueryCount);
            const OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(oqm.status == ORACLE_QUERY_STATUS_PENDING);
        }

        // check index of reply states with pending reply reveal tx
        replyIdxCount = pendingRevealReplyStateIndices.numValues;
        replyIndices = pendingRevealReplyStateIndices.values;
        for (uint32_t idx = 0; idx < replyIdxCount; ++idx)
        {
            const uint32_t replyIdx = replyIndices[idx];
            ASSERT(replyIdx < MAX_SIMULTANEOUS_ORACLE_QUERIES);
            const ReplyState& replyState = replyStates[replyIdx];
            ASSERT(replyState.queryId);
            uint32_t queryIndex = 0;
            ASSERT(queryIdToIndex->get(replyState.queryId, queryIndex) && queryIndex < oracleQueryCount);
            const OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(oqm.status == ORACLE_QUERY_STATUS_COMMITTED);
        }

        // check index of pending notifications
        queryIdxCount = notificationQueryIndicies.numValues;
        queryIndices = notificationQueryIndicies.values;
        for (uint32_t i = 0; i < queryIdxCount; ++i)
        {
            const uint32_t queryIndex = queryIndices[i];
            ASSERT(queryIndex < oracleQueryCount);
            const OracleQueryMetadata& oqm = queries[queryIndex];
            ASSERT(oqm.status != ORACLE_QUERY_STATUS_PENDING);
        }

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End oracleEngine.checkStateConsistencyWithAssert()");
#endif
    }

    void logStatus() const
    {
        auto appendQuotientWithOneDecimal = [](CHAR16* message, uint64_t dividend, uint64_t divisor)
        {
            unsigned long long quotient10 = (divisor) ? (dividend * 10 / divisor) : 0;
            appendText(message, ", ");
            appendNumber(message, quotient10 / 10, FALSE);
            appendText(message, ".");
            appendNumber(message, quotient10 % 10, FALSE);
        };


        setText(message, L"Oracles queries: pending ");
        // print total number of pending queries
        appendNumber(message, pendingQueryIndices.numValues, FALSE);
        // print number of pending queries currently waiting for OM reply
        appendText(message, " (OM ");
        appendNumber(message, pendingQueryIndices.numValues - pendingCommitReplyStateIndices.numValues - pendingRevealReplyStateIndices.numValues, FALSE);
        // print how many ticks it takes on average until the OM reply is received
        appendQuotientWithOneDecimal(message, stats.oracleMachineReplyTicksSum, stats.oracleMachineReplyCount);
        appendText(message, " ticks; commit ");
        // print number of pending queries currently waiting for commit status
        appendNumber(message, pendingCommitReplyStateIndices.numValues, FALSE);
        // print how many ticks it takes on average until the commit status is reached (until 451 commit tx got executed)
        appendQuotientWithOneDecimal(message, stats.commitTicksSum, stats.commitCount);
        appendText(message, " ticks; reveal ");
        // print number of pending queries currently waiting for reveal transaction
        appendNumber(message, pendingRevealReplyStateIndices.numValues, FALSE);
        // print how many ticks it takes on average until success (ticks until 1 reveal tx got executed)
        appendQuotientWithOneDecimal(message, stats.successTicksSum, stats.successCount);
        appendText(message, " ticks), successful ");
        appendNumber(message, stats.successCount, FALSE);
        appendText(message, ", unresolvable ");
        appendNumber(message, stats.unresolvableCount, FALSE);
        appendText(message, ", timeout ");
        const uint64_t totalTimeouts = stats.timeoutNoReplyCount + stats.timeoutNoCommitCount + stats.timeoutNoRevealCount;
        appendNumber(message, totalTimeouts, FALSE);
        appendText(message, " (OM ");
        appendNumber(message, stats.timeoutNoReplyCount, FALSE);
        appendText(message, ", commit ");
        appendNumber(message, stats.timeoutNoCommitCount, FALSE);
        appendText(message, ", reveal ");
        appendNumber(message, stats.timeoutNoRevealCount, FALSE);
        appendQuotientWithOneDecimal(message, stats.timeoutTicksSum, totalTimeouts);
        appendText(message, " ticks), conflicting OM replies ");
        appendNumber(message, stats.oracleMachineRepliesDisagreeCount, FALSE);
        appendText(message, "), query slots occupied ");
        appendNumber(message, oracleQueryCount * 100 / MAX_ORACLE_QUERIES, FALSE);
        appendText(message, "%, query storage occupied ");
        appendNumber(message, queryStorageBytesUsed * 100 / ORACLE_QUERY_STORAGE_SIZE, FALSE);
        appendText(message, "%");
        appendQuotientWithOneDecimal(message, stats.revealTxCount, stats.successCount);
        appendText(message, " reveal tx per success");
        logToConsole(message);

#if ENABLE_ORACLE_STATS_RECORD
        // Show per-interface oracle stats
        for (uint32_t i = 0; i < OI::oracleInterfacesCount; i++)
        {
            setText(message, L"Oracle[");
            appendNumber(message, i, FALSE);
            appendText(message, L"]: queries=");
            appendNumber(message, oracleStats[i].queryCount, FALSE);
            appendText(message, L", replies=");
            appendNumber(message, oracleStats[i].replyCount, FALSE);
            appendText(message, L", valid=");
            appendNumber(message, oracleStats[i].validCount, FALSE);
            appendText(message, L", data=");
            appendNumber(message, oracleStats[i].extraData[0], TRUE);
            if (i == 1)
            {
                appendText(message, L", ");
                appendNumber(message, oracleStats[i].extraData[1], TRUE);
                appendText(message, L", ");
                appendNumber(message, oracleStats[i].extraData[2], TRUE);
            }
#endif
            logToConsole(message);
        }
    }

    void processRequestOracleData(Peer* peer, RequestResponseHeader* header) const;
};

GLOBAL_VAR_DECL OracleEngine<computorSeedsCount> oracleEngine;

/*
- Handle seamless transitions? Keep state?

*/
