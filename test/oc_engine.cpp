#define NO_UEFI

// gtest must come first: kangaroo_twelve.h pulls in platform/assert.h, whose
// ASSERT definition keys off whether EXPECT_TRUE is already defined. If gtest
// isn't in scope yet, assert.h takes its non-gtest branch and forward-declares
// addDebugMessageAssert() with no definition -> C2129 at link.
#include "gtest/gtest.h"

// Self-contained one-shot K12 wrapper (static/internal linkage) for the
// standalone padding test below. Do NOT include K12/kangaroo_twelve_xkcp.h
// here: its external-linkage XKCP backend would clash at link with
// test/kangaroo_twelve.cpp, which also defines those symbols.
#include "../src/kangaroo_twelve.h"
#include "../src/platform/m256.h"
#include "../src/platform/memory.h"
#include <lib/platform_common/qintrin.h>

#include <cstdint>
#include <cstring>
#include <ostream>
#include <vector>


// This build's prebuilt gtest lib does not emit testing::internal::PrintTo(
// unsigned short, ...), yet UniversalPrinter<unsigned short> references it
// whenever an EXPECT_EQ compares unsigned short operands (OcInvocationRecord
// has several: epoch, interfaceIndex, paramsSize, agreeingSigs). Provide the
// overload once here to satisfy the linker without casting at every call site.
namespace testing
{
namespace internal
{
    void PrintTo(unsigned short value, std::ostream* os)
    {
        *os << static_cast<unsigned int>(value);
    }
}
}


// ---------------------------------------------------------------------------
// Foundation test: padding determinism for OcRequest.
//
// paramsDigest is defined as a K12 hash over the raw OcRequest
// bytes as laid out in memory. For this to be consensus-safe across all
// computors (compilers, optimization levels, debug/release), every byte
// hashed — including struct padding — must be deterministic.
//
// This test does not pull in the engine itself; it verifies the standalone
// property "K12 over zero-initialized struct bytes is deterministic," which
// is independent of any engine machinery.
// ---------------------------------------------------------------------------

namespace
{
    struct TestOcRequest
    {
        unsigned long long value;
        unsigned int aux32;       // intentional second field to provoke padding on some layouts
        unsigned short aux16;
        unsigned char aux8;
    };

    void computeDigest(const TestOcRequest& request, unsigned char out[32])
    {
        KangarooTwelve(
            reinterpret_cast<const unsigned char*>(&request),
            static_cast<unsigned int>(sizeof(TestOcRequest)),
            out,
            32);
    }
}

TEST(OcAuthMessage, PaddingIsDeterministicAcrossCompilers)
{
    constexpr int iterations = 100000;

    // Generate a reference (request, digest) pair from random field values.
    TestOcRequest reference;
    setMem(&reference, sizeof(reference), 0);
    _rdrand64_step(&reference.value);
    _rdrand32_step(&reference.aux32);
    {
        unsigned int tmp;
        _rdrand32_step(&tmp);
        reference.aux16 = static_cast<unsigned short>(tmp);
        reference.aux8 = static_cast<unsigned char>(tmp >> 16);
    }

    unsigned char referenceDigest[32];
    computeDigest(reference, referenceDigest);

    for (int i = 0; i < iterations; ++i)
    {
        TestOcRequest probe;
        setMem(&probe, sizeof(probe), 0);
        probe.value = reference.value;
        probe.aux32 = reference.aux32;
        probe.aux16 = reference.aux16;
        probe.aux8 = reference.aux8;

        unsigned char probeDigest[32];
        computeDigest(probe, probeDigest);

        ASSERT_EQ(0, std::memcmp(referenceDigest, probeDigest, 32))
            << "Digest divergence at iteration " << i
            << ". Padding bytes likely not zero-initialized identically.";
    }
}


// ---------------------------------------------------------------------------
// Engine tests: state transitions, signature processing, timeout, snapshot.
//
// Uses the same heavy harness as test/oracle_engine.cpp (initSpectrum,
// initContractExec, ts.init, broadcastedComputors). Inherits from OcEngine
// to expose protected state via friend access. Tests construct fake
// computor keypairs deterministically so signatures are verifiable.
// ---------------------------------------------------------------------------

#include "oracle_testing.h" // brings in tick storage, logging test base, broadcastedComputors

#include "../src/oc_core/oc_engine.h"
#include "../src/oc_core/snapshot_files.h"
#include "../src/four_q.h"


