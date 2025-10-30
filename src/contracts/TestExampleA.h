using namespace QPI;

constexpr uint64 TESTEXA_ASSET_NAME = 18392928276923732;

struct TESTEXA2
{
};

struct TESTEXA : public ContractBase
{
	//---------------------------------------------------------------
	// QPI FUNCTION TESTING

	struct QpiFunctionsOutput
	{
		id arbitrator;
		id computor0;
		id invocator;
		id originator;
		sint64 invocationReward;
		sint32 numberOfTickTransactions;
		uint32 tick;
		uint16 epoch;
		uint16 millisecond;
		uint8 year;   // [0..99] (0 = 2000, 1 = 2001, ..., 99 = 2099)
		uint8 month;  // [1..12]
		uint8 day;    // [1..31]
		uint8 hour;   // [0..23]
		uint8 minute; // [0..59]
		uint8 second;
		uint8 dayOfWeek;
	};

	struct QueryQpiFunctions_input
	{
		Array<sint8, 128> data;
		Array<sint8, 64> signature;
		id entity;
	};
	struct QueryQpiFunctions_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
		id inputDataK12;
		bit inputSignatureValid;
	};

	struct QueryQpiFunctionsToState_input {};
	struct QueryQpiFunctionsToState_output {};

	struct ReturnQpiFunctionsOutputBeginTick_input 
	{
		uint32 tick;
	};
	struct ReturnQpiFunctionsOutputBeginTick_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
	};

	struct ReturnQpiFunctionsOutputEndTick_input
	{
		uint32 tick;
	};
	struct ReturnQpiFunctionsOutputEndTick_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
	};

	struct ReturnQpiFunctionsOutputUserProc_input
	{
		uint32 tick;
	};
	struct ReturnQpiFunctionsOutputUserProc_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
	};

