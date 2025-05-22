#pragma once

#include "global_var.h"
#include "assert.h"
#include "memory_util.h"
#include "time_stamp_counter.h"
#include "file_io.h"

struct ProfilingData
{
    const char* name;
    unsigned long long line;
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

    // Add run-time measurement to profiling entry of given key (= name + line).
    // For fast access, the address of the name is used to identify the measurement. Using string literals here is recommended.
    // Using a pointer to a dynamic buffer probably cuases problems.
    // The time stamp counter values startTsc and endTsc should be measured on the same processor, because TSC of
    // different processors may be not synchronized.
    void addMeasurement(const char* name, unsigned long long line, unsigned long long startTsc, unsigned long long endTsc)
    {
        // Discard measurement on overflow of time stamp counter register
        if (endTsc < startTsc)
            return;

        ACQUIRE_WITHOUT_DEBUG_LOGGING(mLock);

        // Make sure hash map is initialized
        if (mDataPtr || doInit())
        {
            // Fill entry
            const unsigned long long dt = endTsc - startTsc;
            ProfilingData& newEntry = getEntry(name, line);
            newEntry.numOfExec += 1;
            newEntry.runtimeSum += dt;
            if (newEntry.runtimeMin > dt)
                newEntry.runtimeMin = dt;
            if (newEntry.runtimeMax < dt)
                newEntry.runtimeMax = dt;
        }

        RELEASE(mLock);
    }
    
