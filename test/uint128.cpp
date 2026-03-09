// Copyright (c) 2013 - 2018 Jason Lee @ calccrypto at gmail.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// https://github.com/calccrypto/uint128_t/blob/master/uint128_t.cpp

#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/platform/uint128.h"


TEST(Uint128Arithmetic, add){
    uint128_t low (0, 1);
    uint128_t high(1, 0);

    EXPECT_EQ(low  + low,  uint128_t(0, 2));
    EXPECT_EQ(low  + high, uint128_t(1, 1));
    EXPECT_EQ(high + high, uint128_t(2, 0));

    EXPECT_EQ(low  += low,  uint128_t(0, 2));
    EXPECT_EQ(low  += high, uint128_t(1, 2));
    EXPECT_EQ(high += low,  uint128_t(2, 2));
}

// https://github.com/calccrypto/uint128_t/blob/master/tests/testcases/sub.cpp
TEST(Uint128Arithmetic, subtract){
    uint128_t big  (0xffffffffffffffffULL, 0xffffffffffffffffULL);
    uint128_t small(1);

    EXPECT_EQ(small - small, uint128_t(0, 0));
    EXPECT_EQ(small - big,   uint128_t(0, 2));
    EXPECT_EQ(big   - small, uint128_t(0xffffffffffffffffULL, 0xfffffffffffffffeULL));
    EXPECT_EQ(big   - big,   uint128_t(0, 0));
}

// https://github.com/calccrypto/uint128_t/blob/master/tests/testcases/div.cpp
TEST(Uint128Arithmetic, divide){
    const uint128_t big_val  (0xfedbca9876543210ULL);
    const uint128_t small_val(0xffffULL);
    const uint128_t res_val  (0xfedcc9753fc9ULL);

    EXPECT_EQ(small_val / small_val, uint128_t(0, 1));
    EXPECT_EQ(small_val / big_val,   uint128_t(0, 0));

    EXPECT_EQ(big_val   / big_val,   uint128_t(0, 1));

    // EXPECT_THROW(uint128_t(1) / uint128_t(0), std::domain_error);
}

TEST(Uint128Arithmetic, multiply){
    uint128_t val(0xfedbca9876543210ULL);

    EXPECT_EQ(val * val, uint128_t(0xfdb8e2bacbfe7cefULL, 0x010e6cd7a44a4100ULL));

    const uint128_t zero = 0;
    EXPECT_EQ(val  * zero, zero);
    EXPECT_EQ(zero * val,  zero);

    const uint128_t one = 1;
    EXPECT_EQ(val * one, val);
    EXPECT_EQ(one * val, val);
}

TEST(Uint128Comparison, equals){
    EXPECT_EQ( (uint128_t(0xdeadbeefULL) == uint128_t(0xdeadbeefULL)), true);
    EXPECT_EQ(!(uint128_t(0xdeadbeefULL) == uint128_t(0xfee1baadULL)), true);
}

TEST(Uint128Comparison, less_than){
    const uint128_t big  (0xffffffffffffffffULL, 0xffffffffffffffffULL);
    const uint128_t small(0x0000000000000000ULL, 0x0000000000000000ULL);

    EXPECT_EQ(small < small, false);
    EXPECT_EQ(small < big,    true);

    EXPECT_EQ(big < small,   false);
    EXPECT_EQ(big < big,     false);
}


TEST(Uint128Comparison, greater_than){
    const uint128_t big  (0xffffffffffffffffULL, 0xffffffffffffffffULL);
    const uint128_t small(0x0000000000000000ULL, 0x0000000000000000ULL);

    EXPECT_EQ(small > small,     false);
    EXPECT_EQ(small > big,       false);

    EXPECT_EQ(big > small,        true);
    EXPECT_EQ(big > big,         false);
}

