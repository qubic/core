#define NO_UEFI

#include "contract_testing.h"
#include "test_util.h"


// Pseudo IDs (for testing only)

// QMINE_ISSUER is is also the ADMIN_ADDRESS
static const id QMINE_ISSUER = id::randomValue();
static const id ADMIN_ADDRESS = QMINE_ISSUER;

// temporary holder for the initial 150M QMINE supply
static const id TREASURY_HOLDER = id::randomValue();

// Addresses for governance-set fees
static const id FEE_ADDR_E = id::randomValue(); // Electricity fees address
static const id FEE_ADDR_M = id::randomValue(); // Maintenance fees address
static const id FEE_ADDR_R = id::randomValue(); // Reinvestment fees address

// pseudo test address for QMINE developer
static const id QMINE_DEV_ADDR_TEST = id::randomValue();

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
    friend class QRWA;

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

        // Custom Initialization for qRWA State
        // (Overrides defaults from INITIALIZE() for testing purposes)
        QRWA* state = getState();

        // Set the TEST QMINE asset
        state->mQmineAsset = QMINE_ASSET;

        // Set the TEST Governance Params
        state->mCurrentGovParams.mAdminAddress = ADMIN_ADDRESS;
        state->mCurrentGovParams.qmineDevAddress = QMINE_DEV_ADDR_TEST;

        // Fee addresses
        state->mCurrentGovParams.electricityAddress = FEE_ADDR_E;
        state->mCurrentGovParams.maintenanceAddress = FEE_ADDR_M;
        state->mCurrentGovParams.reinvestmentAddress = FEE_ADDR_R;

        // Fee percentages (35%, 5%, 10%)
        state->mCurrentGovParams.electricityPercent = 350; // 35.0%
        state->mCurrentGovParams.maintenancePercent = 50;  // 5.0%
        state->mCurrentGovParams.reinvestmentPercent = 100; // 10.0%
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

    uint64 voteGovParams(const id& from, const QRWA::qRWAGovParams& params)
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

    // QRWA Wrappers

    QRWA::qRWAGovParams getGovParams()
    {
        QRWA::GetGovParams_input input;
        QRWA::GetGovParams_output output;
        callFunction(QRWA_CONTRACT_INDEX, QRWA_FUNC_GET_GOV_PARAMS, input, output);
        return output;
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

};


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
    QRWA::qRWAGovParams invalidParams = qrwa.getGovParams();
    invalidParams.mAdminAddress = NULL_ID;
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_A, invalidParams), QRWA_STATUS_FAILURE_INVALID_INPUT);

    // Create new poll and vote for it
    QRWA::qRWAGovParams paramsA = qrwa.getGovParams();
    paramsA.electricityPercent = 100; // Change one param

    EXPECT_EQ(qrwa.voteGovParams(HOLDER_A, paramsA), QRWA_STATUS_SUCCESS); // Poll 0
    EXPECT_EQ(qrwa.voteGovParams(HOLDER_B, paramsA), QRWA_STATUS_SUCCESS); // Vote for Poll 0

    // Change vote
    QRWA::qRWAGovParams paramsB = qrwa.getGovParams();
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
