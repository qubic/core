#pragma once

#include "lib/platform_common/qstdint.h"

#include "platform/memory.h"
#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "platform/m256.h"
#include "oracle_core/oracle_transactions.h"
#include "oracle_core/oracle_interfaces_def.h"
#include "network_messages/custom_mining.h"
#include "contract_core/qpi_hash_map_impl.h"
#include "oracle_core/oracle_interfaces_def.h"
#include "kangaroo_twelve.h"


class CustomQubicMiningStorage
{
public:
    static constexpr unsigned int scheduleOracleQueryTickOffset = 8; // offset of 8 ticks to ensure propagation through the network

    static constexpr unsigned int maxNumTasks = 16;

    // A struct for storing an active doge mining task on the node.
    struct StoredDogeMiningTask
    {
        uint64_t jobId;

        uint8_t dispatcherTarget[32]; // dispatcher target, usually easier than pool and network difficulty, full 32-byte representation

        uint8_t version[4]; // 4 bytes version
        uint8_t nBits[4]; // 4 bytes network difficulty (nBits)
        uint8_t prevHash[32]; // 32 bytes prevBlockHash
        uint32_t extraNonce1NumBytes;
        uint32_t coinbase1NumBytes;
        uint32_t coinbase2NumBytes;
        uint32_t numMerkleBranches;

        uint8_t additionalData[OI::DogeShareValidation::OracleQuery::additionalDataSize];
        // Layout for additional data:
        // - extraNonce1
        // - coinbase1
        // - coinbase2
        // - merkleBranch1NumBytes (unsigned int), ... , merkleBranchNNumBytes (unsigned int)
        // - merkleBranch1, ... , merkleBranchN
        // Note: The size of the components contained in the additional data varies, hence the total occupied bytes in the array is not fixed.
    };

    enum OracleQueryStatus : uint8_t
    {
        SCHEDULED, // the query transaction is scheduled to be included in a future tick
        STARTED, // the query transaction was included in a tick and has successfully been started
    };

    // A general struct to store oracle query info, shared across all custom mining types.
    // The type-specific oracle query data as defined by the oracle interface is stored separately in type-specific storage.
    struct OracleQueryInfo
    {
        unsigned int tick;
        OracleQueryStatus status;
        int64_t queryId;
        m256i sourcePublicKey;
        unsigned int numTries;

        static OracleQueryInfo makeDefault()
        {
            return OracleQueryInfo
            {
                .tick = 0,
                .status = SCHEDULED,
                .queryId = -1,
                .sourcePublicKey = m256i::zero(),
                .numTries = 0
            };
        }
    };

    // Initialize the storage. Return true if successful, false if initialization failed (e.g. due to memory allocation failure).
    bool init();

    // Deinitialize the storage and free any allocated memory.
    void deinit();

    // Add a new mining task. Return false if a task with the same jobId and customMiningType already exists, true if added successfully.
    // The task description is expected to be behind the CustomQubicMiningTask struct in memory. The provided size should specify the
    // full size of the CustomQubicMiningTask struct and the task description.
    // For DOGE mining: Adding a task with cleanJobQueue = true will clear all existing tasks and solutions before adding the new task. 
    //                  If maxNumTasks is reached and a job with cleanJobQueue = false is added, it will override the oldest task and its solutions.
    bool addTask(const CustomQubicMiningTask* task, unsigned int size);

    // Return -1 if the solution is invalid (stale or duplicate), 0 if the solution is valid but not added due to storage limit,
    // and 1 if the solution is valid and added successfully. If taskDescription is not nullptr and the solution corresponds to an active task,
    // the task description is written into the provided pointer.
    int addSolution(const CustomQubicMiningSolution* solution, unsigned int size, unsigned char* taskDescription = nullptr);

    // Return true if the solution belongs to an active task and has not been counted for revenue points before, false otherwise.
    bool countSolutionForRevenue(uint8_t customMiningType, uint64_t jobId, const m256i& solutionHash);

    // Return true if there is an active task with the given jobId and customMiningType, false otherwise.
    bool containsTask(uint8_t customMiningType, uint64_t jobId);

