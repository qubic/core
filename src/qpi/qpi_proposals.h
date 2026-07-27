#pragma once

#include "qpi_types.h"

namespace QPI
{
	constexpr uint16 INVALID_PROPOSAL_INDEX = 0xffff;
	constexpr uint32 INVALID_VOTE_INDEX = 0xffffffff;
	constexpr sint64 NO_VOTE_VALUE = 0x8000000000000000;

	// Single vote for all types of proposals defined in August 2024.
	// Input data for contract procedure call
	struct ProposalSingleVoteDataV1
	{
		// Index of proposal the vote is about (can be requested with proposal voting API)
		uint16 proposalIndex;

		// Type of proposal, see ProposalTypes
		uint16 proposalType;

		// Tick when proposal has been set (to make sure that proposal version known by the voter matches the current version).
		uint32 proposalTick;

		// Value of vote. NO_VOTE_VALUE means no vote for every type.
		// For proposals types with multiple options, 0 is no, 1 to N are the other options in order of definition in proposal.
		// For scalar proposal types the value is passed directly.
		sint64 voteValue;
	};
	static_assert(sizeof(ProposalSingleVoteDataV1) == 16, "Unexpected struct size.");

	// For casting multiple votes for all types of proposals defined in August 2024.
	// This makes sense for shareholder voting, where a single shareholder may own multiple shares, allowing to cast
	// multiple votes. With this structs, the votes may be individually distributed to multiple options/values.
	// Input data for contract procedure call, compatible with ProposalSingleVoteDataV1. That is, to cast all votes
	// of a shareholder with the same value, just set element 0 of voteValues and leave/set the rest to zero (including
	// the voteCounts).
	struct ProposalMultiVoteDataV1
	{
		// Index of proposal the vote is about (can be requested with proposal voting API)
		uint16 proposalIndex;

		// Type of proposal, see ProposalTypes
		uint16 proposalType;

		// Tick when proposal has been set (to make sure that proposal version known by the voter matches the current version).
		uint32 proposalTick;

		// Value of vote. NO_VOTE_VALUE means no vote for every type.
		// For proposals types with multiple options, 0 is no, 1 to N are the other options in order of definition in proposal.
		// For scalar proposal types the value is passed directly.
		Array<sint64, 8> voteValues;

		// Count of votes to cast for the corresponding voteValues.
		// For compatibility with ProposalSingleVoteDataV1, voteCounts.get(0) == 0 means all votes of the voter. In
		// the other elements, 0 means no votes for the given value.
		Array<uint32, 8> voteCounts;
	};
	static_assert(sizeof(ProposalMultiVoteDataV1) == 104, "Unexpected struct size.");

	// Voting result summary for all types of proposals defined in August 2024.
	// Output data for contract function call for getting voting results.
	struct ProposalSummarizedVotingDataV1
	{
		// Index of proposal the vote is about (can be requested with proposal voting API)
		uint16 proposalIndex;

		// Count of options in proposal type (number of valid elements in optionVoteCount, 0 for scalar voting)
		uint16 optionCount;

		// Tick when proposal has been set (useful for checking if cached ProposalData is still up to date).
		uint32 proposalTick;

		// Maximal number of votes (number of voters who have the right to vote if there aren't multiple votes per voter)
		uint32 totalVotesAuthorized;

		// Number of total votes casted
		uint32 totalVotesCasted;

		// Voting results
		union
		{
			// Number of votes for different options (0 = no change, 1 to N = yes to specific proposed value)
			Array<uint32, 8> optionVoteCount;

			// Scalar voting result (currently only for proposalType VariableScalarMean, mean value of all valid votes)
			sint64 scalarVotingResult;
		};

		// Return index of most voted option or -1 if this is scalar voting
		sint32 getMostVotedOption() const
		{
			if (optionCount == 0)
				return -1;
			sint32 mostVotedOptionIndex = 0;
			uint32 mostVotedOptionVotes = optionVoteCount.get(0);
			for (sint32 optionIndex = 1; optionIndex < optionCount; ++optionIndex)
			{
				uint32 optionVotes = optionVoteCount.get(optionIndex);
				if (mostVotedOptionVotes < optionVotes)
				{
					mostVotedOptionVotes = optionVotes;
					mostVotedOptionIndex = optionIndex;
				}
			}
			return mostVotedOptionIndex;
		}

