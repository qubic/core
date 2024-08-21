using namespace QPI;

struct CCF2 
{
};

struct CCF : public ContractBase
{
	//----------------------------------------------------------------------------
	// Define common types

	// Proposal data type. We don't support scalar votes, but multi-option voting.
	typedef ProposalDataV1<false> ProposalDataT;

	// Computors have right to propose and vote. There is one proposal slot per computor to make sure
	// that a proposal can never be blocked by no free slots.
	typedef ProposalAndVotingByComputors<NUMBER_OF_COMPUTORS> ProposersAndVotersT;

	// Proposal and voting storage type
	typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT;

	struct Success_output
	{
		bool okay;
	};


private:
	//----------------------------------------------------------------------------
	// Define state
	ProposalVotingT proposals;

	//----------------------------------------------------------------------------
	// Define private procedures and functions with input and output


public:
	//----------------------------------------------------------------------------
	// Define public procedures and functions with input and output

	typedef ProposalDataT SetProposal_input;
	typedef Success_output SetProposal_output;
	struct SetProposal_locals
	{
		uint32 i;
		sint64 millionthAmount;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SetProposal)
		// TODO: Fee? Burn fee?

		// Check requirements for proposals in this contract
		if (ProposalTypes::cls(input.type) == ProposalTypes::Class::Variable)
		{
			// Proposals for setting a variable are not allowed at the moment (lack of meaning)
			output.okay = false;
			return;
		}

		// TODO: anything else to check, for example if there are enough qus in the balance for this funding?

		// Try to set proposal (checks originators rights and general validity of input proposal)
		output.okay = qpi(state.proposals).setProposal(qpi.originator(), input);
	_


	struct GetProposalIndices_input
	{
		bit activeProposals;		// Set true to return indices of active proposals, false for proposals of prior epochs
		sint32 prevProposalIndex;   // Set -1 to start getting indices. If returned index array is full, call again with highest index returned.
	};
	struct GetProposalIndices_output
	{
		uint16 numOfIndices;		// Number of valid entries in indices. Call again if it is 64.
		array<uint16, 64> indices;	// Requested proposal indices. Valid entries are in range 0 ... (numOfIndices - 1).
	};

	PUBLIC_FUNCTION(GetProposalIndices)
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
	_


	struct GetProposal_input
	{
		uint16 proposalIndex;
	};
	struct GetProposal_output
	{
		bit okay;
		uint8 _padding0[7];
		id proposerPubicKey;
		ProposalDataT proposal;
	};

	PUBLIC_FUNCTION(GetProposal)
		output.proposerPubicKey = qpi(state.proposals).proposerId(input.proposalIndex);
		output.okay = qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
	_


	typedef ProposalSingleVoteDataV1 Vote_input;
	typedef Success_output Vote_output;

	PUBLIC_PROCEDURE(Vote)
		// TODO: Fee? Burn fee?
		output.okay = qpi(state.proposals).vote(qpi.originator(), input);
	_


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
		output.okay = qpi(state.proposals).getVote(
			input.proposalIndex,
			qpi(state.proposals).voterIndex(input.voter),
			output.vote);
	_


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
		output.okay = qpi(state.proposals).getVotingSummary(
			input.proposalIndex, output.results);
	_


	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_FUNCTION(GetProposalIndices, 1);
		REGISTER_USER_FUNCTION(GetProposal, 2);
		REGISTER_USER_FUNCTION(GetVote, 3);
		REGISTER_USER_FUNCTION(GetVotingResults, 4);

		REGISTER_USER_PROCEDURE(SetProposal, 1);
		REGISTER_USER_PROCEDURE(Vote, 2);
	_


	struct END_EPOCH_locals
	{
		sint32 proposalIndex;
		ProposalDataT proposal;
		ProposalSummarizedVotingDataV1 results;
		sint32 optionIndex;
		uint32 optionVotes;
		sint32 mostVotedOptionIndex;
		uint32 mostVotedOptionVotes;
	};

	END_EPOCH_WITH_LOCALS
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

				// TODO: apply rule proposed by CFB?
				// https://discord.com/channels/768887649540243497/768890555564163092/1272616099728461916
				// Also apply to GQMPROP?

				// Find most voted "transfer" option (option 0 is "no transfer")
				locals.mostVotedOptionIndex = 0;
				locals.mostVotedOptionVotes = 0;
				for (locals.optionIndex = 1; locals.optionIndex < locals.results.optionCount; ++locals.optionIndex)
				{
					locals.optionVotes = locals.results.optionVoteCount.get(locals.optionIndex);
					if (locals.mostVotedOptionVotes < locals.optionVotes)
					{
						locals.mostVotedOptionVotes = locals.optionVotes;
						locals.mostVotedOptionIndex = locals.optionIndex;
					}
				}

				// Option for transfer has been accepted?
				if (locals.mostVotedOptionVotes > QUORUM / 2)
				{
					// Transfer
					qpi.transfer(locals.proposal.transfer.destination, locals.proposal.transfer.amounts.get(locals.mostVotedOptionIndex - 1));

					// TODO: keep log of last N transfers for query function?
				}
			}
		}
	_

};