// Fake but cryptographically valid computor keypairs derived from index.
// Each computor i has subseed[i] = K12("oc_test_subseed" || i)
// from which a real (publicKey, signingKey) pair is derived via FourQ.
struct FakeComputors
{
    m256i publicKeys[NUMBER_OF_COMPUTORS];
    m256i subseeds[NUMBER_OF_COMPUTORS];

    FakeComputors()
    {
#ifdef __AVX512F__
        initAVX512FourQConstants();
#endif

        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
        {
            // Build a unique seed for computor i and derive subseed/pubkey.
            unsigned char seedInput[16 + sizeof(unsigned int)];
            const char tag[16] = { 'o','c','_','t','e','s','t','_','s','u','b','s','e','e','d',0 };
            copyMem(seedInput, tag, sizeof(tag));
            copyMem(seedInput + sizeof(tag), &i, sizeof(i));

            KangarooTwelve(seedInput, sizeof(seedInput), subseeds[i].m256i_u8, 32);

            unsigned char privateKey[32];
            unsigned char publicKey[32];
            KangarooTwelve(subseeds[i].m256i_u8, 32, privateKey, 32);
            getPublicKey(privateKey, publicKey);
            copyMem(publicKeys[i].m256i_u8, publicKey, 32);
        }
    }
};


// TEST_F requires the fixture to derive from ::testing::Test (it provides
// SetUpTestCase/TearDownTestCase/test_info_). LoggingTest only handles
// logging init/deinit and does NOT derive from ::testing::Test, so inherit
// both.
struct OcEngineTest : public ::testing::Test, public LoggingTest
{
    FakeComputors comps;

    OcEngineTest()
    {
        EXPECT_TRUE(initSpectrum());
        EXPECT_TRUE(commonBuffers.init(1, 1024 * 1024));
        EXPECT_TRUE(initSpecialEntities());
        EXPECT_TRUE(initContractExec());
        EXPECT_TRUE(ts.init());
        EXPECT_TRUE(OI::initOracleInterfaces());
        EXPECT_TRUE(OCI::initOcInterfaces());

        // populate broadcastedComputors with our fake keys
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
            broadcastedComputors.computors.publicKeys[i] = comps.publicKeys[i];

        system.epoch = 100;
        system.tick = 1000;
        ts.beginEpoch(system.tick);
    }

    ~OcEngineTest()
    {
        deinitSpectrum();
        commonBuffers.deinit();
        deinitContractExec();
        ts.deinit();
    }
};


// Subclass exposing protected state for testing.
struct OcEngineForTest : public OcEngine
{
    OcEngineForTest(const m256i* computorPubKeys)
    {
        EXPECT_TRUE(this->init(computorPubKeys));
    }

    ~OcEngineForTest()
    {
        this->deinit();
    }

    uint32_t exposedInvocationCount() const { return this->invocationCount; }

    const OcInvocationRecord& record(uint32_t i) const
    {
        EXPECT_LT(i, this->invocationCount);
        return this->invocations[i];
    }

    const OcInFlightAuthState& authState(uint32_t slot) const
    {
        EXPECT_LT(slot, MAX_OC_IN_FLIGHT_INVOCATIONS);
        return this->inFlightStates[slot];
    }

    // agreeingSigs of a record's in-flight auth state; 0 if the slot was already freed
    // (resolved records hold no auth state).
    uint16_t agreeingSigsOf(uint32_t recordIdx) const
    {
        const OcInvocationRecord& rec = record(recordIdx);
        if (rec.inFlightSlot == OC_IN_FLIGHT_SLOT_NONE)
            return 0;
        return authState(rec.inFlightSlot).agreeingSigs;
    }

    uint32_t exposedFirstActiveIndex() const { return this->firstActiveIndex; }
};


// Build a signed OcAuthSignatureItem for the given invocation, signed by the given computor.
// Helper that mirrors what qubic.cpp's emit loop does.
static void buildSignedAuthItem(const FakeComputors& comps, unsigned int computorIdx,
    int64_t invocationId, uint16_t interfaceIndex, uint16_t epoch, const m256i& paramsDigest,
    OcAuthSignatureItem& outItem)
{
    setMem(&outItem, sizeof(outItem), 0);
    outItem.invocationId = invocationId;
    outItem.interfaceIndex = interfaceIndex;
    outItem.epoch = epoch;
    outItem.paramsDigest = paramsDigest;

    m256i authHash;
    OcEngine::computeOcAuthMessageHash(epoch, interfaceIndex, invocationId, paramsDigest, authHash);
    sign(comps.subseeds[computorIdx].m256i_u8, comps.publicKeys[computorIdx].m256i_u8,
        (const unsigned char*)&authHash, outItem.signature);
}


