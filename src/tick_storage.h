#pragma once

#include "network_messages/tick.h"
#include "network_messages/transactions.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"
#include "platform/debugging.h"

#include "public_settings.h"



// Encapsulated tick storage of current epoch that can additionally keep the last ticks of the previous epoch.
// The number of ticks to keep from the previous epoch is TICKS_TO_KEEP_FROM_PRIOR_EPOCH (defined in public_settings.h).
//
// This is a kind of singleton class with only static members (so all instances refer to the same data).
//
// It comprises:
// - tickData (one TickData struct per tick)
// - ticks (one Tick struct per tick and Computor)
// - tickTransactions (continuous buffer efficiently storing the variable-size transactions)
// - tickTransactionOffsets (offsets of transactions in buffer, order in tickTransactions may differ)
// - nextTickTransactionOffset (offset of next transition to be added)
class TickStorage
{
private:
    static constexpr unsigned long long tickDataLength = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
    static constexpr unsigned long long tickDataSize = tickDataLength * sizeof(TickData);
    
    static constexpr unsigned long long ticksLengthCurrentEpoch = ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_COMPUTORS;
    static constexpr unsigned long long ticksLengthPreviousEpoch = ((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_COMPUTORS;
    static constexpr unsigned long long ticksLength = ticksLengthCurrentEpoch + ticksLengthPreviousEpoch;
    static constexpr unsigned long long ticksSize = ticksLength * sizeof(Tick);

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
    inline static unsigned int tickBegin = 0;
    inline static unsigned int tickEnd = 0;

    // Tick number range of previous epoch storage
    inline static unsigned int oldTickBegin = 0;
    inline static unsigned int oldTickEnd = 0;

    // Allocated tick data buffer with tickDataLength elements (includes current and previous epoch data)
    inline static TickData* tickDataPtr = nullptr;

    // Allocated ticks buffer with ticksLength elements (includes current and previous epoch data)
    inline static Tick* ticksPtr = nullptr;

    // Allocated tickTransactions buffer with tickTransactionsSize bytes (includes current and previous epoch data)
    inline static unsigned char* tickTransactionsPtr = nullptr;

    // Allocated tickTransactionOffsets buffer with tickTransactionOffsetsLength elements (includes current and previous epoch data)
    inline static unsigned long long* tickTransactionOffsetsPtr = nullptr;

    // Tick data of previous epoch. Points to tickData + MAX_NUMBER_OF_TICKS_PER_EPOCH
    inline static TickData* oldTickDataPtr = nullptr;

    // Ticks of previous epoch. Points to ticksPtr + ticksLengthCurrentEpoch
    inline static Tick* oldTicksPtr = nullptr;

    // Tick transaction buffer of previous epoch. Points to tickTransactionsPtr + tickTransactionsSizeCurrentEpoch.
    inline static unsigned char* oldTickTransactionsPtr = nullptr;

    // Tick transaction offsets of previous epoch. Points to tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch.
    inline static unsigned long long* oldTickTransactionOffsetsPtr = nullptr;


    // Lock for securing tickData
    inline static volatile char tickDataLock = 0;

    // One lock per computor for securing ticks element in current tick (only the tick system.tick is written)
    inline static volatile char ticksLocks[NUMBER_OF_COMPUTORS];

    // Lock for securing tickTransactions and tickTransactionOffsets
    inline static volatile char tickTransactionsLock = 0;

public:

    // Init at node startup
    static bool init()
    {
        // TODO: allocate everything with one continuous buffer
        if (!allocatePool(tickDataSize, (void**)&tickDataPtr)
            || !allocatePool(ticksSize, (void**)&ticksPtr)
            || !allocatePool(tickTransactionsSize, (void**)&tickTransactionsPtr)
            || !allocatePool(tickTransactionOffsetsSize, (void**)&tickTransactionOffsetsPtr))
        {
            logToConsole(L"Failed to allocate tick storage memory!");
            return false;
        }

        ASSERT(tickDataLock == 0);
        setMem((void*)ticksLocks, sizeof(ticksLocks), 0);
        ASSERT(tickTransactionsLock == 0);
        nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

        oldTickDataPtr = tickDataPtr + MAX_NUMBER_OF_TICKS_PER_EPOCH;
        oldTicksPtr = ticksPtr + ticksLengthCurrentEpoch;
        oldTickTransactionsPtr = tickTransactionsPtr + tickTransactionsSizeCurrentEpoch;
        oldTickTransactionOffsetsPtr = tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch;

        tickBegin = 0;
        tickEnd = 0;
        oldTickBegin = 0;
        oldTickEnd = 0;

        return true;
    }

    // Cleanup at node shutdown
    static void deinit()
    {
        if (tickDataPtr)
        {
            freePool(tickDataPtr);
        }

        if (ticksPtr)
        {
            freePool(ticksPtr);
        }

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
    static void beginEpoch(unsigned int newInitialTick)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin ts.beginEpoch()");
        CHAR16 dbgMsgBuf[300];
#endif
        if (tickBegin && tickInCurrentEpochStorage(newInitialTick) && tickBegin < newInitialTick)
        {
            // seamless epoch transition: keep some ticks of prior epoch
            oldTickEnd = newInitialTick;
            oldTickBegin = newInitialTick - TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
            if (oldTickBegin < tickBegin)
                oldTickBegin = tickBegin;

#if !defined(NDEBUG) && !defined(NO_UEFI)
            setText(dbgMsgBuf, L"Keep ticks of prior epoch: oldTickBegin=");
            appendNumber(dbgMsgBuf, oldTickBegin, FALSE);
            appendText(dbgMsgBuf, L", oldTickEnd=");
            appendNumber(dbgMsgBuf, oldTickEnd, FALSE);
            addDebugMessage(dbgMsgBuf);
#endif

            const unsigned int tickIndex = tickToIndexCurrentEpoch(oldTickBegin);
            const unsigned int tickCount = oldTickEnd - oldTickBegin;

            // copy ticks and tick data from recently ended epoch into storage of previous epoch
            copyMem(oldTickDataPtr, tickDataPtr + tickIndex, tickCount * sizeof(TickData));
            copyMem(oldTicksPtr, ticksPtr + (tickIndex * NUMBER_OF_COMPUTORS), tickCount * NUMBER_OF_COMPUTORS * sizeof(Tick));

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
                    const unsigned long long* tickOffsets = TickTransactionOffsetsAccess::getByTickInCurrentEpoch(tickId);
                    unsigned long long* tickOffsetsPrevEp = TickTransactionOffsetsAccess::getByTickInPreviousEpoch(tickId);
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
                            Transaction* transactionCurEp = TickTransactionsAccess::ptr(offset);
                            Transaction* transactionPrevEp = TickTransactionsAccess::ptr(offsetPrevEp);
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

            // reset data storage of new epoch
            setMem(tickDataPtr, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(TickData), 0);
            setMem(ticksPtr, ticksLengthCurrentEpoch * sizeof(Tick), 0);
            setMem(tickTransactionOffsetsPtr, tickTransactionOffsetsSizeCurrentEpoch, 0);
            setMem(tickTransactionsPtr, tickTransactionsSizeCurrentEpoch, 0);
        }
        else
        {
            // node startup with no data of prior epoch (also use storage for prior epoch for current)
            setMem(tickDataPtr, tickDataSize, 0);
            setMem(ticksPtr, ticksSize, 0);
            setMem(tickTransactionOffsetsPtr, tickTransactionOffsetsSize, 0);
            setMem(tickTransactionsPtr, tickTransactionsSize, 0);
            oldTickBegin = 0;
            oldTickEnd = 0;
        }
        
        tickBegin = newInitialTick;
        tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;

        nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"End ts.beginEpoch()");
#endif
    }

    // Useful for debugging, but expensive: check that everything is as expected.
    static void checkStateConsistencyWithAssert()
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Begin ts.checkStateConsistencyWithAssert()");
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

        ASSERT(tickDataPtr != nullptr);
        ASSERT(ticksPtr != nullptr);
        ASSERT(tickTransactionsPtr != nullptr);
        ASSERT(tickTransactionOffsetsPtr != nullptr);
        ASSERT(oldTickDataPtr == tickDataPtr + MAX_NUMBER_OF_TICKS_PER_EPOCH);
        ASSERT(oldTicksPtr == ticksPtr + ticksLengthCurrentEpoch);
        ASSERT(oldTickTransactionsPtr == tickTransactionsPtr + tickTransactionsSizeCurrentEpoch);
        ASSERT(oldTickTransactionOffsetsPtr == tickTransactionOffsetsPtr + tickTransactionOffsetsLengthCurrentEpoch);

        ASSERT(nextTickTransactionOffset >= FIRST_TICK_TRANSACTION_OFFSET);
        ASSERT(nextTickTransactionOffset <= tickTransactionsSizeCurrentEpoch);

        // Check previous epoch data
        for (unsigned int tickId = oldTickBegin; tickId < oldTickEnd; ++tickId)
        {
            const TickData& tickData = TickDataAccess::getByTickInPreviousEpoch(tickId);
            ASSERT(tickData.epoch == 0 || (tickData.tick == tickId));

            const Tick* computorsTicks = TicksAccess::getByTickInPreviousEpoch(tickId);
            for (unsigned int computor = 0; computor < NUMBER_OF_COMPUTORS; ++computor)
            {
                const Tick& computorTick = computorsTicks[computor];
                ASSERT(computorTick.epoch == 0 || (computorTick.tick == tickId && computorTick.computorIndex == computor));
            }

            const unsigned long long* tickOffsets = TickTransactionOffsetsAccess::getByTickInPreviousEpoch(tickId);
            for (unsigned int transactionIdx = 0; transactionIdx < NUMBER_OF_TRANSACTIONS_PER_TICK; ++transactionIdx)
            {
                unsigned long long offset = tickOffsets[transactionIdx];
                if (offset)
                {
                    Transaction* transaction = TickTransactionsAccess::ptr(offset);
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
            const TickData& tickData = TickDataAccess::getByTickInCurrentEpoch(tickId);
            ASSERT(tickData.epoch == 0 || (tickData.tick == tickId));

            const Tick* computorsTicks = TicksAccess::getByTickInCurrentEpoch(tickId);
            for (unsigned int computor = 0; computor < NUMBER_OF_COMPUTORS; ++computor)
            {
                const Tick& computorTick = computorsTicks[computor];
                ASSERT(computorTick.epoch == 0 || (computorTick.tick == tickId && computorTick.computorIndex == computor));
            }

            const unsigned long long* tickOffsets = TickTransactionOffsetsAccess::getByTickInCurrentEpoch(tickId);
            for (unsigned int transactionIdx = 0; transactionIdx < NUMBER_OF_TRANSACTIONS_PER_TICK; ++transactionIdx)
            {
                unsigned long long offset = tickOffsets[transactionIdx];
                if (offset)
                {
                    Transaction* transaction = TickTransactionsAccess::ptr(offset);
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
        addDebugMessage(L"End ts.checkStateConsistencyWithAssert()");
#endif
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
    inline static unsigned int tickToIndexCurrentEpoch(unsigned int tick)
    {
        return tick - tickBegin;
    }

    // Return index of tick data in previous epoch (does not check that it is stored).
    inline static unsigned int tickToIndexPreviousEpoch(unsigned int tick)
    {
        return tick - oldTickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }

    // Struct for structured, convenient access via ".tickData"
    struct TickDataAccess
    {
        inline static void acquireLock()
        {
            ACQUIRE(tickDataLock);
        }

        inline static void releaseLock()
        {
            RELEASE(tickDataLock);
        }

        // Return tick if it is stored and not empty, or nullptr otherwise (always checks tick).
        inline static TickData* getByTickIfNotEmpty(unsigned int tick)
        {
            unsigned int index;
            if (tickInCurrentEpochStorage(tick))
                index = tickToIndexCurrentEpoch(tick);
            else if (tickInPreviousEpochStorage(tick))
                index = tickToIndexPreviousEpoch(tick);
            else
                return nullptr;

            TickData* td = tickDataPtr + index;
            if (td->epoch == 0)
                return nullptr;

            return td;
        }

        // Get tick data by tick in current epoch (checking tick with ASSERT)
        inline static TickData& getByTickInCurrentEpoch(unsigned int tick)
        {
            ASSERT(tickInCurrentEpochStorage(tick));
            return tickDataPtr[tickToIndexCurrentEpoch(tick)];
        }

        // Get tick data by tick in previous epoch (checking tick with ASSERT)
        inline static TickData& getByTickInPreviousEpoch(unsigned int tick)
        {
            ASSERT(tickInPreviousEpochStorage(tick));
            return tickDataPtr[tickToIndexPreviousEpoch(tick)];
        }

        // Get tick data at index independent of epoch (checking index with ASSERT)
        inline TickData& operator[](unsigned int index)
        {
            ASSERT(index < tickDataLength);
            return tickDataPtr[index];
        }

        // Get tick data at index independent of epoch (checking index with ASSERT)
        inline const TickData& operator[](unsigned int index) const
        {
            ASSERT(index < tickDataLength);
            return tickDataPtr[index];
        }
    } tickData;

    // Struct for structured, convenient access via ".ticks"
    struct TicksAccess
    {
        // Acquire lock for ticks element of specific computor (only ticks >= system.tick are written)
        inline static void acquireLock(unsigned short computorIndex)
        {
            ACQUIRE(ticksLocks[computorIndex]);
        }

        // Release lock for ticks element of specific computor (only ticks >= system.tick are written)
        inline static void releaseLock(unsigned short computorIndex)
        {
            RELEASE(ticksLocks[computorIndex]);
        }

        // Return pointer to array of one Tick per computor by tick index independent of epoch (checking index with ASSERT)
        inline static Tick* getByTickIndex(unsigned int tickIndex)
        {
            ASSERT(tickIndex < tickDataLength);
            return ticksPtr + tickIndex * NUMBER_OF_COMPUTORS;
        }

        // Return pointer to array of one Tick per computor in current epoch by tick (checking tick with ASSERT)
        inline static Tick* getByTickInCurrentEpoch(unsigned int tick)
        {
            ASSERT(tickInCurrentEpochStorage(tick));
            return ticksPtr + tickToIndexCurrentEpoch(tick) * NUMBER_OF_COMPUTORS;
        }

        // Return pointer to array of one Tick per computor in previous epoch by tick (checking tick with ASSERT)
        inline static Tick* getByTickInPreviousEpoch(unsigned int tick)
        {
            ASSERT(tickInPreviousEpochStorage(tick));
            return ticksPtr + tickToIndexPreviousEpoch(tick) * NUMBER_OF_COMPUTORS;
        }

        // Get ticks element at offset (checking offset with ASSERT)
        inline Tick& operator[](unsigned int offset)
        {
            ASSERT(offset < ticksLength);
            return ticksPtr[offset];
        }

        // Get ticks element at offset (checking offset with ASSERT)
        inline const Tick& operator[](unsigned int offset) const
        {
            ASSERT(offset < ticksLength);
            return ticksPtr[offset];
        }
    } ticks;

    // Struct for structured, convenient access via ".tickTransactionOffsets"
    struct TickTransactionOffsetsAccess
    {
        // Return pointer to offset array of transactions by tick index independent of epoch (checking index with ASSERT)
        inline static unsigned long long* getByTickIndex(unsigned int tickIndex)
        {
            ASSERT(tickIndex < tickDataLength);
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
        inline unsigned long long& operator()(unsigned int tick, unsigned int transaction)
        {
            ASSERT(transaction < NUMBER_OF_TRANSACTIONS_PER_TICK);
            return getByTickInCurrentEpoch(tick)[transaction];
        }
    } tickTransactionOffsets;

    // Offset of next free space in tick transaction storage
    inline static unsigned long long nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

    // Struct for structured, convenient access via ".tickTransactions"
    struct TickTransactionsAccess
    {
        inline static void acquireLock()
        {
            ACQUIRE(tickTransactionsLock);
        }

        inline static void releaseLock()
        {
            RELEASE(tickTransactionsLock);
        }

        // Number of bytes available for transactions in current epoch
        static constexpr unsigned long long storageSpaceCurrentEpoch = tickTransactionsSizeCurrentEpoch;

        // Return pointer to Transaction based on transaction offset independent of epoch (checking offset with ASSERT)
        inline static Transaction* ptr(unsigned long long transactionOffset)
        {
            ASSERT(transactionOffset < tickTransactionsSize);
            return (Transaction*)(tickTransactionsPtr + transactionOffset);
        }

        // Return pointer to Transaction based on transaction offset independent of epoch (checking offset with ASSERT)
        inline Transaction * operator()(unsigned long long transactionOffset)
        {
            return ptr(transactionOffset);
        }
    } tickTransactions;
};
