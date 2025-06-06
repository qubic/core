#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/platform/m256.h"

#include <chrono>


TEST(TestCore256BitClass, ConstructAssignCompare) {
    // Basic construction, comparison, and assignment
    unsigned char buffer_u8[32];
    for (int8_t i = 0; i < 32; ++i)
        buffer_u8[i] = 3 + 2 * i;

    m256i v1(buffer_u8);
    for (int8_t i = 0; i < 32; ++i)
        EXPECT_EQ(v1.m256i_u8[i], 3 + 2 * i);
    EXPECT_TRUE(v1 == buffer_u8);
    EXPECT_FALSE(v1 != buffer_u8);
    EXPECT_FALSE(isZero(v1));

    for (int8_t i = 0; i < 32; ++i)
        buffer_u8[i] = 250 - 3 * i;

    for (int8_t i = 0; i < 32; ++i)
        EXPECT_EQ(v1.m256i_u8[i], 3 + 2 * i);
    EXPECT_TRUE(v1 != buffer_u8);
    EXPECT_FALSE(v1 == buffer_u8);
    EXPECT_FALSE(isZero(v1));

    m256i v2(buffer_u8);
    for (int8_t i = 0; i < 32; ++i)
        EXPECT_EQ(v2.m256i_u8[i], 250 - 3 * i);
    EXPECT_TRUE(v1 != v2);
    EXPECT_FALSE(v1 == v2);
    EXPECT_FALSE(isZero(v2));

    m256i v3(v1);
    for (int8_t i = 0; i < 32; ++i)
        EXPECT_EQ(v3.m256i_u8[i], 3 + 2 * i);
    EXPECT_TRUE(v1 == v3);
    EXPECT_FALSE(v1 != v3);
    EXPECT_FALSE(isZero(v3));

    __m256i buffer_intr;
    unsigned char* bytes_of_buffer_intr = reinterpret_cast<unsigned char*>(&buffer_intr);
    for (uint8_t i = 0; i < 32; ++i) {
        bytes_of_buffer_intr[i] = 90 + i;
    }

    m256i v4(buffer_intr);
    for (int8_t i = 0; i < 32; ++i)
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

    v5.m256i_i64[0] = 0;
    v5.m256i_i64[1] = 0;
    v5.m256i_i64[2] = 0;
    v5.m256i_i64[3] = 0;
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
    m256i v1_original_state = v1; // Capture original state for later comparison
    m256i v5_original_state = v5;

    SUPPRESS_WARNINGS_BEGIN
    IGNORE_SELFASSIGNMENT_WARNING
    v1 = v1; // This line is intentionally testing self-assignment
    SUPPRESS_WARNINGS_END

    EXPECT_TRUE(v1 == v1_original_state);
    EXPECT_TRUE(v1 == v1);
    EXPECT_FALSE(v1 != v1);
    v5 = v5;
    EXPECT_TRUE(v5 == v5_original_state);
    EXPECT_TRUE(v5 == v5);
    EXPECT_FALSE(v5 != v5);

    // Non-aligned assignment and comparison
    unsigned char buffer_u8_64[64];
    for (int8_t i = 0; i < 64; ++i)
        buffer_u8_64[i] = 7 + i;
    for (int8_t i = 0; i < 32; ++i)
    {
        v1 = buffer_u8_64 + i;
        EXPECT_TRUE(v1 == buffer_u8_64 + i);
        EXPECT_FALSE(v1 != buffer_u8_64 + i);
        EXPECT_FALSE(isZero(v1));
        for (int j = 0; j < 32; ++j)
            EXPECT_EQ(v1.m256i_u8[j], 7 + i + j);
    }

    // m256i::zero()
    EXPECT_FALSE(isZero(v1));
    EXPECT_FALSE(v1 == m256i::zero());
    EXPECT_TRUE(v1 != m256i::zero());
    v1 = m256i::zero();
    EXPECT_TRUE(isZero(v1));
    EXPECT_TRUE(v1 == m256i::zero());
    EXPECT_FALSE(v1 != m256i::zero());

    // 4 x u64 constructor
    m256i v7(1234, 5678, 9012, 3456);
    EXPECT_EQ(v7.u64._0, 1234);
    EXPECT_EQ(v7.u64._1, 5678);
    EXPECT_EQ(v7.u64._2, 9012);
    EXPECT_EQ(v7.u64._3, 3456);
    EXPECT_EQ(v7.m256i_u64[0], 1234);
    EXPECT_EQ(v7.m256i_u64[1], 5678);
    EXPECT_EQ(v7.m256i_u64[2], 9012);
    EXPECT_EQ(v7.m256i_u64[3], 3456);
}

