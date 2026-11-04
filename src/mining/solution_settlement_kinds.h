#pragma once

#include "mining/ant_colony/ant_colony_bpp9000.h"
#include "mining/solution_settlement_window.h"

// Record and result layouts for both settlement kinds, and the ant parent-resolution logic.
// Standalone carries layouts only: its score has no parent, so there is nothing to resolve.

// Ready: ROOT or a settled tree solution. Absent: anything else.
enum AntParentStatus
{
    AntParentReady,
    AntParentAbsent
};

// A resolved parent.
struct AntParentView
{
    AntParentStatus status;
    const AntColonyBpp9000T::Ann* ann;      // nullptr for ROOT or when status != Ready
    const AntSolutionRecord* treeRecord;    // non-null exactly when Ready and not ROOT; what
                                             // validateChild() and commit() take
    unsigned int shift;                     // 0 for ROOT
    unsigned int depth;                     // 0 for ROOT
};

// What a worker produces for an ant solution. The network is stored packed
struct AntSolutionResult
{
    score_engine::Rating rating;             // Rating::worst() (invalid) marks an unscorable child
    unsigned int annHash;
    AntColonyBpp9000T::PackedAnn ann;
};
static_assert(sizeof(AntSolutionResult) <= MAX_SOLUTION_RESULT_BYTES,
    "AntSolutionResult exceeds the settlement result budget - re-derive if StoredAnn's size changes");

// Phase 1 capture for an ant solution: what settlement needs beyond the slot's own tick/txIndex.
struct AntSolutionRecordCapture
{
    m256i sourcePublicKey;
    m256i nonce;
    SolutionRef parentRef;
    unsigned int anchorTick;
    unsigned int claimedScore;
    long long amount;
};
static_assert(sizeof(AntSolutionRecordCapture) <= MAX_SOLUTION_RECORD_BYTES,
    "AntSolutionRecordCapture exceeds the settlement record budget");
// A saved record is a fixed 140 bytes whatever a kind stores in it, so the payload size is the
// only guard that two builds read one alike.
static_assert(sizeof(AntSolutionRecordCapture) == 88, "AntSolutionRecordCapture unexpected layout");

// Phase 1 capture for a standalone solution transaction.
struct StandaloneSolutionRecord
{
    m256i sourcePublicKey;
    m256i miningSeed;
    m256i nonce;
    long long amount;
    unsigned int claimedScore;
};
static_assert(sizeof(StandaloneSolutionRecord) <= MAX_SOLUTION_RECORD_BYTES,
    "StandaloneSolutionRecord exceeds the settlement record budget");
// 108 bytes of fields in 112. The window digest covers the padding after claimedScore, so the
// record is zeroed before it is filled.
static_assert(sizeof(StandaloneSolutionRecord) == 112, "StandaloneSolutionRecord unexpected layout");

// What a worker produces for a standalone solution. The score suffices: validity and algo type are
// recomputable from it and the nonce.
struct StandaloneSolutionResult
{
    unsigned int solutionScore;
};
static_assert(sizeof(StandaloneSolutionResult) <= MAX_SOLUTION_RESULT_BYTES,
    "StandaloneSolutionResult exceeds the settlement result budget");

// A solution recorded at tick T may name ROOT or a solution of tick T - W - 1 or earlier. Settlement runs
// W ticks behind, so while tick T is processed the tree holds exactly those; a later parent would
// resolve Absent or Ready depending on when a worker reads the tree.
static bool isAntParentSettledBefore(const SolutionRef& parentRef, unsigned int tick)
{
    return parentRef.isRoot()
        || (parentRef.tick < tick && tick - parentRef.tick > SOLUTION_SETTLEMENT_WINDOW_TICKS);
}

// Resolve a parent reference: ROOT is Ready with no record and no network; a settled tree solution is
// Ready with its record; anything else is Absent, including a slot still in the window - whether a slot
// is scored yet depends on this node's workers, so an answer drawn from it would differ between nodes.
// When Ready and not ROOT, the network is unpacked into scratchAnn, which the view borrows.
static void resolveAntParent(AntColonyBpp9000T& colony, AntColonyBpp9000T::Ann& scratchAnn,
    const SolutionRef& parentRef, AntParentView& out)
{
    setMem(&out, sizeof(out), 0);

    // Virtual root always available
    if (parentRef.isRoot())
    {
        out.status = AntParentReady;
        return;
    }

    const AntSolutionRecord* parentRec = nullptr;
    if (colony.tryGetParent(parentRef, &parentRec) != ValidityResult::Valid || parentRec == nullptr
        || !colony.annOfNonRoot(*parentRec, scratchAnn))
    {
        out.status = AntParentAbsent;
        return;
    }

    out.status = AntParentReady;
    out.ann = &scratchAnn;
    out.treeRecord = parentRec;
    out.shift = parentRec->shift;
    out.depth = parentRec->depth;
}
