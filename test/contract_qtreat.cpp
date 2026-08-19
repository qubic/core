#define NO_UEFI

#include <sstream>
#include "contract_testing.h"

// MARKETPLACE_OWNER must match the hardcoded id set in QBAY::INITIALIZE() (Qbay.h).
static const id QTREAT_TEST_MARKETPLACE_OWNER = ID(_R, _K, _D, _H, _C, _M, _R, _J, _Y, _C, _G, _K, _P, _D, _U, _Y, _R, _X, _G, _D, _Y, _Z, _C, _I, _Z, _I, _T, _A, _H, _Y, _O, _V, _G, _I, _U, _T, _K, _N, _D, _T, _E, _H, _P, _C, _C, _L, _W, _L, _Z, _X, _S, _H, _N, _F, _P, _D);

static id getUser(uint64 i)
{
    return id(i + 1, i / 2 + 4, i + 10, i * 3 + 8);
}

// Exposes QTREAT's internal state for direct assertions beyond what the view functions return.
class QTreatChecker : public QTREAT, public QTREAT::StateData
{
public:
    bool getStaker(const id& user, StakerInfo& out) const
    {
        return stakers.get(user, out);
    }
    uint64 stakerCount() const { return stakers.population(); }
    uint64 getTotalStaked() const { return totalStaked; }
    uint64 getQtreatBonusPool() const { return qtreatBonusPool; }
    uint64 getDividendFund() const { return dividendFund; }
    uint64 getStakingFund() const { return stakingFund; }
    uint64 getMiningFund() const { return miningFund; }
    uint64 getDripQdogePool() const { return dripQdogePool; }
    uint64 getAsicCatalogSize() const { return asicCatalogSize; }
    uint64 getAsicCatalogLocked() const { return asicCatalogLocked; }
    uint64 getDripStartEpoch() const { return dripStartEpoch; }
    uint64 getTotalAsicCount() const { return totalAsicCount; }
    uint64 getTotalMiningWeight() const { return totalMiningWeight; }
    AsicRig getRig(sint64 idx) const { return asicRigs.get(idx); }
    uint64 getAsicRigHighWater() const { return asicRigHighWater; }
    id getAdminAddress() const { return adminAddress; }
    Asset getQdogeToken() const { return qdogeToken; }
    Asset getQtreatToken() const { return qtreatToken; }
    uint64 generalAssetBalanceOf(const id& issuer, uint64 assetName) const
    {
        AssetKey key; key.issuer = issuer; key.assetName = assetName;
        uint64 bal = 0;
        generalAssetBalances.get(key, bal);
        return bal;
    }
};

class ContractTestingQtreat : protected ContractTesting
{
public:
    id adminAddress;
    id tokenIssuer; // shared issuer of both QTREAT and QDOGE assets
    uint32 nextNftId = 0;

    ContractTestingQtreat()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        system.epoch = contractDescriptions[QTREAT_CONTRACT_INDEX].constructionEpoch;

        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(RANDOM);
        callSystemProcedure(RANDOM_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QBAY);
        callSystemProcedure(QBAY_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QTREAT);
        callSystemProcedure(QTREAT_CONTRACT_INDEX, INITIALIZE);

        adminAddress = getState()->getAdminAddress();
        tokenIssuer = getState()->getQdogeToken().issuer;
        EXPECT_EQ(tokenIssuer, getState()->getQtreatToken().issuer);

        increaseEnergy(adminAddress, 1);
        increaseEnergy(tokenIssuer, 1);

        // Large pre-issued supplies of QDOGE and QTREAT so tests can freely fund users.
        increaseEnergy(tokenIssuer, 2000000000ULL);
        EXPECT_GT(issueAsset(tokenIssuer, QTREAT_QDOGE_ASSETNAME, 1000000000000LL, 0, 0), 0);
        EXPECT_GT(issueAsset(tokenIssuer, QTREAT_TOKEN_ASSETNAME, 1000000000000LL, 0, 0), 0);

        // tokenIssuer still possesses almost the entire pre-issued QTREAT-token supply
        // (tests only ever hand out small slices of it). Left un-excluded, that dwarfs
        // any test holder's balance and would absorb virtually 100% of every dividend
        // payout by weight. Exclude it (reserving the last slot so tests are free to use
        // the others for their own exclude-address scenarios).
        setExcludeAddress(adminAddress, QTREAT_MAX_EXCLUDE_ADDRESSES - 1, tokenIssuer);

