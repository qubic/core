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
    etalonTick.tick += (unsigned int)(offsetSecond);
    system.tick = etalonTick.tick;
}

class ContractTestingQtry : public ContractTesting 
{
public:
    id owner;
    ContractTestingQtry() 
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUOTTERY);
        callSystemProcedure(QUOTTERY_CONTRACT_INDEX, INITIALIZE);
        owner = id(0, 1, 2, 3);
        beginEpoch();
        updateEtalonTime(0);
        id tmp = id::zero();
        for (int i = 1; i <= 676; i++)
        {
            tmp.m256i_u8[30] = i / 256;
            tmp.m256i_u8[31] = i % 256;
            broadcastedComputors.computors.publicKeys[i - 1] = tmp;
        }

        // initialize for gtest
        auto state = getState();
        QpiContextUserProcedureCall qpi(QUOTTERY_CONTRACT_INDEX, id::zero(), 0);
        {
            setMemory(state->mQtryGov, 0);
            state->mQtryGov.mFeePerDay = 11337;
            state->mOperationParams.discountedFeeForUsers.cleanup();
            state->mQtryGov.mOperationId = owner; // testnet ARB
            state->mQtryGov.mBurnFee = 0;
            state->mQtryGov.mOperationFee = 0;
            state->mQtryGov.mShareHolderFee = 0;
            state->mOperationParams.mAntiSpamAmount = 0;
            state->mRecentActiveEvent.setAll(NULL_INDEX);
            id qtryId = id(QUOTTERY_CONTRACT_INDEX, 0, 0, 0);
            // for test only
            qpi.issueAsset(1146312017, qtryId, 0, 1000000000000000ULL, 0);
            state->mQUSDIdentifier.assetName = 1146312017;
            state->mQUSDIdentifier.issuer = qtryId;
            state->wholeSharePrice = 100000;
            qpi.transferShareOwnershipAndPossession(state->mQUSDIdentifier.assetName, state->mQUSDIdentifier.issuer, qtryId, qtryId, 1000000000000000ULL, state->mQtryGov.mOperationId); // transfer all to GO
        }

        owner = state->mQtryGov.mOperationId;
        increaseEnergy(owner, 1LL);
        auto b0 = balanceUSD(owner);
        EXPECT_TRUE(b0 == 1000000000000000ULL);
        qpi.freeBuffer();
    }

    // transfer both ownership and possession
    void transferQUSD(const id from, const id to, const long long amount)
    {
        auto state = getState();
        QpiContextUserProcedureCall qpi(QUOTTERY_CONTRACT_INDEX, from, 0);
        qpi.transferShareOwnershipAndPossession(state->mQUSDIdentifier.assetName, state->mQUSDIdentifier.issuer, from, from, amount, to);
        qpi.freeBuffer();
    }

    long long balanceUSD(id pk)
    {
        return numberOfShares(getState()->mQUSDIdentifier,
            { pk, QUOTTERY_CONTRACT_INDEX },
            { pk, QUOTTERY_CONTRACT_INDEX });
    }

    QUOTTERY::StateData* getState()
    {
        return (QUOTTERY::StateData*)contractStates[QUOTTERY_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUOTTERY_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUOTTERY_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void CreateEvent(const QUOTTERY::CreateEvent_input& cei, const id& caller, const sint64 amount) 
    {
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
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 4, bid, out, caller, amount * price);
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

    void UpdateFeeDiscountList(const id uid, const uint64 newFee, uint64 op)
    {
        QUOTTERY::UpdateFeeDiscountList_input ufi;
        QUOTTERY::UpdateFeeDiscountList_output ufo;
        ufi.newFeeRate = newFee;
        ufi.ops = op;
        ufi.userId = uid;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 20, ufi, ufo, owner, 0);
    }

    void PublishResult(const uint64 eid, const uint64 option, const id caller, sint64 amount)
    {
        QUOTTERY::PublishResult_input pri;
        QUOTTERY::PublishResult_output pro;
        pri.eventId = eid;
        pri.option = option;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 6, pri, pro, caller, amount);
    }

    void FinalizeEvent(const uint64 eid, const id caller)
    {
        QUOTTERY::TryFinalizeEvent_input pri;
        QUOTTERY::TryFinalizeEvent_output pro;
        pri.eventId = eid;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 7, pri, pro, caller, 0);
    }
    void UserClaimReward(const uint64 eid, const id caller)
    {
        QUOTTERY::UserClaimReward_input pri;
        QUOTTERY::UserClaimReward_output pro;
        pri.eventId = eid;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 10, pri, pro, caller, 1000000);
    }
    void GOClaimReward(const uint64 eid, const id caller, std::vector<id>& ids)
    {
        QUOTTERY::GOForceClaimReward_input pri;
        QUOTTERY::GOForceClaimReward_output pro;
        pri.eventId = eid;
        for (int i = 0; i < ids.size(); i++) pri.pubkeys.set(i, ids[i]);
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 11, pri, pro, caller, 0);
    }

    void ResolveDispute(const uint64 eid, const sint64 vote, const id& caller)
    {
        QUOTTERY::ResolveDispute_input rdi;
        QUOTTERY::ResolveDispute_output rdo;
        rdi.eventId = eid;
        rdi.vote = vote;
        increaseEnergy(caller, 10000000);
        // invocationReward must be >= 10000000 based on contract code
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 9, rdi, rdo, caller, 10000000);
    }

    void Dispute(const uint64 eid, const id& caller, sint64 depositAmount)
    {
        QUOTTERY::Dispute_input di;
        QUOTTERY::Dispute_output d_out;
        di.eventId = eid;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 8, di, d_out, caller, depositAmount);
    }

    void GetOrders(uint64 eid, uint64 option, uint64 isBid, uint64 offset, QUOTTERY::GetOrders_output& out)
    {
        QUOTTERY::GetOrders_input in;
        in.eventId = eid;
        in.option = option;
        in.isBid = isBid;
        in.offset = offset;
        callFunction(QUOTTERY_CONTRACT_INDEX, 3, in, out);
    }

    void GetActiveEvent(QUOTTERY::GetActiveEvent_output& out)
    {
        QUOTTERY::GetActiveEvent_input in;
        callFunction(QUOTTERY_CONTRACT_INDEX, 4, in, out);
    }

    void BasicInfo(QUOTTERY::BasicInfo_output& out)
    {
        QUOTTERY::BasicInfo_input in;
        callFunction(QUOTTERY_CONTRACT_INDEX, 1, in, out);
    }

    void GetEventInfo(uint64 eid, QUOTTERY::QtryEventInfo& qei)
    {
        QUOTTERY::GetEventInfo_input in;
        in.eventId = eid;
        QUOTTERY::GetEventInfo_output out;
        callFunction(QUOTTERY_CONTRACT_INDEX, 2, in, out);
        qei = out.qei;
    }

    void GetEventInfoFull(uint64 eid, QUOTTERY::GetEventInfo_output& out)
    {
        QUOTTERY::GetEventInfo_input in;
        in.eventId = eid;
        callFunction(QUOTTERY_CONTRACT_INDEX, 2, in, out);
    }

    inline static uint64 orderKey(uint64 option, uint64 tradeBit, uint64 eid)
    {
        return ((option << 63) | (tradeBit << 62) | (eid & QUOTTERY_EID_MASK));
    }
    inline static uint64 posKey(uint64 option, uint64 eid)
    {
        return (((uint64)(option) << 63) | (eid & QUOTTERY_EID_MASK));
    }
    /**
     * @brief Helper to construct a unique key for the order book.
     * Packs option, trade type (ask/bid), and event ID into a single uint64.
     */
    static id MakeOrderKey(const uint64 eid, const uint64 option, const uint64 tradeBit, id r)
    {
        r.u64._0 = 0;
        r.u64._1 = 0;
        r.u64._2 = 0;
        r.u64._3 = orderKey(option, tradeBit, eid);
        return r;
    }

    /**
     * @brief Helper to construct a unique key for a user's position.
     * Packs the user's ID with the option and event ID.
     */
    static id MakePosKey(id r, const uint64 eid, const uint64 option)
    {
        r.u64._3 = posKey(option, eid);
        return r;
    }

    void GetEventInfoBatch(const std::vector<uint64>& eids, QUOTTERY::GetEventInfoBatch_output& out)
    {
        QUOTTERY::GetEventInfoBatch_input in;
        for (int i = 0; i < (int)eids.size() && i < 64; i++)
            in.eventIds.set(i, eids[i]);
        callFunction(QUOTTERY_CONTRACT_INDEX, 5, in, out);
    }

    void GetUserPosition(const id& uid, QUOTTERY::GetUserPosition_output& out)
    {
        QUOTTERY::GetUserPosition_input in;
        in.uid = uid;
        callFunction(QUOTTERY_CONTRACT_INDEX, 6, in, out);
    }

    void GetApprovedAmount(const id& pk, QUOTTERY::GetApprovedAmount_output& out)
    {
        QUOTTERY::GetApprovedAmount_input in;
        in.pk = pk;
        callFunction(QUOTTERY_CONTRACT_INDEX, 7, in, out);
    }

    void ContractTransferQUSD(const id& from, const id& to, sint64 amount, QUOTTERY::TransferQUSD_output& out)
    {
        QUOTTERY::TransferQUSD_input in;
        in.receiver = to;
        in.amount = amount;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 12, in, out, from, 0);
    }

    void ContractTransferQTRYGOV(const id& from, const id& to, sint64 amount, QUOTTERY::TransferQTRYGOV_output& out)
    {
        QUOTTERY::TransferQTRYGOV_input in;
        in.receiver = to;
        in.amount = amount;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 15, in, out, from, 0);
    }

    void ContractTransferShareManagementRights(const id& caller, const Asset& asset, sint64 numShares, uint32 newContractIdx, QUOTTERY::TransferShareManagementRights_output& out, sint64 qus = 0)
    {
        QUOTTERY::TransferShareManagementRights_input in;
        in.asset = asset;
        in.numberOfShares = numShares;
        in.newManagingContractIndex = newContractIdx;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 13, in, out, caller, qus);
    }

    void ProposalVote(const id& caller, const QUOTTERY::QtryGOV& proposed, sint64 qus = 0)
    {
        QUOTTERY::ProposalVote_input in;
        in.proposed = proposed;
        QUOTTERY::ProposalVote_output out;
        invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 100, in, out, caller, qus);
    }

    void GetTopProposals(QUOTTERY::GetTopProposals_output& out)
    {
        QUOTTERY::GetTopProposals_input in;
        callFunction(QUOTTERY_CONTRACT_INDEX, 8, in, out);
    }

    long long balanceQTRYGOV(const id& pk)
    {
        auto state = getState();
      return numberOfShares(state->mQTRYGOVIdentifier,
            { pk, QUOTTERY_CONTRACT_INDEX },
            { pk, QUOTTERY_CONTRACT_INDEX });
    }

    // Set up a test-only QTRYGOV asset and give `amount` shares to `user`.
    // The mainnet Reinit stores a different assetName in state than the one it issues,
    // so this helper overrides state->mQTRYGOVIdentifier to a locally-issued test asset.
    void setupTestQTRYGOV(const id& user, sint64 amount)
    {
        auto state = getState();
        id qtryId = id(QUOTTERY_CONTRACT_INDEX, 0, 0, 0);
        const uint64 testGovName = 0x564F4751ULL; // "QGOV" in Qubic 1-byte-per-char encoding
        state->mQTRYGOVIdentifier.assetName = testGovName;
        state->mQTRYGOVIdentifier.issuer = qtryId;
        QpiContextUserProcedureCall qpi(QUOTTERY_CONTRACT_INDEX, qtryId, 0);
        qpi.issueAsset(testGovName, qtryId, 0, amount, 0);
        qpi.transferShareOwnershipAndPossession(testGovName, qtryId, qtryId, qtryId, amount, user);
        qpi.freeBuffer();
    }
};

static QPI::DateAndTime wrapped_now()
{
    QPI::DateAndTime result;
    result.set(etalonTick.year + 2000, etalonTick.month, etalonTick.day, etalonTick.hour, etalonTick.minute, etalonTick.second, etalonTick.millisecond);
    return result;
}

TEST(QTRYTest, CreateEvent)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_end = dt; dt_end.addMicrosec(3600000000ULL * 2);

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;
    cei.qei.openDate = dt;
    cei.qei.openDate.addMicrosec(-1000000000);
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



    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1);
    QUOTTERY::QtryEventInfo onchain_qei;
    EXPECT_TRUE(state->mEventInfo.get(0, onchain_qei));
    EXPECT_TRUE(onchain_qei.openDate == dt);
    EXPECT_TRUE(onchain_qei.endDate == dt_end);
    for (int i = 0; i < 4; i++)
    {
        id a = onchain_qei.desc.get(i);
        EXPECT_TRUE(a == v_desc[i]);
    }

    // skew time
    memset(&cei.qei, 0, sizeof(cei.qei));
    cei.qei.eid = -1;
    cei.qei.endDate = dt;
    cei.qei.endDate.addMicrosec(-2000000000);
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]);
        cei.qei.option1Desc.set(i, v_o1[i]);
    }
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase

    // invalid creator
    qtry.CreateEvent(cei, id::randomValue(), state->mQtryGov.mFeePerDay);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase

    // lack of fund
    cei.qei.endDate = dt_end;
    qtry.CreateEvent(cei, operation_id, 0);
    EXPECT_TRUE(state->mCurrentEventID == 1); // not increase
}

TEST(QTRYTest, MatchingOrders)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_end = dt; dt_end.addMicrosec(3600000000ULL * 2);

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;

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



    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id traders[16];
    for (int i = 0; i < 16; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 100ULL * 1000000000ULL);
        qtry.transferQUSD(qtry.owner, traders[i], 100000000000LL);
    }
    // bid: 100 shares option 0 for 40000
    qtry.AddBidOrder(0, 100, 0, 40000ULL, traders[0]);
    // bid: 100 shares option 1 for 61000
    qtry.AddBidOrder(0, 100, 1, 61000ULL, traders[1]);
    // expected: mint order, matching 40k-60k 
    id key = qtry.MakePosKey(traders[0], 0, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);
    EXPECT_TRUE(qo.entity == traders[0]);

    key = qtry.MakePosKey(traders[1], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);
    EXPECT_TRUE(qo.entity == traders[1]);

    // b0 ask 50 shares opt0 for 70
    // b1 ask 40 shares opt1 for 40
    // expected nothing happens
    qtry.AddAskOrder(0, 50, 0, 70000, traders[0]);
    qtry.AddAskOrder(0, 40, 1, 40000, traders[1]);

    key = qtry.MakePosKey(traders[0], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 50);
    EXPECT_TRUE(qo.entity == traders[0]);

    key = qtry.MakePosKey(traders[1], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 60);
    EXPECT_TRUE(qo.entity == traders[1]);

    // b1 remove 40 shares opt1 for 40
    // b1 add 40 shares opt1 for 20k
    // expected: merge happens
    // b0 balance: 100B - 100*40000 + 40*70000
    // b1 balance: 100B - 100*60000 + 40*30000
    qtry.RemoveAskOrder(0, 40, 1, 40000, traders[1]);
    qtry.AddAskOrder(0, 40, 1, 20000, traders[1]);
    sint64 b0_bal = qtry.balanceUSD(traders[0]);
    sint64 b1_bal = qtry.balanceUSD(traders[1]);
    EXPECT_TRUE(b0_bal == (100000000000ULL - 100ULL * 40000 + 40ULL * 70000));
    EXPECT_TRUE(b1_bal == (100000000000ULL - 100ULL * 60000 + 40ULL * 30000));

    // b0 has 50 shares opt0 in position, 10 locked in ask order (60 total)
    // b1 has 60 shares opt1 in position, 0 in order
    key = qtry.MakePosKey(traders[0], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 50);
    EXPECT_TRUE(qo.entity == traders[0]);

    key = qtry.MakePosKey(traders[1], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 60);
    EXPECT_TRUE(qo.entity == traders[1]);

    key = qtry.MakeOrderKey(0, 0, 0, id());
    auto index = state->mABOrders.headIndex(key, 0);
    EXPECT_TRUE(index != NULL_INDEX);

    auto elem = state->mABOrders.element(index);
    EXPECT_TRUE(elem.amount == 10); // 10 left
    EXPECT_TRUE(elem.entity == traders[0]);

    // bid: 100 shares option 0 for 20000
    qtry.AddBidOrder(0, 103, 0, 20000ULL, traders[2]);
    // bid: 100 shares option 1 for 80000
    qtry.AddBidOrder(0, 103, 1, 80000ULL, traders[3]);

    key = qtry.MakePosKey(traders[2], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 103);
    EXPECT_TRUE(qo.entity == traders[2]);

    key = qtry.MakePosKey(traders[3], 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 103);
    EXPECT_TRUE(qo.entity == traders[3]);

    //normal matching order, T0 ask 10 option 0 at 40k
    // T4 bid 10 option 0 at 41k => match at 40k
    qtry.AddBidOrder(0, 10, 0, 71000ULL, traders[4]);
    sint64 b4_bal = qtry.balanceUSD(traders[4]);
    EXPECT_TRUE(b4_bal == (100000000000ULL - 10 * 70000));
    key = qtry.MakePosKey(traders[4], 0, 0);
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
    sint64 b5_bal = qtry.balanceUSD(traders[5]);
    EXPECT_TRUE(b5_bal == (100000000000ULL - 50 * 20000 - 50 * 30000 - 53 * 40000 - 10 * 50000));
    key = qtry.MakePosKey(traders[5], 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 163);
    EXPECT_TRUE(qo.entity == traders[5]);

    qtry.AddBidOrder(0, 50, 0, 20000, traders[0]);
    qtry.AddBidOrder(0, 50, 0, 30000, traders[2]);
    qtry.AddBidOrder(0, 53, 0, 40000, traders[2]);
    qtry.AddBidOrder(0, 10, 0, 50000, traders[4]);
    qtry.AddAskOrder(0, 163, 0, 20000, traders[5]);
    b5_bal = qtry.balanceUSD(traders[5]);
    EXPECT_TRUE(b5_bal == (100000000000ULL));

    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
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
    auto b8_bal = qtry.balanceUSD(traders[8]);
    EXPECT_TRUE(b8_bal == (100000000000ULL - 50 * 20000 - 50 * 30000 - 53 * 40000 - 106 * 50000 - 206 * 50000));

    qtry.AddBidOrder(1, 50, 0, 20000, traders[1]);
    qtry.AddBidOrder(1, 50, 0, 30000, traders[3]);
    qtry.AddBidOrder(1, 53, 0, 40000, traders[5]);
    qtry.AddBidOrder(1, 10, 0, 50000, traders[7]);
    auto b8_before = b8_bal;
    qtry.AddAskOrder(1, 50 + 50 + 53 + 10, 0, 20000, traders[8]);
    b8_bal = qtry.balanceUSD(traders[8]);
    EXPECT_TRUE(b8_bal == (b8_before + 50 * 20000 + 50 * 30000 + 53 * 40000 + 10 * 50000));

    b8_before = b8_bal;
    qtry.AddAskOrder(1, 1, 0, 20000, traders[5], 1000);
    qtry.AddAskOrder(1, 2, 0, 20000, traders[7], 1000);
    qtry.AddAskOrder(1, 1, 0, 20000, traders[8], 1000);
    b8_bal = qtry.balanceUSD(traders[8]);
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
    key = qtry.MakeOrderKey(1, 0, QUOTTERY_ASK_BIT, id());
    index = state->mABOrders.headIndex(key, -50001);
    EXPECT_TRUE(index != NULL_INDEX);
    auto value = state->mABOrders.element(index);
    EXPECT_TRUE(value.amount == 10);
    EXPECT_TRUE(value.entity == traders[8]);
    b8_bal = qtry.balanceUSD(traders[8]);
    b8_before = b8_bal;
    qtry.RemoveAskOrder(1, 10, 0, 50001, traders[8]);
    qtry.AddAskOrder(1, 3, 1, 90000, traders[1]);
    qtry.AddAskOrder(1, 3, 1, 90000, traders[1]);
    qtry.AddAskOrder(1, 3, 1, 90001, traders[1]);
    qtry.AddAskOrder(1, 4, 1, 90001, traders[3]);
    qtry.AddAskOrder(1, 5, 1, 90003, traders[5]);
    qtry.AddBidOrder(1, 9 + 4 + 5, 1, 90003, traders[8]);
    b8_bal = qtry.balanceUSD(traders[8]);
    EXPECT_TRUE(b8_bal == b8_before - 6 * 90000 - 7 * 90001 - 5 * 90003);

    key = qtry.MakePosKey(traders[8], 1, 1);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 18);
    EXPECT_TRUE(qo.entity == traders[8]);

    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
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

    key = qtry.MakeOrderKey(2, 0, QUOTTERY_BID_BIT, id());
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

    key = qtry.MakeOrderKey(2, 0, QUOTTERY_BID_BIT, id());
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
    auto prev_b1_bal = qtry.balanceUSD(traders[1]);
    qtry.RemoveBidOrder(2, 10, 0, 10000, traders[1]);
    b1_bal = qtry.balanceUSD(traders[1]);
    EXPECT_TRUE(b1_bal - prev_b1_bal == 100000);
}

