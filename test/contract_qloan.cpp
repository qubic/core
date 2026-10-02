#define NO_UEFI

#include "contract_testing.h"

const id testAddress1 = ID(_H, _O, _G, _T, _K, _D, _N, _D, _V, _U, _U, _Z, _U, _F, _L, _A, _M, _L, _V, _B, _L, _Z, _D, _S, _G, _D, _D, _A, _E, _B, _E, _K, _K, _L, _N, _Z, _J, _B, _W, _S, _C, _A, _M, _D, _S, _X, _T, _C, _X, _A, _M, _A, _X, _U, _D, _F);
const id testAddress2 = ID(_E, _Q, _M, _B, _B, _V, _Y, _G, _Z, _O, _F, _U, _I, _H, _E, _X, _F, _O, _X, _K, _T, _F, _T, _A, _N, _E, _K, _B, _X, _L, _B, _X, _H, _A, _Y, _D, _F, _F, _M, _R, _E, _E, _M, _R, _Q, _E, _V, _A, _D, _Y, _M, _M, _E, _W, _A, _C);
const id testAddress3 = ID(_E, _C, _M, _C, _B, _V, _Y, _G, _Z, _O, _F, _L, _I, _H, _E, _X, _F, _O, _X, _K, _T, _F, _T, _A, _N, _E, _K, _B, _X, _L, _B, _X, _H, _A, _Y, _D, _F, _F, _M, _R, _E, _E, _M, _R, _Q, _E, _V, _A, _D, _Y, _M, _M, _E, _W, _A, _C);

class QLoanChecker : public QLOAN, public QLOAN::StateData
{
public:
    uint64 getLoanReqsPopulation()
    {
        return _loanReqs.population();
    }

    uint64 getTotalReqs()
    {
        return _totalReqs;
    }
};

