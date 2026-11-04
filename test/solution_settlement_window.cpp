#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/mining/solution_settlement_window.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// Two stub kinds. The hooks record their calls; the loops under test are run end to end with no
// scorer behind them.
static constexpr unsigned short KIND_A = 2;    // MiningSolutionTransaction::transactionType()
static constexpr unsigned short KIND_B = 12;   // ANT_COLONY_MINING_SOLUTION_INPUT_TYPE
static constexpr unsigned short KIND_UNREGISTERED = 99;
static constexpr unsigned int RECORD_BYTES_A = 72;
static constexpr unsigned int RECORD_BYTES_B = 48;

static constexpr unsigned short TEST_EPOCH = 200;
// Ticks are absolute. Every test starts with this tick settled, so appends begin at TEST_TICK.
static constexpr unsigned int TEST_SETTLED_THROUGH = 99999;
static constexpr unsigned int TEST_TICK = TEST_SETTLED_THROUGH + 1;
static constexpr unsigned int W = SOLUTION_SETTLEMENT_WINDOW_TICKS;
static constexpr unsigned int TICK_INDEX_SIZE = SOLUTION_SETTLEMENT_TICK_INDEX_SIZE;

struct HookCall
{
    char hook;                  // 's' score, 't' settle
    unsigned short kind;        // which stub ran: proves dispatch, independent of slot.inputType
    unsigned short inputType;   // what the slot says
    unsigned int tick;
    unsigned int txIndex;
    unsigned long long processorNumber;
};

static std::mutex gCallsLock;
static std::vector<HookCall> gCalls;

static void recordCall(char hook, unsigned short kind, const SettlementSlot& slot, unsigned long long processorNumber)
{
    std::lock_guard<std::mutex> guard(gCallsLock);
    HookCall call;
    call.hook = hook;
    call.kind = kind;
    call.inputType = slot.inputType;
    call.tick = slot.tick;
    call.txIndex = slot.txIndex;
    call.processorNumber = processorNumber;
    gCalls.push_back(call);
}

static std::vector<HookCall> takeCalls()
{
    std::lock_guard<std::mutex> guard(gCallsLock);
    std::vector<HookCall> calls = gCalls;
    gCalls.clear();
    return calls;
}

// A worker writes its result and nothing else. The bytes are arbitrary; the digest test checks they
// do not count.
static void scoreStubA(SettlementSlot& slot, unsigned long long processorNumber)
{
    recordCall('s', KIND_A, slot, processorNumber);
    slot.result[0] = 0xA5;
    slot.result[MAX_SOLUTION_RESULT_BYTES - 1] = (unsigned char)slot.txIndex;
}

static void scoreStubB(SettlementSlot& slot, unsigned long long processorNumber)
{
    recordCall('s', KIND_B, slot, processorNumber);
    slot.result[0] = 0xB6;
    slot.result[MAX_SOLUTION_RESULT_BYTES - 1] = (unsigned char)slot.txIndex;
}

// Settlement only ever sees a scored slot; that is the whole point of the backstop.
static void settleStubA(const SettlementSlot& slot, unsigned long long processorNumber)
{
    EXPECT_TRUE(slot.scored);
    recordCall('t', KIND_A, slot, processorNumber);
}

static void settleStubB(const SettlementSlot& slot, unsigned long long processorNumber)
{
    EXPECT_TRUE(slot.scored);
    recordCall('t', KIND_B, slot, processorNumber);
}

static void fillRecord(unsigned char* out, unsigned int size, unsigned int seed)
{
    for (unsigned int i = 0; i < size; i++)
    {
        out[i] = (unsigned char)((seed * 31u + i * 7u) & 0xFFu);
    }
}

static unsigned int recordBytesOf(unsigned short kind)
{
    return (kind == KIND_A) ? RECORD_BYTES_A : RECORD_BYTES_B;
}

// Append one solution of the given kind with a payload derived from seed.
static bool put(SolutionSettlementWindow* window, unsigned int tick, unsigned int txIndex, unsigned short kind,
    unsigned int seed = 1)
{
    unsigned char record[MAX_SOLUTION_RECORD_BYTES];
    const unsigned int size = recordBytesOf(kind);
    fillRecord(record, size, seed);
    return window->append(tick, txIndex, kind, record, size);
}

