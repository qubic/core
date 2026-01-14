#pragma once

#include "public_settings.h"
#include "platform/console_logging.h"

#include "network_messages/common_def.h"

#include "contracts/qpi.h"


static void appendQubicVersion(CHAR16* dst)
{
    appendNumber(dst, VERSION_A, FALSE);
    appendText(dst, L".");
    appendNumber(dst, VERSION_B, FALSE);
    appendText(dst, L".");
    appendNumber(dst, VERSION_C, FALSE);
}

static void appendIPv4Address(CHAR16* dst, const IPv4Address& address)
{
    appendNumber(dst, address.u8[0], FALSE);
    appendText(dst, L".");
    appendNumber(dst, address.u8[1], FALSE);
    appendText(dst, L".");
    appendNumber(dst, address.u8[2], FALSE);
    appendText(dst, L".");
    appendNumber(dst, address.u8[3], FALSE);
}

static void appendDateAndTime(CHAR16* dst, const QPI::DateAndTime& dt, bool microsec = false)
{
    appendNumber(dst, dt.getYear(), FALSE);
    appendText(dst, L"-");
    appendNumber(dst, dt.getMonth(), FALSE);
    appendText(dst, L"-");
    appendNumber(dst, dt.getDay(), FALSE);
    appendText(dst, L" ");
    appendNumber(dst, dt.getHour(), FALSE);
    appendText(dst, L":");
    appendNumber(dst, dt.getMinute(), FALSE);
    appendText(dst, L":");
    appendNumber(dst, dt.getSecond(), FALSE);
    appendText(dst, L".");
    const uint16 millisec = dt.getMillisec();
    appendNumber(dst, millisec / 100, FALSE);
    appendNumber(dst, (millisec % 100) / 10, FALSE);
    appendNumber(dst, millisec % 10, FALSE);
    if (microsec)
    {
        const uint16 microsec = dt.getMicrosecDuringMillisec();
        appendNumber(dst, microsec / 100, FALSE);
        appendNumber(dst, (microsec % 100) / 10, FALSE);
        appendNumber(dst, microsec % 10, FALSE);
    }
}
