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

static const std::string PRODUCTION_TASK_FILE_NAME = "data/bpp9000.task";
static const std::string PRODUCTION_FILE_NAME = "data/gt_production.csv";
static const std::string PRODUCTION_ANT_FILE_NAME = "data/gt_ant_production.csv";
static const std::string PRODUCTION_POSITIVE_FILE_NAME = "data/gt_ant_positive.csv";

// true  = ALSO run the engine-vs-reference cross-check on random tasks, for isolating a divergence.
static bool gCompareReference = true;   // engine vs the scalar reference: the SIMD bit-exactness check

// The engine-vs-reference tests when gCompareReference = true, 0 runs all rows.
static constexpr unsigned long long ENGINE_VS_REFERENCE_SAMPLES = 32;
// Chains of gt_ant_positive.csv to run, 0 runs all chains.
static constexpr unsigned int POSITIVE_SAMPLE_CHAINS = 2;
// Worker threads for the parallel path; min with hardware_concurrency and numSamples. 0 falls back to 1 (serial).
static constexpr unsigned int TEST_NUMBER_OF_THREADS = 4;

// Number of samples and worker threads for the Bpp9000Profile timing run, 0 runs all.
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
    if (limit != 0 && n > limit)
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

// gt_ant_production.csv rows as profiling inputs; limit 0 takes every row.
static void loadProfileSamples(std::vector<m256i>& seeds, std::vector<m256i>& pubkeys, std::vector<m256i>& nonces, unsigned long long limit)
{
    auto rows = readCSV(PRODUCTION_ANT_FILE_NAME);
    ASSERT_GT(rows.size(), 1u) << "missing/empty " << PRODUCTION_ANT_FILE_NAME;
    unsigned long long n = rows.size() - 1;
    if (limit != 0 && n > limit)
    {
        n = limit;
    }
    for (unsigned long long i = 0; i < n; ++i)
    {
        pubkeys.push_back(hexTo32Bytes(trim(rows[i + 1][2]), 32));
        nonces.push_back(hexTo32Bytes(trim(rows[i + 1][3]), 32));
        seeds.push_back(hexTo32Bytes(trim(rows[i + 1][5]), 32));
    }
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
    unsigned int requested = (TEST_NUMBER_OF_THREADS == 0) ? 1u : TEST_NUMBER_OF_THREADS;   // 0 falls back to 1 (serial)
    unsigned int chosen = (hw < requested) ? hw : requested;                                // min(hardware_concurrency, requested)
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
    // The config's N/M must equal the task file's; the data is offset past the topology, sized by the
    // file's own header (so a task written at any population subviews correctly).
    const score_task_file::TaskFileHeader* h = (const score_task_file::TaskFileHeader*)taskBytes.data();
    EXPECT_EQ(h->numInputTrits, (unsigned int)Cfg::numberOfInputNeurons) << "task file N != config N";
    EXPECT_EQ(h->numOutputTrits, (unsigned int)Cfg::numberOfOutputNeurons) << "task file M != config M";
    EXPECT_GE(h->numPairs, (unsigned long long)Cfg::sequenceLength) << "task file T < config T";
#if BPP9000_TASK_HAS_TOPOLOGY
    EXPECT_EQ(h->population, (unsigned int)Cfg::populationThreshold) << "task file P != config P";
    EXPECT_EQ(h->numNeighbors, (unsigned int)Cfg::numberOfNeighbors) << "task file K != config K";
#endif

    const unsigned long long topoBytes = score_task_file::topologyBytes(
        h->numInputTrits, h->numOutputTrits, h->population, h->numNeighbors);
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

// The node's childAnnHash of a network (qubic.cpp folds it into resourceTestingDigest); the childAnnHash
// golden columns hold this value for each row's best network.
template<typename Cfg>
static unsigned int childAnnHash(const typename score_engine::ScoreBpp9000<Cfg>::ANN& ann)
{
    unsigned int hash = 0;
    KangarooTwelve(&ann, sizeof(ann), &hash, sizeof(hash));
    return hash;
}

template<typename Cfg>
static void referenceLoadAnn(score_bpp9000_reference::Miner<Cfg>& ref, const typename score_engine::ScoreBpp9000<Cfg>::ANN& ann)
{
    memcpy(ref.curInitial, ann.initialNeuronValues, sizeof(ref.curInitial));
    for (unsigned long long i = 0; i < Cfg::populationThreshold * Cfg::numberOfNeighbors; ++i)
    {
        ref.neighborIndices[i] = ann.neighbor[i];
    }
    memcpy(ref.curLut, ann.lut, sizeof(ref.curLut));
}

template<typename Cfg>
static void referenceBestAnn(const score_bpp9000_reference::Miner<Cfg>& ref, typename score_engine::ScoreBpp9000<Cfg>::ANN& out)
{
    memcpy(out.initialNeuronValues, ref.bestInitial, sizeof(out.initialNeuronValues));
    for (unsigned long long i = 0; i < Cfg::populationThreshold * Cfg::numberOfNeighbors; ++i)
    {
        out.neighbor[i] = (unsigned short)ref.bestNeighborIndices[i];
    }
    memcpy(out.lut, ref.bestLut, sizeof(out.lut));
}

template<typename Cfg>
static score_engine::Rating referenceScoreFromParent(score_bpp9000_reference::Miner<Cfg>& ref,
    const typename score_engine::ScoreBpp9000<Cfg>::ANN& parent, unsigned long long parentShift, const m256i& pubkey,
    const m256i& nonce, const m256i& anchor, typename score_engine::ScoreBpp9000<Cfg>::ANN& outAnn)
{
    using Ref = score_bpp9000_reference::Miner<Cfg>;
    referenceLoadAnn<Cfg>(ref, parent);
    ref.deriveMutationSeeds(pubkey.m256i_u8, nonce.m256i_u8, anchor.m256i_u8);
    ref.shift = parentShift;
    const score_engine::Rating rating = ref.computeScoreFromCurrent(Ref::changesPerStep(nonce.m256i_u8), nonce.m256i_u8[2],
        Ref::modeOf(nonce.m256i_u8));
    referenceBestAnn<Cfg>(ref, outAnn);
    return rating;
}

// ScoreBpp9000 vs the reference on a random task. Used to debug a mismatch.
// Note: each thread's reference owns a full pool, so this path costs ~512MB per thread.
template<typename Cfg>
static void runRefVsEngineConfig(const std::vector<m256i>& seeds, const std::vector<m256i>& pubkeys, const std::vector<m256i>& nonces)
{
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
        // The engine needs control/output derived explicitly; the reference folds it into initialize().
        engine->deriveControlOutput(seeds[0].m256i_u8, enginePool.data());
        auto ref = std::make_unique<score_bpp9000_reference::Miner<Cfg>>();
        ref->initialize(seeds[0].m256i_u8);   // reference's own pool (own generator), once per thread
        if (!ref->loadTaskFromMemory(topo.data(), data.data()))
        {
            ADD_FAILURE() << "reference loadTaskFromMemory failed";
            return;
        }
        auto engineAnn = std::make_unique<typename score_engine::ScoreBpp9000<Cfg>::ANN>();
        auto refAnn = std::make_unique<typename score_engine::ScoreBpp9000<Cfg>::ANN>();
        for (unsigned long long s = threadIdx; s < seeds.size(); s += numThreads)
        {
            const m256i& n = nonces[s];
            const score_engine::Rating eng = engine->computeScore(pubkeys[s].m256i_u8, n.m256i_u8, enginePool.data());
            const score_engine::Rating r = ref->computeScore(pubkeys[s].m256i_u8, n.m256i_u8);
            EXPECT_EQ(eng.error, r.error) << "sample " << s;
            EXPECT_EQ(eng.shift, r.shift) << "sample " << s;
            EXPECT_EQ(engine->parentRating.error, ref->parentRating.error) << "parent rating, sample " << s;
            EXPECT_EQ(engine->parentRating.shift, ref->parentRating.shift) << "parent rating, sample " << s;
            engine->getBestANN(*engineAnn);
            referenceBestAnn<Cfg>(*ref, *refAnn);
            EXPECT_EQ(memcmp(engineAnn.get(), refAnn.get(), sizeof(*refAnn)), 0) << "best ANN bytes, sample " << s;
        }
    });
}

