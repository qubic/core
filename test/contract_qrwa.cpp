#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"

#define ENABLE_BALANCE_DEBUG 0

// Pseudo IDs (for testing only)

// QMINE_ISSUER is is also the ADMIN_ADDRESS
static const id QMINE_ISSUER = ID(
    _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
    _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
    _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
    _R, _V, _Z, _B, _T, _Y, _K, _G
);
static const id ADMIN_ADDRESS = QMINE_ISSUER;

// temporary holder for the initial 150M QMINE supply
static const id TREASURY_HOLDER = id::randomValue();

// Addresses for governance-set fees
static const id FEE_ADDR_E = id::randomValue(); // Electricity fees address
static const id FEE_ADDR_M = id::randomValue(); // Maintenance fees address
static const id FEE_ADDR_R = id::randomValue(); // Reinvestment fees address

// pseudo test address for QMINE developer
static const id QMINE_DEV_ADDR_TEST = ID(
    _Z, _O, _X, _X, _I, _D, _C, _Z, _I, _M, _G, _C, _E, _C, _C, _F,
    _A, _X, _D, _D, _C, _M, _B, _B, _X, _C, _D, _A, _Q, _J, _I, _H,
    _G, _O, _O, _A, _T, _A, _F, _P, _S, _B, _F, _I, _O, _F, _O, _Y,
    _E, _C, _F, _K, _U, _F, _P, _B
);

// Test accounts for holders and users
static const id HOLDER_A = id::randomValue();
static const id HOLDER_B = id::randomValue();
static const id HOLDER_C = id::randomValue();
static const id USER_D = id::randomValue(); // no-share user
static const id DESTINATION_ADDR = id::randomValue(); // dest for asset releases

// Test QMINE Asset (using the random issuer for testing only)
static const Asset QMINE_ASSET = { QMINE_ISSUER, 297666170193ULL };

// Fees for dependent contracts
static constexpr uint64 QX_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QX_TRANSFER_FEE = 100ull; // Fee for transfering assets back to QX
static constexpr uint64 QX_MGT_TRANSFER_FEE = 0ull; // Fee for QX::TransferShareManagementRights
static constexpr sint64 QUTIL_STM1_FEE = 10LL; // QUTIL SendToManyV1 fee (QUTIL_STM1_INVOCATION_FEE)


enum qRWAFunctionIds
{
    QRWA_FUNC_GET_GOV_PARAMS = 1,
    QRWA_FUNC_GET_GOV_POLL = 2,
    QRWA_FUNC_GET_ASSET_RELEASE_POLL = 3,
    QRWA_FUNC_GET_TREASURY_BALANCE = 4,
    QRWA_FUNC_GET_DIVIDEND_BALANCES = 5,
    QRWA_FUNC_GET_TOTAL_DISTRIBUTED = 6
};

enum qRWAProcedureIds
{
    QRWA_PROC_DONATE_TO_TREASURY = 3,
    QRWA_PROC_VOTE_GOV_PARAMS = 4,
    QRWA_PROC_CREATE_ASSET_RELEASE_POLL = 5,
    QRWA_PROC_VOTE_ASSET_RELEASE = 6,
    QRWA_PROC_DEPOSIT_GENERAL_ASSET = 7,
    QRWA_PROC_REVOKE_ASSET = 8,
};

enum QxProcedureIds
{
    QX_PROC_ISSUE_ASSET = 1,
    QX_PROC_TRANSFER_SHARES = 2,
    QX_PROC_TRANSFER_MANAGEMENT = 9
};

enum QutilProcedureIds
{
    QUTIL_PROC_SEND_TO_MANY_V1 = 1
};


class ContractTestingQRWA : protected ContractTesting
{
    // Grant access to protected/private members for setup
    friend struct QRWA;

public:
    ContractTestingQRWA()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QRWA);
        callSystemProcedure(QRWA_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QUTIL);
        callSystemProcedure(QUTIL_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QSWAP);
        callSystemProcedure(QSWAP_CONTRACT_INDEX, INITIALIZE);

        // Custom Initialization for qRWA State
        // (Overrides defaults from INITIALIZE() for testing purposes)
        QRWA* state = getState();

        // Fee addresses
        // Note: We want to check these Fee Addresses separately,
        // we use different addresses instead of same address as the Admin Address
        state->mCurrentGovParams.electricityAddress = FEE_ADDR_E;
        state->mCurrentGovParams.maintenanceAddress = FEE_ADDR_M;
        state->mCurrentGovParams.reinvestmentAddress = FEE_ADDR_R;
    }

    QRWA* getState()
    {
        return (QRWA*)contractStates[QRWA_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QRWA_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QRWA_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void endTick(bool expectSuccess = true)
    {
        callSystemProcedure(QRWA_CONTRACT_INDEX, END_TICK, expectSuccess);
    }

    // manually reset the last payout time for testing.
    void resetPayoutTime()
    {
        getState()->mLastPayoutTime = { 0, 0, 0, 0, 0, 0, 0 };
    }

    // QX/QUTIL Contract Wrappers

    void issueAsset(const id& issuer, uint64 assetName, sint64 shares)
    {
        QX::IssueAsset_input input{ assetName, shares, 0, 0 };
        QX::IssueAsset_output output;
        increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
        invokeUserProcedure(QX_CONTRACT_INDEX, QX_PROC_ISSUE_ASSET, input, output, issuer, QX_ISSUE_ASSET_FEE);
    }

    // Transfers asset ownership and possession on QX.
    void transferAsset(const id& from, const id& to, const Asset& asset, sint64 shares)
    {
        QX::TransferShareOwnershipAndPossession_input input{ asset.issuer, to, asset.assetName, shares };
        QX::TransferShareOwnershipAndPossession_output output;
        increaseEnergy(from, QX_TRANSFER_FEE);
        invokeUserProcedure(QX_CONTRACT_INDEX, QX_PROC_TRANSFER_SHARES, input, output, from, QX_TRANSFER_FEE);
    }

    // Transfers management rights of an asset to another contract
    void transferManagementRights(const id& from, const Asset& asset, sint64 shares, uint32 toContract)
    {
        QX::TransferShareManagementRights_input input{ asset, shares, toContract };
        QX::TransferShareManagementRights_output output;
        increaseEnergy(from, QX_MGT_TRANSFER_FEE);
        invokeUserProcedure(QX_CONTRACT_INDEX, QX_PROC_TRANSFER_MANAGEMENT, input, output, from, QX_MGT_TRANSFER_FEE);
    }

    // Simulates a dividend payout from QLI pool using QUTIL::SendToManyV1.
    void sendToMany(const id& from, const id& to, sint64 amount)
    {
        QUTIL::SendToManyV1_input input = {};
        input.dst0 = to;
        input.amt0 = amount;
        QUTIL::SendToManyV1_output output;
        increaseEnergy(from, amount + QUTIL_STM1_FEE);
        invokeUserProcedure(QUTIL_CONTRACT_INDEX, QUTIL_PROC_SEND_TO_MANY_V1, input, output, from, amount + QUTIL_STM1_FEE);
    }

    // QRWA Procedure Wrappers

    uint64 donateToTreasury(const id& from, uint64 amount)
    {
        QRWA::DonateToTreasury_input input{ amount };
        QRWA::DonateToTreasury_output output;
        invokeUserProcedure(QRWA_CONTRACT_INDEX, QRWA_PROC_DONATE_TO_TREASURY, input, output, from, 0);
        return output.status;
    }

    uint64 voteGovParams(const id& from, const QRWA::QRWAGovParams& params)
    {
        QRWA::VoteGovParams_input input{ params };
        QRWA::VoteGovParams_output output;
        invokeUserProcedure(QRWA_CONTRACT_INDEX, QRWA_PROC_VOTE_GOV_PARAMS, input, output, from, 0);
        return output.status;
    }

    QRWA::CreateAssetReleasePoll_output createAssetReleasePoll(const id& from, const QRWA::CreateAssetReleasePoll_input& input)
    {
        QRWA::CreateAssetReleasePoll_output output;
        memset(&output, 0, sizeof(output));
        invokeUserProcedure(QRWA_CONTRACT_INDEX, QRWA_PROC_CREATE_ASSET_RELEASE_POLL, input, output, from, 0);
        return output;
    }

    uint64 voteAssetRelease(const id& from, uint64 pollId, uint64 option)
    {
        QRWA::VoteAssetRelease_input input{ pollId, option };
        QRWA::VoteAssetRelease_output output;
        invokeUserProcedure(QRWA_CONTRACT_INDEX, QRWA_PROC_VOTE_ASSET_RELEASE, input, output, from, 0);
        return output.status;
    }

    uint64 depositGeneralAsset(const id& from, const Asset& asset, uint64 amount)
    {
        QRWA::DepositGeneralAsset_input input{ asset, amount };
        QRWA::DepositGeneralAsset_output output;
        invokeUserProcedure(QRWA_CONTRACT_INDEX, QRWA_PROC_DEPOSIT_GENERAL_ASSET, input, output, from, 0);
        return output.status;
    }

    QRWA::RevokeAssetManagementRights_output revokeAssetManagementRights(const id& from, const Asset& asset, sint64 numberOfShares)
    {
        QRWA::RevokeAssetManagementRights_input input;
        input.asset = asset;
        input.numberOfShares = numberOfShares;

        QRWA::RevokeAssetManagementRights_output output;
        memset(&output, 0, sizeof(output));

        invokeUserProcedure(QRWA_CONTRACT_INDEX, QRWA_PROC_REVOKE_ASSET, input, output, from, QRWA_RELEASE_MANAGEMENT_FEE);
        return output;
    }

    // QRWA Wrappers

    QRWA::QRWAGovParams getGovParams()
    {
        QRWA::GetGovParams_input input;
        QRWA::GetGovParams_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_GOV_PARAMS, input, output);
        return output.params;
    }

    QRWA::GetGovPoll_output getGovPoll(uint64 pollId)
    {
        QRWA::GetGovPoll_input input{ pollId };
        QRWA::GetGovPoll_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_GOV_POLL, input, output);
        return output;
    }

    QRWA::GetAssetReleasePoll_output getAssetReleasePoll(uint64 pollId)
    {
        QRWA::GetAssetReleasePoll_input input{ pollId };
        QRWA::GetAssetReleasePoll_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_ASSET_RELEASE_POLL, input, output);
        return output;
    }

    uint64 getTreasuryBalance()
    {
        QRWA::GetTreasuryBalance_input input;
        QRWA::GetTreasuryBalance_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_TREASURY_BALANCE, input, output);
        return output.balance;
    }

    QRWA::GetDividendBalances_output getDividendBalances()
    {
        QRWA::GetDividendBalances_input input;
        QRWA::GetDividendBalances_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_DIVIDEND_BALANCES, input, output);
        return output;
    }

    QRWA::GetTotalDistributed_output getTotalDistributed()
    {
        QRWA::GetTotalDistributed_input input;
        QRWA::GetTotalDistributed_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_TOTAL_DISTRIBUTED, input, output);
        return output;
    }

    void issueContractSharesHelper(unsigned int contractIndex, std::vector<std::pair<m256i, unsigned int>>& shares)
    {
        issueContractShares(contractIndex, shares);
    }

    void createQswapPool(const id& source, const id& assetIssuer, uint64 assetName, sint64 fee)
    {
        QSWAP::CreatePool_input input{ assetIssuer, assetName };
        QSWAP::CreatePool_output output;
        invokeUserProcedure(QSWAP_CONTRACT_INDEX, 3, input, output, source, fee);
    }

    void getQswapFees(QSWAP::Fees_output& output)
    {
        QSWAP::Fees_input input;
        callFunction(QSWAP_CONTRACT_INDEX, 1, input, output);
    }

    void runQswapEndTick()
    {
        callSystemProcedure(QSWAP_CONTRACT_INDEX, END_TICK);
    }

};

