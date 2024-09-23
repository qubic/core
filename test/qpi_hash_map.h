#pragma once

#include "../src/contracts/qpi.h"
#include "../src/kangaroo_twelve.h"
#include "../src/platform/m256.h"

namespace QPI {

	template <typename KeyT> class HashFunction {
	public:
		static uint64 hash(const KeyT& key) {
			uint64 ret;
			KangarooTwelve(&key, sizeof(KeyT), &ret, 8);
			return ret;
		}
	};

	template <>
	static uint64 HashFunction<m256i>::hash(const m256i& key) {
		return key.u64._0;
	}

	// Hash map of (key, value) pairs of type (KeyT, ValueT) and total element capacity L.
	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc = HashFunction<KeyT>>
	class HashMap
	{
	private:
		static_assert(L && !(L& (L - 1)),
			"The capacity of the hash map must be 2^N."
			);
		static constexpr sint64 _nEncodedFlags = L > 32 ? 32 : L;

		// Hash map of (key, value) pairs
		struct Element
		{
			KeyT key;
			ValueT value;
		} _elements[L];

		// 2 bits per element of _elements: 0b00 = not occupied; 0b01 = occupied; 0b10 = occupied but marked for removal; 0b11 is unused
		// The state "occupied but marked for removal" is needed for finding the index of a key in the hash map. Setting an entry to
		// "not occupied" in remove() would potentially undo a collision, create a gap, and mess up the entry search.
		uint64 _occupationFlags[(L * 2 + 63) / 64];

		uint64 _population;
		uint64 _markRemovalCounter;

		// Read and encode 32 POV occupation flags, return a 64bits number presents 32 occupation flags
		uint64 _getEncodedOccupationFlags(const uint64* occupationFlags, const sint64 elementIndex) const;

	public:
		HashMap() 
		{
			reset();
		}

		// Return maximum number of elements that may be stored.
		static constexpr uint64 capacity()
		{
			return L;
		}

		// Return overall number of elements.
		inline uint64 population() const;

		// Return boolean indicating whether key is contained in the hash map.
		// If key is contained, write the associated value into the provided ValueT&. 
		bool get(const KeyT& key, ValueT& value) const;

		// Return index of element with key in hash map _elements, or NULL_INDEX if not found.
		sint64 getElementIndex(const KeyT& key) const;

		// Return key at elementIndex.
		inline KeyT key(sint64 elementIndex) const;

		// Return value at elementIndex.
		inline ValueT value(sint64 elementIndex) const;

		// Add element (key, value) to the hash map, return elementIndex of new element.
		// If key already exists in the hash map, the old value will be overwritten.
		// If the hash map is full, return NULL_INDEX.
		sint64 set(const KeyT& key, const ValueT& value);

		// Mark element for removal.
		void removeByIndex(sint64 elementIdx);

		// Mark element for removal if key is contained in the hash map, 
		// returning the elementIndex (or NULL_INDEX if the hash map does not contain the key).
		sint64 removeByKey(const KeyT& key);

		// Remove all elements marked for removal, this is a very expensive operation.
		void cleanup();

		// Replace value for *existing* key, do nothing otherwise.
		// - The key exists: replace its value. Return true.
		// - The key is not contained in the hash map: no action is taken. Return false.
		bool replace(const KeyT& key, const ValueT& newValue);

		// Reinitialize as empty hash map.
		void reset();
	};

}