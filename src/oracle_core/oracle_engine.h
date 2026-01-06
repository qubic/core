#pragma once

#include "contract_core/pre_qpi_def.h"
#include "contracts/qpi.h"
#include "oracle_core/oracle_interfaces_def.h"

#include "system.h"
#include "common_buffers.h"
#include "spectrum/special_entities.h"
#include "ticking/tick_storage.h"
#include "contract_core/contract_exec.h"

#include "oracle_transactions.h"
#include "core_om_network_messages.h"

#include "platform/memory_util.h"


void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);

constexpr uint32_t MAX_ORACLE_QUERIES = (1 << 18);
constexpr uint64_t ORACLE_QUERY_STORAGE_SIZE = MAX_ORACLE_QUERIES * 512;
constexpr uint32_t MAX_SIMULTANEOUS_ORACLE_QUERIES = 1024;

struct OracleQueryMetadata
{
    int64_t queryId;          ///< bits 31-62 encode index in tick, bits 0-30 are index in tick, negative values indicate error
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
            uint32_t notificationProcId;
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

    struct {
        /// total number of successful oracle queries
        unsigned long long successCount;

        /// total number of timeout oracle queries
        unsigned long long timeoutCount;

        /// total number of timeout oracle queries
        unsigned long long unresolvableCount;
    } stats;

    /// fast lookup of oracle query index (sequential position in queries array) from oracle query ID (composed of query tick and other info)
    QPI::HashMap<int64_t, uint32_t, MAX_ORACLE_QUERIES>* queryIdToIndex;

    /// array of ownComputorSeedsCount public keys (mainly for testing, in EFI core this points to computorPublicKeys from special_entities.h)
    const m256i* ownComputorPublicKeys;

    /// buffer used to store input for contract notifications
    uint8_t contractNotificationInputBuffer[16 + MAX_ORACLE_REPLY_SIZE];

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

public:
    /// Initialize object, passing array of own computor public keys (with number of elements given by template param ownComputorSeedsCount).
    bool init(const m256i* ownComputorPublicKeys)
    {
        this->ownComputorPublicKeys = ownComputorPublicKeys;
        lock = 0;

        // alloc arrays and set to 0
        if (!allocPoolWithErrorLog(L"OracleEngine::queries", MAX_ORACLE_QUERIES * sizeof(*queries), (void**)&queries, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::queryStorage", ORACLE_QUERY_STORAGE_SIZE, (void**)&queryStorage, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::replyCommitState", MAX_SIMULTANEOUS_ORACLE_QUERIES * sizeof(*replyStates), (void**)&replyStates, __LINE__)
            || !allocPoolWithErrorLog(L"OracleEngine::queryIdToIndex", sizeof(*queryIdToIndex), (void**)&queryIdToIndex, __LINE__))
        {
            return false;
        }

        oracleQueryCount = 0;
        queryStorageBytesUsed = 8; // reserve offset 0 for "no data"
        setMem(&contractQueryIdState, sizeof(contractQueryIdState), 0);
        replyStatesIndex = 0;
        pendingQueryIndices.numValues = 0;
        pendingCommitReplyStateIndices.numValues = 0;
        pendingRevealReplyStateIndices.numValues = 0;
        setMem(&stats, sizeof(stats), 0);

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
        LockGuard lockGuard(lock);
        // save state (excluding queryIdToIndex and unused parts of large buffers)
    }

    void load()
    {
        LockGuard lockGuard(lock);
        // load state (excluding queryIdToIndex and unused parts of large buffers)
        // init queryIdToIndex
    }

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
            return -1;

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // check that still have free capacity for the query
        if (oracleQueryCount >= MAX_ORACLE_QUERIES || pendingQueryIndices.numValues >= MAX_SIMULTANEOUS_ORACLE_QUERIES || queryStorageBytesUsed + querySize > ORACLE_QUERY_STORAGE_SIZE)
            return -1;

        // find slot storing temporary reply state
        uint32_t replyStateSlotIdx = getEmptyReplyStateSlot();
        if (replyStateSlotIdx >= MAX_SIMULTANEOUS_ORACLE_QUERIES)
            return -1;

        // compute timeout as absolute point in time
        auto timeout = QPI::DateAndTime::now();
        if (!timeout.addMillisec(timeoutMillisec))
            return -1;

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
                return -1;
            ++cs.queryIndexInTick;
        }

        // compose query ID
        int64_t queryId = ((int64_t)system.tick << 31) | cs.queryIndexInTick;
        static_assert(((0xFFFFFFFFll << 31) & 0x7FFFFFFFll) == 0ll && ((0xFFFFFFFFll << 31) | 0x7FFFFFFFll) > 0);

        // map ID to index
        ASSERT(!queryIdToIndex->contains(queryId));
        if (queryIdToIndex->set(queryId, oracleQueryCount) == QPI::NULL_INDEX)
            return -1;

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

        // TODO: send log event ORACLE_QUERY with queryId, query starter, interface, type, status

        return queryId;
    }

