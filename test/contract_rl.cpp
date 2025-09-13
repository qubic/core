#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_BUY_TICKET = 1;
constexpr uint16 PROCEDURE_INDEX_SET_TICKET_PRICE = 2;
constexpr uint16 PROCEDURE_INDEX_SET_FEE_PRECENT = 3;
constexpr uint16 FUNCTION_INDEX_GET_FEES = 1;
constexpr uint16 FUNCTION_INDEX_GET_PLAYERS = 2;
constexpr uint16 FUNCTION_INDEX_GET_WINNERS = 3;

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
        EXPECT_EQ(fees.burnPrecent, burnPrecent);
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

    void Check_Winners(const GetWinners_output &output) {
        EXPECT_EQ(output.returnCode, (uint8) EReturnCode::SUCCESS);
        EXPECT_EQ(output.winners.capacity(), winners.capacity());
        EXPECT_EQ(output.numberOfWinners, winnersInfoNextEmptyIndex);

        for (uint64 i = 0; i < output.numberOfWinners; ++i) {
            EXPECT_EQ(output.winners.get(i), winners.get(i));
        }
    }

    void RandomlyAddPlayers(uint64 maxNewPlayers) {
        unsigned int newPlayers = mod<uint64>(maxNewPlayers, players.capacity());
        for (unsigned int i = 0; i < newPlayers; ++i) {
            players.add(id::randomValue());
        }
    }

    void RandomlyAddWinners(uint64 maxNewWinners) {
        unsigned int newWinners = mod<uint64>(maxNewWinners, winners.capacity());

        winnersInfoNextEmptyIndex = 0;
        WinnerInfo wi;

        for (unsigned int i = 0; i < newWinners; ++i) {
            wi.epoch = 1;
            wi.tick = 1;
            wi.revenue = 1000000;
            wi.winnerAddress = id::randomValue();
            winners.set(winnersInfoNextEmptyIndex++, wi);
        }
    }

    void SetSelling() {
        currentState = EState::SELLING;
    }

    void SetLocked() {
        currentState = EState::LOCKED;
    }

    uint64 PlayersPopulation() {
        return players.population();
    }

    uint64 GetTicketPrice() {
        return ticketPrice;
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

    RL::GetWinners_output GetWinners() {
        RL::GetWinners_input input;
        RL::GetWinners_output output;

        callFunction(RL_CONTRACT_INDEX, FUNCTION_INDEX_GET_WINNERS, input, output);
        return output;
    }

    RL::SetFeePrecent_output SetFeePrecent(const id &authAddress, const RL::SetFeePrecent_input &feePrecent) {
        RL::SetFeePrecent_output output;

        invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_SET_FEE_PRECENT, feePrecent, output, authAddress, 0);
        return output;
    }

    RL::BuyTicket_output BuyTicket(const id &user, uint64 reward) {
        RL::BuyTicket_input input;
        RL::BuyTicket_output output;
        invokeUserProcedure(RL_CONTRACT_INDEX, PROCEDURE_INDEX_BUY_TICKET, input, output, user, reward);
        return output;
    }

    void BeginEpoch() {
        callSystemProcedure(RL_CONTRACT_INDEX, BEGIN_EPOCH);
    }

    void EndEpoch() {
        callSystemProcedure(RL_CONTRACT_INDEX, END_EPOCH);
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

TEST(ContractRandomLottery, SetFeePrecent) {
    ContractTestingRL ctl;

    RL::SetFeePrecent_input validFeePrecent;
    validFeePrecent.burnPrecent = 2;
    validFeePrecent.teamFeePercent = 10;
    validFeePrecent.distributionFeePercent = 20;
    const uint8 validWinnerFeePercent = 100 - validFeePrecent.distributionFeePercent - validFeePrecent.
                                        teamFeePercent - validFeePrecent.burnPrecent;

    RL::SetFeePrecent_input invalidFeePrecent;
    invalidFeePrecent.burnPrecent = 2;
    invalidFeePrecent.teamFeePercent = 10;
    invalidFeePrecent.distributionFeePercent = 100;

    auto CheckWithValidData = [&]() {
        const RL::GetFees_output feesOutput = ctl.GetFees();
        EXPECT_EQ(feesOutput.returnCode, (uint8) RL::EReturnCode::SUCCESS);
        EXPECT_EQ(feesOutput.burnPrecent, validFeePrecent.burnPrecent);
        EXPECT_EQ(feesOutput.teamFeePercent, validFeePrecent.teamFeePercent);
        EXPECT_EQ(feesOutput.distributionFeePercent, validFeePrecent.distributionFeePercent);
        EXPECT_EQ(feesOutput.winnerFeePercent, validWinnerFeePercent);
    };

    // Valid data
    {
        increaseEnergy(RL_OWNER_ADDRESS, 100);
        EXPECT_EQ(ctl.SetFeePrecent(RL_OWNER_ADDRESS, validFeePrecent).returnCode, (uint8) RL::EReturnCode::SUCCESS);
        CheckWithValidData();
    }

    // Access denied
    {
        const id randomUser = id::randomValue();
        increaseEnergy(randomUser, 100);

        EXPECT_EQ(ctl.SetFeePrecent(randomUser, invalidFeePrecent).returnCode, (uint8) RL::EReturnCode::ACCESS_DENIED);

        CheckWithValidData();
    }

    // Invalid precent value
    {
        EXPECT_EQ(ctl.SetFeePrecent(RL_OWNER_ADDRESS, invalidFeePrecent).returnCode,
                  (uint8) RL::EReturnCode::FEE_INVALID_PRECENT_VALUE);

        CheckWithValidData();
    }
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

TEST(ContractRandomLottery, GetWinners) {
    ContractTestingRL ctl;

    ctl.getState()->RandomlyAddWinners(10);
    RL::GetWinners_output winnersOutput = ctl.GetWinners();
    ctl.getState()->Check_Winners(winnersOutput);
}

TEST(ContractRandomLottery, BuyTicket) {
    ContractTestingRL ctl;

    const uint64 ticketPrice = ctl.getState()->GetTicketPrice();

    // 1. Attempt when state is LOCKED
    {
        id userLocked = id::randomValue();
        increaseEnergy(userLocked, ticketPrice * 2);
        RL::BuyTicket_output out = ctl.BuyTicket(userLocked, ticketPrice);
        EXPECT_EQ(out.returnCode, (uint8) RL::EReturnCode::TICKET_SELLING_CLOSED);
        EXPECT_EQ(ctl.getState()->PlayersPopulation(), 0);
    }

    // Switch to SELLING state
    ctl.getState()->SetSelling();

    // 2. Loop over several users
    constexpr uint64 userCount = 5;

    uint64 expectedPlayers = 0;

    for (uint16 i = 0; i < userCount; ++i) {
        id u = id::randomValue();
        // Give enough energy for multiple attempts
        increaseEnergy(u, ticketPrice * 5);

        // (a) Invalid price (should NOT add player)
        {
            RL::BuyTicket_output outInvalid = ctl.BuyTicket(u, ticketPrice - 1);
            EXPECT_EQ(outInvalid.returnCode, (uint8) RL::EReturnCode::TICKET_INVALID_PRICE);
            EXPECT_EQ(ctl.getState()->PlayersPopulation(), expectedPlayers);
        }

        // (b) Valid purchase
        {
            RL::BuyTicket_output outOk = ctl.BuyTicket(u, ticketPrice);
            EXPECT_EQ(outOk.returnCode, (uint8) RL::EReturnCode::SUCCESS);
            ++expectedPlayers;
            EXPECT_EQ(ctl.getState()->PlayersPopulation(), expectedPlayers);
        }

        // (c) Duplicate purchase
        {
            auto outDup = ctl.BuyTicket(u, ticketPrice);
            EXPECT_EQ(outDup.returnCode, (uint8) RL::EReturnCode::TICKET_ALREADY_PURCHASED);
            EXPECT_EQ(ctl.getState()->PlayersPopulation(), expectedPlayers);
        }
    }

    // 3. Sanity: ensure all stored users really counted
    EXPECT_EQ(expectedPlayers, userCount);
}

TEST(ContractRandomLottery, EndEpoch) {
    ContractTestingRL ctl;

    // Утилиты
    auto contractAddress = id(RL_CONTRACT_INDEX, 0, 0, 0);
    const uint64 ticketPrice = ctl.getState()->GetTicketPrice();

    auto fees = ctl.GetFees();
    const uint8 teamPercent = fees.teamFeePercent; // 10
    const uint8 distributionPercent = fees.distributionFeePercent; // 20
    const uint8 burnPercent = fees.burnPrecent; // 2
    const uint8 winnerPercent = fees.winnerFeePercent; // 68

    // --- 1. Случай: 0 игроков ---
    {
        ctl.BeginEpoch();
        EXPECT_EQ(ctl.getState()->PlayersPopulation(), 0u);

        RL::GetWinners_output before = ctl.GetWinners();
        EXPECT_EQ(before.numberOfWinners, 0u);

        ctl.EndEpoch();

        RL::GetWinners_output after = ctl.GetWinners();
        EXPECT_EQ(after.numberOfWinners, 0u);
        EXPECT_EQ(ctl.getState()->PlayersPopulation(), 0u);
    }

    // --- 2. Случай: 1 игрок (должен получить возврат) ---
    {
        ctl.BeginEpoch();

        id solo = id::randomValue();
        increaseEnergy(solo, ticketPrice);
        uint64 balanceBefore = getBalance(solo);

        auto out = ctl.BuyTicket(solo, ticketPrice);
        EXPECT_EQ(out.returnCode, (uint8) RL::EReturnCode::SUCCESS);
        EXPECT_EQ(ctl.getState()->PlayersPopulation(), 1u);
        EXPECT_EQ(getBalance(solo), balanceBefore - ticketPrice);

        ctl.EndEpoch();

        // Рефанд
        EXPECT_EQ(getBalance(solo), balanceBefore);
        EXPECT_EQ(ctl.getState()->PlayersPopulation(), 0u);

        RL::GetWinners_output winners = ctl.GetWinners();
        EXPECT_EQ(winners.numberOfWinners, 0u);
    }

    // --- 3. Случай: >1 игрока (розыгрыш + распределение) ---
    {
        ctl.BeginEpoch();

        const uint32 N = 5;
        std::vector < id > players;
        players.reserve(N);

        struct PlayerInfo {
            id addr;
            uint64 balanceBefore;
            uint64 balanceAfterBuy;
        };
        std::vector < PlayerInfo > infos;

        for (uint32 i = 0; i < N; ++i) {
            id p = id::randomValue();
            increaseEnergy(p, ticketPrice * 2);
            uint64 bBefore = getBalance(p);
            auto out = ctl.BuyTicket(p, ticketPrice);
            EXPECT_EQ(out.returnCode, (uint8) RL::EReturnCode::SUCCESS);
            EXPECT_EQ(getBalance(p), bBefore - ticketPrice);
            infos.push_back({p, bBefore, bBefore - ticketPrice});
            players.push_back(p);
        }

        EXPECT_EQ(ctl.getState()->PlayersPopulation(), N);

        uint64 contractBalanceBefore = getBalance(contractAddress);
        EXPECT_EQ(contractBalanceBefore, ticketPrice * N);

        uint64 teamBalanceBefore = getBalance(RL_DEV_ADDRESS);

        RL::GetWinners_output winnersBefore = ctl.GetWinners();
        uint64 winnersCountBefore = winnersBefore.numberOfWinners;

        ctl.EndEpoch();

        // После окончания продажи очищены
        EXPECT_EQ(ctl.getState()->PlayersPopulation(), 0u);

        RL::GetWinners_output winnersAfter = ctl.GetWinners();
        EXPECT_EQ(winnersAfter.numberOfWinners, winnersCountBefore + 1);

        // Последняя запись (индекс winnersCountBefore)
        const RL::WinnerInfo wi = winnersAfter.winners.get(winnersCountBefore);
        EXPECT_NE(wi.winnerAddress, id::zero());
        EXPECT_EQ(wi.revenue, (ticketPrice * N * winnerPercent) / 100);

        // Победитель среди игроков
        bool found = false;
        for (auto &inf: infos) {
            if (inf.addr == wi.winnerAddress) {
                found = true;
            }
        }
        EXPECT_TRUE(found);

        // Проверка начисления выигрыша победителю
        for (auto &inf: infos) {
            uint64 bal = getBalance(inf.addr);
            if (inf.addr == wi.winnerAddress) {
                EXPECT_EQ(bal, inf.balanceAfterBuy + wi.revenue);
            } else {
                EXPECT_EQ(bal, inf.balanceAfterBuy); // без изменений
            }
        }

        // Проверка комиссии команды
        uint64 teamFeeExpected = (ticketPrice * N * teamPercent) / 100;
        EXPECT_EQ(getBalance(RL_DEV_ADDRESS), teamBalanceBefore + teamFeeExpected);
    }
}