		// Return index of option accepted by quorum or -1 if none is accepted
		sint32 getAcceptedOption(uint32 totalVotesThresh = QUORUM, uint32 mostVotedThreshold = QUORUM / 2) const
		{
			if (totalVotesCasted >= totalVotesThresh)
			{
				sint32 opt = getMostVotedOption();
				if (opt >= 0 && optionVoteCount.get(opt) > mostVotedThreshold)
					return opt;
			}
			return -1;
		}

		ProposalSummarizedVotingDataV1() = default;
		ProposalSummarizedVotingDataV1(const ProposalSummarizedVotingDataV1& src)
		{
			copyMemory(*this, src);
		}
		ProposalSummarizedVotingDataV1& operator=(const ProposalSummarizedVotingDataV1& src)
		{
			copyMemory(*this, src);
			return *this;
		}
	};
	static_assert(sizeof(ProposalSummarizedVotingDataV1) == 16 + 8 * 4, "Unexpected struct size.");

	// Proposal type constants and functions.
	// Each proposal type is composed of a class and a number of options. As an alternative to having N options (option votes),
	// some proposal classes (currently the one to set a variable) may allow to vote with a scalar value in a range defined
	// by the proposal (scalar voting).
	namespace ProposalTypes
	{
		// Class of proposal type
		namespace Class
		{
			// Options without extra data. Supported options: 2 <= N <= 8 with ProposalDataV1.
			static constexpr uint16 GeneralOptions = 0;

			// Propose to transfer amount to address. Supported options: 2 <= N <= 5 with ProposalDataV1.
			static constexpr uint16 Transfer = 0x100;

			// Propose to set variable to a value. Supported options: 2 <= N <= 5 with ProposalDataV1; N == 0 means scalar voting.
			static constexpr uint16 Variable = 0x200;

			// Propose to set multiple variables. Supported options: 2 <= N <= 8 with ProposalDataV1
			static constexpr uint16 MultiVariables = 0x300;

			// Propose to transfer amount to address in a specific epoch. Supported options: 1 with ProposalDataV1.
			static constexpr uint16 TransferInEpoch = 0x400;
		};

		// Invalid proposal type returned to encode error in some interfaces
		static constexpr uint16 Invalid = 0;

		// Options yes and no without extra data -> result is histogram of options
		static constexpr uint16 YesNo = Class::GeneralOptions | 2;

		// 3 options without extra data -> result is histogram of options
		static constexpr uint16 ThreeOptions = Class::GeneralOptions | 3;

		// 3 options without extra data -> result is histogram of options
		static constexpr uint16 FourOptions = Class::GeneralOptions | 4;

		// Transfer given amount to address with options yes/no
		static constexpr uint16 TransferYesNo = Class::Transfer | 2;

		// Transfer amount to address with two options of amounts and option "no change"
		static constexpr uint16 TransferTwoAmounts = Class::Transfer | 3;

		// Transfer amount to address with three options of amounts and option "no change"
		static constexpr uint16 TransferThreeAmounts = Class::Transfer | 4;

		// Transfer amount to address with four options of amounts and option "no change"
		static constexpr uint16 TransferFourAmounts = Class::Transfer | 5;

		// Transfer given amount to address in a specific epoch, with options yes/no
		static constexpr uint16 TransferInEpochYesNo = Class::TransferInEpoch | 2;

		// Set given variable to proposed value with options yes/no
		static constexpr uint16 VariableYesNo = Class::Variable | 2;

		// Set given variable to proposed value with two options of values and option "no change"
		static constexpr uint16 VariableTwoValues = Class::Variable | 3;

		// Set given variable to proposed value with three options of values and option "no change"
		static constexpr uint16 VariableThreeValues = Class::Variable | 4;

		// Set given variable to proposed value with four options of values and option "no change"
		static constexpr uint16 VariableFourValues = Class::Variable | 5;

		// Set given variable to value, allowing to vote with scalar value, voting result is mean value
		static constexpr uint16 VariableScalarMean = Class::Variable | 0;

		// TODO: support quorum value max / min as voting result