// Build a single-item OcAuthSignatureTransaction (without outer signature — the engine
// only verifies per-item signatures; the outer tx signature check is done elsewhere
// in tx pipeline and not exercised here).
static void buildAuthTx(const FakeComputors& comps, unsigned int computorIdx,
    int64_t invocationId, uint16_t interfaceIndex, uint16_t epoch, const m256i& paramsDigest,
    unsigned char* txBuffer)
{
    auto* tx = reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuffer);
    tx->sourcePublicKey = comps.publicKeys[computorIdx];
    tx->destinationPublicKey = m256i::zero();
    tx->amount = 0;
    tx->tick = system.tick;
    tx->inputType = OcAuthSignatureTransactionPrefix::transactionType();
    tx->inputSize = (unsigned short)(2 * sizeof(unsigned short) + sizeof(OcAuthSignatureItem));

    unsigned char* payload = txBuffer + sizeof(Transaction);
    *reinterpret_cast<unsigned short*>(payload) = 1; // itemCount
    *reinterpret_cast<unsigned short*>(payload + 2) = 0; // padding
    auto* item = reinterpret_cast<OcAuthSignatureItem*>(payload + 4);
    buildSignedAuthItem(comps, computorIdx, invocationId, interfaceIndex, epoch, paramsDigest, *item);
}


TEST_F(OcEngineTest, StartContractInvocationHappyPath)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    req.value = 0xDEADBEEFCAFEBABEull;

    const int64_t id = engine.startContractInvocation(
        /*contractIndex=*/0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    EXPECT_GE(id, 0);
    EXPECT_EQ(engine.exposedInvocationCount(), 1u);

    const auto& rec = engine.record(0);
    EXPECT_EQ(rec.invocationId, id);
    EXPECT_EQ(rec.epoch, system.epoch);
    EXPECT_EQ(rec.interfaceIndex, OCI::Mock::ocInterfaceIndex);
    EXPECT_EQ(rec.paramsSize, sizeof(OCI::Mock::OcRequest));
    EXPECT_EQ(rec.creationTick, system.tick);
    EXPECT_EQ(rec.status, OC_INVOCATION_STATUS_PENDING_AUTH);
    // in-flight slot is held from creation (admission control), not allocated lazily
    EXPECT_LT(rec.inFlightSlot, MAX_OC_IN_FLIGHT_INVOCATIONS);
    EXPECT_EQ(engine.agreeingSigsOf(0), 0);
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_PENDING_AUTH);

    // unknown id returns UNKNOWN
    EXPECT_EQ(engine.getOcInvocationStatus(id + 1), OC_INVOCATION_STATUS_UNKNOWN);
}


TEST_F(OcEngineTest, StartContractInvocationRejectsBadInputs)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);

    // contractIndex out of range
    EXPECT_EQ(engine.startContractInvocation(
        MAX_NUMBER_OF_CONTRACTS, OCI::Mock::ocInterfaceIndex, &req, sizeof(req)), -1);

    // interfaceIndex out of range
    EXPECT_EQ(engine.startContractInvocation(
        0, OCI::ocInterfacesCount, &req, sizeof(req)), -1);

    // wrong requestSize
    EXPECT_EQ(engine.startContractInvocation(
        0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req) + 1), -1);

    EXPECT_EQ(engine.exposedInvocationCount(), 0u);
}


TEST_F(OcEngineTest, AuthSignaturesAccumulateAndQuorumTransitions)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    req.value = 12345;
    const int64_t id = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    ASSERT_GE(id, 0);
    const m256i digest = engine.record(0).paramsDigest;

    unsigned char txBuf[MAX_TRANSACTION_SIZE];

    // Send QUORUM - 1 distinct sigs — should remain PENDING_AUTH
    for (unsigned int c = 0; c < QUORUM - 1; ++c)
    {
        buildAuthTx(comps, c, id, OCI::Mock::ocInterfaceIndex, system.epoch, digest, txBuf);
        EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
            reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));
    }
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_PENDING_AUTH);
    EXPECT_EQ(engine.agreeingSigsOf(0), QUORUM - 1);

    // QUORUM-th sig — transitions to AUTHORIZED
    buildAuthTx(comps, QUORUM - 1, id, OCI::Mock::ocInterfaceIndex, system.epoch, digest, txBuf);
    EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
        reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_AUTHORIZED);
    EXPECT_EQ(engine.agreeingSigsOf(0), QUORUM);

    // additional sig past quorum is rejected silently
    buildAuthTx(comps, QUORUM, id, OCI::Mock::ocInterfaceIndex, system.epoch, digest, txBuf);
    EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
        reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));
    EXPECT_EQ(engine.agreeingSigsOf(0), QUORUM); // unchanged
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_AUTHORIZED);
}


