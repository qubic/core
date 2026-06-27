#pragma once

#define NO_UEFI

#include "gtest/gtest.h"

#ifndef contractCount
#define contractCount 1024
#endif

#include "../src/revenue.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <memory>

#ifndef CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
#endif

unsigned int random(const unsigned int range)
{
    unsigned int value;
    _rdrand32_step(&value);
    return value % range;
}

TEST(TestCoreRevenue, GetQuorumScore)
{
    unsigned long long data[NUMBER_OF_COMPUTORS];

    // Zeros data
    setMem(data, sizeof(data), 0);
    unsigned long long quorumScore = getQuorumScore(data);
    EXPECT_EQ(quorumScore, 1);

    // Constant data
    const unsigned long long CONSTANT_VALUE = random(0xFFFFFFFF);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        data[i] = CONSTANT_VALUE;
    }
    quorumScore = getQuorumScore(data);
    EXPECT_EQ(quorumScore, CONSTANT_VALUE);

    // Generate sort
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        data[i] = random(0xFFFFFFFF);
    }
    quorumScore = getQuorumScore(data);
    std::sort(data, data + NUMBER_OF_COMPUTORS, std::greater<unsigned long long>());
    EXPECT_EQ(quorumScore, data[QUORUM - 1]);
}

TEST(TestCoreRevenue, ComputeRevFactor)
{
    const unsigned long long scaleFactor = 1024;
    unsigned long long data[NUMBER_OF_COMPUTORS];
    unsigned long long dataFactor[NUMBER_OF_COMPUTORS];

    // All zeros. No reveue for alls
    setMem(data, sizeof(data), 0);
    computeRevFactor(data, scaleFactor, dataFactor);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(dataFactor[i], 0);
    }

    // Constant values. Max revenue for all
    const unsigned long long CONSTANT_VALUE = random(0xFFFFFFFF);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        data[i] = CONSTANT_VALUE;
    }
    computeRevFactor(data, scaleFactor, dataFactor);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(dataFactor[i], scaleFactor);
    }

    // General case
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        data[i] = random(0xFFFFFFFF);
    }
    computeRevFactor(data, scaleFactor, dataFactor);
    // Data in range [0, scaleFactor] and quorum get max scale factor
    unsigned long long quorumValue = getQuorumScore(data);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_LE(dataFactor[i], scaleFactor);
        EXPECT_GE(dataFactor[i], 0);
        if (data[i] >= quorumValue)
        {
            EXPECT_EQ(dataFactor[i], scaleFactor);
        }
        else
        {
            EXPECT_LT(dataFactor[i], scaleFactor);
        }
    }

    // One zeros case
    unsigned int zeroPositions = random(NUMBER_OF_COMPUTORS);
    data[zeroPositions] = 0;
    computeRevFactor(data, scaleFactor, dataFactor);
    EXPECT_EQ(dataFactor[zeroPositions], 0);

    // Very small data
    unsigned int smallPosition = random(NUMBER_OF_COMPUTORS);
    data[smallPosition] = 1;
    computeRevFactor(data, scaleFactor, dataFactor);
    EXPECT_EQ(dataFactor[smallPosition], 0);
}

