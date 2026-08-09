#pragma once

#include "mining/ant_colony/ant_colony_bpp9000.h"
#include "platform/file_io.h"

// Only what cannot be derived is written. The tick index, both head maps and the dedup set are
// rebuilt from the records

struct AntColonySnapshotMeta
{
    unsigned int magic;
    unsigned int version;
    unsigned int epoch;
    unsigned int solutionCount;
    // Layout guards. A snapshot written by a build with a different record or ANN size must be
    // refused rather than reinterpreted
    unsigned int recordSizeBytes;
    unsigned int annPoolEntryBytes;
    unsigned int errorThreshold;
    // The base every selfRef.tickOffset in the records file is relative to. Records address each
    // other by offset, so a snapshot restored against a different base is silently mis-addressed
    unsigned int initialTick;
    m256i rootSeed;

    static constexpr unsigned int MAGIC = 0x414E5443;   // "ANTC"
    static constexpr unsigned int VERSION = 1;
};
static_assert(sizeof(AntColonySnapshotMeta) == 32 + 32, "AntColonySnapshotMeta unexpected padding");

static void antSnapshotFailure(const CHAR16* what, unsigned long long a, unsigned long long b)
{
    CHAR16 message[256];
    setText(message, L"[ant-colony] snapshot: ");
    appendText(message, what);
    appendText(message, L" ");
    appendNumber(message, a, FALSE);
    appendText(message, L" / ");
    appendNumber(message, b, FALSE);
    logToConsole(message);
}

// Records and pool are sized by this, never by the raw count. At least one slot is always written,
// so no snapshot file is ever zero length
static unsigned long long antSnapshotSlotCount(unsigned int solutionCount)
{
    return (solutionCount > 0) ? (unsigned long long)solutionCount : 1ULL;
}

