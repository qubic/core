#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/public_settings.h"
#include "../src/mining/prediction_phase.h"
#include "../src/platform/time.h"

#include <memory>
#include <vector>

static constexpr unsigned int TEST_EPOCH = 241;
// First hour of the epoch, and the hour it ends at. Hour 0 of the series is the start of epoch 0.
static constexpr unsigned int EPOCH_HOUR = PREDICTION_HOURS_PER_EPOCH * TEST_EPOCH;
static constexpr unsigned int END_HOUR = EPOCH_HOUR + PREDICTION_HOURS_PER_EPOCH;
static constexpr unsigned int BASE_HOUR = EPOCH_HOUR;

static m256i key(unsigned long long n)
{
    m256i k = m256i::zero();
    k.m256i_u64[0] = n + 1;
    return k;
}

static long long msAt(unsigned int hour, long long minutes = 0)
{
    return (long long)hour * PREDICTION_MS_PER_HOUR + minutes * 60000;
}

static score_engine::Rating rating(unsigned int error, unsigned int shift)
{
    score_engine::Rating r;
    r.error = error;
    r.shift = shift;
    return r;
}

static std::vector<unsigned char> answersOf(unsigned char value)
{
    return std::vector<unsigned char>(PREDICTION_MAX_HOURS, value);
}

static std::unique_ptr<PredictionPhase> makePhase()
{
    auto phase = std::make_unique<PredictionPhase>();
    phase->reset();
    return phase;
}

// Qualifies identities first, first + 1, ... as computors, each answering `answer` for every hour.
static void qualifyComputors(PredictionPhase& phase, unsigned int first, unsigned int number, long long nowMs, unsigned char answer)
{
    const std::vector<unsigned char> answers = answersOf(answer);
    for (unsigned int i = 0; i < number; i++)
    {
        const score_engine::Rating r = rating(BPP9000_PREDICTION_THRESHOLD, 0);
        ASSERT_TRUE(phase.needsAnswers(key(first + i), r));
        phase.recordSolution(key(first + i), r, 1000 + i, 0, nowMs, BASE_HOUR, true, answers.data());
    }
}

TEST(TestPredictionPhase, DeadlineIsTheLastInstantForTheHour)
{
    const unsigned int hour = EPOCH_HOUR + 10;
    EXPECT_EQ(PredictionPhase::deadlineMs(hour), msAt(hour) - PREDICTION_DEADLINE_MS);
    EXPECT_EQ(PredictionPhase::firstOpenHourAt(PredictionPhase::deadlineMs(hour)), hour);
    EXPECT_EQ(PredictionPhase::firstOpenHourAt(PredictionPhase::deadlineMs(hour) + 1), hour + 1);
    EXPECT_EQ(PredictionPhase::firstOpenHourAt(PredictionPhase::deadlineMs(hour) - 1), hour);
}

TEST(TestPredictionPhase, QualifyingNeedsTheThreshold)
{
    auto phase = makePhase();
    EXPECT_FALSE(phase->needsAnswers(key(1), rating(BPP9000_PREDICTION_THRESHOLD + 1, 0)));
    EXPECT_TRUE(phase->needsAnswers(key(1), rating(BPP9000_PREDICTION_THRESHOLD, 0)));
    // A solution past the first frame has mastered it.
    EXPECT_TRUE(phase->needsAnswers(key(1), rating(BPP9000_PREDICTION_THRESHOLD + 500, 1)));
    EXPECT_EQ(phase->count, 0u);
}

TEST(TestPredictionPhase, SwitchNeedsEnoughComputors)
{
    auto phase = makePhase();
    const long long nowMs = msAt(EPOCH_HOUR + 5);
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS - 1, nowMs, 1);
    EXPECT_FALSE(phase->trySwitch(2000, nowMs, END_HOUR));

    // A candidate qualifies but is not counted for the switch.
    const std::vector<unsigned char> answers = answersOf(1);
    phase->recordSolution(key(5000), rating(100, 3), 1500, 0, nowMs, BASE_HOUR, false, answers.data());
    EXPECT_FALSE(phase->trySwitch(2000, nowMs, END_HOUR));
    EXPECT_FALSE(phase->isSwitched());

    qualifyComputors(*phase, 6000, 1, nowMs, 1);
    EXPECT_EQ(phase->computorsQualified, BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));
    EXPECT_TRUE(phase->isSwitched());
    EXPECT_EQ(phase->switchFirstHour, PredictionPhase::firstOpenHourAt(nowMs));
    // Only once.
    EXPECT_FALSE(phase->trySwitch(2001, nowMs, END_HOUR));
    EXPECT_EQ(phase->switchTick, 2000u);
}

