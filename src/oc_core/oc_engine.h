#pragma once

#include "contract_core/pre_qpi_def.h"
#include "contracts/qpi.h"
#include "oc_core/oc_interfaces_def.h"

#include "system.h"
#include "platform/memory_util.h"

#include "oc_transactions.h"
#include "core_oc_network_messages.h"

// Only the one-shot KangarooTwelve() is used below (computeOcAuthMessageHash,
// paramsDigest). Pull in the self-contained wrapper (static/internal linkage)
// rather than K12/kangaroo_twelve_xkcp.h, whose external-linkage XKCP backend
// symbols would clash at link if another TU (e.g. test/kangaroo_twelve.cpp)
// also defines them.
#include "kangaroo_twelve.h"


void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);


// Maximum number of OC invocations that may be recorded in a single epoch.
// Mirrors OM's MAX_ORACLE_QUERIES (must be a power of 2 for invocationIdToIndex).
constexpr uint32_t MAX_OC_INVOCATIONS_PER_EPOCH = (1 << 21);

// Maximum size of pinned OcRequest payload, mirrors MAX_ORACLE_QUERY_SIZE.
constexpr uint16_t MAX_OC_REQUEST_SIZE = MAX_INPUT_SIZE - 16;

// Total bytes reserved for per-invocation pinned request storage. Provisioned as a
// total byte budget assuming an average request size of 256 bytes (mirrors OM's
// ORACLE_QUERY_STORAGE_SIZE), NOT as MAX_OC_REQUEST_SIZE * capacity. Interfaces with
// large fixed request sizes exhaust the byte budget before the record-count cap;
// startContractInvocation rejects on either limit.
constexpr uint64_t OC_REQUEST_STORAGE_SIZE = 256ULL * MAX_OC_INVOCATIONS_PER_EPOCH;

// Minimum invocation fee an interface may return.
constexpr int64_t MIN_OC_INVOCATION_FEE = 10;

// Maximum number of ticks an invocation may remain in PENDING_AUTH before timing out.
constexpr uint32_t OC_INVOCATION_TIMEOUT_DEFAULT_TICKS = 3;

// Maximum number of invocations that may be in flight (PENDING_AUTH, or AUTHORIZED but
// not yet delivered) at any moment. Bounds the heavy per-computor auth-tracking state
// the same way OM's MAX_SIMULTANEOUS_ORACLE_QUERIES bounds OracleReplyState: it exists
// only while an invocation is unresolved (at most OC_INVOCATION_TIMEOUT_DEFAULT_TICKS
// plus the delivery tick), never for the whole epoch. Because every invocation needs a
// slot from creation, this also acts as deterministic admission control: slot state is
// derived purely from tick processing, so all nodes reject the same invocation when
// the pool is exhausted.
constexpr uint32_t MAX_OC_IN_FLIGHT_INVOCATIONS = 1024;

// Sentinel meaning "this record holds no in-flight slot" (invocation resolved, or
// auth state discarded after delivery).
constexpr uint32_t OC_IN_FLIGHT_SLOT_NONE = 0xFFFFFFFFu;


// Consensus status values (OC_INVOCATION_STATUS_*) are defined in network_messages/common_def.h
// so contracts can see them, mirroring ORACLE_QUERY_STATUS_*.


// Per-invocation engine record, kept for the whole epoch. Deliberately slim (64 bytes):
// all per-computor tracking (signature bundle, signedBy/scheduledBy bitmaps) lives in
// the bounded in-flight pool referenced by inFlightSlot, and only while the invocation
// is unresolved — the same split OM uses between OracleQueryMetadata and
// OracleReplyState.
struct OcInvocationRecord
{
    long long invocationId;
    m256i paramsDigest;                 // K12 over pinned OcRequest bytes
    unsigned int paramsOffset;          // offset into requestStorage
    unsigned int creationTick;
    unsigned int inFlightSlot;          // index into inFlightStates, or OC_IN_FLIGHT_SLOT_NONE once resolved
    unsigned short epoch;
    unsigned short contractIndex;
    unsigned short interfaceIndex;
    unsigned short paramsSize;          // equals OCI::ocInterfaces[interfaceIndex].requestSize
    unsigned char status;               // one of OC_INVOCATION_STATUS_*
    unsigned char delivered;            // 1 = OcMachineInvocation has been enqueued for delivery. Node-local: written to the snapshot as part of the record but reset to 0 on load (not consensus state).
};

