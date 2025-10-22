#define NO_UEFI

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

#include "../src/contract_core/contract_def.h"
#include "../src/contract_core/contract_exec.h"

#include "../src/public_settings.h"
#undef PENDING_TXS_POOL_NUM_TICKS
#define PENDING_TXS_POOL_NUM_TICKS 50ULL
#undef NUMBER_OF_TRANSACTIONS_PER_TICK
#define NUMBER_OF_TRANSACTIONS_PER_TICK 128ULL
#include "../src/ticking/pending_txs_pool.h"

#include <random>
#include <vector>

static constexpr unsigned int NUM_INITIALIZED_ENTITIES = 200U;

class TestPendingTxsPool : public PendingTxsPool
{
    unsigned char transactionBuffer[MAX_TRANSACTION_SIZE];
public:
    TestPendingTxsPool()
    {
        // we need the spectrum for tx priority calculation
        EXPECT_TRUE(initSpectrum());
        memset(spectrum, 0, spectrumSizeInBytes);
        for (unsigned int i = 0; i < NUM_INITIALIZED_ENTITIES; i++)
        {
            // create NUM_INITIALIZED_ENTITIES entities with balance > 0 to get desired txs priority
            spectrum[i].incomingAmount = i + 1;
            spectrum[i].outgoingAmount = 0;
            spectrum[i].publicKey = m256i{0, 0, 0, i + 1 };

            // create NUM_INITIALIZED_ENTITIES entities with balance = 0 for testing
            spectrum[NUM_INITIALIZED_ENTITIES + i].incomingAmount = 0;
            spectrum[NUM_INITIALIZED_ENTITIES + i].outgoingAmount = 0;
            spectrum[NUM_INITIALIZED_ENTITIES + i].publicKey = m256i{ 0, 0, 0, NUM_INITIALIZED_ENTITIES + i + 1 };
        }
        updateSpectrumInfo();
    }

    ~TestPendingTxsPool()
    {
        deinitSpectrum();
    }

    static constexpr unsigned int getMaxNumTxsPerTick()
    {
        return maxNumTxsPerTick;
    }

    bool addTransaction(unsigned int tick, long long amount, unsigned int inputSize, const m256i* dest = nullptr, const m256i* src = nullptr)
    {
        Transaction* transaction = (Transaction*)transactionBuffer;
        transaction->amount = amount;
        if (dest == nullptr)
            transaction->destinationPublicKey.setRandomValue();
        else
            transaction->destinationPublicKey.assign(*dest);
        if (src == nullptr)
            transaction->sourcePublicKey.setRandomValue();
        else
            transaction->sourcePublicKey.assign(*src);
        transaction->inputSize = inputSize;
        transaction->inputType = 0;
        transaction->tick = tick;

        return add(transaction);
    }

    unsigned int addTickTransactions(unsigned int tick, unsigned long long seed, unsigned int maxTransactions)
    {
        // use pseudo-random sequence
        std::mt19937_64 gen64(seed);

        unsigned int numTransactionsAdded = 0;

        // add transactions of tick
        unsigned int transactionNum = gen64() % (maxTransactions + 1);
        for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
        {
            unsigned int inputSize = gen64() % MAX_INPUT_SIZE;
            long long amount = gen64() % MAX_AMOUNT;
            m256i srcPublicKey = m256i{ 0, 0, 0, (gen64() % NUM_INITIALIZED_ENTITIES) + 1 };
            if (addTransaction(tick, amount, inputSize, /*dest=*/nullptr, &srcPublicKey))
                numTransactionsAdded++;
        }
        checkStateConsistencyWithAssert();

        return numTransactionsAdded;
    }