TEST(TestPredictionPhase, NoSwitchInTheLastHoursOfTheEpoch)
{
    auto closed = makePhase();
    const long long closedMs = msAt(EPOCH_HOUR + PREDICTION_HOURS_PER_EPOCH - BPP9000_PREDICTION_SWITCH_CLOSE_HOURS);
    qualifyComputors(*closed, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, closedMs, 1);
    EXPECT_FALSE(closed->trySwitch(2000, closedMs, END_HOUR));

    auto open = makePhase();
    const long long openMs = closedMs - 1;
    qualifyComputors(*open, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, openMs, 1);
    EXPECT_TRUE(open->trySwitch(2000, openMs, END_HOUR));
}

TEST(TestPredictionPhase, EpochEndHourIsTheHourOfTheEpochTransition)
{
    EXPECT_EQ(PredictionPhase::epochEndHour(TEST_EPOCH), END_HOUR);
    // Hour 0 is 2022-04-13 12:00 UTC; an epoch ends on day 738570 + epoch * 7 of dayIndex(), at 12:00.
    const int dayOfHourZero = dayIndex(22, 4, 13);
    for (unsigned int epoch = 239; epoch < 244; epoch++)
    {
        const int endDay = 738570 + (int)epoch * 7;
        EXPECT_EQ(PredictionPhase::epochEndHour(epoch), (unsigned int)((endDay - dayOfHourZero) * 24));
    }
}

TEST(TestPredictionPhase, NoSwitchAtOrAfterTheEpochEnd)
{
    // The last tick of an epoch has a tick time at or after the end hour; a late epoch transition has more of them.
    const long long times[] = { msAt(END_HOUR), msAt(END_HOUR) + 1, msAt(END_HOUR, 30), msAt(END_HOUR + 1), msAt(END_HOUR + 200) };
    for (const long long lateMs : times)
    {
        auto phase = makePhase();
        qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, lateMs, 1);
        EXPECT_FALSE(phase->trySwitch(2000, lateMs, END_HOUR));
        EXPECT_FALSE(phase->isSwitched());
    }

    // The same instants are open for the epoch after.
    auto next = makePhase();
    qualifyComputors(*next, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, msAt(END_HOUR), 1);
    EXPECT_TRUE(next->trySwitch(2000, msAt(END_HOUR), PredictionPhase::epochEndHour(TEST_EPOCH + 1)));
}

TEST(TestPredictionPhase, BestNetworkAnswersUntilTheSwitchThenTheLastAccepted)
{
    auto phase = makePhase();
    const long long nowMs = msAt(EPOCH_HOUR + 5);
    const m256i id = key(0);
    const std::vector<unsigned char> up = answersOf(1);
    const std::vector<unsigned char> down = answersOf(0);

    ASSERT_TRUE(phase->needsAnswers(id, rating(3000, 0)));
    phase->recordSolution(id, rating(3000, 0), 1000, 0, nowMs, BASE_HOUR, true, up.data());

    // Before the switch a weaker solution changes nothing.
    EXPECT_FALSE(phase->needsAnswers(id, rating(3200, 0)));
    // A better one replaces the answers.
    ASSERT_TRUE(phase->needsAnswers(id, rating(2900, 0)));
    const PredictionEntry& entry = phase->recordSolution(id, rating(2900, 0), 1001, 2, nowMs, BASE_HOUR, true, down.data());
    EXPECT_EQ(entry.best.error, 2900u);
    EXPECT_EQ(entry.bestTick, 1001u);
    EXPECT_EQ(entry.answers[PREDICTION_MAX_HOURS - 1], 0);

    qualifyComputors(*phase, 1, BPP9000_PREDICTION_SWITCH_COMPUTORS - 1, nowMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));

    // After the switch a weaker solution answers too; the best rating stays.
    ASSERT_TRUE(phase->needsAnswers(id, rating(4000, 0)));
    const PredictionEntry& after = phase->recordSolution(id, rating(4000, 0), 2001, 5, nowMs, BASE_HOUR, true, up.data());
    EXPECT_EQ(after.best.error, 2900u);
    EXPECT_EQ(after.bestTick, 1001u);
    EXPECT_EQ(after.solutionTick, 2001u);
    EXPECT_EQ(after.solutionIndexInTick, 5u);
    EXPECT_EQ(after.answers[PREDICTION_MAX_HOURS - 1], 1);
    EXPECT_EQ(phase->count, BPP9000_PREDICTION_SWITCH_COMPUTORS);
}

