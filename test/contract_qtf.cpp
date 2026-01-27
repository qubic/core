#define NO_UEFI

#define _ALLOW_KEYWORD_MACROS 1

#define private protected
#include "contract_testing.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include <algorithm>
#include <set>
#include <vector>

// Procedure/function indices (must match REGISTER_USER_FUNCTIONS_AND_PROCEDURES in `src/contracts/QThirtyFour.h`).
constexpr uint16 QTF_PROCEDURE_BUY_TICKET = 1;
constexpr uint16 QTF_PROCEDURE_SET_PRICE = 2;
constexpr uint16 QTF_PROCEDURE_SET_SCHEDULE = 3;
constexpr uint16 QTF_PROCEDURE_SET_TARGET_JACKPOT = 4;
constexpr uint16 QTF_PROCEDURE_SET_DRAW_HOUR = 5;

constexpr uint16 QTF_FUNCTION_GET_TICKET_PRICE = 1;
constexpr uint16 QTF_FUNCTION_GET_NEXT_EPOCH_DATA = 2;
constexpr uint16 QTF_FUNCTION_GET_WINNER_DATA = 3;
constexpr uint16 QTF_FUNCTION_GET_POOLS = 4;
constexpr uint16 QTF_FUNCTION_GET_SCHEDULE = 5;
constexpr uint16 QTF_FUNCTION_GET_DRAW_HOUR = 6;
constexpr uint16 QTF_FUNCTION_GET_STATE = 7;
constexpr uint16 QTF_FUNCTION_GET_FEES = 8;
constexpr uint16 QTF_FUNCTION_ESTIMATE_PRIZE_PAYOUTS = 9;

using QTFRandomValues = Array<uint8, QTF_RANDOM_VALUES_COUNT>;

namespace
{
	static void issueRlSharesTo(std::vector<std::pair<m256i, unsigned int>>& initialOwnerShares)
	{
		issueContractShares(RL_CONTRACT_INDEX, initialOwnerShares, false);
	}

	static void primeQpiFunctionContext(QpiContextUserFunctionCall& qpi)
	{
		QTF::GetTicketPrice_input input{};
		qpi.call(QTF_FUNCTION_GET_TICKET_PRICE, &input, sizeof(input));
	}

	static void primeQpiProcedureContext(QpiContextUserProcedureCall& qpi, uint8 drawHour)
	{
		QTF::SetDrawHour_input input{};
		input.newDrawHour = drawHour;
		qpi.call(QTF_PROCEDURE_SET_DRAW_HOUR, &input, sizeof(input));
		ASSERT_EQ(contractError[QTF_CONTRACT_INDEX], 0);
	}

	static bool valuesEqual(const QTFRandomValues& a, const QTFRandomValues& b)
	{
		return memcmp(&a, &b, sizeof(a)) == 0;
	}

	static void expectWinnerValuesValidAndUnique(const QTF::GetWinnerData_output& winnerData)
	{
		std::set<uint8> unique;
		for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
		{
			const uint8 v = winnerData.winnerData.winnerValues.get(i);
			EXPECT_GE(v, 1u) << "Winning value " << i << " should be >= 1";
			EXPECT_LE(v, QTF_MAX_RANDOM_VALUE) << "Winning value " << i << " should be <= 30";
			unique.insert(v);
		}
		EXPECT_EQ(unique.size(), static_cast<size_t>(QTF_RANDOM_VALUES_COUNT)) << "All 4 winning numbers should be unique";
		EXPECT_GT(static_cast<uint64>(winnerData.winnerData.epoch), 0u) << "Epoch should be recorded after draw";
	}

