#define NO_UEFI

#include "gtest/gtest.h"

#define ENABLE_PROFILING 0

// The bound colony, not the bare template: these tests check bpp9000's binding as well as the rules.
#include "../src/mining/ant_colony/ant_colony_bpp9000.h"

#include <vector>

static constexpr unsigned int TEST_THRESHOLD = 3838;   // BPP9000_SOLUTION_THRESHOLD_DEFAULT
// Ticks are absolute. A commit at TEST_PUBLISH_TICK lands in tick-index slot (TEST_PUBLISH_TICK -
// TEST_INITIAL_TICK), which must stay under MAX_NUMBER_OF_TICKS_PER_EPOCH (3005 on the testnet setting).
static constexpr unsigned int TEST_INITIAL_TICK = 99000;
static constexpr unsigned int TEST_PUBLISH_TICK = 100000;

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
    unsigned int childCount)
{
    return AntColonyBpp9000T::validateChild(child, parent, childCount, TEST_THRESHOLD);
}

// The packing itself is generic and tested exhaustively
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

// The threshold is checked before the parent comparison, so nodes worse than it are never stored 
TEST(TestAntColonyValidate, ThresholdIsAnUpperBoundOnError)
{
    const m256i me = makeKey(1);

    // A parent that would otherwise admit anything, so only the threshold can reject.
    const AntSolutionRecord looseParent = makeParent(me, WORST_SCORE);
    EXPECT_EQ(admit(makeChild(me, 3839), &looseParent, 0),
        ValidityResult::RejectBelowThreshold);

    // Exactly at the bound is accepted: the rule is score > threshold, not >=.
    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD), &looseParent, 0), ValidityResult::Valid);
}

TEST(TestAntColonyValidate, MustStrictlyBeatParent)
{
    const m256i me = makeKey(2);
    const AntSolutionRecord parent = makeParent(me, 3800);

    EXPECT_EQ(admit(makeChild(me, 3799), &parent, 0), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 3800), &parent, 0), ValidityResult::RejectLeParent);
    EXPECT_EQ(admit(makeChild(me, 3801), &parent, 0), ValidityResult::RejectLeParent);
}

// A root has no score of its own, so any threshold-passing child improves on it. This is what lets a
// lineage start at all.
TEST(TestAntColonyValidate, RootParentAdmitsAnyPassingScore)
{
    const m256i me = makeKey(3);

    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD), nullptr, 0), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 0), nullptr, 0), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, TEST_THRESHOLD + 1), nullptr, 0),
        ValidityResult::RejectBelowThreshold);
}

// Trees are isolated per identity: a miner cannot branch off someone else's node.
TEST(TestAntColonyValidate, CannotBranchFromAnotherIdentity)
{
    const m256i me = makeKey(4);
    const m256i someoneElse = makeKey(5);
    const AntSolutionRecord theirNode = makeParent(someoneElse, 3800);

    EXPECT_EQ(admit(makeChild(me, 3700), &theirNode, 0), ValidityResult::RejectWrongTree);

    const AntSolutionRecord myNode = makeParent(me, 3800);
    EXPECT_EQ(admit(makeChild(me, 3700), &myNode, 0), ValidityResult::Valid);
}

// Per-parent child cap. The cap is compile-time; 0 means unbound.
TEST(TestAntColonyValidate, RejectsAtTheChildCap)
{
    const m256i me = makeKey(6);
    const AntSolutionRecord parent = makeParent(me, WORST_SCORE);

    // Below the cap - and always, when unbound - a passing child is admitted.
    EXPECT_EQ(admit(makeChild(me, 3799), &parent, 0), ValidityResult::Valid);

    // At the cap it is refused. Skipped when unbound (0). The runtime copy keeps the compile-time
    // zero from tripping a constant-condition warning.
    const unsigned int cap = ANT_MAX_CHILDREN_PER_PARENT;
    if (cap != 0)
    {
        EXPECT_EQ(admit(makeChild(me, 3799), &parent, cap),
            ValidityResult::RejectMaxChildrenPerParent);
    }
}

// Freshness
TEST(TestAntColonyValidate, FreshnessWindowBoundaries)
{
    const m256i me = makeKey(7);
    const AntSolutionRecord parent = makeParent(me, WORST_SCORE);
    const unsigned int anchor = 100000;

    // Published in the same tick it anchored to: the tightest legal case.
    EXPECT_EQ(admit(makeChild(me, 3700, anchor, anchor), &parent, 0),
        ValidityResult::Valid);

    // Exactly at the window edge is still legal; one past it is not.
    EXPECT_EQ(admit(makeChild(me, 3700, anchor, anchor + ANT_PUBLISH_WINDOW_TICKS),
        &parent, 0), ValidityResult::Valid);
    EXPECT_EQ(admit(makeChild(me, 3700, anchor, anchor + ANT_PUBLISH_WINDOW_TICKS + 1),
        &parent, 0), ValidityResult::RejectStale);

    // An anchor in the future is rejected rather than wrapping the unsigned subtraction.
    EXPECT_EQ(admit(makeChild(me, 3700, anchor + 1, anchor), &parent, 0),
        ValidityResult::RejectStale);
}

