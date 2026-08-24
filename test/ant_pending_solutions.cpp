#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/mining/ant_colony/ant_pending_solutions.h"

static m256i key(unsigned long long n)
{
    m256i k = m256i::zero();
    k.m256i_u64[0] = n + 1;   // never zero: a zero pubkey marks an unused slot
    return k;
}

static SolutionRef ref(unsigned int tick, unsigned int idx)
{
    SolutionRef r;
    r.tick = tick;
    r.solutionIndexInTick = idx;
    return r;
}

// 5.75 MB, so one buffer for the file, reset between tests.
static AntPendingSolutions* freshPool()
{
    static AntPendingSolutions pool;
    static bool allocated = false;
    static bool failed = false;
    if (!allocated && !failed)
    {
        failed = !pool.init();
        allocated = !failed;
    }
    if (failed)
    {
        return nullptr;
    }
    pool.reset();
    return &pool;
}

// The key is (computor, parentRef, nonce). Same triple twice is one solution, whatever else differs.
TEST(TestAntColonyPending, DedupsOnTheConsensusKey)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    // A different anchor is the SAME solution - anchorTick is deliberately not in the key, so a
    // re-anchored resend cannot be published twice.
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 9999, 0, key(900)));

    // Any other field differing makes it a different solution.
    EXPECT_TRUE(pool->add(key(2), ref(100, 0), 5000, 0, key(900)));
    EXPECT_TRUE(pool->add(key(1), ref(100, 1), 5000, 0, key(900)));
    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(901)));
}

// A fresh entry is selectable, and only by the computor it belongs to.
TEST(TestAntColonyPending, SelectsOnlyForItsOwnComputor)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    AntPendingSolution out;
    EXPECT_EQ(pool->selectForPublish(key(2), 5000, out), AntPendingSolutions::NO_ENTRY);

    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    EXPECT_TRUE(out.nonce == key(900));
    EXPECT_EQ(out.anchorTick, 5000u);
}

// Scheduling records a deadline. Before it passes the entry must not come back, or the node would
// republish a transaction that is still in flight.
TEST(TestAntColonyPending, ScheduledEntryIsNotReselectedBeforeItsDeadline)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(idx, 5003);

    EXPECT_EQ(pool->selectForPublish(key(1), 5001, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_EQ(pool->selectForPublish(key(1), 5002, out), AntPendingSolutions::NO_ENTRY);

    // Deadline reached with no acknowledgement: republish.
    EXPECT_EQ(pool->selectForPublish(key(1), 5003, out), idx);
}

// The whole reason the state is a tick and not a flag.
TEST(TestAntColonyPending, RetriesOutrankFreshEntries)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));
    AntPendingSolution out;
    const unsigned int stale = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(stale, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(stale, 5003);

    // Newer solutions keep arriving while the first one's transaction is lost.
    ASSERT_TRUE(pool->add(key(1), ref(100, 1), 5001, 0, key(901)));
    ASSERT_TRUE(pool->add(key(1), ref(100, 2), 5002, 0, key(902)));

    // Past the deadline the retry must win, or a steady stream of new work starves it forever.
    EXPECT_EQ(pool->selectForPublish(key(1), 5003, out), stale);
}

// RECORDED comes from observing the chain, and must both stop republication and suppress a resend.
TEST(TestAntColonyPending, RecordedStopsRepublishingAndSuppressesResend)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(idx, 5003);
    pool->markRecorded(key(1), ref(100, 0), key(900));

    EXPECT_EQ(pool->selectForPublish(key(1), 9000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));
}

// A transaction the node never queued still has to be remembered, or a miner resending a solution
// that is already on-chain makes the pool publish it a second time and pay a second deposit.
TEST(TestAntColonyPending, RecordingAnUnqueuedSolutionSuppressesALaterSubmission)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    pool->markRecorded(key(1), ref(100, 0), key(900));
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));
}

// Past the publish window the commit path rejects it as stale, so publishing spends the deposit for
// nothing. Selection has to drop it rather than hand it over.
TEST(TestAntColonyPending, ExpiredEntriesAreRetiredNotPublished)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    AntPendingSolution out;
    EXPECT_EQ(pool->selectForPublish(key(1), 5000 + ANT_PUBLISH_WINDOW_TICKS, out), 0u);
    EXPECT_EQ(pool->selectForPublish(key(1), 5000 + ANT_PUBLISH_WINDOW_TICKS + 1, out), AntPendingSolutions::NO_ENTRY);

    AntPendingSolutions::Stats stats;
    unsigned int count = 0;
    pool->getStats(stats, count);
    EXPECT_EQ(stats.obsoleteExpired, 1u);
}

// An entry retired for expiry was never published, so the seen filter was never marked and the key
// is still usable. A resubmission with a fresh anchor must replace it rather than be refused as a
// duplicate - otherwise the solution is stranded and the miner is never told why.
TEST(TestAntColonyPending, ExpiredEntryCanBeResubmittedWithAFreshAnchor)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    // Selection retires it instead of publishing: publishing a stale one would mark the seen filter
    // and kill this key permanently.
    AntPendingSolution out;
    ASSERT_EQ(pool->selectForPublish(key(1), 5000 + ANT_PUBLISH_WINDOW_TICKS + 1, out), AntPendingSolutions::NO_ENTRY);

    // Same triple, newer anchor. This is the replacement, not a duplicate.
    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 40000, 0, key(900)));

    const unsigned int idx = pool->selectForPublish(key(1), 40000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    EXPECT_EQ(out.anchorTick, 40000u);
}

// The score is computed once, at receipt, and the publisher reads it back from the entry.
TEST(TestAntColonyPending, CarriesTheScore)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 3771, key(900)));

    AntPendingSolution out;
    ASSERT_NE(pool->selectForPublish(key(1), 5000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_EQ(out.score, 3771u);
}

// A live entry is a real duplicate, whether it has been scheduled or not.
TEST(TestAntColonyPending, LiveEntryStillRejectsAResubmission)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, 0, key(900)));

    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 6000, 0, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(idx, 5003);
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 6000, 0, key(900)));
}

// Finished slots are reused in place, so a pool that has published for a whole epoch does not fill.
TEST(TestAntColonyPending, FinishedSlotsAreReclaimed)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    for (unsigned int i = 0; i < 4; i++)
    {
        ASSERT_TRUE(pool->add(key(1), ref(100, i), 5000, 0, key(900 + i)));
        pool->markRecorded(key(1), ref(100, i), key(900 + i));
    }

    AntPendingSolutions::Stats stats;
    unsigned int count = 0;
    pool->getStats(stats, count);
    EXPECT_EQ(stats.recorded, 4u);

    // Nothing left to publish, and new work still fits.
    AntPendingSolution out;
    EXPECT_EQ(pool->selectForPublish(key(1), 5000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_TRUE(pool->add(key(1), ref(200, 0), 5000, 0, key(1000)));
}