	static void computeBaselinePrizePools(uint64 revenue, const QTF::GetFees_output& fees, uint64& winnersBlock, uint64& k2Pool, uint64& k3Pool)
	{
		winnersBlock = (revenue * static_cast<uint64>(fees.winnerFeePercent)) / 100ULL;
		k2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000ULL;
		k3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000ULL;
	}
} // namespace

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
	void setOverflowAlphaBP(uint64 value) { overflowAlphaBP = value; }

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
		QTF::GetTicketPrice_input input{};
		QTF::GetTicketPrice_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_TICKET_PRICE, input, output);
		return output;
	}

	QTF::GetNextEpochData_output getNextEpochData()
	{
		QTF::GetNextEpochData_input input{};
		QTF::GetNextEpochData_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_NEXT_EPOCH_DATA, input, output);
		return output;
	}

	QTF::GetWinnerData_output getWinnerData()
	{
		QTF::GetWinnerData_input input{};
		QTF::GetWinnerData_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_WINNER_DATA, input, output);
		return output;
	}

	QTF::GetPools_output getPools()
	{
		QTF::GetPools_input input{};
		QTF::GetPools_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_POOLS, input, output);
		return output;
	}

	QTF::GetSchedule_output getSchedule()
	{
		QTF::GetSchedule_input input{};
		QTF::GetSchedule_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_SCHEDULE, input, output);
		return output;
	}

	QTF::GetDrawHour_output getDrawHour()
	{
		QTF::GetDrawHour_input input{};
		QTF::GetDrawHour_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_DRAW_HOUR, input, output);
		return output;
	}

	QTF::GetState_output getStateInfo()
	{
		QTF::GetState_input input{};
		QTF::GetState_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_STATE, input, output);
		return output;
	}

	QTF::GetFees_output getFees()
	{
		QTF::GetFees_input input{};
		QTF::GetFees_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_GET_FEES, input, output);
		return output;
	}

	QTF::EstimatePrizePayouts_output estimatePrizePayouts(uint64 k2WinnerCount, uint64 k3WinnerCount)
	{
		QTF::EstimatePrizePayouts_input input{};
		input.k2WinnerCount = k2WinnerCount;
		input.k3WinnerCount = k3WinnerCount;
		QTF::EstimatePrizePayouts_output output{};
		callFunction(QTF_CONTRACT_INDEX, QTF_FUNCTION_ESTIMATE_PRIZE_PAYOUTS, input, output);
		return output;
	}

	// Procedure wrappers
	QTF::BuyTicket_output buyTicket(const id& user, uint64 reward, const QTFRandomValues& numbers)
	{
		QTF::BuyTicket_input input{};
		input.randomValues = numbers;
		QTF::BuyTicket_output output{};
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_BUY_TICKET, input, output, user, reward))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetPrice_output setPrice(const id& invocator, uint64 newPrice)
	{
		QTF::SetPrice_input input{};
		input.newPrice = newPrice;
		QTF::SetPrice_output output{};
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_PRICE, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetSchedule_output setSchedule(const id& invocator, uint8 newSchedule)
	{
		QTF::SetSchedule_input input{};
		input.newSchedule = newSchedule;
		QTF::SetSchedule_output output{};
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_SCHEDULE, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetTargetJackpot_output setTargetJackpot(const id& invocator, uint64 newTarget)
	{
		QTF::SetTargetJackpot_input input{};
		input.newTargetJackpot = newTarget;
		QTF::SetTargetJackpot_output output{};
		if (!invokeUserProcedure(QTF_CONTRACT_INDEX, QTF_PROCEDURE_SET_TARGET_JACKPOT, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(QTF::EReturnCode::MAX_VALUE);
		}
		return output;
	}

	QTF::SetDrawHour_output setDrawHour(const id& invocator, uint8 newHour)
	{
		QTF::SetDrawHour_input input{};
		input.newDrawHour = newHour;
		QTF::SetDrawHour_output output{};
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
		system.tick = system.tick + (RL_TICK_UPDATE_PERIOD - (system.tick % RL_TICK_UPDATE_PERIOD));
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

	void forceFRDisabledForBaseline()
	{
		state()->setFrActive(false);
		state()->setFrRoundsSinceK4(QTF_FR_POST_K4_WINDOW_ROUNDS);
		state()->setOverflowAlphaBP(QTF_BASELINE_OVERFLOW_ALPHA_BP);
	}

	void forceFREnabledWithinWindow(uint16 roundsSinceK4 = 1)
	{
		state()->setFrActive(true);
		state()->setFrRoundsSinceK4(roundsSinceK4);
	}

	void startAnyDayEpoch()
	{
		forceSchedule(QTF_ANY_DAY_SCHEDULE);
		beginEpochWithValidTime();
	}

	// Trigger a tick that performs the draw (time is set to a scheduled day and hour).
	void triggerDrawTick()
	{
		constexpr uint16 y = 2025;
		constexpr uint8 m = 1;
		constexpr uint8 d = 10;
		setDateTime(y, m, d, 12);
		__pauseLogMessage();
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

	void drawWithDigest(const m256i& digest)
	{
		setPrevSpectrumDigest(digest);
		triggerDrawTick();
	}

	// Compute the winning numbers that would be generated for a given prevSpectrumDigest.
	// Uses the contract GetRandomValues() implementation (so tests don't duplicate RNG logic).
	// Returns values in generation order (not sorted).
	QTFRandomValues computeWinningNumbersForDigest(const m256i& digest)
	{
		m256i hashResult;
		KangarooTwelve((const uint8*)&digest, sizeof(m256i), (uint8*)&hashResult, sizeof(m256i));
		const uint64 seed = hashResult.m256i_u64[0];

		QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
		primeQpiFunctionContext(qpi);
		const auto out = state()->callGetRandomValues(qpi, seed);
		return out.values;
	}

	struct WinningAndLosing
	{
		QTFRandomValues winning;
		QTFRandomValues losing;
	};

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

	WinningAndLosing computeWinningAndLosing(const m256i& digest)
	{
		WinningAndLosing out;
		out.winning = computeWinningNumbersForDigest(digest);
		out.losing = makeLosingNumbers(out.winning);
		return out;
	}

	void buyRandomTickets(uint64 count, uint64 ticketPrice, const QTFRandomValues& numbers)
	{
		for (uint64 i = 0; i < count; ++i)
		{
			const id user = id::randomValue();
			fundAndBuyTicket(user, ticketPrice, numbers);
		}
	}

	// Create a ticket that matches exactly `matchCount` numbers with `winningNumbers`.
	// `variant` makes it deterministic to generate multiple distinct tickets for the same winning set.
	// Guarantees values are unique and in [1..30].
	QTFRandomValues makeNumbersWithExactMatches(const QTFRandomValues& winningNumbers, uint8 matchCount, uint8 variant = 0)
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

		// Take `matchCount` winning numbers as the matches (variant-dependent, wrap around 4).
		for (uint8 i = 0; i < matchCount; ++i)
		{
			const uint8 v = winningNumbers.get((variant + i) % QTF_RANDOM_VALUES_COUNT);
			used[v] = true;
			ticket.set(outIndex++, v);
		}

		// Fill the remaining positions with non-winning numbers.
		const uint8 start = static_cast<uint8>((variant * 7) % QTF_MAX_RANDOM_VALUE + 1);
		for (uint8 step = 0; step < QTF_MAX_RANDOM_VALUE && outIndex < QTF_RANDOM_VALUES_COUNT; ++step)
		{
			const uint8 candidate = static_cast<uint8>(((start - 1 + step) % QTF_MAX_RANDOM_VALUE) + 1);
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

	QTFRandomValues makeK2Numbers(const QTFRandomValues& winningNumbers, uint8 variant = 0)
	{
		return makeNumbersWithExactMatches(winningNumbers, 2, variant);
	}
	QTFRandomValues makeK3Numbers(const QTFRandomValues& winningNumbers, uint8 variant = 0)
	{
		return makeNumbersWithExactMatches(winningNumbers, 3, variant);
	}
};

// ============================================================================
// PRIVATE METHOD TESTS
// ============================================================================

TEST(ContractQThirtyFour_Private, CountMatches_CountsOverlappingNumbers)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);

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
	primeQpiFunctionContext(qpi);

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
	primeQpiFunctionContext(qpi);

	const uint64 seed = 0x123456789ABCDEF0ULL;
	const auto out1 = ctl.state()->callGetRandomValues(qpi, seed);
	const auto out2 = ctl.state()->callGetRandomValues(qpi, seed);
	EXPECT_TRUE(valuesEqual(out1.values, out2.values));

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
	primeQpiFunctionContext(qpi);

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
	primeQpiFunctionContext(qpi);

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
	primeQpiFunctionContext(qpi);

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
	primeQpiFunctionContext(qpi);

	const uint64 P = 1000000ULL;

	// Below soft floor => nothing can be topped up.
	{
		const uint64 softFloor = smul(P, QTF_RESERVE_SOFT_FLOOR_MULT);
		const auto out = ctl.state()->callCalcReserveTopUp(qpi, softFloor - 1, 1000ULL, 1000000000ULL, P);
		EXPECT_EQ(out.topUpAmount, 0ULL);
	}

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
	primeQpiFunctionContext(qpi);

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
	primeQpiFunctionContext(qpi);

	const uint64 revenue = 1000000ULL;
	const uint64 winnersBlock = 680000ULL;

	const auto out = ctl.state()->callCalculateBaseGain(qpi, revenue, winnersBlock);
	EXPECT_EQ(out.baseGain, 118600ULL);
}

TEST(ContractQThirtyFour_Private, CalculateExtraRedirectBP_ReturnsZeroOrClampsToMax)
{
	ContractTestingQTF ctl;

	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);

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
	primeQpiProcedureContext(qpi, static_cast<uint8>(ctl.state()->getDrawHourInternal()));

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

	// Per-winner cap applies and leaves overflow.
	{
		const uint64 P = 1000000ULL;
		const uint64 cap = smul(P, QTF_TOPUP_PER_WINNER_CAP_MULT);
		const auto out = ctl.state()->callProcessTierPayout(qpi, div<uint64>(P, 2), 1, sadd(cap, 1234ULL), cap, 0, P);
		EXPECT_EQ(out.perWinnerPayout, cap);
		EXPECT_EQ(out.topUpReceived, 0ULL);
		EXPECT_EQ(out.overflow, 1234ULL);
	}
}

TEST(ContractQThirtyFour_Private, ReturnAllTickets_RefundsEachPlayerAndClearsViaSettleEpochRevenueZeroBranch)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	const id originator = id::randomValue();
	QpiContextUserProcedureCall qpi(QTF_CONTRACT_INDEX, originator, 0);
	primeQpiProcedureContext(qpi, static_cast<uint8>(ctl.state()->getDrawHourInternal()));

	// Setup a few players and refund them.
	const uint64 ticketPrice = 10;
	ctl.state()->setTicketPriceInternal(ticketPrice);

	const id p1 = id::randomValue();
	const id p2 = id::randomValue();
	const QTFRandomValues n1 = ctl.makeValidNumbers(1, 2, 3, 4);
	const QTFRandomValues n2 = ctl.makeValidNumbers(5, 6, 7, 8);
	ctl.addPlayerDirect(p1, n1);
	ctl.addPlayerDirect(p2, n2);

	increaseEnergy(ctl.qtfSelf(), ticketPrice * 2);
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
	ctl.triggerDrawTick();
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

TEST(ContractQThirtyFour, BuyTicket_ZeroPrice_RefundsAndFails)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();
	increaseEnergy(user, ticketPrice * 2);
	const uint64 balBefore = getBalance(user);

	const QTFRandomValues nums = ctl.makeValidNumbers(1, 2, 3, 4);

	const QTF::BuyTicket_output out = ctl.buyTicket(user, 0, nums);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::INVALID_TICKET_PRICE));
	EXPECT_EQ(getBalance(user), balBefore); // Fully refunded (0)
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
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>(1 + i), static_cast<uint8>(11 + i), 21, 30);

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