// Order of checks: Freshness first, then tree isolation, then threshold, then
// parent, then the child cap.
TEST(TestAntColonyValidate, ReportsTheFirstFailingRule)
{
    const m256i me = makeKey(8);
    const m256i other = makeKey(9);
    const unsigned int anchor = 100000;
    const unsigned int stalePublish = anchor + ANT_PUBLISH_WINDOW_TICKS + 1;

    const AntSolutionRecord theirs = makeParent(other, 3000);

    // Stale AND wrong tree AND above threshold AND worse than parent -> reports Stale.
    EXPECT_EQ(admit(makeChild(me, 9999, anchor, stalePublish), &theirs, 0),
        ValidityResult::RejectStale);

    // Fresh, but wrong tree AND above threshold -> reports WrongTree.
    EXPECT_EQ(admit(makeChild(me, 9999, anchor, anchor), &theirs, 0),
        ValidityResult::RejectWrongTree);

    // Own tree, above threshold AND worse than parent -> reports the threshold.
    const AntSolutionRecord mine = makeParent(me, 3000);
    EXPECT_EQ(admit(makeChild(me, 9999, anchor, anchor), &mine, 0),
        ValidityResult::RejectBelowThreshold);

    // Passes the threshold but worse than parent -> reports the parent.
    EXPECT_EQ(admit(makeChild(me, 3500, anchor, anchor), &mine, 0),
        ValidityResult::RejectLeParent);
}

// For a fixed parent and threshold, acceptance
// must be monotone in the score - every score at or below the tightest bound is accepted, every
// score above it is rejected. An inverted comparison anywhere breaks this even if the individual
// boundary tests above were adjusted to match it.
TEST(TestAntColonyValidate, AcceptanceIsMonotoneInScore)
{
    const m256i me = makeKey(10);
    const unsigned int parentScore = 3800;
    const AntSolutionRecord parent = makeParent(me, parentScore);

    // Tightest of: <= threshold, < parent.
    const unsigned int bestRejected = parentScore;

    bool sawAccept = false;
    for (unsigned int score = 3700; score <= 3900; score++)
    {
        const ValidityResult r = admit(makeChild(me, score), &parent, 0);
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

// init() allocates ~6.2 GB, so the colony is built once for the file and re-seeded between tests.
// Lazy rather than SetUpTestSuite: that does not exist before gtest 1.10, and test.vcxproj builds
// against 1.8.1, where it would compile clean and never run.
static AntColonyBpp9000T* freshColony()
{
    static AntColonyBpp9000T colony;
    static bool allocated = false;
    static bool allocationFailed = false;

    if (!allocated && !allocationFailed)
    {
        allocationFailed = !colony.init();
        allocated = !allocationFailed;
    }
    if (allocationFailed)
    {
        return nullptr;
    }

    colony.beginEpoch(makeKey(999), TEST_INITIAL_TICK);
    colony.setErrorThreshold(TEST_THRESHOLD);
    return &colony;
}

// Commits one child of the root, returns its index or ANT_INVALID_INDEX. nonceSeed keeps calls
// distinct, since (pubkey, nonce, parentRef) is the replay key.
static long long commitRootChild(AntColonyBpp9000T* colony, const m256i& owner, unsigned int score,
    unsigned int txIdx, unsigned long long nonceSeed, unsigned int tick = 100000)
{
    AntCommitInput in;
    in.pubkey = owner;
    in.nonce = makeKey(nonceSeed);
    in.parentRef = ROOT_REF;
    in.selfRef.tick = tick;
    in.selfRef.solutionIndexInTick = txIdx;
    in.anchorTick = tick;
    in.publishTick = tick;

    AntColonyBpp9000T::Ann ann;
    setMem(&ann, sizeof(ann), 0);
    ann.lut[0] = (unsigned char)(score % 3);

    // The real hash, not a stand-in: the snapshot rebuild re-derives it from the stored network.
    unsigned int annHash;
    KangarooTwelve(&ann, sizeof(ann), &annHash, sizeof(annHash));

    const long long landsAt = (long long)colony->solutionCount();
    if (colony->commit(in, nullptr, score, ann, annHash) != ValidityResult::Valid)
    {
        return ANT_INVALID_INDEX;
    }
    return landsAt;
}

// A child of an existing node, so a test can build a lineage rather than a flat set of root children.
static long long commitChild(AntColonyBpp9000T* colony, const m256i& owner, const SolutionRef& parentRef,
    unsigned int score, unsigned int txIdx, unsigned long long nonceSeed, unsigned int tick = 100000)
{
    const AntSolutionRecord* parentRec = nullptr;
    if (colony->tryGetParent(parentRef, &parentRec) != ValidityResult::Valid)
    {
        return ANT_INVALID_INDEX;
    }

    AntCommitInput in;
    in.pubkey = owner;
    in.nonce = makeKey(nonceSeed);
    in.parentRef = parentRef;
    in.selfRef.tick = tick;
    in.selfRef.solutionIndexInTick = txIdx;
    in.anchorTick = tick;
    in.publishTick = tick;

    AntColonyBpp9000T::Ann ann;
    setMem(&ann, sizeof(ann), 0);
    ann.lut[0] = (unsigned char)(score % 3);
    unsigned int annHash;
    KangarooTwelve(&ann, sizeof(ann), &annHash, sizeof(annHash));

    const long long landsAt = (long long)colony->solutionCount();
    if (colony->commit(in, parentRec, score, ann, annHash) != ValidityResult::Valid)
    {
        return ANT_INVALID_INDEX;
    }
    return landsAt;
}

// commit() head-inserts, so children chain from newest to oldest. countChildren() walks this chain
// from the head, so it must stay intact and terminate.
TEST(TestAntColonyStore, SiblingsChainNewestFirst)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(1);
    // Same anchor tick, so all three are inside the freshness window and coexist.
    const long long a = commitRootChild(colony, me, 3800, 0, 500);
    const long long b = commitRootChild(colony, me, 3810, 1, 501);
    const long long c = commitRootChild(colony, me, 3820, 2, 502);
    ASSERT_NE(a, ANT_INVALID_INDEX);
    ASSERT_NE(b, ANT_INVALID_INDEX);
    ASSERT_NE(c, ANT_INVALID_INDEX);

    EXPECT_EQ(colony->recordAt(c)->nextSiblingIdx, (unsigned int)b);
    EXPECT_EQ(colony->recordAt(b)->nextSiblingIdx, (unsigned int)a);
    EXPECT_EQ(colony->recordAt(a)->nextSiblingIdx, NO_SIBLING);
}

// parentRef is a logical address, so it must map back to the record index.
TEST(TestAntColonyStore, SolutionRefResolvesToItsRecord)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(2);
    const long long idx = commitRootChild(colony, me, 3800, 42, 600);
    ASSERT_NE(idx, ANT_INVALID_INDEX);

    const SolutionRef ref = { TEST_PUBLISH_TICK,42 };
    EXPECT_EQ(colony->findIndexBySolutionRef(ref), idx);

    // An uncommitted ref must not resolve to a neighbour.
    const SolutionRef missing = { TEST_PUBLISH_TICK,43 };
    EXPECT_EQ(colony->findIndexBySolutionRef(missing), ANT_INVALID_INDEX);
}

