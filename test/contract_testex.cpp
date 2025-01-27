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
