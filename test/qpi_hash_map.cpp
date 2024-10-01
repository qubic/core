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
#include "../src/contract_core/qpi_hash_map_impl.h"
#include <unordered_set>
#include <array>
#include <ranges>


// New KeyT, ValueT combinations for testing need to implement the following functions:
template <typename KeyT, typename ValueT>
struct HashMapTestData
{
	static std::array<std::pair<KeyT, ValueT>, 4> CreateKeyValueTestPairs();
	static inline KeyT GetKeyNotInTestPairs();
	static inline ValueT GetValueNotInTestPairs();
};

// Test data for KeyT = QPI::id, ValueT = int.
template<>
std::array<std::pair<QPI::id, int>, 4> HashMapTestData<QPI::id, int>::CreateKeyValueTestPairs()
{
	std::array<std::pair<QPI::id, int>, 4> res = { {
		{{  87, 456,  29, 823},    17 },
		{{ 897,  23,  64,  90 }, 5467},
		{{5478, 908, 123,  45},  8752},
		{{ 143, 746,  87,   6 }, 5348},
	} };
	return res;
}

template<>
inline QPI::id HashMapTestData<QPI::id, int>::GetKeyNotInTestPairs()
{
	return { 1,2,3,4 };
}

template<>
inline int HashMapTestData<QPI::id, int>::GetValueNotInTestPairs()
{
	return 42;
}

// Test data for KeyT = QPI::sint64, ValueT = char.
template<>
std::array<std::pair<QPI::sint64, char>, 4> HashMapTestData<QPI::sint64, char>::CreateKeyValueTestPairs()
{
	std::array<std::pair<QPI::sint64, char>, 4> res = { {
		{ -564, 'a'},
		{   67, 'z'},
		{ -783, 'f'},
		{ 8924, 'h'},
	} };
	return res;
}

template<>
inline QPI::sint64 HashMapTestData<QPI::sint64, char>::GetKeyNotInTestPairs()
{
	return 1234;
}

template<>
inline char HashMapTestData<QPI::sint64, char>::GetValueNotInTestPairs()
{
	return '*';
}


// Define the test fixture class with a single template parameter T because the type list
// for test instantiation will contain pair-types std::pair<KeyT, ValueT> to use for T.
template <typename T>
class QPIHashMapTest : public testing::Test {};

using testing::Types;

TYPED_TEST_CASE_P(QPIHashMapTest);


TEST(NonTypedQPIHashMapTest, TestHashFunctionQPIID)
{
	auto randomId = QPI::id::randomValue();

	// Check that the hash function returns the first 8 bytes as hash for QPI::id.
	QPI::uint64 hashRes = QPI::HashFunction<QPI::id>::hash(randomId);
	EXPECT_EQ(hashRes, randomId.u64._0);
}

TEST(NonTypedQPIHashMapTest, TestHashFunction)
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

TYPED_TEST_P(QPIHashMapTest, TestCreation)
{
	constexpr QPI::uint64 capacity = 2;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	EXPECT_EQ(hashMap.capacity(), capacity);
	EXPECT_EQ(hashMap.population(), 0);
}

TYPED_TEST_P(QPIHashMapTest, TestGetters)
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	typedef HashMapTestData<TypeParam::first_type, TypeParam::second_type> TestData;

	std::array<TypeParam, 4> keyValuePairs = TestData::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 4; ++i)
	{
		returnedIndex = hashMap.set(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.getElementIndex(ids[3]), returnedIndex);
	EXPECT_EQ(hashMap.key(returnedIndex), ids[3]);
	EXPECT_EQ(hashMap.value(returnedIndex), values[3]);

	typename TypeParam::second_type valueAfter = {};
	typename TypeParam::second_type valueBefore = valueAfter;

	EXPECT_FALSE(hashMap.get(TestData::GetKeyNotInTestPairs(), valueAfter));
	EXPECT_EQ(valueAfter, valueBefore);

	EXPECT_TRUE(hashMap.get(ids[0], valueAfter));
	EXPECT_EQ(valueAfter, values[0]);

	// Test getElementIndex when all slots are marked for removal so it has to iterate through the whole hash map.
	for (int i = 0; i < 3; ++i)
	{
		hashMap.removeByKey(ids[i]);
	}
	EXPECT_EQ(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);
}

