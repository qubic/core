#define NO_UEFI

#include "gtest/gtest.h"

#define ENABLE_PROFILING 1

#include "../src/public_settings.h"
#include "../src/mining/score_bpp9000.h"
#include "../src/mining/task_file.h"

#include "score_bpp9000_reference.h"
#include "score_params.h"

#include "utils.h"

#include <vector>
#include <array>
#include <tuple>
#include <memory>
#include <string>
#include <fstream>
#include <utility>
#include <thread>
#include <cstring>
#include <chrono>
#include <iostream>

using namespace score_params;
using namespace test_utils;

static const std::string TASK_FILE_NAME = "data/example_task_bpp9000.bin";
static const std::string SAMPLES_FILE_NAME = "data/samples_bpp9000.csv";
static const std::string SCORES_FILE_NAME = "data/scores_bpp9000.csv";

// true  = ALSO run the engine-vs-reference cross-check on random tasks, for isolating a divergence.
static bool gCompareReference = false;

// Samples run per config
static constexpr unsigned long long TEST_NUMBER_OF_SAMPLES = 64;
// Worker threads for the parallel path; effective count = min(this, hardware_concurrency, numSamples).
static constexpr unsigned int TEST_NUMBER_OF_THREADS = 0;

// Samples and worker threads for the Bpp9000Profile timing run.
static constexpr unsigned long long PROFILING_NUMBER_OF_SAMPLES = 48;
static constexpr unsigned int MAX_NUMBER_OF_PROFILING_THREADS = 12;

static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
    {
        return "";
    }
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::vector<unsigned char> readBinaryFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// samples_bpp9000.csv: header + rows of "seed, publickey, nonce" (each 32-byte hex).
static void loadSamples(std::vector<m256i>& seeds, std::vector<m256i>& pubkeys, std::vector<m256i>& nonces, unsigned long long limit)
{
    auto rows = readCSV(SAMPLES_FILE_NAME);
    ASSERT_GT(rows.size(), 1u) << "missing/empty " << SAMPLES_FILE_NAME;
    unsigned long long n = rows.size() - 1;   // minus header
    if (n > limit)
    {
        n = limit;
    }
    for (unsigned long long i = 0; i < n; ++i)
    {
        seeds.push_back(hexTo32Bytes(trim(rows[i + 1][0]), 32));
        pubkeys.push_back(hexTo32Bytes(trim(rows[i + 1][1]), 32));
        nonces.push_back(hexTo32Bytes(trim(rows[i + 1][2]), 32));
    }
}

// scores_bpp9000.csv: header (config params) + rows of FAILURE counts, one column per config.
static std::vector<std::vector<unsigned int>> loadGolden()
{
    auto rows = readCSV(SCORES_FILE_NAME);
    std::vector<std::vector<unsigned int>> golden;
    for (unsigned long long i = 1; i < rows.size(); ++i)   // skip header
    {
        std::vector<unsigned int> row;
        for (const auto& cell : rows[i])
        {
            row.push_back((unsigned int)std::stoul(trim(cell)));
        }
        golden.push_back(row);
    }
    return golden;
}

