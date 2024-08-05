#pragma once

#include "../contracts/qpi.h"

namespace QPI
{

	template <uint16 proposalSlotCount>
	struct ProposalAndVotingByComputors
	{
		// Maximum number of proposals (may be lower than number of proposers = IDs with right to propose and lower than num. of voters)
		static constexpr uint16 maxProposals = proposalSlotCount;

		// Maximum number of voters
		static constexpr uint32 maxVoters = NUMBER_OF_COMPUTORS;

		// Check if proposer has right to propose (and is not NULL_ID)
		bool isValidProposer(const QpiContextFunctionCall& qpi, const id& proposerId) const
		{
			// Check if proposer is currently a computor (voter index is computor index here)
			return getVoterIndex(qpi, proposerId) < maxVoters;
		}

		// Get new proposal slot (each propser may have at most one).
		// Returns proposal index or INVALID_PROPOSAL_INDEX on error.
		// CAUTION: Only pass valid proposers!
		uint16 getNewProposalIndex(const QpiContextFunctionCall& qpi, const id& proposerId)
		{
			// Reuse slot if proposer has existing proposal
			uint16 idx = getExistingProposalIndex(qpi, proposerId);
			if (idx < maxProposals)
			{
				currentProposalProposers[idx] = proposerId;
				return idx;
			}

			// Otherwise, try to find empty slot
			for (idx = 0; idx < maxProposals; ++idx)
			{
				if (isZero(currentProposalProposers[idx]))
				{
					currentProposalProposers[idx] = proposerId;
					return idx;
				}
			}

			// No empty slot -> fail
			return INVALID_PROPOSAL_INDEX;
		}

		void freeProposalByIndex(const QpiContextFunctionCall& qpi, uint16 proposalIndex)
		{
			if (proposalIndex < maxProposals)
				currentProposalProposers[proposalIndex] = NULL_ID;
		}

		// Return proposer ID for given proposal index or NULL_ID if there is no proposal
		id getProposerId(const QpiContextFunctionCall& qpi, uint16 proposalIndex) const
		{
			if (proposalIndex >= maxProposals)
				return NULL_ID;
			return currentProposalProposers[proposalIndex];
		}

		// Get new index of existing used proposal of proposer if any; only pass valid proposers!
		// Returns proposal index or INVALID_PROPOSAL_INDEX if there is no proposal for given proposer..
		uint16 getExistingProposalIndex(const QpiContextFunctionCall& qpi, const id& proposerId) const
		{
			if (isZero(proposerId))
				return INVALID_PROPOSAL_INDEX;
			for (uint16 i = 0; i < maxProposals; ++i)
			{
				if (currentProposalProposers[i] == proposerId)
					return i;
			}
			return INVALID_PROPOSAL_INDEX;
		}

		// Get voter index for given ID or INVALID_VOTER_INDEX if has no right to vote
		// Voter index is computor index
		uint32 getVoterIndex(const QpiContextFunctionCall& qpi, const id& voterId) const
		{
			// NULL_ID is invalid
			if (isZero(voterId))
				return INVALID_VOTER_INDEX;

			for (uint16 compIdx = 0; compIdx < maxVoters; ++compIdx)
			{
				if (qpi.computor(compIdx) == voterId)
					return compIdx;
			}
			return INVALID_VOTER_INDEX;
		}

		// Return voter ID for given voter index or NULL_ID on error
		id getVoterId(const QpiContextFunctionCall& qpi, uint16 voterIndex) const
		{
			if (voterIndex >= maxVoters)
				return NULL_ID;
			return qpi.computor(voterIndex);
		}

	protected:
		// TODO: maybe replace by hash map?
		// needs to be initialized with zeros
		id currentProposalProposers[maxProposals];
	};

	template <unsigned int maxShareholders>
	struct ProposalAndVotingByShareholders
	{
	};

	// Used internally by ProposalVoting to store a proposal with all votes
	template <typename ProposalDataType, uint32 numOfVoters>
	struct ProposalWithAllVoteData : public ProposalDataType
	{
		// votes: type 2 uses s64 with NO_VOTE_VALUE, others use u8 with 0xff to encode missing vote
		union {
			uint8 u8[numOfVoters];
			sint64 s64[numOfVoters];
		} votes;

