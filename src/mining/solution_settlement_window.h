#pragma once

#include "platform/assert.h"
#include "platform/concurrency.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
#include "platform/console_logging.h"
#include "platform/file_io.h"
#include "platform/debugging.h"
#include "kangaroo_twelve.h"
#include "public_settings.h"

// Deferred settlement of mining solutions.
// Phase 1 (tick processor): append() captures a solution into a slot. Workers take slots in append
// order through one shared cursor and write a result. Phase 2 (tick processor): settleTick() walks a
// tick's slots in ascending txIndex and calls each kind's settle hook, the only place consensus state
// is touched; a slot still unscored then is scored inline.
//
// Storage is one append-order arena: _head is the oldest unsettled slot, _cursor the next a worker
// takes, _tail the next free. Appends and settlement both run in (tick, txIndex) order, so a tick's
// slots are one contiguous run, found through _tickIndex.
//
// Writers:
//  - tick processor   append(): record, then the tick index count last. settleTick(): _head, the
//                     index count (cleared), _settledThroughTick.
//  - any worker       result, then scored last. Never a record field.
//
// A settled slot is released, not scrubbed: a pointer taken earlier reads the same bytes until an
// append reuses it.

// Kinds are the solution transaction types. Each registers two hooks; the loops here call them.
struct SettlementSlot;
typedef void (*SolutionScoreFn)(SettlementSlot& slot, unsigned long long processorNumber);
// processorNumber is whichever processor runs settleTick/drainAll. A settle hook indexing
// per-processor scratch indexes it with this value.
typedef void (*SolutionSettleFn)(const SettlementSlot& slot, unsigned long long processorNumber);

static constexpr unsigned int MAX_SOLUTION_KINDS = 4;

// Every transaction of every tick in the window, plus the tick being processed, can be a
// solution, so append() cannot run out of room while settlement keeps up.
static constexpr unsigned long long SOLUTION_SETTLEMENT_CAPACITY =
    (unsigned long long)(SOLUTION_SETTLEMENT_WINDOW_TICKS + 1) * NUMBER_OF_TRANSACTIONS_PER_TICK;

// Per-tick index, addressed tick & (size - 1): the smallest power of two holding the window plus the
// tick being processed, every tick that can be unsettled at once. settleTick() must keep up with that;
// a tick that would alias a still-unsettled one is refused by append().
static constexpr unsigned int solutionSettlementTickIndexSize(unsigned int window)
{
    unsigned int size = 1;
    while (size < window + 1u)
    {
        size <<= 1;
    }
    return size;
}
static constexpr unsigned int SOLUTION_SETTLEMENT_TICK_INDEX_SIZE = solutionSettlementTickIndexSize(SOLUTION_SETTLEMENT_WINDOW_TICKS);
static constexpr unsigned int SOLUTION_SETTLEMENT_TICK_INDEX_MASK = SOLUTION_SETTLEMENT_TICK_INDEX_SIZE - 1;

// Records per chunk file on save. Bounds the serial scratch the save gathers into.
static constexpr unsigned int SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS = 65536;

// What phase 1 captured. This is the part of a slot that is hashed and saved: identical on every
// node, never written after append().
struct SettlementRecord
{
    unsigned int tick;               // ABSOLUTE system tick the transaction executed in
    unsigned int txIndex;            // its index in that tick; settlement order within the tick
    unsigned short inputType;        // the kind
    unsigned short recordSize;       // bytes of record[] the kind filled; the rest is zero
    unsigned char record[MAX_SOLUTION_RECORD_BYTES];
};
static_assert(sizeof(SettlementRecord) == 4 + 4 + 2 + 2 + MAX_SOLUTION_RECORD_BYTES, "SettlementRecord unexpected padding");

struct SettlementSlot : public SettlementRecord
{
    unsigned char result[MAX_SOLUTION_RESULT_BYTES];   // what a worker produced; node-local, recomputable
    volatile char scored;                              // written last by whoever scored the slot
    unsigned char reserved[3];
};
static_assert(sizeof(SettlementSlot) == sizeof(SettlementRecord) + MAX_SOLUTION_RESULT_BYTES + 4, "SettlementSlot unexpected padding");

