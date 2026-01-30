#define NO_UEFI
#define _ALLOW_KEYWORD_MACROS
#define private protected
#include "contract_testing.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

constexpr uint16 PROCEDURE_INDEX_CREATE_ROOM = 1;
constexpr uint16 PROCEDURE_INDEX_CONNECT_ROOM = 2;
constexpr uint16 PROCEDURE_INDEX_SET_PERCENT_FEES = 3;
constexpr uint16 PROCEDURE_INDEX_SET_TTL_HOURS = 4;
constexpr uint16 PROCEDURE_INDEX_DEPOSIT = 5;
constexpr uint16 PROCEDURE_INDEX_WITHDRAW = 6;
constexpr uint16 FUNCTION_INDEX_GET_PERCENT_FEES = 1;
constexpr uint16 FUNCTION_INDEX_GET_ROOMS = 2;
constexpr uint16 FUNCTION_INDEX_GET_TTL_HOURS = 3;
constexpr uint16 FUNCTION_INDEX_GET_USER_PROFILE = 4;

static const id QDUEL_TEAM_ADDRESS =
    ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L, _W, _E, _T, _H, _N, _G,
       _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

class QpiContextUserFunctionCallWithInvocator : public QpiContextFunctionCall
{
public:
	QpiContextUserFunctionCallWithInvocator(unsigned int contractIndex, const id& invocator)
	    : QpiContextFunctionCall(contractIndex, invocator, 0, USER_FUNCTION_CALL)
	{}
};

class QDuelChecker : public QDUEL
{
public:
	// Expose read-only accessors for internal state so tests can assert without
	// modifying contract storage directly.
	uint64 roomCount() const { return rooms.population(); }
	id team() const { return teamAddress; }
	uint8 ttl() const { return ttlHours; }
	uint8 devFee() const { return devFeePercentBps; }
	uint8 burnFee() const { return burnFeePercentBps; }
	uint8 shareholdersFee() const { return shareholdersFeePercentBps; }
	sint64 minDuelAmount() const { return minimumDuelAmount; }
	void setState(EState newState) { currentState = newState; }
	EState getState() const { return currentState; }
	// Helper to fetch user record without exposing contract internals.
	bool getUserData(const id& user, UserData& data) const { return users.get(user, data); }
	// Directly set a user record to simulate edge-case storage edits.
	void setUserData(const UserData& data) { users.set(data.userId, data); }

	RoomInfo firstRoom() const
	{
		// Map storage can be sparse; walk to first element.
		const sint64 index = rooms.nextElementIndex(NULL_INDEX);
		if (index == NULL_INDEX)
		{
			return RoomInfo{};
		}
		return rooms.value(index);
	}

	bool hasRoom(const id& roomId) const { return rooms.contains(roomId); }

	id computeWinner(const id& player1, const id& player2) const
	{
		// Run the same winner function as the contract to keep tests deterministic.
		QpiContextUserFunctionCall qpi(QDUEL_CONTRACT_INDEX);
		GetWinnerPlayer_input input{player1, player2};
		GetWinnerPlayer_output output{};
		GetWinnerPlayer_locals locals{};
		GetWinnerPlayer(qpi, *this, input, output, locals);
		return output.winner;
	}

	void calculateRevenue(uint64 amount, CalculateRevenue_output& output) const
	{
		QpiContextUserFunctionCall qpi(QDUEL_CONTRACT_INDEX);

		// Contract helpers require zeroed outputs and locals.
		output = {};
		CalculateRevenue_input revenueInput{amount};
		CalculateRevenue_locals revenueLocals{};
		CalculateRevenue(qpi, *this, revenueInput, output, revenueLocals);
	}

	GetUserProfile_output getUserProfileFor(const id& user) const
	{
		QpiContextUserFunctionCallWithInvocator qpi(QDUEL_CONTRACT_INDEX, user);
		GetUserProfile_input input{};
		GetUserProfile_output output{};
		GetUserProfile_locals locals{};
		GetUserProfile(qpi, *this, input, output, locals);
		return output;
	}
};

class ContractTestingQDuel : protected ContractTesting
{
public:
	ContractTestingQDuel()
	{
		// Build an empty chain state and deploy the contract under test.
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QDUEL);
		system.epoch = contractDescriptions[QDUEL_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(QDUEL_CONTRACT_INDEX, INITIALIZE);
	}

	// Access helper for the underlying contract state.
	QDuelChecker* state() { return reinterpret_cast<QDuelChecker*>(contractStates[QDUEL_CONTRACT_INDEX]); }