		// set proposal and reset all votes
		void set(const ProposalDataType& proposal)
		{
			copyMemory(*(ProposalDataType*)this, proposal);

			if (this->type == 0)
			{
				// yes/no voting (1 byte per voter)
				constexpr uint8 noVoteValue = 0xff;
				setMemory(votes.u8, noVoteValue);
			}
			else
			{
				// scalar variable voting
				for (uint32 i = 0; i < numOfVoters; ++i)
					votes.s64[i] = NO_VOTE_VALUE;
			}
		}

		// Set vote value of given voter as used in SingleProposalVoteData
		bool setVoteValue(uint32 voterIndex, sint64 voteValue)
		{
			bool ok = false;
			if (voterIndex < numOfVoters)
			{
				switch (this->type)
				{
				case 0:
					if (voteValue == NO_VOTE_VALUE)
						votes.u8[voterIndex] = 0xff;
					else if (voteValue == 0)
						votes.u8[voterIndex] = 0;
					else
						votes.u8[voterIndex] = 1;
					ok = true;
					break;
				case 1:
					if (voteValue >= 0 || voteValue == NO_VOTE_VALUE)
					{
						votes.s64[voterIndex] = voteValue;
						ok = true;
					}
					break;
				case 2:
					if ((voteValue >= this->variableValue.minValue && voteValue <= this->variableValue.maxValue) ||
						voteValue == NO_VOTE_VALUE)
					{
						votes.s64[voterIndex] = voteValue;
						ok = true;
					}
					break;
				}
			}
			return ok;
		}

		// Get vote value of given voter as used in SingleProposalVoteData
		sint64 getVoteValue(uint32 voterIndex) const
		{
			sint64 vv = NO_VOTE_VALUE;
			if (voterIndex < numOfVoters)
			{
				switch (this->type)
				{
				case 0:
					vv = votes.u8[voterIndex];
					if (vv == 0xff)
						vv = NO_VOTE_VALUE;
					break;
				case 1:
				case 2:
					vv = votes.s64[voterIndex];
					break;
				}
			}
			return vv;
		}
	};

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType>::setProposal(
		const id& proposer,
		const ProposalDataType& proposal
	)
	{
		// epoch must be current epoch or 0 (to clear proposal)
		uint16 curEpoch = this->qpi.epoch();
		if (proposal.epoch != curEpoch)
		{
			if (proposal.epoch == 0)
				return clearProposal(this->pv.proposersAndVoters.getExistingProposalIndex(this->qpi, proposer));
			else
				return false;
		}

		// check if proposal is valid
		if (!proposal.checkValidity())
			return false;

		// check if proposer ID has right to propose
		if (!this->pv.proposersAndVoters.isValidProposer(this->qpi, proposer))
			return false;

		// get proposal index
		unsigned int proposalIndex = this->pv.proposersAndVoters.getNewProposalIndex(this->qpi, proposer);
		if (proposalIndex >= this->pv.maxProposals)
		{
			// no empty slots -> try to free a slot of oldest proposal before current epoch
			uint16 minEpoch = curEpoch;
			for (uint16 i = 0; i < this->pv.maxProposals; ++i)
			{
				if (this->pv.proposals[i].epoch < minEpoch)
				{
					proposalIndex = i;
					minEpoch = this->pv.proposals[i].epoch;
				}
			}

			// all occupied slots are used in current epoch? -> error
			if (proposalIndex >= this->pv.maxProposals)
				return false;
			
			// remove oldest proposal
			clearProposal(proposalIndex);

			// call voters interface again in case it needs to register the proposer
			// TODO: add ASSERT, because this should always return the same value if the interface is implemented correctly
			proposalIndex = this->pv.proposersAndVoters.getNewProposalIndex(this->qpi, proposer);
		}

		// set proposal (and reset previous votes if any)
		this->pv.proposals[proposalIndex].set(proposal);

		return true;
	}

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType>::clearProposal(
		uint16 proposalIndex
	)
	{
		if (proposalIndex >= this->pv.maxProposals)
			return false;
		this->pv.proposersAndVoters.freeProposalByIndex(this->qpi, proposalIndex);
		setMemory(this->pv.proposals[proposalIndex], 0);
		return true;
	}


	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType>::vote(
		const id& voter,
		const SingleProposalVoteData& vote
	)
	{
		if (vote.proposalIndex >= this->pv.maxProposals)
			return false;

		auto& proposal = this->pv.proposals[vote.proposalIndex];
		if (vote.proposalType != proposal.type || this->qpi.epoch() != proposal.epoch)
			return false;

		unsigned int voterIndex = this->pv.proposersAndVoters.getVoterIndex(this->qpi, voter);
		return proposal.setVoteValue(voterIndex, vote.voteValue);
	}

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::getProposal(
		uint16 proposalIndex,
		ProposalDataType& proposal
	) const
	{
		if (proposalIndex >= pv.maxProposals || !pv.proposals[proposalIndex].epoch)
			return false;
		const ProposalDataType& storedProposal = *static_cast<ProposalDataType*>(&pv.proposals[proposalIndex]);
		copyMemory(proposal, storedProposal);
		return true;
	}

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::getVote(
		uint16 proposalIndex,
		uint32 voterIndex,
		SingleProposalVoteData& vote
	) const
	{
		if (proposalIndex >= pv.maxProposals || voterIndex >= pv.maxVoters || !pv.proposals[proposalIndex].epoch)
			return false;

		vote.proposalIndex = proposalIndex;
		vote.proposalType = pv.proposals[proposalIndex].type;
		vote.voteValue = pv.proposals[proposalIndex].getVoteValue(voterIndex);
		return true;
	}

