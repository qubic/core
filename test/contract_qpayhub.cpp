#define NO_UEFI

#include "contract_testing.h"
#include "oracle_testing.h"

static const id SELLER1(1, 1, 1, 1);
static const id SELLER2(2, 2, 2, 2);
static const id BUYER1(11, 11, 11, 11);
static const id BUYER2(22, 22, 22, 22);
static const id RESOURCE1(101, 101, 101, 101);
static const id RESOURCE2(202, 202, 202, 202);
static const id SHAREHOLDER1(301, 301, 301, 301);
static const id TOKENHOLDER1(401, 401, 401, 401);
static const id OPERATOR1(501, 501, 501, 501);
static const id RECOVERY1(601, 601, 601, 601);
static const id IMPOSTOR1(701, 701, 701, 701);
static const id NEWOPERATOR1(801, 801, 801, 801);
static const id REGISTRAR1(901, 901, 901, 901);
static const id NEWREGISTRAR1(1001, 1001, 1001, 1001);
static const id AFFILIATE1(1101, 1101, 1101, 1101);
static const id AFFILIATE2(1201, 1201, 1201, 1201);

static const id QPAYHUB_CONTRACT_ID(QPAYHUB_CONTRACT_INDEX, 0, 0, 0);

// The exact issuer identity QPayhub.h's INITIALIZE() hardcodes for the QPAY
// dividend token: QPAYNOWSWZMGHFEAEVJXGZAVSHABAZDDBDIHTEBOPCOGHRGBCYCUZOHCVLXG.
// Copied verbatim so tests can issue the matching "QPAY" asset via QX and
// exercise the token-holder dividend path.
static const id QPAYHUB_DIVIDEND_TOKEN_ISSUER = ID(
    _Q, _P, _A, _Y, _N, _O, _W, _S,
    _W, _Z, _M, _G, _H, _F, _E, _A,
    _E, _V, _J, _X, _G, _Z, _A, _V,
    _S, _H, _A, _B, _A, _Z, _D, _D,
    _B, _D, _I, _H, _T, _E, _B, _O,
    _P, _C, _O, _G, _H, _R, _G, _B,
    _C, _Y, _C, _U, _Z, _O, _H, _C
);

// Exposes state fields directly (same pattern as StateCheckerTestExampleA in
// contract_testex.cpp: reinterpret the raw contract state buffer through a
// class that inherits both the contract and its StateData) plus the
// protected notification procedure id, needed to invoke the oracle-reply
// callback directly in tests (see invokeNotifyQuUsdPriceReply() below).
class QPayhubChecker : public QPAYHUB, public QPAYHUB::StateData
{
public:
    static unsigned int notifyQuUsdPriceReplyProcId()
    {
        return __id_NotifyQuUsdPriceReply;
    }
};

class ContractTestingQPayhub : protected ContractTesting
{
public:
    QX::Fees_output qxFees;

    ContractTestingQPayhub()
    {
        initEmptySpectrum();
        initEmptyUniverse();

        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QPAYHUB);
        callSystemProcedure(QPAYHUB_CONTRACT_INDEX, INITIALIZE);

        system.epoch = 200;
        system.tick = 123456783;
        etalonTick.year = 25;
        etalonTick.month = 12;
        etalonTick.day = 15;
        etalonTick.hour = 16;
        etalonTick.minute = 51;
        etalonTick.second = 12;

        // Needed for SubscribeToPriceFeed() / oracle-engine-backed tests only;
        // harmless setup for the tests that don't touch the oracle at all.
        // computorPublicKeys holds one entry per computorSeeds entry (currently
        // one), so it cannot supply NUMBER_OF_COMPUTORS distinct keys. Commit
        // quorum needs every computor to map to its own index via
        // computorIndex(), so use synthetic distinct keys - same approach as
        // OracleEngineTest in test/oracle_engine.cpp.
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
        {
            broadcastedComputors.computors.publicKeys[i] = m256i(i * 2, 42, 13, 1337);
        }
        EXPECT_TRUE(oracleEngine.init(broadcastedComputors.computors.publicKeys));
        EXPECT_TRUE(OI::initOracleInterfaces());
        EXPECT_TRUE(ts.init());
        ts.beginEpoch((unsigned int)system.tick);

        checkContractExecCleanup();

