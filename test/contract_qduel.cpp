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

class QDuelChecker : public QDUEL
{
public:
	uint64 roomCount() const { return rooms.population(); }
	id team() const { return teamAddress; }
	uint8 ttl() const { return ttlHours; }
	uint8 devFee() const { return devFeePercentBps; }
	uint8 burnFee() const { return burnFeePercentBps; }
	uint8 shareholdersFee() const { return shareholdersFeePercentBps; }
	uint64 minDuelAmount() const { return minimumDuelAmount; }
	void setState(EState newState) { currentState = newState; }
	EState getState() const { return currentState; }
	bool getUserData(const id& user, UserData& data) const { return users.get(user, data); }

	RoomInfo firstRoom() const
	{
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

		output = {};
		CalculateRevenue_input revenueInput{amount};
		CalculateRevenue_locals revenueLocals{};
		CalculateRevenue(qpi, *this, revenueInput, output, revenueLocals);
	}
};

class ContractTestingQDuel : protected ContractTesting
{
public:
	ContractTestingQDuel()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QDUEL);
		system.epoch = contractDescriptions[QDUEL_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(QDUEL_CONTRACT_INDEX, INITIALIZE);
	}

	QDuelChecker* state() { return reinterpret_cast<QDuelChecker*>(contractStates[QDUEL_CONTRACT_INDEX]); }

	QDUEL::CreateRoom_output createRoom(const id& user, const id& allowedPlayer, uint64 stake, uint64 raiseStep, uint64 maxStake, sint64 reward)
	{
		QDUEL::CreateRoom_input input{allowedPlayer, stake, raiseStep, maxStake};
		QDUEL::CreateRoom_output output;
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
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_PERCENT_FEES, input, output);
		return output;
	}

	QDUEL::GetRooms_output getRooms()
	{
		QDUEL::GetRooms_input input{};
		QDUEL::GetRooms_output output;
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_ROOMS, input, output);
		return output;
	}

	QDUEL::GetTTLHours_output getTtlHours()
	{
		QDUEL::GetTTLHours_input input{};
		QDUEL::GetTTLHours_output output;
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_TTL_HOURS, input, output);
		return output;
	}

	QDUEL::GetUserProfile_output getUserProfile()
	{
		QDUEL::GetUserProfile_input input{};
		QDUEL::GetUserProfile_output output;
		callFunction(QDUEL_CONTRACT_INDEX, FUNCTION_INDEX_GET_USER_PROFILE, input, output);
		return output;
	}

	QDUEL::Deposit_output deposit(const id& user, sint64 reward)
	{
		QDUEL::Deposit_input input{};
		QDUEL::Deposit_output output;
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_DEPOSIT, input, output, user, reward))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	QDUEL::Withdraw_output withdraw(const id& user, uint64 amount)
	{
		QDUEL::Withdraw_input input{amount};
		QDUEL::Withdraw_output output;
		if (!invokeUserProcedure(QDUEL_CONTRACT_INDEX, PROCEDURE_INDEX_WITHDRAW, input, output, user, 0))
		{
			output.returnCode = QDUEL::toReturnCode(QDUEL::EReturnCode::UNKNOWN_ERROR);
		}
		return output;
	}

	void endTick() { callSystemProcedure(QDUEL_CONTRACT_INDEX, END_TICK); }

	void endEpoch() { callSystemProcedure(QDUEL_CONTRACT_INDEX, END_EPOCH); }

	void forceEndTick()
	{
		system.tick = system.tick + (QDUEL_TICK_UPDATE_PERIOD - mod(system.tick, static_cast<uint32>(QDUEL_TICK_UPDATE_PERIOD)));

		endTick();
	}

	void setDeterministicTime(uint16 year = 2025, uint8 month = 1, uint8 day = 1, uint8 hour = 0)
	{
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
		// Setup shareholders
		const id shareholder1 = id::randomValue();
		const id shareholder2 = id::randomValue();
		constexpr unsigned int rlSharesOwner1 = 100;
		constexpr unsigned int rlSharesOwner2 = 576;
		std::vector<std::pair<m256i, unsigned int>> rlShares{
		    {shareholder1, rlSharesOwner1},
		    {shareholder2, rlSharesOwner2},
		};
		issueContractShares(RL_CONTRACT_INDEX, rlShares);

		// Set fees
		constexpr uint8 devFee = 15;
		constexpr uint8 burnFee = 30;
		constexpr uint8 shareholdersFee = 55;
		increaseEnergy(qduel.state()->team(), 1);
		EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), devFee, burnFee, shareholdersFee).returnCode,
		          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

		// Setup: Give players initial balance
		constexpr uint64 duelAmount = 100000ULL;
		increaseEnergy(player1, duelAmount);
		increaseEnergy(player2, duelAmount);
		const uint64 player1Before = getBalance(player1);
		const uint64 player2Before = getBalance(player2);

		const uint64 teamBefore = getBalance(qduel.state()->team());
		const uint64 shareholder1Before = getBalance(shareholder1);
		const uint64 shareholder2Before = getBalance(shareholder2);

		// Create room and play
		EXPECT_EQ(qduel.createRoom(player1, NULL_ID, duelAmount, 1, duelAmount, duelAmount).returnCode,
		          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
		const uint64 player1AfterCreateRoom = getBalance(player1);


		const id winner = qduel.state()->computeWinner(player1, player2);
		EXPECT_EQ(winner, expectedWinner);

		// Calculate expected revenue distribution
		QDUEL::CalculateRevenue_output revenueOutput{};
		qduel.state()->calculateRevenue(duelAmount * 2, revenueOutput);

		// Player 2 joins
		EXPECT_EQ(qduel.connectToRoom(player2, qduel.state()->firstRoom().roomId, duelAmount).returnCode,
		          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
		const uint64 player2AfterConnectToRoom = getBalance(player2);


		// Check fee distribution
		EXPECT_EQ(getBalance(qduel.state()->team()), teamBefore + revenueOutput.devFee);

		// Check shareholder dividends
		const uint64 dividendPerShare = revenueOutput.shareholdersFee / NUMBER_OF_COMPUTORS;
		EXPECT_EQ(getBalance(shareholder1), shareholder1Before + dividendPerShare * rlSharesOwner1);
		EXPECT_EQ(getBalance(shareholder2), shareholder2Before + dividendPerShare * rlSharesOwner2);

		// Check winner gets correct amount
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

TEST(ContractQDuel, InitializeDefaults)
{
	ContractTestingQDuel qduel;

	EXPECT_EQ(qduel.state()->team(), QDUEL_TEAM_ADDRESS);
	EXPECT_EQ(qduel.state()->devFee(), QDUEL_DEV_FEE_PERCENT_BPS);
	EXPECT_EQ(qduel.state()->burnFee(), QDUEL_BURN_FEE_PERCENT_BPS);
	EXPECT_EQ(qduel.state()->shareholdersFee(), QDUEL_SHAREHOLDERS_FEE_PERCENT_BPS);
	EXPECT_EQ(qduel.state()->ttl(), QDUEL_TTL_HOURS);
	EXPECT_EQ(qduel.state()->minDuelAmount(), QDUEL_MINIMUM_DUEL_AMOUNT);
}

TEST(ContractQDuel, CreateRoomRejectsInsufficientAmount)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT);

	const uint64 balanceBefore = getBalance(host);
	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT - 1);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT));
	EXPECT_EQ(getBalance(host), balanceBefore);
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);
}

