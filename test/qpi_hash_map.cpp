#define NO_UEFI

#include "gtest/gtest.h"

static void* __scratchpadBuffer = nullptr;
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
#include "qpi_hash_map.h"
#include "qpi_hash_map_impl.h"
#include <unordered_set>


TEST(TestQPIHashMap, TestHashFunctionQPIID) 
{
	auto randomId = QPI::id::randomValue();

	// Check that the hash function returns the first 8 bytes as hash for QPI::id.
	QPI::uint64 hashRes = QPI::HashFunction<QPI::id>::hash(randomId);
	EXPECT_EQ(hashRes, randomId.u64._0);
}

TEST(TestQPIHashMap, TestHashFunction) 
{
	std::unordered_set<QPI::uint64> hashesSoFar;

	for (int i = 0; i < 1000; ++i)
	{
		// We expect the hash function to produce different hashes for 0...N.
		QPI::uint64 hashRes = QPI::HashFunction<int>::hash(i);
		EXPECT_FALSE(hashesSoFar.contains(hashRes));
		hashesSoFar.insert(hashRes);
	}
}

TEST(TestQPIHashMap, TestCreation) 
{
	constexpr QPI::uint64 capacity = 2;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	EXPECT_EQ(hashMap.capacity(), capacity);
	EXPECT_EQ(hashMap.population(), 0);
}

TEST(TestQPIHashMap, TestGetters) 
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[4] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45}, {143, 746, 87, 6} };
	int values[4] = { 17, 5467, 8752, 5348 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 4; ++i)
	{
		returnedIndex = hashMap.set(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.getElementIndex(ids[3]), returnedIndex);
	EXPECT_EQ(hashMap.key(returnedIndex), ids[3]);
	EXPECT_EQ(hashMap.value(returnedIndex), values[3]);

	int value = -1;

	EXPECT_FALSE(hashMap.get({ 1,2,3,4 }, value));
	EXPECT_EQ(value, -1);

	EXPECT_TRUE(hashMap.get(ids[0], value));
	EXPECT_EQ(value, values[0]);

	// Test getElementIndex when all slots are marked for removal so it has to iterate through the whole hash map.
	for (int i = 0; i < 3; ++i)
	{
		hashMap.removeByKey(ids[i]);
	}
	EXPECT_EQ(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);
}

TEST(TestQPIHashMap, TestSet) 
{
	constexpr QPI::uint64 capacity = 2;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { QPI::id::randomValue(), QPI::id::randomValue(), QPI::id::randomValue() };
	int values[3] = { 17, 5467, 8752 };

	QPI::sint64 returnedIndex = hashMap.set(ids[0], values[0]);
	EXPECT_EQ(hashMap.population(), 1);
	EXPECT_GE(returnedIndex, 0);
	EXPECT_LT(returnedIndex, capacity);

	// Set with existing key should overwrite the value.
	EXPECT_EQ(hashMap.set(ids[0], 42), returnedIndex);
	EXPECT_EQ(hashMap.value(returnedIndex), 42);

	returnedIndex = hashMap.set(ids[1], values[1]);
	EXPECT_EQ(hashMap.population(), hashMap.capacity());

	// Set when full should return NULL_INDEX for new key.
	EXPECT_EQ(hashMap.set(ids[2], values[2]), QPI::NULL_INDEX);

	// Set when full should still work for existing key and replace the value.
	EXPECT_EQ(hashMap.set(ids[1], values[2]), returnedIndex);
	EXPECT_EQ(hashMap.value(returnedIndex), values[2]);
}

TEST(TestQPIHashMap, TestRemove) 
{
	constexpr QPI::uint64 capacity = 8;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45} };
	int values[3] = { 17, 5467, 8752 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 3; ++i)
	{
		returnedIndex = hashMap.set(ids[i], values[i]);
	}
	EXPECT_EQ(hashMap.population(), 3);

	// Remove by element index.
	hashMap.removeByIndex(returnedIndex);

	EXPECT_EQ(hashMap.population(), 2);
	EXPECT_NE(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);
	EXPECT_NE(hashMap.getElementIndex(ids[1]), QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.getElementIndex(ids[2]), QPI::NULL_INDEX);

	// Try to remove key not contained in the hash map. 
	returnedIndex = hashMap.removeByKey({ 1,2,3,4 });
	EXPECT_EQ(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 2);

	// Remove by key that is contained in the hash map.
	returnedIndex = hashMap.removeByKey(ids[0]);
	EXPECT_NE(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 1);
	EXPECT_EQ(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);
	EXPECT_NE(hashMap.getElementIndex(ids[1]), QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.getElementIndex(ids[2]), QPI::NULL_INDEX);
}

