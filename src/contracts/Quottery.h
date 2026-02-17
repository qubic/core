using namespace QPI;

// FIXED CONSTANTS
constexpr unsigned long long QUOTTERY_INITIAL_MAX_BET = 1024;
constexpr unsigned long long QUOTTERY_MAX_BET = QUOTTERY_INITIAL_MAX_BET * X_MULTIPLIER;
constexpr unsigned long long QUOTTERY_MAX_OPTION = 8;
constexpr unsigned long long QUOTTERY_MAX_ORACLE_PROVIDER = 8;
constexpr unsigned long long QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET = 2048;

// adjustable with no change:
constexpr unsigned long long QUOTTERY_FEE_PER_SLOT_PER_HOUR = 10ULL;
constexpr unsigned long long QUOTTERY_MIN_AMOUNT_PER_BET_SLOT_ = 10000ULL;

constexpr unsigned long long QUOTTERY_SHAREHOLDER_FEE_ = 1000; // 10%
constexpr unsigned long long QUOTTERY_GAME_OPERATOR_FEE_ = 50; // 0.5%
constexpr unsigned long long QUOTTERY_BURN_FEE_ = 200; // 2%
STATIC_ASSERT(QUOTTERY_BURN_FEE_ > 0, BurningRequiredToOperate);
constexpr unsigned long long QUOTTERY_TICK_TO_KEEP_AFTER_END = 100ULL;




