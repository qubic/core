#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include <lib/platform_efi/uefi.h>
#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"
#include "platform/memory_util.h"
#include "platform/profiling.h"

#include "network_messages/assets.h"

#include "contract_core/contract_def.h"

#include "public_settings.h"
#include "logging/logging.h"
#include "kangaroo_twelve.h"
#include "four_q.h"
#include "common_buffers.h"


// CAUTION: Currently, there is no locking of universeLock if contracts use the QPI asset iteration classes directly.
// This shouldn't be a problem as long as:
// - write access to assets only happens in contractProcessor(), which runs contract procedures,
//   or tickProcessor(), which doesn't run in parallel to contractProcessor(),
//   or during a single-threading phase (node startup); NO WRITING OF ASSETS IN REQUEST PROCESSOR OR MAIN THREAD!
// - QPI asset iteration classes do not allow writing access to the universe (if this is changed in the future,
//   note that all write access requires locking universeLock)

// TODO: move this into AssetStorage class
GLOBAL_VAR_DECL volatile char universeLock GLOBAL_VAR_INIT(0);
GLOBAL_VAR_DECL AssetRecord* assets GLOBAL_VAR_INIT(nullptr);
GLOBAL_VAR_DECL m256i* assetDigests GLOBAL_VAR_INIT(nullptr);
static constexpr unsigned long long assetDigestsSizeInBytes = (ASSETS_CAPACITY * 2 - 1) * 32ULL;
GLOBAL_VAR_DECL unsigned long long* assetChangeFlags GLOBAL_VAR_INIT(nullptr);
static constexpr char CONTRACT_ASSET_UNIT_OF_MEASUREMENT[7] = { 0, 0, 0, 0, 0, 0, 0 };

static constexpr unsigned int NO_ASSET_INDEX = 0xffffffff;


struct AssetStorage
{

    // TODO: iterators / filters
    // class for iterating issuance
    // class for selecting issuance
    // class for selecting / iterating ownership with given issuance
    // class for selecting / iterating possession with given issuance and potentially given ownership

public: // TODO: make protected
    


    // Lists (single-linked) of
    // - all issuances,
    // - all ownerships belonging to each issuance
    // - all possessions belonging to each ownership
    struct IndexLists
    {
        unsigned int issuancesFirstIdx;
        unsigned int ownershipsPossessionsFirstIdx[ASSETS_CAPACITY];

        unsigned int nextIdx[ASSETS_CAPACITY];

        void addIssuance(unsigned int newIssuanceIdx)
        {
            // add as first element in linked list of all issuances
            ASSERT(newIssuanceIdx < ASSETS_CAPACITY);
            ASSERT(assets[newIssuanceIdx].varStruct.issuance.type == ISSUANCE);
            ASSERT(issuancesFirstIdx == NO_ASSET_INDEX || assets[issuancesFirstIdx].varStruct.issuance.type == ISSUANCE);
            nextIdx[newIssuanceIdx] = issuancesFirstIdx;
            issuancesFirstIdx = newIssuanceIdx;
        }

        // Add newOwnershipIdx as first element in linked list of all ownerships of issuanceIdx
        void addOwnership(unsigned int issuanceIdx, unsigned int newOwnershipIdx)
        {
            ASSERT(issuanceIdx < ASSETS_CAPACITY);
            ASSERT(newOwnershipIdx < ASSETS_CAPACITY);
            ASSERT(assets[issuanceIdx].varStruct.issuance.type == ISSUANCE);
            ASSERT(assets[newOwnershipIdx].varStruct.ownership.type == OWNERSHIP);
            ASSERT(ownershipsPossessionsFirstIdx[issuanceIdx] == NO_ASSET_INDEX || assets[ownershipsPossessionsFirstIdx[issuanceIdx]].varStruct.issuance.type == OWNERSHIP);
            nextIdx[newOwnershipIdx] = ownershipsPossessionsFirstIdx[issuanceIdx];
            ownershipsPossessionsFirstIdx[issuanceIdx] = newOwnershipIdx;
        }

