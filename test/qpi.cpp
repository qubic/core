#define NO_UEFI

#include "gtest/gtest.h"

void* __scratchpadBuffer = nullptr;
static void* __scratchpad()
{
    return __scratchpadBuffer;
}

#include "../src/smart_contracts/qpi.h"

#include <vector>
#include <map>
#include <random>


template <typename T, unsigned long long capacity>
void checkPriorityQueue(const QPI::collection<T, capacity>& coll, const QPI::id& pov, bool print = false)
{
    if (print)
    {
        std::cout << "Priority queue ID(" << pov.m256i_u64[0] << ", " << pov.m256i_u64[1] << ", "
            << pov.m256i_u64[2] << ", " << pov.m256i_u64[3] << ")" << std::endl;
    }
    bool first = true;
    QPI::sint64 elementIndex = coll.headIndex(pov);
    QPI::sint64 prevPriority;
    QPI::sint64 prevElementIdx = QPI::NULL_INDEX;
    int elementCount = 0;
    while (elementIndex != QPI::NULL_INDEX)
    {
        if (print)
        {
            std::cout << "\tindex " << elementIndex << ", value " << coll.element(elementIndex)
                << ", priority " << coll.priority(elementIndex)
                << ", prev " << coll.prevElementIndex(elementIndex)
                << ", next " << coll.nextElementIndex(elementIndex) << std::endl;
        }

        if (!first)
        {
            EXPECT_GE(coll.priority(elementIndex), prevPriority);
        }
        EXPECT_EQ(coll.prevElementIndex(elementIndex), prevElementIdx);
        EXPECT_EQ(coll.pov(elementIndex), pov);

        prevElementIdx = elementIndex;
        prevPriority = coll.priority(elementIndex);

        first = false;
        elementIndex = coll.nextElementIndex(elementIndex);
        ++elementCount;
    }
    EXPECT_EQ(elementCount, coll.population(pov));
    EXPECT_EQ(prevElementIdx, coll.tailIndex(pov));
}

void printPovElementCounts(const std::map<QPI::id, unsigned long long>& povElementCounts)
{
    std::cout << "PoV element counts:\n";
    for (const auto& id_count_pair : povElementCounts)
    {
        QPI::id id = id_count_pair.first;
        unsigned long long count = id_count_pair.second;
        std::cout << "\t(" << id.m256i_u64[0] << ", " << id.m256i_u64[1] << ", " << id.m256i_u64[2] << ", " << id.m256i_u64[3] << "): " << count << std::endl;
    }
}

// return sorted set of PoVs
template <typename T, unsigned long long capacity>
std::map<QPI::id, unsigned long long> getPovElementCounts(const QPI::collection<T, capacity>& coll)
{
    // use that in current implementation elements are always in range 0 to N-1
    std::map<QPI::id, unsigned long long> povs;
    for (unsigned long long i = 0; i < coll.population(); ++i)
    {
        QPI::id id = coll.pov(i);
        EXPECT_NE(coll.headIndex(id), QPI::NULL_INDEX);
        EXPECT_NE(coll.tailIndex(id), QPI::NULL_INDEX);
        ++povs[id];
    }

    for (const auto& id_count_pair : povs)
    {
        EXPECT_EQ(coll.population(id_count_pair.first), id_count_pair.second);
    }

    return povs;
}

template <typename T, unsigned long long capacity>
bool isCompletelySame(const QPI::collection<T, capacity>& coll1, const QPI::collection<T, capacity>& coll2)
{
    return memcmp(&coll1, &coll2, sizeof(coll1)) == 0;
}

