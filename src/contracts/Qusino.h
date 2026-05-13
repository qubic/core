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
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(earnSTAR)
    {
        if (input.amount * QUSINO_STAR_PRICE * 100 > (uint32)qpi.invocationReward()) 
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
        if (input.amount * QUSINO_STAR_PRICE * 100 < (uint32)qpi.invocationReward()) 
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (input.amount * QUSINO_STAR_PRICE * 100));
        }
        state.get().userAssetVolume.get(qpi.invocator(), locals.user);
        locals.user.volumeOfSTAR += input.amount * 100;
        locals.user.volumeOfQSC += input.amount;
        state.mut().userAssetVolume.set(qpi.invocator(), locals.user);
        state.mut().STARCirclatingSupply += input.amount * 100;
        state.mut().QSCCirclatingSupply += input.amount;
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
            locals.dest.volumeOfSTAR += input.amount;
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
            locals.dest.volumeOfQSC += input.amount;
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
        state.mut().epochRevenue += QUSINO_GAME_SUBMIT_FEE - div(QUSINO_GAME_SUBMIT_FEE, 676 * 10ULL) * 676;
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
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(depositBonus)
    {
        state.mut().bonusAmount += qpi.invocationReward();
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
        locals.userVolume.volumeOfSTAR += QUSINO_BONUS_CLAIM_AMOUNT_STAR;
        locals.userVolume.volumeOfQSC += QUSINO_BONUS_CLAIM_AMOUNT_QSC;
        state.mut().userAssetVolume.set(qpi.invocator(), locals.userVolume);
        state.mut().STARCirclatingSupply += QUSINO_BONUS_CLAIM_AMOUNT_STAR;
        state.mut().QSCCirclatingSupply += QUSINO_BONUS_CLAIM_AMOUNT_QSC;
        state.mut().lastClaimedTime = locals.curDate;
        state.mut().userDailyClaimedBonus.set(qpi.invocator(), locals.curDate);
        state.mut().epochRevenue += QUSINO_BONUS_CLAIM_AMOUNT_STAR;      // 100STAR is 100Qubic for each bonus claim
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
        if (qpi.transfer(qpi.invocator(), input.amount * QUSINO_QSC_PRICE) < 0)
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
        if (input.offset + 32 >= 1024) 
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
        if (input.offset + 32 >= QUSINO_MAX_NUMBER_OF_GAMES) 
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

        REGISTER_USER_PROCEDURE(earnSTAR, 1);
        REGISTER_USER_PROCEDURE(transferSTAROrQSC, 2);
        REGISTER_USER_PROCEDURE(submitGame, 3);
        REGISTER_USER_PROCEDURE(voteInGameProposal, 4);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 5);
        REGISTER_USER_PROCEDURE(depositBonus, 6);
        REGISTER_USER_PROCEDURE(dailyClaimBonus, 7);
        REGISTER_USER_PROCEDURE(redemptionQSCToQubic, 8);
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
            state.mut().epochRevenue += div<uint64>(locals.userVolume.volumeOfQSC * QUSINO_QSC_PRICE * (1000 - QUSINO_DEVELOPER_FEE) * 1ULL, 1000ULL);
            qpi.transfer(locals.game.proposer, locals.userVolume.volumeOfQSC * QUSINO_QSC_PRICE - div<uint64>(locals.userVolume.volumeOfQSC * QUSINO_QSC_PRICE * (1000 - QUSINO_DEVELOPER_FEE) * 1ULL, 1000ULL));
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

        qpi.transfer(state.get().LPDividendsAddress, div(state.get().epochRevenue * QUSINO_LP_DIVIDENDS_PERCENT * 1ULL, 100ULL));
        qpi.transfer(state.get().CCFDividendsAddress, div(state.get().epochRevenue * QUSINO_CCF_DIVIDENDS_PERCENT * 1ULL, 100ULL));
        qpi.transfer(state.get().treasuryAddress, div(state.get().epochRevenue * QUSINO_TREASURY_DIVIDENDS_PERCENT * 1ULL, 100ULL));
        qpi.distributeDividends(div(state.get().epochRevenue * QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT * 1ULL, 67600ULL));
        locals.QSTAsset.assetName = state.get().QSTAssetName;
        locals.QSTAsset.issuer = state.get().QSTIssuer;
        locals.iter.begin(locals.QSTAsset);
        while (!locals.iter.reachedEnd())
        {
            qpi.transfer(locals.iter.possessor(), div<uint64>(state.get().epochRevenue * QUSINO_QST_HOLDERS_DIVIDENDS_PERCENT * 1ULL, QUSINO_SUPPLY_OF_QST * 1000ULL) * locals.iter.numberOfPossessedShares());
            locals.QSTDividends += div<uint64>(state.get().epochRevenue * QUSINO_QST_HOLDERS_DIVIDENDS_PERCENT * 1ULL, QUSINO_SUPPLY_OF_QST * 1000ULL) * locals.iter.numberOfPossessedShares();
            locals.iter.next();
        }
        state.mut().epochRevenue -= div(state.get().epochRevenue * QUSINO_LP_DIVIDENDS_PERCENT * 1ULL, 100ULL) + div(state.get().epochRevenue * QUSINO_CCF_DIVIDENDS_PERCENT * 1ULL, 100ULL) + div(state.get().epochRevenue * QUSINO_TREASURY_DIVIDENDS_PERCENT * 1ULL, 100ULL) + (div(state.get().epochRevenue * QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT * 1ULL, 67600ULL) * 676) + locals.QSTDividends;
	}

    PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
