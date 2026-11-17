#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/mining/ant_colony/ant_pending_solutions.h"
#include "../src/mining/ant_colony/ant_tick_solutions.h"

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

    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

    // A different anchor is the SAME solution - anchorTick is deliberately not in the key, so a
    // re-anchored resend cannot be published twice.
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 9999, score_engine::Rating{ 0, 0 }, key(900)));

    // Any other field differing makes it a different solution.
    EXPECT_TRUE(pool->add(key(2), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));
    EXPECT_TRUE(pool->add(key(1), ref(100, 1), 5000, score_engine::Rating{ 0, 0 }, key(900)));
    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(901)));
}

// A fresh entry is selectable, and only by the computor it belongs to.
TEST(TestAntColonyPending, SelectsOnlyForItsOwnComputor)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

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
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

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

    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));
    AntPendingSolution out;
    const unsigned int stale = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(stale, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(stale, 5003);

    // Newer solutions keep arriving while the first one's transaction is lost.
    ASSERT_TRUE(pool->add(key(1), ref(100, 1), 5001, score_engine::Rating{ 0, 0 }, key(901)));
    ASSERT_TRUE(pool->add(key(1), ref(100, 2), 5002, score_engine::Rating{ 0, 0 }, key(902)));

    // Past the deadline the retry must win, or a steady stream of new work starves it forever.
    EXPECT_EQ(pool->selectForPublish(key(1), 5003, out), stale);
}

// RECORDED comes from observing the chain, and must both stop republication and suppress a resend.
TEST(TestAntColonyPending, RecordedStopsRepublishingAndSuppressesResend)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(idx, 5003);
    pool->markRecorded(key(1), ref(100, 0), key(900));

    EXPECT_EQ(pool->selectForPublish(key(1), 9000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));
}

// A transaction the node never queued still has to be remembered, or a miner resending a solution
// that is already on-chain makes the pool publish it a second time and pay a second deposit.
TEST(TestAntColonyPending, RecordingAnUnqueuedSolutionSuppressesALaterSubmission)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    pool->markRecorded(key(1), ref(100, 0), key(900));
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));
}

// Past the publish window the commit path rejects it as stale, so publishing spends the deposit for
// nothing. Selection has to drop it rather than hand it over.
TEST(TestAntColonyPending, ExpiredEntriesAreRetiredNotPublished)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

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
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

    // Selection retires it instead of publishing: publishing a stale one would mark the seen filter
    // and kill this key permanently.
    AntPendingSolution out;
    ASSERT_EQ(pool->selectForPublish(key(1), 5000 + ANT_PUBLISH_WINDOW_TICKS + 1, out), AntPendingSolutions::NO_ENTRY);

    // Same triple, newer anchor. This is the replacement, not a duplicate.
    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 40000, score_engine::Rating{ 0, 0 }, key(900)));

    const unsigned int idx = pool->selectForPublish(key(1), 40000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    EXPECT_EQ(out.anchorTick, 40000u);
}

// The score is computed once, at receipt, and the publisher reads it back from the entry.
TEST(TestAntColonyPending, CarriesTheScore)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 3771, 0 }, key(900)));

    AntPendingSolution out;
    ASSERT_NE(pool->selectForPublish(key(1), 5000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_EQ(out.rating.error, 3771u);
}

// A live entry is a real duplicate, whether it has been scheduled or not.
TEST(TestAntColonyPending, LiveEntryStillRejectsAResubmission)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 6000, score_engine::Rating{ 0, 0 }, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);
    pool->markScheduled(idx, 5003);
    EXPECT_FALSE(pool->add(key(1), ref(100, 0), 6000, score_engine::Rating{ 0, 0 }, key(900)));
}

