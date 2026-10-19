#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"

#include "network_messages/computors.h"

#include "four_q.h"
#include "private_settings.h"
#include "public_settings.h"
#include "frozen_addresses.h"

static constexpr unsigned long long computorSeedsCount = sizeof(computorSeeds) / sizeof(computorSeeds[0]);

GLOBAL_VAR_DECL m256i operatorPublicKey;
GLOBAL_VAR_DECL m256i computorSubseeds[computorSeedsCount];
GLOBAL_VAR_DECL m256i computorPrivateKeys[computorSeedsCount];
GLOBAL_VAR_DECL m256i computorPublicKeys[computorSeedsCount];
GLOBAL_VAR_DECL m256i arbitratorPublicKey;
GLOBAL_VAR_DECL m256i dispatcherPublicKey;
GLOBAL_VAR_DECL m256i frozenEscrowPublicKey;

GLOBAL_VAR_DECL BroadcastComputors broadcastedComputors;


static constexpr unsigned long long NUMBER_OF_FROZEN_ADDRESSES = sizeof(gFrozenIdentities) / sizeof(gFrozenIdentities[0]);

static constexpr unsigned long long frozenCapacity(unsigned long long count)
{
    unsigned long long capacity = 1;
    while (capacity < count * 2 + 1)
    {
        capacity <<= 1;
    }
    return capacity;
}
static constexpr unsigned long long FROZEN_ADDRESSES_CAPACITY = frozenCapacity(NUMBER_OF_FROZEN_ADDRESSES);
static_assert((FROZEN_ADDRESSES_CAPACITY & (FROZEN_ADDRESSES_CAPACITY - 1)) == 0,
    "FROZEN_ADDRESSES_CAPACITY must be a power of two");

GLOBAL_VAR_DECL m256i gFrozenAddresses[FROZEN_ADDRESSES_CAPACITY];

static bool initSpecialEntities()
{
    getPublicKeyFromIdentity((const unsigned char*)OPERATOR, operatorPublicKey.m256i_u8);
    if (isZero(operatorPublicKey))
    {
        operatorPublicKey.setRandomValue();
    }

    for (unsigned int i = 0; i < computorSeedsCount; i++)
    {
        if (!getSubseed(computorSeeds[i], computorSubseeds[i].m256i_u8))
        {
            return false;
        }
        getPrivateKey(computorSubseeds[i].m256i_u8, computorPrivateKeys[i].m256i_u8);
        getPublicKey(computorPrivateKeys[i].m256i_u8, computorPublicKeys[i].m256i_u8);
    }

    getPublicKeyFromIdentity((const unsigned char*)ARBITRATOR, (unsigned char*)&arbitratorPublicKey);
    getPublicKeyFromIdentity((const unsigned char*)DISPATCHER, dispatcherPublicKey.m256i_u8);
    getPublicKeyFromIdentity((const unsigned char*)FROZEN_ESCROW, frozenEscrowPublicKey.m256i_u8);

    setMem(gFrozenAddresses, sizeof(gFrozenAddresses), 0);
    for (unsigned long long i = 0; i < NUMBER_OF_FROZEN_ADDRESSES; i++)
    {
        m256i publicKey;
        getPublicKeyFromIdentity(gFrozenIdentities[i], publicKey.m256i_u8);
        unsigned int index = publicKey.m256i_u32[0] & (FROZEN_ADDRESSES_CAPACITY - 1);
        while (!isZero(gFrozenAddresses[index]))
        {
            index = (index + 1) & (FROZEN_ADDRESSES_CAPACITY - 1);
        }
        gFrozenAddresses[index] = publicKey;
    }

    setMem(&broadcastedComputors, sizeof(broadcastedComputors), 0);

    return true;
}

static bool isFrozenAddress(const m256i& publicKey)
{
    if (isZero(publicKey))
    {
        return false;
    }
    unsigned int index = publicKey.m256i_u32[0] & (FROZEN_ADDRESSES_CAPACITY - 1);
    while (!isZero(gFrozenAddresses[index]))
    {
        if (gFrozenAddresses[index] == publicKey)
        {
            return true;
        }
        index = (index + 1) & (FROZEN_ADDRESSES_CAPACITY - 1);
    }
    return false;
}

static void deinitSpecialEntities()
{
    setMem(computorSeeds, sizeof(computorSeeds), 0);
    setMem(computorSubseeds, sizeof(computorSubseeds), 0);
    setMem(computorPrivateKeys, sizeof(computorPrivateKeys), 0);
    setMem(computorPublicKeys, sizeof(computorPublicKeys), 0);
}

static int computorIndex(const m256i& computor)
{
    for (int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        if (broadcastedComputors.computors.publicKeys[computorIndex] == computor)
        {
            return computorIndex;
        }
    }

    return -1;
}
