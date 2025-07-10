#pragma once

// Compiler detection
#if defined(_MSC_VER) && !defined(__clang__)
    // Microsoft Visual C++
    #define OPTIMIZE_OFF() __pragma(optimize("", off))
    #define OPTIMIZE_ON()  __pragma(optimize("", on))
    #define NO_OPTIMIZE    __declspec(noinline)
#elif defined(__clang__)
    // Clang
    #define OPTIMIZE_OFF() _Pragma("clang optimize off")
    #define OPTIMIZE_ON()  _Pragma("clang optimize on")
    #define NO_OPTIMIZE    __attribute__((optnone)) __attribute__((noinline))
#else
    // Fallback for unknown compilers
    #define OPTIMIZE_OFF()
    #define OPTIMIZE_ON()
    #define NO_OPTIMIZE
    #warning "Optimization control not supported for this compiler"
#endif
