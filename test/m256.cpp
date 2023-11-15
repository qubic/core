#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/platform/m256.h"
#include "uc32.h"

TEST(TestCore256BitFunctions, isZero) {
    EXPECT_TRUE(isZero(UC32x(0, 0, 0, 0).m256i));
    EXPECT_FALSE(isZero(UC32x(1, 0, 0, 0).m256i));
    EXPECT_FALSE(isZero(UC32x(0, 1, 0, 0).m256i));
    EXPECT_FALSE(isZero(UC32x(0, 0, 1, 0).m256i));
    EXPECT_FALSE(isZero(UC32x(0, 0, 0, 1).m256i));
    EXPECT_FALSE(isZero(UC32x(-1, -1, -1, -1).m256i));
}

TEST(TestCore256BitFunctions, operatorEqual) {
    EXPECT_TRUE(UC32x(0, 0, 0, 0).m256i == UC32x(0, 0, 0, 0).m256i);
    EXPECT_FALSE(UC32x(0, 0, 0, 0).m256i == UC32x(0, 0, 0, 1).m256i);
    EXPECT_FALSE(UC32x(0, 0, 0, 0).m256i == UC32x(0, 0, 1, 0).m256i);
    EXPECT_FALSE(UC32x(0, 0, 0, 0).m256i == UC32x(0, 1, 0, 0).m256i);
    EXPECT_FALSE(UC32x(0, 0, 0, 0).m256i == UC32x(1, 0, 0, 0).m256i);

    EXPECT_TRUE(UC32x(42, 42, 42, 42).m256i == UC32x(42, 42, 42, 42).m256i);
    EXPECT_FALSE(UC32x(0, 42, 42, 42).m256i == UC32x(42, 42, 42, 42).m256i);
    EXPECT_FALSE(UC32x(42, 0, 42, 42).m256i == UC32x(42, 42, 42, 42).m256i);
    EXPECT_FALSE(UC32x(42, 42, 0, 42).m256i == UC32x(42, 42, 42, 42).m256i);
    EXPECT_FALSE(UC32x(42, 42, 42, 0).m256i == UC32x(42, 42, 42, 42).m256i);
}

TEST(TestCore256BitFunctions, operatorNotEqual) {
    EXPECT_FALSE(UC32x(0, 0, 0, 0).m256i != UC32x(0, 0, 0, 0).m256i);
    EXPECT_TRUE(UC32x(0, 0, 0, 0).m256i != UC32x(0, 0, 0, 1).m256i);
    EXPECT_TRUE(UC32x(0, 0, 0, 0).m256i != UC32x(0, 0, 1, 0).m256i);
    EXPECT_TRUE(UC32x(0, 0, 0, 0).m256i != UC32x(0, 1, 0, 0).m256i);
    EXPECT_TRUE(UC32x(0, 0, 0, 0).m256i != UC32x(1, 0, 0, 0).m256i);

    EXPECT_FALSE(UC32x(42, 42, 42, 42).m256i != UC32x(42, 42, 42, 42).m256i);
    EXPECT_TRUE(UC32x(0, 42, 42, 42).m256i != UC32x(42, 42, 42, 42).m256i);
    EXPECT_TRUE(UC32x(42, 0, 42, 42).m256i != UC32x(42, 42, 42, 42).m256i);
    EXPECT_TRUE(UC32x(42, 42, 0, 42).m256i != UC32x(42, 42, 42, 42).m256i);
    EXPECT_TRUE(UC32x(42, 42, 42, 0).m256i != UC32x(42, 42, 42, 42).m256i);
}
