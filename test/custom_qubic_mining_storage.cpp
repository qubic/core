#define NO_UEFI

#include <array>
#include <random>
#include <cstdint>

#include "gtest/gtest.h"

#include "oracle_testing.h"
#include "src/mining/custom_qubic_mining_storage.h"

void createTestTask(std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)>& buffer, uint64_t jobId, bool cleanJobQueue, std::mt19937& gen)
{
    auto* task = reinterpret_cast<CustomQubicMiningTask*>(buffer.data());
    task->customMiningType = CustomMiningType::DOGE;
    task->jobId = jobId;
    auto* dogeTask = reinterpret_cast<QubicDogeMiningTask*>(buffer.data() + sizeof(CustomQubicMiningTask));

    dogeTask->cleanJobQueue = cleanJobQueue;

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
}

void createTestSolution(std::array<unsigned char, sizeof(CustomQubicMiningSolution) + sizeof(QubicDogeMiningSolution)>& buffer, uint64_t jobId, std::mt19937& gen)
{
    auto* sol = reinterpret_cast<CustomQubicMiningSolution*>(buffer.data());
    sol->customMiningType = CustomMiningType::DOGE;
    sol->jobId = jobId;

    auto* dogeSol = reinterpret_cast<QubicDogeMiningSolution*>(buffer.data() + sizeof(CustomQubicMiningSolution));

    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 32; ++i)
    {
        sol->sourcePublicKey[i] = static_cast<unsigned char>(dis(gen));
        dogeSol->merkleRoot[i] = static_cast<unsigned char>(dis(gen));
    }
    for (int i = 0; i < 4; ++i)
    {
        dogeSol->nTime[i] = static_cast<unsigned char>(dis(gen));
        dogeSol->nonce[i] = static_cast<unsigned char>(dis(gen));
    }
    for (int i = 0; i < 8; ++i)
    {
        dogeSol->extraNonce2[i] = static_cast<unsigned char>(dis(gen));
    }
}

TEST(CustomQubicMiningStorage, InitAndDeinit)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());
    storage.deinit();
}

TEST(CustomQubicMiningStorage, AddTask)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> buffer;

    std::random_device rd;
    std::mt19937 gen(rd());
    createTestTask(buffer, /*jobId=*/12345, /*cleanJobQueue=*/false, gen);

    auto* task = reinterpret_cast<CustomQubicMiningTask*>(buffer.data());

    EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
    EXPECT_FALSE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask))); // duplicate task is not added

    storage.deinit();
}

TEST(CustomQubicMiningStorage, ContainsTask)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> buffer;

    std::random_device rd;
    std::mt19937 gen(rd());
    createTestTask(buffer, /*jobId=*/12345, /*cleanJobQueue=*/false, gen);

    auto* task = reinterpret_cast<CustomQubicMiningTask*>(buffer.data());

    EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
    EXPECT_TRUE(storage.containsTask(task->customMiningType, task->jobId));
    EXPECT_FALSE(storage.containsTask(task->customMiningType, task->jobId + 1));

    storage.deinit();
}

TEST(CustomQubicMiningStorage, AddCleanJobQueueTask)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> buffer;
    auto* task = reinterpret_cast<CustomQubicMiningTask*>(buffer.data());

    std::random_device rd;
    std::mt19937 gen(rd());

    createTestTask(buffer, /*jobId=*/12345, /*cleanJobQueue=*/false, gen);
    EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
    EXPECT_TRUE(storage.containsTask(task->customMiningType, 12345));

    // Create and add a task that will clean the job queue. This should remove the previous task.
    createTestTask(buffer, /*jobId=*/532789, /*cleanJobQueue=*/true, gen);
    EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
    EXPECT_TRUE(storage.containsTask(task->customMiningType, 532789));
    EXPECT_FALSE(storage.containsTask(task->customMiningType, 12345));

    storage.deinit();
}

