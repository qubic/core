#pragma once

// TODO: move dependencies to platform_* libs
// TODO: move to cpp file in platform_common lib (independent of NDEBUG)

#include <lib/platform_common/processor.h>
#include "concurrency.h"
#include "time_stamp_counter.h"
#include "debugging.h"

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
        setText(msgBuffer, L"Waiting for ");
        appendTextShortenBack(msgBuffer, mExpr, 50);
        appendText(msgBuffer, " in total for ");
        appendNumber(msgBuffer, (curTsc - mStartTsc) * 1000 / frequency, false);
        appendText(msgBuffer, " milliseconds in ");
        appendTextShortenFront(msgBuffer, mFile, 50);
        appendText(msgBuffer, " line ");
        appendNumber(msgBuffer, mLine, false);
        addDebugMessage(msgBuffer);
        if (isMainProcessor())
            printDebugMessages();
    }
}

void BusyWaitingTracker::wait()
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
        addDebugMessage(msgBuffer);
        if (isMainProcessor())
            printDebugMessages();
        mNextReportTscDelta *= 2;
        mTotalWaitTimeReport = true;
    }
    _mm_pause();
}

#endif
