#define NO_UEFI

#include "gtest/gtest.h"

#define ENABLE_PROFILING 0

// The bound colony, not the bare template: these tests check bpp9000's binding as well as the rules.
#include "../src/mining/ant_colony_bpp9000.h"

#include <vector>

// The colony's rules and its stored network form. validateChild() is static and touches no member state,
// so the whole rule set is exercised here without allocating a colony, loading a task, or running
// the engine.
//
// Score is an ERROR COUNT: lower is better. Every expectation below depends on that direction, which
// is the thing most likely to be silently inverted by a future edit - a stale `>` reads fine and
// quietly accepts the wrong children.

static constexpr unsigned int TEST_THRESHOLD = 3838;   // BPP9000_SOLUTION_THRESHOLD_DEFAULT

static m256i makeKey(unsigned long long n)
{
    m256i k = m256i::zero();
    k.m256i_u64[0] = n + 1;
    return k;
}

// A parent sitting at the given score, owned by the given identity.
static AntSolutionRecord makeParent(const m256i& owner, unsigned int score, unsigned int depth = 1)
{
    AntSolutionRecord r;
    setMem(&r, sizeof(r), 0);
    r.pubkey = owner;
    r.score = score;
    r.depth = depth;
    r.parentRef = ROOT_REF;
    r.nextSiblingIdx = NO_SIBLING;
    return r;
}

// A candidate from `owner` at `score`, anchored and published in the same tick unless the test is
// about freshness.
static ChildCandidate makeChild(const m256i& owner, unsigned int score,
    unsigned int anchorTick = 1000, unsigned int publishTick = 1000)
{
    ChildCandidate c;
    c.pubkey = owner;
    c.score = score;
    c.anchorTick = anchorTick;
    c.publishTick = publishTick;
    return c;
}

// Every test runs at TEST_THRESHOLD, so wrapping it keeps the assertions on one line.
static ValidityResult admit(const ChildCandidate& child, const AntSolutionRecord* parent,
    unsigned int siblingFloorScore)
{
    return AntColonyBpp9000T::validateChild(child, parent, siblingFloorScore, TEST_THRESHOLD);
}

// ---------------------------------------------------------------------------------------------
// Stored network form
// ---------------------------------------------------------------------------------------------

// The packing itself is generic and tested exhaustively in trit_pack.cpp. What is ant-specific is
// the binding: that PackedAnn is dimensioned from the scorer's real ANN and covers all of it. The
// hazard is bpp9000's two strides - ANN is packed at lutSize (27) while the engine's internal
// PaddedLut is 32 - so a PackedAnn built on the wrong one would drop or duplicate entries, change
// childAnnHash, and surface as a resourceTestingDigest split rather than a crash.
TEST(TestAntColonyPackedAnn, CoversAWholeAnnAtTheUnpaddedStride)
{
    AntColonyBpp9000T::Ann src;
    for (unsigned long long i = 0; i < sizeof(src); i++)
    {
        src.lut[i] = (unsigned char)(i % 3);   // mutate() only ever writes 0, 1 or 2
    }

    AntColonyBpp9000T::PackedAnn packed;
    packed.pack(src.lut);

    AntColonyBpp9000T::Ann back;
    setMem(&back, sizeof(back), 0xFF);
    packed.unpack(back.lut);

    for (unsigned long long i = 0; i < sizeof(src); i++)
    {
        ASSERT_EQ(back.lut[i], src.lut[i]) << "entry " << i;
    }
}

// ---------------------------------------------------------------------------------------------
// Threshold
// ---------------------------------------------------------------------------------------------

// The threshold is checked before the parent comparison, so nodes worse than it are never stored -
// which is why the early descent never consumes the store.
TEST(TestAntColonyValidate, ThresholdIsAnUpperBoundOnError)
{
    const m256i me = makeKey(1);

    // A parent that would otherwise admit anything, so only the threshold can reject.
    const AntSolutionRecord looseParent = makeParent(me, WORST_SCORE);
    EXPECT_EQ(admit(makeChild(me, 3839), &looseParent, WORST_SCORE),
        ValidityResult::RejectBelowThreshold);

    // Exactly at the bound is accepted: the rule is score > threshold, not >=.
    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD), &looseParent, WORST_SCORE), ValidityResult::Valid);
}

// ---------------------------------------------------------------------------------------------
// Parent
// ---------------------------------------------------------------------------------------------

TEST(TestAntColonyValidate, MustStrictlyBeatParent)
{
    const m256i me = makeKey(2);
    const AntSolutionRecord parent = makeParent(me, 3800);

    EXPECT_EQ(admit(makeChild(me, 3799), &parent, WORST_SCORE), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 3800), &parent, WORST_SCORE), ValidityResult::RejectLeParent);
    EXPECT_EQ(admit(makeChild(me, 3801), &parent, WORST_SCORE), ValidityResult::RejectLeParent);
}

// A root has no score of its own, so any threshold-passing child improves on it. This is what lets a
// lineage start at all.
TEST(TestAntColonyValidate, RootParentAdmitsAnyPassingScore)
{
    const m256i me = makeKey(3);

    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD), nullptr, WORST_SCORE), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 0), nullptr, WORST_SCORE), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD + 1), nullptr, WORST_SCORE),
        ValidityResult::RejectBelowThreshold);
}

