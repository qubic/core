// adjustable with type changes
constexpr unsigned long long QUOTTERY_MAX_BET = 1024; //tmp, will delete
constexpr unsigned long long QUOTTERY_MAX_OPTION = 8;
constexpr unsigned long long QUOTTERY_MAX_ORACLE_PROVIDER = 8;
constexpr unsigned long long QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET = 2048;

// adjustable with no change:
constexpr unsigned long long QUOTTERY_FEE_PER_SLOT_PER_DAY_ = 10000ULL;
constexpr unsigned long long QUOTTERY_MIN_AMOUNT_PER_BET_SLOT_ = 10000ULL;
constexpr unsigned long long QUOTTERY_SHAREHOLDER_FEE_ = 1000;
constexpr unsigned long long QUOTTERY_GAME_OPERATOR_FEE_ = 50;




enum QuotteryLogInfo {
    invalidMaxBetSlotPerOption=0,
    invalidOption = 1,
    invalidBetAmount = 2,
    invalidDate = 3,
    outOfStorage = 4,
    insufficientFund = 5,
    invalidBetId = 6,
    expiredBet = 7,
    invalidNumberOfOracleProvider = 8,
    outOfSlot = 9,
    invalidOPId = 10,
    notEnoughVote = 11,
    notGameOperator = 12,
    totalError = 13
};
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

struct QUOTTERY
{
public:
    /**************************************/
    /********INPUT AND OUTPUT STRUCTS******/
    /**************************************/
    struct basicInfo_input
    {
    };
    struct basicInfo_output
    {
        uint64 feePerSlotPerDay; // Amount of qus
        uint64 gameOperatorFee; // Amount of qus
        uint64 shareholderFee; // 4 digit number ABCD means AB.CD% | 1234 is 12.34%
        uint64 minBetSlotAmount; // 4 digit number ABCD means AB.CD% | 1234 is 12.34%
        id gameOperator;
    };
    struct getBetInfo_input {
        uint32 betId;
    };
    struct getBetInfo_output {
        // meta data info
        uint32 betId;
        uint32 nOption;      // options number
        id creator;
        id betDesc;      // 32 bytes 
        id_8 optionDesc;  // 8x(32)=256bytes
        id_8 oracleProviderId; // 256x8=2048bytes
        uint32_8 oracleFees;   // 4x8 = 32 bytes

        uint8_4 openDate;     // creation date, start to receive bet
        uint8_4 closeDate;    // stop receiving bet date
        uint8_4 endDate;       // result date
        // Amounts and numbers
        uint64 minBetAmount;
        uint32 maxBetSlotPerOption;
        uint32_8 currentBetState; // how many bet slots have been filled on each option
    };
    struct issueBet_input {
        id betDesc;
        id_8 optionDesc;
        id_8 oracleProviderId; // 256x8=2048bytes
        uint32_8 oracleFees;   // 4x8 = 32 bytes
        uint8_4 closeDate;
        uint8_4 endDate;
        uint64 amountPerSlot;
        uint32 maxBetSlotPerOption;
        uint32 numberOfOption;
    };
    struct issueBet_output {
    };
    struct joinBet_input {
        uint32 betId;
        uint32 numberOfSlot;
        uint32 option;
        uint32 _placeHolder;
    };
    struct joinBet_output {
    };
    struct getBetOptionDetail_input {
        uint32 betId;
        uint32 betOption;
    };
    struct getBetOptionDetail_output {
        id_1024 bettor;
    };
    struct getActiveBet_input {
    };
    struct getActiveBet_output {
        uint32 count;
        uint32_1024 activeBetId;
    };
    struct getBetByCreator_input {
        id creator;
    };
    struct getBetByCreator_output {
        uint32 count;
        uint32_1024 betId;
    };
    struct cancelBet_input {
        uint32 betId;
    };
    struct cancelBet_output {
    };
    struct publishResult_input {
        uint32 betId;
        uint32 option;
    };
    struct publishResult_output {
    };
    struct tryFinalizeBet_input {
        uint32 betId;
        sint64 slotId;
    };
    struct tryFinalizeBet_output {
    };
    tryFinalizeBet_input _tryFinalizeBet_input;
    tryFinalizeBet_output _tryFinalizeBet_output;
    struct cleanMemorySlot_input {
        sint64 slotId;
    };
    struct cleanMemorySlot_output {
    };
    cleanMemorySlot_input _cleanMemorySlot_input;
    cleanMemorySlot_output _cleanMemorySlot_output;

    /**************************************/
    /************CONTRACT STATES***********/
    /**************************************/

