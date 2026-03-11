#define NO_UEFI

#include <iostream>

#include "contract_testing.h"

namespace {
constexpr unsigned short PROCEDURE_CREATE_ORDER = 1;
constexpr unsigned short PROCEDURE_TRANSFER_TO_CONTRACT = 4;

uint64 requiredFee(uint64 amount)
{
    // Total fee is 0.5% (ETH) + 0.5% (Qubic) = 1% of amount
    return 2 * ((amount * 5000000ULL) / 1000000000ULL);
}
}

class ContractTestingVottunBridge : protected ContractTesting
{
public:
    using ContractTesting::invokeUserProcedure;

    ContractTestingVottunBridge()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(VOTTUNBRIDGE);
        callSystemProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, INITIALIZE);
    }

    VOTTUNBRIDGE::StateData* state()
    {
        return reinterpret_cast<VOTTUNBRIDGE::StateData*>(contractStates[VOTTUNBRIDGE_CONTRACT_INDEX]);
    }

    bool findOrder(uint64 orderId, VOTTUNBRIDGE::BridgeOrder& out)
    {
        for (uint64 i = 0; i < state()->orders.capacity(); ++i)
        {
            VOTTUNBRIDGE::BridgeOrder order = state()->orders.get(i);
            if (order.orderId == orderId)
            {
                out = order;
                return true;
            }
        }
        return false;
    }

    bool findProposal(uint64 proposalId, VOTTUNBRIDGE::AdminProposal& out)
    {
        for (uint64 i = 0; i < state()->proposals.capacity(); ++i)
        {
            VOTTUNBRIDGE::AdminProposal proposal = state()->proposals.get(i);
            if (proposal.proposalId == proposalId)
            {
                out = proposal;
                return true;
            }
        }
        return false;
    }

    bool setOrderById(uint64 orderId, const VOTTUNBRIDGE::BridgeOrder& updated)
    {
        for (uint64 i = 0; i < state()->orders.capacity(); ++i)
        {
            VOTTUNBRIDGE::BridgeOrder order = state()->orders.get(i);
            if (order.orderId == orderId)
            {
                state()->orders.set(i, updated);
                return true;
            }
        }
        return false;
    }

    bool setProposalById(uint64 proposalId, const VOTTUNBRIDGE::AdminProposal& updated)
    {
        for (uint64 i = 0; i < state()->proposals.capacity(); ++i)
        {
            VOTTUNBRIDGE::AdminProposal proposal = state()->proposals.get(i);
            if (proposal.proposalId == proposalId)
            {
                state()->proposals.set(i, updated);
                return true;
            }
        }
        return false;
    }

    VOTTUNBRIDGE::createOrder_output createOrder(
        const id& user, uint64 amount, bit fromQubicToEthereum, uint64 fee)
    {
        VOTTUNBRIDGE::createOrder_input input{};
        VOTTUNBRIDGE::createOrder_output output{};
        input.qubicDestination = id(9, 0, 0, 0);
        input.amount = amount;
        input.fromQubicToEthereum = fromQubicToEthereum;
        for (uint64 i = 0; i < 42; ++i)
        {
            input.ethAddress.set(i, static_cast<uint8>('A'));
        }

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, PROCEDURE_CREATE_ORDER,
                                  input, output, user, static_cast<sint64>(fee));
        return output;
    }

    void seedBalance(const id& user, uint64 amount)
    {
        increaseEnergy(user, amount);
    }

    VOTTUNBRIDGE::transferToContract_output transferToContract(
        const id& user, uint64 amount, uint64 orderId, uint64 invocationReward)
    {
        VOTTUNBRIDGE::transferToContract_input input{};
        VOTTUNBRIDGE::transferToContract_output output{};
        input.amount = amount;
        input.orderId = orderId;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, PROCEDURE_TRANSFER_TO_CONTRACT,
                                  input, output, user, static_cast<sint64>(invocationReward));
        return output;
    }

    VOTTUNBRIDGE::createProposal_output createProposal(
        const id& admin, uint8 proposalType, const id& target, const id& oldAddress, uint64 amount)
    {
        VOTTUNBRIDGE::createProposal_input input{};
        VOTTUNBRIDGE::createProposal_output output{};
        input.proposalType = proposalType;
        input.targetAddress = target;
        input.oldAddress = oldAddress;
        input.amount = amount;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 6, input, output, admin, 0);
        return output;
    }

    VOTTUNBRIDGE::approveProposal_output approveProposal(const id& admin, uint64 proposalId)
    {
        VOTTUNBRIDGE::approveProposal_input input{};
        VOTTUNBRIDGE::approveProposal_output output{};
        input.proposalId = proposalId;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 7, input, output, admin, 0);
        return output;
    }

    VOTTUNBRIDGE::cancelProposal_output cancelProposal(const id& user, uint64 proposalId)
    {
        VOTTUNBRIDGE::cancelProposal_input input{};
        VOTTUNBRIDGE::cancelProposal_output output{};
        input.proposalId = proposalId;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 8, input, output, user, 0);
        return output;
    }

    VOTTUNBRIDGE::refundOrder_output refundOrder(const id& manager, uint64 orderId)
    {
        VOTTUNBRIDGE::refundOrder_input input{};
        VOTTUNBRIDGE::refundOrder_output output{};
        input.orderId = orderId;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 3, input, output, manager, 0);
        return output;
    }

    void callEndTick()
    {
        callSystemProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, END_TICK);
    }
};

