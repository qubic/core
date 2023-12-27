#pragma once

#include "common_def.h"

struct Entity
{
    m256i publicKey;
    long long incomingAmount, outgoingAmount;
    unsigned int numberOfIncomingTransfers, numberOfOutgoingTransfers;
    unsigned int latestIncomingTransferTick, latestOutgoingTransferTick;
};

static_assert(sizeof(::Entity) == 32 + 2 * 8 + 2 * 4 + 2 * 4, "Something is wrong with the struct size.");


#define REQUEST_ENTITY 31

typedef struct
{
    m256i publicKey;
} RequestedEntity;

static_assert(sizeof(RequestedEntity) == 32, "Something is wrong with the struct size.");


#define RESPOND_ENTITY 32

typedef struct
{
    ::Entity entity;
    unsigned int tick;
    int spectrumIndex;
    m256i siblings[SPECTRUM_DEPTH];
} RespondedEntity;

static_assert(sizeof(RespondedEntity) == sizeof(::Entity) + 4 + 4 + 32 * SPECTRUM_DEPTH, "Something is wrong with the struct size.");
