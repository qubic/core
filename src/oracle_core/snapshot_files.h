#pragma once

#include "oracle_core/oracle_engine.h"
#include "network_messages/oracles.h"

#if TICK_STORAGE_AUTOSAVE_MODE

#include "platform/file_io.h"

static unsigned short ORACLE_SNAPSHOT_ENGINE_FILENAME[] = L"snapshotOracleEngine.???";
static unsigned short ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME[] = L"snapshotOracleQueryMetadata.???";
static unsigned short ORACLE_SNAPSHOT_QUERY_DATA_FILENAME[] = L"snapshotOracleQueryData.???";
static unsigned short ORACLE_SNAPSHOT_REPLY_STATES_FILENAME[] = L"snapshotOracleReplyStates.???";
static unsigned short ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME[] = L"snapshotOracleSubscriptions.???";
static unsigned short ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME[] = L"snapshotOracleSubscribers.???";


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
    MinHeap<uint32_t, MAX_SIMULTANEOUS_ORACLE_QUERIES> notificationQueryIndexQueue;
    uint64_t revenuePoints[NUMBER_OF_COMPUTORS];
    int32_t usedSubscriptionSlots;
    int32_t usedSubscriberSlots;
    OracleEngineStatistics stats;
};


// save state (excluding queryIdToIndex and unused parts of large buffers)
bool OracleEngine::saveSnapshot(unsigned short epoch, CHAR16* directory) const
{
    addEpochToFileName(ORACLE_SNAPSHOT_ENGINE_FILENAME, sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME) / sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME) / sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME, sizeof(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME) / sizeof(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME, sizeof(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME) / sizeof(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME[0]), epoch);

    LockGuard lockGuard(lock);

    OracleEngineSnapshotData engineData;
    engineData.queryStorageBytesUsed = queryStorageBytesUsed;
    engineData.oracleQueryCount = oracleQueryCount;
    engineData.contractQueryIdStateTick = contractQueryIdState.tick;
    engineData.contractQueryIdStateQueryIndexInTick = contractQueryIdState.queryIndexInTick;
    engineData.replyStatesIndex = replyStatesIndex;
    engineData.usedSubscriptionSlots = usedSubscriptionSlots;
    engineData.usedSubscriberSlots = usedSubscriberSlots;
    copyMemory(engineData.pendingQueryIndices, pendingQueryIndices);
    copyMemory(engineData.pendingCommitReplyStateIndices, pendingCommitReplyStateIndices);
    copyMemory(engineData.pendingRevealReplyStateIndices, pendingRevealReplyStateIndices);
    copyMemory(engineData.notificationQueryIndexQueue, notificationQueryIndexQueue);
    copyMemory(engineData.revenuePoints, revenuePoints);
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
    sizeToSave = sizeof(OracleReplyState) * MAX_SIMULTANEOUS_ORACLE_QUERIES;
    sz = saveLargeFile(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeToSave, (unsigned char*)replyStates, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save oracle reply states!");
        return false;
    }

    logToConsole(L"Saving oracle subscriptions ...");
    sizeToSave = sizeof(OracleSubscription) * MAX_ORACLE_SUBSCRIPTIONS;
    sz = saveLargeFile(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME, sizeToSave, (unsigned char*)subscriptions, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save oracle subscriptions!");
        return false;
    }

    logToConsole(L"Saving oracle subscribers ...");
    sizeToSave = sizeof(OracleSubscriber) * MAX_ORACLE_SUBSCRIBERS;
    sz = saveLargeFile(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME, sizeToSave, (unsigned char*)subscribers, directory);
    if (sz != sizeToSave)
    {
        logToConsole(L"Failed to save oracle subscribers!");
        return false;
    }

    logToConsole(L"Successfully saved all oracle engine data to snapshot!");

    return true;
}

