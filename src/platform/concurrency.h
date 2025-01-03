#pragma once

#include <intrin.h>

// Acquire lock, may block
#define ACQUIRE(lock) while (_InterlockedCompareExchange8(&lock, 1, 0)) _mm_pause()

// Try to acquire lock and return if successful (without blocking)
#define TRY_ACQUIRE(lock) (_InterlockedCompareExchange8(&lock, 1, 0) == 0)

// Release lock
#define RELEASE(lock) lock = 0

#define ATOMIC_STORE8(target, val) _InterlockedExchange8(&target, val)
#define ATOMIC_INC64(target) _InterlockedIncrement64(&target)
#define ATOMIC_AND64(target, val) _InterlockedAnd64(&target, val)
