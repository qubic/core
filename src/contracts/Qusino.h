using namespace QPI;

constexpr uint64 QUSINO_MAX_USERS = 131072;
constexpr uint64 QUSINO_MAX_NUMBER_OF_GAMES = 131072;
constexpr uint64 QUSINO_GAME_SUBMIT_FEE = 100000000;
constexpr uint32 QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER = 64;
constexpr uint32 QUSINO_REVOTE_DURATION = 78;        // in number of weeks(18 Months)
constexpr uint32 QUSINO_STAR_BONUS_FOR_QSC = 1000;
constexpr uint32 QUSINO_STAR_PRICE = 1;
constexpr uint32 QUSINO_VOTE_FEE = 1000;
constexpr uint32 QUSINO_LP_DIVIDENDS_PERCENT = 20;
constexpr uint32 QUSINO_CCF_DIVIDENDS_PERCENT = 5;
constexpr uint32 QUSINO_TREASURY_DIVIDENDS_PERCENT = 25;
constexpr uint32 QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT = 20;
constexpr uint32 QUSINO_QST_HOLDERS_DIVIDENDS_PERCENT = 30;
constexpr uint64 QUSINO_INFINITY_PRICE = 1000000000000000000ULL;
constexpr uint64 QUSINO_QSC_PRICE = 100;    // 1QSC = 100Qubic
constexpr uint64 QUSINO_DEVELOPER_FEE = 333;             // 33.3%
constexpr uint64 QUSINO_SUPPLY_OF_QST = 1200000000ULL;    // 1.2 billion
constexpr uint64 QUSINO_DAILY_CLAIM_BONUS_DURATION = 24 * 60 * 60; // in number of seconds
constexpr uint64 QUSINO_BONUS_CLAIM_DURATION = 60;   // 60s
constexpr uint64 QUSINO_BONUS_CLAIM_AMOUNT = 100;    // 100STAR + 1QSC = 100Qubic,  STAR isnt redeemable for qubic.
constexpr uint64 QUSINO_BONUS_CLAIM_AMOUNT_STAR = 100;
constexpr uint64 QUSINO_BONUS_CLAIM_AMOUNT_QSC = 1;

constexpr sint32 QUSINO_SUCCESS = 0;
constexpr sint32 QUSINO_INSUFFICIENT_FUNDS = 1;
constexpr sint32 QUSINO_INSUFFICIENT_STAR = 2;
constexpr sint32 QUSINO_INSUFFICIENT_QSC = 3;
constexpr sint32 QUSINO_INSUFFICIENT_VOTE_FEE = 6;
constexpr sint32 QUSINO_WRONG_GAME_URI_FOR_VOTE = 7;
constexpr sint32 QUSINO_ALREADY_VOTED = 8;
constexpr sint32 QUSINO_NOT_VOTE_TIME = 9;
constexpr sint32 QUSINO_NO_EMPTY_SLOT = 10;
constexpr sint32 QUSINO_INSUFFICIENT_QST_AMOUNT_FOR_SALE = 11;
constexpr sint32 QUSINO_INVALID_TRANSFER = 12;
constexpr sint32 QUSINO_INSUFFICIENT_QST = 13;
constexpr sint32 QUSINO_WRONG_ASSET_TYPE = 14;
constexpr sint32 QUSINO_ALREADY_VOTED_WITH_SAME_VOTE = 15;
constexpr sint32 QUSINO_ALREADY_CLAIMED_TODAY = 16;
constexpr sint32 QUSINO_BONUS_CLAIM_TIME_NOT_COME = 17;
constexpr sint32 QUSINO_INSUFFICIENT_BONUS_AMOUNT = 18;
constexpr sint32 QUSINO_INVALID_GAME_PROPOSER = 19;
constexpr sint32 QUSINO_INVALID_INPUT = 20;
constexpr sint32 QUSINO_RNG_NOT_READY = 21;
constexpr sint32 QUSINO_RNG_REFILL_TOO_SOON = 22;
constexpr sint32 QUSINO_RNG_REFILL_FAILED = 23;

constexpr uint8 QUSINO_ASSET_TYPE_QUBIC = 0;
constexpr uint8 QUSINO_ASSET_TYPE_QSC = 1;
constexpr uint8 QUSINO_ASSET_TYPE_STAR = 2;
constexpr uint8 QUSINO_ASSET_TYPE_QST = 3;

constexpr uint32 QUSINO_LOG_SUCCESS = 0;
constexpr uint32 QUSINO_LOG_INSUFFICIENT_FUNDS = 1;
constexpr uint32 QUSINO_LOG_INSUFFICIENT_STAR = 2;
constexpr uint32 QUSINO_LOG_INSUFFICIENT_QSC = 3;
constexpr uint32 QUSINO_LOG_INSUFFICIENT_VOTE_FEE = 4;
constexpr uint32 QUSINO_LOG_WRONG_GAME_URI = 5;
constexpr uint32 QUSINO_LOG_NOT_VOTE_TIME = 6;
constexpr uint32 QUSINO_LOG_ALREADY_VOTED_WITH_SAME_VOTE = 7;
constexpr uint32 QUSINO_LOG_WRONG_ASSET_TYPE = 8;
constexpr uint32 QUSINO_LOG_ALREADY_CLAIMED_TODAY = 9;
constexpr uint32 QUSINO_LOG_BONUS_CLAIM_TIME_NOT_COME = 10;
constexpr uint32 QUSINO_LOG_INSUFFICIENT_BONUS_AMOUNT = 11;
constexpr uint32 QUSINO_LOG_INVALID_GAME_PROPOSER = 12;
constexpr uint32 QUSINO_LOG_INVALID_INPUT = 13;
constexpr uint32 QUSINO_LOG_RNG_NOT_READY = 14;
constexpr uint32 QUSINO_LOG_RNG_REFILL_TOO_SOON = 15;
constexpr uint32 QUSINO_LOG_RNG_REFILL_FAILED = 16;
constexpr uint32 QUSINO_LOG_RNG_REFILL_SUCCESS = 17;
constexpr uint32 QUSINO_LOG_COINFLIP_RESULT = 18;

// ---------------------------------------------------------------------------
// Coin Flip + shared RNG "Result Bank"
//
// Entropy is bought in bulk from RANDOM and cached in a per-game pool backed by a
// shared overflow reserve, so each coinFlip() draw is instant instead of waiting on
// a fresh BuyEntropy call. refillRandomBank() is the only procedure that talks to
// RANDOM -- permissionless but rate-limited. Pool/reserve arrays are sized for
// QUSINO_RNG_MAX_GAMES so future games (Blackjack, Baccarat, ...) can reuse this
// plumbing; only Coin Flip is wired up so far. getRandom() itself is deliberately
// not a public procedure -- only this contract's own game logic can draw from it.
// ---------------------------------------------------------------------------
constexpr uint16 QUSINO_RNG_ENTROPY_BITS = 256;                                  // bits bought from RANDOM per refill
constexpr uint8  QUSINO_RNG_COLLATERAL_TIER = 0;                                 // cheapest / most populated RANDOM tier
constexpr uint64 QUSINO_RNG_ENTROPY_FEE = RANDOM_BITFEE * QUSINO_RNG_ENTROPY_BITS; // paid from bonusAmount (see QUSINO_GAME_BANKROLL_CAP)

constexpr uint32 QUSINO_RNG_MAX_GAMES = 32;                                      // array capacity for future games (~2KB state per slot)
constexpr uint32 QUSINO_RNG_ACTIVE_GAMES = 1;                                    // games actually bootstrapped by refillRandomBank --
                                                                                  // bump as new games launch, never QUSINO_RNG_MAX_GAMES
