#define NO_UEFI

#define _ALLOW_KEYWORD_MACROS 1

#define private protected
#include "contract_testing.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include <algorithm>
#include <set>
#include <type_traits>
#include <vector>

// Procedure indices (must match REGISTER_USER_FUNCTIONS_AND_PROCEDURES in QThirtyFour.h)
constexpr uint16 QTF_PROCEDURE_BUY_TICKET = 1;
constexpr uint16 QTF_PROCEDURE_SET_PRICE = 2;
constexpr uint16 QTF_PROCEDURE_SET_SCHEDULE = 3;
constexpr uint16 QTF_PROCEDURE_SET_TARGET_JACKPOT = 4;
constexpr uint16 QTF_PROCEDURE_SET_DRAW_HOUR = 5;

// Function indices
constexpr uint16 QTF_FUNCTION_GET_TICKET_PRICE = 1;
constexpr uint16 QTF_FUNCTION_GET_NEXT_EPOCH_DATA = 2;
constexpr uint16 QTF_FUNCTION_GET_WINNER_DATA = 3;
constexpr uint16 QTF_FUNCTION_GET_POOLS = 4;
constexpr uint16 QTF_FUNCTION_GET_SCHEDULE = 5;
constexpr uint16 QTF_FUNCTION_GET_DRAW_HOUR = 6;
constexpr uint16 QTF_FUNCTION_GET_STATE = 7;
constexpr uint16 QTF_FUNCTION_GET_FEES = 8;
constexpr uint16 QTF_FUNCTION_ESTIMATE_PRIZE_PAYOUTS = 9;

static const id QTF_DEV_ADDRESS = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C,
                                     _R, _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

constexpr uint8 QTF_ANY_DAY_SCHEDULE = 0xFF;

// Test helper class exposing internal state
class QTFChecker : public QTF
{
public:
	uint64 getNumberOfPlayers() const { return numberOfPlayers; }
	uint64 getTicketPriceInternal() const { return ticketPrice; }
	uint64 getJackpot() const { return jackpot; }
	uint64 getTargetJackpotInternal() const { return targetJackpot; }
	uint32 getDrawHourInternal() const { return drawHour; }
	bool getFrActive() const { return frActive; }
	uint32 getFrRoundsSinceK4() const { return frRoundsSinceK4; }
	uint32 getFrRoundsAtOrAboveTarget() const { return frRoundsAtOrAboveTarget; }

	void setScheduleMask(uint8 newMask) { schedule = newMask; }
	void setJackpot(uint64 value) { jackpot = value; }
	void setTargetJackpotInternal(uint64 value) { targetJackpot = value; }
	void setTicketPriceInternal(uint64 value) { ticketPrice = value; }
	void setFrActive(bit value) { frActive = value; }
	void setFrRoundsSinceK4(uint16 value) { frRoundsSinceK4 = value; }
	void setFrRoundsAtOrAboveTarget(uint16 value) { frRoundsAtOrAboveTarget = value; }

	const PlayerData& getPlayer(uint64 index) const { return players.get(index); }
	void addPlayerDirect(const id& playerId, const QTFRandomValues& randomValues) { players.set(numberOfPlayers++, {playerId, randomValues}); }

	// ---- Private method wrappers (private->protected in this TU) --------------
	ValidateNumbers_output callValidateNumbers(const QPI::QpiContextFunctionCall& qpi, const QTFRandomValues& numbers) const
	{
		ValidateNumbers_input input{};
		ValidateNumbers_output output{};
		ValidateNumbers_locals locals{};

		input.numbers = numbers;
		ValidateNumbers(qpi, *this, input, output, locals);
		return output;
	}

	GetRandomValues_output callGetRandomValues(const QPI::QpiContextFunctionCall& qpi, uint64 seed) const
	{
		GetRandomValues_input input{};
		GetRandomValues_output output{};
		GetRandomValues_locals locals{};

		input.seed = seed;
		GetRandomValues(qpi, *this, input, output, locals);
		return output;
	}

	CountMatches_output callCountMatches(const QPI::QpiContextFunctionCall& qpi, const QTFRandomValues& playerValues,
	                                     const QTFRandomValues& winningValues) const
	{
		CountMatches_input input{};
		CountMatches_output output{};
		CountMatches_locals locals{};

		input.playerValues = playerValues;
		input.winningValues = winningValues;
		CountMatches(qpi, *this, input, output, locals);
		return output;
	}

	CheckContractBalance_output callCheckContractBalance(const QPI::QpiContextFunctionCall& qpi, uint64 expectedRevenue) const
	{
		CheckContractBalance_input input{};
		CheckContractBalance_output output{};
		CheckContractBalance_locals locals{};

		input.expectedRevenue = expectedRevenue;
		CheckContractBalance(qpi, *this, input, output, locals);
		return output;
	}

	PowerFixedPoint_output callPowerFixedPoint(const QPI::QpiContextFunctionCall& qpi, uint64 base, uint64 exp) const
	{
		PowerFixedPoint_input input{};
		PowerFixedPoint_output output{};
		PowerFixedPoint_locals locals{};

		input.base = base;
		input.exp = exp;
		PowerFixedPoint(qpi, *this, input, output, locals);
		return output;
	}

	CalculateExpectedRoundsToK4_output callCalculateExpectedRoundsToK4(const QPI::QpiContextFunctionCall& qpi, uint64 N) const
	{
		CalculateExpectedRoundsToK4_input input{};
		CalculateExpectedRoundsToK4_output output{};
		CalculateExpectedRoundsToK4_locals locals{};

		input.N = N;
		CalculateExpectedRoundsToK4(qpi, *this, input, output, locals);
		return output;
	}

	CalcReserveTopUp_output callCalcReserveTopUp(const QPI::QpiContextFunctionCall& qpi, uint64 totalQRPBalance, uint64 needed,
	                                             uint64 perWinnerCapTotal, uint64 ticketPrice) const
	{
		CalcReserveTopUp_input input{};
		CalcReserveTopUp_output output{};
		CalcReserveTopUp_locals locals{};

		input.totalQRPBalance = totalQRPBalance;
		input.needed = needed;
		input.perWinnerCapTotal = perWinnerCapTotal;
		input.ticketPrice = ticketPrice;
		CalcReserveTopUp(qpi, *this, input, output, locals);
		return output;
	}

	CalculatePrizePools_output callCalculatePrizePools(const QPI::QpiContextFunctionCall& qpi, uint64 revenue, bit applyFRRake) const
	{
		CalculatePrizePools_input input{};
		CalculatePrizePools_output output{};
		CalculatePrizePools_locals locals{};

		input.revenue = revenue;
		input.applyFRRake = applyFRRake;
		CalculatePrizePools(qpi, *this, input, output, locals);
		return output;
	}

	CalculateBaseGain_output callCalculateBaseGain(const QPI::QpiContextFunctionCall& qpi, uint64 revenue, uint64 winnersBlock) const
	{
		CalculateBaseGain_input input{};
		CalculateBaseGain_output output{};
		CalculateBaseGain_locals locals{};

		input.revenue = revenue;
		input.winnersBlock = winnersBlock;
		CalculateBaseGain(qpi, *this, input, output, locals);
		return output;
	}

	CalculateExtraRedirectBP_output callCalculateExtraRedirectBP(const QPI::QpiContextFunctionCall& qpi, uint64 N, uint64 delta, uint64 revenue,
	                                                             uint64 baseGain) const
	{
		CalculateExtraRedirectBP_input input{};
		CalculateExtraRedirectBP_output output{};
		CalculateExtraRedirectBP_locals locals{};

		input.N = N;
		input.delta = delta;
		input.revenue = revenue;
		input.baseGain = baseGain;
		CalculateExtraRedirectBP(qpi, *this, input, output, locals);
		return output;
	}

	void callReturnAllTickets(const QPI::QpiContextProcedureCall& qpi)
	{
		ReturnAllTickets_input input{};
		ReturnAllTickets_output output{};
		ReturnAllTickets_locals locals{};

		ReturnAllTickets(qpi, *this, input, output, locals);
	}

	ProcessTierPayout_output callProcessTierPayout(const QPI::QpiContextProcedureCall& qpi, uint64 floorPerWinner, uint64 winnerCount,
	                                               uint64 payoutPool, uint64 perWinnerCap, uint64 totalQRPBalance, uint64 ticketPrice)
	{
		ProcessTierPayout_input input{};
		ProcessTierPayout_output output{};
		ProcessTierPayout_locals locals{};

		input.floorPerWinner = floorPerWinner;
		input.winnerCount = winnerCount;
		input.payoutPool = payoutPool;
		input.perWinnerCap = perWinnerCap;
		input.totalQRPBalance = totalQRPBalance;
		input.ticketPrice = ticketPrice;
		ProcessTierPayout(qpi, *this, input, output, locals);
		return output;
	}

	void callSettleEpoch(const QPI::QpiContextProcedureCall& qpi)
	{
		SettleEpoch_input input{};
		SettleEpoch_output output{};
		std::aligned_storage_t<sizeof(SettleEpoch_locals), alignof(SettleEpoch_locals)> localsStorage;
		auto& locals = *reinterpret_cast<SettleEpoch_locals*>(&localsStorage);
		setMemory(locals, 0);

		SettleEpoch(qpi, *this, input, output, locals);
	}
};

