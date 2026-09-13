#pragma once

// Parallel K12 leaf hashing for large contract state digests.
//
// getComputerDigest() hashes every contract state that changed since the last tick. K12 hashes its
// input as a tree: the first K12_chunkSize bytes go into the final node, every further full chunk
// is an independent leaf whose 32-byte chaining value is absorbed by the final node, and only the
// final node is sequential. For a 600 MB state that is ~75000 independent leaves and 2.4 MB of
// sequential work, so the leaves are split into tasks that the request processors pick up in their
// loop (like parallelSignVotes) while the tick processor claims the rest and then races stragglers.
//
// The result is bit-identical to KangarooTwelve(state, size, out, 32): KangarooTwelveLeaf() performs
// exactly the leaf steps of the one-shot code and KangarooTwelveStream absorbs the chaining values in
// leaf order. Tasks are idempotent (every core computes the same values), each core hashes into a
// private scratch buffer, and only the winner of the done-CAS stores its result, so a straggler and
// the ticker racing the same task never tear the output. A round is published only when no worker
// is inside the pool any more (busy counter), and the task fields are published before the
// generation is bumped, so a worker that copied stale fields always fails the generation check and
// never stores a stale result.
//
// Disable via USE_PARALLEL_K12_LEAVES in private_settings.h; getComputerDigest() then hashes on
// the tick processor as before.

#include "../kangaroo_twelve.h"
#include "../platform/memory.h"

#if defined(_MSC_VER)
#define PK12_CAS32(target, exchange, comparand) _InterlockedCompareExchange((volatile long*)(target), (exchange), (comparand))
#define PK12_ADD32(target, value) _InterlockedExchangeAdd((volatile long*)(target), (value))
#define PK12_COMPILER_BARRIER() _ReadWriteBarrier()
#else
#define PK12_CAS32(target, exchange, comparand) __sync_val_compare_and_swap((volatile long*)(target), (comparand), (exchange))
#define PK12_ADD32(target, value) __sync_fetch_and_add((volatile long*)(target), (value))
#define PK12_COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#endif

// States below this size are hashed on the tick processor alone: the dispatch is not worth it.
#define PARALLEL_K12_LEAVES_MIN_STATE_SIZE (4 * 1024 * 1024)

template <unsigned int LEAVES_PER_TASK_ = 256, unsigned int MAX_TASKS_ = 1024>
struct ParallelK12LeafTasksT
{
    static constexpr unsigned int LEAVES_PER_TASK = LEAVES_PER_TASK_;   // 2 MiB of input per task
    static constexpr unsigned int MAX_TASKS = MAX_TASKS_;               // 2 GiB per round; larger states take several rounds

    struct alignas(64) Task
    {
        const unsigned char* leaves;
        unsigned char* chainingValues;
        unsigned int leafCount;
        volatile long claimed;   // 0 = free, 1 = a core owns it
        volatile long done;      // 0 = in progress, 1 = chaining values stored
    };

