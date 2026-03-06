#pragma once

#include "platform/global_var.h"
#include "platform/memory_util.h"
#include "platform/assert.h"
#include "platform/concurrency.h"

#include "network_messages/entity.h"
#include "network_messages/assets.h"
#include "contract_core/pre_qpi_def.h"
#include "contracts/math_lib.h"


constexpr unsigned long long spectrumSizeInBytes = SPECTRUM_CAPACITY * sizeof(EntityRecord);
constexpr unsigned long long universeSizeInBytes = ASSETS_CAPACITY * sizeof(AssetRecord);
constexpr unsigned long long defaultCommonBuffersSize = math_lib::max(MAX_CONTRACT_STATE_SIZE, math_lib::max(spectrumSizeInBytes, universeSizeInBytes));

// Buffer(s) used for:
// - reorganizing spectrum and universe hash maps (tick processor)
// - scratchpad buffer used internally in QPI::Collection, QPI::HashMap, QPI::HashSet,
//   QPI::ProposalAndVotingByShareholders
//   (often used in contract processor which does not run concurrently with tick processor, but now also used outside
//   of contracts, e.g. pendingTxsPool.add() running in request processor may trigger Collection::_rebuild() which
//   uses scratchpad)
// - building oracle transactions in processTick() in tick processor
// - calculateStableComputorIndex() in tick processor
// - saving and loading of logging state
// - DustBurnLogger used in increaseEnergy() in tick / contract processor
// Must be large enough to fit any contract, full spectrum, and full universe!
class CommonBuffers
{
public:
    // Allocate common buffers. With count > 1, multiple buffers may be used concurrently. The maximum buffer size
    // that can be acquired is given by size.
    bool init(unsigned int count, unsigned long long size = defaultCommonBuffersSize)
    {
        if (!count || !size)
            return false;

        // soft limit, just to detect mistakes in usage like init(sizeof(Object))
        ASSERT(count < 16);

        // memory layout of buffer: sub buffer pointers | sub buffer locks | sub buffer 1 | sub buffer 2 | ...
        unsigned char* buffer = nullptr;
        const unsigned long long ptrSize = count * sizeof(unsigned char*);
        const unsigned long long lockSize = (count + 7) / 8;
        const unsigned long long bufSize = count * size;

        if (!allocPoolWithErrorLog(L"commonBuffers", ptrSize + lockSize + bufSize, (void**)&buffer, __LINE__))
        {
            return false;
        }

        bufferCount = count;
        subBufferSize = size;
        subBufferPtr = (unsigned char**)buffer;
        subBufferLock = (volatile char*)(buffer + ptrSize);
        unsigned char* subBuf = buffer + ptrSize + lockSize;
        for (unsigned int i = 0; i < count; ++i)
        {
            subBufferPtr[i] = subBuf;
            subBuf += size;
        }

        return true;
    }

    // Free common buffers.
    void deinit()
    {
        if (subBufferPtr)
        {
            freePool(subBufferPtr);
            subBufferPtr = nullptr;
            subBufferLock = nullptr;
            subBufferSize = 0;
            bufferCount = 0;
            waitingCount = 0;
            maxWaitingCount = 0;
            invalidReleaseCount = 0;
        }
    }

    // Get buffer of given size.
    // Returns nullptr if size is too big. Otherwise may block until buffer is available.
    // Does not init buffer! Buffer needs to be released with releaseBuffer() after use.
    void* acquireBuffer(unsigned long long size)
    {
        ASSERT(subBufferLock && subBufferPtr);
#if !defined(NO_UEFI)
        ASSERT(size <= subBufferSize);
#endif
        if (size > subBufferSize)
            return nullptr;

        // shortcut for default case
        if (TRY_ACQUIRE(subBufferLock[0]))
        {
            return subBufferPtr[0];
        }

        long cnt = _InterlockedIncrement(&waitingCount);
        if (maxWaitingCount < cnt)
            maxWaitingCount = cnt;

        unsigned int i = 0;
        BEGIN_WAIT_WHILE(TRY_ACQUIRE(subBufferLock[i]) == false)
        {
            ++i;
            if (i >= bufferCount)
                i = 0;
        }
        END_WAIT_WHILE();

        _InterlockedDecrement(&waitingCount);

        return subBufferPtr[i];
    }

    // Release buffer that was acquired with acquireBuffer() before.
    void releaseBuffer(void* buffer)
    {
        ASSERT(subBufferLock && subBufferPtr && buffer);

        // shortcut for default case
        if (subBufferPtr[0] == buffer)
        {
            if (subBufferLock[0])
                RELEASE(subBufferLock[0]);
            else
                ++invalidReleaseCount;
            return;
        }

        // find buffer
        unsigned int bufferIdx = 1;
        while (bufferIdx < bufferCount && subBufferPtr[bufferIdx] != buffer)
            ++bufferIdx;

        // invalid pointer passed?
#if !defined(NO_UEFI)
        ASSERT(bufferIdx < bufferCount);
        ASSERT(subBufferLock[bufferIdx]);
#endif
        if (bufferIdx >= bufferCount || !subBufferLock[bufferIdx])
        {
            ++invalidReleaseCount;
            return;
        }

        // release buffer
        RELEASE(subBufferLock[bufferIdx]);
    }

    // Heuristics how many processors were waiting for a buffer in parallel (for deciding the count of buffers)
    long getMaxWaitingProcessorCount() const
    {
        return maxWaitingCount;
    }

    // Counter of invalid release calls as an indicator if debugging is needed
    long getInvalidReleaseCount() const
    {
        return invalidReleaseCount;
    }

    // Returns number of buffers currently acquired
    unsigned int acquiredBuffers() const
    {
        unsigned int count = 0;
        for (unsigned int i = 0; i < bufferCount; ++i)
            if (subBufferLock[i])
                ++count;
        return count;
    }

protected:
    unsigned char** subBufferPtr = nullptr;
    volatile char* subBufferLock = nullptr;
    unsigned long long subBufferSize = 0;
    unsigned int bufferCount = 0;
    volatile long waitingCount = 0;
    long maxWaitingCount = 0;
    long invalidReleaseCount = 0;
};


GLOBAL_VAR_DECL CommonBuffers commonBuffers;


static void* __acquireScratchpad(unsigned long long size, bool initZero = true)
{
    void* ptr = commonBuffers.acquireBuffer(size);
    if (ptr && initZero)
        setMem(ptr, size, 0);
    return ptr;
}

static void __releaseScratchpad(void* ptr)
{
    commonBuffers.releaseBuffer(ptr);
}
