#define NO_UEFI

#include "gtest/gtest.h"

void* __scratchpadBuffer = nullptr;
static void* __scratchpad()
{
    return __scratchpadBuffer;
}
namespace QPI
{
    struct QpiContextProcedureCall;
    struct QpiContextFunctionCall;
}
typedef void (*USER_FUNCTION)(const QPI::QpiContextFunctionCall&, void* state, void* input, void* output, void* locals);
typedef void (*USER_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);

#include "../src/contracts/qpi.h"
#include "../src/contract_core/qpi_collection_impl.h"
#include "../src/contract_core/qpi_proposal_voting.h"

struct QpiContextUserProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextUserProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QPI::QpiContextProcedureCall(contractIndex, originator, invocationReward)
    {
    }
};

// changing offset simulates changed computor set with changed epoch
static int computorIdOffset = 0;

QPI::id QPI::QpiContextFunctionCall::computor(unsigned short computorIndex) const
{
    return QPI::id(computorIndex + computorIdOffset, 9, 8, 7);
}

QPI::uint16 currentEpoch = 12345;

QPI::uint16 QPI::QpiContextFunctionCall::epoch() const
{
    return currentEpoch;
}



#include <vector>
#include <map>
#include <random>
#include <chrono>

template <typename T, unsigned long long capacity>
void checkPriorityQueue(const QPI::collection<T, capacity>& coll, const QPI::id& pov, bool print = false)
{
    if (print)
    {
        std::cout << "Priority queue ID(" << pov.u64._0 << ", " << pov.u64._1 << ", "
            << pov.u64._2 << ", " << pov.u64._3 << ")" << std::endl;
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
            EXPECT_LE(coll.priority(elementIndex), prevPriority);
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
        std::cout << "\t(" << id.u64._0 << ", " << id.u64._1 << ", " << id.u64._2 << ", " << id.u64._3 << "): " << count << std::endl;
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
    // id1 priority queue order: secondElement, firstElement
    constexpr int secondElementValue = 987;
    constexpr QPI::sint64 secondElementPriority = 12345;
    QPI::sint64 secondElementIdx = coll.add(id1, secondElementValue, secondElementPriority);
    EXPECT_TRUE(secondElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(secondElementIdx != firstElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 2);
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), firstElementIdx);
    EXPECT_EQ(coll.population(id1), 2);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id2), 0);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);

    // add another element with id1, but lower priority value
    // id1 priority queue order: secondElement, firstElement, thirdElement
    constexpr int thirdElementValue = 98;
    constexpr QPI::sint64 thirdElementPriority = 12;
    QPI::sint64 thirdElementIdx = coll.add(id1, thirdElementValue, thirdElementPriority);
    EXPECT_TRUE(thirdElementIdx != QPI::NULL_INDEX);
    EXPECT_TRUE(thirdElementIdx != firstElementIdx);
    EXPECT_TRUE(thirdElementIdx != secondElementIdx);
    EXPECT_EQ(coll.capacity(), capacity);
    EXPECT_EQ(coll.population(), 3);
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), thirdElementIdx);
    EXPECT_EQ(coll.population(id1), 3);
    EXPECT_EQ(coll.headIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id2), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id2), 0);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), firstElementIdx);
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
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), thirdElementIdx);
    EXPECT_EQ(coll.population(id1), 3);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.tailIndex(id3), QPI::NULL_INDEX);
    EXPECT_EQ(coll.population(id3), 0);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), firstElementIdx);
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
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), thirdElementIdx);
    EXPECT_EQ(coll.population(id1), 3);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.population(id3), 1);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), firstElementIdx);
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
    // id1 priority queue order: secondElement, firstElement, thirdElement, sixthElement
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
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), sixthElementIdx);
    EXPECT_EQ(coll.population(id1), 4);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.population(id3), 1);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), sixthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), firstElementIdx);
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
    EXPECT_EQ(coll.nextElementIndex(sixthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(sixthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(sixthElementIdx), sixthElementPriority);

    // add another element with id3, with highest priority value
    // id3 priority queue order: seventhElement, fifthElement
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
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), sixthElementIdx);
    EXPECT_EQ(coll.population(id1), 4);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), seventhElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.population(id3), 2);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), sixthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), firstElementIdx);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);
    EXPECT_EQ(coll.pov(fifthElementIdx), id3);
    EXPECT_EQ(coll.element(fifthElementIdx), fifthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fifthElementIdx), seventhElementIdx);
    EXPECT_EQ(coll.priority(fifthElementIdx), fifthElementPriority);
    EXPECT_EQ(coll.pov(sixthElementIdx), id1);
    EXPECT_EQ(coll.element(sixthElementIdx), sixthElementValue);
    EXPECT_EQ(coll.nextElementIndex(sixthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(sixthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(sixthElementIdx), sixthElementPriority);
    EXPECT_EQ(coll.pov(seventhElementIdx), id3);
    EXPECT_EQ(coll.element(seventhElementIdx), seventhElementValue);
    EXPECT_EQ(coll.nextElementIndex(seventhElementIdx), fifthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(seventhElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(seventhElementIdx), seventhElementPriority);

    // add another element with id1, with medium priority value
    // id1 priority queue order: secondElement, firstElement,   eighthElement,  thirdElement,   sixthElement
    //               priorities: 12345,         1234,           123,            12,             -60
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
    EXPECT_EQ(coll.headIndex(id1), secondElementIdx);
    EXPECT_EQ(coll.tailIndex(id1), sixthElementIdx);
    EXPECT_EQ(coll.population(id1), 5);
    EXPECT_EQ(coll.headIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.tailIndex(id2), fourthElementIdx);
    EXPECT_EQ(coll.population(id2), 1);
    EXPECT_EQ(coll.headIndex(id3), seventhElementIdx);
    EXPECT_EQ(coll.tailIndex(id3), fifthElementIdx);
    EXPECT_EQ(coll.population(id3), 2);
    EXPECT_EQ(coll.pov(firstElementIdx), id1);
    EXPECT_EQ(coll.element(firstElementIdx), firstElementValue);
    EXPECT_EQ(coll.nextElementIndex(firstElementIdx), eighthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(firstElementIdx), secondElementIdx);
    EXPECT_EQ(coll.priority(firstElementIdx), firstElementPriority);
    EXPECT_EQ(coll.pov(secondElementIdx), id1);
    EXPECT_EQ(coll.element(secondElementIdx), secondElementValue);
    EXPECT_EQ(coll.nextElementIndex(secondElementIdx), firstElementIdx);
    EXPECT_EQ(coll.prevElementIndex(secondElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(secondElementIdx), secondElementPriority);
    EXPECT_EQ(coll.pov(thirdElementIdx), id1);
    EXPECT_EQ(coll.element(thirdElementIdx), thirdElementValue);
    EXPECT_EQ(coll.nextElementIndex(thirdElementIdx), sixthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(thirdElementIdx), eighthElementIdx);
    EXPECT_EQ(coll.priority(thirdElementIdx), thirdElementPriority);
    EXPECT_EQ(coll.pov(fourthElementIdx), id2);
    EXPECT_EQ(coll.element(fourthElementIdx), fourthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fourthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(fourthElementIdx), fourthElementPriority);
    EXPECT_EQ(coll.pov(fifthElementIdx), id3);
    EXPECT_EQ(coll.element(fifthElementIdx), fifthElementValue);
    EXPECT_EQ(coll.nextElementIndex(fifthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(fifthElementIdx), seventhElementIdx);
    EXPECT_EQ(coll.priority(fifthElementIdx), fifthElementPriority);
    EXPECT_EQ(coll.pov(sixthElementIdx), id1);
    EXPECT_EQ(coll.element(sixthElementIdx), sixthElementValue);
    EXPECT_EQ(coll.nextElementIndex(sixthElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(sixthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.priority(sixthElementIdx), sixthElementPriority);
    EXPECT_EQ(coll.pov(seventhElementIdx), id3);
    EXPECT_EQ(coll.element(seventhElementIdx), seventhElementValue);
    EXPECT_EQ(coll.nextElementIndex(seventhElementIdx), fifthElementIdx);
    EXPECT_EQ(coll.prevElementIndex(seventhElementIdx), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(seventhElementIdx), seventhElementPriority);
    EXPECT_EQ(coll.pov(eighthElementIdx), id1);
    EXPECT_EQ(coll.element(eighthElementIdx), eighthElementValue);
    EXPECT_EQ(coll.nextElementIndex(eighthElementIdx), thirdElementIdx);
    EXPECT_EQ(coll.prevElementIndex(eighthElementIdx), firstElementIdx);
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
    EXPECT_EQ(coll.nextElementIndex(0), QPI::NULL_INDEX);
    EXPECT_EQ(coll.prevElementIndex(0), QPI::NULL_INDEX);
    EXPECT_EQ(coll.priority(0), 0);
}

template <unsigned long long capacity>
void testCollectionOnePovMultiElements(int prioAmpFactor, int prioFreqDiv)
{
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
        QPI::sint64 prio = QPI::sint64(i * prioAmpFactor * sin(i / prioFreqDiv));
        int value = i * 4;

        EXPECT_EQ(coll.capacity(), capacity);
        EXPECT_EQ(coll.population(), i);
        EXPECT_EQ(coll.population(pov), i);

        QPI::sint64 elementIndex = coll.add(pov, value, prio);
        elementIndices.push_back(elementIndex);
        checkPriorityQueue(coll, pov);

        EXPECT_TRUE(elementIndex != QPI::NULL_INDEX);
        EXPECT_EQ(coll.priority(elementIndex), prio);
        EXPECT_EQ(coll.element(elementIndex), value);
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
        QPI::sint64 prio = QPI::sint64(i * prioAmpFactor * sin(i / prioFreqDiv));
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
        QPI::sint64 followingRemovedIndex = coll.remove(headIndex);
        EXPECT_EQ(coll.priority(followingRemovedIndex), afterHeadPrio);
        EXPECT_EQ(coll.element(followingRemovedIndex), afterHeadValue);
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
        QPI::sint64 followingRemovedIndex = coll.remove(tailIndex);
        EXPECT_EQ(followingRemovedIndex, QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(), coll.capacity() - 2);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 2);

        checkPriorityQueue(coll, pov);

        tailIndex = coll.tailIndex(pov);
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
        QPI::sint64 followingRemovedIndex = coll.remove(beforeTailIndex);
        EXPECT_EQ(coll.priority(followingRemovedIndex), tailPrio);
        EXPECT_EQ(coll.element(followingRemovedIndex), tailValue);
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

        // remove one
        int followingRemovedValue = coll.element(coll.nextElementIndex(0));
        QPI::sint64 followingRemovedPrio = coll.priority(coll.nextElementIndex(0));
        QPI::sint64 followingRemovedIndex = coll.remove(0);
        EXPECT_EQ(followingRemovedValue, coll.element(followingRemovedIndex));
        EXPECT_EQ(followingRemovedPrio, coll.priority(followingRemovedIndex));
        checkPriorityQueue(coll, pov);

        // remove another (not head or tail!)
        QPI::sint64 removeIdx = followingRemovedIndex;
        followingRemovedValue = coll.element(coll.nextElementIndex(removeIdx));
        followingRemovedPrio = coll.priority(coll.nextElementIndex(removeIdx));
        followingRemovedIndex = coll.remove(removeIdx);
        EXPECT_EQ(followingRemovedValue, coll.element(followingRemovedIndex));
        EXPECT_EQ(followingRemovedPrio, coll.priority(followingRemovedIndex));
        checkPriorityQueue(coll, pov);

        EXPECT_EQ(coll.population(), coll.capacity() - 3);
        EXPECT_EQ(coll.population(pov), coll.capacity() - 3);

        // check tail and head
        newIdx1 = coll.tailIndex(pov);
        newIdx2 = coll.headIndex(pov);
        EXPECT_EQ(coll.priority(newIdx1), newPrio2);
        EXPECT_EQ(coll.priority(newIdx2), newPrio1);
        EXPECT_EQ(coll.element(newIdx1), newValue2);
        EXPECT_EQ(coll.element(newIdx2), newValue1);
    }

    // remove remaining elements except last
    while (coll.population() > 1)
    {
        int followingRemovedValue = coll.element(coll.nextElementIndex(0));
        QPI::sint64 followingRemovedPrio = coll.priority(coll.nextElementIndex(0));
        QPI::sint64 followingRemovedIndex = coll.remove(0);
        EXPECT_EQ(followingRemovedValue, coll.element(followingRemovedIndex));
        EXPECT_EQ(followingRemovedPrio, coll.priority(followingRemovedIndex));
        checkPriorityQueue(coll, pov);
        EXPECT_EQ(coll.population(), coll.population(pov));
    }

    // remove last element
    {
        EXPECT_EQ(coll.headIndex(pov), 0);
        EXPECT_EQ(coll.tailIndex(pov), 0);
        EXPECT_EQ(coll.remove(0), QPI::NULL_INDEX);
        checkPriorityQueue(coll, pov);
        EXPECT_EQ(coll.headIndex(pov), QPI::NULL_INDEX);
        EXPECT_EQ(coll.tailIndex(pov), QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(), 0);
        EXPECT_EQ(coll.population(pov), 0);
    }

    // test that removing element from empty collection has no effect
    {
        EXPECT_EQ(coll.remove(0), QPI::NULL_INDEX);
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

TEST(TestCoreQPI, CollectionOnePovMultiElements)
{
    testCollectionOnePovMultiElements<16>(10, 3);
    testCollectionOnePovMultiElements<128>(10, 3);
    testCollectionOnePovMultiElements<128>(10, 10);
    testCollectionOnePovMultiElements<128>(1, 10);
    testCollectionOnePovMultiElements<128>(1, 1);
}

TEST(TestCoreQPI, CollectionOnePovMultiElementsSamePrioOrder)
{
    constexpr unsigned long long capacity = 16;

    // for valid init you either need to call reset or load the data from a file (in SC, state is zeroed before INITIALIZE is called)
    QPI::collection<int, capacity> coll;
    coll.reset();

    // these tests support changing the implementation of the element array filling to non-sequential
    // by saving element indices in order
    std::vector<QPI::sint64> elementIndices;

    // fill completely with same priority
    QPI::id pov(1, 2, 3, 4);
    constexpr QPI::sint64 prio = 100;
    for (int i = 0; i < capacity; ++i)
    {
        int value = i * 3;

        EXPECT_EQ(coll.capacity(), capacity);
        EXPECT_EQ(coll.population(), i);
        EXPECT_EQ(coll.population(pov), i);

        QPI::sint64 elementIndex = coll.add(pov, value, prio);
        elementIndices.push_back(elementIndex);
        checkPriorityQueue(coll, pov);

        EXPECT_TRUE(elementIndex != QPI::NULL_INDEX);
        EXPECT_EQ(coll.element(elementIndex), value);
        EXPECT_EQ(coll.priority(elementIndex), prio);
        EXPECT_EQ(coll.population(pov), i + 1);
        EXPECT_EQ(coll.population(), i + 1);
    }

    // check that priority queue order of same priorty items matches the order of insertion
    QPI::sint64 elementIndex = coll.headIndex(pov);
    for (int i = 0; i < capacity; ++i)
    {
        int value = i * 3;
        EXPECT_NE(elementIndex, QPI::NULL_INDEX);
        EXPECT_EQ(coll.element(elementIndex), value);
        EXPECT_EQ(coll.priority(elementIndex), prio);
        elementIndex = coll.nextElementIndex(elementIndex);
    }
    EXPECT_EQ(elementIndex, QPI::NULL_INDEX);
}

template <unsigned long long capacity>
void testCollectionMultiPovOneElement(bool cleanupAfterEachRemove)
{
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
        EXPECT_EQ(coll.remove(coll.headIndex(removePov)), QPI::NULL_INDEX);
        EXPECT_EQ(coll.population(removePov), 0);
        EXPECT_EQ(coll.population(), capacity - j - 1);

        if (cleanupAfterEachRemove)
            coll.cleanup();
    }

    // check that cleanup after removing all elements leads to same as reset() in terms of memory
    QPI::collection<int, capacity> resetColl;
    resetColl.reset();
    if (!cleanupAfterEachRemove)
        EXPECT_FALSE(isCompletelySame(resetColl, coll));
    EXPECT_TRUE(haveSameContent(resetColl, coll, true));
    coll.cleanup();
    EXPECT_TRUE(isCompletelySame(resetColl, coll));
}

TEST(TestCoreQPI, CollectionMultiPovOneElement)
{
    bool cleanupAfterEachRemove = false;
    testCollectionMultiPovOneElement<16>(cleanupAfterEachRemove);
    testCollectionMultiPovOneElement<32>(cleanupAfterEachRemove);
    testCollectionMultiPovOneElement<64>(cleanupAfterEachRemove);
    testCollectionMultiPovOneElement<128>(cleanupAfterEachRemove);
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
    EXPECT_EQ(coll.remove(1), 0);
    checkPriorityQueue(coll, pov, print);

    coll.reset();
    coll.add(pov, 1234, 10000);
    coll.add(pov, 1234, 1000);
    checkPriorityQueue(coll, pov, print);
    EXPECT_EQ(coll.remove(1), QPI::NULL_INDEX);
    checkPriorityQueue(coll, pov, print);
}

TEST(TestCoreQPI, CollectionSubCollections) {
    QPI::id pov(1, 2, 3, 4);

    QPI::collection<size_t, 512> coll;
    coll.reset();

    // test empty
    auto headIdx = coll.headIndex(pov, 0);
    EXPECT_EQ(headIdx, QPI::NULL_INDEX);
    auto tailIdx = coll.tailIndex(pov, 0);
    EXPECT_EQ(tailIdx, QPI::NULL_INDEX);

    std::vector<QPI::sint64> priorities = {
        44, 22, 88, 111, 55, 56, 11, 55, 55, 54, 66, 77, 99
    };

    for (size_t i = 0; i < priorities.size(); i++)
    {
        coll.add(pov, i, priorities[i]);
    }
    checkPriorityQueue(coll, pov, false);

    // sorted priorities: 111, 99, 88, 77, 66, .... 44, 22, 11

    // test head/tail
    headIdx = coll.headIndex(pov);
    EXPECT_EQ(coll.priority(headIdx), 111);
    tailIdx = coll.tailIndex(pov);
    EXPECT_EQ(coll.priority(tailIdx), 11);

    // test prev/next
    auto idx = coll.prevElementIndex(tailIdx);
    idx = coll.prevElementIndex(idx);
    EXPECT_EQ(coll.priority(idx), 44);
    idx = coll.nextElementIndex(headIdx);
    idx = coll.nextElementIndex(idx);
    EXPECT_EQ(coll.priority(idx), 88);

    // test sub-collection's head priority <= maxPriority
    headIdx = coll.headIndex(pov, 112);
    EXPECT_EQ(coll.priority(headIdx), 111);

    // test sub-collection's tail priority > maxPriority
    headIdx = coll.headIndex(pov, 10);
    EXPECT_EQ(headIdx, QPI::NULL_INDEX);

    // test sub-collection's head priority < minPriority
    tailIdx = coll.tailIndex(pov, 112);
    EXPECT_EQ(tailIdx, QPI::NULL_INDEX);

    // test sub-collection's tail priority >= minPriority
    tailIdx = coll.tailIndex(pov, 10);
    EXPECT_EQ(coll.priority(tailIdx), 11);

    // test sub-collection's head
    headIdx = coll.headIndex(pov, 100);
    EXPECT_EQ(coll.priority(headIdx), 99);
    headIdx = coll.headIndex(pov, 99);
    EXPECT_EQ(coll.priority(headIdx), 99);

    // test sub-collection's head: duplicated priorites
    headIdx = coll.headIndex(pov, 55);
    EXPECT_EQ(coll.priority(headIdx), 55);
    idx = coll.prevElementIndex(headIdx);
    EXPECT_EQ(coll.priority(idx), 56);

    // test sub-collection's tail
    tailIdx = coll.tailIndex(pov, 33);
    EXPECT_EQ(coll.priority(tailIdx), 44);
    tailIdx = coll.tailIndex(pov, 44);
    EXPECT_EQ(coll.priority(tailIdx), 44);

    // test sub-collection's tail: duplicated priorites
    tailIdx = coll.tailIndex(pov, 55);
    EXPECT_EQ(coll.priority(tailIdx), 55);
    idx = coll.nextElementIndex(tailIdx);
    EXPECT_EQ(coll.priority(idx), 54);
}

TEST(TestCoreQPI, CollectionSubCollectionsRandom) {
    QPI::id pov(1, 2, 3, 4);

    QPI::collection<size_t, 1024> coll;
    coll.reset();

    const int seed = 246357;
    std::mt19937_64 gen64(seed);

    const int numTests = 10;
    for (int test = 1; test <= numTests; test++)
    {
        coll.reset();
        std::vector< QPI::sint64> priorities(777);
        for (size_t i = 0; i < priorities.size(); i++) {
            auto v = std::abs((QPI::sint64)gen64()) % 0xFFFF;
            priorities[i] = v;
            coll.add(pov, (i + 1) * test, priorities[i]);
        }
        checkPriorityQueue(coll, pov, false);

        std::sort(priorities.begin(), priorities.end(), std::greater<>());
        const auto size = priorities.size();

        // test sub-collection's head priority <= maxPriority
        auto headIdx = coll.headIndex(pov, priorities[0] + 1);
        EXPECT_EQ(coll.priority(headIdx), priorities[0]);
        headIdx = coll.headIndex(pov, priorities[0]);
        EXPECT_EQ(coll.priority(headIdx), priorities[0]);

        // test sub-collection's tail priority > maxPriority
        headIdx = coll.headIndex(pov, priorities[size - 1] - 1);
        EXPECT_EQ(headIdx, QPI::NULL_INDEX);

        // test sub-collection's head priority < minPriority
        auto tailIdx = coll.tailIndex(pov, priorities[0] + 1);
        EXPECT_EQ(tailIdx, QPI::NULL_INDEX);

        // test sub-collection's tail priority >= minPriority
        tailIdx = coll.tailIndex(pov, priorities[size - 1] - 1);
        EXPECT_EQ(coll.priority(tailIdx), priorities[size - 1]);
        tailIdx = coll.tailIndex(pov, priorities[size - 1]);
        EXPECT_EQ(coll.priority(tailIdx), priorities[size - 1]);

        std::vector<size_t> indices(std::min(size, std::max(1llu, size / 5)));
        for (size_t i = 0; i < indices.size(); i++) {
            indices[i] = std::abs((QPI::sint64)gen64()) % indices.size();
        }

        for (size_t i : indices)
        {
            const auto priority = priorities[i];

            // test sub-collection: head
            auto idx = coll.headIndex(pov, priority);
            EXPECT_EQ(coll.priority(idx), priority);

            // test sub-collection: higher priority
            if (idx != coll.headIndex(pov))
            {
                auto higher_priority = priority;
                for (size_t j = i - 1; j >= 0; j--)
                {
                    if (priorities[j] > priority)
                    {
                        higher_priority = priorities[j];
                        break;
                    }
                }
                auto prev_idx = coll.prevElementIndex(idx);
                EXPECT_EQ(coll.priority(prev_idx), higher_priority);
            }

            // test sub-collection: tail
            idx = coll.tailIndex(pov, priority);
            EXPECT_EQ(coll.priority(idx), priority);

            // test sub-collection: lower priority
            if (idx != coll.tailIndex(pov))
            {
                auto lower_priority = priority;
                for (size_t j = i + 1; j < size; j++)
                {
                    if (priorities[j] < priority)
                    {
                        lower_priority = priorities[j];
                        break;
                    }
                }
                auto next_idx = coll.nextElementIndex(idx);
                EXPECT_EQ(coll.priority(next_idx), lower_priority);
            }
        }
    }
}

TEST(TestCoreQPI, CollectionReplaceElements) {
    QPI::id pov(1, 2, 3, 4);

    QPI::collection<size_t, 1024> coll;
    coll.reset();

    const int seed = 246357;
    std::mt19937_64 gen64(seed);

    // init a collection for test
    const size_t numElements = 1000;
    std::vector<QPI::sint64> priorities(numElements);
    for (size_t i = 0; i < numElements; i++)
    {
        priorities[i] = std::abs((QPI::sint64)gen64()) % 0xFFFF;
        coll.add(pov, i, priorities[i]);
    }
    checkPriorityQueue(coll, pov, false);

    // test for special cases out of bound element index
    size_t replaceElement = 999;

    // out of collection's capacity
    coll.replace(coll.capacity(), replaceElement);
    for (size_t i = 0; i < numElements; i++)
    {
        EXPECT_EQ(coll.element(i), i);
    }

    // out of  collection's size
    coll.replace(numElements, replaceElement);
    for (size_t i = 0; i < numElements; i++)
    {
        EXPECT_EQ(coll.element(i), i);
    }

    // generate random replace indices
    const size_t numTests = 500;
    std::vector<QPI::sint64> replaceIndices(numTests);
    for (size_t i = 0; i < numTests; i++)
    {
        replaceIndices[i] = std::abs((QPI::sint64)gen64()) % numElements;
        // remove some of indices in the list
        if (i % 4)
        {
            coll.remove(replaceIndices[i]);
        }
    }

    // get the new priorities list
    size_t numRemainedElements = coll.population();
    priorities.resize(numRemainedElements);
    for (size_t i = 0; i < numRemainedElements; i++)
    {
        priorities[i] = coll.priority(i);
    }

    // run the test on the list
    for (size_t i = 0; i < numTests; i++)
    {
        QPI::sint64 replaceElement = i;
        QPI::sint64 replaceIndex = replaceIndices[i];

        QPI::sint64 nextElementIndex = coll.nextElementIndex(replaceIndex);
        QPI::sint64 prevElementIndex = coll.prevElementIndex(replaceIndex);

        coll.replace(replaceIndex, replaceElement);

        if (size_t(replaceIndex) < numRemainedElements)
        {
            EXPECT_EQ(coll.element(replaceIndex), replaceElement);
            EXPECT_EQ(coll.priority(replaceIndex), priorities[replaceIndex]);
            EXPECT_EQ(coll.nextElementIndex(replaceIndex), nextElementIndex);
            EXPECT_EQ(coll.prevElementIndex(replaceIndex), prevElementIndex);
        }
    }
}

template <unsigned long long capacity>
void testCollectionCleanupPseudoRandom(int povs, int seed, bool povCollisions)
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
            QPI::id pov = (povCollisions) ? QPI::id(0, 0, 0, gen64() % povs) : QPI::id(gen64() % povs, 0, 0, 0);
            coll.add(pov, gen64(), gen64());
        }
        else if (coll.population() > 0)
        {
            // remove from collection (also testing next index returned by remove)
            QPI::sint64 removeIdx = gen64() % coll.population();
            QPI::sint64 followingRemovedIndex = coll.nextElementIndex(removeIdx);
            if (followingRemovedIndex != QPI::NULL_INDEX)
            {
                unsigned long long followingRemovedValue = coll.element(followingRemovedIndex);
                QPI::sint64 followingRemovedPrio = coll.priority(followingRemovedIndex);
                followingRemovedIndex = coll.remove(removeIdx);
                EXPECT_EQ(followingRemovedValue, coll.element(followingRemovedIndex));
                EXPECT_EQ(followingRemovedPrio, coll.priority(followingRemovedIndex));
            }
            else
            {
                EXPECT_EQ(coll.remove(removeIdx), QPI::NULL_INDEX);
            }
        }
    }
}

TEST(TestCoreQPI, CollectionCleanup) {
    __scratchpadBuffer = new char[10 * 1024 * 1024];
    for (int i = 0; i < 3; ++i)
    {
        bool povCollisions = false;
        testCollectionCleanupPseudoRandom<512>(300, 12345 + i, povCollisions);
        testCollectionCleanupPseudoRandom<256>(256, 1234 + i, povCollisions);
        testCollectionCleanupPseudoRandom<256>(10, 123 + i, povCollisions);
        testCollectionCleanupPseudoRandom<16>(10, 12 + i, povCollisions);

        povCollisions = true;
        testCollectionCleanupPseudoRandom<512>(300, 12345 + i, povCollisions);
        testCollectionCleanupPseudoRandom<256>(256, 1234 + i, povCollisions);
        testCollectionCleanupPseudoRandom<256>(10, 123 + i, povCollisions);
        testCollectionCleanupPseudoRandom<16>(10, 12 + i, povCollisions);
    }
    delete[] __scratchpadBuffer;
    __scratchpadBuffer = nullptr;
}

TEST(TestCoreQPI, CollectionCleanupWithPovCollisions)
{
    // Shows bugs in cleanup() that occur in case of massive pov hash map collisions and in case of capacity < 32
    __scratchpadBuffer = new char[10 * 1024 * 1024];
    bool cleanupAfterEachRemove = true;
    testCollectionMultiPovOneElement<16>(cleanupAfterEachRemove);
    testCollectionMultiPovOneElement<32>(cleanupAfterEachRemove);
    testCollectionMultiPovOneElement<64>(cleanupAfterEachRemove);
    testCollectionMultiPovOneElement<128>(cleanupAfterEachRemove);
    delete[] __scratchpadBuffer;
    __scratchpadBuffer = nullptr;
}


template<typename T>
T genNumber(
    const T* genBuffer,
    const QPI::uint64 genSize,
    QPI::uint64& idx)
{
    T val = genBuffer[idx];
    idx = (idx + 1) % genSize;
    return val;
}

template <unsigned long long capacity>
QPI::uint64 testCollectionPerformance(
    QPI::collection<QPI::uint64, capacity>& coll,
    const QPI::uint64 povs,
    const QPI::sint64* genBuffer,
    const QPI::uint64 genSize,
    const QPI::uint64 genSeed,
    const QPI::uint64 maxCleanupCounter)
{
    // add and remove entries with pseudo-random sequence
    QPI::uint64 idx = genSeed % genSize;

    // test cleanup of empty collection
    coll.cleanup();

#define GEN64 genNumber(genBuffer, genSize, idx)

    QPI::uint64 cleanupCounter = 0;
    while (cleanupCounter < maxCleanupCounter)
    {
        int p = GEN64 % 100;

        if (p == 0)
        {
            // cleanup (after about 100 add/remove)
            coll.cleanup();
            ++cleanupCounter;
        }

        if (p < 70)
        {
            // add to collection (more probable than remove)
            QPI::id pov(GEN64 % povs, 0, 0, 0);
            if (coll.add(pov, GEN64, GEN64) == QPI::NULL_INDEX)
            {
                for (int i = 0; i < 10; i++)
                {
                    p = GEN64 % 100;
                    if (p < 70)
                    {
                        if (coll.population() > 0)
                        {
                            coll.remove(GEN64 % coll.population());
                            if (!coll.population())
                            {
                                break;
                            }
                        }
                        else
                        {
                            coll.cleanup();
                            ++cleanupCounter;
                            break;
                        }
                    }
                }
            }
        }
        else if (coll.population() > 0)
        {
            // remove from collection
            coll.remove(GEN64 % coll.population());
        }
    }

    return coll.population();
}

template <unsigned long long capacity>
QPI::uint64 testCollectionPerformance(
    const QPI::uint64 maxPovsCount, const QPI::uint64 maxCleanupCounter)
{
    std::mt19937_64 gen64(113377);
    const QPI::uint64 genSize = 113377;
    QPI::sint64 gen_buffers[genSize];
    for (QPI::uint64 i = 0; i < genSize; i++)
    {
        gen_buffers[i] = gen64();
    }

    QPI::collection<QPI::uint64, capacity> * coll = new QPI::collection<QPI::uint64, capacity>();
    coll->reset();

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 333; ++i)
    {
        testCollectionPerformance(*coll,
            maxPovsCount, gen_buffers, genSize, i + 11, maxCleanupCounter);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    auto duration = t1 - t0;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

    delete coll;

    return ms.count();
}

TEST(TestCoreQPI, CollectionPerformance) {

    __scratchpadBuffer = new char[16 * 1024 * 1024];

    std::vector<QPI::uint64> durations;
    std::vector<std::string> descriptions;

    durations.push_back(testCollectionPerformance<1024>(128, 333));
    descriptions.push_back("[CollectionPerformance] Collection<1024>(128, 333)");

    durations.push_back(testCollectionPerformance<1024>(64, 333));
    descriptions.push_back("[CollectionPerformance] Collection<1024>(64, 333)");

    durations.push_back(testCollectionPerformance<1024>(32, 333));
    descriptions.push_back("[CollectionPerformance] Collection<1024>(32, 333)");

    durations.push_back(testCollectionPerformance<1024>(16, 333));
    descriptions.push_back("[CollectionPerformance] Collection<1024>(16, 333)");

    durations.push_back(testCollectionPerformance<512>(128, 333));
    descriptions.push_back("[CollectionPerformance] Collection<512>(128, 333)");

    durations.push_back(testCollectionPerformance<512>(64, 333));
    descriptions.push_back("[CollectionPerformance] Collection<512>(64, 333)");

    durations.push_back(testCollectionPerformance<512>(32, 333));
    descriptions.push_back("[CollectionPerformance] Collection<512>(32, 333)");

    durations.push_back(testCollectionPerformance<512>(16, 333));
    descriptions.push_back("[CollectionPerformance] Collection<512>(16, 333)");

    delete[] __scratchpadBuffer;
    __scratchpadBuffer = nullptr;

    bool verbose = true;
    if (verbose)
    {
        QPI::uint64 total = 0;
        for (size_t i = 0; i < durations.size(); i++)
        {
            total += durations[i];
            std::cout << "- " << descriptions[i] << ":\t" << durations[i] << " ms\n";
        }
        std::cout << "* [CollectionPerformance] Total:\t\t" << total << " ms\n";
    }
}

TEST(TestCoreQPI, Array)
{
    //QPI::array<int, 0> mustFail; // should raise compile error

    QPI::array<QPI::uint8, 4> uint8_4;
    EXPECT_EQ(uint8_4.capacity(), 4);
    //uint8_4.setMem(QPI::id(1, 2, 3, 4)); // should raise compile error
    uint8_4.setAll(2);
    EXPECT_EQ(uint8_4.get(0), 2);
    EXPECT_EQ(uint8_4.get(1), 2);
    EXPECT_EQ(uint8_4.get(2), 2);
    EXPECT_EQ(uint8_4.get(3), 2);
    for (int i = 0; i < uint8_4.capacity(); ++i)
        uint8_4.set(i, i+1);
    for (int i = 0; i < uint8_4.capacity(); ++i)
        EXPECT_EQ(uint8_4.get(i), i+1);

    QPI::array<QPI::uint64, 4> uint64_4;
    uint64_4.setMem(QPI::id(101, 102, 103, 104));
    for (int i = 0; i < uint64_4.capacity(); ++i)
        EXPECT_EQ(uint64_4.get(i), i + 101);
    //uint64_4.setMem(uint8_4); // should raise compile error

    QPI::array<QPI::uint16, 2> uint16_2;
    EXPECT_EQ(uint8_4.capacity(), 4);
    //uint16_2.setMem(QPI::id(1, 2, 3, 4)); // should raise compile error
    uint16_2.setAll(12345);
    EXPECT_EQ((int)uint16_2.get(0), 12345);
    EXPECT_EQ((int)uint16_2.get(1), 12345);
    for (int i = 0; i < uint16_2.capacity(); ++i)
        uint16_2.set(i, i + 987);
    for (int i = 0; i < uint16_2.capacity(); ++i)
        EXPECT_EQ((int)uint16_2.get(i), i + 987);
    uint16_2.setMem(uint8_4);
    for (int i = 0; i < uint16_2.capacity(); ++i)
        EXPECT_EQ((int)uint16_2.get(i), (int)(((2*i+2) << 8) | (2*i + 1)));
}

TEST(TestCoreQPI, BitArray)
{
    //QPI::bit_array<0> mustFail;

    QPI::bit_array<1> b1;
    EXPECT_EQ(b1.capacity(), 1);
    b1.setAll(0);
    EXPECT_EQ(b1.get(0), 0);
    b1.setAll(1);
    EXPECT_EQ(b1.get(0), 1);
    b1.setAll(true);
    EXPECT_EQ(b1.get(0), 1);
    b1.set(0, 1);
    EXPECT_EQ(b1.get(0), 1);
    b1.set(0, 0);
    EXPECT_EQ(b1.get(0), 0);
    b1.set(0, true);
    EXPECT_EQ(b1.get(0), 1);

    QPI::bit_array<64> b64;
    EXPECT_EQ(b64.capacity(), 64);
    b64.setMem(0x11llu);
    EXPECT_EQ(b64.get(0), 1);
    EXPECT_EQ(b64.get(1), 0);
    EXPECT_EQ(b64.get(2), 0);
    EXPECT_EQ(b64.get(3), 0);
    EXPECT_EQ(b64.get(4), 1);
    EXPECT_EQ(b64.get(5), 0);
    EXPECT_EQ(b64.get(6), 0);
    EXPECT_EQ(b64.get(7), 0);
    QPI::array<QPI::uint64, 1> llu1;
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0x11llu);
    b64.setAll(0);
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0x0);
    b64.setAll(1);
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0xffffffffffffffffllu);

    //QPI::bit_array<96> b96; // must trigger compile error

    QPI::bit_array<128> b128;
    EXPECT_EQ(b128.capacity(), 128);
    QPI::array<QPI::uint64, 2> llu2;
    llu2.setAll(0x4llu);
    EXPECT_EQ(llu2.get(0), 0x4llu);
    EXPECT_EQ(llu2.get(1), 0x4llu);
    b128.setMem(llu2);
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            EXPECT_EQ(b128.get(i * 64 + j), j == 2);
        }
    }
    b128.set(0, 1);
    b128.set(2, 0);
    llu2.setMem(b128);
    EXPECT_EQ(llu2.get(0), 0x1llu);
    EXPECT_EQ(llu2.get(1), 0x4llu);
    for (int i = 0; i < b128.capacity(); ++i)
    {
        b128.set(i, i % 2 == 0);
        EXPECT_EQ(b128.get(i), i % 2 == 0);
    }
    llu2.setMem(b128);
    EXPECT_EQ(llu2.get(0), 0x5555555555555555llu);
    EXPECT_EQ(llu2.get(1), 0x5555555555555555llu);
    for (int i = 0; i < b128.capacity(); ++i)
    {
        EXPECT_EQ(b128.get(i), i % 2 == 0);
    }
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

