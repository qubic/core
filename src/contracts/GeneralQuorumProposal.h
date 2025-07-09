using namespace QPI;

struct GQMPROP2
{
};

struct GQMPROP : public ContractBase
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

	struct RevenueDonationEntry
	{
		id destinationPublicKey;
		sint64 millionthAmount;
		uint16 firstEpoch;
	};

	typedef Array<RevenueDonationEntry, 128> RevenueDonationT;

protected:
	//----------------------------------------------------------------------------
	// Define state

	// Storage of proposals and votes
	ProposalVotingT proposals;

	// Revenue donation table:
	// - order of IDs (destinationPublicKey) defines the order of donations
	// - order of IDs does not change except for IDs being added at the end, no ID is removed or inserted in the middle
	// - there may be multiple entries of one ID, exactly one with a firstEpoch in the past and multiple with
	//   firstEpoch in the future
	// - each pair of ID and firstEpoch exists at most once (it identifies the entry that may be overwritten)
	// - entries of the same ID are grouped in a continuous block and sorted by firstEpoch in ascending order
	// - the first entry with NULL_ID marks the end of the current table
	// - an ID is never completely removed, even if donation is set to 0% (for keeping order)
	RevenueDonationT revenueDonation;

	//----------------------------------------------------------------------------
	// Define private procedures and functions with input and output

	typedef RevenueDonationEntry _SetRevenueDonationEntry_input;
	typedef Success_output _SetRevenueDonationEntry_output;
	struct _SetRevenueDonationEntry_locals
	{
		sint64 idx;
		sint64 firstEmptyIdx;
		sint64 beforeInsertIdx;
		RevenueDonationEntry entry;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(_SetRevenueDonationEntry)
	{
		// Based on publicKey and firstEpoch, find entry to update or indices required to add entry later
		locals.firstEmptyIdx = -1;
		locals.beforeInsertIdx = -1;
		for (locals.idx = 0; locals.idx < (sint64)state.revenueDonation.capacity(); ++locals.idx)
		{
			locals.entry = state.revenueDonation.get(locals.idx);
			if (input.destinationPublicKey == locals.entry.destinationPublicKey)
			{
				// Found entry with matching publicKey
				if (input.firstEpoch == locals.entry.firstEpoch)
				{
					// Found entry with same publicKey and "first epoch" -> update entry and we are done
					state.revenueDonation.set(locals.idx, input);
					output.okay = true;
					return;
				}
				else if (input.firstEpoch > locals.entry.firstEpoch)
				{
					// Update insertion index for sorting by firstEpoch
					locals.beforeInsertIdx = locals.idx;
				}
				else // input.firstEpoch < locals.entry.firstEpoch
				{
					if (locals.beforeInsertIdx == -1)
					{
						// Insert before first entry with this publicKey
						locals.beforeInsertIdx = locals.idx - 1;
					}
				}
			}
			else if (isZero(locals.entry.destinationPublicKey))
			{
				// We reached the end of the used list
				locals.firstEmptyIdx = locals.idx;
				break;
			}
		}

		// Found no entry for updating -> entry needs to be added
		if (locals.firstEmptyIdx >= 0)
		{
			// We have at least one free slot for adding the entry
			if (locals.beforeInsertIdx >= 0)
			{
				// Insert entry at correct position to keep order after moving items to free the slot
				ASSERT(locals.beforeInsertIdx < locals.firstEmptyIdx);
				ASSERT(locals.beforeInsertIdx + 1 < (sint64)state.revenueDonation.capacity());
				for (locals.idx = locals.firstEmptyIdx - 1; locals.idx > locals.beforeInsertIdx; --locals.idx)
				{
					state.revenueDonation.set(locals.idx + 1, state.revenueDonation.get(locals.idx));
				}
				state.revenueDonation.set(locals.beforeInsertIdx + 1, input);
			}
			else
			{
				// PublicKey not found so far -> add entry at the end
				state.revenueDonation.set(locals.firstEmptyIdx, input);
				output.okay = true;
			}
		}
	}

	typedef NoData _CleanupRevenueDonation_input;
	typedef NoData _CleanupRevenueDonation_output;
	struct _CleanupRevenueDonation_locals
	{
		uint64 idx, idxMove;
		RevenueDonationEntry entry1;
		RevenueDonationEntry entry2;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(_CleanupRevenueDonation)
	{
		// Make sure we have at most one entry with non-future firstEpoch per destinationPublicKey.
		// Use that entries are grouped by ID and sorted by firstEpoch.
		for (locals.idx = 1; locals.idx < state.revenueDonation.capacity(); ++locals.idx)
		{
			locals.entry1 = state.revenueDonation.get(locals.idx - 1);
			if (isZero(locals.entry1.destinationPublicKey))
				break;
			locals.entry2 = state.revenueDonation.get(locals.idx);
			if (locals.entry1.destinationPublicKey == locals.entry2.destinationPublicKey)
			{
				ASSERT(locals.entry1.firstEpoch < locals.entry2.firstEpoch);
				if (locals.entry1.firstEpoch < qpi.epoch() && locals.entry2.firstEpoch <= qpi.epoch())
				{
					// We have found two non-future entries of the same ID, so remove older one and fill gap
					//for (locals.idxMove = locals.idx; locals.idxMove < state.revenueDonation.capacity() && !isZero(state.revenueDonation.get(locals.idxMove).destinationPublicKey); ++locals.idxMove)
					locals.idxMove = locals.idx;
					while (locals.idxMove < state.revenueDonation.capacity() && !isZero(state.revenueDonation.get(locals.idxMove).destinationPublicKey))
					{
						state.revenueDonation.set(locals.idxMove - 1, state.revenueDonation.get(locals.idxMove));
						++locals.idxMove;
					}
					setMemory(locals.entry2, 0);
					state.revenueDonation.set(locals.idxMove - 1, locals.entry2);
				}
			}
		}
	}

public:
	//----------------------------------------------------------------------------
	// Define public procedures and functions with input and output

	typedef ProposalDataT SetProposal_input;
	struct SetProposal_output
	{
		uint16 proposalIndex;
		bool okay;
	};
	struct SetProposal_locals
	{
		uint32 i;
		sint64 millionthAmount;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SetProposal)
	{
		// TODO: Fee? Burn fee?

		// Set default return values to error
		output.okay = false;
		output.proposalIndex = INVALID_PROPOSAL_INDEX;

		// Check requirements for proposals in this contract
		switch (ProposalTypes::cls(input.type))
		{
		case ProposalTypes::Class::GeneralOptions:
			// No extra checks required
			break;

		case ProposalTypes::Class::Transfer:
			// Check that amounts, which are in millionth, are in range of 0 (= 0%) to 1000000 (= 100%)
			for (locals.i = 0; locals.i < 4; ++locals.i)
			{
				locals.millionthAmount = input.transfer.amounts.get(locals.i);
				if (locals.millionthAmount < 0 || locals.millionthAmount > 1000000)
				{
					return;
				}
			}
			break;

		case ProposalTypes::Class::TransferInEpoch:
			// Check amount and epoch
			locals.millionthAmount = input.transferInEpoch.amount;
			if (locals.millionthAmount < 0 || locals.millionthAmount > 1000000 || input.transferInEpoch.targetEpoch <= qpi.epoch())
			{
				return;
			}
			break;

		default:
			// Default: not allowed (only allow classes listed expliticly, because new classes may be added)
			return;
		}

		// Try to set proposal (checks originators rights and general validity of input proposal)
		output.proposalIndex = qpi(state.proposals).setProposal(qpi.originator(), input);
		output.okay = (output.proposalIndex != INVALID_PROPOSAL_INDEX);
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
		// TODO: Fee? Burn fee?
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


	typedef NoData GetRevenueDonation_input;
	typedef RevenueDonationT GetRevenueDonation_output;

	PUBLIC_FUNCTION(GetRevenueDonation)
	{
		output = state.revenueDonation;
	}


    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
        REGISTER_USER_FUNCTION(GetProposalIndices, 1);
        REGISTER_USER_FUNCTION(GetProposal, 2);
        REGISTER_USER_FUNCTION(GetVote, 3);
        REGISTER_USER_FUNCTION(GetVotingResults, 4);
        REGISTER_USER_FUNCTION(GetRevenueDonation, 5);

        REGISTER_USER_PROCEDURE(SetProposal, 1);
        REGISTER_USER_PROCEDURE(Vote, 2);
    }

		
	struct BEGIN_EPOCH_locals
	{
		sint32 proposalIndex;
		ProposalDataT proposal;
		uint16 propClass;
		ProposalSummarizedVotingDataV1 results;
		sint32 optionIndex;
		uint32 optionVotes;
		sint32 mostVotedOptionIndex;
		uint32 mostVotedOptionVotes;
		RevenueDonationEntry revenueDonationEntry;
		Success_output success;
		_CleanupRevenueDonation_input cleanupInput;
		_CleanupRevenueDonation_output cleanupOutput;
	};

	BEGIN_EPOCH_WITH_LOCALS()
	{
		// Analyze transfer proposal results

		// Iterate all proposals that were open for voting in previous epoch ...
		locals.proposalIndex = -1;
		while ((locals.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.proposalIndex, qpi.epoch() - 1)) >= 0)
		{
			if (qpi(state.proposals).getProposal(locals.proposalIndex, locals.proposal))
			{
				// ... and have transfer proposal type
				locals.propClass = ProposalTypes::cls(locals.proposal.type);
				if (locals.propClass == ProposalTypes::Class::Transfer || locals.propClass == ProposalTypes::Class::TransferInEpoch)
				{
					// Get voting results and check if conditions for proposal acceptance are met
					if (qpi(state.proposals).getVotingSummary(locals.proposalIndex, locals.results))
					{
						// The total number of votes needs to be at least the quorum
						if (locals.results.totalVotes >= QUORUM)
						{
							// Find most voted option
							locals.mostVotedOptionIndex = 0;
							locals.mostVotedOptionVotes = locals.results.optionVoteCount.get(0);
							for (locals.optionIndex = 1; locals.optionIndex < locals.results.optionCount; ++locals.optionIndex)
							{
								locals.optionVotes = locals.results.optionVoteCount.get(locals.optionIndex);
								if (locals.mostVotedOptionVotes < locals.optionVotes)
								{
									locals.mostVotedOptionVotes = locals.optionVotes;
									locals.mostVotedOptionIndex = locals.optionIndex;
								}
							}

							// Option for changing status quo has been accepted? (option 0 is "no change")
							if (locals.mostVotedOptionIndex > 0 && locals.mostVotedOptionVotes > div<uint32>(QUORUM, 2U))
							{
								// Set in revenueDonation table (cannot be done in END_EPOCH, because this may overwrite entries that
								// are still needed unchanged for this epoch for the revenue donation which is run after END_EPOCH)
								if (locals.propClass == ProposalTypes::Class::TransferInEpoch)
								{
									ASSERT(locals.mostVotedOptionIndex == 1);
									ASSERT(locals.proposal.transferInEpoch.targetEpoch >= qpi.epoch());
									locals.revenueDonationEntry.destinationPublicKey = locals.proposal.transferInEpoch.destination;
									locals.revenueDonationEntry.millionthAmount = locals.proposal.transferInEpoch.amount;
									locals.revenueDonationEntry.firstEpoch = locals.proposal.transferInEpoch.targetEpoch;
								}
								else
								{
									locals.revenueDonationEntry.destinationPublicKey = locals.proposal.transfer.destination;
									locals.revenueDonationEntry.millionthAmount = locals.proposal.transfer.amounts.get(locals.mostVotedOptionIndex - 1);
									locals.revenueDonationEntry.firstEpoch = qpi.epoch();
								}
								CALL(_SetRevenueDonationEntry, locals.revenueDonationEntry, locals.success);
							}
						}
					}
				}
			}
		}

		// Cleanup revenue donation table (remove outdated entires)
		CALL(_CleanupRevenueDonation, locals.cleanupInput, locals.cleanupOutput);
	}


	struct INITIALIZE_locals
	{
		RevenueDonationEntry revenueDonationEntry;
		Success_output success;
	};

	INITIALIZE_WITH_LOCALS()
	{
		// All works with zeroed state, but:
		// In the construction epoch 123, directly add the 15% revenue donation to the Supply Watcher contract,
		// which has been accepted by quorum with the old proposal system
		locals.revenueDonationEntry.destinationPublicKey = id(7, 0, 0, 0);
		locals.revenueDonationEntry.millionthAmount = 150 * 1000;
		locals.revenueDonationEntry.firstEpoch = 123;
		CALL(_SetRevenueDonationEntry, locals.revenueDonationEntry, locals.success);
	}
};
