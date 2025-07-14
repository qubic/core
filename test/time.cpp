#include "gtest/gtest.h"
#define NO_UEFI

#include "platform/time.h"

#include <iostream>
#include <ctime>

// Convert TimeDate to std::tm
std::tm toTm(const TimeDate& t)
{
    std::tm tmTime{};
    tmTime.tm_sec = t.second;
    tmTime.tm_min = t.minute;
    tmTime.tm_hour = t.hour;
    tmTime.tm_mday = t.day;
    tmTime.tm_mon = t.month - 1;         // tm_mon is 0-based
    tmTime.tm_year = t.year + 100;        // tm_year is years since 1900
    return tmTime;
}

// Compute difference in seconds between two TimeDate values
long long stdSecondsBetween(const TimeDate& a, const TimeDate& b)
{
    std::tm tmA = toTm(a);
    std::tm tmB = toTm(b);
    std::time_t timeA = std::mktime(&tmA);
    std::time_t timeB = std::mktime(&tmB);
    return static_cast<long long>(std::difftime(timeB, timeA));
}

TEST(TestTime, DiffDateSecond)
{
    TimeDate A;
    TimeDate B;

    // Normal
    A.second = 56;
    A.minute = 12;
    A.hour = 4;
    A.day = 7;
    A.month = 5;  
    A.year  = 1;  

    B.second = 56;
    B.minute = 12;
    B.hour = 4;
    B.day = 7;
    B.month = 5;
    B.year = 11;

    EXPECT_EQ(stdSecondsBetween(A, B), diffDateSecond(A, B));

    // A is gap, B is not
    A.second = 56;
    A.minute = 12;
    A.hour = 4;
    A.day = 7;
    A.month = 3;
    A.year = 12;

    B.second = 56;
    B.minute = 12;
    B.hour = 4;
    B.day = 7;
    B.month = 5;
    B.year = 21;

    // A is not gap, B is
    A.second = 56;
    A.minute = 12;
    A.hour = 4;
    A.day = 21;
    A.month = 3;
    A.year = 13;

    B.second = 56;
    B.minute = 12;
    B.hour = 4;
    B.day = 1;
    B.month = 3;
    B.year = 24;
    EXPECT_EQ(stdSecondsBetween(A, B), diffDateSecond(A, B));

    // A,B is  gap
    A.second = 56;
    A.minute = 12;
    A.hour = 4;
    A.day = 21;
    A.month = 3;
    A.year = 12;

    B.second = 56;
    B.minute = 12;
    B.hour = 4;
    B.day = 1;
    B.month = 3;
    B.year = 24;

    EXPECT_EQ(stdSecondsBetween(A, B), diffDateSecond(A, B));

}

TEST(TestTime, Unpack)
{
    unsigned int packedWeekTime;
    WeekDay wd;

    // Thurs 15:20:30
    packedWeekTime = 0x040F141E;
    wd = convertWeekTimeFromPackedData(packedWeekTime);
    EXPECT_EQ(wd.dayOfWeek, 4);
    EXPECT_EQ(wd.hour, 15);
    EXPECT_EQ(wd.minute, 20);
    EXPECT_EQ(wd.second, 30);

    // Sat 12:00:00
    packedWeekTime = 0x060C0000;
    wd = convertWeekTimeFromPackedData(packedWeekTime);
    EXPECT_EQ(wd.dayOfWeek, 6);
    EXPECT_EQ(wd.hour, 12);
    EXPECT_EQ(wd.minute, 0);
    EXPECT_EQ(wd.second, 0);

    // Sun 12:00:00
    packedWeekTime = 0x000C0000;
    wd = convertWeekTimeFromPackedData(packedWeekTime);
    EXPECT_EQ(wd.dayOfWeek, 0);
    EXPECT_EQ(wd.hour, 12);
    EXPECT_EQ(wd.minute, 0);
    EXPECT_EQ(wd.second, 0);
}

TEST(TestTime, WeekDay)
{
    TimeDate A;

    // Normal
    A.second = 56;
    A.minute = 12;
    A.hour = 4;
    A.day = 7;
    A.month = 5;
    A.year = 1;

    std::tm tmA = toTm(A);
    std::mktime(&tmA);

    EXPECT_EQ(getDayOfWeek(A.day, A.month, A.year), tmA.tm_wday);

}

