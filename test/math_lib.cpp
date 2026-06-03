#include "gtest/gtest.h"

#include <cmath>
#include "../src/contracts/math_lib.h"

TEST(TestCoreMathLib, Max) {
    EXPECT_EQ(math_lib::max(0, 0), 0);
    EXPECT_EQ(math_lib::max(10, 0), 10);
    EXPECT_EQ(math_lib::max(0, 20), 20);
    EXPECT_EQ(math_lib::max(-10, 0), 0);
    EXPECT_EQ(math_lib::max(0, -20), 0);

    EXPECT_EQ(math_lib::max(0.0, 0.0), 0.0);
    EXPECT_EQ(math_lib::max(10.0, 0.0), 10.0);
    EXPECT_EQ(math_lib::max(0.0, 20.0), 20.0);
    EXPECT_EQ(math_lib::max(-10.0, 0.0), 0.0);
    EXPECT_EQ(math_lib::max(0.0, -20.0), 0.0);
}

TEST(TestCoreMathLib, Min) {
    EXPECT_EQ(math_lib::min(0, 0), 0);
    EXPECT_EQ(math_lib::min(10, 0), 0);
    EXPECT_EQ(math_lib::min(0, 20), 0);
    EXPECT_EQ(math_lib::min(-10, 0), -10);
    EXPECT_EQ(math_lib::min(0, -20), -20);

    EXPECT_EQ(math_lib::min(0.0, 0.0), 0.0);
    EXPECT_EQ(math_lib::min(10.0, 0.0), 0.0);
    EXPECT_EQ(math_lib::min(0.0, 20.0), 0.0);
    EXPECT_EQ(math_lib::min(-10.0, 0.0), -10.0);
    EXPECT_EQ(math_lib::min(0.0, -20.0), -20.0);
}

TEST(TestCoreMathLib, Abs) {
    EXPECT_EQ(math_lib::abs(0), 0);
    EXPECT_EQ(math_lib::abs(10), 10);
    EXPECT_EQ(math_lib::abs(-1110), 1110);
    EXPECT_EQ(math_lib::abs(-987654ll), 987654llu);

    EXPECT_EQ(math_lib::abs(0.0), 0.0);
    EXPECT_EQ(math_lib::abs(-0.0), 0.0);
    EXPECT_EQ(math_lib::abs(-1230.0f), 1230.0f);
    EXPECT_EQ(math_lib::abs(INFINITY), INFINITY);
    EXPECT_EQ(math_lib::abs(-INFINITY), INFINITY);
    EXPECT_EQ(math_lib::abs(FP_NAN), FP_NAN);
}

// div() and mod() are defined in qpi.h

template <typename T>
void testDivUp()
{
    EXPECT_EQ(int(math_lib::divUp(T(0), T(0))), 0);
    EXPECT_EQ(int(math_lib::divUp(T(1), T(0))), 0);
    EXPECT_EQ(int(math_lib::divUp(T(0), T(1))), 0);
    EXPECT_EQ(int(math_lib::divUp(T(1), T(1))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(2), T(1))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(3), T(1))), 3);
    EXPECT_EQ(int(math_lib::divUp(T(0), T(2))), 0);
    EXPECT_EQ(int(math_lib::divUp(T(1), T(2))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(2), T(2))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(3), T(2))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(4), T(2))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(5), T(2))), 3);
    EXPECT_EQ(int(math_lib::divUp(T(0), T(3))), 0);
    EXPECT_EQ(int(math_lib::divUp(T(1), T(3))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(2), T(3))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(3), T(3))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(4), T(3))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(5), T(3))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(6), T(3))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(7), T(3))), 3);
    EXPECT_EQ(int(math_lib::divUp(T(20), T(19))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(20), T(20))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(20), T(21))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(20), T(22))), 1);
    EXPECT_EQ(int(math_lib::divUp(T(50), T(24))), 3);
    EXPECT_EQ(int(math_lib::divUp(T(50), T(25))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(50), T(26))), 2);
    EXPECT_EQ(int(math_lib::divUp(T(50), T(27))), 2);
}

TEST(TestCoreMathLib, DivUp) {
    testDivUp<unsigned char>();
    testDivUp<unsigned short>();
    testDivUp<unsigned int>();
    testDivUp<unsigned long long>();
}

template <typename T>
void testPositiveGreatestCommonDivisor()
{
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(0), T(0)), 0);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(0), T(1)), 1);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(1), T(0)), 1);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(2), T(20)), 2);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(20), T(2)), 2);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(12), T(18)), 6);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(18), T(12)), 6);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(36), T(27)), 9);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(27), T(36)), 9);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(30), T(30)), 30);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(30), T(30)), 30);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(99), T(100)), 1);
    EXPECT_EQ((int)math_lib::greatestCommonDivisor(T(100), T(99)), 1);
}

