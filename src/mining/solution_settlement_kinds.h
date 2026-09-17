#pragma once

#include "mining/ant_colony/ant_colony_bpp9000.h"
#include "mining/solution_settlement_window.h"

// Record and result layouts for both settlement kinds, and the ant parent-resolution logic.
// Standalone carries layouts only: its score has no parent, so there is nothing to resolve.

// Pending exists because of the gap: settleTick(T-W) runs at tick T's END_TICK, so for the whole
// of tick T up to that point the tree is settled only through T-W-1, and from then through T-W.
// The unsettled remainder is the window.
enum AntParentStatus
{
    AntParentReady,
    AntParentPending,
    AntParentAbsent
};

// Determine which kind of parent, wherever it came from: the derived ROOT, a solution
// scored but not yet settled, or the settled tree.
struct AntParentView
{
    AntParentStatus status;
    const AntColonyBpp9000T::Ann* ann;      // nullptr for ROOT or when status != Ready
    const AntSolutionRecord* treeRecord;    // non-null only when resolved from the tree (never ROOT
                                             // or an unsettled parent); commit()'s only valid input
    score_engine::Rating rating;            // Rating::worst() for ROOT
    unsigned int shift;                     // == rating.shift; what makeAntReplayKey takes directly
    unsigned int depth;                     // 0 for ROOT
    bool wouldCommit;
    m256i pubkey;                           // unused for ROOT
};

// What a worker produces for an ant solution. The network is stored packed
struct AntSolutionResult
{
    score_engine::Rating rating;             // Rating::worst() (invalid) marks an unscorable child
    unsigned int annHash;
    AntColonyBpp9000T::PackedAnn ann;
    unsigned int depth;
    // Prediction of commit()'s verdict at T+W.
    // This flag value is computed right after the score, but still don't have enough information
    // to make sure this sol will be commited.
    // Valus: false is permanent, true provisional.
    unsigned char wouldCommit;
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

// Build a parent view from a solution that is scored but not yet settled, so a caller can use it
// exactly as it would one already in the tree. Unpacks the network into scratchAnn. A solution
// that scored invalid is not a usable parent and resolves Absent.
static void antSolutionResultFromSlot(const SettlementSlot& slot, AntColonyBpp9000T::Ann& scratchAnn,
    AntParentView& out)
{
    AntSolutionResult result;
    copyMem(&result, slot.result, sizeof(result));
    if (!result.rating.isValid())
    {
        out.status = AntParentAbsent;   // scored but invalid: not a usable parent, same as not found
        return;
    }
    score_engine::ScoreBpp9000T::load(result.ann, scratchAnn);
    out.status = AntParentReady;
    out.ann = &scratchAnn;
    out.treeRecord = nullptr;   // ring-pending: not yet a real record
    out.rating = result.rating;
    out.shift = result.rating.shift;
    out.depth = result.depth;
    out.wouldCommit = (result.wouldCommit != 0);
    AntSolutionRecordCapture parentCapture;
    copyMem(&parentCapture, slot.record, sizeof(parentCapture));
    out.pubkey = parentCapture.sourcePublicKey;
}

// Determine which type of this parent by checking three places in order:
// - ROOT                       : Ready. Virtual, so no record and no stored network.
// - the window, if allowUnsettled   : Pending while unscored, Ready once scored,
//                                Absent if it scored invalid.
// - otherwise the settled tree : Ready, carrying its record.
// - found in none of them      : Absent.
//
// Settlement passes allowUnsettled = false; every other caller passes true. The others only steer
// node-local decisions, where nodes may legitimately differ. Settlement's answer is folded into
// the digest, where they may not: it would fold whatever this node's worker had finished by
// then, and the network would split. It would gain nothing anyway, since by T+W every parent is
// settled or rejected and Pending cannot occur.
//
// When Ready and not ROOT, the network is unpacked into scratchAnn. The view borrows it, so the
// view is valid only as long as scratchAnn is.
static void resolveAntParent(AntColonyBpp9000T& colony, SolutionSettlementWindow& window,
    unsigned short antInputType, AntColonyBpp9000T::Ann& scratchAnn, const SolutionRef& parentRef,
    bool allowUnsettled, AntParentView& out)
{
    setMem(&out, sizeof(out), 0);

    // Virtual root always available
    if (parentRef.isRoot())
    {
        out.status = AntParentReady;
        out.rating = score_engine::Rating::worst();
        out.wouldCommit = true;
        return;
    }

    // Look into the windows
    if (allowUnsettled)
    {
        const SettlementSlot* slot = window.lookup(parentRef.tick, parentRef.solutionIndexInTick);
        if (slot != nullptr && slot->inputType == antInputType)
        {
            if (!slot->scored)
            {
                out.status = AntParentPending;
                return;
            }
            antSolutionResultFromSlot(*slot, scratchAnn, out);
            return;
        }
    }

    // Look into already settled nodes
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
    out.wouldCommit = true;
    out.pubkey = parentRec->pubkey;
}

// Whether a parent may be built on. Two conditions:
//   - AntParentReady: it has an ANN and a valid rating, so a child can be scored against it.
//   - wouldCommit: this node predicts it will satisfy the tree's rules and settle.
//
// Ready alone is not enough. A parent that will fail commit() still has a real ANN, so a child of
// it passes every receipt check and the computor pays a deposit on a solution settlement rejects.
//
// wouldCommit is a prediction, and node-local: it depends on when this node's worker ran. It
// gates what this node accepts and publishes, and must never feed scoring or settlement.
static bool antParentMineable(const AntParentView& parent)
{
    return parent.status == AntParentReady && parent.wouldCommit;
}

// Carries only what validateChild() reads, for an unsettled parent, which has no record.
static const AntSolutionRecord* antParentRecordForValidation(const AntParentView& parent,
    AntSolutionRecord& scratch)
{
    if (parent.status != AntParentReady || parent.ann == nullptr)
    {
        return nullptr;   // ROOT, or a status the caller must already have handled
    }
    if (parent.treeRecord != nullptr)
    {
        return parent.treeRecord;
    }
    setMem(&scratch, sizeof(scratch), 0);
    scratch.pubkey = parent.pubkey;
    scratch.score = parent.rating.error;
    scratch.shift = parent.rating.shift;
    return &scratch;
}

// Node-local prediction; commit() at settlement is the verdict. childCount counts settled children
// only, so it under-counts if ANT_MAX_CHILDREN_PER_PARENT is ever given a cap.
static bool computeAntWouldCommit(const AntParentView& parentView, const ChildCandidate& candidate,
    unsigned int childCount, unsigned int threshold)
{
    AntSolutionRecord validationScratch;
    const AntSolutionRecord* parentForValidation = antParentRecordForValidation(parentView, validationScratch);
    // treeRecord != nullptr -  certainty, available 676 ticks late
    // wouldCommit           -  a prediction, available shortly s after landing
    const bool parentWouldCommit = (parentView.treeRecord != nullptr) || parentView.wouldCommit;
    return parentWouldCommit
        && (AntColonyBpp9000T::validateChild(candidate, parentForValidation, childCount, threshold)
            == ValidityResult::Valid);
}
