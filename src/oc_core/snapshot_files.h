#pragma once

#include "oc_core/oc_engine.h"

#if TICK_STORAGE_AUTOSAVE_MODE

#include "platform/file_io.h"


static CHAR16 OC_SNAPSHOT_ENGINE_FILENAME[] = L"snapshotOcEngine.???";
static CHAR16 OC_SNAPSHOT_INVOCATIONS_FILENAME[] = L"snapshotOcInvocations.???";
static CHAR16 OC_SNAPSHOT_REQUEST_STORAGE_FILENAME[] = L"snapshotOcRequestStorage.???";
static CHAR16 OC_SNAPSHOT_IN_FLIGHT_FILENAME[] = L"snapshotOcInFlight.???";
static CHAR16 OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME[] = L"snapshotOcInFlightOwners.???";


// Fixed-size header saved to snapshotOcEngine.???
//
// Persists invocation records, pinned request storage, the in-flight auth-state pool
// with its per-slot owner map, and the invocationId-state counters. The per-record
// delivery flag is written as part of the record but reset to "not yet delivered" on
// load (delivery is node-local state). firstActiveIndex (recomputed by the next per-tick
// scan) and the invocationIdToIndex hash map (rebuilt from invocation records on load)
// are not persisted.
struct OcEngineSnapshotData
{
    uint64_t requestStorageBytesUsed;
    uint32_t invocationCount;
    uint32_t inFlightSlotCursor;
    uint32_t invocationIdStateTick;
    uint32_t invocationIdStateIndexInTick;
    OcEngineStatistics stats;
};


