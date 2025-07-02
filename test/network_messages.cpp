#include "gtest/gtest.h"

#define NETWORK_MESSAGES_WITHOUT_CORE_DEPENDENCIES
#include "../src/network_messages/all.h"


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

TEST(TestCoreRequestResponseHeader, TestPayload) {
    RequestResponseHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    EXPECT_EQ(hdr.getPayload<char>(), ((char*)&hdr) + sizeof(RequestResponseHeader));

    EXPECT_TRUE(hdr.checkAndSetSize(1234 + sizeof(RequestResponseHeader)));
    EXPECT_TRUE(hdr.checkPayloadSize(1234));
    EXPECT_FALSE(hdr.checkPayloadSize(1235));
    EXPECT_TRUE(hdr.checkPayloadSizeMinMax(1234, 1234));
    EXPECT_TRUE(hdr.checkPayloadSizeMinMax(123, 1234));
    EXPECT_TRUE(hdr.checkPayloadSizeMinMax(1234, 12346));
    EXPECT_TRUE(hdr.checkPayloadSizeMinMax(123, 12346));
    EXPECT_FALSE(hdr.checkPayloadSizeMinMax(1235, 1235));
    EXPECT_FALSE(hdr.checkPayloadSizeMinMax(12, 13));
    EXPECT_FALSE(hdr.checkPayloadSizeMinMax(13, 12));
    EXPECT_EQ(hdr.getPayloadSize(), 1234);
}

TEST(TestCoreCommonDef, IPv4Address) {
    const unsigned char ip_char_array[4] = { 1, 2, 3, 4 };
    const IPv4Address& ip_ref = *reinterpret_cast<const IPv4Address*>(ip_char_array);
    EXPECT_EQ(ip_ref.u8[0], 1);
    EXPECT_EQ(ip_ref.u8[1], 2);
    EXPECT_EQ(ip_ref.u8[2], 3);
    EXPECT_EQ(ip_ref.u8[3], 4);
    EXPECT_EQ(ip_ref.u32, 0x04030201);

    IPv4Address ip2 {{100, 200, 32, 4}};
    EXPECT_TRUE(ip_ref != ip2);
    EXPECT_FALSE(ip_ref == ip2);

    ip2 = ip_ref;
    EXPECT_TRUE(ip_ref == ip2);
    EXPECT_FALSE(ip_ref != ip2);
}
