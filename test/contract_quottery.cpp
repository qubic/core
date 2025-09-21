#define NO_UEFI

#include "contract_testing.h"

#include <random>
#include <chrono>
#include <ctime>

static void updateEtalonTime(uint64 offsetSecond)
{
    auto now = std::chrono::system_clock::now() + std::chrono::seconds(offsetSecond);
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);
    // init time
    etalonTick.year = localTime->tm_year % 100;
    etalonTick.month = localTime->tm_mon + 1;
    etalonTick.day = localTime->tm_mday;
    etalonTick.hour = localTime->tm_hour;
    etalonTick.minute = localTime->tm_min;
    etalonTick.second = localTime->tm_sec;
}

class ContractTestingQtry : public ContractTesting {
public:
    id owner;
    ContractTestingQtry() {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUOTTERY);
        callSystemProcedure(QUOTTERY_CONTRACT_INDEX, INITIALIZE);
        owner = id(0, 1, 2, 3);
        increaseEnergy(owner, 1000000000ULL);
        beginEpoch();
        updateEtalonTime(0);

        // initialize for gtest
        auto state = getState();
        state->mOperationParams.feePerDay = 11337;
        state->mOperationParams.eligibleCreators.cleanup();
        state->mOperationParams.eligibleOracles.cleanup();
        state->mOperationParams.discountedFeeForUsers.cleanup();
        state->mOperationParams.wholeSharePriceInQU = 100000;
        setMemory(state->mQtryGov, 0);
        state->mQtryGov.mOperationId = owner;
        state->mQtryGov.mBurnFee = 0;
        state->mQtryGov.mOperationFee = 0;
        state->mQtryGov.mShareHolderFee = 0;
    }
    QUOTTERY* getState()
    {
        return (QUOTTERY*)contractStates[QUOTTERY_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUOTTERY_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUOTTERY_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void CreateEvent(const QUOTTERY::CreateEvent_input& cei, const id& caller, const sint64 amount) {
        increaseEnergy(caller, amount + 1); // give caller some coins
        QUOTTERY::CreateEvent_output ceo;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 1, cei, ceo, caller, amount);
    }

    void AddAskOrder(uint64 eid, uint64 amount, uint64 option, uint64 price, const id& caller, sint64 amountQUs = 0)
    {
        QUOTTERY::AddToAskOrder_input ask;
        ask.eventId = eid;
        ask.amount = amount;
        ask.option = option;
        ask.price = price;
        QUOTTERY::AddToAskOrder_output out;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 2, ask, out, caller, amountQUs);
    }

    void RemoveAskOrder(uint64 eid, uint64 amount, uint64 option, uint64 price, const id& caller, sint64 amountQUs = 0)
    {
        QUOTTERY::AddToAskOrder_input ask;
        ask.eventId = eid;
        ask.amount = amount;
        ask.option = option;
        ask.price = price;
        QUOTTERY::AddToAskOrder_output out;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 3, ask, out, caller, amountQUs);
    }

    void AddBidOrder(uint64 eid, uint64 amount, uint64 option, uint64 price, const id& caller)
    {
        QUOTTERY::AddToBidOrder_input bid;
        bid.eventId = eid;
        bid.amount = amount;
        bid.option = option;
        bid.price = price;
        QUOTTERY::AddToBidOrder_output out;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 4, bid, out, caller, amount*price);
    }

    void RemoveBidOrder(uint64 eid, uint64 amount, uint64 option, uint64 price, const id& caller)
    {
        QUOTTERY::AddToBidOrder_input bid;
        bid.eventId = eid;
        bid.amount = amount;
        bid.option = option;
        bid.price = price;
        QUOTTERY::AddToBidOrder_output out;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 5, bid, out, caller, amount * price);
    }

    void UpdateCreator(const id& creator, std::string name, sint64 fee, uint64 ops)
    {
        QUOTTERY::UpdateCreatorList_input ucli;
        QUOTTERY::UpdateCreatorList_output uclo;
        QUOTTERY::CreatorInfo ci;
        ci.feeRate = fee;
        ci.name = id::zero();
        memcpy(ci.name.m256i_i8, name.c_str(), min(name.size(), 32ULL));;
        ucli.creatorId = creator;
        ucli.ci = ci;
        ucli.ops = ops;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 8, ucli, uclo, owner, 0);
    }

    void UpdateOracle(const id& oracle, std::string name, sint64 fee, uint64 ops)
    {
        QUOTTERY::UpdateOracleList_input uoli;
        QUOTTERY::UpdateOracleList_output uolo;
        QUOTTERY::OracleInfo ci;
        ci.feeRate = fee;
        ci.name = id::zero();
        memcpy(ci.name.m256i_i8, name.c_str(), min(name.size(), 32ULL));;
        uoli.oracleId = oracle;
        uoli.oi = ci;
        uoli.ops = ops;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 9, uoli, uolo, owner, 0);
    }

    void UpdateFeePerDay(const id caller, const uint64 newFee)
    {
        QUOTTERY::UpdateFeePerDay_input ufi;
        QUOTTERY::UpdateFeePerDay_output ufo;
        ufi.newFee = newFee;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 11, ufi, ufo, caller, 0);
    }

    void UpdateFeeDiscountList(const id uid, const uint64 newFee, uint64 op)
    {
        QUOTTERY::UpdateFeeDiscountList_input ufi;
        QUOTTERY::UpdateFeeDiscountList_output ufo;
        ufi.newFeeRate = newFee;
        ufi.ops = op;
        ufi.userId = uid;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 10, ufi, ufo, owner, 0);
    }

    void PublishResult(const uint64 eid, const uint64 option, const id caller)
    {
        QUOTTERY::PublishResult_input pri;
        QUOTTERY::PublishResult_output pro;
        pri.eventId = eid;
        pri.option = option;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 6, pri, pro, caller, 0);
    }

    void ResolveEvent(const uint64 eid, const uint64 option)
    {
        QUOTTERY::ResolveEvent_input rei;
        QUOTTERY::ResolveEvent_output reo;
        rei.eventId = eid;
        rei.option = option;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 7, rei, reo, owner, 0);
    }
};
//uint64 eid;
//DateAndTime openDate; // submitted date
//DateAndTime closeDate; // close trading date and OPs start broadcasting result
//DateAndTime endDate; // stop receiving result from OPs
//
//id creator;
///*256 chars to describe*/
//Array<id, 4> desc;
///*128 chars to describe option */
//Array<id, 2> option0Desc;
//Array<id, 2> option1Desc;
///*8 oracle IDs*/
//Array<id, 8> oracleId;
//
//// payment info
//uint64 type; // 0: QUs - 1: token (must be managed by QX)
//id issuer;
//uint64 assetName;

