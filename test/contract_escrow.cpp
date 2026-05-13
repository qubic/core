#define NO_UEFI

#include "contract_testing.h"

const id testAddress1 = ID(_H, _O, _G, _T, _K, _D, _N, _D, _V, _U, _U, _Z, _U, _F, _L, _A, _M, _L, _V, _B, _L, _Z, _D, _S, _G, _D, _D, _A, _E, _B, _E, _K, _K, _L, _N, _Z, _J, _B, _W, _S, _C, _A, _M, _D, _S, _X, _T, _C, _X, _A, _M, _A, _X, _U, _D, _F);
const id testAddress2 = ID(_E, _Q, _M, _B, _B, _V, _Y, _G, _Z, _O, _F, _U, _I, _H, _E, _X, _F, _O, _X, _K, _T, _F, _T, _A, _N, _E, _K, _B, _X, _L, _B, _X, _H, _A, _Y, _D, _F, _F, _M, _R, _E, _E, _M, _R, _Q, _E, _V, _A, _D, _Y, _M, _M, _E, _W, _A, _C);
const id shareHolder = ID(_U, _F, _X, _W, _I, _Y, _K, _G, _D, _G, _W, _Q, _N, _D, _A, _V, _T, _G, _L, _R, _L, _W, _S, _N, _D, _Q, _Y, _C, _K, _O, _S, _Y, _K, _P, _S, _Q, _G, _Y, _P, _P, _U, _F, _H, _K, _H, _K, _Y, _E, _Y, _P, _Z, _K, _Z, _Z, _N, _B);

enum class EscrowOperation : unsigned short
{
    AcceptDeal = 2,
    MakeDealPublic = 3,
    CancelDeal = 4
};

class EscrowChecker : public ESCROW, public ESCROW::StateData
{
public:
    uint64_t getDealsAmount()
    {
        return _deals.population();
    }

    uint64_t getDealsAmount(const id& pov)
    {
        return _ownerDealIndexes.population(pov);
    }
};

class ContractTestingEscrow : protected ContractTesting
{
public:
    ContractTestingEscrow()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(ESCROW);
        callSystemProcedure(ESCROW_CONTRACT_INDEX, INITIALIZE);
    }

    EscrowChecker* getState()
    {
        return (EscrowChecker*)contractStates[ESCROW_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(ESCROW_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(ESCROW_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void createDeal(const id& caller, const ESCROW::CreateDeal_input& input, const int64_t& quAmount)
    {
        ESCROW::CreateDeal_output output;
        invokeUserProcedure(ESCROW_CONTRACT_INDEX, 1, input, output, caller, quAmount);
    }

    void operateDeal(const id& caller, const int64_t& dealIndex, const EscrowOperation type, const int64_t& quAmount)
    {
        ESCROW::AcceptDeal_input input{ dealIndex };
        ESCROW::AcceptDeal_output output;
        invokeUserProcedure(ESCROW_CONTRACT_INDEX, static_cast<unsigned short>(type), input, output, caller, quAmount);
    }

    ESCROW::GetDeals_output getDeals(const id& user, const int64_t& privateDealsOffset, const int64_t& publicDealsOffset)
    {
        ESCROW::GetDeals_input input{ user, privateDealsOffset, publicDealsOffset };
        ESCROW::GetDeals_output output;
        callFunction(ESCROW_CONTRACT_INDEX, 1, input, output);
        return output;
    }
};

TEST(ContractEscrow, CreateDeal)
{
    system.epoch = 190;
    ContractTestingEscrow escrow;
    escrow.beginEpoch();

    increaseEnergy(testAddress1, 100000000LL);
    increaseEnergy(testAddress2, 100000000LL);

    int issuanceIndex;
    int ownershipIndex;
    int possessionIndex;
    issueAsset(testAddress1, "TOKEN", 0, "000000", 1000000, ESCROW_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);

    ESCROW::AssetWithAmount emptyAsset = { {NULL_ID}, 0, 0 };

    // scenario 1: attempt to create a deal by testAddress1, but not enough commissions
    ESCROW::CreateDeal_input input{};
    input.acceptorId = NULL_ID;
    input.offeredQU = 0;
    input.offeredAssetsNumber = 1;
    input.offeredAssets.setAll(emptyAsset);
    input.offeredAssets.set(0, { testAddress1, assetNameFromString("TOKEN"), 500 });
    input.requestedQU = 1000000;
    input.requestedAssetsNumber = 0;
    input.requestedAssets.setAll(emptyAsset);
    escrow.createDeal(testAddress1, input, 1);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 0);

    // scenario 2: attempt to create a deal by testAddress1, but not enough tokens for offered assets
    input.offeredAssets.set(0, { testAddress1, assetNameFromString("TOKEN"), 1000001 });
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 0);

    // scenario 3: attempt to create a deal by testAddress1, but no assets in offeredAssets and requestedAssets
    input.offeredAssetsNumber = 0;
    input.offeredAssets.setAll(emptyAsset);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 0);

    // scenario 4: successful public deal creation by testAddress1
    input.offeredAssetsNumber = 1;
    input.offeredAssets.set(0, { testAddress1, assetNameFromString("TOKEN"), 1000 });
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 1);

    // scenario 5: successful private deal creation by testAddress2 for testAddress1
    input.acceptorId = testAddress1;
    input.requestedQU = 0;
    input.offeredQU = 1000000;
    input.offeredAssets.setAll(emptyAsset);
    input.requestedAssetsNumber = 1;
    input.requestedAssets.set(0, { testAddress1, assetNameFromString("TOKEN"), 500000 });
    escrow.createDeal(testAddress2, input, 1250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 2);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress2), 1);

    ESCROW::GetDeals_output testAddress1Deals = escrow.getDeals(testAddress1, 0, 0);
    EXPECT_EQ(testAddress1Deals.ownedDealsAmount, 1);
    EXPECT_EQ(testAddress1Deals.publicDealsAmount, 0);
    EXPECT_EQ(testAddress1Deals.proposedDealsAmount, 1);

    ESCROW::GetDeals_output testAddress2Deals = escrow.getDeals(testAddress2, 0, 0);
    EXPECT_EQ(testAddress2Deals.ownedDealsAmount, 1);
    EXPECT_EQ(testAddress2Deals.publicDealsAmount, 1);
    EXPECT_EQ(testAddress2Deals.proposedDealsAmount, 0);

    // scenario 6: attempt to create more than 8 deals by testAddress1
    input.acceptorId = NULL_ID;
    input.offeredQU = 0;
    input.offeredAssetsNumber = 1;
    input.offeredAssets.set(0, { testAddress1, assetNameFromString("TOKEN"), 1 });
    input.requestedQU = 10;
    input.requestedAssetsNumber = 0;
    input.requestedAssets.setAll(emptyAsset);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 2);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 3);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 4);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 5);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 6);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 7);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 8);
    escrow.createDeal(testAddress1, input, 250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 8);
}