    // Add a new oracle query. Return true if added successfully or the same query already exists. Return false if the storage is full
    // or the oracle interface is unrecognized.
    bool addOracleQuery(const OracleUserQueryTransactionPrefix* queryTx);

    // Returns true if the fail counter for this query could be increased without hitting maxNumTries. In this case, the query's tick is updated
    // to newScheduledTick, the status is set to SCHEDULED and the queryId is reset to -1. Returns false if the query is not found or the counter
    // has hit maxNumTries. A query that has hit maxNumTries is removed from storage.
    bool increaseOracleQueryFailCounterAndReschedule(unsigned int oracleInterfaceIndex, int64_t queryId, unsigned int newScheduledTick);

    // Remove the given oracle query from storage. Return true if removed successfully, false if the query is not found.
    bool removeOracleQuery(const OracleUserQueryTransactionPrefix* queryTx);

    // Remove the given oracle query from storage. Return true if removed successfully, false if the query is not found.
    // This method only works for queries with status STARTED that already have a valid queryId.
    bool removeOracleQuery(unsigned int oracleInterfaceIndex, int64_t queryId);

    // Mark the given oracle query as started and add the queryId. Return true if marked successfully, false if the query is not found.
    bool markOracleQueryStarted(const OracleUserQueryTransactionPrefix* queryTx, int64_t queryId);

    // Return the index of the next oracle query with status SCHEDULED following currentQueryIndex for the given mining type and tick.
    // Use currentQueryIndex = -1 to get the first SCHEDULED query index for the tick. Return -1 if no more query is found.
    int getNextScheduledQueryIndexForTick(uint8_t customMiningType, int currentQueryIndex, unsigned int tick);

    // Return the OracleQueryInfo for the given mining type and oracle query index.
    OracleQueryInfo getOracleQueryInfo(uint8_t customMiningType, unsigned int queryIndex);

    // Update the scheduled tick for the given oracle query. Return true if updated successfully, false if the query is not found.
    bool updateOracleQueryScheduledTick(uint8_t customMiningType, unsigned int queryIndex, unsigned int newScheduledTick);

    // Return true if the type-specific oracle query data for the given mining type and oracle query index is written to the provided output pointer,
    // false if the query is not found.
    bool getTypeSpecificOracleQuery(uint8_t customMiningType, unsigned int queryIndex, unsigned char* queryData);

private:
    static constexpr unsigned int maxNumSolutionsPerTask = 256;
    static constexpr unsigned int oracleQueryMaxNumTries = 3;

    // General storage that is the same for all mining types.
    uint64_t activeTasks[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks];
    unsigned int nextTaskIndex[CustomMiningType::TOTAL_NUM_TYPES];
    OracleQueryInfo oracleQueries[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks * maxNumSolutionsPerTask];

    // For each task, we store a set of received solution hashes to prevent sending oracle queries for duplicate solutions.
    // Currently, only own solutions (i.e. from any comp index running on the node) are stored.
    // Two-dimensional array [CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks] indexed by mining type and type-specific task index.
    QPI::HashSet<m256i, maxNumSolutionsPerTask>* receivedSolutions;
    static constexpr unsigned long long receivedSolutionsSize = CustomMiningType::TOTAL_NUM_TYPES * maxNumTasks * sizeof(QPI::HashSet<m256i, maxNumSolutionsPerTask>);

    // For each task, we store a set of solution hashes that were already counted for revenue points to prevent counting duplicates.
    // Any solution from any computor that received a valid oracle reply is stored.
    // Two-dimensional array [CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks] indexed by mining type and type-specific task index.
    QPI::HashSet<m256i, maxNumSolutionsPerTask>* countedRevSolutions;
    static constexpr unsigned long long countedRevSolutionsSize = CustomMiningType::TOTAL_NUM_TYPES * maxNumTasks * sizeof(QPI::HashSet<m256i, maxNumSolutionsPerTask>);

    // Storage for type-specific task descriptions.
    StoredDogeMiningTask dogeTasks[maxNumTasks];

