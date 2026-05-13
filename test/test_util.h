#pragma once

#include <iostream>

#include "gtest/gtest.h"

#include "four_q.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

// for QPI::DateAndTime
#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"
#include "contract_core/qpi_spectrum_impl.h"

// for etalonTick
#include "contract_core/qpi_ticking_impl.h"


static std::ostream& operator<<(std::ostream& s, const m256i& v)
{
    CHAR16 identityWchar[61];
    char identityChar[61];
    getIdentity(v.m256i_u8, identityWchar, false);
    size_t size;
    wcstombs_s(&size, identityChar, identityWchar, 61);
    s << identityChar;
    return s;
}

static ::std::ostream& operator<<(::std::ostream& os, const QPI::DateAndTime& dt)
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

static unsigned long long assetNameFromString(const char* assetName)
{
    size_t n = strlen(assetName);
    EXPECT_LE(n, 7);
    unsigned long long integer = 0;
    copyMem(&integer, assetName, n);
    return integer;
}

static std::string assetNameFromInt64(unsigned long long assetName)
{
    char buffer[8];
    copyMem(&buffer, &assetName, sizeof(assetName));
    buffer[7] = 0;
    return buffer;
}

static void advanceTimeAndTick(uint64_t milliseconds)
{
    QPI::DateAndTime now = QPI::DateAndTime::now();
    EXPECT_TRUE(now.addMillisec(milliseconds));
    etalonTick.year = now.getYear() - 2000;
    etalonTick.month = now.getMonth();
    etalonTick.day = now.getDay();
    etalonTick.hour = now.getHour();
    etalonTick.minute = now.getMinute();
    etalonTick.second = now.getSecond();
    etalonTick.millisecond = now.getMillisec();
    ++system.tick;
}
