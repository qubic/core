#define NO_UEFI

#include <array>
#include <random>

#include "gtest/gtest.h"

#include "src/mining/custom_qubic_mining_storage.h"

TEST(CustomQubicMiningStorage, InitAndDeinit)
{
    CustomQubicMiningStorage storage;
    EXPECT_EQ(storage.init(), true);
    storage.deinit();
}

TEST(CustomQubicMiningStorage, AddTask)
{
    CustomQubicMiningStorage storage;
    EXPECT_EQ(storage.init(), true);

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> buffer;

    auto* task = reinterpret_cast<CustomQubicMiningTask*>(buffer.data());
    task->customMiningType = CustomMiningType::DOGE;
    task->jobId = 12345;
    auto* dogeTask = reinterpret_cast<QubicDogeMiningTask*>(buffer.data() + sizeof(CustomQubicMiningTask));

    dogeTask->cleanJobQueue = false;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 3; ++i)
    {
        dogeTask->dispatcherDifficulty[i] = static_cast<unsigned char>(dis(gen));
    }
    // exponent should be set to a value that results in a reasonable target, e.g. 0x1d (same as Bitcoin's initial difficulty)
    dogeTask->dispatcherDifficulty[3] = 0x1d;
    for (int i = 0; i < 4; ++i)
    {
        dogeTask->version[i] = static_cast<unsigned char>(dis(gen));
        dogeTask->nTime[i] = static_cast<unsigned char>(dis(gen));
        dogeTask->nBits[i] = static_cast<unsigned char>(dis(gen));
    }
    for (int i = 0; i < 32; ++i)
    {
        dogeTask->prevHash[i] = static_cast<unsigned char>(dis(gen));
    }

    dogeTask->extraNonce1NumBytes = 0;
    dogeTask->coinbase1NumBytes = 0;
    dogeTask->coinbase2NumBytes = 0;
    dogeTask->numMerkleBranches = 0;

    EXPECT_EQ(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)), true);
    EXPECT_EQ(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)), false); // duplicate task
    
    storage.deinit();
}