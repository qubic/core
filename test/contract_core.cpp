#define NO_UEFI

#include "gtest/gtest.h"

#define TRACK_MAX_STACK_BUFFER_SIZE
#include "../src/contract_core/stack_buffer.h"
#include "../src/contract_core/contract_action_tracker.h"

TEST(TestCoreContractCore, StackBuffer)
{
    StackBuffer<unsigned char, 120> s1;
    s1.init();
    EXPECT_EQ(s1.capacity(), 120);
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 0);
    EXPECT_EQ(s1.failedAllocAttempts(), 0);
    EXPECT_FALSE(s1.free());

    EXPECT_NE(s1.allocate(70), nullptr);    // success
    EXPECT_EQ(s1.allocate(50), nullptr);    // fail
    EXPECT_EQ(s1.allocate(100), nullptr);   // fail
    EXPECT_EQ(s1.allocate(255), nullptr);   // fail
    EXPECT_EQ(s1.size(), 71);
    EXPECT_TRUE(s1.free());
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 71);
    EXPECT_EQ(s1.failedAllocAttempts(), 3);

    EXPECT_NE(s1.allocate(10), nullptr);    // success
        EXPECT_NE(s1.allocate(20), nullptr);    // success
            EXPECT_NE(s1.allocate(30), nullptr);    // success
                EXPECT_NE(s1.allocate(40), nullptr);    // success
                    EXPECT_EQ(s1.size(), 104);
                EXPECT_TRUE(s1.free());
                EXPECT_EQ(s1.size(), 63);
                EXPECT_NE(s1.allocate(20), nullptr);    // success
                    EXPECT_EQ(s1.size(), 84);
                    EXPECT_EQ(s1.allocate(50), nullptr);    // fail
                    EXPECT_EQ(s1.allocate(100), nullptr);   // fail
                    EXPECT_EQ(s1.allocate(255), nullptr);   // fail
                EXPECT_TRUE(s1.free());
                EXPECT_EQ(s1.size(), 63);
            EXPECT_TRUE(s1.free());
            EXPECT_EQ(s1.size(), 32);
        EXPECT_TRUE(s1.free());
        EXPECT_EQ(s1.size(), 11);
    EXPECT_TRUE(s1.free());
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 104);
    EXPECT_EQ(s1.failedAllocAttempts(), 6);
    EXPECT_FALSE(s1.free());

    char* p;
    EXPECT_NE(p = s1.allocate(70), nullptr);    // success
    *p = 100;
    EXPECT_EQ(*p, 100);
    EXPECT_NE(p = s1.allocate(40), nullptr);    // success
    *p = 20;
    EXPECT_EQ(*p, 20);
    EXPECT_EQ(s1.size(), 112);
    s1.freeAll();
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 112);
    EXPECT_EQ(s1.failedAllocAttempts(), 6);

    char* ptr;
    unsigned char sz;
    bool special;
    EXPECT_FALSE(s1.unwind(ptr, sz, special));

    EXPECT_NE(p = s1.allocate(112, false), nullptr);    // success
    EXPECT_EQ(s1.size(), 113);
    EXPECT_EQ(s1.maxSizeObserved(), 113);
    EXPECT_TRUE(s1.unwind(ptr, sz, special));
    EXPECT_EQ(sz, 112);
    EXPECT_EQ(ptr, p);
    EXPECT_EQ(special, false);

    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.maxSizeObserved(), 113);
    EXPECT_FALSE(s1.unwind(ptr, sz, special));

    EXPECT_EQ(p = s1.allocate(120, true), nullptr);    // fail
    EXPECT_EQ(s1.failedAllocAttempts(), 7);

    EXPECT_NE(p = s1.allocate(119, true), nullptr);    // success
    EXPECT_EQ(s1.size(), 120);
    EXPECT_EQ(s1.maxSizeObserved(), 120);
    EXPECT_TRUE(s1.unwind(ptr, sz, special));
    EXPECT_EQ(sz, 119);
    EXPECT_EQ(ptr, p);
    EXPECT_EQ(special, true);

    StackBuffer<unsigned int, 128000> s2;
    s2.init();
    EXPECT_EQ(s2.capacity(), 128000);
    EXPECT_EQ(s2.size(), 0);
    EXPECT_EQ(s2.maxSizeObserved(), 0);
    EXPECT_EQ(s2.failedAllocAttempts(), 0);
    EXPECT_FALSE(s2.free());

    constexpr int depth = 10;
    unsigned int sz2;
    char* ptrArray[depth];
    for (int i = 0; i < depth; ++i)
    {
        EXPECT_NE(ptrArray[i] = s2.allocate(i * 1000, i % 3 == 0), nullptr);    // success
    }
    for (int i = depth - 1; i >= 0; --i)
    {
        EXPECT_TRUE(s2.unwind(ptr, sz2, special));
        EXPECT_EQ(sz2, i * 1000);
        EXPECT_EQ(ptr, ptrArray[i]);
        EXPECT_EQ(special, i % 3 == 0);
    }
}

TEST(TestCoreContractCore, ContractActionTracker)
{
    m256i id0(0, 1, 2, 3);
    m256i id1(2, 3, 4, 5);
    m256i id2(3, 4, 5, 6);

    ContractActionTracker<6> at;
    EXPECT_TRUE(at.allocBuffer());
    at.init();
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), 0);

    EXPECT_TRUE(at.addQuTransfer(id0, id1, 100));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), -100);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 100);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 0);

    EXPECT_TRUE(at.addQuTransfer(id1, id0, 100));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), 0);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 0);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 0);

    EXPECT_TRUE(at.addQuTransfer(id0, id1, 1000));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), -1000);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 1000);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 0);

    EXPECT_TRUE(at.addQuTransfer(id1, id2, 800));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), -1000);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 200);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 800);

    EXPECT_TRUE(at.addQuTransfer(id2, id0, 500));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), -500);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 200);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 300);

    // Transfer to own address does not change anything
    EXPECT_TRUE(at.addQuTransfer(id0, id0, 10000));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), -500);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 200);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 300);

    // Fails because action cannot be stored
    EXPECT_FALSE(at.addQuTransfer(id2, id0, 500));
    EXPECT_EQ(at.getOverallQuTransferBalance(id0), -500);
    EXPECT_EQ(at.getOverallQuTransferBalance(id1), 200);
    EXPECT_EQ(at.getOverallQuTransferBalance(id2), 300);

    at.freeBuffer();
}
