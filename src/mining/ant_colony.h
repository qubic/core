#pragma once

#include "platform/assert.h"
#include "platform/concurrency.h"
#include "platform/m256.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
#include "kangaroo_twelve.h"
#include "platform/file_io.h"
#include "contract_core/pre_qpi_def.h"
#include "qpi/qpi.h"
#include "qpi/impl/qpi_hash_map_impl.h"
#include "public_settings.h"
#include "mining.h"
#include "trit_pack.h"

// (tickOffset, solutionIndexInTick), epoch-relative tick plus the solution transaction's index in tick
struct SolutionRef
{
    unsigned int tickOffset;          // RELATIVE to the epoch start, never an absolute system tick
    unsigned int solutionIndexInTick;

    bool operator==(const SolutionRef& other) const
    {
        return (tickOffset == other.tickOffset) && (solutionIndexInTick == other.solutionIndexInTick);
    }

    bool isRoot() const
    {
        return (tickOffset == 0) && (solutionIndexInTick == 0xFFFFFFFFu);
    }
};

// A root of all trees
static constexpr SolutionRef ROOT_REF = { 0u, 0xFFFFFFFFu };

// A solution is uniquely identified by (pubkey, parentRef, nonce).
struct AntDedupKey
{
    m256i pubkey;
    m256i nonce;
    SolutionRef parentRef;

    bool operator==(const AntDedupKey& other) const
    {
        return (pubkey == other.pubkey) && (nonce == other.nonce) && (parentRef == other.parentRef);
    }
};

struct AntSolutionRecord
{
    m256i pubkey;
    m256i nonce;
    SolutionRef parentRef;        // this solution's parent, or ROOT_REF
    SolutionRef selfRef;          // this solution's own address (RELATIVE tick inside)
    unsigned int score;           // error count, lower is better
    unsigned int anchorTick;      // ABSOLUTE. tick whose digest seeded the RNG; clock for the sibling floor
    unsigned int depth;           // a child of the root is depth 1; the root itself is never stored
    unsigned int childAnnHash;    // K12 of the canonical ANN at commit; digest-fold input
    unsigned int annStateSlot;    // index into the ANN pool; always equals the record index
    unsigned int nextSiblingIdx;  // next child of the same parent, NO_SIBLING terminates
};
static constexpr unsigned int NO_SIBLING = 0xFFFFFFFFu;
static constexpr unsigned int WORST_SCORE = 0xFFFFFFFFu;
static constexpr long long ANT_INVALID_INDEX = -1;
static_assert(sizeof(AntSolutionRecord) == 104, "AntSolutionRecord unexpected padding");

// ANN that will be saved for the epoch
template<typename PackedAnnT>
struct AntExportSlotT
{
    m256i pubkey;
    unsigned int score;
    unsigned int depth;
    PackedAnnT ann;
};

// tickOffset -> the run of records committed in that tick, so findIndexBySolutionRef() resolves a
// SolutionRef without scanning the store. The run is unbroken because the store is append-only and
// a tick's solutions all commit while that tick is processed
struct AntTickSlot
{
    unsigned int startIdx;   // this tick's first record
    unsigned int count;      // records this tick produced; written last, so it gates the run
};

// Results and diagnostics
enum ValidityResult
{
    Valid,
    // Passed every rule but the store is full, so it was not recorded. Still a valid solution: its
    // score is already folded into resourceTestingDigest, and the caller must refund and rank it.
    ValidNotStored,
    RejectParentNotRegistered,
    RejectStale,                 // anchor in the future, or published more than N ticks after it
    RejectWrongTree,             // parent belongs to a different identity
    RejectBelowThreshold,        // score above the per-epoch error bound
    RejectLeParent,              // did not strictly beat the parent
    RejectBelowSiblingFloor,     // did not strictly beat the best sibling anchored more than N earlier
    RejectTickOutOfRange,
    RejectReplay,                // (pubkey, parentRef, nonce) already committed this epoch
    RejectDedupFull,
    RejectMinerIndexFull,        // more than MAX_NUMBER_OF_MINERS identities hold a tree this epoch
    RejectNonCanonicalNonce,     // scorer refused the nonce; no score was produced
};

struct AntColonyDiagnostics
{
    unsigned long long rejectParentNotRegistered;
    unsigned long long rejectStale;
    unsigned long long rejectWrongTree;
    unsigned long long rejectThreshold;
    unsigned long long rejectLeParent;
    unsigned long long rejectSiblingFloor;
    unsigned long long rejectTickOutOfRange;
    unsigned long long rejectReplay;
    unsigned long long rejectDedupFull;
    unsigned long long rejectMinerIndexFull;
    unsigned long long rejectNonCanonicalNonce;

    unsigned long long acceptedSolutions;
    unsigned long long acceptedNotStored;
    unsigned long long treeDepthMax;
    unsigned long long treeSizeCurrent;

    void reset()
    {
        setMem(this, sizeof(*this), 0);
    }

    void count(ValidityResult r)
    {
        switch (r)
        {
        case ValidityResult::RejectParentNotRegistered: rejectParentNotRegistered++; break;
        case ValidityResult::RejectStale:               rejectStale++; break;
        case ValidityResult::RejectWrongTree:           rejectWrongTree++; break;
        case ValidityResult::RejectBelowThreshold:      rejectThreshold++; break;
        case ValidityResult::RejectLeParent:            rejectLeParent++; break;
        case ValidityResult::RejectBelowSiblingFloor:   rejectSiblingFloor++; break;
        case ValidityResult::RejectTickOutOfRange:      rejectTickOutOfRange++; break;
        case ValidityResult::RejectReplay:              rejectReplay++; break;
        case ValidityResult::RejectDedupFull:           rejectDedupFull++; break;
        case ValidityResult::RejectMinerIndexFull:      rejectMinerIndexFull++; break;
        case ValidityResult::RejectNonCanonicalNonce:   rejectNonCanonicalNonce++; break;
        default: break;
        }
    }
};