TEST(TestCore256BitFunctionsIntrinsicType, isZero) {
    EXPECT_TRUE(isZero(m256i(0, 0, 0, 0).getIntrinsicValue()));
    EXPECT_FALSE(isZero(m256i(1, 0, 0, 0).getIntrinsicValue()));
    EXPECT_FALSE(isZero(m256i(0, 1, 0, 0).getIntrinsicValue()));
    EXPECT_FALSE(isZero(m256i(0, 0, 1, 0).getIntrinsicValue()));
    EXPECT_FALSE(isZero(m256i(0, 0, 0, 1).getIntrinsicValue()));
    EXPECT_FALSE(isZero(m256i(0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff).getIntrinsicValue()));
}

TEST(TestCore256BitFunctionsIntrinsicType, isZeroPerformance)
{
    constexpr int N = 50000000;
    volatile m256i optimizeBarrierValue(0, 0, 0, 0);
    m256i value;
    [[maybe_unused]] volatile bool optimizeBarrierResult;

    // measure isZero
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i)
    {
        optimizeBarrierValue.i64._0 = i;
        value = optimizeBarrierValue;
        optimizeBarrierResult = isZero(value);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << N << " x isZero: " << ms << " milliseconds" << std::endl;

    // measure comparison with existing zero m256i
    t0 = std::chrono::high_resolution_clock::now();
    m256i zero = m256i::zero();
    for (int i = 0; i < N; ++i)
    {
        optimizeBarrierValue.i64._0 = i;
        value = optimizeBarrierValue;
        optimizeBarrierResult = (value == zero);
    }
    t1 = std::chrono::high_resolution_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << N << " x compare with existing zero instance: " << ms << " milliseconds" << std::endl;

    // measure comparison with new zero m256i
    t0 = std::chrono::high_resolution_clock::now();
    volatile m256i zeroVol;
    for (int i = 0; i < N; ++i)
    {
        optimizeBarrierValue.i64._0 = i;
        value = optimizeBarrierValue;
        zeroVol = m256i::zero();
        optimizeBarrierResult = (value == zeroVol);
    }
    t1 = std::chrono::high_resolution_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << N << " x compare with new zero instance: " << ms << " milliseconds" << std::endl;
}


TEST(TestCore256BitFunctions, operatorEqual) {
    EXPECT_TRUE(m256i(0, 0, 0, 0)  == m256i(0, 0, 0, 0));
    EXPECT_FALSE(m256i(0, 0, 0, 0) == m256i(0, 0, 0, 1));
    EXPECT_FALSE(m256i(0, 0, 0, 0) == m256i(0, 0, 1, 0));
    EXPECT_FALSE(m256i(0, 0, 0, 0) == m256i(0, 1, 0, 0));
    EXPECT_FALSE(m256i(0, 0, 0, 0) == m256i(1, 0, 0, 0));

    EXPECT_TRUE(m256i(42, 42, 42, 42) == m256i(42, 42, 42, 42));
    EXPECT_FALSE(m256i(0, 42, 42, 42) == m256i(42, 42, 42, 42));
    EXPECT_FALSE(m256i(42, 0, 42, 42) == m256i(42, 42, 42, 42));
    EXPECT_FALSE(m256i(42, 42, 0, 42) == m256i(42, 42, 42, 42));
    EXPECT_FALSE(m256i(42, 42, 42, 0) == m256i(42, 42, 42, 42));
}

TEST(TestCore256BitFunctions, operatorNotEqual) {
    EXPECT_FALSE(m256i(0, 0, 0, 0) != m256i(0, 0, 0, 0));
    EXPECT_TRUE(m256i(0, 0, 0, 0)  != m256i(0, 0, 0, 1));
    EXPECT_TRUE(m256i(0, 0, 0, 0)  != m256i(0, 0, 1, 0));
    EXPECT_TRUE(m256i(0, 0, 0, 0)  != m256i(0, 1, 0, 0));
    EXPECT_TRUE(m256i(0, 0, 0, 0)  != m256i(1, 0, 0, 0));

    EXPECT_FALSE(m256i(42, 42, 42, 42) != m256i(42, 42, 42, 42));
    EXPECT_TRUE(m256i(0, 42, 42, 42)   != m256i(42, 42, 42, 42));
    EXPECT_TRUE(m256i(42, 0, 42, 42)   != m256i(42, 42, 42, 42));
    EXPECT_TRUE(m256i(42, 42, 0, 42)   != m256i(42, 42, 42, 42));
    EXPECT_TRUE(m256i(42, 42, 42, 0)   != m256i(42, 42, 42, 42));
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