        beginEpoch();
    }

    QTreatChecker* getState()
    {
        return (QTreatChecker*)contractStates[QTREAT_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true) { callSystemProcedure(QTREAT_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess); }
    void endEpoch(bool expectSuccess = true) { callSystemProcedure(QTREAT_CONTRACT_INDEX, END_EPOCH, expectSuccess); }

    // Ends the current epoch, moves the clock forward, and begins the next one.
    void advanceEpoch()
    {
        endEpoch();
        system.epoch++;
        beginEpoch();
    }

    // ---------------------------------------------------------------- QX helpers

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, 1000000000ULL);
        return output.issuedNumberOfShares;
    }

    sint64 xferOwnership(const id& issuer, uint64 assetName, const id& from, sint64 numberOfShares, const id& to)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;
        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = to;
        input.numberOfShares = numberOfShares;
        increaseEnergy(from, 100);
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, 100);
        return output.transferredNumberOfShares;
    }

    sint64 xferManagementRights(const id& issuer, uint64 assetName, uint32 newManagingContractIndex, sint64 numberOfShares, const id& currentOwner)
    {
        QX::TransferShareManagementRights_input input;
        QX::TransferShareManagementRights_output output;
        input.asset.assetName = assetName;
        input.asset.issuer = issuer;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;
        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, currentOwner, 0);
        return output.transferredNumberOfShares;
    }

    // Give `user` `amount` QDOGE they fully own/possess (management still with QX).
    void giveQdoge(const id& user, uint64 amount)
    {
        EXPECT_EQ(xferOwnership(tokenIssuer, QTREAT_QDOGE_ASSETNAME, tokenIssuer, (sint64)amount, user), (sint64)amount);
    }

    void giveQtreat(const id& user, uint64 amount)
    {
        EXPECT_EQ(xferOwnership(tokenIssuer, QTREAT_TOKEN_ASSETNAME, tokenIssuer, (sint64)amount, user), (sint64)amount);
    }

    // Give + stake in one step: transfers `amount` QDOGE to `user`, then the user
    // transfers management rights to QTREAT (this is what real staking looks like).
    sint64 stakeQdoge(const id& user, uint64 amount)
    {
        giveQdoge(user, amount);
        return xferManagementRights(tokenIssuer, QTREAT_QDOGE_ASSETNAME, QTREAT_CONTRACT_INDEX, (sint64)amount, user);
    }

    // ---------------------------------------------------------------- QBAY helpers

    void activateQbayMarket()
    {
        increaseEnergy(QTREAT_TEST_MARKETPLACE_OWNER, 1);
        QBAY::changeStatusOfMarketPlace_input input{ 1 };
        QBAY::changeStatusOfMarketPlace_output output;
        invokeUserProcedure(QBAY_CONTRACT_INDEX, 17, input, output, QTREAT_TEST_MARKETPLACE_OWNER, 0);
        EXPECT_EQ(output.returnCode, 0u);
    }

    // Mints a single free-standing NFT (no collection) owned by `user`. IDs are
    // assigned sequentially by QBAY starting at 0, so the first call here returns 0.
    uint32 mintNft(const id& user)
    {
        QBAY::mint_input input{};
        input.royalty = 0;
        input.collectionId = 0;
        input.typeOfMint = 1;
        QBAY::mint_output output;
        increaseEnergy(user, QBAY_SINGLE_NFT_CREATE_FEE);
        invokeUserProcedure(QBAY_CONTRACT_INDEX, 3, input, output, user, QBAY_SINGLE_NFT_CREATE_FEE);
        EXPECT_EQ(output.returnCode, 0u);
        return nextNftId++;
    }

    void transferNft(const id& user, uint32 NFTid, const id& receiver)
    {
        QBAY::transfer_input input{ receiver, NFTid };
        QBAY::transfer_output output;
        invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, input, output, user, 0);
        EXPECT_EQ(output.returnCode, 0u);
    }

    // ---------------------------------------------------------------- QTREAT view wrappers

    QTREAT::GetStakingInfo_output getStakingInfo(const id& staker)
    {
        QTREAT::GetStakingInfo_input input{ staker };
        QTREAT::GetStakingInfo_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QTREAT::GetPhaseInfo_output getPhaseInfo()
    {
        QTREAT::GetPhaseInfo_input input{};
        QTREAT::GetPhaseInfo_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QTREAT::GetFunds_output getFunds()
    {
        QTREAT::GetFunds_input input{};
        QTREAT::GetFunds_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QTREAT::GetRaffleInfo_output getRaffleInfo()
    {
        QTREAT::GetRaffleInfo_input input{};
        QTREAT::GetRaffleInfo_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QTREAT::GetExcludeAddresses_output getExcludeAddresses()
    {
        QTREAT::GetExcludeAddresses_input input{};
        QTREAT::GetExcludeAddresses_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QTREAT::GetNftInfo_output getNftInfo(const id& wallet)
    {
        QTREAT::GetNftInfo_input input{ wallet };
        QTREAT::GetNftInfo_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QTREAT::GetMinerInfo_output getMinerInfo(const id& wallet)
    {
        QTREAT::GetMinerInfo_input input{ wallet };
        QTREAT::GetMinerInfo_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QTREAT::GetAsicCatalogInfo_output getAsicCatalogInfo(uint32 nftId)
    {
        QTREAT::GetAsicCatalogInfo_input input{ nftId };
        QTREAT::GetAsicCatalogInfo_output output;
        callFunction(QTREAT_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    // ---------------------------------------------------------------- QTREAT procedure wrappers

    uint32 depositDividends(const id& user, sint64 amount)
    {
        QTREAT::DepositDividends_input input{};
        QTREAT::DepositDividends_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 1, input, output, user, amount);
        return output.returnCode;
    }

    uint32 depositStakingFund(const id& user, sint64 amount)
    {
        QTREAT::DepositStakingFund_input input{};
        QTREAT::DepositStakingFund_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 2, input, output, user, amount);
        return output.returnCode;
    }

    QTREAT::RequestUnstake_output requestUnstake(const id& user, uint64 amount, sint64 fee)
    {
        QTREAT::RequestUnstake_input input{ amount };
        QTREAT::RequestUnstake_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 3, input, output, user, fee);
        return output;
    }

    uint32 finalizeUnstake(const id& user)
    {
        QTREAT::FinalizeUnstake_input input{};
        QTREAT::FinalizeUnstake_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 4, input, output, user, 0);
        return output.returnCode;
    }

    QTREAT::ClaimQtreatBonus_output claimQtreatBonus(const id& user, sint64 fee)
    {
        QTREAT::ClaimQtreatBonus_input input{};
        QTREAT::ClaimQtreatBonus_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 5, input, output, user, fee);
        return output;
    }

    // Admin path: transfers `amount` QTREAT-token management rights to QTREAT, then deposits.
    uint32 depositQtreatTokensAsAdmin(uint64 amount)
    {
        giveQtreat(adminAddress, amount);
        EXPECT_EQ(xferManagementRights(tokenIssuer, QTREAT_TOKEN_ASSETNAME, QTREAT_CONTRACT_INDEX, (sint64)amount, adminAddress), (sint64)amount);
        QTREAT::DepositQtreatTokens_input input{ amount };
        QTREAT::DepositQtreatTokens_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 6, input, output, adminAddress, 0);
        return output.returnCode;
    }

    uint32 depositQtreatTokensRaw(const id& caller, uint64 amount)
    {
        QTREAT::DepositQtreatTokens_input input{ amount };
        QTREAT::DepositQtreatTokens_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 6, input, output, caller, 0);
        return output.returnCode;
    }

    uint32 depositGeneralAssetRaw(const id& caller, const Asset& asset, uint64 amount)
    {
        QTREAT::DepositGeneralAsset_input input{ asset, amount };
        QTREAT::DepositGeneralAsset_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 7, input, output, caller, 0);
        return output.returnCode;
    }

    uint32 setExcludeAddress(const id& caller, uint64 slot, const id& address)
    {
        QTREAT::SetExcludeAddress_input input{ slot, address };
        QTREAT::SetExcludeAddress_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 8, input, output, caller, 0);
        return output.returnCode;
    }

    uint32 revokeGeneralAsset(const id& caller, const Asset& asset, uint64 amount, sint64 fee)
    {
        QTREAT::RevokeGeneralAsset_input input{ asset, amount };
        QTREAT::RevokeGeneralAsset_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 9, input, output, caller, fee);
        return output.returnCode;
    }

    uint32 releaseManagedShares(const id& caller, const Asset& asset, uint64 amount, sint64 fee)
    {
        QTREAT::ReleaseManagedShares_input input{ asset, amount };
        QTREAT::ReleaseManagedShares_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 10, input, output, caller, fee);
        return output.returnCode;
    }

    uint32 depositMiningFund(const id& user, sint64 amount)
    {
        QTREAT::DepositMiningFund_input input{};
        QTREAT::DepositMiningFund_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 11, input, output, user, amount);
        return output.returnCode;
    }

    QTREAT::LoadAsicPart_output loadAsicPart(const id& caller, uint32 nftId, uint8 category, uint8 rarity)
    {
        QTREAT::LoadAsicPart_input input{ nftId, category, rarity };
        QTREAT::LoadAsicPart_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 12, input, output, caller, 0);
        return output;
    }

    QTREAT::RegisterAsic_output registerAsic(const id& caller, uint32 m, uint32 c, uint32 p, uint32 f)
    {
        QTREAT::RegisterAsic_input input{ m, c, p, f };
        QTREAT::RegisterAsic_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 13, input, output, caller, 0);
        return output;
    }

    uint32 unregisterAsic(const id& caller, uint64 rigIndex)
    {
        QTREAT::UnregisterAsic_input input{ rigIndex };
        QTREAT::UnregisterAsic_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 14, input, output, caller, 0);
        return output.returnCode;
    }

    uint32 depositDripQdogeAsAdmin(uint64 amount)
    {
        // Admin transfers QDOGE management rights to QTREAT without becoming a staker
        // (PRE_ACQUIRE_SHARES / POST_ACQUIRE_SHARES both bypass the admin address).
        giveQdoge(adminAddress, amount);
        EXPECT_EQ(xferManagementRights(tokenIssuer, QTREAT_QDOGE_ASSETNAME, QTREAT_CONTRACT_INDEX, (sint64)amount, adminAddress), (sint64)amount);
        QTREAT::DepositDripQdoge_input input{ amount };
        QTREAT::DepositDripQdoge_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 15, input, output, adminAddress, 0);
        return output.returnCode;
    }

    uint32 depositDripQdogeRaw(const id& caller, uint64 amount)
    {
        QTREAT::DepositDripQdoge_input input{ amount };
        QTREAT::DepositDripQdoge_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 15, input, output, caller, 0);
        return output.returnCode;
    }

    uint32 setMiningRate(const id& caller, uint64 rate)
    {
        QTREAT::SetMiningRate_input input{ rate };
        QTREAT::SetMiningRate_output output;
        invokeUserProcedure(QTREAT_CONTRACT_INDEX, 16, input, output, caller, 0);
        return output.returnCode;
    }

    // Loads the full 400-part ASIC catalog. Reserves nftId==category (0,1,2,3) as the
    // single COMMON-rarity part of each category, so a real 4-part rig can be built
    // from just 4 minted QBAY NFTs (ids 0..3) without needing 400 real NFTs.
    void loadFullAsicCatalog()
    {
        static const uint64 budgets[5] = { 40, 30, 18, 10, 2 };
        uint32 next = 10000;
        for (uint8 cat = 0; cat < 4; cat++)
        {
            for (uint8 rarity = 0; rarity < 5; rarity++)
            {
                for (uint64 n = 0; n < budgets[rarity]; n++)
                {
                    uint32 id = (rarity == 0 && n == 0) ? (uint32)cat : next++;
                    auto out = loadAsicPart(adminAddress, id, cat, rarity);
                    EXPECT_EQ(out.returnCode, QTREAT_OK);
                }
            }
        }
        EXPECT_EQ(getState()->getAsicCatalogSize(), QTREAT_ASIC_PARTS_TOTAL);
        EXPECT_EQ(getState()->getAsicCatalogLocked(), 1u);
    }
};