// The payload put() wrote for (kind, seed) is back byte for byte, and the rest of record[] is zero.
static bool recordMatches(const SettlementSlot* slot, unsigned short kind, unsigned int seed)
{
    if (slot == nullptr)
    {
        return false;
    }
    const unsigned int size = recordBytesOf(kind);
    if (slot->inputType != kind || slot->recordSize != size)
    {
        return false;
    }
    unsigned char expected[MAX_SOLUTION_RECORD_BYTES];
    fillRecord(expected, size, seed);
    for (unsigned int i = 0; i < size; i++)
    {
        if (slot->record[i] != expected[i])
        {
            return false;
        }
    }
    for (unsigned int i = size; i < MAX_SOLUTION_RECORD_BYTES; i++)
    {
        if (slot->record[i] != 0)
        {
            return false;
        }
    }
    return true;
}

static void scoreEverything(SolutionSettlementWindow* window, unsigned long long processorNumber = 1)
{
    while (window->workerStep(processorNumber))
    {
    }
}

// init() allocates ~2 GB, so the window is built once for the file and reset between tests. Lazy
// rather than SetUpTestSuite for the same reason as the colony tests (gtest 1.8.1 in test.vcxproj).
static SolutionSettlementWindow* freshWindow()
{
    static SolutionSettlementWindow window;
    static bool allocated = false;
    static bool allocationFailed = false;

    if (!allocated && !allocationFailed)
    {
        allocationFailed = !window.init();
        allocated = !allocationFailed;
        if (allocated)
        {
            EXPECT_TRUE(window.registerKind(KIND_A, scoreStubA, settleStubA));
            EXPECT_TRUE(window.registerKind(KIND_B, scoreStubB, settleStubB));
            // A kind registers once.
            EXPECT_FALSE(window.registerKind(KIND_A, scoreStubA, settleStubA));
        }
    }
    if (allocationFailed)
    {
        return nullptr;
    }

    window.reset(TEST_SETTLED_THROUGH);
    takeCalls();
    return &window;
}

static void expectCall(const std::vector<HookCall>& calls, unsigned long long i, char hook, unsigned short kind,
    unsigned int tick, unsigned int txIndex)
{
    ASSERT_LT(i, calls.size());
    EXPECT_EQ(calls[i].hook, hook) << "call " << i;
    EXPECT_EQ(calls[i].kind, kind) << "call " << i;
    EXPECT_EQ(calls[i].inputType, kind) << "call " << i;
    EXPECT_EQ(calls[i].tick, tick) << "call " << i;
    EXPECT_EQ(calls[i].txIndex, txIndex) << "call " << i;
}

// Settlement order is the consensus order: ascending txIndex, each slot to the hook of its kind.
TEST(TestSolutionSettlementWindow, SettleCallsKindsInTxIndexOrder)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK, 3, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK, 5, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK, 9, KIND_B));

    // Only registered kinds can enter, so dispatch is total.
    EXPECT_FALSE(put(window, TEST_TICK, 10, KIND_UNREGISTERED));
    // Within a tick the order of arrival is the transaction order; a step back is refused, so the
    // run is ascending by construction.
    EXPECT_FALSE(put(window, TEST_TICK, 4, KIND_A));
    EXPECT_FALSE(put(window, TEST_TICK, 9, KIND_A));
    EXPECT_EQ(window->unsettledCount(), 4u);

    scoreEverything(window);
    takeCalls();

    window->settleTick(TEST_TICK, 7);

    const std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 4u);
    expectCall(calls, 0, 't', KIND_A, TEST_TICK, 0);
    expectCall(calls, 1, 't', KIND_B, TEST_TICK, 3);
    expectCall(calls, 2, 't', KIND_A, TEST_TICK, 5);
    expectCall(calls, 3, 't', KIND_B, TEST_TICK, 9);
    EXPECT_EQ(window->stats().settled, 4u);
}