		// Set multiple variables with options yes/no (data stored by contract) -> result is histogram of options
		static constexpr uint16 MultiVariablesYesNo = Class::MultiVariables | 2;

		// Set multiple variables with 3 options "no change" / "values A" / "values B" (data stored by contract)
		// -> result is histogram of options
		static constexpr uint16 MultiVariablesThreeOptions = Class::MultiVariables | 3;

		// Set multiple variables with 4 options "no change" / "values A" / "values B" / "values C" (data stored by
		// contract) -> result is histogram of options
		static constexpr uint16 MultiVariablesFourOptions = Class::MultiVariables | 4;

		// Construct type from class + number of options (no checking if type is valid)
		static constexpr uint16 type(uint16 cls, uint16 options)
		{
			return cls | options;
		}

		// Return option count for a given proposal type (including "no change" option),
		// 0 for scalar voting (no checking if type is valid).
		static uint16 optionCount(uint16 proposalType)
		{
			return proposalType & 0x00ff;
		}

		// Return class of proposal type (no checking if type is valid).
		static uint16 cls(uint16 proposalType)
		{
			return proposalType & 0xff00;
		}

		// Check if given type is valid (supported by most comprehensive ProposalData class).
		inline static bool isValid(uint16 proposalType);
	};

	// Proposal data struct for all types of proposals defined in August 2024 and revised in June 2025.
	// Input data for contract procedure call, usable as ProposalDataType in ProposalVoting (persisted in contract states).
	// You have to choose, whether to support scalar votes next to option votes. Scalar votes require 8x more storage in the state.
	template <bool SupportScalarVotes>
	struct ProposalDataV1
	{
		// URL explaining proposal, zero-terminated string.
		Array<uint8, 256> url;

		// Epoch, when proposal is active. For setProposal(), 0 means to clear proposal and non-zero means the current epoch.
		uint16 epoch;

		// Type of proposal, see ProposalTypes.
		uint16 type;

		// Tick when proposal has been set. Output only, overwritten in setProposal().
		uint32 tick;

		// Proposal payload data (for all except types with class GeneralProposal)
		union Data
		{
			// Used if type class is Transfer
			struct Transfer
			{
				id destination;
				Array<sint64, 4> amounts;   // N first amounts are the proposed options (non-negative, sorted without duplicates), rest zero
			} transfer;

			// Used if type class is TransferInEpoch
			struct TransferInEpoch
			{
				id destination;
				sint64 amount;              // non-negative
				uint16 targetEpoch;         // not checked by isValid()!
			} transferInEpoch;

			// Used if type class is Variable and type is not VariableScalarMean
			struct VariableOptions
			{
				uint64 variable;            // For identifying variable (interpreted by contract only)
				Array<sint64, 4> values;    // N first amounts are proposed options sorted without duplicates, rest zero
			} variableOptions;

			// Used if type is VariableScalarMean
			struct VariableScalar
			{
				uint64 variable;            // For identifying variable (interpreted by contract only)
				sint64 minValue;            // Minimum value allowed in proposedValue and votes, must be > NO_VOTE_VALUE
				sint64 maxValue;            // Maximum value allowed in proposedValue and votes, must be >= minValue
				sint64 proposedValue;       // Needs to be in range between minValue and maxValue

				static constexpr sint64 minSupportedValue = 0x8000000000000001;
				static constexpr sint64 maxSupportedValue = 0x7fffffffffffffff;
			} variableScalar;
		} data;

