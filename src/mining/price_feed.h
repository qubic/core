#pragma once

// Hourly BTC/USDT price series from the Price oracle (Binance); it is the bpp9000 task data.
// The leader of every BTC_FEED_SLOT_PERIOD-th tick may send one free query for the next missing hour.
// Target t of an epoch: whole-USDT price of hour baseHour - BPP9000_WINDOW_WIDTH + 1 + t not below the hour before.
// A solution is graded on the hours appended before its anchor tick.

#include "contract_core/pre_qpi_def.h"
#include "qpi/qpi.h"
#include "oracle_core/oracle_interfaces_def.h"
#include "oracle_core/oracle_transactions.h"
#include "network_messages/common_def.h"
#include "platform/m256.h"
#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/time.h"
#include "platform/uint128.h"
#include "platform/file_io.h"
#include "kangaroo_twelve.h"
#include "public_settings.h"

static constexpr unsigned int BTC_PRICE_RING_SIZE = 16384;
static constexpr unsigned int BTC_PRICE_IMAGE_MAGIC = 0x46435442; // BTCF

// Fewest consecutive hours before nextHour in a usable series.
static constexpr unsigned int BTC_PRICE_MIN_POINTS = 24 * 365 + 24 * 7 + 1;

static constexpr unsigned int BTC_PRICE_KIND_BOOTSTRAP = 1;
static constexpr unsigned int BTC_PRICE_KIND_SWITCH = 2;
static constexpr unsigned int BTC_PRICE_KIND_F6 = 3;
static constexpr unsigned int BTC_PRICE_KIND_SNAPSHOT = 4;

static constexpr unsigned int BTC_PRICE_FLAG_FILLED = 1;

static constexpr long long BTC_FEED_MS_PER_HOUR = (long long)BTC_FEED_PERIOD_SEC * 1000;
static constexpr long long BTC_FEED_PRICE_SCALE = 100000000;

static_assert((BTC_PRICE_RING_SIZE & (BTC_PRICE_RING_SIZE - 1)) == 0, "the ring index masks with the size");
static_assert(BTC_PRICE_RING_SIZE > BTC_PRICE_MIN_POINTS, "the ring must hold a full series");
static_assert(BTC_PRICE_MIN_POINTS > BPP9000_WINDOW_WIDTH, "a series must cover the first frame of an epoch");
static_assert(BPP9000_SEQUENCE_LENGTH == BPP9000_WINDOW_WIDTH + BPP9000_SHIFT_CAP, "one target per frame after the first");
static_assert(BTC_FEED_SLOT_PERIOD >= 3, "the previous slot must have executed before the next leader decides");
static_assert(BTC_FEED_PERIOD_SEC >= 60 && BTC_FEED_PERIOD_SEC % 60 == 0, "the price oracle answers per minute");
static_assert(NUMBER_OF_COMPUTORS % BTC_FEED_SLOT_PERIOD != 0 && BTC_FEED_SLOT_PERIOD % 2 != 0 && BTC_FEED_SLOT_PERIOD % 13 != 0,
    "the slot period must share no factor with the number of computors");

struct BtcPricePoint
{
    unsigned int hour;
    unsigned int appendTick;
    unsigned int flags;
    unsigned int reserved;
    long long priceE8;
};

struct BtcPriceImage
{
    unsigned int magic;
    unsigned int headerSize;
    unsigned int recordSize;
    unsigned int kind;
    unsigned int epoch;
    unsigned int savedAtTick;
    unsigned char checksum[32];

    // From here on: covered by the series digest.
    unsigned int nextHour;
    unsigned int failedAttempts;
    long long firstFailMs;
    long long lastFailMs;
    // Last hour known when the epoch began; its first frame ends there.
    unsigned int baseHour;
    unsigned int reserved2;
    BtcPricePoint points[BTC_PRICE_RING_SIZE];
};

static constexpr unsigned int BTC_PRICE_IMAGE_HEADER_SIZE = 88;