static QPI::DateAndTime wrapped_now()
{
    QPI::DateAndTime result;
    result.year = etalonTick.year;
    result.month = etalonTick.month;
    result.day = etalonTick.day;
    result.hour = etalonTick.hour;
    result.minute = etalonTick.minute;
    result.second = etalonTick.second;
    result.millisecond = etalonTick.millisecond;
    return result;
}

TEST(QTRYTest, MakeCreatorAndOracleList)
{
    ContractTestingQtry qtry;
    std::vector<id> creators(QUOTTERY_MAX_CREATOR_AND_COUNT+1);
    std::vector<id> oracles(QUOTTERY_MAX_CREATOR_AND_COUNT+1);
    // test #1: overload the array
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT+1; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
    }
    uint64 ops = 1; //add
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT + 1; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator0", 10+(i%2), ops);
        qtry.UpdateOracle(oracles[i], "TestOracle0", 10+(i%3), ops);
    }
    auto state = qtry.getState();
    id c_name = id::zero();
    id o_name = id::zero();
    memcpy(c_name.m256i_i8, "TestCreator0", 12);
    memcpy(o_name.m256i_i8, "TestOracle0", 11);
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT + 1; i++)
    {
        if (i < QUOTTERY_MAX_CREATOR_AND_COUNT)
        {
            EXPECT_TRUE(state->mOperationParams.eligibleCreators.contains(creators[i]));
            EXPECT_TRUE(state->mOperationParams.eligibleOracles.contains(oracles[i]));
            auto index = state->mOperationParams.eligibleCreators.getElementIndex(creators[i]);
            auto ci = state->mOperationParams.eligibleCreators.value(index);

            index = state->mOperationParams.eligibleOracles.getElementIndex(oracles[i]);
            auto oi = state->mOperationParams.eligibleOracles.value(index);

            EXPECT_TRUE(ci.feeRate == 10 + (i%2));
            EXPECT_TRUE(oi.feeRate == 10 + (i%3));

            EXPECT_TRUE(c_name == ci.name);
            EXPECT_TRUE(o_name == oi.name);
        }
        else
        {
            EXPECT_FALSE(state->mOperationParams.eligibleCreators.contains(creators[i]));
            EXPECT_FALSE(state->mOperationParams.eligibleOracles.contains(oracles[i]));
        }
    }

    // test #2: remove half
    ops = 0; //remove
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT/2; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator888", 10 + i%2, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle999", 10 + i%3, ops);
    }
    for (int i = QUOTTERY_MAX_CREATOR_AND_COUNT/2; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        EXPECT_TRUE(state->mOperationParams.eligibleCreators.contains(creators[i]));
        EXPECT_TRUE(state->mOperationParams.eligibleOracles.contains(oracles[i]));
        auto index = state->mOperationParams.eligibleCreators.getElementIndex(creators[i]);
        auto ci = state->mOperationParams.eligibleCreators.value(index);

        index = state->mOperationParams.eligibleOracles.getElementIndex(oracles[i]);
        auto oi = state->mOperationParams.eligibleOracles.value(index);

        EXPECT_TRUE(ci.feeRate == 10 + (i%2));
        EXPECT_TRUE(oi.feeRate == 10 + (i%3));

        EXPECT_TRUE(c_name == ci.name);
        EXPECT_TRUE(o_name == oi.name);
    }
    // test #3: update
    ops = 2; //update
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT / 2; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator111", 6 + i%2, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle222", 6+i%3, ops);
    }
    for (int i = QUOTTERY_MAX_CREATOR_AND_COUNT / 2; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator888", 6 + i%2, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle999", 6 + i%3, ops);
    }
    memcpy(c_name.m256i_i8, "TestCreator888", 14);
    memcpy(o_name.m256i_i8, "TestOracle999", 13);
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        if (i < QUOTTERY_MAX_CREATOR_AND_COUNT / 2)
        {
            EXPECT_FALSE(state->mOperationParams.eligibleCreators.contains(creators[i]));
            EXPECT_FALSE(state->mOperationParams.eligibleOracles.contains(oracles[i]));
        }
        else
        {
            EXPECT_TRUE(state->mOperationParams.eligibleCreators.contains(creators[i]));
            EXPECT_TRUE(state->mOperationParams.eligibleOracles.contains(oracles[i]));
            auto index = state->mOperationParams.eligibleCreators.getElementIndex(creators[i]);
            auto ci = state->mOperationParams.eligibleCreators.value(index);

            index = state->mOperationParams.eligibleOracles.getElementIndex(oracles[i]);
            auto oi = state->mOperationParams.eligibleOracles.value(index);

            EXPECT_TRUE(ci.feeRate == 6 + i%2);
            EXPECT_TRUE(oi.feeRate == 6 + i%3);

            EXPECT_TRUE(c_name == ci.name);
            EXPECT_TRUE(o_name == oi.name);
        }
    }
    
    // test #4: malformed update
    for (int i = QUOTTERY_MAX_CREATOR_AND_COUNT / 2; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator666", QUOTTERY_HARD_CAP_CREATOR_FEE + 1, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle666", QUOTTERY_HARD_CAP_CREATOR_FEE + 1, ops);
    }
    for (int i = QUOTTERY_MAX_CREATOR_AND_COUNT / 2; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        EXPECT_TRUE(state->mOperationParams.eligibleCreators.contains(creators[i]));
        EXPECT_TRUE(state->mOperationParams.eligibleOracles.contains(oracles[i]));
        auto index = state->mOperationParams.eligibleCreators.getElementIndex(creators[i]);
        auto ci = state->mOperationParams.eligibleCreators.value(index);

        index = state->mOperationParams.eligibleOracles.getElementIndex(oracles[i]);
        auto oi = state->mOperationParams.eligibleOracles.value(index);

        EXPECT_FALSE(ci.feeRate == QUOTTERY_HARD_CAP_CREATOR_FEE + 1);
        EXPECT_FALSE(oi.feeRate == QUOTTERY_HARD_CAP_CREATOR_FEE + 1);
    }
}

