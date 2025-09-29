#define NO_UEFI

#include "contract_testing.h"

std::string assetNameFromInt64(uint64 assetName);
const id adminAddress = ID(_B, _O, _N, _D, _A, _A, _F, _B, _U, _G, _H, _E, _L, _A, _N, _X, _G, _H, _N, _L, _M, _S, _U, _I, _V, _B, _K, _B, _H, _A, _Y, _E, _Q, _S, _Q, _B, _V, _P, _V, _N, _B, _H, _L, _F, _J, _I, _A, _Z, _F, _Q, _C, _W, _W, _B, _V, _E);
const id testAddress1 = ID(_H, _O, _G, _T, _K, _D, _N, _D, _V, _U, _U, _Z, _U, _F, _L, _A, _M, _L, _V, _B, _L, _Z, _D, _S, _G, _D, _D, _A, _E, _B, _E, _K, _K, _L, _N, _Z, _J, _B, _W, _S, _C, _A, _M, _D, _S, _X, _T, _C, _X, _A, _M, _A, _X, _U, _D, _F);
const id testAddress2 = ID(_E, _Q, _M, _B, _B, _V, _Y, _G, _Z, _O, _F, _U, _I, _H, _E, _X, _F, _O, _X, _K, _T, _F, _T, _A, _N, _E, _K, _B, _X, _L, _B, _X, _H, _A, _Y, _D, _F, _F, _M, _R, _E, _E, _M, _R, _Q, _E, _V, _A, _D, _Y, _M, _M, _E, _W, _A, _C);

class QBondChecker : public QBOND
{
public:
    int64_t getCFAPopulation()
    {
        return _commissionFreeAddresses.population();
    }
};

