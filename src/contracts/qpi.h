// Qubic Programming Interface 1.0.0

#pragma once

// m256i is used for the id data type
#include "../platform/m256.h"


namespace QPI
{
	/*

	Prohibited character combinations in contracts:

	"
	#
	%
	'
	* not as multiplication operator
	..
	/ as division operator
	::
	[
	]
	__
	double
	float
	typedef
	union

	const_cast
	QpiContext
	TODO: prevent other casts trying to cast const away from state

	*/

	typedef bool bit;
	typedef signed char sint8;
	typedef unsigned char uint8;
	typedef signed short sint16;
	typedef unsigned short uint16;
	typedef signed int sint32;
	typedef unsigned int uint32;
	typedef signed long long sint64;
	typedef unsigned long long uint64;

	typedef m256i id;

#define NULL_ID id(0, 0, 0, 0)
	constexpr sint64 NULL_INDEX = -1;

#define _A 0
#define _B 1
#define _C 2
#define _D 3
#define _E 4
#define _F 5
#define _G 6
#define _H 7
#define _I 8
#define _J 9
#define _K 10
#define _L 11
#define _M 12
#define _N 13
#define _O 14
#define _P 15
#define _Q 16
#define _R 17
#define _S 18
#define _T 19
#define _U 20
#define _V 21
#define _W 22
#define _X 23
#define _Y 24
#define _Z 25
#define ID(_00, _01, _02, _03, _04, _05, _06, _07, _08, _09, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55) _mm256_set_epi64x(((((((((((((((uint64)_55) * 26 + _54) * 26 + _53) * 26 + _52) * 26 + _51) * 26 + _50) * 26 + _49) * 26 + _48) * 26 + _47) * 26 + _46) * 26 + _45) * 26 + _44) * 26 + _43) * 26 + _42, ((((((((((((((uint64)_41) * 26 + _40) * 26 + _39) * 26 + _38) * 26 + _37) * 26 + _36) * 26 + _35) * 26 + _34) * 26 + _33) * 26 + _32) * 26 + _31) * 26 + _30) * 26 + _29) * 26 + _28, ((((((((((((((uint64)_27) * 26 + _26) * 26 + _25) * 26 + _24) * 26 + _23) * 26 + _22) * 26 + _21) * 26 + _20) * 26 + _19) * 26 + _18) * 26 + _17) * 26 + _16) * 26 + _15) * 26 + _14, ((((((((((((((uint64)_13) * 26 + _12) * 26 + _11) * 26 + _10) * 26 + _09) * 26 + _08) * 26 + _07) * 26 + _06) * 26 + _05) * 26 + _04) * 26 + _03) * 26 + _02) * 26 + _01) * 26 + _00)

#define NUMBER_OF_COMPUTORS 676

#define JANUARY 1
#define FEBRUARY 2
#define MARCH 3
#define APRIL 4
#define MAY 5
#define JUNE 6
#define JULY 7
#define AUGUST 8
#define SEPTEMBER 9
#define OCTOBER 10
#define NOVEMBER 11
#define DECEMBER 12

#define WEDNESDAY 0
#define THURSDAY 1
#define FRIDAY 2
#define SATURDAY 3
#define SUNDAY 4
#define MONDAY 5
#define TUESDAY 6

	constexpr unsigned long long X_MULTIPLIER = 1ULL;

	// Copy memory of src to dst. Both may have different types, but size of both must match exactly.
	template <typename T1, typename T2>
	void copyMemory(T1& dst, const T2& src);

	// Set all memory of dst to byte value.
	template <typename T>
	void setMemory(T& dst, uint8 value);


	// Array of L bits encoded in array of uint64 (overall size is at least 8 bytes, L must be 2^N)
	template <uint64 L>
	struct bit_array
	{
	private:
		static_assert(L && !(L & (L - 1)),
			"The capacity of the bit_array must be 2^N."
			);

		static constexpr uint64 _bits = L;
		static constexpr uint64 _elements = ((L + 63) / 64);

		uint64 _values[_elements];

	public:
		// Return number of bits
		static inline constexpr uint64 capacity()
		{
			return L;
		}

		// Return bit value at given index
		inline bit get(uint64 index) const
		{
			return (_values[(index >> 6) & (_elements - 1)] >> (index & 63)) & 1;
		}

		// Set bit with given index to the given value
		inline void set(uint64 index, bit value)
		{
			_values[(index >> 6) & (_elements - 1)] = (_values[(index >> 6) & (_elements - 1)] & (~(1ULL << (index & 63)))) | (((uint64)value) << (index & 63));
		}

		// Set content of bit array by copying memory (size must match)
		template <typename AT>
		inline void setMem(const AT& value)
		{
			static_assert(sizeof(_values) == sizeof(value), "This function can only be used if the overall size of both objects match.");
			// This if is resolved at compile time
			if (sizeof(_values) == 32)
			{
				// assignment uses __m256i intrinsic CPU functions which should be very fast
				*((id*)_values) = *((id*)&value);
			}
			else
			{
				// generic copying
				copyMemory(*this, value);
			}
		}