// Build synthetic task
template<typename Cfg>
static void buildSyntheticTask(const unsigned char* pool, std::vector<unsigned char>& topoBlock, std::vector<unsigned char>& dataBlock)
{
    constexpr unsigned long long N = Cfg::numberOfInputNeurons;
    constexpr unsigned long long M = Cfg::numberOfOutputNeurons;
    constexpr unsigned long long T = Cfg::sequenceLength;
    constexpr unsigned long long P = Cfg::populationThreshold;
    constexpr unsigned long long K = Cfg::numberOfNeighbors;

    const unsigned long long need = P * K + (N + M + 1) + T * N + T * M;
    const unsigned long long padded = ((need + 63) / 64) * 64;   // random2 draws multiples of 64
    std::vector<unsigned char> rnd(padded);
    unsigned char taskSeed[32] = {};
    score_engine::random2(taskSeed, pool, rnd.data(), padded);

    unsigned long long off = 0;

    // Each neuron gets K distinct neighbours, none the neuron itself. On a clash, bump +1 mod P (this
    // consumes the same P*K random bytes, so the draw stream stays deterministic).
    std::vector<unsigned int> neighborIndices(P * K);
    for (unsigned long long n = 0; n < P; ++n)
    {
        for (unsigned long long j = 0; j < K; ++j)
        {
            unsigned int cand = (unsigned int)(rnd[off++] % P);
            bool ok = false;
            while (!ok)
            {
                ok = (cand != (unsigned int)n);
                for (unsigned long long p = 0; p < j && ok; ++p)
                {
                    if (neighborIndices[n * K + p] == cand)
                    {
                        ok = false;
                    }
                }
                if (!ok)
                {
                    cand = (unsigned int)((cand + 1) % P);
                }
            }
            neighborIndices[n * K + j] = cand;
        }
    }

    std::vector<char> used(P, 0);
    auto pickDistinct = [&]() -> unsigned int
    {
        unsigned int idx = (unsigned int)(rnd[off++] % P);
        while (used[idx])
        {
            idx = (unsigned int)((idx + 1) % P);
        }
        used[idx] = 1;
        return idx;
    };

    std::vector<unsigned int> inputNeuronIndices(N);
    for (unsigned long long i = 0; i < N; ++i)
    {
        inputNeuronIndices[i] = pickDistinct();
    }
    std::vector<unsigned int> outputNeuronIndices(M);
    for (unsigned long long i = 0; i < M; ++i)
    {
        outputNeuronIndices[i] = pickDistinct();
    }
    unsigned int signalNeuronIndex = pickDistinct();

    topoBlock.resize(score_task_file::topologyBytes((unsigned int)N, (unsigned int)M, (unsigned int)P, (unsigned int)K));
    score_task_file::serializeTopologyBlock((unsigned int)N, (unsigned int)M, (unsigned int)P, (unsigned int)K,
                                      inputNeuronIndices.data(), outputNeuronIndices.data(),
                                      signalNeuronIndex, neighborIndices.data(), topoBlock.data());

    std::vector<unsigned char> inputsTrits(T * N);
    for (unsigned long long i = 0; i < T * N; ++i)
    {
        inputsTrits[i] = (unsigned char)(rnd[off++] % 3);
    }
    std::vector<unsigned char> outputsTrits(T * M);
    for (unsigned long long i = 0; i < T * M; ++i)
    {
        outputsTrits[i] = (unsigned char)(rnd[off++] % 3);
    }

    dataBlock.resize(score_task_file::dataBytes((unsigned int)N, (unsigned int)M, T));
    score_task_file::packDataBlock((unsigned int)N, (unsigned int)M, T, inputsTrits.data(), outputsTrits.data(), dataBlock.data());
}

// Threading helpers: workerThreadCount() picks the thread count; runWorkers() spawns that many threads,
// each running worker(threadIdx, numThreads) on its strided sample slice [threadIdx, +numThreads, ...].
// Runs inline when a single thread is enough.
static unsigned int workerThreadCount(unsigned long long numSamples)
{
    unsigned int hw = std::thread::hardware_concurrency();
    if (hw == 0)
    {
        hw = 1;
    }
    unsigned int chosen = (hw < TEST_NUMBER_OF_THREADS) ? hw : TEST_NUMBER_OF_THREADS;   // min(hardware_concurrency, chosen)
    return (unsigned int)((numSamples < (unsigned long long)chosen) ? numSamples : (unsigned long long)chosen);   // no more than one thread per sample
}

template<typename Worker>
static void runWorkers(unsigned int numThreads, const Worker& worker)
{
    if (numThreads <= 1)
    {
        worker(0u, 1u);
        return;
    }
    std::vector<std::thread> threads;
    for (unsigned int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(worker, t, numThreads);
    }
    for (auto& th : threads)
    {
        th.join();
    }
}

// Fill `pool` with the random2 pool derived from `seed` (state is a throwaway scratch buffer).
static void generatePool(const m256i& seed, std::vector<unsigned char>& pool)
{
    std::vector<unsigned char> state(score_engine::STATE_SIZE);
    pool.resize(score_engine::POOL_VEC_PADDING_SIZE);
    score_engine::generateRandom2Pool(seed.m256i_u8, state.data(), pool.data());
}