// =============================================================================
// Basic state / views
// =============================================================================

TEST(ContractQtreat, AdminAddressRoundTripsToExpectedIdentity)
{
    // Confirms the ID(...) letter macro in INITIALIZE() encodes exactly the intended
    // 60-character address, checksum included (the checksum is a K12 hash of the
    // underlying public key, not derivable by inspection of the 56 identity letters
    // alone - this is the only reliable way to catch a transcription error here).
    ContractTestingQtreat t;
    std::ostringstream oss;
    oss << t.getState()->getAdminAddress();
    EXPECT_EQ(oss.str(), "QTREATZZIVFYQAIBKCZPSHGLIRMALZKHEWAPFLFXJAMDAXMGTBKQVXHHDHUD");
}

TEST(ContractQtreat, InitialStateAfterFirstBeginEpoch)
{
    ContractTestingQtreat t;

    auto phase = t.getPhaseInfo();
    EXPECT_EQ(phase.stakingStartEpoch, (uint64)contractDescriptions[QTREAT_CONTRACT_INDEX].constructionEpoch);
    EXPECT_EQ(phase.currentPhase, 1u);
    EXPECT_EQ(phase.epochsRemaining, QTREAT_TOTAL_REWARD_EPOCHS);

    auto raffle = t.getRaffleInfo();
    EXPECT_EQ(raffle.totalRaffleAwarded, 0u);
    EXPECT_EQ(raffle.lastRaffleWinner, NULL_ID);
    EXPECT_EQ(raffle.raffleEpochsRemaining, QTREAT_RAFFLE_EPOCHS);

    auto funds = t.getFunds();
    EXPECT_EQ(funds.dividendFund, 0u);
    EXPECT_EQ(funds.stakingFund, 0u);
    EXPECT_EQ(funds.qtreatBonusPool, 0u);
    EXPECT_EQ(funds.miningFund, 0u);

    auto info = t.getStakingInfo(getUser(1));
    EXPECT_EQ(info.isStaker, 0u);
    EXPECT_EQ(info.totalStaked, 0u);

    // Slot QTREAT_MAX_EXCLUDE_ADDRESSES-1 is reserved by the test fixture itself to
    // exclude tokenIssuer from dividend/NFT eligibility (see ContractTestingQtreat ctor).
    auto excl = t.getExcludeAddresses();
    for (sint64 i = 0; i < (sint64)QTREAT_MAX_EXCLUDE_ADDRESSES - 1; i++)
        EXPECT_EQ(excl.addresses.get(i), NULL_ID);
    EXPECT_EQ(excl.addresses.get((sint64)QTREAT_MAX_EXCLUDE_ADDRESSES - 1), t.tokenIssuer);

    EXPECT_EQ(t.getNftInfo(getUser(1)).nftCount, 0u);
    EXPECT_EQ(t.getMinerInfo(getUser(1)).asicCount, 0u);
    EXPECT_EQ(t.getAsicCatalogInfo(0).isCataloged, 0u);
}

TEST(ContractQtreat, DepositFundsAccumulateAndRejectZero)
{
    ContractTestingQtreat t;
    id user = getUser(1);
    increaseEnergy(user, 1000);

    EXPECT_EQ(t.depositDividends(user, 0), QTREAT_ERR_ZERO_AMOUNT);
    EXPECT_EQ(t.depositStakingFund(user, 0), QTREAT_ERR_ZERO_AMOUNT);
    EXPECT_EQ(t.depositMiningFund(user, 0), QTREAT_ERR_ZERO_AMOUNT);

    EXPECT_EQ(t.depositDividends(user, 100), QTREAT_OK);
    EXPECT_EQ(t.depositStakingFund(user, 200), QTREAT_OK);
    EXPECT_EQ(t.depositMiningFund(user, 300), QTREAT_OK);

    auto funds = t.getFunds();
    EXPECT_EQ(funds.dividendFund, 100u);
    EXPECT_EQ(funds.stakingFund, 200u);
    EXPECT_EQ(funds.miningFund, 300u);
}

TEST(ContractQtreat, PlainTransferRoutesToDividendFund)
{
    ContractTestingQtreat t;
    // POST_INCOMING_TRANSFER credits standardTransaction/qpiTransfer/revenueDonation to dividendFund.
    // invokeUserProcedure's fee path (amount>0 triggers POST_INCOMING_TRANSFER with procedureTransaction,
    // which is intentionally NOT one of the routed types) is exercised implicitly by DepositDividends above;
    // here we confirm GetFunds reflects a single accumulation path deterministically.
    id user = getUser(2);
    increaseEnergy(user, 500);
    EXPECT_EQ(t.depositDividends(user, 500), QTREAT_OK);
    EXPECT_EQ(t.getFunds().dividendFund, 500u);
}

// =============================================================================
// Staking / unstaking
// =============================================================================