		// Set all bits to passed bit value
		inline void setAll(bit value)
		{
			uint64 setValue = (value) ? 0xffffffffffffffffllu : 0llu;
			for (uint64 i = 0; i < _elements; ++i)
				_values[i] = setValue;
		}
	};

	// Bit array convenience definitions
	typedef bit_array<2> bit_2;
	typedef bit_array<4> bit_4;
	typedef bit_array<8> bit_8;
	typedef bit_array<16> bit_16;
	typedef bit_array<32> bit_32;
	typedef bit_array<64> bit_64;
	typedef bit_array<128> bit_128;
	typedef bit_array<256> bit_256;
	typedef bit_array<512> bit_512;
	typedef bit_array<1024> bit_1024;
	typedef bit_array<2048> bit_2048;
	typedef bit_array<4096> bit_4096;


	// Array of L elements of type T (L must be 2^N)
	template <typename T, uint64 L>
	struct array
	{
	private:
		static_assert(L && !(L & (L - 1)),
			"The capacity of the array must be 2^N."
			);

		T _values[L];

	public:
		// Return number of elements in array
		static inline constexpr uint64 capacity()
		{
			return L;
		}

		// Get element of array
		inline const T& get(uint64 index) const
		{
			return _values[index & (L - 1)];
		}

		// Set element of array
		inline void set(uint64 index, const T& value)
		{
			_values[index & (L - 1)] = value;
		}

		// Set content of array by copying memory (size must match)
		template <typename AT>
		inline void setMem(const AT& value)
		{
			static_assert(sizeof(_values) == sizeof(value), "This function can only be used if the overall size of both objects match.");
			// This if is resolved at compile time
			if (sizeof(_values) == 32)
			{
				// assignment uses __m256i intrinsic CPU functions which should be very fast
				*((id*)_values) = *((id*)&value);
			}
			else
			{
				// generic copying
				copyMemory(*this, value);
			}
		}

		// Set all elements to passed value
		inline void setAll(const T& value)
		{
			for (uint64 i = 0; i < L; ++i)
				_values[i] = value;
		}
	};
	
	// Array convenience definitions
	typedef array<sint8, 2> sint8_2;
	typedef array<sint8, 4> sint8_4;
	typedef array<sint8, 8> sint8_8;

	typedef array<uint8, 2> uint8_2;
	typedef array<uint8, 4> uint8_4;
	typedef array<uint8, 8> uint8_8;

	typedef array<sint16, 2> sint16_2;
	typedef array<sint16, 4> sint16_4;
	typedef array<sint16, 8> sint16_8;

	typedef array<uint16, 2> uint16_2;
	typedef array<uint16, 4> uint16_4;
	typedef array<uint16, 8> uint16_8;

	typedef array<sint32, 2> sint32_2;
	typedef array<sint32, 4> sint32_4;
	typedef array<sint32, 8> sint32_8;

	typedef array<uint32, 2> uint32_2;
	typedef array<uint32, 4> uint32_4;
	typedef array<uint32, 8> uint32_8;

	typedef array<sint64, 2> sint64_2;
	typedef array<sint64, 4> sint64_4;
	typedef array<sint64, 8> sint64_8;

	typedef array<uint64, 2> uint64_2;
	typedef array<uint64, 4> uint64_4;
	typedef array<uint64, 8> uint64_8;

	typedef array<id, 2> id_2;
	typedef array<id, 8> id_4;
	typedef array<id, 8> id_8;


	// Collection of priority queues of elements with type T and total element capacity L.
	// Each ID pov (point of view) has an own queue.
	template <typename T, uint64 L>
	struct collection
	{
	private:
		static_assert(L && !(L & (L - 1)),
			"The capacity of the collection must be 2^N."
			);
		static constexpr sint64 _nEncodedFlags = L > 32 ? 32 : L;

		// Hash map of point of views = element filters, each with one priority queue (or empty)
		struct PoV
		{
			id value;
			uint64 population;
			sint64 headIndex, tailIndex;
			sint64 bstRootIndex;
		} _povs[L];

		// 2 bits per element of _povs: 0b00 = not occupied; 0b01 = occupied; 0b10 = occupied but marked for removal; 0b11 is unused
		// The state "occupied but marked for removal" is needed for finding the index of a pov in the hash map. Setting an entry to
		// "not occupied" in remove() would potentially undo a collision, create a gap, and mess up the entry search.
		uint64 _povOccupationFlags[(L * 2 + 63) / 64];

		// Array of elements (filled sequentially), each belongs to one PoV / priority queue (or is empty)
		// Elements of a POV entry will be stored as a binary search tree (BST); so this structure has some properties related to BST
		// (bstParentIndex, bstLeftIndex, bstRightIndex).
		struct Element
		{
			T value;
			sint64 priority;
			sint64 povIndex;
			sint64 bstParentIndex;
			sint64 bstLeftIndex;
			sint64 bstRightIndex;