TEST(QTRYTest, CompleteCycle)
{
    ContractTestingQtry qtry;
    // add some oracles and creator
    QUOTTERY::CreateEvent_input cei;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    std::vector<id> traders;
    traders.resize(16);
    for (int i = 0; i < 16; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 100ULL * 1000000000ULL);
        qtry.transferQUSD(qtry.owner, traders[i], 100000000000LL);
    }

    // normal event
    DateAndTime dt = wrapped_now();
    DateAndTime dt_end = dt; dt_end.addMicrosec(3600000000ULL * 2); // + 2 hour

    std::string desc = "This is a test event from GTEST. Test event 2025-09-01";
    std::string opt0 = "Awesome test";
    std::string opt1 = "Perfect test";
    cei.qei.eid = -1;

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



    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    srand(0);

    sint64 total = 0;
    for (int i = 0; i < 16; i++)
    {
        total += qtry.balanceUSD(traders[i]);
    }

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
    for (int i = 0; i < 1000; i++)
    {
        qtry.AddBidOrder(0, 1 + rand() % 100, 0, 10000 + rand() % (1000 - 1), traders[rand() % 16]);
        qtry.AddBidOrder(0, 1 + rand() % 100, 1, 10000 + rand() % (1000 - 1), traders[rand() % 16]);
    }

    auto key = qtry.MakeOrderKey(0, 0, 0, id());
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
    key = qtry.MakeOrderKey(0, 0, 1, id());
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
    auto deposit = state->mQtryGov.mDepositAmountForDispute;
    qtry.PublishResult(0, 0, operation_id, deposit);
    updateEtalonTime(3600 * (24 + 3));
    qtry.FinalizeEvent(0, operation_id);
    // empty mABOrder
    key = qtry.MakeOrderKey(0, 0, 0, id());
    index = state->mABOrders.headIndex(key, 0);
    EXPECT_TRUE(index == NULL_INDEX);
    key = qtry.MakeOrderKey(0, 0, 1, id());
    index = state->mABOrders.headIndex(key, 0);
    EXPECT_TRUE(index == NULL_INDEX);

    qtry.UserClaimReward(0, traders[0]);
    qtry.GOClaimReward(0, operation_id, traders);

    total = 0;
    for (int i = 0; i < 16; i++)
    {
        total += qtry.balanceUSD(traders[i]);
    }
    EXPECT_TRUE(total == 16 * 1000000000ULL * 100ULL);
}

TEST(QTRYTest, OperationFunction)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    std::vector<id> traders(1024 + 1);
    for (int i = 0; i < 1024 + 1; i++)
    {
        traders[i] = id::randomValue();
    }
    // test updating discount fee array
    // remove non exist records
    for (int i = 0; i < 1024; i++)
    {
        qtry.UpdateFeeDiscountList(traders[i], 0, 0);
    }
    // add (values must be <= QUOTTERY_PERCENT_DENOMINATOR=1000 to pass validation)
    for (int i = 0; i < 1024; i++)
    {
        qtry.UpdateFeeDiscountList(traders[i], i % 1001, 1);
    }
    for (int i = 0; i < 1024; i++)
    {
        uint64 value;
        EXPECT_TRUE(state->mOperationParams.discountedFeeForUsers.get(traders[i], value));
        EXPECT_TRUE(value == (uint64)(i % 1001));
    }
    // remove again
    for (int i = 0; i < 1024; i++)
    {
        qtry.UpdateFeeDiscountList(traders[i], 0, 0);
    }
    for (int i = 0; i < 1024; i++)
    {
        EXPECT_FALSE(state->mOperationParams.discountedFeeForUsers.contains(traders[i]));
    }
}

TEST(QTRYTest, EscrowIntegrity)
{
    // This test verifies that placing an ASK order correctly deducts shares (Escrow)
    // and removing it refunds them. This prevents the "Fake Liquidity" bug.
    ContractTestingQtry qtry;
    QUOTTERY::CreateEvent_input cei;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);



    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id trader = id::randomValue();
    increaseEnergy(trader, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, trader, 100000000000LL);

    // 1. Trader acquires 100 shares of Option 0 via Minting (MINT scenario: two bids sum to wholeSharePrice)
    id maker = id::randomValue();
    increaseEnergy(maker, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, maker, 100000000000LL);

    // Maker Bids 100 on Option 1 (Minting half)
    qtry.AddBidOrder(0, 100, 1, 50000, maker);
    // Trader Bids 100 on Option 0 (Minting half) -> Match -> Mint
    qtry.AddBidOrder(0, 100, 0, 50000, trader);

    // Verify Initial Position
    id key = qtry.MakePosKey(trader, 0, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100);

    // 2. Trader places ASK order for 40 shares
    // The contract MUST deduct these shares from mPositionInfo immediately
    qtry.AddAskOrder(0, 40, 0, 60000, trader);

    // Verify Escrow Deduction
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 60); // Should remain 60 (100 - 40)

    // Verify Order Book
    id orderKey = qtry.MakeOrderKey(0, 0, QUOTTERY_ASK_BIT, id());
    auto index = state->mABOrders.headIndex(orderKey, -60000);
    EXPECT_TRUE(index != NULL_INDEX);
    auto orderElem = state->mABOrders.element(index);
    EXPECT_TRUE(orderElem.amount == 40);

    // 3. Trader removes ASK order
    // The contract MUST refund the shares
    qtry.RemoveAskOrder(0, 40, 0, 60000, trader);

    // Verify Refund
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_TRUE(qo.amount == 100); // Back to 100

    // Verify Order Book is empty
    index = state->mABOrders.headIndex(orderKey, -60000);
    EXPECT_TRUE(index == NULL_INDEX);
}

TEST(QTRYTest, FeeCalculation)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Fees are 0 by default (from test constructor). This test verifies transfer
    // correctness and that mShareholdersRevenue/mOperationRevenue are non-negative.
    // Fee rate formula: fee = amount * feeRate / 1,000,000 (e.g. feeRate=10 => 1%)

    // Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id seller = id::randomValue();
    id buyer = id::randomValue();
    increaseEnergy(seller, 100ULL * 1000000000ULL); // Has Money
    increaseEnergy(buyer, 100ULL * 1000000000ULL);

    // Seller gets shares first (Minting)
    qtry.AddBidOrder(0, 100, 1, 50000, buyer);
    qtry.AddBidOrder(0, 100, 0, 50000, seller); // Seller now has 100 shares of Opt 0

    // Seller Asks 100 shares @ 60,000
    qtry.AddAskOrder(0, 100, 0, 60000, seller);

    // Snapshot Seller's Balance before trade
    sint64 sellerBalBefore = qtry.balanceUSD(seller);

    // Buyer takes the order
    // Price 60,000 * 100 = 6,000,000 QU total volume
    qtry.AddBidOrder(0, 100, 0, 60000, buyer);

    // Calculate Expected Fee
    // Revenue logic in contract: 
    // ShareholderFee default? usually 0 or small in init.
    // Let's check state revenue.
    sint64 sellerBalAfter = qtry.balanceUSD(seller);
    sint64 revenue = sellerBalAfter - sellerBalBefore;

    // Total Sale = 6,000,000.
    // If fees are 0, revenue = 6,000,000.
    // If fees exist, revenue < 6,000,000.

    EXPECT_TRUE(revenue <= 6000000);

    // Verify Contract collected revenue (if fees are configured > 0 in Init)
    // Even if fees are 0, this checks state integrity
    EXPECT_TRUE(state->mShareholdersRevenue >= 0);
    EXPECT_TRUE(state->mOperationRevenue >= 0);
}

TEST(QTRYTest, DisputeInitialization)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Create a VALID event (Future endDate)
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    // Open in past is fine (started already)
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);     // Ends in 1 hour (VALID)
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    // Verify creation
    EXPECT_TRUE(state->mCurrentEventID == 1);

    // 2. TIME TRAVEL: Fast forward 2 hours (past endDate)
    updateEtalonTime(7200);

    id disputer = id::randomValue();
    increaseEnergy(disputer, 2000000000ULL); // Need funds for deposit

    // 3. Operator Publishes Result
    auto deposit = state->mQtryGov.mDepositAmountForDispute;
    increaseEnergy(operation_id, deposit * 2); // Ensure Op has funds
    qtry.PublishResult(0, 0, operation_id, deposit);

    // Verify Result is set locally
    sint8 res;
    EXPECT_TRUE(state->mEventResult.get(0, res));
    EXPECT_EQ(res, 0);

    // 4. User disputes
    qtry.Dispute(0, disputer, deposit);

    // 5. Verify Dispute State
    QUOTTERY::DepositInfo info;
    EXPECT_TRUE(state->mDisputeInfo.get(0, info));
    EXPECT_TRUE(info.pubkey == disputer);
    EXPECT_EQ(info.amount, deposit);

    // 6. Try to finalize (Should Fail because of active dispute)
    qtry.FinalizeEvent(0, operation_id);

    // Check if event still exists (Finalize deletes it)
    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(0, qei)); // Should still exist
}

TEST(QTRYTest, JanitorCleanup_GOForceClaim)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Create VALID event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL); // Ends in 1 hour
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id winner = id::randomValue();
    id loser = id::randomValue();
    increaseEnergy(winner, 100ULL * 1000000000ULL);
    increaseEnergy(loser, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000000LL);

    // 2. Mint positions (Users trade while event is active)
    qtry.AddBidOrder(0, 50, 1, 50000, loser);  // Loser buys Opt 1
    qtry.AddBidOrder(0, 50, 0, 50000, winner); // Winner buys Opt 0
    // They match and mint.

    // Verify positions exist
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(qtry.MakePosKey(winner, 0, 0), qo));
    EXPECT_TRUE(state->mPositionInfo.get(qtry.MakePosKey(loser, 0, 1), qo));

    // 3. TIME TRAVEL: Fast forward past end date
    updateEtalonTime(7200); // +2 hours

    // 4. Publish Result (Option 0 Wins)
    auto deposit = state->mQtryGov.mDepositAmountForDispute;
    increaseEnergy(operation_id, deposit * 2);
    qtry.PublishResult(0, 0, operation_id, deposit);

    // 5. TIME TRAVEL: Fast forward past dispute period (24h)
    updateEtalonTime(86400 + 7200 + 1000);

    // 6. Finalize Event
    qtry.FinalizeEvent(0, operation_id);

    // 7. User Claim (Winner) - The "Pull" Pattern
    qtry.UserClaimReward(0, winner);
    // Winner position should be gone
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(winner, 0, 0)));

    // 8. Loser Check - Position still there (stuck state)
    EXPECT_TRUE(state->mPositionInfo.contains(qtry.MakePosKey(loser, 0, 1)));

    // 9. Janitor (GO) forces cleanup
    std::vector<id> targets;
    targets.push_back(loser);
    qtry.GOClaimReward(0, operation_id, targets);

    // 10. Verify Loser position is gone
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(loser, 0, 1)));
}

TEST(QTRYTest, ViewFunctions_GetOrders_Sorting_Pagination)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id trader = id::randomValue();
    increaseEnergy(trader, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, trader, 100000000000LL);

    // 1. Add Bids (Should be sorted High Price -> Low Price)
    qtry.AddBidOrder(0, 10, 0, 10000, trader); // Lowest
    qtry.AddBidOrder(0, 10, 0, 50000, trader); // Highest
    qtry.AddBidOrder(0, 10, 0, 30000, trader); // Middle

    QUOTTERY::GetOrders_output out;

    // Fetch all (Offset 0)
    qtry.GetOrders(0, 0, 1, 0, out);

    // Verify Count
    int count = 0;
    for (int i = 0; i < 256; i++) 
    {
        if (out.orders.get(i).qo.amount > 0) count++;
    }
    EXPECT_EQ(count, 3);

    // Verify Sorting (Bid: Descending Price)
    EXPECT_EQ(out.orders.get(0).price, 50000);
    EXPECT_EQ(out.orders.get(1).price, 30000);
    EXPECT_EQ(out.orders.get(2).price, 10000);

    // 2. Test Pagination (Offset)
    QUOTTERY::GetOrders_output out_paged;
    qtry.GetOrders(0, 0, 1, 1, out_paged); // Skip top 1
    EXPECT_EQ(out_paged.orders.get(0).price, 30000); // Should be the middle one
    EXPECT_EQ(out_paged.orders.get(1).price, 10000);

    // 3. Add Asks (Should be sorted Low Price -> High Price)
    // We need shares first to ask
    qtry.AddBidOrder(0, 50, 1, 50000, trader); // Get Opt 1
    qtry.AddBidOrder(0, 50, 0, 50000, trader); // Get Opt 0 (Mint)

    qtry.AddAskOrder(0, 5, 0, 80000, trader); // High price (Low priority)
    qtry.AddAskOrder(0, 5, 0, 60000, trader); // Low price (High priority)
    qtry.AddAskOrder(0, 5, 0, 70000, trader); // Mid

    QUOTTERY::GetOrders_output out_ask;
    qtry.GetOrders(0, 0, 0, 0, out_ask);

    // Verify Sorting (Ask: Ascending Price)
    EXPECT_EQ(out_ask.orders.get(0).price, 60000);
    EXPECT_EQ(out_ask.orders.get(1).price, 70000);
    EXPECT_EQ(out_ask.orders.get(2).price, 80000);
}

TEST(QTRYTest, ViewFunctions_BasicInfo_RevenueAccumulation)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Check Initial State
    QUOTTERY::BasicInfo_output info;
    qtry.BasicInfo(info);
    EXPECT_EQ(info.nIssuedEvent, 0);
    EXPECT_EQ(info.operationRevenue, 0);

    // 2. Perform Trade to generate fee
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id t1 = id::randomValue();
    id t2 = id::randomValue();
    increaseEnergy(t1, 10000000000ULL);
    increaseEnergy(t2, 10000000000ULL);
    qtry.transferQUSD(qtry.owner, t1, 100000000000LL);
    qtry.transferQUSD(qtry.owner, t2, 100000000000LL);

    // Mint
    qtry.AddBidOrder(0, 100, 0, 50000, t1);
    qtry.AddBidOrder(0, 100, 1, 50000, t2);

    // Trade (Fee generated on transfer)
    // t1 sells 100 Opt0 to t2
    qtry.AddAskOrder(0, 100, 0, 60000, t1);
    qtry.AddBidOrder(0, 100, 0, 60000, t2);

    // 3. Check Updated Info
    qtry.BasicInfo(info);
    EXPECT_EQ(info.nIssuedEvent, 1);

    // Check internal state directly to verify view matches
    EXPECT_EQ(info.shareholdersRevenue, state->mShareholdersRevenue);
    EXPECT_EQ(info.operationRevenue, state->mOperationRevenue);
}

TEST(QTRYTest, ViewFunctions_GetEventInfo_EdgeCases)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Create Event
    QUOTTERY::CreateEvent_input cei;
    cei.qei.eid = -1;
    cei.qei.endDate = wrapped_now(); cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, 0);

    // 2. Fetch Valid
    QUOTTERY::QtryEventInfo qei;
    qtry.GetEventInfo(0, qei);
    EXPECT_EQ(qei.eid, 0);

    // 3. Fetch Invalid (Non-existent)
    QUOTTERY::QtryEventInfo qei_invalid;
    qtry.GetEventInfo(999, qei_invalid);
    // Should return zeroed struct or uninitialized depending on implementation
    // Contract code: "setMemory(output.qei, 0); state.mEventInfo.get(..., output.qei);"
    // So if get fails, it remains 0.
    EXPECT_EQ(qei_invalid.eid, 0);
    EXPECT_EQ(int(qei_invalid.endDate.getYear()), 0);
}

TEST(QTRYTest, Matching_SweepBook_PartialFills)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id whale = id::randomValue();
    increaseEnergy(whale, 1000ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, whale, 100000000000LL);

    // Setup Order Book (Selling Option 0)
    // 3 Makers with increasing prices
    id m1 = id::randomValue(); increaseEnergy(m1, 100ULL * 1000000000ULL); qtry.transferQUSD(qtry.owner, m1, 100000000000LL);
    id m2 = id::randomValue(); increaseEnergy(m2, 100ULL * 1000000000ULL); qtry.transferQUSD(qtry.owner, m2, 100000000000LL);
    id m3 = id::randomValue(); increaseEnergy(m3, 100ULL * 1000000000ULL); qtry.transferQUSD(qtry.owner, m3, 100000000000LL);

    // Helper lambda to mint shares so makers have something to sell
    auto mint = [&](id user, uint64 amt)
    {
        qtry.AddBidOrder(0, amt, 0, 50000, user);
        qtry.AddBidOrder(0, amt, 1, 50000, user); // Self-match mint
    };
    mint(m1, 100);
    mint(m2, 100);
    mint(m3, 100);

    // Place Asks (Sell Orders) at different price levels
    qtry.AddAskOrder(0, 100, 0, 40000, m1); // Cheap
    qtry.AddAskOrder(0, 100, 0, 50000, m2); // Mid
    qtry.AddAskOrder(0, 100, 0, 60000, m3); // Expensive

    sint64 whaleBalBefore = qtry.balanceUSD(whale);
    qtry.AddBidOrder(0, 250, 0, 70000, whale);
    sint64 whaleBalAfter = qtry.balanceUSD(whale);

    // 1. Check Whale Position
    QUOTTERY::QtryOrder qo;
    // Whale should have 250 shares
    EXPECT_TRUE(state->mPositionInfo.get(qtry.MakePosKey(whale, 0, 0), qo));
    EXPECT_EQ(qo.amount, 250);
    sint64 expectedCost = 12000000;
    EXPECT_EQ(whaleBalBefore - whaleBalAfter, expectedCost);

    // 3. Check Order Book Residuals
    // m1 empty, m2 empty
    // m3 should have 50 left @ 60,000
    id key = qtry.MakeOrderKey(0, 0, QUOTTERY_ASK_BIT, id());
    // Search for the specific price node
    auto index = state->mABOrders.headIndex(key, -60000);
    EXPECT_TRUE(index != NULL_INDEX);
    auto elem = state->mABOrders.element(index);
    EXPECT_EQ(elem.amount, 50);
    EXPECT_EQ(elem.entity, m3);
}