	// Return index of existing proposal or INVALID_PROPOSAL_INDEX if there is no proposal by given proposer
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	uint16 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::proposalIndex(
		const id& proposerId
	) const
	{
		return pv.proposersAndVoters.getExistingProposalIndex(qpi, proposerId);
	}

	// Return proposer ID of given proposal index or NULL_ID if there is no proposal at this index
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	id QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::proposerId(
		uint16 proposalIndex
	) const
	{
		if (proposalIndex >= pv.maxProposals || !pv.proposals[proposalIndex].epoch)
			return NULL_ID;
		return pv.proposersAndVoters.getProposerId(qpi, proposalIndex);
	}

	// Return voter index for given ID or INVALID_VOTER_INDEX if ID has no right to vote
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	uint32 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::voterIndex(
		const id& voterId
	) const
	{
		return pv.proposersAndVoters.getVoterIndex(qpi, voterId);
	}

	// Return ID for given voter index or NULL_ID if index is invalid
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	id QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::voterId(
		uint32 voterIndex
	) const
	{
		return pv.proposersAndVoters.getVoterId(qpi, voterIndex);
	}

	// Return next proposal index of active proposal (voting possible this epoch)
	// or -1 if there are not any more such proposals behind the passed index.
	// Pass -1 to get first index.
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	sint32 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::nextActiveProposalIndex(
		sint32 prevProposalIndex
	) const
	{
		if (prevProposalIndex >= pv.maxProposals)
			return -1;

		uint16 idx = (prevProposalIndex < 0) ? 0 : prevProposalIndex + 1;
		while (idx < pv.maxProposals)
		{
			if (pv.proposals[idx].epoch == qpi.epoch())
				return idx;
			++idx;
		}

		return -1;
	}

	// Return next proposal index of finished proposal (voting not possible anymore)
	// or -1 if there are not any more such proposals behind the passed index.
	// Pass -1 to get first index.
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	sint32 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::nextFinishedProposalIndex(
		sint32 prevProposalIndex
	) const
	{
		if (prevProposalIndex >= pv.maxProposals)
			return -1;

		uint16 idx = (prevProposalIndex < 0) ? 0 : prevProposalIndex + 1;
		while (idx < pv.maxProposals)
		{
			uint16 idxEpoch = pv.proposals[idx].epoch;
			if (idxEpoch != qpi.epoch() && idxEpoch != 0)
				return idx;
			++idx;
		}

		return -1;
	}

	// Access proposal functions with qpi(proposalVotingObject).func().
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	inline QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType> QpiContextFunctionCall::operator()(
		const ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
	) const
	{
		return QpiContextProposalFunctionCall(*this, proposalVoting);
	}

	// Access proposal procedures with qpi(proposalVotingObject).proc().
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	inline QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType> QpiContextProcedureCall::operator()(
		ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
		) const
	{
		return QpiContextProposalProcedureCall(*this, proposalVoting);
	}
}