// Pointers to the topology and data blocks inside a task file, for a config's first-T-rows subview.
struct TaskBlocks
{
    const unsigned char* topo;
    const unsigned char* data;
};

template<typename Cfg>
static TaskBlocks taskSubview(const std::vector<unsigned char>& taskBytes)
{
    // The subview only offsets by T; the config's N/M/P/K must equal the task file's - P and K are NOT
    // sub-viewable (the wiring is global). Fail loudly on a mismatch instead of pointing into garbage.
    const score_task_file::TaskFileHeader* h = (const score_task_file::TaskFileHeader*)taskBytes.data();
    EXPECT_EQ(h->population, (unsigned int)Cfg::populationThreshold) << "task file P != config P";
    EXPECT_EQ(h->numInputTrits, (unsigned int)Cfg::numberOfInputNeurons) << "task file N != config N";
    EXPECT_EQ(h->numOutputTrits, (unsigned int)Cfg::numberOfOutputNeurons) << "task file M != config M";
    EXPECT_EQ(h->numNeighbors, (unsigned int)Cfg::numberOfNeighbors) << "task file K != config K";
    EXPECT_GE(h->numPairs, (unsigned long long)Cfg::sequenceLength) << "task file T < config T";

    const unsigned long long topoBytes = score_task_file::topologyBytes(
        (unsigned int)Cfg::numberOfInputNeurons, (unsigned int)Cfg::numberOfOutputNeurons,
        (unsigned int)Cfg::populationThreshold, (unsigned int)Cfg::numberOfNeighbors);
    const unsigned char* topo = taskBytes.data() + sizeof(score_task_file::TaskFileHeader);
    return { topo, topo + topoBytes };
}

// A fresh engine loaded with the task; nullptr (and a recorded failure) if the task is rejected.
template<typename Cfg>
static std::unique_ptr<score_engine::ScoreBpp9000<Cfg>> makeEngine(const unsigned char* topo, const unsigned char* data)
{
    auto engine = std::make_unique<score_engine::ScoreBpp9000<Cfg>>();
    engine->initMemory();
    if (!engine->loadTaskFromMemory(topo, data))
    {
        ADD_FAILURE() << "loadTaskFromMemory failed (T=" << Cfg::sequenceLength << ")";
        return nullptr;
    }
    return engine;
}

// regression: ScoreBpp9000 on the real task subview vs the groundtruth. One read-only pool is shared
// across threads (all samples use the same mining seed)
template<std::size_t I>
static void runRegressionConfig(const std::vector<m256i>& seeds, const std::vector<m256i>& pubkeys, const std::vector<m256i>& nonces,
                                const std::vector<unsigned char>& taskBytes, const std::vector<std::vector<unsigned int>>& golden)
{
    using Cfg = std::tuple_element_t<I, ConfigList>;

    const TaskBlocks tb = taskSubview<Cfg>(taskBytes);   // configs are first-T-rows subviews of the task file
    std::vector<unsigned char> pool;
    generatePool(seeds[0], pool);

    runWorkers(workerThreadCount(seeds.size()), [&](unsigned int threadIdx, unsigned int numThreads)
    {
        auto engine = makeEngine<Cfg>(tb.topo, tb.data);
        if (!engine)
        {
            return;
        }
        for (unsigned long long s = threadIdx; s < seeds.size(); s += numThreads)
        {
            const m256i& n = nonces[s];
            unsigned int eng = engine->computeScore(pubkeys[s].m256i_u8, n.m256i_u8, pool.data());

            EXPECT_EQ(eng, golden[s][I]) << "config " << I << " sample " << s;
        }
    });
}