// Finished slots are reused in place, so a pool that has published for a whole epoch does not fill.
TEST(TestAntColonyPending, FinishedSlotsAreReclaimed)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    for (unsigned int i = 0; i < 4; i++)
    {
        ASSERT_TRUE(pool->add(key(1), ref(100, i), 5000, score_engine::Rating{ 0, 0 }, key(900 + i)));
        pool->markRecorded(key(1), ref(100, i), key(900 + i));
    }

    AntPendingSolutions::Stats stats;
    unsigned int count = 0;
    pool->getStats(stats, count);
    EXPECT_EQ(stats.recorded, 4u);

    // Nothing left to publish, and new work still fits.
    AntPendingSolution out;
    EXPECT_EQ(pool->selectForPublish(key(1), 5000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_TRUE(pool->add(key(1), ref(200, 0), 5000, score_engine::Rating{ 0, 0 }, key(1000)));
}

// A publish queue that fills to capacity refuses the overflow and counts it in droppedFull,
// rather than overwriting a live entry. This is the fail-safe when one computor saturates.
TEST(TestAntColonyPending, FullPoolRefusesAndCounts)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);

    for (unsigned int i = 0; i < AntPendingSolutions::CAPACITY; i++)
    {
        ASSERT_TRUE(pool->add(key(i), ref(100, 0), 5000,
                              score_engine::Rating{ 0, 0 }, key(1000000 + i)));
    }
    // One past capacity: every slot holds a live entry, so there is nothing to reclaim.
    EXPECT_FALSE(pool->add(key(777777), ref(100, 0), 5000,
                           score_engine::Rating{ 0, 0 }, key(2000000)));

    AntPendingSolutions::Stats stats;
    unsigned int count = 0;
    pool->getStats(stats, count);
    EXPECT_EQ(stats.droppedFull, 1u);
    EXPECT_EQ(count, AntPendingSolutions::CAPACITY);
}

// An entry the publisher explicitly retires (gate rejected it) is counted, stops being selected,
// and is NOT treated as a duplicate: a corrected resubmission on the same key revives it in place.
TEST(TestAntColonyPending, ExplicitRetireIsCountedAndAllowsRevival)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);

    pool->markObsoleteGateRejected(idx);

    AntPendingSolutions::Stats stats;
    unsigned int count = 0;
    pool->getStats(stats, count);
    EXPECT_EQ(stats.obsoleteGateRejected, 1u);

    // OBSOLETE: not selectable, and not a duplicate - the resend revives it.
    EXPECT_EQ(pool->selectForPublish(key(1), 5000, out), AntPendingSolutions::NO_ENTRY);
    EXPECT_TRUE(pool->add(key(1), ref(100, 0), 6000, score_engine::Rating{ 0, 0 }, key(900)));

    const unsigned int revived = pool->selectForPublish(key(1), 6000, out);
    ASSERT_NE(revived, AntPendingSolutions::NO_ENTRY);
    EXPECT_EQ(out.anchorTick, 6000u);
}

// markObsoleteParentGone retires the entry and increments its own counter, kept distinct from the
// gate-rejected and expiry counters.
TEST(TestAntColonyPending, ParentGoneRetireIsCountedSeparately)
{
    AntPendingSolutions* pool = freshPool();
    ASSERT_NE(pool, nullptr);
    ASSERT_TRUE(pool->add(key(1), ref(100, 0), 5000, score_engine::Rating{ 0, 0 }, key(900)));

    AntPendingSolution out;
    const unsigned int idx = pool->selectForPublish(key(1), 5000, out);
    ASSERT_NE(idx, AntPendingSolutions::NO_ENTRY);

    pool->markObsoleteParentGone(idx);

    AntPendingSolutions::Stats stats;
    unsigned int count = 0;
    pool->getStats(stats, count);
    EXPECT_EQ(stats.obsoleteParentGone, 1u);
    EXPECT_EQ(stats.obsoleteGateRejected, 0u);
    EXPECT_EQ(stats.obsoleteExpired, 0u);

    // Retired: no longer selectable.
    EXPECT_EQ(pool->selectForPublish(key(1), 5000, out), AntPendingSolutions::NO_ENTRY);
}

// ---------------------------------------------------------------------------------------------
// One solution per sender per tick

using TickSolutions = AntTickSolutions<16>;

// One filter for the file, reset between tests.
static TickSolutions* freshTickSolutions()
{
    static TickSolutions filter;
    static const bool allocated = filter.init();
    if (!allocated)
    {
        return nullptr;
    }
    filter.reset();
    return &filter;
}

