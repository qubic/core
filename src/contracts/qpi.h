// Qubic Programming Interface 1.0.0

#pragma once

// m256i is used for the id data type
#include "../platform/m256.h"

// uint128
#include "../platform/uint128.h"

// ASSERT can be used to support debugging and speed-up development
#include "../platform/assert.h"

namespace QPI
{
	/*

	Prohibited character combinations in contracts:

	"
	#
	%
	'
	* (not prohibited as multiplication operator)
	...
	/ as division operator
	:: (not prohibited as scope operator for structs, enums, and namespaces defined in contracts and `qpi.h`)
	[
	]
	__
	double
	float
	typedef
	union

	const_cast
	QpiContext
	TODO: prevent other casts trying to cast const away from state

	*/

	typedef bool bit;
	typedef signed char sint8;
	typedef unsigned char uint8;
	typedef signed short sint16;
	typedef unsigned short uint16;
	typedef signed int sint32;
	typedef unsigned int uint32;
	typedef signed long long sint64;
	typedef unsigned long long uint64;

	typedef uint128_t uint128;
	typedef m256i id;

#define STATIC_ASSERT(condition, identifier) static_assert(condition, #identifier);

#define NULL_ID id::zero()

	constexpr sint64 NULL_INDEX = -1;

	constexpr sint64 INVALID_AMOUNT = 0x8000000000000000;

	constexpr long long _A = 0;
	constexpr long long _B = 1;
	constexpr long long _C = 2;
	constexpr long long _D = 3;
	constexpr long long _E = 4;
	constexpr long long _F = 5;
	constexpr long long _G = 6;
	constexpr long long _H = 7;
	constexpr long long _I = 8;
	constexpr long long _J = 9;
	constexpr long long _K = 10;
	constexpr long long _L = 11;
	constexpr long long _M = 12;
	constexpr long long _N = 13;
	constexpr long long _O = 14;
	constexpr long long _P = 15;
	constexpr long long _Q = 16;
	constexpr long long _R = 17;
	constexpr long long _S = 18;
	constexpr long long _T = 19;
	constexpr long long _U = 20;
	constexpr long long _V = 21;
	constexpr long long _W = 22;
	constexpr long long _X = 23;
	constexpr long long _Y = 24;
	constexpr long long _Z = 25;

	inline id ID(long long _00, long long _01, long long _02, long long _03, long long _04, long long _05, long long _06, long long _07, long long _08, long long _09,
		long long _10, long long _11, long long _12, long long _13, long long _14, long long _15, long long _16, long long _17, long long _18, long long _19,
		long long _20, long long _21, long long _22, long long _23, long long _24, long long _25, long long _26, long long _27, long long _28, long long _29,
		long long _30, long long _31, long long _32, long long _33, long long _34, long long _35, long long _36, long long _37, long long _38, long long _39,
		long long _40, long long _41, long long _42, long long _43, long long _44, long long _45, long long _46, long long _47, long long _48, long long _49,
		long long _50, long long _51, long long _52, long long _53, long long _54, long long _55)
	{ 
		return _mm256_set_epi64x(((((((((((((((uint64)_55) * 26 + _54) * 26 + _53) * 26 + _52) * 26 + _51) * 26 + _50) * 26 + _49) * 26 + _48) * 26 + _47) * 26 + _46) * 26 + _45) * 26 + _44) * 26 + _43) * 26 + _42, ((((((((((((((uint64)_41) * 26 + _40) * 26 + _39) * 26 + _38) * 26 + _37) * 26 + _36) * 26 + _35) * 26 + _34) * 26 + _33) * 26 + _32) * 26 + _31) * 26 + _30) * 26 + _29) * 26 + _28, ((((((((((((((uint64)_27) * 26 + _26) * 26 + _25) * 26 + _24) * 26 + _23) * 26 + _22) * 26 + _21) * 26 + _20) * 26 + _19) * 26 + _18) * 26 + _17) * 26 + _16) * 26 + _15) * 26 + _14, ((((((((((((((uint64)_13) * 26 + _12) * 26 + _11) * 26 + _10) * 26 + _09) * 26 + _08) * 26 + _07) * 26 + _06) * 26 + _05) * 26 + _04) * 26 + _03) * 26 + _02) * 26 + _01) * 26 + _00); 
	}

#define NUMBER_OF_COMPUTORS 676
#define QUORUM (NUMBER_OF_COMPUTORS * 2 / 3 + 1)

    constexpr int JANUARY = 1;
    constexpr int FEBRUARY = 2;
    constexpr int MARCH = 3;
    constexpr int APRIL = 4;
    constexpr int MAY = 5;
    constexpr int JUNE = 6;
    constexpr int JULY = 7;
    constexpr int AUGUST = 8;
    constexpr int SEPTEMBER = 9;
    constexpr int OCTOBER = 10;
    constexpr int NOVEMBER = 11;
    constexpr int DECEMBER = 12;

    constexpr int WEDNESDAY = 0;
    constexpr int THURSDAY = 1;
    constexpr int FRIDAY = 2;
    constexpr int SATURDAY = 3;
    constexpr int SUNDAY = 4;
    constexpr int MONDAY = 5;
    constexpr int TUESDAY = 6;

	constexpr unsigned long long X_MULTIPLIER = 1ULL;

	// Copy memory of src to dst. Both may have different types, but size of both must match exactly.
	template <typename T1, typename T2>
	inline void copyMemory(T1& dst, const T2& src);

	// Set all memory of dst to byte value.
	template <typename T>
	inline void setMemory(T& dst, uint8 value);

	struct DateAndTime
	{
		// --- Member Variables ---
		unsigned short millisecond;
		unsigned char second;
		unsigned char minute;
		unsigned char hour;
		unsigned char day;
		unsigned char month;
		unsigned char year;

		// --- Public Member Operators ---

		/**
		 * @brief Checks if this date is earlier than the 'other' date.
		 */
		bool operator<(const DateAndTime& other) const
		{
			if (year != other.year) return year < other.year;
			if (month != other.month) return month < other.month;
			if (day != other.day) return day < other.day;
			if (hour != other.hour) return hour < other.hour;
			if (minute != other.minute) return minute < other.minute;
			if (second != other.second) return second < other.second;
			return millisecond < other.millisecond;
		}

		/**
		 * @brief Checks if this date is later than the 'other' date.
		 */
		bool operator>(const DateAndTime& other) const
		{
			return other < *this; // Reuses the operator< on the 'other' object
		}

		/**
		 * @brief Checks if this date is identical to the 'other' date.
		 */
		bool operator==(const DateAndTime& other) const
		{
			return year == other.year &&
				month == other.month &&
				day == other.day &&
				hour == other.hour &&
				minute == other.minute &&
				second == other.second &&
				millisecond == other.millisecond;
		}

		/**
		 * @brief Computes the difference between this date and 'other' in milliseconds.
		 */
		long long operator-(const DateAndTime& other) const
		{
			// A member function can access private members of other instances of the same class.
			return this->toMilliseconds() - other.toMilliseconds();
		}

		/**
		 * @brief Adds a duration in milliseconds to the current date/time.
		 * @param msToAdd The number of milliseconds to add. Can be negative.
		 * @return A new DateAndTime object representing the result.
		 */
		DateAndTime operator+(long long msToAdd) const
		{
			long long totalMs = this->toMilliseconds() + msToAdd;

			DateAndTime result = { 0,0,0,0,0,0,0 };

			// Handle negative totalMs (dates before the epoch) if necessary
			// For this implementation, we assume resulting dates are >= year 2000
			if (totalMs < 0) totalMs = 0;

			long long days = totalMs / 86400000LL;
			long long msInDay = totalMs % 86400000LL;

			// Calculate time part
			result.hour = (unsigned char)(msInDay / 3600000LL);
			msInDay %= 3600000LL;
			result.minute = (unsigned char)(msInDay / 60000LL);
			msInDay %= 60000LL;
			result.second = (unsigned char)(msInDay / 1000LL);
			result.millisecond = (unsigned short)(msInDay % 1000LL);

			// Calculate date part from total days since epoch
			unsigned char currentYear = 0;
			while (true)
			{
				long long daysThisYear = isLeap(currentYear) ? 366 : 365;
				if (days >= daysThisYear)
				{
					days -= daysThisYear;
					currentYear++;
				}
				else
				{
					break;
				}
			}
			result.year = currentYear;

			unsigned char currentMonth = 1;
			const int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			while (true)
			{
				long long daysThisMonth = daysInMonth[currentMonth];
				if (currentMonth == 2 && isLeap(result.year))
				{
					daysThisMonth = 29;
				}
				if (days >= daysThisMonth)
				{
					days -= daysThisMonth;
					currentMonth++;
				}
				else
				{
					break;
				}
			}
			ASSERT(days <= 31);
			result.month = currentMonth;
			result.day = (unsigned char)(days) + 1; // days is 0-indexed, day is 1-indexed

			return result;
		}