class ContractTestingQLoan : protected ContractTesting
{
public:
    ContractTestingQLoan()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QLOAN);
        callSystemProcedure(QLOAN_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QLoanChecker* getState()
    {
        return (QLoanChecker*)contractStates[QLOAN_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QX_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
        callSystemProcedure(QLOAN_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QLOAN_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void issueAsset(const id& user, const std::string& assetName, const uint64& numberOfShares)
    {
        QX::IssueAsset_input input;
        input.assetName = assetNameFromString(assetName.c_str());
        input.numberOfShares = numberOfShares;
        input.unitOfMeasurement = 67;
        input.numberOfDecimalPlaces = 2;
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, user, 1000000000);
    }

    void transferRightsToQloan(const id& user, const std::string& assetName, const id& issuer, const uint64 numberOfShares)
    {
        QX::TransferShareManagementRights_input input;
        input.asset.assetName = assetNameFromString(assetName.c_str());
        input.asset.issuer = issuer;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = QLOAN_CONTRACT_INDEX;
        QX::TransferShareManagementRights_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, user, 2000);
    }

    void transferRightsToQx(const id& user, const std::string& assetName, const id& issuer, const uint64 numberOfShares)
    {
        QLOAN::TransferShareManagementRights_input input;
        input.asset.assetName = assetNameFromString(assetName.c_str());
        input.asset.issuer = issuer;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = QX_CONTRACT_INDEX;
        QLOAN::TransferShareManagementRights_output output;
        invokeUserProcedure(QLOAN_CONTRACT_INDEX, 4, input, output, user, 2000);
    }

    void placeLoanReq(const id& user, const std::string& assetName, const id& assetIssuer, const uint64& numberOfShares,
        const uint64& price, const uint64& interestRate, const uint64& returnPeriod, bool isLoanReq, bool assetsToCreditor,
        const int64_t& quAmount, const id& privateId = NULL_ID)
    {
        Asset testAsset;
        testAsset.assetName = assetNameFromString(assetName.c_str());
        testAsset.issuer = assetIssuer;

        QLOAN::PlaceLoanReq_input input;
        input.privateId = privateId;
        input.assets.set(0, testAsset);
        input.assetAmount.set(0, numberOfShares);
        input.assetsNum = 1;
        input.price = price;
        input.interestRate = interestRate;
        input.returnPeriodInEpochs = returnPeriod;
        input.isLoanReq = isLoanReq;
        input.assetsToCreditor = assetsToCreditor;

        QLOAN::PlaceLoanReq_output output;
        invokeUserProcedure(QLOAN_CONTRACT_INDEX, 1, input, output, user, quAmount);
    }

    void acceptLoanReq(const id& user, const uint64& reqId, const int64_t& quAmount)
    {
        QLOAN::AcceptLoanReq_input input{ reqId };
        QLOAN::AcceptLoanReq_output output;
        invokeUserProcedure(QLOAN_CONTRACT_INDEX, 3, input, output, user, quAmount);
    }

    void removeLoanReq(const id& user, const uint64& reqId, const int64_t& quAmount)
    {
        QLOAN::RemoveLoanReq_input input{ reqId };
        QLOAN::RemoveLoanReq_output output;
        invokeUserProcedure(QLOAN_CONTRACT_INDEX, 2, input, output, user, quAmount);
    }

    void payLoanDebt(const id& user, const uint64& reqId, const int64_t& quAmount)
    {
        QLOAN::PayLoanDebt_input input{ reqId };
        QLOAN::PayLoanDebt_output output;
        invokeUserProcedure(QLOAN_CONTRACT_INDEX, 5, input, output, user, quAmount);
    }

    QLOAN::GetAllLoanReqs_output getAllLoanReqs()
    {
        QLOAN::GetAllLoanReqs_input input;
        QLOAN::GetAllLoanReqs_output output;
        callFunction(QLOAN_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QLOAN::GetFeesInfo_output getFeesInfo()
    {
        QLOAN::GetFeesInfo_input input;
        QLOAN::GetFeesInfo_output output;
        callFunction(QLOAN_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QLOAN::GetUserDebt_output getUserDebt(const id& user)
    {
        QLOAN::GetUserDebt_input input{ user };
        QLOAN::GetUserDebt_output output;
        callFunction(QLOAN_CONTRACT_INDEX, 3, input, output);
        return output;
    }
};

TEST(ContractQLoan, PlaceLoanReqValidation)
{
    system.epoch = 250;
    ContractTestingQLoan qloan;
    qloan.beginEpoch();

    increaseEnergy(testAddress1, 2000000000);

    std::string assetName = "ABCD";
    qloan.placeLoanReq(testAddress1, assetName, testAddress1, 1000, 10, 10, 10, true, false, QLOAN_PLACE_LOAN_REQ_FEE);
    EXPECT_EQ(qloan.getState()->getLoanReqsPopulation(), 0);

    qloan.issueAsset(testAddress1, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress1, testAddress1, testAddress1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress1, assetName, testAddress1, 600);

    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress1, testAddress1, testAddress1, QLOAN_CONTRACT_INDEX, QLOAN_CONTRACT_INDEX), 600);
    qloan.placeLoanReq(testAddress1, assetName, testAddress1, 300, 10, 10, 10, true, false, QLOAN_PLACE_LOAN_REQ_FEE);
    EXPECT_EQ(qloan.getState()->getLoanReqsPopulation(), 1);

    qloan.placeLoanReq(testAddress1, assetName, testAddress1, 300, 10, 10, 10, true, false, QLOAN_PLACE_LOAN_REQ_FEE);
    EXPECT_EQ(qloan.getState()->getLoanReqsPopulation(), 2);
}

TEST(ContractQLoan, PlaceLoanPrivateRequest)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCE";
    qloan.issueAsset(testAddress1, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress1, testAddress1, testAddress1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress1, assetName, testAddress1, 300);

    qloan.placeLoanReq(testAddress1, assetName, testAddress1, 300, 2000, 10, 10, true, true, QLOAN_PLACE_LOAN_REQ_FEE, testAddress3);

    auto allReqsBefore = qloan.getAllLoanReqs();
    EXPECT_EQ(allReqsBefore.reqsAmount, 1);
    EXPECT_EQ(allReqsBefore.reqs.get(0).borrower, testAddress1);
    EXPECT_EQ(allReqsBefore.reqs.get(0).creditor, NULL_ID);
    EXPECT_EQ(allReqsBefore.reqs.get(0).privateId, testAddress3);
    EXPECT_EQ(allReqsBefore.reqs.get(0).state, QLOAN::LoanReqState::IDLE);

    uint64 reqId = allReqsBefore.reqs.get(0).reqId;
    qloan.acceptLoanReq(testAddress2, reqId, allReqsBefore.reqs.get(0).priceAmount);

    auto allReqsAfter = qloan.getAllLoanReqs();
    EXPECT_EQ(allReqsAfter.reqsAmount, 1);
    EXPECT_EQ(allReqsAfter.reqs.get(0).creditor, NULL_ID);
    EXPECT_EQ(allReqsAfter.reqs.get(0).state, QLOAN::LoanReqState::IDLE);
}

TEST(ContractQLoan, AcceptLoanReq)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCE";
    qloan.issueAsset(testAddress1, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress1, testAddress1, testAddress1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress1, assetName, testAddress1, 600);

    qloan.placeLoanReq(testAddress1, assetName, testAddress1, 300, 2000, 10, 10, true, false, QLOAN_PLACE_LOAN_REQ_FEE);

    auto allReqsBefore = qloan.getAllLoanReqs();
    EXPECT_EQ(allReqsBefore.reqsAmount, 1);
    EXPECT_EQ(allReqsBefore.reqs.get(0).borrower, testAddress1);
    EXPECT_EQ(allReqsBefore.reqs.get(0).creditor, NULL_ID);
    EXPECT_EQ(allReqsBefore.reqs.get(0).state, QLOAN::LoanReqState::IDLE);

    uint64 reqId = allReqsBefore.reqs.get(0).reqId;
    qloan.acceptLoanReq(testAddress2, reqId, allReqsBefore.reqs.get(0).priceAmount);

    auto allReqsAfter = qloan.getAllLoanReqs();
    EXPECT_EQ(allReqsAfter.reqsAmount, 1);
    EXPECT_EQ(allReqsAfter.reqs.get(0).creditor, testAddress2);
    EXPECT_EQ(allReqsAfter.reqs.get(0).acceptedBy, testAddress2);
    EXPECT_EQ(allReqsAfter.reqs.get(0).state, QLOAN::LoanReqState::ACTIVE);

    auto fees = qloan.getFeesInfo();
    EXPECT_EQ(fees.earnedAmount, QLOAN_PLACE_LOAN_REQ_FEE + 30);
}

TEST(ContractQLoan, AcceptLoanReqCreditRequest)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCF";
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 10, 10, false, false, QLOAN_PLACE_LOAN_REQ_FEE + 2000);

    qloan.issueAsset(testAddress2, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress2, testAddress2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress2, assetName, testAddress2, 300);

    auto allReqsBefore = qloan.getAllLoanReqs();
    uint64 reqId = allReqsBefore.reqs.get(0).reqId;
    qloan.acceptLoanReq(testAddress2, reqId, 1000);

    auto allReqsAfter = qloan.getAllLoanReqs();
    EXPECT_EQ(allReqsAfter.reqs.get(0).creditor, testAddress1);
    EXPECT_EQ(allReqsAfter.reqs.get(0).borrower, testAddress2);

    EXPECT_EQ(allReqsAfter.reqs.get(0).state, QLOAN::LoanReqState::ACTIVE);
}

