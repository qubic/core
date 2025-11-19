using namespace QPI;

constexpr uint32 CCF_MAX_SUBSCRIPTIONS = 8192;
constexpr uint8 CCF_SUBSCRIPTION_PERIOD_WEEK = 1;
constexpr uint8 CCF_SUBSCRIPTION_PERIOD_MONTH = 2;

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

	struct SetProposal_output
	{
		sint32 proposalIndex;
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

	// Subscription data for regular payment proposals
	struct SubscriptionData
	{
		id proposerId;                  // ID of the proposer
		id destination;                 // ID of the destination
		Array<uint8, 256> url;          // URL of the subscription
		bit isActive;					// Whether this subscription is active
		uint8 periodType;				// SubscriptionPeriod::Week or SubscriptionPeriod::Month
		uint32 numberOfPeriods;			// Total number of periods (e.g., 12 for 12 months)
		sint64 amountPerPeriod;			// Amount in Qubic per period
		uint32 startEpoch;				// Epoch when subscription started (proposal approval epoch)
		sint32 currentPeriod;			// Current period index (0-based, 0 to numberOfPeriods-1)
		Array<uint8, 2> _padding;		// Padding for alignment
	};

	// Array to store subscription data, indexed by proposerId
	typedef Array<SubscriptionData, CCF_MAX_SUBSCRIPTIONS> SubscriptionsT;

	// Regular payment entry (similar to LatestTransfersEntry but for subscriptions)
	struct RegularPaymentEntry
	{
		id destination;
		Array<uint8, 256> url;
		sint64 amount;
		uint32 tick;
		sint32 periodIndex;				// Which period this payment is for (0-based)
		bool success;
		Array<uint8, 1> _padding1;
		Array<uint8, 1> _padding2;
		Array<uint8, 1> _padding3;
	};

	typedef Array<RegularPaymentEntry, 128> RegularPaymentsT;

protected:
	//----------------------------------------------------------------------------
	// Define state
	ProposalVotingT proposals;

	LatestTransfersT latestTransfers;
	uint8 lastTransfersNextOverwriteIdx;

	RegularPaymentsT regularPayments;
	uint8 lastRegularPaymentsNextOverwriteIdx;

	SubscriptionsT subscriptions;		// Subscription data indexed by proposerId

	uint32 setProposalFee;
	uint32 maxSubscriptionEpochs;		// Maximum total time range in epochs (configurable)

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
		uint8 periodType;				// SubscriptionPeriod::Week or SubscriptionPeriod::Month
		uint32 startEpoch;				// Epoch when subscription starts
		uint32 numberOfPeriods;			// Total number of periods
		sint64 amountPerPeriod;			// Amount per period (in Qubic)
		Array<uint8, 4> _padding;
	};

	typedef SetProposal_output SetProposal_output;

	struct SetProposal_locals
	{
		uint32 maxEpochsForSubscription;
		sint32 subIndex, spSlot;
		SubscriptionData subscriptionData;
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
			// Validate period type
			if (input.periodType != CCF_SUBSCRIPTION_PERIOD_WEEK && input.periodType != CCF_SUBSCRIPTION_PERIOD_MONTH)
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}

			// Validate number of periods
			if (input.numberOfPeriods == 0)
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}

			// Validate amount per period
			if (input.amountPerPeriod <= 0)
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}

			// Validate start epoch
			if (input.startEpoch <= qpi.epoch())
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}

			// Calculate maximum epochs for this subscription
			// Approximate: 1 week ≈ 1 epoch, 1 month ≈ 4 epochs
			
			if (input.periodType == CCF_SUBSCRIPTION_PERIOD_WEEK)
			{
				locals.maxEpochsForSubscription = input.numberOfPeriods; // 1 week ≈ 1 epoch
			}
			else // Month
			{
				locals.maxEpochsForSubscription = input.numberOfPeriods * 4; // 1 month ≈ 4 epochs
			}

			// Check against maximum allowed subscription time range
			if (locals.maxEpochsForSubscription > state.maxSubscriptionEpochs)
			{
				output.proposalIndex = INVALID_PROPOSAL_INDEX;
				return;
			}
		}

		// Try to set proposal (checks originators rights and general validity of input proposal)
		output.proposalIndex = qpi(state.proposals).setProposal(qpi.originator(), input.proposal);

		// If proposal was set successfully and it's a subscription, store subscription data
		if (output.proposalIndex != INVALID_PROPOSAL_INDEX && input.isSubscription)
		{
			// If proposal is being cleared (epoch 0), also cancel any active subscription for this proposer
			if (input.proposal.epoch == 0)
			{
				// Find and cancel any subscription for this proposer
				for (locals.subIndex = 0; locals.subIndex < CCF_MAX_SUBSCRIPTIONS; ++locals.subIndex)
				{
					locals.subscriptionData = state.subscriptions.get(locals.subIndex);
					if (locals.subscriptionData.proposerId == qpi.originator())
					{
						locals.subscriptionData.isActive = false;
						state.subscriptions.set(locals.subIndex, locals.subscriptionData);
					}
				}
			}
			else 
			{
				locals.spSlot = -1;
				for (locals.subIndex = 0; locals.subIndex < CCF_MAX_SUBSCRIPTIONS; ++locals.subIndex)
				{
					locals.subscriptionData = state.subscriptions.get(locals.subIndex);

					// If the proposer has an active subscription, cancel the proposal
					if (locals.subscriptionData.proposerId == qpi.originator() && locals.subscriptionData.isActive == true)
					{
						locals.proposal = input.proposal;
						locals.proposal.epoch = 0;
						qpi(state.proposals).setProposal(qpi.originator(), locals.proposal);
						output.proposalIndex = INVALID_PROPOSAL_INDEX;
						return;
					}

					// If the element is inactive, find a free slot
					if (locals.subscriptionData.isActive == false && locals.spSlot == -1)
					{
						locals.spSlot = locals.subIndex;
					}
				}

				// If a free slot was found, store the subscription data
				if (locals.spSlot != -1)
				{
					locals.subscriptionData.proposerId = qpi.originator();
					locals.subscriptionData.destination = input.proposal.transfer.destination;
					copyMemory(locals.subscriptionData.url, input.proposal.url);
					locals.subscriptionData.isActive = true;
					locals.subscriptionData.periodType = input.periodType;
					locals.subscriptionData.numberOfPeriods = input.numberOfPeriods;
					locals.subscriptionData.amountPerPeriod = input.amountPerPeriod;
					locals.subscriptionData.startEpoch = input.startEpoch;
					locals.subscriptionData.currentPeriod = -1;
					state.subscriptions.set(locals.spSlot, locals.subscriptionData);
				}
				else
				{
					output.proposalIndex = INVALID_PROPOSAL_INDEX;
				}
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
		id subscriptionProposerId;
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
		SubscriptionData subscription;
	};

	struct GetProposal_locals
	{
		sint32 subIndex;
		SubscriptionData subscriptionData;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetProposal)
	{

		output.proposerPubicKey = qpi(state.proposals).proposerId(input.proposalIndex);
		output.okay = qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
		for (locals.subIndex = 0; locals.subIndex < CCF_MAX_SUBSCRIPTIONS; ++locals.subIndex)
		{
			locals.subscriptionData = state.subscriptions.get(locals.subIndex);
			if (locals.subscriptionData.proposerId == input.subscriptionProposerId && locals.subscriptionData.isActive == true)
			{
				output.subscription = locals.subscriptionData;
				break;
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
		state.maxSubscriptionEpochs = 52; // Default: 52 epochs (approximately 1 year if 1 epoch ≈ 1 week)
	}


	struct END_EPOCH_locals
	{
		sint32 proposalIndex, subIdx;
		ProposalDataT proposal;
		ProposalSummarizedVotingDataV1 results;
		LatestTransfersEntry transfer;
		RegularPaymentEntry regularPayment;
		SubscriptionData subscription;
		id proposerPubicKey;
		uint32 currentEpoch;
		uint32 epochsSinceStart;
		uint32 epochsPerPeriod;
		sint32 periodIndex;
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

			locals.proposerPubicKey = qpi(state.proposals).proposerId(locals.proposalIndex);
			locals.isSubscription = false;
			// Inactive the proposal before passed by voting, it will be actived again after passed by voting. if it is not passed by voting, it will not be actived again.
			for (locals.subIdx = 0; locals.subIdx < CCF_MAX_SUBSCRIPTIONS; ++locals.subIdx)
			{
				locals.subscription = state.subscriptions.get(locals.subIdx);
				if (locals.subscription.proposerId == locals.proposerPubicKey && locals.subscription.isActive == true)
				{
					locals.subscription.isActive = false;
					state.subscriptions.set(locals.subIdx, locals.subscription);
					locals.isSubscription = true;
					break;
				}
			}
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
					if (locals.isSubscription)
					{
						locals.subscription.isActive = true;
						state.subscriptions.set(locals.subIdx, locals.subscription);
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
		// Iterate through all subscriptions and check if payment is due
		for (locals.subIdx = 0; locals.subIdx < CCF_MAX_SUBSCRIPTIONS; ++locals.subIdx)
		{
			locals.subscription = state.subscriptions.get(locals.subIdx);
			
			// Skip inactive or invalid subscriptions
			if (!locals.subscription.isActive || locals.subscription.numberOfPeriods == 0)
				continue;

			// Check if subscription has expired (all periods completed)
			if (locals.subscription.currentPeriod >= (sint32)locals.subscription.numberOfPeriods)
				continue;

			// Calculate epochs per period
			if (locals.subscription.periodType == CCF_SUBSCRIPTION_PERIOD_WEEK)
			{
				locals.epochsPerPeriod = 1; // 1 week ≈ 1 epoch
			}
			else // Month
			{
				locals.epochsPerPeriod = 4; // 1 month ≈ 4 epochs
			}

			// Calculate how many epochs have passed since subscription started
			if (locals.currentEpoch < locals.subscription.startEpoch)
				continue; // Should not happen, but safety check

			locals.epochsSinceStart = locals.currentEpoch - locals.subscription.startEpoch;

			// Calculate which period we should be in (0-based: 0 = first period, 1 = second period, etc.)
			// At the start of each period, we make a payment for that period
			locals.periodIndex = div<sint32>(locals.epochsSinceStart, locals.epochsPerPeriod);

			// Check if we need to make a payment for the current period
			// currentPeriod tracks the last period for which payment was made
			// We make payment at the start of each period, so when periodIndex > currentPeriod
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
				state.subscriptions.set(locals.subIdx, locals.subscription);

				// Add log entry
				state.regularPayments.set(state.lastRegularPaymentsNextOverwriteIdx, locals.regularPayment);
				state.lastRegularPaymentsNextOverwriteIdx = (state.lastRegularPaymentsNextOverwriteIdx + 1) & (state.regularPayments.capacity() - 1);
			}
		}
	}

};



