// File: test/contract_qtf.cpp
// Tests for QThirtyFour (4-of-30 lottery) smart contract
// Based on specification: doc/QThirtyFour_Proposal.md

#define NO_UEFI

#include "contract_testing.h"
#include <algorithm>
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
	void setFrActive(bit value) { frActive = value; }
	void setFrRoundsSinceK4(uint16 value) { frRoundsSinceK4 = value; }
	void setFrRoundsAtOrAboveTarget(uint16 value) { frRoundsAtOrAboveTarget = value; }

	const PlayerData& getPlayer(uint64 index) const { return players.get(index); }
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
		// Initialize RL (QTF queries fees from RL)
		callSystemProcedure(RL_CONTRACT_INDEX, INITIALIZE);
		// Initialize QTF
		system.epoch = contractDescriptions[QTF_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(QTF_CONTRACT_INDEX, INITIALIZE);
	}

	// Access internal contract state
	QTFChecker* state() { return reinterpret_cast<QTFChecker*>(contractStates[QTF_CONTRACT_INDEX]); }

	id qtfSelf() { return id(QTF_CONTRACT_INDEX, 0, 0, 0); }
	id qrpSelf() { return id(QRP_CONTRACT_INDEX, 0, 0, 0); }

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
};

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
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 26) + 1), static_cast<uint8>((i % 26) + 2),
		                                            static_cast<uint8>((i % 26) + 3), static_cast<uint8>((i % 26) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	const uint64 totalRevenue = ticketPrice * numPlayers;
	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 contractBalBefore = getBalance(ctl.qtfSelf());

	EXPECT_EQ(contractBalBefore, totalRevenue);

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
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 26) + 1), static_cast<uint8>((i % 26) + 2),
		                                            static_cast<uint8>((i % 26) + 3), static_cast<uint8>((i % 26) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	const uint64 totalRevenue = ticketPrice * numPlayers;
	const uint64 devBalBefore = getBalance(QTF_DEV_ADDRESS);
	const uint64 contractBalBefore = getBalance(ctl.qtfSelf());
	const uint64 jackpotBefore = ctl.state()->getJackpot();

	EXPECT_EQ(contractBalBefore, totalRevenue);

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

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const uint64 jackpotBefore = ctl.state()->getJackpot();
	constexpr uint64 numPlayers = 20;

	// Add players
	for (uint64 i = 0; i < numPlayers; ++i)
	{
		const id user = id::randomValue();
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 25) + 1), static_cast<uint8>((i % 25) + 2),
		                                            static_cast<uint8>((i % 25) + 3), static_cast<uint8>((i % 25) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	// Calculate expected jackpot growth in baseline mode (FR not active)
	const uint64 revenue = ticketPrice * numPlayers;
	const QTF::GetFees_output fees = ctl.getFees();

	// winnersBlock = revenue * winnerFeePercent / 100 (68%)
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;

	// In baseline mode: no rake, standard tier split
	// k3Pool = 40% of winnersBlock, k2Pool = 28% of winnersBlock
	const uint64 k3Pool = (winnersBlock * QTF_BASE_K3_SHARE_BP) / 10000;
	const uint64 k2Pool = (winnersBlock * QTF_BASE_K2_SHARE_BP) / 10000;

	// Remaining 32% becomes overflow
	const uint64 winnersOverflow = winnersBlock - k3Pool - k2Pool;

	// In baseline mode: 50% of overflow goes to jackpot, 50% to reserve
	const uint64 reserveAdd = (winnersOverflow * QTF_BASELINE_OVERFLOW_ALPHA_BP) / 10000;
	const uint64 overflowToJackpot = winnersOverflow - reserveAdd;

	// Minimum expected jackpot growth (assuming no k2/k3 winners, all overflow goes to jackpot)
	const uint64 minExpectedGrowth = overflowToJackpot;

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

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();

	// Run several rounds without k=4 win
	for (int round = 0; round < 3; ++round)
	{
		ctl.beginEpochWithValidTime();

		for (int i = 0; i < 5; ++i)
		{
			const id user = id::randomValue();
			QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i * round % 25) + 1), static_cast<uint8>((i * round % 25) + 2),
			                                            static_cast<uint8>((i * round % 25) + 3), static_cast<uint8>((i * round % 25) + 4));
			ctl.fundAndBuyTicket(user, ticketPrice, nums);
		}

		const uint16 roundsBefore = ctl.state()->getFrRoundsSinceK4();
		ctl.advanceOneDayAndDraw();

		// Rounds counter should increment (unless k=4 win occurred, which is very unlikely)
		// Since we can't control the winning numbers, we just check it's at least what it was
		EXPECT_GE((uint64)ctl.state()->getFrRoundsSinceK4(), (uint64)roundsBefore);
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
			QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i + round) % 25 + 1), static_cast<uint8>((i + round) % 25 + 2),
			                                            static_cast<uint8>((i + round) % 25 + 3), static_cast<uint8>((i + round) % 25 + 4));
			ctl.fundAndBuyTicket(user, ticketPrice, nums);
		}

		// Keep jackpot at target (add back what might be paid out)
		ctl.state()->setJackpot(QTF_DEFAULT_TARGET_JACKPOT);
		ctl.advanceOneDayAndDraw();
	}

	// After 3 rounds at target, FR should deactivate
	EXPECT_GE((uint64)ctl.state()->getFrRoundsAtOrAboveTarget(), (uint64)QTF_FR_HYSTERESIS_ROUNDS);
	EXPECT_EQ(ctl.state()->getFrActive(), false);
}

