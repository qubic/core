#pragma once

#include "../src/contracts/qpi.h"
#include "../src/kangaroo_twelve.h"
#include "../src/platform/m256.h"

namespace QPI {

	template <typename KeyT> class HashFunction {
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

		// Internal reinitialize as empty hash map.
		void _softReset();

		// Return index of element with key in hash map _elements, or NULL_INDEX if not found
		sint64 _elementIndex(const KeyT& key) const;

		// Read and encode 32 POV occupation flags, return a 64bits number presents 32 occupation flags
		uint64 _getEncodedOccupationFlags(const uint64* occupationFlags, const sint64 elementIndex) const;

	public:
		// Add element (key, value) to the hash map, return elementIndex of new element.
		// If key already exists in the hash map, the old value will be overwritten.
		sint64 add(const KeyT& key, const ValueT& value);

		// Return maximum number of elements that may be stored.
		static constexpr uint64 capacity()
		{
			return L;
		}

		// Remove all elements marked for removal, this is a very expensive operation.
		void cleanup();

		// Return key at elementIndex.
		inline KeyT key(sint64 elementIndex) const;

		// Return key at elementIndex.
		inline ValueT value(sint64 elementIndex) const;

		// Return overall number of elements.
		inline uint64 population() const;

		// Mark element for removal.
		void remove(sint64 elementIdx);

		// Mark element for removal if key is contained in the hash map, 
		// returning the elementIndex (or NULL_INDEX if the hash map does not contain the key).
		sint64 remove(const KeyT& key);

		// Replace value for *existing* key, do nothing otherwise.
		// - The key exists: replace its value.
		// - The key is not contained in the hash map: no action is taken.
		void replace(const KeyT& key, const ValueT& newValue);

		// Reinitialize as empty hash map.
		void reset();
	};

}