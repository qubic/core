#pragma once

#include "network_messages/transactions.h"

#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"
#include "platform/debugging.h"

#include "spectrum/spectrum.h"

#include "mining/mining.h"

#include "contracts/qpi.h"
#include "contracts/math_lib.h"
#include "contract_core/qpi_collection_impl.h"

#include "public_settings.h"
#include "kangaroo_twelve.h"
#include "vote_counter.h"

// Mempool that saves pending transactions (txs) of all entities.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class PendingTxsPool
{
protected:
    // The PendingTxsPool will always leave space for the two protocol-level txs (tick votes and custom mining).
    static constexpr unsigned int maxNumTxsPerTick = NUMBER_OF_TRANSACTIONS_PER_TICK - 2;
    static constexpr unsigned long long maxNumTxsTotal = PENDING_TXS_POOL_NUM_TICKS * maxNumTxsPerTick;

    // Sizes of different buffers in bytes
    static constexpr unsigned long long tickTransactionsSize =  maxNumTxsTotal * MAX_TRANSACTION_SIZE;
    static constexpr unsigned long long txsDigestsSize = maxNumTxsTotal * sizeof(m256i);

    // `maxNumTxsTotal` priorities have to be saved at a time. Collection capacity has to be 2^N so find the next bigger power of 2.
    static constexpr unsigned long long txsPrioritiesCapacity = math_lib::findNextPowerOf2(maxNumTxsTotal);

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
    inline static unsigned int buffersBeginIndex = 0;

    // Lock for securing the data in the PendingTxsPool
    inline static volatile char lock = 0;

    // Priority queues for transactions in each saved tick
    inline static Collection<unsigned int, txsPrioritiesCapacity>* txsPriorities;

    static void cleanupTxsPriorities(unsigned int tickIndex)
    {
        sint64 elementIndex = txsPriorities->headIndex(m256i{ tickIndex, 0, 0, 0 });
        // use a `for` instead of a `while` loop to make sure it cannot run forever 
        // there can be at most `maxNumTxsPerTick` elements in one pov
        for (unsigned int t = 0; t < maxNumTxsPerTick; ++t)
        {
            if (elementIndex != NULL_INDEX)
                elementIndex = txsPriorities->remove(elementIndex);
            else
                break;
        }
        txsPriorities->cleanupIfNeeded();
    }

    static sint64 calculateTxPriority(const Transaction* tx)
    {
        sint64 priority = 0;
        int sourceIndex = spectrumIndex(tx->sourcePublicKey);
        if (sourceIndex >= 0)
        {
            sint64 balance = energy(sourceIndex);
            if (balance > 0)
            {
                if (isZero(tx->destinationPublicKey) && tx->amount == 0LL
                    && (tx->inputType == VOTE_COUNTER_INPUT_TYPE || tx->inputType == CustomMiningSolutionTransaction::transactionType()))
                {
                    // protocol-level tx always have max priority
                    return INT64_MAX;
                }
                else
                {
                    // calculate tx priority as [balance of src] * [scheduledTick - latestOutgoingTransferTick + 1]
                    EntityRecord entity = spectrum[sourceIndex];
                    priority = smul(balance, static_cast<sint64>(tx->tick - entity.latestOutgoingTransferTick + 1));
                    // decrease by 1 to make sure no normal tx reaches max priority
                    priority--;
                }
            }
        }
        return priority;
    }

    // Return pointer to Transaction based on tickIndex and transactionIndex (checking offset with ASSERT)
    inline static Transaction* getTxPtr(unsigned int tickIndex, unsigned int transactionIndex)
    {
        ASSERT(tickIndex < PENDING_TXS_POOL_NUM_TICKS);
        ASSERT(transactionIndex < maxNumTxsPerTick);
        return (Transaction*)(tickTransactionsBuffer + (tickIndex * maxNumTxsPerTick + transactionIndex) * MAX_TRANSACTION_SIZE);
    }

    // Return pointer to transaction digest based on tickIndex and transactionIndex (checking offset with ASSERT)
    inline static m256i* getDigestPtr(unsigned int tickIndex, unsigned int transactionIndex)
    {
        ASSERT(tickIndex < PENDING_TXS_POOL_NUM_TICKS);
        ASSERT(transactionIndex < maxNumTxsPerTick);
        return &txsDigestsBuffer[tickIndex * maxNumTxsPerTick + transactionIndex];
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
            || !allocPoolWithErrorLog(L"PendingTxsPool::txsDigestsPtr ", txsDigestsSize, (void**)&txsDigestsBuffer, __LINE__)
            || !allocPoolWithErrorLog(L"PendingTxsPool::txsPriorities", sizeof(Collection<unsigned int, txsPrioritiesCapacity>), (void**)&txsPriorities, __LINE__))
        {
            return false;
        }

        ASSERT(lock == 0);

        setMem(tickTransactionsBuffer, tickTransactionsSize, 0);
        setMem(txsDigestsBuffer, txsDigestsSize, 0);
        setMem(numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

        txsPriorities->reset();

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
        if (txsPriorities)
        {
            freePool(txsPriorities);
        }
    }

    // Acquire lock for returned pointers to transactions or digests.
    inline static void acquireLock()
    {
        ACQUIRE(lock);
    }

    // Release lock for returned pointers to transactions or digests.
    inline static void releaseLock()
    {
        RELEASE(lock);
    }

    // Return number of transactions scheduled for the specified tick.
    static unsigned int getNumberOfPendingTickTxs(unsigned int tick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.getNumberOfPendingTickTxs()");
#endif
        unsigned int res = 0;
        ACQUIRE(lock);
        if (tickInStorage(tick))
        {
            res = numSavedTxsPerTick[tickToIndex(tick)];
        }
        RELEASE(lock);

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
        ACQUIRE(lock);
        if (tickInStorage(tick + 1))
        {
            unsigned int startIndex = tickToIndex(tick + 1);

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
        }
        RELEASE(lock);

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
    static bool add(const Transaction* tx)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.add()");
#endif
        bool txAdded = false;
        ACQUIRE(lock);
        if (tx->checkValidity() && tickInStorage(tx->tick))
        {
            unsigned int tickIndex = tickToIndex(tx->tick);
            const unsigned int transactionSize = tx->totalSize();

            // check if tx with same digest already exists
            m256i digest;
            KangarooTwelve(tx, transactionSize, &digest, sizeof(m256i));
            for (unsigned int txIndex = 0; txIndex < numSavedTxsPerTick[tickIndex]; ++txIndex)
            {
                if (*getDigestPtr(tickIndex, txIndex) == digest)
                {
#if !defined(NDEBUG) && !defined(NO_UEFI)
                    CHAR16 dbgMsgBuf[100];
                    setText(dbgMsgBuf, L"tx with the same digest already exists for tick ");
                    appendNumber(dbgMsgBuf, tx->tick, FALSE);
                    addDebugMessage(dbgMsgBuf);
#endif
                    goto end_add_function;
                }
            }

            sint64 priority = calculateTxPriority(tx);
            if (priority > 0)
            {
                m256i povIndex{ tickIndex, 0, 0, 0 };

                if (numSavedTxsPerTick[tickIndex] < maxNumTxsPerTick)
                {
                    copyMem(getDigestPtr(tickIndex, numSavedTxsPerTick[tickIndex]), &digest, sizeof(m256i));
                    copyMem(getTxPtr(tickIndex, numSavedTxsPerTick[tickIndex]), tx, transactionSize);
                    txsPriorities->add(povIndex, numSavedTxsPerTick[tickIndex], priority);

                    numSavedTxsPerTick[tickIndex]++;
                    txAdded = true;
                }
                else
                {
                    // check if priority is higher than lowest priority tx in this tick and replace in this case
                    sint64 lowestElementIndex = txsPriorities->tailIndex(povIndex);
                    if (lowestElementIndex != NULL_INDEX)
                    {
                        if (txsPriorities->priority(lowestElementIndex) < priority)
                        {
                            unsigned int replacedTxIndex = txsPriorities->element(lowestElementIndex);
                            txsPriorities->remove(lowestElementIndex);
                            txsPriorities->add(povIndex, replacedTxIndex, priority);

                            copyMem(getDigestPtr(tickIndex, replacedTxIndex), &digest, sizeof(m256i));
                            copyMem(getTxPtr(tickIndex, replacedTxIndex), tx, transactionSize);

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
                            appendText(dbgMsgBuf, L" and priority ");
                            appendNumber(dbgMsgBuf, priority, FALSE);
                            appendText(dbgMsgBuf, L" is lower than lowest saved priority ");
                            appendNumber(dbgMsgBuf, txsPriorities->priority(lowestElementIndex), FALSE);
                            addDebugMessage(dbgMsgBuf);
                        }
#endif      
                    }
#if !defined(NDEBUG) && !defined(NO_UEFI)
                    else
                    {
                        // debug log, this should never happen
                        CHAR16 dbgMsgBuf[300];
                        setText(dbgMsgBuf, L"maximum number of txs ");
                        appendNumber(dbgMsgBuf, numSavedTxsPerTick[tickIndex], FALSE);
                        appendText(dbgMsgBuf, L" saved for tick ");
                        appendNumber(dbgMsgBuf, tx->tick, FALSE);
                        appendText(dbgMsgBuf, L" but povIndex is unknown. This should never happen.");
                        addDebugMessage(dbgMsgBuf);
                    }
#endif
                }
            }
#if !defined(NDEBUG) && !defined(NO_UEFI)
            else
            {
                CHAR16 dbgMsgBuf[100];
                setText(dbgMsgBuf, L"tx with priority 0 was rejected for tick ");
                appendNumber(dbgMsgBuf, tx->tick, FALSE);
                addDebugMessage(dbgMsgBuf);
            }
#endif
        }

    end_add_function:
        RELEASE(lock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        if (txAdded)
            addDebugMessage(L"End pendingTxsPool.add(), txAdded true");
        else
            addDebugMessage(L"End pendingTxsPool.add(), txAdded false");
#endif
        return txAdded;
    }

    // Get a transaction for the specified tick. If no more transactions for this tick, return nullptr.
    // ATTENTION: when running multiple threads, you need to have acquired the lock via acquireLock() before calling this function.
    static Transaction* getTx(unsigned int tick, unsigned int index)
    {
        unsigned int tickIndex;

        if (tickInStorage(tick))
            tickIndex = tickToIndex(tick);
        else
            return nullptr;

        bool hasTx = index < numSavedTxsPerTick[tickIndex];

        if (hasTx)
            return getTxPtr(tickIndex, index);
        else
            return nullptr;
    }

    // Get a transaction digest for the specified tick. If no more transactions for this tick, return nullptr.
    // ATTENTION: when running multiple threads, you need to have acquired the lock via acquireLock() before calling this function.
    static m256i* getDigest(unsigned int tick, unsigned int index)
    {
        unsigned int tickIndex;

        if (tickInStorage(tick))
            tickIndex = tickToIndex(tick);
        else
            return nullptr;

        bool hasTx = index < numSavedTxsPerTick[tickIndex];

        if (hasTx)
            return getDigestPtr(tickIndex, index);
        else
            return nullptr;
    }

    static void incrementFirstStoredTick()
    {
        ACQUIRE(lock);

        // set memory at buffersBeginIndex to 0 
        unsigned long long numTxsBeforeBegin = buffersBeginIndex * maxNumTxsPerTick;
        setMem(tickTransactionsBuffer + numTxsBeforeBegin * MAX_TRANSACTION_SIZE, maxNumTxsPerTick * MAX_TRANSACTION_SIZE, 0);
        setMem(txsDigestsBuffer + numTxsBeforeBegin, maxNumTxsPerTick * sizeof(m256i), 0);
        numSavedTxsPerTick[buffersBeginIndex] = 0;

        // remove txs priorities stored for firstStoredTick
        cleanupTxsPriorities(tickToIndex(firstStoredTick));

        // increment buffersBeginIndex and firstStoredTick
        firstStoredTick++;
        buffersBeginIndex = (buffersBeginIndex + 1) % PENDING_TXS_POOL_NUM_TICKS;

        RELEASE(lock);
    }

    static void beginEpoch(unsigned int newInitialTick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin pendingTxsPool.beginEpoch()");
#endif
        ACQUIRE(lock);
        if (tickInStorage(newInitialTick))
        {
            unsigned int newInitialIndex = tickToIndex(newInitialTick);

            // reset memory of discarded ticks
            if (newInitialIndex < buffersBeginIndex)
            {
                unsigned long long numTxsBeforeNew = newInitialIndex * maxNumTxsPerTick;
                setMem(tickTransactionsBuffer, numTxsBeforeNew * MAX_TRANSACTION_SIZE, 0);
                setMem(txsDigestsBuffer, numTxsBeforeNew * sizeof(m256i), 0);
                setMem(numSavedTxsPerTick, newInitialIndex * sizeof(unsigned int), 0);

                for (unsigned int tickIndex = 0; tickIndex < newInitialIndex; ++tickIndex)
                    cleanupTxsPriorities(tickIndex);

                unsigned long long numTxsBeforeBegin = buffersBeginIndex * maxNumTxsPerTick;
                unsigned long long numTxsStartingAtBegin = (PENDING_TXS_POOL_NUM_TICKS - buffersBeginIndex) * maxNumTxsPerTick;
                setMem(tickTransactionsBuffer + numTxsBeforeBegin * MAX_TRANSACTION_SIZE, numTxsStartingAtBegin * MAX_TRANSACTION_SIZE, 0);
                setMem(txsDigestsBuffer + numTxsBeforeBegin, numTxsStartingAtBegin * sizeof(m256i), 0);
                setMem(numSavedTxsPerTick + buffersBeginIndex, (PENDING_TXS_POOL_NUM_TICKS - buffersBeginIndex) * sizeof(unsigned int), 0);

                for (unsigned int tickIndex = buffersBeginIndex; tickIndex < PENDING_TXS_POOL_NUM_TICKS; ++tickIndex)
                    cleanupTxsPriorities(tickIndex);
            }
            else
            {
                unsigned long long numTxsBeforeBegin = buffersBeginIndex * maxNumTxsPerTick;
                unsigned long long numTxsStartingAtBegin = (newInitialIndex - buffersBeginIndex) * maxNumTxsPerTick;
                setMem(tickTransactionsBuffer + numTxsBeforeBegin * MAX_TRANSACTION_SIZE, numTxsStartingAtBegin * MAX_TRANSACTION_SIZE, 0);
                setMem(txsDigestsBuffer + numTxsBeforeBegin, numTxsStartingAtBegin * sizeof(m256i), 0);
                setMem(numSavedTxsPerTick + buffersBeginIndex, (newInitialIndex - buffersBeginIndex) * sizeof(unsigned int), 0);

                for (unsigned int tickIndex = buffersBeginIndex; tickIndex < newInitialIndex; ++tickIndex)
                    cleanupTxsPriorities(tickIndex);
            }

            buffersBeginIndex = newInitialIndex;
        }
        else
        {
            setMem(tickTransactionsBuffer, tickTransactionsSize, 0);
            setMem(txsDigestsBuffer, txsDigestsSize, 0);
            setMem(numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

            txsPriorities->reset();

            buffersBeginIndex = 0;
        }

        firstStoredTick = newInitialTick;

        RELEASE(lock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End pendingTxsPool.beginEpoch()");
#endif
    }

    // Useful for debugging, but expensive: check that everything is as expected.
    static void checkStateConsistencyWithAssert()
    {
        ACQUIRE(lock);

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
                ASSERT(numSavedForTick <= maxNumTxsPerTick);
                for (unsigned int txIndex = 0; txIndex < numSavedForTick; ++txIndex)
                {
                    Transaction* transaction = (Transaction*)(tickTransactionsBuffer + (tickIndex * maxNumTxsPerTick + txIndex) * MAX_TRANSACTION_SIZE);
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

        RELEASE(lock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End pendingTxsPool.checkStateConsistencyWithAssert()");
#endif
    }

};