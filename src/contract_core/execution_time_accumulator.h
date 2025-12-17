#pragma once

#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"
#include "../contracts/math_lib.h"

// A class for accumulating contract execution time over a phase.
// Also saves the accumulation result of the previous phase.
class ExecutionTimeAccumulator
{
private:
    // Two arrays to accumulate and save the contract execution time (as CPU clock cycles) for two consecutive phases.
    // This only includes actions that are charged an execution fee (digest computation, system procedures, user procedures).
    // contractExecutionTimePerPhase[contractExecutionTimeActiveArrayIndex] is used to accumulate the contract execution ticks for the current phase n.
    // contractExecutionTimePerPhase[!contractExecutionTimeActiveArrayIndex] saves the contract execution ticks from the previous phase n-1 that are sent out as transactions in phase n.
    
    unsigned long long contractExecutionTimePerPhase[2][contractCount];
    bool contractExecutionTimeActiveArrayIndex = 0;
    volatile char lock = 0;

public:
    void init()
    {
        setMem((void*)contractExecutionTimePerPhase, sizeof(contractExecutionTimePerPhase), 0);
        contractExecutionTimeActiveArrayIndex = 0;

        ASSERT(lock == 0);
    }

    void acquireLock()
    {
        ACQUIRE(lock);
    }

    void releaseLock()
    {
        RELEASE(lock);
    }

    void startNewAccumulation()
    {
        ACQUIRE(lock);
        contractExecutionTimeActiveArrayIndex = !contractExecutionTimeActiveArrayIndex;
        setMem((void*)contractExecutionTimePerPhase[contractExecutionTimeActiveArrayIndex], sizeof(contractExecutionTimePerPhase[contractExecutionTimeActiveArrayIndex]), 0);
        RELEASE(lock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"Switched contract execution time array for new accumulation phase");
#endif
    }

    // Converts the input time specified as CPU ticks to microseconds and accumulates it for the current phase.
    // If the CPU frequency is not available, the time will be added as raw CPU ticks.
    void addTime(unsigned int contractIndex, unsigned long long time)
    {
        unsigned long long timeMicroSeconds = frequency > 0 ? (time * 1000000 / frequency) : time;
        ACQUIRE(lock);
        contractExecutionTimePerPhase[contractExecutionTimeActiveArrayIndex][contractIndex] =
            math_lib::sadd(contractExecutionTimePerPhase[contractExecutionTimeActiveArrayIndex][contractIndex], timeMicroSeconds);
        RELEASE(lock);

#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[128];
        setText(dbgMsgBuf, L"Execution time added for contract ");
        appendNumber(dbgMsgBuf, contractIndex, FALSE);
        appendText(dbgMsgBuf, L": ");
        appendNumber(dbgMsgBuf, timeMicroSeconds, FALSE);
        appendText(dbgMsgBuf, L" microseconds");
        addDebugMessage(dbgMsgBuf);
#endif
    }

    // Returns a pointer to the accumulated times from the previous phase for each contract.
    // Make sure to acquire the lock before calling this function and only release it when finished accessing the returned data.
    const unsigned long long* getPrevPhaseAccumulatedTimes()
    {
        return contractExecutionTimePerPhase[!contractExecutionTimeActiveArrayIndex];
    }

    bool saveToFile(const CHAR16* fileName, const CHAR16* directory = NULL)
    {
        long long savedSize = save(fileName, sizeof(ExecutionTimeAccumulator), (unsigned char*)this, directory);
        if (savedSize == sizeof(ExecutionTimeAccumulator))
            return true;
        else
            return false;
    }

    bool loadFromFile(const CHAR16* fileName, const CHAR16* directory = NULL)
    {
        long long loadedSize = load(fileName, sizeof(ExecutionTimeAccumulator), (unsigned char*)this, directory);
        if (loadedSize == sizeof(ExecutionTimeAccumulator))
            return true;
        else
            return false;
    }

};
