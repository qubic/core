#define NO_UEFI
#include "gtest/gtest.h"

#include <type_traits>

// workaround for name clash with stdlib
#define system qubicSystemStruct

#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include "../src/contract_core/qpi_ticking_impl.h"

#include <random>

::std::ostream& operator<<(::std::ostream& os, const DateAndTime& dt)
{
    std::ios_base::fmtflags f(os.flags());
    os << std::setfill('0') << dt.getYear() << "-"
        << std::setw(2) << (int)dt.getMonth() << "-"
        << std::setw(2) << (int)dt.getDay() << " "
        << std::setw(2) << (int)dt.getHour() << ":"
        << std::setw(2) << (int)dt.getMinute() << ":"
        << std::setw(2) << (int)dt.getSecond() << "."
        << std::setw(3) << dt.getMillisec() << "'"
        << std::setw(3) << dt.getMicrosecDuringMillisec();
    os.flags(f);
    return os;
}

TEST(DateAndTimeTest, IsLeapYear)
{
    EXPECT_TRUE(DateAndTime::isLeapYear(1600));
    EXPECT_FALSE(DateAndTime::isLeapYear(1700));
    EXPECT_FALSE(DateAndTime::isLeapYear(1800));
    EXPECT_FALSE(DateAndTime::isLeapYear(1900));
    EXPECT_TRUE(DateAndTime::isLeapYear(2000));
    EXPECT_FALSE(DateAndTime::isLeapYear(2019));
    EXPECT_TRUE(DateAndTime::isLeapYear(2020));
    EXPECT_FALSE(DateAndTime::isLeapYear(2021));
    EXPECT_FALSE(DateAndTime::isLeapYear(2022));
    EXPECT_FALSE(DateAndTime::isLeapYear(2023));
    EXPECT_TRUE(DateAndTime::isLeapYear(2024));
    EXPECT_FALSE(DateAndTime::isLeapYear(2025));
    EXPECT_FALSE(DateAndTime::isLeapYear(2026));
    EXPECT_FALSE(DateAndTime::isLeapYear(2027));
    EXPECT_TRUE(DateAndTime::isLeapYear(2028));
    EXPECT_FALSE(DateAndTime::isLeapYear(2029));
    EXPECT_FALSE(DateAndTime::isLeapYear(2030));
    EXPECT_FALSE(DateAndTime::isLeapYear(2031));
    EXPECT_TRUE(DateAndTime::isLeapYear(2032));
    EXPECT_FALSE(DateAndTime::isLeapYear(2100));
    EXPECT_FALSE(DateAndTime::isLeapYear(2200));
    EXPECT_TRUE(DateAndTime::isLeapYear(2400));
    EXPECT_FALSE(DateAndTime::isLeapYear(2500));
    EXPECT_TRUE(DateAndTime::isLeapYear(2800));
    EXPECT_TRUE(DateAndTime::isLeapYear(3200));
}

TEST(DateAndTimeTest, IsValid)
{
    // Checking year
    EXPECT_TRUE(DateAndTime::isValid(0, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2100, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(65535, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(65536, 1, 1, 0, 0, 0, 0, 0));

    // Checking month
    EXPECT_FALSE(DateAndTime::isValid(2025, 0, 1, 0, 0, 0, 0, 0));
    for (int i = 1; i <= 12; ++i)
        EXPECT_TRUE(DateAndTime::isValid(2025, i, 1, 0, 0, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 13, 1, 0, 0, 0, 0, 0));

    // Checking day range
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 0, 0, 0, 0, 0, 0));
    for (int i = 1; i <= 31; ++i)
        EXPECT_TRUE(DateAndTime::isValid(2025, 1, i, 0, 0, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 32, 0, 0, 0, 0, 0));

    // Checking last day of month (except Feb)
    int daysPerMonth[] = { 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    for (int i = 0; i < 12; ++i)
    {
        if (daysPerMonth[i])
        {
            EXPECT_TRUE(DateAndTime::isValid(2025, i + 1, daysPerMonth[i], 0, 0, 0, 0, 0));
            EXPECT_FALSE(DateAndTime::isValid(2025, i + 1, daysPerMonth[i] + 1, 0, 0, 0, 0, 0));
        }
    }

    // Checking last day of February
    for (int year = 1582; year < 3000; ++year)
    {
        int daysInFeb = (DateAndTime::isLeapYear(year)) ? 29 : 28;
        EXPECT_TRUE(DateAndTime::isValid(year, 2, daysInFeb, 0, 0, 0, 0, 0));
        EXPECT_FALSE(DateAndTime::isValid(year, 2, daysInFeb + 1, 0, 0, 0, 0, 0));
    }

    // Checking hour
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 3, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 14, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 23, 0, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 24, 0, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 25, 0, 0, 0, 0));

    // Checking minute
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 49, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 59, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 60, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 61, 0, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 101, 0, 0, 0));

    // Checking second
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 49, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 59, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 60, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 61, 0, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 101, 0, 0));

    // Checking millisec
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 999, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 1000, 0));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 1002, 0));

    // Checking microsec
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 999));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 1000));
    EXPECT_FALSE(DateAndTime::isValid(2025, 1, 1, 0, 0, 0, 0, 1002));
}