// One K12 step of the window digest: the running value chained with the next record.
struct SettlementDigestFold
{
    unsigned int previous;
    SettlementRecord record;
};
static_assert(sizeof(SettlementDigestFold) == 4 + sizeof(SettlementRecord), "SettlementDigestFold unexpected padding");

struct SolutionSettlementWindowMeta
{
    unsigned int epoch;
    unsigned int settledThroughTick;
    unsigned int recordCount;
    unsigned int windowDigest;                  // chained K12 of the records, in order
    unsigned int resourceTestingDigestAtSave;   // pairs the file with the node state saved beside it
    // Layout guards. A file from a build with another record size, window or chunking is refused.
    unsigned int recordBytes;
    unsigned int windowTicks;
    unsigned int chunkRecords;
};
static_assert(sizeof(SolutionSettlementWindowMeta) == 32, "SolutionSettlementWindowMeta unexpected padding");

struct SolutionSettlementStats
{
    unsigned long long appended;
    unsigned long long refused;
    unsigned long long scoredByWorker;
    unsigned long long scoredInline;    // backstop: scored on the tick processor at settlement
    unsigned long long scoredByDrain;   // scored on the tick processor by drainAll(), alongside the workers
    unsigned long long stalledOnWorker; // settlement waited on a slot a worker had already claimed
    unsigned long long settled;
    unsigned long long drains;
};

static void solutionSettlementNameFilesForEpoch(unsigned short epoch)
{
    addEpochToFileName(SOLUTION_SETTLEMENT_HEADER_FILENAME,
        sizeof(SOLUTION_SETTLEMENT_HEADER_FILENAME) / sizeof(SOLUTION_SETTLEMENT_HEADER_FILENAME[0]), epoch);
    addEpochToFileName(SOLUTION_SETTLEMENT_RECORDS_FILENAME,
        sizeof(SOLUTION_SETTLEMENT_RECORDS_FILENAME) / sizeof(SOLUTION_SETTLEMENT_RECORDS_FILENAME[0]), epoch);
}

// Chunk file name: the epoch-suffixed records name plus a three-digit chunk id. `out` holds 64.
static void solutionSettlementRecordsChunkName(CHAR16* out, unsigned int chunkId)
{
    setText(out, SOLUTION_SETTLEMENT_RECORDS_FILENAME);
    appendText(out, L".XXX");
    addEpochToFileName(out, getTextSize(out, 64) + 1, (short)chunkId);
}

static void solutionSettlementFailure(const CHAR16* what, unsigned long long a, unsigned long long b)
{
    CHAR16 message[256];
    setText(message, L"[settlement] ");
    appendText(message, what);
    appendText(message, L" ");
    appendNumber(message, a, FALSE);
    appendText(message, L" / ");
    appendNumber(message, b, FALSE);
    logToConsole(message);
}

class SolutionSettlementWindow
{
public:
    bool init();
    void deinit();

    // Empty the window, mid-flight slots included; ticks <= settledThroughTick count as settled. Only
    // safe with no worker scoring, which would keep writing into a reused slot: drain first.
    void reset(unsigned int settledThroughTick);

    // false: no hook, a duplicate inputType, or the registry is full.
    bool registerKind(unsigned short inputType, SolutionScoreFn score, SolutionSettleFn settle);

    // Phase 1, tick processor. Refuses (false) a tick already settled, a tick behind the newest
    // appended one, a txIndex not above the tick's last, an unregistered kind, an oversized record,
    // an index entry still held by an unsettled tick, or a full arena. Every refusal is a function
    // of the appended sequence alone, so all nodes refuse the same transactions.
    bool append(unsigned int tick, unsigned int txIndex, unsigned short inputType,
        const void* record, unsigned int size);

    // Workers. Take the next unclaimed slot and score it; false when there is none.
    bool workerStep(unsigned long long processorNumber);
    unsigned int unscoredCount() const;

    // Any thread. nullptr once the tick is settled or if (tick, txIndex) was never appended.
    const SettlementSlot* lookup(unsigned int tick, unsigned int txIndex) const;

