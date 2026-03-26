 #pragma once

// Header file for the inclusion of platform specific x86/x64 intrinsics header files.
//
// Historically the project included x86 intrinsics headers unconditionally.
// That causes hard build failures on non-x86 systems (for example Apple
// Silicon) because <immintrin.h> and related headers declare x86-only
// intrinsics and inline assembly. To remain backward compatible for x86
// builds while allowing native builds on other architectures, include
// x86 intrinsic headers only when the compiler target is an x86 family.

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#else
// Non-x86 targets: do not include x86 intrinsic headers.
#endif
#endif