static_assert(sizeof(BtcPricePoint) == 24, "point record must be 24 bytes without padding");
static_assert(offsetof(BtcPriceImage, checksum) == 24, "unexpected image layout");
static_assert(offsetof(BtcPriceImage, nextHour) == 56, "unexpected image layout");
static_assert(offsetof(BtcPriceImage, baseHour) == 80, "unexpected image layout");
static_assert(offsetof(BtcPriceImage, points) == BTC_PRICE_IMAGE_HEADER_SIZE, "unexpected image layout");
static_assert(sizeof(BtcPriceImage) == BTC_PRICE_IMAGE_HEADER_SIZE + BTC_PRICE_RING_SIZE * sizeof(BtcPricePoint), "unexpected image size");

// Snapshot file: the image plus the node-local binding.
struct BtcPriceSnapshot
{
    BtcPriceImage image;
    long long boundQueryId;
    unsigned int synced;
    unsigned int reserved;
};

static_assert(sizeof(BtcPriceSnapshot) == sizeof(BtcPriceImage) + 16, "unexpected snapshot size");

// The state digest hashes this header, then one record per hour.
struct BtcPriceStateHeader
{
    unsigned int baseHour;
    unsigned int nextHour;
    unsigned int failedAttempts;
    unsigned int reserved;
    long long firstFailMs;
    long long lastFailMs;
};

struct BtcPriceStateRecord
{
    long long priceE8;
    unsigned int hour;
    unsigned int appendTick;
};

static_assert(sizeof(BtcPriceStateHeader) == 32, "state header must be 32 bytes without padding");
static_assert(sizeof(BtcPriceStateRecord) == 16, "state record must be 16 bytes without padding");

// Milliseconds since the start of hour 0, 2022-04-13 12:00 UTC. False for an invalid date or a year outside 2022..2099.
static bool btcFeedTimeToMs(const QPI::DateAndTime& t, long long& milliseconds)
{
    if (!t.isValid())
    {
        return false;
    }
    const unsigned int year = t.getYear();
    if (year < 2022 || year > 2099)
    {
        return false;
    }
    milliseconds = ms((unsigned char)(year - 2000), (unsigned char)t.getMonth(), (unsigned char)t.getDay(),
        (unsigned char)t.getHour(), (unsigned char)t.getMinute(), (unsigned char)t.getSecond(), (unsigned short)t.getMillisec())
        - ms(22, 4, 13, 12, 0, 0, 0);
    return milliseconds >= 0;
}

static bool btcFeedHourTime(unsigned int hour, QPI::DateAndTime& t)
{
    t.set(2022, 4, 13, 12, 0, 0);
    return t.add(0, 0, 0, 0, 0, (QPI::sint64)hour * BTC_FEED_PERIOD_SEC);
}

// The free query: Binance BTC/USDT at the start of the hour.
static bool btcFeedMakeQuery(unsigned int hour, OI::Price::OracleQuery& query)
{
    using namespace QPI::Ch;
    setMem(&query, sizeof(query), 0);
    query.oracle = OI::Price::getBinanceOracleId();
    query.currency1 = QPI::id(B, T, C, null, null);
    query.currency2 = QPI::id(U, S, D, T, null);
    return btcFeedHourTime(hour, query.timestamp);
}

// Whole USDT, rounded half up; the task targets compare these.
static long long btcFeedPriceUsdt(long long priceE8)
{
    return (priceE8 + BTC_FEED_PRICE_SCALE / 2) / BTC_FEED_PRICE_SCALE;
}

// 1e-8 units, rounded down. False for a non-positive input, a zero result or an overflow.
static bool btcFeedPriceE8(long long numerator, long long denominator, long long& priceE8)
{
    if (numerator <= 0 || denominator <= 0)
    {
        return false;
    }
    const uint128_t scaled = uint128_t((unsigned long long)numerator) * uint128_t((unsigned long long)BTC_FEED_PRICE_SCALE);
    const uint128_t quotient = scaled / uint128_t((unsigned long long)denominator);
    if (quotient.high != 0 || quotient.low == 0 || quotient.low > 0x7FFFFFFFFFFFFFFFULL)
    {
        return false;
    }
    priceE8 = (long long)quotient.low;
    return true;
}

