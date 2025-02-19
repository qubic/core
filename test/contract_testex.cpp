#define NO_UEFI

#include "contract_testing.h"

static const id TESTEXA_CONTRACT_ID(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
static const id TESTEXB_CONTRACT_ID(TESTEXB_CONTRACT_INDEX, 0, 0, 0);
static const id USER1(123, 456, 789, 876);
static const id USER2(42, 424, 4242, 42424);
static const id USER3(98, 76, 54, 3210);

void checkPreManagementRightsTransferInput(const PreManagementRightsTransfer_input& observed, const PreManagementRightsTransfer_input& expected)
{
    EXPECT_EQ(observed.asset.assetName, expected.asset.assetName);
    EXPECT_EQ(observed.asset.issuer, expected.asset.issuer);
    EXPECT_EQ(observed.numberOfShares, expected.numberOfShares);
    EXPECT_EQ(observed.offeredFee, expected.offeredFee);
    EXPECT_EQ((int)observed.otherContractIndex, (int)expected.otherContractIndex);
    EXPECT_EQ(observed.owner, expected.owner);
    EXPECT_EQ(observed.possessor, expected.possessor);
}

void checkPostManagementRightsTransferInput(const PostManagementRightsTransfer_input& observed, const PostManagementRightsTransfer_input& expected)
{
    EXPECT_EQ(observed.asset.assetName, expected.asset.assetName);
    EXPECT_EQ(observed.asset.issuer, expected.asset.issuer);
    EXPECT_EQ(observed.numberOfShares, expected.numberOfShares);
    EXPECT_EQ(observed.receivedFee, expected.receivedFee);
    EXPECT_EQ((int)observed.otherContractIndex, (int)expected.otherContractIndex);
    EXPECT_EQ(observed.owner, expected.owner);
    EXPECT_EQ(observed.possessor, expected.possessor);
}

class StateCheckerTestExampleA : public TESTEXA
{
public:
    void checkPostReleaseCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postReleaseSharesCounter, expectedCount);
    }
    
    const PreManagementRightsTransfer_input& getPreReleaseInput() const
    {
        return this->prevPreReleaseSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostReleaseInput() const
    {
        return this->prevPostReleaseSharesInput;
    }

    void checkPostAcquireCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postAcquireShareCounter, expectedCount);
    }

    const PreManagementRightsTransfer_input& getPreAcquireInput() const
    {
        return this->prevPreAcquireSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostAcquireInput() const
    {
        return this->prevPostAcquireSharesInput;
    }
};

