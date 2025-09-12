#pragma once

// Helper functions for wchar_t strings in linux (linux expects 32-bit wchar_t, while we use 16-bit wchar_t everywhere else)
#ifndef _MSC_VER
#include <stdarg.h>
std::string wchar_to_string(const wchar_t* wstr) {
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

void print_wstr(const wchar_t* wstr, ...) {
    std::string utf8String = wchar_to_string(wstr);
    va_list args;
    va_start(args, utf8String.c_str());
    vprintf(utf8String.c_str(), args);
    va_end(args);
    printf("\n");
}
#endif
