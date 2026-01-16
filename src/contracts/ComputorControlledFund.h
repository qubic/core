using namespace QPI;

constexpr uint32 CCF_MAX_SUBSCRIPTIONS = 1024;

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

	// Only computors can set a proposal and vote. Up to 100 proposals are supported simultaneously.
    typedef ProposalAndVotingByComputors<100> ProposersAndVotersT;

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

	// Subscription proposal data (for proposals being voted on)
	struct SubscriptionProposalData
	{
		id proposerId;                  // ID of the proposer (for cancellation checks)
		id destination;                 // ID of the destination
		Array<uint8, 256> url;          // URL of the subscription
		uint8 weeksPerPeriod;			// Number of weeks between payments (e.g., 1 for weekly, 4 for monthly)
		Array<uint8, 1> _padding0;		// Padding for alignment
		Array<uint8, 2> _padding1;      // Padding for alignment
		uint32 numberOfPeriods;			// Total number of periods (e.g., 12 for 12 periods)
		uint64 amountPerPeriod;			// Amount in Qubic per period
		uint32 startEpoch;				// Epoch when subscription should start
	};

	// Active subscription data (for accepted subscriptions)
	struct SubscriptionData
	{
		id destination;                 // ID of the destination (used as key, one per destination)
		Array<uint8, 256> url;          // URL of the subscription
		uint8 weeksPerPeriod;			// Number of weeks between payments (e.g., 1 for weekly, 4 for monthly)
		Array<uint8, 1> _padding1;		// Padding for alignment
		Array<uint8, 2> _padding2;		// Padding for alignment
		uint32 numberOfPeriods;			// Total number of periods (e.g., 12 for 12 periods)
		uint64 amountPerPeriod;			// Amount in Qubic per period
		uint32 startEpoch;				// Epoch when subscription started (startEpoch >= proposal approval epoch)
		sint32 currentPeriod;			// Current period index (0-based, 0 to numberOfPeriods-1)
	};

	// Array to store subscription proposals, one per proposal slot (indexed by proposalIndex)
	typedef Array<SubscriptionProposalData, 128> SubscriptionProposalsT;
	
	// Array to store active subscriptions, indexed by destination ID
	typedef Array<SubscriptionData, CCF_MAX_SUBSCRIPTIONS> ActiveSubscriptionsT;

	// Regular payment entry (similar to LatestTransfersEntry but for subscriptions)
	struct RegularPaymentEntry
	{
		id destination;
		Array<uint8, 256> url;
		sint64 amount;
		uint32 tick;
		sint32 periodIndex;				// Which period this payment is for (0-based)
		bool success;
		Array<uint8, 1> _padding0;
		Array<uint8, 2> _padding1;
	};

	typedef Array<RegularPaymentEntry, 128> RegularPaymentsT;

protected:
	//----------------------------------------------------------------------------
	// Define state
	ProposalVotingT proposals;

	LatestTransfersT latestTransfers;
	uint8 lastTransfersNextOverwriteIdx;

	uint32 setProposalFee;

	RegularPaymentsT regularPayments;

	SubscriptionProposalsT subscriptionProposals;	// Subscription proposals, one per proposal slot (indexed by proposalIndex)
	ActiveSubscriptionsT activeSubscriptions;		// Active subscriptions, identified by destination ID

	uint8 lastRegularPaymentsNextOverwriteIdx;

	//----------------------------------------------------------------------------
	// Define private procedures and functions with input and output


