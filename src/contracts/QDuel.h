using namespace QPI;

constexpr uint16 QDUEL_MAX_NUMBER_OF_ROOMS = 512;
constexpr uint16 QDUEL_MINIMUM_DUEL_AMOUNT = 10000;
constexpr uint8 QDUEL_DEV_FEE_PERCENT_BPS = 15;          // 0.15% * QDUEL_PERCENT_SCALE
constexpr uint8 QDUEL_BURN_FEE_PERCENT_BPS = 30;         // 0.3% * QDUEL_PERCENT_SCALE
constexpr uint8 QDUEL_SHAREHOLDERS_FEE_PERCENT_BPS = 55; // 0.55% * QDUEL_PERCENT_SCALE
constexpr uint16 QDUEL_PERCENT_SCALE = 1000;
constexpr uint8 QDUEL_TTL_HOURS = 3;
constexpr uint8 QDUEL_TICK_UPDATE_PERIOD = 100;           // Process TICK logic once per this many ticks
constexpr uint16 QDUEL_RANDOM_LOTTERY_ASSET_NAME = 19538; // RL
constexpr uint16 QDUEL_ROOMS_REMOVAL_THRESHOLD_PERCENT = 75;

struct QDUEL2
{
};

struct QDUEL : public ContractBase
{
public:
	enum class EState : uint8
	{
		NONE = 0,
		WAIT_TIME = 1 << 0,

		LOCKED = WAIT_TIME
	};

