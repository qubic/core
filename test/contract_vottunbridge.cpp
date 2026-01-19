#define NO_UEFI

#include <iostream>

#include "contract_testing.h"

namespace {
constexpr unsigned short PROCEDURE_CREATE_ORDER = 1;
constexpr unsigned short PROCEDURE_TRANSFER_TO_CONTRACT = 6;

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

    VOTTUNBRIDGE* state()
    {
        return reinterpret_cast<VOTTUNBRIDGE*>(contractStates[VOTTUNBRIDGE_CONTRACT_INDEX]);
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

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 9, input, output, admin, 0);
        return output;
    }

    VOTTUNBRIDGE::approveProposal_output approveProposal(const id& admin, uint64 proposalId)
    {
        VOTTUNBRIDGE::approveProposal_input input{};
        VOTTUNBRIDGE::approveProposal_output output{};
        input.proposalId = proposalId;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 10, input, output, admin, 0);
        return output;
    }

    VOTTUNBRIDGE::cancelProposal_output cancelProposal(const id& user, uint64 proposalId)
    {
        VOTTUNBRIDGE::cancelProposal_input input{};
        VOTTUNBRIDGE::cancelProposal_output output{};
        input.proposalId = proposalId;

        this->invokeUserProcedure(VOTTUNBRIDGE_CONTRACT_INDEX, 11, input, output, user, 0);
        return output;
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
    const uint64 amount = 100;
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
    const uint64 amount = 150;
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

        uint64 emptySlots = 0;
    for (uint64 i = 0; i < bridge.state()->orders.capacity(); ++i)
        {
        if (bridge.state()->orders.get(i).status == 255)
            {
                emptySlots++;
            }
    }
    EXPECT_GT(emptySlots, 0);
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

    uint64 clearedSlots = 0;
    for (uint64 i = 0; i < bridge.state()->proposals.capacity(); ++i)
    {
        VOTTUNBRIDGE::AdminProposal p = bridge.state()->proposals.get(i);
        if (!p.active && p.proposalId == 0)
        {
            clearedSlots++;
        }
    }
    EXPECT_GT(clearedSlots, 0);
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
