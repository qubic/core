#define NO_UEFI

#include "oracle_testing.h"

#include "mining/price_feed.h"

#include <memory>
#include <vector>

// Start of epoch 239, 2026-11-11 12:00 UTC.
static constexpr unsigned int BASE_HOUR = 168 * 239;

static QPI::DateAndTime timeAt(unsigned int hour, long long minutes = 0, long long seconds = 0)
{
    QPI::DateAndTime t;
    EXPECT_TRUE(btcFeedHourTime(hour, t));
    EXPECT_TRUE(t.add(0, 0, 0, 0, minutes, seconds));
    return t;
}

// Fills the hours before nextHour with a rising price and returns an image with header and checksum set.
static std::unique_ptr<BtcPriceImage> makeImage(unsigned int nextHour, unsigned int kind, unsigned int epoch, unsigned int tick,
    unsigned int numPoints = BTC_PRICE_MIN_POINTS)
{
    auto image = std::make_unique<BtcPriceImage>();
    setMem(image.get(), sizeof(BtcPriceImage), 0);
    image->nextHour = nextHour;
    image->baseHour = nextHour - 1;
    for (unsigned int i = 1; i <= numPoints; ++i)
    {
        const unsigned int hour = nextHour - i;
        BtcPricePoint& point = image->points[hour & (BTC_PRICE_RING_SIZE - 1)];
        point.hour = hour;
        point.priceE8 = 5000000000000LL + hour;
    }
    btcFeedSetImageHeader(*image, kind, epoch, tick);
    return image;
}

static std::unique_ptr<BtcPriceFeed> makeSyncedFeed(unsigned int nextHour = BASE_HOUR)
{
    auto feed = std::make_unique<BtcPriceFeed>();
    feed->reset();
    const auto image = makeImage(nextHour, BTC_PRICE_KIND_SWITCH, 239, 1000);
    feed->adoptImage(*image);
    return feed;
}

// The shift limit of a solution anchored at anchorTick. False if the feed holds no series.
static bool shiftLimitAt(BtcPriceFeed& feed, unsigned int anchorTick, unsigned int& limit)
{
    std::vector<unsigned char> targets(BPP9000_SEQUENCE_LENGTH);
    unsigned int baseHour = 0;
    return feed.taskDataAt(anchorTick, targets.data(), (unsigned int)BPP9000_SEQUENCE_LENGTH, limit, baseHour) != 0;
}

struct FeedQueryTx
{
    std::vector<unsigned char> bytes;

    FeedQueryTx(unsigned int hour, unsigned int tick, const m256i& source)
    {
        bytes.resize(sizeof(OracleUserQueryTransactionPrefix) + sizeof(OI::Price::OracleQuery) + SIGNATURE_SIZE, 0);
        OracleUserQueryTransactionPrefix prefix;
        setMem(&prefix, sizeof(prefix), 0);
        prefix.sourcePublicKey = source;
        prefix.destinationPublicKey = m256i::zero();
        prefix.amount = 0;
        prefix.tick = tick;
        prefix.inputType = OracleUserQueryTransactionPrefix::transactionType();
        prefix.inputSize = OracleUserQueryTransactionPrefix::minInputSize() + sizeof(OI::Price::OracleQuery);
        prefix.oracleInterfaceIndex = OI::Price::oracleInterfaceIndex;
        prefix.timeoutMilliseconds = BTC_FEED_QUERY_TIMEOUT_MS;
        copyMem(bytes.data(), &prefix, sizeof(prefix));
        OI::Price::OracleQuery q;
        EXPECT_TRUE(btcFeedMakeQuery(hour, q));
        setQuery(q);
    }

    OracleUserQueryTransactionPrefix prefix() const
    {
        OracleUserQueryTransactionPrefix p;
        copyMem(&p, bytes.data(), sizeof(p));
        return p;
    }

    void setPrefix(const OracleUserQueryTransactionPrefix& p)
    {
        copyMem(bytes.data(), &p, sizeof(p));
    }

    OI::Price::OracleQuery query() const
    {
        OI::Price::OracleQuery q;
        copyMem(&q, bytes.data() + sizeof(OracleUserQueryTransactionPrefix), sizeof(q));
        return q;
    }

    void setQuery(const OI::Price::OracleQuery& q)
    {
        copyMem(bytes.data() + sizeof(OracleUserQueryTransactionPrefix), &q, sizeof(q));
    }

    const Transaction* tx() const
    {
        return (const Transaction*)bytes.data();
    }
};

static const m256i LEADER(11, 22, 33, 44);
static const m256i OTHER(55, 66, 77, 88);
static constexpr unsigned int SLOT_TICK = 91688000;

// Appends the next hour through a successful query that finishes at the given tick.
static void appendHour(BtcPriceFeed& feed, long long priceE8, unsigned int tick)
{
    const unsigned int hour = feed.image.nextHour;
    ASSERT_TRUE(feed.onQueryStarted(9000 + hour, hour));
    ASSERT_TRUE(feed.onQueryFinished(9000 + hour, ORACLE_QUERY_STATUS_SUCCESS, true, priceE8, 100000000, timeAt(hour, 6), tick));
    ASSERT_EQ(feed.image.nextHour, hour + 1);
}

static constexpr unsigned int WINDOW = (unsigned int)BPP9000_WINDOW_WIDTH;
static constexpr unsigned int SEQUENCE = (unsigned int)BPP9000_SEQUENCE_LENGTH;

static bool isFree(const FeedQueryTx& t, unsigned int tick, const QPI::DateAndTime& now)
{
    unsigned int hour = 0;
    return isFreeBtcFeedQuery(t.tx(), tick, LEADER, now, hour);
}

// Hour index of a time on the hour.
static unsigned int hourOf(const QPI::DateAndTime& t)
{
    long long ms = -1;
    EXPECT_TRUE(btcFeedTimeToMs(t, ms));
    EXPECT_EQ(ms % BTC_FEED_MS_PER_HOUR, 0);
    return (unsigned int)(ms / BTC_FEED_MS_PER_HOUR);
}

// Hour 0 is 2022-04-13 12:00 UTC and an epoch is 168 hours; month and year ends carry over.
TEST(TestBtcPriceFeed, HourIndexMatchesEpochStart)
{
    EXPECT_EQ(hourOf(QPI::DateAndTime(2022, 4, 13, 12)), 0u);
    EXPECT_EQ(hourOf(QPI::DateAndTime(2026, 11, 4, 12)), 168u * 238u);
    EXPECT_EQ(hourOf(QPI::DateAndTime(2026, 11, 11, 12)), BASE_HOUR);
    QPI::DateAndTime next;
    EXPECT_TRUE(btcFeedHourTime(hourOf(QPI::DateAndTime(2024, 2, 29, 23)) + 1, next));
    EXPECT_EQ(next, QPI::DateAndTime(2024, 3, 1, 0));
    EXPECT_TRUE(btcFeedHourTime(hourOf(QPI::DateAndTime(2026, 12, 31, 23)) + 1, next));
    EXPECT_EQ(next, QPI::DateAndTime(2027, 1, 1, 0));
}

