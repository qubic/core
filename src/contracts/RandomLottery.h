﻿/**
 * @file RandomLottery.h
 * @brief Random Lottery contract definition: state, data structures, and user / internal
 * procedures.
 *
 * This header declares the RL (Random Lottery) contract which:
 *  - Sells tickets during a SELLING epoch.
 *  - Draws a pseudo-random winner when the epoch ends.
 *  - Distributes fees (team, distribution, burn, winner).
 *  - Records winners' history in a ring-like buffer.
 */

using namespace QPI;

/// Maximum number of players allowed in the lottery.
constexpr uint16 RL_MAX_NUMBER_OF_PLAYERS = 1024;

/// Maximum number of winners kept in the on-chain winners history buffer.
constexpr uint16 RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY = 1024;

constexpr uint64 RL_TICKET_PRICE = 1000000;

constexpr uint8 RL_TEAM_FEE_PERCENT = 10;

constexpr uint8 RL_SHAREHOLDER_FEE_PERCENT = 20;

constexpr uint8 RL_BURN_PERCENT = 2;

/// Placeholder structure for future extensions.
struct RL2
{
};

/**
 * @brief Main contract class implementing the random lottery mechanics.
 *
 * Lifecycle:
 *  1. INITIALIZE sets defaults (fees, ticket price, state LOCKED).
 *  2. BEGIN_EPOCH opens ticket selling (SELLING).
 *  3. Users call BuyTicket while SELLING.
 *  4. END_EPOCH closes, computes fees, selects winner, distributes, burns rest.
 *  5. Players list is cleared for next epoch.
 */
struct RL : public ContractBase
{
public:
	/**
	 * @brief High-level finite state of the lottery.
	 * SELLING: tickets can be purchased.
	 * LOCKED: purchases closed; waiting for epoch transition.
	 */
	enum class EState : uint8
	{
		SELLING,
		LOCKED,

		INVALID = 255
	};

	/**
	 * @brief Standardized return / error codes for procedures.
	 */
	enum class EReturnCode : uint8
	{
		SUCCESS = 0,
		// Ticket-related errors
		TICKET_INVALID_PRICE = 1,
		TICKET_ALL_SOLD_OUT = 2,
		TICKET_SELLING_CLOSED = 3,
		// Access-related errors
		ACCESS_DENIED = 4,
		// Fee-related errors
		FEE_INVALID_PERCENT_VALUE = 5,
		// Fallback
		UNKNOW_ERROR = UINT8_MAX
	};

	//---- User-facing I/O structures -------------------------------------------------------------

	struct BuyTicket_input
	{
	};

	struct BuyTicket_output
	{
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct GetFees_input
	{
	};

	struct GetFees_output
	{
		uint8 teamFeePercent = 0;
		uint8 distributionFeePercent = 0;
		uint8 winnerFeePercent = 0;
		uint8 burnPercent = 0;
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct GetPlayers_input
	{
	};

	struct GetPlayers_output
	{
		Array<id, RL_MAX_NUMBER_OF_PLAYERS> players;
		uint64 playerCounter = 0;
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	/**
	 * @brief Stored winner snapshot for an epoch.
	 */
	struct WinnerInfo
	{
		id winnerAddress = id::zero();
		uint64 revenue = 0;
		uint16 epoch = 0;
		uint32 tick = 0;
	};

	struct FillWinnersInfo_input
	{
		id winnerAddress = id::zero();
		uint64 revenue = 0;
	};

	struct FillWinnersInfo_output
	{
	};

	struct FillWinnersInfo_locals
	{
		WinnerInfo winnerInfo = {};
	};

	struct GetWinner_input
	{
	};

	struct GetWinner_output
	{
		id winnerAddress = id::zero();
		uint64 index = 0;
	};

	struct GetWinner_locals
	{
		uint64 randomNum = 0;
		sint64 i = 0;
		uint64 j = 0;
	};

	struct GetWinners_input
	{
	};

