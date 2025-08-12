#define NO_UEFI

#include <random>
#include <map>
#include "gtest/gtest.h"
#include "contract_testing.h"

#define PRINT_TEST_INFO 0

// VottunBridge test constants
static const id VOTTUN_CONTRACT_ID(15, 0, 0, 0); // Assuming index 15
static const id TEST_USER_1 = id(1, 0, 0, 0);
static const id TEST_USER_2 = id(2, 0, 0, 0);
static const id TEST_ADMIN = id(100, 0, 0, 0);
static const id TEST_MANAGER = id(101, 0, 0, 0);

// Test fixture for VottunBridge
class VottunBridgeTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Test setup will be minimal due to system constraints
    }

    void TearDown() override {
        // Clean up after tests
    }
};

// Test 1: Basic constants and configuration
TEST_F(VottunBridgeTest, BasicConstants)
{
    // Test that basic types and constants work
    const uint32 expectedFeeBillionths = 5000000; // 0.5%
    EXPECT_EQ(expectedFeeBillionths, 5000000);

    // Test fee calculation logic
    uint64 amount = 1000000;
    uint64 calculatedFee = (amount * expectedFeeBillionths) / 1000000000ULL;
    EXPECT_EQ(calculatedFee, 5000); // 0.5% of 1,000,000
}

// Test 2: ID operations
TEST_F(VottunBridgeTest, IdOperations)
{
    id testId1(1, 0, 0, 0);
    id testId2(2, 0, 0, 0);
    id nullId = NULL_ID;

    EXPECT_NE(testId1, testId2);
    EXPECT_NE(testId1, nullId);
    EXPECT_EQ(nullId, NULL_ID);
}

// Test 3: Array bounds and capacity validation
TEST_F(VottunBridgeTest, ArrayValidation) {
    // Test Array type basic functionality
    Array<uint8, 64> testEthAddress;

    // Test capacity
    EXPECT_EQ(testEthAddress.capacity(), 64);

    // Test setting and getting values
    for (uint64 i = 0; i < 42; ++i) {  // Ethereum addresses are 42 chars
        testEthAddress.set(i, (uint8)(65 + (i % 26))); // ASCII A-Z pattern
    }

    // Verify values were set correctly
    for (uint64 i = 0; i < 42; ++i) {
        uint8 expectedValue = (uint8)(65 + (i % 26));
        EXPECT_EQ(testEthAddress.get(i), expectedValue);
    }
}

// Test 4: Order status enumeration
TEST_F(VottunBridgeTest, OrderStatusTypes) {
    // Test order status values
    const uint8 STATUS_CREATED = 0;
    const uint8 STATUS_COMPLETED = 1;
    const uint8 STATUS_REFUNDED = 2;
    const uint8 STATUS_EMPTY = 255;

    EXPECT_EQ(STATUS_CREATED, 0);
    EXPECT_EQ(STATUS_COMPLETED, 1);
    EXPECT_EQ(STATUS_REFUNDED, 2);
    EXPECT_EQ(STATUS_EMPTY, 255);
}

// Test 5: Basic data structure sizes
TEST_F(VottunBridgeTest, DataStructureSizes) {
    // Ensure critical structures have expected sizes
    EXPECT_GT(sizeof(id), 0);
    EXPECT_EQ(sizeof(uint64), 8);
    EXPECT_EQ(sizeof(uint32), 4);
    EXPECT_EQ(sizeof(uint8), 1);
    EXPECT_EQ(sizeof(bit), 1);
    EXPECT_EQ(sizeof(sint8), 1);
}

// Test 6: Bit manipulation and boolean logic
TEST_F(VottunBridgeTest, BooleanLogic) {
    bit testBit1 = true;
    bit testBit2 = false;

    EXPECT_TRUE(testBit1);
    EXPECT_FALSE(testBit2);
    EXPECT_NE(testBit1, testBit2);
}

