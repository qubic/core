using namespace QPI;

constexpr uint64 QRAFFLE_REGISTER_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_MAX_QRE_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_ASSET_NAME = 19505638103142993;
constexpr uint32 QRAFFLE_LOGOUT_FEE = 50000000;
constexpr uint32 QRAFFLE_TRANSFER_SHARE_FEE = 100;
constexpr uint32 QRAFFLE_BURN_FEE = 10; // percent
constexpr uint32 QRAFFLE_REGISTER_FEE = 5; // percent
constexpr uint32 QRAFFLE_FEE = 1; // percent
constexpr uint32 QRAFFLE_CHARITY_FEE = 1; // percent
constexpr uint32 QRAFFLE_SHRAEHOLDER_FEE = 3; // percent
constexpr uint32 QRAFFLE_MAX_EPOCH = 65536;
constexpr uint32 QRAFFLE_MAX_PROPOSAL_EPOCH = 128;
constexpr uint32 QRAFFLE_MAX_MEMBER = 65536;

constexpr sint32 QRAFFLE_SUCCESS = 0;
constexpr sint32 QRAFFLE_INSUFFICIENT_FUND = 1;
constexpr sint32 QRAFFLE_ALREADY_REGISTERED = 2;
constexpr sint32 QRAFFLE_UNREGISTERED = 3;
constexpr sint32 QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED = 4;
constexpr sint32 QRAFFLE_INVALID_PROPOSAL = 5;
constexpr sint32 QRAFFLE_FAILED_TO_DEPOSITE = 6;
constexpr sint32 QRAFFLE_ALREADY_VOTED = 7;
constexpr sint32 QRAFFLE_INVALID_TOKEN_RAFFLE = 8;
constexpr sint32 QRAFFLE_INVALID_OFFSET_OR_LIMIT = 9;
constexpr sint32 QRAFFLE_INVALID_EPOCH = 10;

struct QRAFFLE2
{
};

struct QRAFFLE : public ContractBase
{
public:
	struct registerInSystem_input
	{
	};

	struct registerInSystem_output
	{
		sint32 returnCode;
	};

	struct logoutInSystem_input
	{
	};

	struct logoutInSystem_output
	{
		sint32 returnCode;
	};

	struct submitEntryAmount_input
	{
		uint64 amount;
	};

	struct submitEntryAmount_output
	{
		sint32 returnCode;
	};

	struct submitProposal_input
	{
		Asset token;
		uint64 entryAmount;
	};

	struct submitProposal_output
	{
		sint32 returnCode;
	};

	struct voteInProposal_input
	{
		uint32 indexOfProposal;
		bit yes;
	};

	struct voteInProposal_output
	{
		sint32 returnCode;
	};

	struct depositeInQuRaffle_input
	{
		
	};

	struct depositeInQuRaffle_output
	{
		sint32 returnCode;
	};

	struct depositeInTokenRaffle_input
	{
		uint32 indexOfTokenRaffle;
	};

	struct depositeInTokenRaffle_output
	{
		sint32 returnCode;
	};

	struct getRegisters_input
	{
		uint32 offset;
		uint32 limit;
	};

	struct getRegisters_output
	{
		Array <id, 1024> registers;
		sint32 returnCode;
	};

	struct getAnalaytics_input
	{
	};

	struct getAnalaytics_output
	{
		uint64 totalBurnAmount;
		uint64 totalCharityAmount;
		uint64 totalShareholderAmount;
		uint64 totalRegisterAmount;
		uint64 totalFeeAmount;
		uint64 totalWinnerAmount;
		uint64 lagestWinnerAmount;
		uint32 numberOfRegisters;
		uint32 numberOfProposals;
		uint32 numberOfQuRaffleMembers;
		uint32 numberOfActiveTokenRaffle;
		uint32 numberOfEndedTokenRaffle;
		uint32 numberOfEntryAmountSubmitted;		
		sint32 returnCode;
	};

	struct getActiveProposal_input
	{
		uint32 indexOfProposal;
	};

