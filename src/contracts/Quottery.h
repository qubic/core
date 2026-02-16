using namespace QPI;
constexpr uint64 QUOTTERY_INITIAL_MAX_EVENT = 4096;
constexpr uint64 QUOTTERY_MAX_CONCURRENT_EVENT = QUOTTERY_INITIAL_MAX_EVENT * X_MULTIPLIER; // Maximum number of concurrent events (at the same time)
constexpr uint64 QUOTTERY_MAX_NUMBER_OF_USER = QUOTTERY_MAX_CONCURRENT_EVENT * 2048;

constexpr uint64 QUOTTERY_PERCENT_DENOMINATOR = 1000; // 1000 == 100%
constexpr uint64 QUOTTERY_HARD_CAP_CREATOR_FEE = 50; // 5%

constexpr uint64 QUOTTERY_DOUBLE_BUFFER_SIZE = QUOTTERY_MAX_CONCURRENT_EVENT * 2; // bug in SC verify tool, need this to bypass

constexpr uint64 QUOTTERY_CONTRACT_ASSET_NAME = 1498567761ULL;

constexpr uint64 QUOTTERY_INVALID_DATETIME = 1;
constexpr uint64 QUOTTERY_INSUFFICIENT_FUND = 5;
constexpr uint64 QUOTTERY_INVALID_EVENT_ID = 6;
constexpr uint64 QUOTTERY_INVALID_POSITION = 7;
constexpr uint64 QUOTTERY_OUT_OF_MEMORY = 8;
constexpr uint64 QUOTTERY_CREATED_EVENT = 100001;
constexpr uint64 QUOTTERY_PUBLISH_RESULT = 100002;
constexpr uint64 QUOTTERY_RESOLVE_DISPUTE = 100003;
constexpr uint64 QUOTTERY_FINALIZE_EVENT = 100004;
constexpr uint64 QUOTTERY_UPDATED_NEW_GOV_PARAMS = 100005;
constexpr uint64 QUOTTERY_UPDATED_NEW_DISCOUNT_FEE = 100006;
constexpr uint64 QUOTTERY_UPDATED_NEW_HOSTING_FEE = 100007;
constexpr uint64 QUOTTERY_ARCHIVE_EVENT = 100008;
constexpr uint64 QUOTTERY_MATCH_TYPE_0 = 100009; // A0,B0;
constexpr uint64 QUOTTERY_MATCH_TYPE_1 = 100010; // A1,B1;
constexpr uint64 QUOTTERY_MATCH_TYPE_2 = 100011; // A0,A1;
constexpr uint64 QUOTTERY_MATCH_TYPE_3 = 100012; // B0,B1;
constexpr uint64 QUOTTERY_ADD_BID = 100013;
constexpr uint64 QUOTTERY_ADD_ASK = 100014;
constexpr uint64 QUOTTERY_ASK_BIT = 0;
constexpr uint64 QUOTTERY_BID_BIT = 1;
constexpr uint64 QUOTTERY_EID_MASK = 0x3FFFFFFFFFFFFFFFULL; // (2^62 - 1);
constexpr uint64 QUOTTERY_MAX_AMOUNT = 2000000000000LL; // 2 trillion;
constexpr uint64 QUOTTERY_RESULT_NOT_SET = -1;
constexpr uint64 QUOTTERY_RESULT_NO = 0;
constexpr uint64 QUOTTERY_RESULT_YES = 1;

struct QUOTTERY2
{
};

struct QUOTTERY : public ContractBase
{
public:
    // logging enum
    struct QuotteryLogger
    {
        uint32 _contractIndex;
        uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
        // Other data go here
        sint8 _terminator; // Only data before "_terminator" are logged
    };

    // logging with a number
    struct QuotteryLoggerWithData
    {
        uint32 _contractIndex;
        uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
        id data;
        // Other data go here
        sint8 _terminator; // Only data before "_terminator" are logged
    };

    struct QuotteryTradeLogger
    {
        uint32 _contractIndex;
        uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
        id A, B;
        uint64 eid;
        uint64 option;
        uint64 padding0;
        uint64 padding1;
        sint64 amount;
        sint64 price0;
        sint64 price1;
        sint8 _terminator; // Only data before "_terminator" are logged
    };
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
        uint64 shareholdersRevenue;
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
        DateAndTime endDate; // stop receiving result from OPs

        /*256 sint8s to describe*/
        Array<id, 4> desc;
        /*128 sint8s to describe option */
        Array<id, 2> option0Desc;
        Array<id, 2> option1Desc;
    };

    HashMap<uint64, QtryEventInfo, QUOTTERY_MAX_CONCURRENT_EVENT> mEventInfo;
    HashMap<uint64, sint8, QUOTTERY_MAX_CONCURRENT_EVENT> mEventResult; // NOT_SET: -1 , NO: 0, 1: YES: 1
    HashMap<uint64, bit, QUOTTERY_MAX_CONCURRENT_EVENT> mEventFinalFlag; // flag if the event is already finalized (result is set and pass dispute window) 

    struct DepositInfo
    {
        id pubkey;
        sint64 amount;
    };

    HashMap<uint64, DepositInfo, QUOTTERY_MAX_CONCURRENT_EVENT> mDisputeInfo;
    struct DisputeResolveInfo
    {
        Array<uint16, 1024> epochData;
        Array<sint8, 1024> voteData;
    };
    HashMap<uint64, DisputeResolveInfo, QUOTTERY_MAX_CONCURRENT_EVENT> mDisputeResolver;

    HashMap<uint64, DepositInfo, QUOTTERY_MAX_CONCURRENT_EVENT> mGODepositInfo;
    // contain info about token with key id(user_id_u64_0, user_id_u64_1, user_id_u64_2, 

    // qtry-v2: orders array
    struct QtryOrder
    {
        id entity;
        sint64 amount;
    };
    struct OperationParams // can be changed by operation team
    {
        HashMap<id, uint64, 8192 * X_MULTIPLIER> discountedFeeForUsers; // for professional market maker
        sint64 feePerDay; // in QUs
    } mOperationParams;

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

    Array<uint64, QUOTTERY_MAX_CONCURRENT_EVENT> mRecentActiveEvent;

    // other stats
    uint64 mCurrentEventID;

    uint64 mShareholdersRevenue;
    uint64 mDistributedShareholdersRevenue;

    uint64 mOperationRevenue;
    uint64 mDistributedOperationRevenue;
    uint64 mBurnedAmount;


    /**************************************/
    /************UTIL FUNCTIONS************/
    /**************************************/
