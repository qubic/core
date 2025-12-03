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
        enqueueResponse(peer, 0, EndResponse::type(), header->dejavu(), NULL);
    }
    else
    {
        if (assets[universeIndex].varStruct.issuance.type == ISSUANCE
            && assets[universeIndex].varStruct.issuance.publicKey == request->publicKey)
        {
            copyMem(&response.asset, &assets[universeIndex], sizeof(AssetRecord));
            response.tick = system.tick;
            response.universeIndex = universeIndex;
            getSiblings<ASSETS_DEPTH>(response.universeIndex, assetDigests, response.siblings);

            enqueueResponse(peer, sizeof(response), RespondIssuedAssets::type(), header->dejavu(), &response);
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
        enqueueResponse(peer, 0, EndResponse::type(), header->dejavu(), NULL);
    }
    else
    {
        if (assets[universeIndex].varStruct.issuance.type == OWNERSHIP
            && assets[universeIndex].varStruct.issuance.publicKey == request->publicKey)
        {
            copyMem(&response.asset, &assets[universeIndex], sizeof(AssetRecord));
            copyMem(&response.issuanceAsset, &assets[assets[universeIndex].varStruct.ownership.issuanceIndex], sizeof(AssetRecord));
            response.tick = system.tick;
            response.universeIndex = universeIndex;
            getSiblings<ASSETS_DEPTH>(response.universeIndex, assetDigests, response.siblings);

            enqueueResponse(peer, sizeof(response), RespondOwnedAssets::type(), header->dejavu(), &response);
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
        enqueueResponse(peer, 0, EndResponse::type(), header->dejavu(), NULL);
    }
    else
    {
        if (assets[universeIndex].varStruct.issuance.type == POSSESSION
            && assets[universeIndex].varStruct.issuance.publicKey == request->publicKey)
        {
            copyMem(&response.asset, &assets[universeIndex], sizeof(AssetRecord));
            copyMem(&response.ownershipAsset, &assets[assets[universeIndex].varStruct.possession.ownershipIndex], sizeof(AssetRecord));
            copyMem(&response.issuanceAsset, &assets[assets[assets[universeIndex].varStruct.possession.ownershipIndex].varStruct.ownership.issuanceIndex], sizeof(AssetRecord));
            response.tick = system.tick;
            response.universeIndex = universeIndex;
            getSiblings<ASSETS_DEPTH>(response.universeIndex, assetDigests, response.siblings);

            enqueueResponse(peer, sizeof(response), RespondPossessedAssets::type(), header->dejavu(), &response);
        }

        universeIndex = (universeIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }

    RELEASE(universeLock);
}

static void processRequestAssetsSendRecord(Peer* peer, RequestResponseHeader* responseHeader, unsigned int universeIndex)
{
    if (universeIndex >= ASSETS_CAPACITY)
        return;
    RespondAssetsWithSiblings* payload = responseHeader->getPayload<RespondAssetsWithSiblings>();
    copyMemory(payload->asset, assets[universeIndex]);
    payload->tick = system.tick;
    payload->universeIndex = universeIndex;
    if (!responseHeader->checkPayloadSize(sizeof(RequestAssets)))
    {
        getSiblings<ASSETS_DEPTH>(universeIndex, assetDigests, payload->siblings);
    }
    enqueueResponse(peer, responseHeader);
}

static void processRequestAssets(Peer* peer, RequestResponseHeader* header)
{
    // check size of recieved message (request by universe index may be smaller than sizeof(RequestAssets))
    if (!header->checkPayloadSizeMinMax(sizeof(RequestAssets::byUniverseIdx), sizeof(RequestAssets)))
        return;
    RequestAssets* request = header->getPayload<RequestAssets>();
    if (request->assetReqType != RequestAssets::requestByUniverseIdx && !header->checkPayloadSize(sizeof(RequestAssets)))
        return;

    // initalize output message (with siblings because the variant without siblings is just a subset)
    struct
    {
        RequestResponseHeader header;
        RespondAssetsWithSiblings payload;
    } response;
    setMemory(response, 0);
    response.header.setType(RespondAssets::type());
    response.header.setDejavu(header->dejavu());

    // size of output message depends on whether sibilings are requested
    if (request->byFilter.flags & RequestAssets::getSiblings)
        response.header.setSize<sizeof(RequestResponseHeader) + sizeof(RespondAssetsWithSiblings)>();
    else
        response.header.setSize<sizeof(RequestResponseHeader) + sizeof(RespondAssets)>();

    // find asset records and enqueue response messages (depending on request type)
    switch (request->assetReqType)
    {
    case RequestAssets::requestIssuanceRecords:
    {
        // setup issuance filter
        AssetIssuanceSelect issuanceFilter;
        if (request->byFilter.flags & RequestAssets::anyIssuer)
        {
            if (request->byFilter.flags & RequestAssets::anyAssetName)
                issuanceFilter = AssetIssuanceSelect::any();
            else
                issuanceFilter = AssetIssuanceSelect::byName(request->byFilter.assetName);
        }
        else
        {
            if (request->byFilter.flags & RequestAssets::anyAssetName)
                issuanceFilter = AssetIssuanceSelect::byIssuer(request->byFilter.issuer);
            else
                issuanceFilter = { request->byFilter.issuer, request->byFilter.assetName };
        }

        // iterate through assets records with filter
        ACQUIRE(universeLock);
        AssetIssuanceIterator iter(issuanceFilter);
        while (!iter.reachedEnd())
        {
            processRequestAssetsSendRecord(peer, &response.header, iter.issuanceIndex());
            iter.next();
        }
        RELEASE(universeLock);
    }
    break;

    case RequestAssets::requestOwnershipRecords:
    case RequestAssets::requestPossessionRecords:
    {
        Asset asset{request->byFilter.issuer, request->byFilter.assetName};

        // setup ownership filter
        AssetOwnershipSelect ownershipFilter;
        if (request->byFilter.flags & RequestAssets::anyOwner)
        {
            if (request->byFilter.flags & RequestAssets::anyOwnershipManagingContract)
                ownershipFilter = AssetOwnershipSelect::any();
            else
                ownershipFilter = AssetOwnershipSelect::byManagingContract(request->byFilter.ownershipManagingContract);
        }
        else
        {
            if (request->byFilter.flags & RequestAssets::anyOwnershipManagingContract)
                ownershipFilter = AssetOwnershipSelect::byOwner(request->byFilter.owner);
            else
                ownershipFilter = { request->byFilter.owner, request->byFilter.ownershipManagingContract };
        }

        if (request->assetReqType == RequestAssets::requestOwnershipRecords)
        {
            // iterate through asset ownership records with filter
            ACQUIRE(universeLock);
            AssetOwnershipIterator iter(asset, ownershipFilter);
            while (!iter.reachedEnd())
            {
                processRequestAssetsSendRecord(peer, &response.header, iter.ownershipIndex());
                iter.next();
            }
            RELEASE(universeLock);
        }
        else
        {
            // possession records are requested -> setup filter
            AssetPossessionSelect possessionFilter;
            if (request->byFilter.flags & RequestAssets::anyPossessor)
            {
                if (request->byFilter.flags & RequestAssets::anyPossessionManagingContract)
                    possessionFilter = AssetPossessionSelect::any();
                else
                    possessionFilter = AssetPossessionSelect::byManagingContract(request->byFilter.possessionManagingContract);
            }
            else
            {
                if (request->byFilter.flags & RequestAssets::anyPossessionManagingContract)
                    possessionFilter = AssetPossessionSelect::byPossessor(request->byFilter.possessor);
                else
                    possessionFilter = { request->byFilter.possessor, request->byFilter.possessionManagingContract };
            }

            // iterate through asset possession records with filter
            ACQUIRE(universeLock);
            AssetPossessionIterator iter(asset, ownershipFilter, possessionFilter);
            while (!iter.reachedEnd())
            {
                processRequestAssetsSendRecord(peer, &response.header, iter.possessionIndex());
                iter.next();
            }
            RELEASE(universeLock);
        }
    }
    break;

    case RequestAssets::requestByUniverseIdx:
    {
        ACQUIRE(universeLock);
        processRequestAssetsSendRecord(peer, &response.header, request->byUniverseIdx.universeIdx);
        RELEASE(universeLock);
    }
    break;
    }

    enqueueResponse(peer, 0, EndResponse::type(), header->dejavu(), nullptr);
}