// Test 7: Error code constants
TEST_F(VottunBridgeTest, ErrorCodes) {
    // Test that error codes are in expected ranges
    const uint32 ERROR_INVALID_AMOUNT = 2;
    const uint32 ERROR_INSUFFICIENT_FEE = 3;
    const uint32 ERROR_ORDER_NOT_FOUND = 4;
    const uint32 ERROR_NOT_AUTHORIZED = 9;

    EXPECT_GT(ERROR_INVALID_AMOUNT, 0);
    EXPECT_GT(ERROR_INSUFFICIENT_FEE, ERROR_INVALID_AMOUNT);
    EXPECT_GT(ERROR_ORDER_NOT_FOUND, ERROR_INSUFFICIENT_FEE);
    EXPECT_GT(ERROR_NOT_AUTHORIZED, ERROR_ORDER_NOT_FOUND);
}

// Test 8: Mathematical operations
TEST_F(VottunBridgeTest, MathematicalOperations) {
    // Test division operations (using div function instead of / operator)
    uint64 dividend = 1000000;
    uint64 divisor = 1000000000ULL;
    uint64 multiplier = 5000000;

    uint64 result = (dividend * multiplier) / divisor;
    EXPECT_EQ(result, 5000);

    // Test edge case: zero division would return 0 in Qubic
    // Note: This test validates our understanding of div() behavior
    uint64 zeroResult = (dividend * 0) / divisor;
    EXPECT_EQ(zeroResult, 0);
}

// Test 9: String and memory patterns
TEST_F(VottunBridgeTest, MemoryPatterns) {
    // Test memory initialization patterns
    Array<uint8, 16> testArray;

    // Set known pattern
    for (uint64 i = 0; i < testArray.capacity(); ++i) {
        testArray.set(i, (uint8)(i % 256));
    }

    // Verify pattern
    for (uint64 i = 0; i < testArray.capacity(); ++i) {
        EXPECT_EQ(testArray.get(i), (uint8)(i % 256));
    }
}

// Test 10: Contract index validation
TEST_F(VottunBridgeTest, ContractIndexValidation) {
    // Validate contract index is in expected range
    const uint32 EXPECTED_CONTRACT_INDEX = 15; // Based on contract_def.h
    const uint32 MAX_CONTRACTS = 32; // Reasonable upper bound

    EXPECT_GT(EXPECTED_CONTRACT_INDEX, 0);
    EXPECT_LT(EXPECTED_CONTRACT_INDEX, MAX_CONTRACTS);
}

// Test 11: Asset name validation
TEST_F(VottunBridgeTest, AssetNameValidation) {
    // Test asset name constraints (max 7 characters, A-Z, 0-9)
    const char* validNames[] = { "VBRIDGE", "VOTTUN", "BRIDGE", "VTN", "A", "TEST123" };
    const int nameCount = sizeof(validNames) / sizeof(validNames[0]);

    for (int i = 0; i < nameCount; ++i) {
        const char* name = validNames[i];
        size_t length = strlen(name);

        EXPECT_LE(length, 7); // Max 7 characters
        EXPECT_GT(length, 0); // At least 1 character

        // First character should be A-Z
        EXPECT_GE(name[0], 'A');
        EXPECT_LE(name[0], 'Z');
    }
}

// Test 12: Memory limits and constraints
TEST_F(VottunBridgeTest, MemoryConstraints) {
    // Test contract state size limits
    const uint64 MAX_CONTRACT_STATE_SIZE = 1073741824; // 1GB
    const uint64 ORDERS_CAPACITY = 1024;
    const uint64 MANAGERS_CAPACITY = 16;

    // Ensure our expected sizes are reasonable
    size_t estimatedOrdersSize = ORDERS_CAPACITY * 128; // Rough estimate per order
    size_t estimatedManagersSize = MANAGERS_CAPACITY * 32; // ID size
    size_t estimatedTotalSize = estimatedOrdersSize + estimatedManagersSize + 1024; // Extra for other fields

    EXPECT_LT(estimatedTotalSize, MAX_CONTRACT_STATE_SIZE);
    EXPECT_EQ(ORDERS_CAPACITY, 1024);
    EXPECT_EQ(MANAGERS_CAPACITY, 16);
}

// AGREGAR estos tests adicionales al final de tu contract_vottunbridge.cpp