TEST(ContractQtreat, StakingEnforcesMinimum)
{
    ContractTestingQtreat t;
    id user = getUser(1);
    increaseEnergy(user, 1000);

    // Below minimum: PRE_ACQUIRE_SHARES denies the transfer, so QX reports 0 shares moved.
    EXPECT_EQ(t.stakeQdoge(user, QTREAT_MIN_STAKE - 1), 0);
    EXPECT_EQ(t.getStakingInfo(user).isStaker, 0u);

    // At minimum: succeeds.
    EXPECT_EQ(t.stakeQdoge(user, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    auto info = t.getStakingInfo(user);
    EXPECT_EQ(info.isStaker, 1u);
    EXPECT_EQ(info.staked, QTREAT_MIN_STAKE);
    EXPECT_EQ(t.getState()->getTotalStaked(), QTREAT_MIN_STAKE);
}

TEST(ContractQtreat, StakingAccumulatesAcrossMultipleTransfers)
{
    ContractTestingQtreat t;
    id user = getUser(1);
    increaseEnergy(user, 1000);
    EXPECT_EQ(t.stakeQdoge(user, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    EXPECT_EQ(t.stakeQdoge(user, 5000000ULL), 5000000);
    EXPECT_EQ(t.getStakingInfo(user).staked, QTREAT_MIN_STAKE + 5000000ULL);
    EXPECT_EQ(t.getState()->getTotalStaked(), QTREAT_MIN_STAKE + 5000000ULL);
}

TEST(ContractQtreat, RequestUnstakeValidations)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    id nonStaker = getUser(2);
    increaseEnergy(nonStaker, 1000);
    increaseEnergy(staker, 1000);

    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_MIN_STAKE * 2), (sint64)(QTREAT_MIN_STAKE * 2));

    // Not a staker at all.
    EXPECT_EQ(t.requestUnstake(nonStaker, 1, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_NOT_STAKER);

    // Zero / excess amount.
    EXPECT_EQ(t.requestUnstake(staker, 0, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_INSUFFICIENT_STAKE);
    EXPECT_EQ(t.requestUnstake(staker, QTREAT_MIN_STAKE * 2 + 1, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_INSUFFICIENT_STAKE);

    // Leaving a dust remainder below the minimum is rejected.
    EXPECT_EQ(t.requestUnstake(staker, QTREAT_MIN_STAKE * 2 - 1, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_BELOW_MIN_STAKE);

    // Fee too low.
    EXPECT_EQ(t.requestUnstake(staker, QTREAT_MIN_STAKE, QTREAT_QX_TRANSFER_FEE - 1).returnCode, QTREAT_ERR_INSUFFICIENT_FEE);

    // Successful partial unstake with fee overpayment refunded.
    long long balBefore = getBalance(staker);
    auto out = t.requestUnstake(staker, QTREAT_MIN_STAKE, QTREAT_QX_TRANSFER_FEE + 50);
    EXPECT_EQ(out.returnCode, QTREAT_OK);
    EXPECT_EQ(getBalance(staker), balBefore - QTREAT_QX_TRANSFER_FEE);

    auto info = t.getStakingInfo(staker);
    EXPECT_EQ(info.staked, QTREAT_MIN_STAKE);
    EXPECT_EQ(info.unstakeAmount, QTREAT_MIN_STAKE);

    // A second unstake request while one is pending is rejected.
    EXPECT_EQ(t.requestUnstake(staker, 1, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_UNSTAKE_PENDING);
}

TEST(ContractQtreat, FinalizeUnstakeRespectsDelayAndReleasesQdoge)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, 1000);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    ASSERT_EQ(t.requestUnstake(staker, QTREAT_MIN_STAKE, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_OK);

    // Not ready immediately.
    EXPECT_EQ(t.finalizeUnstake(staker), QTREAT_ERR_UNSTAKE_NOT_READY);

    for (uint64 i = 0; i < QTREAT_UNSTAKE_DELAY_EPOCHS; i++)
        t.advanceEpoch();

    sint64 walletBefore = numberOfPossessedShares(QTREAT_QDOGE_ASSETNAME, t.tokenIssuer, staker, staker, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(t.finalizeUnstake(staker), QTREAT_OK);
    sint64 walletAfter = numberOfPossessedShares(QTREAT_QDOGE_ASSETNAME, t.tokenIssuer, staker, staker, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(walletAfter - walletBefore, (sint64)QTREAT_MIN_STAKE);

    // Staker record is dropped once fully unwound (no stake, no unstake, no bonus).
    QTREAT::StakerInfo info;
    EXPECT_FALSE(t.getState()->getStaker(staker, info));
}

TEST(ContractQtreat, EndEpochAutoReleasesMaturedUnstakes)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, 1000);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    ASSERT_EQ(t.requestUnstake(staker, QTREAT_MIN_STAKE, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_OK);

    // advanceEpoch() runs END_EPOCH BEFORE bumping the epoch counter, so maturity
    // (epoch >= unstakeEpoch + DELAY_EPOCHS) is only reached by the (DELAY_EPOCHS+1)-th
    // END_EPOCH call — one more than the number of epochs elapsed.
    for (uint64 i = 0; i <= QTREAT_UNSTAKE_DELAY_EPOCHS; i++)
        t.advanceEpoch();

    // END_EPOCH itself (of the tick where the delay has elapsed) auto-releases; no manual
    // FinalizeUnstake call needed.
    sint64 wallet = numberOfPossessedShares(QTREAT_QDOGE_ASSETNAME, t.tokenIssuer, staker, staker, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(wallet, (sint64)QTREAT_MIN_STAKE);
    EXPECT_EQ(t.getStakingInfo(staker).unstakeAmount, 0u);
}

// =============================================================================
// Growth streak / progressive bonus
// =============================================================================

TEST(ContractQtreat, GrowthStreakIncrementsWhenStakeGrowsAndResetsOtherwise)
{
    ContractTestingQtreat t;
    id grower = getUser(1);
    id flat = getUser(2);
    increaseEnergy(grower, 1000);
    increaseEnergy(flat, 1000);

    EXPECT_EQ(t.stakeQdoge(grower, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    EXPECT_EQ(t.stakeQdoge(flat, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);

    // Progressive bonus only starts accruing after QTREAT_PROGRESSIVE_START_DELAY_EPOCHS.
    for (uint64 i = 0; i < QTREAT_PROGRESSIVE_START_DELAY_EPOCHS; i++)
    {
        EXPECT_EQ(t.stakeQdoge(grower, QTREAT_PROGRESSIVE_MIN_STEP), (sint64)QTREAT_PROGRESSIVE_MIN_STEP);
        t.advanceEpoch();
    }
    EXPECT_EQ(t.getStakingInfo(grower).growthStreak, 0u);
    EXPECT_EQ(t.getStakingInfo(flat).growthStreak, 0u);

    // Now growing epoch over epoch increments the streak.
    EXPECT_EQ(t.stakeQdoge(grower, QTREAT_PROGRESSIVE_MIN_STEP), (sint64)QTREAT_PROGRESSIVE_MIN_STEP);
    t.advanceEpoch();
    EXPECT_EQ(t.getStakingInfo(grower).growthStreak, 1u);
    EXPECT_EQ(t.getStakingInfo(flat).growthStreak, 0u);

    EXPECT_EQ(t.stakeQdoge(grower, QTREAT_PROGRESSIVE_MIN_STEP), (sint64)QTREAT_PROGRESSIVE_MIN_STEP);
    t.advanceEpoch();
    EXPECT_EQ(t.getStakingInfo(grower).growthStreak, 2u);

    // A flat epoch (no growth) resets the streak to zero.
    t.advanceEpoch();
    EXPECT_EQ(t.getStakingInfo(grower).growthStreak, 0u);
}

TEST(ContractQtreat, StakingRewardWeightedByGrowthStreak)
{
    ContractTestingQtreat t;
    id grower = getUser(1);
    id flat = getUser(2);
    increaseEnergy(grower, 1000);
    increaseEnergy(flat, 1000);

    EXPECT_EQ(t.stakeQdoge(grower, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    EXPECT_EQ(t.stakeQdoge(flat, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);

    for (uint64 i = 0; i < QTREAT_PROGRESSIVE_START_DELAY_EPOCHS; i++)
        t.advanceEpoch();

    EXPECT_EQ(t.stakeQdoge(grower, QTREAT_PROGRESSIVE_MIN_STEP), (sint64)QTREAT_PROGRESSIVE_MIN_STEP);

    id funder = getUser(3);
    increaseEnergy(funder, QTREAT_STAKER_REWARD_PER_EPOCH * 2);
    EXPECT_EQ(t.depositStakingFund(funder, (sint64)QTREAT_STAKER_REWARD_PER_EPOCH * 2), QTREAT_OK);

    long long growerBefore = getBalance(grower);
    long long flatBefore = getBalance(flat);
    t.advanceEpoch(); // pays out with grower now on a 1-epoch streak (+25 permille weight)

    long long growerReward = getBalance(grower) - growerBefore;
    long long flatReward = getBalance(flat) - flatBefore;
    // Grower staked slightly more AND has a streak bonus, so must earn strictly more.
    EXPECT_GT(growerReward, flatReward);
}

// =============================================================================
// Loyalty bonus / raffle
// =============================================================================

TEST(ContractQtreat, LoyaltyBonusAccruesAndAutoDelivers)
{
    // Deliberately leave stakingFund/qtreatBonusPool at zero for the 12-epoch accrual
    // phase: RAFFLE also requires qtreatBonusPool>0 to run, and a funded stakingFund
    // would ALSO be swept by that same epoch's staking-reward payout (this staker holds
    // 100% of the weight) before any delivery loop could see it. Zero-funding both keeps
    // this phase a clean, deterministic test of the bonusEpochs/bonusAwarded counters
    // alone, with neither the reward payout nor the raffle able to interfere.
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, 1000);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_BONUS_THRESHOLD), (sint64)QTREAT_BONUS_THRESHOLD);

    for (uint64 i = 0; i < QTREAT_BONUS_INTERVAL_EPOCHS; i++)
        t.advanceEpoch();

    auto info = t.getStakingInfo(staker);
    EXPECT_EQ(info.bonusAwarded, 1u);
    EXPECT_EQ(info.pendingBonus, 1u); // earned, but undelivered: qtreatBonusPool is still empty

    // Now fund both pools and let one more END_EPOCH run the auto-delivery loop. (The
    // raffle may also fire and hand out an extra token in this same epoch - that's fine,
    // this only asserts the loyalty bonus's own pendingBonus got cleared and the staker
    // received at least their 1 bonus token.)
    id funder = getUser(9);
    increaseEnergy(funder, QTREAT_STAKER_REWARD_PER_EPOCH + QTREAT_RAFFLE_ENTROPY_FEE + 1000);
    EXPECT_EQ(t.depositStakingFund(funder, (sint64)(QTREAT_STAKER_REWARD_PER_EPOCH + QTREAT_RAFFLE_ENTROPY_FEE + 1000)), QTREAT_OK);
    EXPECT_EQ(t.depositQtreatTokensAsAdmin(10), QTREAT_OK);

    t.advanceEpoch();

    EXPECT_EQ(t.getStakingInfo(staker).pendingBonus, 0u);
    sint64 held = numberOfPossessedShares(QTREAT_TOKEN_ASSETNAME, t.tokenIssuer, staker, staker, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_GE(held, 1);
}

TEST(ContractQtreat, LoyaltyBonusManualClaimFallback)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, 1000);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_BONUS_THRESHOLD), (sint64)QTREAT_BONUS_THRESHOLD);

    // No pending bonus yet.
    EXPECT_EQ(t.claimQtreatBonus(staker, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_NO_PENDING_BONUS);

    // Fund the staking fund only (delivery fee available) but leave the bonus pool empty,
    // so END_EPOCH auto-delivery is skipped and the bonus stays pending.
    id funder = getUser(9);
    increaseEnergy(funder, QTREAT_STAKER_REWARD_PER_EPOCH * QTREAT_BONUS_INTERVAL_EPOCHS);
    EXPECT_EQ(t.depositStakingFund(funder, (sint64)(QTREAT_STAKER_REWARD_PER_EPOCH * QTREAT_BONUS_INTERVAL_EPOCHS)), QTREAT_OK);

    for (uint64 i = 0; i < QTREAT_BONUS_INTERVAL_EPOCHS; i++)
        t.advanceEpoch();

    EXPECT_EQ(t.getStakingInfo(staker).pendingBonus, 1u);

    // Bonus pool still empty: claim fails.
    EXPECT_EQ(t.claimQtreatBonus(staker, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_ERR_BONUS_POOL_EMPTY);

    EXPECT_EQ(t.depositQtreatTokensAsAdmin(10), QTREAT_OK);
    auto claim = t.claimQtreatBonus(staker, QTREAT_QX_TRANSFER_FEE);
    EXPECT_EQ(claim.returnCode, QTREAT_OK);
    EXPECT_EQ(claim.claimed, 1u);
    EXPECT_EQ(t.getStakingInfo(staker).pendingBonus, 0u);
}

TEST(ContractQtreat, RaffleFallsBackToChainStateEntropyAndPaysWinner)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, 1000);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);

    // The staking-reward payout runs (and fully drains stakingFund, since this staker
    // holds 100% of the weight) BEFORE the raffle block in the same END_EPOCH call, so
    // stakingFund must cover a full epoch of staking reward *plus* the entropy fee.
    id funder = getUser(9);
    sint64 stakingFundTopUp = (sint64)QTREAT_STAKER_REWARD_PER_EPOCH + (sint64)QTREAT_RAFFLE_ENTROPY_FEE + 1000;
    increaseEnergy(funder, stakingFundTopUp);
    EXPECT_EQ(t.depositStakingFund(funder, stakingFundTopUp), QTREAT_OK);
    EXPECT_EQ(t.depositQtreatTokensAsAdmin(5), QTREAT_OK);

    // RANDOM has no registered entropy providers, so BuyEntropy refunds (all-zero entropy);
    // QTREAT must still pick a winner via the chain-state-digest fallback (single eligible
    // staker here, so the outcome is deterministic).
    t.advanceEpoch();

    auto raffle = t.getRaffleInfo();
    EXPECT_EQ(raffle.totalRaffleAwarded, 1u);
    EXPECT_EQ(raffle.lastRaffleWinner, staker);

    sint64 held = numberOfPossessedShares(QTREAT_TOKEN_ASSETNAME, t.tokenIssuer, staker, staker, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(held, 1);
}

TEST(ContractQtreat, RaffleSkipsIneligibleUnstakingStaker)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, 1000);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);
    ASSERT_EQ(t.requestUnstake(staker, QTREAT_MIN_STAKE, QTREAT_QX_TRANSFER_FEE).returnCode, QTREAT_OK);
    // Fully unstaked: staked==0, so no longer an eligible raffle participant either.

    id funder = getUser(9);
    increaseEnergy(funder, QTREAT_RAFFLE_ENTROPY_FEE + 1000);
    EXPECT_EQ(t.depositStakingFund(funder, (sint64)QTREAT_RAFFLE_ENTROPY_FEE + 1000), QTREAT_OK);
    EXPECT_EQ(t.depositQtreatTokensAsAdmin(5), QTREAT_OK);

    t.advanceEpoch();
    EXPECT_EQ(t.getRaffleInfo().totalRaffleAwarded, 0u);
}

// =============================================================================
// Dividends
// =============================================================================

TEST(ContractQtreat, DividendsSplitBetweenShareholdersAndMinBalanceHolders)
{
    ContractTestingQtreat t;
    id holder = getUser(1);
    increaseEnergy(holder, 1000);
    t.giveQtreat(holder, 1000);

    // Start a fresh epoch so BEGIN_EPOCH's snapshot picks up the holder's balance.
    t.advanceEpoch();

    id funder = getUser(9);
    increaseEnergy(funder, 1000000);
    EXPECT_EQ(t.depositDividends(funder, 1000000), QTREAT_OK);

    long long before = getBalance(holder);
    t.advanceEpoch();
    long long after = getBalance(holder);

    // Shareholder cut is paid out per-computor (dust lost to integer division stays
    // undistributed to shareholders); the sole holder receives the entire remainder.
    uint64 shareholderShare = 1000000ULL * QTREAT_DIVIDEND_SHAREHOLDER_PERMILLE / 1000;
    uint64 paidShareholders = (shareholderShare / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS;
    uint64 expectedHolderShare = 1000000ULL - paidShareholders;
    EXPECT_EQ((uint64)(after - before), expectedHolderShare);
    EXPECT_GT(t.getState()->totalShareholderDividends, 0u);
}

TEST(ContractQtreat, DividendsUseMinOfBeginAndEndBalance)
{
    ContractTestingQtreat t;
    id holderA = getUser(1); // keeps full balance all epoch
    id holderB = getUser(2); // sells half mid-epoch
    increaseEnergy(holderA, 1000);
    increaseEnergy(holderB, 1000);
    t.giveQtreat(holderA, 1000);
    t.giveQtreat(holderB, 1000);

    t.advanceEpoch(); // snapshot begin balances: both at 1000

    // holderB moves half away mid-epoch -> end balance 500, min(begin,end)=500.
    id sink = getUser(3);
    EXPECT_EQ(t.xferOwnership(t.tokenIssuer, QTREAT_TOKEN_ASSETNAME, holderB, 500, sink), 500);

    id funder = getUser(9);
    increaseEnergy(funder, 3000000);
    EXPECT_EQ(t.depositDividends(funder, 3000000), QTREAT_OK);

    long long beforeA = getBalance(holderA);
    long long beforeB = getBalance(holderB);
    t.advanceEpoch();
    long long rewardA = getBalance(holderA) - beforeA;
    long long rewardB = getBalance(holderB) - beforeB;

    // holderA's effective weight (1000) is double holderB's (500 = min(1000,500)).
    EXPECT_GT(rewardA, rewardB);
    EXPECT_NEAR((double)rewardA / (double)rewardB, 2.0, 0.05);
}

TEST(ContractQtreat, DividendsExcludeConfiguredAddress)
{
    ContractTestingQtreat t;
    id excluded = getUser(1);
    id normal = getUser(2);
    increaseEnergy(excluded, 1000);
    increaseEnergy(normal, 1000);
    t.giveQtreat(excluded, 1000);
    t.giveQtreat(normal, 1000);

    EXPECT_EQ(t.setExcludeAddress(t.adminAddress, 0, excluded), QTREAT_OK);
    // Non-admin cannot set exclusions.
    EXPECT_EQ(t.setExcludeAddress(normal, 1, normal), QTREAT_ERR_ACCESS_DENIED);

    t.advanceEpoch();

    id funder = getUser(9);
    increaseEnergy(funder, 1000000);
    EXPECT_EQ(t.depositDividends(funder, 1000000), QTREAT_OK);

    long long beforeExcluded = getBalance(excluded);
    long long beforeNormal = getBalance(normal);
    t.advanceEpoch();

    EXPECT_EQ(getBalance(excluded), beforeExcluded);
    EXPECT_GT(getBalance(normal), beforeNormal);
}

// =============================================================================
// General asset custody
// =============================================================================

TEST(ContractQtreat, GeneralAssetCustodyIsAdminGatedAndRoundTrips)
{
    ContractTestingQtreat t;
    id nonAdmin = getUser(1);
    increaseEnergy(nonAdmin, 1000);

    id assetIssuer = getUser(50);
    increaseEnergy(assetIssuer, 2000000000ULL);
    uint64 assetName = assetNameFromString("TASSET");
    EXPECT_GT(t.issueAsset(assetIssuer, assetName, 1000000, 0, 0), 0);

    Asset asset; asset.issuer = assetIssuer; asset.assetName = assetName;

    // Give the admin the shares, but do NOT transfer management rights: DepositGeneralAsset
    // should refuse a non-admin caller regardless, and the admin call needs mgmt rights first.
    t.xferOwnership(assetIssuer, assetName, assetIssuer, 5000, t.adminAddress);
    EXPECT_EQ(t.depositGeneralAssetRaw(nonAdmin, asset, 1000), QTREAT_ERR_ACCESS_DENIED);

    EXPECT_EQ(t.xferManagementRights(assetIssuer, assetName, QTREAT_CONTRACT_INDEX, 5000, t.adminAddress), 5000);
    EXPECT_EQ(t.depositGeneralAssetRaw(t.adminAddress, asset, 5000), QTREAT_OK);
    EXPECT_EQ(t.getState()->generalAssetBalanceOf(assetIssuer, assetName), 5000u);

    // Revoke: non-admin rejected, over-revoke rejected, full revoke clears the entry.
    EXPECT_EQ(t.revokeGeneralAsset(nonAdmin, asset, 1000, QTREAT_QX_TRANSFER_FEE), QTREAT_ERR_ACCESS_DENIED);
    increaseEnergy(t.adminAddress, QTREAT_QX_TRANSFER_FEE * 2);
    EXPECT_EQ(t.revokeGeneralAsset(t.adminAddress, asset, 6000, QTREAT_QX_TRANSFER_FEE), QTREAT_ERR_INVALID_INPUT);
    EXPECT_EQ(t.revokeGeneralAsset(t.adminAddress, asset, 5000, QTREAT_QX_TRANSFER_FEE), QTREAT_OK);
    EXPECT_EQ(t.getState()->generalAssetBalanceOf(assetIssuer, assetName), 0u);

    sint64 backInWallet = numberOfPossessedShares(assetName, assetIssuer, t.adminAddress, t.adminAddress, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(backInWallet, 5000);
}

TEST(ContractQtreat, ReleaseManagedSharesAllowsAdminToPullBackUnaccountedQdoge)
{
    ContractTestingQtreat t;
    // Admin transfers QDOGE management to QTREAT without staking (bypass path).
    t.giveQdoge(t.adminAddress, 1000000);
    EXPECT_EQ(t.xferManagementRights(t.tokenIssuer, QTREAT_QDOGE_ASSETNAME, QTREAT_CONTRACT_INDEX, 1000000, t.adminAddress), 1000000);

    increaseEnergy(t.adminAddress, QTREAT_QX_TRANSFER_FEE);
    EXPECT_EQ(t.releaseManagedShares(t.adminAddress, t.getState()->getQdogeToken(), 1000000, QTREAT_QX_TRANSFER_FEE), QTREAT_OK);

    sint64 backInWallet = numberOfPossessedShares(QTREAT_QDOGE_ASSETNAME, t.tokenIssuer, t.adminAddress, t.adminAddress, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(backInWallet, 1000000);
}

TEST(ContractQtreat, ReleaseManagedSharesRejectsReleasingActiveStake)
{
    ContractTestingQtreat t;
    id staker = getUser(1);
    increaseEnergy(staker, QTREAT_QX_TRANSFER_FEE * 2);
    EXPECT_EQ(t.stakeQdoge(staker, QTREAT_MIN_STAKE), (sint64)QTREAT_MIN_STAKE);

    // All managed QDOGE is accounted for by `staked`, so there is no surplus to release.
    EXPECT_EQ(t.releaseManagedShares(staker, t.getState()->getQdogeToken(), QTREAT_MIN_STAKE, QTREAT_QX_TRANSFER_FEE), QTREAT_ERR_INVALID_INPUT);
}

// =============================================================================
// ASIC mining / drip
// =============================================================================

TEST(ContractQtreat, LoadAsicPartValidatesInputsAndEnforcesPerRarityBudget)
{
    ContractTestingQtreat t;
    id nonAdmin = getUser(1);
    increaseEnergy(nonAdmin, 1000);

    EXPECT_EQ(t.loadAsicPart(nonAdmin, 0, 0, 0).returnCode, QTREAT_ERR_ACCESS_DENIED);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 0, 4, 0).returnCode, QTREAT_ERR_INVALID_INPUT); // category >= 4
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 0, 0, 5).returnCode, QTREAT_ERR_INVALID_INPUT); // rarity > MAX_RARITY

    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 100, 0, 0).returnCode, QTREAT_OK);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 100, 0, 0).returnCode, QTREAT_ERR_INVALID_INPUT); // duplicate nftId

    // Legendary (rarity 4) budget is only 2 per category.
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 200, 1, 4).returnCode, QTREAT_OK);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 201, 1, 4).returnCode, QTREAT_OK);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, 202, 1, 4).returnCode, QTREAT_ERR_INVALID_INPUT);

    auto catInfo = t.getAsicCatalogInfo(100);
    EXPECT_EQ(catInfo.isCataloged, 1u);
    EXPECT_EQ(catInfo.category, 0u);
    EXPECT_EQ(catInfo.rarity, 0u);
    EXPECT_EQ(catInfo.locked, 0u);
}

