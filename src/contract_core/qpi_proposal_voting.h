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

		// Get new proposal slot (each proposer may have at most one).
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

		// Get voter index for given ID or INVALID_VOTER_INDEX if has no right to vote.
		// Voter index is computor index
		uint32 getVoterIndex(const QpiContextFunctionCall& qpi, const id& voterId, uint16 proposalIndex = 0) const
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

		// Get count of votes of a voter specified by voter index (return 0 on error).
		uint32 getVoteCount(const QpiContextFunctionCall& qpi, uint32 voterIndex, uint16 proposalIndex = 0) const
		{
			if (voterIndex >= maxVoters)
				return 0;

			return 1;
		}

		// Return voter ID for given voter index or NULL_ID on error
		id getVoterId(const QpiContextFunctionCall& qpi, uint32 voterIndex, uint16 proposalIndex = 0) const
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

	template <uint16 proposalSlotCount>
	struct ProposalByAnyoneVotingByComputors : public ProposalAndVotingByComputors<proposalSlotCount>
	{
		// Check if proposer has right to propose (and is not NULL_ID)
		bool isValidProposer(const QpiContextFunctionCall& qpi, const id& proposerId) const
		{
			return !isZero(proposerId);
		}
	};

	// Option for ProposerAndVoterHandlingT in ProposalVoting that allows both voting and setting proposals for contract shareholders only.
	// A shareholder can have multiple votes and each may be set individually. Voting rights are assigned to current shareholders when a proposal
	// is created or overwritten and cannot be sold or transferred afterwards.
	template <uint16 proposalSlotCount, uint64 contractAssetName>
	struct ProposalAndVotingByShareholders
	{
		// Maximum number of proposals (may be lower than number of proposers = IDs with right to propose and lower than num. of voters)
		static constexpr uint16 maxProposals = proposalSlotCount;

		// Maximum number of voters
		static constexpr uint32 maxVoters = NUMBER_OF_COMPUTORS;

		// Check if proposer has right to propose (and is not NULL_ID)
		bool isValidProposer(const QpiContextFunctionCall& qpi, const id& proposerId) const
		{
			return qpi.numberOfShares({ NULL_ID, contractAssetName }, AssetOwnershipSelect::byOwner(proposerId), AssetPossessionSelect::byPossessor(proposerId)) > 0;
		};

		// Setup proposal in proposal index. Asset possession at this point in time defines the right to vote.
		void setupNewProposal(const QpiContextFunctionCall& qpi, const id& proposerId, uint16 propsalIdx)
		{
			if (propsalIdx >= maxProposals || isZero(proposerId))
				return;

			currentProposalProposers[propsalIdx] = proposerId;

			// prepare temporary array to gather shareholder info
			struct Shareholder
			{
				id possessor;
				sint64 shares;
			};
			Shareholder* shareholders = reinterpret_cast<Shareholder*>(__scratchpad(sizeof(Shareholder) * maxVoters));
			int lastShareholderIdx = -1;

			// gather shareholder info in sorted array
			for (AssetPossessionIterator iter({ NULL_ID, contractAssetName }); !iter.reachedEnd(); iter.next())
			{
				if (iter.numberOfPossessedShares() > 0)
				{
					// search sorted array backwards
					// (iter will provide possessors mostly in increasing order leading to low number of search
					// and move iterations)
					const id& possessor = iter.possessor();
					int idx = lastShareholderIdx;
					while (idx >= 0 && possessor < shareholders[idx].possessor)
					{
						--idx;
					}
					++idx;

					// update array: idx is the position to insert at with ID[idx] >= NewID
					if (idx < lastShareholderIdx && shareholders[idx].possessor == possessor)
					{
						// possessor is already in array -> increase number of shares
						shareholders[idx].shares += iter.numberOfPossessedShares();
					}
					else
					{
						// possessor is not in array yet -> add it to the right place (after moving items if needed)
						for (int idxMove = lastShareholderIdx; idxMove >= idx; --idxMove)
						{
							shareholders[idxMove + 1] = shareholders[idxMove];
						}
						shareholders[idx].possessor = possessor;
						shareholders[idx].shares = iter.numberOfPossessedShares();
						++lastShareholderIdx;
					}
				}
			}

#ifndef NDEBUG
			// sanity check of array (sorted, has expected size, and 676 shares in total)
			ASSERT(lastShareholderIdx >= 0);
			ASSERT(lastShareholderIdx < maxVoters);
			sint64 totalShares = 0;
			for (int idx = 0; idx < lastShareholderIdx; ++idx)
			{
				ASSERT(shareholders[idx].possessor < shareholders[idx + 1].possessor);
				ASSERT(shareholders[idx].shares > 0);
				totalShares += shareholders[idx].shares;
			}
			ASSERT(shareholders[lastShareholderIdx].shares > 0);
			totalShares += shareholders[lastShareholderIdx].shares;
			ASSERT(totalShares == maxVoters);
#endif

			// build sorted array of voters (one entry per share)
			int voterIdx = 0;
			for (int shareholderIdx = 0; shareholderIdx <= lastShareholderIdx; ++shareholderIdx)
			{
				const Shareholder& shareholder = shareholders[shareholderIdx];
				for (int shareIdx = 0; shareIdx < shareholder.shares; ++shareIdx)
				{
					currentProposalShareholders[propsalIdx][voterIdx] = shareholder.possessor;
					++voterIdx;
				}
			}
			ASSERT(voterIdx == maxVoters);
		}

		// Get new proposal slot (each proposer may have at most one).
		// Returns proposal index or INVALID_PROPOSAL_INDEX on error.
		// CAUTION: Only pass valid proposers!
		uint16 getNewProposalIndex(const QpiContextFunctionCall& qpi, const id& proposerId)
		{
			// Reuse slot if proposer has existing proposal
			uint16 idx = getExistingProposalIndex(qpi, proposerId);
			if (idx < maxProposals)
			{
				setupNewProposal(qpi, proposerId, idx);
				return idx;
			}

			// Otherwise, try to find empty slot
			for (idx = 0; idx < maxProposals; ++idx)
			{
				if (isZero(currentProposalProposers[idx]))
				{
					setupNewProposal(qpi, proposerId, idx);
					return idx;
				}
			}

			// No empty slot -> fail
			return INVALID_PROPOSAL_INDEX;
		}

		void freeProposalByIndex(const QpiContextFunctionCall& qpi, uint16 proposalIndex)
		{
			if (proposalIndex < maxProposals)
			{
				currentProposalProposers[proposalIndex] = NULL_ID;
				setMem(currentProposalShareholders[proposalIndex], sizeof(id) * maxVoters, 0);
			}
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

		// Return voter index for given ID or INVALID_VOTER_INDEX if ID has no right to vote. If the voter has multiple
		// votes, this returns the first index. All votes of a voter are stored consecutively.
		uint32 getVoterIndex(const QpiContextFunctionCall& qpi, const id& voterId, uint16 proposalIndex) const
		{
			// NULL_ID is invalid
			if (isZero(voterId) || proposalIndex >= maxProposals)
				return INVALID_VOTER_INDEX;

			// Search for first voter index with voterId
			// Note: This may be speeded up a bit because the array is sorted, but it is required to return the first element
			//       in a set of duplicates.
			for (uint16 voterIdx = 0; voterIdx < maxVoters; ++voterIdx)
			{
				if (currentProposalShareholders[proposalIndex][voterIdx] == voterId)
					return voterIdx;
			}

			return INVALID_VOTER_INDEX;
		}

		// Get count of votes of a voter specified by voter index (return 0 on error).
		uint32 getVoteCount(const QpiContextFunctionCall& qpi, uint32 voterIndex, uint16 proposalIndex) const
		{
			if (voterIndex >= maxVoters || proposalIndex >= maxProposals)
				return 0;

			const id* shareholders = currentProposalShareholders[proposalIndex];
			uint32 count = 1;
			const id& voterId = shareholders[voterIndex];
			for (uint32 idx = voterIndex + 1; idx < maxVoters; ++idx)
			{
				if (shareholders[idx] != voterId)
					break;
				++count;
			}

			return count;
		}

		// Return voter ID for given voter index or NULL_ID on error
		id getVoterId(const QpiContextFunctionCall& qpi, uint32 voterIndex, uint16 proposalIndex) const
		{
			if (voterIndex >= maxVoters || proposalIndex >= maxProposals)
				return NULL_ID;
			return currentProposalShareholders[proposalIndex][voterIndex];
		}

		// 1 Shareholder-ID kann N votes haben!!!
		// Stimmrechte werden bei Erstellung des Proposals zugeordnet, Verkaufen von Shares hat keinen Einfluss

		// ProposalSingleVoteDataV2 wird superset von V1 für mehrere Votes
		// ProposalSingleVoteDataV1 (neue Felder von V2 = 0) setzt alle votes



		// Vote-Content für Var-Set wird in contract gespeichert: -> var set options idx in proposal

	protected:
		// needs to be initialized with zeros
		id currentProposalProposers[maxProposals];
		id currentProposalShareholders[maxProposals][NUMBER_OF_COMPUTORS];
	};

	// Check if given type is valid (supported by most comprehensive ProposalData class).
	inline bool ProposalTypes::isValid(uint16 proposalType)
	{
		bool valid = false;
		uint16 cls = ProposalTypes::cls(proposalType);
		uint16 options = ProposalTypes::optionCount(proposalType);
		switch (cls)
		{
		case ProposalTypes::Class::GeneralOptions:
		case ProposalTypes::Class::MultiVariables:
			valid = (options >= 2 && options <= 8);
			break;
		case ProposalTypes::Class::Transfer:
			valid = (options >= 2 && options <= 5);
			break;
		case ProposalTypes::Class::TransferInEpoch:
			valid = options == 2;
			break;
		case ProposalTypes::Class::Variable:
			valid =
				(options >= 2 && options <= 5) // option voting
				|| options == 0;               // scalar voting
			break;
		}
		return valid;
	}

	// Selection of min size type for vote storage
	template <bool scalarVotesSupported>
	struct __VoteStorageTypeSelector { typedef uint8 type;  };
	template <>
	struct __VoteStorageTypeSelector<true> { typedef sint64 type; };


	// Used internally by ProposalVoting to store a proposal with all votes.
	// Supports all vote types.
	template <typename ProposalDataType, uint32 numOfVoters>
	struct ProposalWithAllVoteData : public ProposalDataType
	{
		// Select type for storage (sint64 if scalar votes are supported, uint8 otherwise).
		static constexpr bool supportScalarVotes = ProposalDataType::supportScalarVotes;
		typedef __VoteStorageTypeSelector<supportScalarVotes>::type VoteStorageType;

		// Vote storage
		VoteStorageType votes[numOfVoters];

		// Set proposal and reset all votes
		bool set(const ProposalDataType& proposal)
		{
			if (!supportScalarVotes && proposal.type == ProposalTypes::VariableScalarMean)
				return false;
				
			copyMemory(*(ProposalDataType*)this, proposal);

			if (!supportScalarVotes)
			{
				// option voting only (1 byte per voter)
				ASSERT(proposal.type != ProposalTypes::VariableScalarMean);
				constexpr uint8 noVoteValue = 0xff;
				setMemory(votes, noVoteValue);
			}
			else
			{
				// scalar voting supported (sint64 per voter)
				// (cast should not be needed but is to get rid of warning)
				for (uint32 i = 0; i < numOfVoters; ++i)
					votes[i] = static_cast<VoteStorageType>(NO_VOTE_VALUE);
			}
			return true;
		}

		// Set vote value (as used in ProposalSingleVoteData) of given voter if voter and value are valid
		bool setVoteValue(uint32 voterIndex, sint64 voteValue)
		{
			bool ok = false;
			if (voterIndex < numOfVoters)
			{
				if (voteValue == NO_VOTE_VALUE)
				{
					votes[voterIndex] = (supportScalarVotes) ? NO_VOTE_VALUE : 0xff;
					ok = true;
				}
				else
				{
					if (this->type == ProposalTypes::VariableScalarMean)
					{
						// scalar vote
						if (supportScalarVotes)
						{
							ASSERT(sizeof(votes[0]) == 8);
							if ((voteValue >= this->variableScalar.minValue && voteValue <= this->variableScalar.maxValue))
							{
								// (cast should not be needed but is to get rid of warning)
								votes[voterIndex] = static_cast<VoteStorageType>(voteValue);
								ok = true;
							}
						}
					}
					else
					{
						// option vote
						int numOptions = ProposalTypes::optionCount(this->type);
						if (voteValue >= 0 && voteValue < numOptions)
						{
							votes[voterIndex] = static_cast<VoteStorageType>(voteValue);
							ok = true;
						}
					}
				}
			}
			return ok;
		}

		// Get vote value of given voter as used in ProposalSingleVoteData
		sint64 getVoteValue(uint32 voterIndex) const
		{
			sint64 vv = NO_VOTE_VALUE;
			if (voterIndex < numOfVoters)
			{
				if (supportScalarVotes)
				{
					// stored in sint64 -> set directly
					vv = votes[voterIndex];
				}
				else
				{
					// stored in uint8 -> set if valid vote (not no-vote value 0xff)
					if (votes[voterIndex] != 0xff)
					{
						vv = votes[voterIndex];
					}
				}
			}
			return vv;
		}
	};

	// Used internally by ProposalVoting to store a proposal with all votes
	// Template specialization if only yes/no is supported (saves storage space in votes)
	template <uint32 numOfVoters>
	struct ProposalWithAllVoteData<ProposalDataYesNo, numOfVoters> : public ProposalDataYesNo
	{
		// Vote storage (2 bit per voter)
		uint8 votes[(2 * numOfVoters + 7) / 8];

		// Set proposal and reset all votes
		bool set(const ProposalDataYesNo& proposal)
		{
			if (proposal.type == ProposalTypes::VariableScalarMean)
				return false;

			copyMemory(*(ProposalDataYesNo*)this, proposal);

			// option voting only (2 bit per voter)
			constexpr uint8 noVoteValue = 0xff;
			setMemory(votes, noVoteValue);

			return true;
		}

		// Set vote value (as used in ProposalSingleVoteData) of given voter if voter and value are valid
		bool setVoteValue(uint32 voterIndex, sint64 voteValue)
		{
			bool ok = false;
			if (voterIndex < numOfVoters)
			{
				if (voteValue == NO_VOTE_VALUE)
				{
					uint8 bits = (3 << ((voterIndex & 3) * 2));
					votes[voterIndex >> 2] |= bits;
					ok = true;
				}
				else
				{
					uint16 numOptions = ProposalTypes::optionCount(this->type);
					if (voteValue >= 0 && voteValue < numOptions)
					{
						uint8 bitMask = (3 << ((voterIndex & 3) * 2));
						uint8 bitNum = (uint8(voteValue) << ((voterIndex & 3) * 2));
						votes[voterIndex >> 2] &= ~bitMask;
						votes[voterIndex >> 2] |= bitNum;
						ok = true;
					}
				}
			}
			return ok;
		}

		// Get vote value of given voter as used in ProposalSingleVoteData
		sint64 getVoteValue(uint32 voterIndex) const
		{
			sint64 vv = NO_VOTE_VALUE;
			if (voterIndex < numOfVoters)
			{
				// stored in uint8 -> set if valid vote (not no-vote value 0xff)
				uint8 value = (votes[voterIndex >> 2] >> ((voterIndex & 3) * 2)) & 3;
				if (value != 3)
				{
					vv = value;
				}
			}
			return vv;
		}
	};

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	uint16 QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType>::setProposal(
		const id& proposer,
		const ProposalDataType& proposal
	)
	{
		ProposalVotingType& pv = const_cast<ProposalVotingType&>(this->pv);
		const QpiContextFunctionCall& qpi = this->qpi;

		// epoch 0 means to clear proposal
		if (proposal.epoch == 0)
		{
			unsigned int proposalIndex = pv.proposersAndVoters.getExistingProposalIndex(qpi, proposer);
			return clearProposal(proposalIndex) ? proposalIndex : INVALID_PROPOSAL_INDEX;
		}

		// check if proposal is valid
		if (!proposal.checkValidity())
			return INVALID_PROPOSAL_INDEX;

		// check if proposer ID has right to propose
		if (!pv.proposersAndVoters.isValidProposer(qpi, proposer))
			return INVALID_PROPOSAL_INDEX;

		// get proposal index
		unsigned int proposalIndex = pv.proposersAndVoters.getNewProposalIndex(qpi, proposer);
		if (proposalIndex >= pv.maxProposals)
		{
			// no empty slots -> try to free a slot of oldest proposal before current epoch
			uint16 minEpoch = qpi.epoch();
			for (uint16 i = 0; i < pv.maxProposals; ++i)
			{
				if (pv.proposals[i].epoch < minEpoch)
				{
					proposalIndex = i;
					minEpoch = pv.proposals[i].epoch;
				}
			}

			// all occupied slots are used in current epoch? -> error
			if (proposalIndex >= pv.maxProposals)
				return INVALID_PROPOSAL_INDEX;
			
			// remove oldest proposal
			clearProposal(proposalIndex);

			// call voters interface again in case it needs to register the proposer (should always return the same value)
			proposalIndex = pv.proposersAndVoters.getNewProposalIndex(qpi, proposer);
		}

		// set proposal (and reset previous votes if any)
		ASSERT(proposalIndex < pv.maxProposals);
		if (pv.proposals[proposalIndex].set(proposal))
		{
			pv.proposals[proposalIndex].tick = qpi.tick();
			pv.proposals[proposalIndex].epoch = qpi.epoch();
			return proposalIndex;
		}
		else
		{
			// cleanup in case of this error, which indicates a bug
			pv.proposersAndVoters.freeProposalByIndex(qpi, proposalIndex);
			return INVALID_PROPOSAL_INDEX;
		}
	}

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType>::clearProposal(
		uint16 proposalIndex
	)
	{
		ProposalVotingType& pv = const_cast<ProposalVotingType&>(this->pv);
		const QpiContextFunctionCall& qpi = this->qpi;

		if (proposalIndex >= pv.maxProposals)
			return false;

		pv.proposersAndVoters.freeProposalByIndex(qpi, proposalIndex);
		setMemory(pv.proposals[proposalIndex], 0);
		return true;
	}


	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType>::vote(
		const id& voter,
		const ProposalSingleVoteDataV1& vote
	)
	{
		ProposalVotingType& pv = const_cast<ProposalVotingType&>(this->pv);
		const QpiContextFunctionCall& qpi = this->qpi;

		if (vote.proposalIndex >= pv.maxProposals)
			return false;

		// Check that vote matches proposal
		auto& proposal = pv.proposals[vote.proposalIndex];
		if (vote.proposalType != proposal.type)
			return false;
		if (qpi.epoch() != proposal.epoch)
			return false;
		if (vote.proposalTick != proposal.tick)
			return false;

		// Return voter index (which may be INVALID_VOTER_INDEX if voter has no right to vote)
		unsigned int voterIndex = pv.proposersAndVoters.getVoterIndex(qpi, voter, vote.proposalIndex);

		// Set vote value (checking that voter index and value are valid)
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
		const ProposalDataType& storedProposal = *static_cast<const ProposalDataType*>(&pv.proposals[proposalIndex]);
		copyMemory(proposal, storedProposal);
		return true;
	}

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::getVote(
		uint16 proposalIndex,
		uint32 voterIndex,
		ProposalSingleVoteDataV1& vote
	) const
	{
		if (proposalIndex >= pv.maxProposals || voterIndex >= pv.maxVoters || !pv.proposals[proposalIndex].epoch)
			return false;

		vote.proposalIndex = proposalIndex;
		vote.proposalType = pv.proposals[proposalIndex].type;
		vote.proposalTick = pv.proposals[proposalIndex].tick;
		vote.voteValue = pv.proposals[proposalIndex].getVoteValue(voterIndex);
		return true;
	}


	// Compute voting summary of scalar votes
	template <typename ProposalDataType, uint32 maxVoters>
	bool __getVotingSummaryScalarVotes(
		const ProposalWithAllVoteData<ProposalDataType, maxVoters>& p,
		ProposalSummarizedVotingDataV1& votingSummary
	)
	{
		if (p.type != ProposalTypes::VariableScalarMean)
			return false;

		// scalar voting -> compute mean value of votes
		sint64 value;
		sint64 accumulation = 0;
		if (p.variableScalar.maxValue > p.variableScalar.maxSupportedValue / maxVoters
			|| p.variableScalar.minValue < p.variableScalar.minSupportedValue / maxVoters)
		{
			// calculating mean in a way that avoids overflow of sint64
			// algorithm based on https://stackoverflow.com/questions/56663116/how-to-calculate-average-of-int64-t
			sint64 acc2 = 0;
			for (uint32 i = 0; i < maxVoters; ++i)
			{
				value = p.getVoteValue(i);
				if (value != NO_VOTE_VALUE)
				{
					++votingSummary.totalVotes;
				}
			}
			if (votingSummary.totalVotes)
			{
				for (uint32 i = 0; i < maxVoters; ++i)
				{
					value = p.getVoteValue(i);
					if (value != NO_VOTE_VALUE)
					{
						accumulation += value / votingSummary.totalVotes;
						acc2 += value % votingSummary.totalVotes;
					}
				}
				acc2 /= votingSummary.totalVotes;
				accumulation += acc2;
			}
		}
		else
		{
			// compute mean the regular way (faster than above)
			for (uint32 i = 0; i < maxVoters; ++i)
			{
				value = p.getVoteValue(i);
				if (value != NO_VOTE_VALUE)
				{
					++votingSummary.totalVotes;
					accumulation += value;
				}
			}
			if (votingSummary.totalVotes)
				accumulation /= votingSummary.totalVotes;
		}

		// make sure union is zeroed and set result
		setMemory(votingSummary.optionVoteCount, 0);
		votingSummary.scalarVotingResult = accumulation;

		return true;
	}

	// Specialization of "Compute voting summary of scalar votes" for ProposalDataYesNo, which has no struct members about support scalar votes
	template <uint32 maxVoters>
	bool __getVotingSummaryScalarVotes(
		const ProposalWithAllVoteData<ProposalDataYesNo, maxVoters>& p,
		ProposalSummarizedVotingDataV1& votingSummary
	)
	{
		return false;
	}

	// Get summary of all votes casted
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	bool QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::getVotingSummary(
		uint16 proposalIndex,
		ProposalSummarizedVotingDataV1& votingSummary
	) const
	{
		if (proposalIndex >= pv.maxProposals || !pv.proposals[proposalIndex].epoch)
			return false;

		const ProposalWithAllVoteData<ProposalDataType, pv.maxVoters>& p = pv.proposals[proposalIndex];
		votingSummary.proposalIndex = proposalIndex;
		votingSummary.optionCount = ProposalTypes::optionCount(p.type);
		votingSummary.proposalTick = p.tick;
		votingSummary.authorizedVoters = pv.maxVoters;
		votingSummary.totalVotes = 0;

		if (p.type == ProposalTypes::VariableScalarMean)
		{
			// scalar voting -> compute mean value of votes
			if (!__getVotingSummaryScalarVotes(p, votingSummary))
				return false;
		}
		else
		{
			// option voting -> compute histogram
			ASSERT(votingSummary.optionCount > 0);
			ASSERT(votingSummary.optionCount <= votingSummary.optionVoteCount.capacity());
			auto& hist = votingSummary.optionVoteCount;
			hist.setAll(0);
			for (uint32 i = 0; i < pv.maxVoters; ++i)
			{
				sint64 value = p.getVoteValue(i);
				if (value != NO_VOTE_VALUE && value >= 0 && value < votingSummary.optionCount)
				{
					++votingSummary.totalVotes;
					hist.set(value, hist.get(value) + 1);
				}
			}
		}

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

	// Return voter index for given ID or INVALID_VOTER_INDEX if ID has no right to vote. If the voter has multiple
	// votes, this returns the first index. All votes of a voter are stored consecutively.
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	uint32 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::voterIndex(
		const id& voterId,
		uint16 proposalIndex
	) const
	{
		return pv.proposersAndVoters.getVoterIndex(qpi, voterId, proposalIndex);
	}

	// Return ID for given voter index or NULL_ID if index is invalid
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	id QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::voterId(
		uint32 voterIndex,
		uint16 proposalIndex
	) const
	{
		return pv.proposersAndVoters.getVoterId(qpi, voterIndex, proposalIndex);
	}

	// Return count of votes of a voter if the first voter index is passed. Otherwise return the number of votes
	// including this and the following indices. Returns 0 if an invalid index is passed.
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	uint32 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::voteCount(
		uint32 voterIndex,
		uint16 proposalIndex
	) const
	{
		return pv.proposersAndVoters.voteCount(qpi, voterIndex, proposalIndex);
	}

	// Return next proposal index of proposals of given epoch (default: current epoch)
	// or -1 if there are not any more such proposals behind the passed index.
	// Pass -1 to get first index.
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	sint32 QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>::nextProposalIndex(
		sint32 prevProposalIndex,
		uint16 epoch
	) const
	{
		if (prevProposalIndex >= pv.maxProposals)
			return -1;
		if (epoch == 0)
			epoch = qpi.epoch();

		uint16 idx = (prevProposalIndex < 0) ? 0 : prevProposalIndex + 1;
		while (idx < pv.maxProposals)
		{
			if (pv.proposals[idx].epoch == epoch)
				return idx;
			++idx;
		}

		return -1;
	}

	// Return next proposal index of finished proposal (not created in current epoch, voting not possible anymore)
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
