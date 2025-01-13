#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"
#include <map>
#include <vector>
#include <random>

static std::mt19937_64 rand64;
static const id ETHBRIDGE_CONTRACT_ID(ETHBRIDGE_CONTRACT_INDEX, 0, 0, 0);
// Predefined IDs (55 characters each)
const id ETHBRIDGE_ADMIN = ID(_P, _X, _A, _B, _Y, _V, _D, _P, _J, _R, _R, _D, _K, _E, _L, _E, _Y, _S, _H, _Z, _W, _J, _C, _B, _E, _F, _J, _C, _N, _E, _R, _N, _K, _K, _U, _W, _X, _H, _A, _N, _C, _D, _P, _Q, _E, _F, _G, _D, _I, _U, _G, _U, _G, _A, _U, _B, _B, _C, _Y, _K);
const id ETHBRIDGE_MANAGER1 = ID(_F, _P, _O, _B, _X, _D, _X, _M, _C, _S, _N, _Z, _L, _C, _F, _W, _I, _E, _R, _C, _Y, _W, _Y, _S, _Q, _B, _L, _D, _P, _X, _R, _M, _H, _S, _D, _K, _V, _I, _F, _M, _T, _E, _S, _I, _U, _G, _N, _F, _L, _A, _Z, _L, _D, _L, _T, _D, _Y, _M, _G, _F);
const id ETHBRIDGE_USER1 = ID(_W, _D, _N, _G, _K, _C, _G, _N, _A, _T, _A, _N, _C, _E, _G, _B, _Y, _W, _A, _D, _J, _Q, _S, _A, _M, _W, _P, _A, _H, _Y, _S, _M, _E, _C, _K, _F, _G, _J, _F, _I, _C, _E, _T, _K, _G, _Y, _D, _X, _Z, _Q, _G, _H, _Z, _K, _H, _B, _T, _L, _H, _F);
const id ADDRESS2 = ID(_R, _G, _I, _C, _M, _Q, _T, _X, _U, _R, _X, _I, _I, _G, _G, _B, _B, _M, _G, _R, _I, _J, _Q, _A, _C, _I, _W, _A, _H, _M, _A, _A, _C, _L, _J, _T, _R, _G, _H, _I, _P, _A, _I, _S, _C, _V, _C, _P, _N, _L, _O, _E, _S, _M, _I, _H, _Q, _O, _C, _B);

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
        initEmptySpectrum();
        INIT_CONTRACT(ETHBRIDGE);
        callSystemProcedure(ETHBRIDGE_CONTRACT_INDEX, INITIALIZE);
        qLogger::initLogging();

        //increasing energy of each user (100 QU) so procedures can be invoke
        increaseEnergy(ETHBRIDGE_ADMIN, 100);
        increaseEnergy(ETHBRIDGE_MANAGER1, 100);
        increaseEnergy(ETHBRIDGE_USER1, 100);
        increaseEnergy(ADDRESS2, 100);

        rand64.seed(42);
    }

    ETHBRIDGEChecker* getState() {
        return (ETHBRIDGEChecker*)contractStates[ETHBRIDGE_CONTRACT_INDEX];
    }

    // Admin Methods
    sint32 setAdmin(const id& newAdmin) {
        ETHBRIDGE::setAdmin_input input{ newAdmin };
        ETHBRIDGE::setAdmin_output output;
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 3, input, output, ETHBRIDGE_ADMIN, 0);
        if (success && output.status != 0)
            success = false;
        return success;
    }

    sint32 addManager(const id& manager) {
        ETHBRIDGE::addManager_input input{ manager };
        ETHBRIDGE::addManager_output output;
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 4, input, output, ETHBRIDGE_ADMIN, 0);
        if (success && output.status != 0)
            success = false;
        return success;
    }

    sint32 removeManager(const id& manager) {
        ETHBRIDGE::removeManager_input input{ manager };
        ETHBRIDGE::removeManager_output output;
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 5, input, output, ETHBRIDGE_ADMIN, 0);
        if (success && output.status != 0)
            success = false;
        return success;
    }

    // Order Methods
    sint32 createOrder(const id& ethAddress, uint64 amount, bit fromQubicToEthereum) {
        ETHBRIDGE::createOrder_input input{ ethAddress, amount, fromQubicToEthereum };
        ETHBRIDGE::createOrder_output output;
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 1, input, output, ETHBRIDGE_USER1, 0);
        if (success && output.status != 0)
            success = false;
        return success;
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
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 6, input, output, ETHBRIDGE_MANAGER1, 0);
        if (success && output.status != 0)
            success = false;
        return success;
    }

    sint32 refundOrder(uint64 orderId) {
        ETHBRIDGE::refundOrder_input input{ orderId };
        ETHBRIDGE::refundOrder_output output;
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 7, input, output, ETHBRIDGE_MANAGER1, 0);
        if (success && output.status != 0)
            success = false;
        return success;
    }

    sint32 transferToContract(uint64 amount) {
        ETHBRIDGE::transferToContract_input input{ amount };
        ETHBRIDGE::transferToContract_output output;
        bool success = invokeUserProcedure(ETHBRIDGE_CONTRACT_INDEX, 8, input, output, ETHBRIDGE_MANAGER1, 0);
        if (success && output.status != 0)
            success = false;
        return success;
    }

    uint64 getTotalReceivedTokens() {
        ETHBRIDGE::getTotalReceivedTokens_input input{};
        ETHBRIDGE::getTotalReceivedTokens_output output;
        callFunction(ETHBRIDGE_CONTRACT_INDEX, 11, input, output);
        return output.totalTokens;
    }

    uint64 getTotalLockedTokens() {
        ETHBRIDGE::getTotalLockedTokens_input input{};
        ETHBRIDGE::getTotalLockedTokens_output output;
        callFunction(ETHBRIDGE_CONTRACT_INDEX, 14, input, output);
        return output.totalLockedTokens;
    }
};

