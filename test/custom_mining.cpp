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
    CustomMiningTaskV2Storage storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTaskV2 task;
        task.taskIndex = NUMBER_OF_TASKS - i;

        storage.addData(&task);
    }

    // Expect the task are sort in ascending order
    for (unsigned long long i = 0; i < NUMBER_OF_TASKS - 1; i++)
    {
        CustomMiningTaskV2* task0 = storage.getDataByIndex(i);
        CustomMiningTaskV2* task1 = storage.getDataByIndex(i + 1);
        EXPECT_LT(task0->taskIndex, task1->taskIndex);
    }
    EXPECT_EQ(storage.getCount(), NUMBER_OF_TASKS);

    storage.deinit();
}

TEST(CustomMining, TaskStorageDuplicatedItems)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    constexpr unsigned long long DUPCATED_TASKS = 10;
    CustomMiningTaskV2Storage storage;

    storage.init();

    // For DUPCATED_TASKS will only recorded 1 task
    for (unsigned long long i = 0; i < DUPCATED_TASKS; i++)
    {
        CustomMiningTaskV2 task;
        task.taskIndex = 1;

        storage.addData(&task);
    }

    for (unsigned long long i = DUPCATED_TASKS; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTaskV2 task;
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
    CustomMiningTaskV2Storage storage;

    storage.init();

    for (unsigned long long i = 1; i < NUMBER_OF_TASKS + 1 ; i++)
    {
        CustomMiningTaskV2 task;
        task.taskIndex = i;
        storage.addData(&task);
    }

    // Test an existed task
    CustomMiningTaskV2 task;
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
    CustomMiningTaskV2Storage storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningTaskV2 task;
        task.taskIndex = i;
        storage.addData(&task);
    }

    // Overflow. Add one more and get error status
    CustomMiningTaskV2 task;
    task.taskIndex = NUMBER_OF_TASKS + 1;
    EXPECT_NE(storage.addData(&task), 0);

    storage.deinit();
}

TEST(CustomMining, SolutionStorageGeneral)
{
    constexpr unsigned long long NUMBER_OF_TASKS = 100;
    CustomMiningSolutionStorage storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_TASKS; i++)
    {
        CustomMiningSolutionStorageEntry entry;
        entry.taskIndex = NUMBER_OF_TASKS - i;

        storage.addData(&entry);
    }

    // Expect the task are sorted in ascending order
    for (unsigned long long i = 0; i < NUMBER_OF_TASKS - 1; i++)
    {
        CustomMiningSolutionStorageEntry* entry0 = storage.getDataByIndex(i);
        CustomMiningSolutionStorageEntry* entry1 = storage.getDataByIndex(i + 1);
        EXPECT_LT(entry0->taskIndex, entry1->taskIndex);
    }
    EXPECT_EQ(storage.getCount(), NUMBER_OF_TASKS);

    storage.deinit();
}

TEST(CustomMining, SolutionStorageDuplicatedItems)
{
    constexpr unsigned long long NUMBER_OF_SOLS = 100;
    constexpr unsigned long long DUPLICATED_SOLS = 10;
    CustomMiningSolutionStorage storage;

    storage.init();

    // For DUPCATED_TASKS will only recorded many
    for (unsigned long long i = 0; i < DUPLICATED_SOLS; i++)
    {
        CustomMiningSolutionStorageEntry entry;
        entry.taskIndex = 1;
        entry.nonce = i;

        storage.addData(&entry);
    }

    for (unsigned long long i = DUPLICATED_SOLS; i < NUMBER_OF_SOLS; i++)
    {
        CustomMiningSolutionStorageEntry entry;
        entry.taskIndex = i;
        entry.nonce = i;

        storage.addData(&entry);
    }

    // Expect all elements are added
    EXPECT_EQ(storage.getCount(), NUMBER_OF_SOLS);

    // Data is still in ascending order
    int duplicatedDataCount = 0;
    for (unsigned long long i = 0; i < NUMBER_OF_SOLS - 1; i++)
    {
        CustomMiningSolutionStorageEntry* entry0 = storage.getDataByIndex(i);
        CustomMiningSolutionStorageEntry* entry1 = storage.getDataByIndex(i + 1);
        EXPECT_LE(entry0->taskIndex, entry1->taskIndex);
        if (entry0->taskIndex == entry1->taskIndex)
        {
            duplicatedDataCount++;
        }
    }

    storage.deinit();
}

TEST(CustomMining, SolutionStorageExistedItem)
{
    constexpr unsigned long long NUMBER_OF_SOLS = 100;
    constexpr unsigned long long DUPCATED_SOLS = 10;
    CustomMiningSolutionStorage storage;

    storage.init();

    for (unsigned long long i = 1; i < NUMBER_OF_SOLS + 1; i++)
    {
        CustomMiningSolutionStorageEntry entry;
        entry.taskIndex = i;
        entry.nonce = i;

        storage.addData(&entry);
    }

    // Test an existed task
    CustomMiningSolutionStorageEntry entry;
    entry.taskIndex = NUMBER_OF_SOLS - 10;
    storage.addData(&entry);

    EXPECT_EQ(storage.dataExisted(&entry), true);
    unsigned long long idx = storage.lookForTaskGE(entry.taskIndex);
    EXPECT_EQ(storage.getDataByIndex(idx) != NULL, true);
    EXPECT_EQ(storage.getDataByIndex(idx)->taskIndex, entry.taskIndex);
    EXPECT_NE(idx, CUSTOM_MINING_INVALID_INDEX);

    // Test a non-existed task whose the taskIndex is greater than the last task
    entry.taskIndex = NUMBER_OF_SOLS + 10;
    EXPECT_EQ(storage.dataExisted(&entry), false);
    idx = storage.lookForTaskGE(entry.taskIndex);
    EXPECT_EQ(idx, CUSTOM_MINING_INVALID_INDEX);

    // Test a non-existed task whose the taskIndex is lower than the last task
    entry.taskIndex = 0;
    EXPECT_EQ(storage.dataExisted(&entry), false);
    idx = storage.lookForTaskGE(entry.taskIndex);
    EXPECT_NE(idx, CUSTOM_MINING_INVALID_INDEX);


    storage.deinit();
}

TEST(CustomMining, SolutionsStorageOverflow)
{
    constexpr unsigned long long NUMBER_OF_SOLS = CUSTOM_MINING_SOLUTION_STORAGE_COUNT;
    CustomMiningSolutionStorage storage;

    storage.init();

    for (unsigned long long i = 0; i < NUMBER_OF_SOLS; i++)
    {
        CustomMiningSolutionStorageEntry entry;
        entry.taskIndex = i;
        entry.nonce = i;
        storage.addData(&entry);
    }

    // Overflow. Add one more and get error status
    CustomMiningSolutionStorageEntry entry;
    entry.taskIndex = NUMBER_OF_SOLS + 1;
    EXPECT_NE(storage.addData(&entry), CustomMiningSolutionStorage::OK);

    storage.deinit();
}