	QDUEL::CreateRoom_output createRoom(const id& user, const id& allowedPlayer, sint64 stake, sint64 raiseStep, sint64 maxStake, sint64 reward)
	{
		QDUEL::CreateRoom_input input{allowedPlayer, stake, raiseStep, maxStake};
		QDUEL::CreateRoom_output output;
		// Route through contract procedure to keep call path identical to production.
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_CREATE_ROOM, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	QDUEL::ConnectToRoom_output connectToRoom(const id& user, const id& roomId, sint64 reward)
	{
		QDUEL::ConnectToRoom_input input{roomId};
		QDUEL::ConnectToRoom_output output;
		// Call the user procedure so validation and state updates are exercised.
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_CONNECT_ROOM, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	QDUEL::SetPercentFees_output setPercentFees(const id& user, uint8 devFee, uint8 burnFee, uint8 shareholdersFee, sint64 reward = 0)
	{
		QDUEL::SetPercentFees_input input{devFee, burnFee, shareholdersFee};
		QDUEL::SetPercentFees_output output;
		// System procedures are tested via normal user invocation.
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_SET_PERCENT_FEES, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	QDUEL::SetTTLHours_output setTtlHours(const id& user, uint8 ttlHours, sint64 reward = 0)
	{
		QDUEL::SetTTLHours_input input{ttlHours};
		QDUEL::SetTTLHours_output output;
		// Ensure contract state gets updated through procedure validation.
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_SET_TTL_HOURS, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	QDUEL::GetPercentFees_output getPercentFees()
	{
		QDUEL::GetPercentFees_input input{};
		QDUEL::GetPercentFees_output output;
		// Read-only function call for fee snapshot.
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_PERCENT_FEES, input, output);
		return output;
	}

	QDUEL::GetRooms_output getRooms()
	{
		QDUEL::GetRooms_input input{};
		QDUEL::GetRooms_output output;
		// Read-only function call for rooms snapshot.
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_ROOMS, input, output);
		return output;
	}

	QDUEL::GetTTLHours_output getTtlHours()
	{
		QDUEL::GetTTLHours_input input{};
		QDUEL::GetTTLHours_output output;
		// Read-only function call for TTL configuration.
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_TTL_HOURS, input, output);
		return output;
	}

	QDUEL::GetUserProfile_output getUserProfile()
	{
		QDUEL::GetUserProfile_input input{};
		QDUEL::GetUserProfile_output output;
		// Read-only function call for caller profile.
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_USER_PROFILE, input, output);
		return output;
	}

