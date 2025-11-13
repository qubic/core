#define NO_UEFI

#include <algorithm>
#include <functional>
#include <random>

#include "gtest/gtest.h"
#include "lib/platform_common/sorting.h"

constexpr unsigned int MAX_NUM_ELEMENTS = 10000U;
constexpr unsigned int MAX_NUM_TESTS_PER_TYPE = 100U;

TEST(FixedTypeSortingTest, SortAscendingSimple)
{
    int arr[5] = { 3, 6, 1, 9, 2 };

    quickSort(arr, 0, 4, SortingOrder::SortAscending);

    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 6);
    EXPECT_EQ(arr[4], 9);
}

TEST(FixedTypeSortingTest, SortDescendingSimple)
{
    int arr[5] = { 3, 6, 1, 9, 2 };

    quickSort(arr, 0, 4, SortingOrder::SortDescending);

    EXPECT_EQ(arr[0], 9);
    EXPECT_EQ(arr[1], 6);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 2);
    EXPECT_EQ(arr[4], 1);
}

template <typename T>
std::vector<T> prepareData(unsigned int seed)
{
    std::mt19937 gen32(seed);

    unsigned int numberOfBlocks = (sizeof(T) + 3) / 4;

    unsigned int numElements = (gen32() % MAX_NUM_ELEMENTS) + 1;
    std::vector<T> vec(numElements, 0);
    for (unsigned int i = 0; i < numElements; ++i)
    {
        // generate 4 bytes at a time until the whole number is generated 
        for (unsigned int b = 0; b < numberOfBlocks; ++b)
        {
            vec[i] |= (static_cast<T>(gen32()) << (b * 32));
        }
    }

    return vec;
}

template <typename T>
void testSortAscending(unsigned int seed)
{
    std::vector vec = prepareData<T>(seed);

    std::vector<T> referenceVec = vec;
    std::sort(referenceVec.begin(), referenceVec.end(), std::less<>());

    quickSort(vec.data(), 0, static_cast<int>(vec.size() - 1), SortingOrder::SortAscending);

    EXPECT_EQ(vec, referenceVec);
}

template <typename T>
void testSortDescending(unsigned int seed)
{
    std::vector vec = prepareData<T>(seed);

    std::vector<T> referenceVec = vec;
    std::sort(referenceVec.begin(), referenceVec.end(), std::greater<>());

    quickSort(vec.data(), 0, static_cast<int>(vec.size()  - 1), SortingOrder::SortDescending);

    EXPECT_EQ(vec, referenceVec);
}

template <typename T>
class SortingTest : public testing::Test {};

using testing::Types;

TYPED_TEST_CASE_P(SortingTest);

TYPED_TEST_P(SortingTest, SortAscending)
{
    unsigned int metaSeed = 32467;
    std::mt19937 gen32(metaSeed);

    for (unsigned int t = 0; t < MAX_NUM_TESTS_PER_TYPE; ++t)
        testSortAscending<TypeParam>(/*seed=*/gen32());
}

TYPED_TEST_P(SortingTest, SortDescending)
{
    unsigned int metaSeed = 45787;
    std::mt19937 gen32(metaSeed);

    for (unsigned int t = 0; t < MAX_NUM_TESTS_PER_TYPE; ++t)
        testSortDescending<TypeParam>(/*seed=*/gen32());
}

REGISTER_TYPED_TEST_CASE_P(SortingTest,
	SortAscending,
    SortDescending
);

// GTest produces a linker error when using `unsigned short` as test type due to unresolved print function - skip for now.
typedef Types<char, unsigned char, short, int, unsigned int, long long, unsigned long long> TestTypes;
INSTANTIATE_TYPED_TEST_CASE_P(TypeParamSortingTests, SortingTest, TestTypes);