using namespace QPI;

constexpr uint64 QRAFFLE_REGISTER_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_QXMR_REGISTER_AMOUNT = 100000000ull;
constexpr uint64 QRAFFLE_MAX_QRE_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_ASSET_NAME = 19505638103142993;
constexpr uint64 QRAFFLE_QXMR_ASSET_NAME = 1380800593; // QXMR token asset name
constexpr uint32 QRAFFLE_LOGOUT_FEE = 50000000;
constexpr uint32 QRAFFLE_QXMR_LOGOUT_FEE = 5000000; // QXMR logout fee
constexpr uint32 QRAFFLE_TRANSFER_SHARE_FEE = 100;
constexpr uint32 QRAFFLE_BURN_FEE = 10; // percent
constexpr uint32 QRAFFLE_REGISTER_FEE = 5; // percent
constexpr uint32 QRAFFLE_FEE = 1; // percent
constexpr uint32 QRAFFLE_CHARITY_FEE = 1; // percent
constexpr uint32 QRAFFLE_SHRAEHOLDER_FEE = 3; // percent
constexpr uint32 QRAFFLE_MAX_EPOCH = 65536;
constexpr uint32 QRAFFLE_MAX_PROPOSAL_EPOCH = 128;
constexpr uint32 QRAFFLE_MAX_MEMBER = 65536;
constexpr uint32 QRAFFLE_DEFAULT_QRAFFLE_AMOUNT = 10000000ull;
constexpr uint32 QRAFFLE_MIN_QRAFFLE_AMOUNT = 1000000ull;
constexpr uint32 QRAFFLE_MAX_QRAFFLE_AMOUNT = 1000000000ull;

constexpr sint32 QRAFFLE_SUCCESS = 0;
constexpr sint32 QRAFFLE_INSUFFICIENT_FUND = 1;
constexpr sint32 QRAFFLE_ALREADY_REGISTERED = 2;
constexpr sint32 QRAFFLE_UNREGISTERED = 3;
constexpr sint32 QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED = 4;
constexpr sint32 QRAFFLE_INVALID_PROPOSAL = 5;
constexpr sint32 QRAFFLE_FAILED_TO_DEPOSIT = 6;
constexpr sint32 QRAFFLE_ALREADY_VOTED = 7;
constexpr sint32 QRAFFLE_INVALID_TOKEN_RAFFLE = 8;
constexpr sint32 QRAFFLE_INVALID_OFFSET_OR_LIMIT = 9;
constexpr sint32 QRAFFLE_INVALID_EPOCH = 10;
constexpr sint32 QRAFFLE_MAX_MEMBER_REACHED = 11;
constexpr sint32 QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT = 12;
constexpr sint32 QRAFFLE_INSUFFICIENT_QXMR = 13;
constexpr sint32 QRAFFLE_INVALID_TOKEN_TYPE = 14;
constexpr sint32 QRAFFLE_USER_NOT_FOUND = 15;
constexpr sint32 QRAFFLE_INVALID_ENTRY_AMOUNT = 16;
constexpr sint32 QRAFFLE_EMPTY_QU_RAFFLE = 17;
constexpr sint32 QRAFFLE_EMPTY_TOKEN_RAFFLE = 18;

struct QRAFFLE2
{
};

struct QRAFFLE : public ContractBase
{
public:
	enum LogInfo {
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
		QRAFFLE_shareManagementRightsTransferred = 30,
		QRAFFLE_emptyQuRaffle = 31,
		QRAFFLE_emptyTokenRaffle = 32
	};

	struct Logger
	{
		uint32 _contractIndex;
		uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	// Enhanced logger for END_EPOCH with detailed information
	struct EndEpochLogger
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
	struct RevenueLogger
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
	struct TokenRaffleLogger
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
	struct ProposalLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _proposalIndex; // Index of the proposal
		id _proposer; // Proposer of the proposal
		uint32 _yesVotes; // Number of yes votes
		uint32 _noVotes; // Number of no votes
		uint64 _assetName; // Asset name if approved
		uint64 _entryAmount; // Entry amount if approved
		sint8 _terminator;
	};