// ScoreBpp9000 vs the reference on a random task. Used to debug a mismatch.
// Note: each thread's reference owns a full pool, so this path costs ~512MB per thread.
template<std::size_t I>
static void runRefVsEngineConfig(const std::vector<m256i>& seeds, const std::vector<m256i>& pubkeys, const std::vector<m256i>& nonces)
{
    using Cfg = std::tuple_element_t<I, ConfigList>;

    std::vector<unsigned char> enginePool;
    generatePool(seeds[0], enginePool);

    // One task (all samples share the seed -> same task); shared read-only across threads.
    std::vector<unsigned char> topo;
    std::vector<unsigned char> data;
    buildSyntheticTask<Cfg>(enginePool.data(), topo, data);

    runWorkers(workerThreadCount(seeds.size()), [&](unsigned int threadIdx, unsigned int numThreads)
    {
        auto engine = makeEngine<Cfg>(topo.data(), data.data());
        if (!engine)
        {
            return;
        }
        auto ref = std::make_unique<score_bpp9000_reference::Miner<Cfg>>();
        ref->initialize(seeds[0].m256i_u8);   // reference's own pool (own generator), once per thread
        if (!ref->loadTaskFromMemory(topo.data(), data.data()))
        {
            ADD_FAILURE() << "config " << I << ": reference loadTaskFromMemory failed";
            return;
        }
        for (unsigned long long s = threadIdx; s < seeds.size(); s += numThreads)
        {
            const m256i& n = nonces[s];
            unsigned int eng = engine->computeScore(pubkeys[s].m256i_u8, n.m256i_u8, enginePool.data());
            unsigned int r = ref->computeScore(pubkeys[s].m256i_u8, n.m256i_u8);
            EXPECT_EQ(eng, r) << "config " << I << " sample " << s;
        }
    });
}

template<std::size_t I = 0>
static void runRegression(const std::vector<m256i>& seeds, const std::vector<m256i>& pubkeys, const std::vector<m256i>& nonces,
                          const std::vector<unsigned char>& taskBytes, const std::vector<std::vector<unsigned int>>& golden)
{
    if constexpr (I < CONFIG_COUNT)
    {
        runRegressionConfig<I>(seeds, pubkeys, nonces, taskBytes, golden);
        runRegression<I + 1>(seeds, pubkeys, nonces, taskBytes, golden);
    }
}

template<std::size_t I = 0>
static void runRefVsEngine(const std::vector<m256i>& seeds, const std::vector<m256i>& pubkeys, const std::vector<m256i>& nonces)
{
    if constexpr (I < CONFIG_COUNT)
    {
        runRefVsEngineConfig<I>(seeds, pubkeys, nonces);
        runRefVsEngine<I + 1>(seeds, pubkeys, nonces);
    }
}

// TestBpp9000, internal score vs the samples groundtruth
TEST(TestQubicScoreFunction, Bpp9000Regression)
{
    std::vector<m256i> seeds, pubkeys, nonces;
    loadSamples(seeds, pubkeys, nonces, TEST_NUMBER_OF_SAMPLES);

    auto golden = loadGolden();
    ASSERT_GE(golden.size(), seeds.size()) << "fewer golden rows than samples";

    auto taskBytes = readBinaryFile(TASK_FILE_NAME);
    ASSERT_GT(taskBytes.size(), sizeof(score_task_file::TaskFileHeader)) << "missing/short " << TASK_FILE_NAME;

    // The parallel path shares one pool, valid because all samples use the same mining seed.
    for (unsigned long long i = 1; i < seeds.size(); ++i)
    {
        ASSERT_EQ(memcmp(seeds[i].m256i_u8, seeds[0].m256i_u8, 32), 0)
            << "all samples must share one mining seed for the shared-pool parallel path";
    }

    runRegression(seeds, pubkeys, nonces, taskBytes, golden);
}

// TestBpp9000, internal score vs the score reference from Qiner
TEST(TestQubicScoreFunction, Bpp9000EngineVsReference)
{
    if (gCompareReference)
    {
        std::vector<m256i> seeds, pubkeys, nonces;
        loadSamples(seeds, pubkeys, nonces, TEST_NUMBER_OF_SAMPLES);

        runRefVsEngine(seeds, pubkeys, nonces);
    }
}

