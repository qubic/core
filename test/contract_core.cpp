#define NO_UEFI

#include "gtest/gtest.h"

#define TRACK_MAX_STACK_BUFFER_SIZE
#include "../src/contract_core/stack_buffer.h"

TEST(TestCoreContractCore, StackBuffer)
{
    StackBuffer<unsigned char, 120> s1;
    s1.init();
    EXPECT_EQ(s1.capacity(), 120);
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 0);
    EXPECT_EQ(s1.failedAllocAttempts(), 0);

    EXPECT_NE(s1.allocate(70), nullptr);    // success
    EXPECT_EQ(s1.allocate(50), nullptr);    // fail
    EXPECT_EQ(s1.allocate(100), nullptr);   // fail
    EXPECT_EQ(s1.allocate(255), nullptr);   // fail
    EXPECT_EQ(s1.size(), 71);
    s1.free();
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 71);
    EXPECT_EQ(s1.failedAllocAttempts(), 3);

    EXPECT_NE(s1.allocate(10), nullptr);    // success
        EXPECT_NE(s1.allocate(20), nullptr);    // success
            EXPECT_NE(s1.allocate(30), nullptr);    // success
                EXPECT_NE(s1.allocate(40), nullptr);    // success
                    EXPECT_EQ(s1.size(), 104);
                s1.free();
                EXPECT_EQ(s1.size(), 63);
                EXPECT_NE(s1.allocate(20), nullptr);    // success
                    EXPECT_EQ(s1.size(), 84);
                    EXPECT_EQ(s1.allocate(50), nullptr);    // fail
                    EXPECT_EQ(s1.allocate(100), nullptr);   // fail
                    EXPECT_EQ(s1.allocate(255), nullptr);   // fail
                s1.free();
                EXPECT_EQ(s1.size(), 63);
            s1.free();
            EXPECT_EQ(s1.size(), 32);
        s1.free();
        EXPECT_EQ(s1.size(), 11);
    s1.free();
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 104);
    EXPECT_EQ(s1.failedAllocAttempts(), 6);

    char* p = s1.allocate(70);
    *p = 100;
    EXPECT_EQ(*p, 100);
    s1.free();
}
