#include "gtest/gtest.h"

#include "../src/network_messages.h"

TEST(TestCoreRequestResponseHeader, TestSize) {
    RequestResponseHeader hdr;
    memset(& hdr, 0, sizeof(hdr));
    EXPECT_EQ(0, hdr.size());
    EXPECT_EQ(0, hdr.type());
    EXPECT_EQ(0, hdr.dejavu());

    EXPECT_TRUE(hdr.checkAndSetSize(1));
    EXPECT_EQ(1, hdr.size());

    hdr.setSize<10>();
    EXPECT_EQ(10, hdr.size());

    EXPECT_TRUE(hdr.checkAndSetSize(13579864));
    EXPECT_EQ(13579864, hdr.size());

    hdr.setSize<9876543>();
    EXPECT_EQ(9876543, hdr.size());

    EXPECT_TRUE(hdr.checkAndSetSize(0xffffff));
    EXPECT_EQ(0xffffff, hdr.size());

    // maximum size is 0xffffff = 16777215
    EXPECT_FALSE(hdr.checkAndSetSize(RequestResponseHeader::max_size + 1));
    EXPECT_FALSE(hdr.checkAndSetSize(RequestResponseHeader::max_size * 2));
}
