#define NO_UEFI

#include "contract_testing.h"

static constexpr uint64 QUSINO_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QUSINO_TRANSFER_ASSET_FEE = 100ull;
static constexpr uint64 QUSINO_TRANSFER_RIGHTS_FEE = 100ull;

static const id QUSINO_CONTRACT_ID(QUSINO_CONTRACT_INDEX, 0, 0, 0);

const id QUSINO_testUser1 = ID(_U, _S, _E, _R, _1, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_testUser2 = ID(_U, _S, _E, _R, _2, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_testUser3 = ID(_U, _S, _E, _R, _3, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);

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
        input.asset = asset;
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

    QUSINO::saleQST_output saleQST(const id& user, uint64 amount, sint64 invocationReward)
    {
        QUSINO::saleQST_input input;
        input.amount = amount;
        QUSINO::saleQST_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 4, input, output, user, invocationReward);
        return output;
    }

    QUSINO::transferSTAROrQSC_output transferSTAROrQSC(const id& user, const id& dest, uint64 amount, bool type, sint64 invocationReward)
    {
        QUSINO::transferSTAROrQSC_input input;
        input.dest = dest;
        input.amount = amount;
        input.type = type;
        QUSINO::transferSTAROrQSC_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 5, input, output, user, invocationReward);
        return output;
    }

    QUSINO::stakeAssets_output stakeAssets(const id& user, uint64 amount, uint32 type, uint32 typeOfAsset, sint64 invocationReward)
    {
        QUSINO::stakeAssets_input input;
        input.amount = amount;
        input.type = type;
        input.typeOfAsset = typeOfAsset;
        QUSINO::stakeAssets_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 6, input, output, user, invocationReward);
        return output;
    }

    QUSINO::submitGame_output submitGame(const id& user, const Array<uint8, 64>& URI, sint64 invocationReward)
    {
        QUSINO::submitGame_input input;
        copyMemory(input.URI, URI);
        QUSINO::submitGame_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 7, input, output, user, invocationReward);
        return output;
    }

    QUSINO::voteInGameProposal_output voteInGameProposal(const id& user, const Array<uint8, 64>& URI, uint64 gameIndex, bool yesNo, sint64 invocationReward)
    {
        QUSINO::voteInGameProposal_input input;
        copyMemory(input.URI, URI);
        input.gameIndex = gameIndex;
        input.yesNo = yesNo;
        QUSINO::voteInGameProposal_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 8, input, output, user, invocationReward);
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
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 13153214312341;
    sint64 totalShares = 1000000;
    
    // Issue QST asset
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    // Transfer QST to contract for sale
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, QUSINO_CONTRACT_ID, qstAssetName, qstIssuer, 100000), 100000);
    
    // Transfer management rights to QUSINO contract
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, totalShares, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), totalShares);
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 1000;
    sint64 requiredReward = buyAmount * QUSINO_QST_PRICE;
    
    increaseEnergy(buyer, requiredReward);
    
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 0, requiredReward);
    EXPECT_EQ(output.returnCode, QusinoSuccess);
    
    // Verify buyer received QST
    EXPECT_EQ(numberOfPossessedShares(qstAssetName, qstIssuer, buyer, buyer, QUSINO_CONTRACT_INDEX, QUSINO_CONTRACT_INDEX), buyAmount);
    
    // Check QSTAmountForSale decreased
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.QSTAmountForSale, 100000 - buyAmount);
}

TEST(ContractQUSINO, buyQST_WithQSC_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 13153214312341;
    sint64 totalShares = 1000000;
    
    // Issue QST asset
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    // Transfer QST to contract for sale
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, QUSINO_CONTRACT_ID, qstAssetName, qstIssuer, 100000), 100000);
    
    // Transfer management rights to QUSINO contract
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, totalShares, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), totalShares);
    
    // First earn QSC
    id buyer = QUSINO_testUser2;
    uint64 qstAmount = 5000;
    increaseEnergy(buyer, 1);
    QUSINO::earnQSC_output earnOutput = QUSINO.earnQSC(buyer, qstAmount, 1);
    EXPECT_EQ(earnOutput.returnCode, QusinoSuccess);
    
    // Now buy QST with QSC
    uint64 buyAmount = 1000;
    increaseEnergy(buyer, 1);
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 1, 1);
    EXPECT_EQ(output.returnCode, QusinoSuccess);
    
    // Verify buyer received QST
    EXPECT_EQ(numberOfPossessedShares(qstAssetName, qstIssuer, buyer, buyer, QUSINO_CONTRACT_INDEX, QUSINO_CONTRACT_INDEX), buyAmount);
    
    // Check QSC decreased
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(buyer);
    EXPECT_EQ(userVolume.QSCAmount, qstAmount - buyAmount);
}

