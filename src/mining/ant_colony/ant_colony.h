#pragma once

#include "platform/assert.h"
#include "platform/concurrency.h"
#include "platform/m256.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
#include "kangaroo_twelve.h"
#include "platform/file_io.h"
#include "platform/debugging.h"
#include "contract_core/pre_qpi_def.h"
#include "qpi/qpi.h"
#include "qpi/impl/qpi_hash_map_impl.h"
#include "public_settings.h"
#include "mining/mining.h"
#include "mining/trit_pack.h"

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

// Serial scratch for the save/load header (meta + anchor ring + export set) and the solution export.
// In case of this grow to large, consider use the one in common buffer
static constexpr unsigned long long ANT_SNAPSHOT_SCRATCH_BYTES = 2ULL * 1024 * 1024; // 2MB

static constexpr unsigned int NO_SIBLING = 0xFFFFFFFFU;
static constexpr unsigned int WORST_SCORE = 0xFFFFFFFFU;
static constexpr long long ANT_INVALID_INDEX = -1;

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
static constexpr unsigned int ANT_ANCHOR_RING_SIZE = antAnchorRingSize(ANT_PUBLISH_WINDOW_TICKS);
static constexpr unsigned int ANT_ANCHOR_TICK_NONE = 0xFFFFFFFFU;

// (tick, solutionIndexInTick), ABSOLUTE system tick plus the solution transaction's index in tick.
// Every tick in the subsystem is absolute; slotOf() is the only place a tick becomes a tick-index offset.
struct SolutionRef
{
    unsigned int tick;                // ABSOLUTE system tick
    unsigned int solutionIndexInTick;

    bool operator==(const SolutionRef& other) const
    {
        return (tick == other.tick) && (solutionIndexInTick == other.solutionIndexInTick);
    }

    bool isRoot() const
    {
        return (tick == 0) && (solutionIndexInTick == 0xFFFFFFFFu);
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
    SolutionRef selfRef;          // this solution's own address (ABSOLUTE tick inside)
    unsigned int score;           // error count, lower is better
    unsigned int anchorTick;      // ABSOLUTE. tick whose digest seeded the RNG
    unsigned int depth;           // a child of the root is depth 1; the root itself is never stored
    unsigned int childAnnHash;    // K12 of the canonical ANN at commit; digest-fold input
    unsigned int annStateSlot;    // index into the ANN pool; always equals the record index
    unsigned int nextSiblingIdx;  // next child of the same parent, NO_SIBLING terminates
};
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

// slotOf(tick) -> the run of records committed in that tick, so findIndexBySolutionRef() resolves a
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
    RejectMaxChildrenPerParent,  // the parent already holds ANT_MAX_CHILDREN_PER_PARENT children
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
    unsigned long long rejectMaxChildren;
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