// TEST CASES
TEST(ContractEthBridge, SetAdmin) {
    ContractTestingEthBridge ethBridge;
    id newAdmin = ETHBRIDGE_USER1;

    // Test setting new admin
    EXPECT_EQ(ethBridge.setAdmin(newAdmin), 0);
    auto output = ethBridge.getTotalReceivedTokens();
    ethBridge.getState()->adminChecker(newAdmin, newAdmin);
}

TEST(ContractEthBridge, AddAndRemoveManager) {
    ContractTestingEthBridge ethBridge;
    id manager = ETHBRIDGE_MANAGER1;

    // Add and verify manager
    EXPECT_EQ(ethBridge.addManager(manager), 0);
    std::vector<id> managers{ manager };
    ethBridge.getState()->managerChecker(managers, manager);

    // Remove and verify manager removal
    EXPECT_EQ(ethBridge.removeManager(manager), 0);
}

TEST(ContractEthBridge, AddManagerThatAlreadyExists) {
    ContractTestingEthBridge ethBridge;
    ethBridge.addManager(ETHBRIDGE_MANAGER1);
    EXPECT_NE(ethBridge.addManager(ETHBRIDGE_MANAGER1), 0);
}

TEST(ContractEthBridge, RemoveManagerDoesNotExist) {
    ContractTestingEthBridge ethBridge;
    EXPECT_NE(ethBridge.removeManager(ADDRESS2), 0);
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

TEST(ContractEthBridge, CreateOrderNoManager) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;
    uint64 amount = 1000;
    constexpr char memo[64] = "Bridge transfer details";
    uint32 sourceChain = 1;
    uint8 expectedStatus = 1;
    array<uint8, 32> expectedMessage{};

    EXPECT_NE(ethBridge.createOrder(ADDRESS2, amount, true), 0);
    auto orderOutput = ethBridge.getOrder(orderId);
    checker.orderChecker(orderOutput, orderId, ETHBRIDGE_USER1, ADDRESS2,
        amount, memo, sourceChain, expectedStatus, expectedMessage);
}

TEST(ContractEthBridge, CreateOrderWithZeroAmount) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;
    uint64 amount = 0;
    constexpr char memo[64] = "Bridge transfer details";
    uint32 sourceChain = 1;
    uint8 expectedStatus = 1;
    array<uint8, 32> expectedMessage{};

    EXPECT_NE(ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true), 0);
    auto orderOutput = ethBridge.getOrder(orderId);
    checker.orderChecker(orderOutput, orderId, ETHBRIDGE_USER1, ETHBRIDGE_MANAGER1,
        amount, memo, sourceChain, expectedStatus, expectedMessage);
}

TEST(ContractEthBridge, CompleteOrderSuccess) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;
    uint64 amount = 1000;
    uint64 initAmountLocked = ethBridge.getTotalLockedTokens();

    ethBridge.transferToContract(amount);
    ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true);
    EXPECT_EQ(ethBridge.completeOrder(orderId), 0);
    checker.totalTokensChecker(amount + initAmountLocked, ethBridge.getTotalLockedTokens());
}

TEST(ContractEthBridge, CompleteOrderInsufficientTokens) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;
    uint64 orderId = 0;
    uint64 amount = 1000;
    uint64 initAmountReceived = ethBridge.getTotalReceivedTokens();

    ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true);
    checker.totalTokensChecker(amount + initAmountReceived, ethBridge.getTotalReceivedTokens());
    EXPECT_NE(ethBridge.completeOrder(orderId), 0);
}

TEST(ContractEthBridge, CompleteOrderBalanceCheck) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;
    uint64 amount = 1000;
    uint64 initAmountLocked = ethBridge.getTotalLockedTokens();

    // Transfer tokens and create order
    ethBridge.transferToContract(amount);
    ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true);
    EXPECT_EQ(ethBridge.completeOrder(orderId), 0);

    checker.totalTokensChecker(amount + initAmountLocked, ethBridge.getTotalLockedTokens());
}


TEST(ContractEthBridge, RefundOrderSuccess) {
    ContractTestingEthBridge ethBridge;
    ETHBRIDGEChecker checker;

    uint64 orderId = 0;
    uint64 amount = 1000;
    uint64 initAmountLocked = ethBridge.getTotalLockedTokens();

    ethBridge.transferToContract(amount);
    ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true);
    EXPECT_EQ(ethBridge.completeOrder(orderId), 0); //Completed
    checker.totalTokensChecker(initAmountLocked + amount, ethBridge.getTotalLockedTokens());

    EXPECT_EQ(ethBridge.refundOrder(orderId), 0); //Refund
    checker.totalTokensChecker(initAmountLocked, ethBridge.getTotalLockedTokens());
}

TEST(ContractEthBridge, RefundOrderAlreadyRefunded) {
    ContractTestingEthBridge ethBridge;
    uint64 orderId = 0;
    uint64 amount = 1000;
  
    ethBridge.createOrder(ETHBRIDGE_MANAGER1, amount, true);
    ethBridge.transferToContract(amount);
    EXPECT_EQ(ethBridge.completeOrder(orderId), 0);

    EXPECT_EQ(ethBridge.refundOrder(orderId), 0); //first refund
    EXPECT_NE(ethBridge.refundOrder(orderId), 0); //second refund 
}

TEST(ContractEthBridge, RefundOrderNoManager) {
    ContractTestingEthBridge ethBridge;
    ethBridge.createOrder(ETHBRIDGE_MANAGER1, 500, true);
    EXPECT_FALSE(ethBridge.refundOrder(0), 0);
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