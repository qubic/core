#pragma once

// Helper functions for wchar_t strings in linux (linux expects 32-bit wchar_t, while we use 16-bit wchar_t everywhere else)
#include "lib/platform_efi/uefi.h"
#include <stdarg.h>
#include <codecvt>

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif
static std::string wchar_to_string(const wchar_t* wstr) {
    if (!wstr)
    {
        return std::string();
    }

    std::u16string u16str;
    for (size_t i = 0; wstr[i]; i++) {
        uint16_t c = static_cast<uint16_t>(wstr[i]); // truncate to 16-bit
        u16str += c;
    }

    // Convert UTF-16 to UTF-8
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(u16str);
}

#ifdef _MSC_VER
#pragma warning(disable: 5082)
#endif
static void print_wstr(const wchar_t* wstr, ...) {
    std::string utf8String = wchar_to_string(wstr);
    va_list args;
    va_start(args, utf8String.c_str());
    vprintf(utf8String.c_str(), args);
    va_end(args);
    printf("\n");
}

// convert 4 wchar_t to a number
constexpr static unsigned long long wcharToNumber(const CHAR16* text) {
    unsigned long long result = 0;
    result |= ((unsigned long long)text[0] & 0xFFFF) <<  0;
    result |= ((unsigned long long)text[1] & 0xFFFF) << 16;
    result |= ((unsigned long long)text[2] & 0xFFFF) << 32;
    result |= ((unsigned long long)text[3] & 0xFFFF) << 48;
    return result;
}

// convert a number to 4 wchar_t + null terminator
constexpr static void numberToWchar(unsigned long long number, CHAR16* text) {
    text[0] = (number >>  0) & 0xFFFF;
    text[1] = (number >> 16) & 0xFFFF;
    text[2] = (number >> 32) & 0xFFFF;
    text[3] = (number >> 48) & 0xFFFF;
    text[4] = 0; // null terminator
}

static bool isAllBytesZero(void *buffer, unsigned long long length) {
    for (size_t i = 0; i < length; i++) {
        if (((char*)buffer)[i] != 0) {
            return false;
        }
    }

    return true;
}