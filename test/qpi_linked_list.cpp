#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/contract_core/pre_qpi_def.h"
#include "../src/contracts/qpi.h"
#include "../src/common_buffers.h"
#include "../src/contract_core/qpi_linked_list_impl.h"
#include <vector>
#include <random>


// Helper: validate structural integrity of a LinkedList.
// Checks that forward traversal count == backward traversal count == population,
// head has no prev, tail has no next, and all traversed nodes are occupied.
template <typename T, QPI::uint64 L>
void validateLinkedList(const QPI::LinkedList<T, L>& list)
{
	QPI::uint64 pop = list.population();

	if (pop == 0)
	{
		EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
		EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);
		return;
	}

	// Head has no prev
	EXPECT_NE(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.prevElementIndex(list.headIndex()), QPI::NULL_INDEX);

	// Tail has no next
	EXPECT_NE(list.tailIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.nextElementIndex(list.tailIndex()), QPI::NULL_INDEX);

	// Forward traversal count
	QPI::uint64 forwardCount = 0;
	QPI::sint64 idx = list.headIndex();
	while (idx != QPI::NULL_INDEX)
	{
		EXPECT_FALSE(list.isEmptySlot(idx));
		forwardCount++;
		ASSERT_LE(forwardCount, pop) << "Forward traversal exceeded population (possible cycle)";
		idx = list.nextElementIndex(idx);
	}
	EXPECT_EQ(forwardCount, pop);

	// Backward traversal count
	QPI::uint64 backwardCount = 0;
	idx = list.tailIndex();
	while (idx != QPI::NULL_INDEX)
	{
		EXPECT_FALSE(list.isEmptySlot(idx));
		backwardCount++;
		ASSERT_LE(backwardCount, pop) << "Backward traversal exceeded population (possible cycle)";
		idx = list.prevElementIndex(idx);
	}
	EXPECT_EQ(backwardCount, pop);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Empty list

TEST(QPILinkedListTest, EmptyList)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	EXPECT_EQ(list.population(), 0u);
	EXPECT_EQ(list.capacity(), 8u);
	EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: addHead

TEST(QPILinkedListTest, AddHeadSingle)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	QPI::sint64 idx = list.addHead(42);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.headIndex(), idx);
	EXPECT_EQ(list.tailIndex(), idx);
	EXPECT_EQ(list.element(idx), 42);
	EXPECT_FALSE(list.isEmptySlot(idx));
	validateLinkedList(list);
}

TEST(QPILinkedListTest, AddHeadMultiple)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx3 = list.addHead(30);
	QPI::sint64 idx2 = list.addHead(20);
	QPI::sint64 idx1 = list.addHead(10);

	EXPECT_EQ(list.population(), 3u);
	EXPECT_EQ(list.headIndex(), idx1);
	EXPECT_EQ(list.tailIndex(), idx3);

	// Forward: 10 -> 20 -> 30
	EXPECT_EQ(list.element(list.headIndex()), 10);
	QPI::sint64 cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 10);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 20);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 30);
	EXPECT_EQ(list.nextElementIndex(cur), QPI::NULL_INDEX);

	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: addTail

TEST(QPILinkedListTest, AddTailSingle)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	QPI::sint64 idx = list.addTail(99);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.headIndex(), idx);
	EXPECT_EQ(list.tailIndex(), idx);
	EXPECT_EQ(list.element(idx), 99);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, AddTailMultiple)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx2 = list.addTail(20);
	QPI::sint64 idx3 = list.addTail(30);

	EXPECT_EQ(list.population(), 3u);
	EXPECT_EQ(list.headIndex(), idx1);
	EXPECT_EQ(list.tailIndex(), idx3);

	// Forward: 10 -> 20 -> 30
	QPI::sint64 cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 10);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 20);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 30);
	EXPECT_EQ(list.nextElementIndex(cur), QPI::NULL_INDEX);

	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: insertAfter / insertBefore

