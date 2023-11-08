#pragma once

#include <intrin.h>

#if 0
#define EQUAL(a, b) (_mm256_movemask_epi8(_mm256_cmpeq_epi64(a, b)) == 0xFFFFFFFF)
#else

static inline bool EQUAL(const __m256i& a, const __m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(a, b)) == 0xFFFFFFFF;
}

static inline bool operator==(const __m256i& a, const __m256i& b)
{
    return EQUAL(a, b);
}

static inline bool operator!=(const __m256i& a, const __m256i& b)
{
    return !EQUAL(a, b);
}

static inline bool isZero(const __m256i& a)
{
    return _mm256_testz_si256(a, a) == 1;
}

#endif
