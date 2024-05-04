#pragma once

#include "../platform/debugging.h"

// Last-In-First-Out storage for data of different size.
// Size type used for StackBuffer needs to be unsigned.
// #define TRACK_MAX_STACK_BUFFER_SIZE to collect info on how much stack is used.
template <typename StackBufferSizeType, StackBufferSizeType bufferSize>
struct StackBuffer
{
    // Data type used for size and index.
    typedef StackBufferSizeType SizeType;
    static_assert(SizeType(-1) > 0, "Signed StackBufferSizeType is not supported!");

    // Initialize as empty stack (momory not zeroed)
    void init()
    {
        _allocatedSize = 0;
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
        _maxAllocatedSize = 0;
#endif
    }

    // Return capacity of buffer in bytes
    static constexpr SizeType capacity()
    {
        return bufferSize;
    }

    // Number of bytes currently used.
    SizeType size() const
    {
        return _allocatedSize;
    }

#ifdef TRACK_MAX_STACK_BUFFER_SIZE
    SizeType maxSizeObserved() const
    {
        return _maxAllocatedSize;
    }
#endif

    // Allocate storage in buffer.
    void* allocate(SizeType size)
    {
        // allocate fails of size after allocating overflows buffer size or the used size type
        StackBufferSizeType newSize = _allocatedSize + size + sizeof(SizeType);
        if (newSize > bufferSize || newSize <= _allocatedSize)
            return nullptr;

        // get pointer to return
        char* allocatedBuffer = _buffer + _allocatedSize;

        // store size from before allocating buffer
        SizeType* sizeBeforeAlloc = reinterpret_cast<SizeType*>(allocatedBuffer + size);
        *sizeBeforeAlloc = _allocatedSize;
         
        // update size
        _allocatedSize = newSize;
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
        if (_allocatedSize > _maxAllocatedSize)
            _maxAllocatedSize = _allocatedSize;
#endif

        return allocatedBuffer;
    }

    // Free storage allocated by last call to allocate().
    void free()
    {
        // get size before last alloc
        ASSERT(_allocatedSize >= sizeof(SizeType));
        SizeType sizeBeforeLastAlloc = *reinterpret_cast<SizeType*>(_buffer + _allocatedSize - sizeof(SizeType));

        // reduce current size down to size before last alloc
        ASSERT(sizeBeforeLastAlloc < _allocatedSize);
        _allocatedSize = sizeBeforeLastAlloc;
    }

protected:
    // structure of buffer content: [ allocated buffer 1 | size before allocating buffer 1 | allocated buffer 2 | size before buffer 2 | ... | alloc. buf. n | size bef. buf. n ]
    char _buffer[bufferSize];

    // number of bytes used in buffer
    SizeType _allocatedSize;

#ifdef TRACK_MAX_STACK_BUFFER_SIZE
    SizeType _maxAllocatedSize;
#endif
};