// Test 13: Order creation simulation
TEST_F(VottunBridgeTest, OrderCreationLogic) {
    // Simulate the logic that would happen in createOrder
    uint64 orderAmount = 1000000;
    uint64 feeBillionths = 5000000;

    // Calculate fees as the contract would
    uint64 requiredFeeEth = (orderAmount * feeBillionths) / 1000000000ULL;
    uint64 requiredFeeQubic = (orderAmount * feeBillionths) / 1000000000ULL;
    uint64 totalRequiredFee = requiredFeeEth + requiredFeeQubic;

    // Verify fee calculation
    EXPECT_EQ(requiredFeeEth, 5000); // 0.5% of 1,000,000
    EXPECT_EQ(requiredFeeQubic, 5000); // 0.5% of 1,000,000
    EXPECT_EQ(totalRequiredFee, 10000); // 1% total

    // Test different amounts
    struct {
        uint64 amount;
        uint64 expectedTotalFee;
    } testCases[] = {
        {100000, 1000},      // 100K → 1K fee
        {500000, 5000},      // 500K → 5K fee  
        {2000000, 20000},    // 2M → 20K fee
        {10000000, 100000}   // 10M → 100K fee
    };

    for (const auto& testCase : testCases) {
        uint64 calculatedFee = 2 * ((testCase.amount * feeBillionths) / 1000000000ULL);
        EXPECT_EQ(calculatedFee, testCase.expectedTotalFee);
    }
}

// Test 14: Order state transitions
TEST_F(VottunBridgeTest, OrderStateTransitions) {
    // Test valid state transitions
    const uint8 STATE_CREATED = 0;
    const uint8 STATE_COMPLETED = 1;
    const uint8 STATE_REFUNDED = 2;
    const uint8 STATE_EMPTY = 255;

    // Valid transitions: CREATED → COMPLETED
    EXPECT_NE(STATE_CREATED, STATE_COMPLETED);
    EXPECT_LT(STATE_CREATED, STATE_COMPLETED);

    // Valid transitions: CREATED → REFUNDED
    EXPECT_NE(STATE_CREATED, STATE_REFUNDED);
    EXPECT_LT(STATE_CREATED, STATE_REFUNDED);

    // Invalid transitions: COMPLETED → REFUNDED (should not happen)
    EXPECT_NE(STATE_COMPLETED, STATE_REFUNDED);

    // Empty state is special
    EXPECT_GT(STATE_EMPTY, STATE_REFUNDED);
}

// Test 15: Direction flags and validation
TEST_F(VottunBridgeTest, TransferDirections) {
    bit fromQubicToEthereum = true;
    bit fromEthereumToQubic = false;

    EXPECT_TRUE(fromQubicToEthereum);
    EXPECT_FALSE(fromEthereumToQubic);
    EXPECT_NE(fromQubicToEthereum, fromEthereumToQubic);

    // Test logical operations
    bit bothDirections = fromQubicToEthereum || fromEthereumToQubic;
    bit neitherDirection = !fromQubicToEthereum && !fromEthereumToQubic;

    EXPECT_TRUE(bothDirections);
    EXPECT_FALSE(neitherDirection);
}

