#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"

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

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void registerVault(uint16 vaultType, id vaultName, const std::vector<id>& owners, uint64 fee)
    {
        MSVAULT::registerVault_input input;
        for (uint16 i = 0; i < MSVAULT_MAX_OWNERS; i++)
        {
            input.owners.set(i, (i < owners.size()) ? owners[i] : NULL_ID);
        }
        input.vaultType = vaultType;
        input.vaultName = vaultName;
        MSVAULT::registerVault_output regOut;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 1, input, regOut, owners[0], fee);
    }

    void deposit(uint64 vaultId, uint64 amount, const id& from)
    {
        MSVAULT::deposit_input input;
        input.vaultId = vaultId;
        increaseEnergy(from, amount);
        MSVAULT::deposit_output depOut;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 2, input, depOut, from, amount);
    }

    void releaseTo(uint64 vaultId, uint64 amount, const id& destination, const id& owner, uint64 fee = MSVAULT_RELEASE_FEE)
    {
        MSVAULT::releaseTo_input input;
        input.vaultId = vaultId;
        input.amount = amount;
        input.destination = destination;

        increaseEnergy(owner, fee);
        MSVAULT::releaseTo_output relOut;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 3, input, relOut, owner, fee);
    }

    void resetRelease(uint64 vaultId, const id& owner, uint64 fee = MSVAULT_RELEASE_RESET_FEE)
    {
        MSVAULT::resetRelease_input input;
        input.vaultId = vaultId;

        increaseEnergy(owner, fee);
        MSVAULT::resetRelease_output rstOut;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 4, input, rstOut, owner, fee);
    }

    MSVAULT::getVaultName_output getVaultName(uint64 vaultId) const
    {
        MSVAULT::getVaultName_input input;
        MSVAULT::getVaultName_output output;
        input.vaultId = vaultId;
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

    MSVAULT::getBalanceOf_output getBalanceOf(uint64 vaultId) const
    {
        MSVAULT::getBalanceOf_input input;
        MSVAULT::getBalanceOf_output output;
        input.vaultId = vaultId;
        callFunction(MSVAULT_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    MSVAULT::getReleaseStatus_output getReleaseStatus(uint64 vaultId) const
    {
        MSVAULT::getReleaseStatus_input input;
        MSVAULT::getReleaseStatus_output output;
        input.vaultId = vaultId;
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

    // Helper: find newly created vault by difference
    sint64 findNewvaultIdAfterRegister(const id& owner, uint16 prevCount)
    {
        auto vaultsAfter = getVaults(owner);
        if (vaultsAfter.numberOfVaults == prevCount + 1)
        {
            return (sint64)vaultsAfter.vaultIds.get(prevCount);
        }
        return -1;
    }
};

TEST(ContractMsVault, RegisterVault_InsufficientFee)
{
    ContractTestingMsVault msVault;

    // Check how many vaults OWNER1 has initially
    auto vaultsO1Before = msVault.getVaults(OWNER1);

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    // Attempt with insufficient fee
    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, 5000ULL);

    // No new vault should be created
    auto vaultsO1After = msVault.getVaults(OWNER1);
    EXPECT_EQ(static_cast<unsigned int>(vaultsO1After.numberOfVaults),
        static_cast<unsigned int>(vaultsO1Before.numberOfVaults));
}

TEST(ContractMsVault, RegisterVault_OneOwner)
{
    ContractTestingMsVault msVault;
    auto vaultsO1Before = msVault.getVaults(OWNER1);
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    // Only one owner
    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1 }, MSVAULT_REGISTERING_FEE);

    // Should fail, no new vault
    auto vaultsO1After = msVault.getVaults(OWNER1);
    EXPECT_EQ(static_cast<unsigned int>(vaultsO1After.numberOfVaults),
        static_cast<unsigned int>(vaultsO1Before.numberOfVaults));
}

