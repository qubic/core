// File: test/contract_rl.cpp
#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_BUY_TICKET = 1;
constexpr uint16 FUNCTION_INDEX_GET_FEES = 1;
constexpr uint16 FUNCTION_INDEX_GET_PLAYERS = 2;
constexpr uint16 FUNCTION_INDEX_GET_WINNERS = 3;
constexpr uint16 FUNCTION_INDEX_GET_TICKET_PRICE = 4;
constexpr uint16 FUNCTION_INDEX_GET_MAX_NUM_PLAYERS = 5;
constexpr uint16 FUNCTION_INDEX_GET_STATE = 6;
constexpr uint16 FUNCTION_INDEX_GET_BALANCE = 7;

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
		EXPECT_EQ(output.playerCounter, playerCounter);

		for (uint64 i = 0; i < playerCounter; ++i)
		{
			EXPECT_EQ(output.players.get(i), players.get(i));
		}
	}

	void checkWinners(const GetWinners_output& output) const
	{
		EXPECT_EQ(output.returnCode, static_cast<uint8>(EReturnCode::SUCCESS));
		EXPECT_EQ(output.winners.capacity(), winners.capacity());
		EXPECT_EQ(output.winnersCounter, winnersCounter);

		for (uint64 i = 0; i < winnersCounter; ++i)
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

	uint64 getPlayerCounter() const { return playerCounter; }

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
	RL::GetBalance_output getBalanceInfo()
	{
		RL::GetBalance_input input;
		RL::GetBalance_output output;
		callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_BALANCE, input, output);
		return output;
	}

	RL::BuyTicket_output buyTicket(const id& user, uint64 reward)
	{
		RL::BuyTicket_input input;
		RL::BuyTicket_output output;
		if (!invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_BUY_TICKET, input, output, user, reward))
		{
			output.returnCode = static_cast<uint8>(RL::EReturnCode::UNKNOW_ERROR);
		}
		return output;
	}

	void BeginEpoch() { callSystemProcedure(RL_CONTRACT_INDEX, BEGIN_EPOCH); }

	void EndEpoch() { callSystemProcedure(RL_CONTRACT_INDEX, END_EPOCH); }

	// Returns the SELF contract account address
	id rlSelf() { return id(RL_CONTRACT_INDEX, 0, 0, 0); }

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
		EXPECT_EQ(out.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
	}

	// Assert contract account balance equals the value returned by RL::GetBalance
	void expectContractBalanceEqualsGetBalance(ContractTestingRL& ctl, const id& contractAddress)
	{
		const RL::GetBalance_output out = ctl.getBalanceInfo();
		EXPECT_EQ(out.balance, getBalance(contractAddress));
	}
};

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
		EXPECT_EQ(out.returnCode, static_cast<uint8>(RL::EReturnCode::TICKET_SELLING_CLOSED));
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0);
	}

	// Switch to SELLING to allow purchases
	ctl.BeginEpoch();

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
			EXPECT_EQ(outInvalid.returnCode, static_cast<uint8>(RL::EReturnCode::TICKET_INVALID_PRICE));
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);

			// == 0
			outInvalid = ctl.buyTicket(user, 0);
			EXPECT_EQ(outInvalid.returnCode, static_cast<uint8>(RL::EReturnCode::TICKET_INVALID_PRICE));
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);

			// < 0
			outInvalid = ctl.buyTicket(user, -ticketPrice);
			EXPECT_NE(outInvalid.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);
		}

		// (b) Valid purchase — player added
		{
			const RL::BuyTicket_output outOk = ctl.buyTicket(user, ticketPrice);
			EXPECT_EQ(outOk.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
			++expectedPlayers;
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);
		}

		// (c) Duplicate purchase — allowed, increases count
		{
			const RL::BuyTicket_output outDup = ctl.buyTicket(user, ticketPrice);
			EXPECT_EQ(outDup.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
			++expectedPlayers;
			EXPECT_EQ(ctl.state()->getPlayerCounter(), expectedPlayers);
		}
	}

	// 3. Sanity check: number of tickets equals twice the number of users (due to duplicate buys)
	EXPECT_EQ(ctl.state()->getPlayerCounter(), userCount * 2);
}