TEST(CustomQubicMiningStorage, AddingMoreThanMaxNumTasksOverwritesOldest)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> buffer;
    auto* task = reinterpret_cast<CustomQubicMiningTask*>(buffer.data());

    std::random_device rd;
    std::mt19937 gen(rd());

    uint64_t firstJobId = 1000;

    for (uint64_t jobId = firstJobId; jobId <= firstJobId + CustomQubicMiningStorage::maxNumTasks; ++jobId)
    {
        createTestTask(buffer, jobId, /*cleanJobQueue=*/false, gen);
        EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
        EXPECT_TRUE(storage.containsTask(task->customMiningType, jobId));
    }
    // The first added task should have been overwritten.
    EXPECT_FALSE(storage.containsTask(task->customMiningType, firstJobId));

    storage.deinit();
}

TEST(CustomQubicMiningStorage, AddSolutionSuccess)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> taskBuffer;
    auto* task = reinterpret_cast<CustomQubicMiningTask*>(taskBuffer.data());

    std::random_device rd;
    std::mt19937 gen(rd());

    // Create and add a task.
    createTestTask(taskBuffer, /*jobId=*/12345, /*cleanJobQueue=*/false, gen);
    EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
    EXPECT_TRUE(storage.containsTask(task->customMiningType, 12345));

    // Create a solution matching the task, adding should be successful.
    std::array<unsigned char, sizeof(CustomQubicMiningSolution) + sizeof(QubicDogeMiningSolution)> solBuffer;
    auto* sol = reinterpret_cast<CustomQubicMiningSolution*>(solBuffer.data());

    createTestSolution(solBuffer, /*jobId=*/12345, gen);
    EXPECT_EQ(storage.addSolution(sol, sizeof(CustomQubicMiningSolution) + sizeof(QubicDogeMiningSolution)), 1);

    storage.deinit();
}

TEST(CustomQubicMiningStorage, AddSolutionTaskDescriptionMatches)
{
    CustomQubicMiningStorage storage;
    EXPECT_TRUE(storage.init());

    std::array<unsigned char, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)> taskBuffer;
    auto* task = reinterpret_cast<CustomQubicMiningTask*>(taskBuffer.data());

    std::random_device rd;
    std::mt19937 gen(rd());

    // Create and add a task.
    createTestTask(taskBuffer, /*jobId=*/12345, /*cleanJobQueue=*/false, gen);
    EXPECT_TRUE(storage.addTask(task, sizeof(CustomQubicMiningTask) + sizeof(QubicDogeMiningTask)));
    EXPECT_TRUE(storage.containsTask(task->customMiningType, 12345));

    // Create a solution matching the task, adding should be successful.
    std::array<unsigned char, sizeof(CustomQubicMiningSolution) + sizeof(QubicDogeMiningSolution)> solBuffer;
    auto* sol = reinterpret_cast<CustomQubicMiningSolution*>(solBuffer.data());
    CustomQubicMiningStorage::StoredDogeMiningTask storedTask;

    createTestSolution(solBuffer, /*jobId=*/12345, gen);
    EXPECT_EQ(storage.addSolution(sol, sizeof(CustomQubicMiningSolution) + sizeof(QubicDogeMiningSolution), reinterpret_cast<unsigned char*>(&storedTask)), 1);

    auto* dogeTask = reinterpret_cast<QubicDogeMiningTask*>(taskBuffer.data() + sizeof(CustomQubicMiningTask));
    for (int i = 0; i < 32; ++i)
    {
        EXPECT_EQ(storedTask.prevHash[i], dogeTask->prevHash[i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(storedTask.version[i], dogeTask->version[i]);
        EXPECT_EQ(storedTask.nBits[i], dogeTask->nBits[i]);
    }
    uint8_t exponent = dogeTask->dispatcherDifficulty[3];
    for (int i = 0; i < 32; ++i)
    {
        uint8_t expectedByte = (i >= exponent - 3 && i < exponent) ? dogeTask->dispatcherDifficulty[i - (exponent - 3)] : 0;
        EXPECT_EQ(storedTask.dispatcherTarget[i], expectedByte);
    }

    storage.deinit();
}