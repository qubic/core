#pragma once

// Helper functions for wchar_t strings in linux (linux expects 32-bit wchar_t, while we use 16-bit wchar_t everywhere else)
#include "lib/platform_efi/uefi.h"
#include <codecvt>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <vector>

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

#ifdef __linux__
static int exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");   // "r" = read output (even if we ignore it)
    if (!pipe) return -1;

    // just discard output like system() does
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        // no need to store or print
    }

    int status = pclose(pipe);      // wait for command to finish
    return WEXITSTATUS(status);     // return exit code like system()
}
#endif

static std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    // 1. Create a stringstream from the input string
    std::stringstream ss(str);
    std::string token;

    // 2. Use std::getline to extract tokens up to the delimiter
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

static void hexToByte(const std::string& hex, const int sizeInByte, unsigned char* out)
{
    if (hex.length() != sizeInByte * 2)
    {
        throw std::invalid_argument("Hex string length does not match the expected size");
    }

    for (size_t i = 0; i < sizeInByte; ++i)
    {
        out[i] = std::stoi(hex.substr(i * 2, 2), nullptr, 16);
    }
}


static std::string byteToHex(const unsigned char* byteArray, size_t sizeInByte)
{
    std::ostringstream oss;
    for (size_t i = 0; i < sizeInByte; ++i)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byteArray[i]);
    }
    return oss.str();
}

static const int T[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
};

std::vector<unsigned char> base64_decode(const std::string &s)
{
    std::vector<unsigned char> out;
    int val = 0, valb = -8;

    for (unsigned char c : s)
    {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back((unsigned char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}