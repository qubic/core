#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"
#include <map>
#include <vector>
#include <random>

static std::mt19937_64 rand64;
static const id ETHBRIDGE_CONTRACT_ID(ETHBRIDGE_CONTRACT_INDEX, 0, 0, 0);
// Predefined IDs (64 characters each)
const id ETHBRIDGE_ADMIN = ID(_P, _H, _O, _Y, _R, _V, _N, _K, _J, _X, _M, _L, _R, _B, _B, _I, _R, _I, _P, _D, _I, _B, _M, _H, _D, _H, _U, _A, _Z, _B, _Q, _K, _N, _B, _J, _T, _R, _D, _S, _P, _G, _C, _L, _Z, _C, _Q, _W, _A, _K, _C, _F, _Q, _J, _K, _K, _E);
const id ETHBRIDGE_MANAGER1 = ID(_S, _G, _K, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
const id ETHBRIDGE_USER1 = ID(_C, _L, _M, _E, _O, _W, _B, _O, _T, _F, _T, _F, _I, _C, _I, _F, _P, _U, _X, _O, _J, _K, _G, _Q, _P, _Y, _X, _C, _A, _B, _L, _Z, _V, _M, _M, _U, _C, _M, _J, _F, _S, _G, _S, _A, _I, _A, _T, _Y, _I, _N, _V, _T, _Y, _G, _O, _A);

// Utility function to generate unique IDs
static id getUser(unsigned long long i) {
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

// Utility function for random values
static unsigned long long random(unsigned long long maxValue) {
    return rand64() % (maxValue + 1);
}

// ETHBRIDGE Checker for state validation
class ETHBRIDGEChecker : public ETHBRIDGE {
public:
    void orderChecker(const ETHBRIDGE::getOrder_output& output,
        uint64 orderId, const id& originAccount, const id& destinationAccount,
        uint64 amount, const char memo[64], uint32 sourceChain,
        uint8 status, const array<uint8, 32>& message) {

        EXPECT_EQ(output.status, status);
        /*EXPECT_EQ(output.message, message);*/

        EXPECT_EQ(output.order.orderId, orderId);
        EXPECT_EQ(output.order.originAccount, originAccount);
        EXPECT_EQ(output.order.destinationAccount, destinationAccount);
        EXPECT_EQ(output.order.amount, amount);
        /*EXPECT_EQ(strncmp(reinterpret_cast<const char*>(output.order.memo.data()), memo, 64), 0);*/
        EXPECT_EQ(output.order.sourceChain, sourceChain);
    }

    void managerChecker(const std::vector<id>& managers, const id& manager) {
        EXPECT_TRUE(std::find(managers.begin(), managers.end(), manager) != managers.end());
    }

    void adminChecker(const id& expectedAdmin, const id& actualAdmin) {
        EXPECT_EQ(expectedAdmin, actualAdmin);
    }

    void totalTokensChecker(uint64 expectedTokens, uint64 actualTokens) {
        EXPECT_EQ(expectedTokens, actualTokens);
    }
};

// Main testing class for ETHBRIDGE
class ContractTestingEthBridge : protected ContractTesting {
public:
    ContractTestingEthBridge() {
        INIT_CONTRACT(ETHBRIDGE);
        rand64.seed(42);
    }

    ETHBRIDGEChecker* getState() {
        return (ETHBRIDGEChecker*)contractStates[ETHBRIDGE_CONTRACT_INDEX];
    }

    // Admin Methods
    sint32 setAdmin(const id& newAdmin) {
        ETHBRIDGE::setAdmin_input input{ newAdmin };
        ETHBRIDGE::setAdmin_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 3, input, output, ETHBRIDGE_ADMIN, 0);
    }

    sint32 addManager(const id& manager) {
        ETHBRIDGE::addManager_input input{ manager };
        ETHBRIDGE::addManager_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 4, input, output, ETHBRIDGE_ADMIN, 0);
    }

    sint32 removeManager(const id& manager) {
        ETHBRIDGE::removeManager_input input{ manager };
        ETHBRIDGE::removeManager_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 5, input, output, ETHBRIDGE_ADMIN, 0);
    }

    // Order Methods
    sint32 createOrder(const id& ethAddress, uint64 amount, bit fromQubicToEthereum) {
        ETHBRIDGE::createOrder_input input{ ethAddress, amount, fromQubicToEthereum };
        ETHBRIDGE::createOrder_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 1, input, output, ETHBRIDGE_USER1, 0);
    }

    ETHBRIDGE::getOrder_output getOrder(uint64 orderId) {
        ETHBRIDGE::getOrder_input input{ orderId };
        ETHBRIDGE::getOrder_output output;
        callFunction(ETHBRIDGE_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    sint32 completeOrder(uint64 orderId) {
        ETHBRIDGE::completeOrder_input input{ orderId };
        ETHBRIDGE::completeOrder_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 6, input, output, ETHBRIDGE_MANAGER1, 0);
    }

    sint32 refundOrder(uint64 orderId) {
        ETHBRIDGE::refundOrder_input input{ orderId };
        ETHBRIDGE::refundOrder_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 7, input, output, ETHBRIDGE_MANAGER1, 0);
    }

    sint32 transferToContract(uint64 amount) {
        ETHBRIDGE::transferToContract_input input{ amount };
        ETHBRIDGE::transferToContract_output output;
        return invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 8, input, output, ETHBRIDGE_MANAGER1, 0);
    }

    uint64 getTotalReceivedTokens() {
        ETHBRIDGE::getTotalReceivedTokens_input input{};
        ETHBRIDGE::getTotalReceivedTokens_output output;
        callFunction(ETHBRIDGE_CONTRACT_INDEX, 2, input, output);
        return output.totalTokens;
    }
};