TEST(ContractQDuel, CreateRoomBlockedWhenLocked)
{
	ContractTestingQDuel qduel;

	const id host = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT);
	qduel.state()->setState(QDUEL::EState::LOCKED);

	const uint64 balanceBefore = getBalance(host);
	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::STATE_LOCKED));
	EXPECT_EQ(getBalance(host), balanceBefore);
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);
}

TEST(ContractQDuel, CreateRoomLockedUntilValidTime)
{
	ContractTestingQDuel qduel;

	// Set default time
	qduel.setDeterministicTime(2022, 4, 13);
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id joiner = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);
	increaseEnergy(joiner, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	EXPECT_EQ(qduel.state()->getState() & QDUEL::EState::WAIT_TIME, QDUEL::EState::WAIT_TIME);

	const uint64 hostBalance = getBalance(host);
	const QDUEL::CreateRoom_output& lockedOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(lockedOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::STATE_LOCKED));
	EXPECT_EQ(getBalance(host), hostBalance);
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);

	qduel.setDeterministicTime(2025, 5, 1, 0);
	qduel.forceEndTick();
	EXPECT_EQ(qduel.state()->getState() & QDUEL::EState::WAIT_TIME, QDUEL::EState::NONE);

	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.state()->roomCount(), 1ull);

	const QDUEL::ConnectToRoom_output& connectOut = qduel.connectToRoom(joiner, qduel.state()->firstRoom().roomId, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(connectOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
}

TEST(ContractQDuel, CreateRoomListedInGetRooms)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id allowed = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	constexpr sint64 reward = QDUEL_MINIMUM_DUEL_AMOUNT;
	const uint64 balanceBefore = getBalance(host);
	const QDUEL::CreateRoom_output& createOut = qduel.createRoom(host, allowed, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, reward);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::RoomInfo room = qduel.state()->firstRoom();
	EXPECT_EQ(room.owner, host);
	EXPECT_EQ(room.allowedPlayer, allowed);
	EXPECT_EQ(room.amount, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(getBalance(host), balanceBefore - QDUEL_MINIMUM_DUEL_AMOUNT);

	const QDUEL::GetRooms_output& roomsOut = qduel.getRooms();
	EXPECT_EQ(roomsOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_NE(roomsOut.rooms.get(0).roomId, NULL_ID);

	uint64 found = 0;
	for (uint64 i = 0; i < roomsOut.rooms.capacity(); ++i)
	{
		const QDUEL::RoomInfo& listed = roomsOut.rooms.get(i);
		if (listed.roomId == NULL_ID)
		{
			continue;
		}
		EXPECT_EQ(listed.roomId, room.roomId);
		EXPECT_EQ(listed.owner, room.owner);
		EXPECT_EQ(listed.allowedPlayer, room.allowedPlayer);
		EXPECT_EQ(listed.amount, room.amount);
		++found;
	}
	EXPECT_EQ(found, 1ull);
}

TEST(ContractQDuel, SetPercentFeesAccessDeniedAndSuccess)
{
	ContractTestingQDuel qduel;

	const id attacker = id::randomValue();
	increaseEnergy(attacker, 1);

	const uint64 balanceBefore = getBalance(attacker);
	const QDUEL::SetPercentFees_output& denied = qduel.setPercentFees(attacker, 1, 1, 1, 1);
	EXPECT_EQ(denied.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(getBalance(attacker), balanceBefore);

	increaseEnergy(qduel.state()->team(), 1);
	const QDUEL::SetPercentFees_output& applied = qduel.setPercentFees(qduel.state()->team(), 2, 3, 4);
	EXPECT_EQ(applied.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::GetPercentFees_output& fees = qduel.getPercentFees();
	EXPECT_EQ(fees.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(fees.devFeePercentBps, 2);
	EXPECT_EQ(fees.burnFeePercentBps, 3);
	EXPECT_EQ(fees.shareholdersFeePercentBps, 4);
	EXPECT_EQ(fees.percentScale, QDUEL_PERCENT_SCALE);
}

TEST(ContractQDuel, SetTTLHoursAccessDeniedInvalidAndSuccess)
{
	ContractTestingQDuel qduel;

	const id attacker = id::randomValue();
	increaseEnergy(attacker, 1);

	const uint64 balanceBefore = getBalance(attacker);
	const QDUEL::SetTTLHours_output& denied = qduel.setTtlHours(attacker, 1, 1);
	EXPECT_EQ(denied.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(getBalance(attacker), balanceBefore);

	increaseEnergy(qduel.state()->team(), 1);
	const QDUEL::SetTTLHours_output& invalid = qduel.setTtlHours(qduel.state()->team(), 0);
	EXPECT_EQ(invalid.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INVALID_VALUE));

	const QDUEL::SetTTLHours_output& applied = qduel.setTtlHours(qduel.state()->team(), 5);
	EXPECT_EQ(applied.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.getTtlHours().ttlHours, 5);
}

TEST(ContractQDuel, ConnectToRoomRejectsInvalidRequests)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id intruder = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);
	increaseEnergy(intruder, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, host, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const id& roomId = qduel.state()->firstRoom().roomId;

	const uint64 intruderBalance = getBalance(intruder);
	const QDUEL::ConnectToRoom_output& denied = qduel.connectToRoom(intruder, roomId, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(denied.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_ACCESS_DENIED));
	EXPECT_EQ(getBalance(intruder), intruderBalance);

	const QDUEL::ConnectToRoom_output& notFound = qduel.connectToRoom(host, id::randomValue(), QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(notFound.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_NOT_FOUND));

	const uint64 hostBalance = getBalance(host);
	const QDUEL::ConnectToRoom_output& insufficient = qduel.connectToRoom(host, roomId, QDUEL_MINIMUM_DUEL_AMOUNT - 1);
	EXPECT_EQ(insufficient.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT));
	EXPECT_EQ(getBalance(host), hostBalance);
}

TEST(ContractQDuel, ConnectToRoomBlockedWhenLocked)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id joiner = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);
	increaseEnergy(joiner, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	EXPECT_EQ(qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const id& roomId = qduel.state()->firstRoom().roomId;
	qduel.state()->setState(QDUEL::EState::LOCKED);

	const uint64 joinerBalance = getBalance(joiner);
	const QDUEL::ConnectToRoom_output& connectOut = qduel.connectToRoom(joiner, roomId, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(connectOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::STATE_LOCKED));
	EXPECT_EQ(getBalance(joiner), joinerBalance);
	EXPECT_TRUE(qduel.state()->hasRoom(roomId));
}

TEST(ContractQDuel, ConnectToRoomSuccessPaysWinner)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id joiner = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);
	increaseEnergy(joiner, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo& room = qduel.state()->firstRoom();

	const id winner = qduel.state()->computeWinner(host, joiner);

	QDUEL::CalculateRevenue_output revenueOutput{};
	qduel.state()->calculateRevenue(QDUEL_MINIMUM_DUEL_AMOUNT * 2, revenueOutput);

	const uint64 hostBefore = getBalance(host);
	const uint64 joinerBefore = getBalance(joiner);
	const QDUEL::ConnectToRoom_output& connectOut = qduel.connectToRoom(joiner, room.roomId, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(connectOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_TRUE(winner == host || winner == joiner);

	const uint64 hostAfter = getBalance(host);
	const uint64 joinerAfter = getBalance(joiner);
	if (winner == host)
	{
		EXPECT_EQ(hostAfter, hostBefore + revenueOutput.winner);
		EXPECT_EQ(joinerAfter, joinerBefore - QDUEL_MINIMUM_DUEL_AMOUNT);
	}
	else
	{
		EXPECT_EQ(joinerAfter, joinerBefore - QDUEL_MINIMUM_DUEL_AMOUNT + revenueOutput.winner);
		EXPECT_EQ(hostAfter, hostBefore);
	}
}

TEST(ContractQDuel, ConnectToRoomRefundsOverpayment)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id joiner = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);
	increaseEnergy(joiner, QDUEL_MINIMUM_DUEL_AMOUNT * 3);

	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	const QDUEL::RoomInfo& room = qduel.state()->firstRoom();

	const id& winner = qduel.state()->computeWinner(host, joiner);

	QDUEL::CalculateRevenue_output revenueOutput{};
	qduel.state()->calculateRevenue(QDUEL_MINIMUM_DUEL_AMOUNT * 2, revenueOutput);

	const uint64 joinerBefore = getBalance(joiner);
	const uint64 hostBefore = getBalance(host);
	constexpr uint64 overpay = QDUEL_MINIMUM_DUEL_AMOUNT + 123;
	const QDUEL::ConnectToRoom_output& connectOut = qduel.connectToRoom(joiner, room.roomId, overpay);
	EXPECT_EQ(connectOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const uint64 joinerAfter = getBalance(joiner);
	const uint64 hostAfter = getBalance(host);
	if (winner == joiner)
	{
		EXPECT_EQ(joinerAfter, joinerBefore - QDUEL_MINIMUM_DUEL_AMOUNT + revenueOutput.winner);
		EXPECT_EQ(hostAfter, hostBefore);
	}
	else
	{
		EXPECT_EQ(joinerAfter, joinerBefore - QDUEL_MINIMUM_DUEL_AMOUNT);
		EXPECT_EQ(hostAfter, hostBefore + revenueOutput.winner);
	}
}

TEST(ContractQDuel, ConnectToRoomPaysRLDividendsToShareholders)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id shareholder1 = id::randomValue();
	const id shareholder2 = id::randomValue();
	const id shareholder3 = id::randomValue();
	constexpr unsigned int rlSharesOwner1 = 100;
	constexpr unsigned int rlSharesOwner2 = 200;
	constexpr unsigned int rlSharesOwner3 = 376;
	std::vector<std::pair<m256i, unsigned int>> rlShares{
	    {shareholder1, rlSharesOwner1},
	    {shareholder2, rlSharesOwner2},
	    {shareholder3, rlSharesOwner3},
	};
	issueContractShares(RL_CONTRACT_INDEX, rlShares);

	constexpr uint8 shareholdersFeePercentBps = 10;
	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, shareholdersFeePercentBps).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const id host = id::randomValue();
	const id joiner = id::randomValue();
	constexpr uint64 duelAmount = 67600ULL;
	increaseEnergy(host, duelAmount);
	increaseEnergy(joiner, duelAmount);

	EXPECT_EQ(qduel.createRoom(host, NULL_ID, duelAmount, 1, duelAmount, duelAmount).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::CalculateRevenue_output revenueOutput{};
	constexpr uint64 duelPayoutAmount = duelAmount * 2;
	qduel.state()->calculateRevenue(duelPayoutAmount, revenueOutput);
	const uint64 dividendPerShare = revenueOutput.shareholdersFee / NUMBER_OF_COMPUTORS;

	const uint64 shareholder1Before = getBalance(shareholder1);
	const uint64 shareholder2Before = getBalance(shareholder2);
	const uint64 shareholder3Before = getBalance(shareholder3);

	EXPECT_EQ(qduel.connectToRoom(joiner, qduel.state()->firstRoom().roomId, duelAmount).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(getBalance(shareholder1), shareholder1Before + dividendPerShare * rlShares[0].second);
	EXPECT_EQ(getBalance(shareholder2), shareholder2Before + dividendPerShare * rlShares[1].second);
	EXPECT_EQ(getBalance(shareholder3), shareholder3Before + dividendPerShare * rlShares[2].second);
}

TEST(ContractQDuel, EndTickRefundsOnlyAfterTtl)
{
	ContractTestingQDuel qduel;

	const id host = id::randomValue();
	increaseEnergy(host, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const uint64 balanceBefore = getBalance(host);
	const QDUEL::CreateRoom_output& createOut =
	    qduel.createRoom(host, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.state()->roomCount(), 1ull);
	EXPECT_EQ(getBalance(host), balanceBefore - QDUEL_MINIMUM_DUEL_AMOUNT);

	qduel.forceEndTick();
	EXPECT_EQ(qduel.state()->roomCount(), 1ull);
	EXPECT_EQ(getBalance(host), balanceBefore - QDUEL_MINIMUM_DUEL_AMOUNT);

	qduel.setDeterministicTime(2025, 1, 1, QDUEL_TTL_HOURS + 1);
	qduel.forceEndTick();
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);
	EXPECT_EQ(getBalance(host), balanceBefore);
}

TEST(ContractQDuel, EndEpochRefundsAllRooms)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host1 = id::randomValue();
	const id host2 = id::randomValue();
	increaseEnergy(host1, QDUEL_MINIMUM_DUEL_AMOUNT * 2);
	increaseEnergy(host2, QDUEL_MINIMUM_DUEL_AMOUNT * 2);

	const uint64 host1Before = getBalance(host1);
	const uint64 host2Before = getBalance(host2);
	EXPECT_EQ(qduel.createRoom(host1, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.createRoom(host2, NULL_ID, QDUEL_MINIMUM_DUEL_AMOUNT, 1, QDUEL_MINIMUM_DUEL_AMOUNT, QDUEL_MINIMUM_DUEL_AMOUNT).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.state()->roomCount(), 2ull);

	qduel.endEpoch();
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);
	EXPECT_EQ(getBalance(host1), host1Before);
	EXPECT_EQ(getBalance(host2), host2Before);
}

TEST(ContractQDuel, DepositWithdrawUpdatesProfile)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id host = id::randomValue();
	const id joiner = id::randomValue();

	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	constexpr uint64 depositAmount = stake * 2;
	increaseEnergy(host, depositAmount);
	increaseEnergy(joiner, stake);

	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(qduel.createRoom(host, NULL_ID, stake, 2, stake * 10, depositAmount).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.connectToRoom(joiner, qduel.state()->firstRoom().roomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(qduel.state()->roomCount(), 0ull);
	QDUEL::UserData userData{};
	EXPECT_FALSE(qduel.state()->getUserData(host, userData));
}

TEST(ContractQDuel, PrivateFunctionGetWinnerPlayerDeterministic)
{
	ContractTestingQDuel qduel;

	const id player1 = id::randomValue();
	const id player2 = id::randomValue();
	const id winner1 = qduel.state()->computeWinner(player1, player2);
	const id winner2 = qduel.state()->computeWinner(player1, player2);
	EXPECT_EQ(winner1, winner2);
	EXPECT_TRUE(winner1 == player1 || winner1 == player2);
}

TEST(ContractQDuel, PrivateFunctionCalculateRevenueComputesFees)
{
	ContractTestingQDuel qduel;

	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 2, 3, 4).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::CalculateRevenue_output revenueOutput{};
	qduel.state()->calculateRevenue(1000000ull, revenueOutput);

	constexpr uint64 expectedDev = 1000000ull * 2 / QDUEL_PERCENT_SCALE;
	constexpr uint64 expectedBurn = 1000000ull * 3 / QDUEL_PERCENT_SCALE;
	constexpr uint64 expectedShare = 1000000ull * 4 / QDUEL_PERCENT_SCALE / 676ULL * 676ULL;
	constexpr uint64 expectedWinner = 1000000ull - (expectedDev + expectedBurn + expectedShare);

	EXPECT_EQ(revenueOutput.devFee, expectedDev);
	EXPECT_EQ(revenueOutput.burnFee, expectedBurn);
	EXPECT_EQ(revenueOutput.shareholdersFee, expectedShare);
	EXPECT_EQ(revenueOutput.winner, expectedWinner);
}

// ============================================================================
// FULL GAME CYCLE TESTS
// ============================================================================

TEST(ContractQDuel, FullGameCycle_SimpleGame)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player1 = id::randomValue();
	const id player2 = id::randomValue();

	// Setup: Give players initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 10;
	increaseEnergy(player1, initialBalance);
	increaseEnergy(player2, initialBalance);

	// Disable fees for simplicity
	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Player 1 creates a room
	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	const uint64 player1BalanceBefore = getBalance(player1);
	const QDUEL::CreateRoom_output& createOut = qduel.createRoom(player1, NULL_ID, stake, 1, stake, stake);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(getBalance(player1), player1BalanceBefore - stake);
	EXPECT_EQ(qduel.state()->roomCount(), 1ull);

	// Get room info
	const QDUEL::RoomInfo& room = qduel.state()->firstRoom();
	EXPECT_EQ(room.owner, player1);
	EXPECT_EQ(room.amount, stake);
	EXPECT_EQ(room.allowedPlayer, NULL_ID);

	// Player 2 connects to the room
	const id winner = qduel.state()->computeWinner(player1, player2);
	const uint64 player2BalanceBefore = getBalance(player2);
	const QDUEL::ConnectToRoom_output& connectOut = qduel.connectToRoom(player2, room.roomId, stake);
	EXPECT_EQ(connectOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Room should be removed after the game
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);

	// Winner should receive the total stake
	const uint64 expectedWinnings = stake * 2;
	if (winner == player1)
	{
		EXPECT_EQ(getBalance(player1), player1BalanceBefore + stake); // lost stake but won total
		EXPECT_EQ(getBalance(player2), player2BalanceBefore - stake);
	}
	else
	{
		EXPECT_EQ(getBalance(player1), player1BalanceBefore - stake);
		EXPECT_EQ(getBalance(player2), player2BalanceBefore + stake); // lost stake but won total
	}
}

TEST(ContractQDuel, FullGameCycle_WithDeposits)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player = id::randomValue();

	// Setup: Give player initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 100;
	increaseEnergy(player, initialBalance);

	// Disable fees for simplicity
	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Player creates a room with extra deposit
	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	constexpr uint64 extraDeposit = stake * 3;
	constexpr uint64 totalReward = stake + extraDeposit;

	const uint64 playerBalanceBefore = getBalance(player);
	const QDUEL::CreateRoom_output& createOut = qduel.createRoom(player, NULL_ID, stake, 1, stake, totalReward);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Check user profile
	QDUEL::UserData userData{};
	EXPECT_TRUE(qduel.state()->getUserData(player, userData));
	EXPECT_EQ(userData.depositedAmount, extraDeposit);
	EXPECT_EQ(userData.locked, stake);
	EXPECT_EQ(userData.stake, stake);

	// Player's balance should be reduced by total reward
	EXPECT_EQ(getBalance(player), playerBalanceBefore - totalReward);

	// Additional deposit to existing user
	constexpr uint64 additionalDeposit = stake * 2;
	increaseEnergy(player, additionalDeposit);
	const QDUEL::Deposit_output& depositOut = qduel.deposit(player, additionalDeposit);
	EXPECT_EQ(depositOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Check updated user profile
	EXPECT_TRUE(qduel.state()->getUserData(player, userData));
	EXPECT_EQ(userData.depositedAmount, extraDeposit + additionalDeposit);
	EXPECT_EQ(userData.locked, stake);

	// Withdraw some funds
	constexpr uint64 withdrawAmount = stake;
	const uint64 balanceBeforeWithdraw = getBalance(player);
	const QDUEL::Withdraw_output& withdrawOut = qduel.withdraw(player, withdrawAmount);
	EXPECT_EQ(withdrawOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(getBalance(player), balanceBeforeWithdraw + withdrawAmount);

	// Check updated user profile
	EXPECT_TRUE(qduel.state()->getUserData(player, userData));
	EXPECT_EQ(userData.depositedAmount, extraDeposit + additionalDeposit - withdrawAmount);

	// Room should still exist
	EXPECT_EQ(qduel.state()->roomCount(), 1ull);
}

TEST(ContractQDuel, FullGameCycle_WithdrawInsufficientFunds)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player = id::randomValue();

	// Setup: Give player initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 10;
	increaseEnergy(player, initialBalance);

	// Player creates a room with extra deposit
	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	constexpr uint64 extraDeposit = stake * 2;
	constexpr uint64 totalReward = stake + extraDeposit;

	const QDUEL::CreateRoom_output& createOut = qduel.createRoom(player, NULL_ID, stake, 1, stake, totalReward);
	EXPECT_EQ(createOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	QDUEL::UserData userData{};
	EXPECT_TRUE(qduel.state()->getUserData(player, userData));
	EXPECT_EQ(userData.depositedAmount, extraDeposit);
	EXPECT_EQ(userData.locked, stake);

	// Try to withdraw more than deposited (should fail)
	const uint64 withdrawAmount = extraDeposit + 1;
	const uint64 balanceBeforeWithdraw = getBalance(player);
	const QDUEL::Withdraw_output& withdrawOut = qduel.withdraw(player, withdrawAmount);
	EXPECT_EQ(withdrawOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INSUFFICIENT_FREE_DEPOSIT));
	EXPECT_EQ(getBalance(player), balanceBeforeWithdraw);

	// User data should remain unchanged
	EXPECT_TRUE(qduel.state()->getUserData(player, userData));
	EXPECT_EQ(userData.depositedAmount, extraDeposit);

	// Try to withdraw zero (should fail)
	const QDUEL::Withdraw_output& withdrawZeroOut = qduel.withdraw(player, 0);
	EXPECT_EQ(withdrawZeroOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::INSUFFICIENT_FREE_DEPOSIT));
}