TEST(TestQPIHashMap, TestCleanup) 
{
	__scratchpadBuffer = new char[256];

	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[4] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45}, {9754, 932, 237, 1} };
	int values[4] = { 17, 5467, 8752, 342 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 4; ++i)
	{
		hashMap.set(ids[i], values[i]);
	}

	// This will mark for removal but slot will not become available.
	hashMap.removeByKey(ids[3]);
	EXPECT_EQ(hashMap.population(), 3);
	// So the next set will fail because no slot is available.
	returnedIndex = hashMap.set(ids[3], values[3]);
	EXPECT_EQ(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 3);

	// Cleanup will properly remove the element marked for removal.
	hashMap.cleanup();
	EXPECT_EQ(hashMap.population(), 3);

	EXPECT_NE(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);
	EXPECT_NE(hashMap.getElementIndex(ids[1]), QPI::NULL_INDEX);
	EXPECT_NE(hashMap.getElementIndex(ids[2]), QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.getElementIndex(ids[3]), QPI::NULL_INDEX);

	// So the next set should work again.
	returnedIndex = hashMap.set(ids[3], values[3]);
	EXPECT_NE(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 4);

	delete[] __scratchpadBuffer;
	__scratchpadBuffer = nullptr;
}

TEST(TestQPIHashMap, TestCleanupPerformanceShortcuts) 
{

	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[4] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45}, {9754, 932, 237, 1} };
	int values[4] = { 17, 5467, 8752, 342 };

	for (int i = 0; i < 4; ++i)
	{
		hashMap.set(ids[i], values[i]);
	}

	// Test cleanup when no elements have been marked for removal.
	hashMap.cleanup();

	for (int i = 0; i < 4; ++i)
	{
		hashMap.removeByKey(ids[i]);
	}
	EXPECT_EQ(hashMap.population(), 0);

	// Test cleanup when all elements have been marked for removal.
	hashMap.cleanup();
}

TEST(TestQPIHashMap, TestCleanupLargeMapSameHashes) 
{
	constexpr QPI::uint64 capacity = 64;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	__scratchpadBuffer = new char[2 * sizeof(hashMap)];

	for (QPI::uint64 i = 0; i < 64; ++i)
	{
		// Add 64 elements with different keys but same hash.
		// The hash for QPI::id is the first 8 bytes.
		hashMap.set({ 3478, i, i + 1, i + 2 }, i * 2 + 7);
	}
	hashMap.removeByKey({ 3478, 63, 64, 65 });

	// Cleanup will have to iterate through the whole map to find an empty slot for the last element.
	hashMap.cleanup();

	delete[] __scratchpadBuffer;
	__scratchpadBuffer = nullptr;
}

TEST(TestQPIHashMap, TestReplace) 
{
	constexpr QPI::uint64 capacity = 8;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45} };
	int values[3] = { 17, 5467, 8752 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 3; ++i)
	{
		returnedIndex = hashMap.set(ids[i], values[i]);
	}
	EXPECT_EQ(hashMap.population(), 3);

	EXPECT_FALSE(hashMap.replace({ 1,2,3,4 }, 42));
	EXPECT_EQ(hashMap.population(), 3);

	EXPECT_TRUE(hashMap.replace(ids[1], 42));

	EXPECT_EQ(hashMap.population(), 3);
	int value = -1;
	EXPECT_TRUE(hashMap.get(ids[1], value));
	EXPECT_EQ(value, 42);
	EXPECT_TRUE(hashMap.get(ids[0], value));
	EXPECT_EQ(value, values[0]);
	EXPECT_TRUE(hashMap.get(ids[2], value));
	EXPECT_EQ(value, values[2]);
}

TEST(TestQPIHashMap, TestReset) 
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[4] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45}, {9754, 932, 237, 1} };
	int values[4] = { 17, 5467, 8752, 342 };

	for (int i = 0; i < 4; ++i)
	{
		hashMap.set(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.population(), 4);
	hashMap.reset();
	EXPECT_EQ(hashMap.population(), 0);
}