	struct getActiveProposal_output
	{
		Asset token;
		uint64 entryAmount;
		uint32 nYes;
		uint32 nNo;
		sint32 returnCode;
	};

	struct getEndedTokenRaffle_input
	{
		uint32 indexOfRaffle;
	};

	struct getEndedTokenRaffle_output
	{
		id epochWinner;
		Asset token;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		uint16 epoch;
		sint32 returnCode;
	};

	struct getEpochWinner_input
	{
		uint32 epoch;
	};

	struct getEpochWinner_output
	{
		id winner;
		sint32 returnCode;
	};

	struct getEpochRaffleIndexes_input
	{
		uint16 epoch;
	};

	struct getEpochRaffleIndexes_output
	{
		uint32 StartIndex;
		uint32 EndIndex;
		sint32 returnCode;
	};

	struct getEndedQuRaffle_input
	{
		uint16 epoch;
	};
	
	struct getEndedQuRaffle_output
	{
		id epochWinner;
		uint64 receivedAmount;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		sint32 returnCode;
	};

	struct getActiveTokenRaffle_input
	{
		uint32 indexOfTokenRaffle;
	};
	
	struct getActiveTokenRaffle_output
	{
		Asset token;
		uint64 entryAmount;
		sint32 returnCode;
	};

protected:

	HashSet <id, QRAFFLE_MAX_MEMBER> registers;

	struct proposalInfo {
		Asset token;
		uint64 entryAmount;
		uint32 nYes;
		uint32 nNo;
	};
	Array <proposalInfo, QRAFFLE_MAX_PROPOSAL_EPOCH> proposals;

	struct votedId {
		id user;
		bit status;
	};
	HashMap <uint32, Array <votedId, QRAFFLE_MAX_MEMBER>, QRAFFLE_MAX_PROPOSAL_EPOCH> voteStatus;
	Array <votedId, QRAFFLE_MAX_MEMBER> tmpVoteStatus;
	Array <uint32, QRAFFLE_MAX_PROPOSAL_EPOCH> numberOfVotedInProposal;
	Array <id, QRAFFLE_MAX_MEMBER> quRaffleMembers;

	struct activeTokenRaffleInfo {
		Asset token;
		uint64 entryAmount;
	};
	Array <activeTokenRaffleInfo, QRAFFLE_MAX_PROPOSAL_EPOCH> activeTokenRaffle;
	HashMap <uint32, Array <id, QRAFFLE_MAX_MEMBER>, QRAFFLE_MAX_PROPOSAL_EPOCH> tokenRaffleMembers;
	Array <uint32, QRAFFLE_MAX_PROPOSAL_EPOCH> numberOfTokenRaffleMembers;
	Array <id, QRAFFLE_MAX_MEMBER> tmpTokenRaffleMembers;

	struct QuRaffleInfo
	{
		id epochWinner;
		uint64 receivedAmount;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
	};
	Array <QuRaffleInfo, QRAFFLE_MAX_EPOCH> QuRaffles;
	struct tokenRaffleInfo
	{
		id epochWinner;
		Asset token;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		uint16 epoch;
	};
	Array <tokenRaffleInfo, 1048576> tokenRaffle;
	HashMap <id, uint64, QRAFFLE_MAX_MEMBER> quRaffleEntryAmount;
	HashSet <id, 1024> shareholdersList;

	id charityAddress, feeAddress;
	uint64 epochRevenue, qREAmount, totalBurnAmount, totalCharityAmount, totalShareholderAmount, totalRegisterAmount, totalFeeAmount, totalWinnerAmount, lagestWinnerAmount;
	uint32 numberOfRegisters, numberOfQuRaffleMembers, numberOfEntryAmountSubmitted, numberOfProposals, numberOfActiveTokenRaffle, numberOfEndedTokenRaffle;