constexpr uint32 QUSINO_RNG_POOL_SIZE = 256;                                     // pre-drawn values held per game
constexpr uint32 QUSINO_RNG_RESERVE_SIZE = 1024;                                 // shared overflow reserve, refilled in one shot
constexpr uint32 QUSINO_RNG_MIN_REFILL_TICK_GAP = 5;                             // rate-limit for the permissionless refill call

constexpr uint8 QUSINO_GAME_ID_COINFLIP = 0;

// Coin Flip is played with QSC or STAR only, never raw Qu. A QSC bet is redeemed
// for Qu (QUSINO_QSC_PRICE); a win credits new QSC back to the user -- they redeem
// it themselves via redemptionQSCToQubic() -- debiting bonusAmount (QUSINO's Qu
// game bankroll, funded via depositBonus, also what refillRandomBank spends on
// RANDOM fees). A loss tops bonusAmount back up. STAR bets never touch Qu or
// bonusAmount: STAR isn't redeemable for Qubic, so a win mints STAR and a loss
// burns it, like a vote fee.
constexpr uint64 QUSINO_COINFLIP_MIN_BET = 1000000ULL;                          // min bet, in QSC or STAR units
constexpr uint64 QUSINO_COINFLIP_PAYOUT_PERCENT = 196ULL;                       // 1.96x on win == ~2% house edge, placeholder

// bonusAmount is shared by the daily-claim-bonus feature and Coin Flip's Qu
// bankroll, pinned at QUSINO_GAME_BANKROLL_CAP -- anything that would push it past
// the cap (an oversized depositBonus, or a Coin Flip loss) goes to epochRevenue
// instead (see addWithCap()).
constexpr uint64 QUSINO_GAME_BANKROLL_CAP = 1200000000ULL;                      // 1.2B Qu

struct QUSINOLogger
{
    uint32 _contractIndex;
    uint32 _type;
    sint8 _terminator;
};

struct QUSINO2
{
};

struct QUSINO : public ContractBase
{
public:
    struct earnSTAR_input
    {
        uint64 amount;                    // amount of STAR / 100 to earn
    };
    struct earnSTAR_output
    {
        sint32 returnCode;
    };
    struct transferSTAROrQSC_input
    {
        id dest;
        uint64 amount;
        uint8 type;              // QUSINO_ASSET_TYPE_STAR or QUSINO_ASSET_TYPE_QSC
    };
    struct transferSTAROrQSC_output
    {
        sint32 returnCode;
    };
    struct submitGame_input
    {
        Array<uint8, 64> URI;
    };
    struct submitGame_output
    {
        sint32 returnCode;
    };
    struct voteInGameProposal_input
    {
        Array<uint8, 64> URI;
        uint64 gameIndex;
        uint8 yesNo;                 // 1 - yes, 2 - no
    };
    struct voteInGameProposal_output
    {
        sint32 returnCode;
    };

    struct depositBonus_input
    {
        uint64 amount;
    };
    struct depositBonus_output
    {
        sint32 returnCode;
    };

    struct dailyClaimBonus_input
    {
    };
    struct dailyClaimBonus_output
    {
        sint32 returnCode;
    };

    struct redemptionQSCToQubic_input
    {
        uint64 amount;
    };
    struct redemptionQSCToQubic_output
    {
        sint32 returnCode;
    };

    struct refillRandomBank_input
    {
    };
    struct refillRandomBank_output
    {
        sint32 returnCode;
        uint32 valuesAdded;
    };

    struct coinFlip_input
    {
        uint8 guess;              // 0 = heads, 1 = tails
        uint8 assetType;          // QUSINO_ASSET_TYPE_QSC or QUSINO_ASSET_TYPE_STAR -- no other type is valid
        uint64 amount;            // bet size, in units of assetType; no invocationReward is taken
    };
    struct coinFlip_output
    {
        sint32 returnCode;
        uint8 result;              // 0 = heads, 1 = tails
        bit won;
        uint64 payout;             // QSC bets: QSC credited (redeem via redemptionQSCToQubic).
                                   // STAR bets: STAR minted. 0 on a loss.
    };

    struct getRandomBankStatus_input
    {
    };
    struct getRandomBankStatus_output
    {
        bit poolInitialized;
        uint32 reserveFilled;
        uint32 lastRefillTick;
    };

    struct getUserAssetVolume_input
    {
        id user;
    };
    struct getUserAssetVolume_output
    {
        uint64 STARAmount;
        uint64 QSCAmount;
    };

    struct GameInfo
    {
        Array<uint8, 64> URI;
        id proposer;
        uint32 yesVotes;
        uint32 noVotes;
        uint32 proposedEpoch;
    };
    struct getFailedGameList_input
    {
        uint32 offset;
    };
    struct getFailedGameList_output
    {
        Array<GameInfo, 32> games;
    };

    struct getSCInfo_input
    {

    };
    struct getSCInfo_output
    {
        uint64 QSCCirclatingSupply;
        uint64 STARCirclatingSupply;
        uint64 burntSTAR;
        uint64 epochRevenue;
        uint64 maxGameIndex;
        uint64 bonusAmount;
    };

    struct getActiveGameList_input
    {
        uint32 offset;
    };
    struct getActiveGameList_output
    {
        Array<GameInfo, 32> games;
        Array<uint64, 32> gameIndexes;
    };

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
    struct getProposerEarnedQSCInfo_input
    {
        id proposer;
        uint32 epoch;
    };
    struct getProposerEarnedQSCInfo_output
    {
        uint64 earnedQSC;
    };

    struct STARAndQSC
    {
        uint64 volumeOfSTAR;
        uint64 volumeOfQSC;
    };
    struct EarnedQSCInfo
    {
        id proposer;
        uint32 epoch;
        bool operator==(const EarnedQSCInfo& other) const
        {
            return proposer == other.proposer && epoch == other.epoch;
        }
    };
    struct VoteInfo
    {
        id voter;
        uint64 gameIndex;

        bool operator==(const VoteInfo& other) const
        {
            return voter == other.voter && gameIndex == other.gameIndex;
        }
    };
    //----------------------------------------------------------------------------
    // Define state
    struct StateData
    {
        HashMap<id, STARAndQSC, QUSINO_MAX_USERS> userAssetVolume;
        HashMap<uint64, GameInfo, QUSINO_MAX_NUMBER_OF_GAMES> gameList;
        HashMap<uint64, GameInfo, 1024> failedGameList;
        HashMap<VoteInfo, uint8, QUSINO_MAX_USERS * QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER> voteList;
        HashMap<id, uint32, QUSINO_MAX_USERS> userDailyClaimedBonus;
        HashMap<EarnedQSCInfo, uint64, QUSINO_MAX_NUMBER_OF_GAMES> userEarnedQSCInfo;
        id LPDividendsAddress;
        id CCFDividendsAddress;
        id treasuryAddress;
        id QSTIssuer;
        uint64 QSCCirclatingSupply;
        uint64 STARCirclatingSupply;
        uint64 burntSTAR;
        uint64 epochRevenue;
        uint64 maxGameIndex;
        uint64 QSTAssetName;
        uint64 bonusAmount;
        sint64 transferRightsFee;
        uint32 lastClaimedTime;