TEST(VottunBridge, CreateOrder_RequiresFee)
{
    ContractTestingVottunBridge bridge;
    const id user = id(1, 0, 0, 0);
    const uint64 amount = 1000;
    const uint64 fee = requiredFee(amount);

    std::cout << "[VottunBridge] CreateOrder_RequiresFee: amount=" << amount
              << " fee=" << fee << " (sending fee-1)" << std::endl;

    increaseEnergy(user, fee - 1);
    auto output = bridge.createOrder(user, amount, true, fee - 1);
    EXPECT_EQ(output.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::insufficientTransactionFee));
}

TEST(VottunBridge, TransferToContract_RejectsMissingReward)
{
    ContractTestingVottunBridge bridge;
    const id user = id(2, 0, 0, 0);
    const uint64 amount = 200;
    const uint64 fee = requiredFee(amount);
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);

    std::cout << "[VottunBridge] TransferToContract_RejectsMissingReward: amount=" << amount
              << " fee=" << fee << " reward=0 contractBalanceSeed=1000" << std::endl;

    // Seed balances: user only has fees; contract already has balance > amount
    increaseEnergy(user, fee);
    increaseEnergy(contractId, 1000);

    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    const uint64 lockedBefore = bridge.state()->lockedTokens;
    const long long contractBalanceBefore = getBalance(contractId);
    const long long userBalanceBefore = getBalance(user);

    auto transferOutput = bridge.transferToContract(user, amount, orderOutput.orderId, 0);

    EXPECT_EQ(transferOutput.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::invalidAmount));
    EXPECT_EQ(bridge.state()->lockedTokens, lockedBefore);
    EXPECT_EQ(getBalance(contractId), contractBalanceBefore);
    EXPECT_EQ(getBalance(user), userBalanceBefore);
}

TEST(VottunBridge, TransferToContract_AcceptsExactReward)
{
    ContractTestingVottunBridge bridge;
    const id user = id(3, 0, 0, 0);
    const uint64 amount = 500;
    const uint64 fee = requiredFee(amount);
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);

    std::cout << "[VottunBridge] TransferToContract_AcceptsExactReward: amount=" << amount
              << " fee=" << fee << " reward=amount" << std::endl;

    increaseEnergy(user, fee + amount);

    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    const uint64 lockedBefore = bridge.state()->lockedTokens;
    const long long contractBalanceBefore = getBalance(contractId);

    auto transferOutput = bridge.transferToContract(user, amount, orderOutput.orderId, amount);

    EXPECT_EQ(transferOutput.status, 0);
    EXPECT_EQ(bridge.state()->lockedTokens, lockedBefore + amount);
    EXPECT_EQ(getBalance(contractId), contractBalanceBefore + amount);
}