TEST(ContractQRWA, QswapDividend_PoolB)
{
    ContractTestingQRWA qrwa;

    // Create QRWA Shareholders
    const id QRWA_SH1 = id::randomValue();
    increaseEnergy(QRWA_SH1, 100000000);

    std::vector<std::pair<m256i, unsigned int>> qrwaShares{
        {QRWA_SH1, NUMBER_OF_COMPUTORS}
    };

    qrwa.issueContractSharesHelper(QRWA_CONTRACT_INDEX, qrwaShares);

    //create QMINE Shareholders
    const id QMINE_HOLDER = id::randomValue();
    increaseEnergy(QMINE_HOLDER, 100000000);
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 1000000);
    qrwa.transferAsset(QMINE_ISSUER, QMINE_HOLDER, QMINE_ASSET, 1000000);


    // Create QSWAP Shares and deposit them to QRWA
    // QRWA owns 100 shares. Random holder owns the rest (576)
    const id QSWAP_OTHER_HOLDER = id::randomValue();
    std::vector<std::pair<m256i, unsigned int>> qswapShares{
        {id(QRWA_CONTRACT_INDEX, 0, 0, 0), 100},
        {QSWAP_OTHER_HOLDER, 576}
    };
    qrwa.issueContractSharesHelper(QSWAP_CONTRACT_INDEX, qswapShares);

    // now generate Revenue in QSWAP

    const id TRADER = id::randomValue();
    const id ASSET_ISSUER = id::randomValue();
    const uint64 ASSET_NAME = assetNameFromString("TSTCOIN");

    increaseEnergy(TRADER, 10000000000);
    increaseEnergy(ASSET_ISSUER, 10000000000);

    QSWAP::Fees_output qswapFees;
    qrwa.getQswapFees(qswapFees);

    // issue asset on QX
    qrwa.issueAsset(ASSET_ISSUER, ASSET_NAME, 1000000000);

    // Create Pool on QSWAP
    // This generates 'poolCreationFee' for QSWAP shareholders, generating substantial revenue
    qrwa.createQswapPool(ASSET_ISSUER, ASSET_ISSUER, ASSET_NAME, qswapFees.poolCreationFee);

    // We skip AddLiquidity/Swap expectations as the pool creation fee
    // alone is sufficient to test dividend routing

    uint64 totalShareholderRevenue = qswapFees.poolCreationFee;

    // Dividend Distribution

    // Get QRWA dividend balances BEFORE
    auto qrwaDivsBefore = qrwa.getDividendBalances();
    EXPECT_EQ(qrwaDivsBefore.revenuePoolA, 0);
    EXPECT_EQ(qrwaDivsBefore.revenuePoolB, 0);

    // Run END_TICK for QSWAP to distribute dividends
    qrwa.runQswapEndTick();

    // Calculate expected dividend for QRWA (100 shares)
    // (TotalRevenue / 676) * 100
    uint64 expectedDividend = totalShareholderRevenue / NUMBER_OF_COMPUTORS * 100;

    // Get QRWA Dividend Balances AFTER
    auto qrwaDivsAfter = qrwa.getDividendBalances();

    // Verify Dividend Routing
    // Pool A should be 0 (Only QUTIL transfers go here)
    EXPECT_EQ(qrwaDivsAfter.revenuePoolA, 0);

    // Pool B should contain the dividend from QSWAP
    EXPECT_EQ(qrwaDivsAfter.revenuePoolB, expectedDividend);
}


TEST(ContractQRWA, Initialization)
{
    ContractTestingQRWA qrwa;

    // Check gov params (set in test constructor)
    auto params = qrwa.getGovParams();
    EXPECT_EQ(params.mAdminAddress, ADMIN_ADDRESS);
    EXPECT_EQ(params.qmineDevAddress, QMINE_DEV_ADDR_TEST);
    EXPECT_EQ(params.electricityAddress, FEE_ADDR_E);
    EXPECT_EQ(params.maintenanceAddress, FEE_ADDR_M);
    EXPECT_EQ(params.reinvestmentAddress, FEE_ADDR_R);
    EXPECT_EQ(params.electricityPercent, 350);
    EXPECT_EQ(params.maintenancePercent, 50);
    EXPECT_EQ(params.reinvestmentPercent, 100);

    // Check pools and balances via public functions
    EXPECT_EQ(qrwa.getTreasuryBalance(), 0);
    auto divBalances = qrwa.getDividendBalances();
    EXPECT_EQ(divBalances.revenuePoolA, 0);
    EXPECT_EQ(divBalances.revenuePoolB, 0);
    EXPECT_EQ(divBalances.qmineDividendPool, 0);
    EXPECT_EQ(divBalances.qrwaDividendPool, 0);

    auto distTotals = qrwa.getTotalDistributed();
    EXPECT_EQ(distTotals.totalQmineDistributed, 0);
    EXPECT_EQ(distTotals.totalQRWADistributed, 0);
}


TEST(ContractQRWA, RevenueAccounting_POST_INCOMING_TRANSFER)
{
    ContractTestingQRWA qrwa;

    // Pool A from SC QUTIL
    // We simulate this by calling QUTIL's SendToMany
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 1000000);

    auto divBalances = qrwa.getDividendBalances();
    EXPECT_EQ(divBalances.revenuePoolA, 1000000);
    // We cannot test pool B as the test environment does not support standard transfer
    // as noted in contract_testex.cpp
    EXPECT_EQ(divBalances.revenuePoolB, 0);
}

TEST(ContractQRWA, Governance_VoteGovParams_And_EndEpochCount)
{
    ContractTestingQRWA qrwa;

    // Issue QMINE, distribute, and run BEGIN_EPOCH
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 1000000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 1000000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, 400000); // 40%
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_A, QX_CONTRACT_INDEX },
        { HOLDER_A, QX_CONTRACT_INDEX }), 400000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 600000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_B, QMINE_ASSET, 300000); // 30%
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_B, QX_CONTRACT_INDEX },
        { HOLDER_B, QX_CONTRACT_INDEX }), 300000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 300000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_C, QMINE_ASSET, 200000); // 20%
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_C, QX_CONTRACT_INDEX },
        { HOLDER_C, QX_CONTRACT_INDEX }), 200000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 100000);

    increaseEnergy(HOLDER_A, 1000000);
    increaseEnergy(HOLDER_B, 1000000);
    increaseEnergy(HOLDER_C, 1000000);
    increaseEnergy(USER_D, 1000000);

    qrwa.beginEpoch();
    // Quorum should be 2/3 of 900,000 = 600,000

    // Not a holder
    EXPECT_EQ(qrwa.voteGovParams(USER_D, {}), QRWA_STATUS_FAILURE_NOT_AUTHORIZED);

    // Invalid params (Admin NULL_ID)
    QRWA::QRWAGovParams invalidParams = qrwa.getGovParams();
    invalidParams.mAdminAddress = NULL_ID;
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_A, invalidParams), QRWA_STATUS_FAILURE_INVALID_INPUT);

    // Create new poll and vote for it
    QRWA::QRWAGovParams paramsA = qrwa.getGovParams();
    paramsA.electricityPercent = 100; // Change one param

    EXPECT_EQ(qrwa.voteGovParams(HOLDER_A, paramsA), QRWA_STATUS_SUCCESS); // Poll 0
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_B, paramsA), QRWA_STATUS_SUCCESS); // Vote for Poll 0

    // Change vote
    QRWA::QRWAGovParams paramsB = qrwa.getGovParams();
    paramsB.maintenancePercent = 100; // Change another param

    EXPECT_EQ(qrwa.voteGovParams(HOLDER_A, paramsB), QRWA_STATUS_SUCCESS); // Poll 1

    // Mid-epoch sale
    qrwa.transferAsset(HOLDER_B, USER_D, QMINE_ASSET, 150000); // B's balance is now 150k
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { USER_D, QX_CONTRACT_INDEX },
        { USER_D, QX_CONTRACT_INDEX }), 150000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_B, QX_CONTRACT_INDEX },
        { HOLDER_B, QX_CONTRACT_INDEX }), 150000);


    // Accountant at END_EPOCH
    qrwa.endEpoch();

    // Check results:
    // Poll 0 (ParamsA): HOLDER_B voted. Begin=300k, End=150k. VotingPower = 150k.
    // Poll 1 (ParamsB): HOLDER_A voted. Begin=400k, End=400k. VotingPower = 400k.
    // Total power = 900k. Quorum = 600k.

    // Poll 0 (ParamsA) failed.
    auto poll0 = qrwa.getGovPoll(0);
    EXPECT_EQ(poll0.status, QRWA_STATUS_SUCCESS);
    EXPECT_EQ(poll0.proposal.score, 150000);
    EXPECT_EQ(poll0.proposal.status, QRWA_POLL_STATUS_FAILED_VOTE);

    // Poll 1 (ParamsB) failed.
    auto poll1 = qrwa.getGovPoll(1);
    EXPECT_EQ(poll1.status, QRWA_STATUS_SUCCESS);
    EXPECT_EQ(poll1.proposal.score, 400000);
    EXPECT_EQ(poll1.proposal.status, QRWA_POLL_STATUS_FAILED_VOTE);

    // Params should be unchanged (still 50 from init)
    EXPECT_EQ(qrwa.getGovParams().maintenancePercent, 50);

    // New Epoch: Test successful vote
    qrwa.beginEpoch(); // New snapshot total: A(400k) + B(150k) + C(200k) = 750k. Quorum = 500k.

    // All holders vote for ParamsB
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_A, paramsB), QRWA_STATUS_SUCCESS); // Creates Poll 2
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_B, paramsB), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_C, paramsB), QRWA_STATUS_SUCCESS);

    qrwa.endEpoch();

    // Check results:
    // Poll 2 (ParamsB): A(400k) + B(150k) + C(200k) = 750k vote power.
    // Vote passes.
    auto poll2 = qrwa.getGovPoll(2);
    EXPECT_EQ(poll2.status, QRWA_STATUS_SUCCESS);
    EXPECT_EQ(poll2.proposal.score, 750000);
    EXPECT_EQ(poll2.proposal.status, QRWA_POLL_STATUS_PASSED_EXECUTED);

    // Verify params were updated
    EXPECT_EQ(qrwa.getGovParams().maintenancePercent, 100);
}