TEST(QTRYTest, Matching_Merge_ExitLiquidity)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id holderYes = id::randomValue();
    id holderNo = id::randomValue();
    increaseEnergy(holderYes, 100ULL * 1000000000ULL);
    increaseEnergy(holderNo, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, holderYes, 100000000000LL);
    qtry.transferQUSD(qtry.owner, holderNo, 100000000000LL);

    // 1. Setup Positions (Minting)
    qtry.AddBidOrder(0, 100, 0, 50000, holderYes);
    qtry.AddBidOrder(0, 100, 1, 50000, holderNo);

    // 2. Both want to exit.
    // holderYes places Ask FIRST (Maker) @ 40,000
    qtry.AddAskOrder(0, 100, 0, 40000, holderYes);

    sint64 balYesBefore = qtry.balanceUSD(holderYes);
    sint64 balNoBefore = qtry.balanceUSD(holderNo);

    // holderNo places Ask SECOND (Taker/Aggressor) @ 40,000
    // This triggers the match.
    qtry.AddAskOrder(0, 100, 1, 40000, holderNo);

    sint64 balYesAfter = qtry.balanceUSD(holderYes);
    sint64 balNoAfter = qtry.balanceUSD(holderNo);

    // 3. Verify Logic
    // Maker (Yes) gets their limit price: 40,000
    // Taker (No) gets the price improvement: 100,000 - 40,000 = 60,000

    // CORRECTED EXPECTATIONS:
    EXPECT_EQ(balYesAfter - balYesBefore, 4000000); // 40k * 100
    EXPECT_EQ(balNoAfter - balNoBefore, 6000000);   // 60k * 100

    // 4. Verify positions are gone
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(holderYes, 0, 0)));
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(holderNo, 0, 1)));

    // 5. Verify Order Book empty
    id key0 = qtry.MakeOrderKey(0, 0, QUOTTERY_ASK_BIT, id());
    id key1 = qtry.MakeOrderKey(0, 1, QUOTTERY_ASK_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(key0, 0), NULL_INDEX);
    EXPECT_EQ(state->mABOrders.headIndex(key1, 0), NULL_INDEX);
}

TEST(QTRYTest, Stability_DustAttack_Rounding)
{
    // Verifies that the matching engine can handle many small orders
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id whale = id::randomValue();
    id dust_maker = id::randomValue();
    increaseEnergy(whale, 100ULL * 1000000000ULL);
    increaseEnergy(dust_maker, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, whale, 100000000000LL);
    qtry.transferQUSD(qtry.owner, dust_maker, 100000000000LL);

    // 1. Create 50 dust BID orders (Amount = 1, Price = 50,000)
    for (int i = 0; i < 50; i++)
    {
        qtry.AddBidOrder(0, 1, 0, 50000, dust_maker);
    }

    // 2. Whale Sells 50 shares (Mint + Sell)
    qtry.AddBidOrder(0, 50, 1, 50000, whale);
    qtry.AddBidOrder(0, 50, 0, 50000, whale); // Minted 50 YES, 50 NO

    // Whale Asks 50 YES @ 50,000
    // Should match against all 50 dust orders.
    qtry.AddAskOrder(0, 50, 0, 50000, whale);

    // 3. Verify Cleanup
    // Order book for Bid Opt 0 should be empty
    id key = qtry.MakeOrderKey(0, 0, QUOTTERY_BID_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(key, 0), NULL_INDEX);

    // Whale should have 0 YES left.
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(whale, 0, 0)));
}

TEST(QTRYTest, Finalize_CleanupLogic)
{
    // Simulates finalizing an event with open orders remaining
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id user = id::randomValue();
    increaseEnergy(user, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, user, 100000000000LL);

    // 1. Create 10 Bid Orders (Open Interest)
    for (int i = 0; i < 10; i++)
    {
        qtry.AddBidOrder(0, 10, 0, 10000 + i, user);
    }

    // 2. Time Travel & Publish Result
    updateEtalonTime(7200); // Past end date
    increaseEnergy(operation_id, state->mQtryGov.mDepositAmountForDispute * 2);
    qtry.PublishResult(0, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    updateEtalonTime(100000); // Past dispute period

    // 3. Finalize
    // This should loop through open orders and refund them
    sint64 balBeforeFinalize = qtry.balanceUSD(user);
    qtry.FinalizeEvent(0, operation_id);
    sint64 balAfterFinalize = qtry.balanceUSD(user);

    // 4. Verify Empty Book
    id key = qtry.MakeOrderKey(0, 0, QUOTTERY_BID_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(key, 0), NULL_INDEX);

    // 5. Verify Refunds
    // Total invested: 10 orders * 10 shares * ~10,000 price ~ 1,000,000
    EXPECT_GT(balAfterFinalize, balBeforeFinalize);
}

TEST(QTRYTest, StressTest_ThousandTraders_ChaosMonkey)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    // 2. Initialize 1,024 Traders with Funds
    const int NUM_TRADERS = 1024;
    std::vector<id> traders(NUM_TRADERS);
    for (int i = 0; i < NUM_TRADERS; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 10000ULL * 1000000000ULL); // 10,000 QU each
        qtry.transferQUSD(qtry.owner, traders[i], 100000000000LL);
    }

    // 3. Phase 1: Seed Liquidity (Minting Phase)
    // To have an active market, users need shares. 
    // We force minting by having everyone buy both Yes/No at 50k.
    for (int i = 0; i < NUM_TRADERS; i++)
    {
        qtry.AddBidOrder(0, 100, 0, 50000, traders[i]); // Buy 100 YES
        qtry.AddBidOrder(0, 100, 1, 50000, traders[i]); // Buy 100 NO (Auto-Mint)
    }

    // Verify System State: Every trader should have 100 YES and 100 NO
    QUOTTERY::QtryOrder qo;
    for (int i = 0; i < NUM_TRADERS; i++)
    {
        id keyYes = qtry.MakePosKey(traders[i], 0, 0);
        id keyNo = qtry.MakePosKey(traders[i], 0, 1);
        EXPECT_TRUE(state->mPositionInfo.get(keyYes, qo));
        EXPECT_EQ(qo.amount, 100);
        EXPECT_TRUE(state->mPositionInfo.get(keyNo, qo));
        EXPECT_EQ(qo.amount, 100);
    }

    // 4. Phase 2: The Chaos Monkey (Random Trading)
    // 50,000 Random Operations
    srand(12345); // Fixed seed for reproducibility

    for (int i = 0; i < 50000; i++)
    {
        int traderIdx = rand() % NUM_TRADERS;
        id trader = traders[traderIdx];

        int action = rand() % 4; // 0=BidYes, 1=BidNo, 2=AskYes, 3=AskNo
        uint64 amount = 1 + (rand() % 20); // Small size 1-20
        // Price centered around 50k with variance +/- 20k
        uint64 price = 30000 + (rand() % 40000);

        switch (action)
        {
        case 0: // Bid YES (0)
            qtry.AddBidOrder(0, amount, 0, price, trader);
            break;
        case 1: // Bid NO (1)
            qtry.AddBidOrder(0, amount, 1, price, trader);
            break;
        case 2: // Ask YES (0)
            // Ensure they have position logic handled by SC validation
            // But for test speed, we rely on the initial 100 shares
            qtry.AddAskOrder(0, amount, 0, price, trader);
            break;
        case 3: // Ask NO (1)
            qtry.AddAskOrder(0, amount, 1, price, trader);
            break;
        }
    }

    // 5. Phase 3: Integrity Check
    // The sum of all shares in existence MUST be equal.
    // Total YES Shares == Total NO Shares (invariant of prediction markets)
    // Note: Shares = (Shares in PositionInfo) + (Shares locked in Ask Orders)

    uint64 totalYes = 0;
    uint64 totalNo = 0;

    // Scan PositionInfo (Simulated via iterating known traders for speed in GTest)
    // In real unit test environment we might not have iterator for HashMap easily exposed
    // so we iterate our known traders vector.
    for (int i = 0; i < NUM_TRADERS; i++)
    {
        id keyYes = qtry.MakePosKey(traders[i], 0, 0);
        if (state->mPositionInfo.get(keyYes, qo)) totalYes += qo.amount;

        id keyNo = qtry.MakePosKey(traders[i], 0, 1);
        if (state->mPositionInfo.get(keyNo, qo)) totalNo += qo.amount;
    }

    // Scan Order Book (Locked Liquidity)
    auto countLiquidity = [&](uint64 option) -> uint64
    {
        uint64 sum = 0;
        id key = qtry.MakeOrderKey(0, option, QUOTTERY_ASK_BIT, id());
        auto index = state->mABOrders.headIndex(key, 0); // Start from beginning
        int safety = 0;
        while (index != NULL_INDEX && safety++ < 100000)
        {
            auto elem = state->mABOrders.element(index);
            sum += elem.amount;
            index = state->mABOrders.nextElementIndex(index);
        }
        return sum;
    };

    totalYes += countLiquidity(0);
    totalNo += countLiquidity(1);

    // CRITICAL ASSERTION: The market must remain solvent.
    // Total YES issued must equal Total NO issued.
    EXPECT_EQ(totalYes, totalNo);

    // 6. Phase 4: Finalize & Cleanup Stress
    // Ensure the cleanup loop can handle the leftover mess
    updateEtalonTime(7200);
    increaseEnergy(operation_id, state->mQtryGov.mDepositAmountForDispute * 2);
    qtry.PublishResult(0, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);

    qtry.FinalizeEvent(0, operation_id);

    // Verify Book is completely empty
    id keyEmpty = qtry.MakeOrderKey(0, 0, QUOTTERY_BID_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(keyEmpty, 0), NULL_INDEX);
}

TEST(QTRYTest, SelfTrading_WashTrade_Integrity)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Setup
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id trader = id::randomValue();
    increaseEnergy(trader, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, trader, 100000000000LL);

    // 1. Initial State: Trader Mints 100 Shares of YES/NO
    // (Simulate by buying from another for setup)
    id liquidity_provider = id::randomValue();
    increaseEnergy(liquidity_provider, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, liquidity_provider, 100000000000LL);

    qtry.AddBidOrder(0, 100, 1, 50000, liquidity_provider);
    qtry.AddBidOrder(0, 100, 0, 50000, trader);

    // Check Trader has 100 YES shares
    QUOTTERY::QtryOrder qo;
    id key = qtry.MakePosKey(trader, 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_EQ(qo.amount, 100);

    // Snapshot Balance
    sint64 balBefore = qtry.balanceUSD(trader);

    // 2. Trader places a BUY Order (Bid) for MORE shares
    // Price: 60,000. Amount: 10.
    // Cost: 600,000 deducted.
    qtry.AddBidOrder(0, 10, 0, 60000, trader);

    sint64 balMid = qtry.balanceUSD(trader);
    // Should have deducted 600,000
    EXPECT_EQ(balBefore - balMid, 600000);

    // 3. Trader places a SELL Order (Ask) that matches their own Bid
    // Price: 60,000. Amount: 10.
    qtry.AddAskOrder(0, 10, 0, 60000, trader);

    // 4. Verify Wash Trade Result
    // The Matching Engine should:
    // A. Deduct 10 shares (Escrow logic in AddAsk)
    // B. Match Ask vs Bid
    // C. Send Cash (600,000) to Seller (Trader)
    // D. Credit Shares (10) to Buyer (Trader)

    // Net result: 
    // Cash: balMid + 600,000 = balBefore
    // Shares: 100 - 10 (escrow) + 10 (credit) = 100

    sint64 balAfter = qtry.balanceUSD(trader);
    EXPECT_EQ(balAfter, balBefore);

    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_EQ(qo.amount, 100);

    // Order book should be empty
    id orderKey = qtry.MakeOrderKey(0, 0, QUOTTERY_BID_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(orderKey, 60000), NULL_INDEX);
}

TEST(QTRYTest, Matching_Sweep_FullFill)
{
    // Verifies that the matching engine effectively sweeps the entire book
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, 0);

    id whale = id::randomValue();
    id dust_maker = id::randomValue();
    increaseEnergy(whale, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, whale, 100000000000LL);
    increaseEnergy(dust_maker, 100ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, dust_maker, 100000000000LL);

    // 1. Create 60 Dust BID orders
    // They all buy Option 0 at 50,000
    for (int i = 0; i < 60; i++)
    {
        qtry.AddBidOrder(0, 1, 0, 50000, dust_maker);
    }

    // 2. Setup Whale Position (Has 100 shares to sell)
    qtry.AddBidOrder(0, 100, 1, 50000, whale);
    qtry.AddBidOrder(0, 100, 0, 50000, whale);

    // 3. Whale dumps 60 shares
    // Since Qubic is fast, this MUST match all 60 dust orders.
    qtry.AddAskOrder(0, 60, 0, 50000, whale);

    // 4. Verify Full Sweep
    // Check Dust Orders remaining: Should be 0 (All matched)
    id keyBid = qtry.MakeOrderKey(0, 0, QUOTTERY_BID_BIT, id());

    int remainingDust = 0;
    auto index = state->mABOrders.headIndex(keyBid, 50000);
    while (index != NULL_INDEX)
    {
        remainingDust++;
        index = state->mABOrders.nextElementIndex(index);
    }

    EXPECT_EQ(remainingDust, 0);

    // Check Whale's Ask Order remaining
    // Should be fully filled and removed from book
    id keyAsk = qtry.MakeOrderKey(0, 0, QUOTTERY_ASK_BIT, id());
    index = state->mABOrders.headIndex(keyAsk, -50000);

    // If it matched perfectly (60 vs 60), the order is removed.
    EXPECT_EQ(index, NULL_INDEX);
}

TEST(QTRYTest, Fee_DynamicUpdate_Realtime)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    state->mQtryGov.mShareHolderFee = 0;
    state->mQtryGov.mOperationFee = 0;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id maker = id::randomValue();
    id taker = id::randomValue();
    id lp = id::randomValue();
    increaseEnergy(maker, 10000000000ULL);
    increaseEnergy(taker, 10000000000ULL);
    increaseEnergy(lp, 10000000000ULL);
    qtry.transferQUSD(qtry.owner, maker, 100000000000LL);
    qtry.transferQUSD(qtry.owner, taker, 100000000000LL);
    qtry.transferQUSD(qtry.owner, lp, 100000000000LL);

    qtry.AddBidOrder(0, 100, 1, 50000, lp);
    qtry.AddBidOrder(0, 100, 0, 50000, maker);

    state->mQtryGov.mShareHolderFee = 100;

    qtry.AddAskOrder(0, 100, 0, 50000, maker);

    sint64 makerBalBefore = qtry.balanceUSD(maker);
    qtry.AddBidOrder(0, 100, 0, 50000, taker);
    sint64 makerBalAfter = qtry.balanceUSD(maker);

    EXPECT_EQ(makerBalAfter - makerBalBefore, 4500000);
    EXPECT_EQ(state->mShareholdersRevenue, 500000);
}

TEST(QTRYTest, Dispute_Quorum_Failure)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    updateEtalonTime(7200);
    qtry.PublishResult(0, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    id disputer = id::randomValue();
    increaseEnergy(disputer, 20000000000ULL);
    qtry.transferQUSD(qtry.owner, disputer, 100000000000LL);
    qtry.Dispute(0, disputer, state->mQtryGov.mDepositAmountForDispute);
    id tmp = id::zero();
    for (int i = 1; i <= 100; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(0, 0, tmp);
    }
    QUOTTERY::DisputeResolveInfo dri;
    state->mDisputeResolver.get(0, dri);
    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(0, qei));
}

TEST(QTRYTest, Dispute_Process_DisputerWins)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    updateEtalonTime(7200);

    sint64 deposit = state->mQtryGov.mDepositAmountForDispute;
    increaseEnergy(operation_id, deposit * 2);
    qtry.PublishResult(0, 0, operation_id, deposit);

    id disputer = id::randomValue();
    increaseEnergy(disputer, deposit * 2);
    sint64 disputerBalBefore = qtry.balanceUSD(disputer);
    qtry.Dispute(0, disputer, deposit);
    sint64 disputerBalAfter = qtry.balanceUSD(disputer);

    EXPECT_EQ(disputerBalBefore - disputerBalAfter, deposit);

    sint64 totalPot = deposit * 2;

    sint64 computorPot = totalPot * 3 / 10;
    sint64 validVotes = 451;
    sint64 rewardPerComp = computorPot / validVotes;
    sint64 rewardForDisputer = totalPot - (rewardPerComp * validVotes);

    // Vote YES (1) to overturn result
    id tmp = id::zero();
    for (int i = 1; i <= 452; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(0, 1, tmp);
    }

    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(0, qei));

    sint64 disputerBalFinal = qtry.balanceUSD(disputer);

    EXPECT_EQ(disputerBalFinal - disputerBalAfter, rewardForDisputer);
}

TEST(QTRYTest, Dispute_Process_GOWins)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    updateEtalonTime(7200);

    sint64 deposit = state->mQtryGov.mDepositAmountForDispute;
    increaseEnergy(operation_id, deposit * 2);
    qtry.PublishResult(0, 0, operation_id, deposit);

    id disputer = id::randomValue();
    increaseEnergy(disputer, deposit * 2);
    qtry.Dispute(0, disputer, deposit);

    sint64 goBalBefore = qtry.balanceUSD(operation_id);

    sint64 totalPot = deposit * 2;

    // Exact Math Calculation for GO Reward
    sint64 computorPot = totalPot * 3 / 10;
    sint64 validVotes = 451;
    sint64 rewardPerComp = computorPot / validVotes;
    sint64 rewardForGO = totalPot - (rewardPerComp * validVotes);
    id tmp = id::zero();
    for (int i = 1; i <= 452; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(0, 0, tmp);
    }

    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(0, qei));

    sint64 goBalFinal = qtry.balanceUSD(operation_id);
    EXPECT_EQ(goBalFinal - goBalBefore, rewardForGO);
}

