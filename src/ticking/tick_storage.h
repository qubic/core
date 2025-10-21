#pragma once

#include "network_messages/tick.h"
#include "network_messages/transactions.h"

#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"
#include "platform/debugging.h"

#include "public_settings.h"

#if TICK_STORAGE_AUTOSAVE_MODE
static unsigned short SNAPSHOT_METADATA_FILE_NAME[] = L"snapshotMetadata.???";
static unsigned short SNAPSHOT_TICK_DATA_FILE_NAME[] = L"snapshotTickdata.???";
static unsigned short SNAPSHOT_TICKS_FILE_NAME[] = L"snapshotTicks.???";
static unsigned short SNAPSHOT_TICK_TRANSACTION_OFFSET_FILE_NAME[] = L"snapshotTickTransactionOffsets.???";
static unsigned short SNAPSHOT_TRANSACTIONS_FILE_NAME[] = L"snapshotTickTransaction.???";
#endif
constexpr unsigned short INVALIDATED_TICK_DATA = 0xffff;
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
// - nextTickTransactionOffset (offset of next transaction to be added)
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

    // Allocated transaction access digest buffer with current epoch transactions.
    inline static unsigned char* tickTransactionsDigestPtr = nullptr;

    // Lock for securing tickData
    inline static volatile char tickDataLock = 0;

    // One lock per computor for securing ticks element in current tick (only the tick system.tick is written)
    inline static volatile char ticksLocks[NUMBER_OF_COMPUTORS];

    // Lock for securing tickTransactions and tickTransactionOffsets
    inline static volatile char tickTransactionsLock = 0;

    // Lock for securing tickTransactions and tickTransactionsDigestPtr
    inline static volatile char tickTransactionsDigestAccessLock = 0;