TEST(ContractQThirtyFour, GetState_NoneThenSelling)
{
	ContractTestingQTF ctl;

	// Initially not selling
	EXPECT_EQ(ctl.getStateInfo().currentState, static_cast<uint8>(QTF::EState::STATE_NONE));

	// After epoch start with valid time it should sell
	ctl.beginEpochWithValidTime();
	EXPECT_EQ(ctl.getStateInfo().currentState, static_cast<uint8>(QTF::EState::STATE_SELLING));
}

TEST(ContractQThirtyFour, GetPools_ReserveReflectsQRPAvailable)
{
	ContractTestingQTF ctl;

	const QTF::GetPools_output poolsBefore = ctl.getPools();
	const uint64 before = poolsBefore.pools.reserve;

	constexpr uint64 qrpFunding = 10'000'000'000ULL;
	increaseEnergy(ctl.qrpSelf(), qrpFunding);

	const QTF::GetPools_output poolsAfter = ctl.getPools();
	const uint64 after = poolsAfter.pools.reserve;

	EXPECT_GE(after, before);
	EXPECT_GT(after, 0u);
	EXPECT_LE(after, before + qrpFunding);
}

// ============================================================================
// SETTLEMENT AND PAYOUT TESTS
// ============================================================================

TEST(ContractQThirtyFour, Settlement_WithPlayers_FeesDistributed)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();
	ctl.forceFRDisabledForBaseline();

	// Fix RNG so we can deterministically avoid winners (and especially k=4).
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x1010101010101010ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const QTF::GetFees_output fees = ctl.getFees();
	constexpr uint64 numPlayers = 10;

	// Ensure RL shares exist so distribution path is exercised deterministically.
	const id shareholder1 = id::randomValue();
	const id shareholder2 = id::randomValue();
	constexpr uint32 shares1 = NUMBER_OF_COMPUTORS / 3;
	constexpr uint32 shares2 = NUMBER_OF_COMPUTORS - shares1;
	std::vector<std::pair<m256i, uint32>> rlShares{{shareholder1, shares1}, {shareholder2, shares2}};
	issueRlSharesTo(rlShares);

	// Verify FR is not active initially (baseline mode)
	EXPECT_EQ(ctl.state()->getFrActive(), false);

	// Add players
	ctl.buyRandomTickets(numPlayers, ticketPrice, nums.losing);

	const uint64 totalRevenue = ticketPrice * numPlayers;
	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 sh1Before = getBalance(shareholder1);
	const uint64 sh2Before = getBalance(shareholder2);
	const uint64 rlBefore = getBalance(id(RL_CONTRACT_INDEX, 0, 0, 0));
	const uint64 contractBalBefore = getBalance(ctl.qtfSelf());

	EXPECT_EQ(contractBalBefore, totalRevenue);

	ctl.drawWithDigest(testDigest);

	EXPECT_EQ(ctl.state()->getFrActive(), false);

	// In baseline mode (FR not active), dev receives full 10% of revenue
	// No redirects are applied
	const uint64 expectedDevFee = (totalRevenue * fees.teamFeePercent) / 100;
	EXPECT_EQ(getBalance(QTF_DEV_ADDRESS), devBalBefore + expectedDevFee)
	    << "In baseline mode, dev should receive full " << static_cast<int>(fees.teamFeePercent) << "% of revenue";

	// Distribution is paid to RL shareholders with flooring to dividendPerShare and payback remainder to RL contract.
	const uint64 expectedDistFee = (totalRevenue * fees.distributionFeePercent) / 100;
	const uint64 dividendPerShare = expectedDistFee / NUMBER_OF_COMPUTORS;
	const uint64 expectedSh1Gain = static_cast<uint64>(shares1) * dividendPerShare;
	const uint64 expectedSh2Gain = static_cast<uint64>(shares2) * dividendPerShare;
	const uint64 expectedPayback = expectedDistFee - (dividendPerShare * NUMBER_OF_COMPUTORS);
	EXPECT_EQ(getBalance(shareholder1), sh1Before + expectedSh1Gain);
	EXPECT_EQ(getBalance(shareholder2), sh2Before + expectedSh2Gain);
	EXPECT_EQ(getBalance(id(RL_CONTRACT_INDEX, 0, 0, 0)), rlBefore + expectedPayback);

	// No winners -> winnersOverflow == winnersBlock. In baseline: 50/50 split reserve/jackpot.
	const uint64 winnersBlock = (totalRevenue * fees.winnerFeePercent) / 100;
	const uint64 reserveAdd = (winnersBlock * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 expectedJackpotAdd = winnersBlock - reserveAdd;
	EXPECT_EQ(ctl.state()->getJackpot(), expectedJackpotAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qtfSelf())), expectedJackpotAdd) << "Contract balance should match carry (jackpot) after settlement";

	// Players cleared
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, Settlement_NoPlayers_NoChanges)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	const uint64 jackpotBefore = ctl.state()->getJackpot();
	const QTF::GetWinnerData_output winnersBefore = ctl.getWinnerData();

	ctl.triggerDrawTick();

	// No changes when no players
	EXPECT_EQ(ctl.state()->getJackpot(), jackpotBefore);
	const QTF::GetWinnerData_output winnersAfter = ctl.getWinnerData();
	EXPECT_EQ(winnersAfter.winnerData.winnerCounter, winnersBefore.winnerData.winnerCounter);
}

TEST(ContractQThirtyFour, Settlement_InsufficientBalance_ClearsPlayersAndAbortsSettlement)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x3030303030303030ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	constexpr uint64 numPlayers = 2;

	ctl.buyRandomTickets(numPlayers, ticketPrice, nums.losing);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numPlayers);

	// Drain the contract so CheckContractBalance() fails in SettleEpoch.
	const uint64 totalRevenue = ticketPrice * numPlayers;
	const int qtfIndex = spectrumIndex(ctl.qtfSelf());
	ASSERT_GE(qtfIndex, 0);
	ASSERT_TRUE(decreaseEnergy(qtfIndex, totalRevenue));
	EXPECT_EQ(getBalance(ctl.qtfSelf()), 0);

	ctl.drawWithDigest(testDigest);

	// Even if refunds can't be paid (because we drained balance), the contract must clear the epoch state.
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0ULL);
}

TEST(ContractQThirtyFour, Settlement_WithPlayers_FeesDistributed_FRMode)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	// Fix RNG so we can deterministically avoid winners (and especially k=4).
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x2020202020202020ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	// Activate FR mode
	ctl.state()->setJackpot(100000000ULL); // Below target
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.forceFREnabledWithinWindow(5);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const QTF::GetFees_output fees = ctl.getFees();
	constexpr uint64 numPlayers = 10;

	// Verify FR is active
	EXPECT_EQ(ctl.state()->getFrActive(), true);

	// Add players
	ctl.buyRandomTickets(numPlayers, ticketPrice, nums.losing);

	const uint64 totalRevenue = ticketPrice * numPlayers;
	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 contractBalBefore = getBalance(ctl.qtfSelf());
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	const uint64 roundsSinceK4Before = ctl.state()->getFrRoundsSinceK4();

	EXPECT_EQ(contractBalBefore, totalRevenue);

	ctl.drawWithDigest(testDigest);

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

	// No k=4 can happen (we buy losing tickets), so counter increments.
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), roundsSinceK4Before + 1);

	// Players cleared
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, Settlement_JackpotGrowsFromOverflow)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();
	ctl.forceFRDisabledForBaseline();

	// Fix RNG so we can deterministically create "no winners" tickets.
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xBADC0FFEE0DDF00DULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	constexpr uint64 numPlayers = 20;

	// Add players
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
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

	ctl.drawWithDigest(testDigest);

	// Verify jackpot growth
	const uint64 jackpotAfter = ctl.state()->getJackpot();
	const uint64 actualGrowth = jackpotAfter - jackpotBefore;

	// Deterministic: losing tickets guarantee no winners, so growth should match exactly.
	EXPECT_EQ(actualGrowth, minExpectedGrowth) << "Actual growth: " << actualGrowth << ", Expected: " << minExpectedGrowth
	                                           << ", Overflow to jackpot (50%): " << overflowToJackpot;

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
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Run several rounds without k=4 win
	for (int round = 0; round < 3; ++round)
	{
		ctl.beginEpochWithValidTime();

		ctl.buyRandomTickets(5, ticketPrice, nums.losing);

		const uint64 roundsBefore = ctl.state()->getFrRoundsSinceK4();
		ctl.drawWithDigest(testDigest);

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

	ctl.triggerDrawTick();

	// FR should be active since jackpot < target and within window
	EXPECT_EQ(ctl.state()->getFrActive(), true);
}

