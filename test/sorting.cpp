#define NO_UEFI

#include "gtest/gtest.h"
#include "lib/platform_common/sorting.h"

TEST(SortingTest, SortAscendingSimple)
{
    int arr[5] = { 3, 6, 1, 9, 2 };

    quickSort(arr, 0, 4, SortingOrder::SortAscending);

    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 6);
    EXPECT_EQ(arr[4], 9);
}

TEST(SortingTest, SortDescendingSimple)
{
    int arr[5] = { 3, 6, 1, 9, 2 };

    quickSort(arr, 0, 4, SortingOrder::SortDescending);

    EXPECT_EQ(arr[0], 9);
    EXPECT_EQ(arr[1], 6);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 2);
    EXPECT_EQ(arr[4], 1);
}