// A proposed child, reduced to what the admission rules read. Deliberately narrower than
// AntCommitInput so validateChild() stays a pure predicate.
struct ChildCandidate
{
    m256i pubkey;
    unsigned int score;           // error count, lower is better
    unsigned int anchorTick;      // ABSOLUTE
    unsigned int publishTick;     // ABSOLUTE
};

// Carries BOTH tick bases. selfRef/parentRef hold epoch-RELATIVE ticks, anchorTick/publishTick are
// ABSOLUTE system ticks. They are all unsigned int and all named "tick", so comparing one against
// the other compiles silently and is meaningless - on mainnet that is ~2,000,000 against ~70,000,000.
struct AntCommitInput
{
    m256i pubkey;
    m256i nonce;
    SolutionRef parentRef;
    SolutionRef selfRef;
    unsigned int anchorTick;      // ABSOLUTE
    unsigned int publishTick;     // ABSOLUTE
};

// The keyed structures are twice the population they index.
static constexpr unsigned long long ANT_DEDUP_SIZE = 2ULL * ANT_MAX_NODES_PER_EPOCH;
// At most one key per record, and records are capped, so load stays at or below 50% and set() cannot fail.
static constexpr unsigned long long ANT_CHILD_HEAD_BY_PARENT_SIZE = 2ULL * ANT_MAX_NODES_PER_EPOCH;
static_assert(ANT_CHILD_HEAD_BY_PARENT_SIZE >= 2ULL * ANT_MAX_NODES_PER_EPOCH,
    "child-head-by-parent map must stay at or below 50% load so its set() cannot fail");
// One entry per identity holding a tree. Unlike the map above, nothing caps how many identities
// submit, so this one CAN fill - commit() fails closed with RejectMinerIndexFull.
static constexpr unsigned long long ANT_CHILD_HEAD_BY_MINER_SIZE = 2ULL * MAX_NUMBER_OF_MINERS;

// How many ANN the epoch's harvest keeps. The target is the LUT with the best error, so this is
// simply the lowest N scores of the epoch - not one per identity, and not tied to the ranking
static constexpr unsigned int ANT_EXPORT_MAX_SOLUTIONS = NUMBER_OF_COMPUTORS;

// Anchor digests for recent ticks, indexed by tick & (size - 1). Smallest power of two holding
// 2*(N+1) entries so a lookup inside the freshness window can never be aliased by a newer tick.
static constexpr unsigned int antAnchorRingSize(unsigned int window)
{
    unsigned int size = 1;
    while (size < 2u * (window + 1u))
    {
        size <<= 1;
    }
    return size;
}
static constexpr unsigned int ANT_ANCHOR_RING_SIZE = antAnchorRingSize(ANT_FRESHNESS_WINDOW_TICKS);
static constexpr unsigned int ANT_ANCHOR_TICK_NONE = 0xFFFFFFFFU;

struct AnchorRing
{
    unsigned int ticks[ANT_ANCHOR_RING_SIZE];
    m256i digests[ANT_ANCHOR_RING_SIZE];
};

// ---------------------------------------------------------------------------------------------

template<typename ScoreT>
class AntColony
{
public:
    // The ANN state will depend on score type
    using Ann = typename ScoreT::ANN;

    // In-store form of Ann: 2 bits per trit
    using PackedAnn = score_engine::PackedTrits<ScoreT::maxNumberOfNeurons, ScoreT::lutSize>;
    static_assert(sizeof(PackedAnn) == PackedAnn::groupCount * sizeof(unsigned long long),
        "PackedAnn must not be padded");
    // Catches sizing from a scorer's padded genome (bpp9000: lutSize 27 vs PaddedLut 32).
    static_assert(PackedAnn::tritCount == sizeof(Ann), "PackedAnn must cover exactly one ANN");

    using ExportSlot = AntExportSlotT<PackedAnn>;

    // The epoch's best ANN, maintained as solutions arrive
    struct ExportSet
    {
        ExportSlot slots[ANT_EXPORT_MAX_SOLUTIONS];
        // Indices into slots, ascending by score. Kept separate so an insert shifts 4-byte indices
        // rather than 552-byte slots.
        unsigned int order[ANT_EXPORT_MAX_SOLUTIONS];
        unsigned int count;
        unsigned int padding;
    };

    static constexpr unsigned long long ANT_RECORDS_BYTES =
        (unsigned long long)ANT_MAX_NODES_PER_EPOCH * sizeof(AntSolutionRecord);
    static constexpr unsigned long long ANT_ANN_POOL_BYTES =
        (unsigned long long)ANT_MAX_NODES_PER_EPOCH * sizeof(PackedAnn);

    // The score is actually a function of below
    struct ReplayKey
    {
        m256i pubkey;
        m256i nonce;
        m256i parentAnnHash;   // K12 of the parent's ANN bytes; zero for a child of the root
        m256i anchorDigest;    // the digest the walk consumed, not the tick it came from

        bool operator==(const ReplayKey& other) const
        {
            return (pubkey == other.pubkey) && (nonce == other.nonce)
                && (parentAnnHash == other.parentAnnHash) && (anchorDigest == other.anchorDigest);
        }
    };
    static_assert(sizeof(ReplayKey) == 4 * sizeof(m256i), "ReplayKey must be padding-free");

