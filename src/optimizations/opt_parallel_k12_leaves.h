#pragma once

// Parallel K12 leaf hashing for large contract state digests.
//
// getComputerDigest() hashes every contract state that changed since the last tick. K12 hashes its
// input as a tree: the first K12_chunkSize bytes go into the final node, every further full chunk
// is an independent leaf whose 32-byte chaining value is absorbed by the final node, and only the
// final node is sequential. For a 600 MB state that is ~75000 independent leaves and 2.4 MB of
// sequential work, so the leaves are handed out in fixed-size tasks that the request processors
// pick up in their loop (like parallelSignVotes) while the tick processor takes the rest.
//
// The result is bit-identical to KangarooTwelve(state, size, out, 32): KangarooTwelveLeaf() performs
// exactly the leaf steps of the one-shot code and KangarooTwelveStream absorbs the chaining values in
// leaf order.
//
// Ownership is exclusive: a task is claimed by an atomic counter, hashed by exactly one core straight
// into its slot of the chaining-value buffer, and never redone. A leaf hash is deterministic and takes
// microseconds, and application processors are not preempted under UEFI, so racing a claimed task
// could not finish it earlier anyway; it would only duplicate work. The tail of a digest is therefore
// bounded by one task of a helper's time. digest() returns only when every worker has left the pool,
// because the caller releases the state's read lock right after.
//
// Workers see a consistent (range, counter) pair without a generation counter: a range is published
// only while active == 0 and after the previous digest drained busy to 0, and a worker registers as
// busy before it re-checks active and reads the range.
//
// Disable via USE_PARALLEL_K12_LEAVES in private_settings.h; getComputerDigest() then hashes on
// the tick processor as before. PARALLEL_K12_LEAVES_MAX_HELPERS caps the request processors that
// hash at the same time; the others keep serving requests.

#include "../kangaroo_twelve.h"
#include "../platform/memory.h"

#if defined(_MSC_VER)
#define PK12_ADD32(target, value) _InterlockedExchangeAdd((volatile long*)(target), (value))
#else
#define PK12_ADD32(target, value) __sync_fetch_and_add((volatile long*)(target), (value))
#endif

// States below this size are hashed on the tick processor alone: the dispatch is not worth it.
#ifndef PARALLEL_K12_LEAVES_MIN_STATE_SIZE
#define PARALLEL_K12_LEAVES_MIN_STATE_SIZE (4 * 1024 * 1024)
#endif

// Request processors hashing at the same time. Scaling flattens between 8 and 16 and every helper
// stops handling requests while it hashes.
#ifndef PARALLEL_K12_LEAVES_MAX_HELPERS
#define PARALLEL_K12_LEAVES_MAX_HELPERS 8
#endif

template <unsigned int TASK_LEAVES_ = 64, unsigned int MAX_HELPERS_ = PARALLEL_K12_LEAVES_MAX_HELPERS>
struct ParallelK12LeafTasksT
{
    static constexpr unsigned int TASK_LEAVES = TASK_LEAVES_;   // 512 KiB of input per task, ~0.4 ms
    static constexpr unsigned int MAX_HELPERS = MAX_HELPERS_;

    // the published range: task i covers leaves [i * TASK_LEAVES, (i + 1) * TASK_LEAVES)
    const unsigned char* volatile leaves = nullptr;
    unsigned char* volatile chainingValues = nullptr;
    volatile long taskCount = 0;
    volatile long lastTaskLeaves = 0;   // leaves in the last task (1..TASK_LEAVES)

    volatile long nextTask = 0;         // claim counter
    volatile long completedCount = 0;
    volatile long active = 0;           // 1 while a range is published
    volatile long busy = 0;             // workers currently inside the pool (ticker excluded)

    unsigned char* chainingValueBuffer = nullptr;   // room for chainingValueBufferSize(maxStateSize) bytes
    unsigned long long maxStateSize = 0;

    static unsigned long long chainingValueBufferSize(unsigned long long maxStateSize)
    {
        return (maxStateSize / K12_chunkSize + 1) * K12_capacityInBytes;
    }