// Consensus rule: the free feed query of this tick's leader. Reads only its arguments.
static bool isFreeBtcFeedQuery(const Transaction* transaction, unsigned int tick, const m256i& leaderKey, const QPI::DateAndTime& now, unsigned int& hour)
{
    if (transaction->inputType != OracleUserQueryTransactionPrefix::transactionType()
        || transaction->inputSize != OracleUserQueryTransactionPrefix::minInputSize() + sizeof(OI::Price::OracleQuery))
    {
        return false;
    }
    const OracleUserQueryTransactionPrefix* queryTx = (const OracleUserQueryTransactionPrefix*)transaction;
    if (queryTx->oracleInterfaceIndex != OI::Price::oracleInterfaceIndex)
    {
        return false;
    }
    if (tick % BTC_FEED_SLOT_PERIOD != 0 || transaction->tick != tick)
    {
        return false;
    }
    if (!(transaction->sourcePublicKey == leaderKey) || !isZero(transaction->destinationPublicKey))
    {
        return false;
    }
    if (transaction->amount != 0 || queryTx->timeoutMilliseconds != BTC_FEED_QUERY_TIMEOUT_MS)
    {
        return false;
    }

    OI::Price::OracleQuery query;
    copyMem(&query, ((const unsigned char*)transaction) + sizeof(OracleUserQueryTransactionPrefix), sizeof(query));
    // Only the canonical query of the timestamp's hour passes the comparison below.
    long long queryMs = 0;
    if (!btcFeedTimeToMs(query.timestamp, queryMs))
    {
        return false;
    }
    const unsigned int queriedHour = (unsigned int)(queryMs / BTC_FEED_MS_PER_HOUR);
    if (queriedHour < BTC_FEED_MIN_HOUR)
    {
        return false;
    }
    OI::Price::OracleQuery expected;
    if (!btcFeedMakeQuery(queriedHour, expected) || compareMem(&query, &expected, sizeof(query)) != 0)
    {
        return false;
    }

    long long nowMs = 0;
    if (!btcFeedTimeToMs(now, nowMs))
    {
        return false;
    }
    if (nowMs < (long long)queriedHour * BTC_FEED_MS_PER_HOUR + (long long)BTC_FEED_QUERY_DELAY_SEC * 1000)
    {
        return false;
    }
    hour = queriedHour;
    return true;
}

static void btcFeedSetImageHeader(BtcPriceImage& image, unsigned int kind, unsigned int epoch, unsigned int tick)
{
    image.magic = BTC_PRICE_IMAGE_MAGIC;
    image.headerSize = BTC_PRICE_IMAGE_HEADER_SIZE;
    image.recordSize = sizeof(BtcPricePoint);
    image.kind = kind;
    image.epoch = epoch;
    image.savedAtTick = tick;
    setMem(image.checksum, sizeof(image.checksum), 0);
    unsigned char digest[32];
    KangarooTwelve(&image, sizeof(image), digest, sizeof(digest));
    copyMem(image.checksum, digest, sizeof(image.checksum));
}