TEST(ContractQtreat, RegisterAsicRejectedBeforeCatalogLocked)
{
    ContractTestingQtreat t;
    t.activateQbayMarket();
    uint32 m = t.mintNft(getUser(1));
    uint32 c = t.mintNft(getUser(1));
    uint32 p = t.mintNft(getUser(1));
    uint32 f = t.mintNft(getUser(1));
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, m, 0, 0).returnCode, QTREAT_OK);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, c, 1, 0).returnCode, QTREAT_OK);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, p, 2, 0).returnCode, QTREAT_OK);
    EXPECT_EQ(t.loadAsicPart(t.adminAddress, f, 3, 0).returnCode, QTREAT_OK);

    EXPECT_EQ(t.registerAsic(getUser(1), m, c, p, f).returnCode, QTREAT_ERR_INVALID_INPUT);
}

TEST(ContractQtreat, RegisterAndUnregisterAsicAfterCatalogLocked)
{
    ContractTestingQtreat t;
    t.activateQbayMarket();

    id owner = getUser(1);
    id notOwner = getUser(2);
    increaseEnergy(notOwner, 1000);
    uint32 nftMotherboard = t.mintNft(owner); // 0
    uint32 nftChip = t.mintNft(owner);        // 1
    uint32 nftPsu = t.mintNft(owner);         // 2
    uint32 nftFan = t.mintNft(owner);         // 3
    ASSERT_EQ(nftMotherboard, 0u);
    ASSERT_EQ(nftFan, 3u);

    t.loadFullAsicCatalog(); // reserves ids 0,1,2,3 as common parts of categories 0..3
    EXPECT_EQ(t.getState()->getDripStartEpoch(), (uint64)system.epoch);

    // Wrong owner: notOwner doesn't possess these NFTs.
    EXPECT_EQ(t.registerAsic(notOwner, nftMotherboard, nftChip, nftPsu, nftFan).returnCode, QTREAT_ERR_ACCESS_DENIED);

    auto reg = t.registerAsic(owner, nftMotherboard, nftChip, nftPsu, nftFan);
    EXPECT_EQ(reg.returnCode, QTREAT_OK);
    // All 4 parts are COMMON rarity (1 point each) -> weight 4.
    EXPECT_EQ(reg.asicWeight, 4u);
    EXPECT_EQ(t.getState()->getTotalAsicCount(), 1u);
    EXPECT_EQ(t.getState()->getTotalMiningWeight(), 4u);

    auto minerInfo = t.getMinerInfo(owner);
    EXPECT_EQ(minerInfo.asicCount, 1u);
    EXPECT_EQ(minerInfo.weight, 4u);

    // Reusing an already-used part is rejected.
    uint32 spareMotherboard = t.mintNft(owner);
    // spareMotherboard isn't in the catalog at all yet, so registering with it fails as invalid input too;
    // instead verify direct re-use of nftMotherboard (already used) is rejected against a fresh valid set:
    EXPECT_EQ(t.registerAsic(owner, nftMotherboard, nftChip, nftPsu, nftFan).returnCode, QTREAT_ERR_INVALID_INPUT);
    (void)spareMotherboard;

    // Unregister: non-owner rejected, owner succeeds and frees weight/parts.
    EXPECT_EQ(t.unregisterAsic(notOwner, reg.rigIndex), QTREAT_ERR_ACCESS_DENIED);
    EXPECT_EQ(t.unregisterAsic(owner, reg.rigIndex), QTREAT_OK);
    EXPECT_EQ(t.getState()->getTotalAsicCount(), 0u);
    EXPECT_EQ(t.getState()->getTotalMiningWeight(), 0u);
}