// V2 overflow and extreme value tests.
// Verify that no intermediate computation overflows u64 under worst-case inputs.
TEST(TestCoreRevenue, V2OverflowExtremeValues)
{
    constexpr unsigned long long S = REVENUE_SCALE;
    constexpr unsigned long long B = REVENUE_BONUS_CAP;
    constexpr unsigned long long ipc = (unsigned long long)REVENUE_IPC;
    constexpr unsigned long long u64Max = 0xFFFFFFFFFFFFFFFFULL;

    // Formula intermediate: IPC * M * (S^2 + B*E)
    //    Max case: M=S=1024, E=S=1024
    //    numerator = 1024 * (1048576 + 262144) = 1,342,177,280
    //    product = 1,479,289,940 * 1,342,177,280 ~ 1.985e18 (headroom ~9.3x)
    {
        unsigned long long maxNumerator = S * (S * S + B * S);
        EXPECT_EQ(maxNumerator, 1342177280ULL);
        EXPECT_LE(maxNumerator, u64Max / ipc);

        unsigned long long maxProduct = ipc * maxNumerator;
        EXPECT_LE(maxProduct, u64Max);

        // Full factors -> revenue == IPC
        unsigned long long result = maxProduct / REVENUE_DIVISOR;
        EXPECT_EQ(result, ipc);

        // Headroom at least 9x
        EXPECT_GE(u64Max / maxProduct, 9ULL);
    }

    // Sliding window per-tick score: logScore * S * WINDOW_SIZE
    //    Max logScore = 34071 (gTxRevenuePoints[4096])
    //    Per-tick max = 34071 * 1024 * 1351 = 47,134,639,104 (needs u64)
    {
        unsigned long long perTickMax = (unsigned long long)gTxRevenuePoints[MAX_TX_LUT_INDEX] * S * REVENUE_WINDOW_SIZE;
        EXPECT_EQ(perTickMax, 47134639104ULL);
        EXPECT_LE(perTickMax, u64Max);
    }

    // Sliding window accumulated per computor across full epoch
    //    Each computor gets MAX_NUMBER_OF_TICKS_PER_EPOCH / 676 ticks
    //    Max accumulated = perTickMax * ticksPerComputor ~ 2.51e13
    {
        unsigned long long perTickMax = (unsigned long long)gTxRevenuePoints[MAX_TX_LUT_INDEX] * S * REVENUE_WINDOW_SIZE;
        unsigned long long ticksPerComputor = MAX_NUMBER_OF_TICKS_PER_EPOCH / NUMBER_OF_COMPUTORS;
        unsigned long long maxAccum = perTickMax * ticksPerComputor;
        EXPECT_LE(maxAccum, u64Max);
        // Headroom should be very large (>100000x)
        EXPECT_GT(u64Max / maxAccum, 100000ULL);
    }

    // computeRevFactor overflow: score[i] * scalingThreshold
    //    Max mining score per computor: 1023 shares/phase * 675 reporters * ~1278 phases ~ 882M
    //    score * S = 882M * 1024 ~ 9.04e11 -> must fit u64
    {
        unsigned long long maxSharesPerPhase = (1ULL << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) - 1;
        unsigned long long phaseCycles = MAX_NUMBER_OF_TICKS_PER_EPOCH / (2 * NUMBER_OF_COMPUTORS);
        unsigned long long maxReporters = NUMBER_OF_COMPUTORS - 1;
        unsigned long long maxMiningScore = maxSharesPerPhase * maxReporters * phaseCycles;
        unsigned long long intermediate = maxMiningScore * S;
        EXPECT_LE(intermediate, u64Max);
    }

    // End-to-end: run computeRevenueV2 with max TX (NUMBER_OF_TRANSACTIONS_PER_TICK/tick), max mining scores,
    //    full epoch worth of ticks. Must not produce negative or >IPC revenue.
    {
        constexpr unsigned int TOTAL_TICKS = REVENUE_WINDOW_SIZE + NUMBER_OF_COMPUTORS * 10;
        // EpochRevenueData is ~17 MB; allocate on the heap to avoid stack overflow.
        auto dataPtr = std::make_unique<EpochRevenueData>();
        EpochRevenueData& data = *dataPtr;
        setMem(&data, sizeof(data), 0);
        data.initialTick = 0;
        data.totalTicks = TOTAL_TICKS;
        for (unsigned int t = 0; t < TOTAL_TICKS; t++)
        {
            data.perTickTxCount[t] = NUMBER_OF_TRANSACTIONS_PER_TICK;
        }
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            data.dogeMiningScore[i] = 1000000;
        }
        computeRevenueV2(data);

        long long totalRevenue = 0;
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_GE(data.v2Revenue[i], 0);
            EXPECT_LE(data.v2Revenue[i], REVENUE_IPC);
            totalRevenue += data.v2Revenue[i];
        }
        EXPECT_LE(totalRevenue, ISSUANCE_RATE);
        EXPECT_GT(totalRevenue, 0LL);
    }

    // Zero everything: totalTicks=0, all scores zero -> no crash, zero revenue
    {
        // EpochRevenueData is ~17 MB; allocate on the heap to avoid stack overflow.
        auto dataPtr = std::make_unique<EpochRevenueData>();
        EpochRevenueData& data = *dataPtr;
        setMem(&data, sizeof(data), 0);
        computeRevenueV2(data);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_EQ(data.v2Revenue[i], 0);
        }
    }
}