TEST(ContractQThirtyFour, FR_Deactivation_AfterHysteresis)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x3030303030303030ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

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

		ctl.buyRandomTickets(5, ticketPrice, nums.losing);

		// Keep jackpot at target (add back what might be paid out)
		ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT);
		ctl.drawWithDigest(testDigest);
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
	const auto nums = ctl.computeWinningAndLosing(testDigest);

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
	ctl.buyRandomTickets(numPlayers, ticketPrice, nums.losing);

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

	// Dev and Dist redirects in FR mode: base (1% each) + extra (deficit-driven)
	// First calculate base gain to pass to extra redirect calculation
	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);
	const auto baseGainOut = ctl.state()->callCalculateBaseGain(qpi, revenue, winnersBlock);

	// Calculate extra redirect based on deficit
	const uint64 delta = ctl.state()->getTargetJackpotInternal() - jackpotBefore;
	const auto extraOut = ctl.state()->callCalculateExtraRedirectBP(qpi, numPlayers, delta, revenue, baseGainOut.baseGain);

	// Total redirect BP = base + extra (split 50/50 between dev and dist)
	const uint64 devExtraBP = extraOut.extraBP / 2;
	const uint64 distExtraBP = extraOut.extraBP - devExtraBP;
	const uint64 totalDevRedirectBP = QTF_FR_DEV_REDIRECT_BP + devExtraBP;
	const uint64 totalDistRedirectBP = QTF_FR_DIST_REDIRECT_BP + distExtraBP;

	const uint64 devRedirect = (revenue * totalDevRedirectBP) / 10000;
	const uint64 distRedirect = (revenue * totalDistRedirectBP) / 10000;

	// Expected jackpot growth (with both base and extra redirects, assuming no k2/k3 winners)
	// totalJackpotContribution = overflowToJackpot + winnersRake + devRedirect + distRedirect
	const uint64 expectedGrowth = overflowToJackpot + winnersRake + devRedirect + distRedirect;

	ctl.drawWithDigest(testDigest);

	// Verify that jackpot grew by the expected amount
	const uint64 actualGrowth = ctl.state()->getJackpot() - jackpotBefore;

	// Deterministic: losing tickets guarantee no winners, so growth should match exactly.
	EXPECT_EQ(actualGrowth, expectedGrowth) << "Actual growth: " << actualGrowth << ", Expected: " << expectedGrowth
	                                        << ", Overflow to jackpot (95%): " << overflowToJackpot << ", Winners rake: " << winnersRake
	                                        << ", Extra redirect BP: " << extraOut.extraBP;

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
	ctl.startAnyDayEpoch();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// At least one ticket is required, otherwise END_EPOCH returns early and winner values are not generated.
	const id user = id::randomValue();
	ctl.fundAndBuyTicket(user, ticketPrice, ctl.makeValidNumbers(1, 2, 3, 4));

	ctl.triggerDrawTick();

	const QTF::GetWinnerData_output winnerData = ctl.getWinnerData();
	expectWinnerValuesValidAndUnique(winnerData);
}

TEST(ContractQThirtyFour, WinnerData_ResetEachRound)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Round 1: force a deterministic k=2 winner so winnerCounter becomes > 0.
	m256i digest1 = {};
	digest1.m256i_u64[0] = 0x13579BDF2468ACE0ULL;
	const auto nums1 = ctl.computeWinningAndLosing(digest1);

	ctl.beginEpochWithValidTime();
	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	QTFRandomValues k2Numbers = ctl.makeK2Numbers(nums1.winning);
	const id k2Winner = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner, ticketPrice, k2Numbers);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);

	ctl.drawWithDigest(digest1);

	const QTF::GetWinnerData_output afterRound1 = ctl.getWinnerData();
	EXPECT_GT(afterRound1.winnerData.winnerCounter, 0u);

	// Round 2: force a deterministic "no winners" round, winnerCounter must reset to 0.
	m256i digest2 = {};
	digest2.m256i_u64[0] = 0x0F0E0D0C0B0A0908ULL;
	const auto nums2 = ctl.computeWinningAndLosing(digest2);

	ctl.beginEpochWithValidTime();
	ctl.buyRandomTickets(5, ticketPrice, nums2.losing);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 5u);

	ctl.drawWithDigest(digest2);

	const QTF::GetWinnerData_output afterRound2 = ctl.getWinnerData();
	EXPECT_EQ(afterRound2.winnerData.winnerCounter, 0u) << "Winner snapshot must reset each round";
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST(ContractQThirtyFour, BuyTicket_ValidNumberSelections_EdgeCases_Success)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

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
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	uint64 prevJackpot = 0;

	// Run multiple rounds
	for (int round = 0; round < 5; ++round)
	{
		ctl.beginEpochWithValidTime();

		ctl.buyRandomTickets(10, ticketPrice, nums.losing);
		ctl.drawWithDigest(testDigest);

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

		ctl.triggerDrawTick();

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

TEST(ContractQThirtyFour, Schedule_WednesdayAlwaysDraws_IgnoresScheduleMask)
{
	ContractTestingQTF ctl;

	// Exclude Wednesday from schedule mask (e.g., Monday only).
	constexpr uint8 mondayOnly = 1 << MONDAY;
	ctl.forceSchedule(mondayOnly);

	ctl.beginEpochWithValidTime();

	const m256i testDigest = {};
	ctl.setPrevSpectrumDigest(testDigest);
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	for (int i = 0; i < 5; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
	}
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 5u);

	// Wednesday should always trigger a draw at/after draw hour, even if schedule mask does not include it.
	const uint8 drawHour = ctl.state()->getDrawHourInternal();
	ctl.setDateTime(2025, 1, 15, drawHour);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);
}

TEST(ContractQThirtyFour, Schedule_DrawOnlyOnScheduledDays)
{
	ContractTestingQTF ctl;

	// Set schedule to Wednesday only (default)
	constexpr uint8 wednesdayOnly = 1 << WEDNESDAY;
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

TEST(ContractQThirtyFour, Schedule_DrawAtMostOncePerDay_LastDrawDateStampGuards)
{
	ContractTestingQTF ctl;

	// Use a non-Wednesday scheduled day so selling is re-enabled after the draw.
	constexpr uint8 thursdayOnly = 1 << THURSDAY;
	ctl.forceSchedule(thursdayOnly);

	ctl.beginEpochWithValidTime();

	const m256i testDigest = {};
	ctl.setPrevSpectrumDigest(testDigest);
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
	}

	const uint8 drawHour = ctl.state()->getDrawHourInternal();

	// First draw on Thursday.
	ctl.setDateTime(2025, 1, 16, drawHour);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);

	const uint64 jackpotAfterFirst = ctl.state()->getJackpot();
	const QTF::GetWinnerData_output winnersAfterFirst = ctl.getWinnerData();

	// Buy another ticket on the same date (selling should be open on non-Wednesday).
	{
		const id user2 = id::randomValue();
		ctl.fundAndBuyTicket(user2, ticketPrice, nums.losing);
	}
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);

	// Second tick on the same date must NOT trigger another draw.
	ctl.setDateTime(2025, 1, 16, drawHour);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);
	EXPECT_EQ(ctl.state()->getJackpot(), jackpotAfterFirst);
	const QTF::GetWinnerData_output winnersAfterSecondAttempt = ctl.getWinnerData();
	for (uint64 i = 0; i < QTF_RANDOM_VALUES_COUNT; ++i)
	{
		EXPECT_EQ(winnersAfterSecondAttempt.winnerData.winnerValues.get(i), winnersAfterFirst.winnerData.winnerValues.get(i));
	}
	EXPECT_EQ((uint64)winnersAfterSecondAttempt.winnerData.epoch, (uint64)winnersAfterFirst.winnerData.epoch);
}