    Task tasks[MAX_TASKS];
    volatile long taskCount = 0;
    volatile long completedCount = 0;
    volatile long active = 0;       // 1 while a round is published
    volatile long busy = 0;         // workers currently inside tryProcessOne()
    volatile long generation = 0;   // bumped per round, after the task fields are published
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
        for (unsigned int i = 0; i < MAX_TASKS; i++)
        {
            tasks[i].leaves = nullptr;
            tasks[i].chainingValues = nullptr;
            tasks[i].leafCount = 0;
            tasks[i].claimed = 0;
            tasks[i].done = 0;
        }
        taskCount = 0;
        completedCount = 0;
        active = 0;
        busy = 0;
        generation = 0;
    }

    // Request processor: hash one task if any is unclaimed, otherwise race an unfinished one.
    // Returns true if it did work.
    bool tryProcessOne()
    {
        if (!active)
        {
            return false;
        }
        PK12_ADD32(&busy, 1);
        bool didWork = false;
        if (active) // re-check after registering as busy: the ticker waits for busy == 0 while active == 0
        {
            const long n = taskCount;
            for (long i = 0; i < n && !didWork; i++)
            {
                if (PK12_CAS32(&tasks[i].claimed, 1, 0) == 0)
                {
                    processTask((unsigned int)i);
                    didWork = true;
                }
            }
            for (long i = 0; i < n && !didWork; i++)
            {
                if (tasks[i].done == 0)
                {
                    processTask((unsigned int)i);
                    didWork = true;
                }
            }
        }
        PK12_ADD32(&busy, -1);
        return didWork;
    }

    // Tick processor: digest of `size` bytes of state, bit-identical to KangarooTwelve(state, size, out, 32).
    // The first chunk and the trailing partial leaf are fed on this core, all full leaves in between are
    // hashed by hashLeaves(). Caller holds the state's read lock for the whole call.
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

    // Tick processor: chaining values of leafCount consecutive full leaves into chainingValues (32 bytes
    // each), using the request processors that call tryProcessOne() plus this core. Returns when all
    // chaining values are stored.
    void hashLeaves(const unsigned char* leaves, unsigned long long leafCount, unsigned char* chainingValues)
    {
        while (leafCount)
        {
            // no worker may still be inside the previous round when its slots are reused
            while (busy)
            {
                _mm_pause();
            }
            // publish one round of tasks: fields first, then the generation, then active
            long n = 0;
            while (leafCount && n < (long)MAX_TASKS)
            {
                const unsigned int count = leafCount < LEAVES_PER_TASK ? (unsigned int)leafCount : LEAVES_PER_TASK;
                tasks[n].leaves = leaves;
                tasks[n].chainingValues = chainingValues;
                tasks[n].leafCount = count;
                tasks[n].claimed = 0;
                tasks[n].done = 0;
                leaves += (unsigned long long)count * K12_chunkSize;
                chainingValues += (unsigned long long)count * K12_capacityInBytes;
                leafCount -= count;
                n++;
            }
            completedCount = 0;
            taskCount = n;
            PK12_ADD32(&generation, 1); // full barrier: fields are visible before the new generation
            active = 1;

            // claim what is free, then race stragglers until every task is done
            for (long i = 0; i < n; i++)
            {
                if (PK12_CAS32(&tasks[i].claimed, 1, 0) == 0)
                {
                    processTask((unsigned int)i);
                }
            }
            while (completedCount < n)
            {
                long straggler = -1;
                for (long i = 0; i < n; i++)
                {
                    if (tasks[i].done == 0)
                    {
                        straggler = i;
                        break;
                    }
                }
                if (straggler >= 0)
                {
                    processTask((unsigned int)straggler);
                }
                else
                {
                    _mm_pause();
                }
            }
            active = 0;
            taskCount = 0;
        }
    }

private:
    // Hash one task into a private scratch buffer; the done-CAS winner stores it. Bails early if
    // another core finished the task meanwhile.
    void processTask(unsigned int i)
    {
        // read the generation before the fields: fields are published before the generation is bumped,
        // so stale fields always come with a stale generation and the check below rejects the result
        const long myGeneration = generation;
        PK12_COMPILER_BARRIER();
        const unsigned char* leaves = tasks[i].leaves;
        unsigned char* chainingValues = tasks[i].chainingValues;
        const unsigned int leafCount = tasks[i].leafCount;
        unsigned char scratch[LEAVES_PER_TASK * K12_capacityInBytes];
        for (unsigned int k = 0; k < leafCount; k++)
        {
            if (tasks[i].done)
            {
                return;
            }
            KangarooTwelveLeaf(leaves + (unsigned long long)k * K12_chunkSize, scratch + k * K12_capacityInBytes);
        }
        PK12_COMPILER_BARRIER();
        if (generation != myGeneration)
        {
            return;
        }
        if (PK12_CAS32(&tasks[i].done, 1, 0) == 0)
        {
            copyMem(chainingValues, scratch, leafCount * K12_capacityInBytes);
            PK12_ADD32(&completedCount, 1);
        }
    }
};

typedef ParallelK12LeafTasksT<> ParallelK12LeafTasks;

static ParallelK12LeafTasks parallelK12Leaves;
