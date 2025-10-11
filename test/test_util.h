#pragma once

#include "gtest/gtest.h"

#include "four_q.h"

static std::ostream& operator<<(std::ostream& s, const m256i& v)
{
    CHAR16 identityWchar[61];
    char identityChar[61];
    setMem(identityChar, 61, 0);
    getIdentity(v.m256i_u8, identityWchar, false);
    size_t size;
#ifdef _WIN32
    wcstombs_s(&size, identityChar, identityWchar, 61);
#else
    copyMem(identityChar, wchar_to_string(identityWchar).c_str(), 60);
#endif
    s << identityChar;
    return s;
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