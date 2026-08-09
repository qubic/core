#define NO_UEFI

#include "gtest/gtest.h"

#define ENABLE_PROFILING 0

#include "../src/public_settings.h"
#include "../src/mining/score_bpp9000.h"
#include "../src/mining/task_file.h"
#include "../src/score.h"

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
#include <atomic>
#include <iostream>
#include <cstddef>

using namespace score_params;
using namespace test_utils;

static const std::string TASK_FILE_NAME = "data/example_task_bpp9000.bin";
static const std::string SAMPLES_FILE_NAME = "data/samples_bpp9000.csv";
static const std::string SCORES_FILE_NAME = "data/scores_bpp9000.csv";

// true  = ALSO run the engine-vs-reference cross-check on random tasks, for isolating a divergence.
static bool gCompareReference = false;

// Samples run per config
static constexpr unsigned long long TEST_NUMBER_OF_SAMPLES = 32;
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

// =============================================================================
// Ant-colony related 

namespace
{
using AntCfg = ProductionConfig;
using AntEngine = score_engine::ScoreBpp9000<AntCfg>;

// Pool + synthetic task + loaded engine. Every ant test starts from one of these.
template<typename Cfg>
struct AntFixtureT
{
    std::vector<unsigned char> pool;
    std::vector<unsigned char> taskBytes;
    std::unique_ptr<score_engine::ScoreBpp9000<Cfg>> engine;
};
using AntFixture = AntFixtureT<AntCfg>;

template<typename Cfg>
static bool makeAntFixtureT(AntFixtureT<Cfg>& f)
{
    std::vector<m256i> seeds;
    std::vector<m256i> pubkeys;
    std::vector<m256i> nonces;
    loadSamples(seeds, pubkeys, nonces, 1);
    if (seeds.empty())
    {
        ADD_FAILURE() << "missing/short " << SAMPLES_FILE_NAME;
        return false;
    }
    generatePool(seeds[0], f.pool);

    f.taskBytes = readBinaryFile(TASK_FILE_NAME);
    if (f.taskBytes.size() <= sizeof(score_task_file::TaskFileHeader))
    {
        ADD_FAILURE() << "missing/short " << TASK_FILE_NAME;
        return false;
    }
    const TaskBlocks tb = taskSubview<Cfg>(f.taskBytes);
    f.engine = makeEngine<Cfg>(tb.topo, tb.data);
    return f.engine != nullptr;
}

static bool makeAntFixture(AntFixture& f)
{
    return makeAntFixtureT<AntCfg>(f);
}

static m256i makePubkey(unsigned char tag)
{
    m256i k = m256i::zero();
    k.m256i_u8[0] = tag;
    k.m256i_u8[31] = (unsigned char)(tag * 3 + 1);
    return k;
}

// Canonical ant nonce: nonce[0] selects bpp9000, nonce[1] = L, nonce[2] = K, rest is the walk seed.
static m256i makeAntNonce(unsigned char L, unsigned char K, unsigned char tag)
{
    m256i n = m256i::zero();
    n.m256i_u8[0] = (unsigned char)score_engine::AlgoType::Bpp9000;
    n.m256i_u8[1] = L;
    n.m256i_u8[2] = K;
    n.m256i_u8[3] = tag;
    n.m256i_u8[17] = (unsigned char)(tag ^ 0x5A);
    return n;
}

// Find a nonce whose walk from this parent actually improves on the parent's score. Needed only by
// tests that compare two children of the SAME parent
static bool findImprovingNonce(AntEngine& engine, const AntEngine::ANN& parent, const m256i& pk,
                               const m256i& anchor, const unsigned char* pool, m256i& outNonce)
{
    for (unsigned char tag = 1; tag <= 6; ++tag)
    {
        const m256i n = makeAntNonce(6, 5, (unsigned char)(50 + tag));
        const unsigned int sc = engine.computeScoreFromParent(parent, pk.m256i_u8, n.m256i_u8,
                                                              anchor.m256i_u8, pool);
        if (sc != score_engine::INVALID_SCORE_VALUE)
        {
            outNonce = n;
            return true;
        }
    }
    return false;
}
}