template <typename T, unsigned long long capacity>
bool haveSameContent(const QPI::collection<T, capacity>& coll1, const QPI::collection<T, capacity>& coll2, bool verbose = true)
{
    // check that both contain the same PoVs, each with the same number of elements
    auto coll1PovCounts = getPovElementCounts(coll1);
    auto coll2PovCounts = getPovElementCounts(coll2);
    if (coll1PovCounts != coll2PovCounts)
    {
        if (verbose)
        {
            std::cout << "Differences in PoV sets of collections!" << std::endl;
            if (coll1PovCounts.size() != coll2PovCounts.size())
                std::cout << "\tPoV count: " << coll1PovCounts.size() << " vs " << coll2PovCounts.size() << std::endl;
            printPovElementCounts(coll1PovCounts);
            printPovElementCounts(coll2PovCounts);
        }
        return false;
    }

    // check that values and priorities of the elements are the same
    for (const auto& id_count_pair : coll1PovCounts)
    {
        QPI::id pov = id_count_pair.first;
        QPI::sint64 elementIndex1 = coll1.headIndex(pov);
        QPI::sint64 elementIndex2 = coll2.headIndex(pov);
        while (elementIndex1 != QPI::NULL_INDEX && elementIndex2 != QPI::NULL_INDEX)
        {
            if (coll1.priority(elementIndex1) != coll2.priority(elementIndex2))
                return false;
            if (coll1.element(elementIndex1) != coll2.element(elementIndex2))
                return false;

            EXPECT_EQ(coll1.pov(elementIndex1), pov);
            EXPECT_EQ(coll2.pov(elementIndex2), pov);

            elementIndex1 = coll1.nextElementIndex(elementIndex1);
            elementIndex2 = coll2.nextElementIndex(elementIndex2);
        }
        EXPECT_EQ(elementIndex1, QPI::NULL_INDEX);
        EXPECT_EQ(elementIndex2, QPI::NULL_INDEX);
        EXPECT_EQ(coll1.nextElementIndex(coll1.tailIndex(pov)), QPI::NULL_INDEX);
        EXPECT_EQ(coll2.nextElementIndex(coll2.tailIndex(pov)), QPI::NULL_INDEX);
    }

    return true;
}

template <typename T, unsigned long long capacity>
void cleanupCollectionReferenceImplementation(const QPI::collection<T, capacity>& coll, QPI::collection<T, capacity> & newColl)
{
    newColl.reset();

    // for each pov, add all elements of priority queue in order
    auto povs = getPovElementCounts(coll);
    for (const auto& id_count_pair : povs)
    {
        QPI::id pov = id_count_pair.first;
        QPI::sint64 elementIndex = coll.headIndex(pov);
        while (elementIndex != QPI::NULL_INDEX)
        {
            newColl.add(pov, coll.element(elementIndex), coll.priority(elementIndex));
            elementIndex = coll.nextElementIndex(elementIndex);
        }
    }
}

template <typename T, unsigned long long capacity>
void cleanupCollection(QPI::collection<T, capacity>& coll)
{
    // save original data for checking
    QPI::collection<T, capacity> origColl;
    copyMem(&origColl, &coll, sizeof(coll));

    // run reference cleanup and test that cleanup did not change any relevant content
    cleanupCollectionReferenceImplementation(origColl, coll);
    EXPECT_TRUE(haveSameContent(origColl, coll));

    // run faster cleanup and check result
    origColl.cleanup();
    EXPECT_TRUE(haveSameContent(origColl, coll));
}