class ContractTestingQBond : protected ContractTesting
{
public:
    ContractTestingQBond()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QBOND);
        callSystemProcedure(QBOND_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QEARN);
        callSystemProcedure(QEARN_CONTRACT_INDEX, INITIALIZE);
    }

    QBondChecker* getState()
    {
        return (QBondChecker*)contractStates[QBOND_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QBOND_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QBOND_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void stake(const id& staker, const int64_t& quMillions, const int64_t& quAmount)
    {
        QBOND::Stake_input input{ quMillions };
        QBOND::Stake_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 1, input, output, staker, quAmount);
    }

    QBOND::TransferMBondOwnershipAndPossession_output transfer(const id& from, const id& to, const uint16_t& epoch, const int64_t& mbondsAmount, const int64_t& quAmount)
    {
        QBOND::TransferMBondOwnershipAndPossession_input input{ to, epoch, mbondsAmount };
        QBOND::TransferMBondOwnershipAndPossession_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 2, input, output, from, quAmount);
        return output;
    }

    QBOND::AddAskOrder_output addAskOrder(const id& asker, const uint16_t& epoch, const int64_t& price, const int64_t& mbondsAmount, const int64_t& quAmount)
    {
        QBOND::AddAskOrder_input input{ epoch, price, mbondsAmount };
        QBOND::AddAskOrder_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 3, input, output, asker, quAmount);
        return output;
    }

    QBOND::RemoveAskOrder_output removeAskOrder(const id& asker, const uint16_t& epoch, const int64_t& price, const int64_t& mbondsAmount, const int64_t& quAmount)
    {
        QBOND::RemoveAskOrder_input input{ epoch, price, mbondsAmount };
        QBOND::RemoveAskOrder_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 4, input, output, asker, quAmount);
        return output;
    }

    QBOND::AddBidOrder_output addBidOrder(const id& bider, const uint16_t& epoch, const int64_t& price, const int64_t& mbondsAmount, const int64_t& quAmount)
    {
        QBOND::AddBidOrder_input input{ epoch, price, mbondsAmount };
        QBOND::AddBidOrder_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 5, input, output, bider, quAmount);
        return output;
    }

    QBOND::RemoveBidOrder_output removeBidOrder(const id& bider, const uint16_t& epoch, const int64_t& price, const int64_t& mbondsAmount, const int64_t& quAmount)
    {
        QBOND::RemoveBidOrder_input input{ epoch, price, mbondsAmount };
        QBOND::RemoveBidOrder_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 6, input, output, bider, quAmount);
        return output;
    }

    QBOND::BurnQU_output burnQU(const id& invocator, const int64_t& quToBurn, const int64_t& quAmount)
    {
        QBOND::BurnQU_input input{ quToBurn };
        QBOND::BurnQU_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 7, input, output, invocator, quAmount);
        return output;
    }

    bool updateCFA(const id& invocator, const id& address, const bool operation)
    {
        QBOND::UpdateCFA_input input{ address, operation };
        QBOND::UpdateCFA_output output;
        invokeUserProcedure(QBOND_CONTRACT_INDEX, 8, input, output, invocator, 0);
        return output.result;
    }

    QBOND::GetEarnedFees_output getEarnedFees()
    {
        QBOND::GetEarnedFees_input input;
        QBOND::GetEarnedFees_output output;
        callFunction(QBOND_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QBOND::GetInfoPerEpoch_output getInfoPerEpoch(const uint16_t& epoch)
    {
        QBOND::GetInfoPerEpoch_input input{ epoch };
        QBOND::GetInfoPerEpoch_output output;
        callFunction(QBOND_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QBOND::GetOrders_output getOrders(const uint16_t& epoch, const int64_t& asksOffset, const int64_t& bidsOffset)
    {
        QBOND::GetOrders_input input{ epoch, asksOffset, bidsOffset };
        QBOND::GetOrders_output output;
        callFunction(QBOND_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QBOND::GetUserOrders_output getUserOrders(const id& user, const int64_t& asksOffset, const int64_t& bidsOffset)
    {
        QBOND::GetUserOrders_input input{ user, asksOffset, bidsOffset };
        QBOND::GetUserOrders_output output;
        callFunction(QBOND_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QBOND::GetMBondsTable_output getMBondsTable()
    {
        QBOND::GetMBondsTable_input input;
        QBOND::GetMBondsTable_output output;
        callFunction(QBOND_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QBOND::GetUserMBonds_output getUserMBonds(const id& user)
    {
        QBOND::GetUserMBonds_input input{ user };
        QBOND::GetUserMBonds_output output;
        callFunction(QBOND_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QBOND::GetCFA_output getCFA()
    {
        QBOND::GetCFA_input input;
        QBOND::GetCFA_output output;
        callFunction(QBOND_CONTRACT_INDEX, 8, input, output);
        return output;
    }
};

TEST(ContractQBond, Stake)
{
    system.epoch = QBOND_START_EPOCH;
    ContractTestingQBond qbond;
    qbond.beginEpoch();

    increaseEnergy(testAddress1, 100000000LL);
    increaseEnergy(testAddress2, 100000000LL);

    // scenario 1: testAddress1 want to stake 50 millions, but send to sc 30 millions
    qbond.stake(testAddress1, 50, 30000000LL);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress1, testAddress1, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 0);

    // scenario 2: testAddress1 want to stake 50 millions, but send to sc 50 millions (without commission)
    qbond.stake(testAddress1, 50, 50000000LL);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress1, testAddress1, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 0);

    // scenario 3: testAddress1 want to stake 50 millions and send full amount with commission
    qbond.stake(testAddress1, 50, 50250000LL);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress1, testAddress1, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 50LL);

    // scenario 4.1: testAddress2 want to stake 5 millions, recieve 0 MBonds, because minimum is 10 and 5 were put in queue
    qbond.stake(testAddress2, 5, 5025000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 0);

    // scenario 4.2: testAddress1 want to stake 7 millions, testAddress1 recieve 7 MBonds and testAddress2 recieve 5 MBonds, because the total qu millions in the queue became more than 10
    qbond.stake(testAddress1, 7, 7035000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress1, testAddress1, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 57);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 5);
}


TEST(ContractQBond, TransferMBondOwnershipAndPossession)
{
    ContractTestingQBond qbond;
    qbond.beginEpoch();
    increaseEnergy(testAddress1, 1000000000);
    qbond.stake(testAddress1, 50, 50250000);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress1, testAddress1, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 50);

    // scenario 1: not enough gas, 100 needed
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 10, 50).transferredMBonds, 0);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 0);

    // scenario 2: enough gas, not enough mbonds
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 70, 100).transferredMBonds, 0);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 0);

    // scenario 3: success
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 40, 100).transferredMBonds, 40);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 40);
}

