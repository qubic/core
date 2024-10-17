#define NO_UEFI

#include "gtest/gtest.h"

// needed for scoring task queue
#define NUMBER_OF_TRANSACTIONS_PER_TICK 1024

// current optimized implementation
#include "../src/score.h"

// reference implementation
#include "score_reference.h"

// params settting
#include "score_params.h"

#include "utils.h"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <thread>

using namespace score_params;
using namespace test_utils;

// When algorithm change, belows need to do
// - score_params.h: adjust the number of ParamType and change the config of kSettings
// - Modify the score reference run with new setting
// - Need to verify about the idex of setting in the template of score/score_reference class
// - Re-run the test_score_generation for generating 2 csv files, scores and samples
// - Copy the files into test/data
// - Rename COMMON_TEST_SAMPLES_FILE_NAME and COMMON_TEST_SCORES_FILE_NAME if neccessary


static const std::string COMMON_TEST_SAMPLES_FILE_NAME = "data/samples_20240815.csv";
static const std::string COMMON_TEST_SCORES_FILE_NAME = "data/scores_v3.csv";
static constexpr unsigned long long COMMON_TEST_NUMBER_OF_SAMPLES = 32; // set to 0 for run all available samples
static constexpr bool PRINT_DETAILED_INFO = false;
static constexpr int MAX_NUMBER_OF_THREADS = 0; // set 0 for run maximum number of threads of the computer.
static bool gCompareReference = false;

// Only run on specific index of samples and setting
std::vector<unsigned int> filteredSamples;// = { 0 };
std::vector<unsigned int> filteredSettings;// = { 0,1 };

std::vector<std::vector<unsigned int>> gScoresGroundTruth;
std::map<unsigned int, unsigned long long> gScoreProcessingTime;
std::map<unsigned long long, unsigned long long> gScoreIndexMap;

// Recursive template to process each element in scoreSettings
template <unsigned long long i>
static void processElement(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex)
{
    if (!filteredSettings.empty()
        && std::find(filteredSettings.begin(), filteredSettings.end(), i) == filteredSettings.end())
    {
        return;
    }

    auto pScore = std::make_unique<ScoreFunction<kDataLength, kSettings[i][NR_NEURONS], kSettings[i][NR_NEIGHBOR_NEURONS], kSettings[i][DURATIONS], 1>>();
    pScore->initMemory();
    pScore->initMiningData(miningSeed);
    int x = 0;
    top_of_stack = (unsigned long long)(&x);
    auto t0 = std::chrono::high_resolution_clock::now();
    unsigned int score_value = (*pScore)(0, publicKey, miningSeed, nonce);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto d = t1 - t0;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();

    unsigned int refScore = 0;
    if (gCompareReference)
    {
        ScoreReferenceImplementation<kDataLength, kSettings[i][NR_NEURONS], kSettings[i][NR_NEIGHBOR_NEURONS], kSettings[i][DURATIONS], 1> score;
        score.initMemory();
        score.initMiningData(miningSeed);
        refScore = score(0, publicKey, nonce);
    }
#pragma omp critical
    if (gCompareReference)
    {
        EXPECT_EQ(refScore, score_value);
    }
    else
    {
        long long gtIndex = -1;
        if (gScoreIndexMap.count(i) > 0)
        {
            gtIndex = gScoreIndexMap[i];
        }

        if (PRINT_DETAILED_INFO || gtIndex < 0 || (score_value != gScoresGroundTruth[sampleIndex][gtIndex]))
        {
            if (gScoreProcessingTime.count(i) == 0)
            {
                gScoreProcessingTime[i] = elapsed;
            }
            else
            {
                gScoreProcessingTime[i] += elapsed;
            }

            std::cout << "[sample " << sampleIndex
                << "; setting " << i
                << ": NEURON " << kSettings[i][NR_NEURONS]
                << ", NEIGHBOR " << kSettings[i][NR_NEIGHBOR_NEURONS]
                << ", DURATIONS " << kSettings[i][DURATIONS] << "]" << std::endl;
            std::cout << "    score " << score_value;
            if (gtIndex >= 0)
            {
                std::cout << " vs gt " << gScoresGroundTruth[sampleIndex][gtIndex] << std::endl;
            }
            else // No mapping from ground truth
            {
                std::cout << " vs gt NA" << std::endl;
            }
            std::cout << "    time " << elapsed << " ms " << std::endl;
        }

        EXPECT_GT(gScoreIndexMap.count(i), 0);
        if (gtIndex >= 0)
        {
            EXPECT_EQ(gScoresGroundTruth[sampleIndex][gtIndex], score_value);
        }
    }
}

// Main processing function
template <unsigned long long N, unsigned long long... Is>
static void processHelper(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex, std::index_sequence<Is...>)
{
    (processElement<Is>(miningSeed, publicKey, nonce, sampleIndex), ...);
}

// Recursive template to process each element in scoreSettings
template <unsigned long long N>
static void process(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex)
{
    processHelper<N>(miningSeed, publicKey, nonce, sampleIndex, std::make_index_sequence<N>{});
}

