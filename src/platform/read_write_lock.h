#pragma once

#include <intrin.h>
#include "debugging.h"

// Lock that allows multiple readers but only one writer.
// Priorizes writers, that is, additional readers can only gain access if no writer is waiting.
class ReadWriteLock
{
public:
    // Constructor (disabled because not called without MS CRT, you need to call reset() to init)
    //ReadWriteLock()
    //{
    //    reset();
    //}

    // Set lock fully unlocked state.
    void reset()
    {
        readers = 0;
        writersWaiting = 0;
    }

    // Aquire lock for reading (wait if needed).
    void acquireRead()
    {
        ASSERT(readers >= -1);
        ASSERT(writersWaiting >= 0);

        // Wait until getting read access
        while (!tryAcquireRead())
            _mm_pause();
    }

    // Try to aquire lock for reading without waiting. Return true if lock has been acquired.
    bool tryAcquireRead()
    {
        ASSERT(readers >= -1);
        ASSERT(writersWaiting >= 0);

        // Prioritize writers over readers (do not grant access for new reader if writer is waiting)
        if (writersWaiting)
            return false;

        // It may happen that writersWaiting changes before the read lock is acquired below. But this should
        // be rare and does not justify the overhead of additional locking.

        // If not acquired by writer (would be -1 readers), acquire read access by incrementing number of readers.
        // Below is the atomic version of the following:
        // 
        //      if (readers >= 0) {
        //          ++readers;
        //          return true;
        //      } else {
        //          return false;
        //      }
        long currentReaders, newReaders;
        while (1)
        {
            // Get number of current readers and return false if locked by writer
            currentReaders = readers;
            if (currentReaders < 0)
                return false;

            // Increment readers counter.
            // If readers is changed after setting currentReaders, the _InterlockedCompareExchange will fail
            // and we retry in the next iteration of the while loop.
            newReaders = currentReaders + 1;
            if (_InterlockedCompareExchange(&readers, newReaders, currentReaders) == currentReaders)
                return true;
        }        
    }

    // Release read lock. Needs to follow corresponding acquireRead() or successful tryAcquireRead().
    void releaseRead()
    {
        ASSERT(readers > 0);
        ASSERT(writersWaiting >= 0);

        _InterlockedDecrement(&readers);
    }

    // Aquire lock for writing (wait if needed).
    void acquireWrite()
    {
        ASSERT(readers >= -1);
        ASSERT(writersWaiting >= 0);
        
        // Indicate that one more writer is waiting for access
        // -> don't grant access to additional readers in tryAcquireRead() in concurrent threads
        _InterlockedIncrement(&writersWaiting);

        // Wait until getting write access
        while (!tryAcquireWrite())
            _mm_pause();

        // Writer got access -> decrement counter of writers waiting for access
        _InterlockedDecrement(&writersWaiting);
    }

    // Try to aquire lock for writing without waiting. Return true if lock has been acquired.
    bool tryAcquireWrite()
    {
        ASSERT(readers >= -1);
        ASSERT(writersWaiting >= 0);

        // If no readers or writers, lock for writing.
        // Below is the atomic version of the following:
        // 
        //      if (readers == 0)
        //      {
        //          readers = -1;
        //          return true;
        //      }
        if (_InterlockedCompareExchange(&readers, -1, 0) == 0)
            return true;

        return false;
    }

    // Release write lock. Needs to follow corresponding acquireWrite() or successful tryAcquireWrite().
    void releaseWrite()
    {
        ASSERT(readers == -1);
        ASSERT(writersWaiting >= 0);
        readers = 0;
    }

private:
    // Positive values means number of readers using having acquired a read lock; -1 means writer has acquired lock; 0 mean no lock is active
    volatile long readers;

    // Number of writers waiting for lock (>= 0)
    volatile long writersWaiting;
};
