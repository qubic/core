// File: test/contract_rl.cpp
#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_BUY_TICKET = 1;
constexpr uint16 FUNCTION_INDEX_GET_FEES = 1;
constexpr uint16 FUNCTION_INDEX_GET_PLAYERS = 2;
constexpr uint16 FUNCTION_INDEX_GET_WINNERS = 3;

static const id RL_DEV_ADDRESS = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C,
	_R, _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

// Equality operator for comparing WinnerInfo objects
bool operator==(const RL::WinnerInfo& left, const RL::WinnerInfo& right)
{
	return left.winnerAddress == right.winnerAddress && left.revenue == right.revenue && left.epoch == right.epoch && left.tick == right.tick;
}

// Test helper that exposes internal state assertions
class RLChecker : public RL
{
public:
	void checkTicketPrice(const uint64& price) { EXPECT_EQ(ticketPrice, price); }

	void checkFees(const GetFees_output& fees)
	{
		EXPECT_EQ(fees.returnCode, static_cast<uint8>(EReturnCode::SUCCESS));

		EXPECT_EQ(fees.distributionFeePercent, distributionFeePercent);
		EXPECT_EQ(fees.teamFeePercent, teamFeePercent);
		EXPECT_EQ(fees.winnerFeePercent, winnerFeePercent);
		EXPECT_EQ(fees.burnPercent, burnPercent);
	}

	void checkPlayers(const GetPlayers_output& output) const
	{
		EXPECT_EQ(output.returnCode, static_cast<uint8>(EReturnCode::SUCCESS));
		EXPECT_EQ(output.players.capacity(), players.capacity());
		EXPECT_EQ(static_cast<uint64>(output.numberOfPlayers), players.population());

		for (uint64 i = 0, playerArrayIndex = 0; i < players.capacity(); ++i)
		{
			if (!players.isEmptySlot(i))
			{
				EXPECT_EQ(output.players.get(playerArrayIndex++), players.key(i));
			}
		}
	}

	void checkWinners(const GetWinners_output& output) const
	{
		EXPECT_EQ(output.returnCode, static_cast<uint8>(EReturnCode::SUCCESS));
		EXPECT_EQ(output.winners.capacity(), winners.capacity());
		EXPECT_EQ(output.numberOfWinners, winnersInfoNextEmptyIndex);

		for (uint64 i = 0; i < output.numberOfWinners; ++i)
		{
			EXPECT_EQ(output.winners.get(i), winners.get(i));
		}
	}

	void randomlyAddPlayers(uint64 maxNewPlayers)
	{
		const uint64 newPlayerCount = mod<uint64>(maxNewPlayers, players.capacity());
		for (uint64 i = 0; i < newPlayerCount; ++i)
		{
			players.add(id::randomValue());
		}
	}

	void randomlyAddWinners(uint64 maxNewWinners)
	{
		const uint64 newWinnerCount = mod<uint64>(maxNewWinners, winners.capacity());

		winnersInfoNextEmptyIndex = 0;
		WinnerInfo wi;

		for (uint64 i = 0; i < newWinnerCount; ++i)
		{
			wi.epoch = 1;
			wi.tick = 1;
			wi.revenue = 1000000;
			wi.winnerAddress = id::randomValue();
			winners.set(winnersInfoNextEmptyIndex++, wi);
		}
	}

	void setSelling() { currentState = EState::SELLING; }

	void setLocked() { currentState = EState::LOCKED; }

	uint64 playersPopulation() const { return players.population(); }

	uint64 getTicketPrice() const { return ticketPrice; }
};

class ContractTestingRL : protected ContractTesting
{
public:
	ContractTestingRL()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(RL);
		callSystemProcedure(RL_CONTRACT_INDEX, INITIALIZE);
	}

	RLChecker* getState() { return reinterpret_cast<RLChecker*>(contractStates[RL_CONTRACT_INDEX]); }

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

	RL::BuyTicket_output buyTicket(const id& user, uint64 reward)
	{
		RL::BuyTicket_input input;
		RL::BuyTicket_output output;
		invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_BUY_TICKET, input, output, user, reward);
		return output;
	}

	void BeginEpoch() { callSystemProcedure(RL_CONTRACT_INDEX, BEGIN_EPOCH); }

	void EndEpoch() { callSystemProcedure(RL_CONTRACT_INDEX, END_EPOCH); }
};

