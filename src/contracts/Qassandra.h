using namespace QPI;
// Qassandra scope: QUBIC/USD forecast markets and ecosystem milestone markets.
// TODO: Add oracle-first settlement in a later commit; this scaffold keeps the current operator/dispute flow.
constexpr uint64 QASSANDRA_INITIAL_MAX_EVENT = 4096;
constexpr uint64 QASSANDRA_MAX_CONCURRENT_EVENT = QASSANDRA_INITIAL_MAX_EVENT * X_MULTIPLIER; // Maximum number of concurrent forecast markets (at the same time)
constexpr uint64 QASSANDRA_MAX_NUMBER_OF_USER = QASSANDRA_MAX_CONCURRENT_EVENT * 2048;

constexpr uint64 QASSANDRA_PERCENT_DENOMINATOR = 1000; // 1000 == 100%
constexpr uint64 QASSANDRA_HARD_CAP_CREATOR_FEE = 50; // 5%

constexpr uint64 QASSANDRA_DOUBLE_BUFFER_SIZE = QASSANDRA_MAX_CONCURRENT_EVENT * 2; // bug in SC verify tool, need this to bypass

constexpr uint64 QASSANDRA_CONTRACT_ASSET_NAME = 1095910481ULL;
constexpr uint64 QASSANDRA_GOV_ASSET_NAME = 24294015454299217ULL;

constexpr uint8 QASSANDRA_MARKET_TYPE_GENERIC = 0;
constexpr uint8 QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD = 1;
constexpr uint8 QASSANDRA_MARKET_TYPE_ECOSYSTEM_MILESTONE = 2;

constexpr uint8 QASSANDRA_COMPARISON_UNSPECIFIED = 0;
constexpr uint8 QASSANDRA_COMPARISON_GTE = 1;
constexpr uint8 QASSANDRA_COMPARISON_LTE = 2;

constexpr uint64 QASSANDRA_QUBIC_USD_PRICE_SCALE = 1000000000ULL;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_NONE = 0;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_PENDING = 1;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_SUCCESS = 2;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_TIMEOUT = 3;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_UNRESOLVABLE = 4;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_INVALID_REPLY = 5;
constexpr uint8 QASSANDRA_ORACLE_SETTLEMENT_QUERY_ERROR = 6;

constexpr uint32 QASSANDRA_DISPUTE_WINDOW = 1000;
constexpr uint64 QASSANDRA_INVALID_DATETIME = 1;
constexpr uint64 QASSANDRA_INSUFFICIENT_FUND = 5;
constexpr uint64 QASSANDRA_INVALID_EVENT_ID = 6;
constexpr uint64 QASSANDRA_INVALID_POSITION = 7;
constexpr uint64 QASSANDRA_OUT_OF_MEMORY = 8;
constexpr uint64 QASSANDRA_CREATED_EVENT = 100001;
constexpr uint64 QASSANDRA_PUBLISH_RESULT = 100002;
constexpr uint64 QASSANDRA_RESOLVE_DISPUTE = 100003;
constexpr uint64 QASSANDRA_FINALIZE_EVENT = 100004;
constexpr uint64 QASSANDRA_UPDATED_NEW_GOV_PARAMS = 100005;
constexpr uint64 QASSANDRA_UPDATED_NEW_DISCOUNT_FEE = 100006;
constexpr uint64 QASSANDRA_UPDATED_NEW_HOSTING_FEE = 100007;
constexpr uint64 QASSANDRA_ARCHIVE_EVENT = 100008;
constexpr uint64 QASSANDRA_MATCH_TYPE_0 = 100009; // A0,B0;
constexpr uint64 QASSANDRA_MATCH_TYPE_1 = 100010; // A1,B1;
constexpr uint64 QASSANDRA_MATCH_TYPE_2 = 100011; // A0,A1;
constexpr uint64 QASSANDRA_MATCH_TYPE_3 = 100012; // B0,B1;
constexpr uint64 QASSANDRA_ADD_BID = 100013;
constexpr uint64 QASSANDRA_ADD_ASK = 100014;
constexpr uint64 QASSANDRA_ASK_BIT = 0;
constexpr uint64 QASSANDRA_BID_BIT = 1;
constexpr uint64 QASSANDRA_EID_MASK = 0x3FFFFFFFFFFFFFFFULL; // (2^62 - 1);
constexpr uint64 QASSANDRA_MAX_AMOUNT = 2000000000000LL; // 2 trillion;
constexpr sint8 QASSANDRA_RESULT_NOT_SET = -1;
constexpr sint8 QASSANDRA_RESULT_NO = 0;
constexpr sint8 QASSANDRA_RESULT_YES = 1;

struct QASSANDRA2
{
};

struct QASSANDRA : public ContractBase
{
public:
    // logging enum
    struct QassandraLogger
    {
        uint32 _contractIndex;
        uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
        // Other data go here
        sint8 _terminator; // Only data before "_terminator" are logged
    };

    // logging with a number
    struct QassandraLoggerWithData
    {
        uint32 _contractIndex;
        uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
        id data;
        // Other data go here
        sint8 _terminator; // Only data before "_terminator" are logged
    };

    struct QassandraTradeLogger
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
        uint64 operationFee; // 1000 is 100%
        uint64 shareholderFee;
        uint64 burnFee; // percentage
        uint64 nIssuedEvent; // number of issued forecast markets
        uint64 shareholdersRevenue;
        uint64 operationRevenue;
        uint64 burnedAmount;
        uint64 mFeePerDay;
        uint64 antiSpamAmount;
        uint64 depositAmountForDispute;
        id gameOperator; // ABI name retained; represents the market operator
    };

    /**************************************/
    /************CONTRACT STATES***********/
    /**************************************/
    /*
    * QDRA V2: QUBIC/USD and ecosystem milestone forecasting marketplace scaffold.
    * Oracle-first settlement is planned later; current settlement comments describe the inherited operator/dispute flow.
    */

    struct QdraEventInfo
    {
        uint64 eid;
        DateAndTime openDate; // submitted date
        DateAndTime endDate; // stop receiving outcomes from the market operator

        /*256 sint8s to describe the forecast market*/
        Array<id, 4> desc;
        /*128 sint8s to describe forecast outcomes */
        Array<id, 2> option0Desc;
        Array<id, 2> option1Desc;
    };
    struct QdraMarketMetadata
    {
        uint8 marketType; // QASSANDRA_MARKET_TYPE_*
        uint8 comparison; // QASSANDRA_COMPARISON_* for threshold markets
        uint16 reserved0;
        sint64 thresholdValue; // reserved for scaled QUBIC/USD threshold values
        uint64 targetDate; // reserved deadline or target date for threshold/milestone markets
        Array<id, 2> reference; // reserved for future oracle feed, milestone, or release references
    };
    struct QdraOracleSettlement
    {
        sint64 queryId;
        uint32 requestedTick;
        uint8 status;
        sint8 oracleOutcome;
        uint16 reserved0;
        sint64 numerator;
        sint64 denominator;
    };
    struct DepositInfo
    {
        id pubkey;
        sint64 amount;
    };
    struct DisputeResolveInfo
    {
        Array<uint16, 1024> epochData;
        Array<sint8, 1024> voteData;
    };
    // qdra-v2: orders array
    struct QdraOrder
    {
        id entity;
        sint64 amount;
    };

    struct OrderInfo
    {
        uint64 eid;
        uint64 option; // 0 or 1
        uint64 tradeBit; // 0 ask, 1 bid
    };

    struct DetailedOrderInfo
    {
        QdraOrder qo;
        uint64 eid;
        sint64 price;
        sint64 index;
        bit justAdded;
    };
    // gov struct
    struct QdraGOV // votable by mQDRAGOVIdentifier holders
    {
        uint64 mShareHolderFee;
        uint64 mBurnFee;
        uint64 mOperationFee;
        sint64 mFeePerDay;
        sint64 mDepositAmountForDispute;
        id mOperationId;
        bool isValid()
        {
            return mShareHolderFee < QASSANDRA_PERCENT_DENOMINATOR &&
                mBurnFee < QASSANDRA_PERCENT_DENOMINATOR &&
                mOperationFee < QASSANDRA_PERCENT_DENOMINATOR &&
                (mShareHolderFee + mBurnFee + mOperationFee) < QASSANDRA_PERCENT_DENOMINATOR &&
                mOperationId != NULL_ID;
        }
    };

    // holders can submit a new set of params, if 2/3 agree with the same config, that will be used in next epoch
    struct proposalVoter
    {
        id publicKey;
        QdraGOV proposed;
        uint64 amountOfShares;
        uint16 proposedEpoch;
    };
    // buffers for calculating at the end of epoch
    struct GovHolder
    {
        id publicKey;
        sint64 amount;
    };

    struct StateData
    {
        // Existing generic fields map into Qassandra forecast markets as follows:
        // mEventInfo.desc stores the market question, option descriptions store binary outcomes,
        // openDate controls trading start, and endDate controls the current operator outcome window.
        HashMap<uint64, QdraEventInfo, QASSANDRA_MAX_CONCURRENT_EVENT> mEventInfo;
        // Optional type-specific metadata for QUBIC/USD threshold and ecosystem milestone markets.
        // This is metadata only; oracle-first settlement and typed market validation are future work.
        HashMap<uint64, QdraMarketMetadata, QASSANDRA_MAX_CONCURRENT_EVENT> mMarketMetadata;
        HashMap<uint64, QdraOracleSettlement, QASSANDRA_MAX_CONCURRENT_EVENT> mOracleSettlement;
        HashMap<sint64, uint64, QASSANDRA_MAX_CONCURRENT_EVENT> mOracleQueryToEvent;
        HashMap<uint64, sint8, QASSANDRA_MAX_CONCURRENT_EVENT> mEventResult; // forecast outcome: NOT_SET: -1 , NO: 0, 1: YES: 1
        HashMap<uint64, uint32, QASSANDRA_MAX_CONCURRENT_EVENT> mEventResultPublishTickTime; // tick tracker for when the forecast outcome was published
        HashMap<uint64, bit, QASSANDRA_MAX_CONCURRENT_EVENT> mEventFinalFlag; // flag if the forecast market is finalized (outcome is set and past dispute window)
        HashMap<uint64, DepositInfo, QASSANDRA_MAX_CONCURRENT_EVENT> mDisputeInfo;
        HashMap<uint64, DisputeResolveInfo, QASSANDRA_MAX_CONCURRENT_EVENT> mDisputeResolver;
        HashMap<uint64, DepositInfo, QASSANDRA_MAX_CONCURRENT_EVENT> mGODepositInfo;
        HashMap<id, QdraOrder, QASSANDRA_MAX_NUMBER_OF_USER> mPositionInfo;
        Collection<QdraOrder, 2097152 * X_MULTIPLIER> mABOrders;

        Array<uint64, QASSANDRA_MAX_CONCURRENT_EVENT> mRecentActiveEvent;

        // other stats
        uint64 mCurrentEventID;

        uint64 mShareholdersRevenue;
        uint64 mDistributedShareholdersRevenue;

        uint64 mOperationRevenue;
        uint64 mDistributedOperationRevenue;
        uint64 mBurnedAmount;
        Asset mQUSDIdentifier;
        Asset mQDRAGOVIdentifier;
        sint64 wholeSharePrice;
        QdraGOV mQdraGov;
        struct OperationParams // can be changed by the market operation team
        {
            HashMap<id, uint64, 8192 * X_MULTIPLIER> discountedFeeForUsers; // for professional market maker
            sint64 mAntiSpamAmount; // in QUs
        } mOperationParams;
        Array< proposalVoter, 1024> mGovVoters;
        HashMap<id, sint32, 1024> mVoteMap;
        Array<GovHolder, 1024> mGovArray;
        Array<sint64, 1024> mAccumulatedSum;
    };


    /**************************************/
    /************UTIL FUNCTIONS************/
    /**************************************/