    // Phase 2, tick processor. Settles every unsettled tick up to and including tick, ascending;
    // no-op when tick <= settledThroughTick.
    void settleTick(unsigned int tick, unsigned long long processorNumber);
    // Epoch end: score everything left alongside the workers, wait for their in-flight
    // slots, settle every outstanding tick. The arena is empty afterwards.
    void drainAll(unsigned long long processorNumber);
    unsigned int settledThroughTick() const;
    unsigned int unsettledCount() const;

    // Persistence: records of the unsettled slots only. Results are recomputed after a load.
    unsigned int windowDigest() const;
    bool save(unsigned short epoch, CHAR16* directory, unsigned int resourceTestingDigestNow) const;
    // false means refuse to start: the header does not pair with the node state loaded beside it,
    // a file is missing or short, a record fails admission, or the records do not hash to the header.
    bool load(unsigned short epoch, CHAR16* directory, unsigned int resourceTestingDigestLoaded);

    const SolutionSettlementStats& stats() const
    {
        return _stats;
    }
    void appendStatusTo(CHAR16* message) const;

private:
    struct SolutionKind
    {
        unsigned short inputType;
        SolutionScoreFn score;
        SolutionSettleFn settle;
    };

    // A tick's run in the arena. count is written last on append and cleared on settlement, so a
    // reader that sees count > 0 sees complete slots.
    struct TickIndexEntry
    {
        unsigned int tick;
        unsigned int count;
        unsigned long long startIdx;   // logical arena index of the run's first slot
    };

    const SolutionKind* findKind(unsigned short inputType) const;
    SettlementSlot& slotAt(unsigned long long logicalIdx) const;
    SettlementSlot* claimNextUnscored(bool byWorker);
    void scoreSlot(SettlementSlot& slot, unsigned long long processorNumber);
    void settleOneTick(unsigned int tick, unsigned long long processorNumber);

    SettlementSlot* _slots;
    TickIndexEntry* _tickIndex;
    unsigned char* _scratch;

    volatile char _lock;                     // _head, _cursor, _tail
    unsigned long long _head;
    unsigned long long _cursor;
    unsigned long long _tail;
    volatile long long _unscored;
    volatile unsigned int _settledThroughTick;
    unsigned int _latestAppendedTick;

    SolutionKind _kinds[MAX_SOLUTION_KINDS];
    unsigned int _kindCount;
    SolutionSettlementStats _stats;
};

inline bool SolutionSettlementWindow::init()
{
    setMem(this, sizeof(*this), 0);

    if (!allocPoolWithErrorLog(L"SolutionSettlementWindow::_slots",
        SOLUTION_SETTLEMENT_CAPACITY * sizeof(SettlementSlot), (void**)&_slots, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"SolutionSettlementWindow::_tickIndex",
        (unsigned long long)SOLUTION_SETTLEMENT_TICK_INDEX_SIZE * sizeof(TickIndexEntry), (void**)&_tickIndex, __LINE__))
    {
        return false;
    }
    if (!allocPoolWithErrorLog(L"SolutionSettlementWindow::_scratch",
        (unsigned long long)SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS * sizeof(SettlementRecord), (void**)&_scratch, __LINE__))
    {
        return false;
    }

    reset(0);
    return true;
}

inline void SolutionSettlementWindow::deinit()
{
    if (_scratch)
    {
        freePool(_scratch);
    }
    if (_tickIndex)
    {
        freePool(_tickIndex);
    }
    if (_slots)
    {
        freePool(_slots);
    }
    _scratch = nullptr;
    _tickIndex = nullptr;
    _slots = nullptr;
}

inline void SolutionSettlementWindow::reset(unsigned int settledThroughTick)
{
    ASSERT(_tickIndex != nullptr);

    LockGuard guard(_lock);
    setMem(_tickIndex, (unsigned long long)SOLUTION_SETTLEMENT_TICK_INDEX_SIZE * sizeof(TickIndexEntry), 0);
    _head = 0;
    _cursor = 0;
    _tail = 0;
    _unscored = 0;
    _settledThroughTick = settledThroughTick;
    _latestAppendedTick = settledThroughTick;
    setMem(&_stats, sizeof(_stats), 0);
}

