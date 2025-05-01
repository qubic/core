#pragma once

#include "global_var.h"
#include "assert.h"
#include "memory_util.h"
#include "time_stamp_counter.h"
#include "file_io.h"

struct ProfilingData
{
    const char* scopeName;
    unsigned long long scopeLine;
    unsigned long long numOfExec;
    unsigned long long runtimeSum;
    unsigned long long runtimeMax;
    unsigned long long runtimeMin;
};

class ProfilingDataCollector
{
public:
    // Init buffer (optional). This reduces the number of allocations.
    bool init(unsigned int expectedProfilingDataItems = 8)
    {
        ACQUIRE_WITHOUT_DEBUG_LOGGING(mLock);
        bool okay = doInit(expectedProfilingDataItems);
        RELEASE(mLock);
        return okay;
    }

    // Clear buffer, discarding all measurements
    void clear()
    {
        ACQUIRE_WITHOUT_DEBUG_LOGGING(mLock);
        if (mDataPtr)
        {
            setMem(mDataPtr, mDataSize * sizeof(ProfilingData), 0);
            mDataUsedEntryCount = 0;
        }
        RELEASE(mLock);
    }

    // Free buffer
    void deinit()
    {
        ACQUIRE_WITHOUT_DEBUG_LOGGING(mLock);
        if (mDataPtr)
            freePool(mDataPtr);
        mDataPtr = nullptr;
        mDataSize = 0;
        mDataUsedEntryCount = 0;
        RELEASE(mLock);
    }

    // Add run-time measurement to profiling entry of given key (= scopeName + scopeLine)
    void addMeasurement(const char* scopeName, unsigned long long scopeLine, unsigned long long startTsc, unsigned long long endTsc)
    {
        // Discard measurement on overflow of time stamp counter register
        if (endTsc < startTsc)
            return;

#ifdef NO_UEFI
        // Init time stamp counter frequency if needed
        if (!frequency)
            initTimeStampCounter();
#endif

        ACQUIRE_WITHOUT_DEBUG_LOGGING(mLock);

        // Make sure hash map is initialized
        if (mDataPtr || doInit())
        {
            // Make sure hash map has enough free space
            if (mDataUsedEntryCount * 2 <= mDataSize || doInit(mDataUsedEntryCount * 2))
            {
                // Fill entry
                unsigned long long dtMicroseconds = (endTsc - startTsc) * 1000000 / frequency;
                ProfilingData& newEntry = getEntry(scopeName, scopeLine);
                newEntry.numOfExec += 1;
                newEntry.runtimeSum += dtMicroseconds;
                if (newEntry.runtimeMin > dtMicroseconds)
                    newEntry.runtimeMin = dtMicroseconds;
                if (newEntry.runtimeMax < dtMicroseconds)
                    newEntry.runtimeMax = dtMicroseconds;
            }
        }

        RELEASE(mLock);
    }
    
