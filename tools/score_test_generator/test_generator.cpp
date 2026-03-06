#define NO_UEFI

#include <iostream>
#include <assert.h>



#include "../../src/public_settings.h"
#include "../../src/platform/m256.h"
#include "../../test/score_params.h"
#include "../../test/score_reference.h"
#include "../../test/utils.h"

#include <vector>
#include <thread>
#include <tuple>
#include <sstream>
#include <omp.h>

using namespace score_params;
using namespace test_utils;

EFI_TIME utcTime;

constexpr unsigned int kDefaultTotalSamples = 32;
std::vector<m256i> miningSeeds;
std::vector<m256i> publicKeys;
std::vector<m256i> nonces;
std::vector<std::vector<unsigned int>> scoreResults;
std::vector<std::vector<unsigned long long>> scoreProcessingTimes;
unsigned int processedSamplesCount = 0;
int gSelectedAlgorithm = 0;

template<typename P>
void writeParamsHeader(std::ostream& os, const std::string& sep = "-")
{
    // Because currently 2 params set shared the same things, incase of new algo have different params
    // need to make a separate check
    if constexpr (P::algoType == score_engine::AlgoType::HyperIdentity)
    {
        os << P::numberOfInputNeurons << sep
            << P::numberOfOutputNeurons << sep
            << P::numberOfTicks << sep
            << P::numberOfNeighbors << sep
            << P::populationThreshold << sep
            << P::numberOfMutations << sep
            << P::solutionThreshold;
    }
    else if constexpr (P::algoType == score_engine::AlgoType::Addition)
    {
        os << P::numberOfInputNeurons << sep
            << P::numberOfOutputNeurons << sep
            << P::numberOfTicks << sep
            << P::numberOfNeighbors << sep
            << P::populationThreshold << sep
            << P::numberOfMutations << sep
            << P::solutionThreshold;
    }
    else
    {
        std::cerr << "UNKNOWN ALGO !" << std::endl;
    }
}



template<std::size_t... Is>
void writeConfigs(std::ostream& oFile, std::index_sequence<Is...>)
{
    constexpr std::size_t N = sizeof...(Is);

    switch (gSelectedAlgorithm)
    {
    case 0:
        // HyperIdentity
        ((writeParamsHeader<typename std::tuple_element_t<Is, ConfigList>::HyperIdentity>(oFile),
            (Is < N - 1 ? (oFile << ", ", 0) : 0)), ...);
        break;
    case 1:
        // Addition
        ((writeParamsHeader<typename std::tuple_element_t<Is, ConfigList>::Addition>(oFile),
            (Is < N - 1 ? (oFile << ", ", 0) : 0)), ...);
        break;
    default:
        break;
    }
}

