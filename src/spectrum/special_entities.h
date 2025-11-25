#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"

#include "network_messages/computors.h"

#include "four_q.h"
#include "private_settings.h"
#include "public_settings.h"

static constexpr unsigned long long computerSeedsCount = sizeof(computorSeeds) / sizeof(computorSeeds[0]);

GLOBAL_VAR_DECL m256i operatorPublicKey;
GLOBAL_VAR_DECL m256i computorSubseeds[computerSeedsCount];
GLOBAL_VAR_DECL m256i computorPrivateKeys[computerSeedsCount];
GLOBAL_VAR_DECL m256i computorPublicKeys[computerSeedsCount];
GLOBAL_VAR_DECL m256i arbitratorPublicKey;
GLOBAL_VAR_DECL m256i dispatcherPublicKey;

GLOBAL_VAR_DECL BroadcastComputors broadcastedComputors;


static bool initSpecialEntities()
{
    getPublicKeyFromIdentity((const unsigned char*)OPERATOR, operatorPublicKey.m256i_u8);
    if (isZero(operatorPublicKey))
    {
        operatorPublicKey.setRandomValue();
    }

    for (unsigned int i = 0; i < computerSeedsCount; i++)
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

    setMem(&broadcastedComputors, sizeof(broadcastedComputors), 0);

    return true;
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
