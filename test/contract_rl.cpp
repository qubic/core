// File: test/contract_rl.cpp
#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_BUY_TICKET = 1;
constexpr uint16 PROCEDURE_INDEX_SET_PRICE = 2;
constexpr uint16 PROCEDURE_INDEX_SET_SCHEDULE = 3;
constexpr uint16 FUNCTION_INDEX_GET_FEES = 1;
constexpr uint16 FUNCTION_INDEX_GET_PLAYERS = 2;
constexpr uint16 FUNCTION_INDEX_GET_WINNERS = 3;
constexpr uint16 FUNCTION_INDEX_GET_TICKET_PRICE = 4;
constexpr uint16 FUNCTION_INDEX_GET_MAX_NUM_PLAYERS = 5;
constexpr uint16 FUNCTION_INDEX_GET_STATE = 6;
constexpr uint16 FUNCTION_INDEX_GET_BALANCE = 7;
constexpr uint16 FUNCTION_INDEX_GET_NEXT_EPOCH_DATA = 8;
constexpr uint16 FUNCTION_INDEX_GET_DRAW_HOUR = 9;
constexpr uint16 FUNCTION_INDEX_GET_SCHEDULE = 10;
constexpr uint8 STATE_SELLING = static_cast<uint8>(RL::EState::SELLING);
constexpr uint8 STATE_LOCKED = 0u;

static const id RL_DEV_ADDRESS = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C,
                                    _R, _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

constexpr uint8 RL_ANY_DAY_DRAW_SCHEDULE = 0xFF; // 0xFF sets bits 0..6 (WED..TUE); bit 7 is unused/ignored by logic

static uint32 makeDateStamp(uint16 year, uint8 month, uint8 day)
{
	const uint8 shortYear = static_cast<uint8>(year - 2000);
	return static_cast<uint32>(shortYear << 9 | month << 5 | day);
}

inline bool operator==(uint8 left, RL::EReturnCode right)
{
	return left == RL::toReturnCode(right);
}
inline bool operator==(RL::EReturnCode left, uint8 right)
{
	return right == left;
}
inline bool operator!=(uint8 left, RL::EReturnCode right)
{
	return !(left == right);
}
inline bool operator!=(RL::EReturnCode left, uint8 right)
{
	return !(right == left);
}

// Equality operator for comparing WinnerInfo objects
// Compares all fields (address, revenue, epoch, tick, dayOfWeek)
bool operator==(const RL::WinnerInfo& left, const RL::WinnerInfo& right)
{
	return left.winnerAddress == right.winnerAddress && left.revenue == right.revenue && left.epoch == right.epoch && left.tick == right.tick &&
	       left.dayOfWeek == right.dayOfWeek;
}

// Test helper that exposes internal state assertions and utilities
class RLChecker : public RL
{
public:
	void checkFees(const GetFees_output& fees)
	{
		EXPECT_EQ(fees.returnCode, EReturnCode::SUCCESS);

		EXPECT_EQ(fees.distributionFeePercent, distributionFeePercent);
		EXPECT_EQ(fees.teamFeePercent, teamFeePercent);
		EXPECT_EQ(fees.winnerFeePercent, winnerFeePercent);
		EXPECT_EQ(fees.burnPercent, burnPercent);
	}

	void checkPlayers(const GetPlayers_output& output) const
	{
		EXPECT_EQ(output.returnCode, EReturnCode::SUCCESS);
		EXPECT_EQ(output.players.capacity(), players.capacity());
		EXPECT_EQ(output.playerCounter, playerCounter);

		for (uint64 i = 0; i < playerCounter; ++i)
		{
			EXPECT_EQ(output.players.get(i), players.get(i));
		}
	}

	void checkWinners(const GetWinners_output& output) const
	{
		EXPECT_EQ(output.returnCode, EReturnCode::SUCCESS);
		EXPECT_EQ(output.winners.capacity(), winners.capacity());

		const uint64 expectedCount = mod(winnersCounter, winners.capacity());
		EXPECT_EQ(output.winnersCounter, expectedCount);

		for (uint64 i = 0; i < expectedCount; ++i)
		{
			EXPECT_EQ(output.winners.get(i), winners.get(i));
		}
	}

	void randomlyAddPlayers(uint64 maxNewPlayers)
	{
		playerCounter = mod<uint64>(maxNewPlayers, players.capacity());
		for (uint64 i = 0; i < playerCounter; ++i)
		{
			players.set(i, id::randomValue());
		}
	}

	void randomlyAddWinners(uint64 maxNewWinners)
	{
		const uint64 newWinnerCount = mod<uint64>(maxNewWinners, winners.capacity());

		winnersCounter = 0;
		WinnerInfo wi;

		for (uint64 i = 0; i < newWinnerCount; ++i)
		{
			wi.epoch = 1;
			wi.tick = 1;
			wi.revenue = 1000000;
			wi.winnerAddress = id::randomValue();
			winners.set(winnersCounter++, wi);
		}
	}

	void setScheduleMask(uint8 newMask) { schedule = newMask; }

	uint64 getPlayerCounter() const { return playerCounter; }

	uint64 getTicketPrice() const { return ticketPrice; }

	uint32 getLastDrawDateStamp() const { return lastDrawDateStamp; }
};