static_assert(sizeof(OcInvocationRecord) == 64, "OcInvocationRecord must stay 64 bytes — it is allocated MAX_OC_INVOCATIONS_PER_EPOCH times.");


// Per-invocation authorization tracking state, alive only while the invocation is in
// flight (creation until timeout or delivery). Holds the accumulating signature bundle
// plus the per-computor bitmaps. signatures/signerIndices grow in lockstep — entry i
// is filled when the (i+1)-th valid signature arrives.
struct OcInFlightAuthState
{
    unsigned char signatures[QUORUM][SIGNATURE_SIZE]; // first QUORUM accepted signatures
    unsigned short signerIndices[QUORUM];             // computor index in broadcastedComputors for each
    unsigned char signedBy[(NUMBER_OF_COMPUTORS + 7) / 8];    // bitmap; computor's sig tx has executed and been counted
    unsigned char scheduledBy[(NUMBER_OF_COMPUTORS + 7) / 8]; // bitmap; computor has queued an OcAuthSignatureTransaction
    unsigned short agreeingSigs;                      // count of distinct valid signatures observed
};

static_assert(sizeof(OcInFlightAuthState) == QUORUM * (SIGNATURE_SIZE + sizeof(unsigned short)) + 2 * ((NUMBER_OF_COMPUTORS + 7) / 8) + sizeof(unsigned short),
    "OcInFlightAuthState size mismatch.");


// Statistics for diagnostics.
struct OcEngineStatistics
{
    unsigned long long invocationCount;
    unsigned long long authorizedCount;
    unsigned long long timeoutCount;
    unsigned long long poolExhaustedRejectCount;
};


class OcEngine
{
protected:
    /// array of all invocation records of the epoch (capacity MAX_OC_INVOCATIONS_PER_EPOCH)
    OcInvocationRecord* invocations;

    /// number of slots used in invocations array
    uint32_t invocationCount;

    /// index of the first record that may still be unresolved (PENDING_AUTH, or
    /// AUTHORIZED and not yet delivered). Records resolve within a few ticks of
    /// creation (fixed timeout), so per-tick scans start here instead of at 0 —
    /// without this, processTimeouts/deliverAuthorizedInvocations/
    /// getAuthSignatureTransaction degrade to O(epoch total) per tick.
    /// Node-local scan optimization; never part of consensus state or snapshots.
    uint32_t firstActiveIndex;

    /// buffer continuously filled with pinned OcRequest payloads
    uint8_t* requestStorage;

    /// how many bytes of requestStorage are already in use / offset for adding new data
    uint64_t requestStorageBytesUsed;

    /// pool of in-flight auth states; record.inFlightSlot indexes into inFlightStates[]
    OcInFlightAuthState* inFlightStates;

    /// next slot to consider when looking for an empty in-flight slot (cyclic; mirrors OM's replyStatesIndex)
    uint32_t inFlightSlotCursor;

    /// per-slot bookkeeping: which invocation owns the slot, or -1 if free.
    /// Sized as int64_t to hold an invocationId; negative => free slot.
    int64_t* inFlightSlotOwners;

    /// state for assigning invocation IDs (mirrors OracleEngine::contractQueryIdState)
    struct
    {
        uint32_t tick;
        uint32_t indexInTick;
    } invocationIdState;

    /// fast lookup of invocation index by invocationId
    QPI::HashMap<int64_t, uint32_t, MAX_OC_INVOCATIONS_PER_EPOCH>* invocationIdToIndex;

    /// pointer to global array of 676 computor public keys
    const m256i* computorPublicKeys;

    /// statistics
    OcEngineStatistics stats;

    /// buffer used to assemble OcMachineInvocation messages before enqueueing.
    /// Sized for the maximum message: header + max request + QUORUM SignerEntries.
    uint8_t deliveryBuffer[sizeof(OcMachineInvocation) + MAX_OC_REQUEST_SIZE + QUORUM * sizeof(SignerEntry)];

