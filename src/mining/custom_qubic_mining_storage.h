#pragma once

#include <cstdint>

#include "platform/memory.h"
#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "platform/m256.h"
#include "oracle_core/oracle_transactions.h"
#include "oracle_core/oracle_interfaces_def.h"
#include "network_messages/custom_mining.h"
#include "contract_core/qpi_hash_map_impl.h"
#include "kangaroo_twelve.h"


class CustomQubicMiningStorage
{
public:
    static constexpr unsigned int maxNumTasks = 32;

    // A struct for storing an active doge mining task on the node.
    struct StoredDogeMiningTask
    {
        uint8_t dispatcherTarget[32]; // dispatcher target, usually easier than pool and network difficulty, full 32-byte representation

        // Full header can be constructed via concatenating version + prevHash + merkleRoot + miner's nTime + nBits + miner's nonce.
        uint8_t version[4]; // 4 bytes version
        uint8_t prevHash[32]; // 32 bytes prevBlockHash
        uint8_t nBits[4]; // 4 bytes network difficulty (nBits)
    };

    enum OracleQueryStatus : uint8_t
    {
        SCHEDULED,
        STARTED,
    };

    struct OracleQueryInfo
    {
        unsigned int tick;
        OracleQueryStatus status;
        m256i sourcePublicKey;
        unsigned int numTries;
    };

    bool init();

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

    // Return true if there is an active task with the given jobId and customMiningType, false otherwise.
    bool containsTask(uint8_t customMiningType, uint64_t jobId);

    bool addOracleQuery(const OracleUserQueryTransactionPrefix* queryTx);

    // Returns true if the fail counter for this query could be increased without hitting maxNumTries. Returns false if the query is not found
    // or the counter has hit maxNumTries. A query that has hit maxNumTries is removed from storage.
    bool increaseOracleQueryFailCounter(const OracleUserQueryTransactionPrefix* queryTx);

    bool updateOracleQueryScheduledTick(const OracleUserQueryTransactionPrefix* queryTx, unsigned int newScheduledTick);

    bool removeOracleQuery(const OracleUserQueryTransactionPrefix* queryTx);

    bool markOracleQueryStarted(const OracleUserQueryTransactionPrefix* queryTx);

    void resendNotStartedOracleQueriesForTick(unsigned int tick, unsigned long long compSeedsCount, const m256i* compSubseeds, const m256i* compPubKeys);

private:
    static constexpr unsigned int maxNumSolutionsPerTask = 128;
    static constexpr unsigned int oracleQueryMaxNumTries = 5;

    // General storage that is the same for all mining types.
    uint64_t activeTasks[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks];
    unsigned int nextTaskIndex[CustomMiningType::TOTAL_NUM_TYPES];
    OracleQueryInfo oracleQueries[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks * maxNumSolutionsPerTask];

    // For each task, we store a set of received solution hashes to prevent duplicate solutions.
    // Two-dimensional array [CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks] indexed by mining type and type-specific task index.
    QPI::HashSet<m256i, maxNumSolutionsPerTask>* receivedSolutions;
    static constexpr unsigned long long receivedSolutionsSize = CustomMiningType::TOTAL_NUM_TYPES * maxNumTasks * sizeof(QPI::HashSet<m256i, maxNumSolutionsPerTask>);

    // Storage for type-specific task descriptions.
    StoredDogeMiningTask dogeTasks[maxNumTasks];

    // Storage for type-specific oracle queries.
    OI::DogeShareValidation::OracleQuery dogeOracleQueries[maxNumTasks * maxNumSolutionsPerTask];

    inline static volatile char lock = 0;

    // Converts the compact representation of the target (4 bytes) to the full 32-byte representation, and writes it to the provided output pointer.
    // Expected input byte order: bytes 0 - 2 mantissa in little endian, byte 3 exponent.
    // Output byte order: little endian, i.e. least-significant byte in index 0.
    static void convertTargetCompactToFull(const uint8_t* compact, uint8_t* full);