#if TICK_STORAGE_AUTOSAVE_MODE
    struct MetaData {
        unsigned int epoch;
        unsigned int tickBegin;
        unsigned int tickEnd;
        long long outTotalTransactionSize;
        unsigned long long outNextTickTransactionOffset;
        // may need to store more meta data here to verify consistency when loading (ie: some nodes have different configs and can't use the saved files)
    } metaData;
    inline static unsigned long long lastCheckTransactionOffset = 0; // use for save/load transaction state
    void prepareMetaDataFilename(short epoch)
    {
        addEpochToFileName(SNAPSHOT_METADATA_FILE_NAME, sizeof(SNAPSHOT_METADATA_FILE_NAME) / sizeof(SNAPSHOT_METADATA_FILE_NAME[0]), epoch);
    }
    void prepareFilenames(short epoch)
    {
        prepareMetaDataFilename(epoch);
        addEpochToFileName(SNAPSHOT_TICK_DATA_FILE_NAME, sizeof(SNAPSHOT_TICK_DATA_FILE_NAME) / sizeof(SNAPSHOT_TICK_DATA_FILE_NAME[0]), epoch);
        addEpochToFileName(SNAPSHOT_TICKS_FILE_NAME, sizeof(SNAPSHOT_TICKS_FILE_NAME) / sizeof(SNAPSHOT_TICKS_FILE_NAME[0]), epoch);
        addEpochToFileName(SNAPSHOT_TICK_TRANSACTION_OFFSET_FILE_NAME, sizeof(SNAPSHOT_TICK_TRANSACTION_OFFSET_FILE_NAME) / sizeof(SNAPSHOT_TICK_TRANSACTION_OFFSET_FILE_NAME[0]), epoch);
        addEpochToFileName(SNAPSHOT_TRANSACTIONS_FILE_NAME, sizeof(SNAPSHOT_TRANSACTIONS_FILE_NAME) / sizeof(SNAPSHOT_TRANSACTIONS_FILE_NAME[0]), epoch);
    }
    bool saveMetaData(short epoch, unsigned int tickEnd, long long outTotalTransactionSize, unsigned long long outNextTickTransactionOffset, CHAR16* directory = NULL)
    {
        metaData.epoch = epoch;
        metaData.tickBegin = tickBegin;
        metaData.tickEnd = tickEnd;
        metaData.outTotalTransactionSize = outTotalTransactionSize;
        metaData.outNextTickTransactionOffset = outNextTickTransactionOffset;
        auto sz = saveLargeFile(SNAPSHOT_METADATA_FILE_NAME, sizeof(metaData), (unsigned char*) & metaData, directory);
        if (sz != sizeof(metaData))
        {
            return false;
        }
        return true;
    }
    bool saveTickData(unsigned long long nTick, CHAR16* directory = NULL)
    {
        long long totalWriteSize = nTick * sizeof(TickData);
        auto sz = saveLargeFile(SNAPSHOT_TICK_DATA_FILE_NAME, totalWriteSize, (unsigned char*)tickDataPtr, directory);
        if (sz != totalWriteSize)
        {
            return false;
        }
        return true;
    }
    bool saveTicks(unsigned long long nTick, CHAR16* directory = NULL)
    {
        long long totalWriteSize = nTick * sizeof(Tick) * NUMBER_OF_COMPUTORS;
        auto sz = saveLargeFile(SNAPSHOT_TICKS_FILE_NAME, totalWriteSize, (unsigned char*)ticksPtr, directory);
        if (sz != totalWriteSize)
        {
            return false;
        }
        return true;
    }
    bool saveTickTransactionOffsets(unsigned long long nTick, CHAR16* directory = NULL)
    {
        long long totalWriteSize = nTick * sizeof(tickTransactionOffsetsPtr[0]) * NUMBER_OF_TRANSACTIONS_PER_TICK;
        auto sz = saveLargeFile(SNAPSHOT_TICK_TRANSACTION_OFFSET_FILE_NAME, totalWriteSize, (unsigned char*)tickTransactionOffsetsPtr, directory);
        if (sz != totalWriteSize)
        {
            return false;
        }
        return true;
    }
    bool saveTransactions(unsigned long long nTick, long long& outTotalTransactionSize, unsigned long long& outNextTickTransactionOffset, CHAR16* directory = NULL)
    {
        unsigned int toTick = tickBegin + (unsigned int)(nTick);
        unsigned long long toPtr = 0;
        outNextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;
        lastCheckTransactionOffset = tickBegin > lastCheckTransactionOffset ? tickBegin : lastCheckTransactionOffset;
        // find the offset
        {
            unsigned long long maxOffset = FIRST_TICK_TRANSACTION_OFFSET;
            unsigned int tick = 0;
            for (tick = toTick; tick >= lastCheckTransactionOffset; tick--)
            {
                for (int idx = NUMBER_OF_TRANSACTIONS_PER_TICK - 1; idx >= 0; idx--)
                {
                    if (this->tickTransactionOffsets(tick, idx))
                    {
                        unsigned long long offset = this->tickTransactionOffsets(tick, idx);
                        Transaction* tx = (Transaction*)(tickTransactionsPtr + offset);
                        unsigned long long tmp = offset + tx->totalSize();
                        if (tmp > maxOffset){
                            maxOffset = tmp;
                            lastCheckTransactionOffset = tick;
                        }
                    }
                }
            }
            toPtr = maxOffset;
            outNextTickTransactionOffset = maxOffset;
        }
        
        // saving from the first tx of from tick to the last tx of (totick)
        long long totalWriteSize = toPtr;
        unsigned char* ptr = tickTransactionsPtr;
        auto sz = saveLargeFile(SNAPSHOT_TRANSACTIONS_FILE_NAME, totalWriteSize, (unsigned char*)ptr, directory);
        if (sz != totalWriteSize)
        {
            outTotalTransactionSize = -1;
            return false;
        }
        outTotalTransactionSize = totalWriteSize;

        return true;
    }
    bool loadMetaData(CHAR16* directory = NULL)
    {
        auto sz = loadLargeFile(SNAPSHOT_METADATA_FILE_NAME, sizeof(metaData), (unsigned char*)&metaData, directory);
        if (sz != sizeof(metaData))
        {
            return false;
        }
        return true;
    }
    bool checkMetaData()
    {
        if (metaData.tickBegin > metaData.tickEnd) {
            return false;
        }
        if (metaData.tickBegin != tickBegin) {
            return false;
        }
        if (metaData.tickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH < metaData.tickEnd) {
            return false;
        }
#ifndef NO_UEFI
        if (metaData.epoch != EPOCH) {
            return false;
        }
#endif
        return true;
    }
    bool loadTickData(unsigned long long nTick, CHAR16* directory = NULL)
    {
        long long totalLoadSize = nTick * sizeof(TickData);
        auto sz = loadLargeFile(SNAPSHOT_TICK_DATA_FILE_NAME, totalLoadSize, (unsigned char*)tickDataPtr, directory);
        if (sz != totalLoadSize)
        {
            return false;
        }
        return true;
    }
    bool loadTicks(unsigned long long nTick, CHAR16* directory = NULL)
    {
        long long totalLoadSize = nTick * sizeof(Tick) * NUMBER_OF_COMPUTORS;
        auto sz = loadLargeFile(SNAPSHOT_TICKS_FILE_NAME, totalLoadSize, (unsigned char*)ticksPtr, directory);
        if (sz != totalLoadSize)
        {
            return false;
        }
        return true;
    }
    bool loadTickTransactionOffsets(unsigned long long nTick, CHAR16* directory = NULL)
    {
        long long totalLoadSize = nTick * sizeof(tickTransactionOffsetsPtr[0]) * NUMBER_OF_TRANSACTIONS_PER_TICK;
        auto sz = loadLargeFile(SNAPSHOT_TICK_TRANSACTION_OFFSET_FILE_NAME, totalLoadSize, (unsigned char*)tickTransactionOffsetsPtr, directory);
        if (sz != totalLoadSize)
        {
            return false;
        }
        return true;
    }
    bool loadTransactions(unsigned long long nTick, unsigned long long totalLoadSize, CHAR16* directory = NULL)
    {
        unsigned char* ptr = tickTransactionsPtr;
        auto sz = loadLargeFile(SNAPSHOT_TRANSACTIONS_FILE_NAME, totalLoadSize, (unsigned char*)ptr, directory);
        if (sz != totalLoadSize)
        {
            return false;
        }
        return true;
    }
