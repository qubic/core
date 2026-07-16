#pragma once

#include "qpi_types.h"

namespace QPI
{
	// Array of L bits encoded in array of uint64 (overall size is at least 8 bytes, L must be 2^N)
	template <uint64 L>
	struct BitArray
	{
	private:
		static_assert(L && !(L& (L - 1)),
			"The capacity of the BitArray must be 2^N."
			);

		static constexpr uint64 _bits = L;
		static constexpr uint64 _elements = ((L + 63) / 64);

		uint64 _values[_elements];

	public:
		// Return number of bits
		static inline constexpr uint64 capacity()
		{
			return L;
		}

		// Return bit value at given index
		inline bit get(uint64 index) const
		{
			return (_values[(index >> 6) & (_elements - 1)] >> (index & 63)) & 1;
		}

		// Set bit with given index to the given value
		inline void set(uint64 index, bit value)
		{
			_values[(index >> 6) & (_elements - 1)] = (_values[(index >> 6) & (_elements - 1)] & (~(1ULL << (index & 63)))) | (((uint64)value) << (index & 63));
		}

		// Set content of bit array by copying memory (size must match)
		template <typename AT>
		inline void setMem(const AT& value)
		{
			static_assert(sizeof(_values) == sizeof(value), "This function can only be used if the overall size of both objects match.");
			// This if is resolved at compile time
			if (sizeof(_values) == 32)
			{
				// assignment uses __m256i intrinsic CPU functions which should be very fast
				*((id*)_values) = *((id*)&value);
			}
			else
			{
				// generic copying
				copyMemory(*this, value);
			}
		}

		// Set all bits to passed bit value
		inline void setAll(bit value)
		{
			uint64 setValue = (value) ? 0xffffffffffffffffllu : 0llu;
			for (uint64 i = 0; i < _elements; ++i)
				_values[i] = setValue;
		}


		bool operator==(const BitArray<L>& other) const
		{
			for (uint64 i = 0; i < _elements; ++i)
			{
				if (_values[i] != other._values[i])
				{
					return false;
				}
			}
			return true;
		}

		bool operator!=(const BitArray<L>& other) const
		{
			return !(*this == other);
		}

	};

	// Bit array convenience definitions
	typedef BitArray<2> bit_2;
	typedef BitArray<4> bit_4;
	typedef BitArray<8> bit_8;
	typedef BitArray<16> bit_16;
	typedef BitArray<32> bit_32;
	typedef BitArray<64> bit_64;
	typedef BitArray<128> bit_128;
	typedef BitArray<256> bit_256;
	typedef BitArray<512> bit_512;
	typedef BitArray<1024> bit_1024;
	typedef BitArray<2048> bit_2048;
	typedef BitArray<4096> bit_4096;

	constexpr bit_2 BIT2_ZERO = {};
	constexpr bit_4 BIT4_ZERO = {};
	constexpr bit_8 BIT8_ZERO = {};
	constexpr bit_16 BIT16_ZERO = {};
	constexpr bit_32 BIT32_ZERO = {};
	constexpr bit_64 BIT64_ZERO = {};
	constexpr bit_128 BIT128_ZERO = {};
	constexpr bit_256 BIT256_ZERO = {};
	constexpr bit_512 BIT512_ZERO = {};
	constexpr bit_1024 BIT1024_ZERO = {};
	constexpr bit_2048 BIT2048_ZERO = {};
	constexpr bit_4096 BIT4096_ZERO = {};


	// Array of L elements of type T (L must be 2^N)
	template <typename T, uint64 L>
	struct Array
	{
	private:
		static_assert(L && !(L& (L - 1)),
			"The capacity of the array must be 2^N."
			);

		T _values[L];

	public:
		// Return number of elements in array
		static inline constexpr uint64 capacity()
		{
			return L;
		}

		// Get element of array
		inline const T& get(uint64 index) const
		{
			return _values[index & (L - 1)];
		}

		// Set element of array
		inline void set(uint64 index, const T& value)
		{
			_values[index & (L - 1)] = value;
		}