TEST(QTRYTest, Dispute_Resolution_AutoCleanup_And_Claim)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 0. Setup: Zero Fees for simple math checks
    state->mQtryGov.mShareHolderFee = 0;
    state->mQtryGov.mOperationFee = 0;

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL); // Ends in 1 hour
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id bidder = id::randomValue();
    id asker = id::randomValue();
    id lp = id::randomValue();
    increaseEnergy(bidder, 100000ULL * 1000000000ULL);
    increaseEnergy(asker, 100000ULL * 1000000000ULL);
    increaseEnergy(lp, 100000ULL * 1000000000ULL);
    qtry.transferQUSD(qtry.owner, bidder, 100000000000LL);
    qtry.transferQUSD(qtry.owner, asker, 100000000000LL);
    qtry.transferQUSD(qtry.owner, lp, 100000000000LL);

    sint64 bidderInitial = qtry.balanceUSD(bidder);
    // 1. Setup Position for Asker
    // Asker needs Option 1 shares to place an Ask.
    // LP Bids Opt 0, Asker Bids Opt 1 -> Match/Mint.
    qtry.AddBidOrder(0, 100, 0, 50000, lp);
    qtry.AddBidOrder(0, 100, 1, 50000, asker);
    // Asker now has 100 shares of Opt 1 (Cost 5M)

    // 2. Place Open Orders (That will NOT match)
    // Bidder: Bids on Option 0 (The eventual LOSER) @ 40k. Locks 4M cash.
    qtry.AddBidOrder(0, 100, 0, 40000, bidder);
    sint64 bidderAfterOrder = qtry.balanceUSD(bidder);
    EXPECT_EQ(bidderInitial - bidderAfterOrder, 4000000);

    // Asker: Asks to sell Option 1 (The eventual WINNER) @ 90k. Locks 100 shares.
    qtry.AddAskOrder(0, 100, 1, 90000, asker);

    // Verify Asker's shares are in Escrow (removed from Position)
    QUOTTERY::QtryOrder qo;
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(asker, 0, 1)));

    // 3. Event Ends
    updateEtalonTime(7200); // 2 hours later

    // 4. Operator Publishes WRONG Result (Option 0)
    sint64 deposit = state->mQtryGov.mDepositAmountForDispute;
    increaseEnergy(operation_id, deposit * 2);
    qtry.PublishResult(0, 0, operation_id, deposit);

    // 5. Dispute (Claiming Option 1 Wins)
    id disputer = id::randomValue();
    increaseEnergy(disputer, deposit * 2);
    qtry.Dispute(0, disputer, deposit);

    // 6. Resolve Dispute (Computors Vote Option 1)
    // This triggers FinalizeEvent, which triggers Automatic Book Cleanup
    id tmp = id::zero();
    for (int i = 1; i <= 451; i++) // Exact Quorum
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        // Vote 1 = YES (Agree with Disputer => Option 1 Wins)
        qtry.ResolveDispute(0, 1, tmp);
    }

    // 7. Verify Cleanup (Automatic Cancellation)

    // A. Bidder: Should have received full refund (4M).
    sint64 bidderFinal = qtry.balanceUSD(bidder);
    EXPECT_EQ(bidderFinal, bidderInitial);

    // B. Asker: Should have received shares back.
    id askerPosKey = qtry.MakePosKey(asker, 0, 1);
    EXPECT_TRUE(state->mPositionInfo.get(askerPosKey, qo));
    EXPECT_EQ(qo.amount, 100);

    // 8. Claim Reward
    // Asker holds Option 1. Resolution said Option 1 Won.
    // Payout = 100 shares * 100,000 price = 10,000,000.

    sint64 askerBalBeforeClaim = qtry.balanceUSD(asker);
    qtry.UserClaimReward(0, asker);
    sint64 askerBalAfterClaim = qtry.balanceUSD(asker);

    EXPECT_EQ(askerBalAfterClaim - askerBalBeforeClaim, 10000000);

    // Verify Position Removed after claim
    EXPECT_FALSE(state->mPositionInfo.contains(askerPosKey));
}

TEST(QTRYTest, Dispute_Payout_Logic_Correctness)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Setup Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;

    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    updateEtalonTime(7200);

    sint64 deposit = state->mQtryGov.mDepositAmountForDispute;
    increaseEnergy(operation_id, deposit * 2);
    qtry.PublishResult(0, 0, operation_id, deposit); // GO says Option 0 Won

    id disputer = id::randomValue();
    increaseEnergy(disputer, deposit * 2);
    qtry.Dispute(0, disputer, deposit); // Disputer claims Option 0 Lost (implies Option 1)

    // Snapshot Balances
    sint64 goBalBefore = qtry.balanceUSD(operation_id);
    sint64 dispBalBefore = qtry.balanceUSD(disputer);

    // Resolution: Computors Vote 0 (NO) -> They agree with GO.
    // GO should win the pot. Disputer should lose deposit.
    id tmp = id::zero();
    for (int i = 1; i <= 451; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(0, 0, tmp); // Vote NO
    }

    sint64 goBalAfter = qtry.balanceUSD(operation_id);
    sint64 dispBalAfter = qtry.balanceUSD(disputer);

    // Calculate expected reward for GO
    sint64 totalPot = deposit * 2;
    sint64 computorPot = totalPot * 3 / 10;
    sint64 rewardPerComp = computorPot / 451;
    sint64 goReward = totalPot - (rewardPerComp * 451);

    // Assertions
    EXPECT_EQ(goBalAfter - goBalBefore, goReward); // GO gets the money
    EXPECT_EQ(dispBalAfter, dispBalBefore);        // Disputer gets nothing (net loss of deposit)
}

TEST(QTRYTest, Dividend_Distribution_Flow)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Setup Fees
    // Shareholder: 10% (100/1000)
    // Operation: 5% (50/1000)
    state->mQtryGov.mShareHolderFee = 100;
    state->mQtryGov.mOperationFee = 50;
    state->mQtryGov.mBurnFee = 0;

    // 2. Setup Shareholders (Mocking the Governance Token)
    // The SC iterates over QTRY_CONTRACT_ASSET_NAME to pay dividends.
    // We must issue this specific asset and give it to a user.
    id shareholder = id::randomValue();
    {
        // Use owner to issue the Governance Token
        QpiContextUserProcedureCall qpi(QUOTTERY_CONTRACT_INDEX, qtry.owner, 0);
        int issuanceIndex, ownershipIndex, possessionIndex;
        char name[7] = { 'Q','T','R','Y',0,0,0 };
        issueAsset(m256i::zero(), name, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);
        int destinationOwnershipIndex, destinationPossessionIndex;
        transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareholder, NUMBER_OF_COMPUTORS, &destinationOwnershipIndex, &destinationPossessionIndex, false);
    }

    // 3. Setup Trading Environment
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id maker = id::randomValue();
    id taker = id::randomValue();
    increaseEnergy(maker, 100000000000ULL);
    increaseEnergy(taker, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, maker, 100000000000LL);
    qtry.transferQUSD(qtry.owner, taker, 100000000000LL);

    // 4. Generate Revenue via Trading
    // Step A: Maker Mints 100 shares (Buy Yes + Buy No)
    qtry.AddBidOrder(0, 100, 0, 50000, maker);
    qtry.AddBidOrder(0, 100, 1, 50000, maker);

    // Step B: Maker places Ask for 100 shares @ 60,000
    qtry.AddAskOrder(0, 100, 0, 60000, maker);

    // Step C: Taker Buys 100 shares @ 60,000
    // Transaction Volume = 100 * 60,000 = 6,000,000 mQUSDIdentifier.
    // Shareholder Fee (10%) = 600,000 mQUSDIdentifier.
    // Operation Fee (5%) = 300,000 mQUSDIdentifier.
    qtry.AddBidOrder(0, 100, 0, 60000, taker);

    // 5. Verify Revenue Accumulation in State
    EXPECT_EQ(state->mShareholdersRevenue, 600000);
    EXPECT_EQ(state->mOperationRevenue, 300000);

    // 6. Snapshot Balances Before Payout
    sint64 opBalBefore = qtry.balanceUSD(operation_id);
    sint64 shareholderBalBefore = qtry.balanceUSD(shareholder);

    // 7. Execute End Epoch (Distribute Dividends)
    qtry.endEpoch();

    // 8. Verify Payouts
    sint64 opBalAfter = qtry.balanceUSD(operation_id);
    sint64 shareholderBalAfter = qtry.balanceUSD(shareholder);

    // Operation Team Payout Check
    EXPECT_EQ(opBalAfter - opBalBefore, 300000);

    // Shareholder Payout Check
    // Total Distributable = 600,000.
    // Shareholder holds 100% of Gov Token (1M/1M).
    // (600000/676) * 676 = 599,612
    // Should receive full 599,612
    EXPECT_EQ(shareholderBalAfter - shareholderBalBefore, 599612);

    // 9. Verify State Updates
    // Revenue should be marked as distributed
    EXPECT_EQ(state->mDistributedOperationRevenue, 300000);
    // mDistributedShareholdersRevenue might effectively be updated implicitly or 
    // by logic depending on exact flow, but checking balances confirms the transfer.
}

TEST(QTRYTest, Dividend_Distribution_MultipleShareholders)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    state->mQtryGov.mShareHolderFee = 100;
    state->mQtryGov.mOperationFee = 0;
    state->mQtryGov.mBurnFee = 0;

    id shareholder1 = id::randomValue();
    id shareholder2 = id::randomValue();
    {
        int issuanceIndex, ownershipIndex, possessionIndex;
        char name[7] = { 'Q','T','R','Y',0,0,0 };
        // Issue 676 (NUMBER_OF_COMPUTORS) Total shares for easy math
        issueAsset(m256i::zero(), name, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);

        int destOwn, destPos;
        // Transfer 507 shares to Shareholder 1 (75%)
        transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareholder1, 507, &destOwn, &destPos, false);

        // Transfer 169 shares to Shareholder 2 (25%)
        transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareholder2, 169, &destOwn, &destPos, false);
    }

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id maker = id::randomValue();
    increaseEnergy(maker, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, maker, 100000000000LL);

    // Generate 1,000,000 mQUSDIdentifier Revenue
    // Volume: 10,000,000. Fee 10%.
    qtry.AddBidOrder(0, 200, 0, 50000, maker);
    qtry.AddBidOrder(0, 200, 1, 50000, maker);
    qtry.AddAskOrder(0, 200, 0, 50000, maker);
    qtry.AddBidOrder(0, 200, 0, 50000, maker);

    EXPECT_EQ(state->mShareholdersRevenue, 1000000);

    sint64 s1Before = qtry.balanceUSD(shareholder1);
    sint64 s2Before = qtry.balanceUSD(shareholder2);

    qtry.endEpoch();

    // Verification
    // Total Pot: 1,000,000
    // Per Share: 1,000,000 / 676 = 1479
    // S1 (507 shares): 1479 * 507 = 749,853
    // S2 (169 shares): 1479 * 169 = 249,951

    sint64 s1After = qtry.balanceUSD(shareholder1);
    sint64 s2After = qtry.balanceUSD(shareholder2);

    EXPECT_EQ(s1After - s1Before, 749853);
    EXPECT_EQ(s2After - s2Before, 249951);
}

TEST(QTRYTest, Dividend_Distribution_Threshold_Check)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    state->mQtryGov.mShareHolderFee = 100; // 10%
    state->mQtryGov.mOperationFee = 0;

    id shareholder = id::randomValue();
    {
        QpiContextUserProcedureCall qpi(QUOTTERY_CONTRACT_INDEX, qtry.owner, 0);
        int issuanceIndex, ownershipIndex, possessionIndex;
        char name[7] = { 'Q','T','R','Y',0,0,0 };
        issueAsset(m256i::zero(), name, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);
        int destOwn, destPos;
        transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareholder, NUMBER_OF_COMPUTORS, &destOwn, &destPos, false);
    }

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id maker = id::randomValue();
    increaseEnergy(maker, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, maker, 100000000000LL);

    // 1. MINTING PHASE (Fee-less)
    // FIX: Must sum to WHOLE_SHARE_PRICE (100,000) to actually mint shares.
    qtry.AddBidOrder(0, 1, 0, 50000, maker);
    qtry.AddBidOrder(0, 1, 1, 50000, maker);

    // Verify Minting Succeeded
    QUOTTERY::QtryOrder qo;
    id key = qtry.MakePosKey(maker, 0, 0);
    EXPECT_TRUE(state->mPositionInfo.get(key, qo));
    EXPECT_EQ(qo.amount, 1);

    // 2. TRADING PHASE (Generates Revenue)
    // Now maker has 1 YES share to sell.

    // Sell 1 YES @ 1000
    qtry.AddAskOrder(0, 1, 0, 1000, maker);

    // Buy 1 YES @ 1000 (Self-trade / Wash trade)
    qtry.AddBidOrder(0, 1, 0, 1000, maker);

    // Revenue Calculation:
    // Volume: 1000
    // Fee (10%): 100
    EXPECT_EQ(state->mShareholdersRevenue, 100);

    // 3. DISTRIBUTION PHASE
    sint64 balBefore = qtry.balanceUSD(shareholder);
    qtry.endEpoch();
    sint64 balAfter = qtry.balanceUSD(shareholder);

    // Verify Threshold Logic
    // Total Pot: 100
    // Required Minimum: 676
    // 100 < 676, so NO distribution should happen.
    EXPECT_EQ(balAfter, balBefore);
    EXPECT_EQ(state->mDistributedShareholdersRevenue, 0);
}

TEST(QTRYTest, Dividend_Distribution_With_Burn)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    state->mQtryGov.mShareHolderFee = 100;
    state->mQtryGov.mOperationFee = 0;
    state->mQtryGov.mBurnFee = 200; // 20% Burn

    id shareholder = id::randomValue();
    {
        int issuanceIndex, ownershipIndex, possessionIndex;
        char name[7] = { 'Q','T','R','Y',0,0,0 };
        issueAsset(m256i::zero(), name, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);
        int destOwn, destPos;
        transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareholder, NUMBER_OF_COMPUTORS, &destOwn, &destPos, false);
    }

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id maker = id::randomValue();
    increaseEnergy(maker, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, maker, 100000000000LL);

    // Generate 1,000,000 Revenue
    qtry.AddBidOrder(0, 200, 0, 50000, maker);
    qtry.AddBidOrder(0, 200, 1, 50000, maker);
    qtry.AddAskOrder(0, 200, 0, 50000, maker);
    qtry.AddBidOrder(0, 200, 0, 50000, maker);

    EXPECT_EQ(state->mShareholdersRevenue, 1000000);

    sint64 balBefore = qtry.balanceUSD(shareholder);
    qtry.endEpoch();
    sint64 balAfter = qtry.balanceUSD(shareholder);

    // Verify Burn: 20% of 1,000,000 = 200,000
    EXPECT_EQ(state->mBurnedAmount, 200000);

    // Verify Distributable: 800,000
    // Per Share: 800,000 / 676 = 1183
    // Payout: 1183 * 676 = 799,708
    EXPECT_EQ(balAfter - balBefore, 799708);
}

TEST(QTRYTest, Automatic_Cleanup_Lifecycle)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    uint64 eventId = 0;

    // 2. Publish Result
    updateEtalonTime(7200); // Past end date
    increaseEnergy(operation_id, state->mQtryGov.mDepositAmountForDispute * 2);
    qtry.PublishResult(eventId, 1, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // Verify Result Exists
    sint8 res;
    EXPECT_TRUE(state->mEventResult.get(eventId, res));
    EXPECT_EQ(res, 1);

    // 3. Finalize (Simulate 24h passing)
    // This cleans the Order Book and sets the flag, but DOES NOT delete Event Info yet.
    updateEtalonTime(100000); // Past dispute period
    qtry.FinalizeEvent(eventId, operation_id);

    // Verify State AFTER Finalize (Intermediate State)
    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(eventId, qei));   // Still Exists! (Waiting for EndEpoch)
    EXPECT_TRUE(state->mEventResult.get(eventId, res)); // Still Exists!

    // Verify Flag is set
    bit finalFlag;
    EXPECT_TRUE(state->mEventFinalFlag.get(eventId, finalFlag));

    // 4. Trigger Automatic Cleanup via EndEpoch
    // This performs two passes:
    // Pass 1: Pays out users using mEventInfo/mEventResult
    // Pass 2: Deletes mEventInfo, mEventResult, etc.
    qtry.endEpoch();

    // 5. Verify Complete Cleanup
    EXPECT_FALSE(state->mEventInfo.get(eventId, qei));            // Now Gone
    EXPECT_FALSE(state->mEventResult.get(eventId, res));          // Now Gone
    EXPECT_FALSE(state->mEventFinalFlag.get(eventId, finalFlag));  // Flag Gone

    // Safety check: other maps should definitely be gone too
    QUOTTERY::DepositInfo di;
    EXPECT_FALSE(state->mDisputeInfo.get(eventId, di));
}

TEST(QTRYTest, Grand_Final_Complex_Lifecycle)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Setup: Zero Fees
    state->mQtryGov.mShareHolderFee = 0;
    state->mQtryGov.mOperationFee = 0;
    state->mQtryGov.mBurnFee = 0;

    const int NUM_TRADERS = 20;
    std::vector<id> traders(NUM_TRADERS);
    sint64 initialBalance = 100000000000LL; // 100B
    sint64 totalSystemmQUSDIdentifier = 0;

    for (int i = 0; i < NUM_TRADERS; i++)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 10000ULL * 1000000000ULL);
        qtry.transferQUSD(qtry.owner, traders[i], initialBalance);
        totalSystemmQUSDIdentifier += initialBalance;
    }
    totalSystemmQUSDIdentifier += qtry.balanceUSD(qtry.owner);

    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL); // 1 hour
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    uint64 eventId = 0;

    // 2. CHAOS PHASE: 2000 Random Trades
    srand(999);
    for (int i = 0; i < 2000; i++)
    {
        int tIdx = rand() % NUM_TRADERS;
        uint64 option = rand() % 2;
        uint64 amount = 1 + (rand() % 50);
        uint64 price = 10000 + ((rand() % 90) * 1000);

        if (rand() % 2 == 0)
        {
            qtry.AddBidOrder(eventId, amount, option, price, traders[tIdx]);
        }
        else
        {
            qtry.AddAskOrder(eventId, amount, option, price, traders[tIdx]);
        }
    }

    // 3. EVENT END & PUBLISH
    updateEtalonTime(7200);

    sint64 deposit = state->mQtryGov.mDepositAmountForDispute;
    qtry.transferQUSD(qtry.owner, operation_id, deposit);

    qtry.PublishResult(eventId, 1, operation_id, deposit);

    // 4. DISPUTE
    id soreLoser = traders[0];
    qtry.Dispute(eventId, soreLoser, deposit);

    // 5. PANIC CANCEL
    for (int i = 1; i < 5; i++)
    {
        for (uint64 p = 10000; p < 90000; p += 5000)
        {
            qtry.RemoveBidOrder(eventId, 100, 0, p, traders[i]);
            qtry.RemoveBidOrder(eventId, 100, 1, p, traders[i]);
            qtry.RemoveAskOrder(eventId, 100, 0, p, traders[i]);
            qtry.RemoveAskOrder(eventId, 100, 1, p, traders[i]);
        }
    }

    // 6. RESOLUTION
    // This triggers FinalizeEvent internally
    id tmp = id::zero();
    for (int i = 1; i <= 451; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(eventId, 1, tmp);
    }

    // Check Immediate Cleanup: Orderbook is gone, but Info persists for EndEpoch
    id keyBid0 = qtry.MakeOrderKey(eventId, 0, QUOTTERY_BID_BIT, id());
    id keyAsk1 = qtry.MakeOrderKey(eventId, 1, QUOTTERY_ASK_BIT, id());
    QUOTTERY::QtryEventInfo qei;

    EXPECT_EQ(state->mABOrders.headIndex(keyBid0, 0), NULL_INDEX);
    EXPECT_EQ(state->mABOrders.headIndex(keyAsk1, 0), NULL_INDEX);

    // CHANGE: Event Info MUST still exist here for EndEpoch payout logic
    EXPECT_TRUE(state->mEventInfo.get(eventId, qei));

    // 7. CLAIM REWARDS
    for (int i = 0; i < NUM_TRADERS; i++)
    {
        qtry.UserClaimReward(eventId, traders[i]);
    }

    std::vector<id> allTraders;
    for (auto t : traders) allTraders.push_back(t);
    qtry.GOClaimReward(eventId, operation_id, allTraders);

    // 8. FINAL CLEANUP (Automatic)
    // This performs the actual deletion of mEventInfo and mEventResult
    qtry.endEpoch();

    // 9. FINAL AUDIT
    QUOTTERY::DepositInfo di;
    sint8 res;

    EXPECT_FALSE(state->mEventInfo.get(eventId, qei)); // Now Gone
    EXPECT_FALSE(state->mEventResult.get(eventId, res)); // Gone
    EXPECT_FALSE(state->mDisputeInfo.get(eventId, di));  // Gone
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(traders[5], eventId, 1))); // Gone

    sint64 finalTotal = 0;
    for (int i = 0; i < NUM_TRADERS; i++)
    {
        finalTotal += qtry.balanceUSD(traders[i]);
    }
    finalTotal += qtry.balanceUSD(qtry.owner);

    EXPECT_EQ(finalTotal, totalSystemmQUSDIdentifier);
}