TEST(VottunBridge, TransferToContract_OrderNotFound)
{
    ContractTestingVottunBridge bridge;
    const id user = id(5, 0, 0, 0);
    const uint64 amount = 100;

    bridge.seedBalance(user, amount);

    auto output = bridge.transferToContract(user, amount, 9999, amount);
    EXPECT_EQ(output.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::orderNotFound));
}

TEST(VottunBridge, TransferToContract_InvalidAmountMismatch)
{
    ContractTestingVottunBridge bridge;
    const id user = id(6, 0, 0, 0);
    const uint64 amount = 1000;  // Must be >= minimumOrderAmount (200)
    const uint64 fee = requiredFee(amount);

    bridge.seedBalance(user, fee + amount + 1);

    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    auto output = bridge.transferToContract(user, amount + 1, orderOutput.orderId, amount + 1);
    EXPECT_EQ(output.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::invalidAmount));
}

TEST(VottunBridge, TransferToContract_InvalidOrderState)
{
    ContractTestingVottunBridge bridge;
    const id user = id(7, 0, 0, 0);
    const uint64 amount = 1000;
    const uint64 fee = requiredFee(amount);

    bridge.seedBalance(user, fee + amount);

    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    VOTTUNBRIDGE::BridgeOrder order{};
    ASSERT_TRUE(bridge.findOrder(orderOutput.orderId, order));
    order.status = 1; // completed
    ASSERT_TRUE(bridge.setOrderById(orderOutput.orderId, order));

    auto output = bridge.transferToContract(user, amount, orderOutput.orderId, amount);
    EXPECT_EQ(output.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::invalidOrderState));
}

TEST(VottunBridge, CreateOrder_CleansCompletedAndRefundedSlots)
{
    ContractTestingVottunBridge bridge;
    const id user = id(4, 0, 0, 0);
    const uint64 amount = 1000;
    const uint64 fee = requiredFee(amount);

    VOTTUNBRIDGE::BridgeOrder filledOrder{};
    filledOrder.orderId = 1;
    filledOrder.amount = amount;
    filledOrder.status = 1; // completed
    filledOrder.fromQubicToEthereum = true;
    filledOrder.qubicSender = user;

    for (uint64 i = 0; i < bridge.state()->orders.capacity(); ++i)
    {
        filledOrder.orderId = i + 1;
        filledOrder.status = (i % 2 == 0) ? 1 : 2; // completed/refunded
        bridge.state()->orders.set(i, filledOrder);
    }

    increaseEnergy(user, fee);
    auto output = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(output.status, 0);

    VOTTUNBRIDGE::BridgeOrder createdOrder{};
    EXPECT_TRUE(bridge.findOrder(output.orderId, createdOrder));
    EXPECT_EQ(createdOrder.status, 0);

    // Single-pass recycling: the recyclable slot is directly overwritten
    // with the new order (status=0), so no slots end up as 255.
    // Verify that the number of completed/refunded orders decreased by 1
    // (one was recycled for the new order).
    uint64 recycledCount = 0;
    for (uint64 i = 0; i < bridge.state()->orders.capacity(); ++i)
    {
        uint8 s = bridge.state()->orders.get(i).status;
        if (s == 1 || s == 2)
        {
            recycledCount++;
        }
    }
    EXPECT_EQ(recycledCount, bridge.state()->orders.capacity() - 1);
}

