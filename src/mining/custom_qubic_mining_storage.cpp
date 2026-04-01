#include "custom_qubic_mining_storage.h"

#include <cstdint>

#include "platform/memory.h"
#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "oracle_core/oracle_transactions.h"
#include "oracle_core/oracle_interfaces_def.h"
#include "network_messages/custom_mining.h"
#include "contract_core/qpi_hash_map_impl.h"
#include "kangaroo_twelve.h"

namespace
{
    // Converts the compact representation of the target (4 bytes) to the full 32-byte representation, and writes it to the provided output pointer.
    // Expected input byte order: bytes 0 - 2 mantissa in little endian, byte 3 exponent.
    // Output byte order: little endian, i.e. least-significant byte in index 0.
    void convertTargetCompactToFull(const uint8_t* compact, uint8_t* full)
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
            if (dogeQuery->solutionNonce[i] != dogeOracleQueries[queryIndex].solutionNonce[i])
                return false;
        }
        for (int i = 0; i < 32; ++i)
        {
            if (dogeQuery->taskPartialHeaderPrevBlockHash[i] != dogeOracleQueries[queryIndex].taskPartialHeaderPrevBlockHash[i])
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

int CustomQubicMiningStorage::addSolution(const CustomQubicMiningSolution* solution, unsigned int size, unsigned char* taskDescription = nullptr)
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

