#define NO_UEFI

#include "gtest/gtest.h"

#define ENABLE_PROFILING 0

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
static const std::string COMMON_TEST_SCORES_FILE_NAME = "data/scores_v5.csv";
static constexpr bool PRINT_DETAILED_INFO = false;

// set to 0 for run all available samples
// For profiling enable, run all available samples
static constexpr unsigned long long COMMON_TEST_NUMBER_OF_SAMPLES = 32;
static constexpr unsigned long long PROFILING_NUMBER_OF_SAMPLES = 32;


// set 0 for run maximum number of threads of the computer.
// For profiling enable, set it equal to deployment setting
static constexpr int MAX_NUMBER_OF_THREADS = 0;
static constexpr int MAX_NUMBER_OF_PROFILING_THREADS = 12;
static bool gCompareReference = false;

// Only run on specific index of samples and setting
std::vector<unsigned int> filteredSamples;// = { 0 };
std::vector<unsigned int> filteredSettings;// = { 0 };

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
    auto pScore = std::make_unique<ScoreFunction<
        kSettings[i][score_params::NUMBER_OF_INPUT_NEURONS],
        kSettings[i][score_params::NUMBER_OF_OUTPUT_NEURONS],
        kSettings[i][score_params::NUMBER_OF_TICKS],
        kSettings[i][score_params::NUMBER_OF_NEIGHBORS],
        kSettings[i][score_params::POPULATION_THRESHOLD],
        kSettings[i][score_params::NUMBER_OF_MUTATIONS],
        kSettings[i][score_params::SOLUTION_THRESHOLD],
        1
        >>();

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
        score_reference::ScoreReferenceImplementation<
            kSettings[i][score_params::NUMBER_OF_INPUT_NEURONS],
            kSettings[i][score_params::NUMBER_OF_OUTPUT_NEURONS],
            kSettings[i][score_params::NUMBER_OF_TICKS],
            kSettings[i][score_params::NUMBER_OF_NEIGHBORS],
            kSettings[i][score_params::POPULATION_THRESHOLD],
            kSettings[i][score_params::NUMBER_OF_MUTATIONS],
            kSettings[i][score_params::SOLUTION_THRESHOLD],
            1
        > score;
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
            {
                std::cout << "[sample " << sampleIndex
                    << "; setting " << i << ": "
                    << kSettings[i][score_params::NUMBER_OF_INPUT_NEURONS] << ", "
                    << kSettings[i][score_params::NUMBER_OF_OUTPUT_NEURONS] << ", "
                    << kSettings[i][score_params::NUMBER_OF_TICKS] << ", "
                    << kSettings[i][score_params::POPULATION_THRESHOLD] << ", "
                    << kSettings[i][score_params::NUMBER_OF_MUTATIONS] << ", "
                    << kSettings[i][score_params::SOLUTION_THRESHOLD]
                    << "]"
                    << std::endl;
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
        }
        {
            EXPECT_GT(gScoreIndexMap.count(i), 0);
            if (gtIndex >= 0)
            {
                EXPECT_EQ(gScoresGroundTruth[sampleIndex][gtIndex], score_value);
            }
        }
    }
}

// Recursive template to process each element in scoreSettings
template <unsigned long long i>
static void processElementWithPerformance(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex)
{
    auto pScore = std::make_unique<ScoreFunction<
        kProfileSettings[i][score_params::NUMBER_OF_INPUT_NEURONS],
        kProfileSettings[i][score_params::NUMBER_OF_OUTPUT_NEURONS],
        kProfileSettings[i][score_params::NUMBER_OF_TICKS],
        kProfileSettings[i][score_params::NUMBER_OF_NEIGHBORS],
        kProfileSettings[i][score_params::POPULATION_THRESHOLD],
        kProfileSettings[i][score_params::NUMBER_OF_MUTATIONS],
        kProfileSettings[i][score_params::SOLUTION_THRESHOLD],
        1
        >>();

    pScore->initMemory();
    pScore->initMiningData(miningSeed);
    int x = 0;
    top_of_stack = (unsigned long long)(&x);
    auto t0 = std::chrono::high_resolution_clock::now();
    unsigned int score_value = (*pScore)(0, publicKey, miningSeed, nonce);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto d = t1 - t0;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();

#pragma omp critical
    {
        if (gScoreProcessingTime.count(i) == 0)
        {
            gScoreProcessingTime[i] = elapsed;
        }
        else
        {
            gScoreProcessingTime[i] += elapsed;
        }
    }
}

