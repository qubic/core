/**
 * @file RandomLottery.h
 * @brief Random Lottery contract definition: state, data structures, and user / internal
 * procedures.
 *
 * This header declares the RL (Random Lottery) contract which:
 *  - Sells tickets during a SELLING epoch.
 *  - Draws a pseudo-random winner when the epoch ends.
 *  - Distributes fees (team, distribution, burn, winner).
 *  - Records winners' history in a ring-like buffer.
 *
 * Notes:
 *  - Percentages must sum to <= 100; the remainder goes to the winner.
 *  - Players array stores one entry per ticket, so a single address can appear multiple times.
 *  - When only one player bought a ticket in the epoch, funds are refunded instead of drawing.
 */

using namespace QPI;

/// Maximum number of players allowed in the lottery for a single epoch (one entry == one ticket).
constexpr uint16 RL_MAX_NUMBER_OF_PLAYERS = 1024;

/// Maximum number of winners stored in the on-chain winners history ring buffer.
constexpr uint16 RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY = 1024;

/// Default ticket price (denominated in the smallest currency unit).
constexpr uint64 RL_TICKET_PRICE = 1000000;

/// Team fee percent of epoch revenue (0..100).
constexpr uint8 RL_TEAM_FEE_PERCENT = 10;

/// Distribution (shareholders/validators) fee percent of epoch revenue (0..100).
constexpr uint8 RL_SHAREHOLDER_FEE_PERCENT = 20;