	QDUEL::Deposit_output deposit(const id& user, sint64 reward)
	{
		QDUEL::Deposit_input input{};
		QDUEL::Deposit_output output;
		// Deposit is a user procedure that mutates balance and state.
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_DEPOSIT, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	QDUEL::Withdraw_output withdraw(const id& user, sint64 amount, sint64 reward = 0)
	{
		QDUEL::Withdraw_input input{amount};
		QDUEL::Withdraw_output output;
		// Withdraw uses user procedure to enforce validations and limits.
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_WITHDRAW, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	// Helpers that dispatch system procedures during lifecycle tests.
	void endTick() { callSystemProcedure(QDUEL_CONTRACT_INDEX, END_TICK); }

	void endEpoch() { callSystemProcedure(QDUEL_CONTRACT_INDEX, END_EPOCH); }

	void beginEpoch() { callSystemProcedure(QDUEL_CONTRACT_INDEX, BEGIN_EPOCH); }

	// Control time and tick for deterministic tests.
	void setTick(uint32 tick) { system.tick = tick; }
	uint32 getTick() const { return system.tick; }

	void forceEndTick()
	{
		// Align tick to update period so END_TICK work executes.
		system.tick = system.tick + (QDUEL_TICK_UPDATE_PERIOD - mod(system.tick, static_cast<uint32>(QDUEL_TICK_UPDATE_PERIOD)));

		endTick();
	}

	void setDeterministicTime(uint16 year = 2025, uint8 month = 1, uint8 day = 1, uint8 hour = 0)
	{
		// Set a fixed time and reset etalon tick so tests are stable.
		setMemory(utcTime, 0);
		utcTime.Year = year;
		utcTime.Month = month;
		utcTime.Day = day;
		utcTime.Hour = hour;
		utcTime.Minute = 0;
		utcTime.Second = 0;
		utcTime.Nanosecond = 0;
		updateQpiTime();
		etalonTick.prevSpectrumDigest = m256i::zero();
	}
};

namespace
{
	bool findPlayersForWinner(ContractTestingQDuel& qduel, bool wantPlayer1Win, id& player1, id& player2)
	{
		// Brute-force deterministic ids until winner matches desired side.
		for (uint64 i = 1; i < 10000; ++i)
		{
			const id candidate1(i, 0, 0, 0);
			const id candidate2(i + 1, 0, 0, 0);
			const id winner = qduel.state()->computeWinner(candidate1, candidate2);
			if (winner == (wantPlayer1Win ? candidate1 : candidate2))
			{
				player1 = candidate1;
				player2 = candidate2;
				return true;
			}
		}
		return false;
	}

	void runFullGameCycleWithFees(ContractTestingQDuel& qduel, const id& player1, const id& player2, const id& expectedWinner)
	{
		// Setup shareholders so revenue distribution can be validated.
		const id shareholder1 = id::randomValue();
		const id shareholder2 = id::randomValue();
		constexpr unsigned int rlSharesOwner1 = 100;
		constexpr unsigned int rlSharesOwner2 = 576;
		std::vector<std::pair<m256i, unsigned int>> rlShares{
		    {shareholder1, rlSharesOwner1},
		    {shareholder2, rlSharesOwner2},
		};
		issueContractShares(RL_CONTRACT_INDEX, rlShares);

		// Set fees as the team address (contract owner).
		constexpr uint8 devFee = 15;
		constexpr uint8 burnFee = 30;
		constexpr uint8 shareholdersFee = 55;
		increaseEnergy(qduel.state()->team(), 1);
		EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), devFee, burnFee, shareholdersFee).returnCode,
		          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

		// Setup: give both players enough balance to cover the duel.
		constexpr sint64 duelAmount = 100000ULL;
		increaseEnergy(player1, duelAmount);
		increaseEnergy(player2, duelAmount);
		const uint64 player1Before = getBalance(player1);
		const uint64 player2Before = getBalance(player2);

		const uint64 teamBefore = getBalance(qduel.state()->team());
		const uint64 shareholder1Before = getBalance(shareholder1);
		const uint64 shareholder2Before = getBalance(shareholder2);

		// Create room and keep initial balance snapshots for payout assertions.
		EXPECT_EQ(qduel.createRoom(player1, NULL_ID, duelAmount, 1, duelAmount, duelAmount).returnCode,
		          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
		const uint64 player1AfterCreateRoom = getBalance(player1);

		const id winner = qduel.state()->computeWinner(player1, player2);
		EXPECT_EQ(winner, expectedWinner);

		// Calculate expected revenue distribution for fees and winner.
		QDUEL::CalculateRevenue_output revenueOutput{};
		qduel.state()->calculateRevenue(duelAmount * 2, revenueOutput);

		// Player 2 joins and triggers finalize logic.
		EXPECT_EQ(qduel.connectToRoom(player2, qduel.state()->firstRoom().roomId, duelAmount).returnCode,
		          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
		const uint64 player2AfterConnectToRoom = getBalance(player2);

		// Check fee distribution for team and shareholders.
		EXPECT_EQ(getBalance(qduel.state()->team()), teamBefore + revenueOutput.devFee);

		// Check shareholder dividends across the full set of computors.
		const uint64 dividendPerShare = revenueOutput.shareholdersFee / NUMBER_OF_COMPUTORS;
		EXPECT_EQ(getBalance(shareholder1), shareholder1Before + dividendPerShare * rlSharesOwner1);
		EXPECT_EQ(getBalance(shareholder2), shareholder2Before + dividendPerShare * rlSharesOwner2);

		// Check winner receives the remainder and loser only pays entry.
		if (winner == player1)
		{
			EXPECT_EQ(getBalance(player1), player1AfterCreateRoom + revenueOutput.winner);
			EXPECT_EQ(getBalance(player2), player2Before - duelAmount);
		}
		else
		{
			EXPECT_EQ(getBalance(player1), player1Before - duelAmount);
			EXPECT_EQ(getBalance(player2), (player2Before - duelAmount) + revenueOutput.winner);
		}
	}
} // namespace

TEST(ContractQDuel, EndEpochKeepsDepositWhileRoomsRecreatedEachEpoch)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(40, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	const uint64 epochs = 3;
	const uint64 reward = stake + (stake * epochs);
	increaseEnergy(owner, reward);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, reward).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::UserData ownerData{};
	ASSERT_TRUE(qduel.state()->getUserData(owner, ownerData));
	uint64 expectedDeposit = ownerData.depositedAmount;
	id currentRoomId = ownerData.roomId;

	for (uint32 epoch = 0; epoch < epochs; ++epoch)
	{
		qduel.beginEpoch();
		qduel.endEpoch();
		qduel.setTick(qduel.getTick() + 1);

		QDUEL::UserData afterEndEpoch{};
		ASSERT_TRUE(qduel.state()->getUserData(owner, afterEndEpoch));
		EXPECT_EQ(afterEndEpoch.depositedAmount, expectedDeposit);
		EXPECT_EQ(afterEndEpoch.roomId, currentRoomId);

		qduel.state()->setState(QDUEL::EState::NONE);

		const id opponent(200 + epoch, 0, 0, 0);
		increaseEnergy(opponent, stake);
		EXPECT_EQ(qduel.connectToRoom(opponent, currentRoomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

		ASSERT_TRUE(qduel.state()->getUserData(owner, ownerData));
		EXPECT_NE(ownerData.roomId, currentRoomId);
		EXPECT_EQ(ownerData.locked, stake);
		expectedDeposit -= stake;
		EXPECT_EQ(ownerData.depositedAmount, expectedDeposit);
		currentRoomId = ownerData.roomId;
	}
}

TEST(ContractQDuel, BeginEpochKeepsRoomsAndUsers)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2022, 4, 13, 0);

	const id owner(1, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Give the owner enough balance to create a room.
	increaseEnergy(owner, stake);

	// Create a room and verify it survives epoch transition.
	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();
	QDUEL::UserData userBefore{};
	EXPECT_TRUE(qduel.state()->getUserData(owner, userBefore));

	// Begin epoch should not wipe persistent data.
	qduel.beginEpoch();

	// Room and user record should still exist after epoch transition.
	EXPECT_TRUE(qduel.state()->hasRoom(roomBefore.roomId));
	QDUEL::UserData userAfter{};
	EXPECT_TRUE(qduel.state()->getUserData(owner, userAfter));
	EXPECT_EQ(userAfter.roomId, roomBefore.roomId);
}

TEST(ContractQDuel, FirstTickAfterUnlockResetsTimerStart)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2022, 4, 13, 0);