TEST(TestCoreQPI, CollectionMultiPovMultiElements) {
    QPI::id id1(1, 2, 3, 4);
    QPI::id id2(3, 100, 579, 5431);
    QPI::id id3(1, 100, 579, 5431);
    
    constexpr unsigned long long capacity = 8;

    // for valid init you either need to call reset or load the data from a file (in SC, state is zeroed before INITIALIZE is called)
    QPI::collection<int, capacity> coll;
    coll.reset();

    // test behavior of empty collection
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 0);
    EXPECT_EQ(coll.headIndex(id1), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id1), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id1), 0);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id2), 0);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    // all properties of non-occupied elements are initialized to 0 (by reset function), but in practice only occupied
    // elements should be accessed
    EXPECT_EQ(coll.pov(0), QPI::id(0, 0, 0, 0));
    EXPECT_EQ(coll.element(0), 0);
    EXPECT_EQ(coll.nextElementIndex(0), 0);
    EXPECT_EQ(coll.prevElementIndex(0), 0);
    EXPECT_EQ(coll.priority(0), 0);

    // add an element with id1
    constexpr int firstElementValue = 42;
    constexpr QPI::sint64 firstElementPriority = 1234;
    QPI::sint64 firstElementIdx = coll.add(id1, firstElementValue, firstElementPriority);
    EXPECT_TRUE(firstElementIdx != QPI::NULL_INDEX);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 1);
    EXPECT_EQ(coll.headIndex(id1), firstElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), firstElementIdx);
    EXPECT_EQ(coll.population(id1), 1);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id2), 0);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    
    // add another element with with id1, but higher priority value
    // id1 priority queue order: firstElement, secondElement
    constexpr int secondElementValue = 987;
    constexpr QPI::sint64 secondElementPriority = 12345;
    QPI::sint64 secondElementIdx = coll.add(id1, secondElementValue, secondElementPriority);
    EXPECT_TRUE(secondElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(secondElementIdx != firstElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 2);
    EXPECT_EQ(coll.headIndex(id1), firstElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 2);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id2), 0);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);

    // add another element with id1, but lower priority value
    // id1 priority queue order: thirdElement, firstElement, secondElement
    constexpr int thirdElementValue = 98;
    constexpr QPI::sint64 thirdElementPriority = 12;
    QPI::sint64 thirdElementIdx = coll.add(id1, thirdElementValue, thirdElementPriority);
    EXPECT_TRUE(thirdElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(thirdElementIdx != firstElementIdx);
    EXPECT_TRUE(thirdElementIdx != secondElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 3);
    EXPECT_EQ(coll.headIndex(id1), thirdElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 3);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id2), 0);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);

    // add element with id2
    // id2 priority queue order: fourthElement
    constexpr int fourthElementValue = 4;
    constexpr QPI::sint64 fourthElementPriority = -10;
    QPI::sint64 fourthElementIdx = coll.add(id2, fourthElementValue, fourthElementPriority);
    EXPECT_TRUE(fourthElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(fourthElementIdx != firstElementIdx);
    EXPECT_TRUE(fourthElementIdx != secondElementIdx);
    EXPECT_TRUE(fourthElementIdx != thirdElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 4);
    EXPECT_EQ(coll.headIndex(id1), thirdElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 3);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);

    // add element with id3
    // id3 priority queue order: fifthElement
    constexpr int fifthElementValue = 50;
    constexpr QPI::sint64 fifthElementPriority = -10;
    QPI::sint64 fifthElementIdx = coll.add(id3, fifthElementValue, fifthElementPriority);
    EXPECT_TRUE(fifthElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(fifthElementIdx != firstElementIdx);
    EXPECT_TRUE(fifthElementIdx != secondElementIdx);
    EXPECT_TRUE(fifthElementIdx != thirdElementIdx);
    EXPECT_TRUE(fifthElementIdx != fourthElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 5);
    EXPECT_EQ(coll.headIndex(id1), thirdElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 3);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.population(id3), 1);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);
    EXPECT_EQ(coll.pov(fifthElementIdx), id3);
    EXPECT_EQ(coll.element(fifthElementIdx), fifthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fifthElementIdx), fifthElementPriority);

    // add another element with id1, with lowest priority value
    // id1 priority queue order: sixthElement, thirdElement, firstElement, secondElement
    constexpr int sixthElementValue = 600;
    constexpr QPI::sint64 sixthElementPriority = -60;
    QPI::sint64 sixthElementIdx = coll.add(id1, sixthElementValue, sixthElementPriority);
    EXPECT_TRUE(sixthElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(sixthElementIdx != firstElementIdx);
    EXPECT_TRUE(sixthElementIdx != secondElementIdx);
    EXPECT_TRUE(sixthElementIdx != thirdElementIdx);
    EXPECT_TRUE(sixthElementIdx != fourthElementIdx);
    EXPECT_TRUE(sixthElementIdx != fifthElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 6);
    EXPECT_EQ(coll.headIndex(id1), sixthElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 4);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.population(id3), 1);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), sixthElementIdx);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);
    EXPECT_EQ(coll.pov(fifthElementIdx), id3);
    EXPECT_EQ(coll.element(fifthElementIdx), fifthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fifthElementIdx), fifthElementPriority);
    EXPECT_EQ(coll.pov(sixthElementIdx), id1);
    EXPECT_EQ(coll.element(sixthElementIdx), sixthElementValue);
    EXPECT_EQ(coll.nextElementIndex(sixthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(sixthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(sixthElementIdx), sixthElementPriority);

    // add another element with id3, with highest priority value
    // id3 priority queue order: fifthElement, seventhElement
    constexpr int seventhElementValue = 700;
    constexpr QPI::sint64 seventhElementPriority = 70000;
    QPI::sint64 seventhElementIdx = coll.add(id3, seventhElementValue, seventhElementPriority);
    EXPECT_TRUE(seventhElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(seventhElementIdx != firstElementIdx);
    EXPECT_TRUE(seventhElementIdx != secondElementIdx);
    EXPECT_TRUE(seventhElementIdx != thirdElementIdx);
    EXPECT_TRUE(seventhElementIdx != fourthElementIdx);
    EXPECT_TRUE(seventhElementIdx != fifthElementIdx);
    EXPECT_TRUE(seventhElementIdx != sixthElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 7);
    EXPECT_EQ(coll.headIndex(id1), sixthElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 4);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), seventhElementIdx);
    EXPECT_EQ(coll.population(id3), 2);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), sixthElementIdx);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);
    EXPECT_EQ(coll.pov(fifthElementIdx), id3);
    EXPECT_EQ(coll.element(fifthElementIdx), fifthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fifthElementIdx), seventhElementIdx);
    EXPECT_EQ(coll.prevElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fifthElementIdx), fifthElementPriority);
    EXPECT_EQ(coll.pov(sixthElementIdx), id1);
    EXPECT_EQ(coll.element(sixthElementIdx), sixthElementValue);
    EXPECT_EQ(coll.nextElementIndex(sixthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(sixthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(sixthElementIdx), sixthElementPriority);
    EXPECT_EQ(coll.pov(seventhElementIdx), id3);
    EXPECT_EQ(coll.element(seventhElementIdx), seventhElementValue);
    EXPECT_EQ(coll.nextElementIndex(seventhElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(seventhElementIdx), fifthElementIdx);
    EXPECT_EQ(coll.priority(seventhElementIdx), seventhElementPriority);

    // add another element with id1, with medium priority value
    // id1 priority queue order: sixthElement, thirdElement, eighthElement, firstElement, secondElement
    //               priorities:          -60,           12,           123,         1234,         12345
    constexpr int eighthElementValue = 800;
    constexpr QPI::sint64 eighthElementPriority = 123;
    QPI::sint64 eighthElementIdx = coll.add(id1, eighthElementValue, eighthElementPriority);
    EXPECT_TRUE(eighthElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(eighthElementIdx != firstElementIdx);
    EXPECT_TRUE(eighthElementIdx != secondElementIdx);
    EXPECT_TRUE(eighthElementIdx != thirdElementIdx);
    EXPECT_TRUE(eighthElementIdx != fourthElementIdx);
    EXPECT_TRUE(eighthElementIdx != fifthElementIdx);
    EXPECT_TRUE(eighthElementIdx != sixthElementIdx);
    EXPECT_TRUE(eighthElementIdx != seventhElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 8);
    EXPECT_EQ(coll.headIndex(id1), sixthElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.population(id1), 5);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), seventhElementIdx);
    EXPECT_EQ(coll.population(id3), 2);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), eighthElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), eighthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), sixthElementIdx);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);
    EXPECT_EQ(coll.pov(fifthElementIdx), id3);
    EXPECT_EQ(coll.element(fifthElementIdx), fifthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fifthElementIdx), seventhElementIdx);
    EXPECT_EQ(coll.prevElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fifthElementIdx), fifthElementPriority);
    EXPECT_EQ(coll.pov(sixthElementIdx), id1);
    EXPECT_EQ(coll.element(sixthElementIdx), sixthElementValue);
    EXPECT_EQ(coll.nextElementIndex(sixthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(sixthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(sixthElementIdx), sixthElementPriority);
    EXPECT_EQ(coll.pov(seventhElementIdx), id3);
    EXPECT_EQ(coll.element(seventhElementIdx), seventhElementValue);
    EXPECT_EQ(coll.nextElementIndex(seventhElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(seventhElementIdx), fifthElementIdx);
    EXPECT_EQ(coll.priority(seventhElementIdx), seventhElementPriority);
    EXPECT_EQ(coll.pov(eighthElementIdx), id1);
    EXPECT_EQ(coll.element(eighthElementIdx), eighthElementValue);
    EXPECT_EQ(coll.nextElementIndex(eighthElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(eighthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(eighthElementIdx), eighthElementPriority);
    
    checkPriorityQueue(coll, id1);
    checkPriorityQueue(coll, id2);
    checkPriorityQueue(coll, id3);

    // test that nothing is added to full
    EXPECT_EQ(capacity, 8);
    QPI::sint64 ninthElementIdx = coll.add(id1, 1234, 6544);
    EXPECT_TRUE(ninthElementIdx == QPI::NULL_INDEX);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 8);

    // test comparison function of full collection
    QPI::collection<int, capacity> empty_coll;
    empty_coll.reset();
    EXPECT_TRUE(isCompletelySame(coll, coll));
    EXPECT_TRUE(haveSameContent(coll, coll));
    EXPECT_FALSE(isCompletelySame(coll, empty_coll));
    EXPECT_FALSE(haveSameContent(coll, empty_coll, false));

    // test behavior of collection after resetting non-empty collection
    coll.reset();
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 0);
    EXPECT_EQ(coll.headIndex(id1), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id1), QPI::NULL_INDEX);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    // all properties of non-occupied elements are initialized to 0 (by reset function), but in practice only occupied
    // elements should be accessed
    EXPECT_EQ(coll.pov(0), QPI::id(0, 0, 0, 0));
    EXPECT_EQ(coll.element(0), 0);
    EXPECT_EQ(coll.nextElementIndex(0), 0);
    EXPECT_EQ(coll.prevElementIndex(0), 0);
    EXPECT_EQ(coll.priority(0), 0);
}


TEST(TestCoreQPI, CollectionOnePovMultiElements) {
    constexpr unsigned long long capacity = 128;

    // for valid init you either need to call reset or load the data from a file (in SC, state is zeroed before INITIALIZE is called)
    QPI::collection<int, capacity> coll;
    coll.reset();

    // these tests support changing the implementation of the element array filling to non-sequential
    // by saving element indices in order
    std::vector<QPI::sint64> elementIndices;

    // fill completely with alternating priorities
    QPI::id pov(1, 2, 3, 4);
    for (int i = 0; i < capacity; ++i)
    {
        QPI::sint64 prio = QPI::sint64(i * 10 * sin(i / 3));
        int value = i * 4;

        EXPECT_EQ(coll.capacity(), capacity);
        EXPECT_EQ(coll.population(), i);
        EXPECT_EQ(coll.population(pov), i);

        QPI::sint64 elementIndex = coll.add(pov, value, prio);
        elementIndices.push_back(elementIndex);

        EXPECT_TRUE(elementIndex != QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(pov), i + 1);
        EXPECT_EQ(coll.population(), i + 1);
    }

    // check that nothing can be added
    QPI::sint64 elementIndex = coll.add(pov, 1234, 12345);
    EXPECT_TRUE(elementIndex == QPI::NULL_INDEX);
    EXPECT_EQ(coll.capacity(), coll.population());
    EXPECT_EQ(coll.population(pov), coll.capacity());

    // check validity of data
    checkPriorityQueue(coll, pov);
    for (int i = 0; i < capacity; ++i)
    {
        QPI::sint64 prio = QPI::sint64(i * 10 * sin(i / 3));
        int value = i * 4;

        QPI::sint64 elementIndex = elementIndices[i];
        EXPECT_EQ(coll.element(elementIndex), value);
        EXPECT_EQ(coll.priority(elementIndex), prio);
    }

    // remove first element
    {
        QPI::sint64 headIndex = coll.headIndex(pov);
        QPI::sint64 afterHeadIndex = coll.nextElementIndex(headIndex);
        QPI::sint64 afterHeadPrio = coll.priority(afterHeadIndex);
        int afterHeadValue = coll.element(afterHeadIndex);
        EXPECT_EQ(coll.population(), coll.capacity());
        EXPECT_EQ(coll.population(pov), coll.capacity());
        checkPriorityQueue(coll, pov);
        coll.remove(headIndex);
        EXPECT_EQ(coll.population(), coll.capacity() - 1);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 1);
        
        checkPriorityQueue(coll, pov);
        
        headIndex = coll.headIndex(pov);
        EXPECT_EQ(coll.prevElementIndex(headIndex), QPI::NULL_INDEX);
        EXPECT_EQ(coll.priority(headIndex), afterHeadPrio);
        EXPECT_EQ(coll.element(headIndex), afterHeadValue);
    }

    // remove last element
    {
        QPI::sint64 tailIndex = coll.tailIndex(pov);
        QPI::sint64 beforeTailIndex = coll.prevElementIndex(tailIndex);
        QPI::sint64 beforeTailPrio = coll.priority(beforeTailIndex);
        int beforeTailValue = coll.element(beforeTailIndex);
        EXPECT_EQ(coll.population(), coll.capacity() - 1);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 1);
        coll.remove(tailIndex);
        EXPECT_EQ(coll.population(), coll.capacity() - 2);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 2);

        checkPriorityQueue(coll, pov);

        tailIndex = coll.tailIndex(pov);
        EXPECT_EQ(tailIndex, beforeTailIndex);
        EXPECT_EQ(coll.nextElementIndex(tailIndex), QPI::NULL_INDEX);
        EXPECT_EQ(coll.priority(tailIndex), beforeTailPrio);
        EXPECT_EQ(coll.element(tailIndex), beforeTailValue);
    }

    // remove element in front of tail
    {
        QPI::sint64 tailIndex = coll.tailIndex(pov);
        QPI::sint64 beforeTailIndex = coll.prevElementIndex(tailIndex);
        QPI::sint64 twoBeforeTailIndex = coll.prevElementIndex(beforeTailIndex);
        QPI::sint64 tailPrio = coll.priority(tailIndex);
        QPI::sint64 twoBeforeTailPrio = coll.priority(twoBeforeTailIndex);
        int tailValue = coll.element(tailIndex);
        int twoBeforeTailValue = coll.element(twoBeforeTailIndex);
        EXPECT_EQ(coll.population(), coll.capacity() - 2);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 2);
        coll.remove(beforeTailIndex);
        EXPECT_EQ(coll.population(), coll.capacity() - 3);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 3);

        checkPriorityQueue(coll, pov);

        tailIndex = coll.tailIndex(pov);
        beforeTailIndex = coll.prevElementIndex(tailIndex);
        EXPECT_EQ(coll.nextElementIndex(tailIndex), QPI::NULL_INDEX);
        EXPECT_EQ(coll.priority(tailIndex), tailPrio);
        EXPECT_EQ(coll.priority(beforeTailIndex), twoBeforeTailPrio);
        EXPECT_EQ(coll.element(tailIndex), tailValue);
        EXPECT_EQ(coll.element(beforeTailIndex), twoBeforeTailValue);
    }

    // add new highest and lowest priority element and remove others to trigger uncovered case of moving
    // last element to other index
    {
        int newValue1 = 4278956;
        QPI::sint64 newPrio1 = 10000000000ll;
        QPI::sint64 newIdx1 = coll.add(pov, newValue1, newPrio1);
        EXPECT_EQ(newIdx1, coll.population() - 1);
        int newValue2 = 2568956;
        QPI::sint64 newPrio2 = -10000000000ll;
        QPI::sint64 newIdx2 = coll.add(pov, newValue2, newPrio2);
        EXPECT_EQ(newIdx2, coll.population() - 1);

        EXPECT_EQ(coll.population(), coll.capacity() - 1);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 1);

        checkPriorityQueue(coll, pov);
        coll.remove(0);
        checkPriorityQueue(coll, pov);
        coll.remove(1);
        checkPriorityQueue(coll, pov);

        EXPECT_EQ(coll.population(), coll.capacity() - 3);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 3);

        newIdx1 = coll.tailIndex(pov);
        newIdx2 = coll.headIndex(pov);
        EXPECT_EQ(coll.priority(newIdx1), newPrio1);
        EXPECT_EQ(coll.priority(newIdx2), newPrio2);
        EXPECT_EQ(coll.element(newIdx1), newValue1);
        EXPECT_EQ(coll.element(newIdx2), newValue2);
    }

    // remove remaining elements except last
    while (coll.population() > 1)
    {
        coll.remove(0);
        checkPriorityQueue(coll, pov);
        EXPECT_EQ(coll.population(), coll.population(pov));
    }

    // remove last element
    {
        EXPECT_EQ(coll.headIndex(pov), 0);
        EXPECT_EQ(coll.tailIndex(pov), 0);
        coll.remove(0);
        checkPriorityQueue(coll, pov);
        EXPECT_EQ(coll.headIndex(pov), QPI::NULL_INDEX);
        EXPECT_EQ(coll.tailIndex(pov), QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(), 0);
        EXPECT_EQ(coll.population(pov), 0);
    }

    // test that removing element from empty collection has no effect
    {
        coll.remove(0);
        checkPriorityQueue(coll, pov);
        EXPECT_EQ(coll.headIndex(pov), QPI::NULL_INDEX);
        EXPECT_EQ(coll.tailIndex(pov), QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(), 0);
        EXPECT_EQ(coll.population(pov), 0);
    }

    // check that cleanup after removing all elements leads to same as reset() in terms of memory
    QPI::collection<int, capacity> resetColl;
    resetColl.reset();
    EXPECT_FALSE(isCompletelySame(resetColl, coll));
    coll.cleanup();
    EXPECT_TRUE(isCompletelySame(resetColl, coll));
}