TEST(VottunBridge, CreateProposal_CleansExecutedProposalsWhenFull)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(10, 0, 0, 0);
    const id admin2 = id(11, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    for (uint64 i = 2; i < bridge.state()->admins.capacity(); ++i)
    {
        bridge.state()->admins.set(i, NULL_ID);
    }

    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);

    VOTTUNBRIDGE::AdminProposal proposal{};
    proposal.approvalsCount = 1;
    proposal.active = true;
    proposal.executed = false;
    for (uint64 i = 0; i < bridge.state()->proposals.capacity(); ++i)
    {
        proposal.proposalId = i + 1;
        proposal.executed = (i % 2 == 0);
        bridge.state()->proposals.set(i, proposal);
    }

    auto output = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                        NULL_ID, NULL_ID, 2);

    EXPECT_EQ(output.status, 0);

    VOTTUNBRIDGE::AdminProposal createdProposal{};
    EXPECT_TRUE(bridge.findProposal(output.proposalId, createdProposal));
    EXPECT_TRUE(createdProposal.active);
    EXPECT_FALSE(createdProposal.executed);

    // Single-pass recycling: the executed slot is cleared and immediately
    // filled with the new proposal, so no slots end up fully empty.
    // Verify that the number of executed proposals decreased by 1.
    uint64 executedCount = 0;
    for (uint64 i = 0; i < bridge.state()->proposals.capacity(); ++i)
    {
        VOTTUNBRIDGE::AdminProposal p = bridge.state()->proposals.get(i);
        if (p.executed)
        {
            executedCount++;
        }
    }
    // Originally half (16) were executed; one was recycled for the new proposal
    EXPECT_EQ(executedCount, 15);
}

TEST(VottunBridge, CreateProposal_InvalidTypeRejected)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(10, 0, 0, 0);

    bridge.state()->numberOfAdmins = 1;
    bridge.state()->requiredApprovals = 1;
    bridge.state()->admins.set(0, admin1);
    bridge.seedBalance(admin1, 1);

    auto output = bridge.createProposal(admin1, 99, NULL_ID, NULL_ID, 0);
    EXPECT_EQ(output.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::invalidAmount));
}

TEST(VottunBridge, ApproveProposal_NotOwnerRejected)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(10, 0, 0, 0);
    const id admin2 = id(11, 0, 0, 0);
    const id outsider = id(99, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);
    bridge.seedBalance(outsider, 1);

    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    auto approveOutput = bridge.approveProposal(outsider, proposalOutput.proposalId);
    EXPECT_EQ(approveOutput.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::notOwner));
    EXPECT_FALSE(approveOutput.executed);
}

TEST(VottunBridge, ApproveProposal_DoubleApprovalRejected)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(10, 0, 0, 0);
    const id admin2 = id(11, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);

    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    auto approveOutput = bridge.approveProposal(admin1, proposalOutput.proposalId);
    EXPECT_EQ(approveOutput.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::proposalAlreadyApproved));
    EXPECT_FALSE(approveOutput.executed);
}

TEST(VottunBridge, ApproveProposal_ExecutesChangeThreshold)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(10, 0, 0, 0);
    const id admin2 = id(11, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);

    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    auto approveOutput = bridge.approveProposal(admin2, proposalOutput.proposalId);
    EXPECT_EQ(approveOutput.status, 0);
    EXPECT_TRUE(approveOutput.executed);
    EXPECT_EQ(bridge.state()->requiredApprovals, 2);
}

TEST(VottunBridge, ApproveProposal_ProposalNotFound)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(12, 0, 0, 0);

    bridge.state()->numberOfAdmins = 1;
    bridge.state()->requiredApprovals = 1;
    bridge.state()->admins.set(0, admin1);
    bridge.seedBalance(admin1, 1);

    auto output = bridge.approveProposal(admin1, 12345);
    EXPECT_EQ(output.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::proposalNotFound));
    EXPECT_FALSE(output.executed);
}

