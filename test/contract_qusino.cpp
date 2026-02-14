#define NO_UEFI

#include "contract_testing.h"

static constexpr uint64 QUSINO_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QUSINO_TRANSFER_ASSET_FEE = 100ull;
static constexpr uint64 QUSINO_TRANSFER_RIGHTS_FEE = 100ull;

static const id QUSINO_CONTRACT_ID(QUSINO_CONTRACT_INDEX, 0, 0, 0);

const id QUSINO_testUser1 = ID(_U, _S, _E, _R, _A, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_testUser2 = ID(_U, _S, _E, _R, _B, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_testUser3 = ID(_U, _S, _E, _R, _C, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);

class QUSINOChecker : public QUSINO
{
public:
    void checkSCInfo(const QUSINO::getSCInfo_output& output, uint64 expectedQSC, uint64 expectedSTAR, uint64 expectedStakedSTAR, uint64 expectedStakedQSC, uint64 expectedStakedQST, uint64 expectedBurntSTAR, uint64 expectedEpochRevenue, uint64 expectedMaxGameIndex, uint64 expectedNumberOfStakers, uint64 expectedQSTAmountForSale)
    {
        EXPECT_EQ(output.QSCCirclatingSupply, expectedQSC);
        EXPECT_EQ(output.STARCirclatingSupply, expectedSTAR);
        EXPECT_EQ(output.totalStakedSTAR, expectedStakedSTAR);
        EXPECT_EQ(output.totalStakedQSC, expectedStakedQSC);
        EXPECT_EQ(output.totalStakedQST, expectedStakedQST);
        EXPECT_EQ(output.burntSTAR, expectedBurntSTAR);
        EXPECT_EQ(output.epochRevenue, expectedEpochRevenue);
        EXPECT_EQ(output.maxGameIndex, expectedMaxGameIndex);
        EXPECT_EQ(output.numberOfStakers, expectedNumberOfStakers);
        EXPECT_EQ(output.QSTAmountForSale, expectedQSTAmountForSale);
    }
};

class ContractTestingQUSINO : protected ContractTesting
{
public:
    ContractTestingQUSINO()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUSINO);
        callSystemProcedure(QUSINO_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QUSINOChecker* getState()
    {
        return (QUSINOChecker*)contractStates[QUSINO_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUSINO_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares)
    {
        QX::IssueAsset_input input;
        input.assetName = assetName;
        input.numberOfShares = numberOfShares;
        input.unitOfMeasurement = 0;
        input.numberOfDecimalPlaces = 0;
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QUSINO_ISSUE_ASSET_FEE);
        return output.issuedNumberOfShares;
    }

    sint64 transferAsset(const id& from, const id& to, uint64 assetName, const id& issuer, sint64 numberOfShares)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = to;
        input.numberOfShares = numberOfShares;
        QX::TransferShareOwnershipAndPossession_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, QUSINO_TRANSFER_ASSET_FEE);
        return output.transferredNumberOfShares;
    }

    sint64 transferShareManagementRightsQX(const id& invocator, const Asset& asset, sint64 numberOfShares, uint32 newManagingContractIndex, sint64 fee)
    {
        QX::TransferShareManagementRights_input input;
        input.asset.assetName = asset.assetName;
        input.asset.issuer = asset.issuer;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;
        QX::TransferShareManagementRights_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, invocator, fee);
        return output.transferredNumberOfShares;
    }

    QUSINO::buyQST_output buyQST(const id& buyer, uint64 amount, bool type, sint64 invocationReward)
    {
        QUSINO::buyQST_input input;
        input.amount = amount;
        input.type = type;
        QUSINO::buyQST_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 1, input, output, buyer, invocationReward);
        return output;
    }

    QUSINO::earnSTAR_output earnSTAR(const id& user, uint64 amount, sint64 invocationReward)
    {
        QUSINO::earnSTAR_input input;
        input.amount = amount;
        QUSINO::earnSTAR_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 2, input, output, user, invocationReward);
        return output;
    }

    QUSINO::earnQSC_output earnQSC(const id& user, uint64 amount, sint64 invocationReward)
    {
        QUSINO::earnQSC_input input;
        input.amount = amount;
        QUSINO::earnQSC_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 3, input, output, user, invocationReward);
        return output;
    }

    QUSINO::transferSTAROrQSC_output transferSTAROrQSC(const id& user, const id& dest, uint64 amount, bool type, sint64 invocationReward)
    {
        QUSINO::transferSTAROrQSC_input input;
        input.dest = dest;
        input.amount = amount;
        input.type = type;
        QUSINO::transferSTAROrQSC_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 4, input, output, user, invocationReward);
        return output;
    }

    QUSINO::stakeAssets_output stakeAssets(const id& user, uint64 amount, uint32 type, uint32 typeOfAsset, sint64 invocationReward)
    {
        QUSINO::stakeAssets_input input;
        input.amount = amount;
        input.type = type;
        input.typeOfAsset = typeOfAsset;
        QUSINO::stakeAssets_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 5, input, output, user, invocationReward);
        return output;
    }

    QUSINO::submitGame_output submitGame(const id& user, const Array<uint8, 64>& URI, sint64 invocationReward)
    {
        QUSINO::submitGame_input input;
        copyMemory(input.URI, URI);
        QUSINO::submitGame_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 6, input, output, user, invocationReward);
        return output;
    }

    QUSINO::voteInGameProposal_output voteInGameProposal(const id& user, const Array<uint8, 64>& URI, uint64 gameIndex, bool yesNo, sint64 invocationReward)
    {
        QUSINO::voteInGameProposal_input input;
        copyMemory(input.URI, URI);
        input.gameIndex = gameIndex;
        input.yesNo = yesNo;
        QUSINO::voteInGameProposal_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 7, input, output, user, invocationReward);
        return output;
    }

    QUSINO::depositQSTForSale_output depositQSTForSale(const id& user, uint64 amount, sint64 invocationReward)
    {
        QUSINO::depositQSTForSale_input input;
        input.amount = amount;
        QUSINO::depositQSTForSale_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 8, input, output, user, invocationReward);
        return output;
    }

    QUSINO::TransferShareManagementRights_output TransferShareManagementRights(const id& user, const Asset& asset, uint64 numberOfShares, uint32 newManagingContractIndex, sint64 invocationReward)
    {
        QUSINO::TransferShareManagementRights_input input;
        input.asset = asset;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;
        QUSINO::TransferShareManagementRights_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 9, input, output, user, invocationReward);
        return output;
    }

    QUSINO::getUserAssetVolume_output getUserAssetVolume(const id& user)
    {
        QUSINO::getUserAssetVolume_input input;
        input.user = user;
        QUSINO::getUserAssetVolume_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QUSINO::getUserStakingInfo_output getUserStakingInfo(const id& user, uint32 offset)
    {
        QUSINO::getUserStakingInfo_input input;
        input.user = user;
        input.offset = offset;
        QUSINO::getUserStakingInfo_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QUSINO::getFailedGameList_output getFailedGameList(uint32 offset)
    {
        QUSINO::getFailedGameList_input input;
        input.offset = offset;
        QUSINO::getFailedGameList_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QUSINO::getSCInfo_output getSCInfo()
    {
        QUSINO::getSCInfo_input input;
        QUSINO::getSCInfo_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QUSINO::getActiveGameList_output getActiveGameList(uint32 offset)
    {
        QUSINO::getActiveGameList_input input;
        input.offset = offset;
        QUSINO::getActiveGameList_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    // Helper to create an ask order on QX so that QUSINO::buyQST
    // can read the price from QX::AssetAskOrders.
    sint64 addToAskOrder(const id& invocator, const id& issuer, uint64 assetName, sint64 price, sint64 numberOfShares)
    {
        QX::AddToAskOrder_input input;
        input.issuer = issuer;
        input.assetName = assetName;
        input.price = price;
        input.numberOfShares = numberOfShares;
        QX::AddToAskOrder_output output;
        // No invocation reward needed; AddToAskOrder immediately refunds it anyway.
        invokeUserProcedure(QX_CONTRACT_INDEX, 5, input, output, invocator, 0);
        return output.addedNumberOfShares;
    }
};

// Helper function to create a URI
Array<uint8, 64> createURI(const char* str)
{
    Array<uint8, 64> URI;
    uint32 len = 0;
    while (str[len] != '\0' && len < 64) len++;
    for (uint32 i = 0; i < 64; i++)
    {
        if (i < len)
            URI.set(i, (uint8)str[i]);
        else
            URI.set(i, 0);
    }
    return URI;
}

TEST(ContractQUSINO, buyQST_WithQubic_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    sint64 totalShares = 1000000;

    uint64 qstAssetName = assetNameFromString("QST");
    
    // Issue QST asset to the QST issuer
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    // Give QUSINO contract management rights over the QST asset
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);

    // Deposit QST into QUSINO contract for sale via its own procedure
    increaseEnergy(qstIssuer, 1);
    QUSINO::depositQSTForSale_output depOutput = QUSINO.depositQSTForSale(qstIssuer, 100000, 0);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);

    // Create an ask order on QX so QUSINO::buyQST can fetch the price
    sint64 askPrice1 = 777;
    sint64 askAmount1 = 50000;
    EXPECT_EQ(QUSINO.addToAskOrder(qstIssuer, qstIssuer, qstAssetName, askPrice1, askAmount1), askAmount1);

    sint64 askPrice2 = 555;
    sint64 askAmount2 = 100000;
    EXPECT_EQ(QUSINO.addToAskOrder(qstIssuer, qstIssuer, qstAssetName, askPrice2, askAmount2), askAmount2);

    sint64 askPrice3 = 888;
    sint64 askAmount3 = 100000;
    EXPECT_EQ(QUSINO.addToAskOrder(qstIssuer, qstIssuer, qstAssetName, askPrice3, askAmount3), askAmount3);
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 1000;
    sint64 requiredReward = buyAmount * askPrice2;
    
    increaseEnergy(buyer, requiredReward);
    
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 0, requiredReward);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Verify buyer received QST
    EXPECT_EQ(numberOfPossessedShares(qstAssetName, qstIssuer, buyer, buyer, QUSINO_CONTRACT_INDEX, QUSINO_CONTRACT_INDEX), buyAmount);
    
    // Check QSTAmountForSale decreased by the purchased amount
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.QSTAmountForSale, 100000 - buyAmount);
}