TEST(ContractQThirtyFour, DrawHour_NoDrawBeforeScheduledHour)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

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

TEST(ContractQThirtyFour, DrawHour_WednesdayDrawClosesTicketSelling)
{
	ContractTestingQTF ctl;

	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);
	ctl.beginEpochWithValidTime();

	const m256i testDigest = {};
	ctl.setPrevSpectrumDigest(testDigest);
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
	}

	const uint8 drawHour = ctl.state()->getDrawHourInternal();
	ctl.setDateTime(2025, 1, 15, drawHour);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 0u);

	// After a Wednesday draw, selling must remain closed until next epoch.
	const id lateBuyer = id::randomValue();
	increaseEnergy(lateBuyer, ticketPrice * 2);
	const uint64 before = getBalance(lateBuyer);
	const QTF::BuyTicket_output out = ctl.buyTicket(lateBuyer, ticketPrice, nums.losing);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(QTF::EReturnCode::TICKET_SELLING_CLOSED));
	EXPECT_EQ(getBalance(lateBuyer), before);
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

	ctl.startAnyDayEpoch();

	// Buy 100 tickets
	constexpr uint64 ticketPrice = 1000000ull; // 1M QU
	constexpr uint64 numTickets = 100;

	const QTFRandomValues numbers = ctl.makeValidNumbers(1, 2, 3, 4);
	ctl.buyRandomTickets(numTickets, ticketPrice, numbers);

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
	uint64 winnersBlock = 0, k2PoolExpected = 0, k3PoolExpected = 0;
	computeBaselinePrizePools(expectedRevenue, fees, winnersBlock, k2PoolExpected, k3PoolExpected);

	EXPECT_EQ(estimate.k2Pool, k2PoolExpected);
	EXPECT_EQ(estimate.k3Pool, k3PoolExpected);

	// With 1 winner each: k2 payout equals pool (below cap), k3 payout is capped at 25*P
	EXPECT_EQ(estimate.k2PayoutPerWinner, k2PoolExpected); // 19.04M < 25M cap
	EXPECT_EQ(estimate.k3PayoutPerWinner, expectedCap);    // 27.2M capped to 25M
}

TEST(ContractQThirtyFour, EstimatePrizePayouts_WithMultipleWinners)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	// Buy 1000 tickets
	const uint64 ticketPrice = 1000000ull;
	const uint64 numTickets = 1000;

	const QTFRandomValues numbers = ctl.makeValidNumbers(5, 10, 15, 20);
	ctl.buyRandomTickets(numTickets, ticketPrice, numbers);

	// Verify tickets were purchased
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numTickets);

	// Estimate for 10 k2 winners and 5 k3 winners
	QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(10, 5);

	const uint64 expectedRevenue = ticketPrice * numTickets;
	const QTF::GetFees_output fees = ctl.getFees();
	uint64 winnersBlock = 0, k2Pool = 0, k3Pool = 0;
	computeBaselinePrizePools(expectedRevenue, fees, winnersBlock, k2Pool, k3Pool);

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
	ctl.startAnyDayEpoch();

	// Buy 50 tickets
	const uint64 ticketPrice = 1000000ull;
	const uint64 numTickets = 50;

	const QTFRandomValues numbers = ctl.makeValidNumbers(7, 14, 21, 28);
	ctl.buyRandomTickets(numTickets, ticketPrice, numbers);

	// Verify tickets were purchased
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numTickets);

	// Estimate with 0 winners (shows what a single winner would get)
	QTF::EstimatePrizePayouts_output estimate = ctl.estimatePrizePayouts(0, 0);

	const uint64 expectedRevenue = ticketPrice * numTickets;
	const QTF::GetFees_output fees = ctl.getFees();
	uint64 winnersBlock = 0, k2Pool = 0, k3Pool = 0;
	computeBaselinePrizePools(expectedRevenue, fees, winnersBlock, k2Pool, k3Pool);

	// When no winners specified, should show full pool (capped)
	EXPECT_EQ(estimate.k2PayoutPerWinner, std::min(k2Pool, estimate.perWinnerCap));
	EXPECT_EQ(estimate.k3PayoutPerWinner, std::min(k3Pool, estimate.perWinnerCap));
}

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
//   2. Compute expected winning numbers for that digest
//   3. Buy tickets with exact winning numbers (for k=4), partial matches (for k=2/k=3), etc.
//   4. Trigger settlement with drawWithDigest(testDigest)
//   5. Settlement will use our fixed digest, generating the pre-computed winning numbers
//   6. Verify actual payouts, jackpot depletion, FR resets, etc.
//
// This enables deterministic testing of:
// - Actual k=4 jackpot win payouts and jackpot depletion
// - Actual k=2/k=3 winner payouts with real matching logic
// - Actual FR reset behavior after k=4 win (frRoundsSinceK4 = 0)
// - Pool splitting among multiple winners
// - Revenue distribution and fee calculations with real winners

TEST(ContractQThirtyFour, DeterministicWinner_K4JackpotWin_DepletesAndReseeds)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	// Ensure QRP has enough reserve to reseed to target.
	increaseEnergy(ctl.qrpSelf(), QTF_DEFAULT_TARGET_JACKPOT + 1000000ULL);
	const uint64 qrpBalanceBefore = static_cast<uint64>(getBalance(ctl.qrpSelf()));

	// Create a deterministic prevSpectrumDigest
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x123456789ABCDEF0ULL; // Arbitrary seed

	const auto nums = ctl.computeWinningAndLosing(testDigest);

	// Setup: FR active with jackpot below target
	const uint64 initialJackpot = 800000000ULL; // 800M QU
	ctl.state()->setJackpot(initialJackpot);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT); // 1B target
	ctl.forceFREnabledWithinWindow(10);
	// IMPORTANT: internal `state.jackpot` must be backed by actual contract balance, otherwise transfers will fail.
	increaseEnergy(ctl.qtfSelf(), initialJackpot);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// User1: Buy ticket with EXACT winning numbers (k=4 winner)
	const id k4Winner = id::randomValue();
	ctl.fundAndBuyTicket(k4Winner, ticketPrice, nums.winning);

	// User2: Buy ticket with 3 matching numbers (k=3 winner)
	QTFRandomValues k3Numbers = ctl.makeK3Numbers(nums.winning);
	const id k3Winner = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner, ticketPrice, k3Numbers);

	// User3: Buy ticket with 2 matching numbers (k=2 winner)
	QTFRandomValues k2Numbers = ctl.makeK2Numbers(nums.winning);
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

	// Trigger settlement using our fixed prevSpectrumDigest
	const uint64 k4WinnerBefore = getBalance(k4Winner);
	ctl.drawWithDigest(testDigest);
	const uint64 k4WinnerAfter = getBalance(k4Winner);

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
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(0), nums.winning.get(0));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(1), nums.winning.get(1));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(2), nums.winning.get(2));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(3), nums.winning.get(3));

	// Verify k=4 winner received exact payout (jackpotBefore / countK4).
	EXPECT_EQ(static_cast<uint64>(k4WinnerAfter - k4WinnerBefore), initialJackpot);
}