// btcFeedHourTime and btcFeedTimeToMs are inverse on the hour grid.
TEST(TestBtcPriceFeed, HourTimeRoundTrip)
{
    for (unsigned int hour = 0; hour < 200000; hour += 37)
    {
        QPI::DateAndTime t;
        ASSERT_TRUE(btcFeedHourTime(hour, t));
        long long ms = 0;
        ASSERT_TRUE(btcFeedTimeToMs(t, ms));
        ASSERT_EQ(ms, (long long)hour * BTC_FEED_MS_PER_HOUR);
    }
}

// An invalid date, a year outside 2022..2099 and a time before hour 0 give no value.
TEST(TestBtcPriceFeed, TimeToMsRejectsInvalidAndOutOfRangeTimes)
{
    long long ms = 0;
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(), ms));
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(2026, 2, 30, 12), ms));
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(2026, 13, 1, 12), ms));
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(2021, 11, 11, 12), ms));
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(2100, 1, 1, 0), ms));
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(2022, 4, 13, 11), ms));
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(26, 11, 11, 12), ms));
}

// The value counts milliseconds from hour 0.
TEST(TestBtcPriceFeed, TimeToMsOrdersTimes)
{
    long long a = 0;
    long long b = 0;
    EXPECT_TRUE(btcFeedTimeToMs(QPI::DateAndTime(2026, 11, 11, 12, 4, 59, 999), a));
    EXPECT_TRUE(btcFeedTimeToMs(QPI::DateAndTime(2026, 11, 11, 12, 5, 0, 0), b));
    EXPECT_EQ(b - a, 1);
    EXPECT_FALSE(btcFeedTimeToMs(QPI::DateAndTime(2022, 4, 13, 11, 59, 59, 999), a));
}

// priceE8 = numerator * 10^8 / denominator, rounded down. A result of 0, a non-positive input and an overflow are refused.
TEST(TestBtcPriceFeed, PriceConversion)
{
    long long price = 0;
    EXPECT_TRUE(btcFeedPriceE8(6712345, 100, price));
    EXPECT_EQ(price, 6712345000000LL);
    EXPECT_TRUE(btcFeedPriceE8(6712345000000LL, 100000000, price));
    EXPECT_EQ(price, 6712345000000LL);
    EXPECT_TRUE(btcFeedPriceE8(67123, 1, price));
    EXPECT_EQ(price, 6712300000000LL);
    EXPECT_TRUE(btcFeedPriceE8(13424691, 200, price));
    EXPECT_EQ(price, 6712345500000LL);
    EXPECT_TRUE(btcFeedPriceE8(1, 3, price));
    EXPECT_EQ(price, 33333333LL);
    EXPECT_FALSE(btcFeedPriceE8(1, 200000000, price));
    EXPECT_FALSE(btcFeedPriceE8(0, 1, price));
    EXPECT_FALSE(btcFeedPriceE8(-5, 1, price));
    EXPECT_FALSE(btcFeedPriceE8(5, 0, price));
    EXPECT_FALSE(btcFeedPriceE8(5, -1, price));
    EXPECT_FALSE(btcFeedPriceE8(0x7FFFFFFFFFFFFFFFLL, 1, price));
    EXPECT_TRUE(btcFeedPriceE8(92233720368LL, 1, price));
    EXPECT_FALSE(btcFeedPriceE8(92233720369LL, 1, price));
}

// The query of btcFeedMakeQuery is free once its hour is BTC_FEED_QUERY_DELAY_SEC old.
TEST(TestBtcPriceFeed, FreeQueryRuleAcceptsTheCanonicalQuery)
{
    const FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
    unsigned int hour = 0;
    EXPECT_TRUE(isFreeBtcFeedQuery(t.tx(), SLOT_TICK, LEADER, timeAt(BASE_HOUR, 5), hour));
    EXPECT_EQ(hour, BASE_HOUR);
    EXPECT_TRUE(isFree(t, SLOT_TICK, timeAt(BASE_HOUR + 100)));

    // Too recent: the hour must be over for BTC_FEED_QUERY_DELAY_SEC.
    EXPECT_FALSE(isFree(t, SLOT_TICK, timeAt(BASE_HOUR, 4, 59)));
    EXPECT_FALSE(isFree(t, SLOT_TICK, timeAt(BASE_HOUR - 1)));
    // Clock of a from-scratch start.
    EXPECT_FALSE(isFree(t, SLOT_TICK, QPI::DateAndTime(2022, 4, 13, 12)));
    EXPECT_FALSE(isFree(t, SLOT_TICK, QPI::DateAndTime()));
}

// Free only in a slot tick, from the leader of that tick, and for the tick it executes in.
TEST(TestBtcPriceFeed, FreeQueryRuleSlotAndLeader)
{
    const QPI::DateAndTime now = timeAt(BASE_HOUR, 10);
    for (unsigned int offset = 1; offset < BTC_FEED_SLOT_PERIOD; ++offset)
    {
        const FeedQueryTx t(BASE_HOUR, SLOT_TICK + offset, LEADER);
        EXPECT_FALSE(isFree(t, SLOT_TICK + offset, now));
    }
    const FeedQueryTx nextSlot(BASE_HOUR, SLOT_TICK + BTC_FEED_SLOT_PERIOD, LEADER);
    EXPECT_TRUE(isFree(nextSlot, SLOT_TICK + BTC_FEED_SLOT_PERIOD, now));

    const FeedQueryTx notLeader(BASE_HOUR, SLOT_TICK, OTHER);
    EXPECT_FALSE(isFree(notLeader, SLOT_TICK, now));

    // The transaction must be scheduled for the executing tick.
    const FeedQueryTx otherTick(BASE_HOUR, SLOT_TICK + BTC_FEED_SLOT_PERIOD, LEADER);
    EXPECT_FALSE(isFree(otherTick, SLOT_TICK, now));
}

// Another amount, timeout, interface, input size, input type or destination is not free.
TEST(TestBtcPriceFeed, FreeQueryRuleTransactionFields)
{
    const QPI::DateAndTime now = timeAt(BASE_HOUR, 10);
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OracleUserQueryTransactionPrefix p = t.prefix();
        p.amount = 10;
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OracleUserQueryTransactionPrefix p = t.prefix();
        p.timeoutMilliseconds = BTC_FEED_QUERY_TIMEOUT_MS + 1;
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        p.timeoutMilliseconds = 3600000;
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OracleUserQueryTransactionPrefix p = t.prefix();
        p.oracleInterfaceIndex = OI::Mock::oracleInterfaceIndex;
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OracleUserQueryTransactionPrefix p = t.prefix();
        p.inputSize = (unsigned short)(p.inputSize - 1);
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        p.inputSize = OracleUserQueryTransactionPrefix::minInputSize();
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OracleUserQueryTransactionPrefix p = t.prefix();
        p.inputType = 0;
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OracleUserQueryTransactionPrefix p = t.prefix();
        p.destinationPublicKey = OTHER;
        t.setPrefix(p);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
}

