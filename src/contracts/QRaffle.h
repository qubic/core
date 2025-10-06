using namespace QPI;

constexpr uint64 QRAFFLE_REGISTER_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_QXMR_REGISTER_AMOUNT = 100000000ull;
constexpr uint64 QRAFFLE_MAX_QRE_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_ASSET_NAME = 19505638103142993;
constexpr uint64 QXMR_ASSET_NAME = 1380800593; // QXMR token asset name
constexpr uint32 QRAFFLE_LOGOUT_FEE = 50000000;
constexpr uint32 QXMR_LOGOUT_FEE = 5000000; // QXMR logout fee
constexpr uint32 QRAFFLE_TRANSFER_SHARE_FEE = 100;
constexpr uint32 QRAFFLE_BURN_FEE = 10; // percent
constexpr uint32 QRAFFLE_REGISTER_FEE = 5; // percent
constexpr uint32 QRAFFLE_FEE = 1; // percent
constexpr uint32 QRAFFLE_CHARITY_FEE = 1; // percent
constexpr uint32 QRAFFLE_SHRAEHOLDER_FEE = 3; // percent
constexpr uint32 QRAFFLE_MAX_EPOCH = 65536;
constexpr uint32 QRAFFLE_MAX_PROPOSAL_EPOCH = 128;
constexpr uint32 QRAFFLE_MAX_MEMBER = 65536;
constexpr uint32 QRAFFLE_QXMR_DISTRIBUTION_FEE = 67600;

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
constexpr sint32 QRAFFLE_MAX_MEMBER_REACHED = 11;
constexpr sint32 QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT = 12;
constexpr sint32 QRAFFLE_INSUFFICIENT_QXMR = 13;
constexpr sint32 QRAFFLE_INVALID_TOKEN_TYPE = 14;

enum QRAFFLELogInfo {
    QRAFFLE_success = 0,
    QRAFFLE_insufficientQubic = 1,
    QRAFFLE_insufficientQXMR = 2,
    QRAFFLE_alreadyRegistered = 3,
    QRAFFLE_unregistered = 4,
    QRAFFLE_maxMemberReached = 5,
    QRAFFLE_maxProposalEpochReached = 6,
    QRAFFLE_invalidProposal = 7,
    QRAFFLE_failedToDeposit = 8,
    QRAFFLE_alreadyVoted = 9,
    QRAFFLE_invalidTokenRaffle = 10,
    QRAFFLE_invalidOffsetOrLimit = 11,
    QRAFFLE_invalidEpoch = 12,
    QRAFFLE_initialRegisterCannotLogout = 13,
    QRAFFLE_invalidTokenType = 14,
    QRAFFLE_invalidEntryAmount = 15,
    QRAFFLE_maxMemberReachedForQuRaffle = 16,
    QRAFFLE_proposalNotFound = 17,
    QRAFFLE_proposalAlreadyEnded = 18,
    QRAFFLE_notEnoughShares = 19,
    QRAFFLE_transferFailed = 20,
    QRAFFLE_epochEnded = 21,
    QRAFFLE_winnerSelected = 22,
    QRAFFLE_revenueDistributed = 23,
    QRAFFLE_tokenRaffleCreated = 24,
    QRAFFLE_tokenRaffleEnded = 25,
    QRAFFLE_proposalSubmitted = 26,
    QRAFFLE_proposalVoted = 27,
    QRAFFLE_quRaffleDeposited = 28,
    QRAFFLE_tokenRaffleDeposited = 29,
    QRAFFLE_shareManagementRightsTransferred = 30
};

struct QRAFFLELogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    sint8 _terminator; // Only data before "_terminator" are logged
};

// Enhanced logger for END_EPOCH with detailed information
struct QRAFFLEEndEpochLogger
{
    uint32 _contractIndex;
    uint32 _type;
    uint32 _epoch; // Current epoch number
    uint32 _memberCount; // Number of QuRaffle members
    uint64 _totalAmount; // Total amount being processed
    uint64 _winnerAmount; // Amount won by winner
    uint32 _winnerIndex; // Index of the winner
    sint8 _terminator;
};

// Enhanced logger for revenue distribution
struct QRAFFLERevenueLogger
{
    uint32 _contractIndex;
    uint32 _type;
    uint64 _burnAmount; // Amount burned
    uint64 _charityAmount; // Amount sent to charity
    uint64 _shareholderAmount; // Amount distributed to shareholders
    uint64 _registerAmount; // Amount distributed to registers
    uint64 _feeAmount; // Amount sent to fee address
    uint64 _winnerAmount; // Amount sent to winner
    sint8 _terminator;
};

