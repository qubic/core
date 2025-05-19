#pragma once

// Header file for the definition of standard integer types for different platforms.

// Try to detect if standard headers have already been included e.g., from gtest. If already included, ignore qstdint.h.
#if defined(__has_include)
    #if __has_include(<cstdint>)
        #include <cstdint>
        #define STDINT_USING_STD 1
    #elif __has_include(<stdint.h>)
        #include <stdint.h>
        #define STDINT_USING_STD 1
    #endif
#endif

#ifndef STDINT_USING_STD

#if defined(_MSC_VER) && !defined(__clang__) 
    // MSVC definitions
    typedef signed __int8     int8_t;
    typedef unsigned __int8   uint8_t;
    typedef signed __int16    int16_t;
    typedef unsigned __int16  uint16_t;
    typedef signed __int32    int32_t;
    typedef unsigned __int32  uint32_t;
    typedef signed __int64    int64_t;
    typedef unsigned __int64  uint64_t;
#else
    // Clang/Linux definitions
    typedef signed char       int8_t;
    typedef unsigned char     uint8_t;
    typedef short             int16_t;
    typedef unsigned short    uint16_t;
    typedef int               int32_t;
    typedef unsigned int      uint32_t;
    typedef long long         int64_t;
    typedef unsigned long long uint64_t;
#endif

// static asserts to validate sizes
static_assert(sizeof(int8_t) == 1,   "int8_t must be 1 byte");
static_assert(sizeof(uint8_t) == 1,  "uint8_t must be 1 byte");
static_assert(sizeof(int16_t) == 2,  "int16_t must be 2 bytes");
static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
static_assert(sizeof(int32_t) == 4,  "int32_t must be 4 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
static_assert(sizeof(int64_t) == 8,  "int64_t must be 8 bytes");
static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");

#endif