TEST(ContractQThirtyFour, DeterministicWinner_K4JackpotWin_MultipleWinners_SplitsEvenly)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	// Ensure QRP has enough reserve to reseed (so settlement completes without relying on carry math).
	increaseEnergy(ctl.qrpSelf(), QTF_DEFAULT_TARGET_JACKPOT + 1000000ULL);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xA5A5A5A5A5A5A5A5ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 initialJackpot = 900000000ULL;
	ctl.state()->setJackpot(initialJackpot);
	ctl.forceFREnabledWithinWindow(1);
	increaseEnergy(ctl.qtfSelf(), initialJackpot);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	const id w1 = id::randomValue();
	const id w2 = id::randomValue();
	ctl.fundAndBuyTicket(w1, ticketPrice, nums.winning);
	ctl.fundAndBuyTicket(w2, ticketPrice, nums.winning);

	const uint64 w1Before = getBalance(w1);
	const uint64 w2Before = getBalance(w2);

	ctl.drawWithDigest(testDigest);

	const uint64 expectedPerWinner = initialJackpot / 2;
	EXPECT_EQ(static_cast<uint64>(getBalance(w1) - w1Before), expectedPerWinner);
	EXPECT_EQ(static_cast<uint64>(getBalance(w2) - w2Before), expectedPerWinner);
}

TEST(ContractQThirtyFour, DeterministicWinner_K4JackpotWin_ReseedLimitedByQRP)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();
	ctl.forceFRDisabledForBaseline();

	// Fund QRP below target so reseed amount is limited by available reserve.
	const uint64 qrpFunded = 200000000ULL;
	increaseEnergy(ctl.qrpSelf(), qrpFunded);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x0A0B0C0D0E0F1011ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 initialJackpot = 800000000ULL;
	ctl.state()->setJackpot(initialJackpot);
	increaseEnergy(ctl.qtfSelf(), initialJackpot);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id w1 = id::randomValue();
	ctl.fundAndBuyTicket(w1, ticketPrice, nums.winning);

	const uint64 qrpBefore = static_cast<uint64>(getBalance(ctl.qrpSelf()));
	const uint64 w1Before = getBalance(w1);

	ctl.drawWithDigest(testDigest);

	EXPECT_EQ(static_cast<uint64>(getBalance(w1) - w1Before), initialJackpot);

	// With a single winning ticket and baseline overflow split, winnersOverflow == winnersBlock, reserveAdd == winnersBlock/2, carryAdd ==
	// winnersBlock/2.
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 revenue = ticketPrice;
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;
	const uint64 reserveAdd = (winnersBlock * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 carryAdd = winnersBlock - reserveAdd;

	EXPECT_EQ(ctl.state()->getJackpot(), qrpFunded + carryAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qrpSelf())), qrpBefore - qrpFunded + reserveAdd);
}

// Test k=2 and k=3 payouts with deterministic winning numbers
TEST(ContractQThirtyFour, DeterministicWinner_K2K3Payouts_VerifyRevenueSplit)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	// This test validates baseline k2/k3 pool splitting (no FR rake).
	// Force FR activation window to be expired so SettleEpoch cannot auto-enable FR.
	ctl.forceFRDisabledForBaseline();

	// Create deterministic prevSpectrumDigest
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xFEDCBA9876543210ULL; // Different seed

	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Create multiple k=2 and k=3 winners to test pool splitting
	// 2 k=3 winners
	QTFRandomValues k3Numbers1 = ctl.makeK3Numbers(nums.winning, 0);
	const id k3Winner1 = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner1, ticketPrice, k3Numbers1);

	QTFRandomValues k3Numbers2 = ctl.makeK3Numbers(nums.winning, 1);
	const id k3Winner2 = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner2, ticketPrice, k3Numbers2);

	// 3 k=2 winners
	QTFRandomValues k2Numbers1 = ctl.makeK2Numbers(nums.winning, 0);
	const id k2Winner1 = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner1, ticketPrice, k2Numbers1);

	QTFRandomValues k2Numbers2 = ctl.makeK2Numbers(nums.winning, 1);
	const id k2Winner2 = id::randomValue();
	ctl.fundAndBuyTicket(k2Winner2, ticketPrice, k2Numbers2);

	QTFRandomValues k2Numbers3 = ctl.makeK2Numbers(nums.winning, 2);
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

	// Get balances before settlement
	const uint64 k3Winner1Before = getBalance(k3Winner1);
	const uint64 k2Winner1Before = getBalance(k2Winner1);

	// Trigger settlement
	ctl.drawWithDigest(testDigest);

	// Verify winner payouts
	// k=3 pool split between 2 winners
	const uint64 expectedK3PayoutPerWinner = expectedK3Pool / 2;
	const uint64 k3Winner1After = getBalance(k3Winner1);
	const uint64 k3Winner1Gained = k3Winner1After - k3Winner1Before;
	EXPECT_EQ(static_cast<uint64>(k3Winner1Gained), expectedK3PayoutPerWinner) << "k=3 winner should receive half of k3 pool";

	// k=2 pool split between 3 winners
	const uint64 expectedK2PayoutPerWinner = expectedK2Pool / 3;
	const uint64 k2Winner1After = getBalance(k2Winner1);
	const uint64 k2Winner1Gained = k2Winner1After - k2Winner1Before;
	EXPECT_EQ(static_cast<uint64>(k2Winner1Gained), expectedK2PayoutPerWinner) << "k=2 winner should receive one-third of k2 pool";

	// Verify winning numbers in winner data
	QTF::GetWinnerData_output winnerData = ctl.getWinnerData();
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(0), nums.winning.get(0));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(1), nums.winning.get(1));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(2), nums.winning.get(2));
	EXPECT_EQ(winnerData.winnerData.winnerValues.get(3), nums.winning.get(3));

	// Jackpot should have grown (no k=4 winner)
	EXPECT_GT(ctl.state()->getJackpot(), 0ULL);
}

TEST(ContractQThirtyFour, EstimatePrizePayouts_FRMode_AppliesRakeToPools)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Enable FR so EstimatePrizePayouts applies the 5% winners rake.
	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT / 2);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.forceFREnabledWithinWindow(1);

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

TEST(ContractQThirtyFour, Settlement_PerWinnerCap_AppliesToK3Winner_OverflowAccountsForRemainder)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();
	ctl.forceFRDisabledForBaseline();

	// Ensure RL shares exist so distribution payouts leave the contract (otherwise most of distPayout can remain in QTF balance).
	const id shareholder1 = id::randomValue();
	const id shareholder2 = id::randomValue();
	constexpr uint32 shares1 = NUMBER_OF_COMPUTORS / 3;
	constexpr uint32 shares2 = NUMBER_OF_COMPUTORS - shares1;
	std::vector<std::pair<m256i, uint32>> rlShares{{shareholder1, shares1}, {shareholder2, shares2}};
	issueRlSharesTo(rlShares);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xD1CEB00BD1CEB00BULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 P = ctl.state()->getTicketPriceInternal();
	const uint64 perWinnerCap = smul(P, QTF_TOPUP_PER_WINNER_CAP_MULT);

	const id k3Winner = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner, P, ctl.makeK3Numbers(nums.winning, 0));

	constexpr uint64 numLosers = 100;
	ctl.buyRandomTickets(numLosers, P, nums.losing);
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), numLosers + 1);

	const uint64 qrpBefore = static_cast<uint64>(getBalance(ctl.qrpSelf()));
	const uint64 k3Before = getBalance(k3Winner);

	ctl.drawWithDigest(testDigest);

	EXPECT_EQ(static_cast<uint64>(getBalance(k3Winner) - k3Before), perWinnerCap);

	// Baseline settlement: with no k2 winners and exactly one k3 winner capped at 25*P,
	// winnersOverflow ends up being winnersBlock - perWinnerCap.
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 revenue = smul(P, numLosers + 1);
	const uint64 winnersBlock = div<uint64>(smul(revenue, static_cast<uint64>(fees.winnerFeePercent)), 100);
	const uint64 winnersOverflow = winnersBlock - perWinnerCap;
	const uint64 reserveAdd = (winnersOverflow * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 carryAdd = winnersOverflow - reserveAdd;

	EXPECT_EQ(ctl.state()->getJackpot(), carryAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qtfSelf())), carryAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qrpSelf())), qrpBefore + reserveAdd);
}