        callFunction(QX_CONTRACT_INDEX, 1, QX::Fees_input(), qxFees);
    }

    ~ContractTestingQPayhub()
    {
        oracleEngine.deinit();
        ts.deinit();
        checkContractExecCleanup();
    }

    QPayhubChecker* state()
    {
        return (QPayhubChecker*)contractStates[QPAYHUB_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QPAYHUB_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QPAYHUB_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    QPAYHUB::Pay_output pay(const id& payer, const id& seller, const id& resourceId, uint64 nonce, sint64 amount)
    {
        QPAYHUB::Pay_input input{ seller, resourceId, nonce };
        QPAYHUB::Pay_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 1, input, output, payer, amount);
        return output;
    }

    QPAYHUB::Consume_output consume(const id& invocator, const id& receiptKey, sint64 reward = 0)
    {
        QPAYHUB::Consume_input input{ receiptKey };
        QPAYHUB::Consume_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 2, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::GetReceipt_output getReceipt(const id& receiptKey)
    {
        QPAYHUB::GetReceipt_input input{ receiptKey };
        QPAYHUB::GetReceipt_output output;
        callFunction(QPAYHUB_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    id computeReceiptKey(const id& payer, const id& seller, const id& resourceId, uint64 nonce)
    {
        QPAYHUB::ComputeReceiptKey_input input{ payer, seller, resourceId, nonce };
        QPAYHUB::ComputeReceiptKey_output output;
        callFunction(QPAYHUB_CONTRACT_INDEX, 2, input, output);
        return output.receiptKey;
    }

    QPAYHUB::GetInfo_output getInfo()
    {
        QPAYHUB::GetInfo_input input;
        QPAYHUB::GetInfo_output output;
        callFunction(QPAYHUB_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QPAYHUB::GetQuUsdPrice_output getQuUsdPrice()
    {
        QPAYHUB::GetQuUsdPrice_input input;
        QPAYHUB::GetQuUsdPrice_output output;
        callFunction(QPAYHUB_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QPAYHUB::SubscribeToPriceFeed_output subscribeToPriceFeed(const id& invocator, sint64 reward)
    {
        QPAYHUB::SubscribeToPriceFeed_input input;
        QPAYHUB::SubscribeToPriceFeed_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 3, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::GetPromoRate_output getPromoRate(const id& seller)
    {
        QPAYHUB::GetPromoRate_input input{ seller };
        QPAYHUB::GetPromoRate_output output;
        callFunction(QPAYHUB_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QPAYHUB::SetPromoRate_output setPromoRate(const id& invocator, const id& seller, uint64 feePermille, sint64 reward = 0)
    {
        QPAYHUB::SetPromoRate_input input{ seller, feePermille };
        QPAYHUB::SetPromoRate_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 4, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::RemovePromoRate_output removePromoRate(const id& invocator, const id& seller, sint64 reward = 0)
    {
        QPAYHUB::RemovePromoRate_input input{ seller };
        QPAYHUB::RemovePromoRate_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 5, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::ChangeOperator_output changeOperator(const id& invocator, const id& newOperator, sint64 reward = 0)
    {
        QPAYHUB::ChangeOperator_input input{ newOperator };
        QPAYHUB::ChangeOperator_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 6, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::GetAffiliate_output getAffiliate(const id& seller)
    {
        QPAYHUB::GetAffiliate_input input{ seller };
        QPAYHUB::GetAffiliate_output output;
        callFunction(QPAYHUB_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QPAYHUB::SetAffiliate_output setAffiliate(const id& invocator, const id& seller, const id& affiliate, sint64 reward = 0)
    {
        QPAYHUB::SetAffiliate_input input{ seller, affiliate };
        QPAYHUB::SetAffiliate_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 7, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::RemoveAffiliate_output removeAffiliate(const id& invocator, const id& seller, sint64 reward = 0)
    {
        QPAYHUB::RemoveAffiliate_input input{ seller };
        QPAYHUB::RemoveAffiliate_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 8, input, output, invocator, reward);
        return output;
    }

    QPAYHUB::ChangeAffiliateRegistrar_output changeAffiliateRegistrar(const id& invocator, const id& newRegistrar, sint64 reward = 0)
    {
        QPAYHUB::ChangeAffiliateRegistrar_input input{ newRegistrar };
        QPAYHUB::ChangeAffiliateRegistrar_output output;
        invokeUserProcedure(QPAYHUB_CONTRACT_INDEX, 9, input, output, invocator, reward);
        return output;
    }

    // Directly invokes the private oracle-reply notification callback exactly
    // as qubic.cpp's contract processor does for USER_PROCEDURE_NOTIFICATION_CALL
    // (see QpiContextUserProcedureNotificationCall usage at qubic.cpp:2357).
    // This unit-tests NotifyQuUsdPriceReply's own guard-clause logic without
    // requiring the full oracle notification-queue delivery pipeline, which is
    // a separate concern owned by the oracle engine / qubic.cpp main loop.
    void invokeNotifyQuUsdPriceReply(const QPAYHUB::NotifyQuUsdPriceReply_input& input)
    {
        const UserProcedureRegistry::UserProcedureData* procData =
            userProcedureRegistry->get(QPayhubChecker::notifyQuUsdPriceReplyProcId());
        ASSERT_NE(procData, nullptr);
        QpiContextUserProcedureNotificationCall qpiContext(*procData);
        qpiContext.call(&input);
    }

    // Drives one real oracle query all the way through the commit + reveal
    // quorum pipeline so that oracleEngine.getOracleReply() has a genuine
    // resolved value for it - mirrors test/oracle_engine.cpp's
    // OracleEngine.ContractQuerySuccess test (the reference this was derived
    // from), collapsed to a single engine instance instead of three
    // simulated nodes, since here we only need one authoritative state.
    // NotifyQuUsdPriceReply() itself calls qpi.getOracleReply() rather than
    // trusting the notification input's reply field, so this is the only way
    // to exercise its "reply is actually fetched and validated" branches.
    sint64 startAndResolvePriceQuery(sint64 numerator, sint64 denominator)
    {
        OI::Price::OracleQuery query;
        query.oracle = OI::Price::getBinanceMexcOracleId();
        {
            using namespace Ch;
            query.currency1 = id(Q, U, B, I, C);
            query.currency2 = id(U, S, D, T);
        }
        query.timestamp = QPI::DateAndTime::now();

        const uint32 notificationProcId = QPayhubChecker::notifyQuUsdPriceReplyProcId();
        const uint32 timeout = 60000;
        sint64 queryId = oracleEngine.startContractQuery(
            QPAYHUB_CONTRACT_INDEX, OI::Price::oracleInterfaceIndex,
            &query, sizeof(query), timeout, notificationProcId);
        EXPECT_GE(queryId, 0);

        // Simulate the oracle machine node reply landing.
        struct
        {
            OracleMachineReply metadata;
            OI::Price::OracleReply data;
        } machineReply;
        machineReply.metadata.oracleMachineErrorFlags = 0;
        machineReply.metadata.oracleQueryId = queryId;
        machineReply.data.numerator = numerator;
        machineReply.data.denominator = denominator;
        oracleEngine.processOracleMachineReply(&machineReply.metadata, sizeof(machineReply));

        // Every computor commits to the identical reply digest, reaching quorum.
        uint8_t txBuffer[MAX_TRANSACTION_SIZE];
        auto* commitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
        system.tick += 3;
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
        {
            if (oracleEngine.getOracleQueryStatus(queryId) == ORACLE_QUERY_STATUS_COMMITTED)
                break;
            uint32_t rc = oracleEngine.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0);
            if (rc == 0)
                continue;
            EXPECT_TRUE(oracleEngine.processOracleReplyCommitTransaction(commitTx));
        }
        EXPECT_EQ(oracleEngine.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_COMMITTED);

        // Reveal the committed reply so it becomes retrievable via getOracleReply().
        system.tick += 3;
        uint32_t revealRc = oracleEngine.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0);
        EXPECT_NE(revealRc, 0u);
        system.tick += 3;
        auto* revealTx = (OracleReplyRevealTransactionPrefix*)txBuffer;
        const unsigned int txIndex = 0;
        addOracleTransactionToTickStorage(revealTx, txIndex);
        oracleEngine.processOracleReplyRevealTransaction(revealTx, txIndex);

        EXPECT_EQ(oracleEngine.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_SUCCESS);
        return queryId;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares)
    {
        QX::IssueAsset_input input;
        input.assetName = assetName;
        input.numberOfShares = numberOfShares;
        input.unitOfMeasurement = 0;
        input.numberOfDecimalPlaces = 0;
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, qxFees.assetIssuanceFee);
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
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, qxFees.transferFee);
        return output.transferredNumberOfShares;
    }
};

TEST(ContractQPayhub, ComputeReceiptKeyIsDeterministicAndSensitiveToEachField)
{
    ContractTestingQPayhub qpayhub;

    const id k1 = qpayhub.computeReceiptKey(BUYER1, SELLER1, RESOURCE1, 1);
    const id k1again = qpayhub.computeReceiptKey(BUYER1, SELLER1, RESOURCE1, 1);
    EXPECT_EQ(k1, k1again);

    EXPECT_NE(k1, qpayhub.computeReceiptKey(BUYER2, SELLER1, RESOURCE1, 1));
    EXPECT_NE(k1, qpayhub.computeReceiptKey(BUYER1, SELLER2, RESOURCE1, 1));
    EXPECT_NE(k1, qpayhub.computeReceiptKey(BUYER1, SELLER1, RESOURCE2, 1));
    EXPECT_NE(k1, qpayhub.computeReceiptKey(BUYER1, SELLER1, RESOURCE1, 2));
}

TEST(ContractQPayhub, PayHappyPathRecordsReceiptAndPaysSellerNetOfFee)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    const sint64 amount = 1000000;
    const sint64 expectedFee = 7500;  // 0.75% of 1,000,000
    const sint64 expectedNet = amount - expectedFee;

    const sint64 sellerBalanceBefore = getBalance(SELLER1);
    const sint64 buyerBalanceBefore = getBalance(BUYER1);

    auto output = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, amount);

    EXPECT_EQ(output.returnCode, QPAYHUB_OK);
    EXPECT_EQ(output.fee, expectedFee);
    EXPECT_EQ(output.net, expectedNet);
    EXPECT_EQ(output.receiptKey, qpayhub.computeReceiptKey(BUYER1, SELLER1, RESOURCE1, 1));

    EXPECT_EQ(getBalance(SELLER1), sellerBalanceBefore + expectedNet);
    EXPECT_EQ(getBalance(BUYER1), buyerBalanceBefore - amount);

    auto receipt = qpayhub.getReceipt(output.receiptKey);
    EXPECT_EQ(receipt.returnCode, QPAYHUB_OK);
    EXPECT_EQ(receipt.payer, BUYER1);
    EXPECT_EQ(receipt.seller, SELLER1);
    EXPECT_EQ(receipt.resourceId, RESOURCE1);
    EXPECT_EQ(receipt.nonce, 1ULL);
    EXPECT_EQ(receipt.amountPaid, amount);
    EXPECT_EQ(receipt.fee, expectedFee);
    EXPECT_EQ(receipt.epochPaid, (uint32)system.epoch);
    EXPECT_EQ(receipt.tickPaid, (uint32)system.tick);
    EXPECT_EQ(receipt.consumed, 0);

    auto info = qpayhub.getInfo();
    EXPECT_EQ(info.receiptCount, 1ULL);
    EXPECT_EQ(info.feePool, expectedFee);
    EXPECT_EQ(info.totalPayments, 1ULL);
    EXPECT_EQ(info.totalVolume, (uint64)amount);
    EXPECT_EQ(info.totalFeesCollected, (uint64)expectedFee);
}

TEST(ContractQPayhub, PayInvalidSellerNullIdAndSelfRefundsAndRejects)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    const sint64 amount = 5000;
    const sint64 balanceBefore = getBalance(BUYER1);

    auto outNull = qpayhub.pay(BUYER1, NULL_ID, RESOURCE1, 1, amount);
    EXPECT_EQ(outNull.returnCode, QPAYHUB_ERR_INVALID_SELLER);
    EXPECT_EQ(getBalance(BUYER1), balanceBefore);

    auto outSelf = qpayhub.pay(BUYER1, QPAYHUB_CONTRACT_ID, RESOURCE1, 2, amount);
    EXPECT_EQ(outSelf.returnCode, QPAYHUB_ERR_INVALID_SELLER);
    EXPECT_EQ(getBalance(BUYER1), balanceBefore);

    EXPECT_EQ(qpayhub.getInfo().receiptCount, 0ULL);
}

TEST(ContractQPayhub, PayAmountBelowMinimumRefundsAndRejects)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    const sint64 balanceBefore = getBalance(BUYER1);
    auto output = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, QPAYHUB_MIN_PAYMENT - 1);

    EXPECT_EQ(output.returnCode, QPAYHUB_ERR_AMOUNT_TOO_LOW);
    EXPECT_EQ(getBalance(BUYER1), balanceBefore);
    EXPECT_EQ(qpayhub.getInfo().receiptCount, 0ULL);
}

TEST(ContractQPayhub, PayDuplicateKeyRefundsAndRejects)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    auto first = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);
    EXPECT_EQ(first.returnCode, QPAYHUB_OK);

    const sint64 balanceBeforeDup = getBalance(BUYER1);
    auto dup = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);

    EXPECT_EQ(dup.returnCode, QPAYHUB_ERR_DUPLICATE);
    EXPECT_EQ(getBalance(BUYER1), balanceBeforeDup);
    EXPECT_EQ(qpayhub.getInfo().receiptCount, 1ULL);
}

TEST(ContractQPayhub, PayFeeFloorDominatesBelowPercentThreshold)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    // 0.75% of 5000 is 37 (5000*75/10000), below the 100 QU floor, so the floor applies.
    auto output = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);
    EXPECT_EQ(output.returnCode, QPAYHUB_OK);
    EXPECT_EQ(output.fee, QPAYHUB_FEE_FLOOR_QU);
    EXPECT_EQ(output.net, 5000 - QPAYHUB_FEE_FLOOR_QU);
}

TEST(ContractQPayhub, PayFeePercentDominatesAbovePercentThreshold)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    // 0.75% of 20000 is 150 (20000*75/10000), above the 100 QU floor, so the percentage applies.
    auto output = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 20000);
    EXPECT_EQ(output.returnCode, QPAYHUB_OK);
    EXPECT_EQ(output.fee, 150);
    EXPECT_EQ(output.net, 20000 - 150);
}

