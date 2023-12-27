#pragma once

#include "common_def.h"

// used with Transaction
struct ContractIPOBid
{
    long long price;
    unsigned short quantity;
};

#define BROADCAST_TRANSACTION 24

typedef struct
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    long long amount;
    unsigned int tick;
    unsigned short inputType;
    unsigned short inputSize;
} Transaction;

static_assert(sizeof(Transaction) == 32 + 32 + 8 + 4 + 2 + 2, "Something is wrong with the struct size.");

#define REQUEST_TICK_TRANSACTIONS 29

typedef struct
{
    unsigned int tick;
    unsigned char transactionFlags[NUMBER_OF_TRANSACTIONS_PER_TICK / 8];
} RequestedTickTransactions;