// Another oracle or currency pair, a timestamp off the hour grid and an hour below BTC_FEED_MIN_HOUR are not free.
TEST(TestBtcPriceFeed, FreeQueryRuleQueryFields)
{
    using namespace QPI::Ch;
    const QPI::DateAndTime now = timeAt(BASE_HOUR + 1000);
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OI::Price::OracleQuery q = t.query();
        q.oracle = OI::Price::getMexcOracleId();
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OI::Price::OracleQuery q = t.query();
        q.currency1 = QPI::id(E, T, H, null, null);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OI::Price::OracleQuery q = t.query();
        q.currency2 = QPI::id(U, S, D, C, null);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        // A byte behind the name still counts.
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OI::Price::OracleQuery q = t.query();
        q.oracle.m256i_u8[31] = 1;
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        FeedQueryTx t(BASE_HOUR, SLOT_TICK, LEADER);
        OI::Price::OracleQuery q = t.query();
        q.timestamp = QPI::DateAndTime(2026, 11, 11, 12, 1);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        q.timestamp = QPI::DateAndTime(2026, 11, 11, 12, 0, 1);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        q.timestamp = QPI::DateAndTime(2026, 11, 11, 12, 0, 0, 1);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        q.timestamp = QPI::DateAndTime(2026, 11, 11, 12, 0, 0, 0, 7);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        q.timestamp = QPI::DateAndTime();
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
        q.timestamp = QPI::DateAndTime(2026, 2, 30, 12);
        t.setQuery(q);
        EXPECT_FALSE(isFree(t, SLOT_TICK, now));
    }
    {
        const FeedQueryTx atMinHour(BTC_FEED_MIN_HOUR, SLOT_TICK, LEADER);
        EXPECT_TRUE(isFree(atMinHour, SLOT_TICK, now));
        const FeedQueryTx belowMinHour(BTC_FEED_MIN_HOUR - 1, SLOT_TICK, LEADER);
        EXPECT_FALSE(isFree(belowMinHour, SLOT_TICK, now));
    }
}

// The first query of a tick takes the free slot.
TEST(TestBtcPriceFeed, OnlyOneFreeQueryPerTick)
{
    auto feed = makeSyncedFeed();
    EXPECT_TRUE(feed->consumeFreeQuerySlot(SLOT_TICK));
    EXPECT_FALSE(feed->consumeFreeQuerySlot(SLOT_TICK));
    EXPECT_FALSE(feed->consumeFreeQuerySlot(SLOT_TICK));
    EXPECT_TRUE(feed->consumeFreeQuerySlot(SLOT_TICK + BTC_FEED_SLOT_PERIOD));

    // The limit does not depend on the node holding a series.
    auto idle = std::make_unique<BtcPriceFeed>();
    idle->reset();
    EXPECT_TRUE(idle->consumeFreeQuerySlot(SLOT_TICK));
    EXPECT_FALSE(idle->consumeFreeQuerySlot(SLOT_TICK));
}

// A successful reply to the bound query stores the hour and moves nextHour.
TEST(TestBtcPriceFeed, SuccessAppendsAndAdvances)
{
    auto feed = makeSyncedFeed();
    unsigned char digestBefore[32];
    copyMem(digestBefore, feed->seriesDigest, 32);

    EXPECT_TRUE(feed->onQueryStarted(777, BASE_HOUR));
    EXPECT_TRUE(feed->isBound());
    // A second query for the same hour is not bound.
    EXPECT_FALSE(feed->onQueryStarted(778, BASE_HOUR));
    // A reply of another query is not ours.
    EXPECT_FALSE(feed->onQueryFinished(778, ORACLE_QUERY_STATUS_SUCCESS, true, 1, 1, timeAt(BASE_HOUR, 6), 1234));
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR);

    EXPECT_TRUE(feed->onQueryFinished(777, ORACLE_QUERY_STATUS_SUCCESS, true, 10300050, 100, timeAt(BASE_HOUR, 6), 1234));
    EXPECT_FALSE(feed->isBound());
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR + 1);
    const BtcPricePoint* point = feed->findPoint(BASE_HOUR);
    ASSERT_NE(point, nullptr);
    EXPECT_EQ(point->priceE8, 10300050000000LL);
    EXPECT_EQ(point->flags, 0u);
    EXPECT_EQ(point->appendTick, 1234u);
    EXPECT_EQ(feed->numSuccess, 1ull);
    EXPECT_NE(compareMem(digestBefore, feed->seriesDigest, 32), 0);

    // The finished query is not delivered twice.
    EXPECT_FALSE(feed->onQueryFinished(777, ORACLE_QUERY_STATUS_SUCCESS, true, 1, 1, timeAt(BASE_HOUR, 7), 1235));
}

// Only a query for nextHour is bound.
TEST(TestBtcPriceFeed, QueryForAnotherHourIsNotBound)
{
    auto feed = makeSyncedFeed();
    EXPECT_FALSE(feed->onQueryStarted(1, BASE_HOUR - 1));
    EXPECT_FALSE(feed->onQueryStarted(2, BASE_HOUR + 1));
    EXPECT_FALSE(feed->isBound());
}

// A timeout, an unresolvable query and an invalid reply are failed attempts; the next attempt waits
// BTC_FEED_RETRY_BACKOFF_SEC.
TEST(TestBtcPriceFeed, FailureKeepsTheHourAndBacksOff)
{
    auto feed = makeSyncedFeed();
    unsigned int hour = 0;
    EXPECT_TRUE(feed->shouldSend(SLOT_TICK, timeAt(BASE_HOUR, 5), hour));
    EXPECT_EQ(hour, BASE_HOUR);

    EXPECT_TRUE(feed->onQueryStarted(10, BASE_HOUR));
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK + 5, timeAt(BASE_HOUR, 6), hour));
    EXPECT_TRUE(feed->onQueryFinished(10, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR, 8), 2000));
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR);
    EXPECT_EQ(feed->image.failedAttempts, 1u);
    EXPECT_EQ(feed->numTimeout, 1ull);
    EXPECT_FALSE(feed->isBound());

    // No new attempt during the back-off.
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK + 10, timeAt(BASE_HOUR, 12), hour));
    EXPECT_TRUE(feed->shouldSend(SLOT_TICK + 10, timeAt(BASE_HOUR, 13), hour));

    EXPECT_TRUE(feed->onQueryStarted(11, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(11, ORACLE_QUERY_STATUS_UNRESOLVABLE, false, 0, 0, timeAt(BASE_HOUR, 20), 2100));
    EXPECT_EQ(feed->image.failedAttempts, 2u);
    EXPECT_EQ(feed->numUnresolvable, 1ull);

    // An invalid reply is a failed attempt too.
    EXPECT_TRUE(feed->onQueryStarted(12, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(12, ORACLE_QUERY_STATUS_SUCCESS, true, 0, 1, timeAt(BASE_HOUR, 40), 2200));
    EXPECT_EQ(feed->image.failedAttempts, 3u);
    EXPECT_EQ(feed->numInvalidReply, 1ull);
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR);

    // A later success clears the attempts.
    EXPECT_TRUE(feed->onQueryStarted(13, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(13, ORACLE_QUERY_STATUS_SUCCESS, true, 5, 1, timeAt(BASE_HOUR, 50), 2300));
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR + 1);
    EXPECT_EQ(feed->image.failedAttempts, 0u);
    EXPECT_EQ(feed->image.firstFailMs, 0);
    EXPECT_EQ(feed->image.lastFailMs, 0);
}