bool OcEngine::saveSnapshot(unsigned short epoch, CHAR16* directory) const
{
    addEpochToFileName(OC_SNAPSHOT_ENGINE_FILENAME, sizeof(OC_SNAPSHOT_ENGINE_FILENAME) / sizeof(OC_SNAPSHOT_ENGINE_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_INVOCATIONS_FILENAME, sizeof(OC_SNAPSHOT_INVOCATIONS_FILENAME) / sizeof(OC_SNAPSHOT_INVOCATIONS_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME, sizeof(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME) / sizeof(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_IN_FLIGHT_FILENAME, sizeof(OC_SNAPSHOT_IN_FLIGHT_FILENAME) / sizeof(OC_SNAPSHOT_IN_FLIGHT_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME, sizeof(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME) / sizeof(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME[0]), epoch);

    LockGuard lockGuard(lock);

    OcEngineSnapshotData engineData;
    engineData.requestStorageBytesUsed = requestStorageBytesUsed;
    engineData.invocationCount = invocationCount;
    engineData.inFlightSlotCursor = inFlightSlotCursor;
    engineData.invocationIdStateTick = invocationIdState.tick;
    engineData.invocationIdStateIndexInTick = invocationIdState.indexInTick;
    copyMemory(engineData.stats, stats);

    logToConsole(L"Saving OC engine data ...");
    long long sz = saveLargeFile(OC_SNAPSHOT_ENGINE_FILENAME, sizeof(engineData), (unsigned char*)&engineData, directory);
    if (sz != sizeof(engineData))
    {
        logToConsole(L"Failed to save OC engine data!");
        return false;
    }

    logToConsole(L"Saving OC invocation records ...");
    unsigned long long sizeToSave = sizeof(*invocations) * invocationCount;
    sz = saveLargeFile(OC_SNAPSHOT_INVOCATIONS_FILENAME, sizeToSave, (unsigned char*)invocations, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save OC invocation records!");
        return false;
    }

    logToConsole(L"Saving OC request storage ...");
    sizeToSave = requestStorageBytesUsed;
    sz = saveLargeFile(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME, sizeToSave, requestStorage, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save OC request storage!");
        return false;
    }

    logToConsole(L"Saving OC in-flight auth states ...");
    sizeToSave = sizeof(*inFlightStates) * MAX_OC_IN_FLIGHT_INVOCATIONS;
    sz = saveLargeFile(OC_SNAPSHOT_IN_FLIGHT_FILENAME, sizeToSave, (unsigned char*)inFlightStates, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save OC in-flight auth states!");
        return false;
    }

    logToConsole(L"Saving OC in-flight slot owners ...");
    sizeToSave = sizeof(*inFlightSlotOwners) * MAX_OC_IN_FLIGHT_INVOCATIONS;
    sz = saveLargeFile(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME, sizeToSave, (unsigned char*)inFlightSlotOwners, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save OC in-flight slot owners!");
        return false;
    }

    logToConsole(L"Successfully saved all OC engine data to snapshot!");
    return true;
}


bool OcEngine::loadSnapshot(unsigned short epoch, CHAR16* directory)
{
    addEpochToFileName(OC_SNAPSHOT_ENGINE_FILENAME, sizeof(OC_SNAPSHOT_ENGINE_FILENAME) / sizeof(OC_SNAPSHOT_ENGINE_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_INVOCATIONS_FILENAME, sizeof(OC_SNAPSHOT_INVOCATIONS_FILENAME) / sizeof(OC_SNAPSHOT_INVOCATIONS_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME, sizeof(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME) / sizeof(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_IN_FLIGHT_FILENAME, sizeof(OC_SNAPSHOT_IN_FLIGHT_FILENAME) / sizeof(OC_SNAPSHOT_IN_FLIGHT_FILENAME[0]), epoch);
    addEpochToFileName(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME, sizeof(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME) / sizeof(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME[0]), epoch);

    LockGuard lockGuard(lock);

    OcEngineSnapshotData engineData;

    logToConsole(L"Loading OC engine data ...");
    long long sz = loadLargeFile(OC_SNAPSHOT_ENGINE_FILENAME, sizeof(engineData), (unsigned char*)&engineData, directory);
    if (sz != sizeof(engineData))
    {
        logToConsole(L"Failed to load OC engine data!");
        return false;
    }

    requestStorageBytesUsed = engineData.requestStorageBytesUsed;
    invocationCount = engineData.invocationCount;
    inFlightSlotCursor = engineData.inFlightSlotCursor;
    invocationIdState.tick = engineData.invocationIdStateTick;
    invocationIdState.indexInTick = engineData.invocationIdStateIndexInTick;
    copyMemory(stats, engineData.stats);

    if (invocationCount > MAX_OC_INVOCATIONS_PER_EPOCH
        || requestStorageBytesUsed > OC_REQUEST_STORAGE_SIZE
        || inFlightSlotCursor >= MAX_OC_IN_FLIGHT_INVOCATIONS)
    {
        logToConsole(L"OC engine data is invalid!");
        return false;
    }

    logToConsole(L"Loading OC invocation records ...");
    unsigned long long sizeToLoad = sizeof(*invocations) * invocationCount;
    sz = loadLargeFile(OC_SNAPSHOT_INVOCATIONS_FILENAME, sizeToLoad, (unsigned char*)invocations, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load OC invocation records!");
        return false;
    }
    if (invocationCount < MAX_OC_INVOCATIONS_PER_EPOCH)
    {
        unsigned long long sizeToZero = (MAX_OC_INVOCATIONS_PER_EPOCH - invocationCount) * sizeof(*invocations);
        setMem(invocations + invocationCount, sizeToZero, 0);
    }

    // Delivery is node-local: clear the delivered flag on every record so this node
    // attempts delivery of any AUTHORIZED record whose auth state is still present.
    // Also validate that in-flight slot references are in range.
    for (uint32_t i = 0; i < invocationCount; ++i)
    {
        invocations[i].delivered = 0;
        if (invocations[i].inFlightSlot != OC_IN_FLIGHT_SLOT_NONE
            && invocations[i].inFlightSlot >= MAX_OC_IN_FLIGHT_INVOCATIONS)
        {
            logToConsole(L"OC invocation records are invalid!");
            return false;
        }
        // Bound paramsSize/paramsOffset before deliverAuthorizedInvocations copies
        // paramsSize bytes from requestStorage+paramsOffset into the fixed deliveryBuffer.
        // A corrupted record must fail the load, not overflow at delivery time.
        if (invocations[i].paramsSize > MAX_OC_REQUEST_SIZE
            || (uint64_t)invocations[i].paramsOffset + invocations[i].paramsSize > requestStorageBytesUsed)
        {
            logToConsole(L"OC invocation records are invalid!");
            return false;
        }
    }

    // Recomputed lazily by the next per-tick scan (node-local scan optimization).
    firstActiveIndex = 0;

    logToConsole(L"Loading OC request storage ...");
    sizeToLoad = requestStorageBytesUsed;
    sz = loadLargeFile(OC_SNAPSHOT_REQUEST_STORAGE_FILENAME, sizeToLoad, requestStorage, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load OC request storage!");
        return false;
    }
    if (requestStorageBytesUsed < OC_REQUEST_STORAGE_SIZE)
    {
        unsigned long long sizeToZero = OC_REQUEST_STORAGE_SIZE - requestStorageBytesUsed;
        setMem(requestStorage + requestStorageBytesUsed, sizeToZero, 0);
    }

    logToConsole(L"Loading OC in-flight auth states ...");
    sizeToLoad = sizeof(*inFlightStates) * MAX_OC_IN_FLIGHT_INVOCATIONS;
    sz = loadLargeFile(OC_SNAPSHOT_IN_FLIGHT_FILENAME, sizeToLoad, (unsigned char*)inFlightStates, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load OC in-flight auth states!");
        return false;
    }

    logToConsole(L"Loading OC in-flight slot owners ...");
    sizeToLoad = sizeof(*inFlightSlotOwners) * MAX_OC_IN_FLIGHT_INVOCATIONS;
    sz = loadLargeFile(OC_SNAPSHOT_IN_FLIGHT_OWNERS_FILENAME, sizeToLoad, (unsigned char*)inFlightSlotOwners, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load OC in-flight slot owners!");
        return false;
    }

    // Rebuild invocationIdToIndex (not snapshotted; recomputable from invocation records).
    invocationIdToIndex->reset();
    for (uint32_t idx = 0; idx < invocationCount; ++idx)
        invocationIdToIndex->set(invocations[idx].invocationId, idx);

    logToConsole(L"Successfully loaded all OC engine data from snapshot!");
    return true;
}


#else


bool OcEngine::saveSnapshot(unsigned short /*epoch*/, CHAR16* /*directory*/) const
{
    return false;
}

bool OcEngine::loadSnapshot(unsigned short /*epoch*/, CHAR16* /*directory*/)
{
    return false;
}


#endif