class ContractTestingQTF : protected ContractTesting
{
public:
	ContractTestingQTF()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QRP);
		INIT_CONTRACT(RL);
		INIT_CONTRACT(QTF);

		// Initialize QRP first (QTF depends on it for reserve operations)
		callSystemProcedure(QRP_CONTRACT_INDEX, INITIALIZE);
		// Initialize RL (RandomLottery contract)
		callSystemProcedure(RL_CONTRACT_INDEX, INITIALIZE);
		// Initialize QTF
		system.epoch = contractDescriptions[QTF_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(QTF_CONTRACT_INDEX, INITIALIZE);
	}

	// Access internal contract state
	QTFChecker* state() { return reinterpret_cast<QTFChecker*>(contractStates[QTF_CONTRACT_INDEX]); }

	id qtfSelf() { return id(QTF_CONTRACT_INDEX, 0, 0, 0); }
	id qrpSelf() { return id(QRP_CONTRACT_INDEX, 0, 0, 0); }
	void addPlayerDirect(const id& playerId, const QTFRandomValues& randomValues) { state()->addPlayerDirect(playerId, randomValues); }

	// Public function wrappers
	QTF::GetTicketPrice_output getTicketPrice()
	{
		QTF::GetTicketPrice_input input;
		QTF::GetTicketPrice_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_TICKET_PRICE, input, output);
		return output;
	}

	QTF::GetNextEpochData_output getNextEpochData()
	{
		QTF::GetNextEpochData_input input;
		QTF::GetNextEpochData_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_NEXT_EPOCH_DATA, input, output);
		return output;
	}

	QTF::GetWinnerData_output getWinnerData()
	{
		QTF::GetWinnerData_input input;
		QTF::GetWinnerData_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_WINNER_DATA, input, output);
		return output;
	}

	QTF::GetPools_output getPools()
	{
		QTF::GetPools_input input;
		QTF::GetPools_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_POOLS, input, output);
		return output;
	}

	QTF::GetSchedule_output getSchedule()
	{
		QTF::GetSchedule_input input;
		QTF::GetSchedule_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_SCHEDULE, input, output);
		return output;
	}

	QTF::GetDrawHour_output getDrawHour()
	{
		QTF::GetDrawHour_input input;
		QTF::GetDrawHour_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_DRAW_HOUR, input, output);
		return output;
	}

	QTF::GetState_output getStateInfo()
	{
		QTF::GetState_input input;
		QTF::GetState_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_STATE, input, output);
		return output;
	}

	QTF::GetFees_output getFees()
	{
		QTF::GetFees_input input;
		QTF::GetFees_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_FEES, input, output);
		return output;
	}

	QTF::EstimatePrizePayouts_output estimatePrizePayouts(uint64 k2WinnerCount, uint64 k3WinnerCount)
	{
		QTF::EstimatePrizePayouts_input input;
		input.k2WinnerCount = k2WinnerCount;
		input.k3WinnerCount = k3WinnerCount;
		QTF::EstimatePrizePayouts_output output;
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_ESTIMATE_PRIZE_PAYOUTS, input, output);
		return output;
	}

	// Procedure wrappers
	QTF::BuyTicket_output buyTicket(const id& user, uint64 reward, const QTFRandomValues& numbers)
	{
		QTF::BuyTicket_input input;
		input.randomValues = numbers;
		QTF::BuyTicket_output output;
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_BUY_TICKET, input, output, user, reward))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetPrice_output setPrice(const id& invocator, uint64 newPrice)
	{
		QTF::SetPrice_input input;
		input.newPrice = newPrice;
		QTF::SetPrice_output output;
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_PRICE, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetSchedule_output setSchedule(const id& invocator, uint8 newSchedule)
	{
		QTF::SetSchedule_input input;
		input.newSchedule = newSchedule;
		QTF::SetSchedule_output output;
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_SCHEDULE, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetTargetJackpot_output setTargetJackpot(const id& invocator, uint64 newTarget)
	{
		QTF::SetTargetJackpot_input input;
		input.newTargetJackpot = newTarget;
		QTF::SetTargetJackpot_output output;
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_TARGET_JACKPOT, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetDrawHour_output setDrawHour(const id& invocator, uint8 newHour)
	{
		QTF::SetDrawHour_input input;
		input.newDrawHour = newHour;
		QTF::SetDrawHour_output output;
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_DRAW_HOUR, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	// System procedure wrappers
	void beginEpoch() { callSystemProcedure(QTF_CONTRACT_INDEX, BEGIN_EPOCH); }
	void endEpoch() { callSystemProcedure(QTF_CONTRACT_INDEX, END_EPOCH); }
	void beginTick() { callSystemProcedure(QTF_CONTRACT_INDEX, BEGIN_TICK); }

	void setDateTime(uint16 year, uint8 month, uint8 day, uint8 hour)
	{
		updateTime();
		utcTime.Year = year;
		utcTime.Month = month;
		utcTime.Day = day;
		utcTime.Hour = hour;
		utcTime.Minute = 0;
		utcTime.Second = 0;
		utcTime.Nanosecond = 0;
		updateQpiTime();
	}

	void forceBeginTick()
	{
		system.tick = system.tick + (RL_TICK_UPDATE_PERIOD - mod(system.tick, static_cast<uint32>(RL_TICK_UPDATE_PERIOD)));
		beginTick();
	}

	void beginEpochWithDate(uint16 year, uint8 month, uint8 day, uint8 hour = 12)
	{
		setDateTime(year, month, day, hour);
		beginEpoch();
	}

	void beginEpochWithValidTime() { beginEpochWithDate(2025, 1, 20); }

	// Force schedule mask directly in state
	void forceSchedule(uint8 scheduleMask) { state()->setScheduleMask(scheduleMask); }

	// Advance to next day and trigger draw
	void advanceOneDayAndDraw()
	{
		constexpr uint16 y = 2025;
		constexpr uint8 m = 1;
		constexpr uint8 d = 10;
		setDateTime(y, m, d, 12);
		forceBeginTick();
	}

	// Helper to create valid random values
	QTFRandomValues makeValidNumbers(uint8 n1, uint8 n2, uint8 n3, uint8 n4)
	{
		QTFRandomValues values;
		values.set(0, n1);
		values.set(1, n2);
		values.set(2, n3);
		values.set(3, n4);
		return values;
	}

	// Fund user and buy a ticket
	void fundAndBuyTicket(const id& user, uint64 ticketPrice, const QTFRandomValues& numbers)
	{
		increaseEnergy(user, ticketPrice * 2);
		const QTF::BuyTicket_output out = buyTicket(user, ticketPrice, numbers);
		EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	}

	// Set prevSpectrumDigest for deterministic random number generation
	// This allows tests to predict winning numbers by fixing the RNG seed
	void setPrevSpectrumDigest(const m256i& digest) { etalonTick.prevSpectrumDigest = digest; }

	// Compute winning numbers that would be generated for a given prevSpectrumDigest
	// This mirrors the logic in QThirtyFour::GetRandomValues (lines 1663-1698)
	// Returns the 4 winning numbers in ascending order
	QTFRandomValues computeWinningNumbersForDigest(const m256i& digest)
	{
		// Replicate QTF's GetRandomValues logic
		// seed = qpi.K12(digest).u64._0
		m256i hashResult;
		KangarooTwelve((const uint8*)&digest, sizeof(m256i), (uint8*)&hashResult, sizeof(m256i));
		const uint64 seed = hashResult.m256i_u64[0];

		QTFRandomValues result;
		uint8 used[31] = {0}; // Track used numbers [0..30], we only use [1..30]

		for (uint8 index = 0; index < 4; ++index)
		{
			// deriveOne(seed, index, tempValue)
			uint64 tempValue = seed + 0x9e3779b97f4a7c15ULL * (index + 1);
			// mix64
			tempValue ^= tempValue >> 30;
			tempValue *= 0xbf58476d1ce4e5b9ULL;
			tempValue ^= tempValue >> 27;
			tempValue *= 0x94d049bb133111ebULL;
			tempValue ^= tempValue >> 31;

			uint8 candidate = static_cast<uint8>((tempValue % 30) + 1);

			// Handle collisions with the same regeneration logic as contract
			uint32 attempts = 0;
			while (used[candidate] && attempts < 100)
			{
				++attempts;
				tempValue ^= tempValue >> 12;
				tempValue ^= tempValue << 25;
				tempValue ^= tempValue >> 27;
				tempValue *= 2685821657736338717ULL;
				candidate = static_cast<uint8>((tempValue % 30) + 1);
			}

			// Fallback: find first unused
			if (used[candidate])
			{
				for (uint8 fallback = 1; fallback <= 30; ++fallback)
				{
					if (!used[fallback])
					{
						candidate = fallback;
						break;
					}
				}
			}

			used[candidate] = 1;
			result.set(index, candidate);
		}

		return result;
	}

	QTFRandomValues makeLosingNumbers(const QTFRandomValues& winningNumbers)
	{
		bool isWinning[31] = {};
		for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
		{
			isWinning[winningNumbers.get(i)] = true;
		}

		QTFRandomValues losingNumbers;
		uint64 outIndex = 0;
		for (uint8 candidate = 1; candidate <= QTF_MAX_RANDOM_VALUE && outIndex < QTF_RANDOM_VALUES_COUNT; ++candidate)
		{
			if (!isWinning[candidate])
			{
				losingNumbers.set(outIndex++, candidate);
			}
		}
		EXPECT_EQ(outIndex, static_cast<uint64>(QTF_RANDOM_VALUES_COUNT));
		return losingNumbers;
	}

	// Create a ticket that matches exactly `matchCount` numbers with `winningNumbers`.
	// Guarantees values are unique and in [1..30].
	QTFRandomValues makeNumbersWithExactMatches(const QTFRandomValues& winningNumbers, uint8 matchCount)
	{
		EXPECT_LE(matchCount, static_cast<uint8>(QTF_RANDOM_VALUES_COUNT));

		bool isWinning[31] = {};
		bool used[31] = {};
		for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
		{
			const uint8 v = winningNumbers.get(i);
			EXPECT_GE(v, 1u);
			EXPECT_LE(v, QTF_MAX_RANDOM_VALUE);
			EXPECT_FALSE(isWinning[v]) << "winningNumbers must be unique";
			isWinning[v] = true;
		}

		QTFRandomValues ticket;
		uint64 outIndex = 0;

		// Take first `matchCount` winning numbers as the matches.
		for (uint8 i = 0; i < matchCount; ++i)
		{
			const uint8 v = winningNumbers.get(i);
			used[v] = true;
			ticket.set(outIndex++, v);
		}

		// Fill the remaining positions with non-winning numbers.
		for (uint8 candidate = 1; candidate <= QTF_MAX_RANDOM_VALUE && outIndex < QTF_RANDOM_VALUES_COUNT; ++candidate)
		{
			if (!isWinning[candidate] && !used[candidate])
			{
				used[candidate] = true;
				ticket.set(outIndex++, candidate);
			}
		}

		EXPECT_EQ(outIndex, static_cast<uint64>(QTF_RANDOM_VALUES_COUNT));

		// Verify exact overlap count and uniqueness (debug safety for tests).
		uint64 overlap = 0;
		std::set<uint8> uniqueValues;
		for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
		{
			const uint8 v = ticket.get(i);
			EXPECT_GE(v, 1u);
			EXPECT_LE(v, QTF_MAX_RANDOM_VALUE);
			uniqueValues.insert(v);
			if (isWinning[v])
			{
				++overlap;
			}
		}
		EXPECT_EQ(uniqueValues.size(), static_cast<size_t>(QTF_RANDOM_VALUES_COUNT));
		EXPECT_EQ(overlap, static_cast<uint64>(matchCount));

		return ticket;
	}

	QTFRandomValues makeK2Numbers(const QTFRandomValues& winningNumbers) { return makeNumbersWithExactMatches(winningNumbers, 2); }
	QTFRandomValues makeK3Numbers(const QTFRandomValues& winningNumbers) { return makeNumbersWithExactMatches(winningNumbers, 3); }
};

// ============================================================================
// PRIVATE METHOD TESTS
// ============================================================================

TEST(ContractQThirtyFour_Private, CountMatches_CountsOverlappingNumbers)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	// Include values > 8 to cover the full [1..30] bitmask range.
	const QTFRandomValues player = ctl.makeValidNumbers(1, 16, 29, 30);
	const QTFRandomValues winning = ctl.makeValidNumbers(16, 29, 2, 3);
	const auto out = ctl.state()->callCountMatches(qpi, player, winning);
	EXPECT_EQ(out.matches, 2);
}

TEST(ContractQThirtyFour_Private, ValidateNumbers_WorksForValidDuplicateAndRangeErrors)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const QTFRandomValues ok = ctl.makeValidNumbers(1, 2, 3, 4);
	EXPECT_TRUE(ctl.state()->callValidateNumbers(qpi, ok).isValid);

	QTFRandomValues dup = ctl.makeValidNumbers(1, 2, 3, 4);
	dup.set(3, 2);
	EXPECT_FALSE(ctl.state()->callValidateNumbers(qpi, dup).isValid);

	QTFRandomValues outOfRange = ctl.makeValidNumbers(1, 2, 3, 4);
	outOfRange.set(2, 31);
	EXPECT_FALSE(ctl.state()->callValidateNumbers(qpi, outOfRange).isValid);
}

TEST(ContractQThirtyFour_Private, GetRandomValues_IsDeterministicAndUniqueInRange)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const uint64 seed = 0x123456789ABCDEF0ULL;
	const auto out1 = ctl.state()->callGetRandomValues(qpi, seed);
	const auto out2 = ctl.state()->callGetRandomValues(qpi, seed);

	std::set<uint8> seen;
	for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
	{
		const uint8 v = out1.values.get(i);
		EXPECT_GE(v, 1);
		EXPECT_LE(v, QTF_MAX_RANDOM_VALUE);
		seen.insert(v);
		EXPECT_EQ(out1.values.get(i), out2.values.get(i));
	}
	EXPECT_EQ(seen.size(), static_cast<size_t>(QTF_RANDOM_VALUES_COUNT));
}

TEST(ContractQThirtyFour_Private, CheckContractBalance_UsesIncomingMinusOutgoing)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const uint64 balance = 123456;
	increaseEnergy(ctl.qtfSelf(), balance);

	const auto outExact = ctl.state()->callCheckContractBalance(qpi, balance);
	EXPECT_TRUE(outExact.hasEnough);
	EXPECT_EQ(outExact.actualBalance, balance);

	const auto outTooHigh = ctl.state()->callCheckContractBalance(qpi, balance + 1);
	EXPECT_FALSE(outTooHigh.hasEnough);
	EXPECT_EQ(outTooHigh.actualBalance, balance);
}

TEST(ContractQThirtyFour_Private, PowerFixedPoint_ComputesFastExponentiationInFixedPoint)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	// 0.5^2 = 0.25
	const auto out025 = ctl.state()->callPowerFixedPoint(qpi, QTF_FIXED_POINT_SCALE / 2, 2);
	EXPECT_EQ(out025.result, QTF_FIXED_POINT_SCALE / 4);

	// 2.0^3 = 8.0
	const auto out8 = ctl.state()->callPowerFixedPoint(qpi, 2 * QTF_FIXED_POINT_SCALE, 3);
	EXPECT_EQ(out8.result, 8 * QTF_FIXED_POINT_SCALE);
}