    /// lock for preventing race conditions in concurrent execution
    mutable volatile char lock;

public:
    /// Compute K12 hash of the canonical OC authorization message.
    /// SOLE serializer used by both per-computor signing and engine signature verification.
    static void computeOcAuthMessageHash(
        unsigned short epoch,
        unsigned short interfaceIndex,
        long long invocationId,
        const m256i& paramsDigest,
        m256i& outHash)
    {
        OcAuthMessageBytes msg;
        copyMem(msg.domainSeparator, OC_AUTH_DOMAIN_SEPARATOR, OC_AUTH_DOMAIN_SEPARATOR_SIZE);
        msg.epoch = epoch;
        msg.interfaceIndex = interfaceIndex;
        msg.invocationId = invocationId;
        msg.paramsDigest = paramsDigest;
        KangarooTwelve((const unsigned char*)&msg, sizeof(msg), (unsigned char*)&outHash, 32);
    }

    /// Compute K12 hash over the pinned OcRequest bytes.
    /// Caller is responsible for zero-initializing requestData before populating fields.
    static void computeOcRequestDigest(const void* requestData, unsigned int requestSize, m256i& outDigest)
    {
        KangarooTwelve((const unsigned char*)requestData, requestSize, (unsigned char*)&outDigest, 32);
    }

    /// Initialize object, passing array of computor public keys.
    bool init(const m256i* computorPublicKeys)
    {
        this->computorPublicKeys = computorPublicKeys;
        lock = 0;

        if (!allocPoolWithErrorLog(L"OcEngine::invocations", MAX_OC_INVOCATIONS_PER_EPOCH * sizeof(*invocations), (void**)&invocations, __LINE__)
            || !allocPoolWithErrorLog(L"OcEngine::requestStorage", OC_REQUEST_STORAGE_SIZE, (void**)&requestStorage, __LINE__)
            || !allocPoolWithErrorLog(L"OcEngine::inFlightStates", MAX_OC_IN_FLIGHT_INVOCATIONS * sizeof(*inFlightStates), (void**)&inFlightStates, __LINE__)
            || !allocPoolWithErrorLog(L"OcEngine::inFlightSlotOwners", MAX_OC_IN_FLIGHT_INVOCATIONS * sizeof(*inFlightSlotOwners), (void**)&inFlightSlotOwners, __LINE__)
            || !allocPoolWithErrorLog(L"OcEngine::invocationIdToIndex", sizeof(*invocationIdToIndex), (void**)&invocationIdToIndex, __LINE__))
        {
            return false;
        }

        reset();
        return true;
    }

    /// Delete all invocations and statistics.
    void reset()
    {
        ASSERT(invocations && requestStorage && inFlightStates && inFlightSlotOwners && invocationIdToIndex);
        setMem(invocations, MAX_OC_INVOCATIONS_PER_EPOCH * sizeof(*invocations), 0);
        setMem(requestStorage, OC_REQUEST_STORAGE_SIZE, 0);
        setMem(inFlightStates, MAX_OC_IN_FLIGHT_INVOCATIONS * sizeof(*inFlightStates), 0);
        for (uint32_t i = 0; i < MAX_OC_IN_FLIGHT_INVOCATIONS; ++i)
            inFlightSlotOwners[i] = -1;
        invocationIdToIndex->reset();

        invocationCount = 0;
        firstActiveIndex = 0;
        requestStorageBytesUsed = 0;
        inFlightSlotCursor = 0;
        setMem(&invocationIdState, sizeof(invocationIdState), 0);
        setMem(&stats, sizeof(stats), 0);
    }

    void deinit()
    {
        if (invocations)
            freePool(invocations);
        if (requestStorage)
            freePool(requestStorage);
        if (inFlightStates)
            freePool(inFlightStates);
        if (inFlightSlotOwners)
            freePool(inFlightSlotOwners);
        if (invocationIdToIndex)
            freePool(invocationIdToIndex);
    }

    /// Drop all invocations of the previous epoch.
    void beginEpoch()
    {
        LockGuard lockGuard(lock);
        reset();
    }

    /// Save current state to snapshot files. Can only be called from main processor.
    /// Impl in oc_core/snapshot_files.h, matching OM split.
    bool saveSnapshot(unsigned short epoch, CHAR16* directory) const;