		DateAndTime& operator+=(long long msToAdd)
		{
			*this = *this + msToAdd; // Reuse operator+ and assign the result back to this object
			return *this;
		}

		DateAndTime& operator-=(long long msToSubtract)
		{
			*this = *this + (-msToSubtract); // Reuse operator+ with a negative value
			return *this;
		}

	private:
		// --- Private Helper Functions ---

		/**
		 * @brief A static helper to check if a year (yy format) is a leap year.
		 */
		static bool isLeap(unsigned char yr) {
			// here we only handle the case where yr is in range [00 to 99]
			return (2000 + yr) % 4 == 0;
		}

		/**
		 * @brief Helper to convert this specific DateAndTime instance to total milliseconds since Jan 1, 2000.
		 */
		long long toMilliseconds() const {
			long long totalDays = 0;

			// Add days for full years passed since 2000
			for (unsigned char y = 0; y < year; ++y) {
				totalDays += isLeap(y) ? 366 : 365;
			}

			// Add days for full months passed in the current year
			const int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			for (unsigned char m = 1; m < month; ++m) {
				totalDays += daysInMonth[m];
				if (m == 2 && isLeap(year)) {
					totalDays += 1;
				}
			}

			// Add days in the current month
			totalDays += day - 1;

			// Convert total days and the time part to milliseconds
			long long totalMs = totalDays * 86400000LL; // 24 * 60 * 60 * 1000
			totalMs += hour * 3600000LL;     // 60 * 60 * 1000
			totalMs += minute * 60000LL;       // 60 * 1000
			totalMs += second * 1000LL;
			totalMs += millisecond;

			return totalMs;
		}
	};