TEST(ContractQRWA, Governance_AssetReleasePolls)
{
    ContractTestingQRWA qrwa;

    increaseEnergy(HOLDER_A, 1000000);
    increaseEnergy(HOLDER_B, 1000000);
    increaseEnergy(USER_D, 1000000);
    increaseEnergy(DESTINATION_ADDR, 1000000);

    // Issue QMINE, distribute, and run BEGIN_EPOCH
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 1000000 + 1000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 1001000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, 700000); // 70%
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_A, QX_CONTRACT_INDEX },
        { HOLDER_A, QX_CONTRACT_INDEX }), 700000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 301000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_B, QMINE_ASSET, 300000); // 30%
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_B, QX_CONTRACT_INDEX },
        { HOLDER_B, QX_CONTRACT_INDEX }), 300000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 1000);
    // QMINE_ISSUER (ADMIN_ADDRESS) now holds 1000

    // Give SC 1000 QMINE for its treasury
    qrwa.transferManagementRights(QMINE_ISSUER, QMINE_ASSET, 1000, QRWA_CONTRACT_INDEX);
    EXPECT_EQ(qrwa.donateToTreasury(QMINE_ISSUER, 1000), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.getTreasuryBalance(), 1000);

    qrwa.beginEpoch();

    // Not Admin
    QRWA::CreateAssetReleasePoll_input pollInput = {};
    pollInput.proposalName = id::randomValue();
    pollInput.asset = QMINE_ASSET;
    pollInput.amount = 100;
    pollInput.destination = DESTINATION_ADDR;

    auto pollOut = qrwa.createAssetReleasePoll(HOLDER_A, pollInput); // HOLDER_A is not admin
    EXPECT_EQ(pollOut.status, QRWA_STATUS_FAILURE_NOT_AUTHORIZED);

    // Admin creates poll
    pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput);
    EXPECT_EQ(pollOut.status, QRWA_STATUS_SUCCESS);
    EXPECT_EQ(pollOut.proposalId, 0);

    // Not a holder
    EXPECT_EQ(qrwa.voteAssetRelease(USER_D, 0, 1), QRWA_STATUS_FAILURE_NOT_AUTHORIZED);

    // Holders vote
    EXPECT_EQ(qrwa.voteAssetRelease(HOLDER_A, 0, 1), QRWA_STATUS_SUCCESS); // 700k YES
    EXPECT_EQ(qrwa.voteAssetRelease(HOLDER_B, 0, 0), QRWA_STATUS_SUCCESS); // 300k NO

    // Add revenue to Pool A so the contract can pay the release fee
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 1000000);
    EXPECT_EQ(qrwa.getDividendBalances().revenuePoolA, 1000000);

    // Count at end epoch (Pass)
    qrwa.endEpoch();

    auto poll = qrwa.getAssetReleasePoll(0);
    EXPECT_EQ(poll.status, QRWA_STATUS_SUCCESS);
    EXPECT_EQ(poll.proposal.status, QRWA_POLL_STATUS_PASSED_EXECUTED); // Should pass now
    EXPECT_EQ(poll.proposal.votesYes, 700000);
    EXPECT_EQ(poll.proposal.votesNo, 300000);

    // Verify balances
    EXPECT_EQ(qrwa.getTreasuryBalance(), 900); // 1000 - 100
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { DESTINATION_ADDR, QX_CONTRACT_INDEX },
        { DESTINATION_ADDR, QX_CONTRACT_INDEX }), 100); // Should be 100 now

    // Count at end epoch (Fail Vote)
    qrwa.beginEpoch();
    pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput); // Poll 1
    EXPECT_EQ(qrwa.voteAssetRelease(HOLDER_A, 1, 0), QRWA_STATUS_SUCCESS); // 700k NO
    EXPECT_EQ(qrwa.voteAssetRelease(HOLDER_B, 1, 1), QRWA_STATUS_SUCCESS); // 300k YES
    qrwa.endEpoch();

    poll = qrwa.getAssetReleasePoll(1);
    EXPECT_EQ(poll.proposal.status, QRWA_POLL_STATUS_FAILED_VOTE);
    EXPECT_EQ(qrwa.getTreasuryBalance(), 900); // Unchanged

    // Count at end epoch (Fail Execution - Insufficient)
    qrwa.beginEpoch();
    pollInput.amount = 1000; // Try to release 1000 (only 900 left)
    pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput); // Poll 2
    EXPECT_EQ(qrwa.voteAssetRelease(HOLDER_A, 2, 1), QRWA_STATUS_SUCCESS); // 700k YES
    qrwa.endEpoch();

    poll = qrwa.getAssetReleasePoll(2);
    EXPECT_EQ(poll.proposal.status, QRWA_POLL_STATUS_PASSED_FAILED_EXECUTION);
    EXPECT_EQ(qrwa.getTreasuryBalance(), 900); // Unchanged
}

TEST(ContractQRWA, Governance_AssetRelease_FailAndRevoke)
{
    ContractTestingQRWA qrwa;

    const sint64 initialEnergy = 1000000000;
    increaseEnergy(HOLDER_A, initialEnergy);
    increaseEnergy(HOLDER_B, initialEnergy);
    increaseEnergy(ADMIN_ADDRESS, initialEnergy + QX_ISSUE_ASSET_FEE);
    increaseEnergy(DESTINATION_ADDR, initialEnergy);

    const sint64 treasuryAmount = 1000;
    const sint64 voterShares = 1000000;
    const sint64 releaseAmount = 500;

    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, voterShares + treasuryAmount);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, 700000);
    qrwa.transferAsset(QMINE_ISSUER, HOLDER_B, QMINE_ASSET, 300000);

    // Give qRWA management rights over the treasury shares
    qrwa.transferManagementRights(QMINE_ISSUER, QMINE_ASSET, treasuryAmount, QRWA_CONTRACT_INDEX);

    // Verify management rights were transferred
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QRWA_CONTRACT_INDEX },
                                          { QMINE_ISSUER, QRWA_CONTRACT_INDEX }), treasuryAmount);

    // Donate the shares to the treasury
    EXPECT_EQ(qrwa.donateToTreasury(QMINE_ISSUER, treasuryAmount), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.getTreasuryBalance(), treasuryAmount);

    // Verify Revenue Pool A (for fees) is empty.
    auto divBalances = qrwa.getDividendBalances();
    EXPECT_EQ(divBalances.revenuePoolA, 0);

    qrwa.beginEpoch();
    // Total voting power = 1,000,000 (HOLDER_A + HOLDER_B)
    // Quorum = (1,000,000 * 2 / 3) + 1 = 666,667

    QRWA::CreateAssetReleasePoll_input pollInput = {};
    pollInput.proposalName = id::randomValue();
    pollInput.asset = QMINE_ASSET;
    pollInput.amount = releaseAmount;
    pollInput.destination = DESTINATION_ADDR;

    // create poll
    auto pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput);
    EXPECT_EQ(pollOut.status, QRWA_STATUS_SUCCESS);
    uint64 pollId = pollOut.proposalId;

    // HOLDER_A votes YES, passing the poll (700k > 666k quorum)
    EXPECT_EQ(qrwa.voteAssetRelease(HOLDER_A, pollId, 1), QRWA_STATUS_SUCCESS);

    qrwa.endEpoch();

    // Check poll status
    // It should have passed the vote but failed execution (due to lack of 100 QUs fee for QX management transfer)
    auto poll = qrwa.getAssetReleasePoll(pollId);
    EXPECT_EQ(poll.proposal.status, QRWA_POLL_STATUS_PASSED_FAILED_EXECUTION);
    EXPECT_EQ(poll.proposal.votesYes, 700000);

    // Check SC asset state
    // Asserts the INTERNAL counter is now decreased
    EXPECT_EQ(qrwa.getTreasuryBalance(), treasuryAmount - releaseAmount); // 1000 - 500 = 500

    // the SC balance is decreased
    sint64 scOwnedBalance = numberOfShares(QMINE_ASSET,
        { id(QRWA_CONTRACT_INDEX, 0, 0, 0), QRWA_CONTRACT_INDEX },
        { id(QRWA_CONTRACT_INDEX, 0, 0, 0), QRWA_CONTRACT_INDEX });
    EXPECT_EQ(scOwnedBalance, treasuryAmount - releaseAmount); // 1000 - 500 = 500

    // DESTINATION_ADDR should now owns the shares, but they are MANAGED by qRWA
    sint64 destManagedByQrwa = numberOfShares(QMINE_ASSET,
        { DESTINATION_ADDR, QRWA_CONTRACT_INDEX },
        { DESTINATION_ADDR, QRWA_CONTRACT_INDEX });
    EXPECT_EQ(destManagedByQrwa, releaseAmount); // 500 shares are stuck

    // DESTINATION_ADDR should have 0 shares managed by QX
    sint64 destManagedByQx = numberOfShares(QMINE_ASSET,
        { DESTINATION_ADDR, QX_CONTRACT_INDEX },
        { DESTINATION_ADDR, QX_CONTRACT_INDEX });
    EXPECT_EQ(destManagedByQx, 0);

    // Test Revoke
    qrwa.beginEpoch();

    // Fund DESTINATION_ADDR with the fee for the revoke procedure
    increaseEnergy(DESTINATION_ADDR, QRWA_RELEASE_MANAGEMENT_FEE);
    sint64 destBalanceBeforeRevoke = getBalance(DESTINATION_ADDR);

    // DESTINATION_ADDR calls revokeAssetManagementRights
    auto revokeOut = qrwa.revokeAssetManagementRights(DESTINATION_ADDR, QMINE_ASSET, releaseAmount);

    // check the outcome
    EXPECT_EQ(revokeOut.status, QRWA_STATUS_SUCCESS);
    EXPECT_EQ(revokeOut.transferredNumberOfShares, releaseAmount);

    // check final on-chain asset state
    // DESTINATION_ADDR should be no longer have shares managed by qRWA
    destManagedByQrwa = numberOfShares(QMINE_ASSET,
        { DESTINATION_ADDR, QRWA_CONTRACT_INDEX },
        { DESTINATION_ADDR, QRWA_CONTRACT_INDEX });
    EXPECT_EQ(destManagedByQrwa, 0);

    // DESTINATION_ADDR's shares should now be managed by QX
    destManagedByQx = numberOfShares(QMINE_ASSET,
        { DESTINATION_ADDR, QX_CONTRACT_INDEX },
        { DESTINATION_ADDR, QX_CONTRACT_INDEX });
    EXPECT_EQ(destManagedByQx, releaseAmount);

    // check if the fee was paid by the user
    sint64 destBalanceAfterRevoke = getBalance(DESTINATION_ADDR);
    EXPECT_EQ(destBalanceAfterRevoke, destBalanceBeforeRevoke - QRWA_RELEASE_MANAGEMENT_FEE);

    // Critical check:
    // Verify that the fee sent to the SC was NOT permanently added to Pool B.
    // The POST_INCOMING_TRANSFER adds 100 QU to Pool B.
    // The procedure executes, spends 100 QU to QX, and logic must subtract 100 from Pool B.
    // Net result for Pool B must be 0.
    auto finalDivBalances = qrwa.getDividendBalances();
    EXPECT_EQ(finalDivBalances.revenuePoolB, 0);
}

