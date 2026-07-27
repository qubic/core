#pragma once

#include "contract_core/pre_qpi_def.h"
#include "contracts/qpi.h"
#include "oc_core/oc_interfaces_def.h"

#include "system.h"
#include "platform/memory_util.h"

#include "oc_transactions.h"
#include "core_oc_network_messages.h"

// Self-contained K12 wrapper; K12/kangaroo_twelve_xkcp.h's external-linkage
// XKCP symbols would clash at link with other TUs defining them.
#include "kangaroo_twelve.h"


void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);


// Maximum OC invocations recorded per epoch (power of 2, required by invocationIdToIndex).
constexpr uint32_t MAX_OC_INVOCATIONS_PER_EPOCH = (1 << 21);

// Maximum size of a pinned OcRequest payload.
constexpr uint16_t MAX_OC_REQUEST_SIZE = MAX_INPUT_SIZE - 16;

// Byte budget for pinned request storage, assuming 256 bytes average request size.
// startContractInvocation rejects when either this or the record cap is exhausted.
constexpr uint64_t OC_REQUEST_STORAGE_SIZE = 256ULL * MAX_OC_INVOCATIONS_PER_EPOCH;

// Minimum invocation fee an interface may return.
constexpr int64_t MIN_OC_INVOCATION_FEE = 10;

// Maximum ticks an invocation may stay PENDING_AUTH before timing out.
// Leaves room for two auth-tx retry rounds (see OC_AUTH_RESCHEDULE_TICKS).
constexpr uint32_t OC_INVOCATION_TIMEOUT_DEFAULT_TICKS = 12;

// Minimum ticks between this node's auth-tx emissions for the same invocation.
// Must exceed OC_AUTH_SIGNATURE_PUBLICATION_OFFSET so a retry cannot duplicate
// a tx that may still execute.
constexpr uint32_t OC_AUTH_RESCHEDULE_TICKS = 4;

// Maximum invocations in flight (PENDING_AUTH, or AUTHORIZED but undelivered) at once.
// Every invocation holds a slot from creation, so this is deterministic admission
// control: slot state derives purely from tick processing, all nodes reject alike.
constexpr uint32_t MAX_OC_IN_FLIGHT_INVOCATIONS = 1024;

// Sentinel: record holds no in-flight slot (resolved, or discarded after delivery).
constexpr uint32_t OC_IN_FLIGHT_SLOT_NONE = 0xFFFFFFFFu;


// Consensus status values (OC_INVOCATION_STATUS_*) are defined in
// network_messages/common_def.h so contracts can see them.


// Per-invocation record, kept for the whole epoch. Slim (64 bytes): per-computor
// auth tracking lives in the in-flight pool (inFlightSlot), only while unresolved.
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
    unsigned char delivered;            // 1 = OcMachineInvocation enqueued. Node-local: reset to 0 on snapshot load
};

static_assert(sizeof(OcInvocationRecord) == 64, "OcInvocationRecord must stay 64 bytes — it is allocated MAX_OC_INVOCATIONS_PER_EPOCH times.");


// Auth tracking state, alive only while the invocation is in flight (creation
// until timeout or delivery). signatures/signerIndices grow in lockstep.
struct OcInFlightAuthState
{
    unsigned char signatures[QUORUM][SIGNATURE_SIZE]; // first QUORUM accepted signatures
    unsigned short signerIndices[QUORUM];             // computor index in broadcastedComputors for each
    unsigned short agreeingSigs;                      // count of distinct valid signatures observed
    unsigned int lastScheduledTick;                   // tick this node last emitted auth items for this slot; 0 = never. Node-local retry pacing, not consensus state.
    unsigned char signedBy[(NUMBER_OF_COMPUTORS + 7) / 8]; // bitmap; computor's sig tx has executed and been counted
    unsigned char padding[3];                         // explicit tail padding — keeps sizeof stable regardless of compiler packing
};

