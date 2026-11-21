#pragma once

// Prediction phase of a bpp9000 epoch. An identity qualifies with a solution that clears
// BPP9000_PREDICTION_THRESHOLD. Once BPP9000_PREDICTION_SWITCH_COMPUTORS computors have qualified, the hours
// a qualified identity's network answers right are counted, and with enough counted hours the identity is
// ranked by its accuracy.
// A solution executed at least BPP9000_PREDICTION_DEADLINE_SEC before an hour answers that hour and the later ones.

#include "platform/m256.h"
#include "platform/memory.h"
#include "kangaroo_twelve.h"
#include "public_settings.h"
#include "mining/mining.h"
#include "mining/rating.h"

// Hours of an epoch a network can answer: one per frame after the first.
static constexpr unsigned int PREDICTION_MAX_HOURS = (unsigned int)BPP9000_SHIFT_CAP;
static constexpr long long PREDICTION_MS_PER_HOUR = (long long)BTC_FEED_PERIOD_SEC * 1000;
static constexpr long long PREDICTION_DEADLINE_MS = (long long)BPP9000_PREDICTION_DEADLINE_SEC * 1000;
static constexpr unsigned int PREDICTION_HOURS_PER_EPOCH = 24 * 7;
// What a network without an output for the hour answers; never equal to a target.
static constexpr unsigned char PREDICTION_NO_ANSWER = 2;

// Ranking key of an identity that is not ranked by its accuracy; sorts after every key of one that is.
static constexpr unsigned int PREDICTION_NO_RANKING_KEY = 0xFFFFFFFFU;
// The ranking key holds the accuracy as correct * PREDICTION_ACCURACY_ONE / hours.
static constexpr unsigned int PREDICTION_ACCURACY_ONE = 0x10000;

static_assert(PREDICTION_MAX_HOURS <= 0xFF, "the prediction ranking key holds the counted hours in 8 bits");
static_assert(PREDICTION_MAX_HOURS * PREDICTION_MAX_HOURS < PREDICTION_ACCURACY_ONE,
    "two different accuracies must give two different ranking keys");
static_assert(BPP9000_PREDICTION_MIN_PERFECT_ATTEMPTS >= 1 && BPP9000_PREDICTION_MIN_PERFECT_ATTEMPTS <= BPP9000_PREDICTION_MIN_ATTEMPTS,
    "an identity ranked by its accuracy has at least one counted hour");
static_assert(BPP9000_PREDICTION_DEADLINE_SEC <= BTC_FEED_PERIOD_SEC, "the deadline of an hour must not lie before the hour it is compared with");
static_assert(BPP9000_PREDICTION_SWITCH_CLOSE_HOURS < PREDICTION_HOURS_PER_EPOCH, "the switch must be possible in some hour of the epoch");

struct PredictionEntry
{
    m256i publicKey;
    // Best rating of the identity and the tick it was reached in.
    score_engine::Rating best;
    unsigned int bestTick;
    // First hour that is counted for the identity.
    unsigned int firstHour;
    // The solution whose network answers the hours still open.
    unsigned int solutionTick;
    unsigned int solutionIndexInTick;
    unsigned short hours;
    unsigned short correct;
    // One answer per hour after the base hour.
    unsigned char answers[PREDICTION_MAX_HOURS];
};

struct PredictionPhase
{
    unsigned int count;
    unsigned int computorsQualified;
    // 0: training ranking.
    unsigned int switchTick;
    // First hour that is counted at all.
    unsigned int switchFirstHour;
    // First hour that is not closed yet, 0 in training.
    unsigned int firstOpenHour;
    // digest() at its last fold into the mining digest.
    unsigned int foldedDigest;
    PredictionEntry entries[MAX_NUMBER_OF_MINERS];

    void reset()
    {
        setMem(this, sizeof(*this), 0);
    }

    bool isSwitched() const
    {
        return switchTick != 0;
    }

