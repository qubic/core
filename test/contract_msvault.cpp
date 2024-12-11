#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"

static constexpr uint64 MSVAULT_REGISTERING_FEE = 10000ULL;
static constexpr uint64 MSVAULT_DEPOSIT_FEE = 0ULL;
static constexpr uint64 MSVAULT_RELEASE_FEE = 1000ULL;
static constexpr uint64 MSVAULT_RESET_FEE = 500ULL;

static const id OWNER1 = ID(_T, _K, _U, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
static const id OWNER2 = ID(_F, _X, _J, _F, _B, _T, _J, _M, _Y, _F, _J, _H, _P, _B, _X, _C, _D, _Q, _T, _L, _Y, _U, _K, _G, _M, _H, _B, _B, _Z, _A, _A, _F, _T, _I, _C, _W, _U, _K, _R, _B, _M, _E, _K, _Y, _N, _U, _P, _M, _R, _M, _B, _D, _N, _D, _R, _G);
static const id OWNER3 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
static const id TEST_VAULT_NAME = ID(_M, _Y, _M, _S, _V, _A, _U, _L, _U, _S, _E, _D, _F, _O, _R, _U, _N, _I, _T, _T, _T, _E, _S, _T, _I, _N, _G, _P, _U, _R, _P, _O, _S, _E, _S, _O, _N, _L, _Y, _U, _N, _I, _T, _T, _E, _S, _C, _O, _R, _E, _S, _M, _A, _R, _T, _T);


class ContractTestingMsVault : protected ContractTesting
{
public:
    ContractTestingMsVault()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(MSVAULT);
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, INITIALIZE);
    }

    MSVAULT::registerVault_output registerVault(uint16 vaultType, id vaultName, const std::vector<id>& owners, uint64 fee)
    {
        MSVAULT::registerVault_input input;
        MSVAULT::registerVault_output output;
        for (uint16 i = 0; i < MAX_OWNERS; i++)
        {
            input.owners.set(i, (i < owners.size()) ? owners[i] : NULL_ID);
        }
        input.vaultType = vaultType;
        input.vaultName = vaultName;

        // For simplicity, the first owner will be the one who invokes and pays the fee
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 1, input, output, owners[0], fee);
        return output;
    }

    MSVAULT::deposit_output deposit(uint64 vaultID, uint64 amount, const id& from)
    {
        MSVAULT::deposit_input input;
        MSVAULT::deposit_output output;
        input.vaultID = vaultID;

        // Increase energy for invocation
        increaseEnergy(from, amount);
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 2, input, output, from, amount);
        return output;
    }

    MSVAULT::releaseTo_output releaseTo(uint64 vaultID, uint64 amount, const id& destination, const id& owner, uint64 fee = MSVAULT_RELEASE_FEE)
    {
        MSVAULT::releaseTo_input input;
        MSVAULT::releaseTo_output output;
        input.vaultID = vaultID;
        input.amount = amount;
        input.destination = destination;

        increaseEnergy(owner, fee);
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 3, input, output, owner, fee);
        return output;
    }

    MSVAULT::resetRelease_output resetRelease(uint64 vaultID, const id& owner, uint64 fee = MSVAULT_RESET_FEE)
    {
        MSVAULT::resetRelease_input input;
        MSVAULT::resetRelease_output output;
        input.vaultID = vaultID;

        increaseEnergy(owner, fee);
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 4, input, output, owner, fee);
        return output;
    }

    MSVAULT::getVaultName_output getVaultName(uint64 vaultID) const
    {
        MSVAULT::getVaultName_input input;
        MSVAULT::getVaultName_output output;
        input.vaultID = vaultID;
        callFunction(MSVAULT_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    MSVAULT::getVaults_output getVaults(const id& pubKey) const
    {
        MSVAULT::getVaults_input input;
        MSVAULT::getVaults_output output;
        input.publicKey = pubKey;
        callFunction(MSVAULT_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    MSVAULT::getBalanceOf_output getBalanceOf(uint64 vaultID) const
    {
        MSVAULT::getBalanceOf_input input;
        MSVAULT::getBalanceOf_output output;
        input.vaultID = vaultID;
        callFunction(MSVAULT_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    MSVAULT::getReleaseStatus_output getReleaseStatus(uint64 vaultID) const
    {
        MSVAULT::getReleaseStatus_input input;
        MSVAULT::getReleaseStatus_output output;
        input.vaultID = vaultID;
        callFunction(MSVAULT_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    MSVAULT::getRevenueInfo_output getRevenueInfo() const
    {
        MSVAULT::getRevenueInfo_input input;
        MSVAULT::getRevenueInfo_output output;
        callFunction(MSVAULT_CONTRACT_INDEX, 9, input, output);
        return output;
    }
};


TEST(ContractMsVault, RegisterVault_InsufficientFee)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    auto out = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, 5000ULL); // less than REGISTERING_FEE
    EXPECT_EQ(out.vaultID, (uint64) - 1); // should fail
}

TEST(ContractMsVault, RegisterVault_OneOwner)
{
    ContractTestingMsVault msVault;
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    auto out = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1 }, MSVAULT_REGISTERING_FEE);
    EXPECT_EQ(out.vaultID, (uint64) - 1); // should fail because only one owner
}

TEST(ContractMsVault, RegisterVault_Success)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);

    auto out = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);
    EXPECT_NE(out.vaultID, (uint64) - 1);

    // Check vault name
    auto nameOut = msVault.getVaultName(out.vaultID);
    EXPECT_EQ(nameOut.vaultName, TEST_VAULT_NAME);

    // Check that owners see this vault
    auto vaultsO1 = msVault.getVaults(OWNER1);
    EXPECT_EQ(static_cast<unsigned int>(vaultsO1.numberOfVaults), 1);
    EXPECT_EQ(vaultsO1.vaultIDs.get(0), out.vaultID);

    // Check revenue info
    auto revenue = msVault.getRevenueInfo();
    EXPECT_EQ(revenue.numberOfActiveVaults, 1U);
    EXPECT_EQ(revenue.totalRevenue, MSVAULT_REGISTERING_FEE);
}