    uint32_1024x mBetID;
    static_assert(sizeof(mBetID) == (sizeof(uint32) * QUOTTERY_MAX_BET), "bet id array");
    id_1024x  mCreator;
    static_assert(sizeof(mCreator) == (sizeof(id) * QUOTTERY_MAX_BET), "creator array");
    id_1024x mBetDesc;
    static_assert(sizeof(mBetDesc) == (sizeof(id) * QUOTTERY_MAX_BET), "desc array");
    id_8192x mOptionDesc;
    static_assert(sizeof(mOptionDesc) == (sizeof(id) * QUOTTERY_MAX_BET * QUOTTERY_MAX_OPTION), "option desc array");
    uint64_1024x mBetAmountPerSlot;
    static_assert(sizeof(mBetAmountPerSlot) == (sizeof(uint64) * QUOTTERY_MAX_BET), "bet amount per slot array");
    uint64_1024x mMaxNumberOfBetSlotPerOption;
    static_assert(sizeof(mMaxNumberOfBetSlotPerOption) == (sizeof(uint64) * QUOTTERY_MAX_BET), "number of bet slot per option array");
    id_8192x mOracleProvider; // 1024x8=8192, access by stride8
    static_assert(sizeof(mOracleProvider) == (sizeof(QPI::id) * QUOTTERY_MAX_BET * QUOTTERY_MAX_ORACLE_PROVIDER), "oracle providers");
    uint32_8192x mOracleFees;
    static_assert(sizeof(mOracleFees) == (sizeof(uint32) * QUOTTERY_MAX_BET * QUOTTERY_MAX_ORACLE_PROVIDER), "oracle providers fees");
    uint32_8192x mCurrentBetState; // 8xuint32 per bet, each represent number of bettor for according option
    static_assert(sizeof(mCurrentBetState) == (sizeof(uint32) * QUOTTERY_MAX_BET * QUOTTERY_MAX_ORACLE_PROVIDER), "bet states");
    uint8_1024x mNumberOption;
    static_assert(sizeof(mNumberOption) == (sizeof(uint8) * QUOTTERY_MAX_BET), "number of options");
    uint8_4096x mOpenDate; // take 00:00 UTC | 4xuint8 per bet
    static_assert(sizeof(mOpenDate) == (sizeof(uint8) * 4 * QUOTTERY_MAX_BET), "open date");
    uint8_4096x mCloseDate; // take 23:59 UTC
    static_assert(sizeof(mCloseDate) == (sizeof(uint8) * 4 * QUOTTERY_MAX_BET), "close date");
    uint8_4096x mEndDate; // take 23:59 UTC
    static_assert(sizeof(mEndDate) == (sizeof(uint8) * 4 * QUOTTERY_MAX_BET), "end date");
    bit_1024x mIsOccupied;
    static_assert(sizeof(mIsOccupied) == (QUOTTERY_MAX_BET/ ( sizeof(uint8_64) / sizeof(bit_64)) ), "occupy array");

    // bet result:
    sint8_8192x mBetResultWonOption; // 8xuint8 per bet
    static_assert(sizeof(mBetResultWonOption) == (QUOTTERY_MAX_BET * 8), "won option array");
    sint8_8192x mBetResultOPId; // 8xuint8 per bet
    static_assert(sizeof(mBetResultOPId) == (QUOTTERY_MAX_BET * 8), "op id array");

    //bettor struct:
    id_16777216x mBettorID;
    static_assert(sizeof(mBettorID) == (QUOTTERY_MAX_BET * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION * sizeof(id)), "bettor array");
    uint8_16777216x mBettorBetOption;
    static_assert(sizeof(mBettorBetOption) == (QUOTTERY_MAX_BET * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION * sizeof(uint8)), "bet option");
    // other stats
    uint32 mCurrentBetID;
    uint64 mRevenue;
    uint64 mRevenueFromIssue;
    uint64 mRevenueFromBet;
    uint64 mEarnedAmountForShareHolder;
    uint64 mPaidAmountForShareHolder;
    uint64 mEarnedAmountForBetWinner;
    uint64 mDistributedAmount;
    uint64 mBurnedAmount;

    // adjustable variables
    uint64 mFeePerSlotPerDay;
    uint64 mMinAmountPerBetSlot;
    uint64 mShareHolderFee;
    uint64 mGameOperatorFee;
    id mGameOperatorId;

    // temp registers and buffer will be used for computation
    struct {
        sint32 i0, i1, i2, i3;
        uint32 u0, u1, u2, u3;
        sint64 i64_0, i64_1, i64_2, i64_3;
        uint64 u64_0, u64_1, u64_2, u64_3;
        sint32 numberOP, requiredVote, winOption, totalOption, voteCounter, numberOfSlot, currentState;
        sint64 slotId;
        sint32 opId, writeId;
        uint64 baseId0, baseId1, nOption;
        uint64 amountPerSlot, totalBetSlot, potAmountTotal, feeChargedAmount, transferredAmount, fee, profitPerBetSlot, nWinBet;
        uint64 duration, numberOfOption, maxBetSlotPerOption;
        uint32 betId, availableSlotForBet;
        id_256 buffer;
        uint8_4 curDate;
        uint8_4 closeDate;
        uint8_4 endDate;
        QuotteryLogger log;
    } r;
    


    /**************************************/
    /************UTIL FUNCTIONS************/
    /**************************************/
    static uint32 divUp(uint32 a, uint32 b)
    {
        return b ? ((a + b - 1) / b) : 0;
    }
    static sint32 min(sint32 a, sint32 b)
    {
        return (a < b) ? a : b;
    }
    /**
     * Return all sent amount to invocator
     */
    static inline void returnAllFund() {
        transfer(QPI::invocator(), QPI::invocationReward());
    }

