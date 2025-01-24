#pragma once

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

// enable all logging that is used in tests
#include "private_settings.h"
#undef LOG_DUST_BURNINGS
#undef LOG_SPECTRUM_STATS
#define LOG_DUST_BURNINGS 1
#define LOG_SPECTRUM_STATS 1

// reduced size of logging buffer (512 MB instead of 8 GB)
#define LOG_BUFFER_SIZE (2*268435456ULL)

// also reduce size of logging tx index by reducing maximum number of ticks per epoch
#include "public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 3000

#include "logging/logging.h"

class LoggingTest
{
public:
    LoggingTest()
    {
        EXPECT_TRUE(qLogger::initLogging());
    }

    ~LoggingTest()
    {
        qLogger::deinitLogging();
    }
};