class ContractTestingRL : protected ContractTesting
{
public:
	ContractTestingRL()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QX);
		system.epoch = contractDescriptions[QX_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
		INIT_CONTRACT(RL);
		system.epoch = contractDescriptions[RL_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(RL_CONTRACT_INDEX, INITIALIZE);
	}

	// Access internal contract state for assertions
	RLChecker* state() { return reinterpret_cast<RLChecker*>(contractStates[RL_CONTRACT_INDEX]); }

	RL::GetFees_output getFees()
	{
		RL::GetFees_input input;
		RL::GetFees_output output;

		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_FEES, input, output);
		return output;
	}

	RL::GetPlayers_output getPlayers()
	{
		RL::GetPlayers_input input;
		RL::GetPlayers_output output;

		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_PLAYERS, input, output);
		return output;
	}

	RL::GetWinners_output getWinners()
	{
		RL::GetWinners_input input;
		RL::GetWinners_output output;

		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_WINNERS, input, output);
		return output;
	}

	// Wrapper for public function RL::GetTicketPrice
	RL::GetTicketPrice_output getTicketPrice()
	{
		RL::GetTicketPrice_input input;
		RL::GetTicketPrice_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_TICKET_PRICE, input, output);
		return output;
	}

	// Wrapper for public function RL::GetMaxNumberOfPlayers
	RL::GetMaxNumberOfPlayers_output getMaxNumberOfPlayers()
	{
		RL::GetMaxNumberOfPlayers_input input;
		RL::GetMaxNumberOfPlayers_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_MAX_NUM_PLAYERS, input, output);
		return output;
	}

	// Wrapper for public function RL::GetState
	RL::GetState_output getStateInfo()
	{
		RL::GetState_input input;
		RL::GetState_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_STATE, input, output);
		return output;
	}

	// Wrapper for public function RL::GetBalance
	// Returns current contract on-chain balance (incoming - outgoing)
	RL::GetBalance_output getBalanceInfo()
	{
		RL::GetBalance_input input;
		RL::GetBalance_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_BALANCE, input, output);
		return output;
	}

	// Wrapper for public function RL::GetNextEpochData
	RL::GetNextEpochData_output getNextEpochData()
	{
		RL::GetNextEpochData_input input;
		RL::GetNextEpochData_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_NEXT_EPOCH_DATA, input, output);
		return output;
	}

	// Wrapper for public function RL::GetDrawHour
	RL::GetDrawHour_output getDrawHour()
	{
		RL::GetDrawHour_input input;
		RL::GetDrawHour_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_DRAW_HOUR, input, output);
		return output;
	}

	// Wrapper for public function RL::GetSchedule
	RL::GetSchedule_output getSchedule()
	{
		RL::GetSchedule_input input;
		RL::GetSchedule_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_SCHEDULE, input, output);
		return output;
	}

	RL::BuyTicket_output buyTicket(const id& user, sint64 reward)
	{
		RL::BuyTicket_input input;
		RL::BuyTicket_output output;
		if (!invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_BUY_TICKET, input, output, user, reward))
		{
			output.returnCode = RL::toReturnCode(RL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	// Added: wrapper for SetPrice procedure
	RL::SetPrice_output setPrice(const id& invocator, uint64 newPrice)
	{
		RL::SetPrice_input input;
		input.newPrice = newPrice;
		RL::SetPrice_output output;
		if (!invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_SET_PRICE, input, output, invocator, 0))
		{
			output.returnCode = RL::toReturnCode(RL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	// Added: wrapper for SetSchedule procedure
	RL::SetSchedule_output setSchedule(const id& invocator, uint8 newSchedule)
	{
		RL::SetSchedule_input input;
		input.newSchedule = newSchedule;
		RL::SetSchedule_output output;
		if (!invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_SET_SCHEDULE, input, output, invocator, 0))
		{
			output.returnCode = RL::toReturnCode(RL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	void BeginEpoch() { callSystemProcedure(RL_CONTRACT_INDEX, BEGIN_EPOCH); }

	void EndEpoch() { callSystemProcedure(RL_CONTRACT_INDEX, END_EPOCH); }

	void BeginTick() { callSystemProcedure(RL_CONTRACT_INDEX, BEGIN_TICK); }

	// Returns the SELF contract account address
	id rlSelf() const { return id(RL_CONTRACT_INDEX, 0, 0, 0); }

	// Computes remaining contract balance after winner/team/distribution/burn payouts
	// Distribution is floored to a multiple of NUMBER_OF_COMPUTORS
	uint64 expectedRemainingAfterPayout(uint64 before, const RL::GetFees_output& fees)
	{
		const uint64 burn = (before * fees.burnPercent) / 100;
		const uint64 distribPer = ((before * fees.distributionFeePercent) / 100) / NUMBER_OF_COMPUTORS;
		const uint64 distrib = distribPer * NUMBER_OF_COMPUTORS; // floor to a multiple
		const uint64 team = (before * fees.teamFeePercent) / 100;
		const uint64 winner = (before * fees.winnerFeePercent) / 100;
		return before - burn - distrib - team - winner;
	}

	// Fund user and buy a ticket, asserting success
	void increaseAndBuy(ContractTestingRL& ctl, const id& user, uint64 ticketPrice)
	{
		increaseEnergy(user, ticketPrice * 2);
		const RL::BuyTicket_output out = ctl.buyTicket(user, ticketPrice);
		EXPECT_EQ(out.returnCode, RL::EReturnCode::SUCCESS);
	}

	// Assert contract account balance equals the value returned by RL::GetBalance
	void expectContractBalanceEqualsGetBalance(ContractTestingRL& ctl, const id& contractAddress)
	{
		const RL::GetBalance_output out = ctl.getBalanceInfo();
		EXPECT_EQ(out.balance, getBalance(contractAddress));
	}

	// New: set full date and hour (UTC), then sync QPI time
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

	// New: advance to the next tick boundary where tick % RL_TICK_UPDATE_PERIOD == 0 and run BEGIN_TICK once
	void forceBeginTick()
	{
		system.tick = system.tick + (RL_TICK_UPDATE_PERIOD - mod(system.tick, static_cast<uint32>(RL_TICK_UPDATE_PERIOD)));

		BeginTick();
	}

	// New: helper to advance one calendar day and perform a scheduled draw at 12:00 UTC
	void advanceOneDayAndDraw()
	{
		// Use a safe base month to avoid invalid dates: January 2025
		static uint16 y = 2025;
		static uint8 m = 1;
		static uint8 d = 10; // start from 10th
		// advance one day within January bounds
		d = static_cast<uint8>(d + 1);
		if (d > 31)
		{
			d = 1; // wrap within month for simplicity in tests
		}
		setDateTime(y, m, d, 12);
		forceBeginTick();
	}

	// Force schedule mask directly in state (bypasses external call, suitable for tests)
	void forceSchedule(uint8 scheduleMask)
	{
		state()->setScheduleMask(scheduleMask);
		// NOTE: we do not call SetSchedule here to avoid epoch transitions in tests.
	}

	void beginEpochWithDate(uint16 year, uint8 month, uint8 day, uint8 hour = static_cast<uint8>(RL_DEFAULT_DRAW_HOUR + 1))
	{
		setDateTime(year, month, day, hour);
		BeginEpoch();
	}

	void beginEpochWithValidTime() { beginEpochWithDate(2025, 1, 20); }
};

TEST(ContractRandomLottery, SetPriceAndScheduleApplyNextEpoch)
{
	ContractTestingRL ctl;
	ctl.beginEpochWithValidTime();

	// Default epoch configuration: draws 3 times per week at the default price
	const uint64 oldPrice = ctl.state()->getTicketPrice();
	EXPECT_EQ(ctl.getSchedule().schedule, RL_DEFAULT_SCHEDULE);

	// Queue a new price (5,000,000) and limit draws to only Wednesday
	constexpr uint64 newPrice = 5000000;
	constexpr uint8 wednesdayOnly = static_cast<uint8>(1 << WEDNESDAY);
	increaseEnergy(RL_DEV_ADDRESS, 3);
	EXPECT_EQ(ctl.setPrice(RL_DEV_ADDRESS, newPrice).returnCode, RL::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setSchedule(RL_DEV_ADDRESS, wednesdayOnly).returnCode, RL::EReturnCode::SUCCESS);

	const RL::NextEpochData nextDataBefore = ctl.getNextEpochData().nextEpochData;
	EXPECT_EQ(nextDataBefore.newPrice, newPrice);
	EXPECT_EQ(nextDataBefore.schedule, wednesdayOnly);

	// Until END_EPOCH the old settings remain active
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
	EXPECT_EQ(ctl.getSchedule().schedule, RL_DEFAULT_SCHEDULE);

	// Transition closes the epoch and applies both pending changes
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, newPrice);
	EXPECT_EQ(ctl.getSchedule().schedule, wednesdayOnly);

	const RL::NextEpochData nextDataAfter = ctl.getNextEpochData().nextEpochData;
	EXPECT_EQ(nextDataAfter.newPrice, 0u);
	EXPECT_EQ(nextDataAfter.schedule, 0u);

	// In the next epoch tickets must sell at the updated price
	ctl.beginEpochWithDate(2025, 1, 15); // Wednesday
	const id buyer = id::randomValue();
	increaseEnergy(buyer, newPrice * 2);
	const uint64 balBefore = getBalance(buyer);
	const uint64 playersBefore = ctl.state()->getPlayerCounter();
	const RL::BuyTicket_output buyOut = ctl.buyTicket(buyer, newPrice);
	EXPECT_EQ(buyOut.returnCode, RL::EReturnCode::SUCCESS);
	const uint64 playersAfterFirstBuy = playersBefore + 1;
	EXPECT_EQ(ctl.state()->getPlayerCounter(), playersAfterFirstBuy);
	EXPECT_EQ(getBalance(buyer), balBefore - newPrice);

	// Second user also buys a ticket at the new price
	const id secondBuyer = id::randomValue();
	increaseEnergy(secondBuyer, newPrice * 2);
	const uint64 secondBalBefore = getBalance(secondBuyer);
	const RL::BuyTicket_output secondBuyOut = ctl.buyTicket(secondBuyer, newPrice);
	EXPECT_EQ(secondBuyOut.returnCode, RL::EReturnCode::SUCCESS);
	const uint64 playersAfterBuy = playersAfterFirstBuy + 1;
	EXPECT_EQ(ctl.state()->getPlayerCounter(), playersAfterBuy);
	EXPECT_EQ(getBalance(secondBuyer), secondBalBefore - newPrice);

	// Draws should only trigger on Wednesdays now: starting on Wednesday means the draw
	// is deferred until the next Wednesday in the schedule.
	const uint64 winnersBefore = ctl.getWinners().winnersCounter;
	ctl.setDateTime(2025, 1, 15, RL_DEFAULT_DRAW_HOUR + 1); // current Wednesday
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getPlayerCounter(), playersAfterBuy);
	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_SELLING);
	EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBefore);

	// No draw on non-scheduled days between Wednesdays
	ctl.setDateTime(2025, 1, 21, RL_DEFAULT_DRAW_HOUR + 1); // Tuesday next week
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getPlayerCounter(), playersAfterBuy);
	EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBefore);

	// Next Wednesday processes the draw
	ctl.setDateTime(2025, 1, 22, RL_DEFAULT_DRAW_HOUR + 1); // next Wednesday
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);
	EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBefore + 1);
	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_LOCKED);

	// After the draw and before the next epoch begins, ticket purchases are blocked
	const id lockedBuyer = id::randomValue();
	increaseEnergy(lockedBuyer, newPrice);
	const RL::BuyTicket_output lockedOut = ctl.buyTicket(lockedBuyer, newPrice);
	EXPECT_EQ(lockedOut.returnCode, RL::EReturnCode::TICKET_SELLING_CLOSED);
}

