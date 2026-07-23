#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/public_settings.h"
#include "../src/mining/score_bpp9000.h"
#include "../src/mining/task_file.h"

#include "score_params.h"

#include <vector>
#include <string>
#include <fstream>
#include <iterator>

using namespace score_params;

static const std::string TASK_FILE_NAME = "data/example_task_bpp9000.bin";

// The task file is the full production task (its N/M/P/K/T match ProductionConfig).
using StubConfig = ProductionConfig;
using StubEngine = score_engine::ScoreBpp9000<StubConfig>;

static std::vector<unsigned char> readBinaryFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// A 32-byte value with byte[0] set to the bpp9000 algorithm id and nonce[30:31] set to `v`.
static m256i stubNonce(unsigned int v)
{
    m256i n = m256i::zero();
    n.m256i_u8[0] = 1u;                       // AlgoType::Bpp9000
    n.m256i_u8[30] = (unsigned char)(v & 0xFF);
    n.m256i_u8[31] = (unsigned char)((v >> 8) & 0xFF);
    return n;
}

// The value the stub scorer must return for a given nonce.
static unsigned int stubExpected(const m256i& nonce)
{
    const unsigned int v = (unsigned int)nonce.m256i_u8[30] | ((unsigned int)nonce.m256i_u8[31] << 8);
    return v % (unsigned int)(StubEngine::numberOfWindows + 1);
}

// The scorer on this branch is a protocol-test stub: score = f(nonce[30:31]), independent of the ANN.
// This test verifies (a) the task-file load path the test team depends on still works, and (b) the stub
// mapping and the threshold gate direction (error semantics: score <= threshold passes).
TEST(TestQubicScoreFunction, Bpp9000ProtocolStub)
{
    // (a) Task-file load path.
    std::vector<unsigned char> taskBytes = readBinaryFile(TASK_FILE_NAME);
    ASSERT_GT(taskBytes.size(), sizeof(score_task_file::TaskFileHeader)) << "missing/empty " << TASK_FILE_NAME;

    const score_task_file::TaskFileHeader* h = (const score_task_file::TaskFileHeader*)taskBytes.data();
    ASSERT_EQ(h->population, (unsigned int)StubConfig::populationThreshold) << "task P != config P";
    ASSERT_EQ(h->numInputTrits, (unsigned int)StubConfig::numberOfInputNeurons) << "task N != config N";
    ASSERT_EQ(h->numOutputTrits, (unsigned int)StubConfig::numberOfOutputNeurons) << "task M != config M";
    ASSERT_EQ(h->numNeighbors, (unsigned int)StubConfig::numberOfNeighbors) << "task K != config K";
    ASSERT_GE(h->numPairs, (unsigned long long)StubConfig::sequenceLength) << "task T < config T";

    const unsigned long long topoBytes = score_task_file::topologyBytes(
        (unsigned int)StubConfig::numberOfInputNeurons, (unsigned int)StubConfig::numberOfOutputNeurons,
        (unsigned int)StubConfig::populationThreshold, (unsigned int)StubConfig::numberOfNeighbors);
    const unsigned char* topo = taskBytes.data() + sizeof(score_task_file::TaskFileHeader);
    const unsigned char* data = topo + topoBytes;

    StubEngine engine;
    engine.initMemory();
    ASSERT_TRUE(engine.loadTaskFromMemory(topo, data)) << "loadTaskFromMemory failed";

    // The stub ignores the pool, but pass a real (zeroed) buffer of the expected size.
    std::vector<unsigned char> pool(score_engine::POOL_VEC_PADDING_SIZE, 0);
    const m256i pubkey = m256i::zero();

    // (b) Stub mapping is exact across the range, including the modulo wrap-around.
    const unsigned int probes[] = { 0u, 100u, 5392u, 6000u, 8088u, 8089u, 40000u, 65535u };
    for (unsigned int v : probes)
    {
        const m256i n = stubNonce(v);
        const unsigned int got = engine.computeScore(pubkey.m256i_u8, n.m256i_u8, pool.data());
        EXPECT_EQ(got, stubExpected(n)) << "stub mapping wrong for v=" << v;
        EXPECT_LE(got, (unsigned int)StubEngine::numberOfWindows) << "score out of range for v=" << v;
    }

    // Threshold gate direction: an error at/below the default threshold passes, above it fails.
    const unsigned int threshold = BPP9000_SOLUTION_THRESHOLD_DEFAULT;
    const m256i passNonce = stubNonce(threshold - 1);
    const m256i failNonce = stubNonce(threshold + 1);
    EXPECT_LE(engine.computeScore(pubkey.m256i_u8, passNonce.m256i_u8, pool.data()), threshold);
    EXPECT_GT(engine.computeScore(pubkey.m256i_u8, failNonce.m256i_u8, pool.data()), threshold);
}
