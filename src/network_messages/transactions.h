#pragma once

#include "common_def.h"

// used with Transaction
struct ContractIPOBid
{
    long long price;
    unsigned short quantity;
};

#define BROADCAST_TRANSACTION 24


// A transaction is made of this struct, followed by inputSize Bytes payload data and SIGNATURE_SIZE Bytes signature
struct Transaction
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    long long amount;
    unsigned int tick;
    unsigned short inputType;
    unsigned short inputSize;

    // Return total transaction data size with payload data and signature
    unsigned int totalSize() const
    {
        return sizeof(Transaction) + inputSize + SIGNATURE_SIZE;
    }

    // Check if transaction is valid
    bool checkValidity() const
    {
        return amount >= 0 && amount <= MAX_AMOUNT && inputSize <= MAX_INPUT_SIZE;
    }

    // Return pointer to transaction's payload (CAUTION: This is behind the memory reserved for this struct!)
    unsigned char* inputPtr()
    {
        return (((unsigned char*)this) + sizeof(Transaction));
    }

    // Return pointer to transaction's payload (CAUTION: This is behind the memory reserved for this struct!)
    const unsigned char* inputPtr() const
    {
        return (((const unsigned char*)this) + sizeof(Transaction));
    }

    // Return pointer to signature (CAUTION: This is behind the memory reserved for this struct!)
    unsigned char* signaturePtr()
    {
        return ((unsigned char*)this) + sizeof(Transaction) + inputSize;
    }
};

static_assert(sizeof(Transaction) == 32 + 32 + 8 + 4 + 2 + 2, "Something is wrong with the struct size.");

#define REQUEST_TICK_TRANSACTIONS 29

struct RequestedTickTransactions
{
    unsigned int tick;
    unsigned char transactionFlags[NUMBER_OF_TRANSACTIONS_PER_TICK / 8];
};

#define REQUEST_TRANSACTION_INFO 26
struct RequestedTransactionInfo
{
    m256i txDigest;
};