TEST(ContractQPayhub, PayFeeBoundaryJustBelowAndAboveFloor)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    // 0.75% of 13200 is exactly 99 (13200*75/10000), just below the 100 QU
    // floor, so the floor applies.
    auto below = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 13200);
    EXPECT_EQ(below.returnCode, QPAYHUB_OK);
    EXPECT_EQ(below.fee, 100);
    EXPECT_EQ(below.net, 13200 - 100);

    // 0.75% of 13600 is exactly 102 (13600*75/10000), just above the 100 QU
    // floor, so the percentage applies.
    auto above = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 2, 13600);
    EXPECT_EQ(above.returnCode, QPAYHUB_OK);
    EXPECT_EQ(above.fee, 102);
    EXPECT_EQ(above.net, 13600 - 102);
}

TEST(ContractQPayhub, PayAtMinimumPaymentClampsFeeToAmountForZeroNet)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    // At the minimum payment, the floor would exceed the amount and gets
    // clamped: fee == amount, net == 0, never negative.
    auto output = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, QPAYHUB_MIN_PAYMENT);
    EXPECT_EQ(output.returnCode, QPAYHUB_OK);
    EXPECT_EQ(output.fee, QPAYHUB_MIN_PAYMENT);
    EXPECT_EQ(output.net, 0);
}

TEST(ContractQPayhub, ConsumeHappyPathMarksConsumedBySeller)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    auto payOut = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);
    auto consumeOut = qpayhub.consume(SELLER1, payOut.receiptKey);

    EXPECT_EQ(consumeOut.returnCode, QPAYHUB_OK);

    auto receipt = qpayhub.getReceipt(payOut.receiptKey);
    EXPECT_EQ(receipt.consumed, 1);
    EXPECT_EQ(qpayhub.getInfo().totalConsumed, 1ULL);
}

TEST(ContractQPayhub, ConsumeByNonSellerIsRejected)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    auto payOut = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);
    auto consumeOut = qpayhub.consume(BUYER1, payOut.receiptKey);

    EXPECT_EQ(consumeOut.returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    EXPECT_EQ(qpayhub.getReceipt(payOut.receiptKey).consumed, 0);
}

TEST(ContractQPayhub, ConsumeAlreadyConsumedIsRejected)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    auto payOut = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);
    EXPECT_EQ(qpayhub.consume(SELLER1, payOut.receiptKey).returnCode, QPAYHUB_OK);

    auto second = qpayhub.consume(SELLER1, payOut.receiptKey);
    EXPECT_EQ(second.returnCode, QPAYHUB_ERR_ALREADY_CONSUMED);
    EXPECT_EQ(qpayhub.getInfo().totalConsumed, 1ULL);
}

TEST(ContractQPayhub, ConsumeNotFoundIsRejected)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(SELLER1, 10000000);

    auto output = qpayhub.consume(SELLER1, id::randomValue());
    EXPECT_EQ(output.returnCode, QPAYHUB_ERR_NOT_FOUND);
}

TEST(ContractQPayhub, ConsumeRefundsLeftoverInvocationReward)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);
    increaseEnergy(SELLER1, 10000000);

    auto payOut = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);

    const sint64 sellerBalanceBefore = getBalance(SELLER1);
    auto consumeOut = qpayhub.consume(SELLER1, payOut.receiptKey, 777);

    EXPECT_EQ(consumeOut.returnCode, QPAYHUB_OK);
    // The 777 QU attached to Consume is not a price; it must come straight back.
    EXPECT_EQ(getBalance(SELLER1), sellerBalanceBefore);
}

TEST(ContractQPayhub, GetReceiptNotFoundForUnknownKey)
{
    ContractTestingQPayhub qpayhub;
    auto receipt = qpayhub.getReceipt(id::randomValue());
    EXPECT_EQ(receipt.returnCode, QPAYHUB_ERR_NOT_FOUND);
}