    // Write CSV file with ProfilingData and hash function values (to check distribution)
    bool writeToFile()
    {
        ASSERT(isMainProcessor());

        // Init time stamp counter frequency if needed
        if (!frequency)
        {
            initTimeStampCounter();
#ifdef NO_UEFI
            const unsigned long long secondsOverflow = 0xffffffffffffffffllu / frequency;
            std::cout << "runtimeSum overflow after " << secondsOverflow << " sconds = " << secondsOverflow / 3600 << " hours" << std::endl;
#endif
        }
        ASSERT(frequency);

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
        bool okay = writeStringToFile(file, L"idx,name,line,count,sum_microseconds,avg_microseconds,min_microseconds,max_microseconds\r\n");
        for (unsigned int i = 0; i < mDataSize; ++i)
        {
            if (mDataPtr[i].name)
            {
                unsigned long long runtimeSumMicroseconds = ticksToMicroseconds(mDataPtr[i].runtimeSum);
                setNumber(message, i, false);
                appendText(message, ",\"");
                appendText(message, mDataPtr[i].name);
                appendText(message, "\",");
                appendNumber(message, mDataPtr[i].line, false);
                appendText(message, ",");
                appendNumber(message, mDataPtr[i].numOfExec, false);
                appendText(message, ",");
                appendNumber(message, runtimeSumMicroseconds, false);
                appendText(message, ",");
                appendNumber(message, (mDataPtr[i].numOfExec > 0) ? runtimeSumMicroseconds / mDataPtr[i].numOfExec : 0, false);
                appendText(message, ",");
                appendNumber(message, ticksToMicroseconds(mDataPtr[i].runtimeMin), false);
                appendText(message, ",");
                appendNumber(message, ticksToMicroseconds(mDataPtr[i].runtimeMax), false);
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

    static unsigned long long ticksToMicroseconds(unsigned long long ticks)
    {
        ASSERT(frequency);
        if (ticks <= (0xffffffffffffffffllu / 1000000llu))
        {
            return (ticks * 1000000llu) / frequency;
        }
        else
        {
            // prevent overflow of ticks * 1000000llu
            unsigned long long seconds = ticks / frequency;
            if (seconds <= (0xffffffffffffffffllu / 1000000llu))
            {
                // tolerate inaccuracy
                return seconds * 1000000llu;
            }
            else
            {
                // number of microseconds does not fit in type -> max value
                return 0xffffffffffffffffllu;
            }
        }
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
                if (oldDataPtr[i].name)
                {
                    const ProfilingData& oldEntry = oldDataPtr[i];
                    ProfilingData& newEntry = getEntry(oldEntry.name, oldEntry.line);
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
    ProfilingData& getEntry(const char* name, unsigned long long line)
    {
        ASSERT(mDataPtr && mDataSize > 0);          // requires data to be initialized
        ASSERT((mDataSize & (mDataSize - 1)) == 0); // mDataSize must be 2^N

        const unsigned long long mask = (mDataSize - 1);
        unsigned long long i = hashFunction(name, line) & mask;
    iteration:
        if (mDataPtr[i].name == name && mDataPtr[i].line == line)
        {
            // found entry in hash map
            return mDataPtr[i];
        }
        else
        {
            if (!mDataPtr[i].name)
            {
                // free slot -> entry not available yet -> add new entry
                ++mDataUsedEntryCount;
                mDataPtr[i].name = name;
                mDataPtr[i].line = line;
                mDataPtr[i].runtimeMin = (unsigned long long)-1;
                return mDataPtr[i];
            }
            else
            {
                // hash collision
                // -> check if hash map has enough free space
                if (mDataUsedEntryCount * 2 > mDataSize)
                {
                    if (doInit(mDataUsedEntryCount * 2))
                    {
                        // hash map has been extended -> restart getting entry
                        return getEntry(name, line);
                    }
                }

                // -> check next entry in hash map
                i = (i + 1) & mask;
                goto iteration;
            }
        }
    }

    // Compute hash sum for given key
    static unsigned long long hashFunction(const char* name, unsigned long long line)
    {
        return (((unsigned long long)name) ^ line);
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


// Measure profiling statistics during life-time of object (from construction to destruction).
// Construction and destruction must happen on the same processor to ensure accurate results.
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

// Measure profiling statistics with pairs of start/stop calls.
// CAUTION: Attempts to use this led to freezes on EFI. Furthermore, the main intended use case, which is measuring
// hand-over time between different processors probably doesn't work reliably because TSC may not be synced between
// processors.
// TODO: Remove or rewrite this using another way of measuring run-time.
class ProfilingStopwatch
{
public:
    ProfilingStopwatch(const char* stopwatchName, unsigned long long stopwatchLine) : mStopwatchName(stopwatchName), mStopwatchLine(stopwatchLine), mStartTsc(0)
    {
    }

    void start()
    {
        mStartTsc = __rdtsc();
    }

    void stop()
    {
        // Start should be called before stop
        ASSERT(mStartTsc != 0);

        gProfilingDataCollector.addMeasurement(mStopwatchName, mStopwatchLine, mStartTsc, __rdtsc());
        mStartTsc = 0;
    }

protected:
    const char* mStopwatchName;
    unsigned long long mStopwatchLine;
    unsigned long long mStartTsc;
};



#ifdef ENABLE_PROFILING
#define PROFILE_SCOPE() ProfilingScope __profilingScopeObject(__FUNCTION__, __LINE__)
#define PROFILE_NAMED_SCOPE(name) ProfilingScope __profilingScopeObject(name, __LINE__)
#define PROFILE_SCOPE_BEGIN() { PROFILE_SCOPE()
#define PROFILE_NAMED_SCOPE_BEGIN(name) { PROFILE_NAMED_SCOPE(name)
#define PROFILE_SCOPE_END() }
/*
See ProfilingStopwatch for comments.
#define PROFILE_STOPWATCH_DEF(objectName, descriptiveNameString) ProfilingStopwatch objectName(descriptiveNameString, __LINE__)
#define PROFILE_STOPWATCH_START(objectName) objectName.start();
#define PROFILE_STOPWATCH_STOP(objectName) objectName.stop();
*/
#else
#define PROFILE_SCOPE()
#define PROFILE_NAMED_SCOPE(name)
#define PROFILE_SCOPE_BEGIN() {
#define PROFILE_NAMED_SCOPE_BEGIN(name) {
#define PROFILE_SCOPE_END() }
/*
See ProfilingStopwatch for comments.
#define PROFILE_STOPWATCH_DEF(objectName, descriptiveNameString)
#define PROFILE_STOPWATCH_START(objectName)
#define PROFILE_STOPWATCH_STOP(objectName)
*/
#endif