// A slot no worker reached is scored on the settling processor, before its settle hook.
TEST(TestSolutionSettlementWindow, SettleScoresUnscoredSlotInline)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_B));
    EXPECT_EQ(window->unscoredCount(), 1u);

    window->settleTick(TEST_TICK, 7);

    const std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 2u);
    expectCall(calls, 0, 's', KIND_B, TEST_TICK, 0);
    EXPECT_EQ(calls[0].processorNumber, 7u);
    expectCall(calls, 1, 't', KIND_B, TEST_TICK, 0);
    EXPECT_EQ(window->unscoredCount(), 0u);
    EXPECT_EQ(window->stats().scoredInline, 1u);
    EXPECT_EQ(window->stats().scoredByWorker, 0u);
}

// Several slots no worker reached: settlement takes all of them through the shared cursor on the
// settling processor, then settles them in txIndex order. The next tick's slot is left to the workers.
TEST(TestSolutionSettlementWindow, SettleScoresTheRunBeforeSettlingIt)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK, 2, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK, 5, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK + 1, 0, KIND_B));

    window->settleTick(TEST_TICK, 7);

    const std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 6u);
    expectCall(calls, 0, 's', KIND_A, TEST_TICK, 0);
    expectCall(calls, 1, 's', KIND_B, TEST_TICK, 2);
    expectCall(calls, 2, 's', KIND_A, TEST_TICK, 5);
    expectCall(calls, 3, 't', KIND_A, TEST_TICK, 0);
    expectCall(calls, 4, 't', KIND_B, TEST_TICK, 2);
    expectCall(calls, 5, 't', KIND_A, TEST_TICK, 5);
    for (unsigned long long i = 0; i < calls.size(); i++)
    {
        EXPECT_EQ(calls[i].processorNumber, 7u) << "call " << i;
    }
    EXPECT_EQ(window->stats().scoredInline, 3u);
    EXPECT_EQ(window->stats().stalledOnWorker, 0u);
    EXPECT_EQ(window->stats().settled, 3u);

    EXPECT_EQ(window->unscoredCount(), 1u);
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_FALSE(window->workerStep(1));
    std::vector<HookCall> after = takeCalls();
    ASSERT_EQ(after.size(), 1u);
    expectCall(after, 0, 's', KIND_B, TEST_TICK + 1, 0);
    EXPECT_EQ(window->stats().scoredByWorker, 1u);
}

TEST(TestSolutionSettlementWindow, SettleClearsAndAdvances)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK, 4, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK + 1, 2, KIND_A));
    scoreEverything(window);
    EXPECT_NE(window->lookup(TEST_TICK, 0), nullptr);
    EXPECT_NE(window->lookup(TEST_TICK, 4), nullptr);

    window->settleTick(TEST_TICK, 1);

    EXPECT_EQ(window->lookup(TEST_TICK, 0), nullptr);
    EXPECT_EQ(window->lookup(TEST_TICK, 4), nullptr);
    EXPECT_NE(window->lookup(TEST_TICK + 1, 2), nullptr);
    EXPECT_EQ(window->settledThroughTick(), TEST_TICK);
    EXPECT_EQ(window->unsettledCount(), 1u);

    // A settled tick cannot be appended to again.
    EXPECT_FALSE(put(window, TEST_TICK, 8, KIND_A));
}

TEST(TestSolutionSettlementWindow, SettleBelowSettledThroughIsNoOp)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";
    window->reset(1000);

    EXPECT_TRUE(put(window, 1005, 0, KIND_A));
    scoreEverything(window);
    takeCalls();

    window->settleTick(999, 1);
    window->settleTick(1000, 1);

    EXPECT_TRUE(takeCalls().empty());
    EXPECT_EQ(window->settledThroughTick(), 1000u);
    EXPECT_NE(window->lookup(1005, 0), nullptr);
    EXPECT_EQ(window->unsettledCount(), 1u);
    EXPECT_EQ(window->stats().settled, 0u);
}