TEST(ContractRandomLottery, DefaultInitTimeGuardSkipsPlaceholderDate)
{
	ContractTestingRL ctl;

	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	// Allow draws every day so weekday logic does not block BEGIN_TICK
	ctl.forceSchedule(RL_ANY_DAY_DRAW_SCHEDULE);

	// Simulate the placeholder 2022-04-13 QPI date during initialization
	ctl.setDateTime(2022, 4, 13, RL_DEFAULT_DRAW_HOUR + 1);
	ctl.BeginEpoch();
	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_LOCKED);

	// Selling is blocked until a valid date arrives
	const id blockedBuyer = id::randomValue();
	increaseEnergy(blockedBuyer, ticketPrice);
	const RL::BuyTicket_output denied = ctl.buyTicket(blockedBuyer, ticketPrice);
	EXPECT_EQ(denied.returnCode, RL::EReturnCode::TICKET_SELLING_CLOSED);
	EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

	const uint64 winnersBefore = ctl.getWinners().winnersCounter;

	// BEGIN_TICK should detect the placeholder date and skip processing, but remember the sentinel day
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getLastDrawDateStamp(), RL_DEFAULT_INIT_TIME);
	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_LOCKED);
	EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);
	EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBefore);

	// First valid day re-opens selling but still skips the draw
	ctl.setDateTime(2025, 1, 10, RL_DEFAULT_DRAW_HOUR + 1);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_SELLING);
	EXPECT_NE(ctl.state()->getLastDrawDateStamp(), RL_DEFAULT_INIT_TIME);

	const id playerA = id::randomValue();
	const id playerB = id::randomValue();
	ctl.increaseAndBuy(ctl, playerA, ticketPrice);
	ctl.increaseAndBuy(ctl, playerB, ticketPrice);
	EXPECT_EQ(ctl.state()->getPlayerCounter(), 2u);

	// The immediate next valid day should run the actual draw
	ctl.setDateTime(2025, 1, 11, RL_DEFAULT_DRAW_HOUR + 1);
	ctl.forceBeginTick();
	EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);
	EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBefore + 1);
	EXPECT_NE(ctl.state()->getLastDrawDateStamp(), RL_DEFAULT_INIT_TIME);
}

