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

        // Precent
        FEE_INVALID_PRECENT_VALUE = 6,

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

    struct WinnerInfo {
        id winnerAddress = id::zero();
        uint64 revenue = 0;
        uint16 epoch = 0;
        uint32 tick = 0;

        bool operator==(const WinnerInfo &other) const {
            return winnerAddress == other.winnerAddress && revenue == other.revenue && epoch == other.epoch &&
                   tick == other.tick;
        }

        bool operator!=(const WinnerInfo &other) const {
            return !(*this == other);
        }
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
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
        REGISTER_USER_FUNCTION(GetFees, 1);
        REGISTER_USER_FUNCTION(GetPlayers, 2);
        REGISTER_USER_FUNCTION(GetWinners, 3);
        REGISTER_USER_PROCEDURE(BuyTicket, 1);
        REGISTER_USER_PROCEDURE(SetTicketPrice, 2);
        REGISTER_USER_PROCEDURE(SetFeePrecent, 3);
    }

    INITIALIZE_WITH_LOCALS() {
        // Address
        state.teamAddress = RL_DEV_ADDRESS;
        state.ownerAddress = RL_OWNER_ADDRESS;

        // Fee precents
        locals.setFeePrecentInnerInput.setFeePrecentInput.burnPrecent = 2;
        locals.setFeePrecentInnerInput.setFeePrecentInput.teamFeePercent = 10;
        locals.setFeePrecentInnerInput.setFeePrecentInput.distributionFeePercent = 20;
        CALL(SetFeePrecentInner, locals.setFeePrecentInnerInput, locals.setFeePrecentInnerOutput);

        // Tick price
        locals.setTicketPriceInnerInput.setTicketPriceInput.newTicketPrice = 1000000;
        CALL(SetTicketPriceInner, locals.setTicketPriceInnerInput, locals.setTicketPriceInnerOutput);

        // State
        state.currentState = EState::LOCKED;
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
                locals.winnerAmount = div<uint64>(locals.revenue * state.winnerFeePercent, 100Ull);
                locals.teamFee = div<uint64>(locals.revenue * state.teamFeePercent, 100Ull);
                locals.distributionFee = div<uint64>(locals.revenue * state.distributionFeePercent, 100ULL);

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

                // Burn the entire remaining balance.
                // We do this instead of calculating percentages, because division rounding
                // might otherwise leave a few qus on the account.
                {
                    qpi.getEntity(SELF, locals.entity);
                    locals.burnedAmount = locals.entity.incomingAmount - locals.entity.outgoingAmount;
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
        output.burnPrecent = state.burnPrecent;
    }

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

    PUBLIC_FUNCTION(GetWinners) {
        output.winners = state.winners;
        output.numberOfWinners = state.winnersInfoNextEmptyIndex;
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
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);

            qpi.transfer(qpi.invocator(), qpi.invocationReward());

            state.players.remove(qpi.invocator());
            return;
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(SetTicketPrice) {
        if (qpi.invocator() != state.ownerAddress) {
            output.returnCode = static_cast<uint8>(EReturnCode::ACCESS_DENIED);
            return;
        }

        locals.setTicketPriceInnerInput.setTicketPriceInput = input;

        CALL(SetTicketPriceInner, locals.setTicketPriceInnerInput, locals.setTicketPriceInnerOutput);
        output.returnCode = locals.setTicketPriceInnerOutput.returnCode;
    }

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

    PRIVATE_PROCEDURE(SetFeePrecentInner) {
        if (input.setFeePrecentInput.teamFeePercent + input.setFeePrecentInput.distributionFeePercent + input.
            setFeePrecentInput.burnPrecent > 100) {
            output.returnCode = static_cast<uint8>(EReturnCode::FEE_INVALID_PRECENT_VALUE);
            return;
        }

        state.teamFeePercent = input.setFeePrecentInput.teamFeePercent;
        state.distributionFeePercent = input.setFeePrecentInput.distributionFeePercent;
        state.burnPrecent = input.setFeePrecentInput.burnPrecent;
        state.winnerFeePercent = 100 - state.teamFeePercent - state.distributionFeePercent - state.burnPrecent;
    }

    PRIVATE_PROCEDURE(SetTicketPriceInner) {
        if (input.setTicketPriceInput.newTicketPrice == 0) {
            output.returnCode = static_cast<uint8>(EReturnCode::TICKET_INVALID_PRICE);
            return;
        }

        state.ticketPrice = input.setTicketPriceInput.newTicketPrice;
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
    uint8 burnPrecent = 0;

    // Ticket price
    uint64 ticketPrice = 0;

    HashSet<id, MAX_NUMBER_OF_PLAYERS> players = {};

    Array<WinnerInfo, MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners = {};
    uint64 winnersInfoNextEmptyIndex = 0;

    EState currentState = EState::LOCKED;
};