// Two workers share one cursor: every slot is scored exactly once, in (tick, txIndex) order.
TEST(TestSolutionSettlementWindow, WorkerStepIsFifoByTickThenIndex)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_FALSE(window->workerStep(1));

    EXPECT_TRUE(put(window, TEST_TICK, 1, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK, 2, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK + 1, 0, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK + 2, 0, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK + 2, 7, KIND_A));
    EXPECT_EQ(window->unscoredCount(), 5u);

    EXPECT_TRUE(window->workerStep(1));
    EXPECT_TRUE(window->workerStep(2));
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_TRUE(window->workerStep(2));
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_FALSE(window->workerStep(2));
    EXPECT_FALSE(window->workerStep(1));

    const std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 5u);
    expectCall(calls, 0, 's', KIND_A, TEST_TICK, 1);
    expectCall(calls, 1, 's', KIND_B, TEST_TICK, 2);
    expectCall(calls, 2, 's', KIND_A, TEST_TICK + 1, 0);
    expectCall(calls, 3, 's', KIND_B, TEST_TICK + 2, 0);
    expectCall(calls, 4, 's', KIND_A, TEST_TICK + 2, 7);
    EXPECT_EQ(calls[0].processorNumber, 1u);
    EXPECT_EQ(calls[1].processorNumber, 2u);
    EXPECT_EQ(calls[2].processorNumber, 1u);
    EXPECT_EQ(calls[3].processorNumber, 2u);
    EXPECT_EQ(calls[4].processorNumber, 1u);

    EXPECT_EQ(window->unscoredCount(), 0u);
    EXPECT_EQ(window->stats().scoredByWorker, 5u);
    for (unsigned long long i = 0; i < calls.size(); i++)
    {
        const SettlementSlot* slot = window->lookup(calls[i].tick, calls[i].txIndex);
        ASSERT_NE(slot, nullptr);
        EXPECT_TRUE(slot->scored);
    }
}

// Three outstanding ticks, one slot still unscored: the drain scores it, settles all three in
// order and leaves nothing behind.
TEST(TestSolutionSettlementWindow, DrainAllSettlesEveryOutstandingTickInOrder)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK, 1, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK + 3, 2, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK + 7, 0, KIND_A));
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_EQ(window->unscoredCount(), 1u);
    takeCalls();

    window->drainAll(9);

    const std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 5u);
    expectCall(calls, 0, 's', KIND_A, TEST_TICK + 7, 0);
    EXPECT_EQ(calls[0].processorNumber, 9u);
    expectCall(calls, 1, 't', KIND_A, TEST_TICK, 0);
    expectCall(calls, 2, 't', KIND_B, TEST_TICK, 1);
    expectCall(calls, 3, 't', KIND_B, TEST_TICK + 3, 2);
    expectCall(calls, 4, 't', KIND_A, TEST_TICK + 7, 0);

    EXPECT_EQ(window->unsettledCount(), 0u);
    EXPECT_EQ(window->unscoredCount(), 0u);
    EXPECT_EQ(window->settledThroughTick(), TEST_TICK + 7);
    EXPECT_EQ(window->lookup(TEST_TICK, 0), nullptr);
    EXPECT_EQ(window->lookup(TEST_TICK + 3, 2), nullptr);
    EXPECT_EQ(window->lookup(TEST_TICK + 7, 0), nullptr);
    EXPECT_EQ(window->stats().drains, 1u);
    EXPECT_EQ(window->stats().scoredByDrain, 1u);
    EXPECT_EQ(window->stats().scoredInline, 0u);   // a drain is not a settlement backstop

    // Nothing outstanding: a second drain is a no-op.
    window->drainAll(9);
    EXPECT_TRUE(takeCalls().empty());
    EXPECT_EQ(window->settledThroughTick(), TEST_TICK + 7);
}

// Settlement runs on every tick number whether or not the tick carried solutions. Empties advance
// the settled mark and never touch a pending slot ahead of them.
TEST(TestSolutionSettlementWindow, RunOfEmptyTicksLongerThanW)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    // A solution the full window ahead, the farthest a live append ever is from the settled mark.
    const unsigned int ahead = TEST_SETTLED_THROUGH + W + 1;
    EXPECT_TRUE(put(window, ahead, 0, KIND_A));

    for (unsigned int t = TEST_SETTLED_THROUGH + 1; t < ahead; t++)
    {
        window->settleTick(t, 1);
        ASSERT_EQ(window->settledThroughTick(), t);
    }
    EXPECT_TRUE(takeCalls().empty());
    EXPECT_NE(window->lookup(ahead, 0), nullptr);
    EXPECT_EQ(window->unscoredCount(), 1u);
    EXPECT_EQ(window->unsettledCount(), 1u);

    window->settleTick(ahead, 1);
    std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 2u);
    expectCall(calls, 0, 's', KIND_A, ahead, 0);
    expectCall(calls, 1, 't', KIND_A, ahead, 0);
    EXPECT_EQ(window->unsettledCount(), 0u);

    // A run of empties longer than the tick index, so every entry is passed over at least once.
    const unsigned int last = ahead + TICK_INDEX_SIZE + 10;
    for (unsigned int t = ahead + 1; t <= last; t++)
    {
        window->settleTick(t, 1);
    }
    EXPECT_TRUE(takeCalls().empty());
    EXPECT_EQ(window->settledThroughTick(), last);
    EXPECT_EQ(window->stats().settled, 1u);

    // And the window still works after it.
    EXPECT_TRUE(put(window, last + 1, 3, KIND_B));
    window->settleTick(last + 1, 1);
    calls = takeCalls();
    ASSERT_EQ(calls.size(), 2u);
    expectCall(calls, 1, 't', KIND_B, last + 1, 3);
    EXPECT_EQ(window->unsettledCount(), 0u);
}