TEST(QTRYTest, CreateEvent)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    std::vector<id> creators(8);
    std::vector<id> oracles(8);
    // test #1: overload the array
    for (int i = 0; i < 8; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
    }
    uint64 ops = 1; //add
    for (int i = 0; i < 8; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator" + std::to_string(i), 6 + i, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle" + std::to_string(i), 6 + i * 2, ops);
    }
    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);

    
    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_close = dt + 3600000ULL; // + 1 hour
    DateAndTime dt_end = dt + 3600000ULL * 2; // + 1 hour

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt_close;
    cei.qei.endDate = dt_end;
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.assetIssuer = id::zero();
    cei.qei.assetName = 0;
    auto state = qtry.getState();
    qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1);
    QUOTTERY::QtryEventInfo onchain_qei;
    EXPECT_TRUE(state->mEventInfo.get(0, onchain_qei));
    EXPECT_TRUE(onchain_qei.creator == creators[0]);
    EXPECT_TRUE(onchain_qei.openDate == dt);
    EXPECT_TRUE(onchain_qei.closeDate == dt_close);
    EXPECT_TRUE(onchain_qei.endDate == dt_end);
    for (int i = 0; i < 4; i++)
    {
        id a = onchain_qei.desc.get(i);
        EXPECT_TRUE(a == v_desc[i]);
    }

    // skew time
    memset(&cei.qei, 0, sizeof(cei.qei));
    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);
    cei.qei.eid = -1;
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt + (-1000000);
    cei.qei.endDate = dt + (-2000000);
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.assetIssuer = id::zero();
    cei.qei.assetName = 0;
    qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase

    // invalid creator
    qtry.CreateEvent(cei, id::randomValue(), state->mOperationParams.feePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase

    // invalid oracles
    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, id::randomValue());
    qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase

    // lack of fund
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt_close;
    cei.qei.endDate = dt_end;
    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);
    qtry.CreateEvent(cei, creators[0], 0);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase
}