TEST(ContractMsVault, GetVaultName)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);

    auto reg = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    EXPECT_NE(reg.vaultID, (uint64) - 1);

    auto nameOut = msVault.getVaultName(reg.vaultID);

    EXPECT_EQ(nameOut.vaultName, TEST_VAULT_NAME);

    auto invalidNameOut = msVault.getVaultName(9999ULL);
    EXPECT_EQ(invalidNameOut.vaultName, NULL_ID);
}

TEST(ContractMsVault, Deposit_InvalidVault)
{
    ContractTestingMsVault msVault;

    auto out = msVault.deposit(999ULL, 5000ULL, OWNER1);
    EXPECT_FALSE(out.success);
}

TEST(ContractMsVault, Deposit_Success)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    auto reg = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    EXPECT_NE(reg.vaultID, (uint64) - 1);

    auto depositOut = msVault.deposit(reg.vaultID, 10000ULL, OWNER1);
    EXPECT_TRUE(depositOut.success);

    auto bal = msVault.getBalanceOf(reg.vaultID);
    EXPECT_EQ(bal.balance, 10000);
}

TEST(ContractMsVault, ReleaseTo_NonOwner)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    auto reg = msVault.registerVault(VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    EXPECT_NE(reg.vaultID, (uint64) - 1);

    msVault.deposit(reg.vaultID, 10000ULL, OWNER1);

    // Non-owner attempt release
    auto out = msVault.releaseTo(reg.vaultID, 5000ULL, OWNER3, OWNER3 /*invoking as a non-owner*/);
    EXPECT_FALSE(out.success);
}

TEST(ContractMsVault, ReleaseTo_InvalidParams)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    auto reg = msVault.registerVault(VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    msVault.deposit(reg.vaultID, 10000ULL, OWNER1);

    // amount=0
    auto out1 = msVault.releaseTo(reg.vaultID, 0ULL, OWNER2, OWNER1);
    EXPECT_FALSE(out1.success);

    // destination NULL_ID
    auto out2 = msVault.releaseTo(reg.vaultID, 5000ULL, NULL_ID, OWNER1);
    EXPECT_FALSE(out2.success);
}

