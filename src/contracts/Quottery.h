#include "math_lib.h"

// adjustable with type changes
constexpr unsigned long long QUOTTERY_MAX_BET = 1024;
constexpr unsigned long long QUOTTERY_MAX_OPTION = 8;
constexpr unsigned long long QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET = 1024;

// adjustable with no change:
constexpr unsigned long long QUOTTERY_FEE_PER_SLOT_PER_DAY_ = 10000ULL;
constexpr unsigned long long QUOTTERY_MIN_AMOUNT_PER_BET_SLOT_ = 10000ULL;
constexpr unsigned long long QUOTTERY_SHAREHOLDER_FEE_ = 1000;
constexpr unsigned long long QUOTTERY_GAME_OPERATOR_FEE_ = 50;

typedef QPI::uint8_4 quotteryDate; // Datetime order in bytes: year-month-day, last byte for padding
typedef QPI::id quotteryText;
typedef QPI::id_8 quotteryText8;

// adjustable. For scaling later:
typedef QPI::id_1024 quotteryTextArray;
typedef QPI::id_8192 quotteryText8Array; // 1024*8=8192, access by stride8
typedef QPI::uint32_1024 quotteryBetIDArray;
typedef QPI::id_1024 quotteryCreatorArray;
typedef QPI::uint64_1024 quotteryBetAmountArray;
typedef QPI::uint64_1024 quotteryMaxNumberOfBetSlotPerOptionArray;
typedef QPI::id_8192 quotteryOracleProviderArray;
typedef QPI::uint32_8192 quotteryOracleFeeArray;
typedef QPI::uint32_8192 quotteryBetStateArray;
typedef QPI::uint8_1024 quotteryNumberOfOptionArray;
typedef QPI::uint8_4096 quotteryDateArray;
typedef QPI::bit_1024 quotteryOccupiedArray;
typedef QPI::sint8_8192 quotteryBetResultArray;
typedef QPI::id_8388608 quotteryBettorIDArray;
typedef QPI::uint8_8388608 quotteryBettorOptionArray;

static_assert(sizeof(quotteryTextArray) == (sizeof(quotteryText) * QUOTTERY_MAX_BET), "quotteryTextArray");
static_assert(sizeof(quotteryText8Array) == (sizeof(quotteryText) * QUOTTERY_MAX_BET * 8), "quotteryText8Array");
static_assert(sizeof(quotteryBetIDArray) == (sizeof(QPI::uint32) * QUOTTERY_MAX_BET), "quotteryBetIDArray");
static_assert(sizeof(quotteryCreatorArray) == (sizeof(QPI::id) * QUOTTERY_MAX_BET), "quotteryCreatorArray");
static_assert(sizeof(quotteryBetAmountArray) == (sizeof(QPI::uint64) * QUOTTERY_MAX_BET), "quotteryBetAmountArray");
static_assert(sizeof(quotteryMaxNumberOfBetSlotPerOptionArray) == (sizeof(QPI::uint64) * QUOTTERY_MAX_BET), "quotteryMaxNumberOfBetSlotPerOptionArray");
static_assert(sizeof(quotteryOracleProviderArray) == (sizeof(QPI::id) * QUOTTERY_MAX_BET * 8), "quotteryOracleProviderArray");
static_assert(sizeof(quotteryOracleFeeArray) == (sizeof(QPI::uint32) * QUOTTERY_MAX_BET * 8), "quotteryOracleFeeArray");
static_assert(sizeof(quotteryBetStateArray) == (sizeof(QPI::uint32) * QUOTTERY_MAX_BET * 8), "quotteryBetStateArray");
static_assert(sizeof(quotteryNumberOfOptionArray) == (sizeof(QPI::uint8) * QUOTTERY_MAX_BET), "quotteryNumberOfOptionArray");
static_assert(sizeof(quotteryDateArray) == (sizeof(quotteryDate) * QUOTTERY_MAX_BET), "quotteryDateArray");
static_assert(sizeof(quotteryOccupiedArray) == (sizeof(QPI::bit) * QUOTTERY_MAX_BET / 8), "quotteryOccupiedArray");
static_assert(sizeof(quotteryBetResultArray) == (sizeof(QPI::sint8) * QUOTTERY_MAX_BET * 8), "quotteryBetResultArray");
static_assert(sizeof(quotteryBettorIDArray) == (sizeof(QPI::id) * QUOTTERY_MAX_BET * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET), "quotteryBettorIDArray");
static_assert(sizeof(quotteryBettorOptionArray) == (sizeof(QPI::uint8) * QUOTTERY_MAX_BET * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET), "quotteryBettorOptionArray");