enum QUOTTERYLogInfo {
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
    betIsAlreadyFinalized = 13,
    totalError = 14
};
struct QUOTTERYLogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    // Other data go here
    sint8 _terminator; // Only data before "_terminator" are logged
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
    struct basicInfo_input
    {
    };
    struct basicInfo_output
    {
        uint64 feePerSlotPerHour; // Amount of qus
        uint64 gameOperatorFee; // 4 digit number ABCD means AB.CD% | 1234 is 12.34%
        uint64 shareholderFee; // 4 digit number ABCD means AB.CD% | 1234 is 12.34%
        uint64 minBetSlotAmount; // amount of qus
        uint64 burnFee; // percentage
        uint32 nIssuedBet; // number of issued bet
        uint64 moneyFlow;
        uint64 moneyFlowThroughIssueBet;
        uint64 moneyFlowThroughJoinBet;
        uint64 moneyFlowThroughFinalizeBet;
        uint64 earnedAmountForShareHolder;
        uint64 paidAmountForShareHolder;
        uint64 earnedAmountForBetWinner;
        uint64 distributedAmount;
        uint64 burnedAmount;
        id gameOperator;
    };
    struct getBetInfo_input
    {
        uint32 betId;
    };
    struct getBetInfo_output
    {
        // meta data info
        uint32 betId;
        uint32 nOption;      // options number
        id creator;
        id betDesc;      // 32 bytes 
        id_8 optionDesc;  // 8x(32)=256bytes
        id_8 oracleProviderId; // 256x8=2048bytes
        uint32_8 oracleFees;   // 4x8 = 32 bytes

        uint32 openDate;     // creation date, start to receive bet
        uint32 closeDate;    // stop receiving bet date
        uint32 endDate;       // result date
        // Amounts and numbers
        uint64 minBetAmount;
        uint32 maxBetSlotPerOption;
        uint32_8 currentBetState; // how many bet slots have been filled on each option
        sint8_8 betResultWonOption;
        sint8_8 betResultOPId;
    };
    struct issueBet_input
    {
        id betDesc;
        id_8 optionDesc;
        id_8 oracleProviderId; // 256x8=2048bytes
        uint32_8 oracleFees;   // 4x8 = 32 bytes
        uint32 closeDate;
        uint32 endDate;
        uint64 amountPerSlot;
        uint32 maxBetSlotPerOption;
        uint32 numberOfOption;
    };
    struct issueBet_output
    {
    };
    struct joinBet_input
    {
        uint32 betId;
        uint32 numberOfSlot;
        uint32 option;
        uint32 _placeHolder;
    };
    struct joinBet_output
    {
    };
    struct getBetOptionDetail_input
    {
        uint32 betId;
        uint32 betOption;
    };
    struct getBetOptionDetail_output
    {
        Array<id, 1024> bettor;
    };
    struct getActiveBet_input
    {
    };
    struct getActiveBet_output
    {
        uint32 count;
        Array<uint32, 1024> activeBetId;
    };
    struct getBetByCreator_input
    {
        id creator;
    };
    struct getBetByCreator_output
    {
        uint32 count;
        Array<uint32, 1024> betId;
    };
    struct cancelBet_input
    {
        uint32 betId;
    };
    struct cancelBet_output
    {
    };
    struct publishResult_input
    {
        uint32 betId;
        uint32 option;
    };
    struct publishResult_output
    {
    };
    
    struct cleanMemorySlot_input
    {
        sint64 slotId;
    };
    struct cleanMemorySlot_output {};
    struct cleanMemorySlot_locals
    {
        uint32 i0, i1;
        uint64 baseId0, baseId1;
    };

    struct tryFinalizeBet_input
    {
        uint32 betId;
        sint64 slotId;
    };
    struct tryFinalizeBet_output
    {
    };

    struct tryFinalizeBet_locals
    {
        sint32 i0, i1;
        uint64 baseId0, baseId1;
        sint32 numberOP, requiredVote, winOption, totalOption, voteCounter, numberOfSlot, currentState;
        uint64 amountPerSlot, totalBetSlot, potAmountTotal, feeChargedAmount, transferredAmount, fee, profitPerBetSlot, nWinBet;
        QUOTTERYLogger log;
    };

    /**************************************/
    /************CONTRACT STATES***********/
    /**************************************/
    Array<uint32, QUOTTERY_MAX_BET> mBetID;
    Array<id, QUOTTERY_MAX_BET> mCreator;
    Array<id, QUOTTERY_MAX_BET> mBetDesc;
    Array<id, QUOTTERY_MAX_BET* QUOTTERY_MAX_OPTION> mOptionDesc;
    Array<uint64, QUOTTERY_MAX_BET> mBetAmountPerSlot;
    Array<uint32, QUOTTERY_MAX_BET> mMaxNumberOfBetSlotPerOption;
    Array<id, QUOTTERY_MAX_BET* QUOTTERY_MAX_ORACLE_PROVIDER> mOracleProvider;
    Array<uint32, QUOTTERY_MAX_BET* QUOTTERY_MAX_OPTION> mOracleFees;
    Array<uint32, QUOTTERY_MAX_BET* QUOTTERY_MAX_OPTION> mCurrentBetState;
    Array<uint8, QUOTTERY_MAX_BET> mNumberOption;
    Array<uint32, QUOTTERY_MAX_BET> mOpenDate;
    Array<uint32, QUOTTERY_MAX_BET> mCloseDate;
    Array<uint32, QUOTTERY_MAX_BET> mEndDate;
    Array<bit, QUOTTERY_MAX_BET> mIsOccupied;
    Array<uint32, QUOTTERY_MAX_BET> mBetEndTick;
    //bettor info:
    Array<id, QUOTTERY_MAX_BET* QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET* QUOTTERY_MAX_OPTION> mBettorID;
    Array<uint8, QUOTTERY_MAX_BET* QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET* QUOTTERY_MAX_OPTION> mBettorBetOption;
    // bet result:
    Array<sint8, QUOTTERY_MAX_BET* QUOTTERY_MAX_OPTION> mBetResultWonOption;
    Array<sint8, QUOTTERY_MAX_BET* QUOTTERY_MAX_OPTION> mBetResultOPId;

    // static assert for developing:
    STATIC_ASSERT(sizeof(mBetID) == (sizeof(uint32) * QUOTTERY_MAX_BET), BetIdArray);
    STATIC_ASSERT(sizeof(mCreator) == (sizeof(id) * QUOTTERY_MAX_BET), CreatorArray);
    STATIC_ASSERT(sizeof(mBetDesc) == (sizeof(id) * QUOTTERY_MAX_BET), DescArray);
    STATIC_ASSERT(sizeof(mOptionDesc) == (sizeof(id) * QUOTTERY_MAX_BET * QUOTTERY_MAX_OPTION), OptionDescArray);
    STATIC_ASSERT(sizeof(mBetAmountPerSlot) == (sizeof(uint64) * QUOTTERY_MAX_BET), BetAmountPerSlotArray);
    STATIC_ASSERT(sizeof(mMaxNumberOfBetSlotPerOption) == (sizeof(uint32) * QUOTTERY_MAX_BET), NumberOfBetSlotPerOptionArray);
    STATIC_ASSERT(sizeof(mOracleProvider) == (sizeof(QPI::id) * QUOTTERY_MAX_BET * QUOTTERY_MAX_ORACLE_PROVIDER), OracleProviders);
    STATIC_ASSERT(sizeof(mOracleFees) == (sizeof(uint32) * QUOTTERY_MAX_BET * QUOTTERY_MAX_ORACLE_PROVIDER), OracleProvidersFees);
    STATIC_ASSERT(sizeof(mCurrentBetState) == (sizeof(uint32) * QUOTTERY_MAX_BET * QUOTTERY_MAX_ORACLE_PROVIDER), BetStates);
    STATIC_ASSERT(sizeof(mNumberOption) == (sizeof(uint8) * QUOTTERY_MAX_BET), NumberOfOptions);
    STATIC_ASSERT(sizeof(mOpenDate) == (sizeof(uint8) * 4 * QUOTTERY_MAX_BET), OpenDate);
    STATIC_ASSERT(sizeof(mCloseDate) == (sizeof(uint8) * 4 * QUOTTERY_MAX_BET), CloseDate);
    STATIC_ASSERT(sizeof(mEndDate) == (sizeof(uint8) * 4 * QUOTTERY_MAX_BET), EndDate);
    STATIC_ASSERT(sizeof(mBetResultWonOption) == (QUOTTERY_MAX_BET * 8), WonOptionArray);
    STATIC_ASSERT(sizeof(mBetResultOPId) == (QUOTTERY_MAX_BET * 8), OpIdArray);
    STATIC_ASSERT(sizeof(mBettorID) == (QUOTTERY_MAX_BET * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION * sizeof(id)), BettorArray);
    STATIC_ASSERT(sizeof(mBettorBetOption) == (QUOTTERY_MAX_BET * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION * sizeof(uint8)), BetOption);

    // other stats
    uint32 mCurrentBetID;
    uint64 mMoneyFlow;
    uint64 mMoneyFlowThroughIssueBet;
    uint64 mMoneyFlowThroughJoinBet;
    uint64 mMoneyFlowThroughFinalizeBet;
    uint64 mEarnedAmountForShareHolder;
    uint64 mPaidAmountForShareHolder;
    uint64 mEarnedAmountForBetWinner;
    uint64 mDistributedAmount;
    uint64 mBurnedAmount;
    // adjustable variables
    uint64 mFeePerSlotPerHour;
    uint64 mMinAmountPerBetSlot;
    uint64 mShareHolderFee;
    uint64 mBurnFee;
    uint64 mGameOperatorFee;    
    id mGameOperatorId;   


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

    /**
     * Compare 2 date in uint32 format
     * @return -1 lesser(ealier) A<B, 0 equal A=B, 1 greater(later) A>B
     */
    inline static sint32 dateCompare(uint32& A, uint32& B, sint32& i)
    {
        if (A == B) return 0;
        if (A < B) return -1;
        return 1;
    }

    /**
     * @return pack qtry datetime data from year, month, day, hour, minute, second to a uint32
     * year is counted from 24 (2024)
     */
    inline static void packQuotteryDate(uint32 _year, uint32 _month, uint32 _day, uint32 _hour, uint32 _minute, uint32 _second, uint32& res)
    {
        res = ((_year - 24) << 26) | (_month << 22) | (_day << 17) | (_hour << 12) | (_minute << 6) | (_second);
    }

    inline static uint32 qtryGetYear(uint32 data)
    {
        return ((data >> 26) + 24);
    }
    inline static uint32 qtryGetMonth(uint32 data)
    {
        return ((data >> 22) & 0b1111);
    }
    inline static uint32 qtryGetDay(uint32 data)
    {
        return ((data >> 17) & 0b11111);
    }
    inline static uint32 qtryGetHour(uint32 data)
    {
        return ((data >> 12) & 0b11111);
    }
    inline static uint32 qtryGetMinute(uint32 data)
    {
        return ((data >> 6) & 0b111111);
    }
    inline static uint32 qtryGetSecond(uint32 data)
    {
        return (data & 0b111111);
    }
    /*
    * @return unpack qtry datetime from uin32 to year, month, day, hour, minute, secon
    */
    inline static void unpackQuotteryDate(uint8& _year, uint8& _month, uint8& _day, uint8& _hour, uint8& _minute, uint8& _second, uint32 data)
    {
        _year = qtryGetYear(data); // 6 bits
        _month = qtryGetMonth(data); //4bits
        _day = qtryGetDay(data); //5bits
        _hour = qtryGetHour(data); //5bits
        _minute = qtryGetMinute(data); //6bits
        _second = qtryGetSecond(data); //6bits
    }

    inline static void accumulatedDay(sint32 month, uint64& res)
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
     * @return difference in number of second, A must be smaller than or equal B to have valid value
     */
    inline static void diffDateInSecond(uint32& A, uint32& B, sint32& i, uint64& dayA, uint64& dayB, uint64& res)
    {
        if (dateCompare(A, B, i) >= 0)
        {
            res = 0;
            return;
        }
        accumulatedDay(qtryGetMonth(A), dayA);
        dayA += qtryGetDay(A);
        accumulatedDay(qtryGetMonth(B), dayB);
        dayB += (qtryGetYear(B) - qtryGetYear(A)) * 365ULL + qtryGetDay(B);

        // handling leap-year: only store last 2 digits of year here, don't care about mod 100 & mod 400 case
        for (i = qtryGetYear(A); (uint32)(i) < qtryGetYear(B); i++)
        {
            if (mod(i, 4) == 0)
            {
                dayB++;
            }
        }
        if (mod(sint32(qtryGetYear(A)), 4) == 0 && (qtryGetMonth(A) > 2)) dayA++;
        if (mod(sint32(qtryGetYear(B)), 4) == 0 && (qtryGetMonth(B) > 2)) dayB++;
        res = (dayB - dayA) * 3600ULL * 24;
        res += (qtryGetHour(B) * 3600 + qtryGetMinute(B) * 60 + qtryGetSecond(B));
        res -= (qtryGetHour(A) * 3600 + qtryGetMinute(A) * 60 + qtryGetSecond(A));
    }

    inline static bool checkValidQtryDateTime(uint32& A)
    {
        if (qtryGetMonth(A) > 12) return false;
        if (qtryGetDay(A) > 31) return false;
        if ((qtryGetDay(A) == 31) &&
            (qtryGetMonth(A) != 1) && (qtryGetMonth(A) != 3) && (qtryGetMonth(A) != 5) &&
            (qtryGetMonth(A) != 7) && (qtryGetMonth(A) != 8) && (qtryGetMonth(A) != 10) && (qtryGetMonth(A) != 12)) return false;
        if ((qtryGetDay(A) == 30) && (qtryGetMonth(A) == 2)) return false;
        if ((qtryGetDay(A) == 29) && (qtryGetMonth(A) == 2) && (mod(qtryGetYear(A), 4u) != 0)) return false;
        if (qtryGetHour(A) >= 24) return false;
        if (qtryGetMinute(A) >= 60) return false;
        if (qtryGetSecond(A) >= 60) return false;
        return true;
    }
    /**
     * Clean all memory of a slot Id, set the flag IsOccupied to zero
     * @param slotId
     */
    PRIVATE_PROCEDURE_WITH_LOCALS(cleanMemorySlot)
    {
        state.mBetID.set(input.slotId, NULL_INDEX);
        state.mCreator.set(input.slotId, NULL_ID);
        state.mBetDesc.set(input.slotId, NULL_ID);
        {
            locals.baseId0 = input.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                state.mOptionDesc.set(locals.baseId0 + locals.i0, NULL_ID);
            }
        }
        state.mBetAmountPerSlot.set(input.slotId, 0);
        state.mMaxNumberOfBetSlotPerOption.set(input.slotId, 0);
        {
            locals.baseId0 = input.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                state.mOracleProvider.set(locals.baseId0 + locals.i0, NULL_ID);
                state.mOracleFees.set(locals.baseId0 + locals.i0, 0);
                state.mCurrentBetState.set(locals.baseId0 + locals.i0, 0);
            }
        }
        state.mNumberOption.set(input.slotId, 0);
        {
            state.mOpenDate.set(input.slotId, 0);
            state.mCloseDate.set(input.slotId, 0);
            state.mEndDate.set(input.slotId, 0);
        }
        state.mIsOccupied.set(input.slotId, 0); // close the bet
        state.mBetEndTick.set(input.slotId, 0); // set end tick to null
        {
            locals.baseId0 = input.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_OPTION; locals.i0++)
            {
                locals.baseId1 = locals.baseId0 + locals.i0 * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (locals.i1 = 0; locals.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; locals.i1++)
                {
                    state.mBettorID.set(locals.baseId1 + locals.i1, NULL_ID);
                    state.mBettorBetOption.set(locals.baseId1 + locals.i1, NULL_INDEX);
                }
            }
        }

        {
            // clean OP votes
            locals.baseId0 = input.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                state.mBetResultWonOption.set(locals.baseId0 + locals.i0, (sint8)(NULL_INDEX));
                state.mBetResultOPId.set(locals.baseId0 + locals.i0, (sint8)(NULL_INDEX));
            }
        }
    }

    struct checkAndCleanMemorySlots_input
    {
    };
    struct checkAndCleanMemorySlots_output
    {
    };
    struct checkAndCleanMemorySlots_locals
    {
        int i;
        cleanMemorySlot_locals cms;
        cleanMemorySlot_input _cleanMemorySlot_input;
        cleanMemorySlot_output _cleanMemorySlot_output;
    };
    /**
    * Scan through all memory slots and clean any expired bet (>100 ticks)
    */
    PRIVATE_PROCEDURE_WITH_LOCALS(checkAndCleanMemorySlots)
    {
        for (locals.i = 0; locals.i < QUOTTERY_MAX_BET; locals.i++)
        {
            if ((state.mIsOccupied.get(locals.i) == 1) // if the bet is currently occupied
                && (state.mBetEndTick.get(locals.i) != 0) // bet end tick marker is not null
                && (state.mBetEndTick.get(locals.i) < qpi.tick() + QUOTTERY_TICK_TO_KEEP_AFTER_END) // the bet is already ended more than 100 ticks
                )
            {
                // cleaning bet storage
                locals._cleanMemorySlot_input.slotId = locals.i;
                cleanMemorySlot(qpi, state, locals._cleanMemorySlot_input, locals._cleanMemorySlot_output, locals.cms);
            }
        }
    }

    /**
    * Try to finalize a bet when the system receive a result publication from a oracle provider
    * If there are 2/3 votes agree with the same result, the bet is finalized.
    * @param betId, slotId
    */
    PRIVATE_PROCEDURE_WITH_LOCALS(tryFinalizeBet)
    {
        locals.numberOP = 0;
        {
            locals.baseId0 = input.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                if (state.mOracleProvider.get(locals.baseId0 + locals.i0) != NULL_ID)
                {
                    locals.numberOP++;
                }
            }
        }
        locals.requiredVote = divUp(uint32(locals.numberOP * 2), 3U);
        locals.winOption = -1;
        locals.totalOption = state.mNumberOption.get(input.slotId);
        for (locals.i0 = 0; locals.i0 < locals.totalOption; locals.i0++)
        {
            locals.voteCounter = 0;
            locals.baseId0 = input.slotId * 8;
            for (locals.i1 = 0; locals.i1 < locals.numberOP; locals.i1++)
            {
                if (state.mBetResultWonOption.get(locals.baseId0 + locals.i1) == locals.i0)
                {
                    locals.voteCounter++;
                }
            }
            if (locals.voteCounter >= locals.requiredVote)
            {
                locals.winOption = locals.i0;
                break;
            }
        }
        if (locals.winOption != -1)
        {
            //distribute coins
            locals.amountPerSlot = state.mBetAmountPerSlot.get(input.slotId);
            locals.totalBetSlot = 0;
            locals.baseId0 = input.slotId * 8;
            for (locals.i0 = 0; locals.i0 < locals.totalOption; locals.i0++)
            {
                locals.totalBetSlot += state.mCurrentBetState.get(locals.baseId0 + locals.i0);
            }
            locals.nWinBet = state.mCurrentBetState.get(locals.baseId0 + locals.winOption);

            locals.potAmountTotal = locals.totalBetSlot * locals.amountPerSlot;
            locals.feeChargedAmount = locals.potAmountTotal - locals.nWinBet * locals.amountPerSlot; // only charge winning amount
            locals.transferredAmount = 0;
            // fee to OP
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                if (state.mOracleProvider.get(locals.baseId0 + locals.i0) != NULL_ID)
                {
                    locals.fee = QPI::div(locals.feeChargedAmount * state.mOracleFees.get(locals.baseId0 + locals.i0), 10000ULL);
                    if (locals.fee != 0)
                    {
                        qpi.transfer(state.mOracleProvider.get(locals.baseId0 + locals.i0), locals.fee);
                        locals.transferredAmount += locals.fee;
                    }
                }
            }
            // fee to share holders
            {
                locals.fee = QPI::div(locals.feeChargedAmount * state.mShareHolderFee, 10000ULL);
                locals.transferredAmount += locals.fee; // will transfer to shareholders at the end of epoch
                state.mEarnedAmountForShareHolder += locals.fee;
                state.mMoneyFlow += locals.fee;
                state.mMoneyFlowThroughFinalizeBet += locals.fee;
            }
            // fee to game operator
            {
                locals.fee = QPI::div(locals.feeChargedAmount * state.mGameOperatorFee, 10000ULL);
                qpi.transfer(state.mGameOperatorId, locals.fee);
                locals.transferredAmount += locals.fee;
            }
            // burn qu
            {
                locals.fee = QPI::div(locals.feeChargedAmount * state.mBurnFee, 10000ULL);
                qpi.burn(locals.fee);
                locals.transferredAmount += locals.fee;
                state.mBurnedAmount += locals.fee;
            }
            // profit to winners
            {
                locals.feeChargedAmount -= locals.transferredAmount; // left over go to winners
                locals.profitPerBetSlot = QPI::div(locals.feeChargedAmount, locals.nWinBet);
                locals.baseId0 = input.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                locals.baseId1 = locals.baseId0 + locals.winOption * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (locals.i1 = 0; locals.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; locals.i1++)
                {
                    if (state.mBettorID.get(locals.baseId1 + locals.i1) != NULL_ID)
                    {
                        qpi.transfer(state.mBettorID.get(locals.baseId1 + locals.i1), locals.amountPerSlot + locals.profitPerBetSlot);
                        state.mEarnedAmountForBetWinner += (locals.amountPerSlot + locals.profitPerBetSlot);
                        state.mDistributedAmount += (locals.amountPerSlot + locals.profitPerBetSlot);
                        locals.transferredAmount += (locals.amountPerSlot + locals.profitPerBetSlot);
                    }
                }
            }
            // in a rare case, if no one bet on winning option, burn all of the QUs
            if (locals.transferredAmount < locals.potAmountTotal)
            {
                qpi.burn(locals.potAmountTotal - locals.transferredAmount);
                state.mBurnedAmount += (locals.potAmountTotal - locals.transferredAmount);
            }
            state.mBetEndTick.set(input.slotId, qpi.tick());
        }
        else
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::notEnoughVote,0 };
            LOG_INFO(locals.log);
        }
    }
    /**************************************/
    /************VIEW FUNCTIONS************/
    /**************************************/
    /**
     * @return feePerSlotPerDay, gameOperatorFee, shareholderFee, minBetSlotAmount, gameOperator
     */
    PUBLIC_FUNCTION(basicInfo)
    {
        output.feePerSlotPerHour = state.mFeePerSlotPerHour;
        output.gameOperatorFee = state.mGameOperatorFee;
        output.shareholderFee = state.mShareHolderFee;
        output.minBetSlotAmount = state.mMinAmountPerBetSlot;
        output.burnFee = state.mBurnFee;
        output.nIssuedBet = state.mCurrentBetID;
        output.moneyFlow = state.mMoneyFlow;
        output.moneyFlowThroughJoinBet = state.mMoneyFlowThroughJoinBet;
        output.moneyFlowThroughIssueBet = state.mMoneyFlowThroughIssueBet;
        output.moneyFlowThroughFinalizeBet = state.mMoneyFlowThroughFinalizeBet;
        output.earnedAmountForBetWinner = state.mEarnedAmountForBetWinner;
        output.earnedAmountForShareHolder = state.mEarnedAmountForShareHolder;
        output.distributedAmount = state.mDistributedAmount;
        output.burnedAmount = state.mBurnedAmount;
        output.gameOperator = state.mGameOperatorId;
    }

    struct getBetInfo_locals
    {
        sint64 slotId;
        uint64 baseId0, baseId1, u64Tmp;
        uint32 i0, i1;
    };
    /**
     * @param betId
     * @return meta data of a bet and its current state
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getBetInfo)
    {
        output.betId = NULL_INDEX;
        locals.slotId = NULL_INDEX;
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_BET; locals.i0++)
        {
            if (state.mBetID.get(locals.i0) == input.betId)
            {
                locals.slotId = locals.i0;
                break;
            }
        }
        if (locals.slotId == NULL_INDEX)
        {
            // can't find betId
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(locals.slotId) == 0)
        {
            // the bet is over
            return;
        }
        output.betId = input.betId;
        output.nOption = state.mNumberOption.get(locals.slotId);
        output.creator = state.mCreator.get(locals.slotId);
        output.betDesc = state.mBetDesc.get(locals.slotId);
        {
            // option desc
            locals.baseId0 = locals.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                output.optionDesc.set(locals.i0, state.mOptionDesc.get(locals.baseId0 + locals.i0));
            }
        }
        {
            // oracle
            locals.baseId0 = locals.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                output.oracleProviderId.set(locals.i0, state.mOracleProvider.get(locals.baseId0 + locals.i0));
                output.oracleFees.set(locals.i0, state.mOracleFees.get(locals.baseId0 + locals.i0));
            }
        }
        {
            output.openDate = state.mOpenDate.get(locals.slotId);
            output.closeDate = state.mCloseDate.get(locals.slotId);
            output.endDate = state.mEndDate.get(locals.slotId);
        }
        locals.u64Tmp = (uint64)(locals.slotId); // just reduce warnings
        output.minBetAmount = state.mBetAmountPerSlot.get(locals.u64Tmp);
        output.maxBetSlotPerOption = state.mMaxNumberOfBetSlotPerOption.get(locals.u64Tmp);
        {
            locals.baseId0 = locals.u64Tmp * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                output.currentBetState.set(locals.i0, state.mCurrentBetState.get(locals.baseId0 + locals.i0));
            }
        }

        // get OP votes if possible
        {
            locals.u64Tmp = (uint64)(locals.slotId); // just reduce warnings
            locals.baseId0 = locals.u64Tmp * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                output.betResultWonOption.set(locals.i0, state.mBetResultWonOption.get(locals.baseId0 + locals.i0));
                output.betResultOPId.set(locals.i0, state.mBetResultOPId.get(locals.baseId0 + locals.i0));
            }
        }
    }

    struct getBetOptionDetail_locals
    {
        sint64 slotId;
        uint64 baseId0, baseId1;
        uint32 i0;
    };
    /**
     * @param betID, optionID
     * @return a list of ID that bet on optionID of bet betID
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getBetOptionDetail)
    {
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; locals.i0++)
        {
            output.bettor.set(locals.i0, NULL_ID);
        }
        locals.slotId = -1;
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_BET; locals.i0++)
        {
            if (state.mBetID.get(locals.i0) == input.betId)
            {
                locals.slotId = locals.i0;
                break;
            }
        }
        if (locals.slotId == -1)
        {
            // can't find betId
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(locals.slotId) == 0)
        {
            // the bet is over
            return;
        }
        if (input.betOption >= state.mNumberOption.get(locals.slotId))
        {
            // invalid betOption
            return;
        }
        locals.baseId0 = locals.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
        locals.baseId1 = locals.baseId0 + input.betOption * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; locals.i0++)
        {
            output.bettor.set(locals.i0, state.mBettorID.get(locals.baseId1 + locals.i0));
        }
    }

    struct getActiveBet_locals
    {
        sint64 slotId;
    };
    /**
     * @return a list of active betID
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getActiveBet)
    {
        output.count = 0;
        for (locals.slotId = 0; locals.slotId < QUOTTERY_MAX_BET; locals.slotId++)
        {
            if (state.mBetID.get(locals.slotId) != NULL_INDEX)
            {
                if (state.mIsOccupied.get(locals.slotId) == 1)
                { //not available == active
                    output.activeBetId.set(output.count, state.mBetID.get(locals.slotId));
                    output.count++;
                }
            }
        }
    }

    struct getBetByCreator_locals
    {
        sint64 slotId;
    };
    /**
    * @param creatorID
    * @return a list of active betID that created by creatorID
    */
    PUBLIC_FUNCTION_WITH_LOCALS(getBetByCreator)
    {
        output.count = 0;
        for (locals.slotId = 0; locals.slotId < QUOTTERY_MAX_BET; locals.slotId++)
        {
            if (state.mBetID.get(locals.slotId) != NULL_INDEX)
            {
                if (state.mIsOccupied.get(locals.slotId) == 1)
                { //not available == active
                    if (state.mCreator.get(locals.slotId) == input.creator)
                    {
                        output.betId.set(output.count, state.mBetID.get(locals.slotId));
                        output.count++;
                    }
                }
            }
        }
    }

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
    struct issueBet_locals
    {
        uint32 curDate;
        sint32 i0, i1;
        uint64 baseId0, baseId1;
        sint32 numberOP;
        sint64 fee;
        uint64 maxBetSlotPerOption, duration, u64_0, u64_1;
        uint8 numberOfOption;
        uint32 betId;
        sint64 slotId;
        checkAndCleanMemorySlots_input _checkAndCleanMemorySlots_input;
        checkAndCleanMemorySlots_output _checkAndCleanMemorySlots_output;
        checkAndCleanMemorySlots_locals _checkAndCleanMemorySlots_locals;
        QUOTTERYLogger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(issueBet)
    {
        // only allow users to create bet
        if (qpi.invocator() != qpi.originator())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        if (!checkValidQtryDateTime(input.closeDate) || !checkValidQtryDateTime(input.endDate))
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidDate,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        // check valid date: closeDate <= endDate
        if (dateCompare(input.closeDate, input.endDate, locals.i0) == 1 ||
            dateCompare(locals.curDate, input.closeDate, locals.i0) == 1)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidDate,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (input.amountPerSlot < state.mMinAmountPerBetSlot)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidBetAmount,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (input.numberOfOption > QUOTTERY_MAX_OPTION || input.numberOfOption < 2)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidOption,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (input.maxBetSlotPerOption == 0 || input.maxBetSlotPerOption > QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidMaxBetSlotPerOption,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        locals.maxBetSlotPerOption = input.maxBetSlotPerOption;
        locals.numberOfOption = input.numberOfOption;
        diffDateInSecond(locals.curDate, input.endDate, locals.i0, locals.u64_0, locals.u64_1, locals.duration);
        locals.duration = divUp(locals.duration, 3600ULL);
        locals.fee = locals.duration * locals.maxBetSlotPerOption * locals.numberOfOption * state.mFeePerSlotPerHour;

        // fee is higher than sent amount, exit
        if (locals.fee > qpi.invocationReward())
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::insufficientFund,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        // clean any expired bet slot
        checkAndCleanMemorySlots(qpi, state, locals._checkAndCleanMemorySlots_input, locals._checkAndCleanMemorySlots_output, locals._checkAndCleanMemorySlots_locals);

        locals.betId = state.mCurrentBetID++;
        locals.slotId = -1;
        // find an empty slot
        {
            locals.i0 = 0;
            locals.i1 = locals.betId & (QUOTTERY_MAX_BET - 1);
            while (locals.i0++ < QUOTTERY_MAX_BET)
            {
                if (state.mIsOccupied.get(locals.i1) == 0)
                {
                    locals.slotId = locals.i1;
                    break;
                }
                locals.i1 = (locals.i1 + 1) & (QUOTTERY_MAX_BET - 1);
            }
        }
        //out of bet storage, exit
        if (locals.slotId == -1)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::outOfStorage,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        // return the fee changes
        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }
        state.mMoneyFlow += locals.fee;
        state.mMoneyFlowThroughIssueBet += locals.fee;
        state.mEarnedAmountForShareHolder += locals.fee;
        // write data to slotId
        state.mBetID.set(locals.slotId, locals.betId); // 1
        state.mNumberOption.set(locals.slotId, locals.numberOfOption); // 2
        state.mCreator.set(locals.slotId, qpi.invocator()); // 3
        state.mBetDesc.set(locals.slotId, input.betDesc); // 4
        state.mBetAmountPerSlot.set(locals.slotId, input.amountPerSlot); // 5
        state.mMaxNumberOfBetSlotPerOption.set(locals.slotId, input.maxBetSlotPerOption); //6
        locals.numberOP = 0;
        {
            // write option desc, oracle provider, oracle fee, bet state
            locals.baseId0 = locals.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                state.mOptionDesc.set(locals.baseId0 + locals.i0, input.optionDesc.get(locals.i0)); // 7
                state.mOracleProvider.set(locals.baseId0 + locals.i0, input.oracleProviderId.get(locals.i0)); // 8
                if (input.oracleProviderId.get(locals.i0) != NULL_ID)
                {
                    locals.numberOP++;
                }
                state.mOracleFees.set(locals.baseId0 + locals.i0, input.oracleFees.get(locals.i0)); // 9
                state.mCurrentBetState.set(locals.baseId0 + locals.i0, 0); //10
                state.mBetResultWonOption.set(locals.baseId0 + locals.i0, -1);
                state.mBetResultOPId.set(locals.baseId0 + locals.i0, -1);
            }
        }
        if (locals.numberOP == 0)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidNumberOfOracleProvider,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        {
            // write date and time
            state.mOpenDate.set(locals.slotId, locals.curDate);
            state.mCloseDate.set(locals.slotId, input.closeDate);
            state.mEndDate.set(locals.slotId, input.endDate);
        }
        state.mIsOccupied.set(locals.betId, 1); // 14
        // done write, 14 fields
        // set zero to bettor info
        {
            locals.baseId0 = locals.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
            for (locals.i0 = 0; locals.i0 < locals.numberOfOption; locals.i0++)
            {
                locals.baseId1 = locals.baseId0 + locals.i0 * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (locals.i1 = 0; locals.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; locals.i1++)
                {
                    state.mBettorID.set(locals.baseId1 + locals.i1, NULL_ID);
                    state.mBettorBetOption.set(locals.baseId1 + locals.i1, NULL_INDEX);
                }
            }
        }
    }

    struct joinBet_locals
    {
        uint32 curDate, closeDate;
        sint32 i0, i2, i3;
        uint64 baseId0;
        sint32 numberOfSlot, currentState;
        sint64 amountPerSlot, fee;
        uint32 availableSlotForBet;
        sint64 slotId;
        QUOTTERYLogger log;
    };
    /**
    * Join a bet
    * if the provided info is failed to join a bet, fund will be returned to invocator.
    * @param betId (4 bytes)
    * @param numberOfSlot (4 bytes): number of bet slot that invocator wants to join
    * @param option (4 bytes): the option Id that invocator wants to bet
    * @param _placeHolder (4 bytes): for padding
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(joinBet)
    {
        // only allow users to join bet
        if (qpi.invocator() != qpi.originator())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        locals.slotId = -1;
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_BET; locals.i0++)
        {
            if (state.mBetID.get(locals.i0) == input.betId)
            {
                locals.slotId = locals.i0;
                break;
            }
        }
        if (locals.slotId == -1)
        {
            // can't find betId
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidBetId,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(locals.slotId) == 0)
        {
            // the bet is over
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        locals.closeDate = state.mCloseDate.get(locals.slotId);

        if (dateCompare(locals.curDate, locals.closeDate, locals.i0) > 0)
        {
            // bet is closed for betting
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::expiredBet,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // check invalid option
        if (input.option >= state.mNumberOption.get(locals.slotId))
        {
            // bet is closed for betting
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidOption,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // compute available number of options
        locals.availableSlotForBet = 0;
        {
            locals.baseId0 = locals.slotId * 8;
            locals.availableSlotForBet = state.mMaxNumberOfBetSlotPerOption.get(locals.slotId) -
                state.mCurrentBetState.get(locals.baseId0 + input.option);
        }
        locals.numberOfSlot = min(locals.availableSlotForBet, input.numberOfSlot);
        if (locals.numberOfSlot == 0)
        {
            // out of slot
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::outOfSlot,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        locals.amountPerSlot = state.mBetAmountPerSlot.get(locals.slotId);
        locals.fee = locals.amountPerSlot * locals.numberOfSlot;
        if (locals.fee > qpi.invocationReward())
        {
            // not send enough amount
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::insufficientFund,0 };
            LOG_INFO(locals.log);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (locals.fee < qpi.invocationReward())
        {
            // return changes
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }

        state.mMoneyFlow += locals.fee;
        state.mMoneyFlowThroughJoinBet += locals.fee;
        // write bet info to memory
        {
            locals.i2 = QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET - locals.availableSlotForBet;
            locals.i3 = locals.i2 + locals.numberOfSlot;
            locals.baseId0 = locals.slotId * QUOTTERY_MAX_OPTION * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET + QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * input.option;
            for (locals.i0 = locals.i2; locals.i0 < locals.i3; locals.i0++)
            {
                state.mBettorID.set(locals.baseId0 + locals.i0, qpi.invocator());
                state.mBettorBetOption.set(locals.baseId0 + locals.i0, input.option);
            }
        }
        //update stats
        {
            locals.baseId0 = locals.slotId * 8;
            locals.currentState = state.mCurrentBetState.get(locals.baseId0 + input.option);
            state.mCurrentBetState.set(locals.baseId0 + input.option, locals.currentState + locals.numberOfSlot);
        }
        // done
    }
    
    struct publishResult_locals
    {
        uint32 curDate, endDate;
        sint32 i0;
        uint64 baseId0;
        sint64 slotId, writeId;
        sint8 opId;
        QUOTTERYLogger log;
        tryFinalizeBet_locals tfb;
        tryFinalizeBet_input _tryFinalizeBet_input;
        tryFinalizeBet_output _tryFinalizeBet_output;
    };
    /**
    * Publish result of a bet (Oracle Provider only)
    * After the vote is updated, it will try to finalize the bet, see tryFinalizeBet
    * @param betId (4 bytes)
    * @param option (4 bytes): winning option
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(publishResult)
    {
        // only allow users to publish result
        if (qpi.invocator() != qpi.originator())
        {
            return;
        }
        locals.slotId = -1;
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_BET; locals.i0++)
        {
            if (state.mBetID.get(locals.i0) == input.betId)
            {
                locals.slotId = locals.i0;
                break;
            }
        }
        if (locals.slotId == -1)
        {
            // can't find betId
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidBetId,0 };
            LOG_INFO(locals.log);
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(locals.slotId) == 0)
        {
            // the bet is over
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::expiredBet,0 };
            LOG_INFO(locals.log);
            return;
        }
        packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        locals.endDate = state.mEndDate.get(locals.slotId);
        // endDate is counted as 23:59 of that day
        if (dateCompare(locals.curDate, locals.endDate, locals.i0) <= 0)
        {
            // bet is not end yet
            return;
        }

        locals.opId = -1;
        {
            locals.baseId0 = locals.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                if (state.mOracleProvider.get(locals.baseId0 + locals.i0) == qpi.invocator())
                {
                    locals.opId = locals.i0;
                    break;
                }
            }
            if (locals.opId == -1)
            {
                // is not oracle provider
                locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidOPId,0 };
                LOG_INFO(locals.log);
                return;
            }
        }

        // check if bet is already finalized
        {
            if (state.mBetEndTick.get(locals.slotId) != 0)
            {
                // is already finalized
                locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::betIsAlreadyFinalized,0 };
                LOG_INFO(locals.log);
                return;
            }
        }

        // check if this OP already published result
        locals.writeId = -1;
        {
            locals.baseId0 = locals.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                if (state.mBetResultOPId.get(locals.baseId0 + locals.i0) == locals.opId)
                {
                    locals.writeId = locals.i0;
                    break;
                }
            }
        }

        // OP already published result, now they want to change
        if (locals.writeId != -1)
        {
            locals.baseId0 = locals.slotId * 8;
            state.mBetResultOPId.set(locals.baseId0 + locals.writeId, locals.opId);
            state.mBetResultWonOption.set(locals.baseId0 + locals.writeId, input.option);
        }
        else
        {
            locals.baseId0 = locals.slotId * 8;
            for (locals.i0 = 0; locals.i0 < 8; locals.i0++)
            {
                if (state.mBetResultOPId.get(locals.baseId0 + locals.i0) == -1 && state.mBetResultWonOption.get(locals.baseId0 + locals.i0) == -1)
                {
                    locals.writeId = locals.i0;
                    break;
                }
            }
            state.mBetResultOPId.set(locals.baseId0 + locals.writeId, locals.opId);
            state.mBetResultWonOption.set(locals.baseId0 + locals.writeId, input.option);
        }

        // try to finalize the bet
        {
            locals._tryFinalizeBet_input.betId = input.betId;
            locals._tryFinalizeBet_input.slotId = locals.slotId;
            tryFinalizeBet(qpi, state, locals._tryFinalizeBet_input, locals._tryFinalizeBet_output, locals.tfb);
        }

    }

    struct cancelBet_locals
    {
        uint32 curDate, endDate;
        sint32 i0, i1;
        uint64 baseId0, baseId1, nOption;
        uint64 amountPerSlot;
        uint64 duration, u64_0, u64_1;
        sint64 slotId;
        QUOTTERYLogger log;
        cleanMemorySlot_locals cms;
        cleanMemorySlot_input _cleanMemorySlot_input;
        cleanMemorySlot_output _cleanMemorySlot_output;
    };
    /**
    * Publish cancel a bet (Game Operator only)
    * In ermergency case, this function removes a bet from system and return all funds back to original invocators (except issueing fee)
    * @param betId (4 bytes)
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(cancelBet)
    {
        // game operator invocation only. In any case that oracle providers can't reach consensus or unable to broadcast result after a long period,         
        // all funds will be returned
        if (qpi.invocator() != state.mGameOperatorId)
        {
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::notGameOperator,0 };
            LOG_INFO(locals.log);
            return;
        }
        locals.slotId = -1;
        for (locals.i0 = 0; locals.i0 < QUOTTERY_MAX_BET; locals.i0++)
        {
            if (state.mBetID.get(locals.i0) == input.betId)
            {
                locals.slotId = locals.i0;
                break;
            }
        }
        if (locals.slotId == -1)
        {
            // can't find betId
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::invalidBetId,0 };
            LOG_INFO(locals.log);
            return;
        }
        // check if the bet is occupied
        if (state.mIsOccupied.get(locals.slotId) == 0)
        {
            // the bet is over
            locals.log = QUOTTERYLogger{ 0,QUOTTERYLogInfo::expiredBet,0 };
            LOG_INFO(locals.log);
            return;
        }
        packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        locals.endDate = state.mEndDate.get(locals.slotId);

        // endDate is counted as 23:59 of that day
        if (dateCompare(locals.curDate, locals.endDate, locals.i0) <= 0)
        {
            // bet is not end yet
            return;
        }

        diffDateInSecond(locals.endDate, locals.curDate, locals.i0, locals.u64_0, locals.u64_1, locals.duration);
        locals.duration = div(locals.duration, 3600ULL * 24);
        if (locals.duration < 2)
        {
            // need 2+ days to do this
            return;
        }

        {
            locals.nOption = state.mNumberOption.get(locals.slotId);
            locals.amountPerSlot = state.mBetAmountPerSlot.get(locals.slotId);
            locals.baseId0 = locals.slotId * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET * QUOTTERY_MAX_OPTION;
            for (locals.i0 = 0; locals.i0 < locals.nOption; locals.i0++)
            {
                locals.baseId1 = locals.baseId0 + locals.i0 * QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET;
                for (locals.i1 = 0; locals.i1 < QUOTTERY_MAX_SLOT_PER_OPTION_PER_BET; locals.i1++)
                {
                    if (state.mBettorID.get(locals.baseId1 + locals.i1) != NULL_ID)
                    {
                        qpi.transfer(state.mBettorID.get(locals.baseId1 + locals.i1), locals.amountPerSlot);
                    }
                }
            }
        }
        {
            // cleaning bet storage
            locals._cleanMemorySlot_input.slotId = locals.slotId;
            cleanMemorySlot(qpi, state, locals._cleanMemorySlot_input, locals._cleanMemorySlot_output, locals.cms);
        }
    }

        
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(basicInfo, 1);
        REGISTER_USER_FUNCTION(getBetInfo, 2);
        REGISTER_USER_FUNCTION(getBetOptionDetail, 3);
        REGISTER_USER_FUNCTION(getActiveBet, 4);
        REGISTER_USER_FUNCTION(getBetByCreator, 5);

        REGISTER_USER_PROCEDURE(issueBet, 1);
        REGISTER_USER_PROCEDURE(joinBet, 2);
        REGISTER_USER_PROCEDURE(cancelBet, 3);
        REGISTER_USER_PROCEDURE(publishResult, 4);
    }

    BEGIN_EPOCH()
    {
        state.mFeePerSlotPerHour = QUOTTERY_FEE_PER_SLOT_PER_HOUR;
        state.mMinAmountPerBetSlot = QUOTTERY_MIN_AMOUNT_PER_BET_SLOT_;
        state.mShareHolderFee = QUOTTERY_SHAREHOLDER_FEE_;
        state.mGameOperatorFee = QUOTTERY_GAME_OPERATOR_FEE_;
        state.mBurnFee = QUOTTERY_BURN_FEE_;
        state.mGameOperatorId = id(0x63a7317950fa8886ULL, 0x4dbdf78085364aa7ULL, 0x21c6ca41e95bfa65ULL, 0xcbc1886b3ea8e647ULL);
    }

    END_EPOCH()
    {
        if ((div(state.mEarnedAmountForShareHolder - state.mPaidAmountForShareHolder, uint64(NUMBER_OF_COMPUTORS)) > 0) && (state.mEarnedAmountForShareHolder > state.mPaidAmountForShareHolder))
        {
            if (qpi.distributeDividends(div(state.mEarnedAmountForShareHolder - state.mPaidAmountForShareHolder, uint64(NUMBER_OF_COMPUTORS))))
            {
                state.mPaidAmountForShareHolder += div(state.mEarnedAmountForShareHolder - state.mPaidAmountForShareHolder, uint64(NUMBER_OF_COMPUTORS)) * NUMBER_OF_COMPUTORS;
            }
        }
    }
};
