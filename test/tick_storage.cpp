#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 50
#undef TICKS_TO_KEEP_FROM_PRIOR_EPOCH
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 5
#include "../src/tick_storage.h"

#include <random>


class TestTickStorage : public TickStorage
{
    unsigned char transactionBuffer[MAX_TRANSACTION_SIZE];
public:

    void addTransaction(unsigned int tick, unsigned int transactionIdx, unsigned int inputSize)
    {
        ASSERT_TRUE(inputSize <= MAX_INPUT_SIZE);
        Transaction* transaction = (Transaction*)transactionBuffer;
        transaction->amount = 10;
        transaction->destinationPublicKey.setRandomValue();
        transaction->sourcePublicKey.setRandomValue();
        transaction->inputSize = inputSize;
        transaction->inputType = 0;
        transaction->tick = tick;

        unsigned int transactionSize = transaction->totalSize();

        auto* offsets = tickTransactionOffsets.getByTickInCurrentEpoch(tick);
        if (nextTickTransactionOffset + transactionSize <= tickTransactions.storageSpaceCurrentEpoch)
        {
            EXPECT_EQ(offsets[transactionIdx], 0);
            offsets[transactionIdx] = nextTickTransactionOffset;
            copyMem(tickTransactions(nextTickTransactionOffset), transaction, transactionSize);
            nextTickTransactionOffset += transactionSize;
        }
    }
};

TestTickStorage ts;

void addTick(unsigned int tick, unsigned long long seed, unsigned short maxTransactions)
{
    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // add tick data
    TickData& td = ts.tickData.getByTickInCurrentEpoch(tick);
    td.epoch = 1234;
    td.tick = tick;

    // add computor ticks
    Tick* computorTicks = ts.ticks.getByTickInCurrentEpoch(tick);
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        computorTicks[i].epoch = 1234;
        computorTicks[i].computorIndex = i;
        computorTicks[i].tick = tick;
        computorTicks[i].prevResourceTestingDigest = gen64();
    }

    // add transactions of tick
    unsigned int transactionNum = gen64() % (maxTransactions + 1);
    unsigned int orderMode = gen64() % 2;
    unsigned int transactionSlot;
    for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
    {
        if (orderMode == 0)
            transactionSlot = transaction;  // standard order
        else if (orderMode == 1)
            transactionSlot = transactionNum - 1 - transaction;  // backward order
        ts.addTransaction(tick, transactionSlot, gen64() % MAX_INPUT_SIZE);
    }
    ts.checkStateConsistencyWithAssert();
}

void checkTick(unsigned int tick, unsigned long long seed, unsigned short maxTransactions, bool previousEpoch = false)
{
    // only last ticks of previous epoch are kept in storage -> check okay
    if (previousEpoch && !ts.tickInPreviousEpochStorage(tick))
        return;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // check tick data
    TickData& td = previousEpoch ? ts.tickData.getByTickInPreviousEpoch(tick) : ts.tickData.getByTickInCurrentEpoch(tick);
    EXPECT_EQ((int)td.epoch, (int)1234);
    EXPECT_EQ(td.tick, tick);

    // check computor ticks
    Tick* computorTicks = previousEpoch ? ts.ticks.getByTickInPreviousEpoch(tick) : ts.ticks.getByTickInCurrentEpoch(tick);
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        EXPECT_EQ((int)computorTicks[i].epoch, (int)1234);
        EXPECT_EQ((int)computorTicks[i].computorIndex, (int)i);
        EXPECT_EQ(computorTicks[i].tick, tick);
        EXPECT_EQ(computorTicks[i].prevResourceTestingDigest, gen64());
    }

    // check transactions of tick
    {
        const auto* offsets = previousEpoch ? ts.tickTransactionOffsets.getByTickInPreviousEpoch(tick) : ts.tickTransactionOffsets.getByTickInCurrentEpoch(tick);
        unsigned int transactionNum = gen64() % (maxTransactions + 1);
        unsigned int orderMode = gen64() % 2;
        unsigned int transactionSlot;

        for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
        {
            int expectedInputSize = (int)(gen64() % MAX_INPUT_SIZE);

            if (orderMode == 0)
                transactionSlot = transaction;  // standard order
            else if (orderMode == 1)
                transactionSlot = transactionNum - 1 - transaction;  // backward order

            // If previousEpoch, some transactions at the beginning may not have fit into the storage and are missing -> check okay
            // If current epoch, some may be missing at he end due to limited storage -> check okay
            if (!offsets[transactionSlot])
                continue;

            Transaction* tp = ts.tickTransactions(offsets[transactionSlot]);
            EXPECT_TRUE(tp->checkValidity());
            EXPECT_EQ(tp->tick, tick);
            EXPECT_EQ((int)tp->inputSize, expectedInputSize);
        }
    }
}


TEST(TestCoreTickStorage, EpochTransition) {

    unsigned long long seed = 42;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 2 epoch transitions
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned short maxTransactions = (testIdx == 0) ? 0 : NUMBER_OF_TRANSACTIONS_PER_TICK;

        ts.init();
        ts.checkStateConsistencyWithAssert();

        const int firstEpochTicks = gen64() % (MAX_NUMBER_OF_TICKS_PER_EPOCH + 1);
        const int secondEpochTicks = gen64() % (MAX_NUMBER_OF_TICKS_PER_EPOCH + 1);
        const int thirdEpochTicks = gen64() % (MAX_NUMBER_OF_TICKS_PER_EPOCH + 1);
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        const unsigned int secondEpochTick0 = firstEpochTick0 + firstEpochTicks;
        const unsigned int thirdEpochTick0 = secondEpochTick0 + secondEpochTicks;
        unsigned long long firstEpochSeeds[MAX_NUMBER_OF_TICKS_PER_EPOCH];
        unsigned long long secondEpochSeeds[MAX_NUMBER_OF_TICKS_PER_EPOCH];
        unsigned long long thirdEpochSeeds[MAX_NUMBER_OF_TICKS_PER_EPOCH];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();
        for (int i = 0; i < secondEpochTicks; ++i)
            secondEpochSeeds[i] = gen64();
        for (int i = 0; i < thirdEpochTicks; ++i)
            thirdEpochSeeds[i] = gen64();

        // first epoch
        ts.beginEpoch(firstEpochTick0);
        ts.checkStateConsistencyWithAssert();

        // add ticks
        for (int i = 0; i < firstEpochTicks; ++i)
            addTick(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);

        // check ticks
        for (int i = 0; i < firstEpochTicks; ++i)
            checkTick(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);

        // Epoch transistion
        ts.beginEpoch(secondEpochTick0);
        ts.checkStateConsistencyWithAssert();

        // add ticks
        for (int i = 0; i < secondEpochTicks; ++i)
            addTick(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions);

        // check ticks
        for (int i = 0; i < secondEpochTicks; ++i)
            checkTick(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions);
        bool previousEpoch = true;
        for (int i = 0; i < firstEpochTicks; ++i)
            checkTick(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions, previousEpoch);

        // Epoch transistion
        ts.beginEpoch(thirdEpochTick0);
        ts.checkStateConsistencyWithAssert();

        // add ticks
        for (int i = 0; i < thirdEpochTicks; ++i)
            addTick(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions);

        // check ticks
        for (int i = 0; i < thirdEpochTicks; ++i)
            checkTick(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions);
        for (int i = 0; i < secondEpochTicks; ++i)
            checkTick(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions, previousEpoch);

        ts.deinit();
    }
}