// A failed attempt BTC_FEED_GIVE_UP_SEC after the first one stores the previous price, flagged as filled.
TEST(TestBtcPriceFeed, GiveUpOnlyAfterADayOfFailedAttempts)
{
    auto feed = makeSyncedFeed();
    const long long previousPrice = feed->findPoint(BASE_HOUR - 1)->priceE8;

    EXPECT_TRUE(feed->onQueryStarted(1, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(1, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR, 10), 100));

    // One second before the day is over: still retrying.
    EXPECT_TRUE(feed->onQueryStarted(2, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(2, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR + 24, 9, 59), 200));
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR);
    EXPECT_EQ(feed->numFilled, 0ull);

    // A failed attempt a day after the first one fills the hour with the previous price.
    EXPECT_TRUE(feed->onQueryStarted(3, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(3, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR + 24, 10), 300));
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR + 1);
    EXPECT_EQ(feed->numFilled, 1ull);
    const BtcPricePoint* filled = feed->findPoint(BASE_HOUR);
    ASSERT_NE(filled, nullptr);
    EXPECT_EQ(filled->priceE8, previousPrice);
    EXPECT_EQ(filled->flags, BTC_PRICE_FLAG_FILLED);
    EXPECT_EQ(feed->image.failedAttempts, 0u);

    // The next hour gets its own day.
    EXPECT_TRUE(feed->onQueryStarted(4, BASE_HOUR + 1));
    EXPECT_TRUE(feed->onQueryFinished(4, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR + 24, 20), 400));
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR + 1);
    EXPECT_EQ(feed->image.failedAttempts, 1u);
}

// A series that is a week behind is caught up hour by hour with fetched prices.
TEST(TestBtcPriceFeed, CatchUpFetchesOldHoursInsteadOfFillingThem)
{
    auto feed = makeSyncedFeed(BASE_HOUR - 168);
    const QPI::DateAndTime now = timeAt(BASE_HOUR, 30);
    long long queryId = 1;
    for (unsigned int i = 0; i < 168; ++i)
    {
        unsigned int hour = 0;
        ASSERT_TRUE(feed->shouldSend(SLOT_TICK + i * BTC_FEED_SLOT_PERIOD, now, hour));
        ASSERT_TRUE(feed->onQueryStarted(queryId, hour));
        ASSERT_TRUE(feed->onQueryFinished(queryId, ORACLE_QUERY_STATUS_SUCCESS, true, 60000 + i, 1, now, 5000 + i));
        queryId++;
    }
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR);
    EXPECT_EQ(feed->numFilled, 0ull);
    EXPECT_EQ(feed->numSuccess, 168ull);
}

// The give-up time is checked only when an attempt fails.
TEST(TestBtcPriceFeed, LateRestartDoesNotFillWithoutAFailedAttempt)
{
    auto feed = makeSyncedFeed();
    EXPECT_TRUE(feed->onQueryStarted(1, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(1, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR, 10), 100));

    // 30 hours pass with no attempt; the first attempt after that succeeds
    unsigned int hour = 0;
    EXPECT_TRUE(feed->shouldSend(SLOT_TICK, timeAt(BASE_HOUR + 30), hour));
    EXPECT_TRUE(feed->onQueryStarted(2, hour));
    EXPECT_TRUE(feed->onQueryFinished(2, ORACLE_QUERY_STATUS_SUCCESS, true, 70000, 1, timeAt(BASE_HOUR + 30, 1), 200));
    EXPECT_EQ(feed->numFilled, 0ull);
    EXPECT_EQ(feed->findPoint(BASE_HOUR)->flags, 0u);
}

// A query is sent in a slot tick, after the query delay, and only by a synced feed.
TEST(TestBtcPriceFeed, ShouldSendConditions)
{
    auto feed = makeSyncedFeed();
    unsigned int hour = 0;
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK, timeAt(BASE_HOUR, 4, 59), hour));
    EXPECT_TRUE(feed->shouldSend(SLOT_TICK, timeAt(BASE_HOUR, 5), hour));
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK + 1, timeAt(BASE_HOUR, 5), hour));
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK, QPI::DateAndTime(2022, 4, 13, 12), hour));

    feed->synced = false;
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK, timeAt(BASE_HOUR, 5), hour));
    EXPECT_FALSE(feed->onQueryStarted(1, BASE_HOUR));
}

// The epoch switch drops the bound query without a failed attempt.
TEST(TestBtcPriceFeed, EpochSwitchUnbindsWithoutCountingAFailure)
{
    auto feed = makeSyncedFeed();
    EXPECT_TRUE(feed->onQueryStarted(1, BASE_HOUR));
    feed->onEpochSwitch();
    EXPECT_FALSE(feed->isBound());
    EXPECT_TRUE(feed->synced);
    EXPECT_EQ(feed->image.failedAttempts, 0u);
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR);
}

// A bound query that is neither pending nor committed leaves the feed unsynced.
TEST(TestBtcPriceFeed, BoundQueryMustStillBeRunning)
{
    auto feed = makeSyncedFeed();
    feed->checkBoundQuery(ORACLE_QUERY_STATUS_UNKNOWN);
    EXPECT_TRUE(feed->synced);

    EXPECT_TRUE(feed->onQueryStarted(1, BASE_HOUR));
    feed->checkBoundQuery(ORACLE_QUERY_STATUS_PENDING);
    feed->checkBoundQuery(ORACLE_QUERY_STATUS_COMMITTED);
    EXPECT_TRUE(feed->isBound());
    EXPECT_TRUE(feed->synced);

    feed->checkBoundQuery(ORACLE_QUERY_STATUS_SUCCESS);
    EXPECT_FALSE(feed->isBound());
    EXPECT_FALSE(feed->synced);
}

// A target is 1 when the price in whole USDT, rounded half up, is not below the hour before.
TEST(TestBtcPriceFeed, TaskTargetsCompareWholeUsdtPrices)
{
    // The stored hours all round to 50000 USDT, so every target of the first frame is "not below".
    auto feed = makeSyncedFeed();
    EXPECT_EQ(feed->image.baseHour, BASE_HOUR - 1);
    std::vector<unsigned char> targets(SEQUENCE, 7);
    ASSERT_EQ(feed->buildTaskTargets(targets.data(), SEQUENCE), WINDOW);
    for (unsigned int t = 0; t < WINDOW; ++t)
    {
        ASSERT_EQ(targets[t], 1) << "target " << t;
    }
    EXPECT_EQ(targets[WINDOW], 7);

    appendHour(*feed, 5000049999999LL, 2000);   // 50000.49999999 -> 50000, held
    appendHour(*feed, 5000050000000LL, 2010);   // 50000.5 -> 50001, up
    appendHour(*feed, 5000049999999LL, 2020);   // 50000, down
    appendHour(*feed, 4999950000000LL, 2030);   // 49999.5 -> 50000, held
    appendHour(*feed, 4999949999999LL, 2040);   // 49999, down
    ASSERT_EQ(feed->buildTaskTargets(targets.data(), SEQUENCE), WINDOW + 5);
    const unsigned char expected[5] = { 1, 1, 0, 1, 0 };
    for (unsigned int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(targets[WINDOW + i], expected[i]) << "hour " << i;
    }
    EXPECT_EQ(targets[WINDOW + 5], 7);

    // The caller's buffer bounds the output.
    std::vector<unsigned char> shortTargets(WINDOW + 2, 7);
    EXPECT_EQ(feed->buildTaskTargets(shortTargets.data(), WINDOW + 2), WINDOW + 2);

    // A node without the series has no task.
    feed->synced = false;
    EXPECT_EQ(feed->buildTaskTargets(targets.data(), SEQUENCE), 0u);
}