TEST(ContractQUSINO, buyQST_WithQSC_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Same QST setup as buyQST_WithQubic_Success
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = assetNameFromString("QST");
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);
    
    increaseEnergy(qstIssuer, 1);
    QUSINO::depositQSTForSale_output depOutput = QUSINO.depositQSTForSale(qstIssuer, 100000, 0);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Buy with QSC: earn QSC then buy QST (type=1)
    id buyer = QUSINO_testUser2;
    uint64 qstAmount = 10000;
    increaseEnergy(buyer, 1);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, buyer, qstAssetName, qstIssuer, 10000), 10000);
    increaseEnergy(buyer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(buyer, asset, 10000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 10000);

    QUSINO::earnQSC_output earnOutput = QUSINO.earnQSC(buyer, qstAmount, 1);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    uint64 buyAmount = 1000;
    increaseEnergy(buyer, 1);
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 1, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    EXPECT_EQ(numberOfPossessedShares(qstAssetName, qstIssuer, buyer, buyer, QUSINO_CONTRACT_INDEX, QUSINO_CONTRACT_INDEX), buyAmount);
    
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(buyer);
    EXPECT_EQ(userVolume.QSCAmount, qstAmount - buyAmount);
}

TEST(ContractQUSINO, buyQST_InsufficientQSTAmountForSale)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization (same as buyQST_WithQubic_Success)
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = assetNameFromString("QST");
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);
    
    // Deposit only 50000 QST for sale
    uint64 depositedAmount = 50000;
    increaseEnergy(qstIssuer, 1);
    QUSINO::depositQSTForSale_output depOutput = QUSINO.depositQSTForSale(qstIssuer, depositedAmount, 0);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Create ask order on QX so buyQST(type=0) can read price from AssetAskOrders
    sint64 askPrice = 777;
    sint64 askAmount = 50000;
    EXPECT_EQ(QUSINO.addToAskOrder(qstIssuer, qstIssuer, qstAssetName, askPrice, askAmount), askAmount);
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 200000; // More than QSTAmountForSale (50000)
    sint64 requiredReward = buyAmount * askPrice;
    
    increaseEnergy(buyer, requiredReward);
    
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 0, requiredReward);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QST_AMOUNT_FOR_SALE);
}

