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
// busy before it re-checks active and reads the range. Closing admission (active = 0) and registering
// (busy++) are both locked instructions, i.e. full barriers, so either the ticker sees the worker's
// busy and waits for it, or the worker sees active == 0 and leaves: a plain store for active = 0
// could stay in the store buffer while the ticker already reads busy == 0, letting a worker slip
// into a range that is about to be replaced.
//
// K12ChainingValueCache keeps the chaining values of one state between digests: with a dirty-leaf
// bitmap filled by the caller, digest(state, size, cache, out) rehashes only the written leaves and
// absorbs the cached rest, still bit-identical to the one-shot digest. See the struct for the invariant.
//
// Disable via USE_PARALLEL_K12_LEAVES in private_settings.h; getComputerDigest() then hashes on
// the tick processor as before. PARALLEL_K12_LEAVES_MAX_HELPERS caps the request processors that
// hash at the same time; the others keep serving requests.

#include "../kangaroo_twelve.h"
#include "../platform/memory.h"

// Both are full memory barriers on x86 (locked instructions); the protocol below relies on that.
#if defined(_MSC_VER)
#define PK12_ADD32(target, value) _InterlockedExchangeAdd((volatile long*)(target), (value))
#define PK12_XCHG32(target, value) _InterlockedExchange((volatile long*)(target), (value))
#else
#define PK12_ADD32(target, value) __sync_fetch_and_add((volatile long*)(target), (value))
#define PK12_XCHG32(target, value) __atomic_exchange_n((volatile long*)(target), (value), __ATOMIC_SEQ_CST)
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

// Persistent chaining values of one contract state. The K12 tree shape is fixed: after the first
// K12_chunkSize bytes, every full K12_chunkSize leaf yields a 32-byte chaining value that depends on
// that leaf alone. Keeping them between ticks turns the digest into "rehash the leaves that were
// written, then absorb all chaining values", with the first chunk and the trailing partial leaf always
// re-read from the state. The result stays bit-identical to KangarooTwelve(state, size, out, 32).
//
// Invariant the caller must uphold: every byte written since the last digest() has either been reported
// via markDirtyBytes()/markDirtyLeaf() or the cache has been invalidate()d. Writes to the first chunk
// or the tail need no report. Dirty bits stay set until a digest() consumes them.
struct K12ChainingValueCache
{
    unsigned char* chainingValues = nullptr;    // leafCount * K12_capacityInBytes
    unsigned long long* dirtyLeaves = nullptr;  // one bit per leaf, wordCount(size) words
    unsigned long long leafCount = 0;
    unsigned long long stateSize = 0;
    volatile bool valid = false;                // false: every leaf is rehashed by the next digest()

    static unsigned long long leafCountOf(unsigned long long size)
    {
        return size > K12_chunkSize ? (size - K12_chunkSize) / K12_chunkSize : 0;
    }
    static unsigned long long wordCountOf(unsigned long long size)
    {
        return (leafCountOf(size) + 63) / 64;
    }
    static unsigned long long chainingValuesSize(unsigned long long size)
    {
        return leafCountOf(size) * K12_capacityInBytes;
    }
    static unsigned long long dirtyBitmapSize(unsigned long long size)
    {
        return wordCountOf(size) * sizeof(unsigned long long);
    }

    void init(unsigned char* chainingValueBuffer, unsigned long long* dirtyBitmapBuffer, unsigned long long size)
    {
        chainingValues = chainingValueBuffer;
        dirtyLeaves = dirtyBitmapBuffer;
        leafCount = leafCountOf(size);
        stateSize = size;
        invalidate();
    }

    // Forget everything: the next digest() rehashes all leaves. Use after any write that was not tracked
    // (state load, migration, snapshot restore, tracking failure, epoch transition).
    void invalidate()
    {
        valid = false;
    }

    void markDirtyLeaf(unsigned long long leaf)
    {
        ASSERT(leaf < leafCount);
        dirtyLeaves[leaf >> 6] |= (1ULL << (leaf & 63));
    }

    // Report a written byte range [offset, offset + length). Bytes outside the leaf region (first chunk,
    // tail) are ignored: they are always re-read.
    void markDirtyBytes(unsigned long long offset, unsigned long long length)
    {
        if (!length || !leafCount)
        {
            return;
        }
        const unsigned long long end = offset + length; // exclusive
        const unsigned long long leavesBegin = K12_chunkSize;
        const unsigned long long leavesEnd = K12_chunkSize + leafCount * K12_chunkSize;
        if (end <= leavesBegin || offset >= leavesEnd)
        {
            return;
        }
        const unsigned long long first = (offset > leavesBegin ? offset - leavesBegin : 0) / K12_chunkSize;
        const unsigned long long last = ((end < leavesEnd ? end : leavesEnd) - leavesBegin - 1) / K12_chunkSize;
        for (unsigned long long leaf = first; leaf <= last; leaf++)
        {
            markDirtyLeaf(leaf);
        }
    }

    bool isDirtyLeaf(unsigned long long leaf) const
    {
        return (dirtyLeaves[leaf >> 6] >> (leaf & 63)) & 1;
    }