TEST(ContractMsVault, ReleaseTo_PartialApproval)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);
    auto reg = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);
    msVault.deposit(reg.vaultID, 15000ULL, OWNER1);

    // First owner tries to release 5000 QUs to OWNER3
    auto out = msVault.releaseTo(reg.vaultID, 5000ULL, OWNER3, OWNER1);
    EXPECT_FALSE(out.success); // Not enough approvals yet

    // Check release status
    auto status = msVault.getReleaseStatus(reg.vaultID);
    EXPECT_EQ(status.amounts.get(0), 5000ULL);
    EXPECT_EQ(status.destinations.get(0), OWNER3);
    // others still 0 & NULL
}

TEST(ContractMsVault, ReleaseTo_FullApproval)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    auto reg = msVault.registerVault(VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);
    msVault.deposit(reg.vaultID, 10000ULL, OWNER1);

    // OWNER1 requests 5000 Qubics to OWNER3
    auto out1 = msVault.releaseTo(reg.vaultID, 5000ULL, OWNER3, OWNER1);
    EXPECT_FALSE(out1.success); // Need at least one more approval
    // OWNER2 approves the same request
    auto out2 = msVault.releaseTo(reg.vaultID, 5000ULL, OWNER3, OWNER2);
    EXPECT_TRUE(out2.success);  // Now it should release

    auto bal = msVault.getBalanceOf(reg.vaultID);
    EXPECT_EQ(bal.balance, (sint64)(10000ULL - 5000ULL));
}

TEST(ContractMsVault, ReleaseTo_InsufficientBalance)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    auto reg = msVault.registerVault(VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    msVault.deposit(reg.vaultID, 10000ULL, OWNER1);

    // Attempt to release more than balance
    auto out = msVault.releaseTo(reg.vaultID, 20000ULL, OWNER3, OWNER1);
    EXPECT_FALSE(out.success);
}

TEST(ContractMsVault, ResetRelease_NonOwner)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    auto reg = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    msVault.deposit(reg.vaultID, 5000ULL, OWNER1);

    // OWNER1 tries partial release
    msVault.releaseTo(reg.vaultID, 2000ULL, OWNER2, OWNER1);

    // Now a non-owner tries reset
    auto out = msVault.resetRelease(reg.vaultID, OWNER3);
    EXPECT_FALSE(out.success);
}

TEST(ContractMsVault, ResetRelease_Success)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    auto reg = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    msVault.deposit(reg.vaultID, 5000ULL, OWNER1);

    // OWNER2 requests a releaseTo
    msVault.releaseTo(reg.vaultID, 2000ULL, OWNER1, OWNER2);

    // Then OWNER2 resets the release requests
    auto out = msVault.resetRelease(reg.vaultID, OWNER2);
    EXPECT_TRUE(out.success);

    auto status = msVault.getReleaseStatus(reg.vaultID);
    for (uint16 i = 0; i < 3; i++)
    {
        EXPECT_EQ(status.amounts.get(i), 0ULL);
        EXPECT_EQ(status.destinations.get(i), NULL_ID);
    }
}

TEST(ContractMsVault, GetVaults_Multiple)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    auto v1 = msVault.registerVault(VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    auto v2 = msVault.registerVault(VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);
    EXPECT_NE(v1.vaultID, (uint64) - 1);
    EXPECT_NE(v2.vaultID, (uint64) - 1);

    auto vaultsForOwner2 = msVault.getVaults(OWNER2);
    EXPECT_GE(static_cast<unsigned int>(vaultsForOwner2.numberOfVaults), 2U); // Owner2 should be in both vaults

    // Check correctness
    bool foundV1 = false, foundV2 = false;
    for (uint16 i = 0; i < vaultsForOwner2.numberOfVaults; i++)
    {
        if (vaultsForOwner2.vaultIDs.get(i) == v1.vaultID) 
        {
            foundV1 = true;
        }
        if (vaultsForOwner2.vaultIDs.get(i) == v2.vaultID) 
        {
            foundV2 = true;
        }
    }
    EXPECT_TRUE(foundV1);
    EXPECT_TRUE(foundV2);
}