    // Storage for type-specific oracle queries. Linear array of length maxNumTasks * maxNumSolutionsPerTask.
    OI::DogeShareValidation::OracleQuery* dogeOracleQueries;
    static constexpr unsigned long long dogeOracleQueriesSize = maxNumTasks * maxNumSolutionsPerTask * sizeof(OI::DogeShareValidation::OracleQuery);

    inline static volatile char lock = 0;

    // Converts the compact representation of the target (4 bytes) to the full 32-byte representation, and writes it to the provided output pointer.
    // Expected input byte order: bytes 0 - 2 mantissa in little endian, byte 3 exponent.
    // Output byte order: little endian, i.e. least-significant byte in index 0.
    static void convertTargetCompactToFull(const uint8_t* compact, uint8_t* full);

    // Map the oracle interface index to the corresponding custom mining type. Returns an invalid mining type if the oracle interface index is not recognized.
    static uint8_t oracleInterfaceIndexToCustomMiningType(unsigned int oracleInterfaceIndex);

    // Return the mining-type-specific index of the task with the given jobId and customMiningType, or -1 if not found.
    // The returned index can then be used to access the type-specific task descriptions and the solution hash set for this task.
    // Assumes the caller holds the lock and has verified valid range for customMiningType.
    int findTask(uint8_t customMiningType, uint64_t jobId) const;

    // Return the mining-type-specific index of the oracle query, or -1 if not found.
    // Assumes the caller holds the lock and has verified valid range for customMiningType.
    int findOracleQuery(uint8_t customMiningType, const OracleUserQueryTransactionPrefix* queryTx) const;

    // Return the mining-type-specific index of the oracle query, or -1 if not found.
    // Assumes the caller holds the lock and has verified valid range for customMiningType.
    // This method only works for queries with status STARTED that already have a valid queryId.
    int findOracleQuery(uint8_t customMiningType, int64_t queryId) const;

    // Return true if the type-specific oracle query matches the stored query at the given index, false otherwise.
    // Assumes the caller holds the lock and has verified valid ranges for customMiningType and queryIndex.
    bool isQueryEqual(uint8_t customMiningType, unsigned int queryIndex, const char* typeSpecificOracleQuery) const;

    // Remove the oracle query at the given index from storage.
    // Assumes the caller holds the lock and has verified valid ranges for customMiningType and queryIndex.
    void removeOracleQuery(uint8_t customMiningType, unsigned int queryIndex);
};

// --- Implementation follows below ---

// --- private methods ---

void CustomQubicMiningStorage::convertTargetCompactToFull(const uint8_t* compact, uint8_t* full)
{
    setMem(full, 32, 0);

    uint8_t exponent = compact[3];

    // The target is mantissa * 256^(exponent - 3).
    // This means the mantissa starts at byte index (exponent - 3).
    int start_index = exponent - 3;

    for (int i = 0; i < 3; ++i)
    {
        int target_idx = start_index + i;
        if (target_idx >= 0 && target_idx < 32)
        {
            full[target_idx] = compact[i];
        }
    }
}

uint8_t CustomQubicMiningStorage::oracleInterfaceIndexToCustomMiningType(unsigned int oracleInterfaceIndex)
{
    if (oracleInterfaceIndex == OI::DogeShareValidation::oracleInterfaceIndex)
        return CustomMiningType::DOGE;
    // Add more mappings for other oracle interfaces when needed.
    // Return an invalid mining type if the oracle interface index is not recognized.
    return CustomMiningType::TOTAL_NUM_TYPES;
}

int CustomQubicMiningStorage::findTask(uint8_t customMiningType, uint64_t jobId) const
{
    ASSERT(customMiningType < CustomMiningType::TOTAL_NUM_TYPES);

    for (unsigned int i = 0; i < maxNumTasks; ++i)
    {
        if (activeTasks[customMiningType][i] == jobId)
            return i;
    }
    return -1;
}