TEST(QPILinkedListTest, InsertAfterMiddle)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx3 = list.addTail(30);

	// Insert 20 after 10
	QPI::sint64 idx2 = list.insertAfter(idx1, 20);
	EXPECT_NE(idx2, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 3u);

	// Forward: 10 -> 20 -> 30
	QPI::sint64 cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 10);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 20);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 30);

	validateLinkedList(list);
}

TEST(QPILinkedListTest, InsertAfterTail)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx2 = list.insertAfter(idx1, 20);

	EXPECT_EQ(list.population(), 2u);
	EXPECT_EQ(list.tailIndex(), idx2);
	EXPECT_EQ(list.element(idx2), 20);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, InsertBeforeMiddle)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx3 = list.addTail(30);

	// Insert 20 before 30
	QPI::sint64 idx2 = list.insertBefore(idx3, 20);
	EXPECT_NE(idx2, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 3u);

	// Forward: 10 -> 20 -> 30
	QPI::sint64 cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 10);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 20);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 30);

	validateLinkedList(list);
}

TEST(QPILinkedListTest, InsertBeforeHead)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx2 = list.addTail(20);
	QPI::sint64 idx1 = list.insertBefore(idx2, 10);

	EXPECT_EQ(list.population(), 2u);
	EXPECT_EQ(list.headIndex(), idx1);
	EXPECT_EQ(list.element(idx1), 10);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, InsertInvalidIndex)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	// Insert with invalid indices
	EXPECT_EQ(list.insertAfter(QPI::NULL_INDEX, 10), QPI::NULL_INDEX);
	EXPECT_EQ(list.insertBefore(QPI::NULL_INDEX, 10), QPI::NULL_INDEX);
	EXPECT_EQ(list.insertAfter(100, 10), QPI::NULL_INDEX);
	EXPECT_EQ(list.insertBefore(100, 10), QPI::NULL_INDEX);

	// Insert at an empty slot
	list.addTail(10);
	EXPECT_EQ(list.insertAfter(3, 20), QPI::NULL_INDEX);  // slot 3 is empty
	EXPECT_EQ(list.insertBefore(3, 20), QPI::NULL_INDEX);

	EXPECT_EQ(list.population(), 1u);
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: remove

TEST(QPILinkedListTest, RemoveHead)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx2 = list.addTail(20);
	QPI::sint64 idx3 = list.addTail(30);

	list.remove(idx1);
	EXPECT_EQ(list.population(), 2u);
	EXPECT_EQ(list.headIndex(), idx2);
	EXPECT_EQ(list.element(list.headIndex()), 20);
	EXPECT_TRUE(list.isEmptySlot(idx1));
	validateLinkedList(list);
}

TEST(QPILinkedListTest, RemoveTail)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx2 = list.addTail(20);
	QPI::sint64 idx3 = list.addTail(30);

	list.remove(idx3);
	EXPECT_EQ(list.population(), 2u);
	EXPECT_EQ(list.tailIndex(), idx2);
	EXPECT_EQ(list.element(list.tailIndex()), 20);
	EXPECT_TRUE(list.isEmptySlot(idx3));
	validateLinkedList(list);
}

TEST(QPILinkedListTest, RemoveMiddle)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 idx1 = list.addTail(10);
	QPI::sint64 idx2 = list.addTail(20);
	QPI::sint64 idx3 = list.addTail(30);

	list.remove(idx2);
	EXPECT_EQ(list.population(), 2u);
	EXPECT_EQ(list.headIndex(), idx1);
	EXPECT_EQ(list.tailIndex(), idx3);

	// 10 -> 30
	EXPECT_EQ(list.nextElementIndex(idx1), idx3);
	EXPECT_EQ(list.prevElementIndex(idx3), idx1);
	EXPECT_TRUE(list.isEmptySlot(idx2));
	validateLinkedList(list);
}