// A filled hour repeats the previous price: its target is 1 and it counts for the shift limit.
TEST(TestBtcPriceFeed, FilledHourCountsAsHeld)
{
    auto feed = makeSyncedFeed();
    EXPECT_TRUE(feed->onQueryStarted(1, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(1, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0, timeAt(BASE_HOUR, 6), 3000));
    EXPECT_TRUE(feed->onQueryStarted(2, BASE_HOUR));
    EXPECT_TRUE(feed->onQueryFinished(2, ORACLE_QUERY_STATUS_TIMEOUT, false, 0, 0,
        timeAt(BASE_HOUR, 6, BTC_FEED_GIVE_UP_SEC), 3500));
    ASSERT_EQ(feed->image.nextHour, BASE_HOUR + 1);
    std::vector<unsigned char> targets(SEQUENCE, 7);
    ASSERT_EQ(feed->buildTaskTargets(targets.data(), SEQUENCE), WINDOW + 1);
    EXPECT_EQ(targets[WINDOW], 1);
    unsigned int limit = 99;
    EXPECT_TRUE(shiftLimitAt(*feed, 3501, limit));
    EXPECT_EQ(limit, 1u);
}

// The shift limit is the number of hours stored in a tick before the anchor tick.
TEST(TestBtcPriceFeed, ShiftLimitCountsHoursAppendedBeforeTheAnchorTick)
{
    auto feed = makeSyncedFeed();
    unsigned int limit = 99;
    EXPECT_TRUE(shiftLimitAt(*feed, 5000, limit));
    EXPECT_EQ(limit, 0u);

    appendHour(*feed, 5000100000000LL, 2000);
    appendHour(*feed, 5000200000000LL, 2020);
    appendHour(*feed, 5000300000000LL, 2040);
    const unsigned int anchors[7] = { 1999, 2000, 2001, 2020, 2021, 2041, 0xFFFFFFFFu };
    const unsigned int expected[7] = { 0, 0, 1, 1, 2, 3, 3 };
    for (unsigned int i = 0; i < 7; ++i)
    {
        limit = 99;
        EXPECT_TRUE(shiftLimitAt(*feed, anchors[i], limit));
        EXPECT_EQ(limit, expected[i]) << "anchor " << anchors[i];
    }

    feed->synced = false;
    EXPECT_FALSE(shiftLimitAt(*feed, 5000, limit));
}

// The shift limit does not exceed BPP9000_SHIFT_CAP.
TEST(TestBtcPriceFeed, ShiftLimitStopsAtTheCap)
{
    auto feed = makeSyncedFeed();
    const unsigned int cap = (unsigned int)BPP9000_SHIFT_CAP;
    for (unsigned int i = 0; i < cap + 3; ++i)
    {
        appendHour(*feed, 5000100000000LL + i, 2000 + i);
    }
    unsigned int limit = 0;
    EXPECT_TRUE(shiftLimitAt(*feed, 2000 + cap - 1, limit));
    EXPECT_EQ(limit, cap - 1);
    EXPECT_TRUE(shiftLimitAt(*feed, 2000 + cap + 3, limit));
    EXPECT_EQ(limit, cap);
    std::vector<unsigned char> targets(SEQUENCE, 7);
    EXPECT_EQ(feed->buildTaskTargets(targets.data(), SEQUENCE), SEQUENCE);
}

// At the epoch switch the base hour becomes the last stored hour, and the first frame moves with it.
TEST(TestBtcPriceFeed, EpochSwitchMovesTheBaseHour)
{
    auto feed = makeSyncedFeed();
    appendHour(*feed, 5000150000000LL, 2000);
    appendHour(*feed, 4999850000000LL, 2020);
    appendHour(*feed, 5000250000000LL, 2040);
    std::vector<unsigned char> before(SEQUENCE, 7);
    ASSERT_EQ(feed->buildTaskTargets(before.data(), SEQUENCE), WINDOW + 3);
    unsigned char digestBefore[32];
    copyMem(digestBefore, feed->seriesDigest, 32);

    // An image taken before the switch cannot start the next epoch.
    auto image = std::make_unique<BtcPriceImage>();
    feed->exportImage(*image, BTC_PRICE_KIND_SWITCH, 240, 3000);
    EXPECT_TRUE(btcFeedImageIsValid(*image));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*image, 240, 3000, true));
    feed->exportImage(*image, BTC_PRICE_KIND_F6, 239, 2500);
    EXPECT_TRUE(btcFeedImageIsAcceptable(*image, 239, 2600, false));

    feed->onEpochSwitch();
    EXPECT_EQ(feed->image.baseHour, BASE_HOUR + 2);
    EXPECT_EQ(feed->image.nextHour, BASE_HOUR + 3);
    EXPECT_NE(compareMem(digestBefore, feed->seriesDigest, 32), 0);
    unsigned int limit = 99;
    EXPECT_TRUE(shiftLimitAt(*feed, 0xFFFFFFFFu, limit));
    EXPECT_EQ(limit, 0u);

    // The new first frame is the old one moved by the three hours.
    std::vector<unsigned char> after(SEQUENCE, 7);
    ASSERT_EQ(feed->buildTaskTargets(after.data(), SEQUENCE), WINDOW);
    for (unsigned int t = 0; t < WINDOW; ++t)
    {
        ASSERT_EQ(after[t], before[t + 3]) << "target " << t;
    }

    feed->exportImage(*image, BTC_PRICE_KIND_SWITCH, 240, 3000);
    EXPECT_TRUE(btcFeedImageIsValid(*image));
    EXPECT_TRUE(btcFeedImageIsAcceptable(*image, 240, 3000, true));

    // A node without the series keeps none.
    auto idle = std::make_unique<BtcPriceFeed>();
    idle->reset();
    idle->onEpochSwitch();
    EXPECT_EQ(idle->image.baseHour, 0u);
}

// taskDataAt returns one frame plus the hours stored before the anchor tick, and nothing behind them.
TEST(TestBtcPriceFeed, TaskDataForAnAnchorTick)
{
    auto feed = makeSyncedFeed();
    appendHour(*feed, 5000150000000LL, 2000);
    appendHour(*feed, 4999850000000LL, 2020);
    appendHour(*feed, 5000250000000LL, 2040);
    std::vector<unsigned char> all(SEQUENCE, 7);
    ASSERT_EQ(feed->buildTaskTargets(all.data(), SEQUENCE), WINDOW + 3);

    const unsigned int anchors[4] = { 2000, 2001, 2021, 9999 };
    const unsigned int limits[4] = { 0, 1, 2, 3 };
    for (unsigned int i = 0; i < 4; ++i)
    {
        std::vector<unsigned char> targets(SEQUENCE, 7);
        unsigned int limit = 99;
        unsigned int baseHour = 0;
        ASSERT_EQ(feed->taskDataAt(anchors[i], targets.data(), SEQUENCE, limit, baseHour), WINDOW + limits[i]) << "anchor " << anchors[i];
        EXPECT_EQ(limit, limits[i]);
        EXPECT_EQ(baseHour, BASE_HOUR - 1);
        EXPECT_EQ(memcmp(targets.data(), all.data(), WINDOW + limits[i]), 0);
        // Nothing after the hours of this anchor is handed out.
        EXPECT_EQ(targets[WINDOW + limits[i]], 7);
    }

    // A buffer that cannot hold the answer, and a node without the series.
    std::vector<unsigned char> targets(SEQUENCE, 7);
    unsigned int limit = 99;
    unsigned int baseHour = 99;
    EXPECT_EQ(feed->taskDataAt(9999, targets.data(), WINDOW + 2, limit, baseHour), 0u);
    feed->synced = false;
    EXPECT_EQ(feed->taskDataAt(9999, targets.data(), SEQUENCE, limit, baseHour), 0u);
    EXPECT_EQ(limit, 0u);
}