static void antSnapshotNameForEpoch(unsigned short epoch)
{
    addEpochToFileName(ANT_SNAPSHOT_META_FILENAME, sizeof(ANT_SNAPSHOT_META_FILENAME) / sizeof(ANT_SNAPSHOT_META_FILENAME[0]), epoch);
    addEpochToFileName(ANT_SNAPSHOT_ANCHORS_FILENAME, sizeof(ANT_SNAPSHOT_ANCHORS_FILENAME) / sizeof(ANT_SNAPSHOT_ANCHORS_FILENAME[0]), epoch);
    addEpochToFileName(ANT_SNAPSHOT_RECORDS_FILENAME, sizeof(ANT_SNAPSHOT_RECORDS_FILENAME) / sizeof(ANT_SNAPSHOT_RECORDS_FILENAME[0]), epoch);
    addEpochToFileName(ANT_SNAPSHOT_POOL_FILENAME, sizeof(ANT_SNAPSHOT_POOL_FILENAME) / sizeof(ANT_SNAPSHOT_POOL_FILENAME[0]), epoch);
    addEpochToFileName(ANT_SNAPSHOT_EXPORT_FILENAME, sizeof(ANT_SNAPSHOT_EXPORT_FILENAME) / sizeof(ANT_SNAPSHOT_EXPORT_FILENAME[0]), epoch);
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::saveSnapshot(unsigned short epoch, CHAR16* directory,
    unsigned int initialTick) const
{
    ASSERT(_records != nullptr);
    ASSERT(_annPool != nullptr);
    ASSERT(_anchors != nullptr);

    antSnapshotNameForEpoch(epoch);

    AntColonySnapshotMeta meta;
    setMem(&meta, sizeof(meta), 0);
    meta.magic = AntColonySnapshotMeta::MAGIC;
    meta.version = AntColonySnapshotMeta::VERSION;
    meta.epoch = epoch;
    meta.solutionCount = _solutionCount;
    meta.recordSizeBytes = (unsigned int)sizeof(AntSolutionRecord);
    meta.annPoolEntryBytes = (unsigned int)sizeof(PackedAnn);
    meta.errorThreshold = _errorThreshold;
    meta.initialTick = initialTick;
    meta.rootSeed = _rootSeed;

    if (save(ANT_SNAPSHOT_META_FILENAME, sizeof(meta), (unsigned char*)&meta, directory)
        != (long long)sizeof(meta))
    {
        logToConsole(L"[ant-colony] failed to save snapshot meta");
        return false;
    }
    if (save(ANT_SNAPSHOT_ANCHORS_FILENAME, sizeof(AnchorRing), (unsigned char*)_anchors, directory)
        != (long long)sizeof(AnchorRing))
    {
        logToConsole(L"[ant-colony] failed to save snapshot anchors");
        return false;
    }

    // Written even when the colony is empty, so the operator's snapshot is always the same number of files
    const unsigned long long slots = antSnapshotSlotCount(_solutionCount);
    const unsigned long long recordBytes = slots * sizeof(AntSolutionRecord);
    const unsigned long long poolBytes = slots * sizeof(PackedAnn);
    if (saveLargeFile(ANT_SNAPSHOT_RECORDS_FILENAME, recordBytes, (unsigned char*)_records, directory, false)
        != (long long)recordBytes)
    {
        logToConsole(L"[ant-colony] failed to save snapshot records");
        return false;
    }
    if (saveLargeFile(ANT_SNAPSHOT_POOL_FILENAME, poolBytes, (unsigned char*)_annPool, directory, false)
        != (long long)poolBytes)
    {
        logToConsole(L"[ant-colony] failed to save snapshot pool");
        return false;
    }
    // Whole struct, fixed size - there is no used prefix to take, the order array indexes all of it.
    if (save(ANT_SNAPSHOT_EXPORT_FILENAME, sizeof(ExportSet), (unsigned char*)_exportSet, directory)
        != (long long)sizeof(ExportSet))
    {
        logToConsole(L"[ant-colony] failed to save snapshot export set");
        return false;
    }
    return true;
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::loadSnapshot(unsigned short epoch, CHAR16* directory,
    const m256i& rootSeed, unsigned int errorThreshold, unsigned int initialTick)
{
    ASSERT(_records != nullptr);
    ASSERT(_annPool != nullptr);
    ASSERT(_anchors != nullptr);

    antSnapshotNameForEpoch(epoch);

    AntColonySnapshotMeta meta;
    if (load(ANT_SNAPSHOT_META_FILENAME, sizeof(meta), (unsigned char*)&meta, directory)
        != (long long)sizeof(meta))
    {
        logToConsole(L"[ant-colony] failed to load snapshot meta");
        return false;
    }
    if (meta.magic != AntColonySnapshotMeta::MAGIC || meta.version != AntColonySnapshotMeta::VERSION)
    {
        antSnapshotFailure(L"bad magic/version", meta.magic, meta.version);
        return false;
    }
    if (meta.epoch != epoch)
    {
        antSnapshotFailure(L"epoch mismatch, file/expected", meta.epoch, epoch);
        return false;
    }
    // A differently sized record or ANN would parse cleanly and produce a tree that is silently wrong.
    if (meta.recordSizeBytes != sizeof(AntSolutionRecord) || meta.annPoolEntryBytes != sizeof(PackedAnn))
    {
        antSnapshotFailure(L"layout mismatch, record/ann", meta.recordSizeBytes, meta.annPoolEntryBytes);
        return false;
    }
    if (meta.solutionCount > ANT_MAX_NODES_PER_EPOCH)
    {
        antSnapshotFailure(L"solutionCount exceeds the cap, count/cap", meta.solutionCount, ANT_MAX_NODES_PER_EPOCH);
        return false;
    }
    // Cross-check against the node state restored alongside this file. A mismatch means the two
    // snapshots are not from the same moment - loading the tree anyway would derive every root from
    // a seed the rest of the network is not using.
    if (!(meta.rootSeed == rootSeed))
    {
        antSnapshotFailure(L"root seed does not match the restored node state, record/0", 0, 0);
        return false;
    }
    if (meta.errorThreshold != errorThreshold)
    {
        antSnapshotFailure(L"threshold does not match the node, file/node", meta.errorThreshold, errorThreshold);
        return false;
    }
    // Every selfRef.tickOffset is relative to this. Restoring against a different base would not
    // fail anywhere - it would resolve parent references to the wrong records
    if (meta.initialTick != initialTick)
    {
        antSnapshotFailure(L"initial tick does not match the node, file/node", meta.initialTick, initialTick);
        return false;
    }

    // Everything above only read the meta, so a refusal there leaves the colony untouched. From here
    // on the state is being overwritten
    reset();

    if (load(ANT_SNAPSHOT_ANCHORS_FILENAME, sizeof(AnchorRing), (unsigned char*)_anchors, directory)
        != (long long)sizeof(AnchorRing))
    {
        logToConsole(L"[ant-colony] failed to load snapshot anchors");
        reset();
        return false;
    }

    // Read unconditionally and at the same sizing the save used, so an incomplete copy is refused
    // here rather than booting a node with an empty tree.
    const unsigned long long slots = antSnapshotSlotCount(meta.solutionCount);
    const unsigned long long recordBytes = slots * sizeof(AntSolutionRecord);
    const unsigned long long poolBytes = slots * sizeof(PackedAnn);
    if (loadLargeFile(ANT_SNAPSHOT_RECORDS_FILENAME, recordBytes, (unsigned char*)_records, directory)
        != (long long)recordBytes)
    {
        logToConsole(L"[ant-colony] failed to load snapshot records");
        reset();
        return false;
    }
    if (loadLargeFile(ANT_SNAPSHOT_POOL_FILENAME, poolBytes, (unsigned char*)_annPool, directory)
        != (long long)poolBytes)
    {
        logToConsole(L"[ant-colony] failed to load snapshot pool");
        reset();
        return false;
    }

    if (load(ANT_SNAPSHOT_EXPORT_FILENAME, sizeof(ExportSet), (unsigned char*)_exportSet, directory)
        != (long long)sizeof(ExportSet))
    {
        logToConsole(L"[ant-colony] failed to load snapshot export set");
        reset();
        return false;
    }
    if (_exportSet->count > ANT_EXPORT_MAX_SOLUTIONS)
    {
        antSnapshotFailure(L"export count exceeds the cap, count/cap", _exportSet->count, ANT_EXPORT_MAX_SOLUTIONS);
        reset();
        return false;
    }
    for (unsigned int i = 0; i < _exportSet->count; i++)
    {
        // order[] indexes slots[]; a bad entry would make the export read a slot that was never written.
        if (_exportSet->order[i] >= ANT_EXPORT_MAX_SOLUTIONS)
        {
            antSnapshotFailure(L"export order out of range, position/slot", i, _exportSet->order[i]);
            reset();
            return false;
        }
    }

    // The caller's values, which the checks above proved the file agrees with.
    _rootSeed = rootSeed;
    _errorThreshold = errorThreshold;
    _solutionCount = meta.solutionCount;

    // Rebuild the intermediate data
    if (!rebuildDerivedState(initialTick))
    {
        reset();
        return false;
    }
    return true;
}

template<typename ScoreT>
inline bool AntColony<ScoreT>::rebuildDerivedState(unsigned int initialTick)
{
    // Hoisted out of the loop: unpacking each stored network to verify its hash needs somewhere to
    // put it, and this path runs once at boot.
    Ann annBuffer;

    for (unsigned int i = 0; i < _solutionCount; i++)
    {
        const AntSolutionRecord& rec = _records[i];

        if (rec.selfRef.tickOffset >= MAX_NUMBER_OF_TICKS_PER_EPOCH)
        {
            antSnapshotFailure(L"tickOffset out of range, record/tickOffset", i, rec.selfRef.tickOffset);
            return false;
        }
        // commit() writes the record and its network at the same index, and findIndexBySolutionRef
        // and annOfNonRoot both rely on it
        // annStateSlot point to the ANN that must have similar index to the record
        if (rec.annStateSlot != i)
        {
            antSnapshotFailure(L"annStateSlot is not the record index, record/slot", i, rec.annStateSlot);
            return false;
        }

        // The freshness rule validateChild() applied at admission, re-checked here. This is the only
        // guard on anchorTick, which is consensus-relevant: siblingFloor() compares a stored record's
        // anchorTick against a later child's, so a corrupt one changes which siblings compete and
        // therefore which solutions the node accepts. publishTick was not stored because it does not
        // need to be - commit() sets tickOffset to (publishTick - initialTick), so it inverts exactly
        const unsigned int publishTick = initialTick + rec.selfRef.tickOffset;
        if (rec.anchorTick > publishTick
            || publishTick - rec.anchorTick > ANT_PUBLISH_WINDOW_TICKS)
        {
            antSnapshotFailure(L"anchor tick outside the freshness window, record/anchorTick", i, rec.anchorTick);
            return false;
        }

        // Rebuild the tick index
        AntTickSlot& tslot = _tickIndex[rec.selfRef.tickOffset];
        if (tslot.count == 0)
        {
            tslot.startIdx = i;
        }
        else if (tslot.startIdx + tslot.count != i)
        {
            antSnapshotFailure(L"record breaks tick contiguity, record/tickOffset", i, rec.selfRef.tickOffset);
            return false;
        }
        tslot.count++;

        // The parent must be ROOT or an EARLIER record. The tick index built so far covers only
        // records before this one, so a forward or self reference fails to resolve - which is what
        // rejects a cycle.
        const AntSolutionRecord* parentRec = nullptr;
        if (!rec.parentRef.isRoot())
        {
            const long long parentIdx = findIndexBySolutionRef(rec.parentRef);
            if (parentIdx == ANT_INVALID_INDEX || (unsigned long long)parentIdx >= i)
            {
                antSnapshotFailure(L"parent not an earlier record, record/parentTickOffset", i, rec.parentRef.tickOffset);
                return false;
            }
            parentRec = &_records[parentIdx];
            if (!(parentRec->pubkey == rec.pubkey))
            {
                antSnapshotFailure(L"parent belongs to another identity, record", i, 0);
                return false;
            }
        }
        const unsigned int expectedDepth = (parentRec != nullptr) ? (parentRec->depth + 1) : 1;
        if (rec.depth != expectedDepth)
        {
            antSnapshotFailure(L"depth does not match the parent, record/depth", i, rec.depth);
            return false;
        }

        // The two score rules validateChild() enforced when this record was admitted. A corrupt
        // score would otherwise set a wrong sibling floor and a wrong bar for its own children.
        if (rec.score > _errorThreshold)
        {
            antSnapshotFailure(L"score above the epoch threshold, record/score", i, rec.score);
            return false;
        }
        if (parentRec != nullptr && rec.score >= parentRec->score)
        {
            antSnapshotFailure(L"score does not beat the parent, record/score", i, rec.score);
            return false;
        }

        // Re-derive the hash from the stored network
        _annPool[i].unpack(annBuffer.lut);
        unsigned int annHash;
        KangarooTwelve(&annBuffer, sizeof(annBuffer), &annHash, sizeof(annHash));
        if (annHash != rec.childAnnHash)
        {
            antSnapshotFailure(L"stored network does not match childAnnHash, record", i, 0);
            return false;
        }

        const AntDedupKey key{ rec.pubkey, rec.nonce, rec.parentRef };
        if (_dedup->contains(key))
        {
            antSnapshotFailure(L"duplicate solution, record", i, 0);
            return false;
        }
        if (_dedup->add(key) == QPI::NULL_INDEX)
        {
            antSnapshotFailure(L"dedup set full, record", i, 0);
            return false;
        }

        // Rebuild the _childHeadByMiner and _childHeadByParent
        unsigned int prevHead = NO_SIBLING;
        if (rec.parentRef.isRoot())
        {
            _childHeadByMiner->get(rec.pubkey, prevHead);
            _records[i].nextSiblingIdx = prevHead;
            if (_childHeadByMiner->set(rec.pubkey, i) == QPI::NULL_INDEX)
            {
                antSnapshotFailure(L"miner index full, record", i, 0);
                return false;
            }
        }
        else
        {
            _childHeadByParent->get(rec.parentRef, prevHead);
            _records[i].nextSiblingIdx = prevHead;
            _childHeadByParent->set(rec.parentRef, i);
        }

        // Restore the stats also
        _stats.acceptedSolutions++;
        if (rec.depth > _stats.treeDepthMax)
        {
            _stats.treeDepthMax = rec.depth;
        }
    }

    _stats.treeSizeCurrent = _solutionCount;
    return true;
}