	const id owner(2, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund owner so the room creation succeeds.
	increaseEnergy(owner, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();
	const uint64 initialCloseTimer = roomBefore.closeTimer;
	const DateAndTime initialLastUpdate = roomBefore.lastUpdate;

	// Locking occurs at epoch start; timers should not advance while locked.
	qduel.beginEpoch();

	// Still locked: no timer or lastUpdate changes.
	qduel.setDeterministicTime(2022, 4, 13, 1);
	qduel.forceEndTick();

	const QDUEL::RoomInfo lockedRoom = qduel.state()->firstRoom();
	EXPECT_EQ(lockedRoom.closeTimer, initialCloseTimer);
	EXPECT_EQ(lockedRoom.lastUpdate, initialLastUpdate);

	// First unlocked tick: reset lastUpdate to "now" without reducing timer.
	qduel.setDeterministicTime(2022, 4, 14, 2);
	qduel.forceEndTick();

	const QDUEL::RoomInfo unlockedRoom = qduel.state()->firstRoom();
	EXPECT_EQ(unlockedRoom.closeTimer, initialCloseTimer);
	const DateAndTime expectedNow(2022, 4, 14, 2, 0, 0);
	EXPECT_EQ(unlockedRoom.lastUpdate, expectedNow);
}

TEST(ContractQDuel, EndTickExpiresRoomCreatesNewWhenDepositAvailable)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(3, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund owner with enough to re-create room after finalize.
	increaseEnergy(owner, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();
	EXPECT_EQ(qduel.state()->roomCount(), 1ULL);

	// Advance beyond TTL to trigger finalize and auto room creation.
	qduel.setDeterministicTime(2025, 1, 1, 3);
	qduel.forceEndTick();

	// A new room should replace the expired one.
	EXPECT_EQ(qduel.state()->roomCount(), 1ULL);
	const QDUEL::RoomInfo roomAfter = qduel.state()->firstRoom();
	EXPECT_NE(roomAfter.roomId, roomBefore.roomId);

	QDUEL::UserData userAfter{};
	EXPECT_TRUE(qduel.state()->getUserData(owner, userAfter));
	// User should be re-bound to the new room with locked stake.
	EXPECT_EQ(userAfter.roomId, roomAfter.roomId);
	EXPECT_EQ(userAfter.locked, stake);
}

TEST(ContractQDuel, EndTickExpiresRoomWithoutAvailableDepositRemovesUser)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(4, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund owner just enough to create the initial room.
	increaseEnergy(owner, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::UserData userData{};
	ASSERT_TRUE(qduel.state()->getUserData(owner, userData));
	// Remove available balance so finalize cannot recreate the room.
	userData.depositedAmount = 0;
	userData.locked = 0;
	qduel.state()->setUserData(userData);

	// Expire room and expect cleanup.
	qduel.setDeterministicTime(2025, 1, 1, 3);
	qduel.forceEndTick();

	// Room and user data should be removed when deposit is insufficient.
	EXPECT_EQ(qduel.state()->roomCount(), 0ULL);
	QDUEL::UserData userAfter{};
	EXPECT_FALSE(qduel.state()->getUserData(owner, userAfter));
}

TEST(ContractQDuel, EndTickSkipsNonPeriodTicks)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(5, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund owner to create a room.
	increaseEnergy(owner, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();
	qduel.setDeterministicTime(2025, 1, 1, 1);
	qduel.setTick(1);
	// Non-period tick: no updates expected.
	qduel.endTick();

	const QDUEL::RoomInfo roomAfterSkipped = qduel.state()->firstRoom();
	EXPECT_EQ(roomAfterSkipped.closeTimer, roomBefore.closeTimer);
	EXPECT_EQ(roomAfterSkipped.lastUpdate, roomBefore.lastUpdate);

	// Period tick: updates should apply.
	qduel.setTick(QDUEL_TICK_UPDATE_PERIOD);
	qduel.endTick();

	const QDUEL::RoomInfo roomAfterProcessed = qduel.state()->firstRoom();
	// Close timer should have decreased by one hour and lastUpdate bumped.
	EXPECT_EQ(roomAfterProcessed.closeTimer, roomBefore.closeTimer - 3600ULL);
	const DateAndTime expectedNow(2025, 1, 1, 1, 0, 0);
	EXPECT_EQ(roomAfterProcessed.lastUpdate, expectedNow);
}

TEST(ContractQDuel, LockedStateBlocksCreateAndConnect)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(6, 0, 0, 0);
	const id other(7, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund owner to create the baseline room.
	increaseEnergy(owner, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();

	// Lock contract and verify user procedures are blocked.
	qduel.state()->setState(QDUEL::EState::LOCKED);
	// Fund the other user so only the lock gate can fail.
	increaseEnergy(other, stake);

	EXPECT_EQ(qduel.createRoom(other, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::STATE_LOCKED));
	EXPECT_EQ(qduel.connectToRoom(other, roomBefore.roomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::STATE_LOCKED));
	// Existing room should remain unchanged.
	EXPECT_TRUE(qduel.state()->hasRoom(roomBefore.roomId));
}

TEST(ContractQDuel, EndTickRecreatesRoomWithUpdatedStake)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(8, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund owner so next stake can be doubled.
	increaseEnergy(owner, stake * 2);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 2, 0, stake * 2).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();

	// Expire the room and expect a new one using computed next stake.
	qduel.setDeterministicTime(2025, 1, 1, 3);
	qduel.forceEndTick();

	const QDUEL::RoomInfo roomAfter = qduel.state()->firstRoom();
	EXPECT_NE(roomAfter.roomId, roomBefore.roomId);
	// Amount should reflect the raiseStep applied to the original stake.
	EXPECT_EQ(roomAfter.amount, stake * 2);

	QDUEL::UserData userAfter{};
	EXPECT_TRUE(qduel.state()->getUserData(owner, userAfter));
	// User should be locked into the new room with the updated stake.
	EXPECT_EQ(userAfter.roomId, roomAfter.roomId);
	EXPECT_EQ(userAfter.locked, stake * 2);
	EXPECT_EQ(userAfter.depositedAmount, 0ULL);
}

TEST(ContractQDuel, ConnectFinalizeIgnoresLockedAmount)
{
	ContractTestingQDuel qduel;
	// Start from a deterministic time and unlocked state.
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(9, 0, 0, 0);
	const id opponent(10, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	// Fund both players so creation and join can proceed.
	increaseEnergy(owner, stake);
	increaseEnergy(opponent, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();

	// On connect, finalize uses includeLocked=false, so owner data is cleared.
	EXPECT_EQ(qduel.connectToRoom(opponent, roomBefore.roomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Room is removed and owner record should be purged after finalize.
	EXPECT_FALSE(qduel.state()->hasRoom(roomBefore.roomId));
	QDUEL::UserData ownerAfter{};
	EXPECT_FALSE(qduel.state()->getUserData(owner, ownerAfter));
}

TEST(ContractQDuel, InitializeDefaults)
{
	ContractTestingQDuel qduel;

	EXPECT_EQ(qduel.state()->team(), QDUEL_TEAM_ADDRESS);
	EXPECT_EQ(qduel.state()->minDuelAmount(), static_cast<uint64>(QDUEL_MINIMUM_DUEL_AMOUNT));
	EXPECT_EQ(qduel.state()->devFee(), QDUEL_DEV_FEE_PERCENT_BPS);
	EXPECT_EQ(qduel.state()->burnFee(), QDUEL_BURN_FEE_PERCENT_BPS);
	EXPECT_EQ(qduel.state()->shareholdersFee(), QDUEL_SHAREHOLDERS_FEE_PERCENT_BPS);
	EXPECT_EQ(qduel.state()->ttl(), QDUEL_TTL_HOURS);
	EXPECT_EQ(qduel.state()->getState(), QDUEL::EState::NONE);
	EXPECT_EQ(qduel.state()->roomCount(), 0ULL);
}

TEST(ContractQDuel, CreateRoomStoresRoomAndUser)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);
	qduel.setDeterministicTime(2025, 1, 1, 0);

	const id owner(11, 0, 0, 0);
	const id allowed(12, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	const uint64 reward = stake + 5000;
	increaseEnergy(owner, reward);

	EXPECT_EQ(qduel.createRoom(owner, allowed, stake, 2, stake * 3, reward).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(qduel.state()->roomCount(), 1ULL);
	const QDUEL::RoomInfo room = qduel.state()->firstRoom();
	EXPECT_EQ(room.owner, owner);
	EXPECT_EQ(room.allowedPlayer, allowed);
	EXPECT_EQ(room.amount, stake);
	EXPECT_EQ(room.closeTimer, static_cast<uint64>(qduel.state()->ttl()) * 3600ULL);
	const DateAndTime expectedNow(2025, 1, 1, 0, 0, 0);
	EXPECT_EQ(room.lastUpdate, expectedNow);

	QDUEL::UserData user{};
	EXPECT_TRUE(qduel.state()->getUserData(owner, user));
	EXPECT_EQ(user.roomId, room.roomId);
	EXPECT_EQ(user.allowedPlayer, allowed);
	EXPECT_EQ(user.depositedAmount, reward - stake);
	EXPECT_EQ(user.locked, stake);
	EXPECT_EQ(user.stake, stake);
	EXPECT_EQ(user.raiseStep, 2ULL);
	EXPECT_EQ(user.maxStake, stake * 3);
}

TEST(ContractQDuel, CreateRoomRejectsStakeBelowMinimum)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(13, 0, 0, 0);
	const uint64 stake = qduel.state()->minDuelAmount() - 1;
	const uint64 reward = qduel.state()->minDuelAmount();
	increaseEnergy(owner, reward);
	const uint64 balanceBefore = getBalance(owner);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, reward).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INVALID_VALUE));
	EXPECT_EQ(getBalance(owner), balanceBefore);
	EXPECT_EQ(qduel.state()->roomCount(), 0ULL);
}

TEST(ContractQDuel, CreateRoomRejectsMaxStakeBelowStake)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(14, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	const uint64 reward = stake;
	increaseEnergy(owner, reward);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake - 1, reward).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INVALID_VALUE));
	EXPECT_EQ(qduel.state()->roomCount(), 0ULL);
}

TEST(ContractQDuel, CreateRoomRejectsRewardBelowMinimum)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(15, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	const uint64 reward = qduel.state()->minDuelAmount() - 1;
	increaseEnergy(owner, reward);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, reward).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT));
	EXPECT_EQ(qduel.state()->roomCount(), 0ULL);
}

