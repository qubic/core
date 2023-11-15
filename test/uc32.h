#pragma once

#include <intrin.h>

struct UC32x
{
    union {
        unsigned char uc32x[32];
        unsigned long long ull4[4];
        __m256i m256i;
    };

    UC32x(bool random = false)
    {
        if (random)
        {
            _rdrand64_step(&ull4[0]);
            _rdrand64_step(&ull4[1]);
            _rdrand64_step(&ull4[2]);
            _rdrand64_step(&ull4[3]);
        }
        else
        {
            ull4[0] = 0ULL;
            ull4[1] = 0ULL;
            ull4[2] = 0ULL;
            ull4[3] = 0ULL;
        }
    }

    UC32x(unsigned long long ull0, unsigned long long ull1, unsigned long long ull2, unsigned long long ull3)
    {
        ull4[0] = ull0;
        ull4[1] = ull1;
        ull4[2] = ull2;
        ull4[3] = ull3;
    }
};