int CustomQubicMiningStorage::findOracleQuery(uint8_t customMiningType, const OracleUserQueryTransactionPrefix* queryTx) const
{
    ASSERT(customMiningType < CustomMiningType::TOTAL_NUM_TYPES);

    for (unsigned int i = 0; i < maxNumTasks * maxNumSolutionsPerTask; ++i)
    {
        if (oracleQueries[customMiningType][i].tick == queryTx->tick &&
            oracleQueries[customMiningType][i].sourcePublicKey == queryTx->sourcePublicKey)
        {
            if (isQueryEqual(customMiningType, i, reinterpret_cast<const char*>(queryTx) + sizeof(OracleUserQueryTransactionPrefix)))
            {
                return i;
            }
        }
    }
    return -1;
}

int CustomQubicMiningStorage::findOracleQuery(uint8_t customMiningType, int64_t queryId) const
{
    ASSERT(customMiningType < CustomMiningType::TOTAL_NUM_TYPES);

    for (unsigned int i = 0; i < maxNumTasks * maxNumSolutionsPerTask; ++i)
    {
        if (oracleQueries[customMiningType][i].status == STARTED
            && oracleQueries[customMiningType][i].queryId == queryId)
        {
            return i;
        }
    }
    return -1;
}

bool CustomQubicMiningStorage::isQueryEqual(uint8_t customMiningType, unsigned int queryIndex, const char* typeSpecificOracleQuery) const
{
    if (customMiningType == CustomMiningType::DOGE)
    {
        // A doge query is treated as equal if the jobId and the solution (nonce, extraNonce2, time) are all the same.
        const auto* dogeQuery = reinterpret_cast<const OI::DogeShareValidation::OracleQuery*>(typeSpecificOracleQuery);
        if (dogeQuery->jobId != dogeOracleQueries[queryIndex].jobId)
            return false;
        for (int i = 0; i < 4; ++i)
        {
            if (dogeQuery->solutionNonce.get(i) != dogeOracleQueries[queryIndex].solutionNonce.get(i))
                return false;
            if (dogeQuery->solutionTime.get(i) != dogeOracleQueries[queryIndex].solutionTime.get(i))
                return false;
        }
        for (int i = 0; i < 8; ++i)
        {
            if (dogeQuery->solutionExtraNonce2.get(i) != dogeOracleQueries[queryIndex].solutionExtraNonce2.get(i))
                return false;
        }
        return true;
    }

    return false;
}

void CustomQubicMiningStorage::removeOracleQuery(uint8_t customMiningType, unsigned int queryIndex)
{
    ASSERT(customMiningType < CustomMiningType::TOTAL_NUM_TYPES);
    ASSERT(queryIndex < maxNumTasks * maxNumSolutionsPerTask);

    setMem(&oracleQueries[customMiningType][queryIndex], sizeof(OracleQueryInfo), 0);

    if (customMiningType == CustomMiningType::DOGE)
        setMem(&dogeOracleQueries[queryIndex], sizeof(OI::DogeShareValidation::OracleQuery), 0);
}

// --- public methods ---

bool CustomQubicMiningStorage::init()
{
    setMem(activeTasks, sizeof(activeTasks), 0);
    setMem(nextTaskIndex, sizeof(nextTaskIndex), 0);
    setMem(oracleQueries, sizeof(oracleQueries), 0);
    setMem(dogeTasks, sizeof(dogeTasks), 0);

    if (!allocPoolWithErrorLog(L"CustomQubicMiningStorage::receivedSolutions ", receivedSolutionsSize, (void**)&receivedSolutions, __LINE__))
        return false;
    if (!allocPoolWithErrorLog(L"CustomQubicMiningStorage::countedRevSolutions ", countedRevSolutionsSize, (void**)&countedRevSolutions, __LINE__))
        return false;
    if (!allocPoolWithErrorLog(L"CustomQubicMiningStorage::dogeOracleQueries ", dogeOracleQueriesSize, (void**)&dogeOracleQueries, __LINE__))
        return false;

    ASSERT(lock == 0);

    return true;
}

void CustomQubicMiningStorage::deinit()
{
    if (receivedSolutions)
        freePool(receivedSolutions);
    if (countedRevSolutions)
        freePool(countedRevSolutions);
    if (dogeOracleQueries)
        freePool(dogeOracleQueries);
}