TEST(VottunBridge, ApproveProposal_AlreadyExecuted)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(13, 0, 0, 0);
    const id admin2 = id(14, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);

    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    auto approveOutput = bridge.approveProposal(admin2, proposalOutput.proposalId);
    EXPECT_EQ(approveOutput.status, 0);
    EXPECT_TRUE(approveOutput.executed);

    auto secondApprove = bridge.approveProposal(admin1, proposalOutput.proposalId);
    EXPECT_EQ(secondApprove.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::proposalAlreadyExecuted));
    EXPECT_FALSE(secondApprove.executed);
}

TEST(VottunBridge, TransferToContract_RefundsExcess)
{
    ContractTestingVottunBridge bridge;
    const id user = id(20, 0, 0, 0);
    const uint64 amount = 500;
    const uint64 excess = 100;
    const uint64 fee = requiredFee(amount);
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);

    std::cout << "[VottunBridge] TransferToContract_RefundsExcess: amount=" << amount
              << " excess=" << excess << " fee=" << fee << std::endl;

    increaseEnergy(user, fee + amount + excess);

    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    const uint64 lockedBefore = bridge.state()->lockedTokens;
    const long long userBalanceBefore = getBalance(user);

    // Send more than required (amount + excess)
    auto transferOutput = bridge.transferToContract(user, amount, orderOutput.orderId, amount + excess);

    EXPECT_EQ(transferOutput.status, 0);
    // Should lock only the required amount
    EXPECT_EQ(bridge.state()->lockedTokens, lockedBefore + amount);
    // User should get excess back
    EXPECT_EQ(getBalance(user), userBalanceBefore - amount);
}

TEST(VottunBridge, TransferToContract_RefundsAllOnInsufficient)
{
    ContractTestingVottunBridge bridge;
    const id user = id(21, 0, 0, 0);
    const uint64 amount = 500;
    const uint64 insufficientAmount = 200;
    const uint64 fee = requiredFee(amount);
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);

    std::cout << "[VottunBridge] TransferToContract_RefundsAllOnInsufficient: amount=" << amount
              << " sent=" << insufficientAmount << std::endl;

    increaseEnergy(user, fee + insufficientAmount);

    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    const uint64 lockedBefore = bridge.state()->lockedTokens;
    const long long userBalanceBefore = getBalance(user);

    // Send less than required
    auto transferOutput = bridge.transferToContract(user, amount, orderOutput.orderId, insufficientAmount);

    EXPECT_EQ(transferOutput.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::invalidAmount));
    // Should NOT lock anything
    EXPECT_EQ(bridge.state()->lockedTokens, lockedBefore);
    // User should get everything back
    EXPECT_EQ(getBalance(user), userBalanceBefore);
}

TEST(VottunBridge, CancelProposal_Success)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(22, 0, 0, 0);

    bridge.state()->numberOfAdmins = 1;
    bridge.state()->requiredApprovals = 1;
    bridge.state()->admins.set(0, admin1);
    bridge.seedBalance(admin1, 1);

    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    // Verify proposal is active
    VOTTUNBRIDGE::AdminProposal proposal;
    EXPECT_TRUE(bridge.findProposal(proposalOutput.proposalId, proposal));
    EXPECT_TRUE(proposal.active);

    // Cancel the proposal
    auto cancelOutput = bridge.cancelProposal(admin1, proposalOutput.proposalId);
    EXPECT_EQ(cancelOutput.status, 0);

    // Verify proposal is inactive
    EXPECT_TRUE(bridge.findProposal(proposalOutput.proposalId, proposal));
    EXPECT_FALSE(proposal.active);
}

TEST(VottunBridge, CancelProposal_NotCreatorRejected)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(23, 0, 0, 0);
    const id admin2 = id(24, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);

    // Admin1 creates proposal
    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    // Admin2 tries to cancel (should fail - not the creator)
    auto cancelOutput = bridge.cancelProposal(admin2, proposalOutput.proposalId);
    EXPECT_EQ(cancelOutput.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::notAuthorized));

    // Verify proposal is still active
    VOTTUNBRIDGE::AdminProposal proposal;
    EXPECT_TRUE(bridge.findProposal(proposalOutput.proposalId, proposal));
    EXPECT_TRUE(proposal.active);
}

