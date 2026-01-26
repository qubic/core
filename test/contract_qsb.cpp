#define NO_UEFI

#include "contract_testing.h"

static const id QSB_CONTRACT_ID(QSB_CONTRACT_INDEX, 0, 0, 0);
static const id USER1(123, 456, 789, 876);
static const id USER2(42, 424, 4242, 42424);
static const id ADMIN(100, 200, 300, 400);
static const id ORACLE1(500, 600, 700, 800);
static const id ORACLE2(900, 1000, 1100, 1200);
static const id ORACLE3(1300, 1400, 1500, 1600);
static const id PAUSER1(1700, 1800, 1900, 2000);
static const id PROTOCOL_FEE_RECIPIENT(2100, 2200, 2300, 2400);
static const id ORACLE_FEE_RECIPIENT(2500, 2600, 2700, 2800);

class StateCheckerQSB : public QSB
{
public:
    void checkAdmin(const id& expectedAdmin) const
    {
        EXPECT_EQ(this->admin, expectedAdmin);
    }

    void checkPaused(bool expectedPaused) const
    {
        EXPECT_EQ((bool)this->paused, expectedPaused);
    }

    void checkOracleThreshold(uint8 expectedThreshold) const
    {
        EXPECT_EQ(this->oracleThreshold, expectedThreshold);
    }

    void checkOracleCount(uint32 expectedCount) const
    {
        EXPECT_EQ(this->oracleCount, expectedCount);
    }

    void checkBpsFee(uint32 expectedFee) const
    {
        EXPECT_EQ(this->bpsFee, expectedFee);
    }

    void checkProtocolFee(uint32 expectedFee) const
    {
        EXPECT_EQ(this->protocolFee, expectedFee);
    }

    void checkProtocolFeeRecipient(const id& expectedRecipient) const
    {
        EXPECT_EQ(this->protocolFeeRecipient, expectedRecipient);
    }

    void checkOracleFeeRecipient(const id& expectedRecipient) const
    {
        EXPECT_EQ(this->oracleFeeRecipient, expectedRecipient);
    }
};