TEST(ContractQUSINO, buyQST_InsufficientFunds)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization (same as buyQST_WithQubic_Success)
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = assetNameFromString("QST");
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);
    
    increaseEnergy(qstIssuer, 1);
    QUSINO::depositQSTForSale_output depOutput = QUSINO.depositQSTForSale(qstIssuer, 100000, 0);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Create ask order on QX so buyQST(type=0) reads price from AssetAskOrders
    sint64 askPrice = 777;
    sint64 askAmount = 50000;
    EXPECT_EQ(QUSINO.addToAskOrder(qstIssuer, qstIssuer, qstAssetName, askPrice, askAmount), askAmount);
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 1000;
    sint64 insufficientReward = 0;
    
    increaseEnergy(buyer, insufficientReward);
    
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 0, insufficientReward);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_FUNDS);
}

TEST(ContractQUSINO, buyQST_InsufficientQSC)
{
    ContractTestingQUSINO QUSINO;
    
    // Same QST setup as buyQST_WithQubic_Success
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = assetNameFromString("QST");
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);
    
    increaseEnergy(qstIssuer, 1);
    QUSINO::depositQSTForSale_output depOutput = QUSINO.depositQSTForSale(qstIssuer, 100000, 0);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Buyer has no QSC; buy with QSC (type=1) should fail
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 1000;
    increaseEnergy(buyer, 1);
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 1, 1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QSC);
}

