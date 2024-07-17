#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/public_settings.h"
#include "../src/vote_counter.h"

#include <random>


class TestVoteCounter : public VoteCounter
{
public:
    unsigned int testExtract10Bit(unsigned char* data, unsigned int idx)
    {
        return extract10Bit(data, idx);
    }
    void testUpdate10Bit(unsigned char* data, unsigned int idx, unsigned int value)
    {
        update10Bit(data, idx, value);
    }
};

TestVoteCounter tvc;


TEST(TestCoreVoteCounter, TenBitsEncodeDecode) {
    unsigned char data_u10[848];
    unsigned int data_u32[676];
    srand(1);
    for (int i = 0; i < 676; i++)
    {
        unsigned int rd = rand() % 676;
        data_u32[i] = rd;
        tvc.testUpdate10Bit(data_u10, i, rd);
    }
    bool isMatched = true;
    for (int i = 0; i < 676; i++)
    {
        unsigned int extracted = tvc.testExtract10Bit(data_u10, i);
        if (extracted != data_u32[i])
        {
            isMatched = false;
            break;
        }
    }
    EXPECT_TRUE(isMatched);
}