#endif
    

public:
#if TICK_STORAGE_AUTOSAVE_MODE
    unsigned int getPreloadTick() const
    {
        return metaData.tickEnd;
    }

    // Here we only save all data from tickStorage, which will save ~70-80% of syncing time since it's mostly networking (fetching tick data)
    // with scoreCache feature, nodes can get synced to the network in a few hours instead of days.
    // We can actually save all states (ie: etalonTick, minerScore, contract states...) of the node beside tickStorage and resume the node without any computation.
    // But that will need extra effort to maintain this feature when we add something new to the protocol.
    // And probably cause critical bugs if we forget to do update this feature.
    // 
    // Save procedure:
    // (1) check current meta data state
    // (2) write all missing chunks to disk
    // (3) update metadata state
    int trySaveToFile(unsigned int epoch, unsigned int tick, CHAR16* directory = NULL)
    {   
        if (tick <= tickBegin) {
            return 6;
        }
        unsigned long long nTick = tick - tickBegin + 1; // inclusive [tickBegin, tick]
        prepareFilenames(epoch);

        logToConsole(L"Saving tick data...");
        tickData.acquireLock();
        if (!saveTickData(nTick, directory))
        {
            tickData.releaseLock();
            logToConsole(L"Failed to save tickData");
            return 5;
        }
        tickData.releaseLock();

        logToConsole(L"Saving quorum ticks");
        for (int i = 0; i < NUMBER_OF_COMPUTORS; i++) ticks.acquireLock(i);
        if (!saveTicks(nTick, directory))
        {
            for (int i = 0; i < NUMBER_OF_COMPUTORS; i++) ticks.releaseLock(i);
            logToConsole(L"Failed to save Ticks");
            return 4;
        }
        for (int i = 0; i < NUMBER_OF_COMPUTORS; i++) ticks.releaseLock(i);


        tickTransactions.acquireLock();
        logToConsole(L"Saving tick transaction offset");
        if (!saveTickTransactionOffsets(nTick, directory))
        {
            tickTransactions.releaseLock();
            logToConsole(L"Failed to save transactionOffset");
            return 3;
        }
        logToConsole(L"Saving transactions");
        long long outTotalTransactionSize = 0;
        unsigned long long outNextTickTransactionOffset = 0;
        if (!saveTransactions(nTick, outTotalTransactionSize, outNextTickTransactionOffset, directory))
        {
            tickTransactions.releaseLock();
            logToConsole(L"Failed to save transactions");
            return 2;
        }
        tickTransactions.releaseLock();

        logToConsole(L"Saving meta data");
        if (!saveMetaData(epoch, tick, outTotalTransactionSize, outNextTickTransactionOffset, directory))
        {
            logToConsole(L"Failed to save metaData");
            return 1;
        }

        return 0;
    }

    // Load procedure:
    // (1) try to load metadata file
    // (2) sanity check meta data file
    // (3) load these in order: tickData -> Ticks -> tx offset -> tx 
    // only load once at start up
    int tryLoadFromFile(unsigned short epoch, CHAR16* directory)
    {
        prepareMetaDataFilename(epoch);

        logToConsole(L"Loading checkpoint meta data...");
        if (!loadMetaData(directory)) {
            logToConsole(L"Cannot load meta data file, Computor will not load tickStorage data from files");
            initMetaData(epoch);
            return 1;
        }
        if (!checkMetaData()) {
            logToConsole(L"Invalid meta data file for tick storage");
            initMetaData(epoch);
            return 2;
        }
        nextTickTransactionOffset = metaData.outNextTickTransactionOffset;
        unsigned long long nTick = metaData.tickEnd - metaData.tickBegin + 1;
        prepareFilenames(epoch);

        logToConsole(L"Loading tick data...");
        if (!loadTickData(nTick, directory))
        {
            logToConsole(L"Failed to load loadTickData");
            initMetaData(epoch);
            return 5;
        }

        logToConsole(L"Loading ticks...");
        if (!loadTicks(nTick, directory))
        {
            logToConsole(L"Failed to load loadTicks");
            initMetaData(epoch);
            return 4;
        }

        logToConsole(L"Loading transaction offset...");
        if (!loadTickTransactionOffsets(nTick, directory))
        {
            logToConsole(L"Failed to load loadTickTransactionOffsets");
            initMetaData(epoch);
            return 3;
        }

        logToConsole(L"Loading transactions...");
        if (!loadTransactions(nTick, metaData.outTotalTransactionSize, directory))
        {
            logToConsole(L"Failed to load loadTransactions");
            initMetaData(epoch);
            return 2;
        }
        return 0;
    }

    // Save a dummy metadata that invalidate the current snapshot
    bool saveInvalidateData(unsigned int epoch, CHAR16* directory = NULL)
    {
        MetaData invalidMetaData;
        invalidMetaData.epoch = 0;
        invalidMetaData.tickBegin = 0;
        invalidMetaData.tickEnd = 0;
        invalidMetaData.outTotalTransactionSize = 0;
        invalidMetaData.outNextTickTransactionOffset = 0;
        prepareMetaDataFilename(epoch);
        auto sz = saveLargeFile(SNAPSHOT_METADATA_FILE_NAME, sizeof(invalidMetaData), (unsigned char*)&invalidMetaData, directory);
        if (sz != sizeof(invalidMetaData))
        {
            return false;
        }
        return true;
    }


    bool initMetaData(short epoch)
    {
        metaData.tickBegin = tickBegin;
        metaData.tickEnd = tickBegin;
        metaData.epoch = epoch;
        lastCheckTransactionOffset = tickBegin;
        return true;
    }
