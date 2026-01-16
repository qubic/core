#define NO_UEFI

#include "contract_testing.h"

static constexpr uint64 QIP_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QIP_TRANSFER_ASSET_FEE = 100ull;
static constexpr uint64 QIP_TRANSFER_RIGHTS_FEE = 100ull;

static const id QIP_CONTRACT_ID(QIP_CONTRACT_INDEX, 0, 0, 0);

const id QIP_testIssuer = ID(_T, _E, _S, _T, _I, _S, _S, _U, _E, _R, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T);
const id QIP_testAddress1 = ID(_A, _D, _D, _R, _A, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QIP_testAddress2 = ID(_A, _D, _D, _R, _B, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QIP_testAddress3 = ID(_A, _D, _D, _R, _C, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QIP_testBuyer = ID(_B, _U, _Y, _E, _R, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);

class QIPChecker : public QIP
{
public:
    uint32 getNumberOfICO() const { return numberOfICO; }
    
    void checkICOInfo(const QIP::getICOInfo_output& output, const QIP::createICO_input& input, const id& creator)
    {
        EXPECT_EQ(output.creatorOfICO, creator);
        EXPECT_EQ(output.issuer, input.issuer);
        EXPECT_EQ(output.address1, input.address1);
        EXPECT_EQ(output.address2, input.address2);
        EXPECT_EQ(output.address3, input.address3);
        EXPECT_EQ(output.address4, input.address4);
        EXPECT_EQ(output.address5, input.address5);
        EXPECT_EQ(output.address6, input.address6);
        EXPECT_EQ(output.address7, input.address7);
        EXPECT_EQ(output.address8, input.address8);
        EXPECT_EQ(output.address9, input.address9);
        EXPECT_EQ(output.address10, input.address10);
        EXPECT_EQ(output.assetName, input.assetName);
        EXPECT_EQ(output.price1, input.price1);
        EXPECT_EQ(output.price2, input.price2);
        EXPECT_EQ(output.price3, input.price3);
        EXPECT_EQ(output.saleAmountForPhase1, input.saleAmountForPhase1);
        EXPECT_EQ(output.saleAmountForPhase2, input.saleAmountForPhase2);
        EXPECT_EQ(output.saleAmountForPhase3, input.saleAmountForPhase3);
        EXPECT_EQ(output.remainingAmountForPhase1, input.saleAmountForPhase1);
        EXPECT_EQ(output.remainingAmountForPhase2, input.saleAmountForPhase2);
        EXPECT_EQ(output.remainingAmountForPhase3, input.saleAmountForPhase3);
        EXPECT_EQ(output.percent1, input.percent1);
        EXPECT_EQ(output.percent2, input.percent2);
        EXPECT_EQ(output.percent3, input.percent3);
        EXPECT_EQ(output.percent4, input.percent4);
        EXPECT_EQ(output.percent5, input.percent5);
        EXPECT_EQ(output.percent6, input.percent6);
        EXPECT_EQ(output.percent7, input.percent7);
        EXPECT_EQ(output.percent8, input.percent8);
        EXPECT_EQ(output.percent9, input.percent9);
        EXPECT_EQ(output.percent10, input.percent10);
        EXPECT_EQ(output.startEpoch, input.startEpoch);
    }
};

class ContractTestingQIP : protected ContractTesting
{
public:
    ContractTestingQIP()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QIP);
        callSystemProcedure(QIP_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QIPChecker* getState()
    {
        return (QIPChecker*)contractStates[QIP_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QIP_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares)
    {
        QX::IssueAsset_input input;
        input.assetName = assetName;
        input.numberOfShares = numberOfShares;
        input.unitOfMeasurement = 0;
        input.numberOfDecimalPlaces = 0;
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QIP_ISSUE_ASSET_FEE);
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
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, QIP_TRANSFER_ASSET_FEE);
        return output.transferredNumberOfShares;
    }

    QIP::createICO_output createICO(const id& creator, const QIP::createICO_input& input)
    {
        QIP::createICO_output output;
        invokeUserProcedure(QIP_CONTRACT_INDEX, 1, input, output, creator, 0);
        return output;
    }

    QIP::buyToken_output buyToken(const id& buyer, uint32 indexOfICO, uint64 amount, sint64 invocationReward)
    {
        QIP::buyToken_input input;
        input.indexOfICO = indexOfICO;
        input.amount = amount;
        QIP::buyToken_output output;
        invokeUserProcedure(QIP_CONTRACT_INDEX, 2, input, output, buyer, invocationReward);
        return output;
    }

    QIP::getICOInfo_output getICOInfo(uint32 indexOfICO)
    {
        QIP::getICOInfo_input input;
        input.indexOfICO = indexOfICO;
        QIP::getICOInfo_output output;
        callFunction(QIP_CONTRACT_INDEX, 1, input, output);
        return output;
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

    sint64 transferShareManagementRights(const id& invocator, const Asset& asset, sint64 numberOfShares, uint32 newManagingContractIndex, sint64 invocationReward)
    {
        QIP::TransferShareManagementRights_input input;
        input.asset = asset;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;
        QIP::TransferShareManagementRights_output output;
        invokeUserProcedure(QIP_CONTRACT_INDEX, 3, input, output, invocator, invocationReward);
        return output.transferredNumberOfShares;
    }
};

TEST(ContractQIP, createICO_Success)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    // Issue asset and transfer to creator
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    // Prepare ICO input
    QIP::createICO_input input;
    input.issuer = issuer;
    input.address1 = QIP_testAddress1;
    input.address2 = QIP_testAddress2;
    input.address3 = QIP_testAddress3;
    input.address4 = QIP_testAddress1;
    input.address5 = QIP_testAddress2;
    input.address6 = QIP_testAddress3;
    input.address7 = QIP_testAddress1;
    input.address8 = QIP_testAddress2;
    input.address9 = QIP_testAddress3;
    input.address10 = QIP_testAddress1;
    input.assetName = assetName;
    input.price1 = 100;
    input.price2 = 200;
    input.price3 = 300;
    input.saleAmountForPhase1 = 300000;
    input.saleAmountForPhase2 = 300000;
    input.saleAmountForPhase3 = 400000;
    input.percent1 = 10;
    input.percent2 = 10;
    input.percent3 = 10;
    input.percent4 = 10;
    input.percent5 = 10;
    input.percent6 = 10;
    input.percent7 = 10;
    input.percent8 = 10;
    input.percent9 = 10;
    input.percent10 = 5;
    input.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output output = QIP.createICO(creator, input);
    EXPECT_EQ(output.returnCode, QIPLogInfo::QIP_success);
    
    // Check ICO info
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    QIP.getState()->checkICOInfo(icoInfo, input, creator);
    
    // Verify shares were transferred to contract
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, creator, creator, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QIP_CONTRACT_ID, QIP_CONTRACT_ID, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), totalShares);
}

