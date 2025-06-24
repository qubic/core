#pragma once

// Macros to suppress specific compiler warnings for specific sections of code.
// Useful to hide warnings about unknown pragmas while using other compilers.

// Example usage:
// #include "lib/platform_common/compiler_warnings.h"
//
// int foo() {
//     SUPPRESS_WARNINGS_BEGIN
//     IGNORE_CAST_ALIGN_WARNING
//     // Code that generates warnings
//     SUPPRESS_WARNINGS_END
//    return 0;
// }

#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC specific pragmas
    #define PRAGMA(x) __pragma(x) // MSVC uses __pragma instead of _Pragma
    #define WARNING_PUSH __pragma(warning(push))
    #define WARNING_POP __pragma(warning(pop))
    #define WARNING_IGNORE_CAST_ALIGN // No direct common MSVC equivalent for -Wcast-align, often handled differently or via other warnings
    #define WARNING_IGNORE_UNUSED __pragma(warning(disable: 4100)) // C4100: 'identifier' : unreferenced formal parameter
    #define WARNING_IGNORE_SELFASSIGNMENT // No direct common MSVC equivalent for -Wself-assign-overloaded
    #define WARNING_IGNORE_CONVERSION_DATALOSS __pragma(warning(disable: 4310)) // C4310: cast truncates constant value

#elif defined(__clang__)
    #define PRAGMA(x) _Pragma(#x)
    #define WARNING_PUSH PRAGMA(clang diagnostic push)
    #define WARNING_POP PRAGMA(clang diagnostic pop)
    #define WARNING_IGNORE_CAST_ALIGN PRAGMA(clang diagnostic ignored "-Wcast-align")
    #define WARNING_IGNORE_UNUSED PRAGMA(clang diagnostic ignored "-Wunused-parameter")
    #define WARNING_IGNORE_SELFASSIGNMENT PRAGMA(clang diagnostic ignored "-Wself-assign-overloaded")
    #define WARNING_IGNORE_CONVERSION_DATALOSS PRAGMA(clang diagnostic ignored "-Wconversion")

#else
    #define WARNING_PUSH
    #define WARNING_POP
    #define WARNING_IGNORE_CAST_ALIGN
    #define WARNING_IGNORE_UNUSED
    #define WARNING_IGNORE_SELFASSIGNMENT
    #define WARNING_IGNORE_CONVERSION_DATALOSS
#endif

// Shortcuts
#define SUPPRESS_WARNINGS_BEGIN WARNING_PUSH
#define SUPPRESS_WARNINGS_END WARNING_POP
#define IGNORE_CAST_ALIGN_WARNING WARNING_IGNORE_CAST_ALIGN 
#define IGNORE_UNUSED_WARNING WARNING_IGNORE_UNUSED
#define IGNORE_SELFASSIGNMENT_WARNING WARNING_IGNORE_SELFASSIGNMENT
#define IGNORE_CONVERSION_DATALOSS_WARNING WARNING_IGNORE_CONVERSION_DATALOSS