// Neither the tick index nor the arena ever overwrites a slot that has not been settled.
TEST(TestSolutionSettlementWindow, NoOverwriteOfUnsettledSlot)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    // Tick index: the tick an index length later maps to the same entry.
    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 11));
    EXPECT_FALSE(put(window, TEST_TICK + TICK_INDEX_SIZE, 0, KIND_B, 12));
    EXPECT_TRUE(recordMatches(window->lookup(TEST_TICK, 0), KIND_A, 11));
    EXPECT_EQ(window->lookup(TEST_TICK + TICK_INDEX_SIZE, 0), nullptr);
    EXPECT_EQ(window->unsettledCount(), 1u);

    window->settleTick(TEST_TICK, 1);
    EXPECT_TRUE(put(window, TEST_TICK + TICK_INDEX_SIZE, 0, KIND_B, 12));
    EXPECT_TRUE(recordMatches(window->lookup(TEST_TICK + TICK_INDEX_SIZE, 0), KIND_B, 12));
    EXPECT_EQ(window->stats().refused, 1u);

    // Arena: fill the window plus one tick with a full tick of transactions each, which is exactly
    // the capacity. One more is refused until the oldest tick settles.
    window->reset(TEST_SETTLED_THROUGH);
    const unsigned int firstTick = TEST_TICK;
    const unsigned int lastTick = TEST_TICK + W;
    unsigned char record[MAX_SOLUTION_RECORD_BYTES];
    fillRecord(record, RECORD_BYTES_B, 5);
    for (unsigned int t = firstTick; t <= lastTick; t++)
    {
        for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
        {
            ASSERT_TRUE(window->append(t, i, KIND_B, record, RECORD_BYTES_B)) << "tick " << t << " tx " << i;
        }
    }
    EXPECT_EQ(window->unsettledCount(), (unsigned int)SOLUTION_SETTLEMENT_CAPACITY);
    EXPECT_FALSE(window->append(lastTick + 1, 0, KIND_B, record, RECORD_BYTES_B));
    EXPECT_TRUE(recordMatches(window->lookup(firstTick, 0), KIND_B, 5));
    EXPECT_TRUE(recordMatches(window->lookup(lastTick, NUMBER_OF_TRANSACTIONS_PER_TICK - 1), KIND_B, 5));

    window->settleTick(firstTick, 1);
    EXPECT_EQ(window->unsettledCount(), (unsigned int)(SOLUTION_SETTLEMENT_CAPACITY - NUMBER_OF_TRANSACTIONS_PER_TICK));
    EXPECT_TRUE(window->append(lastTick + 1, 0, KIND_B, record, RECORD_BYTES_B));
    EXPECT_TRUE(recordMatches(window->lookup(lastTick + 1, 0), KIND_B, 5));
    takeCalls();
}