		// Set content of array by copying memory (size must match)
		template <typename AT>
		inline void setMem(const AT& value)
		{
			static_assert(sizeof(_values) == sizeof(value), "This function can only be used if the overall size of both objects match.");
			// This if is resolved at compile time
			if (sizeof(_values) == 32)
			{
				// assignment uses __m256i intrinsic CPU functions which should be very fast
				*((id*)_values) = *((id*)&value);
			}
			else
			{
				// generic copying
				copyMemory(*this, value);
			}
		}

		// Set all elements to passed value
		inline void setAll(const T& value)
		{
			for (uint64 i = 0; i < L; ++i)
				_values[i] = value;
		}

		// Set elements in range to passed value
		inline void setRange(uint64 indexBegin, uint64 indexEnd, const T& value)
		{
			for (uint64 i = indexBegin; i < indexEnd; ++i)
				_values[i & (L - 1)] = value;
		}

		// Returns true if all elements of the range equal value (and range is valid).
		inline bool rangeEquals(uint64 indexBegin, uint64 indexEnd, const T& value) const
		{
			if (indexEnd > L || indexBegin > indexEnd)
				return false;
			for (uint64 i = indexBegin; i < indexEnd; ++i)
			{
				if (!(_values[i] == value))
					return false;
			}
			return true;
		}

		// Implement assignment operator to prevent generating call to unavailable memcpy()
		inline Array<T, L>& operator=(const Array<T, L>& other)
		{
			copyMemory(*this, other);
			return *this;
		}

		// Implement copy constructor to prevent generating call to unavailable memcpy()
		inline Array(const Array<T, L>& other)
		{
			copyMemory(*this, other);
		}

		Array() = default;
	};

	// Array convenience definitions
	typedef Array<sint8, 2> sint8_2;
	typedef Array<sint8, 4> sint8_4;
	typedef Array<sint8, 8> sint8_8;

	typedef Array<uint8, 2> uint8_2;
	typedef Array<uint8, 4> uint8_4;
	typedef Array<uint8, 8> uint8_8;

	typedef Array<sint16, 2> sint16_2;
	typedef Array<sint16, 4> sint16_4;
	typedef Array<sint16, 8> sint16_8;

	typedef Array<uint16, 2> uint16_2;
	typedef Array<uint16, 4> uint16_4;
	typedef Array<uint16, 8> uint16_8;

	typedef Array<sint32, 2> sint32_2;
	typedef Array<sint32, 4> sint32_4;
	typedef Array<sint32, 8> sint32_8;

	typedef Array<uint32, 2> uint32_2;
	typedef Array<uint32, 4> uint32_4;
	typedef Array<uint32, 8> uint32_8;

	typedef Array<sint64, 2> sint64_2;
	typedef Array<sint64, 4> sint64_4;
	typedef Array<sint64, 8> sint64_8;

	typedef Array<uint64, 2> uint64_2;
	typedef Array<uint64, 4> uint64_4;
	typedef Array<uint64, 8> uint64_8;

	typedef Array<id, 2> id_2;
	typedef Array<id, 8> id_4;
	typedef Array<id, 8> id_8;

	// Check if array is sorted in given range (duplicates allowed). Returns false if range is invalid.
	template <typename T, uint64 L>
	bool isArraySorted(const Array<T, L>& Array, uint64 beginIdx = 0, uint64 endIdx = L);

	// Check if array is sorted without duplicates in given range. Returns false if range is invalid.
	template <typename T, uint64 L>
	bool isArraySortedWithoutDuplicates(const Array<T, L>& Array, uint64 beginIdx = 0, uint64 endIdx = L);

	// Array of L elements of type T that is slower than normal Array but may have any capacity L.
	// This should be only used when a specific L != 2^N is needed for a good reason, e.g., in input/output.
	template <typename T, uint64 L>
	struct SlowAnySizeArray
	{
	private:
		static_assert(L, "The capacity of the array must be != 0.");

		T _values[L];

	public:
		// Return number of elements in array
		static inline constexpr uint64 capacity()
		{
			return L;
		}

		// Get element of array
		inline const T& get(uint64 index) const
		{
			return _values[index % L];
		}

		// Set element of array
		inline void set(uint64 index, const T& value)
		{
			_values[index % L] = value;
		}