    void checkTickTransactions(unsigned int tick, unsigned long long seed, unsigned int maxTransactions)
    {
        // use pseudo-random sequence
        std::mt19937_64 gen64(seed);

        // check transactions of tick
        unsigned int transactionNum = gen64() % (maxTransactions + 1);

        for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
        {
            unsigned int expectedInputSize = gen64() % MAX_INPUT_SIZE;
            long long expectedAmount = gen64() % MAX_AMOUNT;
            m256i expectedSrcPublicKey = m256i{ 0, 0, 0, (gen64() % NUM_INITIALIZED_ENTITIES) + 1 };

            Transaction* tp = getTx(tick, transaction);

            ASSERT_NE(tp, nullptr);

            EXPECT_TRUE(tp->checkValidity());
            EXPECT_EQ(tp->tick, tick);
            EXPECT_EQ(static_cast<unsigned int>(tp->inputSize), expectedInputSize);
            EXPECT_EQ(tp->amount, expectedAmount);
            EXPECT_TRUE(tp->sourcePublicKey == expectedSrcPublicKey);

            m256i* digest = getDigest(tick, transaction);

            ASSERT_NE(digest, nullptr);

            m256i tpDigest;
            KangarooTwelve(tp, tp->totalSize(), &tpDigest, 32);
            EXPECT_EQ(*digest, tpDigest);
        }
    }
};


TEST(TestPendingTxsPool, EpochTransition) 
{
    TestPendingTxsPool pendingTxsPool;

    unsigned long long seed = 42;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 3 epoch transitions
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned int maxTransactions = (testIdx == 0) ? 0 : pendingTxsPool.getMaxNumTxsPerTick();

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        constexpr unsigned int firstEpochTicks = PENDING_TXS_POOL_NUM_TICKS;
        // second epoch start will reset the pool completely because secondEpochTick0 is not contained
        constexpr unsigned int secondEpochTicks = PENDING_TXS_POOL_NUM_TICKS / 2;
        // thirdEpochTick0 will be contained with newInitialIndex >= buffersBeginIndex
        constexpr unsigned int thirdEpochTicks = PENDING_TXS_POOL_NUM_TICKS / 2 + PENDING_TXS_POOL_NUM_TICKS / 4;
        // fourthEpochTick0 will be contained with newInitialIndex < buffersBeginIndex
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        const unsigned int secondEpochTick0 = firstEpochTick0 + firstEpochTicks;
        const unsigned int thirdEpochTick0 = secondEpochTick0 + secondEpochTicks;
        const unsigned int fourthEpochTick0 = thirdEpochTick0 + thirdEpochTicks;
        unsigned long long firstEpochSeeds[firstEpochTicks];
        unsigned long long secondEpochSeeds[secondEpochTicks];
        unsigned long long thirdEpochSeeds[thirdEpochTicks];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();
        for (int i = 0; i < secondEpochTicks; ++i)
            secondEpochSeeds[i] = gen64();
        for (int i = 0; i < thirdEpochTicks; ++i)
            thirdEpochSeeds[i] = gen64();
        unsigned int numAdded = 0;

        // first epoch
        pendingTxsPool.beginEpoch(firstEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        // add ticks transactions
        for (int i = 0; i < firstEpochTicks; ++i)
            numAdded = pendingTxsPool.addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);

        // check ticks transactions
        for (int i = 0; i < firstEpochTicks; ++i)
            pendingTxsPool.checkTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);

        pendingTxsPool.checkStateConsistencyWithAssert();

        // Epoch transistion
        pendingTxsPool.beginEpoch(secondEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(secondEpochTick0), 0);

        // add ticks transactions
        for (int i = 0; i < secondEpochTicks; ++i)
            numAdded = pendingTxsPool.addTickTransactions(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions);

        // check ticks transactions
        for (int i = 0; i < secondEpochTicks; ++i)
            pendingTxsPool.checkTickTransactions(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions);

        // add a transaction for the next epoch
        numAdded = pendingTxsPool.addTickTransactions(thirdEpochTick0 + 1, thirdEpochSeeds[1], maxTransactions);

        pendingTxsPool.checkStateConsistencyWithAssert();

        // Epoch transistion
        pendingTxsPool.beginEpoch(thirdEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(thirdEpochTick0), numAdded);

        // add ticks transactions
        for (int i = 2; i < thirdEpochTicks; ++i)
            numAdded = pendingTxsPool.addTickTransactions(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions);

        // check ticks transactions
        for (int i = 1; i < thirdEpochTicks; ++i)
            pendingTxsPool.checkTickTransactions(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions);

        // add a transaction for the next epoch
        numAdded = pendingTxsPool.addTickTransactions(fourthEpochTick0 + 1, /*seed=*/42, maxTransactions);

        pendingTxsPool.checkStateConsistencyWithAssert();

        // Epoch transistion
        pendingTxsPool.beginEpoch(fourthEpochTick0);
        pendingTxsPool.checkStateConsistencyWithAssert();

        EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(fourthEpochTick0), numAdded);

        pendingTxsPool.deinit();
    }
}

