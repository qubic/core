#pragma once

#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/revenue.h"

#include <fstream>

#ifndef CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
#endif

std::string TEST_DIR = "data/";
std::vector <std::string> REVENUE_FILES = {
"custom_revenue.eoe"
};

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

TEST(TestCoreRevenue, GeneralTest)
{
    unsigned long long tx[NUMBER_OF_COMPUTORS];
    unsigned long long votes[NUMBER_OF_COMPUTORS];
    unsigned long long customMiningShares[NUMBER_OF_COMPUTORS];
    long long revenuePerComputors[NUMBER_OF_COMPUTORS];

    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        tx[i] = random(0xFFFFFFFF);
    }
    computeRevFactor(tx, gTxScoreScalingThreshold, gTxScoreFactor);

    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        votes[i] = random(0xFFFFFFFF);
    }
    computeRevFactor(votes, gVoteScoreScalingThreshold, gVoteScoreFactor);

    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        customMiningShares[i] = random(0xFFFFFFFF);
    }
    computeRevFactor(customMiningShares, gCustomMiningScoreScalingThreshold, gCustomMiningScoreFactor);

    long long arbitratorRevenue = ISSUANCE_RATE;
    constexpr long long issuancePerComputor = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;
    for (unsigned int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        // Compute initial computor revenue, reducing arbitrator revenue
        unsigned long long combinedScoreFactor = gTxScoreFactor[computorIndex] * gVoteScoreFactor[computorIndex] * gCustomMiningScoreFactor[computorIndex];
        long long revenue = (long long)(combinedScoreFactor * issuancePerComputor / gTxScoreScalingThreshold / gVoteScoreScalingThreshold / gCustomMiningScoreScalingThreshold);
        revenuePerComputors[computorIndex] = revenue;
    }

    unsigned long long txQuorumScore = getQuorumScore(tx);
    unsigned long long voteQuorumScore = getQuorumScore(votes);
    unsigned long long customMiningQuorumScore = getQuorumScore(customMiningShares);

    // Checking score factor
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_LE(revenuePerComputors[i], arbitratorRevenue);
        EXPECT_GE(revenuePerComputors[i], 0);
    }
}

// V2 bonus factor end-to-end: run full computeRevenueV2 with different XMR/DOGE
// combinations. Oracle zero -> fallback (full factor). Uniform TX -> full factor.
// M = S for all cases, revenue depends only on E = (xmrFactor + dogeFactor) / 2.
TEST(TestCoreRevenue, V2BonusFactorCombination)
{
    constexpr unsigned int TOTAL_TICKS = NUMBER_OF_COMPUTORS * 2;  // must be multiple of 676 for equal rotation
    static_assert(TOTAL_TICKS >= REVENUE_WINDOW_SIZE, "need enough ticks for sliding window");
    constexpr unsigned long long S = REVENUE_SCALE;
    constexpr unsigned long long B = REVENUE_BONUS_CAP;

    EpochRevenueData data;

    // Helper: setup uniform epoch data for each sub-case
    // All oracle scores zero -> fallback gives full oracle factor
    // Uniform TX -> all computors get full TX factor -> M = S
    auto resetData = [&]()
        {
            setMem(&data, sizeof(data), 0);
            data.initialTick = 0;
            data.totalTicks = TOTAL_TICKS;
            for (unsigned int t = 0; t < TOTAL_TICKS; t++)
            {
                data.perTickTxCount[t] = 100;
            }
        };

    // Case 1: Both XMR and DOGE full -> E = S -> revenue = IPC (100%)
    resetData();
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        gRevenueComponents.customMiningScore[i] = 1000;
        data.dogeMiningScore[i] = 1000;
    }
    computeRevenueV2(data);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(data.v2Revenue[i], REVENUE_IPC);
    }

    // Case 2: XMR full, DOGE zero -> E = S/2 -> revenue = IPC * S * (S^2 + B*S/2) / DIVISOR
    resetData();
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        gRevenueComponents.customMiningScore[i] = 1000;
        data.dogeMiningScore[i] = 0;
    }
    computeRevenueV2(data);
    {
        unsigned long long numerator = S * (S * S + B * (S / 2));
        long long expected = (long long)((unsigned long long)REVENUE_IPC * numerator / REVENUE_DIVISOR);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_EQ(data.v2Revenue[i], expected);
        }
    }

    // Case 3: XMR zero, DOGE full -> same as case 2 (symmetric)
    resetData();
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        gRevenueComponents.customMiningScore[i] = 0;
        data.dogeMiningScore[i] = 1000;
    }
    computeRevenueV2(data);
    {
        unsigned long long numerator = S * (S * S + B * (S / 2));
        long long expected = (long long)((unsigned long long)REVENUE_IPC * numerator / REVENUE_DIVISOR);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_EQ(data.v2Revenue[i], expected);
        }
    }

    // Case 4: Both zero -> E = 0 -> revenue = IPC * S/(S+B) ~ 80%
    resetData();
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        gRevenueComponents.customMiningScore[i] = 0;
        data.dogeMiningScore[i] = 0;
    }
    computeRevenueV2(data);
    {
        unsigned long long numerator = S * (S * S);
        long long expected = (long long)((unsigned long long)REVENUE_IPC * numerator / REVENUE_DIVISOR);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_EQ(data.v2Revenue[i], expected);
        }
        // Sanity: base revenue should be ~80% of IPC
        EXPECT_GT(expected, REVENUE_IPC * 79 / 100);
        EXPECT_LT(expected, REVENUE_IPC * 81 / 100);
    }
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
    //    Max logScore = 7099 (gTxRevenuePoints[1024])
    //    Per-tick max = 7099 * 1024 * 1351 = 9,820,926,976 (fits u32? no, needs u64)
    {
        unsigned long long perTickMax = (unsigned long long)maxTxRevPoints * S * REVENUE_WINDOW_SIZE;
        EXPECT_EQ(perTickMax, 9820926976ULL);
        EXPECT_LE(perTickMax, u64Max);
    }

    // Sliding window accumulated per computor across full epoch
    //    Each computor gets MAX_NUMBER_OF_TICKS_PER_EPOCH / 676 ticks
    //    Max accumulated = perTickMax * ticksPerComputor ~ 2.51e13
    {
        unsigned long long perTickMax = (unsigned long long)maxTxRevPoints * S * REVENUE_WINDOW_SIZE;
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

    // End-to-end: run computeRevenueV2 with max TX (1024/tick), max mining scores,
    //    full epoch worth of ticks. Must not produce negative or >IPC revenue.
    {
        constexpr unsigned int TOTAL_TICKS = REVENUE_WINDOW_SIZE + NUMBER_OF_COMPUTORS * 10;
        EpochRevenueData data;
        setMem(&data, sizeof(data), 0);
        data.initialTick = 0;
        data.totalTicks = TOTAL_TICKS;
        for (unsigned int t = 0; t < TOTAL_TICKS; t++)
        {
            data.perTickTxCount[t] = 1024;
        }
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            gRevenueComponents.customMiningScore[i] = 1000000;
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
        EpochRevenueData data;
        setMem(&data, sizeof(data), 0);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            gRevenueComponents.customMiningScore[i] = 0;
        }
        computeRevenueV2(data);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            EXPECT_EQ(data.v2Revenue[i], 0);
        }
    }
}