// Main processing function
template <char profiling, unsigned long long N, unsigned long long... Is>
static void processHelper(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex, std::index_sequence<Is...>)
{
    if constexpr (profiling)
    {
        (processElementWithPerformance<Is>(miningSeed, publicKey, nonce, sampleIndex), ...);
    }
    else
    {
        (processElement<Is>(miningSeed, publicKey, nonce, sampleIndex), ...);
    }
}

// Recursive template to process each element in scoreSettings
template <char profiling, unsigned long long N>
static void process(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex)
{
    processHelper<profiling, N>(miningSeed, publicKey, nonce, sampleIndex, std::make_index_sequence<N>{});
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
    unsigned long long numberOfSamplesReadFromFile = sampleString.size();
    unsigned long long numberOfSamples = numberOfSamplesReadFromFile;
    unsigned long long requestedNumberOfSamples = COMMON_TEST_NUMBER_OF_SAMPLES;

    if (requestedNumberOfSamples > 0)
    {
        std::cout << "Request testing with " << requestedNumberOfSamples << " samples." << std::endl;

        numberOfSamples = std::min(requestedNumberOfSamples, numberOfSamples);
        if (requestedNumberOfSamples <= numberOfSamples)
        {
            numberOfSamples = requestedNumberOfSamples;
        }
        else // Request number of samples greater than existed. Only valid for reference score validation only
        {
            if (gCompareReference)
            {
                numberOfSamples = requestedNumberOfSamples;
                std::cout << "Refenrece comparison mode: " << numberOfSamples << " samples are read from file for comparision."
                    << "Remained are generated randomly."
                    << std::endl;
            }
            else
            {
                std::cout << "Only " << numberOfSamples << " samples can be read from file for comparison" << std::endl;
            }
        }
    }

    std::vector<m256i> miningSeeds(numberOfSamples);
    std::vector<m256i> publicKeys(numberOfSamples);
    std::vector<m256i> nonces(numberOfSamples);

    // Reading the input samples
    for (unsigned long long i = 0; i < numberOfSamples; ++i)
    {
        if (i < numberOfSamplesReadFromFile)
        {
            miningSeeds[i] = hexTo32Bytes(sampleString[i][0], 32);
            publicKeys[i] = hexTo32Bytes(sampleString[i][1], 32);
            nonces[i] = hexTo32Bytes(sampleString[i][2], 32);
        }
        else // Samples from files are not enough, randomly generate more
        {
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[0]);
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[8]);
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[16]);
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[24]);

            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[0]);
            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[8]);
            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[16]);
            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[24]);

            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[0]);
            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[8]);
            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[16]);
            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[24]);

        }
    }

    // Reading the header of score and verification
    if (!gCompareReference)
    {
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
                    if (scoresSettingHeader[j] == kSettings[i][j])
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
        if (gScoreIndexMap.size() < numberOfGeneratedSetting)
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
    }


    // Run the test
    unsigned int numberOfThreads = std::thread::hardware_concurrency();
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
        std::cout << "Running one sample on one thread for collecting single thread performance." << std::endl;
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

    std::string compTerm = "and compare with groundtruths from file.";
    if (gCompareReference)
    {
        compTerm = "and compare with reference code.";
    }

    std::cout << "Processing " << samples.size() << " samples " << compTerm << "..." << std::endl;
    gScoreProcessingTime.clear();
#pragma omp parallel for num_threads(numberOfThreads)
    for (int i = 0; i < samples.size(); ++i)
    {
        int index = samples[i];
        process<0, numberOfGeneratedSetting>(miningSeeds[index].m256i_u8, publicKeys[index].m256i_u8, nonces[index].m256i_u8, index);
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
            std::cout << "Avg processing time [setting " << scoreTime.first << " "
                << kSettings[scoreTime.first][score_params::NUMBER_OF_INPUT_NEURONS] << ", "
                << kSettings[scoreTime.first][score_params::NUMBER_OF_OUTPUT_NEURONS] << ", "
                << kSettings[scoreTime.first][score_params::NUMBER_OF_TICKS] << ", "
                << kSettings[scoreTime.first][score_params::NUMBER_OF_NEIGHBORS] << ", "
                << kSettings[scoreTime.first][score_params::POPULATION_THRESHOLD] << ", "
                << kSettings[scoreTime.first][score_params::NUMBER_OF_MUTATIONS] << ", "
                << kSettings[scoreTime.first][score_params::SOLUTION_THRESHOLD]
                << "]: " << processingTime << " ms" << std::endl;
        }
    }
}