protected:
    inline static uint64 orderKey(uint64 option, uint64 tradeBit, uint64 eid)
    {
        return ((option << 63) | (tradeBit << 62) | (eid & QUOTTERY_EID_MASK));
    }
    inline static uint64 posKey(uint64 option, uint64 eid)
    {
        return (((uint64)(option) << 63) | (eid & QUOTTERY_EID_MASK));
    }
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
        return amount > 0 && amount < QUOTTERY_MAX_AMOUNT;
    }

    /**
     * @brief Checks if a price is within valid contract limits.
     */
    static bool isPriceValid(sint64 price, sint64 wholeSharePrice)
    {
        return price > 0 && price <= wholeSharePrice;
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

        if (locals.dt > locals.qei.endDate) // now > end_date
        {
            return;
        }
        output.isValid = 1;
        return;
    }
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

    // Update position of an user - must call validation before calling this
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
                output.ok = 0;
                return;
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
                output.ok = 0;
                return;
            }
        }
        output.ok = 1;
    }

    struct RewardTransfer_input
    {
        id receiver;
        uint64 eid;
        sint64 amount;
        bit needsint8geFee;
    };

    struct RewardTransfer_output
    {
        bit ok;
    };

    struct RewardTransfer_locals
    {
        sint64 total, feeTotal, fee;
        sint64 eindex;
        uint64 rate, afterDiscountRate, amountWithDiscount;
    };

    /**
     * @brief Handles all outgoing transfers from the contract. It calculates and deducts fees
     * for shareholders, operations, creators, and oracles before sending the net amount.
     * @param receiver The recipient of the funds.
     * @param eid The associated event ID for fee calculation.
     * @param amount The gross amount to transfer.
     * @param needsint8geFee If true, fees are calculated and deducted.
     */
    PRIVATE_PROCEDURE_WITH_LOCALS(RewardTransfer)
    {
        output.ok = false;
        if (input.amount <= 0 || input.amount >= MAX_AMOUNT) return;
        if (input.receiver == NULL_ID) return;
        // at the moment, it only handles QUs
        // in future it will also handle a stablecoin here
        locals.total = input.amount;
        locals.feeTotal = 0;

        if (input.needsint8geFee)
        {
            locals.eindex = state.mEventInfo.getElementIndex(input.eid);

            // get discounted rate for this user
            locals.rate = 0;
            if (state.mOperationParams.discountedFeeForUsers.contains(input.receiver))
            {
                state.mOperationParams.discountedFeeForUsers.get(input.receiver, locals.rate);
            }
            locals.afterDiscountRate = QUOTTERY_PERCENT_DENOMINATOR - locals.rate;

            if (locals.afterDiscountRate)
            {
                locals.amountWithDiscount = smul((uint64)input.amount, locals.afterDiscountRate); 
                // shareholders
                locals.fee = div(smul(locals.amountWithDiscount, state.mQtryGov.mShareHolderFee), QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mShareholdersRevenue += locals.fee;

                // operation team
                locals.fee = div(smul(locals.amountWithDiscount, state.mQtryGov.mOperationFee), QUOTTERY_PERCENT_DENOMINATOR * QUOTTERY_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mOperationRevenue += locals.fee;
            }
        }

        if (qpi.transferShareOwnershipAndPossession(state.QUSD.assetName, state.QUSD.issuer, SELF, SELF, locals.total - locals.feeTotal, input.receiver) < 0)
        {
            return;
        }
        output.ok = true;
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
        QtryEventInfo qei;

        sint64 matchedAmount;
        sint64 matchedPrice;
        sint64 matchedPrice0;
        sint64 matchedPrice1;

        id key, r;

        RewardTransfer_input ti;
        RewardTransfer_output to;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        QuotteryTradeLogger log;
    };

    // only invoke when recently added order is any of (A0, A1, B0, B1)
    PRIVATE_PROCEDURE_WITH_LOCALS(MatchingOrders)
    {
        /*Initialize data*/
        output.matched = 0;
        locals.key = MakeOrderKey(input.eventId, 0, QUOTTERY_ASK_BIT, locals.r);
        locals.A0.index = state.mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_ASK_BIT, locals.r);
        locals.A1.index = state.mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 0, QUOTTERY_BID_BIT, locals.r);
        locals.B0.index = state.mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_BID_BIT, locals.r);
        locals.B1.index = state.mABOrders.headIndex(locals.key);

        state.mEventInfo.get(input.eventId, locals.qei);

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
            if (locals.A0.justAdded)
            {
                locals.matchedPrice = locals.B0.price;
            }
            else
            {
                locals.matchedPrice = locals.A0.price;
            }
            locals.matchedAmount = min(locals.A0.qo.amount, locals.B0.qo.amount);
            locals.log = QuotteryTradeLogger{ 0, QUOTTERY_MATCH_TYPE_0, locals.A0.qo.entity, locals.B0.qo.entity, input.eventId, 0, 0, 0, locals.matchedAmount, locals.matchedPrice, 0, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = locals.matchedPrice * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A0.qo.entity;
            locals.ti.needsint8geFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            // add shares to B (A is already deducted when they do ask order)
            locals.upi.uid = locals.B0.qo.entity;
            locals.upi.amountChange = locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 0;
            locals.upi.oi.tradeBit = QUOTTERY_ASK_BIT;
            CALL(UpdatePosition, locals.upi, locals.upo);

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice)
            {
                // refund <~ not sint8ging fees
                locals.ti.amount = (locals.B0.price - locals.matchedPrice) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B0.qo.entity;
                locals.ti.needsint8geFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
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
            locals.key = MakeOrderKey(input.eventId, 0, QUOTTERY_BID_BIT, locals.r);
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
            locals.log = QuotteryTradeLogger{ 0, QUOTTERY_MATCH_TYPE_1, locals.A1.qo.entity, locals.B1.qo.entity, input.eventId, 1, 0, 0, locals.matchedAmount, locals.matchedPrice, 0, 0 };
            LOG_INFO(locals.log);


            locals.ti.amount = locals.matchedPrice * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A1.qo.entity;
            locals.ti.needsint8geFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            // add shares to B (A is already deducted when they do ask order)
            locals.upi.uid = locals.B1.qo.entity;
            locals.upi.amountChange = locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 1;
            locals.upi.oi.tradeBit = QUOTTERY_ASK_BIT;
            CALL(UpdatePosition, locals.upi, locals.upo);

            if (locals.B1.justAdded && locals.B1.price > locals.matchedPrice)
            {
                // refund <~ not sint8ging fees
                locals.ti.amount = (locals.B1.price - locals.matchedPrice) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needsint8geFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
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
            locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_BID_BIT, locals.r);
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
        // A0 and A1 want to exit => A0.price + A1.price < state.wholeSharePrice
        if (locals.A0.price + locals.A1.price <= state.wholeSharePrice && locals.A0.index != NULL_INDEX && locals.A1.index != NULL_INDEX)
        {
            if (locals.A0.justAdded)
            {
                locals.matchedPrice0 = state.wholeSharePrice - locals.A1.price;
                locals.matchedPrice1 = locals.A1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.A0.price;
                locals.matchedPrice1 = state.wholeSharePrice - locals.A0.price;
            }
            locals.matchedAmount = min(locals.A0.qo.amount, locals.A1.qo.amount);
            locals.log = QuotteryTradeLogger{ 0, QUOTTERY_MATCH_TYPE_2, locals.A0.qo.entity, locals.A1.qo.entity, input.eventId, 2, 0, 0, locals.matchedAmount, locals.matchedPrice0, locals.matchedPrice1, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = locals.matchedPrice0 * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A0.qo.entity;
            locals.ti.needsint8geFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            locals.ti.amount = locals.matchedPrice1 * locals.matchedAmount;
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A1.qo.entity;
            locals.ti.needsint8geFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            // the position is already deducted when adding ask orders, no need to do anything here about position shares

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
            locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_ASK_BIT, locals.r);
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
        if (locals.B0.price + locals.B1.price >= state.wholeSharePrice && locals.B0.index != NULL_INDEX && locals.B1.index != NULL_INDEX)
        {
            if (locals.B0.justAdded)
            {
                locals.matchedPrice0 = state.wholeSharePrice - locals.B1.price;
                locals.matchedPrice1 = locals.B1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.B0.price;
                locals.matchedPrice1 = state.wholeSharePrice - locals.B0.price;
            }
            locals.matchedAmount = min(locals.B0.qo.amount, locals.B1.qo.amount);
            locals.log = QuotteryTradeLogger{ 0, QUOTTERY_MATCH_TYPE_3, locals.B0.qo.entity, locals.B1.qo.entity, input.eventId, 2, 0, 0, locals.matchedAmount, locals.matchedPrice0, locals.matchedPrice1, 0 };
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
                // refund <~ not sint8ging fees
                locals.ti.amount = (locals.B1.price - locals.matchedPrice1) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needsint8geFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
            }

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice0)
            {
                // refund <~ not sint8ging fees
                locals.ti.amount = (locals.B0.price - locals.matchedPrice0) * locals.matchedAmount;
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B0.qo.entity;
                locals.ti.needsint8geFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
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
            locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_BID_BIT, locals.r);
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
        for (locals.i = 0; locals.i < 64; locals.i++)
        {
            if (state.mEventInfo.get(input.eventIds.get(locals.i), locals.qei))
            {
                output.aqei.set(locals.i, locals.qei);
            }
        }
    }
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
        id key, r;
        // temp variables
        uint64 index;
        sint64 price;
        bit flag;
        QtryEventInfo qei;
        QtryOrder order;

        OrderInfo oi;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        ValidatePosition_input vpi;
        ValidatePosition_output vpo;

        MatchingOrders_input moi;
        MatchingOrders_output moo;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

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
        if (state.mQtryGov.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.mQtryGov.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.mQtryGov.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.mQtryGov.mAntiSpamAmount);
        }
        locals.oi.eid = input.eventId;
        locals.oi.option = input.option;
        locals.oi.tradeBit = QUOTTERY_ASK_BIT;

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            locals.log = QuotteryLogger{ 0, QUOTTERY_INVALID_EVENT_ID ,0 };
            LOG_WARNING(locals.log);
            return;
        }

        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.wholeSharePrice))
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
            locals.log = QuotteryLogger{ 0, QUOTTERY_INVALID_POSITION, 0 };
            LOG_WARNING(locals.log);
            return;
        }

        {
            // deduct share
            locals.upi.uid = qpi.invocator();
            locals.upi.amountChange = -input.amount; // Negative to reduce balance
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = input.option;
            locals.upi.oi.tradeBit = QUOTTERY_ASK_BIT;
            CALL(UpdatePosition, locals.upi, locals.upo);
        }

        locals.tradeLog = QuotteryTradeLogger{ 0, QUOTTERY_ADD_ASK, qpi.invocator(), NULL_ID, input.eventId, input.option, 0, 0, (sint64)input.amount, (sint64)input.price, 0, 0 };
        LOG_INFO(locals.tradeLog);

        locals.key = MakeOrderKey(input.eventId, input.option, QUOTTERY_ASK_BIT, locals.r);
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
                    locals.key = MakeOrderKey(input.eventId, input.option, QUOTTERY_ASK_BIT, locals.r);
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
        id key, r;
        sint64 index;
        bit flag;
        QtryEventInfo qei;
        QtryOrder order;
        sint64 price;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

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
        if (state.mQtryGov.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.mQtryGov.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.mQtryGov.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.mQtryGov.mAntiSpamAmount);
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.wholeSharePrice))
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

        locals.key = MakeOrderKey(input.eventId, input.option, QUOTTERY_ASK_BIT, locals.r);
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
                    {
                        // add the position share back to mPositionInfo array
                        locals.upi.uid = qpi.invocator();
                        locals.upi.amountChange = input.amount;
                        locals.upi.oi.eid = input.eventId;
                        locals.upi.oi.option = input.option;
                        locals.upi.oi.tradeBit = QUOTTERY_ASK_BIT;
                        CALL(UpdatePosition, locals.upi, locals.upo);
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
        id key, r;
        sint64 index;
        bit flag;
        QtryEventInfo qei;
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
        if (state.mQtryGov.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.mQtryGov.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.mQtryGov.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.mQtryGov.mAntiSpamAmount);
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.wholeSharePrice))
        {
            return;
        }

        locals.key = MakeOrderKey(input.eventId, input.option, QUOTTERY_BID_BIT, locals.r);
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
                    locals.rti.needsint8geFee = 0;
                    locals.rti.receiver = qpi.invocator();
                    CALL(RewardTransfer, locals.rti, locals.rto);
                    if (!locals.rto.ok) return;
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
        sint64 amount;
        sint64 price;
    };
    struct AddToBidOrder_output
    {
        sint64 status;
    };
    struct AddToBidOrder_locals
    {
        id key, r;
        // temp variables
        uint64 index;
        sint64 price;
        uint64 userBalance;
        uint64 totalCost;
        bit flag;
        QtryOrder order;
        QtryEventInfo qei;

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
        if (state.mQtryGov.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.mQtryGov.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.mQtryGov.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.mQtryGov.mAntiSpamAmount);
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.wholeSharePrice))
        {
            return;
        }
        locals.userBalance = qpi.numberOfShares(state.QUSD, { qpi.invocator(), SELF_INDEX }, { qpi.invocator(), SELF_INDEX });
        locals.totalCost = smul(input.amount, input.price);
        // verify enough amount
        if (locals.totalCost > locals.userBalance)
        {
            return;
        }
        if (qpi.transferShareOwnershipAndPossession(state.QUSD.assetName, state.QUSD.issuer, qpi.invocator(), qpi.invocator(), locals.totalCost, SELF) < 0)
        {
            // critical error
            return;
        }

        locals.oi.eid = input.eventId;
        locals.oi.option = input.option;
        locals.oi.tradeBit = QUOTTERY_BID_BIT;



        locals.log = QuotteryTradeLogger{ 0, QUOTTERY_ADD_BID, qpi.invocator(), NULL_ID, input.eventId, input.option, 0, 0, (sint64)input.amount, (sint64)input.price, 0, 0 };
        LOG_INFO(locals.log);

        locals.key = MakeOrderKey(input.eventId, input.option, QUOTTERY_BID_BIT, locals.r);
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
                    locals.key = MakeOrderKey(input.eventId, input.option, QUOTTERY_BID_BIT, locals.r);
                    locals.moi.justAddedIndex = state.mABOrders.headIndex(locals.key, input.price);
                }
            }
            return;
        }
    }

    struct Dispute_input
    {
        uint64 eventId;
    };
    struct Dispute_output
    {
    };

    struct Dispute_locals
    {
        sint8 result;
        DepositInfo di;
    };

    // when users not agree with operator, they can dispute
    // the event will be resolved by computors
    PUBLIC_PROCEDURE_WITH_LOCALS(Dispute)
    {
        // deposit
        if (qpi.invocationReward() != state.mQtryGov.mDepositAmount)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (!state.mEventResult.get(input.eventId, locals.result))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (locals.result == QUOTTERY_RESULT_NOT_SET) // no result yet
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (state.mDisputeInfo.contains(input.eventId)) // already being disputed by other users
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        locals.di.pubkey = qpi.invocator();
        locals.di.amount = state.mQtryGov.mDepositAmount;
        state.mDisputeInfo.set(input.eventId, locals.di);
    }

    struct UserClaimReward_input
    {
        uint64 eventId;
    };
    struct UserClaimReward_output
    {
    };

    struct UserClaimReward_locals
    {
        id key;
        QtryOrder qo;
        uint64 winCondition;
        sint8 winOption;
        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };
    
    // allow users to claim rewards before epoch ends
    PUBLIC_PROCEDURE_WITH_LOCALS(UserClaimReward)
    {
        if (qpi.invocationReward() != 1000000)
        {
            return; // no refund because of spam
        }
        state.mEventResult.get(input.eventId, locals.winOption);
        if (locals.winOption == QUOTTERY_RESULT_NOT_SET)
        {
            return; // no refund because of spam
        }

        locals.winCondition = posKey((uint64)(locals.winOption), input.eventId);
        locals.key = qpi.invocator();
        locals.key.u64._3 = locals.winCondition;
        if (state.mPositionInfo.contains(locals.key))
        {
            // return deposit
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            // payout
            state.mPositionInfo.get(locals.key, locals.qo);
            locals.rti.amount = state.wholeSharePrice * locals.qo.amount;
            locals.rti.eid = input.eventId;
            locals.rti.receiver = locals.qo.entity;
            locals.rti.needsint8geFee = 1;
            CALL(RewardTransfer, locals.rti, locals.rto);
            if (!locals.rto.ok) return;
            // remove the position
            state.mPositionInfo.removeByKey(locals.key);
        }
        state.mPositionInfo.cleanupIfNeeded();
    }

    struct GOForceClaimReward_input
    {
        uint64 eventId;
        Array<id, 16> pubkeys;
    };
    struct GOForceClaimReward_output
    {
    };

    struct GOForceClaimReward_locals
    {
        sint32 i;
        id key;
        QtryOrder qo;
        uint64 winCondition, loseCondition;
        sint8 winOption;
        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(GOForceClaimReward)
    {
        state.mEventResult.get(input.eventId, locals.winOption);
        if (locals.winOption == QUOTTERY_RESULT_NOT_SET)
        {
            return;
        }

        locals.winCondition = posKey(locals.winOption, input.eventId);
        locals.loseCondition = posKey(1ULL - locals.winOption, input.eventId);

        for (locals.i = 0; locals.i < input.pubkeys.capacity(); locals.i++)
        {
            locals.key = input.pubkeys.get(locals.i);
            if (locals.key == NULL_ID) continue;
            locals.key.u64._3 = locals.winCondition;
            if (state.mPositionInfo.contains(locals.key))
            {
                // payout
                state.mPositionInfo.get(locals.key, locals.qo);
                locals.rti.amount = state.wholeSharePrice * locals.qo.amount;
                locals.rti.eid = input.eventId;
                locals.rti.receiver = locals.qo.entity;
                locals.rti.needsint8geFee = 1;
                CALL(RewardTransfer, locals.rti, locals.rto);
                if (!locals.rto.ok) return;
                // remove the position
                state.mPositionInfo.removeByKey(locals.key);
            }

            locals.key.u64._3 = locals.loseCondition;
            if (state.mPositionInfo.contains(locals.key))
            {
                state.mPositionInfo.removeByKey(locals.key);
            }
        }
        state.mPositionInfo.cleanupIfNeeded();
    }

    struct FinalizeEvent_input
    {
        uint64 eventId;
        uint64 winOption;
    };
    struct FinalizeEvent_output
    {
    };

    struct FinalizeEvent_locals
    {
        uint64 winOption;
        sint64 index;
        id key, r;
        QtryOrder value;
        sint64 price;

        RewardTransfer_input rti;
        RewardTransfer_output rto;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        QuotteryLoggerWithData log;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeEvent)
    {
        locals.log = QuotteryLoggerWithData{ 0, QUOTTERY_FINALIZE_EVENT, id(0,0,input.eventId,input.winOption), 0 };
        LOG_INFO(locals.log);
        state.mEventFinalFlag.set(input.eventId, true);
        locals.winOption = input.winOption;
        // cleaning all ABOrder
        locals.index = 0;

        // deleting ask 0
        locals.key = MakeOrderKey(input.eventId, 0, QUOTTERY_ASK_BIT, locals.r);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.mABOrders.element(locals.index);
            {
                // return share
                locals.upi.uid = locals.value.entity;
                locals.upi.amountChange = locals.value.amount;
                locals.upi.oi.eid = input.eventId;
                locals.upi.oi.option = 0;
                locals.upi.oi.tradeBit = QUOTTERY_ASK_BIT;
                CALL(UpdatePosition, locals.upi, locals.upo);
            }
            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        // deleting ask 1
        locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_ASK_BIT, locals.r);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.mABOrders.element(locals.index);
            {
                // return share
                locals.upi.uid = locals.value.entity;
                locals.upi.amountChange = locals.value.amount;
                locals.upi.oi.eid = input.eventId;
                locals.upi.oi.option = 1;
                locals.upi.oi.tradeBit = QUOTTERY_ASK_BIT;
                CALL(UpdatePosition, locals.upi, locals.upo);
            }
            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        // deleting bid 0
        locals.key = MakeOrderKey(input.eventId, 0, QUOTTERY_BID_BIT, locals.r);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.mABOrders.element(locals.index);
            locals.price = state.mABOrders.priority(locals.index);
            // refund to users
            locals.rti.receiver = locals.value.entity;
            locals.rti.amount = locals.value.amount * locals.price;
            locals.rti.eid = input.eventId;
            locals.rti.needsint8geFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);
            if (!locals.rto.ok) return;

            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        // deleting bid 1
        locals.key = MakeOrderKey(input.eventId, 1, QUOTTERY_BID_BIT, locals.r);
        locals.index = state.mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.mABOrders.element(locals.index);
            locals.price = state.mABOrders.priority(locals.index);
            // refund to users
            locals.rti.receiver = locals.value.entity;
            locals.rti.amount = locals.value.amount * locals.price;
            locals.rti.eid = input.eventId;
            locals.rti.needsint8geFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);
            if (!locals.rto.ok) return;

            state.mABOrders.remove(locals.index);
            locals.index = state.mABOrders.headIndex(locals.key);
        }

        state.mABOrders.cleanupIfNeeded();
    }