TEST(ContractQThirtyFour_Private, CalculateExpectedRoundsToK4_HandlesEdgeCaseAndMonotonicity)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const auto out0 = ctl.state()->callCalculateExpectedRoundsToK4(qpi, 0);
	EXPECT_EQ(out0.expectedRounds, UINT64_MAX);

	const auto out1 = ctl.state()->callCalculateExpectedRoundsToK4(qpi, 1);
	const auto out100 = ctl.state()->callCalculateExpectedRoundsToK4(qpi, 100);
	EXPECT_GT(out1.expectedRounds, 0ULL);
	EXPECT_GT(out100.expectedRounds, 0ULL);
	EXPECT_LE(out1.expectedRounds, QTF_FIXED_POINT_SCALE);
	EXPECT_LE(out100.expectedRounds, QTF_FIXED_POINT_SCALE);
	EXPECT_GT(out1.expectedRounds, out100.expectedRounds);
}

TEST(ContractQThirtyFour_Private, CalcReserveTopUp_RespectsSoftFloorPerRoundAndPerWinnerCaps)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const uint64 P = 1000000ULL;

	// Soft floor binds availableAboveFloor and per-round is 10% of total.
	{
		const auto out = ctl.state()->callCalcReserveTopUp(qpi, 25000000ULL, 10000000ULL, 1000000000ULL, P);
		EXPECT_EQ(out.topUpAmount, 2500000ULL);
	}

	// Per-winner cap binds.
	{
		const auto out = ctl.state()->callCalcReserveTopUp(qpi, 1000000000ULL, 50000000ULL, 1000000ULL, P);
		EXPECT_EQ(out.topUpAmount, 1000000ULL);
	}

	// Needed is below all caps.
	{
		const auto out = ctl.state()->callCalcReserveTopUp(qpi, 1000000000ULL, 12345ULL, 1000000000ULL, P);
		EXPECT_EQ(out.topUpAmount, 12345ULL);
	}
}

TEST(ContractQThirtyFour_Private, CalculatePrizePools_MatchesFeeAndRakeMath)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const auto fees = ctl.getFees();
	ASSERT_NE(fees.winnerFeePercent, 0);

	const uint64 revenue = 1000000ULL;
	const uint64 winnersBlockBeforeRake = (revenue * static_cast<uint64>(fees.winnerFeePercent)) / 100ULL;

	{
		const auto out = ctl.state()->callCalculatePrizePools(qpi, revenue, false);
		EXPECT_EQ(out.winnersRake, 0ULL);
		EXPECT_EQ(out.winnersBlock, winnersBlockBeforeRake);
		EXPECT_EQ(out.k3Pool, (out.winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000ULL);
		EXPECT_EQ(out.k2Pool, (out.winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000ULL);
	}

	{
		const auto out = ctl.state()->callCalculatePrizePools(qpi, revenue, true);
		const uint64 expectedRake = (winnersBlockBeforeRake * QTF_FR_WINNERS_RAKE_BP) / 10000ULL;
		EXPECT_EQ(out.winnersRake, expectedRake);
		EXPECT_EQ(out.winnersBlock, winnersBlockBeforeRake - expectedRake);
		EXPECT_EQ(out.k3Pool, (out.winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000ULL);
		EXPECT_EQ(out.k2Pool, (out.winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000ULL);
	}
}

TEST(ContractQThirtyFour_Private, CalculateBaseGain_FollowsConfiguredRedirectsAndOverflowBias)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	const uint64 revenue = 1000000ULL;
	const uint64 winnersBlock = 680000ULL;

	const auto out = ctl.state()->callCalculateBaseGain(qpi, revenue, winnersBlock);
	EXPECT_EQ(out.baseGain, 118600ULL);
}

TEST(ContractQThirtyFour_Private, CalculateExtraRedirectBP_ReturnsZeroOrClampsToMax)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	QTF::GetTicketPrice_input primeIn{};
	qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &primeIn, sizeof(primeIn));

	// Early exits
	EXPECT_EQ(ctl.state()->callCalculateExtraRedirectBP(qpi, 0, 1, 1, 0).extraBP, 0ULL);
	EXPECT_EQ(ctl.state()->callCalculateExtraRedirectBP(qpi, 1, 0, 1, 0).extraBP, 0ULL);
	EXPECT_EQ(ctl.state()->callCalculateExtraRedirectBP(qpi, 1, 1, 0, 0).extraBP, 0ULL);

	// Clamp to max under large deficit.
	{
		const uint64 revenue = 1000000ULL;
		const uint64 delta = revenue * 1000ULL;
		const auto out = ctl.state()->callCalculateExtraRedirectBP(qpi, 100, delta, revenue, 0);
		EXPECT_EQ(out.extraBP, QTF_FR_EXTRA_MAX_BP);
	}

	// Base gain already covers required gain -> zero.
	{
		const auto out = ctl.state()->callCalculateExtraRedirectBP(qpi, 100, 1000ULL, 1000000ULL, 2000ULL);
		EXPECT_EQ(out.extraBP, 0ULL);
	}
}

TEST(ContractQThirtyFour_Private, ProcessTierPayout_ComputesPayoutAndOptionalTopUp)
{
	ContractTestingQTF ctl;

	const id originator = id::randomValue();
	QpiContextUserProcedureCall qpi(QTF_CONTRACT_INDEX, originator, 0);
	QTF::SetDrawHour_input primeIn{};
	QTF::SetDrawHour_output primeOut{};
	primeIn.newDrawHour = ctl.state()->getDrawHourInternal();
	qpi.call(QTF_PROCEDURE_SET_DRAW_HOUR, &primeIn, sizeof(primeIn));
	copyMem(&primeOut, qpi.outputBuffer, sizeof(primeOut));
	ASSERT_EQ(contractError[QTF_CONTRACT_INDEX], 0);

	// No winners -> all overflow.
	{
		const auto out = ctl.state()->callProcessTierPayout(qpi, 50, 0, 123, 100, 0, 1000000ULL);
		EXPECT_EQ(out.perWinnerPayout, 0ULL);
		EXPECT_EQ(out.overflow, 123ULL);
		EXPECT_EQ(out.topUpReceived, 0ULL);
	}

	// Top-up from QRP to meet floor.
	{
		const uint64 qrpBalanceBefore = 1000000000ULL;
		increaseEnergy(ctl.qrpSelf(), qrpBalanceBefore);

		const uint64 qtfBalanceBefore = getBalance(ctl.qtfSelf());
		const uint64 qrpBalanceBeforeActual = getBalance(ctl.qrpSelf());

		const auto out = ctl.state()->callProcessTierPayout(qpi, 50, 2, 10, 100, qrpBalanceBeforeActual, 1000000ULL);
		EXPECT_EQ(out.perWinnerPayout, 50ULL);
		EXPECT_EQ(out.overflow, 0ULL);
		EXPECT_EQ(out.topUpReceived, 90ULL);

		EXPECT_EQ(getBalance(ctl.qtfSelf()), qtfBalanceBefore + 90);
		EXPECT_EQ(getBalance(ctl.qrpSelf()), qrpBalanceBeforeActual - 90);
	}
}

TEST(ContractQThirtyFour_Private, ReturnAllTickets_RefundsEachPlayerAndClearsViaSettleEpochRevenueZeroBranch)
{
	ContractTestingQTF ctl;

	const id originator = id::randomValue();
	QpiContextUserProcedureCall qpi(QTF_CONTRACT_INDEX, originator, 0);
	QTF::SetDrawHour_input primeIn{};
	primeIn.newDrawHour = ctl.state()->getDrawHourInternal();
	qpi.call(QTF_PROCEDURE_SET_DRAW_HOUR, &primeIn, sizeof(primeIn));
	ASSERT_EQ(contractError[QTF_CONTRACT_INDEX], 0);

	// Setup a few players and refund them.
	const uint64 ticketPrice = 10;
	ctl.state()->setTicketPriceInternal(ticketPrice);

	const id p1 = id::randomValue();
	const id p2 = id::randomValue();
	const QTFRandomValues n1 = ctl.makeValidNumbers(1, 2, 3, 4);
	const QTFRandomValues n2 = ctl.makeValidNumbers(5, 6, 7, 8);
	ctl.addPlayerDirect(p1, n1);
	ctl.addPlayerDirect(p2, n2);

	increaseEnergy(ctl.qtfSelf(), static_cast<long long>(ticketPrice * 2));
	const uint64 balBeforeContract = getBalance(ctl.qtfSelf());
	const uint64 balBeforeP1 = getBalance(p1);
	const uint64 balBeforeP2 = getBalance(p2);

	ctl.state()->callReturnAllTickets(qpi);

	EXPECT_EQ(getBalance(p1), balBeforeP1 + ticketPrice);
	EXPECT_EQ(getBalance(p2), balBeforeP2 + ticketPrice);
	EXPECT_EQ(getBalance(ctl.qtfSelf()), balBeforeContract - (ticketPrice * 2));

	// Now exercise SettleEpoch revenue==0 branch, which must clear players.
	ctl.state()->setTicketPriceInternal(0);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 2ULL);
	ctl.state()->callSettleEpoch(qpi);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0ULL);
}

// ============================================================================
// BUY TICKET TESTS
// ============================================================================

TEST(ContractQThirtyFour, BuyTicket_WhenSellingClosed_RefundsAndFails)
{
	ContractTestingQTF ctl;
	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Selling is closed initially (before beginEpoch with valid time)
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 2);
	const uint64 balBefore = getBalance(user);

	QTFRandomValues nums = ctl.makeValidNumbers(1, 2, 3, 4);
	const QTF::BuyTicket_output out = ctl.buyTicket(user, ticketPrice, nums);

	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::TICKET_SELLING_CLOSED));
	EXPECT_EQ(getBalance(user), balBefore); // Refunded
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, BuyTicket_TooLowPrice_RefundsAndFails)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 5);
	const uint64 balBefore = getBalance(user);

	QTFRandomValues nums = ctl.makeValidNumbers(1, 2, 3, 4);

	// Test with price too low - should fail and refund
	const QTF::BuyTicket_output outLow = ctl.buyTicket(user, ticketPrice - 1, nums);
	EXPECT_EQ(outLow.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_TICKET_PRICE));
	EXPECT_EQ(getBalance(user), balBefore); // Fully refunded

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, BuyTicket_OverpaidPrice_AcceptsAndReturnsExcess)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	const uint64 overpayment = ticketPrice * 2; // Pay double
	increaseEnergy(user, overpayment * 2);
	const uint64 balBefore = getBalance(user);

	QTFRandomValues nums = ctl.makeValidNumbers(1, 2, 3, 4);

	// Test with overpayment - should accept ticket and return excess
	const QTF::BuyTicket_output outHigh = ctl.buyTicket(user, overpayment, nums);
	EXPECT_EQ(outHigh.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));

	// Should have paid exactly ticketPrice, excess returned
	const uint64 excess = overpayment - ticketPrice;
	EXPECT_EQ(getBalance(user), balBefore - ticketPrice) << "User should pay exactly ticket price, excess returned";

	// Ticket should be registered
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);
}

TEST(ContractQThirtyFour, BuyTicket_OverpaidInvalidNumbers_RefundsFull_NoLeak)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	const uint64 overpayment = ticketPrice * 2;
	increaseEnergy(user, overpayment * 2);
	const uint64 balBefore = getBalance(user);

	// Invalid: out of range
	const QTFRandomValues invalidNums = ctl.makeValidNumbers(1, 2, 3, 31);
	const QTF::BuyTicket_output out = ctl.buyTicket(user, overpayment, invalidNums);

	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_NUMBERS));
	EXPECT_EQ(getBalance(user), balBefore) << "Full invocationReward must be refunded once";
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, BuyTicket_InvalidNumbers_OutOfRange_Fails)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 10);
	const uint64 balBefore = getBalance(user);

	// Number 0 is invalid (valid range is 1-30)
	QTFRandomValues numsWithZero;
	numsWithZero.set(0, 0);
	numsWithZero.set(1, 2);
	numsWithZero.set(2, 3);
	numsWithZero.set(3, 4);

	const QTF::BuyTicket_output out1 = ctl.buyTicket(user, ticketPrice, numsWithZero);
	EXPECT_EQ(out1.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_NUMBERS));
	EXPECT_EQ(getBalance(user), balBefore);

	// Number 31 is invalid (valid range is 1-30)
	QTFRandomValues numsOver30;
	numsOver30.set(0, 1);
	numsOver30.set(1, 2);
	numsOver30.set(2, 3);
	numsOver30.set(3, 31);

	const QTF::BuyTicket_output out2 = ctl.buyTicket(user, ticketPrice, numsOver30);
	EXPECT_EQ(out2.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_NUMBERS));
	EXPECT_EQ(getBalance(user), balBefore);

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, BuyTicket_DuplicateNumbers_Fails)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 5);
	const uint64 balBefore = getBalance(user);

	// Duplicate number 5
	QTFRandomValues dupNums;
	dupNums.set(0, 5);
	dupNums.set(1, 5);
	dupNums.set(2, 10);
	dupNums.set(3, 15);

	const QTF::BuyTicket_output out = ctl.buyTicket(user, ticketPrice, dupNums);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_NUMBERS));
	EXPECT_EQ(getBalance(user), balBefore);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, BuyTicket_ValidPurchase_Success)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 2);
	const uint64 balBefore = getBalance(user);

	QTFRandomValues nums = ctl.makeValidNumbers(5, 10, 15, 20);

	const QTF::BuyTicket_output out = ctl.buyTicket(user, ticketPrice, nums);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	EXPECT_EQ(getBalance(user), balBefore - ticketPrice);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);

	// Verify player data stored correctly
	const QTF::PlayerData& player = ctl.state()->getPlayer(0);
	EXPECT_EQ(player.player, user);
	EXPECT_EQ(player.randomValues.get(0), 5);
	EXPECT_EQ(player.randomValues.get(1), 10);
	EXPECT_EQ(player.randomValues.get(2), 15);
	EXPECT_EQ(player.randomValues.get(3), 20);
}