inline bool SolutionSettlementWindow::registerKind(unsigned short inputType, SolutionScoreFn score, SolutionSettleFn settle)
{
    if (score == nullptr || settle == nullptr)
    {
        return false;
    }
    if (findKind(inputType) != nullptr)
    {
        return false;
    }
    if (_kindCount >= MAX_SOLUTION_KINDS)
    {
        return false;
    }
    _kinds[_kindCount].inputType = inputType;
    _kinds[_kindCount].score = score;
    _kinds[_kindCount].settle = settle;
    _kindCount++;
    return true;
}

inline const SolutionSettlementWindow::SolutionKind* SolutionSettlementWindow::findKind(unsigned short inputType) const
{
    for (unsigned int i = 0; i < _kindCount; i++)
    {
        if (_kinds[i].inputType == inputType)
        {
            return &_kinds[i];
        }
    }
    return nullptr;
}

inline SettlementSlot& SolutionSettlementWindow::slotAt(unsigned long long logicalIdx) const
{
    return _slots[logicalIdx % SOLUTION_SETTLEMENT_CAPACITY];
}

inline bool SolutionSettlementWindow::append(unsigned int tick, unsigned int txIndex, unsigned short inputType,
    const void* record, unsigned int size)
{
    ASSERT(_slots != nullptr);

    if (size > MAX_SOLUTION_RECORD_BYTES || txIndex >= NUMBER_OF_TRANSACTIONS_PER_TICK)
    {
        _stats.refused++;
        return false;
    }
    if (tick <= _settledThroughTick || tick < _latestAppendedTick)
    {
        _stats.refused++;
        return false;
    }
    if (findKind(inputType) == nullptr)
    {
        _stats.refused++;
        return false;
    }

    TickIndexEntry& entry = _tickIndex[tick & SOLUTION_SETTLEMENT_TICK_INDEX_MASK];
    // Still held by an unsettled tick one index length back: refusing keeps that tick's slots intact.
    if (entry.count > 0 && entry.tick != tick)
    {
        _stats.refused++;
        return false;
    }

    const bool continuesTick = (entry.count > 0);
    // A tick's run stays ascending in txIndex: lookup() binary-searches it and settlement walks it in that order.
    if (continuesTick && txIndex <= slotAt(entry.startIdx + entry.count - 1).txIndex)
    {
        _stats.refused++;
        return false;
    }

    // The tick processor is the only writer of _head and _tail, so this read needs no lock.
    if (_tail - _head >= SOLUTION_SETTLEMENT_CAPACITY)
    {
        _stats.refused++;
        return false;
    }

    // Filled before _tail advances, so no worker can claim the slot yet; the lock orders the write
    // before any claim.
    SettlementSlot& slot = slotAt(_tail);
    setMem(&slot, sizeof(slot), 0);
    slot.tick = tick;
    slot.txIndex = txIndex;
    slot.inputType = inputType;
    slot.recordSize = (unsigned short)size;
    copyMem(slot.record, record, size);

    LockGuard guard(_lock);
    if (!continuesTick)
    {
        entry.tick = tick;
        entry.startIdx = _tail;
    }
    _tail++;
    ATOMIC_INC64(_unscored);
    ATOMIC_STORE32(entry.count, (long)(entry.count + 1));
    _latestAppendedTick = tick;
    _stats.appended++;
    return true;
}

inline SettlementSlot* SolutionSettlementWindow::claimNextUnscored(bool byWorker)
{
    LockGuard guard(_lock);
    if (_cursor >= _tail)
    {
        return nullptr;
    }
    SettlementSlot* slot = &slotAt(_cursor);
    _cursor++;
    if (byWorker)
    {
        _stats.scoredByWorker++;
    }
    else
    {
        _stats.scoredByDrain++;
    }
    return slot;
}

inline void SolutionSettlementWindow::scoreSlot(SettlementSlot& slot, unsigned long long processorNumber)
{
    const SolutionKind* kind = findKind(slot.inputType);
    ASSERT(kind != nullptr);
    if (kind != nullptr)
    {
        kind->score(slot, processorNumber);
    }
    ATOMIC_STORE8(slot.scored, 1);
    ATOMIC_ADD64(_unscored, -1);
}