TEST(TestPredictionPhase, SolutionAfterTheDeadlineAnswersTheNextHour)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long earlyMs = PredictionPhase::deadlineMs(hour);
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, earlyMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, earlyMs, END_HOUR));
    EXPECT_EQ(phase->switchFirstHour, hour);

    // One instant late for `hour`: the answer for it stays, the later hours change.
    const std::vector<unsigned char> down = answersOf(0);
    const PredictionEntry& entry = phase->recordSolution(key(0), rating(100, 0), 2001, 0, earlyMs + 1, BASE_HOUR, true, down.data());
    EXPECT_EQ(entry.answers[hour - BASE_HOUR - 1], 1);
    EXPECT_EQ(entry.answers[hour - BASE_HOUR], 0);

    EXPECT_EQ(phase->countHour(hour, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(entry.hours, 1);
    EXPECT_EQ(entry.correct, 1);
    EXPECT_EQ(phase->countHour(hour + 1, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(entry.hours, 2);
    EXPECT_EQ(entry.correct, 1);
    EXPECT_EQ(phase->entries[1].correct, 2);
}

TEST(TestPredictionPhase, HoursBeforeTheSwitchOrBeforeQualifyingAreNotCounted)
{
    auto phase = makePhase();
    const unsigned int switchHour = EPOCH_HOUR + 6;
    const long long switchMs = PredictionPhase::deadlineMs(switchHour);

    // Nothing is counted in training.
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS - 1, switchMs, 1);
    EXPECT_EQ(phase->countHour(switchHour, BASE_HOUR, 1), 0u);

    qualifyComputors(*phase, 1000, 1, switchMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, switchMs, END_HOUR));
    // The hour before the first counted one.
    EXPECT_EQ(phase->countHour(switchHour - 1, BASE_HOUR, 1), 0u);

    // A late identity is counted from the first hour its own deadline allows.
    const unsigned int lateHour = switchHour + 3;
    const std::vector<unsigned char> up = answersOf(1);
    const m256i late = key(9000);
    ASSERT_TRUE(phase->needsAnswers(late, rating(BPP9000_PREDICTION_THRESHOLD, 0)));
    const PredictionEntry& lateEntry = phase->recordSolution(late, rating(BPP9000_PREDICTION_THRESHOLD, 0), 2100, 0,
        PredictionPhase::deadlineMs(lateHour), BASE_HOUR, false, up.data());
    EXPECT_FALSE(phase->participatesIn(lateEntry, lateHour - 1));
    EXPECT_TRUE(phase->participatesIn(lateEntry, lateHour));

    EXPECT_EQ(phase->countHour(switchHour, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(lateEntry.hours, 0);
    EXPECT_EQ(phase->countHour(lateHour, BASE_HOUR, 0), BPP9000_PREDICTION_SWITCH_COMPUTORS + 1);
    EXPECT_EQ(lateEntry.hours, 1);
    EXPECT_EQ(lateEntry.correct, 0);

    // Hours outside the epoch's range are counted for nobody.
    EXPECT_EQ(phase->countHour(BASE_HOUR, BASE_HOUR, 1), 0u);
    EXPECT_EQ(phase->countHour(BASE_HOUR + PREDICTION_MAX_HOURS + 1, BASE_HOUR, 1), 0u);
}

TEST(TestPredictionPhase, HoursBetweenQualifyingAndTheSwitchAreNotCounted)
{
    auto phase = makePhase();
    const unsigned int earlyHour = EPOCH_HOUR + 3;
    const unsigned int switchHour = EPOCH_HOUR + 9;
    const std::vector<unsigned char> up = answersOf(1);

    // One identity qualifies six hours before the switch.
    const m256i early = key(7000);
    const PredictionEntry& earlyEntry = phase->recordSolution(early, rating(BPP9000_PREDICTION_THRESHOLD, 0), 1900, 0,
        PredictionPhase::deadlineMs(earlyHour), BASE_HOUR, true, up.data());
    EXPECT_EQ(earlyEntry.firstHour, earlyHour);
    EXPECT_EQ(phase->computorsQualified, 1u);

    const long long switchMs = PredictionPhase::deadlineMs(switchHour);
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS - 1, switchMs, 1);
    EXPECT_EQ(phase->computorsQualified, BPP9000_PREDICTION_SWITCH_COMPUTORS);
    ASSERT_TRUE(phase->trySwitch(2000, switchMs, END_HOUR));
    EXPECT_EQ(phase->switchFirstHour, switchHour);

    for (unsigned int hour = earlyHour; hour < switchHour; hour++)
    {
        EXPECT_FALSE(phase->participatesIn(earlyEntry, hour));
        EXPECT_EQ(phase->countHour(hour, BASE_HOUR, 1), 0u);
    }
    EXPECT_EQ(earlyEntry.hours, 0);
    EXPECT_EQ(phase->countHour(switchHour, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(earlyEntry.hours, 1);
    EXPECT_EQ(earlyEntry.correct, 1);
}

TEST(TestPredictionPhase, FirstAndLastHourOfTheAnswerTableAreCounted)
{
    auto phase = makePhase();
    // Up for the first and the last hour, down for the hours between.
    std::vector<unsigned char> answers = answersOf(0);
    answers[0] = 1;
    answers[PREDICTION_MAX_HOURS - 1] = 1;
    const long long nowMs = PredictionPhase::deadlineMs(BASE_HOUR + 1);
    const m256i first = key(0);
    for (unsigned int i = 0; i < BPP9000_PREDICTION_SWITCH_COMPUTORS; i++)
    {
        phase->recordSolution(key(i), rating(BPP9000_PREDICTION_THRESHOLD, 0), 1000 + i, 0, nowMs, BASE_HOUR, true, answers.data());
    }
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));
    ASSERT_EQ(phase->switchFirstHour, BASE_HOUR + 1);
    const PredictionEntry& entry = phase->entries[phase->find(first)];

    EXPECT_EQ(phase->countHour(BASE_HOUR + 1, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(entry.hours, 1);
    EXPECT_EQ(entry.correct, 1);
    EXPECT_EQ(phase->countHour(BASE_HOUR + 2, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(entry.hours, 2);
    EXPECT_EQ(entry.correct, 1);
    EXPECT_EQ(phase->countHour(BASE_HOUR + PREDICTION_MAX_HOURS, BASE_HOUR, 1), BPP9000_PREDICTION_SWITCH_COMPUTORS);
    EXPECT_EQ(entry.hours, 3);
    EXPECT_EQ(entry.correct, 2);
}

TEST(TestPredictionPhase, ALoadedStateWithATooLargeCountIsNotValid)
{
    auto phase = makePhase();
    EXPECT_TRUE(phase->isValid());
    phase->count = MAX_NUMBER_OF_MINERS;
    EXPECT_TRUE(phase->isValid());
    phase->count = MAX_NUMBER_OF_MINERS + 1;
    EXPECT_FALSE(phase->isValid());
    phase->count = 0xFFFFFFFFU;
    EXPECT_FALSE(phase->isValid());
}

TEST(TestPredictionPhase, ALoadedStateWithCountersNoNodeWritesIsNotValid)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long nowMs = PredictionPhase::deadlineMs(hour);
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, nowMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));
    phase->countHour(hour, BASE_HOUR, 1);
    unsigned int closed = 0;
    ASSERT_TRUE(phase->closeNextHour(nowMs + 1, closed));
    EXPECT_TRUE(phase->isValid());

    PredictionEntry& entry = phase->entries[3];
    // More correct hours than counted hours.
    entry.correct = 2;
    EXPECT_FALSE(phase->isValid());
    entry.correct = 1;
    EXPECT_TRUE(phase->isValid());

    // The most hours an epoch can count, and one more. Above 255 correct hours the ranking key would wrap.
    entry.hours = (unsigned short)PREDICTION_MAX_HOURS;
    entry.correct = (unsigned short)PREDICTION_MAX_HOURS;
    EXPECT_TRUE(phase->isValid());
    entry.hours = (unsigned short)(PREDICTION_MAX_HOURS + 1);
    EXPECT_FALSE(phase->isValid());
    entry.hours = 1;
    entry.correct = 1;

    // An entry past count is not looked at.
    phase->entries[phase->count].correct = 5;
    EXPECT_TRUE(phase->isValid());

    phase->computorsQualified = phase->count + 1;
    EXPECT_FALSE(phase->isValid());
    phase->computorsQualified = phase->count;

    phase->firstOpenHour = phase->switchFirstHour - 1;
    EXPECT_FALSE(phase->isValid());
}

TEST(TestPredictionPhase, PredictionTickTimeIsTheLaterOfTickTimeAndStamp)
{
    EXPECT_EQ(PredictionPhase::laterTimeMs(1000, 5000), 5000);
    EXPECT_EQ(PredictionPhase::laterTimeMs(5000, 1000), 5000);
    EXPECT_EQ(PredictionPhase::laterTimeMs(5000, 5000), 5000);
    EXPECT_EQ(PredictionPhase::laterTimeMs(5000, 0), 5000);
}

// A tick's time is the stamp of the tick data before it. After a stall the first tick whose content was chosen
// after the stall still has a tick time from before it.
TEST(TestPredictionPhase, ASolutionChosenAfterAStallIsTimedByItsTickData)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long tickTimeMs = PredictionPhase::deadlineMs(hour) - 60000;
    const long long tickDataMs = tickTimeMs + 30 * 60000;
    ASSERT_EQ(PredictionPhase::firstOpenHourAt(tickTimeMs), hour);

    const std::vector<unsigned char> up = answersOf(1);
    const PredictionEntry& entry = phase->recordSolution(key(0), rating(BPP9000_PREDICTION_THRESHOLD, 0), 1000, 0,
        PredictionPhase::laterTimeMs(tickTimeMs, tickDataMs), BASE_HOUR, true, up.data());
    EXPECT_EQ(entry.firstHour, hour + 1);
    EXPECT_EQ(entry.answers[hour - BASE_HOUR - 1], PREDICTION_NO_ANSWER);
    EXPECT_EQ(entry.answers[hour - BASE_HOUR], 1);

    // A stamp set back by the tick's leader does not move the prediction tick time before the tick time.
    EXPECT_EQ(PredictionPhase::firstOpenHourAt(PredictionPhase::laterTimeMs(tickDataMs, tickTimeMs)), hour + 1);
}

// The node's tick time rule: the next tick takes the stamp of this tick's data if it is later, else 1 ms more.
TEST(TestPredictionPhase, PredictionTickTimeNeverRunsBackwards)
{
    // Stamps of consecutive ticks: steady, set back, missing (0), a stall, set back again.
    const long long stamps[] = { 1000, 2000, 1500, 0, 0, 900000, 2500, 901000, 0, 902000 };
    long long tickTimeMs = 500;
    long long previousTimeMs = 0;
    for (const long long stampMs : stamps)
    {
        const long long timeMs = PredictionPhase::laterTimeMs(tickTimeMs, stampMs);
        EXPECT_GE(timeMs, previousTimeMs);
        EXPECT_GE(timeMs, tickTimeMs);
        previousTimeMs = timeMs;
        if (stampMs > tickTimeMs)
        {
            tickTimeMs = stampMs;
        }
        else
        {
            tickTimeMs = tickTimeMs + 1;
        }
    }
}

TEST(TestPredictionPhase, NoAnswerIsNeverCorrect)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long nowMs = PredictionPhase::deadlineMs(hour);
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, nowMs, PREDICTION_NO_ANSWER);
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));
    phase->countHour(hour, BASE_HOUR, 0);
    phase->countHour(hour + 1, BASE_HOUR, 1);
    EXPECT_EQ(phase->entries[0].hours, 2);
    EXPECT_EQ(phase->entries[0].correct, 0);
}

