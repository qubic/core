#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/platform/read_write_lock.h"
#include "../src/platform/stack_size_tracker.h"
#include "../src/platform/custom_stack.h"
#include "../src/platform/profiling.h"

#include "common_buffers.h"
#include <thread>

TEST(TestCoreReadWriteLock, SimpleSingleThread)
{
    ReadWriteLock l;
    l.reset();

    // Aquire lock for multiple readers
    constexpr int numReaders = 10;
    for (int i = 0; i < numReaders; ++i)
    {
        EXPECT_TRUE(l.tryAcquireRead());
        EXPECT_FALSE(l.tryAcquireWrite());
    }

    // Release read locks
    for (int i = 0; i < numReaders; ++i)
    {
        l.releaseRead();
    }

    // Aquire lock for writer
    EXPECT_TRUE(l.tryAcquireWrite());
    EXPECT_FALSE(l.tryAcquireWrite());
    EXPECT_FALSE(l.tryAcquireRead());

    // Release write lock
    l.releaseWrite();

    l.acquireRead();
    l.releaseRead();

    l.acquireWrite();
    l.releaseWrite();
}


StackSizeTracker stackSizeTracker;

template <unsigned int stackVarSize>
void recursion(int i)
{
    // use volatile and get address to make sure this is not optimized away and kept on stack
    volatile unsigned long long data[stackVarSize];
    data[0] = (unsigned long long)(volatile unsigned long long*)data;

    unsigned long long oldSize = stackSizeTracker.maxStackSize();
    stackSizeTracker.update();
    unsigned long long newSize = stackSizeTracker.maxStackSize();

    EXPECT_NE(newSize, StackSizeTracker::uninitialized);
    EXPECT_GT(newSize, oldSize);

    if (i > 0)
        recursion<stackVarSize>(i - 1);
}

template <unsigned int stackVarSize>
unsigned long long testStackSizeTracker(int recursionLevel)
{
    INIT_STACK_SIZE_TRACKER(stackSizeTracker);
    long long data[10]; data[0] = 0;
    stackSizeTracker.update();
    auto size = stackSizeTracker.maxStackSize();
    {
        // this does not change size -> shows that storage of all stack variables of function
        // is reserved, independently of scope
        long long data2[10]; data2[0] = 0;
        stackSizeTracker.update();
        size = stackSizeTracker.maxStackSize();
    }
    recursion<stackVarSize>(recursionLevel);
    size = stackSizeTracker.maxStackSize();
    stackSizeTracker.reset();
    return size;
}

// Test that max. measured stack size is kept if tracker is reused with other stack top address
// (relevant if restarted processor/thread gets other stack address)
void testStackSizeTrackerKeepSizeCheck(unsigned long long prevSize)
{
    // check that max size is kept on reuse
    INIT_STACK_SIZE_TRACKER(stackSizeTracker);
    EXPECT_EQ(stackSizeTracker.maxStackSize(), prevSize);
}
void testStackSizeTrackerKeepSize()
{
    // first use
    stackSizeTracker.reset();
    INIT_STACK_SIZE_TRACKER(stackSizeTracker);
    UPDATE_STACK_SIZE_TRACKER(stackSizeTracker);
    testStackSizeTrackerKeepSizeCheck(stackSizeTracker.maxStackSize());
}


TEST(TestCoreStackSizeTracker, SimpleTest)
{
    // Return uninit if not not both functions are called
    DEFINE_STACK_SIZE_TRACKER(s1);
    EXPECT_EQ(s1.maxStackSize(), StackSizeTracker::uninitialized);
    INIT_STACK_SIZE_TRACKER(s1);
    EXPECT_EQ(s1.maxStackSize(), StackSizeTracker::uninitialized);
    DEFINE_STACK_SIZE_TRACKER(s2);
    EXPECT_EQ(s2.maxStackSize(), StackSizeTracker::uninitialized);
    UPDATE_STACK_SIZE_TRACKER(s2);
    EXPECT_EQ(s2.maxStackSize(), StackSizeTracker::uninitialized);

    // Test that more recursion and more data per function lead to bigger stack size
    EXPECT_LT(testStackSizeTracker<10>(0), testStackSizeTracker<10>(1));
    EXPECT_LT(testStackSizeTracker<10>(5), testStackSizeTracker<10>(10));
    EXPECT_LT(testStackSizeTracker<10>(10), testStackSizeTracker<20>(10));
    EXPECT_LT(testStackSizeTracker<30>(20), testStackSizeTracker<30>(40));

    // Test that max. measured stack size is kept if tracker is reused with other stack top address
    testStackSizeTrackerKeepSize();
}


