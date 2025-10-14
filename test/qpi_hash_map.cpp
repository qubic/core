#define NO_UEFI

#include "gtest/gtest.h"

namespace QPI
{
	struct QpiContextProcedureCall;
	struct QpiContextFunctionCall;
}
typedef void (*USER_FUNCTION)(const QPI::QpiContextFunctionCall&, void* state, void* input, void* output, void* locals);
typedef void (*USER_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);

#include "../src/contracts/qpi.h"
#include "../src/common_buffers.h"
#include "../src/contract_core/qpi_hash_map_impl.h"
#include <unordered_set>
#include <array>
#include <ranges>
#include <set>
#include <random>
#include <chrono>


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


// testdata for KeyT = QPI::BitArray<1024>, ValueT = uint64
template<>
std::array<std::pair<QPI::bit_1024, QPI::uint64>, 4>  HashMapTestData<QPI::bit_1024, QPI::uint64>::CreateKeyValueTestPairs()
{
    // Create a properly sized array to work with BitArray's setMem
    alignas(32) unsigned char buffer[128] = {0}; // 1024 bits = 128 bytes

    // Helper lambda to set a string pattern in bit_1024
    auto setStringKey = [](QPI::bit_1024& bits, const std::string& str) {
        bits.setAll(0);  // Clear all bits first
        // Each character becomes 8 bits
        for(size_t i = 0; i < str.length() && i < 128; i++)  // 128 is max bytes (1024/8)
        {
            unsigned char c = str[i];
            // Set 8 bits for this character
            for(int bit = 0; bit < 8; bit++)
            {
                bits.set(i * 8 + bit, (c & (1 << bit)) != 0);
            }
        }
    };

    QPI::bit_1024 key1, key2, key3, key4;
    setStringKey(key1, "TestString1");
    setStringKey(key2, "AnotherTest2");
    setStringKey(key3, "ThirdString3");
    setStringKey(key4, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()_-+={}[]|;:\"\'<>,.?/~`abcdefghijklmnopqrstuvwxyzABCDEFGH");

    std::array<std::pair<QPI::bit_1024, QPI::uint64>, 4> res;
	res = { {
		{ key1, 12},
		{ key2, 99},
		{ key3, 723},
		{ key4, 0},
	} };
	return res;
}

template<>
inline QPI::bit_1024 HashMapTestData<QPI::bit_1024, QPI::uint64>::GetKeyNotInTestPairs()
{
    alignas(32) unsigned char buffer[128] = {0};
    QPI::bit_1024 key;
    std::string str = "NotInTestPairs";
    for(size_t i = 0; i < str.length() && i < 128; i++)
    {
        unsigned char c = str[i];
        for(int bit = 0; bit < 8; bit++)
        {
            key.set(i * 8 + bit, (c & (1 << bit)) != 0);
        }
    }
    return key;
}

template<>
inline QPI::uint64 HashMapTestData<QPI::bit_1024, QPI::uint64>::GetValueNotInTestPairs()
{
	return 42;
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

	EXPECT_EQ(hashMap.getElementIndex(ids[3]), QPI::NULL_INDEX);
	EXPECT_FALSE(hashMap.contains(ids[3]));
	EXPECT_TRUE(hashMap.isEmptySlot(0));

	for (int i = 0; i < 4; ++i)
	{
		returnedIndex = hashMap.set(ids[i], values[i]);
	}

	EXPECT_EQ(hashMap.getElementIndex(ids[3]), returnedIndex);
	EXPECT_TRUE(hashMap.contains(ids[3]));
	EXPECT_EQ(hashMap.key(returnedIndex), ids[3]);
	EXPECT_EQ(hashMap.value(returnedIndex), values[3]);
	EXPECT_FALSE(hashMap.isEmptySlot(returnedIndex));

	typename TypeParam::second_type valueAfter = {};
	typename TypeParam::second_type valueBefore = valueAfter;

	EXPECT_FALSE(hashMap.get(TestData::GetKeyNotInTestPairs(), valueAfter));
	EXPECT_EQ(valueAfter, valueBefore);

	EXPECT_TRUE(hashMap.get(ids[0], valueAfter));
	EXPECT_EQ(valueAfter, values[0]);

	// Test getElementIndex when all slots are marked for removal so it has to iterate through the whole hash map.
	for (int i = 0; i < 4; ++i)
	{
		hashMap.removeByKey(ids[i]);
	}
	EXPECT_EQ(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);

	EXPECT_TRUE(hashMap.isEmptySlot(0));

	hashMap.cleanupIfNeeded();
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
	EXPECT_TRUE(hashMap.contains(ids[0]));
	EXPECT_TRUE(hashMap.contains(ids[1]));
	EXPECT_FALSE(hashMap.contains(ids[2]));

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
	EXPECT_FALSE(hashMap.contains(ids[0]));
	EXPECT_TRUE(hashMap.contains(ids[1]));
	EXPECT_FALSE(hashMap.contains(ids[2]));
}