// Make sure the score at the full computeScore flow have the same score with
// the engine start directly from the best/final LUT
TEST(TestQubicScoreAntColony, BestAnnReproducesReturnedScore)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(1);
    const m256i nonce = makeAntNonce(3, 0, 11);

    const unsigned int best = f.engine->computeScore(pk.m256i_u8, nonce.m256i_u8, f.pool.data());
    // Re-score the LUT the walk kept, taken out and put back through the public form - this also
    // exercises the compact/expand round trip the tree relies on.
    AntEngine::ANN bestLut;
    f.engine->getBestANN(bestLut);
    f.engine->expand(bestLut, f.engine->currentANN);
    EXPECT_EQ(f.engine->score(), best);
}

// Make sure the score at the full computeScoreFromParent flow have the same score with
// the engine start directly from the best/final LUT
TEST(TestQubicScoreAntColony, BestAnnReproducesScoreFromParent)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(2);
    const m256i nonce = makeAntNonce(4, 2, 23);
    const m256i anchor = makePubkey(9);

    AntEngine::ANN root;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), root);

    const unsigned int childScore = f.engine->computeScoreFromParent(
        root, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data());

    // Re-score the LUT the walk kept, taken out and put back through the public form - this also
    // exercises the compact/expand round trip the tree relies on.
    AntEngine::ANN bestLut;
    f.engine->getBestANN(bestLut);
    f.engine->expand(bestLut, f.engine->currentANN);
    EXPECT_EQ(f.engine->score(), childScore);
}

// An ANN is exactly its LUT: no storage padding escapes the engine, so hashing or shipping one is
// just sizeof(ANN) and a future change to lutStride cannot alter a digest or the wire format.
TEST(TestQubicScoreAntColony, AnnCarriesOnlyTheLut)
{
    static_assert(sizeof(AntEngine::ANN) == AntCfg::populationThreshold * AntEngine::lutSize,
                  "ANN must be the LUT and nothing else");
    static_assert(sizeof(AntEngine::ANN) < sizeof(AntEngine::PaddedLut),
                  "the working layout is the padded one, not the other way round");

    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(3);
    AntEngine::ANN root;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), root);

    // Every byte handed out is a trit; nothing from the padded rows leaked in.
    for (unsigned long long i = 0; i < sizeof(root.lut); ++i)
    {
        ASSERT_LT(root.lut[i], 3) << "byte " << i << " of the returned ANN is not a trit";
    }

    // Scribbling on the working layout's padding cannot change what comes out of it.
    AntEngine::PaddedLut working;
    f.engine->expand(root, working);
    for (unsigned long long k = 0; k < AntEngine::maxNumberOfNeurons; ++k)
    {
        for (unsigned long long b = AntEngine::lutSize; b < AntEngine::lutStride; ++b)
        {
            working.lut[k * AntEngine::lutStride + b] = (unsigned char)(0xA5 + k + b);
        }
    }
    AntEngine::ANN again;
    f.engine->compact(working, again);
    EXPECT_EQ(memcmp(&again, &root, sizeof(root)), 0) << "storage padding reached the ANN";
}

// expand/compact must be lossless, since every parent read from the tree goes through expand and
// every child written back goes through compact.
TEST(TestQubicScoreAntColony, AnnSurvivesExpandAndCompact)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    AntEngine::ANN original;
    f.engine->deriveRootANN(makePubkey(44).m256i_u8, f.pool.data(), original);

    AntEngine::PaddedLut working;
    f.engine->expand(original, working);
    AntEngine::ANN restored;
    f.engine->compact(working, restored);

    EXPECT_EQ(memcmp(&restored, &original, sizeof(original)), 0) << "expand/compact is not lossless";
}

TEST(TestQubicScoreAntColony, RootAnnIsDeterministicAndPerIdentity)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pkA = makePubkey(4);
    const m256i pkB = makePubkey(5);

    AntEngine::ANN a1;
    AntEngine::ANN a2;
    AntEngine::ANN b1;

    f.engine->deriveRootANN(pkA.m256i_u8, f.pool.data(), a1);
    // Deriving another identity's root overwrites initValue.lutInit, which is the state a1 came from.
    f.engine->deriveRootANN(pkB.m256i_u8, f.pool.data(), b1);
    f.engine->deriveRootANN(pkA.m256i_u8, f.pool.data(), a2);

    EXPECT_EQ(memcmp(&a1, &a2, sizeof(a1)), 0) << "root depends on engine state";
    EXPECT_NE(memcmp(&a1, &b1, sizeof(a1)), 0) << "two identities share a root";
}

