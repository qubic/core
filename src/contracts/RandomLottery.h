#pragma once

/**
 * @file RandomLottery.h
 * @brief Random Lottery contract definition: state, data structures, and user / internal procedures.
 *
 * This header declares the RL (Random Lottery) contract which:
 *  - Sells tickets during a SELLING epoch.
 *  - Draws a pseudo-random winner when the epoch ends.
 *  - Distributes fees (team, distribution, burn, winner).
 *  - Records winners' history in a ring-like buffer.
 */

using namespace QPI;

// Maximum number of players allowed in the lottery.
constexpr uint16 MAX_NUMBER_OF_PLAYERS = 1024;

/// Maximum number of winners kept in the on-chain winners history buffer.
constexpr uint16 MAX_NUMBER_OF_WINNERS_IN_HISTORY = 1024;

/**
 * @brief Developer address for the RandomLottery contract.
 *
 * IMPORTANT:
 *  The macro ID and the individual token macros (_Z, _T, _Q, etc.) must be available.
 *  If clang reports 'ID' undeclared here, include the QPI identity / address utilities first.
 */
static const id RL_DEV_ADDRESS = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L,
                                    _A,
                                    _K, _F, _T, _D, _X, _C, _R, _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y,
                                    _Q,
                                    _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E, _U, _E, _J, _J);

/// Owner address (currently identical to developer address; can be split in future revisions).
static const id RL_OWNER_ADDRESS = RL_DEV_ADDRESS;

/// Placeholder structure for future extensions.
struct RL2 {
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
struct RL : public ContractBase {
public:
    /**
     * @brief High-level finite state of the lottery.
     * SELLING: tickets can be purchased.
     * LOCKED: purchases closed; waiting for epoch transition.
     */
    enum class EState : uint8 {
        SELLING,
        LOCKED
    };

    /**
     * @brief Standardized return / error codes for procedures.
     */
    enum class EReturnCode : uint8 {
        SUCCESS = 0,
        // Ticket-related errors
        TICKET_INVALID_PRICE = 1,
        TICKET_ALREADY_PURCHASED = 2,
        TICKET_ALL_SOLD_OUT = 3,
        TICKET_SELLING_CLOSED = 4,
        // Access-related errors
        ACCESS_DENIED = 5,
        // Fee-related errors
        FEE_INVALID_PRECENT_VALUE = 6,
        // Fallback
        UNKNOW_ERROR = UINT8_MAX
    };

    //---- User-facing I/O structures -------------------------------------------------------------

    struct BuyTicket_input {
    };

    struct BuyTicket_output {
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct GetFees_input {
    };

    struct GetFees_output {
        uint8 teamFeePercent = 0;
        uint8 distributionFeePercent = 0;
        uint8 winnerFeePercent = 0;
        uint8 burnPrecent = 0;
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct GetPlayers_input {
    };

    struct GetPlayers_output {
        Array<id, MAX_NUMBER_OF_PLAYERS> players;
        uint16 numberOfPlayers = 0;
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct GetPlayers_locals {
        uint64 arrayIndex = 0;
    };

    struct SetTicketPrice_input {
        uint64 newTicketPrice = 0;
    };

    struct SetTicketPrice_output {
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct SetTicketPriceInner_input {
        SetTicketPrice_input setTicketPriceInput;
    };

    struct SetTicketPriceInner_output {
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct SetTicketPrice_locals {
        SetTicketPriceInner_input setTicketPriceInnerInput;
        SetTicketPriceInner_output setTicketPriceInnerOutput;
    };

    /**
     * @brief Stored winner snapshot for an epoch.
     */
    struct WinnerInfo {
        id winnerAddress = id::zero();
        uint64 revenue = 0;
        uint16 epoch = 0;
        uint32 tick = 0;
    };

    struct FillWinnersInfo_input {
        id winnerAddress = id::zero();
        uint64 revenue = 0;
    };

    struct FillWinnersInfo_output {
    };

    struct FillWinnersInfo_locals {
        WinnerInfo winnerInfo = {};
    };

    struct GetWinner_input {
    };

    struct GetWinner_output {
        id winnerAddress = id::zero();
        uint64 index = 0;
    };

    struct GetWinner_locals {
        uint64 randomNum = 0;
    };

