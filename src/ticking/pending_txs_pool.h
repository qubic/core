#pragma once

#include "network_messages/transactions.h"

#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"

#include "public_settings.h"
#include "kangaroo_twelve.h"

// Mempool that saves pending transactions (txs) of all entities.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class PendingTxsPool
{
private:
    static constexpr unsigned long long maxNumTxs = PENDING_TXS_POOL_NUM_TICKS * NUMBER_OF_TRANSACTIONS_PER_TICK;

    // Sizes of different buffers in bytes
    static constexpr unsigned long long tickTransactionsSize =  maxNumTxs * MAX_TRANSACTION_SIZE;
    static constexpr unsigned long long tickTransactionOffsetsSize = maxNumTxs * sizeof(unsigned long long);
    static constexpr unsigned long long txsDigestsSize = maxNumTxs * sizeof(m256i);

    // The pool stores the tick range [firstStoredTick, firstStoredTick + PENDING_TXS_POOL_NUM_TICKS[
    inline static unsigned int firstStoredTick = 0;

    // Allocated tickTransactions buffer with tickTransactionsSize bytes
    inline static unsigned char* tickTransactionsBuffer = nullptr;

    // Allocated txsDigests buffer with maxNumTxs elements
    inline static m256i* txsDigestsBuffer = nullptr;

    // Records the number of saved transactions for each tick
    inline static unsigned int numSavedTxsPerTick[PENDING_TXS_POOL_NUM_TICKS];

    // Begin index for tickTransactionOffsetsBuffer, txsDigestsBuffer, and numSavedTxsPerTick
    // buffersBeginIndex corresponds to firstStoredTick
    inline static unsigned long long buffersBeginIndex = 0;

    // Lock for securing tickTransactions and tickTransactionOffsets
    inline static volatile char tickTransactionsLock = 0;

    // Lock for securing txsDigests
    inline static volatile char txsDigestsLock = 0;
    
    // Lock for securing numSavedTxsPerTick
    inline static volatile char numSavedLock = 0;

    // Return pointer to Transaction based on tickIndex and transactionIndex (checking offset with ASSERT)
    inline static Transaction* getTxPtr(unsigned int tickIndex, unsigned int transactionIndex)
    {
        ASSERT(tickIndex < PENDING_TXS_POOL_NUM_TICKS);
        ASSERT(transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK);
        return (Transaction*)(tickTransactionsBuffer + (tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + transactionIndex) * MAX_TRANSACTION_SIZE);
    }

    // Return pointer to transaction digest based on tickIndex and transactionIndex (checking offset with ASSERT)
    inline static m256i* getDigestPtr(unsigned int tickIndex, unsigned int transactionIndex)
    {
        ASSERT(tickIndex < PENDING_TXS_POOL_NUM_TICKS);
        ASSERT(transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK);
        return &txsDigestsBuffer[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + transactionIndex];
    }

    // Check whether tick is stored in the pending txs pool
    inline static bool tickInStorage(unsigned int tick)
    {
        return tick >= firstStoredTick && tick < firstStoredTick + PENDING_TXS_POOL_NUM_TICKS;
    }

    // Return index of tick data in current storage window (does not check tick).
    inline static unsigned int tickToIndex(unsigned int tick)
    {
        return ((tick - firstStoredTick) + buffersBeginIndex) % PENDING_TXS_POOL_NUM_TICKS;
    }

public:

    // Init at node startup.
    static bool init()
    {
        if (!allocPoolWithErrorLog(L"PendingTxsPool::tickTransactionsPtr ", tickTransactionsSize, (void**)&tickTransactionsBuffer, __LINE__)
            || !allocPoolWithErrorLog(L"PendingTxsPool::txsDigestsPtr ", txsDigestsSize, (void**)&txsDigestsBuffer, __LINE__))
        {
            return false;
        }

        ASSERT(tickTransactionsLock == 0);
        ASSERT(txsDigestsLock == 0);
        ASSERT(numSavedLock == 0);

        setMem(tickTransactionsBuffer, tickTransactionsSize, 0);
        setMem(txsDigestsBuffer, txsDigestsSize, 0);
        setMem(numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

        firstStoredTick = 0;
        buffersBeginIndex = 0;

        return true;
    }

    // Cleanup at node shutdown.
    static void deinit()
    {
        if (tickTransactionsBuffer)
        {
            freePool(tickTransactionsBuffer);
        }
        if (txsDigestsBuffer)
        {
            freePool(txsDigestsBuffer);
        }
    }

    // Acquire lock for returned pointers to transactions, transaction offsets, or digests.
    inline static void acquireLock()
    {
        ACQUIRE(txsDigestsLock);
        ACQUIRE(tickTransactionsLock);
    }

    // Release lock for returned pointers to transactions, transaction offsets, or digests.
    inline static void releaseLock()
    {
        RELEASE(tickTransactionsLock);
        RELEASE(txsDigestsLock);
    }

    // Return number of transactions scheduled for the specified tick.
    static unsigned int getNumberOfPendingTickTxs(unsigned int tick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.getNumberOfPendingTickTxs()");
#endif
        unsigned int res = 0;
        ACQUIRE(numSavedLock);
        if (tickInStorage(tick))
        {
            res = numSavedTxsPerTick[tickToIndex(tick)];
        }
        RELEASE(numSavedLock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"End pendingTxsPool.getNumberOfPendingTickTxs() for tick=");
        appendNumber(dbgMsgBuf, tick, FALSE);
        appendText(dbgMsgBuf, L" -> res=");
        appendNumber(dbgMsgBuf, res, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        return res;
    }

    // Return number of transactions scheduled later than the specified tick.
    static unsigned int getTotalNumberOfPendingTxs(unsigned int tick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.getTotalNumberOfPendingTxs()");
#endif
        unsigned int res = 0;

        if (tickInStorage(tick + 1))
        {
            unsigned int startIndex = tickToIndex(tick + 1);

            ACQUIRE(numSavedLock);
            if (startIndex < buffersBeginIndex)
            {
                for (unsigned int t = startIndex; t < buffersBeginIndex; ++t)
                    res += numSavedTxsPerTick[t];
            }
            else
            {
                for (unsigned int t = startIndex; t < PENDING_TXS_POOL_NUM_TICKS; ++t)
                    res += numSavedTxsPerTick[t];
                for (unsigned int t = 0; t < buffersBeginIndex; ++t)
                    res += numSavedTxsPerTick[t];
            }
            RELEASE(numSavedLock);
        }

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"End pendingTxsPool.getTotalNumberOfPendingTxs() for tick=");
        appendNumber(dbgMsgBuf, tick, FALSE);
        appendText(dbgMsgBuf, L" -> res=");
        appendNumber(dbgMsgBuf, res, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        return res;
    }

    // Check validity of transaction and add to the pool. Return boolean indicating whether transaction was added.
    static bool add(Transaction* tx)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.update()");
#endif
        bool txAdded = false;
        if (tx->checkValidity() && tickInStorage(tx->tick))
        {
            unsigned int tickIndex = tickToIndex(tx->tick);
            const unsigned int transactionSize = tx->totalSize();

            acquireLock();
            ACQUIRE(numSavedLock);

            if (numSavedTxsPerTick[tickIndex] < NUMBER_OF_TRANSACTIONS_PER_TICK)
            {
                KangarooTwelve(tx, transactionSize, getDigestPtr(tickIndex, numSavedTxsPerTick[tickIndex]), sizeof(m256i));

                copyMem(getTxPtr(tickIndex, numSavedTxsPerTick[tickIndex]), tx, transactionSize);

                numSavedTxsPerTick[tickIndex]++;
                txAdded = true;
            }
#if !defined(NDEBUG) && !defined(NO_UEFI)
            else
            {
                CHAR16 dbgMsgBuf[300];
                setText(dbgMsgBuf, L"tx could not be added, already saved ");
                appendNumber(dbgMsgBuf, numSavedTxsPerTick[tickIndex], FALSE);
                appendText(dbgMsgBuf, L" txs for tick ");
                appendNumber(dbgMsgBuf, tx->tick, FALSE);
                addDebugMessage(dbgMsgBuf);
            }
#endif

            RELEASE(numSavedLock);
            releaseLock();
        }
#if !defined(NDEBUG) && !defined(NO_UEFI)
        if (txAdded)
            addDebugMessage(L"End pendingTxsPool.update(), txAdded true");
        else
            addDebugMessage(L"End pendingTxsPool.update(), txAdded false");
#endif
        return txAdded;
    }

    // Get a transaction for the specified tick.
    // If no more transactions for this tick, return nullptr.
    static Transaction* get(unsigned int tick, unsigned int index)
    {
        unsigned int tickIndex;
        if (tickInStorage(tick))
            tickIndex = tickToIndex(tick);
        else
            return nullptr;

        ACQUIRE(numSavedLock);
        bool hasTx = index < numSavedTxsPerTick[tickIndex];
        RELEASE(numSavedLock);

        if (hasTx)
            return getTxPtr(tickIndex, index);
        else
            return nullptr;
    }

    // Get a transaction digest for the specified tick.
    // If no more transactions for this tick, return nullptr.
    static m256i* getDigest(unsigned int tick, unsigned int index)
    {
        unsigned int tickIndex;
        if (tickInStorage(tick))
            tickIndex = tickToIndex(tick);
        else
            return nullptr;

        ACQUIRE(numSavedLock);
        bool hasTx = index < numSavedTxsPerTick[tickIndex];
        RELEASE(numSavedLock);

        if (hasTx)
            return getDigestPtr(tickIndex, index);
        else
            return nullptr;
    }

    static void incrementFirstStoredTick()
    {
        // set memory at buffersBeginIndex to 0 
        unsigned long long numTxsBeforeBegin = buffersBeginIndex * NUMBER_OF_TRANSACTIONS_PER_TICK;
        setMem(tickTransactionsBuffer + numTxsBeforeBegin * MAX_TRANSACTION_SIZE, NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE, 0);
        setMem(txsDigestsBuffer + numTxsBeforeBegin, NUMBER_OF_TRANSACTIONS_PER_TICK * sizeof(m256i), 0);
        numSavedTxsPerTick[buffersBeginIndex] = 0;

        // increment buffersBeginIndex and firstStoredTick
        firstStoredTick++;
        buffersBeginIndex = (buffersBeginIndex + 1) % PENDING_TXS_POOL_NUM_TICKS;
    }

    static void beginEpoch(unsigned int newInitialTick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.beginEpoch()");
#endif

        if (tickInStorage(newInitialTick))
        {
            unsigned int newInitialIndex = tickToIndex(newInitialTick);

            // reset memory of discarded ticks
            if (newInitialIndex < buffersBeginIndex)
            {
                unsigned long long numTxsBeforeNew = newInitialIndex * NUMBER_OF_TRANSACTIONS_PER_TICK;
                setMem(tickTransactionsBuffer, numTxsBeforeNew * MAX_TRANSACTION_SIZE, 0);
                setMem(txsDigestsBuffer, numTxsBeforeNew * sizeof(m256i), 0);
                setMem(numSavedTxsPerTick, newInitialIndex * sizeof(unsigned int), 0);

                unsigned long long numTxsBeforeBegin = buffersBeginIndex * NUMBER_OF_TRANSACTIONS_PER_TICK;
                unsigned long long numTxsStartingAtBegin = (PENDING_TXS_POOL_NUM_TICKS - buffersBeginIndex) * NUMBER_OF_TRANSACTIONS_PER_TICK;
                setMem(tickTransactionsBuffer + numTxsBeforeBegin * MAX_TRANSACTION_SIZE, numTxsStartingAtBegin * MAX_TRANSACTION_SIZE, 0);
                setMem(txsDigestsBuffer + numTxsBeforeBegin, numTxsStartingAtBegin * sizeof(m256i), 0);
                setMem(numSavedTxsPerTick + buffersBeginIndex, (PENDING_TXS_POOL_NUM_TICKS - buffersBeginIndex) * sizeof(unsigned int), 0);
            }
            else
            {
                unsigned long long numTxsBeforeBegin = buffersBeginIndex * NUMBER_OF_TRANSACTIONS_PER_TICK;
                unsigned long long numTxsStartingAtBegin = (newInitialIndex - buffersBeginIndex) * NUMBER_OF_TRANSACTIONS_PER_TICK;
                setMem(tickTransactionsBuffer + numTxsBeforeBegin * MAX_TRANSACTION_SIZE, numTxsStartingAtBegin * MAX_TRANSACTION_SIZE, 0);
                setMem(txsDigestsBuffer + numTxsBeforeBegin, numTxsStartingAtBegin * sizeof(m256i), 0);
                setMem(numSavedTxsPerTick + buffersBeginIndex, (newInitialIndex - buffersBeginIndex) * sizeof(unsigned int), 0);
            }

            buffersBeginIndex = newInitialIndex;
        }
        else
        {
            setMem(tickTransactionsBuffer, tickTransactionsSize, 0);
            setMem(txsDigestsBuffer, txsDigestsSize, 0);
            setMem(numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

            buffersBeginIndex = 0;
        }

        firstStoredTick = newInitialTick;

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End pendingTxsPool.beginEpoch()");
#endif
    }

    // Useful for debugging, but expensive: check that everything is as expected.
    static void checkStateConsistencyWithAssert()
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin tsxPool.checkStateConsistencyWithAssert()");
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"firstStoredTick=");
        appendNumber(dbgMsgBuf, firstStoredTick, FALSE);
        appendText(dbgMsgBuf, L", buffersBeginIndex=");
        appendNumber(dbgMsgBuf, buffersBeginIndex, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        ASSERT(buffersBeginIndex >= 0);
        ASSERT(buffersBeginIndex < PENDING_TXS_POOL_NUM_TICKS);

        ASSERT(tickTransactionsBuffer != nullptr);
        ASSERT(txsDigestsBuffer != nullptr);

        for (unsigned int tick = firstStoredTick; tick < firstStoredTick + PENDING_TXS_POOL_NUM_TICKS; ++tick)
        {
            ASSERT(tickInStorage(tick));
            if (tickInStorage(tick))
            {
                unsigned int tickIndex = tickToIndex(tick);
                unsigned int numSavedForTick = numSavedTxsPerTick[tickIndex];
                ASSERT(numSavedForTick <= NUMBER_OF_TRANSACTIONS_PER_TICK);
                for (unsigned int txIndex = 0; txIndex < numSavedForTick; ++txIndex)
                {
                    Transaction* transaction = (Transaction*)(tickTransactionsBuffer + (tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + txIndex) * MAX_TRANSACTION_SIZE);
                    ASSERT(transaction->checkValidity());
                    ASSERT(transaction->tick == tick);
#if !defined(NDEBUG) && !defined(NO_UEFI)
                    if (!transaction->checkValidity() || transaction->tick != tick)
                    {
                        setText(dbgMsgBuf, L"Error in previous epoch transaction ");
                        appendNumber(dbgMsgBuf, txIndex, FALSE);
                        appendText(dbgMsgBuf, L" in tick ");
                        appendNumber(dbgMsgBuf, tick, FALSE);
                        addDebugMessage(dbgMsgBuf);

                        setText(dbgMsgBuf, L"t->tick ");
                        appendNumber(dbgMsgBuf, transaction->tick, FALSE);
                        appendText(dbgMsgBuf, L", t->inputSize ");
                        appendNumber(dbgMsgBuf, transaction->inputSize, FALSE);
                        appendText(dbgMsgBuf, L", t->inputType ");
                        appendNumber(dbgMsgBuf, transaction->inputType, FALSE);
                        appendText(dbgMsgBuf, L", t->amount ");
                        appendNumber(dbgMsgBuf, transaction->amount, TRUE);
                        addDebugMessage(dbgMsgBuf);
                    }
#endif
                }
            }
        }

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End pendingTxsPool.checkStateConsistencyWithAssert()");
#endif
    }

};