	PUBLIC_PROCEDURE(registerInSystem)
	{
		if (qpi.invocationReward() < QRAFFLE_REGISTER_AMOUNT)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			return ;
		}
		if (state.registers.contains(qpi.invocator()))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_ALREADY_REGISTERED;
			return ;
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_REGISTER_AMOUNT);

		state.registers.add(qpi.invocator());
		state.numberOfRegisters++;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_PROCEDURE(logoutInSystem)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			return ;
		}
		qpi.transfer(qpi.invocator(), QRAFFLE_REGISTER_AMOUNT - QRAFFLE_LOGOUT_FEE);
		state.epochRevenue += QRAFFLE_LOGOUT_FEE;
		state.registers.remove(qpi.invocator());
		state.numberOfRegisters--;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_PROCEDURE(submitEntryAmount)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			return ;
		}
		if (state.quRaffleEntryAmount.contains(qpi.invocator()) == 0)
		{
			state.numberOfEntryAmountSubmitted++;
		}
		state.quRaffleEntryAmount.set(qpi.invocator(), input.amount);
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct submitProposal_locals
	{
		proposalInfo proposal;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitProposal)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			return ;
		}
		if (state.numberOfProposals >= QRAFFLE_MAX_PROPOSAL_EPOCH)
		{
			output.returnCode = QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED;
			return ;
		}
		locals.proposal.token = input.token;
		locals.proposal.entryAmount = input.entryAmount;
		state.proposals.set(state.numberOfProposals, locals.proposal);
		state.numberOfProposals++;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct voteInProposal_locals
	{
		proposalInfo proposal;
		votedId votedId;
		uint32 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(voteInProposal)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			return ;
		}
		if (input.indexOfProposal >= state.numberOfProposals)
		{
			output.returnCode = QRAFFLE_INVALID_PROPOSAL;
			return ;
		}
		locals.proposal = state.proposals.get(input.indexOfProposal);
		state.voteStatus.get(input.indexOfProposal, state.tmpVoteStatus);
		for (locals.i = 0; locals.i < state.numberOfVotedInProposal.get(input.indexOfProposal); locals.i++)
		{
			if (state.tmpVoteStatus.get(locals.i).user == qpi.invocator())
			{
				if (state.tmpVoteStatus.get(locals.i).status == input.yes)
				{
					output.returnCode = QRAFFLE_ALREADY_VOTED;
					return ;
				}
				else
				{
					if (input.yes)
					{
						locals.proposal.nYes++;
						locals.proposal.nNo--;
					}
					else
					{
						locals.proposal.nNo++;
						locals.proposal.nYes--;
					}
					state.proposals.set(input.indexOfProposal, locals.proposal);
				}

				locals.votedId.user = qpi.invocator();
				locals.votedId.status = input.yes;
				state.tmpVoteStatus.set(locals.i, locals.votedId);
				state.voteStatus.set(input.indexOfProposal, state.tmpVoteStatus);
				output.returnCode = QRAFFLE_SUCCESS;
				return ;
			}
		}
		if (input.yes)
		{
			locals.proposal.nYes++;
		}
		else
		{
			locals.proposal.nNo++;
		}
		state.proposals.set(input.indexOfProposal, locals.proposal);

		locals.votedId.user = qpi.invocator();
		locals.votedId.status = input.yes;
		state.tmpVoteStatus.set(state.numberOfVotedInProposal.get(input.indexOfProposal), locals.votedId);
		state.voteStatus.set(input.indexOfProposal, state.tmpVoteStatus);
		state.numberOfVotedInProposal.set(input.indexOfProposal, state.numberOfVotedInProposal.get(input.indexOfProposal) + 1);
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct depositeInQuRaffle_locals
	{
		uint32 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositeInQuRaffle)
	{
		if (qpi.invocationReward() < (sint64)state.qREAmount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			return ;
		}
		for (locals.i = 0 ; locals.i < state.numberOfQuRaffleMembers; locals.i++)
		{
			if (state.quRaffleMembers.get(locals.i) == qpi.invocator())
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = QRAFFLE_ALREADY_REGISTERED;
				return ;
			}
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.qREAmount);
		state.quRaffleMembers.set(state.numberOfQuRaffleMembers, qpi.invocator());
		state.numberOfQuRaffleMembers++;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct depositeInTokenRaffle_locals
	{
		uint32 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositeInTokenRaffle)
	{
		if (qpi.invocationReward() < QRAFFLE_TRANSFER_SHARE_FEE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			return ;
		}
		if (input.indexOfTokenRaffle >= state.numberOfActiveTokenRaffle)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			return ;
		}
		if (qpi.transferShareOwnershipAndPossession(state.activeTokenRaffle.get(input.indexOfTokenRaffle).token.assetName, state.activeTokenRaffle.get(input.indexOfTokenRaffle).token.issuer, qpi.invocator(), qpi.invocator(), state.activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount, SELF) != state.activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_FAILED_TO_DEPOSITE;
			return ;
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_TRANSFER_SHARE_FEE);
		state.tokenRaffleMembers.get(input.indexOfTokenRaffle, state.tmpTokenRaffleMembers);
		state.tmpTokenRaffleMembers.set(state.numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle), qpi.invocator());
		state.numberOfTokenRaffleMembers.set(input.indexOfTokenRaffle, state.numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle) + 1);
		state.tokenRaffleMembers.set(input.indexOfTokenRaffle, state.tmpTokenRaffleMembers);
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct getRegisters_locals
	{
		id user;
		sint64 idx;
		uint32 i;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getRegisters)
	{
		if (input.limit > 1024)
		{
			output.returnCode = QRAFFLE_INVALID_OFFSET_OR_LIMIT;
			return ;
		}
		if (input.offset + input.limit > state.numberOfRegisters)
		{
			output.returnCode = QRAFFLE_INVALID_OFFSET_OR_LIMIT;
			return ;
		}
		locals.idx = state.registers.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.user = state.registers.key(locals.idx);
			if (locals.i >= input.offset && locals.i < input.offset + input.limit)
			{
				output.registers.set(locals.i - input.offset, locals.user);
			}
			if (locals.i >= input.offset + input.limit)
			{
				break;
			}
			locals.i++;
			locals.idx = state.registers.nextElementIndex(locals.idx);
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}
	
	PUBLIC_FUNCTION(getAnalaytics)
	{
		output.totalBurnAmount = state.totalBurnAmount;
		output.totalCharityAmount = state.totalCharityAmount;
		output.totalShareholderAmount = state.totalShareholderAmount;
		output.totalRegisterAmount = state.totalRegisterAmount;
		output.totalFeeAmount = state.totalFeeAmount;
		output.totalWinnerAmount = state.totalWinnerAmount;
		output.lagestWinnerAmount = state.lagestWinnerAmount;
		output.numberOfRegisters = state.numberOfRegisters;
		output.numberOfProposals = state.numberOfProposals;
		output.numberOfQuRaffleMembers = state.numberOfQuRaffleMembers;
		output.numberOfActiveTokenRaffle = state.numberOfActiveTokenRaffle;
		output.numberOfEndedTokenRaffle = state.numberOfEndedTokenRaffle;
		output.numberOfEntryAmountSubmitted = state.numberOfEntryAmountSubmitted;
		output.returnCode = QRAFFLE_SUCCESS;
	}
	
	PUBLIC_FUNCTION(getActiveProposal)
	{
		output.token.assetName = state.proposals.get(input.indexOfProposal).token.assetName;
		output.token.issuer = state.proposals.get(input.indexOfProposal).token.issuer;
		output.entryAmount = state.proposals.get(input.indexOfProposal).entryAmount;
		output.nYes = state.proposals.get(input.indexOfProposal).nYes;
		output.nNo = state.proposals.get(input.indexOfProposal).nNo;
		output.returnCode = QRAFFLE_SUCCESS;
	}
	
	PUBLIC_FUNCTION(getEndedTokenRaffle)
	{
		output.epochWinner = state.tokenRaffle.get(input.indexOfRaffle).epochWinner;
		output.token.assetName = state.tokenRaffle.get(input.indexOfRaffle).token.assetName;
		output.token.issuer = state.tokenRaffle.get(input.indexOfRaffle).token.issuer;
		output.entryAmount = state.tokenRaffle.get(input.indexOfRaffle).entryAmount;
		output.numberOfMembers = state.tokenRaffle.get(input.indexOfRaffle).numberOfMembers;
		output.winnerIndex = state.tokenRaffle.get(input.indexOfRaffle).winnerIndex;
		output.epoch = state.tokenRaffle.get(input.indexOfRaffle).epoch;
		output.returnCode = QRAFFLE_SUCCESS;
	}
	
	PUBLIC_FUNCTION(getEpochWinner)
	{
		output.winner = state.QuRaffles.get(input.epoch).epochWinner;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct getEpochRaffleIndexes_locals
	{
		sint32 i;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getEpochRaffleIndexes)
	{
		if (input.epoch > qpi.epoch())
		{
			output.returnCode = QRAFFLE_INVALID_EPOCH;
			return ;
		}
		if (input.epoch == qpi.epoch())
		{
			output.StartIndex = 0;
			output.EndIndex = state.numberOfActiveTokenRaffle;
			return ;
		}
		for (locals.i = 0; locals.i < (sint32)state.numberOfEndedTokenRaffle; locals.i++)
		{
			if (state.tokenRaffle.get(locals.i).epoch == input.epoch)
			{
				output.StartIndex = locals.i;
				break;
			}
		}
		for (locals.i = (sint32)state.numberOfEndedTokenRaffle - 1; locals.i >= 0; locals.i--)
		{
			if (state.tokenRaffle.get(locals.i).epoch == input.epoch)
			{
				output.EndIndex = locals.i;
				break;
			}
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getEndedQuRaffle)
	{
		output.epochWinner = state.QuRaffles.get(input.epoch).epochWinner;
		output.receivedAmount = state.QuRaffles.get(input.epoch).receivedAmount;
		output.entryAmount = state.QuRaffles.get(input.epoch).entryAmount;
		output.numberOfMembers = state.QuRaffles.get(input.epoch).numberOfMembers;
		output.winnerIndex = state.QuRaffles.get(input.epoch).winnerIndex;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getActiveTokenRaffle)
	{
		output.token.assetName = state.activeTokenRaffle.get(input.indexOfTokenRaffle).token.assetName;
		output.token.issuer = state.activeTokenRaffle.get(input.indexOfTokenRaffle).token.issuer;
		output.entryAmount = state.activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(getRegisters, 1);
		REGISTER_USER_FUNCTION(getAnalaytics, 2);
		REGISTER_USER_FUNCTION(getActiveProposal, 3);
		REGISTER_USER_FUNCTION(getEndedTokenRaffle, 4);
		REGISTER_USER_FUNCTION(getEndedQuRaffle, 5);
		REGISTER_USER_FUNCTION(getActiveTokenRaffle, 6);
		REGISTER_USER_FUNCTION(getEpochRaffleIndexes, 7);

		REGISTER_USER_PROCEDURE(registerInSystem, 1);
		REGISTER_USER_PROCEDURE(logoutInSystem, 2);
		REGISTER_USER_PROCEDURE(submitEntryAmount, 3);
		REGISTER_USER_PROCEDURE(submitProposal, 4);
		REGISTER_USER_PROCEDURE(voteInProposal, 5);
		REGISTER_USER_PROCEDURE(depositeInQuRaffle, 6);
		REGISTER_USER_PROCEDURE(depositeInTokenRaffle, 7);
	}

	INITIALIZE()
	{
		state.qREAmount = 10000000;
		state.charityAddress = ID(_D, _P, _Q, _R, _L, _S, _Z, _S, _S, _C, _X, _I, _Y, _F, _I, _Q, _G, _B, _F, _B, _X, _X, _I, _S, _D, _D, _E, _B, _E, _G, _Q, _N, _W, _N, _T, _Q, _U, _E, _I, _F, _S, _C, _U, _W, _G, _H, _V, _X, _J, _P, _L, _F, _G, _M, _Y, _D);
	}

	struct BEGIN_EPOCH_locals
	{
	};

	BEGIN_EPOCH_WITH_LOCALS()
	{
	}
	
	struct END_EPOCH_locals
	{
		proposalInfo proposal;
		QuRaffleInfo qraffle;
		tokenRaffleInfo tRaffle;
		activeTokenRaffleInfo acTokenRaffle;
		AssetPossessionIterator iter;
		Asset QraffleAsset;
		id digest, winner, shareholder;
		sint64 idx;
		uint64 sumOfEntryAmountSubmitted, r, winnerRevenue, burnAmount, charityRevenue, shareholderRevenue, registerRevenue, fee;
		uint32 i, j, winnerIndex;
	};

	END_EPOCH_WITH_LOCALS()
	{
		qpi.distributeDividends(div(state.epochRevenue, 676ull));
		state.epochRevenue -= div(state.epochRevenue, 676ull) * 676;

		locals.digest = qpi.getPrevSpectrumDigest();
		locals.r = (qpi.numberOfTickTransactions() + 1) * locals.digest.u64._0 + (qpi.second()) * locals.digest.u64._1 + locals.digest.u64._2;
		locals.winnerIndex = (uint32)mod(locals.r, state.numberOfQuRaffleMembers * 1ull) + 1;
		locals.winner = state.quRaffleMembers.get(locals.winnerIndex);

		locals.burnAmount = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_BURN_FEE, 100ull);
		locals.charityRevenue = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_CHARITY_FEE, 100ull);
		locals.shareholderRevenue = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_SHRAEHOLDER_FEE, 100ull);
		locals.registerRevenue = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_REGISTER_FEE, 100ull);
		locals.fee = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_FEE, 100ull);
		locals.winnerRevenue = state.qREAmount * state.numberOfQuRaffleMembers - locals.burnAmount - locals.charityRevenue - div(locals.shareholderRevenue, 676ull) * 676 - div(locals.registerRevenue, state.numberOfRegisters * 1ull) * state.numberOfRegisters - locals.fee;

		qpi.transfer(locals.winner, locals.winnerRevenue);
		qpi.burn(locals.burnAmount);
		qpi.transfer(state.charityAddress, locals.charityRevenue);
		qpi.distributeDividends(div(locals.shareholderRevenue, 676ull));
		qpi.transfer(state.feeAddress, locals.fee);

		state.totalBurnAmount += locals.burnAmount;
		state.totalCharityAmount += locals.charityRevenue;
		state.totalShareholderAmount += div(locals.shareholderRevenue, 676ull) * 676;
		state.totalRegisterAmount += div(locals.registerRevenue, state.numberOfRegisters * 1ull) * state.numberOfRegisters;
		state.totalFeeAmount += locals.fee;
		state.totalWinnerAmount += locals.winnerRevenue;
		if (locals.winnerRevenue > state.lagestWinnerAmount)
		{
			state.lagestWinnerAmount = locals.winnerRevenue;
		}

		locals.idx = state.registers.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			qpi.transfer(state.registers.key(locals.idx), div(locals.registerRevenue, state.numberOfRegisters * 1ull));
			locals.idx = state.registers.nextElementIndex(locals.idx);
		}

		locals.qraffle.epochWinner = locals.winner;
		locals.qraffle.receivedAmount = locals.winnerRevenue;
		locals.qraffle.entryAmount = state.qREAmount;
		locals.qraffle.numberOfMembers = state.numberOfQuRaffleMembers;
		locals.qraffle.winnerIndex = locals.winnerIndex;
		state.QuRaffles.set(qpi.epoch(), locals.qraffle);

		locals.QraffleAsset.assetName = QRAFFLE_ASSET_NAME;
		locals.QraffleAsset.issuer = NULL_ID;
		locals.iter.begin(locals.QraffleAsset);
		while (!locals.iter.reachedEnd())
		{
			locals.shareholder = locals.iter.possessor();
			if (state.shareholdersList.contains(locals.shareholder) == 0)
			{
				state.shareholdersList.add(locals.shareholder);
			}

			locals.iter.next();
		}
		
		state.numberOfQuRaffleMembers = 0;

		for (locals.i = 0 ; locals.i < state.numberOfActiveTokenRaffle; locals.i++)
		{
			locals.winnerIndex = (uint32)mod(locals.r, state.numberOfTokenRaffleMembers.get(locals.i) * 1ull) + 1;
			state.tokenRaffleMembers.get(locals.i, state.tmpTokenRaffleMembers);
			locals.winner = state.tmpTokenRaffleMembers.get(locals.winnerIndex);

			locals.acTokenRaffle = state.activeTokenRaffle.get(locals.i);

			locals.burnAmount = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_BURN_FEE, 100ull);
			locals.charityRevenue = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_CHARITY_FEE, 100ull);
			locals.shareholderRevenue = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_SHRAEHOLDER_FEE, 100ull);
			locals.registerRevenue = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_REGISTER_FEE, 100ull);
			locals.fee = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_FEE, 100ull);
			locals.winnerRevenue = locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) - locals.burnAmount - locals.charityRevenue - div(locals.shareholderRevenue, 676ULL) * 676 - div(locals.registerRevenue, state.numberOfRegisters * 1ull) * state.numberOfRegisters - locals.fee;
			
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.winnerRevenue, locals.winner);
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.burnAmount, NULL_ID);
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.charityRevenue, state.charityAddress);
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.fee, state.feeAddress);

			locals.idx = state.shareholdersList.nextElementIndex(NULL_INDEX);
			while (locals.idx != NULL_INDEX)
			{
				locals.shareholder = state.shareholdersList.key(locals.idx);
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, div(locals.shareholderRevenue, 676ULL) * qpi.numberOfPossessedShares(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, locals.shareholder, locals.shareholder, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), locals.shareholder);
				locals.idx = state.shareholdersList.nextElementIndex(locals.idx);
			}

			locals.idx = state.registers.nextElementIndex(NULL_INDEX);
			while (locals.idx != NULL_INDEX)
			{
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, div(locals.registerRevenue, state.numberOfRegisters * 1ull), state.registers.key(locals.idx));
				locals.idx = state.registers.nextElementIndex(locals.idx);
			}

			locals.tRaffle.epochWinner = locals.winner;
			locals.tRaffle.token.assetName = locals.acTokenRaffle.token.assetName;
			locals.tRaffle.token.issuer = locals.acTokenRaffle.token.issuer;
			locals.tRaffle.entryAmount = locals.acTokenRaffle.entryAmount;
			locals.tRaffle.numberOfMembers = state.numberOfTokenRaffleMembers.get(locals.i);
			locals.tRaffle.winnerIndex = locals.winnerIndex;
			locals.tRaffle.epoch = qpi.epoch();
			state.tokenRaffle.set(state.numberOfEndedTokenRaffle++, locals.tRaffle);

			state.numberOfTokenRaffleMembers.set(locals.i, 0);
		}

		locals.sumOfEntryAmountSubmitted = 0;
		locals.idx = state.quRaffleEntryAmount.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.sumOfEntryAmountSubmitted += state.quRaffleEntryAmount.value(locals.idx);
			locals.idx = state.quRaffleEntryAmount.nextElementIndex(locals.idx);
		}
		state.qREAmount = div(locals.sumOfEntryAmountSubmitted, state.numberOfEntryAmountSubmitted * 1ull);
		
		state.numberOfActiveTokenRaffle = 0;

		for (locals.i = 0 ; locals.i < state.numberOfProposals; locals.i++)
		{
			locals.proposal = state.proposals.get(locals.i);
			if (locals.proposal.nYes > locals.proposal.nNo)
			{
				locals.acTokenRaffle.token.assetName = locals.proposal.token.assetName;
				locals.acTokenRaffle.token.issuer = locals.proposal.token.issuer;
				locals.acTokenRaffle.entryAmount = locals.proposal.entryAmount;

				state.activeTokenRaffle.set(state.numberOfActiveTokenRaffle++, locals.acTokenRaffle);
			}
		}

		state.quRaffleEntryAmount.reset();
		state.shareholdersList.reset();
		state.numberOfEntryAmountSubmitted = 0;
		state.numberOfProposals = 0;
		state.registers.cleanupIfNeeded();
		state.shareholdersList.cleanupIfNeeded();
		state.quRaffleEntryAmount.cleanupIfNeeded();
	}

	PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