TEST(QTRYTest, MatchingOrders)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    std::vector<id> creators(8);
    std::vector<id> oracles(8);
    // test #1: overload the array
    for (int i = 0; i < 8; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
    }
    auto state = qtry.getState();
    uint64 ops = 1; //add
    for (int i = 0; i < 8; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator" + std::to_string(i), 0, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle" + std::to_string(i), 0, ops);
    }
    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);
    


    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_close = dt + 3600000ULL; // + 1 hour
    DateAndTime dt_end = dt + 3600000ULL * 2; // + 1 hour

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt_close;
    cei.qei.endDate = dt_end;
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.assetIssuer = id::zero();
    cei.qei.assetName = 0;
    
    qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    
    id traders[16];
    for (int i = 0; i < 16; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 100LL * 1000000000LL);
    }
    // bid: 100 shares option 0 for 40000
    qtry.AddBidOrder(0, 100, 0, 40000ULL, traders[0]);
    // bid: 100 shares option 1 for 61000
    qtry.AddBidOrder(0, 100, 1, 61000ULL, traders[1]);
    // expected: mint order, matching 40k-60k 
    id key = QUOTTERY::MakePosKey(traders[0], 0, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);
    EXPECT_TRUE(qo.entity == traders[0]);

    key = QUOTTERY::MakePosKey(traders[1], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);
    EXPECT_TRUE(qo.entity == traders[1]);

    // b0 ask 50 shares opt0 for 70
    // b1 ask 40 shares opt1 for 40
    // expected nothing happens
    qtry.AddAskOrder(0, 50, 0, 70000, traders[0]);
    qtry.AddAskOrder(0, 40, 1, 40000, traders[1]);

    key = QUOTTERY::MakePosKey(traders[0], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);
    EXPECT_TRUE(qo.entity == traders[0]);

    key = QUOTTERY::MakePosKey(traders[1], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);
    EXPECT_TRUE(qo.entity == traders[1]);

    // b1 remove 40 shares opt1 for 40
    // b1 add 40 shares opt1 for 20k
    // expected: merge happens
    // b0 balance: 100B - 100*40000 + 40*70000
    // b1 balance: 100B - 100*60000 + 40*30000
    qtry.RemoveAskOrder(0, 40, 1, 40000, traders[1]);
    qtry.AddAskOrder(0, 40, 1, 20000, traders[1]);
    sint64 b0_bal = getBalance(traders[0]);
    sint64 b1_bal = getBalance(traders[1]);
    EXPECT_TRUE(b0_bal == (100000000000ULL - 100LL * 40000 + 40ULL * 70000));
    EXPECT_TRUE(b1_bal == (100000000000ULL - 100LL * 60000 + 40ULL * 30000));

    // b0 has 60 shares opt0, 10 is in order
    // b1 has 60 shares opt1, 0 is in order
    key = QUOTTERY::MakePosKey(traders[0], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 60);
    EXPECT_TRUE(qo.entity == traders[0]);

    key = QUOTTERY::MakePosKey(traders[1], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 60);
    EXPECT_TRUE(qo.entity == traders[1]);

    key = QUOTTERY::MakeOrderKey(0, 0, 0);
    auto index = state->mABOrders.headIndex(key, 0);
    EXPECT_TRUE(index != NULL_INDEX);

    auto elem = state->mABOrders.element(index);
    EXPECT_TRUE(elem.amount == 10); // 10 left
    EXPECT_TRUE(elem.entity == traders[0]);

    // bid: 100 shares option 0 for 20000
    qtry.AddBidOrder(0, 103, 0, 20000ULL, traders[2]);
    // bid: 100 shares option 1 for 80000
    qtry.AddBidOrder(0, 103, 1, 80000ULL, traders[3]);

    key = QUOTTERY::MakePosKey(traders[2], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 103);
    EXPECT_TRUE(qo.entity == traders[2]);

    key = QUOTTERY::MakePosKey(traders[3], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 103);
    EXPECT_TRUE(qo.entity == traders[3]);

    //normal matching order, T0 ask 10 option 0 at 40k
    // T4 bid 10 option 0 at 41k => match at 40k
    qtry.AddBidOrder(0, 10, 0, 71000ULL, traders[4]);
    sint64 b4_bal = getBalance(traders[4]);
    EXPECT_TRUE(b4_bal == (100000000000ULL - 10 * 70000));
    key = QUOTTERY::MakePosKey(traders[4], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 10);
    EXPECT_TRUE(qo.entity == traders[4]);

    //T0 has 60 share opt0
    //T1 has 60 share opt1
    //T2 has 103 shares opt0
    //T3 has 103 shares opt1
    //T4 has 10 shares opt0
    qtry.AddAskOrder(0, 50, 0, 20000, traders[0]);
    qtry.AddAskOrder(0, 50, 0, 30000, traders[2]);
    qtry.AddAskOrder(0, 53, 0, 40000, traders[2]);
    qtry.AddAskOrder(0, 10, 0, 50000, traders[4]);
    qtry.AddBidOrder(0, 163, 0, 50000, traders[5]);
    sint64 b5_bal = getBalance(traders[5]);
    EXPECT_TRUE(b5_bal == (100000000000ULL - 50 * 20000 - 50 * 30000 - 53 * 40000 - 10 * 50000));
    key = QUOTTERY::MakePosKey(traders[5], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 163);
    EXPECT_TRUE(qo.entity == traders[5]);

    qtry.AddBidOrder(0, 50, 0, 20000, traders[0]);
    qtry.AddBidOrder(0, 50, 0, 30000, traders[2]);
    qtry.AddBidOrder(0, 53, 0, 40000, traders[2]);
    qtry.AddBidOrder(0, 10, 0, 50000, traders[4]);
    qtry.AddAskOrder(0, 163, 0, 20000, traders[5]);
    b5_bal = getBalance(traders[5]);
    EXPECT_TRUE(b5_bal == (100000000000ULL));

    qtry.CreateEvent(cei, creators[1], state->mOperationParams.feePerDay);
    qtry.AddBidOrder(1, 103, 0, 20000ULL, traders[0]);
    qtry.AddBidOrder(1, 103, 1, 80000ULL, traders[1]);
    qtry.AddBidOrder(1, 203, 0, 20000ULL, traders[2]);
    qtry.AddBidOrder(1, 203, 1, 80000ULL, traders[3]);
    qtry.AddBidOrder(1, 303, 0, 20000ULL, traders[4]);
    qtry.AddBidOrder(1, 303, 1, 80000ULL, traders[5]);
    qtry.AddBidOrder(1, 403, 0, 20000ULL, traders[6]);
    qtry.AddBidOrder(1, 403, 1, 80000ULL, traders[7]);

    qtry.AddAskOrder(1, 50, 0, 20000, traders[0]);
    qtry.AddAskOrder(1, 50, 0, 30000, traders[2]);
    qtry.AddAskOrder(1, 53, 0, 40000, traders[2]);
    qtry.AddAskOrder(1, 106, 0, 50000, traders[4]);
    qtry.AddAskOrder(1, 206, 0, 50000, traders[6]);
    qtry.AddBidOrder(1, 50 + 50 + 53 + 106 + 206, 0, 50000, traders[8]);
    auto b8_bal = getBalance(traders[8]);
    EXPECT_TRUE(b8_bal == (100000000000ULL - 50*20000 - 50 * 30000 - 53 * 40000 - 106 * 50000 - 206 * 50000));

    qtry.AddBidOrder(1, 50, 0, 20000, traders[1]);
    qtry.AddBidOrder(1, 50, 0, 30000, traders[3]);
    qtry.AddBidOrder(1, 53, 0, 40000, traders[5]);
    qtry.AddBidOrder(1, 10, 0, 50000, traders[7]);
    auto b8_before = b8_bal;
    qtry.AddAskOrder(1, 50 + 50 + 53 + 10, 0, 20000, traders[8]);
    b8_bal = getBalance(traders[8]);
    EXPECT_TRUE(b8_bal == (b8_before + 50 * 20000 + 50 *30000 + 53 * 40000 + 10 * 50000));

    b8_before = b8_bal;
    qtry.AddAskOrder(1, 1, 0, 20000, traders[5], 1000);
    qtry.AddAskOrder(1, 2, 0, 20000, traders[7], 1000);
    qtry.AddAskOrder(1, 1, 0, 20000, traders[8], 1000);
    b8_bal = getBalance(traders[8]);
    EXPECT_TRUE(b8_before == b8_bal); // expect refund
    qtry.RemoveAskOrder(1, 1, 0, 20000, traders[8], 1000);
    qtry.RemoveAskOrder(10, 1, 0, 20000, traders[8], 1000);
    qtry.RemoveAskOrder(1, 1999999, 0, 20000, traders[8], 1000);
    qtry.RemoveAskOrder(1, 1, 0, 20000, traders[7], 1000);

    // send invalid ask order
    qtry.AddAskOrder(99999, 1, 0, 20000, traders[8], 1000);
    // send invalid position
    qtry.AddAskOrder(1, 1000, 0, 20000, traders[9], 1000);
    // increasing order - (same price)
    qtry.AddAskOrder(1, 5, 0, 50001, traders[8], 1000);
    qtry.AddAskOrder(1, 5, 0, 50001, traders[8], 1000);
    key = QUOTTERY::MakeOrderKey(1, 0, ASK_BIT);
    index = state->mABOrders.headIndex(key, -50001);
    EXPECT_TRUE(index != NULL_INDEX);
    auto value = state->mABOrders.element(index);
    EXPECT_TRUE(value.amount == 10);
    EXPECT_TRUE(value.entity == traders[8]);
    b8_bal = getBalance(traders[8]);
    b8_before = b8_bal;
    qtry.RemoveAskOrder(1, 10, 0, 50001, traders[8]);
    qtry.AddAskOrder(1, 3, 1, 90000, traders[1]);
    qtry.AddAskOrder(1, 3, 1, 90000, traders[1]);
    qtry.AddAskOrder(1, 3, 1, 90001, traders[1]);
    qtry.AddAskOrder(1, 4, 1, 90001, traders[3]);
    qtry.AddAskOrder(1, 5, 1, 90003, traders[5]);
    qtry.AddBidOrder(1, 9 + 4 + 5, 1, 90003, traders[8]);
    b8_bal = getBalance(traders[8]);
    EXPECT_TRUE(b8_bal == b8_before - 6 * 90000 - 7 * 90001 - 5 * 90003);

    key = QUOTTERY::MakePosKey(traders[8], 1, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 18);
    EXPECT_TRUE(qo.entity == traders[8]);

    qtry.CreateEvent(cei, creators[2], state->mOperationParams.feePerDay);
    qtry.AddBidOrder(2, 103, 1, 20000ULL, traders[0]);
    qtry.AddBidOrder(2, 103, 0, 80000ULL, traders[1]);
    qtry.AddBidOrder(2, 203, 1, 20000ULL, traders[2]);
    qtry.AddBidOrder(2, 203, 0, 80000ULL, traders[3]);
    qtry.AddBidOrder(2, 303, 1, 20000ULL, traders[4]);
    qtry.AddBidOrder(2, 303, 0, 80000ULL, traders[5]);
    qtry.AddBidOrder(2, 403, 1, 20000ULL, traders[6]);
    qtry.AddBidOrder(2, 403, 0, 80000ULL, traders[7]);

    qtry.AddBidOrder(2, 16, 0, 80000ULL, traders[9]);
    qtry.AddBidOrder(2, 16, 0, 80000ULL, traders[8]);

    key = QUOTTERY::MakeOrderKey(2, 0, BID_BIT);
    index = state->mABOrders.headIndex(key, 80000ULL);
    EXPECT_TRUE(index != NULL_INDEX);
    elem = state->mABOrders.element(index);
    EXPECT_TRUE(elem.amount == 16);
    EXPECT_TRUE(elem.entity == traders[8] || elem.entity == traders[9]);

    qtry.RemoveBidOrder(2, 7, 0, 80000, traders[8]);
    qtry.RemoveBidOrder(2, 7, 0, 80000, traders[8]);
    qtry.RemoveBidOrder(2, 2, 0, 90000, traders[8]);
    qtry.RemoveBidOrder(2, 2, 0, 80000, traders[8]);
    qtry.RemoveBidOrder(2, 16, 0, 80000, traders[9]);
    
    index = state->mABOrders.headIndex(key, 80000ULL);
    EXPECT_TRUE(index == NULL_INDEX);

    qtry.AddBidOrder(2, 16, 0, 80000ULL, traders[8]);
    qtry.AddBidOrder(2, 16, 0, 80000ULL, traders[9]);
    qtry.AddBidOrder(2, 16, 0, 80000ULL, traders[10]);
    qtry.AddBidOrder(2, 8, 0, 80000ULL, traders[8]);

    key = QUOTTERY::MakeOrderKey(2, 0, BID_BIT);
    index = state->mABOrders.headIndex(key, 80000ULL);
    EXPECT_TRUE(index != NULL_INDEX);
    qtry.RemoveBidOrder(2, 16, 0, 80000, traders[9]);
    qtry.RemoveBidOrder(2, 16, 0, 80000, traders[10]);
    index = state->mABOrders.headIndex(key, 80000ULL);
    EXPECT_TRUE(index != NULL_INDEX);
    elem = state->mABOrders.element(index);
    EXPECT_TRUE(elem.amount == 24);
    EXPECT_TRUE(elem.entity == traders[8]);

    qtry.AddBidOrder(2, 10, 0, 10000, traders[1]);
    auto prev_b1_bal = getBalance(traders[1]);
    qtry.RemoveBidOrder(2, 10, 0, 10000, traders[1]);
    b1_bal = getBalance(traders[1]);
    EXPECT_TRUE(b1_bal - prev_b1_bal == 100000);
}