TEST(VottunBridge, CancelProposal_AlreadyExecutedRejected)
{
    ContractTestingVottunBridge bridge;
    const id admin1 = id(25, 0, 0, 0);
    const id admin2 = id(26, 0, 0, 0);

    bridge.state()->numberOfAdmins = 2;
    bridge.state()->requiredApprovals = 2;
    bridge.state()->admins.set(0, admin1);
    bridge.state()->admins.set(1, admin2);
    bridge.seedBalance(admin1, 1);
    bridge.seedBalance(admin2, 1);

    auto proposalOutput = bridge.createProposal(admin1, VOTTUNBRIDGE::PROPOSAL_CHANGE_THRESHOLD,
                                                NULL_ID, NULL_ID, 2);
    EXPECT_EQ(proposalOutput.status, 0);

    // Execute the proposal by approving with a different admin (threshold is 2)
    auto approveOutput = bridge.approveProposal(admin2, proposalOutput.proposalId);
    EXPECT_EQ(approveOutput.status, 0);
    EXPECT_TRUE(approveOutput.executed);

    // Ensure proposal is marked executed in state (explicit for this cancellation test)
    VOTTUNBRIDGE::AdminProposal proposal;
    ASSERT_TRUE(bridge.findProposal(proposalOutput.proposalId, proposal));
    proposal.executed = true;
    ASSERT_TRUE(bridge.setProposalById(proposalOutput.proposalId, proposal));
    ASSERT_TRUE(bridge.findProposal(proposalOutput.proposalId, proposal));
    ASSERT_TRUE(proposal.executed);

    // Trying to cancel an executed proposal should fail
    auto cancelOutput = bridge.cancelProposal(admin1, proposalOutput.proposalId);
    EXPECT_EQ(cancelOutput.status, static_cast<uint8>(VOTTUNBRIDGE::EthBridgeError::proposalAlreadyExecuted));
}
// Additional tests for VottunBridge refund functionality with reserved fees
// Append these tests to contract_vottunbridge.cpp

TEST(VottunBridge, RefundOrder_ReturnsFullAmountPlusFees)
{
    ContractTestingVottunBridge bridge;
    const id user = id(100, 0, 0, 0);
    const id manager = id(101, 0, 0, 0);
    const uint64 amount = 10000;
    const uint64 fee = requiredFee(amount); // 100 QU (50 + 50)

    std::cout << "[VottunBridge] RefundOrder_ReturnsFullAmountPlusFees: amount=" << amount
              << " fee=" << fee << std::endl;

    // Setup: Add manager to managers array
    bridge.state()->managers.set(0, manager);
    bridge.seedBalance(manager, 1); // Ensure manager exists in spectrum

    // Give contract balance for refunds
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);
    bridge.seedBalance(contractId, amount + fee);

    // User creates order and transfers tokens
    bridge.seedBalance(user, fee + amount);

    const long long userBalanceBefore = getBalance(user);

    // Create order (user pays fees)
    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);
    EXPECT_GT(orderOutput.orderId, 0);

    // Transfer tokens to contract
    auto transferOutput = bridge.transferToContract(user, amount, orderOutput.orderId, amount);
    EXPECT_EQ(transferOutput.status, 0);

    // Verify fees are reserved
    EXPECT_EQ(bridge.state()->_reservedFees, 50);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 50);

    // Call END_TICK to simulate tick completion
    bridge.callEndTick();

    // Verify reserved fees were NOT distributed (still reserved for pending order)
    EXPECT_EQ(bridge.state()->_reservedFees, 50);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 50);

    // Manager refunds the order
    std::cout << "  Contract balance BEFORE refund: " << getBalance(id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0)) << std::endl;
    std::cout << "  User balance BEFORE refund: " << getBalance(user) << std::endl;

    auto refundOutput = bridge.refundOrder(manager, orderOutput.orderId);

    std::cout << "  Refund status: " << (int)refundOutput.status << std::endl;
    std::cout << "  Contract balance AFTER refund: " << getBalance(id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0)) << std::endl;
    std::cout << "  User balance AFTER refund: " << getBalance(user) << std::endl;
    EXPECT_EQ(refundOutput.status, 0);

    // Verify order is refunded
    VOTTUNBRIDGE::BridgeOrder order{};
    ASSERT_TRUE(bridge.findOrder(orderOutput.orderId, order));
    std::cout << "  Order status after refund: " << (int)order.status << " (expected 2)" << std::endl;
    std::cout << "  Order tokensReceived: " << order.tokensReceived << std::endl;
    std::cout << "  Order tokensLocked: " << order.tokensLocked << std::endl;
    std::cout << "  lockedTokens state: " << bridge.state()->lockedTokens << std::endl;
    EXPECT_EQ(order.status, 2); // Refunded

    // Verify fees are no longer reserved
    EXPECT_EQ(bridge.state()->_reservedFees, 0);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 0);

    // Verify user received full refund: amount + both fees
    const long long userBalanceAfter = getBalance(user);
    EXPECT_EQ(userBalanceAfter, userBalanceBefore); // Back to original balance

    std::cout << "  ✅ User balance: before=" << userBalanceBefore
              << " after=" << userBalanceAfter
              << " (full refund verified)" << std::endl;
}