TEST(QPILinkedListTest, RemoveSingleElement)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	QPI::sint64 idx = list.addTail(42);
	list.remove(idx);

	EXPECT_EQ(list.population(), 0u);
	EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);
	EXPECT_TRUE(list.isEmptySlot(idx));
	validateLinkedList(list);
}

TEST(QPILinkedListTest, RemoveAllElements)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	QPI::sint64 indices[4];
	for (int i = 0; i < 4; i++)
		indices[i] = list.addTail(i * 10);

	for (int i = 0; i < 4; i++)
		list.remove(indices[i]);

	EXPECT_EQ(list.population(), 0u);
	EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, RemoveInvalidAndDouble)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	// Remove from empty list
	list.remove(0);
	EXPECT_EQ(list.population(), 0u);

	// Remove with invalid indices
	list.remove(QPI::NULL_INDEX);
	list.remove(100);

	QPI::sint64 idx = list.addTail(42);
	list.remove(idx);
	// Double remove should be a no-op (slot already empty)
	list.remove(idx);
	EXPECT_EQ(list.population(), 0u);
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Free list reuse

TEST(QPILinkedListTest, FreeListReuse)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	// Fill completely
	QPI::sint64 idx0 = list.addTail(10);
	QPI::sint64 idx1 = list.addTail(20);
	QPI::sint64 idx2 = list.addTail(30);
	QPI::sint64 idx3 = list.addTail(40);
	EXPECT_EQ(list.population(), 4u);

	// Full, should return NULL_INDEX
	EXPECT_EQ(list.addTail(50), QPI::NULL_INDEX);

	// Remove two elements
	list.remove(idx1);
	list.remove(idx3);
	EXPECT_EQ(list.population(), 2u);

	// Add two more - should reuse freed slots
	QPI::sint64 newIdx1 = list.addTail(50);
	QPI::sint64 newIdx2 = list.addTail(60);
	EXPECT_NE(newIdx1, QPI::NULL_INDEX);
	EXPECT_NE(newIdx2, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 4u);

	// The new indices should be from the freed slots (idx3 and idx1, LIFO order)
	EXPECT_TRUE(newIdx1 == idx1 || newIdx1 == idx3);
	EXPECT_TRUE(newIdx2 == idx1 || newIdx2 == idx3);
	EXPECT_NE(newIdx1, newIdx2);

	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Full list

TEST(QPILinkedListTest, FullList)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	for (int i = 0; i < 4; i++)
	{
		QPI::sint64 idx = list.addTail(i);
		EXPECT_NE(idx, QPI::NULL_INDEX);
	}

	EXPECT_EQ(list.population(), 4u);

	// All insertion methods should return NULL_INDEX
	EXPECT_EQ(list.addHead(100), QPI::NULL_INDEX);
	EXPECT_EQ(list.addTail(100), QPI::NULL_INDEX);
	EXPECT_EQ(list.insertAfter(list.headIndex(), 100), QPI::NULL_INDEX);
	EXPECT_EQ(list.insertBefore(list.tailIndex(), 100), QPI::NULL_INDEX);

	EXPECT_EQ(list.population(), 4u);
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: replace

TEST(QPILinkedListTest, ReplaceValid)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	QPI::sint64 idx = list.addTail(10);
	EXPECT_TRUE(list.replace(idx, 20));
	EXPECT_EQ(list.element(idx), 20);
	EXPECT_EQ(list.population(), 1u);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, ReplaceInvalid)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	// Replace on empty list
	EXPECT_FALSE(list.replace(0, 42));

	// Replace on invalid indices
	EXPECT_FALSE(list.replace(QPI::NULL_INDEX, 42));
	EXPECT_FALSE(list.replace(100, 42));

	// Replace on freed slot
	QPI::sint64 idx = list.addTail(10);
	list.remove(idx);
	EXPECT_FALSE(list.replace(idx, 42));
}


//////////////////////////////////////////////////////////////////////////////
// Test: isEmptySlot