	friend EState operator|(const EState& a, const EState& b) { return static_cast<EState>(static_cast<uint8>(a) | static_cast<uint8>(b)); }
	friend EState operator&(const EState& a, const EState& b) { return static_cast<EState>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
	friend EState operator~(const EState& a) { return static_cast<EState>(~static_cast<uint8>(a)); }
	template<typename T> friend bool operator==(const EState& a, const T& b) { return static_cast<uint8>(a) == b; }
	template<typename T> friend bool operator!=(const EState& a, const T& b) { return !(a == b); }

	static EState removeStateFlag(EState state, EState flag) { return state & ~flag; }
	static EState addStateFlag(EState state, EState flag) { return state | flag; }

	enum class EReturnCode : uint8
	{
		SUCCESS,
		ACCESS_DENIED,
		INVALID_VALUE,
		USER_ALREADY_EXISTS,
		USER_NOT_FOUND,
		INSUFFICIENT_FREE_DEPOSIT,

		// Room
		ROOM_INSUFFICIENT_DUEL_AMOUNT,
		ROOM_NOT_FOUND,
		ROOM_FULL,
		ROOM_FAILED_CREATE,
		ROOM_FAILED_GET_WINNER,
		ROOM_ACCESS_DENIED,
		ROOM_FAILED_CALCULATE_REVENUE,

		STATE_LOCKED,

		UNKNOWN_ERROR = UINT8_MAX
	};

	static constexpr uint8 toReturnCode(EReturnCode code) { return static_cast<uint8>(code); }

	struct RoomInfo
	{
		id roomId;
		id owner;
		id allowedPlayer; // If zero, anyone can join
		sint64 amount;
		uint64 closeTimer;
		DateAndTime lastUpdate;
	};

	struct UserData
	{
		id userId;
		id roomId;
		id allowedPlayer;
		sint64 depositedAmount;
		sint64 locked;
		sint64 stake;
		sint64 raiseStep;
		sint64 maxStake;
	};

	struct AddUserData_input
	{
		id userId;
		id roomId;
		id allowedPlayer;
		sint64 depositedAmount;
		sint64 stake;
		sint64 raiseStep;
		sint64 maxStake;
	};

	struct AddUserData_output
	{
		uint8 returnCode;
	};

	struct AddUserData_locals
	{
		UserData newUserData;
	};

	struct CreateRoom_input
	{
		id allowedPlayer; // If zero, anyone can join
		sint64 stake;
		sint64 raiseStep;
		sint64 maxStake;
	};

	struct CreateRoom_output
	{
		uint8 returnCode;
	};

	struct CreateRoomRecord_input
	{
		id owner;
		id allowedPlayer;
		sint64 amount;
	};

	struct CreateRoomRecord_output
	{
		id roomId;
		uint8 returnCode;
	};

	struct CreateRoomRecord_locals
	{
		RoomInfo newRoom;
		id roomId;
		uint64 attempt;
	};

	struct ComputeNextStake_input
	{
		sint64 stake;
		sint64 raiseStep;
		sint64 maxStake;
	};

	struct ComputeNextStake_output
	{
		sint64 nextStake;
		uint8 returnCode;
	};

	struct CreateRoom_locals
	{
		AddUserData_input addUserInput;
		AddUserData_output addUserOutput;
		CreateRoomRecord_input createRoomInput;
		CreateRoomRecord_output createRoomOutput;
	};

	struct GetWinnerPlayer_input
	{
		id player1;
		id player2;
	};

	struct GetWinnerPlayer_output
	{
		id winner;
	};

	struct GetWinnerPlayer_locals
	{
		m256i randomValue;
		m256i minPlayerId;
		m256i maxPlayerId;
	};

	struct CalculateRevenue_input
	{
		uint64 amount;
	};

	struct CalculateRevenue_output
	{
		uint64 devFee;
		uint64 burnFee;
		uint64 shareholdersFee;
		uint64 winner;
	};

	struct TransferToShareholders_input
	{
		uint64 amount;
	};

	struct TransferToShareholders_output
	{
		uint64 remainder;
	};

	struct TransferToShareholders_locals
	{
		Entity entity;
		uint64 shareholdersCount;
		uint64 perShareholderAmount;
		uint64 remainder;
		sint64 index;
		Asset rlAsset;
		uint64 dividendPerShare;
		AssetPossessionIterator rlIter;
		uint64 rlShares;
		uint64 transferredAmount;
		sint64 toTransfer;
	};

	struct FinalizeRoom_input
	{
		id roomId;
		id owner;
		uint64 roomAmount;
		bit includeLocked;
	};

	struct FinalizeRoom_output
	{
		uint8 returnCode;
	};

	struct FinalizeRoom_locals
	{
		UserData userData;
		sint64 availableDeposit;
		CreateRoomRecord_input createRoomInput;
		CreateRoomRecord_output createRoomOutput;
		ComputeNextStake_input nextStakeInput;
		ComputeNextStake_output nextStakeOutput;
	};

	struct ConnectToRoom_input
	{
		id roomId;
	};

	struct ConnectToRoom_output
	{
		uint8 returnCode;
	};

	struct ConnectToRoom_locals
	{
		RoomInfo room;
		GetWinnerPlayer_input getWinnerPlayer_input;
		GetWinnerPlayer_output getWinnerPlayer_output;
		CalculateRevenue_input calculateRevenue_input;
		CalculateRevenue_output calculateRevenue_output;
		TransferToShareholders_input transferToShareholders_input;
		TransferToShareholders_output transferToShareholders_output;
		FinalizeRoom_input finalizeInput;
		FinalizeRoom_output finalizeOutput;
		id winner;
		uint64 returnAmount;
		uint64 amount;
		bit failedGetWinner;
	};

	struct GetPercentFees_input
	{
	};

	struct GetPercentFees_output
	{
		uint8 devFeePercentBps;
		uint8 burnFeePercentBps;
		uint8 shareholdersFeePercentBps;
		uint16 percentScale;
		uint64 returnCode;
	};

	struct SetPercentFees_input
	{
		uint8 devFeePercentBps;
		uint8 burnFeePercentBps;
		uint8 shareholdersFeePercentBps;
	};

	struct SetPercentFees_output
	{
		uint8 returnCode;
	};

	struct SetPercentFees_locals
	{
		uint16 totalPercent;
	};

	struct GetRooms_input
	{
	};

	struct GetRooms_output
	{
		Array<RoomInfo, QDUEL_MAX_NUMBER_OF_ROOMS> rooms;

		uint8 returnCode;
	};

	struct GetRooms_locals
	{
		sint64 hashSetIndex;
		uint64 arrayIndex;
	};

	struct SetTTLHours_input
	{
		uint8 ttlHours;
	};

	struct SetTTLHours_output
	{
		uint8 returnCode;
	};

	struct GetTTLHours_input
	{
	};

	struct GetTTLHours_output
	{
		uint8 ttlHours;
		uint8 returnCode;
	};

	struct GetUserProfile_input
	{
	};

	struct GetUserProfile_output
	{
		id roomId;
		uint64 depositedAmount;
		uint64 locked;
		uint64 stake;
		uint64 raiseStep;
		uint64 maxStake;
		uint8 returnCode;
	};

	struct GetUserProfile_locals
	{
		UserData userData;
	};

	struct Deposit_input
	{
	};

	struct Deposit_output
	{
		uint8 returnCode;
	};

	struct Deposit_locals
	{
		UserData userData;
	};

	struct Withdraw_input
	{
		sint64 amount;
	};

	struct Withdraw_output
	{
		uint8 returnCode;
	};

	struct Withdraw_locals
	{
		UserData userData;
		sint64 freeAmount;
	};

	struct END_TICK_locals
	{
		UserData userData;
		RoomInfo room;
		DateAndTime now;
		sint64 roomIndex;
		uint32 currentTimestamp;
		uint64 elapsedSeconds;
		FinalizeRoom_input finalizeInput;
		FinalizeRoom_output finalizeOutput;
	};

public:
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateRoom, 1);
		REGISTER_USER_PROCEDURE(ConnectToRoom, 2);
		REGISTER_USER_PROCEDURE(SetPercentFees, 3);
		REGISTER_USER_PROCEDURE(SetTTLHours, 4);
		REGISTER_USER_PROCEDURE(Deposit, 5);
		REGISTER_USER_PROCEDURE(Withdraw, 6);