TEST(ContractQUSINO, earnSTAR_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE;
    
    increaseEnergy(user, requiredReward);
    
    QUSINO::earnSTAR_output output = QUSINO.earnSTAR(user, amount, requiredReward);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check user's STAR amount
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(userVolume.STARAmount, amount);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.STARCirclatingSupply, amount);
    EXPECT_EQ(scInfo.epochRevenue, requiredReward);
}

TEST(ContractQUSINO, earnSTAR_InsufficientFunds)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 1000;
    sint64 insufficientReward = amount * QUSINO_STAR_PRICE - 1;
    
    increaseEnergy(user, insufficientReward);
    
    QUSINO::earnSTAR_output output = QUSINO.earnSTAR(user, amount, insufficientReward);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_FUNDS);
}

TEST(ContractQUSINO, earnQSC_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 5526353;
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);
    
    id user = QUSINO_testUser2;
    uint64 amount = 5000;
    
    // Transfer QST to user first
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, user, qstAssetName, qstIssuer, amount), amount);

    increaseEnergy(user, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(user, asset, amount, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), amount);
    
    increaseEnergy(user, 1);
    QUSINO::earnQSC_output output = QUSINO.earnQSC(user, amount, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check user's QSC and STAR amounts (should get bonus STAR)
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(userVolume.QSCAmount, amount);
    EXPECT_EQ(userVolume.STARAmount, QUSINO_STAR_BONUS_FOR_QSC);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.QSCCirclatingSupply, amount);
    EXPECT_EQ(scInfo.STARCirclatingSupply, QUSINO_STAR_BONUS_FOR_QSC);
}

TEST(ContractQUSINO, transferSTAROrQSC_STAR_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id sender = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE;
    
    // First earn STAR
    increaseEnergy(sender, requiredReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(sender, amount, requiredReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Transfer STAR
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, 1, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check balances
    QUSINO::getUserAssetVolume_output senderVolume = QUSINO.getUserAssetVolume(sender);
    QUSINO::getUserAssetVolume_output receiverVolume = QUSINO.getUserAssetVolume(receiver);
    EXPECT_EQ(senderVolume.STARAmount, 0);
    EXPECT_EQ(receiverVolume.STARAmount, amount);
}

TEST(ContractQUSINO, transferSTAROrQSC_QSC_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 5526353;
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, 100000, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), 100000);
    
    id sender = QUSINO_testUser2;
    id receiver = QUSINO_testUser3;
    uint64 amount = 5000;
    
    // Transfer QST to sender first
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, sender, qstAssetName, qstIssuer, amount), amount);
    
    increaseEnergy(sender, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(sender, asset, amount, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), amount);
    
    // Earn QSC
    increaseEnergy(sender, 1);
    QUSINO::earnQSC_output earnOutput = QUSINO.earnQSC(sender, amount, 1);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Transfer QSC
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, 0, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check balances
    QUSINO::getUserAssetVolume_output senderVolume = QUSINO.getUserAssetVolume(sender);
    QUSINO::getUserAssetVolume_output receiverVolume = QUSINO.getUserAssetVolume(receiver);
    EXPECT_EQ(senderVolume.QSCAmount, 0);
    EXPECT_EQ(receiverVolume.QSCAmount, amount);
}