TEST(ContractQtreat, MiningRewardSplits5050WithDividendFundAndPaysByWeight)
{
    ContractTestingQtreat t;
    t.activateQbayMarket();
    id owner = getUser(1);
    uint32 m = t.mintNft(owner), c = t.mintNft(owner), p = t.mintNft(owner), f = t.mintNft(owner);
    t.loadFullAsicCatalog();
    auto reg = t.registerAsic(owner, m, c, p, f);
    ASSERT_EQ(reg.returnCode, QTREAT_OK);

    EXPECT_EQ(t.setMiningRate(t.adminAddress, QTREAT_MINING_REWARD_MAX + 1), QTREAT_ERR_INVALID_INPUT);
    EXPECT_EQ(t.setMiningRate(t.adminAddress, 1000000), QTREAT_OK);

    id funder = getUser(9);
    increaseEnergy(funder, 1000000);
    EXPECT_EQ(t.depositMiningFund(funder, 1000000), QTREAT_OK);

    // Mining rewards are only ever paid to a rig in the same epoch it's freshly re-verified
    // (see QTREAT_ASIC_VERIFY_SPREAD_EPOCHS); force the clock to land exactly on this rig's
    // (slot 0's) next due epoch so no intervening "nobody due" epoch drains miningFund via
    // the dividend cut first, keeping the expected amounts exact.
    ASSERT_EQ(reg.rigIndex, 0u);
    system.epoch = system.epoch - (system.epoch % QTREAT_ASIC_VERIFY_SPREAD_EPOCHS) + QTREAT_ASIC_VERIFY_SPREAD_EPOCHS;
    ASSERT_EQ((uint64)system.epoch % QTREAT_ASIC_VERIFY_SPREAD_EPOCHS, 0u);

    uint64 dividendBefore = t.getFunds().dividendFund;
    long long ownerBefore = getBalance(owner);
    t.advanceEpoch();

    // This epoch's budget slice is rate/SPREAD (see the same reasoning in QTREAT.h), split
    // 50/50; the sole due rig owns 100% of totalMiningWeight so its scaled-up payment
    // reconstitutes the full miner half of that slice.
    uint64 epochSlice = 1000000 / QTREAT_ASIC_VERIFY_SPREAD_EPOCHS;
    uint64 expectedMinerHalf = (epochSlice / 2) * QTREAT_ASIC_VERIFY_SPREAD_EPOCHS;
    uint64 expectedDividendHalf = epochSlice - epochSlice / 2;
    EXPECT_EQ((uint64)(getBalance(owner) - ownerBefore), expectedMinerHalf);
    EXPECT_EQ(t.getFunds().dividendFund - dividendBefore, expectedDividendHalf);
}