TEST(TestPendingTxsPool, TotalNumberOfPendingTxs) 
{
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 1337;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 1 epoch
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned int maxTransactions = (testIdx == 0) ? 0 : pendingTxsPool.getMaxNumTxsPerTick();

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        const int firstEpochTicks = (gen64() % (4 * PENDING_TXS_POOL_NUM_TICKS)) + 1;
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        unsigned long long firstEpochSeeds[4 * PENDING_TXS_POOL_NUM_TICKS];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();

        // first epoch
        pendingTxsPool.beginEpoch(firstEpochTick0);

        // add ticks transactions
        std::vector<unsigned short> numTransactionsAdded(firstEpochTicks);
        std::vector<unsigned short> numPendingTransactions(firstEpochTicks, 0);
        for (int i = firstEpochTicks - 1; i >= 0; --i)
        {
            numTransactionsAdded[i] = pendingTxsPool.addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);
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
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 67534;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 1 epoch
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned int maxTransactions = (testIdx == 0) ? 0 : pendingTxsPool.getMaxNumTxsPerTick();

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        constexpr unsigned int firstEpochTicks = PENDING_TXS_POOL_NUM_TICKS;
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        unsigned long long firstEpochSeeds[firstEpochTicks];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();

        // first epoch
        pendingTxsPool.beginEpoch(firstEpochTick0);

        // add ticks transactions
        std::vector<unsigned short> numTransactionsAdded(firstEpochTicks);
        for (int i = firstEpochTicks - 1; i >= 0; --i)
        {
            numTransactionsAdded[i] = pendingTxsPool.addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);
        }

        EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0 - 1), 0);
        for (int i = 0; i < firstEpochTicks; ++i)
        {
            EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0 + i), (unsigned int)numTransactionsAdded[i]);
        }

        pendingTxsPool.deinit();
    }
}

TEST(TestPendingTxsPool, IncrementFirstStoredTick)
{
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 84129;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 1 epoch
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions
        unsigned int maxTransactions = (testIdx == 0) ? 0 : pendingTxsPool.getMaxNumTxsPerTick();

        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        const int firstEpochTicks = (gen64() % (4 * PENDING_TXS_POOL_NUM_TICKS)) + 1;
        const unsigned int firstEpochTick0 = gen64() % 10000000;
        unsigned long long firstEpochSeeds[4 * PENDING_TXS_POOL_NUM_TICKS];
        for (int i = 0; i < firstEpochTicks; ++i)
            firstEpochSeeds[i] = gen64();

        // first epoch
        pendingTxsPool.beginEpoch(firstEpochTick0);

        // add ticks transactions
        std::vector<unsigned short> numTransactionsAdded(firstEpochTicks);
        std::vector<unsigned short> numPendingTransactions(firstEpochTicks, 0);
        for (int i = firstEpochTicks - 1; i >= 0; --i)
        {
            numTransactionsAdded[i] = pendingTxsPool.addTickTransactions(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions);
            if (i > 0)
            {
                numPendingTransactions[i - 1] = numPendingTransactions[i] + numTransactionsAdded[i];
            }
        }

        EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 - 1), (unsigned int)numTransactionsAdded[0] + numPendingTransactions[0]);
        for (int i = 0; i < firstEpochTicks; ++i)
        {
            pendingTxsPool.incrementFirstStoredTick();
            for (int tx = 0; tx < numTransactionsAdded[i]; ++tx)
            {
                EXPECT_EQ(pendingTxsPool.getTx(firstEpochTick0 + i, 0), nullptr);
                EXPECT_EQ(pendingTxsPool.getDigest(firstEpochTick0 + i, 0), nullptr);
            }
            EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 + i), (unsigned int)numPendingTransactions[i]);
        }

        pendingTxsPool.deinit();
    }
}