static PredictionEntry counted(unsigned int hours, unsigned int correct)
{
    PredictionEntry entry;
    setMem(&entry, sizeof(entry), 0);
    entry.hours = (unsigned short)hours;
    entry.correct = (unsigned short)correct;
    return entry;
}

TEST(TestPredictionPhase, RankedByAccuracyNeedsEnoughCountedHours)
{
    const unsigned int minAttempts = BPP9000_PREDICTION_MIN_ATTEMPTS;
    const unsigned int minCorrect = BPP9000_PREDICTION_MIN_CORRECT;

    // The full minimum passes with any number of correct hours.
    EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(minAttempts, 0)));
    EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(minAttempts, minAttempts)));
    EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(PREDICTION_MAX_HOURS, 1)));

    // Below it a record passes exactly when it has the minimum of correct hours.
    EXPECT_FALSE(PredictionPhase::isRankedByAccuracy(counted(0, 0)));
    EXPECT_FALSE(PredictionPhase::isRankedByAccuracy(counted(minCorrect - 1, minCorrect - 1)));
    EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(minCorrect, minCorrect)));
    for (unsigned int hours = minCorrect; hours < minAttempts; hours++)
    {
        EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(hours, hours)));
        EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(hours, minCorrect)));
        EXPECT_FALSE(PredictionPhase::isRankedByAccuracy(counted(hours, minCorrect - 1)));
    }

    // A wrong hour after the minimum of correct hours leaves the identity ranked, with a worse key.
    EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(minCorrect + 1, minCorrect)));
    EXPECT_NE(PredictionPhase::rankingKey(counted(minCorrect + 1, minCorrect)), PREDICTION_NO_RANKING_KEY);
    EXPECT_LT(PredictionPhase::rankingKey(counted(minCorrect, minCorrect)), PredictionPhase::rankingKey(counted(minCorrect + 1, minCorrect)));

    EXPECT_EQ(PredictionPhase::rankingKey(counted(minAttempts - 1, minCorrect - 1)), PREDICTION_NO_RANKING_KEY);
    EXPECT_NE(PredictionPhase::rankingKey(counted(minAttempts, 0)), PREDICTION_NO_RANKING_KEY);
}

