// Basic math functions (not optimized but with minimal dependencies)

#pragma once

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

}
