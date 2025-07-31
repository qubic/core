#pragma once

#include "kangaroo_twelve.h"
#include "network_messages/transactions.h"
#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"
#include "public_settings.h"

// Mempool that saves pending transactions (txs) of all entities.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class TxsPool
{
private:
    static constexpr unsigned long long maxNumTicksToSave = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
    static constexpr unsigned long long maxNumTxsCurrentEpoch = ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK;
    static constexpr unsigned long long maxNumTxsPreviousEpoch = ((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK;
    static constexpr unsigned long long maxNumTxs = maxNumTxsCurrentEpoch + maxNumTxsPreviousEpoch;

    static constexpr unsigned long long tickTransactionsSizeCurrentEpoch = FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS);
    static constexpr unsigned long long tickTransactionsSizePreviousEpoch = (((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS);
    static constexpr unsigned long long tickTransactionsSize = tickTransactionsSizeCurrentEpoch + tickTransactionsSizePreviousEpoch;

    static constexpr unsigned long long tickTransactionOffsetsSizeCurrentEpoch = maxNumTxsCurrentEpoch * sizeof(unsigned long long);
    static constexpr unsigned long long tickTransactionOffsetsSizePreviousEpoch = maxNumTxsPreviousEpoch * sizeof(unsigned long long);
    static constexpr unsigned long long tickTransactionOffsetsSize = maxNumTxs * sizeof(unsigned long long);

    static constexpr unsigned long long txsDigestsSizeCurrentEpoch = maxNumTxsCurrentEpoch * 32ULL;
    static constexpr unsigned long long txsDigestsSizePreviousEpoch = maxNumTxsPreviousEpoch * 32ULL;
    static constexpr unsigned long long txsDigestsSize = txsDigestsSizeCurrentEpoch + txsDigestsSizePreviousEpoch;

    // Tick number range of current epoch storage
    inline static unsigned int tickBegin = 0;
    inline static unsigned int tickEnd = 0;

    // Tick number range of previous epoch storage
    inline static unsigned int oldTickBegin = 0;
    inline static unsigned int oldTickEnd = 0;

    // Allocated tickTransactions buffer with tickTransactionsSize bytes (includes current and previous epoch data)
    inline static unsigned char* tickTransactionsPtr = nullptr;

    // Allocated tickTransactionOffsets buffer with tickTransactionOffsetsLength elements (includes current and previous epoch data)
    inline static unsigned long long* tickTransactionOffsetsPtr = nullptr;

    // Allocated txsDigests buffer with txsDigestsLength elements (includes current and previous epoch data)
    inline static m256i* txsDigestsPtr = nullptr;

    // Tick transaction buffer of previous epoch. Points to tickTransactionsPtr + tickTransactionsSizeCurrentEpoch.
    inline static unsigned char* oldTickTransactionsPtr = nullptr;

    // Tick transaction offsets of previous epoch. Points to tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch.
    inline static unsigned long long* oldTickTransactionOffsetsPtr = nullptr;

    // Transaction digests buffer of previous epoch. Points to txsDigestsPtr + txsDigestsLengthCurrentEpoch
    inline static m256i* oldTxsDigestsPtr = nullptr;

    // Records the number of saved transactions for each tick (includes current and previous epoch data)
    inline static unsigned int numSavedTxsPerTick[maxNumTicksToSave];

    // Offset of next free space in tick transaction storage
    inline static unsigned long long nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

    // Lock for securing tickTransactions and tickTransactionOffsets
    inline static volatile char tickTransactionsLock = 0;

    // Lock for securing txsDigests
    inline static volatile char txsDigestsLock = 0;
    
    // Lock for securing numSavedTxsPerTick
    inline static volatile char numSavedLock = 0;

public:
    // Struct for structured, convenient access via ".tickTransactionOffsets"
    static struct TickTransactionOffsetsAccess
    {
        // Return pointer to offset array of transactions by tick index independent of epoch (checking index with ASSERT)
        inline static unsigned long long* getByTickIndex(unsigned int tickIndex)
        {
            ASSERT(tickIndex < maxNumTicksToSave);
            return tickTransactionOffsetsPtr + (tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK);
        }

        // Return pointer to offset array of transactions of tick in current epoch by tick (checking tick with ASSERT)
        inline static unsigned long long* getByTickInCurrentEpoch(unsigned int tick)
        {
            ASSERT(tickInCurrentEpochStorage(tick));
            const unsigned int tickIndex = tickToIndexCurrentEpoch(tick);
            return getByTickIndex(tickIndex);
        }

        // Return pointer to offset array of transactions of tick in previous epoch by tick (checking tick with ASSERT)
        inline static unsigned long long* getByTickInPreviousEpoch(unsigned int tick)
        {
            ASSERT(tickInPreviousEpochStorage(tick));
            const unsigned int tickIndex = tickToIndexPreviousEpoch(tick);
            return getByTickIndex(tickIndex);
        }

        // Return reference to offset by tick and transaction in current epoch (checking inputs with ASSERT)
        inline static unsigned long long& get(unsigned int tick, unsigned int transaction)
        {
            ASSERT(transaction < NUMBER_OF_TRANSACTIONS_PER_TICK);
            return getByTickInCurrentEpoch(tick)[transaction];
        }
    } tickTransactionOffsets;

    // Struct for structured, convenient access via ".tickTransactions"
    static struct TickTransactionsAccess
    {
        void acquireLock()
        {
            ACQUIRE(tickTransactionsLock);
        }

        void releaseLock()
        {
            RELEASE(tickTransactionsLock);
        }

        // Return pointer to Transaction based on transaction offset independent of epoch (checking offset with ASSERT)
        inline static Transaction* ptr(unsigned long long transactionOffset)
        {
            ASSERT(transactionOffset < tickTransactionsSize);
            return (Transaction*)(tickTransactionsPtr + transactionOffset);
        }
    } tickTransactions;


    // Init at node startup.
    static bool init()
    {
        if (!allocatePool(tickTransactionsSize, (void**)&tickTransactionsPtr)
            || !allocatePool(tickTransactionOffsetsSize, (void**)&tickTransactionOffsetsPtr)
            || !allocatePool(txsDigestsSize, (void**)&txsDigestsPtr))
        {
            logToConsole(L"Failed to allocate transaction pool memory!");
            return false;
        }

        nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

        oldTickTransactionsPtr = tickTransactionsPtr + tickTransactionsSizeCurrentEpoch;
        oldTickTransactionOffsetsPtr = tickTransactionOffsetsPtr + maxNumTxsCurrentEpoch;
        oldTxsDigestsPtr = txsDigestsPtr + maxNumTxsCurrentEpoch;

        ASSERT(tickTransactionsLock == 0);
        ASSERT(txsDigestsLock == 0);
        ASSERT(numSavedLock == 0);
        setMem((void*)numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

        tickBegin = 0;
        tickEnd = 0;
        oldTickBegin = 0;
        oldTickEnd = 0;

        return true;
    }

    // Cleanup at node shutdown.
    static void deinit()
    {
        if (tickTransactionOffsetsPtr)
        {
            freePool(tickTransactionOffsetsPtr);
        }
        if (tickTransactionsPtr)
        {
            freePool(tickTransactionsPtr);
        }
        if (txsDigestsPtr)
        {
            freePool(txsDigestsPtr);
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

    // Check whether tick is stored in the current epoch storage.
    inline static bool tickInCurrentEpochStorage(unsigned int tick)
    {
        return tick >= tickBegin && tick < tickEnd;
    }

    // Check whether tick is stored in the previous epoch storage.
    inline static bool tickInPreviousEpochStorage(unsigned int tick)
    {
        return oldTickBegin <= tick && tick < oldTickEnd;
    }

    // Return index of tick data in current epoch (does not check tick).
    unsigned static int tickToIndexCurrentEpoch(unsigned int tick)
    {
        return tick - tickBegin;
    }

    // Return index of tick data in previous epoch (does not check that it is stored).
    unsigned static int tickToIndexPreviousEpoch(unsigned int tick)
    {
        return tick - oldTickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }

    // Return number of transactions scheduled for the specified tick.
    static unsigned int getNumberOfTickTxs(unsigned int tick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin txsPool.getNumberOfTickTxs()");
#endif
        unsigned int res = 0;
        ACQUIRE(numSavedLock);
        if (tickInPreviousEpochStorage(tick))
        {
            res = numSavedTxsPerTick[tickToIndexPreviousEpoch(tick)];
        }
        else if (tickInCurrentEpochStorage(tick))
        {
            res = numSavedTxsPerTick[tickToIndexCurrentEpoch(tick)];
        }
        RELEASE(numSavedLock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"End txsPool.getNumberOfTickTxs(), res=");
        appendNumber(dbgMsgBuf, res, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        return res;
    }

    // Return number of transactions scheduled later than the specified tick.
    static unsigned int getNumberOfPendingTxs(unsigned int tick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin txsPool.getNumberOfPendingTxs()");
#endif
        unsigned int res = 0;
        unsigned int startTick = tickEnd;
        unsigned int oldStartTick = oldTickEnd;

        if (tick < oldTickBegin || oldTickBegin == 0 && tick < tickBegin)
        {
            startTick = tickBegin;
            oldStartTick = oldTickBegin;
        }
        else if (tickInPreviousEpochStorage(tick))
        {
            startTick = tickBegin;
            oldStartTick = tick + 1;
        }
        else if (tickInCurrentEpochStorage(tick))
        {
            startTick = tick + 1;
        }

        ACQUIRE(numSavedLock);
        for (unsigned int t = startTick; t < tickEnd; ++t)
        {
            res += numSavedTxsPerTick[tickToIndexCurrentEpoch(t)];
        }
        for (unsigned int t = oldStartTick; t < oldTickEnd; ++t)
        {
            res += numSavedTxsPerTick[tickToIndexPreviousEpoch(t)];
        }
        RELEASE(numSavedLock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"End txsPool.getNumberOfPendingTxs(), res=");
        appendNumber(dbgMsgBuf, res, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        return res;
    }

    // Check validity of transaction and add to the pool. Return boolean indicating whether transaction was added.
    static bool update(Transaction* tx)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin txsPool.update()");
#endif
        bool txAdded = false;
        if (tx->checkValidity() && tickInCurrentEpochStorage(tx->tick))
        {
            unsigned int tickIndex = tickToIndexCurrentEpoch(tx->tick);
            const unsigned int transactionSize = tx->totalSize();

            ACQUIRE(numSavedLock);
            acquireLock();

            if (numSavedTxsPerTick[tickIndex] < NUMBER_OF_TRANSACTIONS_PER_TICK
                && nextTickTransactionOffset + transactionSize <= tickTransactionsSizeCurrentEpoch)
            {
                unsigned long long& transactionOffset = tickTransactionOffsets.getByTickIndex(tickIndex)[numSavedTxsPerTick[tickIndex]];
                ASSERT(transactionOffset == 0);

                KangarooTwelve(tx, transactionSize, &txsDigestsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + numSavedTxsPerTick[tickIndex]], 32ULL);
                transactionOffset = nextTickTransactionOffset;
                copyMem(tickTransactions.ptr(transactionOffset), tx, transactionSize);
                nextTickTransactionOffset += transactionSize;

                numSavedTxsPerTick[tickIndex]++;
                txAdded = true;
            }

            releaseLock();
            RELEASE(numSavedLock);
        }
#if !defined(NDEBUG) && !defined(NO_UEFI)
        if (txAdded)
            addDebugMessage(L"End txsPool.update(), txAdded true");
        else
            addDebugMessage(L"End txsPool.update(), txAdded false");
#endif
        return txAdded;
    }

    // Get a transaction for the specified tick.
    // If no more transactions for this tick, return nullptr.
    static Transaction* get(unsigned int tick, unsigned int index)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"txsPool.get()");
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"tick=");
        appendNumber(dbgMsgBuf, tick, FALSE);
        appendText(dbgMsgBuf, L", index=");
        appendNumber(dbgMsgBuf, index, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        unsigned int tickIndex;
        if (tickInCurrentEpochStorage(tick))
        {
            tickIndex = tickToIndexCurrentEpoch(tick);
        }
        else if (tickInPreviousEpochStorage(tick))
        {
            tickIndex = tickToIndexPreviousEpoch(tick);
        }
        else
        {
            return nullptr;
        }

        ACQUIRE(numSavedLock);
        bool hasTx = index < numSavedTxsPerTick[tickIndex];
        RELEASE(numSavedLock);

        if (hasTx)
        {
            ASSERT(index < NUMBER_OF_TRANSACTIONS_PER_TICK);
            unsigned long long offset = tickTransactionOffsets.getByTickIndex(tickIndex)[index];
            ASSERT(offset != 0);
            return tickTransactions.ptr(offset);
        }
        else
        {
            return nullptr;
        }
    }

    // Get a transaction digest for the specified tick.
    // If no more transactions for this tick, return nullptr.
    static m256i* getDigest(unsigned int tick, unsigned int index)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"txsPool.getDigest()");
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"tick=");
        appendNumber(dbgMsgBuf, tick, FALSE);
        appendText(dbgMsgBuf, L", index=");
        appendNumber(dbgMsgBuf, index, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        unsigned int tickIndex;
        if (tickInCurrentEpochStorage(tick))
        {
            tickIndex = tickToIndexCurrentEpoch(tick);
        }
        else if (tickInPreviousEpochStorage(tick))
        {
            tickIndex = tickToIndexPreviousEpoch(tick);
        }
        else
        {
            return nullptr;
        }

        ACQUIRE(numSavedLock);
        bool hasTx = index < numSavedTxsPerTick[tickIndex];
        RELEASE(numSavedLock);

        if (hasTx)
        {
            ASSERT(index < NUMBER_OF_TRANSACTIONS_PER_TICK);
            return &txsDigestsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + index];
        }
        else
        {
            return nullptr;
        }
    }


    // Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
    // are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
    void beginEpoch(unsigned int newInitialTick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin txsPool.beginEpoch()");
#endif
        if (tickBegin && tickInCurrentEpochStorage(newInitialTick) && tickBegin < newInitialTick)
        {
            // seamless epoch transition: keep some ticks of prior epoch
            oldTickEnd = newInitialTick;
            oldTickBegin = newInitialTick - TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
            if (oldTickBegin < tickBegin)
                oldTickBegin = tickBegin;

            const unsigned int tickIndex = tickToIndexCurrentEpoch(oldTickBegin);
            const unsigned int tickCount = oldTickEnd - oldTickBegin;

            // copy transactions and transactionOffsets
            {
                // copy transactions
                const unsigned long long totalTransactionSizesSum = nextTickTransactionOffset - FIRST_TICK_TRANSACTION_OFFSET;
                const unsigned long long keepTransactionSizesSum = (totalTransactionSizesSum <= tickTransactionsSizePreviousEpoch) ? totalTransactionSizesSum : tickTransactionsSizePreviousEpoch;
                const unsigned long long firstToKeepOffset = nextTickTransactionOffset - keepTransactionSizesSum;
                copyMem(oldTickTransactionsPtr, tickTransactionsPtr + firstToKeepOffset, keepTransactionSizesSum);

                // adjust offsets (based on end of transactions)
                const unsigned long long offsetDelta = tickTransactionsSizeCurrentEpoch - firstToKeepOffset;
                for (unsigned int tickId = oldTickBegin; tickId < oldTickEnd; ++tickId)
                {
                    const unsigned long long* tickOffsets = tickTransactionOffsets.getByTickInCurrentEpoch(tickId);
                    unsigned long long* tickOffsetsPrevEp = tickTransactionOffsets.getByTickInPreviousEpoch(tickId);
                    for (unsigned int transactionIdx = 0; transactionIdx < NUMBER_OF_TRANSACTIONS_PER_TICK; ++transactionIdx)
                    {
                        const unsigned long long offset = tickOffsets[transactionIdx];
                        if (!offset || offset < firstToKeepOffset)
                        {
                            // transaction not available (either not available overall or not fitting in storage of previous epoch)
                            tickOffsetsPrevEp[transactionIdx] = 0;
                        }
                        else
                        {
                            // set offset of transcation
                            const unsigned long long offsetPrevEp = offset + offsetDelta;
                            tickOffsetsPrevEp[transactionIdx] = offsetPrevEp;

                            // check offset and transaction
                            ASSERT(offset >= FIRST_TICK_TRANSACTION_OFFSET);
                            ASSERT(offset < tickTransactionsSizeCurrentEpoch);
                            ASSERT(offsetPrevEp >= tickTransactionsSizeCurrentEpoch);
                            ASSERT(offsetPrevEp < tickTransactionsSize);
                            Transaction* transactionCurEp = tickTransactions.ptr(offset);
                            Transaction* transactionPrevEp = tickTransactions.ptr(offsetPrevEp);
                            ASSERT(transactionCurEp->checkValidity());
                            ASSERT(transactionPrevEp->checkValidity());
                            ASSERT(transactionPrevEp->tick == tickId);
                            ASSERT(transactionPrevEp->tick == tickId);
                            ASSERT(transactionPrevEp->amount == transactionCurEp->amount);
                            ASSERT(transactionPrevEp->sourcePublicKey == transactionCurEp->sourcePublicKey);
                            ASSERT(transactionPrevEp->destinationPublicKey == transactionCurEp->destinationPublicKey);
                            ASSERT(transactionPrevEp->inputSize == transactionCurEp->inputSize);
                            ASSERT(transactionPrevEp->inputType == transactionCurEp->inputType);
                            ASSERT(offset + transactionCurEp->totalSize() <= tickTransactionsSizeCurrentEpoch);
                            ASSERT(offsetPrevEp + transactionPrevEp->totalSize() <= tickTransactionsSize);
                        }
                    }
                }
            }

            setMem(tickTransactionOffsetsPtr, tickTransactionOffsetsSizeCurrentEpoch, 0);
            setMem(tickTransactionsPtr, tickTransactionsSizeCurrentEpoch, 0);

            copyMem(oldTxsDigestsPtr, &txsDigestsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK], tickCount * NUMBER_OF_TRANSACTIONS_PER_TICK * 32ULL);
            copyMem(&numSavedTxsPerTick[MAX_NUMBER_OF_TICKS_PER_EPOCH], &numSavedTxsPerTick[tickIndex], tickCount * sizeof(numSavedTxsPerTick[0]));

            setMem(txsDigestsPtr, txsDigestsSizeCurrentEpoch, 0);
            setMem(numSavedTxsPerTick, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(numSavedTxsPerTick[0]), 0);

            // Some txs from previous epoch might not have fit into prev epoch memory, check and adjust numSavedTxsPerTick accordingly.
            // Additionally move offsets and digests s.t. there are no empty entries at the beginning of the array storage for each tick.
            for (unsigned int t = oldTickBegin; t < oldTickEnd; ++t)
            {
                unsigned int index = tickToIndexPreviousEpoch(t);
                unsigned long long* offsets = tickTransactionOffsets.getByTickIndex(index);
                m256i* digests = &txsDigestsPtr[index * NUMBER_OF_TRANSACTIONS_PER_TICK];
                if (numSavedTxsPerTick[index])
                {
                    int firstNonZero = -1;
                    for (int tx = 0; tx < NUMBER_OF_TRANSACTIONS_PER_TICK; ++tx)
                    {
                        if (offsets[tx])
                        {
                            firstNonZero = tx;
                            break;
                        }
                    }
                    if (firstNonZero < 0)
                    {
                        numSavedTxsPerTick[index] = 0;
                    }
                    else if (firstNonZero > 0)
                    {
                        numSavedTxsPerTick[index] -= firstNonZero;
                        for (unsigned int tx = 0; tx < numSavedTxsPerTick[index]; ++tx)
                        {
                            offsets[tx] = offsets[tx + firstNonZero];
                            digests[tx] = digests[tx + firstNonZero];
                        }
                        setMem(&offsets[numSavedTxsPerTick[index]], firstNonZero * sizeof(unsigned long long), 0);
                        setMem(&digests[numSavedTxsPerTick[index]], firstNonZero * sizeof(m256i), 0);
                    }
                }
            }
        }
        else
        {
            // node startup with no data of prior epoch (also use storage for prior epoch for current)
            setMem(tickTransactionOffsetsPtr, tickTransactionOffsetsSize, 0);
            setMem(tickTransactionsPtr, tickTransactionsSize, 0);
            setMem(txsDigestsPtr, txsDigestsSize, 0);
            setMem(numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

            oldTickBegin = 0;
            oldTickEnd = 0;
        }

        tickBegin = newInitialTick;
        tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;

        nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End txsPool.beginEpoch()");
#endif
    }

    // Useful for debugging, but expensive: check that everything is as expected.
    static void checkStateConsistencyWithAssert()
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin tsxPool.checkStateConsistencyWithAssert()");
        CHAR16 dbgMsgBuf[200];
        setText(dbgMsgBuf, L"oldTickBegin=");
        appendNumber(dbgMsgBuf, oldTickBegin, FALSE);
        appendText(dbgMsgBuf, L", oldTickEnd=");
        appendNumber(dbgMsgBuf, oldTickEnd, FALSE);
        appendText(dbgMsgBuf, L", tickBegin=");
        appendNumber(dbgMsgBuf, tickBegin, FALSE);
        appendText(dbgMsgBuf, L", tickEnd=");
        appendNumber(dbgMsgBuf, tickEnd, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        ASSERT(tickBegin <= tickEnd);
        ASSERT(tickEnd - tickBegin <= MAX_NUMBER_OF_TICKS_PER_EPOCH);
        ASSERT(oldTickBegin <= oldTickEnd);
        ASSERT(oldTickEnd - oldTickBegin <= TICKS_TO_KEEP_FROM_PRIOR_EPOCH);
        ASSERT(oldTickEnd <= tickBegin);

        ASSERT(tickTransactionsPtr != nullptr);
        ASSERT(tickTransactionOffsetsPtr != nullptr);
        ASSERT(txsDigestsPtr != nullptr);

        ASSERT(oldTickTransactionsPtr != nullptr);
        ASSERT(oldTickTransactionOffsetsPtr != nullptr);
        ASSERT(oldTxsDigestsPtr != nullptr);

        ASSERT(oldTickTransactionsPtr == tickTransactionsPtr + tickTransactionsSizeCurrentEpoch);
        ASSERT(oldTickTransactionOffsetsPtr == tickTransactionOffsetsPtr + maxNumTxsCurrentEpoch);
        ASSERT(oldTxsDigestsPtr == txsDigestsPtr + maxNumTxsCurrentEpoch);

        ASSERT(nextTickTransactionOffset >= FIRST_TICK_TRANSACTION_OFFSET);
        ASSERT(nextTickTransactionOffset <= tickTransactionsSizeCurrentEpoch);

        // Check previous epoch data
        for (unsigned int tickId = oldTickBegin; tickId < oldTickEnd; ++tickId)
        {
            const unsigned long long* tickOffsets = tickTransactionOffsets.getByTickInPreviousEpoch(tickId);
            for (unsigned int transactionIdx = 0; transactionIdx < NUMBER_OF_TRANSACTIONS_PER_TICK; ++transactionIdx)
            {
                unsigned long long offset = tickOffsets[transactionIdx];
                if (offset)
                {
                    Transaction* transaction = tickTransactions.ptr(offset);
                    ASSERT(transaction->checkValidity());
                    ASSERT(transaction->tick == tickId);
#if !defined(NDEBUG) && !defined(NO_UEFI)
                    if (!transaction->checkValidity() || transaction->tick != tickId)
                    {
                        setText(dbgMsgBuf, L"Error in previous epoch transaction ");
                        appendNumber(dbgMsgBuf, transactionIdx, FALSE);
                        appendText(dbgMsgBuf, L" in tick ");
                        appendNumber(dbgMsgBuf, tickId, FALSE);
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

                        addDebugMessage(L"Skipping to check current epoch transactions and ticks");
                        goto test_current_epoch;
                    }
#endif
                }
            }
        }

        // Check current epoch data
#if !defined(NDEBUG) && !defined(NO_UEFI)
        test_current_epoch:
#endif
        unsigned long long lastTransactionEndOffset = FIRST_TICK_TRANSACTION_OFFSET;
        for (unsigned int tickId = tickBegin; tickId < tickEnd; ++tickId)
        {
            const unsigned long long* tickOffsets = tickTransactionOffsets.getByTickInCurrentEpoch(tickId);
            for (unsigned int transactionIdx = 0; transactionIdx < NUMBER_OF_TRANSACTIONS_PER_TICK; ++transactionIdx)
            {
                unsigned long long offset = tickOffsets[transactionIdx];
                if (offset)
                {
                    Transaction* transaction = tickTransactions.ptr(offset);
                    ASSERT(transaction->checkValidity());
                    ASSERT(transaction->tick == tickId);
#if !defined(NDEBUG) && !defined(NO_UEFI)
                    if (!transaction->checkValidity() || transaction->tick != tickId)
                    {
                        setText(dbgMsgBuf, L"Error in current epoch transaction ");
                        appendNumber(dbgMsgBuf, transactionIdx, FALSE);
                        appendText(dbgMsgBuf, L" in tick ");
                        appendNumber(dbgMsgBuf, tickId, FALSE);
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

                        addDebugMessage(L"Skipping to check digests and number of saved txs");
                        goto test_digests_and_num_saved;
                    }
#endif

                    unsigned long long transactionEndOffset = offset + transaction->totalSize();
                    if (lastTransactionEndOffset < transactionEndOffset)
                        lastTransactionEndOffset = transactionEndOffset;
                }
            }
        }
        ASSERT(lastTransactionEndOffset == nextTickTransactionOffset);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        test_digests_and_num_saved:
#endif
        // Check that transaction digests and number of saved txs per tick are consistent with transactions
        unsigned int begins[2] = { oldTickBegin, tickBegin };
        unsigned int ends[2] = { oldTickEnd, tickEnd };
        typedef unsigned int (*TickToIndexFunction) (unsigned int t);
        TickToIndexFunction tickToIndexFc[2] = { tickToIndexPreviousEpoch, tickToIndexCurrentEpoch };
        for (unsigned int section = 0; section < 2; ++section)
        {
            for (unsigned int tickId = begins[section]; tickId < ends[section]; ++tickId)
            {
                unsigned int tickIndex = tickToIndexFc[section](tickId);
                const unsigned int numSavedTxs = numSavedTxsPerTick[tickIndex];
                ASSERT(numSavedTxs <= NUMBER_OF_TRANSACTIONS_PER_TICK);
                for (unsigned int txIndex = 0; txIndex < numSavedTxs; ++txIndex)
                {
                    unsigned long long txOffset = tickTransactionOffsets.getByTickIndex(tickIndex)[txIndex];
                    ASSERT(txOffset != 0);
                    Transaction* txPtrFromStorage = tickTransactions.ptr(txOffset);
                    Transaction* txPtr = get(tickId, txIndex);
                    ASSERT(txPtr);
                    ASSERT(txPtr == txPtrFromStorage);
                }
                for (unsigned int txIndex = numSavedTxs; txIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; ++txIndex)
                {
                    unsigned long long txOffset = tickTransactionOffsets.getByTickIndex(tickIndex)[txIndex];
                    ASSERT(txOffset == 0);
                    Transaction* txPtr = get(tickId, txIndex);
                    ASSERT(txPtr == nullptr);
                }
            }
        }

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End txsPool.checkStateConsistencyWithAssert()");
#endif
    }

};