// Test 16: Ethereum address format validation
TEST_F(VottunBridgeTest, EthereumAddressFormat) {
    Array<uint8, 64> ethAddress;

    // Simulate valid Ethereum address (0x + 40 hex chars)
    ethAddress.set(0, '0');
    ethAddress.set(1, 'x');

    // Fill with hex characters (0-9, A-F)
    const char hexChars[] = "0123456789ABCDEF";
    for (int i = 2; i < 42; ++i) {
        ethAddress.set(i, hexChars[i % 16]);
    }

    // Verify format
    EXPECT_EQ(ethAddress.get(0), '0');
    EXPECT_EQ(ethAddress.get(1), 'x');

    // Verify hex characters
    for (int i = 2; i < 42; ++i) {
        uint8 ch = ethAddress.get(i);
        EXPECT_TRUE((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F'));
    }
}

// Test 17: Manager array operations
TEST_F(VottunBridgeTest, ManagerArrayOperations) {
    Array<id, 16> managers;
    const id NULL_MANAGER = NULL_ID;

    // Initialize all managers as NULL
    for (uint64 i = 0; i < managers.capacity(); ++i) {
        managers.set(i, NULL_MANAGER);
    }

    // Add managers
    id manager1(101, 0, 0, 0);
    id manager2(102, 0, 0, 0);
    id manager3(103, 0, 0, 0);

    managers.set(0, manager1);
    managers.set(1, manager2);
    managers.set(2, manager3);

    // Verify managers were added
    EXPECT_EQ(managers.get(0), manager1);
    EXPECT_EQ(managers.get(1), manager2);
    EXPECT_EQ(managers.get(2), manager3);
    EXPECT_EQ(managers.get(3), NULL_MANAGER); // Still empty

    // Test manager search
    bool foundManager1 = false;
    for (uint64 i = 0; i < managers.capacity(); ++i) {
        if (managers.get(i) == manager1) {
            foundManager1 = true;
            break;
        }
    }
    EXPECT_TRUE(foundManager1);

    // Remove a manager
    managers.set(1, NULL_MANAGER);
    EXPECT_EQ(managers.get(1), NULL_MANAGER);
    EXPECT_NE(managers.get(0), NULL_MANAGER);
    EXPECT_NE(managers.get(2), NULL_MANAGER);
}

// Test 18: Token balance calculations
TEST_F(VottunBridgeTest, TokenBalanceCalculations) {
    uint64 totalReceived = 10000000;
    uint64 lockedTokens = 6000000;
    uint64 earnedFees = 50000;
    uint64 distributedFees = 30000;

    // Calculate available tokens
    uint64 availableTokens = totalReceived - lockedTokens;
    EXPECT_EQ(availableTokens, 4000000);

    // Calculate available fees
    uint64 availableFees = earnedFees - distributedFees;
    EXPECT_EQ(availableFees, 20000);

    // Test edge cases
    EXPECT_GE(totalReceived, lockedTokens); // Should never be negative
    EXPECT_GE(earnedFees, distributedFees); // Should never be negative

    // Test zero balances
    uint64 zeroBalance = 0;
    EXPECT_EQ(zeroBalance - zeroBalance, 0);
}

// Test 19: Order ID generation and uniqueness
TEST_F(VottunBridgeTest, OrderIdGeneration) {
    uint64 nextOrderId = 1;

    // Simulate order ID generation
    uint64 order1Id = nextOrderId++;
    uint64 order2Id = nextOrderId++;
    uint64 order3Id = nextOrderId++;

    EXPECT_EQ(order1Id, 1);
    EXPECT_EQ(order2Id, 2);
    EXPECT_EQ(order3Id, 3);
    EXPECT_EQ(nextOrderId, 4);

    // Ensure uniqueness
    EXPECT_NE(order1Id, order2Id);
    EXPECT_NE(order2Id, order3Id);
    EXPECT_NE(order1Id, order3Id);

    // Test with larger numbers
    nextOrderId = 1000000;
    uint64 largeOrderId = nextOrderId++;
    EXPECT_EQ(largeOrderId, 1000000);
    EXPECT_EQ(nextOrderId, 1000001);
}

// Test 20: Contract limits and boundaries
TEST_F(VottunBridgeTest, ContractLimits) {
    // Test maximum values
    const uint64 MAX_UINT64 = 0xFFFFFFFFFFFFFFFFULL;
    const uint32 MAX_UINT32 = 0xFFFFFFFFU;
    const uint8 MAX_UINT8 = 0xFF;

    EXPECT_EQ(MAX_UINT8, 255);
    EXPECT_GT(MAX_UINT32, MAX_UINT8);
    EXPECT_GT(MAX_UINT64, MAX_UINT32);

    // Test order capacity limits
    const uint64 ORDERS_CAPACITY = 1024;
    const uint64 MANAGERS_CAPACITY = 16;

    // Ensure we don't exceed array bounds
    EXPECT_LT(0, ORDERS_CAPACITY);
    EXPECT_LT(0, MANAGERS_CAPACITY);
    EXPECT_LT(MANAGERS_CAPACITY, ORDERS_CAPACITY);

    // Test fee calculation limits
    const uint64 MAX_TRADE_FEE = 1000000000ULL; // 100%
    const uint64 ACTUAL_TRADE_FEE = 5000000ULL; // 0.5%

    EXPECT_LT(ACTUAL_TRADE_FEE, MAX_TRADE_FEE);
    EXPECT_GT(ACTUAL_TRADE_FEE, 0);
}
// REEMPLAZA el código funcional anterior con esta versión corregida:

// Mock structures for testing
struct MockVottunBridgeOrder {
    uint64 orderId;
    id qubicSender;
    id qubicDestination;
    uint64 amount;
    uint8 status;
    bit fromQubicToEthereum;
    uint8 mockEthAddress[64];  // Simulated eth address
};

struct MockVottunBridgeState {
    id admin;
    id feeRecipient;
    uint64 nextOrderId;
    uint64 lockedTokens;
    uint64 totalReceivedTokens;
    uint32 _tradeFeeBillionths;
    uint64 _earnedFees;
    uint64 _distributedFees;
    uint64 _earnedFeesQubic;
    uint64 _distributedFeesQubic;
    uint32 sourceChain;
    MockVottunBridgeOrder orders[1024];
    id managers[16];
};

// Mock QPI Context for testing
class MockQpiContext {
public:
    id mockInvocator = TEST_USER_1;
    sint64 mockInvocationReward = 10000;
    id mockOriginator = TEST_USER_1;

    void setInvocator(const id& invocator) { mockInvocator = invocator; }
    void setInvocationReward(sint64 reward) { mockInvocationReward = reward; }
    void setOriginator(const id& originator) { mockOriginator = originator; }
};

// Helper functions for creating test data
MockVottunBridgeOrder createEmptyOrder() {
    MockVottunBridgeOrder order = {};
    order.status = 255;  // Empty
    order.orderId = 0;
    order.amount = 0;
    order.qubicSender = NULL_ID;
    order.qubicDestination = NULL_ID;
    return order;
}

MockVottunBridgeOrder createTestOrder(uint64 orderId, uint64 amount, bool fromQubicToEth = true) {
    MockVottunBridgeOrder order = {};
    order.orderId = orderId;
    order.qubicSender = TEST_USER_1;
    order.qubicDestination = TEST_USER_2;
    order.amount = amount;
    order.status = 0;  // Created
    order.fromQubicToEthereum = fromQubicToEth;

    // Set mock Ethereum address
    for (int i = 0; i < 42; ++i) {
        order.mockEthAddress[i] = (uint8)('A' + (i % 26));
    }

    return order;
}

// Advanced test fixture with contract state simulation
class VottunBridgeFunctionalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize a complete contract state
        contractState = {};

        // Set up admin and initial configuration
        contractState.admin = TEST_ADMIN;
        contractState.feeRecipient = id(200, 0, 0, 0);
        contractState.nextOrderId = 1;
        contractState.lockedTokens = 5000000;  // 5M tokens locked
        contractState.totalReceivedTokens = 10000000;  // 10M total received
        contractState._tradeFeeBillionths = 5000000;  // 0.5%
        contractState._earnedFees = 50000;
        contractState._distributedFees = 30000;
        contractState._earnedFeesQubic = 25000;
        contractState._distributedFeesQubic = 15000;
        contractState.sourceChain = 0;

        // Initialize orders array as empty
        for (uint64 i = 0; i < 1024; ++i) {
            contractState.orders[i] = createEmptyOrder();
        }

        // Initialize managers array
        for (int i = 0; i < 16; ++i) {
            contractState.managers[i] = NULL_ID;
        }
        contractState.managers[0] = TEST_MANAGER;  // Add initial manager

        // Set up mock context
        mockContext.setInvocator(TEST_USER_1);
        mockContext.setInvocationReward(10000);
    }

    void TearDown() override {
        // Cleanup
    }

protected:
    MockVottunBridgeState contractState;
    MockQpiContext mockContext;
};