// Format, checksum, and a full series before nextHour.
static bool btcFeedImageIsValid(BtcPriceImage& image)
{
    if (image.magic != BTC_PRICE_IMAGE_MAGIC || image.headerSize != BTC_PRICE_IMAGE_HEADER_SIZE
        || image.recordSize != sizeof(BtcPricePoint))
    {
        return false;
    }

    unsigned char stored[32];
    copyMem(stored, image.checksum, sizeof(stored));
    setMem(image.checksum, sizeof(image.checksum), 0);
    unsigned char computed[32];
    KangarooTwelve(&image, sizeof(image), computed, sizeof(computed));
    copyMem(image.checksum, stored, sizeof(image.checksum));
    if (compareMem(stored, computed, sizeof(stored)) != 0)
    {
        return false;
    }

    if (image.nextHour < BTC_PRICE_MIN_POINTS || image.baseHour >= image.nextHour || image.baseHour < BPP9000_WINDOW_WIDTH)
    {
        return false;
    }
    // Every hour from the first frame on must be present.
    unsigned int numPoints = image.nextHour - image.baseHour + (unsigned int)BPP9000_WINDOW_WIDTH;
    if (numPoints > BTC_PRICE_RING_SIZE)
    {
        return false;
    }
    if (numPoints < BTC_PRICE_MIN_POINTS)
    {
        numPoints = BTC_PRICE_MIN_POINTS;
    }
    for (unsigned int i = 1; i <= numPoints; ++i)
    {
        const unsigned int hour = image.nextHour - i;
        const BtcPricePoint& point = image.points[hour & (BTC_PRICE_RING_SIZE - 1)];
        if (point.hour != hour || point.priceE8 <= 0)
        {
            return false;
        }
    }
    return true;
}

// A new epoch starts from the switch file or the bootstrap, a mid-epoch restart from the F6 file.
static bool btcFeedImageIsAcceptable(const BtcPriceImage& image, unsigned int epoch, unsigned int initialTick, bool tickIsFirstTickOfEpoch)
{
    if (image.epoch != epoch)
    {
        return false;
    }
    // A new epoch starts with no hour after its base.
    if (tickIsFirstTickOfEpoch && image.baseHour + 1 != image.nextHour)
    {
        return false;
    }
    if (image.kind == BTC_PRICE_KIND_BOOTSTRAP)
    {
        return tickIsFirstTickOfEpoch;
    }
    if (image.savedAtTick > initialTick)
    {
        return false;
    }
    if (tickIsFirstTickOfEpoch)
    {
        return image.kind == BTC_PRICE_KIND_SWITCH;
    }
    return image.kind == BTC_PRICE_KIND_F6;
}

struct BtcPriceFeed
{
    BtcPriceImage image;
    unsigned char seriesDigest[32];

    // Node-local. Not synced: no usable series, no part in the capture.
    long long boundQueryId;
    volatile char lock;
    bool synced;
    unsigned int lastFreeQueryTick;
    unsigned long long numSuccess;
    unsigned long long numTimeout;
    unsigned long long numUnresolvable;
    unsigned long long numInvalidReply;
    unsigned long long numStartFailed;
    unsigned long long numFilled;

    void reset()
    {
        setMem(this, sizeof(*this), 0);
        boundQueryId = -1;
    }

    bool isBound() const
    {
        return boundQueryId >= 0;
    }

    void recomputeDigest()
    {
        KangarooTwelve(&image.nextHour, (unsigned int)(sizeof(image) - offsetof(BtcPriceImage, nextHour)), seriesDigest, sizeof(seriesDigest));
    }

    const BtcPricePoint* findPoint(unsigned int hour) const
    {
        const BtcPricePoint* point = &image.points[hour & (BTC_PRICE_RING_SIZE - 1)];
        if (point->hour != hour || point->priceE8 <= 0)
        {
            return nullptr;
        }
        return point;
    }

    // Consensus: one free query per tick.
    bool consumeFreeQuerySlot(unsigned int tick)
    {
        if (lastFreeQueryTick == tick)
        {
            return false;
        }
        lastFreeQueryTick = tick;
        return true;
    }

    // A started free query becomes the attempt for nextHour if none is running.
    bool onQueryStarted(long long queryId, unsigned int hour)
    {
        if (!synced || isBound() || hour != image.nextHour)
        {
            return false;
        }
        boundQueryId = queryId;
        return true;
    }

    void onQueryStartFailed()
    {
        numStartFailed++;
    }

