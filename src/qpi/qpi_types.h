#pragma once

// m256i is used for the id data type
#include "../platform/m256.h"

// uint128
#include "../platform/uint128.h"

namespace QPI
{
	typedef signed char sint8;
	typedef unsigned char uint8;
	typedef signed short sint16;
	typedef unsigned short uint16;
	typedef signed int sint32;
	typedef unsigned int uint32;
	typedef signed long long sint64;
	typedef unsigned long long uint64;

	//////////

#define NULL_ID id::zero()

	constexpr sint64 NULL_INDEX = -1;

	constexpr sint64 INVALID_AMOUNT = 0x8000000000000000;

#define NUMBER_OF_COMPUTORS 676
#define QUORUM (NUMBER_OF_COMPUTORS * 2 / 3 + 1)

	//////////

	// Boolean type ensuring that input values > 1 are mapped to 1.
	struct bit
	{
		bit(bool v = false) : charValue(v)
		{
		}

		operator bool() const
		{
			return !!charValue;
		}

		char charValue;
	};

	//////////

	// Error codes for inter-contract calls (used when calling other contracts fails)
	// These are returned to the calling contract so it can handle the error
	enum InterContractCallError : uint8
	{
		NoCallError = 0,
		CallErrorContractInErrorState = 1,      // Called contract is already in error state
		CallErrorInsufficientFees = 2,          // Called contract has no execution fee reserve
		CallErrorAllocationFailed = 3,          // Failed to allocate context on stack
		CallErrorContractInactive = 4,			// Called contract has been inactive
	};

	typedef uint128_t uint128;
	typedef m256i id;

	//////////

	struct Entity
	{
		id publicKey;
		sint64 incomingAmount, outgoingAmount;

		// Numbers of transfers. These may overflow for entities with high traffic, such as Qx.
		uint32 numberOfIncomingTransfers, numberOfOutgoingTransfers;

		uint32 latestIncomingTransferTick, latestOutgoingTransferTick;
	};

	struct Asset
	{
		id issuer;
		uint64 assetName;
	};

	//////////

	namespace TransferType
	{
		constexpr uint8 standardTransaction = 0;
		constexpr uint8 procedureTransaction = 1;
		constexpr uint8 qpiTransfer = 2;
		constexpr uint8 qpiDistributeDividends = 3;
		constexpr uint8 revenueDonation = 4;
		constexpr uint8 ipoBidRefund = 5;
		constexpr uint8 procedureInvocationByOtherContract = 6;
	};

	//////////

	// Wrapper around a contract's entire state struct.
	// sizeof(ContractState<T, contractIndex>) == sizeof(T), standard layout, zero-init compatible.
	// Use get() for reads, mut() for writes (marks dirty).
	template <typename T, unsigned int contractIndex>
	struct ContractState {
		static constexpr unsigned int __contract_index = contractIndex;
		const T& get() const { return _data; }
		T& mut() { ::__markContractStateDirty(contractIndex); return _data; }
	private:
		T _data;
	};

	//////////

	// Used if no locals, input, or output is needed in a procedure or function
	struct NoData {};

	//////////

	struct QpiContextProcedureCall; // forward declaration

	struct ContractBase
	{
		struct StateData {};
		enum { __initializeEmpty = 1, __initializeLocalsSize = sizeof(NoData) };
		static void __initialize(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __beginEpochEmpty = 1, __beginEpochLocalsSize = sizeof(NoData) };
		static void __beginEpoch(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __endEpochEmpty = 1, __endEpochLocalsSize = sizeof(NoData) };
		static void __endEpoch(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __beginTickEmpty = 1, __beginTickLocalsSize = sizeof(NoData) };
		static void __beginTick(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __endTickEmpty = 1, __endTickLocalsSize = sizeof(NoData) };
		static void __endTick(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __preAcquireSharesEmpty = 1, __preAcquireSharesLocalsSize = sizeof(NoData) };
		static void __preAcquireShares(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __preReleaseSharesEmpty = 1, __preReleaseSharesLocalsSize = sizeof(NoData) };
		static void __preReleaseShares(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __postAcquireSharesEmpty = 1, __postAcquireSharesLocalsSize = sizeof(NoData) };
		static void __postAcquireShares(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __postReleaseSharesEmpty = 1, __postReleaseSharesLocalsSize = sizeof(NoData) };
		static void __postReleaseShares(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __postIncomingTransferEmpty = 1, __postIncomingTransferLocalsSize = sizeof(NoData) };
		static void __postIncomingTransfer(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __setShareholderProposalEmpty = 1, __setShareholderProposalLocalsSize = sizeof(NoData) };
		static void __setShareholderProposal(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __setShareholderVotesEmpty = 1, __setShareholderVotesLocalsSize = sizeof(NoData) };
		static void __setShareholderVotes(const QpiContextProcedureCall&, void*, void*, void*, void*) {}
		enum { __expandEmpty = 1 };
		static void __expand(const QpiContextProcedureCall& qpi, void*, void*) {}
		enum { __migrateEmpty = 1, __migrateOldStateSize = 0, __migrateLocalsSize = sizeof(NoData) };
		static void __migrate(const QpiContextProcedureCall& qpi, void* state, void* oldState, void* locals) {}
	};