// Counted and correct hours only grow, so no record that is ranked by accuracy can lose that with another hour.
TEST(TestPredictionPhase, RankedByAccuracyIsKeptByEveryFurtherHour)
{
    for (unsigned int hours = 1; hours < PREDICTION_MAX_HOURS; hours++)
    {
        for (unsigned int correct = 0; correct <= hours; correct++)
        {
            if (PredictionPhase::isRankedByAccuracy(counted(hours, correct)))
            {
                EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(hours + 1, correct)));
                EXPECT_TRUE(PredictionPhase::isRankedByAccuracy(counted(hours + 1, correct + 1)));
            }
        }
    }
}

// Every pair of records that are ranked by accuracy: the keys must order them as the exact fractions do.
TEST(TestPredictionPhase, RankingKeyOrdersByAccuracyThenCountedHours)
{
    std::vector<PredictionEntry> entries;
    for (unsigned int hours = 1; hours <= PREDICTION_MAX_HOURS; hours++)
    {
        for (unsigned int correct = 0; correct <= hours; correct++)
        {
            const PredictionEntry entry = counted(hours, correct);
            if (PredictionPhase::isRankedByAccuracy(entry))
            {
                entries.push_back(entry);
            }
        }
    }
    ASSERT_GT(entries.size(), 10000u);

    unsigned long long wrong = 0;
    for (const PredictionEntry& a : entries)
    {
        const unsigned int keyA = PredictionPhase::rankingKey(a);
        for (const PredictionEntry& b : entries)
        {
            const unsigned int keyB = PredictionPhase::rankingKey(b);
            const unsigned int left = (unsigned int)a.correct * (unsigned int)b.hours;
            const unsigned int right = (unsigned int)b.correct * (unsigned int)a.hours;
            bool aFirst = false;
            bool bFirst = false;
            if (left != right)
            {
                aFirst = left > right;
                bFirst = !aFirst;
            }
            else if (a.hours != b.hours)
            {
                aFirst = a.hours > b.hours;
                bFirst = !aFirst;
            }
            if ((keyA < keyB) != aFirst || (keyB < keyA) != bFirst)
            {
                wrong++;
            }
        }
    }
    EXPECT_EQ(wrong, 0u);
}

