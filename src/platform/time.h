#pragma once

#include "global_var.h"
#include "uefi.h"
#include "memory.h"
#include <stddef.h>


GLOBAL_VAR_DECL EFI_TIME utcTime;


#ifdef NO_UEFI

// Defined in test/stdlib_impl.cpp
void updateTime();

#else

static void updateTime()
{
    EFI_TIME newTime;
    if (!rs->GetTime(&newTime, NULL))
    {
        bs->CopyMem(&utcTime, &newTime, sizeof(utcTime));
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