TYPED_TEST_P(QPIHashMapTest, TestRemoveReuse)
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;
	hashMap.reset();

	typedef HashMapTestData<TypeParam::first_type, TypeParam::second_type> TestData;

	const auto keyValuePairs = TestData::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);

    // add and remove same item multiple times for testing simple case of reusing slot
    for (int i = 0; i < capacity; ++i)
    {
        for (int j = 0; j <= i; ++j)
        {
            QPI::sint64 elementIndex =  hashMap.set(ids[j], values[j]);
            EXPECT_NE(elementIndex, QPI::NULL_INDEX);
			EXPECT_FALSE(hashMap.isEmptySlot(elementIndex));
			EXPECT_EQ(hashMap.key(elementIndex), ids[j]);
			EXPECT_EQ(hashMap.value(elementIndex), values[j]);
			EXPECT_EQ(hashMap.population(), 1);

			hashMap.removeByIndex(elementIndex);
			EXPECT_TRUE(hashMap.isEmptySlot(elementIndex));
			EXPECT_EQ(hashMap.population(), 0);
        }
    }

	// fill to full capacity
	for (int i = capacity - 1; i >= 0; --i)
	{
		QPI::sint64 elementIndex = hashMap.set(ids[i], values[i]);
		EXPECT_NE(elementIndex, QPI::NULL_INDEX);
		EXPECT_FALSE(hashMap.isEmptySlot(elementIndex));
		EXPECT_EQ(hashMap.key(elementIndex), ids[i]);
		EXPECT_EQ(hashMap.value(elementIndex), values[i]);
		EXPECT_EQ(hashMap.population(), capacity - i);
	}

	// adding another item fails
	EXPECT_EQ(hashMap.set(TestData::GetKeyNotInTestPairs(), TestData::GetValueNotInTestPairs()), QPI::NULL_INDEX);

	// mark all items for removal
	for (int i = 0; i < capacity; ++i)
	{
		EXPECT_NE(hashMap.removeByKey(ids[i]), QPI::NULL_INDEX);
	}
	EXPECT_EQ(hashMap.population(), 0);

	// lookup of non-existing key will now need to iterate through the whole hash map until next cleanup
	EXPECT_FALSE(hashMap.contains(ids[0]));

	// for adding an item, set() also needs to go through the whole hash map until next cleanup
	EXPECT_NE(hashMap.set(ids[0], values[0]), QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 1);
}