        // Add newPossessionIdx as first element in linked list of all possessions of ownershipIdx
        void addPossession(unsigned int ownershipIdx, unsigned int newPossessionIdx)
        {
            ASSERT(ownershipIdx < ASSETS_CAPACITY);
            ASSERT(newPossessionIdx < ASSETS_CAPACITY);
            ASSERT(assets[ownershipIdx].varStruct.ownership.type == OWNERSHIP);
            ASSERT(assets[newPossessionIdx].varStruct.possession.type == POSSESSION);
            ASSERT(ownershipsPossessionsFirstIdx[ownershipIdx] == NO_ASSET_INDEX || assets[ownershipsPossessionsFirstIdx[ownershipIdx]].varStruct.possession.type == POSSESSION);
            nextIdx[newPossessionIdx] = ownershipsPossessionsFirstIdx[ownershipIdx];
            ownershipsPossessionsFirstIdx[ownershipIdx] = newPossessionIdx;
        }

        // Reset lists to empty
        void reset()
        {
            issuancesFirstIdx = NO_ASSET_INDEX;
            static_assert(NO_ASSET_INDEX == 0xffffffff, "Following setMem() expects NO_ASSET_INDEX == 0xffffffff");
            setMem(ownershipsPossessionsFirstIdx, sizeof(ownershipsPossessionsFirstIdx), 0xff);
            setMem(nextIdx, sizeof(nextIdx), 0xff);
        }

        // Rebuild lists from assets array (includes reset)
        void rebuild()
        {
            PROFILE_SCOPE();

            reset();
            for (int index = 0; index < ASSETS_CAPACITY; index++)
            {
                switch (assets[index].varStruct.issuance.type)
                {
                case ISSUANCE:
                    addIssuance(index);
                    break;
                case OWNERSHIP:
                    addOwnership(assets[index].varStruct.ownership.issuanceIndex, index);
                    break;
                case POSSESSION:
                    addPossession(assets[index].varStruct.possession.ownershipIndex, index);
                    break;
                }
            }
        }
    };

    inline static IndexLists indexLists;
};

GLOBAL_VAR_DECL AssetStorage as;