TEST(TestCoreQPI, ProposalAndVotingByComputors)
{
    QpiContextUserProcedureCall qpi(0, QPI::id(1, 2, 3, 4), 123);
    QPI::ProposalAndVotingByComputors pv;

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(pv, 0);

    // voter index is computor index
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        EXPECT_EQ(pv.getVoterIndex(qpi, qpi.computor(i)), i);
        EXPECT_EQ(pv.getVoterId(qpi, i), qpi.computor(i));
    }
    for (int i = NUMBER_OF_COMPUTORS; i < 800; ++i)
    {
        EXPECT_EQ(pv.getVoterIndex(qpi, qpi.computor(i)), QPI::INVALID_VOTER_INDEX);
        EXPECT_EQ(pv.getVoterId(qpi, i), QPI::NULL_ID);
    }
    EXPECT_EQ(pv.getVoterIndex(qpi, qpi.originator()), QPI::INVALID_VOTER_INDEX);

    // valid proposers are computors
    for (int i = 0; i < 2 * NUMBER_OF_COMPUTORS; ++i)
        EXPECT_EQ(pv.isValidProposer(qpi, qpi.computor(i)), (i < NUMBER_OF_COMPUTORS));
    EXPECT_FALSE(pv.isValidProposer(qpi, QPI::NULL_ID));
    EXPECT_FALSE(pv.isValidProposer(qpi, qpi.originator()));

    // no existing proposals
    for (int i = 0; i < 2*NUMBER_OF_COMPUTORS; ++i)
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(i)), (int)QPI::INVALID_PROPOSAL_INDEX);

    // fill all slots
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getNewProposalIndex(qpi, qpi.computor(j)), i);
    }

    // proposals now available
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(j)), i);
    }

    // using other ID fails if full (new computor ID after epoch change)
    QPI::id newId(9, 8, 7, 6);
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, newId), (int)QPI::INVALID_PROPOSAL_INDEX);

    // reuseing all slots should work
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getNewProposalIndex(qpi, qpi.computor(j)), i);
    }

    // free one slot
    pv.freeProposalByIndex(qpi, 0);

    // using other ID now works (new computor ID after epoch change)
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, newId), 0);

    // check content
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, newId), 0);
    for (int i = 1; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(j)), i);
    }
}