TEST(TestCoreQPI, CollectionMultiPovOneElement) {
    constexpr unsigned long long capacity = 32;

    // for valid init you either need to call reset or load the data from a file (in SC, state is zeroed before INITIALIZE is called)
    QPI::collection<int, capacity> coll;
    coll.reset();

    for (int i = 0; i < capacity; ++i)
    {
        // select pov to ensure hash collisions
        QPI::id pov(i / 2, i % 2, i * 2, i * 3);
        int value = i * 4;
        QPI::sint64 prio = i * 5;

        EXPECT_EQ(coll.capacity(), capacity);
        EXPECT_EQ(coll.population(pov), 0);
        EXPECT_EQ(coll.population(), i);

        QPI::sint64 elementIndex = coll.add(pov, value, prio);

        EXPECT_TRUE(elementIndex != QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(pov), 1);
        EXPECT_EQ(coll.population(), i + 1);

        EXPECT_EQ(coll.element(elementIndex), value);
        EXPECT_EQ(coll.priority(elementIndex), prio);
        EXPECT_EQ(coll.pov(elementIndex), pov);
        checkPriorityQueue(coll, pov, false);
    }

    // check that nothing can be added
    QPI::sint64 elementIndex = coll.add(QPI::id(1, 2, 3, 4), 12345, 123456);
    EXPECT_TRUE(elementIndex == QPI::NULL_INDEX);
    EXPECT_EQ(coll.capacity(), coll.population());

    // check and remove
    for (int j = 0; j < capacity; ++j)
    {
        // check integrity of povs not removed yet
        for (int i = j; i < capacity; ++i)
        {
            QPI::id pov(i / 2, i % 2, i * 2, i * 3);
            int value = i * 4;
            QPI::sint64 prio = i * 5;

            QPI::sint64 elementIndex = coll.headIndex(pov);
            EXPECT_NE(elementIndex, -1);
            EXPECT_EQ(elementIndex, coll.tailIndex(pov));

            EXPECT_EQ(coll.element(elementIndex), value);
            EXPECT_EQ(coll.priority(elementIndex), prio);
            EXPECT_EQ(coll.pov(elementIndex), pov);
            checkPriorityQueue(coll, pov, false);
        }

        // remove
        QPI::id removePov(j / 2, j % 2, j * 2, j * 3);
        EXPECT_EQ(coll.population(removePov), 1);
        coll.remove(coll.headIndex(removePov));
        EXPECT_EQ(coll.population(removePov), 0);
        EXPECT_EQ(coll.population(), capacity - j - 1);
    }

    // check that cleanup after removing all elements leads to same as reset() in terms of memory
    QPI::collection<int, capacity> resetColl;
    resetColl.reset();
    EXPECT_FALSE(isCompletelySame(resetColl, coll));
    coll.cleanup();
    EXPECT_TRUE(isCompletelySame(resetColl, coll));
}