TEST(TestPendingTxsPool, TxsPrioritizationMoreThanMaxTxs)
{
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 9532;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    pendingTxsPool.init();
    pendingTxsPool.checkStateConsistencyWithAssert();

    const unsigned int firstEpochTick0 = gen64() % 10000000;
    unsigned int numAdditionalTxs = 64;

    pendingTxsPool.beginEpoch(firstEpochTick0);

    // add more than `pendingTxsPool.getMaxNumTxsPerTick()` with increasing priority
    // (entities were set up in a way that u64._0 of the public key corresponds to their balance)
    m256i srcPublicKey = m256i::zero();
    for (unsigned int t = 0; t < pendingTxsPool.getMaxNumTxsPerTick() + numAdditionalTxs; ++t)
    {
        srcPublicKey.u64._3 = t + 1;
        EXPECT_TRUE(pendingTxsPool.addTransaction(firstEpochTick0, /*amount=*/t + 1, /*inputSize=*/0, /*dest=*/nullptr, &srcPublicKey));
    }

    // adding lower priority tx does not work
    srcPublicKey.u64._3 = 1;
    EXPECT_FALSE(pendingTxsPool.addTransaction(firstEpochTick0, /*amount=*/1, /*inputSize=*/0, /*dest=*/nullptr, &srcPublicKey));

    EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 - 1), pendingTxsPool.getMaxNumTxsPerTick());
    EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0), pendingTxsPool.getMaxNumTxsPerTick());

    for (unsigned int t = 0; t < pendingTxsPool.getMaxNumTxsPerTick(); ++t)
    {
        if (t < numAdditionalTxs)
            EXPECT_EQ(pendingTxsPool.getTx(firstEpochTick0, t)->amount, pendingTxsPool.getMaxNumTxsPerTick() + t + 1);
        else
            EXPECT_EQ(pendingTxsPool.getTx(firstEpochTick0, t)->amount, t + 1);
    }

    pendingTxsPool.deinit();
}

TEST(TestPendingTxsPool, TxsPrioritizationDuplicateTxs)
{
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 9532;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    pendingTxsPool.init();
    pendingTxsPool.checkStateConsistencyWithAssert();

    const unsigned int firstEpochTick0 = gen64() % 10000000;
    constexpr unsigned int numTxs = pendingTxsPool.getMaxNumTxsPerTick() / 2;

    pendingTxsPool.beginEpoch(firstEpochTick0);

    // add duplicate transactions: same dest, src, and amount
    m256i dest{ 562, 789, 234, 121 };
    m256i src{ 0, 0, 0, NUM_INITIALIZED_ENTITIES / 3 };
    long long amount = 1;
    for (unsigned int t = 0; t < numTxs; ++t)
        EXPECT_TRUE(pendingTxsPool.addTransaction(firstEpochTick0, amount, /*inputSize=*/0, &dest, & src));

    EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 - 1), numTxs);
    EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0), numTxs);

    for (unsigned int t = 0; t < numTxs; ++t)
    {
        Transaction* tx = pendingTxsPool.getTx(firstEpochTick0, t);
        EXPECT_TRUE(tx->checkValidity());
        EXPECT_EQ(tx->amount, amount);
        EXPECT_EQ(tx->tick, firstEpochTick0);
        EXPECT_EQ(static_cast<unsigned int>(tx->inputSize), 0U);
        EXPECT_TRUE(tx->destinationPublicKey == dest);
        EXPECT_TRUE(tx->sourcePublicKey == src);
    }

    pendingTxsPool.deinit();
}