TEST(ContractQIP, createICO_InvalidStartEpoch)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input input;
    input.issuer = issuer;
    input.address1 = QIP_testAddress1;
    input.address2 = QIP_testAddress2;
    input.address3 = QIP_testAddress3;
    input.address4 = QIP_testAddress1;
    input.address5 = QIP_testAddress2;
    input.address6 = QIP_testAddress3;
    input.address7 = QIP_testAddress1;
    input.address8 = QIP_testAddress2;
    input.address9 = QIP_testAddress3;
    input.address10 = QIP_testAddress1;
    input.assetName = assetName;
    input.price1 = 100;
    input.price2 = 200;
    input.price3 = 300;
    input.saleAmountForPhase1 = 300000;
    input.saleAmountForPhase2 = 300000;
    input.saleAmountForPhase3 = 400000;
    input.percent1 = 10;
    input.percent2 = 10;
    input.percent3 = 10;
    input.percent4 = 10;
    input.percent5 = 10;
    input.percent6 = 10;
    input.percent7 = 10;
    input.percent8 = 10;
    input.percent9 = 10;
    input.percent10 = 5;
    
    // Test with startEpoch <= current epoch + 1
    input.startEpoch = system.epoch;
    increaseEnergy(creator, 1);
    QIP::createICO_output output1 = QIP.createICO(creator, input);
    EXPECT_EQ(output1.returnCode, QIPLogInfo::QIP_invalidStartEpoch);
    
    input.startEpoch = system.epoch + 1;
    QIP::createICO_output output2 = QIP.createICO(creator, input);
    EXPECT_EQ(output2.returnCode, QIPLogInfo::QIP_invalidStartEpoch);
}

