// Basic math functions (not optimized but with minimal dependencies)

#pragma once
#include <lib/platform_common/qintrin.h> 

namespace math_lib
{

template <class T>
inline constexpr const T& max(const T& left, const T& right)
{
    return (left < right) ? right : left;
}

template <class T>
inline constexpr const T& min(const T& left, const T& right)
{
    return (left < right) ? left : right;
}

template <typename T>
inline constexpr T abs(const T& a)
{
    return (a < 0) ? -a : a;
}

// div() and mod() are defined in qpi.h

// Divides a by b rounding up, but return 0 if b is 0 (only supports unsigned integers)
inline static unsigned long long divUp(unsigned long long a, unsigned long long b)
{
    return b ? ((a + b - 1) / b) : 0;
}

// Divides a by b rounding up, but return 0 if b is 0 (only supports unsigned integers)
inline static unsigned int divUp(unsigned int a, unsigned int b)
{
    return b ? ((a + b - 1) / b) : 0;
}

// Divides a by b rounding up, but return 0 if b is 0 (only supports unsigned integers)
inline static unsigned short divUp(unsigned short a, unsigned short b)
{
    return b ? ((a + b - 1) / b) : 0;
}

// Divides a by b rounding up, but return 0 if b is 0 (only supports unsigned integers)
inline static unsigned char divUp(unsigned char a, unsigned char b)
{
    return b ? ((a + b - 1) / b) : 0;
}

inline constexpr unsigned long long findNextPowerOf2(unsigned long long num)
{
    if (num == 0)
        return 1;

    num--;
    num |= num >> 1;
    num |= num >> 2;
    num |= num >> 4;
    num |= num >> 8;
    num |= num >> 16;
    num |= num >> 32;
    num++;

    return num;
}

//////////
// safety multiplying a and b and then clamp

inline static long long smul(long long a, long long b)
{
	long long hi, lo;
	lo = _mul128(a, b, &hi);
	if (hi != (lo >> 63))
	{
		return ((a > 0) == (b > 0)) ? INT64_MAX : INT64_MIN;
	}
	return lo;
}

inline static unsigned long long smul(unsigned long long a, unsigned long long b)
{
	unsigned long long hi, lo;
	lo = _umul128(a, b, &hi);
	if (hi != 0)
	{
		return UINT64_MAX;
	}
	return lo;
}

inline static int smul(int a, int b)
{
	long long r = (long long)(a) * (long long)(b);
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
		return (int)r;
	}
}

inline static unsigned int smul(unsigned int a, unsigned int b)
{
	unsigned long long r = (unsigned long long)(a) * (unsigned long long)(b);
	if (r > UINT32_MAX)
	{
		return UINT32_MAX;
	}
	return (unsigned int)r;
}

//////////
// safety adding a and b and then clamp

inline static long long sadd(long long a, long long b)
{
	long long sum = a + b;
	if (a < 0 && b < 0 && sum > 0) // negative overflow
		return INT64_MIN;
	if (a > 0 && b > 0 && sum < 0) // positive overflow
		return INT64_MAX;
	return sum;
}

inline static unsigned long long sadd(unsigned long long a, unsigned long long b)
{
	if (UINT64_MAX - a < b)
		return UINT64_MAX;
	return a + b;
}

inline static int sadd(int a, int b)
{
	long long sum = (long long)(a)+(long long)(b);
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
		return (int)sum;
	}
}

inline static unsigned int sadd(unsigned int a, unsigned int b)
{
	unsigned long long sum = (unsigned long long)(a)+(unsigned long long)(b);
	if (sum > UINT32_MAX)
	{
		return UINT32_MAX;
	}
	return (unsigned int)sum;
}

}
