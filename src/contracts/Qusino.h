using namespace QPI;

constexpr uint64 QUSINO_MAX_USERS = 131072;
constexpr uint64 QUSINO_MAX_NUMBER_OF_GAMES = 131072;
constexpr uint64 QUSINO_GAME_SUBMIT_FEE = 100000000;
constexpr uint32 QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER = 128;
constexpr uint32 QUSINO_REVOTE_DURATION = 78;        // in number of weeks(18 Months)
constexpr uint32 QUSINO_STAR_BONUS_FOR_QSC = 1000;
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
constexpr uint64 QUSINO_INFINITY_PRICE = 1000000000000000000ULL;

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
constexpr sint32 QusinoinsufficientQSTAmountForSale = 11;
constexpr sint32 QusinoinvalidTransfer = 12;
constexpr sint32 QusinoinsufficientQST = 13;
constexpr sint32 QusinoWrongAssetType = 14;

struct QUSINO2
{
};

struct QUSINO : public ContractBase
{
public:
    struct buyQST_input
    {
        uint64 amount;
        bool type;                  // 0 - Qubic, 1 - QSC
    };
    struct buyQST_output
    {
        sint32 returnCode;
    };
    struct earnSTAR_input
    {
        uint64 amount;
    };
    struct earnSTAR_output
    {
        sint32 returnCode;
    };
    struct earnQSC_input
    {
        uint64 amount;
    };
    struct earnQSC_output
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
    struct stakeAssets_input
    {
        uint64 amount;
        uint32 type;            // 1 - a month, 2 - 3 months, 3 - 6 months, 4 - 12 months
        uint32 typeOfAsset;      // 1 - STAR, 2 - QSC, 3 - QST
    };
    struct stakeAssets_output
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