TEST(ContractQIP, createICO_InvalidSaleAmount)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input input;
    input.issuer = issuer;
    input.address1 = QIP_testAddress1;
    input.address2 = QIP_testAddress2;
    input.address3 = QIP_testAddress3;
    input.address4 = QIP_testAddress1;
    input.address5 = QIP_testAddress2;
    input.address6 = QIP_testAddress3;
    input.address7 = QIP_testAddress1;
    input.address8 = QIP_testAddress2;
    input.address9 = QIP_testAddress3;
    input.address10 = QIP_testAddress1;
    input.assetName = assetName;
    input.price1 = 100;
    input.price2 = 200;
    input.price3 = 300;
    input.saleAmountForPhase1 = 300000;
    input.saleAmountForPhase2 = 300000;
    input.saleAmountForPhase3 = 400001; // Total doesn't match
    input.percent1 = 10;
    input.percent2 = 10;
    input.percent3 = 10;
    input.percent4 = 10;
    input.percent5 = 10;
    input.percent6 = 10;
    input.percent7 = 10;
    input.percent8 = 10;
    input.percent9 = 10;
    input.percent10 = 5;
    input.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output output = QIP.createICO(creator, input);
    EXPECT_EQ(output.returnCode, QIPLogInfo::QIP_invalidSaleAmount);
}

TEST(ContractQIP, createICO_InvalidPrice)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input input;
    input.issuer = issuer;
    input.address1 = QIP_testAddress1;
    input.address2 = QIP_testAddress2;
    input.address3 = QIP_testAddress3;
    input.address4 = QIP_testAddress1;
    input.address5 = QIP_testAddress2;
    input.address6 = QIP_testAddress3;
    input.address7 = QIP_testAddress1;
    input.address8 = QIP_testAddress2;
    input.address9 = QIP_testAddress3;
    input.address10 = QIP_testAddress1;
    input.assetName = assetName;
    input.price1 = 0; // Invalid price
    input.price2 = 200;
    input.price3 = 300;
    input.saleAmountForPhase1 = 300000;
    input.saleAmountForPhase2 = 300000;
    input.saleAmountForPhase3 = 400000;
    input.percent1 = 10;
    input.percent2 = 10;
    input.percent3 = 10;
    input.percent4 = 10;
    input.percent5 = 10;
    input.percent6 = 10;
    input.percent7 = 10;
    input.percent8 = 10;
    input.percent9 = 10;
    input.percent10 = 5;
    input.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output output = QIP.createICO(creator, input);
    EXPECT_EQ(output.returnCode, QIPLogInfo::QIP_invalidPrice);
}

TEST(ContractQIP, createICO_InvalidPercent)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input input;
    input.issuer = issuer;
    input.address1 = QIP_testAddress1;
    input.address2 = QIP_testAddress2;
    input.address3 = QIP_testAddress3;
    input.address4 = QIP_testAddress1;
    input.address5 = QIP_testAddress2;
    input.address6 = QIP_testAddress3;
    input.address7 = QIP_testAddress1;
    input.address8 = QIP_testAddress2;
    input.address9 = QIP_testAddress3;
    input.address10 = QIP_testAddress1;
    input.assetName = assetName;
    input.price1 = 100;
    input.price2 = 200;
    input.price3 = 300;
    input.saleAmountForPhase1 = 300000;
    input.saleAmountForPhase2 = 300000;
    input.saleAmountForPhase3 = 400000;
    input.percent1 = 10;
    input.percent2 = 10;
    input.percent3 = 10;
    input.percent4 = 10;
    input.percent5 = 10;
    input.percent6 = 10;
    input.percent7 = 10;
    input.percent8 = 10;
    input.percent9 = 5;
    input.percent10 = 1; // Total is 96, should be 95
    input.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output output = QIP.createICO(creator, input);
    EXPECT_EQ(output.returnCode, QIPLogInfo::QIP_invalidPercent);
}