protected:
	QpiFunctionsOutput qpiFunctionsOutputTemp;
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputBeginTick; // Output of QPI functions queried by the BEGIN_TICK procedure for the last 16 ticks
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputEndTick; // Output of QPI functions queried by the END_TICK procedure for the last 16 ticks
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputUserProc; // Output of QPI functions queried by the USER_PROCEDURE

	PUBLIC_FUNCTION(QueryQpiFunctions)
	{
		output.qpiFunctionsOutput.year = qpi.year();
		output.qpiFunctionsOutput.month = qpi.month();
		output.qpiFunctionsOutput.day = qpi.day();
		output.qpiFunctionsOutput.hour = qpi.hour();
		output.qpiFunctionsOutput.minute = qpi.minute();
		output.qpiFunctionsOutput.second = qpi.second();
		output.qpiFunctionsOutput.millisecond = qpi.millisecond();
		output.qpiFunctionsOutput.dayOfWeek = qpi.dayOfWeek(output.qpiFunctionsOutput.year, output.qpiFunctionsOutput.month, output.qpiFunctionsOutput.day);
		output.qpiFunctionsOutput.arbitrator = qpi.arbitrator();
		output.qpiFunctionsOutput.computor0 = qpi.computor(0);
		output.qpiFunctionsOutput.epoch = qpi.epoch();
		output.qpiFunctionsOutput.invocationReward = qpi.invocationReward();
		output.qpiFunctionsOutput.invocator = qpi.invocator();
		output.qpiFunctionsOutput.numberOfTickTransactions = qpi.numberOfTickTransactions();
		output.qpiFunctionsOutput.originator = qpi.originator();
		output.qpiFunctionsOutput.tick = qpi.tick();
		output.inputDataK12 = qpi.K12(input.data);
		output.inputSignatureValid = qpi.signatureValidity(input.entity, output.inputDataK12, input.signature);
	}

	PUBLIC_PROCEDURE(QueryQpiFunctionsToState)
	{
		state.qpiFunctionsOutputTemp.year = qpi.year();
		state.qpiFunctionsOutputTemp.month = qpi.month();
		state.qpiFunctionsOutputTemp.day = qpi.day();
		state.qpiFunctionsOutputTemp.hour = qpi.hour();
		state.qpiFunctionsOutputTemp.minute = qpi.minute();
		state.qpiFunctionsOutputTemp.second = qpi.second();
		state.qpiFunctionsOutputTemp.millisecond = qpi.millisecond();
		state.qpiFunctionsOutputTemp.dayOfWeek = qpi.dayOfWeek(state.qpiFunctionsOutputTemp.year, state.qpiFunctionsOutputTemp.month, state.qpiFunctionsOutputTemp.day);
		state.qpiFunctionsOutputTemp.arbitrator = qpi.arbitrator();
		state.qpiFunctionsOutputTemp.computor0 = qpi.computor(0);
		state.qpiFunctionsOutputTemp.epoch = qpi.epoch();
		state.qpiFunctionsOutputTemp.invocationReward = qpi.invocationReward();
		state.qpiFunctionsOutputTemp.invocator = qpi.invocator();
		state.qpiFunctionsOutputTemp.numberOfTickTransactions = qpi.numberOfTickTransactions();
		state.qpiFunctionsOutputTemp.originator = qpi.originator();
		state.qpiFunctionsOutputTemp.tick = qpi.tick();

		state.qpiFunctionsOutputUserProc.set(state.qpiFunctionsOutputTemp.tick, state.qpiFunctionsOutputTemp); // 'set' computes index modulo array size
	}

	PUBLIC_FUNCTION(ReturnQpiFunctionsOutputBeginTick)
	{
		if (state.qpiFunctionsOutputBeginTick.get(input.tick).tick == input.tick) // 'get' computes index modulo array size
			output.qpiFunctionsOutput = state.qpiFunctionsOutputBeginTick.get(input.tick);
	}

	PUBLIC_FUNCTION(ReturnQpiFunctionsOutputEndTick)
	{
		if (state.qpiFunctionsOutputEndTick.get(input.tick).tick == input.tick) // 'get' computes index modulo array size
			output.qpiFunctionsOutput = state.qpiFunctionsOutputEndTick.get(input.tick);
	}

	PUBLIC_FUNCTION(ReturnQpiFunctionsOutputUserProc)
	{
		if (state.qpiFunctionsOutputUserProc.get(input.tick).tick == input.tick) // 'get' computes index modulo array size
			output.qpiFunctionsOutput = state.qpiFunctionsOutputUserProc.get(input.tick);
	}

	BEGIN_TICK()
	{
		state.qpiFunctionsOutputTemp.year = qpi.year();
		state.qpiFunctionsOutputTemp.month = qpi.month();
		state.qpiFunctionsOutputTemp.day = qpi.day();
		state.qpiFunctionsOutputTemp.hour = qpi.hour();
		state.qpiFunctionsOutputTemp.minute = qpi.minute();
		state.qpiFunctionsOutputTemp.second = qpi.second();
		state.qpiFunctionsOutputTemp.millisecond = qpi.millisecond();
		state.qpiFunctionsOutputTemp.dayOfWeek = qpi.dayOfWeek(state.qpiFunctionsOutputTemp.year, state.qpiFunctionsOutputTemp.month, state.qpiFunctionsOutputTemp.day);
		state.qpiFunctionsOutputTemp.arbitrator = qpi.arbitrator();
		state.qpiFunctionsOutputTemp.computor0 = qpi.computor(0);
		state.qpiFunctionsOutputTemp.epoch = qpi.epoch();
		state.qpiFunctionsOutputTemp.invocationReward = qpi.invocationReward();
		state.qpiFunctionsOutputTemp.invocator = qpi.invocator();
		state.qpiFunctionsOutputTemp.numberOfTickTransactions = qpi.numberOfTickTransactions();
		state.qpiFunctionsOutputTemp.originator = qpi.originator();
		state.qpiFunctionsOutputTemp.tick = qpi.tick();

		state.qpiFunctionsOutputBeginTick.set(state.qpiFunctionsOutputTemp.tick, state.qpiFunctionsOutputTemp); // 'set' computes index modulo array size
	}

	END_TICK()
	{
		state.qpiFunctionsOutputTemp.year = qpi.year();
		state.qpiFunctionsOutputTemp.month = qpi.month();
		state.qpiFunctionsOutputTemp.day = qpi.day();
		state.qpiFunctionsOutputTemp.hour = qpi.hour();
		state.qpiFunctionsOutputTemp.minute = qpi.minute();
		state.qpiFunctionsOutputTemp.second = qpi.second();
		state.qpiFunctionsOutputTemp.millisecond = qpi.millisecond();
		state.qpiFunctionsOutputTemp.dayOfWeek = qpi.dayOfWeek(state.qpiFunctionsOutputTemp.year, state.qpiFunctionsOutputTemp.month, state.qpiFunctionsOutputTemp.day);
		state.qpiFunctionsOutputTemp.arbitrator = qpi.arbitrator();
		state.qpiFunctionsOutputTemp.computor0 = qpi.computor(0);
		state.qpiFunctionsOutputTemp.epoch = qpi.epoch();
		state.qpiFunctionsOutputTemp.invocationReward = qpi.invocationReward();
		state.qpiFunctionsOutputTemp.invocator = qpi.invocator();
		state.qpiFunctionsOutputTemp.numberOfTickTransactions = qpi.numberOfTickTransactions();
		state.qpiFunctionsOutputTemp.originator = qpi.originator();
		state.qpiFunctionsOutputTemp.tick = qpi.tick();

		state.qpiFunctionsOutputEndTick.set(state.qpiFunctionsOutputTemp.tick, state.qpiFunctionsOutputTemp); // 'set' computes index modulo array size
	}

	//---------------------------------------------------------------
	// ASSET MANAGEMENT RIGHTS TRANSFER

