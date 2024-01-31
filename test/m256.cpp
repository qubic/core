#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/platform/m256.h"


TEST(TestCore256BitClass, ConstructAssignCompare) {
    // Basic construction, comparison, and assignment
    unsigned char buffer_u8[32];
    for (int i = 0; i < 32; ++i)
        buffer_u8[i] = 3 + 2 * i;

    m256i v1(buffer_u8);
    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(v1.m256i_u8[i], 3 + 2 * i);
    EXPECT_TRUE(v1 == buffer_u8);
    EXPECT_FALSE(v1 != buffer_u8);
    EXPECT_FALSE(isZero(v1));

    for (int i = 0; i < 32; ++i)
        buffer_u8[i] = 250 - 3 * i;

    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(v1.m256i_u8[i], 3 + 2 * i);
    EXPECT_TRUE(v1 != buffer_u8);
    EXPECT_FALSE(v1 == buffer_u8);
    EXPECT_FALSE(isZero(v1));

    m256i v2(buffer_u8);
    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(v2.m256i_u8[i], 250 - 3 * i);
    EXPECT_TRUE(v1 != v2);
    EXPECT_FALSE(v1 == v2);
    EXPECT_FALSE(isZero(v2));

    m256i v3(v1);
    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(v3.m256i_u8[i], 3 + 2 * i);
    EXPECT_TRUE(v1 == v3);
    EXPECT_FALSE(v1 != v3);
    EXPECT_FALSE(isZero(v3));

    __m256i buffer_intr;
    for (int i = 0; i < 32; ++i)
        buffer_intr.m256i_u8[i] = 90 + i;

    m256i v4(buffer_intr);
    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(v4.m256i_u8[i], 90 + i);
    EXPECT_TRUE(v4 == buffer_intr);
    EXPECT_FALSE(v4 != buffer_intr);
    EXPECT_TRUE(v4 != v3);
    EXPECT_FALSE(v4 == v3);
    EXPECT_TRUE(v4 != v2);
    EXPECT_FALSE(v4 == v2);
    EXPECT_TRUE(v4 != v1);
    EXPECT_FALSE(v4 == v1);
    EXPECT_FALSE(isZero(v4));

    // Construction, comparison, and assignment involving volatile
    volatile m256i v5(v4);
    EXPECT_TRUE(v4 == v5);
    EXPECT_FALSE(v4 != v5);
    EXPECT_FALSE(isZero(v5));

    m256i v6(v5);
    EXPECT_TRUE(v6 == v5);
    EXPECT_FALSE(v6 != v5);
    EXPECT_FALSE(isZero(v6));

    EXPECT_TRUE(v1 != v5);
    EXPECT_FALSE(v1 == v5);
    v1 = v5;
    EXPECT_TRUE(v1 == v5);
    EXPECT_FALSE(v1 != v5);
    EXPECT_FALSE(isZero(v1));

    EXPECT_TRUE(v2 != v5);
    EXPECT_FALSE(v2 == v5);
    v5 = v2;
    EXPECT_TRUE(v2 == v5);
    EXPECT_FALSE(v2 != v5);
    EXPECT_FALSE(isZero(v2));

    v5.m256i_i64[0] = v5.m256i_i64[1] = v5.m256i_i64[2] = v5.m256i_i64[3] = 0;
    EXPECT_TRUE(isZero(v5));
    EXPECT_TRUE(v2 != v5);
    EXPECT_FALSE(v2 == v5);
    v2 = v5;
    EXPECT_TRUE(v2 == v5);
    EXPECT_FALSE(v2 != v5);
    EXPECT_TRUE(isZero(v2));

    // single-bit difference test
    for (int i = 0; i < 32; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            v2 = v5;
            EXPECT_TRUE(isZero(v2));
            EXPECT_TRUE(v2 == v5);
            EXPECT_FALSE(v2 != v5);
            v2.m256i_u8[i] |= (1 << j);
            EXPECT_FALSE(isZero(v2));
            EXPECT_TRUE(v2 != v5);
            EXPECT_FALSE(v2 == v5);
        }
    }

    // self-assignment and comparison
    v1 = v1;
    EXPECT_TRUE(v1 == v1);
    EXPECT_FALSE(v1 != v1);
    v5 = v5;
    EXPECT_TRUE(v5 == v5);
    EXPECT_FALSE(v5 != v5);

    // Non-aligned assignment and comparison
    unsigned char buffer_u8_64[64];
    for (int i = 0; i < 64; ++i)
        buffer_u8_64[i] = 7 + i;
    for (int i = 0; i < 32; ++i)
    {
        v1 = buffer_u8_64 + i;
        EXPECT_TRUE(v1 == buffer_u8_64 + i);
        EXPECT_FALSE(v1 != buffer_u8_64 + i);
        EXPECT_FALSE(isZero(v1));
        for (int j = 0; j < 32; ++j)
            EXPECT_EQ(v1.m256i_u8[j], 7 + i + j);
    }
}

