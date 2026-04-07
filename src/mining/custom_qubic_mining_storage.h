#pragma once

#include <cstdint>

#include "platform/memory.h"
#include "platform/memory_util.h"
#include "platform/concurrency.h"
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

private:
    static constexpr unsigned int maxNumSolutionsPerTask = 256;

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

public:

    // Initialize the storage. Return true if successful, false if initialization failed (e.g. due to memory allocation failure).
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

    // Deinitialize the storage and free any allocated memory.
    void deinit()
    {
        if (receivedSolutions)
            freePool(receivedSolutions);
    }

    // Add a new mining task. Return false if a task with the same jobId and customMiningType already exists, true if added successfully.
    // The task description is expected to be behind the CustomQubicMiningTask struct in memory. The provided size should specify the
    // full size of the CustomQubicMiningTask struct and the task description.
    // For DOGE mining: Adding a task with cleanJobQueue = true will clear all existing tasks and solutions before adding the new task. 
    //                  If maxNumTasks is reached and a job with cleanJobQueue = false is added, it will override the oldest task and its solutions.
    bool addTask(const CustomQubicMiningTask* task, unsigned int size)
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

    // Return -1 if the solution is invalid (stale or duplicate), 0 if the solution is valid but not added due to storage limit,
    // and 1 if the solution is valid and added successfully. If taskDescription is not nullptr and the solution corresponds to an active task,
    // the task description is written into the provided pointer.
    int addSolution(const CustomQubicMiningSolution* solution, unsigned int size, unsigned char* taskDescription = nullptr)
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

    // Return true if there is an active task with the given jobId and customMiningType, false otherwise.
    bool containsTask(uint8_t customMiningType, uint64_t jobId)
    {
        LockGuard guard(lock);
        return findTask(customMiningType, jobId) >= 0;
    }
};