bool CustomQubicMiningStorage::addTask(const CustomQubicMiningTask* task, unsigned int size)
{
    LockGuard guard(lock);

    if (findTask(task->customMiningType, task->jobId) < 0)
    {
        unsigned int typeSpecificTaskIndex = 0;
        if (task->customMiningType == CustomMiningType::DOGE)
        {
            // Type-specific task info is stored behind general CustomQubicMiningTask struct.
            static constexpr unsigned int combinedTaskStructSize = sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask);
            if (size < combinedTaskStructSize)
                return false;
            unsigned int additionalDataSize = size - combinedTaskStructSize;
            if (additionalDataSize > OI::DogeShareValidation::OracleQuery::additionalDataSize)
                return false;

            const auto* taskAsCharPtr = reinterpret_cast<const char*>(task);

            unsigned int& nextDogeTaskId = nextTaskIndex[CustomMiningType::DOGE];
            const QubicDogeMiningTask* dogeTask = reinterpret_cast<const QubicDogeMiningTask*>(taskAsCharPtr + sizeof(CustomQubicMiningTask));

            // If maxNumTasks is already reached, we will override the oldest task. Clean the corresponding solution hash sets.
            receivedSolutions[CustomMiningType::DOGE * maxNumTasks + nextDogeTaskId].reset();
            countedRevSolutions[CustomMiningType::DOGE * maxNumTasks + nextDogeTaskId].reset();

            dogeTasks[nextDogeTaskId].jobId = task->jobId;
            convertTargetCompactToFull(dogeTask->dispatcherDifficulty, dogeTasks[nextDogeTaskId].dispatcherTarget);
            copyMem(dogeTasks[nextDogeTaskId].version, dogeTask->version, 4);
            copyMem(dogeTasks[nextDogeTaskId].nBits, dogeTask->nBits, 4);
            copyMem(dogeTasks[nextDogeTaskId].prevHash, dogeTask->prevHash, 32);
            dogeTasks[nextDogeTaskId].extraNonce1NumBytes = dogeTask->extraNonce1NumBytes;
            dogeTasks[nextDogeTaskId].coinbase1NumBytes = dogeTask->coinbase1NumBytes;
            dogeTasks[nextDogeTaskId].coinbase2NumBytes = dogeTask->coinbase2NumBytes;
            dogeTasks[nextDogeTaskId].numMerkleBranches = dogeTask->numMerkleBranches;

            if (additionalDataSize > 0)
                copyMem(dogeTasks[nextDogeTaskId].additionalData, taskAsCharPtr + combinedTaskStructSize, additionalDataSize);
            // Zero out the leftover tail.
            if (additionalDataSize < OI::DogeShareValidation::OracleQuery::additionalDataSize)
                setMem(dogeTasks[nextDogeTaskId].additionalData + additionalDataSize, OI::DogeShareValidation::OracleQuery::additionalDataSize - additionalDataSize, 0);

            typeSpecificTaskIndex = nextDogeTaskId;
            nextDogeTaskId = (nextDogeTaskId + 1) % maxNumTasks;
        }
        else
        {
            return false;
        }
        activeTasks[task->customMiningType][typeSpecificTaskIndex] = task->jobId;
        return true;
    }

    return false;
}

int CustomQubicMiningStorage::addSolution(const CustomQubicMiningSolution* solution, unsigned int size, unsigned char* taskDescription)
{
    if (size <= sizeof(CustomQubicMiningSolution))
        return -1;

    LockGuard guard(lock);

    // Check if the solution corresponds to an active task.
    int typeSpecificTaskIndex = findTask(solution->customMiningType, solution->jobId);
    if (typeSpecificTaskIndex < 0)
        return -1;

    if (taskDescription)
    {
        if (solution->customMiningType == CustomMiningType::DOGE)
        {
            StoredDogeMiningTask* taskOut = reinterpret_cast<StoredDogeMiningTask*>(taskDescription);
            *taskOut = dogeTasks[typeSpecificTaskIndex];
        }
    }

    // Check if the solution is duplicate.
    m256i digest;
    KangarooTwelve(reinterpret_cast<const char*>(solution) + sizeof(CustomQubicMiningSolution),
        size - sizeof(CustomQubicMiningSolution), &digest, sizeof(digest));

    if (receivedSolutions[solution->customMiningType * maxNumTasks + typeSpecificTaskIndex].contains(digest))
        return -1;

    // Try to add the solution hash to the set of received solutions for this task. May return NULL_INDEX if the set is full.
    QPI::sint64 indexAdded = receivedSolutions[solution->customMiningType * maxNumTasks + typeSpecificTaskIndex].add(digest);

    return (indexAdded == QPI::NULL_INDEX) ? 0 : 1;
}

