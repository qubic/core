#pragma once

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

// enable all logging that is used in tests
#include "private_settings.h"
#undef LOG_SPECTRUM
#define LOG_SPECTRUM 1

// also reduce size of logging tx index by reducing maximum number of ticks per epoch
#include "public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 3000

// Reduce virtual memory size for testing
#undef LOG_BUFFER_PAGE_SIZE
#undef PMAP_LOG_PAGE_SIZE
#undef IMAP_LOG_PAGE_SIZE
#undef VM_NUM_CACHE_PAGE
#define LOG_BUFFER_PAGE_SIZE 10000000ULL
#define PMAP_LOG_PAGE_SIZE 1000000ULL
#define IMAP_LOG_PAGE_SIZE 300ULL
#define VM_NUM_CACHE_PAGE 1

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