TYPED_TEST_P(QPIHashMapTest, TestSet)
{
	constexpr QPI::uint64 capacity = 2;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	std::array<TypeParam, 4> keyValuePairs = HashMapTestData<TypeParam::first_type, TypeParam::second_type>::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);

	QPI::sint64 returnedIndex = hashMap.set(ids[0], values[0]);
	EXPECT_EQ(hashMap.population(), 1);
	EXPECT_GE(returnedIndex, 0);
	EXPECT_LT(QPI::uint64(returnedIndex), capacity);

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

TYPED_TEST_P(QPIHashMapTest, TestRemove)
{
	constexpr QPI::uint64 capacity = 8;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	typedef HashMapTestData<TypeParam::first_type, TypeParam::second_type> TestData;

	std::array<TypeParam, 4> keyValuePairs = TestData::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);
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
	returnedIndex = hashMap.removeByKey(TestData::GetKeyNotInTestPairs());
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

TYPED_TEST_P(QPIHashMapTest, TestCleanup)
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	__scratchpadBuffer = new char[2 * sizeof(hashMap)];

	std::array<TypeParam, 4> keyValuePairs = HashMapTestData<TypeParam::first_type, TypeParam::second_type>::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);
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

TYPED_TEST_P(QPIHashMapTest, TestCleanupPerformanceShortcuts)
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	std::array<TypeParam, 4> keyValuePairs = HashMapTestData<TypeParam::first_type, TypeParam::second_type>::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);

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

// This test is not type-parameterized because QPI::id is the only type where we can easily create different keys with the same hashes.
TEST(NonTypedQPIHashMapTest, TestCleanupLargeMapSameHashes)
{
	constexpr QPI::uint64 capacity = 64;
	QPI::HashMap<QPI::id, int, capacity> hashMap;

	__scratchpadBuffer = new char[2 * sizeof(hashMap)];

	for (QPI::uint64 i = 0; i < 64; ++i)
	{
		// Add 64 elements with different keys but same hash.
		// The hash for QPI::id is the first 8 bytes.
		hashMap.set({ 3478, i, i + 1, i + 2 }, int(i * 2 + 7));
	}
	hashMap.removeByKey({ 3478, 63, 64, 65 });

	// Cleanup will have to iterate through the whole map to find an empty slot for the last element.
	hashMap.cleanup();

	delete[] __scratchpadBuffer;
	__scratchpadBuffer = nullptr;
}

TYPED_TEST_P(QPIHashMapTest, TestReplace)
{
	constexpr QPI::uint64 capacity = 8;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	typedef HashMapTestData<TypeParam::first_type, TypeParam::second_type> TestData;

	std::array<TypeParam, 4> keyValuePairs = TestData::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 3; ++i)
	{
		returnedIndex = hashMap.set(ids[i], values[i]);
	}
	EXPECT_EQ(hashMap.population(), 3);

	typename TypeParam::first_type newKey = TestData::GetKeyNotInTestPairs();
	typename TypeParam::second_type newValue = TestData::GetValueNotInTestPairs();

	EXPECT_FALSE(hashMap.replace(newKey, newValue));

	EXPECT_EQ(hashMap.population(), 3);
	EXPECT_EQ(hashMap.getElementIndex(newKey), QPI::NULL_INDEX);

	EXPECT_TRUE(hashMap.replace(ids[1], newValue));

	EXPECT_EQ(hashMap.population(), 3);
	typename TypeParam::second_type valueRead = {};
	EXPECT_TRUE(hashMap.get(ids[1], valueRead));
	EXPECT_EQ(valueRead, newValue);
	EXPECT_TRUE(hashMap.get(ids[0], valueRead));
	EXPECT_EQ(valueRead, values[0]);
	EXPECT_TRUE(hashMap.get(ids[2], valueRead));
	EXPECT_EQ(valueRead, values[2]);
}

TYPED_TEST_P(QPIHashMapTest, TestReset)
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	std::array<TypeParam, 4> keyValuePairs = HashMapTestData<TypeParam::first_type, TypeParam::second_type>::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);

	for (int i = 0; i < 4; ++i)
	{
		hashMap.set(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.population(), 4);
	hashMap.reset();
	EXPECT_EQ(hashMap.population(), 0);
}

REGISTER_TYPED_TEST_CASE_P(QPIHashMapTest,
	TestCreation,
	TestGetters,
	TestSet,
	TestRemove,
	TestCleanup,
	TestCleanupPerformanceShortcuts,
	TestReplace,
	TestReset
);

typedef Types<std::pair<QPI::id, int>, std::pair<QPI::sint64, char>> KeyValueTypesToTest;
INSTANTIATE_TYPED_TEST_CASE_P(TypedQPIHashMapTests, QPIHashMapTest, KeyValueTypesToTest);