bool CustomQubicMiningStorage::countSolutionForRevenue(uint8_t customMiningType, uint64_t jobId, const m256i& solutionHash)
{
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return false; // unsupported mining type

    LockGuard guard(lock);

    // Check if the solution corresponds to an active task.
    int typeSpecificTaskIndex = findTask(customMiningType, jobId);
    if (typeSpecificTaskIndex < 0)
        return false;

    if (countedRevSolutions[customMiningType * maxNumTasks + typeSpecificTaskIndex].contains(solutionHash))
        return false;

    // Try to add the solution hash to the hash set for this task. May return NULL_INDEX if the set is full.
    // If the set ever gets full, we have to increase maxNumSolutionsPerTask.
    ASSERT(countedRevSolutions[customMiningType * maxNumTasks + typeSpecificTaskIndex].population()
        < countedRevSolutions[customMiningType * maxNumTasks + typeSpecificTaskIndex].capacity());
    QPI::sint64 indexAdded = countedRevSolutions[customMiningType * maxNumTasks + typeSpecificTaskIndex].add(solutionHash);

    return (indexAdded != QPI::NULL_INDEX);
}

bool CustomQubicMiningStorage::containsTask(uint8_t customMiningType, uint64_t jobId)
{
    LockGuard guard(lock);
    return findTask(customMiningType, jobId) >= 0;
}

bool CustomQubicMiningStorage::addOracleQuery(const OracleUserQueryTransactionPrefix* queryTx)
{
    uint8_t customMiningType = oracleInterfaceIndexToCustomMiningType(queryTx->oracleInterfaceIndex);
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return false; // unsupported oracle interface

    LockGuard guard(lock);

    if (findOracleQuery(customMiningType, queryTx) >= 0) // check if the query already exists, if yes we don't add it again but return true
        return true;

    for (unsigned int i = 0; i < maxNumTasks * maxNumSolutionsPerTask; ++i)
    {
        if (oracleQueries[customMiningType][i].tick == 0) // empty slot
        {
            oracleQueries[customMiningType][i].tick = queryTx->tick;
            oracleQueries[customMiningType][i].status = SCHEDULED;
            oracleQueries[customMiningType][i].queryId = -1;
            oracleQueries[customMiningType][i].sourcePublicKey = queryTx->sourcePublicKey;
            oracleQueries[customMiningType][i].numTries = 0;

            if (queryTx->oracleInterfaceIndex == OI::DogeShareValidation::oracleInterfaceIndex)
            {
                copyMem(&dogeOracleQueries[i], reinterpret_cast<const char*>(queryTx) + sizeof(OracleUserQueryTransactionPrefix), sizeof(OI::DogeShareValidation::OracleQuery));
            }
            return true;
        }
    }

    return false; // all slots full
}

bool CustomQubicMiningStorage::increaseOracleQueryFailCounterAndReschedule(unsigned int oracleInterfaceIndex, int64_t queryId, unsigned int newScheduledTick)
{
    uint8_t customMiningType = oracleInterfaceIndexToCustomMiningType(oracleInterfaceIndex);
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return false; // unsupported oracle interface

    LockGuard guard(lock);

    int queryIndex = findOracleQuery(customMiningType, queryId);
    if (queryIndex >= 0)
    {
        oracleQueries[customMiningType][queryIndex].numTries++;
        if (oracleQueries[customMiningType][queryIndex].numTries >= oracleQueryMaxNumTries)
        {
            removeOracleQuery(customMiningType, queryIndex);
            return false;
        }
        else
        {
            oracleQueries[customMiningType][queryIndex].tick = newScheduledTick;
            oracleQueries[customMiningType][queryIndex].status = SCHEDULED;
            oracleQueries[customMiningType][queryIndex].queryId = -1;
            return true;
        }
    }

    return false;
}

