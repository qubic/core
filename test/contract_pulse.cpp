#define NO_UEFI
#define _ALLOW_KEYWORD_MACROS 1
// Allow tests to call internal helpers without changing production visibility.
#define private protected
#include "contract_testing.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include <vector>

// Procedure/function indices (must match REGISTER_USER_FUNCTIONS_AND_PROCEDURES in `src/contracts/Pulse.h`).
constexpr uint16 PULSE_PROCEDURE_BUY_TICKET = 1;
constexpr uint16 PULSE_PROCEDURE_SET_PRICE = 2;
constexpr uint16 PULSE_PROCEDURE_SET_SCHEDULE = 3;
constexpr uint16 PULSE_PROCEDURE_SET_DRAW_HOUR = 4;
constexpr uint16 PULSE_PROCEDURE_SET_FEES = 5;
constexpr uint16 PULSE_PROCEDURE_SET_QHEART_HOLD_LIMIT = 6;
constexpr uint16 PULSE_PROCEDURE_BUY_RANDOM_TICKETS = 7;
constexpr uint16 PULSE_PROCEDURE_DEPOSIT_AUTO_PARTICIPATION = 8;
constexpr uint16 PULSE_PROCEDURE_WITHDRAW_AUTO_PARTICIPATION = 9;
constexpr uint16 PULSE_PROCEDURE_SET_AUTO_CONFIG = 10;
constexpr uint16 PULSE_PROCEDURE_SET_AUTO_LIMITS = 11;

constexpr uint16 PULSE_FUNCTION_GET_TICKET_PRICE = 1;
constexpr uint16 PULSE_FUNCTION_GET_SCHEDULE = 2;
constexpr uint16 PULSE_FUNCTION_GET_DRAW_HOUR = 3;
constexpr uint16 PULSE_FUNCTION_GET_FEES = 4;
constexpr uint16 PULSE_FUNCTION_GET_QHEART_HOLD_LIMIT = 5;
constexpr uint16 PULSE_FUNCTION_GET_QHEART_WALLET = 6;
constexpr uint16 PULSE_FUNCTION_GET_WINNING_DIGITS = 7;
constexpr uint16 PULSE_FUNCTION_GET_BALANCE = 8;
constexpr uint16 PULSE_FUNCTION_GET_WINNERS = 9;
constexpr uint16 PULSE_FUNCTION_GET_AUTO_PARTICIPATION = 10;
constexpr uint16 PULSE_FUNCTION_GET_AUTO_STATS = 11;

namespace
{
	// QPI contexts must be primed with a call to satisfy internal checks.
	void primeQpiFunctionContext(QpiContextUserFunctionCall& qpi)
	{
		PULSE::GetTicketPrice_input input{};
		qpi.call(PULSE_FUNCTION_GET_TICKET_PRICE, &input, sizeof(input));
	}

	// Use a safe call to seed procedure context for private calls.
	void primeQpiProcedureContext(QpiContextUserProcedureCall& qpi)
	{
		PULSE::SetDrawHour_input input{};
		input.newDrawHour = PULSE_DEFAULT_DRAW_HOUR;
		qpi.call(PULSE_PROCEDURE_SET_DRAW_HOUR, &input, sizeof(input));
		ASSERT_EQ(contractError[PULSE_CONTRACT_INDEX], 0);
	}

	Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> makePlayerDigits(uint8 d0, uint8 d1, uint8 d2, uint8 d3, uint8 d4, uint8 d5)
	{
		Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits = {};
		digits.set(0, d0);
		digits.set(1, d1);
		digits.set(2, d2);
		digits.set(3, d3);
		digits.set(4, d4);
		digits.set(5, d5);
		return digits;
	}

	void expectWinningDigitsInRange(const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& digits)
	{
		for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
		{
			const uint8 v = digits.get(i);
			EXPECT_LE(v, PULSE_MAX_DIGIT);
		}
	}
} // namespace

// Test helper class exposing internal state
class PULSEChecker : public PULSE
{
public:
	uint64 getTicketCounter() const { return ticketCounter; }
	uint64 getTicketPriceInternal() const { return ticketPrice; }
	uint64 getQHeartHoldLimitInternal() const { return qheartHoldLimit; }
	uint32 getLastDrawDateStamp() const { return lastDrawDateStamp; }
	uint8 getScheduleInternal() const { return schedule; }
	uint8 getDrawHourInternal() const { return drawHour; }
	uint8 getDevPercentInternal() const { return devPercent; }
	uint8 getBurnPercentInternal() const { return burnPercent; }
	uint8 getShareholdersPercentInternal() const { return shareholdersPercent; }
	uint8 getQHeartPercentInternal() const { return qheartPercent; }
	const id& getTeamAddressInternal() const { return teamAddress; }
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& getLastWinningDigits() const { return lastWinningDigits; }
	Ticket getTicket(uint64 index) const { return tickets.get(index); }

	void setTicketCounter(uint64 value) { ticketCounter = value; }
	void setTicketPriceInternal(uint64 value) { ticketPrice = value; }
	void setLastDrawDateStamp(uint32 value) { lastDrawDateStamp = value; }
	void setScheduleInternal(uint8 value) { schedule = value; }
	void setDrawHourInternal(uint8 value) { drawHour = value; }

	NextEpochData& nextEpochDataRef() { return nextEpochData; }

	void setTicketDirect(uint64 index, const id& player, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits)
	{
		Ticket ticket;
		ticket.player = player;
		ticket.digits = digits;
		tickets.set(index, ticket);
	}

	void forceSelling(bool enable) { enableBuyTicket(*this, enable); }
	bool isSelling() const { return isSellingOpen(*this); }

	ValidateDigits_output callValidateDigits(const QPI::QpiContextFunctionCall& qpi, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits) const
	{
		ValidateDigits_input input{};
		ValidateDigits_output output{};
		ValidateDigits_locals locals{};
		input.digits = digits;
		ValidateDigits(qpi, *this, input, output, locals);
		return output;
	}

	GetRandomDigits_output callGetRandomDigits(const QPI::QpiContextFunctionCall& qpi, uint64 seed) const
	{
		GetRandomDigits_input input{};
		GetRandomDigits_output output{};
		GetRandomDigits_locals locals{};
		input.seed = seed;
		GetRandomDigits(qpi, *this, input, output, locals);
		return output;
	}

	PrepareRandomTickets_output callPrepareRandomTickets(const QPI::QpiContextProcedureCall& qpi, uint16 count)
	{
		PrepareRandomTickets_input input{};
		PrepareRandomTickets_output output{};
		PrepareRandomTickets_locals locals{};
		input.count = count;
		PrepareRandomTickets(qpi, *this, input, output, locals);
		return output;
	}

	ChargeTicketsFromPlayer_output callChargeTicketsFromPlayer(const QPI::QpiContextProcedureCall& qpi, const id& player, uint16 count)
	{
		ChargeTicketsFromPlayer_input input{};
		ChargeTicketsFromPlayer_output output{};
		ChargeTicketsFromPlayer_locals locals{};
		input.player = player;
		input.count = count;
		ChargeTicketsFromPlayer(qpi, *this, input, output, locals);
		return output;
	}

	AllocateRandomTickets_output callAllocateRandomTickets(const QPI::QpiContextProcedureCall& qpi, const id& player, uint16 count)
	{
		AllocateRandomTickets_input input{};
		AllocateRandomTickets_output output{};
		AllocateRandomTickets_locals locals{};
		input.player = player;
		input.count = count;
		AllocateRandomTickets(qpi, *this, input, output, locals);
		return output;
	}

	void callProcessAutoTickets(const QPI::QpiContextProcedureCall& qpi)
	{
		ProcessAutoTickets_input input{};
		ProcessAutoTickets_output output{};
		ProcessAutoTickets_locals locals{};
		ProcessAutoTickets(qpi, *this, input, output, locals);
	}

	GetAutoParticipation_output callGetAutoParticipation(const QPI::QpiContextFunctionCall& qpi) const
	{
		GetAutoParticipation_input input{};
		GetAutoParticipation_output output{};
		GetAutoParticipation_locals locals{};
		GetAutoParticipation(qpi, *this, input, output, locals);
		return output;
	}