enum QuotteryLogInfo {
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
    unsigned int _contractIndex;
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types
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
        quotteryText betDesc;      // 32 bytes 
        quotteryText8 optionDesc;  // 8x(32)=256bytes
        id_8 oracleProviderId; // 256x8=2048bytes
        uint32_8 oracleFees;   // 4x8 = 32 bytes

        quotteryDate openDate;     // creation date, start to receive bet
        quotteryDate closeDate;    // stop receiving bet date
        quotteryDate endDate;       // result date
        // Amounts and numbers
        uint64 minBetAmount;
        uint32 maxBetSlotPerOption;
        uint32_8 currentBetState; // how many bet slots have been filled on each option
    };
    struct issueBet_input {
        quotteryText betDesc;
        quotteryText8 optionDesc;
        id_8 oracleProviderId; // 256x8=2048bytes
        uint32_8 oracleFees;   // 4x8 = 32 bytes
        quotteryDate closeDate;
        quotteryDate endDate;
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
    struct cleanMemorySlot_input {
        sint64 slotId;
    };
    struct cleanMemorySlot_output {
    };

    /**************************************/
    /************CONTRACT STATES***********/
    /**************************************/
    quotteryBetIDArray mBetID;
    quotteryCreatorArray  mCreator;
    quotteryTextArray mBetDesc;
    quotteryText8Array mOptionDesc;
    quotteryBetAmountArray mBetAmountPerSlot;
    quotteryMaxNumberOfBetSlotPerOptionArray mMaxNumberOfBetSlotPerOption;
    quotteryOracleProviderArray mOracleProvider; // 1024x8=8192, access by stride8
    quotteryOracleFeeArray mOracleFees;
    quotteryBetStateArray mCurrentBetState; // 8xuint32 per bet, each represent number of bettor for according option
    quotteryNumberOfOptionArray mNumberOption;
    quotteryDateArray mOpenDate; // take 00:00 UTC | 4xuint8 per bet
    quotteryDateArray mCloseDate; // take 23:59 UTC
    quotteryDateArray mEndDate; // take 23:59 UTC
    quotteryOccupiedArray mIsOccupied;

    // bet result:
    quotteryBetResultArray mBetResultWonOption; // 8xuint8 per bet
    quotteryBetResultArray mBetResultOPId; // 8xuint8 per bet

    //bettor struct:
    quotteryBettorIDArray mBettorID;
    quotteryBettorOptionArray mBettorBetOption;

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
    id mGameOperatorAddress;
    // adjustable variables
    uint64 mFeePerSlotPerDay;
    uint64 mMinAmountPerBetSlot;
    uint64 mShareHolderFee;
    uint64 mGameOperatorFee;
    id mGameOperatorId;

    /**************************************/
    /************UTIL FUNCTIONS************/
    /**************************************/

    /**
     * Return all sent amount to invocator
     */
    static inline void returnAllFund() {
        transfer(QPI::invocator(), QPI::invocationReward());
    }

    /**
     * Compare 2 date in quotteryDate format
     * @return -1 lesser(ealier) A<B, 0 equal A=B, 1 greater(later) A>B
     */
    static sint32 dateCompare(quotteryDate A, quotteryDate B) {
        if (A.get(0) == B.get(0) && A.get(1) == B.get(1) && A.get(2) == B.get(2)) {
            return 0;
        }
        for (sint32 i = 0; i < 3; i++) {
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
    static quotteryDate getCurrentDate() {
        quotteryDate res;
        res.set(0, year());
        res.set(1, month());
        res.set(2, day());
        res.set(3, 0);
        return res;
    }

    /**
     * @return quotteryDate variable from year, month, day
     */
    static quotteryDate makeQuotteryDate(uint8 _year, uint8 _month, uint8 _day) {
        quotteryDate res;
        res.set(0, _year);
        res.set(1, _month);
        res.set(2, _day);
        res.set(3, 0);
        return res;
    }


    static sint32 accumulatedDay(sint32 month)
    {
        sint32 res = 0;
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
        return res;
    }
    /**
     * @return difference in number of day, A must be smaller than or equal B to have valid value
     */
    static uint64 diffDate(quotteryDate A, quotteryDate B) {
        if (dateCompare(A, B) >= 0) {
            return 0;
        }
        sint32 baseYear = A.get(0);
        uint64 dayA = accumulatedDay(A.get(1)) + A.get(2);
        uint64 dayB = (B.get(0) - baseYear) * 365ULL + accumulatedDay(B.get(1)) + B.get(2);

        // handling leap-year: only store last 2 digits of year here, don't care about mod 100 & mod 400 case
        for (sint32 i = baseYear; i < B.get(0); i++) {
            if (i % 4 == 0) {
                dayB++;
            }
        }
        if (B.get(0) % 4 == 0 && B.get(1) > 2) dayB++;
        return dayB - dayA;
    }

    /**
     * Clean all memory of a slot Id, set the flag IsOccupied to zero
     * @param slotId
     */
    PRIVATE(cleanMemorySlot)
        uint64 slotId = input.slotId;
        state.mBetID.set(slotId, NULL_INDEX);
        state.mCreator.set(slotId, NULL_ID);
        state.mBetDesc.set(slotId, NULL_ID);
        {
            uint64 baseId = slotId * 8;
            for (int i = 0; i < 8; i++) {
                state.mOptionDesc.set(baseId + i, NULL_ID);
            }
        }
        state.mBetAmountPerSlot.set(slotId, 0);
        state.mMaxNumberOfBetSlotPerOption.set(slotId, 0);
        {
            uint64 baseId = slotId * 8;
            for (int i = 0; i < 8; i++) {
                state.mOracleProvider.set(baseId + i, NULL_ID);
                state.mOracleFees.set(baseId + i, 0);
                state.mCurrentBetState.set(baseId + i, 0);
            }
        }
        state.mNumberOption.set(slotId, 0);
        {
            uint64 baseId = slotId * 4;
            for (int i = 0; i < 4; i++) {
                state.mOpenDate.set(baseId + i, 0);
                state.mCloseDate.set(baseId + i, 0);
                state.mEndDate.set(baseId + i, 0);
            }
        }
        state.mIsOccupied.set(slotId, 0); // close the bet
        {
            uint64 baseId0 = slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (sint32 i = 0; i < QUOTTERY_MAX_OPTION; i++) {
                uint64 baseId1 = baseId0 + i * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (sint32 j = 0; j < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; j++) {
                    state.mBettorID.set(baseId1 + j, NULL_ID);
                    state.mBettorBetOption.set(baseId1 + j, NULL_INDEX);
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
        sint32 betId = input.betId;
        sint64 slotId = input.slotId;
        // check if this bet can be finalize
        sint32 numberOP = 0;
        {
            uint64 baseId = slotId * 8;
            for (sint32 i = 0; i < 8; i++) {
                if (state.mOracleProvider.get(baseId + i) != NULL_ID) {
                    numberOP++;
                }
            }
        }
        sint32 requiredVote = math_lib::divUp(uint32(numberOP * 2), 3U);
        sint32 winOption = -1;
        sint32 totalOption = state.mNumberOption.get(slotId);
        for (sint32 i = 0; i < totalOption; i++) {
            sint32 voteCounter = 0;
            uint64 baseId = slotId * 8;
            for (sint32 j = 0; j < numberOP; j++) {
                if (state.mBetResultWonOption.get(baseId + j) == i) {
                    voteCounter++;
                }
            }
            if (voteCounter >= requiredVote) {
                winOption = i;
                break;
            }
        }
        if (winOption != -1)
        {
            //distribute coins
            uint64 amountPerSlot = state.mBetAmountPerSlot.get(slotId);
            uint64 totalBetSlot = 0;
            uint64 baseId = slotId * 8;
            for (sint32 i = 0; i < totalOption; i++) {
                totalBetSlot += state.mCurrentBetState.get(baseId + i);
            }
            sint32 nWinBet = state.mCurrentBetState.get(baseId + winOption);

            uint64 potAmountTotal = totalBetSlot * amountPerSlot;
            uint64 feeChargedAmount = potAmountTotal - nWinBet * amountPerSlot; // only charge winning amount
            uint64 transferredAmount = 0;
            // fee to OP
            for (sint32 i = 0; i < 8; i++) {
                id opID = state.mOracleProvider.get(baseId + i);
                if (opID != NULL_ID) {
                    uint32 opFeeRate = state.mOracleFees.get(baseId + i);
                    uint64 opFee = QPI::div(feeChargedAmount * opFeeRate, 10000ULL);
                    if (opFee != 0) {
                        transfer(opID, opFee);
                        transferredAmount += opFee;
                    }
                }
            }
            // fee to share holders
            {
                uint64 shareHolderFee = QPI::div(feeChargedAmount * state.mShareHolderFee, 10000ULL);
                transferredAmount += shareHolderFee;//self transfer
                state.mEarnedAmountForShareHolder += shareHolderFee;
            }
            // fee to game operator
            {
                uint64 gameOperatorFee = QPI::div(feeChargedAmount * state.mGameOperatorFee, 10000ULL);
                transfer(state.mGameOperatorAddress, gameOperatorFee);
                transferredAmount += gameOperatorFee;
            }
            // profit to winners
            {
                feeChargedAmount -= transferredAmount; // left over go to winners
                uint64 profitPerBetSlot = QPI::div(feeChargedAmount, uint64(nWinBet));
                uint64 baseId0 = slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                uint64 baseId1 = baseId0 + winOption * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (sint32 j = 0; j < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; j++) {
                    id betId = state.mBettorID.get(baseId1 + j);
                    if (betId != NULL_ID) {
                        transfer(betId, amountPerSlot + profitPerBetSlot);
                        state.mEarnedAmountForBetWinner += (amountPerSlot + profitPerBetSlot);
                    }
                }
            }
            // done splitting profit
            {
                // cleaning bet storage
                cleanMemorySlot_input input;
                cleanMemorySlot_output output;
                input.slotId = slotId;
                cleanMemorySlot(state, input, output);
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
        output.gameOperator = state.mGameOperatorAddress;
    _
    /**
     * @param betId
     * @return meta data of a bet and its current state
     */
    PUBLIC(getBetInfo)
        output.betId = NULL_INDEX;
        int slotId = -1;
        for (int i = 0; i < QUOTTERY_MAX_BET; i++) {
            if (state.mBetID.get(i) == input.betId) {
                slotId = i;
                break;
            }
        }
        if (slotId == -1) {
            // can't find betId
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(slotId) == 0) {
            // the bet is over
            return;
        }
        output.betId = input.betId;
        output.nOption = state.mNumberOption.get(slotId);
        output.creator = state.mCreator.get(slotId);
        output.betDesc = state.mBetDesc.get(slotId);
        {
            // option desc
            int baseId = slotId * 8;
            for (int i = 0; i < 8; i++) {
                quotteryText option_desc = state.mOptionDesc.get(baseId+i);
                output.optionDesc.set(i, option_desc);
            }
        }
        {
            // oracle
            int baseId = slotId * 8;
            for (int i = 0; i < 8; i++) {
                id OPid = state.mOracleProvider.get(baseId+i);
                uint32 OPfee = state.mOracleFees.get(baseId+i);
                output.oracleProviderId.set(i, OPid);
                output.oracleFees.set(i, OPfee);
            }
        }
        {
            int baseId = slotId * 4;
            output.openDate = makeQuotteryDate(state.mOpenDate.get(baseId), state.mOpenDate.get(baseId+1), state.mOpenDate.get(baseId+2));
            output.closeDate = makeQuotteryDate(state.mCloseDate.get(baseId), state.mCloseDate.get(baseId + 1), state.mCloseDate.get(baseId + 2));
            output.endDate = makeQuotteryDate(state.mEndDate.get(baseId), state.mEndDate.get(baseId + 1), state.mEndDate.get(baseId + 2));
        }
        output.minBetAmount = state.mBetAmountPerSlot.get(slotId);
        output.maxBetSlotPerOption = state.mMaxNumberOfBetSlotPerOption.get(slotId);
        {
            int baseId = slotId * 8;
            for (int i = 0; i < 8; i++){
                output.currentBetState.set(i, state.mCurrentBetState.get(baseId+i));
            }
        }
    _

    /**
     * @param betID, optionID
     * @return a list of ID that bet on optionID of bet betID
     */
    PUBLIC(getBetOptionDetail)
        for (int i = 0; i < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; i++) {
            output.bettor.set(i, NULL_ID);
        }
        int slotId = -1;
        for (int i = 0; i < QUOTTERY_MAX_BET; i++) {
            if (state.mBetID.get(i) == input.betId) {
                slotId = i;
                break;
            }
        }
        if (slotId == -1) {
            // can't find betId
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(slotId) == 0) {
            // the bet is over
            return;
        }
        int betOption = input.betOption;
        if (betOption >= state.mNumberOption.get(slotId)){
            // invalid betOption
            return;
        }
        int baseId0 = slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
        int baseId1 = baseId0 + betOption * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
        for (int i = 0; i < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; i++){
            output.bettor.set(i, state.mBettorID.get(baseId1 + i));
        }
    _

    /**
     * @return a list of active betID
     */
    PUBLIC(getActiveBet)
        output.count = 0;
        for (int slotId = 0; slotId < QUOTTERY_MAX_BET; slotId++) {
            uint32 betId = state.mBetID.get(slotId);
            if (betId != NULL_INDEX) {
                if (state.mIsOccupied.get(slotId) == 1) { //not available == active
                    output.activeBetId.set(output.count, betId);
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
        for (int slotId = 0; slotId < QUOTTERY_MAX_BET; slotId++) {
            uint32 betId = state.mBetID.get(slotId);
            if (betId != NULL_INDEX) {
                if (state.mIsOccupied.get(slotId) == 1) { //not available == active
                    if (state.mCreator.get(slotId) == input.creator) {
                        output.betId.set(output.count, betId);
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
        quotteryDate curDate = getCurrentDate();
        // check valid date: closeDate <= endDate
        if (dateCompare(input.closeDate, input.endDate) == 1 ||
            dateCompare(curDate, input.closeDate) == 1) {
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidDate,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        if (input.amountPerSlot < state.mMinAmountPerBetSlot) {
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidBetAmount,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        if (input.numberOfOption > QUOTTERY_MAX_OPTION || input.numberOfOption < 2) {
            QuotteryLogger log{0,QuotteryLogInfo::invalidOption,0};
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        uint32 maxBetSlotPerOption = math_lib::min(uint64(input.maxBetSlotPerOption), QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET);
        uint8 numberOfOption = math_lib::min(input.numberOfOption, uint32(QUOTTERY_MAX_OPTION));
        uint64 duration = diffDate(curDate, input.endDate) + 1;
        uint64 fee = duration * maxBetSlotPerOption * numberOfOption * state.mFeePerSlotPerDay;

        // fee is higher than sent amount, exit
        if (fee > QPI::invocationReward()) {
            QuotteryLogger log{ 0,QuotteryLogInfo::insufficientFund,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        uint32 betId = state.mCurrentBetID++;
        id creator = QPI::invocator();
        sint64 slotId = -1;
        // find an empty slot
        {
            sint32 count = 0;
            sint32 i = betId & (QUOTTERY_MAX_BET - 1);
            while (count++ < QUOTTERY_MAX_BET) {
                if (state.mIsOccupied.get(i) == 0) {
                    slotId = i;
                    break;
                }
                i = (i+1) & (QUOTTERY_MAX_BET - 1);
            }
        }
        //out of bet storage, exit
        if (slotId == -1) {
            QuotteryLogger log{ 0,QuotteryLogInfo::outOfStorage,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        // return the fee changes
        if (QPI::invocationReward() > fee)
        {
            transfer(QPI::invocator(), QPI::invocationReward() - fee);
        }
        state.mRevenue += fee;
        state.mRevenueFromIssue += fee;
        // write data to slotId
        state.mBetID.set(slotId, betId); // 1
        state.mNumberOption.set(slotId, numberOfOption); // 2
        state.mCreator.set(slotId, creator); // 3
        state.mBetDesc.set(slotId, input.betDesc); // 4
        state.mBetAmountPerSlot.set(slotId, input.amountPerSlot); // 5
        state.mMaxNumberOfBetSlotPerOption.set(slotId, input.maxBetSlotPerOption); //6
        uint32 numberOfOP = 0;
        {
            // write option desc, oracle provider, oracle fee, bet state
            uint64 baseId = slotId * 8;
            for (sint32 i = 0; i < 8; i++) {
                state.mOptionDesc.set(baseId + i, input.optionDesc.get(i)); // 7
                state.mOracleProvider.set(baseId+i, input.oracleProviderId.get(i)); // 8
                if (input.oracleProviderId.get(i) != NULL_ID) {
                    numberOfOP++;
                }
                state.mOracleFees.set(baseId+i, input.oracleFees.get(i)); // 9
                state.mCurrentBetState.set(baseId+i, 0); //10
                state.mBetResultWonOption.set(baseId+i, -1);
                state.mBetResultOPId.set(baseId + i, -1);
            }
        }
        if (numberOfOP == 0) {
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidNumberOfOracleProvider,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        {
            // write date and time
            uint64 baseId = slotId * 4;
            for (sint32 i = 0; i < 4; i++) {
                state.mOpenDate.set(baseId+i, curDate.get(i)); // 11
                state.mCloseDate.set(baseId + i, input.closeDate.get(i)); // 12
                state.mEndDate.set(baseId + i, input.endDate.get(i)); // 13
            }
        }
        state.mIsOccupied.set(betId, 1); // 14
        // done write, 14 fields
        // set zero to bettor info
        {
            uint64 baseId0 = slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (sint32 i = 0; i < numberOfOption; i++) {
                uint64 baseId1 = baseId0 + i * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (sint32 j = 0; j < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; j++) {
                    state.mBettorID.set(baseId1+j, NULL_ID);
                    state.mBettorBetOption.set(baseId1 + j, NULL_INDEX);
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
        sint64 slotId = -1;
        for (sint32 i = 0; i < QUOTTERY_MAX_BET; i++) {
            if (state.mBetID.get(i) == input.betId) {
                slotId = i;
                break;
            }
        }
        if (slotId == -1) {
            // can't find betId
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidBetId,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(slotId) == 0) {
            // the bet is over
            returnAllFund();
            return;
        }
        quotteryDate currentDate = getCurrentDate();
        quotteryDate closeDate;
        {
            uint64 baseId = slotId*4;
            uint8 _year = state.mCloseDate.get(baseId);
            uint8 _month = state.mCloseDate.get(baseId + 1);
            uint8 _day = state.mCloseDate.get(baseId + 2);
            closeDate = makeQuotteryDate(_year, _month, _day);
        }

        // closedate is counted as 23:59 of that day
        if (dateCompare(currentDate, closeDate) > 0) {
            // bet is closed for betting
            QuotteryLogger log{ 0,QuotteryLogInfo::expiredBet,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }

        // check invalid option
        if (input.option >= state.mNumberOption.get(slotId)) {
            // bet is closed for betting
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidOption,0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }

        // compute available number of options
        uint32 availableSlotForBet = 0;
        {
            uint64 baseId = slotId*8;
            availableSlotForBet = state.mMaxNumberOfBetSlotPerOption.get(slotId) - state.mCurrentBetState.get(baseId + sint32(input.option));
        }
        sint32 numberOfSlot = math_lib::min(availableSlotForBet, input.numberOfSlot);
        if (numberOfSlot == 0) {
            // out of slot
            QuotteryLogger log{ 0,QuotteryLogInfo::outOfSlot, 0 };
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        uint64 amountPerSlot = state.mBetAmountPerSlot.get(slotId);
        uint64 fee = amountPerSlot * numberOfSlot;
        if (fee > QPI::invocationReward()) {
            // not send enough amount
            QuotteryLogger log{ 0,QuotteryLogInfo::insufficientFund, 0};
            LOG_INFO(log);
            returnAllFund();
            return;
        }
        if (fee < QPI::invocationReward()) {
            // return changes
            transfer(QPI::invocator(), QPI::invocationReward() - fee);
        }

        state.mRevenue += fee;
        state.mRevenueFromBet += fee;
        // write bet info to memory
        {
            sint32 fromId = QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET - availableSlotForBet;
            sint32 toId = fromId + numberOfSlot;
            uint64 baseId = slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET + sint32(input.option) * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (sint32 i = fromId; i < toId; i++) {
                state.mBettorID.set(baseId+i, QPI::invocator());
                state.mBettorBetOption.set(baseId+i, input.option);
            }
        }
        //update stats
        {
            uint64 baseId = slotId * 8;
            sint32 current = state.mCurrentBetState.get(baseId + input.option);
            state.mCurrentBetState.set(baseId + input.option, current+numberOfSlot);
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
        sint64 slotId = -1;
        for (sint32 i = 0; i < QUOTTERY_MAX_BET; i++) {
            if (state.mBetID.get(i) == input.betId) {
                slotId = i;
                break;
            }
        }
        if (slotId == -1) {
            // can't find betId
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidBetId,0 };
            LOG_INFO(log);
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(slotId) == 0) {
            // the bet is over
            QuotteryLogger log{ 0,QuotteryLogInfo::expiredBet,0 };
            LOG_INFO(log);
            return;
        }
        quotteryDate currentDate = getCurrentDate();
        quotteryDate endDate;
        {
            uint64 baseId = slotId * 4;
            uint8 _year = state.mEndDate.get(baseId);
            uint8 _month = state.mEndDate.get(baseId + 1);
            uint8 _day = state.mEndDate.get(baseId + 2);
            endDate = makeQuotteryDate(_year, _month, _day);
        }

        // endDate is counted as 23:59 of that day
        if (dateCompare(currentDate, endDate) <= 0) {
            // bet is not end yet
            return;
        }

        sint32 opId = -1;
        {            
            uint64 baseId = slotId * 8;
            for (sint32 i = 0; i < 8; i++) {
                if (state.mOracleProvider.get(baseId + i) == QPI::invocator()) {
                    opId = i;
                    break;
                }
            }
            if (opId == -1) {
                // is not oracle provider
                QuotteryLogger log{ 0,QuotteryLogInfo::invalidOPId,0 };
                LOG_INFO(log);
                return;
            }
        }
        // check if this OP already published result
        sint32 writeId = -1;
        {
            uint64 baseId = slotId * 8;
            for (sint32 i = 0; i < 8; i++) {
                if (state.mBetResultOPId.get(baseId + i) == opId) {
                    writeId = i;
                    break;
                }
            }
        }

        // OP already published result, now they want to change
        if (writeId != -1) {
            uint64 baseId = slotId * 8;
            state.mBetResultOPId.set(baseId + writeId, opId);
            state.mBetResultWonOption.set(baseId + writeId, input.option);
        }
        else {
            uint64 baseId = slotId * 8;
            for (sint32 i = 0; i < 8; i++) {
                if (state.mBetResultOPId.get(baseId + i) == -1 && state.mBetResultWonOption.get(baseId+i) == -1) {
                    writeId = i;
                    break;
                }
            }
            state.mBetResultOPId.set(baseId + writeId, opId);
            state.mBetResultWonOption.set(baseId + writeId, input.option);
        }

        // try to finalize the bet
        {
            tryFinalizeBet_input finalizeInput;
            finalizeInput.betId = input.betId;
            finalizeInput.slotId = slotId;
            tryFinalizeBet_output finalizeOutput;
            tryFinalizeBet(state, finalizeInput, finalizeOutput);
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
        if (QPI::invocator() != state.mGameOperatorAddress) {
            QuotteryLogger log{ 0,QuotteryLogInfo::notGameOperator, 0};
            LOG_INFO(log);
            return;
        }
        sint64 slotId = -1;
        for (sint32 i = 0; i < QUOTTERY_MAX_BET; i++) {
            if (state.mBetID.get(i) == input.betId) {
                slotId = i;
                break;
            }
        }
        if (slotId == -1) {
            // can't find betId
            QuotteryLogger log{ 0,QuotteryLogInfo::invalidBetId, 0};
            LOG_INFO(log);
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(slotId) == 0) {
            // the bet is over
            QuotteryLogger log{ 0,QuotteryLogInfo::expiredBet, 0};
            LOG_INFO(log);
            return;
        }
        quotteryDate currentDate = getCurrentDate();
        quotteryDate endDate;
        {
            uint64 baseId = slotId * 4;
            uint8 _year = state.mEndDate.get(baseId);
            uint8 _month = state.mEndDate.get(baseId + 1);
            uint8 _day = state.mEndDate.get(baseId + 2);
            endDate = makeQuotteryDate(_year, _month, _day);
        }

        // endDate is counted as 23:59 of that day
        if (dateCompare(currentDate, endDate) <= 0) {
            // bet is not end yet
            return;
        }

        uint64 duration = diffDate(currentDate, endDate);
        if (duration < 2){
            // need 2+ days to do this
            return;
        }

        {
            sint32 nOption = state.mNumberOption.get(slotId);
            uint64 amountPerSlot = state.mBetAmountPerSlot.get(slotId);
            uint64 baseId0 = slotId * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION;
            for (sint32 i = 0; i < nOption; i++){
                uint64 baseId1 = baseId0 + i * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (sint32 j = 0; j < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; j++){
                    id bettor = state.mBettorID.get(baseId1 + j);
                    if (bettor != NULL_ID){
                        transfer(bettor, amountPerSlot);
                    }
                }
            }
        }
        {
            // cleaning bet storage
            cleanMemorySlot_input input;
            cleanMemorySlot_output output;
            input.slotId = slotId;
            cleanMemorySlot(state, input, output);
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

            state.mGameOperatorAddress = id(0x63a7317950fa8886ULL, 0x4dbdf78085364aa7ULL, 0x21c6ca41e95bfa65ULL, 0xcbc1886b3ea8e647ULL);
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