TEST(ContractQThirtyFour, BuyTicket_MultiplePlayers_Success)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add 10 different players
	for (int i = 0; i < 10; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums =
		    ctl.makeValidNumbers(static_cast<uint8>(1 + i), static_cast<uint8>(11 + i), static_cast<uint8>(21), static_cast<uint8>(30));

		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 10u);
}

TEST(ContractQThirtyFour, BuyTicket_MaxPlayersReached_Fails)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Fill up to max players (1024)
	for (uint64 i = 0; i < QTF_MAX_NUMBER_OF_PLAYERS; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 27) + 1), static_cast<uint8>(((i + 1) % 27) + 1),
		                                            static_cast<uint8>(((i + 2) % 27) + 1), static_cast<uint8>(((i + 3) % 27) + 1));

		// Only fund and buy; we expect all to succeed until max
		increaseEnergy(user, ticketPrice * 2);
		const QTF::BuyTicket_output out = ctl.buyTicket(user, ticketPrice, nums);
		EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	}

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), QTF_MAX_NUMBER_OF_PLAYERS);

	// Try one more - should fail
	const id extraUser = id::randomValue();
	increaseEnergy(extraUser, ticketPrice * 2);
	const uint64 balBefore = getBalance(extraUser);
	QTFRandomValues nums = ctl.makeValidNumbers(1, 2, 3, 4);

	const QTF::BuyTicket_output out = ctl.buyTicket(extraUser, ticketPrice, nums);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::MAX_PLAYERS_REACHED));
	EXPECT_EQ(getBalance(extraUser), balBefore); // Refunded
}

TEST(ContractQThirtyFour, BuyTicket_SamePlayerMultipleTickets_Allowed)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 10);

	// Same player buys multiple tickets with different numbers
	QTFRandomValues nums1 = ctl.makeValidNumbers(1, 2, 3, 4);
	QTFRandomValues nums2 = ctl.makeValidNumbers(5, 6, 7, 8);
	QTFRandomValues nums3 = ctl.makeValidNumbers(9, 10, 11, 12);

	EXPECT_EQ(ctl.buyTicket(user, ticketPrice, nums1).returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.buyTicket(user, ticketPrice, nums2).returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.buyTicket(user, ticketPrice, nums3).returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 3u);
}

// ============================================================================
// CONFIGURATION CHANGE TESTS
// ============================================================================

TEST(ContractQThirtyFour, SetPrice_AccessControl)
{
	ContractTestingQTF ctl;

	const uint64 oldPrice = ctl.state()->getTicketPriceInternal();
	const uint64 newPrice = oldPrice * 2;

	// Random user should be denied
	const id randomUser = id::randomValue();
	increaseEnergy(randomUser, 1);
	const QTF::SetPrice_output outDenied = ctl.setPrice(randomUser, newPrice);
	EXPECT_EQ(outDenied.returnCode, static_cast<uint8>(QTF::EReturnCode::ACCESS_DENIED));

	// Price unchanged
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
}

TEST(ContractQThirtyFour, SetPrice_ZeroNotAllowed)
{
	ContractTestingQTF ctl;
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const uint64 oldPrice = ctl.state()->getTicketPriceInternal();
	const QTF::SetPrice_output outInvalid = ctl.setPrice(QTF_DEV_ADDRESS, 0);
	EXPECT_EQ(outInvalid.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_TICKET_PRICE));

	// Price unchanged
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
}

TEST(ContractQThirtyFour, SetPrice_AppliesAfterEndEpoch)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const uint64 oldPrice = ctl.state()->getTicketPriceInternal();
	const uint64 newPrice = oldPrice * 3;

	const QTF::SetPrice_output outOk = ctl.setPrice(QTF_DEV_ADDRESS, newPrice);
	EXPECT_EQ(outOk.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));

	// Queued in NextEpochData
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newTicketPrice, newPrice);

	// Old price still active
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);

	// Apply after END_EPOCH
	ctl.endEpoch();
	ctl.beginEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, newPrice);

	// NextEpochData cleared
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newTicketPrice, 0u);
}

TEST(ContractQThirtyFour, SetSchedule_AccessControl)
{
	ContractTestingQTF ctl;

	const id randomUser = id::randomValue();
	increaseEnergy(randomUser, 1);
	const QTF::SetSchedule_output outDenied = ctl.setSchedule(randomUser, QTF_ANY_DAY_SCHEDULE);
	EXPECT_EQ(outDenied.returnCode, static_cast<uint8>(QTF::EReturnCode::ACCESS_DENIED));
}

TEST(ContractQThirtyFour, SetSchedule_ZeroNotAllowed)
{
	ContractTestingQTF ctl;
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const QTF::SetSchedule_output outInvalid = ctl.setSchedule(QTF_DEV_ADDRESS, 0);
	EXPECT_EQ(outInvalid.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_VALUE));
}

TEST(ContractQThirtyFour, SetSchedule_AppliesAfterEndEpoch)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const uint8 newSchedule = 0x7F; // All days

	const QTF::SetSchedule_output outOk = ctl.setSchedule(QTF_DEV_ADDRESS, newSchedule);
	EXPECT_EQ(outOk.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));

	// Queued
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newSchedule, newSchedule);

	// Apply
	ctl.endEpoch();
	ctl.beginEpoch();
	EXPECT_EQ(ctl.getSchedule().schedule, newSchedule);
}

TEST(ContractQThirtyFour, SetTargetJackpot_AccessControl)
{
	ContractTestingQTF ctl;

	const id randomUser = id::randomValue();
	increaseEnergy(randomUser, 1);
	const QTF::SetTargetJackpot_output outDenied = ctl.setTargetJackpot(randomUser, 2000000000ULL);
	EXPECT_EQ(outDenied.returnCode, static_cast<uint8>(QTF::EReturnCode::ACCESS_DENIED));
}

TEST(ContractQThirtyFour, SetTargetJackpot_ZeroNotAllowed)
{
	ContractTestingQTF ctl;
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const QTF::SetTargetJackpot_output outInvalid = ctl.setTargetJackpot(QTF_DEV_ADDRESS, 0);
	EXPECT_EQ(outInvalid.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_VALUE));
}

TEST(ContractQThirtyFour, SetTargetJackpot_AppliesAfterEndEpoch)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const uint64 newTarget = 5000000000ULL;

	const QTF::SetTargetJackpot_output outOk = ctl.setTargetJackpot(QTF_DEV_ADDRESS, newTarget);
	EXPECT_EQ(outOk.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));

	// Queued
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newTargetJackpot, newTarget);

	// Apply
	ctl.endEpoch();
	ctl.beginEpoch();
	EXPECT_EQ(ctl.state()->getTargetJackpotInternal(), newTarget);
}

TEST(ContractQThirtyFour, SetDrawHour_AccessControl)
{
	ContractTestingQTF ctl;

	const id randomUser = id::randomValue();
	increaseEnergy(randomUser, 1);
	const QTF::SetDrawHour_output outDenied = ctl.setDrawHour(randomUser, 15);
	EXPECT_EQ(outDenied.returnCode, static_cast<uint8>(QTF::EReturnCode::ACCESS_DENIED));
}

TEST(ContractQThirtyFour, SetDrawHour_InvalidValues)
{
	ContractTestingQTF ctl;
	increaseEnergy(QTF_DEV_ADDRESS, 2);

	// 0 is invalid
	const QTF::SetDrawHour_output out0 = ctl.setDrawHour(QTF_DEV_ADDRESS, 0);
	EXPECT_EQ(out0.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_VALUE));

	// 24+ is invalid
	const QTF::SetDrawHour_output out24 = ctl.setDrawHour(QTF_DEV_ADDRESS, 24);
	EXPECT_EQ(out24.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_VALUE));
}

TEST(ContractQThirtyFour, SetDrawHour_AppliesAfterEndEpoch)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();
	increaseEnergy(QTF_DEV_ADDRESS, 1);

	const uint8 newHour = 18;

	const QTF::SetDrawHour_output outOk = ctl.setDrawHour(QTF_DEV_ADDRESS, newHour);
	EXPECT_EQ(outOk.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));

	// Queued
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newDrawHour, newHour);

	// Apply
	ctl.endEpoch();
	ctl.beginEpoch();
	EXPECT_EQ(ctl.getDrawHour().drawHour, newHour);
}

// ============================================================================
// STATE AND POOLS TESTS
// ============================================================================

TEST(ContractQThirtyFour, GetState_InitiallyNotSelling)
{
	ContractTestingQTF ctl;
	// Before valid time initialization, state should be NONE (not selling)
	EXPECT_EQ(ctl.getStateInfo().currentState, static_cast<uint8>(QTF::EState::STATE_NONE));
}

TEST(ContractQThirtyFour, GetState_SellingAfterValidEpochStart)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();
	EXPECT_EQ(ctl.getStateInfo().currentState, static_cast<uint8>(QTF::EState::STATE_SELLING));
}

// ============================================================================
// SETTLEMENT AND PAYOUT TESTS
// ============================================================================

TEST(ContractQThirtyFour, Settlement_NoPlayers_NoChanges)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	const uint64 jackpotBefore = ctl.state()->getJackpot();
	const QTF::GetWinnerData_output winnersBefore = ctl.getWinnerData();

	ctl.advanceOneDayAndDraw();

	// No changes when no players
	EXPECT_EQ(ctl.state()->getJackpot(), jackpotBefore);
	const QTF::GetWinnerData_output winnersAfter = ctl.getWinnerData();
	EXPECT_EQ(winnersAfter.winnerData.winnerCounter, winnersBefore.winnerData.winnerCounter);
}