TEST(ContractEscrow, OperateDeal)
{
    system.epoch = 190;
    ContractTestingEscrow escrow;
    escrow.beginEpoch();

    increaseEnergy(testAddress1, 100000000LL);
    increaseEnergy(testAddress2, 100000000LL);
    increaseEnergy(NULL_ID, 100000000LL);
    int issuanceIndex;
    int ownershipIndex;
    int possessionIndex;
    issueAsset(testAddress2, "TOKEN", 0, "000000", 1000000, ESCROW_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);

    ESCROW::AssetWithAmount emptyAsset = { {NULL_ID}, 0, 0 };

    ESCROW::CreateDeal_input input{};
    input.acceptorId = testAddress2;
    input.offeredQU = 1000000;
    input.offeredAssetsNumber = 0;
    input.offeredAssets.setAll(emptyAsset);
    input.requestedQU = 0;
    input.requestedAssetsNumber = 1;
    input.requestedAssets.setAll(emptyAsset);
    input.requestedAssets.set(0, { testAddress2, assetNameFromString("TOKEN"), 500 });
    escrow.createDeal(testAddress1, input, 1250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(testAddress1), 1);

    ESCROW::GetDeals_output testAddress1Deals = escrow.getDeals(testAddress1, 0, 0);
    EXPECT_EQ(testAddress1Deals.ownedDeals.get(0).acceptorId, testAddress2);

    ESCROW::GetDeals_output testAddress2Deals = escrow.getDeals(testAddress2, 0, 0);
    EXPECT_EQ(testAddress2Deals.proposedDealsAmount, 1);
    EXPECT_EQ(testAddress2Deals.publicDealsAmount, 0);

    // make deal public
    escrow.operateDeal(testAddress1, testAddress1Deals.ownedDeals.get(0).index, EscrowOperation::MakeDealPublic, 0);
    testAddress1Deals = escrow.getDeals(testAddress1, 0, 0);
    EXPECT_EQ(testAddress1Deals.ownedDeals.get(0).acceptorId, id(ESCROW_CONTRACT_INDEX, 0, 0, 0));

    testAddress2Deals = escrow.getDeals(testAddress2, 0, 0);
    EXPECT_EQ(testAddress2Deals.proposedDealsAmount, 0);
    EXPECT_EQ(testAddress2Deals.publicDealsAmount, 1);

    // attempt to cancel deal by not owner address
    escrow.operateDeal(testAddress2, 1, EscrowOperation::CancelDeal, 0);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 1);

    // cancel deal
    escrow.operateDeal(testAddress1, 1, EscrowOperation::CancelDeal, 0);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 0);

    issueAsset(NULL_ID, "ESCROW", 0, "000000", 676, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);
    int destinationOwnershipIndex;
    int destinationPossessionIndex;
    // transfer 10 ESCROW shares to shareHolder address  
    transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareHolder, 10, &destinationOwnershipIndex, &destinationPossessionIndex, true);

    input.requestedAssets.set(0, { testAddress2, assetNameFromString("TOKEN"), 500000 });
    escrow.createDeal(testAddress1, input, 1250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 1);

    // accept deal, success
    escrow.operateDeal(testAddress2, 2, EscrowOperation::AcceptDeal, 1);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 0);

    EXPECT_EQ(numberOfPossessedShares(assetNameFromString("TOKEN"), testAddress2, testAddress1, testAddress1, ESCROW_CONTRACT_INDEX, ESCROW_CONTRACT_INDEX), 495000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString("TOKEN"), testAddress2, id(ESCROW_CONTRACT_INDEX, 0, 0, 0), id(ESCROW_CONTRACT_INDEX, 0, 0, 0), ESCROW_CONTRACT_INDEX, ESCROW_CONTRACT_INDEX), 5000);  // 1% commission

    // token distribution to shareholders
    escrow.endEpoch();
    // shareHolder address has 10 ESCROW shares and should receive: 5000 TOKEN / 676 shares = 7 TOKEN per share, and 7 TOKEN per share * 10 shares = 70 TOKEN
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString("TOKEN"), testAddress2, shareHolder, shareHolder, ESCROW_CONTRACT_INDEX, ESCROW_CONTRACT_INDEX), 70);

    escrow.beginEpoch();
    escrow.createDeal(testAddress2, input, 1250000);
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 1);
    escrow.endEpoch();
    system.epoch += 4;
    escrow.beginEpoch();
    EXPECT_EQ(escrow.getState()->getDealsAmount(), 0);
}