void customStackTest1(void* data)
{
    EXPECT_EQ(data, (void*)5);
}

void customStackTest2(void* data)
{
    int recursionLevel = (int)(unsigned long long)data;
    testStackSizeTracker<10>(recursionLevel);
}


TEST(TestCoreCustomStack, SimpleTest)
{
    CustomStack s;
    s.init();
    EXPECT_EQ(s.maxStackUsed(), 0);
    EXPECT_TRUE(s.alloc(128*1024));
    EXPECT_EQ(s.maxStackUsed(), 0);

    // run function with small stack requirements on custom stack
    s.setupFunction(customStackTest1, (void*)5);
    CustomStack::runFunction(&s);
    auto size1 = s.maxStackUsed();
    EXPECT_GT(size1, 0u);

    // run recursive function with small stack requirements on custom stack
    s.setupFunction(customStackTest2, (void*)1);
    CustomStack::runFunction(&s);
    auto size2 = s.maxStackUsed();
    EXPECT_GT(size2, size1);

    // run recursive function with medium stack requirements on custom stack
    s.setupFunction(customStackTest2, (void*)5);
    CustomStack::runFunction(&s);
    auto size3 = s.maxStackUsed();
    EXPECT_GT(size3, size2);

    // run recursive function with large stack requirements on custom stack
    s.setupFunction(customStackTest2, (void*)10);
    CustomStack::runFunction(&s);
    auto size4 = s.maxStackUsed();
    EXPECT_GT(size4, size3);
}

void recursiveProfilingTest(int depth)
{
    ProfilingScope profScope(__FUNCTION__, __LINE__);
    if (depth > 0)
    {
        recursiveProfilingTest(depth - 1);
        recursiveProfilingTest(depth - 2);
    }
    else
    {
        sleepMicroseconds(10);
    }
}

void iterativeProfilingTest(int n)
{
    ProfilingScope profScope(__FUNCTION__, __LINE__);
    for (int i = 0; i < n; ++i)
    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        for (int j = 0; j < n; ++j)
        {
            ProfilingScope profScope(__FUNCTION__, __LINE__);
            for (int k = 0; k < n; ++k)
            {
                ProfilingScope profScope(__FUNCTION__, __LINE__);
                sleepMicroseconds(10);
            }
        }
    }
}

TEST(TestCoreProfiling, SleepTest)
{
    ProfilingStopwatch profStopwatch(__FUNCTION__, __LINE__);
    profStopwatch.start();

    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        testStackSizeTrackerKeepSize();
    }

    for (int i = 0; i < 1000; ++i)
    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        testStackSizeTrackerKeepSize();
    }

    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        recursiveProfilingTest(9);
    }

    profStopwatch.stop();

    for (int i = 0; i < 10; ++i)
    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        recursiveProfilingTest(4);

        // calling start() multiple times is no problem (last time is relevant for measuring)
        profStopwatch.start();
    }

    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        iterativeProfilingTest(5);
        profStopwatch.stop();
    }

    for (int i = 0; i < 5; ++i)
    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        iterativeProfilingTest(3);
    }

    gProfilingDataCollector.writeToFile();

    EXPECT_FALSE(gProfilingDataCollector.init(2));
    gProfilingDataCollector.clear();
    gProfilingDataCollector.deinit();
    gProfilingDataCollector.clear();

    //gProfilingDataCollector.writeToFile();
}

static ProfilingStopwatch gProfStopwatch(__FILE__, __LINE__);