TEST(ContractMsVault, RegisterVault_Success)
{
    ContractTestingMsVault msVault;
    auto vaultsO1Before = msVault.getVaults(OWNER1);

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1After = msVault.getVaults(OWNER1);
    EXPECT_EQ(static_cast<unsigned int>((unsigned)vaultsO1After.numberOfVaults),
        static_cast<unsigned int>((unsigned)(vaultsO1Before.numberOfVaults + 1)));

    // Extract the new vaultId
    uint64 vaultId = vaultsO1After.vaultIds.get(vaultsO1Before.numberOfVaults);
    // Check vault name
    auto nameOut = msVault.getVaultName(vaultId);
    EXPECT_EQ(nameOut.vaultName, TEST_VAULT_NAME);

    // Check revenue info
    auto revenue = msVault.getRevenueInfo();
    // At least one vault active, revenue should have increased
    EXPECT_GE(revenue.numberOfActiveVaults, 1U);
    EXPECT_GE(revenue.totalRevenue, MSVAULT_REGISTERING_FEE);

    auto balance = msVault.getBalanceOf(vaultId);
    EXPECT_EQ(balance.balance, 0U);
}

TEST(ContractMsVault, GetVaultName)
{
    ContractTestingMsVault msVault;

    auto vaultsO1Before = msVault.getVaults(OWNER1);
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1After = msVault.getVaults(OWNER1);
    EXPECT_EQ(static_cast<unsigned int>(vaultsO1After.numberOfVaults),
        static_cast<unsigned int>(vaultsO1Before.numberOfVaults + 1));
    uint64 vaultId = vaultsO1After.vaultIds.get(vaultsO1Before.numberOfVaults);

    auto nameOut = msVault.getVaultName(vaultId);
    EXPECT_EQ(nameOut.vaultName, TEST_VAULT_NAME);

    auto invalidNameOut = msVault.getVaultName(9999ULL);
    EXPECT_EQ(invalidNameOut.vaultName, NULL_ID);
}

TEST(ContractMsVault, Deposit_InvalidVault)
{
    ContractTestingMsVault msVault;
    // deposit to a non-existent vault
    auto beforeBalance = msVault.getBalanceOf(999ULL);
    msVault.deposit(999ULL, 5000ULL, OWNER1);
    // no change in balance
    auto afterBalance = msVault.getBalanceOf(999ULL);
    EXPECT_EQ(afterBalance.balance, beforeBalance.balance);
}

TEST(ContractMsVault, Deposit_Success)
{
    ContractTestingMsVault msVault;

    auto vaultsO1Before = msVault.getVaults(OWNER1);
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1After = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1After.vaultIds.get(vaultsO1Before.numberOfVaults);

    auto balBefore = msVault.getBalanceOf(vaultId);
    msVault.deposit(vaultId, 10000ULL, OWNER1);
    auto balAfter = msVault.getBalanceOf(vaultId);
    EXPECT_EQ(balAfter.balance, balBefore.balance + 10000ULL);
}

TEST(ContractMsVault, ReleaseTo_NonOwner)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(MSVAULT_VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 10000ULL, OWNER1);
    auto releaseStatusBefore = msVault.getReleaseStatus(vaultId);

    // Non-owner attempt release
    msVault.releaseTo(vaultId, 5000ULL, OWNER3, OWNER3);
    auto releaseStatusAfter = msVault.getReleaseStatus(vaultId);

    // No approvals should be set
    EXPECT_EQ(releaseStatusAfter.amounts.get(0), releaseStatusBefore.amounts.get(0));
    EXPECT_EQ(releaseStatusAfter.destinations.get(0), releaseStatusBefore.destinations.get(0));
}

TEST(ContractMsVault, ReleaseTo_InvalidParams)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(MSVAULT_VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 10000ULL, OWNER1);

    auto releaseStatusBefore = msVault.getReleaseStatus(vaultId);
    // amount=0
    msVault.releaseTo(vaultId, 0ULL, OWNER2, OWNER1);
    auto releaseStatusAfter1 = msVault.getReleaseStatus(vaultId);
    EXPECT_EQ(releaseStatusAfter1.amounts.get(0), releaseStatusBefore.amounts.get(0));

    // destination NULL_ID
    msVault.releaseTo(vaultId, 5000ULL, NULL_ID, OWNER1);
    auto releaseStatusAfter2 = msVault.getReleaseStatus(vaultId);
    EXPECT_EQ(releaseStatusAfter2.amounts.get(0), releaseStatusBefore.amounts.get(0));
}

TEST(ContractMsVault, ReleaseTo_PartialApproval)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);
    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 15000ULL, OWNER1);
    msVault.releaseTo(vaultId, 5000ULL, OWNER3, OWNER1);

    auto status = msVault.getReleaseStatus(vaultId);
    // Partial approval means just first owner sets the request
    EXPECT_EQ(status.amounts.get(0), 5000ULL);
    EXPECT_EQ(status.destinations.get(0), OWNER3);
}