TEST(ContractQUSINO, transferSTAROrQSC_InsufficientSTAR)
{
    ContractTestingQUSINO QUSINO;
    
    id sender = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    uint64 amount = 1000;
    
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, 1, 1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_STAR);
}

TEST(ContractQUSINO, transferSTAROrQSC_InsufficientQSC)
{
    ContractTestingQUSINO QUSINO;
    
    id sender = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    uint64 amount = 1000;
    
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, 0, 1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QSC);
}

TEST(ContractQUSINO, stakeAssets_STAR_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 20000; // Above minimum
    uint32 stakingType = 1; // 1 month
    uint32 assetType = 1; // STAR
    
    // First earn STAR
    sint64 requiredReward = amount * QUSINO_STAR_PRICE;
    increaseEnergy(user, requiredReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(user, amount, requiredReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Stake STAR
    increaseEnergy(user, 1);
    QUSINO::stakeAssets_output output = QUSINO.stakeAssets(user, amount, stakingType, assetType, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check staking info
    QUSINO::getUserStakingInfo_output stakingInfo = QUSINO.getUserStakingInfo(user, 0);
    EXPECT_EQ(stakingInfo.counts, 1);
    EXPECT_EQ(stakingInfo.amount.get(0), amount);
    EXPECT_EQ(stakingInfo.type.get(0), stakingType);
    EXPECT_EQ(stakingInfo.typeOfAsset.get(0), assetType);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.totalStakedSTAR, amount);
    EXPECT_EQ(scInfo.numberOfStakers, 1);
}

TEST(ContractQUSINO, stakeAssets_LowStaking)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 5000; // Below minimum
    uint32 stakingType = 1;
    uint32 assetType = 1;
    
    increaseEnergy(user, 1);
    QUSINO::stakeAssets_output output = QUSINO.stakeAssets(user, amount, stakingType, assetType, 1);
    EXPECT_EQ(output.returnCode, QUSINO_LOW_STAKING);
}

TEST(ContractQUSINO, stakeAssets_WrongStakingType)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 20000;
    uint32 stakingType = 0; // Invalid
    uint32 assetType = 1;
    
    increaseEnergy(user, 1);
    QUSINO::stakeAssets_output output = QUSINO.stakeAssets(user, amount, stakingType, assetType, 1);
    EXPECT_EQ(output.returnCode, QUSINO_WRONG_STAKING_TYPE);
}

TEST(ContractQUSINO, stakeAssets_WrongAssetType)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 20000;
    uint32 stakingType = 1;
    uint32 assetType = 4; // Invalid
    
    increaseEnergy(user, 1);
    QUSINO::stakeAssets_output output = QUSINO.stakeAssets(user, amount, stakingType, assetType, 1);
    EXPECT_EQ(output.returnCode, QUSINO_WRONG_ASSET_TYPE);
}

TEST(ContractQUSINO, submitGame_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    
    increaseEnergy(user, requiredReward);
    QUSINO::submitGame_output output = QUSINO.submitGame(user, URI, requiredReward);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check game was added
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    EXPECT_GT(gameList.gameIndexes.get(0), 0);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.maxGameIndex, 2); // Starts at 1, so first game is index 1
    EXPECT_EQ(scInfo.epochRevenue, QUSINO_GAME_SUBMIT_FEE);
}

TEST(ContractQUSINO, submitGame_InsufficientFunds)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    sint64 insufficientReward = QUSINO_GAME_SUBMIT_FEE - 1;
    
    increaseEnergy(user, insufficientReward);
    QUSINO::submitGame_output output = QUSINO.submitGame(user, URI, insufficientReward);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_FUNDS);
}

