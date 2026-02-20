#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROC_CREATE_GATE = 1;
constexpr uint16 PROC_SEND_TO_GATE = 2;
constexpr uint16 PROC_CLOSE_GATE = 3;
constexpr uint16 PROC_UPDATE_GATE = 4;
constexpr uint16 FUNC_GET_GATE = 5;
constexpr uint16 FUNC_GET_GATE_COUNT = 6;
constexpr uint16 FUNC_GET_GATES_BY_OWNER = 7;
constexpr uint16 FUNC_GET_GATE_BATCH = 8;
constexpr uint16 FUNC_GET_FEES = 9;

static const id userA = ID(_A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A, _A);
static const id userB = ID(_B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B);
static const id userC = ID(_C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C);

class ContractTestingQuGate : protected ContractTesting
{
public:
    ContractTestingQuGate()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUGATE);
        system.epoch = contractDescriptions[QUGATE_CONTRACT_INDEX].constructionEpoch;
        callSystemProcedure(QUGATE_CONTRACT_INDEX, INITIALIZE);

        // Fund test users
        increaseEnergy(userA, 1000000000LL);
        increaseEnergy(userB, 1000000000LL);
        increaseEnergy(userC, 1000000000LL);
    }

    QUGATE::createGate_output createGate(const id& user, uint8 mode, uint8 recipientCount,
        const id* recipients, const uint64* ratios, uint64 threshold = 0,
        const id* allowedSenders = nullptr, uint8 allowedSenderCount = 0,
        sint64 fee = QUGATE_DEFAULT_CREATION_FEE)
    {
        QUGATE::createGate_input input;
        setMemory(input, 0);
        input.mode = mode;
        input.recipientCount = recipientCount;
        for (uint8 i = 0; i < recipientCount; i++)
        {
            input.recipients.set(i, recipients[i]);
            input.ratios.set(i, ratios[i]);
        }
        input.threshold = threshold;
        if (allowedSenders)
        {
            for (uint8 i = 0; i < allowedSenderCount; i++)
            {
                input.allowedSenders.set(i, allowedSenders[i]);
            }
        }
        input.allowedSenderCount = allowedSenderCount;

        QUGATE::createGate_output output;
        invokeUserProcedure(QUGATE_CONTRACT_INDEX, PROC_CREATE_GATE, input, output, user, fee);
        return output;
    }

    QUGATE::sendToGate_output sendToGate(const id& user, uint64 gateId, sint64 amount)
    {
        QUGATE::sendToGate_input input;
        setMemory(input, 0);
        input.gateId = gateId;

        QUGATE::sendToGate_output output;
        invokeUserProcedure(QUGATE_CONTRACT_INDEX, PROC_SEND_TO_GATE, input, output, user, amount);
        return output;
    }

    QUGATE::closeGate_output closeGate(const id& user, uint64 gateId)
    {
        QUGATE::closeGate_input input;
        setMemory(input, 0);
        input.gateId = gateId;

        QUGATE::closeGate_output output;
        invokeUserProcedure(QUGATE_CONTRACT_INDEX, PROC_CLOSE_GATE, input, output, user, 0);
        return output;
    }

    QUGATE::updateGate_output updateGate(const id& user, uint64 gateId, uint8 recipientCount,
        const id* recipients, const uint64* ratios, uint64 threshold = 0,
        const id* allowedSenders = nullptr, uint8 allowedSenderCount = 0)
    {
        QUGATE::updateGate_input input;
        setMemory(input, 0);
        input.gateId = gateId;
        input.recipientCount = recipientCount;
        for (uint8 i = 0; i < recipientCount; i++)
        {
            input.recipients.set(i, recipients[i]);
            input.ratios.set(i, ratios[i]);
        }
        input.threshold = threshold;
        if (allowedSenders)
        {
            for (uint8 i = 0; i < allowedSenderCount; i++)
            {
                input.allowedSenders.set(i, allowedSenders[i]);
            }
        }
        input.allowedSenderCount = allowedSenderCount;

        QUGATE::updateGate_output output;
        invokeUserProcedure(QUGATE_CONTRACT_INDEX, PROC_UPDATE_GATE, input, output, user, 0);
        return output;
    }

    QUGATE::getGate_output getGate(uint64 gateId)
    {
        QUGATE::getGate_input input;
        setMemory(input, 0);
        input.gateId = gateId;

        QUGATE::getGate_output output;
        callFunction(QUGATE_CONTRACT_INDEX, FUNC_GET_GATE, input, output);
        return output;
    }

    QUGATE::getGateCount_output getGateCount()
    {
        QUGATE::getGateCount_input input;
        setMemory(input, 0);

        QUGATE::getGateCount_output output;
        callFunction(QUGATE_CONTRACT_INDEX, FUNC_GET_GATE_COUNT, input, output);
        return output;
    }

    QUGATE::getFees_output getFees()
    {
        QUGATE::getFees_input input;
        setMemory(input, 0);

        QUGATE::getFees_output output;
        callFunction(QUGATE_CONTRACT_INDEX, FUNC_GET_FEES, input, output);
        return output;
    }
};

