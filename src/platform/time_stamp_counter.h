// The TSC (time stamp counter) is a x86 register counting CPU clock cycles.
// Given several assumptions such as no CPU seed change and no hibernating, it
// can be used to measure run-time with low overhead.
// See https://en.wikipedia.org/wiki/Time_Stamp_Counter

#pragma once

#include <intrin.h>

#include "console_logging.h"

// frequency of CPU clock
static unsigned long long frequency;


static void initTimeStampCounter()
{
    int cpuInfo[4];
    __cpuid(cpuInfo, 0x15);
    if (cpuInfo[2] == 0 || cpuInfo[1] == 0 || cpuInfo[0] == 0)
    {
        logToConsole(L"Theoretical TSC frequency = n/a.");
    }
    else
    {
        setText(message, L"Theoretical TSC frequency = ");
        appendNumber(message, ((unsigned long long)cpuInfo[1]) * cpuInfo[2] / cpuInfo[0], TRUE);
        appendText(message, L" Hz.");
        logToConsole(message);
    }

    frequency = __rdtsc();
    bs->Stall(1000000);
    frequency = __rdtsc() - frequency;
    setText(message, L"Practical TSC frequency = ");
    appendNumber(message, frequency, TRUE);
    appendText(message, L" Hz.");
    logToConsole(message);
}
