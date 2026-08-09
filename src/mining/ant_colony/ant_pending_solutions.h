#pragma once

#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/memory.h"
#include "mining/ant_colony/ant_colony.h"

// Solutions waiting to be published as transactions signed by the node's own computors
// A queue is needed because a broadcast can be lost with nothing reporting it. An entry stores the
// tick its transaction was targeted at, if it is not on-chain by then, publish again.
struct AntPendingSolution
{
    m256i computorPublicKey;
    m256i nonce;
    SolutionRef parentRef;
    unsigned int anchorTick;      // ABSOLUTE. Bounds how long this entry is worth publishing.
    unsigned int score;           // computed at receipt; the publisher uses it without re-scoring
};
static_assert(sizeof(AntPendingSolution) == 32 + 32 + 8 + 8, "AntPendingSolution unexpected padding");

class AntPendingSolutions
{
public:
    static constexpr unsigned int CAPACITY = 65536;
    static_assert((CAPACITY & (CAPACITY - 1)) == 0, "CAPACITY must be a power of two");
    static constexpr unsigned int NO_ENTRY = 0xFFFFFFFFU;

    // publicationTick[] states. A positive value is the tick the transaction was targeted at, which
    // is also the deadline to see it on-chain
    static constexpr int NOT_SCHEDULED = 0;
    static constexpr int RECORDED = -1;   // observed on-chain; never publish again
    static constexpr int OBSOLETE = -2;   // can never land; stop occupying the retry slot

    struct Stats
    {
        unsigned long long received;
        unsigned long long droppedNonCanonical;
        unsigned long long droppedBadAnchor;      // anchor in the future, or aged out of the ring
        unsigned long long droppedParentUnknown;  // parentRef names a node this node does not hold
        unsigned long long droppedUnscorable;     // the scorer returned no usable value
        unsigned long long droppedUnacceptable;   // scored, but the colony would reject it now
        unsigned long long droppedDuplicate;
        unsigned long long droppedFull;
        unsigned long long published;
        unsigned long long recorded;
        unsigned long long obsoleteParentGone;
        unsigned long long obsoleteExpired;
        unsigned long long obsoleteGateRejected;
        unsigned long long claimMismatch;
    };

    bool init()
    {
        setMem(this, sizeof(*this), 0);
        if (!allocPoolWithErrorLog(L"AntPendingSolutions::_entries",
            CAPACITY * sizeof(AntPendingSolution), (void**)&_entries, __LINE__))
        {
            return false;
        }
        if (!allocPoolWithErrorLog(L"AntPendingSolutions::_publicationTick",
            CAPACITY * sizeof(int), (void**)&_publicationTick, __LINE__))
        {
            return false;
        }
        if (!allocPoolWithErrorLog(L"AntPendingSolutions::_index",
            INDEX_CAPACITY * sizeof(unsigned int), (void**)&_index, __LINE__))
        {
            return false;
        }
        reset();
        return true;
    }

    void deinit()
    {
        if (_index)
        {
            freePool(_index);
        }
        if (_publicationTick)
        {
            freePool(_publicationTick);
        }
        if (_entries)
        {
            freePool(_entries);
        }
        _index = nullptr;
        _publicationTick = nullptr;
        _entries = nullptr;
    }

    void reset()
    {
        ASSERT(_entries != nullptr);
        LockGuard guard(_lock);
        setMem(_entries, CAPACITY * sizeof(AntPendingSolution), 0);
        setMem(_publicationTick, CAPACITY * sizeof(int), 0);
        setMem(_index, INDEX_CAPACITY * sizeof(unsigned int), 0xFF);
        _count = 0;
        _nextFree = 0;
        _touchedSlots = 0;
        setMem(&_stats, sizeof(_stats), 0);
    }

    // Ingress only counts what it drops, the caller decides what is worth dropping, because the
    // reasons live where the colony can be consulted.
    void noteReceived()
    {
        LockGuard guard(_lock);
        _stats.received++;
    }
    void noteDroppedNonCanonical()
    {
        LockGuard guard(_lock);
        _stats.droppedNonCanonical++;
    }
    void noteDroppedBadAnchor()
    {
        LockGuard guard(_lock);
        _stats.droppedBadAnchor++;
    }
    void noteDroppedParentUnknown()
    {
        LockGuard guard(_lock);
        _stats.droppedParentUnknown++;
    }
    void noteDroppedDuplicate()
    {
        LockGuard guard(_lock);
        _stats.droppedDuplicate++;
    }
    void noteDroppedUnscorable()
    {
        LockGuard guard(_lock);
        _stats.droppedUnscorable++;
    }
    void noteDroppedUnacceptable()
    {
        LockGuard guard(_lock);
        _stats.droppedUnacceptable++;
    }