void runPerformanceTests()
{

#if defined (__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
#endif
    constexpr unsigned long long numberOfGeneratedSetting = sizeof(score_params::kProfileSettings) / sizeof(score_params::kProfileSettings[0]);

    // Read the parameters and results
    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);
    auto scoresString = readCSV(COMMON_TEST_SCORES_FILE_NAME);
    ASSERT_FALSE(sampleString.empty());
    ASSERT_FALSE(scoresString.empty());

    // Convert the raw string and do the data verification
    unsigned long long numberOfSamplesReadFromFile = sampleString.size();
    unsigned long long numberOfSamples = numberOfSamplesReadFromFile;
    unsigned long long requestedNumberOfSamples = PROFILING_NUMBER_OF_SAMPLES;

    if (requestedNumberOfSamples > 0)
    {
        std::cout << "Request testing with " << requestedNumberOfSamples << " samples." << std::endl;

        numberOfSamples = std::min(requestedNumberOfSamples, numberOfSamples);
        if (requestedNumberOfSamples <= numberOfSamples)
        {
            numberOfSamples = requestedNumberOfSamples;
        }
    }

    std::vector<m256i> miningSeeds(numberOfSamples);
    std::vector<m256i> publicKeys(numberOfSamples);
    std::vector<m256i> nonces(numberOfSamples);

    // Reading the input samples
    for (unsigned long long i = 0; i < numberOfSamples; ++i)
    {
        if (i < numberOfSamplesReadFromFile)
        {
            miningSeeds[i] = hexTo32Bytes(sampleString[i][0], 32);
            publicKeys[i] = hexTo32Bytes(sampleString[i][1], 32);
            nonces[i] = hexTo32Bytes(sampleString[i][2], 32);
        }
        else // Samples from files are not enough, randomly generate more
        {
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[0]);
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[8]);
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[16]);
            _rdrand64_step((unsigned long long*) & miningSeeds[i].m256i_u8[24]);

            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[0]);
            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[8]);
            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[16]);
            _rdrand64_step((unsigned long long*) & publicKeys[i].m256i_u8[24]);

            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[0]);
            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[8]);
            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[16]);
            _rdrand64_step((unsigned long long*) & nonces[i].m256i_u8[24]);

        }
    }

    std::cout << "Profiling " << numberOfGeneratedSetting << " param combinations. " << std::endl;

    // Run the profiling
    unsigned int numberOfThreads = std::thread::hardware_concurrency();
    if (MAX_NUMBER_OF_PROFILING_THREADS > 0)
    {
        numberOfThreads = numberOfThreads > MAX_NUMBER_OF_PROFILING_THREADS ? MAX_NUMBER_OF_PROFILING_THREADS : numberOfThreads;
    }
    std::cout << "Running " << numberOfThreads << " threads for collecting multiple threads performance" << std::endl;

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

    std::string compTerm = "for profiling, don't compare any result.";

    std::cout << "Processing " << samples.size() << " samples " << compTerm << "..." << std::endl;
    gScoreProcessingTime.clear();