TEST(ContractQPayhub, GetInfoReflectsRunningTotalsAndConstants)
{
    ContractTestingQPayhub qpayhub;

    auto emptyInfo = qpayhub.getInfo();
    EXPECT_EQ(emptyInfo.feePermille, QPAYHUB_FEE_PERMILLE);
    EXPECT_EQ(emptyInfo.minPayment, QPAYHUB_MIN_PAYMENT);
    EXPECT_EQ(emptyInfo.retentionEpochs, QPAYHUB_RECEIPT_RETENTION_EPOCHS);
    EXPECT_EQ(emptyInfo.feeFloorQu, QPAYHUB_FEE_FLOOR_QU);
    EXPECT_EQ(emptyInfo.shareholderPermille, QPAYHUB_DIVIDEND_SHAREHOLDER_PERMILLE);
    EXPECT_EQ(emptyInfo.receiptCount, 0ULL);
    EXPECT_EQ(emptyInfo.feePool, 0);

    increaseEnergy(BUYER1, 10000000);
    increaseEnergy(BUYER2, 10000000);
    qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 20000);
    qpayhub.pay(BUYER2, SELLER2, RESOURCE2, 2, 30000);
    auto payOut1 = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 3, 20000);
    qpayhub.consume(SELLER1, payOut1.receiptKey);

    auto info = qpayhub.getInfo();
    EXPECT_EQ(info.receiptCount, 3ULL);
    EXPECT_EQ(info.totalPayments, 3ULL);
    EXPECT_EQ(info.totalVolume, 70000ULL);
    EXPECT_EQ(info.totalFeesCollected, 525ULL); // 150 + 225 + 150
    EXPECT_EQ(info.feePool, 525);
    EXPECT_EQ(info.totalConsumed, 1ULL);
}

TEST(ContractQPayhub, PostIncomingTransferCountsDonationTypesButNotProcedureTransaction)
{
    ContractTestingQPayhub qpayhub;

    auto donate = [&](uint8 type, sint64 amount)
    {
        QpiContextSystemProcedureCall qpiContext(QPAYHUB_CONTRACT_INDEX, POST_INCOMING_TRANSFER);
        QPI::PostIncomingTransfer_input input{ BUYER1, amount, type };
        qpiContext.call(input);
    };

    EXPECT_EQ(qpayhub.getInfo().feePool, 0);

    donate(QPI::TransferType::standardTransaction, 1000);
    EXPECT_EQ(qpayhub.getInfo().feePool, 1000);

    donate(QPI::TransferType::qpiTransfer, 500);
    EXPECT_EQ(qpayhub.getInfo().feePool, 1500);

    donate(QPI::TransferType::qpiDistributeDividends, 250);
    EXPECT_EQ(qpayhub.getInfo().feePool, 1750);

    donate(QPI::TransferType::revenueDonation, 100);
    EXPECT_EQ(qpayhub.getInfo().feePool, 1850);

    // Procedure-attached amounts are already accounted for inside Pay itself;
    // counting them here too would double-count every Pay call.
    donate(QPI::TransferType::procedureTransaction, 999999);
    EXPECT_EQ(qpayhub.getInfo().feePool, 1850);
}

TEST(ContractQPayhub, EndEpochPurgesReceiptsPastRetentionWindow)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    system.epoch = 200;
    auto oldReceipt = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 5000);

    system.epoch = 202;
    auto freshReceipt = qpayhub.pay(BUYER1, SELLER1, RESOURCE2, 2, 5000);

    // cutoff = 203 - QPAYHUB_RECEIPT_RETENTION_EPOCHS(2) = 201; epochPaid 200 < 201 purges,
    // epochPaid 202 survives.
    system.epoch = 203;
    qpayhub.endEpoch();

    EXPECT_EQ(qpayhub.getReceipt(oldReceipt.receiptKey).returnCode, QPAYHUB_ERR_NOT_FOUND);
    EXPECT_EQ(qpayhub.getReceipt(freshReceipt.receiptKey).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.getInfo().totalPurged, 1ULL);
}

TEST(ContractQPayhub, EndEpochDistributesFeePoolAboveReserveToSharesAndTokenHolders)
{
    ContractTestingQPayhub qpayhub;

    // distributable = feePool - QPAYHUB_EXEC_RESERVE(1,000,000) = 676,000,
    // chosen so shareholderPart (10%) divides evenly by NUMBER_OF_COMPUTORS.
    const sint64 feePoolAmount = QPAYHUB_EXEC_RESERVE + 676000;
    qpayhub.state()->feePool = feePoolAmount;
    increaseEnergy(QPAYHUB_CONTRACT_ID, feePoolAmount + 1000000);

    std::vector<std::pair<m256i, unsigned int>> qpayhubShares{
        { SHAREHOLDER1, NUMBER_OF_COMPUTORS }
    };
    issueContractShares(QPAYHUB_CONTRACT_INDEX, qpayhubShares);

    // QX charges 1,000,000,000 QU to issue an asset - fund the issuer for the
    // real fee, otherwise IssueAsset refunds and issues nothing.
    increaseEnergy(QPAYHUB_DIVIDEND_TOKEN_ISSUER, qpayhub.qxFees.assetIssuanceFee + qpayhub.qxFees.transferFee + 10000000);
    EXPECT_EQ(qpayhub.issueAsset(QPAYHUB_DIVIDEND_TOKEN_ISSUER, QPAYHUB_TOKEN_ASSETNAME, 1000000), 1000000);
    EXPECT_EQ(qpayhub.transferAsset(QPAYHUB_DIVIDEND_TOKEN_ISSUER, TOKENHOLDER1, QPAYHUB_TOKEN_ASSETNAME, QPAYHUB_DIVIDEND_TOKEN_ISSUER, 1000000), 1000000);

    const sint64 shareholderBalanceBefore = getBalance(SHAREHOLDER1);
    const sint64 tokenholderBalanceBefore = getBalance(TOKENHOLDER1);

    qpayhub.endEpoch();

    const sint64 expectedShareholderTotal = 67600;  // 10% of 676,000
    const sint64 expectedBurnTotal = 6760;          // 1% of 676,000 - BURN ADDITION
    const sint64 expectedTokenholderTotal = 601640; // remaining 89%

    EXPECT_EQ(getBalance(SHAREHOLDER1), shareholderBalanceBefore + expectedShareholderTotal);
    EXPECT_EQ(getBalance(TOKENHOLDER1), tokenholderBalanceBefore + expectedTokenholderTotal);

    // The three slices account for every distributable QU, with no remainder
    // stranded in feePool.
    EXPECT_EQ(expectedShareholderTotal + expectedBurnTotal + expectedTokenholderTotal, 676000);

    auto info = qpayhub.getInfo();
    EXPECT_EQ(info.totalShareholderDividends, (uint64)expectedShareholderTotal);
    EXPECT_EQ(info.totalTokenholderDividends, (uint64)expectedTokenholderTotal);
    EXPECT_EQ(info.totalBurned, (uint64)expectedBurnTotal);
    EXPECT_EQ(info.burnPermille, QPAYHUB_DIVIDEND_BURN_PERMILLE);
    EXPECT_EQ(info.feePool, QPAYHUB_EXEC_RESERVE);
}