inline bool SolutionSettlementWindow::workerStep(unsigned long long processorNumber)
{
    SettlementSlot* slot = claimNextUnscored(true);
    if (slot == nullptr)
    {
        return false;
    }
    scoreSlot(*slot, processorNumber);
    return true;
}

inline unsigned int SolutionSettlementWindow::unscoredCount() const
{
    const long long unscored = _unscored;
    ASSERT(unscored >= 0);
    return (unsigned int)unscored;
}

inline const SettlementSlot* SolutionSettlementWindow::lookup(unsigned int tick, unsigned int txIndex) const
{
    if (_slots == nullptr)
    {
        return nullptr;
    }
    const TickIndexEntry& entry = _tickIndex[tick & SOLUTION_SETTLEMENT_TICK_INDEX_MASK];
    const unsigned int count = (unsigned int)ATOMIC_LOAD32(entry.count);
    if (count == 0 || entry.tick != tick)
    {
        return nullptr;
    }
    const unsigned long long startIdx = entry.startIdx;

    unsigned int low = 0;
    unsigned int high = count;
    while (low < high)
    {
        const unsigned int mid = low + (high - low) / 2;
        const SettlementSlot& candidate = slotAt(startIdx + mid);
        if (candidate.txIndex < txIndex)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }
    if (low >= count)
    {
        return nullptr;
    }
    // The entry may have been released and reused between the reads above; the slot's own fields
    // are the ground truth.
    const SettlementSlot& found = slotAt(startIdx + low);
    if (found.tick != tick || found.txIndex != txIndex)
    {
        return nullptr;
    }
    return &found;
}

inline void SolutionSettlementWindow::settleTick(unsigned int tick, unsigned long long processorNumber)
{
    if (tick <= _settledThroughTick)
    {
        return;
    }
    for (unsigned int t = _settledThroughTick + 1; ; t++)
    {
        settleOneTick(t, processorNumber);
        _settledThroughTick = t;
        if (t == tick)
        {
            break;
        }
    }
}

inline void SolutionSettlementWindow::settleOneTick(unsigned int tick, unsigned long long processorNumber)
{
    TickIndexEntry& entry = _tickIndex[tick & SOLUTION_SETTLEMENT_TICK_INDEX_MASK];
    if (entry.count == 0 || entry.tick != tick)
    {
        return;
    }
    ASSERT(entry.startIdx == _head);
    const unsigned long long first = entry.startIdx;
    const unsigned long long end = first + entry.count;

    // Slots no worker has taken yet are scored here. Moving the cursor past the run first means no
    // worker can take one of them afterwards.
    unsigned long long claimedFrom = end;
    {
        LockGuard guard(_lock);
        if (_cursor < end)
        {
            claimedFrom = _cursor;
            _cursor = end;
        }
    }

    for (unsigned long long i = first; i < end; i++)
    {
        SettlementSlot& slot = slotAt(i);
        ASSERT(slot.tick == tick);
        ASSERT(i == first || slot.txIndex > slotAt(i - 1).txIndex);

        if (!slot.scored)
        {
            if (i >= claimedFrom)
            {
                scoreSlot(slot, processorNumber);
                _stats.scoredInline++;
            }
            else
            {
                // A worker holds it and is at most one score away. Counted apart from scoredInline:
                // it stalls the tick processor too.
                _stats.stalledOnWorker++;
                while (!slot.scored)
                {
                    _mm_pause();
                }
            }
        }

        const SolutionKind* kind = findKind(slot.inputType);
        ASSERT(kind != nullptr);
        if (kind != nullptr)
        {
            kind->settle(slot, processorNumber);
        }
        _stats.settled++;
    }

    {
        LockGuard guard(_lock);
        _head = end;
    }
    ATOMIC_STORE32(entry.count, 0);
}

inline void SolutionSettlementWindow::drainAll(unsigned long long processorNumber)
{
    // Score alongside the workers; with them parked this is the only scorer.
    SettlementSlot* slot = claimNextUnscored(false);
    while (slot != nullptr)
    {
        scoreSlot(*slot, processorNumber);
        slot = claimNextUnscored(false);
    }
    while (unscoredCount() > 0)
    {
        _mm_pause();
    }
    settleTick(_latestAppendedTick, processorNumber);
    ASSERT(_head == _tail);
    _stats.drains++;
}