public:
	//----------------------------------------------------------------------------
	// Define public procedures and functions with input and output

	// Extended input for SetProposal that includes optional subscription data
	struct SetProposal_input
	{
		ProposalDataT proposal;
		// Optional subscription data (only used if isSubscription is true)
		bit isSubscription;				// Set to true if this is a subscription proposal
		uint8 weeksPerPeriod;			// Number of weeks between payments (e.g., 1 for weekly, 4 for monthly)
		Array<uint8, 2> _padding0;		// Padding for alignment
		uint32 startEpoch;				// Epoch when subscription starts
		uint64 amountPerPeriod;			// Amount per period (in Qubic)
		uint32 numberOfPeriods;			// Total number of periods
	};

	struct SetProposal_output
	{
		uint16 proposalIndex;
	};

	struct SetProposal_locals
	{
		uint32 totalEpochsForSubscription;
		sint32 subIndex;
		SubscriptionProposalData subscriptionProposal;
		ProposalDataT proposal;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SetProposal)
	{
		if (qpi.invocationReward() < state.setProposalFee)
		{
			// Invocation reward not sufficient, undo payment and cancel
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.proposalIndex = INVALID_PROPOSAL_INDEX;
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
		if (ProposalTypes::cls(input.proposal.type) != ProposalTypes::Class::Transfer)
		{
			// Only transfer proposals are allowed
			// -> Cancel if epoch is not 0 (which means clearing the proposal)
			if (input.proposal.epoch != 0)
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}
		}

		// Validate subscription data if provided
		if (input.isSubscription)
		{
			// Validate start epoch
			if (input.startEpoch < qpi.epoch())
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}

			// Calculate total epochs for this subscription
			// 1 week = 1 epoch
			locals.totalEpochsForSubscription = input.numberOfPeriods * input.weeksPerPeriod;

			// Check against total allowed subscription time range
			if (locals.totalEpochsForSubscription > 52)
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}
		}

		// Try to set proposal (checks originators rights and general validity of input proposal)
		output.proposalIndex = qpi(state.proposals).setProposal(qpi.originator(), input.proposal);

		// Handle subscription proposals
		if (output.proposalIndex != INVALID_PROPOSAL_INDEX && input.isSubscription)
		{
			// If proposal is being cleared (epoch 0), clear the subscription proposal
			if (input.proposal.epoch == 0)
			{
				// Check if this is a subscription proposal that can be canceled by the proposer
				if (output.proposalIndex < state.subscriptionProposals.capacity())
				{
					locals.subscriptionProposal = state.subscriptionProposals.get(output.proposalIndex);
					// Only allow cancellation by the proposer
					// The value of below condition should be always true, but set the else condition for safety
					if (locals.subscriptionProposal.proposerId == qpi.originator())
					{
						// Clear the subscription proposal
						setMemory(locals.subscriptionProposal, 0);
						state.subscriptionProposals.set(output.proposalIndex, locals.subscriptionProposal);
					}
					else
					{
						output.proposalIndex = INVALID_PROPOSAL_INDEX;
					}
				}
			}
			else
			{
				// Check if there's already an active subscription for this destination
				// Only the proposer can create a new subscription proposal, but any valid proposer
				// can propose changes to an existing subscription (which will be handled in END_EPOCH)
				// For now, we allow the proposal to be created - it will overwrite the existing subscription if accepted
				
				// Store subscription proposal data in the array indexed by proposalIndex
				locals.subscriptionProposal.proposerId = qpi.originator();
				locals.subscriptionProposal.destination = input.proposal.transfer.destination;
				copyMemory(locals.subscriptionProposal.url, input.proposal.url);
				locals.subscriptionProposal.weeksPerPeriod = input.weeksPerPeriod;
				locals.subscriptionProposal.numberOfPeriods = input.numberOfPeriods;
				locals.subscriptionProposal.amountPerPeriod = input.amountPerPeriod;
				locals.subscriptionProposal.startEpoch = input.startEpoch;
				state.subscriptionProposals.set(output.proposalIndex, locals.subscriptionProposal);
			}
		}
		else if (output.proposalIndex != INVALID_PROPOSAL_INDEX && !input.isSubscription)
		{
			// Clear any subscription proposal at this index if it exists
			if (output.proposalIndex >= 0 && output.proposalIndex < state.subscriptionProposals.capacity())
			{
				setMemory(locals.subscriptionProposal, 0);
				state.subscriptionProposals.set(output.proposalIndex, locals.subscriptionProposal);
			}
		}
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
		id subscriptionDestination;		// Destination ID to look up active subscription (optional, can be zero)
		uint16 proposalIndex;
	};
	struct GetProposal_output
	{
		bit okay;
		bit hasSubscriptionProposal;					// True if this proposal has subscription proposal data
		bit hasActiveSubscription;						// True if an active subscription was found for the destination
		Array<uint8, 1> _padding0;
		Array<uint8, 4> _padding1;
		id proposerPublicKey;
		ProposalDataT proposal;
		SubscriptionData subscription;					// Active subscription data if found
		SubscriptionProposalData subscriptionProposal;	// Subscription proposal data if this is a subscription proposal
	};

	struct GetProposal_locals
	{
		sint32 subIndex;
		SubscriptionData subscriptionData;
		SubscriptionProposalData subscriptionProposalData;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetProposal)
	{
		output.proposerPublicKey = qpi(state.proposals).proposerId(input.proposalIndex);
		output.okay = qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
		output.hasSubscriptionProposal = false;
		output.hasActiveSubscription = false;

		// Check if this proposal has subscription proposal data
		if (input.proposalIndex < state.subscriptionProposals.capacity())
		{
			locals.subscriptionProposalData = state.subscriptionProposals.get(input.proposalIndex);
			if (!isZero(locals.subscriptionProposalData.proposerId))
			{
				output.subscriptionProposal = locals.subscriptionProposalData;
				output.hasSubscriptionProposal = true;
			}
		}

		// Look up active subscription by destination ID
		if (!isZero(input.subscriptionDestination))
		{
			for (locals.subIndex = 0; locals.subIndex < CCF_MAX_SUBSCRIPTIONS; ++locals.subIndex)
			{
				locals.subscriptionData = state.activeSubscriptions.get(locals.subIndex);
				if (locals.subscriptionData.destination == input.subscriptionDestination && !isZero(locals.subscriptionData.destination))
				{
					output.subscription = locals.subscriptionData;
					output.hasActiveSubscription = true;
					break;
				}
			}
		}
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
			qpi(state.proposals).voteIndex(input.voter),
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


	typedef NoData GetRegularPayments_input;
	typedef RegularPaymentsT GetRegularPayments_output;

	PUBLIC_FUNCTION(GetRegularPayments)
	{
		output = state.regularPayments;
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
		REGISTER_USER_FUNCTION(GetRegularPayments, 7);

		REGISTER_USER_PROCEDURE(SetProposal, 1);
		REGISTER_USER_PROCEDURE(Vote, 2);
	}


	INITIALIZE()
	{
		state.setProposalFee = 1000000;
	}


	struct END_EPOCH_locals
	{
		sint32 proposalIndex, subIdx;
		ProposalDataT proposal;
		ProposalSummarizedVotingDataV1 results;
		LatestTransfersEntry transfer;
		RegularPaymentEntry regularPayment;
		SubscriptionData subscription;
		SubscriptionProposalData subscriptionProposal;
		id proposerPublicKey;
		uint32 currentEpoch;
		uint32 epochsSinceStart;
		uint32 epochsPerPeriod;
		sint32 periodIndex;
		sint32 existingSubIdx;
		bit isSubscription;
	};

	END_EPOCH_WITH_LOCALS()
	{
		locals.currentEpoch = qpi.epoch();

		// Analyze transfer proposal results

		// Iterate all proposals that were open for voting in this epoch ...
		locals.proposalIndex = -1;
		while ((locals.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.proposalIndex, locals.currentEpoch)) >= 0)
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
				if (locals.results.totalVotesCasted < QUORUM)
					continue;

				// The transfer option (1) must have more votes than the no-transfer option (0)
				if (locals.results.optionVoteCount.get(1) < locals.results.optionVoteCount.get(0))
					continue;
				
				// Option for transfer has been accepted?
				if (locals.results.optionVoteCount.get(1) > div<uint32>(QUORUM, 2U))
				{
					// Check if this is a subscription proposal
					locals.isSubscription = false;
					if (locals.proposalIndex < state.subscriptionProposals.capacity())
					{
						locals.subscriptionProposal = state.subscriptionProposals.get(locals.proposalIndex);
						// Check if this slot has subscription proposal data (non-zero proposerId indicates valid entry)
						if (!isZero(locals.subscriptionProposal.proposerId))
						{
							locals.isSubscription = true;
						}
					}
					
					if (locals.isSubscription)
					{
						// Handle subscription proposal acceptance
						// If amountPerPeriod is 0 or numberOfPeriods is 0, delete the subscription
						if (locals.subscriptionProposal.amountPerPeriod == 0 || locals.subscriptionProposal.numberOfPeriods == 0 || locals.subscriptionProposal.weeksPerPeriod == 0)
						{
							// Find and delete the subscription by destination ID
							locals.existingSubIdx = -1;
							for (locals.subIdx = 0; locals.subIdx < CCF_MAX_SUBSCRIPTIONS; ++locals.subIdx)
							{
								locals.subscription = state.activeSubscriptions.get(locals.subIdx);
								if (locals.subscription.destination == locals.subscriptionProposal.destination && !isZero(locals.subscription.destination))
								{
									// Clear the subscription entry
									setMemory(locals.subscription, 0);
									state.activeSubscriptions.set(locals.subIdx, locals.subscription);
									break;
								}
							}
						}
						else
						{
							// Find existing subscription by destination ID or find a free slot
							locals.existingSubIdx = -1;
							for (locals.subIdx = 0; locals.subIdx < CCF_MAX_SUBSCRIPTIONS; ++locals.subIdx)
							{
								locals.subscription = state.activeSubscriptions.get(locals.subIdx);
								if (locals.subscription.destination == locals.subscriptionProposal.destination && !isZero(locals.subscription.destination))
								{
									locals.existingSubIdx = locals.subIdx;
									break;
								}
								// Track first free slot (zero destination)
								if (locals.existingSubIdx == -1 && isZero(locals.subscription.destination))
								{
									locals.existingSubIdx = locals.subIdx;
								}
							}

							// If found existing or free slot, update/create subscription
							if (locals.existingSubIdx >= 0)
							{
								locals.subscription.destination = locals.subscriptionProposal.destination;
								copyMemory(locals.subscription.url, locals.subscriptionProposal.url);
								locals.subscription.weeksPerPeriod = locals.subscriptionProposal.weeksPerPeriod;
								locals.subscription.numberOfPeriods = locals.subscriptionProposal.numberOfPeriods;
								locals.subscription.amountPerPeriod = locals.subscriptionProposal.amountPerPeriod;
								locals.subscription.startEpoch = locals.subscriptionProposal.startEpoch; // Use the start epoch from the proposal
								locals.subscription.currentPeriod = -1; // Reset to -1, will be updated when first payment is made
								state.activeSubscriptions.set(locals.existingSubIdx, locals.subscription);
							}
						}

						// Clear the subscription proposal
						setMemory(locals.subscriptionProposal, 0);
						state.subscriptionProposals.set(locals.proposalIndex, locals.subscriptionProposal);
					}
					else 
					{
						// Regular one-time transfer (no subscription data)
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

		// Process active subscriptions for regular payments
		// Iterate through all active subscriptions and check if payment is due
		for (locals.subIdx = 0; locals.subIdx < CCF_MAX_SUBSCRIPTIONS; ++locals.subIdx)
		{
			locals.subscription = state.activeSubscriptions.get(locals.subIdx);
			
			// Skip invalid subscriptions (zero destination indicates empty slot)
			if (isZero(locals.subscription.destination) || locals.subscription.numberOfPeriods == 0)
				continue;

			// Calculate epochs per period (1 week = 1 epoch)
			locals.epochsPerPeriod = locals.subscription.weeksPerPeriod;

			// Calculate how many epochs have passed since subscription started
			if (locals.currentEpoch < locals.subscription.startEpoch)
				continue; // Subscription hasn't started yet

			locals.epochsSinceStart = locals.currentEpoch - locals.subscription.startEpoch;

			// Calculate which period we should be in (0-based: 0 = first period, 1 = second period, etc.)
			// At the start of each period, we make a payment for that period
			// When startEpoch = 189 and currentEpoch = 189: epochsSinceStart = 0, periodIndex = 0 (first period)
			// When startEpoch = 189 and currentEpoch = 190: epochsSinceStart = 1, periodIndex = 1 (second period)
			locals.periodIndex = div<sint32>(locals.epochsSinceStart, locals.epochsPerPeriod);

			// Check if we need to make a payment for the current period
			// currentPeriod tracks the last period for which payment was made (or -1 if none)
			// We make payment at the start of each period, so when periodIndex > currentPeriod
			// For the first payment: currentPeriod = -1, periodIndex = 0, so we pay for period 0
			if (locals.periodIndex > locals.subscription.currentPeriod && locals.periodIndex < (sint32)locals.subscription.numberOfPeriods)
			{
				// Make payment for the current period
				locals.regularPayment.destination = locals.subscription.destination;
				locals.regularPayment.amount = locals.subscription.amountPerPeriod;
				locals.regularPayment.tick = qpi.tick();
				locals.regularPayment.periodIndex = locals.periodIndex;
				copyMemory(locals.regularPayment.url, locals.subscription.url);
				locals.regularPayment.success = (qpi.transfer(locals.regularPayment.destination, locals.regularPayment.amount) >= 0);

				// Update subscription current period to the period we just paid for
				locals.subscription.currentPeriod = locals.periodIndex;
				state.activeSubscriptions.set(locals.subIdx, locals.subscription);

				// Add log entry
				state.regularPayments.set(state.lastRegularPaymentsNextOverwriteIdx, locals.regularPayment);
				state.lastRegularPaymentsNextOverwriteIdx = (uint8)mod<uint64>(state.lastRegularPaymentsNextOverwriteIdx + 1, state.regularPayments.capacity());

				// Check if subscription has expired (all periods completed)
				if (locals.regularPayment.success && locals.subscription.currentPeriod >= (sint32)locals.subscription.numberOfPeriods - 1)
				{
					// Clear the subscription by zeroing out the entry (empty slot is indicated by zero destination)
					setMemory(locals.subscription, 0);
					state.activeSubscriptions.set(locals.subIdx, locals.subscription);
				}
			}
		}
	}

};