// Helper: prepare gMultiDimRevenue + gEpochRevenueData inputs, then call computeMultiDimRevenue().
// Each call zeroes both globals first so tests don't leak state between scenarios.
static void runMultiDimRevenue(
    const unsigned long long* txScores,
    const unsigned long long* oracleScores,
    const unsigned long long* dogeScores)
{
    setMem(&gMultiDimRevenue, sizeof(gMultiDimRevenue), 0);
    setMem(&gEpochRevenueData, sizeof(gEpochRevenueData), 0);
    copyMem(gMultiDimRevenue.txScore, txScores, sizeof(gMultiDimRevenue.txScore));
    copyMem(gEpochRevenueData.oracleScore, oracleScores, sizeof(gEpochRevenueData.oracleScore));
    copyMem(gEpochRevenueData.dogeMiningScore, dogeScores, sizeof(gEpochRevenueData.dogeMiningScore));
    computeMultiDimRevenue();
}

// Multi-dim overflow and extreme value tests at worst-case inputs.
TEST(TestCoreRevenue, MultiDimOverflowExtremeValues)
{
    constexpr unsigned long long S = REVENUE_SCALE;
    constexpr unsigned long long ipc = (unsigned long long)REVENUE_IPC;
    constexpr unsigned long long u64Max = 0xFFFFFFFFFFFFFFFFULL;

    // L2 sumsq: per-dim cap squared, summed across REVENUE_TX_DIM, must fit u64.
    {
        constexpr unsigned long long maxWs = (unsigned long long)NUMBER_OF_TRANSACTIONS_PER_TICK * REVENUE_WINDOW_SIZE;
        EXPECT_EQ(maxWs, 5533696ULL);
        EXPECT_LE(maxWs, u64Max / maxWs);
        EXPECT_LE(maxWs * maxWs, u64Max / REVENUE_TX_DIM);
    }

    // Accumulated txScore per leader: ticks_per_leader * SCALE * SCALE must fit u64 (computeRevFactor headroom).
    {
        constexpr unsigned long long maxAccumTxScore = (unsigned long long)(MAX_NUMBER_OF_TICKS_PER_EPOCH / NUMBER_OF_COMPUTORS) * S;
        EXPECT_LE(maxAccumTxScore, u64Max / S);
    }

    // DOGE softening: ipow(S, K-1) doesn't saturate and irootK64<K> of SCALE^K equals SCALE.
    {
        const unsigned long long sPowKm1 = math_lib::ipow(S, REVENUE_DOGE_K - 1);
        EXPECT_LT(sPowKm1, u64Max);
        EXPECT_LE(S, u64Max / sPowKm1);
        EXPECT_EQ(math_lib::irootK64<REVENUE_DOGE_K>(S * sPowKm1), S);
    }

    // Final numerator IPC * SCALE^3: bound-check each multiply, then verify round-trip and headroom.
    {
        EXPECT_LE(ipc, u64Max / S / S / S);
        constexpr unsigned long long maxNum = ipc * S * S * S;
        EXPECT_EQ(maxNum / (S * S * S), ipc);
        EXPECT_GE(u64Max / maxNum, 11ULL);
    }

    // End-to-end at maximum scores: revenue stays in [0, IPC] and total <= ISSUANCE_RATE.
    {
        unsigned long long maxScores[NUMBER_OF_COMPUTORS];
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            maxScores[i] = (unsigned long long)(MAX_NUMBER_OF_TICKS_PER_EPOCH / NUMBER_OF_COMPUTORS) * S;
        }
        runMultiDimRevenue(maxScores, maxScores, maxScores);

        long long totalRevenue = 0;
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_GE(gMultiDimRevenue.revenue[i], 0);
            EXPECT_LE(gMultiDimRevenue.revenue[i], REVENUE_IPC);
            totalRevenue += gMultiDimRevenue.revenue[i];
        }
        EXPECT_LE(totalRevenue, ISSUANCE_RATE);
        EXPECT_GT(totalRevenue, 0LL);
    }
}

// Revenue is in [0, REVENUE_IPC] for every operator under any valid inputs.
TEST(TestCoreRevenue, MultiDimRevenueWithinBounds)
{
    unsigned long long tx[NUMBER_OF_COMPUTORS];
    unsigned long long oracle[NUMBER_OF_COMPUTORS];
    unsigned long long doge[NUMBER_OF_COMPUTORS];
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        tx[i] = (unsigned long long)random(1 << 30) + 1;
        oracle[i] = (unsigned long long)random(1 << 20) + 1;
        doge[i] = (unsigned long long)random(1 << 16) + 1;
    }
    runMultiDimRevenue(tx, oracle, doge);

    long long totalRevenue = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_GE(gMultiDimRevenue.revenue[i], 0);
        EXPECT_LE(gMultiDimRevenue.revenue[i], REVENUE_IPC);
        totalRevenue += gMultiDimRevenue.revenue[i];
    }
    EXPECT_LE(totalRevenue, ISSUANCE_RATE);
    EXPECT_GT(totalRevenue, 0LL);
}

