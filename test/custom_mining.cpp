#define NO_UEFI

#include "gtest/gtest.h"

#ifndef SOLUTION_SECURITY_DEPOSIT
#define SOLUTION_SECURITY_DEPOSIT 1000000
#endif

// 8GB for custom mining task storage. If set zeros, node will not record any custom mining task
constexpr unsigned long long CUSTOM_MINING_TASK_STORAGE_SIZE = 1ULL << 30;

#include "src/mining/mining.h"


TEST(CustomMining, TaskStorageGeneral)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    CustomMiningTaskStorage storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = NUMBER_OF_TASKS - i;

        storage.addTask(task);
    }

    // Expect the task are sort in ascending order
    for (unsigned long long i = 0; i < NUMBER_OF_TASKS - 1; i++)
    {
        CustomMiningTask* task0 = storage.getTaskByIndex(i);
        CustomMiningTask* task1 = storage.getTaskByIndex(i + 1);
        EXPECT_LT(task0->taskIndex, task1->taskIndex);
    }
    EXPECT_EQ(storage.getTaskCount(), NUMBER_OF_TASKS);

    storage.deinit();
}

TEST(CustomMining, TaskStorageDuplicatedItems)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    constexpr unsigned long long DUPCATED_TASKS = 10;
    CustomMiningTaskStorage storage;

    storage.init();

    // For DUPCATED_TASKS will only recorded 1 task
    for (unsigned long long i = 0; i < DUPCATED_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = 1;

        storage.addTask(task);
    }

    for (unsigned long long i = DUPCATED_TASKS; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = i;

        storage.addTask(task);
    }

    // Expect the task are not duplicated
    EXPECT_EQ(storage.getTaskCount(), NUMBER_OF_TASKS - DUPCATED_TASKS + 1);

    storage.deinit();
}

TEST(CustomMining, TaskStorageExistedItem)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    constexpr unsigned long long DUPCATED_TASKS = 10;
    CustomMiningTaskStorage storage;

    storage.init();

    for (unsigned long long i = 1; i < NUMBER_OF_TASKS + 1 ; i++)
    {
        CustomMiningTask task;
        task.taskIndex = i;
        storage.addTask(task);
    }

    // Test an existed task
    CustomMiningTask task;
    task.taskIndex = NUMBER_OF_TASKS - 10;
    storage.addTask(task);

    EXPECT_EQ(storage.taskExisted(task), true);
    unsigned long long idx = storage.lookForTaskGE(task.taskIndex);
    EXPECT_EQ(storage.getTaskByIndex(idx) != NULL, true);
    EXPECT_EQ(storage.getTaskByIndex(idx)->taskIndex, task.taskIndex);
    EXPECT_NE(idx, CustomMiningTaskStorage::_invalidTaskIndex);

    // Test a non-existed task whose the taskIndex is greater than the last task
    task.taskIndex = NUMBER_OF_TASKS + 10;
    EXPECT_EQ(storage.taskExisted(task), false);
    idx = storage.lookForTaskGE(task.taskIndex);
    EXPECT_EQ(idx, CustomMiningTaskStorage::_invalidTaskIndex);

    // Test a non-existed task whose the taskIndex is lower than the last task
    task.taskIndex = 0;
    EXPECT_EQ(storage.taskExisted(task), false);
    idx = storage.lookForTaskGE(task.taskIndex);
    EXPECT_NE(idx, CustomMiningTaskStorage::_invalidTaskIndex);


    storage.deinit();
}

TEST(CustomMining, TaskStorageOverflow)
{
    constexpr unsigned long long NUMBER_OF_TASKS = CUSTOM_MINING_TASK_STORAGE_COUNT;
    CustomMiningTaskStorage storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = i;
        storage.addTask(task);
    }

    // Overflow. Add one more and it is reset
    CustomMiningTask task;
    task.taskIndex = NUMBER_OF_TASKS + 1;
    storage.addTask(task);

    EXPECT_EQ(storage.getTaskCount(), 1);

    storage.deinit();
}