static void runRefVsEngine(const std::vector<m256i>& seeds, const std::vector<m256i>& pubkeys, const std::vector<m256i>& nonces)
{
    runRefVsEngineConfig<ProductionConfig>(seeds, pubkeys, nonces);
}

TEST(TestQubicScoreFunction, Bpp9000ProductionRegression)
{
    auto rows = readCSV(PRODUCTION_FILE_NAME);
    ASSERT_GT(rows.size(), 1u) << "missing/empty " << PRODUCTION_FILE_NAME;

    std::vector<m256i> pubkeys;
    std::vector<m256i> nonces;
    std::vector<unsigned int> golden;       // error
    std::vector<unsigned int> goldenShift;
    std::vector<m256i> uniqueSeeds;         // distinct mining seeds -> one pool each
    std::vector<unsigned int> poolIndex;    // per row: index into uniqueSeeds
    for (unsigned long long i = 1; i < rows.size(); ++i)
    {
        pubkeys.push_back(hexTo32Bytes(trim(rows[i][0]), 32));
        nonces.push_back(hexTo32Bytes(trim(rows[i][1]), 32));
        const m256i seed = hexTo32Bytes(trim(rows[i][2]), 32);
        goldenShift.push_back((unsigned int)std::stoul(trim(rows[i][3])));
        golden.push_back((unsigned int)std::stoul(trim(rows[i][4])));

        unsigned int idx = (unsigned int)uniqueSeeds.size();
        for (unsigned int k = 0; k < uniqueSeeds.size(); ++k)
        {
            if (memcmp(uniqueSeeds[k].m256i_u8, seed.m256i_u8, 32) == 0)
            {
                idx = k;
                break;
            }
        }
        if (idx == uniqueSeeds.size())
        {
            uniqueSeeds.push_back(seed);
        }
        poolIndex.push_back(idx);
    }
    ASSERT_FALSE(pubkeys.empty());

    std::vector<std::vector<unsigned char>> pools(uniqueSeeds.size());
    for (size_t k = 0; k < uniqueSeeds.size(); ++k)
    {
        generatePool(uniqueSeeds[k], pools[k]);
    }

    // The production task
    auto taskBytes = readBinaryFile(PRODUCTION_TASK_FILE_NAME);
    ASSERT_GT(taskBytes.size(), sizeof(score_task_file::TaskFileHeader)) << "missing/short " << PRODUCTION_TASK_FILE_NAME;
    const TaskBlocks tb = taskSubview<ProductionConfig>(taskBytes);

    runWorkers(workerThreadCount(pubkeys.size()), [&](unsigned int threadIdx, unsigned int numThreads)
    {
        auto engine = makeEngine<ProductionConfig>(tb.topo, tb.data);
        if (!engine)
        {
            return;
        }
        for (unsigned long long s = threadIdx; s < pubkeys.size(); s += numThreads)
        {
            const unsigned char* pool = pools[poolIndex[s]].data();
            // Control/output come from the mining seed (as the tool's initialize did); derive before scoring.
            engine->deriveControlOutput(uniqueSeeds[poolIndex[s]].m256i_u8, pool);
            const score_engine::Rating rating = engine->computeScore(pubkeys[s].m256i_u8, nonces[s].m256i_u8, pool);
            EXPECT_EQ(rating.error, golden[s]) << "gt_production row " << s;
            EXPECT_EQ(rating.shift, goldenShift[s]) << "gt_production row " << s;
        }
    });
}

// Ant-colony score seam (deriveRootANN + computeScoreFromParent)
TEST(TestQubicScoreFunction, Bpp9000AntColonyRegression)
{
    // Group by chain each chain is a lineage - level 0 extends the derived root, level i extends level i-1's bestANN.
    auto rows = readCSV(PRODUCTION_ANT_FILE_NAME);
    ASSERT_GT(rows.size(), 1u) << "missing/empty " << PRODUCTION_ANT_FILE_NAME;

    struct AntNode
    {
        m256i nonce;
        m256i anchor;
        unsigned int score;
        unsigned int shift;
        unsigned int annHash;
    };
    struct AntChain
    {
        m256i pubkey;
        unsigned int poolIndex;
        std::vector<AntNode> nodes;   // indexed by depth
    };
    std::vector<AntChain> chains;
    std::vector<int> chainIds;        // chain id per slot, first-seen order
    std::vector<m256i> uniqueSeeds;

    for (unsigned long long i = 1; i < rows.size(); ++i)
    {
        const int chainId = std::stoi(trim(rows[i][0]));
        const int depth = std::stoi(trim(rows[i][1]));
        const m256i pubkey = hexTo32Bytes(trim(rows[i][2]), 32);
        const m256i seed = hexTo32Bytes(trim(rows[i][5]), 32);

        unsigned int sidx = (unsigned int)uniqueSeeds.size();
        for (unsigned int k = 0; k < uniqueSeeds.size(); ++k)
        {
            if (memcmp(uniqueSeeds[k].m256i_u8, seed.m256i_u8, 32) == 0)
            {
                sidx = k;
                break;
            }
        }
        if (sidx == uniqueSeeds.size())
        {
            uniqueSeeds.push_back(seed);
        }

        size_t cidx = chains.size();
        for (size_t k = 0; k < chainIds.size(); ++k)
        {
            if (chainIds[k] == chainId)
            {
                cidx = k;
                break;
            }
        }
        if (cidx == chains.size())
        {
            chainIds.push_back(chainId);
            AntChain created;
            created.pubkey = pubkey;
            created.poolIndex = sidx;
            chains.push_back(created);
        }

        AntNode node;
        node.nonce = hexTo32Bytes(trim(rows[i][3]), 32);
        node.anchor = hexTo32Bytes(trim(rows[i][4]), 32);
        node.shift = (unsigned int)std::stoul(trim(rows[i][6]));
        node.score = (unsigned int)std::stoul(trim(rows[i][7]));
        ASSERT_GE(rows[i].size(), 9u) << "gt_ant row " << i << " has no childAnnHash";
        node.annHash = (unsigned int)std::stoul(trim(rows[i][8]), nullptr, 16);

        AntChain& chain = chains[cidx];
        if ((size_t)depth >= chain.nodes.size())
        {
            chain.nodes.resize((size_t)depth + 1);
        }
        chain.nodes[(size_t)depth] = node;
    }
    ASSERT_FALSE(chains.empty());

    std::vector<std::vector<unsigned char>> pools(uniqueSeeds.size());
    for (size_t k = 0; k < uniqueSeeds.size(); ++k)
    {
        generatePool(uniqueSeeds[k], pools[k]);
    }

    auto taskBytes = readBinaryFile(PRODUCTION_TASK_FILE_NAME);
    ASSERT_GT(taskBytes.size(), sizeof(score_task_file::TaskFileHeader)) << "missing/short " << PRODUCTION_TASK_FILE_NAME;
    const TaskBlocks tb = taskSubview<ProductionConfig>(taskBytes);

    // Thread across chains; a chain is sequential (each node's bestANN feeds the next depth's parent).
    runWorkers(workerThreadCount(chains.size()), [&](unsigned int threadIdx, unsigned int numThreads)
    {
        auto engine = makeEngine<ProductionConfig>(tb.topo, tb.data);
        if (!engine)
        {
            return;
        }
        for (size_t ci = threadIdx; ci < chains.size(); ci += numThreads)
        {
            const AntChain& chain = chains[ci];
            const unsigned char* pool = pools[chain.poolIndex].data();

            // Control/output from the mining seed, then the identity's own root from its public key.
            engine->deriveControlOutput(uniqueSeeds[chain.poolIndex].m256i_u8, pool);

            score_engine::ScoreBpp9000<ProductionConfig>::ANN parent;
            engine->deriveRootANN(chain.pubkey.m256i_u8, pool, parent);   // depth 0's parent = this identity's own root
            unsigned long long parentShift = 0;   // the identity's root sits at frame 0
            for (size_t d = 0; d < chain.nodes.size(); ++d)
            {
                const AntNode& node = chain.nodes[d];
                const score_engine::Rating rating = engine->computeScoreFromParent(
                    parent, parentShift, chain.pubkey.m256i_u8, node.nonce.m256i_u8, node.anchor.m256i_u8, pool);
                EXPECT_EQ(rating.error, node.score) << "gt_ant chain " << ci << " depth " << d;
                EXPECT_EQ(rating.shift, node.shift) << "gt_ant chain " << ci << " depth " << d;
                engine->getBestANN(parent);   // this node becomes the next depth's parent
                EXPECT_EQ(childAnnHash<ProductionConfig>(parent), node.annHash) << "gt_ant chain " << ci << " depth " << d;
                parentShift = rating.shift;
            }
        }
    });
}