    // Return the mining-type-specific index of the task with the given jobId and customMiningType, or -1 if not found.
    // The returned index can then be used to access the type-specific task descriptions and the solution hash set for this task.
    int findTask(uint8_t customMiningType, uint64_t jobId) const;

    // Return the mining-type-specific index of the oracle query, or -1 if not found.
    int findOracleQuery(uint8_t customMiningType, const OracleUserQueryTransactionPrefix* queryTx) const;

    bool isQueryEqual(uint8_t customMiningType, unsigned int queryIndex, const char* typeSpecificOracleQuery) const;

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

int CustomQubicMiningStorage::findTask(uint8_t customMiningType, uint64_t jobId) const
{
    if (customMiningType >= CustomMiningType::TOTAL_NUM_TYPES)
        return -1;

    for (unsigned int i = 0; i < maxNumTasks; ++i)
    {
        if (activeTasks[customMiningType][i] == jobId)
            return i;
    }
    return -1;
}

int CustomQubicMiningStorage::findOracleQuery(uint8_t customMiningType, const OracleUserQueryTransactionPrefix* queryTx) const
{
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

bool CustomQubicMiningStorage::isQueryEqual(uint8_t customMiningType, unsigned int queryIndex, const char* typeSpecificOracleQuery) const
{
    if (customMiningType == CustomMiningType::DOGE)
    {
        const auto* dogeQuery = reinterpret_cast<const OI::DogeShareValidation::OracleQuery*>(typeSpecificOracleQuery);
        for (int i = 0; i < 4; ++i)
        {
            if (dogeQuery->solutionNonce.get(i) != dogeOracleQueries[queryIndex].solutionNonce.get(i))
                return false;
        }
        for (int i = 0; i < 32; ++i)
        {
            if (dogeQuery->taskPartialHeaderPrevBlockHash.get(i) != dogeOracleQueries[queryIndex].taskPartialHeaderPrevBlockHash.get(i))
                return false;
        }
        return true;
    }

    return false;
}

void CustomQubicMiningStorage::removeOracleQuery(uint8_t customMiningType, unsigned int queryIndex)
{
    setMem(&oracleQueries[customMiningType][queryIndex], sizeof(OracleQueryInfo), 0);

    if (customMiningType == CustomMiningType::DOGE)
        setMem(&dogeOracleQueries[queryIndex], sizeof(OI::DogeShareValidation::OracleQuery), 0);
}

// --- public methods ---

bool CustomQubicMiningStorage::init()
{
    ASSERT(lock == 0);

    if (!allocPoolWithErrorLog(L"CustomQubicMiningStorage::receivedSolutions ", receivedSolutionsSize, (void**)&receivedSolutions, __LINE__))
        return false;

    setMem(activeTasks, sizeof(activeTasks), 0);
    setMem(nextTaskIndex, sizeof(nextTaskIndex), 0);
    setMem(dogeTasks, sizeof(dogeTasks), 0);
    setMem(oracleQueries, sizeof(oracleQueries), 0);
    setMem(dogeOracleQueries, sizeof(dogeOracleQueries), 0);

    return true;
}

void CustomQubicMiningStorage::deinit()
{
    if (receivedSolutions)
        freePool(receivedSolutions);
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
            if (size < sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask))
                return false;
            unsigned int& nextDogeTaskId = nextTaskIndex[CustomMiningType::DOGE];
            const QubicDogeMiningTask* dogeTask = reinterpret_cast<const QubicDogeMiningTask*>(reinterpret_cast<const char*>(task) + sizeof(CustomQubicMiningTask));
            if (dogeTask->cleanJobQueue)
            {
                setMem(activeTasks[CustomMiningType::DOGE], maxNumTasks * sizeof(uint64_t), 0);
                for (int t = 0; t < maxNumTasks; ++t)
                    receivedSolutions[CustomMiningType::DOGE * maxNumTasks + t].reset();
                setMem(dogeTasks, sizeof(dogeTasks), 0);
                nextDogeTaskId = 0;
            }
            else
            {
                // If not cleaning job queue, we will override the oldest task. Clean the corresponding solution hash set.
                receivedSolutions[CustomMiningType::DOGE * maxNumTasks + nextDogeTaskId].reset();
            }
            convertTargetCompactToFull(dogeTask->dispatcherDifficulty, dogeTasks[nextDogeTaskId].dispatcherTarget);
            copyMem(dogeTasks[nextDogeTaskId].nBits, dogeTask->nBits, 4);
            copyMem(dogeTasks[nextDogeTaskId].version, dogeTask->version, 4);
            copyMem(dogeTasks[nextDogeTaskId].prevHash, dogeTask->prevHash, 32);
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

bool CustomQubicMiningStorage::containsTask(uint8_t customMiningType, uint64_t jobId)
{
    LockGuard guard(lock);
    return findTask(customMiningType, jobId) >= 0;
}

bool CustomQubicMiningStorage::addOracleQuery(const OracleUserQueryTransactionPrefix* queryTx)
{
    LockGuard guard(lock);

    if (queryTx->oracleInterfaceIndex == OI::DogeShareValidation::oracleInterfaceIndex)
    {
        for (unsigned int i = 0; i < maxNumTasks * maxNumSolutionsPerTask; ++i)
        {
            if (oracleQueries[CustomMiningType::DOGE][i].tick == 0) // empty slot
            {
                oracleQueries[CustomMiningType::DOGE][i].tick = queryTx->tick;
                oracleQueries[CustomMiningType::DOGE][i].status = SCHEDULED;
                oracleQueries[CustomMiningType::DOGE][i].sourcePublicKey = queryTx->sourcePublicKey;
                oracleQueries[CustomMiningType::DOGE][i].numTries = 0;
                copyMem(&dogeOracleQueries[i], reinterpret_cast<const char*>(queryTx) + sizeof(OracleUserQueryTransactionPrefix), sizeof(OI::DogeShareValidation::OracleQuery));
                return true;
            }
        }
    }

    return false; // all slots full or unsupported oracle interface
}

bool CustomQubicMiningStorage::increaseOracleQueryFailCounter(const OracleUserQueryTransactionPrefix* queryTx)
{
    uint8_t customMiningType;
    if (queryTx->oracleInterfaceIndex == OI::DogeShareValidation::oracleInterfaceIndex)
        customMiningType = CustomMiningType::DOGE;
    else
        return false; // unsupported oracle interface

    LockGuard guard(lock);

    int queryIndex = findOracleQuery(customMiningType, queryTx);
    if (queryIndex >= 0)
    {
        oracleQueries[customMiningType][queryIndex].numTries++;
        if (oracleQueries[customMiningType][queryIndex].numTries >= oracleQueryMaxNumTries)
        {
            removeOracleQuery(customMiningType, queryIndex);
            return false;
        }
        else
            return true;
    }

    return false;
}


bool CustomQubicMiningStorage::updateOracleQueryScheduledTick(const OracleUserQueryTransactionPrefix* queryTx, unsigned int newScheduledTick)
{
    // TODO: implement this
    return false;
}

bool CustomQubicMiningStorage::removeOracleQuery(const OracleUserQueryTransactionPrefix* queryTx)
{
    uint8_t customMiningType;
    if (queryTx->oracleInterfaceIndex == OI::DogeShareValidation::oracleInterfaceIndex)
        customMiningType = CustomMiningType::DOGE;
    else
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

bool CustomQubicMiningStorage::markOracleQueryStarted(const OracleUserQueryTransactionPrefix* queryTx)
{
    // TODO: implement this
    return false;
}

void CustomQubicMiningStorage::resendNotStartedOracleQueriesForTick(unsigned int tick, unsigned long long compSeedsCount, const m256i* compSubseeds, const m256i* compPubKeys)
{
    // TODO: implement this
}