    void init(unsigned char* buffer, unsigned long long maxStateSize_)
    {
        chainingValueBuffer = buffer;
        maxStateSize = maxStateSize_;
        leaves = nullptr;
        chainingValues = nullptr;
        taskCount = 0;
        lastTaskLeaves = 0;
        nextTask = 0;
        completedCount = 0;
        active = 0;
        busy = 0;
    }

    // Request processor: hash one task if any is left and fewer than MAX_HELPERS are hashing.
    // Returns true if it did work.
    bool tryProcessOne()
    {
        if (!active)
        {
            return false;
        }
        if (PK12_ADD32(&busy, 1) >= (long)MAX_HELPERS)
        {
            PK12_ADD32(&busy, -1);
            return false;
        }
        bool didWork = false;
        if (active) // re-check after registering as busy: the ticker publishes only while busy == 0
        {
            didWork = claimAndProcessOne();
        }
        PK12_ADD32(&busy, -1);
        return didWork;
    }

    // Tick processor: digest of `size` bytes of state, bit-identical to KangarooTwelve(state, size, out, 32).
    // The first chunk and the trailing partial leaf are fed on this core, all full leaves in between are
    // hashed by hashLeaves(). Caller holds the state's read lock for the whole call; no worker reads the
    // state after this returns.
    void digest(const unsigned char* state, unsigned long long size, void* output32)
    {
        ASSERT(size <= maxStateSize);
        KangarooTwelveStream stream;
        stream.init();
        if (size <= K12_chunkSize)
        {
            stream.update(state, size);
            stream.finalize(output32);
            return;
        }
        const unsigned long long leafCount = (size - K12_chunkSize) / K12_chunkSize;
        hashLeaves(state + K12_chunkSize, leafCount, chainingValueBuffer);

        stream.update(state, K12_chunkSize);
        for (unsigned long long k = 0; k < leafCount; k++)
        {
            stream.absorbChainingValue(chainingValueBuffer + k * K12_capacityInBytes);
        }
        const unsigned long long tail = K12_chunkSize + leafCount * K12_chunkSize;
        stream.update(state + tail, size - tail);
        stream.finalize(output32);
    }

    // Tick processor: chaining values of leafCount consecutive full leaves into chainingValues_ (32 bytes
    // each), using this core plus the request processors that call tryProcessOne(). Returns when all
    // chaining values are stored and no worker is inside the pool any more.
    void hashLeaves(const unsigned char* leaves_, unsigned long long leafCount, unsigned char* chainingValues_)
    {
        if (!leafCount)
        {
            return;
        }
        ASSERT(!active && !busy);
        leaves = leaves_;
        chainingValues = chainingValues_;
        taskCount = (long)((leafCount + TASK_LEAVES - 1) / TASK_LEAVES);
        lastTaskLeaves = (long)(leafCount - (unsigned long long)(taskCount - 1) * TASK_LEAVES);
        nextTask = 0;
        completedCount = 0;
        PK12_ADD32(&active, 1); // full barrier: the range is visible before workers see active

        while (claimAndProcessOne())
        {
        }
        while (completedCount < taskCount)
        {
            _mm_pause();
        }
        active = 0;
        // a worker may still be between its busy++ and its active re-check: let it leave
        while (busy)
        {
            _mm_pause();
        }
    }

private:
    bool claimAndProcessOne()
    {
        const long i = PK12_ADD32(&nextTask, 1);
        if (i >= taskCount)
        {
            return false;
        }
        const unsigned int count = (i == taskCount - 1) ? (unsigned int)lastTaskLeaves : TASK_LEAVES;
        const unsigned char* leaf = leaves + (unsigned long long)i * TASK_LEAVES * K12_chunkSize;
        unsigned char* chainingValue = chainingValues + (unsigned long long)i * TASK_LEAVES * K12_capacityInBytes;
        for (unsigned int k = 0; k < count; k++)
        {
            KangarooTwelveLeaf(leaf + (unsigned long long)k * K12_chunkSize, chainingValue + k * K12_capacityInBytes);
        }
        PK12_ADD32(&completedCount, 1);
        return true;
    }
};

typedef ParallelK12LeafTasksT<> ParallelK12LeafTasks;

static ParallelK12LeafTasks parallelK12Leaves;