    struct GetWinners_input {
    };

    struct GetWinners_output {
        Array<WinnerInfo, MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners;
        uint64 numberOfWinners = 0;
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct END_EPOCH_locals {
        GetWinner_input getWinnerInput = {};
        GetWinner_output getWinnerOutput = {};
        GetWinner_locals getWinnerLocals = {};

        FillWinnersInfo_input fillWinnersInfoInput = {};
        FillWinnersInfo_output fillWinnersInfoOutput = {};
        FillWinnersInfo_locals fillWinnersInfoLocals = {};

        uint64 teamFee = 0;
        uint64 distributionFee = 0;
        uint64 winnerAmount = 0;
        uint64 burnedAmount = 0;

        uint64 revenue = 0;
        Entity entity;
    };

    struct SetFeePrecent_input {
        uint8 teamFeePercent = 0;
        uint8 distributionFeePercent = 0;
        uint8 burnPrecent = 0;
    };

    struct SetFeePrecent_output {
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct SetFeePrecentInner_input {
        SetFeePrecent_input setFeePrecentInput = {};
    };

    struct SetFeePrecentInner_output {
        uint8 returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
    };

    struct SetFeePrecent_locals {
        SetFeePrecentInner_input setFeePrecentInnerInput = {};
        SetFeePrecentInner_output setFeePrecentInnerOutput = {};
    };

    struct INITIALIZE_locals {
        SetFeePrecentInner_input setFeePrecentInnerInput = {};
        SetFeePrecentInner_output setFeePrecentInnerOutput = {};

        SetTicketPriceInner_input setTicketPriceInnerInput = {};
        SetTicketPriceInner_output setTicketPriceInnerOutput = {};
    };

public:
    /**
     * @brief Registers all externally callable functions and procedures with their numeric identifiers.
     * Mapping numbers must remain stable to preserve external interface compatibility.
     */
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
        REGISTER_USER_FUNCTION(GetFees, 1);
        REGISTER_USER_FUNCTION(GetPlayers, 2);
        REGISTER_USER_FUNCTION(GetWinners, 3);
        REGISTER_USER_PROCEDURE(BuyTicket, 1);
        REGISTER_USER_PROCEDURE(SetTicketPrice, 2);
        REGISTER_USER_PROCEDURE(SetFeePrecent, 3);
    }

    /**
     * @brief Contract initialization hook.
     * Sets default fees, ticket price, addresses, and locks the lottery (no selling yet).
     */
    INITIALIZE_WITH_LOCALS() {
        // Addresses
        state.teamAddress = RL_DEV_ADDRESS;
        state.ownerAddress = RL_OWNER_ADDRESS;

        // Default fee percentages (sum <= 100; winner percent derived)
        state.burnPrecent = 0; // (Will be overridden by inner call)
        locals.setFeePrecentInnerInput.setFeePrecentInput.burnPrecent = 2;
        locals.setFeePrecentInnerInput.setFeePrecentInput.teamFeePercent = 10;
        locals.setFeePrecentInnerInput.setFeePrecentInput.distributionFeePercent = 20;
        CALL(SetFeePrecentInner, locals.setFeePrecentInnerInput, locals.setFeePrecentInnerOutput);

        // Default ticket price
        locals.setTicketPriceInnerInput.setTicketPriceInput.newTicketPrice = 1000000;
        CALL(SetTicketPriceInner, locals.setTicketPriceInnerInput, locals.setTicketPriceInnerOutput);

        // Start locked
        state.currentState = EState::LOCKED;
    }

    /**
     * @brief Opens ticket selling for a new epoch.
     */
    BEGIN_EPOCH() {
        state.currentState = EState::SELLING;
    }