TEST(TestCoreProfiling, AddMeasurementSpeedTest)
{
    for (unsigned long long i = 0; i < 10000; ++i)
    {
        ProfilingScope profScope(__FUNCTION__, __LINE__);
        gProfStopwatch.start();
        for (unsigned long long j = 0; j < 10000; ++j)
        {
            ProfilingScope profScope(__FUNCTION__, __LINE__);
            for (volatile unsigned long long k = 0; k < 10; ++k)
            {
            }
        }
        gProfStopwatch.stop();
    }

    gProfilingDataCollector.writeToFile();
}

void checkTicksToMicroseconds(int type, unsigned long long ticks, unsigned long long frequency)
{
    ::frequency = frequency;
    unsigned long long microsecondsInt = ProfilingDataCollector::ticksToMicroseconds(ticks);
    long double microsecondsFloat = long double(ticks) * long double(1000000) / long double(frequency);
    long double diff = std::abs(microsecondsFloat - microsecondsInt);
    if (type == 0)
    {
        // no overflow
        EXPECT_LT(diff, 1.0);
    }
    else if (type == 1)
    {
        // overflow in calculation -> tolerate inaccuracy
        EXPECT_LT(diff, 1000000.0);
    }
    else
    {
        // overflow in result -> expect max value
        EXPECT_EQ(microsecondsInt, 0xffffffffffffffffllu);
    }
    ::frequency = 0;
}

TEST(TestCoreProfiling, CheckTicksToMicroseconds)
{
    // non-overflow cases
    checkTicksToMicroseconds(0, 10000, 100000000);
    checkTicksToMicroseconds(0, 100000000, 10000);
    checkTicksToMicroseconds(0, 0xffffffffffffffffllu / 1000000llu, 1000000llu);
    checkTicksToMicroseconds(0, 0xffffffffffffffffllu / 1000000llu, 1000000llu + 1);
    checkTicksToMicroseconds(0, 0xffffffffffffffffllu / 1000000llu, 1000000llu - 1);
    checkTicksToMicroseconds(0, 0xffffffffffffffffllu / 1000000llu - 1, 1000000llu);
    checkTicksToMicroseconds(0, 0xffffffffffffffffllu / 1000000llu - 1, 1000000llu + 1);
    checkTicksToMicroseconds(0, 0xffffffffffffffffllu / 1000000llu - 1, 1000000llu - 1);

    // overflow in calculation
    checkTicksToMicroseconds(1, 0xffffffffffffffffllu / 1000000llu + 1, 1000000llu);
    checkTicksToMicroseconds(1, 0xffffffffffffffffllu / 1000000llu + 1, 1000000llu + 1);
    checkTicksToMicroseconds(1, 0xffffffffffffffffllu, 1000000000llu);
    checkTicksToMicroseconds(1, 0xffffffffffffffllu, 1000000000llu);
    checkTicksToMicroseconds(1, 0xffffffffffffffffllu, 1234567890llu);
    checkTicksToMicroseconds(1, 0xffffffffffffffllu, 1234567890llu);

    // overflow in result (low frequency)
    checkTicksToMicroseconds(2, 0xffffffffffffffffllu, 1000);
    checkTicksToMicroseconds(2, 0xffffffffffffffllu, 1000);
    checkTicksToMicroseconds(2, 0xffffffffffffffffllu, 12345);
    checkTicksToMicroseconds(2, 0xffffffffffffffffllu, 123456);
}

static volatile bool waitingForAcquire = false;

static void acquireAndReleaseCommonBuffer()
{
    waitingForAcquire = true;
    void* buf = commonBuffers.acquireBuffer(64);
    waitingForAcquire = false;
    EXPECT_NE(buf, nullptr);
    sleepMilliseconds(100);
    commonBuffers.releaseBuffer(buf);
}

