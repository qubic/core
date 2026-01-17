using namespace QPI;

constexpr uint64 QUSINO_MAX_USERS = 131072;
constexpr uint64 QUSINO_MAX_NUMBER_OF_GAMES = 131072;
constexpr uint64 QUSINO_GAME_SUBMIT_FEE = 100000000;
constexpr uint32 QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER = 128;
constexpr uint32 QUSINO_REVOTE_DURATION = 78;        // in number of weeks(18 Months)
constexpr uint32 QUSINO_QSC_PRICE = 777;
constexpr uint32 QUSINO_STAR_PRICE = 1000;
constexpr uint32 QUSINO_MIN_STAKING_AMOUNT = 10000;
constexpr uint32 QUSINO_VOTE_FEE = 1000;
constexpr uint32 QUSINO_STAR_STAKING_PERCENT_1 = 1;
constexpr uint32 QUSINO_STAR_STAKING_PERCENT_2 = 2;
constexpr uint32 QUSINO_STAR_STAKING_PERCENT_3 = 3;
constexpr uint32 QUSINO_STAR_STAKING_PERCENT_4 = 5;
constexpr uint32 QUSINO_LP_DIVIDENDS_PERCENT = 20;
constexpr uint32 QUSINO_CCF_DIVIDENDS_PERCENT = 15;
constexpr uint32 QUSINO_TREASURY_DIVIDENDS_PERCENT = 25;
constexpr uint32 QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT = 40;

constexpr sint32 QusinoSuccess = 0;
constexpr sint32 QusinoinsufficientFunds = 1;
constexpr sint32 QusinoinsufficientSTAR = 2;
constexpr sint32 QusinoinsufficientQSC = 3;
constexpr sint32 QusinoLowStaking = 4;
constexpr sint32 QusinoWrongStakingType = 5;
constexpr sint32 QusinoInsufficientVoteFee = 6;
constexpr sint32 QusinoWrongGameURIForVote = 7;
constexpr sint32 QusinoAlreadyVoted = 8;
constexpr sint32 QusinoNotVoteTime = 9;
constexpr sint32 QusinoNoEmptySlot = 10;

struct QUSINO2
{
};

struct QUSINO : public ContractBase
{
public:
    struct buyQSC_input
    {
        uint64 amount;
    };
    struct buyQSC_output
    {
        sint32 returnCode;
    };
    struct buySTAR_input
    {
        uint64 amount;
    };
    struct buySTAR_output
    {
        sint32 returnCode;
    };
    struct transferSTAROrQSC_input
    {
        id dest;
        uint64 amount;
        bool type;              // STAR or QSC
    };
    struct transferSTAROrQSC_output
    {
        sint32 returnCode;
    };
    struct stakeSTAR_input
    {
        uint64 amount;
        uint32 type;            // 1 - a month, 2 - 3 months, 3 - 6 months, 4 - 12 months
    };
    struct stakeSTAR_output
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
        bool yesNo;                 // 1 - yes, 0 - no
    };
    struct voteInGameProposal_output
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

    struct getUserStakingInfo_input
    {
        id user;
        uint32 offset;
    };
    struct getUserStakingInfo_output
    {
        Array<uint64, 128> amount;
        Array<uint32, 128> type;
        Array<uint32, 128> stakedEpoch;
        uint32 counts;
    };
    
    struct gameInfo
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
        Array<gameInfo, 32> games;
    };

    struct getSCInfo_input
    {

    };
    struct getSCInfo_output
    {
        uint64 QSCCirclatingSupply;
        uint64 STARCirclatingSupply;
        uint64 totalStakedSTAR;
        uint64 burntSTAR;
        uint64 rewardedQSC;
        uint64 epochRevenue;
        uint64 maxGameIndex;
    };

    struct getActiveGameList_input
    {
        uint32 offset;
    };
    struct getActiveGameList_output
    {
        Array<gameInfo, 32> games;
        Array<uint64, 32> gameIndexes;
    };