	//////////

	// Characters for building strings (for example in constructor of id / m256i)
	namespace Ch
	{
		enum : char
		{
			null = 0,
			space = ' ', slash = '/', backslash = '\\', dot = '.', comma = ',', colon = ':', semicolon = ';',
			underscore = '_', minus = '-', plus = '+', star = '*', dollar = '$', question_mark = '?', exclamation_mark = '!',
			a = 'a', b = 'b', c = 'c', d = 'd', e = 'e', f = 'f', g = 'g', h = 'h', i = 'i', j = 'j', k = 'k', l = 'l', m = 'm',
			n = 'n', o = 'o', p = 'p', q = 'q', r = 'r', s = 's', t = 't', u = 'u', v = 'v', w = 'w', x = 'x', y = 'y', z = 'z',
			A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H', I = 'I', J = 'J', K = 'K', L = 'L', M = 'M',
			N = 'N', O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',
			_0 = '0', _1 = '1', _2 = '2', _3 = '3', _4 = '4', _5 = '5', _6 = '6', _7 = '7', _8 = '8', _9 = '9',
		};
	}

	// Letters for defining identity with ID function
	constexpr long long _A = 0;
	constexpr long long _B = 1;
	constexpr long long _C = 2;
	constexpr long long _D = 3;
	constexpr long long _E = 4;
	constexpr long long _F = 5;
	constexpr long long _G = 6;
	constexpr long long _H = 7;
	constexpr long long _I = 8;
	constexpr long long _J = 9;
	constexpr long long _K = 10;
	constexpr long long _L = 11;
	constexpr long long _M = 12;
	constexpr long long _N = 13;
	constexpr long long _O = 14;
	constexpr long long _P = 15;
	constexpr long long _Q = 16;
	constexpr long long _R = 17;
	constexpr long long _S = 18;
	constexpr long long _T = 19;
	constexpr long long _U = 20;
	constexpr long long _V = 21;
	constexpr long long _W = 22;
	constexpr long long _X = 23;
	constexpr long long _Y = 24;
	constexpr long long _Z = 25;

	inline id ID(long long _00, long long _01, long long _02, long long _03, long long _04, long long _05, long long _06, long long _07, long long _08, long long _09,
		long long _10, long long _11, long long _12, long long _13, long long _14, long long _15, long long _16, long long _17, long long _18, long long _19,
		long long _20, long long _21, long long _22, long long _23, long long _24, long long _25, long long _26, long long _27, long long _28, long long _29,
		long long _30, long long _31, long long _32, long long _33, long long _34, long long _35, long long _36, long long _37, long long _38, long long _39,
		long long _40, long long _41, long long _42, long long _43, long long _44, long long _45, long long _46, long long _47, long long _48, long long _49,
		long long _50, long long _51, long long _52, long long _53, long long _54, long long _55)
	{
		return _mm256_set_epi64x(((((((((((((((uint64)_55) * 26 + _54) * 26 + _53) * 26 + _52) * 26 + _51) * 26 + _50) * 26 + _49) * 26 + _48) * 26 + _47) * 26 + _46) * 26 + _45) * 26 + _44) * 26 + _43) * 26 + _42, ((((((((((((((uint64)_41) * 26 + _40) * 26 + _39) * 26 + _38) * 26 + _37) * 26 + _36) * 26 + _35) * 26 + _34) * 26 + _33) * 26 + _32) * 26 + _31) * 26 + _30) * 26 + _29) * 26 + _28, ((((((((((((((uint64)_27) * 26 + _26) * 26 + _25) * 26 + _24) * 26 + _23) * 26 + _22) * 26 + _21) * 26 + _20) * 26 + _19) * 26 + _18) * 26 + _17) * 26 + _16) * 26 + _15) * 26 + _14, ((((((((((((((uint64)_13) * 26 + _12) * 26 + _11) * 26 + _10) * 26 + _09) * 26 + _08) * 26 + _07) * 26 + _06) * 26 + _05) * 26 + _04) * 26 + _03) * 26 + _02) * 26 + _01) * 26 + _00);
	}

	// Months
	constexpr int JANUARY = 1;
	constexpr int FEBRUARY = 2;
	constexpr int MARCH = 3;
	constexpr int APRIL = 4;
	constexpr int MAY = 5;
	constexpr int JUNE = 6;
	constexpr int JULY = 7;
	constexpr int AUGUST = 8;
	constexpr int SEPTEMBER = 9;
	constexpr int OCTOBER = 10;
	constexpr int NOVEMBER = 11;
	constexpr int DECEMBER = 12;


	// Weekdays
	constexpr int WEDNESDAY = 0;
	constexpr int THURSDAY = 1;
	constexpr int FRIDAY = 2;
	constexpr int SATURDAY = 3;
	constexpr int SUNDAY = 4;
	constexpr int MONDAY = 5;
	constexpr int TUESDAY = 6;

	constexpr unsigned long long X_MULTIPLIER = 1ULL;
}
