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

#include <chrono>
#include <fstream>
#include <filesystem>

using namespace score_params;

// Only run on specific samples and settting
std::vector<unsigned int> filteredSamples;// = { 0, 2 };
std::vector<unsigned int> filteredSettings;// = { 0, 2 };

std::map<unsigned int, std::map<unsigned int, unsigned int>> scoresResult;
std::vector<std::vector<unsigned int>> scoresGroundTruth;
std::map<unsigned int, unsigned long long> scoreProcessingTime;

// Recursive template to process each element in scoreSettings
template <unsigned long long i>
static void processElement(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int sampleIndex)
{
    if (!filteredSettings.empty()
        && std::find(filteredSettings.begin(), filteredSettings.end(), i) == filteredSettings.end())
    {
        return;
    }
    std::cout << "[Samples " << sampleIndex
        << ", NEURON " << kSettings[i][NR_NEURONS]
        << ", DURATIONS " << kSettings[i][DURATIONS] << "]" << std::endl;

    auto pScore = std::make_unique<ScoreFunction<kDataLength, kSettings[i][NR_NEURONS], kSettings[i][NR_NEURONS], kSettings[i][DURATIONS], kSettings[i][DURATIONS], 1>>();
    pScore->initMemory();
    pScore->initMiningData(miningSeed);
    int x = 0;
    top_of_stack = (unsigned long long)(&x);
    auto t0 = std::chrono::high_resolution_clock::now();
    unsigned int score_value = (*pScore)(0, publicKey, nonce);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto d = t1 - t0;
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(d);

    if (scoreProcessingTime.count(i) == 0)
    {
        scoreProcessingTime[i] = elapsed.count();
    }
    else
    {
        scoreProcessingTime[i] += elapsed.count();
    }
    scoresResult[sampleIndex][i] = score_value;
    std::cout <<"score " << score_value << " vs reference " << scoresGroundTruth[sampleIndex][i] << std::endl;

    EXPECT_EQ(scoresGroundTruth[sampleIndex][i], score_value);
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

template<
    unsigned int dataLength,
    unsigned int numberOfInputNeurons,
    unsigned int numberOfOutputNeurons,
    unsigned int maxInputDuration,
    unsigned int maxOutputDuration,
    unsigned int solutionBufferCount
>
struct ScoreTester
{
    typedef ScoreFunction<
        dataLength,
        numberOfInputNeurons, numberOfOutputNeurons,
        maxInputDuration, maxOutputDuration,
        solutionBufferCount
    > ScoreFuncOpt;
    typedef ScoreReferenceImplementation<
        dataLength,
        numberOfInputNeurons, numberOfOutputNeurons,
        maxInputDuration, maxOutputDuration,
        solutionBufferCount
    > ScoreFuncRef;

    ScoreFuncOpt* score;
    ScoreFuncRef* score_ref_impl;

    ScoreTester()
    {
        score = new ScoreFuncOpt;
        score_ref_impl = new ScoreFuncRef;
        memset(score, 0, sizeof(ScoreFuncOpt));
        memset(score_ref_impl, 0, sizeof(ScoreFuncRef));
        EXPECT_TRUE(score->initMemory());
        score_ref_impl->initMemory();
        score->initMiningData(_mm256_setzero_si256());
        score_ref_impl->initMiningData();
    }

    ~ScoreTester()
    {
        delete score;
        delete score_ref_impl;
    }

    bool operator()(const unsigned long long processorNumber, unsigned char* publicKey, unsigned char* nonce)
    {
        int x = 0;
        top_of_stack = (unsigned long long)(&x);
        auto t0 = std::chrono::high_resolution_clock::now();
        unsigned int current = (*score)(processorNumber, publicKey, nonce);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto d = t1 - t0;
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
        std::cout << "Optimized version: " << elapsed.count() << "ns" << std::endl;

        t0 = std::chrono::high_resolution_clock::now();
        unsigned int reference = (*score_ref_impl)(processorNumber, publicKey, nonce);
        t1 = std::chrono::high_resolution_clock::now();
        d = t1 - t0;
        elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
        std::cout << "Reference version: " << elapsed.count() << "ns" << std::endl;
        std::cout << "current score() returns " << current << ", reference score() returns " << reference << std::endl;
        return current == reference;
    }
};

static const std::string COMMON_TEST_SAMPLES_FILE_NAME = "test_data/samples_20240815.csv";
static const std::string COMMON_TEST_SCORES_FILE_NAME = "test_data/scores_20240815.csv";

// Function to read and parse the CSV file
std::vector<std::vector<std::string>> readCSV(const std::string& filename)
{
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    std::string line;

    // Read each line from the file
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> parsedLine;

        // Parse each item separated by commas
        while (std::getline(ss, item, ','))
        {
            parsedLine.push_back(item);
        }
        data.push_back(parsedLine);
    }
    return data;
}