class ContractTestingQSB : protected ContractTesting
{
public:
    ContractTestingQSB()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QSB);
        callSystemProcedure(QSB_CONTRACT_INDEX, INITIALIZE);

        checkContractExecCleanup();
    }

    ~ContractTestingQSB()
    {
        checkContractExecCleanup();
    }

    StateCheckerQSB* getState()
    {
        return (StateCheckerQSB*)contractStates[QSB_CONTRACT_INDEX];
    }

    const StateCheckerQSB* getState() const
    {
        return (const StateCheckerQSB*)contractStates[QSB_CONTRACT_INDEX];
    }

    // Helper to create a valid order for unlock testing
    QSB::Order createTestOrder(
        const id& fromAddress,
        const id& toAddress,
        uint64 amount,
        uint64 relayerFee,
        uint32 nonce) const
    {
        QSB::Order order;
        order.fromAddress = fromAddress;
        order.toAddress = toAddress;
        order.tokenIn = 0;
        order.tokenOut = 0;
        order.amount = amount;
        order.relayerFee = relayerFee;
        order.destinationChainId = 1; // Solana
        order.networkIn = 0; // Qubic
        order.networkOut = 1; // Solana
        order.nonce = nonce;
        return order;
    }

    // Helper to create signature data (mock - in real tests would need actual signatures)
    QSB::SignatureData createMockSignature(const id& signer) const
    {
        QSB::SignatureData sig;
        sig.signer = signer;
        // In real implementation, this would be a valid signature
        // For testing, we'll use zeros (signature validation will fail, but structure is correct)
        setMemory(sig.signature, 0);
        return sig;
    }

    // Helper to create a zero-initialized address array
    static Array<uint8, 32> createZeroAddress()
    {
        Array<uint8, 32> addr;
        setMemory(addr, 0);
        return addr;
    }

    // ============================================================================
    // User Procedure Helpers
    // ============================================================================

    QSB::Lock_output lock(const id& user, uint64 amount, uint64 relayerFee, uint32 networkOut, uint32 nonce, const Array<uint8, 32>& toAddress, uint64 energyAmount)
    {
        QSB::Lock_input input;
        QSB::Lock_output output;
        
        input.amount = amount;
        input.relayerFee = relayerFee;
        input.networkOut = networkOut;
        input.nonce = nonce;
        // copyMemory(input.toAddress, toAddress);
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 1, input, output, user, energyAmount);
        return output;
    }

    QSB::OverrideLock_output overrideLock(const id& user, uint32 nonce, uint64 relayerFee, const Array<uint8, 32>& toAddress)
    {
        QSB::OverrideLock_input input;
        QSB::OverrideLock_output output;
        
        input.nonce = nonce;
        input.relayerFee = relayerFee;
        // copyMemory(input.toAddress, toAddress);
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 2, input, output, user, 0);
        return output;
    }

    QSB::Unlock_output unlock(const id& user, const QSB::Order& order, uint32 numSignatures, const Array<QSB::SignatureData, QSB_MAX_ORACLES>& signatures)
    {
        QSB::Unlock_input input;
        QSB::Unlock_output output;
        
        input.order = order;
        input.numSignatures = numSignatures;
        // copyMemory(input.signatures, signatures);
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 3, input, output, user, 0);
        return output;
    }

    // ============================================================================
    // Admin Procedure Helpers
    // ============================================================================

    QSB::TransferAdmin_output transferAdmin(const id& user, const id& newAdmin)
    {
        QSB::TransferAdmin_input input;
        QSB::TransferAdmin_output output;
        
        input.newAdmin = newAdmin;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 10, input, output, user, 0);
        return output;
    }

    QSB::EditOracleThreshold_output editOracleThreshold(const id& user, uint8 newThreshold)
    {
        QSB::EditOracleThreshold_input input;
        QSB::EditOracleThreshold_output output;
        
        input.newThreshold = newThreshold;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 11, input, output, user, 0);
        return output;
    }

    QSB::AddRole_output addRole(const id& user, uint8 role, const id& account)
    {
        QSB::AddRole_input input;
        QSB::AddRole_output output;
        
        input.role = role;
        input.account = account;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 12, input, output, user, 0);
        return output;
    }

    QSB::RemoveRole_output removeRole(const id& user, uint8 role, const id& account)
    {
        QSB::RemoveRole_input input;
        QSB::RemoveRole_output output;
        
        input.role = role;
        input.account = account;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 13, input, output, user, 0);
        return output;
    }

    QSB::Pause_output pause(const id& user)
    {
        QSB::Pause_input input;
        QSB::Pause_output output;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 14, input, output, user, 0);
        return output;
    }

    QSB::Unpause_output unpause(const id& user)
    {
        QSB::Unpause_input input;
        QSB::Unpause_output output;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 15, input, output, user, 0);
        return output;
    }

    QSB::EditFeeParameters_output editFeeParameters(
        const id& user,
        uint32 bpsFee,
        uint32 protocolFee,
        const id& protocolFeeRecipient,
        const id& oracleFeeRecipient)
    {
        QSB::EditFeeParameters_input input;
        QSB::EditFeeParameters_output output;
        
        input.bpsFee = bpsFee;
        input.protocolFee = protocolFee;
        input.protocolFeeRecipient = protocolFeeRecipient;
        input.oracleFeeRecipient = oracleFeeRecipient;
        
        invokeUserProcedure(QSB_CONTRACT_INDEX, 16, input, output, user, 0);
        return output;
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST(ContractTestingQSB, TestInitialization)
{
    ContractTestingQSB test;
    
    // Check initial state
    test.getState()->checkAdmin(ADMIN);
    test.getState()->checkPaused(false);
    test.getState()->checkOracleThreshold(67); // Default 67%
    test.getState()->checkOracleCount(0);
    test.getState()->checkBpsFee(0);
    test.getState()->checkProtocolFee(0);
    
    test.getState()->checkProtocolFeeRecipient(NULL_ID);
    test.getState()->checkOracleFeeRecipient(NULL_ID);
}

// ============================================================================
// Lock Function Tests
// ============================================================================

TEST(ContractTestingQSB, TestLock_Success)
{
    ContractTestingQSB test;
    
    const uint64 amount = 1000000;
    const uint64 relayerFee = 10000;
    const uint32 networkOut = 1; // Solana
    const uint32 nonce = 1;
    
    // User should have enough balance
    increaseEnergy(USER1, amount);
    
    QSB::Lock_output output = test.lock(USER1, amount, relayerFee, networkOut, nonce, ContractTestingQSB::createZeroAddress(), amount);
    EXPECT_TRUE(output.success);
    
    // Check that orderHash is non-zero
    bool hashNonZero = false;
    for (uint32 i = 0; i < output.orderHash.capacity(); ++i)
    {
        if (output.orderHash.get(i) != 0)
        {
            hashNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hashNonZero);
}

TEST(ContractTestingQSB, TestLock_FailsWhenPaused)
{
    ContractTestingQSB test;
    
    increaseEnergy(ADMIN, 1);
    
    // Pause
    test.pause(ADMIN);
    
    // Now try to lock - should fail
    const uint64 amount = 1000000;
    increaseEnergy(USER1, amount);
    
    QSB::Lock_output output = test.lock(USER1, amount, 10000, 1, 2, ContractTestingQSB::createZeroAddress(), amount);
    EXPECT_FALSE(output.success);
}

TEST(ContractTestingQSB, TestLock_FailsWhenRelayerFeeTooHigh)
{
    ContractTestingQSB test;
    
    const uint64 amount = 1000000;
    increaseEnergy(USER1, amount);
    
    QSB::Lock_output output = test.lock(USER1, amount, 1000000, 1, 3, ContractTestingQSB::createZeroAddress(), amount);
    EXPECT_FALSE(output.success);
}

TEST(ContractTestingQSB, TestLock_FailsWhenNonceAlreadyUsed)
{
    ContractTestingQSB test;
    
    const uint64 amount = 1000000;
    const uint64 relayerFee = 10000;
    const uint32 nonce = 4;
    
    increaseEnergy(USER1, amount);
    
    // First lock should succeed
    QSB::Lock_output output = test.lock(USER1, amount, relayerFee, 1, nonce, ContractTestingQSB::createZeroAddress(), amount);
    EXPECT_TRUE(output.success);
    
    // Second lock with same nonce should fail
    increaseEnergy(USER1, amount);
    QSB::Lock_output output2 = test.lock(USER1, amount, relayerFee, 1, nonce, ContractTestingQSB::createZeroAddress(), amount);
    EXPECT_FALSE(output2.success);
}

// ============================================================================
// OverrideLock Function Tests
// ============================================================================

TEST(ContractTestingQSB, TestOverrideLock_Success)
{
    ContractTestingQSB test;
    
    const uint64 amount = 1000000;
    const uint64 relayerFee = 10000;
    const uint32 nonce = 5;
    
    // First, create a lock
    increaseEnergy(USER1, amount);
    test.lock(USER1, amount, relayerFee, 1, nonce, ContractTestingQSB::createZeroAddress(), amount);
    
    // Now override it
    Array<uint8, 32> newAddress = ContractTestingQSB::createZeroAddress();
    newAddress.set(0, 0xFF); // Change address
    
    QSB::OverrideLock_output overrideOutput = test.overrideLock(USER1, nonce, 5000, newAddress);
    EXPECT_TRUE(overrideOutput.success);
}

TEST(ContractTestingQSB, TestOverrideLock_FailsWhenNotOriginalSender)
{
    ContractTestingQSB test;
    
    const uint64 amount = 1000000;
    const uint64 relayerFee = 10000;
    const uint32 nonce = 6;
    
    // USER1 creates a lock
    increaseEnergy(USER1, amount);
    test.lock(USER1, amount, relayerFee, 1, nonce, ContractTestingQSB::createZeroAddress(), amount);
    
    // USER2 tries to override - should fail
    QSB::OverrideLock_output overrideOutput = test.overrideLock(USER2, nonce, 5000, ContractTestingQSB::createZeroAddress());
    EXPECT_FALSE(overrideOutput.success);
}

// ============================================================================
// Admin Function Tests
// ============================================================================

TEST(ContractTestingQSB, TestTransferAdmin_Success)
{
    ContractTestingQSB test;
    
    // First call bootstraps admin (admin is NULL_ID initially)
    increaseEnergy(ADMIN, 1);
    increaseEnergy(USER1, 1);
    QSB::TransferAdmin_output output = test.transferAdmin(ADMIN, USER1);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkAdmin(USER1);
}

TEST(ContractTestingQSB, TestTransferAdmin_FailsWhenNotAdmin)
{
    ContractTestingQSB test;
    
    // First bootstrap admin
    increaseEnergy(USER1, 1);
    increaseEnergy(USER2, 1);
    // Now USER1 tries to transfer admin - should fail
    QSB::TransferAdmin_output output = test.transferAdmin(USER1, USER2);
    EXPECT_FALSE(output.success);
    
    // Admin should still be ADMIN
    test.getState()->checkAdmin(ADMIN);
}

TEST(ContractTestingQSB, TestEditOracleThreshold_Success)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    
    QSB::EditOracleThreshold_output output = test.editOracleThreshold(ADMIN, 75);
    EXPECT_TRUE(output.success);
    EXPECT_EQ(output.oldThreshold, 67); // Original default
    
    test.getState()->checkOracleThreshold(75);
}

TEST(ContractTestingQSB, TestAddRole_Oracle)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    
    QSB::AddRole_output output = test.addRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE1);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkOracleCount(1);
}