	struct GetWinners_output
	{
		Array<WinnerInfo, RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners;
		uint64 winnersCounter = 0;
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct GetTicketPrice_input
	{
	};

	struct GetTicketPrice_output
	{
		uint64 ticketPrice = 0;
	};

	struct GetMaxNumberOfPlayers_input
	{
	};

	struct GetMaxNumberOfPlayers_output
	{
		uint64 numberOfPlayers = 0;
	};

	struct GetState_input
	{
	};

	struct GetState_output
	{
		uint8 currentState = static_cast<uint8>(EState::INVALID);
	};

	struct GetBalance_input
	{
	};

	struct GetBalance_output
	{
		uint64 balance = 0;
	};

	struct GetBalance_locals
	{
		Entity entity = {};
	};

	struct ReturnAllTickets_input
	{
	};
	struct ReturnAllTickets_output
	{
	};

	struct ReturnAllTickets_locals
	{
		sint64 i = 0;
	};

	struct END_EPOCH_locals
	{
		GetWinner_input getWinnerInput = {};
		GetWinner_output getWinnerOutput = {};
		GetWinner_locals getWinnerLocals = {};

		FillWinnersInfo_input fillWinnersInfoInput = {};
		FillWinnersInfo_output fillWinnersInfoOutput = {};
		FillWinnersInfo_locals fillWinnersInfoLocals = {};

		ReturnAllTickets_input returnAllTicketsInput = {};
		ReturnAllTickets_output returnAllTicketsOutput = {};
		ReturnAllTickets_locals returnAllTicketsLocals = {};

		uint64 teamFee = 0;
		uint64 distributionFee = 0;
		uint64 winnerAmount = 0;
		uint64 burnedAmount = 0;

		uint64 revenue = 0;
		Entity entity = {};

		sint32 i = 0;
	};

public:
	/**
	 * @brief Registers all externally callable functions and procedures with their numeric
	 * identifiers. Mapping numbers must remain stable to preserve external interface compatibility.
	 */
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetFees, 1);
		REGISTER_USER_FUNCTION(GetPlayers, 2);
		REGISTER_USER_FUNCTION(GetWinners, 3);
		REGISTER_USER_FUNCTION(GetTicketPrice, 4);
		REGISTER_USER_FUNCTION(GetMaxNumberOfPlayers, 5);
		REGISTER_USER_FUNCTION(GetState, 6);
		REGISTER_USER_FUNCTION(GetBalance, 7);
		REGISTER_USER_PROCEDURE(BuyTicket, 1);
	}

	/**
	 * @brief Contract initialization hook.
	 * Sets default fees, ticket price, addresses, and locks the lottery (no selling yet).
	 */
	INITIALIZE()
	{
		// Addresses
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);
		// Owner address (currently identical to developer address; can be split in future revisions).
		state.ownerAddress = state.teamAddress;

		// Default fee percentages (sum <= 100; winner percent derived)
		state.teamFeePercent = RL_TEAM_FEE_PERCENT;
		state.distributionFeePercent = RL_SHAREHOLDER_FEE_PERCENT;
		state.burnPercent = RL_BURN_PERCENT;
		state.winnerFeePercent = 100 - state.teamFeePercent - state.distributionFeePercent - state.burnPercent;

		// Default ticket price
		state.ticketPrice = RL_TICKET_PRICE;

		// Start locked
		state.currentState = EState::LOCKED;

		// Initialize Player index
		state.playerCounter = 0;
	}

	/**
	 * @brief Opens ticket selling for a new epoch.
	 */
	BEGIN_EPOCH() { state.currentState = EState::SELLING; }