TEST(TestCoreQPI, CollectionOneRemoveLastHeadTail) {
    // Minimal test cases for bug fixed in
    // https://github.com/qubic/core/commit/b379a36666f747b25992d025dd68949b771b1cd0#diff-2435a5cdb31de2a231e71d143e1cba9e4f9207181a6223d736293d40da41d002

    QPI::id pov(1, 2, 3, 4);
    constexpr unsigned long long capacity = 4;

    // for valid init you either need to call reset or load the data from a file (in SC, state is zeroed before INITIALIZE is called)
    QPI::collection<int, capacity> coll;
    coll.reset();

    bool print = false;
    coll.add(pov, 1234, 1000);
    coll.add(pov, 1234, 10000);
    checkPriorityQueue(coll, pov, print);
    coll.remove(1);
    checkPriorityQueue(coll, pov, print);

    coll.reset();
    coll.add(pov, 1234, 10000);
    coll.add(pov, 1234, 1000);
    checkPriorityQueue(coll, pov, print);
    coll.remove(1);
    checkPriorityQueue(coll, pov, print);
}

template <unsigned long long capacity>
void testCollectionCleanupPseudoRandom(int povs, int seed)
{
    // add and remove entries with pseudo-random sequence
    std::mt19937_64 gen64(seed);

    QPI::collection<unsigned long long, capacity> coll;
    coll.reset();

    // test cleanup of empty collection
    cleanupCollection(coll);

    int cleanupCounter = 0;
    while (cleanupCounter < 100)
    {
        int p = gen64() % 100;

        if (p == 0)
        {
            // cleanup (after about 100 add/remove)
            cleanupCollection(coll);
            ++cleanupCounter;
        }

        if (p < 70)
        {
            // add to collection (more probable than remove)
            QPI::id pov(gen64() % povs, 0, 0, 0);
            coll.add(pov, gen64(), gen64());
        }
        else if (coll.population() > 0)
        {
            // remove from collection
            coll.remove(gen64() % coll.population());
        }

    }
}