static void runBpp9000Profile()
{
    using Cfg = ProductionConfig;

    std::vector<m256i> seeds, pubkeys, nonces;
    loadSamples(seeds, pubkeys, nonces, PROFILING_NUMBER_OF_SAMPLES);
    if (seeds.empty())
    {
        return;
    }

    std::vector<unsigned char> pool;
    generatePool(seeds[0], pool);

    // Profile against a synthetic in-memory task (valid random topology + data built for this config),
    // so a parameter change (e.g. population) needs only a rebuild - no regenerated task file.
    std::vector<unsigned char> topo, data;
    buildSyntheticTask<Cfg>(pool.data(), topo, data);

    // Discard any scope measurements accumulated by earlier tests (Bpp9000Regression /
    // Bpp9000EngineVsReference also call computeScore) so profiling.csv reflects only this run.
    gProfilingDataCollector.clear();

    const unsigned int numThreads = std::max(1U, MAX_NUMBER_OF_PROFILING_THREADS);

    // Each thread sums its own computeScore times into its own slot (distinct indices, no lock).
    std::vector<double> threadSumMs(numThreads, 0.0);
    std::vector<unsigned long long> threadCount(numThreads, 0);

    // Per-sample scores: sample s is written by exactly one worker (disjoint stride), so no lock is needed.
    std::vector<unsigned int> scores(seeds.size(), 0);

    runWorkers(numThreads, [&](unsigned int threadIdx, unsigned int nThreads)
    {
        auto engine = makeEngine<Cfg>(topo.data(), data.data());
        if (!engine)
        {
            return;
        }
        for (unsigned long long s = threadIdx; s < seeds.size(); s += nThreads)
        {
            const m256i& n = nonces[s];

            const auto callStart = std::chrono::steady_clock::now();
            const unsigned int score = engine->computeScore(pubkeys[s].m256i_u8, n.m256i_u8, pool.data());
            const auto callEnd = std::chrono::steady_clock::now();
            scores[s] = score;

            threadSumMs[threadIdx] += std::chrono::duration<double, std::milli>(callEnd - callStart).count();
            threadCount[threadIdx] += 1;
        }
    });

    double totalMs = 0.0;
    unsigned long long totalCount = 0;
    for (unsigned int t = 0; t < numThreads; ++t)
    {
        totalMs += threadSumMs[t];
        totalCount += threadCount[t];
    }
    const double avgMs = (totalCount > 0) ? (totalMs / (double)totalCount) : 0.0;

    std::cout << "[bpp9000 profile] config "
              << Cfg::numberOfInputNeurons << "-" << Cfg::numberOfOutputNeurons << "-" << Cfg::sequenceLength
              << "-" << Cfg::windowWidth << "-" << Cfg::maxNumberOfTicks << "-" << Cfg::numberOfNeighbors
              << "-" << Cfg::populationThreshold << "-" << Cfg::numberOfMutations << "-" << Cfg::solutionThreshold
              << " : avg " << avgMs << " ms/solution" << std::endl;

    // Score distribution over the same samples
    const unsigned int infiniteError = score_engine::ScoreBpp9000<Cfg>::INFINITE_ERROR;
    unsigned long long validCount = 0;
    unsigned long long timeoutCount = 0;
    unsigned long long scoreSum = 0;
    unsigned int scoreMin = infiniteError;
    unsigned int scoreMax = 0;
    for (unsigned long long s = 0; s < scores.size(); ++s)
    {
        const unsigned int sc = scores[s];
        if (sc == infiniteError)
        {
            timeoutCount++;
            continue;
        }
        validCount++;
        scoreSum += sc;
        if (sc < scoreMin)
        {
            scoreMin = sc;
        }
        if (sc > scoreMax)
        {
            scoreMax = sc;
        }
    }
    const double scoreMean = (validCount > 0) ? ((double)scoreSum / (double)validCount) : 0.0;
    if (validCount == 0)
    {
        scoreMin = 0;
    }

    std::cout << "[bpp9000 profile] score (valid " << validCount << ", timeout " << timeoutCount << ")"
              << " : min " << scoreMin << " mean " << scoreMean << " max " << scoreMax << std::endl;

    // Dump the PROFILE_NAMED_SCOPE breakdown (per-scope count + avg/min/max microseconds) to profiling.csv.
    gProfilingDataCollector.writeToFile();
    std::cout << "[bpp9000 profile] wrote profiling.csv (scopes: computeScore/score/initializeANN)" << std::endl;
}


#if ENABLE_PROFILING
TEST(TestQubicScoreFunction, Bpp9000Profile)
{
    runBpp9000Profile();
}
#endif