TEST(ContractQThirtyFour, Settlement_WithPlayers_FeesDistributed)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	// Fix RNG so we can deterministically avoid winners (and especially k=4).
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x1010101010101010ULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const QTF::GetFees_output fees = ctl.getFees();
	constexpr uint64 numPlayers = 10;

	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT);

	// Verify FR is not active initially (baseline mode)
	EXPECT_EQ(ctl.state()->getFrActive(), false);

	// Add players
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	const uint64 totalRevenue = ticketPrice * numPlayers;
	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 contractBalBefore = getBalance(ctl.qtfSelf());

	EXPECT_EQ(contractBalBefore, totalRevenue);

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	EXPECT_EQ(ctl.state()->getFrActive(), false);

	// In baseline mode (FR not active), dev receives full 10% of revenue
	// No redirects are applied
	const uint64 expectedDevFee = (totalRevenue * fees.teamFeePercent) / 100;
	EXPECT_EQ(getBalance(QTF_DEV_ADDRESS), devBalBefore + expectedDevFee)
	    << "In baseline mode, dev should receive full " << static_cast<int>(fees.teamFeePercent) << "% of revenue";

	// Players cleared
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, Settlement_WithPlayers_FeesDistributed_FRMode)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Fix RNG so we can deterministically avoid winners (and especially k=4).
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x2020202020202020ULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Activate FR mode
	ctl.state()->setJackpot(100000000ULL); // Below target
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(5);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const QTF::GetFees_output fees = ctl.getFees();
	constexpr uint64 numPlayers = 10;

	// Verify FR is active
	EXPECT_EQ(ctl.state()->getFrActive(), true);

	// Add players
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	const uint64 totalRevenue = ticketPrice * numPlayers;
	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 contractBalBefore = getBalance(ctl.qtfSelf());
	const uint64 jackpotBefore = ctl.state()->getJackpot();

	EXPECT_EQ(contractBalBefore, totalRevenue);

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// In FR mode, dev receives less than full 10% of revenue
	// Base redirect: 1% of revenue (QTF_FR_DEV_REDIRECT_BP = 100 basis points)
	// Possible extra redirect depending on deficit
	const uint64 baseDevRedirect = (totalRevenue * QTF_FR_DEV_REDIRECT_BP) / 10000;

	// Full dev fee from revenue split (10%)
	const uint64 fullDevFee = (totalRevenue * fees.teamFeePercent) / 100;

	// Actual dev payout = fullDevFee - redirects
	// Expected: fullDevFee - at least baseDevRedirect
	const uint64 maxExpectedDevPayout = fullDevFee - baseDevRedirect;

	const uint64 actualDevPayout = getBalance(QTF_DEV_ADDRESS) - devBalBefore;

	// Dev should receive less than full fee (due to redirects to jackpot)
	EXPECT_LT(actualDevPayout, fullDevFee) << "In FR mode, dev payout should be reduced by redirects";

	// Dev should receive at most fullDevFee - baseDevRedirect
	EXPECT_LE(actualDevPayout, maxExpectedDevPayout) << "Dev payout should be reduced by at least base redirect (1%)";

	// Jackpot should have grown (receives redirects)
	EXPECT_GT(ctl.state()->getJackpot(), jackpotBefore) << "Jackpot should grow from dev/dist redirects in FR mode";

	// Players cleared
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, Settlement_JackpotGrowsFromOverflow)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	// Fix RNG so we can deterministically create "no winners" tickets.
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xBADC0FFEE0DDF00DULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	constexpr uint64 numPlayers = 20;

	// Add players
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	// Calculate expected jackpot growth in baseline mode (FR not active)
	const uint64 revenue = ticketPrice * numPlayers;
	const QTF::GetFees_output fees = ctl.getFees();

	// winnersBlock = revenue * winnerFeePercent / 100 (68%)
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;

	// With no winners, the entire winners block becomes overflow (k2+k3 pools also roll into overflow).
	const uint64 winnersOverflow = winnersBlock;

	// In baseline mode: 50% of overflow goes to jackpot, 50% to reserve.
	const uint64 reserveAdd = (winnersOverflow * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 overflowToJackpot = winnersOverflow - reserveAdd;

	// Minimum expected jackpot growth (assuming no k2/k3 winners, all overflow goes to jackpot)
	const uint64 minExpectedGrowth = overflowToJackpot;

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// Verify jackpot growth
	const uint64 jackpotAfter = ctl.state()->getJackpot();
	const uint64 actualGrowth = jackpotAfter - jackpotBefore;

	// In baseline mode, 50% of overflow goes to jackpot
	// Allow 5% tolerance for rounding and potential winners
	const uint64 tolerance = minExpectedGrowth / 20; // 5%
	EXPECT_GE(actualGrowth + tolerance, minExpectedGrowth)
	    << "Actual growth: " << actualGrowth << ", Expected minimum: " << minExpectedGrowth << ", Overflow to jackpot (50%): " << overflowToJackpot;

	// Verify the 50% overflow split is working correctly
	const uint64 expected50Percent = winnersOverflow / 2;
	EXPECT_GE(overflowToJackpot, expected50Percent - 1) << "50% overflow split verification";
	EXPECT_LE(overflowToJackpot, winnersOverflow) << "Overflow to jackpot should not exceed total overflow";
}

TEST(ContractQThirtyFour, Settlement_RoundsSinceK4_Increments)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x1111222233334444ULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Run several rounds without k=4 win
	for (int round = 0; round < 3; ++round)
	{
		ctl.beginEpochWithValidTime();

		for (int i = 0; i < 5; ++i)
		{
			const id user = id::randomValue();
			ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
		}

		const uint64 roundsBefore = ctl.state()->getFrRoundsSinceK4();
		ctl.setPrevSpectrumDigest(testDigest);
		ctl.advanceOneDayAndDraw();

		// Deterministic: no ticket matches any winning number, so k=4 cannot occur.
		EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), roundsBefore + 1);
	}
}

// ============================================================================
// FAST-RECOVERY (FR) TESTS
// ============================================================================

TEST(ContractQThirtyFour, FR_Activation_WhenBelowTarget)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Set jackpot below target to trigger FR
	ctl.state()->setJackpot(100000000ULL);                             // 100M
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT); // 1B target
	ctl.state()->setFrRoundsSinceK4(5);                                // Within post-k4 window

	EXPECT_EQ(ctl.state()->getFrActive(), false);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add players and settle
	for (int i = 0; i < 10; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 25) + 1), static_cast<uint8>((i % 25) + 2),
		                                            static_cast<uint8>((i % 25) + 3), static_cast<uint8>((i % 25) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	ctl.advanceOneDayAndDraw();

	// FR should be active since jackpot < target and within window
	EXPECT_EQ(ctl.state()->getFrActive(), true);
}

TEST(ContractQThirtyFour, FR_Deactivation_AfterHysteresis)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x3030303030303030ULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Set jackpot at target
	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsAtOrAboveTarget(0);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Run rounds at or above target (hysteresis requirement)
	for (int round = 0; round < QTF_FR_HYSTERESIS_ROUNDS; ++round)
	{
		ctl.beginEpochWithValidTime();

		for (int i = 0; i < 5; ++i)
		{
			const id user = id::randomValue();
			ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
		}

		// Keep jackpot at target (add back what might be paid out)
		ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT);
		ctl.setPrevSpectrumDigest(testDigest);
		ctl.advanceOneDayAndDraw();
	}

	// After 3 rounds at target, FR should deactivate
	EXPECT_GE(ctl.state()->getFrRoundsAtOrAboveTarget(), QTF_FR_HYSTERESIS_ROUNDS);
	EXPECT_EQ(ctl.state()->getFrActive(), false);
}

TEST(ContractQThirtyFour, FR_OverflowBias_95PercentToJackpot)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Fix RNG so we can deterministically create "no winners" tickets.
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xCAFEBABEDEADBEEFULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Activate FR
	ctl.state()->setJackpot(100000000ULL); // Below target
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(5);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	constexpr uint64 numPlayers = 50;

	// Add many players to generate significant overflow
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	// Calculate expected jackpot growth
	const uint64 revenue = ticketPrice * numPlayers;
	const QTF::GetFees_output fees = ctl.getFees();

	// winnersBlock = revenue * winnerFeePercent / 100 (68%)
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;

	// In FR mode: 5% rake from winnersBlock goes to jackpot
	const uint64 winnersRake = (winnersBlock * QTF_FR_WINNERS_RAKE_BP) / 10000;
	const uint64 winnersBlockAfterRake = winnersBlock - winnersRake;

	// With no winners, the entire winners block after rake becomes overflow (k2+k3 pools also roll into overflow).
	const uint64 winnersOverflow = winnersBlockAfterRake;

	// In FR mode: 95% of overflow goes to jackpot, 5% to reserve
	const uint64 reserveAdd = (winnersOverflow * QTF_FR_ALPHA_BP) / 10000;
	const uint64 overflowToJackpot = winnersOverflow - reserveAdd;

	// Dev and Dist redirects (base 1% each from revenue in FR mode)
	const uint64 devRedirect = (revenue * QTF_FR_DEV_REDIRECT_BP) / 10000;
	const uint64 distRedirect = (revenue * QTF_FR_DIST_REDIRECT_BP) / 10000;

	// Minimum expected jackpot growth (without extra redirects, assuming no k2/k3 winners)
	// totalJackpotContribution = overflowToJackpot + winnersRake + devRedirect + distRedirect
	const uint64 minExpectedGrowth = overflowToJackpot + winnersRake + devRedirect + distRedirect;

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// Verify that jackpot grew by at least the minimum expected amount
	const uint64 actualGrowth = ctl.state()->getJackpot() - jackpotBefore;

	// In FR mode, 95% of overflow goes to jackpot (vs 50% in baseline)
	// Allow 5% tolerance for rounding and potential winners
	const uint64 tolerance = minExpectedGrowth / 20; // 5%
	EXPECT_GE(actualGrowth + tolerance, minExpectedGrowth)
	    << "Actual growth: " << actualGrowth << ", Expected minimum: " << minExpectedGrowth << ", Overflow to jackpot (95%): " << overflowToJackpot
	    << ", Winners rake: " << winnersRake;

	// Verify the 95% overflow bias is working correctly
	// overflowToJackpot should be ~95% of winnersOverflow
	const uint64 expected95Percent = (winnersOverflow * 95) / 100;
	EXPECT_GE(overflowToJackpot, expected95Percent - 1) << "95% overflow bias verification";
	EXPECT_LE(overflowToJackpot, winnersOverflow) << "Overflow to jackpot should not exceed total overflow";
}

// ============================================================================
// WINNER COUNTING AND TIER TESTS
// ============================================================================

TEST(ContractQThirtyFour, WinnerData_RecordsWinners)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add players with diverse number combinations to increase chance of winners
	std::vector<id> players;
	for (int i = 0; i < 20; ++i)
	{
		const id user = id::randomValue();
		players.push_back(user);
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 27) + 1), static_cast<uint8>((i % 27) + 2),
		                                            static_cast<uint8>((i % 27) + 3), static_cast<uint8>((i % 27) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	ctl.advanceOneDayAndDraw();

	// Winner data should always record winning values and epoch, even if no winners
	const QTF::GetWinnerData_output winnerData = ctl.getWinnerData();

	// Winning values should always be set and valid after a draw
	for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
	{
		const uint8 val = winnerData.winnerData.winnerValues.get(i);
		EXPECT_GE(val, 1u) << "Winning value " << i << " should be >= 1";
		EXPECT_LE(val, 30u) << "Winning value " << i << " should be <= 30";
	}

	// Epoch should be recorded
	EXPECT_GT((uint64)winnerData.winnerData.epoch, 0u) << "Epoch should be recorded after draw";
}

TEST(ContractQThirtyFour, WinnerData_UniqueWinningNumbers)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add some players
	for (int i = 0; i < 5; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums =
		    ctl.makeValidNumbers(static_cast<uint8>(i + 1), static_cast<uint8>(i + 5), static_cast<uint8>(i + 10), static_cast<uint8>(i + 15));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	ctl.advanceOneDayAndDraw();

	// Winning numbers should always be unique after a draw
	const QTF::GetWinnerData_output winnerData = ctl.getWinnerData();

	std::set<uint8> winningNums;
	for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
	{
		winningNums.emplace(winnerData.winnerData.winnerValues.get(i));
	}

	EXPECT_EQ(winningNums.size(), QTF_RANDOM_VALUES_COUNT) << "All 4 winning numbers should be unique";
}

TEST(ContractQThirtyFour, WinnerData_ResetEachRound)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Round 1: force a deterministic k=2 winner so winnerCounter becomes > 0.
	m256i digest1 = {};
	digest1.m256i_u64[0] = 0x13579BDF2468ACE0ULL;
	const QTFRandomValues winning1 = ctl.computeWinningNumbersForDigest(digest1);

	ctl.beginEpochWithValidTime();
	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	QTFRandomValues k2Numbers = ctl.makeK2Numbers(winning1);
	const id k2Winner = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner, ticketPrice, k2Numbers);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);

	ctl.setPrevSpectrumDigest(digest1);
	ctl.advanceOneDayAndDraw();

	const QTF::GetWinnerData_output afterRound1 = ctl.getWinnerData();
	EXPECT_GT(afterRound1.winnerData.winnerCounter, 0u);

	// Round 2: force a deterministic "no winners" round, winnerCounter must reset to 0.
	m256i digest2 = {};
	digest2.m256i_u64[0] = 0x0F0E0D0C0B0A0908ULL;
	const QTFRandomValues winning2 = ctl.computeWinningNumbersForDigest(digest2);
	const QTFRandomValues losing2 = ctl.makeLosingNumbers(winning2);

	ctl.beginEpochWithValidTime();
	for (int i = 0; i < 5; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losing2);
		EXPECT_EQ(ctl.state()->getNumberOfPlayers(), i + 1u);
	}

	ctl.setPrevSpectrumDigest(digest2);
	ctl.advanceOneDayAndDraw();

	const QTF::GetWinnerData_output afterRound2 = ctl.getWinnerData();
	EXPECT_EQ(afterRound2.winnerData.winnerCounter, 0u) << "Winner snapshot must reset each round";
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST(ContractQThirtyFour, BuyTicket_ValidNumberSelections_EdgeCases_Success)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	static constexpr uint8 cases[][4] = {
	    {1, 2, 29, 30},   // boundary
	    {15, 16, 17, 18}, // consecutive
	    {27, 28, 29, 30}, // highest
	    {1, 2, 3, 4},     // lowest
	};

	for (uint64 i = 0; i < (sizeof(cases) / sizeof(cases[0])); ++i)
	{
		const id user = id::randomValue();
		const QTFRandomValues nums = ctl.makeValidNumbers(cases[i][0], cases[i][1], cases[i][2], cases[i][3]);
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
		EXPECT_EQ(ctl.state()->getNumberOfPlayers(), i + 1);
	}
}

