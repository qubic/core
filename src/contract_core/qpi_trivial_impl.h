// Implements several trivial util functions of QPI
// CAUTION: Include this AFTER the contract implementations!

#pragma once

#include "../contracts/qpi.h"
#include "../contracts/math_lib.h"
#include "../platform/memory.h"
#include "../platform/time.h"

#include "../four_q.h"
#include "../kangaroo_twelve.h"
#include "../spectrum/special_entities.h"

namespace QPI
{
	template <typename T1, typename T2>
	inline void copyMemory(T1& dst, const T2& src)
	{
		static_assert(sizeof(dst) == sizeof(src), "Size of source and destination must match to run copyMemory().");
		copyMem(&dst, &src, sizeof(dst));
	}

	template <typename T1, typename T2>
	inline void copyToBuffer(T1& dst, const T2& src, bool setTailToZero)
	{
		static_assert(sizeof(dst) >= sizeof(src), "Destination buffer must be at least the size of the source object.");
		copyMem(&dst, &src, sizeof(src));
		if (sizeof(dst) > sizeof(src) && setTailToZero)
		{
			uint8* tailPtr = reinterpret_cast<uint8*>(&dst) + sizeof(src);
			const uint64 tailSize = sizeof(dst) - sizeof(src);
			setMem(tailPtr, tailSize, 0);
		}
	}

	template <typename T1, typename T2>
	inline void copyFromBuffer(T1& dst, const T2& src)
	{
		static_assert(sizeof(dst) <= sizeof(src), "Destination object must be at most the size of the source buffer.");
		copyMem(&dst, &src, sizeof(dst));
	}

	template <typename T>
	inline void setMemory(T& dst, uint8 value)
	{
		setMem(&dst, sizeof(dst), value);
	}

	// Check if array is sorted in given range (duplicates allowed). Returns false if range is invalid.
	template <typename T, uint64 L>
	bool isArraySorted(const Array<T, L>& Array, uint64 beginIdx, uint64 endIdx)
	{
		if (endIdx > L || beginIdx > endIdx)
			return false;

		for (uint64 i = beginIdx + 1; i < endIdx; ++i)
		{
			if (Array.get(i - 1) > Array.get(i))
				return false;
		}

		return true;
	}

	// Check if array is sorted without duplicates in given range. Returns false if range is invalid.
	template <typename T, uint64 L>
	bool isArraySortedWithoutDuplicates(const Array<T, L>& Array, uint64 beginIdx, uint64 endIdx)
	{
		if (endIdx > L || beginIdx > endIdx)
			return false;

		for (uint64 i = beginIdx + 1; i < endIdx; ++i)
		{
			if (Array.get(i - 1) >= Array.get(i))
				return false;
		}

		return true;
	}
}

QPI::id QPI::QpiContextFunctionCall::arbitrator() const
{
	return arbitratorPublicKey;
}

QPI::id QPI::QpiContextFunctionCall::computor(unsigned short computorIndex) const
{
	return broadcastedComputors.computors.publicKeys[computorIndex % NUMBER_OF_COMPUTORS];
}

unsigned char QPI::QpiContextFunctionCall::dayOfWeek(unsigned char year, unsigned char month, unsigned char day) const
{
	return dayIndex(year, month, day) % 7;
}

bool QPI::QpiContextFunctionCall::signatureValidity(const m256i& entity, const m256i& digest, const Array<signed char, 64>& signature) const
{
	return verify(entity.m256i_u8, digest.m256i_u8, reinterpret_cast<const unsigned char*>(&signature));
}

template <typename T>
m256i QPI::QpiContextFunctionCall::K12(const T& data) const
{
	m256i digest;

	KangarooTwelve(&data, sizeof(data), &digest, sizeof(digest));

	return digest;
}

//////////
// safety multiplying a and b and then clamp

inline static QPI::sint64 QPI::smul(QPI::sint64 a, QPI::sint64 b)
{
	return math_lib::smul(a, b);
}

inline static QPI::uint64 QPI::smul(QPI::uint64 a, QPI::uint64 b)
{
	return math_lib::smul(a, b);
}

inline static QPI::sint32 QPI::smul(QPI::sint32 a, QPI::sint32 b)
{
	return math_lib::smul(a, b);
}

inline static QPI::uint32 QPI::smul(QPI::uint32 a, QPI::uint32 b)
{
	return math_lib::smul(a, b);
}

//////////
// safety adding a and b and then clamp

inline static QPI::sint64 QPI::sadd(QPI::sint64 a, QPI::sint64 b)
{
	return math_lib::sadd(a, b);
}

inline static QPI::uint64 QPI::sadd(QPI::uint64 a, QPI::uint64 b)
{
	return math_lib::sadd(a, b);
}

inline static QPI::sint32 QPI::sadd(QPI::sint32 a, QPI::sint32 b)
{
	return math_lib::sadd(a, b);
}

inline static QPI::uint32 QPI::sadd(QPI::uint32 a, QPI::uint32 b)
{
	return math_lib::sadd(a, b);
}
