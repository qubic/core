#pragma once

#include <cstdint>

#include "platform/memory.h"
#include "network_messages/custom_mining.h"
#include "contracts/qpi.h"

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
    QPI::HashSet<m256i, maxNumSolutionsPerTask> receivedSolutions[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks];

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
    void init()
    {
        ASSERT(lock == 0);

        setMem(activeTasks, sizeof(activeTasks), 0);
        setMem(nextTaskIndex, sizeof(nextTaskIndex), 0);
        setMem(dogeTasks, sizeof(dogeTasks), 0);
    }

    bool addTask(const CustomQubicMiningTask* task, unsigned int size)
    {
        ACQUIRE(lock);
        if (findTask(task->customMiningType, task->jobId) >= 0)
        {
            unsigned int typeSpecificTaskIndex = 0;
            if (task->customMiningType == CustomMiningType::DOGE)
            {
                // Type-specific task info is stored behind general CustomQubicMiningTask struct.
                if (size < sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask))
                {
                    RELEASE(lock);
                    return false;
                }
                unsigned int& nextDogeTaskId = nextTaskIndex[CustomMiningType::DOGE];
                const QubicDogeMiningTask* dogeTask = reinterpret_cast<const QubicDogeMiningTask*>(reinterpret_cast<const char*>(task) + sizeof(CustomQubicMiningTask));
                if (dogeTask->cleanJobQueue)
                {
                    setMem(activeTasks[CustomMiningType::DOGE], maxNumTasks * sizeof(uint64_t), 0);
                    for (int t = 0; t < maxNumTasks; ++t)
                        receivedSolutions[CustomMiningType::DOGE][t].reset();
                    setMem(dogeTasks, sizeof(dogeTasks), 0);
                    nextDogeTaskId = 0;
                }
                else
                {
                    // If not cleaning job queue, we will override the oldest task. Clean the corresponding solution hash set.
                    receivedSolutions[CustomMiningType::DOGE][nextDogeTaskId].reset();
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
                RELEASE(lock);
                return false;
            }
            activeTasks[task->customMiningType][typeSpecificTaskIndex] = task->jobId;
            RELEASE(lock);
            return true;
        }

        RELEASE(lock);
        return false;
    }

    // Return -1 if the solution is invalid (stale or duplicate), 0 if the solution is valid but not added due to storage limit,
    // and 1 if the solution is valid and added successfully.
    int addSolution(const CustomQubicMiningSolution* solution, unsigned int size)
    {
        if (size < sizeof(CustomQubicMiningSolution))
            return -1;

        ACQUIRE(lock);
        
        // Check if the solution corresponds to an active task.
        unsigned int typeSpecificTaskIndex = findTask(solution->customMiningType, solution->jobId);
        if (typeSpecificTaskIndex < 0)
        {
            RELEASE(lock);
            return -1;
        }

        // Check if the solution is duplicate.
        m256i digest;
        KangarooTwelve(solution + sizeof(CustomQubicMiningSolution), size - sizeof(CustomQubicMiningSolution), &digest, sizeof(digest));

        if (receivedSolutions[solution->customMiningType][typeSpecificTaskIndex].contains(digest))
        {
            RELEASE(lock);
            return -1;
        }

        // Try to add the solution hash to the set of received solutions for this task. May return NULL_INDEX if the set is full.
        QPI::sint64 indexAdded = receivedSolutions[solution->customMiningType][typeSpecificTaskIndex].add(digest);

        RELEASE(lock);
        return (indexAdded == QPI::NULL_INDEX) ? 0 : 1;
    }

    bool containsTask(uint8_t customMiningType, uint64_t jobId)
    {
        ACQUIRE(lock);
        bool isContained = findTask(customMiningType, jobId) >= 0;
        RELEASE(lock);
        return isContained;
    }
};