bool OracleEngine::loadSnapshot(unsigned short epoch, CHAR16* directory)
{
    addEpochToFileName(ORACLE_SNAPSHOT_ENGINE_FILENAME, sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME) / sizeof(ORACLE_SNAPSHOT_ENGINE_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME, sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME) / sizeof(ORACLE_SNAPSHOT_QUERY_DATA_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME) / sizeof(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME, sizeof(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME) / sizeof(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME[0]), epoch);
    addEpochToFileName(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME, sizeof(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME) / sizeof(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME[0]), epoch);

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
    usedSubscriptionSlots = engineData.usedSubscriptionSlots;
    usedSubscriberSlots = engineData.usedSubscriberSlots;
    copyMemory(pendingQueryIndices, engineData.pendingQueryIndices);
    copyMemory(pendingCommitReplyStateIndices, engineData.pendingCommitReplyStateIndices);
    copyMemory(pendingRevealReplyStateIndices, engineData.pendingRevealReplyStateIndices);
    copyMemory(notificationQueryIndexQueue, engineData.notificationQueryIndexQueue);
    copyMemory(revenuePoints, engineData.revenuePoints);
    copyMemory(stats, engineData.stats);
    if (oracleQueryCount > MAX_ORACLE_QUERIES || queryStorageBytesUsed > ORACLE_QUERY_STORAGE_SIZE
        || replyStatesIndex >= MAX_SIMULTANEOUS_ORACLE_QUERIES
        || usedSubscriberSlots > MAX_ORACLE_SUBSCRIBERS || usedSubscriptionSlots > MAX_ORACLE_SUBSCRIPTIONS)
    {
        logToConsole(L"Oracle engine data is invalid!");
        return false;
    }


    logToConsole(L"Loading oracle query metadata ...");
    unsigned long long sizeToLoad = sizeof(*queries) * oracleQueryCount;
    sz = loadLargeFile(ORACLE_SNAPSHOT_QUERY_METADATA_FILENAME, sizeToLoad, (unsigned char*)queries, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle query metadata!");
        return false;
    }
    if (oracleQueryCount < MAX_ORACLE_QUERIES)
    {
        unsigned long long sizeToZero = (MAX_ORACLE_QUERIES - oracleQueryCount) * sizeof(*queries);
        setMem(queries + oracleQueryCount, sizeToZero, 0);
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
    if (queryStorageBytesUsed < ORACLE_QUERY_STORAGE_SIZE)
    {
        unsigned long long sizeToZero = ORACLE_QUERY_STORAGE_SIZE - queryStorageBytesUsed;
        setMem(queryStorage + queryStorageBytesUsed, sizeToZero, 0);
    }

    logToConsole(L"Loading oracle reply states ...");
    sizeToLoad = sizeof(OracleReplyState) * MAX_SIMULTANEOUS_ORACLE_QUERIES;
    sz = loadLargeFile(ORACLE_SNAPSHOT_REPLY_STATES_FILENAME, sizeToLoad, (unsigned char*)replyStates, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle reply states!");
        return false;
    }

    logToConsole(L"Loading oracle subscriptions ...");
    sizeToLoad = sizeof(OracleSubscription) * MAX_ORACLE_SUBSCRIPTIONS;
    sz = loadLargeFile(ORACLE_SNAPSHOT_SUBSCRIPTIONS_FILENAME, sizeToLoad, (unsigned char*)subscriptions, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle subscriptions!");
        return false;
    }

    logToConsole(L"Loading oracle subscribers ...");
    sizeToLoad = sizeof(OracleSubscriber) * MAX_ORACLE_SUBSCRIBERS;
    sz = loadLargeFile(ORACLE_SNAPSHOT_SUBSCRIBERS_FILENAME, sizeToLoad, (unsigned char*)subscribers, directory);
    if (sz != sizeToLoad)
    {
        logToConsole(L"Failed to load oracle subscribers!");
        return false;
    }

    // init queryIdToIndex (not saved to file)
    queryIdToIndex->reset();
    for (uint32_t queryIndex = 0; queryIndex < oracleQueryCount; ++queryIndex)
        queryIdToIndex->set(queries[queryIndex].queryId, queryIndex);

    logToConsole(L"Successfully loaded all oracle engine data from snapshot!");

    // init nextSubscriptionIdQueue (not saved to file)
    nextSubscriptionIdQueue.init(NextSubscriptionCompare{ subscriptions });
    for (int32_t subscriptionId = 0; subscriptionId < usedSubscriptionSlots; ++subscriptionId)
    {
        if (subscriptions[subscriptionId].nextQueryTimestamp.isValid())
            nextSubscriptionIdQueue.insert(subscriptionId);
    }

    return true;
}

#else

bool OracleEngine::saveSnapshot(unsigned short epoch, CHAR16* directory) const
{
    return false;
}

bool OracleEngine::loadSnapshot(unsigned short epoch, CHAR16* directory)
{
    return false;
}

#endif