TEST(QPILinkedListTest, IsEmptySlot)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	// All slots empty initially
	for (int i = 0; i < 4; i++)
		EXPECT_TRUE(list.isEmptySlot(i));

	// Out-of-range indices are "empty"
	EXPECT_TRUE(list.isEmptySlot(-1));
	EXPECT_TRUE(list.isEmptySlot(4));
	EXPECT_TRUE(list.isEmptySlot(100));

	// After insert
	QPI::sint64 idx = list.addTail(42);
	EXPECT_FALSE(list.isEmptySlot(idx));

	// After remove
	list.remove(idx);
	EXPECT_TRUE(list.isEmptySlot(idx));
}


//////////////////////////////////////////////////////////////////////////////
// Test: reset

TEST(QPILinkedListTest, ResetCleansState)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	// Add some elements
	list.addTail(10);
	list.addTail(20);
	list.addTail(30);
	EXPECT_EQ(list.population(), 3u);

	// Reset
	list.reset();
	EXPECT_EQ(list.population(), 0u);
	EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);

	// All slots should be empty
	for (int i = 0; i < 8; i++)
		EXPECT_TRUE(list.isEmptySlot(i));

	// Should be able to add elements again
	QPI::sint64 idx = list.addTail(100);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.element(idx), 100);
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Bidirectional iteration

TEST(QPILinkedListTest, BidirectionalIteration)
{
	QPI::LinkedList<int, 16> list;
	list.reset();

	// Build list: 0, 10, 20, 30, 40, 50, 60, 70
	for (int i = 0; i < 8; i++)
		list.addTail(i * 10);

	// Forward iteration
	std::vector<int> forward;
	QPI::sint64 cur = list.headIndex();
	while (cur != QPI::NULL_INDEX)
	{
		forward.push_back(list.element(cur));
		cur = list.nextElementIndex(cur);
	}
	ASSERT_EQ(forward.size(), 8u);
	for (int i = 0; i < 8; i++)
		EXPECT_EQ(forward[i], i * 10);

	// Backward iteration
	std::vector<int> backward;
	cur = list.tailIndex();
	while (cur != QPI::NULL_INDEX)
	{
		backward.push_back(list.element(cur));
		cur = list.prevElementIndex(cur);
	}
	ASSERT_EQ(backward.size(), 8u);
	for (int i = 0; i < 8; i++)
		EXPECT_EQ(backward[i], (7 - i) * 10);

	validateLinkedList(list);
}

TEST(QPILinkedListTest, TraversalWithInvalidIndex)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	list.addTail(10);

	// nextElementIndex / prevElementIndex with invalid indices
	EXPECT_EQ(list.nextElementIndex(QPI::NULL_INDEX), QPI::NULL_INDEX);
	EXPECT_EQ(list.prevElementIndex(QPI::NULL_INDEX), QPI::NULL_INDEX);
	EXPECT_EQ(list.nextElementIndex(100), QPI::NULL_INDEX);
	EXPECT_EQ(list.prevElementIndex(100), QPI::NULL_INDEX);

	// Unoccupied slot
	EXPECT_EQ(list.nextElementIndex(3), QPI::NULL_INDEX);
	EXPECT_EQ(list.prevElementIndex(3), QPI::NULL_INDEX);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Different types

TEST(QPILinkedListTest, TypeId)
{
	QPI::LinkedList<QPI::id, 4> list;
	list.reset();

	QPI::id id1 = { 1, 2, 3, 4 };
	QPI::id id2 = { 5, 6, 7, 8 };

	QPI::sint64 idx1 = list.addTail(id1);
	QPI::sint64 idx2 = list.addTail(id2);

	EXPECT_EQ(list.element(idx1), id1);
	EXPECT_EQ(list.element(idx2), id2);
	EXPECT_EQ(list.population(), 2u);
	validateLinkedList(list);
}

struct TestStruct
{
	QPI::sint64 a;
	QPI::uint32 b;
	char c;

	bool operator==(const TestStruct& other) const
	{
		return a == other.a && b == other.b && c == other.c;
	}
};

