#define NO_UEFI

#include "gtest/gtest.h"

#define system qubicSystemStruct
#define Peer void
void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data);

#include "../src/public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 50
#undef TICKS_TO_KEEP_FROM_PRIOR_EPOCH
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 32
#undef ADDON_TX_STATUS_REQUEST
#define ADDON_TX_STATUS_REQUEST 1
#include "../src/addons/tx_status_request.h"

#include <random>

unsigned int numberOfTransactions = 0;


// Add tick with random transactions, return whether all transactions could be stored
static bool addTick(unsigned int tick, unsigned long long seed, unsigned short maxTransactions)
{
    system.tick = tick;

    // set start index of tick transactions
    txStatusData.tickTxIndexStart[system.tick - system.initialTick] = numberOfTransactions;

    // use pseudo-random sequence for generating test data
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

RespondTxStatus responseMessage;


static void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data)
{
    const RespondTxStatus* txStatus = (const RespondTxStatus*)data;

    EXPECT_EQ(type, RESPOND_TX_STATUS);
    EXPECT_EQ(dejavu, requestMessage.header.dejavu());
    EXPECT_EQ(dataSize, txStatus->size());

    copyMem(&responseMessage, txStatus, txStatus->size());
}

static void checkTick(unsigned int tick, unsigned long long seed, unsigned short maxTransactions, bool fullyStoredTick, bool previousEpoch)
{
    // Ensure that we do not skip processRequestConfirmedTx()
    if (system.tick <= tick)
        system.tick = tick + 1;

    // check tick number dependent on if it is previous epoch
    if (previousEpoch)
    {
        ASSERT(txStatusData.confirmedTxPreviousEpochBeginTick != 0);
        EXPECT_LT(tick, txStatusData.confirmedTxCurrentEpochBeginTick);
        if (tick < txStatusData.confirmedTxPreviousEpochBeginTick)
        {
            // tick not available -> okay
            return;
        }
    }
    else
    {
        ASSERT(tick >= txStatusData.confirmedTxCurrentEpochBeginTick && tick < txStatusData.confirmedTxCurrentEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH);
    }

    // prepare request message
    requestMessage.header.checkAndSetSize(sizeof(requestMessage));
    requestMessage.header.setType(REQUEST_TX_STATUS);
    requestMessage.header.setDejavu(seed % UINT_MAX);
    requestMessage.payload.tick = tick;

    // get status of tick transactions
    responseMessage.tick = responseMessage.currentTickOfNode = 0;
    processRequestConfirmedTx(0, nullptr, &requestMessage.header);

    // check that we received right data
    EXPECT_EQ(responseMessage.tick, tick);
    EXPECT_EQ(responseMessage.currentTickOfNode, system.tick);

    // use pseudo-random sequence for generating test data
    std::mt19937_64 gen64(seed);

    // check number of transactions
    unsigned int transactionNum = gen64() % (maxTransactions + 1);
    if (fullyStoredTick)
    {
        EXPECT_EQ(responseMessage.txCount, transactionNum);
    }
    else
    {
        EXPECT_LE(responseMessage.txCount, transactionNum);
    }

    // check tick's transaction digests and money flows
    for (unsigned int transaction = 0; transaction < responseMessage.txCount; ++transaction)
    {
        m256i digest(gen64(), gen64(), gen64(), gen64());
        EXPECT_EQ(responseMessage.txDigests[transaction], digest); // CAUTION: responseMessage.txDigests only available up to responseMessage.txCount

        unsigned char receivedMoneyFlow = (responseMessage.moneyFlew[transaction / 8] >> (transaction % 8)) & 1;
        unsigned char expectedMoneyFlew = gen64() % 2;
        EXPECT_EQ(receivedMoneyFlow, expectedMoneyFlew);
    }
}


TEST(TestCoreTxStatusRequestAddOn, EpochTransition)
{
    unsigned long long seed = 42;

    // use pseudo-random sequence
    std::mt19937_64 gen64(seed);

    // 5x test with running 2 epoch transitions
    for (int testIdx = 0; testIdx < 20; ++testIdx)
    {
        // first, test case of having no transactions, then of having few transaction, later of having many transactions
        unsigned short maxTransactions = 0;
        if (testIdx == 1)
            maxTransactions = NUMBER_OF_TRANSACTIONS_PER_TICK / 4;
        else if (testIdx > 1)
            maxTransactions = NUMBER_OF_TRANSACTIONS_PER_TICK;

        initTxStatusRequestAddOn();

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
        beginEpochTxStatusRequestAddOn(firstEpochTick0);

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
        beginEpochTxStatusRequestAddOn(secondEpochTick0);

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
        beginEpochTxStatusRequestAddOn(thirdEpochTick0);

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

        deinitTxStatusRequestAddOn();
    }
}

