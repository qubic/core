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
// the stub scoreFn below produces: 0 means "scores invalid", anything else becomes that Rating's
// error at shift 0.
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
        result.rating = score_engine::Rating::worst();   // unscorable
    }
    else
    {
        result.rating = score_engine::Rating{ record.claimedScore, 0 };
        result.depth = 1;
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
// isAntParentSettledBefore()

TEST(TestAntParentSettledBefore, RootIsAlwaysSettled)
{
    EXPECT_TRUE(isAntParentSettledBefore(ROOT_REF, TEST_TICK));
    EXPECT_TRUE(isAntParentSettledBefore(ROOT_REF, 0));
}

TEST(TestAntParentSettledBefore, ParentInsideTheWindowIsNotSettled)
{
    EXPECT_FALSE(isAntParentSettledBefore(SolutionRef{ TEST_TICK - SOLUTION_SETTLEMENT_WINDOW_TICKS, 0 }, TEST_TICK));
    EXPECT_FALSE(isAntParentSettledBefore(SolutionRef{ TEST_TICK - 1, 0 }, TEST_TICK));
    EXPECT_TRUE(isAntParentSettledBefore(SolutionRef{ TEST_TICK - SOLUTION_SETTLEMENT_WINDOW_TICKS - 1, 0 }, TEST_TICK));
    EXPECT_TRUE(isAntParentSettledBefore(SolutionRef{ TEST_INITIAL_TICK, 3 }, TEST_TICK));
}

TEST(TestAntParentSettledBefore, SameOrLaterTickIsNotSettled)
{
    EXPECT_FALSE(isAntParentSettledBefore(SolutionRef{ TEST_TICK, 0 }, TEST_TICK));
    EXPECT_FALSE(isAntParentSettledBefore(SolutionRef{ TEST_TICK + 1, 0 }, TEST_TICK));
    EXPECT_FALSE(isAntParentSettledBefore(SolutionRef{ 0xFFFFFFFFu, 0 }, TEST_TICK));
}

// ---------------------------------------------------------------------------------------------
// resolveAntParent() status matrix
//
// Two sources answer Ready - the derived ROOT and the settled tree. Everything else is Absent,
// including a reference into a slot still inside the settlement window.

TEST(TestResolveAntParent, RootIsReadyWithWorstRatingAndNoAnn)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->scratchAnn, ROOT_REF, view);

    EXPECT_EQ(view.status, AntParentReady);
    EXPECT_EQ(view.ann, nullptr);
    EXPECT_EQ(view.treeRecord, nullptr);
    EXPECT_EQ(view.depth, 0u);
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
    resolveAntParent(fx->colony, fx->scratchAnn, ref, view);

    EXPECT_EQ(view.status, AntParentReady);
    ASSERT_NE(view.ann, nullptr);
    ASSERT_NE(view.treeRecord, nullptr);
    EXPECT_TRUE(view.treeRecord->pubkey == owner);
    EXPECT_EQ(view.rating.error, 3800u);
    EXPECT_EQ(view.shift, 0u);
    EXPECT_EQ(view.depth, 1u);
    EXPECT_TRUE(view.pubkey == owner);
}

// The rule that makes every node answer alike: a slot in the window is not a parent, scored or not.
// Whether a worker has reached a given slot depends on this node's own timing, so any answer drawn
// from one would differ between nodes.
TEST(TestResolveAntParent, UnsettledSlotIsAbsentWhetherScoredOrNot)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 0, makeKey(2), 4200));
    ASSERT_TRUE(appendAntSlot(fx->window, TEST_TICK, 1, makeKey(3), 4300));
    ASSERT_TRUE(fx->window.workerStep(1));   // scores (TEST_TICK, 0); (TEST_TICK, 1) stays unscored
    ASSERT_NE(fx->window.lookup(TEST_TICK, 0), nullptr);
    EXPECT_TRUE(fx->window.lookup(TEST_TICK, 0)->scored);
    EXPECT_FALSE(fx->window.lookup(TEST_TICK, 1)->scored);

    AntParentView scored;
    resolveAntParent(fx->colony, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 }, scored);
    EXPECT_EQ(scored.status, AntParentAbsent);

    AntParentView unscored;
    resolveAntParent(fx->colony, fx->scratchAnn, SolutionRef{ TEST_TICK, 1 }, unscored);
    EXPECT_EQ(unscored.status, AntParentAbsent);
}

TEST(TestResolveAntParent, MissingParentIsAbsent)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 }, view);

    EXPECT_EQ(view.status, AntParentAbsent);
}

// ---------------------------------------------------------------------------------------------
// antParentRecordForValidation()

TEST(TestAntParentRecordForValidation, UsesTheTreeRecord)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    ASSERT_NE(commitRootChild(fx->colony, makeKey(6), 3800, TEST_TICK, 0, 501), ANT_INVALID_INDEX);
    AntParentView view;
    resolveAntParent(fx->colony, fx->scratchAnn, SolutionRef{ TEST_TICK, 0 }, view);
    ASSERT_EQ(view.status, AntParentReady);

    EXPECT_EQ(antParentRecordForValidation(view), view.treeRecord);
}

// ROOT is derived, so there is no record to validate against and commit() takes nullptr for it.
TEST(TestAntParentRecordForValidation, RootHasNoRecordToValidateAgainst)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->scratchAnn, ROOT_REF, view);
    ASSERT_EQ(view.status, AntParentReady);

    EXPECT_EQ(antParentRecordForValidation(view), nullptr);
}

TEST(TestAntParentRecordForValidation, AbsentHasNoRecord)
{
    GlueFixture* fx = freshFixture();
    ASSERT_NE(fx, nullptr) << "fixture init failed; needs ~8 GB";

    AntParentView view;
    resolveAntParent(fx->colony, fx->scratchAnn, SolutionRef{ TEST_TICK + 99, 7 }, view);
    ASSERT_EQ(view.status, AntParentAbsent);

    EXPECT_EQ(antParentRecordForValidation(view), nullptr);
}