TEST(ContractQBond, AddRemoveAskOrder)
{
    ContractTestingQBond qbond;
    qbond.beginEpoch();
    increaseEnergy(testAddress1, 1000000000);
    qbond.stake(testAddress1, 50, 50250000);

    // scenario 1: not enough mbonds
    EXPECT_EQ(qbond.addAskOrder(testAddress1, system.epoch, 1500000, 100, 0).addedMBondsAmount, 0);

    // scenario 2: success to add ask, asked mbonds are blocked and cannot be transferred to another address
    EXPECT_EQ(qbond.addAskOrder(testAddress1, system.epoch, 1500000, 30, 0).addedMBondsAmount, 30);
    // not enough free mbonds
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 21, 100).transferredMBonds, 0);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 0);
    // successful transfer
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 20, 100).transferredMBonds, 20);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 20);

    // scenario 3: no orders to remove at this price
    EXPECT_EQ(qbond.removeAskOrder(testAddress1, system.epoch, 1400000, 30, 0).removedMBondsAmount, 0);
    EXPECT_EQ(qbond.removeAskOrder(testAddress1, system.epoch, 1600000, 30, 0).removedMBondsAmount, 0);

    // scenario 4: no free mbonds, then successful removal ask order and transfer to another address
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 1, 100).transferredMBonds, 0);
    EXPECT_EQ(qbond.removeAskOrder(testAddress1, system.epoch, 1500000, 5, 0).removedMBondsAmount, 5);
    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 5, 100).transferredMBonds, 5);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 25);

    EXPECT_EQ(qbond.removeAskOrder(testAddress1, system.epoch, 1500000, 500, 0).removedMBondsAmount, 25);
}