TEST(QTRYTest, CompleteCycle)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    std::vector<id> creators(8);
    std::vector<id> oracles(8);
    // test #1: overload the array
    for (int i = 0; i < 8; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
        increaseEnergy(creators[i], 100LL * 1000000000LL);
        increaseEnergy(oracles[i], 100LL * 1000000000LL);
    }
    auto state = qtry.getState();
    uint64 ops = 1; //add
    for (int i = 0; i < 8; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator" + std::to_string(i), 0, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle" + std::to_string(i), 0, ops);
    }
    id traders[16];
    for (int i = 0; i < 16; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 100LL * 1000000000LL);
    }

    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);

    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_close = dt + 3600000ULL; // + 1 hour
    DateAndTime dt_end = dt + 3600000ULL * 2; // + 2 hour

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt_close;
    cei.qei.endDate = dt_end;
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.assetIssuer = id::zero();
    cei.qei.assetName = 0;

    qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    srand(0);

    id qtryID(QUOTTERY_CONTRACT_INDEX, 0, 0, 0);
    sint64 qtryBal = 0;
    sint64 total = 0;
    for (int i = 0; i < 16; i++)
    {
        total += getBalance(traders[i]);
    }

    for (int i = 0; i < 10000; i++)
    {
        qtry.AddBidOrder(0, 1 + rand() % 1000, rand() % 2, 50000 + (rand()%100), traders[rand() % 16]);
    }

    for (int i = 0; i < 100000; i++) // 100k random operations
    {
        int x = rand();
        if (x % 2)
        {
            qtry.AddBidOrder(0, 1 + rand()% 1000, rand() % 2, 50000 + rand() % (1000 - 1), traders[rand() % 16]);
        }
        else
        {
            qtry.AddAskOrder(0, 1 + rand() % 1000, rand() % 2, 50000 + rand() % (1000 - 1), traders[rand() % 16]);
        }
    }
    for (int i = 0; i < 1000; i++)
    {
        qtry.AddBidOrder(0, 1 + rand() % 100, 0, 10000 + rand() % (1000 - 1), traders[rand() % 16]);
        qtry.AddBidOrder(0, 1 + rand() % 100, 1, 10000 + rand() % (1000 - 1), traders[rand() % 16]);
    }
    
    auto key = QUOTTERY::MakeOrderKey(0, 0, 0);
    auto index = state->mABOrders.headIndex(key, 0);
    int count = 0;
    
    while (index != NULL_INDEX)
    {
        auto elem = state->mABOrders.element(index);
        EXPECT_TRUE(elem.amount > 0);
        EXPECT_TRUE(elem.entity != NULL_ID);
        count++;
        index = state->mABOrders.nextElementIndex(index);
    }
    key = QUOTTERY::MakeOrderKey(0, 0, 1);
    index = state->mABOrders.headIndex(key, 0);
    while (index != NULL_INDEX)
    {
        auto elem = state->mABOrders.element(index);
        EXPECT_TRUE(elem.amount > 0);
        EXPECT_TRUE(elem.entity != NULL_ID);
        count++;
        index = state->mABOrders.nextElementIndex(index);
    }
    EXPECT_TRUE(count > 0);

    updateEtalonTime(3600 * 3);
    // result 0 win
    for (int i = 0; i < 8; i++)
    {
        qtry.PublishResult(0, 0, oracles[i]);
    }
    // empty mABOrder
    key = QUOTTERY::MakeOrderKey(0, 0, 0);
    index = state->mABOrders.headIndex(key, 0);
    EXPECT_TRUE(index == NULL_INDEX);
    key = QUOTTERY::MakeOrderKey(0, 0, 1);
    index = state->mABOrders.headIndex(key, 0);
    EXPECT_TRUE(index == NULL_INDEX);

    qtryBal = getBalance(qtryID);
    total = 0;
    for (int i = 0; i < 16; i++)
    {
        total += getBalance(traders[i]);
    }
    EXPECT_TRUE(total == 16 * 1000000000LL * 100LL);
}