TEST(ContractMsVault, ReleaseTo_FullApproval)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(MSVAULT_VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 10000ULL, OWNER1);

    // OWNER1 requests 5000 Qubics to OWNER3
    msVault.releaseTo(vaultId, 5000ULL, OWNER3, OWNER1);
    // Not approved yet

    msVault.releaseTo(vaultId, 5000ULL, OWNER3, OWNER2); // second approval

    // After full approval, amount should be released
    auto bal = msVault.getBalanceOf(vaultId);
    EXPECT_EQ(bal.balance, 10000ULL - 5000ULL);
}

TEST(ContractMsVault, ReleaseTo_InsufficientBalance)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(MSVAULT_VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 10000ULL, OWNER1);

    auto balBefore = msVault.getBalanceOf(vaultId);
    // Attempt to release more than balance
    msVault.releaseTo(vaultId, 20000ULL, OWNER3, OWNER1);

    // Should fail, balance no change
    auto balAfter = msVault.getBalanceOf(vaultId);
    EXPECT_EQ(balAfter.balance, balBefore.balance);
}

TEST(ContractMsVault, ResetRelease_NonOwner)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 5000ULL, OWNER1);

    msVault.releaseTo(vaultId, 2000ULL, OWNER2, OWNER1);

    auto statusBefore = msVault.getReleaseStatus(vaultId);
    msVault.resetRelease(vaultId, OWNER3); // Non owner tries to reset
    auto statusAfter = msVault.getReleaseStatus(vaultId);

    // No change in release requests
    EXPECT_EQ(statusAfter.amounts.get(0), statusBefore.amounts.get(0));
    EXPECT_EQ(statusAfter.destinations.get(0), statusBefore.destinations.get(0));
}

TEST(ContractMsVault, ResetRelease_Success)
{
    ContractTestingMsVault msVault;

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 5000ULL, OWNER1);

    // OWNER2 requests a releaseTo
    msVault.releaseTo(vaultId, 2000ULL, OWNER1, OWNER2);
    // Now reset by OWNER2
    msVault.resetRelease(vaultId, OWNER2);

    auto status = msVault.getReleaseStatus(vaultId);
    // All cleared
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

    auto vaultsForOwner2Before = msVault.getVaults(OWNER2);

    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(MSVAULT_VAULT_TYPE_TWO_OUT_OF_X, TEST_VAULT_NAME, { OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

    auto vaultsForOwner2After = msVault.getVaults(OWNER2);
    EXPECT_GE(static_cast<unsigned int>((unsigned)vaultsForOwner2After.numberOfVaults),
        static_cast<unsigned int>((unsigned)(vaultsForOwner2Before.numberOfVaults + 2U)));
}

TEST(ContractMsVault, GetRevenue)
{
    ContractTestingMsVault msVault;
    msVault.beginEpoch();

    auto revenueOutput = msVault.getRevenueInfo();
    EXPECT_EQ(revenueOutput.totalRevenue, 0U);
    EXPECT_EQ(revenueOutput.totalDistributedToShareholders, 0U);
    EXPECT_EQ(revenueOutput.numberOfActiveVaults, 0U);

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    msVault.endEpoch();

    msVault.beginEpoch();

    revenueOutput = msVault.getRevenueInfo();
    EXPECT_EQ(revenueOutput.totalRevenue, MSVAULT_REGISTERING_FEE);
    EXPECT_EQ(revenueOutput.totalDistributedToShareholders, ((int)MSVAULT_REGISTERING_FEE / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS);
    EXPECT_EQ(revenueOutput.numberOfActiveVaults, 0U);

    increaseEnergy(OWNER1, 100000ULL);
    increaseEnergy(OWNER2, 100000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(MSVAULT_VAULT_TYPE_QUORUM, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 50000ULL, OWNER1);

    msVault.endEpoch();

    msVault.beginEpoch();

    revenueOutput = msVault.getRevenueInfo();

    auto total_revenue = MSVAULT_REGISTERING_FEE * 2 + MSVAULT_HOLDING_FEE;
    EXPECT_EQ(revenueOutput.totalRevenue, total_revenue);
    EXPECT_EQ(revenueOutput.totalDistributedToShareholders, ((int)(total_revenue) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS);
    EXPECT_EQ(revenueOutput.numberOfActiveVaults, 1U);

}