// The digest is over what phase 1 captured, in order, and nothing a worker writes.
TEST(TestSolutionSettlementWindow, WindowDigestCoversRecordsOnly)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";
    const unsigned int empty = window->windowDigest();

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 1));
    EXPECT_TRUE(put(window, TEST_TICK, 3, KIND_B, 2));
    const unsigned int digest = window->windowDigest();
    EXPECT_NE(digest, empty);

    // Same records again: same digest.
    window->reset(TEST_SETTLED_THROUGH);
    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 1));
    EXPECT_TRUE(put(window, TEST_TICK, 3, KIND_B, 2));
    EXPECT_EQ(window->windowDigest(), digest);

    // One payload byte differs.
    window->reset(TEST_SETTLED_THROUGH);
    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 1));
    EXPECT_TRUE(put(window, TEST_TICK, 3, KIND_B, 3));
    EXPECT_NE(window->windowDigest(), digest);

    // Same payloads at a different txIndex: the identity is part of the record.
    window->reset(TEST_SETTLED_THROUGH);
    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 1));
    EXPECT_TRUE(put(window, TEST_TICK, 4, KIND_B, 2));
    EXPECT_NE(window->windowDigest(), digest);

    // Results and the scored flag are not part of it.
    window->reset(TEST_SETTLED_THROUGH);
    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 1));
    EXPECT_TRUE(put(window, TEST_TICK, 3, KIND_B, 2));
    scoreEverything(window);
    EXPECT_TRUE(window->lookup(TEST_TICK, 0)->scored);
    EXPECT_EQ(window->windowDigest(), digest);

    // Settling the tick empties the window: back to the empty digest.
    window->settleTick(TEST_TICK, 1);
    EXPECT_EQ(window->windowDigest(), empty);
    takeCalls();
}

static constexpr unsigned int TEST_RESOURCE_DIGEST = 0xABCD1234u;

// Save, wipe, load. reset() clears everything the load has to bring back.
static bool saveWipeLoad(SolutionSettlementWindow* window)
{
    if (!window->save(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST))
    {
        return false;
    }
    window->reset(0);
    return window->load(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST);
}

TEST(TestSolutionSettlementWindow, SaveLoadRoundTrip)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 21));
    EXPECT_TRUE(put(window, TEST_TICK, 5, KIND_B, 22));
    EXPECT_TRUE(put(window, TEST_TICK + 2, 1, KIND_A, 23));
    // Two scored, the cursor past them: neither survives the round trip.
    EXPECT_TRUE(window->workerStep(1));
    EXPECT_TRUE(window->workerStep(1));
    const unsigned int digest = window->windowDigest();
    takeCalls();

    ASSERT_TRUE(saveWipeLoad(window));

    EXPECT_EQ(window->settledThroughTick(), TEST_SETTLED_THROUGH);
    EXPECT_EQ(window->unsettledCount(), 3u);
    EXPECT_EQ(window->unscoredCount(), 3u);
    EXPECT_EQ(window->windowDigest(), digest);

    const SettlementSlot* first = window->lookup(TEST_TICK, 0);
    const SettlementSlot* second = window->lookup(TEST_TICK, 5);
    const SettlementSlot* third = window->lookup(TEST_TICK + 2, 1);
    EXPECT_TRUE(recordMatches(first, KIND_A, 21));
    EXPECT_TRUE(recordMatches(second, KIND_B, 22));
    EXPECT_TRUE(recordMatches(third, KIND_A, 23));
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(third, nullptr);
    EXPECT_FALSE(first->scored);
    EXPECT_FALSE(second->scored);
    EXPECT_FALSE(third->scored);

    // The cursor is back at the oldest unsettled slot.
    EXPECT_TRUE(window->workerStep(4));
    const std::vector<HookCall> calls = takeCalls();
    ASSERT_EQ(calls.size(), 1u);
    expectCall(calls, 0, 's', KIND_A, TEST_TICK, 0);

    // And the rest settles as if nothing happened.
    window->settleTick(TEST_TICK + 2, 1);
    EXPECT_EQ(window->unsettledCount(), 0u);
    EXPECT_EQ(takeCalls().size(), 5u);
}