TEST(ContractQRWA, Treasury_Donation)
{
    ContractTestingQRWA qrwa;

    // Issue QMINE to the temporary treasury holder
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 150000000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 150000000);

    qrwa.transferAsset(QMINE_ISSUER, TREASURY_HOLDER, QMINE_ASSET, 150000000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { TREASURY_HOLDER, QX_CONTRACT_INDEX },
        { TREASURY_HOLDER, QX_CONTRACT_INDEX }), 150000000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 0);

    increaseEnergy(TREASURY_HOLDER, 1000000);

    // Fail (No Management Rights)
    EXPECT_EQ(qrwa.donateToTreasury(TREASURY_HOLDER, 1000), QRWA_STATUS_FAILURE_INSUFFICIENT_BALANCE);

    // Success (With Management Rights)
    // Give SC management rights
    qrwa.transferManagementRights(TREASURY_HOLDER, QMINE_ASSET, 150000000, QRWA_CONTRACT_INDEX);

    // Verify rights
    sint64 managedBalance = numberOfShares(QMINE_ASSET,
        { TREASURY_HOLDER, QRWA_CONTRACT_INDEX },
        { TREASURY_HOLDER, QRWA_CONTRACT_INDEX });
    EXPECT_EQ(managedBalance, 150000000);

    // Donate
    EXPECT_EQ(qrwa.donateToTreasury(TREASURY_HOLDER, 150000000), QRWA_STATUS_SUCCESS);

    // Verify treasury balance in SC
    EXPECT_EQ(qrwa.getTreasuryBalance(), 150000000);

    // Verify SC now owns the shares
    sint64 scOwnedBalance = numberOfShares(QMINE_ASSET,
        { id(QRWA_CONTRACT_INDEX, 0, 0, 0), QRWA_CONTRACT_INDEX },
        { id(QRWA_CONTRACT_INDEX, 0, 0, 0), QRWA_CONTRACT_INDEX });
    EXPECT_EQ(scOwnedBalance, 150000000);
}

TEST(ContractQRWA, Payout_FullDistribution)
{
    ContractTestingQRWA qrwa;

    // Issue QMINE, distribute, and run BEGIN_EPOCH
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 1000000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 1000000);

    increaseEnergy(HOLDER_A, 1000000);
    increaseEnergy(HOLDER_B, 1000000);
    increaseEnergy(HOLDER_C, 1000000);
    increaseEnergy(USER_D, 1000000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, 200000); // Holder A
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_A, QX_CONTRACT_INDEX },
        { HOLDER_A, QX_CONTRACT_INDEX }), 200000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 800000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_B, QMINE_ASSET, 300000); // Holder B
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_B, QX_CONTRACT_INDEX },
        { HOLDER_B, QX_CONTRACT_INDEX }), 300000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 500000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_C, QMINE_ASSET, 100000); // Holder C
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_C, QX_CONTRACT_INDEX },
        { HOLDER_C, QX_CONTRACT_INDEX }), 100000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 400000);

    qrwa.beginEpoch();
    // mTotalQmineBeginEpoch = 1,000,000

    // Mid-epoch transfers
    qrwa.transferAsset(HOLDER_A, USER_D, QMINE_ASSET, 50000); // Holder A ends with 150k
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { USER_D, QX_CONTRACT_INDEX },
        { USER_D, QX_CONTRACT_INDEX }), 50000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_A, QX_CONTRACT_INDEX },
        { HOLDER_A, QX_CONTRACT_INDEX }), 150000);

    qrwa.transferAsset(HOLDER_C, USER_D, QMINE_ASSET, 100000); // Holder C ends with 0
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { USER_D, QX_CONTRACT_INDEX },
        { USER_D, QX_CONTRACT_INDEX }), 150000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_C, QX_CONTRACT_INDEX },
        { HOLDER_C, QX_CONTRACT_INDEX }), 0);

    // Deposit revenue
    // Pool A (from SC)
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 1000000);

    // Pool B (from User) - Untestable. We will proceed using only Pool A.

    qrwa.endEpoch();

    // Set time to payout day
    etalonTick.year = 25; etalonTick.month = 11; etalonTick.day = 7; // A Friday
    etalonTick.hour = 12; etalonTick.minute = 1; etalonTick.second = 0;

    // Use helper to reset payout time
    qrwa.resetPayoutTime(); // Reset time to allow payout

    // Call END_TICK to trigger DistributeRewards
    qrwa.endTick();

    // Verification
    // Fees: Pool A = 1M
    // Elec (35%) = 350,000
    // Maint (5%) = 50,000
    // Reinv (10%) = 100,000
    // Total Fees = 500,000
    EXPECT_EQ(getBalance(FEE_ADDR_E), 350000);
    EXPECT_EQ(getBalance(FEE_ADDR_M), 50000);
    EXPECT_EQ(getBalance(FEE_ADDR_R), 100000);

    // Distribution Pool
    // Y_revenue = 1,000,000 - 500,000 = 500,000
    // totalDistribution = 500,000 (Y) + 0 (B) = 500,000
    // mQmineDividendPool = 500k * 90% = 450,000
    // mQRWADividendPool = 500k * 10% = 50,000

    // qRWA Payout (50,000 QUs)
    uint64 qrwaPerShare = 50000 / NUMBER_OF_COMPUTORS; // 73
    auto distTotals = qrwa.getTotalDistributed();
    EXPECT_EQ(distTotals.totalQRWADistributed, qrwaPerShare * NUMBER_OF_COMPUTORS); // 73 * 676 = 49328

    // QMINE Payout (450,000 QUs)
    // mPayoutTotalQmineBegin = 1,000,000

    // Eligible Balances:
    // H1: min(200k, 150k) = 150,000
    // H2: min(300k, 300k) = 300,000
    // H3: min(100k, 0)   = 0
    // Issuer: min(400k, 400k) = 400,000
    // Total Eligible = 850,000

    // Payouts:
    // H1 Payout: (150,000 * 450,000) / 1,000,000 = 67,500
    // H2 Payout: (300,000 * 450,000) / 1,000,000 = 135,000
    // H3 Payout: 0
    // Issuer Payout: (400,000 * 450,000) / 1,000,000 = 180,000
    // Total Eligible Paid = 67,500 + 135,000 + 180,000 = 382,500
    // QMINE_DEV Payout (Remainder) = 450,000 - 382,500 = 67,500

    EXPECT_EQ(getBalance(HOLDER_A), 1000000 + 67500);
    EXPECT_EQ(getBalance(HOLDER_B), 1000000 + 135000);
    EXPECT_EQ(getBalance(HOLDER_C), 1000000 + 0);
    EXPECT_EQ(getBalance(QMINE_DEV_ADDR_TEST), 67500);

    // Re-check balances
    EXPECT_EQ(getBalance(HOLDER_B), 1000000 + 135000);

    // Check pools are empty (or contain only dust from integer division)
    auto divBalances = qrwa.getDividendBalances();
    EXPECT_EQ(divBalances.revenuePoolA, 0);
    EXPECT_EQ(divBalances.revenuePoolB, 0);
    EXPECT_EQ(divBalances.qmineDividendPool, 0);
    EXPECT_EQ(divBalances.qrwaDividendPool, 50000 - (qrwaPerShare * NUMBER_OF_COMPUTORS)); // Dust
}