TEST(ContractRandomLottery, EndEpoch)
{
	ContractTestingRL ctl;

	const id contractAddress = ctl.rlSelf();
	const uint64 ticketPrice = ctl.state()->getTicketPrice();

	// Current fee configuration (set in INITIALIZE)
	const RL::GetFees_output fees = ctl.getFees();
	const uint8 teamPercent = fees.teamFeePercent;                 // Team commission percent
	const uint8 distributionPercent = fees.distributionFeePercent; // Distribution (dividends) percent
	const uint8 burnPercent = fees.burnPercent;                    // Burn percent
	const uint8 winnerPercent = fees.winnerFeePercent;             // Winner payout percent

	// --- Scenario 1: No players (should just lock and clear silently) ---
	{
		ctl.BeginEpoch();
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

		RL::GetWinners_output before = ctl.getWinners();
		EXPECT_EQ(before.winnersCounter, 0u);

		ctl.EndEpoch();

		RL::GetWinners_output after = ctl.getWinners();
		EXPECT_EQ(after.winnersCounter, 0u);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);
	}

	// --- Scenario 2: Exactly one player (ticket refunded, no winner recorded) ---
	{
		ctl.BeginEpoch();

		const id solo = id::randomValue();
		increaseEnergy(solo, ticketPrice);
		const uint64 balanceBefore = getBalance(solo);

		const RL::BuyTicket_output out = ctl.buyTicket(solo, ticketPrice);
		EXPECT_EQ(out.returnCode, static_cast<uint8>(RL::EReturnCode::SUCCESS));
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 1u);
		EXPECT_EQ(getBalance(solo), balanceBefore - ticketPrice);

		ctl.EndEpoch();

		// Refund happened
		EXPECT_EQ(getBalance(solo), balanceBefore);
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

		const RL::GetWinners_output winners = ctl.getWinners();
		EXPECT_EQ(winners.winnersCounter, 0u);
	}

	// --- Scenario 3: Multiple players (winner chosen, fees processed, remainder burned) ---
	{
		ctl.BeginEpoch();

		constexpr uint32 N = 5 * 2;
		struct PlayerInfo
		{
			id addr;
			uint64 balanceBefore;
			uint64 balanceAfterBuy;
		};
		std::vector<PlayerInfo> infos;
		infos.reserve(N);

		// Add N distinct players with valid purchases
		for (uint32 i = 0; i < N; i+=2)
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

		ctl.EndEpoch();

		// Players reset after epoch end
		EXPECT_EQ(ctl.state()->getPlayerCounter(), 0u);

		const RL::GetWinners_output winnersAfter = ctl.getWinners();
		EXPECT_EQ(winnersAfter.winnersCounter, winnersCountBefore + 1);

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

		// Burn (remaining on contract)
		const uint64 burnExpected = ctl.expectedRemainingAfterPayout(contractBalanceBefore, fees);
		EXPECT_EQ(getBalance(contractAddress), burnExpected);
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
	ctl.BeginEpoch();

	constexpr uint32 K = 3;
	for (uint32 i = 0; i < K; ++i)
	{
		const id user = id::randomValue();
		ctl.increaseAndBuy(ctl, user, ticketPrice);
		ctl.expectContractBalanceEqualsGetBalance(ctl, contractAddress);
	}

	// Before ending the epoch, balance equals the total cost of tickets
	{
		const RL::GetBalance_output outBefore = ctl.getBalanceInfo();
		EXPECT_EQ(outBefore.balance, ticketPrice * K);
	}

	// End epoch and verify expected remaining amount against contract balance and function output
	const uint64 contractBalanceBefore = getBalance(contractAddress);
	const RL::GetFees_output fees = ctl.getFees();

	ctl.EndEpoch();

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
	// Compare against the players array capacity, fetched via GetPlayers
	const RL::GetPlayers_output playersOut = ctl.getPlayers();
	EXPECT_EQ(static_cast<unsigned>(out.numberOfPlayers), static_cast<unsigned>(playersOut.players.capacity()));
}

TEST(ContractRandomLottery, GetState)
{
	ContractTestingRL ctl;

	// Initially LOCKED
	{
		const RL::GetState_output out0 = ctl.getStateInfo();
		EXPECT_EQ(out0.currentState, static_cast<uint8>(RL::EState::LOCKED));
	}

	// After BeginEpoch — SELLING
	ctl.BeginEpoch();
	{
		const RL::GetState_output out1 = ctl.getStateInfo();
		EXPECT_EQ(out1.currentState, static_cast<uint8>(RL::EState::SELLING));
	}

	// After EndEpoch — back to LOCKED
	ctl.EndEpoch();
	{
		const RL::GetState_output out2 = ctl.getStateInfo();
		EXPECT_EQ(out2.currentState, static_cast<uint8>(RL::EState::LOCKED));
	}
}