TEST(ContractRandomLottery, GetFees)
{
	ContractTestingRL ctl;
	RL::GetFees_output output = ctl.getFees();
	ctl.getState()->checkFees(output);
}

TEST(ContractRandomLottery, GetPlayers)
{
	ContractTestingRL ctl;

	// Initially empty
	RL::GetPlayers_output output = ctl.getPlayers();
	ctl.getState()->checkPlayers(output);

	// Add random players directly to state (test helper)
	constexpr uint64 maxPlayersToAdd = 10;
	ctl.getState()->randomlyAddPlayers(maxPlayersToAdd);
	output = ctl.getPlayers();
	ctl.getState()->checkPlayers(output);
}

TEST(ContractRandomLottery, GetWinners)
{
	ContractTestingRL ctl;

	// Populate winners history artificially
	constexpr uint64 maxNewWinners = 10;
	ctl.getState()->randomlyAddWinners(maxNewWinners);
	RL::GetWinners_output winnersOutput = ctl.getWinners();
	ctl.getState()->checkWinners(winnersOutput);
}

TEST(ContractRandomLottery, BuyTicket)
{
	ContractTestingRL ctl;

	const uint64 ticketPrice = ctl.getState()->getTicketPrice();

	// 1. Attempt when state is LOCKED (should fail and refund invocation reward)
	{
		const id userLocked = id::randomValue();
		increaseEnergy(userLocked, ticketPrice * 2);
		RL::BuyTicket_output out = ctl.buyTicket(userLocked, ticketPrice);
		EXPECT_EQ(out.returnCode, static_cast<uint8>(RL::EReturnCode::TICKET_SELLING_CLOSED));
		EXPECT_EQ(ctl.getState()->playersPopulation(), 0);
	}

	// Switch to SELLING to allow purchases
	ctl.getState()->setSelling();

	// 2. Loop over several users and test invalid price, success, duplicate
	constexpr uint64 userCount = 5;
	uint64 expectedPlayers = 0;

	for (uint64 i = 0; i < userCount; ++i)
	{
		const id user = id::randomValue();
		increaseEnergy(user, ticketPrice * 5);

		// (a) Invalid price (wrong reward sent) — player not added
		{
			const RL::BuyTicket_output outInvalid = ctl.buyTicket(user, ticketPrice - 1);
			EXPECT_EQ(outInvalid.returnCode, static_cast<uint8>(RL::EReturnCode::TICKET_INVALID_PRICE));
			EXPECT_EQ(ctl.getState()->playersPopulation(), expectedPlayers);
		}

		// (b) Valid purchase — player added
		{
			const RL::BuyTicket_output outOk = ctl.buyTicket(user, ticketPrice);
			EXPECT_EQ(outOk.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
			++expectedPlayers;
			EXPECT_EQ(ctl.getState()->playersPopulation(), expectedPlayers);
		}

		// (c) Duplicate purchase — rejected
		{
			const RL::BuyTicket_output outDup = ctl.buyTicket(user, ticketPrice);
			EXPECT_EQ(outDup.returnCode, static_cast<uint8>(RL::EReturnCode::TICKET_ALREADY_PURCHASED));
			EXPECT_EQ(ctl.getState()->playersPopulation(), expectedPlayers);
		}
	}

	// 3. Sanity check: number of unique players matches expectations
	EXPECT_EQ(expectedPlayers, userCount);
}

TEST(ContractRandomLottery, EndEpoch)
{
	ContractTestingRL ctl;

	// Helper: contract balance holder (SELF account)
	const id contractAddress = id(RL_CONTRACT_INDEX, 0, 0, 0);
	const uint64 ticketPrice = ctl.getState()->getTicketPrice();

	// Current fee configuration (set in INITIALIZE)
	const RL::GetFees_output fees = ctl.getFees();
	const uint8 teamPercent = fees.teamFeePercent;                 // Team commission percent
	const uint8 distributionPercent = fees.distributionFeePercent; // Distribution (dividends) percent
	const uint8 burnPercent = fees.burnPercent;                    // Burn percent
	const uint8 winnerPercent = fees.winnerFeePercent;             // Winner payout percent

	// --- Scenario 1: No players (should just lock and clear silently) ---
	{
		ctl.BeginEpoch();
		EXPECT_EQ(ctl.getState()->playersPopulation(), 0u);

		RL::GetWinners_output before = ctl.getWinners();
		EXPECT_EQ(before.numberOfWinners, 0u);

		ctl.EndEpoch();

		RL::GetWinners_output after = ctl.getWinners();
		EXPECT_EQ(after.numberOfWinners, 0u);
		EXPECT_EQ(ctl.getState()->playersPopulation(), 0u);
	}

	// --- Scenario 2: Exactly one player (ticket refunded, no winner recorded) ---
	{
		ctl.BeginEpoch();

		const id solo = id::randomValue();
		increaseEnergy(solo, ticketPrice);
		const uint64 balanceBefore = getBalance(solo);

		const RL::BuyTicket_output out = ctl.buyTicket(solo, ticketPrice);
		EXPECT_EQ(out.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
		EXPECT_EQ(ctl.getState()->playersPopulation(), 1u);
		EXPECT_EQ(getBalance(solo), balanceBefore - ticketPrice);

		ctl.EndEpoch();

		// Refund happened
		EXPECT_EQ(getBalance(solo), balanceBefore);
		EXPECT_EQ(ctl.getState()->playersPopulation(), 0u);

		const RL::GetWinners_output winners = ctl.getWinners();
		EXPECT_EQ(winners.numberOfWinners, 0u);
	}

	// --- Scenario 3: Multiple players (winner chosen, fees processed, remainder burned) ---
	{
		ctl.BeginEpoch();

		constexpr uint32 N = 5;
		struct PlayerInfo
		{
			id addr;
			uint64 balanceBefore;
			uint64 balanceAfterBuy;
		};
		std::vector<PlayerInfo> infos;
		infos.reserve(N);

		// Add N distinct players with valid purchases
		for (uint32 i = 0; i < N; ++i)
		{
			const id randomUser = id::randomValue();
			increaseEnergy(randomUser, ticketPrice * 2);
			const uint64 bBefore = getBalance(randomUser);
			const RL::BuyTicket_output out = ctl.buyTicket(randomUser, ticketPrice);
			EXPECT_EQ(out.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
			EXPECT_EQ(getBalance(randomUser), bBefore - ticketPrice);
			infos.push_back({randomUser, bBefore, bBefore - ticketPrice});
		}

		EXPECT_EQ(ctl.getState()->playersPopulation(), N);

		const uint64 contractBalanceBefore = getBalance(contractAddress);
		EXPECT_EQ(contractBalanceBefore, ticketPrice * N);

		const uint64 teamBalanceBefore = getBalance(RL_DEV_ADDRESS);

		const RL::GetWinners_output winnersBefore = ctl.getWinners();
		const uint64 winnersCountBefore = winnersBefore.numberOfWinners;

		ctl.EndEpoch();

		// Players reset after epoch end
		EXPECT_EQ(ctl.getState()->playersPopulation(), 0u);

		const RL::GetWinners_output winnersAfter = ctl.getWinners();
		EXPECT_EQ(winnersAfter.numberOfWinners, winnersCountBefore + 1);

		// Newly appended winner info
		const RL::WinnerInfo wi = winnersAfter.winners.get(winnersCountBefore);
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

		// Burn
		const uint64 burnExpected = contractBalanceBefore - ((contractBalanceBefore * burnPercent) / 100) -
		                            ((((contractBalanceBefore * distributionPercent) / 100) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS) -
		                            ((contractBalanceBefore * teamPercent) / 100) - ((contractBalanceBefore * winnerPercent) / 100);
		EXPECT_EQ(getBalance(contractAddress), burnExpected);
	}
}
