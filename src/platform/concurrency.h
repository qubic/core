#pragma once

#include <lib/platform_common/qintrin.h>

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
    void pause();
};

// Acquire lock, may block and may log if it is blocked for a long time
#define ACQUIRE(lock) \
    if (_InterlockedCompareExchange8(&lock, 1, 0)) { \
        BusyWaitingTracker bwt(#lock, __FILE__, __LINE__); \
        while (_InterlockedCompareExchange8(&lock, 1, 0)) \
            bwt.pause(); \
    }

#endif

// Try to acquire lock and return if successful (without blocking)
#define TRY_ACQUIRE(lock) (_InterlockedCompareExchange8(&lock, 1, 0) == 0)

// Release lock
#define RELEASE(lock) lock = 0


#ifdef NDEBUG

// Begin waiting loop (with short expected waiting time). Outputs to debug.log if waiting long and NDEBUG isn't defined.
#define BEGIN_WAIT_WHILE(condition) \
    while (condition) {

// End waiting loop, corresponding to BEGIN_WAIT_WHILE().
#define END_WAIT_WHILE() _mm_pause(); }

#else

// Begin waiting loop (with short expected waiting time). Outputs to debug.log if waiting long and NDEBUG isn't defined.
#define BEGIN_WAIT_WHILE(condition) \
    if (condition) { \
        BusyWaitingTracker bwt(#condition, __FILE__, __LINE__); \
        while (condition) {

// End waiting loop, corresponding to BEGIN_WAIT_WHILE().
#define END_WAIT_WHILE() bwt.pause(); } }

#endif


// Waiting loop with short expected waiting time. Outputs to debug.log if waiting long and NDEBUG isn't defined.
#define WAIT_WHILE(condition) \
    BEGIN_WAIT_WHILE(condition) \
    END_WAIT_WHILE()

#define ATOMIC_STORE8(target, val) _InterlockedExchange8(&target, val)
#define ATOMIC_INC64(target) _InterlockedIncrement64(&target)
#define ATOMIC_AND64(target, val) _InterlockedAnd64(&target, val)
#define ATOMIC_STORE64(target, val) _InterlockedExchange64(&target, val)
#define ATOMIC_LOAD64(target) _InterlockedCompareExchange64(&target, 0, 0)
#define ATOMIC_ADD64(target, val) _InterlockedExchangeAdd64(&target, val)
#define ATOMIC_MAX64(target, val) atomicMax64(&target, val)