	/**
	 * @brief Closes epoch: computes revenue, selects winner (if >1 player),
	 * distributes fees, burns leftover, records winner, then clears players.
	 */
	END_EPOCH_WITH_LOCALS()
	{
		state.currentState = EState::LOCKED;

		// Single-player edge case: refund instead of drawing.
		if (state.playerCounter == 1)
		{
			ReturnAllTickets(qpi, state, locals.returnAllTicketsInput, locals.returnAllTicketsOutput, locals.returnAllTicketsLocals);
		}
		else if (state.playerCounter > 1)
		{
			qpi.getEntity(SELF, locals.entity);
			locals.revenue = locals.entity.incomingAmount - locals.entity.outgoingAmount;

			// Winner selection (pseudo-random).
			GetWinner(qpi, state, locals.getWinnerInput, locals.getWinnerOutput, locals.getWinnerLocals);

			if (locals.getWinnerOutput.winnerAddress != id::zero())
			{
				// Fee splits
				locals.winnerAmount = div<uint64>(locals.revenue * state.winnerFeePercent, 100ULL);
				locals.teamFee = div<uint64>(locals.revenue * state.teamFeePercent, 100ULL);
				locals.distributionFee = div<uint64>(locals.revenue * state.distributionFeePercent, 100ULL);
				locals.burnedAmount = div<uint64>(locals.revenue * state.burnPercent, 100ULL);

				// Team fee
				if (locals.teamFee > 0)
				{
					qpi.transfer(state.teamAddress, locals.teamFee);
				}

				// Distribution fee
				if (locals.distributionFee > 0)
				{
					qpi.distributeDividends(div<uint64>(locals.distributionFee, uint64(NUMBER_OF_COMPUTORS)));
				}

				// Winner payout
				if (locals.winnerAmount > 0)
				{
					qpi.transfer(locals.getWinnerOutput.winnerAddress, locals.winnerAmount);
				}

				// Burn remainder
				if (locals.burnedAmount > 0)
				{
					qpi.burn(locals.burnedAmount);
				}

				// Persist winner record
				locals.fillWinnersInfoInput.winnerAddress = locals.getWinnerOutput.winnerAddress;
				locals.fillWinnersInfoInput.revenue = locals.winnerAmount;
				FillWinnersInfo(qpi, state, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput, locals.fillWinnersInfoLocals);
			}
			else
			{
				// Return funds to players if no winner could be selected (should be impossible).
				ReturnAllTickets(qpi, state, locals.returnAllTicketsInput, locals.returnAllTicketsOutput, locals.returnAllTicketsLocals);
			}
		}

		// Prepare for next epoch.
		state.playerCounter = 0;
	}

	/**
	 * @brief Returns currently configured fee percentages.
	 */
	PUBLIC_FUNCTION(GetFees)
	{
		output.teamFeePercent = state.teamFeePercent;
		output.distributionFeePercent = state.distributionFeePercent;
		output.winnerFeePercent = state.winnerFeePercent;
		output.burnPercent = state.burnPercent;
	}

	/**
	 * @brief Retrieves the active players list for the ongoing epoch.
	 */
	PUBLIC_FUNCTION(GetPlayers)
	{
		output.players = state.players;
		output.playerCounter = min<uint64>(state.playerCounter, state.players.capacity());
	}

	/**
	 * @brief Returns historical winners (ring buffer segment).
	 */
	PUBLIC_FUNCTION(GetWinners)
	{
		output.winners = state.winners;
		output.winnersCounter = min<uint64>(state.winnersCounter, state.winners.capacity());
	}

	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	PUBLIC_FUNCTION(GetMaxNumberOfPlayers) { output.numberOfPlayers = RL_MAX_NUMBER_OF_PLAYERS; }
	PUBLIC_FUNCTION(GetState) { output.currentState = static_cast<uint8>(state.currentState); }
	PUBLIC_FUNCTION_WITH_LOCALS(GetBalance)
	{
		output.balance = qpi.getEntity(SELF, locals.entity) ? locals.entity.incomingAmount - locals.entity.outgoingAmount : 0;
	}

	/**
	 * @brief Attempts to buy a ticket (must send exact price unless zero is forbidden; state must
	 * be SELLING). Reverts with proper return codes for invalid cases.
	 */
	PUBLIC_PROCEDURE(BuyTicket)
	{
		// Selling closed
		if (state.currentState == EState::LOCKED)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_SELLING_CLOSED);
			return;
		}