// A child is a function of (parent, pubkey, nonce, anchor). Same inputs must give the same score AND
// the same inherited LUT; a different parent must not give the same child.
TEST(TestQubicScoreAntColony, ChildIsDeterministicAndInheritsParent)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(6);
    const m256i nonce = makeAntNonce(5, 3, 41);
    const m256i anchor = makePubkey(12);

    AntEngine::ANN parentA;
    AntEngine::ANN parentB;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), parentA);
    f.engine->deriveRootANN(makePubkey(7).m256i_u8, f.pool.data(), parentB);

    const unsigned int s1 = f.engine->computeScoreFromParent(
        parentA, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN child1;
    f.engine->getBestANN(child1);

    const unsigned int s2 = f.engine->computeScoreFromParent(
        parentA, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data());

    EXPECT_EQ(s1, s2) << "same inputs gave different scores";
    AntEngine::ANN child2;
    f.engine->getBestANN(child2);
    EXPECT_EQ(memcmp(&child1, &child2, sizeof(child1)), 0) << "same inputs gave a different child LUT";

    f.engine->computeScoreFromParent(parentB, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN child3;
    f.engine->getBestANN(child3);
    EXPECT_NE(memcmp(&child1, &child3, sizeof(child1)), 0) << "the parent LUT was not inherited";
}

// The anchor digest is part of the child's walk seed, so the same nonce on the same parent must not
// produce the same child at a different anchor.
TEST(TestQubicScoreAntColony, ChildDependsOnAnchorDigest)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(8);
    const m256i anchorA = makePubkey(20);
    const m256i anchorB = makePubkey(21);

    AntEngine::ANN parent;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), parent);

    m256i nonce;
    ASSERT_TRUE(findImprovingNonce(*f.engine, parent, pk, anchorA, f.pool.data(), nonce))
        << "no nonce improved on this parent, so bestANN would not move and the comparison below "
           "would be vacuous";

    f.engine->computeScoreFromParent(parent, pk.m256i_u8, nonce.m256i_u8, anchorA.m256i_u8, f.pool.data());
    AntEngine::ANN c1;
    f.engine->getBestANN(c1);

    f.engine->computeScoreFromParent(parent, pk.m256i_u8, nonce.m256i_u8, anchorB.m256i_u8, f.pool.data());
    AntEngine::ANN c2;
    f.engine->getBestANN(c2);

    EXPECT_NE(memcmp(&c1, &c2, sizeof(c1)), 0) << "anchor digest does not reach the walk";
}

// Non-canonical nonces are refused by the scorer itself, so no caller can score first and check after.
TEST(TestQubicScoreAntColony, NonCanonicalNonceIsRejected)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(10);
    const m256i anchor = makePubkey(30);
    AntEngine::ANN parentA;
    AntEngine::ANN parentB;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), parentA);
    f.engine->deriveRootANN(makePubkey(11).m256i_u8, f.pool.data(), parentB);

    constexpr unsigned char maxK = (unsigned char)AntCfg::numberOfMutations;
    const m256i good = makeAntNonce(3, 5, 62);

    // A rejected nonce and a timed-out walk
    f.engine->computeScoreFromParent(parentA, pk.m256i_u8, good.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN afterA;
    f.engine->getBestANN(afterA);

    // L below range, L above range, K above numberOfMutations, wrong algorithm slot.
    static constexpr unsigned int numberOfBadNonces = 4;
    m256i bad[numberOfBadNonces];
    bad[0] = makeAntNonce(0, 0, 64);
    bad[1] = makeAntNonce((unsigned char)(score_engine::MAX_LUT_ENTRIES_PER_STEP + 1), 0, 65);
    bad[2] = makeAntNonce(3, (unsigned char)(maxK + 1), 66);
    bad[3] = makeAntNonce(3, 0, 67);
    bad[3].m256i_u8[0] = (unsigned char)score_engine::AlgoType::Neuraxon;

    for (unsigned int i = 0; i < numberOfBadNonces; i++)
    {
        // The bad nonce is early rejected in computeScoreFromParent()
        EXPECT_EQ(f.engine->computeScoreFromParent(parentB, pk.m256i_u8, bad[i].m256i_u8, anchor.m256i_u8, f.pool.data()),
                  score_engine::INVALID_SCORE_VALUE) << "non-canonical nonce " << i << " accepted";

        AntEngine::ANN now;
        f.engine->getBestANN(now);
        EXPECT_EQ(memcmp(&now, &afterA, sizeof(now)), 0) << "rejected nonce " << i << " still ran the walk";
    }

    // Now after bad nonce, we feed good nonce, we expect this is ok
    f.engine->computeScoreFromParent(parentB, pk.m256i_u8, good.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN afterB;
    f.engine->getBestANN(afterB);
    EXPECT_NE(memcmp(&afterB, &afterA, sizeof(afterB)), 0) << "canonical nonce was not scored";
}


// L and K boundaries of the canonical rule, checked as a pure predicate so no walk is needed.
TEST(TestQubicScoreAntColony, NonceCanonicalRuleBoundaries)
{
    using AntScorer = score_engine::ScoreBpp9000<AntCfg>;
    constexpr unsigned char maxL = (unsigned char)score_engine::MAX_LUT_ENTRIES_PER_STEP;
    constexpr unsigned char maxK = (unsigned char)AntScorer::numberOfMutations;

    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(1, 0, 70).m256i_u8));
    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(maxL, 0, 71).m256i_u8));
    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, maxK, 72).m256i_u8));

    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(0, 0, 73).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce((unsigned char)(maxL + 1), 0, 74).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, (unsigned char)(maxK + 1), 75).m256i_u8));
}