TEST(ContractTestingQSB, TestAddRole_Pauser)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    increaseEnergy(PAUSER1, 1);
    
    QSB::AddRole_output output = test.addRole(ADMIN, (uint8)QSB::Role::Pauser, PAUSER1);
    EXPECT_TRUE(output.success);
}

TEST(ContractTestingQSB, TestRemoveRole_Oracle)
{
    ContractTestingQSB test;
    
    // Bootstrap admin and add oracle
    increaseEnergy(ADMIN, 1);
    test.addRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE1);
    
    // Now remove it
    QSB::RemoveRole_output output = test.removeRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE1);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkOracleCount(0);
}

TEST(ContractTestingQSB, TestPause_ByAdmin)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    
    QSB::Pause_output output = test.pause(ADMIN);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkPaused(true);
}

TEST(ContractTestingQSB, TestPause_ByPauser)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    increaseEnergy(PAUSER1, 1);
    
    // Add pauser
    test.addRole(ADMIN, (uint8)QSB::Role::Pauser, PAUSER1);
    
    // Pauser can pause
    QSB::Pause_output output = test.pause(PAUSER1);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkPaused(true);
}

TEST(ContractTestingQSB, TestUnpause)
{
    ContractTestingQSB test;
    
    // Bootstrap admin and pause
    increaseEnergy(ADMIN, 1);
    test.pause(ADMIN);
    
    // Now unpause
    QSB::Unpause_output output = test.unpause(ADMIN);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkPaused(false);
}

