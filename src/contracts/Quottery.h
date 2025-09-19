using namespace QPI;
constexpr unsigned long long QUOTTERY_INITIAL_MAX_EVENT = 1024;
constexpr unsigned long long QUOTTERY_INITIAL_CREATOR_AND_COUNT = 1024;
constexpr unsigned long long QUOTTERY_MAX_EVENT = QUOTTERY_INITIAL_MAX_EVENT * X_MULTIPLIER;
constexpr unsigned long long QUOTTERY_MAX_CREATOR_AND_COUNT = 512 * X_MULTIPLIER;
constexpr unsigned long long QUOTTERY_MAX_ORACLE_PROVIDER = 8;
constexpr unsigned long long QUOTTERY_MAX_NUMBER_OF_USER = QUOTTERY_MAX_EVENT * 2048;

constexpr unsigned long long QUOTTERY_PERCENT_DENOMINATOR = 1000; // 1000 == 100%
constexpr unsigned long long QUOTTERY_HARD_CAP_CREATOR_FEE = 50; // 5%

constexpr unsigned long long QUOTTERY_TICK_TO_KEEP_AFTER_END = 100ULL;

// logging enum
struct QuotteryLogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    // Other data go here
    char _terminator; // Only data before "_terminator" are logged
};

struct QuotteryTradeLogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    id A, B;
    sint64 amount;
    sint64 price0;
    sint64 price1;
    uint64 eid;
    uint64 option;
    char _terminator; // Only data before "_terminator" are logged
};

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
#define QTRY_ADD_BID 12
#define QTRY_ADD_ASK 13

