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


//-----------------------------------------------------------------------------
// MinHeap tests

template <typename T, class HeapType>
static void testHeapBasedSorting(const std::vector<T>& vec, const std::vector<T>& sortedVec, HeapType& heap)
{
    T minValue;
    heap.init();
    EXPECT_EQ(heap.size(), 0);
    EXPECT_FALSE(heap.peek(minValue));
    EXPECT_FALSE(heap.extract(minValue));
    EXPECT_FALSE(heap.replace(minValue, vec[0]));

    // add all elements to heap
    for (size_t i = 0; i < vec.size(); ++i)
    {
        EXPECT_TRUE(heap.insert(vec[i]));
        EXPECT_EQ(heap.size(), i + 1);
    }

    // get all elements from heap one by one
    for (size_t i = 0; i < vec.size(); ++i)
    {
        EXPECT_TRUE(heap.peek(minValue));
        EXPECT_EQ(minValue, sortedVec[i]);

        EXPECT_TRUE(heap.extract(minValue));
        EXPECT_EQ(minValue, sortedVec[i]);

        EXPECT_EQ(heap.size(), vec.size() - i - 1);
    }
}

template <typename T, class MyCompareType, class StdCompareType>
static void testHeap(unsigned int seed)
{
    std::mt19937 gen32(seed);
    std::vector<T> vec = prepareData<T>(gen32());

    std::vector<T> sortedVec = vec;
    std::sort(sortedVec.begin(), sortedVec.end(), StdCompareType());

    MinHeap<T, MAX_NUM_ELEMENTS, MyCompareType> heap;
    EXPECT_EQ(heap.capacity(), MAX_NUM_ELEMENTS);

    // test insert, extract, size, and peek
    testHeapBasedSorting(vec, sortedVec, heap);

    // test replace
    while (vec.size() < 200)
        vec = prepareData<T>(gen32());
    std::vector<T> insertedVec;
    T minValue;
    heap.init();
    for (int i = 0; i <= 100; ++i)
    {
        // add element with insert to test replace with different sizes and sets
        EXPECT_TRUE(heap.insert(vec[i]));
        EXPECT_EQ(heap.size(), i + 1);
        insertedVec.push_back(vec[i]);
        std::sort(insertedVec.begin(), insertedVec.end(), StdCompareType());
        EXPECT_TRUE(heap.peek(minValue));
        EXPECT_EQ(minValue, insertedVec[0]);

        // test replace
        EXPECT_TRUE(heap.replace(minValue, vec[i + 100]));
        EXPECT_EQ(minValue, insertedVec[0]);
        EXPECT_EQ(heap.size(), i + 1);
        insertedVec[0] = vec[i + 100];
        std::sort(insertedVec.begin(), insertedVec.end(), StdCompareType());
        EXPECT_TRUE(heap.peek(minValue));
        EXPECT_EQ(minValue, insertedVec[0]);
    }
    for (size_t i = 0; i < insertedVec.size(); ++i)
    {
        EXPECT_TRUE(heap.extract(minValue));
        EXPECT_EQ(minValue, insertedVec[i]);
    }
    EXPECT_EQ(heap.size(), 0);

    // test that insert fails when capacity is reached
    heap.init();
    for (unsigned int i = 0; i < heap.capacity(); ++i)
        EXPECT_TRUE(heap.insert(0));
    EXPECT_FALSE(heap.insert(0));

    // test removeFirstMatch
    EXPECT_FALSE(heap.removeFirstMatch(1));
    EXPECT_TRUE(heap.removeFirstMatch(0));
    heap.init();
    EXPECT_FALSE(heap.removeFirstMatch(0));
    insertedVec.clear();
    for (int i = 0; i <= 100; ++i)
    {
        // add two elements with insert to test removeFirstMatch with different sizes and sets
        EXPECT_TRUE(heap.insert(vec[i]));
        EXPECT_TRUE(heap.insert(vec[i + 100]));
        EXPECT_EQ(heap.size(), i + 2);
        insertedVec.push_back(vec[i]);
        insertedVec.push_back(vec[i + 100]);
        std::sort(insertedVec.begin(), insertedVec.end(), StdCompareType());
        EXPECT_TRUE(heap.peek(minValue));
        EXPECT_EQ(minValue, insertedVec[0]);

        // test removeFirstMatch
        const int removeIdx = gen32() % insertedVec.size();
        EXPECT_TRUE(heap.removeFirstMatch(insertedVec[removeIdx]));
        EXPECT_EQ(heap.size(), i + 1);
        insertedVec.erase(insertedVec.begin() + removeIdx);
        std::sort(insertedVec.begin(), insertedVec.end(), StdCompareType());
        EXPECT_TRUE(heap.peek(minValue));
        EXPECT_EQ(minValue, insertedVec[0]);
    }
    for (size_t i = 0; i < insertedVec.size(); ++i)
    {
        EXPECT_TRUE(heap.extract(minValue));
        EXPECT_EQ(minValue, insertedVec[i]);
    }
    EXPECT_EQ(heap.size(), 0);
}