// Return index of issuance in assets array / universe or NO_ASSET_INDEX is not found.
static unsigned int issuanceIndex(const m256i& issuer, unsigned long long assetName)
{
    PROFILE_SCOPE();

    unsigned int idx = issuer.m256i_u32[0] & (ASSETS_CAPACITY - 1);
    while (assets[idx].varStruct.issuance.type != EMPTY)
    {
        if (assets[idx].varStruct.issuance.type == ISSUANCE
            && ((*((unsigned long long*)assets[idx].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == assetName
            && assets[idx].varStruct.issuance.publicKey == issuer)
        {
            // found matching entry
            return idx;
        }

        idx = (idx + 1) & (ASSETS_CAPACITY - 1);
    }

    // no matching entry found
    return NO_ASSET_INDEX;
}





static bool initAssets()
{
    if (!allocPoolWithErrorLog(L"assets", ASSETS_CAPACITY * sizeof(AssetRecord), (void**)&assets, __LINE__)
        || !allocPoolWithErrorLog(L"assetDigets", assetDigestsSizeInBytes, (void**)&assetDigests, __LINE__)
        || !allocPoolWithErrorLog(L"assetChangeFlags", ASSETS_CAPACITY / 8, (void**)&assetChangeFlags, __LINE__))
    {
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
    PROFILE_SCOPE();

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

                as.indexLists.addIssuance(*issuanceIndex);
                as.indexLists.addOwnership(*issuanceIndex, *ownershipIndex);
                as.indexLists.addPossession(*ownershipIndex, *possessionIndex);

                RELEASE(universeLock);

                AssetIssuance assetIssuance;
                assetIssuance.issuerPublicKey = issuerPublicKey;
                assetIssuance.numberOfShares = numberOfShares;
                assetIssuance.managingContractIndex = managingContractIndex; // any SC can call issueAsset now (eg: QBOND) not just QX
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

static sint64 numberOfShares(
    const Asset& asset,
    const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(),
    const AssetPossessionSelect& possession = AssetPossessionSelect::any())
{
    PROFILE_SCOPE();

    ACQUIRE(universeLock);

    sint64 numOfShares = 0;
    if (possession.anyPossessor && possession.anyManagingContract)
    {
        for (AssetOwnershipIterator iter(asset, ownership); !iter.reachedEnd(); iter.next())
        {
            numOfShares += iter.numberOfOwnedShares();
        }
    }
    else
    {
        for (AssetPossessionIterator iter(asset, ownership, possession); !iter.reachedEnd(); iter.next())
        {
            numOfShares += iter.numberOfPossessedShares();
        }
    }

    RELEASE(universeLock);

    return numOfShares;
}

// Transfer asset management rights, creating new ownership and possession records in the universe.
// A nullptr may be passed as destinationOwnershipIndexPtr and destinationPossessionIndexPtr if this
// info is not needed by the calling function.
static bool transferShareManagementRights(int sourceOwnershipIndex, int sourcePossessionIndex,
    unsigned short destinationOwnershipManagingContractIndex,
    unsigned short destinationPossessionManagingContractIndex,
    long long numberOfShares,
    int* destinationOwnershipIndexPtr, int* destinationPossessionIndexPtr,
    bool lock)
{
    PROFILE_SCOPE();

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

    const m256i& ownershipPublicKey = assets[sourceOwnershipIndex].varStruct.ownership.publicKey;
    const m256i& possessionPublicKey = assets[sourcePossessionIndex].varStruct.possession.publicKey;
    const int issuanceIndex = assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex;

    int destinationOwnershipIndex = ownershipPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
iteration:
    if (assets[destinationOwnershipIndex].varStruct.ownership.type == EMPTY
        || (assets[destinationOwnershipIndex].varStruct.ownership.type == OWNERSHIP
            && assets[destinationOwnershipIndex].varStruct.ownership.managingContractIndex == destinationOwnershipManagingContractIndex
            && assets[destinationOwnershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
            && assets[destinationOwnershipIndex].varStruct.ownership.publicKey == ownershipPublicKey))
    {
        // found empty slot for ownership record or existing record to update
        assets[sourceOwnershipIndex].varStruct.ownership.numberOfShares -= numberOfShares;

        if (assets[destinationOwnershipIndex].varStruct.ownership.type == EMPTY)
        {
            assets[destinationOwnershipIndex].varStruct.ownership.publicKey = ownershipPublicKey;
            assets[destinationOwnershipIndex].varStruct.ownership.type = OWNERSHIP;
            assets[destinationOwnershipIndex].varStruct.ownership.managingContractIndex = destinationOwnershipManagingContractIndex;
            assets[destinationOwnershipIndex].varStruct.ownership.issuanceIndex = issuanceIndex;

            as.indexLists.addOwnership(issuanceIndex, destinationOwnershipIndex);
        }
        assets[destinationOwnershipIndex].varStruct.ownership.numberOfShares += numberOfShares;

        int destinationPossessionIndex = possessionPublicKey.m256i_u32[0] & (ASSETS_CAPACITY - 1);
    iteration2:
        if (assets[destinationPossessionIndex].varStruct.possession.type == EMPTY
            || (assets[destinationPossessionIndex].varStruct.possession.type == POSSESSION
                && assets[destinationPossessionIndex].varStruct.possession.managingContractIndex == destinationPossessionManagingContractIndex
                && assets[destinationPossessionIndex].varStruct.possession.ownershipIndex == destinationOwnershipIndex
                && assets[destinationPossessionIndex].varStruct.possession.publicKey == possessionPublicKey))
        {
            // found empty slot for poss possession or existing record to update
            assets[sourcePossessionIndex].varStruct.possession.numberOfShares -= numberOfShares;

            if (assets[destinationPossessionIndex].varStruct.possession.type == EMPTY)
            {
                assets[destinationPossessionIndex].varStruct.possession.publicKey = possessionPublicKey;
                assets[destinationPossessionIndex].varStruct.possession.type = POSSESSION;
                assets[destinationPossessionIndex].varStruct.possession.managingContractIndex = destinationPossessionManagingContractIndex;
                assets[destinationPossessionIndex].varStruct.possession.ownershipIndex = destinationOwnershipIndex;

                as.indexLists.addPossession(destinationOwnershipIndex, destinationPossessionIndex);
            }
            assets[destinationPossessionIndex].varStruct.possession.numberOfShares += numberOfShares;

            assetChangeFlags[sourceOwnershipIndex >> 6] |= (1ULL << (sourceOwnershipIndex & 63));
            assetChangeFlags[sourcePossessionIndex >> 6] |= (1ULL << (sourcePossessionIndex & 63));
            assetChangeFlags[destinationOwnershipIndex >> 6] |= (1ULL << (destinationOwnershipIndex & 63));
            assetChangeFlags[destinationPossessionIndex >> 6] |= (1ULL << (destinationPossessionIndex & 63));

            if (lock)
            {
                RELEASE(universeLock);
            }

            AssetOwnershipManagingContractChange logOM;
            logOM.ownershipPublicKey = ownershipPublicKey;
            logOM.issuerPublicKey = assets[issuanceIndex].varStruct.issuance.publicKey;
            logOM.sourceContractIndex = assets[sourceOwnershipIndex].varStruct.ownership.managingContractIndex;
            logOM.destinationContractIndex = destinationOwnershipManagingContractIndex;
            logOM.numberOfShares = numberOfShares;
            *((unsigned long long*) & logOM.assetName) = *((unsigned long long*) & assets[issuanceIndex].varStruct.issuance.name); // possible with 7 byte array, because it is followed by memory reserved for terminator byte
            logger.logAssetOwnershipManagingContractChange(logOM);

            AssetPossessionManagingContractChange logPM;
            logPM.possessionPublicKey = possessionPublicKey;
            logPM.ownershipPublicKey = ownershipPublicKey;
            logPM.issuerPublicKey = assets[issuanceIndex].varStruct.issuance.publicKey;
            logPM.sourceContractIndex = assets[sourcePossessionIndex].varStruct.ownership.managingContractIndex;
            logPM.destinationContractIndex = destinationPossessionManagingContractIndex;
            logPM.numberOfShares = numberOfShares;
            *((unsigned long long*) & logPM.assetName) = *((unsigned long long*) & assets[assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex].varStruct.issuance.name);  // possible with 7 byte array, because it is followed by memory reserved for terminator byte
            logger.logAssetPossessionManagingContractChange(logPM);

            if (destinationOwnershipIndexPtr)
            {
                *destinationOwnershipIndexPtr = destinationOwnershipIndex;
            }
            if (destinationPossessionIndexPtr)
            {
                *destinationPossessionIndexPtr = destinationPossessionIndex;
            }

            return true;
        }
        else
        {
            // try next slot for finding new possession record
            destinationPossessionIndex = (destinationPossessionIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration2;
        }
    }
    else
    {
        // try next slot for finding new ownership record
        destinationOwnershipIndex = (destinationOwnershipIndex + 1) & (ASSETS_CAPACITY - 1);

        goto iteration;
    }
}

static bool transferShareOwnershipAndPossession(int sourceOwnershipIndex, int sourcePossessionIndex, const m256i& destinationPublicKey, long long numberOfShares,
    int* destinationOwnershipIndex, int* destinationPossessionIndex,
    bool lock)
{
    PROFILE_SCOPE();

    if (numberOfShares <= 0)
    {
        return false;
    }

    if (lock)
    {
        ACQUIRE(universeLock);
    }

    ASSERT(sourceOwnershipIndex >= 0 && sourceOwnershipIndex < ASSETS_CAPACITY);
    ASSERT(sourcePossessionIndex >= 0 && sourcePossessionIndex < ASSETS_CAPACITY);
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

    // Special case: all-zero destination means burning shares
    if (isZero(destinationPublicKey))
    {
        // Don't allow burning of contract shares
        const unsigned int issuanceIndex = assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex;
        ASSERT(issuanceIndex < ASSETS_CAPACITY);
        const auto& issuance = assets[issuanceIndex].varStruct.issuance;
        ASSERT(issuance.type == ISSUANCE);
        if (isZero(issuance.publicKey))
        {
            if (lock)
            {
                RELEASE(universeLock);
            }

            return false;
        }

        // Burn by subtracting shares from source records
        assets[sourceOwnershipIndex].varStruct.ownership.numberOfShares -= numberOfShares;
        assets[sourcePossessionIndex].varStruct.possession.numberOfShares -= numberOfShares;
        assetChangeFlags[sourceOwnershipIndex >> 6] |= (1ULL << (sourceOwnershipIndex & 63));
        assetChangeFlags[sourcePossessionIndex >> 6] |= (1ULL << (sourcePossessionIndex & 63));

        if (lock)
        {
            RELEASE(universeLock);
        }

        AssetOwnershipChange assetOwnershipChange;
        assetOwnershipChange.sourcePublicKey = assets[sourceOwnershipIndex].varStruct.ownership.publicKey;
        assetOwnershipChange.destinationPublicKey = destinationPublicKey;
        assetOwnershipChange.issuerPublicKey = issuance.publicKey;
        assetOwnershipChange.numberOfShares = numberOfShares;
        *((unsigned long long*) & assetOwnershipChange.name) = *((unsigned long long*) & issuance.name); // Order must be preserved!
        assetOwnershipChange.numberOfDecimalPlaces = issuance.numberOfDecimalPlaces; // Order must be preserved!
        *((unsigned long long*) & assetOwnershipChange.unitOfMeasurement) = *((unsigned long long*) & issuance.unitOfMeasurement); // Order must be preserved!
        logger.logAssetOwnershipChange(assetOwnershipChange);

        AssetPossessionChange assetPossessionChange;
        assetPossessionChange.sourcePublicKey = assets[sourcePossessionIndex].varStruct.possession.publicKey;
        assetPossessionChange.destinationPublicKey = destinationPublicKey;
        assetPossessionChange.issuerPublicKey = issuance.publicKey;
        assetPossessionChange.numberOfShares = numberOfShares;
        *((unsigned long long*) & assetPossessionChange.name) = *((unsigned long long*) & issuance.name); // Order must be preserved!
        assetPossessionChange.numberOfDecimalPlaces = issuance.numberOfDecimalPlaces; // Order must be preserved!
        *((unsigned long long*) & assetPossessionChange.unitOfMeasurement) = *((unsigned long long*) & issuance.unitOfMeasurement); // Order must be preserved!
        logger.logAssetPossessionChange(assetPossessionChange);

        return true;
    }

    // Default case: transfer shares to destinationPublicKey
    ASSERT(destinationOwnershipIndex != nullptr);
    ASSERT(destinationPossessionIndex != nullptr);
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

            as.indexLists.addOwnership(assets[sourceOwnershipIndex].varStruct.ownership.issuanceIndex, *destinationOwnershipIndex);
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

                as.indexLists.addPossession(*destinationOwnershipIndex, *destinationPossessionIndex);
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

static long long numberOfPossessedShares(unsigned long long assetName, const m256i& issuer, const m256i& owner, const m256i& possessor, unsigned short ownershipManagingContractIndex, unsigned short possessionManagingContractIndex)
{
    PROFILE_SCOPE();

    ACQUIRE(universeLock);

    int issuanceIndex = issuer.m256i_u32[0] & (ASSETS_CAPACITY - 1);
iteration:
    if (assets[issuanceIndex].varStruct.issuance.type == EMPTY)
    {
        RELEASE(universeLock);

        return 0;
    }
    else
    {
        if (assets[issuanceIndex].varStruct.issuance.type == ISSUANCE
            && ((*((unsigned long long*)assets[issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == assetName
            && assets[issuanceIndex].varStruct.issuance.publicKey == issuer)
        {
            int ownershipIndex = owner.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        iteration2:
            if (assets[ownershipIndex].varStruct.ownership.type == EMPTY)
            {
                RELEASE(universeLock);

                return 0;
            }
            else
            {
                if (assets[ownershipIndex].varStruct.ownership.type == OWNERSHIP
                    && assets[ownershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
                    && assets[ownershipIndex].varStruct.ownership.publicKey == owner
                    && assets[ownershipIndex].varStruct.ownership.managingContractIndex == ownershipManagingContractIndex)
                {
                    int possessionIndex = possessor.m256i_u32[0] & (ASSETS_CAPACITY - 1);
                iteration3:
                    if (assets[possessionIndex].varStruct.possession.type == EMPTY)
                    {
                        RELEASE(universeLock);

                        return 0;
                    }
                    else
                    {
                        if (assets[possessionIndex].varStruct.possession.type == POSSESSION
                            && assets[possessionIndex].varStruct.possession.ownershipIndex == ownershipIndex
                            && assets[possessionIndex].varStruct.possession.publicKey == possessor
                            && assets[possessionIndex].varStruct.possession.managingContractIndex == possessionManagingContractIndex)
                        {
                            const long long numberOfPossessedShares = assets[possessionIndex].varStruct.possession.numberOfShares;

                            RELEASE(universeLock);

                            return numberOfPossessedShares;
                        }
                        else
                        {
                            possessionIndex = (possessionIndex + 1) & (ASSETS_CAPACITY - 1);

                            goto iteration3;
                        }
                    }
                }
                else
                {
                    ownershipIndex = (ownershipIndex + 1) & (ASSETS_CAPACITY - 1);

                    goto iteration2;
                }
            }
        }
        else
        {
            issuanceIndex = (issuanceIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration;
        }
    }
}

// Should only be called from tick processor to avoid concurrent asset state changes, which may cause race conditions
static void getUniverseDigest(m256i& digest)
{
    PROFILE_SCOPE();

    unsigned int digestIndex;
    for (digestIndex = 0; digestIndex < ASSETS_CAPACITY; digestIndex++)
    {
        if (assetChangeFlags[digestIndex >> 6] & (1ULL << (digestIndex & 63)))
        {
            KangarooTwelve(&assets[digestIndex], sizeof(AssetRecord), &assetDigests[digestIndex], 32);
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


static bool saveUniverse(const CHAR16* fileName = UNIVERSE_FILE_NAME, const CHAR16* directory = NULL)
{
    PROFILE_SCOPE();

    logToConsole(L"Saving universe file...");

    const unsigned long long beginningTick = __rdtsc();

    ACQUIRE(universeLock);
    long long savedSize = save(fileName, ASSETS_CAPACITY * sizeof(AssetRecord), (unsigned char*)assets, directory);
    RELEASE(universeLock);

    if (savedSize == ASSETS_CAPACITY * sizeof(AssetRecord))
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

static bool loadUniverse(const CHAR16* fileName = UNIVERSE_FILE_NAME, CHAR16* directory = NULL)
{
    PROFILE_SCOPE();

    long long loadedSize = load(fileName, ASSETS_CAPACITY * sizeof(AssetRecord), (unsigned char*)assets, directory);
    if (loadedSize != ASSETS_CAPACITY * sizeof(AssetRecord))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);

        return false;
    }
    as.indexLists.rebuild();
    return true;
}

static void assetsEndEpoch()
{
    PROFILE_SCOPE();

    ACQUIRE(universeLock);

    // rebuild asset hash map, getting rid of all elements with zero shares
    AssetRecord* reorgAssets = (AssetRecord*)reorgBuffer;
    setMem(reorgAssets, ASSETS_CAPACITY * sizeof(AssetRecord), 0);
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
                    copyMem(&reorgAssets[issuanceIndex], &assets[oldIssuanceIndex], sizeof(AssetRecord));
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
    copyMem(assets, reorgAssets, ASSETS_CAPACITY * sizeof(AssetRecord));

    setMem(assetChangeFlags, ASSETS_CAPACITY / 8, 0xFF);

    as.indexLists.rebuild();

    RELEASE(universeLock);
}
