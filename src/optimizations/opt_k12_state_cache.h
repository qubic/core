#pragma once

// Per-contract K12 chaining-value caches fed by hardware dirty-page tracking.
//
// Large contract states (>= PARALLEL_K12_LEAVES_MIN_STATE_SIZE) get a K12ChainingValueCache and, if
// the page tables could be split, dirty tracking through PageTables. The contract processor is the
// only core that writes a tracked state while the node ticks, so it also consumes the Dirty bits:
// getComputerDigest() asks it to run collect() (phase COLLECT_DIRTY_STATE_PAGES) before hashing.
// Anything that writes a state on another core (state load, migration, IPO bids by the tick
// processor, snapshot restore) is covered by invalidateAll() or by the IPO-epoch rule in the caller.
//
// Every failure (unmapped page, sample mismatch) stops tracking that contract for good and falls
// back to the untracked full-hash path; the digest is a pure function of the state bytes either way.

#include "../network_messages/common_def.h"
#include "opt_parallel_k12_leaves.h"
#include "../platform/page_tables.h"
#include "../platform/memory_util.h"

struct K12StateCacheSet
{
    struct Entry
    {
        K12ChainingValueCache cache;
        unsigned char* state = nullptr;
        unsigned long long size = 0;
        void* chainingValueBuffer = nullptr;
        void* dirtyBitmapBuffer = nullptr;
        bool tracked = false; // dirty bits are collected from the page tables
    };

    Entry entries[MAX_NUMBER_OF_CONTRACTS];
    PageTables pageTables;
    bool hardwareTracking = false;

    // statistics for the health log
    unsigned long long collections = 0;
    unsigned long long dirtyPagesCollected = 0;
    unsigned long long cachedDigests = 0;
    unsigned long long fullRebuilds = 0;      // cached digests that had to rehash every leaf
    unsigned long long trackingFailures = 0;  // contracts dropped from tracking
    unsigned long long sampleMismatches = 0;
    unsigned long long fullCheckMismatches = 0;

    void init()
    {
        hardwareTracking = pageTables.init();
    }

    // Give contract `index` a cache and try to track it. Returns false only on allocation failure.
    bool add(unsigned int index, unsigned char* state, unsigned long long size)
    {
        Entry& e = entries[index];
        e.state = state;
        e.size = size;
        const unsigned long long cvSize = K12ChainingValueCache::chainingValuesSize(size);
        const unsigned long long bmSize = K12ChainingValueCache::dirtyBitmapSize(size);
        if (!allocPoolWithErrorLog(L"K12StateCache::chainingValues", cvSize ? cvSize : 1, &e.chainingValueBuffer, __LINE__)
            || !allocPoolWithErrorLog(L"K12StateCache::dirtyBitmap", bmSize ? bmSize : 8, &e.dirtyBitmapBuffer, __LINE__))
        {
            return false;
        }
        e.cache.init((unsigned char*)e.chainingValueBuffer, (unsigned long long*)e.dirtyBitmapBuffer, size);
        e.tracked = hardwareTracking && pageTables.splitRange(state, size);
        if (!e.tracked)
        {
            trackingFailures++;
        }
        return true;
    }

    bool isCached(unsigned int index) const
    {
        return entries[index].chainingValueBuffer != nullptr;
    }

    bool isTracked(unsigned int index) const
    {
        return entries[index].tracked;
    }

    void untrack(unsigned int index)
    {
        Entry& e = entries[index];
        if (e.tracked)
        {
            e.tracked = false;
            trackingFailures++;
        }
        e.cache.invalidate();
    }

    void invalidateAll()
    {
        for (unsigned int i = 0; i < MAX_NUMBER_OF_CONTRACTS; i++)
        {
            if (entries[i].chainingValueBuffer)
            {
                entries[i].cache.invalidate();
            }
        }
    }

    // Contract processor only: consume the Dirty bits of every tracked contract whose change flag is
    // set, turning written pages into dirty leaves. A collection failure untracks the contract.
    void collect(const unsigned long long* contractStateChangeFlags)
    {
        collections++;
        for (unsigned int i = 0; i < MAX_NUMBER_OF_CONTRACTS; i++)
        {
            Entry& e = entries[i];
            if (!e.tracked || !(contractStateChangeFlags[i >> 6] & (1ULL << (i & 63))))
            {
                continue;
            }
            unsigned long long pages = 0;
            const bool ok = pageTables.collectDirty(e.state, e.size, [&](unsigned long long page) {
                e.cache.markDirtyBytes(page * PageTables::PAGE, PageTables::PAGE);
                pages++;
            });
            dirtyPagesCollected += pages;
            if (!ok)
            {
                untrack(i);
            }
        }
    }

    // Tick processor: cached digest of contract `index` through the leaf pool. Returns false if the
    // cache was not usable and the caller should take the untracked path instead.
    template <class Pool>
    bool digest(Pool& pool, unsigned int index, void* output32)
    {
        Entry& e = entries[index];
        if (!e.tracked)
        {
            return false;
        }
        if (!e.cache.valid)
        {
            fullRebuilds++;
        }
        pool.digest(e.state, e.size, e.cache, output32);
        cachedDigests++;
        return true;
    }

    // Tick processor, after digest(): recompute a few clean leaves and compare. A mismatch means a write
    // slipped past the dirty tracking; the contract is untracked so the next digest rehashes everything.
    bool verifySample(unsigned int index, unsigned long long sampleCount, unsigned long long seed)
    {
        Entry& e = entries[index];
        if (!e.tracked)
        {
            return true;
        }
        if (e.cache.verifySample(e.state, sampleCount, seed))
        {
            sampleMismatches++;
            untrack(index);
            return false;
        }
        return true;
    }

    void deinit()
    {
        for (unsigned int i = 0; i < MAX_NUMBER_OF_CONTRACTS; i++)
        {
            if (entries[i].chainingValueBuffer)
            {
                freePool(entries[i].chainingValueBuffer);
            }
            if (entries[i].dirtyBitmapBuffer)
            {
                freePool(entries[i].dirtyBitmapBuffer);
            }
            setMem(&entries[i], sizeof(Entry), 0);
        }
    }
};