bool CustomQubicMiningStorage::removeOracleQuery(const OracleUserQueryTransactionPrefix* queryTx)
{
    uint8_t customMiningType = oracleInterfaceIndexToCustomMiningType(queryTx->oracleInterfaceIndex);
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return false; // unsupported oracle interface

    LockGuard guard(lock);

    int queryIndex = findOracleQuery(customMiningType, queryTx);
    if (queryIndex >= 0)
    {
        removeOracleQuery(customMiningType, queryIndex);
        return true;
    }

    return false;
}

bool CustomQubicMiningStorage::removeOracleQuery(unsigned int oracleInterfaceIndex, int64_t queryId)
{
    uint8_t customMiningType = oracleInterfaceIndexToCustomMiningType(oracleInterfaceIndex);
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return false; // unsupported oracle interface

    LockGuard guard(lock);

    int queryIndex = findOracleQuery(customMiningType, queryId);
    if (queryIndex >= 0)
    {
        removeOracleQuery(customMiningType, queryIndex);
        return true;
    }

    return false;
}

bool CustomQubicMiningStorage::markOracleQueryStarted(const OracleUserQueryTransactionPrefix* queryTx, int64_t queryId)
{
    uint8_t customMiningType = oracleInterfaceIndexToCustomMiningType(queryTx->oracleInterfaceIndex);
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return false; // unsupported oracle interface

    LockGuard guard(lock);

    int queryIndex = findOracleQuery(customMiningType, queryTx);
    if (queryIndex >= 0)
    {
        oracleQueries[customMiningType][queryIndex].status = STARTED;
        oracleQueries[customMiningType][queryIndex].queryId = queryId;
        return true;
    }

    return false;
}

int CustomQubicMiningStorage::getNextScheduledQueryIndexForTick(uint8_t customMiningType, int currentQueryIndex, unsigned int tick)
{
    currentQueryIndex++; // start searching from the next index after currentQueryIndex

    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES || currentQueryIndex >= maxNumTasks * maxNumSolutionsPerTask)
        return -1;

    if (currentQueryIndex < 0) // in case someone passes currentQueryIndex < -1
        currentQueryIndex = 0;

    LockGuard guard(lock);

    for (int i = currentQueryIndex; i < maxNumTasks * maxNumSolutionsPerTask; ++i)
    {
        if (oracleQueries[customMiningType][i].tick == tick && oracleQueries[customMiningType][i].status == SCHEDULED)
        {
            return i;
        }
    }

    return -1;
}

CustomQubicMiningStorage::OracleQueryInfo CustomQubicMiningStorage::getOracleQueryInfo(uint8_t customMiningType, unsigned int queryIndex)
{
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES || queryIndex >= maxNumTasks * maxNumSolutionsPerTask)
        return OracleQueryInfo::makeDefault(); // return default OracleQueryInfo if invalid input

    LockGuard guard(lock);

    return oracleQueries[customMiningType][queryIndex];
}

bool CustomQubicMiningStorage::updateOracleQueryScheduledTick(uint8_t customMiningType, unsigned int queryIndex, unsigned int newScheduledTick)
{
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES || queryIndex >= maxNumTasks * maxNumSolutionsPerTask)
        return false;

    LockGuard guard(lock);

    oracleQueries[customMiningType][queryIndex].tick = newScheduledTick;
    oracleQueries[customMiningType][queryIndex].status = SCHEDULED;
    oracleQueries[customMiningType][queryIndex].queryId = -1;

    return true;
}

bool CustomQubicMiningStorage::getTypeSpecificOracleQuery(uint8_t customMiningType, unsigned int queryIndex, unsigned char* queryData)
{
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES || queryIndex >= maxNumTasks * maxNumSolutionsPerTask)
        return false;

    if (customMiningType == CustomMiningType::DOGE)
    {
        copyMem(queryData, &dogeOracleQueries[queryIndex], sizeof(OI::DogeShareValidation::OracleQuery));
        return true;
    }

    return false;
}