inline unsigned int SolutionSettlementWindow::settledThroughTick() const
{
    return _settledThroughTick;
}

inline unsigned int SolutionSettlementWindow::unsettledCount() const
{
    return (unsigned int)(_tail - _head);
}

inline unsigned int SolutionSettlementWindow::windowDigest() const
{
    unsigned int digest = 0;
    SettlementDigestFold fold;
    for (unsigned long long i = _head; i < _tail; i++)
    {
        fold.previous = digest;
        const SettlementRecord* record = &slotAt(i);
        copyMem(&fold.record, record, sizeof(SettlementRecord));
        KangarooTwelve(&fold, sizeof(fold), &digest, sizeof(digest));
    }
    return digest;
}

inline bool SolutionSettlementWindow::save(unsigned short epoch, CHAR16* directory, unsigned int resourceTestingDigestNow) const
{
    ASSERT(_slots != nullptr);
    ASSERT(_scratch != nullptr);

    solutionSettlementNameFilesForEpoch(epoch);

    const unsigned long long recordCount = _tail - _head;
    ASSERT(recordCount <= SOLUTION_SETTLEMENT_CAPACITY);

    SolutionSettlementWindowMeta meta;
    setMem(&meta, sizeof(meta), 0);
    meta.epoch = epoch;
    meta.settledThroughTick = _settledThroughTick;
    meta.recordCount = (unsigned int)recordCount;
    meta.windowDigest = windowDigest();
    meta.resourceTestingDigestAtSave = resourceTestingDigestNow;
    meta.recordBytes = (unsigned int)sizeof(SettlementRecord);
    meta.windowTicks = SOLUTION_SETTLEMENT_WINDOW_TICKS;
    meta.chunkRecords = SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS;
    if (::save(SOLUTION_SETTLEMENT_HEADER_FILENAME, sizeof(meta), (const unsigned char*)&meta, directory) != (long long)sizeof(meta))
    {
        logToConsole(L"[settlement] failed to save window header");
        return false;
    }

    // Chunk 0 is written even when the window is empty, with one zero record, so the file set is
    // always the same shape and a short one means a lost file.
    unsigned long long written = 0;
    unsigned int chunkId = 0;
    do
    {
        unsigned long long inChunk = recordCount - written;
        if (inChunk > SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS)
        {
            inChunk = SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS;
        }
        const unsigned long long slotsInFile = (inChunk > 0) ? inChunk : 1ULL;
        const unsigned long long bytes = slotsInFile * sizeof(SettlementRecord);
        setMem(_scratch, bytes, 0);
        for (unsigned long long i = 0; i < inChunk; i++)
        {
            const SettlementRecord* record = &slotAt(_head + written + i);
            copyMem(_scratch + i * sizeof(SettlementRecord), record, sizeof(SettlementRecord));
        }

        CHAR16 chunkName[64];
        solutionSettlementRecordsChunkName(chunkName, chunkId);
        if (::save(chunkName, bytes, _scratch, directory) != (long long)bytes)
        {
            solutionSettlementFailure(L"failed to save records chunk, chunk/records", chunkId, inChunk);
            return false;
        }
        written += inChunk;
        chunkId++;
    } while (written < recordCount);

    return true;
}