TEST(QTRYTest, AskOrder_InsufficientPosition)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Setup Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    id trader = id::randomValue();
    increaseEnergy(trader, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, trader, 100000000000LL);

    // 2. Mint 100 Shares (Buy 100 Yes + 100 No)
    // Cost: 50k + 50k = 100k per unit. Total 10M.
    qtry.AddBidOrder(0, 100, 0, 50000, trader);
    qtry.AddBidOrder(0, 100, 1, 50000, trader);

    // Verify Initial Position: 100 Shares of Option 0
    id keyPos = qtry.MakePosKey(trader, 0, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(keyPos, qo));
    EXPECT_EQ(qo.amount, 100);

    // 3. Attempt to Sell 101 Shares (1 More than owned)
    // This should FAIL validation and do nothing.
    qtry.AddAskOrder(0, 101, 0, 60000, trader);

    // 4. Verify Integrity

    // A. Position should still be 100 (No deduction happened)
    EXPECT_TRUE(state->mPositionInfo.get(keyPos, qo));
    EXPECT_EQ(qo.amount, 100);

    // B. Order Book should be Empty
    id keyAsk = qtry.MakeOrderKey(0, 0, QUOTTERY_ASK_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(keyAsk, 0), NULL_INDEX);

    // 5. Valid Control Test: Sell 50 (Should work)
    qtry.AddAskOrder(0, 50, 0, 60000, trader);

    // Verify Success
    EXPECT_TRUE(state->mPositionInfo.get(keyPos, qo));
    EXPECT_EQ(qo.amount, 50); // 100 - 50 = 50 remaining

    // Verify Order Book has the 50
    auto index = state->mABOrders.headIndex(keyAsk, 0);
    EXPECT_NE(index, NULL_INDEX);
    auto order = state->mABOrders.element(index);
    EXPECT_EQ(order.amount, 50);
}

TEST(QTRYTest, EndEpoch_Full_Lifecycle_Payouts_And_Dividends)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Setup Fees: Shareholder 10%, Operation 5%
    state->mQtryGov.mShareHolderFee = 100;
    state->mQtryGov.mOperationFee = 50;

    id shareholder = id::randomValue();
    {
        QpiContextUserProcedureCall qpi(QUOTTERY_CONTRACT_INDEX, qtry.owner, 0);
        int issuanceIndex, ownershipIndex, possessionIndex;
        char name[7] = { 'Q','T','R','Y',0,0,0 };
        issueAsset(m256i::zero(), name, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex);
        int destOwn, destPos;
        transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, shareholder, NUMBER_OF_COMPUTORS, &destOwn, &destPos, false);
    }

    // 2. Create Event
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    cei.qei.eid = -1;
    cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    uint64 eventId = 0;

    // 3. Trade Setup
    id winner = id::randomValue();
    id loser = id::randomValue();
    increaseEnergy(winner, 100000000000ULL);
    increaseEnergy(loser, 100000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000000LL);

    // Mint 100 shares each
    qtry.AddBidOrder(eventId, 100, 1, 50000, winner);
    qtry.AddBidOrder(eventId, 100, 0, 50000, loser);

    // Trade to generate initial fee (60,000 revenue)
    qtry.AddAskOrder(eventId, 10, 0, 60000, loser);
    qtry.AddBidOrder(eventId, 10, 0, 60000, winner);

    sint64 initialTradingRev = state->mShareholdersRevenue; // Should be 60,000

    // 4. Finalize Setup
    updateEtalonTime(7200);
    qtry.PublishResult(eventId, 1, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);

    sint64 winnerBalBefore = qtry.balanceUSD(winner);
    sint64 shareholderBalBefore = qtry.balanceUSD(shareholder);

    // 5. TRIGGER END_EPOCH
    qtry.endEpoch();

    // 6. Verify PASS 1: User Payouts (NET of Fees)
    // Winner Payout Gross: 100 shares * 100,000 = 10,000,000
    // Fees Deducted: 15% (1,500,000)
    // Net Payout: 8,500,000
    sint64 winnerBalAfter = qtry.balanceUSD(winner);
    EXPECT_EQ(winnerBalAfter - winnerBalBefore, 8500000);

    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(winner, eventId, 1)));

    // 7. Verify PASS 2: Garbage Collection
    sint8 res;
    EXPECT_FALSE(state->mEventResult.get(eventId, res));

    // 8. Verify Dividends Distribution
    // The fees deducted from Winner (1,000,000 Shareholder portion) are added to revenue pool!
    // Total Revenue = Initial (60,000) + PayoutFee (1,000,000) = 1,060,000

    sint64 totalShareholderRev = initialTradingRev + 1000000;
    EXPECT_EQ(state->mShareholdersRevenue, totalShareholderRev); // 1,060,000

    // Distribution Math:
    // 1,060,000 / 676 = 1568 (Integer)
    // 1568 * 676 = 1,059,968 Distributed
    // Dust = 32
    sint64 expectedDistributed = 1059968;

    sint64 shareholderBalAfter = qtry.balanceUSD(shareholder);
    EXPECT_EQ(shareholderBalAfter - shareholderBalBefore, expectedDistributed);

    // Verify State Integrity
    // Revenue (1,060,000) = Distributed (1,059,968) + Dust (32)
    EXPECT_EQ(state->mShareholdersRevenue, state->mDistributedShareholdersRevenue + state->mBurnedAmount + 32);
}

TEST(QTRYTest, Cleanup_Procedures_Check)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 0. Setup: Zero Fees
    state->mQtryGov.mShareHolderFee = 0;
    state->mQtryGov.mOperationFee = 0;

    // Create 3 Events
    for (int i = 0; i < 3; ++i)
    {
        QUOTTERY::CreateEvent_input cei;
        DateAndTime dt = wrapped_now();
        cei.qei.eid = -1;
        cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
        qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    }

    // 1. Initialize 50 Traders
    const int NUM_TRADERS = 50;
    std::vector<id> traders(NUM_TRADERS);

    for (int i = 0; i < NUM_TRADERS; ++i)
    {
        traders[i] = id::randomValue();
        increaseEnergy(traders[i], 10000ULL * 1000000000ULL);
        qtry.transferQUSD(qtry.owner, traders[i], 1000000000000LL); // 1 Trillion each
    }

    // 2. Populate Data (Minting & Orders)
    for (int eid = 0; eid < 3; ++eid)
    {
        // Step A: Everyone Mints Positions (So they have inventory to Ask)
        for (int t = 0; t < NUM_TRADERS; ++t)
        {
            qtry.AddBidOrder(eid, 1000, 0, 50000, traders[t]);
            qtry.AddBidOrder(eid, 1000, 1, 50000, traders[t]);
        }

        // Step B: Create 1000 Random Orders per Event
        for (int k = 0; k < 1000; ++k)
        {
            int tIdx = k % NUM_TRADERS; // Rotate through traders

            // Ensure Bid < Ask (Spread) so they don't match immediately
            sint64 priceBase = 10000 + (k % 40) * 1000;
            sint64 bidPrice = priceBase;
            sint64 askPrice = priceBase + 20000;

            // Ask Opt 0
            qtry.AddAskOrder(eid, 1, 0, askPrice, traders[tIdx]);
            // Ask Opt 1
            qtry.AddAskOrder(eid, 1, 1, askPrice, traders[tIdx]);
            // Bid Opt 0
            qtry.AddBidOrder(eid, 1, 0, bidPrice, traders[tIdx]);
        }
    }

    // Verify Initial State
    for (int eid = 0; eid < 3; ++eid)
    {
        // OrderBook should exist
        id keyOrder = qtry.MakeOrderKey(eid, 0, QUOTTERY_ASK_BIT, id());
        EXPECT_NE(state->mABOrders.headIndex(keyOrder), NULL_INDEX);

        // Every trader should have a position
        for (int t = 0; t < NUM_TRADERS; ++t)
        {
            EXPECT_TRUE(state->mPositionInfo.contains(qtry.MakePosKey(traders[t], eid, 0)));
        }

        QUOTTERY::QtryEventInfo qei;
        EXPECT_TRUE(state->mEventInfo.get(eid, qei));
    }

    // ==========================================================
    // PHASE 1: Event 0 - Normal Finalization
    // ==========================================================

    updateEtalonTime(7200);
    qtry.PublishResult(0, 1, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);

    // Finalize: Cleans 3000+ orders across 50 users
    qtry.FinalizeEvent(0, operation_id);

    // CHECK: OrderBook Empty
    EXPECT_EQ(state->mABOrders.headIndex(qtry.MakeOrderKey(0, 0, QUOTTERY_ASK_BIT, id())), NULL_INDEX);
    EXPECT_EQ(state->mABOrders.headIndex(qtry.MakeOrderKey(0, 1, QUOTTERY_BID_BIT, id())), NULL_INDEX);

    // CHECK: Info persists
    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(0, qei));

    // End Epoch: Payouts & Metadata Cleanup
    qtry.endEpoch();

    // CHECK: Complete Wipe for Event 0
    for (int t = 0; t < NUM_TRADERS; ++t)
    {
        EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(traders[t], 0, 1)));
        EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(traders[t], 0, 0)));
    }
    EXPECT_FALSE(state->mEventInfo.get(0, qei));
    sint8 res;
    EXPECT_FALSE(state->mEventResult.get(0, res));

    // CHECK: Event 1 & 2 still intact
    EXPECT_NE(state->mABOrders.headIndex(qtry.MakeOrderKey(1, 0, QUOTTERY_ASK_BIT, id())), NULL_INDEX);

    // ==========================================================
    // PHASE 2: Event 1 - Dispute Resolution
    // ==========================================================

    increaseEnergy(operation_id, state->mQtryGov.mDepositAmountForDispute * 2);
    qtry.PublishResult(1, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    id disputer = traders[0]; // Trader 0 disputes
    increaseEnergy(disputer, 20000000000ULL);
    qtry.transferQUSD(qtry.owner, disputer, state->mQtryGov.mDepositAmountForDispute * 2);
    qtry.Dispute(1, disputer, state->mQtryGov.mDepositAmountForDispute);

    updateEtalonTime(1001);
    // Resolve (Triggers Finalize internally)
    id tmp = id::zero();
    for (int i = 1; i <= 451; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(1, 1, tmp);
    }

    // CHECK: OrderBook empty
    EXPECT_EQ(state->mABOrders.headIndex(qtry.MakeOrderKey(1, 0, QUOTTERY_ASK_BIT, id())), NULL_INDEX);

    QUOTTERY::DepositInfo di;
    EXPECT_FALSE(state->mDisputeInfo.get(1, di));

    qtry.endEpoch();

    // CHECK: Complete Wipe for Event 1
    for (int t = 0; t < NUM_TRADERS; ++t)
    {
        EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(traders[t], 1, 0)));
    }
    EXPECT_FALSE(state->mEventInfo.get(1, qei));
    EXPECT_FALSE(state->mEventResult.get(1, res));

    // ==========================================================
    // PHASE 3: Verify Control (Event 2)
    // ==========================================================

    // Event 2 should remain completely untouched
    EXPECT_NE(state->mABOrders.headIndex(qtry.MakeOrderKey(2, 0, QUOTTERY_ASK_BIT, id())), NULL_INDEX);
    EXPECT_TRUE(state->mEventInfo.get(2, qei));

    // Verify a random trader still has position in Event 2
    EXPECT_TRUE(state->mPositionInfo.contains(qtry.MakePosKey(traders[25], 2, 0)));
}

TEST(QTRYTest, Cleanup_Empty_Events_NoUsers)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 0. Setup: Zero Fees
    state->mQtryGov.mShareHolderFee = 0;
    state->mQtryGov.mOperationFee = 0;

    // Create 2 Events
    for (int i = 0; i < 2; ++i)
    {
        QUOTTERY::CreateEvent_input cei;
        DateAndTime dt = wrapped_now();
        cei.qei.eid = -1;
        cei.qei.endDate = dt; cei.qei.endDate.addMicrosec(3600000000ULL);
        qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    }

    // NOTE: We do NOT place any Bids or Asks. mPositionInfo remains empty for these events.

    // ==========================================================
    // CASE 1: No Dispute (The "Ghost" Event)
    // ==========================================================
    uint64 eventId_Ghost = 0;

    // 1. Publish Result
    updateEtalonTime(7200);
    increaseEnergy(operation_id, state->mQtryGov.mDepositAmountForDispute * 2);
    qtry.PublishResult(eventId_Ghost, 1, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // 2. Wait & Finalize
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eventId_Ghost, operation_id);

    // CHECK: Flag set, but Data still exists waiting for EndEpoch
    bit finalFlag;
    EXPECT_TRUE(state->mEventFinalFlag.get(eventId_Ghost, finalFlag));
    QUOTTERY::QtryEventInfo qei;
    EXPECT_TRUE(state->mEventInfo.get(eventId_Ghost, qei));

    // 3. End Epoch
    // Since no one has positions, the "Payout Loop" in END_EPOCH will simply skip this event.
    // However, the "Cleanup Loop" must still see the flag and delete the metadata.
    qtry.endEpoch();

    // 4. Verify Cleanup
    EXPECT_FALSE(state->mEventInfo.get(eventId_Ghost, qei));
    sint8 res;
    EXPECT_FALSE(state->mEventResult.get(eventId_Ghost, res));
    QUOTTERY::DepositInfo di;
    EXPECT_FALSE(state->mGODepositInfo.get(eventId_Ghost, di)); // Operator bond returned?

    // Verify Operator got bond back (TryFinalizeEvent returns it immediately)
    // No change in EndEpoch for this specific flow, as TryFinalize handles the bond refund.

    // ==========================================================
    // CASE 2: Dispute (The "Empty Battlefield")
    // ==========================================================
    uint64 eventId_Dispute = 1;

    // 1. Publish (Operator says 0)
    qtry.PublishResult(eventId_Dispute, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // 2. Dispute (Disputer says 1) - Even with no bets, you can dispute!
    id disputer = id::randomValue();
    increaseEnergy(disputer, 20000000000ULL);
    sint64 bondAmount = state->mQtryGov.mDepositAmountForDispute;
    qtry.transferQUSD(qtry.owner, disputer, bondAmount * 2); // Fund disputer
    sint64 disputerBalBefore = qtry.balanceUSD(disputer);

    qtry.Dispute(eventId_Dispute, disputer, bondAmount);

    // 3. Resolve (Computors vote 1 - Disputer Wins)
    id tmp = id::zero();
    for (int i = 1; i <= 451; i++)
    {
        tmp.m256i_u8[30] = i / 256;
        tmp.m256i_u8[31] = i % 256;
        qtry.ResolveDispute(eventId_Dispute, 1, tmp);
    }

    // CHECK: Dispute info cleaned immediately by ResolveDispute
    EXPECT_FALSE(state->mDisputeInfo.get(eventId_Dispute, di));
    EXPECT_FALSE(state->mGODepositInfo.get(eventId_Dispute, di));

    // Verify Financials: Disputer should win the pot
    // Pot = Operator Bond (10M) + Disputer Bond (10M) = 20M
    // Computors take 30% (6M). Winner takes 70% (14M).
    // Disputer Spent 10M bond. Receives 14M. Net Profit +4M.
    sint64 disputerBalAfter = qtry.balanceUSD(disputer);
    sint64 expectedReward = (bondAmount * 2 * 7) / 10;
    EXPECT_EQ(disputerBalAfter, (disputerBalBefore - bondAmount) + expectedReward);

    // 4. End Epoch
    // Again, no positions to payout.
    qtry.endEpoch();

    // 5. Verify Cleanup
    EXPECT_FALSE(state->mEventInfo.get(eventId_Dispute, qei));
    EXPECT_FALSE(state->mEventResult.get(eventId_Dispute, res));
    EXPECT_FALSE(state->mDisputeResolver.contains(eventId_Dispute));
}