// Enhanced logger for token raffle processing
struct QRAFFLETokenRaffleLogger
{
    uint32 _contractIndex;
    uint32 _type;
    uint32 _raffleIndex; // Index of the token raffle
    uint64 _assetName; // Asset name of the token
    uint32 _memberCount; // Number of members in this raffle
    uint64 _entryAmount; // Entry amount for this raffle
    uint32 _winnerIndex; // Winner index for this raffle
    uint64 _winnerAmount; // Amount won in this raffle
    sint8 _terminator;
};

// Enhanced logger for proposal processing
struct QRAFFLEProposalLogger
{
    uint32 _contractIndex;
    uint32 _type;
    uint32 _proposalIndex; // Index of the proposal
    uint32 _yesVotes; // Number of yes votes
    uint32 _noVotes; // Number of no votes
    uint64 _assetName; // Asset name if approved
    uint64 _entryAmount; // Entry amount if approved
    sint8 _terminator;
};

struct QRAFFLE2
{
};

struct QRAFFLE : public ContractBase
{
public:
	struct registerInSystem_input
	{
		bit useQXMR; // 0 = use qubic, 1 = use QXMR tokens
	};

	struct registerInSystem_output
	{
		sint32 returnCode;
	};

	struct logoutInSystem_input
	{
		bit useQXMR; // 0 = use qubic, 1 = use QXMR tokens
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
		uint32 epoch;
		sint32 returnCode;
	};

	struct getEpochRaffleIndexes_input
	{
		uint32 epoch;
	};

	struct getEpochRaffleIndexes_output
	{
		uint32 StartIndex;
		uint32 EndIndex;
		sint32 returnCode;
	};

	struct getEndedQuRaffle_input
	{
		uint32 epoch;
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

	HashMap <id, uint8, QRAFFLE_MAX_MEMBER> registers;

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
		uint32 epoch;
	};
	Array <tokenRaffleInfo, 1048576> tokenRaffle;
	HashMap <id, uint64, QRAFFLE_MAX_MEMBER> quRaffleEntryAmount;
	HashSet <id, 1024> shareholdersList;

	id initialRegister1, initialRegister2, initialRegister3, initialRegister4, initialRegister5;
	id charityAddress, feeAddress, QXMRIssuer;
	uint64 epochRevenue, epochQXMRRevenue, qREAmount, totalBurnAmount, totalCharityAmount, totalShareholderAmount, totalRegisterAmount, totalFeeAmount, totalWinnerAmount, lagestWinnerAmount;
	uint32 numberOfRegisters, numberOfQuRaffleMembers, numberOfEntryAmountSubmitted, numberOfProposals, numberOfActiveTokenRaffle, numberOfEndedTokenRaffle;