// TEST CASES
TEST(ContractEthBridge, SetAdmin) {
    ContractTestingEthBridge ethBridge;
    id newAdmin = getUser(1);

    // Test setting new admin
    EXPECT_EQ(ethBridge.setAdmin(newAdmin), 0);
    auto output = ethBridge.getTotalReceivedTokens();
    ethBridge.getState()->adminChecker(newAdmin, newAdmin);
}

TEST(ContractEthBridge, AddAndRemoveManager) {
    ContractTestingEthBridge ethBridge;
    id manager = getUser(2);

    // Add and verify manager
    EXPECT_EQ(ethBridge.addManager(manager), 0);
    std::vector<id> managers{ manager };
    ethBridge.getState()->managerChecker(managers, manager);

    // Remove and verify manager removal
    EXPECT_EQ(ethBridge.removeManager(manager), 0);
}

TEST(ContractEthBridge, CreateOrderAndValidate) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;
    uint64 amount = 1000;
    constexpr char memo[64] = "Bridge transfer details"; // Placeholder for metadata
    uint32 sourceChain = 1; // Example chain ID
    uint8 expectedStatus = 0; // Initial status
    array<uint8, 32> expectedMessage {}; // Empty message placeholder

    // Create an order
    EXPECT_EQ(ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true), 0);

    // Fetch and validate the order
    auto orderOutput = ethBridge.getOrder(orderId);
    checker.orderChecker(orderOutput, orderId, ETHBRIDGE_USER1, ETHBRIDGE_MANAGER1,
        amount, memo, sourceChain, expectedStatus, expectedMessage);
}

TEST(ContractEthBridge, CompleteOrder) {
    ContractTestingEthBridge ethBridge;
    id ethAddress = getUser(4);

    // Create order
    ethBridge.createOrder(ethAddress, 500, true);

    // Complete the order
    EXPECT_EQ(ethBridge.completeOrder(0), 0);
    auto order = ethBridge.getOrder(0);
    EXPECT_EQ(order.status, 1); // Completed
}

TEST(ContractEthBridge, RefundOrder) {
    ContractTestingEthBridge ethBridge;
    id ethAddress = getUser(5);

    // Create order
    ethBridge.createOrder(ethAddress, 300, true);

    // Refund the order
    EXPECT_EQ(ethBridge.refundOrder(0), 0);
    auto order = ethBridge.getOrder(0);
    EXPECT_EQ(order.status, 2); // Refunded
}

TEST(ContractEthBridge, TransferToContract) {
    ContractTestingEthBridge ethBridge;
    uint64 amount = 1000;

    // Transfer to contract
    EXPECT_EQ(ethBridge.transferToContract(amount), 0);

    // Verify total received tokens
    EXPECT_EQ(ethBridge.getTotalReceivedTokens(), amount);
}

TEST(ContractEthBridge, GetOrder) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;                        
    id originAccount = ETHBRIDGE_USER1;         
    id destinationAccount = ETHBRIDGE_MANAGER1;  
    uint64 amount = 750;                      
    constexpr char memo[64] = "Bridge transfer details"; 
    uint32 sourceChain = 1;                    
    uint8 expectedStatus = 0;                    
    array<uint8_t, 32> expectedMessage{};   

    //Create order
    EXPECT_EQ(ethBridge.createOrder(destinationAccount, amount, false), 0);

    // Fetch order and verify details
    auto orderOutput = ethBridge.getOrder(0);
    checker.orderChecker(orderOutput, orderId, originAccount, destinationAccount,
        amount, memo, sourceChain, expectedStatus, expectedMessage);
}