TEST(VottunBridge, RefundOrder_BeforeTransfer_ReturnsOnlyFees)
{
    ContractTestingVottunBridge bridge;
    const id user = id(102, 0, 0, 0);
    const id manager = id(103, 0, 0, 0);
    const uint64 amount = 5000;
    const uint64 fee = requiredFee(amount); // 50 QU (25 + 25)

    std::cout << "[VottunBridge] RefundOrder_BeforeTransfer_ReturnsOnlyFees: amount=" << amount
              << " fee=" << fee << std::endl;

    // Setup: Add manager
    bridge.state()->managers.set(0, manager);
    bridge.seedBalance(manager, 1); // Ensure manager exists in spectrum

    // Give contract balance for refunds
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);
    bridge.seedBalance(contractId, fee);

    // User creates order but does NOT transfer tokens
    bridge.seedBalance(user, fee);

    const long long userBalanceBefore = getBalance(user);

    // Create order (user pays fees)
    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    // Verify fees are reserved
    EXPECT_EQ(bridge.state()->_reservedFees, 25);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 25);

    const long long userBalanceAfterCreate = getBalance(user);
    EXPECT_EQ(userBalanceAfterCreate, userBalanceBefore - fee);

    // Manager refunds the order (before transferToContract)
    auto refundOutput = bridge.refundOrder(manager, orderOutput.orderId);
    EXPECT_EQ(refundOutput.status, 0);

    // Verify user received fees back (no tokens to refund since they were never transferred)
    const long long userBalanceAfter = getBalance(user);
    EXPECT_EQ(userBalanceAfter, userBalanceBefore); // Back to original (fees refunded)

    std::cout << "  ✅ User balance: before=" << userBalanceBefore
              << " afterCreate=" << userBalanceAfterCreate
              << " afterRefund=" << userBalanceAfter
              << " (fees refunded)" << std::endl;
}