// BURN ADDITION. The point of the 1% is not to destroy supply - qpi.burn()
// with no explicit contract index credits the *calling* contract's execution
// fee reserve (qpi_spectrum_impl.h: burn -> addToContractFeeReserve). So the
// QU has to leave the contract balance and land in this contract's reserve,
// which is what makes QPayhub pay for its own execution out of its own
// revenue instead of draining a reserve nothing refills. Deliberately runs
// with no QPAY token issued at all: the burn must not depend on there being
// token holders to pay.
TEST(ContractQPayhub, EndEpochBurnsOnePercentIntoOwnExecutionFeeReserve)
{
    ContractTestingQPayhub qpayhub;

    const sint64 feePoolAmount = QPAYHUB_EXEC_RESERVE + 676000;
    qpayhub.state()->feePool = feePoolAmount;
    increaseEnergy(QPAYHUB_CONTRACT_ID, feePoolAmount + 1000000);

    std::vector<std::pair<m256i, unsigned int>> qpayhubShares{
        { SHAREHOLDER1, NUMBER_OF_COMPUTORS }
    };
    issueContractShares(QPAYHUB_CONTRACT_INDEX, qpayhubShares);

    const sint64 reserveBefore = getContractFeeReserve(QPAYHUB_CONTRACT_INDEX);
    const sint64 contractBalanceBefore = getBalance(QPAYHUB_CONTRACT_ID);

    qpayhub.endEpoch();

    const sint64 expectedBurnTotal = 6760;         // 1% of 676,000
    const sint64 expectedShareholderTotal = 67600; // 10%, paid out to shares

    EXPECT_EQ(getContractFeeReserve(QPAYHUB_CONTRACT_INDEX), reserveBefore + expectedBurnTotal);
    EXPECT_EQ(qpayhub.getInfo().totalBurned, (uint64)expectedBurnTotal);

    // Both the burn and the shareholder payout leave the contract's balance.
    // The 89% token slice does not: no QPAY asset exists here, so it stays in
    // feePool for a later epoch.
    EXPECT_EQ(getBalance(QPAYHUB_CONTRACT_ID),
              contractBalanceBefore - expectedBurnTotal - expectedShareholderTotal);
    EXPECT_EQ(qpayhub.getInfo().feePool,
              feePoolAmount - expectedBurnTotal - expectedShareholderTotal);
}

// BURN ADDITION. The burn is sized off `distributable`, so a fee pool at or
// below the execution reserve must burn nothing at all - END_EPOCH returns
// before the split. Guards against the burn quietly eating into the reserve
// the contract is supposed to be protecting.
TEST(ContractQPayhub, EndEpochBurnsNothingWhenFeePoolAtOrBelowReserve)
{
    ContractTestingQPayhub qpayhub;

    qpayhub.state()->feePool = QPAYHUB_EXEC_RESERVE;
    increaseEnergy(QPAYHUB_CONTRACT_ID, QPAYHUB_EXEC_RESERVE);

    const sint64 reserveBefore = getContractFeeReserve(QPAYHUB_CONTRACT_INDEX);

    qpayhub.endEpoch();

    EXPECT_EQ(getContractFeeReserve(QPAYHUB_CONTRACT_INDEX), reserveBefore);
    EXPECT_EQ(qpayhub.getInfo().totalBurned, 0ULL);
    EXPECT_EQ(qpayhub.getInfo().feePool, QPAYHUB_EXEC_RESERVE);
}

TEST(ContractQPayhub, EndEpochDoesNotDistributeWhenFeePoolAtOrBelowReserve)
{
    ContractTestingQPayhub qpayhub;

    qpayhub.state()->feePool = QPAYHUB_EXEC_RESERVE - 1;
    increaseEnergy(QPAYHUB_CONTRACT_ID, QPAYHUB_EXEC_RESERVE);

    std::vector<std::pair<m256i, unsigned int>> qpayhubShares{
        { SHAREHOLDER1, NUMBER_OF_COMPUTORS }
    };
    issueContractShares(QPAYHUB_CONTRACT_INDEX, qpayhubShares);

    const sint64 shareholderBalanceBefore = getBalance(SHAREHOLDER1);
    qpayhub.endEpoch();

    EXPECT_EQ(getBalance(SHAREHOLDER1), shareholderBalanceBefore);
    auto info = qpayhub.getInfo();
    EXPECT_EQ(info.totalShareholderDividends, 0ULL);
    EXPECT_EQ(info.totalTokenholderDividends, 0ULL);
    EXPECT_EQ(info.feePool, QPAYHUB_EXEC_RESERVE - 1);
}

TEST(ContractQPayhub, SubscribeToPriceFeedFeeTooLowRefundsAndRejects)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    const sint64 balanceBefore = getBalance(BUYER1);
    auto output = qpayhub.subscribeToPriceFeed(BUYER1, 100);

    EXPECT_EQ(output.returnCode, QPAYHUB_ERR_SUBSCRIBE_FAILED);
    EXPECT_EQ(getBalance(BUYER1), balanceBefore);
    EXPECT_LT(qpayhub.state()->priceOracleSubscriptionId, 0);
}

TEST(ContractQPayhub, SubscribeToPriceFeedSuccessStoresSubscriptionAndRefundsExcess)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    OI::Price::OracleQuery dummyQuery;
    const sint64 requiredFee = OI::Price::getSubscriptionFee(dummyQuery, QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS);
    const sint64 excess = 500;
    const sint64 balanceBefore = getBalance(BUYER1);

    auto output = qpayhub.subscribeToPriceFeed(BUYER1, requiredFee + excess);

    EXPECT_EQ(output.returnCode, QPAYHUB_OK);
    EXPECT_GE(output.subscriptionId, 0);
    EXPECT_EQ(qpayhub.state()->priceOracleSubscriptionId, output.subscriptionId);
    EXPECT_NE(oracleEngine.getOracleSubscription(output.subscriptionId), nullptr);

    // Only the exact fee left the buyer's balance; the excess was refunded.
    EXPECT_EQ(getBalance(BUYER1), balanceBefore - requiredFee);
}

TEST(ContractQPayhub, SubscribeToPriceFeedAlreadySubscribedRefundsAndRejects)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    OI::Price::OracleQuery dummyQuery;
    const sint64 requiredFee = OI::Price::getSubscriptionFee(dummyQuery, QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS);

    auto first = qpayhub.subscribeToPriceFeed(BUYER1, requiredFee);
    EXPECT_EQ(first.returnCode, QPAYHUB_OK);

    const sint64 balanceBefore = getBalance(BUYER1);
    auto second = qpayhub.subscribeToPriceFeed(BUYER1, requiredFee + 1000);

    EXPECT_EQ(second.returnCode, QPAYHUB_ERR_ALREADY_SUBSCRIBED);
    EXPECT_EQ(second.subscriptionId, first.subscriptionId);
    // The whole attached reward is refunded; the already-subscribed check
    // runs before any fee is even computed.
    EXPECT_EQ(getBalance(BUYER1), balanceBefore);
}

// The currency pair QPayhub subscribes to has to be exactly QUBIC/USDT.
// id()'s char constructor needs at least 5 characters (m256.h: c0..c4 have no
// defaults), so a 4-character id(U, S, D, T) silently binds to the
// id(uint64, uint64, uint64, uint64) overload instead and yields a completely
// different value - the ASCII codes as four 64-bit limbs rather than "USDT"
// packed into the first four bytes. The engine dedups subscriptions by query
// content with the timestamp excluded (see startContractSubscription), so a
// probe subscription carrying the intended query from a *different* contract
// must land on the very same subscription id. If QPayhub asked for something
// else, the probe allocates a second subscription instead.
TEST(ContractQPayhub, SubscribeToPriceFeedUsesQubicUsdtCurrencyPair)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    OI::Price::OracleQuery dummyQuery;
    const sint64 requiredFee = OI::Price::getSubscriptionFee(dummyQuery, QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS);

    auto output = qpayhub.subscribeToPriceFeed(BUYER1, requiredFee);
    ASSERT_EQ(output.returnCode, QPAYHUB_OK);
    ASSERT_GE(output.subscriptionId, 0);

    OI::Price::OracleQuery expected;
    expected.oracle = OI::Price::getBinanceMexcOracleId();
    expected.timestamp = QPI::DateAndTime::now(); // excluded from the comparison
    {
        using namespace QPI::Ch;
        expected.currency1 = id(Q, U, B, I, C);
        expected.currency2 = id(U, S, D, T, null);
    }

    const sint32 probeSubscriptionId = oracleEngine.startContractSubscription(
        QX_CONTRACT_INDEX, OI::Price::oracleInterfaceIndex, &expected, sizeof(expected),
        QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS, 1, offsetof(OI::Price::OracleQuery, timestamp));

    EXPECT_EQ(probeSubscriptionId, output.subscriptionId);
}