// Trees are isolated per identity: a miner cannot branch off someone else's node.
TEST(TestAntColonyValidate, CannotBranchFromAnotherIdentity)
{
    const m256i me = makeKey(4);
    const m256i someoneElse = makeKey(5);
    const AntSolutionRecord theirNode = makeParent(someoneElse, 3800);

    EXPECT_EQ(admit(makeChild(me, 3700), &theirNode, WORST_SCORE), ValidityResult::RejectWrongTree);

    const AntSolutionRecord myNode = makeParent(me, 3800);
    EXPECT_EQ(admit(makeChild(me, 3700), &myNode, WORST_SCORE), ValidityResult::Valid);
}

// ---------------------------------------------------------------------------------------------
// Sibling floor
// ---------------------------------------------------------------------------------------------

TEST(TestAntColonyValidate, MustStrictlyBeatSiblingFloor)
{
    const m256i me = makeKey(6);
    const AntSolutionRecord parent = makeParent(me, WORST_SCORE);

    EXPECT_EQ(admit(makeChild(me, 3799), &parent, 3800), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 3800), &parent, 3800), ValidityResult::RejectBelowSiblingFloor);
    EXPECT_EQ(admit(makeChild(me, 3801), &parent, 3800), ValidityResult::RejectBelowSiblingFloor);

    // No competing sibling yet: the floor is WORST_SCORE and anything passing gets through.
    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD), &parent, WORST_SCORE), ValidityResult::Valid);
}

// ---------------------------------------------------------------------------------------------
// Freshness
// ---------------------------------------------------------------------------------------------

TEST(TestAntColonyValidate, FreshnessWindowBoundaries)
{
    const m256i me = makeKey(7);
    const AntSolutionRecord parent = makeParent(me, WORST_SCORE);
    const unsigned int anchor = 100000;

    // Published in the same tick it anchored to: the tightest legal case.
    EXPECT_EQ(admit(makeChild(me, 3700, anchor, anchor), &parent, WORST_SCORE),
        ValidityResult::Valid);

    // Exactly at the window edge is still legal; one past it is not.
    EXPECT_EQ(admit(makeChild(me, 3700, anchor, anchor + ANT_FRESHNESS_WINDOW_TICKS),
        &parent, WORST_SCORE), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 3700, anchor, anchor + ANT_FRESHNESS_WINDOW_TICKS + 1),
        &parent, WORST_SCORE), ValidityResult::RejectStale);

    // An anchor in the future is rejected rather than wrapping the unsigned subtraction.
    EXPECT_EQ(admit(makeChild(me, 3700, anchor + 1, anchor), &parent, WORST_SCORE),
        ValidityResult::RejectStale);
}

// ---------------------------------------------------------------------------------------------
// Order of checks
// ---------------------------------------------------------------------------------------------

// The order is consensus-visible: it decides which reject a solution is charged with, which drives
// the diagnostics an operator reads. Freshness first, then tree isolation, then threshold, then
// parent, then sibling floor.
TEST(TestAntColonyValidate, ReportsTheFirstFailingRule)
{
    const m256i me = makeKey(8);
    const m256i other = makeKey(9);
    const unsigned int anchor = 100000;
    const unsigned int stalePublish = anchor + ANT_FRESHNESS_WINDOW_TICKS + 1;

    const AntSolutionRecord theirs = makeParent(other, 3000);

    // Stale AND wrong tree AND above threshold AND worse than parent -> reports Stale.
    EXPECT_EQ(admit(makeChild(me, 9999, anchor, stalePublish), &theirs, 100),
        ValidityResult::RejectStale);

    // Fresh, but wrong tree AND above threshold -> reports WrongTree.
    EXPECT_EQ(admit(makeChild(me, 9999, anchor, anchor), &theirs, 100),
        ValidityResult::RejectWrongTree);

    // Own tree, above threshold AND worse than parent -> reports the threshold.
    const AntSolutionRecord mine = makeParent(me, 3000);
    EXPECT_EQ(admit(makeChild(me, 9999, anchor, anchor), &mine, 100),
        ValidityResult::RejectBelowThreshold);

    // Passes the threshold, but worse than parent AND below the floor -> reports the parent.
    EXPECT_EQ(admit(makeChild(me, 3500, anchor, anchor), &mine, 100),
        ValidityResult::RejectLeParent);
}

// ---------------------------------------------------------------------------------------------
// Direction sweep
// ---------------------------------------------------------------------------------------------

// The whole rule set restated as one property: for a fixed parent, floor and threshold, acceptance
// must be monotone in the score - every score at or below the tightest bound is accepted, every
// score above it is rejected. An inverted comparison anywhere breaks this even if the individual
// boundary tests above were adjusted to match it.
TEST(TestAntColonyValidate, AcceptanceIsMonotoneInScore)
{
    const m256i me = makeKey(10);
    const unsigned int parentScore = 3800;
    const unsigned int floor = 3750;
    const AntSolutionRecord parent = makeParent(me, parentScore);

    // Tightest of: <= threshold, < parent, < floor.
    const unsigned int bestRejected = (parentScore < floor) ? parentScore : floor;

    bool sawAccept = false;
    for (unsigned int score = 3700; score <= 3900; score++)
    {
        const ValidityResult r = admit(makeChild(me, score), &parent, floor);
        const bool accepted = (r == ValidityResult::Valid);
        if (score < bestRejected && score <= TEST_THRESHOLD)
        {
            ASSERT_TRUE(accepted) << "score " << score << " should be accepted, got " << (int)r;
            sawAccept = true;
        }
        else
        {
            ASSERT_FALSE(accepted) << "score " << score << " should be rejected";
        }
    }
    EXPECT_TRUE(sawAccept) << "the sweep must cover the accepting region";
}