// TestBpp9000, internal score vs the score reference from Qiner
TEST(TestQubicScoreFunction, Bpp9000EngineVsReference)
{
    if (gCompareReference)
    {
        std::vector<m256i> seeds, pubkeys, nonces;
        loadSamples(seeds, pubkeys, nonces, ENGINE_VS_REFERENCE_SAMPLES);

        runRefVsEngine(seeds, pubkeys, nonces);
    }
}

static void runBpp9000ProfileForMode(unsigned char mode, const char* modeName)
{
    using Cfg = ProductionConfig;

    std::vector<m256i> seeds, pubkeys, nonces;
    loadProfileSamples(seeds, pubkeys, nonces, PROFILING_NUMBER_OF_SAMPLES);
    if (seeds.empty())
    {
        return;
    }

    // Force every sample onto the requested mode, keeping its L, so each mode's cost is timed on the same inputs.
    for (m256i& n : nonces)
    {
        n.m256i_u8[1] = (unsigned char)((n.m256i_u8[1] & 0x0F) | (mode << 4));
    }

    std::vector<unsigned char> pool;
    generatePool(seeds[0], pool);

    // Profile against a synthetic in-memory task (valid random topology + data built for this config),
    // so a parameter change (e.g. population) needs only a rebuild - no regenerated task file.
    std::vector<unsigned char> topo, data;
    buildSyntheticTask<Cfg>(pool.data(), topo, data);

    // Discard any scope measurements accumulated by earlier tests (Bpp9000EngineVsReference and the
    // production regressions also call computeScore) so profiling.csv reflects only this run.
    gProfilingDataCollector.clear();

    const unsigned int numThreads = std::max(1U, MAX_NUMBER_OF_PROFILING_THREADS);

    // Each thread sums its own computeScore times into its own slot (distinct indices, no lock).
    std::vector<double> threadSumMs(numThreads, 0.0);
    std::vector<unsigned long long> threadCount(numThreads, 0);

    // Per-sample ratings: sample s is written by exactly one worker (disjoint stride), so no lock is needed.
    std::vector<score_engine::Rating> ratings(seeds.size(), score_engine::Rating::worst());

    runWorkers(numThreads, [&](unsigned int threadIdx, unsigned int nThreads)
    {
        auto engine = makeEngine<Cfg>(topo.data(), data.data());
        if (!engine)
        {
            return;
        }
        engine->deriveControlOutput(seeds[0].m256i_u8, pool.data());
        for (unsigned long long s = threadIdx; s < seeds.size(); s += nThreads)
        {
            const m256i& n = nonces[s];

            const auto callStart = std::chrono::steady_clock::now();
            const score_engine::Rating rating = engine->computeScore(pubkeys[s].m256i_u8, n.m256i_u8, pool.data());
            const auto callEnd = std::chrono::steady_clock::now();
            ratings[s] = rating;

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

    std::cout << "[bpp9000 profile] mode " << modeName << " config "
              << Cfg::numberOfInputNeurons << "-" << Cfg::numberOfOutputNeurons << "-" << Cfg::sequenceLength
              << "-" << Cfg::windowWidth << "-" << Cfg::maxNumberOfTicks << "-" << Cfg::numberOfNeighbors
              << "-" << Cfg::populationThreshold << "-" << Cfg::numberOfMutations << "-" << Cfg::solutionThreshold
              << "-" << Cfg::shiftCap
              << " : avg " << avgMs << " ms/solution" << std::endl;

    // Error and shift distribution over the same samples
    const unsigned int infiniteError = score_engine::ScoreBpp9000<Cfg>::INFINITE_ERROR;
    unsigned long long validCount = 0;
    unsigned long long timeoutCount = 0;
    unsigned long long scoreSum = 0;
    unsigned int scoreMin = infiniteError;
    unsigned int scoreMax = 0;
    // shift drives the cost: advanceShift() runs one score() per frame it climbs.
    unsigned long long shiftSum = 0;
    unsigned int shiftMin = 0xFFFFFFFFU;
    unsigned int shiftMax = 0;
    for (unsigned long long s = 0; s < ratings.size(); ++s)
    {
        const score_engine::Rating& rt = ratings[s];
        if (!rt.isValid())
        {
            timeoutCount++;
            continue;
        }
        validCount++;
        scoreSum += rt.error;
        if (rt.error < scoreMin)
        {
            scoreMin = rt.error;
        }
        if (rt.error > scoreMax)
        {
            scoreMax = rt.error;
        }
        shiftSum += rt.shift;
        if (rt.shift < shiftMin)
        {
            shiftMin = rt.shift;
        }
        if (rt.shift > shiftMax)
        {
            shiftMax = rt.shift;
        }
    }
    const double scoreMean = (validCount > 0) ? ((double)scoreSum / (double)validCount) : 0.0;
    if (validCount == 0)
    {
        scoreMin = 0;
    }

    std::cout << "[bpp9000 profile] mode " << modeName << " error (valid " << validCount << ", timeout " << timeoutCount << ")"
              << " : min " << scoreMin << " mean " << scoreMean << " max " << scoreMax << std::endl;
    if (validCount > 0)
    {
        std::cout << "[bpp9000 profile] mode " << modeName << " shift"
                  << " : min " << shiftMin << " mean " << ((double)shiftSum / (double)validCount)
                  << " max " << shiftMax << " (cap " << Cfg::shiftCap << ")" << std::endl;
    }

    // Dump the PROFILE_NAMED_SCOPE breakdown (per-scope count + avg/min/max microseconds) to profiling.csv.
    gProfilingDataCollector.writeToFile();
    std::cout << "[bpp9000 profile] wrote profiling.csv (scopes: computeScore/score)" << std::endl;
}


#if ENABLE_PROFILING
TEST(TestQubicScoreFunction, Bpp9000Profile)
{
    runBpp9000ProfileForMode(score_engine::BPP9000_MODE_START, "START");
    runBpp9000ProfileForMode(score_engine::BPP9000_MODE_WIRING, "WIRING");
    runBpp9000ProfileForMode(score_engine::BPP9000_MODE_LUT, "LUT");
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
    if (f.engine == nullptr)
    {
        return false;
    }
    // Global control/output from the sample's mining seed, so score()/computeScore* have a valid clock.
    f.engine->deriveControlOutput(seeds[0].m256i_u8, f.pool.data());
    return true;
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

// Ant nonce: nonce[1] packs L (bits 0-3) + mode (bits 4-5), nonce[2] = K; mode defaults to WIRING, the only canonical mode.
static m256i makeAntNonce(unsigned char L, unsigned char K, unsigned char tag,
                          unsigned char mode = score_engine::BPP9000_MODE_WIRING)
{
    m256i n = m256i::zero();
    n.m256i_u8[0] = (unsigned char)score_engine::AlgoType::Bpp9000;
    n.m256i_u8[1] = (unsigned char)((L & 0x0F) | ((mode & 0x03) << 4));
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
        const unsigned int sc = engine.computeScoreFromParent(parent, 0, pk.m256i_u8, n.m256i_u8,
                                                              anchor.m256i_u8, pool).error;
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
// the engine start directly from the best/final network
TEST(TestQubicScoreAntColony, BestAnnReproducesReturnedScore)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(1);
    const m256i nonce = makeAntNonce(3, 0, 11);

    const unsigned int best = f.engine->computeScore(pk.m256i_u8, nonce.m256i_u8, f.pool.data()).error;
    // Re-score the network the walk kept, taken out and put back through the public form - this also
    // exercises the compact/expand round trip the tree relies on.
    AntEngine::ANN bestAnn;
    f.engine->getBestANN(bestAnn);
    f.engine->expand(bestAnn);
    EXPECT_EQ(f.engine->score(), best);
}

// Make sure the score at the full computeScoreFromParent flow have the same score with
// the engine start directly from the best/final network
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
        root, 0, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data()).error;

    // Re-score the network the walk kept, taken out and put back through the public form - this also
    // exercises the compact/expand round trip the tree relies on.
    AntEngine::ANN bestAnn;
    f.engine->getBestANN(bestAnn);
    f.engine->expand(bestAnn);
    EXPECT_EQ(f.engine->score(), childScore);
}

// The ANN is exactly wiring + start + LUTs (no padding); a derived root's values are all in range.
TEST(TestQubicScoreAntColony, RootAnnIsTritValuedAndInRange)
{
    static_assert(sizeof(AntEngine::ANN)
                      == AntEngine::numberOfLinks * sizeof(unsigned short)
                         + AntEngine::maxNumberOfNeurons
                         + AntEngine::maxNumberOfNeurons * AntEngine::lutSize,
                  "ANN must be wiring + start + LUT with no padding");

    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(3);
    AntEngine::ANN root;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), root);

    for (unsigned long long i = 0; i < sizeof(root.lut); ++i)
    {
        ASSERT_LT(root.lut[i], 3) << "LUT byte " << i << " of the returned ANN is not a trit";
    }
    for (unsigned long long n = 0; n < AntEngine::maxNumberOfNeurons; ++n)
    {
        ASSERT_LT(root.initialNeuronValues[n], 3) << "start byte " << n << " is not a trit";
    }
    for (unsigned long long i = 0; i < AntEngine::numberOfLinks; ++i)
    {
        ASSERT_LT(root.neighbor[i], AntCfg::populationThreshold) << "neighbor " << i << " is out of range";
    }
}