class StateCheckerTestExampleB : public TESTEXB
{
public:
    void checkPostReleaseCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postReleaseSharesCounter, expectedCount);
    }

    const PreManagementRightsTransfer_input& getPreReleaseInput() const
    {
        return this->prevPreReleaseSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostReleaseInput() const
    {
        return this->prevPostReleaseSharesInput;
    }

    void checkPostAcquireCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postAcquireShareCounter, expectedCount);
    }

    const PreManagementRightsTransfer_input& getPreAcquireInput() const
    {
        return this->prevPreAcquireSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostAcquireInput() const
    {
        return this->prevPostAcquireSharesInput;
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
        INIT_CONTRACT(TESTEXB);
        callSystemProcedure(TESTEXB_CONTRACT_INDEX, INITIALIZE);
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

    StateCheckerTestExampleB* getStateTestExampleB()
    {
        return (StateCheckerTestExampleB*)contractStates[TESTEXB_CONTRACT_INDEX];
    }

    sint64 issueAssetQx(const Asset& asset, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ asset.assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, asset.issuer, qxFees.assetIssuanceFee);
        return output.issuedNumberOfShares;
    }

    sint64 issueAssetTestExA(const Asset& asset, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
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

    template <typename StateStruct>
    sint64 transferShareOwnershipAndPossession(const Asset& asset, const id& currentOwnerAndPossesor, const id& newOwnerAndPossesor, sint64 numberOfShares)
    {
        typename StateStruct::TransferShareOwnershipAndPossession_input input;
        typename StateStruct::TransferShareOwnershipAndPossession_output output;

        input.asset = asset;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(StateStruct::__contract_index, 2, input, output, currentOwnerAndPossesor, 0);

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

    template <typename StateStruct>
    sint64 transferShareManagementRights(const Asset& asset, const id& currentOwnerAndPossesor, sint64 numberOfShares, unsigned int newManagingContractIndex, sint64 fee = 0)
    {
        typename StateStruct::TransferShareManagementRights_input input;
        typename StateStruct::TransferShareManagementRights_output output;

        input.asset = asset;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(StateStruct::__contract_index, 3, input, output, currentOwnerAndPossesor, fee);

        return output.transferredNumberOfShares;
    }

    template <typename StateStruct>
    void setPreReleaseSharesOutput(bool allowTransfer, sint64 requestedFee)
    {
        typename StateStruct::SetPreReleaseSharesOutput_input input{ allowTransfer, requestedFee };
        typename StateStruct::SetPreReleaseSharesOutput_output output;
        invokeUserProcedure(StateStruct::__contract_index, 4, input, output, USER1, 0);
    }

    template <typename StateStruct>
    void setPreAcquireSharesOutput(bool allowTransfer, sint64 requestedFee)
    {
        typename StateStruct::SetPreReleaseSharesOutput_input input{ allowTransfer, requestedFee };
        typename StateStruct::SetPreReleaseSharesOutput_output output;
        invokeUserProcedure(StateStruct::__contract_index, 5, input, output, USER1, 0);
    }

    
    template <typename StateStruct>
    sint64 acquireShareManagementRights(const Asset& asset, const id& currentOwnerAndPossesor, sint64 numberOfShares, unsigned int prevManagingContractIndex, sint64 fee = 0, const id& originator = NULL_ID)
    {
        typename StateStruct::AcquireShareManagementRights_input input;
        typename StateStruct::AcquireShareManagementRights_output output;

        input.asset = asset;
        input.ownerAndPossessor = currentOwnerAndPossesor;
        input.oldManagingContractIndex = prevManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(StateStruct::__contract_index, 6, input, output,
            (isZero(originator)) ? currentOwnerAndPossesor : originator, fee);

        return output.transferredNumberOfShares;
    }

    sint64 getTestExAsShareManagementRightsByInvokingTestExB(const Asset& asset, const id& currentOwnerAndPossesor, sint64 numberOfShares, sint64 fee = 0)
    {
        TESTEXB::GetTestExampleAShareManagementRights_input input;
        TESTEXB::GetTestExampleAShareManagementRights_output output;

        input.asset = asset;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(TESTEXB::__contract_index, 7, input, output, currentOwnerAndPossesor, fee);

        return output.transferredNumberOfShares;
    }

    TESTEXA::QueryQpiFunctions_output queryQpiFunctions(const TESTEXA::QueryQpiFunctions_input& input)
    {
        TESTEXA::QueryQpiFunctions_output output;
        callFunction(TESTEXA_CONTRACT_INDEX, 1, input, output);
        return output;
    }
};

TEST(ContractTestEx, QpiReleaseShares)
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
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, USER2, USER3, transferShareCount), 0);
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
    test.getStateTestExampleA()->checkPostAcquireCounter(0);

    // enable that TESTEXA accepts transfer for 0 fee
    test.setPreAcquireSharesOutput<TESTEXA>(true, 0);

    // invoke release of shares in QX to TESTEXA -> succeed
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleA()->getPreAcquireInput(),
        { asset1, USER2, USER2, transferShareCount, 0, QX_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleA()->getPostAcquireInput(),
        { asset1, USER2, USER2, transferShareCount, 0, QX_CONTRACT_INDEX });

    // run ownership/possession transfer in TESTEXA -> should work
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // release shares error case: too few shares -> fail
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, 0, TESTEXA_CONTRACT_INDEX, 100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    // release shares error case: more shares than available -> fail
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, totalShareCount + 1, TESTEXA_CONTRACT_INDEX, 100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    // release shares error case: fee requested by TESTEXA is too high to release shares (QX expects 0) -> fail
    test.setPreAcquireSharesOutput<TESTEXA>(true, 1000);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    // release shares error case: fee requested by TESTEXA is invalid
    test.setPreAcquireSharesOutput<TESTEXA>(true, -100);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    test.setPreAcquireSharesOutput<TESTEXA>(true, MAX_AMOUNT + 1000);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    ////////////////////////////////////
    // RELEASE FROM TESTEXA TO TESTEXB

    // invoke release of shares in TESTEXA to TESTEXB with fee -> succeed
    test.getStateTestExampleB()->checkPostAcquireCounter(0);
    sint64 balanceUser3 = getBalance(USER3);
    sint64 balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    sint64 balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    test.setPreAcquireSharesOutput<TESTEXB>(true, 1000);
    EXPECT_EQ(test.transferShareManagementRights<TESTEXA>(asset1, USER3, transferShareCount, TESTEXB_CONTRACT_INDEX, 1500), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);
    test.getStateTestExampleB()->checkPostAcquireCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleB()->getPreAcquireInput(),
        { asset1, USER3, USER3, transferShareCount, 1500, TESTEXA_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleB()->getPostAcquireInput(),
        { asset1, USER3, USER3, transferShareCount, 1000, TESTEXA_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER3), balanceUser3 - 1500);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 1500 - 1000);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 1000);

    ////////////////////////////////////
    // RELEASE FROM TESTEXB TO QX

    // run ownership/possession transfer in QX -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER3, USER2, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXB to QX with 0 qu -> fails because transfer fee is required by QX
    EXPECT_EQ(test.transferShareManagementRights<TESTEXB>(asset1, USER3, transferShareCount, QX_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXB to QX with sufficient amount but too many shares -> should fail
    EXPECT_EQ(test.transferShareManagementRights<TESTEXB>(asset1, USER3, totalShareCount, QX_CONTRACT_INDEX, test.qxFees.transferFee), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXB to QX with sufficient amount and correct shares -> should work
    EXPECT_EQ(test.transferShareManagementRights<TESTEXB>(asset1, USER3, transferShareCount, QX_CONTRACT_INDEX, test.qxFees.transferFee), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), 0);

    // run ownership/possession transfer in QX -> should work again
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER3, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
}

TEST(ContractTestEx, QpiAcquireShares)
{
    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("BLURB") };
    const sint64 totalShareCount = 100000000;
    const sint64 transferShareCount = totalShareCount / 4;
    
    // make sure the enities have enough qu
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER2, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER3, test.qxFees.assetIssuanceFee * 10);

    // issueAsset with TestExampleA
    EXPECT_EQ(test.issueAssetTestExA(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount);

    // run ownership/possession transfer in TestExampleA -> should work (has management rights from issueAsset)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, asset1.issuer, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // run ownership/possession transfer in QX -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);


    //////////////////////////////////////////////
    // TESTEXB ACQUIRES FROM TESTEXA

    // TestExampleB tries / fails to acquire management rights (negative shares count)
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, -100, TESTEXA_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (more shares than available)
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount + 1, TESTEXA_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (negative offered fee)
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, -100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (rejected by TESTEXA)
    test.setPreReleaseSharesOutput<TESTEXA>(false, 0);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 4999), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (requested fee is negative)
    test.setPreReleaseSharesOutput<TESTEXA>(true, -5000);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 5000), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (offered fee lower than requested fee)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 5000);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 4999), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (not enough QU owned to pay fee)
    test.setPreReleaseSharesOutput<TESTEXA>(true, test.qxFees.assetIssuanceFee * 11);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, test.qxFees.assetIssuanceFee * 11), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (with wrong originator USER3)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 1234);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 1234, USER3), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB acquires management rights (success)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 1234);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount / 4, TESTEXA_CONTRACT_INDEX, 1239), transferShareCount / 4);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount * 3 / 4);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount / 4);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleA()->getPreReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 4, 1239, TESTEXB_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleA()->getPostReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 4, 1234, TESTEXB_CONTRACT_INDEX });

    // TestExampleB acquires management rights (success)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 10);
    sint64 balanceUser2 = getBalance(USER2);
    sint64 balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    sint64 balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount * 3 / 4, TESTEXA_CONTRACT_INDEX, 15), transferShareCount * 3 / 4);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount);
    test.getStateTestExampleA()->checkPostReleaseCounter(2);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleA()->getPreReleaseInput(),
        { asset1, USER2, USER2, transferShareCount * 3 / 4, 15, TESTEXB_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleA()->getPostReleaseInput(),
        { asset1, USER2, USER2, transferShareCount * 3 / 4, 10, TESTEXB_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER2), balanceUser2 - 15);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 10);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 15 - 10);

    // run ownership/possession transfer in TestExampleB -> should work now (after acquiring management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXB>(asset1, USER2, USER3, transferShareCount / 2), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount / 2);

    //////////////////////////////////////////////
    // TESTEXA ACQUIRES FROM TESTEXB

    // TestExampleA acquires management rights from TestExampleB of shares owned by USER2 (success)
    test.getStateTestExampleB()->checkPostReleaseCounter(0);
    test.setPreReleaseSharesOutput<TESTEXB>(true, 13);
    balanceUser2 = getBalance(USER2);
    balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXA>(asset1, USER2, transferShareCount / 10, TESTEXB_CONTRACT_INDEX, 42), transferShareCount / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount * 4 / 10);
    test.getStateTestExampleB()->checkPostReleaseCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleB()->getPreReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 10, 42, TESTEXA_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleB()->getPostReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 10, 13, TESTEXA_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER2), balanceUser2 - 42);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 42 - 13);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 13);

    // TestExampleA acquires management rights from TestExampleB of shares owned by USER3 (success)
    test.setPreReleaseSharesOutput<TESTEXB>(true, 123);
    sint64 balanceUser3 = getBalance(USER3);
    balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXA>(asset1, USER3, transferShareCount * 2 / 10, TESTEXB_CONTRACT_INDEX, 124), transferShareCount * 2 / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount * 2 / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount * 3 / 10);
    test.getStateTestExampleB()->checkPostReleaseCounter(2);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleB()->getPreReleaseInput(),
        { asset1, USER3, USER3, transferShareCount * 2 / 10, 124, TESTEXA_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleB()->getPostReleaseInput(),
        { asset1, USER3, USER3, transferShareCount * 2 / 10, 123, TESTEXA_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER3), balanceUser3 - 124);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 124 - 123);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 123);

    // Some count final checks
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byManagingContract(QX_CONTRACT_INDEX)), 0);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byManagingContract(TESTEXA_CONTRACT_INDEX)), totalShareCount - transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byManagingContract(TESTEXB_CONTRACT_INDEX)), transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byOwner(USER1)), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byOwner(USER2)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byOwner(USER3)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(QX_CONTRACT_INDEX)), 0);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(TESTEXA_CONTRACT_INDEX)), totalShareCount - transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(TESTEXB_CONTRACT_INDEX)), transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(USER1)), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(USER2)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(USER3)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1), totalShareCount);
}

