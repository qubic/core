#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"

#include "network_messages/computors.h"

#include "four_q.h"
#include "private_settings.h"
#include "public_settings.h"

GLOBAL_VAR_DECL m256i operatorPublicKey;
// GLOBAL_VAR_DECL m256i computorSubseeds[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
// GLOBAL_VAR_DECL m256i computorPrivateKeys[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
// GLOBAL_VAR_DECL m256i computorPublicKeys[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
GLOBAL_VAR_DECL std::vector<m256i> computorSubseeds;
GLOBAL_VAR_DECL std::vector<m256i> computorPrivateKeys;
GLOBAL_VAR_DECL std::vector<m256i> computorPublicKeys;
GLOBAL_VAR_DECL m256i arbitratorPublicKey;
GLOBAL_VAR_DECL m256i dispatcherPublicKey;

GLOBAL_VAR_DECL BroadcastComputors broadcastedComputors;


static bool initSpecialEntities()
{
    getPublicKeyFromIdentity((const unsigned char*)OPERATOR.c_str(), operatorPublicKey.m256i_u8);
    if (isZero(operatorPublicKey))
    {
        operatorPublicKey.setRandomValue();
    }

    computorSubseeds.resize(computorSeeds.size());
    computorPrivateKeys.resize(computorSeeds.size());
    computorPublicKeys.resize(computorSeeds.size());
    for (unsigned int i = 0; i < computorSeeds.size(); i++)
    {
        if (!getSubseed(reinterpret_cast<const unsigned char *>(computorSeeds[i].c_str()), computorSubseeds[i].m256i_u8))
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
    // setMem(computorSeeds, sizeof(computorSeeds), 0);
    // setMem(computorSubseeds, sizeof(computorSubseeds), 0);
    // setMem(computorPrivateKeys, sizeof(computorPrivateKeys), 0);
    // setMem(computorPublicKeys, sizeof(computorPublicKeys), 0);
    std::string _55ZeroChar = "";
    for (int i = 0; i < 55; ++i) {
        _55ZeroChar += static_cast<char>(0);
    }

    for (std::string &e : computorSeeds) {
        e = _55ZeroChar;
    }
    for (auto &e : computorSubseeds) {
        e = m256i::zero();
    }
    for (auto &e : computorPrivateKeys) {
        e = m256i::zero();
    }
    for (auto &e : computorPublicKeys)
    {
        e = m256i::zero();
    }
}