public:
	struct IssueAsset_input
	{
		uint64 assetName;
		sint64 numberOfShares;
		uint64 unitOfMeasurement;
		sint8 numberOfDecimalPlaces;
	};
	struct IssueAsset_output
	{
		sint64 issuedNumberOfShares;
	};

	struct TransferShareOwnershipAndPossession_input
	{
		Asset asset;
		sint64 numberOfShares;
		id newOwnerAndPossessor;
	};
	struct TransferShareOwnershipAndPossession_output
	{
		sint64 transferredNumberOfShares;
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

	typedef PreManagementRightsTransfer_output SetPreReleaseSharesOutput_input;
	typedef NoData SetPreReleaseSharesOutput_output;

	typedef PreManagementRightsTransfer_output SetPreAcquireSharesOutput_input;
	typedef NoData SetPreAcquireSharesOutput_output;

	struct AcquireShareManagementRights_input
	{
		Asset asset;
		id ownerAndPossessor;
		sint64 numberOfShares;
		uint32 oldManagingContractIndex;
	};
	struct AcquireShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

protected:
	PreManagementRightsTransfer_output preReleaseSharesOutput;
	PreManagementRightsTransfer_output preAcquireSharesOutput;

	PreManagementRightsTransfer_input prevPreReleaseSharesInput;
	PreManagementRightsTransfer_input prevPreAcquireSharesInput;
	PostManagementRightsTransfer_input prevPostReleaseSharesInput;
	PostManagementRightsTransfer_input prevPostAcquireSharesInput;
	uint32 postReleaseSharesCounter;
	uint32 postAcquireShareCounter;

	PUBLIC_PROCEDURE(IssueAsset)
	{
		output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
	}

	PUBLIC_PROCEDURE(TransferShareOwnershipAndPossession)
	{
		if (qpi.transferShareOwnershipAndPossession(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newOwnerAndPossessor) > 0)
		{
			output.transferredNumberOfShares = input.numberOfShares;
		}
	}

	PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
			input.newManagingContractIndex, input.newManagingContractIndex, qpi.invocationReward()) < 0)
		{
			// error
			output.transferredNumberOfShares = 0;
		}
		else
		{
			// success
			output.transferredNumberOfShares = input.numberOfShares;
		}
	}

	PUBLIC_PROCEDURE(SetPreReleaseSharesOutput)
	{
		state.preReleaseSharesOutput = input;
	}

	PUBLIC_PROCEDURE(SetPreAcquireSharesOutput)
	{
		state.preAcquireSharesOutput = input;
	}

	PUBLIC_PROCEDURE(AcquireShareManagementRights)
	{
		if (qpi.acquireShares(input.asset, input.ownerAndPossessor, input.ownerAndPossessor, input.numberOfShares,
			input.oldManagingContractIndex, input.oldManagingContractIndex, qpi.invocationReward()) >= 0)
		{
			// success
			output.transferredNumberOfShares = input.numberOfShares;
		}
	}

	PRE_RELEASE_SHARES()
	{
		// check that qpi.acquireShares() leading to this callback is triggered by owner,
		// otherwise allowing another contract to acquire management rights is risky
		if (qpi.originator() == input.owner)
		{
			output = state.preReleaseSharesOutput;
		}
		state.prevPreReleaseSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	POST_RELEASE_SHARES()
	{
		state.postReleaseSharesCounter++;
		state.prevPostReleaseSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	PRE_ACQUIRE_SHARES()
	{
		output = state.preAcquireSharesOutput;
		state.prevPreAcquireSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	POST_ACQUIRE_SHARES()
	{
		state.postAcquireShareCounter++;
		state.prevPostAcquireSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	//---------------------------------------------------------------
	// CONTRACT INTERACTION / RESOLVING DEADLOCKS / ERROR HANDLING
public:
	typedef NoData ErrorTriggerFunction_input;
	typedef NoData ErrorTriggerFunction_output;

	typedef sint64 RunHeavyComputation_input;
	typedef sint64 RunHeavyComputation_output;

protected:

	sint64 heavyComputationResult;

	typedef Array<sint8, 32 * 1024> ErrorTriggerFunction_locals;

#pragma warning(push)
#pragma warning(disable: 4717)
	PUBLIC_FUNCTION_WITH_LOCALS(ErrorTriggerFunction)
	{
		// Recursively call itself to trigger error for testing error handling
		CALL(ErrorTriggerFunction, input, output);
	}
#pragma warning(pop)

	struct RunHeavyComputation_locals
	{
		id entityId;
		Entity entity;
		AssetIssuanceIterator issuanceIter;
		AssetPossessionIterator possessionIter;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(RunHeavyComputation)
	{
		state.heavyComputationResult = input;
		
		// Iterate through spectrum
		while (!isZero(locals.entityId = qpi.nextId(locals.entityId)))
		{
			qpi.getEntity(locals.entityId, locals.entity);
			state.heavyComputationResult += locals.entity.incomingAmount;

			// Iterate through universe
			locals.issuanceIter.begin(AssetIssuanceSelect::any());
			while (!locals.issuanceIter.reachedEnd())
			{
				locals.possessionIter.begin(locals.issuanceIter.asset(), AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(locals.entityId));
				while (!locals.possessionIter.reachedEnd())
				{
					state.heavyComputationResult += locals.possessionIter.numberOfPossessedShares();
					locals.possessionIter.next();
				}

				locals.issuanceIter.next();
			}
		}
		
		output = state.heavyComputationResult;
	}

	//---------------------------------------------------------------
	// SHAREHOLDER PROPOSALS WITH COMPACT STORAGE (OPTIONS: NO/YES)

public:
	// Proposal data type. We only support yes/no voting.
	typedef ProposalDataYesNo ProposalDataT;

	// MultiVariables proposal option data type, which is custom per contract
	struct MultiVariablesProposalExtraData
	{
		struct Option
		{
			uint64 dummyStateVariable1;
			uint32 dummyStateVariable2;
			sint8 dummyStateVariable3;
		};

		Option optionYesValues;
		bool hasValueDummyStateVariable1;
		bool hasValueDummyStateVariable2;
		bool hasValueDummyStateVariable3;

		bool isValid() const
		{
			return hasValueDummyStateVariable1 || hasValueDummyStateVariable2 || hasValueDummyStateVariable3;
		}
	};

	struct SetShareholderProposal_input
	{
		ProposalDataT proposalData;
		MultiVariablesProposalExtraData multiVarData; // may be skipped when sending TX if not MultiVariables proposal
	};
	typedef QPI::SET_SHAREHOLDER_PROPOSAL_output SetShareholderProposal_output;

	PUBLIC_PROCEDURE(SetShareholderProposal)
	{
		// - fee can be handled as you like
		// - input.proposalData.epoch == 0 means clearing a proposal

		// default return code: failure
		output = INVALID_PROPOSAL_INDEX;

		// custom checks
		if (input.proposalData.epoch != 0)
		{
			switch (ProposalTypes::cls(input.proposalData.type))
			{
			case ProposalTypes::Class::MultiVariables:
				// check input
				if (!input.multiVarData.isValid())
					return;

				// check that proposed values are in valid range
				if (input.multiVarData.hasValueDummyStateVariable1 && input.multiVarData.optionYesValues.dummyStateVariable1 > 1000000000000llu)
					return;
				if (input.multiVarData.hasValueDummyStateVariable2 && input.multiVarData.optionYesValues.dummyStateVariable2 > 1000000llu)
					return;
				if (input.multiVarData.hasValueDummyStateVariable3 && (input.multiVarData.optionYesValues.dummyStateVariable3 > 100 || input.multiVarData.optionYesValues.dummyStateVariable3 < -100))
					return;

				break;

			case ProposalTypes::Class::Variable:
				// check that variable index is in valid range
				if (input.proposalData.variableOptions.variable >= 3)
					return;

				// check that proposed value is in valid range
				if (input.proposalData.variableOptions.variable == 0 && input.proposalData.variableOptions.value > 1000000000000llu)
					return;
				if (input.proposalData.variableOptions.variable == 1 && input.proposalData.variableOptions.value > 1000000llu)
					return;
				if (input.proposalData.variableOptions.variable == 2 && (input.proposalData.variableOptions.value > 100 || input.proposalData.variableOptions.value < -100))
					return;

				break;

			case ProposalTypes::Class::GeneralOptions:
				// allow without check
				break;

			default:
				// this forbids other proposals including transfers and all future propsasl classes not implemented yet
				return;
			}
		}

		// Try to set proposal (checks invocator's rights and general validity of input proposal), returns proposal index
		output = qpi(state.proposals).setProposal(qpi.invocator(), input.proposalData);

		if (output != INVALID_PROPOSAL_INDEX)
		{
			// success
			if (ProposalTypes::cls(input.proposalData.type) == ProposalTypes::Class::MultiVariables)
			{
				// store custom data of multi-variable proposal in array (at position proposalIdx)
				state.multiVariablesProposalData.set(output, input.multiVarData);
			}
		}
	}

	typedef ProposalMultiVoteDataV1 SetShareholderVotes_input;
	typedef bit SetShareholderVotes_output;

	PUBLIC_PROCEDURE(SetShareholderVotes)
	{
		// - fee can be handled as you like

		output = qpi(state.proposals).vote(qpi.invocator(), input);
	}

	struct END_EPOCH_locals
	{
		sint32 proposalIndex;
		ProposalDataT proposal;
		ProposalSummarizedVotingDataV1 results;
		MultiVariablesProposalExtraData multiVarData;
	};

	END_EPOCH_WITH_LOCALS()
	{
		// Analyze proposal results and set variables

		// Iterate all proposals that were open for voting in this epoch ...
		locals.proposalIndex = -1;
		while ((locals.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.proposalIndex, qpi.epoch())) >= 0)
		{
			if (!qpi(state.proposals).getProposal(locals.proposalIndex, locals.proposal))
				continue;

			// handle Variable proposal type
			if (ProposalTypes::cls(locals.proposal.type) == ProposalTypes::Class::Variable)
			{
				// Get voting results and check if conditions for proposal acceptance are met
				if (!qpi(state.proposals).getVotingSummary(locals.proposalIndex, locals.results))
					continue;

				// Check if the yes option (1) has been accepted
				if (locals.results.getAcceptedOption() == 1)
				{
					if (locals.proposal.variableOptions.variable == 0)
						state.dummyStateVariable1 = uint64(locals.proposal.variableOptions.value);
					if (locals.proposal.variableOptions.variable == 1)
						state.dummyStateVariable2 = uint32(locals.proposal.variableOptions.value);
					if (locals.proposal.variableOptions.variable == 2)
						state.dummyStateVariable3 = sint8(locals.proposal.variableOptions.value);
				}
			}

			// handle MultiVariables proposal type
			if (ProposalTypes::cls(locals.proposal.type) == ProposalTypes::Class::MultiVariables)
			{
				// Get voting results and check if conditions for proposal acceptance are met
				if (!qpi(state.proposals).getVotingSummary(locals.proposalIndex, locals.results))
					continue;

				// Check if the yes option (1) has been accepted
				if (locals.results.getAcceptedOption() == 1)
				{
					locals.multiVarData = state.multiVariablesProposalData.get(locals.proposalIndex);

					if (locals.multiVarData.hasValueDummyStateVariable1)
						state.dummyStateVariable1 = locals.multiVarData.optionYesValues.dummyStateVariable1;
					if (locals.multiVarData.hasValueDummyStateVariable2)
						state.dummyStateVariable2 = locals.multiVarData.optionYesValues.dummyStateVariable2;
					if (locals.multiVarData.hasValueDummyStateVariable3)
						state.dummyStateVariable3 = locals.multiVarData.optionYesValues.dummyStateVariable3;
				}
			}
		}
	}

	struct GetShareholderProposalIndices_input
	{
		bit activeProposals;		// Set true to return indices of active proposals, false for proposals of prior epochs
		sint32 prevProposalIndex;   // Set -1 to start getting indices. If returned index array is full, call again with highest index returned.
	};
	struct GetShareholderProposalIndices_output
	{
		uint16 numOfIndices;		// Number of valid entries in indices. Call again if it is 64.
		Array<uint16, 64> indices;	// Requested proposal indices. Valid entries are in range 0 ... (numOfIndices - 1).
	};

	PUBLIC_FUNCTION(GetShareholderProposalIndices)
	{
		if (input.activeProposals)
		{
			// Return proposals that are open for voting in current epoch
			// (output is initalized with zeros by contract system)
			while ((input.prevProposalIndex = qpi(state.proposals).nextProposalIndex(input.prevProposalIndex, qpi.epoch())) >= 0)
			{
				output.indices.set(output.numOfIndices, input.prevProposalIndex);
				++output.numOfIndices;

				if (output.numOfIndices == output.indices.capacity())
					break;
			}
		}
		else
		{
			// Return proposals of previous epochs not overwritten yet
			// (output is initalized with zeros by contract system)
			while ((input.prevProposalIndex = qpi(state.proposals).nextFinishedProposalIndex(input.prevProposalIndex)) >= 0)
			{
				output.indices.set(output.numOfIndices, input.prevProposalIndex);
				++output.numOfIndices;

				if (output.numOfIndices == output.indices.capacity())
					break;
			}
		}
	}

	typedef NoData GetShareholderProposalFees_input;
	struct GetShareholderProposalFees_output
	{
		sint64 setProposalFee;
		sint64 setVoteFee;
	};

	PUBLIC_FUNCTION(GetShareholderProposalFees)
	{
		output.setProposalFee = 0;
		output.setVoteFee = 0;
	}

	struct GetShareholderProposal_input
	{
		uint16 proposalIndex;
	};
	struct GetShareholderProposal_output
	{
		ProposalDataT proposal;
		id proposerPubicKey;
		MultiVariablesProposalExtraData multiVarData;
	};

	PUBLIC_FUNCTION(GetShareholderProposal)
	{
		// On error, output.proposal.type is set to 0
		output.proposerPubicKey = qpi(state.proposals).proposerId(input.proposalIndex);
		qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
		if (ProposalTypes::cls(output.proposal.type) == ProposalTypes::Class::MultiVariables)
		{
			output.multiVarData = state.multiVariablesProposalData.get(input.proposalIndex);
		}
	}

	struct GetShareholderVotes_input
	{
		id voter;
		uint16 proposalIndex;
	};
	typedef ProposalMultiVoteDataV1 GetShareholderVotes_output;

	PUBLIC_FUNCTION(GetShareholderVotes)
	{
		// On error, output.votes.proposalType is set to 0
		qpi(state.proposals).getVotes(
			input.proposalIndex,
			input.voter,
			output);
	}


	struct GetShareholderVotingResults_input
	{
		uint16 proposalIndex;
	};
	typedef ProposalSummarizedVotingDataV1 GetShareholderVotingResults_output;

	PUBLIC_FUNCTION(GetShareholderVotingResults)
	{
		// On error, output.totalVotesAuthorized is set to 0
		qpi(state.proposals).getVotingSummary(
			input.proposalIndex, output);
	}

	struct SET_SHAREHOLDER_PROPOSAL_locals
	{
		SetShareholderProposal_input userProcInput;
	};

	SET_SHAREHOLDER_PROPOSAL_WITH_LOCALS()
	{
		copyFromBuffer(locals.userProcInput, input);
		CALL(SetShareholderProposal, locals.userProcInput, output);

		// bug-checking: qpi.setShareholder*() must fail
		ASSERT(!qpi.setShareholderVotes(10, ProposalMultiVoteDataV1(), qpi.invocationReward()));
		ASSERT(qpi.setShareholderProposal(10, input, qpi.invocationReward()) == INVALID_PROPOSAL_INDEX);
	}

	SET_SHAREHOLDER_VOTES()
	{
		CALL(SetShareholderVotes, input, output);

#ifdef NO_UEFI
		// bug-checking: qpi.setShareholder*() must fail
		ASSERT(!qpi.setShareholderVotes(10, input, qpi.invocationReward()));
		ASSERT(qpi.setShareholderProposal(10, SET_SHAREHOLDER_PROPOSAL_input(), qpi.invocationReward()) == INVALID_PROPOSAL_INDEX);
#endif
	}

protected:
	// Variables that can be set with proposals
	uint64 dummyStateVariable1;
	uint32 dummyStateVariable2;
	sint8 dummyStateVariable3;

	// Shareholders of TESTEXA have right to propose and vote. Only 16 slots provided.
	typedef ProposalAndVotingByShareholders<16, TESTEXA_ASSET_NAME> ProposersAndVotersT;

	// Proposal and voting storage type
	typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT;

	// Proposal storage
	ProposalVotingT proposals;

	// MultiVariables proposal option data storage (same number of slots as proposals)
	Array<MultiVariablesProposalExtraData, 16> multiVariablesProposalData;


public:
	struct SetProposalInOtherContractAsShareholder_input
	{
		Array<uint8, 512> proposalDataBuffer;
		uint16 otherContractIndex;
	};
	struct SetProposalInOtherContractAsShareholder_output
	{
		uint16 proposalIndex;
	};
	struct SetProposalInOtherContractAsShareholder_locals
	{
		Array<uint8, 1024> proposalDataBuffer;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SetProposalInOtherContractAsShareholder)
	{
		// User procedure for letting TESTEXB create a shareholder proposal in TESTEXA as shareholder of TESTEXA.
		// Skipped here: checking that invocator has right to set proposal for this contract (e.g., is contract "admin")
		copyToBuffer(locals.proposalDataBuffer, input.proposalDataBuffer);
		output.proposalIndex = qpi.setShareholderProposal(input.otherContractIndex, locals.proposalDataBuffer, qpi.invocationReward());
	}

	struct SetVotesInOtherContractAsShareholder_input
	{
		ProposalMultiVoteDataV1 voteData;
		uint16 otherContractIndex;
	};
	struct SetVotesInOtherContractAsShareholder_output
	{
		bit success;
	};

	PUBLIC_PROCEDURE(SetVotesInOtherContractAsShareholder)
	{
		// User procedure for letting TESTEXB cast shareholder votes in TESTEXA as shareholder of TESTEXA.
		// Skipped here: checking that invocator has right to cast votes for this contract (e.g., is contract "admin")
		output.success = qpi.setShareholderVotes(input.otherContractIndex, input.voteData, qpi.invocationReward());
	}

	//---------------------------------------------------------------
	// COMMON PARTS

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(QueryQpiFunctions, 1);
		REGISTER_USER_FUNCTION(ReturnQpiFunctionsOutputBeginTick, 2);
		REGISTER_USER_FUNCTION(ReturnQpiFunctionsOutputEndTick, 3);
		REGISTER_USER_FUNCTION(ReturnQpiFunctionsOutputUserProc, 4);
		REGISTER_USER_FUNCTION(ErrorTriggerFunction, 5);

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
		REGISTER_USER_PROCEDURE(QueryQpiFunctionsToState, 7);
		REGISTER_USER_PROCEDURE(RunHeavyComputation, 8);

		REGISTER_USER_PROCEDURE(SetProposalInOtherContractAsShareholder, 40);
		REGISTER_USER_PROCEDURE(SetVotesInOtherContractAsShareholder, 41);

		// Shareholder proposals: use standard function/procedure indices
		REGISTER_USER_FUNCTION(GetShareholderProposalFees, 65531);
		REGISTER_USER_FUNCTION(GetShareholderProposalIndices, 65532);
		REGISTER_USER_FUNCTION(GetShareholderProposal, 65533);
		REGISTER_USER_FUNCTION(GetShareholderVotes, 65534);
		REGISTER_USER_FUNCTION(GetShareholderVotingResults, 65535);

		REGISTER_USER_PROCEDURE(SetShareholderProposal, 65534);
		REGISTER_USER_PROCEDURE(SetShareholderVotes, 65535);

	}
};