	void setAutoParticipant(const id& player, sint64 deposit, uint16 desiredTickets)
	{
		AutoParticipant entry{};
		entry.player = player;
		entry.deposit = deposit;
		entry.desiredTickets = desiredTickets;
		autoParticipants.set(player, entry);
	}

	uint64 callGetLeftAlignedReward(uint8 matches) const { return getLeftAlignedReward(*this, matches); }
	uint64 callGetAnyPositionReward(uint8 matches) const { return getAnyPositionReward(*this, matches); }
	uint64 callComputePrize(const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits)
	{
		Ticket ticket{};
		ticket.digits = digits;
		ComputePrize_locals locals{};
		return computePrize(*this, ticket, winning, locals);
	}
};

class ContractTestingPulse : protected ContractTesting
{
public:
	ContractTestingPulse()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(PULSE);
		system.epoch = contractDescriptions[PULSE_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(PULSE_CONTRACT_INDEX, INITIALIZE);
	}

	PULSEChecker* state() { return reinterpret_cast<PULSEChecker*>(contractStates[PULSE_CONTRACT_INDEX]); }
	id pulseSelf() const { return id(PULSE_CONTRACT_INDEX, 0, 0, 0); }

	PULSE::GetTicketPrice_output getTicketPrice()
	{
		PULSE::GetTicketPrice_input input{};
		PULSE::GetTicketPrice_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_TICKET_PRICE, input, output);
		return output;
	}

	PULSE::GetSchedule_output getSchedule()
	{
		PULSE::GetSchedule_input input{};
		PULSE::GetSchedule_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_SCHEDULE, input, output);
		return output;
	}

	PULSE::GetDrawHour_output getDrawHour()
	{
		PULSE::GetDrawHour_input input{};
		PULSE::GetDrawHour_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_DRAW_HOUR, input, output);
		return output;
	}

	PULSE::GetFees_output getFees()
	{
		PULSE::GetFees_input input{};
		PULSE::GetFees_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_FEES, input, output);
		return output;
	}

	PULSE::GetQHeartHoldLimit_output getQHeartHoldLimit()
	{
		PULSE::GetQHeartHoldLimit_input input{};
		PULSE::GetQHeartHoldLimit_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_QHEART_HOLD_LIMIT, input, output);
		return output;
	}

	PULSE::GetQHeartWallet_output getQHeartWallet()
	{
		PULSE::GetQHeartWallet_input input{};
		PULSE::GetQHeartWallet_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_QHEART_WALLET, input, output);
		return output;
	}

	PULSE::GetWinningDigits_output getWinningDigits()
	{
		PULSE::GetWinningDigits_input input{};
		PULSE::GetWinningDigits_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_WINNING_DIGITS, input, output);
		return output;
	}

	PULSE::GetBalance_output getBalance()
	{
		PULSE::GetBalance_input input{};
		PULSE::GetBalance_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_BALANCE, input, output);
		return output;
	}

	PULSE::GetWinners_output getWinners()
	{
		PULSE::GetWinners_input input{};
		PULSE::GetWinners_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_WINNERS, input, output);
		return output;
	}

	PULSE::GetAutoParticipation_output getAutoParticipation(const id& user)
	{
		QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, user, 0);
		return state()->callGetAutoParticipation(qpi);
	}

	PULSE::GetAutoStats_output getAutoStats()
	{
		PULSE::GetAutoStats_input input{};
		PULSE::GetAutoStats_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_AUTO_STATS, input, output);
		return output;
	}

	PULSE::BuyTicket_output buyTicket(const id& user, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits)
	{
		ensureUserEnergy(user);
		PULSE::BuyTicket_input input{};
		input.digits = digits;
		PULSE::BuyTicket_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_BUY_TICKET, input, output, user, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::BuyRandomTickets_output buyRandomTickets(const id& user, uint16 count)
	{
		ensureUserEnergy(user);
		PULSE::BuyRandomTickets_input input{};
		input.count = count;
		PULSE::BuyRandomTickets_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_BUY_RANDOM_TICKETS, input, output, user, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::DepositAutoParticipation_output depositAutoParticipation(const id& user, sint64 amount, sint16 desiredTickets, bool buyNow)
	{
		ensureUserEnergy(user);
		PULSE::DepositAutoParticipation_input input{};
		input.amount = amount;
		input.desiredTickets = desiredTickets;
		input.buyNow = buyNow;
		PULSE::DepositAutoParticipation_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_DEPOSIT_AUTO_PARTICIPATION, input, output, user, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::WithdrawAutoParticipation_output withdrawAutoParticipation(const id& user, sint64 amount)
	{
		ensureUserEnergy(user);
		PULSE::WithdrawAutoParticipation_input input{};
		input.amount = amount;
		PULSE::WithdrawAutoParticipation_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_WITHDRAW_AUTO_PARTICIPATION, input, output, user, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetAutoConfig_output setAutoConfig(const id& user, sint16 desiredTickets)
	{
		ensureUserEnergy(user);
		PULSE::SetAutoConfig_input input{};
		input.desiredTickets = desiredTickets;
		PULSE::SetAutoConfig_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_AUTO_CONFIG, input, output, user, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetAutoLimits_output setAutoLimits(const id& invocator, uint16 maxTicketsPerUser)
	{
		ensureUserEnergy(invocator);
		PULSE::SetAutoLimits_input input{};
		input.maxTicketsPerUser = maxTicketsPerUser;
		PULSE::SetAutoLimits_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_AUTO_LIMITS, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetPrice_output setPrice(const id& invocator, uint64 newPrice)
	{
		ensureUserEnergy(invocator);
		PULSE::SetPrice_input input{};
		input.newPrice = newPrice;
		PULSE::SetPrice_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_PRICE, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetSchedule_output setSchedule(const id& invocator, uint8 newSchedule)
	{
		ensureUserEnergy(invocator);
		PULSE::SetSchedule_input input{};
		input.newSchedule = newSchedule;
		PULSE::SetSchedule_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_SCHEDULE, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetDrawHour_output setDrawHour(const id& invocator, uint8 newDrawHour)
	{
		ensureUserEnergy(invocator);
		PULSE::SetDrawHour_input input{};
		input.newDrawHour = newDrawHour;
		PULSE::SetDrawHour_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_DRAW_HOUR, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetFees_output setFees(const id& invocator, uint8 dev, uint8 burn, uint8 shareholders, uint8 qheart)
	{
		ensureUserEnergy(invocator);
		PULSE::SetFees_input input{};
		input.devPercent = dev;
		input.burnPercent = burn;
		input.shareholdersPercent = shareholders;
		input.qheartPercent = qheart;
		PULSE::SetFees_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_FEES, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	PULSE::SetQHeartHoldLimit_output setQHeartHoldLimit(const id& invocator, uint64 newLimit)
	{
		ensureUserEnergy(invocator);
		PULSE::SetQHeartHoldLimit_input input{};
		input.newQHeartHoldLimit = newLimit;
		PULSE::SetQHeartHoldLimit_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_QHEART_HOLD_LIMIT, input, output, invocator, 0))
		{
			output.returnCode = static_cast<uint8>(PULSE::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	void beginEpoch() { callSystemProcedure(PULSE_CONTRACT_INDEX, BEGIN_EPOCH); }
	void endEpoch() { callSystemProcedure(PULSE_CONTRACT_INDEX, END_EPOCH); }
	void beginTick() { callSystemProcedure(PULSE_CONTRACT_INDEX, BEGIN_TICK); }

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
		// Align to update period so BEGIN_TICK evaluates draw logic.
		system.tick = system.tick + (PULSE_TICK_UPDATE_PERIOD - (system.tick % PULSE_TICK_UPDATE_PERIOD));
		beginTick();
	}

	struct QHeartIssuance
	{
		int issuanceIndex;
		int ownershipIndex;
		int possessionIndex;
	};

	QHeartIssuance issueQHeart(sint64 totalShares)
	{
		static constexpr char name[7] = {'Q', 'H', 'E', 'A', 'R', 'T', 0};
		static constexpr char unit[7] = {};
		QHeartIssuance info{};
		const sint64 issued = issueAsset(PULSE_QHEART_ISSUER, name, 0, unit, totalShares, PULSE_CONTRACT_INDEX, &info.issuanceIndex,
		                                 &info.ownershipIndex, &info.possessionIndex);
		EXPECT_EQ(issued, totalShares);
		return info;
	}

	void transferQHeart(const QHeartIssuance& issuance, const id& dest, sint64 amount)
	{
		int destOwnershipIndex = 0;
		int destPossessionIndex = 0;
		EXPECT_TRUE(transferShareOwnershipAndPossession(issuance.ownershipIndex, issuance.possessionIndex, dest, amount, &destOwnershipIndex,
		                                                &destPossessionIndex, true));
	}

	void issuePulseSharesTo(const id& holder, unsigned int shares)
	{
		std::vector<std::pair<m256i, unsigned int>> initialShares;
		initialShares.emplace_back(holder, shares);
		issueContractShares(PULSE_CONTRACT_INDEX, initialShares, false);
	}

	uint64 qheartBalanceOf(const id& owner) const
	{
		const long long balance =
		    numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, owner, owner, PULSE_CONTRACT_INDEX, PULSE_CONTRACT_INDEX);
		return (balance > 0) ? static_cast<uint64>(balance) : 0;
	}

private:
	static void ensureUserEnergy(const id& user) { increaseEnergy(user, 1); }
};

namespace
{
	// Mirror contract RNG path so tests can assert deterministic winners.
	Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> deriveWinningDigits(ContractTestingPulse& ctl, const m256i& digest)
	{
		m256i hashResult;
		KangarooTwelve(reinterpret_cast<const uint8*>(&digest), sizeof(m256i), reinterpret_cast<uint8*>(&hashResult), sizeof(m256i));
		const uint64 seed = hashResult.m256i_u64[0];

		QpiContextUserFunctionCall qpiFunc(PULSE_CONTRACT_INDEX);
		primeQpiFunctionContext(qpiFunc);
		return ctl.state()->callGetRandomDigits(qpiFunc, seed).digits;
	}

	uint8 findMissingDigit(const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning)
	{
		bool seen[PULSE_MAX_DIGIT + 1] = {};
		for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
		{
			seen[winning.get(i)] = true;
		}
		for (uint8 d = 0; d <= PULSE_MAX_DIGIT; ++d)
		{
			if (!seen[d])
			{
				return d;
			}
		}
		return 0;
	}

} // namespace

// ============================================================================
// STATIC + PRIVATE METHOD TESTS
// ============================================================================

// Regression coverage for deterministic helpers used by draw logic.
TEST(ContractPulse_Static, MakeDateStampMinMaxAndMixingAreDeterministic)
{
	uint32 stamp = 0;
	PULSE::makeDateStamp(25, 1, 10, stamp);
	EXPECT_EQ(stamp, static_cast<uint32>(25 << 9 | 1 << 5 | 10));
	EXPECT_EQ(PULSE::min<uint32>(3, 5), 3u);
	EXPECT_EQ(PULSE::max<uint32>(3, 5), 5u);

	uint64 mixed1 = 0;
	uint64 mixed2 = 0;
	PULSE::mix64(0x12345678ULL, mixed1);
	PULSE::mix64(0x12345678ULL, mixed2);
	EXPECT_EQ(mixed1, mixed2);

	uint64 d1 = 0;
	uint64 d2 = 0;
	PULSE::deriveOne(0xABCDEFULL, 0, d1);
	PULSE::deriveOne(0xABCDEFULL, 1, d2);
	EXPECT_NE(d1, d2);
}

// Guard state flag transitions used to open/close ticket sales.
TEST(ContractPulse_Static, SellingFlagToggles)
{
	ContractTestingPulse ctl;
	ctl.state()->forceSelling(true);
	EXPECT_TRUE(ctl.state()->isSelling());
	ctl.state()->forceSelling(false);
	EXPECT_FALSE(ctl.state()->isSelling());
}

// Ensure reward multipliers stay aligned with contract constants.
TEST(ContractPulse_Static, RewardTablesMatchContractConstants)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(6), 2000u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(5), 300u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(4), 60u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(3), 20u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(2), 4u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(1), 1u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetLeftAlignedReward(0), 0u);

	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(6), 150u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(5), 30u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(4), 8u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(3), 2u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(2), 0u);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(1), 0u);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(0), 0u);
}

// Prevent stale config from leaking across epochs.
TEST(ContractPulse_Private, NextEpochDataClearResetsFlagsAndValues)
{
	PULSE::NextEpochData data{};
	data.hasNewPrice = true;
	data.hasNewSchedule = true;
	data.hasNewDrawHour = true;
	data.hasNewFee = true;
	data.hasNewQHeartHoldLimit = true;
	data.newPrice = 10;
	data.newSchedule = 1;
	data.newDrawHour = 2;
	data.newDevPercent = 3;
	data.newBurnPercent = 4;
	data.newShareholdersPercent = 5;
	data.newQHeartPercent = 6;
	data.newQHeartHoldLimit = 7;

	data.clear();
	EXPECT_FALSE(data.hasNewPrice);
	EXPECT_FALSE(data.hasNewSchedule);
	EXPECT_FALSE(data.hasNewDrawHour);
	EXPECT_FALSE(data.hasNewFee);
	EXPECT_FALSE(data.hasNewQHeartHoldLimit);
	EXPECT_EQ(data.newPrice, 0u);
	EXPECT_EQ(data.newSchedule, 0u);
	EXPECT_EQ(data.newDrawHour, 0u);
	EXPECT_EQ(data.newDevPercent, 0u);
	EXPECT_EQ(data.newBurnPercent, 0u);
	EXPECT_EQ(data.newShareholdersPercent, 0u);
	EXPECT_EQ(data.newQHeartPercent, 0u);
	EXPECT_EQ(data.newQHeartHoldLimit, 0u);
}

// Confirm deferred config applies only at epoch boundary.
TEST(ContractPulse_Private, NextEpochDataApplyUpdatesState)
{
	ContractTestingPulse ctl;
	PULSE::NextEpochData data{};
	data.hasNewPrice = true;
	data.hasNewSchedule = true;
	data.hasNewDrawHour = true;
	data.hasNewFee = true;
	data.hasNewQHeartHoldLimit = true;
	data.newPrice = 123;
	data.newSchedule = 0xAA;
	data.newDrawHour = 7;
	data.newDevPercent = 11;
	data.newBurnPercent = 22;
	data.newShareholdersPercent = 33;
	data.newQHeartPercent = 4;
	data.newQHeartHoldLimit = 999;

	data.apply(*ctl.state());
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), 123u);
	EXPECT_EQ(ctl.state()->getScheduleInternal(), 0xAA);
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), 7u);
	EXPECT_EQ(ctl.state()->getDevPercentInternal(), 11u);
	EXPECT_EQ(ctl.state()->getBurnPercentInternal(), 22u);
	EXPECT_EQ(ctl.state()->getShareholdersPercentInternal(), 33u);
	EXPECT_EQ(ctl.state()->getQHeartPercentInternal(), 4u);
	EXPECT_EQ(ctl.state()->getQHeartHoldLimitInternal(), 999u);
}

