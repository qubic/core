#pragma once

#include "oracle_core/oracle_engine.h"
#include "network_messages/oracles.h"

#if TICK_STORAGE_AUTOSAVE_MODE

#include "platform/file_io.h"

static unsigned short ORACLE_SNAPSHOT_ENGINE_FILENAME[] = L"snapshotOracleEngine.???";
static unsigned short ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME[] = L"snapshotOracleQueryMetadata.???";
static unsigned short ORACLE_SNAPSHOT_QUERY_DATA_FILENAME[] = L"snapshotOracleQueryData.???";
static unsigned short ORACLE_SNAPSHOT_REPLY_STATES_FILENAME[] = L"snapshotOracleReplyStates.???";


struct OracleEngineSnapshotData
{
    uint64_t queryStorageBytesUsed;
    uint32_t oracleQueryCount;
    uint32_t contractQueryIdStateTick;
    uint32_t contractQueryIdStateQueryIndexInTick;
    int32_t replyStatesIndex;
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingQueryIndices;
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingCommitReplyStateIndices;
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> pendingRevealReplyStateIndices;
    UnsortedMultiset<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> notificationQueryIndicies;
    OracleEngineStatistics stats;
};


// save state (excluding queryIdToIndex and unused parts of large buffers)
template <uint16_t ownComputorSeedsCount>
bool OracleEngine<ownComputorSeedsCount>::saveSnapshot(unsigned short epoch, CHAR16* directory) const
{
    addEpochToFileName(ORACLE_SNAPSHOT_ENGINE_FILENAME, sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME) / sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME) / sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME[0]), epoch);

    LockGuard lockGuard(lock);

    OracleEngineSnapshotData engineData;
    engineData.queryStorageBytesUsed = queryStorageBytesUsed;
    engineData.oracleQueryCount = oracleQueryCount;
    engineData.contractQueryIdStateTick = contractQueryIdState.tick;
    engineData.contractQueryIdStateQueryIndexInTick = contractQueryIdState.queryIndexInTick;
    engineData.replyStatesIndex = replyStatesIndex;
    copyMemory(engineData.pendingQueryIndices, pendingQueryIndices);
    copyMemory(engineData.pendingCommitReplyStateIndices, pendingCommitReplyStateIndices);
    copyMemory(engineData.pendingRevealReplyStateIndices, pendingRevealReplyStateIndices);
    copyMemory(engineData.notificationQueryIndicies, notificationQueryIndicies);
    copyMemory(engineData.stats, stats);

    logToConsole(L"Saving oracle engine data ...");
    long long sz = saveLargeFile(ORACLE_SNAPSHOT_ENGINE_FILENAME, sizeof(engineData), (unsigned char*)&engineData, directory);
    if (sz != sizeof(engineData))
    {
        logToConsole(L"Failed to save oracle engine data!");
        return false;
    }

    logToConsole(L"Saving oracle query metadata ...");
    unsigned long long sizeToSave = sizeof(*queries) * oracleQueryCount;
    // TODO: only save parts that were added after previous snapshot
    sz = saveLargeFile(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeToSave, (unsigned char*)queries, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save oracle query metadata!");
        return false;
    }

    logToConsole(L"Saving oracle query data storage ...");
    sizeToSave = queryStorageBytesUsed;
    // TODO: only save parts that were added after previous snapshot
    sz = saveLargeFile(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME, sizeToSave, queryStorage, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save oracle query data storage!");
        return false;
    }

    logToConsole(L"Saving oracle reply states ...");
    sizeToSave = sizeof(ReplyState) * MAX_SIMULTANEOUS_ORACLE_QUERIES;
    sz = saveLargeFile(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeToSave, (unsigned char*)replyStates, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save oracle reply states!");
        return false;
    }

    logToConsole(L"Successfully saved all oracle engine data to snapshot!");

    return true;
}

template <uint16_t ownComputorSeedsCount>
bool OracleEngine<ownComputorSeedsCount>::loadSnapshot(unsigned short epoch, CHAR16* directory)
{
    addEpochToFileName(ORACLE_SNAPSHOT_ENGINE_FILENAME, sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME) / sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME) / sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME[0]), epoch);

    LockGuard lockGuard(lock);

    OracleEngineSnapshotData engineData;

    logToConsole(L"Loading oracle engine data ...");
    long long sz = loadLargeFile(ORACLE_SNAPSHOT_ENGINE_FILENAME, sizeof(engineData), (unsigned char*)&engineData, directory);
    if (sz != sizeof(engineData))
    {
        logToConsole(L"Failed to load oracle engine data!");
        return false;
    }

    queryStorageBytesUsed = engineData.queryStorageBytesUsed;
    oracleQueryCount = engineData.oracleQueryCount;
    contractQueryIdState.tick = engineData.contractQueryIdStateTick;
    contractQueryIdState.queryIndexInTick = engineData.contractQueryIdStateQueryIndexInTick;
    replyStatesIndex = engineData.replyStatesIndex;
    copyMemory(pendingQueryIndices, engineData.pendingQueryIndices);
    copyMemory(pendingCommitReplyStateIndices, engineData.pendingCommitReplyStateIndices);
    copyMemory(pendingRevealReplyStateIndices, engineData.pendingRevealReplyStateIndices);
    copyMemory(notificationQueryIndicies, engineData.notificationQueryIndicies);
    copyMemory(stats, engineData.stats);


    logToConsole(L"Loading oracle query metadata ...");
    unsigned long long sizeToLoad = sizeof(*queries) * oracleQueryCount;
    sz = loadLargeFile(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeToLoad, (unsigned char*)queries, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle query metadata!");
        return false;
    }

    logToConsole(L"Loading oracle query data storage ...");
    sizeToLoad = queryStorageBytesUsed;
    // TODO: only save parts that were added after previous snapshot
    sz = loadLargeFile(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME, sizeToLoad, queryStorage, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle query data storage!");
        return false;
    }

    logToConsole(L"Loading oracle reply states ...");
    sizeToLoad = sizeof(ReplyState) * MAX_SIMULTANEOUS_ORACLE_QUERIES;
    sz = loadLargeFile(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeToLoad, (unsigned char*)replyStates, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle reply states!");
        return false;
    }

    // init queryIdToIndex (not saved to file)
    queryIdToIndex->reset();
    for (uint32_t queryIndex = 0; queryIndex < oracleQueryCount; ++queryIndex)
        queryIdToIndex->set(queries[queryIndex].queryId, queryIndex);

    logToConsole(L"Successfully loaded all oracle engine data from snapshot!");
}

#else

template <uint16_t ownComputorSeedsCount>
bool OracleEngine<ownComputorSeedsCount>::saveSnapshot(unsigned short epoch, CHAR16* directory) const
{
}

template <uint16_t ownComputorSeedsCount>
bool OracleEngine<ownComputorSeedsCount>::loadSnapshot(unsigned short epoch, CHAR16* directory)
{
}

#endif