TEST(VottunBridge, RefundOrder_AfterEndTick_StillRefundsFullAmount)
{
    ContractTestingVottunBridge bridge;
    const id user = id(104, 0, 0, 0);
    const id manager = id(105, 0, 0, 0);
    const uint64 amount = 10000;
    const uint64 fee = requiredFee(amount);

    std::cout << "[VottunBridge] RefundOrder_AfterEndTick_StillRefundsFullAmount" << std::endl;

    // Setup
    bridge.state()->managers.set(0, manager);
    bridge.seedBalance(manager, 1); // Ensure manager exists in spectrum

    // Give contract balance for refunds
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);
    bridge.seedBalance(contractId, amount + fee);

    bridge.seedBalance(user, fee + amount);

    const long long userBalanceBefore = getBalance(user);

    // Create order and transfer
    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    auto transferOutput = bridge.transferToContract(user, amount, orderOutput.orderId, amount);
    EXPECT_EQ(transferOutput.status, 0);

    // Simulate multiple ticks (fees should NOT be distributed while order is pending)
    for (int i = 0; i < 5; i++)
    {
        bridge.callEndTick();
    }

    // Verify fees are STILL reserved (not distributed during END_TICK)
    EXPECT_EQ(bridge.state()->_reservedFees, 50);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 50);

    // Refund after multiple ticks
    auto refundOutput = bridge.refundOrder(manager, orderOutput.orderId);
    EXPECT_EQ(refundOutput.status, 0);

    // User should still get full refund
    const long long userBalanceAfter = getBalance(user);
    EXPECT_EQ(userBalanceAfter, userBalanceBefore);

    std::cout << "  ✅ Fees remained reserved through 5 ticks, full refund successful" << std::endl;
}

TEST(VottunBridge, CreateOrder_ReservesFees)
{
    ContractTestingVottunBridge bridge;
    const id user = id(106, 0, 0, 0);
    const uint64 amount = 1000;
    const uint64 fee = requiredFee(amount); // 10 QU (5 + 5)

    bridge.seedBalance(user, fee);

    // Initially, no fees are reserved
    EXPECT_EQ(bridge.state()->_reservedFees, 0);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 0);
    EXPECT_EQ(bridge.state()->_earnedFees, 0);
    EXPECT_EQ(bridge.state()->_earnedFeesQubic, 0);

    // Create order
    auto orderOutput = bridge.createOrder(user, amount, true, fee);
    EXPECT_EQ(orderOutput.status, 0);

    // Verify fees are both earned AND reserved
    EXPECT_EQ(bridge.state()->_earnedFees, 5);
    EXPECT_EQ(bridge.state()->_earnedFeesQubic, 5);
    EXPECT_EQ(bridge.state()->_reservedFees, 5);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 5);
}

TEST(VottunBridge, CompleteOrder_ReleasesReservedFees)
{
    ContractTestingVottunBridge bridge;
    const id user = id(107, 0, 0, 0);
    const id manager = id(108, 0, 0, 0);
    const uint64 amount = 1000;
    const uint64 fee = requiredFee(amount);

    // Setup
    bridge.state()->managers.set(0, manager);
    bridge.seedBalance(manager, 1); // Ensure manager exists in spectrum
    bridge.state()->lockedTokens = 10000; // Ensure enough liquidity for EVM->Qubic
    const id contractId = id(VOTTUNBRIDGE_CONTRACT_INDEX, 0, 0, 0);
    bridge.seedBalance(contractId, amount); // Ensure contract can transfer to user
    bridge.seedBalance(user, fee);

    // Create order (EVM->Qubic direction, fromQubicToEthereum=false)
    auto orderOutput = bridge.createOrder(user, amount, false, fee);
    EXPECT_EQ(orderOutput.status, 0);

    // Verify fees are reserved
    EXPECT_EQ(bridge.state()->_reservedFees, 5);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 5);

    // Manager completes the order
    VOTTUNBRIDGE::completeOrder_input input{};
    VOTTUNBRIDGE::completeOrder_output output{};
    input.orderId = orderOutput.orderId;

    bridge.invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 2, input, output, manager, 0);
    EXPECT_EQ(output.status, 0);

    // Verify reserved fees are released (can now be distributed)
    EXPECT_EQ(bridge.state()->_reservedFees, 0);
    EXPECT_EQ(bridge.state()->_reservedFeesQubic, 0);

    // Earned fees remain (not distributed yet)
    EXPECT_EQ(bridge.state()->_earnedFees, 5);
    EXPECT_EQ(bridge.state()->_earnedFeesQubic, 5);
}
