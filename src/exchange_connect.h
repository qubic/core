#pragma once

typedef struct
{
    unsigned int tick;
    unsigned char moneyFlew;
    unsigned char _padding[3];
    m256i digest;
} ConfirmedTx;

// Memory to store confiremd TX's
static ConfirmedTx *confirmedTx = NULL;
static int tickTxCounter[MAX_NUMBER_OF_TICKS_PER_EPOCH];     // store the amount of tx per tick
static long tickTxIndexStart[MAX_NUMBER_OF_TICKS_PER_EPOCH]; // store the index position per tick
static volatile char confirmedTxLock = 0;

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

static bool initExchangeConnect()
{
    // Allocate pool to store confirmed TX's
    if (bs->AllocatePool(EfiRuntimeServicesData, FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK / TRANSACTION_SPARSENESS) * sizeof(ConfirmedTx), (void**)&confirmedTx) != 0)
        return false;
    // Init pool with 0
    bs->SetMem(confirmedTx, FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK / TRANSACTION_SPARSENESS) * sizeof(ConfirmedTx), 0);
    return true;
}

// adds a tx to the confirmed tx store
// txNumberMinusOne: the current tx number -1
// moneyFlew: if money has been flow
// tick: tick in which this tx was
// digest: digest of tx
static void saveConfirmedTx(unsigned int txNumberMinusOne, unsigned char moneyFlew, unsigned int tick, m256i digest)
{
    ACQUIRE(confirmedTxLock);
    int tickNumber = tick - system.initialTick; // get current tick number in epoch
    ConfirmedTx & txConfirmation = confirmedTx[txNumberMinusOne];
    txConfirmation.tick = tick;
    txConfirmation.moneyFlew = moneyFlew;
    txConfirmation.digest = digest;
    // keep track of tx number in tick to find it later easier
    tickTxCounter[tickNumber]++;
    RELEASE(confirmedTxLock);
}

static void processRequestConfirmedTx(Peer *peer, RequestResponseHeader *header)
{

    RequestTxStatus *request = header->getPayload<RequestTxStatus>();

    RespondTxStatus currentTxStatus = {};

    int tickNumber = request->tick - system.initialTick;
    // get index where confirmedtx are starting to be stored in memory
    int index = tickTxIndexStart[tickNumber];

    // only send a response if the node is in higher tick than the requested tx
    if (request->tick < system.tick)
    {
        currentTxStatus.currentTickOfNode = system.tick;
        currentTxStatus.tickOfTx = request->tick;
        currentTxStatus.executed = 0;
        currentTxStatus.moneyFlew = 0;
        currentTxStatus.notfound = 1;
        currentTxStatus.digest = request->digest;

        // loop over the number of tx which were stored for the given tick
        for (int i = 0; i < tickTxCounter[tickNumber]; i++)
        {
            ConfirmedTx *localConfirmedTx = &confirmedTx[index + i];

            // if requested tx digest match stored digest tx has been found and is confirmed
            if (request->digest == localConfirmedTx->digest)
            {
                currentTxStatus.executed = 1;
                currentTxStatus.notfound = 0;
                currentTxStatus.moneyFlew = localConfirmedTx->moneyFlew;
                break;
            }
        }

        enqueueResponse(peer, sizeof(currentTxStatus), RESPOND_TX_STATUS, header->dejavu(), &currentTxStatus);
    }
}