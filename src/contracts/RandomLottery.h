#pragma once

using namespace QPI;

constexpr uint16 MAX_NUMBER_OF_PLAYERS = 1024;
constexpr uint16 MAX_NUMBER_OF_WINNERS_IN_HISTORY = 1024;
static const id RL_DEV_ADDRESS = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L,
                                    _A,
                                    _K, _F, _T, _D, _X, _C, _R, _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y,
                                    _Q,
                                    _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E, _U, _E, _J, _J);
static const id RL_OWNER_ADDRESS = RL_DEV_ADDRESS;


struct RL2 {
};

struct RL : public ContractBase {
public:
    enum class EState : uint8 {
        SELLING,
        LOCKED
    };

    enum class EReturnCode : uint8 {
        SUCCESS = 0,

        // Tickets
        TICKET_INVALID_PRICE = 1,
        TICKET_ALREADY_PURCHASED = 2,
        TICKET_ALL_SOLD_OUT = 3,
        TICKET_SELLING_CLOSED = 4,


        // Access
        ACCESS_DENIED = 5,

        UNKNOW_ERROR = UINT8_MAX
    };

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

    struct WinnersInfo {
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
        WinnersInfo winnerInfo = {};
    };

    struct GetWinner_input {
    };

    struct GetWinner_output {
        id winnerAddress = id::zero();
        uint64 index = 0;
    };

    struct GetWinner_locals {
        uint64 randomIndex = 0;
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

public:
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
        REGISTER_USER_FUNCTION(GetFees, 1);
        REGISTER_USER_FUNCTION(GetPlayers, 2);
        REGISTER_USER_PROCEDURE(BuyTicket, 1);
        REGISTER_USER_PROCEDURE(SetTicketPrice, 2);
    }

    INITIALIZE() {
        // Address
        state.teamAddress = RL_DEV_ADDRESS;
        state.ownerAddress = RL_OWNER_ADDRESS;

        // Burn
        state.teamBP = 0;
        state.distributionBP = 2;
        state.winnerBP = 2;
        state.amountBP = state.teamBP + state.distributionBP + state.winnerBP;

        // Fee
        state.teamFeePercent = 10 - state.teamBP;
        state.distributionFeePercent = 20 - state.distributionBP - state.distributionBP;
        state.winnerFeePercent = 100 - state.teamFeePercent - state.distributionFeePercent - state.winnerBP;

        // Ticket
        state.ticketPrice = 1000000;
    }

    BEGIN_EPOCH() {
        state.currentState = EState::SELLING;
    }

    END_EPOCH_WITH_LOCALS() {
        state.currentState = EState::LOCKED;

        // Return qus if only one player
        if (state.players.population() == 1) {
            // Refund all players
            for (sint32 i = 0; i < state.players.capacity(); ++i) {
                if (!state.players.isEmptySlot(i)) {
                    qpi.transfer(state.players.key(i), state.ticketPrice);
                    break;
                }
            }
        } else if (state.players.population() > 1) {
            qpi.getEntity(SELF, locals.entity);
            locals.revenue = locals.entity.incomingAmount - locals.entity.outgoingAmount;

            // Get winner
            GetWinner(qpi, state, locals.getWinnerInput, locals.getWinnerOutput, locals.getWinnerLocals);

            if (locals.getWinnerOutput.winnerAddress != id::zero()) {
                // Calculate fees
                locals.teamFee = div<uint64>(locals.revenue * state.teamFeePercent, 100ULL);
                locals.distributionFee = div<uint64>(locals.revenue * state.distributionFeePercent, 100ULL);
                locals.winnerAmount = locals.revenue - locals.teamFee - locals.distributionFee;
                locals.burnedAmount = div<uint64>(locals.revenue * state.winnerBP, 100ULL);

                // Transfer to team
                if (locals.teamFee > 0) {
                    qpi.transfer(state.teamAddress, locals.teamFee);
                }

                if (locals.distributionFee > 0) {
                    qpi.distributeDividends(div<uint64>(locals.distributionFee, uint64(NUMBER_OF_COMPUTORS)));
                }

                // Transfer to winner
                if (locals.winnerAmount > 0) {
                    qpi.transfer(locals.getWinnerOutput.winnerAddress, locals.winnerAmount);
                }

                // Burn
                if (locals.burnedAmount > 0) {
                    qpi.burn(locals.burnedAmount);
                }

                // Record winner info
                locals.fillWinnersInfoInput.winnerAddress = locals.getWinnerOutput.winnerAddress;
                locals.fillWinnersInfoInput.revenue = locals.winnerAmount;
                FillWinnersInfo(qpi, state, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput,
                                locals.fillWinnersInfoLocals);
            }
        }

        state.players.reset();
    }

