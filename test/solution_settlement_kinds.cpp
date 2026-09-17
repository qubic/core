#define NO_UEFI

#include "gtest/gtest.h"

// Pulls in ant_colony_bpp9000.h (the bound colony) and solution_settlement_window.h; every result
// below is injected via the stub scoreFn, never walked, so the real scorer is never exercised.
#include "../src/mining/solution_settlement_kinds.h"

static constexpr unsigned int TEST_THRESHOLD = 3838;   // BPP9000_SOLUTION_THRESHOLD_DEFAULT
static constexpr unsigned int TEST_INITIAL_TICK = 99000;
static constexpr unsigned short ANT_KIND = AntColonyMiningSolutionTransaction::transactionType();
static constexpr unsigned short OTHER_KIND = MiningSolutionTransaction::transactionType();
static constexpr unsigned int TEST_SETTLED_THROUGH = TEST_INITIAL_TICK - 1;
static constexpr unsigned int TEST_TICK = TEST_INITIAL_TICK + 1000;

static m256i makeKey(unsigned long long n)
{
    m256i k = m256i::zero();
    k.m256i_u64[0] = n + 1;
    return k;
}

// Appends an ant solution record directly (bypassing recordAntSolution()'s own dedup/future-parent
// checks, which live in qubic.cpp, not here). claimedScore doubles as this test's control for what
// the stub scoreFn below produces: 0 means "scores invalid" (an unscorable ring parent),
// ENCODING_DOOMED means "scores valid but will fail commit()", anything else becomes that Rating's
// error at shift 0 with wouldCommit set.
static constexpr unsigned int ENCODING_DOOMED = 0xFFFFFFFEU;
static bool appendAntSlot(SolutionSettlementWindow& window, unsigned int tick, unsigned int txIndex,
    const m256i& sourcePublicKey, unsigned int claimedScoreEncoding)
{
    AntSolutionRecordCapture record;
    setMem(&record, sizeof(record), 0);
    record.sourcePublicKey = sourcePublicKey;
    record.claimedScore = claimedScoreEncoding;
    return window.append(tick, txIndex, ANT_KIND, &record, sizeof(record));
}

// Registered scoreFn for ANT_KIND in these tests: turns the record's claimedScore encoding into a
// deterministic AntSolutionResult, so a test controls exactly what a "ring-scored" slot resolves to
// without running the real scorer.
static void testAntScore(SettlementSlot& slot, unsigned long long)
{
    AntSolutionRecordCapture record;
    copyMem(&record, slot.record, sizeof(record));

    AntSolutionResult result;
    setMem(&result, sizeof(result), 0);
    if (record.claimedScore == 0)
    {
        result.rating = score_engine::Rating::worst();   // invalid: an unscorable ring parent
    }
    else if (record.claimedScore == ENCODING_DOOMED)
    {
        // Scored fine, but predicted to fail commit(): a real ANN nobody may build on.
        result.rating = score_engine::Rating{ TEST_THRESHOLD, 0 };
        result.depth = 1;
        result.wouldCommit = 0;
    }
    else
    {
        result.rating = score_engine::Rating{ record.claimedScore, 0 };
        result.depth = 1;
        result.wouldCommit = 1;
    }
    copyMem(slot.result, &result, sizeof(result));
}

static void testAntSettle(const SettlementSlot&, unsigned long long)
{
}

// Also registers OTHER_KIND with a no-op pair, matching the real registry's two-kind shape and
// letting a lookup() on a non-ant slot (for the "not even the right kind" case) behave realistically.
static void otherScore(SettlementSlot&, unsigned long long)
{
}
static void otherSettle(const SettlementSlot&, unsigned long long)
{
}

// init()s both a colony (~6.2 GB) and a settlement window (~2 GB) once for the whole file, resetting
// both between tests - the same lazy-singleton shape as test/ant_colony.cpp and
// test/solution_settlement_window.cpp, combined because resolveAntParent() needs both.
struct GlueFixture
{
    AntColonyBpp9000T colony;
    SolutionSettlementWindow window;
    AntColonyBpp9000T::Ann scratchAnn;
};

