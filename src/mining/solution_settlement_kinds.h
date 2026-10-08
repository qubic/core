#pragma once

#include "mining/ant_colony/ant_colony_bpp9000.h"
#include "mining/solution_settlement_window.h"

// Record and result layouts for both settlement kinds, and the ant parent-resolution logic.
// Standalone carries layouts only: its score has no parent, so there is nothing to resolve.

// A parent is either usable or it is not. Only the settled tree and the derived ROOT qualify, so a
// reference into the unsettled window resolves Absent like any other reference to nothing.
enum AntParentStatus
{
    AntParentReady,
    AntParentAbsent
};

// Determine which kind of parent, wherever it came from: the derived ROOT or the settled tree.
struct AntParentView
{
    AntParentStatus status;
    const AntColonyBpp9000T::Ann* ann;      // nullptr for ROOT or when status != Ready
    const AntSolutionRecord* treeRecord;    // non-null exactly when Ready and not ROOT;
                                             // commit()'s only valid input
    score_engine::Rating rating;            // Rating::worst() for ROOT
    unsigned int shift;                     // == rating.shift; what makeAntReplayKey takes directly
    unsigned int depth;                     // 0 for ROOT
    m256i pubkey;                           // unused for ROOT
};

// What a worker produces for an ant solution. The network is stored packed
struct AntSolutionResult
{
    score_engine::Rating rating;             // Rating::worst() (invalid) marks an unscorable child
    unsigned int annHash;
    AntColonyBpp9000T::PackedAnn ann;
    unsigned int depth;
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

// Resolve a parent reference against the two places a usable parent can be:
// - ROOT                : Ready. Virtual, so no record and no stored network.
// - the settled tree    : Ready, carrying its record.
// - neither             : Absent.
//
// A reference into the unsettled window is Absent. The window is node-local in one respect that
// matters: whether a given slot has been scored yet depends on when this node's workers ran. Any
// answer derived from it therefore differs between nodes, which is why no caller may build on an
// unsettled parent and why settlement, whose answer is folded into the digest, could never have.
//
// When Ready and not ROOT, the network is unpacked into scratchAnn. The view borrows it, so the
// view is valid only as long as scratchAnn is.
static void resolveAntParent(AntColonyBpp9000T& colony, AntColonyBpp9000T::Ann& scratchAnn,
    const SolutionRef& parentRef, AntParentView& out)
{
    setMem(&out, sizeof(out), 0);

    // Virtual root always available
    if (parentRef.isRoot())
    {
        out.status = AntParentReady;
        out.rating = score_engine::Rating::worst();
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
    out.rating.error = parentRec->score;
    out.rating.shift = parentRec->shift;
    out.shift = parentRec->shift;
    out.depth = parentRec->depth;
    out.pubkey = parentRec->pubkey;
}

// What validateChild() reads. nullptr for ROOT, which is derived rather than stored, and for any
// status the caller has not already handled.
static const AntSolutionRecord* antParentRecordForValidation(const AntParentView& parent)
{
    if (parent.status != AntParentReady)
    {
        return nullptr;
    }
    return parent.treeRecord;
}
