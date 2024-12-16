#define NO_UEFI

#include "contract_testing.h"
#include <map>
#include <vector>

#define PRINT_TEST_INFO 0

static const id ETHBRIDGE_CONTRACT_ID(ETHBRIDGE_CONTRACT_INDEX, 0, 0, 0);
static std::mt19937_64 rand64;

// Helper function to generate unique user IDs
static id getUser(unsigned long long i) {
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

// Helper function to generate random values
static unsigned long long random(unsigned long long maxValue) {
    return rand64() % (maxValue + 1);
}

// Helper to access contract state
ETHBRIDGE* getState() {
    return (ETHBRIDGE*)contractStates[ETHBRIDGE_CONTRACT_INDEX];
}

class ContractTestingEthBridge : protected ContractTesting {
public:
    ContractTestingEthBridge() {
        INIT_CONTRACT(ETHBRIDGE);
        rand64.seed(42);
    }

    sint32 setAdmin(const id& newAdmin, bool expectSuccess = true) {
        ETHBRIDGE::setAdmin_input input{ newAdmin };
        ETHBRIDGE::setAdmin_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 3, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    sint32 addManager(const id& manager, bool expectSuccess = true) {
        ETHBRIDGE::addManager_input input{ manager };
        ETHBRIDGE::addManager_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 4, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    sint32 removeManager(const id& manager, bool expectSuccess = true) {
        ETHBRIDGE::removeManager_input input{ manager };
        ETHBRIDGE::removeManager_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 5, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    sint32 createOrder(const id& ethAddress, uint64 amount, bit fromQubicToEthereum, bool expectSuccess = true) {
        ETHBRIDGE::createOrder_input input{ ethAddress, amount, fromQubicToEthereum };
        ETHBRIDGE::createOrder_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 1, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    sint32 completeOrder(uint64 orderId, bool expectSuccess = true) {
        ETHBRIDGE::completeOrder_input input{ orderId };
        ETHBRIDGE::completeOrder_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 6, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    sint32 refundOrder(uint64 orderId, bool expectSuccess = true) {
        ETHBRIDGE::refundOrder_input input{ orderId };
        ETHBRIDGE::refundOrder_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 7, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    sint32 transferToContract(uint64 amount, bool expectSuccess = true) {
        ETHBRIDGE::transferToContract_input input{ amount };
        ETHBRIDGE::transferToContract_output output;
        EXPECT_EQ(invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 8, input, output, qpi.invocator(), 0), expectSuccess);
        return output.status;
    }

    uint64 getTotalReceivedTokens() {
        ETHBRIDGE::getTotalReceivedTokens_input input{};
        ETHBRIDGE::getTotalReceivedTokens_output output;
        callFunction(ETHBRIDGE_CONTRACT_INDEX, 2, input, output);
        return output.totalTokens;
    }
};

// TESTS
TEST(TestContractEthBridge, TestSetAdmin) {
    ContractTestingEthBridge ethBridge;
    id admin(1, 1, 1, 1);
    id newAdmin(2, 2, 2, 2);

    // Successfully set new admin by the current admin
    EXPECT_EQ(ethBridge.setAdmin(newAdmin), 0);

    // Attempt to set admin by non-admin user (should fail)
    EXPECT_EQ(ethBridge.setAdmin(admin, false), 1);
}

TEST(TestContractEthBridge, TestAddRemoveManager) {
    ContractTestingEthBridge ethBridge;
    id admin(1, 1, 1, 1);
    id manager(2, 2, 2, 2);

    // Add a manager
    EXPECT_EQ(ethBridge.addManager(manager), 0);

    // Remove the manager
    EXPECT_EQ(ethBridge.removeManager(manager), 0);
}


TEST(TestContractEthBridge, TestCreateOrder) {
    ContractTestingEthBridge ethBridge;
    id sender(1, 1, 1, 1);
    id receiver(2, 2, 2, 2);

    // Create a valid order
    EXPECT_EQ(ethBridge.createOrder(sender, receiver, 100, true), 0);

    // Retrieve and verify the order
    auto order = ethBridge.getOrder(0);
    EXPECT_EQ(order.order.orderId, 0);
    EXPECT_EQ(order.order.originAccount, sender);
    EXPECT_EQ(order.order.destinationAccount, receiver);
    EXPECT_EQ(order.order.amount, 100);
    EXPECT_EQ(order.order.status, 0);

    // Attempt to create an order with invalid amount
    EXPECT_EQ(ethBridge.createOrder(sender, receiver, 0, true, false), 1);
}

TEST(TestContractEthBridge, TestCompleteOrder) {
    ContractTestingEthBridge ethBridge;
    id sender(1, 1, 1, 1);
    id receiver(2, 2, 2, 2);
    id manager(3, 3, 3, 3);

    // Add manager
    EXPECT_EQ(ethBridge.addManager(manager), 0);

    // Create an order
    ethBridge.createOrder(sender, receiver, 100, true);

    // Attempt to complete the order without permissions
    EXPECT_EQ(ethBridge.completeOrder(0, false), 1);

    // Complete the order with manager permissions
    EXPECT_EQ(ethBridge.completeOrder(0), 0);

    // Verify the order status
    auto order = ethBridge.getOrder(0);
    EXPECT_EQ(order.order.status, 1); // Completed
}

TEST(TestContractEthBridge, TestRefundOrder) {
    ContractTestingEthBridge ethBridge;
    id sender(1, 1, 1, 1);
    id receiver(2, 2, 2, 2);
    id manager(3, 3, 3, 3);

    // Add manager
    EXPECT_EQ(ethBridge.addManager(manager), 0);

    // Create an order
    ethBridge.createOrder(sender, receiver, 100, true);

    // Attempt to refund without permissions
    EXPECT_EQ(ethBridge.refundOrder(0, false), 1);

    // Refund the order with manager permissions
    EXPECT_EQ(ethBridge.refundOrder(0), 0);

    // Verify the order status
    auto order = ethBridge.getOrder(0);
    EXPECT_EQ(order.order.status, 2); // Refunded
}

TEST(TestContractEthBridge, TestTransferToContract) {
    ContractTestingEthBridge ethBridge;
    id sender(1, 1, 1, 1);

    // Transfer tokens to the contract
    EXPECT_EQ(ethBridge.transferToContract(500, sender), 0);

    // Verify total tokens
    EXPECT_EQ(ethBridge.getTotalTokens(), 500);

    // Attempt to transfer zero tokens
    EXPECT_EQ(ethBridge.transferToContract(0, false), 1);
}
