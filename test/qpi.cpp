#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/smart_contracts/qpi.h"

#include <vector>

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
    QPI::sint64 prevElementIdx;
    while (elementIndex != QPI::NULL_INDEX)
    {
        if (print)
        {
            std::cout << "\tindex " << elementIndex << ", value " << coll.element(elementIndex)
                << ", priority " << coll.priority(elementIndex)
                << ", prev " << coll.prevElementIndex(elementIndex)
                << ", next " << coll.nextElementIndex(elementIndex) << std::endl;
        }

        if (first)
        {
            EXPECT_EQ(coll.prevElementIndex(elementIndex), QPI::NULL_INDEX);
        }
        else
        {
            EXPECT_EQ(coll.prevElementIndex(elementIndex), prevElementIdx);
            EXPECT_GE(coll.priority(elementIndex), prevPriority);
        }
        EXPECT_EQ(coll.pov(elementIndex), pov);

        prevElementIdx = elementIndex;
        prevPriority = coll.priority(elementIndex);

        first = false;
        elementIndex = coll.nextElementIndex(elementIndex);
    }
    EXPECT_EQ(prevElementIdx, coll.tailIndex(pov));
}


TEST(TestCoreQPI, CollectionMultiPovMultiElements) {
    QPI::id id1(1, 2, 3, 4);
    QPI::id id2(3, 100, 579, 5431);
    QPI::id id3(1, 100, 579, 5431);
    
    constexpr unsigned long long capacity = 8;

    // for valid init you either need to call reset or load the data from a file
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
    EXPECT_EQ(coll.pov(0), QPI::id(0, 0, 0, 0));
    EXPECT_EQ(coll.element(0), 0);
    EXPECT_EQ(coll.nextElementIndex(0), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(0), QPI::NULL_INDEX);
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
    EXPECT_EQ(coll.pov(0), QPI::id(0, 0, 0, 0));
    EXPECT_EQ(coll.element(0), 0);
    EXPECT_EQ(coll.nextElementIndex(0), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(0), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(0), 0);
}


TEST(TestCoreQPI, CollectionOnePovMultiElements) {
    constexpr unsigned long long capacity = 128;

    // for valid init you either need to call reset or load the data from a file
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
}

TEST(TestCoreQPI, CollectionMultiPovOneElement) {
    constexpr unsigned long long capacity = 32;

    // for valid init you either need to call reset or load the data from a file
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
}