TEST_F(OcEngineTest, AuthSignaturesRejectDuplicateComputor)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    req.value = 7;
    const int64_t id = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    const m256i digest = engine.record(0).paramsDigest;

    unsigned char txBuf[MAX_TRANSACTION_SIZE];
    buildAuthTx(comps, 0, id, OCI::Mock::ocInterfaceIndex, system.epoch, digest, txBuf);
    EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
        reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));
    EXPECT_EQ(engine.agreeingSigsOf(0), 1);

    // Same computor submits again — should be deduped.
    EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
        reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));
    EXPECT_EQ(engine.agreeingSigsOf(0), 1);
}


TEST_F(OcEngineTest, AuthSignaturesRejectMismatchedParamsDigest)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    req.value = 7;
    const int64_t id = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));

    // Build an item with a tampered paramsDigest. The signature won't be over the record's digest,
    // so the engine should drop the item even though it carries a valid signature over the wrong msg.
    m256i wrongDigest;
    setMem(&wrongDigest, sizeof(wrongDigest), 0xAA);

    unsigned char txBuf[MAX_TRANSACTION_SIZE];
    buildAuthTx(comps, 0, id, OCI::Mock::ocInterfaceIndex, system.epoch, wrongDigest, txBuf);

    EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
        reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));
    EXPECT_EQ(engine.agreeingSigsOf(0), 0);
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_PENDING_AUTH);
}


TEST_F(OcEngineTest, TimeoutFlipsPendingAuthToTimeout)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    const int64_t id = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    ASSERT_GE(id, 0);

    // Same tick — not yet timed out
    engine.processTimeouts();
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_PENDING_AUTH);

    // Advance by exactly TIMEOUT-1 ticks — still not timed out
    system.tick += OC_INVOCATION_TIMEOUT_DEFAULT_TICKS - 1;
    engine.processTimeouts();
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_PENDING_AUTH);

    // One more tick — should flip and release the in-flight slot
    ++system.tick;
    engine.processTimeouts();
    EXPECT_EQ(engine.getOcInvocationStatus(id), OC_INVOCATION_STATUS_TIMEOUT);
    EXPECT_EQ(engine.record(0).inFlightSlot, OC_IN_FLIGHT_SLOT_NONE);
}


TEST_F(OcEngineTest, InFlightPoolExhaustionRejectsInvocation)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);

    // Fill the whole in-flight pool with pending invocations.
    for (uint32_t i = 0; i < MAX_OC_IN_FLIGHT_INVOCATIONS; ++i)
    {
        req.value = i;
        ASSERT_GE(engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req)), 0)
            << "creation " << i << " should still fit in the in-flight pool";
    }

    // Pool exhausted — admission control must reject deterministically.
    req.value = MAX_OC_IN_FLIGHT_INVOCATIONS;
    EXPECT_EQ(engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req)), -1);
    EXPECT_EQ(engine.exposedInvocationCount(), MAX_OC_IN_FLIGHT_INVOCATIONS);

    // Timing out the pending invocations releases their slots for new creations.
    system.tick += OC_INVOCATION_TIMEOUT_DEFAULT_TICKS;
    engine.processTimeouts();
    EXPECT_EQ(engine.getOcInvocationStatus(engine.record(0).invocationId), OC_INVOCATION_STATUS_TIMEOUT);

    const int64_t idAfterRelease = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    EXPECT_GE(idAfterRelease, 0);
    EXPECT_EQ(engine.getOcInvocationStatus(idAfterRelease), OC_INVOCATION_STATUS_PENDING_AUTH);
}


TEST_F(OcEngineTest, FirstActiveIndexSkipsResolvedRecords)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);

    // Two invocations that will time out.
    req.value = 1;
    ASSERT_GE(engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req)), 0);
    req.value = 2;
    ASSERT_GE(engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req)), 0);

    EXPECT_EQ(engine.exposedFirstActiveIndex(), 0u);

    system.tick += OC_INVOCATION_TIMEOUT_DEFAULT_TICKS;
    engine.processTimeouts();

    // Watermark advanced past both resolved records.
    EXPECT_EQ(engine.exposedFirstActiveIndex(), 2u);

    // A new pending invocation is still found by the tx builder despite startIdx = 0.
    req.value = 3;
    const int64_t newId = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    ASSERT_GE(newId, 0);

    unsigned char txBuf[MAX_TRANSACTION_SIZE];
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), UINT32_MAX);
    auto* tx = reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf);
    EXPECT_EQ(*reinterpret_cast<const unsigned short*>(tx->inputPtr()), 1);
    const auto* item = reinterpret_cast<const OcAuthSignatureItem*>(tx->inputPtr() + 4);
    EXPECT_EQ(item->invocationId, newId);
}


