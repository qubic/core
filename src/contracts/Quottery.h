using namespace QPI;
//schedule:
// FIXED CONSTANTS
constexpr unsigned long long QUOTTERY_INITIAL_MAX_EVENT = 1024;
constexpr unsigned long long QUOTTERY_INITIAL_CREATOR_AND_COUNT = 1024;
constexpr unsigned long long QUOTTERY_MAX_EVENT = QUOTTERY_INITIAL_MAX_EVENT * X_MULTIPLIER;
constexpr unsigned long long QUOTTERY_MAX_OPTION = 8;
constexpr unsigned long long QUOTTERY_MAX_CREATOR_AND_COUNT = QUOTTERY_INITIAL_CREATOR_AND_COUNT * X_MULTIPLIER;
constexpr unsigned long long QUOTTERY_MAX_ORACLE_PROVIDER = 8;
constexpr unsigned long long QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET = 2048;
constexpr unsigned long long QUOTTERY_MAX_NUMBER_OF_USER = QUOTTERY_MAX_EVENT * 2048;

constexpr unsigned long long QUOTTERY_PERCENT_DENOMINATOR = 1000; // 1000 == 100%
constexpr unsigned long long QUOTTERY_HARD_CAP_CREATOR_FEE = 50; // 5%

constexpr unsigned long long QUOTTERY_TICK_TO_KEEP_AFTER_END = 100ULL;

#define QTRY_INVALID_DATETIME 1
#define QTRY_INVALID_USER 2
#define QTRY_NOT_ELIGIBLE_USER 3
#define QTRY_NOT_ELIGIBLE_ORACLE 4
#define QTRY_INSUFFICIENT_FUND 5
#define QTRY_INVALID_EVENT_ID 6
#define QTRY_INVALID_POSITION 7
#define QTRY_MATCH_TYPE_0 8 // A0,B0
#define QTRY_MATCH_TYPE_1 9 // A1,B1
#define QTRY_MATCH_TYPE_2 10 // A0,A1
#define QTRY_MATCH_TYPE_3 11 // B0,B1
#define QTRY_MAX_AMOUNT 1000000000000LL * 200ULL
struct QuotteryLogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    // Other data go here
    char _terminator; // Only data before "_terminator" are logged
};

struct QUOTTERY2
{
};

struct QUOTTERY : public ContractBase
{
public:
    /**************************************/
    /********INPUT AND OUTPUT STRUCTS******/
    /**************************************/
    struct BasicInfo_input
    {
    };
    struct BasicInfo_output
    {
        uint64 operationFee; // 4 digit number ABCD means AB.CD% | 1234 is 12.34%
        uint64 shareholderFee; // 4 digit number ABCD means AB.CD% | 1234 is 12.34%
        uint64 burnFee; // percentage
        uint64 nIssuedEvent; // number of issued event
        uint64 moneyFlow;
        uint64 totalRevenue;
        uint64 shareholdersRevenue;
        uint64 creatorRevenue;
        uint64 oracleRevenue;
        uint64 operationRevenue;
        uint64 burnedAmount;
        id gameOperator;
    };

    /**************************************/
    /************CONTRACT STATES***********/
    /**************************************/
    /*
    * QTRY V2
    */

    struct QtryEventInfo
    {
        uint64 eid;
        DateAndTime openDate; // submitted date
        DateAndTime closeDate; // close trading date and OPs start broadcasting result
        DateAndTime endDate; // stop receiving result from OPs

        id creator;
        /*256 chars to describe*/
        Array<id, 4> desc;
        /*128 chars to describe option */
        Array<id, 2> option0Desc;
        Array<id, 2> option1Desc;
        /*8 oracle IDs*/
        Array<id, 8> oracleId;

        // payment info
        uint64 type; // 0: QUs - 1: token (must be managed by QX)
        id assetIssuer;
        uint64 assetName;
    };

#define WHOLE_SHARE_PRICE 100000ULL
#define ASK_BIT 0
#define BID_BIT 1
#define EID_MASK 0x3FFFFFFFFFFFFFFFULL // (2^62 - 1)
#define ORDER_KEY(option, trade_bit, eid) ((option<<63)|(trade_bit<<62)|(eid & EID_MASK))
#define POS_KEY(option, eid) (((option)<<63)|(eid & EID_MASK)) // Adjust mask if more bits are available

#define GET_O(x) (x>>63)
#define GET_T(x) ((x>>62) & 1)
#define GET_E(x) (x & 0x3fffffffffffffffULL)

#define IS_ASK(x) (GET_T(x) == ASK_BIT)
#define IS_BID(x) (GET_T(x) == BID_BIT)

    HashMap<uint64, QtryEventInfo, QUOTTERY_MAX_EVENT> mEventInfo;
    HashMap<uint64, Array<sint8, 8>, QUOTTERY_MAX_EVENT> mEventResult;
    // contain info about token with key id(user_id_u64_0, user_id_u64_1, user_id_u64_2, 
    
    // qtry-v2: orders array
    struct QtryOrder
    {
        id entity;
        sint64 amount;
    };
    struct CreatorInfo
    {
        id name;
        uint64 feeRate;
    };
    struct OracleInfo
    {
        id name;
        uint64 feeRate;
    };
    struct OperationParams // can be changed by operation team
    {
        HashMap<id, CreatorInfo, QUOTTERY_MAX_CREATOR_AND_COUNT> eligibleCreators;
        HashMap<id, OracleInfo, QUOTTERY_MAX_CREATOR_AND_COUNT> eligibleOracles;
        HashMap<id, uint64, 8192 * X_MULTIPLIER> discountedFeeForUsers;
        sint64 feePerDay; // in QUs
    } mOperationParams;

    // gov struct
    struct QtryGOV // votable by shareholders
    {
        uint64 mShareHolderFee;
        uint64 mBurnFee;
        uint64 mOperationFee;
        uint64 mTradeFee;
        id mOperationId;
    };
    QtryGOV mQtryGov;

    struct OrderInfo
    {
        uint64 eid;
        uint64 option; // 0 or 1
        uint64 tradeBit; // 0 ask, 1 bid
    };

    struct DetailedOrderInfo
    {
        QtryOrder qo;
        uint64 eid;
        sint64 price;
        sint64 index;
        bit justAdded;
    };

    HashMap<id, QtryOrder, QUOTTERY_MAX_NUMBER_OF_USER> mPositionInfo;
    Collection<QtryOrder, 2097152 * X_MULTIPLIER> mABOrders;

    Array<uint64, QUOTTERY_MAX_EVENT> mRecentActiveEvent;

    struct QtryRevInfo
    {
        uint64 mTotal;
        uint64 mDistributed;
    };
    QtryRevInfo mQtryRev;

    // other stats
    uint64 mCurrentEventID;
    uint64 mTotalMoneyFlow; // total money flow thru this SC, this counts some bid orders may not matched in the whole event lifetime

    // these totalRevenue are distributed in real time, no need to track "distributed" totalRevenue
    uint64 mTotalRevenue; // is sum of all revenues + burned
    struct Paylist
    {
        uint64 creatorFee;
        Array<uint64, 8> oracleFee;
    };
    HashMap<uint64, Paylist, QUOTTERY_MAX_EVENT> mPaylist; // the pay list to send after resolving event