// expand() takes the parent's bytes as they are, and both the tick's LUT offset and the store's base-3
// encoding assume trits. A parent carrying anything else is refused before it is scored.
TEST(TestQubicScoreAntColony, NonTritParentAnnIsRejected)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i pk = makePubkey(11);
    const m256i anchor = makePubkey(31);
    const m256i nonce = makeAntNonce(3, 0, 91);

    static AntEngine::ANN parent;
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), parent);
    ASSERT_TRUE(f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data()).isValid()) << "the untouched root must score";

    const unsigned char goodLut = parent.lut[0];
    parent.lut[0] = 3;
    EXPECT_FALSE(f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data()).isValid()) << "a LUT byte above 2 was accepted";
    parent.lut[0] = goodLut;

    // The last neuron, so a scan that stops early is caught.
    const unsigned long long last = AntEngine::maxNumberOfNeurons - 1;
    parent.initialNeuronValues[last] = 3;
    EXPECT_FALSE(f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data()).isValid()) << "a start byte above 2 was accepted";
}

// expand/compact must be lossless, since every parent read from the tree goes through expand and
// every child written back goes through compact.
TEST(TestQubicScoreAntColony, AnnSurvivesExpandAndCompact)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    AntEngine::ANN original;
    f.engine->deriveRootANN(makePubkey(44).m256i_u8, f.pool.data(), original);

    f.engine->expand(original);
    AntEngine::ANN restored;
    f.engine->compact(restored);

    EXPECT_EQ(memcmp(&restored, &original, sizeof(original)), 0) << "expand/compact is not lossless";
}

// The root is per identity (pubkey over the epoch pool): same pubkey -> same root, different -> different.
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
    f.engine->deriveRootANN(pkB.m256i_u8, f.pool.data(), b1);
    f.engine->deriveRootANN(pkA.m256i_u8, f.pool.data(), a2);

    EXPECT_EQ(memcmp(&a1, &a2, sizeof(a1)), 0) << "root depends on engine state";
    EXPECT_NE(memcmp(&a1, &b1, sizeof(a1)), 0) << "two different pubkeys share a root";
}

// A child is a function of (parent, pubkey, nonce, anchor). Same inputs must give the same score AND
// the same inherited network; a different parent must not give the same child. The second parent is a
// mutated child of the root (built here), not another identity's root.
TEST(TestQubicScoreAntColony, ChildIsDeterministicAndInheritsParent)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i seed = makePubkey(4);
    const m256i pk = makePubkey(6);
    const m256i nonce = makeAntNonce(5, 3, 41);
    const m256i anchor = makePubkey(12);

    AntEngine::ANN parentA;
    f.engine->deriveRootANN(seed.m256i_u8, f.pool.data(), parentA);

    m256i improvingNonce;
    ASSERT_TRUE(findImprovingNonce(*f.engine, parentA, pk, anchor, f.pool.data(), improvingNonce))
        << "no nonce improved on the root, so no distinct second parent can be built";
    f.engine->computeScoreFromParent(parentA, 0, pk.m256i_u8, improvingNonce.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN parentB;
    f.engine->getBestANN(parentB);
    ASSERT_NE(memcmp(&parentA, &parentB, sizeof(parentA)), 0) << "the mutated child equals the root";

    const unsigned int s1 = f.engine->computeScoreFromParent(
        parentA, 0, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data()).error;
    AntEngine::ANN child1;
    f.engine->getBestANN(child1);

    const unsigned int s2 = f.engine->computeScoreFromParent(
        parentA, 0, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data()).error;

    EXPECT_EQ(s1, s2) << "same inputs gave different scores";
    AntEngine::ANN child2;
    f.engine->getBestANN(child2);
    EXPECT_EQ(memcmp(&child1, &child2, sizeof(child1)), 0) << "same inputs gave a different child network";

    f.engine->computeScoreFromParent(parentB, 0, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN child3;
    f.engine->getBestANN(child3);
    EXPECT_NE(memcmp(&child1, &child3, sizeof(child1)), 0) << "the parent network was not inherited";
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

    f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, nonce.m256i_u8, anchorA.m256i_u8, f.pool.data());
    AntEngine::ANN c1;
    f.engine->getBestANN(c1);

    f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, nonce.m256i_u8, anchorB.m256i_u8, f.pool.data());
    AntEngine::ANN c2;
    f.engine->getBestANN(c2);

    EXPECT_NE(memcmp(&c1, &c2, sizeof(c1)), 0) << "anchor digest does not reach the walk";
}

// Non-canonical nonces are refused by the scorer itself, so no caller can score first and check after.
TEST(TestQubicScoreAntColony, NonCanonicalNonceIsRejected)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));

    const m256i seed = makePubkey(4);
    const m256i pk = makePubkey(10);
    const m256i anchor = makePubkey(30);
    AntEngine::ANN parent;
    f.engine->deriveRootANN(seed.m256i_u8, f.pool.data(), parent);

    constexpr unsigned char maxK = (unsigned char)AntCfg::numberOfMutations;
    const m256i good = makeAntNonce(3, 5, 62);

    // A rejected nonce and a timed-out walk
    f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, good.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN afterA;
    f.engine->getBestANN(afterA);

    // L below range, L above range, wrong algorithm slot, start-state mode, LUT mode.
    // K-over-max is only testable when numberOfMutations fits in a byte.
    static constexpr bool kOverflowTestable = (AntCfg::numberOfMutations <= 255);
    static constexpr unsigned int numberOfBadNonces = kOverflowTestable ? 6 : 5;
    m256i bad[6];
    bad[0] = makeAntNonce(0, 0, 64);
    bad[1] = makeAntNonce((unsigned char)(score_engine::BPP9000_MAX_CHANGES_PER_STEP + 1), 0, 65);
    bad[2] = makeAntNonce(3, 0, 67);
    bad[2].m256i_u8[0] = (unsigned char)score_engine::AlgoType::Neuraxon;
    bad[3] = makeAntNonce(3, 5, 68, score_engine::BPP9000_MODE_START);
    bad[4] = makeAntNonce(3, 5, 69, score_engine::BPP9000_MODE_LUT);
    if constexpr (kOverflowTestable)
    {
        bad[5] = makeAntNonce(3, (unsigned char)(maxK + 1), 66);
    }

    for (unsigned int i = 0; i < numberOfBadNonces; i++)
    {
        // The bad nonce is early rejected in computeScoreFromParent()
        EXPECT_FALSE(f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, bad[i].m256i_u8,
                     anchor.m256i_u8, f.pool.data()).isValid()) << "non-canonical nonce " << i << " accepted";

        AntEngine::ANN now;
        f.engine->getBestANN(now);
        EXPECT_EQ(memcmp(&now, &afterA, sizeof(now)), 0) << "rejected nonce " << i << " still ran the walk";
    }

    // Now after bad nonces, an improving canonical nonce on the same parent must move bestANN again
    m256i good2;
    ASSERT_TRUE(findImprovingNonce(*f.engine, parent, pk, anchor, f.pool.data(), good2))
        << "no nonce improved on the parent, so the post-rejection walk check would be vacuous";
    f.engine->computeScoreFromParent(parent, 0, pk.m256i_u8, good2.m256i_u8, anchor.m256i_u8, f.pool.data());
    AntEngine::ANN afterB;
    f.engine->getBestANN(afterB);
    EXPECT_NE(memcmp(&afterB, &afterA, sizeof(afterB)), 0) << "canonical nonce was not scored";
}