// All-zero scores produce zero revenue and don't crash.
TEST(TestCoreRevenue, MultiDimRevenueAllZeroScores)
{
    unsigned long long zeros[NUMBER_OF_COMPUTORS];
    setMem(zeros, sizeof(zeros), 0);
    runMultiDimRevenue(zeros, zeros, zeros);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(gMultiDimRevenue.revenue[i], 0);
    }
}

// Holding txScore and oracleScore equal across operators, increasing one operator's
// dogeMiningScore strictly increases their revenue (until cap).
TEST(TestCoreRevenue, MultiDimMonotonicityInDoge)
{
    constexpr unsigned int TARGET = 100;
    unsigned long long tx[NUMBER_OF_COMPUTORS];
    unsigned long long oracle[NUMBER_OF_COMPUTORS];
    unsigned long long doge[NUMBER_OF_COMPUTORS];
    // Constant tx/oracle so factor is uniform; doge varies.
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        tx[i] = 1000000;
        oracle[i] = 1000000;
        doge[i] = 1000000;
    }

    long long prevRevenue = -1;
    // Sweep TARGET's dogeScore upward; require revenue to be non-decreasing.
    for (unsigned long long dogeVal : {1ULL, 100ULL, 10000ULL, 500000ULL, 999999ULL})
    {
        doge[TARGET] = dogeVal;
        runMultiDimRevenue(tx, oracle, doge);
        EXPECT_GE(gMultiDimRevenue.revenue[TARGET], prevRevenue);
        prevRevenue = gMultiDimRevenue.revenue[TARGET];
    }
}

// When factor as cap, the math return value as cap
// (S * (S^(K - 1)))^(1/K) = S
TEST(TestCoreRevenue, MultiDimDogeAtFullCapNoKEffect)
{
    constexpr unsigned long long S = REVENUE_SCALE;
    EXPECT_EQ(math_lib::irootK64<1>(S * math_lib::ipow(S, 0)), S);
    EXPECT_EQ(math_lib::irootK64<2>(S * math_lib::ipow(S, 1)), S);
    EXPECT_EQ(math_lib::irootK64<3>(S * math_lib::ipow(S, 2)), S);
    EXPECT_EQ(math_lib::irootK64<4>(S * math_lib::ipow(S, 3)), S);
}

// For a partial DOGE participation (dogeFactor < SCALE), higher K mean softer penalty
// and larger dogeRootScaled
TEST(TestCoreRevenue, MultiDimKSensitivityForDeficient)
{
    constexpr unsigned long long S = REVENUE_SCALE;
    constexpr unsigned long long partialDoge = S / 4;  // 25% of cap

    const unsigned long long rootK1 = math_lib::irootK64<1>(partialDoge * math_lib::ipow(S, 0));
    const unsigned long long rootK2 = math_lib::irootK64<2>(partialDoge * math_lib::ipow(S, 1));
    const unsigned long long rootK3 = math_lib::irootK64<3>(partialDoge * math_lib::ipow(S, 2));
    const unsigned long long rootK4 = math_lib::irootK64<4>(partialDoge * math_lib::ipow(S, 3));

    // All roots must stay in [0, SCALE]: monotone non-decreasing as K grows (softer penalty).
    EXPECT_LE(rootK1, S);
    EXPECT_LE(rootK4, S);
    EXPECT_LT(rootK1, rootK2);  // 256 < ~512
    EXPECT_LT(rootK2, rootK3);  // ~512 < ~645
    EXPECT_LT(rootK3, rootK4);  // ~645 < ~723

    // Specifically: K=1 returns partialDoge unchanged
    EXPECT_EQ(rootK1, partialDoge);
}

static unsigned long long invokeFinalizeTickScore(
    unsigned int tickOffset,
    const unsigned short* observed,
    const unsigned long long* windowSum)
{
    gMultiDimRevenue.initialTick = 0;
    const unsigned int leader = tickOffset % NUMBER_OF_COMPUTORS;
    gMultiDimRevenue.txScore[leader] = 0;
    finalizeTickScore(tickOffset, observed, windowSum);
    return gMultiDimRevenue.txScore[leader];
}