    uint64 mShareholdersRevenue;
    uint64 mDistributedShareholdersRevenue;

    uint64 mCreatorRevenue;
    uint64 mOracleRevenue;
    uint64 mOperationRevenue;
    uint64 mBurnedAmount;


    /**************************************/
    /************UTIL FUNCTIONS************/
    /**************************************/
    inline static uint32 divUp(uint32 a, uint32 b)
    {
        return div((a + b - 1), b);
    }
    inline static uint64 divUp(uint64 a, uint64 b)
    {
        return div((a + b - 1), b);
    }
    inline static sint32 min(sint32 a, sint32 b)
    {
        return (a < b) ? a : b;
    }

    inline static sint64 min(sint64 a, sint64 b)
    {
        return (a < b) ? a : b;
    }

    struct GetEventInfo_input
    {
        uint64 eventId;
    };
    struct GetEventInfo_output
    {
        QtryEventInfo qei;
    };



    struct GetEventInfo_locals
    {
        sint64 slotId;
        uint64 baseId0, baseId1, u64Tmp;
        uint32 i0, i1;
    };
    /**
     * @param eventId
     * @return meta data of a event and its current state
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetEventInfo)
    {
        setMemory(output.qei, 0);
        state.mEventInfo.get(input.eventId, output.qei);
    }

    struct ValidateEvent_input
    {
        uint64 eventId;
    };
    struct ValidateEvent_output
    {
        sint64 isValid;
    };
    struct ValidateEvent_locals
    {
        QtryEventInfo qei;
        DateAndTime dt;
        bool status;
    };
    /*
    * @brief validate an event based on eventId
    */
    PRIVATE_FUNCTION_WITH_LOCALS(ValidateEvent)
    {
        output.isValid = 0;
        locals.status = state.mEventInfo.get(input.eventId, locals.qei);
        if (!locals.status)
        {
            return;
        }

        locals.dt = qpi.now();

        if (locals.dt < locals.qei.openDate) // now < open_date
        {
            return;
        }
        if (locals.dt > locals.qei.closeDate) // now > end_date
        {
            return;
        }
        output.isValid = 1;
        return;
    }
    public:
    static id MakeOrderKey(const uint64 eid, const uint64 option, const uint64 tradeBit)
    {
        id r;
        r.u64._0 = 0;
        r.u64._1 = 0;
        r.u64._2 = 0;
        r.u64._3 = ORDER_KEY(option, tradeBit, eid);
        return r;
    }
    static id MakePosKey(const id uid, const uint64 eid, const uint64 option)
    {
        id r = uid;
        r.u64._3 = POS_KEY(option, eid);
        return r;
    }

    static bool isOptionValid(uint64 option)
    {
        return (option == 0 || option == 1);
    }

    static bool isAmountValid(uint64 amount)
    {
        return amount > 0 && amount < QTRY_MAX_AMOUNT;
    }

    static bool isPriceValid(sint64 price)
    {
        return price > 0 && price < QTRY_MAX_AMOUNT;
    }

    struct ValidatePosition_input
    {
        id uid;
        uint64 eventId;
        uint64 option; // 0 or 1
        sint64 amount;
    };
    struct ValidatePosition_output
    {
        bit isValid;
    };
    struct ValidatePosition_locals
    {
        id key;
        QtryOrder qo;
    };
    /*
    * @brief validate a position of an event - check if user actually owns this position
    */
    PRIVATE_FUNCTION_WITH_LOCALS(ValidatePosition)
    {
        output.isValid = 0;
        locals.key = MakePosKey(input.uid, input.eventId, input.option);
        if (!state.mPositionInfo.get(locals.key, locals.qo))
        {
            // doesn't exist
            return;
        }
        if (locals.qo.amount < input.amount)
        {
            // less than owned amount
            return;
        }
        output.isValid = 1;
        return;
    }

    struct TransferPosition_input
    {
        id from;
        id to;
        OrderInfo oi;
        sint64 amount;
    };
    struct TransferPosition_output
    {
        bit ok;
    };
    struct TransferPosition_locals
    {
        id keyFrom;
        id keyTo;
        QtryOrder value;
    };

    // transfer trading position from A to B
    PRIVATE_PROCEDURE_WITH_LOCALS(TransferPosition)
    {
        output.ok = false;
        // decrease position amount FROM
        locals.keyFrom = MakePosKey(input.from, input.oi.eid, input.oi.option);
        if (!state.mPositionInfo.get(locals.keyFrom, locals.value))
        {
            return;
        }
        if (locals.value.amount < input.amount)
        {
            return;
        }

        locals.value.amount -= input.amount;
        if (locals.value.amount)
        {
            state.mPositionInfo.set(locals.keyFrom, locals.value);
        }
        else
        {
            state.mPositionInfo.removeByKey(locals.keyFrom);
        }

        locals.keyTo = MakePosKey(input.to, input.oi.eid, input.oi.option);

        if (!state.mPositionInfo.get(locals.keyTo, locals.value))
        {
            locals.value.amount = 0;
            locals.value.entity = input.to;
        }
        locals.value.amount += input.amount;
        state.mPositionInfo.set(locals.keyTo, locals.value);
    }

    struct CreatePosition_input
    {
        id uid;
        OrderInfo oi;
        sint64 amount;
    };
    struct CreatePosition_output
    {
        bit ok;
    };
    struct CreatePosition_locals
    {
        id key;
        QtryOrder value;
    };

    // create a trading position
    PRIVATE_PROCEDURE_WITH_LOCALS(CreatePosition)
    {
        output.ok = false;

        locals.key = MakePosKey(input.uid, input.oi.eid, input.oi.option);

        if (!state.mPositionInfo.get(locals.key, locals.value))
        {
            locals.value.amount = input.amount;
            locals.value.entity = input.uid;
            state.mPositionInfo.set(locals.key, locals.value);
        }
        else
        {
            // same position already exist, bug!
        }
    }

    struct UpdatePosition_input
    {
        id uid;
        OrderInfo oi;
        sint64 amountChange; // positive => increase, negative => decrease
    };
    struct UpdatePosition_output
    {
        bit ok;
    };
    struct UpdatePosition_locals
    {
        id key;
        QtryOrder value;
    };

    // Update position of an user
    PRIVATE_PROCEDURE_WITH_LOCALS(UpdatePosition)
    {
        output.ok = 0;

        locals.key = MakePosKey(input.uid, input.oi.eid, input.oi.option);

        if (!state.mPositionInfo.get(locals.key, locals.value))
        {
            if (input.amountChange >= 0)
            {
                locals.value.amount = input.amountChange;
                locals.value.entity = input.uid;
                state.mPositionInfo.set(locals.key, locals.value);
            }
            else
            {
                // bug, negative value to NULL
            }
        }
        else
        {
            locals.value.amount += input.amountChange;
            if (locals.value.amount)
            {
                state.mPositionInfo.set(locals.key, locals.value);
            }
            else if (locals.value.amount == 0)
            {
                state.mPositionInfo.removeByKey(locals.key);
            }
            else
            {
                // bug, negative value
            }
            output.ok = 1;
        }
    }