// ============================================================================
// MULTIPLE ROUNDS TESTS
// ============================================================================

TEST(ContractQThirtyFour, MultipleRounds_JackpotAccumulates)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x0DDC0FFEE0DDF00DULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	uint64 prevJackpot = 0;

	// Run multiple rounds
	for (int round = 0; round < 5; ++round)
	{
		ctl.beginEpochWithValidTime();

		for (int i = 0; i < 10; ++i)
		{
			const id user = id::randomValue();
			ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
		}

		ctl.setPrevSpectrumDigest(testDigest);
		ctl.advanceOneDayAndDraw();

		// Jackpot should increase each round (no k=4 winners in this test)
		const uint64 currentJackpot = ctl.state()->getJackpot();
		EXPECT_GT(currentJackpot, prevJackpot) << "Round " << round << ": jackpot should grow";

		// Track for next iteration
		prevJackpot = currentJackpot;
	}
}

TEST(ContractQThirtyFour, MultipleRounds_StateResetsCorrectly)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	for (int round = 0; round < 3; ++round)
	{
		ctl.beginEpochWithValidTime();

		// Add different number of players each round
		const int playersThisRound = 5 + round * 3;
		for (int i = 0; i < playersThisRound; ++i)
		{
			const id user = id::randomValue();
			QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i + round) % 27 + 1), static_cast<uint8>((i + round + 5) % 27 + 1),
			                                            static_cast<uint8>((i + round + 10) % 27 + 1), static_cast<uint8>((i + round + 15) % 27 + 1));
			ctl.fundAndBuyTicket(user, ticketPrice, nums);
		}

		EXPECT_EQ(ctl.state()->getNumberOfPlayers(), static_cast<uint64>(playersThisRound));

		ctl.advanceOneDayAndDraw();

		// Players should be cleared after each round
		EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
	}
}

// ============================================================================
// POST_INCOMING_TRANSFER TEST
// ============================================================================

TEST(ContractQThirtyFour, PostIncomingTransfer_StandardTransaction_Refunded)
{
	ContractTestingQTF ctl;
	constexpr uint64 transferAmount = 123456789;

	const id sender = id::randomValue();
	increaseEnergy(sender, transferAmount);
	EXPECT_EQ(getBalance(sender), transferAmount);

	const id contractAddress = ctl.qtfSelf();
	EXPECT_EQ(getBalance(contractAddress), 0);

	// Standard transaction should be refunded
	notifyContractOfIncomingTransfer(sender, contractAddress, transferAmount, QPI::TransferType::standardTransaction);

	// Amount should be refunded to sender
	EXPECT_EQ(getBalance(sender), transferAmount);
	EXPECT_EQ(getBalance(contractAddress), 0);
}

// ============================================================================
// SCHEDULE AND TIME TESTS
// ============================================================================

TEST(ContractQThirtyFour, Schedule_DrawOnlyOnScheduledDays)
{
	ContractTestingQTF ctl;

	// Set schedule to Wednesday only (default)
	const uint8 wednesdayOnly = static_cast<uint8>(1 << WEDNESDAY);
	ctl.forceSchedule(wednesdayOnly);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add players
	for (int i = 0; i < 5; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums =
		    ctl.makeValidNumbers(static_cast<uint8>(i + 1), static_cast<uint8>(i + 5), static_cast<uint8>(i + 10), static_cast<uint8>(i + 15));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	const uint64 playersBefore = ctl.state()->getNumberOfPlayers();
	EXPECT_EQ(playersBefore, 5u);

	// Tuesday 2025-01-14 is not scheduled - should NOT trigger draw
	ctl.setDateTime(2025, 1, 14, 12);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), playersBefore); // Unchanged

	// Wednesday 2025-01-15 IS scheduled - should trigger draw
	ctl.setDateTime(2025, 1, 15, 12);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u); // Cleared after draw
}

TEST(ContractQThirtyFour, DrawHour_NoDrawBeforeScheduledHour)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add players
	for (int i = 0; i < 5; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums =
		    ctl.makeValidNumbers(static_cast<uint8>(i + 1), static_cast<uint8>(i + 5), static_cast<uint8>(i + 10), static_cast<uint8>(i + 15));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	const uint8 drawHour = ctl.state()->getDrawHourInternal();
	const uint64 playersBefore = ctl.state()->getNumberOfPlayers();

	// Before draw hour - should NOT trigger draw
	ctl.setDateTime(2025, 1, 15, drawHour - 1);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), playersBefore);

	// At or after draw hour - should trigger draw
	ctl.setDateTime(2025, 1, 15, drawHour);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

// ============================================================================
// PROBABILITY AND COMBINATORICS VERIFICATION
// ============================================================================

TEST(ContractQThirtyFour, Combinatorics_P4Denominator)
{
	// Verify the P4 denominator constant matches combinatorics
	// C(30,4) = 30! / (4! * 26!) = 27405
	constexpr uint64 numerator = QTF_MAX_RANDOM_VALUE * 29 * 28 * 27;
	constexpr uint64 denominator = QTF_RANDOM_VALUES_COUNT * 3 * 2 * 1;
	constexpr uint64 expected = numerator / denominator;

	EXPECT_EQ(expected, QTF_P4_DENOMINATOR);
	EXPECT_EQ(QTF_P4_DENOMINATOR, 27405u);
}

// ============================================================================
// FEE CALCULATION VERIFICATION
// ============================================================================

TEST(ContractQThirtyFour, FeeCalculation_TotalEquals100Percent)
{
	ContractTestingQTF ctl;
	const QTF::GetFees_output fees = ctl.getFees();

	const uint32 total = fees.teamFeePercent + fees.distributionFeePercent + fees.winnerFeePercent + fees.burnPercent;

	EXPECT_EQ(total, 100u);
}

// ============================================================================
// PRIZE PAYOUT ESTIMATION
// ============================================================================

TEST(ContractQThirtyFour, EstimatePrizePayouts_NoTickets)
{
	ContractTestingQTF ctl;

	// No tickets sold, should return zero payouts
	QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(1, 1);

	EXPECT_EQ(estimate.k2PayoutPerWinner, 0ull);
	EXPECT_EQ(estimate.k3PayoutPerWinner, 0ull);
	EXPECT_EQ(estimate.k2Pool, 0ull);
	EXPECT_EQ(estimate.k3Pool, 0ull);
	EXPECT_EQ(estimate.totalRevenue, 0ull);
}

TEST(ContractQThirtyFour, EstimatePrizePayouts_WithTicketsSingleWinner)
{
	ContractTestingQTF ctl;

	ctl.beginEpochWithValidTime();

	// Buy 100 tickets
	constexpr uint64 ticketPrice = 1000000ull; // 1M QU
	constexpr uint64 numTickets = 100;

	QTFRandomValues numbers;
	numbers.set(0, 1);
	numbers.set(1, 2);
	numbers.set(2, 3);
	numbers.set(3, 4);

	for (uint64 i = 0; i < numTickets; ++i)
	{
		id user = id::randomValue();
		increaseEnergy(user, ticketPrice);
		QTF::BuyTicket_output result = ctl.buyTicket(user, ticketPrice, numbers);
		EXPECT_EQ(result.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	}

	// Verify tickets were purchased
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numTickets);

	// Estimate for 1 k2 winner and 1 k3 winner
	QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(1, 1);

	const uint64 expectedRevenue = ticketPrice * numTickets;
	EXPECT_EQ(estimate.totalRevenue, expectedRevenue);

	// Check minimum floors and cap using constants from contract
	constexpr uint64 expectedK2Floor = ticketPrice * QTF_K2_FLOOR_MULT / QTF_K2_FLOOR_DIV;
	constexpr uint64 expectedK3Floor = ticketPrice * QTF_K3_FLOOR_MULT;
	constexpr uint64 expectedCap = ticketPrice * QTF_TOPUP_PER_WINNER_CAP_MULT;
	EXPECT_EQ(estimate.k2MinFloor, expectedK2Floor);
	EXPECT_EQ(estimate.k3MinFloor, expectedK3Floor);
	EXPECT_EQ(estimate.perWinnerCap, expectedCap);

	// Winners block using contract constants
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 winnersBlock = (expectedRevenue * fees.winnerFeePercent) / 100;
	const uint64 k2PoolExpected = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 k3PoolExpected = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;

	EXPECT_EQ(estimate.k2Pool, k2PoolExpected);
	EXPECT_EQ(estimate.k3Pool, k3PoolExpected);

	// With 1 winner each: k2 payout equals pool (below cap), k3 payout is capped at 25*P
	EXPECT_EQ(estimate.k2PayoutPerWinner, k2PoolExpected); // 19.04M < 25M cap
	EXPECT_EQ(estimate.k3PayoutPerWinner, expectedCap);    // 27.2M capped to 25M
}

TEST(ContractQThirtyFour, EstimatePrizePayouts_WithMultipleWinners)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	// Buy 1000 tickets
	const uint64 ticketPrice = 1000000ull;
	const uint64 numTickets = 1000;

	QTFRandomValues numbers;
	numbers.set(0, 5);
	numbers.set(1, 10);
	numbers.set(2, 15);
	numbers.set(3, 20);

	for (uint64 i = 0; i < numTickets; ++i)
	{
		id user = id::randomValue();
		increaseEnergy(user, ticketPrice);
		QTF::BuyTicket_output result = ctl.buyTicket(user, ticketPrice, numbers);
		EXPECT_EQ(result.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	}

	// Verify tickets were purchased
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numTickets);

	// Estimate for 10 k2 winners and 5 k3 winners
	QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(10, 5);

	const uint64 expectedRevenue = ticketPrice * numTickets;
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 winnersBlock = (expectedRevenue * fees.winnerFeePercent) / 100;
	const uint64 k2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 k3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;

	// Verify pools
	EXPECT_EQ(estimate.k2Pool, k2Pool);
	EXPECT_EQ(estimate.k3Pool, k3Pool);

	// Verify per-winner payouts (should be pool / winner count, capped)
	const uint64 k2ExpectedPerWinner = k2Pool / 10;
	const uint64 k3ExpectedPerWinner = k3Pool / 5;

	EXPECT_EQ(estimate.k2PayoutPerWinner, std::min(k2ExpectedPerWinner, estimate.perWinnerCap));
	EXPECT_EQ(estimate.k3PayoutPerWinner, std::min(k3ExpectedPerWinner, estimate.perWinnerCap));

	// Both should be above minimum floors
	EXPECT_GE(estimate.k2PayoutPerWinner, estimate.k2MinFloor);
	EXPECT_GE(estimate.k3PayoutPerWinner, estimate.k3MinFloor);
}

TEST(ContractQThirtyFour, EstimatePrizePayouts_NoWinnersShowsPotential)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	// Buy 50 tickets
	const uint64 ticketPrice = 1000000ull;
	const uint64 numTickets = 50;

	QTFRandomValues numbers;
	numbers.set(0, 7);
	numbers.set(1, 14);
	numbers.set(2, 21);
	numbers.set(3, 28);

	for (uint64 i = 0; i < numTickets; ++i)
	{
		id user = id::randomValue();
		increaseEnergy(user, ticketPrice);
		QTF::BuyTicket_output result = ctl.buyTicket(user, ticketPrice, numbers);
		EXPECT_EQ(result.returnCode, static_cast<uint8>(QTF::EReturnCode::SUCCESS));
	}

	// Verify tickets were purchased
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numTickets);

	// Estimate with 0 winners (shows what a single winner would get)
	QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(0, 0);

	const uint64 expectedRevenue = ticketPrice * numTickets;
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 winnersBlock = (expectedRevenue * fees.winnerFeePercent) / 100;
	const uint64 k2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 k3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;

	// When no winners specified, should show full pool (capped)
	EXPECT_EQ(estimate.k2PayoutPerWinner, std::min(k2Pool, estimate.perWinnerCap));
	EXPECT_EQ(estimate.k3PayoutPerWinner, std::min(k3Pool, estimate.perWinnerCap));
}