TEST(ContractQBond, AddRemoveBidOrder)
{
    ContractTestingQBond qbond;
    qbond.beginEpoch();
    increaseEnergy(testAddress1, 1000000000);
    increaseEnergy(testAddress2, 1000000000);
    qbond.stake(testAddress1, 50, 50250000);

    // scenario 1: not enough qu
    EXPECT_EQ(qbond.addBidOrder(testAddress2, system.epoch, 1500000, 10, 100).addedMBondsAmount, 0);

    // scenario 2: success to add bid
    EXPECT_EQ(qbond.addBidOrder(testAddress2, system.epoch, 1500000, 10, 15000000).addedMBondsAmount, 10);

    // scenario 3: testAddress1 add ask order which matches the bid order
    EXPECT_EQ(qbond.addAskOrder(testAddress1, system.epoch, 1500000, 3, 0).addedMBondsAmount, 3);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress1, testAddress1, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 47);
    EXPECT_EQ(numberOfPossessedShares(assetNameFromString(std::string("MBND").append(std::to_string(system.epoch)).c_str()), id(QBOND_CONTRACT_INDEX, 0, 0, 0), testAddress2, testAddress2, QBOND_CONTRACT_INDEX, QBOND_CONTRACT_INDEX), 3);

    // scenario 3: no orders to remove at this price
    EXPECT_EQ(qbond.removeBidOrder(testAddress2, system.epoch, 1400000, 30, 0).removedMBondsAmount, 0);
    EXPECT_EQ(qbond.removeBidOrder(testAddress2, system.epoch, 1600000, 30, 0).removedMBondsAmount, 0);

    // scenario 4: successful removal bid order, qu are returned (7 mbonds per 1500000 each)
    int64_t prevBalance = getBalance(testAddress2);
    EXPECT_EQ(qbond.removeBidOrder(testAddress2, system.epoch, 1500000, 100, 0).removedMBondsAmount, 7);
    EXPECT_EQ(getBalance(testAddress2) - prevBalance, 10500000);

    // check earned fees
    auto fees = qbond.getEarnedFees();
    EXPECT_EQ(fees.stakeFees, 250000);
    EXPECT_EQ(fees.tradeFees, 1350); // 1500000 (MBond price) * 3 (MBonds) * 0.0003 (0.03% fees for trade)

    // getOrders checks
    EXPECT_EQ(qbond.addAskOrder(testAddress1, system.epoch, 1600000, 5, 0).addedMBondsAmount, 5);
    EXPECT_EQ(qbond.addAskOrder(testAddress2, system.epoch, 1500000, 3, 0).addedMBondsAmount, 3);
    EXPECT_EQ(qbond.addBidOrder(testAddress1, system.epoch, 1400000, 10, 14000000).addedMBondsAmount, 10);
    EXPECT_EQ(qbond.addBidOrder(testAddress2, system.epoch, 1300000, 5, 6500000).addedMBondsAmount, 5);

    // all orders sorted by price, therefore the element with index 0 contains an order with a price of 1500000
    auto orders = qbond.getOrders(system.epoch, 0, 0);
    EXPECT_EQ(orders.askOrders.get(0).epoch, 182);
    EXPECT_EQ(orders.askOrders.get(0).numberOfMBonds, 3);
    EXPECT_EQ(orders.askOrders.get(0).owner, testAddress2);
    EXPECT_EQ(orders.askOrders.get(0).price, 1500000);

    EXPECT_EQ(orders.bidOrders.get(0).epoch, 182);
    EXPECT_EQ(orders.bidOrders.get(0).numberOfMBonds, 10);
    EXPECT_EQ(orders.bidOrders.get(0).owner, testAddress1);
    EXPECT_EQ(orders.bidOrders.get(0).price, 1400000);

    // with offset
    orders = qbond.getOrders(system.epoch, 1, 1);
    EXPECT_EQ(orders.askOrders.get(0).epoch, 182);
    EXPECT_EQ(orders.askOrders.get(0).numberOfMBonds, 5);
    EXPECT_EQ(orders.askOrders.get(0).owner, testAddress1);
    EXPECT_EQ(orders.askOrders.get(0).price, 1600000);

    EXPECT_EQ(orders.bidOrders.get(0).epoch, 182);
    EXPECT_EQ(orders.bidOrders.get(0).numberOfMBonds, 5);
    EXPECT_EQ(orders.bidOrders.get(0).owner, testAddress2);
    EXPECT_EQ(orders.bidOrders.get(0).price, 1300000);

    // user orders
    auto userOrders = qbond.getUserOrders(testAddress1, 0, 0);
    EXPECT_EQ(userOrders.askOrders.get(0).epoch, 182);
    EXPECT_EQ(userOrders.askOrders.get(0).numberOfMBonds, 5);
    EXPECT_EQ(userOrders.askOrders.get(0).owner, testAddress1);
    EXPECT_EQ(userOrders.askOrders.get(0).price, 1600000);

    EXPECT_EQ(userOrders.bidOrders.get(0).epoch, 182);
    EXPECT_EQ(userOrders.bidOrders.get(0).numberOfMBonds, 10);
    EXPECT_EQ(userOrders.bidOrders.get(0).owner, testAddress1);
    EXPECT_EQ(userOrders.bidOrders.get(0).price, 1400000);

    // with offset
    userOrders = qbond.getUserOrders(testAddress1, 1, 1);
    EXPECT_EQ(userOrders.askOrders.get(0).epoch, 0);
    EXPECT_EQ(userOrders.askOrders.get(0).numberOfMBonds, 0);
    EXPECT_EQ(userOrders.askOrders.get(0).owner, NULL_ID);
    EXPECT_EQ(userOrders.askOrders.get(0).price, 0);

    EXPECT_EQ(userOrders.bidOrders.get(0).epoch, 0);
    EXPECT_EQ(userOrders.bidOrders.get(0).numberOfMBonds, 0);
    EXPECT_EQ(userOrders.bidOrders.get(0).owner, NULL_ID);
    EXPECT_EQ(userOrders.bidOrders.get(0).price, 0);
}

