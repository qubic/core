// Implements functions of QPI::HashMap and QPI::HashSet in order to:
// 1. keep setMem() and copyMem() unavailable to contracts
// 2. keep QPI file smaller and easier to read for contract devs
// CAUTION: Include this AFTER the contract implementations!

#pragma once

#include "../contracts/qpi.h"
#include "../platform/memory.h"
#include "../kangaroo_twelve.h"
#include "../contracts/math_lib.h"

namespace QPI
{
	template <typename KeyT>
	uint64 HashFunction<KeyT>::hash(const KeyT& key) 
	{
		uint64 ret;
		KangarooTwelve(&key, sizeof(KeyT), &ret, 8);
		return ret;
	}

	// For performance reasons, we use the first 8 bytes as hash for m256i/id types.
	template <>
	inline uint64 HashFunction<m256i>::hash(const m256i& key)
	{
		return key.u64._0;
	}

	//////////////////////////////////////////////////////////////////////////////
	// HashMap template class

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	uint64 HashMap<KeyT, ValueT, L, HashFunc>::_getEncodedOccupationFlags(const uint64* occupationFlags, const sint64 elementIndex) const
	{
		const sint64 offset = (elementIndex & 31) << 1;
		uint64 flags = occupationFlags[elementIndex >> 5] >> offset;
		if (offset > 0)
		{
			flags |= occupationFlags[((elementIndex + 32) & (L - 1)) >> 5] << (2 * _nEncodedFlags - offset);
		}
		return flags;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	bool HashMap<KeyT, ValueT, L, HashFunc>::contains(const KeyT& key) const
	{
		return getElementIndex(key) != NULL_INDEX;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	bool HashMap<KeyT, ValueT, L, HashFunc>::get(const KeyT& key, ValueT& value) const 
	{
		sint64 elementIndex = getElementIndex(key);
		if (elementIndex != NULL_INDEX) 
		{
			value = _elements[elementIndex].value;
			return true;
		}
		return false;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	sint64 HashMap<KeyT, ValueT, L, HashFunc>::getElementIndex(const KeyT& key) const
	{
		sint64 index = HashFunc::hash(key) & (L - 1);
		for (sint64 counter = 0; counter < L; counter += 32)
		{
			uint64 flags = _getEncodedOccupationFlags(_occupationFlags, index);
			for (auto i = 0; i < _nEncodedFlags; i++, flags >>= 2)
			{
				switch (flags & 3ULL)
				{
				case 0:
					return NULL_INDEX;
				case 1:
					if (_elements[index].key == key)
					{
						return index;
					}
					break;
				}
				index = (index + 1) & (L - 1);
			}
		}
		return NULL_INDEX;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	inline const KeyT& HashMap<KeyT, ValueT, L, HashFunc>::key(sint64 elementIndex) const
	{
		return _elements[elementIndex & (L - 1)].key;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	inline const ValueT& HashMap<KeyT, ValueT, L, HashFunc>::value(sint64 elementIndex) const
	{
		return _elements[elementIndex & (L - 1)].value;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	inline uint64 HashMap<KeyT, ValueT, L, HashFunc>::population() const
	{
		return _population;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	sint64 HashMap<KeyT, ValueT, L, HashFunc>::set(const KeyT& key, const ValueT& value)
	{
		if (_population < capacity())
		{
			// search in hash map
			sint64 markedForRemovalIndexForReuse = NULL_INDEX;
			sint64 index = HashFunc::hash(key) & (L - 1);
			for (sint64 counter = 0; counter < L; counter += 32)
			{
				uint64 flags = _getEncodedOccupationFlags(_occupationFlags, index);
				for (auto i = 0; i < _nEncodedFlags; i++, flags >>= 2)
				{
					switch (flags & 3ULL)
					{
					case 0:
						// empty entry -> key isn't in set yet
						// If we have already seen an entry marked for removal, reuse this slot because it is closer to the hash index
						if (markedForRemovalIndexForReuse != NULL_INDEX)
							goto reuse_slot;
						// ... otherwise put element and mark as occupied
						_occupationFlags[index >> 5] |= (1ULL << ((index & 31) << 1));
						_elements[index].key = key;
						_elements[index].value = value;
						_population++;
						return index;
					case 1:
						if (_elements[index].key == key)
						{
							// found key -> insert new value
							_elements[index].value = value;
							return index;
						}
						break;
					case 2:
						// marked for removal -> reuse slot (first slot we see) later if we are sure that key isn't in the map
						if (markedForRemovalIndexForReuse == NULL_INDEX)
							markedForRemovalIndexForReuse = index;
						break;
					}
					index = (index + 1) & (L - 1);
				}
			}

			if (markedForRemovalIndexForReuse != NULL_INDEX)
			{
			reuse_slot:
				// Reuse slot marked for removal: put key here and set flags from 2 to 1.
				// But don't decrement _markRemovalCounter, because it is used to check if cleanup() is needed.
				// Without cleanup, we don't get new unoccupied slots and at least lookup of keys that aren't contained in the map
				// stays slow.
				index = markedForRemovalIndexForReuse;
				_occupationFlags[index >> 5] ^= (3ULL << ((index & 31) << 1));
				_elements[index].key = key;
				_elements[index].value = value;
				_population++;
				return index;
			}
		}
		else // _population == capacity()
		{
			// Check if key exists for value replacement.
			sint64 index = getElementIndex(key);
			if (index != NULL_INDEX)
			{
				_elements[index].value = value;
				return index;
			}
		}
		return NULL_INDEX;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	bool HashMap<KeyT, ValueT, L, HashFunc>::isEmptySlot(sint64 elementIndex) const
	{
		elementIndex &= (L - 1);
		uint64 flags = _getEncodedOccupationFlags(_occupationFlags, elementIndex);
		return ((flags & 3ULL) != 1);
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	sint64 HashMap<KeyT, ValueT, L, HashFunc>::nextElementIndex(sint64 elementIndex) const
	{
		if (!_population)
			return NULL_INDEX;

		if (elementIndex < 0)
			elementIndex = 0;
		else
			++elementIndex;

		// search for next occupied element until end of hash map array
		constexpr uint64 flagsLength = math_lib::max(L >> 5, 1ull);
		sint64 flagsIdx = elementIndex >> 5;
		sint64 offset = elementIndex & 31ll;
		uint64 flags = _occupationFlags[flagsIdx] >> (2 * offset);
		while (flagsIdx < flagsLength)
		{
			for (sint64 i = offset; i < _nEncodedFlags; ++i, flags >>= 2)
			{
				if (!flags)
				{
					// no occupied entries in current flags
					break;
				}
				if ((flags & 3ULL) == 1)
				{
					// found occupied entry
					return (flagsIdx << 5) + i;
				}
			}

			flags = _occupationFlags[++flagsIdx];
			offset = 0;
		}

		return NULL_INDEX;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	void HashMap<KeyT, ValueT, L, HashFunc>::removeByIndex(sint64 elementIdx)
	{
		elementIdx &= (L - 1);
		uint64 flags = _getEncodedOccupationFlags(_occupationFlags, elementIdx);

		if ((flags & 3ULL) == 1)
		{
			_population--;
			_markRemovalCounter++;
			_occupationFlags[elementIdx >> 5] ^= (3ULL << ((elementIdx & 31) << 1));

			const bool CLEAR_UNUSED_ELEMENT = true;
			if (CLEAR_UNUSED_ELEMENT)
			{
				setMem(&_elements[elementIdx], sizeof(Element), 0);
			}
		}
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	sint64 HashMap<KeyT, ValueT, L, HashFunc>::removeByKey(const KeyT& key)
	{
		sint64 elementIndex = getElementIndex(key);
		if (elementIndex == NULL_INDEX)
		{
			return NULL_INDEX;
		}
		else
		{
			removeByIndex(elementIndex);
			return elementIndex;
		}
	}
	
	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	void HashMap<KeyT, ValueT, L, HashFunc>::cleanupIfNeeded(uint64 removalThresholdPercent)
	{
		if (_markRemovalCounter > (removalThresholdPercent * L / 100))
		{
			cleanup();
		}
	}
	
	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	void HashMap<KeyT, ValueT, L, HashFunc>::cleanup()
	{
		// _elements gets occupied over time with entries of type 3 which means they are marked for cleanup.
		// Once cleanup is called it's necessary to remove all these type 3 entries by reconstructing a fresh hash map residing in scratchpad buffer.
		// Cleanup() called for a hash map having only type 3 entries must give the result equal to reset() memory content wise.

		// Quick check to cleanup
		if (!_markRemovalCounter)
		{
			return;
		}

		// Speedup case of empty hash map but existed marked for removal elements
		if (!population())
		{
			reset();
			return;
		}

		// Init buffers
		auto* _elementsBuffer = reinterpret_cast<Element*>(::__scratchpad(sizeof(_elements) + sizeof(_occupationFlags)));
		auto* _occupationFlagsBuffer = reinterpret_cast<uint64*>(_elementsBuffer + L);
		auto* _stackBuffer = reinterpret_cast<sint64*>(
			_occupationFlagsBuffer + sizeof(_occupationFlags) / sizeof(_occupationFlags[0]));
		uint64 newPopulation = 0;

		// Go through hash map. For each element that is occupied but not marked for removal, insert element in new hash map's buffers.
		constexpr uint64 oldIndexGroupCount = (L >> 5) ? (L >> 5) : 1;
		for (sint64 oldIndexGroup = 0; oldIndexGroup < oldIndexGroupCount; oldIndexGroup++)
		{
			const uint64 flags = _occupationFlags[oldIndexGroup];
			uint64 maskBits = (0xAAAAAAAAAAAAAAAA & (flags << 1));
			maskBits &= maskBits ^ (flags & 0xAAAAAAAAAAAAAAAA);
			sint64 oldIndexOffset = _tzcnt_u64(maskBits) & 0xFE;
			const sint64 oldIndexOffsetEnd = 64 - (_lzcnt_u64(maskBits) & 0xFE);
			for (maskBits >>= oldIndexOffset;
				oldIndexOffset < oldIndexOffsetEnd; oldIndexOffset += 2, maskBits >>= 2)
			{
				// Only add elements to new hash map that are occupied and not marked for removal
				if (maskBits & 3ULL)
				{
					// find empty position in new hash map
					const sint64 oldIndex = (oldIndexGroup << 5) + (oldIndexOffset >> 1);
					sint64 newIndex = HashFunc::hash(_elements[oldIndex].key) & (L - 1);
					for (sint64 counter = 0; counter < L; counter += 32)
					{
						QPI::uint64 newFlags = _getEncodedOccupationFlags(_occupationFlagsBuffer, newIndex);
						for (sint64 i = 0; i < _nEncodedFlags; i++, newFlags >>= 2)
						{
							if ((newFlags & 3ULL) == 0)
							{
								newIndex = (newIndex + i) & (L - 1);
								goto foundEmptyPosition;
							}
						}
						newIndex = (newIndex + _nEncodedFlags) & (L - 1);
					}
#ifdef NO_UEFI
					// should never be reached, because old and new map have same capacity (there should always be an empty slot)
					goto cleanupBug;
#endif

				foundEmptyPosition:
					// occupy empty hash map entry
					_occupationFlagsBuffer[newIndex >> 5] |= (1ULL << ((newIndex & 31) << 1));
					copyMem(&_elementsBuffer[newIndex], &_elements[oldIndex], sizeof(Element));

					// check if we are done
					newPopulation += 1;
					if (newPopulation == _population)
					{
						// all elements have been transferred -> overwrite old array with new array
						copyMem(_elements, _elementsBuffer, sizeof(_elements));
						copyMem(_occupationFlags, _occupationFlagsBuffer, sizeof(_occupationFlags));
						_markRemovalCounter = 0;
						return;
					}
				}
			}
		}

#ifdef NO_UEFI
		cleanupBug :
		// don't expect here, certainly got error!!!
		printf("ERROR: Something went wrong at cleanup!\n");
#endif
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	bool HashMap<KeyT, ValueT, L, HashFunc>::replace(const KeyT& key, const ValueT& newValue)
	{
		sint64 elementIndex = getElementIndex(key);
		if (elementIndex != NULL_INDEX) 
		{
			_elements[elementIndex].value = newValue;
			return true;
		}
		return false;
	}

	template <typename KeyT, typename ValueT, uint64 L, typename HashFunc>
	void HashMap<KeyT, ValueT, L, HashFunc>::reset()
	{
		setMem(this, sizeof(*this), 0);
	}

	//////////////////////////////////////////////////////////////////////////////
	// HashSet template class

	template <typename KeyT, uint64 L, typename HashFunc>
	uint64 HashSet<KeyT, L, HashFunc>::_getEncodedOccupationFlags(const uint64* occupationFlags, const sint64 elementIndex) const
	{
		const sint64 offset = (elementIndex & 31) << 1;
		uint64 flags = occupationFlags[elementIndex >> 5] >> offset;
		if (offset > 0)
		{
			flags |= occupationFlags[((elementIndex + 32) & (L - 1)) >> 5] << (2 * _nEncodedFlags - offset);
		}
		return flags;
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	bool HashSet<KeyT, L, HashFunc>::contains(const KeyT& key) const
	{
		return getElementIndex(key) != NULL_INDEX;
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	sint64 HashSet<KeyT, L, HashFunc>::getElementIndex(const KeyT& key) const
	{
		sint64 index = HashFunc::hash(key) & (L - 1);
		for (sint64 counter = 0; counter < L; counter += 32)
		{
			uint64 flags = _getEncodedOccupationFlags(_occupationFlags, index);
			for (auto i = 0; i < _nEncodedFlags; i++, flags >>= 2)
			{
				switch (flags & 3ULL)
				{
				case 0:
					return NULL_INDEX;
				case 1:
					if (_keys[index] == key)
					{
						return index;
					}
					break;
				}
				index = (index + 1) & (L - 1);
			}
		}
		return NULL_INDEX;
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	inline KeyT HashSet<KeyT, L, HashFunc>::key(sint64 elementIndex) const
	{
		return _keys[elementIndex & (L - 1)];
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	inline uint64 HashSet<KeyT, L, HashFunc>::population() const
	{
		return _population;
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	sint64 HashSet<KeyT, L, HashFunc>::add(const KeyT& key)
	{
		if (_population < capacity())
		{
			// search in hash map
			sint64 markedForRemovalIndexForReuse = NULL_INDEX;
			sint64 index = HashFunc::hash(key) & (L - 1);
			for (sint64 counter = 0; counter < L; counter += 32)
			{
				uint64 flags = _getEncodedOccupationFlags(_occupationFlags, index);
				for (auto i = 0; i < _nEncodedFlags; i++, flags >>= 2)
				{
					switch (flags & 3ULL)
					{
					case 0:
						// empty entry -> key isn't in set yet
						// If we have already seen an entry marked for removal, reuse this slot because it is closer to the hash index
						if (markedForRemovalIndexForReuse != NULL_INDEX)
							goto reuse_slot;
						// ... otherwise put element and mark as occupied
						_occupationFlags[index >> 5] |= (1ULL << ((index & 31) << 1));
						_keys[index] = key;
						_population++;
						return index;
					case 1:
						// used entry
						if (_keys[index] == key)
						{
							// found key -> return index
							return index;
						}
						break;
					case 2:
						// marked for removal -> reuse slot (first slot we see) later if we are sure that key isn't in the set
						if (markedForRemovalIndexForReuse == NULL_INDEX)
							markedForRemovalIndexForReuse = index;
						break;
					}
					index = (index + 1) & (L - 1);
				}
			}

			if (markedForRemovalIndexForReuse != NULL_INDEX)
			{
			reuse_slot:
				// Reuse slot marked for removal: put key here and set flags from 2 to 1.
				// But don't decrement _markRemovalCounter, because it is used to check if cleanup() is needed.
				// Without cleanup, we don't get new unoccupied slots and at least lookup of keys that aren't contained in the set
				// stays slow.
				index = markedForRemovalIndexForReuse;
				_occupationFlags[index >> 5] ^= (3ULL << ((index & 31) << 1));
				_keys[index] = key;
				_population++;
				return index;
			}
		}
		else // _population == capacity()
		{
			// Check if key exists.
			sint64 index = getElementIndex(key);
			if (index != NULL_INDEX)
			{
				return index;
			}
		}
		return NULL_INDEX;
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	bool HashSet<KeyT, L, HashFunc>::isEmptySlot(sint64 elementIndex) const
	{
		elementIndex &= (L - 1);
		uint64 flags = _getEncodedOccupationFlags(_occupationFlags, elementIndex);
		return ((flags & 3ULL) != 1);
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	sint64 HashSet<KeyT, L, HashFunc>::nextElementIndex(sint64 elementIndex) const
	{
		if (!_population)
			return NULL_INDEX;

		if (elementIndex < 0)
			elementIndex = 0;
		else
			++elementIndex;

		// search for next occupied element until end of hash map array
		constexpr uint64 flagsLength = math_lib::max(L >> 5, 1ull);
		sint64 flagsIdx = elementIndex >> 5;
		sint64 offset = elementIndex & 31ll;
		uint64 flags = _occupationFlags[flagsIdx] >> (2 * offset);
		while (flagsIdx < flagsLength)
		{
			for (sint64 i = offset; i < _nEncodedFlags; ++i, flags >>= 2)
			{
				if (!flags)
				{
					// no occupied entries in current flags
					break;
				}
				if ((flags & 3ULL) == 1)
				{
					// found occupied entry
					return (flagsIdx << 5) + i;
				}
			}

			flags = _occupationFlags[++flagsIdx];
			offset = 0;
		}

		return NULL_INDEX;
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	void HashSet<KeyT, L, HashFunc>::removeByIndex(sint64 elementIdx)
	{
		elementIdx &= (L - 1);
		uint64 flags = _getEncodedOccupationFlags(_occupationFlags, elementIdx);

		if ((flags & 3ULL) == 1)
		{
			_population--;
			_markRemovalCounter++;
			_occupationFlags[elementIdx >> 5] ^= (3ULL << ((elementIdx & 31) << 1));

			const bool CLEAR_UNUSED_ELEMENT = true;
			if (CLEAR_UNUSED_ELEMENT)
			{
				setMem(&_keys[elementIdx], sizeof(KeyT), 0);
			}
		}
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	sint64 HashSet<KeyT, L, HashFunc>::remove(const KeyT& key)
	{
		sint64 elementIndex = getElementIndex(key);
		if (elementIndex == NULL_INDEX)
		{
			return NULL_INDEX;
		}
		else
		{
			removeByIndex(elementIndex);
			return elementIndex;
		}
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	void HashSet<KeyT, L, HashFunc>::cleanupIfNeeded(uint64 removalThresholdPercent)
	{
		if (_markRemovalCounter > (removalThresholdPercent * L / 100))
		{
			cleanup();
		}
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	void HashSet<KeyT, L, HashFunc>::cleanup()
	{
		// _keys gets occupied over time with entries of type 3 which means they are marked for cleanup.
		// Once cleanup is called it's necessary to remove all these type 3 entries by reconstructing a fresh hash map residing in scratchpad buffer.
		// Cleanup() called for a hash map having only type 3 entries must give the result equal to reset() memory content wise.

		// If no elements have been removed, no cleanup is needed
		if (!_markRemovalCounter)
		{
			return;
		}

		// Speedup case of empty hash map with elements marked for removal
		if (!population())
		{
			reset();
			return;
		}

		// Init buffers
		auto* _keyBuffer = reinterpret_cast<KeyT*>(::__scratchpad(sizeof(_keys) + sizeof(_occupationFlags)));
		auto* _occupationFlagsBuffer = reinterpret_cast<uint64*>(_keyBuffer + L);
		auto* _stackBuffer = reinterpret_cast<sint64*>(
			_occupationFlagsBuffer + sizeof(_occupationFlags) / sizeof(_occupationFlags[0]));
		uint64 newPopulation = 0;

		// Go through hash map. For each element that is occupied but not marked for removal, insert element in new hash map's buffers.
		constexpr uint64 oldIndexGroupCount = (L >> 5) ? (L >> 5) : 1;
		for (sint64 oldIndexGroup = 0; oldIndexGroup < oldIndexGroupCount; oldIndexGroup++)
		{
			const uint64 flags = _occupationFlags[oldIndexGroup];
			uint64 maskBits = (0xAAAAAAAAAAAAAAAA & (flags << 1));
			maskBits &= maskBits ^ (flags & 0xAAAAAAAAAAAAAAAA);
			sint64 oldIndexOffset = _tzcnt_u64(maskBits) & 0xFE;
			const sint64 oldIndexOffsetEnd = 64 - (_lzcnt_u64(maskBits) & 0xFE);
			for (maskBits >>= oldIndexOffset;
				oldIndexOffset < oldIndexOffsetEnd; oldIndexOffset += 2, maskBits >>= 2)
			{
				// Only add elements to new hash map that are occupied and not marked for removal
				if (maskBits & 3ULL)
				{
					// find empty position in new hash map
					const sint64 oldIndex = (oldIndexGroup << 5) + (oldIndexOffset >> 1);
					sint64 newIndex = HashFunc::hash(_keys[oldIndex]) & (L - 1);
					for (sint64 counter = 0; counter < L; counter += 32)
					{
						QPI::uint64 newFlags = _getEncodedOccupationFlags(_occupationFlagsBuffer, newIndex);
						for (sint64 i = 0; i < _nEncodedFlags; i++, newFlags >>= 2)
						{
							if ((newFlags & 3ULL) == 0)
							{
								newIndex = (newIndex + i) & (L - 1);
								goto foundEmptyPosition;
							}
						}
						newIndex = (newIndex + _nEncodedFlags) & (L - 1);
					}
#ifdef NO_UEFI
					// should never be reached, because old and new map have same capacity (there should always be an empty slot)
					goto cleanupBug;
#endif

				foundEmptyPosition:
					// occupy empty hash map entry
					_occupationFlagsBuffer[newIndex >> 5] |= (1ULL << ((newIndex & 31) << 1));
					_keyBuffer[newIndex] = _keys[oldIndex];

					// check if we are done
					newPopulation += 1;
					if (newPopulation == _population)
					{
						// all elements have been transferred -> overwrite old array with new array
						copyMem(_keys, _keyBuffer, sizeof(_keys));
						copyMem(_occupationFlags, _occupationFlagsBuffer, sizeof(_occupationFlags));
						_markRemovalCounter = 0;
						return;
					}
				}
			}
		}

#ifdef NO_UEFI
		cleanupBug :
		// don't expect here, certainly got error!!!
		printf("ERROR: Something went wrong at cleanup!\n");
#endif
	}

	template <typename KeyT, uint64 L, typename HashFunc>
	void HashSet<KeyT, L, HashFunc>::reset()
	{
		setMem(this, sizeof(*this), 0);
	}
}