protected:
    inline static uint64 orderKey(uint64 option, uint64 tradeBit, uint64 eid)
    {
        return ((option << 63) | (tradeBit << 62) | (eid & QASSANDRA_EID_MASK));
    }
    inline static uint64 posKey(uint64 option, uint64 eid)
    {
        return (((uint64)(option) << 63) | (eid & QASSANDRA_EID_MASK));
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
     * Packs forecast outcome option, trade type (ask/bid), and market ID into a single uint64.
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
     * Packs the user's ID with the forecast outcome option and market ID.
     */
    static id MakePosKey(id r, const uint64 eid, const uint64 option)
    {
        r.u64._3 = posKey(option, eid);
        return r;
    }

    /**
     * @brief Checks if a binary forecast outcome option is valid (0 or 1).
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
        return amount > 0 && amount < QASSANDRA_MAX_AMOUNT;
    }

    /**
     * @brief Checks if a price is within valid contract limits.
     */
    static bool isPriceValid(sint64 price, sint64 wholeSharePrice)
    {
        return price > 0 && price <= wholeSharePrice;
    }

    inline static id qubicCurrencyId()
    {
        return id(Ch::Q, Ch::U, Ch::B, Ch::I, Ch::C);
    }

    inline static id usdCurrencyId()
    {
        return id(Ch::U, Ch::S, Ch::D, Ch::null, Ch::null);
    }

    inline static id usdtCurrencyId()
    {
        return id(Ch::U, Ch::S, Ch::D, Ch::T, Ch::null);
    }

    inline static bool isQubicUsdOracleReplyValid(const OI::Price::OracleReply& reply)
    {
        return reply.numerator > 0 && reply.denominator > 0;
    }

    inline static bool mapQubicUsdThresholdOutcome(const OI::Price::OracleReply& reply, const QdraMarketMetadata& metadata, sint8& outcome)
    {
        outcome = QASSANDRA_RESULT_NOT_SET;

        if (!isQubicUsdOracleReplyValid(reply) ||
            metadata.marketType != QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD ||
            metadata.thresholdValue <= 0)
        {
            return false;
        }

        const uint128 observed = uint128((uint64)reply.numerator) * uint128(QASSANDRA_QUBIC_USD_PRICE_SCALE);
        const uint128 threshold = uint128((uint64)metadata.thresholdValue) * uint128((uint64)reply.denominator);

        if (metadata.comparison == QASSANDRA_COMPARISON_GTE)
        {
            outcome = (observed >= threshold) ? QASSANDRA_RESULT_YES : QASSANDRA_RESULT_NO;
            return true;
        }

        if (metadata.comparison == QASSANDRA_COMPARISON_LTE)
        {
            outcome = (observed <= threshold) ? QASSANDRA_RESULT_YES : QASSANDRA_RESULT_NO;
            return true;
        }

        return false;
    }

    inline static bool decodeQassandraTargetDate(uint64 targetDate, DateAndTime& output)
    {
        uint64 year;
        uint64 month;
        uint64 day;
        uint64 hour = 0;
        uint64 minute = 0;
        uint64 second = 0;

        if (targetDate >= 10000000ULL && targetDate <= 99999999ULL)
        {
            year = div(targetDate, 10000ULL);
            month = mod(div(targetDate, 100ULL), 100ULL);
            day = mod(targetDate, 100ULL);
        }
        else if (targetDate >= 10000000000000ULL && targetDate <= 99999999999999ULL)
        {
            year = div(targetDate, 10000000000ULL);
            month = mod(div(targetDate, 100000000ULL), 100ULL);
            day = mod(div(targetDate, 1000000ULL), 100ULL);
            hour = mod(div(targetDate, 10000ULL), 100ULL);
            minute = mod(div(targetDate, 100ULL), 100ULL);
            second = mod(targetDate, 100ULL);
        }
        else
        {
            output = DateAndTime();
            return false;
        }

        return output.setIfValid(year, month, day, hour, minute, second);
    }

    inline static bool resolveQubicUsdSettlementOracle(const id& requestedOracle, const QdraMarketMetadata& metadata, id& selectedOracle)
    {
        selectedOracle = requestedOracle;
        if (isZero(selectedOracle))
        {
            selectedOracle = metadata.reference.get(0);
        }

        return !isZero(selectedOracle);
    }

    inline static bool resolveQubicUsdQuoteCurrency(const id& requestedQuoteCurrency, id& selectedQuoteCurrency)
    {
        if (isZero(requestedQuoteCurrency))
        {
            selectedQuoteCurrency = usdCurrencyId();
            return true;
        }

        if (requestedQuoteCurrency == usdCurrencyId() || requestedQuoteCurrency == usdtCurrencyId())
        {
            selectedQuoteCurrency = requestedQuoteCurrency;
            return true;
        }

        selectedQuoteCurrency = NULL_ID;
        return false;
    }

    inline static bool isQubicUsdSettlementRequestMetadataValid(const QdraMarketMetadata& metadata, DateAndTime& targetTimestamp)
    {
        if (metadata.marketType != QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD ||
            (metadata.comparison != QASSANDRA_COMPARISON_GTE && metadata.comparison != QASSANDRA_COMPARISON_LTE) ||
            metadata.thresholdValue <= 0)
        {
            targetTimestamp = DateAndTime();
            return false;
        }

        return decodeQassandraTargetDate(metadata.targetDate, targetTimestamp);
    }

    inline static bool canRetryQubicUsdSettlement(uint8 status)
    {
        return status == QASSANDRA_ORACLE_SETTLEMENT_TIMEOUT ||
            status == QASSANDRA_ORACLE_SETTLEMENT_UNRESOLVABLE ||
            status == QASSANDRA_ORACLE_SETTLEMENT_INVALID_REPLY ||
            status == QASSANDRA_ORACLE_SETTLEMENT_QUERY_ERROR;
    }

    inline static uint8 qassandraSettlementStatusFromOracleStatus(uint8 oracleStatus)
    {
        if (oracleStatus == ORACLE_QUERY_STATUS_SUCCESS)
        {
            return QASSANDRA_ORACLE_SETTLEMENT_SUCCESS;
        }

        if (oracleStatus == ORACLE_QUERY_STATUS_TIMEOUT)
        {
            return QASSANDRA_ORACLE_SETTLEMENT_TIMEOUT;
        }

        if (oracleStatus == ORACLE_QUERY_STATUS_UNRESOLVABLE)
        {
            return QASSANDRA_ORACLE_SETTLEMENT_UNRESOLVABLE;
        }

        return QASSANDRA_ORACLE_SETTLEMENT_QUERY_ERROR;
    }

    inline static bool isMarketMetadataValid(const QdraMarketMetadata& metadata)
    {
        if (metadata.reserved0 != 0 || metadata.targetDate == 0)
        {
            return false;
        }

        if (metadata.marketType == QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD)
        {
            return (metadata.comparison == QASSANDRA_COMPARISON_GTE ||
                metadata.comparison == QASSANDRA_COMPARISON_LTE) &&
                metadata.thresholdValue > 0;
        }

        if (metadata.marketType == QASSANDRA_MARKET_TYPE_ECOSYSTEM_MILESTONE)
        {
            return metadata.comparison == QASSANDRA_COMPARISON_UNSPECIFIED &&
                metadata.thresholdValue == 0;
        }

        return false;
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
        QdraOrder qo;
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
        QdraEventInfo qei;
        DateAndTime dt;
        bool status;
    };

    /**
     * @brief Checks if a forecast market is valid for trading (exists and is within its trading window).
     * @param eventId The unique identifier of the forecast market.
     * @return 1 if the forecast market is valid for trading, 0 otherwise.
     */
    PRIVATE_FUNCTION_WITH_LOCALS(ValidateEvent)
    {
        output.isValid = 0;
        locals.status = state.get().mEventInfo.get(input.eventId, locals.qei);
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
     * @param eventId The forecast market ID.
     * @param option The binary forecast outcome option (0 or 1).
     * @param amount The amount to check for.
     * @return 1 if the user's position is sufficient, 0 otherwise.
     */
    PRIVATE_FUNCTION_WITH_LOCALS(ValidatePosition)
    {
        output.isValid = 0;
        locals.key = MakePosKey(input.uid, input.eventId, input.option);
        if (!state.get().mPositionInfo.get(locals.key, locals.qo))
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
        QdraOrder value;
    };

    // Update position of an user - must call validation before calling this
    PRIVATE_PROCEDURE_WITH_LOCALS(UpdatePosition)
    {
        output.ok = 0;

        locals.key = MakePosKey(input.uid, input.oi.eid, input.oi.option);

        if (!state.get().mPositionInfo.get(locals.key, locals.value))
        {
            if (input.amountChange >= 0)
            {
                locals.value.amount = input.amountChange;
                locals.value.entity = input.uid;
                state.mut().mPositionInfo.set(locals.key, locals.value);
            }
            else
            {
                // invalid state: cannot apply negative change to non-existent position
                output.ok = 0;
                return;
            }
        }
        else
        {
            locals.value.amount += input.amountChange;
            if (locals.value.amount > 0)
            {
                state.mut().mPositionInfo.set(locals.key, locals.value);
            }
            else if (locals.value.amount == 0)
            {
                state.mut().mPositionInfo.removeByKey(locals.key);
            }
            else
            {
                // invalid state: position amount went negative
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
        bit needChargeFee;
    };

    struct RewardTransfer_output
    {
        bit ok;
    };

    struct RewardTransfer_locals
    {
        sint64 total, feeTotal, fee;
        uint64 rate, afterDiscountRate, amountWithDiscount;
    };

    /**
     * @brief Handles all outgoing transfers from the contract. It calculates and deducts fees
     * for shareholders and operations before sending the net amount.
     * TODO: Add oracle-specific accounting when oracle-first settlement is implemented.
     * @param receiver The recipient of the funds.
     * @param eid The associated forecast market ID for fee calculation.
     * @param amount The gross amount to transfer.
     * @param needChargeFee If true, fees are calculated and deducted.
     */
    PRIVATE_PROCEDURE_WITH_LOCALS(RewardTransfer)
    {
        output.ok = false;
        if (input.amount <= 0 || input.amount >= MAX_AMOUNT) return;
        if (input.receiver == NULL_ID) return;
        locals.total = input.amount;
        locals.feeTotal = 0;

        if (input.needChargeFee)
        {
            // get discounted rate for this user
            locals.rate = 0;
            if (state.get().mOperationParams.discountedFeeForUsers.contains(input.receiver))
            {
                state.get().mOperationParams.discountedFeeForUsers.get(input.receiver, locals.rate);
            }
            locals.afterDiscountRate = QASSANDRA_PERCENT_DENOMINATOR - locals.rate;

            if (locals.afterDiscountRate)
            {
                locals.amountWithDiscount = smul((uint64)input.amount, locals.afterDiscountRate);
                // shareholders
                locals.fee = div(smul(locals.amountWithDiscount, state.get().mQdraGov.mShareHolderFee), QASSANDRA_PERCENT_DENOMINATOR * QASSANDRA_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mut().mShareholdersRevenue += locals.fee;

                // market operation team
                locals.fee = div(smul(locals.amountWithDiscount, state.get().mQdraGov.mOperationFee), QASSANDRA_PERCENT_DENOMINATOR * QASSANDRA_PERCENT_DENOMINATOR);
                locals.feeTotal += locals.fee;
                state.mut().mOperationRevenue += locals.fee;
            }
        }

        if (qpi.transferShareOwnershipAndPossession(state.get().mQUSDIdentifier.assetName, state.get().mQUSDIdentifier.issuer, SELF, SELF, locals.total - locals.feeTotal, input.receiver) < 0)
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

        sint64 matchedAmount;
        sint64 matchedPrice;
        sint64 matchedPrice0;
        sint64 matchedPrice1;

        id key, r;

        RewardTransfer_input ti;
        RewardTransfer_output to;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        QassandraTradeLogger log;
    };

    // only invoke when recently added order is any of (A0, A1, B0, B1)
    PRIVATE_PROCEDURE_WITH_LOCALS(MatchingOrders)
    {
        /*Initialize data*/
        output.matched = 0;
        locals.key = MakeOrderKey(input.eventId, 0, QASSANDRA_ASK_BIT, locals.r);
        locals.A0.index = state.mut().mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_ASK_BIT, locals.r);
        locals.A1.index = state.mut().mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 0, QASSANDRA_BID_BIT, locals.r);
        locals.B0.index = state.mut().mABOrders.headIndex(locals.key);

        locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_BID_BIT, locals.r);
        locals.B1.index = state.mut().mABOrders.headIndex(locals.key);

        if (locals.A0.index != NULL_INDEX)
        {
            locals.A0.qo = state.mut().mABOrders.element(locals.A0.index);
            locals.A0.price = -state.mut().mABOrders.priority(locals.A0.index);
            locals.A0.justAdded = (input.justAddedIndex == locals.A0.index);
        }
        if (locals.A1.index != NULL_INDEX)
        {
            locals.A1.qo = state.mut().mABOrders.element(locals.A1.index);
            locals.A1.price = -state.mut().mABOrders.priority(locals.A1.index);
            locals.A1.justAdded = (input.justAddedIndex == locals.A1.index);
        }
        if (locals.B0.index != NULL_INDEX)
        {
            locals.B0.qo = state.mut().mABOrders.element(locals.B0.index);
            locals.B0.price = state.mut().mABOrders.priority(locals.B0.index);
            locals.B0.justAdded = (input.justAddedIndex == locals.B0.index);
        }
        if (locals.B1.index != NULL_INDEX)
        {
            locals.B1.qo = state.mut().mABOrders.element(locals.B1.index);
            locals.B1.price = state.mut().mABOrders.priority(locals.B1.index);
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
            locals.log = QassandraTradeLogger{ 0, QASSANDRA_MATCH_TYPE_0, locals.A0.qo.entity, locals.B0.qo.entity, input.eventId, 0, 0, 0, locals.matchedAmount, locals.matchedPrice, 0, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = smul(locals.matchedPrice, locals.matchedAmount);
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A0.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            // add shares to B (A is already deducted when they do ask order)
            locals.upi.uid = locals.B0.qo.entity;
            locals.upi.amountChange = locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 0;
            locals.upi.oi.tradeBit = QASSANDRA_ASK_BIT;
            CALL(UpdatePosition, locals.upi, locals.upo);

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice)
            {
                // refund without charging fees
                locals.ti.amount = smul(locals.B0.price - locals.matchedPrice, locals.matchedAmount);
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B0.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
            }

            output.matched = 1;
            // update order size
            locals.A0.qo.amount -= locals.matchedAmount;
            locals.B0.qo.amount -= locals.matchedAmount;
            if (locals.A0.qo.amount)
            {
                state.mut().mABOrders.replace(locals.A0.index, locals.A0.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.A0.index);
            }

            // A0 maybe removed and change the index of B0
            locals.key = MakeOrderKey(input.eventId, 0, QASSANDRA_BID_BIT, locals.r);
            locals.B0.index = state.mut().mABOrders.headIndex(locals.key);

            if (locals.B0.qo.amount)
            {
                state.mut().mABOrders.replace(locals.B0.index, locals.B0.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.B0.index);
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
            locals.log = QassandraTradeLogger{ 0, QASSANDRA_MATCH_TYPE_1, locals.A1.qo.entity, locals.B1.qo.entity, input.eventId, 1, 0, 0, locals.matchedAmount, locals.matchedPrice, 0, 0 };
            LOG_INFO(locals.log);


            locals.ti.amount = smul(locals.matchedPrice, locals.matchedAmount);
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A1.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            // add shares to B (A is already deducted when they do ask order)
            locals.upi.uid = locals.B1.qo.entity;
            locals.upi.amountChange = locals.matchedAmount;
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = 1;
            locals.upi.oi.tradeBit = QASSANDRA_ASK_BIT;
            CALL(UpdatePosition, locals.upi, locals.upo);

            if (locals.B1.justAdded && locals.B1.price > locals.matchedPrice)
            {
                // refund without charging fees
                locals.ti.amount = smul(locals.B1.price - locals.matchedPrice, locals.matchedAmount);
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
            }

            output.matched = 1;
            // update order size
            locals.A1.qo.amount -= locals.matchedAmount;
            locals.B1.qo.amount -= locals.matchedAmount;
            if (locals.A1.qo.amount)
            {
                state.mut().mABOrders.replace(locals.A1.index, locals.A1.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.A1.index);
            }

            // A1 maybe removed and change the index of B1
            locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_BID_BIT, locals.r);
            locals.B1.index = state.mut().mABOrders.headIndex(locals.key);

            if (locals.B1.qo.amount)
            {
                state.mut().mABOrders.replace(locals.B1.index, locals.B1.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.B1.index);
            }
            return;
        }

        // Scenario 2: MERGE - both want to sell
        // A0 and A1 want to exit => A0.price + A1.price < state.mut().wholeSharePrice
        if (locals.A0.price + locals.A1.price <= state.mut().wholeSharePrice && locals.A0.index != NULL_INDEX && locals.A1.index != NULL_INDEX)
        {
            if (locals.A0.justAdded)
            {
                locals.matchedPrice0 = state.mut().wholeSharePrice - locals.A1.price;
                locals.matchedPrice1 = locals.A1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.A0.price;
                locals.matchedPrice1 = state.mut().wholeSharePrice - locals.A0.price;
            }
            locals.matchedAmount = min(locals.A0.qo.amount, locals.A1.qo.amount);
            locals.log = QassandraTradeLogger{ 0, QASSANDRA_MATCH_TYPE_2, locals.A0.qo.entity, locals.A1.qo.entity, input.eventId, 2, 0, 0, locals.matchedAmount, locals.matchedPrice0, locals.matchedPrice1, 0 };
            LOG_INFO(locals.log);

            locals.ti.amount = smul(locals.matchedPrice0, locals.matchedAmount);
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A0.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            locals.ti.amount = smul(locals.matchedPrice1, locals.matchedAmount);
            locals.ti.eid = input.eventId;
            locals.ti.receiver = locals.A1.qo.entity;
            locals.ti.needChargeFee = 1;
            CALL(RewardTransfer, locals.ti, locals.to);
            if (!locals.to.ok) return;

            // the position is already deducted when adding ask orders, no need to do anything here about position shares

            output.matched = 1;

            // update order size
            locals.A0.qo.amount -= locals.matchedAmount;
            locals.A1.qo.amount -= locals.matchedAmount;
            if (locals.A0.qo.amount)
            {
                state.mut().mABOrders.replace(locals.A0.index, locals.A0.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.A0.index);
            }

            // A0 maybe removed and change the index of A1
            locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_ASK_BIT, locals.r);
            locals.A1.index = state.mut().mABOrders.headIndex(locals.key);

            if (locals.A1.qo.amount)
            {
                state.mut().mABOrders.replace(locals.A1.index, locals.A1.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.A1.index);
            }
            return;
        }

        // Scenario 3: MINT - both want to buy
        if (locals.B0.price + locals.B1.price >= state.mut().wholeSharePrice && locals.B0.index != NULL_INDEX && locals.B1.index != NULL_INDEX)
        {
            if (locals.B0.justAdded)
            {
                locals.matchedPrice0 = state.mut().wholeSharePrice - locals.B1.price;
                locals.matchedPrice1 = locals.B1.price;
            }
            else
            {
                locals.matchedPrice0 = locals.B0.price;
                locals.matchedPrice1 = state.mut().wholeSharePrice - locals.B0.price;
            }
            locals.matchedAmount = min(locals.B0.qo.amount, locals.B1.qo.amount);
            locals.log = QassandraTradeLogger{ 0, QASSANDRA_MATCH_TYPE_3, locals.B0.qo.entity, locals.B1.qo.entity, input.eventId, 2, 0, 0, locals.matchedAmount, locals.matchedPrice0, locals.matchedPrice1, 0 };
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
                // refund without charging fees
                locals.ti.amount = smul(locals.B1.price - locals.matchedPrice1, locals.matchedAmount);
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B1.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
            }

            if (locals.B0.justAdded && locals.B0.price > locals.matchedPrice0)
            {
                // refund without charging fees
                locals.ti.amount = smul(locals.B0.price - locals.matchedPrice0, locals.matchedAmount);
                locals.ti.eid = input.eventId;
                locals.ti.receiver = locals.B0.qo.entity;
                locals.ti.needChargeFee = 0;
                CALL(RewardTransfer, locals.ti, locals.to);
                if (!locals.to.ok) return;
            }
            // update order size
            locals.B0.qo.amount -= locals.matchedAmount;
            locals.B1.qo.amount -= locals.matchedAmount;
            if (locals.B0.qo.amount)
            {
                state.mut().mABOrders.replace(locals.B0.index, locals.B0.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.B0.index);
            }

            // B0 maybe removed and change the index of B1
            locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_BID_BIT, locals.r);
            locals.B1.index = state.mut().mABOrders.headIndex(locals.key);

            if (locals.B1.qo.amount)
            {
                state.mut().mABOrders.replace(locals.B1.index, locals.B1.qo);
            }
            else
            {
                state.mut().mABOrders.remove(locals.B1.index);
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
        QdraEventInfo qei;
        sint32 resultByGO; // market-operator outcome: NOT_SET: -1 , NO: 0, 1: YES: 1
        uint32 publishTickTime; // ignore if outcome is not set. if outcome is set and
        // publishTickTime is max uint32 then this forecast market is already finalized
        DepositInfo disputerInfo; // NULL if no dispute
        uint32 computorsVote0;
        uint32 computorsVote1;
    };

    struct GetEventInfo_locals
    {
        sint8 result;
        DisputeResolveInfo dri;
        sint32 i;
    };
    /**
     * @param eventId
     * @return metadata of a forecast market and its current state
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetEventInfo)
    {
        setMemory(output.qei, 0);
        state.get().mEventInfo.get(input.eventId, output.qei);
        state.get().mEventResult.get(input.eventId, locals.result);
        state.get().mEventResultPublishTickTime.get(input.eventId, output.publishTickTime);
        output.resultByGO = (sint32)locals.result; // for padding
        if (state.get().mDisputeInfo.contains(input.eventId))
        {
            state.get().mDisputeInfo.get(input.eventId, output.disputerInfo);
            state.get().mDisputeResolver.get(input.eventId, locals.dri);
            for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
            {
                if (locals.dri.epochData.get(locals.i) == qpi.epoch())
                {
                    if (locals.dri.voteData.get(locals.i) == 0) output.computorsVote0++;
                    if (locals.dri.voteData.get(locals.i) == 1) output.computorsVote1++;
                }
            }
        }
        else
        {
            output.disputerInfo.pubkey = NULL_ID;
            output.disputerInfo.amount = 0;
        }
        if (locals.result != QASSANDRA_RESULT_NOT_SET && state.get().mEventFinalFlag.contains(input.eventId))
        {
            output.publishTickTime = 0xffffffffu;
        }
    }

    struct GetEventInfoBatch_input
    {
        Array<uint64, 64> eventIds;
    };
    struct GetEventInfoBatch_output
    {
        Array<QdraEventInfo, 64> aqei;
    };

    struct GetEventInfoBatch_locals
    {
        uint64 i;
        QdraEventInfo qei;
    };

    /**
     * @brief Retrieves the metadata for 64 specific forecast markets.
     * @param array of eventId
     * @return The array of QdraEventInfo struct containing the forecast market details.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetEventInfoBatch)
    {
        setMemory(output.aqei, 0);
        for (locals.i = 0; locals.i < 64; locals.i++)
        {
            if (state.get().mEventInfo.get(input.eventIds.get(locals.i), locals.qei))
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
        QdraEventInfo qei;
        QdraOrder order;

        OrderInfo oi;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        ValidatePosition_input vpi;
        ValidatePosition_output vpo;

        MatchingOrders_input moi;
        MatchingOrders_output moo;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        QassandraLogger log;
        QassandraTradeLogger tradeLog;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to place an ask (sell) order for a position they own.
     * @param eventId The forecast market to place the order on.
     * @param option The binary forecast outcome option (0 or 1) to sell.
     * @param amount The number of shares to sell.
     * @param price The price per share.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(AddToAskOrder)
    {
        if (state.get().mOperationParams.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.get().mOperationParams.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.get().mOperationParams.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().mOperationParams.mAntiSpamAmount);
        }
        locals.oi.eid = input.eventId;
        locals.oi.option = input.option;
        locals.oi.tradeBit = QASSANDRA_ASK_BIT;

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            locals.log = QassandraLogger{ 0, QASSANDRA_INVALID_EVENT_ID ,0 };
            LOG_WARNING(locals.log);
            return;
        }

        if (!state.get().mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.get().wholeSharePrice))
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
            locals.log = QassandraLogger{ 0, QASSANDRA_INVALID_POSITION, 0 };
            LOG_WARNING(locals.log);
            return;
        }

        {
            // deduct share
            locals.upi.uid = qpi.invocator();
            locals.upi.amountChange = -(sint64)(input.amount); // Negative to reduce balance
            locals.upi.oi.eid = input.eventId;
            locals.upi.oi.option = input.option;
            locals.upi.oi.tradeBit = QASSANDRA_ASK_BIT;
            CALL(UpdatePosition, locals.upi, locals.upo);
        }

        locals.tradeLog = QassandraTradeLogger{ 0, QASSANDRA_ADD_ASK, qpi.invocator(), NULL_ID, input.eventId, input.option, 0, 0, (sint64)input.amount, (sint64)input.price, 0, 0 };
        LOG_INFO(locals.tradeLog);

        locals.key = MakeOrderKey(input.eventId, input.option, QASSANDRA_ASK_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key, -input.price);
        locals.flag = false;

        // if there is exact 100% same order as this (same amount, trade bit, price, market ID):
        // if same user => replace new one
        // if diff user => add new one
        // final => exit
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.get().mABOrders.element(locals.index);
            locals.price = -state.get().mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                // this means there's another unmatched order with the same price
                locals.flag = true;
                if (locals.order.entity == qpi.invocator())
                {
                    // same entity => update => exit
                    locals.order.amount += input.amount;
                    state.mut().mABOrders.replace(locals.index, locals.order);
                    output.status = 1;
                    return;
                }
                locals.index = state.get().mABOrders.nextElementIndex(locals.index);
            }
            else
            {
                break;
            }
        }

        locals.order.amount = input.amount;
        locals.order.entity = qpi.invocator();
        locals.index = state.mut().mABOrders.add(locals.key, locals.order, -input.price);

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
                    locals.key = MakeOrderKey(input.eventId, input.option, QASSANDRA_ASK_BIT, locals.r);
                    locals.moi.justAddedIndex = state.get().mABOrders.headIndex(locals.key, -input.price);
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
        QdraEventInfo qei;
        QdraOrder order;
        sint64 price;
        sint64 amountToRemove;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        UpdatePosition_input upi;
        UpdatePosition_output upo;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to remove an existing ask (sell) order from the order book.
     * @param eventId The forecast market of the order.
     * @param option The binary forecast outcome option (0 or 1) of the order.
     * @param amount The amount to remove.
     * @param price The price of the order to remove.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveAskOrder)
    {
        if (state.get().mOperationParams.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.get().mOperationParams.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.get().mOperationParams.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().mOperationParams.mAntiSpamAmount);
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        if (!state.get().mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.get().wholeSharePrice))
        {
            return;
        }

        // No ValidatePosition here: shares were already deducted from position
        // when the ask order was placed. Validation is against the order book below.

        locals.key = MakeOrderKey(input.eventId, input.option, QASSANDRA_ASK_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key, -input.price);

        // finding and modifying order
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.get().mABOrders.element(locals.index);
            locals.price = -state.get().mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                if (locals.order.entity == qpi.invocator())
                {
                    // clamp to actual order amount to prevent underflow
                    locals.amountToRemove = min(input.amount, locals.order.amount);
                    // same entity => update => exit
                    locals.order.amount -= locals.amountToRemove;
                    if (locals.order.amount)
                    {
                        state.mut().mABOrders.replace(locals.index, locals.order);
                    }
                    else
                    {
                        state.mut().mABOrders.remove(locals.index);
                    }
                    output.status = 1;
                    {
                        // add the position share back to mPositionInfo array
                        locals.upi.uid = qpi.invocator();
                        locals.upi.amountChange = locals.amountToRemove;
                        locals.upi.oi.eid = input.eventId;
                        locals.upi.oi.option = input.option;
                        locals.upi.oi.tradeBit = QASSANDRA_ASK_BIT;
                        CALL(UpdatePosition, locals.upi, locals.upo);
                    }
                    return;
                }
                locals.index = state.get().mABOrders.nextElementIndex(locals.index);
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
        QdraEventInfo qei;
        QdraOrder order;
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
     * @param eventId The forecast market of the order.
     * @param option The binary forecast outcome option (0 or 1) of the order.
     * @param amount The amount to remove.
     * @param price The price of the order to remove.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveBidOrder)
    {
        if (state.get().mOperationParams.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.get().mOperationParams.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.get().mOperationParams.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().mOperationParams.mAntiSpamAmount);
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        if (!state.get().mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.get().wholeSharePrice))
        {
            return;
        }

        locals.key = MakeOrderKey(input.eventId, input.option, QASSANDRA_BID_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key, input.price);

        // finding and modifying order
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.get().mABOrders.element(locals.index);
            locals.price = state.get().mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                if (locals.order.entity == qpi.invocator())
                {
                    locals.amountToRemove = min(input.amount, locals.order.amount);
                    // same entity => update => exit
                    locals.order.amount -= locals.amountToRemove;
                    if (locals.order.amount)
                    {
                        state.mut().mABOrders.replace(locals.index, locals.order);
                    }
                    else
                    {
                        state.mut().mABOrders.remove(locals.index);
                    }
                    // refund
                    locals.rti.amount = smul(locals.amountToRemove, input.price);
                    locals.rti.eid = input.eventId;
                    locals.rti.needChargeFee = 0;
                    locals.rti.receiver = qpi.invocator();
                    CALL(RewardTransfer, locals.rti, locals.rto);
                    if (!locals.rto.ok) return;
                    output.status = 1;
                    return;
                }
                locals.index = state.get().mABOrders.nextElementIndex(locals.index);
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
        QdraOrder order;
        QdraEventInfo qei;

        ValidateEvent_input vei;
        ValidateEvent_output veo;

        MatchingOrders_input moi;
        MatchingOrders_output moo;

        QassandraTradeLogger log;
    };

    /**
     * @brief PUBLIC PROCEDURE
     * Allows a user to place a bid (buy) order, locking funds to back it.
     * Triggers the matching engine after the order is placed.
     * @param eventId The forecast market to place the order on.
     * @param option The binary forecast outcome option (0 or 1) to buy.
     * @param amount The number of shares to buy.
     * @param price The price per share.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(AddToBidOrder)
    {
        if (state.get().mOperationParams.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.get().mOperationParams.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.get().mOperationParams.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().mOperationParams.mAntiSpamAmount);
        }

        locals.vei.eventId = input.eventId;
        CALL(ValidateEvent, locals.vei, locals.veo);
        if (!locals.veo.isValid)
        {
            return;
        }

        if (!state.get().mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (!isOptionValid(input.option) || !isAmountValid(input.amount) || !isPriceValid(input.price, state.get().wholeSharePrice))
        {
            return;
        }
        locals.userBalance = qpi.numberOfShares(state.get().mQUSDIdentifier, { qpi.invocator(), SELF_INDEX }, { qpi.invocator(), SELF_INDEX });
        locals.totalCost = smul(input.amount, input.price);
        // verify enough amount
        if (locals.totalCost > locals.userBalance)
        {
            return;
        }
        if (qpi.transferShareOwnershipAndPossession(state.get().mQUSDIdentifier.assetName, state.get().mQUSDIdentifier.issuer, qpi.invocator(), qpi.invocator(), locals.totalCost, SELF) < 0)
        {
            // critical error
            return;
        }

        locals.log = QassandraTradeLogger{ 0, QASSANDRA_ADD_BID, qpi.invocator(), NULL_ID, input.eventId, input.option, 0, 0, (sint64)input.amount, (sint64)input.price, 0, 0 };
        LOG_INFO(locals.log);

        locals.key = MakeOrderKey(input.eventId, input.option, QASSANDRA_BID_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key, input.price);
        locals.flag = false;

        // if there is exact 100% same order as this (same amount, trade bit, price, market ID):
        // if same user => replace new one
        // if diff user => add new one
        // final => exit
        while (locals.index != NULL_INDEX)
        {
            locals.order = state.get().mABOrders.element(locals.index);
            locals.price = state.get().mABOrders.priority(locals.index);
            if (locals.price == input.price)
            {
                // this means there's another unmatched order with the same price
                locals.flag = true;
                if (locals.order.entity == qpi.invocator())
                {
                    // same entity => update => exit
                    locals.order.amount += input.amount;
                    state.mut().mABOrders.replace(locals.index, locals.order);
                    output.status = 1;
                    return;
                }
                locals.index = state.get().mABOrders.nextElementIndex(locals.index);
            }
            else
            {
                break;
            }
        }

        locals.order.amount = input.amount;
        locals.order.entity = qpi.invocator();
        locals.index = state.mut().mABOrders.add(locals.key, locals.order, input.price);

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
                    locals.key = MakeOrderKey(input.eventId, input.option, QASSANDRA_BID_BIT, locals.r);
                    locals.moi.justAddedIndex = state.get().mABOrders.headIndex(locals.key, input.price);
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

    // when users do not agree with the published market outcome, they can dispute
    // the forecast market will be resolved by computors
    PUBLIC_PROCEDURE_WITH_LOCALS(Dispute)
    {
        // deposit
        if (qpi.invocationReward() != state.get().mQdraGov.mDepositAmountForDispute)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (!state.get().mEventResult.get(input.eventId, locals.result))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (locals.result == QASSANDRA_RESULT_NOT_SET) // no outcome yet
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (state.get().mDisputeInfo.contains(input.eventId)) // already being disputed by other users
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (state.get().mEventFinalFlag.contains(input.eventId)) // already finalized
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.di.pubkey = qpi.invocator();
        locals.di.amount = state.get().mQdraGov.mDepositAmountForDispute;
        state.mut().mDisputeInfo.set(input.eventId, locals.di);
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
        QdraOrder qo;
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
        // only allow claiming after the forecast market is finalized (dispute window closed)
        if (!state.get().mEventFinalFlag.contains(input.eventId))
        {
            return; // no refund because of spam
        }
        state.get().mEventResult.get(input.eventId, locals.winOption);
        if (locals.winOption == QASSANDRA_RESULT_NOT_SET)
        {
            return; // no refund because of spam
        }

        locals.winCondition = posKey((uint64)(locals.winOption), input.eventId);
        locals.key = qpi.invocator();
        locals.key.u64._3 = locals.winCondition;
        if (state.get().mPositionInfo.contains(locals.key))
        {
            // return deposit
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            // payout
            state.get().mPositionInfo.get(locals.key, locals.qo);
            locals.rti.amount = smul(state.get().wholeSharePrice, locals.qo.amount);
            locals.rti.eid = input.eventId;
            locals.rti.receiver = locals.qo.entity;
            locals.rti.needChargeFee = 1;
            CALL(RewardTransfer, locals.rti, locals.rto);
            if (!locals.rto.ok) return;
            // remove the position
            state.mut().mPositionInfo.removeByKey(locals.key);
        }
        state.mut().mPositionInfo.cleanupIfNeeded();
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
        QdraOrder qo;
        uint64 winCondition, loseCondition;
        sint8 winOption;
        RewardTransfer_input rti;
        RewardTransfer_output rto;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(GOForceClaimReward)
    {
        if (qpi.invocator() != state.get().mQdraGov.mOperationId)
        {
            return;
        }
        // only allow claiming after the forecast market is finalized (dispute window closed)
        if (!state.get().mEventFinalFlag.contains(input.eventId))
        {
            return;
        }
        state.get().mEventResult.get(input.eventId, locals.winOption);
        if (locals.winOption == QASSANDRA_RESULT_NOT_SET)
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
            if (state.get().mPositionInfo.contains(locals.key))
            {
                // payout
                state.get().mPositionInfo.get(locals.key, locals.qo);
                locals.rti.amount = smul(state.get().wholeSharePrice, locals.qo.amount);
                locals.rti.eid = input.eventId;
                locals.rti.receiver = locals.qo.entity;
                locals.rti.needChargeFee = 1;
                CALL(RewardTransfer, locals.rti, locals.rto);
                if (!locals.rto.ok) return;
                // remove the position
                state.mut().mPositionInfo.removeByKey(locals.key);
            }

            locals.key.u64._3 = locals.loseCondition;
            if (state.get().mPositionInfo.contains(locals.key))
            {
                state.mut().mPositionInfo.removeByKey(locals.key);
            }
        }
        state.mut().mPositionInfo.cleanupIfNeeded();
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
        sint64 index;
        id key, r;
        QdraOrder value;
        sint64 price;

        RewardTransfer_input rti;
        RewardTransfer_output rto;

        UpdatePosition_input upi;
        UpdatePosition_output upo;

        QassandraLoggerWithData log;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeEvent)
    {
        locals.log = QassandraLoggerWithData{ 0, QASSANDRA_FINALIZE_EVENT, id(0,0,input.eventId,input.winOption), 0 };
        LOG_INFO(locals.log);
        state.mut().mEventFinalFlag.set(input.eventId, true);
        // cleaning all ABOrder
        locals.index = 0;

        // deleting ask 0
        locals.key = MakeOrderKey(input.eventId, 0, QASSANDRA_ASK_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.get().mABOrders.element(locals.index);
            {
                // return share
                locals.upi.uid = locals.value.entity;
                locals.upi.amountChange = locals.value.amount;
                locals.upi.oi.eid = input.eventId;
                locals.upi.oi.option = 0;
                locals.upi.oi.tradeBit = QASSANDRA_ASK_BIT;
                CALL(UpdatePosition, locals.upi, locals.upo);
            }
            state.mut().mABOrders.remove(locals.index);
            locals.index = state.get().mABOrders.headIndex(locals.key);
        }

        // deleting ask 1
        locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_ASK_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.get().mABOrders.element(locals.index);
            {
                // return share
                locals.upi.uid = locals.value.entity;
                locals.upi.amountChange = locals.value.amount;
                locals.upi.oi.eid = input.eventId;
                locals.upi.oi.option = 1;
                locals.upi.oi.tradeBit = QASSANDRA_ASK_BIT;
                CALL(UpdatePosition, locals.upi, locals.upo);
            }
            state.mut().mABOrders.remove(locals.index);
            locals.index = state.get().mABOrders.headIndex(locals.key);
        }

        // deleting bid 0
        locals.key = MakeOrderKey(input.eventId, 0, QASSANDRA_BID_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.get().mABOrders.element(locals.index);
            locals.price = state.get().mABOrders.priority(locals.index);
            // refund to users (continue on failure so remaining orders aren't stuck)
            locals.rti.receiver = locals.value.entity;
            locals.rti.amount = smul(locals.value.amount, locals.price);
            locals.rti.eid = input.eventId;
            locals.rti.needChargeFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);

            state.mut().mABOrders.remove(locals.index);
            locals.index = state.get().mABOrders.headIndex(locals.key);
        }

        // deleting bid 1
        locals.key = MakeOrderKey(input.eventId, 1, QASSANDRA_BID_BIT, locals.r);
        locals.index = state.get().mABOrders.headIndex(locals.key);
        while (locals.index != NULL_INDEX)
        {
            locals.value = state.get().mABOrders.element(locals.index);
            locals.price = state.get().mABOrders.priority(locals.index);
            // refund to users (continue on failure so remaining orders aren't stuck)
            locals.rti.receiver = locals.value.entity;
            locals.rti.amount = smul(locals.value.amount, locals.price);
            locals.rti.eid = input.eventId;
            locals.rti.needChargeFee = 0;
            CALL(RewardTransfer, locals.rti, locals.rto);

            state.mut().mABOrders.remove(locals.index);
            locals.index = state.get().mABOrders.headIndex(locals.key);
        }

        state.mut().mABOrders.cleanupIfNeeded();
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
        sint32 voteCount; // final vote count for outcome
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

        QassandraLoggerWithData log;
    };
    // resolve a disputed forecast market, can only be called by computors
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
        if (!state.get().mDisputeInfo.contains(input.eventId))
        {
            return;
        }

        if (input.vote != 0 && input.vote != 1) // invalid vote
        {
            return;
        }

        state.get().mDisputeResolver.get(input.eventId, locals.dri);
        locals.dri.epochData.set(locals.isComputor, qpi.epoch());
        locals.dri.voteData.set(locals.isComputor, (sint8)(input.vote));
        state.mut().mDisputeResolver.set(input.eventId, locals.dri);

        locals.log = QassandraLoggerWithData{ 0, QASSANDRA_RESOLVE_DISPUTE, id(0,input.eventId, locals.isComputor, input.vote), 0 };
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
        locals.result = QASSANDRA_RESULT_NOT_SET;
        if (locals.voteNo >= QUORUM)
        {
            locals.result = QASSANDRA_RESULT_NO;
            locals.voteCount = locals.voteNo;
        }
        if (locals.voteYes >= QUORUM)
        {
            locals.result = QASSANDRA_RESULT_YES;
            locals.voteCount = locals.voteYes;
        }

        if (locals.result == QASSANDRA_RESULT_NOT_SET)
        {
            return;
        }

        // POT has: mDepositAmountForDispute x 2, from disputer and market operator
        // computors takes 0.3 x POT
        // disputer takes 0.7 x POT
        state.get().mDisputeInfo.get(input.eventId, locals.diDisputer);
        state.get().mGODepositInfo.get(input.eventId, locals.diGO);
        locals.disputeRewardPot = locals.diDisputer.amount + locals.diGO.amount;

        locals.rewardPerComputor = QPI::div(QPI::div(locals.disputeRewardPot * 3LL, 10LL), sint64(locals.voteCount));

        locals.rewardForWinner = locals.disputeRewardPot - locals.rewardPerComputor * sint64(locals.voteCount);
        state.get().mEventResult.get(input.eventId, locals.GOResult);
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
        state.mut().mDisputeInfo.removeByKey(input.eventId);
        state.mut().mGODepositInfo.removeByKey(input.eventId);
        state.mut().mDisputeResolver.removeByKey(input.eventId);

        // finalize the forecast market (guard against double-finalization)
        if (!state.get().mEventFinalFlag.contains(input.eventId))
        {
            locals.fei.eventId = input.eventId;
            locals.fei.winOption = locals.result;
            state.mut().mEventResult.set(input.eventId, locals.result);
            CALL(FinalizeEvent, locals.fei, locals.feo);
        }
    }

    /**
    * Try to finalize a forecast market
    * called by the market operator - at least 24 hours after end date
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
        QdraEventInfo qei;
        QassandraLogger log;
        uint32 publishResultTick;

        FinalizeEvent_input fei;
        FinalizeEvent_output feo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TryFinalizeEvent)
    {
        if (qpi.invocator() != state.get().mQdraGov.mOperationId)
        {
            return;
        }
        if (!state.get().mEventInfo.get(input.eventId, locals.qei))
        {
            return;
        }

        if (state.get().mDisputeInfo.contains(input.eventId))
        {
            // this forecast market is being disputed: users do not agree with the outcome from the market operation team
            return;
        }
        // only finalizing after 1000 ticks since the outcome is published
        state.get().mEventResultPublishTickTime.get(input.eventId, locals.publishResultTick);
        if (locals.publishResultTick + QASSANDRA_DISPUTE_WINDOW > qpi.tick())
        {
            return;
        }

        state.get().mEventResult.get(input.eventId, locals.result);
        if (locals.result == QASSANDRA_RESULT_NOT_SET)
        {
            return;
        }
        if (locals.result == QASSANDRA_RESULT_NO)
        {
            locals.winOption = 0;
        }
        else
        {
            locals.winOption = 1;
        }

        // ALL passed, no dispute, return the deposit to the market operator
        state.get().mGODepositInfo.get(input.eventId, locals.di);
        qpi.transfer(locals.di.pubkey, locals.di.amount);
        state.mut().mGODepositInfo.removeByKey(input.eventId); // clean up

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
        output.operationFee = state.get().mQdraGov.mOperationFee;
        output.gameOperator = state.get().mQdraGov.mOperationId;
        output.shareholderFee = state.get().mQdraGov.mShareHolderFee;
        output.burnFee = state.get().mQdraGov.mBurnFee;
        output.shareholdersRevenue = state.get().mShareholdersRevenue;
        output.operationRevenue = state.get().mOperationRevenue;
        output.nIssuedEvent = state.get().mCurrentEventID;
        output.burnedAmount = state.get().mBurnedAmount;
        output.mFeePerDay = state.get().mQdraGov.mFeePerDay;
        output.antiSpamAmount = state.get().mOperationParams.mAntiSpamAmount;
        output.depositAmountForDispute = state.get().mQdraGov.mDepositAmountForDispute;
    }

    struct GetActiveEvent_input
    {
    };
    struct GetActiveEvent_output
    {
        Array<uint64, QASSANDRA_MAX_CONCURRENT_EVENT> recentActiveEvent;
    };

    struct GetActiveEvent_locals
    {
        sint64 i;
    };
    /**
     * @return a list of active forecast market IDs.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetActiveEvent)
    {
        copyMemory(output.recentActiveEvent, state.get().mRecentActiveEvent);
    }

    struct GetUserPosition_input
    {
        id uid;
    };

    struct GetUserPosition_output
    {
        struct PositionInfo
        {
            uint64 eo; // packed market ID and outcome option bit
            sint64 amount;
        };
        sint64 count;
        Array<PositionInfo, 1024> p;
    };

    struct GetUserPosition_locals
    {
        uint64 k, e;
        id key;
        QdraOrder qo;
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
        for (locals.i = 0; locals.i < QASSANDRA_MAX_CONCURRENT_EVENT; locals.i++)
        {
            if (output.count == output.p.capacity()) break;
            locals.e = state.get().mRecentActiveEvent.get(locals.i);
            locals.key = MakePosKey(input.uid, locals.e, 0);
            if (state.get().mPositionInfo.get(locals.key, locals.qo))
            {
                locals.pi.amount = locals.qo.amount;
                locals.pi.eo = posKey(0, locals.e);
                output.p.set(output.count++, locals.pi);
            }

            locals.key = MakePosKey(input.uid, locals.e, 1);
            if (state.get().mPositionInfo.get(locals.key, locals.qo))
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
    * Create a forecast market for QUBIC/USD or ecosystem milestone forecasting.
    * If the provided info fails validation, funds will be returned to the invocator.
    * @param eventDesc Forecast market description.
    * @param optionDesc Binary forecast outcome descriptions.
    * @param closeDate (4 bytes): date in qassandraData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    * @param endDate (4 bytes): date in qassandraData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    */
    struct CreateEvent_input
    {
        QdraEventInfo qei;
    };
    struct CreateEvent_output
    {
    };
    struct CreateEvent_locals
    {
        DateAndTime dtNow;
        uint64 duration;
        sint64 fee;

        QdraEventInfo qei;
        QassandraLogger log;
        sint32 i;
        DisputeResolveInfo dri;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateEvent)
    {
        // only allow the market operator to create forecast markets
        if (qpi.invocator() != state.get().mQdraGov.mOperationId)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.dtNow = qpi.now();
        if (input.qei.endDate < locals.dtNow)
        {
            locals.log = QassandraLogger{ 0, QASSANDRA_INVALID_DATETIME ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.duration = input.qei.endDate.durationMicrosec(locals.dtNow);
        locals.duration = divUp(locals.duration, 86400000000ULL); // 86400000000 us per day

        locals.fee = locals.duration * state.get().mQdraGov.mFeePerDay;


        // fee is higher than sent amount, exit
        if (locals.fee > qpi.invocationReward())
        {
            locals.log = QassandraLogger{ 0, QASSANDRA_INSUFFICIENT_FUND ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (state.get().mEventInfo.population() == QASSANDRA_MAX_CONCURRENT_EVENT)
        {
            // out of storage
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.qei = input.qei;
        locals.qei.eid = state.mut().mCurrentEventID++;
        locals.qei.openDate = locals.dtNow;

        state.mut().mEventInfo.set(locals.qei.eid, locals.qei);
        state.mut().mEventResult.set(locals.qei.eid, QASSANDRA_RESULT_NOT_SET);
        state.mut().mEventResultPublishTickTime.set(locals.qei.eid, 0);
        state.mut().mDisputeInfo.removeByKey(locals.qei.eid); // clean if any

        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }
        state.mut().mRecentActiveEvent.set(mod(locals.qei.eid, QASSANDRA_MAX_CONCURRENT_EVENT), locals.qei.eid);

        for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
        {
            locals.dri.epochData.set(locals.i, 10000);
            locals.dri.voteData.set(locals.i, QASSANDRA_RESULT_NOT_SET);
        }
        state.mut().mDisputeResolver.set(locals.qei.eid, locals.dri);

        locals.log = QassandraLogger{ 0, QASSANDRA_CREATED_EVENT ,0 };
        LOG_INFO(locals.log);
    }

    struct CreateTypedForecastMarket_input
    {
        QdraEventInfo qei;
        QdraMarketMetadata metadata;
    };
    struct CreateTypedForecastMarket_output
    {
    };
    struct CreateTypedForecastMarket_locals
    {
        DateAndTime dtNow;
        uint64 duration;
        sint64 fee;

        QdraEventInfo qei;
        QassandraLogger log;
        sint32 i;
        DisputeResolveInfo dri;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateTypedForecastMarket)
    {
        // Qassandra-only typed forecast market creation. Generic callers keep using CreateEvent.
        if (qpi.invocator() != state.get().mQdraGov.mOperationId)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.dtNow = qpi.now();
        if (input.qei.endDate < locals.dtNow)
        {
            locals.log = QassandraLogger{ 0, QASSANDRA_INVALID_DATETIME ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.duration = input.qei.endDate.durationMicrosec(locals.dtNow);
        locals.duration = divUp(locals.duration, 86400000000ULL); // 86400000000 us per day

        locals.fee = locals.duration * state.get().mQdraGov.mFeePerDay;

        if (locals.fee > qpi.invocationReward())
        {
            locals.log = QassandraLogger{ 0, QASSANDRA_INSUFFICIENT_FUND ,0 };
            LOG_INFO(locals.log);
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (state.get().mEventInfo.population() == QASSANDRA_MAX_CONCURRENT_EVENT)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (!isMarketMetadataValid(input.metadata))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.qei = input.qei;
        locals.qei.eid = state.mut().mCurrentEventID++;
        locals.qei.openDate = locals.dtNow;

        state.mut().mEventInfo.set(locals.qei.eid, locals.qei);
        state.mut().mMarketMetadata.set(locals.qei.eid, input.metadata);
        state.mut().mEventResult.set(locals.qei.eid, QASSANDRA_RESULT_NOT_SET);
        state.mut().mEventResultPublishTickTime.set(locals.qei.eid, 0);
        state.mut().mDisputeInfo.removeByKey(locals.qei.eid); // clean if any

        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }
        state.mut().mRecentActiveEvent.set(mod(locals.qei.eid, QASSANDRA_MAX_CONCURRENT_EVENT), locals.qei.eid);

        for (locals.i = 0; locals.i < NUMBER_OF_COMPUTORS; locals.i++)
        {
            locals.dri.epochData.set(locals.i, 10000);
            locals.dri.voteData.set(locals.i, QASSANDRA_RESULT_NOT_SET);
        }
        state.mut().mDisputeResolver.set(locals.qei.eid, locals.dri);

        locals.log = QassandraLogger{ 0, QASSANDRA_CREATED_EVENT ,0 };
        LOG_INFO(locals.log);
    }

    struct RequestQubicUsdSettlement_input
    {
        uint64 eventId;
        id oracle;
        id quoteCurrency;
        uint32 timeoutMilliseconds;
    };
    struct RequestQubicUsdSettlement_output
    {
        uint64 errorCode;
        sint64 oracleQueryId;
        uint8 settlementStatus;
        sint64 queryFee;
    };
    struct RequestQubicUsdSettlement_locals
    {
        QdraEventInfo qei;
        QdraMarketMetadata metadata;
        QdraOracleSettlement existingSettlement;
        QdraOracleSettlement settlement;
        DateAndTime targetTimestamp;
        id selectedOracle;
        id selectedQuoteCurrency;
        OI::Price::OracleQuery query;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RequestQubicUsdSettlement)
    {
        output.errorCode = 0;
        output.oracleQueryId = -1;
        output.settlementStatus = QASSANDRA_ORACLE_SETTLEMENT_QUERY_ERROR;
        output.queryFee = 0;

        if (!state.get().mEventInfo.contains(input.eventId) ||
            !state.get().mMarketMetadata.contains(input.eventId))
        {
            output.errorCode = QASSANDRA_INVALID_EVENT_ID;
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        state.get().mEventInfo.get(input.eventId, locals.qei);
        state.get().mMarketMetadata.get(input.eventId, locals.metadata);

        if (!isQubicUsdSettlementRequestMetadataValid(locals.metadata, locals.targetTimestamp) ||
            !resolveQubicUsdSettlementOracle(input.oracle, locals.metadata, locals.selectedOracle) ||
            !resolveQubicUsdQuoteCurrency(input.quoteCurrency, locals.selectedQuoteCurrency))
        {
            output.errorCode = QASSANDRA_INVALID_DATETIME;
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (state.get().mEventFinalFlag.contains(input.eventId))
        {
            output.errorCode = QASSANDRA_INVALID_EVENT_ID;
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (state.get().mOracleSettlement.contains(input.eventId))
        {
            state.get().mOracleSettlement.get(input.eventId, locals.existingSettlement);
            if (locals.existingSettlement.status == QASSANDRA_ORACLE_SETTLEMENT_PENDING ||
                !canRetryQubicUsdSettlement(locals.existingSettlement.status))
            {
                output.settlementStatus = locals.existingSettlement.status;
                if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }

            if (locals.existingSettlement.queryId >= 0)
            {
                state.mut().mOracleQueryToEvent.removeByKey(locals.existingSettlement.queryId);
            }
        }
        else if (state.get().mOracleSettlement.population() == QASSANDRA_MAX_CONCURRENT_EVENT)
        {
            output.errorCode = QASSANDRA_OUT_OF_MEMORY;
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.query.oracle = locals.selectedOracle;
        locals.query.timestamp = locals.targetTimestamp;
        locals.query.currency1 = qubicCurrencyId();
        locals.query.currency2 = locals.selectedQuoteCurrency;
        output.queryFee = OI::Price::getQueryFee(locals.query);

        if (qpi.invocationReward() < output.queryFee)
        {
            output.errorCode = QASSANDRA_INSUFFICIENT_FUND;
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        output.oracleQueryId = QUERY_ORACLE(OI::Price, locals.query, NotifyQubicUsdPriceReply, input.timeoutMilliseconds);
        if (output.oracleQueryId < 0)
        {
            locals.settlement.queryId = output.oracleQueryId;
            locals.settlement.requestedTick = qpi.tick();
            locals.settlement.status = QASSANDRA_ORACLE_SETTLEMENT_QUERY_ERROR;
            locals.settlement.oracleOutcome = QASSANDRA_RESULT_NOT_SET;
            locals.settlement.reserved0 = 0;
            locals.settlement.numerator = 0;
            locals.settlement.denominator = 0;
            state.mut().mOracleSettlement.set(input.eventId, locals.settlement);
            output.errorCode = QASSANDRA_INSUFFICIENT_FUND;
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.settlement.queryId = output.oracleQueryId;
        locals.settlement.requestedTick = qpi.tick();
        locals.settlement.status = QASSANDRA_ORACLE_SETTLEMENT_PENDING;
        locals.settlement.oracleOutcome = QASSANDRA_RESULT_NOT_SET;
        locals.settlement.reserved0 = 0;
        locals.settlement.numerator = 0;
        locals.settlement.denominator = 0;

        state.mut().mOracleSettlement.set(input.eventId, locals.settlement);
        state.mut().mOracleQueryToEvent.set(output.oracleQueryId, input.eventId);
        output.settlementStatus = QASSANDRA_ORACLE_SETTLEMENT_PENDING;
    }

    typedef OracleNotificationInput<OI::Price> NotifyQubicUsdPriceReply_input;
    typedef NoData NotifyQubicUsdPriceReply_output;
    struct NotifyQubicUsdPriceReply_locals
    {
        uint64 eventId;
        QdraMarketMetadata metadata;
        QdraOracleSettlement settlement;
        sint8 oracleOutcome;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(NotifyQubicUsdPriceReply)
    {
        if (!state.get().mOracleQueryToEvent.contains(input.queryId))
        {
            return;
        }

        state.get().mOracleQueryToEvent.get(input.queryId, locals.eventId);
        if (!state.get().mOracleSettlement.contains(locals.eventId) ||
            !state.get().mMarketMetadata.contains(locals.eventId))
        {
            return;
        }

        state.get().mOracleSettlement.get(locals.eventId, locals.settlement);
        state.get().mMarketMetadata.get(locals.eventId, locals.metadata);
        if (locals.metadata.marketType != QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD)
        {
            return;
        }

        locals.settlement.queryId = input.queryId;
        locals.settlement.numerator = 0;
        locals.settlement.denominator = 0;
        locals.settlement.oracleOutcome = QASSANDRA_RESULT_NOT_SET;

        if (input.status == ORACLE_QUERY_STATUS_SUCCESS)
        {
            locals.settlement.numerator = input.reply.numerator;
            locals.settlement.denominator = input.reply.denominator;
            if (mapQubicUsdThresholdOutcome(input.reply, locals.metadata, locals.oracleOutcome))
            {
                locals.settlement.status = QASSANDRA_ORACLE_SETTLEMENT_SUCCESS;
                locals.settlement.oracleOutcome = locals.oracleOutcome;
            }
            else
            {
                locals.settlement.status = QASSANDRA_ORACLE_SETTLEMENT_INVALID_REPLY;
            }
        }
        else
        {
            locals.settlement.status = qassandraSettlementStatusFromOracleStatus(input.status);
        }

        state.mut().mOracleSettlement.set(locals.eventId, locals.settlement);
    }

    struct PublishResult_locals
    {
        QdraEventInfo qei;
        QassandraLoggerWithData log;
        DepositInfo di;
        sint8 existingResult;
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
    * Publish the outcome of a forecast market (market operator only).
    * The market operator is required to LOCK mDepositAmountForDispute for each published outcome.
    * TODO: Replace operator-published outcomes with oracle-first settlement in a later commit.
    * @param eventId (8 bytes)
    * @param option (8 bytes): winning forecast outcome option
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(PublishResult)
    {
        if (qpi.invocator() != state.get().mQdraGov.mOperationId)
        {
            return;
        }
        // deposit
        if (qpi.invocationReward() != state.get().mQdraGov.mDepositAmountForDispute)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (!isOptionValid(input.option))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (!state.get().mEventInfo.get(input.eventId, locals.qei))
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (qpi.now() < locals.qei.endDate)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // prevent re-publishing (deposit loss, timer reset, outcome flip)
        state.get().mEventResult.get(input.eventId, locals.existingResult);
        if (locals.existingResult != QASSANDRA_RESULT_NOT_SET)
        {
            if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.di.pubkey = qpi.invocator();
        locals.di.amount = qpi.invocationReward();
        state.mut().mGODepositInfo.set(input.eventId, locals.di);
        state.mut().mEventResult.set(input.eventId, sint8(input.option));
        state.mut().mEventResultPublishTickTime.set(input.eventId, qpi.tick());
        locals.log = QassandraLoggerWithData{ 0, QASSANDRA_PUBLISH_RESULT, id(0,0,input.eventId,input.option), 0 };
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
        struct QdraOrderWithPrice
        {
            QdraOrder qo;
            sint64 price;
        };
        Array<QdraOrderWithPrice, 256> orders;
    };
    struct GetOrders_locals
    {
        sint64 i, c, sign;
        id key, r;
        GetOrders_output::QdraOrderWithPrice order;
    };
    /**
     * @brief PUBLIC VIEW FUNCTION
     * Retrieves a paginated list of orders from the order book for a specific side of a forecast market.
     * @param eventId The forecast market to query.
     * @param option The binary forecast outcome option (0 or 1).
     * @param isBid 1 for bids (buy orders), 0 for asks (sell orders).
     * @param offset The number of orders to skip (for pagination).
     * @return A list of up to 256 orders with their price and amount.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetOrders)
    {
        setMemory(output, 0);
        if (input.option > 1) return;
        if (!state.get().mEventInfo.contains(input.eventId)) return;
        locals.key = MakeOrderKey(input.eventId, input.option, input.isBid, locals.r);
        locals.i = state.get().mABOrders.headIndex(locals.key);
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
                locals.order.price = locals.sign * state.get().mABOrders.priority(locals.i);
                locals.order.qo = state.get().mABOrders.element(locals.i);
                output.orders.set(locals.c++, locals.order);
            }
            locals.i = state.get().mABOrders.nextElementIndex(locals.i);
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
        QassandraLoggerWithData log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(UpdateFeeDiscountList)
    {
        if (qpi.invocationReward()) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().mQdraGov.mOperationId) return;

        locals.log = QassandraLoggerWithData{ 0, QASSANDRA_UPDATED_NEW_DISCOUNT_FEE, input.userId, 0 };
        LOG_INFO(locals.log);

        if (input.ops == 0)
        {
            state.mut().mOperationParams.discountedFeeForUsers.removeByKey(input.userId);
            return;
        }
        else
        {
            // cap discount rate to prevent underflow in fee calculation
            if (input.newFeeRate > QASSANDRA_PERCENT_DENOMINATOR) return;
            state.mut().mOperationParams.discountedFeeForUsers.set(input.userId, input.newFeeRate);
            return;
        }
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
        REGISTER_USER_FUNCTION(GetTopProposals, 8);

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
        REGISTER_USER_PROCEDURE(CleanMemory, 14);
        REGISTER_USER_PROCEDURE(TransferQDRAGOV, 15);
        REGISTER_USER_PROCEDURE(CreateTypedForecastMarket, 16);
        REGISTER_USER_PROCEDURE(RequestQubicUsdSettlement, 17);
        REGISTER_USER_PROCEDURE_NOTIFICATION(NotifyQubicUsdPriceReply);

        // market operation team procedures
        REGISTER_USER_PROCEDURE(UpdateFeeDiscountList, 20);

        // Shareholder proposals: use standard function/procedure indices
        REGISTER_USER_PROCEDURE(ProposalVote, 100);
    }

    typedef NoData Reinit_input;
    typedef NoData Reinit_output;
    struct Reinit_locals {
        id dst;
        AssetPossessionIterator qdraIterator;
        Asset qdraAsset;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(Reinit)
    {
        state.mut().mOperationParams.mAntiSpamAmount = 11;
        state.mut().mOperationParams.discountedFeeForUsers.cleanup();
        setMemory(state.mut().mQdraGov, 0);
        state.mut().mQdraGov.mOperationId = ID(_V, _A, _E, _F, _X, _O, _E, _V, _K, _P, _O, _X, _B, _A, _R, _T, _U, _T, _B, _M, _C, _P, _L, _T, _V, _G, _I, _B, _Z, _N, _C, _Y, _K, _K, _W, _X, _U, _V, _F, _N, _G, _G, _Q, _M, _F, _P, _X, _E, _E, _F, _X, _F, _F, _O, _E, _E);
        state.mut().mQdraGov.mBurnFee = 0;
        state.mut().mQdraGov.mOperationFee = 50; // 5%
        state.mut().mQdraGov.mShareHolderFee = 0; // 0%
        state.mut().mQdraGov.mDepositAmountForDispute = 1000000000;
        state.mut().mRecentActiveEvent.setAll(NULL_INDEX);
        state.mut().wholeSharePrice = 100000;

        qpi.issueAsset(QASSANDRA_GOV_ASSET_NAME, SELF, 0, 676, 0); // mQDRAGOVIdentifier
        state.mut().mQDRAGOVIdentifier.assetName = QASSANDRA_GOV_ASSET_NAME;
        state.mut().mQDRAGOVIdentifier.issuer = SELF;

        // temp replacement for mQUSDIdentifier
        state.mut().mQUSDIdentifier.assetName = 310652322119ULL; // GARTH
        state.mut().mQUSDIdentifier.issuer = ID(_P, _H, _O, _E, _N, _I, _X, _C, _L, _Q, _O, _B, _H, _D, _Z, _C, _H, _J, _O, _C, _K, _C, _P, _Z, _V, _T, _K, _A, _L, _Q, _B, _M, _X, _Y, _O, _E, _D, _B, _U, _H, _S, _D, _C, _J, _R, _M, _T, _U, _C, _U, _B, _P, _L, _S, _U, _F);
        
        // distribute mQDRAGOVIdentifier to current shareholders
        locals.qdraAsset.assetName = QASSANDRA_CONTRACT_ASSET_NAME;
        locals.qdraAsset.issuer = NULL_ID;

        // distribute QDRA GOV to current shareholders
        for (locals.qdraIterator.begin(locals.qdraAsset); !locals.qdraIterator.reachedEnd(); locals.qdraIterator.next())
        {
            if (locals.qdraIterator.numberOfOwnedShares() > 0)
            {
                qpi.transferShareOwnershipAndPossession(state.mut().mQDRAGOVIdentifier.assetName, state.mut().mQDRAGOVIdentifier.issuer, SELF, SELF, locals.qdraIterator.numberOfOwnedShares(), locals.qdraIterator.owner());
            }
        }
    }

    typedef NoData CleanMemory_input;
    typedef NoData CleanMemory_output;
    struct CleanMemory_locals
    {
        sint64 index;
        uint64 eid;
        id key;
        QdraOrder v;
        sint8 userOption;
        QdraEventInfo qei;
        id uid;
        sint8 winOption;
        RewardTransfer_input rti;
        RewardTransfer_output rto;
        sint32 i;
        bit flag;
        QassandraLoggerWithData log;
        uint32 publishResultTick;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(CleanMemory)
    {
        // Only the system or market operator can call this
        if (qpi.invocator() == NULL_ID || qpi.invocator() == state.get().mQdraGov.mOperationId)
        {
            // payout all positions that have final outcomes and then clean all finalized forecast markets
            locals.index = NULL_INDEX;
            do {
                locals.index = state.get().mPositionInfo.nextElementIndex(locals.index);
                if (locals.index != NULL_INDEX)
                {
                    locals.key = state.get().mPositionInfo.key(locals.index);
                    locals.v = state.get().mPositionInfo.value(locals.index);
                    locals.eid = locals.key.u64._3 & QASSANDRA_EID_MASK;
                    locals.userOption = locals.key.u64._3 >> 63;
                    state.get().mEventResult.get(locals.eid, locals.winOption);
                    if (locals.winOption != QASSANDRA_RESULT_NOT_SET)
                    {
                        // skip forecast markets that are currently under active dispute
                        if (state.get().mDisputeInfo.contains(locals.eid))
                        {
                            continue;
                        }
                        // outcome is available, now checking if it's still in dispute window
                        state.get().mEventInfo.get(locals.eid, locals.qei);
                        // only finalizing after 1000 ticks since outcome publication
                        state.get().mEventResultPublishTickTime.get(locals.eid, locals.publishResultTick);
                        if (locals.publishResultTick + QASSANDRA_DISPUTE_WINDOW < qpi.tick()) // 1000 ticks passed the outcome publication
                        {
                            if (!state.get().mEventFinalFlag.contains(locals.eid))
                            {
                                state.mut().mEventFinalFlag.set(locals.eid, true);
                            }
                            if (locals.userOption == locals.winOption)
                            {
                                locals.rti.amount = smul(state.get().wholeSharePrice, locals.v.amount);
                                locals.rti.eid = locals.eid;
                                locals.rti.receiver = locals.v.entity;
                                locals.rti.needChargeFee = 1;
                                CALL(RewardTransfer, locals.rti, locals.rto);
                                if (!locals.rto.ok)
                                {
                                    state.mut().mEventFinalFlag.set(locals.eid, false); // can't finalize this because failed to give reward to users
                                    continue;
                                }
                            }
                            // remove the position
                            state.mut().mPositionInfo.removeByKey(locals.key);
                        }
                    }
                }
            } while (locals.index != NULL_INDEX);

            locals.index = NULL_INDEX;
            do {
                locals.index = state.get().mEventFinalFlag.nextElementIndex(locals.index);
                if (locals.index != NULL_INDEX)
                {
                    locals.eid = state.get().mEventFinalFlag.key(locals.index);
                    if (!state.get().mEventFinalFlag.value(locals.index)) // flag as false
                    {
                        state.mut().mEventFinalFlag.removeByIndex(locals.index); // clean it for next epoch
                        continue;
                    }
                    locals.log = QassandraLoggerWithData{ 0, QASSANDRA_ARCHIVE_EVENT, id(0,0, 0, locals.eid), 0 };
                    LOG_INFO(locals.log);

                    // Cleanup the outcome, freeing the slot in mEventResult
                    state.mut().mEventResult.removeByKey(locals.eid);
                    state.mut().mEventResultPublishTickTime.removeByKey(locals.eid);

                    // Safety cleanup just in case
                    state.mut().mDisputeInfo.removeByKey(locals.eid);
                    state.mut().mGODepositInfo.removeByKey(locals.eid);
                    state.mut().mDisputeResolver.removeByKey(locals.eid);
                    state.mut().mEventInfo.removeByKey(locals.eid);
                    state.mut().mMarketMetadata.removeByKey(locals.eid);
                    state.mut().mEventFinalFlag.removeByIndex(locals.index);

                    // Clear the active forecast market slot so GetActiveEvent/GetUserPosition don't see stale IDs
                    state.mut().mRecentActiveEvent.set(mod(locals.eid, QASSANDRA_MAX_CONCURRENT_EVENT), NULL_INDEX);
                }
            } while (locals.index != NULL_INDEX);

            state.mut().mEventInfo.cleanup();
            state.mut().mMarketMetadata.cleanup();
            state.mut().mEventFinalFlag.cleanup();
            state.mut().mPositionInfo.cleanup();
            state.mut().mEventResult.cleanup();
            state.mut().mEventResultPublishTickTime.cleanup();
            state.mut().mDisputeInfo.cleanup();
            state.mut().mGODepositInfo.cleanup();
            state.mut().mDisputeResolver.cleanup();
        }
    }

    BEGIN_EPOCH()
    {
        // TODO: reinitialize after proposal getting passed
        if (qpi.epoch() == 210)
        {
            CALL(Reinit, input, output);
        }
    }

    struct END_EPOCH_locals
    {
        AssetPossessionIterator iter;
        Asset asset;
        uint64 payoutPerShare, payout, total, burn;

        CleanMemory_input cmi;
        CleanMemory_output cmo;
    };
    END_EPOCH_WITH_LOCALS()
    {
        CALL(CleanMemory, locals.cmi, locals.cmo);
        // distribute to QDRA shareholders
        if ((state.get().mShareholdersRevenue - state.get().mDistributedShareholdersRevenue - state.get().mBurnedAmount > 676) && (state.get().mShareholdersRevenue > state.get().mDistributedShareholdersRevenue + state.get().mBurnedAmount))
        {
            // burn fee will be applied on shareholder revenue
            locals.total = state.get().mShareholdersRevenue - state.get().mDistributedShareholdersRevenue - state.get().mBurnedAmount;
            locals.burn = div(smul(locals.total, state.get().mQdraGov.mBurnFee), QASSANDRA_PERCENT_DENOMINATOR);
            if (locals.burn) qpi.burn(locals.burn);

            state.mut().mBurnedAmount += locals.burn;
            locals.total = state.get().mShareholdersRevenue - state.get().mDistributedShareholdersRevenue - state.get().mBurnedAmount;
            locals.payoutPerShare = div(locals.total, (uint64)NUMBER_OF_COMPUTORS);

            locals.asset.assetName = QASSANDRA_CONTRACT_ASSET_NAME;
            locals.asset.issuer = NULL_ID;

            if (locals.payoutPerShare)
            {
                // distribute dividends by tokens, not QUs
                for (locals.iter.begin(locals.asset); !locals.iter.reachedEnd(); locals.iter.next())
                {
                    if (locals.iter.numberOfPossessedShares() > 0)
                    {
                        locals.payout = smul(locals.payoutPerShare, (uint64)locals.iter.numberOfPossessedShares());
                        if (qpi.transferShareOwnershipAndPossession(state.get().mQUSDIdentifier.assetName, state.get().mQUSDIdentifier.issuer, SELF, SELF, locals.payout, locals.iter.possessor()) < 0)
                        {
                            //critical error
                        }
                        else
                        {
                            state.mut().mDistributedShareholdersRevenue += locals.payout;
                        }
                    }
                }
            }
        }
        // distribute to market operation team
        if (state.get().mOperationRevenue > state.get().mDistributedOperationRevenue)
        {
            locals.payout = state.get().mOperationRevenue - state.get().mDistributedOperationRevenue;
            if (qpi.transferShareOwnershipAndPossession(state.get().mQUSDIdentifier.assetName, state.get().mQUSDIdentifier.issuer, SELF, SELF, locals.payout, state.get().mQdraGov.mOperationId) < 0)
            {
                //critical error
            }
            else
            {
                state.mut().mDistributedOperationRevenue += locals.payout;
            }
        }

        CALL(FinalizeProposalAndGovernment, input, output);
    }

    /* GOV/PROPOSAL AREA */

    struct ProposalVote_input
    {
        QdraGOV proposed;
    };
    struct ProposalVote_output {};
    struct ProposalVote_locals
    {
        uint64 amountOfShares;
        sint32 i;
        sint32 chosenIndex;
        id publicKey;
        proposalVoter pv;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(ProposalVote)
    {
        if (state.get().mOperationParams.mAntiSpamAmount)
        {
            if (qpi.invocationReward() < state.get().mOperationParams.mAntiSpamAmount) return;
            if (qpi.invocationReward() > state.get().mOperationParams.mAntiSpamAmount) qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().mOperationParams.mAntiSpamAmount);
        }
        locals.amountOfShares = qpi.numberOfShares(state.get().mQDRAGOVIdentifier, { qpi.invocator(), SELF_INDEX }, { qpi.invocator(), SELF_INDEX });
        if (locals.amountOfShares == 0) return;
        // check duplicated publickey
        locals.chosenIndex = -1;
        for (locals.i = 0; locals.i < 676; locals.i++)
        {
            if (state.get().mGovVoters.get(locals.i).publicKey == qpi.invocator())
            {
                locals.chosenIndex = locals.i;
                break;
            }
        }
        if (locals.chosenIndex == -1)
        {
            // get any spot that is not valid:  epoch != qpi.epoch
            for (locals.i = 0; locals.i < 676; locals.i++)
            {
                if (state.get().mGovVoters.get(locals.i).proposedEpoch != qpi.epoch())
                {
                    locals.chosenIndex = locals.i;
                    break;
                }
            }
        }
        if (locals.chosenIndex != -1)
        {
            locals.pv.publicKey = qpi.invocator();
            locals.pv.amountOfShares = locals.amountOfShares;
            locals.pv.proposed = input.proposed;
            locals.pv.proposedEpoch = qpi.epoch();
            state.mut().mGovVoters.set(locals.chosenIndex, locals.pv);
        }
        // update old votes, invalidate the slot if needed
        for (locals.i = 0; locals.i < 676; locals.i++)
        {
            locals.pv = state.get().mGovVoters.get(locals.i);
            if (locals.pv.proposedEpoch == qpi.epoch())
            {
                locals.amountOfShares = qpi.numberOfShares(state.get().mQDRAGOVIdentifier, { locals.pv.publicKey, SELF_INDEX }, { locals.pv.publicKey, SELF_INDEX });
                if (locals.amountOfShares == 0)
                {
                    locals.pv.proposedEpoch = 0;
                    state.mut().mGovVoters.set(locals.i, locals.pv);
                }
            }
        }
    }

    typedef NoData FinalizeProposalAndGovernment_input;
    typedef NoData FinalizeProposalAndGovernment_output;
    struct FinalizeProposalAndGovernment_locals
    {
        sint32 i;
        sint32 currentPoint;

        proposalVoter pv;
        id proposalHash;

        QdraGOV maxVotedProposal;
        sint32 maxVoteCount;

        AssetOwnershipIterator aoi;
        id owner;
        Entity e;
        bit shouldRedistribute;
        sint64 amountOfShares;

        sint32 govCount;
        sint64 govSum;
        sint64 redistributeSum;

        sint64 prev;
        id seed;
        sint32 rd;
    };
    PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeProposalAndGovernment)
    {
        // clear stale vote counts from previous epochs
        state.mut().mVoteMap.reset();
        locals.maxVoteCount = -1;
        for (locals.i = 0; locals.i < 676; locals.i++)
        {
            locals.pv = state.get().mGovVoters.get(locals.i);
            if (locals.pv.proposedEpoch == qpi.epoch())
            {
                locals.pv.amountOfShares = qpi.numberOfShares(state.get().mQDRAGOVIdentifier, { locals.pv.publicKey, SELF_INDEX }, { locals.pv.publicKey, SELF_INDEX });
                locals.proposalHash = qpi.K12(locals.pv.proposed);
                if (!state.get().mVoteMap.get(locals.proposalHash, locals.currentPoint))
                {
                    locals.currentPoint = 0;
                }
                state.mut().mVoteMap.set(locals.proposalHash, locals.currentPoint + sint32(locals.pv.amountOfShares));
                if (locals.maxVoteCount < locals.currentPoint + sint32(locals.pv.amountOfShares))
                {
                    locals.maxVoteCount = locals.currentPoint + sint32(locals.pv.amountOfShares);
                    locals.maxVotedProposal = locals.pv.proposed;
                }
            }
        }
        if (locals.maxVoteCount >= 451)
        {
            state.mut().mQdraGov = locals.maxVotedProposal;
        }
        // reset all vote slots
        for (locals.i = 0; locals.i < 676; locals.i++)
        {
            locals.pv = state.get().mGovVoters.get(locals.i);
            locals.pv.proposedEpoch = 0;
            state.mut().mGovVoters.set(locals.i, locals.pv);
        }
        // checking for GOV holders activities - here we only transfer both ownership and possession at the same time, so we only need to check the ownership
        locals.aoi = AssetOwnershipIterator(state.get().mQDRAGOVIdentifier);
        locals.govCount = 0;
        locals.govSum = 0;
        locals.redistributeSum = qpi.numberOfShares(state.get().mQDRAGOVIdentifier, { SELF, SELF_INDEX }, { SELF, SELF_INDEX });

        while (!locals.aoi.reachedEnd())
        {
            if (locals.aoi.owner() != SELF) // only process if holder isn't QDRA itself
            {
                locals.shouldRedistribute = false;
                if (qpi.getEntity(locals.aoi.owner(), locals.e))
                {
                    if (qpi.tick() - 5000000 >= locals.e.latestOutgoingTransferTick)
                    {
                        // no outgoing last 5000000 ticks, need to move the gov token
                        locals.shouldRedistribute = true;
                    }
                }
                else
                {
                    // this ID doesn't even have 1QU, need to move the gov token
                    locals.shouldRedistribute = true;
                }
                locals.amountOfShares = qpi.numberOfShares(state.get().mQDRAGOVIdentifier, { locals.aoi.owner(), SELF_INDEX }, { locals.aoi.owner(), SELF_INDEX });
                if (locals.shouldRedistribute)
                {
                    // transfer this amount of GOV token to this SC
                    qpi.transferShareOwnershipAndPossession(state.get().mQDRAGOVIdentifier.assetName, state.get().mQDRAGOVIdentifier.issuer, locals.aoi.owner(), locals.aoi.owner(), locals.amountOfShares, SELF);
                    locals.redistributeSum += locals.amountOfShares;
                }
                else
                {
                    state.mut().mGovArray.set(locals.govCount, { locals.aoi.owner() , locals.amountOfShares });
                    locals.govCount++;
                    locals.govSum += locals.amountOfShares;
                }
            }
            locals.aoi.next();
        }
        if (locals.redistributeSum && locals.govCount > 0 && locals.govSum > 0)
        {
            state.mut().mAccumulatedSum.set(0, state.get().mGovArray.get(0).amount);
            for (locals.i = 1; locals.i < locals.govCount; locals.i++)
            {
                locals.prev = state.get().mAccumulatedSum.get(locals.i - 1);
                state.mut().mAccumulatedSum.set(locals.i, locals.prev + state.get().mGovArray.get(locals.i).amount);
            }
            // get the pseudo random from spectrum
            locals.seed = qpi.getPrevSpectrumDigest();
            //randomly airdrop the inactive token to active holders
            while (locals.redistributeSum)
            {
                locals.seed = qpi.K12(locals.seed);
                locals.rd = mod(locals.seed.u32._0, uint32(locals.govSum));
                for (locals.i = 0; locals.i < locals.govCount; locals.i++)
                {
                    if (locals.rd < state.get().mAccumulatedSum.get(locals.i))
                    {
                        // airdrop from SC to active holder
                        locals.owner = state.get().mGovArray.get(locals.i).publicKey;
                        qpi.transferShareOwnershipAndPossession(state.get().mQDRAGOVIdentifier.assetName, state.get().mQDRAGOVIdentifier.issuer, SELF, SELF, 1, locals.owner);
                        locals.redistributeSum--;
                        break;
                    }
                }
            }
        }

    }
    /******END OF GOV/PROPOSAL AREA*****/
public:

    struct GetTopProposals_input
    {
    };
    struct GetTopProposals_output
    {
        struct ProposalInfo
        {
            QdraGOV proposed;
            sint64 totalVotes;
        };
        Array<ProposalInfo, 4> top; // top 3 proposals (index 3 unused)
        sint32 uniqueCount; // total number of distinct proposals this epoch
    };
    struct ProposalEntry
    {
        id hash;
        QdraGOV proposed;
        sint64 votes;
    };

    struct GetTopProposals_locals
    {
        Array<ProposalEntry, 64> entries;
        sint32 entryCount;
        sint32 i, j;
        proposalVoter pv;
        id h;
        bit found;
        sint64 amountOfShares;
        ProposalEntry tmpEntry;
        // for top-3 selection
        sint32 bestIdx;
        sint64 bestVotes;
        Array<sint8, 64> used; // 0=not used, 1=used
        GetTopProposals_output::ProposalInfo pi;
    };

    /**
     * @brief Returns the top 3 governance proposals by total weighted votes in the current epoch.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetTopProposals)
    {
        setMemory(output, 0);
        setMemory(locals.entries, 0);
        locals.entryCount = 0;

        // accumulate votes per unique proposal hash
        for (locals.i = 0; locals.i < 676; locals.i++)
        {
            locals.pv = state.get().mGovVoters.get(locals.i);
            if (locals.pv.proposedEpoch != qpi.epoch()) continue;

            locals.amountOfShares = qpi.numberOfShares(state.get().mQDRAGOVIdentifier, { locals.pv.publicKey, SELF_INDEX }, { locals.pv.publicKey, SELF_INDEX });
            if (locals.amountOfShares == 0) continue;

            locals.h = qpi.K12(locals.pv.proposed);
            locals.found = false;
            for (locals.j = 0; locals.j < locals.entryCount; locals.j++)
            {
                if (locals.entries.get(locals.j).hash == locals.h)
                {
                    locals.found = true;
                    locals.tmpEntry = locals.entries.get(locals.j);
                    locals.tmpEntry.votes += locals.amountOfShares;
                    locals.entries.set(locals.j, locals.tmpEntry);
                    break;
                }
            }
            if (!locals.found && locals.entryCount < 64)
            {
                locals.tmpEntry.hash = locals.h;
                locals.tmpEntry.proposed = locals.pv.proposed;
                locals.tmpEntry.votes = locals.amountOfShares;
                locals.entries.set(locals.entryCount, locals.tmpEntry);
                locals.entryCount++;
            }
        }
        output.uniqueCount = locals.entryCount;

        // pick top 3
        setMemory(locals.used, 0);
        for (locals.i = 0; locals.i < 3 && locals.i < locals.entryCount; locals.i++)
        {
            locals.bestIdx = -1;
            locals.bestVotes = -1;
            for (locals.j = 0; locals.j < locals.entryCount; locals.j++)
            {
                if (!locals.used.get(locals.j) && locals.entries.get(locals.j).votes > locals.bestVotes)
                {
                    locals.bestVotes = locals.entries.get(locals.j).votes;
                    locals.bestIdx = locals.j;
                }
            }
            if (locals.bestIdx != -1)
            {
                locals.pi.proposed = locals.entries.get(locals.bestIdx).proposed;
                locals.pi.totalVotes = locals.entries.get(locals.bestIdx).votes;
                output.top.set(locals.i, locals.pi);
                locals.used.set(locals.bestIdx, true);
            }
        }
    }

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
            state.get().mQUSDIdentifier,
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
    PUBLIC_PROCEDURE(TransferQUSD) // asset managed by this contract
    {
        if (input.amount <= 0) { output.amount = -1; return; }
        if (qpi.transferShareOwnershipAndPossession(state.get().mQUSDIdentifier.assetName, state.get().mQUSDIdentifier.issuer, qpi.invocator(), qpi.invocator(), input.amount, input.receiver) < 0)
        {
            output.amount = -1;
        }
        else
        {
            output.amount = input.amount;
        }
    }

    struct TransferQDRAGOV_input
    {
        id receiver;
        sint64 amount;
    };
    struct TransferQDRAGOV_output
    {
        uint64 amount;
    };
    PUBLIC_PROCEDURE(TransferQDRAGOV) // asset managed by this contract
    {
        if (input.amount <= 0) { output.amount = -1; return; }
        if (qpi.transferShareOwnershipAndPossession(state.get().mQDRAGOVIdentifier.assetName, state.get().mQDRAGOVIdentifier.issuer, qpi.invocator(), qpi.invocator(), input.amount, input.receiver) < 0)
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
        // only allow to transfer mgmt right from mQUSDIdentifier
        if (input.asset.assetName != state.get().mQUSDIdentifier.assetName || input.asset.issuer != state.get().mQUSDIdentifier.issuer)
        {
            return;
        }
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