			Element& init(const T& value, const sint64& priority, const sint64& povIndex)
			{
				this->value = value;
				this->priority = priority;
				this->povIndex = povIndex;
				this->bstParentIndex = NULL_INDEX;
				this->bstLeftIndex = NULL_INDEX;
				this->bstRightIndex = NULL_INDEX;
				return *this;
			}
		} _elements[L];
		uint64 _population;
		uint64 _markRemovalCounter;

		// Internal reinitialize as empty collection.
		void _softReset();

		// Return index of id pov in hash map _povs, or NULL_INDEX if not found
		sint64 _povIndex(const id& pov) const;

		// Return elementIndex of first element in priority queue of pov,
		// and ignore elements with priority greater than maxPriority
		sint64 _headIndex(const sint64 povIndex, const sint64 maxPriority) const;

		// Return elementIndex of last element in priority queue of pov,
		// and ignore elements with priority less than minPriority
		sint64 _tailIndex(const sint64 povIndex, const sint64 minPriority) const;

		// Return index of parent element to insert a priority
		sint64 _searchElement(const sint64 bstRootIndex,
			const sint64 priority, int* pIterationsCount = nullptr) const;

		// Add element to priority queue, return elementIndex of new element
		sint64 _addPovElement(const sint64 povIndex, const T value, const sint64 priority);

		// Get element indices and store them in an array, return number of elements
		uint64 _getSortedElements(const sint64 rootIdx, sint64* sortedElementIndices) const;

		// Fill a sint64_4 vector with specified values
		inline void _set(sint64_4& vec, sint64 v0, sint64 v1, sint64 v2, sint64 v3) const;

		// Rebuild pov's elements indexing as balanced BST
		sint64 _rebuild(sint64 rootIdx);

		// Return most left element index
		sint64 _getMostLeft(sint64 elementIdx) const;

		// Return most right element index
		sint64 _getMostRight(sint64 elementIdx) const;

		// Return elementIndex of previous element in priority queue (or NULL_INDEX if this is the last element).
		sint64 _previousElementIndex(sint64 elementIdx) const;

		// Return elementIndex of next element in priority queue (or NULL_INDEX if this is the last element).
		sint64 _nextElementIndex(sint64 elementIdx) const;

		// Update parent of the current element into parent of the new element, return true if exists parent
		inline bool _updateParent(const sint64 elementIdx, const sint64 newElementIdx);

		// Move the current element into new position
		void _moveElement(const sint64 srcIdx, const sint64 dstIdx);

		// Read and encode 32 POV occupation flags, return a 64bits number presents 32 occupation flags
		uint64 _getEncodedPovOccupationFlags(const uint64* povOccupationFlags, const sint64 povIndex) const;;

	public:
		// Add element to priority queue of ID pov, return elementIndex of new element
		sint64 add(const id& pov, T element, sint64 priority);

		// Return maximum number of elements that may be stored.
		static constexpr uint64 capacity()
		{
			return L;
		}

		// Remove all povs marked for removal, this is a very expensive operation
		void cleanup();

		// Return element value at elementIndex.
		inline T element(sint64 elementIndex) const;

		// Return elementIndex of first element in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 headIndex(const id& pov) const;

		// Return elementIndex of first element with priority <= maxPriority in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 headIndex(const id& pov, sint64 maxPriority) const;

		// Return elementIndex of next element in priority queue (or NULL_INDEX if this is the last element).
		sint64 nextElementIndex(sint64 elementIndex) const;

		// Return overall number of elements.
		inline uint64 population() const;

		// Return number of elements of specific PoV.
		uint64 population(const id& pov) const;

		// Return point of view elementIndex belongs to (or 0 id if unused).
		id pov(sint64 elementIndex) const;

		// Return elementIndex of previous element in priority queue (or NULL_INDEX if this is the last element).
		sint64 prevElementIndex(sint64 elementIndex) const;

		// Return priority of elementIndex (or 0 id if unused).
		sint64 priority(sint64 elementIndex) const;

		// Remove element and mark its pov for removal, if the last element.
		// Returns element index of next element in priority queue (the one following elementIdx).
		// Element indices obtained before this call are invalidated, because at least one element is moved.
		sint64 remove(sint64 elementIdx);

		// Replace *existing* element, do nothing otherwise.
		// - The element exists: replace its value.
		// - The index is out of bounds: no action is taken.
		void replace(sint64 oldElementIndex, const T& newElement);

		// Reinitialize as empty collection.
		void reset();

		// Return elementIndex of last element in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 tailIndex(const id& pov) const;

		// Return elementIndex of last element with priority >= minPriority in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 tailIndex(const id& pov, sint64 minPriority) const;
	};

	//////////

	// Divide a by b, but return 0 if b is 0 (rounding to lower magnitude in case of integers)
	template <typename T>
	inline static T div(T a, T b)
	{
		return b ? (a / b) : 0;
	}

	// Return remainder of dividing a by b, but return 0 if b is 0 (requires modulo % operator)
	template <typename T>
	inline static T mod(T a, T b)
	{
		return b ? (a % b) : 0;
	}

	//////////

	struct Entity
	{
		id publicKey;
		sint64 incomingAmount, outgoingAmount;

		// Numbers of transfers. These may overflow for entities with high traffic, such as Qx.
		uint32 numberOfIncomingTransfers, numberOfOutgoingTransfers;

