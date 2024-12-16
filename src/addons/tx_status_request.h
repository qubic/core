// This is an extension by qli allowing to query transaction status with RequestTxStatus message

#pragma once

#if ADDON_TX_STATUS_REQUEST

#include "../platform/m256.h"
#include "../platform/debugging.h"
#include "../platform/memory_util.h"

#include "../network_messages/header.h"

#include "../public_settings.h"
#include "../system.h"


typedef struct
{
    unsigned int tick;
    unsigned char moneyFlew;
    unsigned char _padding[3];
    m256i digest;
} ConfirmedTx;

constexpr unsigned long long confirmedTxCurrentEpochLength = (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK / TRANSACTION_SPARSENESS);
constexpr unsigned long long confirmedTxPreviousEpochLength = (((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK / TRANSACTION_SPARSENESS);
constexpr unsigned long long confirmedTxLength = confirmedTxCurrentEpochLength + confirmedTxPreviousEpochLength;

// Memory to store confirmed TX's (arrays store data of current epoch, followed by last ticks of previous epoch)
static ConfirmedTx *confirmedTx = NULL;
static volatile char confirmedTxLock = 0;

static struct
{
    unsigned int tickTxCounter[MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH];    // store the amount of tx per tick
    unsigned int tickTxIndexStart[MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH]; // store the index position per tick
    unsigned int confirmedTxPreviousEpochBeginTick;  // first tick kept from previous epoch (or 0 if no tick from prev. epoch available)
    unsigned int confirmedTxCurrentEpochBeginTick;   // first tick of current epoch stored
} txStatusData;


#define REQUEST_TX_STATUS 201

struct RequestTxStatus
{
    unsigned int tick;
};

static_assert(sizeof(RequestTxStatus) == 4, "unexpected size");

#define RESPOND_TX_STATUS 202

#pragma pack(push, 1)
struct RespondTxStatus
{
    unsigned int currentTickOfNode;
    unsigned int tick;
    unsigned int txCount;
    unsigned char moneyFlew[(NUMBER_OF_TRANSACTIONS_PER_TICK + 7) / 8];

    // only txCount digests are sent with this message, so only read the first txCount digests when using this as a view to the received data
    m256i txDigests[NUMBER_OF_TRANSACTIONS_PER_TICK];

    // return size of this struct to be sent (last txDigests are 0 and do not need to be sent)
    unsigned int size() const
    {
        return offsetof(RespondTxStatus, txDigests) + txCount * sizeof(m256i);
    }
};
#pragma pack(pop)
static RespondTxStatus* tickTxStatusStorage = NULL;

// Allocate buffers
static bool initTxStatusRequestAddOn()
{
    // Allocate pool to store confirmed TX's
    if (!allocPoolWithErrorLog(L"confirmedTx", confirmedTxLength * sizeof(ConfirmedTx), (void**)&confirmedTx, __LINE__))
        return false;
    // allocate tickTxStatus responses storage
    if (!allocPoolWithErrorLog(L"tickTxStatusStorage", MAX_NUMBER_OF_PROCESSORS * sizeof(RespondTxStatus), (void**)&tickTxStatusStorage, __LINE__))
        return false;
    txStatusData.confirmedTxPreviousEpochBeginTick = 0;
    txStatusData.confirmedTxCurrentEpochBeginTick = 0;
    return true;
}


// Free buffers
static void deinitTxStatusRequestAddOn()
{
    if (confirmedTx)
        freePool(confirmedTx);
}


// Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
// are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
static void beginEpochTxStatusRequestAddOn(unsigned int newInitialTick)
{
    unsigned int& tickBegin = txStatusData.confirmedTxCurrentEpochBeginTick;
    unsigned int& oldTickBegin = txStatusData.confirmedTxPreviousEpochBeginTick;

    bool keepTicks = tickBegin && newInitialTick > tickBegin && newInitialTick <= tickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    if (keepTicks)
    {
        // Seamless epoch transition: keep some ticks of prior epoch
        // The number of ticks to keep is limited by:
        // - the length of the previous epoch tick buffer (tickCount < TICKS_TO_KEEP_FROM_PRIOR_EPOCH)
        // - the length of the previous epoch confirmedTx array (confirmedTxPreviousEpochLength)
        // - the number of ticks available from in ended epoch (tickIndex >= 0)
        unsigned int tickCount = 0;
        unsigned int txCount = 0;
        for (int tickIndex = (newInitialTick - 1) - tickBegin; tickIndex >= 0; --tickIndex)
        {
            unsigned int newTxCount = txCount + txStatusData.tickTxCounter[tickIndex];
            if (newTxCount <= confirmedTxPreviousEpochLength && tickCount < TICKS_TO_KEEP_FROM_PRIOR_EPOCH)
            {
                txCount = newTxCount;
                ++tickCount;
            }
            else
            {
                break;
            }
        }

        oldTickBegin = newInitialTick - tickCount;
        const unsigned int oldTickEnd = newInitialTick;
        const unsigned int tickIndex = oldTickBegin - tickBegin;

        // copy confirmedTx and tickTxCounter from recently ended epoch into storage of previous epoch
        copyMem(txStatusData.tickTxCounter + MAX_NUMBER_OF_TICKS_PER_EPOCH, txStatusData.tickTxCounter + tickIndex, tickCount * sizeof(txStatusData.tickTxCounter[0]));
        copyMem(confirmedTx + confirmedTxCurrentEpochLength, confirmedTx + txStatusData.tickTxIndexStart[tickIndex], txCount * sizeof(confirmedTx[0]));

        // copy adjusted tickTxIndexStart
        const unsigned int indexStartDelta = confirmedTxCurrentEpochLength - txStatusData.tickTxIndexStart[tickIndex];
        for (unsigned int tickOffset = 0; tickOffset < tickCount; ++tickOffset)
        {
            txStatusData.tickTxIndexStart[MAX_NUMBER_OF_TICKS_PER_EPOCH + tickOffset] = txStatusData.tickTxIndexStart[tickIndex + tickOffset] + indexStartDelta;
        }

        // init pool of current epoch with 0
        setMem(confirmedTx, confirmedTxCurrentEpochLength * sizeof(ConfirmedTx), 0);
        setMem(txStatusData.tickTxCounter, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(txStatusData.tickTxCounter[0]), 0);
        setMem(txStatusData.tickTxIndexStart, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(txStatusData.tickTxIndexStart[0]), 0);
    }
    else
    {
        // node startup with no data of prior epoch or no ticks to keep
        oldTickBegin = 0;

        // init pool with 0
        setMem(confirmedTx, confirmedTxLength * sizeof(ConfirmedTx), 0);
        setMem(txStatusData.tickTxCounter, sizeof(txStatusData.tickTxCounter), 0);
        setMem(txStatusData.tickTxIndexStart, sizeof(txStatusData.tickTxIndexStart), 0);
    }

    tickBegin = newInitialTick;
}


// adds a tx to the confirmed tx store
// txNumberMinusOne: the current tx number -1
// moneyFlew: if money has been flow
// tick: tick of tx (needs to be in range of current epoch)
// digest: digest of tx
static bool saveConfirmedTx(unsigned int txNumberMinusOne, bool moneyFlew, unsigned int tick, const m256i& digest)
{
    ASSERT(txStatusData.confirmedTxCurrentEpochBeginTick == system.initialTick);
    ASSERT(tick >= system.initialTick && tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH);

    // skip if confirmedTx storage is full
    if (txNumberMinusOne >= confirmedTxCurrentEpochLength)
        return false;

    ACQUIRE(confirmedTxLock);

    // set confirmedTx data
    ConfirmedTx & txConfirmation = confirmedTx[txNumberMinusOne];
    txConfirmation.tick = tick;
    txConfirmation.moneyFlew = moneyFlew;
    txConfirmation.digest = digest;

    // get current tick number in epoch
    int tickIndex = tick - system.initialTick;
    // keep track of tx number in tick to find it later easier
    txStatusData.tickTxCounter[tickIndex]++;

    RELEASE(confirmedTxLock);

    return true;
}


static void processRequestConfirmedTx(long long processorNumber, Peer *peer, RequestResponseHeader *header)
{
    ASSERT(txStatusData.confirmedTxCurrentEpochBeginTick == system.initialTick);
    ASSERT(system.tick >= system.initialTick);

    RequestTxStatus *request = header->getPayload<RequestTxStatus>();

    // only send a response if the node is in higher tick than the requested tx
    if (request->tick >= system.tick)
        return;

    int tickIndex;
    if (request->tick >= txStatusData.confirmedTxCurrentEpochBeginTick && request->tick < txStatusData.confirmedTxCurrentEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH)
    {
        // current epoch
        tickIndex = request->tick - txStatusData.confirmedTxCurrentEpochBeginTick;
    }
    else if (txStatusData.confirmedTxPreviousEpochBeginTick != 0 && request->tick >= txStatusData.confirmedTxPreviousEpochBeginTick && request->tick < txStatusData.confirmedTxCurrentEpochBeginTick)
    {
        // available tick of previous epoch (stored behind current epoch data)
        tickIndex = request->tick - txStatusData.confirmedTxPreviousEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }
    else
    {
        // tick not available
        return;
    }

    // get index where confirmedTx are starting to be stored in memory
    int index = txStatusData.tickTxIndexStart[tickIndex];

    // init response message data, get it from the storage to avoid increasing stack mem
    RespondTxStatus& tickTxStatus = tickTxStatusStorage[processorNumber];
    tickTxStatus.currentTickOfNode = system.tick;
    tickTxStatus.tick = request->tick;
    tickTxStatus.txCount = txStatusData.tickTxCounter[tickIndex];
    setMem(&tickTxStatus.moneyFlew, sizeof(tickTxStatus.moneyFlew), 0);

    // loop over the number of tx which were stored for the given tick
    ASSERT(tickTxStatus.txCount <= NUMBER_OF_TRANSACTIONS_PER_TICK);
    for (unsigned int i = 0; i < tickTxStatus.txCount; i++)
    {
        ConfirmedTx& localConfirmedTx = confirmedTx[index + i];

        ASSERT(index + i < confirmedTxLength);
        ASSERT(localConfirmedTx.tick == request->tick);
        ASSERT(localConfirmedTx.moneyFlew == 1 || localConfirmedTx.moneyFlew == 0);

        tickTxStatus.txDigests[i] = localConfirmedTx.digest;
        tickTxStatus.moneyFlew[i >> 3] |= (localConfirmedTx.moneyFlew << (i & 7));
    }

    ASSERT(tickTxStatus.size() <= sizeof(tickTxStatus));
    enqueueResponse(peer, tickTxStatus.size(), RESPOND_TX_STATUS, header->dejavu(), &tickTxStatus);
}

#if TICK_STORAGE_AUTOSAVE_MODE
// can only be called from main thread
static bool saveStateTxStatus(const unsigned int numberOfTransactions, CHAR16* directory)
{
    static unsigned short TX_STATUS_SNAPSHOT_FILE_NAME[] = L"snapshotTxStatusData";
    long long savedSize = save(TX_STATUS_SNAPSHOT_FILE_NAME, sizeof(txStatusData), (unsigned char*)&txStatusData, directory);
    if (savedSize != sizeof(txStatusData))
    {
        logToConsole(L"Failed to save txStatusData");
        return false;
    }

    static unsigned short CONFIRMED_TX_SNAPSHOT_FILE_NAME[] = L"snapshotConfirmedTx";
    savedSize = saveLargeFile(CONFIRMED_TX_SNAPSHOT_FILE_NAME, numberOfTransactions*sizeof(ConfirmedTx), (unsigned char*)confirmedTx, directory);
    if (savedSize != numberOfTransactions * sizeof(ConfirmedTx))
    {
        logToConsole(L"Failed to save ConfirmedTx");
        return false;
    }
    return true;
}

// can only be called from main thread
// numberOfTransactions must be known before calling this
static bool loadStateTxStatus(const unsigned int numberOfTransactions, CHAR16* directory)
{
    static unsigned short TX_STATUS_SNAPSHOT_FILE_NAME[] = L"snapshotTxStatusData";
    long long loadedSize = load(TX_STATUS_SNAPSHOT_FILE_NAME, sizeof(txStatusData), (unsigned char*)&txStatusData, directory);
    if (loadedSize != sizeof(txStatusData))
    {
        logToConsole(L"Failed to load txStatusData");
        return false;
    }

    if (numberOfTransactions)
    {
        static unsigned short CONFIRMED_TX_SNAPSHOT_FILE_NAME[] = L"snapshotConfirmedTx";
        loadedSize = loadLargeFile(CONFIRMED_TX_SNAPSHOT_FILE_NAME, numberOfTransactions * sizeof(ConfirmedTx), (unsigned char*)confirmedTx, directory);
        if (loadedSize != numberOfTransactions * sizeof(ConfirmedTx))
        {
            logToConsole(L"Failed to load ConfirmedTx");
            return false;
        }
    }
    return true;
}
#endif // TICK_STORAGE_AUTOSAVE_MODE
#endif // ADDON_TX_STATUS_REQUEST