TEST(ContractQIP, buyToken_Phase1)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Advance to start epoch
    ++system.epoch;
    ++system.epoch;
    
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 10000;
    uint64 price = createInput.price1;
    sint64 requiredReward = buyAmount * price;
    
    increaseEnergy(buyer, requiredReward);
    increaseEnergy(QIP_testAddress1, 1);
    increaseEnergy(QIP_testAddress2, 1);
    increaseEnergy(QIP_testAddress3, 1);
    
    // Record balances before purchase for all addresses
    sint64 balanceBefore1 = getBalance(QIP_testAddress1);
    sint64 balanceBefore2 = getBalance(QIP_testAddress2);
    sint64 balanceBefore3 = getBalance(QIP_testAddress3);
    sint64 contractBalanceBefore = getBalance(QIP_CONTRACT_ID);
    
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 0, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Verify buyer received the shares
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, buyer, buyer, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), buyAmount);
    
    // Check remaining amounts
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase1, createInput.saleAmountForPhase1 - buyAmount);
    EXPECT_EQ(icoInfo.remainingAmountForPhase2, createInput.saleAmountForPhase2);
    EXPECT_EQ(icoInfo.remainingAmountForPhase3, createInput.saleAmountForPhase3);
    
    // Calculate expected distributions for all 10 addresses
    sint64 totalPayment = buyAmount * price;
    uint64 expectedDist1 = div(totalPayment * createInput.percent1 * 1ULL, 100ULL);
    uint64 expectedDist2 = div(totalPayment * createInput.percent2 * 1ULL, 100ULL);
    uint64 expectedDist3 = div(totalPayment * createInput.percent3 * 1ULL, 100ULL);
    uint64 expectedDist4 = div(totalPayment * createInput.percent4 * 1ULL, 100ULL);
    uint64 expectedDist5 = div(totalPayment * createInput.percent5 * 1ULL, 100ULL);
    uint64 expectedDist6 = div(totalPayment * createInput.percent6 * 1ULL, 100ULL);
    uint64 expectedDist7 = div(totalPayment * createInput.percent7 * 1ULL, 100ULL);
    uint64 expectedDist8 = div(totalPayment * createInput.percent8 * 1ULL, 100ULL);
    uint64 expectedDist9 = div(totalPayment * createInput.percent9 * 1ULL, 100ULL);
    uint64 expectedDist10 = div(totalPayment * createInput.percent10 * 1ULL, 100ULL);
    
    // Calculate total distributed to addresses (should be 95% of total payment)
    sint64 totalDistributedToAddresses = expectedDist1 + expectedDist2 + expectedDist3 + expectedDist4 + expectedDist5 +
                                         expectedDist6 + expectedDist7 + expectedDist8 + expectedDist9 + expectedDist10;
    
    // Calculate expected dividend amount (remaining 5% divided by 676)
    sint64 remainingForDividends = totalPayment - totalDistributedToAddresses;
    uint64 expectedDividendAmount = div(remainingForDividends * 1ULL, 676ULL) * 676;
    
    // Verify all addresses received correct amounts
    // Note: addresses 1, 4, 7, 10 map to QIP_testAddress1
    //       addresses 2, 5, 8 map to QIP_testAddress2
    //       addresses 3, 6, 9 map to QIP_testAddress3
    sint64 expectedForAddress1 = expectedDist1 + expectedDist4 + expectedDist7 + expectedDist10;
    sint64 expectedForAddress2 = expectedDist2 + expectedDist5 + expectedDist8;
    sint64 expectedForAddress3 = expectedDist3 + expectedDist6 + expectedDist9;
    
    EXPECT_EQ(getBalance(QIP_testAddress1), balanceBefore1 + expectedForAddress1);
    EXPECT_EQ(getBalance(QIP_testAddress2), balanceBefore2 + expectedForAddress2);
    EXPECT_EQ(getBalance(QIP_testAddress3), balanceBefore3 + expectedForAddress3);
    
    // Verify contract balance decreased by total payment (minus any refund to buyer)
    sint64 contractBalanceAfter = getBalance(QIP_CONTRACT_ID);
    sint64 contractBalanceChange = contractBalanceAfter - contractBalanceBefore;
    // Contract should have received the payment and distributed it, so balance should increase by fee minus distributions
    // But since we're transferring from contract to addresses, the contract balance should decrease
    // Actually, the contract receives the invocation reward, then transfers to addresses
    // So the contract balance should be: initial + requiredReward - totalDistributedToAddresses - expectedDividendAmount
    sint64 expectedContractBalanceChange = requiredReward - totalDistributedToAddresses - expectedDividendAmount;
    EXPECT_EQ(contractBalanceChange, expectedContractBalanceChange);
}