TEST(QPILinkedListTest, TypeCustomStruct)
{
	QPI::LinkedList<TestStruct, 4> list;
	list.reset();

	TestStruct s1 = { 100, 200, 'x' };
	TestStruct s2 = { -50, 0, 'y' };

	QPI::sint64 idx1 = list.addHead(s1);
	QPI::sint64 idx2 = list.addTail(s2);

	EXPECT_EQ(list.element(idx1).a, 100);
	EXPECT_EQ(list.element(idx1).b, 200u);
	EXPECT_EQ(list.element(idx1).c, 'x');
	EXPECT_EQ(list.element(idx2).a, -50);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, SmallCapacityOne)
{
	QPI::LinkedList<int, 1> list;
	list.reset();

	EXPECT_EQ(list.capacity(), 1u);
	EXPECT_EQ(list.population(), 0u);

	QPI::sint64 idx = list.addTail(42);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.element(idx), 42);
	EXPECT_EQ(list.headIndex(), idx);
	EXPECT_EQ(list.tailIndex(), idx);

	// Full
	EXPECT_EQ(list.addTail(99), QPI::NULL_INDEX);
	EXPECT_EQ(list.addHead(99), QPI::NULL_INDEX);

	list.remove(idx);
	EXPECT_EQ(list.population(), 0u);

	// Reuse
	QPI::sint64 idx2 = list.addHead(77);
	EXPECT_NE(idx2, QPI::NULL_INDEX);
	EXPECT_EQ(list.element(idx2), 77);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, SmallCapacityTwo)
{
	QPI::LinkedList<int, 2> list;
	list.reset();

	EXPECT_EQ(list.capacity(), 2u);

	QPI::sint64 idx1 = list.addHead(10);
	QPI::sint64 idx2 = list.addTail(20);
	EXPECT_EQ(list.population(), 2u);
	EXPECT_EQ(list.addTail(30), QPI::NULL_INDEX);

	EXPECT_EQ(list.element(list.headIndex()), 10);
	EXPECT_EQ(list.element(list.tailIndex()), 20);

	list.remove(idx1);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.headIndex(), idx2);
	EXPECT_EQ(list.tailIndex(), idx2);

	QPI::sint64 idx3 = list.addHead(5);
	EXPECT_NE(idx3, QPI::NULL_INDEX);
	EXPECT_EQ(list.element(list.headIndex()), 5);
	EXPECT_EQ(list.element(list.tailIndex()), 20);
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Stress test with random operations