protected:
    struct STARAndQSC
    {
        uint64 volumeOfSTAR;
        uint64 volumeOfQSC;
    };
    HashMap<id, STARAndQSC, QUSINO_MAX_USERS> userAssetVolume;
    struct stakeInfo
    {
        id user;
        uint64 amount;
        uint32 type;                // 1 - a month, 2 - 3 months, 3 - 6 months, 4 - 12 months
        uint32 stakedEpoch;
    };
    Array<stakeInfo, QUSINO_MAX_USERS * 64> userStakeList;
    HashMap<uint64, gameInfo, QUSINO_MAX_NUMBER_OF_GAMES> gameList;
    HashMap<uint64, gameInfo, 1024> failedGameList;
    HashMap<id, Array<uint64, QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER>, QUSINO_MAX_USERS> voteList;
    id LPDividendsAddress, CCFDividendsAddress, treasuryAddress;
    uint64 QSCCirclatingSupply, STARCirclatingSupply, totalStakedSTAR, burntSTAR, rewardedQSC, epochRevenue, maxGameIndex, numberOfStakers;

    struct buyQSC_locals
    {
        STARAndQSC user;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(buyQSC)
    {
        if (input.amount * QUSINO_QSC_PRICE > (uint32)qpi.invocationReward()) 
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QusinoinsufficientFunds;
            return ;
        }
        if (input.amount * QUSINO_QSC_PRICE < (uint32)qpi.invocationReward()) 
        {
            qpi.transfer(qpi.invocator(), input.amount * QUSINO_QSC_PRICE - qpi.invocationReward());
        }
        state.epochRevenue += input.amount * QUSINO_QSC_PRICE;
        state.userAssetVolume.get(qpi.invocator(), locals.user);
        locals.user.volumeOfQSC += input.amount;
        state.userAssetVolume.set(qpi.invocator(), locals.user);
        state.QSCCirclatingSupply += input.amount;
        output.returnCode = QusinoSuccess;
    }

    struct buySTAR_locals
    {
        STARAndQSC user;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(buySTAR)
    {
        if (input.amount * QUSINO_STAR_PRICE > (uint32)qpi.invocationReward()) 
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QusinoinsufficientFunds;
            return ;
        }
        if (input.amount * QUSINO_STAR_PRICE < (uint32)qpi.invocationReward()) 
        {
            qpi.transfer(qpi.invocator(), input.amount * QUSINO_STAR_PRICE - qpi.invocationReward());
        }
        state.epochRevenue += input.amount * QUSINO_STAR_PRICE;
        state.userAssetVolume.get(qpi.invocator(), locals.user);
        locals.user.volumeOfSTAR += input.amount;
        state.userAssetVolume.set(qpi.invocator(), locals.user);
        state.STARCirclatingSupply += input.amount;
        output.returnCode = QusinoSuccess;
    }

    struct transferSTAROrQSC_locals
    {
        STARAndQSC dest, sender;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(transferSTAROrQSC)
    {
        state.userAssetVolume.get(qpi.invocator(), locals.sender);
        state.userAssetVolume.get(input.dest, locals.dest);
        if (input.type)             // if STAR transfer
        {
            if (locals.sender.volumeOfSTAR < input.amount) 
            {
                output.returnCode = QusinoinsufficientSTAR;
                return;
            }
            locals.sender.volumeOfSTAR -= input.amount;
            locals.dest.volumeOfSTAR += input.amount;
        }
        else                        // if QSC transfer
        {
            if (locals.sender.volumeOfQSC < input.amount) 
            {
                output.returnCode = QusinoinsufficientQSC;
                return;
            }
            locals.sender.volumeOfQSC -= input.amount;
            locals.dest.volumeOfQSC += input.amount;
        }
        state.userAssetVolume.set(qpi.invocator(), locals.sender);
        state.userAssetVolume.get(input.dest, locals.dest);
        output.returnCode = QusinoSuccess;
    }

    struct stakeSTAR_locals
    {
        stakeInfo user;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(stakeSTAR)
    {
        if (input.amount < QUSINO_MIN_STAKING_AMOUNT) 
        {
            output.returnCode = QusinoLowStaking;
            return ;
        }
        if (input.type == 0 || input.type > 4) 
        {
            output.returnCode = QusinoWrongStakingType;
            return ;
        }
        state.totalStakedSTAR += input.amount;
        locals.user.user = qpi.invocator();
        locals.user.amount = input.amount;
        locals.user.type = input.type;
        locals.user.stakedEpoch = qpi.epoch();
        state.userStakeList.set(state.numberOfStakers, locals.user);
        state.numberOfStakers++;
        output.returnCode = QusinoSuccess;
    }

    struct submitGame_locals
    {
        gameInfo newGame;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(submitGame)
    {
        if (qpi.invocationReward() < QUSINO_GAME_SUBMIT_FEE) 
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QusinoinsufficientFunds;
            return ;
        }
        if (qpi.invocationReward() > QUSINO_GAME_SUBMIT_FEE) 
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QUSINO_GAME_SUBMIT_FEE);   
        }
        state.epochRevenue += QUSINO_GAME_SUBMIT_FEE;
        locals.newGame.proposedEpoch = qpi.epoch();
        locals.newGame.proposer = qpi.invocator();
        copyMemory(locals.newGame.URI, input.URI);
        locals.newGame.yesVotes = 0;
        locals.newGame.noVotes = 0;
        state.gameList.set(state.maxGameIndex, locals.newGame);
        state.maxGameIndex++;
        output.returnCode = QusinoSuccess;
    }

    struct voteInGameProposal_locals
    {
        STARAndQSC userVolume;
        gameInfo game;
        Array<uint64, QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER> voteStatus;
        uint32 i;
        sint32 emptySlot;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(voteInGameProposal)
    {
        state.userAssetVolume.get(qpi.invocator(), locals.userVolume);
        if (locals.userVolume.volumeOfSTAR < QUSINO_VOTE_FEE) 
        {
            output.returnCode = QusinoInsufficientVoteFee;
            return ;
        }
        state.gameList.get(input.gameIndex, locals.game);
        if (locals.game.proposedEpoch != qpi.epoch() && locals.game.proposedEpoch + QUSINO_REVOTE_DURATION != qpi.epoch()) 
        {
            output.returnCode = QusinoNotVoteTime;
            return ;
        }
        for (locals.i = 0; locals.i < 64; locals.i++) 
        {
            if (locals.game.URI.get(locals.i) != input.URI.get(locals.i)) 
            {
                output.returnCode = QusinoWrongGameURIForVote;
                return ;
            }
        }
        state.voteList.get(qpi.invocator(), locals.voteStatus);
        locals.emptySlot = -1;
        for (locals.i = 0; locals.i < QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER; locals.i++) 
        {
            if (locals.voteStatus.get(locals.i) == input.gameIndex) 
            {
                output.returnCode = QusinoAlreadyVoted;
                return ;
            }
            if (locals.voteStatus.get(locals.i) == 0 && locals.emptySlot == -1) 
            {
                locals.emptySlot = locals.i;
            }
        }
        if (locals.emptySlot == -1) 
        {
            output.returnCode = QusinoNoEmptySlot;
            return ;
        }
        locals.userVolume.volumeOfSTAR -= QUSINO_VOTE_FEE;
        state.burntSTAR += QUSINO_VOTE_FEE;
        state.userAssetVolume.set(qpi.invocator(), locals.userVolume);
        if (input.yesNo) 
        {
            locals.game.yesVotes++;
        }
        else {
            locals.game.noVotes++;
        }
        state.gameList.set(input.gameIndex, locals.game);
        locals.voteStatus.set(locals.emptySlot, input.gameIndex);
        state.voteList.set(qpi.invocator(), locals.voteStatus);
    }

    struct getUserAssetVolume_locals
    {
        STARAndQSC userAsset;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(getUserAssetVolume)
    {
        state.userAssetVolume.get(input.user, locals.userAsset);
        output.QSCAmount = locals.userAsset.volumeOfQSC;
        output.STARAmount = locals.userAsset.volumeOfSTAR;
    }

    struct getUserStakingInfo_locals
    {
        stakeInfo staker;
        uint32 i;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(getUserStakingInfo)
    {
        output.counts = 0;
        for (locals.i = 0 ; locals.i < state.numberOfStakers; locals.i++) {
        
            locals.staker = state.userStakeList.get(locals.i);
            if (input.user == locals.staker.user) 
            {
                if (output.counts >= input.offset)
                {
                    output.amount.set(output.counts - input.offset, locals.staker.amount);
                    output.type.set(output.counts - input.offset, locals.staker.type);
                    output.stakedEpoch.set(output.counts - input.offset, locals.staker.stakedEpoch);
                }
                output.counts++;
            }
            if (output.counts == input.offset + 128) 
            {
                return ;
            }
		}
    }

    struct getFailedGameList_locals
    {
        gameInfo game;
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
        locals.idx = state.failedGameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
            if (locals.cur >= input.offset) 
            {
                if (locals.cur >= input.offset + 32) 
                {
                    return ;
                }
                locals.game = state.failedGameList.value(locals.idx);
                output.games.set(locals.cur - input.offset, locals.game);
            }
            locals.cur++;
            locals.idx = state.failedGameList.nextElementIndex(locals.idx);
		}
    }

    PUBLIC_FUNCTION(getSCInfo)
    {
        output.QSCCirclatingSupply = state.QSCCirclatingSupply;
        output.STARCirclatingSupply = state.STARCirclatingSupply;
        output.totalStakedSTAR = state.totalStakedSTAR;
        output.burntSTAR = state.burntSTAR;
        output.rewardedQSC = state.rewardedQSC;
        output.epochRevenue = state.epochRevenue;
        output.maxGameIndex = state.maxGameIndex;
    }

    struct getActiveGameList_locals
    {
        gameInfo game;
        sint64 idx;
        uint32 cur;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(getActiveGameList)
    {
        if (input.offset + 32 >= QUSINO_MAX_NUMBER_OF_GAMES) 
        {
            return ;
        }
        locals.cur = 0;
        locals.idx = state.gameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
            if (locals.cur >= input.offset) 
            {
                if (locals.cur >= input.offset + 32) 
                {
                    return ;
                }
                locals.game = state.gameList.value(locals.idx);
                output.games.set(locals.cur - input.offset, locals.game);
                output.gameIndexes.set(locals.cur - input.offset, state.gameList.key(locals.idx));
            }
            locals.cur++;
            locals.idx = state.gameList.nextElementIndex(locals.idx);
		}
    }
    
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
        REGISTER_USER_FUNCTION(getUserAssetVolume, 1);
        REGISTER_USER_FUNCTION(getUserStakingInfo, 2);
        REGISTER_USER_FUNCTION(getFailedGameList, 3);
        REGISTER_USER_FUNCTION(getSCInfo, 4);
        REGISTER_USER_FUNCTION(getActiveGameList, 5);

        REGISTER_USER_PROCEDURE(buyQSC, 1);
        REGISTER_USER_PROCEDURE(buySTAR, 2);
        REGISTER_USER_PROCEDURE(transferSTAROrQSC, 3);
        REGISTER_USER_PROCEDURE(stakeSTAR, 4);
        REGISTER_USER_PROCEDURE(submitGame, 5);
        REGISTER_USER_PROCEDURE(voteInGameProposal, 6);
	}

	INITIALIZE()
	{
        state.maxGameIndex = 1;
        state.LPDividendsAddress = ID(_V, _D, _I, _H, _Y, _F, _G, _B, _J, _Z, _P, _V, _V, _F, _O, _R, _Y, _Q, _V, _O, _I, _D, _U, _P, _S, _I, _H, _C, _B, _D, _K, _B, _K, _Y, _J, _V, _X, _L, _P, _Q, _W, _D, _A, _K, _L, _D, _M, _K, _A, _G, _G, _P, _O, _C, _Y, _G);
        state.CCFDividendsAddress = id(CCF_CONTRACT_INDEX, 0, 0, 0);
        state.treasuryAddress = ID(_B, _Z, _X, _I, _A, _E, _X, _W, _R, _S, _X, _M, _C, _A, _W, _A, _N, _G, _V, _Y, _T, _W, _D, _A, _U, _E, _I, _A, _D, _F, _N, _O, _F, _C, _K, _G, _X, _V, _Q, _M, _P, _C, _K, _U, _H, _S, _M, _L, _F, _E, _E, _B, _E, _P, _C, _C);
	}

    struct END_EPOCH_locals
    {
        STARAndQSC userVolume;
        stakeInfo staker;
        gameInfo game;
        sint64 idx;
        uint32 i, stakingPercent;
    };
	END_EPOCH_WITH_LOCALS()
	{
		for (locals.i = 0 ; locals.i < state.numberOfStakers; locals.i++)
        {
			locals.staker = state.userStakeList.get(locals.i);
            locals.stakingPercent = 0;
            if(locals.staker.type == 1 && locals.staker.stakedEpoch + 4 == qpi.epoch())
            {
                locals.stakingPercent = QUSINO_STAR_STAKING_PERCENT_1;
            }
			else if(locals.staker.type == 2 && locals.staker.stakedEpoch + 13 == qpi.epoch())
            {
                locals.stakingPercent = QUSINO_STAR_STAKING_PERCENT_2;
            }
            else if(locals.staker.type == 3 && locals.staker.stakedEpoch + 26 == qpi.epoch())
            {
                locals.stakingPercent = QUSINO_STAR_STAKING_PERCENT_3;
            }
            else if(locals.staker.type == 4 && locals.staker.stakedEpoch + 52 == qpi.epoch())
            {
                locals.stakingPercent = QUSINO_STAR_STAKING_PERCENT_4;
            }
            if (locals.stakingPercent) 
            {
                state.userAssetVolume.get(locals.staker.user, locals.userVolume);
                locals.userVolume.volumeOfSTAR += locals.staker.amount;
                locals.userVolume.volumeOfQSC += div(locals.staker.amount * locals.stakingPercent * 1ULL, 100ULL);
                state.rewardedQSC += div(locals.staker.amount * locals.stakingPercent * 1ULL, 100ULL);
                state.userAssetVolume.set(locals.staker.user, locals.userVolume);
                state.userStakeList.set(locals.i, state.userStakeList.get(state.numberOfStakers - 1));
                state.numberOfStakers--;
                locals.i--;
            }
		}
        
        state.failedGameList.reset();
        locals.idx = state.gameList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
        {
            locals.game = state.gameList.value(locals.idx);
            if (locals.game.noVotes >= locals.game.yesVotes) 
            {
                if (locals.game.proposedEpoch == qpi.epoch() || locals.game.proposedEpoch + QUSINO_REVOTE_DURATION == qpi.epoch()) 
                {
                    state.failedGameList.set(state.gameList.key(locals.idx), locals.game);
                    state.gameList.removeByIndex(locals.idx);
                }
            }
            locals.idx = state.gameList.nextElementIndex(locals.idx);
        }
        state.gameList.cleanupIfNeeded();
        state.voteList.reset();

        qpi.transfer(state.LPDividendsAddress, div(state.epochRevenue * QUSINO_LP_DIVIDENDS_PERCENT * 1ULL, 100ULL));
        qpi.transfer(state.CCFDividendsAddress, div(state.epochRevenue * QUSINO_CCF_DIVIDENDS_PERCENT * 1ULL, 100ULL));
        qpi.transfer(state.treasuryAddress, div(state.epochRevenue * QUSINO_TREASURY_DIVIDENDS_PERCENT * 1ULL, 100ULL));
        qpi.distributeDividends(div(state.epochRevenue * QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT * 1ULL, 67600ULL));
        state.epochRevenue -= div(state.epochRevenue * QUSINO_LP_DIVIDENDS_PERCENT * 1ULL, 100ULL) + div(state.epochRevenue * QUSINO_CCF_DIVIDENDS_PERCENT * 1ULL, 100ULL) + div(state.epochRevenue * QUSINO_TREASURY_DIVIDENDS_PERCENT * 1ULL, 100ULL) + (div(state.epochRevenue * QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT * 1ULL, 67600ULL) * 676);
	}
};