TEST(ContractQDuel, CreateRoomRejectsRewardBelowStake)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(16, 0, 0, 0);
	const uint64 stake = qduel.state()->minDuelAmount() + 1000;
	const uint64 reward = stake - 1;
	increaseEnergy(owner, reward);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, reward).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT));
	EXPECT_EQ(qduel.state()->roomCount(), 0ULL);
}

TEST(ContractQDuel, CreateRoomRejectsDuplicateUser)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(17, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake * 2);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::USER_ALREADY_EXISTS));
	EXPECT_EQ(qduel.state()->roomCount(), 1ULL);
}

TEST(ContractQDuel, CreateRoomRejectsWhenRoomsFull)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const sint64 stake = qduel.state()->minDuelAmount();
	for (uint32 i = 0; i < QDUEL_MAX_NUMBER_OF_ROOMS; ++i)
	{
		const id owner(100 + i, 0, 0, 0);
		qduel.setTick(i);
		increaseEnergy(owner, stake);
		const auto output = qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake);
		EXPECT_EQ(output.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS)) << "at[" << i << "]";
	}

	EXPECT_EQ(qduel.state()->roomCount(), static_cast<uint64>(QDUEL_MAX_NUMBER_OF_ROOMS));

	const id extraOwner(9999, 0, 0, 0);
	increaseEnergy(extraOwner, stake);
	EXPECT_EQ(qduel.createRoom(extraOwner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_FULL));
}