    void appendPoint(long long priceE8, unsigned int flags, unsigned int tick)
    {
        ACQUIRE(lock);
        const unsigned int hour = image.nextHour;
        BtcPricePoint& point = image.points[hour & (BTC_PRICE_RING_SIZE - 1)];
        point.hour = hour;
        point.appendTick = tick;
        point.flags = flags;
        point.reserved = 0;
        point.priceE8 = priceE8;
        image.nextHour = hour + 1;
        image.failedAttempts = 0;
        image.firstFailMs = 0;
        image.lastFailMs = 0;
        recomputeDigest();
        RELEASE(lock);
    }

    // True if queryId is the bound attempt.
    bool onQueryFinished(long long queryId, unsigned char status, bool replyOk, long long numerator, long long denominator,
        const QPI::DateAndTime& now, unsigned int tick)
    {
        if (!isBound() || queryId != boundQueryId)
        {
            return false;
        }
        boundQueryId = -1;
        if (!synced)
        {
            return true;
        }

        long long priceE8 = 0;
        if (status == ORACLE_QUERY_STATUS_SUCCESS && replyOk && btcFeedPriceE8(numerator, denominator, priceE8))
        {
            numSuccess++;
            appendPoint(priceE8, 0, tick);
            return true;
        }

        if (status == ORACLE_QUERY_STATUS_TIMEOUT)
        {
            numTimeout++;
        }
        else if (status == ORACLE_QUERY_STATUS_UNRESOLVABLE)
        {
            numUnresolvable++;
        }
        else
        {
            numInvalidReply++;
        }

        long long nowMs = 0;
        if (!btcFeedTimeToMs(now, nowMs))
        {
            return true;
        }
        ACQUIRE(lock);
        if (image.failedAttempts == 0)
        {
            image.firstFailMs = nowMs;
        }
        image.failedAttempts++;
        image.lastFailMs = nowMs;
        const bool giveUp = nowMs >= image.firstFailMs + (long long)BTC_FEED_GIVE_UP_SEC * 1000;
        if (!giveUp)
        {
            recomputeDigest();
        }
        RELEASE(lock);

        if (giveUp)
        {
            const BtcPricePoint* previous = findPoint(image.nextHour - 1);
            if (previous)
            {
                numFilled++;
                appendPoint(previous->priceE8, BTC_PRICE_FLAG_FILLED, tick);
            }
            else
            {
                synced = false;
            }
        }
        return true;
    }

    // Once per tick: a bound attempt must still be running.
    void checkBoundQuery(unsigned char status)
    {
        if (isBound() && status != ORACLE_QUERY_STATUS_PENDING && status != ORACLE_QUERY_STATUS_COMMITTED)
        {
            boundQueryId = -1;
            synced = false;
        }
    }

    // The epoch switch drops the running query without a failed attempt and moves the base to the last known hour.
    void onEpochSwitch()
    {
        boundQueryId = -1;
        if (!synced)
        {
            return;
        }
        ACQUIRE(lock);
        image.baseHour = image.nextHour - 1;
        recomputeDigest();
        RELEASE(lock);
    }

    // Caller holds the lock. Hours after baseHour appended strictly before anchorTick, at most BPP9000_SHIFT_CAP.
    unsigned int hoursBeforeLocked(unsigned int anchorTick) const
    {
        unsigned int count = 0;
        unsigned int hour = image.nextHour;
        while (hour > image.baseHour + 1)
        {
            hour--;
            if (image.points[hour & (BTC_PRICE_RING_SIZE - 1)].appendTick < anchorTick)
            {
                count = hour - image.baseHour;
                break;
            }
        }
        return (count < (unsigned int)BPP9000_SHIFT_CAP) ? count : (unsigned int)BPP9000_SHIFT_CAP;
    }