protected:
    // Enqueue oracle machine query message. May be called from tick processor or contract processor only (uses reorgBuffer).
    static void enqueueOracleQuery(int64_t queryId, uint32_t interfaceIdx, uint32_t timeoutMillisec, const void* queryData, uint16_t querySize)
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

    void notifyContractsIfAny(const OracleQueryMetadata& oqm, const void* replyData = nullptr)
    {
        /*
        // TODO: change to run in contract prcocessor -> move parts to qubic.cpp
        ASSERT(oqm.queryId == replyState.queryId);
        if (oqm.type == ORACLE_QUERY_TYPE_USER_QUERY)
            return;

        const auto replySize = OI::oracleInterfaces[oqm.interfaceIndex].replySize;
        ASSERT(16 + replySize < 0xffff);

        UserProcedureNotification notification;
        if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY)
        {
            // setup notification
            notification.contractIndex = oqm.typeVar.contract.queryingContract;
            notification.procedure = replyState.notificationProcedure;
            notification.inputSize = (uint16_t)(16 + replySize);
            notification.inputPtr = contractNotificationInputBuffer;
            notification.localsSize = replyState.notificationLocalsSize;
            setMem(contractNotificationInputBuffer, notification.inputSize, 0);
            *(int64_t*)(contractNotificationInputBuffer + 0) = oqm.queryId;
            *(uint32_t*)(contractNotificationInputBuffer + 8) = 0;
            *(uint32_t*)(contractNotificationInputBuffer + 12) = oqm.status;
            if (replyData)
            {
                copyMem(contractNotificationInputBuffer + 16, replyData, replySize);
            }

            // run notification
            QpiContextUserProcedureNotificationCall qpiContext(notification);
            qpiContext.call();
        }

        // TODO: handle subscriptions
        */
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
        ReplyState& replyState = replyStates[replyStateIdx];
        ASSERT(replyState.queryId == replyMessage->oracleQueryId);

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
            return false;

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // process the N commits in this tx
        const OracleReplyCommitTransactionItem* item = (const OracleReplyCommitTransactionItem*)transaction->inputPtr();
        uint32_t size = sizeof(OracleReplyCommitTransactionItem);
        while (size <= transaction->inputSize)
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
                // -> switch to status COMMITTED
                if (oqm.status != ORACLE_QUERY_STATUS_COMMITTED)
                {
                    oqm.status = ORACLE_QUERY_STATUS_COMMITTED;
                    pendingCommitReplyStateIndices.removeByValue(replyStateIdx);
                    pendingRevealReplyStateIndices.add(replyStateIdx);
                    // TODO: send log event ORACLE_QUERY with queryId, query starter, interface, type, status
                }
            }
            else if (replyState.totalCommits - mostCommitsCount > NUMBER_OF_COMPUTORS - QUORUM)
            {
                // more than 1/3 of commits don't vote for most voted digest -> getting quorum isn't possible
                // -> switch to status UNRESOLVABLE
                oqm.status = ORACLE_QUERY_STATUS_UNRESOLVABLE;
                oqm.statusFlags |= ORACLE_FLAG_COMP_DISAGREE;
                oqm.statusVar.failure.agreeingCommits = mostCommitsCount;
                oqm.statusVar.failure.totalCommits = replyState.totalCommits;
                pendingQueryIndices.removeByValue(queryIndex);
                ++stats.unresolvableCount;

                // run contract notification(s) if needed
                notifyContractsIfAny(oqm);

                // cleanup data of pending reply immediately (no info for revenue required)
                pendingCommitReplyStateIndices.removeByValue(replyStateIdx);
                freeReplyStateSlot(replyStateIdx);

                // TODO: send log event ORACLE_QUERY with queryId, query starter, interface, type, status
            }

            // go to next commit in tx
            size += sizeof(OracleReplyCommitTransactionItem);
            ++item;
        }

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
    }

    // Called from tick processor.
    bool processOracleReplyRevealTransaction(const OracleReplyRevealTransactionPrefix* transaction, uint32_t txSlotInTickData)
    {
        ASSERT(txSlotInTickData < NUMBER_OF_TRANSACTIONS_PER_TICK);
        ASSERT(transaction->tick == system.tick);

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // check tx and get reply state + query metadata
        uint32_t queryIndex;
        ReplyState* replyState = checkReplyRevealTransaction(transaction, &queryIndex);
        if (!replyState)
            return false;
        OracleQueryMetadata& oqm = queries[queryIndex];
        const auto replyStateIdx = oqm.statusVar.pending.replyStateIndex;

        // TODO: check knowledge proofs of all computors and add revenue points for computors who sent correct commit tx fastest

        // update state
        oqm.statusVar.success.revealTick = transaction->tick;
        oqm.statusVar.success.revealTxIndex = txSlotInTickData;
        oqm.status = ORACLE_QUERY_STATUS_SUCCESS;
        pendingQueryIndices.removeByValue(queryIndex);
        ++stats.successCount;

        // run contract notification(s) if needed
        const void* replyData = transaction + 1;
        notifyContractsIfAny(oqm, replyData);

        // cleanup data of pending reply
        pendingRevealReplyStateIndices.removeByValue(replyStateIdx);
        freeReplyStateSlot(replyStateIdx);

        return true;
    }


    void beginEpoch()
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // TODO
        // clean all subscriptions
        // clean all queries (except for last n ticks in case of seamless transition)
    }

    bool getOracleQuery(int64_t queryId, void* queryData, uint16_t querySize) const
    {
        // lock for accessing engine data
        LockGuard lockGuard(lock);

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

    void logStatus(CHAR16* message) const
    {
        setText(message, L"Oracles queries: pending ");
        appendNumber(message, pendingCommitReplyStateIndices.numValues, FALSE);
        appendText(message, " / ");
        appendNumber(message, pendingRevealReplyStateIndices.numValues, FALSE);
        appendText(message, " / ");
        appendNumber(message, pendingQueryIndices.numValues, FALSE);
        appendText(message, ", successful ");
        appendNumber(message, stats.successCount, FALSE);
        appendText(message, ", timeout ");
        appendNumber(message, stats.timeoutCount, FALSE);
        appendText(message, ", unresolvable ");
        appendNumber(message, stats.unresolvableCount, FALSE);
        logToConsole(message);
    }
};

GLOBAL_VAR_DECL OracleEngine<computorSeedsCount> oracleEngine;

/*
- Handle seamless transitions? Keep state?

*/