    /// Load state from snapshot files. Can only be called from main processor.
    /// Impl in oc_core/snapshot_files.h, matching OM split.
    bool loadSnapshot(unsigned short epoch, CHAR16* directory);

    /// Record a new invocation from a contract; returns invocationId or -1 on error.
    int64_t startContractInvocation(uint16_t contractIndex, uint16_t interfaceIndex,
        const void* requestData, uint16_t requestSize)
    {
        // check inputs
        if (contractIndex >= MAX_NUMBER_OF_CONTRACTS
            || interfaceIndex >= OCI::ocInterfacesCount
            || requestSize != OCI::ocInterfaces[interfaceIndex].requestSize)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start OC invocation due to contractIndex / interfaceIndex / requestSize issue!");
#endif
            return -1;
        }

        // lock for accessing engine data
        LockGuard lockGuard(lock);

        // check capacity: record slots and pinned-request byte budget
        if (invocationCount >= MAX_OC_INVOCATIONS_PER_EPOCH
            || requestStorageBytesUsed + requestSize > OC_REQUEST_STORAGE_SIZE)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start OC invocation due to engine storage exhaustion!");
#endif
            return -1;
        }

        // get new invocation ID
        const int64_t invocationId = getNewInvocationId();
        if (invocationId < 0)
            return -1;

        // admission control: every invocation holds an in-flight slot from creation
        // until resolution. Rejecting here (instead of accepting and silently dropping
        // signatures later) keeps the outcome deterministic across nodes and refundable
        // for the calling contract.
        const uint32_t slot = allocateInFlightSlot(invocationId);
        if (slot == OC_IN_FLIGHT_SLOT_NONE)
        {
            ++stats.poolExhaustedRejectCount;
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start OC invocation: in-flight pool exhausted!");
#endif
            return -1;
        }

        // map ID to index
        ASSERT(!invocationIdToIndex->contains(invocationId));
        if (invocationIdToIndex->set(invocationId, invocationCount) == QPI::NULL_INDEX)
        {
            freeInFlightSlot(slot);
#if !defined(NDEBUG) && !defined(NO_UEFI)
            addDebugMessage(L"Cannot start OC invocation due to invocationIdToIndex issue!");
#endif
            return -1;
        }

        // reserve and zero-init pinned request storage
        const uint32_t paramsOffset = (uint32_t)requestStorageBytesUsed;
        setMem(requestStorage + paramsOffset, requestSize, 0);

        // copy request bytes
        copyMem(requestStorage + paramsOffset, requestData, requestSize);
        requestStorageBytesUsed += requestSize;

        // compute paramsDigest over pinned bytes
        m256i paramsDigest;
        computeOcRequestDigest(requestStorage + paramsOffset, requestSize, paramsDigest);

        // initialize record
        const uint32_t slotIdx = invocationCount++;
        OcInvocationRecord& rec = invocations[slotIdx];
        setMem(&rec, sizeof(rec), 0);
        rec.invocationId = invocationId;
        rec.epoch = system.epoch;
        rec.contractIndex = contractIndex;
        rec.interfaceIndex = interfaceIndex;
        rec.paramsSize = requestSize;
        rec.paramsOffset = paramsOffset;
        rec.paramsDigest = paramsDigest;
        rec.creationTick = system.tick;
        rec.inFlightSlot = slot;
        rec.status = OC_INVOCATION_STATUS_PENDING_AUTH;

        ++stats.invocationCount;

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsg[200];
        setText(dbgMsg, L"ocEngine.startContractInvocation(), tick ");
        appendNumber(dbgMsg, system.tick, FALSE);
        appendText(dbgMsg, L", invocationId ");
        appendNumber(dbgMsg, invocationId, FALSE);
        appendText(dbgMsg, L", interfaceIndex ");
        appendNumber(dbgMsg, interfaceIndex, FALSE);
        addDebugMessage(dbgMsg);