TEST(ContractQRWA, Payout_SnapshotLogic)
{
    ContractTestingQRWA qrwa;

    // Give energy to all participants
    increaseEnergy(QMINE_ISSUER, 1000000000);
    increaseEnergy(HOLDER_A, 1000000);
    increaseEnergy(HOLDER_B, 1000000);
    increaseEnergy(HOLDER_C, 1000000);
    increaseEnergy(USER_D, 1000000);

    // Issue 3500 QMINE
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 3500);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX }, { QMINE_ISSUER, QX_CONTRACT_INDEX }), 3500);

    // Epoch 1 Setup: Distribute initial shares
    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, 1000);
    qrwa.transferAsset(QMINE_ISSUER, HOLDER_B, QMINE_ASSET, 1000);
    qrwa.transferAsset(QMINE_ISSUER, HOLDER_C, QMINE_ASSET, 1000);
    // QMINE_ISSUER keeps 500

    qrwa.beginEpoch();
    // Snapshot (Begin Epoch 1):
    // Total: 3500 (A, B, C, Issuer)
    // A: 1000
    // B: 1000
    // C: 1000
    // D: 0
    // Issuer: 500

    // Epoch 1 Mid-Epoch Transfers
    qrwa.transferAsset(HOLDER_A, USER_D, QMINE_ASSET, 500); // A: 500, D: 500
    qrwa.transferAsset(HOLDER_B, USER_D, QMINE_ASSET, 1000); // B: 0, D: 1500
    qrwa.transferAsset(QMINE_ISSUER, HOLDER_C, QMINE_ASSET, 500); // C: 1500, Issuer: 0

    // Deposit 1M QUs into Pool A
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 1000000);

    qrwa.endEpoch();
    // Payout Snapshots (Epoch 1):
    // mPayoutTotalQmineBegin: 3500
    // Eligible:
    // A: min(1000, 500) = 500
    // B: min(1000, 0)   = 0
    // C: min(1000, 1500) = 1000
    // D: (not in begin map) = 0
    // Issuer: min(500, 0) = 0
    // Total Eligible: 1500

    // Payout Calculation (Epoch 1):
    // Pool A: 1,000,000 -> Fees (50%) = 500,000 -> Y_revenue = 500,000
    // mQmineDividendPool (90%): 450,000
    // mQRWADividendPool (10%): 50,000

    // Payouts:
    // A: (500 * 450,000) / 3,500 = 64,285
    // B: 0
    // C: (1000 * 450,000) / 3,500 = 128,571
    // D: 0
    // Issuer: 0
    // totalEligiblePaid = 192,856
    // movedSharesPayout (QMINE_DEV) = 450,000 - 192,856 = 257,144

    // Trigger Payout
    etalonTick.year = 25; etalonTick.month = 11; etalonTick.day = 14; // Next Friday
    etalonTick.hour = 12; etalonTick.minute = 1; etalonTick.second = 0;
    qrwa.resetPayoutTime();
    qrwa.endTick();

    // Verify Payout 1
    EXPECT_EQ(getBalance(HOLDER_A), 1000000 + 64285);
    EXPECT_EQ(getBalance(HOLDER_B), 1000000 + 0);
    EXPECT_EQ(getBalance(HOLDER_C), 1000000 + 128571);
    EXPECT_EQ(getBalance(USER_D), 1000000 + 0);
    EXPECT_EQ(getBalance(QMINE_DEV_ADDR_TEST), 257144);

    // Check C's balance again
    EXPECT_EQ(getBalance(HOLDER_C), 1000000 + 128571);


    // Epoch 2
    qrwa.beginEpoch();
    // Snapshot (Begin Epoch 2):
    // Total: 3500
    // A: 500, B: 0, C: 1500, D: 1500, Issuer: 0

    // Epoch 2 Mid-Epoch Transfers
    qrwa.transferAsset(USER_D, HOLDER_A, QMINE_ASSET, 500); // A: 1000, D: 1000
    qrwa.transferAsset(HOLDER_C, HOLDER_B, QMINE_ASSET, 1000); // C: 500, B: 1000

    // Deposit 1M QUs into Pool A
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 1000000);

    qrwa.endEpoch();
    // Snapshot (End Epoch 2):
    // A: 1000, B: 1000, C: 500, D: 1000, Issuer: 0
    //
    // Payout Snapshots (Epoch 2):
    // mPayoutTotalQmineBegin: 3500
    // Eligible:
    // A: min(500, 1000)  = 500
    // B: min(0, 1000)    = 0
    // C: min(1500, 500)  = 500
    // D: min(1500, 1000) = 1000
    // Total Eligible: 2000

    // Payout Calculation (Epoch 2):
    // Pool A: 1,000,000 -> Fees (50%) = 500,000 -> Y_revenue = 500,000
    // mQmineDividendPool (90%): 450,000
    // Payouts:
    // A: (500 * 450,000) / 3,500 = 64,285
    // B: 0
    // C: (500 * 450,000) / 3,500 = 64,285
    // D: (1000 * 450,000) / 3,500 = 128,571
    // totalEligiblePaid = 257,141
    // movedSharesPayout (QMINE_DEV) = 450,000 - 257,141 = 192,859

    // Trigger Payout 2
    etalonTick.year = 25; etalonTick.month = 11; etalonTick.day = 21; // Next Friday
    etalonTick.hour = 12; etalonTick.minute = 1; etalonTick.second = 0;
    qrwa.resetPayoutTime();
    qrwa.endTick();

    // Verify Payout 2 (Cumulative)
    // A: Base + payout1 + payout2
    EXPECT_EQ(getBalance(HOLDER_A), 1000000 + 64285 + 64285);
    // B: Base + payout1 + payout2
    EXPECT_EQ(getBalance(HOLDER_B), 1000000 + 0 + 0);
    // C: Base + payout1 + payout2
    EXPECT_EQ(getBalance(HOLDER_C), 1000000 + 128571 + 64285);
    // D: Base + payout1 + payout2
    EXPECT_EQ(getBalance(USER_D), 1000000 + 0 + 128571);
    // QMINE dev: payout1 + payout2
    EXPECT_EQ(getBalance(QMINE_DEV_ADDR_TEST), 257144 + 192859);
}

TEST(ContractQRWA, Payout_FullDistribution2)
{
    ContractTestingQRWA qrwa;

    // Issue QMINE, distribute, and run BEGIN_EPOCH
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, 1000000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 1000000);

    increaseEnergy(HOLDER_A, 1000000);
    increaseEnergy(HOLDER_B, 1000000);
    increaseEnergy(HOLDER_C, 1000000);
    increaseEnergy(USER_D, 1000000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, 200000); // Holder A
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_A, QX_CONTRACT_INDEX },
        { HOLDER_A, QX_CONTRACT_INDEX }), 200000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 800000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_B, QMINE_ASSET, 300000); // Holder B
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_B, QX_CONTRACT_INDEX },
        { HOLDER_B, QX_CONTRACT_INDEX }), 300000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 500000);

    qrwa.transferAsset(QMINE_ISSUER, HOLDER_C, QMINE_ASSET, 100000); // Holder C
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_C, QX_CONTRACT_INDEX },
        { HOLDER_C, QX_CONTRACT_INDEX }), 100000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { QMINE_ISSUER, QX_CONTRACT_INDEX },
        { QMINE_ISSUER, QX_CONTRACT_INDEX }), 400000);

    qrwa.beginEpoch();
    // mTotalQmineBeginEpoch = 1,000,000 (A:200k, B:300k, C:100k, Issuer:400k)

    // Mid-epoch transfers
    qrwa.transferAsset(HOLDER_A, USER_D, QMINE_ASSET, 50000); // Holder A ends with 150k
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { USER_D, QX_CONTRACT_INDEX },
        { USER_D, QX_CONTRACT_INDEX }), 50000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_A, QX_CONTRACT_INDEX },
        { HOLDER_A, QX_CONTRACT_INDEX }), 150000);

    qrwa.transferAsset(HOLDER_C, USER_D, QMINE_ASSET, 100000); // Holder C ends with 0
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { USER_D, QX_CONTRACT_INDEX },
        { USER_D, QX_CONTRACT_INDEX }), 150000);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { HOLDER_C, QX_CONTRACT_INDEX },
        { HOLDER_C, QX_CONTRACT_INDEX }), 0);

    // Deposit revenue
    // Pool A (from SC)
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 3000000); // Increased revenue

    // Pool B (from User): Untestable. We will proceed using only Pool A.

    qrwa.endEpoch();

    // Set time to payout day
    etalonTick.year = 25; etalonTick.month = 11; etalonTick.day = 7; // A Friday
    etalonTick.hour = 12; etalonTick.minute = 1; etalonTick.second = 0;

    // Use helper to reset payout time
    qrwa.resetPayoutTime(); // Reset time to allow payout

    // Call END_TICK to trigger DistributeRewards
    qrwa.endTick();

    // Verification
    // Fees: Pool A = 3M
    // Elec (35%) = 1,050,000
    // Maint (5%) = 150,000
    // Reinv (10%) = 300,000
    // Total Fees = 1,500,000
    EXPECT_EQ(getBalance(FEE_ADDR_E), 1050000);
    EXPECT_EQ(getBalance(FEE_ADDR_M), 150000);
    EXPECT_EQ(getBalance(FEE_ADDR_R), 300000);

    // Distribution Pool
    // Y_revenue = 3,000,000 - 1,500,000 = 1,500,000
    // totalDistribution = 1,500,000 (Y) + 0 (B) = 1,500,000
    // mQmineDividendPool = 1.5M * 90% = 1,350,000
    // mQRWADividendPool = 1.5M * 10% = 150,000

    // qRWA Payout (150,000 QUs)
    uint64 qrwaPerShare = 150000 / NUMBER_OF_COMPUTORS; // 150000 / 676 = 221
    auto distTotals = qrwa.getTotalDistributed();
    EXPECT_EQ(distTotals.totalQRWADistributed, qrwaPerShare * NUMBER_OF_COMPUTORS); // 221 * 676 = 149416

    // QMINE Payout (1,350,000 QUs)
    // mPayoutTotalQmineBegin = 1,000,000 (A:200k, B:300k, C:100k, Issuer:400k)

    // Eligible:
    // H1: min(200k, 150k) = 150,000
    // H2: min(300k, 300k) = 300,000
    // H3: min(100k, 0)   = 0
    // Issuer: min(400k, 400k) = 400,000
    // Total Eligible = 850,000

    // Payouts:
    // H1 Payout: (150,000 * 1,350,000) / 1,000,000 = 202,500
    // H2 Payout: (300,000 * 1,350,000) / 1,000,000 = 405,000
    // H3 Payout: 0
    // Issuer Payout: (400,000 * 1,350,000) / 1,000,000 = 540,000
    // Total Eligible Paid = 202,500 + 405,000 + 540,000 = 1,147,500
    // QMINE dev Payout (Remainder) = 1,350,000 - 1,147,500 = 202,500

    EXPECT_EQ(getBalance(HOLDER_A), 1000000 + 202500);
    EXPECT_EQ(getBalance(HOLDER_B), 1000000 + 405000);
    EXPECT_EQ(getBalance(HOLDER_C), 1000000 + 0);
    EXPECT_EQ(getBalance(QMINE_DEV_ADDR_TEST), 202500);

    // Re-check B's balance
    EXPECT_EQ(getBalance(HOLDER_B), 1000000 + 405000);


    // Check pools are empty (or contain only dust from integer division)
    auto divBalances = qrwa.getDividendBalances();
    EXPECT_EQ(divBalances.revenuePoolA, 0);
    EXPECT_EQ(divBalances.revenuePoolB, 0);
    EXPECT_EQ(divBalances.qmineDividendPool, 0); // QMINE dev gets the remainder
    EXPECT_EQ(divBalances.qrwaDividendPool, 150000 - (qrwaPerShare * NUMBER_OF_COMPUTORS)); // Dust (584)
}

