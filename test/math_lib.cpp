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

TEST(TestCoreMathLib, Div) {
    EXPECT_EQ(math_lib::div(0, 0), 0);
    EXPECT_EQ(math_lib::div(10, 0), 0);
    EXPECT_EQ(math_lib::div(0, 10), 0);
    EXPECT_EQ(math_lib::div(20, 19), 1);
    EXPECT_EQ(math_lib::div(20, 20), 1);
    EXPECT_EQ(math_lib::div(20, 21), 0);
    EXPECT_EQ(math_lib::div(20, 22), 0);
    EXPECT_EQ(math_lib::div(50, 24), 2);
    EXPECT_EQ(math_lib::div(50, 25), 2);
    EXPECT_EQ(math_lib::div(50, 26), 1);
    EXPECT_EQ(math_lib::div(50, 27), 1);
    EXPECT_EQ(math_lib::div(-2, 0), 0);
    EXPECT_EQ(math_lib::div(-2, 1), -2);
    EXPECT_EQ(math_lib::div(-2, 2), -1);
    EXPECT_EQ(math_lib::div(-2, 3), 0);
    EXPECT_EQ(math_lib::div(2, -3), 0);
    EXPECT_EQ(math_lib::div(2, -2), -1);
    EXPECT_EQ(math_lib::div(2, -1), -2);

    EXPECT_EQ(math_lib::div(0.0, 0.0), 0.0);
    EXPECT_EQ(math_lib::div(50.0, 0.0), 0.0);
    EXPECT_EQ(math_lib::div(0.0, 50.0), 0.0);
    EXPECT_EQ(math_lib::div(-25.0, 50.0), -0.5);
    EXPECT_EQ(math_lib::div(-25.0, -0.5), 50.0);
}

TEST(TestCoreMathLib, Mod) {
    EXPECT_EQ(math_lib::mod(0, 0), 0);
    EXPECT_EQ(math_lib::mod(10, 0), 0);
    EXPECT_EQ(math_lib::mod(0, 10), 0);
    EXPECT_EQ(math_lib::mod(20, 19), 1);
    EXPECT_EQ(math_lib::mod(20, 20), 0);
    EXPECT_EQ(math_lib::mod(20, 21), 20);
    EXPECT_EQ(math_lib::mod(20, 22), 20);
    EXPECT_EQ(math_lib::mod(50, 23), 4);
    EXPECT_EQ(math_lib::mod(50, 24), 2);
    EXPECT_EQ(math_lib::mod(50, 25), 0);
    EXPECT_EQ(math_lib::mod(50, 26), 24);
    EXPECT_EQ(math_lib::mod(50, 27), 23);
    EXPECT_EQ(math_lib::mod(-2, 0), 0);
    EXPECT_EQ(math_lib::mod(-2, 1), 0);
    EXPECT_EQ(math_lib::mod(-2, 2), 0);
    EXPECT_EQ(math_lib::mod(-2, 3), -2);
    EXPECT_EQ(math_lib::mod(2, -3), 2);
    EXPECT_EQ(math_lib::mod(2, -2), 0);
    EXPECT_EQ(math_lib::mod(2, -1), 0);
}

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