TEST(ContractQIP, buyToken_Phase2)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Advance to Phase 2 (startEpoch + 1)
    ++system.epoch;
    ++system.epoch;
    ++system.epoch; // Now at startEpoch + 1
    
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 10000;
    uint64 price = createInput.price2;
    sint64 requiredReward = buyAmount * price;
    
    increaseEnergy(buyer, requiredReward);
    increaseEnergy(QIP_testAddress1, 1);
    increaseEnergy(QIP_testAddress2, 1);
    increaseEnergy(QIP_testAddress3, 1);
    
    // Record balances before purchase
    sint64 balanceBefore1 = getBalance(QIP_testAddress1);
    sint64 balanceBefore2 = getBalance(QIP_testAddress2);
    sint64 balanceBefore3 = getBalance(QIP_testAddress3);
    sint64 contractBalanceBefore = getBalance(QIP_CONTRACT_ID);
    
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 0, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Verify buyer received the shares
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, buyer, buyer, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), buyAmount);
    
    // Check remaining amounts
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase1, createInput.saleAmountForPhase1);
    EXPECT_EQ(icoInfo.remainingAmountForPhase2, createInput.saleAmountForPhase2 - buyAmount);
    EXPECT_EQ(icoInfo.remainingAmountForPhase3, createInput.saleAmountForPhase3);
    
    // Verify fee distribution for all addresses
    sint64 totalPayment = buyAmount * price;
    uint64 expectedDist1 = div(totalPayment * createInput.percent1 * 1ULL, 100ULL);
    uint64 expectedDist2 = div(totalPayment * createInput.percent2 * 1ULL, 100ULL);
    uint64 expectedDist3 = div(totalPayment * createInput.percent3 * 1ULL, 100ULL);
    uint64 expectedDist4 = div(totalPayment * createInput.percent4 * 1ULL, 100ULL);
    uint64 expectedDist5 = div(totalPayment * createInput.percent5 * 1ULL, 100ULL);
    uint64 expectedDist6 = div(totalPayment * createInput.percent6 * 1ULL, 100ULL);
    uint64 expectedDist7 = div(totalPayment * createInput.percent7 * 1ULL, 100ULL);
    uint64 expectedDist8 = div(totalPayment * createInput.percent8 * 1ULL, 100ULL);
    uint64 expectedDist9 = div(totalPayment * createInput.percent9 * 1ULL, 100ULL);
    uint64 expectedDist10 = div(totalPayment * createInput.percent10 * 1ULL, 100ULL);
    
    sint64 totalDistributedToAddresses = expectedDist1 + expectedDist2 + expectedDist3 + expectedDist4 + expectedDist5 +
                                         expectedDist6 + expectedDist7 + expectedDist8 + expectedDist9 + expectedDist10;
    sint64 remainingForDividends = totalPayment - totalDistributedToAddresses;
    uint64 expectedDividendAmount = div(remainingForDividends * 1ULL, 676ULL) * 676;
    
    sint64 expectedForAddress1 = expectedDist1 + expectedDist4 + expectedDist7 + expectedDist10;
    sint64 expectedForAddress2 = expectedDist2 + expectedDist5 + expectedDist8;
    sint64 expectedForAddress3 = expectedDist3 + expectedDist6 + expectedDist9;
    
    EXPECT_EQ(getBalance(QIP_testAddress1), balanceBefore1 + expectedForAddress1);
    EXPECT_EQ(getBalance(QIP_testAddress2), balanceBefore2 + expectedForAddress2);
    EXPECT_EQ(getBalance(QIP_testAddress3), balanceBefore3 + expectedForAddress3);
    
    sint64 contractBalanceAfter = getBalance(QIP_CONTRACT_ID);
    sint64 contractBalanceChange = contractBalanceAfter - contractBalanceBefore;
    sint64 expectedContractBalanceChange = requiredReward - totalDistributedToAddresses - expectedDividendAmount;
    EXPECT_EQ(contractBalanceChange, expectedContractBalanceChange);
}