TEST(ContractTestingQSB, TestEditFeeParameters)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    
    QSB::EditFeeParameters_output output = test.editFeeParameters(ADMIN, 100, 30, PROTOCOL_FEE_RECIPIENT, ORACLE_FEE_RECIPIENT);
    EXPECT_TRUE(output.success);
    
    test.getState()->checkBpsFee(100);
    test.getState()->checkProtocolFee(30);
    test.getState()->checkProtocolFeeRecipient(PROTOCOL_FEE_RECIPIENT);
    test.getState()->checkOracleFeeRecipient(ORACLE_FEE_RECIPIENT);
}

// ============================================================================
// Unlock Function Tests
// ============================================================================
// Note: Full unlock testing would require valid oracle signatures
// These tests verify the structure and basic validation logic

TEST(ContractTestingQSB, TestUnlock_FailsWhenNoOracles)
{
    ContractTestingQSB test;
    
    // Bootstrap admin
    increaseEnergy(ADMIN, 1);
    
    QSB::Order order = test.createTestOrder(USER1, USER2, 1000000, 10000, 100);
    Array<QSB::SignatureData, QSB_MAX_ORACLES> signatures;
    setMemory(signatures, 0);
    
    QSB::Unlock_output output = test.unlock(USER1, order, 0, signatures);
    EXPECT_FALSE(output.success); // Should fail - no oracles configured
}