TEST(ContractQThirtyFour, Settlement_FloorTopUp_LimitedBySafetyCaps_PayoutBelowFloor)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();
	ctl.forceFRDisabledForBaseline();

	// Ensure RL shares exist so distribution payouts leave the contract (otherwise most of distPayout can remain in QTF balance).
	const id shareholder1 = id::randomValue();
	const id shareholder2 = id::randomValue();
	constexpr uint32 shares1 = NUMBER_OF_COMPUTORS / 2;
	constexpr uint32 shares2 = NUMBER_OF_COMPUTORS - shares1;
	std::vector<std::pair<m256i, uint32>> rlShares{{shareholder1, shares1}, {shareholder2, shares2}};
	issueRlSharesTo(rlShares);

	// Fund QRP just above soft floor so top-up is limited by both 10% cap and soft floor.
	const uint64 P = ctl.state()->getTicketPriceInternal();
	const uint64 softFloor = smul(P, QTF_RESERVE_SOFT_FLOOR_MULT); // 20*P
	const uint64 qrpFunding = softFloor + 5 * P;                  // 25*P
	increaseEnergy(ctl.qrpSelf(), qrpFunding);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x0DDC0FFEE0DDF00DULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const id k3Winner = id::randomValue();
	ctl.fundAndBuyTicket(k3Winner, P, ctl.makeK3Numbers(nums.winning, 0));
	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);

	const uint64 qrpBefore = static_cast<uint64>(getBalance(ctl.qrpSelf()));
	const uint64 k3Before = getBalance(k3Winner);

	ctl.drawWithDigest(testDigest);

	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 revenue = P;
	const uint64 winnersBlock = div<uint64>(smul(revenue, static_cast<uint64>(fees.winnerFeePercent)), 100);
	const uint64 k3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;
	const uint64 k3Floor = smul(P, QTF_K3_FLOOR_MULT);
	const uint64 needed = k3Floor - k3Pool;
	const uint64 availableAboveFloor = qrpBefore - softFloor;                     // 5*P
	const uint64 maxPerRound = (qrpBefore * QTF_TOPUP_RESERVE_PCT_BP) / 10000;    // 10% of total
	const uint64 perWinnerCapTotal = smul(P, QTF_TOPUP_PER_WINNER_CAP_MULT);      // 25*P
	const uint64 maxAllowed = std::min(std::min(maxPerRound, availableAboveFloor), perWinnerCapTotal); // 2.5*P
	const uint64 expectedTopUp = std::min(needed, maxAllowed);
	const uint64 expectedPayout = k3Pool + expectedTopUp;

	EXPECT_LT(expectedPayout, k3Floor);
	EXPECT_EQ(static_cast<uint64>(getBalance(k3Winner) - k3Before), expectedPayout);

	// With no k2 winners and k3 pool fully paid (top-ups only increase payouts),
	// winnersOverflow equals winnersBlock - k3Pool.
	const uint64 winnersOverflow = winnersBlock - k3Pool;
	const uint64 reserveAdd = (winnersOverflow * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 carryAdd = winnersOverflow - reserveAdd;

	EXPECT_EQ(ctl.state()->getJackpot(), carryAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qtfSelf())), carryAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qrpSelf())), qrpBefore - expectedTopUp + reserveAdd);
	EXPECT_GE(static_cast<uint64>(getBalance(ctl.qrpSelf())), softFloor);
}

TEST(ContractQThirtyFour, Settlement_FloorTopUp_Integration_K2K3FloorsMetWhenReserveSufficient)
{
	ContractTestingQTF ctl;
	ctl.startAnyDayEpoch();
	ctl.forceFRDisabledForBaseline();

	// Ensure RL shares exist so distribution path is exercised (and rounding/payback is deterministic).
	const id shareholder1 = id::randomValue();
	const id shareholder2 = id::randomValue();
	constexpr uint32 shares1 = NUMBER_OF_COMPUTORS / 4;
	constexpr uint32 shares2 = NUMBER_OF_COMPUTORS - shares1;
	std::vector<std::pair<m256i, uint32>> rlShares{{shareholder1, shares1}, {shareholder2, shares2}};
	issueRlSharesTo(rlShares);

	// Fund QRP enough so both tiers can be topped up to floors under all caps.
	const uint64 qrpFunding = 100000000ULL; // 100M, 10% cap = 10M, soft floor = 20M.
	increaseEnergy(ctl.qrpSelf(), qrpFunding);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x5566778899AABBCCULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	const uint64 P = ctl.state()->getTicketPriceInternal();

	// Create deterministic winners: 2x k2 winners and 1x k3 winner => pools are small and must be topped up.
	const id k2w1 = id::randomValue();
	const id k2w2 = id::randomValue();
	const id k3w1 = id::randomValue();
	ctl.fundAndBuyTicket(k2w1, P, ctl.makeK2Numbers(nums.winning, 0));
	ctl.fundAndBuyTicket(k2w2, P, ctl.makeK2Numbers(nums.winning, 1));
	ctl.fundAndBuyTicket(k3w1, P, ctl.makeK3Numbers(nums.winning, 2));

	const uint64 qrpBefore = static_cast<uint64>(getBalance(ctl.qrpSelf()));
	const uint64 qtfBefore = static_cast<uint64>(getBalance(ctl.qtfSelf()));
	const uint64 k2w1Before = getBalance(k2w1);
	const uint64 k3w1Before = getBalance(k3w1);
	const uint64 sh1Before = getBalance(shareholder1);
	const uint64 sh2Before = getBalance(shareholder2);
	const uint64 rlBefore = getBalance(id(RL_CONTRACT_INDEX, 0, 0, 0));

	EXPECT_EQ(qtfBefore, 3 * P);

	ctl.drawWithDigest(testDigest);

	// Expected pools and top-ups.
	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 revenue = 3 * P;
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;
	const uint64 k2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 k3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;

	const uint64 k2Floor = P / 2;
	const uint64 k3Floor = 5 * P;
	const uint64 k2TopUp = (k2Floor * 2 > k2Pool) ? (k2Floor * 2 - k2Pool) : 0;
	const uint64 k3TopUp = (k3Floor > k3Pool) ? (k3Floor - k3Pool) : 0;

	// Winners must receive the floors (no per-winner cap binding in this scenario).
	EXPECT_EQ(static_cast<uint64>(getBalance(k2w1) - k2w1Before), k2Floor);
	EXPECT_EQ(static_cast<uint64>(getBalance(k3w1) - k3w1Before), k3Floor);

	// Baseline overflow is the unallocated 32% of winnersBlock (tier pools are fully paid out with floor top-ups, so no extra overflow).
	const uint64 winnersOverflow = winnersBlock - k2Pool - k3Pool;
	const uint64 reserveAdd = (winnersOverflow * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 carryAdd = winnersOverflow - reserveAdd;

	// Contract balance should match carry (jackpot) after settlement.
	EXPECT_EQ(ctl.state()->getJackpot(), carryAdd);
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qtfSelf())), carryAdd);

	// QRP: receives reserveAdd, pays out top-ups.
	EXPECT_EQ(static_cast<uint64>(getBalance(ctl.qrpSelf())), qrpBefore - k2TopUp - k3TopUp + reserveAdd);

	// Distribution: verify two holders and RL payback remainder.
	const uint64 expectedDistFee = (revenue * fees.distributionFeePercent) / 100;
	const uint64 dividendPerShare = expectedDistFee / NUMBER_OF_COMPUTORS;
	const uint64 expectedSh1Gain = static_cast<uint64>(shares1) * dividendPerShare;
	const uint64 expectedSh2Gain = static_cast<uint64>(shares2) * dividendPerShare;
	const uint64 expectedPayback = expectedDistFee - (dividendPerShare * NUMBER_OF_COMPUTORS);
	EXPECT_EQ(getBalance(shareholder1), sh1Before + expectedSh1Gain);
	EXPECT_EQ(getBalance(shareholder2), sh2Before + expectedSh2Gain);
	EXPECT_EQ(getBalance(id(RL_CONTRACT_INDEX, 0, 0, 0)), rlBefore + expectedPayback);
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
	const auto nums = ctl.computeWinningAndLosing(testDigest);

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
	ctl.buyRandomTickets(numPlayers, ticketPrice, nums.losing);

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

	ctl.drawWithDigest(testDigest);

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