        // RNG "Result Bank" (see comment above QUSINO_RNG_ENTROPY_BITS)
        Array<uint64, QUSINO_RNG_MAX_GAMES * QUSINO_RNG_POOL_SIZE> rngPools;    // flattened [gameId * QUSINO_RNG_POOL_SIZE + slot]
        Array<uint32, QUSINO_RNG_MAX_GAMES> rngPoolNonce;                       // per-game nonce, folded into index-selection entropy each draw
        Array<uint8, QUSINO_RNG_MAX_GAMES> rngPoolInitialized;                  // 1 once a game's pool has been seeded, else 0
        Array<uint64, QUSINO_RNG_RESERVE_SIZE> rngReserve;
        uint32 rngReserveHead;                                                  // next reserve slot to hand out (circular)
        uint32 rngReserveFilled;                                                // number of valid, unconsumed entries left in the reserve
        uint32 rngLastRefillTick;                                               // for rate-limiting refillRandomBank()
        bit rngBankEverFilled;                                                  // set once the first refill succeeds; lets the tick-gap
                                                                                 // check skip the very first refill
    };
protected:
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
    // Adds toAdd to current, clamped at cap; whatever doesn't fit is reported via
    // overflow instead of wrapping. Used to keep bonusAmount pinned at
    // QUSINO_GAME_BANKROLL_CAP.
    inline static void addWithCap(uint64 current, uint64 toAdd, uint64 cap, uint64& newValue, uint64& overflow)
    {
        uint64 headroom = (cap > current) ? (cap - current) : 0;
        if (toAdd <= headroom)
        {
            newValue = current + toAdd;
            overflow = 0;
        }
        else
        {
            newValue = cap;
            overflow = toAdd - headroom;
        }
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
        * @return pack qusino datetime data from year, month, day, hour, minute, second to a uint32
        * year is counted from 24 (2024)
    */
    inline static void packQusinoDate(uint32 _year, uint32 _month, uint32 _day, uint32 _hour, uint32 _minute, uint32 _second, uint32& res)
    {
        res = ((_year - 24) << 26) | (_month << 22) | (_day << 17) | (_hour << 12) | (_minute << 6) | (_second);
    }
 
    inline static uint32 qusinoGetYear(uint32 data)
    {
        return ((data >> 26) + 24);
    }
    inline static uint32 qusinoGetMonth(uint32 data)
    {
        return ((data >> 22) & 0b1111);
    }
    inline static uint32 qusinoGetDay(uint32 data)
    {
        return ((data >> 17) & 0b11111);
    }
    inline static uint32 qusinoGetHour(uint32 data)
    {
        return ((data >> 12) & 0b11111);
    }
    inline static uint32 qusinoGetMinute(uint32 data)
    {
        return ((data >> 6) & 0b111111);
    }
    inline static uint32 qusinoGetSecond(uint32 data)
    {
        return (data & 0b111111);
    }
    /*
        * @return unpack qusino datetime from uin32 to year, month, day, hour, minute, secon
    */
    inline static void unpackQusinoDate(uint8& _year, uint8& _month, uint8& _day, uint8& _hour, uint8& _minute, uint8& _second, uint32 data)
    {
        _year = qusinoGetYear(data); // 6 bits
        _month = qusinoGetMonth(data); //4bits
        _day = qusinoGetDay(data); //5bits
        _hour = qusinoGetHour(data); //5bits
        _minute = qusinoGetMinute(data); //6bits
        _second = qusinoGetSecond(data); //6bits
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
    inline static void diffQusinoDateInSecond(uint32& A, uint32& B, sint32& i, uint64& dayA, uint64& dayB, uint64& res)
    {
        if (dateCompare(A, B, i) >= 0)
        {
            res = 0;
            return;
        }
        accumulatedDay(qusinoGetMonth(A), dayA);
        dayA += qusinoGetDay(A);
        accumulatedDay(qusinoGetMonth(B), dayB);
        dayB += (qusinoGetYear(B) - qusinoGetYear(A)) * 365ULL + qusinoGetDay(B);

        // handling leap-year: only store last 2 digits of year here, don't care about mod 100 & mod 400 case
        for (i = qusinoGetYear(A); (uint32)(i) < qusinoGetYear(B); i++)
        {
            if (mod(i, 4) == 0)
            {
                dayB++;
            }
        }
        if (mod(sint32(qusinoGetYear(A)), 4) == 0 && (qusinoGetMonth(A) > 2)) dayA++;
        if (mod(sint32(qusinoGetYear(B)), 4) == 0 && (qusinoGetMonth(B) > 2)) dayB++;
        res = (dayB - dayA) * 3600ULL * 24;
        res += (qusinoGetHour(B) * 3600 + qusinoGetMinute(B) * 60 + qusinoGetSecond(B));
        res -= (qusinoGetHour(A) * 3600 + qusinoGetMinute(A) * 60 + qusinoGetSecond(A));
    }
    inline static bool checkValidQusinoDateTime(uint32& A)
    {
        if (qusinoGetMonth(A) > 12) return false;
        if (qusinoGetDay(A) > 31) return false;
        if ((qusinoGetDay(A) == 31) &&
            (qusinoGetMonth(A) != 1) && (qusinoGetMonth(A) != 3) && (qusinoGetMonth(A) != 5) &&
            (qusinoGetMonth(A) != 7) && (qusinoGetMonth(A) != 8) && (qusinoGetMonth(A) != 10) && (qusinoGetMonth(A) != 12)) return false;
        if ((qusinoGetDay(A) == 30) && (qusinoGetMonth(A) == 2)) return false;
        if ((qusinoGetDay(A) == 29) && (qusinoGetMonth(A) == 2) && (mod(qusinoGetYear(A), 4u) != 0)) return false;
        if (qusinoGetHour(A) >= 24) return false;
        if (qusinoGetMinute(A) >= 60) return false;
        if (qusinoGetSecond(A) >= 60) return false;
        return true;
    }

public:
    //----------------------------------------------------------------------------
    // Define user procedures and functions (with input and output)
    struct earnSTAR_locals
    {
        STARAndQSC user;
        QUSINOLogger log;
        uint64 starVolumeIncrease;
        uint64 qubicCost;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(earnSTAR)
    {
        locals.starVolumeIncrease = smul(input.amount, 100ULL);
        locals.qubicCost = smul(locals.starVolumeIncrease, (uint64)QUSINO_STAR_PRICE);
        if (locals.qubicCost > (uint64)qpi.invocationReward())
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QUSINO_INSUFFICIENT_FUNDS;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_FUNDS, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        if (locals.qubicCost < (uint64)qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)locals.qubicCost);
        }
        state.get().userAssetVolume.get(qpi.invocator(), locals.user);
        locals.user.volumeOfSTAR = sadd(locals.user.volumeOfSTAR, locals.starVolumeIncrease);
        locals.user.volumeOfQSC = sadd(locals.user.volumeOfQSC, input.amount);
        state.mut().userAssetVolume.set(qpi.invocator(), locals.user);
        state.mut().STARCirclatingSupply = sadd(state.get().STARCirclatingSupply, locals.starVolumeIncrease);
        state.mut().QSCCirclatingSupply = sadd(state.get().QSCCirclatingSupply, input.amount);
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }
    struct transferSTAROrQSC_locals
    {
        STARAndQSC dest, sender;
        QUSINOLogger log;
        sint64 idx;
        GameInfo game;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(transferSTAROrQSC)
    {
        if (input.type != QUSINO_ASSET_TYPE_STAR && input.type != QUSINO_ASSET_TYPE_QSC)
        {
            output.returnCode = QUSINO_WRONG_ASSET_TYPE;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_WRONG_ASSET_TYPE, 0 };
            LOG_INFO(locals.log);
            return;
        }
        locals.idx = state.get().gameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
        {
            locals.game = state.get().gameList.value(locals.idx);
            if (locals.game.proposer == qpi.invocator())
            {
                output.returnCode = QUSINO_INVALID_GAME_PROPOSER;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INVALID_GAME_PROPOSER, 0 };
                LOG_INFO(locals.log);
                return;
            }
            locals.idx = state.get().gameList.nextElementIndex(locals.idx);
        }
        state.get().userAssetVolume.get(qpi.invocator(), locals.sender);
        state.get().userAssetVolume.get(input.dest, locals.dest);
        if (input.type == QUSINO_ASSET_TYPE_STAR)
        {
            if (locals.sender.volumeOfSTAR < input.amount) 
            {
                output.returnCode = QUSINO_INSUFFICIENT_STAR;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_STAR, 0 };
                LOG_INFO(locals.log);
                return;
            }
            locals.sender.volumeOfSTAR -= input.amount;
            locals.dest.volumeOfSTAR = sadd(locals.dest.volumeOfSTAR, input.amount);
        }
        else if (input.type == QUSINO_ASSET_TYPE_QSC)
        {
            if (locals.sender.volumeOfQSC < input.amount) 
            {
                output.returnCode = QUSINO_INSUFFICIENT_QSC;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_QSC, 0 };
                LOG_INFO(locals.log);
                return;
            }
            locals.sender.volumeOfQSC -= input.amount;
            locals.dest.volumeOfQSC = sadd(locals.dest.volumeOfQSC, input.amount);
        }
        state.mut().userAssetVolume.set(qpi.invocator(), locals.sender);
        state.mut().userAssetVolume.set(input.dest, locals.dest);
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct submitGame_locals
    {
        GameInfo newGame;
        QUSINOLogger log;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(submitGame)
    {
        if (qpi.invocationReward() < QUSINO_GAME_SUBMIT_FEE) 
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QUSINO_INSUFFICIENT_FUNDS;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_FUNDS, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        if (qpi.invocationReward() > QUSINO_GAME_SUBMIT_FEE) 
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QUSINO_GAME_SUBMIT_FEE);   
        }
        qpi.distributeDividends(div(QUSINO_GAME_SUBMIT_FEE, 676 * 10ULL));
        state.mut().epochRevenue = sadd(state.get().epochRevenue, QUSINO_GAME_SUBMIT_FEE - div(QUSINO_GAME_SUBMIT_FEE, 676 * 10ULL) * 676);
        locals.newGame.proposedEpoch = qpi.epoch();
        locals.newGame.proposer = qpi.invocator();
        copyMemory(locals.newGame.URI, input.URI);
        locals.newGame.yesVotes = 0;
        locals.newGame.noVotes = 0;
        state.mut().gameList.set(state.mut().maxGameIndex, locals.newGame);
        state.mut().maxGameIndex++;
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct voteInGameProposal_locals
    {
        STARAndQSC userVolume;
        VoteInfo voteInfo;
        GameInfo game;
        uint32 i;
        uint8 voteStatus;
        QUSINOLogger log;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(voteInGameProposal)
    {
        if (qpi.invocationReward() > 0) 
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        state.get().userAssetVolume.get(qpi.invocator(), locals.userVolume);
        if (locals.userVolume.volumeOfSTAR < QUSINO_VOTE_FEE) 
        {
            output.returnCode = QUSINO_INSUFFICIENT_VOTE_FEE;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_VOTE_FEE, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        state.get().gameList.get(input.gameIndex, locals.game);
        if (locals.game.proposedEpoch != qpi.epoch() && locals.game.proposedEpoch + QUSINO_REVOTE_DURATION != qpi.epoch()) 
        {
            output.returnCode = QUSINO_NOT_VOTE_TIME;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_NOT_VOTE_TIME, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        for (locals.i = 0; locals.i < 64; locals.i++) 
        {
            if (locals.game.URI.get(locals.i) != input.URI.get(locals.i)) 
            {
                output.returnCode = QUSINO_WRONG_GAME_URI_FOR_VOTE;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_WRONG_GAME_URI, 0 };
                LOG_INFO(locals.log);
                return ;
            }
        }
        locals.voteInfo.voter = qpi.invocator();
        locals.voteInfo.gameIndex = input.gameIndex;
        state.get().voteList.get(locals.voteInfo, locals.voteStatus);
        if (locals.voteStatus && input.yesNo == locals.voteStatus)
        {
            output.returnCode = QUSINO_ALREADY_VOTED_WITH_SAME_VOTE;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_ALREADY_VOTED_WITH_SAME_VOTE, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        if (locals.voteStatus)
        {
            if (input.yesNo == 1)
            {
                locals.game.yesVotes++;
                locals.game.noVotes--;
            }
            else if (input.yesNo == 2)
            {
                locals.game.yesVotes--;
                locals.game.noVotes++;
            }
        }
        else
        {
            if (input.yesNo == 1)
            {
                locals.game.yesVotes++;
            }
            else if (input.yesNo == 2)
            {
                locals.game.noVotes++;
            }
        }
        locals.userVolume.volumeOfSTAR -= QUSINO_VOTE_FEE;
        state.mut().burntSTAR += QUSINO_VOTE_FEE;
        state.mut().STARCirclatingSupply -= QUSINO_VOTE_FEE;
        state.mut().userAssetVolume.set(qpi.invocator(), locals.userVolume);

        locals.voteStatus = input.yesNo;
        state.mut().gameList.set(input.gameIndex, locals.game);
        state.mut().voteList.set(locals.voteInfo, locals.voteStatus);
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct depositBonus_locals
    {
        QUSINOLogger log;
        uint64 newBonus;
        uint64 overflow;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(depositBonus)
    {
        if (qpi.invocationReward() > 0)
        {
            // bonusAmount is capped at QUSINO_GAME_BANKROLL_CAP; excess goes to epochRevenue.
            addWithCap(state.get().bonusAmount, (uint64)qpi.invocationReward(), QUSINO_GAME_BANKROLL_CAP, locals.newBonus, locals.overflow);
            state.mut().bonusAmount = locals.newBonus;
            if (locals.overflow > 0)
            {
                state.mut().epochRevenue = sadd(state.get().epochRevenue, locals.overflow);
            }
        }
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct dailyClaimBonus_locals
    {
        STARAndQSC userVolume;
        uint32 lastClaimedTime;
        uint32 curDate;
        sint32 i;
        uint64 diffTime, dayA, dayB;
        QUSINOLogger log;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(dailyClaimBonus)
    {
        packQusinoDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        state.get().userDailyClaimedBonus.get(qpi.invocator(), locals.lastClaimedTime);
        diffQusinoDateInSecond(locals.lastClaimedTime, locals.curDate, locals.i, locals.dayA, locals.dayB, locals.diffTime);
        if (locals.lastClaimedTime && locals.diffTime < QUSINO_DAILY_CLAIM_BONUS_DURATION)
        {
            output.returnCode = QUSINO_ALREADY_CLAIMED_TODAY;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_ALREADY_CLAIMED_TODAY, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        locals.lastClaimedTime = state.get().lastClaimedTime;
        diffQusinoDateInSecond(locals.lastClaimedTime, locals.curDate, locals.i, locals.dayA, locals.dayB, locals.diffTime);
        if (locals.lastClaimedTime && locals.diffTime < QUSINO_BONUS_CLAIM_DURATION)
        {
            output.returnCode = QUSINO_BONUS_CLAIM_TIME_NOT_COME;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_BONUS_CLAIM_TIME_NOT_COME, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        if (state.get().bonusAmount < QUSINO_BONUS_CLAIM_AMOUNT)
        {
            output.returnCode = QUSINO_INSUFFICIENT_BONUS_AMOUNT;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_BONUS_AMOUNT, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        state.mut().bonusAmount -= QUSINO_BONUS_CLAIM_AMOUNT;
        state.get().userAssetVolume.get(qpi.invocator(), locals.userVolume);
        locals.userVolume.volumeOfSTAR = sadd(locals.userVolume.volumeOfSTAR, QUSINO_BONUS_CLAIM_AMOUNT_STAR);
        locals.userVolume.volumeOfQSC = sadd(locals.userVolume.volumeOfQSC, QUSINO_BONUS_CLAIM_AMOUNT_QSC);
        state.mut().userAssetVolume.set(qpi.invocator(), locals.userVolume);
        state.mut().STARCirclatingSupply = sadd(state.get().STARCirclatingSupply, QUSINO_BONUS_CLAIM_AMOUNT_STAR);
        state.mut().QSCCirclatingSupply = sadd(state.get().QSCCirclatingSupply, QUSINO_BONUS_CLAIM_AMOUNT_QSC);
        state.mut().lastClaimedTime = locals.curDate;
        state.mut().userDailyClaimedBonus.set(qpi.invocator(), locals.curDate);
        state.mut().epochRevenue = sadd(state.get().epochRevenue, QUSINO_BONUS_CLAIM_AMOUNT_STAR);      // 100STAR is 100Qubic for each bonus claim
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct redemptionQSCToQubic_locals
    {
        STARAndQSC userVolume;
        QUSINOLogger log;
        sint64 idx;
        GameInfo game;
        uint64 qubicPayout;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(redemptionQSCToQubic)
    {
        locals.idx = state.get().gameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
        {
            locals.game = state.get().gameList.value(locals.idx);
            if (locals.game.proposer == qpi.invocator())
            {
                output.returnCode = QUSINO_INVALID_GAME_PROPOSER;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INVALID_GAME_PROPOSER, 0 };
                LOG_INFO(locals.log);
                return;
            }
            locals.idx = state.get().gameList.nextElementIndex(locals.idx);
        }
        state.get().userAssetVolume.get(qpi.invocator(), locals.userVolume);
        if (locals.userVolume.volumeOfQSC < input.amount)
        {
            output.returnCode = QUSINO_INSUFFICIENT_QSC;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_QSC, 0 };
            LOG_INFO(locals.log);
            return;
        }
        locals.qubicPayout = smul(input.amount, QUSINO_QSC_PRICE);
        if (locals.qubicPayout > (uint64)INT64_MAX)
        {
            output.returnCode = QUSINO_INSUFFICIENT_FUNDS;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_FUNDS, 0 };
            LOG_INFO(locals.log);
            return;
        }
        if (qpi.transfer(qpi.invocator(), (sint64)locals.qubicPayout) < 0)
        {
            output.returnCode = QUSINO_INSUFFICIENT_FUNDS;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_FUNDS, 0 };
            LOG_INFO(locals.log);
            return;
        }
        locals.userVolume.volumeOfQSC -= input.amount;
        state.mut().userAssetVolume.set(qpi.invocator(), locals.userVolume);
        state.mut().QSCCirclatingSupply -= input.amount;
        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }
    // refillRandomBank
    // ---------------------------------------------------------------------------
    // Permissionless call that tops up the RNG reserve by buying entropy from
    // RANDOM, paid from bonusAmount (refuses if it can't cover the fee). Refuses
    // while the reserve still has unspent values (would waste the fee); once
    // drained, rate-limited to one refill per QUSINO_RNG_MIN_REFILL_TICK_GAP ticks
    // (first-ever refill exempt). Also bootstraps any active game's pool that
    // hasn't been seeded yet.
    // Return codes: QUSINO_SUCCESS, QUSINO_RNG_REFILL_TOO_SOON,
    // QUSINO_INSUFFICIENT_BONUS_AMOUNT, QUSINO_RNG_REFILL_FAILED.
    // ---------------------------------------------------------------------------
    struct refillRandomBank_locals
    {
        RANDOM::BuyEntropy_input buyEntropyInput;
        RANDOM::BuyEntropy_output buyEntropyOutput;
        m256i baseSeed;
        m256i expanded;
        uint64 seedIdx;
        uint32 g;
        uint32 slot;
        QUSINOLogger log;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(refillRandomBank)
    {
        // Takes no payment from the caller -- QUSINO funds the RANDOM purchase itself.
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        // Don't buy while the reserve still has unspent values (would waste the fee).
        // Once empty, rate-limit successful refills too, except the very first ever.
        if (state.get().rngReserveFilled > 0
            || (state.get().rngBankEverFilled && qpi.tick() < state.get().rngLastRefillTick + QUSINO_RNG_MIN_REFILL_TICK_GAP))
        {
            output.returnCode = QUSINO_RNG_REFILL_TOO_SOON;
            output.valuesAdded = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_RNG_REFILL_TOO_SOON, 0 };
            LOG_INFO(locals.log);
            return;
        }

        // Don't even attempt a purchase the game bankroll can't afford.
        if (state.get().bonusAmount < QUSINO_RNG_ENTROPY_FEE)
        {
            output.returnCode = QUSINO_INSUFFICIENT_BONUS_AMOUNT;
            output.valuesAdded = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_BONUS_AMOUNT, 0 };
            LOG_INFO(locals.log);
            return;
        }

        locals.buyEntropyInput.collateralTier = QUSINO_RNG_COLLATERAL_TIER;
        locals.buyEntropyInput.numberOfBits = QUSINO_RNG_ENTROPY_BITS;
        locals.buyEntropyInput.trustee = id::zero();
        INVOKE_OTHER_CONTRACT_PROCEDURE(RANDOM, BuyEntropy, locals.buyEntropyInput, locals.buyEntropyOutput, QUSINO_RNG_ENTROPY_FEE);

        if (interContractCallError != NoCallError || locals.buyEntropyOutput.entropy == BIT4096_ZERO)
        {
            // No entropy available this round -- try again on a later tick.
            output.returnCode = QUSINO_RNG_REFILL_FAILED;
            output.valuesAdded = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_RNG_REFILL_FAILED, 0 };
            LOG_INFO(locals.log);
            return;
        }

        // Collapse entropy into a seed, then derive many values from it by re-hashing
        // with an incrementing counter (same technique QRaffle uses).
        locals.baseSeed = qpi.K12(locals.buyEntropyOutput.entropy);
        for (locals.seedIdx = 0; locals.seedIdx < QUSINO_RNG_RESERVE_SIZE; locals.seedIdx++)
        {
            locals.expanded = qpi.K12(m256i(locals.baseSeed.u64._0, locals.baseSeed.u64._1, locals.baseSeed.u64._2, locals.baseSeed.u64._3 ^ (locals.seedIdx + 1ULL)));
            state.mut().rngReserve.set(locals.seedIdx, locals.expanded.u64._0);
        }
        state.mut().rngReserveHead = 0;
        state.mut().rngReserveFilled = QUSINO_RNG_RESERVE_SIZE;
        state.mut().rngLastRefillTick = qpi.tick();
        state.mut().rngBankEverFilled = 1;
        // Debit the fee actually spent (not done on the failure path -- RANDOM refunds
        // QUSINO in full when it has no entropy to sell).
        state.mut().bonusAmount -= QUSINO_RNG_ENTROPY_FEE;

        // Bootstrap any active game's pool that hasn't been seeded yet, straight out of
        // the reserve just filled. Bounded by ACTIVE_GAMES, not MAX_GAMES, so we don't
        // burn the reserve priming pools nothing uses yet.
        for (locals.g = 0; locals.g < QUSINO_RNG_ACTIVE_GAMES; locals.g++)
        {
            if (state.get().rngPoolInitialized.get(locals.g) == 0 && state.get().rngReserveFilled >= QUSINO_RNG_POOL_SIZE)
            {
                for (locals.slot = 0; locals.slot < QUSINO_RNG_POOL_SIZE; locals.slot++)
                {
                    state.mut().rngPools.set((uint64)locals.g * QUSINO_RNG_POOL_SIZE + locals.slot, state.get().rngReserve.get(state.get().rngReserveHead));
                    state.mut().rngReserveHead = mod<uint32>(state.get().rngReserveHead + 1, QUSINO_RNG_RESERVE_SIZE);
                    state.mut().rngReserveFilled--;
                }
                state.mut().rngPoolInitialized.set(locals.g, 1);
            }
        }

        output.returnCode = QUSINO_SUCCESS;
        output.valuesAdded = QUSINO_RNG_RESERVE_SIZE;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_RNG_REFILL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    // coinFlip
    // ---------------------------------------------------------------------------
    // Bets QSC or STAR (input.assetType) on heads/tails (input.guess); never raw
    // Qu, no invocationReward taken. The wager always leaves the caller's balance
    // up front. QSC: redeemed for Qu (QUSINO_QSC_PRICE); a win credits new QSC
    // back to the caller -- redeem via redemptionQSCToQubic() -- debiting
    // bonusAmount by that QSC's Qu backing; a loss tops bonusAmount back up
    // (capped, overflow to epochRevenue). Rejected up front unless bonusAmount can
    // cover the win. STAR: never touches Qu/bonusAmount -- a win mints STAR, a
    // loss burns it (like a vote fee). Outcome is drawn instantly from the Coin
    // Flip RNG pool (see Result Bank comment above), then the slot is topped up.
    // Return codes: QUSINO_SUCCESS, QUSINO_INVALID_INPUT, QUSINO_WRONG_ASSET_TYPE,
    // QUSINO_INSUFFICIENT_FUNDS, QUSINO_RNG_NOT_READY, QUSINO_INSUFFICIENT_QSC /
    // QUSINO_INSUFFICIENT_STAR, QUSINO_INSUFFICIENT_BONUS_AMOUNT.
    // ---------------------------------------------------------------------------
    struct CoinFlipSelectContext
    {
        m256i prevDigest;
        id invocator;
        uint32 tick;
        uint32 nonce;
    };
    struct CoinFlipOutcomeContext
    {
        uint64 poolValue;
        m256i selectHash;
    };
    struct coinFlip_locals
    {
        CoinFlipSelectContext selectCtx;
        m256i selectHash;
        CoinFlipOutcomeContext outcomeCtx;
        m256i outcomeHash;
        STARAndQSC userVolume;
        uint64 index;
        uint64 poolValue;
        uint64 qscRedemptionValueQu;
        uint64 winAmount;
        uint64 qscPayout;
        uint64 newBonus;
        uint64 overflow;
        uint8 outcome;
        QUSINOLogger log;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(coinFlip)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (input.guess > 1)
        {
            output.returnCode = QUSINO_INVALID_INPUT;
            output.result = 0;
            output.won = 0;
            output.payout = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INVALID_INPUT, 0 };
            LOG_INFO(locals.log);
            return;
        }

        if (input.assetType != QUSINO_ASSET_TYPE_QSC && input.assetType != QUSINO_ASSET_TYPE_STAR)
        {
            output.returnCode = QUSINO_WRONG_ASSET_TYPE;
            output.result = 0;
            output.won = 0;
            output.payout = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_WRONG_ASSET_TYPE, 0 };
            LOG_INFO(locals.log);
            return;
        }

        if (input.amount < QUSINO_COINFLIP_MIN_BET)
        {
            output.returnCode = QUSINO_INSUFFICIENT_FUNDS;
            output.result = 0;
            output.won = 0;
            output.payout = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_FUNDS, 0 };
            LOG_INFO(locals.log);
            return;
        }

        if (state.get().rngPoolInitialized.get(QUSINO_GAME_ID_COINFLIP) == 0)
        {
            // Bank not primed yet -- caller should trigger refillRandomBank() and retry.
            output.returnCode = QUSINO_RNG_NOT_READY;
            output.result = 0;
            output.won = 0;
            output.payout = 0;
            locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_RNG_NOT_READY, 0 };
            LOG_INFO(locals.log);
            return;
        }

        state.get().userAssetVolume.get(qpi.invocator(), locals.userVolume);

        if (input.assetType == QUSINO_ASSET_TYPE_QSC)
        {
            if (locals.userVolume.volumeOfQSC < input.amount)
            {
                output.returnCode = QUSINO_INSUFFICIENT_QSC;
                output.result = 0;
                output.won = 0;
                output.payout = 0;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_QSC, 0 };
                LOG_INFO(locals.log);
                return;
            }

            // Gate on the win payout (not just the bet) so a win can't underflow bonusAmount.
            locals.qscRedemptionValueQu = smul(input.amount, QUSINO_QSC_PRICE);
            locals.winAmount = div(smul(locals.qscRedemptionValueQu, QUSINO_COINFLIP_PAYOUT_PERCENT), 100ULL);
            if (state.get().bonusAmount < locals.winAmount)
            {
                output.returnCode = QUSINO_INSUFFICIENT_BONUS_AMOUNT;
                output.result = 0;
                output.won = 0;
                output.payout = 0;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_BONUS_AMOUNT, 0 };
                LOG_INFO(locals.log);
                return;
            }
        }
        else // QUSINO_ASSET_TYPE_STAR
        {
            if (locals.userVolume.volumeOfSTAR < input.amount)
            {
                output.returnCode = QUSINO_INSUFFICIENT_STAR;
                output.result = 0;
                output.won = 0;
                output.payout = 0;
                locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_INSUFFICIENT_STAR, 0 };
                LOG_INFO(locals.log);
                return;
            }
            locals.winAmount = div(smul(input.amount, QUSINO_COINFLIP_PAYOUT_PERCENT), 100ULL);
        }

        // Pick a pool slot from context unique to this call, so it can't be predicted.
        locals.selectCtx.prevDigest = qpi.getPrevSpectrumDigest();
        locals.selectCtx.invocator = qpi.invocator();
        locals.selectCtx.tick = qpi.tick();
        locals.selectCtx.nonce = state.get().rngPoolNonce.get(QUSINO_GAME_ID_COINFLIP);
        locals.selectHash = qpi.K12(locals.selectCtx);
        locals.index = mod<uint64>(locals.selectHash.u64._0, (uint64)QUSINO_RNG_POOL_SIZE);

        locals.poolValue = state.get().rngPools.get((uint64)QUSINO_GAME_ID_COINFLIP * QUSINO_RNG_POOL_SIZE + locals.index);

        // Consume the slot and refill it from the reserve so it's never handed out twice.
        if (state.get().rngReserveFilled > 0)
        {
            state.mut().rngPools.set((uint64)QUSINO_GAME_ID_COINFLIP * QUSINO_RNG_POOL_SIZE + locals.index, state.get().rngReserve.get(state.get().rngReserveHead));
            state.mut().rngReserveHead = mod<uint32>(state.get().rngReserveHead + 1, QUSINO_RNG_RESERVE_SIZE);
            state.mut().rngReserveFilled--;
        }
        state.mut().rngPoolNonce.set(QUSINO_GAME_ID_COINFLIP, state.get().rngPoolNonce.get(QUSINO_GAME_ID_COINFLIP) + 1);

        // Re-hash the drawn value with the selection hash so the outcome stays unpredictable.
        locals.outcomeCtx.poolValue = locals.poolValue;
        locals.outcomeCtx.selectHash = locals.selectHash;
        locals.outcomeHash = qpi.K12(locals.outcomeCtx);
        locals.outcome = (uint8)(locals.outcomeHash.u64._0 & 1);

        output.result = locals.outcome;
        output.won = (locals.outcome == input.guess) ? 1 : 0;

        if (input.assetType == QUSINO_ASSET_TYPE_QSC)
        {
            // The wager always leaves QSC circulation up front, redeemed either way.
            locals.userVolume.volumeOfQSC -= input.amount;
            state.mut().QSCCirclatingSupply -= input.amount;

            if (output.won)
            {
                // Credit QSC instead of sending Qu -- caller redeems it themselves later.
                // Debit the bankroll by exactly the Qu backing the credited QSC.
                locals.qscPayout = div(locals.winAmount, QUSINO_QSC_PRICE);
                output.payout = locals.qscPayout;
                locals.userVolume.volumeOfQSC = sadd(locals.userVolume.volumeOfQSC, locals.qscPayout);
                state.mut().QSCCirclatingSupply = sadd(state.get().QSCCirclatingSupply, locals.qscPayout);
                state.mut().bonusAmount -= smul(locals.qscPayout, QUSINO_QSC_PRICE);
            }
            else
            {
                // The wager's Qu value tops up the bankroll; overflow goes to epochRevenue.
                output.payout = 0;
                addWithCap(state.get().bonusAmount, locals.qscRedemptionValueQu, QUSINO_GAME_BANKROLL_CAP, locals.newBonus, locals.overflow);
                state.mut().bonusAmount = locals.newBonus;
                if (locals.overflow > 0)
                {
                    state.mut().epochRevenue = sadd(state.get().epochRevenue, locals.overflow);
                }
            }
            state.mut().userAssetVolume.set(qpi.invocator(), locals.userVolume);
        }
        else // QUSINO_ASSET_TYPE_STAR
        {
            // The wager always leaves the caller's STAR balance up front; a win mints
            // the payout back on top, a loss is recorded as burnt (like a vote fee).
            locals.userVolume.volumeOfSTAR -= input.amount;
            state.mut().STARCirclatingSupply -= input.amount;

            if (output.won)
            {
                output.payout = locals.winAmount;
                locals.userVolume.volumeOfSTAR = sadd(locals.userVolume.volumeOfSTAR, locals.winAmount);
                state.mut().STARCirclatingSupply = sadd(state.get().STARCirclatingSupply, locals.winAmount);
            }
            else
            {
                output.payout = 0;
                state.mut().burntSTAR = sadd(state.get().burntSTAR, input.amount);
            }
            state.mut().userAssetVolume.set(qpi.invocator(), locals.userVolume);
        }

        output.returnCode = QUSINO_SUCCESS;
        locals.log = QUSINOLogger{ CONTRACT_INDEX, QUSINO_LOG_COINFLIP_RESULT, 0 };
        LOG_INFO(locals.log);
    }

    PUBLIC_FUNCTION(getRandomBankStatus)
    {
        output.poolInitialized = (state.get().rngPoolInitialized.get(QUSINO_GAME_ID_COINFLIP) != 0);
        output.reserveFilled = state.get().rngReserveFilled;
        output.lastRefillTick = state.get().rngLastRefillTick;
    }

    struct getUserAssetVolume_locals
    {
        STARAndQSC userAsset;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(getUserAssetVolume)
    {
        state.get().userAssetVolume.get(input.user, locals.userAsset);
        output.QSCAmount = locals.userAsset.volumeOfQSC;
        output.STARAmount = locals.userAsset.volumeOfSTAR;
    }

    struct getFailedGameList_locals
    {
        GameInfo game;
        sint64 idx;
        uint32 cur;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(getFailedGameList)
    {
        if (input.offset > 1024u - 33u)
        {
            return ;
        }
        locals.cur = 0;
        locals.idx = state.get().failedGameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
            if (locals.cur >= input.offset) 
            {
                if (locals.cur >= input.offset + 32) 
                {
                    return ;
                }
                locals.game = state.get().failedGameList.value(locals.idx);
                output.games.set(locals.cur - input.offset, locals.game);
            }
            locals.cur++;
            locals.idx = state.get().failedGameList.nextElementIndex(locals.idx);
		}
    }

    PUBLIC_FUNCTION(getSCInfo)
    {
        output.QSCCirclatingSupply = state.get().QSCCirclatingSupply;
        output.STARCirclatingSupply = state.get().STARCirclatingSupply;
        output.burntSTAR = state.get().burntSTAR;
        output.epochRevenue = state.get().epochRevenue;
        output.maxGameIndex = state.get().maxGameIndex;
        output.bonusAmount = state.get().bonusAmount;
    }

    struct getActiveGameList_locals
    {
        GameInfo game;
        sint64 idx;
        sint32 cur;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(getActiveGameList)
    {
        if (input.offset > QUSINO_MAX_NUMBER_OF_GAMES - 33u)
        {
            return ;
        }
        locals.cur = 0;
        locals.idx = state.get().gameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
            if (locals.cur >= (sint32)input.offset) 
            {
                if (locals.cur >= (sint32)(input.offset + 32)) 
                {
                    return ;
                }
                locals.game = state.get().gameList.value(locals.idx);
                output.games.set(locals.cur - input.offset, locals.game);
                output.gameIndexes.set(locals.cur - input.offset, state.get().gameList.key(locals.idx));
            }
            locals.cur++;
            locals.idx = state.get().gameList.nextElementIndex(locals.idx);
		}
    }

    PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < state.get().transferRightsFee)
		{
			return ;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
		}
		else
		{
			if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, state.get().transferRightsFee) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
				if (qpi.invocationReward() > state.get().transferRightsFee)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.get().transferRightsFee);
				}
			}
		}
	}

    struct getProposerEarnedQSCInfo_locals
    {
        EarnedQSCInfo earnedQSCInfo;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getProposerEarnedQSCInfo)
    {
        locals.earnedQSCInfo.proposer = input.proposer;
        locals.earnedQSCInfo.epoch = input.epoch;
        state.get().userEarnedQSCInfo.get(locals.earnedQSCInfo, output.earnedQSC);
    }

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
        REGISTER_USER_FUNCTION(getUserAssetVolume, 1);
        REGISTER_USER_FUNCTION(getFailedGameList, 2);
        REGISTER_USER_FUNCTION(getSCInfo, 3);
        REGISTER_USER_FUNCTION(getActiveGameList, 4);
        REGISTER_USER_FUNCTION(getProposerEarnedQSCInfo, 5);
        REGISTER_USER_FUNCTION(getRandomBankStatus, 6);

        REGISTER_USER_PROCEDURE(earnSTAR, 1);
        REGISTER_USER_PROCEDURE(transferSTAROrQSC, 2);
        REGISTER_USER_PROCEDURE(submitGame, 3);
        REGISTER_USER_PROCEDURE(voteInGameProposal, 4);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 5);
        REGISTER_USER_PROCEDURE(depositBonus, 6);
        REGISTER_USER_PROCEDURE(dailyClaimBonus, 7);
        REGISTER_USER_PROCEDURE(redemptionQSCToQubic, 8);
        REGISTER_USER_PROCEDURE(refillRandomBank, 9);
        REGISTER_USER_PROCEDURE(coinFlip, 10);
	}

	INITIALIZE()
	{
        state.mut().maxGameIndex = 1;
        state.mut().transferRightsFee = 100;
        state.mut().LPDividendsAddress = ID(_V, _D, _I, _H, _Y, _F, _G, _B, _J, _Z, _P, _V, _V, _F, _O, _R, _Y, _Q, _V, _O, _I, _D, _U, _P, _S, _I, _H, _C, _B, _D, _K, _B, _K, _Y, _J, _V, _X, _L, _P, _Q, _W, _D, _A, _K, _L, _D, _M, _K, _A, _G, _G, _P, _O, _C, _Y, _G);
        state.mut().CCFDividendsAddress = id(CCF_CONTRACT_INDEX, 0, 0, 0);
        state.mut().treasuryAddress = ID(_B, _Z, _X, _I, _A, _E, _X, _W, _R, _S, _X, _M, _C, _A, _W, _A, _N, _G, _V, _Y, _T, _W, _D, _A, _U, _E, _I, _A, _D, _F, _N, _O, _F, _C, _K, _G, _X, _V, _Q, _M, _P, _C, _K, _U, _H, _S, _M, _L, _F, _E, _E, _B, _E, _P, _C, _C);
        state.mut().QSTAssetName = 5526353;
        state.mut().QSTIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
    }

    struct END_EPOCH_locals
    {
        STARAndQSC userVolume;
        GameInfo game;
        uint64 QSTDividends;
        sint64 idx;
        AssetPossessionIterator iter;
        Asset QSTAsset;
        EarnedQSCInfo earnedQSCInfo;
        uint64 epochSnapshot;
        uint64 grossQubicFromQsc;
        uint64 qscToEpochRevenue;
        uint64 lpShare;
        uint64 ccfShare;
        uint64 treasuryShare;
        uint64 shareholders676Part;
        uint64 qstPerShareRate;
        sint64 possessionCount;
        uint64 qstPayout;
        uint64 proposerQubic;
    };
	END_EPOCH_WITH_LOCALS()
	{
        state.mut().failedGameList.reset();
        locals.idx = state.get().gameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
        {
            locals.game = state.get().gameList.value(locals.idx);
            if (locals.game.noVotes >= locals.game.yesVotes) 
            {
                if (locals.game.proposedEpoch == qpi.epoch() || locals.game.proposedEpoch + QUSINO_REVOTE_DURATION == qpi.epoch()) 
                {
                    state.mut().failedGameList.set(state.get().gameList.key(locals.idx), locals.game);
                    state.mut().gameList.removeByIndex(locals.idx);
                    locals.idx = state.get().gameList.nextElementIndex(locals.idx);
                    continue;
                }
            }
            // distribute QSC to the proposer
            state.get().userAssetVolume.get(locals.game.proposer, locals.userVolume);
            locals.grossQubicFromQsc = smul(locals.userVolume.volumeOfQSC, QUSINO_QSC_PRICE);
            locals.qscToEpochRevenue = div<uint64>(smul(locals.grossQubicFromQsc, (uint64)(1000 - QUSINO_DEVELOPER_FEE)), 1000ULL);
            state.mut().epochRevenue = sadd(state.get().epochRevenue, locals.qscToEpochRevenue);
            locals.proposerQubic = 0;
            if (locals.grossQubicFromQsc >= locals.qscToEpochRevenue)
            {
                locals.proposerQubic = locals.grossQubicFromQsc - locals.qscToEpochRevenue;
            }
            if (locals.proposerQubic <= (uint64)INT64_MAX)
            {
                qpi.transfer(locals.game.proposer, (sint64)locals.proposerQubic);
            }
            state.mut().QSCCirclatingSupply -= locals.userVolume.volumeOfQSC;

            // add earned QSC to userEarnedQSCInfo
            locals.earnedQSCInfo.proposer = locals.game.proposer;
            locals.earnedQSCInfo.epoch = qpi.epoch();
            state.mut().userEarnedQSCInfo.set(locals.earnedQSCInfo, locals.userVolume.volumeOfQSC);

            // set userVolume to 0
            locals.userVolume.volumeOfQSC = 0;
            state.mut().userAssetVolume.set(locals.game.proposer, locals.userVolume);

            // remove game from gameList
            state.mut().gameList.removeByIndex(locals.idx);
            locals.idx = state.get().gameList.nextElementIndex(locals.idx);
        }
        state.mut().gameList.cleanupIfNeeded();
        state.mut().voteList.reset();

        locals.idx = state.get().userAssetVolume.nextElementIndex(NULL_INDEX);
        while (locals.idx != NULL_INDEX)
        {
            locals.userVolume = state.get().userAssetVolume.value(locals.idx);
            if (locals.userVolume.volumeOfSTAR  == 0 && locals.userVolume.volumeOfQSC == 0)
            {
                state.mut().userAssetVolume.removeByIndex(locals.idx);
            }
            locals.idx = state.get().userAssetVolume.nextElementIndex(locals.idx);
        }
        state.mut().userAssetVolume.cleanupIfNeeded();

        locals.epochSnapshot = state.get().epochRevenue;
        locals.lpShare = div(smul(smul(locals.epochSnapshot, (uint64)QUSINO_LP_DIVIDENDS_PERCENT), 1ULL), 100ULL);
        locals.ccfShare = div(smul(smul(locals.epochSnapshot, (uint64)QUSINO_CCF_DIVIDENDS_PERCENT), 1ULL), 100ULL);
        locals.treasuryShare = div(smul(smul(locals.epochSnapshot, (uint64)QUSINO_TREASURY_DIVIDENDS_PERCENT), 1ULL), 100ULL);
        locals.shareholders676Part = smul(
            div(smul(smul(locals.epochSnapshot, (uint64)QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT), 1ULL), 67600ULL),
            676ULL);

        qpi.transfer(state.get().LPDividendsAddress, (sint64)locals.lpShare);
        qpi.transfer(state.get().CCFDividendsAddress, (sint64)locals.ccfShare);
        qpi.transfer(state.get().treasuryAddress, (sint64)locals.treasuryShare);
        qpi.distributeDividends(div(smul(smul(locals.epochSnapshot, (uint64)QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT), 1ULL), 67600ULL));
        locals.QSTDividends = 0;
        locals.qstPerShareRate = div<uint64>(smul(smul(locals.epochSnapshot, (uint64)QUSINO_QST_HOLDERS_DIVIDENDS_PERCENT), 1ULL), QUSINO_SUPPLY_OF_QST * 1000ULL);
        locals.QSTAsset.assetName = state.get().QSTAssetName;
        locals.QSTAsset.issuer = state.get().QSTIssuer;
        locals.iter.begin(locals.QSTAsset);
        while (!locals.iter.reachedEnd())
        {
            locals.possessionCount = locals.iter.numberOfPossessedShares();
            if (locals.possessionCount > 0)
            {
                locals.qstPayout = smul(locals.qstPerShareRate, (uint64)locals.possessionCount);
                locals.QSTDividends = sadd(locals.QSTDividends, locals.qstPayout);
                if (locals.qstPayout <= (uint64)INT64_MAX)
                {
                    qpi.transfer(locals.iter.possessor(), (sint64)locals.qstPayout);
                }
            }
            locals.iter.next();
        }
        state.mut().epochRevenue -= sadd(sadd(sadd(sadd(locals.lpShare, locals.ccfShare), locals.treasuryShare), locals.shareholders676Part), locals.QSTDividends);
	}

    PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