m256i convertm256iFromString(std::string& rStr)
{
    m256i value;
    std::stringstream ss(rStr);
    std::string item;
    int i = 0;
    while (std::getline(ss, item, '-'))
    {
        value.m256i_u64[i++] = std::stoull(item);
    }
    return value;
}

std::vector<unsigned long long> convertULLFromString(std::string& rStr)
{
    std::vector<unsigned long long> values;
    std::stringstream ss(rStr);
    std::string item;
    int i = 0;
    while (std::getline(ss, item, '-'))
    {
        values.push_back(std::stoull(item));
    }
    return values;
}

void runCommonTests()
{
#ifdef __AVX512F__
    initAVX512KangarooTwelveConstants();
#endif

    constexpr unsigned long long numberOfGeneratedSetting = sizeof(score_params::kSettings) / sizeof(score_params::kSettings[0]);
    std::cout << "Testing " << numberOfGeneratedSetting << " param combinations" << std::endl;

    // Read the parameters and results
    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);
    auto scoresString = readCSV(COMMON_TEST_SCORES_FILE_NAME);

    // Convert the raw string and do the data verification
    unsigned long long numberOfSamples = sampleString.size();

    std::vector<m256i> miningSeeds(numberOfSamples);
    std::vector<m256i> publicKeys(numberOfSamples);
    std::vector<m256i> nonces(numberOfSamples);

    // Reading the input samples
    for (unsigned long long i = 0; i < numberOfSamples; ++i)
    {
        miningSeeds[i] = convertm256iFromString(sampleString[i][0]);
        publicKeys[i] = convertm256iFromString(sampleString[i][1]);
        nonces[i] = convertm256iFromString(sampleString[i][2]);
    }

    // Reading the header of score and verification
    auto scoreHeader = scoresString[0];
    EXPECT_EQ(scoreHeader.size(), numberOfGeneratedSetting, "Mismatched the toltal number of param setting and provided input scores");
    if (scoreHeader.size() != numberOfGeneratedSetting)
    {
        return;
    }

    for (unsigned long long i = 0; i < numberOfGeneratedSetting; ++i)
    {
        auto scoresSettingHeader = convertULLFromString(scoreHeader[i]);

        // Check matching between number of parameters types
        if (scoresSettingHeader.size() != score_params::MAX_PARAM_TYPE)
        {
            EXPECT_EQ(scoresSettingHeader.size(), score_params::MAX_PARAM_TYPE, "Mismatched the number of params (NEURONS, DURATION ...) and MAX_PARAM_TYPE");
            return;
        }

        for (unsigned long long j = 0; j < score_params::MAX_PARAM_TYPE; ++j)
        {
            if (scoresSettingHeader[j] != score_params::kSettings[i][j])
            {
                EXPECT_EQ(scoresSettingHeader[j], score_params::kSettings[i][j], "Mismatched the number of params (NEURONS, DURATION ...) and MAX_PARAM_TYPE");
                return;
            }
        }
    }

    // Read the groudtruth scores and init result scores
    scoresGroundTruth.resize(numberOfSamples);
    for (size_t i = 0; i < numberOfSamples; ++i)
    {
       auto scoresStr = scoresString[i + 1];
       size_t scoreSize = scoresStr.size();
       for (size_t j = 0; j < scoreSize; ++j)
       {
           scoresGroundTruth[i].push_back(std::stoi(scoresStr[j]));
       }
    }

    // Run the test
    for (int i = 0; i < numberOfSamples; ++i)
    {
        if (!filteredSamples.empty()
            && std::find(filteredSamples.begin(), filteredSamples.end(), i) == filteredSamples.end())
        {
            continue;
        }
        process<numberOfGeneratedSetting>(miningSeeds[i].m256i_u8, publicKeys[i].m256i_u8, nonces[i].m256i_u8, i);
    }

    // Check  filtered if enable
    if (!filteredSamples.empty())
    {
        EXPECT_EQ(filteredSamples.size(), scoresResult.size());
    }
    if (!filteredSettings.empty())
    {
        for (auto sample : scoresResult)
        {
            EXPECT_EQ(filteredSettings.size(), sample.second.size());
        }
    }

    // Print the average processing time
    for (auto scoreTime : scoreProcessingTime)
    {
        unsigned long long processingTime = filteredSamples.empty() ? scoreTime.second / numberOfSamples : scoreTime.second / filteredSamples.size();
        std::cout << "Avg processing [" << ", NEURON " << kSettings[scoreTime.first][NR_NEURONS]
            << ", DURATIONS " << kSettings[scoreTime.first][DURATIONS]
            << "]: " << processingTime << " ns" << std::endl;
    }
}


TEST(TestQubicScoreFunction, CommonTests) {
    runCommonTests();
}
