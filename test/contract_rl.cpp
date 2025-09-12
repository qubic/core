#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_SET_TICKET_PRICE = 2;

class RLChecker : public RL {
public:
    void TickPrice_EQ(const uint64 &Price) {
        EXPECT_EQ(ticketPrice, Price);
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
};

TEST(ContractRandomLottery, SetTicketPrice) {
    ContractTestingRL ctl;

    constexpr uint64 validPrice = 3333;
    constexpr uint64 invalidPrice = 2222;

    increaseEnergy(RL_OWNER_ADDRESS, 100);
    EXPECT_EQ(ctl.SetTicketPrice(RL_OWNER_ADDRESS, validPrice), (uint8) RL::EReturnCode::SUCCESS);
    ctl.getState()->TickPrice_EQ(validPrice);

    EXPECT_EQ(ctl.SetTicketPrice(RL_OWNER_ADDRESS, 0), (uint8) RL::EReturnCode::TICKET_INVALID_PRICE);

    const id randomUser = id::randomValue();
    increaseEnergy(randomUser, 100);
    EXPECT_EQ(ctl.SetTicketPrice(randomUser, invalidPrice), (uint8) RL::EReturnCode::ACCESS_DENIED);
    ctl.getState()->TickPrice_EQ(validPrice);
}