// L, mode and K boundaries of the canonical rule, checked as a pure predicate so no walk is needed.
TEST(TestQubicScoreAntColony, NonceCanonicalRuleBoundaries)
{
    using AntScorer = score_engine::ScoreBpp9000<AntCfg>;
    constexpr unsigned char maxL = (unsigned char)score_engine::BPP9000_MAX_CHANGES_PER_STEP;
    constexpr unsigned char maxK = (unsigned char)AntScorer::numberOfMutations;

    // L in [1, maxL], the wiring mode, K in [0, maxK] is canonical.
    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(1, 0, 70).m256i_u8));
    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(maxL, 0, 71).m256i_u8));
    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, maxK, 72).m256i_u8));
    EXPECT_TRUE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, 0, 74, score_engine::BPP9000_MODE_WIRING).m256i_u8));
    EXPECT_TRUE(AntScorer::isCanonicalStandaloneNonce(makeAntNonce(3, 0, 82, score_engine::BPP9000_MODE_WIRING).m256i_u8));

    // L out of range, mode 0 (the empty mode field), the start-state mode and the LUT mode are all rejected.
    // K-over-max is only testable when numberOfMutations fits in a byte.
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(0, 0, 75).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce((unsigned char)(maxL + 1), 0, 76).m256i_u8));
    if constexpr (AntScorer::numberOfMutations <= 255)
    {
        EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, (unsigned char)(maxK + 1), 77).m256i_u8));
    }
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, 0, 78, 0).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, 0, 73, score_engine::BPP9000_MODE_START).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalAntNonce(makeAntNonce(3, 0, 79, score_engine::BPP9000_MODE_LUT).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalStandaloneNonce(makeAntNonce(3, 0, 80, score_engine::BPP9000_MODE_START).m256i_u8));
    EXPECT_FALSE(AntScorer::isCanonicalStandaloneNonce(makeAntNonce(3, 0, 81, score_engine::BPP9000_MODE_LUT).m256i_u8));
}


// ---------------------------------------------------------------------------
// ScoreFunction task targets: handed in with each call, one frame plus one target per hour known at the anchor tick.

// The walk is graded on the targets passed with the call; their count sets the last frame it may reach.
TEST(TestQubicScoreTaskTargets, WalkIsGradedOnTheTargetsHandedIn)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));
    std::vector<m256i> seeds;
    std::vector<m256i> pubkeys;
    std::vector<m256i> nonces;
    loadSamples(seeds, pubkeys, nonces, 1);
    ASSERT_FALSE(seeds.empty());

    const unsigned int window = (unsigned int)AntCfg::windowWidth;
    std::vector<unsigned char> targets(f.engine->targetOutputs, f.engine->targetOutputs + AntCfg::sequenceLength);

    auto scoreFunction = std::make_unique<ScoreFunction<1>>();
    scoreFunction->initMemory();
    scoreFunction->initMiningData(seeds[0]);

    const m256i pk = makePubkey(31);
    const m256i nonce = makeAntNonce(3, 0, 33);
    const m256i anchor = makePubkey(32);
    auto wrapperAnn = std::make_unique<AntEngine::ANN>();
    auto engineAnn = std::make_unique<AntEngine::ANN>();
    auto rootAnn = std::make_unique<AntEngine::ANN>();
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), *rootAnn);

    // No targets, and less than one full frame.
    EXPECT_FALSE(scoreFunction->computeAntChildScore(0, nullptr, 0, targets.data(), 0, pk, nonce, anchor, *wrapperAnn).isValid());
    EXPECT_FALSE(scoreFunction->computeAntChildScore(0, nullptr, 0, targets.data(), window - 1, pk, nonce, anchor,
        *wrapperAnn).isValid());

    // One frame: the walk stays at frame 0.
    score_engine::Rating wrapperRating = scoreFunction->computeAntChildScore(0, nullptr, 0, targets.data(), window, pk, nonce,
        anchor, *wrapperAnn);
    score_engine::Rating engineRating = f.engine->computeScoreFromParent(*rootAnn, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data(), 0);
    f.engine->getBestANN(*engineAnn);
    ASSERT_TRUE(wrapperRating.isValid());
    EXPECT_EQ(wrapperRating.error, engineRating.error);
    EXPECT_EQ(wrapperRating.shift, 0u);
    EXPECT_EQ(engineRating.shift, 0u);
    EXPECT_EQ(memcmp(wrapperAnn.get(), engineAnn.get(), sizeof(*engineAnn)), 0);

    // Three more hours.
    wrapperRating = scoreFunction->computeAntChildScore(0, nullptr, 0, targets.data(), window + 3, pk, nonce, anchor, *wrapperAnn);
    engineRating = f.engine->computeScoreFromParent(*rootAnn, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data(), 3);
    f.engine->getBestANN(*engineAnn);
    ASSERT_TRUE(wrapperRating.isValid());
    EXPECT_EQ(wrapperRating.error, engineRating.error);
    EXPECT_EQ(wrapperRating.shift, engineRating.shift);
    EXPECT_LE(wrapperRating.shift, 3u);
    EXPECT_EQ(memcmp(wrapperAnn.get(), engineAnn.get(), sizeof(*engineAnn)), 0);

    // The limit follows the target count of each call, not the call before.
    wrapperRating = scoreFunction->computeAntChildScore(0, nullptr, 0, targets.data(), window, pk, nonce, anchor, *wrapperAnn);
    ASSERT_TRUE(wrapperRating.isValid());
    EXPECT_EQ(wrapperRating.shift, 0u);

    // Not more targets than the sequence holds.
    EXPECT_FALSE(scoreFunction->computeAntChildScore(0, nullptr, 0, targets.data(), (unsigned int)AntCfg::sequenceLength + 1,
        pk, nonce, anchor, *wrapperAnn).isValid());
}

