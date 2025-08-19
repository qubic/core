#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"

static const id OWNER1 = ID(_T, _K, _U, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
static const id OWNER2 = ID(_F, _X, _J, _F, _B, _T, _J, _M, _Y, _F, _J, _H, _P, _B, _X, _C, _D, _Q, _T, _L, _Y, _U, _K, _G, _M, _H, _B, _B, _Z, _A, _A, _F, _T, _I, _C, _W, _U, _K, _R, _B, _M, _E, _K, _Y, _N, _U, _P, _M, _R, _M, _B, _D, _N, _D, _R, _G);
static const id OWNER3 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
static const id TEST_VAULT_NAME = ID(_M, _Y, _M, _S, _V, _A, _U, _L, _U, _S, _E, _D, _F, _O, _R, _U, _N, _I, _T, _T, _T, _E, _S, _T, _I, _N, _G, _P, _U, _R, _P, _O, _S, _E, _S, _O, _N, _L, _Y, _U, _N, _I, _T, _T, _E, _S, _C, _O, _R, _E, _S, _M, _A, _R, _T, _T);

static constexpr uint64 TWO_OF_TWO = 2ULL;
static constexpr uint64 TWO_OF_THREE = 2ULL;

static const id DESTINATION = id::randomValue();
static constexpr uint64 QX_ISSUE_ASSET_FEE = 1000000000ull;

class ContractTestingMsVault : protected ContractTesting
{
public:
    ContractTestingMsVault()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(MSVAULT);
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void registerVault(uint64 requiredApprovals, id vaultName, const std::vector<id>& owners, uint64 fee)
    {
        MSVAULT::registerVault_input input;
        for (uint64 i = 0; i < MSVAULT_MAX_OWNERS; i++)
        {
            input.owners.set(i, (i < owners.size()) ? owners[i] : NULL_ID);
        }
        input.requiredApprovals = requiredApprovals;
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
    uint64 findNewvaultIdAfterRegister(const id& owner, uint64 prevCount)
    {
        auto vaultsAfter = getVaults(owner);
        if (vaultsAfter.numberOfVaults == prevCount + 1)
        {
            return (uint64)vaultsAfter.vaultIds.get(prevCount);
        }
        return -1;
    }

    void issueAsset(const id& issuer, const std::string& assetNameStr, sint64 numberOfShares)
    {
        uint64 assetName = assetNameFromString(assetNameStr.c_str());
        QX::IssueAsset_input input{ assetName, numberOfShares, 0, 0 };
        QX::IssueAsset_output output;
        increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QX_ISSUE_ASSET_FEE);
    }

    QX::TransferShareOwnershipAndPossession_output transferAsset(const id& from, const id& to, const Asset& asset, uint64_t amount) {
        QX::TransferShareOwnershipAndPossession_input input;
        input.issuer = asset.issuer;
        input.newOwnerAndPossessor = to;
        input.assetName = asset.assetName;
        input.numberOfShares = amount;
        QX::TransferShareOwnershipAndPossession_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, 1000000);
        return output;
    }

    int64_t transferShareManagementRights(const id& from, const Asset& asset, sint64 numberOfShares, uint32 newManagingContractIndex)
    {
        QX::TransferShareManagementRights_input input;
        input.asset = asset;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;
        QX::TransferShareManagementRights_output output;
        output.transferredNumberOfShares = 0;
        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, from, 0);
        return output.transferredNumberOfShares;
    }

    sint64 revokeAssetManagementRights(const id& from, const Asset& asset, sint64 numberOfShares)
    {
        MSVAULT::revokeAssetManagementRights_input input;
        input.asset = asset;
        input.numberOfShares = numberOfShares;
        MSVAULT::revokeAssetManagementRights_output output;
        output.transferredNumberOfShares = 0;

        // The fee required by QX is 100. Do this to ensure enough fee.
        const uint64 fee = 100;
        increaseEnergy(from, fee);

        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 25, input, output, from, fee);
        return output.transferredNumberOfShares;
    }

    void depositAsset(uint64 vaultId, const Asset& asset, uint64 amount, const id& from)
    {
        MSVAULT::depositAsset_input input;
        input.vaultId = vaultId;
        input.asset = asset;
        input.amount = amount;
        MSVAULT::depositAsset_output output;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 19, input, output, from, 0);
    }

    void releaseAssetTo(uint64 vaultId, const Asset& asset, uint64 amount, const id& destination, const id& owner, uint64 fee = MSVAULT_RELEASE_FEE)
    {
        MSVAULT::releaseAssetTo_input input;
        input.vaultId = vaultId;
        input.asset = asset;
        input.amount = amount;
        input.destination = destination;

        increaseEnergy(owner, fee);
        MSVAULT::releaseAssetTo_output output;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 20, input, output, owner, fee);
    }

    void resetAssetRelease(uint64 vaultId, const id& owner, uint64 fee = MSVAULT_RELEASE_RESET_FEE)
    {
        MSVAULT::resetAssetRelease_input input;
        input.vaultId = vaultId;

        increaseEnergy(owner, fee);
        MSVAULT::resetAssetRelease_output output;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 21, input, output, owner, fee);
    }

    MSVAULT::getVaultAssetBalances_output getVaultAssetBalances(uint64 vaultId) const
    {
        MSVAULT::getVaultAssetBalances_input input;
        input.vaultId = vaultId;
        MSVAULT::getVaultAssetBalances_output output;
        callFunction(MSVAULT_CONTRACT_INDEX, 22, input, output);
        return output;
    }

    MSVAULT::getAssetReleaseStatus_output getAssetReleaseStatus(uint64 vaultId) const
    {
        MSVAULT::getAssetReleaseStatus_input input;
        input.vaultId = vaultId;
        MSVAULT::getAssetReleaseStatus_output output;
        callFunction(MSVAULT_CONTRACT_INDEX, 23, input, output);
        return output;
    }

    ~ContractTestingMsVault()
    {
    }
};