static_assert(sizeof(OcInFlightAuthState) == QUORUM * (SIGNATURE_SIZE + sizeof(unsigned short)) + sizeof(unsigned short) + sizeof(unsigned int) + (NUMBER_OF_COMPUTORS + 7) / 8 + 3,
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

    /// index of the first record that may still be unresolved; per-tick scans start
    /// here instead of at 0. Node-local optimization, not consensus state.
    uint32_t firstActiveIndex;

    /// buffer continuously filled with pinned OcRequest payloads
    uint8_t* requestStorage;

    /// how many bytes of requestStorage are already in use / offset for adding new data
    uint64_t requestStorageBytesUsed;

    /// pool of in-flight auth states; record.inFlightSlot indexes into inFlightStates[]
    OcInFlightAuthState* inFlightStates;

    /// next slot to consider when looking for an empty in-flight slot (cyclic)
    uint32_t inFlightSlotCursor;

    /// per-slot bookkeeping: which invocation owns the slot, or -1 if free.
    /// Sized as int64_t to hold an invocationId; negative => free slot.
    int64_t* inFlightSlotOwners;

    /// state for assigning invocation IDs
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
    /// Impl in oc_core/snapshot_files.h.
    bool saveSnapshot(unsigned short epoch, CHAR16* directory) const;

    /// Load state from snapshot files. Can only be called from main processor.
    /// Impl in oc_core/snapshot_files.h.
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
        // until resolution; rejecting here is deterministic across nodes and refundable
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

        logger.logOcInvocationStatusChange({ rec.invocationId, rec.contractIndex, rec.interfaceIndex, rec.status });

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
    static void refundFees(const m256i& contractId, int64_t refundAmount)
    {
        ASSERT(refundAmount >= 0);
        increaseEnergy(contractId, refundAmount);
        const QuTransfer quTransfer = { m256i::zero(), contractId, refundAmount };
        logger.logQuTransfer(quTransfer);
    }

    /// Return consensus status for an invocation, or OC_INVOCATION_STATUS_UNKNOWN if not found.
    uint8_t getOcInvocationStatus(int64_t invocationId) const
    {
        LockGuard lockGuard(lock);
        uint32_t idx;
        if (!invocationIdToIndex->get(invocationId, idx) || idx >= invocationCount)
            return OC_INVOCATION_STATUS_UNKNOWN;
        return invocations[idx].status;
    }

    /**
    * Build an OcAuthSignatureTransaction batching items for invocations this computor has not
    * yet scheduled. Fills everything EXCEPT per-item signatures and the outer tx signature;
    * the caller must sign both.
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
            // skip if emitted within the reschedule window; stamp == system.tick is this
            // tick's own round (later own computors / continuation calls still add items)
            if (auth.lastScheduledTick != 0 && auth.lastScheduledTick != system.tick
                && system.tick - auth.lastScheduledTick < OC_AUTH_RESCHEDULE_TICKS)
                continue;
            // skip if this computor's signature already executed on-chain
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

            // suppress re-emit until the reschedule window elapses
            auth.lastScheduledTick = system.tick;
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
    * Enqueue OcMachineInvocation messages for all AUTHORIZED records not yet delivered,
    * then mark them delivered (node-local) and free their in-flight slots.
    *
    * Called once per tick from the tick processor. MUST run unconditionally on every node
    * (even without OC machine peers): freeing the in-flight slot here is part of the
    * deterministic pool state that admission control depends on.
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
                // auth state was discarded; mark delivered so we don't retry forever
                rec.delivered = 1;
                continue;
            }
            const OcInFlightAuthState& auth = inFlightStates[rec.inFlightSlot];
            ASSERT(auth.agreeingSigs == QUORUM);

            // Assemble message in deliveryBuffer; on nodes with no OC machine peers,
            // pushToOcMachineNodes drops the enqueued message.
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

            // Peer sentinel (Peer*)2 routes to pushToOcMachineNodes in the dispatcher.
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

            logger.logOcInvocationStatusChange({ rec.invocationId, rec.contractIndex, rec.interfaceIndex, rec.status });

            // free the in-flight slot (held since creation)
            if (rec.inFlightSlot != OC_IN_FLIGHT_SLOT_NONE)
            {
                freeInFlightSlot(rec.inFlightSlot);
                rec.inFlightSlot = OC_IN_FLIGHT_SLOT_NONE;
            }
        }
        advanceFirstActiveIndex();
    }

    /// Print a one-line OC engine status summary. Called from the main loop's logInfo path.
    void logStatus() const
    {
        LockGuard lockGuard(lock);

        // Scan only the active window; resolved-prefix counts come from the stats counters.
        uint32_t pending = 0, timeoutActive = 0, delivered = 0;
        for (uint32_t i = firstActiveIndex; i < invocationCount; ++i)
        {
            const OcInvocationRecord& rec = invocations[i];
            if (rec.status == OC_INVOCATION_STATUS_PENDING_AUTH)
                ++pending;
            else if (rec.status == OC_INVOCATION_STATUS_TIMEOUT)
                ++timeoutActive;
            if (rec.delivered)
                ++delivered;
        }
        const uint32_t authorized = (uint32_t)stats.authorizedCount;
        const uint32_t timeout = (uint32_t)stats.timeoutCount;
        // resolved prefix = timeouts + delivered AUTHORIZED records
        delivered += firstActiveIndex - (timeout - timeoutActive);

        // In-flight pool occupancy (slots with a live owner).
        uint32_t inFlightSlotsUsed = 0;
        for (uint32_t i = 0; i < MAX_OC_IN_FLIGHT_INVOCATIONS; ++i)
        {
            if (inFlightSlotOwners[i] >= 0)
                ++inFlightSlotsUsed;
        }

        setText(::message, L"OC invocations: total ");
        appendNumber(::message, invocationCount, FALSE);
        appendText(::message, L" (pending ");
        appendNumber(::message, pending, FALSE);
        appendText(::message, L", authorized ");
        appendNumber(::message, authorized, FALSE);
        appendText(::message, L", timeout ");
        appendNumber(::message, timeout, FALSE);
        appendText(::message, L", delivered ");
        appendNumber(::message, delivered, FALSE);
        appendText(::message, L"); pool-full rejects ");
        appendNumber(::message, stats.poolExhaustedRejectCount, FALSE);
        appendText(::message, L"; record slots ");
        appendNumber(::message, invocationCount * 100 / MAX_OC_INVOCATIONS_PER_EPOCH, FALSE);
        appendText(::message, L"%, in-flight slots ");
        appendNumber(::message, inFlightSlotsUsed * 100 / MAX_OC_IN_FLIGHT_INVOCATIONS, FALSE);
        appendText(::message, L"%, request storage ");
        appendNumber(::message, (unsigned long long)(requestStorageBytesUsed * 100 / OC_REQUEST_STORAGE_SIZE), FALSE);
        appendText(::message, L"%");
        logToConsole(::message);
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

        // resolve source pubkey to computor index in the current epoch's broadcastedComputors
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

            // step 5+6: recompute auth message from record-authoritative fields & verify signature
            m256i authHash;
            computeOcAuthMessageHash(rec.epoch, rec.interfaceIndex, rec.invocationId, rec.paramsDigest, authHash);
            if (!verify((const unsigned char*)&transaction->sourcePublicKey, (const unsigned char*)&authHash, item.signature))
                continue;

            // step 7: record signature + signer index, set bitmap
            copyMem(auth.signatures[auth.agreeingSigs], item.signature, SIGNATURE_SIZE);
            auth.signerIndices[auth.agreeingSigs] = (unsigned short)compIdx;
            auth.signedBy[byteIdx] |= bitMask;
            ++auth.agreeingSigs;

            // step 8: transition to AUTHORIZED at the QUORUM threshold; the in-flight
            // slot is kept until deliverAuthorizedInvocations frees it
            if (auth.agreeingSigs >= QUORUM)
            {
                rec.status = OC_INVOCATION_STATUS_AUTHORIZED;
                ++stats.authorizedCount;

                logger.logOcInvocationStatusChange({ rec.invocationId, rec.contractIndex, rec.interfaceIndex, rec.status });
            }
        }

        return true;
    }

protected:
    /// Find a free in-flight slot or return OC_IN_FLIGHT_SLOT_NONE.
    /// Caller must hold the engine lock.
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

    /// Advance firstActiveIndex past fully resolved records (TIMEOUT, or AUTHORIZED
    /// and delivered). Caller must hold the engine lock.
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