TEST(QPILinkedListTest, StressTest)
{
	constexpr QPI::uint64 CAP = 256;
	QPI::LinkedList<int, CAP> list;
	list.reset();

	std::mt19937 rng(42);  // Fixed seed for reproducibility
	std::uniform_int_distribution<int> opDist(0, 5);
	std::uniform_int_distribution<int> valDist(0, 10000);

	// Track valid indices for random removal
	std::vector<QPI::sint64> validIndices;

	for (int iter = 0; iter < 2000; iter++)
	{
		int op = opDist(rng);
		int val = valDist(rng);

		switch (op)
		{
		case 0: // addHead
		{
			QPI::sint64 idx = list.addHead(val);
			if (idx != QPI::NULL_INDEX)
				validIndices.push_back(idx);
			break;
		}
		case 1: // addTail
		{
			QPI::sint64 idx = list.addTail(val);
			if (idx != QPI::NULL_INDEX)
				validIndices.push_back(idx);
			break;
		}
		case 2: // insertAfter random existing
		{
			if (!validIndices.empty())
			{
				std::uniform_int_distribution<size_t> idxDist(0, validIndices.size() - 1);
				QPI::sint64 target = validIndices[idxDist(rng)];
				QPI::sint64 idx = list.insertAfter(target, val);
				if (idx != QPI::NULL_INDEX)
					validIndices.push_back(idx);
			}
			break;
		}
		case 3: // insertBefore random existing
		{
			if (!validIndices.empty())
			{
				std::uniform_int_distribution<size_t> idxDist(0, validIndices.size() - 1);
				QPI::sint64 target = validIndices[idxDist(rng)];
				QPI::sint64 idx = list.insertBefore(target, val);
				if (idx != QPI::NULL_INDEX)
					validIndices.push_back(idx);
			}
			break;
		}
		case 4: // remove random existing
		case 5:
		{
			if (!validIndices.empty())
			{
				std::uniform_int_distribution<size_t> idxDist(0, validIndices.size() - 1);
				size_t pos = idxDist(rng);
				list.remove(validIndices[pos]);
				validIndices.erase(validIndices.begin() + pos);
			}
			break;
		}
		}

		EXPECT_EQ(list.population(), validIndices.size());
	}

	// Final structural validation
	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Mixed operations sequence

TEST(QPILinkedListTest, MixedOperations)
{
	QPI::LinkedList<int, 8> list;
	list.reset();

	// Build: addTail(1), addTail(2), addHead(0) => 0,1,2
	QPI::sint64 i1 = list.addTail(1);
	QPI::sint64 i2 = list.addTail(2);
	QPI::sint64 i0 = list.addHead(0);

	EXPECT_EQ(list.population(), 3u);

	// insertAfter(i1, 15) => 0,1,15,2
	QPI::sint64 i15 = list.insertAfter(i1, 15);

	// insertBefore(i2, 17) => 0,1,15,17,2
	QPI::sint64 i17 = list.insertBefore(i2, 17);

	EXPECT_EQ(list.population(), 5u);

	// Verify: 0 -> 1 -> 15 -> 17 -> 2
	QPI::sint64 cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 0);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 1);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 15);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 17);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 2);
	EXPECT_EQ(list.nextElementIndex(cur), QPI::NULL_INDEX);

	// Remove middle (15)
	list.remove(i15);
	EXPECT_EQ(list.population(), 4u);

	// Verify: 0 -> 1 -> 17 -> 2
	cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 0);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 1);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 17);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 2);
	EXPECT_EQ(list.nextElementIndex(cur), QPI::NULL_INDEX);

	// Replace i17 value: 17 -> 99
	EXPECT_TRUE(list.replace(i17, 99));
	EXPECT_EQ(list.element(i17), 99);

	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Fill, empty, refill cycle

TEST(QPILinkedListTest, FillEmptyRefill)
{
	QPI::LinkedList<int, 4> list;
	list.reset();

	// First fill
	for (int i = 0; i < 4; i++)
		list.addTail(i);
	EXPECT_EQ(list.population(), 4u);

	// Empty via remove from head
	while (list.headIndex() != QPI::NULL_INDEX)
		list.remove(list.headIndex());
	EXPECT_EQ(list.population(), 0u);

	// Refill (should reuse all freed slots)
	for (int i = 10; i < 14; i++)
	{
		QPI::sint64 idx = list.addTail(i);
		EXPECT_NE(idx, QPI::NULL_INDEX);
	}
	EXPECT_EQ(list.population(), 4u);
	EXPECT_EQ(list.addTail(99), QPI::NULL_INDEX);  // Still full

	// Verify values: 10, 11, 12, 13
	QPI::sint64 cur = list.headIndex();
	for (int i = 10; i < 14; i++)
	{
		ASSERT_NE(cur, QPI::NULL_INDEX);
		EXPECT_EQ(list.element(cur), i);
		cur = list.nextElementIndex(cur);
	}

	validateLinkedList(list);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Data is zeroed after removal (security)

TEST(QPILinkedListTest, DataZeroedAfterRemoval)
{
	QPI::LinkedList<QPI::uint64, 4> list;
	list.reset();

	QPI::sint64 idx = list.addTail(0xDEADBEEFCAFEBABEULL);
	EXPECT_EQ(list.element(idx), 0xDEADBEEFCAFEBABEULL);

	list.remove(idx);

	// After removal, the value field should be zeroed (security measure)
	// We read via element() which masks with (L-1), so the value at that slot is accessible
	EXPECT_EQ(list.element(idx), 0ULL);
}


//////////////////////////////////////////////////////////////////////////////
// Test: Zero-initialized state (simulating contract state without reset())

TEST(QPILinkedListTest, ZeroInitReadOnly)
{
	// Simulate contract state: zero-filled memory, no constructor, no reset()
	QPI::LinkedList<int, 8> list;
	memset(&list, 0, sizeof(list));

	// Must behave as an empty list
	EXPECT_EQ(list.population(), 0u);
	EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);
	EXPECT_TRUE(list.isEmptySlot(0));
	EXPECT_TRUE(list.isEmptySlot(1));
	EXPECT_EQ(list.nextElementIndex(0), QPI::NULL_INDEX);
	EXPECT_EQ(list.prevElementIndex(0), QPI::NULL_INDEX);
	EXPECT_FALSE(list.replace(0, 42));
	validateLinkedList(list);
}