		uint32 latestIncomingTransferTick, latestOutgoingTransferTick;
	};

	//////////
	
	constexpr uint16 INVALID_PROPOSAL_INDEX = 0xffff;
	constexpr uint32 INVALID_VOTER_INDEX = 0xffffffff;
	constexpr sint64 NO_VOTE_VALUE = 0x8000000000000000;

	// Input data for contract procedure call
	struct SingleProposalVoteData
	{
		// Index of proposal the vote is about (can be requested with proposal voting API)
		uint16 proposalIndex;

		// Type of proposal, see ProposalTypes
		uint16 proposalType;

		// Value of vote. NO_VOTE_VALUE means no vote for every type.
		// For proposals types with multiple options, 0 is no, 1 to N are the other options in order of definition in proposal.
		// For scalar proposal types the value is passed directly.
		sint64 voteValue;
	};

	// Proposal type constants
	struct ProposalTypes
	{
		// Options yes and no without extra data -> result is histogram of options
		static constexpr uint16 YesNo = 0;

		// Transfer given amount to address with options yes/no
		static constexpr uint16 TransferYesNo = 1;

		// Transfer amount to address with two options of amounts and option "no change"
		static constexpr uint16 TransferTwoAmounts = 2;

		// Transfer amount to address with three options of amounts and option "no change"
		static constexpr uint16 TransferThreeAmounts = 3;

		// Transfer amount to address with four options of amounts and option "no change"
		static constexpr uint16 TransferFourAmounts = 4;

		// Set given variable to proposed value with options yes/no
		static constexpr uint16 VariableYesNo = 10;

		// Set given variable to proposed value with two options of values and option "no change"
		static constexpr uint16 VariableTwoValues = 11;

		// Set given variable to proposed value with three options of values and option "no change"
		static constexpr uint16 VariableThreeValues = 12;

		// Set given variable to proposed value with four options of values and option "no change"
		static constexpr uint16 VariableFourValues = 13;

		// Set given variable to value, allowing to vote with scalar value, voting result is mean value
		static constexpr uint16 VariableScalarMean = 20;
	};

	// Proposal data struct for all types of proposals defined in August 2024.
	// Input data for contract procedure call, usable as ProposalDataType in ProposalVoting
	// You have to choose, whether to support scalar votes next to option votes. Scalar votes require 8x more storage in the state.
	template <bool SupportScalarVotes>
	struct ProposalDataV1
	{
		// URL explaining proposal, zero-terminated string.
		array<uint8, 256> url;	
		
		// Epoch, when proposal is active. For setProposal(), this must be current epoch or 0 (to clear proposal).
		uint16 epoch;

		// Type of proposal, see ProposalTypes.
		uint16 type;

		// Proposal payload data (for all except ProposalTypes::YesNo)
		union
		{
			struct Transfer
			{
				id targetAddress;
				array<sint64, 4> amounts;
			} transfer;
			struct VariableOptions
			{
				array<sint64, 4> values;
				uint16 variable;
			} variableOptions;
			struct VariableScalar
			{
				sint64 minValue;
				sint64 maxValue;
				sint64 proposedValue;
				uint16 variable;
			} variableScalar;
		};

		// Check if content of instance are valid. Epoch is not checked.
		// Also useful to show requirements of valid proposal.
		bool checkValidity() const
		{
			bool okay = false;
			// TODO: validate URL
			switch (type)
			{
			case ProposalTypes::YesNo:
				okay = true;
				break;
			case ProposalTypes::TransferYesNo:
			case ProposalTypes::TransferTwoAmounts:
			case ProposalTypes::TransferThreeAmounts:
			case ProposalTypes::TransferFourAmounts:
				if (!isZero(transfer.targetAddress))
				{
					uint16 proposedAmounts = type - ProposalTypes::TransferYesNo + 1;
					okay = true;
					for (uint16 i = 0; i < proposedAmounts; ++i)
					{
						if (transfer.amounts.get(i) < 0)
							okay = false;
					}
					for (uint16 i = proposedAmounts; i < 4; ++i)
					{
						if (transfer.amounts.get(i) != 0)
							okay = false;
					}
				}
				break;
			case ProposalTypes::VariableYesNo:
			case ProposalTypes::VariableTwoValues:
			case ProposalTypes::VariableThreeValues:
			case ProposalTypes::VariableFourValues:
				{
					uint16 proposedValues = type - ProposalTypes::VariableYesNo + 1;
					okay = true;
					for (uint16 i = 0; i < proposedValues; ++i)
					{
						if (variableOptions.values.get(i) < 0)
							okay = false;
					}
					for (uint16 i = proposedValues; i < 4; ++i)
					{
						if (variableOptions.values.get(i) != 0)
							okay = false;
					}
				}
				break;
			case ProposalTypes::VariableScalarMean:
				if (supportScalarVotes)
					okay = variableScalar.minValue <= variableScalar.proposedValue
						&& variableScalar.proposedValue <= variableScalar.maxValue
						&& variableScalar.minValue > NO_VOTE_VALUE;
				break;
			}
			return okay;
		}