TEST(ContractQThirtyFour, Settlement_FRMode_ExtraRedirect_ClampsToMax_AndAffectsDevAndDist)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	// Ensure RL shares exist so distribution can be asserted.
	const id shareholder1 = id::randomValue();
	const id shareholder2 = id::randomValue();
	constexpr uint32 shares1 = NUMBER_OF_COMPUTORS / 2;
	constexpr uint32 shares2 = NUMBER_OF_COMPUTORS - shares1;
	std::vector<std::pair<m256i, uint32>> rlShares{{shareholder1, shares1}, {shareholder2, shares2}};
	issueRlSharesTo(rlShares);

	// Deterministic no-winner tickets.
	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0x7777777777777777ULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	// Force FR on and create an extreme deficit to guarantee extra redirect clamps to max.
	ctl.state()->setJackpot(0ULL);
	ctl.state()->setTargetJackpotInternal(1000000000000000ULL); // 1e15
	ctl.state()->setFrActive(true);
	ctl.state()->setFrRoundsSinceK4(1);

	ctl.beginEpochWithValidTime();

	const uint64 P = ctl.state()->getTicketPriceInternal();
	constexpr uint64 numPlayers = 10;
	ctl.buyRandomTickets(numPlayers, P, nums.losing);

	const QTF::GetFees_output fees = ctl.getFees();
	const uint64 revenue = P * numPlayers;

	const uint64 devBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 sh1Before = getBalance(shareholder1);
	const uint64 sh2Before = getBalance(shareholder2);
	const uint64 rlBefore = getBalance(id(RL_CONTRACT_INDEX, 0, 0, 0));

	// Pre-compute expected extra BP using the same private helpers as the contract.
	QpiContextUserFunctionCall qpi(QTF_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);
	const auto pools = ctl.state()->callCalculatePrizePools(qpi, revenue, true);
	const auto baseGainOut = ctl.state()->callCalculateBaseGain(qpi, revenue, pools.winnersBlock);
	const uint64 delta = ctl.state()->getTargetJackpotInternal() - ctl.state()->getJackpot();
	const auto extraOut = ctl.state()->callCalculateExtraRedirectBP(qpi, numPlayers, delta, revenue, baseGainOut.baseGain);
	ASSERT_EQ(extraOut.extraBP, QTF_FR_EXTRA_MAX_BP);

	const uint64 devExtraBP = extraOut.extraBP / 2;
	const uint64 distExtraBP = extraOut.extraBP - devExtraBP;
	const uint64 totalDevRedirectBP = QTF_FR_DEV_REDIRECT_BP + devExtraBP;
	const uint64 totalDistRedirectBP = QTF_FR_DIST_REDIRECT_BP + distExtraBP;

	const uint64 fullDevFee = (revenue * fees.teamFeePercent) / 100;
	const uint64 fullDistFee = (revenue * fees.distributionFeePercent) / 100;

	const uint64 expectedDevRedirect = (revenue * totalDevRedirectBP) / 10000;
	const uint64 expectedDistRedirect = (revenue * totalDistRedirectBP) / 10000;
	const uint64 expectedDevPayout = fullDevFee - expectedDevRedirect;
	const uint64 expectedDistPayout = fullDistFee - expectedDistRedirect;

	ctl.drawWithDigest(testDigest);

	// Dev payout must match exact base+extra redirect math (no caps expected in this scenario).
	EXPECT_EQ(static_cast<uint64>(getBalance(QTF_DEV_ADDRESS) - devBefore), expectedDevPayout);

	// Distribution must match expectedDistPayout (dividendPerShare flooring + payback).
	const uint64 dividendPerShare = expectedDistPayout / NUMBER_OF_COMPUTORS;
	const uint64 expectedSh1Gain = static_cast<uint64>(shares1) * dividendPerShare;
	const uint64 expectedSh2Gain = static_cast<uint64>(shares2) * dividendPerShare;
	const uint64 expectedPayback = expectedDistPayout - (dividendPerShare * NUMBER_OF_COMPUTORS);
	EXPECT_EQ(getBalance(shareholder1), sh1Before + expectedSh1Gain);
	EXPECT_EQ(getBalance(shareholder2), sh2Before + expectedSh2Gain);
	EXPECT_EQ(getBalance(id(RL_CONTRACT_INDEX, 0, 0, 0)), rlBefore + expectedPayback);
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
	const auto nums = ctl.computeWinningAndLosing(testDigest);

	// Setup: Jackpot below target, but window expired and FR inactive.
	ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT / 2);
	ctl.state()->setTargetJackpotInternal(QTF_DEFAULT_TARGET_JACKPOT);
	ctl.state()->setFrActive(false);
	ctl.state()->setFrRoundsSinceK4(QTF_FR_POST_K4_WINDOW_ROUNDS);

	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	constexpr int numPlayers = 10;
	ctl.buyRandomTickets(numPlayers, ticketPrice, nums.losing);

	ctl.drawWithDigest(testDigest);

	EXPECT_EQ(ctl.state()->getFrActive(), false);
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS + 1);
}

TEST(ContractQThirtyFour, FR_PostK4WindowExpiry_DoesNotReactivateWhenWindowExpired)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	m256i testDigest = {};
	testDigest.m256i_u64[0] = 0xFACEFEEDFACEFEEDULL;
	const auto nums = ctl.computeWinningAndLosing(testDigest);

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
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
	}

	// Verify FR is active before settlement
	EXPECT_EQ(ctl.state()->getFrActive(), true);
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS - 1);
	EXPECT_LT(ctl.state()->getJackpot(), ctl.state()->getTargetJackpotInternal());

	ctl.drawWithDigest(testDigest);

	// After settlement (deterministic: no k=4 win is possible):
	// - roundsSinceK4 should increment to 50
	// - Next round starts outside the FR post-k4 window.

	const uint64 roundsSinceK4After = ctl.state()->getFrRoundsSinceK4();
	EXPECT_EQ(roundsSinceK4After, QTF_FR_POST_K4_WINDOW_ROUNDS) << "Counter should increment to 50 after draw";

	// Run one more round: FR must be OFF because roundsSinceK4 >= 50.
	ctl.beginEpochWithValidTime();

	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
	}

	ctl.drawWithDigest(testDigest);

	// After second round:
	// - Jackpot still below target
	// - roundsSinceK4 = 51 (>= 50)
	// - FR is forced OFF outside the window.
	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS + 1);
	EXPECT_EQ(ctl.state()->getFrActive(), false);

	// Run a third round to ensure FR stays OFF while still outside the window.
	ctl.beginEpochWithValidTime();
	for (int i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		ctl.fundAndBuyTicket(user, ticketPrice, nums.losing);
	}
	ctl.drawWithDigest(testDigest);

	EXPECT_EQ(ctl.state()->getFrRoundsSinceK4(), QTF_FR_POST_K4_WINDOW_ROUNDS + 2);
	EXPECT_EQ(ctl.state()->getFrActive(), false);
}
