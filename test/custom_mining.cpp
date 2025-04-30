#define NO_UEFI

#include "gtest/gtest.h"

#ifndef SOLUTION_SECURITY_DEPOSIT
#define SOLUTION_SECURITY_DEPOSIT 1000000
#endif

#ifndef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 600000
#endif

#ifndef MAX_NUMBER_OF_PROCESSORS
#define MAX_NUMBER_OF_PROCESSORS 4
#endif

#include "src/mining/mining.h"

TEST(CustomMining, TaskStorageGeneral)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    CustomMiningSortedStorage<CustomMiningTask, CUSTOM_MINING_TASK_STORAGE_COUNT, 0, false> storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = NUMBER_OF_TASKS - i;

        storage.addData(&task);
    }

    // Expect the task are sort in ascending order
    for (unsigned long long i = 0; i < NUMBER_OF_TASKS - 1; i++)
    {
        CustomMiningTask* task0 = storage.getDataByIndex(i);
        CustomMiningTask* task1 = storage.getDataByIndex(i + 1);
        EXPECT_LT(task0->taskIndex, task1->taskIndex);
    }
    EXPECT_EQ(storage.getCount(), NUMBER_OF_TASKS);

    storage.deinit();
}

TEST(CustomMining, TaskStorageDuplicatedItems)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    constexpr unsigned long long DUPCATED_TASKS = 10;
    CustomMiningSortedStorage<CustomMiningTask, CUSTOM_MINING_TASK_STORAGE_COUNT, 0, false> storage;

    storage.init();

    // For DUPCATED_TASKS will only recorded 1 task
    for (unsigned long long i = 0; i < DUPCATED_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = 1;

        storage.addData(&task);
    }

    for (unsigned long long i = DUPCATED_TASKS; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = i;

        storage.addData(&task);
    }

    // Expect the task are not duplicated
    EXPECT_EQ(storage.getCount(), NUMBER_OF_TASKS - DUPCATED_TASKS + 1);

    storage.deinit();
}

TEST(CustomMining, TaskStorageExistedItem)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    constexpr unsigned long long DUPCATED_TASKS = 10;
    CustomMiningSortedStorage<CustomMiningTask, CUSTOM_MINING_TASK_STORAGE_COUNT, 0, false> storage;

    storage.init();

    for (unsigned long long i = 1; i < NUMBER_OF_TASKS + 1 ; i++)
    {
        CustomMiningTask task;
        task.taskIndex = i;
        storage.addData(&task);
    }

    // Test an existed task
    CustomMiningTask task;
    task.taskIndex = NUMBER_OF_TASKS - 10;
    storage.addData(&task);

    EXPECT_EQ(storage.dataExisted(&task), true);
    unsigned long long idx = storage.lookForTaskGE(task.taskIndex);
    EXPECT_EQ(storage.getDataByIndex(idx) != NULL, true);
    EXPECT_EQ(storage.getDataByIndex(idx)->taskIndex, task.taskIndex);
    EXPECT_NE(idx, CUSTOM_MINING_INVALID_INDEX);

    // Test a non-existed task whose the taskIndex is greater than the last task
    task.taskIndex = NUMBER_OF_TASKS + 10;
    EXPECT_EQ(storage.dataExisted(&task), false);
    idx = storage.lookForTaskGE(task.taskIndex);
    EXPECT_EQ(idx, CUSTOM_MINING_INVALID_INDEX);

    // Test a non-existed task whose the taskIndex is lower than the last task
    task.taskIndex = 0;
    EXPECT_EQ(storage.dataExisted(&task), false);
    idx = storage.lookForTaskGE(task.taskIndex);
    EXPECT_NE(idx, CUSTOM_MINING_INVALID_INDEX);


    storage.deinit();
}

TEST(CustomMining, TaskStorageOverflow)
{
    constexpr unsigned long long NUMBER_OF_TASKS = CUSTOM_MINING_TASK_STORAGE_COUNT;
    CustomMiningSortedStorage<CustomMiningTask, CUSTOM_MINING_TASK_STORAGE_COUNT, 0, false> storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTask task;
        task.taskIndex = i;
        storage.addData(&task);
    }

    // Overflow. Add one more and get error status
    CustomMiningTask task;
    task.taskIndex = NUMBER_OF_TASKS + 1;
    EXPECT_NE(storage.addData(&task), 0);

    storage.deinit();
}