TEST(QTRYTest, OperationFunction)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto prevFee = state->mOperationParams.feePerDay;
    qtry.UpdateFeePerDay(id(2, 3, 4, 5), prevFee + 13);
    EXPECT_TRUE(prevFee == state->mOperationParams.feePerDay);
    qtry.UpdateFeePerDay(qtry.owner, prevFee + 13);
    EXPECT_TRUE(prevFee + 13 == state->mOperationParams.feePerDay);

    
    std::vector<id> creators(QUOTTERY_MAX_CREATOR_AND_COUNT + 1);
    std::vector<id> oracles(QUOTTERY_MAX_CREATOR_AND_COUNT + 1);
    std::vector<id> traders(QUOTTERY_MAX_CREATOR_AND_COUNT + 1);    
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT + 1; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
        traders[i] = id::randomValue();
    }
    // test updating discount fee array
    // remove non exist records
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        qtry.UpdateFeeDiscountList(traders[i], 0, 0);
    }
    // add
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        qtry.UpdateFeeDiscountList(traders[i], 9+i%2, 1);
    }
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        uint64 value;
        EXPECT_TRUE(state->mOperationParams.discountedFeeForUsers.get(traders[i], value));
        EXPECT_TRUE(value == 9 + i%2);
    }
    // remove again
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        qtry.UpdateFeeDiscountList(traders[i], 0, 0);
    }
    for (int i = 0; i < QUOTTERY_MAX_CREATOR_AND_COUNT; i++)
    {
        EXPECT_FALSE(state->mOperationParams.discountedFeeForUsers.contains(traders[i]));
    }
}
TEST(QTRYTest, CreateEventOverflow)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    std::vector<id> creators(8);
    std::vector<id> oracles(8);
    // test #1: overload the array
    for (int i = 0; i < 8; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
        increaseEnergy(creators[i], 100LL * 1000000000LL);
        increaseEnergy(oracles[i], 100LL * 1000000000LL);
    }
    auto state = qtry.getState();
    uint64 ops = 1; //add
    for (int i = 0; i < 8; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator" + std::to_string(i), 0, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle" + std::to_string(i), 0, ops);
    }
    id traders[16];
    for (int i = 0; i < 16; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 100LL * 1000000000LL);
    }

    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);

    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_close = dt + 3600000ULL; // + 1 hour
    DateAndTime dt_end = dt + 3600000ULL * 2; // + 2 hour

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt_close;
    cei.qei.endDate = dt_end;
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.assetIssuer = id::zero();
    cei.qei.assetName = 0;

    for (int i = 0; i < QUOTTERY_MAX_EVENT * 2; i++)
    {
        qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    }
    EXPECT_TRUE(state->mCurrentEventID == QUOTTERY_MAX_EVENT);
    EXPECT_TRUE(state->mEventInfo.population() == QUOTTERY_MAX_EVENT);
}

