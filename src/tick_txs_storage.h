#pragma once

#include "network_messages/transactions.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"
#include "platform/debugging.h"

#include "public_settings.h"

class TickTransactionsStorage
{
public:
    static constexpr unsigned long long tickDataLength = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;

    static constexpr unsigned long long tickTransactionsSizeCurrentEpoch = FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS);
    static constexpr unsigned long long tickTransactionsSizePreviousEpoch = (((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS);
    static constexpr unsigned long long tickTransactionsSize = tickTransactionsSizeCurrentEpoch + tickTransactionsSizePreviousEpoch;

    static constexpr unsigned long long tickTransactionOffsetsLengthCurrentEpoch = ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK;
    static constexpr unsigned long long tickTransactionOffsetsLengthPreviousEpoch = ((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK;
    static constexpr unsigned long long tickTransactionOffsetsLength = tickTransactionOffsetsLengthCurrentEpoch + tickTransactionOffsetsLengthPreviousEpoch;
    static constexpr unsigned long long tickTransactionOffsetsSizeCurrentEpoch = tickTransactionOffsetsLengthCurrentEpoch * sizeof(unsigned long long);
    static constexpr unsigned long long tickTransactionOffsetsSizePreviousEpoch = tickTransactionOffsetsLengthPreviousEpoch * sizeof(unsigned long long);
    static constexpr unsigned long long tickTransactionOffsetsSize = tickTransactionOffsetsLength * sizeof(unsigned long long);

    // Tick number range of current epoch storage
    unsigned int tickBegin = 0;
    unsigned int tickEnd = 0;

    // Tick number range of previous epoch storage
    unsigned int oldTickBegin = 0;
    unsigned int oldTickEnd = 0;

    // Allocated tickTransactions buffer with tickTransactionsSize bytes (includes current and previous epoch data)
    unsigned char* tickTransactionsPtr = nullptr;

    // Allocated tickTransactionOffsets buffer with tickTransactionOffsetsLength elements (includes current and previous epoch data)
    unsigned long long* tickTransactionOffsetsPtr = nullptr;

    // Tick transaction buffer of previous epoch. Points to tickTransactionsPtr + tickTransactionsSizeCurrentEpoch.
    unsigned char* oldTickTransactionsPtr = nullptr;

    // Tick transaction offsets of previous epoch. Points to tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch.
    unsigned long long* oldTickTransactionOffsetsPtr = nullptr;

    // Lock for securing tickTransactions and tickTransactionOffsets
    volatile char tickTransactionsLock = 0;

    bool init()
    {
        if (!allocatePool(tickTransactionsSize, (void**)&tickTransactionsPtr)
            || !allocatePool(tickTransactionOffsetsSize, (void**)&tickTransactionOffsetsPtr))
        {
            logToConsole(L"Failed to allocate tick transactions storage memory!");
            return false;
        }

        ASSERT(tickTransactionsLock == 0);
        nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

        oldTickTransactionsPtr = tickTransactionsPtr + tickTransactionsSizeCurrentEpoch;
        oldTickTransactionOffsetsPtr = tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch;

        tickTransactions.setStoragePtr(this);
        tickTransactionOffsets.setStoragePtr(this);

        tickBegin = 0;
        tickEnd = 0;
        oldTickBegin = 0;
        oldTickEnd = 0;

        return true;
    }

    // Cleanup at node shutdown
    void deinit()
    {
        if (tickTransactionOffsetsPtr)
        {
            freePool(tickTransactionOffsetsPtr);
        }

        if (tickTransactionsPtr)
        {
            freePool(tickTransactionsPtr);
        }
    }

    // Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
    // are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
    void beginEpoch(unsigned int newInitialTick)
    {
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
                const unsigned long long offsetDelta = (tickTransactionsSizeCurrentEpoch + keepTransactionSizesSum) - nextTickTransactionOffset;
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
        }
        else
        {
            // node startup with no data of prior epoch (also use storage for prior epoch for current)
            setMem(tickTransactionOffsetsPtr, tickTransactionOffsetsSize, 0);
            setMem(tickTransactionsPtr, tickTransactionsSize, 0);
            oldTickBegin = 0;
            oldTickEnd = 0;
        }

        tickBegin = newInitialTick;
        tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;

        nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;
    }

    // Useful for debugging, but expensive: check that everything is as expected.
    void checkStateConsistencyWithAssert()
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin ts.transactionsStorage.checkStateConsistencyWithAssert()");
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
        ASSERT(tickEnd - tickBegin <= tickDataLength);
        ASSERT(oldTickBegin <= oldTickEnd);
        ASSERT(oldTickEnd - oldTickBegin <= TICKS_TO_KEEP_FROM_PRIOR_EPOCH);
        ASSERT(oldTickEnd <= tickBegin);

        ASSERT(tickTransactionsPtr != nullptr);
        ASSERT(tickTransactionOffsetsPtr != nullptr);
        ASSERT(oldTickTransactionsPtr == tickTransactionsPtr + tickTransactionsSizeCurrentEpoch);
        ASSERT(oldTickTransactionOffsetsPtr == tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch);

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
                        setText(dbgMsgBuf, L"Error in prev. epoch transaction ");
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

                        addDebugMessage(L"Skipping to check more transactions and ticks");
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
                        setText(dbgMsgBuf, L"Error in cur. epoch transaction ");
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

                        addDebugMessage(L"Skipping to check more transactions and ticks");
                        goto leave_test;
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
        leave_test:
        addDebugMessage(L"End ts.transactionsStorage.checkStateConsistencyWithAssert()");
#endif
    }

    // Check whether tick is stored in the current epoch storage.
    inline bool tickInCurrentEpochStorage(unsigned int tick)
    {
        return tick >= tickBegin && tick < tickEnd;
    }

    // Check whether tick is stored in the previous epoch storage.
    inline bool tickInPreviousEpochStorage(unsigned int tick)
    {
        return oldTickBegin <= tick && tick < oldTickEnd;
    }

    // Return index of tick data in current epoch (does not check tick).
    unsigned int tickToIndexCurrentEpoch(unsigned int tick)
    {
        return tick - tickBegin;
    }

    // Return index of tick data in previous epoch (does not check that it is stored).
    unsigned int tickToIndexPreviousEpoch(unsigned int tick)
    {
        return tick - oldTickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }

    // Struct for structured, convenient access via ".tickTransactionOffsets"
    struct TickTransactionOffsetsAccess
    {
        void setStoragePtr(TickTransactionsStorage* ptr)
        {
            storagePtr = ptr;
        }

        // Return pointer to offset array of transactions by tick index independent of epoch (checking index with ASSERT)
        inline unsigned long long* getByTickIndex(unsigned int tickIndex)
        {
            ASSERT(tickIndex < tickDataLength);
            return storagePtr->tickTransactionOffsetsPtr + (tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK);
        }

        // Return pointer to offset array of transactions of tick in current epoch by tick (checking tick with ASSERT)
        inline unsigned long long* getByTickInCurrentEpoch(unsigned int tick)
        {
            ASSERT(storagePtr->tickInCurrentEpochStorage(tick));
            const unsigned int tickIndex = storagePtr->tickToIndexCurrentEpoch(tick);
            return getByTickIndex(tickIndex);
        }

        // Return pointer to offset array of transactions of tick in previous epoch by tick (checking tick with ASSERT)
        inline unsigned long long* getByTickInPreviousEpoch(unsigned int tick)
        {
            ASSERT(storagePtr->tickInPreviousEpochStorage(tick));
            const unsigned int tickIndex = storagePtr->tickToIndexPreviousEpoch(tick);
            return getByTickIndex(tickIndex);
        }

        // Return reference to offset by tick and transaction in current epoch (checking inputs with ASSERT)
        inline unsigned long long& operator()(unsigned int tick, unsigned int transaction)
        {
            ASSERT(transaction < NUMBER_OF_TRANSACTIONS_PER_TICK);
            return getByTickInCurrentEpoch(tick)[transaction];
        }

    private:
        TickTransactionsStorage* storagePtr = nullptr;
    } tickTransactionOffsets;

    // Offset of next free space in tick transaction storage
    inline static unsigned long long nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

    // Struct for structured, convenient access via ".tickTransactions"
    struct TickTransactionsAccess
    {
        void setStoragePtr(TickTransactionsStorage* ptr)
        {
            storagePtr = ptr;
        }

        void acquireLock()
        {
            ACQUIRE(storagePtr->tickTransactionsLock);
        }

        void releaseLock()
        {
            RELEASE(storagePtr->tickTransactionsLock);
        }

        // Number of bytes available for transactions in current epoch
        unsigned long long storageSpaceCurrentEpoch = tickTransactionsSizeCurrentEpoch;

        // Return pointer to Transaction based on transaction offset independent of epoch (checking offset with ASSERT)
        inline Transaction* ptr(unsigned long long transactionOffset)
        {
            ASSERT(transactionOffset < tickTransactionsSize);
            return (Transaction*)(storagePtr->tickTransactionsPtr + transactionOffset);
        }

        // Return pointer to Transaction based on transaction offset independent of epoch (checking offset with ASSERT)
        inline Transaction* operator()(unsigned long long transactionOffset)
        {
            ASSERT(transactionOffset < tickTransactionsSize);
            return ptr(transactionOffset);
        }

    private:
        TickTransactionsStorage* storagePtr = nullptr;
    } tickTransactions;
};