TEST(TestCoreQPI, CollectionCleanup) {
    __scratchpadBuffer = new char[10 * 1024 * 1024];
    for (int i = 0; i < 3; ++i)
    {
        testCollectionCleanupPseudoRandom<512>(300, 12345 + i);
        testCollectionCleanupPseudoRandom<256>(256, 1234 + i);
        testCollectionCleanupPseudoRandom<256>(10, 123 + i);
    }
    delete[] __scratchpadBuffer;
}


TEST(TestCoreQPI, Div) {
    EXPECT_EQ(QPI::div(0, 0), 0);
    EXPECT_EQ(QPI::div(10, 0), 0);
    EXPECT_EQ(QPI::div(0, 10), 0);
    EXPECT_EQ(QPI::div(20, 19), 1);
    EXPECT_EQ(QPI::div(20, 20), 1);
    EXPECT_EQ(QPI::div(20, 21), 0);
    EXPECT_EQ(QPI::div(20, 22), 0);
    EXPECT_EQ(QPI::div(50, 24), 2);
    EXPECT_EQ(QPI::div(50, 25), 2);
    EXPECT_EQ(QPI::div(50, 26), 1);
    EXPECT_EQ(QPI::div(50, 27), 1);
    EXPECT_EQ(QPI::div(-2, 0), 0);
    EXPECT_EQ(QPI::div(-2, 1), -2);
    EXPECT_EQ(QPI::div(-2, 2), -1);
    EXPECT_EQ(QPI::div(-2, 3), 0);
    EXPECT_EQ(QPI::div(2, -3), 0);
    EXPECT_EQ(QPI::div(2, -2), -1);
    EXPECT_EQ(QPI::div(2, -1), -2);

    EXPECT_EQ(QPI::div(0.0, 0.0), 0.0);
    EXPECT_EQ(QPI::div(50.0, 0.0), 0.0);
    EXPECT_EQ(QPI::div(0.0, 50.0), 0.0);
    EXPECT_EQ(QPI::div(-25.0, 50.0), -0.5);
    EXPECT_EQ(QPI::div(-25.0, -0.5), 50.0);
}