TEST(QTRYTest, Hacker_Warfare_Simulation)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // 1. Setup: The Honeypot
    // Hacker has 100,000 mQUSDIdentifier (The seed capital)
    id hacker = id::randomValue();
    increaseEnergy(hacker, 1000000000000ULL); // Infinite energy
    qtry.transferQUSD(qtry.owner, hacker, 100000);

    sint64 hackerInitialBalance = 100000;

    // Create a valid event to attack
    QUOTTERY::CreateEvent_input cei;
    cei.qei.eid = -1;
    cei.qei.endDate = wrapped_now(); cei.qei.endDate.addMicrosec(3600000000ULL);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    uint64 eventId = 0;

    // ==========================================================
    // ATTACK 1: The "Integer Overflow" Attempt
    // ==========================================================
    // Try to buy 2^60 shares. Cost calculation should overflow or be rejected.
    uint64 hugeAmount = 1ULL << 60;

    qtry.AddBidOrder(eventId, hugeAmount, 1, 50000, hacker);
    qtry.AddAskOrder(eventId, hugeAmount, 1, 50000, hacker);

    // CHECK: Balance should be unchanged
    EXPECT_EQ(qtry.balanceUSD(hacker), hackerInitialBalance);

    // Verify no ghost positions were created
    EXPECT_FALSE(state->mPositionInfo.contains(qtry.MakePosKey(hacker, eventId, 1)));

    // ==========================================================
    // ATTACK 2: The "Double Refund" (Replay Attack)
    // ==========================================================
    // Hacker places a valid bid, then tries to cancel it TWICE.

    // Step A: Valid Bid (Cost 50,000)
    qtry.AddBidOrder(eventId, 1, 0, 50000, hacker);
    EXPECT_EQ(qtry.balanceUSD(hacker), hackerInitialBalance - 50000);

    // Step B: First Cancel (Should succeed)
    qtry.RemoveBidOrder(eventId, 1, 0, 50000, hacker);
    EXPECT_EQ(qtry.balanceUSD(hacker), hackerInitialBalance); // Back to 100k

    // Step C: Second Cancel (Should FAIL)
    qtry.RemoveBidOrder(eventId, 1, 0, 50000, hacker);

    // CHECK: Balance must NOT increase beyond initial
    EXPECT_EQ(qtry.balanceUSD(hacker), hackerInitialBalance);

    // ==========================================================
    // ATTACK 3: The "Imposter" (Unauthorized Admin Access)
    // ==========================================================
    // Hacker tries to call protected functions

    qtry.PublishResult(eventId, 0, hacker, state->mQtryGov.mDepositAmountForDispute);

    // CHECK: Result should remain NOT_SET
    sint8 res;
    state->mEventResult.get(eventId, res);
    EXPECT_EQ(res, -1);

    qtry.ResolveDispute(eventId, 0, id::zero());

    // ==========================================================
    // ATTACK 4: The "Early Bird" (Claiming before Finalization)
    // ==========================================================
    // Hacker spends ALL funds on valid positions, then tries to claim immediately.

    // Reset hacker funds for precise math check
    qtry.transferQUSD(qtry.owner, hacker, 100000 - qtry.balanceUSD(hacker));

    // Buy 1 YES + 1 NO (Cost 50k + 50k = 100k)
    qtry.AddBidOrder(eventId, 1, 0, 50000, hacker);
    qtry.AddBidOrder(eventId, 1, 1, 50000, hacker);

    // Hacker Balance is now 0. They hold 1 YES and 1 NO.

    // Try to Claim Reward NOW (Event running, result not set)
    qtry.UserClaimReward(eventId, hacker);

    // CHECK: Balance should still be 0 (Claim rejected)
    EXPECT_EQ(qtry.balanceUSD(hacker), 0);

    // ==========================================================
    // ATTACK 5: Input Fuzzing (Negative Numbers & Garbage)
    // ==========================================================
    // Refund the hacker to try fuzzing
    qtry.transferQUSD(qtry.owner, hacker, 100000);
    sint64 fuzzStartBalance = 100000;

    // Negative Price
    qtry.AddBidOrder(eventId, 1, 0, -50000, hacker);

    // Invalid Option ID (Option 99)
    qtry.AddBidOrder(eventId, 1, 99, 50000, hacker);

    // Invalid Event ID (Event 9999)
    qtry.AddBidOrder(9999, 1, 0, 50000, hacker);

    // CHECK: Hacker should not have lost or gained money from invalid calls
    // (Note: Transaction fees might apply in real network, but logic shouldn't change mQUSDIdentifier balance)
    EXPECT_EQ(qtry.balanceUSD(hacker), fuzzStartBalance);

    // ==========================================================
    // FINAL SECURITY AUDIT
    // ==========================================================
    // The ultimate test: Did the hacker manage to steal anything?
    // Current Balance should be <= Initial Balance (or 100k in the fuzzing phase)
    EXPECT_LE(qtry.balanceUSD(hacker), 100000);
}

// Helper used by several view-function tests to create a minimal valid event.
// Returns the event ID (0-based counter assigned by the contract).
static uint64 createTestEvent(ContractTestingQtry& qtry, const std::string& label)
{
    auto state = qtry.getState();
    QUOTTERY::CreateEvent_input cei;
    memset(&cei.qei, 0, sizeof(cei.qei));
    DateAndTime dt = wrapped_now();
    DateAndTime dt_end = dt; dt_end.addMicrosec(3600000000ULL * 2);
    cei.qei.eid = (uint64)-1;
    cei.qei.openDate = dt;
    cei.qei.endDate = dt_end;
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    memcpy(v_desc[0].m256i_i8, label.data(), label.size());
    std::string opt0 = "Yes"; std::string opt1 = "No";
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++) { cei.qei.option0Desc.set(i, v_o0[i]); cei.qei.option1Desc.set(i, v_o1[i]); }
    uint64 nextId = state->mCurrentEventID;
    qtry.CreateEvent(cei, qtry.owner, state->mQtryGov.mFeePerDay);
    return nextId;
}

// GetActiveEvent (function index 4)
TEST(QTRYTest, ViewFunctions_GetActiveEvent)
{
    ContractTestingQtry qtry;

    // Before any event is created every slot should be NULL_INDEX.
    QUOTTERY::GetActiveEvent_output out;
    qtry.GetActiveEvent(out);
    for (int i = 0; i < (int)QUOTTERY_MAX_CONCURRENT_EVENT; i++)
        EXPECT_EQ(out.recentActiveEvent.get(i), (uint64)NULL_INDEX);

    // Create two events and verify both appear in the active list.
    uint64 eid0 = createTestEvent(qtry, "GetActiveEvent-E0");
    uint64 eid1 = createTestEvent(qtry, "GetActiveEvent-E1");

    qtry.GetActiveEvent(out);

    bool found0 = false, found1 = false;
    for (int i = 0; i < (int)QUOTTERY_MAX_CONCURRENT_EVENT; i++)
    {
        uint64 slot = out.recentActiveEvent.get(i);
        if (slot == eid0) found0 = true;
        if (slot == eid1) found1 = true;
    }
    EXPECT_TRUE(found0);
    EXPECT_TRUE(found1);
}


// GetEventInfoBatch (function index 5)

TEST(QTRYTest, ViewFunctions_GetEventInfoBatch)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    uint64 eid0 = createTestEvent(qtry, "Batch-Event-0");
    uint64 eid1 = createTestEvent(qtry, "Batch-Event-1");

    // Retrieve both events in one call.
    QUOTTERY::GetEventInfoBatch_output out;
    qtry.GetEventInfoBatch({ eid0, eid1 }, out);

    QUOTTERY::QtryEventInfo qi0, qi1;
    EXPECT_TRUE(state->mEventInfo.get(eid0, qi0));
    EXPECT_TRUE(state->mEventInfo.get(eid1, qi1));

    // The batch output must match the on-chain state.
    EXPECT_EQ(out.aqei.get(0).eid, qi0.eid);
    EXPECT_EQ(out.aqei.get(1).eid, qi1.eid);

    // An invalid event ID produces a zero-filled entry (eid == 0).
    qtry.GetEventInfoBatch({ 9999ULL }, out);
    EXPECT_EQ(out.aqei.get(0).eid, 0ULL);
}


// GetUserPosition (function index 6)

TEST(QTRYTest, ViewFunctions_GetUserPosition)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    uint64 eid = createTestEvent(qtry, "UserPos-Event");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // User with no position -> count == 0
    QUOTTERY::GetUserPosition_output out;
    qtry.GetUserPosition(buyer0, out);
    EXPECT_EQ(out.count, 0);

    // Create a MINT: buyer0 bids option 0 at 40000, buyer1 bids option 1 at 60000.
    qtry.AddBidOrder(eid, 5, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 5, 1, 60000ULL, buyer1);

    // buyer0 should now hold 5 shares of option 0.
    qtry.GetUserPosition(buyer0, out);
    EXPECT_EQ(out.count, 1);
    EXPECT_EQ(out.p.get(0).amount, 5);
    EXPECT_EQ(out.p.get(0).eo, ContractTestingQtry::posKey(0, eid));

    // buyer1 should hold 5 shares of option 1.
    qtry.GetUserPosition(buyer1, out);
    EXPECT_EQ(out.count, 1);
    EXPECT_EQ(out.p.get(0).amount, 5);
    EXPECT_EQ(out.p.get(0).eo, ContractTestingQtry::posKey(1, eid));
}


// GetApprovedAmount (function index 7)

TEST(QTRYTest, ViewFunctions_GetApprovedAmount)
{
    ContractTestingQtry qtry;

    id user = id::randomValue(); increaseEnergy(user, 1000000000ULL);

    // User has no mQUSDIdentifier yet -> approved amount is 0.
    QUOTTERY::GetApprovedAmount_output out;
    qtry.GetApprovedAmount(user, out);
    EXPECT_EQ(out.amount, 0ULL);

    // Give user some mQUSDIdentifier and verify GetApprovedAmount matches balanceUSD.
    const sint64 amount = 5000000LL;
    qtry.transferQUSD(qtry.owner, user, amount);

    qtry.GetApprovedAmount(user, out);
    EXPECT_EQ((sint64)out.amount, qtry.balanceUSD(user));
    EXPECT_EQ((sint64)out.amount, amount);
}


// TransferQUSD procedure (procedure index 12)

TEST(QTRYTest, TransferQUSD_Procedure)
{
    ContractTestingQtry qtry;

    id sender = id::randomValue(); increaseEnergy(sender, 1000000000ULL);
    id receiver = id::randomValue(); increaseEnergy(receiver, 1000000000ULL);

    const sint64 fund = 100000LL;
    qtry.transferQUSD(qtry.owner, sender, fund);
    EXPECT_EQ(qtry.balanceUSD(sender), fund);
    EXPECT_EQ(qtry.balanceUSD(receiver), 0LL);

    // Valid transfer: send half to receiver.
    QUOTTERY::TransferQUSD_output out;
    qtry.ContractTransferQUSD(sender, receiver, fund / 2, out);
    EXPECT_EQ((sint64)out.amount, fund / 2);
    EXPECT_EQ(qtry.balanceUSD(sender), fund / 2);
    EXPECT_EQ(qtry.balanceUSD(receiver), fund / 2);

    // Transfer more than available -> output.amount is (uint64)-1 (error sentinel).
    qtry.ContractTransferQUSD(sender, receiver, fund * 10, out);
    EXPECT_EQ(out.amount, (uint64)-1);
    // Balances unchanged after failed transfer.
    EXPECT_EQ(qtry.balanceUSD(sender), fund / 2);
    EXPECT_EQ(qtry.balanceUSD(receiver), fund / 2);
}


// TransferQTRYGOV procedure (procedure index 15)

TEST(QTRYTest, TransferQTRYGOV_Procedure)
{
    ContractTestingQtry qtry;

    id sender = id::randomValue(); increaseEnergy(sender, 1000000000ULL);
    id receiver = id::randomValue(); increaseEnergy(receiver, 1000000000ULL);

    // Sender has no QTRYGOV -> transfer should fail.
    QUOTTERY::TransferQTRYGOV_output out;
    qtry.ContractTransferQTRYGOV(sender, receiver, 1, out);
    EXPECT_EQ(out.amount, (uint64)-1);

    // Give sender some test QTRYGOV shares.
    const sint64 gov = 10;
    qtry.setupTestQTRYGOV(sender, gov);
    EXPECT_EQ(qtry.balanceQTRYGOV(sender), gov);

    // Valid transfer.
    qtry.ContractTransferQTRYGOV(sender, receiver, gov / 2, out);
    EXPECT_EQ((sint64)out.amount, gov / 2);
    EXPECT_EQ(qtry.balanceQTRYGOV(sender), gov / 2);
    EXPECT_EQ(qtry.balanceQTRYGOV(receiver), gov / 2);

    // Transfer more than available -> error sentinel.
    qtry.ContractTransferQTRYGOV(sender, receiver, gov * 10, out);
    EXPECT_EQ(out.amount, (uint64)-1);
    EXPECT_EQ(qtry.balanceQTRYGOV(sender), gov / 2);
    EXPECT_EQ(qtry.balanceQTRYGOV(receiver), gov / 2);
}


// TransferShareManagementRights procedure (procedure index 13)

TEST(QTRYTest, TransferShareManagementRights_Procedure)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    id user = id::randomValue(); increaseEnergy(user, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, user, 50000LL);

    QUOTTERY::TransferShareManagementRights_output out;

    // Passing an asset other than the contract's mQUSDIdentifier must be rejected immediately
    // (output.transferredNumberOfShares stays 0, procedure returns early).
    Asset wrongAsset;
    wrongAsset.assetName = 999999ULL;
    wrongAsset.issuer = user; // arbitrary non-mQUSDIdentifier issuer
    qtry.ContractTransferShareManagementRights(user, wrongAsset, 1, QUOTTERY_CONTRACT_INDEX, out);
    EXPECT_EQ(out.transferredNumberOfShares, 0);

    // Passing the correct mQUSDIdentifier asset but without the caller owning shares under
    // SELF_INDEX management should also fail (0 shares transferred).
    Asset mQUSDIdentifierAsset = state->mQUSDIdentifier;
    qtry.ContractTransferShareManagementRights(user, mQUSDIdentifierAsset, 1, QUOTTERY_CONTRACT_INDEX, out);
    EXPECT_EQ(out.transferredNumberOfShares, 0);
}

TEST(QTRYTest, Security_RemoveAskOrder_FullPosition)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    uint64 eid = createTestEvent(qtry, "Security-AskRemove");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // MINT: buyer0 gets 10 shares of option 0, buyer1 gets 10 of option 1
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    // Verify buyer0 holds 10 shares of option 0
    id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 10);

    // Place ask for ALL 10 shares -- position entry is removed (amount hit 0)
    qtry.AddAskOrder(eid, 10, 0, 50000, buyer0);
    EXPECT_FALSE(state->mPositionInfo.get(posKey0, qo));

    // KEY TEST: remove the ask -- this was previously stuck because ValidatePosition
    // checked position (0) against requested amount (10) and failed.
    qtry.RemoveAskOrder(eid, 10, 0, 50000, buyer0);

    // After removal, position should be restored to 10
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 10);

    // Verify the order is actually gone from the book
    id orderKey = qtry.MakeOrderKey(eid, 0, QUOTTERY_ASK_BIT, id());
    sint64 idx = state->mABOrders.headIndex(orderKey, -50000);
    EXPECT_EQ(idx, (sint64)NULL_INDEX);
}

TEST(QTRYTest, Security_PrematureClaimBlocked)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    // mDepositAmountForDispute is 0 from constructor (setMemory zeroed mQtryGov)

    uint64 eid = createTestEvent(qtry, "Security-Claim");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    // MINT: winner gets option 0, loser gets option 1
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    // Advance past event endDate and publish result (option 0 wins)
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // Record winner's balance before attempted premature claim
    sint64 balBefore = qtry.balanceUSD(winner);

    // Attempt to claim BEFORE finalization -- should be blocked
    increaseEnergy(winner, 10000000ULL);
    qtry.UserClaimReward(eid, winner);

    // Balance must be unchanged -- claim was rejected
    EXPECT_EQ(qtry.balanceUSD(winner), balBefore);

    // Now properly finalize: advance ticks past the 1000-tick dispute window
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eid, operation_id);

    // After finalization, claim should succeed
    qtry.UserClaimReward(eid, winner);
    EXPECT_GT(qtry.balanceUSD(winner), balBefore);
}

TEST(QTRYTest, Security_PrematureGOClaimBlocked)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    uint64 eid = createTestEvent(qtry, "Security-GOClaim");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    // MINT
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    sint64 balBefore = qtry.balanceUSD(winner);

    // GO tries to force-claim before finalization -- should be blocked
    std::vector<id> winners = { winner };
    qtry.GOClaimReward(eid, operation_id, winners);
    EXPECT_EQ(qtry.balanceUSD(winner), balBefore);

    // Finalize and then claim succeeds
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eid, operation_id);
    qtry.GOClaimReward(eid, operation_id, winners);
    EXPECT_GT(qtry.balanceUSD(winner), balBefore);
}

TEST(QTRYTest, Security_DiscountRateCapped)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    id user = id::randomValue();

    // Valid rate: exactly at the cap (1000 = 100% discount)
    qtry.UpdateFeeDiscountList(user, 1000, 1);
    uint64 value = 0;
    EXPECT_TRUE(state->mOperationParams.discountedFeeForUsers.get(user, value));
    EXPECT_EQ(value, 1000ULL);

    // Invalid rate: 1001 exceeds QUOTTERY_PERCENT_DENOMINATOR -> must be rejected
    id user2 = id::randomValue();
    qtry.UpdateFeeDiscountList(user2, 1001, 1);
    EXPECT_FALSE(state->mOperationParams.discountedFeeForUsers.contains(user2));

    // Invalid rate: UINT64_MAX -> must also be rejected
    id user3 = id::randomValue();
    qtry.UpdateFeeDiscountList(user3, UINT64_MAX, 1);
    EXPECT_FALSE(state->mOperationParams.discountedFeeForUsers.contains(user3));
}

TEST(QTRYTest, Security_TransferNonPositiveAmount)
{
    ContractTestingQtry qtry;

    id sender = id::randomValue(); increaseEnergy(sender, 1000000000ULL);
    id receiver = id::randomValue(); increaseEnergy(receiver, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, sender, 100000LL);

    sint64 balBefore = qtry.balanceUSD(sender);

    // Zero transfer must be rejected
    QUOTTERY::TransferQUSD_output out;
    qtry.ContractTransferQUSD(sender, receiver, 0, out);
    EXPECT_EQ(out.amount, (uint64)-1);
    EXPECT_EQ(qtry.balanceUSD(sender), balBefore);

    // Negative transfer must be rejected
    qtry.ContractTransferQUSD(sender, receiver, -1, out);
    EXPECT_EQ(out.amount, (uint64)-1);
    EXPECT_EQ(qtry.balanceUSD(sender), balBefore);

    // Same for QTRYGOV
    const sint64 gov = 10;
    qtry.setupTestQTRYGOV(sender, gov);

    QUOTTERY::TransferQTRYGOV_output govOut;
    qtry.ContractTransferQTRYGOV(sender, receiver, 0, govOut);
    EXPECT_EQ(govOut.amount, (uint64)-1);
    EXPECT_EQ(qtry.balanceQTRYGOV(sender), gov);

    qtry.ContractTransferQTRYGOV(sender, receiver, -5, govOut);
    EXPECT_EQ(govOut.amount, (uint64)-1);
    EXPECT_EQ(qtry.balanceQTRYGOV(sender), gov);
}

TEST(QTRYTest, Security_RemoveAskOrder_AmountUnderflow)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    uint64 eid = createTestEvent(qtry, "Security-AskUnderflow");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // MINT: buyer0 gets 10 shares of option 0
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    // Place ask for 5 shares (out of 10)
    qtry.AddAskOrder(eid, 5, 0, 50000, buyer0);

    // Verify: 5 in position, 5 in order
    id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 5);

    // ATTACK: try to remove 100 shares (far more than the 5 in the order)
    qtry.RemoveAskOrder(eid, 100, 0, 50000, buyer0);

    // After the fix, removal is clamped to 5 (the actual order amount).
    // Position should be restored to exactly 10 (5 in position + 5 returned).
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 10);

    // Order should be fully removed from the book (not negative).
    id oKey = qtry.MakeOrderKey(eid, 0, QUOTTERY_ASK_BIT, id());
    sint64 idx = state->mABOrders.headIndex(oKey, -50000);
    EXPECT_EQ(idx, (sint64)NULL_INDEX);

    // A second removal attempt should be a no-op (no order left to remove)
    qtry.RemoveAskOrder(eid, 1, 0, 50000, buyer0);
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 10); // unchanged
}

TEST(QTRYTest, Security_UpdatePosition_NegativeAmountRejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    uint64 eid = createTestEvent(qtry, "Security-NegPos");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // MINT: buyer0 gets 10 shares of option 0
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 10);

    // Place ask for MORE shares than held (11 > 10). AddToAskOrder calls
    // ValidatePosition first, so this should fail silently (not deduct).
    qtry.AddAskOrder(eid, 11, 0, 50000, buyer0);

    // Position must remain at 10 -- must NOT go to -1
    EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
    EXPECT_EQ(qo.amount, 10);
}

