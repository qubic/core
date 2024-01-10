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

// Divide a by b, but return 0 if b is 0 (rounding to lower magnitude in case of integers)
template <typename T>
inline static T div(T a, T b)
{
	return b ? (a / b) : 0;
}

// Return remainder of dividing a by b, but return 0 if b is 0 (requires modulo % operator)
template <typename T>
inline static T mod(T a, T b)
{
	return b ? (a % b) : 0;
}

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

}