TEST(ContractMsVault, RegisterVault_InsufficientFee)
{
    ContractTestingMsVault msVault;

    // Check how many vaults OWNER1 has initially
    auto vaultsO1Before = msVault.getVaults(OWNER1);

    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE);
    // Attempt with insufficient fee
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, 5000ULL);

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
    // Only one owner => should fail
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1 }, MSVAULT_REGISTERING_FEE);

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
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1After = msVault.getVaults(OWNER1);
    EXPECT_EQ(static_cast<unsigned int>(vaultsO1After.numberOfVaults),
        static_cast<unsigned int>(vaultsO1Before.numberOfVaults + 1));

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

    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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

    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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
    // 2 out of 2 owners
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    // 2 out of 3 owners
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

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

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    // 2 out of 3
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

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

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    // 2 out of 2
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    // 2 out of 2
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

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

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    auto vaultsForOwner2Before = msVault.getVaults(OWNER2);

    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);

    auto vaultsForOwner2After = msVault.getVaults(OWNER2);
    EXPECT_GE(static_cast<unsigned int>(vaultsForOwner2After.numberOfVaults),
        static_cast<unsigned int>(vaultsForOwner2Before.numberOfVaults + 2U));
}

TEST(ContractMsVault, GetRevenue)
{
    ContractTestingMsVault msVault;
    msVault.beginEpoch();

    auto revenueOutput = msVault.getRevenueInfo();
    EXPECT_EQ(revenueOutput.totalRevenue, 0U);
    EXPECT_EQ(revenueOutput.totalDistributedToShareholders, 0U);
    EXPECT_EQ(revenueOutput.numberOfActiveVaults, 0U);

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000000ULL);
    increaseEnergy(OWNER3, 100000ULL);

    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    msVault.endEpoch();

    msVault.beginEpoch();

    revenueOutput = msVault.getRevenueInfo();
    // first vault is destroyed after paying dividends
    EXPECT_EQ(revenueOutput.totalRevenue, MSVAULT_REGISTERING_FEE);
    EXPECT_EQ(revenueOutput.totalDistributedToShareholders, ((int)MSVAULT_REGISTERING_FEE / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS);
    EXPECT_EQ(revenueOutput.numberOfActiveVaults, 0U);

    increaseEnergy(OWNER1, 100000000ULL);
    increaseEnergy(OWNER2, 100000000ULL);
    increaseEnergy(OWNER3, 100000000ULL);

    msVault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);

    auto vaultsO1 = msVault.getVaults(OWNER1);
    uint64 vaultId = vaultsO1.vaultIds.get(vaultsO1.numberOfVaults - 1);

    msVault.deposit(vaultId, 500000000ULL, OWNER1);

    msVault.endEpoch();

    msVault.beginEpoch();

    revenueOutput = msVault.getRevenueInfo();

    auto total_revenue = MSVAULT_REGISTERING_FEE * 2 + MSVAULT_HOLDING_FEE;
    EXPECT_EQ(revenueOutput.totalRevenue, total_revenue);
    EXPECT_EQ(revenueOutput.totalDistributedToShareholders, ((int)(total_revenue) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS);
    EXPECT_EQ(revenueOutput.numberOfActiveVaults, 1U);

}