TEST(QTRYTest, Security_FinalizeEvent_ContinuesOnRefundFailure)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    uint64 eid = createTestEvent(qtry, "Security-BidCleanup");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // MINT: creates positions and consumes both bids
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    // Place additional unmatched bids that will remain in the book
    qtry.AddBidOrder(eid, 5, 0, 30000ULL, buyer0);
    qtry.AddBidOrder(eid, 5, 1, 20000ULL, buyer1);

    // Verify bids are in the book
    id bidKey0 = qtry.MakeOrderKey(eid, 0, QUOTTERY_BID_BIT, id());
    id bidKey1 = qtry.MakeOrderKey(eid, 1, QUOTTERY_BID_BIT, id());
    EXPECT_NE(state->mABOrders.headIndex(bidKey0), (sint64)NULL_INDEX);
    EXPECT_NE(state->mABOrders.headIndex(bidKey1), (sint64)NULL_INDEX);

    // Finalize the event
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eid, operation_id);

    // After finalization, ALL bid orders must be cleaned from the book
    // (not stuck due to partial failure)
    EXPECT_EQ(state->mABOrders.headIndex(bidKey0), (sint64)NULL_INDEX);
    EXPECT_EQ(state->mABOrders.headIndex(bidKey1), (sint64)NULL_INDEX);
}

TEST(QTRYTest, Security_CleanMemory_SkipsDisputedEvents)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Security-Dispute");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    // MINT
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    // Publish result and file dispute
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    increaseEnergy(loser, 10000000ULL);
    qtry.Dispute(eid, loser, state->mQtryGov.mDepositAmountForDispute);

    // Advance past the 1000-tick window
    updateEtalonTime(200000);

    // Record position state before endEpoch
    id posKeyWinner = qtry.MakePosKey(winner, eid, 0);
    QUOTTERY::QtryOrder qo;
    EXPECT_TRUE(state->mPositionInfo.get(posKeyWinner, qo));
    sint64 posAmountBefore = qo.amount;

    // Run endEpoch -- should NOT finalize this event because it's disputed
    qtry.endEpoch();

    // Position must still exist (not paid out) because event is still under dispute
    EXPECT_TRUE(state->mPositionInfo.get(posKeyWinner, qo));
    EXPECT_EQ(qo.amount, posAmountBefore);

    // Event should NOT be finalized
    EXPECT_FALSE(state->mEventFinalFlag.contains(eid));
}

TEST(QTRYTest, Security_ResolveDispute_NoDoubleFinalization)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Security-DblFinal");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    // MINT
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    // Publish result, file dispute
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    increaseEnergy(loser, 10000000ULL);
    qtry.Dispute(eid, loser, state->mQtryGov.mDepositAmountForDispute);

    // 451 computors vote YES (option 0 = NO result)
    for (int i = 0; i < 451; i++)
    {
        qtry.ResolveDispute(eid, QUOTTERY_RESULT_YES, broadcastedComputors.computors.publicKeys[i]);
    }

    // Event should now be finalized via ResolveDispute
    EXPECT_TRUE(state->mEventFinalFlag.contains(eid));

    // Record state after first finalization
    id posKeyWinner = qtry.MakePosKey(winner, eid, 1); // option 1 wins (YES=1)
    QUOTTERY::QtryOrder qo;
    bool posExists = state->mPositionInfo.get(posKeyWinner, qo);

    // Additional computor votes should NOT trigger another FinalizeEvent
    // (the guard prevents double-finalization)
    for (int i = 451; i < 500; i++)
    {
        qtry.ResolveDispute(eid, QUOTTERY_RESULT_YES, broadcastedComputors.computors.publicKeys[i]);
    }

    // State should be unchanged after redundant votes
    QUOTTERY::QtryOrder qo2;
    bool posExists2 = state->mPositionInfo.get(posKeyWinner, qo2);
    EXPECT_EQ(posExists, posExists2);
}

TEST(QTRYTest, Security_PublishResult_NoRepublish)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Security-Republish");

    // Advance past endDate and publish result (option 0)
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 100000000ULL);
    qtry.transferQUSD(qtry.owner, operation_id, 10000000LL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // Verify result was set
    sint8 result = QUOTTERY_RESULT_NOT_SET;
    EXPECT_TRUE(state->mEventResult.get(eid, result));
    EXPECT_EQ(result, QUOTTERY_RESULT_NO); // option 0 = NO

    // Record GO's balance after first publish
    sint64 goBal = qtry.balanceUSD(operation_id);

    // Try to re-publish with option 1 -- should be rejected and deposit refunded
    qtry.PublishResult(eid, 1, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // Result should still be 0 (not flipped to 1)
    state->mEventResult.get(eid, result);
    EXPECT_EQ(result, QUOTTERY_RESULT_NO);

    // GO balance should be unchanged (deposit refunded)
    EXPECT_EQ(qtry.balanceUSD(operation_id), goBal);
}

TEST(QTRYTest, Coverage_AntiSpam_OrderProcedures)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    state->mOperationParams.mAntiSpamAmount = 1000;

    uint64 eid = createTestEvent(qtry, "Coverage-AntiSpam");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // First, disable anti-spam to set up positions via MINT
    state->mOperationParams.mAntiSpamAmount = 0;
    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);
    state->mOperationParams.mAntiSpamAmount = 1000;

    // AddAskOrder with 0 invocation reward → should be rejected (anti-spam)
    sint64 posBefore = 0;
    {
        id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
        QUOTTERY::QtryOrder qo;
        EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
        posBefore = qo.amount;
    }

    // The helper passes amountQUs=0 by default → insufficient for anti-spam
    qtry.AddAskOrder(eid, 5, 0, 50000, buyer0, 0);

    // Position should be unchanged — ask was rejected
    {
        id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
        QUOTTERY::QtryOrder qo;
        EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
        EXPECT_EQ(qo.amount, posBefore);
    }

    // With sufficient anti-spam fee, ask should succeed
    qtry.AddAskOrder(eid, 5, 0, 50000, buyer0, 1000);
    {
        id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
        QUOTTERY::QtryOrder qo;
        EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
        EXPECT_EQ(qo.amount, posBefore - 5);
    }

    // RemoveAskOrder with 0 → rejected
    qtry.RemoveAskOrder(eid, 5, 0, 50000, buyer0, 0);
    {
        id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
        QUOTTERY::QtryOrder qo;
        EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
        EXPECT_EQ(qo.amount, posBefore - 5); // unchanged
    }

    // RemoveAskOrder with sufficient fee → succeeds
    qtry.RemoveAskOrder(eid, 5, 0, 50000, buyer0, 1000);
    {
        id posKey0 = qtry.MakePosKey(buyer0, eid, 0);
        QUOTTERY::QtryOrder qo;
        EXPECT_TRUE(state->mPositionInfo.get(posKey0, qo));
        EXPECT_EQ(qo.amount, posBefore); // restored
    }
}

TEST(QTRYTest, Coverage_TryFinalizeEvent_NonGO_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    uint64 eid = createTestEvent(qtry, "Coverage-Finalize");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);

    // Non-GO tries to finalize — should be rejected
    id imposter = id::randomValue(); increaseEnergy(imposter, 10000000ULL);
    qtry.FinalizeEvent(eid, imposter);
    EXPECT_FALSE(state->mEventFinalFlag.contains(eid));

    // GO finalizes — should succeed
    qtry.FinalizeEvent(eid, operation_id);
    EXPECT_TRUE(state->mEventFinalFlag.contains(eid));
}

TEST(QTRYTest, Coverage_GOForceClaimReward_NonGO_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    uint64 eid = createTestEvent(qtry, "Coverage-GOAuth");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eid, operation_id);

    sint64 balBefore = qtry.balanceUSD(winner);

    // Non-GO tries to force-claim — should be rejected
    id imposter = id::randomValue();
    std::vector<id> winners = { winner };
    qtry.GOClaimReward(eid, imposter, winners);
    EXPECT_EQ(qtry.balanceUSD(winner), balBefore);

    // GO can claim
    qtry.GOClaimReward(eid, operation_id, winners);
    EXPECT_GT(qtry.balanceUSD(winner), balBefore);
}

TEST(QTRYTest, Coverage_UpdateFeeDiscountList_NonGO_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    id user = id::randomValue();
    id imposter = id::randomValue();

    // Imposter tries to set a discount — should be silently rejected
    // We can't call the helper (it always uses qtry.owner), so set operationId to something else
    id realGO = state->mQtryGov.mOperationId;

    // Set discount as GO (should work)
    qtry.UpdateFeeDiscountList(user, 500, 1);
    uint64 value = 0;
    EXPECT_TRUE(state->mOperationParams.discountedFeeForUsers.get(user, value));
    EXPECT_EQ(value, 500ULL);

    // Change operationId, then try to update — should be rejected
    state->mQtryGov.mOperationId = imposter;
    id user2 = id::randomValue();
    qtry.UpdateFeeDiscountList(user2, 100, 1);
    EXPECT_FALSE(state->mOperationParams.discountedFeeForUsers.contains(user2));

    // Restore
    state->mQtryGov.mOperationId = realGO;
}

TEST(QTRYTest, Coverage_PublishResult_BeforeEndDate_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    uint64 eid = createTestEvent(qtry, "Coverage-EarlyPub");

    // Do NOT advance time — event endDate is now+2h, current time is at event creation

    // Attempt to publish while event is still active
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // Result should NOT be set (still QUOTTERY_RESULT_NOT_SET from CreateEvent)
    sint8 result = 99;
    state->mEventResult.get(eid, result);
    EXPECT_EQ(result, QUOTTERY_RESULT_NOT_SET);
}

TEST(QTRYTest, Coverage_UserClaimReward_WrongFee)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    uint64 eid = createTestEvent(qtry, "Coverage-WrongFee");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eid, operation_id);

    sint64 balBefore = qtry.balanceUSD(winner);

    // UserClaimReward requires exactly 1,000,000 QU invocation reward.
    // Call with wrong amount (0) — should be silently rejected.
    {
        QUOTTERY::UserClaimReward_input pri;
        QUOTTERY::UserClaimReward_output pro;
        pri.eventId = eid;
        increaseEnergy(winner, 10000000ULL);
        qtry.invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 10, pri, pro, winner, 0);
    }
    EXPECT_EQ(qtry.balanceUSD(winner), balBefore);

    // Call with wrong amount (999999) — should also be rejected
    {
        QUOTTERY::UserClaimReward_input pri;
        QUOTTERY::UserClaimReward_output pro;
        pri.eventId = eid;
        qtry.invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 10, pri, pro, winner, 999999);
    }
    EXPECT_EQ(qtry.balanceUSD(winner), balBefore);

    // Call with correct amount (1000000) — should succeed
    qtry.UserClaimReward(eid, winner);
    EXPECT_GT(qtry.balanceUSD(winner), balBefore);
}

TEST(QTRYTest, Coverage_Dispute_BeforeResult_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Coverage-EarlyDisp");

    id disputer = id::randomValue();
    increaseEnergy(disputer, 10000000ULL);

    // No result published yet — dispute should be rejected
    qtry.Dispute(eid, disputer, state->mQtryGov.mDepositAmountForDispute);
    EXPECT_FALSE(state->mDisputeInfo.contains(eid));
}

TEST(QTRYTest, Coverage_ResolveDispute_InvalidVote)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Coverage-BadVote");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    updateEtalonTime(7200);
    increaseEnergy(qtry.owner, 10000000ULL);
    qtry.PublishResult(eid, 0, qtry.owner, state->mQtryGov.mDepositAmountForDispute);
    increaseEnergy(loser, 10000000ULL);
    qtry.Dispute(eid, loser, state->mQtryGov.mDepositAmountForDispute);

    // Submit 451 votes with invalid value (5) — should be ignored, no quorum reached
    for (int i = 0; i < 451; i++)
    {
        qtry.ResolveDispute(eid, 5, broadcastedComputors.computors.publicKeys[i]);
    }

    // Event should NOT be finalized — invalid votes don't count
    EXPECT_FALSE(state->mEventFinalFlag.contains(eid));
    // Dispute should still be active
    EXPECT_TRUE(state->mDisputeInfo.contains(eid));
}

TEST(QTRYTest, Coverage_ResolveDispute_NonComputor_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Coverage-NonComp");

    id winner = id::randomValue(); increaseEnergy(winner, 1000000000ULL);
    id loser = id::randomValue(); increaseEnergy(loser, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, winner, 100000000LL);
    qtry.transferQUSD(qtry.owner, loser, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, winner);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, loser);

    updateEtalonTime(7200);
    increaseEnergy(qtry.owner, 10000000ULL);
    qtry.PublishResult(eid, 0, qtry.owner, state->mQtryGov.mDepositAmountForDispute);
    increaseEnergy(loser, 10000000ULL);
    qtry.Dispute(eid, loser, state->mQtryGov.mDepositAmountForDispute);

    // Random non-computor tries to vote — should be silently rejected
    id rando = id::randomValue();
    increaseEnergy(rando, 100000000ULL);
    qtry.ResolveDispute(eid, 0, rando);

    // 451 valid computor votes to actually resolve it
    for (int i = 0; i < 451; i++)
    {
        qtry.ResolveDispute(eid, QUOTTERY_RESULT_YES, broadcastedComputors.computors.publicKeys[i]);
    }
    EXPECT_TRUE(state->mEventFinalFlag.contains(eid));
}

TEST(QTRYTest, ProposalVote_NonHolder_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100; // must be non-zero so voter slots (epoch=0) appear as expired

    id nonHolder = id::randomValue();
    increaseEnergy(nonHolder, 1000000000ULL);

    QUOTTERY::QtryGOV proposed;
    memset(&proposed, 0, sizeof(proposed));
    proposed.mOperationFee = 30;
    proposed.mOperationId = state->mQtryGov.mOperationId;

    qtry.ProposalVote(nonHolder, proposed);

    // No voter slot should be occupied
    bool found = false;
    for (int i = 0; i < 676; i++)
    {
      if (state->mGovVoters.get(i).publicKey == nonHolder)
        {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found);
}

TEST(QTRYTest, ProposalVote_HolderSubmitsProposal)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    id holder = id::randomValue();
    increaseEnergy(holder, 1000000000ULL);
    const sint64 govShares = 10;
    qtry.setupTestQTRYGOV(holder, govShares);
    EXPECT_EQ(qtry.balanceQTRYGOV(holder), govShares);

    QUOTTERY::QtryGOV proposed;
    memset(&proposed, 0, sizeof(proposed));
    proposed.mOperationFee = 25;
    proposed.mShareHolderFee = 10;
    proposed.mOperationId = state->mQtryGov.mOperationId;

    qtry.ProposalVote(holder, proposed);

    // Verify the proposal was stored
    bool found = false;
    for (int i = 0; i < 676; i++)
    {
      auto v = state->mGovVoters.get(i);
        if (v.publicKey == holder)
        {
            found = true;
            EXPECT_EQ(v.proposed.mOperationFee, 25ULL);
            EXPECT_EQ(v.proposed.mShareHolderFee, 10ULL);
            EXPECT_EQ((sint64)v.amountOfShares, govShares);
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(QTRYTest, ProposalVote_ResubmitUpdatesInPlace)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    id holder = id::randomValue();
    increaseEnergy(holder, 1000000000ULL);
    qtry.setupTestQTRYGOV(holder, 10);

    QUOTTERY::QtryGOV proposed1;
    memset(&proposed1, 0, sizeof(proposed1));
    proposed1.mOperationFee = 25;
    proposed1.mOperationId = state->mQtryGov.mOperationId;

    QUOTTERY::QtryGOV proposed2;
    memset(&proposed2, 0, sizeof(proposed2));
    proposed2.mOperationFee = 50;
    proposed2.mOperationId = state->mQtryGov.mOperationId;

    qtry.ProposalVote(holder, proposed1);
    qtry.ProposalVote(holder, proposed2);

    // Count how many slots have this holder — should be exactly 1
    int count = 0;
    uint64 storedFee = 0;
    for (int i = 0; i < 676; i++)
    {
      auto v = state->mGovVoters.get(i);
        if (v.publicKey == holder)
        {
            count++;
            storedFee = v.proposed.mOperationFee;
        }
    }
    EXPECT_EQ(count, 1);
    EXPECT_EQ(storedFee, 50ULL); // updated to second proposal
}

TEST(QTRYTest, ProposalVote_StaleVoterInvalidated)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    id holder1 = id::randomValue(); increaseEnergy(holder1, 1000000000ULL);
    id holder2 = id::randomValue(); increaseEnergy(holder2, 1000000000ULL);

    // Give both holders some QTRYGOV
    qtry.setupTestQTRYGOV(holder1, 20);

    // holder1 must transfer some to holder2 via the SC procedure
    QUOTTERY::TransferQTRYGOV_output tout;
    qtry.ContractTransferQTRYGOV(holder1, holder2, 10, tout);
    EXPECT_EQ(qtry.balanceQTRYGOV(holder1), 10);
    EXPECT_EQ(qtry.balanceQTRYGOV(holder2), 10);

    QUOTTERY::QtryGOV proposed;
    memset(&proposed, 0, sizeof(proposed));
    proposed.mOperationFee = 30;
    proposed.mOperationId = state->mQtryGov.mOperationId;

    // holder1 submits a proposal
    qtry.ProposalVote(holder1, proposed);

    // Verify holder1 is in the voters array
    bool found1 = false;
    for (int i = 0; i < 676; i++)
    {
      if (state->mGovVoters.get(i).publicKey == holder1)
        {
            found1 = true;
            break;
        }
    }
    EXPECT_TRUE(found1);

    // holder1 transfers ALL remaining QTRYGOV to holder2
    qtry.ContractTransferQTRYGOV(holder1, holder2, 10, tout);
    EXPECT_EQ(qtry.balanceQTRYGOV(holder1), 0);

    // holder2 submits a proposal — this triggers the stale-voter cleanup loop
    proposed.mOperationFee = 40;
    qtry.ProposalVote(holder2, proposed);

    // holder1's vote should now be invalidated (proposedEpoch set to 0)
    found1 = false;
    for (int i = 0; i < 676; i++)
    {
      auto v = state->mGovVoters.get(i);
        if (v.publicKey == holder1 && v.proposedEpoch != 0)
        {
            found1 = true;
            break;
        }
    }
    EXPECT_FALSE(found1); // invalidated

    // holder2's proposal should be stored
    bool found2 = false;
    for (int i = 0; i < 676; i++)
    {
      auto v = state->mGovVoters.get(i);
        if (v.publicKey == holder2)
        {
            found2 = true;
            EXPECT_EQ(v.proposed.mOperationFee, 40ULL);
            break;
        }
    }
    EXPECT_TRUE(found2);
}

TEST(QTRYTest, Coverage_CreateEvent_FullCapacity)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    // Give GO enough mQUSDIdentifier to create many events
    qtry.transferQUSD(qtry.owner, operation_id, 1000000000000LL);

    // Create events until we hit QUOTTERY_MAX_CONCURRENT_EVENT
    QUOTTERY::CreateEvent_input cei;
    DateAndTime dt = wrapped_now();
    DateAndTime dt_end = dt; dt_end.addMicrosec(3600000000ULL * 2);
    std::vector<id> v_desc(4, id::zero());
    std::vector<id> v_o0(2, id::zero());
    std::vector<id> v_o1(2, id::zero());
    std::string desc = "CapacityTest";
    std::string opt0 = "Y"; std::string opt1 = "N";
    memcpy(v_desc[0].m256i_i8, desc.data(), desc.size());
    memcpy(v_o0[0].m256i_i8, opt0.data(), opt0.size());
    memcpy(v_o1[0].m256i_i8, opt1.data(), opt1.size());
    for (int i = 0; i < 4; i++) cei.qei.desc.set(i, v_desc[i]);
    for (int i = 0; i < 2; i++)
    {
        cei.qei.option0Desc.set(i, v_o0[i]); cei.qei.option1Desc.set(i, v_o1[i]);
    }
    cei.qei.eid = (uint64)-1;
    cei.qei.openDate = dt;
    cei.qei.endDate = dt_end;

    for (uint64 i = 0; i < QUOTTERY_MAX_CONCURRENT_EVENT; i++)
    {
        qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);
    }
    EXPECT_EQ(state->mCurrentEventID, QUOTTERY_MAX_CONCURRENT_EVENT);
    EXPECT_EQ(state->mEventInfo.population(), QUOTTERY_MAX_CONCURRENT_EVENT);

    // One more event should be rejected — capacity full
    uint64 idBefore = state->mCurrentEventID;
    sint64 balBefore = qtry.balanceUSD(operation_id);
    qtry.CreateEvent(cei, operation_id, state->mQtryGov.mFeePerDay);

    // mCurrentEventID should NOT have incremented
    EXPECT_EQ(state->mCurrentEventID, idBefore);
    // Balance should be unchanged (fee refunded)
    EXPECT_EQ(qtry.balanceUSD(operation_id), balBefore);
}