public:
    struct ResolveDispute_input
    {
        uint64 eventId;
        sint64 vote;
    };
    struct ResolveDispute_output
    {
    };

    struct ResolveDispute_locals
    {
        sint32 i;
        sint8 result;
        sint32 voteCount; // final vote count for result
        sint32 isComputor;
        DisputeResolveInfo dri;
        sint32 voteNo, voteYes;

        sint64 disputeRewardPot;
        sint64 rewardPerComputor;
        sint64 rewardForWinner;
        id winnerId;
        sint8 GOResult;

        FinalizeEvent_input fei;
        FinalizeEvent_output feo;

        DepositInfo diDisputer;
        DepositInfo diGO;
        id pubkey;

        QuotteryLoggerWithData log;
    };
    // resolve a disputed event, can only be called by computors
    PUBLIC_PROCEDURE_WITH_LOCALS(ResolveDispute)
    {
        if (qpi.invocationReward() < 10000000)
        {
            return;
        }
        locals.isComputor = -1;
        for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
        {
            if (qpi.computor(locals.i) == qpi.invocator())
            {
                locals.isComputor = locals.i;
                break;
            }
        }
        if (locals.isComputor == -1)
        {
            return;
        }
        // if this is called by computor, return deposit
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (!state.mDisputeInfo.contains(input.eventId))
        {
            return;
        }

        if (input.vote != 0 && input.vote != 1) // invalid vote
        {
            return;
        }

        state.mDisputeResolver.get(input.eventId, locals.dri);
        locals.dri.epochData.set(locals.isComputor, qpi.epoch());
        locals.dri.voteData.set(locals.isComputor, input.vote);
        state.mDisputeResolver.set(input.eventId, locals.dri);

        locals.log = QuotteryLoggerWithData{ 0, QUOTTERY_RESOLVE_DISPUTE, id(0,input.eventId, locals.isComputor, input.vote), 0 };
        LOG_INFO(locals.log);

        locals.voteNo = 0;
        locals.voteYes = 0;
        for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
        {
            if (locals.dri.epochData.get(locals.i) == qpi.epoch())
            {
                if (locals.dri.voteData.get(locals.i) == 0) locals.voteNo++;
                else locals.voteYes++;
            }
        }
        locals.result = QUOTTERY_RESULT_NOT_SET;
        if (locals.voteNo >= QUORUM)
        {
            locals.result = QUOTTERY_RESULT_NO;
            locals.voteCount = locals.voteNo;
        }
        if (locals.voteYes >= QUORUM)
        {
            locals.result = QUOTTERY_RESULT_YES;
            locals.voteCount = locals.voteYes;
        }

        if (locals.result == QUOTTERY_RESULT_NOT_SET)
        {
            return;
        }

        // POT have: mDepositAmount x 2: from disputer and GO
        // computors takes 0.3 x POT
        // disputer takes 0.7 x POT
        state.mDisputeInfo.get(input.eventId, locals.diDisputer);
        state.mGODepositInfo.get(input.eventId, locals.diGO);
        locals.disputeRewardPot = locals.diDisputer.amount + locals.diGO.amount;

        locals.rewardPerComputor = QPI::div(QPI::div(locals.disputeRewardPot * 3LL, 10LL), sint64(locals.voteCount));

        locals.rewardForWinner = locals.disputeRewardPot - locals.rewardPerComputor * sint64(locals.voteCount);
        state.mEventResult.get(input.eventId, locals.GOResult);
        if (locals.GOResult == locals.result)
        {
            locals.winnerId = locals.diGO.pubkey;
        }
        else
        {
            locals.winnerId = locals.diDisputer.pubkey;
        }

        qpi.transfer(locals.winnerId, locals.rewardForWinner);

        for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
        {
            if (locals.dri.epochData.get(locals.i) == qpi.epoch())
            {
                if (locals.dri.voteData.get(locals.i) == locals.result)
                {
                    locals.pubkey = qpi.computor(locals.i);
                    qpi.transfer(locals.pubkey, locals.rewardPerComputor);
                }
            }
        }

        // clean array
        state.mDisputeInfo.removeByKey(input.eventId);
        state.mGODepositInfo.removeByKey(input.eventId);
        state.mDisputeResolver.removeByKey(input.eventId);

        // finalize the event
        locals.fei.eventId = input.eventId;
        locals.fei.winOption = locals.result;
        state.mEventResult.set(input.eventId, locals.result);
        CALL(FinalizeEvent, locals.fei, locals.feo);
    }

    /**
    * Try to finalize an event
    * call by GO - at least 24 hours after end date
    * the reward distribution goes thru if there is no dispute
    * @param eventId
    */
    struct TryFinalizeEvent_input
    {
        uint64 eventId;
    };
    struct TryFinalizeEvent_output
    {
    };

    struct TryFinalizeEvent_locals
    {
        uint64 winOption;
        sint8 result;
        DepositInfo di;
        QtryEventInfo qei;
        QuotteryLogger log;

        FinalizeEvent_input fei;
        FinalizeEvent_output feo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TryFinalizeEvent)
    {
        if (qpi.invocator() != state.mQtryGov.mOperationId)
        {
            return;
        }
        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (state.mDisputeInfo.contains(input.eventId))
        {
            // this event is being disputed: users not agree with result from operation team
            return;
        }
        // only finalizing after 24h 
        locals.qei.endDate.addDays(1);
        if (locals.qei.endDate > qpi.now())
        {
            return;
        }

        state.mEventResult.get(input.eventId, locals.result);
        if (locals.result == QUOTTERY_RESULT_NOT_SET)
        {
            return;
        }
        if (locals.result == QUOTTERY_RESULT_NO)
        {
            locals.winOption = 0;
        }
        else
        {
            locals.winOption = 1;
        }

        // ALL passed, no dispute, return the deposit to GO
        state.mGODepositInfo.get(input.eventId, locals.di);
        qpi.transfer(locals.di.pubkey, locals.di.amount);
        state.mGODepositInfo.removeByKey(input.eventId); // clean up

        locals.fei.eventId = input.eventId;
        locals.fei.winOption = locals.winOption;
        CALL(FinalizeEvent, locals.fei, locals.feo);
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
        output.shareholdersRevenue = state.mShareholdersRevenue;
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
        sint64 i;
    };
    /**
     * @return a list of active eventID.
     * (This function only returns active event ids **in the last 128**. To get more active event ids, client needs to track it themself via logging)
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetActiveEvent)
    {
        setMemory(output, 0);
        output.count = 0;
        for (locals.i = 0; locals.i < QUOTTERY_MAX_CONCURRENT_EVENT; locals.i++)
        {
            if (state.mEventInfo.contains(state.mRecentActiveEvent.get(locals.i)))
            {
                if (output.count < output.activeId.capacity())
                {
                    output.activeId.set(output.count++, state.mRecentActiveEvent.get(locals.i));
                }
            }
        }
    }

    struct GetUserPosition_input
    {
        id uid;
    };
    
    struct GetUserPosition_output
    {
        struct PositionInfo
        {
            uint64 eo; //packed eventId and option bit
            sint64 amount;
        };
        sint64 count;
        Array<PositionInfo, 1024> p;
    };

    struct GetUserPosition_locals
    {
        uint64 k, e;
        id key;
        QtryOrder qo;
        sint64 i;
        GetUserPosition_output::PositionInfo pi;
    };
    /**
     * @brief PUBLIC VIEW FUNCTION
     * Returns all positions of an ID
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetUserPosition)
    {
        setMemory(output, 0);
        output.count = 0;
        for (locals.i = 0; locals.i < QUOTTERY_MAX_CONCURRENT_EVENT; locals.i++)
        {
            if (output.count == output.p.capacity()) break;
            locals.e = state.mRecentActiveEvent.get(locals.i);
            locals.key = MakePosKey(input.uid, locals.e, 0);
            if (state.mPositionInfo.get(locals.key, locals.qo))
            {
                locals.pi.amount = locals.qo.amount;
                locals.pi.eo = posKey(0, locals.e);
                output.p.set(output.count++, locals.pi);
            }

            locals.key = MakePosKey(input.uid, locals.e, 1);
            if (state.mPositionInfo.get(locals.key, locals.qo))
            {
                locals.pi.amount = locals.qo.amount;
                locals.pi.eo = posKey(1, locals.e);
                output.p.set(output.count++, locals.pi);
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
    * @param closeDate (4 bytes): date in quotteryData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    * @param endDate (4 bytes): date in quotteryData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
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
        QuotteryLogger log;
        sint32 i;
        DisputeResolveInfo dri;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateEvent)
    {
        // only allow users to create event
        if (qpi.invocator() != state.mQtryGov.mOperationId)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.dtNow = qpi.now();
        if (input.qei.endDate < locals.dtNow)
        {
            locals.log = QuotteryLogger{ 0, QUOTTERY_INVALID_DATETIME ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.duration = input.qei.endDate.durationMicrosec(locals.dtNow);
        locals.duration = divUp(locals.duration, 86400000000ULL); // 86400000000 us per day

        locals.fee = locals.duration * state.mOperationParams.feePerDay;

        // fee is higher than sent amount, exit
        if (locals.fee > qpi.invocationReward())
        {
            locals.log = QuotteryLogger{ 0, QUOTTERY_INSUFFICIENT_FUND ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (state.mEventInfo.population() == QUOTTERY_MAX_CONCURRENT_EVENT)
        {
            // out of storage
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.qei = input.qei;
        locals.qei.eid = state.mCurrentEventID++;
        locals.qei.openDate = locals.dtNow;

        state.mEventInfo.set(locals.qei.eid, locals.qei);
        state.mEventResult.set(locals.qei.eid, QUOTTERY_RESULT_NOT_SET);
        state.mDisputeInfo.removeByKey(locals.qei.eid); // clean if any

        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }
        state.mRecentActiveEvent.set(mod(locals.qei.eid, QUOTTERY_MAX_CONCURRENT_EVENT), locals.qei.eid);

        for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
        {
            locals.dri.epochData.set(locals.i, 10000);
            locals.dri.voteData.set(locals.i, QUOTTERY_RESULT_NOT_SET);
        }
        state.mDisputeResolver.set(locals.qei.eid, locals.dri);

        locals.log = QuotteryLogger{ 0, QUOTTERY_CREATED_EVENT ,0 };
        LOG_INFO(locals.log);
    }

    struct PublishResult_locals
    {
        QtryEventInfo qei;
        QuotteryLoggerWithData log;
        DepositInfo di;
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
    * Publish result of a event (GO only)
    * GO is required to LOCK mDepositAmount for publishing each result
    * @param eventId (4 bytes)
    * @param option (4 bytes): winning option
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(PublishResult)
    {
        if (qpi.invocator() != state.mQtryGov.mOperationId)
        {
            return;
        }
        // deposit
        if (qpi.invocationReward() != state.mQtryGov.mDepositAmount)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (!isOptionValid(input.option))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (!state.mEventInfo.get(input.eventId, locals.qei))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (qpi.now() < locals.qei.endDate)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.di.pubkey = qpi.invocator();
        locals.di.amount = qpi.invocationReward();
        state.mGODepositInfo.set(input.eventId, locals.di);
        state.mEventResult.set(input.eventId, input.option);
        locals.log = QuotteryLoggerWithData{ 0, QUOTTERY_PUBLISH_RESULT, id(0,0,input.eventId,input.option), 0 };
        LOG_INFO(locals.log);
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
        id key, r;
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
        locals.key = MakeOrderKey(input.eventId, input.option, input.isBid, locals.r);
        if (input.isBid)
        {
            locals.i = state.mABOrders.headIndex(locals.key);
        }
        else
        {
            locals.i = state.mABOrders.headIndex(locals.key);
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

    struct UpdateFeeDiscountList_input
    {
        id userId;
        uint64 newFeeRate;
        uint64 ops; // 0 remove, 1 set (add/update)
    };
    struct UpdateFeeDiscountList_output {};
    struct UpdateFeeDiscountList_locals
    {
        QuotteryLoggerWithData log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(UpdateFeeDiscountList)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;

        locals.log = QuotteryLoggerWithData{ 0, QUOTTERY_UPDATED_NEW_DISCOUNT_FEE, input.userId, 0 };
        LOG_INFO(locals.log);

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
    struct UpdateFeePerDay_locals
    {
        QuotteryLoggerWithData log;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(UpdateFeePerDay)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.mQtryGov.mOperationId) return;
        state.mOperationParams.feePerDay = input.newFee;
        locals.log = QuotteryLoggerWithData{ 0, QUOTTERY_UPDATED_NEW_HOSTING_FEE, id(0,0,0, input.newFee), 0 };
        LOG_INFO(locals.log);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        // FUNCTION (view)
        REGISTER_USER_FUNCTION(BasicInfo, 1);
        REGISTER_USER_FUNCTION(GetEventInfo, 2);
        REGISTER_USER_FUNCTION(GetOrders, 3);
        REGISTER_USER_FUNCTION(GetActiveEvent, 4);
        REGISTER_USER_FUNCTION(GetEventInfoBatch, 5);
        REGISTER_USER_FUNCTION(GetUserPosition, 6);
        REGISTER_USER_FUNCTION(GetApprovedAmount, 7);

        REGISTER_USER_PROCEDURE(CreateEvent, 1);
        REGISTER_USER_PROCEDURE(AddToAskOrder, 2);
        REGISTER_USER_PROCEDURE(RemoveAskOrder, 3);
        REGISTER_USER_PROCEDURE(AddToBidOrder, 4);
        REGISTER_USER_PROCEDURE(RemoveBidOrder, 5);
        REGISTER_USER_PROCEDURE(PublishResult, 6);
        REGISTER_USER_PROCEDURE(TryFinalizeEvent, 7);
        REGISTER_USER_PROCEDURE(Dispute, 8);
        REGISTER_USER_PROCEDURE(ResolveDispute, 9);
        REGISTER_USER_PROCEDURE(UserClaimReward, 10);
        REGISTER_USER_PROCEDURE(GOForceClaimReward, 11);
        REGISTER_USER_PROCEDURE(TransferQUSD, 12);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 13);        

        // operation team proc
        REGISTER_USER_PROCEDURE(UpdateFeeDiscountList, 20);
        REGISTER_USER_PROCEDURE(UpdateFeePerDay, 21);

        // Shareholder proposals: use standard function/procedure indices
        REGISTER_USER_FUNCTION(GetShareholderProposalFees, 65531);
        REGISTER_USER_FUNCTION(GetShareholderProposalIndices, 65532);
        REGISTER_USER_FUNCTION(GetShareholderProposal, 65533);
        REGISTER_USER_FUNCTION(GetShareholderVotes, 65534);
        REGISTER_USER_FUNCTION(GetShareholderVotingResults, 65535);

        REGISTER_USER_PROCEDURE(SetShareholderProposal, 65534);
        REGISTER_USER_PROCEDURE(SetShareholderVotes, 65535);
    }

    typedef NoData Reinit_input;
    typedef NoData Reinit_output;
    // public but no register, can't be invoke directly
    PUBLIC_PROCEDURE(Reinit)
    {
        state.mOperationParams.feePerDay = 0;
        state.mOperationParams.discountedFeeForUsers.cleanup();
        setMemory(state.mQtryGov, 0);
        state.mQtryGov.mOperationId = ID(_C, _K, _Y, _C, _S, _P, _R, _W, _S, _T, _K, _L, _Y, _G, _O, _Y, _A, _E, _N, _P, _C, _C, _W, _J, _O, _Y, _C, _D, _U, _H, _D, _F, _N, _B, _N, _O, _S, _P, _I, _K, _O, _C, _G, _Q, _E, _X, _L, _V, _D, _H, _K, _Q, _U, _L, _G, _H);
        state.mQtryGov.mBurnFee = 0;
        state.mQtryGov.mOperationFee = 1000; // 100% - force shareholder to work and elect new operation team as soon as posible
        state.mQtryGov.mShareHolderFee = 0; // 0%
        state.mRecentActiveEvent.setAll(NULL_INDEX);

        // temp replacement for qusd
        state.QUSD.assetName = 310652322119ULL; // GARTH
        state.QUSD.issuer = ID(_G, _A, _R, _T, _H, _F, _A, _N, _X, _M, _P, _X, _M, _D, _P, _E, _Z, _F, _Q, _P, _W, _F, _P, _Y, _M, _H, _O, _A, _W, _T, _K, _I, _L, _I, _N, _C, _T, _R, _M, _V, _L, _F, _F, _V, _A, _T, _K, _V, _J, _R, _K, _E, _D, _Y, _X, _G);
        state.wholeSharePrice = 100000;
    }

    BEGIN_EPOCH()
    {
        // TODO: reinitialize after proposal getting passed
        if (qpi.epoch() == 201)
        {
            CALL(Reinit, input, output);
        }
    }

    struct END_EPOCH_locals
    {
        AssetPossessionIterator iter;
        Asset asset;
        uint64 payoutPerShare, payout, total, burn;
        sint64 index;
        id key;
        QtryOrder v;
        
        uint64 eid;
        sint8 userOption;
        QtryEventInfo qei;
        id uid;
        sint8 winOption;
        RewardTransfer_input rti;
        RewardTransfer_output rto;
        sint32 i;
        bit flag;
        QuotteryLoggerWithData log;
    };
    END_EPOCH_WITH_LOCALS()
    {
        {
            // payout all positions that have final result and then clean all finalized events
            locals.index = NULL_INDEX;
            do {
                locals.index = state.mPositionInfo.nextElementIndex(locals.index);
                if (locals.index != NULL_INDEX)
                {
                    locals.key = state.mPositionInfo.key(locals.index);
                    locals.v = state.mPositionInfo.value(locals.index);
                    locals.eid = locals.key.u64._3 & QUOTTERY_EID_MASK;
                    locals.userOption = locals.key.u64._3 >> 63;
                    state.mEventResult.get(locals.eid, locals.winOption);
                    if (locals.winOption != QUOTTERY_RESULT_NOT_SET)
                    {
                        // result is available, now checking if it's still in dispute window
                        state.mEventInfo.get(locals.eid, locals.qei);
                        // only finalizing after 24h 
                        locals.qei.endDate.addDays(1);
                        if (locals.qei.endDate < qpi.now())
                        {
                            if (!state.mEventFinalFlag.contains(locals.eid))
                            {
                                state.mEventFinalFlag.set(locals.eid, true);
                            }
                            if (locals.userOption == locals.winOption)
                            {
                                locals.rti.amount = state.wholeSharePrice * locals.v.amount;
                                locals.rti.eid = locals.eid;
                                locals.rti.receiver = locals.v.entity;
                                locals.rti.needsint8geFee = 1;
                                CALL(RewardTransfer, locals.rti, locals.rto);
                                if (!locals.rto.ok)
                                {
                                    state.mEventFinalFlag.set(locals.eid, false); // can't finalize this because failed to give reward to users
                                    continue;
                                }
                            }
                            // remove the position
                            state.mPositionInfo.removeByKey(locals.key);
                        }
                    }
                }
            } while (locals.index != NULL_INDEX);
            
            locals.index = NULL_INDEX;
            do {
                locals.index = state.mEventFinalFlag.nextElementIndex(locals.index);
                if (locals.index != NULL_INDEX)
                {
                    locals.eid = state.mEventFinalFlag.key(locals.index);
                    if (!state.mEventFinalFlag.value(locals.index)) // flag as false
                    {
                        state.mEventFinalFlag.removeByIndex(locals.index); // clean it for next epoch
                        continue;
                    }
                    locals.log = QuotteryLoggerWithData{ 0, QUOTTERY_ARCHIVE_EVENT, id(0,0, 0, locals.eid), 0 };
                    LOG_INFO(locals.log);

                    // Cleanup the result, freeing the slot in mEventResult
                    state.mEventResult.removeByKey(locals.eid);

                    // Safety cleanup just in case
                    state.mDisputeInfo.removeByKey(locals.eid);
                    state.mGODepositInfo.removeByKey(locals.eid);
                    state.mDisputeResolver.removeByKey(locals.eid);
                    state.mEventInfo.removeByKey(locals.eid);
                    state.mEventFinalFlag.removeByIndex(locals.index);
                }
            } while (locals.index != NULL_INDEX);

            state.mEventInfo.cleanup();
            state.mEventFinalFlag.cleanup();
            state.mPositionInfo.cleanup();
            state.mEventResult.cleanup();
            state.mDisputeInfo.cleanup();
            state.mGODepositInfo.cleanup();
            state.mDisputeResolver.cleanup();

        }

        // distribute to QTRY shareholders
        if ((state.mShareholdersRevenue - state.mDistributedShareholdersRevenue - state.mBurnedAmount > 676) && (state.mShareholdersRevenue > state.mDistributedShareholdersRevenue + state.mBurnedAmount))
        {
            // burn fee will be applied on shareholder revenue
            locals.total = state.mShareholdersRevenue - state.mDistributedShareholdersRevenue - state.mBurnedAmount;
            locals.burn = div(smul(locals.total, state.mQtryGov.mBurnFee), QUOTTERY_PERCENT_DENOMINATOR);
            if (locals.burn) qpi.burn(locals.burn);

            state.mBurnedAmount += locals.burn;
            locals.total = state.mShareholdersRevenue - state.mDistributedShareholdersRevenue - state.mBurnedAmount;
            locals.payoutPerShare = div(locals.total, (uint64)NUMBER_OF_COMPUTORS);

            locals.asset.assetName = QUOTTERY_CONTRACT_ASSET_NAME;
            locals.asset.issuer = NULL_ID;

            if (locals.payoutPerShare)
            {
                // distribute dividends by tokens, not QUs
                for (locals.iter.begin(locals.asset); !locals.iter.reachedEnd(); locals.iter.next())
                {
                    if (locals.iter.numberOfPossessedShares() > 0)
                    {
                        locals.payout = smul(locals.payoutPerShare, (uint64)locals.iter.numberOfPossessedShares());
                        if (qpi.transferShareOwnershipAndPossession(state.QUSD.assetName, state.QUSD.issuer, SELF, SELF, locals.payout, locals.iter.possessor()) < 0)
                        {
                            //critical error
                        }
                        else
                        {
                            state.mDistributedShareholdersRevenue += locals.payout;
                        }
                    }
                }
            }
        }
        // distribute to operation team
        if (state.mOperationRevenue > state.mDistributedOperationRevenue)
        {
            locals.payout = state.mOperationRevenue - state.mDistributedOperationRevenue;
            if (qpi.transferShareOwnershipAndPossession(state.QUSD.assetName, state.QUSD.issuer, SELF, SELF, locals.payout, state.mQtryGov.mOperationId) < 0)
            {
                //critical error
            }
            else
            {
                state.mDistributedOperationRevenue += locals.payout;
            }
        }

        CALL(FinalizeGovProposal, input, output);
    }

    /* GOV/PROPOSAL AREA */

    // gov struct
    struct QtryGOV // votable by shareholders
    {
        uint64 mShareHolderFee;
        uint64 mBurnFee;
        uint64 mOperationFee;
        sint64 mAntiSpamAmount;
        sint64 mDepositAmount;
        id mOperationId;
        bool isValid()
        {
            return mShareHolderFee < QUOTTERY_PERCENT_DENOMINATOR &&
                mBurnFee < QUOTTERY_PERCENT_DENOMINATOR &&
                mOperationFee < QUOTTERY_PERCENT_DENOMINATOR &&
                (mShareHolderFee + mBurnFee + mOperationFee) < QUOTTERY_PERCENT_DENOMINATOR &&
                mOperationId != NULL_ID;
        }
    };
    QtryGOV mQtryGov;

    typedef ProposalDataYesNo ProposalDataT;
    typedef ProposalAndVotingByShareholders<8, QUOTTERY_CONTRACT_ASSET_NAME> ProposersAndVotersT;
    // Proposal and voting storage type
    typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT;
    // Proposal storage
    ProposalVotingT proposals;
    // MultiVariables proposal option data storage (same number of slots as proposals)
    Array<QtryGOV, 8> multiVariablesProposalData;

    struct SetShareholderProposal_input
    {
        ProposalDataT proposalData;
        QtryGOV govStruct;
    };
    // typedef QPI::SET_SHAREHOLDER_PROPOSAL_output SetShareholderProposal_output; <~ SC verify tool not allow this
    typedef uint16 SetShareholderProposal_output;

    // Procedures:

    PUBLIC_PROCEDURE(SetShareholderProposal)
    {
        if (ProposalTypes::cls(input.proposalData.type) != ProposalTypes::Class::MultiVariables) // not support
        {
            return;
        }

        // default return code: failure
        output = INVALID_PROPOSAL_INDEX;

        // check input
        if (!input.govStruct.isValid()) return;

        // Try to set proposal (checks invocator's rights and general validity of input proposal), returns proposal index
        output = qpi(state.proposals).setProposal(qpi.invocator(), input.proposalData);

        if (output != INVALID_PROPOSAL_INDEX)
        {
            // success
            if (ProposalTypes::cls(input.proposalData.type) == ProposalTypes::Class::MultiVariables)
            {
                // store custom data of multi-variable proposal in array (at position proposalIdx)
                state.multiVariablesProposalData.set(output, input.govStruct);
            }
        }
    }

    IMPLEMENT_SetShareholderVotes()

    IMPLEMENT_SET_SHAREHOLDER_PROPOSAL();
    IMPLEMENT_SET_SHAREHOLDER_VOTES();

    typedef NoData FinalizeGovProposal_input;
    typedef NoData FinalizeGovProposal_output;
    struct FinalizeGovProposal_locals
    {
        sint32 proposalIndex;
        ProposalDataT proposal;
        ProposalSummarizedVotingDataV1 results;
        QtryGOV newQtryGovParams;
        QuotteryLogger log;
    };
    PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeGovProposal)
    {
        locals.proposalIndex = -1;
        while ((locals.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.proposalIndex, qpi.epoch())) >= 0)
        {
            if (!qpi(state.proposals).getProposal(locals.proposalIndex, locals.proposal))
                continue;

            // handle MultiVariables proposal type
            if (ProposalTypes::cls(locals.proposal.type) == ProposalTypes::Class::MultiVariables)
            {
                // Get voting results and check if conditions for proposal acceptance are met
                if (!qpi(state.proposals).getVotingSummary(locals.proposalIndex, locals.results))
                    continue;

                // Check if the yes option (1) has been accepted
                if (locals.results.getAcceptedOption() == 1)
                {
                    state.mQtryGov = state.multiVariablesProposalData.get(locals.proposalIndex);
                    locals.log = QuotteryLogger{ 0, QUOTTERY_UPDATED_NEW_GOV_PARAMS ,0 };
                    LOG_INFO(locals.log);
                }
            }
        }
    }
    // FUNCTIONs:

    IMPLEMENT_GetShareholderProposalIndices()

    IMPLEMENT_GetShareholderProposalFees(0)

    IMPLEMENT_GetShareholderVotes()

    IMPLEMENT_GetShareholderVotingResults()

    struct GetShareholderProposal_input
    {
        uint16 proposalIndex;
    };
    struct GetShareholderProposal_output
    {
        ProposalDataT proposal;
        id proposerPubicKey;
        QtryGOV newQtryGovParams;
    };

    PUBLIC_FUNCTION(GetShareholderProposal)
    {
        // On error, output.proposal.type is set to 0
        output.proposerPubicKey = qpi(state.proposals).proposerId(input.proposalIndex);
        qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
        if (ProposalTypes::cls(output.proposal.type) == ProposalTypes::Class::MultiVariables)
        {
            output.newQtryGovParams = state.multiVariablesProposalData.get(input.proposalIndex);
        }
    }

    // QUSD Information
    // at the moment we use temp token: XXX
    Asset QUSD;
    sint64 wholeSharePrice;

    struct GetApprovedAmount_input
    {
        id pk;
    };
    struct GetApprovedAmount_output
    {
        uint64 amount;
    };
    PUBLIC_FUNCTION(GetApprovedAmount)
    {
        output.amount = qpi.numberOfShares(
            state.QUSD,
            { input.pk, SELF_INDEX },
            { input.pk, SELF_INDEX }
        );
    }

    struct TransferQUSD_input
    {
        id receiver;
        sint64 amount;
    };
    struct TransferQUSD_output
    {
        uint64 amount;
    };
    PUBLIC_PROCEDURE(TransferQUSD) // that is managed by this SC
    {
        if (qpi.transferShareOwnershipAndPossession(state.QUSD.assetName, state.QUSD.issuer, qpi.invocator(), qpi.invocator(), input.amount, input.receiver) < 0)
        {
            output.amount = -1;
        }
        else
        {
            output.amount = input.amount;
        }
    }

    // copy from QSWAP: use this to transfer management right back to wherever SC users want

    struct TransferShareManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
        uint32 newManagingContractIndex;
    };
    struct TransferShareManagementRights_output
    {
        sint64 transferredNumberOfShares;
    };

    struct TransferShareManagementRights_locals
    {
        sint64 result;
        sint64 reward;
        sint64 refundAmount;
        sint64 requiredFee;
        bit success;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
    {
        locals.reward = qpi.invocationReward();
        locals.refundAmount = locals.reward;

        output.transferredNumberOfShares = 0;

        locals.success = false;

        if (qpi.numberOfPossessedShares(
            input.asset.assetName,
            input.asset.issuer,
            qpi.invocator(),
            qpi.invocator(),
            SELF_INDEX,
            SELF_INDEX) >= input.numberOfShares)
        {
            locals.result = qpi.releaseShares(
                input.asset,
                qpi.invocator(),
                qpi.invocator(),
                input.numberOfShares,
                input.newManagingContractIndex,
                input.newManagingContractIndex,
                locals.reward
            );

            if (locals.result != INVALID_AMOUNT && locals.result >= 0)
            {
                locals.success = true;
                locals.refundAmount = locals.reward - locals.result;
            }
        }

        if (locals.success)
        {
            output.transferredNumberOfShares = input.numberOfShares;
        }

        if (locals.refundAmount > 0)
        {
            qpi.transfer(qpi.invocator(), locals.refundAmount);
        }
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }
};