bool operator==(const QPI::ProposalData& p1, const QPI::ProposalData& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}

bool operator==(const QPI::SingleProposalVoteData& p1, const QPI::SingleProposalVoteData& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}

template <typename ProposalVotingType>
void setProposalWithSuccessCheck(const QpiContextUserProcedureCall& qpi, const ProposalVotingType& pv, const QPI::id& proposerId, const QPI::ProposalData& proposal)
{
    QPI::ProposalData proposalReturned;
    EXPECT_TRUE(qpi(*pv).setProposal(proposerId, proposal));
    QPI::uint16 proposalIdx = qpi(*pv).proposalIndex(proposerId);
    EXPECT_NE((int)proposalIdx, (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(proposalIdx), proposerId);
    EXPECT_TRUE(qpi(*pv).getProposal(proposalIdx, proposalReturned));
    EXPECT_TRUE(proposal == proposalReturned);
    expectNoVotes(qpi, pv, proposalIdx);
}

template <bool successExpected, typename ProposalVotingType>
void voteWithValidVoter(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 voteProposalIndex,
    QPI::uint16 voteProposalType,
    QPI::sint64 voteValue
)
{
    QPI::uint32 voterIdx = qpi(*pv).voterIndex(voterId);
    EXPECT_NE(voterIdx, QPI::INVALID_VOTER_INDEX);
    QPI::id voterIdReturned = qpi(*pv).voterId(voterIdx);
    EXPECT_EQ(voterIdReturned, voterId);

    QPI::SingleProposalVoteData voteReturnedBefore;
    bool oldVoteAvailable = qpi(*pv).getVote(voteProposalIndex, voterIdx, voteReturnedBefore);

    QPI::SingleProposalVoteData vote;
    vote.proposalIndex = voteProposalIndex;
    vote.proposalType = voteProposalType;
    vote.voteValue = voteValue;
    EXPECT_EQ(qpi(*pv).vote(voterId, vote), successExpected);

    QPI::SingleProposalVoteData voteReturned;
    if (successExpected)
    {
        if (voteProposalType == 0)
        {
            if (vote.voteValue != 0 && vote.voteValue != QPI::NO_VOTE_VALUE)
                vote.voteValue = 1;
        }

        EXPECT_TRUE(qpi(*pv).getVote(vote.proposalIndex, voterIdx, voteReturned));
        EXPECT_TRUE(vote == voteReturned);

        QPI::ProposalData proposalReturned;
        EXPECT_TRUE(qpi(*pv).getProposal(vote.proposalIndex, proposalReturned));
        EXPECT_TRUE(proposalReturned.type == voteReturned.proposalType);
    }
    else if (oldVoteAvailable)
    {
        EXPECT_TRUE(qpi(*pv).getVote(vote.proposalIndex, voterIdx, voteReturned));
        EXPECT_TRUE(voteReturnedBefore == voteReturned);
    }
}

template <typename ProposalVotingType>
void voteWithInvalidVoter(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 voteProposalIndex,
    QPI::uint16 voteProposalType,
    QPI::sint64 voteValue
)
{
    QPI::SingleProposalVoteData vote;
    vote.proposalIndex = voteProposalIndex;
    vote.proposalType = voteProposalType;
    vote.voteValue = voteValue;
    EXPECT_FALSE(qpi(*pv).vote(voterId, vote));
}

template <typename ProposalVotingType>
void expectNoVotes(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv,
    QPI::uint16 proposalIndex
)
{
    QPI::SingleProposalVoteData vote;
    for (QPI::uint32 i = 0; i < pv->maxVoters; ++i)
    {
        EXPECT_TRUE(qpi(*pv).getVote(proposalIndex, i, vote));
        EXPECT_EQ(vote.voteValue, QPI::NO_VOTE_VALUE);
    }
}

template <typename ProposalVotingType>
int countActiveProposals(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv
)
{
    int activeProposals = 0;
    QPI::sint32 idx = -1;
    while ((idx = qpi(*pv).nextActiveProposalIndex(idx)) >= 0)
        ++activeProposals;
    return activeProposals;
}

template <typename ProposalVotingType>
int countFinishedProposals(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv
)
{
    int finishedProposals = 0;
    QPI::sint32 idx = -1;
    while ((idx = qpi(*pv).nextFinishedProposalIndex(idx)) >= 0)
        ++finishedProposals;
    return finishedProposals;
}

TEST(TestCoreQPI, ProposalVoting)
{
    QpiContextUserProcedureCall qpi(0, QPI::id(1,2,3,4), 123);
    auto * pv = new QPI::ProposalVoting<
        QPI::ProposalAndVotingByComputors<200>,   // Allow less proposals than NUMBER_OF_COMPUTORS to check handling of full arrays
        QPI::ProposalData>;

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(*pv, 0);

    // fail: get before propsals have been set
    QPI::ProposalData proposalReturned;
    QPI::SingleProposalVoteData voteDataReturned;
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        EXPECT_FALSE(qpi(*pv).getProposal(i, proposalReturned));
        EXPECT_FALSE(qpi(*pv).getVote(i, 0, voteDataReturned));
    }
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(123456), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(123456), -1);

    // fail: get with invalid proposal index or vote index
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals + 1, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getVote(pv->maxProposals, 0, voteDataReturned));
    EXPECT_FALSE(qpi(*pv).getVote(pv->maxProposals + 1, 0, voteDataReturned));

    // fail: no proposals for given IDs and invalid input
    EXPECT_EQ((int)qpi(*pv).proposalIndex(QPI::NULL_ID), (int)QPI::INVALID_PROPOSAL_INDEX); // always equal
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.originator()), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(0)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(QPI::INVALID_PROPOSAL_INDEX), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(pv->maxProposals), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(0), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).proposerId(1), QPI::NULL_ID);

    // okay: voters are available independently of propsals (= computors)
    for (int i = 0; i < pv->maxVoters; ++i)
    {
        EXPECT_EQ(qpi(*pv).voterIndex(qpi.computor(i)), i);
        EXPECT_EQ(qpi(*pv).voterId(i), qpi.computor(i));
    }

    // fail: IDs / indcies of non-voters
    EXPECT_EQ(qpi(*pv).voterIndex(qpi.originator()), QPI::INVALID_VOTER_INDEX);
    EXPECT_EQ(qpi(*pv).voterIndex(QPI::NULL_ID), QPI::INVALID_VOTER_INDEX);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVoters), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVoters + 1), QPI::NULL_ID);

    // okay: set proposal for computor 0
    QPI::ProposalData proposal;
    proposal.url.set(0, 0);
    proposal.epoch = qpi.epoch();
    proposal.type = 0;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(0), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(0)), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote although no proposal is available
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 1, 0, 0);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 12345, 0, 0);

    // fail: vote with wrong type
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 0, 1, 0);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 0, 2, 0);

    // fail: vote with non-computor
    voteWithInvalidVoter(qpi, pv, qpi.originator(), 0, 0, 0);
    voteWithInvalidVoter(qpi, pv, QPI::NULL_ID, 0, 0, 0);

    // okay: correct votes in proposalIndex 0
    expectNoVotes(qpi, pv, 0);
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), 0, 0, (i - 4) % 5);
    voteWithValidVoter<true>(qpi, pv, qpi.computor(0), 0, 0, QPI::NO_VOTE_VALUE); // remove vote

    // fail: originator id(1,2,3,4) is no computor (see custom qpi.computor() above)
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.originator(), proposal));

    // fail: invalid type
    proposal.type = 123;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));

    // fail: wrong epoch (only current epoch possible)
    proposal.type = 0;
    proposal.epoch = 1;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));
    proposal.epoch = qpi.epoch() + 1;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));

    // okay: set proposal for computor 2 (proposal index 1, first use)
    proposal.epoch = qpi.epoch();
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(2), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(2)), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // okay: correct votes in proposalIndex 1 (first use)
    expectNoVotes(qpi, pv, 1);
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), 1, proposal.type, (i - 4) % 5);

    // fail: proposal of revenue distribution with wrong address
    proposal.type = 1;
    proposal.revenueDistribution.targetAddress = QPI::NULL_ID;
    proposal.revenueDistribution.proposedAmount = 100000;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));
    // check that overwrite did not work
    EXPECT_TRUE(qpi(*pv).getProposal(qpi(*pv).proposalIndex(qpi.computor(2)), proposalReturned));
    EXPECT_FALSE(proposalReturned == proposal);

    // fail: proposal of revenue distribution with invalid amount
    proposal.revenueDistribution.targetAddress = qpi.originator();
    proposal.revenueDistribution.proposedAmount = -123456;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));

    // okay: revenue distribution, overwrite existing proposal of comp 2 (proposal index 1, reused)
    proposal.revenueDistribution.targetAddress = qpi.originator();
    proposal.revenueDistribution.proposedAmount = 1005;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(2), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(2)), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, -1);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(1), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, -123456);

    // okay: correct votes in proposalIndex 1 (reused)
    expectNoVotes(qpi, pv, 1); // checks that setProposal clears previous votes
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, 1000 * i);
    voteWithValidVoter<true>(qpi, pv, qpi.computor(2), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, QPI::NO_VOTE_VALUE); // remove vote

    // fail: type 2 proposal with wrong min/max
    proposal.type = 2;
    proposal.variableValue.proposedValue = 10;
    proposal.variableValue.minValue = 11;
    proposal.variableValue.maxValue = 20;
    proposal.variableValue.variable = 123; // not checked, full range usable
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));
    proposal.variableValue.minValue = 0;
    proposal.variableValue.maxValue = 9;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));

    // fail: type 2 proposal with full range is invalid, because NO_VOTE_VALUE is reserved for no vote
    proposal.variableValue.minValue = 0x8000000000000000;
    proposal.variableValue.maxValue = 0x7fffffffffffffff;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));

    // okay: type 2 proposal with nearly full range
    proposal.variableValue.minValue = 0x8000000000000001;
    proposal.variableValue.maxValue = 0x7fffffffffffffff;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(1), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(1)), 2);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(2)), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), 2);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(2), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // okay: type 2 proposal with limited range
    proposal.variableValue.minValue = -100;
    proposal.variableValue.maxValue = 100;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(10), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(10)), 3);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), 2);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(2), 3);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(3), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, -101);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(1), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, 101);

    // okay: correct votes in proposalIndex of computor 10
    expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(qpi.computor(10)));
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, (i % 201) - 100);
    voteWithValidVoter<true>(qpi, pv, qpi.computor(2), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, QPI::NO_VOTE_VALUE); // remove vote

    // fill proposal storage
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    }
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: no space left
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(pv->maxProposals), proposal));

    // simulate epoch change
    ++currentEpoch;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals);

    // okay: same setProposal after epoch change, because the oldest proposal will be deleted
    proposal.epoch = qpi.epoch();
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(pv->maxProposals), proposal);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 1);

    // fail: vote in wrong epoch
    voteWithValidVoter<false>(qpi, pv, qpi.computor(123), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, 0);

    // manually clear some proposals
    EXPECT_FALSE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.originator())));
    EXPECT_TRUE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.computor(5))));
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(5)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);
    proposal.epoch = 0;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.originator(), proposal));
    EXPECT_TRUE(qpi(*pv).setProposal(qpi.computor(7), proposal));
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(7)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 3);

    // simulate epoch change with changes in computors
    ++currentEpoch;
    computorIdOffset += 100;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);

    // set new proposals, adding new computor IDs (simulated next epoch)
    proposal.epoch = qpi.epoch();
    proposal.type = 0;
    for (int i = 0; i < pv->maxProposals; ++i)
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    for (int i = 0; i < pv->maxProposals; ++i)
        expectNoVotes(qpi, pv, i);
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(countFinishedProposals(qpi, pv), 0);

    // get results
}
