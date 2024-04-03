#define NO_UEFI

#include "gtest/gtest.h"

#define system systemStructInstance
#define Peer void
void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);

#include "../src/public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 50
#undef TICKS_TO_KEEP_FROM_PRIOR_EPOCH
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 32
#include "../src/exchange_connect.h"

#include <random>

unsigned int numberOfTransactions = 0;


// Add tick with random transactions, return whether all transactions could be stored
static bool addTick(unsigned int tick, unsigned long long seed, unsigned short maxTransactions)
{
    system.tick = tick;

    // set start index of tick transactions
    tickTxIndexStart[system.tick - system.initialTick] = numberOfTransactions;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // add transactions of tick
    unsigned int transactionNum = gen64() % (maxTransactions + 1);
    for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
    {
        ++numberOfTransactions;
        m256i digest(gen64(), gen64(), gen64(), gen64());
        if (!saveConfirmedTx(numberOfTransactions - 1, gen64() % 2, system.tick, digest))
            return false;
    }

    return true;
}


struct {
    RequestResponseHeader header;
    RequestTxStatus payload;
} requestMessage;

int expectedMoneyFlew = 0;
bool expectedToBeFound = true;


static void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data)
{
    EXPECT_EQ(type, RESPOND_TX_STATUS);
    EXPECT_EQ(dejavu, requestMessage.header.dejavu());
    EXPECT_EQ(dataSize, sizeof(RespondTxStatus));

    const RespondTxStatus* txStatus = (const RespondTxStatus*)data;
    EXPECT_EQ(txStatus->digest, requestMessage.payload.digest);
    EXPECT_EQ(txStatus->tickOfTx, requestMessage.payload.tick);
    if (expectedToBeFound)
    {
        EXPECT_EQ(txStatus->moneyFlew, expectedMoneyFlew);
        EXPECT_EQ(txStatus->notfound, 0);
        EXPECT_EQ(txStatus->executed, 1);
    }
}

static void checkTick(unsigned int tick, unsigned long long seed, unsigned short maxTransactions, bool fullyStoredTick, bool previousEpoch)
{
    // Ensure that we do not skip processRequestConfirmedTx()
    if (system.tick <= tick)
        system.tick = tick + 1;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    requestMessage.header.checkAndSetSize(sizeof(requestMessage));
    requestMessage.header.setType(REQUEST_TX_STATUS);
    requestMessage.header.setDejavu(seed % UINT_MAX);
    requestMessage.payload.tick = tick;

    expectedToBeFound = fullyStoredTick;

    // check transactions of tick
    unsigned int transactionNum = gen64() % (maxTransactions + 1);
    for (unsigned int transaction = 0; transaction < transactionNum; ++transaction)
    {
        requestMessage.payload.digest = m256i(gen64(), gen64(), gen64(), gen64());
        expectedMoneyFlew = gen64() % 2;
        processRequestConfirmedTx(nullptr, &requestMessage.header);
    }

    unsigned int tickIndex;
    if (previousEpoch)
    {
        ASSERT(confirmedTxPreviousEpochBeginTick != 0);
        EXPECT_LT(tick, confirmedTxCurrentEpochBeginTick);
        if (tick < confirmedTxPreviousEpochBeginTick)
        {
            // tick not available -> okay
            return;
        }
        tickIndex = tick - confirmedTxPreviousEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }
    else
    {
        ASSERT(tick >= confirmedTxCurrentEpochBeginTick && tick < confirmedTxCurrentEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH);
        tickIndex = tick - confirmedTxCurrentEpochBeginTick;
    }
    if (fullyStoredTick)
    {
        EXPECT_EQ(tickTxCounter[tickIndex], transactionNum);
    }
    else
    {
        EXPECT_LE(tickTxCounter[tickIndex], transactionNum);
    }
}


TEST(TestCoreExchangeConnect, EpochTransition)
{
    unsigned long long seed = 42;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 2 epoch transitions
    for (int testIdx = 0; testIdx < 6; ++testIdx)
    {
        // first, test case of having no transactions, then of having few transaction, later of having many transactions
        unsigned short maxTransactions = 0;
        if (testIdx == 1)
            maxTransactions = NUMBER_OF_TRANSACTIONS_PER_TICK / 4;
        else if (testIdx > 1)
            maxTransactions = NUMBER_OF_TRANSACTIONS_PER_TICK;

        initExchangeConnect();

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
        numberOfTransactions = 0;
        system.initialTick = firstEpochTick0;
        beginEpochExchangeConnect(firstEpochTick0);

        // add ticks
        int firstEpochLastFullyStoredTick = -1;
        for (int i = 0; i < firstEpochTicks; ++i)
        {
            if (addTick(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions))
                firstEpochLastFullyStoredTick = i;
        }

        // check ticks
        bool previousEpoch = true;
        for (int i = 0; i < firstEpochTicks; ++i)
            checkTick(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions, i < firstEpochLastFullyStoredTick, !previousEpoch);

        // Epoch transistion
        numberOfTransactions = 0;
        system.initialTick = secondEpochTick0;
        beginEpochExchangeConnect(secondEpochTick0);

        // add ticks
        int secondEpochLastFullyStoredTick = -1;
        for (int i = 0; i < secondEpochTicks; ++i)
        {
            if (addTick(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions))
                secondEpochLastFullyStoredTick = i;
        }

        // check ticks
        for (int i = 0; i < secondEpochTicks; ++i)
            checkTick(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions, i < secondEpochLastFullyStoredTick, !previousEpoch);
        for (int i = 0; i < firstEpochTicks; ++i)
            checkTick(firstEpochTick0 + i, firstEpochSeeds[i], maxTransactions, i < firstEpochLastFullyStoredTick, previousEpoch);

        // Epoch transistion
        numberOfTransactions = 0;
        system.initialTick = thirdEpochTick0;
        beginEpochExchangeConnect(thirdEpochTick0);

        // add ticks
        int thirdEpochLastFullyStoredTick = -1;
        for (int i = 0; i < thirdEpochTicks; ++i)
        {
            if (addTick(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions))
                thirdEpochLastFullyStoredTick = i;
        }

        // check ticks
        for (int i = 0; i < thirdEpochTicks; ++i)
            checkTick(thirdEpochTick0 + i, thirdEpochSeeds[i], maxTransactions, i < thirdEpochLastFullyStoredTick, !previousEpoch);
        for (int i = 0; i < secondEpochTicks; ++i)
            checkTick(secondEpochTick0 + i, secondEpochSeeds[i], maxTransactions, i < secondEpochLastFullyStoredTick, previousEpoch);

        deinitExchangeConnect();
    }
}