#endif
    // Init at node startup
    static bool init()
    {
        // TODO: allocate everything with one continuous buffer
        if (!allocPoolWithErrorLog(L"tickDataPtr ", tickDataSize, (void**)&tickDataPtr, __LINE__)
            || !allocPoolWithErrorLog(L"tickPtr", ticksSize, (void**)&ticksPtr, __LINE__)
            || !allocPoolWithErrorLog(L"tickTransactionPtr", tickTransactionsSize, (void**)&tickTransactionsPtr, __LINE__)
            || !allocPoolWithErrorLog(L"tickTransactionOffset", tickTransactionOffsetsSize, (void**)&tickTransactionOffsetsPtr, __LINE__)
            || !allocPoolWithErrorLog(L"tickTransactionsDigestPtr", tickTransactionOffsetsLengthCurrentEpoch * sizeof(TransactionsDigestAccess::HashMapEntry), (void**)&tickTransactionsDigestPtr, __LINE__))
        {
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

        setMem((void*)tickTransactionsDigestPtr, tickTransactionOffsetsLengthCurrentEpoch * sizeof(TransactionsDigestAccess::HashMapEntry), 0);

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

        if (tickTransactionsDigestPtr)
        {
            freePool(tickTransactionsDigestPtr);
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
            // node startup with no data of prior epoch
            setMem(tickDataPtr, tickDataSize, 0);
            setMem(ticksPtr, ticksSize, 0);
            setMem(tickTransactionOffsetsPtr, tickTransactionOffsetsSize, 0);
            setMem(tickTransactionsPtr, tickTransactionsSize, 0);
            oldTickBegin = 0;
            oldTickEnd = 0;
        }
        // Transaction digest look up need to reset at the begining of epoch for pointing to valid current epoch transaction
        setMem((void*)tickTransactionsDigestPtr, tickTransactionOffsetsLengthCurrentEpoch * sizeof(TransactionsDigestAccess::HashMapEntry), 0);

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
            ASSERT(tickData.epoch == 0 || tickData.epoch == INVALIDATED_TICK_DATA || (tickData.tick == tickId));

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
            ASSERT(tickData.epoch == 0 || tickData.epoch == INVALIDATED_TICK_DATA || (tickData.tick == tickId));

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
            // td->epoch == 0: not yet received or temporarily disabled
            // td->epoch == INVALIDATED_TICK_DATA: invalidated by this node
            // in both cases, this data shouldn't be sent out
            if (td->epoch == 0 || td->epoch == INVALIDATED_TICK_DATA)
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

    // Struct for access the transaction using its digest. It contains the offset in tickTransactionsPtr
    struct TransactionsDigestAccess
    {
        inline static void acquireLock()
        {
            ACQUIRE(tickTransactionsDigestAccessLock);
        }

        inline static void releaseLock()
        {
            RELEASE(tickTransactionsDigestAccessLock);
        }

        struct HashMapEntry
        {
            m256i digest; // isZero mean not occupied
            const Transaction* transaction;
        };
        unsigned long long hashFunc(const m256i& digest)
        {
            return digest.m256i_u32[7] % tickTransactionOffsetsLengthCurrentEpoch;
        }

        void insertTransaction(const m256i& digest, const Transaction* transaction)
        {
            // Zero digest. No further process
            if (isZero(digest))
            {
                return;
            }

            HashMapEntry* pHashMap = (HashMapEntry*)tickTransactionsDigestPtr;
            unsigned long long index = hashFunc(digest);
            unsigned long long original_index = index;
            // TODO: check alraeady added tx ?
            while (!isZero(pHashMap[index].digest))
            {
                index = (index + 1) % tickTransactionOffsetsLengthCurrentEpoch;
                if (index == original_index)
                {
                    // Don't have enough place in the table
                    return;
                }
            }
            pHashMap[index].transaction = transaction;
            pHashMap[index].digest = digest;
        }

        const Transaction* findTransaction(const m256i& digest)
        {
            // Zero digest. No further process
            if (isZero(digest))
            {
                return NULL;
            }

            HashMapEntry* pHashMap = (HashMapEntry*)tickTransactionsDigestPtr;
            unsigned long long index = hashFunc(digest);
            unsigned long long original_index = index;
            while (!isZero(pHashMap[index].digest))
            {
                if (pHashMap[index].digest == digest)
                {
                    return pHashMap[index].transaction;
                }
                index = (index + 1) % tickTransactionOffsetsLengthCurrentEpoch;
                if (index == original_index)
                {
                    break;
                }
            }
            return NULL;
        }
    } transactionsDigestAccess;
};
