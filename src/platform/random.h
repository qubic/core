#pragma once

#include <lib/platform_common/qintrin.h>

inline static unsigned int random(const unsigned int range)
{
    unsigned int value;
    _rdrand32_step(&value);

    return value % range;
}

inline static void random64(unsigned long long* dst)
{
    ASSERT(dst != NULL);
    _rdrand64_step(dst);
}

inline static void random256(m256i* dst)
{
    ASSERT(dst != NULL);
    dst->setRandomValue();
}