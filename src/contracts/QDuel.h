using namespace QPI;

constexpr uint32 QDUEL_MAX_NUMBER_OF_ROOMS = 512;
constexpr uint64 QDUEL_MINIMUM_DUEL_AMOUNT = 10000;
constexpr uint8 QDUEL_DEV_FEE_PERCENT_BPS = 15;          // 0.15% * QDUEL_PERCENT_SCALE
constexpr uint8 QDUEL_BURN_FEE_PERCENT_BPS = 30;         // 0.3% * QDUEL_PERCENT_SCALE
constexpr uint8 QDUEL_SHAREHOLDERS_FEE_PERCENT_BPS = 25; // 0.25% * QDUEL_PERCENT_SCALE
constexpr uint8 QDUEL_PERCENT_SCALE = 100;
constexpr uint8 QDUEL_TTL_HOURS = 2;
constexpr uint8 QDUEL_TICK_UPDATE_PERIOD = 10;            // Process TICK logic once per this many ticks
constexpr uint64 QDUEL_RANDOM_LOTTERY_ASSET_NAME = 19538; // RL

struct QDUEL2
{
};

struct QDUEL : public ContractBase
{
public:
	enum class EReturnCode : uint8
	{
		SUCCESS,
		ACCESS_DENIED,
		INVALID_VALUE,

		// Room
		ROOM_INSUFFICIENT_DUEL_AMOUNT,
		ROOM_NOT_FOUND,
		ROOM_FULL,
		ROOM_FAILED_CREATE,
		ROOM_FAILED_GET_WINNER,
		ROOM_ACCESS_DENIED,
		ROOM_FAILED_CALCULATE_REVENUE,

		UNKNOWN_ERROR = UINT8_MAX
	};

	static constexpr uint8 toReturnCode(EReturnCode code) { return static_cast<uint8>(code); }

	struct RoomInfo
	{
		id roomId;
		id player1;
		id allowedPlayer; // If zero, anyone can join
		uint64 amount;
		DateAndTime creationTimestamp;
	};

	struct CreateRoom_input
	{
		id allowedPlayer; // If zero, anyone can join
	};

	struct CreateRoom_output
	{
		uint8 returnCode;
	};

	struct CreateRoom_locals
	{
		RoomInfo newRoom;
		id roomId;
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
		id winner;
		uint64 returnAmount;
		uint64 amount;
	};

	struct GetPercentFees_input
	{
	};

	struct GetPercentFees_output
	{
		uint8 devFeePercentBps;
		uint8 burnFeePercentBps;
		uint8 shareholdersFeePercentBps;
		uint8 percentScale;
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

	struct END_TICK_locals
	{
		sint64 roomIndex;
		RoomInfo room;
		DateAndTime threshold;
	};

	struct END_EPOCH_locals
	{
		sint64 roomIndex;
		RoomInfo room;
	};

public:
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateRoom, 1);
		REGISTER_USER_PROCEDURE(ConnectToRoom, 2);
		REGISTER_USER_PROCEDURE(SetPercentFees, 3);
		REGISTER_USER_PROCEDURE(SetTTLHours, 4);

		REGISTER_USER_FUNCTION(GetPercentFees, 1);
		REGISTER_USER_FUNCTION(GetRooms, 2);
		REGISTER_USER_FUNCTION(GetTTLHours, 3);
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

	END_TICK_WITH_LOCALS()
	{
		if (mod<uint32>(qpi.tick(), QDUEL_TICK_UPDATE_PERIOD) != 0)
		{
			return;
		}

		locals.roomIndex = state.rooms.nextElementIndex(NULL_INDEX);
		while (locals.roomIndex != NULL_INDEX)
		{
			locals.room = state.rooms.value(locals.roomIndex);
			locals.threshold = locals.room.creationTimestamp;
			locals.threshold.add(0, 0, 0, state.ttlHours, 0, 0);

			if (locals.threshold < qpi.now() || locals.threshold == qpi.now())
			{
				qpi.transfer(locals.room.player1, locals.room.amount);
				state.rooms.removeByIndex(locals.roomIndex);
			}

			locals.roomIndex = state.rooms.nextElementIndex(locals.roomIndex);
		}
	}

	END_EPOCH_WITH_LOCALS()
	{
		locals.roomIndex = state.rooms.nextElementIndex(NULL_INDEX);
		while (locals.roomIndex != NULL_INDEX)
		{
			locals.room = state.rooms.value(locals.roomIndex);
			qpi.transfer(locals.room.player1, locals.room.amount);

			locals.roomIndex = state.rooms.nextElementIndex(locals.roomIndex);
		}

		state.rooms.reset();
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(CreateRoom)
	{
		if (qpi.invocationReward() < state.minimumDuelAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_INSUFFICIENT_DUEL_AMOUNT); // insufficient duel amount
			return;
		}

		if (state.rooms.population() >= state.rooms.capacity())
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_FULL); // no more rooms available
			return;
		}

		locals.roomId = qpi.K12(qpi.tick() ^ state.rooms.population() ^ qpi.invocator().u64._0);
		if (state.rooms.contains(locals.roomId))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_CREATE); // room creation failed

			return;
		}

		locals.newRoom.roomId = locals.roomId;
		locals.newRoom.player1 = qpi.invocator();
		locals.newRoom.allowedPlayer = input.allowedPlayer;
		locals.newRoom.amount = qpi.invocationReward();
		locals.newRoom.creationTimestamp = qpi.now();

		if (state.rooms.set(locals.roomId, locals.newRoom) == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_CREATE);
			return;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(ConnectToRoom)
	{
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

		locals.getWinnerPlayer_input.player1 = locals.room.player1;
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

			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_GET_WINNER);
			return;
		}

		locals.calculateRevenue_input.amount = locals.amount;
		CALL(CalculateRevenue, locals.calculateRevenue_input, locals.calculateRevenue_output);

		if (locals.calculateRevenue_output.winner > 0)
		{
			qpi.transfer(locals.winner, locals.calculateRevenue_output.winner);
		}
		else
		{
			// Return fund to player1
			qpi.transfer(locals.getWinnerPlayer_input.player1, locals.room.amount);
			// Return fund to player2
			qpi.transfer(locals.getWinnerPlayer_input.player2, locals.room.amount);

			output.returnCode = toReturnCode(EReturnCode::ROOM_FAILED_CALCULATE_REVENUE);
			return;
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

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
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
		locals.totalPercent = div(locals.totalPercent, static_cast<uint16>(QDUEL_PERCENT_SCALE));

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

protected:
	HashMap<id, RoomInfo, QDUEL_MAX_NUMBER_OF_ROOMS> rooms;
	id teamAddress;
	uint64 minimumDuelAmount;
	uint8 devFeePercentBps;
	uint8 burnFeePercentBps;
	uint8 shareholdersFeePercentBps;
	uint8 ttlHours;

protected:
	template<typename T> static constexpr const T& min(const T& a, const T& b) { return (a < b) ? a : b; }
	template<typename T> static constexpr const T& max(const T& a, const T& b) { return (a > b) ? a : b; }
	static constexpr const m256i& max(const m256i& a, const m256i& b) { return (a < b) ? b : a; }

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
};