    // Write CSV file with ProfilingData and hash function values (to check distribution)
    bool writeToFile()
    {
        ASSERT(isMainProcessor());

        // TODO: define file object in platform lib, including writeStringToFile()
#ifdef NO_UEFI
        FILE* file = fopen("profiling.csv", "wb");
        if (!file)
            return false;
#else
        ASSERT(root);
        EFI_STATUS status;
        EFI_FILE_PROTOCOL* file;
        if (status = root->Open(root, (void**)&file, (CHAR16*)L"profiling.csv", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
        {
            logStatusToConsole(L"EFI_FILE_PROTOCOL::Open() failed in ProfilingDataCollector::writeToFile()", status, __LINE__);
            return false;
        }
#endif

        ACQUIRE_WITHOUT_DEBUG_LOGGING(mLock);

        // output hash for each entry?
        bool okay = writeStringToFile(file, L"idx,scopeName,scopeLine,count,sum_microseconds,avg_microseconds,min_microseconds,max_microseconds\r\n");
        for (unsigned int i = 0; i < mDataSize; ++i)
        {
            if (mDataPtr[i].scopeName)
            {
                setNumber(message, i, false);
                appendText(message, ",");
                appendText(message, mDataPtr[i].scopeName);
                appendText(message, ",");
                appendNumber(message, mDataPtr[i].scopeLine, false);
                appendText(message, ",");
                appendNumber(message, mDataPtr[i].numOfExec, false);
                appendText(message, ",");
                appendNumber(message, mDataPtr[i].runtimeSum, false);
                appendText(message, ",");
                appendNumber(message, (mDataPtr[i].numOfExec > 0) ? mDataPtr[i].runtimeSum / mDataPtr[i].numOfExec : 0, false);
                appendText(message, ",");
                appendNumber(message, mDataPtr[i].runtimeMin, false);
                appendText(message, ",");
                appendNumber(message, mDataPtr[i].runtimeMax, false);
                appendText(message, "\r\n");
                okay &= writeStringToFile(file, message);
            }
        }

        RELEASE(mLock);

#ifdef NO_UEFI
        fclose(file);
#else
        file->Close(file);
#endif

        return okay;
    }

protected:
    // Hash map of mDataSize = 2^N elements of ProfilingData
    ProfilingData* mDataPtr = nullptr;
    unsigned int mDataSize = 0;
    unsigned int mDataUsedEntryCount = 0;
    
    // Lock preventing concurrent access (acquired and released in public functions)
    volatile char mLock = 0;

    // Init buffer. Assumes caller has acquired mLock.
    bool doInit(unsigned int expectedProfilingDataItems = 8)
    {
        // Compute size of new hash map
        unsigned int newDataSize = 8;
        while (newDataSize < expectedProfilingDataItems)
            newDataSize <<= 1;

        // Make sure there is enough space in the new hash map
        if (newDataSize < mDataUsedEntryCount)
            return false;
        newDataSize <<= 1;

        // Keep old data for copying to new hash map
        ProfilingData* oldDataPtr = mDataPtr;
        const unsigned int oldDataSize = mDataSize;
        const unsigned int oldDataUsedEntryCount = mDataUsedEntryCount;

        // Allocate new hash map (init to zero)
        if (!allocPoolWithErrorLog(L"ProfilingDataCollector", newDataSize * sizeof(ProfilingData), (void**)&mDataPtr, __LINE__))
            return false;
        mDataSize = newDataSize;
        mDataUsedEntryCount = 0;

        // If there was a hash map before, fill entries in new map and free old map
        if (oldDataPtr)
        {
            for (unsigned int i = 0; i < oldDataSize; ++i)
            {
                if (oldDataPtr[i].scopeName)
                {
                    const ProfilingData& oldEntry = oldDataPtr[i];
                    ProfilingData& newEntry = getEntry(oldEntry.scopeName, oldEntry.scopeLine);
                    newEntry.numOfExec = oldEntry.numOfExec;
                    newEntry.runtimeSum = oldEntry.runtimeSum;
                    newEntry.runtimeMin = oldEntry.runtimeMin;
                    newEntry.runtimeMax = oldEntry.runtimeMax;
                }
            }
            ASSERT(mDataUsedEntryCount == oldDataUsedEntryCount);

            freePool(oldDataPtr);
        }

        return true;
    }

    // Return entry of given key (pair of name and line), create entry if not found. Assumes caller has acquired mLock.
    ProfilingData& getEntry(const char* scopeName, unsigned long long scopeLine)
    {
        ASSERT(mDataPtr && mDataSize > 0);          // requires data to be initialized
        ASSERT(mDataSize > mDataUsedEntryCount);    // requires at least one free slot
        ASSERT((mDataSize & (mDataSize - 1)) == 0); // mDataSize must be 2^N

        const unsigned long long mask = (mDataSize - 1);
        unsigned long long i = hashFunction(scopeName, scopeLine) & mask;
    iteration:
        if (mDataPtr[i].scopeName == scopeName && mDataPtr[i].scopeLine == scopeLine)
        {
            // found entry in hash map
            return mDataPtr[i];
        }
        else
        {
            if (!mDataPtr[i].scopeName)
            {
                // free slot -> entry not available yet -> add new entry
                ++mDataUsedEntryCount;
                mDataPtr[i].scopeName = scopeName;
                mDataPtr[i].scopeLine = scopeLine;
                mDataPtr[i].runtimeMin = (unsigned long long)-1;
                return mDataPtr[i];
            }
            else
            {
                // hash collision -> check next entry in hash map
                i = (i + 1) & mask;
                goto iteration;
            }
        }
    }

    // Compute hash sum for given key
    static unsigned long long hashFunction(const char* scopeName, unsigned long long scopeLine)
    {
        return (((unsigned long long)scopeName) ^ scopeLine);
    }

#ifdef NO_UEFI
    static bool writeStringToFile(FILE* file, const CHAR16* str)
    {
        unsigned int size = stringLength(str) * sizeof(CHAR16);
        return fwrite(str, 1, size, file) == size;
    }
#else
    static bool writeStringToFile(EFI_FILE_PROTOCOL* file, const CHAR16* str)
    {
        unsigned int strLen = stringLength(str);
        char* buffer = (char*)str;
        unsigned long long totalSize = strLen * sizeof(CHAR16);
        unsigned long long writtenSize = 0;
        EFI_STATUS status;
        while (writtenSize < totalSize)
        {
            const unsigned long long remainingSize = totalSize - writtenSize;
            const unsigned long long sizeToWrite = (WRITING_CHUNK_SIZE <= remainingSize ? WRITING_CHUNK_SIZE : remainingSize);
            unsigned long long size = sizeToWrite;
            status = file->Write(file, &size, &buffer[writtenSize]);
            if (status || size != sizeToWrite)
            {
                logStatusToConsole(L"EFI_FILE_PROTOCOL::Write() failed in ProfilingDataCollector::writeToFile()", status, __LINE__);

                return false;
            }
            writtenSize += size;
        }
        return true;
    }
#endif
};

// Global profiling data collector used by ProfilingScope
GLOBAL_VAR_DECL ProfilingDataCollector gProfilingDataCollector;


// Measure profiling statistics during life-time of object (from construction to destruction)
class ProfilingScope
{
public:
    ProfilingScope(const char* scopeName, unsigned long long scopeLine) : mScopeName(scopeName), mScopeLine(scopeLine), mStartTsc(__rdtsc())
    {
    }

    ~ProfilingScope()
    {
        gProfilingDataCollector.addMeasurement(mScopeName, mScopeLine, mStartTsc, __rdtsc());
    }

protected:
    const char* mScopeName;
    unsigned long long mScopeLine;
    unsigned long long mStartTsc;
};

#ifdef ENABLE_PROFILING
#define PROFILE_SCOPE() ProfilingScope __profilingScopeObject(__FUNCTION__, __LINE__)
#define PROFILE_NAMED_SCOPE(name) ProfilingScope __profilingScopeObject(name, __LINE__)
#else
#define PROFILE_SCOPE()
#define PROFILE_NAMED_SCOPE(name)
#endif