TEST(ContractQRWA, FullScenario_DividendsAndGovernance)
{
    ContractTestingQRWA qrwa;

    /* --- SETUP --- */

    etalonTick.year = 25;   // 2025
    etalonTick.month = 11;  // November
    etalonTick.day = 7;     // 7th (Friday)
    etalonTick.hour = 12;
    etalonTick.minute = 1;
    etalonTick.second = 0;
    etalonTick.millisecond = 0;

    // Helper to handle month rollovers for this test
    auto advanceTime7Days = [&]()
	{
		etalonTick.day += 7;
		// Simple logic for Nov/Dec 2025
		if (etalonTick.month == 11 && etalonTick.day > 30) {
			etalonTick.day -= 30;
			etalonTick.month++;
		}
		else if (etalonTick.month == 12 && etalonTick.day > 31) {
			etalonTick.day -= 31;
			etalonTick.month = 1;
			etalonTick.year++;
		}
	};

    // Constants
    const sint64 TOTAL_SUPPLY = 1000000000000LL; // 1,000,000,000,000 = 1 Trillion
    const sint64 TREASURY_INIT = 150000000000LL; // 150 Billion
    const sint64 SHAREHOLDERS_TOTAL = 850000000000LL; // 850 Billion
    const sint64 SHAREHOLDER_AMT = SHAREHOLDERS_TOTAL / 5; // 170 Billion each
    const sint64 REVENUE_AMT = 10000000LL; // 10 Million QUs per epoch revenue

    // Known Pool Amounts derived from REVENUE_AMT and 50% total fees
    // Revenue 10M -> Fees 5M -> Net 5M
    const sint64 QMINE_POOL_AMT = 4500000LL; // 90% of 5M
    const sint64 QRWA_POOL_AMT_BASE = 500000LL; // 10% of 5M

    const sint64 QRWA_TOTAL_SHARES = 676LL;

    // Track dust for qRWA pool to calculate accurate rates per epoch
    sint64 currentQrwaDust = 0;
    sint64 currentQXReleaseFee = 0;

    auto getQrwaRateForEpoch = [&](sint64 poolAmount) -> sint64 {
        sint64 totalPool = poolAmount + currentQrwaDust;
        sint64 rate = totalPool / QRWA_TOTAL_SHARES;
        currentQrwaDust = totalPool % QRWA_TOTAL_SHARES; // Update dust for next epoch
        return rate;
        };

    // Entities
    const id S1 = id::randomValue(); // Hybrid: Holds QMINE + qRWA shares
    const id S2 = id::randomValue(); // Control QMINE: Holds only QMINE
    const id S3 = id::randomValue(); // QMINE only
    const id S4 = id::randomValue(); // QMINE only
    const id S5 = id::randomValue(); // QMINE only
    const id Q1 = id::randomValue(); // Control qRWA: Holds only qRWA shares
    const id Q2 = id::randomValue(); // qRWA only

    // Energy Funding
    increaseEnergy(QMINE_ISSUER, QX_ISSUE_ASSET_FEE * 2 + 100000000);
    increaseEnergy(TREASURY_HOLDER, 100000000);
    increaseEnergy(S1, 100000000);
    increaseEnergy(S2, 100000000);
    increaseEnergy(S3, 100000000);
    increaseEnergy(S4, 100000000);
    increaseEnergy(S5, 100000000);
    increaseEnergy(Q1, 100000000);
    increaseEnergy(Q2, 100000000);
    increaseEnergy(DESTINATION_ADDR, 1000000);
    increaseEnergy(ADMIN_ADDRESS, 1000000);

    // Issue QMINE
    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, TOTAL_SUPPLY);

    // Distribute to Treasury Holder
    qrwa.transferAsset(QMINE_ISSUER, TREASURY_HOLDER, QMINE_ASSET, TREASURY_INIT);

    // Distribute to 5 Shareholders (170B each)
    qrwa.transferAsset(QMINE_ISSUER, S1, QMINE_ASSET, SHAREHOLDER_AMT);
    qrwa.transferAsset(QMINE_ISSUER, S2, QMINE_ASSET, SHAREHOLDER_AMT);
    qrwa.transferAsset(QMINE_ISSUER, S3, QMINE_ASSET, SHAREHOLDER_AMT);
    qrwa.transferAsset(QMINE_ISSUER, S4, QMINE_ASSET, SHAREHOLDER_AMT);
    qrwa.transferAsset(QMINE_ISSUER, S5, QMINE_ASSET, SHAREHOLDER_AMT);

    // Issue and Distribute qrwa Contract Shares
    std::vector<std::pair<m256i, unsigned int>> qrwaShares{
            {S1, 200},
            {Q1, 200},
            {Q2, 276}
    };
    issueContractShares(QRWA_CONTRACT_INDEX, qrwaShares);

    // Snapshot balances
    std::map<id, sint64> prevBalances;
    auto snapshotBalances = [&]() {
        prevBalances[S1] = getBalance(S1);
        prevBalances[S2] = getBalance(S2);
        prevBalances[S3] = getBalance(S3);
        prevBalances[S4] = getBalance(S4);
        prevBalances[S5] = getBalance(S5);
        prevBalances[Q1] = getBalance(Q1);
        prevBalances[Q2] = getBalance(Q2);
        prevBalances[DESTINATION_ADDR] = getBalance(DESTINATION_ADDR);
        };
    snapshotBalances();

    // Helper to calculate exact QMINE payout matching contract logic
    // Payout = (EligibleBalance * DividendPool) / PayoutBase
    auto calculateQminePayout = [&](sint64 balance, sint64 payoutBase, sint64 poolAmount) -> sint64 {
        if (payoutBase == 0) return 0;
        // Contract uses: div<uint128>((uint128)balance * pool, totalEligible)
        // We mimic that integer math here
        uint128 res = (uint128)balance * (uint128)poolAmount;
        res = res / (uint128)payoutBase;
        return (sint64)res.low;
        };

    // Helper that uses the calculated rate for the current epoch
    auto calculateQrwaPayout = [&](sint64 shares, sint64 currentRate) -> sint64 {
        return shares * currentRate;
        };

#if ENABLE_BALANCE_DEBUG
    auto print_balances = [&]()
        {
            std::cout << "\n--- Current Balances ---" << std::endl;
            std::cout << "S1: " << getBalance(S1) << std::endl;
            std::cout << "S2: " << getBalance(S2) << std::endl;
            std::cout << "S3: " << getBalance(S3) << std::endl;
            std::cout << "S4: " << getBalance(S4) << std::endl;
            std::cout << "S5: " << getBalance(S5) << std::endl;
            std::cout << "Q1: " << getBalance(Q1) << std::endl;
            std::cout << "Q2: " << getBalance(Q2) << std::endl;
            std::cout << "Dest: " << getBalance(DESTINATION_ADDR) << std::endl;
            std::cout << "Treasury: " << getBalance(TREASURY_HOLDER) << std::endl;
            std::cout << "Dev: " << getBalance(QMINE_DEV_ADDR_TEST) << std::endl;
            std::cout << "------------------------\n" << std::endl;
        };

    std::cout << "PRE-EPOCH 1\n";
    print_balances();
#endif
    // epoch 1
    qrwa.beginEpoch();

    //Shareholders Exchange
    qrwa.transferAsset(S1, S2, QMINE_ASSET, 10000000000LL);
    qrwa.transferAsset(S3, S4, QMINE_ASSET, 5000000000LL);

    // Treasury Donation
    qrwa.transferManagementRights(TREASURY_HOLDER, QMINE_ASSET, 10, QRWA_CONTRACT_INDEX);
    EXPECT_EQ(qrwa.donateToTreasury(TREASURY_HOLDER, 10), QRWA_STATUS_SUCCESS);

    //Revenue
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);

    qrwa.endEpoch();

    // Checks Ep 1
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << "END-EPOCH 1\n";
    print_balances();