// Keep RNG output deterministic for auditability.
TEST(ContractPulse_Private, GetRandomDigitsDeterministic)
{
	ContractTestingPulse ctl;
	QpiContextUserFunctionCall qpi(PULSE_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);

	static constexpr uint64 seed = 0x123456789ABCDEF0ULL;
	const PULSE::GetRandomDigits_output& out1 = ctl.state()->callGetRandomDigits(qpi, seed);
	const PULSE::GetRandomDigits_output& out2 = ctl.state()->callGetRandomDigits(qpi, seed);

	for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
	{
		const uint8 v1 = out1.digits.get(i);
		const uint8 v2 = out2.digits.get(i);
		EXPECT_EQ(v1, v2);
		EXPECT_LE(v1, PULSE_MAX_DIGIT);
	}
}

// Validate PrepareRandomTickets error cases.
TEST(ContractPulse_Private, PrepareRandomTicketsRejectsInvalidInputs)
{
	ContractTestingPulse ctl;
	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);

	PULSE::PrepareRandomTickets_output out = ctl.state()->callPrepareRandomTickets(qpi, 0);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	ctl.endEpoch();
	out = ctl.state()->callPrepareRandomTickets(qpi, 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_SELLING_CLOSED));
}

// Guard sold-out behavior in PrepareRandomTickets.
TEST(ContractPulse_Private, PrepareRandomTicketsRejectsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);
	const PULSE::PrepareRandomTickets_output out = ctl.state()->callPrepareRandomTickets(qpi, 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_ALL_SOLD_OUT));
}