// Same (pubkey, nonce, parentRef) is a replay whatever its score.
TEST(TestAntColonyStore, SameSolutionCannotCommitTwice)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(3);
    ASSERT_NE(commitRootChild(colony, me, 3800, 0, 700), ANT_INVALID_INDEX);

    EXPECT_EQ(commitRootChild(colony, me, 3700, 1, 700), ANT_INVALID_INDEX);
    EXPECT_EQ(colony->stats().rejectReplay, 1u);
    EXPECT_EQ(colony->solutionCount(), 1u);
}

// ROOT is never stored, so resolving it is Valid with a null record, not a lookup failure.
TEST(TestAntColonyStore, RootRefResolvesToValidWithNoRecord)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const AntSolutionRecord* parent = (const AntSolutionRecord*)1;   // must be overwritten
    EXPECT_EQ(colony->tryGetParent(ROOT_REF, &parent), ValidityResult::Valid);
    EXPECT_EQ(parent, nullptr);

    const SolutionRef missing = { TEST_PUBLISH_TICK,0 };
    EXPECT_EQ(colony->tryGetParent(missing, &parent), ValidityResult::RejectParentNotRegistered);
}

// Anchor ring
TEST(TestAntColonyStore, AnchorDigestRoundTrips)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i digest = makeKey(4242);
    colony->recordAnchorDigest(100000, digest);

    m256i out = m256i::zero();
    EXPECT_TRUE(colony->getAnchorDigest(100000, out));
    EXPECT_TRUE(out == digest);

    // An unrecorded tick is a miss, not whatever sits in that slot.
    EXPECT_FALSE(colony->getAnchorDigest(100001, out));
}

// A tick ANT_ANCHOR_RING_SIZE later lands in the same slot. The evicted one must miss - returning
// the new digest would score against a network the miner never used.
TEST(TestAntColonyStore, AgedOutAnchorIsAMissNotTheWrongDigest)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const unsigned int oldTick = 100000;
    const unsigned int newTick = oldTick + ANT_ANCHOR_RING_SIZE;
    const m256i oldDigest = makeKey(11);
    const m256i newDigest = makeKey(22);

    colony->recordAnchorDigest(oldTick, oldDigest);
    colony->recordAnchorDigest(newTick, newDigest);

    m256i out = m256i::zero();
    EXPECT_FALSE(colony->getAnchorDigest(oldTick, out));
    EXPECT_TRUE(colony->getAnchorDigest(newTick, out));
    EXPECT_TRUE(out == newDigest);
}