// ---------------------------------------------------------------------------
// ScoreFunction task queue.

typedef ScoreFunction<1> TaskQueueScoreFunction;

static constexpr unsigned int TASK_QUEUE_PROBE_CAPACITY = 256;

// What the work functions record, so a test can see which tasks ran and what they received.
struct TaskQueueProbe
{
    std::atomic<unsigned int> runCount[TASK_QUEUE_PROBE_CAPACITY];
    std::atomic<unsigned int> altRunCount;
    std::atomic<unsigned int> payloadMismatches;
    std::atomic<unsigned int> started;
    std::atomic<unsigned int> finished;

    void reset()
    {
        for (unsigned int i = 0; i < TASK_QUEUE_PROBE_CAPACITY; i++)
        {
            runCount[i].store(0);
        }
        altRunCount.store(0);
        payloadMismatches.store(0);
        started.store(0);
        finished.store(0);
    }
};

struct TaskQueuePayload
{
    TaskQueueProbe* probe;
    unsigned int id;
    unsigned int patternSize;
    unsigned char pattern[64];
};
static_assert(sizeof(TaskQueuePayload) <= TaskQueueScoreFunction::TASK_PAYLOAD_MAX,
    "TaskQueuePayload must fit one queue slot");

// Bigger than one slot, but starts with a valid payload so a wrongly accepted task records the run
// instead of dereferencing garbage.
struct TaskQueueOversizedPayload
{
    TaskQueuePayload base;
    unsigned char extra[TaskQueueScoreFunction::TASK_PAYLOAD_MAX];
};

static TaskQueueProbe gTaskQueueProbe;
static std::unique_ptr<TaskQueueScoreFunction> gTaskQueueOwner;
static std::atomic<bool> gTaskQueueHelpersStop;

static TaskQueuePayload makeTaskQueuePayload(unsigned int id, unsigned int patternSize = sizeof(TaskQueuePayload::pattern))
{
    TaskQueuePayload task;
    setMem(&task, sizeof(task), 0);
    task.probe = &gTaskQueueProbe;
    task.id = id;
    task.patternSize = patternSize;
    // Fill the pattern with id+i, so it varies by task and by position
    for (unsigned int i = 0; i < patternSize; i++)
    {
        task.pattern[i] = (unsigned char)(id + i);
    }
    return task;
}

// Records the run and checks the payload survived the copy into and out of the queue.
static void countTaskRun(unsigned long long, void* payload)
{
    const TaskQueuePayload* task = (const TaskQueuePayload*)payload;
    if (task->id >= TASK_QUEUE_PROBE_CAPACITY)
    {
        // Surfaces as a failed test rather than a write past runCount.
        task->probe->payloadMismatches.fetch_add(1);
        return;
    }
    if (task->patternSize > sizeof(task->pattern))
    {
        // A scalar that did not survive the copy is itself a mismatch, and it must not be trusted as
        // the loop bound below.
        task->probe->payloadMismatches.fetch_add(1);
        return;
    }
    for (unsigned int i = 0; i < task->patternSize; i++)
    {
        if (task->pattern[i] != (unsigned char)(task->id + i))
        {
            task->probe->payloadMismatches.fetch_add(1);
            break;
        }
    }
    task->probe->runCount[task->id].fetch_add(1);
}

