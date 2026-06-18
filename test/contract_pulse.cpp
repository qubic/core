#define NO_UEFI

#include "contract_testing.h"
#include <vector>

// Procedure/function indices (must match REGISTER_USER_FUNCTIONS_AND_PROCEDURES in `src/contracts/Pulse.h`).
constexpr uint16 PULSE_PROCEDURE_BUY_TICKET = 1;
constexpr uint16 PULSE_PROCEDURE_SET_PRICE = 2;
constexpr uint16 PULSE_PROCEDURE_SET_SCHEDULE = 3;
constexpr uint16 PULSE_PROCEDURE_SET_DRAW_HOUR = 4;
constexpr uint16 PULSE_PROCEDURE_SET_FEES = 5;
constexpr uint16 PULSE_PROCEDURE_SET_QHEART_HOLD_LIMIT = 6;
constexpr uint16 PULSE_PROCEDURE_BUY_RANDOM_TICKETS = 7;
constexpr uint16 PULSE_PROCEDURE_DEPOSIT_MANAGED_QHEART = 13;

constexpr uint16 QX_PROCEDURE_TRANSFER_SHARE_MANAGEMENT_RIGHTS = 9;

constexpr uint16 PULSE_FUNCTION_GET_TICKET_PRICE = 1;
constexpr uint16 PULSE_FUNCTION_GET_ROUND_STATE = 3;
constexpr uint16 PULSE_FUNCTION_GET_FEES = 4;
constexpr uint16 PULSE_FUNCTION_GET_QHEART_HOLD_LIMIT = 5;
constexpr uint16 PULSE_FUNCTION_GET_QHEART_WALLET = 6;
constexpr uint16 PULSE_FUNCTION_GET_WINNING_DIGITS = 7;
constexpr uint16 PULSE_FUNCTION_GET_BALANCE = 8;
constexpr uint16 PULSE_FUNCTION_GET_WINNERS = 9;
constexpr uint16 PULSE_FUNCTION_VALIDATE_DIGITS = 12;
constexpr uint16 PULSE_FUNCTION_GET_PLAYERS = 13;
constexpr uint16 PULSE_FUNCTION_GET_PRIZE_TABLE = 14;

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
class PULSEChecker : public PULSE, public PULSE::StateData
{
public:
	const QPI::ContractState<StateData, PULSE_CONTRACT_INDEX>& asState() const
	{
		return *reinterpret_cast<const QPI::ContractState<StateData, PULSE_CONTRACT_INDEX>*>(static_cast<const StateData*>(this));
	}
	QPI::ContractState<StateData, PULSE_CONTRACT_INDEX>& asMutState()
	{
		return *reinterpret_cast<QPI::ContractState<StateData, PULSE_CONTRACT_INDEX>*>(static_cast<StateData*>(this));
	}

	uint64 getTicketCounter() const { return ticketCounter; }
	uint64 getTicketPriceInternal() const { return ticketPrice; }
	uint64 getQHeartHoldLimitInternal() const { return qheartHoldLimit; }
	uint32 getLastDrawDateStamp() const { return lastDrawDateStamp; }
	uint8 getScheduleInternal() const { return schedule; }
	uint8 getDrawHourInternal() const { return drawHour; }
	uint8 getDevPercentInternal() const { return devPercent; }
	uint8 getBurnPercentInternal() const { return burnPercent; }
	uint8 getShareholdersPercentInternal() const { return shareholdersPercent; }
	uint8 getRLShareholdersPercentInternal() const { return rlShareholdersPercent; }
	const id& getTeamAddressInternal() const { return teamAddress; }
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& getLastWinningDigits() const { return lastWinningDigits; }
	Ticket getTicket(uint64 index) const { return tickets.get(index); }
	id getQHeartIssuer() const { return qheartIssuer; }

	void setTicketCounter(uint64 value) { ticketCounter = value; }
	void setTicketPriceInternal(uint64 value) { ticketPrice = value; }
	void setLastDrawDateStamp(uint32 value) { lastDrawDateStamp = value; }
	void setScheduleInternal(uint8 value) { schedule = value; }
	void setDrawHourInternal(uint8 value) { drawHour = value; }

	NextEpochData& nextEpochDataRef() { return nextEpochData; }

	void setTicketDirect(uint64 index, const id& player, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits)
	{
		Ticket ticket{player, digits};

		tickets.set(index, ticket);
	}

	void forceSelling(bool enable) { enableBuyTicket(asMutState(), enable); }
	bool isSelling() const { return isSellingOpen(asState()); }

	ValidateDigits_output callValidateDigits(const QPI::QpiContextFunctionCall& qpi, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits) const
	{
		ValidateDigits_input input{};
		ValidateDigits_output output{};
		ValidateDigits_locals locals{};
		input.digits = digits;
		ValidateDigits(qpi, asState(), input, output, locals);
		return output;
	}

	GetRandomDigits_output callGetRandomDigits(const QPI::QpiContextFunctionCall& qpi, uint64 seed) const
	{
		GetRandomDigits_input input{};
		GetRandomDigits_output output{};
		GetRandomDigits_locals locals{};
		input.seed = seed;
		GetRandomDigits(qpi, asState(), input, output, locals);
		return output;
	}

	PrepareRandomTickets_output callPrepareRandomTickets(const QPI::QpiContextProcedureCall& qpi, uint16 count)
	{
		PrepareRandomTickets_input input{};
		PrepareRandomTickets_output output{};
		PrepareRandomTickets_locals locals{};
		input.count = count;
		PrepareRandomTickets(qpi, asMutState(), input, output, locals);
		return output;
	}

	ChargeTicketsFromPlayer_output callChargeTicketsFromPlayer(const QPI::QpiContextProcedureCall& qpi, const id& player, uint16 count)
	{
		ChargeTicketsFromPlayer_input input{};
		ChargeTicketsFromPlayer_output output{};
		ChargeTicketsFromPlayer_locals locals{};
		input.player = player;
		input.count = count;
		ChargeTicketsFromPlayer(qpi, asMutState(), input, output, locals);
		return output;
	}

	AllocateRandomTickets_output callAllocateRandomTickets(const QPI::QpiContextProcedureCall& qpi, const id& player, uint16 count)
	{
		AllocateRandomTickets_input input{};
		AllocateRandomTickets_output output{};
		AllocateRandomTickets_locals locals{};
		input.player = player;
		input.count = count;
		AllocateRandomTickets(qpi, asMutState(), input, output, locals);
		return output;
	}