// beginEpoch() wipes the ring. It fills with ANT_ANCHOR_TICK_NONE, not zero, so tick 0 does not
// look recorded.
TEST(TestAntColonyStore, EpochResetClearsTheRing)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    colony->recordAnchorDigest(100000, makeKey(7));
    m256i out = m256i::zero();
    ASSERT_TRUE(colony->getAnchorDigest(100000, out));

    colony->beginEpoch(makeKey(999), TEST_INITIAL_TICK);
    EXPECT_FALSE(colony->getAnchorDigest(100000, out));
    EXPECT_FALSE(colony->getAnchorDigest(0, out));
}


// Snapshot test cases

static constexpr unsigned short TEST_EPOCH = 200;
static const m256i TEST_ROOT_SEED = makeKey(999);   // what freshColony() seeds with

// Save, wipe, load. beginEpoch() clears everything the load has to bring back, so anything that
// survives came out of the files.
static bool saveWipeLoad(AntColonyBpp9000T* colony)
{
    if (!colony->saveSnapshot(TEST_EPOCH, NULL, TEST_INITIAL_TICK))
    {
        return false;
    }
    colony->beginEpoch(TEST_ROOT_SEED, TEST_INITIAL_TICK);
    colony->setErrorThreshold(TEST_THRESHOLD);
    return colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK);
}

// Records and the tick index come back, and the sibling chain is rebuilt to the same shape commit()
// built. Only the records are on disk - nextSiblingIdx is replayed, so matching the pre-save chain
// is what proves the replay reproduces the head-insert.
TEST(TestAntColonySnapshot, RoundTripRestoresTheTree)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(1);
    ASSERT_NE(commitRootChild(colony, me, 3800, 0, 500), ANT_INVALID_INDEX);
    ASSERT_NE(commitRootChild(colony, me, 3810, 1, 501), ANT_INVALID_INDEX);
    ASSERT_NE(commitRootChild(colony, me, 3820, 2, 502), ANT_INVALID_INDEX);

    ASSERT_TRUE(saveWipeLoad(colony));

    ASSERT_EQ(colony->solutionCount(), 3u);
    EXPECT_EQ(colony->recordAt(2)->nextSiblingIdx, 1u);
    EXPECT_EQ(colony->recordAt(1)->nextSiblingIdx, 0u);
    EXPECT_EQ(colony->recordAt(0)->nextSiblingIdx, NO_SIBLING);
    EXPECT_EQ(colony->recordAt(1)->score, 3810u);
    EXPECT_TRUE(colony->recordAt(1)->pubkey == me);

    // The tick index is derived too, so resolving a logical ref proves it was rebuilt.
    const SolutionRef ref = { TEST_PUBLISH_TICK,1 };
    EXPECT_EQ(colony->findIndexBySolutionRef(ref), 1LL);
}

// The stored network must come back byte for byte, otherwise children score against a parent the
// rest of the network does not have.
TEST(TestAntColonySnapshot, RoundTripRestoresTheStoredNetwork)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(2);
    ASSERT_NE(commitRootChild(colony, me, 3800, 0, 900), ANT_INVALID_INDEX);

    AntColonyBpp9000T::Ann before;
    ASSERT_TRUE(colony->annOfNonRoot(*colony->recordAt(0), before));

    ASSERT_TRUE(saveWipeLoad(colony));

    AntColonyBpp9000T::Ann after;
    ASSERT_TRUE(colony->annOfNonRoot(*colony->recordAt(0), after));
    for (unsigned long long i = 0; i < sizeof(before); i++)
    {
        ASSERT_EQ(after.lut[i], before.lut[i]) << "entry " << i;
    }
}

// The dedup set is not written to disk. If the rebuild misses it, a restarted node re-accepts
// solutions it already has.
TEST(TestAntColonySnapshot, DedupIsRebuiltSoReplaysStillFail)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(3);
    ASSERT_NE(commitRootChild(colony, me, 3800, 0, 600), ANT_INVALID_INDEX);
    ASSERT_TRUE(saveWipeLoad(colony));

    // Same (pubkey, nonce, parentRef) as before the restart.
    EXPECT_EQ(commitRootChild(colony, me, 3700, 1, 600), ANT_INVALID_INDEX);
    EXPECT_EQ(colony->solutionCount(), 1u);
}

// A cold ring would reject solutions anchored before the restart that peers accept.
TEST(TestAntColonySnapshot, AnchorRingSurvivesTheRoundTrip)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i digest = makeKey(4242);
    colony->recordAnchorDigest(100000, digest);
    ASSERT_TRUE(saveWipeLoad(colony));

    m256i out = m256i::zero();
    EXPECT_TRUE(colony->getAnchorDigest(100000, out));
    EXPECT_TRUE(out == digest);
    EXPECT_FALSE(colony->getAnchorDigest(100001, out));
}