TEST(TestPendingTxsPool, ProtocolLevelTxsMaxPriority)
{
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 9532;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    pendingTxsPool.init();
    pendingTxsPool.checkStateConsistencyWithAssert();

    const unsigned int firstEpochTick0 = gen64() % 10000000;

    pendingTxsPool.beginEpoch(firstEpochTick0);

    // fill the PendingTxsPool completely for tick `firstEpochTick0`
    m256i srcPublicKey = m256i::zero();
    for (unsigned int t = 0; t < pendingTxsPool.getMaxNumTxsPerTick(); ++t)
    {
        srcPublicKey.u64._3 = t + 1;
        EXPECT_TRUE(pendingTxsPool.addTransaction(firstEpochTick0, gen64() % MAX_AMOUNT, gen64() % MAX_INPUT_SIZE, /*dest=*/nullptr, &srcPublicKey));
    }

    EXPECT_EQ(pendingTxsPool.getTotalNumberOfPendingTxs(firstEpochTick0 - 1), pendingTxsPool.getMaxNumTxsPerTick());
    EXPECT_EQ(pendingTxsPool.getNumberOfPendingTickTxs(firstEpochTick0), pendingTxsPool.getMaxNumTxsPerTick());

    Transaction tx { 
        .sourcePublicKey = m256i{ 0, 0, 0, (gen64() % NUM_INITIALIZED_ENTITIES) + 1 },
        .destinationPublicKey = m256i::zero(),
        .amount = 0, .tick = firstEpochTick0, 
        .inputType = VOTE_COUNTER_INPUT_TYPE,
        .inputSize = 0,
    };

    EXPECT_TRUE(pendingTxsPool.add(&tx));

    tx.inputType = CustomMiningSolutionTransaction::transactionType();

    EXPECT_TRUE(pendingTxsPool.add(&tx));

    pendingTxsPool.deinit();
}

TEST(TestPendingTxsPool, TxsWithSrcBalance0AreRejected)
{
    TestPendingTxsPool pendingTxsPool;
    unsigned long long seed = 3452;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        pendingTxsPool.init();
        pendingTxsPool.checkStateConsistencyWithAssert();

        const unsigned int firstEpochTick0 = gen64() % 10000000;

        pendingTxsPool.beginEpoch(firstEpochTick0);

        // partially fill the PendingTxsPool for tick `firstEpochTick0`    
        m256i srcPublicKey = m256i::zero();
        for (unsigned int t = 0; t < pendingTxsPool.getMaxNumTxsPerTick() / 2; ++t)
        {
            srcPublicKey.u64._3 = t + 1;
            EXPECT_TRUE(pendingTxsPool.addTransaction(firstEpochTick0, gen64() % MAX_AMOUNT, gen64() % MAX_INPUT_SIZE, /*dest=*/nullptr, &srcPublicKey));
        }

        // public key with balance 0
        srcPublicKey.u64._3 = NUM_INITIALIZED_ENTITIES + 1 + (gen64() % NUM_INITIALIZED_ENTITIES);
        EXPECT_FALSE(pendingTxsPool.addTransaction(firstEpochTick0, gen64() % MAX_AMOUNT, gen64() % MAX_INPUT_SIZE, /*dest=*/nullptr, &srcPublicKey));

        // non-existant public key
        srcPublicKey = m256i{ 0, gen64() % MAX_AMOUNT, 0, 0};
        EXPECT_FALSE(pendingTxsPool.addTransaction(firstEpochTick0, gen64() % MAX_AMOUNT, gen64() % MAX_INPUT_SIZE, /*dest=*/nullptr, &srcPublicKey));

        pendingTxsPool.deinit();
    }
}