		// Price mismatch (validate before any state mutation)
		if (qpi.invocationReward() != state.ticketPrice && qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		// Capacity full
		if (state.playerCounter >= state.players.capacity())
		{
			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_ALL_SOLD_OUT);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// Protect against rewriting existing players (should not happen due to prior checks).
		if (state.playerCounter < state.players.capacity())
		{
			state.players.set(state.playerCounter, qpi.invocator());
			state.playerCounter = min<uint64>(state.playerCounter + 1, state.players.capacity());
		}
	}

private:
	/**
	 * @brief Internal: records a winner into the cyclic winners array.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo)
	{
		if (input.winnerAddress == id::zero())
		{
			return;
		}

		state.winnersCounter = mod<uint64>(state.winnersCounter, state.winners.capacity());

		locals.winnerInfo.winnerAddress = input.winnerAddress;
		locals.winnerInfo.revenue = input.revenue;
		locals.winnerInfo.epoch = qpi.epoch();
		locals.winnerInfo.tick = qpi.tick();
		state.winners.set(state.winnersCounter++, locals.winnerInfo);
	}

	/**
	 * @brief Internal: pseudo-random selection of a winner index using hardware RNG.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(GetWinner)
	{
		if (state.playerCounter == 0)
		{
			return;
		}

		locals.randomNum = mod<uint64>(qpi.K12(qpi.getPrevSpectrumDigest()).u64._0, state.playerCounter);

		// Direct indexing for Array
		output.winnerAddress = state.players.get(locals.randomNum);
		output.index = locals.randomNum;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ReturnAllTickets)
	{
		for (locals.i = 0; locals.i < state.playerCounter; ++locals.i)
		{
			qpi.transfer(state.players.get(locals.i), state.ticketPrice);
		}
	}

protected:
	/**
	 * @brief Address of the team managing the lottery contract.
	 * Initialized to a zero address.
	 */
	id teamAddress = id::zero();

	/**
	 * @brief Address of the owner of the lottery contract.
	 * Initialized to a zero address.
	 */
	id ownerAddress = id::zero();

	/**
	 * @brief Percentage of the revenue allocated to the team.
	 * Value is between 0 and 100.
	 */
	uint8 teamFeePercent = 0;

	/**
	 * @brief Percentage of the revenue allocated for distribution.
	 * Value is between 0 and 100.
	 */
	uint8 distributionFeePercent = 0;

	/**
	 * @brief Percentage of the revenue allocated to the winner.
	 * Automatically calculated as the remainder after other fees.
	 */
	uint8 winnerFeePercent = 0;

	/**
	 * @brief Percentage of the revenue to be burned.
	 * Value is between 0 and 100.
	 */
	uint8 burnPercent = 0;

	/**
	 * @brief Price of a single lottery ticket.
	 * Value is in the smallest currency unit (e.g., cents).
	 */
	uint64 ticketPrice = 0;

	uint64 playerCounter = 0;

	/**
	 * @brief Set of players participating in the current lottery epoch.
	 * Maximum capacity is defined by RL_MAX_NUMBER_OF_PLAYERS.
	 */
	Array<id, RL_MAX_NUMBER_OF_PLAYERS> players = {};

	/**
	 * @brief Circular buffer storing the history of winners.
	 * Maximum capacity is defined by RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY.
	 */
	Array<WinnerInfo, RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners = {};

	/**
	 * @brief Index pointing to the next empty slot in the winners array.
	 * Used for maintaining the circular buffer of winners.
	 */
	uint64 winnersCounter = 0;

	/**
	 * @brief Current state of the lottery contract.
	 * Can be either SELLING (tickets available) or LOCKED (epoch closed).
	 */
	EState currentState = EState::LOCKED;

protected:
	template<typename T> static constexpr const T& min(const T& a, const T& b) { return (a < b) ? a : b; }
};
