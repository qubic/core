#pragma once

#include "network_messages/common_def.h"
#include "contracts/qpi.h"

#include "system.h"

#include "oracle_transactions.h"
#include "core_om_network_messages.h"

#include "platform/memory_util.h"

void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);

constexpr uint32_t MAX_ORACLE_QUERIES = (1 << 18);
constexpr uint64_t ORACLE_QUERY_STORAGE_SIZE = MAX_ORACLE_QUERIES * 512;
constexpr uint32_t MAX_SIMULTANEOUS_ORACLE_QUERIES = 1024;

struct OracleQueryMetadata
{
    uint64_t queryId;         ///< Higher 32 bits are query tick, lower are composed of contract ID and index. 0 is reserved for invalid ID.
    uint8_t type;             ///< contract query, user query, subscription (may be by multiple contracts)
    uint8_t status;           ///< overall status (pending -> success or timeout)
    uint16_t statusFlags;     ///< status and error flags (especially as returned by oracle machine connected to this node)
    uint32_t interfaceIndex;
    uint32_t queryTick;
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
            uint32_t subscriptionId; ///< 0 is reserved for "no subscription"
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

// One per contract
struct OracleContractStatus
{
    uint32_t tick;
    uint16_t queryIndexInTick;
};

struct OracleSubscriptionContractStatus
{
    uint32_t subscriptionId;
    uint16_t contractIndex;
    uint16_t notificationPeriodMinutes;
    QPI::DateAndTime nextNotification;
};

struct OracleSubscriptionMetadata
{
    uint64_t initialQueryStorageOffset;

    // Number of minutes between consecutive queries, scheduler triggers query in processTick()
    uint16_t queryIntervalMinutes;

    // Offset of DateAndTime timestamp member variable in oracle query struct
    uint16_t queryTimestampOffset;

    // ID of last query
    uint64_t lastQueryId;

    // ID of last revealed, for notifying with cached value on new subscription?
    uint64_t lastRevealId;

    // Timestamp for triggering next query and notifying about timeout if no query was received
    QPI::DateAndTime nextQueryTimestamp;
};

// State of received OM reply and computor commits for a single oracle query
struct OracleReplyState
{
    uint64_t queryId;

    m256i ownReplyDigest;
    uint16_t ownReplySize;
    uint8_t ownReplyData[MAX_ORACLE_REPLY_SIZE + 2];

    uint16_t ownReplyCommitExecCount;
    uint32 ownReplyCommitComputorTxTick[computerSeedsCount];
    uint32 ownReplyCommitComputorTxExecuted[computerSeedsCount];

    m256i replyCommitDigests[NUMBER_OF_COMPUTORS];
    m256i replyCommitKnowledgeProofs[NUMBER_OF_COMPUTORS];
    uint32_t replyCommitTicks[NUMBER_OF_COMPUTORS];

    uint16_t totalCommits;
    uint16_t mostCommitsCount;
    m256i mostCommitsDigest;

    uint32_t revealTick;
    uint32_t revealTxIndex;

    USER_PROCEDURE notificationProcedure;
    uint32_t notificationLocalsSize;
};

// 
struct OracleRevenueCounter
{
    uint32_t computorRevPoints[NUMBER_OF_COMPUTORS];
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

    bool remove(unsigned int idx)
    {
        ASSERT(numValues <= N);
        if (idx >= numValues || numValues == 0)
            return false;
        --numValues;
        if (idx != numValues)
        {
            values[idx] = values[numValues];
        }
    }
};


// TODO: locking, implement hash function for queryIdToIndex based on tick
class OracleEngine
{
    /// array of all oracle queries of the epoch with capacity for MAX_ORACLE_QUERIES elements
    OracleQueryMetadata* queries;

    /// number of elements used in queries
    uint32_t oracleQueryCount;

    // buffer continuously filled with variable-size query data (similar to tx storage)
    uint8_t* queryStorage;

    // how many bytes of queryStorage array are already in use / offset for adding new data
    uint64_t queryStorageBytesUsed;