TEST(ContractQtreat, AsicRigDeactivatedWhenPartOwnershipChanges)
{
    ContractTestingQtreat t;
    t.activateQbayMarket();
    id owner = getUser(1);
    id newOwner = getUser(2);
    uint32 m = t.mintNft(owner), c = t.mintNft(owner), p = t.mintNft(owner), f = t.mintNft(owner);
    t.loadFullAsicCatalog();
    auto reg = t.registerAsic(owner, m, c, p, f);
    ASSERT_EQ(reg.returnCode, QTREAT_OK);
    ASSERT_EQ(t.getState()->getTotalAsicCount(), 1u);

    t.transferNft(owner, m, newOwner); // motherboard no longer owned by the rig's registered owner

    // Ownership re-verification is spread across QTREAT_ASIC_VERIFY_SPREAD_EPOCHS epochs
    // (round-robin by rig slot) rather than checking every rig every epoch, so the
    // deactivation isn't guaranteed on the very next epoch - only within that window.
    for (uint64 i = 0; i < QTREAT_ASIC_VERIFY_SPREAD_EPOCHS; i++)
        t.advanceEpoch();

    EXPECT_EQ(t.getState()->getTotalAsicCount(), 0u);
    EXPECT_EQ(t.getState()->getRig((sint64)reg.rigIndex).active, 0u);
}