#endif

    // Contract holds 10 shares. Base = Total Supply - 10
    sint64 payoutBaseEp1 = TOTAL_SUPPLY - 10;
    sint64 qrwaRateEp1 = getQrwaRateForEpoch(QRWA_POOL_AMT_BASE); // Standard pool for Ep 1

    sint64 divS1 = calculateQminePayout(160000000000LL, payoutBaseEp1, QMINE_POOL_AMT);
    sint64 divS2 = calculateQminePayout(170000000000LL, payoutBaseEp1, QMINE_POOL_AMT);
    sint64 divS3 = calculateQminePayout(165000000000LL, payoutBaseEp1, QMINE_POOL_AMT);
    sint64 divS4 = calculateQminePayout(170000000000LL, payoutBaseEp1, QMINE_POOL_AMT);
    sint64 divS5 = calculateQminePayout(170000000000LL, payoutBaseEp1, QMINE_POOL_AMT);

    sint64 divQS1 = calculateQrwaPayout(200, qrwaRateEp1);
    sint64 divQQ1 = calculateQrwaPayout(200, qrwaRateEp1);
    sint64 divQQ2 = calculateQrwaPayout(276, qrwaRateEp1);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);

    snapshotBalances();

#if ENABLE_BALANCE_DEBUG
    std::cout << "PRE-EPOCH 2\n";
    print_balances();
#endif

    // epoch 2
    qrwa.beginEpoch();

    // Treasury Donation (Remaining)
    sint64 treasuryRemaining = TREASURY_INIT - 10;
    qrwa.transferManagementRights(TREASURY_HOLDER, QMINE_ASSET, treasuryRemaining, QRWA_CONTRACT_INDEX);
    EXPECT_EQ(qrwa.donateToTreasury(TREASURY_HOLDER, treasuryRemaining), QRWA_STATUS_SUCCESS);

    // Exchange
    qrwa.transferAsset(S1, S2, QMINE_ASSET, 10000000000LL);
    qrwa.transferAsset(S2, S3, QMINE_ASSET, 10000000000LL);
    qrwa.transferAsset(S3, S4, QMINE_ASSET, 10000000000LL);
    qrwa.transferAsset(S4, S5, QMINE_ASSET, 10000000000LL);

    // Revenue
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);

    // Release Poll
    QRWA::CreateAssetReleasePoll_input pollInput;
    pollInput.proposalName = id::randomValue();
    pollInput.asset = QMINE_ASSET;
    pollInput.amount = 1000;
    pollInput.destination = DESTINATION_ADDR;

    auto pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput);
    uint64 pollIdEp2 = pollOut.proposalId;

    EXPECT_EQ(qrwa.voteAssetRelease(S1, pollIdEp2, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S2, pollIdEp2, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S3, pollIdEp2, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S4, pollIdEp2, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S5, pollIdEp2, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(Q1, pollIdEp2, 1), QRWA_STATUS_FAILURE_NOT_AUTHORIZED);

    qrwa.endEpoch();

    // Checks Ep 2
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << "END-EPOCH 2\n";
    print_balances();
#endif

    auto pollResultEp2 = qrwa.getAssetReleasePoll(pollIdEp2);
    EXPECT_EQ(pollResultEp2.proposal.status, QRWA_POLL_STATUS_PASSED_EXECUTED);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { DESTINATION_ADDR, QX_CONTRACT_INDEX }), 1000);

    // Calculate Pools based on Revenue - 100 QU Fee
    sint64 netRevenueEp2 = REVENUE_AMT - 100;
    sint64 feeAmtEp2 = (netRevenueEp2 * 500) / 1000; // 50% fees
    sint64 distributableEp2 = netRevenueEp2 - feeAmtEp2;
    sint64 qminePoolEp2 = (distributableEp2 * 900) / 1000;
    sint64 qrwaPoolEp2 = distributableEp2 - qminePoolEp2;

    // Correct Base: TOTAL_SUPPLY - 10 (Shares held by SC at START of epoch)
    sint64 payoutBaseEp2 = TOTAL_SUPPLY - 10;

    sint64 qrwaRateEp2 = getQrwaRateForEpoch(qrwaPoolEp2);

    divS1 = calculateQminePayout(150000000000LL, payoutBaseEp2, qminePoolEp2);
    divS2 = calculateQminePayout(180000000000LL, payoutBaseEp2, qminePoolEp2);
    divS3 = calculateQminePayout(165000000000LL, payoutBaseEp2, qminePoolEp2);
    divS4 = calculateQminePayout(175000000000LL, payoutBaseEp2, qminePoolEp2);
    divS5 = calculateQminePayout(170000000000LL, payoutBaseEp2, qminePoolEp2);

    divQS1 = calculateQrwaPayout(200, qrwaRateEp2);
    divQQ1 = calculateQrwaPayout(200, qrwaRateEp2);
    divQQ2 = calculateQrwaPayout(276, qrwaRateEp2);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);

    snapshotBalances();

#if ENABLE_BALANCE_DEBUG
    std::cout << " PRE-EPOCH 3\n";
    print_balances();
#endif

    // epoch 3
    qrwa.beginEpoch();

    // Exchange
    qrwa.transferAsset(S1, S2, QMINE_ASSET, 5000000000LL);
    qrwa.transferAsset(S2, S3, QMINE_ASSET, 5000000000LL);
    qrwa.transferAsset(S3, S4, QMINE_ASSET, 5000000000LL);
    qrwa.transferAsset(S4, S5, QMINE_ASSET, 5000000000LL);

    // Revenue
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);

    // Release Poll
    pollInput.amount = 500;
    pollInput.proposalName = id::randomValue();
    pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput);
    uint64 pollIdEp3 = pollOut.proposalId;

    EXPECT_EQ(qrwa.voteAssetRelease(S1, pollIdEp3, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S2, pollIdEp3, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S3, pollIdEp3, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S4, pollIdEp3, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S5, pollIdEp3, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(Q1, pollIdEp3, 1), QRWA_STATUS_FAILURE_NOT_AUTHORIZED);

    // Gov Vote
    QRWA::QRWAGovParams newParams = qrwa.getGovParams();
    newParams.electricityPercent = 300;
    newParams.maintenancePercent = 100;

    newParams.mAdminAddress = ADMIN_ADDRESS;
    newParams.qmineDevAddress = QMINE_DEV_ADDR_TEST;
    newParams.electricityAddress = FEE_ADDR_E;
    newParams.maintenanceAddress = FEE_ADDR_M;
    newParams.reinvestmentAddress = FEE_ADDR_R;

    EXPECT_EQ(qrwa.voteGovParams(S1, newParams), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteGovParams(S2, newParams), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteGovParams(S3, newParams), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteGovParams(S4, newParams), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteGovParams(S5, newParams), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteGovParams(Q1, newParams), QRWA_STATUS_FAILURE_NOT_AUTHORIZED);

    qrwa.endEpoch();

    // Checks Ep 3
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << " END-EPOCH 3\n";
    print_balances();
#endif

    auto pollResultEp3 = qrwa.getAssetReleasePoll(pollIdEp3);
    EXPECT_EQ(pollResultEp3.proposal.status, QRWA_POLL_STATUS_PASSED_EXECUTED);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { DESTINATION_ADDR, QX_CONTRACT_INDEX }), 1000 + 500);

    auto activeParams = qrwa.getGovParams();
    EXPECT_EQ(activeParams.electricityPercent, 300);
    EXPECT_EQ(activeParams.maintenancePercent, 100);

    // Calculate Pools based on Revenue - 100 QU Fee
    sint64 netRevenueEp3 = REVENUE_AMT - 100;
    sint64 feeAmtEp3 = (netRevenueEp3 * 500) / 1000; // 50% fees still (params update next epoch)
    sint64 distributableEp3 = netRevenueEp3 - feeAmtEp3;
    sint64 qminePoolEp3 = (distributableEp3 * 900) / 1000;
    sint64 qrwaPoolEp3 = distributableEp3 - qminePoolEp3;

    // Contract released 1000 + 500. Balance = 150B - 1500.
    sint64 payoutBaseEp3 = TOTAL_SUPPLY - (TREASURY_INIT - 1500);

    sint64 qrwaRateEp3 = getQrwaRateForEpoch(qrwaPoolEp3);

    divS1 = calculateQminePayout(145000000000LL, payoutBaseEp3, qminePoolEp3);
    divS2 = calculateQminePayout(180000000000LL, payoutBaseEp3, qminePoolEp3);
    divS3 = calculateQminePayout(165000000000LL, payoutBaseEp3, qminePoolEp3);
    divS4 = calculateQminePayout(175000000000LL, payoutBaseEp3, qminePoolEp3);
    divS5 = calculateQminePayout(180000000000LL, payoutBaseEp3, qminePoolEp3);

    divQS1 = calculateQrwaPayout(200, qrwaRateEp3);
    divQQ1 = calculateQrwaPayout(200, qrwaRateEp3);
    divQQ2 = calculateQrwaPayout(276, qrwaRateEp3);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);

    snapshotBalances();

#if ENABLE_BALANCE_DEBUG
    std::cout << " PRE-EPOCH 4\n";
    print_balances();
#endif

    // epoch 4 (no transfers)
    qrwa.beginEpoch();
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);
    qrwa.endEpoch();

    // Checks Ep 4
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << " END-EPOCH 4\n";
    print_balances();
#endif

    // Payout base remains same as previous epoch (no new releases)
    sint64 payoutBaseEp4 = payoutBaseEp3;
    // Revenue is full 10M (no releases)
    sint64 qminePoolEp4 = QMINE_POOL_AMT;

    sint64 qrwaRateEp4 = getQrwaRateForEpoch(QRWA_POOL_AMT_BASE);

    divS1 = calculateQminePayout(145000000000LL, payoutBaseEp4, qminePoolEp4);
    divS2 = calculateQminePayout(180000000000LL, payoutBaseEp4, qminePoolEp4);
    divS3 = calculateQminePayout(165000000000LL, payoutBaseEp4, qminePoolEp4);
    divS4 = calculateQminePayout(175000000000LL, payoutBaseEp4, qminePoolEp4);
    divS5 = calculateQminePayout(185000000000LL, payoutBaseEp4, qminePoolEp4);

    divQS1 = calculateQrwaPayout(200, qrwaRateEp4);
    divQQ1 = calculateQrwaPayout(200, qrwaRateEp4);
    divQQ2 = calculateQrwaPayout(276, qrwaRateEp4);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);

    snapshotBalances();