    // status of each contract for assigning IDs
    OracleContractStatus contractStatus[MAX_NUMBER_OF_CONTRACTS];

    // state of received OM reply and computor commits for each oracle query (used before reveal)
    OracleReplyState* replyStates;

    // index in replyStates to check next for empty slot (cyclic buffer)
    int32_t replyStatesIndex;

    /// fast lookup of query indices for which oracles are in pending state
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingQueryIndices;

    /// fast lookup of reply state indices for which commit tx is pending
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingCommitReplyStateIndices;

    /// fast lookup of oracle query index (sequential position in queries array) from oracle query ID (composed of query tick and other info)
    QPI::HashMap<uint64_t, uint32_t, MAX_ORACLE_QUERIES>* queryIdToIndex;

    /// Return empty reply state slot or max uint32 value on error
    uint32_t getEmptyReplyStateSlot()
    {
        ASSERT(replyStatesIndex < MAX_SIMULTANEOUS_ORACLE_QUERIES);
        for (uint32_t i = 0; i < MAX_SIMULTANEOUS_ORACLE_QUERIES; ++i)
        {
            if (!replyStates[replyStatesIndex].queryId)
                return replyStatesIndex;

            ++replyStatesIndex;
            if (replyStatesIndex >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
                replyStatesIndex = 0;
        }
        return 0xffffffff;
    }

public:
    bool init()
    {
        // alloc arrays and set to 0
        if (!allocPoolWithErrorLog(L"OracleEngine::queries", MAX_ORACLE_QUERIES * sizeof(*queries), (void**)&queries, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::queryStorage", ORACLE_QUERY_STORAGE_SIZE, (void**)&queryStorage, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::replyCommitState", MAX_SIMULTANEOUS_ORACLE_QUERIES * sizeof(*replyStates), (void**)&replyStates, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::queryIdToIndex", sizeof(*queryIdToIndex), (void**)&queryIdToIndex, __LINE__))
        {
            return false;
        }

        oracleQueryCount = 0;
        queryStorageBytesUsed = 1; // reserve offset 0 for "no data"
        setMem(contractStatus, sizeof(contractStatus), 0);
        replyStatesIndex = 0;
        pendingQueryIndices.numValues = 0;
        pendingCommitReplyStateIndices.numValues = 0;

        return true;
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

    uint64_t startContractQuery(uint16_t contractIndex, uint32_t interfaceIndex,
        const void* queryData, uint16_t querySize, uint32_t timeoutMillisec,
        USER_PROCEDURE notificationProcedure, uint32_t notificationLocalsSize)
    {
        // check inputs
        if (contractIndex >= MAX_NUMBER_OF_CONTRACTS || interfaceIndex >= OI::oracleInterfacesCount || querySize != OI::oracleInterfaces[interfaceIndex].querySize)
            return 0;

        // check that still have free capacity for the query
        if (oracleQueryCount >= MAX_ORACLE_QUERIES || pendingQueryIndices.numValues >= MAX_SIMULTANEOUS_ORACLE_QUERIES || queryStorageBytesUsed + querySize > ORACLE_QUERY_STORAGE_SIZE)
            return 0;

        // find slot storing temporary reply state
        uint32_t replyStateSlotIdx = getEmptyReplyStateSlot();
        if (replyStateSlotIdx >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
            return 0;

        // compute timeout as absolute point in time
        DateAndTime timeout = DateAndTime::now();
        if (!timeout.addMillisec(timeoutMillisec))
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
        uint64_t queryId = ((uint64_t)system.tick << 32) | ((uint64_t)contractIndex << 16) | cs.queryIndexInTick;

        // map ID to index
        ASSERT(!queryIdToIndex->contains(queryId));
        if (queryIdToIndex->set(queryId, oracleQueryCount) == NULL_INDEX)
            return 0;

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
        queryMetadata.statusVar.pending.replyStateIndex = replyStateSlotIdx;

        // init reply state (temporary until reply is revealed)
        auto& replyState = replyStates[replyStateSlotIdx];
        setMemory(replyState, 0);
        replyState.queryId = queryId;
        replyState.notificationProcedure = notificationProcedure;
        replyState.notificationLocalsSize = notificationLocalsSize;

        // copy oracle query data to permanent storage
        copyMem(queryStorage + queryStorageBytesUsed, queryData, querySize);
        queryStorageBytesUsed += querySize;

        // enqueue query message to oracle machine node
        enqueueOracleQuery(queryId, interfaceIndex, timeoutMillisec, queryData, querySize);

        return queryId;
    }

    // Enqueue oracle machine query message. May be called from tick processor or contract processor only (uses reorgBuffer).
    void enqueueOracleQuery(uint64_t queryId, uint32_t interfaceIdx, uint16_t timeoutMillisec, const void* queryData, uint16 querySize)
    {
        // Prepare message payload
        OracleMachineQuery* omq = reinterpret_cast<OracleMachineQuery*>(reorgBuffer);
        omq->oracleQueryId = queryId;
        omq->oracleInterfaceIndex = interfaceIdx;
        omq->timeoutInMilliseconds = timeoutMillisec;
        copyMem(omq + 1, queryData, querySize);

        // Enqueue for sending to all oracle machine peers (peer pointer address 0x1 is reserved for that)
        enqueueResponse((Peer*)0x1, sizeof(*omq) + querySize, OracleMachineQuery::type(), 0, omq);
    }

    // CAUTION: Called from request processor, requires locking!
    void processOracleMachineReply(const OracleMachineReply* replyMessage, uint32_t replyMessageSize)
    {
        // check input params
        ASSERT(replyMessage);
        if (replyMessageSize < sizeof(OracleMachineReply))
            return;

        // get query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(replyMessage->oracleQueryId, queryIndex) || queryIndex >= oracleQueryCount)
            return;

        // get query metadata
        OracleQueryMetadata& oqm = queries[queryIndex];
        if (oqm.status != ORACLE_QUERY_STATUS_PENDING)
            return;

        // update query status flags
        oqm.statusFlags |= (replyMessage->oracleMachineErrorFlags & ORACLE_FLAG_OM_ERROR_FLAGS);

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
        OracleReplyState& replyState = replyStates[replyStateIdx];

        // return if we already got a reply
        if (replyState.ownReplySize)
        {
            // if the digests don't match, set an error flag
            if (replyDigest != replyState.ownReplyDigest)
                oqm.statusFlags |= ORACLE_FLAG_OM_DISAGREE;
            return;
        }

        // copy reply from message to state struct and set status flag
        copyMem(replyState.ownReplyData, replyData, replySize);
        replyState.ownReplyDigest = replyDigest;
        replyState.ownReplySize = replySize;
        oqm.statusFlags |= ORACLE_FLAG_REPLY_RECEIVED;

        // add reply state to set of indices with pending commit tx
        pendingCommitReplyStateIndices.add(replyStateIdx);
    }

    /// Return array of reply indices and size of array (as output-by-reference parameter). To be used for getReplyCommitTransactionItem().
    const uint32_t* getPendingReplyCommitTransactionIndices(uint32_t& arraySizeOutput) const
    {
        arraySizeOutput = pendingCommitReplyStateIndices.numValues;
        return pendingCommitReplyStateIndices.values;
    }

    /**
    * Return commit items in OracleReplyCommitTransaction.
    * @param computorIdx Index of computor list of computors broadcasted by arbitrator.
    * @param ownComputorIdx Index of computor in local array computorSeeds.
    * @param replyIdx Index of reply to consider. Use getPendingReplyCommitTransactionIndices() to get an array of those.
    * @param txScheduleTick Tick, in which the transaction is supposed to be scheduled.
    * @param commit Pointer to output buffer of commit data in transaction that is being constructed.
    * @return Whether this computor/reply is supposed to be added to tx. If false, commit is untouched.
    *
    * Called from tick processor.
    */
    bool getReplyCommitTransactionItem(
        uint16_t computorIdx, uint16_t ownComputorIdx,
        int32_t replyIdx, uint32_t txScheduleTick,
        OracleReplyCommitTransactionItem* commit)
    {
        // check inputs
        ASSERT(commit);
        if (ownComputorIdx >= computerSeedsCount || computorIdx >= NUMBER_OF_COMPUTORS || replyIdx >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
            return false;

        // get reply state and check that oracle reply has been received
        OracleReplyState& replyState = replyStates[replyIdx];
        if (replyState.queryId == 0 || replyState.ownReplySize == 0)
            return false;

        // tx already executed or scheduled?
        if (replyState.ownReplyCommitComputorTxExecuted[ownComputorIdx] ||
            replyState.ownReplyCommitComputorTxTick[ownComputorIdx] >= system.tick) // TODO: > or >= ?
            return false;

        // set known data of commit tx part
        commit->queryId = replyState.queryId;
        commit->replyDigest = replyState.ownReplyDigest;

        // compute knowledge proof of commit = K12(oracle reply + computor index)
        ASSERT(replyState.ownReplySize <= MAX_ORACLE_REPLY_SIZE);
        *(uint16_t*)(replyState.ownReplyData + replyState.ownReplySize) = computorIdx;
        KangarooTwelve(replyState.ownReplyData, replyState.ownReplySize + 2, &commit->replyKnowledgeProof, 32);

        // signal to schedule tx for given tick
        replyState.ownReplyCommitComputorTxTick[ownComputorIdx] = txScheduleTick;
        return true;
    }

    // Called from tick processor.
    void processTransactionOracleReplyCommit(const OracleReplyCommitTransactionPrefix* transaction)
    {
        ASSERT(transaction != nullptr);
        ASSERT(transaction->checkValidity());
        ASSERT(isZero(transaction->destinationPublicKey));
        ASSERT(transaction->tick == system.tick);

        if (transaction->inputSize < OracleReplyCommitTransactionPrefix::minInputSize())
            return;

        int compIdx = computorIndex(transaction->sourcePublicKey);
        if (compIdx < 0)
            return;

        const OracleReplyCommitTransactionItem* item = (const OracleReplyCommitTransactionItem*)transaction->inputPtr();
        uint32_t size = sizeof(OracleReplyCommitTransactionItem);
        while (size <= transaction->inputSize)
        {
            uint32_t queryIndex;
            if (!queryIdToIndex->get(item->queryId, queryIndex) || queryIndex >= oracleQueryCount)
                continue;

            OracleQueryMetadata& oqm = queries[queryIndex];
            if (oqm.status != ORACLE_QUERY_STATUS_PENDING && oqm.status != ORACLE_QUERY_STATUS_COMMITTED)
                continue;

            // TODO

            size += sizeof(OracleReplyCommitTransactionItem);
            ++item;
        }
    }

    void beginEpoch()
    {
        // TODO
        // clean all subscriptions
        // clean all queries (except for last n ticks in case of seamless transition)
    }

    bool getOracleQuery(uint64_t queryId, const void* queryData, uint16_t querySize) const
    {
        // get query index
        uint32_t queryIndex;
        if (!queryIdToIndex->get(queryId, queryIndex) || queryIndex >= oracleQueryCount)
            return false;

        const auto& queryMetadata = queries[queryIndex];
        // TODO

        return true;
    }

    void logStatus(CHAR16* message) const
    {
        setText(message, L"Oracles queries: ");
        appendNumber(message, pendingCommitReplyStateIndices.numValues, FALSE);
        appendText(message, " / ");
        appendNumber(message, pendingQueryIndices.numValues, FALSE);
        appendText(message, " got replies from OM node");
        logToConsole(message);
    }
};

GLOBAL_VAR_DECL OracleEngine oracleEngine;

/*
- Handle seamless transitions? Keep state?

*/