// The node drops every oracle subscription at the epoch boundary
// (qubic.cpp beginEpoch() -> oracleEngine.beginEpoch() -> reset(), documented
// there as "Drop all queries of the previous epoch") and only afterwards runs
// each contract's BEGIN_EPOCH. If QPayhub kept its stale subscription id, the
// >= 0 guard in SubscribeToPriceFeed would reject every renewal from then on
// and the price feed would stay dead for the life of the contract.
TEST(ContractQPayhub, BeginEpochClearsStaleSubscriptionSoFeedCanBeRenewed)
{
    ContractTestingQPayhub qpayhub;
    increaseEnergy(BUYER1, 10000000);

    OI::Price::OracleQuery dummyQuery;
    const sint64 requiredFee = OI::Price::getSubscriptionFee(dummyQuery, QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS);

    auto first = qpayhub.subscribeToPriceFeed(BUYER1, requiredFee);
    ASSERT_EQ(first.returnCode, QPAYHUB_OK);
    ASSERT_GE(first.subscriptionId, 0);

    // Epoch rollover, in the order the node performs it.
    qpayhub.endEpoch();
    oracleEngine.beginEpoch();
    qpayhub.beginEpoch();

    EXPECT_LT(qpayhub.state()->priceOracleSubscriptionId, 0);

    auto second = qpayhub.subscribeToPriceFeed(BUYER1, requiredFee);
    EXPECT_EQ(second.returnCode, QPAYHUB_OK);
    EXPECT_GE(second.subscriptionId, 0);
    EXPECT_EQ(qpayhub.state()->priceOracleSubscriptionId, second.subscriptionId);
}

TEST(ContractQPayhub, GetQuUsdPriceFreshContractIsStale)
{
    ContractTestingQPayhub qpayhub;
    auto price = qpayhub.getQuUsdPrice();
    EXPECT_EQ(price.denominator, 0);
    EXPECT_TRUE(price.stale);
}

TEST(ContractQPayhub, GetQuUsdPriceRecentUpdateIsNotStale)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->quUsdNumerator = 12345;
    qpayhub.state()->quUsdDenominator = 100;
    qpayhub.state()->quUsdUpdatedTick = (uint32)system.tick;

    auto price = qpayhub.getQuUsdPrice();
    EXPECT_EQ(price.numerator, 12345);
    EXPECT_EQ(price.denominator, 100);
    EXPECT_FALSE(price.stale);
}

TEST(ContractQPayhub, GetQuUsdPriceOldUpdateIsStale)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->quUsdNumerator = 12345;
    qpayhub.state()->quUsdDenominator = 100;
    qpayhub.state()->quUsdUpdatedTick = (uint32)system.tick;

    system.tick += QPAYHUB_PRICE_STALE_TICKS + 1;

    auto price = qpayhub.getQuUsdPrice();
    EXPECT_TRUE(price.stale);
}

TEST(ContractQPayhub, NotifyQuUsdPriceReplyIgnoresNonSuccessStatus)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->quUsdDenominator = 0;

    QPAYHUB::NotifyQuUsdPriceReply_input input{};
    input.queryId = -1;
    input.subscriptionId = -1;
    input.status = ORACLE_QUERY_STATUS_TIMEOUT;
    input.reply.numerator = 999;
    input.reply.denominator = 1;

    qpayhub.invokeNotifyQuUsdPriceReply(input);

    EXPECT_EQ(qpayhub.state()->quUsdDenominator, 0);
    EXPECT_TRUE(qpayhub.getQuUsdPrice().stale);
}

TEST(ContractQPayhub, NotifyQuUsdPriceReplyIgnoresUnresolvedQueryId)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->quUsdDenominator = 0;

    // status is SUCCESS, but this queryId was never actually queried/resolved
    // in the oracle engine, so qpi.getOracleReply() must fail and the
    // notification must be a no-op.
    QPAYHUB::NotifyQuUsdPriceReply_input input{};
    input.queryId = 424242;
    input.subscriptionId = -1;
    input.status = ORACLE_QUERY_STATUS_SUCCESS;
    input.reply.numerator = 999;
    input.reply.denominator = 1;

    qpayhub.invokeNotifyQuUsdPriceReply(input);

    EXPECT_EQ(qpayhub.state()->quUsdDenominator, 0);
    EXPECT_TRUE(qpayhub.getQuUsdPrice().stale);
}

TEST(ContractQPayhub, NotifyQuUsdPriceReplyWithValidResolvedReplyUpdatesState)
{
    ContractTestingQPayhub qpayhub;

    const sint64 numerator = 1234;
    const sint64 denominator = 1000;
    sint64 queryId = qpayhub.startAndResolvePriceQuery(numerator, denominator);

    QPAYHUB::NotifyQuUsdPriceReply_input input{};
    input.queryId = queryId;
    input.subscriptionId = -1;
    input.status = ORACLE_QUERY_STATUS_SUCCESS;
    // Deliberately left as garbage: NotifyQuUsdPriceReply must fetch the
    // reply itself via qpi.getOracleReply(), not trust this field.
    input.reply.numerator = -1;
    input.reply.denominator = -1;

    qpayhub.invokeNotifyQuUsdPriceReply(input);

    EXPECT_EQ(qpayhub.state()->quUsdNumerator, numerator);
    EXPECT_EQ(qpayhub.state()->quUsdDenominator, denominator);
    EXPECT_EQ(qpayhub.state()->quUsdUpdatedTick, (uint32)system.tick);

    auto price = qpayhub.getQuUsdPrice();
    EXPECT_EQ(price.numerator, numerator);
    EXPECT_EQ(price.denominator, denominator);
    EXPECT_FALSE(price.stale);
}

TEST(ContractQPayhub, NotifyQuUsdPriceReplyWithInvalidResolvedReplyIsIgnored)
{
    ContractTestingQPayhub qpayhub;

    // denominator == 0 fails OI::Price::replyIsValid().
    sint64 queryId = qpayhub.startAndResolvePriceQuery(1234, 0);

    QPAYHUB::NotifyQuUsdPriceReply_input input{};
    input.queryId = queryId;
    input.subscriptionId = -1;
    input.status = ORACLE_QUERY_STATUS_SUCCESS;

    qpayhub.invokeNotifyQuUsdPriceReply(input);

    EXPECT_EQ(qpayhub.state()->quUsdDenominator, 0);
    EXPECT_TRUE(qpayhub.getQuUsdPrice().stale);
}

TEST(ContractQPayhub, GetPromoRateReflectsStandardRateWhenUnset)
{
    ContractTestingQPayhub qpayhub;

    auto rate = qpayhub.getPromoRate(SELLER1);
    EXPECT_EQ(rate.feePermille, QPAYHUB_FEE_PERMILLE);
    EXPECT_EQ(rate.isPromo, 0);
}