TEST(DateAndTimeTest, SetAndGet)
{
    // Default constructor (invalid value)
    DateAndTime d1;
    EXPECT_FALSE(d1.isValid());
    EXPECT_EQ((int)d1.getYear(), 0);
    EXPECT_EQ(d1.getMonth(), 0);
    EXPECT_EQ(d1.getDay(), 0);
    EXPECT_EQ(d1.getHour(), 0);
    EXPECT_EQ(d1.getMinute(), 0);
    EXPECT_EQ(d1.getSecond(), 0);
    EXPECT_EQ((int)d1.getMillisec(), 0);
    EXPECT_EQ((int)d1.getMicrosecDuringMillisec(), 0);

    // Set if valid
    EXPECT_FALSE(d1.setIfValid(2025, 0, 0, 0, 0, 0));
    EXPECT_TRUE(d1.setIfValid(2025, 1, 2, 3, 4, 5, 6, 7));
    EXPECT_EQ((int)d1.getYear(), 2025);
    EXPECT_EQ(d1.getMonth(), 1);
    EXPECT_EQ(d1.getDay(), 2);
    EXPECT_EQ(d1.getHour(), 3);
    EXPECT_EQ(d1.getMinute(), 4);
    EXPECT_EQ(d1.getSecond(), 5);
    EXPECT_EQ((int)d1.getMillisec(), 6);
    EXPECT_EQ((int)d1.getMicrosecDuringMillisec(), 7);

    // Copy constructor
    DateAndTime d2(d1);
    EXPECT_EQ((int)d2.getYear(), 2025);
    EXPECT_EQ(d2.getMonth(), 1);
    EXPECT_EQ(d2.getDay(), 2);
    EXPECT_EQ(d2.getHour(), 3);
    EXPECT_EQ(d2.getMinute(), 4);
    EXPECT_EQ(d2.getSecond(), 5);
    EXPECT_EQ((int)d2.getMillisec(), 6);
    EXPECT_EQ((int)d2.getMicrosecDuringMillisec(), 7);

    // Set edge case values (invalid as date but good for checking if
    // bit-level processing works)
    d2.set(65535, 15, 31, 31, 63, 63, 1023, 1023);
    EXPECT_FALSE(d2.isValid());
    EXPECT_EQ((int)d2.getYear(), 65535);
    EXPECT_EQ(d2.getMonth(), 15);
    EXPECT_EQ(d2.getDay(), 31);
    EXPECT_EQ(d2.getHour(), 31);
    EXPECT_EQ(d2.getMinute(), 63);
    EXPECT_EQ(d2.getSecond(), 63);
    EXPECT_EQ((int)d2.getMillisec(), 1023);
    EXPECT_EQ((int)d2.getMicrosecDuringMillisec(), 1023);

    // Operator =
    EXPECT_NE(d1, d2);
    d2 = d1;
    EXPECT_EQ(d1, d2);

    // Test setTime() and setDate(), which runs without checking validity
    EXPECT_EQ(d1, DateAndTime(2025, 1, 2, 3, 4, 5, 6, 7));
    d1.setDate(65535, 15, 31);
    EXPECT_EQ(d1, DateAndTime(65535, 15, 31, 3, 4, 5, 6, 7));
    d1.setTime(31, 63, 63, 1023, 1023);
    EXPECT_EQ(d1, DateAndTime(65535, 15, 31, 31, 63, 63, 1023, 1023));
    d1.setDate(2030, 7, 10);
    EXPECT_EQ(d1, DateAndTime(2030, 7, 10, 31, 63, 63, 1023, 1023));
    d1.setTime(20, 15, 16, 457, 738);
    EXPECT_EQ(d1, DateAndTime(2030, 7, 10, 20, 15, 16, 457, 738));
}