    // Caller holds the lock. One byte per hour, 1 = price not below the hour before. Returns the count.
    unsigned int buildTaskTargetsLocked(unsigned char* targets, unsigned int capacity) const
    {
        unsigned int count = 0;
        if (synced && image.baseHour >= BPP9000_WINDOW_WIDTH && image.baseHour < image.nextHour)
        {
            unsigned int hour = image.baseHour - (unsigned int)BPP9000_WINDOW_WIDTH;
            const BtcPricePoint* point = findPoint(hour);
            while (point && count < capacity && hour + 1 < image.nextHour)
            {
                const long long previousUsdt = btcFeedPriceUsdt(point->priceE8);
                hour++;
                point = findPoint(hour);
                if (point)
                {
                    targets[count] = (btcFeedPriceUsdt(point->priceE8) >= previousUsdt) ? 1 : 0;
                    count++;
                }
            }
        }
        return count;
    }

    // Task targets of the epoch. Returns the count, 0 without a series.
    unsigned int buildTaskTargets(unsigned char* targets, unsigned int capacity)
    {
        ACQUIRE(lock);
        const unsigned int count = buildTaskTargetsLocked(targets, capacity);
        RELEASE(lock);
        return count;
    }

    // Digest of what grading depends on: base and next hour, attempt state, prices from the first frame on,
    // append ticks of the hours after the base. False without a series.
    bool stateDigest(unsigned int& digest)
    {
        digest = 0;
        bool complete = false;
        ACQUIRE(lock);
        if (synced && image.baseHour >= BPP9000_WINDOW_WIDTH && image.baseHour < image.nextHour)
        {
            BtcPriceStateHeader header;
            header.baseHour = image.baseHour;
            header.nextHour = image.nextHour;
            header.failedAttempts = image.failedAttempts;
            header.reserved = 0;
            header.firstFailMs = image.firstFailMs;
            header.lastFailMs = image.lastFailMs;
            KangarooTwelveStream stream;
            stream.init();
            stream.update(&header, sizeof(header));

            complete = true;
            for (unsigned int hour = image.baseHour - (unsigned int)BPP9000_WINDOW_WIDTH; hour < image.nextHour; hour++)
            {
                const BtcPricePoint* point = findPoint(hour);
                if (!point)
                {
                    complete = false;
                    break;
                }
                BtcPriceStateRecord record;
                record.priceE8 = point->priceE8;
                record.hour = hour;
                record.appendTick = (hour > image.baseHour) ? point->appendTick : 0;
                stream.update(&record, sizeof(record));
            }
            if (complete)
            {
                unsigned char streamDigest[32];
                stream.finalize(streamDigest);
                copyMem(&digest, streamDigest, sizeof(digest));
            }
        }
        RELEASE(lock);
        return complete;
    }

    // Shift limit and targets of a solution anchored at anchorTick, in one step.
    // Returns the target count (BPP9000_WINDOW_WIDTH + shiftLimit), 0 without a series.
    unsigned int taskDataAt(unsigned int anchorTick, unsigned char* targets, unsigned int capacity, unsigned int& shiftLimit, unsigned int& baseHour)
    {
        unsigned int count = 0;
        shiftLimit = 0;
        baseHour = 0;
        ACQUIRE(lock);
        if (synced)
        {
            const unsigned int limit = hoursBeforeLocked(anchorTick);
            const unsigned int needed = (unsigned int)BPP9000_WINDOW_WIDTH + limit;
            if (needed <= capacity && buildTaskTargetsLocked(targets, needed) == needed)
            {
                count = needed;
                shiftLimit = limit;
                baseHour = image.baseHour;
            }
        }
        RELEASE(lock);
        return count;
    }

    // True if the leader of targetTick sends a query now; hour is the hour to ask for.
    bool shouldSend(unsigned int targetTick, const QPI::DateAndTime& now, unsigned int& hour) const
    {
        if (!synced || isBound() || targetTick % BTC_FEED_SLOT_PERIOD != 0 || image.nextHour < BTC_FEED_MIN_HOUR)
        {
            return false;
        }
        long long nowMs = 0;
        if (!btcFeedTimeToMs(now, nowMs))
        {
            return false;
        }
        if (nowMs < (long long)image.nextHour * BTC_FEED_MS_PER_HOUR + (long long)BTC_FEED_QUERY_DELAY_SEC * 1000)
        {
            return false;
        }
        if (image.failedAttempts != 0 && nowMs < image.lastFailMs + (long long)BTC_FEED_RETRY_BACKOFF_SEC * 1000)
        {
            return false;
        }
        hour = image.nextHour;
        return true;
    }