    /**
     * Compare 2 date in uint8_4 format
     * @return -1 lesser(ealier) A<B, 0 equal A=B, 1 greater(later) A>B
     */
    static sint32 dateCompare(uint8_4& A, uint8_4& B, sint32& i) {
        if (A.get(0) == B.get(0) && A.get(1) == B.get(1) && A.get(2) == B.get(2)) {
            return 0;
        }
        for (i = 0; i < 3; i++) {
            if (A.get(i) != B.get(i)) {
                if (A.get(i) < B.get(i)) return -1;
                else return 1;
            }
        }
        return 0;
    }

    /**
     * @return Current date from core node system
     */
    static void getCurrentDate(uint8_4& res) {
        res.set(0, year());
        res.set(1, month());
        res.set(2, day());
        res.set(3, 0);
    }

    /**
     * @return uint8_4 variable from year, month, day
     */
    static void makeQuotteryDate(uint8 _year, uint8 _month, uint8 _day, uint8_4& res) {
        res.set(0, _year);
        res.set(1, _month);
        res.set(2, _day);
        res.set(3, 0);
    }


    static void accumulatedDay(sint32 month, uint64& res)
    {
        switch (month)
        {
            case 1: res = 0; break;
            case 2: res = 31; break;
            case 3: res = 59; break;
            case 4: res = 90; break;
            case 5: res = 120; break;
            case 6: res = 151; break;
            case 7: res = 181; break;
            case 8: res = 212; break;
            case 9: res = 243; break;
            case 10:res = 273; break;
            case 11:res = 304; break;
            case 12:res = 334; break;
        }
    }
    /**
     * @return difference in number of day, A must be smaller than or equal B to have valid value
     */
    static void diffDate(uint8_4& A, uint8_4& B, sint32& i, sint32& baseYear, uint64& dayA, uint64& dayB, uint64& res) {
        if (dateCompare(A, B, i) >= 0) {
            res = 0;
            return;
        }
        baseYear = A.get(0);
        accumulatedDay(A.get(1), dayA);
        dayA += A.get(2);
        accumulatedDay(B.get(1), dayB);
        dayB += (B.get(0) - baseYear) * 365ULL + B.get(2);

        // handling leap-year: only store last 2 digits of year here, don't care about mod 100 & mod 400 case
        for (i = baseYear; i < B.get(0); i++) {
            if (mod(i,4) == 0) {
                dayB++;
            }
        }
        if (mod(sint32(B.get(0)), 4) == 0 && B.get(1) > 2) dayB++;
        res = dayB - dayA;
    }