TEST(ContractQLoan, RemoveLoanReq)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);

    std::string assetName = "ABCG";
    qloan.issueAsset(testAddress1, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress1, testAddress1, testAddress1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress1, assetName, testAddress1, 300);

    qloan.placeLoanReq(testAddress1, assetName, testAddress1, 300, 2000, 10, 10, true, false, QLOAN_PLACE_LOAN_REQ_FEE);

    EXPECT_EQ(qloan.getState()->getLoanReqsPopulation(), 1);
    EXPECT_EQ(qloan.getState()->getTotalReqs(), 1);

    auto allReqsAfter = qloan.getAllLoanReqs();
    uint64 reqId = allReqsAfter.reqs.get(0).reqId;
    qloan.removeLoanReq(testAddress1, reqId, 0);

    EXPECT_EQ(qloan.getState()->getLoanReqsPopulation(), 0);
    EXPECT_EQ(qloan.getState()->getTotalReqs(), 0);
}

TEST(ContractQLoan, EpochDebtCalculation)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCF";
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 10, 2, false, false, QLOAN_PLACE_LOAN_REQ_FEE + 2000);

    qloan.issueAsset(testAddress2, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress2, testAddress2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress2, assetName, testAddress2, 300);

    auto reqs = qloan.getAllLoanReqs();
    uint64 reqId = reqs.reqs.get(0).reqId;
    qloan.acceptLoanReq(testAddress2, reqId, 1000);

    reqs = qloan.getAllLoanReqs();
    EXPECT_EQ(reqs.reqs.get(0).creditor, testAddress1);
    EXPECT_EQ(reqs.reqs.get(0).borrower, testAddress2);

    qloan.beginEpoch();
    reqs = qloan.getAllLoanReqs();
    EXPECT_EQ(reqs.reqs.get(0).debtAmount, 2100);
    EXPECT_EQ(reqs.reqs.get(0).epochsLeft, 1);

    qloan.beginEpoch();
    reqs = qloan.getAllLoanReqs();
    EXPECT_EQ(reqs.reqs.get(0).debtAmount, 2200);
    EXPECT_EQ(reqs.reqs.get(0).epochsLeft, 0);
}

