#pragma once

#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/revenue.h"

#include <fstream>

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