TEST(ContractQIP, buyToken_Phase3)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Advance to Phase 3 (startEpoch + 2)
    ++system.epoch;
    ++system.epoch;
    ++system.epoch; // Now at startEpoch + 1
    ++system.epoch; // Now at startEpoch + 2
    
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 10000;
    uint64 price = createInput.price3;
    sint64 requiredReward = buyAmount * price;
    
    increaseEnergy(buyer, requiredReward);
    increaseEnergy(QIP_testAddress1, 1);
    increaseEnergy(QIP_testAddress2, 1);
    increaseEnergy(QIP_testAddress3, 1);
    
    // Record balances before purchase
    sint64 balanceBefore1 = getBalance(QIP_testAddress1);
    sint64 balanceBefore2 = getBalance(QIP_testAddress2);
    sint64 balanceBefore3 = getBalance(QIP_testAddress3);
    sint64 contractBalanceBefore = getBalance(QIP_CONTRACT_ID);
    
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 0, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Verify buyer received the shares
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, buyer, buyer, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), buyAmount);
    
    // Check remaining amounts
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase1, createInput.saleAmountForPhase1);
    EXPECT_EQ(icoInfo.remainingAmountForPhase2, createInput.saleAmountForPhase2);
    EXPECT_EQ(icoInfo.remainingAmountForPhase3, createInput.saleAmountForPhase3 - buyAmount);
    
    // Verify fee distribution for all addresses
    sint64 totalPayment = buyAmount * price;
    uint64 expectedDist1 = div(totalPayment * createInput.percent1 * 1ULL, 100ULL);
    uint64 expectedDist2 = div(totalPayment * createInput.percent2 * 1ULL, 100ULL);
    uint64 expectedDist3 = div(totalPayment * createInput.percent3 * 1ULL, 100ULL);
    uint64 expectedDist4 = div(totalPayment * createInput.percent4 * 1ULL, 100ULL);
    uint64 expectedDist5 = div(totalPayment * createInput.percent5 * 1ULL, 100ULL);
    uint64 expectedDist6 = div(totalPayment * createInput.percent6 * 1ULL, 100ULL);
    uint64 expectedDist7 = div(totalPayment * createInput.percent7 * 1ULL, 100ULL);
    uint64 expectedDist8 = div(totalPayment * createInput.percent8 * 1ULL, 100ULL);
    uint64 expectedDist9 = div(totalPayment * createInput.percent9 * 1ULL, 100ULL);
    uint64 expectedDist10 = div(totalPayment * createInput.percent10 * 1ULL, 100ULL);
    
    sint64 totalDistributedToAddresses = expectedDist1 + expectedDist2 + expectedDist3 + expectedDist4 + expectedDist5 +
                                         expectedDist6 + expectedDist7 + expectedDist8 + expectedDist9 + expectedDist10;
    sint64 remainingForDividends = totalPayment - totalDistributedToAddresses;
    uint64 expectedDividendAmount = div(remainingForDividends * 1ULL, 676ULL) * 676;
    
    sint64 expectedForAddress1 = expectedDist1 + expectedDist4 + expectedDist7 + expectedDist10;
    sint64 expectedForAddress2 = expectedDist2 + expectedDist5 + expectedDist8;
    sint64 expectedForAddress3 = expectedDist3 + expectedDist6 + expectedDist9;
    
    EXPECT_EQ(getBalance(QIP_testAddress1), balanceBefore1 + expectedForAddress1);
    EXPECT_EQ(getBalance(QIP_testAddress2), balanceBefore2 + expectedForAddress2);
    EXPECT_EQ(getBalance(QIP_testAddress3), balanceBefore3 + expectedForAddress3);
    
    sint64 contractBalanceAfter = getBalance(QIP_CONTRACT_ID);
    sint64 contractBalanceChange = contractBalanceAfter - contractBalanceBefore;
    sint64 expectedContractBalanceChange = requiredReward - totalDistributedToAddresses - expectedDividendAmount;
    EXPECT_EQ(contractBalanceChange, expectedContractBalanceChange);
}

TEST(ContractQIP, buyToken_InvalidEpoch)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Try to buy before start epoch
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 10000;
    uint64 price = createInput.price1;
    sint64 requiredReward = buyAmount * price;
    
    increaseEnergy(buyer, requiredReward);
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 0, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_invalidEpoch);
}

TEST(ContractQIP, buyToken_InvalidAmount)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Advance to start epoch
    ++system.epoch;
    ++system.epoch;
    
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 300001; // More than remaining
    uint64 price = createInput.price1;
    sint64 requiredReward = buyAmount * price;
    
    increaseEnergy(buyer, requiredReward);
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 0, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_invalidAmount);
}

TEST(ContractQIP, buyToken_ICONotFound)
{
    ContractTestingQIP QIP;
    
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 10000;
    sint64 requiredReward = buyAmount * 100;
    
    increaseEnergy(buyer, requiredReward);
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 999, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_ICONotFound);
}

TEST(ContractQIP, buyToken_InsufficientInvocationReward)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Advance to start epoch
    ++system.epoch;
    ++system.epoch;
    
    id buyer = QIP_testBuyer;
    uint64 buyAmount = 10000;
    uint64 price = createInput.price1;
    sint64 requiredReward = buyAmount * price;
    sint64 insufficientReward = requiredReward - 1;
    
    increaseEnergy(buyer, insufficientReward);
    QIP::buyToken_output buyOutput = QIP.buyToken(buyer, 0, buyAmount, insufficientReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_insufficientInvocationReward);
}

TEST(ContractQIP, END_EPOCH_Phase1Rollover)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Check initial state
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    uint64 initialPhase1 = icoInfo.remainingAmountForPhase1;
    uint64 initialPhase2 = icoInfo.remainingAmountForPhase2;
    
    // Advance to startEpoch (Phase 1 ends)
    ++system.epoch; // epoch = startEpoch - 1
    ++system.epoch; // epoch = startEpoch
    
    // End epoch should rollover Phase 1 remaining to Phase 2
    QIP.endEpoch();
    
    // Check that Phase 1 remaining was set to 0
    icoInfo = QIP.getICOInfo(0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase1, 0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase2, initialPhase2 + initialPhase1);
}