    // False for a state no node writes. The loops over the entries rely on count, the ranking key on
    // correct <= hours <= PREDICTION_MAX_HOURS.
    bool isValid() const
    {
        if (count > MAX_NUMBER_OF_MINERS || computorsQualified > count || firstOpenHour < switchFirstHour)
        {
            return false;
        }
        for (unsigned int i = 0; i < count; i++)
        {
            if ((unsigned int)entries[i].hours > PREDICTION_MAX_HOURS || entries[i].correct > entries[i].hours)
            {
                return false;
            }
        }
        return true;
    }

    // Time the rules use for a tick: the later of the tick time and the time stamp of the tick data it executes.
    static long long laterTimeMs(long long tickTimeMs, long long tickDataMs)
    {
        if (tickDataMs > tickTimeMs)
        {
            return tickDataMs;
        }
        return tickTimeMs;
    }

    // Last instant a solution may be executed at to answer the hour.
    static long long deadlineMs(unsigned int hour)
    {
        return (long long)hour * PREDICTION_MS_PER_HOUR - PREDICTION_DEADLINE_MS;
    }

    // First hour a solution executed at nowMs answers.
    static unsigned int firstOpenHourAt(long long nowMs)
    {
        return (unsigned int)((nowMs + PREDICTION_DEADLINE_MS + PREDICTION_MS_PER_HOUR - 1) / PREDICTION_MS_PER_HOUR);
    }

    int find(const m256i& publicKey) const
    {
        for (unsigned int i = 0; i < count; i++)
        {
            if (entries[i].publicKey == publicKey)
            {
                return (int)i;
            }
        }
        return -1;
    }

    // True if an accepted solution with this rating changes the identity's state; its answers are needed then.
    bool needsAnswers(const m256i& publicKey, const score_engine::Rating& rating) const
    {
        const int index = find(publicKey);
        if (index < 0)
        {
            return count < MAX_NUMBER_OF_MINERS && rating.clearsFloor(BPP9000_PREDICTION_THRESHOLD);
        }
        return isSwitched() || rating.isBetterThan(entries[index].best);
    }

    // Records an accepted solution needsAnswers() agreed to. answers: the network's answer for each hour after baseHour.
    // Before the switch the best network of the identity answers, after it the last accepted one.
    PredictionEntry& recordSolution(const m256i& publicKey, const score_engine::Rating& rating, unsigned int tick, unsigned int solutionIndexInTick,
        long long nowMs, unsigned int baseHour, bool isComputor, const unsigned char* answers)
    {
        const unsigned int firstHour = firstOpenHourAt(nowMs);
        int index = find(publicKey);
        if (index < 0)
        {
            index = (int)count;
            count++;
            PredictionEntry& created = entries[index];
            created.publicKey = publicKey;
            created.best = rating;
            created.bestTick = tick;
            created.firstHour = firstHour;
            setMem(created.answers, sizeof(created.answers), PREDICTION_NO_ANSWER);
            if (isComputor)
            {
                computorsQualified++;
            }
        }

        PredictionEntry& entry = entries[index];
        if (rating.isBetterThan(entry.best))
        {
            entry.best = rating;
            entry.bestTick = tick;
        }
        entry.solutionTick = tick;
        entry.solutionIndexInTick = solutionIndexInTick;

        // An hour whose deadline has passed keeps the answer it has.
        unsigned int from = 0;
        if (firstHour > baseHour + 1)
        {
            from = firstHour - baseHour - 1;
        }
        if (from < PREDICTION_MAX_HOURS)
        {
            copyMem(entry.answers + from, answers + from, PREDICTION_MAX_HOURS - from);
        }
        return entry;
    }

    // Hour an epoch ends at: Wednesday 12:00 UTC, the time the epoch transition waits for.
    static unsigned int epochEndHour(unsigned int epoch)
    {
        return (epoch + 1) * PREDICTION_HOURS_PER_EPOCH;
    }