// Validate ChargeTicketsFromPlayer rejects invalid inputs and insufficient balance.
TEST(ContractPulse_Private, ChargeTicketsFromPlayerRejectsInvalidOrInsufficient)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);

	PULSE::ChargeTicketsFromPlayer_output out = ctl.state()->callChargeTicketsFromPlayer(qpi, user, 0);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	out = ctl.state()->callChargeTicketsFromPlayer(qpi, id::zero(), 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	out = ctl.state()->callChargeTicketsFromPlayer(qpi, user, 2);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_INVALID_PRICE));
}

// Validate AllocateRandomTickets rejects invalid inputs and closed/sold-out states.
TEST(ContractPulse_Private, AllocateRandomTicketsRejectsInvalidOrClosed)
{
	ContractTestingPulse ctl;
	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);

	PULSE::AllocateRandomTickets_output out = ctl.state()->callAllocateRandomTickets(qpi, id::zero(), 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	out = ctl.state()->callAllocateRandomTickets(qpi, id::randomValue(), 0);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	ctl.endEpoch();
	out = ctl.state()->callAllocateRandomTickets(qpi, id::randomValue(), 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_SELLING_CLOSED));
}

// Guard AllocateRandomTickets sold-out path.
TEST(ContractPulse_Private, AllocateRandomTicketsRejectsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);
	const PULSE::AllocateRandomTickets_output out = ctl.state()->callAllocateRandomTickets(qpi, id::randomValue(), 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_ALL_SOLD_OUT));
}

// Ensure ProcessAutoTickets skips when selling is closed.
TEST(ContractPulse_Private, ProcessAutoTicketsSkipsWhenSellingClosed)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	ctl.state()->setAutoParticipant(user, 1, 1);
	ctl.state()->forceSelling(false);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);
	ctl.state()->callProcessAutoTickets(qpi);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
}

// Ensure ProcessAutoTickets skips when no slots are left.
TEST(ContractPulse_Private, ProcessAutoTicketsSkipsWhenSoldOut)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.state()->setAutoParticipant(user, static_cast<sint64>(ticketPrice), 1);

	ctl.state()->forceSelling(true);
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);
	ctl.state()->callProcessAutoTickets(qpi);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(PULSE_MAX_NUMBER_OF_PLAYERS));
}

// Remove auto participants that cannot afford a ticket.
TEST(ContractPulse_Private, ProcessAutoTicketsRemovesUnaffordableParticipant)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.state()->setAutoParticipant(user, static_cast<sint64>(ticketPrice - 1), 1);

	ctl.state()->forceSelling(true);
	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);
	ctl.state()->callProcessAutoTickets(qpi);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Keep auto participant when desired ticket count is zero.
TEST(ContractPulse_Private, ProcessAutoTicketsSkipsZeroDesiredTickets)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.state()->setAutoParticipant(user, static_cast<sint64>(ticketPrice * 2), 0);

	ctl.state()->forceSelling(true);
	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, PULSE_QHEART_ISSUER, 0);
	primeQpiProcedureContext(qpi);
	ctl.state()->callProcessAutoTickets(qpi);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, ticketPrice * 2);
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 0u);
	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
}

// ============================================================================
// PUBLIC FUNCTIONS AND PROCEDURES
// ============================================================================

// Confirm defaults are visible through the public API after init.
TEST(ContractPulse_Public, GettersReturnDefaultsAfterInitialize)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, PULSE_TICKET_PRICE_DEFAULT);
	EXPECT_EQ(ctl.getSchedule().schedule, PULSE_DEFAULT_SCHEDULE);
	EXPECT_EQ(ctl.getDrawHour().drawHour, PULSE_DEFAULT_DRAW_HOUR);
	EXPECT_EQ(ctl.getQHeartHoldLimit().qheartHoldLimit, PULSE_DEFAULT_QHEART_HOLD_LIMIT);

	const PULSE::GetFees_output& fees = ctl.getFees();
	EXPECT_EQ(fees.devPercent, PULSE_DEFAULT_DEV_PERCENT);
	EXPECT_EQ(fees.burnPercent, PULSE_DEFAULT_BURN_PERCENT);
	EXPECT_EQ(fees.shareholdersPercent, PULSE_DEFAULT_SHAREHOLDERS_PERCENT);
	EXPECT_EQ(fees.qheartPercent, PULSE_DEFAULT_QHEART_PERCENT);
	EXPECT_EQ(fees.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetWinningDigits_output& win = ctl.getWinningDigits();
	for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
	{
		EXPECT_EQ(win.digits.get(i), 0u);
	}
	EXPECT_EQ(ctl.getBalance().balance, 0u);
	EXPECT_EQ(ctl.getQHeartWallet().wallet, PULSE_QHEART_ISSUER);
}

// Guard admin-only price changes and deferred apply.
TEST(ContractPulse_Public, SetPriceGuardsAccessAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setPrice(id::randomValue(), 123).returnCode, static_cast<uint8>(PULSE::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(ctl.setPrice(PULSE_QHEART_ISSUER, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_INVALID_PRICE));

	EXPECT_EQ(ctl.setPrice(PULSE_QHEART_ISSUER, 555).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), PULSE_TICKET_PRICE_DEFAULT);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), 555u);
}

// Ensure schedule validation and deferred apply are enforced.
TEST(ContractPulse_Public, SetScheduleValidatesAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setSchedule(id::randomValue(), 1).returnCode, static_cast<uint8>(PULSE::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(ctl.setSchedule(PULSE_QHEART_ISSUER, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	EXPECT_EQ(ctl.setSchedule(PULSE_QHEART_ISSUER, 0x7F).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getScheduleInternal(), PULSE_DEFAULT_SCHEDULE);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getScheduleInternal(), 0x7Fu);
}

// Ensure draw hour range checks and deferred apply are enforced.
TEST(ContractPulse_Public, SetDrawHourValidatesAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setDrawHour(id::randomValue(), 12).returnCode, static_cast<uint8>(PULSE::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(ctl.setDrawHour(PULSE_QHEART_ISSUER, 24).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	EXPECT_EQ(ctl.setDrawHour(PULSE_QHEART_ISSUER, 9).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), PULSE_DEFAULT_DRAW_HOUR);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), 9u);
}

// Protect against invalid fee splits and apply on epoch end.
TEST(ContractPulse_Public, SetFeesValidatesAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setFees(id::randomValue(), 1, 2, 3, 4).returnCode, static_cast<uint8>(PULSE::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(ctl.setFees(PULSE_QHEART_ISSUER, 60, 60, 0, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	EXPECT_EQ(ctl.setFees(PULSE_QHEART_ISSUER, 11, 22, 33, 4).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getDevPercentInternal(), PULSE_DEFAULT_DEV_PERCENT);
	EXPECT_EQ(ctl.state()->getBurnPercentInternal(), PULSE_DEFAULT_BURN_PERCENT);
	EXPECT_EQ(ctl.state()->getShareholdersPercentInternal(), PULSE_DEFAULT_SHAREHOLDERS_PERCENT);
	EXPECT_EQ(ctl.state()->getQHeartPercentInternal(), PULSE_DEFAULT_QHEART_PERCENT);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getDevPercentInternal(), 11u);
	EXPECT_EQ(ctl.state()->getBurnPercentInternal(), 22u);
	EXPECT_EQ(ctl.state()->getShareholdersPercentInternal(), 33u);
	EXPECT_EQ(ctl.state()->getQHeartPercentInternal(), 4u);
}

// Ensure hold-limit changes do not affect the current round.
TEST(ContractPulse_Public, SetQHeartHoldLimitAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setQHeartHoldLimit(id::randomValue(), 100).returnCode, static_cast<uint8>(PULSE::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(ctl.setQHeartHoldLimit(PULSE_QHEART_ISSUER, 1234).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getQHeartHoldLimitInternal(), PULSE_DEFAULT_QHEART_HOLD_LIMIT);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getQHeartHoldLimitInternal(), 1234u);
}

// Ensure getters report newly applied config values after epoch end.
TEST(ContractPulse_Public, GettersReflectAppliedChanges)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setPrice(PULSE_QHEART_ISSUER, 555).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setSchedule(PULSE_QHEART_ISSUER, 0x7F).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setDrawHour(PULSE_QHEART_ISSUER, 9).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setFees(PULSE_QHEART_ISSUER, 11, 22, 33, 4).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setQHeartHoldLimit(PULSE_QHEART_ISSUER, 4321).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	ctl.endEpoch();

	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, 555u);
	EXPECT_EQ(ctl.getSchedule().schedule, 0x7Fu);
	EXPECT_EQ(ctl.getDrawHour().drawHour, 9u);
	EXPECT_EQ(ctl.getQHeartHoldLimit().qheartHoldLimit, 4321u);

	const PULSE::GetFees_output fees = ctl.getFees();
	EXPECT_EQ(fees.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(fees.devPercent, 11u);
	EXPECT_EQ(fees.burnPercent, 22u);
	EXPECT_EQ(fees.shareholdersPercent, 33u);
	EXPECT_EQ(fees.qheartPercent, 4u);
}

// Prevent ticket purchases outside the selling window.
TEST(ContractPulse_Public, BuyTicketWhenSellingClosedFails)
{
	ContractTestingPulse ctl;
	const PULSE::BuyTicket_output& out = ctl.buyTicket(id::randomValue(), makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_SELLING_CLOSED));
}

// Reject malformed tickets before funds are transferred.
TEST(ContractPulse_Public, BuyTicketValidatesDigits)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	ctl.transferQHeart(issuance, user, PULSE_TICKET_PRICE_DEFAULT);

	const PULSE::BuyTicket_output& out = ctl.buyTicket(user, makePlayerDigits(0, 1, 2, 3, 4, 10));
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_NUMBERS));
}