TEST(ContractQIP, END_EPOCH_Phase2Rollover)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Check initial state
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    uint64 initialPhase2 = icoInfo.remainingAmountForPhase2;
    uint64 initialPhase3 = icoInfo.remainingAmountForPhase3;
    
    // Advance to startEpoch + 1 (Phase 2 ends)
    ++system.epoch; // epoch = startEpoch - 1
    ++system.epoch; // epoch = startEpoch
    ++system.epoch; // epoch = startEpoch + 1
    
    // End epoch should rollover Phase 2 remaining to Phase 3
    QIP.endEpoch();
    
    // Check that Phase 2 remaining was set to 0
    icoInfo = QIP.getICOInfo(0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase2, 0);
    EXPECT_EQ(icoInfo.remainingAmountForPhase3, initialPhase3 + initialPhase2);
}

TEST(ContractQIP, END_EPOCH_Phase3ReturnToCreator)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);
    
    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Check initial state - verify shares are in contract
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QIP_CONTRACT_ID, QIP_CONTRACT_ID, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), totalShares);
    QIP::getICOInfo_output icoInfo = QIP.getICOInfo(0);
    uint64 remainingPhase3 = icoInfo.remainingAmountForPhase3;
    sint64 creatorSharesBefore = numberOfPossessedShares(assetName, issuer, creator, creator, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX);
    
    // Advance to startEpoch + 2 (Phase 3 ends)
    ++system.epoch; // epoch = startEpoch - 1
    ++system.epoch; // epoch = startEpoch
    ++system.epoch; // epoch = startEpoch + 1
    ++system.epoch; // epoch = startEpoch + 2
    
    // End epoch should return Phase 3 remaining to creator and remove ICO
    QIP.endEpoch();
    
    // Verify shares were returned to creator
    sint64 creatorSharesAfter = numberOfPossessedShares(assetName, issuer, creator, creator, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX);
    EXPECT_EQ(creatorSharesAfter, creatorSharesBefore + remainingPhase3);
    
    // Verify ICO was removed
    QIPChecker* state = QIP.getState();
    EXPECT_EQ(state->getNumberOfICO(), 0);
    
    // Verify contract no longer has the returned shares
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QIP_CONTRACT_ID, QIP_CONTRACT_ID, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), totalShares - remainingPhase3);
}

