#pragma once

#include "public_settings.h"
#include "platform/console_logging.h"

static void appendQubicVersion(CHAR16* dst)
{
    appendNumber(dst, VERSION_A, FALSE);
    appendText(dst, L".");
    appendNumber(dst, VERSION_B, FALSE);
    appendText(dst, L".");
    appendNumber(dst, VERSION_C, FALSE);
}