TEST(ContractQLoan, LoanExpiration)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCG";
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 10, 2, false, false, QLOAN_PLACE_LOAN_REQ_FEE + 2000);

    qloan.issueAsset(testAddress2, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress2, testAddress2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress2, assetName, testAddress2, 300);

    auto reqs = qloan.getAllLoanReqs();
    uint64 reqId = reqs.reqs.get(0).reqId;
    qloan.acceptLoanReq(testAddress2, reqId, 1000);
    EXPECT_EQ(reqs.reqs.get(0).debtAmount, 2066);
    EXPECT_EQ(1, qloan.getState()->_loanReqs.population());
    EXPECT_EQ(1, qloan.getState()->_totalReqs);

    for (int i = 0; i < 3; i++) qloan.beginEpoch();

    EXPECT_EQ(qloan.getState()->_loanReqs.population(), 0);
    EXPECT_EQ(qloan.getState()->_totalReqs, 0);
}

TEST(ContractQLoan, PayLoanDebt)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCH";
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 10, 10, false, true, QLOAN_PLACE_LOAN_REQ_FEE + 2000);

    qloan.issueAsset(testAddress2, assetName, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress2, testAddress2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 4000);
    qloan.transferRightsToQloan(testAddress2, assetName, testAddress2, 300);

    auto reqs = qloan.getAllLoanReqs();
    uint64 reqId = reqs.reqs.get(0).reqId;
    qloan.acceptLoanReq(testAddress2, reqId, 1000);

    for (int i = 0; i < 3; i++) qloan.beginEpoch();

    reqs = qloan.getAllLoanReqs();
    EXPECT_EQ(reqs.reqs.get(0).debtAmount, 2066);
    EXPECT_EQ(reqs.reqs.get(0).epochsLeft, 7);
    EXPECT_EQ(reqs.reqs.get(0).state, QLOAN::LoanReqState::ACTIVE);

    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress1, testAddress1, QLOAN_CONTRACT_INDEX, QLOAN_CONTRACT_INDEX), 300);
    qloan.transferRightsToQx(testAddress1, assetName, testAddress2, 200);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress1, testAddress1, QLOAN_CONTRACT_INDEX, QLOAN_CONTRACT_INDEX), 300);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress1, testAddress1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 0);

    qloan.payLoanDebt(testAddress2, reqs.reqs.get(0).reqId, reqs.reqs.get(0).debtAmount);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress1, testAddress1, QLOAN_CONTRACT_INDEX, QLOAN_CONTRACT_INDEX), 0);
    reqs = qloan.getAllLoanReqs();
    EXPECT_EQ(reqs.reqs.get(0).debtAmount, 0);
    EXPECT_EQ(qloan.getState()->_loanReqs.population(), 0);
    EXPECT_EQ(qloan.getState()->_totalReqs, 0);
}