TEST(TestPredictionPhase, RankingKeyValues)
{
    // 18 of 24 before 20 of 30; at the same accuracy 27 of 36 before 18 of 24.
    EXPECT_LT(PredictionPhase::rankingKey(counted(24, 18)), PredictionPhase::rankingKey(counted(30, 20)));
    EXPECT_LT(PredictionPhase::rankingKey(counted(36, 27)), PredictionPhase::rankingKey(counted(24, 18)));
    // A record of the minimum of correct hours with none wrong is before every record with a wrong hour.
    EXPECT_LT(PredictionPhase::rankingKey(counted(BPP9000_PREDICTION_MIN_CORRECT, BPP9000_PREDICTION_MIN_CORRECT)),
        PredictionPhase::rankingKey(counted(PREDICTION_MAX_HOURS, PREDICTION_MAX_HOURS - 1)));
    // Every key is below the key of an identity that is not ranked by accuracy.
    EXPECT_LT(PredictionPhase::rankingKey(counted(PREDICTION_MAX_HOURS, 0)), PREDICTION_NO_RANKING_KEY);
    // The best possible key: every hour of the epoch right.
    EXPECT_EQ(PredictionPhase::rankingKey(counted(PREDICTION_MAX_HOURS, PREDICTION_MAX_HOURS)), 0xFFU - PREDICTION_MAX_HOURS);
}