class TestQubicScoreTaskQueue : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (gTaskQueueOwner.get() == nullptr)
        {
            gTaskQueueOwner.reset(new TaskQueueScoreFunction());
        }
        gTaskQueueOwner->resetTaskQueue();
        gTaskQueueProbe.reset();
        gTaskQueueHelpersStop.store(false);
    }

    TaskQueueScoreFunction& queue()
    {
        return *gTaskQueueOwner;
    }
};

// Task run once. Normal case
TEST_F(TestQubicScoreTaskQueue, EveryTaskRunsExactlyOnce)
{
    const unsigned int taskCount = TASK_QUEUE_PROBE_CAPACITY;
    for (unsigned int i = 0; i < taskCount; i++)
    {
        const TaskQueuePayload task = makeTaskQueuePayload(i);
        EXPECT_TRUE(queue().addTask(countTaskRun, &task, sizeof(task)));
    }

    // Try to process every task in queue until all done
    queue().runUntilDone(0);

    for (unsigned int i = 0; i < taskCount; i++)
    {
        // Each task is expected run once
        EXPECT_EQ(gTaskQueueProbe.runCount[i].load(), 1u) << "task " << i;
    }
}

// Mixed mutiple size of tasks
TEST_F(TestQubicScoreTaskQueue, PayloadArrivesIntact)
{
    // Bytes a task of this pattern length hands to addTask.
    const auto taskQueuePayloadBytes = [](unsigned int patternSize) -> unsigned int
    {
        return (unsigned int)offsetof(TaskQueuePayload, pattern) + patternSize;
    };

    const unsigned int patternSizes[] = { 0, sizeof(TaskQueuePayload::pattern) };
    const unsigned int sizeCount = (unsigned int)(sizeof(patternSizes) / sizeof(patternSizes[0]));
    const unsigned int perSize = 4;

    unsigned int id = 0;
    for (unsigned int s = 0; s < sizeCount; s++)
    {
        for (unsigned int i = 0; i < perSize; i++)
        {
            const TaskQueuePayload task = makeTaskQueuePayload(id, patternSizes[s]);
            EXPECT_TRUE(queue().addTask(countTaskRun, &task, taskQueuePayloadBytes(patternSizes[s])));
            id++;
        }
    }

    // Try to process every task in queue until all done
    queue().runUntilDone(0);

    EXPECT_EQ(gTaskQueueProbe.payloadMismatches.load(), 0u);
    for (unsigned int i = 0; i < id; i++)
    {
        EXPECT_EQ(gTaskQueueProbe.runCount[i].load(), 1u) << "task " << i;
    }
}

// A payload larger than one slot must be refused, not truncated into the slot or written past it.
TEST_F(TestQubicScoreTaskQueue, OversizedPayloadIsRejected)
{
    TaskQueueOversizedPayload oversized;
    setMem(&oversized, sizeof(oversized), 0);
    oversized.base = makeTaskQueuePayload(0);

    EXPECT_FALSE(queue().addTask(countTaskRun, &oversized, sizeof(oversized)));

    // Nothing was queued, so the drain has nothing to run.
    queue().runUntilDone(0);
    EXPECT_EQ(gTaskQueueProbe.runCount[0].load(), 0u);
}

// The queue is bounded. Filling it until addTask refuses shows where the bound is, and that going
// past it fails instead of writing off the end of the array.
TEST_F(TestQubicScoreTaskQueue, QueueRejectsOverflow)
{
    unsigned long long accepted = 0;
    for (unsigned long long i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK + 16; i++)
    {
        const TaskQueuePayload task = makeTaskQueuePayload(0);
        const bool added = queue().addTask(countTaskRun, &task, sizeof(task));
        if (!added)
        {
            break;
        }
        accepted++;
    }

    EXPECT_EQ(accepted, NUMBER_OF_TRANSACTIONS_PER_TICK);
}