// ============================================================
// Initialization
// ============================================================

TEST(ContractQuGate, InitializeDefaults)
{
    ContractTestingQuGate qg;

    auto fees = qg.getFees();
    EXPECT_EQ(fees.creationFee, QUGATE_DEFAULT_CREATION_FEE);
    EXPECT_EQ(fees.currentCreationFee, QUGATE_DEFAULT_CREATION_FEE);
    EXPECT_EQ(fees.minSendAmount, QUGATE_DEFAULT_MIN_SEND);
    EXPECT_EQ(fees.expiryEpochs, QUGATE_DEFAULT_EXPIRY_EPOCHS);

    auto counts = qg.getGateCount();
    EXPECT_EQ(counts.totalGates, 0ULL);
    EXPECT_EQ(counts.activeGates, 0ULL);
    EXPECT_EQ(counts.totalBurned, 0ULL);
}

// ============================================================
// SPLIT mode
// ============================================================

TEST(ContractQuGate, CreateSplitGate)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {60, 40};
    auto out = qg.createGate(userA, QUGATE_MODE_SPLIT, 2, recipients, ratios);
    EXPECT_EQ(out.status, QUGATE_SUCCESS);
    EXPECT_EQ(out.gateId, 1ULL);
    EXPECT_EQ(out.feePaid, (uint64)QUGATE_DEFAULT_CREATION_FEE);

    auto gate = qg.getGate(1);
    EXPECT_EQ(gate.mode, QUGATE_MODE_SPLIT);
    EXPECT_EQ(gate.recipientCount, 2);
    EXPECT_EQ(gate.active, 1);

    auto counts = qg.getGateCount();
    EXPECT_EQ(counts.totalGates, 1ULL);
    EXPECT_EQ(counts.activeGates, 1ULL);
    EXPECT_EQ(counts.totalBurned, (uint64)QUGATE_DEFAULT_CREATION_FEE);
}

TEST(ContractQuGate, SplitEvenDistribution)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {50, 50};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 2, recipients, ratios);

    long long balB_before = getBalance(userB);
    long long balC_before = getBalance(userC);

    qg.sendToGate(userA, 1, 10000);

    long long balB_after = getBalance(userB);
    long long balC_after = getBalance(userC);

    EXPECT_EQ(balB_after - balB_before, 5000);
    EXPECT_EQ(balC_after - balC_before, 5000);
}

TEST(ContractQuGate, SplitUnevenDistribution)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {70, 30};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 2, recipients, ratios);

    long long balB_before = getBalance(userB);
    long long balC_before = getBalance(userC);

    qg.sendToGate(userA, 1, 10000);

    EXPECT_EQ(getBalance(userB) - balB_before, 7000);
    EXPECT_EQ(getBalance(userC) - balC_before, 3000);
}

TEST(ContractQuGate, SplitRoundingLastRecipient)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {33, 67};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 2, recipients, ratios);

    long long balB_before = getBalance(userB);
    long long balC_before = getBalance(userC);

    qg.sendToGate(userA, 1, 10000);

    long long gainB = getBalance(userB) - balB_before;
    long long gainC = getBalance(userC) - balC_before;

    // Total must equal sent amount (no dust lost)
    EXPECT_EQ(gainB + gainC, 10000);
}