TEST(ContractRandomLottery, SellingUnlocksWhenTimeSetBeforeScheduledDay)
{
	ContractTestingRL ctl;

	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	ctl.setDateTime(2022, 4, 13, RL_DEFAULT_DRAW_HOUR + 1);
	ctl.BeginEpoch();

	const id deniedBuyer = id::randomValue();
	increaseEnergy(deniedBuyer, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(deniedBuyer, ticketPrice).returnCode, RL::EReturnCode::TICKET_SELLING_CLOSED);

	ctl.setDateTime(2025, 1, 14, RL_DEFAULT_DRAW_HOUR + 2); // Tuesday, not scheduled by default
	ctl.forceBeginTick();

	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_SELLING);
	EXPECT_EQ(ctl.state()->getLastDrawDateStamp(), 0u);

	const id allowedBuyer = id::randomValue();
	increaseEnergy(allowedBuyer, ticketPrice);
	const RL::BuyTicket_output allowed = ctl.buyTicket(allowedBuyer, ticketPrice);
	EXPECT_EQ(allowed.returnCode, RL::EReturnCode::SUCCESS);
}

TEST(ContractRandomLottery, SellingUnlocksWhenTimeSetOnDrawDay)
{
	ContractTestingRL ctl;

	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	ctl.setDateTime(2022, 4, 13, RL_DEFAULT_DRAW_HOUR + 1);
	ctl.BeginEpoch();

	const id deniedBuyer = id::randomValue();
	increaseEnergy(deniedBuyer, ticketPrice);
	EXPECT_EQ(ctl.buyTicket(deniedBuyer, ticketPrice).returnCode, RL::EReturnCode::TICKET_SELLING_CLOSED);

	ctl.setDateTime(2025, 1, 15, RL_DEFAULT_DRAW_HOUR + 2); // Wednesday draw day
	ctl.forceBeginTick();

	const uint32 expectedStamp = makeDateStamp(2025, 1, 15);
	EXPECT_EQ(ctl.state()->getLastDrawDateStamp(), expectedStamp);
	EXPECT_EQ(ctl.getStateInfo().currentState, STATE_SELLING);

	const id allowedBuyer = id::randomValue();
	increaseEnergy(allowedBuyer, ticketPrice);
	const RL::BuyTicket_output allowed = ctl.buyTicket(allowedBuyer, ticketPrice);
	EXPECT_EQ(allowed.returnCode, RL::EReturnCode::SUCCESS);
}

TEST(ContractRandomLottery, PostIncomingTransfer)
{
	ContractTestingRL ctl;
	static constexpr uint64 transferAmount = 123456789;

	const id sender = id::randomValue();
	increaseEnergy(sender, transferAmount);
	EXPECT_EQ(getBalance(sender), transferAmount);

	const id contractAddress = ctl.rlSelf();
	EXPECT_EQ(getBalance(contractAddress), 0);

	notifyContractOfIncomingTransfer(sender, contractAddress, transferAmount, QPI::TransferType::standardTransaction);

	EXPECT_EQ(getBalance(sender), transferAmount);
	EXPECT_EQ(getBalance(contractAddress), 0);
}

TEST(ContractRandomLottery, GetFees)
{
	ContractTestingRL ctl;
	RL::GetFees_output output = ctl.getFees();
	ctl.state()->checkFees(output);
}

TEST(ContractRandomLottery, GetPlayers)
{
	ContractTestingRL ctl;

	// Initially empty
	RL::GetPlayers_output output = ctl.getPlayers();
	ctl.state()->checkPlayers(output);

	// Add random players directly to state (test helper)
	constexpr uint64 maxPlayersToAdd = 10;
	ctl.state()->randomlyAddPlayers(maxPlayersToAdd);
	output = ctl.getPlayers();
	ctl.state()->checkPlayers(output);
}

TEST(ContractRandomLottery, GetWinners)
{
	ContractTestingRL ctl;

	// Populate winners history artificially
	constexpr uint64 maxNewWinners = 10;
	ctl.state()->randomlyAddWinners(maxNewWinners);
	RL::GetWinners_output winnersOutput = ctl.getWinners();
	ctl.state()->checkWinners(winnersOutput);
}

TEST(ContractRandomLottery, BuyTicket)
{
	ContractTestingRL ctl;

	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	// 1. Attempt when state is LOCKED (should fail and refund invocation reward)
	{
		const id userLocked = id::randomValue();
		increaseEnergy(userLocked, ticketPrice * 2);
		RL::BuyTicket_output out = ctl.buyTicket(userLocked, ticketPrice);
		EXPECT_EQ(out.returnCode, RL::EReturnCode::TICKET_SELLING_CLOSED);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0);
	}

	// Switch to SELLING to allow purchases
	ctl.beginEpochWithValidTime();

	// 2. Loop over several users and test invalid price, success, duplicate
	constexpr uint64 userCount = 5;
	uint64 expectedPlayers = 0;

	for (uint64 i = 0; i < userCount; ++i)
	{
		const id user = id::randomValue();
		increaseEnergy(user, ticketPrice * 5);

		// (a) Invalid price (wrong reward sent) — player not added
		{
			// < ticketPrice
			RL::BuyTicket_output outInvalid = ctl.buyTicket(user, ticketPrice - 1);
			EXPECT_EQ(outInvalid.returnCode, RL::EReturnCode::TICKET_INVALID_PRICE);
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);

			// == 0
			outInvalid = ctl.buyTicket(user, 0);
			EXPECT_EQ(outInvalid.returnCode, RL::EReturnCode::TICKET_INVALID_PRICE);
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);

			// < 0
			outInvalid = ctl.buyTicket(user, -1LL * ticketPrice);
			EXPECT_NE(outInvalid.returnCode, RL::EReturnCode::SUCCESS);
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);
		}

		// (b) Valid purchase — player added
		{
			const RL::BuyTicket_output outOk = ctl.buyTicket(user, ticketPrice);
			EXPECT_EQ(outOk.returnCode, RL::EReturnCode::SUCCESS);
			++expectedPlayers;
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);
		}

		// (c) Duplicate purchase — allowed, increases count
		{
			const RL::BuyTicket_output outDup = ctl.buyTicket(user, ticketPrice);
			EXPECT_EQ(outDup.returnCode, RL::EReturnCode::SUCCESS);
			++expectedPlayers;
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);
		}
	}

	// 3. Sanity check: number of tickets equals twice the number of users (due to duplicate buys)
	EXPECT_EQ(ctl.state()->getPlayerCounter(), userCount * 2);
}

