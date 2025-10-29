﻿/**
 * @file RandomLottery.h
 * @brief Random Lottery contract definition: state, data structures, and user / internal
 * procedures.
 *
 * This header declares the RL (Random Lottery) contract which:
 *  - Sells tickets during a SELLING epoch.
 *  - Draws a pseudo-random winner when the epoch ends or at scheduled intra-epoch draws.
 *  - Distributes fees (team, distribution, burn, winner).
 *  - Records winners' history in a ring-like buffer.
 *
 * Notes:
 *  - Percentages must sum to <= 100; the remainder goes to the winner.
 *  - Players array stores one entry per ticket, so a single address can appear multiple times.
 *  - When only one player bought a ticket in the epoch, funds are refunded instead of drawing.
 *  - Day-of-week mapping used here is 0..6 where 0 = WEDNESDAY, 1 = THURSDAY, ..., 6 = TUESDAY.
 *  - Schedule uses a 7-bit mask aligned to the mapping above (bit 0 -> WEDNESDAY, bit 6 -> TUESDAY).
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

/// Sentinel for "no valid day".
constexpr uint8 RL_INVALID_DAY = 255;

/// Sentinel for "no valid hour".
constexpr uint8 RL_INVALID_HOUR = 255;

/// Throttling period: process BEGIN_TICK logic once per this many ticks.
constexpr uint8 RL_TICK_UPDATE_PERIOD = 100;

/// Default draw hour (UTC).
constexpr uint8 RL_DEFAULT_DRAW_HOUR = 11; // 11:00 UTC

namespace RLUtils
{
	// Returns current day-of-week in range [0..6], with 0 = WEDNESDAY according to platform mapping.
	static void getCurrentDayOfWeek(const QpiContextFunctionCall& qpi, uint8& dayOfWeek)
	{
		dayOfWeek = qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day());
	}

	// Packs current date into a compact stamp (Y/M/D) used to ensure a single action per calendar day.
	static void makeDateStamp(const QpiContextFunctionCall& qpi, uint32& res)
	{
		res = static_cast<uint32>(qpi.year() << 9 | qpi.month() << 5 | qpi.day());
	}

	// Reads current net on-chain balance of SELF (incoming - outgoing).
	static void getSCRevenue(const QpiContextFunctionCall& qpi, Entity& entity, uint64& revenue)
	{
		qpi.getEntity(SELF, entity);
		revenue = entity.incomingAmount - entity.outgoingAmount;
	}

	template<typename T> static constexpr const T& min(const T& a, const T& b)
	{
		return (a < b) ? a : b;
	}
}; // namespace RLUtils

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

		INVALID = UINT8_MAX
	};

	/**
	 * @brief Standardized return / error codes for procedures.
	 */
	enum class EReturnCode : uint8
	{
		SUCCESS,

		// Ticket-related errors
		TICKET_INVALID_PRICE,  // Not enough funds to buy at least one ticket / price mismatch
		TICKET_ALL_SOLD_OUT,   // No free slots left in players array
		TICKET_SELLING_CLOSED, // Attempted to buy while state is LOCKED
		// Access-related errors
		ACCESS_DENIED, // Caller is not authorized to perform the action

		// Value-related errors
		INVALID_VALUE, // Input value is not acceptable

		UNKNOWN_ERROR = UINT8_MAX
	};

	struct NextEpochData
	{
		uint64 newPrice = 0; // Ticket price to apply after END_EPOCH; 0 means "no change queued"
		uint8 schedule = 0;  // Schedule bitmask (bit 0 = WEDNESDAY, ..., bit 6 = TUESDAY); applied after END_EPOCH
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
		id winnerAddress = id::zero();    // Winner address
		uint64 revenue = 0;               // Payout value sent to the winner for that epoch
		uint32 tick = 0;                  // Tick when the decision was made
		uint16 epoch = 0;                 // Epoch number when winner was recorded
		uint8 dayOfWeek = RL_INVALID_DAY; // Day of week when the winner was drawn [0..6] 0 = WEDNESDAY
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
		uint64 insertIdx = 0;       // Index in ring buffer where to insert new winner
	};

	struct GetWinners_input
	{
	};

	struct GetWinners_output
	{
		Array<WinnerInfo, RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners; // Ring buffer snapshot
		uint64 winnersCounter = 0;                                      // Number of valid entries = (totalWinners % capacity)
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
		uint64 balance = 0; // Current contract net balance (incoming - outgoing)
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

	struct SetPrice_input
	{
		uint64 newPrice = 0; // New ticket price to be applied at the end of the epoch
	};

	struct SetPrice_output
	{
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct SetSchedule_input
	{
		uint8 newSchedule = 0; // New schedule bitmask to be applied at the end of the epoch
	};

	struct SetSchedule_output
	{
		uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
	};

	struct BEGIN_TICK_locals
	{
		id winnerAddress = id::zero();
		Entity entity = {};
		uint64 revenue = 0;
		uint64 randomNum = 0;
		uint64 winnerAmount = 0;
		uint64 teamFee = 0;
		uint64 distributionFee = 0;
		uint64 burnedAmount = 0;
		FillWinnersInfo_locals fillWinnersInfoLocals = {};
		FillWinnersInfo_input fillWinnersInfoInput = {};
		uint32 currentDateStamp = 0;
		uint8 currentDayOfWeek = RL_INVALID_DAY;
		uint8 currentHour = RL_INVALID_HOUR;
		uint8 isWednesday = 0;
		uint8 isScheduledToday = 0;
		ReturnAllTickets_locals returnAllTicketsLocals = {};
		ReturnAllTickets_input returnAllTicketsInput = {};
		ReturnAllTickets_output returnAllTicketsOutput = {};
		FillWinnersInfo_output fillWinnersInfoOutput = {};
	};

	struct GetNextEpochData_input
	{
	};

	struct GetNextEpochData_output
	{
		NextEpochData nextEpochData = {};
	};

	struct GetDrawHour_input
	{
	};

	struct GetDrawHour_output
	{
		uint8 drawHour = RL_INVALID_HOUR;
	};

	// New: expose current schedule mask
	struct GetSchedule_input
	{
	};
	struct GetSchedule_output
	{
		uint8 schedule = 0;
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
		REGISTER_USER_FUNCTION(GetNextEpochData, 8);
		REGISTER_USER_FUNCTION(GetDrawHour, 9);
		REGISTER_USER_FUNCTION(GetSchedule, 10);
		REGISTER_USER_PROCEDURE(BuyTicket, 1);
		REGISTER_USER_PROCEDURE(SetPrice, 2);
		REGISTER_USER_PROCEDURE(SetSchedule, 3);
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

		// Default schedule: WEDNESDAY
		state.schedule = 1 << WEDNESDAY;
	}

	/**
	 * @brief Opens ticket selling for a new epoch.
	 */
	BEGIN_EPOCH()
	{
		if (state.schedule == 0)
		{
			// Default to WEDNESDAY if no schedule is set (bit 0)
			state.schedule = 1 << WEDNESDAY;
		}

		if (state.drawHour == 0)
		{
			state.drawHour = RL_DEFAULT_DRAW_HOUR; // Default draw hour (UTC)
		}

		// Mark the current date as already processed to avoid immediate draw on the same calendar day
		RLUtils::getCurrentDayOfWeek(qpi, state.lastDrawDay);
		state.lastDrawHour = state.drawHour;
		RLUtils::makeDateStamp(qpi, state.lastDrawDateStamp);

		// Open selling for the new epoch
		enableBuyTicket(state, true);
	}

	END_EPOCH()
	{
		enableBuyTicket(state, false);

		clearStateOnEndEpoch(state);
		applyNextEpochData(state);
	}

	BEGIN_TICK_WITH_LOCALS()
	{
		// Throttle: run logic only once per RL_TICK_UPDATE_PERIOD ticks
		if (mod(qpi.tick(), static_cast<uint32>(RL_TICK_UPDATE_PERIOD)) != 0)
		{
			return;
		}

		// Snapshot current day/hour
		RLUtils::getCurrentDayOfWeek(qpi, locals.currentDayOfWeek);
		locals.currentHour = qpi.hour();

		// Do nothing before the configured draw hour
		if (locals.currentHour < state.drawHour)
		{
			return;
		}

		// Ensure only one action per calendar day (UTC)
		RLUtils::makeDateStamp(qpi, locals.currentDateStamp);
		if (state.lastDrawDateStamp == locals.currentDateStamp)
		{
			return;
		}

		locals.isWednesday = (locals.currentDayOfWeek == WEDNESDAY);
		locals.isScheduledToday = ((state.schedule & (1u << locals.currentDayOfWeek)) != 0);

		// Two-Wednesdays rule:
		// - First Wednesday (epoch start) is "consumed" in BEGIN_EPOCH (we set lastDrawDateStamp),
		// - Any subsequent Wednesday performs a draw and leaves selling CLOSED until next BEGIN_EPOCH,
		// - Any other day performs a draw only if included in schedule and then re-opens selling.
		if (!locals.isWednesday && !locals.isScheduledToday)
		{
			return; // Non-Wednesday day that is not scheduled: nothing to do
		}

		// Mark today's action and timestamp
		state.lastDrawDay = locals.currentDayOfWeek;
		state.lastDrawHour = locals.currentHour;
		state.lastDrawDateStamp = locals.currentDateStamp;

		// Temporarily close selling for the draw
		enableBuyTicket(state, false);

		// Draw
		{
			if (state.playerCounter <= 1)
			{
				ReturnAllTickets(qpi, state, locals.returnAllTicketsInput, locals.returnAllTicketsOutput, locals.returnAllTicketsLocals);
			}
			else
			{
				// Current contract net balance = incoming - outgoing for this contract
				RLUtils::getSCRevenue(qpi, locals.entity, locals.revenue);

				// Winner selection (pseudo-random using K12(prevSpectrumDigest)).
				getRandomPlayer(state, qpi, locals.randomNum, locals.winnerAddress);

				if (locals.winnerAddress != id::zero())
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
						qpi.distributeDividends(div(locals.distributionFee, static_cast<uint64>(NUMBER_OF_COMPUTORS)));
					}

					// Winner payout
					if (locals.winnerAmount > 0)
					{
						qpi.transfer(locals.winnerAddress, locals.winnerAmount);
					}

					// Burn configured portion
					if (locals.burnedAmount > 0)
					{
						qpi.burn(locals.burnedAmount);
					}

					// Store winner snapshot into history (ring buffer)
					locals.fillWinnersInfoInput.winnerAddress = locals.winnerAddress;
					locals.fillWinnersInfoInput.revenue = locals.winnerAmount;
					FillWinnersInfo(qpi, state, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput, locals.fillWinnersInfoLocals);
				}
				else
				{
					// Fallback: if winner couldn't be selected (should not happen), refund all tickets
					ReturnAllTickets(qpi, state, locals.returnAllTicketsInput, locals.returnAllTicketsOutput, locals.returnAllTicketsLocals);
				}
			}
		}

		clearStateOnEndDraw(state);

		// Resume selling unless today is Wednesday (remains closed until next epoch)
		enableBuyTicket(state, !locals.isWednesday);
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
		output.playerCounter = RLUtils::min(state.playerCounter, state.players.capacity());
	}

	/**
	 * @brief Returns historical winners (ring buffer segment).
	 */
	PUBLIC_FUNCTION(GetWinners)
	{
		output.winners = state.winners;
		getWinnerCounter(state, output.winnersCounter);
	}

	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	PUBLIC_FUNCTION(GetMaxNumberOfPlayers) { output.numberOfPlayers = RL_MAX_NUMBER_OF_PLAYERS; }
	PUBLIC_FUNCTION(GetState) { output.currentState = static_cast<uint8>(state.currentState); }
	PUBLIC_FUNCTION(GetNextEpochData) { output.nextEpochData = state.nexEpochData; }
	PUBLIC_FUNCTION(GetDrawHour) { output.drawHour = state.drawHour; }
	PUBLIC_FUNCTION(GetSchedule) { output.schedule = state.schedule; }
	PUBLIC_FUNCTION_WITH_LOCALS(GetBalance) { RLUtils::getSCRevenue(qpi, locals.entity, output.balance); }

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

	PUBLIC_PROCEDURE(SetSchedule)
	{
		if (qpi.invocator() != state.teamAddress)
		{
			output.returnCode = static_cast<uint8>(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newSchedule == 0)
		{
			output.returnCode = static_cast<uint8>(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nexEpochData.schedule = input.newSchedule;
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
		locals.desired = div(locals.reward, locals.price);             // How many tickets the caller attempts to buy
		locals.remainder = mod(locals.reward, locals.price);           // Change to return
		locals.toBuy = RLUtils::min(locals.desired, locals.slotsLeft); // Do not exceed available slots

		// Add tickets (the same address may be inserted multiple times)
		for (locals.i = 0; locals.i < locals.toBuy; ++locals.i)
		{
			if (state.playerCounter < locals.capacity)
			{
				state.players.set(state.playerCounter, qpi.invocator());
				state.playerCounter = RLUtils::min(state.playerCounter + 1, locals.capacity);
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
	 * Overwrites oldest entries when capacity is exceeded (ring buffer).
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo)
	{
		if (input.winnerAddress == id::zero())
		{
			return; // Nothing to store
		}

		// Compute ring-buffer index without clamping the total counter
		getWinnerCounter(state, locals.insertIdx);
		++state.winnersCounter;

		locals.winnerInfo.winnerAddress = input.winnerAddress;
		locals.winnerInfo.revenue = input.revenue;
		locals.winnerInfo.epoch = qpi.epoch();
		locals.winnerInfo.tick = qpi.tick();
		RLUtils::getCurrentDayOfWeek(qpi, locals.winnerInfo.dayOfWeek);

		state.winners.set(locals.insertIdx, locals.winnerInfo);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ReturnAllTickets)
	{
		// Refund ticket price to each recorded ticket entry (1 transfer per entry)
		for (locals.i = 0; locals.i < state.playerCounter; ++locals.i)
		{
			qpi.transfer(state.players.get(locals.i), state.ticketPrice);
		}
	}

protected:
	/**
	 * @brief Circular buffer storing the history of winners.
	 * Maximum capacity is defined by RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY.
	 */
	Array<WinnerInfo, RL_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners = {};

	/**
	 * @brief Set of players participating in the current lottery epoch.
	 * Maximum capacity is defined by RL_MAX_NUMBER_OF_PLAYERS.
	 */
	Array<id, RL_MAX_NUMBER_OF_PLAYERS> players = {};

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
	 * @brief Data structure for deferred changes to apply at the end of the epoch.
	 */
	NextEpochData nexEpochData = {};

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
	 * @brief Index pointing to the next empty slot in the winners array.
	 * Used for maintaining the circular buffer of winners.
	 */
	uint64 winnersCounter = 0;

	/**
	 * @brief Date/time guard for draw operations.
	 * lastDrawDateStamp prevents more than one action per calendar day (UTC).
	 */
	uint8 lastDrawDay = RL_INVALID_DAY;
	uint8 lastDrawHour = RL_INVALID_HOUR;
	uint32 lastDrawDateStamp = 0; // Compact YYYY/MM/DD marker

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
	 * @brief Schedule bitmask: bit 0 = WEDNESDAY, 1 = THURSDAY, ..., 6 = TUESDAY.
	 * If a bit is set, a draw may occur on that day (subject to drawHour and daily guard).
	 * Wednesday also follows the "Two-Wednesdays rule" (selling stays closed after Wednesday draw).
	 */
	uint8 schedule = 0;

	/**
	 * @brief UTC hour [0..23] when a draw is allowed to run (daily time gate).
	 */
	uint8 drawHour = 0;

	/**
	 * @brief Current state of the lottery contract.
	 * SELLING: tickets available; LOCKED: selling closed.
	 */
	EState currentState = EState::LOCKED;

protected:
	static void clearStateOnEndEpoch(RL& state)
	{
		// Prepare for next epoch: clear players and reset daily guards
		state.playerCounter = 0;

		state.lastDrawHour = RL_INVALID_HOUR;
		state.lastDrawDay = RL_INVALID_DAY;
		state.lastDrawDateStamp = 0;
	}

	static void clearStateOnEndDraw(RL& state)
	{
		// After each draw period, clear current tickets
		state.playerCounter = 0;
	}

	static void applyNextEpochData(RL& state)
	{
		// Apply deferred ticket price (if any)
		if (state.nexEpochData.newPrice != 0)
		{
			state.ticketPrice = state.nexEpochData.newPrice;
			state.nexEpochData.newPrice = 0;
		}

		// Apply deferred schedule (if any)
		if (state.nexEpochData.schedule != 0)
		{
			state.schedule = state.nexEpochData.schedule;
			state.nexEpochData.schedule = 0;
		}
	}

	static void enableBuyTicket(RL& state, bool bEnable) { state.currentState = bEnable ? EState::SELLING : EState::LOCKED; }

	static void getRandomPlayer(const RL& state, const QpiContextProcedureCall& qpi, uint64& randomNum, id& winnerAddress)
	{
		winnerAddress = id::zero();

		if (state.playerCounter == 0)
		{
			return;
		}

		// Compute pseudo-random index based on K12(prevSpectrumDigest)
		randomNum = mod(qpi.K12(qpi.getPrevSpectrumDigest()).u64._0, state.playerCounter);

		// Index directly into players array
		winnerAddress = state.players.get(randomNum);
	}

	static void getWinnerCounter(const RL& state, uint64& outCounter) { outCounter = mod(state.winnersCounter, state.winners.capacity()); }
};