// Series steps are BTC_FEED_PERIOD_SEC apart, and only a timestamp on that grid is free.
TEST(TestBtcPriceFeed, SeriesStepsLieOnThePeriodGrid)
{
    for (unsigned int hour = BASE_HOUR; hour < BASE_HOUR + 5; ++hour)
    {
        QPI::DateAndTime t;
        ASSERT_TRUE(btcFeedHourTime(hour, t));
        long long ms = 0;
        ASSERT_TRUE(btcFeedTimeToMs(t, ms));
        EXPECT_EQ(ms, (long long)hour * BTC_FEED_PERIOD_SEC * 1000);

        // The query of the step is free; one second, one millisecond or one microsecond off the grid is not.
        const QPI::DateAndTime now = timeAt(hour + 1000);
        FeedQueryTx tx(hour, SLOT_TICK, LEADER);
        EXPECT_TRUE(isFree(tx, SLOT_TICK, now));
        OI::Price::OracleQuery q = tx.query();
        ASSERT_TRUE(q.timestamp.add(0, 0, 0, 0, 0, 1));
        tx.setQuery(q);
        EXPECT_FALSE(isFree(tx, SLOT_TICK, now));
        q.timestamp = t;
        ASSERT_TRUE(q.timestamp.add(0, 0, 0, 0, 0, 0, 1));
        tx.setQuery(q);
        EXPECT_FALSE(isFree(tx, SLOT_TICK, now));
        q.timestamp = t;
        ASSERT_TRUE(q.timestamp.add(0, 0, 0, 0, 0, 0, 0, 1));
        tx.setQuery(q);
        EXPECT_FALSE(isFree(tx, SLOT_TICK, now));
    }
    // Consecutive steps are one period apart.
    QPI::DateAndTime a;
    QPI::DateAndTime b;
    ASSERT_TRUE(btcFeedHourTime(BASE_HOUR, a));
    ASSERT_TRUE(btcFeedHourTime(BASE_HOUR + 1, b));
    long long msA = 0;
    long long msB = 0;
    ASSERT_TRUE(btcFeedTimeToMs(a, msA));
    ASSERT_TRUE(btcFeedTimeToMs(b, msB));
    EXPECT_EQ(msB - msA, (long long)BTC_FEED_PERIOD_SEC * 1000);
}

// The base hour must be before nextHour, and every hour of the first frame must be stored.
TEST(TestBtcPriceFeed, ImageValidationChecksTheBaseHour)
{
    auto image = makeImage(BASE_HOUR, BTC_PRICE_KIND_F6, 239, 1000);
    image->baseHour = BASE_HOUR;
    btcFeedSetImageHeader(*image, BTC_PRICE_KIND_F6, 239, 1000);
    EXPECT_FALSE(btcFeedImageIsValid(*image));

    // The first frame of the epoch must be covered: 168 hours into the epoch it starts 168 hours earlier.
    image->baseHour = BASE_HOUR - 1 - (unsigned int)BPP9000_SHIFT_CAP;
    btcFeedSetImageHeader(*image, BTC_PRICE_KIND_F6, 239, 1000);
    EXPECT_TRUE(btcFeedImageIsValid(*image));
    image->baseHour = BASE_HOUR - 2 - (unsigned int)BPP9000_SHIFT_CAP;
    btcFeedSetImageHeader(*image, BTC_PRICE_KIND_F6, 239, 1000);
    EXPECT_FALSE(btcFeedImageIsValid(*image));

    image->baseHour = WINDOW - 1;
    btcFeedSetImageHeader(*image, BTC_PRICE_KIND_F6, 239, 1000);
    EXPECT_FALSE(btcFeedImageIsValid(*image));
}

// A changed byte, a wrong header size, a short series, a gap and a zero price make an image invalid.
TEST(TestBtcPriceFeed, ImageValidation)
{
    auto image = makeImage(BASE_HOUR, BTC_PRICE_KIND_SWITCH, 239, 1000);
    EXPECT_TRUE(btcFeedImageIsValid(*image));

    image->points[5].priceE8 ^= 1;
    EXPECT_FALSE(btcFeedImageIsValid(*image));
    image->points[5].priceE8 ^= 1;
    EXPECT_TRUE(btcFeedImageIsValid(*image));

    image->headerSize = BTC_PRICE_IMAGE_HEADER_SIZE + 8;
    EXPECT_FALSE(btcFeedImageIsValid(*image));

    // Too short, a gap, a zero price.
    auto shortImage = makeImage(BASE_HOUR, BTC_PRICE_KIND_SWITCH, 239, 1000, BTC_PRICE_MIN_POINTS - 1);
    EXPECT_FALSE(btcFeedImageIsValid(*shortImage));

    auto gapImage = makeImage(BASE_HOUR, BTC_PRICE_KIND_SWITCH, 239, 1000);
    gapImage->points[(BASE_HOUR - 100) & (BTC_PRICE_RING_SIZE - 1)].hour = 1;
    btcFeedSetImageHeader(*gapImage, BTC_PRICE_KIND_SWITCH, 239, 1000);
    EXPECT_FALSE(btcFeedImageIsValid(*gapImage));

    auto zeroImage = makeImage(BASE_HOUR, BTC_PRICE_KIND_SWITCH, 239, 1000);
    zeroImage->points[(BASE_HOUR - 100) & (BTC_PRICE_RING_SIZE - 1)].priceE8 = 0;
    btcFeedSetImageHeader(*zeroImage, BTC_PRICE_KIND_SWITCH, 239, 1000);
    EXPECT_FALSE(btcFeedImageIsValid(*zeroImage));

    auto emptyImage = std::make_unique<BtcPriceImage>();
    setMem(emptyImage.get(), sizeof(BtcPriceImage), 0);
    EXPECT_FALSE(btcFeedImageIsValid(*emptyImage));
}