#endif

        return invocationId;
    }

    /// Refund a fee to the contract when startContractInvocation fails after fee deduction.
    /// Mirrors OracleEngine::refundFees.
    static void refundFees(const m256i& contractId, int64_t refundAmount)
    {
        ASSERT(refundAmount >= 0);
        increaseEnergy(contractId, refundAmount);
        const QuTransfer quTransfer = { m256i::zero(), contractId, refundAmount };
        logger.logQuTransfer(quTransfer);
    }

    /// Return consensus status for an invocation, or OC_INVOCATION_STATUS_UNKNOWN if not found.
    /// Mirrors OracleEngine::getOracleQueryStatus.
    uint8_t getOcInvocationStatus(int64_t invocationId) const
    {
        LockGuard lockGuard(lock);
        uint32_t idx;
        if (!invocationIdToIndex->get(invocationId, idx) || idx >= invocationCount)
            return OC_INVOCATION_STATUS_UNKNOWN;
        return invocations[idx].status;
    }

    /**
    * Build an OcAuthSignatureTransaction batching items for invocations this computor has not yet
    * scheduled. Fills the tx prefix and items with everything EXCEPT per-item signatures and the
    * outer tx signature; the caller must sign each item (via computeOcAuthMessageHash + sign) and
    * the outer tx as usual.
    *
    * Mirrors OracleEngine::getReplyCommitTransaction shape: returns 0 if no items are pending,
    * UINT32_MAX if all pending items fit in a single tx, otherwise a continuation index for the
    * next call (which produces a second tx batching the remaining items).
    *
    * @param txBuffer Caller-provided buffer of at least MAX_TRANSACTION_SIZE bytes.
    * @param computorIdx Global computor slot the tx is being prepared for.
    * @param txScheduleTick The tick at which the tx is meant to land.
    * @param startIdx Continuation index from a previous call, or 0 for the first call.
    * @return 0 (no tx needed), UINT32_MAX (done), or continuation index for next call.
    */
    uint32_t getAuthSignatureTransaction(void* txBuffer, uint16_t computorIdx,
        uint32_t txScheduleTick, uint32_t startIdx = 0)
    {
        ASSERT(txBuffer);
        if (computorIdx >= NUMBER_OF_COMPUTORS || txScheduleTick <= system.tick)
            return 0;

        auto* tx = reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuffer);
        unsigned char* payload = reinterpret_cast<unsigned char*>(tx) + sizeof(Transaction);
        unsigned short* itemCountPtr = reinterpret_cast<unsigned short*>(payload);
        unsigned short* paddingPtr = reinterpret_cast<unsigned short*>(payload + sizeof(unsigned short));
        auto* items = reinterpret_cast<OcAuthSignatureItem*>(payload + 2 * sizeof(unsigned short));

        unsigned short itemsAdded = 0;
        constexpr unsigned short maxItems = OcAuthSignatureTransactionPrefix::maxItemCount();

        LockGuard lockGuard(lock);

        const unsigned int byteIdx = (unsigned int)computorIdx >> 3;
        const unsigned char bitMask = (unsigned char)(1u << (computorIdx & 7));

        // records before firstActiveIndex are resolved (never PENDING_AUTH), skip them
        uint32_t idx = (startIdx < firstActiveIndex) ? firstActiveIndex : startIdx;
        for (; idx < invocationCount; ++idx)
        {
            OcInvocationRecord& rec = invocations[idx];
            if (rec.status != OC_INVOCATION_STATUS_PENDING_AUTH)
                continue;
            ASSERT(rec.inFlightSlot < MAX_OC_IN_FLIGHT_INVOCATIONS);
            OcInFlightAuthState& auth = inFlightStates[rec.inFlightSlot];
            // skip if this computor has already scheduled or signed for this invocation
            if (auth.scheduledBy[byteIdx] & bitMask)
                continue;
            if (auth.signedBy[byteIdx] & bitMask)
                continue;

            // batch limit reached -> emit this tx now and continue next call
            if (itemsAdded == maxItems)
                break;

            OcAuthSignatureItem& item = items[itemsAdded];
            setMem(&item, sizeof(item), 0);
            item.invocationId = rec.invocationId;
            item.interfaceIndex = rec.interfaceIndex;
            item.epoch = rec.epoch;
            item.paramsDigest = rec.paramsDigest;
            // item.signature is left zeroed; caller MUST sign before broadcast.

            // mark scheduled so we don't re-emit until the tx executes (which will set signedBy[c])
            auth.scheduledBy[byteIdx] |= bitMask;
            ++itemsAdded;
        }

        if (!itemsAdded)
            return 0;

        // fill tx prefix
        *itemCountPtr = itemsAdded;
        *paddingPtr = 0;
        tx->sourcePublicKey = computorPublicKeys[computorIdx];
        tx->destinationPublicKey = m256i::zero();
        tx->amount = 0;
        tx->tick = txScheduleTick;
        tx->inputType = OcAuthSignatureTransactionPrefix::transactionType();
        tx->inputSize = (unsigned short)(2 * sizeof(unsigned short) + (unsigned int)itemsAdded * sizeof(OcAuthSignatureItem));

        if (idx < invocationCount)
            return idx;
        return UINT32_MAX;
    }

    /**
    * Enqueue OcMachineInvocation messages for any AUTHORIZED records that have not yet been
    * delivered. Each call processes all undelivered records.
    *
    * Once enqueued, the record is marked delivered (node-local; not in snapshot) and its
    * in-flight slot is freed per the discard-after-delivery optimization.
    *
    * Called once per tick from the tick processor (after processOcAuthSignatureTransaction has
    * had a chance to flip records to AUTHORIZED in the current tick). MUST run unconditionally
    * on every node (not only on nodes with OC machine peers configured): freeing the in-flight
    * slot here is part of the deterministic pool state that admission control depends on.
    */
    void deliverAuthorizedInvocations()
    {
        LockGuard lockGuard(lock);
        for (uint32_t i = firstActiveIndex; i < invocationCount; ++i)
        {
            OcInvocationRecord& rec = invocations[i];
            if (rec.status != OC_INVOCATION_STATUS_AUTHORIZED)
                continue;
            if (rec.delivered)
                continue;
            if (rec.inFlightSlot == OC_IN_FLIGHT_SLOT_NONE)
            {
                // auth state was discarded (e.g. after snapshot restore where the slot wasn't
                // kept); mark delivered so we don't retry forever
                rec.delivered = 1;
                continue;
            }
            const OcInFlightAuthState& auth = inFlightStates[rec.inFlightSlot];
            ASSERT(auth.agreeingSigs == QUORUM);

            // Assemble message in deliveryBuffer. Assembled unconditionally on every node
            // (mirroring OracleEngine's OM query dispatch); on nodes with no OC machine peers
            // configured, pushToOcMachineNodes is a no-op that drops the enqueued message.
            // Freeing the in-flight slot below is deterministic pool state and must run on
            // every node regardless of peer configuration.
            auto* msg = reinterpret_cast<OcMachineInvocation*>(deliveryBuffer);
            msg->invocationId = rec.invocationId;
            msg->epoch = rec.epoch;
            msg->interfaceIndex = rec.interfaceIndex;
            msg->requestSize = rec.paramsSize;
            msg->signatureCount = QUORUM;

            // Append pinned request bytes
            uint8_t* cursor = deliveryBuffer + sizeof(OcMachineInvocation);
            copyMem(cursor, requestStorage + rec.paramsOffset, rec.paramsSize);
            cursor += rec.paramsSize;

            // Append signer entries
            for (unsigned short s = 0; s < QUORUM; ++s)
            {
                auto* entry = reinterpret_cast<SignerEntry*>(cursor);
                entry->computorIndex = auth.signerIndices[s];
                copyMem(entry->signature, auth.signatures[s], SIGNATURE_SIZE);
                cursor += sizeof(SignerEntry);
            }

            const unsigned int payloadSize = (unsigned int)(cursor - deliveryBuffer);

            // Enqueue to all configured OC machine peers. Peer sentinel (Peer*)2 routes to
            // pushToOcMachineNodes in the response-queue dispatcher ((Peer*)1 is the OM sentinel).
            enqueueResponse((Peer*)2, payloadSize, OcMachineInvocation::type(), 0, deliveryBuffer);

            // Mark delivered and discard auth state
            rec.delivered = 1;
            freeInFlightSlot(rec.inFlightSlot);
            rec.inFlightSlot = OC_IN_FLIGHT_SLOT_NONE;
        }
        advanceFirstActiveIndex();
    }

    /// Time out invocations that have been PENDING_AUTH for too long.
    /// Called once per tick from the tick processor.
    void processTimeouts()
    {
        LockGuard lockGuard(lock);
        const uint32_t currentTick = system.tick;
        for (uint32_t i = firstActiveIndex; i < invocationCount; ++i)
        {
            OcInvocationRecord& rec = invocations[i];
            if (rec.status != OC_INVOCATION_STATUS_PENDING_AUTH)
                continue;
            if (currentTick - rec.creationTick < OC_INVOCATION_TIMEOUT_DEFAULT_TICKS)
                continue;

            rec.status = OC_INVOCATION_STATUS_TIMEOUT;
            ++stats.timeoutCount;

            // free the in-flight slot (held since creation)
            if (rec.inFlightSlot != OC_IN_FLIGHT_SLOT_NONE)
            {
                freeInFlightSlot(rec.inFlightSlot);
                rec.inFlightSlot = OC_IN_FLIGHT_SLOT_NONE;
            }
        }
        advanceFirstActiveIndex();
    }

    /// Process an incoming OcAuthSignatureTransaction.
    /// Called from tick processor when a tx with inputType==OcAuthSignatureTransactionPrefix::transactionType() executes.
    /// Returns true if the tx was structurally valid (regardless of how many items survived processing).
    bool processOcAuthSignatureTransaction(const OcAuthSignatureTransactionPrefix* transaction)
    {
        ASSERT(transaction != nullptr);
        ASSERT(transaction->checkValidity());
        ASSERT(isZero(transaction->destinationPublicKey));
        ASSERT(transaction->inputType == OcAuthSignatureTransactionPrefix::transactionType());

        // structural validation
        if (transaction->inputSize < OcAuthSignatureTransactionPrefix::minInputSize())
            return false;

        const unsigned char* payload = transaction->inputPtr();
        const unsigned short itemCount = *reinterpret_cast<const unsigned short*>(payload);
        // payload[2..4) is the 16-bit padding, ignored.
        if (itemCount == 0 || itemCount > OcAuthSignatureTransactionPrefix::maxItemCount())
            return false;
        const unsigned int expectedInputSize = 2 * sizeof(unsigned short) + (unsigned int)itemCount * sizeof(OcAuthSignatureItem);
        if (transaction->inputSize != expectedInputSize)
            return false;

        // resolve source pubkey to computor index in the CURRENT epoch's broadcastedComputors.
        // Records from other epochs were dropped at beginEpoch, so any valid record
        // referenced by this tx has record.epoch == system.epoch.
        const int compIdx = computorIndex(transaction->sourcePublicKey);
        if (compIdx < 0)
            return false;

        LockGuard lockGuard(lock);

        const OcAuthSignatureItem* items = reinterpret_cast<const OcAuthSignatureItem*>(payload + 2 * sizeof(unsigned short));
        for (unsigned short i = 0; i < itemCount; ++i)
        {
            const OcAuthSignatureItem& item = items[i];

            // step 1: locate record, verify epoch match
            uint32_t idx;
            if (!invocationIdToIndex->get(item.invocationId, idx) || idx >= invocationCount)
                continue;
            OcInvocationRecord& rec = invocations[idx];
            if (rec.epoch != item.epoch)
                continue;

            // step 2: guard against post-completion appends
            if (rec.status != OC_INVOCATION_STATUS_PENDING_AUTH)
                continue;

            // step 3: verify interface & params match (computor must sign the same intent the record was created with)
            if (item.interfaceIndex != rec.interfaceIndex || item.paramsDigest != rec.paramsDigest)
                continue;

            // PENDING_AUTH records hold their in-flight slot from creation until resolution
            ASSERT(rec.inFlightSlot < MAX_OC_IN_FLIGHT_INVOCATIONS);
            OcInFlightAuthState& auth = inFlightStates[rec.inFlightSlot];
            if (auth.agreeingSigs >= QUORUM)
                continue;

            // step 4: dedup by computor index
            const unsigned int byteIdx = (unsigned int)compIdx >> 3;
            const unsigned char bitMask = (unsigned char)(1u << (compIdx & 7));
            if (auth.signedBy[byteIdx] & bitMask)
                continue;

            // step 5+6: recompute authMessage using record-authoritative fields & verify signature
            m256i authHash;
            computeOcAuthMessageHash(rec.epoch, rec.interfaceIndex, rec.invocationId, rec.paramsDigest, authHash);
            if (!verify((const unsigned char*)&transaction->sourcePublicKey, (const unsigned char*)&authHash, item.signature))
                continue;

            // step 7: record signature + signer index, set bitmap
            copyMem(auth.signatures[auth.agreeingSigs], item.signature, SIGNATURE_SIZE);
            auth.signerIndices[auth.agreeingSigs] = (unsigned short)compIdx;
            auth.signedBy[byteIdx] |= bitMask;
            ++auth.agreeingSigs;

            // step 8: atomic transition to AUTHORIZED at the QUORUM threshold.
            // Atomic w.r.t. additional signature processing because the engine lock is held;
            // step 2 above will reject subsequent items for this record (status != PENDING_AUTH).
            // The in-flight slot is kept until deliverAuthorizedInvocations frees it.
            if (auth.agreeingSigs >= QUORUM)
            {
                rec.status = OC_INVOCATION_STATUS_AUTHORIZED;
                ++stats.authorizedCount;
            }
        }

        return true;
    }