// Enforce hard cap on ticket count.
TEST(ContractPulse_Public, BuyTicketFailsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	const PULSE::BuyTicket_output& out = ctl.buyTicket(id::randomValue(), makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_ALL_SOLD_OUT));
}

// Avoid unintended debt when buyer lacks funds.
TEST(ContractPulse_Public, BuyTicketFailsWithInsufficientBalance)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	ctl.transferQHeart(issuance, user, PULSE_TICKET_PRICE_DEFAULT - 1);

	const PULSE::BuyTicket_output& out = ctl.buyTicket(user, makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_INVALID_PRICE));
}

// Validate successful purchase moves funds and stores ticket.
TEST(ContractPulse_Public, BuyTicketSucceedsAndMovesQHeart)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	ctl.transferQHeart(issuance, user, PULSE_TICKET_PRICE_DEFAULT * 2);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());

	const PULSE::BuyTicket_output& out = ctl.buyTicket(user, makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), 1u);
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore - PULSE_TICKET_PRICE_DEFAULT);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore + PULSE_TICKET_PRICE_DEFAULT);
}

// Prevent random purchases outside the selling window.
TEST(ContractPulse_Public, BuyRandomTicketsFailsWhenSellingClosed)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_SELLING_CLOSED));
}

// Reject empty batch requests to avoid no-op transfers.
TEST(ContractPulse_Public, BuyRandomTicketsRejectsZeroCount)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const id user = id::randomValue();
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 0);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Enforce capacity checks for batch purchases.
TEST(ContractPulse_Public, BuyRandomTicketsFailsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	const id user = id::randomValue();
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 2);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_ALL_SOLD_OUT));
}

// Avoid partial batch purchases when balance is insufficient.
TEST(ContractPulse_Public, BuyRandomTicketsFailsWithInsufficientBalance)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	ctl.transferQHeart(issuance, user, PULSE_TICKET_PRICE_DEFAULT);

	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 2);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_INVALID_PRICE));
}

// Validate batch purchase moves funds and mints tickets.
TEST(ContractPulse_Public, BuyRandomTicketsSucceedsAndMovesQHeart)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	static constexpr uint16 ticketCount = 3;
	static constexpr uint64 totalPrice = static_cast<uint64>(ticketCount) * PULSE_TICKET_PRICE_DEFAULT;
	ctl.transferQHeart(issuance, user, totalPrice);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	etalonTick.prevSpectrumDigest = m256i(0xAAAABBBBULL, 0xCCCCDDDDULL, 0x11112222ULL, 0x33334444ULL);

	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, ticketCount);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(ticketCount));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore - totalPrice);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore + totalPrice);

	std::set<uint32> seen;
	for (uint16 i = 0; i < ticketCount; ++i)
	{
		const PULSE::Ticket ticket = ctl.state()->getTicket(i);
		EXPECT_EQ(ticket.player, user);
		QpiContextUserFunctionCall qpi(PULSE_CONTRACT_INDEX);
		primeQpiFunctionContext(qpi);
		const PULSE::ValidateDigits_output validated = ctl.state()->callValidateDigits(qpi, ticket.digits);
		EXPECT_TRUE(validated.isValid);

		uint32 key = 0;
		uint32 mul = 1;
		for (uint64 d = 0; d < PULSE_PLAYER_DIGITS; ++d)
		{
			key += static_cast<uint32>(ticket.digits.get(d)) * mul;
			mul *= 10;
		}
		EXPECT_TRUE(seen.insert(key).second);
	}
}

// Validate deterministic random tickets for a fixed spectrum digest.
TEST(ContractPulse_Public, BuyRandomTicketsDeterministicWithFixedDigest)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);

	const m256i digest(0xABCDEF01ULL, 0x12345678ULL, 0xCAFEBABEULL, 0x0BADF00DULL);
	etalonTick.prevSpectrumDigest = digest;

	m256i hashResult;
	KangarooTwelve(reinterpret_cast<const uint8*>(&digest), sizeof(m256i), reinterpret_cast<uint8*>(&hashResult), sizeof(m256i));
	const uint64 randomSeed = hashResult.m256i_u64[0];
	uint64 tempSeed = 0;
	PULSEChecker::deriveOne(randomSeed, 0, tempSeed);

	QpiContextUserFunctionCall qpi(PULSE_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& expected = ctl.state()->callGetRandomDigits(qpi, tempSeed).digits;

	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::Ticket ticket = ctl.state()->getTicket(0);
	for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
	{
		EXPECT_EQ(ticket.digits.get(i), expected.get(i));
	}
}

// Clamp random ticket purchases to remaining capacity.
TEST(ContractPulse_Public, BuyRandomTicketsClampsToSlotsLeft)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice * 2);

	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS - 2);
	etalonTick.prevSpectrumDigest = m256i(0xAAAAULL, 0xBBBBULL, 0xCCCCULL, 0xDDDDULL);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 5);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(PULSE_MAX_NUMBER_OF_PLAYERS));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore - (ticketPrice * 2));
}

// Reject non-positive auto-participation inputs.
TEST(ContractPulse_Public, DepositAutoParticipationRejectsInvalidValues)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();

	EXPECT_EQ(ctl.depositAutoParticipation(user, 0, 1, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
	EXPECT_EQ(ctl.depositAutoParticipation(user, 1, 0, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
	EXPECT_EQ(ctl.depositAutoParticipation(user, 1, -1, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Clamp desired ticket counts to configured limits and store the deposit.
TEST(ContractPulse_Public, DepositAutoParticipationClampsDesiredTicketsAndStoresDeposit)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setAutoLimits(PULSE_QHEART_ISSUER, 2).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 2);
	ctl.transferQHeart(issuance, user, amount);

	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, amount, 5, false);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, static_cast<uint64>(amount));
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 2u);
}

// Clamp deposit amount to available user balance.
TEST(ContractPulse_Public, DepositAutoParticipationClampsAmountToBalance)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 balance = static_cast<sint64>(ticketPrice * 2);
	ctl.transferQHeart(issuance, user, static_cast<uint64>(balance));

	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, balance + ticketPrice, 1, false);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, static_cast<uint64>(balance));
}

// Subsequent deposits should add to the balance and update desired ticket count.
TEST(ContractPulse_Public, DepositAutoParticipationAccumulatesAndUpdatesDesiredTickets)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amountFirst = static_cast<sint64>(ticketPrice * 2);
	const sint64 amountSecond = static_cast<sint64>(ticketPrice * 3);
	const sint64 totalAmount = amountFirst + amountSecond;
	ctl.transferQHeart(issuance, user, static_cast<uint64>(totalAmount));

	EXPECT_EQ(ctl.depositAutoParticipation(user, amountFirst, 1, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.depositAutoParticipation(user, amountSecond, 2, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, static_cast<uint64>(totalAmount));
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 2u);
}