TEST_F(OcEngineTest, GetAuthSignatureTransactionBatchesPendingInvocations)
{
    OcEngineForTest engine(comps.publicKeys);

    // Record 3 invocations.
    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    std::vector<int64_t> ids;
    for (int i = 0; i < 3; ++i)
    {
        req.value = (unsigned long long)i;
        ids.push_back(engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req)));
    }

    unsigned char txBuf[MAX_TRANSACTION_SIZE];
    const uint32_t ret = engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0);
    EXPECT_EQ(ret, UINT32_MAX); // all items fit in one tx

    auto* tx = reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf);
    EXPECT_EQ(tx->inputType, OcAuthSignatureTransactionPrefix::transactionType());
    EXPECT_EQ(tx->tick, system.tick + 1);
    EXPECT_EQ(tx->sourcePublicKey, comps.publicKeys[5]);
    EXPECT_EQ(*reinterpret_cast<const unsigned short*>(tx->inputPtr()), 3);

    // Same-tick repeat calls still emit (lastScheduledTick == current tick is the same
    // emission round, so every own computor gets its items); duplicates dedup on-chain.
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), UINT32_MAX);
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/6, system.tick + 1, 0), UINT32_MAX);

    // Later ticks inside the reschedule window are suppressed for every computor.
    ++system.tick;
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), 0u);
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/6, system.tick + 1, 0), 0u);
}


TEST_F(OcEngineTest, AuthTxRescheduleReEmitsOnlyUnsignedComputors)
{
    OcEngineForTest engine(comps.publicKeys);

    OCI::Mock::OcRequest req;
    setMem(&req, sizeof(req), 0);
    req.value = 77;
    const int64_t id = engine.startContractInvocation(0, OCI::Mock::ocInterfaceIndex, &req, sizeof(req));
    ASSERT_GE(id, 0);
    const m256i digest = engine.record(0).paramsDigest;
    const uint32_t scheduleTick = system.tick;

    // Initial emission round for computors 5 and 6.
    unsigned char txBuf[MAX_TRANSACTION_SIZE];
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), UINT32_MAX);
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/6, system.tick + 1, 0), UINT32_MAX);

    // Computor 6's auth tx executes on-chain; computor 5's is lost.
    buildAuthTx(comps, 6, id, OCI::Mock::ocInterfaceIndex, system.epoch, digest, txBuf);
    EXPECT_TRUE(engine.processOcAuthSignatureTransaction(
        reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf)));

    // Inside the reschedule window: no re-emission for anyone.
    system.tick = scheduleTick + OC_AUTH_RESCHEDULE_TICKS - 1;
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), 0u);
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/6, system.tick + 1, 0), 0u);

    // Window elapsed: the unsigned computor retries, the signed one stays quiet.
    system.tick = scheduleTick + OC_AUTH_RESCHEDULE_TICKS;
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), UINT32_MAX);
    auto* tx = reinterpret_cast<OcAuthSignatureTransactionPrefix*>(txBuf);
    EXPECT_EQ(*reinterpret_cast<const unsigned short*>(tx->inputPtr()), 1);
    EXPECT_EQ(reinterpret_cast<const OcAuthSignatureItem*>(tx->inputPtr() + 4)->invocationId, id);
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/6, system.tick + 1, 0), 0u);

    // Edge-triggered: the retry bumped the stamp, so the next tick is suppressed again.
    ++system.tick;
    EXPECT_EQ(engine.getAuthSignatureTransaction(txBuf, /*computorIdx=*/5, system.tick + 1, 0), 0u);
}


// E2E delivery test deferred: the existing oracle_testing.h provides a static
// enqueueResponse stub that captures OM messages into a per-TU buffer. Driving
// deliverAuthorizedInvocations() through that stub captures bytes into the OM
// shape, which corrupts the OM test buffer without exposing useful per-message
// type discrimination for OC verification. Proper plumbing (a shared dispatching
// stub that branches on message type) belongs in a follow-up. Until then
// deliverAuthorizedInvocations() itself — message assembly, the delivered flag,
// slot freeing, and the snapshot-restore branch — is NOT covered by any test.