// Updated: payout is triggered by BEGIN_TICK with schedule/time gating, not by END_EPOCH
TEST(ContractRandomLottery, DrawAndPayout_BeginTick)
{
	ContractTestingRL ctl;

	const id contractAddress = ctl.rlSelf();
	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	// Current fee configuration (set in INITIALIZE)
	const RL::GetFees_output fees = ctl.getFees();
	const uint8 teamPercent = fees.teamFeePercent;                 // Team commission percent
	const uint8 winnerPercent = fees.winnerFeePercent;             // Winner payout percent

	// Ensure schedule allows draw any day
	ctl.forceSchedule(RL_ANY_DAY_DRAW_SCHEDULE);

	// --- Scenario 1: No players (nothing to payout, no winner recorded) ---
	{
		ctl.beginEpochWithValidTime();
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

		RL::GetWinners_output before = ctl.getWinners();
		const uint64 winnersBefore = before.winnersCounter;

		// Need to move to a new day and call BEGIN_TICK to allow draw
		ctl.advanceOneDayAndDraw();

		RL::GetWinners_output after = ctl.getWinners();
		EXPECT_EQ(after.winnersCounter, winnersBefore);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);
	}

	// --- Scenario 2: Exactly one player (ticket refunded, no winner recorded) ---
	{
		ctl.beginEpochWithValidTime();

		const id solo = id::randomValue();
		increaseEnergy(solo, ticketPrice);
		const uint64 balanceBefore = getBalance(solo);

		const RL::BuyTicket_output out = ctl.buyTicket(solo, ticketPrice);
		EXPECT_EQ(out.returnCode, RL::EReturnCode::SUCCESS);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 1u);
		EXPECT_EQ(getBalance(solo), balanceBefore - ticketPrice);

		const uint64 winnersBeforeCount = ctl.getWinners().winnersCounter;

		ctl.advanceOneDayAndDraw();

		// Refund happened
		EXPECT_EQ(getBalance(solo), balanceBefore);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

		const RL::GetWinners_output winners = ctl.getWinners();
		// No new winners appended
		EXPECT_EQ(winners.winnersCounter, winnersBeforeCount);
	}

	// --- Scenario 2b: Multiple tickets from the same player are treated as single participant ---
	{
		ctl.beginEpochWithValidTime();

		const id solo = id::randomValue();
		increaseEnergy(solo, ticketPrice * 10);
		const uint64 balanceBefore = getBalance(solo);

		for (int i = 0; i < 5; ++i)
		{
			EXPECT_EQ(ctl.buyTicket(solo, ticketPrice).returnCode, RL::EReturnCode::SUCCESS);
		}
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 5u);

		const uint64 winnersBeforeCount = ctl.getWinners().winnersCounter;

		ctl.advanceOneDayAndDraw();

		// All tickets refunded, no winner recorded
		EXPECT_EQ(getBalance(solo), balanceBefore);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);
		EXPECT_EQ(ctl.getWinners().winnersCounter, winnersBeforeCount);
		EXPECT_EQ(getBalance(contractAddress), 0u);
	}

	// --- Scenario 3: Multiple players (winner chosen, fees processed, correct remaining on contract) ---
	{
		ctl.beginEpochWithValidTime();

		constexpr uint32 N = 5 * 2;
		struct PlayerInfo
		{
			id addr;
			uint64 balanceBefore;
			uint64 balanceAfterBuy;
		};
		std::vector<PlayerInfo> infos;
		infos.reserve(N);

		// Add N/2 distinct players, each making two valid purchases
		for (uint32 i = 0; i < N; i += 2)
		{
			const id randomUser = id::randomValue();
			ctl.increaseAndBuy(ctl, randomUser, ticketPrice);
			ctl.increaseAndBuy(ctl, randomUser, ticketPrice);
			const uint64 bBefore = getBalance(randomUser);
			infos.push_back({randomUser, bBefore + (ticketPrice * 2), bBefore}); // account for ticket deduction
		}

		EXPECT_EQ(ctl.state()->getPlayerCounter(), N);

		const uint64 contractBalanceBefore = getBalance(contractAddress);
		EXPECT_EQ(contractBalanceBefore, ticketPrice * N);

		const uint64 teamBalanceBefore = getBalance(RL_DEV_ADDRESS);

		const RL::GetWinners_output winnersBefore = ctl.getWinners();
		const uint64 winnersCountBefore = winnersBefore.winnersCounter;

		ctl.advanceOneDayAndDraw();

		// Players reset after draw
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

		const RL::GetWinners_output winnersAfter = ctl.getWinners();
		EXPECT_EQ(winnersAfter.winnersCounter, winnersCountBefore + 1);

		// Newly appended winner info
		const RL::WinnerInfo wi = winnersAfter.winners.get(mod(winnersCountBefore, winnersAfter.winners.capacity()));
		EXPECT_NE(wi.winnerAddress, id::zero());
		EXPECT_EQ(wi.revenue, (ticketPrice * N * winnerPercent) / 100);

		// Winner address must be one of the players
		bool found = false;
		for (const PlayerInfo& inf : infos)
		{
			if (inf.addr == wi.winnerAddress)
			{
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found);

		// Check winner balance increment and others unchanged
		for (const PlayerInfo& inf : infos)
		{
			const uint64 bal = getBalance(inf.addr);
			const uint64 balanceAfterBuy = inf.addr == wi.winnerAddress ? inf.balanceAfterBuy + wi.revenue : inf.balanceAfterBuy;
			EXPECT_EQ(bal, balanceAfterBuy);
		}

		// Team fee transferred
		const uint64 teamFeeExpected = (ticketPrice * N * teamPercent) / 100;
		EXPECT_EQ(getBalance(RL_DEV_ADDRESS), teamBalanceBefore + teamFeeExpected);

		// Burn (remaining on contract)
		const uint64 burnExpected = ctl.expectedRemainingAfterPayout(contractBalanceBefore, fees);
		EXPECT_EQ(getBalance(contractAddress), burnExpected);
	}

	// --- Scenario 4: Several consecutive draws (winners accumulate, balances consistent) ---
	{
		const uint32 rounds = 3;
		const uint32 playersPerRound = 6 * 2; // even number to mimic duplicates if desired

		// Remember starting winners count and team balance
		const uint64 winnersStart = ctl.getWinners().winnersCounter;
		const uint64 teamStartBal = getBalance(RL_DEV_ADDRESS);

		uint64 teamAccrued = 0;

		for (uint32 r = 0; r < rounds; ++r)
		{
			ctl.beginEpochWithValidTime();

			struct P
			{
				id addr;
				uint64 balAfterBuy;
			};
			std::vector<P> roundPlayers;
			roundPlayers.reserve(playersPerRound);

			// Each player buys two tickets in this round
			for (uint32 i = 0; i < playersPerRound; i += 2)
			{
				const id u = id::randomValue();
				ctl.increaseAndBuy(ctl, u, ticketPrice);
				ctl.increaseAndBuy(ctl, u, ticketPrice);
				const uint64 balAfter = getBalance(u);
				roundPlayers.push_back({u, balAfter});
			}

			EXPECT_EQ(ctl.state()->getPlayerCounter(), playersPerRound);

			const uint64 winnersBefore = ctl.getWinners().winnersCounter;
			const uint64 contractBefore = getBalance(contractAddress);
			const uint64 teamBalBeforeRound = getBalance(RL_DEV_ADDRESS);

			ctl.advanceOneDayAndDraw();

			// Winners should increase by exactly one
			const RL::GetWinners_output wOut = ctl.getWinners();
			EXPECT_EQ(wOut.winnersCounter, winnersBefore + 1);

			// Validate winner entry
			const RL::WinnerInfo newWi = wOut.winners.get(mod(winnersBefore, wOut.winners.capacity()));
			EXPECT_NE(newWi.winnerAddress, id::zero());
			EXPECT_EQ(newWi.revenue, (contractBefore * winnerPercent) / 100);

			// Winner must be one of the current round players
			bool inRound = false;
			for (const auto& p : roundPlayers)
			{
				if (p.addr == newWi.winnerAddress)
				{
					inRound = true;
					break;
				}
			}
			EXPECT_TRUE(inRound);

			// Check players' balances after payout
			for (const auto& p : roundPlayers)
			{
				const uint64 b = getBalance(p.addr);
				const uint64 expected = (p.addr == newWi.winnerAddress) ? (p.balAfterBuy + newWi.revenue) : p.balAfterBuy;
				EXPECT_EQ(b, expected);
			}

			// Team fee for the whole round's contract balance
			const uint64 teamFee = (contractBefore * teamPercent) / 100;
			teamAccrued += teamFee;
			EXPECT_EQ(getBalance(RL_DEV_ADDRESS), teamBalBeforeRound + teamFee);

			// Contract remaining should match expected
			const uint64 expectedRemaining = ctl.expectedRemainingAfterPayout(contractBefore, fees);
			EXPECT_EQ(getBalance(contractAddress), expectedRemaining);
		}

		// After all rounds winners increased by rounds and team received cumulative fees
		EXPECT_EQ(ctl.getWinners().winnersCounter, winnersStart + rounds);
		EXPECT_EQ(getBalance(RL_DEV_ADDRESS), teamStartBal + teamAccrued);
	}
}
TEST(ContractRandomLottery, GetBalance)
{
	ContractTestingRL ctl;

	const id contractAddress = ctl.rlSelf();
	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	// Initially, contract balance is 0
	{
		const RL::GetBalance_output out0 = ctl.getBalanceInfo();
		EXPECT_EQ(out0.balance, 0u);
		EXPECT_EQ(out0.balance, getBalance(contractAddress));
	}

	// Open selling and perform several purchases
	ctl.beginEpochWithValidTime();

	constexpr uint32 K = 3;
	for (uint32 i = 0; i < K; ++i)
	{
		const id user = id::randomValue();
		ctl.increaseAndBuy(ctl, user, ticketPrice);
		ctl.expectContractBalanceEqualsGetBalance(ctl, contractAddress);
	}

	// Before draw, balance equals the total cost of tickets
	{
		const RL::GetBalance_output outBefore = ctl.getBalanceInfo();
		EXPECT_EQ(outBefore.balance, ticketPrice * K);
	}

	// Trigger draw and verify expected remaining amount against contract balance and function output
	const uint64 contractBalanceBefore = getBalance(contractAddress);
	const RL::GetFees_output fees = ctl.getFees();

	// Ensure schedule allows draw and perform it
	ctl.forceSchedule(RL_ANY_DAY_DRAW_SCHEDULE);
	ctl.advanceOneDayAndDraw();

	const RL::GetBalance_output outAfter = ctl.getBalanceInfo();
	const uint64 envAfter = getBalance(contractAddress);
	EXPECT_EQ(outAfter.balance, envAfter);

	const uint64 expectedRemaining = ctl.expectedRemainingAfterPayout(contractBalanceBefore, fees);
	EXPECT_EQ(outAfter.balance, expectedRemaining);
}