TEST(QPILinkedListTest, ZeroInitAddHead)
{
	QPI::LinkedList<int, 4> list;
	memset(&list, 0, sizeof(list));

	QPI::sint64 idx = list.addHead(42);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.headIndex(), idx);
	EXPECT_EQ(list.tailIndex(), idx);
	EXPECT_EQ(list.element(idx), 42);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, ZeroInitAddTail)
{
	QPI::LinkedList<int, 4> list;
	memset(&list, 0, sizeof(list));

	QPI::sint64 idx = list.addTail(99);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.headIndex(), idx);
	EXPECT_EQ(list.tailIndex(), idx);
	EXPECT_EQ(list.element(idx), 99);
	validateLinkedList(list);
}

TEST(QPILinkedListTest, ZeroInitFullCycle)
{
	// Simulate contract state: zero-filled, then use the list fully
	QPI::LinkedList<int, 4> list;
	memset(&list, 0, sizeof(list));

	// Fill
	QPI::sint64 idx0 = list.addTail(10);
	QPI::sint64 idx1 = list.addTail(20);
	QPI::sint64 idx2 = list.addTail(30);
	QPI::sint64 idx3 = list.addTail(40);
	EXPECT_EQ(list.population(), 4u);
	EXPECT_EQ(list.addTail(50), QPI::NULL_INDEX);  // Full

	// Verify order: 10 -> 20 -> 30 -> 40
	QPI::sint64 cur = list.headIndex();
	EXPECT_EQ(list.element(cur), 10);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 20);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 30);
	cur = list.nextElementIndex(cur);
	EXPECT_EQ(list.element(cur), 40);
	EXPECT_EQ(list.nextElementIndex(cur), QPI::NULL_INDEX);

	// Remove and re-add
	list.remove(idx1);
	QPI::sint64 newIdx = list.addTail(50);
	EXPECT_NE(newIdx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 4u);

	validateLinkedList(list);
}

TEST(QPILinkedListTest, ZeroInitRemoveAll)
{
	QPI::LinkedList<int, 8> list;
	memset(&list, 0, sizeof(list));

	// Add and remove all elements
	QPI::sint64 indices[4];
	for (int i = 0; i < 4; i++)
		indices[i] = list.addTail(i * 10);

	for (int i = 0; i < 4; i++)
		list.remove(indices[i]);

	EXPECT_EQ(list.population(), 0u);
	EXPECT_EQ(list.headIndex(), QPI::NULL_INDEX);
	EXPECT_EQ(list.tailIndex(), QPI::NULL_INDEX);
	validateLinkedList(list);

	// Re-add after emptying
	QPI::sint64 idx = list.addHead(100);
	EXPECT_NE(idx, QPI::NULL_INDEX);
	EXPECT_EQ(list.population(), 1u);
	EXPECT_EQ(list.element(idx), 100);
	validateLinkedList(list);
}
