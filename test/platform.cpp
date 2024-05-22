#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/platform/read_write_lock.h"
#include "../src/platform/stack_size_tracker.h"

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
