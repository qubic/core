#pragma once

#include "assets/assets.h"

#include "network_core/peers.h"

static void processRequestIssuedAssets(Peer* peer, RequestResponseHeader* header)
{
    RespondIssuedAssets response;

    RequestIssuedAssets* request = header->getPayload<RequestIssuedAssets>();

    unsigned int universeIndex = request->publicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);

    ACQUIRE(universeLock);

iteration:
    if (universeIndex >= ASSETS_CAPACITY
        || assets[universeIndex].varStruct.issuance.type == EMPTY)
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
    else
    {
        if (assets[universeIndex].varStruct.issuance.type == ISSUANCE
            && assets[universeIndex].varStruct.issuance.publicKey == request->publicKey)
        {
            bs->CopyMem(&response.asset, &assets[universeIndex], sizeof(Asset));
            response.tick = system.tick;
            response.universeIndex = universeIndex;
            getSiblings<ASSETS_DEPTH>(response.universeIndex, assetDigests, response.siblings);

            enqueueResponse(peer, sizeof(response), RespondIssuedAssets::type, header->dejavu(), &response);
        }

        universeIndex = (universeIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }

    RELEASE(universeLock);
}

static void processRequestOwnedAssets(Peer* peer, RequestResponseHeader* header)
{
    RespondOwnedAssets response;

    RequestOwnedAssets* request = header->getPayload<RequestOwnedAssets>();

    unsigned int universeIndex = request->publicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);

    ACQUIRE(universeLock);

iteration:
    if (universeIndex >= ASSETS_CAPACITY
        || assets[universeIndex].varStruct.issuance.type == EMPTY)
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
    else
    {
        if (assets[universeIndex].varStruct.issuance.type == OWNERSHIP
            && assets[universeIndex].varStruct.issuance.publicKey == request->publicKey)
        {
            bs->CopyMem(&response.asset, &assets[universeIndex], sizeof(Asset));
            bs->CopyMem(&response.issuanceAsset, &assets[assets[universeIndex].varStruct.ownership.issuanceIndex], sizeof(Asset));
            response.tick = system.tick;
            response.universeIndex = universeIndex;
            getSiblings<ASSETS_DEPTH>(response.universeIndex, assetDigests, response.siblings);

            enqueueResponse(peer, sizeof(response), RespondOwnedAssets::type, header->dejavu(), &response);
        }

        universeIndex = (universeIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }

    RELEASE(universeLock);
}

static void processRequestPossessedAssets(Peer* peer, RequestResponseHeader* header)
{
    RespondPossessedAssets response;

    RequestPossessedAssets* request = header->getPayload<RequestPossessedAssets>();

    unsigned int universeIndex = request->publicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);

    ACQUIRE(universeLock);

iteration:
    if (universeIndex >= ASSETS_CAPACITY
        || assets[universeIndex].varStruct.issuance.type == EMPTY)
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
    else
    {
        if (assets[universeIndex].varStruct.issuance.type == POSSESSION
            && assets[universeIndex].varStruct.issuance.publicKey == request->publicKey)
        {
            bs->CopyMem(&response.asset, &assets[universeIndex], sizeof(Asset));
            bs->CopyMem(&response.ownershipAsset, &assets[assets[universeIndex].varStruct.possession.ownershipIndex], sizeof(Asset));
            bs->CopyMem(&response.issuanceAsset, &assets[assets[assets[universeIndex].varStruct.possession.ownershipIndex].varStruct.ownership.issuanceIndex], sizeof(Asset));
            response.tick = system.tick;
            response.universeIndex = universeIndex;
            getSiblings<ASSETS_DEPTH>(response.universeIndex, assetDigests, response.siblings);

            enqueueResponse(peer, sizeof(response), RespondPossessedAssets::type, header->dejavu(), &response);
        }

        universeIndex = (universeIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }

    RELEASE(universeLock);
}