// The seed and threshold are supplied by the node, not read from the file. A disagreement means the
// colony files and the node state are from different moments, so the tree is refused.
TEST(TestAntColonySnapshot, FileMustAgreeWithTheNodeState)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    ASSERT_NE(commitRootChild(colony, makeKey(4), 3800, 0, 700), ANT_INVALID_INDEX);
    ASSERT_TRUE(colony->saveSnapshot(TEST_EPOCH, NULL, TEST_INITIAL_TICK));

    EXPECT_FALSE(colony->loadSnapshot(TEST_EPOCH, NULL, makeKey(12345), TEST_THRESHOLD, TEST_INITIAL_TICK));
    EXPECT_FALSE(colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD + 1, TEST_INITIAL_TICK));

    // A different base would resolve every parentRef to the wrong record.
    EXPECT_FALSE(colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK + 1));

    // The epoch is part of the file name, so a different one finds no snapshot at all.
    EXPECT_FALSE(colony->loadSnapshot((unsigned short)(TEST_EPOCH + 1), NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK));

    // And the matching one still loads, so the refusals above were the checks and not a bad file.
    EXPECT_TRUE(colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK));
}

// Those refusals all happen while only the meta has been read, so the tree the node is already
// running on must be left alone.
TEST(TestAntColonySnapshot, RefusedLoadLeavesTheRunningTreeIntact)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    const m256i me = makeKey(5);
    ASSERT_NE(commitRootChild(colony, me, 3800, 0, 800), ANT_INVALID_INDEX);
    ASSERT_TRUE(colony->saveSnapshot(TEST_EPOCH, NULL, TEST_INITIAL_TICK));
    ASSERT_NE(commitRootChild(colony, me, 3790, 1, 801), ANT_INVALID_INDEX);
    ASSERT_EQ(colony->solutionCount(), 2u);

    EXPECT_FALSE(colony->loadSnapshot(TEST_EPOCH, NULL, makeKey(12345), TEST_THRESHOLD, TEST_INITIAL_TICK));
    EXPECT_EQ(colony->solutionCount(), 2u);
}

// An empty colony still writes all three files, so an operator's backup is always the same set and a
// short one means a lost file rather than an empty epoch.
TEST(TestAntColonySnapshot, EmptyColonyWritesTheFullFileSet)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    ASSERT_EQ(colony->solutionCount(), 0u);

    // Clear whatever an earlier test left at this epoch, so the files below can only come from the
    // save under test.
    antSnapshotNameForEpoch(TEST_EPOCH);
    _wremove(ANT_SNAPSHOT_HEADER_FILENAME);
    _wremove(ANT_SNAPSHOT_RECORDS_FILENAME);
    _wremove(ANT_SNAPSHOT_POOL_FILENAME);

    ASSERT_TRUE(colony->saveSnapshot(TEST_EPOCH, NULL, TEST_INITIAL_TICK));

    // All three are written. Records and pool hold a full slot even when empty, so neither is zero
    // length; the header always carries the meta, anchor ring and export set.
    AntColonySnapshotMeta metaSlot;
    AntSolutionRecord recordSlot;
    AntColonyBpp9000T::PackedAnn poolSlot;
    EXPECT_EQ(load(ANT_SNAPSHOT_HEADER_FILENAME, sizeof(metaSlot), (unsigned char*)&metaSlot),
        (long long)sizeof(metaSlot));
    EXPECT_EQ(load(ANT_SNAPSHOT_RECORDS_FILENAME, sizeof(recordSlot), (unsigned char*)&recordSlot),
        (long long)sizeof(recordSlot));
    EXPECT_EQ(load(ANT_SNAPSHOT_POOL_FILENAME, sizeof(poolSlot), (unsigned char*)&poolSlot),
        (long long)sizeof(poolSlot));

    EXPECT_TRUE(colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK));
    EXPECT_EQ(colony->solutionCount(), 0u);
}

// childAnnHash is the only thing tying a record to its stored network, so it is the only check on
// the pool file. Overwrite the pool behind the colony's back and the load must refuse.
TEST(TestAntColonySnapshot, CorruptedPoolIsRefused)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.2 GB";

    // score % 3 == 2, so an all-zero network is not the one this record hashes to.
    ASSERT_NE(commitRootChild(colony, makeKey(6), 3800, 0, 1000), ANT_INVALID_INDEX);
    ASSERT_TRUE(colony->saveSnapshot(TEST_EPOCH, NULL, TEST_INITIAL_TICK));

    AntColonyBpp9000T::PackedAnn junk;
    setMem(&junk, sizeof(junk), 0);
    ASSERT_EQ(save(ANT_SNAPSHOT_POOL_FILENAME, sizeof(junk), (unsigned char*)&junk),
        (long long)sizeof(junk));

    EXPECT_FALSE(colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK));

    // The pool is read after the meta checks pass, so a refusal here does clear the colony.
    EXPECT_EQ(colony->solutionCount(), 0u);
}

// ---------------------------------------------------------------------------------------------
// Replay cache

// A key whose four components are all distinct, so a slot function that ignores one still separates
// these.
static AntColonyBpp9000T::ReplayKey makeReplayKey(unsigned long long n)
{
    AntColonyBpp9000T::ReplayKey k;
    k.pubkey = makeKey(n);
    k.nonce = makeKey(n + 1000);
    k.parentAnnHash = makeKey(n + 2000);
    k.anchorDigest = makeKey(n + 3000);
    return k;
}