// sumCap == 0 (no expected activity in window), tickScore = SCALE (no-traffic case).
TEST(TestCoreRevenue, FinalizeTickScoreSumCapZero)
{
    unsigned short observed[REVENUE_TX_DIM];
    unsigned long long windowSum[REVENUE_TX_DIM];
    setMem(observed, sizeof(observed), 0);
    setMem(windowSum, sizeof(windowSum), 0);

    EXPECT_EQ(invokeFinalizeTickScore(0, observed, windowSum), REVENUE_SCALE);
}

// windowSum > 0, observed all zero: full deficit on every active dim, tickScore = 0.
TEST(TestCoreRevenue, FinalizeTickScoreFullDeficit)
{
    unsigned short observed[REVENUE_TX_DIM];
    unsigned long long windowSum[REVENUE_TX_DIM];
    setMem(observed, sizeof(observed), 0);
    setMem(windowSum, sizeof(windowSum), 0);

    // Heavy expected activity on the transfer dim (last dim).
    windowSum[REVENUE_TX_DIM - 1] = 5000ULL;

    // Observed is 0 across the board -> deficit == cap on every dim with ws > 0
    EXPECT_EQ(invokeFinalizeTickScore(0, observed, windowSum), 0ULL);
}

// observed[d] * REVENUE_WINDOW_SIZE == windowSum[d] for every dim, tickScore=SCALE.
TEST(TestCoreRevenue, FinalizeTickScorePerfectMatch)
{
    unsigned short observed[REVENUE_TX_DIM];
    unsigned long long windowSum[REVENUE_TX_DIM];
    setMem(observed, sizeof(observed), 0);
    setMem(windowSum, sizeof(windowSum), 0);

    // Pick a non-trivial pattern: a source dim and the transfer dim both active.
    observed[0] = 7;                          // wo = 7 * W
    windowSum[0] = (unsigned long long)REVENUE_WINDOW_SIZE * 7ULL;
    observed[REVENUE_TX_DIM - 1] = 4;         // wo = 4 * W
    windowSum[REVENUE_TX_DIM - 1] = (unsigned long long)REVENUE_WINDOW_SIZE * 4ULL;

    EXPECT_EQ(invokeFinalizeTickScore(0, observed, windowSum), REVENUE_SCALE);
}

// Over-delivery (observed*W > windowSum) on every dim still caps at SCALE
TEST(TestCoreRevenue, FinalizeTickScoreOverDeliveryNoBonus)
{
    unsigned short observed[REVENUE_TX_DIM];
    unsigned long long windowSum[REVENUE_TX_DIM];
    setMem(observed, sizeof(observed), 0);
    setMem(windowSum, sizeof(windowSum), 0);

    // Active dim with windowSum = a few hundred, but observed*W vastly exceeds it.
    windowSum[REVENUE_TX_DIM - 1] = 100ULL;
    observed[REVENUE_TX_DIM - 1] = 50;
    // Other dims: 0 windowSum + 0 observed, contribute 0 to both sums.

    const unsigned long long score = invokeFinalizeTickScore(0, observed, windowSum);
    EXPECT_EQ(score, REVENUE_SCALE);
    EXPECT_LE(score, REVENUE_SCALE);   // explicit cap guarantee
}