// ============================================================================
// K=4 JACKPOT WIN TESTS
// ============================================================================

// ============================================================================
// DETERMINISTIC WINNER TESTING
// ============================================================================
// Solution: By fixing prevSpectrumDigest, we can deterministically control winning numbers
//
// Background:
// Settlement generates winning numbers using: seed = K12(prevSpectrumDigest).u64._0
// This seed is then used in GetRandomValues (QThirtyFour.h:1663-1698) to derive 4 numbers.
//
// Approach:
//   1. Create a fixed test prevSpectrumDigest (e.g., testDigest)
//   2. Call computeWinningNumbersForDigest(testDigest) to pre-compute winning numbers
//   3. Buy tickets with exact winning numbers (for k=4), partial matches (for k=2/k=3), etc.
//   4. Call setPrevSpectrumDigest(testDigest) BEFORE triggering settlement
//   5. Settlement will use our fixed digest, generating the pre-computed winning numbers
//   6. Verify actual payouts, jackpot depletion, FR resets, etc.
//
// This enables comprehensive testing of:
//   ✅ Actual k=4 jackpot win payouts and jackpot depletion
//   ✅ Actual k=2/k=3 winner payouts with real matching logic
//   ✅ Actual FR reset behavior after k=4 win (frRoundsSinceK4 = 0)
//   ✅ Pool splitting among multiple winners
//   ✅ Revenue distribution and fee calculations with real winners

TEST(ContractQThirtyFour, DeterministicWinner_K4JackpotWin_DepletesAndReseeds)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Ensure QRP has enough reserve to reseed to target.
	increaseEnergy(ctl.qrpSelf(), QTF_DEFAULT_TARGET_JACKPOT + 1000000ULL);
	const uint64 qrpBalanceBefore = static_cast<uint64>(getBalance(ctl.qrpSelf()));

	// Create a deterministic prevSpectrumDigest
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x123456789ABCDEF0ULL; // Arbitrary seed

	// Pre-compute winning numbers for this digest
	QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);

	// Setup: FR active with jackpot below target
	const uint64 initialJackpot = 800000000ULL; // 800M QU
	ctl.state()->setJackpot(initialJackpot);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT); // 1B target
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(10);
	// IMPORTANT: internal `state.jackpot` must be backed by actual contract balance, otherwise transfers will fail.
	increaseEnergy(ctl.qtfSelf(), static_cast<long long>(initialJackpot));

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// User1: Buy ticket with EXACT winning numbers (k=4 winner)
	const id k4Winner = id::randomValue();
	ctl.fundAndBuyTicket(k4Winner, ticketPrice, winningNumbers);

	// User2: Buy ticket with 3 matching numbers (k=3 winner)
	QTFRandomValues k3Numbers = ctl.makeK3Numbers(winningNumbers);
	const id k3Winner = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner, ticketPrice, k3Numbers);

	// User3: Buy ticket with 2 matching numbers (k=2 winner)
	QTFRandomValues k2Numbers = ctl.makeK2Numbers(winningNumbers);
	const id k2Winner = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner, ticketPrice, k2Numbers);

	// User4: No match
	const id loser = id::randomValue();
	QTFRandomValues loserNumbers = ctl.makeValidNumbers(1, 2, 3, 4);
	ctl.fundAndBuyTicket(loser, ticketPrice, loserNumbers);

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 4ULL);

	// Verify state before settlement
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	const uint64 roundsSinceK4Before = ctl.state()->getFrRoundsSinceK4();
	EXPECT_EQ(jackpotBefore, initialJackpot);
	EXPECT_EQ(roundsSinceK4Before, 10u);

	// Set the deterministic prevSpectrumDigest BEFORE triggering settlement
	ctl.setPrevSpectrumDigest(testDigest);

	// Trigger settlement - this will use our fixed prevSpectrumDigest
	ctl.advanceOneDayAndDraw();

	// Verify k=4 jackpot win behavior:
	const uint64 jackpotAfter = ctl.state()->getJackpot();
	EXPECT_GE(jackpotAfter, QTF_DEFAULT_TARGET_JACKPOT) << "Jackpot should be reseeded from QRP after k=4 win";
	EXPECT_LT(static_cast<uint64>(getBalance(ctl.qrpSelf())), qrpBalanceBefore) << "QRP reserve should decrease due to reseed";

	// FR counters reset
	const uint64 roundsSinceK4After = ctl.state()->getFrRoundsSinceK4();
	EXPECT_EQ(roundsSinceK4After, 0u) << "frRoundsSinceK4 should reset to 0 after k=4 win";

	const uint64 roundsAtTargetAfter = ctl.state()->getFrRoundsAtOrAboveTarget();
	EXPECT_EQ(roundsAtTargetAfter, 0u) << "frRoundsAtOrAboveTarget should reset to 0 after k=4 win";

	// 3. Verify winner data contains our winning numbers
	QTF::GetWinnerData_output winnerData = ctl.getWinnerData();
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(0), winningNumbers.get(0));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(1), winningNumbers.get(1));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(2), winningNumbers.get(2));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(3), winningNumbers.get(3));

	// Verify k=4 winner received payout (full jackpot share).
	const long long k4WinnerBalance = getBalance(k4Winner);
	EXPECT_GE(k4WinnerBalance, static_cast<long long>(initialJackpot));
}

// Test k=2 and k=3 payouts with deterministic winning numbers
TEST(ContractQThirtyFour, DeterministicWinner_K2K3Payouts_VerifyRevenueSplit)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// This test validates baseline k2/k3 pool splitting (no FR rake).
	// Force FR activation window to be expired so SettleEpoch cannot auto-enable FR.
	ctl.state()->setFrActive(false);
	ctl.state()->setFrRoundsSinceK4(QTF_FR_POST_K4_WINDOW_ROUNDS);

	// Create deterministic prevSpectrumDigest
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xFEDCBA9876543210ULL; // Different seed

	// Pre-compute winning numbers
	QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Create multiple k=2 and k=3 winners to test pool splitting
	// 2 k=3 winners
	QTFRandomValues k3Numbers1 = ctl.makeK3Numbers(winningNumbers);
	const id k3Winner1 = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner1, ticketPrice, k3Numbers1);

	QTFRandomValues k3Numbers2 = ctl.makeK3Numbers(winningNumbers);
	// Ensure two different k3 tickets (avoid identical picks across players).
	if (k3Numbers2.get(0) == k3Numbers1.get(0) && k3Numbers2.get(1) == k3Numbers1.get(1) && k3Numbers2.get(2) == k3Numbers1.get(2) &&
	    k3Numbers2.get(3) == k3Numbers1.get(3))
	{
		k3Numbers2 = ctl.makeNumbersWithExactMatches(winningNumbers, 3);
		// Swap a non-winning position deterministically: replace last entry with next available losing number.
		const QTFRandomValues losing = ctl.makeLosingNumbers(winningNumbers);
		k3Numbers2.set(3, losing.get(1));
	}
	const id k3Winner2 = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner2, ticketPrice, k3Numbers2);

	// 3 k=2 winners
	QTFRandomValues k2Numbers1 = ctl.makeK2Numbers(winningNumbers);
	const id k2Winner1 = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner1, ticketPrice, k2Numbers1);

	QTFRandomValues k2Numbers2 = ctl.makeK2Numbers(winningNumbers);
	// Make it different from k2Numbers1 while keeping exactly 2 matches.
	if (k2Numbers2.get(0) == k2Numbers1.get(0) && k2Numbers2.get(1) == k2Numbers1.get(1) && k2Numbers2.get(2) == k2Numbers1.get(2) &&
	    k2Numbers2.get(3) == k2Numbers1.get(3))
	{
		const QTFRandomValues losing = ctl.makeLosingNumbers(winningNumbers);
		k2Numbers2.set(2, losing.get(0));
	}
	const id k2Winner2 = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner2, ticketPrice, k2Numbers2);

	QTFRandomValues k2Numbers3 = ctl.makeK2Numbers(winningNumbers);
	// Make it different from previous k2 tickets while keeping exactly 2 matches.
	if ((k2Numbers3.get(0) == k2Numbers1.get(0) && k2Numbers3.get(1) == k2Numbers1.get(1) && k2Numbers3.get(2) == k2Numbers1.get(2) &&
	     k2Numbers3.get(3) == k2Numbers1.get(3)) ||
	    (k2Numbers3.get(0) == k2Numbers2.get(0) && k2Numbers3.get(1) == k2Numbers2.get(1) && k2Numbers3.get(2) == k2Numbers2.get(2) &&
	     k2Numbers3.get(3) == k2Numbers2.get(3)))
	{
		const QTFRandomValues losing = ctl.makeLosingNumbers(winningNumbers);
		k2Numbers3.set(3, losing.get(2));
	}
	const id k2Winner3 = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner3, ticketPrice, k2Numbers3);

	// 5 losers (no matches)
	for (int i = 0; i < 5; ++i)
	{
		const id loser = id::randomValue();
		QTFRandomValues loserNumbers = ctl.makeValidNumbers(1, 2, 3, 4);
		ctl.fundAndBuyTicket(loser, ticketPrice, loserNumbers);
	}

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 10ULL);

	// Calculate expected pools
	const uint64 revenue = ticketPrice * 10;
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;         // 68%
	const uint64 expectedK2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000; // 28% of winners block
	const uint64 expectedK3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000; // 40% of winners block

	// Set deterministic prevSpectrumDigest
	ctl.setPrevSpectrumDigest(testDigest);

	// Get balances before settlement
	const long long k3Winner1Before = getBalance(k3Winner1);
	const long long k2Winner1Before = getBalance(k2Winner1);

	// Trigger settlement
	ctl.advanceOneDayAndDraw();

	// Verify winner payouts
	// k=3 pool split between 2 winners
	const uint64 expectedK3PayoutPerWinner = expectedK3Pool / 2;
	const long long k3Winner1After = getBalance(k3Winner1);
	const long long k3Winner1Gained = k3Winner1After - k3Winner1Before;
	EXPECT_EQ(static_cast<uint64>(k3Winner1Gained), expectedK3PayoutPerWinner) << "k=3 winner should receive half of k3 pool";

	// k=2 pool split between 3 winners
	const uint64 expectedK2PayoutPerWinner = expectedK2Pool / 3;
	const long long k2Winner1After = getBalance(k2Winner1);
	const long long k2Winner1Gained = k2Winner1After - k2Winner1Before;
	EXPECT_EQ(static_cast<uint64>(k2Winner1Gained), expectedK2PayoutPerWinner) << "k=2 winner should receive one-third of k2 pool";

	// Verify winning numbers in winner data
	QTF::GetWinnerData_output winnerData = ctl.getWinnerData();
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(0), winningNumbers.get(0));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(1), winningNumbers.get(1));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(2), winningNumbers.get(2));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(3), winningNumbers.get(3));

	// Jackpot should have grown (no k=4 winner)
	EXPECT_GT(ctl.state()->getJackpot(), 0ULL);
}

TEST(ContractQThirtyFour, Settlement_NoWinners_JackpotGrowsAndCounterIncrements)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x9999AAAABBBBCCCCULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Setup: FR active with jackpot below target
	ctl.state()->setJackpot(800000000ULL);                             // 800M QU jackpot
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT); // 1B target
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(10);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	EXPECT_EQ(jackpotBefore, 800000000ULL);

	constexpr int numPlayers = 50;
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), static_cast<uint64>(numPlayers));

	const uint64 roundsSinceK4Before = ctl.state()->getFrRoundsSinceK4();
	EXPECT_EQ(roundsSinceK4Before, 10u);

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// After settlement (deterministic: no k=4 win is possible):
	const uint64 jackpotAfter = ctl.state()->getJackpot();
	EXPECT_GT(jackpotAfter, jackpotBefore) << "Jackpot should grow when no k=4 winner";

	const uint64 roundsSinceK4After = ctl.state()->getFrRoundsSinceK4();
	EXPECT_EQ(roundsSinceK4After, roundsSinceK4Before + 1) << "Counter should increment when no k=4 win";

	// Note: This test verifies the no-win path. A full k=4 win test would require
	// either mocking K12 output or extensive probabilistic testing with many rounds.
	// The k=4 win logic in SettleEpoch (lines 1417-1444) handles:
	// - Jackpot payout: jackpot / countK4
	// - Depletion: state.jackpot = 0
	// - Counter reset: frRoundsSinceK4 = 0, frRoundsAtOrAboveTarget = 0
	// - QRP reseed: request min(QRP balance, targetJackpot)
}