TEST(DateAndTimeTest, Equality)
{
    DateAndTime d1{ 2024, 3, 3, 8, 15, 30, 500 }; // 2024-03-03 8:15:30.500
    DateAndTime d2{ 2024, 3, 3, 8, 15, 30, 500 };
    DateAndTime d3{ 2024, 3, 3, 8, 15, 30, 501 }; // Different millisecond
    DateAndTime d4{ 2025, 3, 3, 8, 15, 30, 500 }; // Different year

    EXPECT_EQ(d1, d2);
    EXPECT_EQ(d1, d1);
    EXPECT_NE(d1, d3);
    EXPECT_NE(d1, d4);
}

TEST(DateAndTimeTest, Comparison)
{
    DateAndTime sorted[] = {
        { 2024, 6, 1, 0, 0, 0 },          // June 1, 2024, 00:00:00.0
        { 2025, 5, 1, 0, 0, 0 },          // May 1, 2025, 00:00:00.0
        { 2025, 5, 29, 12, 0, 0 },
        { 2025, 6, 1, 10, 0, 0 },
        { 2025, 6, 1, 10, 10, 0 },
        { 2025, 6, 1, 10, 10, 10 },
        { 2025, 6, 1, 12, 0, 0 },         // June 1, 2025, 12:00:00.0
        { 2025, 6, 1, 12, 0, 0, 0, 999 }, // June 1, 2025, 12:00:00.000999
        { 2025, 6, 1, 12, 0, 0, 1, 0 },   // June 1, 2025, 12:00:00.001000
        { 2025, 6, 1, 12, 0, 1 },         // June 1, 2025, 12:00:01.0
        { 2025, 6, 1, 12, 1, 0 },         // June 1, 2025, 12:01:00.0
        { 2025, 6, 1, 13, 0, 0 },         // June 1, 2025, 13:01:00.0
        { 2025, 6, 2, 10, 0, 0 },         // June 2, 2025, 10:00:00.0
        { 2025, 7, 1, 10, 0, 0 },         // July 1, 2025, 10:00:00.0
        { 2026, 6, 1, 10, 0, 0 },         // June 1, 2026, 10:00:00.0
    };
    const int count = sizeof(sorted) / sizeof(sorted[0]);

    for (int i = 0; i < count; ++i)
    {
        for (int j = i; j < count; ++j)
        {
            if (i == j)
            {
                EXPECT_EQ(sorted[i], sorted[j]);
            }
            else
            {
                EXPECT_LT(sorted[i], sorted[j]);
                EXPECT_GT(sorted[j], sorted[i]);
            }
        }
    }
}

