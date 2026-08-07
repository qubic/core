#pragma once

#include "platform/assert.h"
#include "platform/concurrency.h"
#include "platform/m256.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
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
    RejectParentNotRegistered,
    RejectStale,                 // anchor in the future, or published more than N ticks after it
    RejectWrongTree,             // parent belongs to a different identity
    RejectBelowThreshold,        // score above the per-epoch error bound
    RejectLeParent,              // did not strictly beat the parent
    RejectBelowSiblingFloor,     // did not strictly beat the best sibling anchored more than N earlier
    RejectRecordsCapFull,
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
    unsigned long long rejectRecordsCapFull;
    unsigned long long rejectTickOutOfRange;
    unsigned long long rejectReplay;
    unsigned long long rejectDedupFull;
    unsigned long long rejectMinerIndexFull;
    unsigned long long rejectNonCanonicalNonce;

    unsigned long long acceptedSolutions;
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
        case ValidityResult::RejectRecordsCapFull:      rejectRecordsCapFull++; break;
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

    static constexpr unsigned long long ANT_RECORDS_BYTES =
        (unsigned long long)ANT_MAX_NODES_PER_EPOCH * sizeof(AntSolutionRecord);
    static constexpr unsigned long long ANT_ANN_POOL_BYTES =
        (unsigned long long)ANT_MAX_NODES_PER_EPOCH * sizeof(PackedAnn);

    bool init();
    void deinit();

    // Wipe the whole tree. A new epoch starts empty and reseeded.
    void reset();

    void beginEpoch(const m256i& rootSeed)
    {
        reset();
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

    const AntColonyDiagnostics& stats() const
    {
        return _stats;
    }

    void recordReject(ValidityResult r)
    {
        ASSERT(r != ValidityResult::Valid);
        _stats.count(r);
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
        unsigned int childAnchorTick /* ABSOLUTE */,
        unsigned int walkLimit = ANT_MAX_NODES_PER_EPOCH) const;

    AntSolutionRecord* _records;
    PackedAnn* _annPool;
    AntTickSlot* _tickIndex;
    AnchorRing* _anchors;

    // Solutions already committed this epoch, so a resend is rejected instead of re-added.
    QPI::HashSet<AntDedupKey, ANT_DEDUP_SIZE>* _dedup;
    // Both give siblingFloor() a parent's children without scanning the store: the value is the
    // newest child's record index, and nextSiblingIdx chains back to the older ones.
    // parent's address -> newest child
    QPI::HashMap<SolutionRef, unsigned int, ANT_CHILD_HEAD_BY_PARENT_SIZE>* _childHeadByParent;
    // miner's pubkey -> newest depth-1 node (a child OF that miner's root, not a root itself).
    // Keyed by miner because ROOT_REF is shared by everyone.
    QPI::HashMap<m256i, unsigned int, ANT_CHILD_HEAD_BY_MINER_SIZE>* _childHeadByMiner;

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
    if (!allocPoolWithErrorLog(L"AntColony::_dedup",
        sizeof(QPI::HashSet<AntDedupKey, ANT_DEDUP_SIZE>),
        (void**)&_dedup, __LINE__))
    {
        return false;
    }

    reset();
    return true;
}

template<typename ScoreT>
inline void AntColony<ScoreT>::deinit()
{
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

    setMem(_records, ANT_RECORDS_BYTES, 0);
    setMem(_tickIndex,
        (unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(AntTickSlot), 0);
    _childHeadByParent->reset();
    _childHeadByMiner->reset();
    _dedup->reset();

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
    unsigned int childAnchorTick, unsigned int walkLimit) const
{
    // A competing sibling anchors more than N ticks earlier, so guard the subtraction. This only
    // fires during the network's first N ticks, when there are no siblings to compete with anyway.
    if (childAnchorTick <= ANT_FRESHNESS_WINDOW_TICKS)
    {
        return WORST_SCORE;
    }
    const unsigned int boundary = childAnchorTick - ANT_FRESHNESS_WINDOW_TICKS;

    // Depth-1 nodes chain per identity, so one miner's never raise another's floor.
    // Deeper nodes chain per parent, which is single-identity by the wrong-tree check.
    unsigned int idx = NO_SIBLING;
    if (parentRef.isRoot())
    {
        if (!_childHeadByMiner->get(childPubkey, idx))
        {
            return WORST_SCORE;
        }
    }
    else if (!_childHeadByParent->get(parentRef, idx))
    {
        return WORST_SCORE;
    }

    // The bar is the BEST score among competing siblings, because lower is better.
    // The chain strictly decreases (commit head-inserts) so it terminates on its own; walkLimit is a
    // backstop, and the default does not truncate. Truncating from the head would be wrong rather
    // than merely approximate: entries are newest-first and only the older ones compete, so a
    // head-side cut removes exactly the siblings that set the floor.
    unsigned int floor = WORST_SCORE;
    unsigned int hops = 0;
    while (idx != NO_SIBLING && hops < walkLimit)
    {
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
        hops++;
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
    if (_solutionCount >= ANT_MAX_NODES_PER_EPOCH)
    {
        recordReject(ValidityResult::RejectRecordsCapFull);
        return ValidityResult::RejectRecordsCapFull;
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
    if (in.parentRef.isRoot())
    {
        _childHeadByMiner->get(in.pubkey, prevHead);
        if (_childHeadByMiner->set(in.pubkey, newIdx) == QPI::NULL_INDEX)
        {
            // Fail closed. Degrading instead, accepting the node but leaving the identity without a
            // chain head, would silently drop its sibling floor
            _dedup->remove(dedupKey);
            recordReject(ValidityResult::RejectMinerIndexFull);
            return ValidityResult::RejectMinerIndexFull;
        }
    }
    else
    {
        _childHeadByParent->get(in.parentRef, prevHead);
        _childHeadByParent->set(in.parentRef, newIdx);   // cannot fail, see the static_assert on its size
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

    _stats.acceptedSolutions++;
    _stats.treeSizeCurrent = _solutionCount;
    if (newRec.depth > _stats.treeDepthMax)
    {
        _stats.treeDepthMax = newRec.depth;
    }
    return ValidityResult::Valid;
}