    PUBLIC_FUNCTION(GetFees) {
        output.teamFeePercent = state.teamFeePercent;
        output.distributionFeePercent = state.distributionFeePercent;
        output.winnerFeePercent = state.winnerFeePercent;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetPlayers) {
        locals.arrayIndex = 0;

        for (sint32 i = 0; i < state.players.capacity(); ++i) {
            if (!state.players.isEmptySlot(i)) {
                output.players.set(locals.arrayIndex++, state.players.key(i));
            }
        }

        if (locals.arrayIndex > 0 || output.players.get(locals.arrayIndex) != id::zero()) {
            output.numberOfPlayers = locals.arrayIndex + 1;
        }
    }

    PUBLIC_PROCEDURE(BuyTicket) {
        // Selling closed
        if (state.currentState == EState::LOCKED) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_SELLING_CLOSED);

            if (qpi.invocationReward() > 0) {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }

            return;
        }

        // Already buy the ticket
        if (state.players.contains(qpi.invocator())) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_ALREADY_PURCHASED);
            return;
        }

        // Is full
        if (state.players.add(qpi.invocator()) == NULL_INDEX) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_ALL_SOLD_OUT);
            return;
        }

        // Invalid price
        if (qpi.invocationReward() != state.ticketPrice && qpi.invocationReward() > 0) {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());

            state.players.remove(qpi.invocator());
            return;
        }
    }

    PUBLIC_PROCEDURE(SetTicketPrice) {
        if (qpi.invocator() != state.ownerAddress) {
            output.returnCode = static_cast<uint8>(EReturnCode::ACCESS_DENIED);
            return;
        }

        if (input.newTicketPrice == 0) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
            return;
        }

        state.ticketPrice = input.newTicketPrice;
    }

protected:
    PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo) {
        if (input.winnerAddress == id::zero()) {
            return;
        }

        if (MAX_NUMBER_OF_WINNERS_IN_HISTORY >= state.winners.capacity() - 1) {
            state.winnersInfoIndex = 0;
        }

        locals.winnerInfo.winnerAddress = input.winnerAddress;
        locals.winnerInfo.revenue = input.revenue;
        locals.winnerInfo.epoch = qpi.epoch();
        locals.winnerInfo.tick = qpi.tick();

        state.winners.set(state.winnersInfoIndex++, locals.winnerInfo);
    }

    PRIVATE_FUNCTION_WITH_LOCALS(GetWinner) {
        if (state.players.population() == 0) {
            return;
        }

        _rdrand64_step(&locals.randomIndex);
        locals.randomIndex = mod<uint64>(locals.randomIndex, state.players.population());

        if (state.players.isEmptySlot(locals.randomIndex)) {
            return;
        }

        output.winnerAddress = state.players.key(locals.randomIndex);
        output.index = locals.randomIndex;
    }

protected:
    // Address
    id teamAddress = id::zero();
    id ownerAddress = id::zero();

    // Fee percent
    uint8 teamFeePercent = 0;
    uint8 distributionFeePercent = 0;
    uint8 winnerFeePercent = 0;

    // Burned percent
    uint8 teamBP = 0;
    uint8 distributionBP = 0;
    uint8 winnerBP = 0;
    uint8 amountBP = 0;

    // Ticket price
    uint64 ticketPrice = 0;

    HashSet<id, MAX_NUMBER_OF_PLAYERS> players = {};

    Array<WinnersInfo, MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners = {};
    uint64 winnersInfoIndex = 0;

    EState currentState = EState::LOCKED;
};
