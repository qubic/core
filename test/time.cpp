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
}