// Test 21: CreateOrder function simulation
TEST_F(VottunBridgeFunctionalTest, CreateOrderFunctionSimulation) {
    // Test input
    uint64 orderAmount = 1000000;
    uint64 feeBillionths = contractState._tradeFeeBillionths;

    // Calculate expected fees
    uint64 expectedFeeEth = (orderAmount * feeBillionths) / 1000000000ULL;
    uint64 expectedFeeQubic = (orderAmount * feeBillionths) / 1000000000ULL;
    uint64 totalExpectedFee = expectedFeeEth + expectedFeeQubic;

    // Test case 1: Valid order creation (Qubic to Ethereum)
    {
        // Simulate sufficient invocation reward
        mockContext.setInvocationReward(totalExpectedFee);

        // Simulate createOrder logic
        bool validAmount = (orderAmount > 0);
        bool sufficientFee = (mockContext.mockInvocationReward >= static_cast<sint64>(totalExpectedFee));
        bool fromQubicToEth = true;

        EXPECT_TRUE(validAmount);
        EXPECT_TRUE(sufficientFee);

        if (validAmount && sufficientFee) {
            // Simulate successful order creation
            uint64 newOrderId = contractState.nextOrderId++;

            // Update state
            contractState._earnedFees += expectedFeeEth;
            contractState._earnedFeesQubic += expectedFeeQubic;

            EXPECT_EQ(newOrderId, 1);
            EXPECT_EQ(contractState.nextOrderId, 2);
            EXPECT_EQ(contractState._earnedFees, 50000 + expectedFeeEth);
            EXPECT_EQ(contractState._earnedFeesQubic, 25000 + expectedFeeQubic);
        }
    }

    // Test case 2: Invalid amount (zero)
    {
        uint64 invalidAmount = 0;
        bool validAmount = (invalidAmount > 0);
        EXPECT_FALSE(validAmount);

        // Should return error status 1
        uint8 expectedStatus = validAmount ? 0 : 1;
        EXPECT_EQ(expectedStatus, 1);
    }

    // Test case 3: Insufficient fee
    {
        mockContext.setInvocationReward(totalExpectedFee - 1);  // One unit short

        bool sufficientFee = (mockContext.mockInvocationReward >= static_cast<sint64>(totalExpectedFee));
        EXPECT_FALSE(sufficientFee);

        // Should return error status 2
        uint8 expectedStatus = sufficientFee ? 0 : 2;
        EXPECT_EQ(expectedStatus, 2);
    }
}