TEST(ContractRandomLottery, GetTicketPrice)
{
	ContractTestingRL ctl;

	const RL::GetTicketPrice_output out = ctl.getTicketPrice();
	EXPECT_EQ(out.ticketPrice, ctl.state()->getTicketPrice());
}

TEST(ContractRandomLottery, GetMaxNumberOfPlayers)
{
	ContractTestingRL ctl;

	const RL::GetMaxNumberOfPlayers_output out = ctl.getMaxNumberOfPlayers();
	// Compare against the known constant via GetPlayers capacity
	const RL::GetPlayers_output playersOut = ctl.getPlayers();
	EXPECT_EQ(static_cast<unsigned>(out.numberOfPlayers), static_cast<unsigned>(playersOut.players.capacity()));
}

TEST(ContractRandomLottery, GetState)
{
	ContractTestingRL ctl;

	// Initially LOCKED
	{
		const RL::GetState_output out0 = ctl.getStateInfo();
		EXPECT_EQ(out0.currentState, STATE_LOCKED);
	}

	// After BeginEpoch — SELLING
	ctl.beginEpochWithValidTime();
	{
		const RL::GetState_output out1 = ctl.getStateInfo();
		EXPECT_EQ(out1.currentState, STATE_SELLING);
	}

	// After END_EPOCH — back to LOCKED (selling disabled until next epoch)
	ctl.EndEpoch();
	{
		const RL::GetState_output out2 = ctl.getStateInfo();
		EXPECT_EQ(out2.currentState, STATE_LOCKED);
	}
}

