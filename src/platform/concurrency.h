#pragma once

#include <intrin.h>

// Acquire lock, may block
#define ACQUIRE_WITHOUT_DEBUG_LOGGING(lock) while (_InterlockedCompareExchange8(&lock, 1, 0)) _mm_pause()

#ifdef NDEBUG

// Acquire lock, may block
#define ACQUIRE(lock) ACQUIRE_WITHOUT_DEBUG_LOGGING(lock)

#else

// Emit output if waiting long
class BusyWaitingTracker
{
    unsigned long long mStartTsc;
    unsigned long long mNextReportTscDelta;
    const char* mExpr;
    const char* mFile;
    unsigned int mLine;
    bool mTotalWaitTimeReport;
public:
    BusyWaitingTracker(const char* expr, const char* file, unsigned int line);
    ~BusyWaitingTracker();
    void wait();
};

// Acquire lock, may block and may log if it is blocked for a long time
#define ACQUIRE(lock) \
    if (_InterlockedCompareExchange8(&lock, 1, 0)) { \
        BusyWaitingTracker bwt(#lock, __FILE__, __LINE__); \
        while (_InterlockedCompareExchange8(&lock, 1, 0)) \
            bwt.wait(); \
    }

#endif

// Try to acquire lock and return if successful (without blocking)
#define TRY_ACQUIRE(lock) (_InterlockedCompareExchange8(&lock, 1, 0) == 0)

// Release lock
#define RELEASE(lock) lock = 0

#define ATOMIC_STORE8(target, val) _InterlockedExchange8(&target, val)
#define ATOMIC_INC64(target) _InterlockedIncrement64(&target)
#define ATOMIC_AND64(target, val) _InterlockedAnd64(&target, val)
#define ATOMIC_STORE64(target, val) _InterlockedExchange64(&target, val)
