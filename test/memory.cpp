#include "gtest/gtest.h"

#include "../src/platform/memory.h"

TEST(Test_GetFreeRAMSize, Max) {
    bool status;
    unsigned long long testVar1, testVar2;
    void** buffer;

    testVar1 = GetFreeRAMSize();
    status = allocatePool(30, buffer);
    testVar2 = GetFreeRAMSize();

    EXPECT_EQ(testVar1, testVar2 - 30);
}