	uint64 callGetLeftAlignedReward(uint8 matches) const { return getLeftAlignedReward(asState(), matches); }
	uint64 callGetAnyPositionReward(uint8 matches) const { return getAnyPositionReward(asState(), matches); }
	uint64 callComputePrize(const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits)
	{
		Ticket ticket{};
		ticket.digits = digits;
		ComputePrize_locals locals{};
		return computePrize(asMutState(), ticket, winning, locals);
	}
};

class ContractTestingPulse : protected ContractTesting
{
public:
	ContractTestingPulse()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(RANDOM);
		INIT_CONTRACT(PULSE);
		system.epoch = contractDescriptions[RANDOM_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(RANDOM_CONTRACT_INDEX, INITIALIZE);
		system.epoch = contractDescriptions[PULSE_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(PULSE_CONTRACT_INDEX, INITIALIZE);
	}

	PULSEChecker* state() { return reinterpret_cast<PULSEChecker*>(contractStates[PULSE_CONTRACT_INDEX]); }
	const PULSEChecker* state() const { return reinterpret_cast<PULSEChecker*>(contractStates[PULSE_CONTRACT_INDEX]); }
	RANDOM::StateData* randomState() { return reinterpret_cast<RANDOM::StateData*>(contractStates[RANDOM_CONTRACT_INDEX]); }

	void qxInitialize()
	{
		INIT_CONTRACT(QX);
		callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
	}

	id pulseSelf() const { return id(PULSE_CONTRACT_INDEX, 0, 0, 0); }

	PULSE::GetTicketPrice_output getTicketPrice()
	{
		PULSE::GetTicketPrice_input input{};
		PULSE::GetTicketPrice_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_TICKET_PRICE, input, output);
		return output;
	}

	PULSE::GetRoundState_output getRoundState()
	{
		PULSE::GetRoundState_input input{};
		PULSE::GetRoundState_output output{};
		callFunction(PULSE_CONTRACT_INDEX, PULSE_FUNCTION_GET_ROUND_STATE, input, output);
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

	PULSE::BuyTicket_output buyTicket(const id& user, const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED>& digits)
	{
		ensureUserEnergy(user);
		PULSE::BuyTicket_input input{};
		input.digits = digits;
		PULSE::BuyTicket_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_BUY_TICKET, input, output, user, 0))
		{
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
		}
		return output;
	}

	QPI::bit_4096 seedRandomEntropy(uint64 seed)
	{
		QPI::bit_4096 entropy{};
		for (uint64 i = 0; i < RL_RANDOM_ENTROPY_BITS; ++i)
		{
			entropy.set(i, ((seed + i) & 1ULL) != 0);
		}

		const uint32 stream = (system.tick + 2u) % 3u;
		randomState()->entropy.set(stream * 10u + RL_RANDOM_COLLATERAL_TIER, entropy);
		const uint32 drawTick = system.tick + (PULSE_TICK_UPDATE_PERIOD - (system.tick % PULSE_TICK_UPDATE_PERIOD));
		const uint32 drawStream = (drawTick + 2u) % 3u;
		randomState()->entropy.set(drawStream * 10u + RL_RANDOM_COLLATERAL_TIER, entropy);
		return entropy;
	}

	void seedRandomEntropy(const QPI::bit_4096& entropy)
	{
		const uint32 stream = (system.tick + 2u) % 3u;
		randomState()->entropy.set(stream * 10u + RL_RANDOM_COLLATERAL_TIER, entropy);
		const uint32 drawTick = system.tick + (PULSE_TICK_UPDATE_PERIOD - (system.tick % PULSE_TICK_UPDATE_PERIOD));
		const uint32 drawStream = (drawTick + 2u) % 3u;
		randomState()->entropy.set(drawStream * 10u + RL_RANDOM_COLLATERAL_TIER, entropy);
	}

	Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> computeWinningDigitsForEntropy(const QPI::bit_4096& entropy,
	                                                                          uint64 ticketCounter = static_cast<uint64>(-1))
	{
		PULSE::SettleRound_locals::SettleEntropyData settleEntropyData{};
		settleEntropyData.entropy = entropy;
		settleEntropyData.ticketCounter = (ticketCounter == static_cast<uint64>(-1)) ? state()->getTicketCounter() : ticketCounter;
		settleEntropyData.ticketPrice = state()->getTicketPriceInternal();
		settleEntropyData.tick = system.tick + (PULSE_TICK_UPDATE_PERIOD - (system.tick % PULSE_TICK_UPDATE_PERIOD));
		settleEntropyData.epoch = system.epoch;

		m256i randomDigest;
		KangarooTwelve(reinterpret_cast<const uint8*>(&settleEntropyData), sizeof(settleEntropyData), reinterpret_cast<uint8*>(&randomDigest),
		               sizeof(m256i));

		QpiContextUserFunctionCall qpiFunc(PULSE_CONTRACT_INDEX);
		primeQpiFunctionContext(qpiFunc);
		return state()->callGetRandomDigits(qpiFunc, randomDigest.m256i_u64[0]).digits;
	}

	PULSE::BuyRandomTickets_output buyRandomTickets(const id& user, uint16 count,
	                                                sint64 invocationReward = static_cast<sint64>(RL_RANDOM_ENTROPY_FEE))
	{
		ensureUserEnergy(user, invocationReward);
		PULSE::BuyRandomTickets_input input{};
		input.count = count;
		PULSE::BuyRandomTickets_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_BUY_RANDOM_TICKETS, input, output, user, invocationReward))
		{
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
		}
		return output;
	}

	PULSE::BuyRandomTickets_output buyRandomTicketsWithoutAutoEnergy(const id& user, uint16 count, sint64 invocationReward)
	{
		PULSE::BuyRandomTickets_input input{};
		input.count = count;
		PULSE::BuyRandomTickets_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_BUY_RANDOM_TICKETS, input, output, user, invocationReward))
		{
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
		}
		return output;
	}

	PULSE::DepositManagedQHeart_output depositManagedQHeart(const id& user, sint64 amount)
	{
		ensureUserEnergy(user);
		PULSE::DepositManagedQHeart_input input{};
		input.amount = amount;
		PULSE::DepositManagedQHeart_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_DEPOSIT_MANAGED_QHEART, input, output, user, 0))
		{
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
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
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
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
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
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
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
		}
		return output;
	}

	PULSE::SetFees_output setFees(const id& invocator, uint8 dev, uint8 burn, uint8 shareholders, uint8 rlShareholders)
	{
		ensureUserEnergy(invocator);
		PULSE::SetFees_input input{};
		input.devPercent = dev;
		input.burnPercent = burn;
		input.shareholdersPercent = shareholders;
		input.rlShareholdersPercent = rlShareholders;
		PULSE::SetFees_output output{};
		if (!invokeUserProcedure(PULSE_CONTRACT_INDEX, PULSE_PROCEDURE_SET_FEES, input, output, invocator, 0))
		{
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
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
			output.returnCode = PULSE::EReturnCode::UNKNOWN_ERROR;
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
		const sint64 issued = issueAsset(state()->getQHeartIssuer(), name, 0, unit, totalShares, PULSE_CONTRACT_INDEX, &info.issuanceIndex,
		                                 &info.ownershipIndex, &info.possessionIndex);
		EXPECT_EQ(issued, totalShares);
		return info;
	}

	QHeartIssuance issueQHeartOnQx(sint64 totalShares)
	{
		static constexpr char name[7] = {'Q', 'H', 'E', 'A', 'R', 'T', 0};
		static constexpr char unit[7] = {};
		QHeartIssuance info{};
		const sint64 issued = issueAsset(state()->getQHeartIssuer(), name, 0, unit, totalShares, QX_CONTRACT_INDEX, &info.issuanceIndex,
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

	void issueRandomLotterySharesTo(const id& holder, unsigned int shares)
	{
		std::vector<std::pair<m256i, unsigned int>> initialShares;
		initialShares.emplace_back(holder, shares);
		issueContractShares(RL_CONTRACT_INDEX, initialShares, false);
	}

	sint64 transferQHeartManagementRightsToPulse(const id& currentOwner, sint64 shares)
	{
		ensureUserEnergy(currentOwner);
		QX::TransferShareManagementRights_input input{};
		QX::TransferShareManagementRights_output output{};
		input.asset.assetName = PULSE_QHEART_ASSET_NAME;
		input.asset.issuer = state()->getQHeartIssuer();
		input.numberOfShares = shares;
		input.newManagingContractIndex = PULSE_CONTRACT_INDEX;
		if (!invokeUserProcedure(QX_CONTRACT_INDEX, QX_PROCEDURE_TRANSFER_SHARE_MANAGEMENT_RIGHTS, input, output, currentOwner, 0))
		{
			return 0;
		}
		return output.transferredNumberOfShares;
	}

	uint64 qheartBalanceOf(const id& owner) const
	{
		const long long balance =
		    numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state()->getQHeartIssuer(), owner, owner, PULSE_CONTRACT_INDEX, PULSE_CONTRACT_INDEX);
		return (balance > 0) ? static_cast<uint64>(balance) : 0;
	}

	uint64 managedQheartBalanceOf(const id& owner, unsigned int managingContractIndex) const
	{
		const long long balance =
		    numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state()->getQHeartIssuer(), owner, owner, managingContractIndex, managingContractIndex);
		return (balance > 0) ? static_cast<uint64>(balance) : 0;
	}

private:
	static void ensureUserEnergy(const id& user, sint64 invocationReward = 0) { increaseEnergy(user, invocationReward + 1); }
};