    struct RemovePosition_input
    {
        id uid;
        OrderInfo oi;
    };
    struct RemovePosition_output
    {
        bit ok;
    };
    struct RemovePosition_locals
    {
        id key;
        QtryOrder value;
    };

    // Update position of an user
    PRIVATE_PROCEDURE_WITH_LOCALS(RemovePosition)
    {
        output.ok = 0;

        locals.key = MakePosKey(input.uid, input.oi.eid, input.oi.option);

        if (!state.mPositionInfo.get(locals.key, locals.value))
        {
            // bug, position not exist
        }
        else
        {
            state.mPositionInfo.removeByKey(locals.key);
            output.ok = 1;
        }
    }

    struct RewardTransfer_input
    {
        id receiver;
        uint64 eid;
        sint64 amount;
        bit needChargeFee;
    };

    struct RewardTransfer_output
    {
        sint64 amount;
    };

    struct RewardTransfer_locals
    {
        id key;
        sint64 total, feeTotal, fee, tmp;
        sint64 eindex, index, i;
        uint64 rate, afterDiscountRate;
        Paylist pl;
        Array<id, 8> oracleList;
    };

    // transfer QUs from Qtry to "outside"
    // Qtry uses fee model that charging on the outflow
    PRIVATE_PROCEDURE_WITH_LOCALS(RewardTransfer)
    {
        // at the moment, it only handles QUs
        // in future it will also handle a stablecoin here
        locals.total = input.amount;
        locals.feeTotal = 0;
        
        if (input.needChargeFee)
        {
            locals.eindex = state.mEventInfo.getElementIndex(input.eid);
            state.mPaylist.get(input.eid, locals.pl);

            // get discounted rate for this user
            locals.rate = 0;
            if (state.mOperationParams.discountedFeeForUsers.contains(qpi.invocator()))
            {
                state.mOperationParams.discountedFeeForUsers.get(qpi.invocator(), locals.rate);
            }
            locals.afterDiscountRate = QUOTTERY_PERCENT_DENOMINATOR - locals.rate;

            if (locals.afterDiscountRate)
            {
                // shareholders
                locals.fee = div(input.amount * state.mQtryGov.mShareHolderFee * locals.afterDiscountRate, QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mShareholdersRevenue += locals.fee;

                // operation team
                locals.fee = div(input.amount * state.mQtryGov.mOperationFee * locals.afterDiscountRate, QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mOperationRevenue += locals.fee;

                // creator
                locals.key = state.mEventInfo.value(locals.eindex).creator;
                locals.index = state.mOperationParams.eligibleCreators.getElementIndex(locals.key);
                if (locals.index != NULL_INDEX)
                {
                    locals.rate = state.mOperationParams.eligibleCreators.value(locals.index).feeRate;
                    locals.fee = div(input.amount * locals.rate * locals.afterDiscountRate, QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                    locals.feeTotal += locals.fee;
                    locals.pl.creatorFee += locals.fee;
                }

                // oracles
                locals.oracleList = state.mEventInfo.value(locals.eindex).oracleId;
                for (locals.i = 0; locals.i < 8; locals.i++)
                {
                    locals.index = state.mOperationParams.eligibleOracles.getElementIndex(locals.oracleList.get(locals.i));
                    if (locals.index != NULL_INDEX)
                    {
                        locals.rate = state.mOperationParams.eligibleOracles.value(locals.index).feeRate;
                        locals.fee = div(input.amount * locals.rate * locals.afterDiscountRate, QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                        locals.tmp = locals.pl.oracleFee.get(locals.i);
                        locals.tmp += locals.fee;
                        locals.feeTotal += locals.fee;
                        locals.pl.oracleFee.set(locals.i, locals.tmp);
                    }
                }
            }
        }

        qpi.transfer(input.receiver, locals.total - locals.feeTotal);
    }

    struct MatchingOrders_input
    {
        uint64 eventId;
        uint64 justAddedIndex;
    };

    struct MatchingOrders_output
    {
        bit matched;
    };

    struct MatchingOrders_locals
    {
        /*
        * A0 => lowest amount of QUs for selling option 0
        * A1 => lowest amount of QUs for selling option 1
        * B0 => highest amount of QUs they bid for option 0
        * B1 => highest amount of QUs they bid for option 1
        */
        DetailedOrderInfo A0, A1, B0, B1;

        sint64 matchedAmount;
        sint64 matchedPrice;
        sint64 matchedPrice0;
        sint64 matchedPrice1;
        OrderInfo oi;
        QtryOrder qo;

        id key;

        RewardTransfer_input ti;
        RewardTransfer_output to;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        TransferPosition_input tpi;
        TransferPosition_output tpo;

        QuotteryLogger log;
    };

    // only invoke when recently added order is any of (A0, A1, B0, B1)
    PRIVATE_PROCEDURE_WITH_LOCALS(MatchingOrders)
    {
        /*Initialize data*/
        output.matched = 0;
        locals.key = MakeOrderKey(input.eventId, 0, ASK_BIT);
        locals.A0.index = state.mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 1, ASK_BIT);
        locals.A1.index = state.mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 0, BID_BIT);
        locals.B0.index = state.mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 1, BID_BIT);
        locals.B1.index = state.mABOrders.headIndex(locals.key);

        if (locals.A0.index != NULL_INDEX)
        {
            locals.A0.qo = state.mABOrders.element(locals.A0.index);
            locals.A0.price = -state.mABOrders.priority(locals.A0.index);
            locals.A0.justAdded = (input.justAddedIndex == locals.A0.index);
        }
        if (locals.A1.index != NULL_INDEX)
        {
            locals.A1.qo = state.mABOrders.element(locals.A1.index);
            locals.A1.price = -state.mABOrders.priority(locals.A1.index);
            locals.A1.justAdded = (input.justAddedIndex == locals.A1.index);
        }
        if (locals.B0.index != NULL_INDEX)
        {
            locals.B0.qo = state.mABOrders.element(locals.B0.index);
            locals.B0.price = state.mABOrders.priority(locals.B0.index);
            locals.B0.justAdded = (input.justAddedIndex == locals.B0.index);
        }
        if (locals.B1.index != NULL_INDEX)
        {
            locals.B1.qo = state.mABOrders.element(locals.B1.index);
            locals.B1.price = state.mABOrders.priority(locals.B1.index);
            locals.B1.justAdded = (input.justAddedIndex == locals.B1.index);
        }

        // scenario 1: traditional matching
        // Option 0: ask match with bid
        if (locals.A0.price <= locals.B0.price && locals.A0.index != NULL_INDEX && locals.B0.index != NULL_INDEX)
        {
            locals.log = QuotteryLogger{ 0, QTRY_MATCH_TYPE_0 ,0 };
            LOG_INFO(locals.log);
            // bid > ask
            if (locals.A0.justAdded)
            {
                locals.matchedPrice = locals.B0.price;
            }
            else
            {
                locals.matchedPrice = locals.A0.price;
            }
            locals.matchedAmount = min(locals.A0.qo.amount, locals.B0.qo.amount);

            locals.ti.amount = locals.matchedPrice * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A0.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            
            locals.tpi.amount = locals.matchedAmount;
            locals.tpi.oi.eid = input.eventId;
            locals.tpi.oi.option = 0;
            locals.tpi.oi.tradeBit = ASK_BIT;
            locals.tpi.from = locals.A0.qo.entity;
            locals.tpi.to = locals.B0.qo.entity;
            CALL(TransferPosition, locals.tpi, locals.tpo);

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice)
            {
                // refund <~ not charging fees
                locals.ti.amount = (locals.B0.price - locals.matchedPrice) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B0.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
            }

            output.matched = 1;
            // update order size
            locals.A0.qo.amount -= locals.matchedAmount;
            locals.B0.qo.amount -= locals.matchedAmount;
            if (locals.A0.qo.amount)
            {
                state.mABOrders.replace(locals.A0.index, locals.A0.qo);
            }
            else
            {
                state.mABOrders.remove(locals.A0.index);
            }

            // A0 maybe removed and change the index of B0
            locals.key = MakeOrderKey(input.eventId, 0, BID_BIT);
            locals.B0.index = state.mABOrders.headIndex(locals.key);

            if (locals.B0.qo.amount)
            {
                state.mABOrders.replace(locals.B0.index, locals.B0.qo);
            }
            else
            {
                state.mABOrders.remove(locals.B0.index);
            }
            return;
        }

        // Option 1: ask match with bid
        if (locals.A1.price <= locals.B1.price && locals.A1.index != NULL_INDEX && locals.B1.index != NULL_INDEX)
        {
            locals.log = QuotteryLogger{ 0, QTRY_MATCH_TYPE_1 ,0 };
            LOG_INFO(locals.log);
            // bid > ask
            if (locals.A1.justAdded)
            {
                locals.matchedPrice = locals.B1.price;
            }
            else
            {
                locals.matchedPrice = locals.A1.price;
            }
            locals.matchedAmount = min(locals.A1.qo.amount, locals.B1.qo.amount);

            locals.ti.amount = locals.matchedPrice * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A1.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);

            locals.tpi.amount = locals.matchedAmount;
            locals.tpi.oi.eid = input.eventId;
            locals.tpi.oi.option = 1;
            locals.tpi.oi.tradeBit = ASK_BIT;
            locals.tpi.from = locals.A1.qo.entity;
            locals.tpi.to = locals.B1.qo.entity;
            CALL(TransferPosition, locals.tpi, locals.tpo);

            if (locals.B1.justAdded && locals.B1.price > locals.matchedPrice)
            {
                // refund <~ not charging fees
                locals.ti.amount = (locals.B1.price - locals.matchedPrice) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
            }

            output.matched = 1;
            // update order size
            locals.A1.qo.amount -= locals.matchedAmount;
            locals.B1.qo.amount -= locals.matchedAmount;
            if (locals.A1.qo.amount)
            {
                state.mABOrders.replace(locals.A1.index, locals.A1.qo);
            }
            else
            {
                state.mABOrders.remove(locals.A1.index);
            }

            // A1 maybe removed and change the index of B1
            locals.key = MakeOrderKey(input.eventId, 1, BID_BIT);
            locals.B1.index = state.mABOrders.headIndex(locals.key);

            if (locals.B1.qo.amount)
            {
                state.mABOrders.replace(locals.B1.index, locals.B1.qo);
            }
            else
            {
                state.mABOrders.remove(locals.B1.index);
            }
            return;
        }