TEST(TestCore256BitFunctionsIntrinsicType, isZero) {
    EXPECT_TRUE(isZero(m256i(0, 0, 0, 0).m256i_intr()));
    EXPECT_FALSE(isZero(m256i(1, 0, 0, 0).m256i_intr()));
    EXPECT_FALSE(isZero(m256i(0, 1, 0, 0).m256i_intr()));
    EXPECT_FALSE(isZero(m256i(0, 0, 1, 0).m256i_intr()));
    EXPECT_FALSE(isZero(m256i(0, 0, 0, 1).m256i_intr()));
    EXPECT_FALSE(isZero(m256i(-1, -1, -1, -1).m256i_intr()));
}

TEST(TestCore256BitFunctionsIntrinsicType, operatorEqual) {
    EXPECT_TRUE(m256i(0, 0, 0, 0).m256i_intr() == m256i(0, 0, 0, 0).m256i_intr());
    EXPECT_FALSE(m256i(0, 0, 0, 0).m256i_intr() == m256i(0, 0, 0, 1).m256i_intr());
    EXPECT_FALSE(m256i(0, 0, 0, 0).m256i_intr() == m256i(0, 0, 1, 0).m256i_intr());
    EXPECT_FALSE(m256i(0, 0, 0, 0).m256i_intr() == m256i(0, 1, 0, 0).m256i_intr());
    EXPECT_FALSE(m256i(0, 0, 0, 0).m256i_intr() == m256i(1, 0, 0, 0).m256i_intr());

    EXPECT_TRUE(m256i(42, 42, 42, 42).m256i_intr() == m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_FALSE(m256i(0, 42, 42, 42).m256i_intr() == m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_FALSE(m256i(42, 0, 42, 42).m256i_intr() == m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_FALSE(m256i(42, 42, 0, 42).m256i_intr() == m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_FALSE(m256i(42, 42, 42, 0).m256i_intr() == m256i(42, 42, 42, 42).m256i_intr());
}

TEST(TestCore256BitFunctions, operatorNotEqual) {
    EXPECT_FALSE(m256i(0, 0, 0, 0).m256i_intr() != m256i(0, 0, 0, 0).m256i_intr());
    EXPECT_TRUE(m256i(0, 0, 0, 0).m256i_intr() != m256i(0, 0, 0, 1).m256i_intr());
    EXPECT_TRUE(m256i(0, 0, 0, 0).m256i_intr() != m256i(0, 0, 1, 0).m256i_intr());
    EXPECT_TRUE(m256i(0, 0, 0, 0).m256i_intr() != m256i(0, 1, 0, 0).m256i_intr());
    EXPECT_TRUE(m256i(0, 0, 0, 0).m256i_intr() != m256i(1, 0, 0, 0).m256i_intr());

    EXPECT_FALSE(m256i(42, 42, 42, 42).m256i_intr() != m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_TRUE(m256i(0, 42, 42, 42).m256i_intr() != m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_TRUE(m256i(42, 0, 42, 42).m256i_intr() != m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_TRUE(m256i(42, 42, 0, 42).m256i_intr() != m256i(42, 42, 42, 42).m256i_intr());
    EXPECT_TRUE(m256i(42, 42, 42, 0).m256i_intr() != m256i(42, 42, 42, 42).m256i_intr());
}

TEST(TestCore256BitFunctions, operatorLessThan) {
    EXPECT_FALSE(m256i(0, 0, 0, 0) < m256i(0, 0, 0, 0));
    EXPECT_TRUE(m256i(0, 0, 0, 0) < m256i(0, 0, 0, 1));
    EXPECT_TRUE(m256i(0, 0, 0, 0) < m256i(0, 0, 1, 0));
    EXPECT_TRUE(m256i(0, 0, 0, 0) < m256i(0, 1, 0, 0));
    EXPECT_TRUE(m256i(0, 0, 0, 0) < m256i(1, 0, 0, 0));
    EXPECT_FALSE(m256i(0, 0, 0, 1) < m256i(0, 0, 0, 0));
    EXPECT_FALSE(m256i(0, 0, 1, 0) < m256i(0, 0, 0, 0));
    EXPECT_FALSE(m256i(0, 1, 0, 0) < m256i(0, 0, 0, 0));
    EXPECT_FALSE(m256i(1, 0, 0, 0) < m256i(0, 0, 0, 0));

    EXPECT_FALSE(m256i(100, 100, 100, 100) < m256i(100, 100, 100, 100));
    EXPECT_FALSE(m256i(100, 100, 100, 100) < m256i(100, 100, 100, 1));
    EXPECT_FALSE(m256i(100, 100, 100, 100) < m256i(100, 100, 1, 100));
    EXPECT_FALSE(m256i(100, 100, 100, 100) < m256i(100, 1, 100, 100));
    EXPECT_FALSE(m256i(100, 100, 100, 100) < m256i(1, 100, 100, 100));
    EXPECT_TRUE(m256i(100, 100, 100, 1) < m256i(100, 100, 100, 100));
    EXPECT_TRUE(m256i(100, 100, 1, 100) < m256i(100, 100, 100, 100));
    EXPECT_TRUE(m256i(100, 1, 100, 100) < m256i(100, 100, 100, 100));
    EXPECT_TRUE(m256i(1, 100, 100, 100) < m256i(100, 100, 100, 100));
}
