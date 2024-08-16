// Implements several trivial util functions of QPI
// CAUTION: Include this AFTER the contract implementations!

#pragma once

#include "../contracts/qpi.h"
#include "../platform/memory.h"

namespace QPI
{
	template <typename T1, typename T2>
	inline void copyMemory(T1& dst, const T2& src)
	{
		static_assert(sizeof(dst) == sizeof(src), "Size of source and destination must match to run copyMemory().");
		copyMem(&dst, &src, sizeof(dst));
	}

	template <typename T>
	inline void setMemory(T& dst, uint8 value)
	{
		setMem(&dst, sizeof(dst), value);
	}

	// Check if array is sorted in given range (duplicates allowed). Returns false if range is invalid.
	template <typename T, uint64 L>
	bool isArraySorted(const array<T, L>& array, uint64 beginIdx, uint64 endIdx)
	{
		if (endIdx > L || beginIdx > endIdx)
			return false;

		for (uint64 i = beginIdx + 1; i < endIdx; ++i)
		{
			if (array.get(i - 1) > array.get(i))
				return false;
		}

		return true;
	}

	// Check if array is sorted without duplicates in given range. Returns false if range is invalid.
	template <typename T, uint64 L>
	bool isArraySortedWithoutDuplicates(const array<T, L>& array, uint64 beginIdx, uint64 endIdx)
	{
		if (endIdx > L || beginIdx > endIdx)
			return false;

		for (uint64 i = beginIdx + 1; i < endIdx; ++i)
		{
			if (array.get(i - 1) >= array.get(i))
				return false;
		}

		return true;
	}
}