TEST(ContractQUSINO, buyQST_InsufficientQSTAmountForSale)
{
    ContractTestingQUSINO QUSINO;
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 200000; // More than available
    sint64 requiredReward = buyAmount * QUSINO_QST_PRICE;
    
    increaseEnergy(buyer, requiredReward);
    
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 0, requiredReward);
    EXPECT_EQ(output.returnCode, QusinoinsufficientQSTAmountForSale);
}

TEST(ContractQUSINO, buyQST_InsufficientFunds)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 13153214312341;
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, QUSINO_CONTRACT_ID, qstAssetName, qstIssuer, 100000), 100000);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, totalShares, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), totalShares);
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 1000;
    sint64 insufficientReward = buyAmount * QUSINO_QST_PRICE - 1;
    
    increaseEnergy(buyer, insufficientReward);
    
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 0, insufficientReward);
    EXPECT_EQ(output.returnCode, QusinoinsufficientFunds);
}

TEST(ContractQUSINO, buyQST_InsufficientQSC)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 13153214312341;
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, QUSINO_CONTRACT_ID, qstAssetName, qstIssuer, 100000), 100000);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, totalShares, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), totalShares);
    
    id buyer = QUSINO_testUser2;
    uint64 buyAmount = 1000;
    
    increaseEnergy(buyer, 1);
    QUSINO::buyQST_output output = QUSINO.buyQST(buyer, buyAmount, 1, 1);
    EXPECT_EQ(output.returnCode, QusinoinsufficientQSC);
}

TEST(ContractQUSINO, earnSTAR_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE;
    
    increaseEnergy(user, requiredReward);
    
    QUSINO::earnSTAR_output output = QUSINO.earnSTAR(user, amount, requiredReward);
    EXPECT_EQ(output.returnCode, QusinoSuccess);
    
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
    EXPECT_EQ(output.returnCode, QusinoinsufficientFunds);
}

TEST(ContractQUSINO, earnQSC_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 13153214312341;
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, totalShares, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), totalShares);
    
    id user = QUSINO_testUser2;
    uint64 amount = 5000;
    
    // Transfer QST to user first
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, user, qstAssetName, qstIssuer, amount), amount);
    
    increaseEnergy(user, 1);
    QUSINO::earnQSC_output output = QUSINO.earnQSC(user, amount, 1);
    EXPECT_EQ(output.returnCode, QusinoSuccess);
    
    // Check user's QSC and STAR amounts (should get bonus STAR)
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(userVolume.QSCAmount, amount);
    EXPECT_EQ(userVolume.STARAmount, QUSINO_STAR_BONUS_FOR_QSC);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.QSCCirclatingSupply, amount);
    EXPECT_EQ(scInfo.STARCirclatingSupply, QUSINO_STAR_BONUS_FOR_QSC);
}

TEST(ContractQUSINO, saleQST_Success)
{
    ContractTestingQUSINO QUSINO;
    
    // Use QST asset from contract initialization
    id qstIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _, _B, _V, _G, _B, _S, _U, _Q, _G);
    uint64 qstAssetName = 13153214312341;
    sint64 totalShares = 1000000;
    
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    Asset asset;
    asset.assetName = qstAssetName;
    asset.issuer = qstIssuer;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(QUSINO.transferShareManagementRightsQX(qstIssuer, asset, totalShares, QUSINO_CONTRACT_INDEX, QUSINO_TRANSFER_RIGHTS_FEE), totalShares);
    
    id user = QUSINO_testUser2;
    uint64 amount = 5000;
    
    // Transfer QST to user first
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, user, qstAssetName, qstIssuer, amount), amount);
    
    sint64 balanceBefore = getBalance(user);
    increaseEnergy(user, 1);
    QUSINO::saleQST_output output = QUSINO.saleQST(user, amount, 1);
    EXPECT_EQ(output.returnCode, QusinoSuccess);
    
    // Check user received Qubic
    sint64 balanceAfter = getBalance(user);
    sint64 expectedPayment = amount * QUSINO_QST_PRICE_FOR_SALE;
    EXPECT_EQ(balanceAfter - balanceBefore, expectedPayment);
    
    // Check QSTAmountForSale increased
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.QSTAmountForSale, 100000 + amount);
}