namespace
{
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
	RL::makeDateStamp(25, 1, 10, stamp);
	EXPECT_EQ(stamp, static_cast<uint32>(25 << 9 | 1 << 5 | 10));
	EXPECT_EQ(RL::min<uint32>(3, 5), 3u);
	EXPECT_EQ(RL::max<uint32>(3, 5), 5u);

	uint64 mixed1 = 0;
	uint64 mixed2 = 0;
	RL::mix64(0x12345678ULL, mixed1);
	RL::mix64(0x12345678ULL, mixed2);
	EXPECT_EQ(mixed1, mixed2);

	uint64 d1 = 0;
	uint64 d2 = 0;
	RL::deriveOne(0xABCDEFULL, 0, d1);
	RL::deriveOne(0xABCDEFULL, 1, d2);
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

	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(6), 100u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(5), 10u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(4), 3u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(3), 1u * ctl.getTicketPrice().ticketPrice);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(2), 0u);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(1), 0u);
	EXPECT_EQ(ctl.state()->callGetAnyPositionReward(0), 0u);
}

// Ensure computePrize picks the higher of left-aligned or any-position rewards.
TEST(ContractPulse_Static, ComputePrizeSelectsBestReward)
{
	ContractTestingPulse ctl;
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> winning = makePlayerDigits(0, 1, 2, 3, 4, 5);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> exact = makePlayerDigits(0, 1, 2, 3, 4, 5);
	EXPECT_EQ(ctl.state()->callComputePrize(winning, exact), 2000u * ctl.getTicketPrice().ticketPrice);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> permuted = makePlayerDigits(5, 4, 3, 2, 1, 0);
	EXPECT_EQ(ctl.state()->callComputePrize(winning, permuted), 100u * ctl.getTicketPrice().ticketPrice);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> none = makePlayerDigits(9, 9, 9, 9, 9, 9);
	EXPECT_EQ(ctl.state()->callComputePrize(winning, none), 0u);
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
	data.newQHeartHoldLimit = 999;

	data.apply(*ctl.state());
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), 123u);
	EXPECT_EQ(ctl.state()->getScheduleInternal(), 0xAA);
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), 7u);
	EXPECT_EQ(ctl.state()->getDevPercentInternal(), 11u);
	EXPECT_EQ(ctl.state()->getBurnPercentInternal(), 22u);
	EXPECT_EQ(ctl.state()->getShareholdersPercentInternal(), 33u);
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

