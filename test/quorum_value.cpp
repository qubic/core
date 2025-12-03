#define NO_UEFI

#include <algorithm>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "../src/platform/quorum_value.h"

TEST(FixedTypeQuorumTest, CalculateAscendingQuorumSimple)
{
    long long values[10] = {10, 5, 8, 3, 7, 1, 9, 2, 6, 4};

    long long result = calculateAscendingQuorumValue(values, 10);

    // (10 * 2) / 3 = 6, index 6 after sorting = 7
    EXPECT_EQ(result, 7);
}

TEST(FixedTypeQuorumTest, Calculate676Quorum)
{
    long long values[676];
    for (int i = 0; i < 676; i++)
    {
        values[i] = i + 1;
    }

    long long quorum = calculateAscendingQuorumValue(values, 676);

    // (676 * 2) / 3 = 450, value at index 450 = 451
    EXPECT_EQ(quorum, 451);
}

TEST(FixedTypeQuorumTest, EmptyArray)
{
    long long values[1] = {0};
    long long result = calculateAscendingQuorumValue(values, 0);
    EXPECT_EQ(result, 0);
}

TEST(FixedTypeQuorumTest, SingleElement)
{
    long long values[1] = {42};
    long long result = calculateAscendingQuorumValue(values, 1);
    EXPECT_EQ(result, 42);
}

TEST(FixedTypeQuorumTest, PercentileDescending)
{
    int values[10] = {10, 5, 8, 3, 7, 1, 9, 2, 6, 4};

    int result = calculatePercentileValue<int, 1, 3, SortingOrder::SortDescending>(values, 10);

    // (10 * 1) / 3 = 3, descending sort, index 3 = 7
    EXPECT_EQ(result, 7);
}

TEST(FixedTypeQuorumTest, AllZeros)
{
    long long values[676] = {0};

    long long quorum = calculateAscendingQuorumValue(values, 676);

    // All values are 0, quorum should be 0
    EXPECT_EQ(quorum, 0);
}

TEST(FixedTypeQuorumTest, MostlyZeros)
{
    long long values[676] = {0};

    // Set first 225 elements to non-zero values
    for (int i = 0; i < 225; i++)
    {
        values[i] = i + 1;
    }

    long long quorum = calculateAscendingQuorumValue(values, 676);

    // After sorting: 0,0,0,...,0 (451 zeros), 1,2,3,...,225
    // Index 450 will be 0 (since 676-225 = 451 zeros, and index 450 < 451)
    EXPECT_EQ(quorum, 0);
}

template <typename T>
std::vector<T> prepareData(unsigned int seed, unsigned int numElements)
{
    std::mt19937 gen32(seed);

    unsigned int numberOfBlocks = (sizeof(T) + 3) / 4;

    std::vector<T> vec(numElements, 0);
    for (unsigned int i = 0; i < numElements; ++i)
    {
        for (unsigned int b = 0; b < numberOfBlocks; ++b)
        {
            vec[i] |= (static_cast<T>(gen32()) << (b * 32));
        }
    }

    return vec;
}

template <typename T>
void testCalculatePercentile(unsigned int seed)
{
    std::vector<T> vec = prepareData<T>(seed, 676);

    std::vector<T> referenceVec = vec;
    std::sort(referenceVec.begin(), referenceVec.end());

    T result = calculatePercentileValue<T, 2, 3, SortingOrder::SortAscending>(vec.data(), static_cast<unsigned int>(vec.size()));

    // Calculate expected index: (676 * 2) / 3 = 450
    unsigned int expectedIndex = (676 * 2) / 3;
    EXPECT_EQ(result, referenceVec[expectedIndex]);
}

template <typename T>
class QuorumValueTest : public testing::Test {};

using testing::Types;

TYPED_TEST_CASE_P(QuorumValueTest);

TYPED_TEST_P(QuorumValueTest, CalculatePercentile)
{
    unsigned int metaSeed = 98765;
    std::mt19937 gen32(metaSeed);

    for (unsigned int t = 0; t < 10; ++t)
        testCalculatePercentile<TypeParam>(gen32());
}

REGISTER_TYPED_TEST_CASE_P(QuorumValueTest,
    CalculatePercentile
);

typedef Types<char, unsigned char, short, int, unsigned int, long long, unsigned long long> TestTypes;
INSTANTIATE_TYPED_TEST_CASE_P(TypeParamQuorumValueTests, QuorumValueTest, TestTypes);
