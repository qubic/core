#pragma once

#include "../platform/debugging.h"

// Last-In-First-Out storage for data of different size.
// Size type used for StackBuffer needs to be unsigned.
// Supports unwinding for analyzing stack in error handling and tagging blocks as "special" (for example those
// with infos about locks that need to be released).
// #define TRACK_MAX_STACK_BUFFER_SIZE to collect info on how much stack is used.
template <typename StackBufferSizeType, StackBufferSizeType bufferSize>
struct StackBuffer
{
    // Data type used for size and index.
    typedef StackBufferSizeType SizeType;
    static_assert(SizeType(-1) > 0, "Signed StackBufferSizeType is not supported!");

    // Constructor (disabled because not called without MS CRT, you need to call init() to init)
    //StackBuffer()
    //{
    //    init();
    //}

    // Initialize as empty stack (memory not zeroed)
    void init()
    {
        _allocatedSize = 0;
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
        _maxAllocatedSize = 0;
        _failedAllocAttempts = 0;
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

    unsigned int failedAllocAttempts() const
    {
        return _failedAllocAttempts;
    }
#endif

    // Allocate storage in buffer.
    char* allocate(SizeType size, bool specialBlock = false)
    {
        ASSERT(_allocatedSize <= bufferSize);

        // allocate fails of size after allocating overflows buffer size or the used size type
        StackBufferSizeType newSize = _allocatedSize + size + sizeof(SizeType);
        if (newSize > bufferSize || newSize <= _allocatedSize)
        {
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
            ++_failedAllocAttempts;
#endif
#if !defined(NDEBUG) && !defined(NO_UEFI)
            CHAR16 dbgMsg[200];
            setText(dbgMsg, L"StackBuffer.allocate() failed! old size ");
            appendNumber(dbgMsg, _allocatedSize, TRUE);
            appendText(dbgMsg, L", failed new size ");
            appendNumber(dbgMsg, newSize, TRUE);
            appendText(dbgMsg, L", capacity ");
            appendNumber(dbgMsg, bufferSize, TRUE);
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
            appendText(dbgMsg, L", max alloc ");
            appendNumber(dbgMsg, _maxAllocatedSize, TRUE);
            appendText(dbgMsg, L", failed alloc ");
            appendNumber(dbgMsg, _failedAllocAttempts, TRUE);
#endif
            addDebugMessage(dbgMsg);
#endif
            return nullptr;
        }

        // get pointer to return
        char* allocatedBuffer = _buffer + _allocatedSize;

        // store size from before allocating buffer
        SizeType* sizeBeforeAlloc = reinterpret_cast<SizeType*>(allocatedBuffer + size);
        *sizeBeforeAlloc = _allocatedSize;
        if (specialBlock)
            *sizeBeforeAlloc |= specialBlockFlag;
         
        // update size
        _allocatedSize = newSize;
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
        ASSERT(_maxAllocatedSize <= bufferSize);
        if (_allocatedSize > _maxAllocatedSize)
            _maxAllocatedSize = _allocatedSize;
#endif

        return allocatedBuffer;
    }

    // Allocate "special block" storage in buffer, which is relevant for unwinding.
    inline char* allocateSpecial(SizeType size)
    {
        return allocate(size, true);
    }

    // Free storage allocated by last call to allocate().
    bool free()
    {
        // get size before last alloc
        bool okay = (_allocatedSize <= bufferSize) && (_allocatedSize >= sizeof(SizeType));
#if !defined(NO_UEFI)
        ASSERT(_allocatedSize <= bufferSize);
        ASSERT(_allocatedSize >= sizeof(SizeType));
#endif
        SizeType sizeBeforeLastAlloc = *reinterpret_cast<SizeType*>(_buffer + _allocatedSize - sizeof(SizeType)) & sizeMask;
#if !defined(NO_UEFI)
        ASSERT(sizeBeforeLastAlloc < _allocatedSize);
#endif
        okay = okay && (sizeBeforeLastAlloc < _allocatedSize);

        // some additional checks and ouput for debugging
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
#if !defined(NO_UEFI)
        ASSERT(_maxAllocatedSize <= bufferSize);
        ASSERT(sizeBeforeLastAlloc < _maxAllocatedSize);
#endif
        okay = okay && (_maxAllocatedSize <= bufferSize) && (sizeBeforeLastAlloc < _maxAllocatedSize);
#endif
#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsg[200];
        if (!okay)
        {
            setText(dbgMsg, L"StackBuffer.free() failed! Sizes: before free() ");
            appendNumber(dbgMsg, _allocatedSize, TRUE);
            appendText(dbgMsg, L", after free() ");
            appendNumber(dbgMsg, sizeBeforeLastAlloc, TRUE);
            appendText(dbgMsg, L", capacity ");
            appendNumber(dbgMsg, bufferSize, TRUE);
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
            appendText(dbgMsg, L", max alloc ");
            appendNumber(dbgMsg, _maxAllocatedSize, TRUE);
            appendText(dbgMsg, L", failed alloc ");
            appendNumber(dbgMsg, _failedAllocAttempts, TRUE);
#endif
            addDebugMessage(dbgMsg);
        }
#endif
        if (okay)
        {
            // reduce current size down to size before last alloc
            // (normal behavior if count of alloc matches count of free and memory is not corrupted)
            _allocatedSize = sizeBeforeLastAlloc;
            return true;
        }
        else
        {
            // problem -> reset to prevent wild undefined behavior in following use
            _allocatedSize = 0;
            return false;
        }
    }

    // Free all storage allocated before.
    void freeAll()
    {
        _allocatedSize = 0;
    }

    // Free last allocated block and return pointer, size, and whether it is a special block.
    bool unwind(char*& buffer, SizeType& size, bool& specialBlock)
    {
        if (_allocatedSize < sizeof(SizeType))
            return false;
        SizeType sizePtrOffset = _allocatedSize - sizeof(SizeType);
        SizeType prevSizeAndFlag = *reinterpret_cast<SizeType*>(_buffer + sizePtrOffset);
        size = sizePtrOffset - (prevSizeAndFlag & sizeMask);
        bool okay = free();
        specialBlock = (prevSizeAndFlag & specialBlockFlag) != 0;
        buffer = _buffer + _allocatedSize;
        return okay;
    }

protected:
    // structure of buffer content: [ allocated buffer 1 | size before allocating buffer 1 | allocated buffer 2 | size before buffer 2 | ... | alloc. buf. n | size bef. buf. n ]
    // "size before allocating buffer" may have specialBlockFlag set.
    char _buffer[bufferSize];

    // number of bytes used in buffer
    SizeType _allocatedSize;

    // Flag used internally to indicate a special block (bit set in size on _buffer)
    static constexpr SizeType specialBlockFlag = (1 << (sizeof(StackBufferSizeType) * 8 - 1));

    // Mask to extract size from _buffer
    static constexpr SizeType sizeMask = ~specialBlockFlag;

    static_assert(bufferSize <= sizeMask, "Buffer size too large for the used size type!");

#ifdef TRACK_MAX_STACK_BUFFER_SIZE
    SizeType _maxAllocatedSize;
    unsigned int _failedAllocAttempts;
#endif
};