    // Copy of the series for a file, with header and checksum set.
    void exportImage(BtcPriceImage& out, unsigned int kind, unsigned int epoch, unsigned int tick)
    {
        ACQUIRE(lock);
        copyMem(&out, &image, sizeof(out));
        RELEASE(lock);
        btcFeedSetImageHeader(out, kind, epoch, tick);
    }

    // Takes a validated image as the series; nothing is bound afterwards.
    void adoptImage(const BtcPriceImage& in)
    {
        ACQUIRE(lock);
        copyMem(&image, &in, sizeof(image));
        recomputeDigest();
        boundQueryId = -1;
        synced = true;
        RELEASE(lock);
    }
};

static BtcPriceFeed gBtcPriceFeed;

// File IO scratch, main thread only. During a snapshot load it holds the snapshot's series until it is taken over.
static BtcPriceSnapshot gBtcPriceFeedIoBuffer;

static bool btcFeedLoadImageFile(const CHAR16* fileName, const CHAR16* directory, BtcPriceImage& buffer)
{
    const long long loadedSize = load(fileName, sizeof(buffer), (unsigned char*)&buffer, directory);
    if (loadedSize != sizeof(buffer))
    {
        return false;
    }
    return btcFeedImageIsValid(buffer);
}

static bool btcFeedSaveImageFile(const CHAR16* fileName, const CHAR16* directory, BtcPriceFeed& feed, BtcPriceImage& buffer,
    unsigned int kind, unsigned int epoch, unsigned int tick)
{
    feed.exportImage(buffer, kind, epoch, tick);
    const long long savedSize = save(fileName, sizeof(buffer), (const unsigned char*)&buffer, directory);
    return savedSize == sizeof(buffer);
}

// The snapshot keeps the binding, so it is only good for the tick it was taken at.
static bool btcFeedSaveSnapshotFile(const CHAR16* fileName, const CHAR16* directory, BtcPriceFeed& feed, BtcPriceSnapshot& buffer,
    unsigned int epoch, unsigned int tick)
{
    feed.exportImage(buffer.image, BTC_PRICE_KIND_SNAPSHOT, epoch, tick);
    buffer.boundQueryId = feed.boundQueryId;
    buffer.synced = feed.synced ? 1 : 0;
    buffer.reserved = 0;
    const long long savedSize = save(fileName, sizeof(buffer), (const unsigned char*)&buffer, directory);
    return savedSize == sizeof(buffer);
}

// True if the file holds a series of this epoch saved by a synced node.
static bool btcFeedReadSnapshotFile(const CHAR16* fileName, const CHAR16* directory, BtcPriceSnapshot& buffer, unsigned int epoch)
{
    const long long loadedSize = load(fileName, sizeof(buffer), (unsigned char*)&buffer, directory);
    if (loadedSize != sizeof(buffer) || !btcFeedImageIsValid(buffer.image))
    {
        return false;
    }
    return buffer.image.kind == BTC_PRICE_KIND_SNAPSHOT && buffer.image.epoch == epoch && buffer.synced != 0;
}

// Takes over a checked snapshot. False if it was not taken at this tick.
static bool btcFeedAdoptSnapshot(BtcPriceFeed& feed, const BtcPriceSnapshot& buffer, unsigned int tick)
{
    feed.reset();
    if (buffer.image.savedAtTick != tick)
    {
        return false;
    }
    feed.adoptImage(buffer.image);
    feed.boundQueryId = buffer.boundQueryId;
    return true;
}