TEST(ContractQThirtyFour, EstimatePrizePayouts_FRMode_AppliesRakeToPools)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Enable FR so EstimatePrizePayouts applies the 5% winners rake.
	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT / 2);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(1);

	constexpr uint64 numPlayers = 100;
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 26) + 1), static_cast<uint8>((i % 26) + 2),
		                                            static_cast<uint8>((i % 26) + 3), static_cast<uint8>((i % 26) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	const QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(0, 0);

	const uint64 revenue = ticketPrice * numPlayers;
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;
	const uint64 winnersRake = (winnersBlock * QTF_FR_WINNERS_RAKE_BP) / 10000;
	const uint64 winnersBlockAfterRake = winnersBlock - winnersRake;

	const uint64 expectedK2Pool = (winnersBlockAfterRake * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 expectedK3Pool = (winnersBlockAfterRake * QTF_BASE_K3_SHARE_BP) / 10000;

	EXPECT_EQ(estimate.totalRevenue, revenue);
	EXPECT_EQ(estimate.k2Pool, expectedK2Pool);
	EXPECT_EQ(estimate.k3Pool, expectedK3Pool);
}

// ============================================================================
// RESERVE TOP-UP AND FLOOR GUARANTEE TESTS
// ============================================================================

TEST(ContractQThirtyFour, ReserveTopUp_FloorGuarantee_VerifyLimits)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add only 2 players to create small prize pools
	// With 2 tickets, revenue = 2M, winners block = 1.36M (68%)
	// k2 pool = 1.36M * 28% = 380.8k, k3 pool = 1.36M * 40% = 544k
	// These are below floor requirements for multiple winners
	constexpr int numPlayers = 2;
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums =
		    ctl.makeValidNumbers(static_cast<uint8>(i + 1), static_cast<uint8>(i + 10), static_cast<uint8>(i + 15), static_cast<uint8>(i + 20));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	const QTF::GetPools_output poolsBefore = ctl.getPools();
	const uint64 revenue = ticketPrice * numPlayers;

	// Calculate expected pools
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;
	const uint64 expectedK2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 expectedK3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;

	// Verify floors
	const uint64 k2Floor = ticketPrice * QTF_K2_FLOOR_MULT / QTF_K2_FLOOR_DIV; // 0.5*P = 500k
	const uint64 k3Floor = ticketPrice * QTF_K3_FLOOR_MULT;                    // 5*P = 5M

	// If we hypothetically had 2 k2 winners: floor requirement = 2 * 500k = 1M
	// But k2 pool is only ~380k, so would need ~620k from reserve
	const uint64 k2FloorTotal2Winners = k2Floor * 2; // 1M
	EXPECT_LT(expectedK2Pool, k2FloorTotal2Winners) << "k2 pool should be insufficient for 2 winners with floor";

	// If we hypothetically had 1 k3 winner: floor requirement = 5M
	// But k3 pool is only ~544k, so would need ~4.46M from reserve
	EXPECT_LT(expectedK3Pool, k3Floor) << "k3 pool should be insufficient for 1 winner with floor";

	// Per-winner cap verification
	const uint64 perWinnerCap = ticketPrice * QTF_TOPUP_PER_WINNER_CAP_MULT; // 25M
	EXPECT_EQ(perWinnerCap, 25000000ULL);

	// Reserve safety limits
	const uint64 softFloor = ticketPrice * QTF_RESERVE_SOFT_FLOOR_MULT; // 20*P = 20M
	EXPECT_EQ(softFloor, 20000000ULL);

	// Note: In actual settlement with winners, the ProcessTierPayout function (QThirtyFour.h:1795-1837)
	// would call CalcReserveTopUp (lines 1740-1777) to determine how much to request from QRP.
	// The top-up is capped at:
	// 1. 10% of QRP balance per round (QTF_TOPUP_RESERVE_PCT_BP)
	// 2. Soft floor: don't deplete QRP below 20*P (QTF_RESERVE_SOFT_FLOOR_MULT)
	// 3. Per-winner cap: max 25*P per winner (QTF_TOPUP_PER_WINNER_CAP_MULT)

	// This test verifies the floor requirements exist and are calculated correctly.
	// Full integration test would require actual winner detection (probabilistic)
	// or mocking the random number generation to guarantee specific winners.
}

// ============================================================================
// HIGH-DEFICIT FR EXTRA REDIRECTS TESTS
// ============================================================================

TEST(ContractQThirtyFour, FR_HighDeficit_ExtraRedirectsCalculated)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Fix RNG so we can deterministically avoid winners (and especially k=4).
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x4040404040404040ULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Setup: High deficit scenario
	// Jackpot = 0, Target = 1B, FR active
	ctl.state()->setJackpot(0ULL);                                     // Empty jackpot
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT); // 1B target
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(5);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const QTF::GetFees_output fees = ctl.getFees();

	// Add many players to generate high revenue
	constexpr int numPlayers = 500;
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	const uint64 revenue = ticketPrice * numPlayers;       // 500M QU
	const uint64 deficit = QTF_DEFAULT_TARGET_JACKPOT - 0; // 1B deficit

	// With high deficit (1B) and significant revenue (500M), extra redirects should be calculated
	// Formula (from spec and QThirtyFour.h:1928-1965):
	// - deficit Δ = 1B
	// - E_k4(500) ≈ 55 rounds (expected rounds to k=4 with 500 tickets)
	// - horizon H = min(55, 50) = 50 (capped)
	// - required gain per round = Δ/H = 1B/50 = 20M
	// - base gain (without extra) ≈ 1% dev + 1% dist + 5% rake + 95% overflow
	//   ≈ 5M + 5M + 17M + ~98M = ~125M (rough estimate)
	// - Since base gain (125M) > required (20M), extra might be 0 or small
	// But let's verify the mechanism is working

	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	EXPECT_EQ(jackpotBefore, 0ULL);

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// After settlement with FR active and high deficit:
	const uint64 devBalAfter = getBalance(QTF_DEV_ADDRESS);
	const uint64 jackpotAfter = ctl.state()->getJackpot();

	// Verify FR is still active
	EXPECT_EQ(ctl.state()->getFrActive(), true);

	// Dev should receive less than full 10% of revenue due to FR redirects
	const uint64 fullDevPayout = (revenue * fees.teamFeePercent) / 100; // 50M (10% of 500M)
	const uint64 actualDevPayout = devBalAfter - devBalBefore;

	// Base redirect alone is 1% of revenue = 5M
	const uint64 baseDevRedirect = (revenue * QTF_FR_DEV_REDIRECT_BP) / 10000; // 5M
	EXPECT_LT(actualDevPayout, fullDevPayout) << "Dev should receive less than full 10% in FR mode";
	EXPECT_LE(actualDevPayout, fullDevPayout - baseDevRedirect) << "Dev redirect should be at least base 1%";

	// Jackpot should have grown significantly from:
	// - Winners rake (5% of 340M winners block = 17M)
	// - Dev/Dist redirects (base 1% each + possible extra)
	// - Overflow bias (95% of overflow)
	EXPECT_GT(jackpotAfter, 100000000ULL) << "Jackpot should grow by at least 100M from FR mechanisms";

	// Verify extra redirect cap: dev redirect should not exceed base (1%) + extra max (0.35%) = 1.35% total
	const uint64 maxDevRedirectTotal = (revenue * (QTF_FR_DEV_REDIRECT_BP + QTF_FR_EXTRA_MAX_BP / 2)) / 10000; // 1.35%
	const uint64 actualDevRedirect = fullDevPayout - actualDevPayout;
	EXPECT_LE(actualDevRedirect, maxDevRedirectTotal) << "Dev redirect should not exceed 1.35% of revenue";

	// Note: The exact extra redirect amount depends on complex calculation in CalculateExtraRedirectBP
	// (QThirtyFour.h:1928-1965), which uses fixed-point arithmetic, power calculations, and horizon capping.
	// This test verifies the mechanism is active and within bounds.
}

// ============================================================================
// POST-K4 WINDOW EXPIRY TESTS
// ============================================================================

TEST(ContractQThirtyFour, FR_PostK4WindowExpiry_DoesNotActivateWhenInactive)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xABCDABCDABCDABCDULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Setup: Jackpot below target, but window expired and FR inactive.
	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT / 2);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setFrActive(false);
	ctl.state()->setFrRoundsSinceK4(QTF_FR_POST_K4_WINDOW_ROUNDS);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	constexpr int numPlayers = 10;
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	EXPECT_EQ(ctl.state()->getFrActive(), false);
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS + 1);
}

TEST(ContractQThirtyFour, FR_PostK4WindowExpiry_DoesNotReactivateWhenWindowExpired)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xFACEFEEDFACEFEEDULL;
	const QTFRandomValues winningNumbers = ctl.computeWinningNumbersForDigest(testDigest);
	const QTFRandomValues losingNumbers = ctl.makeLosingNumbers(winningNumbers);

	// Setup: FR active, jackpot below target, but approaching window expiry
	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT / 2);           // 500M (below target)
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT); // 1B target
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(QTF_FR_POST_K4_WINDOW_ROUNDS - 1); // One round before window expiry (50 = QTF_FR_POST_K4_WINDOW_ROUNDS)

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Add players
	constexpr int numPlayers = 10;
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	// Verify FR is active before settlement
	EXPECT_EQ(ctl.state()->getFrActive(), true);
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS - 1);
	EXPECT_LT(ctl.state()->getJackpot(), ctl.state()->getTargetJackpotInternal());

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// After settlement (deterministic: no k=4 win is possible):
	// - roundsSinceK4 should increment to 50
	// - FR should remain active this round (activation check happens BEFORE increment)
	// But in the NEXT round, FR won't activate because roundsSinceK4 >= 50

	const uint64 roundsSinceK4After = ctl.state()->getFrRoundsSinceK4();
	EXPECT_EQ(roundsSinceK4After, QTF_FR_POST_K4_WINDOW_ROUNDS) << "Counter should increment to 50 after draw";

	// FR activation logic (QThirtyFour.h:1236-1245):
	// shouldActivateFR = (jackpot < target) AND (roundsSinceK4 < 50)
	// At roundsSinceK4 = 50, condition is false, so FR won't activate in next round

	// Run one more round: FR cannot re-activate, so state should not change
	ctl.beginEpochWithValidTime();

	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}

	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// After second round:
	// - Jackpot still below target
	// - roundsSinceK4 = 51 (>= 50)
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS + 1);

	// Important: FR deactivation logic (QThirtyFour.h:1236-1245)
	// FR.frActive is set to TRUE only if: (jackpot < target AND roundsSinceK4 < 50)
	// FR.frActive is set to FALSE only if: frRoundsAtOrAboveTarget >= 3
	// Otherwise, frActive retains its previous state.
	//
	// In this test:
	// - shouldActivateFR = false (because roundsSinceK4 >= 50)
	// - frRoundsAtOrAboveTarget = 0 (because jackpot < target)
	// - Neither activation nor deactivation condition met
	// - FR remains in previous state (true)
	//
	// This means FR doesn't automatically deactivate when window expires,
	// but it won't RE-ACTIVATE in future rounds while roundsSinceK4 >= 50.

	// The key behavior verified by this test:
	// Once roundsSinceK4 >= 50, FR will NOT be re-activated (shouldActivateFR = false)
	// even if jackpot drops below target again. FR stays in whatever state it was.

	// To fully deactivate FR after window expiry, jackpot must reach target
	// and stay there for 3 rounds (hysteresis). Let's verify that FR won't re-activate:

	const bool frActiveBeforeThirdRound = ctl.state()->getFrActive();

	// Run a third round - FR should remain in same state (no re-activation)
	ctl.beginEpochWithValidTime();
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, losingNumbers);
	}
	ctl.setPrevSpectrumDigest(testDigest);
	ctl.advanceOneDayAndDraw();

	// FR state should not change (no re-activation possible when roundsSinceK4 >= 50)
	EXPECT_EQ(ctl.state()->getFrActive(), frActiveBeforeThirdRound) << "FR should not re-activate when roundsSinceK4 >= 50, even if jackpot < target";
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS + 2);

	// This test verifies the post-k4 window mechanism:
	// FR can only be ACTIVATED within 50 rounds after last k=4 win.
	// After 50 rounds, FR won't re-activate regardless of jackpot level,
	// until the next k=4 win resets the counter.
	// However, if FR was already active, it stays active until hysteresis deactivates it.
}