		// Whether to support scalar votes next to option votes.
		static constexpr bool supportScalarVotes = SupportScalarVotes;
	};

	// Proposal data struct for yes/no proposal type only (requires less storage space).
	// Input data for contract procedure call, usable as ProposalDataType in ProposalVoting
	struct ProposalDataYesNo
	{
		array<uint8, 256> url;
		uint16 epoch;

		// Type of proposal
		uint16 type;

		bool checkValidity() const
		{
			bool okay = type == ProposalTypes::YesNo;
			// TODO: validate URL
			return okay;
		}
	};


	// Used internally by ProposalVoting to store a proposal with all votes
	template <typename ProposalDataType, uint32 numOfVoters>
	struct ProposalWithAllVoteData;


	template <uint16 proposalSlotCount = NUMBER_OF_COMPUTORS>
	struct ProposalAndVotingByComputors;

	template <unsigned int maxShareholders>
	struct ProposalAndVotingByShareholders;

	/*
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
		static constexpr uint32 maxVoters = ProposerAndVoterHandlingT::maxVoters;

		typedef ProposerAndVoterHandlingT ProposerAndVoterHandlingType;
		typedef ProposalDataT ProposalDataType;
		typedef ProposalWithAllVoteData<
			ProposalDataT,
			maxVoters
		> ProposalAndVotesDataType;

		static_assert(maxProposals <= INVALID_PROPOSAL_INDEX);
		static_assert(maxVoters <= INVALID_VOTER_INDEX);

		// Handling of who has the right to propose and to vote + proposal / voter indices
		ProposerAndVoterHandlingType proposersAndVoters;

		// Proposals and corresponding votes
		ProposalAndVotesDataType proposals[maxProposals];
	};

	// Proposal user interface available in contract functions
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalFunctionCall
	{
		const QpiContextFunctionCall& qpi;
		ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& pv;

		// Get proposal with given index if index is valid and proposal is set (epoch > 0)
		bool getProposal(uint16 proposalIndex, ProposalDataType& proposal) const;

		// Get data of single vote
		bool getVote(uint16 proposalIndex, uint32 voterIndex, SingleProposalVoteData& vote) const;

		// Return index of existing proposal or INVALID_PROPOSAL_INDEX if there is no proposal by given proposer
		uint16 proposalIndex(const id& proposerId) const;

		// Return proposer ID of given proposal index or NULL_ID if there is no proposal at this index
		id proposerId(uint16 proposalIndex) const;

		// Return voter index for given ID or INVALID_VOTER_INDEX if ID has no right to vote
		uint32 voterIndex(const id& voterId) const;

		// Return ID for given voter index or NULL_ID if index is invalid
		id voterId(uint32 voterIndex) const;

		// Return next proposal index of active proposal (voting possible this epoch)
		// or -1 if there are not any more such proposals behind the passed index.
		// Pass -1 to get first index.
		sint32 nextActiveProposalIndex(sint32 prevProposalIndex) const;

		// Return next proposal index of finished proposal (voting not possible anymore)
		// or -1 if there are not any more such proposals behind the passed index.
		// Pass -1 to get first index.
		sint32 nextFinishedProposalIndex(sint32 prevProposalIndex) const;

		// TODO:
		// get current result of voting (number of votes + current overall result)

		// Constructor. Use qpi(proposalVotingObject) to construct instance.
		QpiContextProposalFunctionCall(
			const QpiContextFunctionCall& qpi,
			ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& pv
		) : qpi(qpi), pv(pv) {}
	};

	// Proposal user interface available in contract procedures
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalProcedureCall : public QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>
	{
		bool setProposal(
			const id& proposer,
			const ProposalDataType& proposal
		);

		bool clearProposal(
			uint16 proposalIndex
		);

		bool vote(
			const id& voter,
			const SingleProposalVoteData& vote
		);



		// Constructor. Use qpi(proposalVotingObject) to construct instance.
		QpiContextProposalProcedureCall(
			const QpiContextFunctionCall& qpi,
			ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& pv
		) : QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>(qpi, pv) {}
	};

	//////////

	// QPI context base class (common data, by default has no stack for locals)
	struct QpiContext
	{
	protected:
		// Construction is done in core, not allowed in contracts
		QpiContext(
			unsigned int contractIndex,
			const m256i& originator,
			const m256i& invocator,
			long long invocationReward
		) {
			init(contractIndex, originator, invocator, invocationReward);
		}

		void init(
			unsigned int contractIndex,
			const m256i& originator,
			const m256i& invocator,
			long long invocationReward
		) {
			_currentContractIndex = contractIndex;
			_currentContractId = m256i(contractIndex, 0, 0, 0);
			_originator = originator;
			_invocator = invocator;
			_invocationReward = invocationReward;
			_stackIndex = -1;
		}

		unsigned int _currentContractIndex;
		m256i _currentContractId, _originator, _invocator;
		long long _invocationReward;
		int _stackIndex;

	private:
		// Disabling copy and move
		QpiContext(const QpiContext&) = delete;
		QpiContext(QpiContext&&) = delete;
		QpiContext& operator=(const QpiContext&) = delete;
		QpiContext& operator=(QpiContext&&) = delete;
	};

	// QPI function available to contract functions and procedures
	struct QpiContextFunctionCall : public QpiContext
	{
		id arbitrator(
		) const;

		id computor(
			uint16 computorIndex // [0..675]
		) const;

		uint8 day(
		) const; // [1..31]

		uint8 dayOfWeek(
			uint8 year, // (0 = 2000, 1 = 2001, ..., 99 = 2099)
			uint8 month,
			uint8 day
		) const; // [0..6]

		uint16 epoch(
		) const; // [0..9'999]

		bit getEntity(
			const id& id,
			Entity& entity
		) const; // Returns "true" if the entity has been found, returns "false" otherwise

		uint8 hour(
		) const; // [0..23]

		// Return the invocation reward (amount transferred to contract immediately before invoking)
		sint64 invocationReward(
		) const {
			return _invocationReward;
		}

		// Returns the id of the user/contract who has triggered this contract; returns NULL_ID if there has been no user/contract
		id invocator(
		) const {
			return _invocator;
		}

		template <typename T>
		id K12(
			const T& data
		) const;

		uint16 millisecond(
		) const; // [0..999]

		uint8 minute(
		) const; // [0..59]

		uint8 month(
		) const; // [1..12]

		id nextId(
			const id& currentId
		) const;

		sint64 numberOfPossessedShares(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			uint16 ownershipManagingContractIndex,
			uint16 possessionManagingContractIndex
		) const;

		sint32 numberOfTickTransactions(
		) const;

		// Returns the id of the user who has triggered the whole chain of invocations with their transaction; returns NULL_ID if there has been no user
		id originator(
		) const {
			return _originator;
		}

		uint8 second(
		) const; // [0..59]

		bit signatureValidity(
			const id& entity,
			const id& digest,
			const array<sint8, 64>& signature
		) const;

		uint32 tick(
		) const; // [0..999'999'999]

		uint8 year(
		) const; // [0..99] (0 = 2000, 1 = 2001, ..., 99 = 2099)

		// Access proposal functions with qpi(proposalVotingObject).func().
		template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
		inline QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType> operator()(
			const ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
		) const;

		// Internal functions, calling not allowed in contracts
		void* __qpiAllocLocals(unsigned int sizeOfLocals) const;
		void __qpiFreeLocals() const;
		const QpiContextFunctionCall& __qpiConstructContextOtherContractFunctionCall(unsigned int otherContractIndex) const;
		void __qpiFreeContextOtherContract() const;
		void * __qpiAcquireStateForReading(unsigned int contractIndex) const;
		void __qpiReleaseStateForReading(unsigned int contractIndex) const;
		void __qpiAbort(unsigned int errorCode) const;

	protected:
		// Construction is done in core, not allowed in contracts
		QpiContextFunctionCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QpiContext(contractIndex, originator, originator, invocationReward) {}
	};

	// QPI procedures available to contract procedures (not to contract functions)
	struct QpiContextProcedureCall : public QPI::QpiContextFunctionCall
	{
		bool acquireShares(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			uint16 sourceOwnershipManagingContractIndex,
			uint16 sourcePossessionManagingContractIndex
		) const;

		sint64 burn(
			sint64 amount
		) const;

		sint64 issueAsset(
			uint64 name,
			const id& issuer,
			sint8 numberOfDecimalPlaces,
			sint64 numberOfShares,
			uint64 unitOfMeasurement
		) const; // Returns number of shares or 0 on error

		bool releaseShares(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			uint16 destinationOwnershipManagingContractIndex,
			uint16 destinationPossessionManagingContractIndex
		) const;

		sint64 transfer( // Attempts to transfer energy from this qubic
			const id& destination, // Destination to transfer to, use NULL_ID to destroy the transferred energy
			sint64 amount // Energy amount to transfer, must be in [0..1'000'000'000'000'000] range
		) const; // Returns remaining energy amount; if the value is less than 0 then the attempt has failed, in this case the absolute value equals to the insufficient amount

		sint64 transferShareOwnershipAndPossession(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			const id& newOwnerAndPossessor
		) const; // Returns remaining number of possessed shares satisfying all the conditions; if the value is less than 0 then the attempt has failed, in this case the absolute value equals to the insufficient number

		// Access proposal procedures with qpi(proposalVotingObject).proc().
		template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
		inline QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType> operator()(
			ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
		) const;


		// Internal functions, calling not allowed in contracts
		const QpiContextProcedureCall& __qpiConstructContextOtherContractProcedureCall(unsigned int otherContractIndex, sint64 invocationReward) const;
		void* __qpiAcquireStateForWriting(unsigned int contractIndex) const;
		void __qpiReleaseStateForWriting(unsigned int contractIndex) const;
		template <unsigned int sysProcId, typename InputType, typename OutputType>
		void __qpiCallSystemProcOfOtherContract(unsigned int otherContractIndex, InputType& input, OutputType& output, sint64 invocationReward) const;

	protected:
		// Construction is done in core, not allowed in contracts
		QpiContextProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QpiContextFunctionCall(contractIndex, originator, invocationReward) {}
	};

	// QPI available in REGISTER_USER_FUNCTIONS_AND_PROCEDURES
	struct QpiContextForInit : public QPI::QpiContext
	{
		void __registerUserFunction(USER_FUNCTION, unsigned short, unsigned short, unsigned short, unsigned int) const;
		void __registerUserProcedure(USER_PROCEDURE, unsigned short, unsigned short, unsigned short, unsigned int) const;

		// Construction is done in core, not allowed in contracts
		QpiContextForInit(unsigned int contractIndex) : QpiContext(contractIndex, NULL_ID, NULL_ID, 0) {}
	};

	// Used if no locals, input, or output is needed in a procedure or function
	struct NoData {
		bool isEmpty; // a flag to mark if the procedure do nothing => no need to mark the SC state change flag => no need to recompute K12 of the whole state, K12 the whole SC state is expensive!
		// TODO: consider changing NoData to something else meaningful
	};

	// Management rights transfer: pre-transfer input
	struct PreManagementRightsTransfer_input
	{
		uint64 assetName;
		id issuer;
		id owner;
		id possessor;
		sint64 numberOfShares;
	};

	// Management rights transfer: pre-transfer output
	struct PreManagementRightsTransfer_output
	{
		bool ok;
	};

	// Management rights transfer: post-transfer input
	struct PostManagementRightsTransfer_input
	{
		uint64 assetName;
		id issuer;
		id owner;
		id possessor;
		sint64 numberOfShares;
	};

	//////////

	#define INITIALIZE public: static void __initialize(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; input.isEmpty = false;

	#define BEGIN_EPOCH public: static void __beginEpoch(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; input.isEmpty = false;

	#define END_EPOCH public: static void __endEpoch(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; input.isEmpty = false;

	#define BEGIN_TICK public: static void __beginTick(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; input.isEmpty = false;

	#define END_TICK public: static void __endTick(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; input.isEmpty = false;

	#define EMPTY_INITIALIZE public: static void __initialize(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { input.isEmpty = true;

	#define EMPTY_BEGIN_EPOCH public: static void __beginEpoch(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { input.isEmpty = true;

	#define EMPTY_END_EPOCH public: static void __endEpoch(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { input.isEmpty = true;

	#define EMPTY_BEGIN_TICK public: static void __beginTick(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { input.isEmpty = true;

	#define EMPTY_END_TICK public: static void __endTick(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, NoData& input, NoData& output) { input.isEmpty = true;

	#define EXPAND public: static void __expand(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, CONTRACT_STATE2_TYPE& state2, NoData& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define PRE_ACQUIRE_SHARES public: static void __preAcquireShares(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, PreManagementRightsTransfer_input& input, PreManagementRightsTransfer_output& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define PRE_RELEASE_SHARES public: static void __preReleaseShares(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, PreManagementRightsTransfer_input& input, PreManagementRightsTransfer_output& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define POST_ACQUIRE_SHARES public: static void __postAcquireShares(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, PostManagementRightsTransfer_input& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define POST_RELEASE_SHARES public: static void __postReleaseShares(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, PostManagementRightsTransfer_input& input, NoData& output) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;


	#define LOG_DEBUG(message) __logContractDebugMessage(CONTRACT_INDEX, message);

	#define LOG_ERROR(message) __logContractErrorMessage(CONTRACT_INDEX, message);

	#define LOG_INFO(message) __logContractInfoMessage(CONTRACT_INDEX, message);

	#define LOG_WARNING(message) __logContractWarningMessage(CONTRACT_INDEX, message);

	#define PRIVATE_FUNCTION(function) \
		private: \
			typedef QPI::NoData function##_locals; \
			PRIVATE_FUNCTION_WITH_LOCALS(function)

	#define PRIVATE_FUNCTION_WITH_LOCALS(function) \
		private: \
			enum { __is_function_##function = true }; \
			static void function(const QPI::QpiContextFunctionCall& qpi, const CONTRACT_STATE_TYPE& state, function##_input& input, function##_output& output, function##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define PRIVATE_PROCEDURE(procedure) \
		private: \
			typedef QPI::NoData procedure##_locals; \
			PRIVATE_PROCEDURE_WITH_LOCALS(procedure);

	#define PRIVATE_PROCEDURE_WITH_LOCALS(procedure) \
		private: \
			enum { __is_function_##procedure = false }; \
			static void procedure(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define PUBLIC_FUNCTION(function) \
		public: \
			typedef QPI::NoData function##_locals; \
			PUBLIC_FUNCTION_WITH_LOCALS(function);

	#define PUBLIC_FUNCTION_WITH_LOCALS(function) \
		public: \
			enum { __is_function_##function = true }; \
			static void function(const QPI::QpiContextFunctionCall& qpi, const CONTRACT_STATE_TYPE& state, function##_input& input, function##_output& output, function##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define PUBLIC_PROCEDURE(procedure) \
		public: \
			typedef QPI::NoData procedure##_locals; \
			PUBLIC_PROCEDURE_WITH_LOCALS(procedure);

	#define PUBLIC_PROCEDURE_WITH_LOCALS(procedure) \
		public: \
			enum { __is_function_##procedure = false }; \
			static void procedure(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define REGISTER_USER_FUNCTIONS_AND_PROCEDURES \
		public: \
			enum { __contract_index = CONTRACT_INDEX }; \
			static void __registerUserFunctionsAndProcedures(const QPI::QpiContextForInit& qpi) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;

	#define _ }

	#define REGISTER_USER_FUNCTION(userFunction, inputType) \
		static_assert(__is_function_##userFunction, #userFunction " is procedure"); \
		static_assert(sizeof(userFunction##_output) <= 65536, #userFunction "_output size too large"); \
		static_assert(sizeof(userFunction##_input) <= 65536, #userFunction "_input size too large"); \
		static_assert(sizeof(userFunction##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userFunction "_locals size too large"); \
		qpi.__registerUserFunction((USER_FUNCTION)userFunction, inputType, sizeof(userFunction##_input), sizeof(userFunction##_output), sizeof(userFunction##_locals));

	#define REGISTER_USER_PROCEDURE(userProcedure, inputType) \
		static_assert(!__is_function_##userProcedure, #userProcedure " is function"); \
		static_assert(sizeof(userProcedure##_output) <= 65536, #userProcedure "_output size too large"); \
		static_assert(sizeof(userProcedure##_input) <= 65536, #userProcedure "_input size too large"); \
		static_assert(sizeof(userProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userProcedure "_locals size too large"); \
		qpi.__registerUserProcedure((USER_PROCEDURE)userProcedure, inputType, sizeof(userProcedure##_input), sizeof(userProcedure##_output), sizeof(userProcedure##_locals));

	// Call function or procedure of current contract
	#define CALL(functionOrProcedure, input, output) \
		static_assert(sizeof(CONTRACT_STATE_TYPE::functionOrProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #functionOrProcedure "_locals size too large"); \
		functionOrProcedure(qpi, state, input, output, *(functionOrProcedure##_locals*)qpi.__qpiAllocLocals(sizeof(CONTRACT_STATE_TYPE::functionOrProcedure##_locals))); \
		qpi.__qpiFreeLocals()

	// Call function of other contract
	#define CALL_OTHER_CONTRACT_FUNCTION(contractStateType, function, input, output) \
		static_assert(sizeof(contractStateType::function##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #function "_locals size too large"); \
		static_assert(contractStateType::__is_function_##function, "CALL_OTHER_CONTRACT_FUNCTION() cannot be used to invoke procedures."); \
		static_assert(!(contractStateType::__contract_index == CONTRACT_STATE_TYPE::__contract_index), "Use CALL() to call a function of this contract."); \
		static_assert(contractStateType::__contract_index < CONTRACT_STATE_TYPE::__contract_index, "You can only call contracts with lower index."); \
		contractStateType::function( \
			qpi.__qpiConstructContextOtherContractFunctionCall(contractStateType::__contract_index), \
			*(contractStateType*)qpi.__qpiAcquireStateForReading(contractStateType::__contract_index), \
			input, output, \
			*(contractStateType::function##_locals*)qpi.__qpiAllocLocals(sizeof(contractStateType::function##_locals))); \
		qpi.__qpiReleaseStateForReading(contractStateType::__contract_index); \
		qpi.__qpiFreeContextOtherContract(contractStateType::__contract_index); \
		qpi.__qpiFreeLocals()

	// Transfer invocation reward and invoke of other contract (procedure only)
	#define INVOKE_OTHER_CONTRACT_PROCEDURE(contractStateType, procedure, input, output, invocationReward) \
		static_assert(sizeof(contractStateType::procedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #procedure "_locals size too large"); \
		static_assert(!contractStateType::__is_function_##procedure, "INVOKE_OTHER_CONTRACT_PROCEDURE() cannot be used to call functions."); \
		static_assert(!(contractStateType::__contract_index == CONTRACT_STATE_TYPE::__contract_index), "Use CALL() to call a function/procedure of this contract."); \
		static_assert(contractStateType::__contract_index < CONTRACT_STATE_TYPE::__contract_index, "You can only call contracts with lower index."); \
		static_assert(invocationReward >= 0, "The invocationReward cannot be negative!"); \
		contractStateType::procedure( \
			qpi.__qpiConstructContextOtherContractProcedureCall(contractStateType::__contract_index, invocationReward), \
			*(contractStateType*)qpi.__qpiAcquireStateForWriting(contractStateType::__contract_index), \
			input, output, \
			*(contractStateType::procedure##_locals*)qpi.__qpiAllocLocals(sizeof(contractStateType::procedure##_locals))); \
		qpi.__qpiReleaseStateForWriting(contractStateType::__contract_index); \
		qpi.__qpiFreeContextOtherContract(contractStateType::__contract_index); \
		qpi.__qpiFreeLocals()

	#define SELF id(CONTRACT_INDEX, 0, 0, 0)

	#define SELF_INDEX CONTRACT_INDEX
}
