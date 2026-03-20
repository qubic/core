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
    static constexpr unsigned int maxNumTasks = 32;

    uint64_t activeTasks[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks];
    unsigned int nextTaskIndex[CustomMiningType::TOTAL_NUM_TYPES] = { 0 };

    StoredDogeMiningTask dogeTasks[maxNumTasks];

    bool containsTask(uint8_t customMiningType, uint64_t jobId)
    {
        for (unsigned int i = 0; i < maxNumTasks; ++i)
        {
            if (activeTasks[customMiningType][i] == jobId)
            {
                return true;
            }
        }
        return false;
    }

public:
    void init()
    {
        setMem(activeTasks, sizeof(activeTasks), 0);
        setMem(dogeTasks, sizeof(dogeTasks), 0);
    }

    bool addTask(const CustomQubicMiningTask* task, unsigned int size)
    {
        if (!containsTask(task->customMiningType, task->jobId))
        {
            unsigned int typeSpecificTaskIndex = 0;
            if (task->customMiningType == CustomMiningType::DOGE)
            {
                // Type-specific task info is stored behind general CustomQubicMiningTask struct.
                if (size < sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask))
                    return false;
                unsigned int& nextDogeTaskId = nextTaskIndex[CustomMiningType::DOGE];
                const QubicDogeMiningTask* dogeTask = reinterpret_cast<const QubicDogeMiningTask*>(task + sizeof(CustomQubicMiningTask));
                if (dogeTask->cleanJobQueue)
                {
                    setMem(activeTasks[CustomMiningType::DOGE], maxNumTasks * sizeof(uint64_t), 0);
                    setMem(dogeTasks, sizeof(dogeTasks), 0);
                    nextDogeTaskId = 0;
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
            return activeTasks[task->customMiningType][typeSpecificTaskIndex] = task->jobId;
        }
        return false;
    }
};
