#pragma once

// Parallel signTickVote with STRAGGLER RACING:
//
// signTickVote is a mandatory proof-of-work retry loop: signWithRandomK + verify until the
// signature score < TARGET_TICK_VOTE_SIGNATURE.
//
// This optimization splits the sign tasks one-per-index across the request-processor pool. 
// The makespan is TAIL-BOUND: each index is a SEQUENTIAL PoW loop, so the
// slowest single index (the worst geometric draw over all tries) is ground by ONE core
// alone while the others go idle. Adding cores cannot speed one sequential signature.
//
// Hence, idle workers have to RACE the in-flight stragglers. A signature is valid as soon as ANY
// random-K attempt passes the difficulty, and ANY valid signature is consensus-acceptable — so
// many cores can attempt the SAME index concurrently with different random K and the first valid
// one wins. Once a core finishes its owned task and finds nothing unclaimed, it picks an
// in-flight (not-done) index and races it.
//
// SAFETY (consensus-critical):
//  * Each racer signs into its OWN stack scratch buffer; only the winner of an atomic done-CAS
//    copies its scratch into the real signature slot -> no torn writes.
//  * completedCount is incremented exactly once per index (by the done-CAS winner) -> waitAll's
//    barrier is exact.
//  * The race loop bails the moment `done` is set (another core won this index) -> losers stop
//    promptly, no wasted grind into the next round.
//  * A `generation` counter (bumped each dispatch) guards the commit: a worker only writes +
//    counts if the generation is unchanged since it started -> an impossibly-late straggler from a
//    previous round can never corrupt the next round's slot. (The pool dispatches once per tick,
//    ~hundreds of ms apart, so this is belt-and-suspenders.)
//
// Disable via USE_PARALLEL_SIGN_VOTES in private_settings.h. The serial signTickVote() path
// (USE_PARALLEL_SIGN_VOTES=0, and the inline fallback) is unchanged.

#include "../platform/concurrency.h"
#include "../network_messages/common_def.h"

struct ParallelSignVoteTasks
{
    // One entry per own-computor-index for the current tick. Worker copies out everything it
    // needs before claiming; no race on the inputs after publish.
    struct alignas(64) Task
    {
        const unsigned char* subseed;
        const unsigned char* publicKey;
        const unsigned char* messageDigest;
        unsigned char* signature;   // winner writes the final signature here
        volatile int claimed;   // 0 = no owner yet, 1 = an owner has taken it
        volatile int done;      // 0 = in progress, 1 = a valid signature has been committed
    };

    // Racing sign function: attempt random-K signatures until one is valid (writes it to outSig
    // and returns true) OR `done` is observed set by another core (returns false, bail). Defined
    // in qubic.cpp where verifyTickVoteSignature / signWithRandomK are in scope.
    typedef bool (*SignFunc)(const unsigned char* subseed,
                             const unsigned char* publicKey,
                             const unsigned char* messageDigest,
                             unsigned char* outSig,
                             volatile int* done);

    static constexpr int MAX_TASKS = NUMBER_OF_COMPUTORS;

    Task tasks[MAX_TASKS];
    int taskCount = 0;
    volatile int completedCount{0};
    volatile bool active{false};
    volatile unsigned int generation{0};
    SignFunc func = nullptr;

    void init(SignFunc f)
    {
        func = f;
        for (int i = 0; i < MAX_TASKS; i++)
        {
            tasks[i].subseed = nullptr;
            tasks[i].publicKey = nullptr;
            tasks[i].messageDigest = nullptr;
            tasks[i].signature = nullptr;
            tasks[i].claimed = 0;
            tasks[i].done = 0;
        }
        taskCount = 0;
        completedCount = 0;
        active = false;
        generation = 0;
    }

    // Sign one index (as owner or as a racer). Winner of the done-CAS commits the signature and
    // counts it; losers/bailers do nothing. Generation-guarded so a stale worker can never write
    // into a later round.
    void processTask(int i, bool isTicker)
    {
        const unsigned int myGen = generation;
        unsigned char scratch[SIGNATURE_SIZE];
        if (func(tasks[i].subseed, tasks[i].publicKey, tasks[i].messageDigest, scratch, &tasks[i].done))
        {
            if (generation == myGen)
            {
                if (_InterlockedCompareExchange((long*)&tasks[i].done, 1, 0) == 0)
                {
                    copyMem(tasks[i].signature, scratch, SIGNATURE_SIZE);
                    _InterlockedExchangeAdd((long*)&completedCount, 1);
                    (void)isTicker;
                }
            }
        }
    }

    // Ticker: publish all tasks for this tick.
    void dispatchAll(int n)
    {
        _InterlockedExchangeAdd((long*)&generation, 1);
        for (int i = 0; i < n; i++)
        {
            tasks[i].claimed = 0;
            tasks[i].done = 0;
        }
        completedCount = 0;
        taskCount = n;
        active = true;
    }

    // Request processor: do one unit of sign work. Owns an unclaimed index if one is free,
    // otherwise RACES an in-flight straggler. Returns true if it did (or attempted) work.
    bool tryProcessOne()
    {
        if (!active) return false;
        const int n = taskCount;
        // Phase 1: become the owner of an unclaimed index.
        for (int i = 0; i < n; i++)
        {
            if (_InterlockedCompareExchange((long*)&tasks[i].claimed, 1, 0) == 0)
            {
                processTask(i, false);
                return true;
            }
        }
        // Phase 2: every index has an owner — collapse the tail by racing an in-flight one.
        for (int i = 0; i < n; i++)
        {
            if (tasks[i].done == 0)
            {
                processTask(i, false);
                return true;
            }
        }
        return false;
    }

    // Ticker: take any unclaimed indices, then help race in-flight stragglers until all done.
    void waitAll()
    {
        const int n = taskCount;
        for (int i = 0; i < n; i++)
        {
            if (_InterlockedCompareExchange((long*)&tasks[i].claimed, 1, 0) == 0)
            {
                processTask(i, true);
            }
        }
        while (completedCount < n)
        {
            int straggler = -1;
            for (int i = 0; i < n; i++)
            {
                if (tasks[i].done == 0)
                {
                    straggler = i;
                    break;
                }
            }
            if (straggler >= 0)
            {
                processTask(straggler, true);
            }
            else
            {
                _mm_pause();
            }
        }
        active = false;
        taskCount = 0;
    }
};

static ParallelSignVoteTasks parallelSignVotes;