    /**
     * @brief Closes epoch: computes revenue, selects winner (if >1 player),
     * distributes fees, burns leftover, records winner, then clears players.
     */
    END_EPOCH_WITH_LOCALS() {
        state.currentState = EState::LOCKED;

        // Single-player edge case: refund instead of drawing.
        if (state.players.population() == 1) {
            for (sint32 i = 0; i < state.players.capacity(); ++i) {
                if (!state.players.isEmptySlot(i)) {
                    qpi.transfer(state.players.key(i), state.ticketPrice);
                    break;
                }
            }
        } else if (state.players.population() > 1) {
            qpi.getEntity(SELF, locals.entity);
            locals.revenue = locals.entity.incomingAmount - locals.entity.outgoingAmount;

            // Winner selection (pseudo-random).
            GetWinner(qpi, state, locals.getWinnerInput, locals.getWinnerOutput, locals.getWinnerLocals);

            if (locals.getWinnerOutput.winnerAddress != id::zero()) {
                // Fee splits
                locals.winnerAmount = div<uint64>(locals.revenue * state.winnerFeePercent, 100ULL);
                locals.teamFee = div<uint64>(locals.revenue * state.teamFeePercent, 100ULL);
                locals.distributionFee = div<uint64>(locals.revenue * state.distributionFeePercent, 100ULL);

                // Team fee
                if (locals.teamFee > 0) {
                    qpi.transfer(state.teamAddress, locals.teamFee);
                }
                // Distribution fee
                if (locals.distributionFee > 0) {
                    qpi.distributeDividends(div<uint64>(locals.distributionFee, uint64(NUMBER_OF_COMPUTORS)));
                }
                // Winner payout
                if (locals.winnerAmount > 0) {
                    qpi.transfer(locals.getWinnerOutput.winnerAddress, locals.winnerAmount);
                }

                // Burn all residual (handles rounding dust).
                {
                    qpi.getEntity(SELF, locals.entity);
                    locals.burnedAmount = locals.entity.incomingAmount - locals.entity.outgoingAmount;
                    qpi.burn(locals.burnedAmount);
                }

                // Persist winner record
                locals.fillWinnersInfoInput.winnerAddress = locals.getWinnerOutput.winnerAddress;
                locals.fillWinnersInfoInput.revenue = locals.winnerAmount;
                FillWinnersInfo(qpi, state, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput,
                                locals.fillWinnersInfoLocals);
            }
        }

        // Prepare for next epoch.
        state.players.reset();
    }

    /**
     * @brief Returns currently configured fee percentages.
     */
    PUBLIC_FUNCTION(GetFees) {
        output.teamFeePercent = state.teamFeePercent;
        output.distributionFeePercent = state.distributionFeePercent;
        output.winnerFeePercent = state.winnerFeePercent;
        output.burnPrecent = state.burnPrecent;
    }

    /**
     * @brief Retrieves the active players list for the ongoing epoch.
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetPlayers) {
        locals.arrayIndex = 0;
        output.numberOfPlayers = state.players.population();
        if (output.numberOfPlayers == 0) {
            return;
        }
        for (sint64 i = 0; i < state.players.capacity(); ++i) {
            if (!state.players.isEmptySlot(i)) {
                output.players.set(locals.arrayIndex++, state.players.key(i));
            }
        }
    }

    /**
     * @brief Returns historical winners (ring buffer segment).
     */
    PUBLIC_FUNCTION(GetWinners) {
        output.winners = state.winners;
        output.numberOfWinners = state.winnersInfoNextEmptyIndex;
    }

    /**
     * @brief Attempts to buy a ticket (must send exact price unless zero is forbidden; state must be SELLING).
     * Reverts with proper return codes for invalid cases.
     */
    PUBLIC_PROCEDURE(BuyTicket) {
        // Selling closed
        if (state.currentState == EState::LOCKED) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_SELLING_CLOSED);
            if (qpi.invocationReward() > 0) {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Already purchased
        if (state.players.contains(qpi.invocator())) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_ALREADY_PURCHASED);
            return;
        }

        // Capacity full
        if (state.players.add(qpi.invocator()) == NULL_INDEX) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_ALL_SOLD_OUT);
            return;
        }

        // Price mismatch
        if (qpi.invocationReward() != state.ticketPrice && qpi.invocationReward() > 0) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            state.players.remove(qpi.invocator());
            return;
        }
    }

