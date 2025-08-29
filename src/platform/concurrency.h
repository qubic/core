#pragma once

#include <lib/platform_common/qintrin.h>

#ifdef __linux__
#define _byteswap_ulong bswap_32
#define _InterlockedExchange8(target, val) __atomic_exchange_n(target, val, __ATOMIC_SEQ_CST)
#define _InterlockedIncrement64(target) __atomic_add_fetch(target, 1, __ATOMIC_SEQ_CST)
#define _InterlockedAnd64(target, val) __atomic_fetch_and(target, val, __ATOMIC_SEQ_CST)
#define _InterlockedExchange64(target, val) __atomic_exchange_n(target, val, __ATOMIC_SEQ_CST)
long long _InterlockedCompareExchange64(volatile long long *target, long long exchange, long long comparand) {
    return __atomic_compare_exchange_n(target, &comparand, exchange, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
char _InterlockedCompareExchange8(volatile char *target, char exchange, char comparand) {
    return __atomic_compare_exchange_n(target, &comparand, exchange, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
long _InterlockedCompareExchange(volatile long *target, long exchange, long comparand) {
    return __atomic_compare_exchange_n(target, &comparand, exchange, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
#define _InterlockedExchangeAdd64(target, val) __atomic_fetch_add(target, val, __ATOMIC_SEQ_CST)
#define _interlockedadd64 _InterlockedExchangeAdd64
#define _InterlockedDecrement(target) __atomic_sub_fetch(target, 1, __ATOMIC_SEQ_CST)
#define _InterlockedIncrement(target) __atomic_add_fetch(target, 1, __ATOMIC_SEQ_CST)
#endif

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