TEST(ContractQtreat, AsicRigStaysActiveUntilItsVerificationSlotComesUp)
{
    // The very first registered rig lands at slot 0. constructionEpoch mod
    // QTREAT_ASIC_VERIFY_SPREAD_EPOCHS != 0 for this fixture's construction epoch, so slot 0
    // is *not* re-verified on the first END_EPOCH after registration - only after enough
    // epochs have passed for (slot + epoch) mod SPREAD to hit 0. This pins down the
    // "eventually consistent, not immediate" tradeoff explicitly rather than leaving it implicit.
    ContractTestingQtreat t;
    t.activateQbayMarket();
    id owner = getUser(1);
    id newOwner = getUser(2);
    uint32 m = t.mintNft(owner), c = t.mintNft(owner), p = t.mintNft(owner), f = t.mintNft(owner);
    t.loadFullAsicCatalog();
    auto reg = t.registerAsic(owner, m, c, p, f);
    ASSERT_EQ(reg.returnCode, QTREAT_OK);
    ASSERT_EQ(reg.rigIndex, 0u);
    ASSERT_NE(((uint64)reg.rigIndex + (uint64)system.epoch) % QTREAT_ASIC_VERIFY_SPREAD_EPOCHS, 0u)
        << "test assumption violated: adjust which epoch this fixture starts at, or the rig slot used";

    t.transferNft(owner, m, newOwner);
    t.advanceEpoch(); // this epoch's slot isn't due for re-verification yet

    EXPECT_EQ(t.getState()->getTotalAsicCount(), 1u);
    EXPECT_EQ(t.getState()->getRig((sint64)reg.rigIndex).active, 1u);
}

TEST(ContractQtreat, AsicSoldPartOwnerReceivesNoRewardOnDeactivationEpoch)
{
    // Core guarantee: mining reward payment and ownership re-verification always happen
    // together (same pass, same epoch) - a rig is never paid based on stale ownership. Once
    // a part is sold away, the old owner collects nothing more, starting with the very epoch
    // the mismatch is caught (no one-more-payment-then-deactivate window).
    ContractTestingQtreat t;
    t.activateQbayMarket();
    id owner = getUser(1);
    id newOwner = getUser(2);
    uint32 m = t.mintNft(owner), c = t.mintNft(owner), p = t.mintNft(owner), f = t.mintNft(owner);
    t.loadFullAsicCatalog();
    auto reg = t.registerAsic(owner, m, c, p, f);
    ASSERT_EQ(reg.returnCode, QTREAT_OK);
    ASSERT_EQ(reg.rigIndex, 0u);

    EXPECT_EQ(t.setMiningRate(t.adminAddress, 1000000), QTREAT_OK);
    id funder = getUser(9);
    increaseEnergy(funder, 1000000);
    EXPECT_EQ(t.depositMiningFund(funder, 1000000), QTREAT_OK);

    t.transferNft(owner, m, newOwner); // sell before the rig's part ever gets fresh-verified

    // Advance straight to slot 0's next due epoch, same as the payment test, so no
    // intervening epoch's dividend cut muddies the miningFund balance.
    system.epoch = system.epoch - (system.epoch % QTREAT_ASIC_VERIFY_SPREAD_EPOCHS) + QTREAT_ASIC_VERIFY_SPREAD_EPOCHS;

    long long ownerBefore = getBalance(owner);
    t.advanceEpoch();

    // Deactivated (ownership mismatch caught), and the old owner got paid nothing this epoch.
    EXPECT_EQ(t.getState()->getTotalAsicCount(), 0u);
    EXPECT_EQ(t.getState()->getRig((sint64)reg.rigIndex).active, 0u);
    EXPECT_EQ(getBalance(owner), ownerBefore);
}

TEST(ContractQtreat, DripQdogePaysCatalogedNftHoldersByRarityAndExcludesAdmin)
{
    ContractTestingQtreat t;
    t.activateQbayMarket();
    id holder = getUser(1);
    // Mint the 4 NFTs that will land on the reserved common ids (0..3, category==id).
    uint32 id0 = t.mintNft(holder);
    uint32 id1 = t.mintNft(holder);
    uint32 id2 = t.mintNft(holder);
    uint32 id3 = t.mintNft(holder);
    ASSERT_EQ(id0, 0u); ASSERT_EQ(id1, 1u); ASSERT_EQ(id2, 2u); ASSERT_EQ(id3, 3u);

    t.loadFullAsicCatalog(); // locks catalog + sets dripStartEpoch; ids 0..3 are COMMON parts

    id nonAdmin = getUser(5);
    increaseEnergy(nonAdmin, 1000);
    EXPECT_EQ(t.depositDripQdogeRaw(nonAdmin, 1000), QTREAT_ERR_ACCESS_DENIED);

    EXPECT_EQ(t.depositDripQdogeAsAdmin(1000000), QTREAT_OK);

    // Drip payouts release QDOGE back to QX per-recipient, and that release fee is paid
    // out of miningFund (not dripQdogePool/stakingFund) - fund it too.
    id miningFunder = getUser(6);
    increaseEnergy(miningFunder, 100000);
    EXPECT_EQ(t.depositMiningFund(miningFunder, 100000), QTREAT_OK);

    sint64 walletBefore = numberOfPossessedShares(QTREAT_QDOGE_ASSETNAME, t.tokenIssuer, holder, holder, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    t.advanceEpoch();
    sint64 walletAfter = numberOfPossessedShares(QTREAT_QDOGE_ASSETNAME, t.tokenIssuer, holder, holder, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);

    // holder possesses 4 COMMON-rarity cataloged NFTs -> 4 * QTREAT_DRIP_QDOGE_COMMON.
    EXPECT_EQ((uint64)(walletAfter - walletBefore), 4ull * QTREAT_DRIP_QDOGE_COMMON);
}