TEST(DateAndTimeTest, Addition)
{
    // changing date only (using day count)
    DateAndTime d1{ 2025, 1, 1 };
    EXPECT_TRUE(d1.add(0, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1));
    EXPECT_TRUE(d1.add(0, 0, 10));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 11));
    EXPECT_TRUE(d1.add(0, 0, -6));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 5));
    EXPECT_TRUE(d1.add(0, 0, 26));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 31));
    EXPECT_TRUE(d1.add(0, 0, 15));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 15));
    EXPECT_TRUE(d1.add(0, 0, 15));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 2));
    EXPECT_TRUE(d1.add(0, 0, -2));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, -13));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 15));
    EXPECT_TRUE(d1.add(0, 0, -20));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 26));
    EXPECT_TRUE(d1.add(0, 0, -27));
    EXPECT_EQ(d1, DateAndTime(2024, 12, 30));
    EXPECT_TRUE(d1.add(0, 0, 31));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 30));
    EXPECT_TRUE(d1.add(0, 0, 31));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 2));
    EXPECT_TRUE(d1.add(0, 0, -31));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 30));
    EXPECT_TRUE(d1.add(0, 0, 52));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 23));
    EXPECT_TRUE(d1.add(0, 0, 70));
    EXPECT_EQ(d1, DateAndTime(2025, 6, 1));
    EXPECT_TRUE(d1.add(0, 0, -122));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 30));
    EXPECT_TRUE(d1.add(0, 0, 132));
    EXPECT_EQ(d1, DateAndTime(2025, 6, 11));
    EXPECT_TRUE(d1.add(0, 0, -162));
    EXPECT_EQ(d1, DateAndTime(2024, 12, 31));
    EXPECT_TRUE(d1.add(0, 0, -35));
    EXPECT_EQ(d1, DateAndTime(2024, 11, 26));
    EXPECT_TRUE(d1.add(0, 0, -25));
    EXPECT_EQ(d1, DateAndTime(2024, 11, 1));
    EXPECT_TRUE(d1.add(0, 0, 60));
    EXPECT_EQ(d1, DateAndTime(2024, 12, 31));
    EXPECT_TRUE(d1.add(0, 0, -140));
    EXPECT_EQ(d1, DateAndTime(2024, 8, 13));
    EXPECT_TRUE(d1.add(0, 0, -49));
    EXPECT_EQ(d1, DateAndTime(2024, 6, 25));
    EXPECT_TRUE(d1.add(0, 0, 190));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1));
    EXPECT_TRUE(d1.add(0, 0, -200));
    EXPECT_EQ(d1, DateAndTime(2024, 6, 15));
    EXPECT_TRUE(d1.add(0, 0, 220));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 21));
    EXPECT_TRUE(d1.add(0, 0, -220));
    EXPECT_EQ(d1, DateAndTime(2024, 6, 15));
    EXPECT_TRUE(d1.add(0, 0, -21));
    EXPECT_EQ(d1, DateAndTime(2024, 5, 25));
    EXPECT_TRUE(d1.add(0, 0, -50));
    EXPECT_EQ(d1, DateAndTime(2024, 4, 5));
    EXPECT_TRUE(d1.add(0, 0, 70));
    EXPECT_EQ(d1, DateAndTime(2024, 6, 14));
    EXPECT_TRUE(d1.add(0, 0, -80));
    EXPECT_EQ(d1, DateAndTime(2024, 3, 26));
    EXPECT_TRUE(d1.add(0, 0, -26));
    EXPECT_EQ(d1, DateAndTime(2024, 2, 29));
    EXPECT_TRUE(d1.add(0, 0, 1));
    EXPECT_EQ(d1, DateAndTime(2024, 3, 1));
    EXPECT_TRUE(d1.add(0, 0, -2));
    EXPECT_EQ(d1, DateAndTime(2024, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, 366));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, -366));
    EXPECT_EQ(d1, DateAndTime(2024, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, 366 + 365));
    EXPECT_EQ(d1, DateAndTime(2026, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, 1));
    EXPECT_EQ(d1, DateAndTime(2026, 3, 1));
    EXPECT_TRUE(d1.add(0, 0, -1));
    EXPECT_EQ(d1, DateAndTime(2026, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, -365 - 366 - 365));
    EXPECT_EQ(d1, DateAndTime(2023, 2, 28));
    d1.setDate(2025, 1, 31);
    EXPECT_TRUE(d1.add(0, 0, 31));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 3));
    d1.setDate(2025, 12, 31);
    EXPECT_TRUE(d1.add(0, 0, 31));
    EXPECT_EQ(d1, DateAndTime(2026, 1, 31));
    d1.setDate(2025, 3, 31);
    EXPECT_TRUE(d1.add(0, 0, -31));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 28));
    d1.setDate(2024, 2, 28);
    EXPECT_TRUE(d1.add(0, 0, 366));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, -366));
    EXPECT_EQ(d1, DateAndTime(2024, 2, 28));
    d1.setDate(2024, 2, 29);
    EXPECT_TRUE(d1.add(0, 0, 365));
    EXPECT_EQ(d1, DateAndTime(2025, 2, 28));
    EXPECT_TRUE(d1.add(0, 0, -365));
    EXPECT_EQ(d1, DateAndTime(2024, 2, 29));
    d1.setDate(2024, 2, 29);
    EXPECT_TRUE(d1.add(0, 0, 366));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 1));
    EXPECT_TRUE(d1.add(0, 0, -366));
    EXPECT_EQ(d1, DateAndTime(2024, 2, 29));
    d1.setDate(2024, 3, 1);
    EXPECT_TRUE(d1.add(0, 0, 365));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 1));
    EXPECT_TRUE(d1.add(0, 0, -365));
    EXPECT_EQ(d1, DateAndTime(2024, 3, 1));
    d1.setDate(2024, 3, 1);
    EXPECT_TRUE(d1.add(0, 0, 366));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 2));
    EXPECT_TRUE(d1.add(0, 0, -366));
    EXPECT_EQ(d1, DateAndTime(2024, 3, 1));

    // changing date only using months count
    d1.setDate(2025, 10, 31);
    EXPECT_TRUE(d1.add(0, 1, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 12, 1));
    EXPECT_TRUE(d1.add(0, -1, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 11, 1));
    EXPECT_TRUE(d1.add(0, 5, 0));
    EXPECT_EQ(d1, DateAndTime(2026, 4, 1));
    EXPECT_TRUE(d1.add(0, -6, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 10, 1));
    EXPECT_TRUE(d1.add(0, 9, 0));
    EXPECT_EQ(d1, DateAndTime(2026, 7, 1));
    EXPECT_TRUE(d1.add(0, -12, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 7, 1));
    EXPECT_TRUE(d1.add(0, 12, 0));
    EXPECT_EQ(d1, DateAndTime(2026, 7, 1));
    EXPECT_TRUE(d1.add(0, -18, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1));
    EXPECT_TRUE(d1.add(0, 18, 0));
    EXPECT_EQ(d1, DateAndTime(2026, 7, 1));
    EXPECT_TRUE(d1.add(0, -24, 0));
    EXPECT_EQ(d1, DateAndTime(2024, 7, 1));
    EXPECT_TRUE(d1.add(0, 36, 0));
    EXPECT_EQ(d1, DateAndTime(2027, 7, 1));
    d1.setDate(2024, 2, 29);
    EXPECT_TRUE(d1.add(0, -12, 0));
    EXPECT_EQ(d1, DateAndTime(2023, 3, 1));
    d1.setDate(2024, 2, 29);
    EXPECT_TRUE(d1.add(0, 12, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 1));

    // changing date only using year count
    d1.setDate(2025, 10, 31);
    EXPECT_TRUE(d1.add(100, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2125, 10, 31));
    EXPECT_TRUE(d1.add(-200, 0, 0));
    EXPECT_EQ(d1, DateAndTime(1925, 10, 31));
    d1.setDate(2024, 2, 29);
    EXPECT_TRUE(d1.add(1, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2025, 3, 1));
    d1.setDate(2024, 2, 29);
    EXPECT_TRUE(d1.add(-3, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2021, 3, 1));

    // change time and date
    d1 = DateAndTime{2025, 1, 1, 12, 0, 0, 0, 0};
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 1));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 0, 1));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 3500));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 3, 501));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -1));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 3, 500));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -2500));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 1, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -1));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 0, 999));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 2));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 1, 1));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -10));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 0, 991));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -1000));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 11, 59, 59, 999, 991));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 1009));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 0, 0, 1, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -1001000));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 11, 59, 59, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 61 * 1000000));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 12, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 60 * 60 * 1000000ll));
    EXPECT_EQ(d1, DateAndTime(2025, 1, 1, 13, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, -14 * 60 * 60 * 1000000ll));
    EXPECT_EQ(d1, DateAndTime(2024, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 365 * 24 * 60 * 60 * 1000000ll));
    EXPECT_EQ(d1, DateAndTime(2025, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 365 * 24 * 60 * 60 * 1000ll));
    EXPECT_EQ(d1, DateAndTime(2026, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 365 * 24 * 60 * 60));
    EXPECT_EQ(d1, DateAndTime(2027, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 366 * 24 * 60, 0));
    EXPECT_EQ(d1, DateAndTime(2028, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 365 * 24, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2029, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 365, 0, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2030, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 12, 0, 0, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2031, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(1, 0, 0, 0, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2032, 12, 31, 23, 1, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, -10, -3, 0, -1, 0));
    EXPECT_EQ(d1, DateAndTime(2032, 2, 28, 23, 0, 0, 0, 0));
    EXPECT_TRUE(d1.add(0, 0, 1, 0, 59, 59, 999, 999));
    EXPECT_EQ(d1, DateAndTime(2032, 2, 29, 23, 59, 59, 999, 999));
    EXPECT_TRUE(d1.add(-1, 0, 0, 0, 0, 0, 0, 0));
    EXPECT_EQ(d1, DateAndTime(2031, 3, 1, 23, 59, 59, 999, 999));
    EXPECT_TRUE(d1.add(0, -1, -29, 0, 2, -120, 1, -1999));
    EXPECT_EQ(d1, DateAndTime(2030, 12, 31, 23, 59, 59, 999, 0));
    EXPECT_TRUE(d1.add(0, 0, 0, 0, 0, 0, 0, 1000));
    EXPECT_EQ(d1, DateAndTime(2031, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(d1.add(1, -12, 2, -48, 2, -120, 10, -10000));
    EXPECT_EQ(d1, DateAndTime(2031, 1, 1, 0, 0, 0, 0, 0));
    EXPECT_TRUE(d1.add(-3, 36, -1, 24, -3, 180, -5, 4999));
    EXPECT_EQ(d1, DateAndTime(2030, 12, 31, 23, 59, 59, 999, 999));

    // test addDays() and addMicrosec() helpers
    EXPECT_TRUE(d1.addDays(15));
    EXPECT_EQ(d1, DateAndTime(2031, 1, 15, 23, 59, 59, 999, 999));
    EXPECT_TRUE(d1.addDays(-16));
    EXPECT_EQ(d1, DateAndTime(2030, 12, 30, 23, 59, 59, 999, 999));
    EXPECT_TRUE(d1.addMicrosec(2));
    EXPECT_EQ(d1, DateAndTime(2030, 12, 31, 0, 0, 0, 0, 1));
    EXPECT_TRUE(d1.addMicrosec(-2002));
    EXPECT_EQ(d1, DateAndTime(2030, 12, 30, 23, 59, 59, 997, 999));

    // test speedup of adding large number of days
    d1.set(2000, 1, 1, 0, 0, 0);
    EXPECT_TRUE(d1.addDays(366));
    EXPECT_EQ(d1, DateAndTime(2001, 1, 1, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(-366 - 365));
    EXPECT_EQ(d1, DateAndTime(1999, 1, 1, 0, 0, 0));
    d1.set(2000, 3, 1, 0, 0, 0);
    EXPECT_TRUE(d1.addDays(365));
    EXPECT_EQ(d1, DateAndTime(2001, 3, 1, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(-365));
    EXPECT_EQ(d1, DateAndTime(2000, 3, 1, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(-366));
    EXPECT_EQ(d1, DateAndTime(1999, 3, 1, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(3 * 366 + 7 * 365)); // leap years: 2000, 2004, 2008
    EXPECT_EQ(d1, DateAndTime(2009, 3, 1, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(7 * 366 + 23 * 365)); // leap years: 2012, 2016, 2020, 2024, 2028, 2032, 2036
    EXPECT_EQ(d1, DateAndTime(2039, 3, 1, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(-(3 * 366 + 12 * 365))); // leap years: 2028, 2032, 2036 (2024 not included due to date after Feb)
    EXPECT_EQ(d1, DateAndTime(2024, 3, 1, 0, 0, 0));
    d1.set(2039, 2, 28, 0, 0, 0);
    EXPECT_TRUE(d1.addDays(-(4 * 366 + 11 * 365))); // leap years: 2024, 2028, 2032, 2036
    EXPECT_EQ(d1, DateAndTime(2024, 2, 28, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(97 * 366 + 303 * 365)); // 400 years always have the same number of leap years
    EXPECT_EQ(d1, DateAndTime(2424, 2, 28, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(2 * 366 + 3 * 365)); // leap years: 2424, 2028
    EXPECT_EQ(d1, DateAndTime(2429, 2, 28, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(97 * 366 + 304 * 365)); // 400 years always have the same number of leap years + 1 year
    EXPECT_EQ(d1, DateAndTime(2830, 2, 28, 0, 0, 0));
    EXPECT_TRUE(d1.addDays(-3 * (97 * 366 + 303 * 365))); // 400 years always have the same number of leap years
    EXPECT_EQ(d1, DateAndTime(1630, 2, 28, 0, 0, 0));
    EXPECT_TRUE(d1.addDays((97 * 366 + 303 * 365) + 365 + 1)); // + 400 years + 1 year (2031) + 1 day
    EXPECT_EQ(d1, DateAndTime(2031, 3, 1, 0, 0, 0));

    // test some error cases
    EXPECT_FALSE(d1.addDays(366 * 66000));
    EXPECT_FALSE(d1.add(INT64_MAX - 1000, 0, 0));
    EXPECT_FALSE(d1.add(0, INT64_MAX, 0));
    d1.setTime(0, 0, 0, 0, 999);
    EXPECT_FALSE(d1.add(0, 0, 0, 0, 0, 0, 0, INT64_MAX - 998));
    EXPECT_FALSE(d1.add(0, 0, 0, 0, 0, 0, INT64_MIN, INT64_MIN));
}

uint64 microSeconds(uint64 days, uint64 hours, uint64 minutes, uint64 seconds, uint64 milli, uint64 micro)
{
    return (((((((days * 24) + hours) * 60llu) + minutes) * 60 + seconds) * 1000) + milli) * 1000 + micro;
}

TEST(DateAndTimeTest, Subtraction)
{
    DateAndTime d0;
    DateAndTime d1(2030, 12, 31, 5, 4, 3, 2, 1);
    EXPECT_EQ(d0.durationMicrosec(d0), UINT64_MAX);
    EXPECT_EQ(d0.durationMicrosec(d1), UINT64_MAX);
    EXPECT_EQ(d1.durationMicrosec(d0), UINT64_MAX);

    DateAndTime d2(2030, 12, 31, 0, 0, 0, 0, 0);
    EXPECT_EQ(d1.durationMicrosec(d1), 0);
    EXPECT_EQ(d1.durationMicrosec(d2), d2.durationMicrosec(d1));
    EXPECT_EQ(d1.durationMicrosec(d2), microSeconds(0, 5, 4, 3, 2, 1));
    EXPECT_EQ(d1.durationDays(d2), 0);

    d1.set(2030, 12, 31, 23, 59, 59, 999, 999);
    d2.set(2031, 1, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(d1.durationMicrosec(d2), 1);
    EXPECT_EQ(d1.durationDays(d2), 0);

    d1.set(2025, 1, 1, 0, 0, 0, 0, 0);
    d2.set(2027, 1, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(d1.durationMicrosec(d2), microSeconds(2 * 365, 0, 0, 0, 0, 0));
    EXPECT_EQ(d1.durationDays(d2), 2 * 365);

    d1.set(2027, 1, 1, 12, 15, 43, 123, 456);
    d2.set(2025, 1, 1, 11, 10, 34, 101, 412);
    EXPECT_EQ(d1.durationMicrosec(d2), microSeconds(2 * 365, 1, 5, 9, 22, 44));
    EXPECT_EQ(d1.durationDays(d2), 2 * 365);

    d1.set(2027, 1, 1, 12, 15, 43, 123, 456);
    d2.set(2024, 1, 1, 11, 10, 34, 101, 412);
    EXPECT_EQ(d1.durationMicrosec(d2), microSeconds(366 + 2 * 365, 1, 5, 9, 22, 44));
    EXPECT_EQ(d2.durationMicrosec(d1), microSeconds(366 + 2 * 365, 1, 5, 9, 22, 44));
    EXPECT_EQ(d1.durationDays(d2), 366 + 2 * 365);
    EXPECT_EQ(d2.durationDays(d1), 366 + 2 * 365);

    d2.set(2024, 1, 1, 0, 0, 0, 0, 0);
    d1.set(2024, 4, 1, 0, 0, 0, 0, 42);
    EXPECT_EQ(d1.durationMicrosec(d2), microSeconds(31 + 29 + 31, 0, 0, 0, 0, 42));
    EXPECT_EQ(d1.durationDays(d2), 31 + 29 + 31);

    std::mt19937_64 gen64(42);
    for (int i = 0; i < 1000; ++i)
    {
        d1.set((gen64() % 3000) + 1500, (gen64() % 12) + 1, (gen64() % 28) + 1, gen64() % 24,
            gen64() % 60, gen64() % 60, gen64() % 999, gen64() % 999);
        EXPECT_TRUE(d1.isValid());

        sint64 microsec = (sint64)gen64() % (1000llu * 365 * 24 * 60 * 60 * 1000 * 1000);
        d2 = d1;
        EXPECT_TRUE(d2.addMicrosec(microsec));
        EXPECT_TRUE(d2.isValid());

        EXPECT_EQ(d2.durationMicrosec(d1), microsec);
    }
}
