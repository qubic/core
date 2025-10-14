#pragma once

#include "global_var.h"
#include <lib/platform_efi/uefi.h>
#include "memory.h"
#include <stddef.h>


GLOBAL_VAR_DECL EFI_TIME utcTime;


#ifdef NO_UEFI

// Defined in test/stdlib_impl.cpp
void updateTime();

#else

#include <lib/platform_efi/uefi_globals.h>
#include <lib/platform_common/processor.h>

static void updateTime()
{
    ASSERT(isMainProcessor());
    EFI_TIME newTime;
    if (!rs->GetTime(&newTime, NULL))
    {
        copyMem(&utcTime, &newTime, sizeof(utcTime));
    }
}

#endif

static void initTime()
{
    setMem(&utcTime, sizeof(utcTime), 0);
    utcTime.Year = 2022;
    utcTime.Month = 4;
    utcTime.Day = 13;
    utcTime.Hour = 12;

    updateTime();
}

inline int dayIndex(unsigned int year, unsigned int month, unsigned int day) // 0 = Wednesday
{
    return (year += (2000 - (month = (month + 9) % 12) / 10)) * 365 + year / 4 - year / 100 + year / 400 + (month * 306 + 5) / 10 + day - 1;
}

inline long long ms(unsigned char year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second, unsigned short millisecond)
{
    return (((((long long)dayIndex(year, month, day)) * 24 + hour) * 60 + minute) * 60 + second) * 1000 + millisecond;
}

// Compute the total days from first month of the year to current beginning of the month
static inline unsigned int accumulatedDay(unsigned char month)
{
    unsigned int res = 0;
    switch (month)
    {
    case 1: res = 0; break;
    case 2: res = 31; break;
    case 3: res = 59; break;
    case 4: res = 90; break;
    case 5: res = 120; break;
    case 6: res = 151; break;
    case 7: res = 181; break;
    case 8: res = 212; break;
    case 9: res = 243; break;
    case 10:res = 273; break;
    case 11:res = 304; break;
    case 12:res = 334; break;
    }
    return res;
}

// Check if a year (offset from 2000) is a leap year
static inline bool isLeapYear(unsigned char year)
{
    int y = year + 2000;
    return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}

// Zeller's Congruence : Returns 0 = Sunday, 1 = Monday, ..., 6 = Saturday
static inline int getDayOfWeek(int day, int month, int year)
{
    if (month < 3)
    {
        month += 12;
        year -= 1;
    }
    int K = year % 100;
    int J = year / 100;

    int h = (day + (13 * (month + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;

    // Convert Zeller's output to ISO: 0 = Sunday ... 6 = Saturday
    int d = ((h + 6) % 7);
    return d;
}

struct WeekDay
{
    unsigned short millisecond;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char dayOfWeek;
};

struct TimeDate
{
    unsigned short millisecond;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
};

static inline WeekDay convertWeekTimeFromPackedData(unsigned int packedWeekTime)
{
    WeekDay wd;
    wd.millisecond = 0;
    wd.second = packedWeekTime & 0xFF;
    wd.minute = (packedWeekTime >> 8) & 0xFF;
    wd.hour = (packedWeekTime >> 16) & 0xFF;
    wd.dayOfWeek = (packedWeekTime >> 24) & 0xFF;
    return wd;
}

// Check of weekday is in range [startDay, endDay]
static inline bool isWeekDayInRange(unsigned char day, unsigned char startDay, unsigned char endDay)
{
    if (startDay <= endDay)
        return day >= startDay && day <= endDay;
    else
        return day >= startDay || day <= endDay;
}

static inline bool isWeekDayInRange(WeekDay day, WeekDay startDay, WeekDay endDay)
{
    // Day is not in range. return imediately
    if (!isWeekDayInRange(day.dayOfWeek, startDay.dayOfWeek, endDay.dayOfWeek))
    {
        return false;
    }

    // Check other field
    if ((day.dayOfWeek == startDay.dayOfWeek && day.hour < startDay.hour)
        || (day.dayOfWeek == startDay.dayOfWeek && day.hour == startDay.hour && day.minute < startDay.minute)
        || (day.dayOfWeek == startDay.dayOfWeek && day.hour == startDay.hour && day.minute == startDay.minute && day.second < startDay.second))
    {
        return false;
    }


    if ((day.dayOfWeek == endDay.dayOfWeek && day.hour > endDay.hour)
        || (day.dayOfWeek == endDay.dayOfWeek && day.hour == endDay.hour && day.minute > endDay.minute)
        || (day.dayOfWeek == endDay.dayOfWeek && day.hour == endDay.hour && day.minute == endDay.minute && day.second > endDay.second))
    {
        return false;
    }

    return true;
}

// Return 1 if a > b, -1 if a < b, 0 if equal
static inline int compareTimeDate(TimeDate a, TimeDate b)
{
    if (a.year != b.year) return (a.year > b.year) ? 1 : -1;
    if (a.month != b.month) return (a.month > b.month) ? 1 : -1;
    if (a.day != b.day) return (a.day > b.day) ? 1 : -1;
    if (a.hour != b.hour) return (a.hour > b.hour) ? 1 : -1;
    if (a.minute != b.minute) return (a.minute > b.minute) ? 1 : -1;
    if (a.second != b.second) return (a.second > b.second) ? 1 : -1;
    if (a.millisecond != b.millisecond) return (a.millisecond > b.millisecond) ? 1 : -1;
    return 0;
}

// Return the earlier of two times
static inline TimeDate minTimeDate(struct TimeDate a, struct TimeDate b)
{
    return (compareTimeDate(a, b) <= 0) ? a : b;
}

// Return the later of two times
static inline TimeDate maxTimeDate(struct TimeDate a, struct TimeDate b)
{
    return (compareTimeDate(a, b) >= 0) ? a : b;
}

// Compute time difference in seconds between 2 time date
static inline unsigned long long diffDateSecond(const TimeDate& C, const TimeDate& D)
{
    TimeDate A = minTimeDate(C, D);
    TimeDate B = maxTimeDate(C, D);

    unsigned long long dayA = accumulatedDay(A.month) + A.day;
    unsigned long long dayB = accumulatedDay(B.month) + B.day;
    dayB += ((unsigned long long)B.year - A.year) * 365ULL;

    // handling leap-year
    for (unsigned char i = A.year; i < B.year; i++)
    {
        if (isLeapYear(i))
        {
            dayB++;
        }
    }
    if (isLeapYear(A.year) && (A.month > 2)) dayA++;
    if (isLeapYear(B.year) && (B.month > 2)) dayB++;

    unsigned long long res = (dayB - dayA) * 3600ULL * 24;
    res += B.hour * 3600 + B.minute * 60 + B.second;
    res -= A.hour * 3600 + A.minute * 60 + A.second;

    return res;
}

#ifndef NO_UEFI
inline unsigned long long now_ms()
{
    // utcTime is updated on main thread - because only main thread can do it
    return ms(unsigned char(utcTime.Year % 100), utcTime.Month, utcTime.Day, utcTime.Hour, utcTime.Minute, utcTime.Second, utcTime.Nanosecond / 1000000);
}
#else
unsigned long long now_ms();
#endif