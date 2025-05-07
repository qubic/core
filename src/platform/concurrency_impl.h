#pragma once

// TODO: move dependencies to platform_* libs
// TODO: move to cpp file in platform_common lib (independent of NDEBUG)

#include <lib/platform_common/processor.h>
#include "concurrency.h"
#include "time_stamp_counter.h"
#include "debugging.h"
#include "system.h"

#ifndef NDEBUG

BusyWaitingTracker::BusyWaitingTracker(const char* expr, const char* file, unsigned int line)
{
#ifdef NO_UEFI
    if (!frequency)
        initTimeStampCounter();
#else
    ASSERT(frequency != 0);
#endif
    mStartTsc = __rdtsc();
    mNextReportTscDelta = frequency / 2; // first report after 0.5 seconds
    mExpr = expr;
    mFile = file;
    mLine = line;
    mTotalWaitTimeReport = false;
}

BusyWaitingTracker::~BusyWaitingTracker()
{
    ASSERT(frequency != 0);
    if (mTotalWaitTimeReport)
    {
        CHAR16 msgBuffer[300];
        const unsigned long long curTsc = __rdtsc();
        setText(msgBuffer, L"Finished waiting for ");
        appendTextShortenBack(msgBuffer, mExpr, 50);
        appendText(msgBuffer, " for ");
        appendNumber(msgBuffer, (curTsc - mStartTsc) * 1000 / frequency, false);
        appendText(msgBuffer, " milliseconds in ");
        appendTextShortenFront(msgBuffer, mFile, 50);
        appendText(msgBuffer, " line ");
        appendNumber(msgBuffer, mLine, false);
        appendText(msgBuffer, ", processor #");
        appendNumber(msgBuffer, getRunningProcessorID(), false);
        appendText(msgBuffer, ", tick ");
        appendNumber(msgBuffer, system.tick, false);
        addDebugMessage(msgBuffer);
        if (isMainProcessor())
            printDebugMessages();
    }
}

void BusyWaitingTracker::pause()
{
    ASSERT(frequency != 0);
    const unsigned long long curTsc = __rdtsc();
    if (curTsc > mStartTsc + mNextReportTscDelta)
    {
        CHAR16 msgBuffer[300];
        setText(msgBuffer, L"Waiting for ");
        appendTextShortenBack(msgBuffer, mExpr, 50);
        appendText(msgBuffer, " for ");
        appendNumber(msgBuffer, mNextReportTscDelta * 1000 / frequency, false);
        appendText(msgBuffer, " milliseconds in ");
        appendTextShortenFront(msgBuffer, mFile, 50);
        appendText(msgBuffer, " line ");
        appendNumber(msgBuffer, mLine, false);
        appendText(msgBuffer, ", processor #");
        appendNumber(msgBuffer, getRunningProcessorID(), false);
        addDebugMessage(msgBuffer);
        if (isMainProcessor())
            printDebugMessages();
        mNextReportTscDelta *= 2;
        mTotalWaitTimeReport = true;
    }
    _mm_pause();
}

#endif
// Atomic max
void atomicMax64(long long* dest, long long value)
{
    long long old = *dest;
    while (value > old)
    {
        long long prev = _InterlockedCompareExchange64(dest, value, old);
        if (prev == old)
        {
            break;
        }
        old = prev;
    }
}