// Validate digit range checks for ticket input.
TEST(ContractPulse_Private, ValidateDigitsAcceptsRangeAndRejectsOutOfRange)
{
	ContractTestingPulse ctl;
	QpiContextUserFunctionCall qpi(PULSE_CONTRACT_INDEX);
	primeQpiFunctionContext(qpi);

	const PULSE::ValidateDigits_output valid = ctl.state()->callValidateDigits(qpi, makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_TRUE(valid.isValid);

	const uint8 invalidValue = static_cast<uint8>(PULSE_MAX_DIGIT + 1);
	const PULSE::ValidateDigits_output invalid = ctl.state()->callValidateDigits(qpi, makePlayerDigits(0, 1, 2, 3, 4, invalidValue));
	EXPECT_FALSE(invalid.isValid);
}

// Validate PrepareRandomTickets error cases.
TEST(ContractPulse_Private, PrepareRandomTicketsRejectsInvalidInputs)
{
	ContractTestingPulse ctl;
	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, ctl.state()->getQHeartIssuer(), 0);
	primeQpiProcedureContext(qpi);

	PULSE::PrepareRandomTickets_output out = ctl.state()->callPrepareRandomTickets(qpi, 0);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);

	ctl.endEpoch();
	out = ctl.state()->callPrepareRandomTickets(qpi, 1);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_SELLING_CLOSED);
}

// Guard sold-out behavior in PrepareRandomTickets.
TEST(ContractPulse_Private, PrepareRandomTicketsRejectsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, ctl.state()->getQHeartIssuer(), 0);
	primeQpiProcedureContext(qpi);
	const PULSE::PrepareRandomTickets_output out = ctl.state()->callPrepareRandomTickets(qpi, 1);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_ALL_SOLD_OUT);
}

// Validate ChargeTicketsFromPlayer rejects invalid inputs and insufficient balance.
TEST(ContractPulse_Private, ChargeTicketsFromPlayerRejectsInvalidOrInsufficient)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, ctl.state()->getQHeartIssuer(), 0);
	primeQpiProcedureContext(qpi);

	PULSE::ChargeTicketsFromPlayer_output out = ctl.state()->callChargeTicketsFromPlayer(qpi, user, 0);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);

	out = ctl.state()->callChargeTicketsFromPlayer(qpi, id::zero(), 1);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);

	out = ctl.state()->callChargeTicketsFromPlayer(qpi, user, 2);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_INVALID_PRICE);
}

// Validate AllocateRandomTickets rejects invalid inputs and closed/sold-out states.
TEST(ContractPulse_Private, AllocateRandomTicketsRejectsInvalidOrClosed)
{
	ContractTestingPulse ctl;
	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, ctl.state()->getQHeartIssuer(), 0);
	primeQpiProcedureContext(qpi);

	PULSE::AllocateRandomTickets_output out = ctl.state()->callAllocateRandomTickets(qpi, id::zero(), 1);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);

	out = ctl.state()->callAllocateRandomTickets(qpi, id::randomValue(), 0);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);

	ctl.endEpoch();
	out = ctl.state()->callAllocateRandomTickets(qpi, id::randomValue(), 1);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_SELLING_CLOSED);
}

// Guard AllocateRandomTickets sold-out path.
TEST(ContractPulse_Private, AllocateRandomTicketsRejectsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	QpiContextUserProcedureCall qpi(PULSE_CONTRACT_INDEX, ctl.state()->getQHeartIssuer(), 0);
	primeQpiProcedureContext(qpi);
	const PULSE::AllocateRandomTickets_output out = ctl.state()->callAllocateRandomTickets(qpi, id::randomValue(), 1);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_ALL_SOLD_OUT);
}

// ============================================================================
// PUBLIC FUNCTIONS AND PROCEDURES
// ============================================================================

// Confirm defaults are visible through the public API after init.
TEST(ContractPulse_Public, GettersReturnDefaultsAfterInitialize)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, PULSE_TICKET_PRICE_DEFAULT);
	const PULSE::GetRoundState_output roundState = ctl.getRoundState();
	EXPECT_EQ(roundState.schedule, PULSE_DEFAULT_SCHEDULE);
	EXPECT_EQ(roundState.drawHour, PULSE_DEFAULT_DRAW_HOUR);
	EXPECT_EQ(ctl.getQHeartHoldLimit().qheartHoldLimit, PULSE_DEFAULT_QHEART_HOLD_LIMIT);

	const PULSE::GetFees_output& fees = ctl.getFees();
	EXPECT_EQ(fees.devPercent, PULSE_DEFAULT_DEV_PERCENT);
	EXPECT_EQ(fees.burnPercent, PULSE_DEFAULT_BURN_PERCENT);
	EXPECT_EQ(fees.shareholdersPercent, PULSE_DEFAULT_SHAREHOLDERS_PERCENT);
	EXPECT_EQ(fees.rlShareholdersPercent, PULSE_DEFAULT_RL_SHAREHOLDERS_PERCENT);
	EXPECT_EQ(fees.returnCode, PULSE::EReturnCode::SUCCESS);

	const PULSE::GetWinningDigits_output& win = ctl.getWinningDigits();
	for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
	{
		EXPECT_EQ(win.digits.get(i), 0u);
	}
	EXPECT_EQ(ctl.getBalance().balance, 0u);
	EXPECT_EQ(ctl.getQHeartWallet().wallet, ctl.state()->getQHeartIssuer());
}

// Guard admin-only price changes and deferred apply.
TEST(ContractPulse_Public, SetPriceGuardsAccessAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setPrice(id::randomValue(), 123).returnCode, PULSE::EReturnCode::ACCESS_DENIED);
	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), 0).returnCode, PULSE::EReturnCode::TICKET_INVALID_PRICE);

	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), 555).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), PULSE_TICKET_PRICE_DEFAULT);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getTicketPriceInternal(), 555u);
}

// Ensure schedule validation and deferred apply are enforced.
TEST(ContractPulse_Public, SetScheduleValidatesAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setSchedule(id::randomValue(), 1).returnCode, PULSE::EReturnCode::ACCESS_DENIED);
	EXPECT_EQ(ctl.setSchedule(ctl.state()->getQHeartIssuer(), 0).returnCode, PULSE::EReturnCode::INVALID_VALUE);

	EXPECT_EQ(ctl.setSchedule(ctl.state()->getQHeartIssuer(), 0x7F).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getScheduleInternal(), PULSE_DEFAULT_SCHEDULE);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getScheduleInternal(), 0x7Fu);
}