static GlueFixture* freshFixture()
{
    static GlueFixture fixture;
    static bool allocated = false;
    static bool allocationFailed = false;

    if (!allocated && !allocationFailed)
    {
        allocationFailed = !fixture.colony.init() || !fixture.window.init();
        if (!allocationFailed)
        {
            allocationFailed = !fixture.window.registerKind(ANT_KIND, testAntScore, testAntSettle)
                || !fixture.window.registerKind(OTHER_KIND, otherScore, otherSettle);
        }
        allocated = !allocationFailed;
    }
    if (allocationFailed)
    {
        return nullptr;
    }

    fixture.colony.beginEpoch(makeKey(999), TEST_INITIAL_TICK);
    fixture.colony.setErrorThreshold(TEST_THRESHOLD);
    fixture.window.reset(TEST_SETTLED_THROUGH);
    setMem(&fixture.scratchAnn, sizeof(fixture.scratchAnn), 0);
    return &fixture;
}

// Commits one child of the root directly into the colony (bypassing commit()'s caller-side checks
// in qubic.cpp), so a test has a real tree record to resolve against.
static long long commitRootChild(AntColonyBpp9000T& colony, const m256i& owner, unsigned int score,
    unsigned int tick, unsigned int txIdx, unsigned long long nonceSeed)
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
    unsigned int annHash;
    KangarooTwelve(&ann, sizeof(ann), &annHash, sizeof(annHash));

    const long long landsAt = (long long)colony.solutionCount();
    if (colony.commit(in, nullptr, score_engine::Rating{ score, 0 }, ann, annHash) != ValidityResult::Valid)
    {
        return ANT_INVALID_INDEX;
    }
    return landsAt;
}

// ---------------------------------------------------------------------------------------------
// resolveAntParent() status matrix

TEST(TestResolveAntParent, RootIsReadyWithWorstRatingAndNoAnn)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ROOT_REF, /*allowUnsettled=*/false, view);

    EXPECT_EQ(view.status, AntParentReady);
    EXPECT_EQ(view.ann, nullptr);
    EXPECT_EQ(view.treeRecord, nullptr);
    EXPECT_EQ(view.depth, 0u);
    EXPECT_TRUE(view.wouldCommit);
    EXPECT_FALSE(view.rating.isValid());
}

TEST(TestResolveAntParent, TreeParentResolvesFromTheRecord)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(1);
    ASSERT_NE(commitRootChild(fx->colony, owner, 3800, TEST_TICK, 0, 500), ANT_INVALID_INDEX);
    const SolutionRef ref{ TEST_TICK, 0 };

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/false, view);

    EXPECT_EQ(view.status, AntParentReady);
    ASSERT_NE(view.ann, nullptr);
    ASSERT_NE(view.treeRecord, nullptr);
    EXPECT_TRUE(view.treeRecord->pubkey == owner);
    EXPECT_EQ(view.rating.error, 3800u);
    EXPECT_EQ(view.shift, 0u);
    EXPECT_EQ(view.depth, 1u);
    EXPECT_TRUE(view.wouldCommit);
    EXPECT_TRUE(view.pubkey == owner);
}

TEST(TestResolveAntParent, RingScoredParentResolvesFromItsResult)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(2);
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, owner, 4200));
    ASSERT_TRUE(fx->window.workerStep(1));   // runs testAntScore, rating = {4200, 0}, wouldCommit = true
    const SolutionRef ref{ TEST_TICK, 0 };

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/true, view);

    EXPECT_EQ(view.status, AntParentReady);
    ASSERT_NE(view.ann, nullptr);
    EXPECT_EQ(view.treeRecord, nullptr);   // ring-pending, not yet a real record
    EXPECT_EQ(view.rating.error, 4200u);
    EXPECT_EQ(view.depth, 1u);
    EXPECT_TRUE(view.wouldCommit);
    EXPECT_TRUE(view.pubkey == owner);
}

TEST(TestResolveAntParent, RingUnscoredParentIsPending)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, makeKey(3), 4200));
    const SolutionRef ref{ TEST_TICK, 0 };

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/true, view);

    EXPECT_EQ(view.status, AntParentPending);
}

TEST(TestResolveAntParent, RingParentThatScoredInvalidIsAbsent)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    // claimedScore == 0 is this test's encoding for "the stub scorer produced an invalid rating" -
    // a timed-out or non-canonical walk, same as a genuinely unscorable solution.
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, makeKey(4), 0));
    ASSERT_TRUE(fx->window.workerStep(1));
    const SolutionRef ref{ TEST_TICK, 0 };

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/true, view);

    EXPECT_EQ(view.status, AntParentAbsent);
}

