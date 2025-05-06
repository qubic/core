#pragma once

// Class to suppress specific compiler warnings for specific sections of code.
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


#if defined(__clang__)
    #define PRAGMA(x) _Pragma(#x)
    #define WARNING_PUSH PRAGMA(clang diagnostic push)
    #define WARNING_POP PRAGMA(clang diagnostic pop)
    #define WARNING_IGNORE_CAST_ALIGN PRAGMA(clang diagnostic ignored "-Wcast-align")
    #define WARNING_IGNORE_UNUSED PRAGMA(clang diagnostic ignored "-Wunused-parameter")
    #define WARNING_IGNORE_SELFASSIGNMENT PRAGMA(clang diagnostic ignored "-Wself-assign-overloaded")

#else
    #define WARNING_PUSH
    #define WARNING_POP
    #define WARNING_IGNORE_CAST_ALIGN
    #define WARNING_IGNORE_UNUSED
    #define WARNING_IGNORE_SELFASSIGNMENT
#endif

// Shortcuts
#define SUPPRESS_WARNINGS_BEGIN WARNING_PUSH
#define SUPPRESS_WARNINGS_END WARNING_POP
#define IGNORE_CAST_ALIGN_WARNING WARNING_IGNORE_CAST_ALIGN 
#define IGNORE_UNUSED_WARNING WARNING_IGNORE_UNUSED
#define IGNORE_SELFASSIGNMENT_WARNING WARNING_IGNORE_SELFASSIGNMENT