// Ensure draw hour range checks and deferred apply are enforced.
TEST(ContractPulse_Public, SetDrawHourValidatesAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setDrawHour(id::randomValue(), 12).returnCode, PULSE::EReturnCode::ACCESS_DENIED);
	EXPECT_EQ(ctl.setDrawHour(ctl.state()->getQHeartIssuer(), 24).returnCode, PULSE::EReturnCode::INVALID_VALUE);

	EXPECT_EQ(ctl.setDrawHour(ctl.state()->getQHeartIssuer(), 9).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), PULSE_DEFAULT_DRAW_HOUR);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getDrawHourInternal(), 9u);
}

// Protect against invalid fee splits and apply on epoch end.
TEST(ContractPulse_Public, SetFeesValidatesAndAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setFees(id::randomValue(), 1, 2, 3, 4).returnCode, PULSE::EReturnCode::ACCESS_DENIED);
	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), 60, 60, 0, 0).returnCode, PULSE::EReturnCode::INVALID_VALUE);

	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), 11, 22, 33, 6).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getDevPercentInternal(), PULSE_DEFAULT_DEV_PERCENT);
	EXPECT_EQ(ctl.state()->getBurnPercentInternal(), PULSE_DEFAULT_BURN_PERCENT);
	EXPECT_EQ(ctl.state()->getShareholdersPercentInternal(), PULSE_DEFAULT_SHAREHOLDERS_PERCENT);
	EXPECT_EQ(ctl.state()->getRLShareholdersPercentInternal(), PULSE_DEFAULT_RL_SHAREHOLDERS_PERCENT);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getDevPercentInternal(), 11u);
	EXPECT_EQ(ctl.state()->getBurnPercentInternal(), 22u);
	EXPECT_EQ(ctl.state()->getShareholdersPercentInternal(), 33u);
	EXPECT_EQ(ctl.state()->getRLShareholdersPercentInternal(), 6u);
}

// Ensure hold-limit changes do not affect the current round.
TEST(ContractPulse_Public, SetQHeartHoldLimitAppliesOnEndEpoch)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setQHeartHoldLimit(id::randomValue(), 100).returnCode, PULSE::EReturnCode::ACCESS_DENIED);
	EXPECT_EQ(ctl.setQHeartHoldLimit(ctl.state()->getQHeartIssuer(), 1234).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getQHeartHoldLimitInternal(), PULSE_DEFAULT_QHEART_HOLD_LIMIT);

	ctl.endEpoch();
	EXPECT_EQ(ctl.state()->getQHeartHoldLimitInternal(), 1234u);
}

// Ensure getters report newly applied config values after epoch end.
TEST(ContractPulse_Public, GettersReflectAppliedChanges)
{
	ContractTestingPulse ctl;
	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), 555).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setSchedule(ctl.state()->getQHeartIssuer(), 0x7F).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setDrawHour(ctl.state()->getQHeartIssuer(), 9).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), 11, 22, 33, 6).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setQHeartHoldLimit(ctl.state()->getQHeartIssuer(), 4321).returnCode, PULSE::EReturnCode::SUCCESS);

	ctl.endEpoch();

	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, 555u);
	const PULSE::GetRoundState_output roundState = ctl.getRoundState();
	EXPECT_EQ(roundState.schedule, 0x7Fu);
	EXPECT_EQ(roundState.drawHour, 9u);
	EXPECT_EQ(ctl.getQHeartHoldLimit().qheartHoldLimit, 4321u);

	const PULSE::GetFees_output fees = ctl.getFees();
	EXPECT_EQ(fees.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(fees.devPercent, 11u);
	EXPECT_EQ(fees.burnPercent, 22u);
	EXPECT_EQ(fees.shareholdersPercent, 33u);
	EXPECT_EQ(fees.rlShareholdersPercent, 6u);
}

// Prevent ticket purchases outside the selling window.
TEST(ContractPulse_Public, BuyTicketWhenSellingClosedFails)
{
	ContractTestingPulse ctl;
	const PULSE::BuyTicket_output& out = ctl.buyTicket(id::randomValue(), makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_SELLING_CLOSED);
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
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_NUMBERS);
}

// Enforce hard cap on ticket count.
TEST(ContractPulse_Public, BuyTicketFailsWhenSoldOut)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();
	ctl.state()->setTicketCounter(PULSE_MAX_NUMBER_OF_PLAYERS);

	const PULSE::BuyTicket_output& out = ctl.buyTicket(id::randomValue(), makePlayerDigits(0, 1, 2, 3, 4, 5));
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_ALL_SOLD_OUT);
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
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_INVALID_PRICE);
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
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::SUCCESS);
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
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_SELLING_CLOSED);
}

// Require the caller to fund one entropy purchase per random-ticket batch.
TEST(ContractPulse_Public, BuyRandomTicketsRejectsMissingEntropyFee)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 1, 0);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore);
}

// Reject empty batch requests to avoid no-op transfers.
TEST(ContractPulse_Public, BuyRandomTicketsRejectsZeroCount)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const id user = id::randomValue();
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 0);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::INVALID_VALUE);
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
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_ALL_SOLD_OUT);
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
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::TICKET_INVALID_PRICE);
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
	ctl.seedRandomEntropy(0xAAAABBBBULL);

	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, ticketCount);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(static_cast<uint32>(ticketCount)));
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

TEST(ContractPulse_Public, BuyRandomTicketsRefundsQHeartAndEntropyFeeWhenRandomReturnsZero)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);
	increaseEnergy(user, RL_RANDOM_ENTROPY_FEE);

	QPI::bit_4096 zeroEntropy{};
	ctl.seedRandomEntropy(zeroEntropy);
	const uint64 userQHeartBefore = ctl.qheartBalanceOf(user);
	const uint64 contractQHeartBefore = ctl.qheartBalanceOf(ctl.pulseSelf());
	const uint64 userQuBefore = getBalance(user);
	const uint64 randomEarnedBefore = ctl.randomState()->earnedAmount;

	const PULSE::BuyRandomTickets_output out =
	    ctl.buyRandomTicketsWithoutAutoEnergy(user, 1, static_cast<sint64>(RL_RANDOM_ENTROPY_FEE));

	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::UNKNOWN_ERROR);
	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
	EXPECT_EQ(ctl.qheartBalanceOf(user), userQHeartBefore);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractQHeartBefore);
	EXPECT_EQ(getBalance(user), userQuBefore);
	EXPECT_EQ(ctl.randomState()->earnedAmount, randomEarnedBefore);
}