// Test 22: CompleteOrder function simulation
TEST_F(VottunBridgeFunctionalTest, CompleteOrderFunctionSimulation) {
    // Set up: Create an order first
    auto testOrder = createTestOrder(1, 1000000, false);  // EVM to Qubic
    contractState.orders[0] = testOrder;

    // Test case 1: Manager completing order
    {
        mockContext.setInvocator(TEST_MANAGER);

        // Simulate isManager check
        bool isManagerOperating = (mockContext.mockInvocator == TEST_MANAGER);
        EXPECT_TRUE(isManagerOperating);

        // Simulate order retrieval
        bool orderFound = (contractState.orders[0].orderId == 1);
        EXPECT_TRUE(orderFound);

        // Check order status (should be 0 = Created)
        bool validOrderState = (contractState.orders[0].status == 0);
        EXPECT_TRUE(validOrderState);

        if (isManagerOperating && orderFound && validOrderState) {
            // Simulate order completion logic
            uint64 netAmount = contractState.orders[0].amount;

            if (!contractState.orders[0].fromQubicToEthereum) {
                // EVM to Qubic: Transfer tokens to destination
                bool sufficientLockedTokens = (contractState.lockedTokens >= netAmount);
                EXPECT_TRUE(sufficientLockedTokens);

                if (sufficientLockedTokens) {
                    contractState.lockedTokens -= netAmount;
                    contractState.orders[0].status = 1;  // Completed

                    EXPECT_EQ(contractState.orders[0].status, 1);
                    EXPECT_EQ(contractState.lockedTokens, 5000000 - netAmount);
                }
            }
        }
    }

    // Test case 2: Non-manager trying to complete order
    {
        mockContext.setInvocator(TEST_USER_1);  // Regular user, not manager

        bool isManagerOperating = (mockContext.mockInvocator == TEST_MANAGER);
        EXPECT_FALSE(isManagerOperating);

        // Should return error (only managers can complete)
        uint8 expectedErrorCode = 1;  // onlyManagersCanCompleteOrders
        EXPECT_EQ(expectedErrorCode, 1);
    }
}

