using namespace QPI;

struct CCF2 
{
};

struct CCF : public ContractBase
{
	//----------------------------------------------------------------------------
	// Define common types

	// Proposal data type. We only support yes/no voting. Complex projects should be broken down into milestones
	// and apply for funding multiple times.
	typedef ProposalDataYesNo ProposalDataT;

	// Anyone can set a proposal, but only computors have right vote.
	typedef ProposalByAnyoneVotingByComputors<100> ProposersAndVotersT;

	// Proposal and voting storage type
	typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT;

	struct Success_output
	{
		bool okay;
	};

	struct LatestTransfersEntry
	{
		id destination;
		Array<uint8, 256> url;
		sint64 amount;
		uint32 tick;
		bool success;
	};

	typedef Array<LatestTransfersEntry, 128> LatestTransfersT;

private:
	//----------------------------------------------------------------------------
	// Define state
	ProposalVotingT proposals;

	LatestTransfersT latestTransfers;
	uint8 lastTransfersNextOverwriteIdx;

	uint32 setProposalFee;

	//----------------------------------------------------------------------------
	// Define private procedures and functions with input and output


public:
	//----------------------------------------------------------------------------
	// Define public procedures and functions with input and output

	typedef ProposalDataT SetProposal_input;
	typedef Success_output SetProposal_output;

	PUBLIC_PROCEDURE(SetProposal)
	{
		if (qpi.invocationReward() < state.setProposalFee)
		{
			// Invocation reward not sufficient, undo payment and cancel
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.okay = false;
			return;
		}
		else if (qpi.invocationReward() > state.setProposalFee)
		{
			// Invocation greater than fee, pay back difference
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.setProposalFee);
		}

		// Burn invocation reward
		qpi.burn(qpi.invocationReward());

		// Check requirements for proposals in this contract
		if (ProposalTypes::cls(input.type) != ProposalTypes::Class::Transfer)
		{
			// Only transfer proposals are allowed
			// -> Cancel if epoch is not 0 (which means clearing the proposal)
			if (input.epoch != 0)
			{
				output.okay = false;
				return;
			}
		}