TEST(TestResolveAntParent, MissingParentIsAbsentWhetherOrNotRingIsAllowed)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const SolutionRef ref{ TEST_TICK, 0 };   // never appended, never committed

    AntParentView viewRing;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/true, viewRing);
    EXPECT_EQ(viewRing.status, AntParentAbsent);

    AntParentView viewNoRing;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/false, viewNoRing);
    EXPECT_EQ(viewNoRing.status, AntParentAbsent);
}

// resolveAntParent must not return Pending at settlement. Settlement
// always passes allowUnsettled = false, so a slot that WOULD be Pending with the ring allowed must
// instead fall through to the tree - and since this ref is not there either, Absent, never Pending.
TEST(TestResolveAntParent, NeverReturnsPendingWithAllowRingFalse)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, makeKey(5), 4200));   // left unscored
    const SolutionRef ref{ TEST_TICK, 0 };

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/false, view);

    EXPECT_NE(view.status, AntParentPending);
    EXPECT_EQ(view.status, AntParentAbsent);
}

// A slot of a different kind at the same (tick, txIndex) must not be mistaken for an ant parent.
TEST(TestResolveAntParent, SlotOfAnotherKindAtTheSameRefIsNotAMatch)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    unsigned char otherRecord[8];
    setMem(otherRecord, sizeof(otherRecord), 0);
    ASSERT_TRUE(fx->window.append(TEST_TICK, 0, OTHER_KIND, otherRecord, sizeof(otherRecord)));
    const SolutionRef ref{ TEST_TICK, 0 };

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ref, /*allowUnsettled=*/true, view);

    EXPECT_EQ(view.status, AntParentAbsent);
}

// ---------------------------------------------------------------------------------------------
// antParentRecordForValidation()

// The bar every node-local reader applies before treating a parent as buildable. Ready is not it:
// a scored parent predicted to fail commit() has a real ANN, and a child of it would cost this
// computor a deposit settlement is going to reject.
TEST(TestAntParentMineable, TrueForRootTreeAndRingParentsThatWouldCommit)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView root;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ROOT_REF, /*allowUnsettled=*/false, root);
    EXPECT_TRUE(antParentMineable(root));

    const m256i treeOwner = makeKey(20);
    ASSERT_NE(commitRootChild(fx->colony, treeOwner, 3800, TEST_TICK, 0, 500), ANT_INVALID_INDEX);
    AntParentView tree;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/false, tree);
    EXPECT_TRUE(antParentMineable(tree));

    const m256i ringOwner = makeKey(21);
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK + 1, 0, ringOwner, 3700));
    ASSERT_TRUE(fx->window.workerStep(1));
    AntParentView ring;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK + 1, 0 },
        /*allowUnsettled=*/true, ring);
    ASSERT_EQ(ring.status, AntParentReady);
    EXPECT_TRUE(antParentMineable(ring));
}

TEST(TestAntParentMineable, FalseForAScoredParentThatWouldNotCommit)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(22);
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, owner, ENCODING_DOOMED));
    ASSERT_TRUE(fx->window.workerStep(1));

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/true, view);

    EXPECT_EQ(view.status, AntParentReady);   // scored, with a usable ANN
    EXPECT_TRUE(view.rating.isValid());
    EXPECT_FALSE(view.wouldCommit);
    EXPECT_FALSE(antParentMineable(view));    // but nothing may be built on it
}

TEST(TestAntParentMineable, FalseForPendingAndAbsentParents)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(23);
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, owner, 4000));   // appended, not scored

    AntParentView pending;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/true, pending);
    ASSERT_EQ(pending.status, AntParentPending);
    EXPECT_FALSE(antParentMineable(pending));

    AntParentView absent;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK + 99, 7 },
        /*allowUnsettled=*/true, absent);
    ASSERT_EQ(absent.status, AntParentAbsent);
    EXPECT_FALSE(antParentMineable(absent));
}

TEST(TestAntParentRecordForValidation, UsesTheTreeRecordDirectlyWhenPresent)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(6);
    ASSERT_NE(commitRootChild(fx->colony, owner, 3800, TEST_TICK, 0, 501), ANT_INVALID_INDEX);
    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/false, view);
    ASSERT_EQ(view.status, AntParentReady);

    AntSolutionRecord scratch;
    const AntSolutionRecord* used = antParentRecordForValidation(view, scratch);
    EXPECT_EQ(used, view.treeRecord);
}

