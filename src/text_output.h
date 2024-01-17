#pragma once

#include "public_settings.h"
#include "platform/console_logging.h"

#include "network_messages/common_def.h"


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