/// Burn percent of epoch revenue (0..100).
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
		SELLING, // Ticket selling is open
		LOCKED,  // Ticket selling is closed

		INVALID = 255
	};

	/**
	 * @brief Standardized return / error codes for procedures.
	 */
	enum class EReturnCode : uint8
	{
		SUCCESS = 0,
		// Ticket-related errors
		TICKET_INVALID_PRICE = 1,  // Not enough funds to buy at least one ticket / price mismatch
		TICKET_ALL_SOLD_OUT = 2,   // No free slots left in players array
		TICKET_SELLING_CLOSED = 3, // Attempted to buy while state is LOCKED
		// Access-related errors
		ACCESS_DENIED = 4, // Caller is not authorized to perform the action

		UNKNOWN_ERROR = UINT8_MAX
	};

	struct NextEpochData
	{
		uint64 newPrice = 0; // Ticket price to apply after END_EPOCH; 0 means "no change queued"
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
		uint8 teamFeePercent = 0;         // Team share in percent
		uint8 distributionFeePercent = 0; // Distribution/shareholders share in percent
		uint8 winnerFeePercent = 0;       // Winner share in percent
		uint8 burnPercent = 0;            // Burn share in percent
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct GetPlayers_input
	{
	};

	struct GetPlayers_output
	{
		Array<id, RL_MAX_NUMBER_OF_PLAYERS> players; // Current epoch ticket holders (duplicates allowed)
		uint64 playerCounter = 0;                    // Actual count of filled entries
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	/**
	 * @brief Stored winner snapshot for an epoch.
	 */
	struct WinnerInfo
	{
		id winnerAddress = id::zero(); // Winner address
		uint64 revenue = 0;            // Payout value sent to the winner for that epoch
		uint16 epoch = 0;              // Epoch number when winner was recorded
		uint32 tick = 0;               // Tick when the decision was made
	};

	struct FillWinnersInfo_input
	{
		id winnerAddress = id::zero(); // Winner address to store
		uint64 revenue = 0;            // Winner payout to store
	};

	struct FillWinnersInfo_output
	{
	};

	struct FillWinnersInfo_locals
	{
		WinnerInfo winnerInfo = {}; // Temporary buffer to compose a WinnerInfo record
	};

	struct GetWinner_input
	{
	};

	struct GetWinner_output
	{
		id winnerAddress = id::zero(); // Selected winner address (id::zero if none)
		uint64 index = 0;              // Index into players array
	};

	struct GetWinner_locals
	{
		uint64 randomNum = 0; // Random index candidate in [0, playerCounter)
		sint64 i = 0;         // Reserved for future iteration logic
		uint64 j = 0;         // Reserved for future iteration logic
	};

	struct GetWinners_input
	{
	};

	struct GetWinners_output
	{
		Array<WinnerInfo, RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners; // Ring buffer snapshot
		uint64 winnersCounter = 0;                                      // Number of valid entries (bounded by capacity)
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct GetTicketPrice_input
	{
	};

	struct GetTicketPrice_output
	{
		uint64 ticketPrice = 0; // Current ticket price
	};

	struct GetMaxNumberOfPlayers_input
	{
	};

	struct GetMaxNumberOfPlayers_output
	{
		uint64 numberOfPlayers = 0; // Max capacity of players array
	};

	struct GetState_input
	{
	};

	struct GetState_output
	{
		uint8 currentState = static_cast<uint8>(EState::INVALID); // Current finite state of the lottery
	};

	struct GetBalance_input
	{
	};

	struct GetBalance_output
	{
		uint64 balance = 0; // Net balance (incoming - outgoing) for current epoch
	};

	// Local variables for GetBalance procedure
	struct GetBalance_locals
	{
		Entity entity = {}; // Entity accounting snapshot for SELF
	};

	// Local variables for BuyTicket procedure
	struct BuyTicket_locals
	{
		uint64 price = 0;        // Current ticket price
		uint64 reward = 0;       // Funds sent with call (invocationReward)
		uint64 capacity = 0;     // Max capacity of players array
		uint64 slotsLeft = 0;    // Remaining slots available to fill this epoch
		uint64 desired = 0;      // How many tickets the caller wants to buy
		uint64 remainder = 0;    // Change to return (reward % price)
		uint64 toBuy = 0;        // Actual number of tickets to purchase (bounded by slotsLeft)
		uint64 unfilled = 0;     // Portion of desired tickets not purchased due to capacity limit
		uint64 refundAmount = 0; // Total refund: remainder + unfilled * price
		uint64 i = 0;            // Loop counter
	};

	struct ReturnAllTickets_input
	{
	};
	struct ReturnAllTickets_output
	{
	};

	struct ReturnAllTickets_locals
	{
		uint64 i = 0; // Loop counter for mass-refund
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

		uint64 teamFee = 0;         // Team payout portion
		uint64 distributionFee = 0; // Distribution/shared payout portion
		uint64 winnerAmount = 0;    // Winner payout portion
		uint64 burnedAmount = 0;    // Burn portion

		uint64 revenue = 0; // Epoch revenue = incoming - outgoing
		Entity entity = {}; // Accounting snapshot

		sint32 i = 0; // Reserved
	};

	struct SetPrice_input
	{
		uint64 newPrice = 0; // New ticket price to be applied at the end of the epoch
	};

	struct SetPrice_output
	{
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
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
		REGISTER_USER_PROCEDURE(SetPrice, 2);
	}

	/**
	 * @brief Contract initialization hook.
	 * Sets default fees, ticket price, addresses, and locks the lottery (no selling yet).
	 */
	INITIALIZE()
	{
		// Set team/developer address (owner and team are the same for now)
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);
		state.ownerAddress = state.teamAddress;

		// Fee configuration (winner gets the remainder)
		state.teamFeePercent = RL_TEAM_FEE_PERCENT;
		state.distributionFeePercent = RL_SHAREHOLDER_FEE_PERCENT;
		state.burnPercent = RL_BURN_PERCENT;
		state.winnerFeePercent = 100 - state.teamFeePercent - state.distributionFeePercent - state.burnPercent;

		// Initial ticket price
		state.ticketPrice = RL_TICKET_PRICE;

		// Start in LOCKED state; selling must be explicitly opened with BEGIN_EPOCH
		state.currentState = EState::LOCKED;

		// Reset player counter
		state.playerCounter = 0;
	}

	/**
	 * @brief Opens ticket selling for a new epoch.
	 */
	BEGIN_EPOCH() { state.currentState = EState::SELLING; }

	/**
	 * @brief Closes epoch: computes revenue, selects winner (if >1 player),
	 * distributes fees, burns leftover, records winner, then clears players.
	 *
	 * Behavior:
	 *  - If exactly 1 player, refund ticket price (no draw).
	 *  - If >1 players, compute revenue, select winner with K12-based randomness,
	 *    split revenue into winner/team/distribution/burn, perform transfers/burn,
	 *    and store winner snapshot in the ring buffer.
	 *  - Apply deferred price change for the next epoch if queued.
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
			// Epoch revenue = incoming - outgoing for this contract
			qpi.getEntity(SELF, locals.entity);
			locals.revenue = locals.entity.incomingAmount - locals.entity.outgoingAmount;

			// Winner selection (pseudo-random using K12(prevSpectrumDigest)).
			GetWinner(qpi, state, locals.getWinnerInput, locals.getWinnerOutput, locals.getWinnerLocals);

			if (locals.getWinnerOutput.winnerAddress != id::zero())
			{
				// Split revenue by configured percentages
				locals.winnerAmount = div<uint64>(locals.revenue * state.winnerFeePercent, 100ULL);
				locals.teamFee = div<uint64>(locals.revenue * state.teamFeePercent, 100ULL);
				locals.distributionFee = div<uint64>(locals.revenue * state.distributionFeePercent, 100ULL);
				locals.burnedAmount = div<uint64>(locals.revenue * state.burnPercent, 100ULL);

				// Team payout
				if (locals.teamFee > 0)
				{
					qpi.transfer(state.teamAddress, locals.teamFee);
				}

				// Distribution payout (equal per validator)
				if (locals.distributionFee > 0)
				{
					qpi.distributeDividends(div<uint64>(locals.distributionFee, uint64(NUMBER_OF_COMPUTORS)));
				}

				// Winner payout
				if (locals.winnerAmount > 0)
				{
					qpi.transfer(locals.getWinnerOutput.winnerAddress, locals.winnerAmount);
				}

				// Burn configured portion
				if (locals.burnedAmount > 0)
				{
					qpi.burn(locals.burnedAmount);
				}

				// Store winner snapshot into history (ring buffer)
				locals.fillWinnersInfoInput.winnerAddress = locals.getWinnerOutput.winnerAddress;
				locals.fillWinnersInfoInput.revenue = locals.winnerAmount;
				FillWinnersInfo(qpi, state, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput, locals.fillWinnersInfoLocals);
			}
			else
			{
				// Fallback: if winner couldn't be selected (should not happen), refund all tickets
				ReturnAllTickets(qpi, state, locals.returnAllTicketsInput, locals.returnAllTicketsOutput, locals.returnAllTicketsLocals);
			}
		}

		// Prepare for next epoch: clear players and apply deferred price if any
		state.playerCounter = 0;

		if (state.nexEpochData.newPrice != 0)
		{
			state.ticketPrice = state.nexEpochData.newPrice;
			state.nexEpochData.newPrice = 0;
		}
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
		// Returns balance for current epoch (incoming - outgoing)
		output.balance = qpi.getEntity(SELF, locals.entity) ? locals.entity.incomingAmount - locals.entity.outgoingAmount : 0;
	}

	PUBLIC_PROCEDURE(SetPrice)
	{
		// Only team/owner can queue a price change
		if (qpi.invocator() != state.teamAddress)
		{
			output.returnCode = static_cast<uint8>(EReturnCode::ACCESS_DENIED);
			return;
		}

		// Zero price is invalid
		if (input.newPrice == 0)
		{
			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		// Defer application until END_EPOCH
		state.nexEpochData.newPrice = input.newPrice;
		output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Attempts to buy tickets while SELLING state is active.
	 * Logic:
	 *  - If locked: refund full invocationReward and return TICKET_SELLING_CLOSED.
	 *  - If reward < price: refund full reward and return TICKET_INVALID_PRICE.
	 *  - If no capacity left: refund full reward and return TICKET_ALL_SOLD_OUT.
	 *  - Otherwise: add up to slotsLeft tickets; refund remainder and unfilled part.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		locals.reward = qpi.invocationReward();

		// Selling closed: refund any attached funds and exit
		if (state.currentState == EState::LOCKED)
		{
			if (locals.reward > 0)
			{
				qpi.transfer(qpi.invocator(), locals.reward);
			}

			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_SELLING_CLOSED);
			return;
		}

		locals.price = state.ticketPrice;

		// Not enough to buy even a single ticket: refund everything
		if (locals.reward < locals.price)
		{
			if (locals.reward > 0)
			{
				qpi.transfer(qpi.invocator(), locals.reward);
			}

			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		// Capacity check
		locals.capacity = state.players.capacity();
		locals.slotsLeft = (state.playerCounter < locals.capacity) ? (locals.capacity - state.playerCounter) : 0;
		if (locals.slotsLeft == 0)
		{
			// All sold out: refund full amount
			if (locals.reward > 0)
			{
				qpi.transfer(qpi.invocator(), locals.reward);
			}
			output.returnCode = static_cast<uint8>(EReturnCode::TICKET_ALL_SOLD_OUT);
			return;
		}

		// Compute desired number of tickets and change
		locals.desired = locals.reward / locals.price;                // How many tickets the caller attempts to buy
		locals.remainder = locals.reward % locals.price;              // Change to return
		locals.toBuy = min<uint64>(locals.desired, locals.slotsLeft); // Do not exceed available slots

		// Add tickets (the same address may be inserted multiple times)
		for (locals.i = 0; locals.i < locals.toBuy; ++locals.i)
		{
			if (state.playerCounter < locals.capacity)
			{
				state.players.set(state.playerCounter, qpi.invocator());
				state.playerCounter = min<uint64>(state.playerCounter + 1, locals.capacity);
			}
		}

		// Refund change and unfilled portion (if desired > slotsLeft)
		locals.unfilled = locals.desired - locals.toBuy;
		locals.refundAmount = locals.remainder + (locals.unfilled * locals.price);
		if (locals.refundAmount > 0)
		{
			qpi.transfer(qpi.invocator(), locals.refundAmount);
		}

		output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	}

private:
	/**
	 * @brief Internal: records a winner into the cyclic winners array.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo)
	{
		if (input.winnerAddress == id::zero())
		{
			return; // Nothing to store
		}

		// Use ring-buffer indexing to avoid overflow (overwrite oldest entries)
		state.winnersCounter = mod<uint64>(state.winnersCounter, state.winners.capacity());

		locals.winnerInfo.winnerAddress = input.winnerAddress;
		locals.winnerInfo.revenue = input.revenue;
		locals.winnerInfo.epoch = qpi.epoch();
		locals.winnerInfo.tick = qpi.tick();
		state.winners.set(state.winnersCounter, locals.winnerInfo);
		++state.winnersCounter;
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

		// Compute pseudo-random index based on K12(prevSpectrumDigest)
		locals.randomNum = mod<uint64>(qpi.K12(qpi.getPrevSpectrumDigest()).u64._0, state.playerCounter);

		// Index directly into players array
		output.winnerAddress = state.players.get(locals.randomNum);
		output.index = locals.randomNum;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ReturnAllTickets)
	{
		// Refund ticket price to each recorded player (one transfer per ticket entry)
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

	/**
	 * @brief Number of players (tickets sold) in the current epoch.
	 */
	uint64 playerCounter = 0;

	/**
	 * @brief Data structure for deferred changes to apply at the end of the epoch.
	 */
	NextEpochData nexEpochData = {};

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