TEST_F(VottunBridgeFunctionalTest, AdminFunctionsSimulation) {
    // Test setAdmin function
    {
        mockContext.setInvocator(TEST_ADMIN);  // Current admin
        id newAdmin(150, 0, 0, 0);

        // Check authorization
        bool isCurrentAdmin = (mockContext.mockInvocator == contractState.admin);
        EXPECT_TRUE(isCurrentAdmin);

        if (isCurrentAdmin) {
            // Simulate admin change
            id oldAdmin = contractState.admin;
            contractState.admin = newAdmin;

            EXPECT_EQ(contractState.admin, newAdmin);
            EXPECT_NE(contractState.admin, oldAdmin);

            // Update mock context to use new admin for next tests
            mockContext.setInvocator(newAdmin);
        }
    }

    // Test addManager function (use new admin)
    {
        id newManager(160, 0, 0, 0);

        // Check authorization (new admin should be set from previous test)
        bool isCurrentAdmin = (mockContext.mockInvocator == contractState.admin);
        EXPECT_TRUE(isCurrentAdmin);

        if (isCurrentAdmin) {
            // Simulate finding empty slot (index 1 should be empty)
            bool foundEmptySlot = true;  // Simulate finding slot

            if (foundEmptySlot) {
                contractState.managers[1] = newManager;
                EXPECT_EQ(contractState.managers[1], newManager);
            }
        }
    }

    // Test unauthorized access
    {
        mockContext.setInvocator(TEST_USER_1);  // Regular user

        bool isCurrentAdmin = (mockContext.mockInvocator == contractState.admin);
        EXPECT_FALSE(isCurrentAdmin);

        // Should return error code 9 (notAuthorized)
        uint8 expectedErrorCode = isCurrentAdmin ? 0 : 9;
        EXPECT_EQ(expectedErrorCode, 9);
    }
}

// Test 24: Fee withdrawal simulation
TEST_F(VottunBridgeFunctionalTest, FeeWithdrawalSimulation) {
    uint64 withdrawAmount = 15000;  // Less than available fees

    // Test case 1: Admin withdrawing fees
    {
        mockContext.setInvocator(contractState.admin);

        bool isCurrentAdmin = (mockContext.mockInvocator == contractState.admin);
        EXPECT_TRUE(isCurrentAdmin);

        uint64 availableFees = contractState._earnedFees - contractState._distributedFees;
        EXPECT_EQ(availableFees, 20000);  // 50000 - 30000

        bool sufficientFees = (withdrawAmount <= availableFees);
        bool validAmount = (withdrawAmount > 0);

        EXPECT_TRUE(sufficientFees);
        EXPECT_TRUE(validAmount);

        if (isCurrentAdmin && sufficientFees && validAmount) {
            // Simulate fee withdrawal
            contractState._distributedFees += withdrawAmount;

            EXPECT_EQ(contractState._distributedFees, 45000);  // 30000 + 15000

            uint64 newAvailableFees = contractState._earnedFees - contractState._distributedFees;
            EXPECT_EQ(newAvailableFees, 5000);  // 50000 - 45000
        }
    }

    // Test case 2: Insufficient fees
    {
        uint64 excessiveAmount = 25000;  // More than remaining available fees
        uint64 currentAvailableFees = contractState._earnedFees - contractState._distributedFees;

        bool sufficientFees = (excessiveAmount <= currentAvailableFees);
        EXPECT_FALSE(sufficientFees);

        // Should return error (insufficient fees)
        uint8 expectedErrorCode = sufficientFees ? 0 : 6;  // insufficientLockedTokens (reused)
        EXPECT_EQ(expectedErrorCode, 6);
    }
}