TEST(TestAntParentRecordForValidation, BuildsAScratchRecordForARingPendingParent)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(7);
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, owner, 4200));
    ASSERT_TRUE(fx->window.workerStep(1));
    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/true, view);
    ASSERT_EQ(view.status, AntParentReady);
    EXPECT_EQ(view.treeRecord, nullptr);

    AntSolutionRecord scratch;
    const AntSolutionRecord* used = antParentRecordForValidation(view, scratch);
    ASSERT_EQ(used, &scratch);
    EXPECT_TRUE(scratch.pubkey == owner);
    EXPECT_EQ(scratch.score, 4200u);
    EXPECT_EQ(scratch.shift, 0u);
}

TEST(TestAntParentRecordForValidation, RootHasNoRecordToValidateAgainst)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ROOT_REF, /*allowUnsettled=*/false, view);
    ASSERT_EQ(view.status, AntParentReady);

    AntSolutionRecord scratch;
    EXPECT_EQ(antParentRecordForValidation(view, scratch), nullptr);
}

// ---------------------------------------------------------------------------------------------
// computeAntWouldCommit()

TEST(TestComputeAntWouldCommit, TrueWhenTheParentCommitsAndTheChildValidates)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(8);
    ASSERT_NE(commitRootChild(fx->colony, owner, 3800, TEST_TICK, 0, 502), ANT_INVALID_INDEX);
    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/false, view);
    ASSERT_EQ(view.status, AntParentReady);

    const ChildCandidate child{ owner, score_engine::Rating{ 3700, 0 }, TEST_TICK, TEST_TICK };
    EXPECT_TRUE(computeAntWouldCommit(view, child, /*childCount=*/0, TEST_THRESHOLD));
}

// A parent that is itself still only a node-local prediction to fail commit() (wouldCommit = false)
// must veto every child's prediction too, regardless of how good the child's own score is - this is
// the whole point of propagating wouldCommit down a chain rather than checking only the immediate
// parent (an honest computor must not fund a doomed grandchild's deposit either).
TEST(TestComputeAntWouldCommit, FalseWhenTheParentItselfWouldNotCommit)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(9);
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, owner, 3800));
    ASSERT_TRUE(fx->window.workerStep(1));
    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/true, view);
    ASSERT_EQ(view.status, AntParentReady);
    EXPECT_EQ(view.treeRecord, nullptr);
    ASSERT_TRUE(view.wouldCommit);   // the stub scorer sets this true; override it for the test below
    view.wouldCommit = false;

    const ChildCandidate child{ owner, score_engine::Rating{ 3700, 0 }, TEST_TICK, TEST_TICK };
    EXPECT_FALSE(computeAntWouldCommit(view, child, /*childCount=*/0, TEST_THRESHOLD));
}

TEST(TestComputeAntWouldCommit, FalseWhenTheChildDoesNotStrictlyBeatTheParent)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    const m256i owner = makeKey(10);
    ASSERT_NE(commitRootChild(fx->colony, owner, 3800, TEST_TICK, 0, 503), ANT_INVALID_INDEX);
    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 },
        /*allowUnsettled=*/false, view);
    ASSERT_EQ(view.status, AntParentReady);

    // Same error as the parent: RejectLeParent, not a strict improvement.
    const ChildCandidate child{ owner, score_engine::Rating{ 3800, 0 }, TEST_TICK, TEST_TICK };
    EXPECT_FALSE(computeAntWouldCommit(view, child, /*childCount=*/0, TEST_THRESHOLD));
}

TEST(TestComputeAntWouldCommit, FalseWhenTheChildIsAboveThreshold)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->window, ANT_KIND, fx->scratchAnn, ROOT_REF, /*allowUnsettled=*/false, view);
    ASSERT_EQ(view.status, AntParentReady);

    const ChildCandidate child{ makeKey(11), score_engine::Rating{ TEST_THRESHOLD + 1, 0 }, TEST_TICK, TEST_TICK };
    EXPECT_FALSE(computeAntWouldCommit(view, child, /*childCount=*/0, TEST_THRESHOLD));
}