        // Scenario 2: MERGE - both want to sell
        // A0 and A1 want to exit => A0.price + A1.price < WHOLE_SHARE_PRICE
        if (locals.A0.price + locals.A1.price <= WHOLE_SHARE_PRICE && locals.A0.index != NULL_INDEX && locals.A1.index != NULL_INDEX)
        {
            locals.log = QuotteryLogger{ 0, QTRY_MATCH_TYPE_2 ,0 };
            LOG_INFO(locals.log);
            if (locals.A0.justAdded)
            {
                locals.matchedPrice0 = WHOLE_SHARE_PRICE - locals.A1.price;
                locals.matchedPrice1 = locals.A1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.A0.price;
                locals.matchedPrice1 = WHOLE_SHARE_PRICE - locals.A0.price;
            }
            locals.matchedAmount = min(locals.A0.qo.amount, locals.A1.qo.amount);

            locals.ti.amount = locals.matchedPrice0 * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A0.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            // update position
            locals.upi.amountChange = -locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 0;
            locals.upi.uid = locals.A0.qo.entity;
            CALL(UpdatePosition, locals.upi, locals.upo);

            locals.ti.amount = locals.matchedPrice1 * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A1.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            // update position
            locals.upi.amountChange = -locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 1;
            locals.upi.uid = locals.A1.qo.entity;
            CALL(UpdatePosition, locals.upi, locals.upo);

            output.matched = 1;

            // update order size
            locals.A0.qo.amount -= locals.matchedAmount;
            locals.A1.qo.amount -= locals.matchedAmount;
            if (locals.A0.qo.amount)
            {
                state.mABOrders.replace(locals.A0.index, locals.A0.qo);
            }
            else
            {
                state.mABOrders.remove(locals.A0.index);
            }

            // A0 maybe removed and change the index of A1
            locals.key = MakeOrderKey(input.eventId, 1, ASK_BIT);
            locals.A1.index = state.mABOrders.headIndex(locals.key);

            if (locals.A1.qo.amount)
            {
                state.mABOrders.replace(locals.A1.index, locals.A1.qo);
            }
            else
            {
                state.mABOrders.remove(locals.A1.index);
            }
            return;
        }

        // Scenario 3: MINT - both want to buy
        if (locals.B0.price + locals.B1.price >= WHOLE_SHARE_PRICE && locals.B0.index != NULL_INDEX && locals.B1.index != NULL_INDEX)
        {
            locals.log = QuotteryLogger{ 0, QTRY_MATCH_TYPE_3 ,0 };
            LOG_INFO(locals.log);
            if (locals.B0.justAdded)
            {
                locals.matchedPrice0 = WHOLE_SHARE_PRICE - locals.B1.price;
                locals.matchedPrice1 = locals.B1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.B0.price;
                locals.matchedPrice1 = WHOLE_SHARE_PRICE - locals.B0.price;
            }
            // update position
            locals.matchedAmount = min(locals.B0.qo.amount, locals.B1.qo.amount);
            locals.upi.amountChange = locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 0;
            locals.upi.uid = locals.B0.qo.entity;
            CALL(UpdatePosition, locals.upi, locals.upo);

            locals.upi.amountChange = locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 1;
            locals.upi.uid = locals.B1.qo.entity;
            CALL(UpdatePosition, locals.upi, locals.upo);

            if (locals.B1.justAdded && locals.B1.price > locals.matchedPrice1)
            {
                // refund <~ not charging fees
                locals.ti.amount = (locals.B1.price - locals.matchedPrice1) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
            }

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice0)
            {
                // refund <~ not charging fees
                locals.ti.amount = (locals.B0.price - locals.matchedPrice0) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B0.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
            }
            // update order size
            locals.B0.qo.amount -= locals.matchedAmount;
            locals.B1.qo.amount -= locals.matchedAmount;
            if (locals.B0.qo.amount)
            {
                state.mABOrders.replace(locals.B0.index, locals.B0.qo);
            }
            else
            {
                state.mABOrders.remove(locals.B0.index);
            }