    void getStats(Stats& outStats, unsigned int& outCount) const
    {
        LockGuard guard(_lock);
        outStats = _stats;
        outCount = _count;
    }

    // Queue a solution for publication, called from request processors.
    bool add(const m256i& computorPublicKey, const SolutionRef& parentRef,
        unsigned int anchorTick, unsigned int score, const m256i& nonce)
    {
        LockGuard guard(_lock);
        const unsigned int slot = indexSlotFor(computorPublicKey, parentRef, nonce);
        unsigned int entryIdx = NO_ENTRY;
        if (_index[slot] != INDEX_EMPTY)
        {
            // An OBSOLETE entry is not a duplicate
            // Every other state is a real duplicate: NOT_SCHEDULED and scheduled are still live, and
            // RECORDED already landed, so a resend would be rejected as a replay anyway.
            if (_publicationTick[_index[slot]] != OBSOLETE)
            {
                _stats.droppedDuplicate++;
                return false;
            }
            entryIdx = _index[slot];
        }
        else
        {
            entryIdx = findFreeEntry();
            if (entryIdx == NO_ENTRY)
            {
                _stats.droppedFull++;
                return false;
            }
        }

        AntPendingSolution& e = _entries[entryIdx];
        e.computorPublicKey = computorPublicKey;
        e.nonce = nonce;
        e.parentRef = parentRef;
        e.anchorTick = anchorTick;
        e.score = score;
        _publicationTick[entryIdx] = NOT_SCHEDULED;
        if (_index[slot] == INDEX_EMPTY)
        {
            _index[slot] = entryIdx;
            _count++;
        }
        return true;
    }

    // Pick the next solution this computor should publish, or NO_ENTRY.
    // Anything already past its publish window is retired here rather than published
    unsigned int selectForPublish(const m256i& computorPublicKey, unsigned int currentTick,
        AntPendingSolution& outEntry)
    {
        LockGuard guard(_lock);

        unsigned int found = scanForPublish(computorPublicKey, currentTick, true);
        if (found == NO_ENTRY)
        {
            found = scanForPublish(computorPublicKey, currentTick, false);
        }
        if (found != NO_ENTRY)
        {
            outEntry = _entries[found];
        }
        return found;
    }

    void markScheduled(unsigned int index, int publicationTick)
    {
        LockGuard guard(_lock);
        if (index >= CAPACITY || _publicationTick[index] < 0)
        {
            return;
        }
        _publicationTick[index] = publicationTick;
        _stats.published++;
    }

    void markObsoleteParentGone(unsigned int index)
    {
        retire(index, _stats.obsoleteParentGone);
    }
    void markObsoleteExpired(unsigned int index)
    {
        retire(index, _stats.obsoleteExpired);
    }
    void markObsoleteGateRejected(unsigned int index)
    {
        retire(index, _stats.obsoleteGateRejected);
    }

    void noteClaimMismatch()
    {
        LockGuard guard(_lock);
        _stats.claimMismatch++;
    }

    void markRecorded(const m256i& computorPublicKey, const SolutionRef& parentRef, const m256i& nonce)
    {
        LockGuard guard(_lock);

        const unsigned int slot = indexSlotFor(computorPublicKey, parentRef, nonce);
        if (_index[slot] != INDEX_EMPTY)
        {
            _publicationTick[_index[slot]] = RECORDED;
            _stats.recorded++;
            return;
        }

        const unsigned int entryIdx = findFreeEntry();
        if (entryIdx == NO_ENTRY)
        {
            // Nothing to reclaim
            return;
        }

        AntPendingSolution& e = _entries[entryIdx];
        e.computorPublicKey = computorPublicKey;
        e.nonce = nonce;
        e.parentRef = parentRef;
        e.anchorTick = 0;
        e.score = 0;
        _publicationTick[entryIdx] = RECORDED;
        _index[slot] = entryIdx;
        _count++;
        _stats.recorded++;
    }

private:
    static constexpr unsigned int INDEX_CAPACITY = 2 * CAPACITY;
    // Not INDEX_EMPTY: network_messages/assets.h defines that as a macro, and this header is included
    // after it in qubic.cpp.
    static constexpr unsigned int INDEX_EMPTY = 0xFFFFFFFFU;

