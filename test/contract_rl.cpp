#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_SET_TICKET_PRICE = 2;
constexpr uint16 FUNCTION_INDEX_GET_FEES = 1;
constexpr uint16 FUNCTION_INDEX_GET_PLAYERS = 2;

class RLChecker : public RL {
public:
    void Check_TicketPrice(const uint64 &Price) {
        EXPECT_EQ(ticketPrice, Price);
    }

    void Check_Fees(const GetFees_output &fees) {
        EXPECT_EQ(fees.returnCode, (uint8) EReturnCode::SUCCESS);

        EXPECT_EQ(fees.distributionFeePercent, distributionFeePercent);
        EXPECT_EQ(fees.teamFeePercent, teamFeePercent);
        EXPECT_EQ(fees.winnerFeePercent, winnerFeePercent);
    }

    void RandomlyAddPlayers(unsigned int maxNewPlayers) {
        unsigned int newPlayers = mod<uint64>(maxNewPlayers, players.capacity());
        for (unsigned int i = 0; i < newPlayers; ++i) {
            players.add(id::randomValue());
        }
    }

    void Check_Players(const GetPlayers_output &output) {
        EXPECT_EQ(output.returnCode, (uint8) EReturnCode::SUCCESS);
        EXPECT_EQ(output.players.capacity(), players.capacity());
        EXPECT_EQ((uint64) output.numberOfPlayers, players.population());

        uint64 playerArrayIndex = 0;
        for (uint64 i = 0; i < players.capacity(); ++i) {
            if (!players.isEmptySlot(i)) {
                EXPECT_EQ(output.players.get(playerArrayIndex++), players.key(i));
            }
        }
    }
};

class ContractTestingRL : protected ContractTesting {
public:
    ContractTestingRL() {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(RL);
        callSystemProcedure(RL_CONTRACT_INDEX, INITIALIZE);
    }

    RLChecker *getState() {
        return (RLChecker *) contractStates[RL_CONTRACT_INDEX];
    }

    uint8 SetTicketPrice(const id &authAddress, const uint64 &newPrice) {
        RL::SetTicketPrice_input input{newPrice};
        RL::SetTicketPrice_output output;

        invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_SET_TICKET_PRICE, input, output, authAddress, 0);
        return output.returnCode;
    }

    RL::GetFees_output GetFees() {
        RL::GetFees_input input;
        RL::GetFees_output output;

        callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_FEES, input, output);
        return output;
    }

    RL::GetPlayers_output GetPlayers() {
        RL::GetPlayers_input input;
        RL::GetPlayers_output output;

        callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_PLAYERS, input, output);
        return output;
    }
};

TEST(ContractRandomLottery, SetTicketPrice) {
    ContractTestingRL ctl;

    constexpr uint64 validPrice = 3333;
    constexpr uint64 invalidPrice = 2222;

    increaseEnergy(RL_OWNER_ADDRESS, 100);
    EXPECT_EQ(ctl.SetTicketPrice(RL_OWNER_ADDRESS, validPrice), (uint8) RL::EReturnCode::SUCCESS);
    ctl.getState()->Check_TicketPrice(validPrice);

    EXPECT_EQ(ctl.SetTicketPrice(RL_OWNER_ADDRESS, 0), (uint8) RL::EReturnCode::TICKET_INVALID_PRICE);

    const id randomUser = id::randomValue();
    increaseEnergy(randomUser, 100);
    EXPECT_EQ(ctl.SetTicketPrice(randomUser, invalidPrice), (uint8) RL::EReturnCode::ACCESS_DENIED);
    ctl.getState()->Check_TicketPrice(validPrice);
}

TEST(ContractRandomLottery, GetFees) {
    ContractTestingRL ctl;

    RL::GetFees_output output = ctl.GetFees();
    ctl.getState()->Check_Fees(output);
}

TEST(ContractRandomLottery, GetPlayers) {
    ContractTestingRL ctl;

    RL::GetPlayers_output output = ctl.GetPlayers();
    ctl.getState()->Check_Players(output);

    constexpr uint64 maxPlayersToAdd = 10;
    ctl.getState()->RandomlyAddPlayers(maxPlayersToAdd);
    output = ctl.GetPlayers();
    ctl.getState()->Check_Players(output);
}