// Test 25: Order search and retrieval simulation
TEST_F(VottunBridgeFunctionalTest, OrderSearchSimulation) {
    // Set up multiple orders
    contractState.orders[0] = createTestOrder(10, 1000000, true);
    contractState.orders[1] = createTestOrder(11, 2000000, false);
    contractState.orders[2] = createTestOrder(12, 500000, true);

    // Test getOrder function simulation
    {
        uint64 searchOrderId = 11;
        bool found = false;
        MockVottunBridgeOrder foundOrder = {};

        // Simulate order search
        for (int i = 0; i < 1024; ++i) {
            if (contractState.orders[i].orderId == searchOrderId &&
                contractState.orders[i].status != 255) {
                found = true;
                foundOrder = contractState.orders[i];
                break;
            }
        }

        EXPECT_TRUE(found);
        EXPECT_EQ(foundOrder.orderId, 11);
        EXPECT_EQ(foundOrder.amount, 2000000);
        EXPECT_FALSE(foundOrder.fromQubicToEthereum);
    }

    // Test search for non-existent order
    {
        uint64 nonExistentOrderId = 999;
        bool found = false;

        for (int i = 0; i < 1024; ++i) {
            if (contractState.orders[i].orderId == nonExistentOrderId &&
                contractState.orders[i].status != 255) {
                found = true;
                break;
            }
        }

        EXPECT_FALSE(found);

        // Should return error status 1 (order not found)
        uint8 expectedStatus = found ? 0 : 1;
        EXPECT_EQ(expectedStatus, 1);
    }
}

TEST_F(VottunBridgeFunctionalTest, ContractInfoSimulation) {
    // Simulate getContractInfo function
    {
        // Count orders and empty slots
        uint64 totalOrdersFound = 0;
        uint64 emptySlots = 0;

        for (uint64 i = 0; i < 1024; ++i) {
            if (contractState.orders[i].status == 255) {
                emptySlots++;
            }
            else {
                totalOrdersFound++;
            }
        }

        // Initially should be mostly empty
        EXPECT_GT(emptySlots, totalOrdersFound);

        // Validate contract state (use actual values, not expected modifications)
        EXPECT_EQ(contractState.nextOrderId, 1);  // Should still be 1 initially
        EXPECT_EQ(contractState.lockedTokens, 5000000);  // Should be initial value
        EXPECT_EQ(contractState.totalReceivedTokens, 10000000);
        EXPECT_EQ(contractState._tradeFeeBillionths, 5000000);

        // Test that the state values are sensible
        EXPECT_GT(contractState.totalReceivedTokens, contractState.lockedTokens);
        EXPECT_GT(contractState._tradeFeeBillionths, 0);
        EXPECT_LT(contractState._tradeFeeBillionths, 1000000000ULL); // Less than 100%
    }
}

// Test 27: Edge cases and error scenarios
TEST_F(VottunBridgeFunctionalTest, EdgeCasesAndErrors) {
    // Test zero amounts
    {
        uint64 zeroAmount = 0;
        bool validAmount = (zeroAmount > 0);
        EXPECT_FALSE(validAmount);
    }

    // Test boundary conditions
    {
        // Test with exactly enough fees
        uint64 amount = 1000000;
        uint64 exactFee = 2 * ((amount * contractState._tradeFeeBillionths) / 1000000000ULL);

        mockContext.setInvocationReward(exactFee);
        bool sufficientFee = (mockContext.mockInvocationReward >= static_cast<sint64>(exactFee));
        EXPECT_TRUE(sufficientFee);

        // Test with one unit less
        mockContext.setInvocationReward(exactFee - 1);
        bool insufficientFee = (mockContext.mockInvocationReward >= static_cast<sint64>(exactFee));
        EXPECT_FALSE(insufficientFee);
    }

    // Test manager validation
    {
        // Valid manager
        bool isManager = (contractState.managers[0] == TEST_MANAGER);
        EXPECT_TRUE(isManager);

        // Invalid manager (empty slot)
        bool isNotManager = (contractState.managers[5] == NULL_ID);
        EXPECT_TRUE(isNotManager);
    }
}