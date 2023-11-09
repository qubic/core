#pragma once

#include "uefi.h"
#include <stddef.h>

#ifdef NO_UEFI
#error "NO_UEFI implementation is missing!"
#endif

static EFI_TIME time;

static void initTime()
{
    bs->SetMem(&time, sizeof(time), 0);
    time.Year = 2022;
    time.Month = 4;
    time.Day = 13;
    time.Hour = 12;

    EFI_TIME newTime;
    if (!rs->GetTime(&newTime, NULL))
    {
        bs->CopyMem(&time, &newTime, sizeof(time));
    }
}

static void updateTime()
{
    EFI_TIME newTime;
    if (!rs->GetTime(&newTime, NULL))
    {
        bs->CopyMem(&time, &newTime, sizeof(time));
    }
}

inline int dayIndex(unsigned int year, unsigned int month, unsigned int day) // 0 = Wednesday
{
    return (year += (2000 - (month = (month + 9) % 12) / 10)) * 365 + year / 4 - year / 100 + year / 400 + (month * 306 + 5) / 10 + day - 1;
}

inline long long ms(unsigned char year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second, unsigned short millisecond)
{
    return (((((long long)dayIndex(year, month, day)) * 24 + hour) * 60 + minute) * 60 + second) * 1000 + millisecond;
}