// The drain must return only after every task has finished, including the ones other threads picked
// up. Returning once the last task was merely taken would leave work still running.
TEST_F(TestQubicScoreTaskQueue, DrainWaitsForTasksRunningOnOtherThreads)
{
    // Stays in flight long enough that a drain returning on tasks taken, rather than tasks finished,
    // would be visible.
    const TaskQueueScoreFunction::WorkFunc slowTaskRun = [](unsigned long long, void* payload)
    {
        const TaskQueuePayload* task = (const TaskQueuePayload*)payload;
        task->probe->started.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        task->probe->finished.fetch_add(1);
    };

    const unsigned int taskCount = 64;
    const unsigned int helperCount = 4;
    for (unsigned int i = 0; i < taskCount; i++)
    {
        const TaskQueuePayload task = makeTaskQueuePayload(i);
        EXPECT_TRUE(queue().addTask(slowTaskRun, &task, sizeof(task)));
    }

    // Create another threads for process some tasks in queues
    std::vector<std::thread> helpers;
    for (unsigned int t = 0; t < helperCount; t++)
    {
        const unsigned long long helperProcessorNumber = t + 1;
        helpers.emplace_back([helperProcessorNumber]()
        {
            // What a request processor does: keep offering to run queued work until told to stop.
            while (!gTaskQueueHelpersStop.load())
            {
                gTaskQueueOwner->tryProcessOneTask(helperProcessorNumber);
            }
        });
    }

    // Mark the task queue ready and process remained task
    queue().runUntilDone(0);
    const unsigned int finishedOnReturn = gTaskQueueProbe.finished.load();

    gTaskQueueHelpersStop.store(true);
    for (unsigned int t = 0; t < helperCount; t++)
    {
        helpers[t].join();
    }

    // Expect all task are done
    EXPECT_EQ(finishedOnReturn, taskCount);
    EXPECT_EQ(gTaskQueueProbe.started.load(), taskCount);
}

// Tasks are queued before the drain opens the queue. Until it does, a helper must pick up nothing, so
// a half-built batch is never started.
TEST_F(TestQubicScoreTaskQueue, ClosedQueueHandsOutNothing)
{
    const unsigned int taskCount = 8;
    for (unsigned int i = 0; i < taskCount; i++)
    {
        const TaskQueuePayload task = makeTaskQueuePayload(i);
        EXPECT_TRUE(queue().addTask(countTaskRun, &task, sizeof(task)));
    }

    // Try to run many task but no thing run because the queue is not ready
    for (unsigned int i = 0; i < 32; i++)
    {
        queue().tryProcessOneTask(0);
    }
    for (unsigned int i = 0; i < taskCount; i++)
    {
        EXPECT_EQ(gTaskQueueProbe.runCount[i].load(), 0u) << "task " << i << " ran before the drain";
    }

    // Process all items
    queue().runUntilDone(0);
    for (unsigned int i = 0; i < taskCount; i++)
    {
        EXPECT_EQ(gTaskQueueProbe.runCount[i].load(), 1u) << "task " << i;
    }
}

// Every tick resets the queue and refills it, so a second batch must behave like the first. It will
// not if reset leaves any of the three counters behind.
TEST_F(TestQubicScoreTaskQueue, QueueIsReusableAfterReset)
{
    const unsigned int taskCount = 16;
    for (unsigned int batch = 0; batch < 2; batch++)
    {
        queue().resetTaskQueue();
        gTaskQueueProbe.reset();

        for (unsigned int i = 0; i < taskCount; i++)
        {
            const TaskQueuePayload task = makeTaskQueuePayload(i);
            EXPECT_TRUE(queue().addTask(countTaskRun, &task, sizeof(task)));
        }

        queue().runUntilDone(0);

        for (unsigned int i = 0; i < taskCount; i++)
        {
            EXPECT_EQ(gTaskQueueProbe.runCount[i].load(), 1u) << "batch " << batch << " task " << i;
        }
    }
}

// Each task carries its own work function, so one batch can mix kinds. This is what lets a second
// caller share the queue without changing it.
TEST_F(TestQubicScoreTaskQueue, OneBatchCarriesDifferentWorkFunctions)
{
    // A second work function, so a batch can be shown to carry more than one kind of task.
    const TaskQueueScoreFunction::WorkFunc countAltTaskRun = [](unsigned long long, void* payload)
    {
        const TaskQueuePayload* task = (const TaskQueuePayload*)payload;
        task->probe->altRunCount.fetch_add(1);
    };

    const unsigned int pairCount = 32;
    for (unsigned int i = 0; i < pairCount; i++)
    {
        const TaskQueuePayload counted = makeTaskQueuePayload(i);
        EXPECT_TRUE(queue().addTask(countTaskRun, &counted, sizeof(counted)));
        const TaskQueuePayload alt = makeTaskQueuePayload(i);
        EXPECT_TRUE(queue().addTask(countAltTaskRun, &alt, sizeof(alt)));
    }

    queue().runUntilDone(0);

    for (unsigned int i = 0; i < pairCount; i++)
    {
        EXPECT_EQ(gTaskQueueProbe.runCount[i].load(), 1u) << "task " << i;
    }
    EXPECT_EQ(gTaskQueueProbe.altRunCount.load(), pairCount);
}