TEST(TestCoreQPI, Mod) {
    EXPECT_EQ(QPI::mod(0, 0), 0);
    EXPECT_EQ(QPI::mod(10, 0), 0);
    EXPECT_EQ(QPI::mod(0, 10), 0);
    EXPECT_EQ(QPI::mod(20, 19), 1);
    EXPECT_EQ(QPI::mod(20, 20), 0);
    EXPECT_EQ(QPI::mod(20, 21), 20);
    EXPECT_EQ(QPI::mod(20, 22), 20);
    EXPECT_EQ(QPI::mod(50, 23), 4);
    EXPECT_EQ(QPI::mod(50, 24), 2);
    EXPECT_EQ(QPI::mod(50, 25), 0);
    EXPECT_EQ(QPI::mod(50, 26), 24);
    EXPECT_EQ(QPI::mod(50, 27), 23);
    EXPECT_EQ(QPI::mod(-2, 0), 0);
    EXPECT_EQ(QPI::mod(-2, 1), 0);
    EXPECT_EQ(QPI::mod(-2, 2), 0);
    EXPECT_EQ(QPI::mod(-2, 3), -2);
    EXPECT_EQ(QPI::mod(2, -3), 2);
    EXPECT_EQ(QPI::mod(2, -2), 0);
    EXPECT_EQ(QPI::mod(2, -1), 0);
}
