#pragma once

#include <cstdint>

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "network_messages/custom_mining.h"
#include "contracts/qpi.h"
#include <kangaroo_twelve.h>

// A struct for storing an active doge mining task on the node.
struct StoredDogeMiningTask
{
    uint8_t dispatcherDifficulty[4]; // dispatcher difficulty, usually lower than pool and network difficulty, same compact format

    // Full header can be constructed via concatenating partialHeader1 + merkleRoot + miner's nTime + nBits + miner's nonce.
    uint8_t partialHeader[36]; // 4 bytes version, 32 bytes prevBlockHash
    uint8_t nBits[4]; // 4 bytes network difficulty (nBits)
};

class CustomQubicMiningStorage
{
private:
    static constexpr unsigned int maxNumTasks = 32;
    static constexpr unsigned int maxNumSolutionsPerTask = 128;

    uint64_t activeTasks[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks];
    unsigned int nextTaskIndex[CustomMiningType::TOTAL_NUM_TYPES];

    // For each task, we store a set of received solution hashes to prevent duplicate solutions.
    // Two-dimensional array [CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks] indexed by mining type and type-specific task index.
    QPI::HashSet<m256i, maxNumSolutionsPerTask>* receivedSolutions;
    static constexpr unsigned long long receivedSolutionsSize = CustomMiningType::TOTAL_NUM_TYPES * maxNumTasks * sizeof(QPI::HashSet<m256i, maxNumSolutionsPerTask>);

    // Storage for type-specific task descriptions.
    StoredDogeMiningTask dogeTasks[maxNumTasks];

    inline static volatile char lock = 0;

    // Return the mining-type-specific index of the task with the given jobId and customMiningType, or -1 if not found.
    // The returned index can then be used to access the type-specific task descriptions and the solution hash set for this task.
    int findTask(uint8_t customMiningType, uint64_t jobId)
    {
        for (unsigned int i = 0; i < maxNumTasks; ++i)
        {
            if (activeTasks[customMiningType][i] == jobId)
                return i;
        }
        return -1;
    }

public:
    bool init()
    {
        ASSERT(lock == 0);

        if (!allocPoolWithErrorLog(L"CustomQubicMiningStorage::receivedSolutions ", receivedSolutionsSize, (void**)&receivedSolutions, __LINE__))
            return false;

        setMem(activeTasks, sizeof(activeTasks), 0);
        setMem(nextTaskIndex, sizeof(nextTaskIndex), 0);
        setMem(dogeTasks, sizeof(dogeTasks), 0);

        return true;
    }

    void deinit()
    {
        if (receivedSolutions)
            freePool(receivedSolutions);
    }

    bool addTask(const CustomQubicMiningTask* task, unsigned int size)
    {
        LockGuard guard(lock);

        if (findTask(task->customMiningType, task->jobId) >= 0)
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
                copyMem(dogeTasks[nextDogeTaskId].dispatcherDifficulty, dogeTask->dispatcherDifficulty, 4);
                copyMem(dogeTasks[nextDogeTaskId].nBits, dogeTask->nBits, 4);
                copyMem(dogeTasks[nextDogeTaskId].partialHeader, dogeTask->version, 4);
                copyMem(dogeTasks[nextDogeTaskId].partialHeader + 4, dogeTask->prevHash, 32);
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

    // Return -1 if the solution is invalid (stale or duplicate), 0 if the solution is valid but not added due to storage limit,
    // and 1 if the solution is valid and added successfully. If taskDescription is not nullptr and the solution corresponds to an active task,
    // the task description is written into the provided pointer.
    int addSolution(const CustomQubicMiningSolution* solution, unsigned int size, char* taskDescription = nullptr)
    {
        if (size < sizeof(CustomQubicMiningSolution))
            return -1;

        LockGuard guard(lock);
        
        // Check if the solution corresponds to an active task.
        unsigned int typeSpecificTaskIndex = findTask(solution->customMiningType, solution->jobId);
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

    bool containsTask(uint8_t customMiningType, uint64_t jobId)
    {
        LockGuard guard(lock);
        return findTask(customMiningType, jobId) >= 0;
    }
};