#if ENABLE_BALANCE_DEBUG
    std::cout << " PRE-EPOCH 5\n";
    print_balances();
#endif

    // epoch 5
    qrwa.beginEpoch();
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);

    // Release Poll
    pollInput.amount = 100;
    pollInput.proposalName = id::randomValue();
    pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput);
    uint64 pollIdEp5 = pollOut.proposalId;

    // Vote NO (3/5 Majority)
    EXPECT_EQ(qrwa.voteAssetRelease(S1, pollIdEp5, 0), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S2, pollIdEp5, 0), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S3, pollIdEp5, 0), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S4, pollIdEp5, 1), QRWA_STATUS_SUCCESS);
    EXPECT_EQ(qrwa.voteAssetRelease(S5, pollIdEp5, 1), QRWA_STATUS_SUCCESS);

    qrwa.endEpoch();

    // Checks Ep 5
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << " END-EPOCH 5\n";
    print_balances();
#endif

    auto pollResultEp5 = qrwa.getAssetReleasePoll(pollIdEp5);
    EXPECT_EQ(pollResultEp5.proposal.status, QRWA_POLL_STATUS_FAILED_VOTE);
    EXPECT_EQ(numberOfShares(QMINE_ASSET, { DESTINATION_ADDR, QX_CONTRACT_INDEX }), 1500); // Unchanged

    // Failed vote = No release = No fee = Full Revenue. Base unchanged.
    sint64 qrwaRateEp5 = getQrwaRateForEpoch(QRWA_POOL_AMT_BASE);

    divS1 = calculateQminePayout(145000000000LL, payoutBaseEp4, qminePoolEp4);
    divS2 = calculateQminePayout(180000000000LL, payoutBaseEp4, qminePoolEp4);
    divS3 = calculateQminePayout(165000000000LL, payoutBaseEp4, qminePoolEp4);
    divS4 = calculateQminePayout(175000000000LL, payoutBaseEp4, qminePoolEp4);
    divS5 = calculateQminePayout(185000000000LL, payoutBaseEp4, qminePoolEp4);

    divQS1 = calculateQrwaPayout(200, qrwaRateEp5);
    divQQ1 = calculateQrwaPayout(200, qrwaRateEp5);
    divQQ2 = calculateQrwaPayout(276, qrwaRateEp5);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);

    snapshotBalances();

#if ENABLE_BALANCE_DEBUG
    std::cout << " PRE-EPOCH 6\n";
    print_balances();
#endif

    // epoch 6
    qrwa.beginEpoch();

    // Revenue
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);

    // Create Gov Proposal
    QRWA::QRWAGovParams failParams = qrwa.getGovParams();
    failParams.reinvestmentPercent = 200;

    // Only S1 votes (< 20% supply). Quorum fail
    EXPECT_EQ(qrwa.voteGovParams(S1, failParams), QRWA_STATUS_SUCCESS);

    qrwa.endEpoch();

    // Checks Ep 6
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << " END-EPOCH 6\n";
    print_balances();
#endif

    auto paramsEp6 = qrwa.getGovParams();
    EXPECT_EQ(paramsEp6.reinvestmentPercent, 100);
    EXPECT_NE(paramsEp6.reinvestmentPercent, 200);

    sint64 qrwaRateEp6 = getQrwaRateForEpoch(QRWA_POOL_AMT_BASE);

    divS1 = calculateQminePayout(145000000000LL, payoutBaseEp4, qminePoolEp4);
    divS2 = calculateQminePayout(180000000000LL, payoutBaseEp4, qminePoolEp4);
    divS3 = calculateQminePayout(165000000000LL, payoutBaseEp4, qminePoolEp4);
    divS4 = calculateQminePayout(175000000000LL, payoutBaseEp4, qminePoolEp4);
    divS5 = calculateQminePayout(185000000000LL, payoutBaseEp4, qminePoolEp4);

    divQS1 = calculateQrwaPayout(200, qrwaRateEp6);
    divQQ1 = calculateQrwaPayout(200, qrwaRateEp6);
    divQQ2 = calculateQrwaPayout(276, qrwaRateEp6);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);

    snapshotBalances();

#if ENABLE_BALANCE_DEBUG
    std::cout << " PRE-EPOCH 7\n";
    print_balances();
#endif

    // epoch 7
    qrwa.beginEpoch();

    // Revenue
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), REVENUE_AMT);

    // Create poll, no votes
    pollInput.amount = 100;
    pollInput.proposalName = id::randomValue();
    pollOut = qrwa.createAssetReleasePoll(ADMIN_ADDRESS, pollInput);
    uint64 pollIdEp7 = pollOut.proposalId;

    qrwa.endEpoch();

    // Checks Ep 7
    advanceTime7Days();
    qrwa.resetPayoutTime();
    qrwa.endTick();

#if ENABLE_BALANCE_DEBUG
    std::cout << " END-EPOCH 7\n";
    print_balances();
#endif

    auto pollResultEp7 = qrwa.getAssetReleasePoll(pollIdEp7);
    EXPECT_EQ(pollResultEp7.proposal.status, QRWA_POLL_STATUS_FAILED_VOTE);

    sint64 qrwaRateEp7 = getQrwaRateForEpoch(QRWA_POOL_AMT_BASE);

    divS1 = calculateQminePayout(145000000000LL, payoutBaseEp4, qminePoolEp4);
    divS2 = calculateQminePayout(180000000000LL, payoutBaseEp4, qminePoolEp4);
    divS3 = calculateQminePayout(165000000000LL, payoutBaseEp4, qminePoolEp4);
    divS4 = calculateQminePayout(175000000000LL, payoutBaseEp4, qminePoolEp4);
    divS5 = calculateQminePayout(185000000000LL, payoutBaseEp4, qminePoolEp4);

    divQS1 = calculateQrwaPayout(200, qrwaRateEp7);
    divQQ1 = calculateQrwaPayout(200, qrwaRateEp7);
    divQQ2 = calculateQrwaPayout(276, qrwaRateEp7);

    EXPECT_EQ(getBalance(S1), prevBalances[S1] + divS1 + divQS1);
    EXPECT_EQ(getBalance(S2), prevBalances[S2] + divS2);
    EXPECT_EQ(getBalance(S3), prevBalances[S3] + divS3);
    EXPECT_EQ(getBalance(S4), prevBalances[S4] + divS4);
    EXPECT_EQ(getBalance(S5), prevBalances[S5] + divS5);
    EXPECT_EQ(getBalance(Q1), prevBalances[Q1] + divQQ1);
    EXPECT_EQ(getBalance(Q2), prevBalances[Q2] + divQQ2);
}

TEST(ContractQRWA, Payout_MultiContractManagement)
{
    ContractTestingQRWA qrwa;

    const sint64 totalShares = 1000000;
    const sint64 qxManagedShares = 700000;
    const sint64 qswapManagedShares = 300000; // 30% moved to QSWAP management

    // Issue QMINE and give to HOLDER_A
    // Initially, all 1M shares are managed by QX (default for transfers via QX)
    increaseEnergy(QMINE_ISSUER, 1000000000);
    increaseEnergy(HOLDER_A, 1000000); // For fees

    qrwa.issueAsset(QMINE_ISSUER, QMINE_ASSET.assetName, totalShares);
    qrwa.transferAsset(QMINE_ISSUER, HOLDER_A, QMINE_ASSET, totalShares);

    // Verify initial state managed by QX
    EXPECT_EQ(numberOfPossessedShares(QMINE_ASSET.assetName, QMINE_ASSET.issuer, HOLDER_A, HOLDER_A, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), totalShares);
    EXPECT_EQ(numberOfPossessedShares(QMINE_ASSET.assetName, QMINE_ASSET.issuer, HOLDER_A, HOLDER_A, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), 0);

    // Transfer management rights of 300k shares to QSWAP
    // The user (HOLDER_A) remains the Possessor.
    qrwa.transferManagementRights(HOLDER_A, QMINE_ASSET, qswapManagedShares, QSWAP_CONTRACT_INDEX);

    // Verify the split in management rights
    // 700k should remain under QX
    EXPECT_EQ(numberOfPossessedShares(QMINE_ASSET.assetName, QMINE_ASSET.issuer, HOLDER_A, HOLDER_A, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), qxManagedShares);
    // 300k should now be under QSWAP
    EXPECT_EQ(numberOfPossessedShares(QMINE_ASSET.assetName, QMINE_ASSET.issuer, HOLDER_A, HOLDER_A, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), qswapManagedShares);

    qrwa.beginEpoch();

    // Generate Revenue
    // pool A revenue: 1,000,000 QUs
    // fees (50%): 500,000
    // net revenue: 500,000
    // QMINE pool (90%): 450,000
    qrwa.sendToMany(ADMIN_ADDRESS, id(QRWA_CONTRACT_INDEX, 0, 0, 0), 1000000);

    qrwa.endEpoch();

    // trigger Payout
    etalonTick.year = 25; etalonTick.month = 11; etalonTick.day = 7; // Friday
    etalonTick.hour = 12; etalonTick.minute = 1; etalonTick.second = 0;
    qrwa.resetPayoutTime();

    // snapshot balances for check
    sint64 balanceBefore = getBalance(HOLDER_A);

    qrwa.endTick();

    // Calculate Expected Payout
    // Payout = (UserTotalShares * PoolAmount) / TotalSupply
    // UserTotalShares = 1,000,000 (regardless of manager)
    // PoolAmount = 450,000
    // TotalSupply = 1,000,000
    // Expected = 450,000
    sint64 expectedPayout = (totalShares * 450000) / totalShares;

    sint64 balanceAfter = getBalance(HOLDER_A);

    // If qRWA only counted QX shares, the payout would be (700k/1M * 450k) = 315,000.
    // If qRWA counts ALL shares, the payout is 450,000.
    EXPECT_EQ(balanceAfter - balanceBefore, expectedPayout);
    EXPECT_EQ(balanceAfter - balanceBefore, 450000);
}