TEST(ContractQPayhub, SetPromoRateByOperatorAffectsPayAndGetPromoRate)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);
    increaseEnergy(BUYER1, 10000000);

    // 0.50%, between the 0.25% floor and the 0.75% standard rate.
    auto setOut = qpayhub.setPromoRate(OPERATOR1, SELLER1, 50);
    EXPECT_EQ(setOut.returnCode, QPAYHUB_OK);

    auto rate = qpayhub.getPromoRate(SELLER1);
    EXPECT_EQ(rate.feePermille, 50ULL);
    EXPECT_EQ(rate.isPromo, 1);

    // A different seller is unaffected - the override is per-seller, not global.
    auto otherRate = qpayhub.getPromoRate(SELLER2);
    EXPECT_EQ(otherRate.feePermille, QPAYHUB_FEE_PERMILLE);
    EXPECT_EQ(otherRate.isPromo, 0);

    // amount=1,000,000: standard fee would be 7500 (0.75%); the promo rate
    // must actually be applied by Pay(), not just stored.
    auto payOut = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 1000000);
    EXPECT_EQ(payOut.returnCode, QPAYHUB_OK);
    EXPECT_EQ(payOut.fee, 5000); // 0.50% of 1,000,000
    EXPECT_LT(payOut.fee, 7500); // strictly cheaper than the standard rate
}

TEST(ContractQPayhub, SetPromoRateByNonOperatorRejectedAndRefunded)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(IMPOSTOR1, 10000000);

    const sint64 balanceBefore = getBalance(IMPOSTOR1);
    auto out = qpayhub.setPromoRate(IMPOSTOR1, SELLER1, 50, 1000);

    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    EXPECT_EQ(getBalance(IMPOSTOR1), balanceBefore);
    EXPECT_EQ(qpayhub.getPromoRate(SELLER1).isPromo, 0); // rejected call never touched state
}

TEST(ContractQPayhub, SetPromoRateInvalidSellerRejected)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);

    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, NULL_ID, 50).returnCode, QPAYHUB_ERR_INVALID_SELLER);
    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, QPAYHUB_CONTRACT_ID, 50).returnCode, QPAYHUB_ERR_INVALID_SELLER);
}

TEST(ContractQPayhub, SetPromoRateBelowFloorRejected)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);

    auto below = qpayhub.setPromoRate(OPERATOR1, SELLER1, QPAYHUB_PROMO_FLOOR_PERMILLE - 1);
    EXPECT_EQ(below.returnCode, QPAYHUB_ERR_INVALID_RATE);
    EXPECT_EQ(qpayhub.getPromoRate(SELLER1).isPromo, 0);

    auto atFloor = qpayhub.setPromoRate(OPERATOR1, SELLER1, QPAYHUB_PROMO_FLOOR_PERMILLE);
    EXPECT_EQ(atFloor.returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.getPromoRate(SELLER1).feePermille, QPAYHUB_PROMO_FLOOR_PERMILLE);
}

TEST(ContractQPayhub, SetPromoRateAboveStandardRejected)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);

    // A promo rate can only ever discount - never charge a seller MORE
    // than the standard rate everyone else pays.
    auto out = qpayhub.setPromoRate(OPERATOR1, SELLER1, QPAYHUB_FEE_PERMILLE + 1);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_INVALID_RATE);

    // The standard rate itself is accepted (a no-op discount, but not an error).
    auto atStandard = qpayhub.setPromoRate(OPERATOR1, SELLER1, QPAYHUB_FEE_PERMILLE);
    EXPECT_EQ(atStandard.returnCode, QPAYHUB_OK);
}

TEST(ContractQPayhub, RemovePromoRateRevertsToStandardFee)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);

    ASSERT_EQ(qpayhub.setPromoRate(OPERATOR1, SELLER1, 50).returnCode, QPAYHUB_OK);
    ASSERT_EQ(qpayhub.getPromoRate(SELLER1).isPromo, 1);

    auto removeOut = qpayhub.removePromoRate(OPERATOR1, SELLER1);
    EXPECT_EQ(removeOut.returnCode, QPAYHUB_OK);

    auto rate = qpayhub.getPromoRate(SELLER1);
    EXPECT_EQ(rate.feePermille, QPAYHUB_FEE_PERMILLE);
    EXPECT_EQ(rate.isPromo, 0);

    // Removing an already-absent entry is a clean not-found, not a crash.
    auto removeAgain = qpayhub.removePromoRate(OPERATOR1, SELLER1);
    EXPECT_EQ(removeAgain.returnCode, QPAYHUB_ERR_NOT_FOUND);
}

TEST(ContractQPayhub, RemovePromoRateByNonOperatorRejected)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);
    increaseEnergy(IMPOSTOR1, 10000000);
    ASSERT_EQ(qpayhub.setPromoRate(OPERATOR1, SELLER1, 50).returnCode, QPAYHUB_OK);

    auto out = qpayhub.removePromoRate(IMPOSTOR1, SELLER1);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    EXPECT_EQ(qpayhub.getPromoRate(SELLER1).isPromo, 1); // untouched
}

TEST(ContractQPayhub, PromoRateCapacityEnforcedButUpdatesAlwaysAllowed)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);

    // Fill every promo slot with distinct sellers.
    id sellers[QPAYHUB_PROMO_CAPACITY];
    for (uint64 i = 0; i < QPAYHUB_PROMO_CAPACITY; ++i)
    {
        sellers[i] = id::randomValue();
        auto out = qpayhub.setPromoRate(OPERATOR1, sellers[i], 50);
        ASSERT_EQ(out.returnCode, QPAYHUB_OK);
    }
    EXPECT_EQ(qpayhub.state()->promoFeePermille.population(), QPAYHUB_PROMO_CAPACITY);

    // A brand-new seller is rejected once every slot is taken.
    const id overflow = id::randomValue();
    auto overflowOut = qpayhub.setPromoRate(OPERATOR1, overflow, 50);
    EXPECT_EQ(overflowOut.returnCode, QPAYHUB_ERR_CAPACITY);
    EXPECT_EQ(qpayhub.getPromoRate(overflow).isPromo, 0); // rejected, never inserted

    // Updating an EXISTING seller's rate is a free overwrite - it must
    // still succeed even while every slot is already taken.
    auto updateOut = qpayhub.setPromoRate(OPERATOR1, sellers[0], 60);
    EXPECT_EQ(updateOut.returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.getPromoRate(sellers[0]).feePermille, 60ULL);
    EXPECT_EQ(qpayhub.state()->promoFeePermille.population(), QPAYHUB_PROMO_CAPACITY); // unchanged - no new slot consumed

    // Removing one entry frees a slot for a genuinely new seller.
    ASSERT_EQ(qpayhub.removePromoRate(OPERATOR1, sellers[0]).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.state()->promoFeePermille.population(), QPAYHUB_PROMO_CAPACITY - 1);
    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, overflow, 50).returnCode, QPAYHUB_OK);
}

TEST(ContractQPayhub, ChangeOperatorByCurrentOperatorSucceeds)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);
    increaseEnergy(NEWOPERATOR1, 10000000);

    auto out = qpayhub.changeOperator(OPERATOR1, NEWOPERATOR1);
    EXPECT_EQ(out.returnCode, QPAYHUB_OK);

    // The old operator can no longer administer promo rates; the new one can.
    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, SELLER1, 50).returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    EXPECT_EQ(qpayhub.setPromoRate(NEWOPERATOR1, SELLER1, 50).returnCode, QPAYHUB_OK);
}

TEST(ContractQPayhub, ChangeOperatorByRecoveryOverridesCompromisedOperator)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    qpayhub.state()->recoveryId = RECOVERY1;
    increaseEnergy(OPERATOR1, 10000000);
    increaseEnergy(RECOVERY1, 10000000);
    increaseEnergy(NEWOPERATOR1, 10000000);

    // recoveryId can reassign operatorId even without the current operator's
    // cooperation - the emergency-override path this whole mechanism exists for.
    auto out = qpayhub.changeOperator(RECOVERY1, NEWOPERATOR1);
    EXPECT_EQ(out.returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.setPromoRate(NEWOPERATOR1, SELLER1, 50).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, SELLER2, 50).returnCode, QPAYHUB_ERR_ACCESS_DENIED); // old operator locked out
}