    /**
     * Clean all memory of a slot Id, set the flag IsOccupied to zero
     * @param slotId
     */
    PRIVATE(cleanMemorySlot)
        state.mBetID.set(input.slotId, NULL_INDEX);
        state.mCreator.set(input.slotId, NULL_ID);
        state.mBetDesc.set(input.slotId, NULL_ID);
        {
            state.r.baseId0 = input.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                state.mOptionDesc.set(state.r.baseId0 + state.r.i0, NULL_ID);
            }
        }
        state.mBetAmountPerSlot.set(input.slotId, 0);
        state.mMaxNumberOfBetSlotPerOption.set(input.slotId, 0);
        {
            state.r.baseId0 = input.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                state.mOracleProvider.set(state.r.baseId0 + state.r.i0, NULL_ID);
                state.mOracleFees.set(state.r.baseId0 + state.r.i0, 0);
                state.mCurrentBetState.set(state.r.baseId0 + state.r.i0, 0);
            }
        }
        state.mNumberOption.set(input.slotId, 0);
        {
            state.r.baseId0 = input.slotId * 4;
            for (state.r.i0 = 0; state.r.i0 < 4; state.r.i0++) {
                state.mOpenDate.set(state.r.baseId0 + state.r.i0, 0);
                state.mCloseDate.set(state.r.baseId0 + state.r.i0, 0);
                state.mEndDate.set(state.r.baseId0 + state.r.i0, 0);
            }
        }
        state.mIsOccupied.set(input.slotId, 0); // close the bet
        {
            state.r.baseId0 = input.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_OPTION; state.r.i0++) {
                state.r.baseId1 = state.r.baseId0 + state.r.i0 * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (state.r.i1 = 0; state.r.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; state.r.i1++) {
                    state.mBettorID.set(state.r.baseId1 + state.r.i1, NULL_ID);
                    state.mBettorBetOption.set(state.r.baseId1 + state.r.i1, NULL_INDEX);
                }
            }
        }
    _

    /**
    * Try to finalize a bet when the system receive a result publication from a oracle provider
    * If there are 2/3 votes agree with the same result, the bet is finalized.
    * @param betId, slotId
    */
    PRIVATE(tryFinalizeBet)
        state.r.numberOP = 0;
        {
            state.r.baseId0 = input.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                if (state.mOracleProvider.get(state.r.baseId0 + state.r.i0) != NULL_ID) {
                    state.r.numberOP++;
                }
            }
        }
        state.r.requiredVote = divUp(uint32(state.r.numberOP * 2), 3U);
        state.r.winOption = -1;
        state.r.totalOption = state.mNumberOption.get(input.slotId);
        for (state.r.i0 = 0; state.r.i0 < state.r.totalOption; state.r.i0++) {
            state.r.voteCounter = 0;
            state.r.baseId0 = input.slotId * 8;
            for (state.r.i1 = 0; state.r.i1 < state.r.numberOP; state.r.i1++) {
                if (state.mBetResultWonOption.get(state.r.baseId0 + state.r.i1) == state.r.i0) {
                    state.r.voteCounter++;
                }
            }
            if (state.r.voteCounter >= state.r.requiredVote) {
                state.r.winOption = state.r.i0;
                break;
            }
        }
        if (state.r.winOption != -1)
        {
            //distribute coins
            state.r.amountPerSlot = state.mBetAmountPerSlot.get(input.slotId);
            state.r.totalBetSlot = 0;
            state.r.baseId0 = input.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < state.r.totalOption; state.r.i0++) {
                state.r.totalBetSlot += state.mCurrentBetState.get(state.r.baseId0 + state.r.i0);
            }
            state.r.nWinBet = state.mCurrentBetState.get(state.r.baseId0 + state.r.winOption);

            state.r.potAmountTotal = state.r.totalBetSlot * state.r.amountPerSlot;
            state.r.feeChargedAmount = state.r.potAmountTotal - state.r.nWinBet * state.r.amountPerSlot; // only charge winning amount
            state.r.transferredAmount = 0;
            // fee to OP
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                if (state.mOracleProvider.get(state.r.baseId0 + state.r.i0) != NULL_ID) {
                    state.r.fee = QPI::div(state.r.feeChargedAmount * state.mOracleFees.get(state.r.baseId0 + state.r.i0), 10000ULL);
                    if (state.r.fee != 0) {
                        transfer(state.mOracleProvider.get(state.r.baseId0 + state.r.i0), state.r.fee);
                        state.r.transferredAmount += state.r.fee;
                    }
                }
            }
            // fee to share holders
            {
                state.r.fee = QPI::div(state.r.feeChargedAmount * state.mShareHolderFee, 10000ULL);
                state.r.transferredAmount += state.r.fee;//self transfer
                state.mEarnedAmountForShareHolder += state.r.fee;
            }
            // fee to game operator
            {
                state.r.fee = QPI::div(state.r.feeChargedAmount * state.mGameOperatorFee, 10000ULL);
                transfer(state.mGameOperatorId, state.r.fee);
                state.r.transferredAmount += state.r.fee;
            }
            // profit to winners
            {
                state.r.feeChargedAmount -= state.r.transferredAmount; // left over go to winners
                state.r.profitPerBetSlot = QPI::div(state.r.feeChargedAmount, state.r.nWinBet);
                state.r.baseId0 = input.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                state.r.baseId1 = state.r.baseId0 + state.r.winOption * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (state.r.i1 = 0; state.r.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; state.r.i1++) {
                    if (state.mBettorID.get(state.r.baseId1 + state.r.i1) != NULL_ID) {
                        transfer(state.mBettorID.get(state.r.baseId1 + state.r.i1), state.r.amountPerSlot + state.r.profitPerBetSlot);
                        state.mEarnedAmountForBetWinner += (state.r.amountPerSlot + state.r.profitPerBetSlot);
                    }
                }
            }
            // done splitting profit
            {
                // cleaning bet storage
                state._cleanMemorySlot_input.slotId = input.slotId;
                cleanMemorySlot(state, state._cleanMemorySlot_input, state._cleanMemorySlot_output);
            }
        }
        else {
            QuotteryLogger log{ 0,QuotteryLogInfo::notEnoughVote,0 };
            LOG_INFO(log);
        }
    _
    /**************************************/
    /************VIEW FUNCTIONS************/
    /**************************************/
    /**
     * @return feePerSlotPerDay, gameOperatorFee, shareholderFee, minBetSlotAmount, gameOperator
     */
    PUBLIC(basicInfo)
        output.feePerSlotPerDay = state.mFeePerSlotPerDay;
        output.gameOperatorFee = state.mGameOperatorFee;
        output.shareholderFee = state.mShareHolderFee;
        output.minBetSlotAmount = state.mMinAmountPerBetSlot;
        output.gameOperator = state.mGameOperatorId;
    _
    /**
     * @param betId
     * @return meta data of a bet and its current state
     */
    PUBLIC(getBetInfo)
        output.betId = NULL_INDEX;
        state.r.slotId = -1;
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_BET; state.r.i0++) {
            if (state.mBetID.get(state.r.i0) == input.betId) {
                state.r.slotId = state.r.i0;
                break;
            }
        }
        if (state.r.slotId == -1) {
            // can't find betId
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(state.r.slotId) == 0) {
            // the bet is over
            return;
        }
        output.betId = input.betId;
        output.nOption = state.mNumberOption.get(state.r.slotId);
        output.creator = state.mCreator.get(state.r.slotId);
        output.betDesc = state.mBetDesc.get(state.r.slotId);
        {
            // option desc
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                output.optionDesc.set(state.r.i0, state.mOptionDesc.get(state.r.baseId0 + state.r.i0));
            }
        }
        {
            // oracle
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                output.oracleProviderId.set(state.r.i0, state.mOracleProvider.get(state.r.baseId0 + state.r.i0));
                output.oracleFees.set(state.r.i0, state.mOracleFees.get(state.r.baseId0 + state.r.i0));
            }
        }
        {
            state.r.baseId0 = state.r.slotId * 4;
            makeQuotteryDate(state.mOpenDate.get(state.r.baseId0), state.mOpenDate.get(state.r.baseId0+1), state.mOpenDate.get(state.r.baseId0+2), output.openDate);
            makeQuotteryDate(state.mCloseDate.get(state.r.baseId0), state.mCloseDate.get(state.r.baseId0 + 1), state.mCloseDate.get(state.r.baseId0 + 2), output.closeDate);
            makeQuotteryDate(state.mEndDate.get(state.r.baseId0), state.mEndDate.get(state.r.baseId0 + 1), state.mEndDate.get(state.r.baseId0 + 2), output.endDate);
        }
        output.minBetAmount = state.mBetAmountPerSlot.get(state.r.slotId);
        output.maxBetSlotPerOption = state.mMaxNumberOfBetSlotPerOption.get(state.r.slotId);
        {
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++){
                output.currentBetState.set(state.r.i0, state.mCurrentBetState.get(state.r.baseId0+ state.r.i0));
            }
        }
    _

    /**
     * @param betID, optionID
     * @return a list of ID that bet on optionID of bet betID
     */
    PUBLIC(getBetOptionDetail)
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; state.r.i0++) {
            output.bettor.set(state.r.i0, NULL_ID);
        }
        state.r.slotId = -1;
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_BET; state.r.i0++) {
            if (state.mBetID.get(state.r.i0) == input.betId) {
                state.r.slotId = state.r.i0;
                break;
            }
        }
        if (state.r.slotId == -1) {
            // can't find betId
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(state.r.slotId) == 0) {
            // the bet is over
            return;
        }
        if (input.betOption >= state.mNumberOption.get(state.r.slotId)){
            // invalid betOption
            return;
        }
        state.r.baseId0 = state.r.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
        state.r.baseId1 = state.r.baseId0 + input.betOption * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; state.r.i0++){
            output.bettor.set(state.r.i0, state.mBettorID.get(state.r.baseId1 + state.r.i0));
        }
    _

    /**
     * @return a list of active betID
     */
    PUBLIC(getActiveBet)
        output.count = 0;
        for (state.r.slotId = 0; state.r.slotId < QUOTTERY_MAX_BET; state.r.slotId++) {
            if (state.mBetID.get(state.r.slotId) != NULL_INDEX) {
                if (state.mIsOccupied.get(state.r.slotId) == 1) { //not available == active
                    output.activeBetId.set(output.count, state.mBetID.get(state.r.slotId));
                    output.count++;
                }
            }
        }
    _

    /**
    * @param creatorID
    * @return a list of active betID that created by creatorID
    */
    PUBLIC(getBetByCreator)
        output.count = 0;
        for (state.r.slotId = 0; state.r.slotId < QUOTTERY_MAX_BET; state.r.slotId++) {
            if (state.mBetID.get(state.r.slotId) != NULL_INDEX) {
                if (state.mIsOccupied.get(state.r.slotId) == 1) { //not available == active
                    if (state.mCreator.get(state.r.slotId) == input.creator) {
                        output.betId.set(output.count, state.mBetID.get(state.r.slotId));
                        output.count++;
                    }
                }
            }
        }
    _


    /**************************************/
    /************CORE FUNCTIONS************/
    /**************************************/
    /**
    * Create a bet
    * if the provided info is failed to create a bet, fund will be returned to invocator.
    * @param betDesc (32 bytes): bet description 32 bytes
    * @param optionDesc (256 bytes): option descriptions, 32 bytes for each option from 0 to 7, leave empty(zeroes) for unused memory space
    * @param oracleProviderId (256 bytes): oracle provider IDs, 32 bytes for each ID from 0 to 7, leave empty(zeroes) for unused memory space
    * @param oracleFees (32 bytes): the fee for oracle providers, 4 bytes(uint32) for each ID from 0 to 7, leave empty(zeroes) for unused memory space
    * @param closeDate (4 bytes): date in quotteryData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    * @param endDate (4 bytes): date in quotteryData format, thefirst byte is year, the second byte is month, the third byte is day(in month), the fourth byte is 0.
    * @param amountPerSlot (8 bytes): amount of qu per bet slot
    * @param maxBetSlotPerOption (4 bytes): maximum bet slot per option
    * @param numberOfOption (4 bytes): number of options(outcomes) of the bet
    */
    PUBLIC(issueBet)
        // only allow users to create bet
        if (QPI::invocator() != QPI::originator())
        {
            returnAllFund();
            return;
        }
        getCurrentDate(state.r.curDate);
        // check valid date: closeDate <= endDate
        if (dateCompare(input.closeDate, input.endDate, state.r.i0) == 1 ||
            dateCompare(state.r.curDate, input.closeDate, state.r.i0) == 1) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidDate,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        if (input.amountPerSlot < state.mMinAmountPerBetSlot) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidBetAmount,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        if (input.numberOfOption > QUOTTERY_MAX_OPTION || input.numberOfOption < 2) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidOption,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        if (input.maxBetSlotPerOption == 0 || input.maxBetSlotPerOption > QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET)
        {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidMaxBetSlotPerOption,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        state.r.maxBetSlotPerOption = input.maxBetSlotPerOption;
        state.r.numberOfOption = input.numberOfOption;
        diffDate(state.r.curDate, input.endDate, state.r.i0, state.r.i1, state.r.u64_0, state.r.u64_1, state.r.duration);
        state.r.duration += 1;
        state.r.fee = state.r.duration * state.r.maxBetSlotPerOption * state.r.numberOfOption * state.mFeePerSlotPerDay;

        // fee is higher than sent amount, exit
        if (state.r.fee > QPI::invocationReward()) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::insufficientFund,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        state.r.betId = state.mCurrentBetID++;
        state.r.slotId = -1;
        // find an empty slot
        {
            state.r.i0 = 0;
            state.r.i1 = state.r.betId & (QUOTTERY_MAX_BET - 1);
            while (state.r.i0++ < QUOTTERY_MAX_BET) {
                if (state.mIsOccupied.get(state.r.i1) == 0) {
                    state.r.slotId = state.r.i1;
                    break;
                }
                state.r.i1 = (state.r.i1 + 1) & (QUOTTERY_MAX_BET - 1);
            }
        }
        //out of bet storage, exit
        if (state.r.slotId == -1) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::outOfStorage,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        // return the fee changes
        if (QPI::invocationReward() > state.r.fee)
        {
            transfer(QPI::invocator(), QPI::invocationReward() - state.r.fee);
        }
        state.mRevenue += state.r.fee;
        state.mRevenueFromIssue += state.r.fee;
        // write data to slotId
        state.mBetID.set(state.r.slotId, state.r.betId); // 1
        state.mNumberOption.set(state.r.slotId, state.r.numberOfOption); // 2
        state.mCreator.set(state.r.slotId, QPI::invocator()); // 3
        state.mBetDesc.set(state.r.slotId, input.betDesc); // 4
        state.mBetAmountPerSlot.set(state.r.slotId, input.amountPerSlot); // 5
        state.mMaxNumberOfBetSlotPerOption.set(state.r.slotId, input.maxBetSlotPerOption); //6
        state.r.numberOP = 0;
        {
            // write option desc, oracle provider, oracle fee, bet state
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                state.mOptionDesc.set(state.r.baseId0 + state.r.i0, input.optionDesc.get(state.r.i0)); // 7
                state.mOracleProvider.set(state.r.baseId0 + state.r.i0, input.oracleProviderId.get(state.r.i0)); // 8
                if (input.oracleProviderId.get(state.r.i0) != NULL_ID) {
                    state.r.numberOP++;
                }
                state.mOracleFees.set(state.r.baseId0 + state.r.i0, input.oracleFees.get(state.r.i0)); // 9
                state.mCurrentBetState.set(state.r.baseId0 + state.r.i0, 0); //10
                state.mBetResultWonOption.set(state.r.baseId0 + state.r.i0, -1);
                state.mBetResultOPId.set(state.r.baseId0 + state.r.i0, -1);
            }
        }
        if (state.r.numberOP == 0) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidNumberOfOracleProvider,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        {
            // write date and time
            state.r.baseId0 = state.r.slotId * 4;
            for (state.r.i0 = 0; state.r.i0 < 4; state.r.i0++) {
                state.mOpenDate.set(state.r.baseId0 + state.r.i0, state.r.curDate.get(state.r.i0)); // 11
                state.mCloseDate.set(state.r.baseId0 + state.r.i0, input.closeDate.get(state.r.i0)); // 12
                state.mEndDate.set(state.r.baseId0 + state.r.i0, input.endDate.get(state.r.i0)); // 13
            }
        }
        state.mIsOccupied.set(state.r.betId, 1); // 14
        // done write, 14 fields
        // set zero to bettor info
        {
            state.r.baseId0 = state.r.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (state.r.i0 = 0; state.r.i0 < state.r.numberOfOption; state.r.i0++) {
                state.r.baseId1 = state.r.baseId0 + state.r.i0 * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (state.r.i1 = 0; state.r.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; state.r.i1++) {
                    state.mBettorID.set(state.r.baseId1 + state.r.i1, NULL_ID);
                    state.mBettorBetOption.set(state.r.baseId1 + state.r.i1, NULL_INDEX);
                }
            }
        }
    _

    /**
    * Join a bet
    * if the provided info is failed to join a bet, fund will be returned to invocator.
    * @param betId (4 bytes)
    * @param numberOfSlot (4 bytes): number of bet slot that invocator wants to join
    * @param option (4 bytes): the option Id that invocator wants to bet
    * @param _placeHolder (4 bytes): for padding
    */
    PUBLIC(joinBet)
        // only allow users to join bet
        if (QPI::invocator() != QPI::originator())
        {
            returnAllFund();
            return;
        }
        state.r.slotId = -1;
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_BET; state.r.i0++) {
            if (state.mBetID.get(state.r.i0) == input.betId) {
                state.r.slotId = state.r.i0;
                break;
            }
        }
        if (state.r.slotId == -1) {
            // can't find betId
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidBetId,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(state.r.slotId) == 0) {
            // the bet is over
            returnAllFund();
            return;
        }
        getCurrentDate(state.r.curDate);
        {
            state.r.baseId0 = state.r.slotId*4;
            makeQuotteryDate(state.mCloseDate.get(state.r.baseId0),
            state.mCloseDate.get(state.r.baseId0 + 1), state.mCloseDate.get(state.r.baseId0 + 2), state.r.closeDate);
        }

        // closedate is counted as 23:59 of that day
        if (dateCompare(state.r.curDate, state.r.closeDate, state.r.i0) > 0) {
            // bet is closed for betting
            QuotteryLogger log{ 0,QuotteryLogInfo::expiredBet,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }

        // check invalid option
        if (input.option >= state.mNumberOption.get(state.r.slotId)) {
            // bet is closed for betting
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidOption,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }

        // compute available number of options
        state.r.availableSlotForBet = 0;
        {
            state.r.baseId0 = state.r.slotId*8;
            state.r.availableSlotForBet = state.mMaxNumberOfBetSlotPerOption.get(state.r.slotId) -
                state.mCurrentBetState.get(state.r.baseId0 + input.option);
        }
        state.r.numberOfSlot = min(state.r.availableSlotForBet, input.numberOfSlot);
        if (state.r.numberOfSlot == 0) {
            // out of slot
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::outOfSlot,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        state.r.amountPerSlot = state.mBetAmountPerSlot.get(state.r.slotId);
        state.r.fee = state.r.amountPerSlot * state.r.numberOfSlot;
        if (state.r.fee > QPI::invocationReward()) {
            // not send enough amount
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::insufficientFund,0 };
            LOG_INFO(state.r.log);
            returnAllFund();
            return;
        }
        if (state.r.fee < QPI::invocationReward()) {
            // return changes
            transfer(QPI::invocator(), QPI::invocationReward() - state.r.fee);
        }

        state.mRevenue += state.r.fee;
        state.mRevenueFromBet += state.r.fee;
        // write bet info to memory
        {
            state.r.i2 = QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET - state.r.availableSlotForBet;
            state.r.i3 = state.r.i2 + state.r.numberOfSlot;
            state.r.baseId0 = state.r.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET + QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * input.option;
            for (state.r.i0 = state.r.i2; state.r.i0 < state.r.i3; state.r.i0++) {
                state.mBettorID.set(state.r.baseId0+ state.r.i0, QPI::invocator());
                state.mBettorBetOption.set(state.r.baseId0 + state.r.i0, input.option);
            }
        }
        //update stats
        {
            state.r.baseId0 = state.r.slotId * 8;
            state.r.currentState = state.mCurrentBetState.get(state.r.baseId0 + input.option);
            state.mCurrentBetState.set(state.r.baseId0 + input.option, state.r.currentState + state.r.numberOfSlot);
        }        
        // done
    _    
    
    /**
    * Publish result of a bet (Oracle Provider only)
    * After the vote is updated, it will try to finalize the bet, see tryFinalizeBet
    * @param betId (4 bytes)
    * @param option (4 bytes): winning option
    */
    PUBLIC(publishResult)
        // only allow users to publish result
        if (QPI::invocator() != QPI::originator())
        {
            return;
        }
        state.r.slotId = -1;
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_BET; state.r.i0++) {
            if (state.mBetID.get(state.r.i0) == input.betId) {
                state.r.slotId = state.r.i0;
                break;
            }
        }
        if (state.r.slotId == -1) {
            // can't find betId
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidBetId,0 };
            LOG_INFO(state.r.log);
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(state.r.slotId) == 0) {
            // the bet is over
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::expiredBet,0 };
            LOG_INFO(state.r.log);
            return;
        }
        getCurrentDate(state.r.curDate);
        
        {
            state.r.baseId0 = state.r.slotId * 4;
            makeQuotteryDate(state.mEndDate.get(state.r.baseId0),
            state.mEndDate.get(state.r.baseId0 + 1), state.mEndDate.get(state.r.baseId0 + 2), state.r.endDate);
        }
        // endDate is counted as 23:59 of that day
        if (dateCompare(state.r.curDate, state.r.endDate, state.r.i0) <= 0) {
            // bet is not end yet
            return;
        }

        state.r.opId = -1;
        {            
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                if (state.mOracleProvider.get(state.r.baseId0 + state.r.i0) == QPI::invocator()) {
                    state.r.opId = state.r.i0;
                    break;
                }
            }
            if (state.r.opId == -1) {
                // is not oracle provider
                state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidOPId,0 };
                LOG_INFO(state.r.log);
                return;
            }
        }
        // check if this OP already published result
        state.r.writeId = -1;
        {
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                if (state.mBetResultOPId.get(state.r.baseId0 + state.r.i0) == state.r.opId) {
                    state.r.writeId = state.r.i0;
                    break;
                }
            }
        }

        // OP already published result, now they want to change
        if (state.r.writeId != -1) {
            state.r.baseId0 = state.r.slotId * 8;
            state.mBetResultOPId.set(state.r.baseId0 + state.r.writeId, state.r.opId);
            state.mBetResultWonOption.set(state.r.baseId0 + state.r.writeId, input.option);
        }
        else {
            state.r.baseId0 = state.r.slotId * 8;
            for (state.r.i0 = 0; state.r.i0 < 8; state.r.i0++) {
                if (state.mBetResultOPId.get(state.r.baseId0 + state.r.i0) == -1 && state.mBetResultWonOption.get(state.r.baseId0 + state.r.i0) == -1) {
                    state.r.writeId = state.r.i0;
                    break;
                }
            }
            state.mBetResultOPId.set(state.r.baseId0 + state.r.writeId, state.r.opId);
            state.mBetResultWonOption.set(state.r.baseId0 + state.r.writeId, input.option);
        }

        // try to finalize the bet
        {
            state._tryFinalizeBet_input.betId = input.betId;
            state._tryFinalizeBet_input.slotId = state.r.slotId;
            tryFinalizeBet(state, state._tryFinalizeBet_input, state._tryFinalizeBet_output);
        }
        
    _

    /**
    * Publish cancel a bet (Game Operator only)
    * In ermergency case, this function removes a bet from system and return all funds back to original invocators (except issueing fee)
    * @param betId (4 bytes)
    */
    PUBLIC(cancelBet)
        // game operator invocation only. In any case that oracle providers can't reach consensus or unable to broadcast result after a long period,         
        // all funds will be returned
        if (QPI::invocator() != state.mGameOperatorId) {
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::notGameOperator,0 };
            LOG_INFO(state.r.log);
            return;
        }
        state.r.slotId = -1;
        for (state.r.i0 = 0; state.r.i0 < QUOTTERY_MAX_BET; state.r.i0++) {
            if (state.mBetID.get(state.r.i0) == input.betId) {
                state.r.slotId = state.r.i0;
                break;
            }
        }
        if (state.r.slotId == -1) {
            // can't find betId
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::invalidBetId,0 };
            LOG_INFO(state.r.log);
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(state.r.slotId) == 0) {
            // the bet is over
            state.r.log = QuotteryLogger{ 0,QuotteryLogInfo::expiredBet,0 };
            LOG_INFO(state.r.log);
            return;
        }
        getCurrentDate(state.r.curDate);
        {
            state.r.baseId0 = state.r.slotId * 4;
            makeQuotteryDate(state.mEndDate.get(state.r.baseId0),
            state.mEndDate.get(state.r.baseId0 + 1), state.mEndDate.get(state.r.baseId0 + 2), state.r.endDate);
        }

        // endDate is counted as 23:59 of that day
        if (dateCompare(state.r.curDate, state.r.endDate, state.r.i0) <= 0) {
            // bet is not end yet
            return;
        }
        //static void diffDate(uint8_4& A, uint8_4& B, sint32& i, sint32& baseYear, uint64& dayA, uint64& dayB, uint64& res)
        diffDate(state.r.curDate, state.r.endDate, state.r.i0, state.r.i1, state.r.u64_0, state.r.u64_1, state.r.duration);
        if (state.r.duration < 2){
            // need 2+ days to do this
            return;
        }

        {
            state.r.nOption = state.mNumberOption.get(state.r.slotId);
            state.r.amountPerSlot = state.mBetAmountPerSlot.get(state.r.slotId);
            state.r.baseId0 = state.r.slotId * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION;
            for (state.r.i0 = 0; state.r.i0 < state.r.nOption; state.r.i0++){
                state.r.baseId1 = state.r.baseId0 + state.r.i0 * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (state.r.i1 = 0; state.r.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; state.r.i1++){
                    if (state.mBettorID.get(state.r.baseId1 + state.r.i1) != NULL_ID){
                        transfer(state.mBettorID.get(state.r.baseId1 + state.r.i1), state.r.amountPerSlot);
                    }
                }
            }
        }
        {
            // cleaning bet storage
            state._cleanMemorySlot_input.slotId = state.r.slotId;
            cleanMemorySlot(state, state._cleanMemorySlot_input, state._cleanMemorySlot_output);
        }
    _

        //views function
        REGISTER_USER_FUNCTIONS
        REGISTER_USER_FUNCTION(basicInfo, 1);
        REGISTER_USER_FUNCTION(getBetInfo, 2);
        REGISTER_USER_FUNCTION(getBetOptionDetail, 3);
        REGISTER_USER_FUNCTION(getActiveBet, 4);
        REGISTER_USER_FUNCTION(getBetByCreator, 5);
        _

        REGISTER_USER_PROCEDURES
        REGISTER_USER_PROCEDURE(issueBet, 1);
        REGISTER_USER_PROCEDURE(joinBet, 2);
        REGISTER_USER_PROCEDURE(cancelBet, 3);
        REGISTER_USER_PROCEDURE(publishResult, 4);
        _

        INITIALIZE
        _

        BEGIN_EPOCH
            state.mFeePerSlotPerDay = QUOTTERY_FEE_PER_SLOT_PER_DAY_;
            state.mMinAmountPerBetSlot = QUOTTERY_MIN_AMOUNT_PER_BET_SLOT_;
            state.mShareHolderFee = QUOTTERY_SHAREHOLDER_FEE_;
            state.mGameOperatorFee = QUOTTERY_GAME_OPERATOR_FEE_;

            state.mGameOperatorId = id(0x63a7317950fa8886ULL, 0x4dbdf78085364aa7ULL, 0x21c6ca41e95bfa65ULL, 0xcbc1886b3ea8e647ULL);
        _

        END_EPOCH
        _

        BEGIN_TICK
        _

        END_TICK
        _

        EXPAND
        _
};
