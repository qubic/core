#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 50
#undef TICKS_TO_KEEP_FROM_PRIOR_EPOCH
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 5
#undef TRANSACTION_SPARSENESS
#define TRANSACTION_SPARSENESS 4
#include "../src/ticking/pending_txs_pool.h"

#include <random>
#include <vector>


class TestPendingTxsPool : public PendingTxsPool
{
    unsigned char transactionBuffer[MAX_TRANSACTION_SIZE];
public:

    bool addTransaction(unsigned int tick, unsigned int inputSize)
    {
        Transaction* transaction = (Transaction*)transactionBuffer;
        transaction->amount = 10;
        transaction->destinationPublicKey.setRandomValue();
        transaction->sourcePublicKey.setRandomValue();
        transaction->inputSize = inputSize;
        transaction->inputType = 0;
        transaction->tick = tick;

        return update(transaction);
    }
};

TestPendingTxsPool pendingTxsPool;

void addTickTransactions(unsigned int tick, unsigned long long seed, unsigned short maxTransactions, unsigned short* numTransactionsAdded = nullptr)
{
    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    if (numTransactionsAdded)
        *numTransactionsAdded = 0;

    // add transactions of tick
    unsigned int transactionNum = gen64() % (maxTransactions + 1);
    for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
    {
        unsigned int inputSize = gen64() % MAX_INPUT_SIZE;
        if (pendingTxsPool.addTransaction(tick, inputSize))
            if (numTransactionsAdded)
                *numTransactionsAdded += 1;
    }
    pendingTxsPool.checkStateConsistencyWithAssert();
}

void checkTickTransactions(unsigned int tick, unsigned long long seed, unsigned short maxTransactions, bool previousEpoch = false)
{
    // only last ticks of previous epoch are kept in storage -> check okay
    if (previousEpoch && !pendingTxsPool.tickInPreviousEpochStorage(tick))
        return;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // check transactions of tick
    {
        unsigned int transactionNum = gen64() % (maxTransactions + 1);

        for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
        {
            int expectedInputSize = (int)(gen64() % MAX_INPUT_SIZE);

            Transaction* tp = pendingTxsPool.get(tick, transaction);

            // If previousEpoch, some transactions at the beginning may not have fit into the storage and are missing -> check okay
            // If current epoch, some may be missing at the end due to limited storage -> check okay
            if (!tp)
                continue;

            EXPECT_TRUE(tp->checkValidity());
            EXPECT_EQ(tp->tick, tick);
            if (!previousEpoch)
            {
                // in previous epoch, if transactions at the beginning were cut the expectedInputSize will noch match anymore
                EXPECT_EQ((int)tp->inputSize, expectedInputSize);
            }

            m256i* digest = pendingTxsPool.getDigest(tick, transaction);

            EXPECT_TRUE(digest);
            if (!digest)
                continue;

            m256i tpDigest;
            KangarooTwelve(tp, tp->totalSize(), &tpDigest, 32);
            EXPECT_EQ(*digest, tpDigest);
        }
    }
}


TEST(TestPendingTxsPool, EpochTransition) 
{
    unsigned long long seed = 42;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 2 epoch transitions
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned short maxTransactions = (testIdx == 0) ? 0 : NUMBER_OF_TRANSACTIONS_PER_TICK;

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

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
        pendingTxsPool.beginEpoch(firstEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        // add ticks transactions
        for (int i = 0; i < firstEpochTicks; ++i)
            addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);

        // check ticks transactions
        for (int i = 0; i < firstEpochTicks; ++i)
            checkTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);

        pendingTxsPool.checkStateConsistencyWithAssert();

        // Epoch transistion
        pendingTxsPool.beginEpoch(secondEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        // add ticks transactions
        for (int i = 0; i < secondEpochTicks; ++i)
            addTickTransactions(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions);

        // check ticks transactions
        for (int i = 0; i < secondEpochTicks; ++i)
            checkTickTransactions(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions);
        bool previousEpoch = true;
        for (int i = 0; i < firstEpochTicks; ++i)
            checkTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions, previousEpoch);

        pendingTxsPool.checkStateConsistencyWithAssert();

        // Epoch transistion
        pendingTxsPool.beginEpoch(thirdEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        // add ticks transactions
        for (int i = 0; i < thirdEpochTicks; ++i)
            addTickTransactions(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions);

        // check ticks transactions
        for (int i = 0; i < thirdEpochTicks; ++i)
            checkTickTransactions(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions);
        for (int i = 0; i < secondEpochTicks; ++i)
            checkTickTransactions(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions, previousEpoch);

        pendingTxsPool.checkStateConsistencyWithAssert();

        pendingTxsPool.deinit();
    }
}

TEST(TestPendingTxsPool, TotalNumberOfPendingTxs) 
{
    unsigned long long seed = 1337;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 1 epoch transitions
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned short maxTransactions = (testIdx == 0) ? 0 : NUMBER_OF_TRANSACTIONS_PER_TICK;

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        const int firstEpochTicks = gen64() % (MAX_NUMBER_OF_TICKS_PER_EPOCH + 1);
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        unsigned long long firstEpochSeeds[MAX_NUMBER_OF_TICKS_PER_EPOCH];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();

        // first epoch
        pendingTxsPool.beginEpoch(firstEpochTick0);

        // add ticks transactions
        std::vector<unsigned short> numTransactionsAdded(firstEpochTicks);
        std::vector<unsigned short> numPendingTransactions(firstEpochTicks, 0);
        for (int i = firstEpochTicks - 1; i >= 0; --i)
        {
            addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions, &numTransactionsAdded[i]);
            if (i > 0)
            {
                numPendingTransactions[i - 1] = numPendingTransactions[i] + numTransactionsAdded[i];
            }
        }

        EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 - 1), (unsigned int)numTransactionsAdded[0] + numPendingTransactions[0]);
        for (int i = 0; i < firstEpochTicks; ++i)
        {
            EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 + i), (unsigned int)numPendingTransactions[i]);
        }

        pendingTxsPool.deinit();
    }
}

TEST(TestPendingTxsPool, NumberOfPendingTickTxs) 
{
    unsigned long long seed = 67534;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 1 epoch transitions
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned short maxTransactions = (testIdx == 0) ? 0 : NUMBER_OF_TRANSACTIONS_PER_TICK;

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        const int firstEpochTicks = gen64() % (MAX_NUMBER_OF_TICKS_PER_EPOCH + 1);
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        unsigned long long firstEpochSeeds[MAX_NUMBER_OF_TICKS_PER_EPOCH];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();

        // first epoch
        pendingTxsPool.beginEpoch(firstEpochTick0);

        // add ticks transactions
        std::vector<unsigned short> numTransactionsAdded(firstEpochTicks);
        for (int i = firstEpochTicks - 1; i >= 0; --i)
        {
            addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions, &numTransactionsAdded[i]);
        }

        EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0 - 1), 0);
        for (int i = 0; i < firstEpochTicks; ++i)
        {
            EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0 + i), (unsigned int)numTransactionsAdded[i]);
        }

        pendingTxsPool.deinit();
    }
}