// Enforce minimum balance for desired auto-purchases.
TEST(ContractPulse_Public, DepositAutoParticipationRejectsInsufficientAmount)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);

	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, ticketPrice, 2, false);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_INVALID_PRICE));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Reject new participants once the auto list is at capacity.
TEST(ContractPulse_Public, DepositAutoParticipationRejectsWhenAutoParticipantsFull)
{
	ContractTestingPulse ctl;
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice);
	const sint64 totalShares = static_cast<sint64>(ticketPrice) * static_cast<sint64>(PULSE_MAX_NUMBER_OF_AUTO_PARTICIPANTS + 1);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(totalShares);

	for (uint32 i = 0; i < PULSE_MAX_NUMBER_OF_AUTO_PARTICIPANTS; ++i)
	{
		const id user = id::randomValue();
		ctl.transferQHeart(issuance, user, ticketPrice);
		EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 1, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	}

	const id extraUser = id::randomValue();
	ctl.transferQHeart(issuance, extraUser, ticketPrice);
	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(extraUser, amount, 1, false);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::AUTO_PARTICIPANTS_FULL));
}

// When buy-now spends the entire deposit, no auto-participation entry is created.
TEST(ContractPulse_Public, DepositAutoParticipationBuyNowConsumesAllAndSkipsDeposit)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	static constexpr uint16 desiredTickets = 2;
	const uint64 totalPrice = ticketPrice * desiredTickets;
	ctl.transferQHeart(issuance, user, totalPrice);
	etalonTick.prevSpectrumDigest = m256i(0x1111ULL, 0x2222ULL, 0x3333ULL, 0x4444ULL);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, totalPrice, desiredTickets, true);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(desiredTickets));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore - totalPrice);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore + totalPrice);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// If buy-now leaves a remainder, keep it as a deposit entry.
TEST(ContractPulse_Public, DepositAutoParticipationBuyNowStoresRemainder)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	static constexpr uint16 desiredTickets = 2;
	const uint64 totalPrice = ticketPrice * desiredTickets;
	const sint64 amount = static_cast<sint64>(totalPrice + ticketPrice);
	ctl.transferQHeart(issuance, user, static_cast<uint64>(amount));
	etalonTick.prevSpectrumDigest = m256i(0xAAAAULL, 0xBBBBULL, 0xCCCCULL, 0xDDDDULL);

	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, amount, desiredTickets, true);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(desiredTickets));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, ticketPrice);
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), static_cast<uint32>(desiredTickets));
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore + static_cast<uint64>(amount));
}

// If selling is closed, buy-now should skip buying and keep the deposit.
TEST(ContractPulse_Public, DepositAutoParticipationBuyNowStoresDepositWhenSellingClosed)
{
	ContractTestingPulse ctl;
	ctl.endEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	static constexpr uint16 desiredTickets = 2;
	const sint64 amount = static_cast<sint64>(ticketPrice * desiredTickets);
	ctl.transferQHeart(issuance, user, static_cast<uint64>(amount));

	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, amount, desiredTickets, true);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, static_cast<uint64>(amount));
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), static_cast<uint32>(desiredTickets));
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore + static_cast<uint64>(amount));
}

// If buy-now cannot allocate tickets, the deposit is not recorded.
TEST(ContractPulse_Public, DepositAutoParticipationBuyNowFailsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	static constexpr uint16 desiredTickets = 1;
	const uint64 totalPrice = ticketPrice * desiredTickets;
	ctl.transferQHeart(issuance, user, totalPrice);
	etalonTick.prevSpectrumDigest = m256i(0xAAAAULL, 0xBBBBULL, 0xCCCCULL, 0xDDDDULL);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const PULSE::DepositAutoParticipation_output out = ctl.depositAutoParticipation(user, totalPrice, desiredTickets, true);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TICKET_ALL_SOLD_OUT));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore);

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Reject withdrawals for unknown auto participants.
TEST(ContractPulse_Public, WithdrawAutoParticipationRejectsMissingEntry)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const PULSE::WithdrawAutoParticipation_output out = ctl.withdrawAutoParticipation(user, 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Full withdrawal removes the entry and refunds the deposit.
TEST(ContractPulse_Public, WithdrawAutoParticipationFullRemovesEntry)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 2);
	ctl.transferQHeart(issuance, user, amount);

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 2, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const PULSE::WithdrawAutoParticipation_output out = ctl.withdrawAutoParticipation(user, 0);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore + static_cast<uint64>(amount));
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore - static_cast<uint64>(amount));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Partial withdrawal keeps the entry with the remaining deposit.
TEST(ContractPulse_Public, WithdrawAutoParticipationPartialKeepsEntry)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 3);
	ctl.transferQHeart(issuance, user, amount);

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 3, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	const PULSE::WithdrawAutoParticipation_output out = ctl.withdrawAutoParticipation(user, ticketPrice);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, static_cast<uint64>(amount - ticketPrice));
}

// Withdraws more than the deposit should return the full amount and remove the entry.
TEST(ContractPulse_Public, WithdrawAutoParticipationOverdrawsToFullWithdrawal)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 2);
	ctl.transferQHeart(issuance, user, amount);

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 2, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const PULSE::WithdrawAutoParticipation_output out = ctl.withdrawAutoParticipation(user, static_cast<sint64>(ticketPrice * 5));
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore + static_cast<uint64>(amount));
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore - static_cast<uint64>(amount));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Surface transfer failures when the contract lacks sufficient QHeart.
TEST(ContractPulse_Public, WithdrawAutoParticipationFailsWhenTransferFails)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.state()->setAutoParticipant(user, static_cast<sint64>(ticketPrice), 1);

	const PULSE::WithdrawAutoParticipation_output out = ctl.withdrawAutoParticipation(user, ticketPrice);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::TRANSFER_FROM_PULSE_FAILED));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, ticketPrice);
}

// Validate SetAutoConfig input and clamp to limits.
TEST(ContractPulse_Public, SetAutoConfigValidatesAndClamps)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 3);
	ctl.transferQHeart(issuance, user, amount);

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 3, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setAutoConfig(user, -2).returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));

	EXPECT_EQ(ctl.setAutoLimits(PULSE_QHEART_ISSUER, 2).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setAutoConfig(user, -1).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 3u);

	EXPECT_EQ(ctl.setAutoConfig(user, 5).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 2u);
}

// Allow disabling auto tickets with desiredTickets = 0.
TEST(ContractPulse_Public, SetAutoConfigDisablesDesiredTickets)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 2);
	ctl.transferQHeart(issuance, user, amount);

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 2, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setAutoConfig(user, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 0u);
}