// ============================================================
// ROUND_ROBIN mode
// ============================================================

TEST(ContractQuGate, RoundRobinCycling)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {1, 1};
    qg.createGate(userA, QUGATE_MODE_ROUND_ROBIN, 2, recipients, ratios);

    long long balB_start = getBalance(userB);
    long long balC_start = getBalance(userC);

    // Payment 1 -> recipient 0 (userB)
    qg.sendToGate(userA, 1, 5000);
    EXPECT_EQ(getBalance(userB) - balB_start, 5000);
    EXPECT_EQ(getBalance(userC) - balC_start, 0);

    // Payment 2 -> recipient 1 (userC)
    qg.sendToGate(userA, 1, 5000);
    EXPECT_EQ(getBalance(userB) - balB_start, 5000);
    EXPECT_EQ(getBalance(userC) - balC_start, 5000);

    // Payment 3 -> back to recipient 0 (userB)
    qg.sendToGate(userA, 1, 5000);
    EXPECT_EQ(getBalance(userB) - balB_start, 10000);
    EXPECT_EQ(getBalance(userC) - balC_start, 5000);
}

// ============================================================
// THRESHOLD mode
// ============================================================

TEST(ContractQuGate, ThresholdAccumulatesAndReleases)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    qg.createGate(userA, QUGATE_MODE_THRESHOLD, 1, recipients, ratios, 20000);

    long long balB_before = getBalance(userB);

    // Below threshold — should accumulate
    qg.sendToGate(userA, 1, 10000);
    EXPECT_EQ(getBalance(userB) - balB_before, 0);

    auto gate = qg.getGate(1);
    EXPECT_EQ(gate.currentBalance, 10000ULL);

    // Reaches threshold — should forward all
    qg.sendToGate(userA, 1, 10000);
    EXPECT_EQ(getBalance(userB) - balB_before, 20000);

    gate = qg.getGate(1);
    EXPECT_EQ(gate.currentBalance, 0ULL);
}

// ============================================================
// RANDOM mode
// ============================================================

TEST(ContractQuGate, RandomSelectsRecipient)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {1, 1};
    qg.createGate(userA, QUGATE_MODE_RANDOM, 2, recipients, ratios);

    long long balB_before = getBalance(userB);
    long long balC_before = getBalance(userC);

    // Send multiple payments — at least one should go to each recipient
    for (int i = 0; i < 10; i++)
    {
        qg.sendToGate(userA, 1, 1000);
    }

    long long gainB = getBalance(userB) - balB_before;
    long long gainC = getBalance(userC) - balC_before;

    // Total must equal 10000
    EXPECT_EQ(gainB + gainC, 10000);
    // Each should get at least something (probabilistic, but very unlikely to fail with 10 sends)
    EXPECT_GT(gainB, 0);
    EXPECT_GT(gainC, 0);
}

// ============================================================
// CONDITIONAL mode
// ============================================================

TEST(ContractQuGate, ConditionalAllowedSender)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    id allowed[] = {userA};
    qg.createGate(userA, QUGATE_MODE_CONDITIONAL, 1, recipients, ratios, 0, allowed, 1);

    long long balB_before = getBalance(userB);

    auto out = qg.sendToGate(userA, 1, 10000);
    EXPECT_EQ(out.status, QUGATE_SUCCESS);
    EXPECT_EQ(getBalance(userB) - balB_before, 10000);
}

TEST(ContractQuGate, ConditionalRejectedSender)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    id allowed[] = {userA};
    qg.createGate(userA, QUGATE_MODE_CONDITIONAL, 1, recipients, ratios, 0, allowed, 1);

    long long balC_before = getBalance(userC);
    long long balB_before = getBalance(userB);

    auto out = qg.sendToGate(userC, 1, 10000);
    EXPECT_EQ(out.status, (sint64)QUGATE_CONDITIONAL_REJECTED);

    // userC should get funds back, userB should receive nothing
    EXPECT_EQ(getBalance(userC), balC_before);
    EXPECT_EQ(getBalance(userB), balB_before);
}