TEST(TestCoreRevenue, RevenueOnTickSlidingWindow)
{
    constexpr unsigned int H = REVENUE_HALF_WINDOW;
    constexpr unsigned int W = REVENUE_WINDOW_SIZE;
    constexpr unsigned int TRANSFER_DIM = REVENUE_TX_DIM - 1;
    constexpr unsigned int TRANSFER_PER_TICK = 3;

    setMem(&gMultiDimRevenue, sizeof(gMultiDimRevenue), 0);
    gMultiDimRevenue.initialTick = 0;

    unsigned int obs[REVENUE_TX_DIM];

    // Feed 2H ticks, all stored in head buffer, no finalization yet
    for (unsigned int t = 0; t < 2 * H; t++)
    {
        setMem(obs, sizeof(obs), 0);
        obs[TRANSFER_DIM] = TRANSFER_PER_TICK;
        revenueOnTick(t, obs);

        // No interior tick is finalized while tickOffset < 2H, so txScore stays zero everywhere
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_EQ(gMultiDimRevenue.txScore[i], 0ULL);
        }
    }

    // After 2H ticks, windowSum should equal 2H * TRANSFER_PER_TICK on the transfer dim
    EXPECT_EQ(gMultiDimRevenue.windowSum[TRANSFER_DIM],
              (unsigned long long)(2 * H) * TRANSFER_PER_TICK);

    // tickOffset == 2H finalizes the center tick c = H, txScore[leader-of-H] != 0
    setMem(obs, sizeof(obs), 0);
    obs[TRANSFER_DIM] = TRANSFER_PER_TICK;
    revenueOnTick(2 * H, obs);

    const unsigned int leaderOfH = H % NUMBER_OF_COMPUTORS;
    EXPECT_GT(gMultiDimRevenue.txScore[leaderOfH], 0ULL);
    EXPECT_LE(gMultiDimRevenue.txScore[leaderOfH], REVENUE_SCALE);

    // After feeding 2H+1 = W ticks, windowSum covers the full window
    EXPECT_EQ(gMultiDimRevenue.windowSum[TRANSFER_DIM],
              (unsigned long long)W * TRANSFER_PER_TICK);

    //  tickOffset == W triggers ring slot
    setMem(obs, sizeof(obs), 0);
    obs[TRANSFER_DIM] = TRANSFER_PER_TICK;
    revenueOnTick(W, obs);

    // windowSum on transfer dim stays at W * TRANSFER_PER_TICK because we evicted one entry
    // (the just-written tick-0 value) and added a new identical entry.
    EXPECT_EQ(gMultiDimRevenue.windowSum[TRANSFER_DIM],
              (unsigned long long)W * TRANSFER_PER_TICK);
}

// Serialize gMultiDimRevenue to a byte buffer, zero the global, copy back, verify bit-exact.
TEST(TestCoreRevenue, MultiDimSerializeDeserialize)
{
    // Fill with synthetic but deterministic-looking data.
    setMem(&gMultiDimRevenue, sizeof(gMultiDimRevenue), 0);
    gMultiDimRevenue.initialTick = 0xDEADBEEFu;
    gMultiDimRevenue.totalTicks = REVENUE_WINDOW_SIZE * 2u;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        gMultiDimRevenue.txScore[i] = (unsigned long long)i * 12345ULL;
        gMultiDimRevenue.revenue[i] = (long long)i * 6789LL;
    }
    for (unsigned int d = 0; d < REVENUE_TX_DIM; d++)
    {
        gMultiDimRevenue.windowSum[d] = (unsigned long long)d * 1234ULL;
    }
    // Sprinkle a few ring/head entries to catch layout misalignment.
    gMultiDimRevenue.revenueRing[0] = 11;
    gMultiDimRevenue.revenueRing[REVENUE_WINDOW_SIZE * REVENUE_TX_DIM - 1] = 22;
    gMultiDimRevenue.revenueHead[0] = 33;
    gMultiDimRevenue.revenueHead[2 * REVENUE_HALF_WINDOW * REVENUE_TX_DIM - 1] = 44;

    // Save bytes to heap buffer (struct is ~4 MB).
    auto buf = std::make_unique<unsigned char[]>(sizeof(MultiDimRevenue));
    copyMem(buf.get(), &gMultiDimRevenue, sizeof(gMultiDimRevenue));

    // Wipe global, restore from buffer.
    setMem(&gMultiDimRevenue, sizeof(gMultiDimRevenue), 0);
    copyMem(&gMultiDimRevenue, buf.get(), sizeof(gMultiDimRevenue));

    // Identical check
    EXPECT_EQ(gMultiDimRevenue.initialTick, 0xDEADBEEFu);
    EXPECT_EQ(gMultiDimRevenue.totalTicks, REVENUE_WINDOW_SIZE * 2u);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(gMultiDimRevenue.txScore[i], (unsigned long long)i * 12345ULL);
        EXPECT_EQ(gMultiDimRevenue.revenue[i], (long long)i * 6789LL);
    }
    EXPECT_EQ((unsigned int)gMultiDimRevenue.revenueRing[0], 11u);
    EXPECT_EQ((unsigned int)gMultiDimRevenue.revenueRing[REVENUE_WINDOW_SIZE * REVENUE_TX_DIM - 1], 22u);
    EXPECT_EQ((unsigned int)gMultiDimRevenue.revenueHead[0], 33u);
    EXPECT_EQ((unsigned int)gMultiDimRevenue.revenueHead[2 * REVENUE_HALF_WINDOW * REVENUE_TX_DIM - 1], 44u);
}