	struct EmptyTokenRaffleLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _tokenRaffleIndex; // Index of the token raffle per epoch
		sint8 _terminator;
	};

	struct ProposalInfo {
		Asset token;
		id proposer;
		uint64 entryAmount;
		uint32 nYes;
		uint32 nNo;
	};

	struct VotedId {
		id user;
		bit status;
	};

	struct QuRaffleInfo
	{
		id epochWinner;
		uint64 receivedAmount;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
	};

	struct TokenRaffleInfo
	{
		id epochWinner;
		Asset token;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		uint32 epoch;
	};

	struct ActiveTokenRaffleInfo {
		Asset token;
		uint64 entryAmount;
	};

	struct StateData
	{
		HashMap <id, uint8, QRAFFLE_MAX_MEMBER> registers;

		Array <ProposalInfo, QRAFFLE_MAX_PROPOSAL_EPOCH> proposals;

		HashMap <uint32, Array <VotedId, QRAFFLE_MAX_MEMBER>, QRAFFLE_MAX_PROPOSAL_EPOCH> voteStatus;
		Array <VotedId, QRAFFLE_MAX_MEMBER> tmpVoteStatus;
		Array <uint32, QRAFFLE_MAX_PROPOSAL_EPOCH> numberOfVotedInProposal;
		Array <id, QRAFFLE_MAX_MEMBER> quRaffleMembers;

		Array <ActiveTokenRaffleInfo, QRAFFLE_MAX_PROPOSAL_EPOCH> activeTokenRaffle;
		HashMap <uint32, Array <id, QRAFFLE_MAX_MEMBER>, QRAFFLE_MAX_PROPOSAL_EPOCH> tokenRaffleMembers;
		Array <uint32, QRAFFLE_MAX_PROPOSAL_EPOCH> numberOfTokenRaffleMembers;
		Array <id, QRAFFLE_MAX_MEMBER> tmpTokenRaffleMembers;

		Array <QuRaffleInfo, QRAFFLE_MAX_EPOCH> QuRaffles;
		Array <TokenRaffleInfo, 1048576> tokenRaffle;
		HashMap <id, uint64, QRAFFLE_MAX_MEMBER> quRaffleEntryAmount;
		HashSet <id, 1024> shareholdersList;

		id initialRegister1, initialRegister2, initialRegister3, initialRegister4, initialRegister5;
		id charityAddress, feeAddress, QXMRIssuer;
		uint64 epochRevenue, epochQXMRRevenue, qREAmount, totalBurnAmount, totalCharityAmount, totalShareholderAmount, totalRegisterAmount, totalFeeAmount, totalWinnerAmount, largestWinnerAmount;
		uint32 numberOfRegisters, numberOfQuRaffleMembers, numberOfEntryAmountSubmitted, numberOfProposals, numberOfActiveTokenRaffle, numberOfEndedTokenRaffle;
	};

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
		id tokenIssuer;
		uint64 tokenName;
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

	struct depositInQuRaffle_input
	{

	};

	struct depositInQuRaffle_output
	{
		sint32 returnCode;
	};

	struct depositInTokenRaffle_input
	{
		uint32 indexOfTokenRaffle;
	};

	struct depositInTokenRaffle_output
	{
		sint32 returnCode;
	};

	struct TransferShareManagementRights_input
	{
		id tokenIssuer;
		uint64 tokenName;
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
		id register1, register2, register3, register4, register5, register6, register7, register8, register9, register10, register11, register12, register13, register14, register15, register16, register17, register18, register19, register20;
		sint32 returnCode;
	};

	struct getAnalytics_input
	{
	};

	struct getAnalytics_output
	{
		uint64 currentQuRaffleAmount;
		uint64 totalBurnAmount;
		uint64 totalCharityAmount;
		uint64 totalShareholderAmount;
		uint64 totalRegisterAmount;
		uint64 totalFeeAmount;
		uint64 totalWinnerAmount;
		uint64 largestWinnerAmount;
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
		id tokenIssuer;
		id proposer;
		uint64 tokenName;
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
		id tokenIssuer;
		uint64 tokenName;
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
		id tokenIssuer;
		uint64 tokenName;
		uint64 entryAmount;
		uint32 numberOfMembers;
		sint32 returnCode;
	};

	struct getQuRaffleEntryAmountPerUser_input
	{
		id user;
	};

	struct getQuRaffleEntryAmountPerUser_output
	{
		uint64 entryAmount;
		sint32 returnCode;
	};

	struct getQuRaffleEntryAverageAmount_input
	{
	};

	struct getQuRaffleEntryAverageAmount_output
	{
		uint64 entryAverageAmount;
		sint32 returnCode;
	};

