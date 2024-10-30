#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/uefi.h"
#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"

#include "network_messages/assets.h"

#include "public_settings.h"
#include "logging/logging.h"
#include "kangaroo_twelve.h"
#include "four_q.h"
#include "common_buffers.h"



GLOBAL_VAR_DECL volatile char universeLock GLOBAL_VAR_INIT(0);
GLOBAL_VAR_DECL Asset* assets GLOBAL_VAR_INIT(nullptr);
GLOBAL_VAR_DECL m256i* assetDigests GLOBAL_VAR_INIT(nullptr);
static constexpr unsigned long long assetDigestsSizeInBytes = (ASSETS_CAPACITY * 2 - 1) * 32ULL;
GLOBAL_VAR_DECL unsigned long long* assetChangeFlags GLOBAL_VAR_INIT(nullptr);
static constexpr char CONTRACT_ASSET_UNIT_OF_MEASUREMENT[7] = { 0, 0, 0, 0, 0, 0, 0 };

static bool initAssets()
{
    if (!allocatePool(ASSETS_CAPACITY * sizeof(Asset), (void**)&assets)
        || !allocatePool(assetDigestsSizeInBytes, (void**)&assetDigests)
        || !allocatePool(ASSETS_CAPACITY / 8, (void**)&assetChangeFlags))
    {
        logToConsole(L"Failed to allocate asset buffers!");
        return false;
    }
    setMem(assetChangeFlags, ASSETS_CAPACITY / 8, 0xFF);
    return true;
}

static void deinitAssets()
{
    if (assetChangeFlags)
    {
        freePool(assetChangeFlags);
        assetChangeFlags = nullptr;
    }
    if (assetDigests)
    {
        freePool(assetDigests);
        assetDigests = nullptr;
    }
    if (assets)
    {
        freePool(assets);
        assets = nullptr;
    }
}

static long long issueAsset(const m256i& issuerPublicKey, const char name[7], char numberOfDecimalPlaces, const char unitOfMeasurement[7], long long numberOfShares, unsigned short managingContractIndex,
    int* issuanceIndex, int* ownershipIndex, int* possessionIndex)
{
    *issuanceIndex = issuerPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);

    ACQUIRE(universeLock);

