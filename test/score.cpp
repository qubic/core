#define NO_UEFI

#include "gtest/gtest.h"

#define ENABLE_PROFILING 1

// current optimized implementation
#include "../src/public_settings.h"
#include "../src/mining/score_engine.h"
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
static const std::string COMMON_TEST_SCORES_HYPERIDENTITY_FILE_NAME = "data/scores_hyperidentity.csv";
static const std::string COMMON_TEST_SCORES_ADDITION_FILE_NAME = "data/scores_addition.csv";

static constexpr bool PRINT_DETAILED_INFO = false;
// Variable control the algo tested
// AllAlgo: run the score that alg is retermined by nonce
static constexpr score_engine::AlgoType TEST_ALGO = 
    static_cast<score_engine::AlgoType>(score_engine::AlgoType::HyperIdentity 
                                        | score_engine::AlgoType::Addition
                                        | score_engine::AlgoType::AllAlgo);
//static constexpr score_engine::AlgoType TEST_ALGO = static_cast<score_engine::AlgoType>(score_engine::AlgoType::HyperIdentity);

// set to 0 for run all available samples
// For profiling enable, run all available samples
static constexpr unsigned long long COMMON_TEST_NUMBER_OF_SAMPLES = 1;
static constexpr unsigned long long PROFILING_NUMBER_OF_SAMPLES = 1;


// set 0 for run maximum number of threads of the computer.
// For profiling enable, set it equal to deployment setting
static constexpr int MAX_NUMBER_OF_THREADS = 0;
static constexpr int MAX_NUMBER_OF_PROFILING_THREADS = 12;
static bool gCompareReference = false;

// Only run on specific index of samples and setting
std::vector<unsigned int> filteredSamples;// = { 0 };
std::vector<unsigned int> filteredSettings;// = { 0 };

std::vector<std::vector<unsigned int>> gScoresHyperIdentityGroundTruth;
std::vector<std::vector<unsigned int>> gScoresHyperAdditionGroundTruth;
std::map<unsigned int, unsigned long long> gScoreProcessingTime;
std::map<int, int> gScoreHyperIdentityIndexMap;
std::map<int, int> gScoreAdditionIndexMap;

struct ScoreResult
{
    unsigned int score;
    long long elapsedMs;
};

