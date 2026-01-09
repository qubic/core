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
    appendNumber(message, address.u8[0], FALSE);
    appendText(message, L".");
    appendNumber(message, address.u8[1], FALSE);
    appendText(message, L".");
    appendNumber(message, address.u8[2], FALSE);
    appendText(message, L".");
    appendNumber(message, address.u8[3], FALSE);
}

static void appendDateAndTime(CHAR16* dst, const QPI::DateAndTime& dt, bool microsec = false)
{
    appendNumber(message, dt.getYear(), FALSE);
    appendText(message, L"-");
    appendNumber(message, dt.getMonth(), FALSE);
    appendText(message, L"-");
    appendNumber(message, dt.getDay(), FALSE);
    appendText(message, L" ");
    appendNumber(message, dt.getHour(), FALSE);
    appendText(message, L":");
    appendNumber(message, dt.getMinute(), FALSE);
    appendText(message, L":");
    appendNumber(message, dt.getSecond(), FALSE);
    appendText(message, L".");
    const uint16 millisec = dt.getMillisec();
    appendNumber(message, millisec / 100, FALSE);
    appendNumber(message, (millisec % 100) / 10, FALSE);
    appendNumber(message, millisec % 10, FALSE);
    if (microsec)
    {
        const uint16 microsec = dt.getMicrosecDuringMillisec();
        appendNumber(message, microsec / 100, FALSE);
        appendNumber(message, (microsec % 100) / 10, FALSE);
        appendNumber(message, microsec % 10, FALSE);
    }
}