		// Check if content of instance are valid. Epoch is not checked.
		// Also useful to show requirements of valid proposal.
		bool checkValidity() const
		{
			bool okay = false;
			// TODO: validate URL
			uint16 cls = ProposalTypes::cls(type);
			uint16 options = ProposalTypes::optionCount(type);
			switch (cls)
			{
			case ProposalTypes::Class::GeneralOptions:
			case ProposalTypes::Class::MultiVariables:
				okay = options >= 2 && options <= 8;
				break;
			case ProposalTypes::Class::Transfer:
				if (!isZero(data.transfer.destination) && options >= 2 && options <= 5)
				{
					uint16 proposedAmounts = options - 1;
					okay = true;
					for (uint16 i = 0; i < proposedAmounts; ++i)
					{
						// no negative amounts
						if (data.transfer.amounts.get(i) < 0)
						{
							okay = false;
							break;
						}
					}
					okay = okay
						&& isArraySortedWithoutDuplicates(data.transfer.amounts, 0, proposedAmounts)
						&& data.transfer.amounts.rangeEquals(proposedAmounts, data.transfer.amounts.capacity(), 0);
				}
				break;
			case ProposalTypes::Class::TransferInEpoch:
				okay = options == 2 && !isZero(data.transferInEpoch.destination) && data.transferInEpoch.amount >= 0;
				break;
			case ProposalTypes::Class::Variable:
				if (options >= 2 && options <= 5)
				{
					// option voting
					uint16 proposedValues = options - 1;
					okay = isArraySortedWithoutDuplicates(data.variableOptions.values, 0, proposedValues)
						&& data.variableOptions.values.rangeEquals(proposedValues, data.variableOptions.values.capacity(), 0);
				}
				else if (options == 0)
				{
					// scalar voting
					if (supportScalarVotes)
						okay = data.variableScalar.minValue <= data.variableScalar.proposedValue
						&& data.variableScalar.proposedValue <= data.variableScalar.maxValue
						&& data.variableScalar.minValue > NO_VOTE_VALUE;
				}
				break;
			}
			return okay;
		}

		// Whether to support scalar votes next to option votes.
		static constexpr bool supportScalarVotes = SupportScalarVotes;

		ProposalDataV1() = default;
		ProposalDataV1(const ProposalDataV1<SupportScalarVotes>& src)
		{
			copyMemory(*this, src);
		}
		ProposalDataV1<SupportScalarVotes>& operator=(const ProposalDataV1<SupportScalarVotes>& src)
		{
			copyMemory(*this, src);
			return *this;
		}
	};
	static_assert(sizeof(ProposalDataV1<true>) == 256 + 8 + 64, "Unexpected struct size.");

	// Proposal data struct for 2-option proposals (requires less storage space).
	// Input data for contract procedure call, usable as ProposalDataType in ProposalVoting
	struct ProposalDataYesNo
	{
		// URL explaining proposal, zero-terminated string.
		Array<uint8, 256> url;

		// Epoch, when proposal is active. For setProposal(), 0 means to clear proposal and non-zero means the current epoch.
		uint16 epoch;

		// Type of proposal, see ProposalTypes.
		uint16 type;

		// Tick when proposal has been set. Output only, overwritten in setProposal().
		uint32 tick;

		// Proposal payload data (for all except types with class GeneralProposal)
		union Data
		{
			// Used if type class is Transfer
			struct Transfer
			{
				id destination;
				sint64 amount;		// Amount of proposed option (non-negative)
			} transfer;

			// Used if type class is Variable and type is not VariableScalarMean
			struct VariableOptions
			{
				uint64 variable;    // For identifying variable (interpreted by contract only)
				sint64 value;		// Value of proposed option, rest zero
			} variableOptions;
		} data;

		// Check if content of instance are valid. Epoch is not checked.
		// Also useful to show requirements of valid proposal.
		bool checkValidity() const
		{
			bool okay = false;
			// TODO: validate URL
			uint16 cls = ProposalTypes::cls(type);
			uint16 options = ProposalTypes::optionCount(type);
			switch (cls)
			{
			case ProposalTypes::Class::GeneralOptions:
			case ProposalTypes::Class::MultiVariables:
				okay = options >= 2 && options <= 3; // 3 options can be encoded in the yes/no type of storage as well
				break;
			case ProposalTypes::Class::Transfer:
				okay = (options == 2 && !isZero(data.transfer.destination) && data.transfer.amount >= 0);
				break;
			case ProposalTypes::Class::Variable:
				okay = (options == 2);
				break;
			}
			return okay;
		}

		// Whether to support scalar votes next to option votes.
		static constexpr bool supportScalarVotes = false;

		ProposalDataYesNo() = default;
		ProposalDataYesNo(const ProposalDataYesNo& src)
		{
			copyMemory(*this, src);
		}
		ProposalDataYesNo& operator=(const ProposalDataYesNo& src)
		{
			copyMemory(*this, src);
			return *this;
		}
	};
	static_assert(sizeof(ProposalDataYesNo) == 256 + 8 + 40, "Unexpected struct size.");