	struct registerInSystem_locals
	{
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(registerInSystem)
	{
		if (state.registers.contains(qpi.invocator()))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_ALREADY_REGISTERED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_alreadyRegistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.numberOfRegisters >= QRAFFLE_MAX_MEMBER)
		{
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_maxMemberReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if (input.useQXMR)
		{
			// Use QXMR tokens for registration
			if (qpi.numberOfPossessedShares(QXMR_ASSET_NAME, state.QXMRIssuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < QRAFFLE_QXMR_REGISTER_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			
			// Transfer QXMR tokens to the contract
			if (qpi.transferShareOwnershipAndPossession(QXMR_ASSET_NAME, state.QXMRIssuer, qpi.invocator(), qpi.invocator(), QRAFFLE_QXMR_REGISTER_AMOUNT, SELF) < 0)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			state.registers.set(qpi.invocator(), 2);
		}
		else
		{
			// Use qubic for registration
			if (qpi.invocationReward() < QRAFFLE_REGISTER_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQubic, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_REGISTER_AMOUNT);
			state.registers.set(qpi.invocator(), 1);
		}

		state.numberOfRegisters++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct logoutInSystem_locals
	{
		sint64 refundAmount;
		uint8 tokenType;
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(logoutInSystem)
	{
		if (qpi.invocator() == state.initialRegister1 || qpi.invocator() == state.initialRegister2 || qpi.invocator() == state.initialRegister3 || qpi.invocator() == state.initialRegister4 || qpi.invocator() == state.initialRegister5)
		{
			output.returnCode = QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_initialRegisterCannotLogout, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.registers.get(qpi.invocator(), locals.tokenType);
		if (input.useQXMR)
		{
			if (locals.tokenType != 2)
			{
				output.returnCode = QRAFFLE_INVALID_TOKEN_TYPE;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_invalidTokenType, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			// Use QXMR tokens for logout
			locals.refundAmount = QRAFFLE_QXMR_REGISTER_AMOUNT - QXMR_LOGOUT_FEE;
			
			// Check if contract has enough QXMR tokens
			if (qpi.numberOfPossessedShares(QXMR_ASSET_NAME, state.QXMRIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX) < locals.refundAmount)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			
			// Transfer QXMR tokens back to user
			if (qpi.transferShareOwnershipAndPossession(QXMR_ASSET_NAME, state.QXMRIssuer, SELF, SELF, locals.refundAmount, qpi.invocator()) < 0)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			
			state.epochQXMRRevenue += QXMR_LOGOUT_FEE;
		}
		else
		{
			if (locals.tokenType != 1)
			{
				output.returnCode = QRAFFLE_INVALID_TOKEN_TYPE;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_invalidTokenType, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			// Use qubic for logout
			locals.refundAmount = QRAFFLE_REGISTER_AMOUNT - QRAFFLE_LOGOUT_FEE;
			qpi.transfer(qpi.invocator(), locals.refundAmount);
			state.epochRevenue += QRAFFLE_LOGOUT_FEE;
		}

		state.registers.removeByKey(qpi.invocator());
		state.numberOfRegisters--;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct submitEntryAmount_locals
	{
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitEntryAmount)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.quRaffleEntryAmount.contains(qpi.invocator()) == 0)
		{
			state.numberOfEntryAmountSubmitted++;
		}
		state.quRaffleEntryAmount.set(qpi.invocator(), input.amount);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct submitProposal_locals
	{
		proposalInfo proposal;
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitProposal)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.numberOfProposals >= QRAFFLE_MAX_PROPOSAL_EPOCH)
		{
			output.returnCode = QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_maxProposalEpochReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.proposal.token = input.token;
		locals.proposal.entryAmount = input.entryAmount;
		state.proposals.set(state.numberOfProposals, locals.proposal);
		state.numberOfProposals++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_proposalSubmitted, 0 };
		LOG_INFO(locals.log);
	}

	struct voteInProposal_locals
	{
		proposalInfo proposal;
		votedId votedId;
		uint32 i;
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(voteInProposal)
	{
		if (state.registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.indexOfProposal >= state.numberOfProposals)
		{
			output.returnCode = QRAFFLE_INVALID_PROPOSAL;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_proposalNotFound, 0 };
			LOG_INFO(locals.log);
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
					locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_alreadyVoted, 0 };
					LOG_INFO(locals.log);
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
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_proposalVoted, 0 };
				LOG_INFO(locals.log);
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
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_proposalVoted, 0 };
		LOG_INFO(locals.log);
	}

	struct depositeInQuRaffle_locals
	{
		uint32 i;
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositeInQuRaffle)
	{
		if (state.numberOfQuRaffleMembers >= QRAFFLE_MAX_MEMBER)
		{
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_maxMemberReachedForQuRaffle, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (qpi.invocationReward() < (sint64)state.qREAmount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
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
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_alreadyRegistered, 0 };
				LOG_INFO(locals.log);
				return ;
			}
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.qREAmount);
		state.quRaffleMembers.set(state.numberOfQuRaffleMembers, qpi.invocator());
		state.numberOfQuRaffleMembers++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_quRaffleDeposited, 0 };
		LOG_INFO(locals.log);
	}

	struct depositeInTokenRaffle_locals
	{
		uint32 i;
		QRAFFLELogger log;
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
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.indexOfTokenRaffle >= state.numberOfActiveTokenRaffle)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_invalidTokenRaffle, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (qpi.transferShareOwnershipAndPossession(state.activeTokenRaffle.get(input.indexOfTokenRaffle).token.assetName, state.activeTokenRaffle.get(input.indexOfTokenRaffle).token.issuer, qpi.invocator(), qpi.invocator(), state.activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount, SELF) < 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_FAILED_TO_DEPOSITE;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_transferFailed, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_TRANSFER_SHARE_FEE);
		state.tokenRaffleMembers.get(input.indexOfTokenRaffle, state.tmpTokenRaffleMembers);
		state.tmpTokenRaffleMembers.set(state.numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle), qpi.invocator());
		state.numberOfTokenRaffleMembers.set(input.indexOfTokenRaffle, state.numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle) + 1);
		state.tokenRaffleMembers.set(input.indexOfTokenRaffle, state.tmpTokenRaffleMembers);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_tokenRaffleDeposited, 0 };
		LOG_INFO(locals.log);
	}

	struct TransferShareManagementRights_locals
	{
		QRAFFLELogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < QRAFFLE_TRANSFER_SHARE_FEE)
		{
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_notEnoughShares, 0 };
			LOG_INFO(locals.log);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
		}
		else
		{
			if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, QRAFFLE_TRANSFER_SHARE_FEE) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_transferFailed, 0 };
				LOG_INFO(locals.log);
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
				qpi.transfer(id(QX_CONTRACT_INDEX, 0, 0, 0), QRAFFLE_TRANSFER_SHARE_FEE);
				if (qpi.invocationReward() > QRAFFLE_TRANSFER_SHARE_FEE)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  QRAFFLE_TRANSFER_SHARE_FEE);
				}
				locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_shareManagementRightsTransferred, 0 };
				LOG_INFO(locals.log);
			}
		}
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
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 8);
	}

	INITIALIZE()
	{
		state.qREAmount = 10000000;
		state.charityAddress = ID(_D, _P, _Q, _R, _L, _S, _Z, _S, _S, _C, _X, _I, _Y, _F, _I, _Q, _G, _B, _F, _B, _X, _X, _I, _S, _D, _D, _E, _B, _E, _G, _Q, _N, _W, _N, _T, _Q, _U, _E, _I, _F, _S, _C, _U, _W, _G, _H, _V, _X, _J, _P, _L, _F, _G, _M, _Y, _D);
		state.initialRegister1 = ID(_I, _L, _N, _J, _X, _V, _H, _A, _U, _X, _D, _G, _G, _B, _T, _T, _U, _O, _I, _T, _O, _Q, _G, _P, _A, _Y, _U, _C, _F, _T, _N, _C, _P, _X, _D, _K, _O, _C, _P, _U, _O, _C, _D, _O, _T, _P, _U, _W, _X, _B, _I, _G, _R, _V, _Q, _D);
		state.initialRegister2 = ID(_L, _S, _D, _A, _A, _C, _L, _X, _X, _G, _I, _P, _G, _G, _L, _S, _O, _C, _L, _M, _V, _A, _Y, _L, _N, _T, _G, _D, _V, _B, _N, _O, _S, _S, _Y, _E, _Q, _D, _R, _K, _X, _D, _Y, _W, _B, _C, _G, _J, _I, _K, _C, _M, _Z, _K, _M, _F);
		state.initialRegister3 = ID(_G, _H, _G, _R, _L, _W, _S, _X, _Z, _X, _W, _D, _A, _A, _O, _M, _T, _X, _Q, _Y, _U, _P, _R, _L, _P, _N, _K, _C, _W, _G, _H, _A, _E, _F, _I, _R, _J, _I, _Z, _A, _K, _C, _A, _U, _D, _G, _N, _M, _C, _D, _E, _Q, _R, _O, _Q, _B);
		state.initialRegister4 = ID(_E, _U, _O, _N, _A, _Z, _J, _U, _A, _G, _V, _D, _C, _E, _I, _B, _A, _H, _J, _E, _T, _G, _U, _U, _H, _M, _N, _D, _J, _C, _S, _E, _T, _T, _Q, _V, _G, _Y, _F, _H, _M, _D, _P, _X, _T, _A, _L, _D, _Y, _U, _V, _E, _P, _F, _C, _A);
		state.initialRegister5 = ID(_Q, _W, _H, _L, _C, _V, _S, _Y, _Z, _R, _J, _L, _U, _A, _J, _E, _B, _R, _M, _U, _K, _K, _S, _N, _S, _O, _M, _B, _A, _C, _R, _N, _E, _U, _A, _T, _C, _P, _M, _E, _H, _H, _K, _G, _K, _O, _X, _N, _A, _R, _X, _S, _S, _B, _L, _A);
		state.QXMRIssuer = ID(_Q, _X, _M, _R, _T, _K, _A, _I, _I, _G, _L, _U, _R, _E, _P, _I, _Q, _P, _C, _M, _H, _C, _K, _W, _S, _I, _P, _D, _T, _U, _Y, _F, _C, _F, _N, _Y, _X, _Q, _L, _T, _E, _C, _S, _U, _J, _V, _Y, _E, _M, _M, _D, _E, _L, _B, _M, _D);

		state.registers.set(state.initialRegister1, 0);
		state.registers.set(state.initialRegister2, 0);
		state.registers.set(state.initialRegister3, 0);
		state.registers.set(state.initialRegister4, 0);
		state.registers.set(state.initialRegister5, 0);
		state.numberOfRegisters = 5;
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
		QRAFFLELogger log;
		QRAFFLEEndEpochLogger endEpochLog;
		QRAFFLERevenueLogger revenueLog;
		QRAFFLETokenRaffleLogger tokenRaffleLog;
		QRAFFLEProposalLogger proposalLog;
	};

	END_EPOCH_WITH_LOCALS()
	{
		qpi.distributeDividends(div(state.epochRevenue, 676ull));
		state.epochRevenue -= div(state.epochRevenue, 676ull) * 676;

		locals.digest = qpi.getPrevSpectrumDigest();
		locals.r = (qpi.numberOfTickTransactions() + 1) * locals.digest.u64._0 + (qpi.second()) * locals.digest.u64._1 + locals.digest.u64._2;
		locals.winnerIndex = (uint32)mod(locals.r, state.numberOfQuRaffleMembers * 1ull) + 1;
		locals.winner = state.quRaffleMembers.get(locals.winnerIndex);

		// Calculate fee distributions
		locals.burnAmount = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_BURN_FEE, 100ull);
		locals.charityRevenue = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_CHARITY_FEE, 100ull);
		locals.shareholderRevenue = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_SHRAEHOLDER_FEE, 100ull);
		locals.registerRevenue = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_REGISTER_FEE, 100ull);
		locals.fee = div(state.qREAmount * state.numberOfQuRaffleMembers * QRAFFLE_FEE, 100ull);
		locals.winnerRevenue = state.qREAmount * state.numberOfQuRaffleMembers - locals.burnAmount - locals.charityRevenue - div(locals.shareholderRevenue, 676ull) * 676 - div(locals.registerRevenue, state.numberOfRegisters * 1ull) * state.numberOfRegisters - locals.fee;
		
		// Log QXMR revenue adjustment if applicable
		if (state.epochQXMRRevenue > 0)
		{
			locals.winnerRevenue -= 67600;
		}

		// Log detailed revenue distribution information
		locals.revenueLog = QRAFFLERevenueLogger{ 
			QRAFFLE_CONTRACT_INDEX, 
			QRAFFLELogInfo::QRAFFLE_revenueDistributed, 
			locals.burnAmount, 
			locals.charityRevenue, 
			div(locals.shareholderRevenue, 676ull) * 676, 
			div(locals.registerRevenue, state.numberOfRegisters * 1ull) * state.numberOfRegisters, 
			locals.fee, 
			locals.winnerRevenue, 
			0 
		};
		LOG_INFO(locals.revenueLog);

		// Execute transfers and log each distribution
		qpi.transfer(locals.winner, locals.winnerRevenue);
		qpi.burn(locals.burnAmount);
		qpi.transfer(state.charityAddress, locals.charityRevenue);
		qpi.distributeDividends(div(locals.shareholderRevenue, 676ull));
		qpi.transfer(state.feeAddress, locals.fee);

		// Update total amounts and log largest winner update
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

		// Store QuRaffle results and log completion with detailed information
		locals.qraffle.epochWinner = locals.winner;
		locals.qraffle.receivedAmount = locals.winnerRevenue;
		locals.qraffle.entryAmount = state.qREAmount;
		locals.qraffle.numberOfMembers = state.numberOfQuRaffleMembers;
		locals.qraffle.winnerIndex = locals.winnerIndex;
		state.QuRaffles.set(qpi.epoch(), locals.qraffle);

		// Log QuRaffle completion with detailed information
		locals.endEpochLog = QRAFFLEEndEpochLogger{ 
			QRAFFLE_CONTRACT_INDEX, 
			QRAFFLELogInfo::QRAFFLE_revenueDistributed, 
			qpi.epoch(), 
			state.numberOfQuRaffleMembers, 
			state.qREAmount * state.numberOfQuRaffleMembers, 
			locals.winnerRevenue, 
			locals.winnerIndex, 
			0 
		};
		LOG_INFO(locals.endEpochLog);

		// Process QRAFFLE asset shareholders and log
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

		locals.idx = state.shareholdersList.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.shareholder = state.shareholdersList.key(locals.idx);
			qpi.transferShareOwnershipAndPossession(QXMR_ASSET_NAME, state.QXMRIssuer, SELF, SELF, div(state.epochQXMRRevenue, 676ULL), locals.shareholder);
			locals.idx = state.shareholdersList.nextElementIndex(locals.idx);
		}
		state.epochQXMRRevenue -= div(state.epochQXMRRevenue, 676ULL) * 676;
		
		// Reset QuRaffle members and log
		state.numberOfQuRaffleMembers = 0;

		// Process each active token raffle and log
		for (locals.i = 0 ; locals.i < state.numberOfActiveTokenRaffle; locals.i++)
		{
			locals.winnerIndex = (uint32)mod(locals.r, state.numberOfTokenRaffleMembers.get(locals.i) * 1ull) + 1;
			state.tokenRaffleMembers.get(locals.i, state.tmpTokenRaffleMembers);
			locals.winner = state.tmpTokenRaffleMembers.get(locals.winnerIndex);

			locals.acTokenRaffle = state.activeTokenRaffle.get(locals.i);

			// Calculate token raffle fee distributions
			locals.burnAmount = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_BURN_FEE, 100ull);
			locals.charityRevenue = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_CHARITY_FEE, 100ull);
			locals.shareholderRevenue = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_SHRAEHOLDER_FEE, 100ull);
			locals.registerRevenue = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_REGISTER_FEE, 100ull);
			locals.fee = div(locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_FEE, 100ull);
			locals.winnerRevenue = locals.acTokenRaffle.entryAmount * state.numberOfTokenRaffleMembers.get(locals.i) - locals.burnAmount - locals.charityRevenue - div(locals.shareholderRevenue, 676ULL) * 676 - div(locals.registerRevenue, state.numberOfRegisters * 1ull) * state.numberOfRegisters - locals.fee;
			
			// Execute token transfers and log each
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.winnerRevenue, locals.winner);
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.burnAmount, NULL_ID);
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.charityRevenue, state.charityAddress);
			qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.fee, state.feeAddress);

			locals.idx = state.shareholdersList.nextElementIndex(NULL_INDEX);
			while (locals.idx != NULL_INDEX)
			{
				locals.shareholder = state.shareholdersList.key(locals.idx);
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, div(locals.shareholderRevenue, 676ULL), locals.shareholder);
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
			state.tokenRaffle.set(state.numberOfEndedTokenRaffle, locals.tRaffle);

			// Log token raffle ended with detailed information
			locals.tokenRaffleLog = QRAFFLETokenRaffleLogger{ 
				QRAFFLE_CONTRACT_INDEX, 
				QRAFFLELogInfo::QRAFFLE_tokenRaffleEnded, 
				state.numberOfEndedTokenRaffle++, 
				locals.acTokenRaffle.token.assetName, 
				state.numberOfTokenRaffleMembers.get(locals.i), 
				locals.acTokenRaffle.entryAmount, 
				locals.winnerIndex, 
				locals.winnerRevenue, 
				0
			};
			LOG_INFO(locals.tokenRaffleLog);

			state.numberOfTokenRaffleMembers.set(locals.i, 0);
		}

		// Calculate new qREAmount and log
		locals.log = QRAFFLELogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLELogInfo::QRAFFLE_revenueDistributed, 0 };
		LOG_INFO(locals.log);

		locals.sumOfEntryAmountSubmitted = 0;
		locals.idx = state.quRaffleEntryAmount.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.sumOfEntryAmountSubmitted += state.quRaffleEntryAmount.value(locals.idx);
			locals.idx = state.quRaffleEntryAmount.nextElementIndex(locals.idx);
		}
		state.qREAmount = div(locals.sumOfEntryAmountSubmitted, state.numberOfEntryAmountSubmitted * 1ull);

		// Process approved proposals and create new token raffles
		for (locals.i = 0 ; locals.i < state.numberOfProposals; locals.i++)
		{
			locals.proposal = state.proposals.get(locals.i);
			
			// Log proposal processing with detailed information
			locals.proposalLog = QRAFFLEProposalLogger{ 
				QRAFFLE_CONTRACT_INDEX, 
				QRAFFLELogInfo::QRAFFLE_proposalSubmitted, 
				locals.i, 
				locals.proposal.nYes, 
				locals.proposal.nNo, 
				locals.proposal.token.assetName, 
				locals.proposal.entryAmount, 
				0 
			};
			LOG_INFO(locals.proposalLog);

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