// lut[0] carries n so two networks are distinguishable; the rest stays a legal trit.
static AntColonyBpp9000T::Ann makeAnn(unsigned char n)
{
    AntColonyBpp9000T::Ann a;
    setMem(&a, sizeof(a), 0);
    a.lut[0] = (unsigned char)(n % 3);
    a.lut[1] = (unsigned char)((n / 3) % 3);
    return a;
}

static bool annEquals(const AntColonyBpp9000T::Ann& a, const AntColonyBpp9000T::Ann& b)
{
    for (unsigned long long i = 0; i < sizeof(a); i++)
    {
        if (a.lut[i] != b.lut[i])
        {
            return false;
        }
    }
    return true;
}

// The score and the network both come back. The network matters as much as the score: commit()
// stores it and childAnnHash folds it into resourceTestingDigest.
TEST(TestAntColonyReplayCache, StoresAndReturnsScoreAndNetwork)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.9 GB";

    const AntColonyBpp9000T::ReplayKey key = makeReplayKey(1);
    const AntColonyBpp9000T::Ann ann = makeAnn(7);
    colony->putReplayScore(key, 3800, ann);

    unsigned int score = 0;
    AntColonyBpp9000T::Ann out;
    setMem(&out, sizeof(out), 0xFF);
    ASSERT_TRUE(colony->tryGetReplayScore(key, score, out));
    EXPECT_EQ(score, 3800u);
    EXPECT_TRUE(annEquals(out, ann));
}

// Every component is part of the key, so changing any one of them must miss. Missing one would
// return a score computed from different inputs.
TEST(TestAntColonyReplayCache, EveryKeyComponentIsPartOfTheLookup)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.9 GB";

    const AntColonyBpp9000T::ReplayKey key = makeReplayKey(2);
    colony->putReplayScore(key, 3800, makeAnn(1));

    unsigned int score = 0;
    AntColonyBpp9000T::Ann out;
    for (int component = 0; component < 4; component++)
    {
        AntColonyBpp9000T::ReplayKey altered = key;
        switch (component)
        {
        case 0: altered.pubkey = makeKey(90001); break;
        case 1: altered.nonce = makeKey(90002); break;
        case 2: altered.parentAnnHash = makeKey(90003); break;
        case 3: altered.anchorDigest = makeKey(90004); break;
        }
        EXPECT_FALSE(colony->tryGetReplayScore(altered, score, out)) << "component " << component;
    }
    EXPECT_TRUE(colony->tryGetReplayScore(key, score, out));
}

// A new epoch changes every root and anchor digest, so no entry could hit anyway; keeping them would
// just hold slots.
TEST(TestAntColonyReplayCache, BeginEpochClearsIt)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.9 GB";

    const AntColonyBpp9000T::ReplayKey key = makeReplayKey(3);
    colony->putReplayScore(key, 3800, makeAnn(2));
    ASSERT_EQ(colony->replayCacheOccupancy(), 1u);

    colony->beginEpoch(TEST_ROOT_SEED, TEST_INITIAL_TICK);

    unsigned int score = 0;
    AntColonyBpp9000T::Ann out;
    EXPECT_FALSE(colony->tryGetReplayScore(key, score, out));
    EXPECT_EQ(colony->replayCacheOccupancy(), 0u);
}

// loadSnapshot() calls reset(), and the catch-up that follows a restore is the one moment the cache
// is worth most. Losing it there would defeat the feature.
TEST(TestAntColonyReplayCache, SurvivesResetAndSnapshotLoad)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.9 GB";

    const AntColonyBpp9000T::ReplayKey key = makeReplayKey(4);
    colony->putReplayScore(key, 3800, makeAnn(3));
    ASSERT_TRUE(colony->saveSnapshot(TEST_EPOCH, NULL, TEST_INITIAL_TICK));
    ASSERT_TRUE(colony->loadSnapshot(TEST_EPOCH, NULL, TEST_ROOT_SEED, TEST_THRESHOLD, TEST_INITIAL_TICK));

    unsigned int score = 0;
    AntColonyBpp9000T::Ann out;
    EXPECT_TRUE(colony->tryGetReplayScore(key, score, out));
    EXPECT_EQ(score, 3800u);
}

// The file is the table verbatim, so this checks that entries survive the write and stay findable
// under their own keys. Enough of them that collisions and evictions are in play. One save writes
// the whole ANT_REPLAY_CACHE_BYTES table, so this is the only test here that touches a file.
TEST(TestAntColonyReplayCache, RoundTripsThroughAFile)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.9 GB";

    constexpr unsigned int COUNT = 500;
    for (unsigned int i = 0; i < COUNT; i++)
    {
        colony->putReplayScore(makeReplayKey(10000 + i), 3000 + i, makeAnn((unsigned char)i));
    }
    ASSERT_EQ(colony->replayCacheOccupancy(), COUNT);
    ASSERT_TRUE(colony->saveReplayCache(TEST_EPOCH, NULL));

    colony->clearReplayCache();
    ASSERT_EQ(colony->replayCacheOccupancy(), 0u);

    ASSERT_TRUE(colony->loadReplayCache(TEST_EPOCH, NULL));
    EXPECT_EQ(colony->replayCacheOccupancy(), COUNT);

    unsigned int score = 0;
    AntColonyBpp9000T::Ann out;
    for (unsigned int i = 0; i < COUNT; i++)
    {
        ASSERT_TRUE(colony->tryGetReplayScore(makeReplayKey(10000 + i), score, out)) << "entry " << i;
        ASSERT_EQ(score, 3000 + i) << "entry " << i;
        ASSERT_TRUE(annEquals(out, makeAnn((unsigned char)i))) << "entry " << i;
    }
}