// Refund only the qu sent above the one entropy fee required for the batch.
TEST(ContractPulse_Public, BuyRandomTicketsRefundsEntropyFeeOverpayment)
{
	ContractTestingPulse ctl;
	ctl.setDateTime(2025, 1, 10, 12);
	ctl.beginEpoch();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	const id user = id::randomValue();
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, user, ticketPrice);
	ctl.seedRandomEntropy(0xBBBBULL);

	static constexpr sint64 overpayment = 1234;
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 1, static_cast<sint64>(RL_RANDOM_ENTROPY_FEE) + overpayment);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getTicketCounter(), 1u);
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
	ctl.seedRandomEntropy(0xAAAAULL);

	const uint64 userBefore = ctl.qheartBalanceOf(user);
	const PULSE::BuyRandomTickets_output out = ctl.buyRandomTickets(user, 5);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getTicketCounter(), static_cast<uint64>(PULSE_MAX_NUMBER_OF_PLAYERS));
	EXPECT_EQ(ctl.qheartBalanceOf(user), userBefore - (ticketPrice * 2));
}

// Ensure balance getter reflects actual QHeart wallet holdings.
TEST(ContractPulse_Public, GetBalanceReportsQHeartWalletBalance)
{
	ContractTestingPulse ctl;
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	ctl.transferQHeart(issuance, ctl.pulseSelf(), 12345);
	EXPECT_EQ(ctl.getBalance().balance, 12345u);
}

// QX-managed QHEART can be re-managed by Pulse and then deposited into the Pulse wallet.
TEST(ContractPulse_Public, DepositManagedQHeartAfterQxManagementTransfer)
{
	ContractTestingPulse ctl;
	ctl.qxInitialize();

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeartOnQx(1000000);
	const id user = id::randomValue();
	static constexpr sint64 depositAmount = 12345;

	ctl.transferQHeart(issuance, user, depositAmount);

	EXPECT_EQ(ctl.managedQheartBalanceOf(user, QX_CONTRACT_INDEX), static_cast<uint64>(depositAmount));
	EXPECT_EQ(ctl.managedQheartBalanceOf(user, PULSE_CONTRACT_INDEX), 0u);
	EXPECT_EQ(ctl.transferQHeartManagementRightsToPulse(user, depositAmount), depositAmount);
	EXPECT_EQ(ctl.managedQheartBalanceOf(user, QX_CONTRACT_INDEX), 0u);
	EXPECT_EQ(ctl.managedQheartBalanceOf(user, PULSE_CONTRACT_INDEX), static_cast<uint64>(depositAmount));

	const PULSE::DepositManagedQHeart_output deposit = ctl.depositManagedQHeart(user, depositAmount);
	EXPECT_EQ(deposit.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.managedQheartBalanceOf(user, PULSE_CONTRACT_INDEX), 0u);
	EXPECT_EQ(ctl.getBalance().balance, static_cast<uint64>(depositAmount));
}

// Report empty winner history before any draws.
TEST(ContractPulse_Public, GetWinnersReportsEmptyWhenNoWinners)
{
	ContractTestingPulse ctl;
	const PULSE::GetWinners_output winners = ctl.getWinners();
	EXPECT_EQ(winners.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(winners.winnersCounter, 0u);
}

// Confirm winner history records paid prizes.
TEST(ContractPulse_Public, GetWinnersReportsPaidTickets)
{
	ContractTestingPulse ctl;
	increaseEnergy(id(PULSE_CONTRACT_INDEX, 0, 0, 0), RL_RANDOM_ENTROPY_FEE);
	ctl.issuePulseSharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);

	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), 0, 0, 0, 0).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), 1).returnCode, PULSE::EReturnCode::SUCCESS);
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	ctl.transferQHeart(issuance, ctl.pulseSelf(), 10000);
	const QPI::bit_4096 entropy = ctl.seedRandomEntropy(0x2222ULL);
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = ctl.computeWinningDigitsForEntropy(entropy, 2);
	const uint8 missing = findMissingDigit(winning);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketA =
	    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), winning.get(3), winning.get(4), winning.get(5));
	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketB =
	    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), missing, missing, missing);

	const id playerA = id::randomValue();
	const id playerB = id::randomValue();
	ctl.transferQHeart(issuance, playerA, 1);
	ctl.transferQHeart(issuance, playerB, 1);
	EXPECT_EQ(ctl.buyTicket(playerA, ticketA).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.buyTicket(playerB, ticketB).returnCode, PULSE::EReturnCode::SUCCESS);

	const uint64 prizeA = ctl.state()->callComputePrize(winning, ticketA);
	const uint64 prizeB = ctl.state()->callComputePrize(winning, ticketB);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	const PULSE::GetWinners_output& winners = ctl.getWinners();
	EXPECT_EQ(winners.returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(winners.winnersCounter, 2u);
	EXPECT_EQ(winners.winners.get(0).winnerAddress, playerA);
	EXPECT_EQ(winners.winners.get(0).revenue, prizeA);
	EXPECT_EQ(winners.winners.get(1).winnerAddress, playerB);
	EXPECT_EQ(winners.winners.get(1).revenue, prizeB);
}

TEST(ContractPulse_Public, SettleRound_ZeroEntropy_ReturnsTicketsAndDoesNotRecordWinners)
{
	ContractTestingPulse ctl;
	increaseEnergy(ctl.pulseSelf(), RL_RANDOM_ENTROPY_FEE);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	const id playerA = id::randomValue();
	const id playerB = id::randomValue();
	ctl.transferQHeart(issuance, playerA, PULSE_TICKET_PRICE_DEFAULT);
	ctl.transferQHeart(issuance, playerB, PULSE_TICKET_PRICE_DEFAULT);
	const uint64 playerABefore = ctl.qheartBalanceOf(playerA);
	const uint64 playerBBefore = ctl.qheartBalanceOf(playerB);
	const uint64 contractQHeartBefore = ctl.qheartBalanceOf(ctl.pulseSelf());

	ASSERT_EQ(ctl.buyTicket(playerA, makePlayerDigits(0, 1, 2, 3, 4, 5)).returnCode, PULSE::EReturnCode::SUCCESS);
	ASSERT_EQ(ctl.buyTicket(playerB, makePlayerDigits(1, 2, 3, 4, 5, 6)).returnCode, PULSE::EReturnCode::SUCCESS);
	ASSERT_EQ(ctl.state()->getTicketCounter(), 2u);

	const uint64 winnersBefore = ctl.getWinners().winnersCounter;
	const uint64 randomEarnedBefore = ctl.randomState()->earnedAmount;
	QPI::bit_4096 zeroEntropy{};
	ctl.seedRandomEntropy(zeroEntropy);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.state()->getTicketCounter(), 0u);
	EXPECT_EQ(ctl.qheartBalanceOf(playerA), playerABefore);
	EXPECT_EQ(ctl.qheartBalanceOf(playerB), playerBBefore);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractQHeartBefore);
	EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBefore);
	EXPECT_EQ(ctl.randomState()->earnedAmount, randomEarnedBefore);
	EXPECT_EQ(getBalance(ctl.pulseSelf()), RL_RANDOM_ENTROPY_FEE);
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

