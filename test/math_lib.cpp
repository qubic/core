#include "gtest/gtest.h"

#include "../src/smart_contracts/math_lib.h"

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
