#define NO_UEFI
#include "gtest/gtest.h"

#include <type_traits>

// workaround for name clash with stdlib
#define system qubicSystemStruct

#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include "../src/contract_core/qpi_ticking_impl.h"

TEST(DateAndTimeTest, Equality) {
    DateAndTime d1 = { 500, 30, 15, 8, 3, 3, 24 };
    DateAndTime d2 = { 500, 30, 15, 8, 3, 3, 24 };
    DateAndTime d3 = { 501, 30, 15, 8, 3, 3, 24 }; // Different millisecond
    DateAndTime d4 = { 500, 30, 15, 8, 3, 3, 25 }; // Different year

    EXPECT_EQ(d1, d2);
    EXPECT_EQ(d1, d1);
    EXPECT_NE(d1, d3);
    EXPECT_NE(d1, d4);
}

TEST(DateAndTimeTest, Comparison) {
    DateAndTime base = { 10, 1, 1, 1, 1, 1, 25 }; // Jan 1, 2025

    EXPECT_LT((DateAndTime{ 10, 1, 1, 1, 1, 1, 24 }), base);
    EXPECT_GT(base, (DateAndTime{ 10, 1, 1, 1, 1, 1, 24 }));

    EXPECT_FALSE(base < base);
    EXPECT_FALSE(base > base);
    EXPECT_TRUE(base == base);
}

TEST(DateAndTimeTest, Subtraction) {
    DateAndTime base = { 0, 0, 0, 8, 15, 3, 25 }; // Mar 15, 2025, 08:00:00.000

    EXPECT_EQ(base - base, 0);
    EXPECT_EQ((DateAndTime{ 0, 0, 0, 8, 16, 3, 25 }) - base, 86400000LL);

    // Test leap year scenarios (2024 is a leap year)
    DateAndTime year_2024 = { 0, 0, 0, 0, 1, 1, 24 };
    DateAndTime year_2025 = { 0, 0, 0, 0, 1, 1, 25 };
    long long leap_year_ms = 366LL * 86400000LL;
    EXPECT_EQ(year_2025 - year_2024, leap_year_ms);
}

TEST(DateAndTimeTest, Addition) {
    DateAndTime base = { 0, 59, 59, 23, 31, 12, 23 }; // 2023-12-31 23:59:59.000

    // Add 1 second to roll over everything to the next year
    DateAndTime expected_new_year = { 0, 0, 0, 0, 1, 1, 24 };
    EXPECT_EQ(base + 1000LL, expected_new_year);

    // Add 0
    EXPECT_EQ(base + 0LL, base);

    // Add 1 millisecond
    DateAndTime expected_1ms = { 1, 59, 59, 23, 31, 12, 23 };
    EXPECT_EQ(base + 1LL, expected_1ms);

    // Test adding across a leap day
    // 2024 is a leap year
    DateAndTime before_leap_day = { 0, 0, 0, 0, 28, 2, 24 };
    long long two_days_ms = 2 * 86400000LL;
    DateAndTime after_leap_day = { 0, 0, 0, 0, 1, 3, 24 };
    EXPECT_EQ(before_leap_day + two_days_ms, after_leap_day);

    // Test adding a full common year's worth of milliseconds
    DateAndTime start_of_2025 = { 0, 0, 0, 0, 1, 1, 25 };
    DateAndTime start_of_2026 = { 0, 0, 0, 0, 1, 1, 26 };
    long long common_year_ms = 365LL * 86400000LL;
    EXPECT_EQ(start_of_2025 + common_year_ms, start_of_2026);
}