TEST(TestPredictionPhase, ClosesEachHourOnceAfterItsDeadline)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long nowMs = PredictionPhase::deadlineMs(hour);
    unsigned int closed = 0;
    EXPECT_FALSE(phase->closeNextHour(nowMs + 1, closed));

    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, nowMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));
    EXPECT_FALSE(phase->closeNextHour(nowMs, closed));
    EXPECT_TRUE(phase->closeNextHour(nowMs + 1, closed));
    EXPECT_EQ(closed, hour);
    EXPECT_FALSE(phase->closeNextHour(nowMs + 1, closed));

    // Two deadlines passed since: both are closed, in order.
    const long long laterMs = PredictionPhase::deadlineMs(hour + 2) + 1;
    EXPECT_TRUE(phase->closeNextHour(laterMs, closed));
    EXPECT_EQ(closed, hour + 1);
    EXPECT_TRUE(phase->closeNextHour(laterMs, closed));
    EXPECT_EQ(closed, hour + 2);
    EXPECT_FALSE(phase->closeNextHour(laterMs, closed));
}

TEST(TestPredictionPhase, DigestFollowsTheCounts)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long nowMs = PredictionPhase::deadlineMs(hour);
    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, nowMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));

    auto copy = std::make_unique<PredictionPhase>();
    copyMem(copy.get(), phase.get(), sizeof(PredictionPhase));
    EXPECT_EQ(copy->digest(), phase->digest());

    phase->countHour(hour, BASE_HOUR, 1);
    copy->countHour(hour, BASE_HOUR, 0);
    EXPECT_NE(copy->digest(), phase->digest());
}

TEST(TestPredictionPhase, FoldKeepsTheDigestItReturned)
{
    auto phase = makePhase();
    const unsigned int hour = EPOCH_HOUR + 6;
    const long long nowMs = PredictionPhase::deadlineMs(hour);
    EXPECT_EQ(phase->foldedDigest, 0u);

    qualifyComputors(*phase, 0, BPP9000_PREDICTION_SWITCH_COMPUTORS, nowMs, 1);
    ASSERT_TRUE(phase->trySwitch(2000, nowMs, END_HOUR));
    EXPECT_EQ(phase->foldedDigest, 0u);

    const unsigned int atSwitch = phase->digest();
    EXPECT_EQ(phase->updateFoldedDigest(), atSwitch);
    EXPECT_EQ(phase->foldedDigest, atSwitch);
    // the kept value is not an input of the digest
    EXPECT_EQ(phase->digest(), atSwitch);

    // it stays until the next fold
    phase->countHour(hour, BASE_HOUR, 1);
    EXPECT_NE(phase->digest(), atSwitch);
    EXPECT_EQ(phase->foldedDigest, atSwitch);
    const unsigned int afterHour = phase->updateFoldedDigest();
    EXPECT_EQ(afterHour, phase->digest());
    EXPECT_EQ(phase->foldedDigest, afterHour);

    phase->reset();
    EXPECT_EQ(phase->foldedDigest, 0u);
}