protected:
    /// Find a free in-flight slot or return OC_IN_FLIGHT_SLOT_NONE.
    /// Caller must hold the engine lock. Mirrors OracleEngine::getEmptyReplyStateSlot.
    uint32_t allocateInFlightSlot(int64_t ownerInvocationId)
    {
        ASSERT(inFlightSlotCursor < MAX_OC_IN_FLIGHT_INVOCATIONS);
        for (uint32_t i = 0; i < MAX_OC_IN_FLIGHT_INVOCATIONS; ++i)
        {
            if (inFlightSlotOwners[inFlightSlotCursor] < 0)
            {
                const uint32_t slot = inFlightSlotCursor;
                inFlightSlotOwners[slot] = ownerInvocationId;
                setMem(&inFlightStates[slot], sizeof(*inFlightStates), 0);
                if (++inFlightSlotCursor >= MAX_OC_IN_FLIGHT_INVOCATIONS)
                    inFlightSlotCursor = 0;
                return slot;
            }
            if (++inFlightSlotCursor >= MAX_OC_IN_FLIGHT_INVOCATIONS)
                inFlightSlotCursor = 0;
        }
        return OC_IN_FLIGHT_SLOT_NONE;
    }

    /// Free an in-flight slot (on timeout, or after delivery to all configured OC peers).
    /// Caller must hold the engine lock.
    void freeInFlightSlot(uint32_t slot)
    {
        ASSERT(slot < MAX_OC_IN_FLIGHT_INVOCATIONS);
        inFlightSlotOwners[slot] = -1;
        setMem(&inFlightStates[slot], sizeof(*inFlightStates), 0);
    }

    /// Advance firstActiveIndex past records that are fully resolved (TIMEOUT, or
    /// AUTHORIZED and delivered). Bounded because every record resolves within
    /// OC_INVOCATION_TIMEOUT_DEFAULT_TICKS + 1 ticks of creation.
    /// Caller must hold the engine lock.
    void advanceFirstActiveIndex()
    {
        while (firstActiveIndex < invocationCount)
        {
            const OcInvocationRecord& rec = invocations[firstActiveIndex];
            if (rec.status == OC_INVOCATION_STATUS_PENDING_AUTH)
                break;
            if (rec.status == OC_INVOCATION_STATUS_AUTHORIZED && !rec.delivered)
                break;
            ++firstActiveIndex;
        }
    }

    /// Get the next monotonic invocationId. Caller must hold the engine lock.
    int64_t getNewInvocationId()
    {
        auto& s = invocationIdState;
        if (s.tick < system.tick)
        {
            s.tick = system.tick;
            s.indexInTick = NUMBER_OF_TRANSACTIONS_PER_TICK;
        }
        else
        {
            if (s.indexInTick >= 0x7FFFFFFF)
                return -1;
            ++s.indexInTick;
        }

        int64_t invocationId = ((int64_t)system.tick << 31) | s.indexInTick;
        return invocationId;
    }
};


GLOBAL_VAR_DECL OcEngine ocEngine;