// --- New tests for SetPrice and NextEpochData ---

TEST(ContractRandomLottery, SetPrice_AccessControl)
{
	ContractTestingRL ctl;

	const uint64 oldPrice = ctl.state()->getTicketPrice();
	const uint64 newPrice = oldPrice * 2;

	// Random user must not have permission
	const id randomUser = id::randomValue();
	increaseEnergy(randomUser, 1);

	const RL::SetPrice_output outDenied = ctl.setPrice(randomUser, newPrice);
	EXPECT_EQ(outDenied.returnCode, RL::EReturnCode::ACCESS_DENIED);

	// Price doesn't change immediately nor after END_EPOCH implicitly
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
}

TEST(ContractRandomLottery, SetPrice_ZeroNotAllowed)
{
	ContractTestingRL ctl;

	increaseEnergy(RL_DEV_ADDRESS, 1);

	const uint64 oldPrice = ctl.state()->getTicketPrice();

	const RL::SetPrice_output outInvalid = ctl.setPrice(RL_DEV_ADDRESS, 0);
	EXPECT_EQ(outInvalid.returnCode, RL::EReturnCode::TICKET_INVALID_PRICE);

	// Price remains unchanged even after END_EPOCH
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);
}

TEST(ContractRandomLottery, SetPrice_AppliesAfterEndEpoch)
{
	ContractTestingRL ctl;

	increaseEnergy(RL_DEV_ADDRESS, 1);

	const uint64 oldPrice = ctl.state()->getTicketPrice();
	const uint64 newPrice = oldPrice * 2;

	const RL::SetPrice_output outOk = ctl.setPrice(RL_DEV_ADDRESS, newPrice);
	EXPECT_EQ(outOk.returnCode, RL::EReturnCode::SUCCESS);

	// Check NextEpochData reflects pending change
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newPrice, newPrice);

	// Until END_EPOCH the price remains unchanged
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);

	// Applied after END_EPOCH
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, newPrice);

	// NextEpochData cleared
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newPrice, 0u);

	// Another END_EPOCH without a new SetPrice doesn't change the price
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, newPrice);
}

TEST(ContractRandomLottery, SetPrice_OverrideBeforeEndEpoch)
{
	ContractTestingRL ctl;

	increaseEnergy(RL_DEV_ADDRESS, 1);

	const uint64 oldPrice = ctl.state()->getTicketPrice();
	const uint64 firstPrice = oldPrice + 1000;
	const uint64 secondPrice = oldPrice + 7777;

	// Two SetPrice calls before END_EPOCH — the last one should apply
	EXPECT_EQ(ctl.setPrice(RL_DEV_ADDRESS, firstPrice).returnCode, RL::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.setPrice(RL_DEV_ADDRESS, secondPrice).returnCode, RL::EReturnCode::SUCCESS);

	// NextEpochData shows the last queued value
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newPrice, secondPrice);

	// Until END_EPOCH the old price remains
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, oldPrice);

	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, secondPrice);
}

TEST(ContractRandomLottery, SetPrice_AffectsNextEpochBuys)
{
	ContractTestingRL ctl;

	increaseEnergy(RL_DEV_ADDRESS, 1);

	const uint64 oldPrice = ctl.state()->getTicketPrice();
	const uint64 newPrice = oldPrice * 3;

	// Open selling and buy at the old price
	ctl.beginEpochWithValidTime();
	const id u1 = id::randomValue();
	increaseEnergy(u1, oldPrice * 2);
	{
		const RL::BuyTicket_output out1 = ctl.buyTicket(u1, oldPrice);
		EXPECT_EQ(out1.returnCode, RL::EReturnCode::SUCCESS);
	}

	// Set a new price, but before END_EPOCH purchases should use the old price logic (split by old price)
	{
		const RL::SetPrice_output setOut = ctl.setPrice(RL_DEV_ADDRESS, newPrice);
		EXPECT_EQ(setOut.returnCode, RL::EReturnCode::SUCCESS);
		EXPECT_EQ(ctl.getNextEpochData().nextEpochData.newPrice, newPrice);
	}

	const id u2 = id::randomValue();
	increaseEnergy(u2, newPrice * 2);
	{
		const uint64 balBefore = getBalance(u2);
		const uint64 playersBefore = ctl.state()->getPlayerCounter();
		const RL::BuyTicket_output outNow = ctl.buyTicket(u2, newPrice);
		EXPECT_EQ(outNow.returnCode, RL::EReturnCode::SUCCESS);
		// floor(newPrice/oldPrice) tickets were bought, the remainder was refunded
		const uint64 bought = newPrice / oldPrice;
		EXPECT_EQ(ctl.state()->getPlayerCounter(), playersBefore + bought);
		EXPECT_EQ(getBalance(u2), balBefore - bought * oldPrice);
	}

	// END_EPOCH: new price will apply
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getTicketPrice().ticketPrice, newPrice);

	// In the next epoch, a purchase at the new price should succeed exactly once per price
	ctl.beginEpochWithValidTime();
	{
		const uint64 balBefore = getBalance(u2);
		const uint64 playersBefore = ctl.state()->getPlayerCounter();
		const RL::BuyTicket_output outOk = ctl.buyTicket(u2, newPrice);
		EXPECT_EQ(outOk.returnCode, RL::EReturnCode::SUCCESS);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), playersBefore + 1);
		EXPECT_EQ(getBalance(u2), balBefore - newPrice);
	}
}