inline bool SolutionSettlementWindow::load(unsigned short epoch, CHAR16* directory, unsigned int resourceTestingDigestLoaded)
{
    ASSERT(_slots != nullptr);
    ASSERT(_scratch != nullptr);

    solutionSettlementNameFilesForEpoch(epoch);

    // Everything up to reset() only reads the header, so a refusal there leaves the window untouched.
    SolutionSettlementWindowMeta meta;
    if (::load(SOLUTION_SETTLEMENT_HEADER_FILENAME, sizeof(meta), (unsigned char*)&meta, directory) != (long long)sizeof(meta))
    {
        logToConsole(L"[settlement] failed to load window header");
        return false;
    }
    if (meta.epoch != epoch)
    {
        solutionSettlementFailure(L"epoch mismatch, file/expected", meta.epoch, epoch);
        return false;
    }
    if (meta.recordBytes != sizeof(SettlementRecord)
        || meta.windowTicks != SOLUTION_SETTLEMENT_WINDOW_TICKS
        || meta.chunkRecords != SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS)
    {
        solutionSettlementFailure(L"layout mismatch, recordBytes/windowTicks", meta.recordBytes, meta.windowTicks);
        return false;
    }
    if (meta.recordCount > SOLUTION_SETTLEMENT_CAPACITY)
    {
        solutionSettlementFailure(L"recordCount exceeds the capacity, count/capacity", meta.recordCount, SOLUTION_SETTLEMENT_CAPACITY);
        return false;
    }
    // The window is saved inside the same bracket as the node state. A different digest means the
    // two files are from different moments, and settling one against the other would misfold.
    if (meta.resourceTestingDigestAtSave != resourceTestingDigestLoaded)
    {
        solutionSettlementFailure(L"resourceTestingDigest does not match the restored node state, file/node",
            meta.resourceTestingDigestAtSave, resourceTestingDigestLoaded);
        return false;
    }

    reset(meta.settledThroughTick);

    // Each record goes back in through append(), so a file that breaks the order, the tick index or
    // the capacity is refused by the same rules a live append is.
    unsigned long long loaded = 0;
    unsigned int chunkId = 0;
    do
    {
        unsigned long long inChunk = meta.recordCount - loaded;
        if (inChunk > SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS)
        {
            inChunk = SOLUTION_SETTLEMENT_SAVE_CHUNK_RECORDS;
        }
        const unsigned long long slotsInFile = (inChunk > 0) ? inChunk : 1ULL;
        const unsigned long long bytes = slotsInFile * sizeof(SettlementRecord);

        CHAR16 chunkName[64];
        solutionSettlementRecordsChunkName(chunkName, chunkId);
        if (::load(chunkName, bytes, _scratch, directory) != (long long)bytes)
        {
            solutionSettlementFailure(L"failed to load records chunk, chunk/records", chunkId, inChunk);
            reset(meta.settledThroughTick);
            return false;
        }
        for (unsigned long long i = 0; i < inChunk; i++)
        {
            SettlementRecord record;
            copyMem(&record, _scratch + i * sizeof(SettlementRecord), sizeof(record));
            if (!append(record.tick, record.txIndex, record.inputType, record.record, record.recordSize))
            {
                solutionSettlementFailure(L"record refused, tick/txIndex", record.tick, record.txIndex);
                reset(meta.settledThroughTick);
                return false;
            }
        }
        loaded += inChunk;
        chunkId++;
    } while (loaded < meta.recordCount);

    const unsigned int digest = windowDigest();
    if (digest != meta.windowDigest)
    {
        solutionSettlementFailure(L"records do not hash to the header, file/computed", meta.windowDigest, digest);
        reset(meta.settledThroughTick);
        return false;
    }

    // Loaded slots are unscored and unclaimed: append() left them so, and the cursor is at _head.
    ASSERT(_cursor == _head);
    ASSERT(unscoredCount() == meta.recordCount);
    setMem(&_stats, sizeof(_stats), 0);
    return true;
}

inline void SolutionSettlementWindow::appendStatusTo(CHAR16* message) const
{
    appendText(message, L"settled through ");
    appendNumber(message, _settledThroughTick, FALSE);
    appendText(message, L" | pending ");
    appendNumber(message, unsettledCount(), TRUE);
    appendText(message, L" (unscored ");
    appendNumber(message, unscoredCount(), TRUE);
    appendText(message, L") | appended ");
    appendNumber(message, _stats.appended, TRUE);
    appendText(message, L", refused ");
    appendNumber(message, _stats.refused, TRUE);
    appendText(message, L" | scored by worker ");
    appendNumber(message, _stats.scoredByWorker, TRUE);
    appendText(message, L", inline ");
    appendNumber(message, _stats.scoredInline, TRUE);
    appendText(message, L", by drain ");
    appendNumber(message, _stats.scoredByDrain, TRUE);
    appendText(message, L", stalled ");
    appendNumber(message, _stats.stalledOnWorker, TRUE);
    appendText(message, L" | settled ");
    appendNumber(message, _stats.settled, TRUE);
    appendText(message, L" | drains ");
    appendNumber(message, _stats.drains, FALSE);
}
