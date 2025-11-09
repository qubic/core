using namespace QPI;

struct QTF2
{
};

static constexpr uint64 QTF_MAX_NUMBER_OF_PLAYERS = 1024;
static constexpr uint64 QTF_RANDOM_VALUES_COUNT = 4;
static constexpr uint64 QTF_MAX_RANDOM_VALUE = 30;
static constexpr uint64 QTF_TICKET_PRICE = 1000000;

static id QTF_ADDRESS_DEV_TEAM = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C,
                                    _R, _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

using QTFRandomValues = Array<uint8, QTF_RANDOM_VALUES_COUNT>;
using QFTWinnerPlayers = Array<id, QTF_MAX_NUMBER_OF_PLAYERS>;

struct QTF : public ContractBase
{
public:
	enum class EReturnCode : uint8
	{
		SUCCESS = 0,
		INVALID_TICKET_PRICE = 1,
		MAX_PLAYERS_REACHED = 2,
		ACCESS_DENIED = 3,

		MAX_VALUE = UINT8_MAX
	};

	struct PlayerData
	{
		id player;
		QTFRandomValues randomValues;
	};

	struct WinnerData
	{
		Array<PlayerData, QTF_MAX_NUMBER_OF_PLAYERS> winners;
		QTFRandomValues winnerValues;
		uint64 winnerCounter;
		uint16 epoch;
	};

	struct NextEpochData
	{
		uint64 newTicketPrice;
	};

	// Buy Ticket
	struct BuyTicket_input
	{
		QTFRandomValues randomValues;
	};
	struct BuyTicket_output
	{
		EReturnCode returnCode;
	};

	// Set Price
	struct SetPrice_input
	{
		uint64 newPrice;
	};
	struct SetPrice_output
	{
		EReturnCode returnCode;
	};

	// Ticket Price
	struct GetTicketPrice_input
	{
	};
	struct GetTicketPrice_output
	{
		uint64 ticketPrice;
	};

	// Next Epoch Data
	struct GetNextEpochData_input
	{
	};
	struct GetNextEpochData_output
	{
		NextEpochData nextEpochData;
	};

	// Winner Data
	struct GetWinnerData_input
	{
	};

	struct GetWinnerData_output
	{
		WinnerData winnerData;
	};

public:
	// Contract lifecycle methods
	INITIALIZE()
	{
		// Addresses
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

		// Owner address (currently identical to developer address; can be split in future revisions).
		state.ownerAddress = state.teamAddress;

		state.ticketPrice = QTF_TICKET_PRICE;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// Procedures
		REGISTER_USER_PROCEDURE(BuyTicket, 1);
		REGISTER_USER_PROCEDURE(SetPrice, 2);
		// Functions
		REGISTER_USER_FUNCTION(GetTicketPrice, 1);
		REGISTER_USER_FUNCTION(GetNextEpochData, 2);
		REGISTER_USER_FUNCTION(GetWinnerData, 3);
	}

	BEGIN_EPOCH()
	{
		applyNextEpochData(state);
		clearState(state);
	}

	END_EPOCH() {}

	// Procedures
	PUBLIC_PROCEDURE(BuyTicket)
	{
		if (state.numberOfPlayers >= QTF_MAX_NUMBER_OF_PLAYERS)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = EReturnCode::MAX_PLAYERS_REACHED;
			return;
		}

		if (qpi.invocationReward() != state.ticketPrice)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = EReturnCode::INVALID_TICKET_PRICE;
			return;
		}

		addPlayerInfo(state, qpi.invocator(), input.randomValues);
		output.returnCode = EReturnCode::SUCCESS;
	}

	PUBLIC_PROCEDURE(SetPrice)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newPrice == 0)
		{
			output.returnCode = EReturnCode::INVALID_TICKET_PRICE;
			return;
		}

		state.nextEpochData.newTicketPrice = input.newPrice;
	}

	// Functions
	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	PUBLIC_FUNCTION(GetNextEpochData) { output.nextEpochData = state.nextEpochData; }
	PUBLIC_FUNCTION(GetWinnerData) { output.winnerData = state.lastWinnerData; }

public:
protected:
	static void clearState(QTF& state)
	{
		// Clear players list
		if (state.numberOfPlayers > 0)
		{
			setMemory(state.players, 0);
			state.numberOfPlayers = 0;
		}

		// Clear temporary value
		{
			state.tmpValue64 = 0;
			state.tmpValue8 = 0;
			setMemory(state.tempPlayerInfo, 0);
		}

		setMemory(state.nextEpochData, 0);
	}

	static void applyNextEpochData(QTF& state)
	{
		// Apply new ticket price
		if (state.nextEpochData.newTicketPrice > 0)
		{
			state.ticketPrice = state.nextEpochData.newTicketPrice;
			state.nextEpochData.newTicketPrice = 0;
		}
	}

	static void getRandomValues(const uint64& seed, uint64& tmpValue, uint8& index, QTFRandomValues& output)
	{
		tmpValue = seed;
		for (index = 0; index < output.capacity(); ++index)
		{
			tmpValue ^= tmpValue >> 12;
			tmpValue ^= tmpValue << 25;
			tmpValue ^= tmpValue >> 27;
			tmpValue *= 2685821657736338717ULL;
			tmpValue = mod(tmpValue, QTF_MAX_RANDOM_VALUE) + 1;
			output.set(index, tmpValue);
		}
	}

	static void mix64(const uint64& x, uint64& outValue)
	{
		outValue = x;

		outValue ^= outValue >> 30;
		outValue *= 0xbf58476d1ce4e5b9ULL;
		outValue ^= outValue >> 27;
		outValue *= 0x94d049bb133111ebULL;
		outValue ^= outValue >> 31;
	}

	static void deriveOne(const uint64& r, const uint64& idx, uint64& outValue) { mix64(r + 0x9e3779b97f4a7c15ULL * (idx + 1), outValue); }

	static void deriveFour(const uint64& r, uint64& tempValue, uint8& index, QTFRandomValues& out)
	{
		for (index = 0; index < out.capacity(); ++index)
		{
			deriveOne(r, index, tempValue);
			out.set(index, tempValue);
		}
	}

	static void addPlayerInfo(QTF& state, const id& playerId, const QTFRandomValues& randomValues)
	{
		state.tempPlayerInfo.player = playerId;
		state.tempPlayerInfo.randomValues = randomValues;

		state.players.set(state.numberOfPlayers++, state.tempPlayerInfo);
	}

	static void clearWinnerData(QTF& state) { setMemory(state.lastWinnerData, 0); }

	static void fillWinnerData(QTF& state, const PlayerData& playerData, const QTFRandomValues& winnerValues, const uint16& epoch)
	{
		state.lastWinnerData.winners.set(state.lastWinnerData.winnerCounter++, playerData);
		state.lastWinnerData.winnerValues = winnerValues;
		state.lastWinnerData.epoch = epoch;
	}

protected:
	WinnerData lastWinnerData;

	NextEpochData nextEpochData;

	Array<PlayerData, QTF_MAX_NUMBER_OF_PLAYERS> players;

	id teamAddress;

	id ownerAddress;

	uint64 numberOfPlayers;

	uint64 ticketPrice;

	PlayerData tempPlayerInfo;

	uint64 tmpValue64;

	uint8 tmpValue8;
};