TEST(TestCoreMathLib, GreatestCommonDivisor)
{
    testPositiveGreatestCommonDivisor<unsigned char>();
    testPositiveGreatestCommonDivisor<unsigned short>();
    testPositiveGreatestCommonDivisor<unsigned int>();
    testPositiveGreatestCommonDivisor<unsigned long long>();
    EXPECT_EQ(math_lib::greatestCommonDivisor<int>(-1, -1), 0);
    EXPECT_EQ(math_lib::greatestCommonDivisor<int>(-8, -12), 0);
    EXPECT_EQ(math_lib::greatestCommonDivisor<int>(-8, 12), 0);
    EXPECT_EQ(math_lib::greatestCommonDivisor<int>(8, -12), 0);
    EXPECT_EQ(math_lib::greatestCommonDivisor<int>(-12, -18), 0);
}

template<unsigned int k>
static unsigned long long floorRootRef(unsigned long long n)
{
    if (n < 2) 
    { 
        return n; 
    }
    double est;
    if constexpr (k == 2)
    { 
        est = std::sqrt(static_cast<double>(n));
    }
    else 
    { 
        est = std::pow(static_cast<double>(n), 1.0 / static_cast<double>(k));
    }
    unsigned long long r = static_cast<unsigned long long>(est);
    // double overshot -> down
    while (r > 0 && !math_lib::powerLessOrEqual(r, k, n))
    { 
        r--; 
    }
    // double undershot -> up
    while (r < 0xFFFFFFFFFFFFFFFFULL && math_lib::powerLessOrEqual(r + 1, k, n))
    { 
        r++; 
    }
    return r;
}

template<unsigned int k>
static void expectRoot(unsigned long long n)
{
    EXPECT_EQ(math_lib::irootK64<k>(n), floorRootRef<k>(n)) << "k=" << k << " n=" << n;
}

TEST(TestCoreMathLib, BitLength)
{
    EXPECT_EQ(math_lib::bitLength(0), 0u);
    EXPECT_EQ(math_lib::bitLength(1), 1u);
    EXPECT_EQ(math_lib::bitLength(2), 2u);
    EXPECT_EQ(math_lib::bitLength(3), 2u);
    EXPECT_EQ(math_lib::bitLength(4), 3u);
    EXPECT_EQ(math_lib::bitLength(255), 8u);
    EXPECT_EQ(math_lib::bitLength(256), 9u);
    EXPECT_EQ(math_lib::bitLength(1ULL << 63), 64u);
    EXPECT_EQ(math_lib::bitLength(0xFFFFFFFFFFFFFFFFULL), 64u);
}

TEST(TestCoreMathLib, PowerLessOrEqual)
{
    EXPECT_TRUE(math_lib::powerLessOrEqual(2, 3, 8));      // 2^3 == 8
    EXPECT_FALSE(math_lib::powerLessOrEqual(2, 3, 7));      // 2^3 > 7
    EXPECT_TRUE(math_lib::powerLessOrEqual(0, 0, 5));      // 0^0 == 1
    EXPECT_FALSE(math_lib::powerLessOrEqual(0, 0, 0));      // 0^0 == 1 > 0
    EXPECT_TRUE(math_lib::powerLessOrEqual(0, 3, 5));      // 0^3 == 0
    EXPECT_TRUE(math_lib::powerLessOrEqual(1, 100, 1));    // 1^100 == 1
    EXPECT_TRUE(math_lib::powerLessOrEqual(2, 63, 0xFFFFFFFFFFFFFFFFULL));   // 2^63 <= u64max
    EXPECT_FALSE(math_lib::powerLessOrEqual(2, 64, 0xFFFFFFFFFFFFFFFFULL));   // 2^64 > u64max (no overflow in check)
    EXPECT_FALSE(math_lib::powerLessOrEqual(1000000, 4, 0xFFFFFFFFFFFFFFFFULL)); // 1e24 > u64max
}

TEST(TestCoreMathLib, IRootVsFloorDouble)
{
    const int testCount = 10;
    unsigned long long s[testCount] = {1000, 541, 475, 9978, 1234, 45, 0, 1, 7, 65};
    for (int i = 0; i < testCount; i++)
    {
        const unsigned long long n = s[i];
        expectRoot<2>(n);
        expectRoot<3>(n);
        expectRoot<4>(n);
        expectRoot<6>(n);
        expectRoot<8>(n);
    }
    expectRoot<2>(0xFFFFFFFFFFFFFFFFULL);
    expectRoot<3>(0xFFFFFFFFFFFFFFFFULL);
    expectRoot<8>(0xFFFFFFFFFFFFFFFFULL);
}