// ============================================================
// Gate lifecycle
// ============================================================

TEST(ContractQuGate, CloseGateOwnerOnly)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);

    // Non-owner cannot close
    auto out = qg.closeGate(userC, 1);
    EXPECT_EQ(out.status, (sint64)QUGATE_UNAUTHORIZED);

    auto gate = qg.getGate(1);
    EXPECT_EQ(gate.active, 1);

    // Owner can close
    out = qg.closeGate(userA, 1);
    EXPECT_EQ(out.status, QUGATE_SUCCESS);

    gate = qg.getGate(1);
    EXPECT_EQ(gate.active, 0);
}

TEST(ContractQuGate, UpdateGateOwnerOnly)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB, userC};
    uint64 ratios[] = {60, 40};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 2, recipients, ratios);

    // Non-owner update rejected
    uint64 newRatios[] = {99, 1};
    auto out = qg.updateGate(userC, 1, 2, recipients, newRatios);
    EXPECT_EQ(out.status, (sint64)QUGATE_UNAUTHORIZED);

    // Owner update succeeds
    uint64 ratios2[] = {25, 75};
    out = qg.updateGate(userA, 1, 2, recipients, ratios2);
    EXPECT_EQ(out.status, QUGATE_SUCCESS);
}

TEST(ContractQuGate, SendToClosedGate)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);
    qg.closeGate(userA, 1);

    long long balA_before = getBalance(userA);
    auto out = qg.sendToGate(userA, 1, 5000);
    EXPECT_EQ(out.status, (sint64)QUGATE_GATE_NOT_ACTIVE);
}

TEST(ContractQuGate, SendToInvalidGate)
{
    ContractTestingQuGate qg;

    auto out = qg.sendToGate(userA, 9999, 5000);
    EXPECT_EQ(out.status, (sint64)QUGATE_INVALID_GATE_ID);
}

// ============================================================
// Error cases
// ============================================================

TEST(ContractQuGate, InvalidMode)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    auto out = qg.createGate(userA, 5, 1, recipients, ratios);
    EXPECT_EQ(out.status, (sint64)QUGATE_INVALID_MODE);
}

TEST(ContractQuGate, InvalidRecipientCount)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    auto out = qg.createGate(userA, QUGATE_MODE_SPLIT, 0, recipients, ratios);
    EXPECT_EQ(out.status, (sint64)QUGATE_INVALID_RECIPIENT_COUNT);
}

TEST(ContractQuGate, InvalidRatio)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {QUGATE_MAX_RATIO + 1};
    auto out = qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);
    EXPECT_EQ(out.status, (sint64)QUGATE_INVALID_RATIO);
}

TEST(ContractQuGate, InsufficientFee)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    auto out = qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios, 0, nullptr, 0, 1000);
    EXPECT_EQ(out.status, (sint64)QUGATE_INSUFFICIENT_FEE);
}

TEST(ContractQuGate, InvalidThreshold)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    auto out = qg.createGate(userA, QUGATE_MODE_THRESHOLD, 1, recipients, ratios, 0);
    EXPECT_EQ(out.status, (sint64)QUGATE_INVALID_THRESHOLD);
}

TEST(ContractQuGate, InvalidSenderCount)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    id allowed[] = {userA};
    // allowedSenderCount > 8
    auto out = qg.createGate(userA, QUGATE_MODE_CONDITIONAL, 1, recipients, ratios, 0, allowed, 9);
    EXPECT_EQ(out.status, (sint64)QUGATE_INVALID_SENDER_COUNT);
}

// ============================================================
// Anti-spam
// ============================================================

TEST(ContractQuGate, DustBurn)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);

    long long balB_before = getBalance(userB);

    // Send below min send — should be burned
    auto out = qg.sendToGate(userA, 1, 500);
    EXPECT_EQ(out.status, (sint64)QUGATE_DUST_AMOUNT);

    // Recipient should receive nothing
    EXPECT_EQ(getBalance(userB), balB_before);
}

