#pragma once

#include "platform/m256.h"
#include "platform/debugging.h"
#include "platform/memory.h"

#include "network_messages/header.h"

#include "public_settings.h"
#include "system.h"


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
static unsigned int tickTxCounter[MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH];    // store the amount of tx per tick
static unsigned int tickTxIndexStart[MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH]; // store the index position per tick
static volatile char confirmedTxLock = 0;
static unsigned int confirmedTxPreviousEpochBeginTick = 0;  // first tick kept from previous epoch (or 0 if no tick from prev. epoch available)
static unsigned int confirmedTxCurrentEpochBeginTick = 0;   // first tick of current epoch stored

#define REQUEST_TX_STATUS 201

// Use "#pragma pack" keep the binary struct compatibility after changing from unsigned char [32] to m256i
#pragma pack(push,1)
typedef struct
{
    unsigned int tick;
    m256i digest;
 //   char digest[32];
    unsigned char signature[64];
} RequestTxStatus;
#pragma pack(pop)

static_assert(sizeof(m256i) == 32, "");
static_assert(sizeof(RequestTxStatus) == 100, "unexpected size");

#define RESPOND_TX_STATUS 202

typedef struct
{
    unsigned int currentTickOfNode;
    unsigned int tickOfTx;
    unsigned char moneyFlew;
    unsigned char executed;
    unsigned char notfound;
    unsigned char _padding[5];
    m256i digest;
} RespondTxStatus;


// Allocate buffers
static bool initExchangeConnect()
{
    // Allocate pool to store confirmed TX's
    if (!allocatePool(confirmedTxLength * sizeof(ConfirmedTx), (void**)&confirmedTx))
        return false;
    confirmedTxPreviousEpochBeginTick = 0;
    confirmedTxCurrentEpochBeginTick = 0;
    return true;
}


// Free buffers
static void deinitExchangeConnect()
{
    if (confirmedTx)
        freePool(confirmedTx);
}


// Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
// are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
static void beginEpochExchangeConnect(unsigned int newInitialTick)
{
    unsigned int& tickBegin = confirmedTxCurrentEpochBeginTick;
    unsigned int& oldTickBegin = confirmedTxPreviousEpochBeginTick;

    bool keepTicks = tickBegin && newInitialTick > tickBegin && newInitialTick < tickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
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
            unsigned int newTxCount = txCount + tickTxCounter[tickIndex];
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
        copyMem(tickTxCounter + MAX_NUMBER_OF_TICKS_PER_EPOCH, tickTxCounter + tickIndex, tickCount * sizeof(tickTxCounter[0]));
        copyMem(confirmedTx + confirmedTxCurrentEpochLength, confirmedTx + tickTxIndexStart[tickIndex], txCount * sizeof(confirmedTx[0]));

        // copy adjusted tickTxIndexStart
        const unsigned int indexStartDelta = confirmedTxCurrentEpochLength - tickTxIndexStart[tickIndex];
        for (unsigned int tickOffset = 0; tickOffset < tickCount; ++tickOffset)
        {
            tickTxIndexStart[MAX_NUMBER_OF_TICKS_PER_EPOCH + tickOffset] = tickTxIndexStart[tickIndex + tickOffset] + indexStartDelta;
        }

        // init pool of current epoch with 0
        setMem(confirmedTx, confirmedTxCurrentEpochLength * sizeof(ConfirmedTx), 0);
        setMem(tickTxCounter, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(tickTxCounter[0]), 0);
        setMem(tickTxIndexStart, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(tickTxIndexStart[0]), 0);
    }
    else
    {
        // node startup with no data of prior epoch or no ticks to keep
        oldTickBegin = 0;

        // init pool with 0
        setMem(confirmedTx, confirmedTxLength * sizeof(ConfirmedTx), 0);
        setMem(tickTxCounter, sizeof(tickTxCounter), 0);
        setMem(tickTxIndexStart, sizeof(tickTxIndexStart), 0);
    }

    tickBegin = newInitialTick;
}


// adds a tx to the confirmed tx store
// txNumberMinusOne: the current tx number -1
// moneyFlew: if money has been flow
// tick: tick of tx (needs to be in range of current epoch)
// digest: digest of tx
static bool saveConfirmedTx(unsigned int txNumberMinusOne, unsigned char moneyFlew, unsigned int tick, const m256i& digest)
{
    ASSERT(confirmedTxCurrentEpochBeginTick == system.initialTick);
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
    tickTxCounter[tickIndex]++;

    RELEASE(confirmedTxLock);

    return true;
}


static void processRequestConfirmedTx(Peer *peer, RequestResponseHeader *header)
{
    ASSERT(confirmedTxCurrentEpochBeginTick == system.initialTick);
    ASSERT(system.tick >= system.initialTick);

    RequestTxStatus *request = header->getPayload<RequestTxStatus>();

    // only send a response if the node is in higher tick than the requested tx
    if (request->tick >= system.tick)
        return;

    int tickIndex;
    if (request->tick >= confirmedTxCurrentEpochBeginTick && request->tick < confirmedTxCurrentEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH)
    {
        // current epoch
        tickIndex = request->tick - confirmedTxCurrentEpochBeginTick;
    }
    else if (confirmedTxPreviousEpochBeginTick != 0 && request->tick >= confirmedTxPreviousEpochBeginTick && request->tick < confirmedTxCurrentEpochBeginTick)
    {
        // available tick of previous epoch (stored behind current epoch data)
        tickIndex = request->tick - confirmedTxPreviousEpochBeginTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }
    else
    {
        // tick not available
        return;
    }

    // get index where confirmedTx are starting to be stored in memory
    int index = tickTxIndexStart[tickIndex];

    RespondTxStatus currentTxStatus = {};
    currentTxStatus.currentTickOfNode = system.tick;
    currentTxStatus.tickOfTx = request->tick;
    currentTxStatus.executed = 0;
    currentTxStatus.moneyFlew = 0;
    currentTxStatus.notfound = 1;
    currentTxStatus.digest = request->digest;

    // loop over the number of tx which were stored for the given tick
    for (unsigned int i = 0; i < tickTxCounter[tickIndex]; i++)
    {
        ConfirmedTx& localConfirmedTx = confirmedTx[index + i];

        ASSERT(index + i < confirmedTxLength);
        ASSERT(localConfirmedTx.tick == request->tick);

        // if requested tx digest match stored digest tx has been found and is confirmed
        if (request->digest == localConfirmedTx.digest)
        {
            currentTxStatus.executed = 1;
            currentTxStatus.notfound = 0;
            currentTxStatus.moneyFlew = localConfirmedTx.moneyFlew;
            break;
        }
    }

    enqueueResponse(peer, sizeof(currentTxStatus), RESPOND_TX_STATUS, header->dejavu(), &currentTxStatus);
}