// No cache is the normal state at the start of an epoch, so it must report a miss and leave an empty
// table rather than fail the boot.
TEST(TestAntColonyReplayCache, AbsentFileIsNotAnError)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.9 GB";

    colony->putReplayScore(makeReplayKey(7), 3800, makeAnn(6));
    EXPECT_FALSE(colony->loadReplayCache((unsigned short)(TEST_EPOCH + 77), NULL));
    EXPECT_EQ(colony->replayCacheOccupancy(), 0u);

    unsigned int score = 0;
    AntColonyBpp9000T::Ann out;
    EXPECT_FALSE(colony->tryGetReplayScore(makeReplayKey(7), score, out));
}

// ---------------------------------------------------------------------------------------------
// Export best ANN at the end of epoch

// The file layout: one header, then entryCount of these.
struct ExportFileEntry
{
    AntColonyExportEntry meta;
    AntColonyBpp9000T::Ann ann;
};

// Reads antColonySolutions.eoe back. Header first, since only it says how long the body is.
static bool readExport(AntColonyExportHeader& header, std::vector<ExportFileEntry>& entries)
{
    if (load(ANT_COLONY_SOLUTIONS_EOE_FILENAME, sizeof(header), (unsigned char*)&header)
        != (long long)sizeof(header))
    {
        return false;
    }
    entries.clear();
    if (header.entryCount == 0)
    {
        return true;
    }

    const unsigned long long total = sizeof(header)
        + (unsigned long long)header.entryCount * sizeof(ExportFileEntry);
    std::vector<unsigned char> raw(total);
    if (load(ANT_COLONY_SOLUTIONS_EOE_FILENAME, total, raw.data()) != (long long)total)
    {
        return false;
    }
    entries.resize(header.entryCount);
    copyMem(entries.data(), raw.data() + sizeof(header), total - sizeof(header));
    return true;
}

// More solutions than the file holds, so the cap, the eviction and the ordering are all exercised.
TEST(TestAntColonyExport, KeepsTheLowestScoresInOrder)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.6 GB";

    constexpr unsigned int COMMITTED = ANT_EXPORT_MAX_SOLUTIONS + 24;
    // One identity per solution so the per-parent child cap never binds - the export set is what is
    // under test here, not the tree shape.
    for (unsigned int i = 0; i < COMMITTED; i++)
    {
        ASSERT_NE(commitRootChild(colony, makeKey(1 + i), 3000 + i, i, 5000 + i), ANT_INVALID_INDEX) << "commit " << i;
    }
    ASSERT_TRUE(colony->exportBestSolutions(TEST_EPOCH, NULL));

    AntColonyExportHeader header;
    std::vector<ExportFileEntry> entries;
    ASSERT_TRUE(readExport(header, entries));

    EXPECT_EQ(header.entryCount, ANT_EXPORT_MAX_SOLUTIONS);
    EXPECT_EQ(header.solutionCount, COMMITTED);
    EXPECT_EQ(header.entrySizeBytes, (unsigned int)sizeof(AntColonyExportEntry));
    EXPECT_EQ(header.annSizeBytes, (unsigned int)sizeof(AntColonyBpp9000T::Ann));
    ASSERT_EQ(entries.size(), (size_t)ANT_EXPORT_MAX_SOLUTIONS);

    // The 676 lowest of 3000..3699, so exactly 3000..3675, ascending.
    EXPECT_EQ(entries[0].meta.score, 3000u) << "entry 0 must be the best network of the epoch";
    EXPECT_EQ(entries[ANT_EXPORT_MAX_SOLUTIONS - 1].meta.score, 3000u + ANT_EXPORT_MAX_SOLUTIONS - 1);
    for (unsigned int i = 1; i < entries.size(); i++)
    {
        ASSERT_LE(entries[i - 1].meta.score, entries[i].meta.score) << "not ascending at " << i;
    }
}