// Simulate the revenue fomula from real data
TEST(TestCoreRevenue, ReadFile)
{
    unsigned long long tx[NUMBER_OF_COMPUTORS];
    unsigned long long votes[NUMBER_OF_COMPUTORS];
    unsigned long long customMining[NUMBER_OF_COMPUTORS];
    long long revenue[NUMBER_OF_COMPUTORS];
    constexpr long long issuancePerComputor = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;

    for (size_t i = 0; i < REVENUE_FILES.size(); ++i)
    {
        // Open input file in binary mode
        std::string input = TEST_DIR + REVENUE_FILES[i];
        std::ifstream infile(input, std::ios::binary);
        if (!infile)
        {
            std::cerr << "Error opening file: " << input << "\n";
            std::exit(EXIT_FAILURE);
        }

        // Read transaction, vote and custom mining share
        infile.read(reinterpret_cast<char*>(&tx), sizeof(tx));
        if (!infile)
        {
            std::cerr << "Error reading tx score from file.\n";
            std::exit(EXIT_FAILURE);
        }

        infile.read(reinterpret_cast<char*>(&votes), sizeof(votes));
        if (!infile)
        {
            std::cerr << "Error reading votes score from file.\n";
            std::exit(EXIT_FAILURE);
        }

        infile.read(reinterpret_cast<char*>(&customMining), sizeof(customMining));
        if (!infile)
        {
            std::cerr << "Error reading custom mining score from file.\n";
            std::exit(EXIT_FAILURE);
        }

        infile.close();

        // Start to compute and write out data
        computeRevenue(tx, votes, customMining, revenue);

        // Write the data out for investigation
        // Open output file in text mode
        std::string output = input + ".csv";
        std::ofstream outfile(output);
        if (!outfile)
        {
            std::cerr << "Error opening output file: " << output << "\n";
            std::exit(EXIT_FAILURE);
        }

        // Write CSV header
        outfile << "Index,txScore,voteScore,customMiningScore,txScoreFactor,voteScoreFactor,customMiningScoreFactor,revenue,percentage\n";

        // Write the content
        for (int k = 0; k < NUMBER_OF_COMPUTORS; k++)
        {
            outfile << k << ","
                << tx[k] << ","
                << votes[k] << ","
                << customMining[k] << ","
                << gRevenueComponents.txScoreFactor[k] << ","
                << gRevenueComponents.voteScoreFactor[k] << ","
                << gRevenueComponents.customMiningScoreFactor[k] << ","
                << revenue[k] << ","
                << (double)revenue[k] * 100 / issuancePerComputor
                << "\n";
        }
        outfile.close();
    }
}