TEST(ContractQDuel, ConnectToRoomRejectsMissingRoom)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id player(18, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(player, stake);

	EXPECT_EQ(qduel.connectToRoom(player, id(999, 0, 0, 0), stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_NOT_FOUND));
}

TEST(ContractQDuel, ConnectToRoomRejectsNotAllowedPlayer)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(19, 0, 0, 0);
	const id allowed(20, 0, 0, 0);
	const id other(21, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake);
	increaseEnergy(other, stake);

	EXPECT_EQ(qduel.createRoom(owner, allowed, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo room = qduel.state()->firstRoom();

	EXPECT_EQ(qduel.connectToRoom(other, room.roomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_ACCESS_DENIED));
}

TEST(ContractQDuel, ConnectToRoomRejectsInsufficientReward)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(22, 0, 0, 0);
	const id opponent(23, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake);
	increaseEnergy(opponent, stake - 1);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo room = qduel.state()->firstRoom();

	EXPECT_EQ(qduel.connectToRoom(opponent, room.roomId, stake - 1).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT));
}

TEST(ContractQDuel, ConnectToRoomRefundsExcessRewardForLoser)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	id owner;
	id opponent;
	ASSERT_TRUE(findPlayersForWinner(qduel, true, owner, opponent));

	const sint64 stake = qduel.state()->minDuelAmount();
	const uint64 reward = stake + 5000;
	increaseEnergy(owner, stake);
	increaseEnergy(opponent, reward);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const uint64 opponentBefore = getBalance(opponent);

	EXPECT_EQ(qduel.connectToRoom(opponent, qduel.state()->firstRoom().roomId, reward).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(getBalance(opponent), opponentBefore - stake);
}