	// Used internally by ProposalVoting to store a proposal with all votes
	template <typename ProposalDataType, uint32 numOfVoters>
	struct ProposalWithAllVoteData;


	// Option for ProposerAndVoterHandlingT in ProposalVoting that allows both voting and setting proposals for computors only.
	template <uint16 proposalSlotCount = NUMBER_OF_COMPUTORS>
	struct ProposalAndVotingByComputors;

	// Option for ProposerAndVoterHandlingT in ProposalVoting that allows both voting for computors only and creating/changing proposals for anyone.
	template <uint16 proposalSlotCount>
	struct ProposalByAnyoneVotingByComputors;

	// Option for ProposerAndVoterHandlingT in ProposalVoting that allows both voting and setting proposals for contract shareholders only.
	template <uint16 proposalSlotCount, uint64 contractAssetName>
	struct ProposalAndVotingByShareholders;

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalFunctionCall;

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalProcedureCall;

	/*
	* Proposal voting state for use in contract state.
	* Voting is running until end of epoch, each proposer/computor can have one proposal at a time (or zero).
	* ProposerAndVoterHandlingType:
	*	Class for checking right to propose/vote and getting index in array. May have member data such
	*   as an array of IDs and may be initialized by accessing the public member proposerAndVoter.
	* ProposalDataT:
	*   Class defining supported proposals. Also determines storage for proposals and votes.
	*/
	template <typename ProposerAndVoterHandlingT, typename ProposalDataT>
	class ProposalVoting
	{
	public:
		static constexpr uint16 maxProposals = ProposerAndVoterHandlingT::maxProposals;
		static constexpr uint32 maxVotes = ProposerAndVoterHandlingT::maxVotes;

		typedef ProposerAndVoterHandlingT ProposerAndVoterHandlingType;
		typedef ProposalDataT ProposalDataType;
		typedef ProposalWithAllVoteData<
			ProposalDataT,
			maxVotes
		> ProposalAndVotesDataType;

		static_assert(maxProposals <= INVALID_PROPOSAL_INDEX);
		static_assert(maxVotes <= INVALID_VOTE_INDEX);

		// Handling of who has the right to propose and to vote + proposal / voter indices
		ProposerAndVoterHandlingType proposersAndVoters;

	protected:
		// Proposals and corresponding votes. No direct access for contracts.
		ProposalAndVotesDataType proposals[maxProposals];

		// Give user interface access to proposals
		friend struct QpiContextProposalProcedureCall<ProposerAndVoterHandlingT, ProposalDataT>;
		friend struct QpiContextProposalFunctionCall<ProposerAndVoterHandlingT, ProposalDataT>;
	};