		// Set all elements to passed value
		inline void setAll(const T& value)
		{
			for (uint64 i = 0; i < L; ++i)
				_values[i] = value;
		}

		// Implement assignment operator to prevent generating call to unavailable memcpy()
		inline SlowAnySizeArray<T, L>& operator=(const SlowAnySizeArray<T, L>& other)
		{
			copyMemory(*this, other);
			return *this;
		}

		// Implement copy constructor to prevent generating call to unavailable memcpy()
		inline SlowAnySizeArray(const SlowAnySizeArray<T, L>& other)
		{
			copyMemory(*this, other);
		}

		SlowAnySizeArray() = default;
	};

	// Hash function class to be used with the hash map.
	template <typename KeyT> class HashFunction
	{
	public:
		static uint64 hash(const KeyT& key);
	};

	// Hash map of (key, value) pairs of type (KeyT, ValueT) and total element capacity L. Access time is approx. constant
	// with population < 80% of L but gets close to linear with population > 90% of L.
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
		bool contains(const KeyT& key) const;

		// Return boolean indicating whether key is contained in the hash map.
		// If key is contained, write the associated value into the provided ValueT&. 
		bool get(const KeyT& key, ValueT& value) const;

		// Return index of element with key in hash map _elements, or NULL_INDEX if not found.
		sint64 getElementIndex(const KeyT& key) const;

		// Return if slot at elementIndex is empty (not occupied by an element). If false, key() is valid.
		inline bool isEmptySlot(sint64 elementIndex) const;

		// Return index of the next occupied element following the index passed as an argument. Pass NULL_INDEX to get
		// the first occupied element. Returns NULL_INDEX if there are no more occupied elements.
		inline sint64 nextElementIndex(sint64 elementIndex) const;

		// Return key at elementIndex. Invalid if isEmptySlot(elementIndex).
		inline const KeyT& key(sint64 elementIndex) const;

		// Return value at elementIndex.
		inline const ValueT& value(sint64 elementIndex) const;

		// Add element (key, value) to the hash map, return elementIndex of new element.
		// If key already exists in the hash map, the old value will be overwritten.
		// If the hash map is full, return NULL_INDEX.
		sint64 set(const KeyT& key, const ValueT& value);

		// Mark element for removal.
		void removeByIndex(sint64 elementIdx);

		// Mark element for removal if key is contained in the hash map, 
		// returning the elementIndex (or NULL_INDEX if the hash map does not contain the key).
		sint64 removeByKey(const KeyT& key);

		// Check if cleanup is needed based on the removal threshold, without modifying the container.
		bool needsCleanup(uint64 removalThresholdPercent = 50) const;

		// Call cleanup() if it makes sense. The content of this object may be reordered, so prior indices are invalidated.
		void cleanupIfNeeded(uint64 removalThresholdPercent = 50);

		// Remove all elements marked for removal. This is an expensive operation, but it improves lookup performance
		// if remove has been called often. Content is reordered, so prior indices are invalidated.
		void cleanup();

		// Replace value for *existing* key, do nothing otherwise.
		// - The key exists: replace its value. Return true.
		// - The key is not contained in the hash map: no action is taken. Return false.
		bool replace(const KeyT& key, const ValueT& newValue);