void runCommonTests()
{

#if defined (__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
#endif
    constexpr unsigned long long numberOfGeneratedSetting = sizeof(score_params::kSettings) / sizeof(score_params::kSettings[0]);

    // Read the parameters and results
    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);
    auto scoresString = readCSV(COMMON_TEST_SCORES_FILE_NAME);
    ASSERT_FALSE(sampleString.empty());
    ASSERT_FALSE(scoresString.empty());

    // Convert the raw string and do the data verification
    unsigned long long numberOfSamples = sampleString.size();
    if (COMMON_TEST_NUMBER_OF_SAMPLES > 0)
    {
        numberOfSamples = std::min(COMMON_TEST_NUMBER_OF_SAMPLES, numberOfSamples);
    }

    std::vector<m256i> miningSeeds(numberOfSamples);
    std::vector<m256i> publicKeys(numberOfSamples);
    std::vector<m256i> nonces(numberOfSamples);

    // Reading the input samples
    for (unsigned long long i = 0; i < numberOfSamples; ++i)
    {
        miningSeeds[i] = hexToByte(sampleString[i][0], 32);
        publicKeys[i] = hexToByte(sampleString[i][1], 32);
        nonces[i] = hexToByte(sampleString[i][2], 32);
    }

    // Reading the header of score and verification
    auto scoreHeader = scoresString[0];
    std::cout << "Testing " << numberOfGeneratedSetting << " param combinations on " << scoreHeader.size() << " ground truth settings." << std::endl;
    for (unsigned long long i = 0; i < numberOfGeneratedSetting; ++i)
    {
        long long foundIndex = -1;

        for (unsigned long long gtIdx = 0; gtIdx < scoreHeader.size(); ++gtIdx)
        {
            auto scoresSettingHeader = convertULLFromString(scoreHeader[gtIdx]);

            // Check matching between number of parameters types
            if (scoresSettingHeader.size() != score_params::MAX_PARAM_TYPE)
            {
                std::cout << "Mismatched the number of params (NEURONS, DURATION ...) and MAX_PARAM_TYPE" << std::endl;
                EXPECT_EQ(scoresSettingHeader.size(), score_params::MAX_PARAM_TYPE);
                return;
            }

            // Check the value matching between ground truth file and score params
            // Only record the current available score params
            int count = 0;
            for (unsigned long long j = 0; j < score_params::MAX_PARAM_TYPE; ++j)
            {
                if (scoresSettingHeader[j] == score_params::kSettings[i][j])
                {
                    count++;
                }
            }
            if (count == score_params::MAX_PARAM_TYPE)
            {
                foundIndex = gtIdx;
                break;
            }
        }
        if (foundIndex >= 0)
        {
            gScoreIndexMap[i] = foundIndex;
        }
    }
    // In case of number of setting is lower than the ground truth. Consider we are in experiement, still run but expect the test failed
    if (gScoreIndexMap.size() < numberOfGeneratedSetting && !gCompareReference)
    {
        std::cout << "WARNING: Number of provided ground truth settings is lower than tested settings. Only test with available ones." 
                  << std::endl;
        EXPECT_EQ(gScoreIndexMap.size(), numberOfGeneratedSetting);
    }

    // Read the groudtruth scores and init result scores
    numberOfSamples = std::min(numberOfSamples, scoresString.size() - 1);
    gScoresGroundTruth.resize(numberOfSamples);
    for (size_t i = 0; i < numberOfSamples; ++i)
    {
       auto scoresStr = scoresString[i + 1];
       size_t scoreSize = scoresStr.size();
       for (size_t j = 0; j < scoreSize; ++j)
       {
           gScoresGroundTruth[i].push_back(std::stoi(scoresStr[j]));
       }
    }

    // Run the test
    unsigned int numberOfThreads = PRINT_DETAILED_INFO ? 1 : std::thread::hardware_concurrency();
    if (MAX_NUMBER_OF_THREADS > 0)
    {
        numberOfThreads = numberOfThreads > MAX_NUMBER_OF_THREADS ? MAX_NUMBER_OF_THREADS : numberOfThreads;
    }
    if (numberOfThreads > 1)
    {
        std::cout << "Compare score only. Lauching test with all available " << numberOfThreads << " threads." << std::endl;
    }
    else
    {
        std::cout << "Running one sample on one thread for collecting performance." << std::endl;
    }

    std::vector<int> samples;
    for (int i = 0; i < numberOfSamples; ++i)
    {
        if (!filteredSamples.empty()
            && std::find(filteredSamples.begin(), filteredSamples.end(), i) == filteredSamples.end())
        {
            continue;
        }
        samples.push_back(i);
    }

    std::cout << "Processing " << samples.size() << " samples ..." << std::endl;
#pragma omp parallel for num_threads(numberOfThreads)
    for (int i = 0; i < samples.size(); ++i)
    {
        int index = samples[i];
        process<numberOfGeneratedSetting>(miningSeeds[index].m256i_u8, publicKeys[index].m256i_u8, nonces[index].m256i_u8, index);
#pragma omp critical
        std::cout << i << ", ";
    }
    std::cout << std::endl;

    // Print the average processing time
    if (PRINT_DETAILED_INFO)
    {
        for (auto scoreTime : gScoreProcessingTime)
        {
            unsigned long long processingTime = filteredSamples.empty() ? scoreTime.second / numberOfSamples : scoreTime.second / filteredSamples.size();
            std::cout << "Avg processing time [setting " << scoreTime.first
                << ", NEURON " << kSettings[scoreTime.first][NR_NEURONS]
                << ", NEIGHBOR " << kSettings[scoreTime.first][NR_NEIGHBOR_NEURONS]
                << ", DURATIONS " << kSettings[scoreTime.first][DURATIONS]
                << "]: " << processingTime << " ms" << std::endl;
        }
    }
}


TEST(TestQubicScoreFunction, CommonTests)
{
    runCommonTests();
}