    // A slot is free if it has never been used or if its entry is finished. Reuse is IN PLACE:
    // nothing is ever moved, so an index the tick processor is holding across the publish gate
    // stays valid.
    unsigned int findFreeEntry()
    {
        for (unsigned int i = 0; i < CAPACITY; i++)
        {
            const unsigned int idx = (_nextFree + i) & (CAPACITY - 1);
            if (isZero(_entries[idx].computorPublicKey))
            {
                claim(idx);
                return idx;
            }
            if (_publicationTick[idx] == RECORDED || _publicationTick[idx] == OBSOLETE)
            {
                indexRemove(_entries[idx]);
                _count--;
                claim(idx);
                return idx;
            }
        }
        return NO_ENTRY;
    }

    void claim(unsigned int idx)
    {
        _nextFree = (idx + 1) & (CAPACITY - 1);
        if (idx + 1 > _touchedSlots)
        {
            _touchedSlots = idx + 1;
        }
    }

    unsigned int scanForPublish(const m256i& computorPublicKey, unsigned int currentTick, bool retries)
    {
        for (unsigned int i = 0; i < _touchedSlots; i++)
        {
            const int state = _publicationTick[i];
            if (retries)
            {
                if (state <= NOT_SCHEDULED || state > (int)currentTick)
                {
                    continue;
                }
            }
            else if (state != NOT_SCHEDULED)
            {
                continue;
            }
            if (isZero(_entries[i].computorPublicKey) || !(_entries[i].computorPublicKey == computorPublicKey))
            {
                continue;
            }
            if (currentTick - _entries[i].anchorTick > ANT_PUBLISH_WINDOW_TICKS)
            {
                retireLocked(i, _stats.obsoleteExpired);
                continue;
            }
            return i;
        }
        return NO_ENTRY;
    }

    void retire(unsigned int index, unsigned long long& counter)
    {
        LockGuard guard(_lock);
        retireLocked(index, counter);
    }

    void retireLocked(unsigned int index, unsigned long long& counter)
    {
        if (index >= CAPACITY || _publicationTick[index] < 0)
        {
            return;
        }
        _publicationTick[index] = OBSOLETE;
        counter++;
    }

    static AntDedupKey keyOf(const m256i& computorPublicKey, const SolutionRef& parentRef,
        const m256i& nonce)
    {
        AntDedupKey key;
        key.pubkey = computorPublicKey;
        key.nonce = nonce;
        key.parentRef = parentRef;
        return key;
    }

    // Reads only the low words would put every nonce differing above them in one slot, which is the
    // clustering the index exists to avoid.
    static unsigned int hashOf(const m256i& computorPublicKey, const SolutionRef& parentRef,
        const m256i& nonce)
    {
        const AntDedupKey key = keyOf(computorPublicKey, parentRef, nonce);
        unsigned long long digest;
        KangarooTwelve(&key, sizeof(key), &digest, sizeof(digest));
        return (unsigned int)(digest & (INDEX_CAPACITY - 1));
    }

    // Returns the slot holding this key, or the first free slot if it is absent. Linear probing over
    // a table at most half full, so the walk always terminates.
    unsigned int indexSlotFor(const m256i& computorPublicKey, const SolutionRef& parentRef,
        const m256i& nonce) const
    {
        unsigned int slot = hashOf(computorPublicKey, parentRef, nonce);
        for (unsigned int probe = 0; probe < INDEX_CAPACITY; probe++)
        {
            const unsigned int e = _index[slot];
            if (e == INDEX_EMPTY)
            {
                return slot;
            }
            if (keyOf(_entries[e].computorPublicKey, _entries[e].parentRef, _entries[e].nonce)
                == keyOf(computorPublicKey, parentRef, nonce))
            {
                return slot;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
        }
        return 0;
    }

    void indexRemove(const AntPendingSolution& entry)
    {
        const unsigned int slot = indexSlotFor(entry.computorPublicKey, entry.parentRef, entry.nonce);
        if (_index[slot] == INDEX_EMPTY)
        {
            return;
        }
        _index[slot] = INDEX_EMPTY;

        // Re-place the run that followed it, or a probe would stop early at the hole just made.
        unsigned int next = (slot + 1) & (INDEX_CAPACITY - 1);
        while (_index[next] != INDEX_EMPTY)
        {
            const unsigned int moved = _index[next];
            _index[next] = INDEX_EMPTY;
            const unsigned int target = indexSlotFor(_entries[moved].computorPublicKey,
                _entries[moved].parentRef, _entries[moved].nonce);
            _index[target] = moved;
            next = (next + 1) & (INDEX_CAPACITY - 1);
        }
    }

    AntPendingSolution* _entries;
    int* _publicationTick;
    unsigned int* _index;
    unsigned int _count;
    unsigned int _nextFree;
    unsigned int _touchedSlots;
    Stats _stats;
    mutable volatile char _lock;
};