// A start at the first tick of an epoch takes the switch file or the bootstrap; a mid-epoch restart takes
// the F6 file.
TEST(TestBtcPriceFeed, WhichStoredImageMayStartTheSeries)
{
    const unsigned int initialTick = 91688000;
    const auto switchFile = makeImage(BASE_HOUR, BTC_PRICE_KIND_SWITCH, 239, 91600000);
    const auto f6File = makeImage(BASE_HOUR, BTC_PRICE_KIND_F6, 239, 91700000);
    const auto bootstrap = makeImage(BASE_HOUR, BTC_PRICE_KIND_BOOTSTRAP, 239, 0);
    const auto snapshot = makeImage(BASE_HOUR, BTC_PRICE_KIND_SNAPSHOT, 239, 91600000);

    // New epoch: the file of the epoch switch, or the bootstrap built for this epoch.
    EXPECT_TRUE(btcFeedImageIsAcceptable(*switchFile, 239, initialTick, true));
    EXPECT_TRUE(btcFeedImageIsAcceptable(*bootstrap, 239, initialTick, true));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*f6File, 239, 91800000, true));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*snapshot, 239, initialTick, true));

    // Network restarted in the middle of the epoch: only the F6 file, and only one saved before the new tick.
    EXPECT_TRUE(btcFeedImageIsAcceptable(*f6File, 239, 91800000, false));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*f6File, 239, 91650000, false));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*switchFile, 239, 91800000, false));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*bootstrap, 239, 91800000, false));

    // Another epoch.
    EXPECT_FALSE(btcFeedImageIsAcceptable(*switchFile, 240, initialTick, true));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*bootstrap, 240, initialTick, true));
    EXPECT_FALSE(btcFeedImageIsAcceptable(*f6File, 240, 91800000, false));

    // A switch file cannot be newer than the tick the epoch starts from.
    EXPECT_FALSE(btcFeedImageIsAcceptable(*switchFile, 239, 91500000, true));
}

// The exported image holds the series only: nodes with different node-local state export the same bytes.
TEST(TestBtcPriceFeed, ExportedImageCarriesTheSeriesAndIsIdenticalAcrossNodes)
{
    auto a = makeSyncedFeed();
    auto b = makeSyncedFeed();
    for (auto* feed : { a.get(), b.get() })
    {
        EXPECT_TRUE(feed->onQueryStarted(1, BASE_HOUR));
        EXPECT_TRUE(feed->onQueryFinished(1, ORACLE_QUERY_STATUS_SUCCESS, true, 6500012, 100, timeAt(BASE_HOUR, 6), 4242));
    }
    // Node-local state differs.
    b->numStartFailed = 9;
    b->lastFreeQueryTick = 5;
    EXPECT_TRUE(b->onQueryStarted(2, BASE_HOUR + 1));

    auto imageA = std::make_unique<BtcPriceImage>();
    auto imageB = std::make_unique<BtcPriceImage>();
    a->exportImage(*imageA, BTC_PRICE_KIND_SWITCH, 240, 93000000);
    b->exportImage(*imageB, BTC_PRICE_KIND_SWITCH, 240, 93000000);
    EXPECT_EQ(compareMem(imageA.get(), imageB.get(), sizeof(BtcPriceImage)), 0);
    EXPECT_EQ(compareMem(a->seriesDigest, b->seriesDigest, 32), 0);
    EXPECT_TRUE(btcFeedImageIsValid(*imageA));

    auto c = std::make_unique<BtcPriceFeed>();
    c->reset();
    c->adoptImage(*imageA);
    EXPECT_TRUE(c->synced);
    EXPECT_FALSE(c->isBound());
    EXPECT_EQ(c->image.nextHour, BASE_HOUR + 1);
    EXPECT_EQ(compareMem(a->seriesDigest, c->seriesDigest, 32), 0);
    EXPECT_EQ(c->findPoint(BASE_HOUR)->priceE8, 6500012000000LL);
}

// nextHour sits just past a multiple of the ring size, so the series wraps around the ring end.
TEST(TestBtcPriceFeed, RingWrapKeepsTheSeriesValid)
{
    const unsigned int nextHour = 3 * BTC_PRICE_RING_SIZE + 10;
    auto feed = makeSyncedFeed(nextHour);
    EXPECT_NE(feed->findPoint(nextHour - 1), nullptr);
    EXPECT_NE(feed->findPoint(nextHour - BTC_PRICE_MIN_POINTS), nullptr);
    EXPECT_EQ(feed->findPoint(nextHour), nullptr);
    EXPECT_EQ(feed->findPoint(nextHour - BTC_PRICE_MIN_POINTS - 1), nullptr);

    auto image = std::make_unique<BtcPriceImage>();
    feed->exportImage(*image, BTC_PRICE_KIND_F6, 239, 1);
    EXPECT_TRUE(btcFeedImageIsValid(*image));
}

// A feed after reset() holds no series and takes no query.
TEST(TestBtcPriceFeed, ZeroedStateHoldsNoPoint)
{
    auto feed = std::make_unique<BtcPriceFeed>();
    feed->reset();
    EXPECT_FALSE(feed->synced);
    EXPECT_FALSE(feed->isBound());
    EXPECT_EQ(feed->findPoint(0), nullptr);
    unsigned int hour = 0;
    EXPECT_FALSE(feed->shouldSend(SLOT_TICK, timeAt(BASE_HOUR, 30), hour));
    EXPECT_FALSE(feed->onQueryFinished(0, ORACLE_QUERY_STATUS_SUCCESS, true, 1, 1, timeAt(BASE_HOUR, 30), 1));
}

// A saved image file loads back byte for byte.
TEST(TestBtcPriceFeed, ImageFileRoundTrip)
{
    static unsigned short fileName[] = L"btcPriceSeriesTest.000";
    auto feed = makeSyncedFeed();
    auto buffer = std::make_unique<BtcPriceImage>();
    ASSERT_TRUE(btcFeedSaveImageFile(fileName, NULL, *feed, *buffer, BTC_PRICE_KIND_F6, 239, 91700000));

    auto loaded = std::make_unique<BtcPriceImage>();
    ASSERT_TRUE(btcFeedLoadImageFile(fileName, NULL, *loaded));
    EXPECT_EQ(loaded->kind, BTC_PRICE_KIND_F6);
    EXPECT_EQ(loaded->epoch, 239u);
    EXPECT_EQ(loaded->savedAtTick, 91700000u);
    EXPECT_EQ(compareMem(loaded.get(), buffer.get(), sizeof(BtcPriceImage)), 0);

    auto other = std::make_unique<BtcPriceFeed>();
    other->reset();
    other->adoptImage(*loaded);
    EXPECT_EQ(compareMem(other->seriesDigest, feed->seriesDigest, 32), 0);
}

// Reads and checks the snapshot file, then adopts it for the given tick.
static bool loadSnapshot(const CHAR16* fileName, BtcPriceFeed& feed, BtcPriceSnapshot& buffer, unsigned int epoch, unsigned int tick)
{
    feed.reset();
    return btcFeedReadSnapshotFile(fileName, NULL, buffer, epoch) && btcFeedAdoptSnapshot(feed, buffer, tick);
}