template <typename T>
class MinMaxHeapTest : public testing::Test {};

TYPED_TEST_CASE_P(MinMaxHeapTest);

TYPED_TEST_P(MinMaxHeapTest, MinHeap)
{
    unsigned int metaSeed = 32467;
    std::mt19937 gen32(metaSeed);

    for (unsigned int t = 0; t < MAX_NUM_TESTS_PER_TYPE; ++t)
        testHeap<TypeParam, CompareLess<TypeParam>, std::less<TypeParam>>(/*seed=*/gen32());
}

TYPED_TEST_P(MinMaxHeapTest, MaxHeap)
{
    unsigned int metaSeed = 45787;
    std::mt19937 gen32(metaSeed);

    for (unsigned int t = 0; t < MAX_NUM_TESTS_PER_TYPE; ++t)
        testHeap<TypeParam, CompareGreater<TypeParam>, std::greater<TypeParam>>(/*seed=*/gen32());
}

REGISTER_TYPED_TEST_CASE_P(MinMaxHeapTest,
    MinHeap,
    MaxHeap
);

// GTest produces a linker error when using `unsigned short` as test type due to unresolved print function - skip for now.
typedef Types<char, unsigned char, short, int, unsigned int, long long, unsigned long long> TestTypes;
INSTANTIATE_TYPED_TEST_CASE_P(TypeParamSortingTests, MinMaxHeapTest, TestTypes);

void testMinHeapCustomCompare(const unsigned int seed)
{
    // Create array of data, including "time" as priority key per item
    std::mt19937 gen32(seed);
    constexpr unsigned int MaxN = 1000;
    const unsigned int N = gen32() % MaxN + 1;
    struct MyData
    {
        unsigned int time;
        unsigned int otherData;
    };
    MyData* myDataArray = new MyData[N];
    for (unsigned int i = 0; i < N; ++i)
    {
        myDataArray[i].time = gen32();
        myDataArray[i].otherData = gen32();
    }

    // Use MinHeap of indices pointing into myDataArray to get index of MyData with lowest time
    struct MyDataEarliest
    {
        MyData* array;
        bool operator()(unsigned int lhs, unsigned int rhs) const
        {
            return array[lhs].time < array[rhs].time;
        }
    };
    MinHeap<unsigned int, MaxN, MyDataEarliest> priorityQueue;
    priorityQueue.init(MyDataEarliest{myDataArray});

    // Add N items of array to priorityQueue one by one
    std::vector<unsigned int> sortedIndicesReference;
    unsigned int minIndexExpected = 0;
    unsigned int minIndexHeap = 0;
    for (unsigned int i = 0; i < N; ++i)
    {
        // add item to queue
        EXPECT_TRUE(priorityQueue.insert(i));
        if (myDataArray[i].time < myDataArray[minIndexExpected].time)
            minIndexExpected = i;

        // check that priority queue min element matches expectation
        EXPECT_TRUE(priorityQueue.peek(minIndexHeap));
        EXPECT_EQ(minIndexExpected, minIndexHeap);

        // update list of all indices, sort by time, and check that first element matches
        sortedIndicesReference.push_back(i);
        std::sort(sortedIndicesReference.begin(), sortedIndicesReference.end(), MyDataEarliest{ myDataArray });
        EXPECT_EQ(myDataArray[sortedIndicesReference[0]].time, myDataArray[minIndexHeap].time);
    }

    // Get item with lowest time one after another
    for (unsigned int i = 0; i < N; ++i)
    {
        // extract item with earliest time
        EXPECT_TRUE(priorityQueue.extract(minIndexHeap));

        // check that it matches the expectation
        EXPECT_EQ(myDataArray[sortedIndicesReference[i]].time, myDataArray[minIndexHeap].time);
    }
    EXPECT_EQ(priorityQueue.size(), 0);

    delete[] myDataArray;
}

TEST(MinMaxHeap, MinHeapCustomCompare)
{
    unsigned int metaSeed = 324678;
    std::mt19937 gen32(metaSeed);

    for (unsigned int t = 0; t < MAX_NUM_TESTS_PER_TYPE; ++t)
        testMinHeapCustomCompare(/*seed=*/gen32());
}