TEST(ContractMsVault, ManagementRightsVsDirectDeposit)
{
    ContractTestingMsVault msvault;

    // Create an issuer and two users.
    const id ISSUER = id::randomValue();
    const id USER_WITH_RIGHTS = id::randomValue(); // This user will do it correctly
    const id USER_WITHOUT_RIGHTS = id::randomValue(); // This user will attempt a direct deposit first

    Asset assetTest = { ISSUER, assetNameFromString("ASSET") };
    const sint64 initialDistribution = 50000;

    // Give everyone energy for fees
    increaseEnergy(ISSUER, QX_ISSUE_ASSET_FEE + (1000000 * 2));
    increaseEnergy(USER_WITH_RIGHTS, MSVAULT_REGISTERING_FEE + (1000000 * 3));
    increaseEnergy(USER_WITHOUT_RIGHTS, 1000000 * 3); // More energy for the correct attempt later

    // Issue the asset and distribute it to the two users
    msvault.issueAsset(ISSUER, "ASSET", initialDistribution * 2);
    msvault.transferAsset(ISSUER, USER_WITH_RIGHTS, assetTest, initialDistribution);
    msvault.transferAsset(ISSUER, USER_WITHOUT_RIGHTS, assetTest, initialDistribution);

    // Verify initial on-chain balances (both users' shares are managed by QX currently)
    EXPECT_EQ(numberOfShares(assetTest, { USER_WITH_RIGHTS, QX_CONTRACT_INDEX },
                                        { USER_WITH_RIGHTS, QX_CONTRACT_INDEX }), initialDistribution);
    EXPECT_EQ(numberOfShares(assetTest, { USER_WITHOUT_RIGHTS, QX_CONTRACT_INDEX },
                                        { USER_WITHOUT_RIGHTS, QX_CONTRACT_INDEX }), initialDistribution);

    // Create a simple vault owned by USER_WITH_RIGHTS
    msvault.registerVault(2, TEST_VAULT_NAME, { USER_WITH_RIGHTS, OWNER1 }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(USER_WITH_RIGHTS);
    uint64 vaultId = vaults.vaultIds.get(0);

    // User with Management Rights
    const sint64 sharesToManage1 = 10000;
    msvault.transferShareManagementRights(USER_WITH_RIGHTS, assetTest, sharesToManage1, MSVAULT_CONTRACT_INDEX);

    // verify that management rights were transferred successfully
    EXPECT_EQ(numberOfShares(assetTest, { USER_WITH_RIGHTS, MSVAULT_CONTRACT_INDEX },
                                        { USER_WITH_RIGHTS, MSVAULT_CONTRACT_INDEX }), sharesToManage1);
    EXPECT_EQ(numberOfShares(assetTest, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
                                        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX }), 0);

    // This user now makes multiple deposits
    const sint64 deposit1_U1 = 1000;
    const sint64 deposit2_U1 = 2500;
    msvault.depositAsset(vaultId, assetTest, deposit1_U1, USER_WITH_RIGHTS);

    // Verify balances after first deposit
    sint64 sc_onchain_balance = numberOfShares(assetTest, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
                                                          { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(sc_onchain_balance, deposit1_U1);
    sint64 user_managed_balance = numberOfShares(assetTest, { USER_WITH_RIGHTS, MSVAULT_CONTRACT_INDEX },
                                                            { USER_WITH_RIGHTS, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(user_managed_balance, sharesToManage1 - deposit1_U1);

    msvault.depositAsset(vaultId, assetTest, deposit2_U1, USER_WITH_RIGHTS);

    // verify balances after second deposit
    sc_onchain_balance = numberOfShares(assetTest, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
                                                   { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(sc_onchain_balance, deposit1_U1 + deposit2_U1);
    sint64 user1_managed_balance = numberOfShares(assetTest, { USER_WITH_RIGHTS, MSVAULT_CONTRACT_INDEX },
                                                             { USER_WITH_RIGHTS, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(user1_managed_balance, sharesToManage1 - deposit1_U1 - deposit2_U1);

    // user without management rights
    sint64 sc_balance_before_direct_attempt = sc_onchain_balance;
    sint64 user3_balance_before = numberOfShares(assetTest, { USER_WITHOUT_RIGHTS, QX_CONTRACT_INDEX },
                                                            { USER_WITHOUT_RIGHTS, QX_CONTRACT_INDEX });

    // This user attempts to deposit directly
    msvault.depositAsset(vaultId, assetTest, 500, USER_WITHOUT_RIGHTS);

    // Verify that no shares were transferred
    sint64 sc_balance_after_direct_attempt = numberOfShares(assetTest, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
                                                                       { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(sc_balance_after_direct_attempt, sc_balance_before_direct_attempt);

    sint64 user3_balance_after = numberOfShares(assetTest, { USER_WITHOUT_RIGHTS, QX_CONTRACT_INDEX },
                                                           { USER_WITHOUT_RIGHTS, QX_CONTRACT_INDEX });
    EXPECT_EQ(user3_balance_after, user3_balance_before); // User's balance should be unchanged

    sint64 user3_balance_after_msvault = numberOfShares(assetTest, { USER_WITHOUT_RIGHTS, MSVAULT_CONTRACT_INDEX },
                                                                   { USER_WITHOUT_RIGHTS, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(user3_balance_after_msvault, 0);

    // the second user now does it the correct way
    const sint64 sharesToManage2 = 8000;
    msvault.transferShareManagementRights(USER_WITHOUT_RIGHTS, assetTest, sharesToManage2, MSVAULT_CONTRACT_INDEX);

    // Verify their management rights were transferred successfully
    EXPECT_EQ(numberOfShares(assetTest, { USER_WITHOUT_RIGHTS, MSVAULT_CONTRACT_INDEX },
                                        { USER_WITHOUT_RIGHTS, MSVAULT_CONTRACT_INDEX }), sharesToManage2);

    const sint64 deposit1_U2 = 4000;
    msvault.depositAsset(vaultId, assetTest, deposit1_U2, USER_WITHOUT_RIGHTS);

    // check the total balance in the smart contract
    sint64 final_sc_balance = numberOfShares(assetTest, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
                                                        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    sint64 total_deposited = deposit1_U1 + deposit2_U1 + deposit1_U2;
    EXPECT_EQ(final_sc_balance, total_deposited);

    // Also verify the second user's remaining managed balance
    sint64 user2_managed_balance = numberOfShares(assetTest, { USER_WITHOUT_RIGHTS, MSVAULT_CONTRACT_INDEX },
                                                             { USER_WITHOUT_RIGHTS, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(user2_managed_balance, sharesToManage2 - deposit1_U2);
}

TEST(ContractMsVault, DepositAsset_Success)
{
    ContractTestingMsVault msvault;
    Asset assetTest = { OWNER1, assetNameFromString("ASSET") };

    // Create a vault and issue an asset to OWNER1
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE + QX_ISSUE_ASSET_FEE);
    msvault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(OWNER1);
    uint64 vaultId = vaults.vaultIds.get(0);

    msvault.issueAsset(OWNER1, "ASSET", 1000000);

    auto OWNER4 = id::randomValue();

    auto transfered = msvault.transferShareManagementRights(OWNER1, assetTest, 5000, MSVAULT_CONTRACT_INDEX);

    // Deposit the asset into the vault
    msvault.depositAsset(vaultId, assetTest, 500, OWNER1);

    // Check the vault's asset balance
    auto assetBalances = msvault.getVaultAssetBalances(vaultId);
    EXPECT_EQ(assetBalances.status, 1ULL);
    EXPECT_EQ(assetBalances.numberOfAssetTypes, 1ULL);

    auto firstAssetBalance = assetBalances.assetBalances.get(0);
    EXPECT_EQ(firstAssetBalance.asset.issuer, assetTest.issuer);
    EXPECT_EQ(firstAssetBalance.asset.assetName, assetTest.assetName);
    EXPECT_EQ(firstAssetBalance.balance, 500ULL);

    // Check SC's shares
    sint64 scShares = numberOfShares(assetTest,
        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(scShares, 500LL);
}

TEST(ContractMsVault, DepositAsset_MaxTypes)
{
    ContractTestingMsVault msvault;

    // Create a vault
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE + QX_ISSUE_ASSET_FEE * (MSVAULT_MAX_ASSET_TYPES + 1)); // Extra energy for fees
    msvault.registerVault(2ULL, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(OWNER1);
    uint64 vaultId = vaults.vaultIds.get(0);

    // Deposit the maximum number of different asset types
    for (uint64 i = 0; i < MSVAULT_MAX_ASSET_TYPES; i++)
    {
        std::string assetName = "ASSET" + std::to_string(i);
        Asset currentAsset = { OWNER1, assetNameFromString(assetName.c_str()) };
        msvault.issueAsset(OWNER1, assetName, 1000000);
        msvault.transferShareManagementRights(OWNER1, currentAsset, 100000, MSVAULT_CONTRACT_INDEX);
        msvault.depositAsset(vaultId, currentAsset, 1000, OWNER1);
    }

    // Check if max asset types reached
    auto balances = msvault.getVaultAssetBalances(vaultId);
    EXPECT_EQ(balances.numberOfAssetTypes, MSVAULT_MAX_ASSET_TYPES);

    // Try to deposit one more asset type
    Asset extraAsset = { OWNER1, assetNameFromString("ASSETE") };
    msvault.issueAsset(OWNER1, "ASSETE", 100000);
    msvault.transferShareManagementRights(OWNER1, extraAsset, 100000, MSVAULT_CONTRACT_INDEX);
    msvault.depositAsset(vaultId, extraAsset, 1000, OWNER1);

    // The number of asset types should not have increased
    auto balancesAfter = msvault.getVaultAssetBalances(vaultId);
    EXPECT_EQ(balancesAfter.numberOfAssetTypes, MSVAULT_MAX_ASSET_TYPES);
}

TEST(ContractMsVault, ReleaseAssetTo_FullApproval)
{
    ContractTestingMsVault msvault;
    Asset assetTest = { OWNER1, assetNameFromString("ASSET") };

    // Create a 2-of-3 vault, issue and deposit an asset
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE + MSVAULT_RELEASE_FEE);
    increaseEnergy(OWNER2, MSVAULT_RELEASE_FEE);
    msvault.registerVault(TWO_OF_THREE, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(OWNER1);
    uint64 vaultId = vaults.vaultIds.get(0);

    msvault.issueAsset(OWNER1, "ASSET", 1000000);
    msvault.transferShareManagementRights(OWNER1, assetTest, 800, MSVAULT_CONTRACT_INDEX);
    msvault.depositAsset(vaultId, assetTest, 800, OWNER1);

    // Check initial balances
    sint64 destinationBalanceBefore = numberOfPossessedShares(assetTest.assetName, assetTest.issuer, DESTINATION, DESTINATION, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(destinationBalanceBefore, 0LL);
    auto vaultAssetBalanceBefore = msvault.getVaultAssetBalances(vaultId).assetBalances.get(0).balance;
    EXPECT_EQ(vaultAssetBalanceBefore, 800ULL);

    // Owners approve the release
    msvault.releaseAssetTo(vaultId, assetTest, 500, DESTINATION, OWNER1);
    msvault.releaseAssetTo(vaultId, assetTest, 500, DESTINATION, OWNER2);

    // Check final balances
    sint64 destinationBalanceAfter = numberOfShares(assetTest, AssetOwnershipSelect::byOwner(DESTINATION),
                                                               AssetPossessionSelect::byPossessor(DESTINATION));
    EXPECT_EQ(destinationBalanceAfter, 500LL);

    auto vaultAssetBalanceAfter = msvault.getVaultAssetBalances(vaultId).assetBalances.get(0).balance;
    EXPECT_EQ(vaultAssetBalanceAfter, 300ULL); // 800 - 500

    // The vault's qubic balance should be 0 after paying the fee
    auto vaultQubicBalanceAfter = msvault.getBalanceOf(vaultId);
    EXPECT_EQ(vaultQubicBalanceAfter.balance, 0LL);

    // Release status should be reset
    auto releaseStatus = msvault.getAssetReleaseStatus(vaultId);
    EXPECT_EQ(releaseStatus.amounts.get(0), 0ULL);
    EXPECT_EQ(releaseStatus.destinations.get(0), NULL_ID);
}

TEST(ContractMsVault, ReleaseAssetTo_PartialApproval)
{
    ContractTestingMsVault msvault;
    Asset assetTest = { OWNER1, assetNameFromString("ASSET") };

    // Setup
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE + MSVAULT_RELEASE_FEE);
    msvault.registerVault(TWO_OF_THREE, TEST_VAULT_NAME, { OWNER1, OWNER2, OWNER3 }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(OWNER1);
    uint64 vaultId = vaults.vaultIds.get(0);
    msvault.issueAsset(OWNER1, "ASSET", 1000000);
    msvault.transferShareManagementRights(OWNER1, assetTest, 800, MSVAULT_CONTRACT_INDEX);
    msvault.depositAsset(vaultId, assetTest, 800, OWNER1);

    // Only one owner approves
    msvault.releaseAssetTo(vaultId, assetTest, 500, DESTINATION, OWNER1);

    // Check release status is pending
    auto status = msvault.getAssetReleaseStatus(vaultId);
    EXPECT_EQ(status.status, 1ULL);
    // Owner 1 is at index 0
    EXPECT_EQ(status.assets.get(0).assetName, assetTest.assetName);
    EXPECT_EQ(status.amounts.get(0), 500ULL);
    EXPECT_EQ(status.destinations.get(0), DESTINATION);
    // Other owner slots are empty
    EXPECT_EQ(status.amounts.get(1), 0ULL);

    // Balances should be unchanged
    sint64 destinationBalance = numberOfPossessedShares(assetTest.assetName, assetTest.issuer, DESTINATION, DESTINATION,
                                                        QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(destinationBalance, 0LL);
    auto vaultBalance = msvault.getVaultAssetBalances(vaultId).assetBalances.get(0).balance;
    EXPECT_EQ(vaultBalance, 800ULL);
}

TEST(ContractMsVault, ResetAssetRelease_Success)
{
    ContractTestingMsVault msvault;
    Asset assetTest = { OWNER1, assetNameFromString("ASSET") };

    // Setup
    increaseEnergy(OWNER1, MSVAULT_REGISTERING_FEE + MSVAULT_RELEASE_RESET_FEE + MSVAULT_RELEASE_FEE);
    msvault.registerVault(TWO_OF_TWO, TEST_VAULT_NAME, { OWNER1, OWNER2 }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(OWNER1);
    uint64 vaultId = vaults.vaultIds.get(0);
    msvault.issueAsset(OWNER1, "ASSET", 1000000);
    msvault.transferShareManagementRights(OWNER1, assetTest, 100, MSVAULT_CONTRACT_INDEX);
    msvault.depositAsset(vaultId, assetTest, 100, OWNER1);

    // Propose and then reset a release
    msvault.releaseAssetTo(vaultId, assetTest, 50, DESTINATION, OWNER1);

    // Check status is pending before reset
    auto statusBefore = msvault.getAssetReleaseStatus(vaultId);
    EXPECT_EQ(statusBefore.amounts.get(0), 50ULL);

    // Reset the release
    msvault.resetAssetRelease(vaultId, OWNER1);

    // Status should be cleared for that owner
    auto statusAfter = msvault.getAssetReleaseStatus(vaultId);
    EXPECT_EQ(statusAfter.amounts.get(0), 0ULL);
    EXPECT_EQ(statusAfter.destinations.get(0), NULL_ID);

    // Vault balance should be unchanged
    auto vaultBalance = msvault.getVaultAssetBalances(vaultId).assetBalances.get(0).balance;
    EXPECT_EQ(vaultBalance, 100ULL);
}

TEST(ContractMsVault, FullLifecycle_BalanceVerification)
{
    ContractTestingMsVault msvault;
    const id USER = OWNER1;
    const id PARTNER = OWNER2;
    const id DESTINATION_ACC = OWNER3;
    Asset assetTest = { USER, assetNameFromString("ASSET") };
    const sint64 initialShares = 10000;
    const sint64 sharesToManage = 5000;
    const sint64 sharesToDeposit = 4000;
    const sint64 sharesToRelease = 1500;

    // Issue asset and create a type 2 vault
    increaseEnergy(USER, MSVAULT_REGISTERING_FEE + QX_ISSUE_ASSET_FEE);
    increaseEnergy(PARTNER, MSVAULT_RELEASE_FEE);

    msvault.issueAsset(USER, "ASSET", initialShares);
    msvault.registerVault(TWO_OF_TWO, TEST_VAULT_NAME, { USER, PARTNER }, MSVAULT_REGISTERING_FEE);
    auto vaults = msvault.getVaults(USER);
    uint64 vaultId = vaults.vaultIds.get(0);

    // Verify user has full on-chain balance under QX management
    sint64 userShares_QX = numberOfShares(assetTest, { USER, QX_CONTRACT_INDEX }, { USER, QX_CONTRACT_INDEX });
    EXPECT_EQ(userShares_QX, initialShares);

    // User gives MsVault management rights over a portion of their shares
    msvault.transferShareManagementRights(USER, assetTest, sharesToManage, MSVAULT_CONTRACT_INDEX);

    // Verify on-chain balances after management transfer
    sint64 userShares_MSVAULT_Managed = numberOfShares(assetTest, { USER, MSVAULT_CONTRACT_INDEX }, { USER, MSVAULT_CONTRACT_INDEX });
    userShares_QX = numberOfShares(assetTest, { USER, QX_CONTRACT_INDEX }, { USER, QX_CONTRACT_INDEX });
    EXPECT_EQ(userShares_MSVAULT_Managed, sharesToManage);
    EXPECT_EQ(userShares_QX, initialShares - sharesToManage);

    // User deposits the MsVault-managed shares into the vault
    msvault.depositAsset(vaultId, assetTest, sharesToDeposit, USER);

    // User's on-chain balance of MsVault-managed shares should decrease
    userShares_MSVAULT_Managed = numberOfShares(assetTest, { USER, MSVAULT_CONTRACT_INDEX }, { USER, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(userShares_MSVAULT_Managed, sharesToManage - sharesToDeposit); // 5000 - 4000 = 1000

    // MsVault contract's on-chain balance should increase
    sint64 scShares_onchain = numberOfShares(assetTest,
        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(scShares_onchain, sharesToDeposit);

    // Vault's internal balance should match the on-chain balance
    auto vaultBalances = msvault.getVaultAssetBalances(vaultId);
    EXPECT_EQ(vaultBalances.status, 1ULL);
    EXPECT_EQ(vaultBalances.numberOfAssetTypes, 1ULL);
    EXPECT_EQ(vaultBalances.assetBalances.get(0).balance, sharesToDeposit);

    // Both owners approve a release to the destination
    msvault.releaseAssetTo(vaultId, assetTest, sharesToRelease, DESTINATION_ACC, USER);
    msvault.releaseAssetTo(vaultId, assetTest, sharesToRelease, DESTINATION_ACC, PARTNER);

    // MsVault contract's on-chain balance should decrease
    scShares_onchain = numberOfShares(assetTest, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
        { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(scShares_onchain, sharesToDeposit - sharesToRelease);

    // Vault's internal balance should be updated correctly
    vaultBalances = msvault.getVaultAssetBalances(vaultId);
    EXPECT_EQ(vaultBalances.assetBalances.get(0).balance, sharesToDeposit - sharesToRelease);

    // Destination's on-chain balance should increase
    sint64 destinationShares = numberOfShares(assetTest, { DESTINATION_ACC, MSVAULT_CONTRACT_INDEX }, { DESTINATION_ACC, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(destinationShares, sharesToRelease);
}

TEST(ContractMsVault, StressTest_MultiUser_MultiAsset)
{
    ContractTestingMsVault msvault;

    // Define users, assets, and vaults
    const int USER_COUNT = 16;
    const int ASSET_COUNT = 8;
    const int VAULT_COUNT = 3;
    std::vector<id> users;
    std::vector<Asset> assets;

    for (int i = 0; i < USER_COUNT; ++i)
    {
        users.push_back(id::randomValue());
        increaseEnergy(users[i], 1000000000000ULL);
    }

    for (int i = 0; i < ASSET_COUNT; ++i)
    {
        // Issue each asset from a different user for variety
        id issuer = users[i];
        std::string assetName = "ASSET" + std::to_string(i);
        assets.push_back({ issuer, assetNameFromString(assetName.c_str()) });
        msvault.issueAsset(issuer, assetName, 1000000); // Issue 1M of each token
    }

    // Create 3 vaults with different sets of 4 owners each
    msvault.registerVault(3, id::randomValue(), { users[0], users[1], users[2], users[3] }, MSVAULT_REGISTERING_FEE);
    msvault.registerVault(2, id::randomValue(), { users[4], users[5], users[6], users[7] }, MSVAULT_REGISTERING_FEE);
    msvault.registerVault(4, id::randomValue(), { users[8], users[9], users[10], users[11] }, MSVAULT_REGISTERING_FEE);

    // Each of the 8 assets is deposited twice by its owner
    uint64 targetVaultId = 0;
    const sint64 depositAmount = 100;

    for (int i = 0; i < USER_COUNT; ++i)
    {
        int assetIndex = i % ASSET_COUNT;
        Asset assetToDeposit = assets[assetIndex];
        id owner_of_asset = users[assetIndex];

        msvault.transferShareManagementRights(owner_of_asset, assetToDeposit, depositAmount, MSVAULT_CONTRACT_INDEX);
        msvault.depositAsset(targetVaultId, assetToDeposit, depositAmount, owner_of_asset);
    }

    // Check the state of the target vault
    auto vaultBalances = msvault.getVaultAssetBalances(targetVaultId);
    EXPECT_EQ(vaultBalances.status, 1ULL);
    EXPECT_EQ(vaultBalances.numberOfAssetTypes, (uint64_t)ASSET_COUNT);
    for (uint64 i = 0; i < ASSET_COUNT; ++i)
    {
        // Verify each asset has a balance of 200 (deposited twice)
        EXPECT_EQ(vaultBalances.assetBalances.get(i).balance, depositAmount * 2);
    }

    // From Vault 0, owners 0, 1, and 2 approve a release
    const id releaseDestination = users[15];
    const Asset assetToRelease = assets[0]; // Release ASSET_0
    const sint64 releaseAmount = 75;

    // A 3-of-4 vault, so we need 3 approvals
    msvault.releaseAssetTo(targetVaultId, assetToRelease, releaseAmount, releaseDestination, users[0]);
    msvault.releaseAssetTo(targetVaultId, assetToRelease, releaseAmount, releaseDestination, users[1]);
    msvault.releaseAssetTo(targetVaultId, assetToRelease, releaseAmount, releaseDestination, users[2]);

    // Check destination on-chain balance
    sint64 destBalance = numberOfShares(assetToRelease, { releaseDestination, MSVAULT_CONTRACT_INDEX },
                                                        { releaseDestination, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(destBalance, releaseAmount);

    // Check vault's internal accounting for the released asset
    vaultBalances = msvault.getVaultAssetBalances(targetVaultId);
    bool foundReleasedAsset = false;
    for (uint64 i = 0; i < vaultBalances.numberOfAssetTypes; ++i)
    {
        auto bal = vaultBalances.assetBalances.get(i);
        if (bal.asset.assetName == assetToRelease.assetName && bal.asset.issuer == assetToRelease.issuer)
        {
            // Expected balance is (100 * 2) - 75 = 125
            EXPECT_EQ(bal.balance, (depositAmount * 2) - releaseAmount);
            foundReleasedAsset = true;
            break;
        }
    }
    EXPECT_TRUE(foundReleasedAsset);

    // Check MsVault's on-chain balance for the released asset
    sint64 scOnChainBalance = numberOfShares(assetToRelease, { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX },
                                                             { id(MSVAULT_CONTRACT_INDEX, 0, 0, 0), MSVAULT_CONTRACT_INDEX });
    // The total on-chain balance should also be (100 * 2) - 75 = 125
    EXPECT_EQ(scOnChainBalance, (depositAmount * 2) - releaseAmount);
}

TEST(ContractMsVault, RevokeAssetManagementRights_Success)
{
    ContractTestingMsVault msvault;

    const id USER = OWNER1;
    const Asset asset = { USER, assetNameFromString("REVOKE") };
    const sint64 initialShares = 10000;
    const sint64 sharesToManage = 4000;
    const sint64 sharesToRevoke = 3000;

    // Issue asset and transfer management rights to MsVault
    increaseEnergy(USER, QX_ISSUE_ASSET_FEE + 1000000 + 100); // Energy for all fees
    msvault.issueAsset(USER, "REVOKE", initialShares);

    // Verify initial state: all shares managed by QX
    EXPECT_EQ(numberOfShares(asset, { USER, QX_CONTRACT_INDEX }, { USER, QX_CONTRACT_INDEX }), initialShares);
    EXPECT_EQ(numberOfShares(asset, { USER, MSVAULT_CONTRACT_INDEX }, { USER, MSVAULT_CONTRACT_INDEX }), 0);

    // User gives MsVault management rights over a portion of their shares
    msvault.transferShareManagementRights(USER, asset, sharesToManage, MSVAULT_CONTRACT_INDEX);

    // Verify intermediate state: rights are split between QX and MsVault
    EXPECT_EQ(numberOfShares(asset, { USER, QX_CONTRACT_INDEX }, { USER, QX_CONTRACT_INDEX }), initialShares - sharesToManage);
    EXPECT_EQ(numberOfShares(asset, { USER, MSVAULT_CONTRACT_INDEX }, { USER, MSVAULT_CONTRACT_INDEX }), sharesToManage);

    // User revokes a portion of the managed rights from MsVault. The helper now handles the fee.
    sint64 revokedAmount = msvault.revokeAssetManagementRights(USER, asset, sharesToRevoke);

    // Verify the outcome
    EXPECT_EQ(revokedAmount, sharesToRevoke);

    // The amount managed by MsVault should decrease
    sint64 finalManagedByMsVault = numberOfShares(asset, { USER, MSVAULT_CONTRACT_INDEX }, { USER, MSVAULT_CONTRACT_INDEX });
    EXPECT_EQ(finalManagedByMsVault, sharesToManage - sharesToRevoke); // 4000 - 3000 = 1000

    // The amount managed by QX should increase accordingly
    sint64 finalManagedByQx = numberOfShares(asset, { USER, QX_CONTRACT_INDEX }, { USER, QX_CONTRACT_INDEX });
    EXPECT_EQ(finalManagedByQx, (initialShares - sharesToManage) + sharesToRevoke); // 6000 + 3000 = 9000
}