		REGISTER_USER_FUNCTION(GetPercentFees, 1);
		REGISTER_USER_FUNCTION(GetRooms, 2);
		REGISTER_USER_FUNCTION(GetTTLHours, 3);
		REGISTER_USER_FUNCTION(GetUserProfile, 4);
	}

	INITIALIZE()
	{
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

		state.minimumDuelAmount = QDUEL_MINIMUM_DUEL_AMOUNT;

		// Fee
		state.devFeePercentBps = QDUEL_DEV_FEE_PERCENT_BPS;
		state.burnFeePercentBps = QDUEL_BURN_FEE_PERCENT_BPS;
		state.shareholdersFeePercentBps = QDUEL_SHAREHOLDERS_FEE_PERCENT_BPS;

		state.ttlHours = QDUEL_TTL_HOURS;
	}

	BEGIN_EPOCH()
	{
		state.firstTick = true;
		state.currentState = EState::LOCKED;
	}

	END_EPOCH()
	{
		state.rooms.cleanup();
		state.users.cleanup();
	}

	END_TICK_WITH_LOCALS()
	{
		if (mod<uint32>(qpi.tick(), QDUEL_TICK_UPDATE_PERIOD) != 0)
		{
			return;
		}

		if ((state.currentState & EState::WAIT_TIME) != EState::NONE)
		{
			RL::makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentTimestamp);
			if (RL_DEFAULT_INIT_TIME < locals.currentTimestamp)
			{
				state.currentState = removeStateFlag(state.currentState, EState::WAIT_TIME);
			}
		}

		if ((state.currentState & EState::LOCKED) != EState::NONE)
		{
			return;
		}

		locals.roomIndex = state.rooms.nextElementIndex(NULL_INDEX);
		while (locals.roomIndex != NULL_INDEX)
		{
			locals.room = state.rooms.value(locals.roomIndex);
			locals.now = qpi.now();

			/**
			 * The interval between the end of the epoch and the first valid tick can be large.
			 * To do this, we restore the time before the room was closed.
			 */
			if (state.firstTick)
			{
				locals.room.lastUpdate = locals.now;
			}

			locals.elapsedSeconds = div(locals.room.lastUpdate.durationMicrosec(locals.now), 1000000ULL);
			if (locals.elapsedSeconds >= locals.room.closeTimer)
			{
				locals.finalizeInput.roomId = locals.room.roomId;
				locals.finalizeInput.owner = locals.room.owner;
				locals.finalizeInput.roomAmount = locals.room.amount;
				locals.finalizeInput.includeLocked = true;
				CALL(FinalizeRoom, locals.finalizeInput, locals.finalizeOutput);
			}
			else
			{
				locals.room.closeTimer = usubSatu64(locals.room.closeTimer, locals.elapsedSeconds);
				locals.room.lastUpdate = locals.now;
				state.rooms.set(locals.room.roomId, locals.room);
			}

			locals.roomIndex = state.rooms.nextElementIndex(locals.roomIndex);
		}

		state.firstTick = false;
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(CreateRoom)
	{
		if ((state.currentState & EState::LOCKED) != EState::NONE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::STATE_LOCKED);
			return;
		}

		if (qpi.invocationReward() < state.minimumDuelAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT); // insufficient duel amount
			return;
		}

		if (input.stake < state.minimumDuelAmount || (input.maxStake > 0 && input.maxStake < input.stake))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (qpi.invocationReward() < input.stake)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT);
			return;
		}

		if (state.users.contains(qpi.invocator()))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::USER_ALREADY_EXISTS);
			return;
		}

		locals.createRoomInput.owner = qpi.invocator();
		locals.createRoomInput.allowedPlayer = input.allowedPlayer;
		locals.createRoomInput.amount = input.stake;
		CALL(CreateRoomRecord, locals.createRoomInput, locals.createRoomOutput);
		if (locals.createRoomOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = locals.createRoomOutput.returnCode;
			return;
		}

		locals.addUserInput.userId = qpi.invocator();
		locals.addUserInput.roomId = locals.createRoomOutput.roomId;
		locals.addUserInput.allowedPlayer = input.allowedPlayer;
		if (qpi.invocationReward() > input.stake)
		{
			locals.addUserInput.depositedAmount = qpi.invocationReward() - input.stake;
		}
		else
		{
			locals.addUserInput.depositedAmount = 0;
		}
		locals.addUserInput.stake = input.stake;
		locals.addUserInput.raiseStep = input.raiseStep;
		locals.addUserInput.maxStake = input.maxStake;

		CALL(AddUserData, locals.addUserInput, locals.addUserOutput);
		if (locals.addUserOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
		{
			state.rooms.removeByKey(locals.createRoomOutput.roomId);
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = locals.addUserOutput.returnCode;
			return;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(ConnectToRoom)
	{
		if ((state.currentState & EState::LOCKED) != EState::NONE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::STATE_LOCKED);
			return;
		}

		if (!state.rooms.get(input.roomId, locals.room))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_NOT_FOUND);

			return;
		}

		if (locals.room.allowedPlayer != NULL_ID)
		{
			if (locals.room.allowedPlayer != qpi.invocator())
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());

				output.returnCode = toReturnCode(EReturnCode::ROOM_ACCESS_DENIED);
				return;
			}
		}

		if (qpi.invocationReward() < locals.room.amount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT);
			return;
		}

		if (qpi.invocationReward() > locals.room.amount)
		{
			locals.returnAmount = qpi.invocationReward() - locals.room.amount;
			qpi.transfer(qpi.invocator(), locals.returnAmount);
		}

		locals.amount = (qpi.invocationReward() - locals.returnAmount) + locals.room.amount;

		locals.getWinnerPlayer_input.player1 = locals.room.owner;
		locals.getWinnerPlayer_input.player2 = qpi.invocator();

		CALL(GetWinnerPlayer, locals.getWinnerPlayer_input, locals.getWinnerPlayer_output);
		locals.winner = locals.getWinnerPlayer_output.winner;

		if (locals.winner == id::zero() ||
		    (locals.winner != locals.getWinnerPlayer_input.player1 && locals.winner != locals.getWinnerPlayer_input.player2))
		{
			// Return fund to player1
			qpi.transfer(locals.getWinnerPlayer_input.player1, locals.room.amount);
			// Return fund to player2
			qpi.transfer(locals.getWinnerPlayer_input.player2, locals.room.amount);

			state.rooms.removeByKey(input.roomId);
			locals.amount = 0;
			locals.failedGetWinner = true;
			locals.room.amount = 0;
		}

		if (locals.amount > 0)
		{
			locals.calculateRevenue_input.amount = locals.amount;
			CALL(CalculateRevenue, locals.calculateRevenue_input, locals.calculateRevenue_output);
		}

		if (locals.calculateRevenue_output.winner > 0)
		{
			qpi.transfer(locals.winner, locals.calculateRevenue_output.winner);
		}
		else if (!locals.failedGetWinner)
		{
			// Return fund to player1
			qpi.transfer(locals.getWinnerPlayer_input.player1, locals.room.amount);
			// Return fund to player2
			qpi.transfer(locals.getWinnerPlayer_input.player2, locals.room.amount);

			state.rooms.removeByKey(input.roomId);
		}

		if (locals.calculateRevenue_output.devFee > 0)
		{
			qpi.transfer(state.teamAddress, locals.calculateRevenue_output.devFee);
		}
		if (locals.calculateRevenue_output.burnFee > 0)
		{
			qpi.burn(locals.calculateRevenue_output.burnFee);
		}
		if (locals.calculateRevenue_output.shareholdersFee > 0)
		{
			locals.transferToShareholders_input.amount = locals.calculateRevenue_output.shareholdersFee;

			CALL(TransferToShareholders, locals.transferToShareholders_input, locals.transferToShareholders_output);

			if (locals.transferToShareholders_output.remainder > 0)
			{
				qpi.burn(locals.transferToShareholders_output.remainder);
			}
		}

		locals.finalizeInput.roomId = input.roomId;
		locals.finalizeInput.owner = locals.room.owner;
		locals.finalizeInput.roomAmount = 0;
		locals.finalizeInput.includeLocked = false;
		CALL(FinalizeRoom, locals.finalizeInput, locals.finalizeOutput);
		output.returnCode = locals.finalizeOutput.returnCode;
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(SetPercentFees)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.teamAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		locals.totalPercent = static_cast<uint16>(input.devFeePercentBps) + static_cast<uint16>(input.burnFeePercentBps) +
		                      static_cast<uint16>(input.shareholdersFeePercentBps);
		locals.totalPercent = div(locals.totalPercent, QDUEL_PERCENT_SCALE);

		if (locals.totalPercent >= 100)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.devFeePercentBps = input.devFeePercentBps;
		state.burnFeePercentBps = input.burnFeePercentBps;
		state.shareholdersFeePercentBps = input.shareholdersFeePercentBps;

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(SetTTLHours)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.teamAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.ttlHours == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.ttlHours = input.ttlHours;

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION(GetPercentFees)
	{
		output.devFeePercentBps = state.devFeePercentBps;
		output.burnFeePercentBps = state.burnFeePercentBps;
		output.shareholdersFeePercentBps = state.shareholdersFeePercentBps;
		output.percentScale = QDUEL_PERCENT_SCALE;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetRooms)
	{
		locals.hashSetIndex = state.rooms.nextElementIndex(NULL_INDEX);
		while (locals.hashSetIndex != NULL_INDEX)
		{
			output.rooms.set(locals.arrayIndex++, state.rooms.value(locals.hashSetIndex));

			locals.hashSetIndex = state.rooms.nextElementIndex(locals.hashSetIndex);
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION(GetTTLHours)
	{
		output.ttlHours = state.ttlHours;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetUserProfile)
	{
		if (!state.users.get(qpi.invocator(), locals.userData))
		{
			output.returnCode = toReturnCode(EReturnCode::USER_NOT_FOUND);
			return;
		}

		output.roomId = locals.userData.roomId;
		output.depositedAmount = locals.userData.depositedAmount;
		output.locked = locals.userData.locked;
		output.stake = locals.userData.stake;
		output.raiseStep = locals.userData.raiseStep;
		output.maxStake = locals.userData.maxStake;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(Deposit)
	{
		if (qpi.invocationReward() == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (!state.users.get(qpi.invocator(), locals.userData))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = toReturnCode(EReturnCode::USER_NOT_FOUND);
			return;
		}

		locals.userData.depositedAmount += qpi.invocationReward();
		state.users.set(locals.userData.userId, locals.userData);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(Withdraw)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!state.users.get(qpi.invocator(), locals.userData))
		{
			output.returnCode = toReturnCode(EReturnCode::USER_NOT_FOUND);
			return;
		}

		locals.freeAmount = locals.userData.depositedAmount;

		if (input.amount == 0 || input.amount > locals.freeAmount)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FREE_DEPOSIT);
			return;
		}

		locals.userData.depositedAmount -= input.amount;
		state.users.set(locals.userData.userId, locals.userData);
		qpi.transfer(qpi.invocator(), input.amount);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

protected:
	HashMap<id, RoomInfo, QDUEL_MAX_NUMBER_OF_ROOMS> rooms;
	HashMap<id, UserData, QDUEL_MAX_NUMBER_OF_ROOMS> users;
	id teamAddress;
	sint64 minimumDuelAmount;
	uint8 devFeePercentBps;
	uint8 burnFeePercentBps;
	uint8 shareholdersFeePercentBps;
	uint8 ttlHours;
	uint8 firstTick;
	EState currentState;

protected:
	template<typename T> static constexpr const T& min(const T& a, const T& b) { return (a < b) ? a : b; }
	template<typename T> static constexpr const T& max(const T& a, const T& b) { return (a > b) ? a : b; }
	static constexpr const m256i& max(const m256i& a, const m256i& b) { return (a < b) ? b : a; }

	static void computeNextStake(const ComputeNextStake_input& input, ComputeNextStake_output& output)
	{
		output.nextStake = input.stake;

		if (input.raiseStep > 1LL)
		{
			if (input.maxStake > 0LL && input.stake > 0LL && input.raiseStep > div<sint64>(input.maxStake, input.stake))
			{
				output.nextStake = input.maxStake;
			}
			else
			{
				output.nextStake = smul(input.stake, input.raiseStep);
			}
		}

		if (input.maxStake > 0 && output.nextStake > input.maxStake)
		{
			output.nextStake = input.maxStake;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	static uint64_t usubSatu64(uint64 a, uint64 b) { return (a < b) ? 0 : (a - b); }

private:
	PRIVATE_PROCEDURE_WITH_LOCALS(CreateRoomRecord)
	{
		if (state.rooms.population() >= state.rooms.capacity())
		{
			output.returnCode = toReturnCode(EReturnCode::ROOM_FULL);
			output.roomId = id::zero();
			return;
		}

		locals.attempt = 0;
		while (locals.attempt < 8)
		{
			locals.roomId = qpi.K12(m256i(qpi.tick() ^ state.rooms.population() ^ input.owner.u64._0 ^ locals.attempt,
			                              input.owner.u64._1 ^ input.allowedPlayer.u64._0 ^ (locals.attempt << 1),
			                              input.owner.u64._2 ^ input.allowedPlayer.u64._1 ^ (locals.attempt << 2),
			                              input.owner.u64._3 ^ input.amount ^ (locals.attempt << 3)));
			if (!state.rooms.contains(locals.roomId))
			{
				break;
			}
			++locals.attempt;
		}
		if (locals.attempt >= 8)
		{
			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_CREATE);
			output.roomId = id::zero();
			return;
		}

		locals.newRoom.roomId = locals.roomId;
		locals.newRoom.owner = input.owner;
		locals.newRoom.allowedPlayer = input.allowedPlayer;
		locals.newRoom.amount = input.amount;
		locals.newRoom.closeTimer = static_cast<uint32>(state.ttlHours) * 3600U;
		locals.newRoom.lastUpdate = qpi.now();

		if (state.rooms.set(locals.roomId, locals.newRoom) == NULL_INDEX)
		{
			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_CREATE);
			output.roomId = id::zero();
			return;
		}

		output.roomId = locals.roomId;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeRoom)
	{
		state.rooms.removeByKey(input.roomId);

		if (!state.users.get(input.owner, locals.userData))
		{
			if (input.roomAmount > 0)
			{
				qpi.transfer(input.owner, input.roomAmount);
			}
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}

		locals.availableDeposit = locals.userData.depositedAmount;
		if (input.includeLocked)
		{
			locals.availableDeposit += locals.userData.locked;
		}

		if (locals.availableDeposit == 0)
		{
			state.users.removeByKey(locals.userData.userId);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}

		locals.nextStakeInput.stake = locals.userData.stake;
		locals.nextStakeInput.raiseStep = locals.userData.raiseStep;
		locals.nextStakeInput.maxStake = locals.userData.maxStake;
		computeNextStake(locals.nextStakeInput, locals.nextStakeOutput);
		if (locals.nextStakeOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
		{
			qpi.transfer(locals.userData.userId, locals.availableDeposit);
			state.users.removeByKey(locals.userData.userId);
			output.returnCode = locals.nextStakeOutput.returnCode;
			return;
		}

		if (locals.nextStakeOutput.nextStake > locals.availableDeposit)
		{
			locals.nextStakeOutput.nextStake = locals.availableDeposit;
		}

		if (locals.nextStakeOutput.nextStake < state.minimumDuelAmount)
		{
			qpi.transfer(locals.userData.userId, locals.availableDeposit);
			state.users.removeByKey(locals.userData.userId);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}

		locals.createRoomInput.owner = locals.userData.userId;
		locals.createRoomInput.allowedPlayer = locals.userData.allowedPlayer;
		locals.createRoomInput.amount = locals.nextStakeOutput.nextStake;
		CALL(CreateRoomRecord, locals.createRoomInput, locals.createRoomOutput);
		if (locals.createRoomOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
		{
			qpi.transfer(locals.userData.userId, locals.availableDeposit);
			state.users.removeByKey(locals.userData.userId);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}

		locals.userData.roomId = locals.createRoomOutput.roomId;
		locals.userData.depositedAmount = locals.availableDeposit - locals.nextStakeOutput.nextStake;
		locals.userData.locked = locals.nextStakeOutput.nextStake;
		locals.userData.stake = locals.nextStakeOutput.nextStake;
		state.users.set(locals.userData.userId, locals.userData);

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);

		state.rooms.cleanupIfNeeded(QDUEL_ROOMS_REMOVAL_THRESHOLD_PERCENT);
		state.users.cleanupIfNeeded(QDUEL_ROOMS_REMOVAL_THRESHOLD_PERCENT);
	}

private:
	PRIVATE_FUNCTION_WITH_LOCALS(GetWinnerPlayer)
	{
		locals.minPlayerId = min(input.player1, input.player2);
		locals.maxPlayerId = max(input.player1, input.player2);

		locals.randomValue = qpi.getPrevSpectrumDigest();

		locals.randomValue.u64._0 ^= locals.minPlayerId.u64._0 ^ locals.maxPlayerId.u64._0 ^ qpi.tick();
		locals.randomValue.u64._1 ^= locals.minPlayerId.u64._1 ^ locals.maxPlayerId.u64._1;
		locals.randomValue.u64._2 ^= locals.minPlayerId.u64._2 ^ locals.maxPlayerId.u64._2;
		locals.randomValue.u64._3 ^= locals.minPlayerId.u64._3 ^ locals.maxPlayerId.u64._3;

		locals.randomValue = qpi.K12(locals.randomValue);

		output.winner = locals.randomValue.u64._0 & 1 ? locals.maxPlayerId : locals.minPlayerId;
	}

	PRIVATE_FUNCTION(CalculateRevenue)
	{
		output.devFee = div<uint64>(smul(input.amount, static_cast<uint64>(state.devFeePercentBps)), QDUEL_PERCENT_SCALE);
		output.burnFee = div<uint64>(smul(input.amount, static_cast<uint64>(state.burnFeePercentBps)), QDUEL_PERCENT_SCALE);
		output.shareholdersFee =
		    smul(div(div<uint64>(smul(input.amount, static_cast<uint64>(state.shareholdersFeePercentBps)), QDUEL_PERCENT_SCALE), 676ULL), 676ULL);
		output.winner = input.amount - (output.devFee + output.burnFee + output.shareholdersFee);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(TransferToShareholders)
	{
		if (input.amount == 0)
		{
			return;
		}

		locals.rlAsset.issuer = id::zero();
		locals.rlAsset.assetName = QDUEL_RANDOM_LOTTERY_ASSET_NAME;

		locals.dividendPerShare = div<uint64>(input.amount, NUMBER_OF_COMPUTORS);
		if (locals.dividendPerShare == 0)
		{
			return;
		}

		locals.rlIter.begin(locals.rlAsset);
		while (!locals.rlIter.reachedEnd())
		{
			locals.rlShares = static_cast<uint64>(locals.rlIter.numberOfPossessedShares());
			if (locals.rlShares > 0)
			{
				locals.toTransfer = static_cast<sint64>(smul(locals.rlShares, locals.dividendPerShare));
				if (qpi.transfer(locals.rlIter.possessor(), locals.toTransfer) >= 0)
				{
					locals.transferredAmount += locals.toTransfer;
				}
			}
			locals.rlIter.next();
		}

		output.remainder = input.amount - locals.transferredAmount;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(AddUserData)
	{
		// Already Exist
		if (state.users.contains(input.userId))
		{
			output.returnCode = toReturnCode(EReturnCode::USER_ALREADY_EXISTS);
			return;
		}

		locals.newUserData.userId = input.userId;
		locals.newUserData.roomId = input.roomId;
		locals.newUserData.allowedPlayer = input.allowedPlayer;
		locals.newUserData.depositedAmount = input.depositedAmount;
		locals.newUserData.locked = input.stake;
		locals.newUserData.stake = input.stake;
		locals.newUserData.raiseStep = input.raiseStep;
		locals.newUserData.maxStake = input.maxStake;
		if (state.users.set(input.userId, locals.newUserData) == NULL_INDEX)
		{
			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_CREATE);
			return;
		}
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}
};