// Recursive template to process each element in scoreSettings
template <unsigned long long i>
static void processElement(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int threadId, bool writeFile)
{
    using CurrentConfig = std::tuple_element_t<i, ConfigList>;
    score_reference::ScoreReferenceImplementation <
        typename CurrentConfig::HyperIdentity,
        typename CurrentConfig::Addition
        > score;

    score.initMemory();
    score.initMiningData(miningSeed);

    auto t0 = std::chrono::high_resolution_clock::now();

    unsigned int score_value = 0;
    
    switch (gSelectedAlgorithm)
    {
        case 0:
            score_value = score.computeHyperIdentityScore(publicKey, nonce);
            break;
        case 1:
            score_value = score.computeAdditionScore(publicKey, nonce);
            break;
        default:
            score_value = 0;
            break;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    auto d = t1 - t0;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(d);
    scoreResults[threadId][i] = score_value;
    scoreProcessingTimes[threadId][i] = elapsed.count();
}

// Main processing function
template <unsigned long long N, unsigned long long... Is>
static void processHelper(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int threadId, bool writeFile, std::index_sequence<Is...>) {
    (processElement<Is>(miningSeed, publicKey, nonce, threadId, writeFile), ...);
}

// Recursive template to process each element in scoreSettings
template <unsigned long long N>
static void process(unsigned char* miningSeed, unsigned char* publicKey, unsigned char* nonce, int threadId = 0, bool writeFile = true) {
    processHelper<N>(miningSeed, publicKey, nonce, threadId, writeFile, std::make_index_sequence<N>{});
}

int generateSamples(std::string sampleFileName, unsigned int numberOfSamples, bool initMiningZeros = false)
{
    // Generate a list of samples
    if (numberOfSamples > 0)
    {
        // Open the sample file if there is any
        if (sampleFileName.empty())
        {
            std::cerr << "Sample file name is empty. Exit!";
            return 1;
        }

        std::cout << "Generating sample file " << sampleFileName << " ..." << std::endl;
        miningSeeds.resize(numberOfSamples);
        publicKeys.resize(numberOfSamples);
        nonces.resize(numberOfSamples);
        for (unsigned int i = 0; i < numberOfSamples; i++)
        {
            publicKeys[i].setRandomValue();
            nonces[i].setRandomValue();
            if (initMiningZeros)
            {
                memset(miningSeeds[i].m256i_u8, 0, 32);
            }
            else
            {
                miningSeeds[i].setRandomValue();
            }
        }

        std::ofstream sampleFile;
        sampleFile.open(sampleFileName);
        if (!sampleFile.is_open())
        {
            std::cerr << "Open file " << sampleFileName << " failed. Exit!";
            return 1;
        }

        // Write the input to file
        for (unsigned int i = 0; i < numberOfSamples; i++)
        {
            auto miningSeedHexStr = byteToHex(miningSeeds[i].m256i_u8, 32);
            auto publicKeyHexStr = byteToHex(publicKeys[i].m256i_u8, 32);
            auto nonceHexStr = byteToHex(nonces[i].m256i_u8, 32);
            sampleFile
                << miningSeedHexStr << ", "
                << publicKeyHexStr << ", "
                << nonceHexStr << std::endl;
        }
        if (sampleFile.is_open())
        {
            sampleFile.close();
        }
        std::cout << "Generated sample file DONE " << std::endl;
    }
    else // Read the samples from file
    {
        std::cout << "Reading sample file " << sampleFileName << " ..." << std::endl;
        // Open the sample file if there is any
        if (!std::filesystem::exists(sampleFileName))
        {
            std::cerr << "Sample file name is not existed. Exit!";
            return 1;
        }

        auto sampleString = readCSV(sampleFileName);
        unsigned long long totalSamples = sampleString.size();
        std::cout << "There are " << totalSamples << " samples " << std::endl;

        miningSeeds.resize(totalSamples);
        publicKeys.resize(totalSamples);
        nonces.resize(totalSamples);
        for (auto i = 0; i < totalSamples; i++)
        {
            if (sampleString[i].size() != 3)
            {
                std::cout << "Number of elements is mismatched. " << sampleString[i].size() << " vs 3" << " Exiting..." << std::endl;
                return 1;
            }
            if (initMiningZeros)
            {
                memset(miningSeeds[i].m256i_u8, 0, 32);
            }
            else
            {
                hexToByte(sampleString[i][0], 32, miningSeeds[i].m256i_u8);
            }

            hexToByte(sampleString[i][1], 32, publicKeys[i].m256i_u8);
            hexToByte(sampleString[i][2], 32, nonces[i].m256i_u8);
        }
        std::cout << "Read sample file DONE " << std::endl;
    }
    return 0;
}

void generateScore(
    std::string sampleFileName,
    std::string outputFile,
    unsigned int threadsCount,
    unsigned int numberOfSamples,
    bool initMiningZeros = false)
{
    int sts = 0;

    // Generate samples
    sts = generateSamples(sampleFileName, numberOfSamples, initMiningZeros);
    if (sts)
    {
        return;
    }

    // Check if the ouput file name is not empty
    if (outputFile.empty())
    {
        std::cout << "Empty output file exiting..." << std::endl;
        return;
    }

    // Write the headers for output score file
    std::ofstream scoreFile;
    scoreFile.open(outputFile);
    if (!scoreFile.is_open())
    {
        std::cerr << "Open file " << outputFile << " failed. Exit!";
        return;
    }

    // Number of params settings
    constexpr unsigned long long numberOfGeneratedSetting = CONFIG_COUNT;

    // Write the header config
    writeConfigs(scoreFile, std::make_index_sequence<std::tuple_size_v<ConfigList>>{});
    scoreFile << std::endl;
    if (scoreFile.is_open())
    {
        scoreFile.close();
    }

    // Prepare memory for generated scores
    unsigned long long totalSamples = nonces.size();
    scoreResults.resize(totalSamples);
    scoreProcessingTimes.resize(totalSamples);

    bool writeFilePerSample = false;
#pragma omp parallel for num_threads(threadsCount)
    for (int i = 0; i < totalSamples; ++i)
    {
        scoreResults[i].resize(numberOfGeneratedSetting);
        scoreProcessingTimes[i].resize(numberOfGeneratedSetting);
        if (writeFilePerSample)
        {
            std::string fileName = "score_" + std::to_string(i) + ".txt";
            std::ofstream output_file(fileName);
            if (output_file.is_open())
            {
                output_file.close();
            }
        }
        process<numberOfGeneratedSetting>(miningSeeds[i].m256i_u8, publicKeys[i].m256i_u8, nonces[i].m256i_u8, i, writeFilePerSample);
#pragma omp critical
        {
            processedSamplesCount++;
            if (processedSamplesCount % 16 == 0)
            {
                std::cout << "\rProcessed  " << processedSamplesCount << " / " << totalSamples;
            }
        }
    }

    // Write to a general file
    std::cout << "\nGenerate scores DONE. Collect all into a file..." << std::endl;
    scoreFile.open(outputFile, std::ios::app);
    if (!scoreFile.is_open())
    {
        return;
    }
    for (int i = 0; i < totalSamples; i++)
    {
        for (int j = 0; j < numberOfGeneratedSetting; j++)
        {
            scoreFile << scoreResults[i][j];
            if (j < numberOfGeneratedSetting - 1)
            {
                scoreFile << ", ";
            }
        }
        scoreFile << std::endl;
    }
    scoreFile.close();
}

void print_random_test_case()
{
    // generate random input data
    m256i nonce;
    nonce.setRandomValue();
    m256i publicKey;
    publicKey.setRandomValue();
    unsigned long long processor_number;
    _rdrand64_step(&processor_number);
    processor_number %= 1024;

    std::cout << "EXPECT_TRUE(test_score("
        << processor_number << ", "
        << "m256i(" << publicKey.m256i_u64[0] << "ULL, " << publicKey.m256i_u64[1] << "ULL, " << publicKey.m256i_u64[2] << "ULL, " << publicKey.m256i_u64[3] << "ULL).m256i_u8, "
        << "m256i(" << nonce.m256i_u64[0] << "ULL, " << nonce.m256i_u64[1] << "ULL, " << nonce.m256i_u64[2] << "ULL, " << nonce.m256i_u64[3] << "ULL).m256i_u8));" << std::endl;
}

void printHelp()
{
    std::cout << "Usage: program [options]\n";
    std::cout << "--help, -h  Show this help message\n";
    std::cout << "--mode, -m <mode>                         Available mode: unittest, generator\n";
    std::cout << "                                            unittest: print the random unitest code that can be passed into score unittest \n";
    std::cout << "                                            generator: generate grouthtruth file in csv format that use for score unittest \n";
    std::cout << "--samplefile, -s <filename>              [generator] Sample file \n";
    std::cout << "--numsamples, -n <number>                [generator] Number of samples,  \n";
    std::cout << "                                              zeros/unset sample in samplefile will be use\n";
    std::cout << "                                              otherwise generate new samplefile\n";
    std::cout << "--threads, -t    <number>                [generator] Number of threads use for generating\n";
    std::cout << "--miningzero, -z                         [generator] Force mining seed init as zeros \n";
    std::cout << "--scorefile, -o <output score file>      [generator] Output score file \n";
}

int main(int argc, char* argv[])
{
    std::string mode;
    std::string sampleFile;
    std::string scoreFile;
    unsigned int numberOfSamples = 0;
    bool miningInitZeros = false;
    unsigned int numberOfThreads = std::thread::hardware_concurrency();

    // Loop through each argument
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Check for specific arguments
        if (arg == "--help" || arg == "-h") 
        {
            printHelp();
            return 0;
        }
        else if (arg == "--mode" || arg == "-m") 
        {
            mode = std::string(argv[++i]);
        }
        else if (arg == "--samplefile" || arg == "-s")
        {
            sampleFile = std::string(argv[++i]);
        }
        else if (arg == "--numsamples" || arg == "-n")
        {
            numberOfSamples = std::stoi(argv[++i]);
        }
        else if (arg == "--threads" || arg == "-t")
        {
            numberOfThreads = std::stoi(argv[++i]);
        }
        else if (arg == "--miningzero" || arg == "-z")
        {
            miningInitZeros = true;
        }
        else if (arg == "--scorefile" || arg == "-o")
        {
            scoreFile = std::string(argv[++i]);
        }
        else if (arg == "--algo" || arg == "-a")
        {
            gSelectedAlgorithm = std::atoi(argv[++i]);
        }
        else 
        {
            std::cout << "Unknown argument: " << arg << "\n";
            printHelp();
        }
    }


    std::cout << "Mode: " << mode << std::endl;

    // Print random unittest and exit
    if (mode == "unittest")
    {
        numberOfSamples = std::max(numberOfSamples, kDefaultTotalSamples);
        std::cout << "  Number of samples: " << numberOfSamples << std::endl;
        for (unsigned int i = 0; i < numberOfSamples; ++i)
        {
            print_random_test_case();
        }
        return 0;
    }

    // Generate score to file
    std::cout << "  Sample file: " << sampleFile << std::endl;
    std::cout << "  Init mining zeros: " << miningInitZeros << std::endl;
    std::cout << "  Output score file: " << scoreFile << std::endl;

    std::cout << "Score generator using " << numberOfThreads << " threads." << std::endl;
    generateScore(sampleFile, scoreFile, numberOfThreads, numberOfSamples, miningInitZeros);

    return 0;
}