// A stored parent that leaves no error on its frames was stored at frame 0 when one frame was known. One
// hour later its unchanged network holds frame 1 too: ahead of its record, not ahead of itself on that data.
TEST(TestQubicScoreTaskTargets, ChildMustBeatItsStoredParentOnTheSameData)
{
    AntFixture f;
    ASSERT_TRUE(makeAntFixture(f));
    std::vector<m256i> seeds;
    std::vector<m256i> pubkeys;
    std::vector<m256i> nonces;
    loadSamples(seeds, pubkeys, nonces, 1);
    ASSERT_FALSE(seeds.empty());

    const unsigned int window = (unsigned int)AntCfg::windowWidth;
    const std::vector<unsigned char> zeroTargets(window + 1, 0);
    auto scoreFunction = std::make_unique<ScoreFunction<1>>();
    scoreFunction->initMemory();
    scoreFunction->initMiningData(seeds[0]);
    ASSERT_TRUE(f.engine->loadTaskTargets(zeroTargets.data(), window + 1));

    // Control alternates 0, UNKNOWN; output is constant 0, so every frame of zero targets has no error.
    const m256i pk = makePubkey(41);
    const m256i anchor = makePubkey(42);
    const unsigned int control = f.engine->controlIndex;
    const unsigned int output = f.engine->outputIndex;
    auto rootAnn = std::make_unique<AntEngine::ANN>();
    f.engine->deriveRootANN(pk.m256i_u8, f.pool.data(), *rootAnn);
    auto parent = std::make_unique<AntEngine::ANN>(*rootAnn);
    for (unsigned long long j = 0; j < AntCfg::numberOfNeighbors; ++j)
    {
        parent->neighbor[control * AntCfg::numberOfNeighbors + j] = (unsigned short)control;
    }
    parent->initialNeuronValues[control] = 0;
    parent->lut[control * AntEngine::lutSize + 0] = 2;
    parent->lut[control * AntEngine::lutSize + 26] = 0;
    for (unsigned long long e = 0; e < AntEngine::lutSize; ++e)
    {
        parent->lut[output * AntEngine::lutSize + e] = 0;
    }

    auto childAnn = std::make_unique<AntEngine::ANN>();
    const m256i nonce = makeAntNonce(3, 0, 43);
    const score_engine::Rating onNewHour = f.engine->computeScoreFromParent(*parent, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data(), 1);
    ASSERT_TRUE(onNewHour.isValid());
    EXPECT_EQ(f.engine->parentRating.error, 0u);
    EXPECT_EQ(f.engine->parentRating.shift, 1u);
    EXPECT_TRUE(onNewHour.isBetterThan(score_engine::Rating{ 0, 0 }));
    EXPECT_FALSE(onNewHour.isBetterThan(f.engine->parentRating));
    EXPECT_FALSE(scoreFunction->computeAntChildScore(0, parent.get(), 0, zeroTargets.data(), window + 1, pk, nonce, anchor, *childAnn).isValid());

    // The identity's root handed in as a stored parent can be beaten, and such a child keeps its score.
    bool improved = false;
    for (unsigned char tag = 50; tag < 56 && !improved; ++tag)
    {
        const m256i n = makeAntNonce(3, 0, tag);
        const score_engine::Rating eng = f.engine->computeScoreFromParent(*rootAnn, 0, pk.m256i_u8, n.m256i_u8,
            anchor.m256i_u8, f.pool.data(), 1);
        const score_engine::Rating wrapped = scoreFunction->computeAntChildScore(0, rootAnn.get(), 0, zeroTargets.data(), window + 1, pk, n, anchor,
            *childAnn);
        if (eng.isValid() && eng.isBetterThan(f.engine->parentRating))
        {
            improved = true;
            EXPECT_EQ(wrapped.error, eng.error);
            EXPECT_EQ(wrapped.shift, eng.shift);
        }
        else
        {
            EXPECT_FALSE(wrapped.isValid());
        }
    }
    EXPECT_TRUE(improved) << "no nonce improved on the root";

    // The root is not a stored parent: its child gets the walk's rating whether or not it beats the root.
    const score_engine::Rating rootChild = f.engine->computeScoreFromParent(*rootAnn, 0, pk.m256i_u8, nonce.m256i_u8,
        anchor.m256i_u8, f.pool.data(), 1);
    const score_engine::Rating wrappedRootChild = scoreFunction->computeAntChildScore(0, nullptr, 0, zeroTargets.data(), window + 1, pk, nonce, anchor,
        *childAnn);
    EXPECT_EQ(wrappedRootChild.error, rootChild.error);
    EXPECT_EQ(wrappedRootChild.shift, rootChild.shift);
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

// =============================================================================
// Agreement with the reference on cases the goldens above do not reach

using TimeoutConfig = score_engine::Bpp9000Params<
    BPP9000_NUMBER_OF_INPUT_NEURONS, BPP9000_NUMBER_OF_OUTPUT_NEURONS, BPP9000_SEQUENCE_LENGTH,
    BPP9000_WINDOW_WIDTH, 2 * BPP9000_WINDOW_WIDTH + 3, BPP9000_NUMBER_OF_NEIGHBORS,
    256, 100, BPP9000_SOLUTION_THRESHOLD_DEFAULT, BPP9000_SHIFT_CAP>;

using Pop64Config = score_engine::Bpp9000Params<
    BPP9000_NUMBER_OF_INPUT_NEURONS, BPP9000_NUMBER_OF_OUTPUT_NEURONS, BPP9000_SEQUENCE_LENGTH,
    BPP9000_WINDOW_WIDTH, BPP9000_MAX_NUMBER_OF_TICKS, BPP9000_NUMBER_OF_NEIGHBORS,
    64, BPP9000_NUMBER_OF_MUTATIONS, BPP9000_SOLUTION_THRESHOLD_DEFAULT, BPP9000_SHIFT_CAP>;

// A walk start that masters frames 0 and 1 and times out in frame 2 must report {INF, 2}, as the reference does.
TEST(TestQubicScoreFunction, Bpp9000TimeoutAfterAdvanceMatchesReference)
{
    using Cfg = TimeoutConfig;
    using Engine = score_engine::ScoreBpp9000<Cfg>;
    using Ref = score_bpp9000_reference::Miner<Cfg>;

    std::vector<m256i> seeds;
    std::vector<m256i> pubkeys;
    std::vector<m256i> nonces;
    loadSamples(seeds, pubkeys, nonces, 1);
    ASSERT_FALSE(seeds.empty());
    std::vector<unsigned char> pool;
    generatePool(seeds[0], pool);

    const std::vector<unsigned char> inputs(Cfg::sequenceLength * Cfg::numberOfInputNeurons, 0);
    const std::vector<unsigned char> targets(Cfg::sequenceLength * Cfg::numberOfOutputNeurons, 0);
    std::vector<unsigned char> data(score_task_file::dataBytes((unsigned int)Cfg::numberOfInputNeurons,
        (unsigned int)Cfg::numberOfOutputNeurons, Cfg::sequenceLength));
    score_task_file::packDataBlock((unsigned int)Cfg::numberOfInputNeurons, (unsigned int)Cfg::numberOfOutputNeurons,
        Cfg::sequenceLength, inputs.data(), targets.data(), data.data());

    auto engine = makeEngine<Cfg>(nullptr, data.data());
    ASSERT_TRUE(engine);
    engine->deriveControlOutput(seeds[0].m256i_u8, pool.data());
    auto ref = std::make_unique<Ref>();
    ref->initialize(seeds[0].m256i_u8);
    ASSERT_TRUE(ref->loadTaskFromMemory(nullptr, data.data()));
    const unsigned int control = ref->controlIndex;
    const unsigned int output = ref->outputIndex;
    ASSERT_EQ(engine->controlIndex, control);
    ASSERT_EQ(engine->outputIndex, output);

    const m256i pk = makePubkey(41);
    const m256i anchor = makePubkey(42);
    auto parent = std::make_unique<Engine::ANN>();
    engine->deriveRootANN(pk.m256i_u8, pool.data(), *parent);
    for (unsigned long long j = 0; j < Cfg::numberOfNeighbors; ++j)
    {
        parent->neighbor[control * Cfg::numberOfNeighbors + j] = (unsigned short)control;
    }
    parent->initialNeuronValues[control] = 0;
    parent->lut[control * Engine::lutSize + 0] = 2;
    parent->lut[control * Engine::lutSize + 26] = 0;
    for (unsigned long long e = 0; e < Engine::lutSize; ++e)
    {
        parent->lut[output * Engine::lutSize + e] = 0;
    }

    referenceLoadAnn<Cfg>(*ref, *parent);
    ref->shift = 0;
    const score_engine::Rating start = ref->advanceShift();
    ASSERT_EQ(start.error, Ref::INFINITE_ERROR);
    ASSERT_EQ(start.shift, 2u);
    ASSERT_EQ(ref->shift, 2ull);

    auto engineAnn = std::make_unique<Engine::ANN>();
    auto refAnn = std::make_unique<Engine::ANN>();
    const unsigned char lValues[3] = { 1, 4, 10 };
    const unsigned char kValues[3] = { 0, 7, 100 };
    for (unsigned int li = 0; li < 3; ++li)
    {
        for (unsigned int ki = 0; ki < 3; ++ki)
        {
            const m256i nonce = makeAntNonce(lValues[li], kValues[ki], (unsigned char)(60 + 3 * li + ki));
            const score_engine::Rating eng = engine->computeScoreFromParent(*parent, 0, pk.m256i_u8, nonce.m256i_u8,
                anchor.m256i_u8, pool.data());
            engine->getBestANN(*engineAnn);
            const score_engine::Rating r = referenceScoreFromParent<Cfg>(*ref, *parent, 0, pk, nonce, anchor, *refAnn);
            if (kValues[ki] == Cfg::numberOfMutations)
            {
                EXPECT_EQ(r.error, Ref::INFINITE_ERROR) << "L " << (int)lValues[li];
                EXPECT_EQ(r.shift, 2u) << "L " << (int)lValues[li];
            }
            EXPECT_EQ(eng.error, r.error) << "L " << (int)lValues[li] << " K " << (int)kValues[ki];
            EXPECT_EQ(eng.shift, r.shift) << "L " << (int)lValues[li] << " K " << (int)kValues[ki];
            EXPECT_EQ(engine->parentRating.error, start.error) << "L " << (int)lValues[li] << " K " << (int)kValues[ki];
            EXPECT_EQ(engine->parentRating.shift, start.shift) << "L " << (int)lValues[li] << " K " << (int)kValues[ki];
            EXPECT_EQ(ref->parentRating.error, start.error);
            EXPECT_EQ(ref->parentRating.shift, start.shift);
            EXPECT_EQ(memcmp(engineAnn.get(), refAnn.get(), sizeof(*refAnn)), 0)
                << "best ANN bytes, L " << (int)lValues[li] << " K " << (int)kValues[ki];
        }
    }
}

using LimitConfig = score_engine::Bpp9000Params<
    BPP9000_NUMBER_OF_INPUT_NEURONS, BPP9000_NUMBER_OF_OUTPUT_NEURONS, BPP9000_SEQUENCE_LENGTH,
    BPP9000_WINDOW_WIDTH, BPP9000_MAX_NUMBER_OF_TICKS, BPP9000_NUMBER_OF_NEIGHBORS,
    256, 100, BPP9000_SOLUTION_THRESHOLD_DEFAULT, BPP9000_SHIFT_CAP>;

// A parent that masters every frame slides to the shift limit and no further. The targets after the
// limit are set to the wrong value, so a walk that read them would stop early or score differently.
TEST(TestQubicScoreFunction, Bpp9000ShiftLimitMatchesReference)
{
    using Cfg = LimitConfig;
    using Engine = score_engine::ScoreBpp9000<Cfg>;
    using Ref = score_bpp9000_reference::Miner<Cfg>;

    std::vector<m256i> seeds;
    std::vector<m256i> pubkeys;
    std::vector<m256i> nonces;
    loadSamples(seeds, pubkeys, nonces, 1);
    ASSERT_FALSE(seeds.empty());
    std::vector<unsigned char> pool;
    generatePool(seeds[0], pool);

    const std::vector<unsigned char> inputs(Cfg::sequenceLength * Cfg::numberOfInputNeurons, 0);
    const std::vector<unsigned char> zeroTargets(Cfg::sequenceLength * Cfg::numberOfOutputNeurons, 0);
    std::vector<unsigned char> data(score_task_file::dataBytes((unsigned int)Cfg::numberOfInputNeurons,
        (unsigned int)Cfg::numberOfOutputNeurons, Cfg::sequenceLength));
    score_task_file::packDataBlock((unsigned int)Cfg::numberOfInputNeurons, (unsigned int)Cfg::numberOfOutputNeurons,
        Cfg::sequenceLength, inputs.data(), zeroTargets.data(), data.data());

    auto engine = makeEngine<Cfg>(nullptr, data.data());
    ASSERT_TRUE(engine);
    engine->deriveControlOutput(seeds[0].m256i_u8, pool.data());
    auto ref = std::make_unique<Ref>();
    ref->initialize(seeds[0].m256i_u8);
    ASSERT_TRUE(ref->loadTaskFromMemory(nullptr, data.data()));
    const unsigned int control = ref->controlIndex;
    const unsigned int output = ref->outputIndex;
    ASSERT_EQ(engine->controlIndex, control);
    ASSERT_EQ(engine->outputIndex, output);

    // Control alternates 0, UNKNOWN; output is constant 0.
    const m256i pk = makePubkey(51);
    const m256i anchor = makePubkey(52);
    auto parent = std::make_unique<Engine::ANN>();
    engine->deriveRootANN(pk.m256i_u8, pool.data(), *parent);
    for (unsigned long long j = 0; j < Cfg::numberOfNeighbors; ++j)
    {
        parent->neighbor[control * Cfg::numberOfNeighbors + j] = (unsigned short)control;
    }
    parent->initialNeuronValues[control] = 0;
    parent->lut[control * Engine::lutSize + 0] = 2;
    parent->lut[control * Engine::lutSize + 26] = 0;
    for (unsigned long long e = 0; e < Engine::lutSize; ++e)
    {
        parent->lut[output * Engine::lutSize + e] = 0;
    }

    auto engineAnn = std::make_unique<Engine::ANN>();
    auto refAnn = std::make_unique<Engine::ANN>();
    std::vector<unsigned char> targets(Cfg::sequenceLength);
    const unsigned long long limits[4] = { 0, 1, 37, Cfg::shiftCap };
    const unsigned char kValues[2] = { 0, 7 };
    for (unsigned int li = 0; li < 4; ++li)
    {
        const unsigned long long limit = limits[li];
        const unsigned long long known = Cfg::windowWidth + limit;
        for (unsigned long long t = 0; t < Cfg::sequenceLength; ++t)
        {
            targets[t] = (t < known) ? 0 : 1;
        }
        ASSERT_TRUE(engine->loadTaskTargets(targets.data(), targets.size()));
        memcpy(ref->targetOutputs, targets.data(), targets.size());
        ref->shiftLimit = limit;

        referenceLoadAnn<Cfg>(*ref, *parent);
        ref->shift = 0;
        const score_engine::Rating start = ref->advanceShift();
        ASSERT_EQ(start.error, 0u) << "limit " << limit;
        ASSERT_EQ(start.shift, (unsigned int)limit) << "limit " << limit;

        for (unsigned int ki = 0; ki < 2; ++ki)
        {
            const m256i nonce = makeAntNonce(2, kValues[ki], (unsigned char)(70 + 2 * li + ki));
            const score_engine::Rating eng = engine->computeScoreFromParent(*parent, 0, pk.m256i_u8, nonce.m256i_u8,
                anchor.m256i_u8, pool.data(), limit);
            engine->getBestANN(*engineAnn);
            const score_engine::Rating r = referenceScoreFromParent<Cfg>(*ref, *parent, 0, pk, nonce, anchor, *refAnn);
            EXPECT_LE(eng.shift, (unsigned int)limit) << "limit " << limit;
            EXPECT_EQ(eng.error, r.error) << "limit " << limit << " K " << (int)kValues[ki];
            EXPECT_EQ(eng.shift, r.shift) << "limit " << limit << " K " << (int)kValues[ki];
            EXPECT_EQ(engine->parentRating.error, start.error) << "limit " << limit << " K " << (int)kValues[ki];
            EXPECT_EQ(engine->parentRating.shift, start.shift) << "limit " << limit << " K " << (int)kValues[ki];
            EXPECT_EQ(ref->parentRating.error, start.error);
            EXPECT_EQ(ref->parentRating.shift, start.shift);
            EXPECT_FALSE(eng.isBetterThan(start)) << "a parent with no error left cannot be beaten on its own data";
            EXPECT_EQ(memcmp(engineAnn.get(), refAnn.get(), sizeof(*refAnn)), 0)
                << "best ANN bytes, limit " << limit << " K " << (int)kValues[ki];
        }
    }

    // A walk whose limit drops below its shift (the engine is zeroed at the epoch switch) stops at once.
    engine->expand(*parent);
    engine->shift = 3;
    engine->shiftLimit = 0;
    const score_engine::Rating interrupted = engine->advanceShift();
    EXPECT_EQ(interrupted.shift, 3u);
    EXPECT_EQ(engine->shift, 3ull);

    // A limit above the cap, or a parent already past the limit, cannot be scored.
    const m256i nonce = makeAntNonce(2, 0, 90);
    EXPECT_FALSE(engine->computeScoreFromParent(*parent, 0, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, pool.data(),
        Cfg::shiftCap + 1).isValid());
    EXPECT_FALSE(engine->computeScoreFromParent(*parent, 6, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, pool.data(),
        5).isValid());
    EXPECT_TRUE(engine->computeScoreFromParent(*parent, 5, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, pool.data(),
        5).isValid());

    // A refused walk leaves no parent rating behind from the walk before it.
    EXPECT_TRUE(engine->parentRating.isValid());
    EXPECT_FALSE(engine->computeScoreFromParent(*parent, 6, pk.m256i_u8, nonce.m256i_u8, anchor.m256i_u8, pool.data(),
        5).isValid());
    EXPECT_FALSE(engine->parentRating.isValid());
    EXPECT_EQ(engine->parentRating.shift, 0u);
}

// Population 64: the AVX2 gather tick and the AVX-512 kernel at a small population, against the reference.
TEST(TestQubicScoreFunction, Bpp9000EngineVsReferencePop64)
{
    if (gCompareReference)
    {
        std::vector<m256i> seeds;
        std::vector<m256i> pubkeys;
        std::vector<m256i> nonces;
        loadSamples(seeds, pubkeys, nonces, ENGINE_VS_REFERENCE_SAMPLES);
        runRefVsEngineConfig<Pop64Config>(seeds, pubkeys, nonces);
    }
}

// Walks from positive parents on the production task: error, shift and a hash of the best ANN bytes.
TEST(TestQubicScoreFunction, Bpp9000PositiveSampleRegression)
{
    using Engine = score_engine::ScoreBpp9000<ProductionConfig>;

    struct PositiveRow
    {
        unsigned long long chainStart;
        int parent;
        m256i pubkey;
        m256i nonce;
        m256i anchor;
        unsigned int shift;
        unsigned int score;
        unsigned int annHash;
    };

    const auto csv = readCSV(PRODUCTION_POSITIVE_FILE_NAME);
    ASSERT_GT(csv.size(), 1u) << "missing/empty " << PRODUCTION_POSITIVE_FILE_NAME;
    const m256i seed = hexTo32Bytes(trim(csv[1][6]), 32);

    std::vector<PositiveRow> rows;
    unsigned int chainCount = 0;
    for (unsigned long long i = 1; i < csv.size(); ++i)
    {
        const bool newChain = (i == 1) || (trim(csv[i][0]) != trim(csv[i - 1][0]));
        if (newChain)
        {
            if (POSITIVE_SAMPLE_CHAINS != 0 && chainCount == POSITIVE_SAMPLE_CHAINS)
            {
                break;
            }
            ++chainCount;
        }
        PositiveRow r;
        r.chainStart = newChain ? rows.size() : rows.back().chainStart;
        ASSERT_EQ(std::stoull(trim(csv[i][1])), rows.size() - r.chainStart) << "gt_ant_positive row " << i;
        r.parent = std::stoi(trim(csv[i][2]));
        r.pubkey = hexTo32Bytes(trim(csv[i][3]), 32);
        r.nonce = hexTo32Bytes(trim(csv[i][4]), 32);
        r.anchor = hexTo32Bytes(trim(csv[i][5]), 32);
        r.shift = (unsigned int)std::stoul(trim(csv[i][7]));
        r.score = (unsigned int)std::stoul(trim(csv[i][8]));
        r.annHash = (unsigned int)std::stoul(trim(csv[i][9]), nullptr, 16);
        rows.push_back(r);
    }
    ASSERT_FALSE(rows.empty());

    std::vector<char> isParent(rows.size(), 0);
    for (const PositiveRow& r : rows)
    {
        if (r.parent >= 0)
        {
            isParent[r.chainStart + (unsigned long long)r.parent] = 1;
        }
    }

    std::vector<unsigned char> pool;
    generatePool(seed, pool);
    const auto taskBytes = readBinaryFile(PRODUCTION_TASK_FILE_NAME);
    ASSERT_GT(taskBytes.size(), sizeof(score_task_file::TaskFileHeader)) << "missing/short " << PRODUCTION_TASK_FILE_NAME;
    const TaskBlocks tb = taskSubview<ProductionConfig>(taskBytes);

    std::vector<std::unique_ptr<Engine::ANN>> parentAnn(rows.size());
    for (unsigned long long k = 0; k < rows.size(); ++k)
    {
        if (isParent[k])
        {
            parentAnn[k] = std::make_unique<Engine::ANN>();
        }
    }
    std::vector<score_engine::Rating> results(rows.size(), score_engine::Rating::worst());
    std::vector<unsigned int> hashes(rows.size(), 0);

    const auto scoreRow = [&](Engine& engine, Engine::ANN& scratch, unsigned long long k)
    {
        const PositiveRow& r = rows[k];
        const Engine::ANN* parent = &scratch;
        unsigned long long parentShift = 0;
        if (r.parent < 0)
        {
            engine.deriveRootANN(r.pubkey.m256i_u8, pool.data(), scratch);
        }
        else
        {
            const unsigned long long p = r.chainStart + (unsigned long long)r.parent;
            parent = parentAnn[p].get();
            parentShift = results[p].shift;
        }
        results[k] = engine.computeScoreFromParent(*parent, parentShift, r.pubkey.m256i_u8, r.nonce.m256i_u8,
            r.anchor.m256i_u8, pool.data());
        Engine::ANN& best = isParent[k] ? *parentAnn[k] : scratch;
        engine.getBestANN(best);
        hashes[k] = childAnnHash<ProductionConfig>(best);
    };

    std::vector<unsigned long long> chainStarts;
    for (unsigned long long k = 0; k < rows.size(); ++k)
    {
        if (rows[k].chainStart == k)
        {
            chainStarts.push_back(k);
        }
    }
    chainStarts.push_back(rows.size());
    const unsigned long long numChains = chainStarts.size() - 1;

    runWorkers(workerThreadCount(numChains), [&](unsigned int threadIdx, unsigned int numThreads)
    {
        auto engine = makeEngine<ProductionConfig>(tb.topo, tb.data);
        if (!engine)
        {
            return;
        }
        engine->deriveControlOutput(seed.m256i_u8, pool.data());
        auto scratch = std::make_unique<Engine::ANN>();
        for (unsigned long long c = threadIdx; c < numChains; c += numThreads)
        {
            for (unsigned long long k = chainStarts[c]; k < chainStarts[c + 1]; ++k)
            {
                if (isParent[k])
                {
                    scoreRow(*engine, *scratch, k);
                }
            }
        }
    });

    std::vector<unsigned long long> leaves;
    for (unsigned long long k = 0; k < rows.size(); ++k)
    {
        if (!isParent[k])
        {
            leaves.push_back(k);
        }
    }
    runWorkers(workerThreadCount(leaves.size()), [&](unsigned int threadIdx, unsigned int numThreads)
    {
        auto engine = makeEngine<ProductionConfig>(tb.topo, tb.data);
        if (!engine)
        {
            return;
        }
        engine->deriveControlOutput(seed.m256i_u8, pool.data());
        auto scratch = std::make_unique<Engine::ANN>();
        for (unsigned long long i = threadIdx; i < leaves.size(); i += numThreads)
        {
            scoreRow(*engine, *scratch, leaves[i]);
        }
    });

    for (unsigned long long k = 0; k < rows.size(); ++k)
    {
        EXPECT_EQ(results[k].error, rows[k].score) << "gt_ant_positive row " << k;
        EXPECT_EQ(results[k].shift, rows[k].shift) << "gt_ant_positive row " << k;
        EXPECT_EQ(hashes[k], rows[k].annHash) << "gt_ant_positive row " << k;
    }
}