TEST(Uint128Comparison, greater_than_or_equals){
    const uint128_t big  (0xffffffffffffffffULL, 0xffffffffffffffffULL);
    const uint128_t small(0x0000000000000000ULL, 0x0000000000000000ULL);

    EXPECT_EQ(small >= small,  true);
    EXPECT_EQ(small >= big,   false);

    EXPECT_EQ(big >= small,    true);
    EXPECT_EQ(big >= big,      true);
}

TEST(Uint128Comparison, less_than_or_equals){
    const uint128_t big  (0xffffffffffffffffULL, 0xffffffffffffffffULL);
    const uint128_t small(0x0000000000000000ULL, 0x0000000000000000ULL);

    EXPECT_EQ(small <= small,  true);
    EXPECT_EQ(small <= big,    true);

    EXPECT_EQ(big <= small,   false);
    EXPECT_EQ(big <= big,      true);
}

TEST(Uint128BitShift, left){
    // operator<<
    uint128_t val(0x1);
    uint64_t exp_val = 1;
    for(uint8_t i = 0; i < 64; i++){
        EXPECT_EQ(val << uint128_t(i), uint128_t(exp_val << i));
    }

    uint128_t zero(0);
    for(uint8_t i = 0; i < 64; i++){
        EXPECT_EQ(zero << uint128_t(i), uint128_t(0));
    }

    // operator<<=
    for(uint8_t i = 0; i < 63; i++){ // 1 is already a bit
        EXPECT_EQ(val  <<= uint128_t(1), uint128_t(exp_val <<= 1));
    }

    for(uint8_t i = 0; i < 63; i++){
        EXPECT_EQ(zero <<= uint128_t(1), uint128_t(0));
    }
}

TEST(Uint128BitShift, right){
    // operator>>
    uint128_t val(0xffffffffffffffffULL);
    uint64_t exp = 0xffffffffffffffffULL;
    for(uint8_t i = 0; i < 64; i++){
        EXPECT_EQ(val >> uint128_t(i), uint128_t(exp >> i));
    }

    uint128_t zero(0);
    for(uint8_t i = 0; i < 64; i++){
        EXPECT_EQ(zero >> uint128_t(i), uint128_t(0));
    }

    // operator>>=
    for(uint8_t i = 0; i < 64; i++){
        EXPECT_EQ(val >>= uint128_t(1), uint128_t(exp >>= 1));
    }

    for(uint8_t i = 0; i < 64; i++){
        EXPECT_EQ(zero >>= uint128_t(1), uint128_t(0));
    }
}

TEST(Uint128BitWise, and){
    uint128_t t  ((bool)     true);
    uint128_t f  ((bool)     false);
    uint128_t u8 ((uint8_t)  0xaaULL);
    uint128_t u16((uint16_t) 0xaaaaULL);
    uint128_t u32((uint32_t) 0xaaaaaaaaULL);
    uint128_t u64((uint64_t) 0xaaaaaaaaaaaaaaaaULL);

    const uint128_t val(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL);

    EXPECT_EQ(t   &  val, uint128_t(0));
    EXPECT_EQ(f   &  val, uint128_t(0));
    EXPECT_EQ(u8  &  val, uint128_t(0xa0ULL));
    EXPECT_EQ(u16 &  val, uint128_t(0xa0a0ULL));
    EXPECT_EQ(u32 &  val, uint128_t(0xa0a0a0a0ULL));
    EXPECT_EQ(u64 &  val, uint128_t(0xa0a0a0a0a0a0a0a0ULL));

    EXPECT_EQ(t   &= val, uint128_t(0x0ULL));
    EXPECT_EQ(f   &= val, uint128_t(0x0ULL));
    EXPECT_EQ(u8  &= val, uint128_t(0xa0ULL));
    EXPECT_EQ(u16 &= val, uint128_t(0xa0a0ULL));
    EXPECT_EQ(u32 &= val, uint128_t(0xa0a0a0a0ULL));
    EXPECT_EQ(u64 &= val, uint128_t(0xa0a0a0a0a0a0a0a0ULL));
}