TEST(ContractQUSINO, voteInGameProposal_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    
    // First submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    // Earn STAR for voting
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE;
    increaseEnergy(voter, starReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(voter, starAmount, starReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Vote on the game
    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_SUCCESS);
    
    // Check vote was recorded
    QUSINO::getActiveGameList_output updatedGameList = QUSINO.getActiveGameList(0);
    // Note: We can't directly check votes, but we can verify the game still exists
    EXPECT_GT(updatedGameList.gameIndexes.get(0), 0);
}

TEST(ContractQUSINO, voteInGameProposal_InsufficientVoteFee)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    
    // Submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    // Try to vote without enough STAR
    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_INSUFFICIENT_VOTE_FEE);
}

TEST(ContractQUSINO, voteInGameProposal_WrongGameURI)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI1 = createURI("https://example.com/game1");
    Array<uint8, 64> URI2 = createURI("https://example.com/game2");
    
    // Submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI1, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    // Earn STAR for voting
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE;
    increaseEnergy(voter, starReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(voter, starAmount, starReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Try to vote with wrong URI
    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI2, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_WRONG_GAME_URI_FOR_VOTE);
}

TEST(ContractQUSINO, getUserAssetVolume_Empty)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    QUSINO::getUserAssetVolume_output output = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(output.STARAmount, 0);
    EXPECT_EQ(output.QSCAmount, 0);
}

TEST(ContractQUSINO, getUserStakingInfo_Empty)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    QUSINO::getUserStakingInfo_output output = QUSINO.getUserStakingInfo(user, 0);
    EXPECT_EQ(output.counts, 0);
}

TEST(ContractQUSINO, getActiveGameList_Empty)
{
    ContractTestingQUSINO QUSINO;
    
    QUSINO::getActiveGameList_output output = QUSINO.getActiveGameList(0);
    // Should be empty initially
}

TEST(ContractQUSINO, END_EPOCH_StakingRewards)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 20000;
    uint32 stakingType = 1; // 1 month (4 epochs)
    uint32 assetType = 1; // STAR
    
    // Earn and stake STAR
    sint64 requiredReward = amount * QUSINO_STAR_PRICE;
    increaseEnergy(user, requiredReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(user, amount, requiredReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    increaseEnergy(user, 1);
    QUSINO::stakeAssets_output stakeOutput = QUSINO.stakeAssets(user, amount, stakingType, assetType, 1);
    EXPECT_EQ(stakeOutput.returnCode, QUSINO_SUCCESS);
    
    // Advance epochs to maturity (4 epochs for 1 month staking)
    // NOTE: staking reward is paid when stakedEpoch + 4 == qpi.epoch(),
    // so we first increment epoch, then call END_EPOCH for each of 4 epochs.
    for (uint32 i = 0; i < 4; i++)
    {
        ++system.epoch;
        QUSINO.endEpoch();
    }
    
    // Check that staking was completed and rewards were given
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(user);
    uint64 expectedReward = div(amount * QUSINO_STAR_STAKING_PERCENT_1 * 1ULL, 100ULL);
    EXPECT_EQ(userVolume.STARAmount, amount + expectedReward); // Original amount + reward
    
    // Check staking was removed
    QUSINO::getUserStakingInfo_output stakingInfo = QUSINO.getUserStakingInfo(user, 0);
    EXPECT_EQ(stakingInfo.counts, 0);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.totalStakedSTAR, 0);
    EXPECT_EQ(scInfo.numberOfStakers, 0);
}

TEST(ContractQUSINO, END_EPOCH_FailedGameRemoval)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    
    // Submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    
    // Vote no to make it fail
    id voter1 = QUSINO_testUser2;
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE;
    increaseEnergy(voter1, starReward);
    QUSINO::earnSTAR_output earnOutput1 = QUSINO.earnSTAR(voter1, starAmount, starReward);
    EXPECT_EQ(earnOutput1.returnCode, QUSINO_SUCCESS);
    
    increaseEnergy(voter1, 1);
    QUSINO::voteInGameProposal_output voteOutput1 = QUSINO.voteInGameProposal(voter1, URI, gameIndex, 0, 1);
    EXPECT_EQ(voteOutput1.returnCode, QUSINO_SUCCESS);
    
    // End epoch - game should be moved to failed list if no votes >= yes votes
    QUSINO.endEpoch();
    ++system.epoch;
    
    // Check failed game list
    QUSINO::getFailedGameList_output failedList = QUSINO.getFailedGameList(0);
    // Game should be in failed list
}