protected:

	struct registerInSystem_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(registerInSystem)
	{
		if (state.get().registers.contains(qpi.invocator()))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_ALREADY_REGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyRegistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().numberOfRegisters >= QRAFFLE_MAX_MEMBER)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxMemberReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if (input.useQXMR)
		{
			// refund the invocation reward if the user uses QXMR for registration
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			// Use QXMR tokens for registration
			if (qpi.numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < QRAFFLE_QXMR_REGISTER_AMOUNT)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			// Transfer QXMR tokens to the contract
			if (qpi.transferShareOwnershipAndPossession(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, qpi.invocator(), qpi.invocator(), QRAFFLE_QXMR_REGISTER_AMOUNT, SELF) < 0)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			state.mut().registers.set(qpi.invocator(), 2);
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
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_REGISTER_AMOUNT);
			state.mut().registers.set(qpi.invocator(), 1);
		}

		state.mut().numberOfRegisters++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct logoutInSystem_locals
	{
		sint64 refundAmount;
		uint8 tokenType;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(logoutInSystem)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (qpi.invocator() == state.get().initialRegister1 || qpi.invocator() == state.get().initialRegister2 || qpi.invocator() == state.get().initialRegister3 || qpi.invocator() == state.get().initialRegister4 || qpi.invocator() == state.get().initialRegister5)
		{
			output.returnCode = QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_initialRegisterCannotLogout, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.get().registers.get(qpi.invocator(), locals.tokenType);

		if (locals.tokenType == 1)
		{
			// Use qubic for logout
			locals.refundAmount = QRAFFLE_REGISTER_AMOUNT - QRAFFLE_LOGOUT_FEE;
			qpi.transfer(qpi.invocator(), locals.refundAmount);
			state.mut().epochRevenue += QRAFFLE_LOGOUT_FEE;
		}
		else if (locals.tokenType == 2)
		{
			// Use QXMR tokens for logout
			locals.refundAmount = QRAFFLE_QXMR_REGISTER_AMOUNT - QRAFFLE_QXMR_LOGOUT_FEE;

			// Check if contract has enough QXMR tokens
			if (qpi.numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX) < locals.refundAmount)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			// Transfer QXMR tokens back to user
			if (qpi.transferShareOwnershipAndPossession(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, locals.refundAmount, qpi.invocator()) < 0)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			state.mut().epochQXMRRevenue += QRAFFLE_QXMR_LOGOUT_FEE;
		}

		state.mut().registers.removeByKey(qpi.invocator());
		state.mut().numberOfRegisters--;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct submitEntryAmount_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitEntryAmount)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (input.amount < QRAFFLE_MIN_QRAFFLE_AMOUNT || input.amount > QRAFFLE_MAX_QRAFFLE_AMOUNT)
		{
			output.returnCode = QRAFFLE_INVALID_ENTRY_AMOUNT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidEntryAmount, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().quRaffleEntryAmount.contains(qpi.invocator()) == 0)
		{
			state.mut().numberOfEntryAmountSubmitted++;
		}
		state.mut().quRaffleEntryAmount.set(qpi.invocator(), input.amount);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct submitProposal_locals
	{
		ProposalInfo proposal;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitProposal)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().numberOfProposals >= QRAFFLE_MAX_PROPOSAL_EPOCH)
		{
			output.returnCode = QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxProposalEpochReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.proposal.token.issuer = input.tokenIssuer;
		locals.proposal.token.assetName = input.tokenName;
		locals.proposal.entryAmount = input.entryAmount;
		locals.proposal.proposer = qpi.invocator();
		state.mut().proposals.set(state.get().numberOfProposals, locals.proposal);
		state.mut().numberOfProposals++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalSubmitted, 0 };
		LOG_INFO(locals.log);
	}

	struct voteInProposal_locals
	{
		ProposalInfo proposal;
		VotedId votedId;
		uint32 i;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(voteInProposal)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.indexOfProposal >= state.get().numberOfProposals)
		{
			output.returnCode = QRAFFLE_INVALID_PROPOSAL;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalNotFound, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.proposal = state.get().proposals.get(input.indexOfProposal);
		state.get().voteStatus.get(input.indexOfProposal, state.mut().tmpVoteStatus);
		for (locals.i = 0; locals.i < state.get().numberOfVotedInProposal.get(input.indexOfProposal); locals.i++)
		{
			if (state.get().tmpVoteStatus.get(locals.i).user == qpi.invocator())
			{
				if (state.get().tmpVoteStatus.get(locals.i).status == input.yes)
				{
					output.returnCode = QRAFFLE_ALREADY_VOTED;
					locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyVoted, 0 };
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
					state.mut().proposals.set(input.indexOfProposal, locals.proposal);
				}

				locals.votedId.user = qpi.invocator();
				locals.votedId.status = input.yes;
				state.mut().tmpVoteStatus.set(locals.i, locals.votedId);
				state.mut().voteStatus.set(input.indexOfProposal, state.get().tmpVoteStatus);
				output.returnCode = QRAFFLE_SUCCESS;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalVoted, 0 };
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
		state.mut().proposals.set(input.indexOfProposal, locals.proposal);

		locals.votedId.user = qpi.invocator();
		locals.votedId.status = input.yes;
		state.mut().tmpVoteStatus.set(state.get().numberOfVotedInProposal.get(input.indexOfProposal), locals.votedId);
		state.mut().voteStatus.set(input.indexOfProposal, state.get().tmpVoteStatus);
		state.mut().numberOfVotedInProposal.set(input.indexOfProposal, state.get().numberOfVotedInProposal.get(input.indexOfProposal) + 1);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalVoted, 0 };
		LOG_INFO(locals.log);
	}

	struct depositInQuRaffle_locals
	{
		uint32 i;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositInQuRaffle)
	{
		if (state.get().numberOfQuRaffleMembers >= QRAFFLE_MAX_MEMBER)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxMemberReachedForQuRaffle, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (qpi.invocationReward() < (sint64)state.get().qREAmount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		for (locals.i = 0 ; locals.i < state.get().numberOfQuRaffleMembers; locals.i++)
		{
			if (state.get().quRaffleMembers.get(locals.i) == qpi.invocator())
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = QRAFFLE_ALREADY_REGISTERED;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyRegistered, 0 };
				LOG_INFO(locals.log);
				return ;
			}
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().qREAmount);
		state.mut().quRaffleMembers.set(state.get().numberOfQuRaffleMembers, qpi.invocator());
		state.mut().numberOfQuRaffleMembers++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_quRaffleDeposited, 0 };
		LOG_INFO(locals.log);
	}

	struct depositInTokenRaffle_locals
	{
		uint32 i;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositInTokenRaffle)
	{
		if (qpi.invocationReward() < QRAFFLE_TRANSFER_SHARE_FEE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.indexOfTokenRaffle >= state.get().numberOfActiveTokenRaffle)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidTokenRaffle, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (qpi.transferShareOwnershipAndPossession(state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).token.assetName, state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).token.issuer, qpi.invocator(), qpi.invocator(), state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount, SELF) < 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_FAILED_TO_DEPOSIT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_transferFailed, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_TRANSFER_SHARE_FEE);
		state.get().tokenRaffleMembers.get(input.indexOfTokenRaffle, state.mut().tmpTokenRaffleMembers);
		state.mut().tmpTokenRaffleMembers.set(state.get().numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle), qpi.invocator());
		state.mut().numberOfTokenRaffleMembers.set(input.indexOfTokenRaffle, state.get().numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle) + 1);
		state.mut().tokenRaffleMembers.set(input.indexOfTokenRaffle, state.get().tmpTokenRaffleMembers);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_tokenRaffleDeposited, 0 };
		LOG_INFO(locals.log);
	}

	struct TransferShareManagementRights_locals
	{
		Asset asset;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < QRAFFLE_TRANSFER_SHARE_FEE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if (qpi.numberOfPossessedShares(input.tokenName, input.tokenIssuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_notEnoughShares, 0 };
			LOG_INFO(locals.log);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
		}
		else
		{
			locals.asset.assetName = input.tokenName;
			locals.asset.issuer = input.tokenIssuer;
			if (qpi.releaseShares(locals.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, QRAFFLE_TRANSFER_SHARE_FEE) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_transferFailed, 0 };
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
				if (qpi.invocationReward() > QRAFFLE_TRANSFER_SHARE_FEE)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  QRAFFLE_TRANSFER_SHARE_FEE);
				}
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_shareManagementRightsTransferred, 0 };
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
		if (input.limit > 20)
		{
			output.returnCode = QRAFFLE_INVALID_OFFSET_OR_LIMIT;
			return ;
		}
		if (input.offset + input.limit > state.get().numberOfRegisters)
		{
			output.returnCode = QRAFFLE_INVALID_OFFSET_OR_LIMIT;
			return ;
		}
		locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.user = state.get().registers.key(locals.idx);
			if (locals.i >= input.offset && locals.i < input.offset + input.limit)
			{
				if (locals.i - input.offset == 0)
				{
					output.register1 = locals.user;
				}
				else if (locals.i - input.offset == 1)
				{
					output.register2 = locals.user;
				}
				else if (locals.i - input.offset == 2)
				{
					output.register3 = locals.user;
				}
				else if (locals.i - input.offset == 3)
				{
					output.register4 = locals.user;
				}
				else if (locals.i - input.offset == 4)
				{
					output.register5 = locals.user;
				}
				else if (locals.i - input.offset == 5)
				{
					output.register6 = locals.user;
				}
				else if (locals.i - input.offset == 6)
				{
					output.register7 = locals.user;
				}
				else if (locals.i - input.offset == 7)
				{
					output.register8 = locals.user;
				}
				else if (locals.i - input.offset == 8)
				{
					output.register9 = locals.user;
				}
				else if (locals.i - input.offset == 9)
				{
					output.register10 = locals.user;
				}
				else if (locals.i - input.offset == 10)
				{
					output.register11 = locals.user;
				}
				else if (locals.i - input.offset == 11)
				{
					output.register12 = locals.user;
				}
				else if (locals.i - input.offset == 12)
				{
					output.register13 = locals.user;
				}
				else if (locals.i - input.offset == 13)
				{
					output.register14 = locals.user;
				}
				else if (locals.i - input.offset == 14)
				{
					output.register15 = locals.user;
				}
				else if (locals.i - input.offset == 15)
				{
					output.register16 = locals.user;
				}
				else if (locals.i - input.offset == 16)
				{
					output.register17 = locals.user;
				}
				else if (locals.i - input.offset == 17)
				{
					output.register18 = locals.user;
				}
				else if (locals.i - input.offset == 18)
				{
					output.register19 = locals.user;
				}
				else if (locals.i - input.offset == 19)
				{
					output.register20 = locals.user;
				}
			}
			if (locals.i >= input.offset + input.limit)
			{
				break;
			}
			locals.i++;
			locals.idx = state.get().registers.nextElementIndex(locals.idx);
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getAnalytics)
	{
		output.currentQuRaffleAmount = state.get().qREAmount;
		output.totalBurnAmount = state.get().totalBurnAmount;
		output.totalCharityAmount = state.get().totalCharityAmount;
		output.totalShareholderAmount = state.get().totalShareholderAmount;
		output.totalRegisterAmount = state.get().totalRegisterAmount;
		output.totalFeeAmount = state.get().totalFeeAmount;
		output.totalWinnerAmount = state.get().totalWinnerAmount;
		output.largestWinnerAmount = state.get().largestWinnerAmount;
		output.numberOfRegisters = state.get().numberOfRegisters;
		output.numberOfProposals = state.get().numberOfProposals;
		output.numberOfQuRaffleMembers = state.get().numberOfQuRaffleMembers;
		output.numberOfActiveTokenRaffle = state.get().numberOfActiveTokenRaffle;
		output.numberOfEndedTokenRaffle = state.get().numberOfEndedTokenRaffle;
		output.numberOfEntryAmountSubmitted = state.get().numberOfEntryAmountSubmitted;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getActiveProposal)
	{
		if (input.indexOfProposal >= state.get().numberOfProposals)
		{
			output.returnCode = QRAFFLE_INVALID_PROPOSAL;
			return ;
		}
		output.tokenName = state.get().proposals.get(input.indexOfProposal).token.assetName;
		output.tokenIssuer = state.get().proposals.get(input.indexOfProposal).token.issuer;
		output.proposer = state.get().proposals.get(input.indexOfProposal).proposer;
		output.entryAmount = state.get().proposals.get(input.indexOfProposal).entryAmount;
		output.nYes = state.get().proposals.get(input.indexOfProposal).nYes;
		output.nNo = state.get().proposals.get(input.indexOfProposal).nNo;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getEndedTokenRaffle)
	{
		if (input.indexOfRaffle >= state.get().numberOfEndedTokenRaffle)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			return ;
		}
		output.epochWinner = state.get().tokenRaffle.get(input.indexOfRaffle).epochWinner;
		output.tokenName = state.get().tokenRaffle.get(input.indexOfRaffle).token.assetName;
		output.tokenIssuer = state.get().tokenRaffle.get(input.indexOfRaffle).token.issuer;
		output.entryAmount = state.get().tokenRaffle.get(input.indexOfRaffle).entryAmount;
		output.numberOfMembers = state.get().tokenRaffle.get(input.indexOfRaffle).numberOfMembers;
		output.winnerIndex = state.get().tokenRaffle.get(input.indexOfRaffle).winnerIndex;
		output.epoch = state.get().tokenRaffle.get(input.indexOfRaffle).epoch;
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
			output.EndIndex = state.get().numberOfActiveTokenRaffle;
			return ;
		}
		for (locals.i = 0; locals.i < (sint32)state.get().numberOfEndedTokenRaffle; locals.i++)
		{
			if (state.get().tokenRaffle.get(locals.i).epoch == input.epoch)
			{
				output.StartIndex = locals.i;
				break;
			}
		}
		for (locals.i = (sint32)state.get().numberOfEndedTokenRaffle - 1; locals.i >= 0; locals.i--)
		{
			if (state.get().tokenRaffle.get(locals.i).epoch == input.epoch)
			{
				output.EndIndex = locals.i;
				break;
			}
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getEndedQuRaffle)
	{
		output.epochWinner = state.get().QuRaffles.get(input.epoch).epochWinner;
		output.receivedAmount = state.get().QuRaffles.get(input.epoch).receivedAmount;
		output.entryAmount = state.get().QuRaffles.get(input.epoch).entryAmount;
		output.numberOfMembers = state.get().QuRaffles.get(input.epoch).numberOfMembers;
		output.winnerIndex = state.get().QuRaffles.get(input.epoch).winnerIndex;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getActiveTokenRaffle)
	{
		if (input.indexOfTokenRaffle >= state.get().numberOfActiveTokenRaffle)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			return ;
		}
		output.tokenName = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).token.assetName;
		output.tokenIssuer = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).token.issuer;
		output.entryAmount = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount;
		output.numberOfMembers = state.get().numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle);
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getQuRaffleEntryAmountPerUser)
	{
		if (state.get().quRaffleEntryAmount.contains(input.user) == 0)
		{
			output.entryAmount = 0;
			output.returnCode = QRAFFLE_USER_NOT_FOUND;
		}
		else
		{
			state.get().quRaffleEntryAmount.get(input.user, output.entryAmount);
			output.returnCode = QRAFFLE_SUCCESS;
		}
	}

	struct getQuRaffleEntryAverageAmount_locals
	{
		uint64 entryAmount;
		uint64 totalEntryAmount;
		sint64 idx;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getQuRaffleEntryAverageAmount)
	{
		locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.totalEntryAmount += state.get().quRaffleEntryAmount.value(locals.idx);
			locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(locals.idx);
		}
		if (state.get().numberOfEntryAmountSubmitted > 0)
		{
			output.entryAverageAmount = div<uint64>(locals.totalEntryAmount, state.get().numberOfEntryAmountSubmitted);
		}
		else
		{
			output.entryAverageAmount = 0;
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(getRegisters, 1);
		REGISTER_USER_FUNCTION(getAnalytics, 2);
		REGISTER_USER_FUNCTION(getActiveProposal, 3);
		REGISTER_USER_FUNCTION(getEndedTokenRaffle, 4);
		REGISTER_USER_FUNCTION(getEndedQuRaffle, 5);
		REGISTER_USER_FUNCTION(getActiveTokenRaffle, 6);
		REGISTER_USER_FUNCTION(getEpochRaffleIndexes, 7);
		REGISTER_USER_FUNCTION(getQuRaffleEntryAmountPerUser, 8);
		REGISTER_USER_FUNCTION(getQuRaffleEntryAverageAmount, 9);

		REGISTER_USER_PROCEDURE(registerInSystem, 1);
		REGISTER_USER_PROCEDURE(logoutInSystem, 2);
		REGISTER_USER_PROCEDURE(submitEntryAmount, 3);
		REGISTER_USER_PROCEDURE(submitProposal, 4);
		REGISTER_USER_PROCEDURE(voteInProposal, 5);
		REGISTER_USER_PROCEDURE(depositInQuRaffle, 6);
		REGISTER_USER_PROCEDURE(depositInTokenRaffle, 7);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 8);
	}

	INITIALIZE()
	{
		state.mut().qREAmount = QRAFFLE_DEFAULT_QRAFFLE_AMOUNT;
		state.mut().charityAddress = ID(_D, _P, _Q, _R, _L, _S, _Z, _S, _S, _C, _X, _I, _Y, _F, _I, _Q, _G, _B, _F, _B, _X, _X, _I, _S, _D, _D, _E, _B, _E, _G, _Q, _N, _W, _N, _T, _Q, _U, _E, _I, _F, _S, _C, _U, _W, _G, _H, _V, _X, _J, _P, _L, _F, _G, _M, _Y, _D);
		state.mut().initialRegister1 = ID(_I, _L, _N, _J, _X, _V, _H, _A, _U, _X, _D, _G, _G, _B, _T, _T, _U, _O, _I, _T, _O, _Q, _G, _P, _A, _Y, _U, _C, _F, _T, _N, _C, _P, _X, _D, _K, _O, _C, _P, _U, _O, _C, _D, _O, _T, _P, _U, _W, _X, _B, _I, _G, _R, _V, _Q, _D);
		state.mut().initialRegister2 = ID(_L, _S, _D, _A, _A, _C, _L, _X, _X, _G, _I, _P, _G, _G, _L, _S, _O, _C, _L, _M, _V, _A, _Y, _L, _N, _T, _G, _D, _V, _B, _N, _O, _S, _S, _Y, _E, _Q, _D, _R, _K, _X, _D, _Y, _W, _B, _C, _G, _J, _I, _K, _C, _M, _Z, _K, _M, _F);
		state.mut().initialRegister3 = ID(_G, _H, _G, _R, _L, _W, _S, _X, _Z, _X, _W, _D, _A, _A, _O, _M, _T, _X, _Q, _Y, _U, _P, _R, _L, _P, _N, _K, _C, _W, _G, _H, _A, _E, _F, _I, _R, _J, _I, _Z, _A, _K, _C, _A, _U, _D, _G, _N, _M, _C, _D, _E, _Q, _R, _O, _Q, _B);
		state.mut().initialRegister4 = ID(_E, _U, _O, _N, _A, _Z, _J, _U, _A, _G, _V, _D, _C, _E, _I, _B, _A, _H, _J, _E, _T, _G, _U, _U, _H, _M, _N, _D, _J, _C, _S, _E, _T, _T, _Q, _V, _G, _Y, _F, _H, _M, _D, _P, _X, _T, _A, _L, _D, _Y, _U, _V, _E, _P, _F, _C, _A);
		state.mut().initialRegister5 = ID(_S, _L, _C, _J, _C, _C, _U, _X, _G, _K, _N, _V, _A, _D, _F, _B, _E, _A, _Y, _V, _L, _S, _O, _B, _Z, _P, _A, _B, _H, _K, _S, _G, _M, _H, _W, _H, _S, _H, _G, _G, _B, _A, _P, _J, _W, _F, _V, _O, _K, _Z, _J, _P, _F, _L, _X, _D);
		state.mut().QXMRIssuer = ID(_Q, _X, _M, _R, _T, _K, _A, _I, _I, _G, _L, _U, _R, _E, _P, _I, _Q, _P, _C, _M, _H, _C, _K, _W, _S, _I, _P, _D, _T, _U, _Y, _F, _C, _F, _N, _Y, _X, _Q, _L, _T, _E, _C, _S, _U, _J, _V, _Y, _E, _M, _M, _D, _E, _L, _B, _M, _D);
		state.mut().feeAddress = ID(_H, _H, _R, _L, _C, _Z, _Q, _V, _G, _O, _M, _G, _X, _G, _F, _P, _H, _T, _R, _H, _H, _D, _W, _A, _E, _U, _X, _C, _N, _D, _L, _Z, _S, _Z, _J, _R, _M, _O, _R, _J, _K, _A, _I, _W, _S, _U, _Y, _R, _N, _X, _I, _H, _H, _O, _W, _D);

		state.mut().registers.set(state.get().initialRegister1, 0);
		state.mut().registers.set(state.get().initialRegister2, 0);
		state.mut().registers.set(state.get().initialRegister3, 0);
		state.mut().registers.set(state.get().initialRegister4, 0);
		state.mut().registers.set(state.get().initialRegister5, 0);
		state.mut().numberOfRegisters = 5;
	}

	struct END_EPOCH_locals
	{
		ProposalInfo proposal;
		QuRaffleInfo qraffle;
		TokenRaffleInfo tRaffle;
		ActiveTokenRaffleInfo acTokenRaffle;
		AssetPossessionIterator iter;
		Asset QraffleAsset;
		id digest, winner, shareholder;
		sint64 idx;
		uint64 sumOfEntryAmountSubmitted, r, winnerRevenue, burnAmount, charityRevenue, shareholderRevenue, registerRevenue, fee, oneShareholderRev;
		uint32 i, j, winnerIndex;
		Logger log;
		EmptyTokenRaffleLogger emptyTokenRafflelog;
		EndEpochLogger endEpochLog;
		RevenueLogger revenueLog;
		TokenRaffleLogger tokenRaffleLog;
		ProposalLogger proposalLog;
	};

	END_EPOCH_WITH_LOCALS()
	{
		locals.oneShareholderRev = div<uint64>(state.get().epochRevenue, 676);
		qpi.distributeDividends(locals.oneShareholderRev);
		state.mut().epochRevenue -= locals.oneShareholderRev * 676;

		locals.digest = qpi.getPrevSpectrumDigest();
		locals.r = (qpi.numberOfTickTransactions() + 1) * locals.digest.u64._0 + (qpi.second()) * locals.digest.u64._1 + locals.digest.u64._2;
		locals.winnerIndex = (uint32)mod(locals.r, state.get().numberOfQuRaffleMembers * 1ull);
		locals.winner = state.get().quRaffleMembers.get(locals.winnerIndex);

		// Get QRAFFLE asset shareholders
		locals.QraffleAsset.assetName = QRAFFLE_ASSET_NAME;
		locals.QraffleAsset.issuer = NULL_ID;
		locals.iter.begin(locals.QraffleAsset);
		while (!locals.iter.reachedEnd())
		{
			locals.shareholder = locals.iter.possessor();
			if (state.get().shareholdersList.contains(locals.shareholder) == 0)
			{
				state.mut().shareholdersList.add(locals.shareholder);
			}

			locals.iter.next();
		}

		if (state.get().numberOfQuRaffleMembers > 0)
		{
			// Calculate fee distributions
			locals.burnAmount = div<uint64>(state.get().qREAmount * state.get().numberOfQuRaffleMembers * QRAFFLE_BURN_FEE, 100);
			locals.charityRevenue = div<uint64>(state.get().qREAmount * state.get().numberOfQuRaffleMembers * QRAFFLE_CHARITY_FEE, 100);
			locals.shareholderRevenue = div<uint64>(state.get().qREAmount * state.get().numberOfQuRaffleMembers * QRAFFLE_SHRAEHOLDER_FEE, 100);
			locals.registerRevenue = div<uint64>(state.get().qREAmount * state.get().numberOfQuRaffleMembers * QRAFFLE_REGISTER_FEE, 100);
			locals.fee = div<uint64>(state.get().qREAmount * state.get().numberOfQuRaffleMembers * QRAFFLE_FEE, 100);
			locals.winnerRevenue = state.get().qREAmount * state.get().numberOfQuRaffleMembers - locals.burnAmount - locals.charityRevenue - div<uint64>(locals.shareholderRevenue, 676) * 676 - div<uint64>(locals.registerRevenue, state.get().numberOfRegisters) * state.get().numberOfRegisters - locals.fee;

			// Log detailed revenue distribution information
			locals.revenueLog = RevenueLogger{
				QRAFFLE_CONTRACT_INDEX,
				QRAFFLE_revenueDistributed,
				locals.burnAmount,
				locals.charityRevenue,
				div<uint64>(locals.shareholderRevenue, 676) * 676,
				div<uint64>(locals.registerRevenue, state.get().numberOfRegisters) * state.get().numberOfRegisters,
				locals.fee,
				locals.winnerRevenue,
				0
			};
			LOG_INFO(locals.revenueLog);

			// Execute transfers and log each distribution
			qpi.transfer(locals.winner, locals.winnerRevenue);
			qpi.burn(locals.burnAmount);
			qpi.transfer(state.get().charityAddress, locals.charityRevenue);
			qpi.distributeDividends(div<uint64>(locals.shareholderRevenue, 676));
			qpi.transfer(state.get().feeAddress, locals.fee);

			// Update total amounts and log largest winner update
			state.mut().totalBurnAmount += locals.burnAmount;
			state.mut().totalCharityAmount += locals.charityRevenue;
			state.mut().totalShareholderAmount += div<uint64>(locals.shareholderRevenue, 676) * 676;
			state.mut().totalRegisterAmount += div<uint64>(locals.registerRevenue, state.get().numberOfRegisters) * state.get().numberOfRegisters;
			state.mut().totalFeeAmount += locals.fee;
			state.mut().totalWinnerAmount += locals.winnerRevenue;
			if (locals.winnerRevenue > state.get().largestWinnerAmount)
			{
				state.mut().largestWinnerAmount = locals.winnerRevenue;
			}

			locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
			while (locals.idx != NULL_INDEX)
			{
				qpi.transfer(state.get().registers.key(locals.idx), div<uint64>(locals.registerRevenue, state.get().numberOfRegisters));
				locals.idx = state.get().registers.nextElementIndex(locals.idx);
			}

			// Store QuRaffle results and log completion with detailed information
			locals.qraffle.epochWinner = locals.winner;
			locals.qraffle.receivedAmount = locals.winnerRevenue;
			locals.qraffle.entryAmount = state.get().qREAmount;
			locals.qraffle.numberOfMembers = state.get().numberOfQuRaffleMembers;
			locals.qraffle.winnerIndex = locals.winnerIndex;
			state.mut().QuRaffles.set(qpi.epoch(), locals.qraffle);

			// Log QuRaffle completion with detailed information
			locals.endEpochLog = EndEpochLogger{
				QRAFFLE_CONTRACT_INDEX,
				QRAFFLE_revenueDistributed,
				qpi.epoch(),
				state.get().numberOfQuRaffleMembers,
				state.get().qREAmount * state.get().numberOfQuRaffleMembers,
				locals.winnerRevenue,
				locals.winnerIndex,
				0
			};
			LOG_INFO(locals.endEpochLog);

			if (state.get().epochQXMRRevenue >= 676)
			{
				locals.idx = state.get().shareholdersList.nextElementIndex(NULL_INDEX);
				while (locals.idx != NULL_INDEX)
				{
					locals.shareholder = state.get().shareholdersList.key(locals.idx);
					qpi.transferShareOwnershipAndPossession(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, div<uint64>(state.get().epochQXMRRevenue, 676) * qpi.numberOfShares(locals.QraffleAsset, AssetOwnershipSelect::byOwner(locals.shareholder), AssetPossessionSelect::byPossessor(locals.shareholder)), locals.shareholder);
					locals.idx = state.get().shareholdersList.nextElementIndex(locals.idx);
				}
				state.mut().epochQXMRRevenue -= div<uint64>(state.get().epochQXMRRevenue, 676) * 676;
			}
		}
		else
		{
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_emptyQuRaffle, 0 };
			LOG_INFO(locals.log);
		}

		// Process each active token raffle and log
		for (locals.i = 0 ; locals.i < state.get().numberOfActiveTokenRaffle; locals.i++)
		{
			if (state.get().numberOfTokenRaffleMembers.get(locals.i) > 0)
			{
				locals.winnerIndex = (uint32)mod(locals.r, state.get().numberOfTokenRaffleMembers.get(locals.i) * 1ull);
				state.get().tokenRaffleMembers.get(locals.i, state.mut().tmpTokenRaffleMembers);
				locals.winner = state.get().tmpTokenRaffleMembers.get(locals.winnerIndex);

				locals.acTokenRaffle = state.get().activeTokenRaffle.get(locals.i);

				// Calculate token raffle fee distributions
				locals.burnAmount = div<uint64>(locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_BURN_FEE, 100);
				locals.charityRevenue = div<uint64>(locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_CHARITY_FEE, 100);
				locals.shareholderRevenue = div<uint64>(locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_SHRAEHOLDER_FEE, 100);
				locals.registerRevenue = div<uint64>(locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_REGISTER_FEE, 100);
				locals.fee = div<uint64>(locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i) * QRAFFLE_FEE, 100);
				locals.winnerRevenue = locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i) - locals.burnAmount - locals.charityRevenue - div<uint64>(locals.shareholderRevenue, 676) * 676 - div<uint64>(locals.registerRevenue, state.get().numberOfRegisters) * state.get().numberOfRegisters - locals.fee;

				// Execute token transfers and log each
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.winnerRevenue, locals.winner);
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.burnAmount, NULL_ID);
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.charityRevenue, state.get().charityAddress);
				qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.fee, state.get().feeAddress);

				locals.idx = state.get().shareholdersList.nextElementIndex(NULL_INDEX);
				while (locals.idx != NULL_INDEX)
				{
					locals.shareholder = state.get().shareholdersList.key(locals.idx);
					qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, div<uint64>(locals.shareholderRevenue, 676) * qpi.numberOfShares(locals.QraffleAsset, AssetOwnershipSelect::byOwner(locals.shareholder), AssetPossessionSelect::byPossessor(locals.shareholder)), locals.shareholder);
					locals.idx = state.get().shareholdersList.nextElementIndex(locals.idx);
				}

				locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
				while (locals.idx != NULL_INDEX)
				{
					qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, div<uint64>(locals.registerRevenue, state.get().numberOfRegisters), state.get().registers.key(locals.idx));
					locals.idx = state.get().registers.nextElementIndex(locals.idx);
				}

				locals.tRaffle.epochWinner = locals.winner;
				locals.tRaffle.token.assetName = locals.acTokenRaffle.token.assetName;
				locals.tRaffle.token.issuer = locals.acTokenRaffle.token.issuer;
				locals.tRaffle.entryAmount = locals.acTokenRaffle.entryAmount;
				locals.tRaffle.numberOfMembers = state.get().numberOfTokenRaffleMembers.get(locals.i);
				locals.tRaffle.winnerIndex = locals.winnerIndex;
				locals.tRaffle.epoch = qpi.epoch();
				state.mut().tokenRaffle.set(state.get().numberOfEndedTokenRaffle, locals.tRaffle);

				// Log token raffle ended with detailed information
				locals.tokenRaffleLog = TokenRaffleLogger{
					QRAFFLE_CONTRACT_INDEX,
					QRAFFLE_tokenRaffleEnded,
					state.mut().numberOfEndedTokenRaffle++,
					locals.acTokenRaffle.token.assetName,
					state.get().numberOfTokenRaffleMembers.get(locals.i),
					locals.acTokenRaffle.entryAmount,
					locals.winnerIndex,
					locals.winnerRevenue,
					0
				};
				LOG_INFO(locals.tokenRaffleLog);

				state.mut().numberOfTokenRaffleMembers.set(locals.i, 0);
			}
			else
			{
				locals.emptyTokenRafflelog = EmptyTokenRaffleLogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_emptyTokenRaffle, locals.i, 0 };
				LOG_INFO(locals.emptyTokenRafflelog);
			}
		}

		// Calculate new qREAmount and log
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_revenueDistributed, 0 };
		LOG_INFO(locals.log);

		locals.sumOfEntryAmountSubmitted = 0;
		locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.sumOfEntryAmountSubmitted += state.get().quRaffleEntryAmount.value(locals.idx);
			locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(locals.idx);
		}
		if (state.get().numberOfEntryAmountSubmitted > 0)
		{
			state.mut().qREAmount = div<uint64>(locals.sumOfEntryAmountSubmitted, state.get().numberOfEntryAmountSubmitted);
		}
		else
		{
			state.mut().qREAmount = QRAFFLE_DEFAULT_QRAFFLE_AMOUNT;
		}

		state.mut().numberOfActiveTokenRaffle = 0;

		// Process approved proposals and create new token raffles
		for (locals.i = 0 ; locals.i < state.get().numberOfProposals; locals.i++)
		{
			locals.proposal = state.get().proposals.get(locals.i);

			// Log proposal processing with detailed information
			locals.proposalLog = ProposalLogger{
				QRAFFLE_CONTRACT_INDEX,
				QRAFFLE_proposalSubmitted,
				locals.i,
				locals.proposal.proposer,
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

				state.mut().activeTokenRaffle.set(state.mut().numberOfActiveTokenRaffle++, locals.acTokenRaffle);
			}
		}

		state.mut().numberOfVotedInProposal.setAll(0);
		state.mut().tokenRaffleMembers.reset();
		state.mut().quRaffleEntryAmount.reset();
		state.mut().shareholdersList.reset();
		state.mut().voteStatus.reset();
		state.mut().numberOfEntryAmountSubmitted = 0;
		state.mut().numberOfProposals = 0;
		state.mut().numberOfQuRaffleMembers = 0;
		if (state.get().registers.needsCleanup()) { state.mut().registers.cleanup(); }
	}

	PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