TEST(ContractQLoan, FeesDistribution)
{
    ContractTestingQLoan qloan;

    increaseEnergy(testAddress1, 2000000000);

    std::string assetName = "ABCH";
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 10, 10, false, false, QLOAN_PLACE_LOAN_REQ_FEE + 2000);
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 10, 10, false, false, QLOAN_PLACE_LOAN_REQ_FEE + 2000);

    auto fees1 = qloan.getFeesInfo();
    EXPECT_EQ(fees1.earnedAmount, QLOAN_PLACE_LOAN_REQ_FEE * 2);
    EXPECT_EQ(fees1.distributedAmount, 0);
    EXPECT_EQ(fees1.toQvaultAmount, 0);

    qloan.endEpoch();

    auto fees2 = qloan.getFeesInfo();
    EXPECT_EQ(fees2.distributedAmount, 200000);
    EXPECT_EQ(fees2.burnedAmount, 6000);
    EXPECT_EQ(fees2.toQvaultAmount, 194000);
}

TEST(ContractQLoan, TotalUserDebt)
{
    ContractTestingQLoan qloan;
    qloan.beginEpoch();

    increaseEnergy(testAddress1, 2000000000);
    increaseEnergy(testAddress2, 2000000000);

    std::string assetName = "ABCK";
    qloan.placeLoanReq(testAddress1, assetName, testAddress2, 300, 2000, 100, 10, false, false, QLOAN_PLACE_LOAN_REQ_FEE + 2000);

    qloan.issueAsset(testAddress2, assetName, 4000);
    qloan.transferRightsToQloan(testAddress2, assetName, testAddress2, 300);

    auto reqs = qloan.getAllLoanReqs();

    qloan.acceptLoanReq(testAddress2, reqs.reqs.get(0).reqId, 0);

    auto fees1 = qloan.getFeesInfo();

    // 0
    auto userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 2666);
    qloan.endEpoch();
    
    // 1
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 2666);
    qloan.endEpoch();

    // 2
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 2666);
    qloan.endEpoch();

    // 3
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 2666);
    qloan.endEpoch();

    // 4
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 2800);
    qloan.endEpoch();

    // 5
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 3000);
    qloan.endEpoch();

    // 6
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 3200);
    qloan.endEpoch();

    // 7
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 3400);
    qloan.endEpoch();

    // 8
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 3600);
    qloan.endEpoch();

    // 9
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 3800);
    qloan.endEpoch();

    // 10
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 4000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress1, testAddress1, QLOAN_CONTRACT_INDEX, QLOAN_CONTRACT_INDEX), 0);
    qloan.endEpoch();

    // 11
    system.epoch++;
    qloan.beginEpoch();
    userDebt = qloan.getUserDebt(testAddress2);
    EXPECT_EQ(userDebt.totalUserDebt, 0);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(assetName.c_str()), testAddress2, testAddress1, testAddress1, QLOAN_CONTRACT_INDEX, QLOAN_CONTRACT_INDEX), 300);
    qloan.endEpoch();
}