TEST(TestCoreCommonBuffers, OneBuffer)
{
    if (!frequency)
        initTimeStampCounter();

    // init fail: 0 buffers / size 0
    EXPECT_FALSE(commonBuffers.init(0, 1024));
    EXPECT_FALSE(commonBuffers.init(1, 0));

    // init success
    EXPECT_TRUE(commonBuffers.init(1, 1024));
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // acquire fail: requested buffer size too big
    void* buf = commonBuffers.acquireBuffer(2000);
    EXPECT_EQ(buf, nullptr);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // acquire success
    buf = commonBuffers.acquireBuffer(1024);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);

    // release success
    commonBuffers.releaseBuffer(buf);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // check stats
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 0);
    EXPECT_EQ(commonBuffers.getMaxWaitingProcessorCount(), 0);

    // invalid release (double free)
    commonBuffers.releaseBuffer(buf);
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 1);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // invalid release (invalid pointer)
    commonBuffers.releaseBuffer((void*)0x123456);
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 2);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // acquire success
    buf = commonBuffers.acquireBuffer(512);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);

    // acquire in other thread has to wait
    auto thread = std::thread(acquireAndReleaseCommonBuffer);
    sleepMilliseconds(100);
    EXPECT_TRUE(waitingForAcquire);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);

    // release buffer -> other thread can acquire it
    commonBuffers.releaseBuffer(buf);
    sleepMilliseconds(50);  // after 50 ms the other thread should have acquired the buffer
    EXPECT_FALSE(waitingForAcquire);
    thread.join(); // after ending the thread, the buffer is release again
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    EXPECT_EQ(commonBuffers.getMaxWaitingProcessorCount(), 1);

    // free all buffers, reset state
    commonBuffers.deinit();
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 0);
    EXPECT_EQ(commonBuffers.getMaxWaitingProcessorCount(), 0);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);
}

TEST(TestCoreCommonBuffers, ThreeBuffers)
{
    if (!frequency)
        initTimeStampCounter();

    // init success
    EXPECT_TRUE(commonBuffers.init(3, 1024));
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // acquire fail: requested buffer size too big
    void* buf = commonBuffers.acquireBuffer(2000);
    EXPECT_EQ(buf, nullptr);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // acquire success
    buf = commonBuffers.acquireBuffer(1024);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);

    // release
    commonBuffers.releaseBuffer(buf);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // acquire three buffer success
    void* buf2 = commonBuffers.acquireBuffer(1024);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);
    void* buf3 = commonBuffers.acquireBuffer(42);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 2);
    buf = commonBuffers.acquireBuffer(123);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 3);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(buf3, nullptr);

    // release three buffers
    commonBuffers.releaseBuffer(buf);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 2);
    commonBuffers.releaseBuffer(buf2);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);
    commonBuffers.releaseBuffer(buf3);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    // check stats
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 0);
    EXPECT_EQ(commonBuffers.getMaxWaitingProcessorCount(), 1); // heuristic is off by one (in facto no waiting needed)

    // invalid release (double free)
    commonBuffers.releaseBuffer(buf);
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 1);

    // invalid release (invalid pointer)
    commonBuffers.releaseBuffer((void*)0x123456);
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 2);

    // acquire three buffers
    buf = commonBuffers.acquireBuffer(512);
    buf2 = commonBuffers.acquireBuffer(1024);
    buf3 = commonBuffers.acquireBuffer(987);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(buf3, nullptr);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 3);

    // acquire in two other thread has to wait
    auto thread1 = std::thread(acquireAndReleaseCommonBuffer);
    auto thread2 = std::thread(acquireAndReleaseCommonBuffer);
    sleepMilliseconds(100);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 3);
    EXPECT_TRUE(waitingForAcquire);

    // release 2 buffers -> other threads can acquire them
    commonBuffers.releaseBuffer(buf);
    commonBuffers.releaseBuffer(buf2);
    sleepMilliseconds(50);
    EXPECT_FALSE(waitingForAcquire);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 3);
    thread1.join();
    thread2.join();
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 1);

    commonBuffers.releaseBuffer(buf3);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);

    EXPECT_EQ(commonBuffers.getMaxWaitingProcessorCount(), 2);

    // free all buffers, reset state
    commonBuffers.deinit();
    EXPECT_EQ(commonBuffers.getInvalidReleaseCount(), 0);
    EXPECT_EQ(commonBuffers.getMaxWaitingProcessorCount(), 0);
    EXPECT_EQ(commonBuffers.acquiredBuffers(), 0);
}