TEST(ContractQDuel, FullGameCycle_MultipleRounds)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player1 = id::randomValue();
	const id player2 = id::randomValue();

	// Setup: Give players initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 100;
	increaseEnergy(player1, initialBalance);
	increaseEnergy(player2, initialBalance);

	// Disable fees for simplicity
	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Round 1: Player 1 creates room, Player 2 joins
	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	EXPECT_EQ(qduel.createRoom(player1, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	id roomId1 = qduel.state()->firstRoom().roomId;
	EXPECT_EQ(qduel.connectToRoom(player2, roomId1, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);

	// Round 2: Player 2 creates room, Player 1 joins
	EXPECT_EQ(qduel.createRoom(player2, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	id roomId2 = qduel.state()->firstRoom().roomId;
	EXPECT_EQ(qduel.connectToRoom(player1, roomId2, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);

	// Round 3: Player 1 creates room, Player 2 joins
	EXPECT_EQ(qduel.createRoom(player1, NULL_ID, stake, 1, stake, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	id roomId3 = qduel.state()->firstRoom().roomId;
	EXPECT_EQ(qduel.connectToRoom(player2, roomId3, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);

	// Both players should have balance changes from wins/losses
	const uint64 player1Final = getBalance(player1);
	const uint64 player2Final = getBalance(player2);

	// Total balance should be conserved (no fees)
	EXPECT_EQ(player1Final + player2Final, initialBalance * 2);
}

TEST(ContractQDuel, FullGameCycle_AutoRoomCreationAfterWin)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player1 = id::randomValue();
	const id player2 = id::randomValue();

	// Setup: Give players initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 100;
	increaseEnergy(player1, initialBalance);
	increaseEnergy(player2, initialBalance);

	// Disable fees for simplicity
	increaseEnergy(qduel.state()->team(), 1);
	EXPECT_EQ(qduel.setPercentFees(qduel.state()->team(), 0, 0, 0).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Player 1 creates room with raiseStep=2 and extra deposit
	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	constexpr uint64 raiseStep = 2;
	constexpr uint64 maxStake = stake * 8;
	constexpr uint64 extraDeposit = stake * 10;
	constexpr uint64 totalReward = stake + extraDeposit;

	EXPECT_EQ(qduel.createRoom(player1, NULL_ID, stake, raiseStep, maxStake, totalReward).returnCode,
	          QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	const id roomId = qduel.state()->firstRoom().roomId;
	const id winner = qduel.state()->computeWinner(player1, player2);

	// Player 2 joins and completes the game
	EXPECT_EQ(qduel.connectToRoom(player2, roomId, stake).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	// Check if winner is player1 and has enough deposit for next round
	if (winner == player1)
	{
		// Player 1 won, should have new room created automatically with doubled stake
		EXPECT_EQ(qduel.state()->roomCount(), 1ull);

		const QDUEL::RoomInfo& newRoom = qduel.state()->firstRoom();
		EXPECT_EQ(newRoom.owner, player1);
		EXPECT_EQ(newRoom.amount, stake * raiseStep); // stake doubled

		// Check user data
		QDUEL::UserData userData{};
		EXPECT_TRUE(qduel.state()->getUserData(player1, userData));
		EXPECT_EQ(userData.locked, stake * raiseStep);
		EXPECT_EQ(userData.stake, stake * raiseStep);
		EXPECT_GT(userData.depositedAmount, 0ull); // Should have remaining deposit
	}
	else
	{
		// Player 1 lost, check if removed or new room created based on remaining deposit
		QDUEL::UserData userData{};
		bool hasUserData = qduel.state()->getUserData(player1, userData);
		// Player should be removed if insufficient funds for next round
		if (!hasUserData)
		{
			EXPECT_EQ(qduel.state()->roomCount(), 0ull);
		}
	}
}

TEST(ContractQDuel, FullGameCycle_EndEpochWithDeposits)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player1 = id::randomValue();
	const id player2 = id::randomValue();
	const id player3 = id::randomValue();

	// Setup: Give players initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 10;
	increaseEnergy(player1, initialBalance);
	increaseEnergy(player2, initialBalance);
	increaseEnergy(player3, initialBalance);

	// Create multiple rooms with deposits
	constexpr uint64 stake = QDUEL_MINIMUM_DUEL_AMOUNT;
	constexpr uint64 extraDeposit = stake * 2;

	const uint64 player1Before = getBalance(player1);
	const uint64 player2Before = getBalance(player2);
	const uint64 player3Before = getBalance(player3);

	EXPECT_EQ(qduel.createRoom(player1, NULL_ID, stake, 1, stake, stake + extraDeposit).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.createRoom(player2, NULL_ID, stake, 1, stake, stake + extraDeposit).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));
	EXPECT_EQ(qduel.createRoom(player3, NULL_ID, stake, 1, stake, stake + extraDeposit).returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::SUCCESS));

	EXPECT_EQ(qduel.state()->roomCount(), 3ull);

	// Verify user data exists for all players
	QDUEL::UserData userData{};
	EXPECT_TRUE(qduel.state()->getUserData(player1, userData));
	EXPECT_EQ(userData.depositedAmount, extraDeposit);
	EXPECT_EQ(userData.locked, stake);

	// End epoch - should refund all deposits and locked amounts
	qduel.endEpoch();

	// All rooms should be removed
	EXPECT_EQ(qduel.state()->roomCount(), 0ull);

	// All users should be removed
	EXPECT_FALSE(qduel.state()->getUserData(player1, userData));
	EXPECT_FALSE(qduel.state()->getUserData(player2, userData));
	EXPECT_FALSE(qduel.state()->getUserData(player3, userData));

	// All players should get their full deposits back
	EXPECT_EQ(getBalance(player1), player1Before);
	EXPECT_EQ(getBalance(player2), player2Before);
	EXPECT_EQ(getBalance(player3), player3Before);
}

TEST(ContractQDuel, FullGameCycle_WithFees_WinnerPlayer1)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	id player1{};
	id player2{};
	ASSERT_TRUE(findPlayersForWinner(qduel, true, player1, player2));
	runFullGameCycleWithFees(qduel, player1, player2, player1);
}

TEST(ContractQDuel, FullGameCycle_WithFees_WinnerPlayer2)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	id player1{};
	id player2{};
	ASSERT_TRUE(findPlayersForWinner(qduel, false, player1, player2));
	runFullGameCycleWithFees(qduel, player1, player2, player2);
}

TEST(ContractQDuel, FullGameCycle_DepositWithoutUser)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player = id::randomValue();

	// Setup: Give player initial balance
	constexpr uint64 initialBalance = QDUEL_MINIMUM_DUEL_AMOUNT * 10;
	increaseEnergy(player, initialBalance);

	// Try to deposit without creating a user first
	const uint64 playerBefore = getBalance(player);
	const QDUEL::Deposit_output& depositOut = qduel.deposit(player, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(depositOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::USER_NOT_FOUND));

	// Balance should be refunded
	EXPECT_EQ(getBalance(player), playerBefore);
}

TEST(ContractQDuel, FullGameCycle_WithdrawWithoutUser)
{
	ContractTestingQDuel qduel;

	qduel.setDeterministicTime();
	qduel.forceEndTick();

	const id player = id::randomValue();
	increaseEnergy(player, 1);

	// Try to withdraw without creating a user first
	const QDUEL::Withdraw_output& withdrawOut = qduel.withdraw(player, QDUEL_MINIMUM_DUEL_AMOUNT);
	EXPECT_EQ(withdrawOut.returnCode, QDUEL::toReturnCode(QDUEL::EReturnCode::USER_NOT_FOUND));
}