    // Switches the epoch to the prediction ranking if enough computors have qualified and more than
    // BPP9000_PREDICTION_SWITCH_CLOSE_HOURS hours are left before endHour. True if it switched now.
    // The last tick of an epoch has a tick time at or after endHour, so it never switches.
    bool trySwitch(unsigned int tick, long long nowMs, unsigned int endHour)
    {
        if (isSwitched() || computorsQualified < BPP9000_PREDICTION_SWITCH_COMPUTORS)
        {
            return false;
        }
        const unsigned long long hour = (unsigned long long)(nowMs / PREDICTION_MS_PER_HOUR);
        if (hour + BPP9000_PREDICTION_SWITCH_CLOSE_HOURS >= endHour)
        {
            return false;
        }
        switchTick = tick;
        switchFirstHour = firstOpenHourAt(nowMs);
        firstOpenHour = switchFirstHour;
        return true;
    }

    // True if the identity takes part in the hour: the hour's deadline lies after the switch and after the identity qualified.
    bool participatesIn(const PredictionEntry& entry, unsigned int hour) const
    {
        return isSwitched() && hour >= switchFirstHour && hour >= entry.firstHour;
    }

    // True once enough hours are counted for the identity: BPP9000_PREDICTION_MIN_ATTEMPTS of them, or
    // BPP9000_PREDICTION_MIN_PERFECT_ATTEMPTS with none answered wrong.
    static bool isRankedByAccuracy(const PredictionEntry& entry)
    {
        const unsigned int hours = entry.hours;
        if (hours >= BPP9000_PREDICTION_MIN_ATTEMPTS)
        {
            return true;
        }
        return hours >= BPP9000_PREDICTION_MIN_PERFECT_ATTEMPTS && entry.correct == entry.hours;
    }

    // Ranking key of an identity, lower is better: higher accuracy (correct hours / counted hours), then more
    // counted hours. PREDICTION_NO_RANKING_KEY while the identity is not ranked by its accuracy.
    static unsigned int rankingKey(const PredictionEntry& entry)
    {
        if (!isRankedByAccuracy(entry))
        {
            return PREDICTION_NO_RANKING_KEY;
        }
        const unsigned int hours = entry.hours;
        const unsigned int correct = entry.correct;
        const unsigned int accuracy = correct * PREDICTION_ACCURACY_ONE / hours;
        return ((PREDICTION_ACCURACY_ONE - accuracy) << 8) | (0xFFU - hours);
    }

    // Counts an hour whose target is known. Returns how many identities it was counted for.
    unsigned int countHour(unsigned int hour, unsigned int baseHour, unsigned char target)
    {
        if (hour <= baseHour || hour - baseHour > PREDICTION_MAX_HOURS)
        {
            return 0;
        }
        const unsigned int answerIndex = hour - baseHour - 1;
        unsigned int counted = 0;
        for (unsigned int i = 0; i < count; i++)
        {
            PredictionEntry& entry = entries[i];
            if (participatesIn(entry, hour))
            {
                entry.hours++;
                if (entry.answers[answerIndex] == target)
                {
                    entry.correct++;
                }
                counted++;
            }
        }
        return counted;
    }

    // Closes the first open hour if its deadline has passed. True if it closed one; hour is that hour.
    bool closeNextHour(long long nowMs, unsigned int& hour)
    {
        if (!isSwitched() || nowMs <= deadlineMs(firstOpenHour))
        {
            return false;
        }
        hour = firstOpenHour;
        firstOpenHour++;
        return true;
    }

    // Digest of what the ranking depends on.
    unsigned int digest() const
    {
        KangarooTwelveStream stream;
        stream.init();
        stream.update(&switchTick, sizeof(switchTick));
        stream.update(&switchFirstHour, sizeof(switchFirstHour));
        for (unsigned int i = 0; i < count; i++)
        {
            const PredictionEntry& entry = entries[i];
            stream.update(&entry.publicKey, sizeof(entry.publicKey));
            stream.update(&entry.hours, sizeof(entry.hours));
            stream.update(&entry.correct, sizeof(entry.correct));
        }
        unsigned char streamDigest[32];
        stream.finalize(streamDigest);
        unsigned int result = 0;
        copyMem(&result, streamDigest, sizeof(result));
        return result;
    }

    // Keeps digest() as the digest to fold into the mining digest now.
    unsigned int updateFoldedDigest()
    {
        foldedDigest = digest();
        return foldedDigest;
    }
};