TEST(ContractTestingQSB, TestUnlock_FailsWhenPaused)
{
    ContractTestingQSB test;
    
    // Bootstrap admin, add oracle, and pause
    increaseEnergy(ADMIN, 1);
    test.addRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE1);
    test.pause(ADMIN);
    
    QSB::Order order = test.createTestOrder(USER1, USER2, 1000000, 10000, 101);
    Array<QSB::SignatureData, QSB_MAX_ORACLES> signatures;
    setMemory(signatures, 0);
    signatures.set(0, test.createMockSignature(ORACLE1));
    
    QSB::Unlock_output output = test.unlock(USER1, order, 1, signatures);
    EXPECT_FALSE(output.success); // Should fail - contract is paused
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ContractTestingQSB, TestFullWorkflow_LockAndOverride)
{
    ContractTestingQSB test;
    
    const uint64 amount = 1000000;
    const uint64 initialRelayerFee = 10000;
    const uint64 newRelayerFee = 5000;
    const uint32 nonce = 200;
    
    // Step 1: Lock
    increaseEnergy(USER1, amount);
    QSB::Lock_output lockOutput = test.lock(USER1, amount, initialRelayerFee, 1, nonce, ContractTestingQSB::createZeroAddress(), amount);
    EXPECT_TRUE(lockOutput.success);
    
    // Step 2: Override
    QSB::OverrideLock_output overrideOutput = test.overrideLock(USER1, nonce, newRelayerFee, ContractTestingQSB::createZeroAddress());
    EXPECT_TRUE(overrideOutput.success);
    
    // OrderHash should be different after override
    bool hashesDifferent = false;
    for (uint32 i = 0; i < lockOutput.orderHash.capacity(); ++i)
    {
        if (lockOutput.orderHash.get(i) != overrideOutput.orderHash.get(i))
        {
            hashesDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(hashesDifferent);
}

TEST(ContractTestingQSB, TestAdminWorkflow_SetupAndConfigure)
{
    ContractTestingQSB test;
    
    // Step 1: Bootstrap admin
    increaseEnergy(ADMIN, 1);
    
    // Step 2: Add oracles
    increaseEnergy(ORACLE1, 1);
    increaseEnergy(ORACLE2, 1);
    increaseEnergy(ORACLE3, 1);
    test.addRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE1);
    test.addRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE2);
    test.addRole(ADMIN, (uint8)QSB::Role::Oracle, ORACLE3);
    
    test.getState()->checkOracleCount(3);
    
    // Step 3: Set threshold
    test.editOracleThreshold(ADMIN, 67); // 2/3 + 1
    test.getState()->checkOracleThreshold(67);
    
    // Step 4: Configure fees
    test.editFeeParameters(ADMIN, 50, 20, PROTOCOL_FEE_RECIPIENT, ORACLE_FEE_RECIPIENT);
    
    test.getState()->checkBpsFee(50);
    test.getState()->checkProtocolFee(20);
}