TYPED_TEST_P(QPIHashMapTest, TestCleanup)
{
	constexpr QPI::uint64 capacity = 4;
	QPI::HashMap<TypeParam::first_type, TypeParam::second_type, capacity> hashMap;

	reorgBuffer = new char[2 * sizeof(hashMap)];

	std::array<TypeParam, 4> keyValuePairs = HashMapTestData<TypeParam::first_type, TypeParam::second_type>::CreateKeyValueTestPairs();
	auto ids = std::views::keys(keyValuePairs);
	auto values = std::views::values(keyValuePairs);
	QPI::sint64 returnedIndex;

	for (int i = 0; i < 4; ++i)
	{
		hashMap.set(ids[i], values[i]);
	}

	// This will mark for removal
	hashMap.removeByKey(ids[3]);
	EXPECT_EQ(hashMap.population(), 3);

	// Slots marked for removal will be reused, but performance may suffer with increasing number of removes without
	// calling cleanup
	EXPECT_NE(hashMap.set(ids[3], values[3]), QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 4);

	// Remove again
	hashMap.removeByKey(ids[3]);
	EXPECT_EQ(hashMap.population(), 3);

	// Cleanup will properly remove the element marked for removal.
	hashMap.cleanup();
	EXPECT_EQ(hashMap.population(), 3);

	EXPECT_NE(hashMap.getElementIndex(ids[0]), QPI::NULL_INDEX);
	EXPECT_NE(hashMap.getElementIndex(ids[1]), QPI::NULL_INDEX);
	EXPECT_NE(hashMap.getElementIndex(ids[2]), QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.getElementIndex(ids[3]), QPI::NULL_INDEX);

	// Regular set() without reusing slot of element marked for removal (after cleanup, faster on average)
	returnedIndex = hashMap.set(ids[3], values[3]);
	EXPECT_NE(returnedIndex, QPI::NULL_INDEX);
	EXPECT_EQ(hashMap.population(), 4);

	delete[] reorgBuffer;
	reorgBuffer = nullptr;
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

	reorgBuffer = new char[2 * sizeof(hashMap)];

	for (QPI::uint64 i = 0; i < 64; ++i)
	{
		// Add 64 elements with different keys but same hash.
		// The hash for QPI::id is the first 8 bytes.
		hashMap.set({ 3478, i, i + 1, i + 2 }, int(i * 2 + 7));
	}
	hashMap.removeByKey({ 3478, 63, 64, 65 });

	// Cleanup will have to iterate through the whole map to find an empty slot for the last element.
	hashMap.cleanup();

	delete[] reorgBuffer;
	reorgBuffer = nullptr;
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
	TestRemoveReuse,
	TestCleanup,
	TestCleanupPerformanceShortcuts,
	TestReplace,
	TestReset
);

typedef Types<std::pair<QPI::id, int>, std::pair<QPI::sint64, char>, std::pair<QPI::bit_1024, QPI::uint64>> KeyValueTypesToTest;
INSTANTIATE_TYPED_TEST_CASE_P(TypedQPIHashMapTests, QPIHashMapTest, KeyValueTypesToTest);

template <class KeyT, class ValueT, unsigned int capacity>
void hasSameContent(QPI::HashMap<KeyT, ValueT, capacity>& map, const std::map<KeyT, ValueT>& referenceMap)
{
	EXPECT_EQ(map.population(), referenceMap.size());
	for (const auto& item : referenceMap)
	{
		EXPECT_TRUE(map.contains(item.first));
		ValueT value;
		EXPECT_TRUE(map.get(item.first, value));
		EXPECT_EQ(value, item.second);
	}
	for (unsigned int i = 0; i < capacity; ++i)
	{
		KeyT key = map.key(i);	// returns key value at slot i (like 0), even if the value is not contained
		if (!map.isEmptySlot(i))
		{
			// key is valid, part of map, and exactly at this position
			EXPECT_NE(referenceMap.find(key), referenceMap.end());
			EXPECT_EQ(map.getElementIndex(key), i);
		}
		else
		{
			// key is invalid and not at this slot
			QPI::uint64 elementIndex = map.getElementIndex(key);
			EXPECT_NE(elementIndex, i);
			if (elementIndex == QPI::NULL_INDEX)
			{
				// not in map
				EXPECT_EQ(referenceMap.find(key), referenceMap.end());
			}
			else
			{
				// in map, but at different position
				EXPECT_NE(referenceMap.find(key), referenceMap.end());
			}
		}
	}

	// test iterator
	QPI::sint64 i = map.nextElementIndex(QPI::NULL_INDEX);
	unsigned int cnt = 0;
	while (i != QPI::NULL_INDEX)
	{
		cnt++;
		EXPECT_FALSE(map.isEmptySlot(i));
		auto it = referenceMap.find(map.key(i));
		EXPECT_NE(it, referenceMap.end());
		EXPECT_EQ(it->second, map.value(i));
		i = map.nextElementIndex(i);
	}
	EXPECT_EQ(cnt, referenceMap.size());
}

template <class KeyT, class ValueT, unsigned int capacity>
void cleanupHashMap(QPI::HashMap<KeyT, ValueT, capacity>& map, const std::map<KeyT, ValueT>& referenceMap)
{
	hasSameContent(map, referenceMap);
	map.cleanup();
	//map.cleanupIfNeeded();
	hasSameContent(map, referenceMap);
}