// Ensure draw is skipped before the configured draw hour.
TEST(ContractPulse_System, BeginTickSkipsBeforeDrawHour)
{
	ContractTestingPulse ctl;
	ctl.state()->setDrawHourInternal(23);

	const id player = id::randomValue();
	ctl.state()->setTicketDirect(0, player, makePlayerDigits(0, 1, 2, 3, 4, 5));
	ctl.state()->setTicketCounter(1);

	ctl.setDateTime(2025, 1, 10, 12);
	const uint32 lastStampBefore = ctl.state()->getLastDrawDateStamp();
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.state()->getTicketCounter(), 1u);
	EXPECT_EQ(ctl.state()->getLastDrawDateStamp(), lastStampBefore);
}

// Skip draws on non-scheduled days (excluding Wednesday fallback).
TEST(ContractPulse_System, BeginTickSkipsWhenNotScheduledDay)
{
	ContractTestingPulse ctl;
	ctl.state()->setDrawHourInternal(1);

	const id player = id::randomValue();
	ctl.state()->setTicketDirect(0, player, makePlayerDigits(0, 1, 2, 3, 4, 5));
	ctl.state()->setTicketCounter(1);

	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> beforeWinning = ctl.state()->getLastWinningDigits();

	ctl.setDateTime(2025, 1, 11, 12);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.state()->getTicketCounter(), 1u);
	for (uint64 i = 0; i < PULSE_WINNING_DIGITS; ++i)
	{
		EXPECT_EQ(ctl.state()->getLastWinningDigits().get(i), beforeWinning.get(i));
	}
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
	ctl.seedRandomEntropy(0x1010ULL);
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
		increaseEnergy(id(PULSE_CONTRACT_INDEX, 0, 0, 0), RL_RANDOM_ENTROPY_FEE);

		ctl.setDateTime(2025, 1, rounds[r].startDay, 12);
		ctl.beginEpoch();

		const QPI::bit_4096 entropy = ctl.seedRandomEntropy(0x1111ULL + r);
		const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = ctl.computeWinningDigitsForEntropy(entropy, 4);

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
			EXPECT_EQ(out.returnCode, PULSE::EReturnCode::SUCCESS);

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
	increaseEnergy(id(PULSE_CONTRACT_INDEX, 0, 0, 0), RL_RANDOM_ENTROPY_FEE);

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(2000000);

	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), 0, 0, 0, 0).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), 1).returnCode, PULSE::EReturnCode::SUCCESS);
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	static constexpr uint64 preFund = 1000;
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	ctl.transferQHeart(issuance, ctl.pulseSelf(), preFund);

	const QPI::bit_4096 entropy = ctl.seedRandomEntropy(0x1234ULL);
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = ctl.computeWinningDigitsForEntropy(entropy, 2);
	const uint8 missing = findMissingDigit(winning);

	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketA =
	    makePlayerDigits(winning.get(0), winning.get(1), winning.get(2), winning.get(3), winning.get(4), winning.get(5));
	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> ticketB =
	    makePlayerDigits(winning.get(0), winning.get(2), winning.get(4), winning.get(6), winning.get(8), missing);

	const id playerA = id::randomValue();
	const id playerB = id::randomValue();
	ctl.transferQHeart(issuance, playerA, ticketPrice);
	ctl.transferQHeart(issuance, playerB, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(playerA, ticketA).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.buyTicket(playerB, ticketB).returnCode, PULSE::EReturnCode::SUCCESS);

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
	// ctl.seedRandomEntropy(0x5151ULL);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.qheartBalanceOf(playerA), balanceAfterBuyA + expectedA);
	EXPECT_EQ(ctl.qheartBalanceOf(playerB), balanceAfterBuyB + expectedB);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), contractBefore - (expectedA + expectedB));
}

// Validate fee distribution to dev and shareholders.
TEST(ContractPulse_Gameplay, FeesDistributedToDevAndShareholders)
{
	ContractTestingPulse ctl;
	increaseEnergy(id(PULSE_CONTRACT_INDEX, 0, 0, 0), RL_RANDOM_ENTROPY_FEE);

	const id shareholder = id::randomValue();
	ctl.issuePulseSharesTo(shareholder, NUMBER_OF_COMPUTORS);

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	static constexpr uint8 devPercent = 10;
	static constexpr uint8 burnPercent = 0;
	static constexpr uint8 shareholdersPercent = 10;
	const uint64 ticketPrice = static_cast<uint64>(NUMBER_OF_COMPUTORS) * 10;

	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), devPercent, burnPercent, shareholdersPercent, 0).returnCode, PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), ticketPrice).returnCode, PULSE::EReturnCode::SUCCESS);
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	const id player = id::randomValue();
	ctl.transferQHeart(issuance, player, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(player, makePlayerDigits(0, 1, 2, 3, 4, 5)).returnCode, PULSE::EReturnCode::SUCCESS);

	const id devWallet = ctl.state()->getTeamAddressInternal();
	EXPECT_NE(devWallet, shareholder);
	EXPECT_NE(devWallet, ctl.state()->getQHeartIssuer());

	const uint64 devBefore = ctl.qheartBalanceOf(devWallet);
	const uint64 shareholderBefore = ctl.qheartBalanceOf(shareholder);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.seedRandomEntropy(0x6161ULL);
	ctl.forceBeginTick();

	const uint64 roundRevenue = ticketPrice;
	const uint64 expectedDev = (roundRevenue * devPercent) / 100;
	const uint64 expectedShareholders = (roundRevenue * shareholdersPercent) / 100;
	const uint64 dividendPerShare = expectedShareholders / NUMBER_OF_COMPUTORS;
	const uint64 expectedShareholderGain = dividendPerShare * NUMBER_OF_COMPUTORS;

	EXPECT_EQ(expectedShareholderGain, expectedShareholders);
	EXPECT_EQ(ctl.qheartBalanceOf(devWallet), devBefore + expectedDev);
	EXPECT_EQ(ctl.qheartBalanceOf(shareholder), shareholderBefore + expectedShareholderGain);
}