// A snapshot restores the bound query. One of another tick or epoch, or of an unsynced feed, is refused.
TEST(TestBtcPriceFeed, SnapshotKeepsTheBindingAndChecksTheTick)
{
    static unsigned short fileName[] = L"snapshotBtcPriceSeriesTest.239";
    auto feed = makeSyncedFeed();
    EXPECT_TRUE(feed->onQueryStarted(4711, BASE_HOUR));
    auto buffer = std::make_unique<BtcPriceSnapshot>();
    ASSERT_TRUE(btcFeedSaveSnapshotFile(fileName, NULL, *feed, *buffer, 239, 91700123));

    auto restored = std::make_unique<BtcPriceFeed>();
    EXPECT_TRUE(loadSnapshot(fileName, *restored, *buffer, 239, 91700123));
    EXPECT_TRUE(restored->synced);
    EXPECT_EQ(restored->boundQueryId, 4711);
    EXPECT_EQ(compareMem(restored->seriesDigest, feed->seriesDigest, 32), 0);

    // A snapshot of another tick or epoch leaves the feed idle.
    EXPECT_FALSE(loadSnapshot(fileName, *restored, *buffer, 239, 91700124));
    EXPECT_FALSE(restored->synced);
    EXPECT_FALSE(loadSnapshot(fileName, *restored, *buffer, 240, 91700123));
    EXPECT_FALSE(restored->synced);

    // So does the snapshot of a node that was not synced.
    feed->synced = false;
    ASSERT_TRUE(btcFeedSaveSnapshotFile(fileName, NULL, *feed, *buffer, 239, 91700123));
    EXPECT_FALSE(loadSnapshot(fileName, *restored, *buffer, 239, 91700123));
    EXPECT_FALSE(restored->synced);
}

// btcFeedReadSnapshotFile checks a snapshot file without a feed.
TEST(TestBtcPriceFeed, SnapshotCanBeCheckedBeforeAnyStateIsApplied)
{
    static unsigned short fileName[] = L"snapshotBtcPriceSeriesCheck.239";
    static unsigned short missingName[] = L"snapshotBtcPriceSeriesNone.239";
    auto feed = makeSyncedFeed();
    auto buffer = std::make_unique<BtcPriceSnapshot>();
    ASSERT_TRUE(btcFeedSaveSnapshotFile(fileName, NULL, *feed, *buffer, 239, 91700123));

    // The check reads the file only; the tick is not known yet when it runs.
    setMem(buffer.get(), sizeof(BtcPriceSnapshot), 0);
    EXPECT_TRUE(btcFeedReadSnapshotFile(fileName, NULL, *buffer, 239));
    EXPECT_EQ(buffer->image.savedAtTick, 91700123u);
    EXPECT_FALSE(btcFeedReadSnapshotFile(fileName, NULL, *buffer, 240));
    EXPECT_FALSE(btcFeedReadSnapshotFile(missingName, NULL, *buffer, 239));

    // A node that held no series wrote a snapshot nobody can use.
    feed->synced = false;
    ASSERT_TRUE(btcFeedSaveSnapshotFile(fileName, NULL, *feed, *buffer, 239, 91700123));
    EXPECT_FALSE(btcFeedReadSnapshotFile(fileName, NULL, *buffer, 239));

    // An epoch file is not a snapshot.
    feed->synced = true;
    ASSERT_TRUE(btcFeedSaveImageFile(fileName, NULL, *feed, buffer->image, BTC_PRICE_KIND_SWITCH, 239, 91700123));
    EXPECT_FALSE(btcFeedReadSnapshotFile(fileName, NULL, *buffer, 239));
}

static BtcPricePoint& pointOf(BtcPriceFeed& feed, unsigned int hour)
{
    return feed.image.points[hour & (BTC_PRICE_RING_SIZE - 1)];
}

static unsigned int stateDigestOf(BtcPriceFeed& feed)
{
    unsigned int digest = 0;
    EXPECT_TRUE(feed.stateDigest(digest));
    return digest;
}

// The state digest depends on the series only, not on node-local state.
TEST(TestBtcPriceFeed, StateDigestIsTheSameForTheSameSeries)
{
    auto a = makeSyncedFeed();
    auto b = makeSyncedFeed();
    EXPECT_EQ(stateDigestOf(*a), stateDigestOf(*b));
    EXPECT_NE(stateDigestOf(*a), 0u);

    // Node-local state is not part of it.
    b->numStartFailed = 9;
    b->lastFreeQueryTick = 5;
    EXPECT_TRUE(b->onQueryStarted(2, BASE_HOUR));
    EXPECT_EQ(stateDigestOf(*a), stateDigestOf(*b));

    // A node without a series has none.
    unsigned int digest = 77;
    a->synced = false;
    EXPECT_FALSE(a->stateDigest(digest));
    EXPECT_EQ(digest, 0u);
}

// Every price from the first frame on changes the state digest; older hours do not.
TEST(TestBtcPriceFeed, StateDigestCoversEveryPriceOfTheFirstFrame)
{
    const unsigned int base = BASE_HOUR - 1;
    const unsigned int first = base - WINDOW;
    auto reference = makeSyncedFeed();
    const unsigned int expected = stateDigestOf(*reference);

    for (const unsigned int hour : { first, first + 1, first + 255, first + 256, base - 1, base })
    {
        auto feed = makeSyncedFeed();
        pointOf(*feed, hour).priceE8 += 1;
        EXPECT_NE(stateDigestOf(*feed), expected) << "hour " << hour;
    }

    // Older hours, flags and the storing tick of hours up to the base do not matter for grading.
    auto feed = makeSyncedFeed();
    pointOf(*feed, first - 1).priceE8 += 1;
    pointOf(*feed, first - 100).priceE8 += 1;
    pointOf(*feed, base - 5).flags = BTC_PRICE_FLAG_FILLED;
    pointOf(*feed, base - 5).appendTick = 4242;
    pointOf(*feed, base).appendTick = 4243;
    EXPECT_EQ(stateDigestOf(*feed), expected);

    // A hole in the frame: no digest.
    pointOf(*feed, base - 9).priceE8 = 0;
    unsigned int digest = 77;
    EXPECT_FALSE(feed->stateDigest(digest));
    EXPECT_EQ(digest, 0u);
}

// The base hour, the attempt fields, and the price and storing tick of every hour after the base are part
// of the state digest.
TEST(TestBtcPriceFeed, StateDigestCoversBaseAttemptsAndHoursAfterTheBase)
{
    auto reference = makeSyncedFeed();
    const unsigned int atStart = stateDigestOf(*reference);

    auto feed = makeSyncedFeed();
    feed->image.failedAttempts = 1;
    EXPECT_NE(stateDigestOf(*feed), atStart);
    feed = makeSyncedFeed();
    feed->image.firstFailMs = 1;
    EXPECT_NE(stateDigestOf(*feed), atStart);
    feed = makeSyncedFeed();
    feed->image.lastFailMs = 1;
    EXPECT_NE(stateDigestOf(*feed), atStart);
    feed = makeSyncedFeed();
    feed->image.baseHour -= 1;
    EXPECT_NE(stateDigestOf(*feed), atStart);

    // An hour stored after the base: its price and the tick it was stored in count.
    appendHour(*reference, 6500000000000LL, 3000);
    const unsigned int withHour = stateDigestOf(*reference);
    EXPECT_NE(withHour, atStart);

    feed = makeSyncedFeed();
    appendHour(*feed, 6500000000000LL, 3000);
    EXPECT_EQ(stateDigestOf(*feed), withHour);
    pointOf(*feed, BASE_HOUR).appendTick = 3001;
    EXPECT_NE(stateDigestOf(*feed), withHour);
    feed = makeSyncedFeed();
    appendHour(*feed, 6500000000001LL, 3000);
    EXPECT_NE(stateDigestOf(*feed), withHour);

    // The epoch switch moves the base: the same series starts the next epoch with another digest.
    reference->onEpochSwitch();
    EXPECT_NE(stateDigestOf(*reference), withHour);
}