TEST(ContractQIP, TransferShareManagementRights)
{
    ContractTestingQIP QIP;
    
    id issuer = QIP_testIssuer;
    uint64 assetName = assetNameFromString("ICOASS");
    sint64 totalShares = 1000000;
    
    increaseEnergy(issuer, QIP_ISSUE_ASSET_FEE);
    EXPECT_EQ(QIP.issueAsset(issuer, assetName, totalShares), totalShares);
    
    id creator = QIP_testBuyer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    increaseEnergy(issuer, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferAsset(issuer, creator, assetName, issuer, totalShares), totalShares);

    // Transfer management rights to QIP contract
    Asset asset;
    asset.assetName = assetName;
    asset.issuer = issuer;
    increaseEnergy(creator, QIP_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QIP.transferShareManagementRightsQX(creator, asset, totalShares, QIP_CONTRACT_INDEX, QIP_TRANSFER_ASSET_FEE), totalShares);
    
    // Transfer shares to QIP contract
    QIP::createICO_input createInput;
    createInput.issuer = issuer;
    createInput.address1 = QIP_testAddress1;
    createInput.address2 = QIP_testAddress2;
    createInput.address3 = QIP_testAddress3;
    createInput.address4 = QIP_testAddress1;
    createInput.address5 = QIP_testAddress2;
    createInput.address6 = QIP_testAddress3;
    createInput.address7 = QIP_testAddress1;
    createInput.address8 = QIP_testAddress2;
    createInput.address9 = QIP_testAddress3;
    createInput.address10 = QIP_testAddress1;
    createInput.assetName = assetName;
    createInput.price1 = 100;
    createInput.price2 = 200;
    createInput.price3 = 300;
    createInput.saleAmountForPhase1 = 300000;
    createInput.saleAmountForPhase2 = 300000;
    createInput.saleAmountForPhase3 = 400000;
    createInput.percent1 = 10;
    createInput.percent2 = 10;
    createInput.percent3 = 10;
    createInput.percent4 = 10;
    createInput.percent5 = 10;
    createInput.percent6 = 10;
    createInput.percent7 = 10;
    createInput.percent8 = 10;
    createInput.percent9 = 10;
    createInput.percent10 = 5;
    createInput.startEpoch = system.epoch + 2;
    
    increaseEnergy(creator, 1);
    QIP::createICO_output createOutput = QIP.createICO(creator, createInput);
    EXPECT_EQ(createOutput.returnCode, QIPLogInfo::QIP_success);
    
    // Verify shares are in QIP contract
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QIP_CONTRACT_ID, QIP_CONTRACT_ID, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), totalShares);
    
    system.epoch += 2;
    // buy token
    uint64 buyAmount = 100000;
    uint64 price = createInput.price1;
    sint64 requiredReward = buyAmount * price;
    
    increaseEnergy(creator, requiredReward);
    increaseEnergy(QIP_testAddress1, 1);
    increaseEnergy(QIP_testAddress2, 1);
    increaseEnergy(QIP_testAddress3, 1);
    
    // Record balances before purchase
    sint64 balanceBefore1 = getBalance(QIP_testAddress1);
    sint64 balanceBefore2 = getBalance(QIP_testAddress2);
    sint64 balanceBefore3 = getBalance(QIP_testAddress3);
    sint64 contractBalanceBefore = getBalance(QIP_CONTRACT_ID);
    
    QIP::buyToken_output buyOutput = QIP.buyToken(creator, 0, buyAmount, requiredReward);
    EXPECT_EQ(buyOutput.returnCode, QIPLogInfo::QIP_success);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, creator, creator, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), buyAmount);
    
    // Verify fee distribution for all addresses
    sint64 totalPayment = buyAmount * price;
    uint64 expectedDist1 = div(totalPayment * createInput.percent1 * 1ULL, 100ULL);
    uint64 expectedDist2 = div(totalPayment * createInput.percent2 * 1ULL, 100ULL);
    uint64 expectedDist3 = div(totalPayment * createInput.percent3 * 1ULL, 100ULL);
    uint64 expectedDist4 = div(totalPayment * createInput.percent4 * 1ULL, 100ULL);
    uint64 expectedDist5 = div(totalPayment * createInput.percent5 * 1ULL, 100ULL);
    uint64 expectedDist6 = div(totalPayment * createInput.percent6 * 1ULL, 100ULL);
    uint64 expectedDist7 = div(totalPayment * createInput.percent7 * 1ULL, 100ULL);
    uint64 expectedDist8 = div(totalPayment * createInput.percent8 * 1ULL, 100ULL);
    uint64 expectedDist9 = div(totalPayment * createInput.percent9 * 1ULL, 100ULL);
    uint64 expectedDist10 = div(totalPayment * createInput.percent10 * 1ULL, 100ULL);
    
    sint64 totalDistributedToAddresses = expectedDist1 + expectedDist2 + expectedDist3 + expectedDist4 + expectedDist5 +
                                         expectedDist6 + expectedDist7 + expectedDist8 + expectedDist9 + expectedDist10;
    sint64 remainingForDividends = totalPayment - totalDistributedToAddresses;
    uint64 expectedDividendAmount = div(remainingForDividends * 1ULL, 676ULL) * 676;
    
    sint64 expectedForAddress1 = expectedDist1 + expectedDist4 + expectedDist7 + expectedDist10;
    sint64 expectedForAddress2 = expectedDist2 + expectedDist5 + expectedDist8;
    sint64 expectedForAddress3 = expectedDist3 + expectedDist6 + expectedDist9;
    
    EXPECT_EQ(getBalance(QIP_testAddress1), balanceBefore1 + expectedForAddress1);
    EXPECT_EQ(getBalance(QIP_testAddress2), balanceBefore2 + expectedForAddress2);
    EXPECT_EQ(getBalance(QIP_testAddress3), balanceBefore3 + expectedForAddress3);
    
    sint64 contractBalanceAfter = getBalance(QIP_CONTRACT_ID);
    sint64 contractBalanceChange = contractBalanceAfter - contractBalanceBefore;
    sint64 expectedContractBalanceChange = requiredReward - totalDistributedToAddresses - expectedDividendAmount;
    EXPECT_EQ(contractBalanceChange, expectedContractBalanceChange);
    
    // Transfer management rights
    sint64 transferAmount = 100000;
    
    increaseEnergy(creator, QIP_TRANSFER_RIGHTS_FEE);
    sint64 transferred = QIP.transferShareManagementRights(creator, asset, transferAmount, QX_CONTRACT_INDEX, QIP_TRANSFER_RIGHTS_FEE);
    EXPECT_EQ(transferred, transferAmount);
    
    // Verify shares were transferred
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QIP_CONTRACT_ID, QIP_CONTRACT_ID, QIP_CONTRACT_INDEX, QIP_CONTRACT_INDEX), totalShares - transferAmount);
}