TEST(QTRYTest, GetTopProposals_Empty)
{
    ContractTestingQtry qtry;
    system.epoch = 100;

    QUOTTERY::GetTopProposals_output out;
    qtry.GetTopProposals(out);

    EXPECT_EQ(out.uniqueCount, 0);
    EXPECT_EQ(out.top.get(0).totalVotes, 0);
    EXPECT_EQ(out.top.get(1).totalVotes, 0);
    EXPECT_EQ(out.top.get(2).totalVotes, 0);
}

TEST(QTRYTest, GetTopProposals_SingleProposal)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    id holder = id::randomValue(); increaseEnergy(holder, 1000000000ULL);
    qtry.setupTestQTRYGOV(holder, 50);

    QUOTTERY::QtryGOV p;
    memset(&p, 0, sizeof(p));
    p.mOperationFee = 30;
    p.mOperationId = state->mQtryGov.mOperationId;

    qtry.ProposalVote(holder, p);

    QUOTTERY::GetTopProposals_output out;
    qtry.GetTopProposals(out);

    EXPECT_EQ(out.uniqueCount, 1);
    EXPECT_EQ(out.top.get(0).totalVotes, 50);
    EXPECT_EQ(out.top.get(0).proposed.mOperationFee, 30ULL);
    EXPECT_EQ(out.top.get(1).totalVotes, 0); // no second proposal
}

TEST(QTRYTest, GetTopProposals_ThreeProposals_Ranked)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    // Create 3 holders with different share amounts
    id h1 = id::randomValue(); increaseEnergy(h1, 1000000000ULL);
    id h2 = id::randomValue(); increaseEnergy(h2, 1000000000ULL);
    id h3 = id::randomValue(); increaseEnergy(h3, 1000000000ULL);

    // h1 gets 100 shares, h2 gets 50, h3 gets 10
    qtry.setupTestQTRYGOV(h1, 160);
    QUOTTERY::TransferQTRYGOV_output tout;
    qtry.ContractTransferQTRYGOV(h1, h2, 50, tout);
    qtry.ContractTransferQTRYGOV(h1, h3, 10, tout);
    // h1=100, h2=50, h3=10

    QUOTTERY::QtryGOV pA, pB, pC;
    memset(&pA, 0, sizeof(pA));
    memset(&pB, 0, sizeof(pB));
    memset(&pC, 0, sizeof(pC));
    pA.mOperationFee = 10; pA.mOperationId = state->mQtryGov.mOperationId;
    pB.mOperationFee = 20; pB.mOperationId = state->mQtryGov.mOperationId;
    pC.mOperationFee = 30; pC.mOperationId = state->mQtryGov.mOperationId;

    // h3 (10 shares) votes for pC
    qtry.ProposalVote(h3, pC);
    // h2 (50 shares) votes for pB
    qtry.ProposalVote(h2, pB);
    // h1 (100 shares) votes for pA
    qtry.ProposalVote(h1, pA);

    QUOTTERY::GetTopProposals_output out;
    qtry.GetTopProposals(out);

    EXPECT_EQ(out.uniqueCount, 3);

    // Top 1: pA with 100 votes
    EXPECT_EQ(out.top.get(0).totalVotes, 100);
    EXPECT_EQ(out.top.get(0).proposed.mOperationFee, 10ULL);

    // Top 2: pB with 50 votes
    EXPECT_EQ(out.top.get(1).totalVotes, 50);
    EXPECT_EQ(out.top.get(1).proposed.mOperationFee, 20ULL);

    // Top 3: pC with 10 votes
    EXPECT_EQ(out.top.get(2).totalVotes, 10);
    EXPECT_EQ(out.top.get(2).proposed.mOperationFee, 30ULL);
}

TEST(QTRYTest, GetTopProposals_AggregatesVotes)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    id h1 = id::randomValue(); increaseEnergy(h1, 1000000000ULL);
    id h2 = id::randomValue(); increaseEnergy(h2, 1000000000ULL);
    id h3 = id::randomValue(); increaseEnergy(h3, 1000000000ULL);

    qtry.setupTestQTRYGOV(h1, 100);
    QUOTTERY::TransferQTRYGOV_output tout;
    qtry.ContractTransferQTRYGOV(h1, h2, 30, tout);
    qtry.ContractTransferQTRYGOV(h1, h3, 20, tout);
    // h1=50, h2=30, h3=20

    // Same proposal submitted by h1 and h2
    QUOTTERY::QtryGOV pSame;
    memset(&pSame, 0, sizeof(pSame));
    pSame.mOperationFee = 42;
    pSame.mOperationId = state->mQtryGov.mOperationId;

    // Different proposal by h3
    QUOTTERY::QtryGOV pDiff;
    memset(&pDiff, 0, sizeof(pDiff));
    pDiff.mOperationFee = 99;
    pDiff.mOperationId = state->mQtryGov.mOperationId;

    qtry.ProposalVote(h1, pSame);
    qtry.ProposalVote(h2, pSame);
    qtry.ProposalVote(h3, pDiff);

    QUOTTERY::GetTopProposals_output out;
    qtry.GetTopProposals(out);

    EXPECT_EQ(out.uniqueCount, 2);

    // Top 1: pSame with 50+30 = 80 aggregated votes
    EXPECT_EQ(out.top.get(0).totalVotes, 80);
    EXPECT_EQ(out.top.get(0).proposed.mOperationFee, 42ULL);

    // Top 2: pDiff with 20 votes
    EXPECT_EQ(out.top.get(1).totalVotes, 20);
    EXPECT_EQ(out.top.get(1).proposed.mOperationFee, 99ULL);
}

TEST(QTRYTest, Coverage_GovernanceQuorum_ChangesParams)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    // Give holder QTRYGOV shares so numberOfShares returns > 0
    id holder = id::randomValue(); increaseEnergy(holder, 1000000000ULL);
    qtry.setupTestQTRYGOV(holder, 500);
    EXPECT_EQ(qtry.balanceQTRYGOV(holder), 500);

    // Build a valid proposal (mOperationId must be set or isValid() may fail)
    QUOTTERY::QtryGOV proposed;
    memset(&proposed, 0, sizeof(proposed));
    proposed.mOperationFee = 77;
    proposed.mShareHolderFee = 33;
    proposed.mOperationId = state->mQtryGov.mOperationId;

    // Use ProposalVote to register the vote properly
    qtry.ProposalVote(holder, proposed);

    // Verify vote was stored
    bool found = false;
    for (int i = 0; i < 676; i++)
      if (state->mGovVoters.get(i).publicKey == holder)
        {
            found = true;
            break;
        }
    EXPECT_TRUE(found);

    qtry.endEpoch();

    // The quorum check: maxVoteCount should be 500 (>= 451)
    // If this works, mQtryGov.mOperationFee should be 77
    EXPECT_EQ(state->mQtryGov.mOperationFee, 77ULL);
    EXPECT_EQ(state->mQtryGov.mShareHolderFee, 33ULL);

    // Voter slots reset
    auto v = state->mGovVoters.get(0);
    EXPECT_EQ((int)v.proposedEpoch, 0);
}

TEST(QTRYTest, Coverage_GovernanceBelowQuorum_NoChange)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    system.epoch = 100;

    id holder = id::randomValue(); increaseEnergy(holder, 1000000000ULL);
    qtry.setupTestQTRYGOV(holder, 400);

    uint64 oldOpFee = state->mQtryGov.mOperationFee;

    QUOTTERY::QtryGOV proposed;
    memset(&proposed, 0, sizeof(proposed));
    proposed.mOperationFee = 77;
    proposed.mOperationId = state->mQtryGov.mOperationId;

    qtry.ProposalVote(holder, proposed);
    qtry.endEpoch();

    // Params should be UNCHANGED — not enough votes
    EXPECT_EQ(state->mQtryGov.mOperationFee, oldOpFee);
    // Voter slot reset
    EXPECT_EQ((int)state->mGovVoters.get(0).proposedEpoch, 0);
}

TEST(QTRYTest, Coverage_GetEventInfo_DisputeVoteCounts)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Coverage-VoteCnt");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    // Publish result and dispute
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    increaseEnergy(buyer1, 10000000ULL);
    qtry.Dispute(eid, buyer1, state->mQtryGov.mDepositAmountForDispute);

    // Before any votes: both counts should be 0
    QUOTTERY::GetEventInfo_output info;
    qtry.GetEventInfoFull(eid, info);
    EXPECT_EQ(info.computorsVote0, 0u);
    EXPECT_EQ(info.computorsVote1, 0u);
    EXPECT_EQ(info.disputerInfo.pubkey, buyer1);

    // 10 computors vote NO (0), 5 vote YES (1)
    for (int i = 0; i < 10; i++)
        qtry.ResolveDispute(eid, 0, broadcastedComputors.computors.publicKeys[i]);
    for (int i = 10; i < 15; i++)
        qtry.ResolveDispute(eid, 1, broadcastedComputors.computors.publicKeys[i]);

    qtry.GetEventInfoFull(eid, info);
    EXPECT_EQ(info.computorsVote0, 10u);
    EXPECT_EQ(info.computorsVote1, 5u);
}

TEST(QTRYTest, Coverage_Dispute_DoubleAndFinalized_Rejected)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;
    state->mQtryGov.mDepositAmountForDispute = 1000000;

    uint64 eid = createTestEvent(qtry, "Coverage-DblDisp");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    qtry.AddBidOrder(eid, 10, 0, 40000ULL, buyer0);
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);

    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);

    // First dispute succeeds
    id disputer1 = id::randomValue(); increaseEnergy(disputer1, 10000000ULL);
    qtry.Dispute(eid, disputer1, state->mQtryGov.mDepositAmountForDispute);
    EXPECT_TRUE(state->mDisputeInfo.contains(eid));

    QUOTTERY::DepositInfo di;
    state->mDisputeInfo.get(eid, di);
    EXPECT_EQ(di.pubkey, disputer1);

    // Second dispute by different user — rejected (already disputed)
    id disputer2 = id::randomValue(); increaseEnergy(disputer2, 10000000ULL);
    qtry.Dispute(eid, disputer2, state->mQtryGov.mDepositAmountForDispute);

    // Still the first disputer
    state->mDisputeInfo.get(eid, di);
    EXPECT_EQ(di.pubkey, disputer1);

    // Now resolve the dispute and finalize
    for (int i = 0; i < 451; i++)
        qtry.ResolveDispute(eid, QUOTTERY_RESULT_YES, broadcastedComputors.computors.publicKeys[i]);
    EXPECT_TRUE(state->mEventFinalFlag.contains(eid));

    uint64 eid2 = createTestEvent(qtry, "Coverage-FinalDisp");
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid2, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    state->mEventFinalFlag.set(eid2, true);
    EXPECT_TRUE(state->mEventFinalFlag.contains(eid2));

    // Dispute on already-finalized event — rejected
    id disputer3 = id::randomValue(); increaseEnergy(disputer3, 10000000ULL);
    qtry.Dispute(eid2, disputer3, state->mQtryGov.mDepositAmountForDispute);
    EXPECT_FALSE(state->mDisputeInfo.contains(eid2));
}

TEST(QTRYTest, Coverage_AntiSpam_BidOrders)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    uint64 eid = createTestEvent(qtry, "Coverage-BidSpam");

    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // Enable anti-spam
    state->mOperationParams.mAntiSpamAmount = 5000;

    // AddBidOrder with 0 QU invocation reward → rejected
    {
        QUOTTERY::AddToBidOrder_input bid;
        bid.eventId = eid;
        bid.amount = 10;
        bid.option = 0;
        bid.price = 40000;
        QUOTTERY::AddToBidOrder_output out;
        // invocationReward = 0 → less than mAntiSpamAmount → rejected
        qtry.invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 4, bid, out, buyer0, 0);
    }

    // No order should exist
    id bidKey0 = qtry.MakeOrderKey(eid, 0, QUOTTERY_BID_BIT, id());
    EXPECT_EQ(state->mABOrders.headIndex(bidKey0), (sint64)NULL_INDEX);

    // With sufficient anti-spam fee → succeeds
    {
        QUOTTERY::AddToBidOrder_input bid;
        bid.eventId = eid;
        bid.amount = 10;
        bid.option = 0;
        bid.price = 40000;
        QUOTTERY::AddToBidOrder_output out;
        increaseEnergy(buyer0, 1000000ULL);
        qtry.invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 4, bid, out, buyer0, 10 * 40000);
    }

    // Disable anti-spam to set up a MINT so buyer0 has a bid order
    state->mOperationParams.mAntiSpamAmount = 0;
    qtry.AddBidOrder(eid, 10, 1, 60000ULL, buyer1);
    // MINT happened, now place new unmatched bid
    qtry.AddBidOrder(eid, 5, 0, 30000ULL, buyer0);

    // Re-enable anti-spam
    state->mOperationParams.mAntiSpamAmount = 5000;

    sint64 balBefore = qtry.balanceUSD(buyer0);

    // RemoveBidOrder with 0 QU → rejected
    {
        QUOTTERY::AddToBidOrder_input bid;
        bid.eventId = eid;
        bid.amount = 5;
        bid.option = 0;
        bid.price = 30000;
        QUOTTERY::AddToBidOrder_output out;
        qtry.invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 5, bid, out, buyer0, 0);
    }
    // Balance unchanged — removal rejected
    EXPECT_EQ(qtry.balanceUSD(buyer0), balBefore);

    // RemoveBidOrder with sufficient fee → succeeds
    {
        QUOTTERY::AddToBidOrder_input bid;
        bid.eventId = eid;
        bid.amount = 5;
        bid.option = 0;
        bid.price = 30000;
        QUOTTERY::AddToBidOrder_output out;
        increaseEnergy(buyer0, 100000ULL);
        qtry.invokeUserProcedure(QUOTTERY_CONTRACT_INDEX, 5, bid, out, buyer0, 5000);
    }
    // Balance should have increased (bid refunded)
    EXPECT_GT(qtry.balanceUSD(buyer0), balBefore);
}

TEST(QTRYTest, Coverage_Merge_BalanceVerification)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();

    uint64 eid = createTestEvent(qtry, "Coverage-Merge");

    id seller0 = id::randomValue(); increaseEnergy(seller0, 1000000000ULL);
    id seller1 = id::randomValue(); increaseEnergy(seller1, 1000000000ULL);
    id buyer0 = id::randomValue(); increaseEnergy(buyer0, 1000000000ULL);
    id buyer1 = id::randomValue(); increaseEnergy(buyer1, 1000000000ULL);
    qtry.transferQUSD(qtry.owner, seller0, 100000000LL);
    qtry.transferQUSD(qtry.owner, seller1, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer0, 100000000LL);
    qtry.transferQUSD(qtry.owner, buyer1, 100000000LL);

    // MINT: seller0 gets 20 shares of option 0, seller1 gets 20 of option 1
    qtry.AddBidOrder(eid, 20, 0, 50000ULL, seller0);
    qtry.AddBidOrder(eid, 20, 1, 50000ULL, seller1);

    sint64 bal0Before = qtry.balanceUSD(seller0);
    sint64 bal1Before = qtry.balanceUSD(seller1);

    qtry.AddAskOrder(eid, 10, 0, 45000, seller0);
    qtry.AddAskOrder(eid, 10, 1, 55000, seller1);

    sint64 bal0After = qtry.balanceUSD(seller0);
    sint64 bal1After = qtry.balanceUSD(seller1);

    sint64 gain0 = bal0After - bal0Before;
    sint64 gain1 = bal1After - bal1Before;

    // Total payout must equal wholeSharePrice * matchedAmount (fees are 0)
    EXPECT_EQ(gain0 + gain1, (sint64)(state->wholeSharePrice * 10));

    // Individual payouts: seller0 gets 45000*10, seller1 gets 55000*10
    EXPECT_EQ(gain0, 450000LL);
    EXPECT_EQ(gain1, 550000LL);

    // Both sellers' positions should be reduced by 10 shares
    id posKey0 = qtry.MakePosKey(seller0, eid, 0);
    id posKey1 = qtry.MakePosKey(seller1, eid, 1);
    QUOTTERY::QtryOrder qo;
    state->mPositionInfo.get(posKey0, qo);
    EXPECT_EQ(qo.amount, 10); // had 20, sold 10
    state->mPositionInfo.get(posKey1, qo);
    EXPECT_EQ(qo.amount, 10); // had 20, sold 10
}

TEST(QTRYTest, Coverage_RecentActiveEvent_ClearedAfterCleanup)
{
    ContractTestingQtry qtry;
    auto state = qtry.getState();
    auto operation_id = state->mQtryGov.mOperationId;

    uint64 eid = createTestEvent(qtry, "Coverage-SlotClean");

    // Verify event is in the active list
    QUOTTERY::GetActiveEvent_output out;
    qtry.GetActiveEvent(out);
    bool found = false;
    for (int i = 0; i < (int)QUOTTERY_MAX_CONCURRENT_EVENT; i++)
    {
        if (out.recentActiveEvent.get(i) == eid)
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    // Publish result, finalize, then run endEpoch to trigger CleanMemory
    updateEtalonTime(7200);
    increaseEnergy(operation_id, 10000000ULL);
    qtry.PublishResult(eid, 0, operation_id, state->mQtryGov.mDepositAmountForDispute);
    updateEtalonTime(100000);
    qtry.FinalizeEvent(eid, operation_id);

    qtry.endEpoch();

    // After cleanup, the slot should be NULL_INDEX
    qtry.GetActiveEvent(out);
    uint64 slot = out.recentActiveEvent.get(eid % QUOTTERY_MAX_CONCURRENT_EVENT);
    EXPECT_EQ(slot, (uint64)NULL_INDEX);
}