// Validate fee distribution to Random Lottery shareholders.
TEST(ContractPulse_Gameplay, FeesDistributedToRLShareholders)
{
	ContractTestingPulse ctl;
	increaseEnergy(id(PULSE_CONTRACT_INDEX, 0, 0, 0), RL_RANDOM_ENTROPY_FEE);
	ctl.seedRandomEntropy(0x6161ULL);

	const id rlShareholder = id::randomValue();
	ctl.issueRandomLotterySharesTo(rlShareholder, NUMBER_OF_COMPUTORS);

	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(1000000);
	static constexpr uint8 devPercent = 0;
	static constexpr uint8 burnPercent = 0;
	static constexpr uint8 shareholdersPercent = 0;
	static constexpr uint8 rlShareholdersPercent = 10;
	constexpr uint64 ticketPrice = static_cast<uint64>(NUMBER_OF_COMPUTORS) * 10ULL;

	EXPECT_EQ(ctl.setFees(ctl.state()->getQHeartIssuer(), devPercent, burnPercent, shareholdersPercent, rlShareholdersPercent).returnCode,
	          PULSE::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setPrice(ctl.state()->getQHeartIssuer(), ticketPrice).returnCode, PULSE::EReturnCode::SUCCESS);
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	const id player = id::randomValue();
	ctl.transferQHeart(issuance, player, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(player, makePlayerDigits(0, 1, 2, 3, 4, 5)).returnCode, PULSE::EReturnCode::SUCCESS);

	const uint64 rlBefore = ctl.qheartBalanceOf(rlShareholder);

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	static constexpr uint64 roundRevenue = ticketPrice;
	static constexpr uint64 expectedRL = (roundRevenue * rlShareholdersPercent) / 100;
	static constexpr uint64 dividendPerShare = expectedRL / NUMBER_OF_COMPUTORS;
	static constexpr uint64 expectedRLGain = dividendPerShare * NUMBER_OF_COMPUTORS;

	EXPECT_EQ(ctl.qheartBalanceOf(rlShareholder), rlBefore + expectedRLGain);
}

// Ensure excess balance is swept to QHeart wallet after settlement.
TEST(ContractPulse_Gameplay, QHeartHoldLimitExcessTransferred)
{
	ContractTestingPulse ctl;
	increaseEnergy(id(PULSE_CONTRACT_INDEX, 0, 0, 0), RL_RANDOM_ENTROPY_FEE);

	ctl.issuePulseSharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	ctl.issueRandomLotterySharesTo(id::randomValue(), NUMBER_OF_COMPUTORS);
	const ContractTestingPulse::QHeartIssuance& issuance = ctl.issueQHeart(5000000);
	const uint64 ticketPrice = ctl.getTicketPrice().ticketPrice;
	static constexpr uint64 holdLimit = 100000;
	static constexpr uint64 preFund = 500000;

	EXPECT_EQ(ctl.setQHeartHoldLimit(ctl.state()->getQHeartIssuer(), holdLimit).returnCode, PULSE::EReturnCode::SUCCESS);
	ctl.endEpoch();

	ctl.setDateTime(2025, 1, 9, 12);
	ctl.beginEpoch();

	ctl.transferQHeart(issuance, ctl.pulseSelf(), preFund);

	const id player = id::randomValue();
	ctl.transferQHeart(issuance, player, ticketPrice);
	const Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits = makePlayerDigits(0, 1, 2, 3, 4, 5);
	const PULSE::BuyTicket_output out = ctl.buyTicket(player, digits);
	EXPECT_EQ(out.returnCode, PULSE::EReturnCode::SUCCESS);

	const QPI::bit_4096 entropy = ctl.seedRandomEntropy(0x11112222ULL);
	const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winning = ctl.computeWinningDigitsForEntropy(entropy, 1);
	const uint64 prize = ctl.state()->callComputePrize(winning, digits);

	const uint64 walletBefore = ctl.qheartBalanceOf(ctl.state()->getQHeartIssuer());
	const uint64 contractBefore = ctl.qheartBalanceOf(ctl.pulseSelf());

	const PULSE::GetFees_output fees = ctl.getFees();
	const uint64 roundRevenue = ticketPrice;
	const uint64 devAmount = (roundRevenue * fees.devPercent) / 100;
	const uint64 burnAmount = (roundRevenue * fees.burnPercent) / 100;
	const uint64 shareholdersAmount = (roundRevenue * fees.shareholdersPercent) / 100;
	const uint64 rlShareholdersAmount = (roundRevenue * fees.rlShareholdersPercent) / 100;
	const uint64 dividendPerShare = shareholdersAmount / NUMBER_OF_COMPUTORS;
	const uint64 shareholdersPaid = dividendPerShare * NUMBER_OF_COMPUTORS;
	const uint64 rlDividendPerShare = rlShareholdersAmount / NUMBER_OF_COMPUTORS;
	const uint64 rlShareholdersPaid = rlDividendPerShare * NUMBER_OF_COMPUTORS;
	const uint64 feesTotal = devAmount + burnAmount + shareholdersPaid + rlShareholdersPaid;

	const uint64 balanceAfterFees = contractBefore - feesTotal;
	ASSERT_GE(balanceAfterFees, prize);
	const uint64 balanceAfterPrizes = balanceAfterFees - prize;
	const uint64 excess = (balanceAfterPrizes > holdLimit) ? (balanceAfterPrizes - holdLimit) : 0;
	const uint64 expectedContractAfter = balanceAfterPrizes - excess;
	const uint64 expectedWalletAfter = walletBefore + excess;

	ctl.setDateTime(2025, 1, 10, 12);
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.qheartBalanceOf(ctl.pulseSelf()), expectedContractAfter);
	EXPECT_EQ(ctl.qheartBalanceOf(ctl.state()->getQHeartIssuer()), expectedWalletAfter);
}