TEST(ContractQDuel, ConnectFinalizeCreatesRoomFromDeposit)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner(24, 0, 0, 0);
	const id opponent(25, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake * 2);
	increaseEnergy(opponent, stake);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, 0, stake * 2).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo roomBefore = qduel.state()->firstRoom();

	qduel.setTick(10);

	EXPECT_EQ(qduel.connectToRoom(opponent, roomBefore.roomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(qduel.state()->roomCount(), 1ULL);
	const QDUEL::RoomInfo roomAfter = qduel.state()->firstRoom();
	EXPECT_NE(roomAfter.roomId, roomBefore.roomId);
	EXPECT_EQ(roomAfter.owner, owner);
	EXPECT_EQ(roomAfter.amount, stake);

	QDUEL::UserData userAfter{};
	EXPECT_TRUE(qduel.state()->getUserData(owner, userAfter));
	EXPECT_EQ(userAfter.roomId, roomAfter.roomId);
	EXPECT_EQ(userAfter.locked, stake);
	EXPECT_EQ(userAfter.depositedAmount, 0ULL);
}

TEST(ContractQDuel, GetWinnerPlayerIsOrderInvariant)
{
	ContractTestingQDuel qduel;
	qduel.setTick(1234);

	const id player1(26, 0, 0, 0);
	const id player2(27, 0, 0, 0);

	const id winnerForward = qduel.state()->computeWinner(player1, player2);
	const id winnerReverse = qduel.state()->computeWinner(player2, player1);
	EXPECT_EQ(winnerForward, winnerReverse);
	EXPECT_TRUE(winnerForward == player1 || winnerForward == player2);
}

TEST(ContractQDuel, CalculateRevenueMatchesExpectedSplits)
{
	ContractTestingQDuel qduel;

	constexpr uint64 amount = 1000000ULL;
	QDUEL::CalculateRevenue_output output{};
	qduel.state()->calculateRevenue(amount, output);

	const uint64 expectedDev = (amount * qduel.state()->devFee()) / QDUEL_PERCENT_SCALE;
	const uint64 expectedBurn = (amount * qduel.state()->burnFee()) / QDUEL_PERCENT_SCALE;
	const uint64 expectedShareholders = ((amount * qduel.state()->shareholdersFee()) / QDUEL_PERCENT_SCALE) / 676ULL * 676ULL;
	const uint64 expectedWinner = amount - (expectedDev + expectedBurn + expectedShareholders);

	EXPECT_EQ(output.devFee, expectedDev);
	EXPECT_EQ(output.burnFee, expectedBurn);
	EXPECT_EQ(output.shareholdersFee, expectedShareholders);
	EXPECT_EQ(output.winner, expectedWinner);
}

TEST(ContractQDuel, SetPercentFeesAccessDeniedAndGetPercentFees)
{
	ContractTestingQDuel qduel;
	const auto before = qduel.getPercentFees();

	const id user(28, 0, 0, 0);
	increaseEnergy(user, 10);
	const uint64 balanceBefore = getBalance(user);

	EXPECT_EQ(qduel.setPercentFees(user, 1, 2, 3, 10).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(getBalance(user), balanceBefore);

	const auto after = qduel.getPercentFees();
	EXPECT_EQ(after.devFeePercentBps, before.devFeePercentBps);
	EXPECT_EQ(after.burnFeePercentBps, before.burnFeePercentBps);
	EXPECT_EQ(after.shareholdersFeePercentBps, before.shareholdersFeePercentBps);
	EXPECT_EQ(static_cast<uint32>(after.percentScale), static_cast<uint32>(before.percentScale));
}

TEST(ContractQDuel, SetPercentFeesUpdatesState)
{
	ContractTestingQDuel qduel;

	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 1, 2, 3, 1).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const auto output = qduel.getPercentFees();
	EXPECT_EQ(output.devFeePercentBps, 1);
	EXPECT_EQ(output.burnFeePercentBps, 2);
	EXPECT_EQ(output.shareholdersFeePercentBps, 3);
	EXPECT_EQ(static_cast<uint32>(output.percentScale), static_cast<uint32>(QDUEL_PERCENT_SCALE));
	EXPECT_EQ(output.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
}

TEST(ContractQDuel, SetTTLHoursAccessDeniedAndInvalid)
{
	ContractTestingQDuel qduel;
	const uint8 ttlBefore = qduel.state()->ttl();

	const id user(29, 0, 0, 0);
	increaseEnergy(user, 5);
	const uint64 balanceBefore = getBalance(user);

	EXPECT_EQ(qduel.setTtlHours(user, 5, 5).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(getBalance(user), balanceBefore);
	EXPECT_EQ(qduel.state()->ttl(), ttlBefore);

	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setTtlHours(qduel.state()->team(), 0, 1).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INVALID_VALUE));
	EXPECT_EQ(qduel.state()->ttl(), ttlBefore);
}

TEST(ContractQDuel, SetTTLHoursUpdatesState)
{
	ContractTestingQDuel qduel;
	increaseEnergy(qduel.state()->team(), 1);

	EXPECT_EQ(qduel.setTtlHours(qduel.state()->team(), 6, 1).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const auto output = qduel.getTtlHours();
	EXPECT_EQ(output.ttlHours, 6);
	EXPECT_EQ(output.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
}

TEST(ContractQDuel, GetRoomsReturnsActiveRooms)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id owner1(30, 0, 0, 0);
	const id owner2(31, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner1, stake);
	increaseEnergy(owner2, stake);

	EXPECT_EQ(qduel.createRoom(owner1, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.createRoom(owner2, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const auto output = qduel.getRooms();
	EXPECT_EQ(output.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	uint64 count = 0;
	bool foundOwner1 = false;
	bool foundOwner2 = false;
	for (uint32 i = 0; i < QDUEL_MAX_NUMBER_OF_ROOMS; ++i)
	{
		const QDUEL::RoomInfo room = output.rooms.get(i);
		if (room.roomId != id::zero())
		{
			++count;
			EXPECT_TRUE(qduel.state()->hasRoom(room.roomId));
			if (room.owner == owner1)
			{
				foundOwner1 = true;
			}
			if (room.owner == owner2)
			{
				foundOwner2 = true;
			}
		}
	}
	EXPECT_EQ(count, qduel.state()->roomCount());
	EXPECT_TRUE(foundOwner1);
	EXPECT_TRUE(foundOwner2);
}

TEST(ContractQDuel, GetUserProfileReportsUserData)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const QDUEL::GetUserProfile_output& missing = qduel.getUserProfile();
	EXPECT_EQ(missing.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::USER_NOT_FOUND));

	const id owner(32, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake + 200);

	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 2, stake * 2, stake + 200).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const auto profile = qduel.state()->getUserProfileFor(owner);
	EXPECT_EQ(profile.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(profile.depositedAmount, 200ULL);
	EXPECT_EQ(profile.locked, stake);
	EXPECT_EQ(profile.stake, stake);
	EXPECT_EQ(profile.raiseStep, 2ULL);
	EXPECT_EQ(profile.maxStake, stake * 2);
	EXPECT_NE(profile.roomId, id::zero());
}

TEST(ContractQDuel, DepositValidationsAndUpdatesBalance)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id missingUser(33, 0, 0, 0);
	increaseEnergy(missingUser, 1);
	EXPECT_EQ(qduel.deposit(missingUser, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INVALID_VALUE));

	increaseEnergy(missingUser, 100);
	const uint64 missingBefore = getBalance(missingUser);
	EXPECT_EQ(qduel.deposit(missingUser, 100).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::USER_NOT_FOUND));
	EXPECT_EQ(getBalance(missingUser), missingBefore);

	const id owner(34, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake);
	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::UserData before{};
	ASSERT_TRUE(qduel.state()->getUserData(owner, before));
	increaseEnergy(owner, 500);
	EXPECT_EQ(qduel.deposit(owner, 500).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::UserData after{};
	ASSERT_TRUE(qduel.state()->getUserData(owner, after));
	EXPECT_EQ(after.depositedAmount, before.depositedAmount + 500);
}

TEST(ContractQDuel, WithdrawValidationsAndTransfers)
{
	ContractTestingQDuel qduel;
	qduel.state()->setState(QDUEL::EState::NONE);

	const id missingUser(35, 0, 0, 0);
	increaseEnergy(missingUser, 1);
	EXPECT_EQ(qduel.withdraw(missingUser, 1).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::USER_NOT_FOUND));

	const id owner(36, 0, 0, 0);
	const sint64 stake = qduel.state()->minDuelAmount();
	increaseEnergy(owner, stake + 1000);
	EXPECT_EQ(qduel.createRoom(owner, NULL_ID, stake, 1, stake, stake + 1000).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(qduel.withdraw(owner, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INSUFFICIENT_FREE_DEPOSIT));
	EXPECT_EQ(qduel.withdraw(owner, 2000).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INSUFFICIENT_FREE_DEPOSIT));

	const uint64 balanceBefore = getBalance(owner);
	EXPECT_EQ(qduel.withdraw(owner, 500).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(getBalance(owner), balanceBefore + 500);

	QDUEL::UserData userAfter{};
	ASSERT_TRUE(qduel.state()->getUserData(owner, userAfter));
	EXPECT_EQ(userAfter.depositedAmount, 500ULL);
}

TEST(ContractQDuel, ConnectToRoomDistributesFeesPlayer1Wins)
{
	ContractTestingQDuel qduel;

	id player1;
	id player2;
	ASSERT_TRUE(findPlayersForWinner(qduel, true, player1, player2));
	runFullGameCycleWithFees(qduel, player1, player2, player1);
}

TEST(ContractQDuel, ConnectToRoomDistributesFeesPlayer2Wins)
{
	ContractTestingQDuel qduel;

	id player1;
	id player2;
	ASSERT_TRUE(findPlayersForWinner(qduel, false, player1, player2));
	runFullGameCycleWithFees(qduel, player1, player2, player2);
}