// The file must pair with the node state loaded beside it and must be whole and unmodified.
TEST(TestSolutionSettlementWindow, LoadRefusesMismatchedDigest)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A, 31));
    EXPECT_TRUE(put(window, TEST_TICK + 1, 2, KIND_B, 32));
    ASSERT_TRUE(window->save(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST));

    // Node state from another moment. Only the header was read, so the window is untouched.
    EXPECT_FALSE(window->load(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST + 1));
    EXPECT_EQ(window->unsettledCount(), 2u);

    // The epoch is part of the file name, so another one finds no file at all.
    EXPECT_FALSE(window->load((unsigned short)(TEST_EPOCH + 1), NULL, TEST_RESOURCE_DIGEST));
    EXPECT_EQ(window->unsettledCount(), 2u);

    // Truncated: the records chunk holds one record where the header promises two.
    solutionSettlementNameFilesForEpoch(TEST_EPOCH);
    CHAR16 chunkName[64];
    solutionSettlementRecordsChunkName(chunkName, 0);
    unsigned char chunk[2 * sizeof(SettlementRecord)];
    ASSERT_EQ(load(chunkName, sizeof(chunk), chunk), (long long)sizeof(chunk));
    ASSERT_EQ(save(chunkName, sizeof(SettlementRecord), chunk), (long long)sizeof(SettlementRecord));
    EXPECT_FALSE(window->load(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST));

    // Modified: one payload byte of the first record, with its identity fields intact so it passes
    // admission and only the digest can catch it.
    ASSERT_EQ(save(chunkName, sizeof(chunk), chunk), (long long)sizeof(chunk));
    SettlementRecord first;
    copyMem(&first, chunk, sizeof(first));
    first.record[0] ^= 0x01;
    copyMem(chunk, &first, sizeof(first));
    ASSERT_EQ(save(chunkName, sizeof(chunk), chunk), (long long)sizeof(chunk));
    EXPECT_FALSE(window->load(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST));

    // Malformed: a record that fails admission (its tick is already settled).
    first.record[0] ^= 0x01;
    first.tick = TEST_SETTLED_THROUGH;
    copyMem(chunk, &first, sizeof(first));
    ASSERT_EQ(save(chunkName, sizeof(chunk), chunk), (long long)sizeof(chunk));
    EXPECT_FALSE(window->load(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST));

    // The original bytes load, so the refusals above were the checks and not a bad file.
    first.tick = TEST_TICK;
    copyMem(chunk, &first, sizeof(first));
    ASSERT_EQ(save(chunkName, sizeof(chunk), chunk), (long long)sizeof(chunk));
    EXPECT_TRUE(window->load(TEST_EPOCH, NULL, TEST_RESOURCE_DIGEST));
    EXPECT_EQ(window->unsettledCount(), 2u);
    EXPECT_TRUE(recordMatches(window->lookup(TEST_TICK, 0), KIND_A, 31));
    EXPECT_TRUE(recordMatches(window->lookup(TEST_TICK + 1, 2), KIND_B, 32));
}

TEST(TestSolutionSettlementWindow, ResetEmptiesEverything)
{
    SolutionSettlementWindow* window = freshWindow();
    ASSERT_NE(window, nullptr) << "window init failed; needs ~2 GB";
    const unsigned int empty = window->windowDigest();

    EXPECT_TRUE(put(window, TEST_TICK, 0, KIND_A));
    EXPECT_TRUE(put(window, TEST_TICK + 1, 0, KIND_B));
    EXPECT_TRUE(put(window, TEST_TICK + 4, 6, KIND_A));
    EXPECT_TRUE(window->workerStep(1));
    window->settleTick(TEST_TICK, 1);
    EXPECT_EQ(window->unsettledCount(), 2u);
    EXPECT_EQ(window->unscoredCount(), 2u);
    takeCalls();

    window->reset(500000);

    EXPECT_EQ(window->lookup(TEST_TICK + 1, 0), nullptr);
    EXPECT_EQ(window->lookup(TEST_TICK + 4, 6), nullptr);
    EXPECT_EQ(window->unsettledCount(), 0u);
    EXPECT_EQ(window->unscoredCount(), 0u);
    EXPECT_EQ(window->settledThroughTick(), 500000u);
    EXPECT_EQ(window->windowDigest(), empty);
    EXPECT_FALSE(window->workerStep(1));
    EXPECT_EQ(window->stats().appended, 0u);
    EXPECT_EQ(window->stats().settled, 0u);

    // The old ticks are behind the new mark; the new ones work.
    EXPECT_FALSE(put(window, TEST_TICK + 4, 6, KIND_A));
    EXPECT_TRUE(put(window, 500001, 0, KIND_B));
    window->settleTick(500001, 1);
    EXPECT_EQ(takeCalls().size(), 2u);
    EXPECT_EQ(window->unsettledCount(), 0u);
}

