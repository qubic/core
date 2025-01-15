#define NO_UEFI

#include "contract_testing.h"

static const id TESTEXA_CONTRACT_ID(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
static const id USER1(123, 456, 789, 876);
static const id USER2(42, 424, 4242, 42424);
static const id USER3(98, 76, 54, 3210);

class StateCheckerTestExampleA : public TESTEXA
{
public:
    void checkPostReleaseCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postReleaseSharesCounter, expectedCount);
    }
};

class ContractTestingTestEx : protected ContractTesting
{
public:
    QX::Fees_output qxFees;

    ContractTestingTestEx()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(TESTEXA);
        callSystemProcedure(TESTEXA_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
        qLogger::initLogging();

        // query QX fees
        callFunction(QX_CONTRACT_INDEX, 1, QX::Fees_input(), qxFees);
    }

    ~ContractTestingTestEx()
    {
        qLogger::deinitLogging();
    }

    StateCheckerTestExampleA* getStateTestExampleA()
    {
        return (StateCheckerTestExampleA*)contractStates[TESTEXA_CONTRACT_INDEX];
    }

    sint64 issueAssetQx(const Asset& asset, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ asset.assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, asset.issuer, qxFees.assetIssuanceFee);
        return output.issuedNumberOfShares;
    }

    sint64 issueAssetTestExA(const Asset& asset, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        TESTEXA::IssueAsset_input input{ asset.assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        TESTEXA::IssueAsset_output output;
        invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 1, input, output, asset.issuer, 0);
        return output.issuedNumberOfShares;
    }

    sint64 transferShareOwnershipAndPossessionQx(const Asset& asset, const id& currentOwnerAndPossesor, const id& newOwnerAndPossesor, sint64 numberOfShares)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;

        input.assetName = asset.assetName;
        input.issuer = asset.issuer;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, currentOwnerAndPossesor, qxFees.transferFee);

        return output.transferredNumberOfShares;
    }

    sint64 transferShareOwnershipAndPossessionTestExA(const Asset& asset, const id& currentOwnerAndPossesor, const id& newOwnerAndPossesor, sint64 numberOfShares)
    {
        TESTEXA::TransferShareOwnershipAndPossession_input input;
        TESTEXA::TransferShareOwnershipAndPossession_output output;

        input.asset = asset;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 2, input, output, currentOwnerAndPossesor, 0);

        return output.transferredNumberOfShares;
    }

    sint64 transferShareManagementRightsQx(const Asset& asset, const id& currentOwnerAndPossesor, sint64 numberOfShares, unsigned int newManagingContractIndex, sint64 fee = 0)
    {
        QX::TransferShareManagementRights_input input;
        QX::TransferShareManagementRights_output output;

        input.asset = asset;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, currentOwnerAndPossesor, fee);

        return output.transferredNumberOfShares;
    }

    sint64 transferShareManagementRightsTestExA(const Asset& asset, const id& currentOwnerAndPossesor, sint64 numberOfShares, unsigned int newManagingContractIndex, sint64 fee = 0)
    {
        TESTEXA::TransferShareManagementRights_input input;
        TESTEXA::TransferShareManagementRights_output output;

        input.asset = asset;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 4, input, output, currentOwnerAndPossesor, fee);

        return output.transferredNumberOfShares;
    }

    void setPreReleaseSharesOutput(bool allowTransfer, sint64 requestedFee)
    {
        TESTEXA::SetPreReleaseSharesOutput_input input{ allowTransfer, requestedFee };
        TESTEXA::SetPreReleaseSharesOutput_output output;
        invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 3, input, output, USER1, 0);
    }
};

TEST(ContractTestEx, ReleaseManagementRights)
{
    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("BLOB") };
    const sint64 totalShareCount = 1000000000;
    const sint64 transferShareCount = totalShareCount/4;

    // make sure the enities have enough qu
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER2, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER3, test.qxFees.assetIssuanceFee * 10);

    // issueAsset with QX
    EXPECT_EQ(test.issueAssetQx(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount);

    // run ownership/possession transfer in QX -> should work (has management rights from issueAsset)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, asset1.issuer, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);

    // run ownership/possession transfer in TESTEXA -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionTestExA(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);

    ////////////////////////////////////
    // RELEASE FROM QX TO TESTEXA 

    // invoke release of shares in QX to TESTEXA -> fails because default response of TESTEXA is to reject
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // enable that TESTEXA accepts transfer for 0 fee
    test.setPreReleaseSharesOutput(true, 0);

    // invoke release of shares in QX to TESTEXA -> succeed
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);

    // run ownership/possession transfer in TESTEXA -> should work
    EXPECT_EQ(test.transferShareOwnershipAndPossessionTestExA(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // release shares error case: too few shares -> fail
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, 0, TESTEXA_CONTRACT_INDEX, 100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);

    // release shares error case: more shares than available -> fail
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, totalShareCount + 1, TESTEXA_CONTRACT_INDEX, 100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);

    // release shares error case: fee requested by TESTEXA is too high to release shares (QX expects 0) -> fail
    test.setPreReleaseSharesOutput(true, 1000);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);

    // release shares error case: fee requested by TESTEXA is invalid
    test.setPreReleaseSharesOutput(true, -100);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    test.setPreReleaseSharesOutput(true, MAX_AMOUNT + 1000);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);

    ////////////////////////////////////
    // RELEASE FROM TESTEXA TO QX

    // run ownership/possession transfer in QX -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER3, USER2, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXA to QX with 0 qu -> fails because transfer fee is required by QX
    EXPECT_EQ(test.transferShareManagementRightsTestExA(asset1, USER3, transferShareCount, QX_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXA to QX with sufficient amount but too many shares -> should fail
    EXPECT_EQ(test.transferShareManagementRightsTestExA(asset1, USER3, totalShareCount, QX_CONTRACT_INDEX, test.qxFees.transferFee), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXA to QX with sufficient amount and correct shares -> should work
    EXPECT_EQ(test.transferShareManagementRightsTestExA(asset1, USER3, transferShareCount, QX_CONTRACT_INDEX, test.qxFees.transferFee), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);

    // run ownership/possession transfer in QX -> should work again
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER3, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
}