void getValue(std::mt19937_64& gen64, QPI::id& value)
{
	value.u64._0 = gen64();
	value.u64._1 = gen64();
	value.u64._2 = gen64();
	value.u64._3 = gen64();
}

void getValue(std::mt19937_64& gen64, QPI::uint8& value)
{
	value = gen64() & 0xff;
}

template <class KeyT, class ValueT, unsigned int capacity>
void testHashMapPseudoRandom(int seed, int cleanups, int percentAdd, int percentAddSecondHalf = -1)
{
	// add and remove entries with pseudo-random sequence
	std::mt19937_64 gen64(seed);

	std::map<KeyT, ValueT> referenceMap;
	QPI::HashMap<KeyT, ValueT, capacity> map;

	reorgBuffer = new char[2 * sizeof(map)];

	map.reset();

	// test cleanup of empty collection
	cleanupHashMap(map, referenceMap);

	if (seed & 1)
	{
		// add default value of empty slot to set
		referenceMap[map.key(0)] = map.value(0);
		EXPECT_NE(map.set(map.key(0), map.value(0)), QPI::NULL_INDEX);
		hasSameContent(map, referenceMap);
	}

	// Randomly add, remove, and cleanup until 50 cleanups are reached
	int cleanupCounter = 0;
	while (cleanupCounter < cleanups)
	{
		int p = gen64() % 100;

		if (p == 0)
		{
			// cleanup (with 1% probability)
			cleanupHashMap(map, referenceMap);
			++cleanupCounter;

			if (cleanupCounter == cleanups / 2 && percentAddSecondHalf >= 0)
				percentAdd = percentAddSecondHalf;
		}

		if (p < percentAdd)
		{
			// add to map (more probable than remove)
			KeyT key;
			ValueT value;
			getValue(gen64, key);
			getValue(gen64, value);
			if (map.set(key, value) != QPI::NULL_INDEX)
				referenceMap[key] = value;
			hasSameContent(map, referenceMap);
		}
		else
		{
			// remove from map
			QPI::sint64 removeIdx = gen64() % map.capacity();
			KeyT key = map.key(removeIdx);
			if (!map.isEmptySlot(removeIdx))
			{
				referenceMap.erase(key);
				EXPECT_EQ(map.removeByKey(key), removeIdx);
			}
			else if (referenceMap.find(key) == referenceMap.end())
			{
				EXPECT_EQ(map.removeByKey(key), QPI::NULL_INDEX);
			}
			hasSameContent(map, referenceMap);
		}

		// std::cout << "capacity: " << set.capacity() << ", pupulation:" << set.population() << std::endl;
	}

	delete[] reorgBuffer;
	reorgBuffer = nullptr;
}

TEST(QPIHashMapTest, HashMapPseudoRandom)
{
	constexpr unsigned int numCleanups = 5;
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 1>(42, numCleanups, 20);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 1>(1337, numCleanups, 80);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 8>(42, 4 * numCleanups, 30);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 8>(1337, 4 * numCleanups, 50);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 8>(123456789, 4 * numCleanups, 70);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 8>(42, 10 * numCleanups, 30, 70);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 8>(1337, 10 * numCleanups, 50, 10);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 8>(123456789, 10 * numCleanups, 70, 10);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 128>(42 + 1, 6 * numCleanups, 30);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 128>(1337 + 1, 6 * numCleanups, 50);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 128>(123456789 + 1, 6 * numCleanups, 70);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 1024>(42 + 2, 10 * numCleanups, 30);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 1024>(1337 + 2, 10 * numCleanups, 50);
	testHashMapPseudoRandom<QPI::id, QPI::uint8, 1024>(123456789 + 2, 10 * numCleanups, 70);
	testHashMapPseudoRandom<QPI::uint8, QPI::id, 1>(42, numCleanups, 20);
	testHashMapPseudoRandom<QPI::uint8, QPI::id, 8>(42 + 1, 10 * numCleanups, 30, 70);
	testHashMapPseudoRandom<QPI::uint8, QPI::id, 8>(1337, 10 * numCleanups, 50, 10);
	testHashMapPseudoRandom<QPI::uint8, QPI::id, 8>(123456789, 10 * numCleanups, 70, 10);
	testHashMapPseudoRandom<QPI::uint8, QPI::id, 128>(1337 + 1, 6 * numCleanups, 50);
	testHashMapPseudoRandom<QPI::uint8, QPI::id, 1024>(123456789 + 2, 10 * numCleanups, 70);
}