TEST(ContractRandomLottery, BuyMultipleTickets_ExactMultiple_NoRemainder)
{
	ContractTestingRL ctl;
	ctl.beginEpochWithValidTime();
	const uint64 price = ctl.state()->getTicketPrice();
	const id user = id::randomValue();
	constexpr uint64 k = 7;
	increaseEnergy(user, price * k);
	const uint64 playersBefore = ctl.state()->getPlayerCounter();
	const RL::BuyTicket_output out = ctl.buyTicket(user, price * k);
	EXPECT_EQ(out.returnCode, RL::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getPlayerCounter(), playersBefore + k);
}

TEST(ContractRandomLottery, BuyMultipleTickets_WithRemainder_Refunded)
{
	ContractTestingRL ctl;
	ctl.beginEpochWithValidTime();
	const uint64 price = ctl.state()->getTicketPrice();
	const id user = id::randomValue();
	constexpr uint64 k = 5;
	const uint64 r = price / 3; // partial remainder
	increaseEnergy(user, price * k + r);
	const uint64 balBefore = getBalance(user);
	const uint64 playersBefore = ctl.state()->getPlayerCounter();
	const RL::BuyTicket_output out = ctl.buyTicket(user, price * k + r);
	EXPECT_EQ(out.returnCode, RL::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getPlayerCounter(), playersBefore + k);
	// Remainder refunded, only k * price spent
	EXPECT_EQ(getBalance(user), balBefore - k * price);
}

TEST(ContractRandomLottery, BuyMultipleTickets_CapacityPartialRefund)
{
	ContractTestingRL ctl;
	ctl.beginEpochWithValidTime();
	const uint64 price = ctl.state()->getTicketPrice();
	const uint64 capacity = ctl.getPlayers().players.capacity();

	// Fill almost up to capacity
	const uint64 toFill = (capacity > 5) ? (capacity - 5) : 0;
	for (uint64 i = 0; i < toFill; ++i)
	{
		const id u = id::randomValue();
		increaseEnergy(u, price);
		EXPECT_EQ(ctl.buyTicket(u, price).returnCode, RL::EReturnCode::SUCCESS);
	}
	EXPECT_EQ(ctl.state()->getPlayerCounter(), toFill);

	// Try to buy 10 tickets — only remaining 5 accepted, the rest refunded
	const id buyer = id::randomValue();
	increaseEnergy(buyer, price * 10);
	const uint64 balBefore = getBalance(buyer);
	const RL::BuyTicket_output out = ctl.buyTicket(buyer, price * 10);
	EXPECT_EQ(out.returnCode, RL::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.state()->getPlayerCounter(), capacity);
	EXPECT_EQ(getBalance(buyer), balBefore - price * 5);
}

TEST(ContractRandomLottery, BuyMultipleTickets_AllSoldOut)
{
	ContractTestingRL ctl;
	ctl.beginEpochWithValidTime();
	const uint64 price = ctl.state()->getTicketPrice();
	const uint64 capacity = ctl.getPlayers().players.capacity();

	// Fill to capacity
	for (uint64 i = 0; i < capacity; ++i)
	{
		const id u = id::randomValue();
		increaseEnergy(u, price);
		EXPECT_EQ(ctl.buyTicket(u, price).returnCode, RL::EReturnCode::SUCCESS);
	}
	EXPECT_EQ(ctl.state()->getPlayerCounter(), capacity);

	// Any purchase refunds the full amount and returns ALL_SOLD_OUT code
	const id buyer = id::randomValue();
	increaseEnergy(buyer, price * 3);
	const uint64 balBefore = getBalance(buyer);
	const RL::BuyTicket_output out = ctl.buyTicket(buyer, price * 3);
	EXPECT_EQ(out.returnCode, RL::EReturnCode::TICKET_ALL_SOLD_OUT);
	EXPECT_EQ(getBalance(buyer), balBefore);
}

// functions related to schedule and draw hour

TEST(ContractRandomLottery, GetSchedule_And_SetSchedule)
{
	ContractTestingRL ctl;

	// Default schedule set on initialize must include Wednesday (bit 0)
	const RL::GetSchedule_output s0 = ctl.getSchedule();
	EXPECT_NE(s0.schedule, 0u);

	// Access control: random user cannot set schedule
	const id rnd = id::randomValue();
	increaseEnergy(rnd, 1);
	const RL::SetSchedule_output outDenied = ctl.setSchedule(rnd, RL_ANY_DAY_DRAW_SCHEDULE);
	EXPECT_EQ(outDenied.returnCode, RL::EReturnCode::ACCESS_DENIED);

	// Invalid value: zero mask not allowed
	increaseEnergy(RL_DEV_ADDRESS, 1);
	const RL::SetSchedule_output outInvalid = ctl.setSchedule(RL_DEV_ADDRESS, 0);
	EXPECT_EQ(outInvalid.returnCode, RL::EReturnCode::INVALID_VALUE);

	// Valid update queues into NextEpochData and applies after END_EPOCH
	const uint8 newMask = 0x5A; // some non-zero mask (bits set for selected days)
	const RL::SetSchedule_output outOk = ctl.setSchedule(RL_DEV_ADDRESS, newMask);
	EXPECT_EQ(outOk.returnCode, RL::EReturnCode::SUCCESS);
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.schedule, newMask);

	// Not applied yet
	EXPECT_NE(ctl.getSchedule().schedule, newMask);

	// Apply
	ctl.EndEpoch();
	EXPECT_EQ(ctl.getSchedule().schedule, newMask);
	EXPECT_EQ(ctl.getNextEpochData().nextEpochData.schedule, 0u);
}

TEST(ContractRandomLottery, GetDrawHour_DefaultAfterBeginEpoch)
{
	ContractTestingRL ctl;

	// Initially drawHour is 0 (not configured)
	EXPECT_EQ(ctl.getDrawHour().drawHour, 0u);

	// After BeginEpoch default is 11 UTC
	ctl.beginEpochWithValidTime();
	EXPECT_EQ(ctl.getDrawHour().drawHour, RL_DEFAULT_DRAW_HOUR);
}