    void appendLog(CHAR16* message) const
    {
        appendText(message, L"tree ");
        appendNumber(message, treeSizeCurrent, TRUE);
        appendText(message, L"/");
        appendNumber(message, ANT_MAX_NODES_PER_EPOCH, TRUE);
        appendText(message, L" depth ");
        appendNumber(message, treeDepthMax, FALSE);
        appendText(message, L" | accepted ");
        appendNumber(message, acceptedSolutions, TRUE);
        appendText(message, L" (not stored ");
        appendNumber(message, acceptedNotStored, TRUE);
        appendText(message, L")");

        appendText(message, L" | rejected: parent ");
        appendNumber(message, rejectParentNotRegistered, TRUE);
        appendText(message, L", stale ");
        appendNumber(message, rejectStale, TRUE);
        appendText(message, L", wrongTree ");
        appendNumber(message, rejectWrongTree, TRUE);
        appendText(message, L", threshold ");
        appendNumber(message, rejectThreshold, TRUE);
        appendText(message, L", leParent ");
        appendNumber(message, rejectLeParent, TRUE);
        appendText(message, L", maxChildren ");
        appendNumber(message, rejectMaxChildren, TRUE);
        appendText(message, L", tickRange ");
        appendNumber(message, rejectTickOutOfRange, TRUE);
        appendText(message, L", replay ");
        appendNumber(message, rejectReplay, TRUE);
        appendText(message, L", dedupFull ");
        appendNumber(message, rejectDedupFull, TRUE);
        appendText(message, L", minerIndexFull ");
        appendNumber(message, rejectMinerIndexFull, TRUE);
        appendText(message, L", nonCanonicalNonce ");
        appendNumber(message, rejectNonCanonicalNonce, TRUE);
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
        case ValidityResult::RejectMaxChildrenPerParent: rejectMaxChildren++; break;
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

// Every tick here is an ABSOLUTE system tick: selfRef/parentRef ticks, anchorTick and publishTick all
// share one basis (publishTick equals selfRef.tick), so no cross-basis comparison can slip in.
struct AntCommitInput
{
    m256i pubkey;
    m256i nonce;
    SolutionRef parentRef;
    SolutionRef selfRef;
    unsigned int anchorTick;      // ABSOLUTE
    unsigned int publishTick;     // ABSOLUTE
};

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

    void beginEpoch(const m256i& rootSeed, unsigned int initialTick)
    {
        reset();
        clearReplayCache();
        _rootSeed = rootSeed;
        _initialTick = initialTick;
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

    // Children already recorded under this parent, capped at ANT_MAX_CHILDREN_PER_PARENT, for query
    // purposes. Off-thread safe: the head map is read under the lock, the chain walk after it is not.
    unsigned int childCountForQuery(const SolutionRef& parentRef, const m256i& childPubkey)
    {
        unsigned int head = NO_SIBLING;
        // Take the latest child first with lock to touch the head map
        {
            LockGuard guard(_headMapLock);
            if (!chainHead(parentRef, childPubkey, head))
            {
                return 0;
            }
        }

        // Escape the lock, then count from head, in which the record is imutable
        return childCountFromHead(head);
    }

    // Anchor digests. Both take an ABSOLUTE system tick.
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

    // Admission rules for a proposed child: freshness, tree ownership, threshold, parent, per-parent
    // child cap. Static and pure, so the rule set is testable without a colony. Lower score is better.
    static ValidityResult validateChild(const ChildCandidate& child,
        const AntSolutionRecord* parentRecord, unsigned int childCount, unsigned int threshold);

    // Validates and, if accepted, appends the record and its network to the store.
    ValidityResult commit(const AntCommitInput& in, const AntSolutionRecord* parentRec,
        unsigned int score, const Ann& childAnn, unsigned int childAnnHash);

private:
    // Children already recorded under this parent, capped at ANT_MAX_CHILDREN_PER_PARENT. Root
    // children are keyed by miner, deeper ones by parent.
    unsigned int countChildren(const SolutionRef& parentRef, const m256i& childPubkey) const;

    // The map read, and the ONLY part of a child count that touches a head map. Split out because the
    // walk that follows it does not, which is what lets the query hold the lock for a constant time
    // instead of a whole chain.
    // Depth-1 nodes chain per identity; deeper nodes chain per parent, which is single-identity by the
    // wrong-tree check.
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
    unsigned int childCountFromHead(unsigned int head) const;

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
    // records, treating them as untrusted input. Uses _initialTick, set by the caller beforehand.
    bool rebuildDerivedState();

    static unsigned int replaySlotOf(const ReplayKey& key)
    {
        unsigned long long digest;
        KangarooTwelve(&key, sizeof(key), &digest, sizeof(digest));
        return (unsigned int)(digest & (ANT_REPLAY_CACHE_SIZE - 1));
    }

    // The sole place an absolute tick becomes a tick-index offset. Returns false when the tick falls
    // outside this epoch's window (before initialTick, or past the per-epoch tick cap); callers treat
    // that as "no such record" - RejectParentNotRegistered on lookup, RejectTickOutOfRange on commit.
    bool slotOf(unsigned int tick, unsigned int& slot) const
    {
        if (tick < _initialTick)
        {
            return false;
        }
        slot = tick - _initialTick;
        return slot < MAX_NUMBER_OF_TICKS_PER_EPOCH;
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
    // Both give countChildren() a parent's children without scanning the store: the value is the
    // newest child's record index, and nextSiblingIdx chains back to the older ones.
    // parent's address -> newest child
    QPI::HashMap<SolutionRef, unsigned int, ANT_CHILD_HEAD_BY_PARENT_SIZE>* _childHeadByParent;
    // miner's pubkey -> newest depth-1 node (a child OF that miner's root, not a root itself).
    // Keyed by miner because ROOT_REF is shared by everyone.
    QPI::HashMap<m256i, unsigned int, ANT_CHILD_HEAD_BY_MINER_SIZE>* _childHeadByMiner;

    ReplayEntry* _replayCache;
    volatile char _replayCacheLock;

    // Serial scratch for save/load and the solution export
    unsigned char* _snapshotScratch;
    unsigned int _replayCacheOccupancy;

    unsigned int _solutionCount;
    unsigned int _errorThreshold;
    // This epoch's first tick. slotOf() maps an absolute tick to a tick-index offset against it.
    unsigned int _initialTick;
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
    if (!allocPoolWithErrorLog(L"AntColony::_snapshotScratch",
        ANT_SNAPSHOT_SCRATCH_BYTES, (void**)&_snapshotScratch, __LINE__))
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
    if (_snapshotScratch)
    {
        freePool(_snapshotScratch);
    }
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

template<typename ScoreT>
inline bool AntColony<ScoreT>::saveReplayCache(unsigned short epoch, CHAR16* directory)
{
#if ANT_USE_SCORE_CACHE
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
#else
    return true;
#endif
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::loadReplayCache(unsigned short epoch, CHAR16* directory)
{
#if ANT_USE_SCORE_CACHE
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
#else
    return true;
#endif
}

// The epoch's harvest file


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
        // gAsyncFileIO is NULL only during early init and in NO_UEFI tests; otherwise route the write
        // through the async worker so the tick-processor thread never touches the EFI file protocol.
        const long long headerSaved = gAsyncFileIO
            ? asyncSave(ANT_COLONY_SOLUTIONS_EOE_FILENAME, sizeof(header), (unsigned char*)&header, directory)
            : save(ANT_COLONY_SOLUTIONS_EOE_FILENAME, sizeof(header), (unsigned char*)&header, directory);
        return headerSaved == (long long)sizeof(header);
    }

    static_assert(sizeof(AntColonyExportHeader) + (unsigned long long)ANT_EXPORT_MAX_SOLUTIONS * sizeof(Entry)
        <= ANT_SNAPSHOT_SCRATCH_BYTES, "ant solution export exceeds the scratch buffer");
    const unsigned long long totalBytes = sizeof(header) + (unsigned long long)set.count * sizeof(Entry);
    unsigned char* buffer = _snapshotScratch;

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

    const long long saved = gAsyncFileIO
        ? asyncSave(ANT_COLONY_SOLUTIONS_EOE_FILENAME, totalBytes, buffer, directory)
        : save(ANT_COLONY_SOLUTIONS_EOE_FILENAME, totalBytes, buffer, directory);

    if (saved != (long long)totalBytes)
    {
#ifndef NDEBUG
        addDebugMessage(L"[ant-colony] failed to write the solution export");
#endif
        return false;
    }

#ifndef NDEBUG
    CHAR16 message[192];
    setText(message, L"[ant-colony] exported best networks, entries ");
    appendNumber(message, set.count, FALSE);
    appendText(message, L", best error ");
    appendNumber(message, set.slots[set.order[0]].score, FALSE);
    appendText(message, L", worst kept ");
    appendNumber(message, set.slots[set.order[set.count - 1]].score, FALSE);
    addDebugMessage(message);
#endif
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
    // Seqlock read, the tick is loaded atomically before and after the digest copy, so a re-check
    // that still sees the same tick means the digest was not overwritten mid-copy.
    if ((unsigned int)ATOMIC_LOAD32(_anchors->ticks[slot]) != tick)
    {
        return false;   // never recorded, or aged out and overwritten by a newer tick
    }
    digest = _anchors->digests[slot];
    return ((unsigned int)ATOMIC_LOAD32(_anchors->ticks[slot]) == tick);
}

template<typename ScoreT>
inline long long AntColony<ScoreT>::findIndexBySolutionRef(const SolutionRef& ref) const
{
    unsigned int tickSlot = 0;
    if (ref.isRoot() || !slotOf(ref.tick, tickSlot))
    {
        return ANT_INVALID_INDEX;
    }
    // Start from tick' begin index in the record and loop total of record in the tick
    const AntTickSlot& slot = _tickIndex[tickSlot];
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
inline unsigned int AntColony<ScoreT>::countChildren(const SolutionRef& parentRef, const m256i& childPubkey) const
{
    // Tick processor only, so the head map read needs no lock here - nothing else writes it.
    unsigned int head = NO_SIBLING;
    if (!chainHead(parentRef, childPubkey, head))
    {
        return 0;
    }
    return childCountFromHead(head);
}

template<typename ScoreT>
inline unsigned int AntColony<ScoreT>::childCountFromHead(unsigned int head) const
{
    // Count only up to the cap: past it the child is rejected regardless, so the walk and with it
    // the whole per-commit cost, is bounded by ANT_MAX_CHILDREN_PER_PARENT. A cap of 0 (unbound)
    // leaves the loop empty and returns 0, since the count is then never used.
    unsigned int count = 0;
    unsigned int idx = head;
    while (idx != NO_SIBLING && count < ANT_MAX_CHILDREN_PER_PARENT)
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
        count++;
        idx = _records[idx].nextSiblingIdx;
    }
    return count;
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
    const AntSolutionRecord* parentRecord, unsigned int childCount, unsigned int threshold)
{
    // Freshness, the anchor cannot be in the future, and publication cannot lag it by more than N.
    if (child.anchorTick > child.publishTick
        || (child.publishTick - child.anchorTick) > ANT_PUBLISH_WINDOW_TICKS)
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
    // Per-parent breadth cap. 0 means unbound - no cap.
    if (ANT_MAX_CHILDREN_PER_PARENT != 0 && childCount >= ANT_MAX_CHILDREN_PER_PARENT)
    {
        return ValidityResult::RejectMaxChildrenPerParent;
    }
    return ValidityResult::Valid;
}

template<typename ScoreT>
inline ValidityResult AntColony<ScoreT>::commit(const AntCommitInput& in, const AntSolutionRecord* parentRec,
    unsigned int score, const Ann& childAnn, unsigned int childAnnHash)
{
    const unsigned int childCount = countChildren(in.parentRef, in.pubkey);
    const ChildCandidate child{ in.pubkey, score, in.anchorTick, in.publishTick };

    const ValidityResult result = validateChild(child, parentRec, childCount, _errorThreshold);
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
    unsigned int selfSlot = 0;
    if (!slotOf(in.selfRef.tick, selfSlot))
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
        // chain head, would leave its children uncounted and silently disable its cap. Released
        // first: this touches _dedup, which must not be reached under the head-map lock.
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

    AntTickSlot& tslot = _tickIndex[selfSlot];
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

// Only what cannot be derived is written. The tick index, both head maps and the dedup set are
// rebuilt from the records

struct AntColonySnapshotMeta
{
    unsigned int magic;
    unsigned int version;
    unsigned int epoch;
    unsigned int solutionCount;
    // Layout guards. A snapshot written by a build with a different record or ANN size must be
    // refused rather than reinterpreted
    unsigned int recordSizeBytes;
    unsigned int annPoolEntryBytes;
    unsigned int errorThreshold;
    // This epoch's first tick. Records hold absolute ticks; the base must still match so slotOf() maps
    // them into this node's tick index, and a snapshot from another epoch is refused rather than mis-read.
    unsigned int initialTick;
    unsigned int anchorRingBytes;
    unsigned int exportSetBytes;
    m256i rootSeed;

    static constexpr unsigned int MAGIC = 0x414E5443;   // "ANTC"
    static constexpr unsigned int VERSION = 1;   // SolutionRef holds absolute ticks
};
static_assert(sizeof(AntColonySnapshotMeta) == 40 + 32, "AntColonySnapshotMeta unexpected padding");

static void antSnapshotFailure(const CHAR16* what, unsigned long long a, unsigned long long b)
{
    CHAR16 message[256];
    setText(message, L"[ant-colony] snapshot: ");
    appendText(message, what);
    appendText(message, L" ");
    appendNumber(message, a, FALSE);
    appendText(message, L" / ");
    appendNumber(message, b, FALSE);
    logToConsole(message);
}

// Records and pool are sized by this, never by the raw count. At least one slot is always written,
// so no snapshot file is ever zero length
static unsigned long long antSnapshotSlotCount(unsigned int solutionCount)
{
    return (solutionCount > 0) ? (unsigned long long)solutionCount : 1ULL;
}

static void antSnapshotNameForEpoch(unsigned short epoch)
{
    addEpochToFileName(ANT_SNAPSHOT_HEADER_FILENAME, sizeof(ANT_SNAPSHOT_HEADER_FILENAME) / sizeof(ANT_SNAPSHOT_HEADER_FILENAME[0]), epoch);
    addEpochToFileName(ANT_SNAPSHOT_RECORDS_FILENAME, sizeof(ANT_SNAPSHOT_RECORDS_FILENAME) / sizeof(ANT_SNAPSHOT_RECORDS_FILENAME[0]), epoch);
    addEpochToFileName(ANT_SNAPSHOT_POOL_FILENAME, sizeof(ANT_SNAPSHOT_POOL_FILENAME) / sizeof(ANT_SNAPSHOT_POOL_FILENAME[0]), epoch);
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::saveSnapshot(unsigned short epoch, CHAR16* directory,
    unsigned int initialTick) const
{
    ASSERT(_records != nullptr);
    ASSERT(_annPool != nullptr);
    ASSERT(_anchors != nullptr);

    antSnapshotNameForEpoch(epoch);

    AntColonySnapshotMeta meta;
    setMem(&meta, sizeof(meta), 0);
    meta.magic = AntColonySnapshotMeta::MAGIC;
    meta.version = AntColonySnapshotMeta::VERSION;
    meta.epoch = epoch;
    meta.solutionCount = _solutionCount;
    meta.recordSizeBytes = (unsigned int)sizeof(AntSolutionRecord);
    meta.annPoolEntryBytes = (unsigned int)sizeof(PackedAnn);
    meta.errorThreshold = _errorThreshold;
    meta.initialTick = initialTick;
    meta.anchorRingBytes = (unsigned int)sizeof(AnchorRing);
    meta.exportSetBytes = (unsigned int)sizeof(ExportSet);
    meta.rootSeed = _rootSeed;

    // The meta, anchor ring and export set share one file. The file API writes a single contiguous
    // buffer, so the three are gathered into the serial scratch: meta, then anchors, then export.
    const unsigned long long headerBytes = sizeof(meta) + sizeof(AnchorRing) + sizeof(ExportSet);
    static_assert(sizeof(AntColonySnapshotMeta) + sizeof(AnchorRing) + sizeof(ExportSet) <= ANT_SNAPSHOT_SCRATCH_BYTES,
        "ant snapshot header exceeds the scratch buffer");
    unsigned char* headerBuffer = _snapshotScratch;
    copyMem(headerBuffer, &meta, sizeof(meta));
    copyMem(headerBuffer + sizeof(meta), _anchors, sizeof(AnchorRing));
    copyMem(headerBuffer + sizeof(meta) + sizeof(AnchorRing), _exportSet, sizeof(ExportSet));
    if (save(ANT_SNAPSHOT_HEADER_FILENAME, headerBytes, headerBuffer, directory) != (long long)headerBytes)
    {
        logToConsole(L"[ant-colony] failed to save snapshot header");
        return false;
    }

    // Written even when the colony is empty, so the operator's snapshot is always the same number of files
    const unsigned long long slots = antSnapshotSlotCount(_solutionCount);
    const unsigned long long recordBytes = slots * sizeof(AntSolutionRecord);
    const unsigned long long poolBytes = slots * sizeof(PackedAnn);
    if (saveLargeFile(ANT_SNAPSHOT_RECORDS_FILENAME, recordBytes, (unsigned char*)_records, directory, false)
        != (long long)recordBytes)
    {
        logToConsole(L"[ant-colony] failed to save snapshot records");
        return false;
    }
    if (saveLargeFile(ANT_SNAPSHOT_POOL_FILENAME, poolBytes, (unsigned char*)_annPool, directory, false)
        != (long long)poolBytes)
    {
        logToConsole(L"[ant-colony] failed to save snapshot pool");
        return false;
    }
    return true;
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::loadSnapshot(unsigned short epoch, CHAR16* directory,
    const m256i& rootSeed, unsigned int errorThreshold, unsigned int initialTick)
{
    ASSERT(_records != nullptr);
    ASSERT(_annPool != nullptr);
    ASSERT(_anchors != nullptr);

    antSnapshotNameForEpoch(epoch);

    // The meta, anchor ring and export set share one file. Read it whole, validate the meta before
    // any colony state is touched, then copy the two sections into place.
    const unsigned long long headerBytes = sizeof(AntColonySnapshotMeta) + sizeof(AnchorRing) + sizeof(ExportSet);
    static_assert(sizeof(AntColonySnapshotMeta) + sizeof(AnchorRing) + sizeof(ExportSet) <= ANT_SNAPSHOT_SCRATCH_BYTES,
        "ant snapshot header exceeds the scratch buffer");
    unsigned char* headerBuffer = _snapshotScratch;
    if (load(ANT_SNAPSHOT_HEADER_FILENAME, headerBytes, headerBuffer, directory) != (long long)headerBytes)
    {
        logToConsole(L"[ant-colony] failed to load snapshot header");
        return false;
    }

    AntColonySnapshotMeta meta;
    copyMem(&meta, headerBuffer, sizeof(meta));
    if (meta.magic != AntColonySnapshotMeta::MAGIC || meta.version != AntColonySnapshotMeta::VERSION)
    {
        antSnapshotFailure(L"bad magic/version", meta.magic, meta.version);
        return false;
    }
    if (meta.epoch != epoch)
    {
        antSnapshotFailure(L"epoch mismatch, file/expected", meta.epoch, epoch);
        return false;
    }
    // A differently sized record or ANN would parse cleanly and produce a tree that is silently wrong.
    if (meta.recordSizeBytes != sizeof(AntSolutionRecord) || meta.annPoolEntryBytes != sizeof(PackedAnn))
    {
        antSnapshotFailure(L"layout mismatch, record/ann", meta.recordSizeBytes, meta.annPoolEntryBytes);
        return false;
    }
    // The two sections after the meta must be exactly the size this build lays them out at, or the
    // copies below would read them at the wrong offset.
    if (meta.anchorRingBytes != sizeof(AnchorRing) || meta.exportSetBytes != sizeof(ExportSet))
    {
        antSnapshotFailure(L"layout mismatch, anchors/export", meta.anchorRingBytes, meta.exportSetBytes);
        return false;
    }
    if (meta.solutionCount > ANT_MAX_NODES_PER_EPOCH)
    {
        antSnapshotFailure(L"solutionCount exceeds the cap, count/cap", meta.solutionCount, ANT_MAX_NODES_PER_EPOCH);
        return false;
    }
    // Cross-check against the node state restored alongside this file. A mismatch means the two
    // snapshots are not from the same moment - loading the tree anyway would derive every root from
    // a seed the rest of the network is not using.
    if (!(meta.rootSeed == rootSeed))
    {
        antSnapshotFailure(L"root seed does not match the restored node state, record/0", 0, 0);
        return false;
    }
    if (meta.errorThreshold != errorThreshold)
    {
        antSnapshotFailure(L"threshold does not match the node, file/node", meta.errorThreshold, errorThreshold);
        return false;
    }
    // Records hold absolute ticks; slotOf() maps them against initialTick, so a snapshot taken at a
    // different base would resolve parent references to the wrong records. Refuse it.
    if (meta.initialTick != initialTick)
    {
        antSnapshotFailure(L"initial tick does not match the node, file/node", meta.initialTick, initialTick);
        return false;
    }

    // Everything above only read the meta, so a refusal there leaves the colony untouched. From here
    // on the state is being overwritten
    reset();

    copyMem(_anchors, headerBuffer + sizeof(meta), sizeof(AnchorRing));
    copyMem(_exportSet, headerBuffer + sizeof(meta) + sizeof(AnchorRing), sizeof(ExportSet));

    // Read unconditionally and at the same sizing the save used, so an incomplete copy is refused
    // here rather than booting a node with an empty tree.
    const unsigned long long slots = antSnapshotSlotCount(meta.solutionCount);
    const unsigned long long recordBytes = slots * sizeof(AntSolutionRecord);
    const unsigned long long poolBytes = slots * sizeof(PackedAnn);
    if (loadLargeFile(ANT_SNAPSHOT_RECORDS_FILENAME, recordBytes, (unsigned char*)_records, directory)
        != (long long)recordBytes)
    {
        logToConsole(L"[ant-colony] failed to load snapshot records");
        reset();
        return false;
    }
    if (loadLargeFile(ANT_SNAPSHOT_POOL_FILENAME, poolBytes, (unsigned char*)_annPool, directory)
        != (long long)poolBytes)
    {
        logToConsole(L"[ant-colony] failed to load snapshot pool");
        reset();
        return false;
    }
    if (_exportSet->count > ANT_EXPORT_MAX_SOLUTIONS)
    {
        antSnapshotFailure(L"export count exceeds the cap, count/cap", _exportSet->count, ANT_EXPORT_MAX_SOLUTIONS);
        reset();
        return false;
    }
    for (unsigned int i = 0; i < _exportSet->count; i++)
    {
        const unsigned int slot = _exportSet->order[i];
        if (slot >= _exportSet->count)
        {
            antSnapshotFailure(L"export order out of range, position/slot", i, slot);
            reset();
            return false;
        }
        for (unsigned int j = 0; j < i; j++)
        {
            if (_exportSet->order[j] == slot)
            {
                antSnapshotFailure(L"export order duplicate, position/slot", i, slot);
                reset();
                return false;
            }
        }
    }

    // The caller's values, which the checks above proved the file agrees with.
    _rootSeed = rootSeed;
    _errorThreshold = errorThreshold;
    _solutionCount = meta.solutionCount;
    _initialTick = initialTick;

    // Rebuild the intermediate data
    if (!rebuildDerivedState())
    {
        reset();
        return false;
    }
    return true;
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::rebuildDerivedState()
{
    // Hoisted out of the loop: unpacking each stored network to verify its hash needs somewhere to
    // put it, and this path runs once at boot.
    Ann annBuffer;

    for (unsigned int i = 0; i < _solutionCount; i++)
    {
        const AntSolutionRecord& rec = _records[i];

        unsigned int selfSlot = 0;
        if (!slotOf(rec.selfRef.tick, selfSlot))
        {
            antSnapshotFailure(L"tick out of range, record/tick", i, rec.selfRef.tick);
            return false;
        }
        // commit() writes the record and its network at the same index, and findIndexBySolutionRef
        // and annOfNonRoot both rely on it
        // annStateSlot point to the ANN that must have similar index to the record
        if (rec.annStateSlot != i)
        {
            antSnapshotFailure(L"annStateSlot is not the record index, record/slot", i, rec.annStateSlot);
            return false;
        }

        // The freshness rule validateChild() applied at admission, re-checked here so a tampered
        // snapshot cannot smuggle in a record that was never admissible. anchorTick seeds the score's
        // RNG, so it is consensus-relevant. The record's own absolute tick is its publish tick.
        const unsigned int publishTick = rec.selfRef.tick;
        if (rec.anchorTick > publishTick
            || publishTick - rec.anchorTick > ANT_PUBLISH_WINDOW_TICKS)
        {
            antSnapshotFailure(L"anchor tick outside the freshness window, record/anchorTick", i, rec.anchorTick);
            return false;
        }

        // Rebuild the tick index
        AntTickSlot& tslot = _tickIndex[selfSlot];
        if (tslot.count == 0)
        {
            tslot.startIdx = i;
        }
        else if (tslot.startIdx + tslot.count != i)
        {
            antSnapshotFailure(L"record breaks tick contiguity, record/tick", i, rec.selfRef.tick);
            return false;
        }
        tslot.count++;

        // The parent must be ROOT or an EARLIER record. The tick index built so far covers only
        // records before this one, so a forward or self reference fails to resolve - which is what
        // rejects a cycle.
        const AntSolutionRecord* parentRec = nullptr;
        if (!rec.parentRef.isRoot())
        {
            const long long parentIdx = findIndexBySolutionRef(rec.parentRef);
            if (parentIdx == ANT_INVALID_INDEX || (unsigned long long)parentIdx >= i)
            {
                antSnapshotFailure(L"parent not an earlier record, record/parentTick", i, rec.parentRef.tick);
                return false;
            }
            parentRec = &_records[parentIdx];
            if (!(parentRec->pubkey == rec.pubkey))
            {
                antSnapshotFailure(L"parent belongs to another identity, record", i, 0);
                return false;
            }
        }
        const unsigned int expectedDepth = (parentRec != nullptr) ? (parentRec->depth + 1) : 1;
        if (rec.depth != expectedDepth)
        {
            antSnapshotFailure(L"depth does not match the parent, record/depth", i, rec.depth);
            return false;
        }

        // The two score rules validateChild() enforced when this record was admitted. A corrupt
        // score would otherwise set a wrong bar for its own children.
        if (rec.score > _errorThreshold)
        {
            antSnapshotFailure(L"score above the epoch threshold, record/score", i, rec.score);
            return false;
        }
        if (parentRec != nullptr && rec.score >= parentRec->score)
        {
            antSnapshotFailure(L"score does not beat the parent, record/score", i, rec.score);
            return false;
        }

        // Re-derive the hash from the stored network
        _annPool[i].unpack(annBuffer.lut);
        unsigned int annHash;
        KangarooTwelve(&annBuffer, sizeof(annBuffer), &annHash, sizeof(annHash));
        if (annHash != rec.childAnnHash)
        {
            antSnapshotFailure(L"stored network does not match childAnnHash, record", i, 0);
            return false;
        }

        const AntDedupKey key{ rec.pubkey, rec.nonce, rec.parentRef };
        if (_dedup->contains(key))
        {
            antSnapshotFailure(L"duplicate solution, record", i, 0);
            return false;
        }
        if (_dedup->add(key) == QPI::NULL_INDEX)
        {
            antSnapshotFailure(L"dedup set full, record", i, 0);
            return false;
        }

        // Rebuild the _childHeadByMiner and _childHeadByParent
        unsigned int prevHead = NO_SIBLING;
        if (rec.parentRef.isRoot())
        {
            _childHeadByMiner->get(rec.pubkey, prevHead);
            _records[i].nextSiblingIdx = prevHead;
            if (_childHeadByMiner->set(rec.pubkey, i) == QPI::NULL_INDEX)
            {
                antSnapshotFailure(L"miner index full, record", i, 0);
                return false;
            }
        }
        else
        {
            _childHeadByParent->get(rec.parentRef, prevHead);
            _records[i].nextSiblingIdx = prevHead;
            _childHeadByParent->set(rec.parentRef, i);
        }

        // Restore the stats also
        _stats.acceptedSolutions++;
        if (rec.depth > _stats.treeDepthMax)
        {
            _stats.treeDepthMax = rec.depth;
        }
    }

    _stats.treeSizeCurrent = _solutionCount;
    return true;
}