TEST(QPIHashMapTest, HashSet)
{
	constexpr QPI::uint64 capacity = 128;
	QPI::HashSet<QPI::id, capacity> hashSet;
	reorgBuffer = new char[2 * sizeof(hashSet)];
	EXPECT_EQ(hashSet.capacity(), capacity);

	// Test add() and contains()
	for (int i = 0; i < capacity; ++i)
	{
		const QPI::id newId(i / 3, i + 5, i * 3, i % 10);
		EXPECT_EQ(hashSet.population(), i);
		auto idx = hashSet.add(newId);
		EXPECT_NE(idx, QPI::NULL_INDEX);
		EXPECT_EQ(hashSet.key(idx), newId);
		EXPECT_FALSE(hashSet.isEmptySlot(idx));
		EXPECT_EQ(idx, hashSet.add(newId)); // adding a second time just returns same index
		EXPECT_TRUE(hashSet.contains(newId));
	}
	EXPECT_EQ(hashSet.population(), capacity);
	EXPECT_FALSE(hashSet.contains(QPI::NULL_ID));
	EXPECT_EQ(hashSet.add(QPI::NULL_ID), QPI::NULL_INDEX); // set is full
	EXPECT_FALSE(hashSet.contains(QPI::NULL_ID));

	// Test remove()
	EXPECT_EQ(hashSet.remove(QPI::NULL_ID), QPI::NULL_INDEX);
	for (int i = 0; i < capacity; i += 4)
	{
		const QPI::id theId(i / 3, i + 5, i * 3, i % 10);
		EXPECT_EQ(hashSet.population(), capacity - i / 4);
		EXPECT_TRUE(hashSet.contains(theId));
		EXPECT_NE(hashSet.remove(theId), QPI::NULL_INDEX);
		EXPECT_FALSE(hashSet.contains(theId));
	}

	// Check consistency
	for (int i = 0; i < capacity; i++)
	{
		const QPI::id theId(i / 3, i + 5, i * 3, i % 10);
		if (i % 4 == 0)
			EXPECT_FALSE(hashSet.contains(theId));
		else
			EXPECT_TRUE(hashSet.contains(theId));
	}

	// Check that it works to reuse slots of removed entries
	for (int i = 0; i < capacity / 4; ++i)
	{
		const QPI::id newId(capacity / 4 - 1 - i, 0, 0, 0);
		EXPECT_EQ(hashSet.population(), capacity * 3 / 4 + i);
		auto idx = hashSet.add(newId);
		EXPECT_NE(idx, QPI::NULL_INDEX);
		EXPECT_EQ(hashSet.key(idx), newId);
		EXPECT_FALSE(hashSet.isEmptySlot(idx));
		EXPECT_EQ(idx, hashSet.add(newId)); // adding a second time just returns same index
		EXPECT_TRUE(hashSet.contains(newId));
	}
	EXPECT_TRUE(hashSet.contains(QPI::id(0, 0, 0, 0)));

	// Check consistency
	for (int i = 0; i < capacity; i++)
	{
		const QPI::id theId(i / 3, i + 5, i * 3, i % 10);
		if (i % 4 == 0)
			EXPECT_FALSE(hashSet.contains(theId));
		else
			EXPECT_TRUE(hashSet.contains(theId));
		if (i < capacity / 4)
		{
			const QPI::id theId(capacity / 4 - 1 - i, 0, 0, 0);
			EXPECT_TRUE(hashSet.contains(theId));
		}
	}

	// Remove entries added first
	for (int i = 0; i < capacity; i++)
	{
		const QPI::id theId(i / 3, i + 5, i * 3, i % 10);
		if (i % 4 == 0)
			EXPECT_EQ(hashSet.remove(theId), QPI::NULL_INDEX); // already removed before
		else
			EXPECT_NE(hashSet.remove(theId), QPI::NULL_INDEX);
		EXPECT_FALSE(hashSet.contains(theId));
	}

	// Reorganize hash map, speeding up access
	hashSet.cleanup();

	// Check consistency
	EXPECT_EQ(hashSet.population(), capacity / 4);
	for (int i = 0; i < capacity / 4; ++i)
	{
		EXPECT_TRUE(hashSet.contains(QPI::id(i, 0, 0, 0)));
	}
	for (int i = 0; i < capacity; i++)
	{
		EXPECT_FALSE(hashSet.contains(QPI::id(i / 3, i + 5, i * 3, i % 10)));
	}

	hashSet.reset();
	EXPECT_EQ(hashSet.population(), 0);

	delete[] reorgBuffer;
	reorgBuffer = nullptr;
}