#pragma omp parallel for num_threads(numberOfThreads)
    for (int i = 0; i < samples.size(); ++i)
    {
        int index = samples[i];
        process<1, numberOfGeneratedSetting>(miningSeeds[index].m256i_u8, publicKeys[index].m256i_u8, nonces[index].m256i_u8, index);
#pragma omp critical
        std::cout << i << ", ";
    }
    std::cout << std::endl;

    // Print the average processing time
    for (auto scoreTime : gScoreProcessingTime)
    {
        unsigned long long processingTime = filteredSamples.empty() ? scoreTime.second / numberOfSamples : scoreTime.second / filteredSamples.size();
        std::cout << "Avg processing time [setting " << scoreTime.first << " "
            << kProfileSettings[scoreTime.first][score_params::NUMBER_OF_INPUT_NEURONS] << ", "
            << kProfileSettings[scoreTime.first][score_params::NUMBER_OF_OUTPUT_NEURONS] << ", "
            << kProfileSettings[scoreTime.first][score_params::NUMBER_OF_TICKS] << ", "
            << kProfileSettings[scoreTime.first][score_params::NUMBER_OF_NEIGHBORS] << ", "
            << kProfileSettings[scoreTime.first][score_params::POPULATION_THRESHOLD] << ", "
            << kProfileSettings[scoreTime.first][score_params::NUMBER_OF_MUTATIONS] << ", "
            << kProfileSettings[scoreTime.first][score_params::SOLUTION_THRESHOLD]
            << "]: " << processingTime << " ms" << std::endl;
    }
    gProfilingDataCollector.writeToFile();
}

TEST(TestQubicScoreFunction, CommonTests)
{
    runCommonTests();
}

#if ENABLE_PROFILING

TEST(TestQubicScoreFunction, PerformanceTests)
{
    runPerformanceTests();
}
#endif

#if not ENABLE_PROFILING
TEST(TestQubicScoreFunction, TestDeterministic)
{
    constexpr int NUMBER_OF_THREADS = 4;
    constexpr int NUMBER_OF_PHASES = 2;
    constexpr int NUMBER_OF_SAMPLES = 4;

    // Read the parameters and results
    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);
    ASSERT_FALSE(sampleString.empty());

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
        miningSeeds[i] = hexTo32Bytes(sampleString[i][0], 32);
        publicKeys[i] = hexTo32Bytes(sampleString[i][1], 32);
        nonces[i] = hexTo32Bytes(sampleString[i][2], 32);
    }

    auto pScore = std::make_unique<ScoreFunction<
        ::NUMBER_OF_INPUT_NEURONS,
        ::NUMBER_OF_OUTPUT_NEURONS,
        ::NUMBER_OF_TICKS,
        ::NUMBER_OF_NEIGHBORS,
        ::POPULATION_THRESHOLD,
        ::NUMBER_OF_MUTATIONS,
        ::SOLUTION_THRESHOLD,
        NUMBER_OF_THREADS
        >>();
    pScore->initMemory();

    // Run with 4 mining seeds, each run 4 separate threads and the result need to matched
    int scores[NUMBER_OF_PHASES][NUMBER_OF_THREADS * NUMBER_OF_SAMPLES] = { 0 };
    for (unsigned long long i = 0; i < NUMBER_OF_PHASES; ++i)
    {
        pScore->initMiningData(miningSeeds[i]);

#pragma omp parallel for num_threads(NUMBER_OF_THREADS)
        for (int threadId = 0; threadId < NUMBER_OF_THREADS; ++threadId)
        {
            if (threadId % 2 == 0)
            {
                for (int sampleId = 0; sampleId < NUMBER_OF_SAMPLES; ++sampleId)
                {
                    scores[i][threadId * NUMBER_OF_SAMPLES + sampleId] = (*pScore)(threadId, publicKeys[sampleId], miningSeeds[i], nonces[sampleId]);
                }
            }
            else
            {
                for (int sampleId = NUMBER_OF_SAMPLES - 1; sampleId >= 0; --sampleId)
                {
                    scores[i][threadId * NUMBER_OF_SAMPLES + sampleId] = (*pScore)(threadId, publicKeys[sampleId], miningSeeds[i], nonces[sampleId]);
                }
            }
        }
    }

    // Each threads run with the same samples but the order is reversed. Expect the scores are matched.
    for (unsigned long long i = 0; i < NUMBER_OF_PHASES; ++i)
    {
        for (int threadId = 0; threadId < NUMBER_OF_THREADS - 1; ++threadId)
        {
            for (int sampleId = 0; sampleId < NUMBER_OF_SAMPLES; ++sampleId)
            {
                EXPECT_EQ(scores[i][threadId * NUMBER_OF_SAMPLES + sampleId], scores[i][(threadId + 1) * NUMBER_OF_SAMPLES + sampleId]);
            }
        }
    }
}
#endif