		// Reinitialize as empty hash map.
		void reset();
	};

	// Hash set of keys of type KeyT and total element capacity L. Access time is approx. constant with
	// population < 80% of L but gets close to linear with population > 90% of L.
	template <typename KeyT, uint64 L, typename HashFunc = HashFunction<KeyT>>
	class HashSet
	{
	private:
		static_assert(L && !(L& (L - 1)),
			"The capacity of the hash set must be 2^N."
			);
		static constexpr sint64 _nEncodedFlags = L > 32 ? 32 : L;

		// Hash set
		KeyT _keys[L];

		// 2 bits per element of _elements: 0b00 = not occupied; 0b01 = occupied; 0b10 = occupied but marked for removal; 0b11 is unused
		// The state "occupied but marked for removal" is needed for finding the index of a key in the hash map. Setting an entry to
		// "not occupied" in remove() would potentially undo a collision, create a gap, and mess up the entry search.
		uint64 _occupationFlags[(L * 2 + 63) / 64];

		uint64 _population;
		uint64 _markRemovalCounter;

		// Read and encode 32 POV occupation flags, return a 64bits number presents 32 occupation flags
		uint64 _getEncodedOccupationFlags(const uint64* occupationFlags, const sint64 elementIndex) const;

	public:
		HashSet()
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

		// Return boolean indicating whether key is contained in the hash set.
		bool contains(const KeyT& key) const;

		// Return index of element with key in hash set _keys, or NULL_INDEX if not found.
		sint64 getElementIndex(const KeyT& key) const;

		// Return if slot at elementIndex is empty (not occupied by an element). If false, key() is valid.
		inline bool isEmptySlot(sint64 elementIndex) const;

		// Return index of the next occupied element following the index passed as an argument. Pass NULL_INDEX to get
		// the first occupied element. Returns NULL_INDEX if there are no more occupied elements.
		inline sint64 nextElementIndex(sint64 elementIndex) const;

		// Return key at elementIndex. Invalid if isEmptySlot(elementIndex).
		inline KeyT key(sint64 elementIndex) const;

		// Add key to the hash set, return elementIndex of new element.
		// If key already exists in the hash set, this does nothing.
		// If the hash map is full, return NULL_INDEX.
		sint64 add(const KeyT& key);

		// Mark element for removal.
		void removeByIndex(sint64 elementIdx);

		// Mark element for removal if key is contained in the hash set, 
		// returning the elementIndex (or NULL_INDEX if the hash map does not contain the key).
		sint64 remove(const KeyT& key);

		// Check if cleanup is needed based on the removal threshold, without modifying the container.
		bool needsCleanup(uint64 removalThresholdPercent = 50) const;

		// Call cleanup() if it makes sense. The content of this object may be reordered, so prior indices are invalidated.
		void cleanupIfNeeded(uint64 removalThresholdPercent = 50);

		// Remove all elements marked for removal. This is an expensive operation, but it improves lookup performance
		// if remove has been called often. Content is reordered, so prior indices are invalidated.
		void cleanup();

		// Reinitialize as empty hash set.
		void reset();
	};


	// Collection of priority queues of elements with type T and total element capacity L.
	// Each ID pov (point of view) has an own queue.
	template <typename T, uint64 L>
	struct Collection
	{
	private:
		static_assert(L && !(L& (L - 1)),
			"The capacity of the Collection must be 2^N."
			);
		static constexpr sint64 _nEncodedFlags = L > 32 ? 32 : L;

		// Hash map of point of views = element filters, each with one priority queue (or empty)
		struct PoV
		{
			id value;
			uint64 population;
			sint64 headIndex, tailIndex;
			sint64 bstRootIndex;
		} _povs[L];

		// 2 bits per element of _povs: 0b00 = not occupied; 0b01 = occupied; 0b10 = occupied but marked for removal; 0b11 is unused
		// The state "occupied but marked for removal" is needed for finding the index of a pov in the hash map. Setting an entry to
		// "not occupied" in remove() would potentially undo a collision, create a gap, and mess up the entry search.
		uint64 _povOccupationFlags[(L * 2 + 63) / 64];

		// Array of elements (filled sequentially), each belongs to one PoV / priority queue (or is empty)
		// Elements of a POV entry will be stored as a binary search tree (BST); so this structure has some properties related to BST
		// (bstParentIndex, bstLeftIndex, bstRightIndex).
		struct Element
		{
			T value;
			sint64 priority;
			sint64 povIndex;
			sint64 bstParentIndex;
			sint64 bstLeftIndex;
			sint64 bstRightIndex;

			Element& init(const T& value, const sint64& priority, const sint64& povIndex)
			{
				this->value = value;
				this->priority = priority;
				this->povIndex = povIndex;
				this->bstParentIndex = NULL_INDEX;
				this->bstLeftIndex = NULL_INDEX;
				this->bstRightIndex = NULL_INDEX;
				return *this;
			}
		} _elements[L];
		uint64 _population;
		uint64 _markRemovalCounter;

		// Internal reinitialize as empty collection.
		void _softReset();

		// Return index of id pov in hash map _povs, or NULL_INDEX if not found
		sint64 _povIndex(const id& pov) const;

		// Return elementIndex of first element in priority queue of pov,
		// and ignore elements with priority greater than maxPriority
		sint64 _headIndex(const sint64 povIndex, const sint64 maxPriority) const;

		// Return elementIndex of last element in priority queue of pov,
		// and ignore elements with priority less than minPriority
		sint64 _tailIndex(const sint64 povIndex, const sint64 minPriority) const;

		// Return index of parent element to insert a priority
		sint64 _searchElement(const sint64 bstRootIndex,
			const sint64 priority, int* pIterationsCount = nullptr) const;

		// Add element to priority queue, return elementIndex of new element
		sint64 _addPovElement(const sint64 povIndex, const T value, const sint64 priority);

		// Get element indices and store them in an array, return number of elements
		uint64 _getSortedElements(const sint64 rootIdx, sint64* sortedElementIndices) const;

		// Fill a sint64_4 vector with specified values
		inline void _set(sint64_4& vec, sint64 v0, sint64 v1, sint64 v2, sint64 v3) const;

		// Rebuild pov's elements indexing as balanced BST
		sint64 _rebuild(sint64 rootIdx);

		// Return most left element index
		sint64 _getMostLeft(sint64 elementIdx) const;

		// Return most right element index
		sint64 _getMostRight(sint64 elementIdx) const;

		// Return elementIndex of previous element in priority queue (or NULL_INDEX if this is the last element).
		sint64 _previousElementIndex(sint64 elementIdx) const;

		// Return elementIndex of next element in priority queue (or NULL_INDEX if this is the last element).
		sint64 _nextElementIndex(sint64 elementIdx) const;

		// Update parent of the current element into parent of the new element, return true if exists parent
		inline bool _updateParent(const sint64 elementIdx, const sint64 newElementIdx);

		// Move the current element into new position
		void _moveElement(const sint64 srcIdx, const sint64 dstIdx);

		// Read and encode 32 POV occupation flags, return a 64bits number presents 32 occupation flags
		uint64 _getEncodedPovOccupationFlags(const uint64* povOccupationFlags, const sint64 povIndex) const;;

	public:
		// Add element to priority queue of ID pov, return elementIndex of new element
		sint64 add(const id& pov, T element, sint64 priority);

		// Return maximum number of elements that may be stored.
		static constexpr uint64 capacity()
		{
			return L;
		}

		// Check if cleanup is needed based on the removal threshold, without modifying the collection.
		bool needsCleanup(uint64 removalThresholdPercent = 50) const;

		// Call cleanup() if more than the given percent of pov slots are marked for removal.
		void cleanupIfNeeded(uint64 removalThresholdPercent = 50);

		// Remove all povs marked for removal, this is a very expensive operation, but it improves lookup performance
		// if remove has been called often. Content is reordered, so prior indices are invalidated.
		void cleanup();

		// Return element value at elementIndex.
		inline T element(sint64 elementIndex) const;

		// Return elementIndex of first element in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 headIndex(const id& pov) const;

		// Return elementIndex of first element with priority <= maxPriority in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 headIndex(const id& pov, sint64 maxPriority) const;

		// Return elementIndex of next element in priority queue (or NULL_INDEX if this is the last element).
		sint64 nextElementIndex(sint64 elementIndex) const;

		// Return overall number of elements.
		inline uint64 population() const;

		// Return number of elements of specific PoV.
		uint64 population(const id& pov) const;

		// Return point of view elementIndex belongs to (or 0 id if unused).
		id pov(sint64 elementIndex) const;

		// Return elementIndex of previous element in priority queue (or NULL_INDEX if this is the last element).
		sint64 prevElementIndex(sint64 elementIndex) const;

		// Return priority of elementIndex (or 0 id if unused).
		sint64 priority(sint64 elementIndex) const;

		// Remove element and mark its pov for removal, if the last element.
		// Returns element index of next element in priority queue (the one following elementIdx).
		// Element indices obtained before this call are invalidated, because at least one element is moved.
		sint64 remove(sint64 elementIdx);

		// Replace *existing* element, do nothing otherwise.
		// - The element exists: replace its value.
		// - The index is out of bounds: no action is taken.
		void replace(sint64 oldElementIndex, const T& newElement);

		// Reinitialize as empty collection.
		void reset();

		// Return elementIndex of last element in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 tailIndex(const id& pov) const;

		// Return elementIndex of last element with priority >= minPriority in priority queue of pov (or NULL_INDEX if pov is unknown).
		sint64 tailIndex(const id& pov, sint64 minPriority) const;
	};


	// Doubly-linked list of elements of type T with fixed capacity L.
	// Provides O(1) insertion at head/tail, O(1) insertion before/after a given index,
	// O(1) removal by index, and bidirectional traversal.
	// Removed nodes are immediately recycled via a free list (no deferred cleanup needed).
	template <typename T, uint64 L>
	class LinkedList
	{
	private:
		static_assert(L && !(L& (L - 1)),
			"The capacity of the LinkedList must be 2^N."
			);

		struct Node
		{
			T value;
			sint64 nextIndex;  // Next in data list, or next in free list when freed
			sint64 prevIndex;  // Previous in data list (undefined when freed)
		} _nodes[L];

		// 1 bit per node: 1 = occupied, 0 = free
		uint64 _occupiedFlags[(L + 63) / 64];

		sint64 _headIndex;      // First element in the list (NULL_INDEX if empty)
		sint64 _tailIndex;      // Last element in the list (NULL_INDEX if empty)
		sint64 _freeHeadIndex;  // Head of recycled-nodes free list (NULL_INDEX if none)
		uint64 _nextUnusedIndex; // Next never-used node index (lazy free list init)
		uint64 _population;

		// Check if elementIndex is in range [0, L) and the slot is occupied.
		bool _isValidAndOccupied(sint64 elementIndex) const;

		// Initialize sentinel values if the list has never been used (handles zero-initialized contract state).
		void _initIfNeeded();

		// Get a node from the free list or unused pool. Returns NULL_INDEX if full.
		sint64 _allocateNode();

		// Return node to free list.
		void _freeNode(sint64 nodeIndex);

	public:
		// Return maximum number of elements that may be stored.
		static constexpr uint64 capacity()
		{
			return L;
		}

		// Return current number of elements.
		inline uint64 population() const;

		// Return elementIndex of first element in the list (or NULL_INDEX if empty).
		inline sint64 headIndex() const;

		// Return elementIndex of last element in the list (or NULL_INDEX if empty).
		inline sint64 tailIndex() const;

		// Return elementIndex of next element (or NULL_INDEX if this is the last element).
		sint64 nextElementIndex(sint64 elementIndex) const;

		// Return elementIndex of previous element (or NULL_INDEX if this is the first element).
		sint64 prevElementIndex(sint64 elementIndex) const;

		// Return element value at elementIndex.
		inline const T& element(sint64 elementIndex) const;

		// Return true if the slot at elementIndex is empty (not occupied by an element).
		// Out-of-range indices are considered empty.
		inline bool isEmptySlot(sint64 elementIndex) const;

		// Add element at the head of the list, return elementIndex of new element (or NULL_INDEX if full).
		sint64 addHead(const T& value);

		// Add element at the tail of the list, return elementIndex of new element (or NULL_INDEX if full).
		sint64 addTail(const T& value);

		// Insert element after the element at elementIndex.
		// Returns elementIndex of new element (or NULL_INDEX if full or elementIndex is invalid).
		sint64 insertAfter(sint64 elementIndex, const T& value);

		// Insert element before the element at elementIndex.
		// Returns elementIndex of new element (or NULL_INDEX if full or elementIndex is invalid).
		sint64 insertBefore(sint64 elementIndex, const T& value);

		// Remove element at elementIndex. The node is immediately returned to the free list.
		void remove(sint64 elementIndex);

		// Replace the value at elementIndex with newValue.
		// Returns true if elementIndex was valid and occupied, false otherwise.
		bool replace(sint64 elementIndex, const T& newValue);

		// Reinitialize as empty linked list.
		void reset();
	};
}