template <class T, unsigned int capacity>
void hasSameContent(QPI::HashSet<T, capacity>& set, const std::set<T>& referenceSet)
{
	EXPECT_EQ(set.population(), referenceSet.size());
	for (const T& item: referenceSet)
	{
		EXPECT_TRUE(set.contains(item));
	}
	for (unsigned int i = 0; i < capacity; ++i)
	{
		T key = set.key(i);	// returns key value at slot i (like 0), even if the value is not contained
		if (!set.isEmptySlot(i))
		{
			// key is valid, part of set, and exactly at this position
			EXPECT_NE(referenceSet.find(key), referenceSet.end());
			EXPECT_EQ(set.getElementIndex(key), i);
		}
		else
		{
			// key is invalid and not at this slot
			QPI::uint64 elementIndex = set.getElementIndex(key);
			EXPECT_NE(elementIndex, i);
			if (elementIndex == QPI::NULL_INDEX)
			{
				// not in set
				EXPECT_EQ(referenceSet.find(key), referenceSet.end());
			}
			else
			{
				// in set, but at different position
				EXPECT_NE(referenceSet.find(key), referenceSet.end());
			}
		}
	}

	// test iterator
	QPI::sint64 i = set.nextElementIndex(QPI::NULL_INDEX);
	unsigned int cnt = 0;
	while (i != QPI::NULL_INDEX)
	{
		cnt++;
		EXPECT_FALSE(set.isEmptySlot(i));
		EXPECT_NE(referenceSet.find(set.key(i)), referenceSet.end());
		i = set.nextElementIndex(i);
	}
	EXPECT_EQ(cnt, referenceSet.size());
}

template <class T, unsigned int capacity>
void cleanupHashSet(QPI::HashSet<T, capacity>& set, const std::set<T>& referenceSet)
{
	hasSameContent(set, referenceSet);
	set.cleanup();
	//set.cleanupIfNeeded();
	hasSameContent(set, referenceSet);
}

template <class T, unsigned int capacity>
void testHashSetPseudoRandom(int seed, int cleanups, int percentAdd, int percentAddSecondHalf = -1)
{
	// add and remove entries with pseudo-random sequence
	std::mt19937_64 gen64(seed);

	std::set<T> referenceSet;
	QPI::HashSet<T, capacity> set;

	reorgBuffer = new char[2 * sizeof(set)];

	set.reset();

	// test cleanup of empty collection
	cleanupHashSet(set, referenceSet);

	if (seed & 1)
	{
		// add default value of empty slot to set
		referenceSet.insert(set.key(0));
		EXPECT_NE(set.add(set.key(0)), QPI::NULL_INDEX);
		hasSameContent(set, referenceSet);
	}

	// Randomly add, remove, and cleanup until 50 cleanups are reached
	int cleanupCounter = 0;
	while (cleanupCounter < cleanups)
	{
		int p = gen64() % 100;

		if (p == 0)
		{
			// cleanup (with 1% probability)
			cleanupHashSet(set, referenceSet);
			++cleanupCounter;

			if (cleanupCounter == cleanups / 2 && percentAddSecondHalf >= 0)
				percentAdd = percentAddSecondHalf;
		}

		if (p < percentAdd)
		{
			// add to set (more probable than remove)
			T value;
			getValue(gen64, value);
			if (set.add(value) != QPI::NULL_INDEX)
				referenceSet.insert(value);
			hasSameContent(set, referenceSet);
		}
		else
		{
			// remove from set
			QPI::sint64 removeIdx = gen64() % set.capacity();
			T key = set.key(removeIdx);
			if (!set.isEmptySlot(removeIdx))
			{
				referenceSet.erase(key);
				EXPECT_EQ(set.remove(key), removeIdx);
			}
			else if (referenceSet.find(key) == referenceSet.end())
			{
				EXPECT_EQ(set.remove(key), QPI::NULL_INDEX);
			}
			hasSameContent(set, referenceSet);
		}

		// std::cout << "capacity: " << set.capacity() << ", pupulation:" << set.population() << std::endl;
	}

	delete[] reorgBuffer;
	reorgBuffer = nullptr;
}