// Below the cap the file holds everything, still ordered - and the scores are committed descending
// here, so every insert lands at the front and the shift path is the one being used.
TEST(TestAntColonyExport, OrdersFewerThanTheCap)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.6 GB";

    constexpr unsigned int COUNT = 40;
    // One identity per solution so the per-parent child cap never binds.
    for (unsigned int i = 0; i < COUNT; i++)
    {
        ASSERT_NE(commitRootChild(colony, makeKey(2000 + i), 3800 - i, i, 6000 + i), ANT_INVALID_INDEX);
    }
    ASSERT_TRUE(colony->exportBestSolutions(TEST_EPOCH, NULL));

    AntColonyExportHeader header;
    std::vector<ExportFileEntry> entries;
    ASSERT_TRUE(readExport(header, entries));

    ASSERT_EQ(header.entryCount, COUNT);
    EXPECT_EQ(entries[0].meta.score, 3800u - (COUNT - 1));
    for (unsigned int i = 1; i < entries.size(); i++)
    {
        ASSERT_LE(entries[i - 1].meta.score, entries[i].meta.score) << "not ascending at " << i;
    }
}

// Equal scores keep the incumbent, so the earlier solution ranks first. Without a total order two
// nodes with the same solutions could write different files.
TEST(TestAntColonyExport, TiesKeepTheEarlierSolution)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.6 GB";

    const m256i first = makeKey(3);
    const m256i second = makeKey(4);
    ASSERT_NE(commitRootChild(colony, first, 3500, 0, 6100), ANT_INVALID_INDEX);
    ASSERT_NE(commitRootChild(colony, second, 3500, 1, 6101), ANT_INVALID_INDEX);
    ASSERT_TRUE(colony->exportBestSolutions(TEST_EPOCH, NULL));

    AntColonyExportHeader header;
    std::vector<ExportFileEntry> entries;
    ASSERT_TRUE(readExport(header, entries));

    ASSERT_EQ(header.entryCount, 2u);
    EXPECT_TRUE(entries[0].meta.pubkey == first);
    EXPECT_TRUE(entries[1].meta.pubkey == second);
}

// The stored network has to survive the round trip, a wrong ANN here is a wrong harvest, and
// nothing downstream would notice.
TEST(TestAntColonyExport, CarriesTheNetworkAndItsDepth)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.6 GB";

    const m256i me = makeKey(5);
    ASSERT_NE(commitRootChild(colony, me, 3800, 0, 6200), ANT_INVALID_INDEX);
    const SolutionRef aRef = { TEST_PUBLISH_TICK,0 };
    ASSERT_NE(commitChild(colony, me, aRef, 3700, 1, 6201), ANT_INVALID_INDEX);
    ASSERT_TRUE(colony->exportBestSolutions(TEST_EPOCH, NULL));

    AntColonyExportHeader header;
    std::vector<ExportFileEntry> entries;
    ASSERT_TRUE(readExport(header, entries));
    ASSERT_EQ(header.entryCount, 2u);

    // Best first: the depth-2 child at 3700, then its depth-1 parent at 3800.
    EXPECT_EQ(entries[0].meta.score, 3700u);
    EXPECT_EQ(entries[0].meta.depth, 2u);
    EXPECT_EQ(entries[1].meta.score, 3800u);
    EXPECT_EQ(entries[1].meta.depth, 1u);

    AntColonyBpp9000T::Ann expected;
    ASSERT_TRUE(colony->annOfNonRoot(*colony->recordAt(1), expected));
    for (unsigned long long i = 0; i < sizeof(expected); i++)
    {
        ASSERT_EQ(entries[0].ann.lut[i], expected.lut[i]) << "genome byte " << i;
    }
}

// The set holds networks the records cannot reproduce once the store is full, so it is saved rather
// than rebuilt. If that file went missing the export would come back empty after a restart.
TEST(TestAntColonyExport, SurvivesASnapshot)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.6 GB";

    const m256i me = makeKey(6);
    for (unsigned int i = 0; i < 10; i++)
    {
        ASSERT_NE(commitRootChild(colony, me, 3700 + i, i, 6300 + i), ANT_INVALID_INDEX);
    }
    ASSERT_TRUE(saveWipeLoad(colony));
    ASSERT_TRUE(colony->exportBestSolutions(TEST_EPOCH, NULL));

    AntColonyExportHeader header;
    std::vector<ExportFileEntry> entries;
    ASSERT_TRUE(readExport(header, entries));

    ASSERT_EQ(header.entryCount, 10u);
    EXPECT_EQ(entries[0].meta.score, 3700u);
    EXPECT_EQ(entries[9].meta.score, 3709u);
}

// A new epoch starts with nothing to export, and the file must say so rather than carry last epoch's.
TEST(TestAntColonyExport, BeginEpochClearsIt)
{
    AntColonyBpp9000T* colony = freshColony();
    ASSERT_NE(colony, nullptr) << "colony init failed; needs ~6.6 GB";

    ASSERT_NE(commitRootChild(colony, makeKey(7), 3500, 0, 6400), ANT_INVALID_INDEX);
    colony->beginEpoch(TEST_ROOT_SEED, TEST_INITIAL_TICK);
    ASSERT_TRUE(colony->exportBestSolutions(TEST_EPOCH, NULL));

    AntColonyExportHeader header;
    std::vector<ExportFileEntry> entries;
    ASSERT_TRUE(readExport(header, entries));
    EXPECT_EQ(header.entryCount, 0u);
    EXPECT_EQ(header.solutionCount, 0u);
}