TEST(ContractQPayhub, ChangeOperatorByThirdPartyRejected)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    qpayhub.state()->recoveryId = RECOVERY1;
    increaseEnergy(OPERATOR1, 10000000);
    increaseEnergy(IMPOSTOR1, 10000000);

    auto out = qpayhub.changeOperator(IMPOSTOR1, IMPOSTOR1);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    // Neither operator nor recovery changed.
    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, SELLER1, 50).returnCode, QPAYHUB_OK);
}

TEST(ContractQPayhub, ChangeOperatorRejectsNullNewOperator)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->operatorId = OPERATOR1;
    increaseEnergy(OPERATOR1, 10000000);

    auto out = qpayhub.changeOperator(OPERATOR1, NULL_ID);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_INVALID_SELLER);
    EXPECT_EQ(qpayhub.setPromoRate(OPERATOR1, SELLER1, 50).returnCode, QPAYHUB_OK); // operator unchanged
}

TEST(ContractQPayhub, SetAffiliateByRegistrarThenPaySplitsFeeToAffiliate)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);
    increaseEnergy(BUYER1, 10000000);

    ASSERT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
    auto info = qpayhub.getAffiliate(SELLER1);
    EXPECT_EQ(info.affiliate, AFFILIATE1);
    EXPECT_EQ(info.active, 1);

    const sint64 amount = 100000;
    const sint64 affBefore = getBalance(AFFILIATE1);
    const sint64 sellerBefore = getBalance(SELLER1);

    auto payOut = qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, amount);
    ASSERT_EQ(payOut.returnCode, QPAYHUB_OK);

    // fee = 0.75% of 100,000 = 750; affiliate cut = 5% of 750 = 37.
    EXPECT_EQ(getBalance(AFFILIATE1), affBefore + 37);
    // Seller's net is unchanged by the split - the cut comes out of the fee.
    EXPECT_EQ(getBalance(SELLER1), sellerBefore + payOut.net);
}

TEST(ContractQPayhub, SetAffiliateRejectsSelfReferral)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);

    auto out = qpayhub.setAffiliate(REGISTRAR1, SELLER1, SELLER1);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_INVALID_SELLER);
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).active, 0);
}

TEST(ContractQPayhub, SetAffiliateFirstAttributionWins)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);

    ASSERT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
    auto out = qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE2);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_ALREADY_HAS_AFFILIATE);
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).affiliate, AFFILIATE1);
}

TEST(ContractQPayhub, SetAffiliateByThirdPartyRejected)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(IMPOSTOR1, 10000000);

    auto out = qpayhub.setAffiliate(IMPOSTOR1, SELLER1, AFFILIATE1, 1000);
    EXPECT_EQ(out.returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).active, 0);
}

TEST(ContractQPayhub, RemoveAffiliateFreesTheSellerForReattribution)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);

    ASSERT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
    ASSERT_EQ(qpayhub.removeAffiliate(REGISTRAR1, SELLER1).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).active, 0);

    // A fraudulent or mistaken attribution can be corrected, not just deleted.
    EXPECT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE2).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).affiliate, AFFILIATE2);
}

TEST(ContractQPayhub, RemoveAffiliateByRecoveryAlsoAllowed)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    qpayhub.state()->recoveryId = RECOVERY1;
    increaseEnergy(REGISTRAR1, 10000000);
    increaseEnergy(RECOVERY1, 10000000);

    ASSERT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.removeAffiliate(RECOVERY1, SELLER1).returnCode, QPAYHUB_OK);
}

// An affiliate link stops paying out after QPAYHUB_AFFILIATE_TERM_EPOCHS,
// and END_EPOCH actively purges the entry once expired, same purge pattern
// as receipt retention.
TEST(ContractQPayhub, AffiliateCutStopsAndEntryIsPurgedAfterTerm)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);
    increaseEnergy(BUYER1, 10000000);

    system.epoch = 10;
    ASSERT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).active, 1);

    // Still within term: one epoch short of expiry.
    system.epoch = 10 + QPAYHUB_AFFILIATE_TERM_EPOCHS - 1;
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).active, 1);
    const sint64 affBefore = getBalance(AFFILIATE1);
    qpayhub.pay(BUYER1, SELLER1, RESOURCE1, 1, 100000);
    EXPECT_GT(getBalance(AFFILIATE1), affBefore);

    // Past the term: Pay no longer pays the affiliate, even though the
    // entry has not been purged yet.
    system.epoch = 10 + QPAYHUB_AFFILIATE_TERM_EPOCHS;
    EXPECT_EQ(qpayhub.getAffiliate(SELLER1).active, 0);
    const sint64 affBefore2 = getBalance(AFFILIATE1);
    qpayhub.pay(BUYER1, SELLER1, RESOURCE2, 2, 100000);
    EXPECT_EQ(getBalance(AFFILIATE1), affBefore2);

    // END_EPOCH at or past expiry purges the entry and frees the slot.
    qpayhub.endEpoch();
    EXPECT_EQ(qpayhub.state()->affiliateOf.population(), 0ULL);
}

TEST(ContractQPayhub, AffiliateCapacityEnforced)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);

    id sellers[QPAYHUB_AFFILIATE_CAPACITY];
    for (uint64 i = 0; i < QPAYHUB_AFFILIATE_CAPACITY; ++i)
    {
        sellers[i] = id::randomValue();
        auto out = qpayhub.setAffiliate(REGISTRAR1, sellers[i], id::randomValue());
        ASSERT_EQ(out.returnCode, QPAYHUB_OK);
    }

    const id overflow = id::randomValue();
    auto overflowOut = qpayhub.setAffiliate(REGISTRAR1, overflow, id::randomValue());
    EXPECT_EQ(overflowOut.returnCode, QPAYHUB_ERR_CAPACITY);
    EXPECT_EQ(qpayhub.getAffiliate(overflow).active, 0);

    ASSERT_EQ(qpayhub.removeAffiliate(REGISTRAR1, sellers[0]).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.setAffiliate(REGISTRAR1, overflow, id::randomValue()).returnCode, QPAYHUB_OK);
}

TEST(ContractQPayhub, ChangeAffiliateRegistrarByCurrentRegistrarSucceeds)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    increaseEnergy(REGISTRAR1, 10000000);
    increaseEnergy(NEWREGISTRAR1, 10000000);

    EXPECT_EQ(qpayhub.changeAffiliateRegistrar(REGISTRAR1, NEWREGISTRAR1).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_ERR_ACCESS_DENIED);
    EXPECT_EQ(qpayhub.setAffiliate(NEWREGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
}

TEST(ContractQPayhub, ChangeAffiliateRegistrarByRecoveryOverridesCompromisedRegistrar)
{
    ContractTestingQPayhub qpayhub;
    qpayhub.state()->affiliateRegistrarId = REGISTRAR1;
    qpayhub.state()->recoveryId = RECOVERY1;
    increaseEnergy(REGISTRAR1, 10000000);
    increaseEnergy(RECOVERY1, 10000000);
    increaseEnergy(NEWREGISTRAR1, 10000000);

    auto out = qpayhub.changeAffiliateRegistrar(RECOVERY1, NEWREGISTRAR1);
    EXPECT_EQ(out.returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.setAffiliate(NEWREGISTRAR1, SELLER1, AFFILIATE1).returnCode, QPAYHUB_OK);
    EXPECT_EQ(qpayhub.setAffiliate(REGISTRAR1, SELLER2, AFFILIATE2).returnCode, QPAYHUB_ERR_ACCESS_DENIED); // old registrar locked out
}