TEST(QPIHashMapTest, HashSetPseudoRandom)
{
	constexpr unsigned int numCleanups = 5;
	testHashSetPseudoRandom<QPI::id, 1>(42, numCleanups, 20);
	testHashSetPseudoRandom<QPI::id, 1>(1337, numCleanups, 80);
	testHashSetPseudoRandom<QPI::id, 8>(42, 4 * numCleanups, 30);
	testHashSetPseudoRandom<QPI::id, 8>(1337, 4 * numCleanups, 50);
	testHashSetPseudoRandom<QPI::id, 8>(123456789, 4 * numCleanups, 70);
	testHashSetPseudoRandom<QPI::id, 8>(42, 10 * numCleanups, 30, 70);
	testHashSetPseudoRandom<QPI::id, 8>(1337, 10 * numCleanups, 50, 10);
	testHashSetPseudoRandom<QPI::id, 8>(123456789, 10 * numCleanups, 70, 10);
	testHashSetPseudoRandom<QPI::id, 128>(42 + 1, 6 * numCleanups, 30);
	testHashSetPseudoRandom<QPI::id, 128>(1337 + 1, 6 * numCleanups, 50);
	testHashSetPseudoRandom<QPI::id, 128>(123456789 + 1, 6 * numCleanups, 70);
	testHashSetPseudoRandom<QPI::id, 1024>(42 + 2, 10 * numCleanups, 30);
	testHashSetPseudoRandom<QPI::id, 1024>(1337 + 2, 10 * numCleanups, 50);
	testHashSetPseudoRandom<QPI::id, 1024>(123456789 + 2, 10 * numCleanups, 70);
	testHashSetPseudoRandom<QPI::uint8, 1>(42, numCleanups, 20);
	testHashSetPseudoRandom<QPI::uint8, 8>(42 + 1, 10 * numCleanups, 30, 70);
	testHashSetPseudoRandom<QPI::uint8, 8>(1337, 10 * numCleanups, 50, 10);
	testHashSetPseudoRandom<QPI::uint8, 8>(123456789, 10 * numCleanups, 70, 10);
	testHashSetPseudoRandom<QPI::uint8, 128>(1337 + 1, 6 * numCleanups, 50);
	testHashSetPseudoRandom<QPI::uint8, 1024>(123456789 + 2, 10 * numCleanups, 70);
}



template <unsigned int capacity>
static void perfTestCleanup(int seed)
{
	std::mt19937_64 gen64(seed);

	auto* set = new QPI::HashSet<QPI::id, capacity>();
	reorgBuffer = new char[sizeof(*set)];

	for (QPI::uint64 i = 1; i <= 100; ++i)
	{
		QPI::uint64 population = capacity * i / 100;
		set->reset();

		// add random items
		auto startTime1 = std::chrono::high_resolution_clock::now();
		for (QPI::uint64 j = 0; j < population; ++j)
		{
			EXPECT_NE(set->add(QPI::id(gen64(), 0, 0, 0)), QPI::NULL_INDEX);
		}
		auto addMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime1).count();

		// measure run-time of cleanup
		auto startTime2 = std::chrono::high_resolution_clock::now();
		set->cleanup();
		auto cleanupMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime2).count();

		std::cout << "HastSet<uint64, " << capacity << ">::cleanup() with " << population * 100 / capacity << "% population takes " << cleanupMilliseconds << " ms (adding took " << addMilliseconds * 1000000 / population << " ms/million)" << std::endl;
	}

	delete set;
	delete[] reorgBuffer;
	reorgBuffer = nullptr;
}

TEST(QPIHashMapTest, HashSetPerfTest)
{
	// How often should cleanup() be run?

	// for a set of capacities: vary population
	// keep sequence of add/remove

	// measure run-time of cleanups -> O(N^2) with N = population
	//perfTestCleanup<2 * 1024 * 1024>(42);
	//perfTestCleanup<2 * 1024 * 1024>(13);

	// measure lookups/seconds -> O(1) if population is sparse -> O(N) if population is high with N = max population since last cleanup
}