    struct ReplayEntry
    {
        ReplayKey key;
        PackedAnn ann;
        unsigned int score;
        unsigned int occupied;
    };

    // Padding-free, so the on-disk entry matches the in-memory one byte for byte.
    static_assert(sizeof(ReplayEntry) ==
        sizeof(ReplayKey) + sizeof(PackedAnn) + 2 * sizeof(unsigned int),
        "ReplayEntry unexpected padding");

    static constexpr unsigned long long ANT_REPLAY_CACHE_BYTES =
        (unsigned long long)ANT_REPLAY_CACHE_SIZE * sizeof(ReplayEntry);

    bool init();
    void deinit();

    // Wipe the whole tree. A new epoch starts empty and reseeded.
    void reset();

    void beginEpoch(const m256i& rootSeed)
    {
        reset();
        clearReplayCache();
        _rootSeed = rootSeed;
    }

    const m256i& rootSeed() const
    {
        return _rootSeed;
    }

    void setErrorThreshold(unsigned int t)
    {
        _errorThreshold = t;
    }

    unsigned int errorThreshold() const
    {
        return _errorThreshold;
    }

    unsigned int solutionCount() const
    {
        return _solutionCount;
    }

    // Slots a miner can still claim. Reaching zero does not stop acceptance, a valid solution is
    // still scored, folded, refunded and ranked, but no NEW branch point can be created, which is
    // what a miner needs to know before planning a lineage.
    unsigned int freeAnnSlotsCount() const
    {
        return (_solutionCount < ANT_MAX_NODES_PER_EPOCH) ? (ANT_MAX_NODES_PER_EPOCH - _solutionCount) : 0;
    }

    const AntColonyDiagnostics& stats() const
    {
        return _stats;
    }

    void recordReject(ValidityResult r)
    {
        ASSERT(r != ValidityResult::Valid);
        _stats.count(r);
    }

    // Only records, the ANN pool and the anchor ring are written; 
    // the tick index, both head maps and the dedup set are DERIVED and are
    // rebuilt from the records on load
    bool saveSnapshot(unsigned short epoch, CHAR16* directory, unsigned int initialTick) const;
    // rootSeed and errorThreshold are the NODE's values, not the file's. The snapshot must agree
    // with them or it is refused
    bool loadSnapshot(unsigned short epoch, CHAR16* directory,
        const m256i& rootSeed, unsigned int errorThreshold, unsigned int initialTick);

    void putReplayScore(const ReplayKey& key, unsigned int score, const Ann& ann);
    bool tryGetReplayScore(const ReplayKey& key, unsigned int& outScore, Ann& outAnn);
    void clearReplayCache();
    unsigned int replayCacheOccupancy() const
    {
        return _replayCacheOccupancy;
    }
    bool saveReplayCache(unsigned short epoch, CHAR16* directory);
    bool loadReplayCache(unsigned short epoch, CHAR16* directory);

    // Writes the ANT_EXPORT_MAX_SOLUTIONS lowest-scoring networks of the epoch to a file for offline
    // extraction. MUST be called between endEpoch() and the reset that starts the next epoch
    bool exportBestSolutions(unsigned short epoch, CHAR16* directory);

    // Get the sibling floor for querry purpose. Note that it return the best at current time
    unsigned int siblingFloorForQuery(const SolutionRef& parentRef, const m256i& childPubkey,
        unsigned int childAnchorTick)
    {
        unsigned int head = NO_SIBLING;
        // Take the latest child first with lock to touch the head map
        {
            LockGuard guard(_headMapLock);
            if (!chainHead(parentRef, childPubkey, head))
            {
                return WORST_SCORE;
            }
        }

        // Escape the lock, then read from head, in which the record is imutable 
        return siblingFloorFromHead(head, childAnchorTick);
    }

    // Anchor digests. Both take an ABSOLUTE system tick, never an epoch-relative tickOffset.
    // Called from tick processor only
    void recordAnchorDigest(unsigned int tick, const m256i& digest);
    // Can be called from any processors
    bool getAnchorDigest(unsigned int tick, m256i& digest) const;

    // Tree access

    // nullptr when idx is out of range
    const AntSolutionRecord* recordAt(long long idx) const
    {
        if (idx < 0 || (unsigned long long)idx >= _solutionCount)
        {
            return nullptr;
        }
        return &_records[idx];
    }

    // Unpacks a stored network into the caller's buffer. ROOT is never a record, so callers must
    // handle parentRef.isRoot() before reaching here.
    bool annOfNonRoot(const AntSolutionRecord& rec, Ann& out) const
    {
        if (rec.annStateSlot >= _solutionCount)
        {
            return false;
        }
        _annPool[rec.annStateSlot].unpack(out.lut);
        return true;
    }

    long long findIndexBySolutionRef(const SolutionRef& ref) const;

    // Constraint specific functions

    // Resolves a parent for scoring. outParentRec is null for ROOT_REF, the caller derives the
    // per-identity root from the submitter's pubkey instead.
    ValidityResult tryGetParent(const SolutionRef& parentRef,
        const AntSolutionRecord** outParentRec) const;

    // Admission rules for a proposed child: freshness, tree ownership, threshold, parent, sibling
    // floor. Static and pure, so the rule set is testable without a colony. Lower score is better.
    static ValidityResult validateChild(const ChildCandidate& child,
        const AntSolutionRecord* parentRecord, unsigned int siblingFloorScore, unsigned int threshold);