iteration:
    if (assets[*issuanceIndex].varStruct.issuance.type == EMPTY)
    {
        assets[*issuanceIndex].varStruct.issuance.publicKey = issuerPublicKey;
        assets[*issuanceIndex].varStruct.issuance.type = ISSUANCE;
        copyMem(assets[*issuanceIndex].varStruct.issuance.name, name, sizeof(assets[*issuanceIndex].varStruct.issuance.name));
        assets[*issuanceIndex].varStruct.issuance.numberOfDecimalPlaces = numberOfDecimalPlaces;
        copyMem(assets[*issuanceIndex].varStruct.issuance.unitOfMeasurement, unitOfMeasurement, sizeof(assets[*issuanceIndex].varStruct.issuance.unitOfMeasurement));

        *ownershipIndex = (*issuanceIndex + 1) & (ASSETS_CAPACITY - 1);
    iteration2:
        if (assets[*ownershipIndex].varStruct.ownership.type == EMPTY)
        {
            assets[*ownershipIndex].varStruct.ownership.publicKey = issuerPublicKey;
            assets[*ownershipIndex].varStruct.ownership.type = OWNERSHIP;
            assets[*ownershipIndex].varStruct.ownership.managingContractIndex = managingContractIndex;
            assets[*ownershipIndex].varStruct.ownership.issuanceIndex = *issuanceIndex;
            assets[*ownershipIndex].varStruct.ownership.numberOfShares = numberOfShares;

            *possessionIndex = (*ownershipIndex + 1) & (ASSETS_CAPACITY - 1);
        iteration3:
            if (assets[*possessionIndex].varStruct.possession.type == EMPTY)
            {
                assets[*possessionIndex].varStruct.possession.publicKey = issuerPublicKey;
                assets[*possessionIndex].varStruct.possession.type = POSSESSION;
                assets[*possessionIndex].varStruct.possession.managingContractIndex = managingContractIndex;
                assets[*possessionIndex].varStruct.possession.ownershipIndex = *ownershipIndex;
                assets[*possessionIndex].varStruct.possession.numberOfShares = numberOfShares;

                assetChangeFlags[*issuanceIndex >> 6] |= (1ULL << (*issuanceIndex & 63));
                assetChangeFlags[*ownershipIndex >> 6] |= (1ULL << (*ownershipIndex & 63));
                assetChangeFlags[*possessionIndex >> 6] |= (1ULL << (*possessionIndex & 63));

                RELEASE(universeLock);

                AssetIssuance assetIssuance;
                assetIssuance.issuerPublicKey = issuerPublicKey;
                assetIssuance.numberOfShares = numberOfShares;
                *((unsigned long long*) assetIssuance.name) = *((unsigned long long*) name); // Order must be preserved!
                assetIssuance.numberOfDecimalPlaces = numberOfDecimalPlaces; // Order must be preserved!
                *((unsigned long long*) assetIssuance.unitOfMeasurement) = *((unsigned long long*) unitOfMeasurement); // Order must be preserved!
                logger.logAssetIssuance(assetIssuance);

                return numberOfShares;
            }
            else
            {
                *possessionIndex = (*possessionIndex + 1) & (ASSETS_CAPACITY - 1);

                goto iteration3;
            }
        }
        else
        {
            *ownershipIndex = (*ownershipIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration2;
        }
    }
    else
    {
        if (assets[*issuanceIndex].varStruct.issuance.type == ISSUANCE
            && ((*((unsigned long long*)assets[*issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == ((*((unsigned long long*)name)) & 0xFFFFFFFFFFFFFF)
            && assets[*issuanceIndex].varStruct.issuance.publicKey == issuerPublicKey)
        {
            RELEASE(universeLock);
            return 0;
        }

        *issuanceIndex = (*issuanceIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }
}

static bool transferShareOwnershipAndPossession(int sourceOwnershipIndex, int sourcePossessionIndex, const m256i& destinationPublicKey, long long numberOfShares,
    int* destinationOwnershipIndex, int* destinationPossessionIndex,
    bool lock)
{
    if (numberOfShares <= 0)
    {
        return false;
    }

    if (lock)
    {
        ACQUIRE(universeLock);
    }

    if (assets[sourceOwnershipIndex].varStruct.ownership.type != OWNERSHIP || assets[sourceOwnershipIndex].varStruct.ownership.numberOfShares < numberOfShares
        || assets[sourcePossessionIndex].varStruct.possession.type != POSSESSION || assets[sourcePossessionIndex].varStruct.possession.numberOfShares < numberOfShares
        || assets[sourcePossessionIndex].varStruct.possession.ownershipIndex != sourceOwnershipIndex)
    {
        if (lock)
        {
            RELEASE(universeLock);
        }

        return false;
    }

    *destinationOwnershipIndex = destinationPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
iteration:
    if (assets[*destinationOwnershipIndex].varStruct.ownership.type == EMPTY
        || (assets[*destinationOwnershipIndex].varStruct.ownership.type == OWNERSHIP
            && assets[*destinationOwnershipIndex].varStruct.ownership.managingContractIndex == assets[sourceOwnershipIndex].varStruct.ownership.managingContractIndex
            && assets[*destinationOwnershipIndex].varStruct.ownership.issuanceIndex == assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex
            && assets[*destinationOwnershipIndex].varStruct.ownership.publicKey == destinationPublicKey))
    {
        assets[sourceOwnershipIndex].varStruct.ownership.numberOfShares -= numberOfShares;

        if (assets[*destinationOwnershipIndex].varStruct.ownership.type == EMPTY)
        {
            assets[*destinationOwnershipIndex].varStruct.ownership.publicKey = destinationPublicKey;
            assets[*destinationOwnershipIndex].varStruct.ownership.type = OWNERSHIP;
            assets[*destinationOwnershipIndex].varStruct.ownership.managingContractIndex = assets[sourceOwnershipIndex].varStruct.ownership.managingContractIndex;
            assets[*destinationOwnershipIndex].varStruct.ownership.issuanceIndex = assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex;
        }
        assets[*destinationOwnershipIndex].varStruct.ownership.numberOfShares += numberOfShares;

        *destinationPossessionIndex = destinationPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
    iteration2:
        if (assets[*destinationPossessionIndex].varStruct.possession.type == EMPTY
            || (assets[*destinationPossessionIndex].varStruct.possession.type == POSSESSION
                && assets[*destinationPossessionIndex].varStruct.possession.managingContractIndex == assets[sourcePossessionIndex].varStruct.possession.managingContractIndex
                && assets[*destinationPossessionIndex].varStruct.possession.ownershipIndex == *destinationOwnershipIndex
                && assets[*destinationPossessionIndex].varStruct.possession.publicKey == destinationPublicKey))
        {
            assets[sourcePossessionIndex].varStruct.possession.numberOfShares -= numberOfShares;

            if (assets[*destinationPossessionIndex].varStruct.possession.type == EMPTY)
            {
                assets[*destinationPossessionIndex].varStruct.possession.publicKey = destinationPublicKey;
                assets[*destinationPossessionIndex].varStruct.possession.type = POSSESSION;
                assets[*destinationPossessionIndex].varStruct.possession.managingContractIndex = assets[sourcePossessionIndex].varStruct.possession.managingContractIndex;
                assets[*destinationPossessionIndex].varStruct.possession.ownershipIndex = *destinationOwnershipIndex;
            }
            assets[*destinationPossessionIndex].varStruct.possession.numberOfShares += numberOfShares;

            assetChangeFlags[sourceOwnershipIndex >> 6] |= (1ULL << (sourceOwnershipIndex & 63));
            assetChangeFlags[sourcePossessionIndex >> 6] |= (1ULL << (sourcePossessionIndex & 63));
            assetChangeFlags[*destinationOwnershipIndex >> 6] |= (1ULL << (*destinationOwnershipIndex & 63));
            assetChangeFlags[*destinationPossessionIndex >> 6] |= (1ULL << (*destinationPossessionIndex & 63));

            if (lock)
            {
                RELEASE(universeLock);
            }

            AssetOwnershipChange assetOwnershipChange;
            assetOwnershipChange.sourcePublicKey = assets[sourceOwnershipIndex].varStruct.ownership.publicKey;
            assetOwnershipChange.destinationPublicKey = destinationPublicKey;
            assetOwnershipChange.issuerPublicKey = assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.publicKey;
            assetOwnershipChange.numberOfShares = numberOfShares;
            *((unsigned long long*) & assetOwnershipChange.name) = *((unsigned long long*) & assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.name); // Order must be preserved!
            assetOwnershipChange.numberOfDecimalPlaces = assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.numberOfDecimalPlaces; // Order must be preserved!
            *((unsigned long long*) & assetOwnershipChange.unitOfMeasurement) = *((unsigned long long*) & assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.unitOfMeasurement); // Order must be preserved!
            logger.logAssetOwnershipChange(assetOwnershipChange);

            AssetPossessionChange assetPossessionChange;
            assetPossessionChange.sourcePublicKey = assets[sourcePossessionIndex].varStruct.possession.publicKey;
            assetPossessionChange.destinationPublicKey = destinationPublicKey;
            assetPossessionChange.issuerPublicKey = assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.publicKey;
            assetPossessionChange.numberOfShares = numberOfShares;
            *((unsigned long long*) & assetPossessionChange.name) = *((unsigned long long*) & assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.name); // Order must be preserved!
            assetPossessionChange.numberOfDecimalPlaces = assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.numberOfDecimalPlaces; // Order must be preserved!
            *((unsigned long long*) & assetPossessionChange.unitOfMeasurement) = *((unsigned long long*) & assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.unitOfMeasurement); // Order must be preserved!
            logger.logAssetPossessionChange(assetPossessionChange);

            return true;
        }
        else
        {
            *destinationPossessionIndex = (*destinationPossessionIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration2;
        }
    }
    else
    {
        *destinationOwnershipIndex = (*destinationOwnershipIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }
}

// Should only be called from tick processor to avoid concurrent asset state changes, which may cause race conditions
static void getUniverseDigest(m256i& digest)
{
    unsigned int digestIndex;
    for (digestIndex = 0; digestIndex < ASSETS_CAPACITY; digestIndex++)
    {
        if (assetChangeFlags[digestIndex >> 6] & (1ULL << (digestIndex & 63)))
        {
            KangarooTwelve(&assets[digestIndex], sizeof(Asset), &assetDigests[digestIndex], 32);
        }
    }
    unsigned int previousLevelBeginning = 0;
    unsigned int numberOfLeafs = ASSETS_CAPACITY;
    while (numberOfLeafs > 1)
    {
        for (unsigned int i = 0; i < numberOfLeafs; i += 2)
        {
            if (assetChangeFlags[i >> 6] & (3ULL << (i & 63)))
            {
                KangarooTwelve64To32(&assetDigests[previousLevelBeginning + i], &assetDigests[digestIndex]);
                assetChangeFlags[i >> 6] &= ~(3ULL << (i & 63));
                assetChangeFlags[i >> 7] |= (1ULL << ((i >> 1) & 63));
            }
            digestIndex++;
        }
        previousLevelBeginning += numberOfLeafs;
        numberOfLeafs >>= 1;
    }
    assetChangeFlags[0] = 0;

    digest = assetDigests[(ASSETS_CAPACITY * 2 - 1) - 1];
}


static bool saveUniverse(CHAR16* directory = NULL)
{
    logToConsole(L"Saving universe file...");

    const unsigned long long beginningTick = __rdtsc();

    ACQUIRE(universeLock);
    long long savedSize = save(UNIVERSE_FILE_NAME, ASSETS_CAPACITY * sizeof(Asset), (unsigned char*)assets, directory);
    RELEASE(universeLock);

    if (savedSize == ASSETS_CAPACITY * sizeof(Asset))
    {
        setNumber(message, savedSize, TRUE);
        appendText(message, L" bytes of the universe data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
        return true;
    }
    return false;
}

static bool loadUniverse(CHAR16* directory = NULL)
{
    long long loadedSize = load(UNIVERSE_FILE_NAME, ASSETS_CAPACITY * sizeof(Asset), (unsigned char*)assets, directory);
    if (loadedSize != ASSETS_CAPACITY * sizeof(Asset))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);

        return false;
    }
    return true;
}

static void assetsEndEpoch()
{
    ACQUIRE(universeLock);

    // TODO: comment what is done here
    Asset* reorgAssets = (Asset*)reorgBuffer;
    setMem(reorgAssets, ASSETS_CAPACITY * sizeof(Asset), 0);
    for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
    {
        if (assets[i].varStruct.possession.type == POSSESSION
            && assets[i].varStruct.possession.numberOfShares > 0)
        {
            const unsigned int oldOwnershipIndex = assets[i].varStruct.possession.ownershipIndex;
            const unsigned int oldIssuanceIndex = assets[oldOwnershipIndex].varStruct.ownership.issuanceIndex;
            const m256i& issuerPublicKey = assets[oldIssuanceIndex].varStruct.issuance.publicKey;
            char* name = assets[oldIssuanceIndex].varStruct.issuance.name;
            int issuanceIndex = issuerPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        iteration2:
            if (reorgAssets[issuanceIndex].varStruct.issuance.type == EMPTY
                || (reorgAssets[issuanceIndex].varStruct.issuance.type == ISSUANCE
                    && ((*((unsigned long long*)reorgAssets[issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == ((*((unsigned long long*)name)) & 0xFFFFFFFFFFFFFF)
                    && reorgAssets[issuanceIndex].varStruct.issuance.publicKey == issuerPublicKey))
            {
                if (reorgAssets[issuanceIndex].varStruct.issuance.type == EMPTY)
                {
                    copyMem(&reorgAssets[issuanceIndex], &assets[oldIssuanceIndex], sizeof(Asset));
                }

                const m256i& ownerPublicKey = assets[oldOwnershipIndex].varStruct.ownership.publicKey;
                int ownershipIndex = ownerPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
            iteration3:
                if (reorgAssets[ownershipIndex].varStruct.ownership.type == EMPTY
                    || (reorgAssets[ownershipIndex].varStruct.ownership.type == OWNERSHIP
                        && reorgAssets[ownershipIndex].varStruct.ownership.managingContractIndex == assets[oldOwnershipIndex].varStruct.ownership.managingContractIndex
                        && reorgAssets[ownershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
                        && reorgAssets[ownershipIndex].varStruct.ownership.publicKey == ownerPublicKey))
                {
                    if (reorgAssets[ownershipIndex].varStruct.ownership.type == EMPTY)
                    {
                        reorgAssets[ownershipIndex].varStruct.ownership.publicKey = ownerPublicKey;
                        reorgAssets[ownershipIndex].varStruct.ownership.type = OWNERSHIP;
                        reorgAssets[ownershipIndex].varStruct.ownership.managingContractIndex = assets[oldOwnershipIndex].varStruct.ownership.managingContractIndex;
                        reorgAssets[ownershipIndex].varStruct.ownership.issuanceIndex = issuanceIndex;
                    }
                    reorgAssets[ownershipIndex].varStruct.ownership.numberOfShares += assets[i].varStruct.possession.numberOfShares;

                    int possessionIndex = assets[i].varStruct.possession.publicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
                iteration4:
                    if (reorgAssets[possessionIndex].varStruct.possession.type == EMPTY
                        || (reorgAssets[possessionIndex].varStruct.possession.type == POSSESSION
                            && reorgAssets[possessionIndex].varStruct.possession.managingContractIndex == assets[i].varStruct.possession.managingContractIndex
                            && reorgAssets[possessionIndex].varStruct.possession.ownershipIndex == ownershipIndex
                            && reorgAssets[possessionIndex].varStruct.possession.publicKey == assets[i].varStruct.possession.publicKey))
                    {
                        if (reorgAssets[possessionIndex].varStruct.possession.type == EMPTY)
                        {
                            reorgAssets[possessionIndex].varStruct.possession.publicKey = assets[i].varStruct.possession.publicKey;
                            reorgAssets[possessionIndex].varStruct.possession.type = POSSESSION;
                            reorgAssets[possessionIndex].varStruct.possession.managingContractIndex = assets[i].varStruct.possession.managingContractIndex;
                            reorgAssets[possessionIndex].varStruct.possession.ownershipIndex = ownershipIndex;
                        }
                        reorgAssets[possessionIndex].varStruct.possession.numberOfShares += assets[i].varStruct.possession.numberOfShares;
                    }
                    else
                    {
                        possessionIndex = (possessionIndex + 1) & (ASSETS_CAPACITY - 1);

                        goto iteration4;
                    }
                }
                else
                {
                    ownershipIndex = (ownershipIndex + 1) & (ASSETS_CAPACITY - 1);

                    goto iteration3;
                }
            }
            else
            {
                issuanceIndex = (issuanceIndex + 1) & (ASSETS_CAPACITY - 1);

                goto iteration2;
            }
        }
    }
    copyMem(assets, reorgAssets, ASSETS_CAPACITY * sizeof(Asset));

    setMem(assetChangeFlags, ASSETS_CAPACITY / 8, 0xFF);

    RELEASE(universeLock);
}