            // B0 maybe removed and change the index of B1
            locals.key = MakeOrderKey(input.eventId, 1, BID_BIT);
            locals.B1.index = state.mABOrders.headIndex(locals.key);

            if (locals.B1.qo.amount)
            {
                state.mABOrders.replace(locals.B1.index, locals.B1.qo);
            }
            else
            {
                state.mABOrders.remove(locals.B1.index);
            }
            output.matched = 1;
            return;
        }
    }
    public:
    struct AddToAskOrder_input
    {
        uint64 eventId;
        uint64 option;
        uint64 amount;
        sint64 price;
    };
    struct AddToAskOrder_output
    {
        sint64 status;
    };
    struct AddToAskOrder_locals
    {
        id key;
        // temp variables
        uint64 index;
        sint64 leftAmount;
        sint64 price;
        sint64 fairPrice;
        sint64 targetPrice;
        sint64 matchedAmount;
        sint64 total;
        sint64 fee;
        bit flag;
        QtryOrder order;

        OrderInfo oi;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        ValidatePosition_input vpi;
        ValidatePosition_output vpo;

        MatchingOrders_input moi;
        MatchingOrders_output moo;

        QuotteryLogger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AddToAskOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price))
        {
            return;
        }
        locals.oi.eid = input.eventId;
        locals.oi.option = input.option;
        locals.oi.tradeBit = ASK_BIT;

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_EVENT_ID ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        locals.vpi.uid = qpi.invocator();
        locals.vpi.amount = input.amount;
        locals.vpi.eventId = input.eventId;
        locals.vpi.option = input.option;
        CALL(ValidatePosition, locals.vpi, locals.vpo);
        if (!locals.vpo.isValid)
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_POSITION, 0};
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        
        locals.key = MakeOrderKey(input.eventId, input.option, ASK_BIT);
        locals.index = state.mABOrders.headIndex(locals.key, -input.price);
        locals.flag = false;

        // if there is exact 100% same order as this (same amount, trade bit, price, eventId):
        // if same user => replace new one
        // if diff user => add new one
        // final => exit
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.mABOrders.element(locals.index);
            locals.price = -state.mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                // this means there's another unmatched order with the same price
                locals.flag = true;
                if (locals.order.entity == qpi.invocator())
                {
                    // same entity => update => exit
                    locals.order.amount += input.amount;
                    state.mABOrders.replace(locals.index, locals.order);
                    output.status = 1;
                    return;
                }
                locals.index = state.mABOrders.nextElementIndex(locals.index);
            }
            else
            {
                break;
            }
        }

        locals.order.amount = input.amount;
        locals.order.entity = qpi.invocator();
        locals.index = state.mABOrders.add(locals.key, locals.order, -input.price);

        if (locals.flag) //there is another unmatched order with the same price, no need to resolve
        {
            return;
        }
        else
        {
            locals.moi.eventId = input.eventId;
            locals.moi.justAddedIndex = locals.index;
            locals.flag = 1;
            while (locals.flag)
            {
                // matching orders until no pair of orders can be matched
                CALL(MatchingOrders, locals.moi, locals.moo);
                locals.flag = locals.moo.matched;
                if (locals.flag)
                {
                    locals.key = MakeOrderKey(input.eventId, input.option, ASK_BIT);
                    locals.moi.justAddedIndex = state.mABOrders.headIndex(locals.key, -input.price);
                }
            }
            return;
        }
    }

    struct RemoveAskOrder_input
    {
        uint64 eventId;
        uint64 option;
        uint64 amount;
        sint64 price;
    };
    struct RemoveAskOrder_output
    {
        sint64 status;
    };

    struct RemoveAskOrder_locals
    {
        id key;
        sint64 index;
        bit flag;
        QtryOrder order;
        sint64 price;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        ValidatePosition_input vpi;
        ValidatePosition_output vpo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveAskOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price))
        {
            return;
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }
        locals.vpi.uid = qpi.invocator();
        locals.vpi.amount = input.amount;
        locals.vpi.eventId = input.eventId;
        locals.vpi.option = input.option;
        CALL(ValidatePosition, locals.vpi, locals.vpo);
        if (!locals.vpo.isValid)
        {
            return;
        }

        locals.key = MakeOrderKey(input.eventId, input.option, ASK_BIT);
        locals.index = state.mABOrders.headIndex(locals.key, -input.price);
        locals.flag = 0;

        // finding and modifying order
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.mABOrders.element(locals.index);
            locals.price = -state.mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                // this means there's another unmatched order with the same price
                locals.flag = true;
                if (locals.order.entity == qpi.invocator())
                {
                    // same entity => update => exit
                    locals.order.amount -= input.amount;
                    if (locals.order.amount)
                    {
                        state.mABOrders.replace(locals.index, locals.order);
                    }
                    else
                    {
                        state.mABOrders.remove(locals.index);
                    }
                    output.status = 1;
                    return;
                }
                locals.index = state.mABOrders.nextElementIndex(locals.index);
            }
            else
            {
                break;
            }
        }
    }

    struct RemoveBidOrder_input
    {
        uint64 eventId;
        uint64 option;
        uint64 amount;
        sint64 price;
    };
    struct RemoveBidOrder_output
    {
        sint64 status;
    };

    struct RemoveBidOrder_locals
    {
        id key;
        sint64 index;
        bit flag;
        QtryOrder order;
        sint64 price;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveBidOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price))
        {
            return;
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        locals.key = MakeOrderKey(input.eventId, input.option, BID_BIT);
        locals.index = state.mABOrders.headIndex(locals.key, input.price);
        locals.flag = 0;

        // finding and modifying order
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.mABOrders.element(locals.index);
            locals.price = state.mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                // this means there's another unmatched order with the same price
                locals.flag = true;
                if (locals.order.entity == qpi.invocator())
                {
                    // same entity => update => exit
                    if (locals.order.amount >= input.amount) // only process if the user really has order >= input.amount
                    {
                        locals.order.amount -= input.amount;
                        if (locals.order.amount)
                        {
                            state.mABOrders.replace(locals.index, locals.order);
                        }
                        else
                        {
                            state.mABOrders.remove(locals.index);
                        }
                        // refund
                        locals.rti.amount = input.amount * input.price;
                        locals.rti.eid = input.eventId;
                        locals.rti.needChargeFee = 0;
                        locals.rti.receiver = qpi.invocator();
                        CALL(RewardTransfer, locals.rti, locals.rto);
                        output.status = 1;
                    }
                    return;
                }
                locals.index = state.mABOrders.nextElementIndex(locals.index);
            }
            else
            {
                break;
            }
        }
    }

    struct AddToBidOrder_input
    {
        uint64 eventId;
        uint64 option;
        uint64 amount;
        uint64 price;
    };
    struct AddToBidOrder_output
    {
        sint64 status;
    };
    struct AddToBidOrder_locals
    {
        id key;
        // temp variables
        uint64 index;
        sint64 leftAmount;
        sint64 price;
        sint64 fairPrice;
        sint64 targetPrice;
        sint64 matchedAmount;
        sint64 total;
        sint64 fee;
        bit flag;
        QtryOrder order;

        OrderInfo oi;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        ValidatePosition_input vpi;
        ValidatePosition_output vpo;

        MatchingOrders_input moi;
        MatchingOrders_output moo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AddToBidOrder)
    {
        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price))
        {
            return;
        }
        
        // verify enough amount
        if (input.amount * input.price > qpi.invocationReward())
        {
            qpi.refundIfPossible();
            return;
        }

        // return changes
        if (input.amount * input.price < qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount * input.price);
        }

        locals.oi.eid = input.eventId;
        locals.oi.option = input.option;
        locals.oi.tradeBit = BID_BIT;

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        locals.key = MakeOrderKey(input.eventId, input.option, BID_BIT);
        locals.index = state.mABOrders.headIndex(locals.key, input.price);
        locals.flag = false;

        // if there is exact 100% same order as this (same amount, trade bit, price, eventId):
        // if same user => replace new one
        // if diff user => add new one
        // final => exit
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.mABOrders.element(locals.index);
            locals.price = state.mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                // this means there's another unmatched order with the same price
                locals.flag = true;
                if (locals.order.entity == qpi.invocator())
                {
                    // same entity => update => exit
                    locals.order.amount += input.amount;
                    state.mABOrders.replace(locals.index, locals.order);
                    output.status = 1;
                    return;
                }
                locals.index = state.mABOrders.nextElementIndex(locals.index);
            }
            else
            {
                break;
            }
        }

        locals.order.amount = input.amount;
        locals.order.entity = qpi.invocator();
        locals.index = state.mABOrders.add(locals.key, locals.order, input.price);

        if (locals.flag) //there is another unmatched order with the same price, no need to resolve
        {
            return;
        }
        else
        {
            locals.moi.eventId = input.eventId;
            locals.moi.justAddedIndex = locals.index;
            locals.flag = 1;
            while (locals.flag)
            {
                // matching orders until no pair of orders can be matched
                CALL(MatchingOrders, locals.moi, locals.moo);
                locals.flag = locals.moo.matched;
                if (locals.flag)
                {
                    locals.key = MakeOrderKey(input.eventId, input.option, BID_BIT);
                    locals.moi.justAddedIndex = state.mABOrders.headIndex(locals.key, input.price);
                }
            }
            return;
        }
    }
    
    /**
    * finalize an event
    * If there are 51%+ votes agree with the same result, the event will be finalized.
    * @param eventId
    */
    struct TryFinalizeEvent_input
    {
        uint64 eventId;
        QtryEventInfo qei;
        Array<sint8, 8> result;
    };
    struct TryFinalizeEvent_output
    {
    };

    struct TryFinalizeEvent_locals
    {
        sint32 i0, i1;
        sint32 totalOP, vote0Count, vote1Count, i, threshold;
        uint64 winOption;
        sint64 index;
        uint64 winCondition;
        uint64 loseCondition;
        uint64 bid0Condition;
        uint64 bid1Condition;
        uint64 ask0Condition;
        uint64 ask1Condition;
        id key;
        QtryOrder value;
        sint64 price;
        
        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(TryFinalizeEvent)
    {
        locals.totalOP = 0;
        locals.vote0Count = 0;
        locals.vote1Count = 0;
        for (locals.i = 0; locals.i < 8; locals.i++)
        {
            if (input.qei.oracleId.get(locals.i) != NULL_ID) locals.totalOP++;
            if (input.result.get(locals.i) == 0) locals.vote0Count++;
            if (input.result.get(locals.i) == 1) locals.vote1Count++;
        }

        // not enough votes
        locals.threshold = QPI::div(locals.totalOP, 2);
        if ((locals.vote0Count <= locals.threshold) && (locals.vote1Count <= locals.threshold))
        {
            return;
        }
        if (locals.vote0Count > locals.threshold) locals.winOption = 0;
        if (locals.vote1Count > locals.threshold) locals.winOption = 1;
        // who has position == locals.winOption && eid = input.eid would receive rewards
        // delete position that has position != locals.winOption && eid = input.eid
        locals.index = 0;
        locals.winCondition = POS_KEY(locals.winOption, input.eventId);
        locals.loseCondition = POS_KEY(1ULL - locals.winOption, input.eventId);
        while (locals.index != NULL_INDEX) // TODO: maybe don't need to scan all
        {
            locals.key = state.mPositionInfo.key(locals.index);
            if (locals.key != NULL_ID)
            {
                if (locals.key.u64._3 == locals.winCondition)
                {
                    // send money to this
                    state.mPositionInfo.get(locals.key, locals.value);
                    locals.rti.amount = WHOLE_SHARE_PRICE * locals.value.amount;
                    locals.rti.eid = input.eventId;
                    locals.rti.receiver = locals.value.entity;
                    locals.rti.needChargeFee = 1;
                    CALL(RewardTransfer, locals.rti, locals.rto);
                    // remove the position
                    state.mPositionInfo.removeByIndex(locals.index);
                }
                if (locals.key.u64._3 == locals.loseCondition)
                {
                    // remove the position
                    state.mPositionInfo.removeByIndex(locals.index);
                }
            }
            locals.index = state.mPositionInfo.nextElementIndex(locals.index);
        }
        state.mPositionInfo.cleanupIfNeeded();
        // cleaning all ABOrder
        locals.index = 0;
        
        // deleting ask 0
        locals.key = MakeOrderKey(input.eventId, 0, ASK_BIT);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        // deleting ask 1
        locals.key = MakeOrderKey(input.eventId, 1, ASK_BIT);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        // deleting bid 0
        locals.key = MakeOrderKey(input.eventId, 0, BID_BIT);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.mABOrders.element(locals.index);
            locals.price = state.mABOrders.priority(locals.index);
            // refund to users
            locals.rti.receiver = locals.value.entity;
            locals.rti.amount = locals.value.amount * locals.price;
            locals.rti.eid = input.eventId;
            locals.rti.needChargeFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);
            
            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        // deleting bid 1
        locals.key = MakeOrderKey(input.eventId, 1, BID_BIT);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.mABOrders.element(locals.index);
            locals.price = state.mABOrders.priority(locals.index);
            // refund to users
            locals.rti.receiver = locals.value.entity;
            locals.rti.amount = locals.value.amount * locals.price;
            locals.rti.eid = input.eventId;
            locals.rti.needChargeFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);

            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        state.mABOrders.cleanupIfNeeded();
        state.mEventInfo.removeByKey(input.eventId);
        state.mEventInfo.cleanupIfNeeded();
    }
    /**************************************/
    /************VIEW FUNCTIONS************/
    /**************************************/
    /**
     */
    PUBLIC_FUNCTION(BasicInfo)
    {
        setMemory(output, 0);
        output.operationFee = state.mQtryGov.mOperationFee;
        output.gameOperator = state.mQtryGov.mOperationId;
        output.shareholderFee = state.mQtryGov.mShareHolderFee;
        output.burnFee = state.mQtryGov.mBurnFee;
        output.moneyFlow = state.mTotalMoneyFlow;
        output.totalRevenue = state.mTotalRevenue;
        output.shareholdersRevenue = state.mShareholdersRevenue;
        output.creatorRevenue = state.mCreatorRevenue;
        output.oracleRevenue = state.mOracleRevenue;
        output.operationRevenue = state.mOperationRevenue;
        output.nIssuedEvent = state.mCurrentEventID;
        output.burnedAmount = state.mBurnedAmount;
    }

    struct GetActiveEvent_input
    {
    };
    struct GetActiveEvent_output
    {
        sint64 count;
        Array<uint64, 128> activeId;
    };

    struct GetActiveEvent_locals
    {
        sint64 i, c;
    };
    /**
     * @return a list of active eventID.
     * (This function only returns active event ids **in the last 128**. To get more active event ids, client needs to track it themself via logging)
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetActiveEvent)
    {
        setMemory(output, 0);
        output.count = 0;
        for (locals.i = 0; locals.i < QUOTTERY_MAX_EVENT; locals.i++)
        {
            if (state.mEventInfo.contains(state.mRecentActiveEvent.get(locals.i)))
            {
                output.activeId.set(output.count++, state.mRecentActiveEvent.get(locals.i));
            }
        }
    }

    /**************************************/
    /************CORE FUNCTIONS************/
    /**************************************/
    /**
    * Create a event
    * if the provided info is failed to create a event, fund will be returned to invocator.
    * @param eventDesc (32 bytes): event description 32 bytes
    * @param optionDesc (256 bytes): option descriptions, 32 bytes for each option from 0 to 7, leave empty(zeroes) for unused memory space
    * @param oracleProviderId (256 bytes): oracle provider IDs, 32 bytes for each ID from 0 to 7, leave empty(zeroes) for unused memory space
    * @param oracleFees (32 bytes): the fee for oracle providers, 4 bytes(uint32) for each ID from 0 to 7, leave empty(zeroes) for unused memory space
    * @param closeDate (4 bytes): date in quotteryData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    * @param endDate (4 bytes): date in quotteryData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    * @param amountPerSlot (8 bytes): amount of qu per event slot
    * @param maxEventSlotPerOption (4 bytes): maximum event slot per option
    * @param numberOfOption (4 bytes): number of options(outcomes) of the event
    */
    struct CreateEvent_input
    {
        QtryEventInfo qei;
    };
    struct CreateEvent_output
    {
    };
    struct CreateEvent_locals
    {
        DateAndTime dtNow;
        uint64 duration;
        sint64 fee;
        
        QtryEventInfo qei;
        Array<sint8, 8> zeroResult;
        sint64 index;
        bit found;
        CreatorInfo ci;
        OracleInfo oi;
        QuotteryLogger log;
        sint32 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateEvent)
    {
        // only allow users to create event
        if (qpi.invocator() != qpi.originator())
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_USER ,0 };
            LOG_INFO(locals.log);
            if(qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.found = state.mOperationParams.eligibleCreators.get(qpi.invocator(), locals.ci);
        if (!locals.found)
        {
            locals.log = QuotteryLogger{ 0, QTRY_NOT_ELIGIBLE_USER ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        for (locals.i = 0; locals.i < 8; locals.i++)
        {
            if (input.qei.oracleId.get(locals.i) != NULL_ID)
            {
                locals.found = state.mOperationParams.eligibleOracles.get(input.qei.oracleId.get(locals.i), locals.oi);
                if (!locals.found)
                {
                    locals.log = QuotteryLogger{ 0, QTRY_NOT_ELIGIBLE_ORACLE ,0 };
                    LOG_INFO(locals.log);
                    if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    return;
                }
            }
        }

        locals.dtNow = qpi.now();
        if (input.qei.endDate < locals.dtNow || input.qei.closeDate < locals.dtNow || input.qei.endDate < input.qei.closeDate)
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_DATETIME ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.duration = input.qei.endDate - locals.dtNow;
        locals.duration = divUp(locals.duration, 86400000ULL); // 86400000 ms per day

        locals.fee = locals.duration * state.mOperationParams.feePerDay;

        // fee is higher than sent amount, exit
        if (locals.fee > qpi.invocationReward())
        {
            locals.log = QuotteryLogger{ 0, QTRY_INSUFFICIENT_FUND ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        
        locals.qei = input.qei;
        locals.qei.eid = state.mCurrentEventID++;
        locals.qei.openDate = locals.dtNow;
        locals.qei.creator = qpi.invocator();

        state.mEventInfo.set(locals.qei.eid, locals.qei);
        for (locals.i = 0; locals.i < 8; locals.i++)
        {
            locals.zeroResult.set(locals.i, -1);
        }
        state.mEventResult.set(locals.qei.eid, locals.zeroResult);
        

        state.mTotalRevenue += locals.fee;
        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }
        state.mRecentActiveEvent.set(mod(locals.qei.eid, QUOTTERY_MAX_EVENT), locals.qei.eid);
    }

    struct PublishResult_locals
    {
        QtryEventInfo qei;
        bit found;
        sint32 oracleIndex;
        sint32 i;
        QuotteryLogger log;
        OracleInfo oi;
        DateAndTime dtNow;

        TryFinalizeEvent_input fei;
        TryFinalizeEvent_output feo;
    };

    struct PublishResult_input
    {
        uint64 eventId;
        uint64 option;
    };
    struct PublishResult_output
    {
    };
    /**
    * Publish result of a event (Oracle Provider only)
    * After the vote is updated, it will try to finalize the event, see TryFinalizeEvent
    * @param eventId (4 bytes)
    * @param option (4 bytes): winning option
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(PublishResult)
    {
        // only allow users to publish result
        if (qpi.invocator() != qpi.originator())
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_USER,0 };
            LOG_INFO(locals.log);
            return;
        }
        if (!isOptionValid(input.option))
        {
            return;
        }

        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }
        locals.dtNow = qpi.now();
        if (locals.dtNow < locals.qei.endDate)
        {
            return;
        }

        locals.found = 0;
        for (locals.i = 0; locals.i < 8; locals.i++)
        {
            if (locals.qei.oracleId.get(locals.i) != NULL_ID && locals.qei.oracleId.get(locals.i) == qpi.invocator())
            {
                locals.found = 1;
                locals.oracleIndex = locals.i;
                break;
            }
        }

        if (!locals.found)
        {
            locals.log = QuotteryLogger{ 0, QTRY_NOT_ELIGIBLE_ORACLE ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        
        state.mEventResult.get(input.eventId, locals.fei.result);
        locals.fei.result.set(locals.oracleIndex, (uint8)(input.option));
        state.mEventResult.set(input.eventId, locals.fei.result);
        // try finalizing event
        locals.fei.qei = locals.qei;
        locals.fei.eventId = locals.qei.eid;
        CALL(TryFinalizeEvent, locals.fei, locals.feo);
    }


    struct ResolveEvent_input
    {
        uint64 eventId;
        uint64 option;
    };
    struct ResolveEvent_output
    {
    };

    struct ResolveEvent_locals
    {
        QtryEventInfo qei;
        DateAndTime dtNow;
        TryFinalizeEvent_input fei;
        TryFinalizeEvent_output feo;
        Array<sint8, 8> result;
        sint32 i;
    };
    /**
    * Resolve result for an event (Game Operator only)
    * In emergency cases (pass endDate without result from oracle), operation team can publish result and resolve the event
    * @param eventId (16 bytes)
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(ResolveEvent)
    {
        // game operator invocation only. In any case that oracle providers can't reach consensus or unable to broadcast result after a long period,
        // operation team will resolve it by publishing the result
        if (qpi.invocator() != state.mQtryGov.mOperationId)
        {
            return;
        }
        if (!isOptionValid(input.option))
        {
            return;
        }
        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }
        locals.dtNow = qpi.now();
        if (locals.dtNow < locals.qei.endDate + 24ULL * 86400000ULL) // game operator can only resolve the game 1 day after endDate
        {
            return;
        }

        locals.result = state.mEventResult.value(input.eventId);
        for (locals.i = 0; locals.i < 8; locals.i++)
        {
            locals.result.set(locals.i, (uint8)(input.option)); // set all results same
        }

        state.mEventResult.set(input.eventId, locals.result);
        
        locals.fei.eventId = input.eventId;
        CALL(TryFinalizeEvent, locals.fei, locals.feo);
    }

    struct GetOrders_input
    {
        uint64 eventId;
        uint64 option;
        uint64 isBid;
        uint64 offset;
    };
    struct GetOrders_output
    {
        struct QtryOrderWithPrice
        {
            QtryOrder qo; 
            /*id entity;
            sint64 amount;*/
            sint64 price;
        };
        Array<QtryOrderWithPrice, 256> orders;
    };
    struct GetOrders_locals
    {
        sint64 i, c, sign;
        id key;        
        GetOrders_output::QtryOrderWithPrice order;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetOrders)
    {
        setMemory(output, 0);
        if (input.option < 0 || input.option > 1) return;
        if (!state.mEventInfo.contains(input.eventId)) return;
        locals.key = MakeOrderKey(input.eventId, input.option, input.isBid);
        locals.i = state.mABOrders.headIndex(locals.key, 0);
        locals.c = 0;
        if (input.isBid) locals.sign = 1;
        else locals.sign = -1;
        while (locals.i != NULL_INDEX && locals.c < 256)
        {
            if (input.offset)
            {
                input.offset--;
            }
            else
            {
                locals.order.price = locals.sign * state.mABOrders.priority(locals.i);
                locals.order.qo = state.mABOrders.element(locals.i);
                output.orders.set(locals.c++, locals.order);
            }
            locals.i = state.mABOrders.nextElementIndex(locals.i);
        }
    }



    struct UpdateCreatorList_input
    {
        id creatorId;
        CreatorInfo ci;
        uint64 ops; // 0 remove, 1 set (add/update)
    };
    struct UpdateCreatorList_output {};
    PUBLIC_PROCEDURE(UpdateCreatorList)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;
        if (input.ci.feeRate > QUOTTERY_HARD_CAP_CREATOR_FEE || input.ci.feeRate < 0) return;
        if (input.ops == 0)
        {
            state.mOperationParams.eligibleCreators.removeByKey(input.creatorId);
            return;
        }
        else
        {
            if (input.ops == 1)
            {
                if (state.mOperationParams.eligibleCreators.population() < QUOTTERY_MAX_CREATOR_AND_COUNT)
                {
                    //TODO: log here
                    state.mOperationParams.eligibleCreators.set(input.creatorId, input.ci);
                }
            }
            else
            {
                if (state.mOperationParams.eligibleCreators.contains(input.creatorId))
                {
                    state.mOperationParams.eligibleCreators.set(input.creatorId, input.ci);
                }
            }
            return;
        }
    }

    struct UpdateOracleList_input
    {
        id oracleId;
        OracleInfo oi;
        uint64 ops; // 0 remove, 1 set (add/update)
    };
    struct UpdateOracleList_output {};

    PUBLIC_PROCEDURE(UpdateOracleList)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;
        if (input.oi.feeRate > QUOTTERY_HARD_CAP_CREATOR_FEE || input.oi.feeRate < 0) return;
        if (input.ops == 0)
        {
            state.mOperationParams.eligibleOracles.removeByKey(input.oracleId);
            return;
        }
        else
        {
            if (input.ops == 1)
            {
                if (state.mOperationParams.eligibleOracles.population() < QUOTTERY_MAX_CREATOR_AND_COUNT)
                {
                    //TODO: log here
                    state.mOperationParams.eligibleOracles.set(input.oracleId, input.oi);
                }
            }
            else
            {
                // only update if it exist
                if (state.mOperationParams.eligibleOracles.contains(input.oracleId))
                {
                    state.mOperationParams.eligibleOracles.set(input.oracleId, input.oi);
                }
            }
            return;
        }
    }

    struct UpdateFeeDiscountList_input
    {
        id userId;
        uint64 newFeeRate;
        uint64 ops; // 0 remove, 1 set (add/update)
    };
    struct UpdateFeeDiscountList_output {};

    PUBLIC_PROCEDURE(UpdateFeeDiscountList)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;
        if (input.ops == 0)
        {
            state.mOperationParams.discountedFeeForUsers.removeByKey(input.userId);
            return;
        }
        else
        {
            state.mOperationParams.discountedFeeForUsers.set(input.userId, input.newFeeRate);
            return;
        }
    }

    struct UpdateFeePerDay_input
    {
        uint64 newFee;
    };
    struct UpdateFeePerDay_output {};

    PUBLIC_PROCEDURE(UpdateFeePerDay)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;
        state.mOperationParams.feePerDay = input.newFee;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(BasicInfo, 1);
        REGISTER_USER_FUNCTION(GetEventInfo, 2);
        REGISTER_USER_FUNCTION(GetOrders, 3);
        REGISTER_USER_FUNCTION(GetActiveEvent, 4);

        REGISTER_USER_PROCEDURE(CreateEvent, 1);
        REGISTER_USER_PROCEDURE(AddToAskOrder, 2);
        REGISTER_USER_PROCEDURE(RemoveAskOrder, 3);
        REGISTER_USER_PROCEDURE(AddToBidOrder, 4);
        REGISTER_USER_PROCEDURE(RemoveBidOrder, 5);
        REGISTER_USER_PROCEDURE(PublishResult, 6);
        
        // operation team proc
        REGISTER_USER_PROCEDURE(ResolveEvent, 7);
        REGISTER_USER_PROCEDURE(UpdateCreatorList, 8);
        REGISTER_USER_PROCEDURE(UpdateOracleList, 9);
        REGISTER_USER_PROCEDURE(UpdateFeeDiscountList, 10);
        REGISTER_USER_PROCEDURE(UpdateFeePerDay, 11);
        
    }

    BEGIN_EPOCH()
    {
#ifdef NO_UEFI
        // for unit test
        state.mCurrentEventID = 0;
        state.mOperationParams.feePerDay = 1000;
        state.mOperationParams.eligibleCreators.cleanup();
        state.mOperationParams.eligibleOracles.cleanup();
        state.mOperationParams.discountedFeeForUsers.cleanup();
        setMemory(state.mQtryGov, 0);
        state.mQtryGov.mOperationId = id(0, 1, 2, 3);
#else
        if (qpi.epoch() == 999)
        {
            state.mOperationParams.feePerDay = 1000;
            state.mOperationParams.eligibleCreators.cleanup();
            state.mOperationParams.eligibleOracles.cleanup();
            state.mOperationParams.discountedFeeForUsers.cleanup();            
            setMemory(state.mQtryGov, 0);
            state.mQtryGov.mOperationId = id(0, 1, 2, 3); // set later
        }
#endif
    }

    END_EPOCH()
    {
        // all dividends are paid in real time
    }
};