// macros:
#define ASK_BIT 0
#define BID_BIT 1
#define EID_MASK 0x3FFFFFFFFFFFFFFFULL // (2^62 - 1)
#define ORDER_KEY(option, trade_bit, eid) ((option<<63)|(trade_bit<<62)|(eid & EID_MASK))
#define POS_KEY(option, eid) (((option)<<63)|(eid & EID_MASK)) // Adjust mask if more bits are available
#define QTRY_MAX_AMOUNT (2000000000000LL) // 2 trillion


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
        // for percentage 1000 = 100% | 1 = 0.1%
        uint64 operationFee;
        uint64 shareholderFee;
        uint64 burnFee;
        uint64 nIssuedEvent; // number of issued event        
        uint64 creatorRevenue;
        uint64 oracleRevenue;

        uint64 operationRevenue;
        uint64 distributedOperationRevenue;

        uint64 shareholdersRevenue;
        uint64 distributedShareholdersRevenue;

        uint64 burnedAmount;
        uint64 feePerDay;
        uint64 wholeSharePriceInQU;

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
        Array<id, QUOTTERY_MAX_ORACLE_PROVIDER> oracleId;

        // payment info
        uint64 type; // 0: QUs - 1: token (must be managed by QX)
        id assetIssuer;
        uint64 assetName;
    };

    HashMap<uint64, QtryEventInfo, QUOTTERY_MAX_EVENT> mEventInfo;
    HashMap<uint64, Array<sint8, QUOTTERY_MAX_ORACLE_PROVIDER>, QUOTTERY_MAX_EVENT> mEventResult;

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
        uint64 feePerDay; // in QUs
        uint64 wholeSharePriceInQU;
    } mOperationParams;

    // gov struct
    struct QtryGOV // votable by shareholders
    {
        uint64 mShareHolderFee;
        uint64 mBurnFee;
        uint64 mOperationFee;
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

    // other stats
    uint64 mCurrentEventID;

    struct Paylist
    {
        uint64 creatorFee;
        Array<uint64, QUOTTERY_MAX_ORACLE_PROVIDER> oracleFee;
    };
    HashMap<uint64, Paylist, QUOTTERY_MAX_EVENT> mPaylist; // the pay list to send after resolving event

    uint64 mShareholdersRevenue;
    uint64 mDistributedShareholdersRevenue;
    uint64 mOperationRevenue;
    uint64 mDistributedOperationRevenue;

    uint64 mCreatorRevenue;
    uint64 mOracleRevenue;

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

    /**
     * @brief Retrieves the metadata for a specific event.
     * @param eventId The unique identifier of the event.
     * @return The QtryEventInfo struct containing the event's details.
     */
    PUBLIC_FUNCTION(GetEventInfo)
    {
        setMemory(output.qei, 0);
        state.mEventInfo.get(input.eventId, output.qei);
    }

    struct GetEventInfoBatch_input
    {
        Array<uint64, 64> eventIds;
    };
    struct GetEventInfoBatch_output
    {
        Array<QtryEventInfo, 64> aqei;
    };

    struct GetEventInfoBatch_locals
    {
        uint64 i;
        QtryEventInfo qei;
    };

    /**
     * @brief Retrieves the metadata for 64 specific events.
     * @param array of eventId 
     * @return The array of QtryEventInfo struct containing the event's details.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetEventInfoBatch)
    {
        setMemory(output.aqei, 0);
        for (locals.i = 0; locals.i < 128; locals.i++)
        {
            if (state.mEventInfo.get(input.eventIds.get(locals.i), locals.qei))
            {
                output.aqei.set(locals.i, locals.qei);
            }
        }
    }

    struct GetCreatorInfo_input
    {
        id cid;
    };
    struct GetCreatorInfo_output
    {
        uint64 isCreator;
        CreatorInfo ci;
    };

    /**
     * @brief Retrieves the metadata for a specific creator
     * @param uid of creator
     * @return The CreatorInfo struct containing the details.
     */
    PUBLIC_FUNCTION(GetCreatorInfo)
    {
        setMemory(output, 0);
        if (state.mOperationParams.eligibleCreators.get(input.cid, output.ci))
        {
            output.isCreator = 1;
        }
        else
        {
            output.isCreator = 0;
        }
    }

    struct GetCreatorList_input {};
    struct GetCreatorList_output
    {
        uint64 total;
        Array<id, QUOTTERY_MAX_CREATOR_AND_COUNT> cid;
        Array<CreatorInfo, QUOTTERY_MAX_CREATOR_AND_COUNT> aci;
    };
    struct GetCreatorList_locals
    {
        uint64 index;
        id key;
        CreatorInfo ci;
    };
    /**
     * @brief Retrieves the metadata for all creators
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetCreatorList)
    {
        setMemory(output, 0);
        locals.index = 0;
        while (locals.index != NULL_INDEX)
        {
            locals.key = state.mOperationParams.eligibleCreators.key(locals.index);
            if (locals.key != NULL_ID)
            {
                locals.ci = state.mOperationParams.eligibleCreators.value(locals.index);
                output.cid.set(output.total, locals.key);
                output.aci.set(output.total, locals.ci);
                output.total++;
            }
            locals.index = state.mOperationParams.eligibleCreators.nextElementIndex(locals.index);
        }
    }

    struct GetOracleInfo_input
    {
        id oid;
    };
    struct GetOracleInfo_output
    {
        uint64 isOracle;
        OracleInfo oi;
    };

    /**
     * @brief Retrieves the metadata for a specific oracle
     * @param uid of oracle
     * @return The OracleInfo struct containing the details.
     */
    PUBLIC_FUNCTION(GetOracleInfo)
    {
        setMemory(output, 0);
        if (state.mOperationParams.eligibleOracles.get(input.oid, output.oi))
        {
            output.isOracle = 1;
        }
        else
        {
            output.isOracle = 0;
        }
    }

    struct GetOracleList_input {};
    struct GetOracleList_output
    {
        uint64 total;
        Array<id, QUOTTERY_MAX_CREATOR_AND_COUNT> oid;
        Array<OracleInfo, QUOTTERY_MAX_CREATOR_AND_COUNT> aoi;
    };
    struct GetOracleList_locals
    {
        uint64 index;
        id key;
        OracleInfo oi;
    };
    /**
     * @brief Retrieves the metadata for all Oracles
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetOracleList)
    {
        setMemory(output, 0);
        locals.index = 0;
        while (locals.index != NULL_INDEX)
        {
            locals.key = state.mOperationParams.eligibleOracles.key(locals.index);
            if (locals.key != NULL_ID)
            {
                locals.oi = state.mOperationParams.eligibleOracles.value(locals.index);
                output.oid.set(output.total, locals.key);
                output.aoi.set(output.total, locals.oi);
                output.total++;
            }
            locals.index = state.mOperationParams.eligibleOracles.nextElementIndex(locals.index);
        }
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

    /**
     * @brief Checks if an event is valid for trading (exists and is within its trading window).
     * @param eventId The unique identifier of the event.
     * @return 1 if the event is valid for trading, 0 otherwise.
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

    /**
     * @brief Helper to construct a unique key for the order book.
     * Packs option, trade type (ask/bid), and event ID into a single uint64.
     */
    static id MakeOrderKey(const uint64 eid, const uint64 option, const uint64 tradeBit)
    {
        id r;
        r.u64._0 = 0;
        r.u64._1 = 0;
        r.u64._2 = 0;
        r.u64._3 = ORDER_KEY(option, tradeBit, eid);
        return r;
    }

    /**
     * @brief Helper to construct a unique key for a user's position.
     * Packs the user's ID with the option and event ID.
     */
    static id MakePosKey(const id uid, const uint64 eid, const uint64 option)
    {
        id r = uid;
        r.u64._3 = POS_KEY(option, eid);
        return r;
    }

    /**
     * @brief Checks if an option is valid (0 or 1).
     */
    static bool isOptionValid(uint64 option)
    {
        return (option == 0 || option == 1);
    }

    /**
     * @brief Checks if an amount is within valid contract limits.
     */
    static bool isAmountValid(uint64 amount)
    {
        return amount > 0 && amount < QTRY_MAX_AMOUNT;
    }

    /**
     * @brief Checks if a price is within valid contract limits.
     */
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

    /**
     * @brief Validates that a user owns a sufficient amount of a specific position (shares).
     * @param uid The user's ID.
     * @param eventId The event ID.
     * @param option The option (0 or 1).
     * @param amount The amount to check for.
     * @return 1 if the user's position is sufficient, 0 otherwise.
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

    /**
     * @brief Transfers a specified amount of a position from one user to another.
     * @param from The sender of the position.
     * @param to The receiver of the position.
     * @param oi OrderInfo specifying the event and option.
     * @param amount The amount of the position to transfer.
     */
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

    /**
     * @brief Modifies a user's position by a specified amount (can be positive or negative).
     * Creates the position if it does not exist for a positive change.
     * Deletes the position if the amount becomes zero.
     * @param uid The user's ID.
     * @param oi OrderInfo specifying the event and option.
     * @param amountChange The delta to apply to the user's position.
     */
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
        uint64 rate, afterDiscountRate, amountWithDiscount;
        Paylist pl;
        Array<id, QUOTTERY_MAX_ORACLE_PROVIDER> oracleList;
    };

    /**
     * @brief Handles all outgoing transfers from the contract. It calculates and deducts fees
     * for shareholders, operations, creators, and oracles before sending the net amount.
     * @param receiver The recipient of the funds.
     * @param eid The associated event ID for fee calculation.
     * @param amount The gross amount to transfer.
     * @param needChargeFee If true, fees are calculated and deducted.
     */
    PRIVATE_PROCEDURE_WITH_LOCALS(RewardTransfer)
    {
        if (input.amount <= 0 || input.amount >= MAX_AMOUNT) return;
        if (input.receiver == NULL_ID) return;
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
                locals.amountWithDiscount = smul((uint64)input.amount, locals.afterDiscountRate); // guaranteed not overflow - maxima=2*1e12*1000
                // shareholders
                locals.fee = div(smul(locals.amountWithDiscount, state.mQtryGov.mShareHolderFee), QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mShareholdersRevenue += locals.fee;

                // operation team
                locals.fee = div(smul(locals.amountWithDiscount, state.mQtryGov.mOperationFee), QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mOperationRevenue += locals.fee;

                // creator
                locals.key = state.mEventInfo.value(locals.eindex).creator;
                locals.index = state.mOperationParams.eligibleCreators.getElementIndex(locals.key);
                if (locals.index != NULL_INDEX)
                {
                    locals.rate = state.mOperationParams.eligibleCreators.value(locals.index).feeRate;
                    locals.fee = div(smul(locals.amountWithDiscount, locals.rate), QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                    locals.feeTotal += locals.fee;
                    locals.pl.creatorFee += locals.fee;
                }

                // oracles
                locals.oracleList = state.mEventInfo.value(locals.eindex).oracleId;
                for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
                {
                    locals.index = state.mOperationParams.eligibleOracles.getElementIndex(locals.oracleList.get(locals.i));
                    if (locals.index != NULL_INDEX)
                    {
                        locals.rate = state.mOperationParams.eligibleOracles.value(locals.index).feeRate;
                        locals.fee = div(smul(locals.amountWithDiscount, locals.rate), QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                        locals.tmp = locals.pl.oracleFee.get(locals.i);
                        locals.tmp += locals.fee;
                        locals.feeTotal += locals.fee;
                        locals.pl.oracleFee.set(locals.i, locals.tmp);
                    }
                }
            }
            state.mPaylist.set(input.eid, locals.pl);
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

        id key;

        RewardTransfer_input ti;
        RewardTransfer_output to;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        TransferPosition_input tpi;
        TransferPosition_output tpo;

        QuotteryTradeLogger log;
    };

    /**
     * @brief The core matching engine. It checks for all possible matches:
     * 1. Traditional Ask vs. Bid for the same option.
     * 2. MERGE: An Ask for option 0 matches an Ask for option 1, effectively liquidating both positions.
     * 3. MINT: A Bid for option 0 matches a Bid for option 1, creating new positions for both parties.
     * @param eventId The event to perform matching on.
     * @param justAddedIndex The index of the most recently added order, used for price discovery.
     */
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
            // bid > ask
            locals.matchedAmount = min(locals.A0.qo.amount, locals.B0.qo.amount);
            if (locals.A0.justAdded)
            {
                locals.matchedPrice = locals.B0.price;
            }
            else
            {
                locals.matchedPrice = locals.A0.price;
            }
            locals.log = QuotteryTradeLogger{ 0, QTRY_MATCH_TYPE_0, locals.A0.qo.entity, locals.B0.qo.entity, locals.matchedAmount, locals.matchedPrice, 0, input.eventId, 0, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = smul((uint64)locals.matchedPrice, (uint64)locals.matchedAmount);
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
                locals.ti.amount = smul((locals.B0.price - locals.matchedPrice), locals.matchedAmount);
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
            locals.matchedAmount = min(locals.A1.qo.amount, locals.B1.qo.amount);
            if (locals.A1.justAdded)
            {
                locals.matchedPrice = locals.B1.price;
            }
            else
            {
                locals.matchedPrice = locals.A1.price;
            }
            locals.log = QuotteryTradeLogger{ 0, QTRY_MATCH_TYPE_1, locals.A1.qo.entity, locals.B1.qo.entity, locals.matchedAmount, locals.matchedPrice, 0, input.eventId, 1, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = smul((uint64)locals.matchedPrice, (uint64)locals.matchedAmount);
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
                locals.ti.amount = smul((locals.B1.price - locals.matchedPrice), locals.matchedAmount);
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
        // A0 and A1 want to exit => A0.price + A1.price < state.mOperationParams.wholeSharePriceInQU
        if (locals.A0.price + locals.A1.price <= (sint64)state.mOperationParams.wholeSharePriceInQU && locals.A0.index != NULL_INDEX && locals.A1.index != NULL_INDEX)
        {
            locals.matchedAmount = min(locals.A0.qo.amount, locals.A1.qo.amount);
            if (locals.A0.justAdded)
            {
                locals.matchedPrice0 = state.mOperationParams.wholeSharePriceInQU - locals.A1.price;
                locals.matchedPrice1 = locals.A1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.A0.price;
                locals.matchedPrice1 = state.mOperationParams.wholeSharePriceInQU - locals.A0.price;
            }

            locals.log = QuotteryTradeLogger{ 0, QTRY_MATCH_TYPE_2, locals.A0.qo.entity, locals.A1.qo.entity, locals.matchedAmount, locals.matchedPrice0, locals.matchedPrice1, input.eventId, 2, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = smul(locals.matchedPrice0, locals.matchedAmount);
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

            locals.ti.amount = smul(locals.matchedPrice1, locals.matchedAmount);
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
        if (locals.B0.price + locals.B1.price >= (sint64)state.mOperationParams.wholeSharePriceInQU && locals.B0.index != NULL_INDEX && locals.B1.index != NULL_INDEX)
        {
            locals.matchedAmount = min(locals.B0.qo.amount, locals.B1.qo.amount);
            if (locals.B0.justAdded)
            {
                locals.matchedPrice0 = state.mOperationParams.wholeSharePriceInQU - locals.B1.price;
                locals.matchedPrice1 = locals.B1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.B0.price;
                locals.matchedPrice1 = state.mOperationParams.wholeSharePriceInQU - locals.B0.price;
            }
            locals.log = QuotteryTradeLogger{ 0, QTRY_MATCH_TYPE_3, locals.B0.qo.entity, locals.B1.qo.entity, locals.matchedAmount, locals.matchedPrice0, locals.matchedPrice1, input.eventId, 2, 0 };
            LOG_INFO(locals.log);
            // update position

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
                locals.ti.amount = smul((locals.B1.price - locals.matchedPrice1), locals.matchedAmount);
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
            }

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice0)
            {
                // refund <~ not charging fees
                locals.ti.amount = smul((locals.B0.price - locals.matchedPrice0), locals.matchedAmount);
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
        sint64 price;
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
        QuotteryTradeLogger tradeLog;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to place an ask (sell) order for a position they own.
     * @param eventId The event to place the order on.
     * @param option The option (0 or 1) to sell.
     * @param amount The number of shares to sell.
     * @param price The price per share.
     */
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
            LOG_WARNING(locals.log);
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
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_POSITION, 0 };
            LOG_WARNING(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.tradeLog = QuotteryTradeLogger{ 0, QTRY_ADD_ASK, qpi.invocator(), NULL_ID, (sint64)input.amount, (sint64)input.price, 0, input.eventId, input.option, 0 };
        LOG_INFO(locals.tradeLog);

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

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to remove an existing ask (sell) order from the order book.
     * @param eventId The event of the order.
     * @param option The option (0 or 1) of the order.
     * @param amount The amount to remove.
     * @param price The price of the order to remove.
     */
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
        sint64 amountToRemove;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to remove an existing bid (buy) order and get a refund of their locked funds.
     * @param eventId The event of the order.
     * @param option The option (0 or 1) of the order.
     * @param amount The amount to remove.
     * @param price The price of the order to remove.
     */
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
                    locals.amountToRemove = min(input.amount, locals.order.amount);
                    // same entity => update => exit
                    locals.order.amount -= locals.amountToRemove;
                    if (locals.order.amount)
                    {
                        state.mABOrders.replace(locals.index, locals.order);
                    }
                    else
                    {
                        state.mABOrders.remove(locals.index);
                    }
                    // refund
                    locals.rti.amount = smul(locals.amountToRemove, input.price);
                    locals.rti.eid = input.eventId;
                    locals.rti.needChargeFee = 0;
                    locals.rti.receiver = qpi.invocator();
                    CALL(RewardTransfer, locals.rti, locals.rto);
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
        sint64 price;
        uint64 totalCost;
        bit flag;
        QtryOrder order;

        OrderInfo oi;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        MatchingOrders_input moi;
        MatchingOrders_output moo;

        QuotteryTradeLogger log;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to place a bid (buy) order, locking funds to back it.
     * Triggers the matching engine after the order is placed.
     * @param eventId The event to place the order on.
     * @param option The option (0 or 1) to buy.
     * @param amount The number of shares to buy.
     * @param price The price per share.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(AddToBidOrder)
    {
        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price))
        {
            return;
        }
        locals.totalCost = smul(input.amount, input.price);
        // verify enough amount
        if (locals.totalCost > (uint64)qpi.invocationReward())
        {
            qpi.refundIfPossible();
            return;
        }

        // return changes
        if (locals.totalCost < (uint64)qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.totalCost);
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

        locals.log = QuotteryTradeLogger{ 0, QTRY_ADD_BID, qpi.invocator(), NULL_ID, (sint64)input.amount, (sint64)input.price, 0, input.eventId, input.option, 0 };
        LOG_INFO(locals.log);

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
        Array<sint8, QUOTTERY_MAX_ORACLE_PROVIDER> result;
    };
    struct TryFinalizeEvent_output
    {
    };

    struct TryFinalizeEvent_locals
    {
        sint32 totalOP, vote0Count, vote1Count, i, threshold;
        uint64 winOption;
        sint64 index;
        uint64 winCondition;
        uint64 loseCondition;
        id key;
        QtryOrder value;
        QtryEventInfo qei;
        Paylist pl;
        sint64 price;

        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };

    /**
     * @brief PRIVATE PROCEDURE
     * Attempts to finalize an event. It tallies oracle votes and, if a majority is reached,
     * pays out winners, removes losing positions, and clears all remaining orders from the book.
     * @param eventId The event to finalize.
     * @param qei The event's metadata.
     * @param result An array of oracle votes.
     */
    PRIVATE_PROCEDURE_WITH_LOCALS(TryFinalizeEvent)
    {
        locals.totalOP = 0;
        locals.vote0Count = 0;
        locals.vote1Count = 0;
        for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
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
        while (locals.index != NULL_INDEX)
        {
            locals.key = state.mPositionInfo.key(locals.index);
            if (locals.key != NULL_ID)
            {
                if (locals.key.u64._3 == locals.winCondition)
                {
                    // send money to this
                    state.mPositionInfo.get(locals.key, locals.value);
                    locals.rti.amount = smul((uint64)state.mOperationParams.wholeSharePriceInQU, (uint64)locals.value.amount);
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
            locals.rti.amount = smul(locals.value.amount, locals.price);
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
            locals.rti.amount = smul(locals.value.amount, locals.price);
            locals.rti.eid = input.eventId;
            locals.rti.needChargeFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);

            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }
        // distribute fee to creator and oracles
        state.mPaylist.get(input.eventId, locals.pl);
        state.mEventInfo.get(input.eventId, locals.qei);

        locals.rti.receiver = locals.qei.creator;
        locals.rti.amount = locals.pl.creatorFee;
        locals.rti.eid = input.eventId;
        locals.rti.needChargeFee = 0;
        CALL(RewardTransfer, locals.rti, locals.rto);

        for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
        {
            locals.key = locals.qei.oracleId.get(locals.i);
            if (locals.key != NULL_ID)
            {
                locals.rti.receiver = locals.key;
                locals.rti.amount = locals.pl.oracleFee.get(locals.i);
                locals.rti.eid = input.eventId;
                locals.rti.needChargeFee = 0;
                CALL(RewardTransfer, locals.rti, locals.rto);
            }
        }
        state.mABOrders.cleanupIfNeeded();
        state.mEventInfo.removeByKey(input.eventId);
        state.mEventInfo.cleanupIfNeeded();
    }
    /**************************************/
    /************VIEW FUNCTIONS************/
    /**************************************/

    /**
     * @brief PUBLIC VIEW FUNCTION
     * Returns a high-level overview of the contract's state and financial health.
     */
    PUBLIC_FUNCTION(BasicInfo)
    {
        setMemory(output, 0);
        output.operationFee = state.mQtryGov.mOperationFee;
        output.shareholderFee = state.mQtryGov.mShareHolderFee;
        output.burnFee = state.mQtryGov.mBurnFee;
        output.nIssuedEvent = state.mCurrentEventID;
        output.creatorRevenue = state.mCreatorRevenue;
        output.oracleRevenue = state.mOracleRevenue;

        output.operationRevenue = state.mOperationRevenue;
        output.distributedOperationRevenue = state.mDistributedOperationRevenue;

        output.shareholdersRevenue = state.mShareholdersRevenue;
        output.distributedShareholdersRevenue = state.mDistributedShareholdersRevenue;

        output.burnedAmount = state.mBurnedAmount;

        output.feePerDay = state.mOperationParams.feePerDay;
        output.wholeSharePriceInQU = state.mOperationParams.wholeSharePriceInQU;

        output.gameOperator = state.mQtryGov.mOperationId;
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
        sint64 i;
    };
    /**
     * @brief PUBLIC VIEW FUNCTION
     * Returns a list of recently active event IDs. Note: this is a limited-size
     * list for performance; clients should monitor logs for a complete list.
     * @return A list of up to 128 active event IDs.
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
        bit found;
        CreatorInfo ci;
        OracleInfo oi;
        QuotteryLogger log;
        sint32 i;
        Paylist pl;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Creates a new prediction market event. Can only be called by an eligible creator.
     * @param qei A struct containing all event details, including description, dates, and oracles.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(CreateEvent)
    {
        // only allow users to create event
        if (qpi.invocator() != qpi.originator())
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_USER ,0 };
            LOG_WARNING(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.found = state.mOperationParams.eligibleCreators.get(qpi.invocator(), locals.ci);
        if (!locals.found)
        {
            locals.log = QuotteryLogger{ 0, QTRY_NOT_ELIGIBLE_USER ,0 };
            LOG_WARNING(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
        {
            if (input.qei.oracleId.get(locals.i) != NULL_ID)
            {
                locals.found = state.mOperationParams.eligibleOracles.get(input.qei.oracleId.get(locals.i), locals.oi);
                if (!locals.found)
                {
                    locals.log = QuotteryLogger{ 0, QTRY_NOT_ELIGIBLE_ORACLE ,0 };
                    LOG_WARNING(locals.log);
                    if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    return;
                }
            }
        }

        locals.dtNow = qpi.now();
        if (input.qei.endDate < locals.dtNow || input.qei.closeDate < locals.dtNow || input.qei.endDate < input.qei.closeDate)
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_DATETIME ,0 };
            LOG_WARNING(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.duration = input.qei.endDate - locals.dtNow;
        locals.duration = divUp(locals.duration, 86400000ULL); // 86400000 ms per day

        locals.fee = smul(locals.duration, state.mOperationParams.feePerDay);

        // fee is higher than sent amount, exit
        if (locals.fee > qpi.invocationReward())
        {
            locals.log = QuotteryLogger{ 0, QTRY_INSUFFICIENT_FUND ,0 };
            LOG_WARNING(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.qei = input.qei;
        locals.qei.eid = state.mCurrentEventID++;
        locals.qei.openDate = locals.dtNow;
        locals.qei.creator = qpi.invocator();

        state.mEventInfo.set(locals.qei.eid, locals.qei);
        for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
        {
            locals.zeroResult.set(locals.i, -1);
        }
        state.mEventResult.set(locals.qei.eid, locals.zeroResult);

        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }
        state.mRecentActiveEvent.set(mod(locals.qei.eid, QUOTTERY_MAX_EVENT), locals.qei.eid);
        setMemory(locals.pl, 0);
        state.mPaylist.set(locals.qei.eid, locals.pl);
    }

    struct PublishResult_locals
    {
        QtryEventInfo qei;
        bit found;
        sint32 oracleIndex;
        sint32 i;
        QuotteryLogger log;
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
     * @brief PUBLIC PROCEDURE
     * Allows an authorized oracle to submit a result for an event. Triggers event finalization if enough votes are cast.
     * @param eventId The event to submit a result for.
     * @param option The winning option (0 or 1).
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(PublishResult)
    {
        // only allow users to publish result
        if (qpi.invocator() != qpi.originator())
        {
            locals.log = QuotteryLogger{ 0, QTRY_INVALID_USER,0 };
            LOG_WARNING(locals.log);
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
        for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
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
            LOG_WARNING(locals.log);
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
        DateAndTime dtThreshold;
        TryFinalizeEvent_input fei;
        TryFinalizeEvent_output feo;
        Array<sint8, QUOTTERY_MAX_ORACLE_PROVIDER> result;
        sint32 i;
    };
    /**
     * @brief PUBLIC PROCEDURE (Admin Only)
     * Allows the game operator to manually resolve an event after a grace period.
     * This is a fallback for cases where oracles fail to reach consensus.
     * @param eventId The event to resolve.
     * @param option The winning option to set.
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
        locals.dtThreshold = locals.qei.endDate + 24ULL * 3600000ULL;
        if (locals.dtNow < locals.dtThreshold) // game operator can only resolve the game 1 day after endDate
        {
            return;
        }

        locals.result = state.mEventResult.value(input.eventId);
        for (locals.i = 0; locals.i < QUOTTERY_MAX_ORACLE_PROVIDER; locals.i++)
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

    /**
     * @brief PUBLIC VIEW FUNCTION
     * Retrieves a paginated list of orders from the order book for a specific side of an event.
     * @param eventId The event to query.
     * @param option The option (0 or 1).
     * @param isBid 1 for bids (buy orders), 0 for asks (sell orders).
     * @param offset The number of orders to skip (for pagination).
     * @return A list of up to 256 orders with their price and amount.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetOrders)
    {
        setMemory(output, 0);
        if (input.option < 0 || input.option > 1) return;
        if (!state.mEventInfo.contains(input.eventId)) return;
        locals.key = MakeOrderKey(input.eventId, input.option, input.isBid);
        if (input.isBid)
        {
            locals.i = state.mABOrders.headIndex(locals.key);
        }
        else
        {
            locals.i = state.mABOrders.headIndex(locals.key, 0);
        }
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
    /**
     * @brief PUBLIC PROCEDURE (Admin Only)
     * Adds, updates, or removes an eligible event creator.
     * @param creatorId The ID of the creator to modify.
     * @param ci The creator's info, including fee rate.
     * @param ops 0 to remove, 1 to add/update.
     */
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
    /**
     * @brief PUBLIC PROCEDURE (Admin Only)
     * Adds, updates, or removes an eligible oracle.
     * @param oracleId The ID of the oracle to modify.
     * @param oi The oracle's info, including fee rate.
     * @param ops 0 to remove, 1 to add/update.
     */
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
    /**
     * @brief PUBLIC PROCEDURE (Admin Only)
     * Adds, updates, or removes a user from the fee discount list.
     * @param userId The ID of the user to modify.
     * @param newFeeRate The new discount percentage.
     * @param ops 0 to remove, 1 to add/update.
     */
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
    /**
     * @brief PUBLIC PROCEDURE (Admin Only)
     * Updates the daily fee required to create an event.
     * @param newFee The new fee per day in QUs.
     */
    PUBLIC_PROCEDURE(UpdateFeePerDay)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;
        state.mOperationParams.feePerDay = input.newFee;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        // FUNCTION (view)
        REGISTER_USER_FUNCTION(BasicInfo, 1);
        REGISTER_USER_FUNCTION(GetEventInfo, 2);
        REGISTER_USER_FUNCTION(GetOrders, 3);
        REGISTER_USER_FUNCTION(GetActiveEvent, 4);
        REGISTER_USER_FUNCTION(GetCreatorInfo, 5);
        REGISTER_USER_FUNCTION(GetCreatorList, 6);
        REGISTER_USER_FUNCTION(GetOracleInfo, 7);
        REGISTER_USER_FUNCTION(GetOracleList, 8);
        REGISTER_USER_FUNCTION(GetEventInfoBatch, 9);

        // PROCEDURE
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
        // TODO: reinitialize after proposal getting passed
        if (qpi.epoch() == 178)
        {
            state.mOperationParams.feePerDay = 11337;
            state.mOperationParams.eligibleCreators.cleanup();
            state.mOperationParams.eligibleOracles.cleanup();
            state.mOperationParams.discountedFeeForUsers.cleanup();
            setMemory(state.mQtryGov, 0);
            state.mQtryGov.mOperationId = ID(_M, _E, _F, _K, _Y, _F, _C, _D, _X, _D, _U, _I, _L, _C, _A, _J,
                _K, _O, _I, _K, _W, _Q, _A, _P, _E, _N, _J, _D, _U, _H, _S, _S,
                _Y, _P, _B, _R, _W, _F, _O, _T, _L, _A, _L, _I, _L, _A, _Y, _W,
                _Q, _F, _D, _S, _I, _T, _J, _E); // testnet ARB
            state.mQtryGov.mBurnFee = 1;
            state.mQtryGov.mOperationFee = 5; // 0.5%
            state.mQtryGov.mShareHolderFee = 10; // 1%
            state.mOperationParams.wholeSharePriceInQU = 100000;
            state.mRecentActiveEvent.setAll(NULL_INDEX);
        }
    }

    struct END_EPOCH_locals
    {
        uint64 payout, total, burn;
    };
    END_EPOCH_WITH_LOCALS()
    {
        // distribute to QTRY shareholders
        if ((state.mShareholdersRevenue - state.mDistributedShareholdersRevenue - state.mBurnedAmount > 676) && (state.mShareholdersRevenue > state.mDistributedShareholdersRevenue + state.mBurnedAmount))
        {
            // burn fee will be applied on shareholder revenue
            locals.total = state.mShareholdersRevenue - state.mDistributedShareholdersRevenue - state.mBurnedAmount;
            locals.burn = div(smul(locals.total, state.mQtryGov.mBurnFee), QUOTTERY_PERCENT_DENOMINATOR);
            qpi.burn(locals.burn);

            state.mBurnedAmount += locals.burn;
            locals.total = state.mShareholdersRevenue - state.mDistributedShareholdersRevenue - state.mBurnedAmount;
            locals.payout = div(locals.total, (uint64)NUMBER_OF_COMPUTORS);

            if (qpi.distributeDividends(locals.payout))
            {
                state.mDistributedShareholdersRevenue += smul(locals.payout, (uint64)NUMBER_OF_COMPUTORS);
            }
        }
        // distribute to operation team
        if (state.mOperationRevenue > state.mDistributedOperationRevenue)
        {
            locals.payout = state.mOperationRevenue - state.mDistributedOperationRevenue;
            qpi.transfer(state.mQtryGov.mOperationId, locals.payout);
            state.mDistributedOperationRevenue += locals.payout;
        }
    }
};