    // Validates and, if accepted, appends the record and its network to the store.
    ValidityResult commit(const AntCommitInput& in, const AntSolutionRecord* parentRec,
        unsigned int score, const Ann& childAnn, unsigned int childAnnHash);

private:
    // Computes the bar a new child must beat, beyond just beating its parent: the best (lowest)
    // score among the siblings that count as competition, or WORST_SCORE when there is none.
    //
    // PRIVATE ON PURPOSE - tick processor only. It is the only reader of the two head maps, and
    // QPI::HashMap has no reader/writer protocol: set() makes a slot's key visible before its value,
    // so a reader asking for the key being inserted can get a garbage index. Everything else public
    // here is safe off-thread because records/annPool are append-only behind the _solutionCount
    // barrier, but these maps are mutated in place. If a request path ever needs this value (see
    // RespondAntMineableParents), serve it from a snapshot the tick processor computed, do not
    // recompute it on the request thread.
    unsigned int siblingFloor(const SolutionRef& parentRef, const m256i& childPubkey,
        unsigned int childAnchorTick /* ABSOLUTE */) const;

    // The map read, and the ONLY part of a floor computation that touches a head map. Split out
    // because the walk that follows it does not, which is what lets the query hold the lock for a
    // constant time instead of a whole chain.
    // Depth-1 nodes chain per identity, so one miner's never raise another's floor; deeper nodes
    // chain per parent, which is single-identity by the wrong-tree check.
    bool chainHead(const SolutionRef& parentRef, const m256i& childPubkey, unsigned int& out) const
    {
        if (parentRef.isRoot())
        {
            return _childHeadByMiner->get(childPubkey, out);
        }
        return _childHeadByParent->get(parentRef, out);
    }

    // The walk half, from a chain head already in hand. Takes no lock: _records is append-only and a
    // record's nextSiblingIdx is written once at commit and never touched again, so a chain is stable
    // to follow even while the tick processor is appending elsewhere.
    unsigned int siblingFloorFromHead(unsigned int head, unsigned int childAnchorTick) const;

    // Offers a solution to the epoch's best-N set. Called for EVERY solution that passes the rules
    void noteExportCandidate(const m256i& pubkey, unsigned int score, unsigned int depth, const Ann& ann)
    {
        ExportSet& set = *_exportSet;
        unsigned int slot;
        if (set.count < ANT_EXPORT_MAX_SOLUTIONS)
        {
            slot = set.count;
        }
        else if (score >= set.slots[set.order[ANT_EXPORT_MAX_SOLUTIONS - 1]].score)
        {
            // The common case once the set is full
            return;
        }
        else
        {
            // Reuse the worst entry's storage; its index leaves the order below.
            slot = set.order[ANT_EXPORT_MAX_SOLUTIONS - 1];
        }

        set.slots[slot].pubkey = pubkey;
        set.slots[slot].score = score;
        set.slots[slot].depth = depth;
        set.slots[slot].ann.pack(ann.lut);

        // Insert into the order, shifting indices only. Equal scores keep the incumbent ahead, so
        // among equals the earlier solution ranks first
        const unsigned int end = (set.count < ANT_EXPORT_MAX_SOLUTIONS) ? set.count : (ANT_EXPORT_MAX_SOLUTIONS - 1);
        unsigned int i = end;
        while (i > 0 && set.slots[set.order[i - 1]].score > score)
        {
            set.order[i] = set.order[i - 1];
            i--;
        }
        set.order[i] = slot;
        if (set.count < ANT_EXPORT_MAX_SOLUTIONS)
        {
            set.count++;
        }
    }

    // loadSnapshot() helper: rebuild the tick index, head maps and dedup set from the loaded
    // records, treating them as untrusted input. Defined in ant_colony_snapshot.h.
    bool rebuildDerivedState(unsigned int initialTick);

    static unsigned int replaySlotOf(const ReplayKey& key)
    {
        unsigned long long digest;
        KangarooTwelve(&key, sizeof(key), &digest, sizeof(digest));
        return (unsigned int)(digest & (ANT_REPLAY_CACHE_SIZE - 1));
    }

    AntSolutionRecord* _records;
    PackedAnn* _annPool;
    AntTickSlot* _tickIndex;
    AnchorRing* _anchors;

    // Solutions already committed this epoch, so a resend is rejected instead of re-added.
    QPI::HashSet<AntDedupKey, ANT_DEDUP_SIZE>* _dedup;
    ExportSet* _exportSet;

    // The two head maps have no reader/writer protocol of their own: QPI::HashMap::set() makes a
    // slot's key visible before its value, so a reader asking for the key being inserted can get a
    // garbage index
    volatile char _headMapLock;
    // Both give siblingFloor() a parent's children without scanning the store: the value is the
    // newest child's record index, and nextSiblingIdx chains back to the older ones.
    // parent's address -> newest child
    QPI::HashMap<SolutionRef, unsigned int, ANT_CHILD_HEAD_BY_PARENT_SIZE>* _childHeadByParent;
    // miner's pubkey -> newest depth-1 node (a child OF that miner's root, not a root itself).
    // Keyed by miner because ROOT_REF is shared by everyone.
    QPI::HashMap<m256i, unsigned int, ANT_CHILD_HEAD_BY_MINER_SIZE>* _childHeadByMiner;

    ReplayEntry* _replayCache;
    volatile char _replayCacheLock;
    unsigned int _replayCacheOccupancy;

    unsigned int _solutionCount;
    unsigned int _errorThreshold;
    m256i _rootSeed;
    AntColonyDiagnostics _stats;
};