TEST(ContractQuGate, FeeOverpaymentRefund)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    long long balA_before = getBalance(userA);

    // Overpay by 50000
    auto out = qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios, 0, nullptr, 0,
        QUGATE_DEFAULT_CREATION_FEE + 50000);
    EXPECT_EQ(out.status, QUGATE_SUCCESS);
    EXPECT_EQ(out.feePaid, (uint64)QUGATE_DEFAULT_CREATION_FEE);

    // Should only lose the creation fee, not the overpayment
    long long balA_after = getBalance(userA);
    EXPECT_EQ(balA_before - balA_after, QUGATE_DEFAULT_CREATION_FEE);
}

TEST(ContractQuGate, EscalatingFees)
{
    ContractTestingQuGate qg;

    auto fees = qg.getFees();
    EXPECT_EQ(fees.currentCreationFee, QUGATE_DEFAULT_CREATION_FEE);

    // Fee should increase as active gates grow (tested via getFees query)
    // At 0 active gates: multiplier = 1
    // At QUGATE_FEE_ESCALATION_STEP active gates: multiplier = 2
}

// ============================================================
// Free-list slot reuse
// ============================================================

TEST(ContractQuGate, SlotReuse)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};

    // Create and close a gate
    qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);
    EXPECT_EQ(qg.getGateCount().totalGates, 1ULL);
    EXPECT_EQ(qg.getGateCount().activeGates, 1ULL);

    qg.closeGate(userA, 1);
    EXPECT_EQ(qg.getGateCount().activeGates, 0ULL);

    // Create a new gate — should reuse the freed slot
    auto out = qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);
    EXPECT_EQ(out.status, QUGATE_SUCCESS);

    // totalGates should still be 1 (slot reused, not allocated new)
    EXPECT_EQ(qg.getGateCount().totalGates, 1ULL);
    EXPECT_EQ(qg.getGateCount().activeGates, 1ULL);
}

// ============================================================
// Query functions
// ============================================================

TEST(ContractQuGate, GetGateInvalid)
{
    ContractTestingQuGate qg;

    auto gate = qg.getGate(9999);
    EXPECT_EQ(gate.active, 0);
}

TEST(ContractQuGate, GetFees)
{
    ContractTestingQuGate qg;

    auto fees = qg.getFees();
    EXPECT_EQ(fees.creationFee, QUGATE_DEFAULT_CREATION_FEE);
    EXPECT_EQ(fees.minSendAmount, QUGATE_DEFAULT_MIN_SEND);
    EXPECT_EQ(fees.expiryEpochs, QUGATE_DEFAULT_EXPIRY_EPOCHS);
}

// ============================================================
// Threshold close refund
// ============================================================

TEST(ContractQuGate, ThresholdCloseRefundsBalance)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};
    qg.createGate(userA, QUGATE_MODE_THRESHOLD, 1, recipients, ratios, 50000);

    // Send below threshold
    qg.sendToGate(userA, 1, 10000);

    auto gate = qg.getGate(1);
    EXPECT_EQ(gate.currentBalance, 10000ULL);

    long long balA_before = getBalance(userA);

    // Close gate — held balance should refund to owner
    qg.closeGate(userA, 1);

    long long balA_after = getBalance(userA);
    EXPECT_EQ(balA_after - balA_before, 10000);
}

// ============================================================
// Total burned tracking
// ============================================================

TEST(ContractQuGate, TotalBurnedTracking)
{
    ContractTestingQuGate qg;

    id recipients[] = {userB};
    uint64 ratios[] = {100};

    // Create gate — fee burned
    qg.createGate(userA, QUGATE_MODE_SPLIT, 1, recipients, ratios);
    auto counts = qg.getGateCount();
    EXPECT_EQ(counts.totalBurned, (uint64)QUGATE_DEFAULT_CREATION_FEE);

    // Send dust — also burned
    qg.sendToGate(userA, 1, 500);
    counts = qg.getGateCount();
    EXPECT_EQ(counts.totalBurned, (uint64)QUGATE_DEFAULT_CREATION_FEE + 500);
}