// Remove entry when both deposit and desired tickets are zero.
TEST(ContractPulse_Public, SetAutoConfigRemovesEmptyEntry)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	ctl.state()->setAutoParticipant(user, 0, 1);

	EXPECT_EQ(ctl.setAutoConfig(user, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Reject config updates for users without auto participation.
TEST(ContractPulse_Public, SetAutoConfigRejectsMissingEntry)
{
	ContractTestingPulse ctl;
	const id user = id::randomValue();
	const PULSE::SetAutoConfig_output out = ctl.setAutoConfig(user, 1);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Enforce access and range checks on auto limits.
TEST(ContractPulse_Public, SetAutoLimitsGuardsAccessAndValidates)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setAutoLimits(id::randomValue(), 10).returnCode, static_cast<uint8>(PULSE::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(ctl.setAutoLimits(PULSE_QHEART_ISSUER, PULSE_MAX_NUMBER_OF_PLAYERS + 1).returnCode,
	          static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
	EXPECT_EQ(ctl.setAutoLimits(PULSE_QHEART_ISSUER, 5).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoStats_output stats = ctl.getAutoStats();
	EXPECT_EQ(stats.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(static_cast<uint32>(stats.maxAutoTicketsPerUser), 5u);
}

// Allow disabling auto ticket limits by setting them to zero.
TEST(ContractPulse_Public, SetAutoLimitsAllowsDisabling)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setAutoLimits(PULSE_QHEART_ISSUER, 3).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setAutoLimits(PULSE_QHEART_ISSUER, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoStats_output stats = ctl.getAutoStats();
	EXPECT_EQ(stats.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(static_cast<uint32>(stats.maxAutoTicketsPerUser), 0u);
}

// Report auto participation counts through the public stats API.
TEST(ContractPulse_Public, GetAutoStatsReportsParticipantCount)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;

	const id userA = id::randomValue();
	const id userB = id::randomValue();
	ctl.transferQHeart(issuance, userA, ticketPrice);
	ctl.transferQHeart(issuance, userB, ticketPrice);

	EXPECT_EQ(ctl.depositAutoParticipation(userA, ticketPrice, 1, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.depositAutoParticipation(userB, ticketPrice, 1, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const PULSE::GetAutoStats_output stats = ctl.getAutoStats();
	EXPECT_EQ(stats.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(static_cast<uint32>(stats.autoParticipantsCounter), 2u);
}

// Ensure balance getter reflects actual QHeart wallet holdings.
TEST(ContractPulse_Public, GetBalanceReportsQHeartWalletBalance)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	ctl.transferQHeart(issuance, ctl.pulseSelf(), 12345);
	EXPECT_EQ(ctl.getBalance().balance, 12345u);
}

// Confirm winner history records paid prizes.
TEST(ContractPulse_Public, GetWinnersReportsPaidTickets)
{
	ContractTestingPulse ctl;
	ctl.issuePulseSharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);

	EXPECT_EQ(ctl.setFees(PULSE_QHEART_ISSUER, 0, 0, 0, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setPrice(PULSE_QHEART_ISSUER, 1).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	ctl.transferQHeart(issuance, ctl.pulseSelf(), 10000);
	const m256i digest(0x2222ULL, 0x3333ULL, 0x4444ULL, 0x5555ULL);
	etalonTick.prevSpectrumDigest = digest;
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = deriveWinningDigits(ctl, digest);
	const uint8 missing = findMissingDigit(winning);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketA =
	    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), winning.get(3), winning.get(4), winning.get(5));
	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketB =
	    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), missing, missing, missing);

	const id playerA = id::randomValue();
	const id playerB = id::randomValue();
	ctl.transferQHeart(issuance, playerA, 1);
	ctl.transferQHeart(issuance, playerB, 1);
	EXPECT_EQ(ctl.buyTicket(playerA, ticketA).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.buyTicket(playerB, ticketB).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const uint64 prizeA = ctl.state()->callComputePrize(winning, ticketA);
	const uint64 prizeB = ctl.state()->callComputePrize(winning, ticketB);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	const PULSE::GetWinners_output& winners = ctl.getWinners();
	EXPECT_EQ(winners.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(winners.winnersCounter, 2u);
	EXPECT_EQ(winners.winners.get(0).winnerAddress, playerA);
	EXPECT_EQ(winners.winners.get(0).revenue, prizeA);
	EXPECT_EQ(winners.winners.get(1).winnerAddress, playerB);
	EXPECT_EQ(winners.winners.get(1).revenue, prizeB);
}

// ============================================================================
// SYSTEM PROCEDURES
// ============================================================================

// Ensure epoch start repairs defaults and opens selling.
TEST(ContractPulse_System, BeginEpochRestoresDefaultsAndOpensSelling)
{
	ContractTestingPulse ctl;
	ctl.state()->setScheduleInternal(0);
	ctl.state()->setDrawHourInternal(0);
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	EXPECT_EQ(ctl.state()->getScheduleInternal(), PULSE_DEFAULT_SCHEDULE);
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), PULSE_DEFAULT_DRAW_HOUR);
	EXPECT_TRUE(ctl.state()->isSelling());
}

// BeginEpoch should auto-buy tickets from stored deposits.
TEST(ContractPulse_System, BeginEpochProcessesAutoParticipants)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 2);
	ctl.transferQHeart(issuance, user, amount);

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 2, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	etalonTick.prevSpectrumDigest = m256i(0xDEADULL, 0xBEEFULL, 0xFADEULL, 0xCAFEULL);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	EXPECT_EQ(ctl.state()->getTicketCounter(), 2u);
	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::INVALID_VALUE));
}

// Auto-buy should leave remaining deposit when it is larger than the ticket cost.
TEST(ContractPulse_System, BeginEpochAutoParticipationLeavesRemainingDeposit)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	const sint64 amount = static_cast<sint64>(ticketPrice * 3);
	ctl.transferQHeart(issuance, user, static_cast<uint64>(amount));

	EXPECT_EQ(ctl.depositAutoParticipation(user, amount, 2, false).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	etalonTick.prevSpectrumDigest = m256i(0x1111ULL, 0x2222ULL, 0x3333ULL, 0x4444ULL);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	EXPECT_EQ(ctl.state()->getTicketCounter(), 2u);
	const PULSE::GetAutoParticipation_output entry = ctl.getAutoParticipation(user);
	EXPECT_EQ(entry.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(entry.deposit, ticketPrice);
	EXPECT_EQ(static_cast<uint32>(entry.desiredTickets), 2u);
}

// Ensure epoch end applies pending config and clears state.
TEST(ContractPulse_System, EndEpochAppliesPendingChangesAndClearsState)
{
	ContractTestingPulse ctl;
	ctl.state()->setTicketCounter(3);
	ctl.state()->setLastDrawDateStamp(77);
	ctl.state()->nextEpochDataRef().hasNewPrice = true;
	ctl.state()->nextEpochDataRef().newPrice = 999;

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
	EXPECT_EQ(ctl.state()->getLastDrawDateStamp(), 0u);
	EXPECT_FALSE(ctl.state()->isSelling());
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), 999u);
}

// Validate scheduled draw trigger path.
TEST(ContractPulse_System, BeginTickRunsDrawOnScheduledDay)
{
	ContractTestingPulse ctl;
	ctl.issuePulseSharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	ctl.transferQHeart(issuance, ctl.pulseSelf(), 100000);

	const id player = id::randomValue();
	ctl.state()->setTicketDirect(0, player, makePlayerDigits(0, 1, 2, 3, 4, 5));
	ctl.state()->setTicketCounter(1);

	ctl.setDateTime(2025, 1, 10, 12); // Friday
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
	EXPECT_TRUE(ctl.state()->isSelling());
	expectWinningDigitsInRange(ctl.state()->getLastWinningDigits());

	const PULSE::GetWinningDigits_output win = ctl.getWinningDigits();
	for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
	{
		EXPECT_EQ(win.digits.get(i), ctl.state()->getLastWinningDigits().get(i));
	}
}

// Exercise multi-round lifecycle across multiple players.
TEST(ContractPulse_Gameplay, MultipleRoundsMultiplePlayers)
{
	ContractTestingPulse ctl;
	ctl.state()->setTicketPriceInternal(10);

	ctl.issuePulseSharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(100000000);
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, ctl.pulseSelf(), 10000000);
	EXPECT_EQ(ctl.getBalance().balance, 10000000);

	struct RoundDates
	{
		uint8 startDay;
		uint8 drawDay;
	};
	static constexpr RoundDates rounds[] = {
	    {9, 10},  // Thu -> Fri
	    {11, 12}, // Sat -> Sun
	    {14, 15}, // Tue -> Wed
	};

	for (uint32 r = 0; r < 3; ++r)
	{
		ctl.setDateTime(2025, 1, rounds[r].startDay, 12);
		ctl.beginEpoch();

		const m256i digest(0x1111ULL + r, 0x2222ULL + r, 0x3333ULL + r, 0x4444ULL + r);
		etalonTick.prevSpectrumDigest = digest;
		const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = deriveWinningDigits(ctl, digest);

		const uint8 missing = findMissingDigit(winning);
		const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> tickets[] = {
		    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), winning.get(3), winning.get(4), winning.get(5)),
		    makePlayerDigits(winning.get(1), winning.get(2), winning.get(3), winning.get(4), winning.get(5), winning.get(6)),
		    makePlayerDigits(winning.get(2), winning.get(3), winning.get(4), winning.get(5), winning.get(6), winning.get(7)),
		    makePlayerDigits(missing, winning.get(0), winning.get(2), winning.get(4), winning.get(6), winning.get(8)),
		};

		struct PlayerCheck
		{
			id player;
			uint64 balanceAfterBuy;
			uint64 expectedPrize;
		};
		std::vector<PlayerCheck> players;
		players.reserve(4);

		for (const auto& ticketDigits : tickets)
		{
			const id player = id::randomValue();
			ctl.transferQHeart(issuance, player, ticketPrice);
			const PULSE::BuyTicket_output out = ctl.buyTicket(player, ticketDigits);
			EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

			PlayerCheck info{};
			info.player = player;
			info.balanceAfterBuy = ctl.qheartBalanceOf(player);
			info.expectedPrize = ctl.state()->callComputePrize(winning, ticketDigits);
			players.push_back(info);
		}

		ctl.setDateTime(2025, 1, rounds[r].drawDay, 12);
		ctl.forceBeginTick();

		EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
		for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
		{
			EXPECT_EQ(ctl.state()->getLastWinningDigits().get(i), winning.get(i));
		}

		for (const auto& info : players)
		{
			EXPECT_EQ(ctl.qheartBalanceOf(info.player), info.balanceAfterBuy + info.expectedPrize);
		}
	}
}