TEST(ContractTestEx, GetManagementRightsByInvokingOtherContractsRelease)
{
    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("BLURB") };
    const sint64 totalShareCount = 1000000;
    const sint64 transferShareCount = totalShareCount / 5;

    // make sure the enities have enough qu
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER2, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER3, test.qxFees.assetIssuanceFee * 10);

    // issueAsset with TestExampleA
    EXPECT_EQ(test.issueAssetTestExA(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount);

    ///////////////////////////////////////////////////////////////////////////
    // TESTEXB ACQUIRES FROM TESTEXA BY INVOKING TESTEXA PROCEDURE (QX PATTERN)

    // run ownership/possession transfer to TestExB in TestExampleA -> should work (has management rights from issueAsset)
    // (TestExB needs to own/possess shares in order to invoke TestExA for transferring rights to TestExB in the next step)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, asset1.issuer, TESTEXB_CONTRACT_ID, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // Transfer rights to TestExB using the QX approach
    // -> Test that we don't get a deadlock in the following case:
    //    invoke procedure of TestExB, which invokes procedure of TestExA for calling qpi.releaseShares(), which runs
    //    callback PRE_ACQUIRE_SHARES of TestExB
    // Attempt 1: fail due to forbidding by default
    EXPECT_EQ(test.getTestExAsShareManagementRightsByInvokingTestExB(asset1, USER1, transferShareCount, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // Attempt 2: allow -> fail because requested fee > offered fee
    test.setPreAcquireSharesOutput<TESTEXB>(true, 13);
    EXPECT_EQ(test.getTestExAsShareManagementRightsByInvokingTestExB(asset1, USER1, transferShareCount, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // Attempt 2: allow -> success
    EXPECT_EQ(test.getTestExAsShareManagementRightsByInvokingTestExB(asset1, USER1, transferShareCount, 15), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXB_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXB_CONTRACT_INDEX }), transferShareCount);
}

TEST(ContractTestEx, QueryBasicQpiFunctions)
{
    ContractTestingTestEx test;

    id arbitratorPubKey;
    getPublicKeyFromIdentity((const unsigned char*)ARBITRATOR, arbitratorPubKey.m256i_u8);

    // prepare data for qpi.K12() and qpi.signatureValidity()
    TESTEXA::QueryQpiFunctions_input queryQpiFuncInput1;
    id subseed1(123456789, 987654321, 1357986420, 0xabcdef);
    id privateKey1, digest1;
    getPrivateKey(subseed1.m256i_u8, privateKey1.m256i_u8);
    getPublicKey(privateKey1.m256i_u8, queryQpiFuncInput1.entity.m256i_u8);
    for (uint64 i = 0; i < queryQpiFuncInput1.data.capacity(); ++i)
        queryQpiFuncInput1.data.set(i, static_cast<sint8>(i - 50));
    KangarooTwelve(&queryQpiFuncInput1.data, sizeof(queryQpiFuncInput1.data), &digest1, sizeof(digest1));
    sign(subseed1.m256i_u8, queryQpiFuncInput1.entity.m256i_u8, digest1.m256i_u8, (unsigned char*)&queryQpiFuncInput1.signature);

    // Test 1 with initial time
    setMemory(utcTime, 0);
    utcTime.Year = 2022;
    utcTime.Month = 4;
    utcTime.Day = 13;
    utcTime.Hour = 12;
    updateQpiTime();
    numberTickTransactions = 123;
    system.tick = 4567890;
    system.epoch = 987;
    auto qpiReturned1 = test.queryQpiFunctions(queryQpiFuncInput1);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.year, 22);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.month, 4);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.day, 13);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.hour, 12);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.minute, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.second, 0);
    EXPECT_EQ((int)qpiReturned1.qpiFunctionsOutput.millisecond, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.dayOfWeek, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.arbitrator, arbitratorPubKey);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.computor0, id::zero());
    EXPECT_EQ((int)qpiReturned1.qpiFunctionsOutput.epoch, (int)system.epoch);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.invocationReward, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.invocator, id::zero());
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.numberOfTickTransactions, numberTickTransactions);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.originator, id::zero());
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.tick, system.tick);
    EXPECT_EQ(qpiReturned1.inputDataK12, digest1);
    EXPECT_TRUE(qpiReturned1.inputSignatureValid);

    // K12 test for wrong signature: change data but not signature
    TESTEXA::QueryQpiFunctions_input queryQpiFuncInput2 = queryQpiFuncInput1;
    id digest2;
    queryQpiFuncInput2.data.set(0, 0);
    KangarooTwelve(&queryQpiFuncInput2.data, sizeof(queryQpiFuncInput2.data), &digest2, sizeof(digest2));
    EXPECT_NE(digest1, digest2);

    // Test 2 with current time
    updateTime();
    updateQpiTime();
    numberTickTransactions = 42;
    system.tick = 4567891;
    system.epoch = 988;
    broadcastedComputors.computors.publicKeys[0] = id(12, 34, 56, 78);
    auto qpiReturned2 = test.queryQpiFunctions(queryQpiFuncInput2);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.year, utcTime.Year - 2000);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.month, utcTime.Month);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.day, utcTime.Day);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.hour, utcTime.Hour);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.minute, utcTime.Minute);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.second, utcTime.Second);
    EXPECT_EQ((int)qpiReturned2.qpiFunctionsOutput.millisecond, utcTime.Nanosecond / 1000000);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.dayOfWeek, dayIndex(qpiReturned2.qpiFunctionsOutput.year, qpiReturned2.qpiFunctionsOutput.month, qpiReturned2.qpiFunctionsOutput.day) % 7);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.arbitrator, arbitratorPubKey);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.computor0, id(12, 34, 56, 78));
    EXPECT_EQ((int)qpiReturned2.qpiFunctionsOutput.epoch, (int)system.epoch);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.invocationReward, 0);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.invocator, id::zero());
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.numberOfTickTransactions, numberTickTransactions);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.originator, id::zero());
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.tick, system.tick);
    EXPECT_EQ(qpiReturned2.inputDataK12, digest2);
    EXPECT_FALSE(qpiReturned2.inputSignatureValid);
}