template<template<typename, typename> class ScoreType, typename CurrentConfig>
ScoreResult computeScore(
    const unsigned char* miningSeed,
    const unsigned char* publicKey,
    const unsigned char* nonce,
    score_engine::AlgoType algo,
    unsigned char* externalRandomPool)
{
    std::unique_ptr<ScoreType< typename CurrentConfig::HyperIdentity,typename CurrentConfig::Addition>> scoreEngine = 
        std::make_unique<ScoreType<typename CurrentConfig::HyperIdentity, typename CurrentConfig::Addition >>();

    scoreEngine->initMemory();
    if (nullptr == externalRandomPool)
    {
        scoreEngine->initMiningData(miningSeed);
    }

    unsigned int scoreValue = 0;

    auto t0 = std::chrono::high_resolution_clock::now();

    switch (algo)
    {
    case score_engine::AlgoType::HyperIdentity:
        scoreValue = scoreEngine->computeHyperIdentityScore(publicKey, nonce, externalRandomPool);
        break;
    case score_engine::AlgoType::Addition:
        scoreValue = scoreEngine->computeAdditionScore(publicKey, nonce, externalRandomPool);
        break;
    default:
        scoreValue = scoreEngine->computeScore(publicKey, nonce, externalRandomPool);
        break;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    return { scoreValue, elapsedMs };

}

void processQubicScore(const unsigned char* miningSeed, const unsigned char* publicKey, const unsigned char* nonce, int sampleIndex)
{
    // Core use the external random pool
    std::unique_ptr<ScoreFunction<1>> pScore = std::make_unique<ScoreFunction<1>>();
    pScore->initMemory();
    pScore->initMiningData(miningSeed);

    unsigned int scoreValue = (*pScore)(0, publicKey, miningSeed, nonce);

    // Determine which algo to use to get the correct ground truth
    int gtIndex = -1;
    unsigned int gtScore = 0;
    score_engine::AlgoType effectiveAlgo = (nonce[0] & 1) == 0 ? score_engine::AlgoType::HyperIdentity : score_engine::AlgoType::Addition;

    std::vector<unsigned char> state(score_engine::STATE_SIZE);
    std::vector<unsigned char> externalPoolVec(score_engine::POOL_VEC_PADDING_SIZE);
    score_engine::generateRandom2Pool(miningSeed, state.data(), externalPoolVec.data());
    ScoreResult scoreResult = computeScore<score_engine::ScoreEngine,
        ConfigPair<
            score_engine::HyperIdentityParams<
            HYPERIDENTITY_NUMBER_OF_INPUT_NEURONS,
            HYPERIDENTITY_NUMBER_OF_OUTPUT_NEURONS,
            HYPERIDENTITY_NUMBER_OF_TICKS,
            HYPERIDENTITY_NUMBER_OF_NEIGHBORS,
            HYPERIDENTITY_POPULATION_THRESHOLD,
            HYPERIDENTITY_NUMBER_OF_MUTATIONS,
            HYPERIDENTITY_SOLUTION_THRESHOLD_DEFAULT>,
            score_engine::AdditionParams<
            ADDITION_NUMBER_OF_INPUT_NEURONS,
            ADDITION_NUMBER_OF_OUTPUT_NEURONS,
            ADDITION_NUMBER_OF_TICKS,
            ADDITION_NUMBER_OF_NEIGHBORS,
            ADDITION_POPULATION_THRESHOLD,
            ADDITION_NUMBER_OF_MUTATIONS,
            ADDITION_SOLUTION_THRESHOLD_DEFAULT>
        >>(
        miningSeed, publicKey, nonce, effectiveAlgo, externalPoolVec.data());
    unsigned int refScore = scoreResult.score;

#pragma omp critical
    {
        EXPECT_EQ(refScore, scoreValue);
    }
}


template <unsigned long long i>
static void processElement(const unsigned char* miningSeed, const unsigned char* publicKey, const unsigned char* nonce, 
    int sampleIndex, score_engine::AlgoType algo)
{
    // Skip filter settings
    if (!filteredSettings.empty()
        && std::find(filteredSettings.begin(), filteredSettings.end(), i) == filteredSettings.end())
    {
        return;
    }

    // Get the current config
    using CurrentConfig = std::tuple_element_t<i, ConfigList>;
    
    // Core use the external random pool
    std::vector<unsigned char> state(score_engine::STATE_SIZE);
    std::vector<unsigned char> externalPoolVec(score_engine::POOL_VEC_PADDING_SIZE);
    score_engine::generateRandom2Pool(miningSeed, state.data(), externalPoolVec.data());
    ScoreResult scoreResult = computeScore<score_engine::ScoreEngine, CurrentConfig>(
        miningSeed, publicKey, nonce, algo, externalPoolVec.data());
    unsigned int scoreValue = scoreResult.score;

    // Determine which algo to use to get the correct ground truth
    int gtIndex = -1;
    unsigned int gtScore = 0;
    score_engine::AlgoType effectiveAlgo = algo;
    if (algo != score_engine::AlgoType::HyperIdentity && algo != score_engine::AlgoType::Addition)
    {
        // Default/Mixed mode: select based on nonce
        effectiveAlgo = (nonce[0] & 1) == 0 ? score_engine::AlgoType::HyperIdentity : score_engine::AlgoType::Addition;
    }
    if (effectiveAlgo == score_engine::AlgoType::HyperIdentity)
    {
        if (gScoreHyperIdentityIndexMap.count(i) > 0)
        {
            gtIndex = gScoreHyperIdentityIndexMap[i];
            gtScore = gScoresHyperIdentityGroundTruth[sampleIndex][gtIndex];
        }
    }
    else if (effectiveAlgo == score_engine::AlgoType::Addition)
    {
        if (gScoreAdditionIndexMap.count(i) > 0)
        {
            gtIndex = gScoreAdditionIndexMap[i];
            gtScore = gScoresHyperAdditionGroundTruth[sampleIndex][gtIndex];
        }
    }

    unsigned int refScore = 0;
    if (gCompareReference)
    {
        // Reference score always re-compute the pools
        ScoreResult scoreRefResult = computeScore<score_reference::ScoreReferenceImplementation, CurrentConfig>(
            miningSeed, publicKey, nonce, algo, nullptr);
        refScore = scoreRefResult.score;
    }

#pragma omp critical
    if (gCompareReference)
    {
        EXPECT_EQ(refScore, scoreValue);
    }
    else
    {
        if (PRINT_DETAILED_INFO || gtIndex < 0 || (scoreValue != gtScore))
        {
            std::cout << "    score " << scoreValue;
            if (gtIndex >= 0)
            {
                std::cout << " vs gt " << gtScore << std::endl;
            }
            else // No mapping from ground truth
            {
                std::cout << " vs gt NA" << std::endl;
            }
        }
        {
            EXPECT_GE(gtIndex, 0);
            if (gtIndex >= 0)
            {
                EXPECT_EQ(gtScore, scoreValue);
            }
        }
    }
}

// Recursive template to process each element in scoreSettings
template <unsigned long long i>
static void processElementPerf(const unsigned char* miningSeed, const unsigned char* publicKey, const  unsigned char* nonce, 
    int sampleIndex, score_engine::AlgoType algo)
{
    using CurrentConfig = std::tuple_element_t<i, ProfileConfigList>;

    std::vector<unsigned char> state(score_engine::STATE_SIZE);
    std::vector<unsigned char> externalPoolVec(score_engine::POOL_VEC_PADDING_SIZE);
    score_engine::generateRandom2Pool(miningSeed, state.data(), externalPoolVec.data());
    ScoreResult scoreRefResult = computeScore<score_engine::ScoreEngine, CurrentConfig>(
        miningSeed, publicKey, nonce, algo, externalPoolVec.data());
    auto elapsed = scoreRefResult.elapsedMs;
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
static void processHelper(const unsigned char* miningSeed, const unsigned char* publicKey, const unsigned char* nonce, 
    int sampleIndex, score_engine::AlgoType algo, std::index_sequence<Is...>)
{
    if constexpr (profiling)
    {
        (processElementPerf<Is>(miningSeed, publicKey, nonce, sampleIndex, algo), ...);
    }
    else
    {
        (processElement<Is>(miningSeed, publicKey, nonce, sampleIndex, algo), ...);
    }
    
}

// Recursive template to process each element in scoreSettings
template <char profiling, unsigned long long N>
static void process(const unsigned char* miningSeed, const  unsigned char* publicKey, const unsigned char* nonce, 
    int sampleIndex = 0, score_engine::AlgoType algo = score_engine::AlgoType::AllAlgo)
{
    processHelper<profiling, N>(miningSeed, publicKey, nonce, sampleIndex, algo, std::make_index_sequence<N>{});
}

template<typename P>
void writeParams(std::ostream& os, const std::string& sep = ",")
{
    // Because currently 2 params set shared the same things, incase of new algo have different params
    // need to make a separate check
    if constexpr (P::algoType == score_engine::AlgoType::HyperIdentity)
    {
        os  << "InputNeurons: " << P::numberOfInputNeurons << sep
            << " OutputNeurons: " << P::numberOfOutputNeurons << sep
            << " Ticks: " << P::numberOfTicks << sep
            << " Neighbor: " << P::numberOfNeighbors << sep
            << " Population: " << P::populationThreshold << sep
            << " Mutate: " << P::numberOfMutations << sep
            << " Threshold: " << P::solutionThreshold;
    }
    else if constexpr (P::algoType == score_engine::AlgoType::Addition)
    {
         os << "InputNeurons: " << P::numberOfInputNeurons << sep
            << " OutputNeurons: " << P::numberOfOutputNeurons << sep
            << " Ticks: " << P::numberOfTicks << sep
            << " Neighbor: " << P::numberOfNeighbors << sep
            << " Population: " << P::populationThreshold << sep
            << " Mutate: " << P::numberOfMutations << sep
            << " Threshold: " << P::solutionThreshold;
    }
    else
    {
        std::cerr << "UNKNOWN ALGO !" << std::endl;
    }
}

template<std::size_t I>
void printConfigProfileImpl(score_engine::AlgoType algo)
{
    using CurrentConfig = std::tuple_element_t<I, ProfileConfigList>;

    if (algo & score_engine::AlgoType::HyperIdentity)
    {
        writeParams<typename CurrentConfig::HyperIdentity>(std::cout);
    }
    else if (algo & score_engine::AlgoType::Addition)
    {
        writeParams<typename CurrentConfig::Addition>(std::cout);
    }
}

template<std::size_t I>
void printConfigImpl(score_engine::AlgoType algo)
{
    using CurrentConfig = std::tuple_element_t<I, ConfigList>;

    if (algo & score_engine::AlgoType::HyperIdentity)
    {
        writeParams<typename CurrentConfig::HyperIdentity>(std::cout);
    }
    else if (algo & score_engine::AlgoType::Addition)
    {
        writeParams<typename CurrentConfig::Addition>(std::cout);
    }
}

template<char profiling, std::size_t... Is>
void printConfigByIndex(std::size_t index, score_engine::AlgoType algo, std::index_sequence<Is...>)
{
    if constexpr (profiling)
    {
        ((Is == index ? (printConfigProfileImpl<Is>(algo), 0) : 0), ...);
    }
    else
    {
        ((Is == index ? (printConfigImpl<Is>(algo), 0) : 0), ...);
    }
}

template<char profiling>
void printConfig(std::size_t index, score_engine::AlgoType algo)
{
    if constexpr (profiling)
    {
        printConfigByIndex<profiling>(index, algo, std::make_index_sequence<std::tuple_size_v<ProfileConfigList>>{});
    }
    else
    {
        printConfigByIndex<profiling>(index, algo, std::make_index_sequence<std::tuple_size_v<ConfigList>>{});
    }
}

template<typename P>
bool compareParams(const std::vector<unsigned long long>& values)
{
    // Because currently 2 params set shared the same things, incase of new algo have different params
    // need to make a separate check
    if constexpr (P::algoType == score_engine::AlgoType::HyperIdentity)
    {
        return values[0] == P::numberOfInputNeurons
            && values[1] == P::numberOfOutputNeurons
            && values[2] == P::numberOfTicks
            && values[3] == P::numberOfNeighbors
            && values[4] == P::populationThreshold
            && values[5] == P::numberOfMutations
            && values[6] == P::solutionThreshold;
    }
    else if constexpr (P::algoType == score_engine::AlgoType::Addition)
    {
        return values[0] == P::numberOfInputNeurons
            && values[1] == P::numberOfOutputNeurons
            && values[2] == P::numberOfTicks
            && values[3] == P::numberOfNeighbors
            && values[4] == P::populationThreshold
            && values[5] == P::numberOfMutations
            && values[6] == P::solutionThreshold;
    }
    return false;
}

template<std::size_t I>
bool checkConfig(const std::vector<unsigned long long>& values, score_engine::AlgoType algo)
{
    using CurrentConfig = std::tuple_element_t<I, ConfigList>;
    switch (algo)
    {
    case score_engine::AlgoType::HyperIdentity:
        // HyperIdentity
        return compareParams<typename CurrentConfig::HyperIdentity>(values);
        break;
    case score_engine::AlgoType::Addition:
        // Addition
        return compareParams<typename CurrentConfig::Addition>(values);
        break;
    default:
        return false;
        break;
    }
}

template<std::size_t... Is>
int findMatchingConfigImpl(const std::vector<unsigned long long>& values, score_engine::AlgoType algo, std::index_sequence<Is...>)
{
    int result = -1;
    ((checkConfig<Is>(values, algo) ? (result = Is, false) : true) && ...);
    return result;
}

int findMatchingConfig(const std::vector<unsigned long long>& values, score_engine::AlgoType algo)
{
    return findMatchingConfigImpl(values, algo, std::make_index_sequence<std::tuple_size_v<ConfigList>>{});
}

void loadSamples(
    const std::vector<std::vector<std::string>>& sampleString,
    unsigned long long numberOfSamples,
    unsigned long long numberOfSamplesReadFromFile,
    std::vector<m256i>& miningSeeds,
    std::vector<m256i>& publicKeys,
    std::vector<m256i>& nonces)
{
    miningSeeds.resize(numberOfSamples);
    publicKeys.resize(numberOfSamples);
    nonces.resize(numberOfSamples);

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
}

template<typename ProcessFunc>
void runTest(
    const std::string& testName,
    const std::vector<int>& samples,
    int numberOfThreads,
    ProcessFunc processFunc)
{
    std::cout << "Test " << testName << " ... " << std::endl;
#pragma omp parallel for num_threads(numberOfThreads)
    for (int i = 0; i < static_cast<int>(samples.size()); ++i)
    {
        int index = samples[i];
        processFunc(index);
#pragma omp critical
        std::cout << i << ", ";
    }
    std::cout << std::endl;
}

void runCommonTests()
{

#if defined (__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
#endif
    constexpr unsigned long long numberOfGeneratedSetting = CONFIG_COUNT;

    // Read the parameters and results
    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);

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

    // Reading the input samples
    std::vector<m256i> miningSeeds(numberOfSamples);
    std::vector<m256i> publicKeys(numberOfSamples);
    std::vector<m256i> nonces(numberOfSamples);
    loadSamples(sampleString, numberOfSamples, numberOfSamplesReadFromFile, miningSeeds, publicKeys, nonces);

    // Reading the header of score and verification
    if (!gCompareReference)
    {
        auto scoresStringHyperidentity = readCSV(COMMON_TEST_SCORES_HYPERIDENTITY_FILE_NAME);
        auto scoresStringAddition = readCSV(COMMON_TEST_SCORES_ADDITION_FILE_NAME);

        if (scoresStringAddition.size() == 0 ||  scoresStringAddition.size() == 0)
        {
            ASSERT_GT(scoresStringHyperidentity.size(), 0);
            ASSERT_GT(scoresStringAddition.size(), 0);
            std::cout << "Number of Hyperidentity and Addition settings must greater than zero." << std::endl;
            return;
        }
        if (scoresStringAddition.size() != scoresStringAddition.size())
        {
            ASSERT_EQ(scoresStringHyperidentity.size(), scoresStringAddition.size());
            std::cout << "Number of Hyperidentity and Addition settings must be equal." << std::endl;
            return;
        }

        // 
        auto buildIndexMap = [](
            std::vector<std::string>& header,
            score_engine::AlgoType algo,
            std::map<int, int>& indexMap)
            {
                for (int gtIdx = 0; gtIdx < (int)header.size(); ++gtIdx)
                {
                    auto scoresSettingHeader = convertULLFromString(header[gtIdx]);
                    int foundIndex = findMatchingConfig(scoresSettingHeader, algo);
                    if (foundIndex >= 0)
                    {
                        indexMap[foundIndex] = gtIdx;
                    }
                }
            };
        buildIndexMap(scoresStringHyperidentity[0], score_engine::AlgoType::HyperIdentity, gScoreHyperIdentityIndexMap);
        buildIndexMap(scoresStringAddition[0], score_engine::AlgoType::Addition, gScoreAdditionIndexMap);

        if (gScoreHyperIdentityIndexMap.size() != gScoreHyperIdentityIndexMap.size())
        {
            ASSERT_EQ(gScoreHyperIdentityIndexMap.size(), gScoreHyperIdentityIndexMap.size());
            std::cout << "Number of tested Hyperidentity and Addition must be equal." << std::endl;
            return;
        }
        
        std::cout << "Testing " << CONFIG_COUNT << " param combinations on " << scoresStringHyperidentity[0].size() << " Hyperidentity and Addition ground truth settings." << std::endl;
        // In case of number of setting is lower than the ground truth. Consider we are in experiement, still run but expect the test failed
        if (gScoreHyperIdentityIndexMap.size() < CONFIG_COUNT)
        {
            std::cout << "WARNING: Number of provided ground truth settings is lower than tested settings. Only test with available ones."
                << std::endl;
            EXPECT_EQ(gScoreHyperIdentityIndexMap.size(), CONFIG_COUNT);
        }

        auto loadGroundTruth = [](
            const std::vector<std::vector<std::string>>& scoresString,
            std::vector<std::vector<unsigned int>>& groundTruth,
            int numberOfSamples) -> int
            {
                int numberOfGTSetting = (int)scoresString.size() - 1;
                numberOfSamples = std::min(numberOfSamples, numberOfGTSetting);
                groundTruth.resize(numberOfSamples);

                for (size_t i = 0; i < numberOfSamples; ++i)
                {
                    auto& scoresStr = scoresString[i + 1];
                    for (const auto& str : scoresStr)
                    {
                        groundTruth[i].push_back(std::stoi(str));
                    }
                }

                return numberOfSamples;
            };

        int numHI = loadGroundTruth(scoresStringHyperidentity, gScoresHyperIdentityGroundTruth, (int)numberOfSamples);
        int numAdd = loadGroundTruth(scoresStringAddition, gScoresHyperAdditionGroundTruth, (int)numberOfSamples);

        std::cout << "There are " << numHI << " Hyperidentity results and " << numAdd << " Addition results." << std::endl;
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
    std::vector<std::pair<score_engine::AlgoType, std::string>> algos = {
    {score_engine::AlgoType::HyperIdentity, "HyperIdentity"},
    {score_engine::AlgoType::Addition, "Addition"},
    {score_engine::AlgoType::AllAlgo, "Mixed"}
    };

    for (const auto& [algoType, algoName] : algos)
    {
        if (TEST_ALGO & algoType)
        {
            runTest(algoName, samples, numberOfThreads, [&](int index)
            {
                process<0, CONFIG_COUNT>(
                    miningSeeds[index].m256i_u8,
                    publicKeys[index].m256i_u8,
                    nonces[index].m256i_u8,
                    index,
                    algoType);
            });
        }
    }

    // Test Qubic score vs internal engine (always runs)
    runTest("Qubic's score vs internal score engine on active config", samples, numberOfThreads, [&](int index)
    {
        processQubicScore(
            miningSeeds[index].m256i_u8,
            publicKeys[index].m256i_u8,
            nonces[index].m256i_u8,
            index);
    });

}

template<int Profiling, std::size_t ConfigCount>
void profileAlgo(
    score_engine::AlgoType algo,
    const std::string& algoName,
    const std::vector<int>& samples,
    const std::vector<m256i>& miningSeeds,
    const std::vector<m256i>& publicKeys,
    const std::vector<m256i>& nonces,
    const std::vector<unsigned int>& filteredSamples,
    int numberOfThreads,
    unsigned long long numberOfSamples)
{
    std::cout << "Profile " << algoName << " ... " << std::endl;
    gScoreProcessingTime.clear();

    int numSamples = static_cast<int>(samples.size());

#pragma omp parallel for num_threads(numberOfThreads)
    for (int i = 0; i < numSamples; ++i)
    {
        int index = samples[i];
        process<Profiling, ConfigCount>(
            miningSeeds[index].m256i_u8,
            publicKeys[index].m256i_u8,
            nonces[index].m256i_u8,
            index,
            algo);
#pragma omp critical
        std::cout << i << ", ";
    }
    std::cout << std::endl;

    std::size_t sampleCount = filteredSamples.empty() ? numberOfSamples : filteredSamples.size();
    for (const auto& [settingIndex, totalTime] : gScoreProcessingTime)
    {
        unsigned long long avgTime = totalTime / sampleCount;
        std::cout << "Avg time [setting " << settingIndex << "]: ";
        printConfig<Profiling>(settingIndex, algo);
        std::cout << " - " << avgTime << " ms" << std::endl;
    }
}

void runPerformanceTests()
{
#if defined (__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
#endif
    constexpr unsigned long long numberOfGeneratedSetting = PROFILE_CONFIG_COUNT;

    // Read the parameters and results
    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);

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

    // Loading samples
    std::vector<m256i> miningSeeds(numberOfSamples);
    std::vector<m256i> publicKeys(numberOfSamples);
    std::vector<m256i> nonces(numberOfSamples);
    loadSamples(sampleString, numberOfSamples, numberOfSamplesReadFromFile, miningSeeds, publicKeys, nonces);

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

    profileAlgo<1, PROFILE_CONFIG_COUNT>(
        score_engine::AlgoType::HyperIdentity, "HyperIdentity",
        samples, miningSeeds, publicKeys, nonces, filteredSamples, numberOfThreads, numberOfSamples);

    profileAlgo<1, PROFILE_CONFIG_COUNT>(
        score_engine::AlgoType::Addition, "Addition",
        samples, miningSeeds, publicKeys, nonces, filteredSamples, numberOfThreads, numberOfSamples);

    //gProfilingDataCollector.writeToFile();
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

//#if not ENABLE_PROFILING
//TEST(TestQubicScoreFunction, TestDeterministic)
//{
//    constexpr int NUMBER_OF_THREADS = 4;
//    constexpr int NUMBER_OF_PHASES = 2;
//    constexpr int NUMBER_OF_SAMPLES = 4;
//
//    // Read the parameters and results
//    auto sampleString = readCSV(COMMON_TEST_SAMPLES_FILE_NAME);
//    ASSERT_FALSE(sampleString.empty());
//
//    // Convert the raw string and do the data verification
//    unsigned long long numberOfSamples = sampleString.size();
//    if (COMMON_TEST_NUMBER_OF_SAMPLES > 0)
//    {
//        numberOfSamples = std::min(COMMON_TEST_NUMBER_OF_SAMPLES, numberOfSamples);
//    }
//
//    std::vector<m256i> miningSeeds(numberOfSamples);
//    std::vector<m256i> publicKeys(numberOfSamples);
//    std::vector<m256i> nonces(numberOfSamples);
//
//    // Reading the input samples
//    for (unsigned long long i = 0; i < numberOfSamples; ++i)
//    {
//        miningSeeds[i] = hexTo32Bytes(sampleString[i][0], 32);
//        publicKeys[i] = hexTo32Bytes(sampleString[i][1], 32);
//        nonces[i] = hexTo32Bytes(sampleString[i][2], 32);
//    }
//
//    auto pScore = std::make_unique<ScoreFunction<
//        ::NUMBER_OF_INPUT_NEURONS,
//        ::NUMBER_OF_OUTPUT_NEURONS,
//        ::NUMBER_OF_TICKS,
//        ::NUMBER_OF_NEIGHBORS,
//        ::POPULATION_THRESHOLD,
//        ::NUMBER_OF_MUTATIONS,
//        ::SOLUTION_THRESHOLD,
//        NUMBER_OF_THREADS
//        >>();
//    pScore->initMemory();
//
//    // Run with 4 mining seeds, each run 4 separate threads and the result need to matched
//    int scores[NUMBER_OF_PHASES][NUMBER_OF_THREADS * NUMBER_OF_SAMPLES] = { 0 };
//    for (unsigned long long i = 0; i < NUMBER_OF_PHASES; ++i)
//    {
//        pScore->initMiningData(miningSeeds[i]);
//
//#pragma omp parallel for num_threads(NUMBER_OF_THREADS)
//        for (int threadId = 0; threadId < NUMBER_OF_THREADS; ++threadId)
//        {
//            if (threadId % 2 == 0)
//            {
//                for (int sampleId = 0; sampleId < NUMBER_OF_SAMPLES; ++sampleId)
//                {
//                    scores[i][threadId * NUMBER_OF_SAMPLES + sampleId] = (*pScore)(threadId, publicKeys[sampleId], miningSeeds[i], nonces[sampleId]);
//                }
//            }
//            else
//            {
//                for (int sampleId = NUMBER_OF_SAMPLES - 1; sampleId >= 0; --sampleId)
//                {
//                    scores[i][threadId * NUMBER_OF_SAMPLES + sampleId] = (*pScore)(threadId, publicKeys[sampleId], miningSeeds[i], nonces[sampleId]);
//                }
//            }
//        }
//    }
//
//    // Each threads run with the same samples but the order is reversed. Expect the scores are matched.
//    for (unsigned long long i = 0; i < NUMBER_OF_PHASES; ++i)
//    {
//        for (int threadId = 0; threadId < NUMBER_OF_THREADS - 1; ++threadId)
//        {
//            for (int sampleId = 0; sampleId < NUMBER_OF_SAMPLES; ++sampleId)
//            {
//                EXPECT_EQ(scores[i][threadId * NUMBER_OF_SAMPLES + sampleId], scores[i][(threadId + 1) * NUMBER_OF_SAMPLES + sampleId]);
//            }
//        }
//    }
//}
//#endif