    void clearDirty()
    {
        setMem(dirtyLeaves, dirtyBitmapSize(stateSize), 0);
    }

    // Debug/validation: recompute `sampleCount` pseudo-randomly chosen leaves that are currently marked
    // clean and compare against the cached chaining values. Returns the number of mismatches. A mismatch
    // means a write reached the state without being reported (broken dirty tracking).
    unsigned long long verifySample(const unsigned char* state, unsigned long long sampleCount, unsigned long long seed) const
    {
        if (!valid || !leafCount)
        {
            return 0;
        }
        unsigned long long mismatches = 0;
        for (unsigned long long k = 0; k < sampleCount; k++)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            const unsigned long long leaf = (seed >> 17) % leafCount;
            if (isDirtyLeaf(leaf))
            {
                continue;
            }
            unsigned char cv[K12_capacityInBytes];
            KangarooTwelveLeaf(state + K12_chunkSize + leaf * K12_chunkSize, cv);
            const unsigned char* cached = chainingValues + leaf * K12_capacityInBytes;
            for (unsigned int i = 0; i < K12_capacityInBytes; i++)
            {
                if (cv[i] != cached[i])
                {
                    mismatches++;
                    break;
                }
            }
        }
        return mismatches;
    }
};

template <unsigned int TASK_LEAVES_ = 64, unsigned int MAX_HELPERS_ = PARALLEL_K12_LEAVES_MAX_HELPERS>
struct ParallelK12LeafTasksT
{
    static constexpr unsigned int TASK_LEAVES = TASK_LEAVES_;   // 512 KiB of input per task, ~0.4 ms
    static constexpr unsigned int MAX_HELPERS = MAX_HELPERS_;

    // the published range: task i covers leaves [i * TASK_LEAVES, (i + 1) * TASK_LEAVES)
    const unsigned char* volatile leaves = nullptr;
    unsigned char* volatile chainingValues = nullptr;
    const unsigned long long* volatile dirtyLeaves = nullptr;   // null: every leaf of the range is hashed
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
        dirtyLeaves = nullptr;
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

    // Tick processor: digest of `size` bytes of state through a persistent cache, bit-identical to
    // KangarooTwelve(state, size, out, 32). Only leaves marked dirty in the cache (or all of them if the
    // cache is invalid) are rehashed; the first chunk and the trailing partial leaf are always re-read.
    // On return the cache is valid and its dirty bits are cleared. Same lock rule as digest() above.
    void digest(const unsigned char* state, unsigned long long size, K12ChainingValueCache& cache, void* output32)
    {
        ASSERT(size <= maxStateSize);
        ASSERT(cache.stateSize == size);
        ASSERT(cache.leafCount == K12ChainingValueCache::leafCountOf(size));
        KangarooTwelveStream stream;
        stream.init();
        if (size <= K12_chunkSize)
        {
            stream.update(state, size);
            stream.finalize(output32);
            cache.valid = true;
            return;
        }
        const unsigned long long leafCount = cache.leafCount;
        hashLeaves(state + K12_chunkSize, leafCount, cache.chainingValues, cache.valid ? cache.dirtyLeaves : nullptr);
        cache.clearDirty();
        cache.valid = true;

        stream.update(state, K12_chunkSize);
        for (unsigned long long k = 0; k < leafCount; k++)
        {
            stream.absorbChainingValue(cache.chainingValues + k * K12_capacityInBytes);
        }
        const unsigned long long tail = K12_chunkSize + leafCount * K12_chunkSize;
        stream.update(state + tail, size - tail);
        stream.finalize(output32);
    }

    // Tick processor: chaining values of leafCount consecutive full leaves into chainingValues_ (32 bytes
    // each), using this core plus the request processors that call tryProcessOne(). With a dirty bitmap
    // only the leaves whose bit is set are hashed, the other slots are left untouched. Returns when all
    // chaining values are stored and no worker is inside the pool any more.
    void hashLeaves(const unsigned char* leaves_, unsigned long long leafCount, unsigned char* chainingValues_, const unsigned long long* dirtyLeaves_ = nullptr)
    {
        if (!leafCount)
        {
            return;
        }
        ASSERT(!active);
        while (busy) // defensive: publication must never overlap a worker that is still inside
        {
            _mm_pause();
        }
        leaves = leaves_;
        chainingValues = chainingValues_;
        dirtyLeaves = dirtyLeaves_;
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
        PK12_XCHG32(&active, 0); // full barrier: close admission before reading busy
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
        const unsigned long long* dirty = dirtyLeaves;
        const unsigned long long firstLeaf = (unsigned long long)i * TASK_LEAVES;
        for (unsigned int k = 0; k < count; k++)
        {
            if (dirty)
            {
                const unsigned long long l = firstLeaf + k;
                if (!((dirty[l >> 6] >> (l & 63)) & 1))
                {
                    continue;
                }
            }
            KangarooTwelveLeaf(leaf + (unsigned long long)k * K12_chunkSize, chainingValue + k * K12_capacityInBytes);
        }
        PK12_ADD32(&completedCount, 1);
        return true;
    }
};

typedef ParallelK12LeafTasksT<> ParallelK12LeafTasks;

static ParallelK12LeafTasks parallelK12Leaves;