		// Try to set proposal (checks originators rights and general validity of input proposal)
		output.okay = qpi(state.proposals).setProposal(qpi.originator(), input);
	}


	struct GetProposalIndices_input
	{
		bit activeProposals;		// Set true to return indices of active proposals, false for proposals of prior epochs
		sint32 prevProposalIndex;   // Set -1 to start getting indices. If returned index array is full, call again with highest index returned.
	};
	struct GetProposalIndices_output
	{
		uint16 numOfIndices;		// Number of valid entries in indices. Call again if it is 64.
		Array<uint16, 64> indices;	// Requested proposal indices. Valid entries are in range 0 ... (numOfIndices - 1).
	};

	PUBLIC_FUNCTION(GetProposalIndices)
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


	struct GetProposal_input
	{
		uint16 proposalIndex;
	};
	struct GetProposal_output
	{
		bit okay;
		Array<uint8, 4> _padding0;
		Array<uint8, 2> _padding1;
		Array<uint8, 1> _padding2;
		id proposerPubicKey;
		ProposalDataT proposal;
	};

	PUBLIC_FUNCTION(GetProposal)
	{
		output.proposerPubicKey = qpi(state.proposals).proposerId(input.proposalIndex);
		output.okay = qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
	}


	typedef ProposalSingleVoteDataV1 Vote_input;
	typedef Success_output Vote_output;

	PUBLIC_PROCEDURE(Vote)
	{
		// For voting, there is no fee
		if (qpi.invocationReward() > 0)
		{
			// Pay back invocation reward
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		
		// Try to vote (checks right to vote and match with proposal)
		output.okay = qpi(state.proposals).vote(qpi.originator(), input);
	}


	struct GetVote_input
	{
		id voter;
		uint16 proposalIndex;
	};
	struct GetVote_output
	{
		bit okay;
		ProposalSingleVoteDataV1 vote;
	};

	PUBLIC_FUNCTION(GetVote)
	{
		output.okay = qpi(state.proposals).getVote(
			input.proposalIndex,
			qpi(state.proposals).voterIndex(input.voter),
			output.vote);
	}


	struct GetVotingResults_input
	{
		uint16 proposalIndex;
	};
	struct GetVotingResults_output
	{
		bit okay;
		ProposalSummarizedVotingDataV1 results;
	};

	PUBLIC_FUNCTION(GetVotingResults)
	{
		output.okay = qpi(state.proposals).getVotingSummary(
			input.proposalIndex, output.results);
	}


	typedef NoData GetLatestTransfers_input;
	typedef LatestTransfersT GetLatestTransfers_output;

	PUBLIC_FUNCTION(GetLatestTransfers)
	{
		output = state.latestTransfers;
	}


	typedef NoData GetProposalFee_input;
	struct GetProposalFee_output
	{
		uint32 proposalFee; // Amount of qus
	};

	PUBLIC_FUNCTION(GetProposalFee)
	{
		output.proposalFee = state.setProposalFee;
	}


	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetProposalIndices, 1);
		REGISTER_USER_FUNCTION(GetProposal, 2);
		REGISTER_USER_FUNCTION(GetVote, 3);
		REGISTER_USER_FUNCTION(GetVotingResults, 4);
		REGISTER_USER_FUNCTION(GetLatestTransfers, 5);
		REGISTER_USER_FUNCTION(GetProposalFee, 6);

		REGISTER_USER_PROCEDURE(SetProposal, 1);
		REGISTER_USER_PROCEDURE(Vote, 2);
	}


	INITIALIZE()
	{
		state.setProposalFee = 1000000;
	}


	struct END_EPOCH_locals
	{
		sint32 proposalIndex;
		ProposalDataT proposal;
		ProposalSummarizedVotingDataV1 results;
		LatestTransfersEntry transfer;
	};

	END_EPOCH_WITH_LOCALS()
	{
		// Analyze transfer proposal results

		// Iterate all proposals that were open for voting in this epoch ...
		locals.proposalIndex = -1;
		while ((locals.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.proposalIndex, qpi.epoch())) >= 0)
		{
			if (!qpi(state.proposals).getProposal(locals.proposalIndex, locals.proposal))
				continue;

			// ... and have transfer proposal type
			if (ProposalTypes::cls(locals.proposal.type) == ProposalTypes::Class::Transfer)
			{
				// Get voting results and check if conditions for proposal acceptance are met
				if (!qpi(state.proposals).getVotingSummary(locals.proposalIndex, locals.results))
					continue;

				// The total number of votes needs to be at least the quorum
				if (locals.results.totalVotes < QUORUM)
					continue;

				// The transfer option (1) must have more votes than the no-transfer option (0)
				if (locals.results.optionVoteCount.get(1) < locals.results.optionVoteCount.get(0))
					continue;
				
				// Option for transfer has been accepted?
				if (locals.results.optionVoteCount.get(1) > div<uint32>(QUORUM, 2U))
				{
					// Prepare log entry and do transfer
					locals.transfer.destination = locals.proposal.transfer.destination;
					locals.transfer.amount = locals.proposal.transfer.amount;
					locals.transfer.tick = qpi.tick();
					copyMemory(locals.transfer.url, locals.proposal.url);
					locals.transfer.success = (qpi.transfer(locals.transfer.destination, locals.transfer.amount) >= 0);

					// Add log entry
					state.latestTransfers.set(state.lastTransfersNextOverwriteIdx, locals.transfer);
					state.lastTransfersNextOverwriteIdx = (state.lastTransfersNextOverwriteIdx + 1) & (state.latestTransfers.capacity() - 1);
				}
			}
		}
	}

};