	// Proposal user interface available in contract functions
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalFunctionCall
	{
		// Get proposal with given index if index is valid and proposal is set (epoch > 0). On error returns false and sets proposal.type = 0.
		bool getProposal(uint16 proposalIndex, ProposalDataType& proposal) const;

		// Get data of single vote. On error returns false and sets vote.proposalType = 0.
		bool getVote(uint16 proposalIndex, uint32 voteIndex, ProposalSingleVoteDataV1& vote) const;

		// Get data of votes of a given voter. On error returns false and sets votes.proposalType = 0.
		bool getVotes(uint16 proposalIndex, const id& voter, ProposalMultiVoteDataV1& votes) const;

		// Get summary of all votes casted. On error returns false and sets votingSummary.totalVotesAuthorized = 0.
		bool getVotingSummary(uint16 proposalIndex, ProposalSummarizedVotingDataV1& votingSummary) const;

		// Return index of existing proposal or INVALID_PROPOSAL_INDEX if there is no proposal by given proposer
		uint16 proposalIndex(const id& proposerId) const;

		// Return proposer ID of given proposal index or NULL_ID if there is no proposal at this index
		id proposerId(uint16 proposalIndex) const;

		// Return vote index for given ID or INVALID_VOTE_INDEX if ID has no right to vote. If the voter has multiple
		// votes, this returns the first index. All votes of a voter are stored consecutively.
		// If voters are shareholders, proposalIndex must be passed. If voters are computors, proposalIndex is ignored.
		uint32 voteIndex(const id& voterId, uint16 proposalIndex = 0) const;

		// Return ID for given vote index or NULL_ID if index is invalid.
		// If voters are shareholders, proposalIndex must be passed. If voters are computors, proposalIndex is ignored.
		id voterId(uint32 voteIndex, uint16 proposalIndex = 0) const;

		// Return count of votes of a voter if his first vote index is passed. Otherwise return the number of votes
		// including this and the following indices. Returns 0 if an invalid index is passed.
		// If voters are shareholders, proposalIndex must be passed. If voters are computors, proposalIndex is ignored.
		uint32 voteCount(uint32 voteIndex, uint16 proposalIndex = 0) const;

		// Return next proposal index of proposals of given epoch (default: current epoch)
		// or -1 if there are not any more such proposals behind the passed index.
		// Pass -1 to get first index.
		sint32 nextProposalIndex(sint32 prevProposalIndex, uint16 epoch = 0) const;

		// Return next proposal index of finished proposal (not created in current epoch, voting not possible anymore)
		// or -1 if there are not any more such proposals behind the passed index.
		// Pass -1 to get first index.
		sint32 nextFinishedProposalIndex(sint32 prevProposalIndex) const;

		// ProposalVoting type to work with
		typedef ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType> ProposalVotingType;

		// Constructor. Use qpi(proposalVotingObject) to construct instance.
		QpiContextProposalFunctionCall(
			const QpiContextFunctionCall& qpi,
			const ProposalVotingType& pv
		) : qpi(qpi), pv(pv) {
		}

		const QpiContextFunctionCall& qpi;
		const ProposalVotingType& pv;
	};

	// Proposal user interface available in contract procedures
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalProcedureCall : public QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>
	{
		// Set proposal if proposer has right to do so, proposal is valid, proposal.epoch is current epoch,
		// and a proposal slot is available.
		// If the proposer already has a proposal slot, his previous proposal is overwritten (and all votes
		// are discarded).
		// If there is no free slot, one of the oldest proposals from prior epochs is deleted to free a slot.
		// This may be also used to clear a proposal by setting proposal.epoch = 0.
		// Return proposalIndex if proposal has been set, or INVALID_PROPOSAL_INDEX on error.
		uint16 setProposal(
			const id& proposer,
			const ProposalDataType& proposal
		);

		// Clear proposal of given index (without checking rights). Returns false if proposalIndex is invalid.
		bool clearProposal(
			uint16 proposalIndex
		);

		// Cast vote for proposal with index vote.proposalIndex if voter has right to vote, the proposal's epoch
		// is the current epoch, vote.proposalType and vote.proposalTick match the corresponding proposal's values,
		// and vote.voteValue is valid for the proposal type.
		// If voter has multiple votes (possible in shareholder voting), cast all votes of voter with the same value.
		// This can be used to remove a previous vote by vote.voteValue = NO_VOTE_VALUE.
		// Return whether vote has been casted.
		bool vote(
			const id& voter,
			const ProposalSingleVoteDataV1& vote
		);

		// Cast votes for proposal with index votes.proposalIndex if voter has right to vote, the proposal's epoch
		// is the current epoch, votes.proposalType and votes.proposalTick match the corresponding proposal's values,
		// the votes.voteValues are valid for the proposal type, and the sum of votes.voteCounts does not exceed the
		// number of votes available to the voter.
		// If any vote value is invalid, all votes of the voter are set to NO_VOTE_VALUE.
		// This can be used to remove previous votes by using a vote value of NO_VOTE_VALUE or a total vote count less
		// than the number of votes available to the voter.
		// For compatibility with ProposalSingleVoteDataV1, all votes are set with votes.voteValues.get(0) if the sum
		// of votes.voteCounts is 0.
		// Return whether the votes have been casted.
		bool vote(
			const id& voter,
			const ProposalMultiVoteDataV1& votes
		);

		// ProposalVoting type to work with
		typedef ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType> ProposalVotingType;

		// Base class
		typedef QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType> BaseClass;

		// Constructor. Use qpi(proposalVotingObject) to construct instance.
		QpiContextProposalProcedureCall(
			const QpiContextFunctionCall& qpi,
			ProposalVotingType& pv
		) : BaseClass(qpi, pv) {
		}
	};
}