    /**
     * @brief Owner-only: updates ticket price (must be > 0).
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(SetTicketPrice) {
        if (qpi.invocator() != state.ownerAddress) {
            output.returnCode = static_cast<uint8>(EReturnCode::ACCESS_DENIED);
            return;
        }
        locals.setTicketPriceInnerInput.setTicketPriceInput = input;
        CALL(SetTicketPriceInner, locals.setTicketPriceInnerInput, locals.setTicketPriceInnerOutput);
        output.returnCode = locals.setTicketPriceInnerOutput.returnCode;
    }

    /**
     * @brief Owner-only: sets fee distribution (sum of team + distribution + burn <= 100).
     * Winner share auto-computed as remainder.
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(SetFeePrecent) {
        if (qpi.invocator() != state.ownerAddress) {
            output.returnCode = static_cast<uint8>(EReturnCode::ACCESS_DENIED);
            return;
        }
        locals.setFeePrecentInnerInput.setFeePrecentInput = input;
        CALL(SetFeePrecentInner, locals.setFeePrecentInnerInput, locals.setFeePrecentInnerOutput);
        output.returnCode = locals.setFeePrecentInnerOutput.returnCode;
    }

private:
    /**
     * @brief Internal: records a winner into the cyclic winners array.
     */
    PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo) {
        if (input.winnerAddress == id::zero()) {
            return;
        }
        if (MAX_NUMBER_OF_WINNERS_IN_HISTORY >= state.winners.capacity() - 1) {
            state.winnersInfoNextEmptyIndex = 0;
        }
        locals.winnerInfo.winnerAddress = input.winnerAddress;
        locals.winnerInfo.revenue = input.revenue;
        locals.winnerInfo.epoch = qpi.epoch();
        locals.winnerInfo.tick = qpi.tick();
        state.winners.set(state.winnersInfoNextEmptyIndex++, locals.winnerInfo);
    }

    /**
     * @brief Internal: pseudo-random selection of a winner index using hardware RNG.
     */
    PRIVATE_FUNCTION_WITH_LOCALS(GetWinner) {
        if (state.players.population() == 0) {
            return;
        }
        _rdrand64_step(&locals.randomNum);
        locals.randomNum = mod<uint64>(locals.randomNum, state.players.population());
        for (sint64 i = 0, j = 0; i < state.players.capacity(); ++i) {
            if (!state.players.isEmptySlot(i)) {
                if (j++ == locals.randomNum) {
                    output.winnerAddress = state.players.key(i);
                    output.index = i;
                    break;
                }
            }
        }
    }

    /**
     * @brief Internal: validates and applies fee percentages.
     */
    PRIVATE_PROCEDURE(SetFeePrecentInner) {
        if (input.setFeePrecentInput.teamFeePercent
            + input.setFeePrecentInput.distributionFeePercent
            + input.setFeePrecentInput.burnPrecent > 100) {
            output.returnCode = static_cast<uint8>(EReturnCode::FEE_INVALID_PRECENT_VALUE);
            return;
        }
        state.teamFeePercent = input.setFeePrecentInput.teamFeePercent;
        state.distributionFeePercent = input.setFeePrecentInput.distributionFeePercent;
        state.burnPrecent = input.setFeePrecentInput.burnPrecent;
        state.winnerFeePercent = 100 - state.teamFeePercent - state.distributionFeePercent - state.burnPrecent;
    }

    /**
     * @brief Internal: validates and sets ticket price.
     */
    PRIVATE_PROCEDURE(SetTicketPriceInner) {
        if (input.setTicketPriceInput.newTicketPrice == 0) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
            return;
        }
        state.ticketPrice = input.setTicketPriceInput.newTicketPrice;
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
    uint8 burnPrecent = 0;

    /**
     * @brief Price of a single lottery ticket.
     * Value is in the smallest currency unit (e.g., cents).
     */
    uint64 ticketPrice = 0;

    /**
     * @brief Set of players participating in the current lottery epoch.
     * Maximum capacity is defined by MAX_NUMBER_OF_PLAYERS.
     */
    HashSet<id, MAX_NUMBER_OF_PLAYERS> players = {};

    /**
     * @brief Circular buffer storing the history of winners.
     * Maximum capacity is defined by MAX_NUMBER_OF_WINNERS_IN_HISTORY.
     */
    Array<WinnerInfo, MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners = {};

    /**
     * @brief Index pointing to the next empty slot in the winners array.
     * Used for maintaining the circular buffer of winners.
     */
    uint64 winnersInfoNextEmptyIndex = 0;

    /**
     * @brief Current state of the lottery contract.
     * Can be either SELLING (tickets available) or LOCKED (epoch closed).
     */
    EState currentState = EState::LOCKED;
};
