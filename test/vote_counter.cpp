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
    for (int i = 0; i < 32; i++)
    {
        srand(i);
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
}

TEST(TestCoreVoteCounter, NewVotePacketValidation) {
    unsigned char data_u10[848];
    srand(0);
    setMem(data_u10, sizeof(data_u10), 0);
    for (int i = 0; i < 451; i++)
    {
        unsigned int rd = rand() % 676;
        tvc.testUpdate10Bit(data_u10, i, rd);
    }
    tvc.testUpdate10Bit(data_u10, 0, 0);
    bool isValid = tvc.validateNewVotesPacket(data_u10, 0); // total votes is lower than 451 * 676
    EXPECT_TRUE(!isValid); 

    setMem(data_u10, sizeof(data_u10), 0);
    for (int i = 0; i < 676; i++)
    {
        unsigned int rd = 451 + rand() % (676-451);
        tvc.testUpdate10Bit(data_u10, i, rd);
    }
    tvc.testUpdate10Bit(data_u10, 0, 300);
    isValid = tvc.validateNewVotesPacket(data_u10, 0); // self-report is not zero
    EXPECT_TRUE(!isValid);

    setMem(data_u10, sizeof(data_u10), 0);
    for (int i = 0; i < 676; i++)
    {
        unsigned int rd = 451 + rand() % (676 - 451);
        tvc.testUpdate10Bit(data_u10, i, rd);
    }
    tvc.testUpdate10Bit(data_u10, 0, 0);
    isValid = tvc.validateNewVotesPacket(data_u10, 0); // valid
    EXPECT_TRUE(isValid);
}

TEST(TestCoreVoteCounter, AddGetVotes) {
    unsigned char data_u10[848] = { 0 };
    unsigned int data_u32[676] = { 0 };
    srand(0);
    tvc.init();
    for (int comp = 0; comp < 676; comp++)
    {
        for (int i = 0; i < 676; i++)
        {
            unsigned int rd = 451 + (rand() % (676-451));
            tvc.testUpdate10Bit(data_u10, i, rd);
        }
        tvc.testUpdate10Bit(data_u10, comp, 0);

        tvc.addVotes(data_u10, comp);
        for (int i = 0; i < 676; i++)
        {
            data_u32[i] += tvc.testExtract10Bit(data_u10, i);
        }
    }
    bool isMatched = true;
    for (int i = 0; i < 676; i++)
    {
        if (tvc.getVoteCount(i) != data_u32[i])
        {
            isMatched = false;
            printf("[FAILED] comp %d: %llu vs %lu\n", i, tvc.getVoteCount(i), data_u32[i]);
            break;
        }
    }
    EXPECT_TRUE(isMatched);
}

bool tick_data[676 * 4][676];
TEST(TestCoreVoteCounter, RegisterNewCompressVotes) {
    unsigned char data_u10[848] = { 0 };
    unsigned int data_u32[676] = { 0 };    
    setMem(tick_data, sizeof(tick_data), 0);
    srand(0);
    tvc.init();
    bool isMatched = true;
    unsigned int startTick = 676 + rand() % 676;
    for (unsigned int tick = startTick; tick < 676 * 4; tick++)
    {
        bool flag[676];
        setMem(flag, sizeof(flag), true);
        int n_non_vote = rand() % (676 - 451);
        for (int i = 0; i < n_non_vote; i++)
        {
            flag[rand() % 676] = false;
        }
        for (int i = 0; i < 676; i++)
        {
            if (flag[i])
            {
                tvc.registerNewVote(tick, i);
                tick_data[tick][i] = true;
            }
        }
        for (int k = 0; k < 2; k++) // randomly select 2 comp to test
        {
            int comp = rand() % 676;
            tvc.compressNewVotesPacket(tick - 675, tick + 1, comp, data_u10);
            setMem(data_u32, sizeof(data_u32), 0);
            for (unsigned int t = tick - 675; t <= tick; t++)
            {
                for (int i = 0; i < 676; i++)
                {
                    if (tick_data[t][i]) data_u32[i]++;
                }
            }
            data_u32[comp] = 0;
            for (int i = 0; i < 676; i++)
            {
                unsigned int vote = tvc.testExtract10Bit(data_u10, i);
                if (vote != data_u32[i])
                {
                    isMatched = false;
                    break;
                }
            }
        }
        EXPECT_TRUE(isMatched);
        //printf("[PASSED] tick %u\n", tick);
    }
}