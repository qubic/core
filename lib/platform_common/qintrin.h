#pragma once

// Header file for the inclusion of plartform specific x86/x64 intrinsics header files.

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <immintrin.h>
#endif
