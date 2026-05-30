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

// Compue greatest common divisor of a and b. Returns 0 if a or b are negative and 0 if both are 0.
template <typename T>
inline constexpr T greatestCommonDivisor(T a, T b)
{
	if (a < 0 || b < 0)
		return 0;
	while (b != 0)
	{
		T tmp = b;
		b = a % b;
		a = tmp;
	}
	return a;
}

// hard termination backstop for iroot
static constexpr unsigned int IROOT_NEWTON_CAP = 100;

// Number of significant bits of x (highest set bit + 1); bitLength(0) == 0.
inline static unsigned int bitLength(unsigned long long x)
{
    if (x == 0)
    {
        return 0;
    }
    return 64U - static_cast<unsigned int>(__lzcnt64(x));
}

// Returns true if x^exp <= maxAllowed
inline static bool powerLessOrEqual(unsigned long long x, unsigned int exp, unsigned long long maxAllowed)
{
    if (exp == 0)
    {
        return 1ULL <= maxAllowed;
    }
    if (x <= 1)
    {
        return x <= maxAllowed;
    }

    unsigned long long p = 1;
    for (unsigned int i = 0; i < exp; i++)
    {
        if (p > maxAllowed / x)
        {
            // p * x would exceed maxAllowed
            return false;
        }
        p *= x;
    }
    return true;
}

// One Newton step toward floor(n^(1/k)): exact integer floor of ((k-1)*x + n/x^(k-1)) / k,
// formed so neither (k-1)*x nor x^(k-1) can overflow u64
template<unsigned int k>
inline static unsigned long long irootNewtonStep(unsigned long long n, unsigned long long x)
{
    unsigned long long xPowKm1 = 1;
    bool overflowed = false;
    for (unsigned int j = 0; j < k - 1; j++)
    {
        if (xPowKm1 > 0xFFFFFFFFFFFFFFFFULL / x)
        {
            // x^(k-1) > u64max => > n => divTerm = 0
            overflowed = true;
            break;
        }
        xPowKm1 *= x;
    }
    const unsigned long long divTerm = overflowed ? 0ULL : (n / xPowKm1);

    if (divTerm >= x)
    {
        return x + (divTerm - x) / k;
    }
    return x - (x - divTerm + (k - 1)) / k;
}

// Floor of the integer k-th root: the largest r with r^k <= n
template<unsigned int k>
inline static unsigned long long irootK64(unsigned long long n)
{
    static_assert(k >= 1 && k <= 64, "irootK64 only supports k in [1, 64]");
    if (k == 1)
    {
        return n;
    }

    if (n < 2)
    {
        // iroot(0) = 0, iroot(1) = 1
        return n;
    }

    // Init value
    const unsigned int seedShift = (bitLength(n) + k - 1) / k;   // ceil(bitLength(n)/k); <= 32 for k >= 2
    unsigned long long x = 1ULL << seedShift;

    // Condition-driven Newton
    unsigned long long t = irootNewtonStep<k>(n, x);
    unsigned int iter = 0;
    while (t < x && iter < IROOT_NEWTON_CAP)
    {
        x = t;
        t = irootNewtonStep<k>(n, x);
        iter++;
    }

    // Exact floor correction
    while (!powerLessOrEqual(x, k, n))
    {
        x--;
    }
    while (x < 0xFFFFFFFFFFFFFFFFULL && powerLessOrEqual(x + 1, k, n))
    {
        x++;
    }
    return x;
}

}