	// Array of L bits encoded in array of uint64 (overall size is at least 8 bytes, L must be 2^N)
	template <uint64 L>
	struct BitArray
	{
	private:
		static_assert(L && !(L & (L - 1)),
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


	// Array of L elements of type T (L must be 2^N)
	template <typename T, uint64 L>
	struct Array
	{
	private:
		static_assert(L && !(L & (L - 1)),
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
		static_assert(L && !(L & (L - 1)),
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

	//////////
	// safety multiplying a and b and then clamp
	
	inline static sint64 smul(sint64 a, sint64 b)
	{
		sint64 hi, lo;
		lo = _mul128(a, b, &hi);
		if (hi != (lo >> 63))
		{
			return ((a > 0) == (b > 0)) ? INT64_MAX : INT64_MIN;
		}
		return lo;
	}

	inline static uint64 smul(uint64 a, uint64 b)
	{
		uint64 hi, lo;
		lo = _umul128(a, b, &hi);
		if (hi != 0)
		{
			return UINT64_MAX;
		}
		return lo;
	}

	inline static sint32 smul(sint32 a, sint32 b)
	{
		sint64 r = (sint64)(a) * (sint64)(b);
		if (r < INT32_MIN)
		{
			return INT32_MIN;
		}
		else if (r > INT32_MAX)
		{
			return INT32_MAX;
		}
		else
		{
			return (sint32)r;
		}
	}

	inline static uint32 smul(uint32 a, uint32 b)
	{
		uint64 r = (uint64)(a) * (uint64)(b);
		if (r > UINT32_MAX)
		{
			return UINT32_MAX;
		}
		return (uint32)r;
	}

	//////////
	// safety adding a and b and then clamp

	inline static sint64 sadd(sint64 a, sint64 b)
	{
		sint64 sum = a + b;
		if (a < 0 && b < 0 && sum > 0) // negative overflow
			return INT64_MIN;
		if (a > 0 && b > 0 && sum < 0) // positive overflow
			return INT64_MAX;
		return sum;
	}

	inline static uint64 sadd(uint64 a, uint64 b)
	{
		if (UINT64_MAX - a < b)
			return UINT64_MAX;
		return a + b;
	}

	inline static sint32 sadd(sint32 a, sint32 b)
	{
		sint64 sum = (sint64)(a) + (sint64)(b);
		if (sum < INT32_MIN)
		{
			return INT32_MIN;
		}
		else if (sum > INT32_MAX)
		{
			return INT32_MAX;
		}
		else
		{
			return (sint32)sum;
		}
	}

	inline static uint32 sadd(uint32 a, uint32 b)
	{
		uint64 sum = (uint64)(a) + (uint64)(b);
		if (sum > UINT32_MAX)
		{
			return UINT32_MAX;
		}
		return (uint32)sum;
	}

	// Divide a by b, but return 0 if b is 0 (rounding to lower magnitude in case of integers)
	template <typename T>
	inline static constexpr T div(T a, T b)
	{
		return b ? (a / b) : T(0);
	}

	// Return remainder of dividing a by b, but return 0 if b is 0 (requires modulo % operator)
	template <typename T>
	inline static constexpr T mod(T a, T b)
	{
		return b ? (a % b) : 0;
	}

	//////////

	struct Entity
	{
		id publicKey;
		sint64 incomingAmount, outgoingAmount;

		// Numbers of transfers. These may overflow for entities with high traffic, such as Qx.
		uint32 numberOfIncomingTransfers, numberOfOutgoingTransfers;

		uint32 latestIncomingTransferTick, latestOutgoingTransferTick;
	};

	//////////


	struct Asset
	{
		id issuer;
		uint64 assetName;
	};

	struct AssetIssuanceSelect : public Asset
	{
		bool anyIssuer;
		bool anyName;

		inline static AssetIssuanceSelect any()
		{
			return { id::zero(), 0, true, true };
		}

		inline static AssetIssuanceSelect byIssuer(const id& owner)
		{
			return { owner, 0, false, true };
		}

		inline static AssetIssuanceSelect byName(uint64 assetName)
		{
			return { m256i::zero(), assetName, true, false };
		}
	};

	struct AssetOwnershipSelect
	{
		id owner;
		uint16 managingContract;
		bool anyOwner;
		bool anyManagingContract;

		inline static AssetOwnershipSelect any()
		{
			return { id::zero(), 0, true, true };
		}

		inline static AssetOwnershipSelect byOwner(const id& owner)
		{
			return { owner, 0, false, true };
		}

		inline static AssetOwnershipSelect byManagingContract(uint16 managingContract)
		{
			return { m256i::zero(), managingContract, true, false };
		}
	};

	struct AssetPossessionSelect
	{
		id possessor;
		uint16 managingContract;
		bool anyPossessor;
		bool anyManagingContract;

		inline static AssetPossessionSelect any()
		{
			return { id::zero(), 0, true, true };
		}

		inline static AssetPossessionSelect byPossessor(const id& possessor)
		{
			return { possessor, 0, false, true };
		}

		inline static AssetPossessionSelect byManagingContract(uint16 managingContract)
		{
			return { m256i::zero(), managingContract, true, false };
		}
	};

	// Iterator for asset issuance records.
	// CAUTION CORE DEVS: DOES NOT TAKE CARE FOR LOCKING! (not relevant for contract devs)
	class AssetIssuanceIterator
	{
	protected:
		AssetIssuanceSelect _issuance;
		unsigned int _issuanceIdx;

	public:
		AssetIssuanceIterator(const AssetIssuanceSelect& issuance = AssetIssuanceSelect::any())
		{
			begin(issuance);
		}

		// Start iteration with issuance filter (selects first record).
		inline void begin(const AssetIssuanceSelect& issuance);

		// Return if iteration with next() has reached end.
		inline bool reachedEnd() const;

		// Step to next issuance record matching filtering criteria.
		inline bool next();

		// Issuer of current record
		inline id issuer() const;

		// Asset name of current record
		inline uint64 assetName() const;

		// Return asset (pair of issuer and asset name)
		inline Asset asset() const
		{
			return Asset{issuer(), assetName()};
		}

		// Index of issuance in universe. Should not be used by contracts, because it may change between contract calls.
		// Changed by next(). NO_ASSET_INDEX if issuance has not been found.
		inline unsigned int issuanceIndex() const
		{
			return _issuanceIdx;
		}
	};

	// Iterator for ownership records of specific issuance also providing filtering options.
	// CAUTION CORE DEVS: DOES NOT TAKE CARE OF LOCKING! (not relevant for contract devs)
	class AssetOwnershipIterator
	{
	protected:
		Asset _issuance;
		unsigned int _issuanceIdx;
		AssetOwnershipSelect _ownership;
		unsigned int _ownershipIdx;

		// Constructor for derived classes, which should call begin() themselves.
		AssetOwnershipIterator()
		{
		}

	public:
		AssetOwnershipIterator(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any())
		{
			begin(issuance, ownership);
		}

		// Start iteration with given issuance and given ownership filter (selects first record).
		inline void begin(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any());

		// Return if iteration with next() has reached end.
		inline bool reachedEnd() const;

		// Step to next ownership record matching filtering criteria.
		inline bool next();

		// Issuer of current record
		inline id issuer() const;

		// Asset name of current record
		inline uint64 assetName() const;

		// Owner of current record
		inline id owner() const;

		// Number of shares in current ownership record
		inline sint64 numberOfOwnedShares() const;

		// Contract index of contract having management rights (can transfer ownership)
		inline uint16 ownershipManagingContract() const;

		// Index of issuance in universe. Should not be used by contracts, because it may change between contract calls.
		// Constant not changed by next(). NO_ASSET_INDEX if issuance has not been found.
		inline unsigned int issuanceIndex() const
		{
			return _issuanceIdx;
		}

		// Index of ownership in universe. Should not be used by contracts, because it may change between contract calls.
		// Changed by next(). NO_ASSET_INDEX if no (more) matching ownership has not been found.
		inline unsigned int ownershipIndex() const
		{
			return _ownershipIdx;
		}
	};

	// Iterator for possession records of specific issuance also providing filtering options.
	// CAUTION CORE DEVS: DOES NOT TAKE CARE OF LOCKING! (not relevant for contract devs)
	class AssetPossessionIterator : public AssetOwnershipIterator
	{
	protected:
		AssetPossessionSelect _possession;
		unsigned int _possessionIdx;

	public:
		AssetPossessionIterator(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(), const AssetPossessionSelect& possession = AssetPossessionSelect::any())
		{
			begin(issuance, ownership, possession);
		}

		// Start iteration with given issuance and given ownership + possession filters (selects first record).
		inline void begin(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(), const AssetPossessionSelect& possession = AssetPossessionSelect::any());

		// Return if iteration with next() has reached end.
		inline bool reachedEnd() const;

		// Step to next possession record matching filtering criteria.
		inline bool next();

		// Owner of current record
		inline id possessor() const;

		// Number of shares in current possession record
		inline sint64 numberOfPossessedShares() const;

		// Index of possession record in universe. Should not be used by contracts, because it may change between contract calls.
		// Changed by next(). NO_ASSET_INDEX if no (more) matching ownership has not been found.
		inline unsigned int possessionIndex() const
		{
			return _possessionIdx;
		}

		// Contract index of contract having management rights (can transfer possession)
		inline uint16 possessionManagingContract() const;
	};

	//////////
	
	constexpr uint16 INVALID_PROPOSAL_INDEX = 0xffff;
	constexpr uint32 INVALID_VOTER_INDEX = 0xffffffff;
	constexpr sint64 NO_VOTE_VALUE = 0x8000000000000000;

	// Single vote for all types of proposals defined in August 2024.
	// Input data for contract procedure call
	struct ProposalSingleVoteDataV1
	{
		// Index of proposal the vote is about (can be requested with proposal voting API)
		uint16 proposalIndex;

		// Type of proposal, see ProposalTypes
		uint16 proposalType;

		// Tick when proposal has been set (to make sure that proposal version known by the voter matches the current version).
		uint32 proposalTick;

		// Value of vote. NO_VOTE_VALUE means no vote for every type.
		// For proposals types with multiple options, 0 is no, 1 to N are the other options in order of definition in proposal.
		// For scalar proposal types the value is passed directly.
		sint64 voteValue;
	};
	static_assert(sizeof(ProposalSingleVoteDataV1) == 16, "Unexpected struct size.");

	// Voting result summary for all types of proposals defined in August 2024.
	// Output data for contract function call for getting voting results.
	struct ProposalSummarizedVotingDataV1
	{
		// Index of proposal the vote is about (can be requested with proposal voting API)
		uint16 proposalIndex;

		// Count of options in proposal type (number of valid elements in optionVoteCount, 0 for scalar voting)
		uint16 optionCount;

		// Tick when proposal has been set (useful for checking if cached ProposalData is still up to date).
		uint32 proposalTick;

		// Number of voter who have the right to vote
		uint32 authorizedVoters;

		// Number of total votes casted
		uint32 totalVotes;

		// Voting results
		union
		{
			// Number of votes for different options (0 = no change, 1 to N = yes to specific proposed value)
			Array<uint32, 8> optionVoteCount;

			// Scalar voting result (currently only for proposalType VariableScalarMean, mean value of all valid votes)
			sint64 scalarVotingResult;
		};

		ProposalSummarizedVotingDataV1() = default;
		ProposalSummarizedVotingDataV1(const ProposalSummarizedVotingDataV1& src)
		{
			copyMemory(*this, src);
		}
	};
	static_assert(sizeof(ProposalSummarizedVotingDataV1) == 16 + 8*4, "Unexpected struct size.");

	// Proposal type constants and functions.
	// Each proposal type is composed of a class and a number of options. As an alternative to having N options (option votes),
	// some proposal classes (currently the one to set a variable) may allow to vote with a scalar value in a range defined
	// by the proposal (scalar voting).
	namespace ProposalTypes
	{
		// Class of proposal type
		namespace Class
		{
			// Options without extra data. Supported options: 2 <= N <= 8 with ProposalDataV1.
			static constexpr uint16 GeneralOptions = 0;

			// Propose to transfer amount to address. Supported options: 2 <= N <= 5 with ProposalDataV1.
			static constexpr uint16 Transfer = 0x100;

			// Propose to set variable to a value. Supported options: 2 <= N <= 5 with ProposalDataV1; N == 0 means scalar voting.
			static constexpr uint16 Variable = 0x200;

			// Propose to transfer amount to address in a specific epoch. Supported options: 1 with ProposalDataV1.
			static constexpr uint16 TransferInEpoch = 0x400;
		};

		// Options yes and no without extra data -> result is histogram of options
		static constexpr uint16 YesNo = Class::GeneralOptions | 2;

		// 3 options without extra data -> result is histogram of options
		static constexpr uint16 ThreeOptions = Class::GeneralOptions | 3;

		// 3 options without extra data -> result is histogram of options
		static constexpr uint16 FourOptions = Class::GeneralOptions | 4;

		// Transfer given amount to address with options yes/no
		static constexpr uint16 TransferYesNo = Class::Transfer | 2;

		// Transfer amount to address with two options of amounts and option "no change"
		static constexpr uint16 TransferTwoAmounts = Class::Transfer | 3;

		// Transfer amount to address with three options of amounts and option "no change"
		static constexpr uint16 TransferThreeAmounts = Class::Transfer | 4;

		// Transfer amount to address with four options of amounts and option "no change"
		static constexpr uint16 TransferFourAmounts = Class::Transfer | 5;

		// Transfer given amount to address in a specific epoch, with options yes/no
		static constexpr uint16 TransferInEpochYesNo = Class::TransferInEpoch | 2;

		// Set given variable to proposed value with options yes/no
		static constexpr uint16 VariableYesNo = Class::Variable | 2;

		// Set given variable to proposed value with two options of values and option "no change"
		static constexpr uint16 VariableTwoValues = Class::Variable | 3;

		// Set given variable to proposed value with three options of values and option "no change"
		static constexpr uint16 VariableThreeValues = Class::Variable | 4;

		// Set given variable to proposed value with four options of values and option "no change"
		static constexpr uint16 VariableFourValues = Class::Variable | 5;

		// Set given variable to value, allowing to vote with scalar value, voting result is mean value
		static constexpr uint16 VariableScalarMean = Class::Variable | 0;


		// Contruct type from class + number of options (no checking if type is valid)
		static constexpr uint16 type(uint16 cls, uint16 options)
		{
			return cls | options;
		}

		// Return option count for a given proposal type (including "no change" option),
		// 0 for scalar voting (no checking if type is valid).
		static uint16 optionCount(uint16 proposalType)
		{
			return proposalType & 0x00ff;
		}

		// Return class of proposal type (no checking if type is valid).
		static uint16 cls(uint16 proposalType)
		{
			return proposalType & 0xff00;
		}

		// Check if given type is valid (supported by most comprehensive ProposalData class).
		inline static bool isValid(uint16 proposalType);
	};

	// Proposal data struct for all types of proposals defined in August 2024 and revised in June 2025.
	// Input data for contract procedure call, usable as ProposalDataType in ProposalVoting (persisted in contract states).
	// You have to choose, whether to support scalar votes next to option votes. Scalar votes require 8x more storage in the state.
	template <bool SupportScalarVotes>
	struct ProposalDataV1
	{
		// URL explaining proposal, zero-terminated string.
		Array<uint8, 256> url;	
		
		// Epoch, when proposal is active. For setProposal(), 0 means to clear proposal and non-zero means the current epoch.
		uint16 epoch;

		// Type of proposal, see ProposalTypes.
		uint16 type;

		// Tick when proposal has been set. Output only, overwritten in setProposal().
		uint32 tick;

		// Proposal payload data (for all except types with class GeneralProposal)
		union
		{
			// Used if type class is Transfer
			struct Transfer
			{
				id destination;
				Array<sint64, 4> amounts;   // N first amounts are the proposed options (non-negative, sorted without duplicates), rest zero
			} transfer;

			// Used if type class is TransferInEpoch
			struct TransferInEpoch
			{
				id destination;
				sint64 amount;              // non-negative
				uint16 targetEpoch;         // not checked by isValid()!
			} transferInEpoch;

			// Used if type class is Variable and type is not VariableScalarMean
			struct VariableOptions
			{
				uint64 variable;            // For identifying variable (interpreted by contract only)
				Array<sint64, 4> values;    // N first amounts are proposed options sorted without duplicates, rest zero
			} variableOptions;

			// Used if type is VariableScalarMean
			struct VariableScalar
			{
				uint64 variable;            // For identifying variable (interpreted by contract only)
				sint64 minValue;            // Minimum value allowed in proposedValue and votes, must be > NO_VOTE_VALUE
				sint64 maxValue;            // Maximum value allowed in proposedValue and votes, must be >= minValue
				sint64 proposedValue;       // Needs to be in range between minValue and maxValue

				static constexpr sint64 minSupportedValue = 0x8000000000000001;
				static constexpr sint64 maxSupportedValue = 0x7fffffffffffffff;
			} variableScalar;
		};

		// Check if content of instance are valid. Epoch is not checked.
		// Also useful to show requirements of valid proposal.
		bool checkValidity() const
		{
			bool okay = false;
			// TODO: validate URL
			uint16 cls = ProposalTypes::cls(type);
			uint16 options = ProposalTypes::optionCount(type);
			switch (cls)
			{
			case ProposalTypes::Class::GeneralOptions:
				okay = options >= 2 && options <= 8;
				break;
			case ProposalTypes::Class::Transfer:
				if (!isZero(transfer.destination) && options >= 2 && options <= 5)
				{
					uint16 proposedAmounts = options - 1;
					okay = true;
					for (uint16 i = 0; i < proposedAmounts; ++i)
					{
						// no negative amounts
						if (transfer.amounts.get(i) < 0)
						{
							okay = false;
							break;
						}
					}
					okay = okay
						   && isArraySortedWithoutDuplicates(transfer.amounts, 0, proposedAmounts)
						   && transfer.amounts.rangeEquals(proposedAmounts, transfer.amounts.capacity(), 0);
				}
				break;
			case ProposalTypes::Class::TransferInEpoch:
				okay = options == 2 && !isZero(transferInEpoch.destination) && transferInEpoch.amount >= 0;
				break;
			case ProposalTypes::Class::Variable:
				if (options >= 2 && options <= 5)
				{
					// option voting
					uint16 proposedValues = options - 1;
					okay = isArraySortedWithoutDuplicates(variableOptions.values, 0, proposedValues)
						   && variableOptions.values.rangeEquals(proposedValues, variableOptions.values.capacity(), 0);
				}
				else if (options == 0)
				{
					// scalar voting
					if (supportScalarVotes)
						okay = variableScalar.minValue <= variableScalar.proposedValue
							&& variableScalar.proposedValue <= variableScalar.maxValue
							&& variableScalar.minValue > NO_VOTE_VALUE;
				}
				break;
			}
			return okay;
		}

		// Whether to support scalar votes next to option votes.
		static constexpr bool supportScalarVotes = SupportScalarVotes;

		ProposalDataV1() = default;
		ProposalDataV1(const ProposalDataV1<SupportScalarVotes>& src)
		{
			copyMemory(*this, src);
		}
	};
	static_assert(sizeof(ProposalDataV1<true>) == 256 + 8 + 64, "Unexpected struct size.");

	// Proposal data struct for 2-option proposals (requires less storage space).
	// Input data for contract procedure call, usable as ProposalDataType in ProposalVoting
	struct ProposalDataYesNo
	{
		// URL explaining proposal, zero-terminated string.
		Array<uint8, 256> url;

		// Epoch, when proposal is active. For setProposal(), 0 means to clear proposal and non-zero means the current epoch.
		uint16 epoch;

		// Type of proposal, see ProposalTypes.
		uint16 type;

		// Tick when proposal has been set. Output only, overwritten in setProposal().
		uint32 tick;

		// Proposal payload data (for all except types with class GeneralProposal)
		union
		{
			// Used if type class is Transfer
			struct Transfer
			{
				id destination;
				sint64 amount;		// Amount of proposed option (non-negative)
			} transfer;

			// Used if type class is Variable and type is not VariableScalarMean
			struct VariableOptions
			{
				uint64 variable;    // For identifying variable (interpreted by contract only)
				sint64 value;		// Value of proposed option, rest zero
			} variableOptions;
		};

		// Check if content of instance are valid. Epoch is not checked.
		// Also useful to show requirements of valid proposal.
		bool checkValidity() const
		{
			bool okay = false;
			// TODO: validate URL
			uint16 cls = ProposalTypes::cls(type);
			uint16 options = ProposalTypes::optionCount(type);
			switch (cls)
			{
			case ProposalTypes::Class::GeneralOptions:
				okay = options >= 2 && options <= 3;
				break;
			case ProposalTypes::Class::Transfer:
				okay = (options == 2 && !isZero(transfer.destination) && transfer.amount >= 0);
				break;
			case ProposalTypes::Class::Variable:
				okay = (options == 2);
				break;
			}
			return okay;
		}

		// Whether to support scalar votes next to option votes.
		static constexpr bool supportScalarVotes = false;
	};
	static_assert(sizeof(ProposalDataYesNo) == 256 + 8 + 40, "Unexpected struct size.");


	// Used internally by ProposalVoting to store a proposal with all votes
	template <typename ProposalDataType, uint32 numOfVoters>
	struct ProposalWithAllVoteData;


	// Option for ProposerAndVoterHandlingT in ProposalVoting that allows both voting and setting proposals for computors only.
	template <uint16 proposalSlotCount = NUMBER_OF_COMPUTORS>
	struct ProposalAndVotingByComputors;

	// Option for ProposerAndVoterHandlingT in ProposalVoting that allows both voting for computors only and creating/chaning proposals for anyone.
	template <uint16 proposalSlotCount>
	struct ProposalByAnyoneVotingByComputors;

	template <unsigned int maxShareholders>
	struct ProposalAndVotingByShareholders;

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalFunctionCall;

	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalProcedureCall;

	/*
	* Proposal voting state for use in contract state.
	* Voting is running until end of epoch, each proposer/computor can have one proposal at a time (or zero).
	* ProposerAndVoterHandlingType:
	*	Class for checking right to propose/vote and getting index in array. May have member data such
	*   as an array of IDs and may be initialized by accessing the public member proposerAndVoter.
	* ProposalDataT:
	*   Class defining supported proposals. Also determines storage for proposals and votes.
	*/
	template <typename ProposerAndVoterHandlingT, typename ProposalDataT>
	class ProposalVoting
	{
	public:
		static constexpr uint16 maxProposals = ProposerAndVoterHandlingT::maxProposals;
		static constexpr uint32 maxVoters = ProposerAndVoterHandlingT::maxVoters;

		typedef ProposerAndVoterHandlingT ProposerAndVoterHandlingType;
		typedef ProposalDataT ProposalDataType;
		typedef ProposalWithAllVoteData<
			ProposalDataT,
			maxVoters
		> ProposalAndVotesDataType;

		static_assert(maxProposals <= INVALID_PROPOSAL_INDEX);
		static_assert(maxVoters <= INVALID_VOTER_INDEX);

		// Handling of who has the right to propose and to vote + proposal / voter indices
		ProposerAndVoterHandlingType proposersAndVoters;

	protected:
		// Proposals and corresponding votes. No direct access for contracts.
		ProposalAndVotesDataType proposals[maxProposals];

		// Give user interface access to proposals
		friend struct QpiContextProposalProcedureCall<ProposerAndVoterHandlingT, ProposalDataT>;
		friend struct QpiContextProposalFunctionCall<ProposerAndVoterHandlingT, ProposalDataT>;
	};

	// Proposal user interface available in contract functions
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalFunctionCall
	{
		// Get proposal with given index if index is valid and proposal is set (epoch > 0)
		bool getProposal(uint16 proposalIndex, ProposalDataType& proposal) const;

		// Get data of single vote
		bool getVote(uint16 proposalIndex, uint32 voterIndex, ProposalSingleVoteDataV1& vote) const;

		// Get summary of all votes casted
		bool getVotingSummary(uint16 proposalIndex, ProposalSummarizedVotingDataV1& votingSummary) const;

		// Return index of existing proposal or INVALID_PROPOSAL_INDEX if there is no proposal by given proposer
		uint16 proposalIndex(const id& proposerId) const;

		// Return proposer ID of given proposal index or NULL_ID if there is no proposal at this index
		id proposerId(uint16 proposalIndex) const;

		// Return voter index for given ID or INVALID_VOTER_INDEX if ID has no right to vote
		uint32 voterIndex(const id& voterId) const;

		// Return ID for given voter index or NULL_ID if index is invalid
		id voterId(uint32 voterIndex) const;

		// Return next proposal index of proposals of given epoch (default: current epoch)
		// or -1 if there are not any more such proposals behind the passed index.
		// Pass -1 to get first index.
		sint32 nextProposalIndex(sint32 prevProposalIndex, uint16 epoch = 0) const;

		// Return next proposal index of finished proposal (not created in current epoch, voting not possible anymore)
		// or -1 if there are not any more such proposals behind the passed index.
		// Pass -1 to get first index.
		sint32 nextFinishedProposalIndex(sint32 prevProposalIndex) const;

		// ProposalVoting type to work with
		typedef ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType> ProposalVotingType;

		// Constructor. Use qpi(proposalVotingObject) to construct instance.
		QpiContextProposalFunctionCall(
			const QpiContextFunctionCall& qpi,
			const ProposalVotingType& pv
		) : qpi(qpi), pv(pv) {}

		const QpiContextFunctionCall& qpi;
		const ProposalVotingType& pv;
	};

	// Proposal user interface available in contract procedures
	template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
	struct QpiContextProposalProcedureCall : public QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType>
	{
		// Set proposal if proposer has right to do so, proposal is valid, proposal.epoch is current epoch,
		// and a proposal slot is available.
		// If the proposer already has a proposal slot, his previous proposal is overwritten (and all votes
		// are discarded).
		// If there is no free slot, one of the oldest proposals from prior epochs is deleted to free a slot.
		// This may be also used to clear a proposal by setting proposal.epoch = 0.
		// Return proposalIndex if proposal has been set, or INVALID_PROPOSAL_INDEX on error.
		uint16 setProposal(
			const id& proposer,
			const ProposalDataType& proposal
		);

		// Clear proposal of given index (without checking rights). Returns false if proposalIndex is invalid.
		bool clearProposal(
			uint16 proposalIndex
		);

		// Cast vote for proposal with index vote.proposalIndex if voter has right to vote, the proposal's epoch
		// is the current epoch, vote.proposalType and vote.proposalTick match the corresponding proposal's values,
		// and vote.voteValue is valid for the proposal type.
		// This can be used to remove a previous vote by vote.voteValue = NO_VOTE_VALUE.
		// Return whether vote has been casted.
		bool vote(
			const id& voter,
			const ProposalSingleVoteDataV1& vote
		);

		// ProposalVoting type to work with
		typedef ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType> ProposalVotingType;

		// Base class
		typedef QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType> BaseClass;

		// Constructor. Use qpi(proposalVotingObject) to construct instance.
		QpiContextProposalProcedureCall(
			const QpiContextFunctionCall& qpi,
			ProposalVotingType& pv
		) : BaseClass(qpi, pv) {}
	};

	//////////

	// QPI context base class (common data, by default has no stack for locals)
	struct QpiContext
	{
	protected:
		// Construction is done in core, not allowed in contracts
		QpiContext(
			unsigned int contractIndex,
			const m256i& originator,
			const m256i& invocator,
			long long invocationReward,
			unsigned char entryPoint
		) {
			init(contractIndex, originator, invocator, invocationReward, entryPoint, -1);
		}

		void init(
			unsigned int contractIndex,
			const m256i& originator,
			const m256i& invocator,
			long long invocationReward,
			unsigned char entryPoint,
			int stackIndex
		) {
			ASSERT(invocationReward >= 0);
			_currentContractIndex = contractIndex;
			_currentContractId = m256i(contractIndex, 0, 0, 0);
			_originator = originator;
			_invocator = invocator;
			_invocationReward = invocationReward;
			_entryPoint = entryPoint;
			_stackIndex = stackIndex;
		}

		unsigned int _currentContractIndex;
		int _stackIndex;
		m256i _currentContractId, _originator, _invocator;
		long long _invocationReward;
		unsigned char _entryPoint;

	private:
		// Disabling copy and move
		QpiContext(const QpiContext&) = delete;
		QpiContext(QpiContext&&) = delete;
		QpiContext& operator=(const QpiContext&) = delete;
		QpiContext& operator=(QpiContext&&) = delete;
	};

	// QPI function available to contract functions and procedures
	struct QpiContextFunctionCall : public QpiContext
	{
		inline id arbitrator(
		) const;

		inline id computor(
			uint16 computorIndex // [0..675]
		) const;

		inline uint8 day(
		) const; // [1..31]

		inline uint8 dayOfWeek(
			uint8 year, // (0 = 2000, 1 = 2001, ..., 99 = 2099)
			uint8 month,
			uint8 day
		) const; // [0..6] (0 = Wednesday)

		inline uint16 epoch(
		) const; // [0..9'999]

		inline bit getEntity(
			const id& id,
			Entity& entity
		) const; // Returns "true" if the entity has been found, returns "false" otherwise

		inline uint8 hour(
		) const; // [0..23]

		// Return the invocation reward (amount transferred to contract immediately before invoking)
		sint64 invocationReward() const { return _invocationReward; }

		// Returns the id of the user/contract who has triggered this contract; returns NULL_ID if there has been no user/contract
		id invocator() const { return _invocator; }

		// Returns the ID of the entity who has made this IPO bid or NULL_ID if the ipoContractIndex or ipoBidIndex are invalid.
		inline id ipoBidId(
			uint32 ipoContractIndex,
			uint32 ipoBidIndex
		) const;

		// Returns the price of an IPO bid, -1 if contract index is invalid, -2 if contract is not in IPO, -3 if bid index is invalid.
		inline sint64 ipoBidPrice(
			uint32 ipoContractIndex,
			uint32 ipoBidIndex
		) const;

		template <typename T>
		inline id K12(
			const T& data
		) const;

		inline uint16 millisecond(
		) const; // [0..999]

		inline uint8 minute(
		) const; // [0..59]

		inline uint8 month(
		) const; // [1..12]

		inline id nextId(
			const id& currentId
		) const;

		inline id prevId(
			const id& currentId
		) const;

		inline sint64 numberOfPossessedShares(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			uint16 ownershipManagingContractIndex,
			uint16 possessionManagingContractIndex
		) const;

		inline sint64 numberOfShares(
			const Asset& asset,
			const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(),
			const AssetPossessionSelect& possession = AssetPossessionSelect::any()
		) const;

		inline bool isAssetIssued(
			const m256i& id,
			unsigned long long assetName
		) const;

		// Returns -1 if the current tick is empty, returns the number of the transactions in the tick otherwise, including 0.
		inline sint32 numberOfTickTransactions(
		) const;

		// Returns the id of the user who has triggered the whole chain of invocations with their transaction; returns NULL_ID if there has been no user
		id originator() const { return _originator; }

		inline uint8 second(
		) const; // [0..59]

		// return current datetime (year, month, day, hour, minute, second, millisec)
		inline DateAndTime now() const;

		// return last spectrum digest on etalonTick
		inline m256i getPrevSpectrumDigest() const;

		// return last universe digest on etalonTick
		inline m256i getPrevUniverseDigest() const;

		// return last computer digest on etalonTick
		inline m256i getPrevComputerDigest() const;

		// run the score function (in qubic mining) and return first 256 bit of output
		inline m256i computeMiningFunction(const m256i miningSeed, const m256i publicKey, const m256i nonce) const;

		inline bit signatureValidity(
			const id& entity,
			const id& digest,
			const Array<sint8, 64>& signature
		) const;

		inline uint32 tick(
		) const; // [0..999'999'999]

		inline uint8 year(
		) const; // [0..99] (0 = 2000, 1 = 2001, ..., 99 = 2099)

		// Access proposal functions with qpi(proposalVotingObject).func().
		template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
		inline QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType> operator()(
			const ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
		) const;

		// Internal functions, calling not allowed in contracts
		inline void* __qpiAllocLocals(unsigned int sizeOfLocals) const;
		inline void __qpiFreeLocals() const;
		inline const QpiContextFunctionCall& __qpiConstructContextOtherContractFunctionCall(unsigned int otherContractIndex) const;
		inline void __qpiFreeContext() const;
		inline void * __qpiAcquireStateForReading(unsigned int contractIndex) const;
		inline void __qpiReleaseStateForReading(unsigned int contractIndex) const;
		inline void __qpiAbort(unsigned int errorCode) const;

	protected:
		// Construction is done in core, not allowed in contracts
		QpiContextFunctionCall(unsigned int contractIndex, const m256i& originator, long long invocationReward, unsigned char entryPoint) : QpiContext(contractIndex, originator, originator, invocationReward, entryPoint) {}
	};

	// QPI procedures available to contract procedures (not to contract functions)
	struct QpiContextProcedureCall : public QPI::QpiContextFunctionCall
	{
		inline sint64 acquireShares(
			const Asset& asset,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			uint16 sourceOwnershipManagingContractIndex,
			uint16 sourcePossessionManagingContractIndex,
			sint64 offeredTransferFee
		) const; // Returns payed fee on success (>= 0), -requestedFee if offeredTransferFee or contract balance is not sufficient, INVALID_AMOUNT in case of other error.

		inline sint64 burn(
			sint64 amount
		) const;

		inline bool distributeDividends( //  Attempts to pay dividends
			sint64 amountPerShare // Total amount will be 676x of this
		) const; // "true" if the contract has had enough qus, "false" otherwise

		inline sint64 issueAsset(
			uint64 name,
			const id& issuer,
			sint8 numberOfDecimalPlaces,
			sint64 numberOfShares,
			uint64 unitOfMeasurement
		) const; // Returns number of shares or 0 on error

		// Bid in contract IPO, deducting price * quantity QU. Bids that don't get shares are refunded.
		// Returns number of bids registered or -1 if any invalid value is passed or the owned funds aren't sufficient.
		// If the return value >= 0, the full amount has been deducted, but if return value < quantity it has been partially
		// refunded.
		inline sint64 bidInIPO(
			uint32 IPOContractIndex,
			sint64 price,
			uint32 quantity
		) const;

		inline sint64 releaseShares(
			const Asset& asset,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			uint16 destinationOwnershipManagingContractIndex,
			uint16 destinationPossessionManagingContractIndex,
			sint64 offeredTransferFee
		) const; // Returns payed fee on success (>= 0), -requestedFee if offeredTransferFee or contract balance is not sufficient, INVALID_AMOUNT in case of other error.

		inline sint64 transfer( // Attempts to transfer energy from this qubic
			const id& destination, // Destination to transfer to, use NULL_ID to destroy the transferred energy
			sint64 amount // Energy amount to transfer, must be in [0..1'000'000'000'000'000] range
		) const; // Returns remaining energy amount; if the value is less than 0 then the attempt has failed, in this case the absolute value equals to the insufficient amount

		inline sint64 transferShareOwnershipAndPossession(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			const id& newOwnerAndPossessor // New owner and possessor. Pass NULL_ID to burn shares (not allowed for contract shares).
		) const; // Returns remaining number of possessed shares satisfying all the conditions; if the value is less than 0, the attempt has failed, in this case the absolute value equals to the insufficient number, INVALID_AMOUNT indicates another error

		// Access proposal procedures with qpi(proposalVotingObject).proc().
		template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
		inline QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType> operator()(
			ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
		) const;


		// Internal functions, calling not allowed in contracts
		inline const QpiContextProcedureCall& __qpiConstructProcedureCallContext(unsigned int otherContractIndex, sint64 invocationReward) const;
		inline void* __qpiAcquireStateForWriting(unsigned int contractIndex) const;
		inline void __qpiReleaseStateForWriting(unsigned int contractIndex) const;
		template <unsigned int sysProcId, typename InputType, typename OutputType>
		void __qpiCallSystemProc(unsigned int otherContractIndex, InputType& input, OutputType& output, sint64 invocationReward) const;
		inline void __qpiNotifyPostIncomingTransfer(const id& source, const id& dest, sint64 amount, uint8 type) const;

	protected:
		// Construction is done in core, not allowed in contracts
		QpiContextProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward, unsigned char entryPoint) : QpiContextFunctionCall(contractIndex, originator, invocationReward, entryPoint) {}
	};

	// QPI available in REGISTER_USER_FUNCTIONS_AND_PROCEDURES
	struct QpiContextForInit : public QPI::QpiContext
	{
		inline void __registerUserFunction(USER_FUNCTION, unsigned short, unsigned short, unsigned short, unsigned int) const;
		inline void __registerUserProcedure(USER_PROCEDURE, unsigned short, unsigned short, unsigned short, unsigned int) const;

		// Construction is done in core, not allowed in contracts
		inline QpiContextForInit(unsigned int contractIndex);
	};

	// Used if no locals, input, or output is needed in a procedure or function
	struct NoData {};

	// Management rights transfer: pre-transfer input
	struct PreManagementRightsTransfer_input
	{
		Asset asset;
		id owner;
		id possessor;
		sint64 numberOfShares;
		sint64 offeredFee;
		uint16 otherContractIndex;
	};

	// Management rights transfer: pre-transfer output (default is all-zeroed = don't allow transfer)
	struct PreManagementRightsTransfer_output
	{
		bool allowTransfer;
		sint64 requestedFee;
	};

	// Management rights transfer: post-transfer input
	struct PostManagementRightsTransfer_input
	{
		Asset asset;
		id owner;
		id possessor;
		sint64 numberOfShares;
		sint64 receivedFee;
		uint16 otherContractIndex;
	};

	namespace TransferType
	{
		constexpr uint8 standardTransaction = 0;
		constexpr uint8 procedureTransaction = 1;
		constexpr uint8 qpiTransfer = 2;
		constexpr uint8 qpiDistributeDividends = 3;
		constexpr uint8 revenueDonation = 4;
		constexpr uint8 ipoBidRefund = 5;
	};

	// Input of POST_INCOMING_TRANSFER notification system call
	struct PostIncomingTransfer_input
	{
		id sourceId;
		sint64 amount;
		uint8 type;
	};

	//////////
	
	struct ContractBase
	{
		enum { __initializeEmpty = 1, __initializeLocalsSize = sizeof(NoData) };
		static void __initialize(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __beginEpochEmpty = 1, __beginEpochLocalsSize = sizeof(NoData) };
		static void __beginEpoch(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __endEpochEmpty = 1, __endEpochLocalsSize = sizeof(NoData) };
		static void __endEpoch(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __beginTickEmpty = 1, __beginTickLocalsSize = sizeof(NoData) };
		static void __beginTick(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __endTickEmpty = 1, __endTickLocalsSize = sizeof(NoData) };
		static void __endTick(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __preAcquireSharesEmpty = 1, __preAcquireSharesLocalsSize = sizeof(NoData) };
		static void __preAcquireShares(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __preReleaseSharesEmpty = 1, __preReleaseSharesLocalsSize = sizeof(NoData) };
		static void __preReleaseShares(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __postAcquireSharesEmpty = 1, __postAcquireSharesLocalsSize = sizeof(NoData) };
		static void __postAcquireShares(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __postReleaseSharesEmpty = 1, __postReleaseSharesLocalsSize = sizeof(NoData) };
		static void __postReleaseShares(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __postIncomingTransferEmpty = 1, __postIncomingTransferLocalsSize = sizeof(NoData) };
		static void __postIncomingTransfer(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __acceptOracleTrueReplyEmpty = 1, __acceptOracleTrueReplyLocalsSize = sizeof(NoData) };
		static void __acceptOracleTrueReply(const QpiContextProcedureCall&, void*, void*, void*) {}
		enum { __acceptOracleFalseReplyEmpty = 1, __acceptOracleFalseReplyLocalsSize = sizeof(NoData) };
		static void __acceptOracleFalseReply(const QpiContextProcedureCall&, void*, void*) {}
		enum { __acceptOracleUnknownReplyEmpty = 1, __acceptOracleUnknownReplyLocalsSize = sizeof(NoData) };
		static void __acceptOracleUnknownReply(const QpiContextProcedureCall&, void*, void*) {}
		enum { __expandEmpty = 1 };
		static void __expand(const QpiContextProcedureCall& qpi, void*, void*) {}
	};

	struct OracleBase
	{
	};

	// Internal macro for defining the system procedure macros
	#define NO_IO_SYSTEM_PROC(CapLetterName, FuncName, InputType, OutputType) \
		public: \
			typedef NoData CapLetterName##_locals; \
			NO_IO_SYSTEM_PROC_WITH_LOCALS(CapLetterName, FuncName, InputType, OutputType)

	// Internal macro for defining the system procedure macros
	#define NO_IO_SYSTEM_PROC_WITH_LOCALS(CapLetterName, FuncName, InputType, OutputType) \
		 public: \
			enum { FuncName##Empty = 0, FuncName##LocalsSize = sizeof(CapLetterName##_locals) }; \
			static_assert(sizeof(CapLetterName##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #CapLetterName "_locals size too large"); \
			inline static void FuncName(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, InputType& input, OutputType& output, CapLetterName##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##FuncName(qpi, state, input, output, locals); } \
			static void __impl_##FuncName(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, InputType& input, OutputType& output, CapLetterName##_locals& locals)

	// Define contract system procedure called to initialize contract state after IPO
	#define INITIALIZE()  NO_IO_SYSTEM_PROC(INITIALIZE, __initialize, NoData, NoData)

	// Define contract system procedure called to initialize contract state after IPO, provides zeroed instance of INITIALIZE_locals struct
	#define INITIALIZE_WITH_LOCALS()  NO_IO_SYSTEM_PROC_WITH_LOCALS(INITIALIZE, __initialize, NoData, NoData)

	// Define contract system procedure called at beginning of each epoch
	#define BEGIN_EPOCH()  NO_IO_SYSTEM_PROC(BEGIN_EPOCH, __beginEpoch, NoData, NoData)

	// Define contract system procedure called at beginning of each epoch, provides zeroed instance of BEGIN_EPOCH_locals struct
	#define BEGIN_EPOCH_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(BEGIN_EPOCH, __beginEpoch, NoData, NoData)

	// Define contract system procedure called at end of each epoch
	#define END_EPOCH() NO_IO_SYSTEM_PROC(END_EPOCH, __endEpoch, NoData, NoData)

	// Define contract system procedure called at end of each epoch, provides zeroed instance of END_EPOCH_locals struct
	#define END_EPOCH_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(END_EPOCH, __endEpoch, NoData, NoData)

	// Define contract system procedure called at beginning of each tick
	#define BEGIN_TICK() NO_IO_SYSTEM_PROC(BEGIN_TICK, __beginTick, NoData, NoData)

	// Define contract system procedure called at beginning of each tick, provides zeroed instance of BEGIN_TICK_locals struct
	#define BEGIN_TICK_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(BEGIN_TICK, __beginTick, NoData, NoData)

	// Define contract system procedure called at end of each tick
	#define END_TICK() NO_IO_SYSTEM_PROC(END_TICK, __endTick, NoData, NoData)

	// Define contract system procedure called at end of each tick, provides zeroed instance of BEGIN_TICK_locals struct
	#define END_TICK_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(END_TICK, __endTick, NoData, NoData)

	// Define contract system procedure called before asset management rights transfer with `qpi.releaseShares(). See
	// `doc/contracts.md` for details.
	#define PRE_ACQUIRE_SHARES() \
        NO_IO_SYSTEM_PROC(PRE_ACQUIRE_SHARES, __preAcquireShares, PreManagementRightsTransfer_input, \
                          PreManagementRightsTransfer_output)

	// Define contract system procedure called before asset management rights transfer with `qpi.releaseShares(). Provides
	// zeroed instance of PRE_ACQUIRE_SHARES_locals struct. See `doc/contracts.md` for details.
	#define PRE_ACQUIRE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(PRE_ACQUIRE_SHARES, __preAcquireShares, PreManagementRightsTransfer_input, \
                                      PreManagementRightsTransfer_output)

	// Define contract system procedure called before asset management rights transfer with `qpi.acquireShares(). See
	// `doc/contracts.md` for details.
	#define PRE_RELEASE_SHARES() \
        NO_IO_SYSTEM_PROC(PRE_RELEASE_SHARES, __preReleaseShares, PreManagementRightsTransfer_input, \
                          PreManagementRightsTransfer_output)

	// Define contract system procedure called before asset management rights transfer with `qpi.acquireShares(). Provides
	// zeroed instance of PRE_RELEASE_SHARES_locals struct. See `doc/contracts.md` for details.
	#define PRE_RELEASE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(PRE_RELEASE_SHARES, __preReleaseShares, PreManagementRightsTransfer_input, \
                                      PreManagementRightsTransfer_output)

	// Define contract system procedure called after asset management rights transfer with `qpi.releaseShares(). See
	// `doc/contracts.md` for details.
	#define POST_ACQUIRE_SHARES() \
        NO_IO_SYSTEM_PROC(POST_ACQUIRE_SHARES, __postAcquireShares, PostManagementRightsTransfer_input, NoData)

	// Define contract system procedure called after asset management rights transfer with `qpi.releaseShares(). Provides
	// zeroed instance of POST_ACQUIRE_SHARES_locals struct. See `doc/contracts.md` for details.
	#define POST_ACQUIRE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(POST_ACQUIRE_SHARES, __postAcquireShares, PostManagementRightsTransfer_input, \
                                      NoData)

	// Define contract system procedure called after asset management rights transfer with `qpi.acquireShares(). See
	// `doc/contracts.md` for details.
	#define POST_RELEASE_SHARES() \
        NO_IO_SYSTEM_PROC(POST_RELEASE_SHARES, __postReleaseShares, PostManagementRightsTransfer_input, NoData)

	// Define contract system procedure called after asset management rights transfer with `qpi.acquireShares(). Provides
	// zeroed instance of POST_RELEASE_SHARES_locals struct. See `doc/contracts.md` for details.
	#define POST_RELEASE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(POST_RELEASE_SHARES, __postReleaseShares, PostManagementRightsTransfer_input, \
                                      NoData)

	// Define contract system procedure called when QUs are transferred to the contract. See `doc/contracts.md` for
	// details.
	#define POST_INCOMING_TRANSFER() \
        NO_IO_SYSTEM_PROC(POST_INCOMING_TRANSFER, __postIncomingTransfer, PostIncomingTransfer_input, NoData)

	// Define contract system procedure called when QUs are transferred to the contract. Provides zeroed instance of
	// POST_INCOMING_TRANSFER_locals struct. See `doc/contracts.md` for details.
	#define POST_INCOMING_TRANSFER_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(POST_INCOMING_TRANSFER, __postIncomingTransfer, PostIncomingTransfer_input, \
                                      NoData)


	#define EXPAND() \
      public: \
        enum { __expandEmpty = 0 }; \
		static void __expand(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, CONTRACT_STATE2_TYPE& state2) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller;


	#define LOG_DEBUG(message) __logContractDebugMessage(CONTRACT_INDEX, message);

	#define LOG_ERROR(message) __logContractErrorMessage(CONTRACT_INDEX, message);

	#define LOG_INFO(message) __logContractInfoMessage(CONTRACT_INDEX, message);

	#define LOG_WARNING(message) __logContractWarningMessage(CONTRACT_INDEX, message);

	#define PRIVATE_FUNCTION(function) \
		private: \
			typedef QPI::NoData function##_locals; \
			PRIVATE_FUNCTION_WITH_LOCALS(function)

	#define PRIVATE_FUNCTION_WITH_LOCALS(function) \
		private: \
			enum { __is_function_##function = true }; \
			inline static void function(const QPI::QpiContextFunctionCall& qpi, const CONTRACT_STATE_TYPE& state, function##_input& input, function##_output& output, function##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##function(qpi, state, input, output, locals); } \
			static void __impl_##function(const QPI::QpiContextFunctionCall& qpi, const CONTRACT_STATE_TYPE& state, function##_input& input, function##_output& output, function##_locals& locals)

	#define PRIVATE_PROCEDURE(procedure) \
		private: \
			typedef QPI::NoData procedure##_locals; \
			PRIVATE_PROCEDURE_WITH_LOCALS(procedure)

	#define PRIVATE_PROCEDURE_WITH_LOCALS(procedure) \
		private: \
			enum { __is_function_##procedure = false }; \
			inline static void procedure(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##procedure(qpi, state, input, output, locals); } \
			static void __impl_##procedure(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals)

	#define PUBLIC_FUNCTION(function) \
		public: \
			typedef QPI::NoData function##_locals; \
			PUBLIC_FUNCTION_WITH_LOCALS(function)

	#define PUBLIC_FUNCTION_WITH_LOCALS(function) \
		public: \
			enum { __is_function_##function = true }; \
			inline static void function(const QPI::QpiContextFunctionCall& qpi, const CONTRACT_STATE_TYPE& state, function##_input& input, function##_output& output, function##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##function(qpi, state, input, output, locals); } \
			static void __impl_##function(const QPI::QpiContextFunctionCall& qpi, const CONTRACT_STATE_TYPE& state, function##_input& input, function##_output& output, function##_locals& locals)

	#define PUBLIC_PROCEDURE(procedure) \
		public: \
			typedef QPI::NoData procedure##_locals; \
			PUBLIC_PROCEDURE_WITH_LOCALS(procedure)

	#define PUBLIC_PROCEDURE_WITH_LOCALS(procedure) \
		public: \
			enum { __is_function_##procedure = false }; \
			inline static void procedure(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##procedure(qpi, state, input, output, locals); } \
			static void __impl_##procedure(const QPI::QpiContextProcedureCall& qpi, CONTRACT_STATE_TYPE& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals)

	#define REGISTER_USER_FUNCTIONS_AND_PROCEDURES() \
		public: \
			enum { __contract_index = CONTRACT_INDEX }; \
			inline static void __registerUserFunctionsAndProcedures(const QPI::QpiContextForInit& qpi) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl___registerUserFunctionsAndProcedures(qpi); } \
			static void __impl___registerUserFunctionsAndProcedures(const QPI::QpiContextForInit& qpi)

	#define REGISTER_USER_FUNCTION(userFunction, inputType) \
		static_assert(__is_function_##userFunction, #userFunction " is procedure"); \
		static_assert(inputType >= 1 && inputType <= 65535, "inputType must be >= 1 and <= 65535"); \
		static_assert(sizeof(userFunction##_output) <= 65535, #userFunction "_output size too large"); \
		static_assert(sizeof(userFunction##_input) <= 65535, #userFunction "_input size too large"); \
		static_assert(sizeof(userFunction##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userFunction "_locals size too large"); \
		qpi.__registerUserFunction((USER_FUNCTION)userFunction, inputType, sizeof(userFunction##_input), sizeof(userFunction##_output), sizeof(userFunction##_locals));

	#define REGISTER_USER_PROCEDURE(userProcedure, inputType) \
		static_assert(!__is_function_##userProcedure, #userProcedure " is function"); \
		static_assert(inputType >= 1 && inputType <= 65535, "inputType must be >= 1 and <= 65535"); \
		static_assert(sizeof(userProcedure##_output) <= 65535, #userProcedure "_output size too large"); \
		static_assert(sizeof(userProcedure##_input) <= 65535, #userProcedure "_input size too large"); \
		static_assert(sizeof(userProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userProcedure "_locals size too large"); \
		qpi.__registerUserProcedure((USER_PROCEDURE)userProcedure, inputType, sizeof(userProcedure##_input), sizeof(userProcedure##_output), sizeof(userProcedure##_locals));

	// Call function or procedure of current contract (without changing invocation reward)
	// WARNING: input may be changed by called function
	#define CALL(functionOrProcedure, input, output) \
		static_assert(sizeof(CONTRACT_STATE_TYPE::functionOrProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #functionOrProcedure "_locals size too large"); \
		functionOrProcedure(qpi, state, input, output, *(functionOrProcedure##_locals*)qpi.__qpiAllocLocals(sizeof(CONTRACT_STATE_TYPE::functionOrProcedure##_locals))); \
		qpi.__qpiFreeLocals()

	// Invoke procedure of current contract with changed invocation reward
	// WARNING: input may be changed by called function
	// TODO: INVOKE

	// Call function of other contract
	// WARNING: input may be changed by called function
	#define CALL_OTHER_CONTRACT_FUNCTION(contractStateType, function, input, output) \
		static_assert(sizeof(contractStateType::function##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #function "_locals size too large"); \
		static_assert(contractStateType::__is_function_##function, "CALL_OTHER_CONTRACT_FUNCTION() cannot be used to invoke procedures."); \
		static_assert(!(contractStateType::__contract_index == CONTRACT_STATE_TYPE::__contract_index), "Use CALL() to call a function of this contract."); \
		static_assert(contractStateType::__contract_index < CONTRACT_STATE_TYPE::__contract_index, "You can only call contracts with lower index."); \
		contractStateType::function( \
			qpi.__qpiConstructContextOtherContractFunctionCall(contractStateType::__contract_index), \
			*(contractStateType*)qpi.__qpiAcquireStateForReading(contractStateType::__contract_index), \
			input, output, \
			*(contractStateType::function##_locals*)qpi.__qpiAllocLocals(sizeof(contractStateType::function##_locals))); \
		qpi.__qpiFreeContext(); \
		qpi.__qpiReleaseStateForReading(contractStateType::__contract_index); \
		qpi.__qpiFreeLocals()

	// Transfer invocation reward and invoke of other contract (procedure only)
	// WARNING: input may be changed by called function
	#define INVOKE_OTHER_CONTRACT_PROCEDURE(contractStateType, procedure, input, output, invocationReward) \
		static_assert(sizeof(contractStateType::procedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #procedure "_locals size too large"); \
		static_assert(!contractStateType::__is_function_##procedure, "INVOKE_OTHER_CONTRACT_PROCEDURE() cannot be used to call functions."); \
		static_assert(!(contractStateType::__contract_index == CONTRACT_STATE_TYPE::__contract_index), "Use CALL() to call a function/procedure of this contract."); \
		static_assert(contractStateType::__contract_index < CONTRACT_STATE_TYPE::__contract_index, "You can only call contracts with lower index."); \
		contractStateType::procedure( \
			qpi.__qpiConstructProcedureCallContext(contractStateType::__contract_index, invocationReward), \
			*(contractStateType*)qpi.__qpiAcquireStateForWriting(contractStateType::__contract_index), \
			input, output, \
			*(contractStateType::procedure##_locals*)qpi.__qpiAllocLocals(sizeof(contractStateType::procedure##_locals))); \
		qpi.__qpiFreeContext(); \
		qpi.__qpiReleaseStateForWriting(contractStateType::__contract_index); \
		qpi.__qpiFreeLocals()

	#define QUERY_ORACLE(oracle, query) // TODO

	#define SELF id(CONTRACT_INDEX, 0, 0, 0)

	#define SELF_INDEX CONTRACT_INDEX
}