template<typename ScoreT>
inline bool AntColony<ScoreT>::init()
{
    setMem(this, sizeof(*this), 0);

    if (!allocPoolWithErrorLog(L"AntColony::_records",
        ANT_RECORDS_BYTES, (void**)&_records, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_annPool",
        ANT_ANN_POOL_BYTES, (void**)&_annPool, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_tickIndex",
        (unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(AntTickSlot),
        (void**)&_tickIndex, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_anchors",
        sizeof(AnchorRing), (void**)&_anchors, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_childHeadByParent",
        sizeof(QPI::HashMap<SolutionRef, unsigned int, ANT_CHILD_HEAD_BY_PARENT_SIZE>),
        (void**)&_childHeadByParent, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_childHeadByMiner",
        sizeof(QPI::HashMap<m256i, unsigned int, ANT_CHILD_HEAD_BY_MINER_SIZE>),
        (void**)&_childHeadByMiner, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_exportSet",
        sizeof(ExportSet), (void**)&_exportSet, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_dedup",
        sizeof(QPI::HashSet<AntDedupKey, ANT_DEDUP_SIZE>),
        (void**)&_dedup, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"AntColony::_replayCache",
        ANT_REPLAY_CACHE_BYTES, (void**)&_replayCache, __LINE__))
    {
        return false;
    }

    reset();
    clearReplayCache();
    return true;
}

template<typename ScoreT>
inline void AntColony<ScoreT>::deinit()
{
    if (_replayCache)
    {
        freePool(_replayCache);
    }
    if (_exportSet)
    {
        freePool(_exportSet);
    }
    if (_dedup)
    {
        freePool(_dedup);
    }
    if (_childHeadByMiner)
    {
        freePool(_childHeadByMiner);
    }
    if (_childHeadByParent)
    {
        freePool(_childHeadByParent);
    }
    if (_anchors)
    {
        freePool(_anchors);
    }
    if (_tickIndex)
    {
        freePool(_tickIndex);
    }
    if (_annPool)
    {
        freePool(_annPool);
    }
    if (_records)
    {
        freePool(_records);
    }

    _replayCache = nullptr;
    _exportSet = nullptr;
    _dedup = nullptr;
    _childHeadByMiner = nullptr;
    _childHeadByParent = nullptr;
    _anchors = nullptr;
    _tickIndex = nullptr;
    _annPool = nullptr;
    _records = nullptr;
}

template<typename ScoreT>
inline void AntColony<ScoreT>::reset()
{
    ASSERT(_records != nullptr);
    ASSERT(_annPool != nullptr);
    ASSERT(_tickIndex != nullptr);
    ASSERT(_anchors != nullptr);
    ASSERT(_childHeadByParent != nullptr);
    ASSERT(_childHeadByMiner != nullptr);
    ASSERT(_dedup != nullptr);
    ASSERT(_exportSet != nullptr);

    setMem(_records, ANT_RECORDS_BYTES, 0);
    setMem(_tickIndex,
        (unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(AntTickSlot), 0);
    _childHeadByParent->reset();
    _childHeadByMiner->reset();
    _dedup->reset();
    setMem(_exportSet, sizeof(ExportSet), 0);

    // ANT_ANCHOR_TICK_NONE is used rather than zero
    for (unsigned int i = 0; i < ANT_ANCHOR_RING_SIZE; i++)
    {
        _anchors->ticks[i] = ANT_ANCHOR_TICK_NONE;
        _anchors->digests[i] = m256i::zero();
    }

    _solutionCount = 0;
    _rootSeed = m256i::zero();

    // Forgets setErrorThreshold rejects everything but a perfect score, rather than silently reusing the previous epoch's bound.
    _errorThreshold = 0;

    _stats.reset();
}

template<typename ScoreT>
inline void AntColony<ScoreT>::clearReplayCache()
{
    if (_replayCache == nullptr)
    {
        return;
    }
    LockGuard guard(_replayCacheLock);
    for (unsigned int i = 0; i < ANT_REPLAY_CACHE_SIZE; i++)
    {
        _replayCache[i].occupied = 0;
    }
    _replayCacheOccupancy = 0;
}

template<typename ScoreT>
inline void AntColony<ScoreT>::putReplayScore(const ReplayKey& key, unsigned int score, const Ann& ann)
{
    if (_replayCache == nullptr)
    {
        return;
    }
    ReplayEntry staged;
    staged.key = key;
    staged.ann.pack(ann.lut);
    staged.score = score;
    staged.occupied = 1;

    ReplayEntry& slot = _replayCache[replaySlotOf(key)];
    LockGuard guard(_replayCacheLock);
    if (!slot.occupied)
    {
        _replayCacheOccupancy++;
    }
    copyMem(&slot, &staged, sizeof(ReplayEntry));
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::tryGetReplayScore(const ReplayKey& key, unsigned int& outScore, Ann& outAnn)
{
    if (_replayCache == nullptr)
    {
        return false;
    }
    ReplayEntry& slot = _replayCache[replaySlotOf(key)];
    LockGuard guard(_replayCacheLock);
    if (!slot.occupied || !(slot.key == key))
    {
        return false;
    }
    outScore = slot.score;
    slot.ann.unpack(outAnn.lut);
    return true;
}

// Cache the already computed score for the ant colony
static unsigned short ANT_COLONY_REPLAY_CACHE_FILENAME[] = L"antColonyReplayCache.???";

template<typename ScoreT>
inline bool AntColony<ScoreT>::saveReplayCache(unsigned short epoch, CHAR16* directory)
{
    if (_replayCache == nullptr)
    {
        return false;
    }
    addEpochToFileName(ANT_COLONY_REPLAY_CACHE_FILENAME,
        sizeof(ANT_COLONY_REPLAY_CACHE_FILENAME) / sizeof(ANT_COLONY_REPLAY_CACHE_FILENAME[0]), epoch);

    // Held across the file IO so the table cannot change under the write. That blocks the solution
    // processors for the duration
    LockGuard guard(_replayCacheLock);
    if (saveLargeFile(ANT_COLONY_REPLAY_CACHE_FILENAME, ANT_REPLAY_CACHE_BYTES,
        (unsigned char*)_replayCache, directory, false) != (long long)ANT_REPLAY_CACHE_BYTES)
    {
        logToConsole(L"[ant-colony] failed to save replay cache");
        return false;
    }
    return true;
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::loadReplayCache(unsigned short epoch, CHAR16* directory)
{
    if (_replayCache == nullptr)
    {
        return false;
    }
    addEpochToFileName(ANT_COLONY_REPLAY_CACHE_FILENAME,
        sizeof(ANT_COLONY_REPLAY_CACHE_FILENAME) / sizeof(ANT_COLONY_REPLAY_CACHE_FILENAME[0]), epoch);

    LockGuard guard(_replayCacheLock);
    if (loadLargeFile(ANT_COLONY_REPLAY_CACHE_FILENAME, ANT_REPLAY_CACHE_BYTES,
        (unsigned char*)_replayCache, directory) != (long long)ANT_REPLAY_CACHE_BYTES)
    {
        // Absent at the start of an epoch, and a wrong size means another build wrote it. Either way
        // the table is zeroed and every solution gets computed honestly.
        logToConsole(L"[ant-colony] no usable replay cache, solutions will be recomputed");
        setMem(_replayCache, ANT_REPLAY_CACHE_BYTES, 0);
        _replayCacheOccupancy = 0;
        return false;
    }

    unsigned int occupied = 0;
    for (unsigned int i = 0; i < ANT_REPLAY_CACHE_SIZE; i++)
    {
        if (_replayCache[i].occupied)
        {
            occupied++;
        }
    }
    _replayCacheOccupancy = occupied;

    CHAR16 message[192];
    setText(message, L"[ant-colony] replay cache loaded, entries ");
    appendNumber(message, occupied, FALSE);
    logToConsole(message);
    return true;
}

// The epoch's harvest file
static unsigned short ANT_COLONY_SOLUTIONS_EOE_FILENAME[] = L"antColonySolutions.eoe";


// Written once at the front of the file
struct AntColonyExportHeader
{
    unsigned int epoch;
    unsigned int entryCount;
    unsigned int entrySizeBytes;     // of AntColonyExportEntry, not of a store record
    unsigned int annSizeBytes;
    unsigned int solutionCount;      // the whole epoch, of which entryCount are exported
    unsigned int errorThreshold;
    unsigned char topologyHash[32];  // == BPP9000_TOPOLOGY_HASH of the build that wrote this
    unsigned char dataHash[32];      // == BPP9000_DATA_HASH
    m256i rootSeed;
};
static_assert(sizeof(AntColonyExportHeader) == 24 + 64 + 32, "AntColonyExportHeader unexpected padding");

// What the harvest needs to reproduce the best error
struct AntColonyExportEntry
{
    // Names the identity that found this network, log only
    m256i pubkey;
    unsigned int score;    // error count, lower is better - how good this network is
    unsigned int depth;    // generations of strict improvement behind it, so the chain length is visible
};
static_assert(sizeof(AntColonyExportEntry) == 32 + 8, "AntColonyExportEntry unexpected padding");

template<typename ScoreT>
inline bool AntColony<ScoreT>::exportBestSolutions(unsigned short epoch, CHAR16* directory)
{
    ASSERT(_exportSet != nullptr);

    struct Entry
    {
        AntColonyExportEntry meta;
        Ann ann;
    };
    const ExportSet& set = *_exportSet;

    AntColonyExportHeader header;
    setMem(&header, sizeof(header), 0);
    header.epoch = epoch;
    header.entryCount = set.count;
    header.entrySizeBytes = (unsigned int)sizeof(AntColonyExportEntry);
    header.annSizeBytes = (unsigned int)sizeof(Ann);
    header.solutionCount = _solutionCount;
    header.errorThreshold = _errorThreshold;
    copyMem(header.topologyHash, BPP9000_TOPOLOGY_HASH, sizeof(header.topologyHash));
    copyMem(header.dataHash, BPP9000_DATA_HASH, sizeof(header.dataHash));
    header.rootSeed = _rootSeed;

    if (set.count == 0)
    {
        return save(ANT_COLONY_SOLUTIONS_EOE_FILENAME, sizeof(header), (unsigned char*)&header, directory)
            == (long long)sizeof(header);
    }

    const unsigned long long totalBytes = sizeof(header) + (unsigned long long)set.count * sizeof(Entry);
    unsigned char* buffer = nullptr;
    if (!allocPoolWithErrorLog(L"AntColony::export", totalBytes, (void**)&buffer, __LINE__))
    {
        logToConsole(L"[ant-colony] no memory for the solution export, harvest for this epoch is lost");
        return false;
    }

    copyMem(buffer, &header, sizeof(header));
    Entry* out = (Entry*)(buffer + sizeof(header));
    for (unsigned int i = 0; i < set.count; i++)
    {
        const ExportSlot& slot = set.slots[set.order[i]];
        out[i].meta.pubkey = slot.pubkey;
        out[i].meta.score = slot.score;
        out[i].meta.depth = slot.depth;
        slot.ann.unpack(out[i].ann.lut);
    }

    const long long saved = save(ANT_COLONY_SOLUTIONS_EOE_FILENAME, totalBytes, buffer, directory);
    freePool(buffer);

    if (saved != (long long)totalBytes)
    {
        logToConsole(L"[ant-colony] failed to write the solution export");
        return false;
    }

    CHAR16 message[192];
    setText(message, L"[ant-colony] exported best networks, entries ");
    appendNumber(message, set.count, FALSE);
    appendText(message, L", best error ");
    appendNumber(message, set.slots[set.order[0]].score, FALSE);
    appendText(message, L", worst kept ");
    appendNumber(message, set.slots[set.order[set.count - 1]].score, FALSE);
    logToConsole(message);
    return true;
}

template<typename ScoreT>
inline void AntColony<ScoreT>::recordAnchorDigest(unsigned int tick, const m256i& digest)
{
    const unsigned int slot = tick & (ANT_ANCHOR_RING_SIZE - 1);
    // Invalidate first
    ATOMIC_STORE32(_anchors->ticks[slot], (long)ANT_ANCHOR_TICK_NONE);
    _anchors->digests[slot] = digest;
    // The tick marker is written last, a reader that sees the tick must already see its digest.
    ATOMIC_STORE32(_anchors->ticks[slot], (long)tick);
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::getAnchorDigest(unsigned int tick, m256i& digest) const
{
    if (tick == ANT_ANCHOR_TICK_NONE)
    {
        return false;
    }
    const unsigned int slot = tick & (ANT_ANCHOR_RING_SIZE - 1);
    if (_anchors->ticks[slot] != tick)
    {
        return false;   // never recorded, or aged out and overwritten by a newer tick
    }
    digest = _anchors->digests[slot];
    return (_anchors->ticks[slot] == tick);
}

template<typename ScoreT>
inline long long AntColony<ScoreT>::findIndexBySolutionRef(const SolutionRef& ref) const
{
    if (ref.isRoot() || ref.tickOffset >= MAX_NUMBER_OF_TICKS_PER_EPOCH)
    {
        return ANT_INVALID_INDEX;
    }
    // Start from tick' begin index in the record and loop total of record in the tick
    const AntTickSlot& slot = _tickIndex[ref.tickOffset];
    for (unsigned int i = 0; i < slot.count; i++)
    {
        const unsigned int idx = slot.startIdx + i;
        if (idx >= _solutionCount)
        {
            break;
        }
        if (_records[idx].selfRef == ref)
        {
            return (long long)idx;
        }
    }
    return ANT_INVALID_INDEX;
}

template<typename ScoreT>
inline unsigned int AntColony<ScoreT>::siblingFloor(const SolutionRef& parentRef, const m256i& childPubkey,
    unsigned int childAnchorTick) const
{
    // Tick processor only, so the head map read needs no lock here - nothing else writes it.
    unsigned int head = NO_SIBLING;
    if (!chainHead(parentRef, childPubkey, head))
    {
        return WORST_SCORE;
    }
    return siblingFloorFromHead(head, childAnchorTick);
}

template<typename ScoreT>
inline unsigned int AntColony<ScoreT>::siblingFloorFromHead(unsigned int head,
    unsigned int childAnchorTick) const
{
    // A competing sibling anchors more than N ticks earlier, so guard the subtraction. This only
    // fires during the network's first N ticks, when there are no siblings to compete with anyway.
    if (childAnchorTick <= ANT_FRESHNESS_WINDOW_TICKS)
    {
        return WORST_SCORE;
    }
    const unsigned int boundary = childAnchorTick - ANT_FRESHNESS_WINDOW_TICKS;

    // The bar is the BEST score among competing siblings, because lower is better. The chain is
    // finite and terminates on its own, and there is deliberately no hop limit: entries are
    // newest-first and only the older ones compete, so cutting from the head would remove exactly
    // the siblings that set the floor - wrong, not merely approximate.
    unsigned int floor = WORST_SCORE;
    unsigned int idx = head;
    while (idx != NO_SIBLING)
    {
        // NOT a redundant bounds check - it is the publication barrier this walk relies on. commit()
        // points the head map at a new index BEFORE it writes that record, and only bumps
        // _solutionCount once the record is complete. An off-thread walk that reaches the new index
        // early therefore stops here instead of reading half a record. Removing this reintroduces a
        // torn read that would only ever show up under load.
        if (idx >= _solutionCount)
        {
            break;
        }
        const AntSolutionRecord& s = _records[idx];
        if (s.anchorTick < boundary && s.score < floor)
        {
            floor = s.score;
        }
        idx = s.nextSiblingIdx;
    }
    return floor;
}

template<typename ScoreT>
inline ValidityResult AntColony<ScoreT>::tryGetParent(const SolutionRef& parentRef,
    const AntSolutionRecord** outParentRec) const
{
    *outParentRec = nullptr;
    if (parentRef.isRoot())
    {
        return ValidityResult::Valid;   // root is not a record; a null parent is the valid answer
    }

    const long long parentIdx = findIndexBySolutionRef(parentRef);
    if (parentIdx == ANT_INVALID_INDEX)
    {
        return ValidityResult::RejectParentNotRegistered;
    }
    const AntSolutionRecord* rec = recordAt(parentIdx);
    if (rec == nullptr)
    {
        return ValidityResult::RejectParentNotRegistered;
    }
    *outParentRec = rec;
    return ValidityResult::Valid;
}

template<typename ScoreT>
inline ValidityResult AntColony<ScoreT>::validateChild(const ChildCandidate& child,
    const AntSolutionRecord* parentRecord, unsigned int siblingFloorScore, unsigned int threshold)
{
    // Freshness, the anchor cannot be in the future, and publication cannot lag it by more than N.
    if (child.anchorTick > child.publishTick
        || (child.publishTick - child.anchorTick) > ANT_FRESHNESS_WINDOW_TICKS)
    {
        return ValidityResult::RejectStale;
    }

    // A null parent record means ROOT, it has no score of its own, so seed WORST_SCORE and any child
    // improves on it. A non-root parent must belong to the same identity
    unsigned int parentScore = WORST_SCORE;
    if (parentRecord != nullptr)
    {
        if (!(parentRecord->pubkey == child.pubkey))
        {
            return ValidityResult::RejectWrongTree;
        }
        parentScore = parentRecord->score;
    }

    if (child.score > threshold)
    {
        return ValidityResult::RejectBelowThreshold;
    }
    if (child.score >= parentScore)
    {
        return ValidityResult::RejectLeParent;
    }
    if (child.score >= siblingFloorScore)
    {
        return ValidityResult::RejectBelowSiblingFloor;
    }
    return ValidityResult::Valid;
}

template<typename ScoreT>
inline ValidityResult AntColony<ScoreT>::commit(const AntCommitInput& in, const AntSolutionRecord* parentRec,
    unsigned int score, const Ann& childAnn, unsigned int childAnnHash)
{
    const unsigned int floor = siblingFloor(in.parentRef, in.pubkey, in.anchorTick);
    const ChildCandidate child{ in.pubkey, score, in.anchorTick, in.publishTick };

    const ValidityResult result = validateChild(child, parentRec, floor, _errorThreshold);
    if (result != ValidityResult::Valid)
    {
        recordReject(result);
        return result;
    }

    const AntDedupKey dedupKey{ in.pubkey, in.nonce, in.parentRef };
    if (_dedup->contains(dedupKey))
    {
        recordReject(ValidityResult::RejectReplay);
        return ValidityResult::RejectReplay;
    }
    // Store full. Every rule above already passed, so the solution is honest work and its score is
    // in the digest whatever happens here - rejecting it would burn the deposit for a valid answer.
    // Honour it and stop storing: the tree freezes, the leaderboard does not.
    if (_solutionCount >= ANT_MAX_NODES_PER_EPOCH)
    {
        // The record is dropped, the network is not, still note this sols for end of epoch exppot
        noteExportCandidate(in.pubkey, score, (parentRec != nullptr) ? (parentRec->depth + 1) : 1, childAnn);
        _stats.acceptedNotStored++;
        return ValidityResult::ValidNotStored;
    }
    if (in.selfRef.tickOffset >= MAX_NUMBER_OF_TICKS_PER_EPOCH)
    {
        recordReject(ValidityResult::RejectTickOutOfRange);
        return ValidityResult::RejectTickOutOfRange;
    }
    // never commit a solution without recording its replay key. Cannot fire under the
    // cap (population <= ANT_MAX_NODES_PER_EPOCH = 50% of ANT_DEDUP_SIZE), kept as a defensive check
    if (_dedup->add(dedupKey) == QPI::NULL_INDEX)
    {
        recordReject(ValidityResult::RejectDedupFull);
        return ValidityResult::RejectDedupFull;
    }

    const unsigned int newIdx = _solutionCount;

    // Claim the sibling-chain head before writing the record
    unsigned int prevHead = NO_SIBLING;
    bool headClaimed = true;
    {
        LockGuard guard(_headMapLock);
        if (in.parentRef.isRoot())
        {
            _childHeadByMiner->get(in.pubkey, prevHead);
            headClaimed = (_childHeadByMiner->set(in.pubkey, newIdx) != QPI::NULL_INDEX);
        }
        else
        {
            _childHeadByParent->get(in.parentRef, prevHead);
            _childHeadByParent->set(in.parentRef, newIdx);   // cannot fail, see the static_assert on its size
        }
    }
    if (!headClaimed)
    {
        // Fail closed. Degrading instead, accepting the node but leaving the identity without a
        // chain head, would silently drop its sibling floor. Released first: this touches _dedup,
        // which must not be reached under the head-map lock.
        _dedup->remove(dedupKey);
        recordReject(ValidityResult::RejectMinerIndexFull);
        return ValidityResult::RejectMinerIndexFull;
    }

    // The record and its network share an index, which keeps the used portion of the allocation a
    // contiguous prefix.
    _annPool[newIdx].pack(childAnn.lut);

    AntSolutionRecord& newRec = _records[newIdx];
    newRec.pubkey = in.pubkey;
    newRec.nonce = in.nonce;
    newRec.parentRef = in.parentRef;
    newRec.selfRef = in.selfRef;
    newRec.score = score;
    newRec.anchorTick = in.anchorTick;
    newRec.depth = (parentRec != nullptr) ? (parentRec->depth + 1) : 1;
    newRec.childAnnHash = childAnnHash;
    newRec.annStateSlot = newIdx;
    newRec.nextSiblingIdx = prevHead;

    AntTickSlot& tslot = _tickIndex[in.selfRef.tickOffset];
    if (tslot.count == 0)
    {
        tslot.startIdx = newIdx;
    }

    // PUBLICATION ORDER, load-bearing. Readers on other threads gate on tslot.count, so everything
    // they may then read must already be visible: record fields, then _solutionCount, then
    // tslot.count last. _solutionCount must rise before tslot.count or findIndexBySolutionRef can
    // resolve an index that recordAt() rejects.
    // ATOMIC_STORE32 is here for the ordering barrier, not for atomicity of the value: these are
    // plain unsigned ints written only by the tick processor
    ATOMIC_STORE32(_solutionCount, (long)(newIdx + 1));
    ATOMIC_STORE32(tslot.count, (long)(tslot.count + 1));

    noteExportCandidate(in.pubkey, score, newRec.depth, childAnn);

    _stats.acceptedSolutions++;
    _stats.treeSizeCurrent = _solutionCount;
    if (newRec.depth > _stats.treeDepthMax)
    {
        _stats.treeDepthMax = newRec.depth;
    }
    return ValidityResult::Valid;
}