TEST(QTRYTest, ResolveEvent)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    std::vector<id> creators(8);
    std::vector<id> oracles(8);
    // test #1: overload the array
    for (int i = 0; i < 8; i++)
    {
        creators[i] = id::randomValue();
        oracles[i] = id::randomValue();
        increaseEnergy(creators[i], 100LL * 1000000000LL);
        increaseEnergy(oracles[i], 100LL * 1000000000LL);
    }
    auto state = qtry.getState();
    uint64 ops = 1; //add
    for (int i = 0; i < 8; i++)
    {
        qtry.UpdateCreator(creators[i], "TestCreator" + std::to_string(i), 0, ops);
        qtry.UpdateOracle(oracles[i], "TestOracle" + std::to_string(i), 0, ops);
    }
    id traders[16];
    for (int i = 0; i < 16; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 100LL * 1000000000LL);
    }

    for (int i = 0; i < 8; i++) cei.qei.oracleId.set(i, oracles[i]);



    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_close = dt + 3600000ULL; // + 1 hour
    DateAndTime dt_end = dt + 3600000ULL * 2; // + 2 hour

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;
    cei.qei.openDate = dt + (-1000000);
    cei.qei.closeDate = dt_close;
    cei.qei.endDate = dt_end;
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.assetIssuer = id::zero();
    cei.qei.assetName = 0;

    qtry.CreateEvent(cei, creators[0], state->mOperationParams.feePerDay);
    srand(0);

    id qtryID(QUOTTERY_CONTRACT_INDEX, 0, 0, 0);
    sint64 qtryBal = 0;
    sint64 total = 0;

    for (int i = 0; i < 10000; i++)
    {
        qtry.AddBidOrder(0, 1 + rand() % 1000, rand() % 2, 50000 + (rand() % 100), traders[rand() % 16]);
    }

    for (int i = 0; i < 100000; i++) // 100k random operations
    {
        int x = rand();
        if (x % 2)
        {
            qtry.AddBidOrder(0, 1 + rand() % 1000, rand() % 2, 50000 + rand() % (1000 - 1), traders[rand() % 16]);
        }
        else
        {
            qtry.AddAskOrder(0, 1 + rand() % 1000, rand() % 2, 50000 + rand() % (1000 - 1), traders[rand() % 16]);
        }
    }

    updateEtalonTime(3600 * 30);
    // result 0 win
    qtry.ResolveEvent(0, 0);

    qtryBal = getBalance(qtryID);
    total = 0;
    for (int i = 0; i < 16; i++)
    {
        total += getBalance(traders[i]);
    }
    EXPECT_TRUE(total == 16 * 1000000000LL * 100LL);
}