// Of a sender's solutions in one tick, all but the one at the largest transaction index are superseded.
TEST(TestAntTickSolutions, OnlyTheLargestIndexOfASenderCounts)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    filter->add(key(1), 2);
    filter->add(key(1), 5);
    filter->add(key(1), 9);

    EXPECT_TRUE(filter->isSuperseded(2));
    EXPECT_TRUE(filter->isSuperseded(5));
    EXPECT_FALSE(filter->isSuperseded(9));
}

// The rule is applied per sender.
TEST(TestAntTickSolutions, SendersDoNotAffectEachOther)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    filter->add(key(1), 0);
    filter->add(key(2), 1);
    filter->add(key(3), 2);
    filter->add(key(2), 7);

    EXPECT_FALSE(filter->isSuperseded(0));
    EXPECT_TRUE(filter->isSuperseded(1));
    EXPECT_FALSE(filter->isSuperseded(2));
    EXPECT_FALSE(filter->isSuperseded(7));
}

// The result does not depend on the order the solutions are added in.
TEST(TestAntTickSolutions, OrderOfTheCallsDoesNotMatter)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    filter->add(key(1), 9);
    filter->add(key(1), 2);
    filter->add(key(1), 5);

    EXPECT_TRUE(filter->isSuperseded(2));
    EXPECT_TRUE(filter->isSuperseded(5));
    EXPECT_FALSE(filter->isSuperseded(9));
}

// A sender with one solution keeps it, and an index that was never added is not superseded.
TEST(TestAntTickSolutions, ASingleSolutionIsNeverSuperseded)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    filter->add(key(1), 4);
    filter->add(key(1), 4);   // the same transaction named twice changes nothing

    EXPECT_FALSE(filter->isSuperseded(4));
    // Transactions that are not solutions were never added.
    EXPECT_FALSE(filter->isSuperseded(0));
    EXPECT_FALSE(filter->isSuperseded(15));
}

// reset() clears the tick.
TEST(TestAntTickSolutions, ResetForgetsTheTick)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    filter->add(key(1), 1);
    filter->add(key(1), 3);
    ASSERT_TRUE(filter->isSuperseded(1));

    filter->reset();
    EXPECT_FALSE(filter->isSuperseded(1));
    filter->add(key(1), 1);
    EXPECT_FALSE(filter->isSuperseded(1));
}

// Every transaction of a full tick from a different sender, then all of them from one sender.
TEST(TestAntTickSolutions, FullTick)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    for (unsigned int i = 0; i < 16; i++)
    {
        filter->add(key(100 + i), i);
    }
    for (unsigned int i = 0; i < 16; i++)
    {
        EXPECT_FALSE(filter->isSuperseded(i)) << i;
    }

    filter->reset();
    for (unsigned int i = 0; i < 16; i++)
    {
        filter->add(key(7), i);
    }
    for (unsigned int i = 0; i < 15; i++)
    {
        EXPECT_TRUE(filter->isSuperseded(i)) << i;
    }
    EXPECT_FALSE(filter->isSuperseded(15));
}

// Senders whose keys fall into the same slot are still told apart.
TEST(TestAntTickSolutions, CollidingSendersAreToldApart)
{
    TickSolutions* filter = freshTickSolutions();
    ASSERT_NE(filter, nullptr);
    // 16 transactions, 32 slots: keys that differ by a multiple of 32 all start at the same slot.
    for (unsigned int i = 0; i < 12; i++)
    {
        filter->add(key(1000 + 32 * i), i);
    }
    filter->add(key(1000 + 32 * 3), 12);
    filter->add(key(1000 + 32 * 8), 13);

    for (unsigned int i = 0; i < 12; i++)
    {
        EXPECT_EQ(filter->isSuperseded(i), i == 3 || i == 8) << i;
    }
    EXPECT_FALSE(filter->isSuperseded(12));
    EXPECT_FALSE(filter->isSuperseded(13));

    // An index outside the tick is ignored.
    filter->add(key(1), 16);
    EXPECT_FALSE(filter->isSuperseded(16));
}