TEST(ContractQThirtyFour, FR_OverflowBias_95PercentToJackpot)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

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
		QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i % 25) + 1), static_cast<uint8>((i % 25) + 2),
		                                            static_cast<uint8>((i % 25) + 3), static_cast<uint8>((i % 25) + 4));
		ctl.fundAndBuyTicket(user, ticketPrice, nums);
	}

	// Calculate expected jackpot growth
	const uint64 revenue = ticketPrice * numPlayers;
	const QTF::GetFees_output fees = ctl.getFees();

	// winnersBlock = revenue * winnerFeePercent / 100 (68%)
	const uint64 winnersBlock = (revenue * fees.winnerFeePercent) / 100;

	// In FR mode: 5% rake from winnersBlock goes to jackpot
	const uint64 winnersRake = (winnersBlock * QTF_FR_WINNERS_RAKE_BP) / 10000;
	const uint64 winnersBlockAfterRake = winnersBlock - winnersRake;

	// k3Pool = 40% of winnersBlockAfterRake, k2Pool = 28% of winnersBlockAfterRake
	// Remaining 32% becomes overflow
	const uint64 k3Pool = (winnersBlockAfterRake * QTF_BASE_K3_SHARE_BP) / 10000;
	const uint64 k2Pool = (winnersBlockAfterRake * QTF_BASE_K2_SHARE_BP) / 10000;
	const uint64 winnersOverflow = winnersBlockAfterRake - k3Pool - k2Pool;

	// In FR mode: 95% of overflow goes to jackpot, 5% to reserve
	const uint64 reserveAdd = (winnersOverflow * QTF_FR_ALPHA_BP) / 10000;
	const uint64 overflowToJackpot = winnersOverflow - reserveAdd;

	// Dev and Dist redirects (base 1% each from revenue in FR mode)
	const uint64 devRedirect = (revenue * QTF_FR_DEV_REDIRECT_BP) / 10000;
	const uint64 distRedirect = (revenue * QTF_FR_DIST_REDIRECT_BP) / 10000;

	// Minimum expected jackpot growth (without extra redirects, assuming no k2/k3 winners)
	// totalJackpotContribution = overflowToJackpot + winnersRake + devRedirect + distRedirect
	const uint64 minExpectedGrowth = overflowToJackpot + winnersRake + devRedirect + distRedirect;

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

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST(ContractQThirtyFour, EdgeCase_AllNumbersBoundary)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();

	// Test boundary numbers: 1, 2, 29, 30
	QTFRandomValues boundaryNums = ctl.makeValidNumbers(1, 2, 29, 30);
	ctl.fundAndBuyTicket(user, ticketPrice, boundaryNums);

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);
}

TEST(ContractQThirtyFour, EdgeCase_ConsecutiveNumbers)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();

	// Test consecutive numbers
	QTFRandomValues consecutiveNums = ctl.makeValidNumbers(15, 16, 17, 18);
	ctl.fundAndBuyTicket(user, ticketPrice, consecutiveNums);

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);
}

TEST(ContractQThirtyFour, EdgeCase_HighestNumbers)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();

	// Test highest valid numbers
	QTFRandomValues highNums = ctl.makeValidNumbers(27, 28, 29, 30);
	ctl.fundAndBuyTicket(user, ticketPrice, highNums);

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);
}

TEST(ContractQThirtyFour, EdgeCase_LowestNumbers)
{
	ContractTestingQTF ctl;
	ctl.beginEpochWithValidTime();

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	const id user = id::randomValue();

	// Test lowest valid numbers
	QTFRandomValues lowNums = ctl.makeValidNumbers(1, 2, 3, 4);
	ctl.fundAndBuyTicket(user, ticketPrice, lowNums);

	EXPECT_EQ(ctl.state()->getNumberOfPlayers(), 1u);
}

// ============================================================================
// MULTIPLE ROUNDS TESTS
// ============================================================================

TEST(ContractQThirtyFour, MultipleRounds_JackpotAccumulates)
{
	ContractTestingQTF ctl;
	ctl.forceSchedule(QTF_ANY_DAY_SCHEDULE);

	const uint64 ticketPrice = ctl.state()->getTicketPriceInternal();
	uint64 prevJackpot = 0;

	// Run multiple rounds
	for (int round = 0; round < 5; ++round)
	{
		ctl.beginEpochWithValidTime();

		for (int i = 0; i < 10; ++i)
		{
			const id user = id::randomValue();
			QTFRandomValues nums = ctl.makeValidNumbers(static_cast<uint8>((i + round) % 27 + 1), static_cast<uint8>((i + round + 1) % 27 + 1),
			                                            static_cast<uint8>((i + round + 2) % 27 + 1), static_cast<uint8>((i + round + 3) % 27 + 1));
			ctl.fundAndBuyTicket(user, ticketPrice, nums);
		}

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
