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


TEST(TestQPIHashMap, TestHashFunction) {
	auto randomId = QPI::id::randomValue();

	// Check that the hash function returns the first 8 bytes as hash for QPI::id.
	QPI::uint64 hashRes = QPI::HashFunction<QPI::id>::hash(randomId);
	EXPECT_EQ(hashRes, randomId.u64._0);
}

TEST(TestQPIHashMap, TestCreation) {
	constexpr QPI::uint64 capacity = 2;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	EXPECT_EQ(hashMap.capacity(), capacity);
	EXPECT_EQ(hashMap.population(), 0);
}

TEST(TestQPIHashMap, TestGetters) {
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45} };
	int values[3] = { 17, 5467, 8752 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 3; ++i) {
		returnedIndex = hashMap.add(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.key(returnedIndex), ids[2]);
	EXPECT_EQ(hashMap.value(returnedIndex), values[2]);

	int value = -1;

	EXPECT_FALSE(hashMap.get({ 1,2,3,4 }, value));
	EXPECT_EQ(value, -1);

	EXPECT_TRUE(hashMap.get(ids[0], value));
	EXPECT_EQ(value, values[0]);
}

TEST(TestQPIHashMap, TestAdd) {
	constexpr QPI::uint64 capacity = 2;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { QPI::id::randomValue(), QPI::id::randomValue(), QPI::id::randomValue() };
	int values[3] = { 17, 5467, 8752 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 2; ++i) {
		returnedIndex = hashMap.add(ids[i], values[i]);
		EXPECT_EQ(hashMap.population(), i + 1);
		EXPECT_GE(returnedIndex, 0);
		EXPECT_LT(returnedIndex, capacity);
	}

	returnedIndex = hashMap.add(ids[2], values[2]);
	EXPECT_EQ(returnedIndex, QPI::NULL_INDEX);
}

TEST(TestQPIHashMap, TestRemove) {
	constexpr QPI::uint64 capacity = 8;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45} };
	int values[3] = { 17, 5467, 8752 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 3; ++i) {
		returnedIndex = hashMap.add(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.population(), 3);

	// Remove by element index.
	hashMap.remove(returnedIndex);

	EXPECT_EQ(hashMap.population(), 2);
	
	// Try to remove key not contained in the hash map. 
	returnedIndex = hashMap.remove({ 1,2,3,4 });
	EXPECT_EQ(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 2);

	// Remove by key that is contained in the hash map.
	returnedIndex = hashMap.remove(ids[0]);
	EXPECT_NE(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 1);
}

TEST(TestQPIHashMap, TestCleanup) {
	__scratchpadBuffer = new char[256];

	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[4] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45}, {9754, 932, 237, 1} };
	int values[4] = { 17, 5467, 8752, 342 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 4; ++i) {
		hashMap.add(ids[i], values[i]);
	}

	// This will mark for removal but slot will not become available.
	hashMap.remove(ids[3]);
	EXPECT_EQ(hashMap.population(), 3);
	// So the next add will fail because no slot is available.
	returnedIndex = hashMap.add(ids[3], values[3]);
	EXPECT_EQ(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 3);

	// Cleanup will properly remove the element marked for removal.
	hashMap.cleanup();
	EXPECT_EQ(hashMap.population(), 3);

	// So the next add should work again.
	returnedIndex = hashMap.add(ids[3], values[3]);
	EXPECT_NE(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 4);

	delete[] __scratchpadBuffer;
	__scratchpadBuffer = nullptr;
}

TEST(TestQPIHashMap, TestReplace) {
	constexpr QPI::uint64 capacity = 8;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[3] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45} };
	int values[3] = { 17, 5467, 8752 };
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 3; ++i) {
		returnedIndex = hashMap.add(ids[i], values[i]);
	}
	EXPECT_EQ(hashMap.population(), 3);

	EXPECT_FALSE(hashMap.replace({1,2,3,4}, 42));
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

TEST(TestQPIHashMap, TestReset) {
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	QPI::id ids[4] = { {87, 456, 29, 823}, {897, 23, 64, 90}, {5478, 908, 123, 45}, {9754, 932, 237, 1} };
	int values[4] = { 17, 5467, 8752, 342 };

	for (int i = 0; i < 4; ++i) {
		hashMap.add(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.population(), 4);
	hashMap.reset();
	EXPECT_EQ(hashMap.population(), 0);
}