TEST(ContractQBond, BurnQu)
{
    ContractTestingQBond qbond;
    qbond.beginEpoch();
    increaseEnergy(testAddress1, 1000000000);

    // scenario 1: not enough qu
    EXPECT_EQ(qbond.burnQU(testAddress1, 1000000, 1000).amount, -1);

    // scenario 2: successful burning
    EXPECT_EQ(qbond.burnQU(testAddress1, 1000000, 1000000).amount, 1000000);

    // scenario 3: successful burning, the surplus is returned
    int64_t prevBalance = getBalance(testAddress1);
    EXPECT_EQ(qbond.burnQU(testAddress1, 1000000, 10000000).amount, 1000000);
    EXPECT_EQ(prevBalance - getBalance(testAddress1), 1000000);
}

TEST(ContractQBond, UpdateCFA)
{
    ContractTestingQBond qbond;
    increaseEnergy(testAddress1, 1000);
    increaseEnergy(adminAddress, 1000);

    // only adminAddress can update CFA
    EXPECT_EQ(qbond.getState()->getCFAPopulation(), 1);
    EXPECT_FALSE(qbond.updateCFA(testAddress1, testAddress2, 1));
    EXPECT_EQ(qbond.getState()->getCFAPopulation(), 1);
    EXPECT_TRUE(qbond.updateCFA(adminAddress, testAddress2, 1));
    EXPECT_EQ(qbond.getState()->getCFAPopulation(), 2);

    auto cfa = qbond.getCFA();
    EXPECT_EQ(cfa.commissionFreeAddresses.get(0), testAddress2);
    EXPECT_EQ(cfa.commissionFreeAddresses.get(1), adminAddress);
    EXPECT_EQ(cfa.commissionFreeAddresses.get(2), NULL_ID);

    EXPECT_FALSE(qbond.updateCFA(testAddress1, testAddress2, 0));
    EXPECT_EQ(qbond.getState()->getCFAPopulation(), 2);
    EXPECT_TRUE(qbond.updateCFA(adminAddress, testAddress2, 0));
    EXPECT_EQ(qbond.getState()->getCFAPopulation(), 1);
}

TEST(ContractQBond, GetInfoPerEpoch)
{
    ContractTestingQBond qbond;
    qbond.beginEpoch();
    increaseEnergy(testAddress1, 1000000000);
    increaseEnergy(testAddress2, 1000000000);

    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).stakersAmount, 0);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).totalStaked, 0);

    qbond.stake(testAddress1, 50, 50250000);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).stakersAmount, 1);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).totalStaked, 50);

    qbond.stake(testAddress2, 100, 100500000);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).stakersAmount, 2);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).totalStaked, 150);

    EXPECT_EQ(qbond.transfer(testAddress1, testAddress2, system.epoch, 50, 100).transferredMBonds, 50);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).stakersAmount, 1);
    EXPECT_EQ(qbond.getInfoPerEpoch(system.epoch).totalStaked, 150);
}

TEST(ContractQBond, GetMBondsTable)
{
    ContractTestingQBond qbond;
    qbond.beginEpoch();
    increaseEnergy(testAddress1, 1000000000);
    increaseEnergy(testAddress2, 1000000000);

    qbond.stake(testAddress1, 50, 50250000);
    qbond.stake(testAddress2, 100, 100500000);
    qbond.endEpoch();

    system.epoch++;
    qbond.beginEpoch();
    qbond.stake(testAddress1, 10, 10050000);
    qbond.stake(testAddress2, 20, 20100000);

    auto table = qbond.getMBondsTable();
    EXPECT_EQ(table.info.get(0).epoch, 182);
    EXPECT_EQ(table.info.get(1).epoch, 183);
    EXPECT_EQ(table.info.get(2).epoch, 0);

    auto userMBonds = qbond.getUserMBonds(testAddress1);
    EXPECT_EQ(userMBonds.totalMBondsAmount, 60);
    EXPECT_EQ(userMBonds.mbonds.get(0).epoch, 182);
    EXPECT_EQ(userMBonds.mbonds.get(0).amount, 50);
    EXPECT_EQ(userMBonds.mbonds.get(1).epoch, 183);
    EXPECT_EQ(userMBonds.mbonds.get(1).amount, 10);
}