    struct depositQSTForSale_input
    {
        uint64 amount;
    };
    struct depositQSTForSale_output
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
        Array<uint32, 128> typeOfAsset;
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
        uint64 totalStakedQSC;
        uint64 totalStakedQST;
        uint64 burntSTAR;
        uint64 epochRevenue;
        uint64 maxGameIndex;
        uint64 numberOfStakers;
        uint64 QSTAmountForSale;
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
        uint32 typeOfAsset;          // 1 - STAR, 2 - QSC, 3 - QST
    };
    Array<stakeInfo, QUSINO_MAX_USERS * 64> userStakeList;
    HashMap<uint64, gameInfo, QUSINO_MAX_NUMBER_OF_GAMES> gameList;
    HashMap<uint64, gameInfo, 1024> failedGameList;
    HashMap<id, Array<uint64, QUSINO_MAX_NUMBER_OF_GAMES_FOR_VOTING_PER_USER>, QUSINO_MAX_USERS> voteList;
    id LPDividendsAddress, CCFDividendsAddress, treasuryAddress, QSTIssuer;
    uint64 QSCCirclatingSupply, STARCirclatingSupply, totalStakedSTAR, totalStakedQSC, totalStakedQST, burntSTAR, epochRevenue, maxGameIndex, numberOfStakers, QSTAmountForSale, QSTAssetName;
    sint64 transferRightsFee;

    struct buyQST_locals
    {
        QX::AssetAskOrders_input input;
        QX::AssetAskOrders_output output;
        STARAndQSC user;
        sint64 minPrice;
        uint32 idx;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(buyQST)
    {
        if (state.QSTAmountForSale < input.amount) 
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QusinoinsufficientQSTAmountForSale;
            return ;
        }
        if (input.type == 0)              // if Qubic buy 
        {
            locals.input.issuer = state.QSTIssuer;
            locals.input.assetName = state.QSTAssetName;
            locals.input.offset = 0;
            CALL_OTHER_CONTRACT_FUNCTION(QX, AssetAskOrders, locals.input, locals.output);
            locals.minPrice = QUSINO_INFINITY_PRICE;
            for (locals.idx = 0; locals.idx < 256; locals.idx++)
            {
                if (locals.output.orders.get(locals.idx).price < locals.minPrice && locals.output.orders.get(locals.idx).price > 0)
                {
                    locals.minPrice = locals.output.orders.get(locals.idx).price;
                }
            }
            if (locals.minPrice == QUSINO_INFINITY_PRICE)
            {
                locals.minPrice = 777;
            }
            if (input.amount * locals.minPrice > (uint32)qpi.invocationReward()) 
            {
                if (qpi.invocationReward() > 0) 
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                output.returnCode = QusinoinsufficientFunds;
                return ;
            }
            if (qpi.transferShareOwnershipAndPossession(state.QSTAssetName, state.QSTIssuer, SELF, SELF, input.amount, qpi.invocator()) < 0)
            {
                if (qpi.invocationReward() > 0) 
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                output.returnCode = QusinoinvalidTransfer;
                return ;
            }
            if (input.amount * locals.minPrice < (uint32)qpi.invocationReward()) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount * locals.minPrice);
            }
            state.QSTAmountForSale -= input.amount;
        }
        else                        // if QSC buy
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.userAssetVolume.get(qpi.invocator(), locals.user);
            if (locals.user.volumeOfQSC < input.amount)
            {
                output.returnCode = QusinoinsufficientQSC;
                return ;
            }
            if (qpi.transferShareOwnershipAndPossession(state.QSTAssetName, state.QSTIssuer, SELF, SELF, input.amount, qpi.invocator()) < 0)
            {
                if (qpi.invocationReward() > 0) 
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                output.returnCode = QusinoinvalidTransfer;
                return ;
            }
            locals.user.volumeOfQSC -= input.amount;
            state.userAssetVolume.set(qpi.invocator(), locals.user);
            state.QSCCirclatingSupply -= input.amount;
            state.QSTAmountForSale -= input.amount;
        }
        output.returnCode = QusinoSuccess;
    }

    struct earnSTAR_locals
    {
        STARAndQSC user;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(earnSTAR)
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

    struct earnQSC_locals
    {
        STARAndQSC user;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(earnQSC)
    {
        if (qpi.invocationReward() > 0) 
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.transferShareOwnershipAndPossession(state.QSTAssetName, state.QSTIssuer, qpi.invocator(), qpi.invocator(), input.amount, SELF) < 0)
        {
            output.returnCode = QusinoinvalidTransfer;
            return ;
        }
        state.userAssetVolume.get(qpi.invocator(), locals.user);
        locals.user.volumeOfQSC += input.amount;
        locals.user.volumeOfSTAR += QUSINO_STAR_BONUS_FOR_QSC;
        state.userAssetVolume.set(qpi.invocator(), locals.user);
        state.QSCCirclatingSupply += input.amount;
        state.STARCirclatingSupply += QUSINO_STAR_BONUS_FOR_QSC;
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
        state.userAssetVolume.set(input.dest, locals.dest);
        output.returnCode = QusinoSuccess;
    }

    struct stakeAssets_locals
    {
        stakeInfo user;
        STARAndQSC userVolume;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(stakeAssets)
    {
        if (qpi.invocationReward() > 0) 
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
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
        state.userAssetVolume.get(qpi.invocator(), locals.userVolume);
        if (input.typeOfAsset == 1)
        {
            if (locals.userVolume.volumeOfSTAR < input.amount)
            {
                output.returnCode = QusinoinsufficientSTAR;
                return ;
            }
            locals.userVolume.volumeOfSTAR -= input.amount;
            state.STARCirclatingSupply -= input.amount;
            state.userAssetVolume.set(qpi.invocator(), locals.userVolume);
            state.totalStakedSTAR += input.amount;
        }
        else if (input.typeOfAsset == 2)
        {
            if (locals.userVolume.volumeOfQSC < input.amount)
            {
                output.returnCode = QusinoinsufficientQSC;
                return ;
            }
            locals.userVolume.volumeOfQSC -= input.amount;
            state.QSCCirclatingSupply -= input.amount;
            state.userAssetVolume.set(qpi.invocator(), locals.userVolume);
            state.totalStakedQSC += input.amount;
        }
        else if (input.typeOfAsset == 3)
        {
            if (qpi.transferShareOwnershipAndPossession(state.QSTAssetName, state.QSTIssuer, qpi.invocator(), qpi.invocator(), input.amount, SELF) < 0)
            {
                output.returnCode = QusinoinvalidTransfer;
                return ;
            }
            state.totalStakedQST += input.amount;
        }
        else 
        {
            output.returnCode = QusinoWrongAssetType;
            return ;
        }
        locals.user.user = qpi.invocator();
        locals.user.amount = input.amount;
        locals.user.type = input.type;
        locals.user.stakedEpoch = qpi.epoch();
        locals.user.typeOfAsset = input.typeOfAsset;
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

    PUBLIC_PROCEDURE(depositQSTForSale)
    {
        if (qpi.transferShareOwnershipAndPossession(state.QSTAssetName, state.QSTIssuer, qpi.invocator(), qpi.invocator(), input.amount, SELF) < 0)
        {
            if (qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QusinoinvalidTransfer;
            return ;
        }
        state.QSTAmountForSale += input.amount;
        output.returnCode = QusinoSuccess;
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
                    output.typeOfAsset.set(output.counts - input.offset, locals.staker.typeOfAsset);
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
        output.totalStakedQSC = state.totalStakedQSC;
        output.totalStakedQST = state.totalStakedQST;
        output.burntSTAR = state.burntSTAR;
        output.epochRevenue = state.epochRevenue;
        output.maxGameIndex = state.maxGameIndex;
        output.numberOfStakers = state.numberOfStakers;
        output.QSTAmountForSale = state.QSTAmountForSale;
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

    PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < state.transferRightsFee)
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
				input.newManagingContractIndex, input.newManagingContractIndex, state.transferRightsFee) < 0)
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
				if (qpi.invocationReward() > state.transferRightsFee)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.transferRightsFee);
				}
			}
		}
	}
    
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
        REGISTER_USER_FUNCTION(getUserAssetVolume, 1);
        REGISTER_USER_FUNCTION(getUserStakingInfo, 2);
        REGISTER_USER_FUNCTION(getFailedGameList, 3);
        REGISTER_USER_FUNCTION(getSCInfo, 4);
        REGISTER_USER_FUNCTION(getActiveGameList, 5);

        REGISTER_USER_PROCEDURE(buyQST, 1);
        REGISTER_USER_PROCEDURE(earnSTAR, 2);
        REGISTER_USER_PROCEDURE(earnQSC, 3);
        REGISTER_USER_PROCEDURE(transferSTAROrQSC, 4);
        REGISTER_USER_PROCEDURE(stakeAssets, 5);
        REGISTER_USER_PROCEDURE(submitGame, 6);
        REGISTER_USER_PROCEDURE(voteInGameProposal, 7);
        REGISTER_USER_PROCEDURE(depositQSTForSale, 8);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 9);
	}

	INITIALIZE()
	{
        state.maxGameIndex = 1;
        state.transferRightsFee = 100;
        state.LPDividendsAddress = ID(_V, _D, _I, _H, _Y, _F, _G, _B, _J, _Z, _P, _V, _V, _F, _O, _R, _Y, _Q, _V, _O, _I, _D, _U, _P, _S, _I, _H, _C, _B, _D, _K, _B, _K, _Y, _J, _V, _X, _L, _P, _Q, _W, _D, _A, _K, _L, _D, _M, _K, _A, _G, _G, _P, _O, _C, _Y, _G);
        state.CCFDividendsAddress = id(CCF_CONTRACT_INDEX, 0, 0, 0);
        state.treasuryAddress = ID(_B, _Z, _X, _I, _A, _E, _X, _W, _R, _S, _X, _M, _C, _A, _W, _A, _N, _G, _V, _Y, _T, _W, _D, _A, _U, _E, _I, _A, _D, _F, _N, _O, _F, _C, _K, _G, _X, _V, _Q, _M, _P, _C, _K, _U, _H, _S, _M, _L, _F, _E, _E, _B, _E, _P, _C, _C);
        state.QSTAssetName = 5526353;
        state.QSTIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);
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
                locals.userVolume.volumeOfSTAR += div(locals.staker.amount * locals.stakingPercent * 1ULL, 100ULL);        // reward for staking
                state.STARCirclatingSupply += div(locals.staker.amount * locals.stakingPercent * 1ULL, 100ULL);
                if (locals.staker.typeOfAsset == 1)
                {
                    locals.userVolume.volumeOfSTAR += locals.staker.amount;
                    state.totalStakedSTAR -= locals.staker.amount;
                }
                else if (locals.staker.typeOfAsset == 2)
                {
                    locals.userVolume.volumeOfQSC += locals.staker.amount;
                    state.totalStakedQSC -= locals.staker.amount;
                }
                else if (locals.staker.typeOfAsset == 3)
                {
                    qpi.transferShareOwnershipAndPossession(state.QSTAssetName, state.QSTIssuer, SELF, SELF, locals.staker.amount, locals.staker.user);
                    state.totalStakedQST -= locals.staker.amount;
                }
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

    PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