TEST(TestTime, Comparison)
{
    TimeDate A;
    TimeDate B;

    // Normal
    A.second = 56;
    A.minute = 12;
    A.hour = 4;
    A.day = 7;
    A.month = 5;
    A.year = 1;

    B.second = 56;
    B.minute = 12;
    B.hour = 4;
    B.day = 7;
    B.month = 5;
    B.year = 11;

    EXPECT_EQ(compareTimeDate(A, A), 0);
    EXPECT_EQ(compareTimeDate(B, A), 1);
    EXPECT_EQ(compareTimeDate(A, B), -1);

    // Test week day in range
    // 
    // Monday in [Sunday, Tuesday]
    EXPECT_TRUE(isWeekDayInRange(1, 0, 2));

    // Sunday is in [Saturday, Tuesday]
    EXPECT_TRUE(isWeekDayInRange(0, 6, 2));

    // Wednesday is in [Saturday, Thursday]
    EXPECT_TRUE(isWeekDayInRange(3, 6, 4));

    // Wednesday is in [Saturday, Wednesday]
    EXPECT_TRUE(isWeekDayInRange(3, 6, 3));

    // Wednesday is in [Wednesday, Saturday]
    EXPECT_TRUE(isWeekDayInRange(3, 3, 6));

    // Wednesday is not in [Sunday, Tuesday]
    EXPECT_FALSE(isWeekDayInRange(3, 0, 2));

    // Thursday is not in [Saturday, Tuesday]
    EXPECT_FALSE(isWeekDayInRange(4, 6, 2));

    WeekDay weekDay;
    WeekDay startWeekDay, endWeekDay;

    // [Saturday 12:50:20]
    startWeekDay.second = 20;
    startWeekDay.minute = 50;
    startWeekDay.hour = 12;
    startWeekDay.dayOfWeek = 6;

    // [Monday 08:05:15]
    endWeekDay.second = 15;
    endWeekDay.minute = 5;
    endWeekDay.hour = 8;
    endWeekDay.dayOfWeek = 1;

    // [Sunday 04:12:56]
    weekDay.second = 56;
    weekDay.minute = 12;
    weekDay.hour = 4;
    weekDay.dayOfWeek = 0;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Tuesday 04:12:56]
    weekDay.dayOfWeek = 2;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    //**** Test lower bound of week day
    // [Saturday 12:50:21]
    weekDay.dayOfWeek = 6;
    weekDay.second = 21;
    weekDay.minute = 50;
    weekDay.hour = 12;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Saturday 12:51:20]
    weekDay.dayOfWeek = 6;
    weekDay.second = 20;
    weekDay.minute = 51;
    weekDay.hour = 12;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Saturday 13:50:20]
    weekDay.dayOfWeek = 6;
    weekDay.second = 20;
    weekDay.minute = 50;
    weekDay.hour = 13;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Saturday 12:50:19]
    weekDay.dayOfWeek = 6;
    weekDay.second = 19;
    weekDay.minute = 50;
    weekDay.hour = 12;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Saturday 12:49:20]
    weekDay.dayOfWeek = 6;
    weekDay.second = 20;
    weekDay.minute = 49;
    weekDay.hour = 12;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Saturday 11:50:20]
    weekDay.dayOfWeek = 6;
    weekDay.second = 20;
    weekDay.minute = 50;
    weekDay.hour = 11;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    //**** Test upper bound of week day
    // [Monday 08:05:14]
    weekDay.second = 14;
    weekDay.minute = 5;
    weekDay.hour = 8;
    weekDay.dayOfWeek = 1;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Monday 08:04:15]
    weekDay.second = 15;
    weekDay.minute = 04;
    weekDay.hour = 8;
    weekDay.dayOfWeek = 1;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Monday 07:05:15]
    weekDay.second = 15;
    weekDay.minute = 5;
    weekDay.hour = 7;
    weekDay.dayOfWeek = 1;
    EXPECT_TRUE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Monday 08:05:16]
    weekDay.second = 16;
    weekDay.minute = 5;
    weekDay.hour = 8;
    weekDay.dayOfWeek = 1;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Monday 08:06:15]
    weekDay.second = 15;
    weekDay.minute = 6;
    weekDay.hour = 8;
    weekDay.dayOfWeek = 1;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

    // [Monday 09:06:15]
    weekDay.second = 15;
    weekDay.minute = 5;
    weekDay.hour = 9;
    weekDay.dayOfWeek = 1;
    EXPECT_FALSE(isWeekDayInRange(weekDay, startWeekDay, endWeekDay));

}