// Guard pro-rata payout logic when balance is short.
TEST(ContractPulse_Gameplay, ProRataPayoutWhenBalanceInsufficient)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(2000000);

	EXPECT_EQ(ctl.setFees(PULSE_QHEART_ISSUER, 0, 0, 0, 0).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setPrice(PULSE_QHEART_ISSUER, 1).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	static constexpr uint64 preFund = 1000;
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, ctl.pulseSelf(), preFund);

	const m256i digest(0x1234ULL, 0x5678ULL, 0x9ABCULL, 0xDEF0ULL);
	etalonTick.prevSpectrumDigest = digest;
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = deriveWinningDigits(ctl, digest);
	const uint8 missing = findMissingDigit(winning);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketA =
	    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), winning.get(3), winning.get(4), winning.get(5));
	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketB =
	    makePlayerDigits(winning.get(0), winning.get(2), winning.get(4), winning.get(6), winning.get(8), missing);

	const id playerA = id::randomValue();
	const id playerB = id::randomValue();
	ctl.transferQHeart(issuance, playerA, ticketPrice);
	ctl.transferQHeart(issuance, playerB, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(playerA, ticketA).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.buyTicket(playerB, ticketB).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const uint64 balanceAfterBuyA = ctl.qheartBalanceOf(playerA);
	const uint64 balanceAfterBuyB = ctl.qheartBalanceOf(playerB);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const uint64 prizeA = ctl.state()->callComputePrize(winning, ticketA);
	const uint64 prizeB = ctl.state()->callComputePrize(winning, ticketB);
	const uint64 totalPrize = prizeA + prizeB;
	ASSERT_GT(totalPrize, contractBefore);

	const uint64 expectedA = (prizeA * contractBefore) / totalPrize;
	const uint64 expectedB = (prizeB * contractBefore) / totalPrize;

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.qheartBalanceOf(playerA), balanceAfterBuyA + expectedA);
	EXPECT_EQ(ctl.qheartBalanceOf(playerB), balanceAfterBuyB + expectedB);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore - (expectedA + expectedB));
}

// Validate fee distribution to dev, shareholders, and QHeart wallet.
TEST(ContractPulse_Gameplay, FeesDistributedToDevShareholdersAndQHeartWallet)
{
	ContractTestingPulse ctl;
	const id shareholder = id::randomValue();
	ctl.issuePulseSharesTo(shareholder, NUMBER_OF_COMPUTORS);

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	static constexpr uint8 devPercent = 10;
	static constexpr uint8 burnPercent = 0;
	static constexpr uint8 shareholdersPercent = 10;
	static constexpr uint8 qheartPercent = 10;
	const uint64 ticketPrice = static_cast<uint64>(NUMBER_OF_COMPUTORS) * 10;

	EXPECT_EQ(ctl.setFees(PULSE_QHEART_ISSUER, devPercent, burnPercent, shareholdersPercent, qheartPercent).returnCode,
	          static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	EXPECT_EQ(ctl.setPrice(PULSE_QHEART_ISSUER, ticketPrice).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	const id player = id::randomValue();
	ctl.transferQHeart(issuance, player, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(player, makePlayerDigits(0, 1, 2, 3, 4, 5)).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const id devWallet = ctl.state()->getTeamAddressInternal();
	EXPECT_NE(devWallet, shareholder);
	EXPECT_NE(devWallet, PULSE_QHEART_ISSUER);

	const uint64 devBefore = ctl.qheartBalanceOf(devWallet);
	const uint64 shareholderBefore = ctl.qheartBalanceOf(shareholder);
	const uint64 qheartWalletBefore = ctl.qheartBalanceOf(PULSE_QHEART_ISSUER);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	const uint64 roundRevenue = ticketPrice;
	const uint64 expectedDev = (roundRevenue * devPercent) / 100;
	const uint64 expectedShareholders = (roundRevenue * shareholdersPercent) / 100;
	const uint64 expectedQHeart = (roundRevenue * qheartPercent) / 100;
	const uint64 dividendPerShare = expectedShareholders / NUMBER_OF_COMPUTORS;
	const uint64 expectedShareholderGain = dividendPerShare * NUMBER_OF_COMPUTORS;

	EXPECT_EQ(expectedShareholderGain, expectedShareholders);
	EXPECT_EQ(ctl.qheartBalanceOf(devWallet), devBefore + expectedDev);
	EXPECT_EQ(ctl.qheartBalanceOf(shareholder), shareholderBefore + expectedShareholderGain);
	EXPECT_EQ(ctl.qheartBalanceOf(PULSE_QHEART_ISSUER), qheartWalletBefore + expectedQHeart);
}

// Ensure excess balance is swept to QHeart wallet after settlement.
TEST(ContractPulse_Gameplay, QHeartHoldLimitExcessTransferred)
{
	ContractTestingPulse ctl;
	ctl.issuePulseSharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(5000000);
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	static constexpr uint64 holdLimit = 100000;
	static constexpr uint64 preFund = 500000;

	EXPECT_EQ(ctl.setQHeartHoldLimit(PULSE_QHEART_ISSUER, holdLimit).returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	ctl.transferQHeart(issuance, ctl.pulseSelf(), preFund);

	const id player = id::randomValue();
	ctl.transferQHeart(issuance, player, ticketPrice);
	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits = makePlayerDigits(0, 1, 2, 3, 4, 5);
	const PULSE::BuyTicket_output out = ctl.buyTicket(player, digits);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(PULSE::EReturnCode::SUCCESS));

	const m256i digest(0x11112222ULL, 0x33334444ULL, 0x55556666ULL, 0x77778888ULL);
	etalonTick.prevSpectrumDigest = digest;
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = deriveWinningDigits(ctl, digest);
	const uint64 prize = ctl.state()->callComputePrize(winning, digits);

	const uint64 walletBefore = ctl.qheartBalanceOf(PULSE_QHEART_ISSUER);
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());

	const PULSE::GetFees_output fees = ctl.getFees();
	const uint64 roundRevenue = ticketPrice;
	const uint64 devAmount = (roundRevenue * fees.devPercent) / 100;
	const uint64 burnAmount = (roundRevenue * fees.burnPercent) / 100;
	const uint64 shareholdersAmount = (roundRevenue * fees.shareholdersPercent) / 100;
	const uint64 qheartAmount = (roundRevenue * fees.qheartPercent) / 100;
	const uint64 dividendPerShare = shareholdersAmount / NUMBER_OF_COMPUTORS;
	const uint64 shareholdersPaid = dividendPerShare * NUMBER_OF_COMPUTORS;
	const uint64 feesTotal = devAmount + burnAmount + shareholdersPaid + qheartAmount;

	const uint64 balanceAfterFees = contractBefore - feesTotal;
	ASSERT_GE(balanceAfterFees, prize);
	const uint64 balanceAfterPrizes = balanceAfterFees - prize;
	const uint64 excess = (balanceAfterPrizes > holdLimit) ? (balanceAfterPrizes - holdLimit) : 0;
	const uint64 expectedContractAfter = balanceAfterPrizes - excess;
	const uint64 expectedWalletAfter = walletBefore + qheartAmount + excess;

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), expectedContractAfter);
	EXPECT_EQ(ctl.qheartBalanceOf(PULSE_QHEART_ISSUER), expectedWalletAfter);
}
