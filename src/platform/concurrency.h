#pragma once

#include <intrin.h>

#define ACQUIRE(lock) while (_InterlockedCompareExchange8(&lock, 1, 0)) _mm_pause()
#define RELEASE(lock) lock = 0
