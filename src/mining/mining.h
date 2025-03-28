#include "platform/memory.h"
#include "network_messages/transactions.h"

struct MiningSolutionTransaction : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return 2; // TODO: Set actual value
    }

    static constexpr long long minAmount()
    {
        return SOLUTION_SECURITY_DEPOSIT;
    }

    static constexpr unsigned short minInputSize()
    {
        return sizeof(miningSeed) + sizeof(nonce);
    }

    m256i miningSeed;
    m256i nonce;
    unsigned char signature[SIGNATURE_SIZE];
};

constexpr int CUSTOM_MINING_SHARE_COUNTER_INPUT_TYPE = 8;
constexpr int TICK_CUSTOM_MINING_SHARE_COUNTER_PUBLICATION_OFFSET = 4;

struct CustomMiningSolutionTransaction : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return CUSTOM_MINING_SHARE_COUNTER_INPUT_TYPE;
    }

    static constexpr long long minAmount()
    {
        return 0;
    }

    static constexpr unsigned short minInputSize()
    {
        return sizeof(miningSeed) + sizeof(nonce);
    }

    m256i miningSeed;
    m256i nonce;
    unsigned char signature[SIGNATURE_SIZE];
};

struct CustomMiningTask
{
    unsigned long long taskIndex; // ever increasing number (unix timestamp in ms)

    unsigned char m_blob[408]; // Job data from pool
    unsigned long long m_size;  // length of the blob
    unsigned long long m_target; // Pool difficulty
    unsigned long long m_height; // Block height
    unsigned char m_seed[32]; // Seed hash for XMR
    unsigned int m_extraNonce;
};

struct CustomMiningSolution
{
    unsigned long long taskIndex; // should match the index from task
    unsigned int nonce;         // xmrig::JobResult.nonce
    m256i result;               // xmrig::JobResult.result, 32 bytes
};

#define CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES 848
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
static_assert((1 << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP, "Invalid data size");
static char accumulatedSharedCountLock = 0;

class CustomMiningSharesCounter
{
private:
    unsigned int _shareCount[NUMBER_OF_COMPUTORS];
    unsigned long long _accumulatedSharesCount[NUMBER_OF_COMPUTORS];
    unsigned int buffer[NUMBER_OF_COMPUTORS];
protected:
    unsigned int extract10Bit(const unsigned char* data, unsigned int idx)
    {
        //TODO: simplify this
        unsigned int byte0 = data[idx + (idx >> 2)];
        unsigned int byte1 = data[idx + (idx >> 2) + 1];
        unsigned int last_bit0 = 8 - (idx & 3) * 2;
        unsigned int first_bit1 = 10 - last_bit0;
        unsigned int res = (byte0 & ((1 << last_bit0) - 1)) << first_bit1;
        res |= (byte1 >> (8 - first_bit1));
        return res;
    }
    void update10Bit(unsigned char* data, unsigned int idx, unsigned int value)
    {
        //TODO: simplify this
        unsigned char& byte0 = data[idx + (idx >> 2)];
        unsigned char& byte1 = data[idx + (idx >> 2) + 1];
        unsigned int last_bit0 = 8 - (idx & 3) * 2;
        unsigned int first_bit1 = 10 - last_bit0;
        unsigned char mask0 = ~((1 << last_bit0) - 1);
        unsigned char mask1 = ((1 << (8 - first_bit1)) - 1);
        byte0 &= mask0;
        byte1 &= mask1;
        unsigned char ubyte0 = (value >> first_bit1);
        unsigned char ubyte1 = (value & ((1 << first_bit1) - 1)) << (8 - first_bit1);
        byte0 |= ubyte0;
        byte1 |= ubyte1;
    }

    void accumulateSharesCount(unsigned int computorIdx, unsigned int value)
    {
        _accumulatedSharesCount[computorIdx] += value;
    }

public:
    static constexpr unsigned int CustomMiningSolutionCounterDataSize = sizeof(_shareCount) + sizeof(_accumulatedSharesCount);
    void init()
    {
        setMem(_shareCount, sizeof(_shareCount), 0);
        setMem(_accumulatedSharesCount, sizeof(_accumulatedSharesCount), 0);
    }

    void registerNewShareCount(const unsigned int* sharesCount)
    {
        copyMem(_shareCount, sharesCount, sizeof(_shareCount));
    }

    // get and compress number of shares of 676 computors to 676x10 bit numbers
    void compressNewSharesPacket(unsigned int ownComputorIdx, unsigned char customMiningShareCountPacket[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES])
    {
        setMem(customMiningShareCountPacket, sizeof(customMiningShareCountPacket), 0);
        setMem(buffer, sizeof(buffer), 0);
        for (int j = 0; j < NUMBER_OF_COMPUTORS; j++)
        {
            buffer[j] = _shareCount[j];
        }

        buffer[ownComputorIdx] = 0; // remove self-report
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            update10Bit(customMiningShareCountPacket, i, buffer[i]);
        }
    }

    bool validateNewSharesPacket(const unsigned char* customMiningShareCountPacket, unsigned int computorIdx)
    {
        //unsigned long long sum = 0;
        //for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        //{
        //    buffer[i] = extract10Bit(customMiningShareCountPacket, i);
        //    if (buffer[i] > NUMBER_OF_COMPUTORS)
        //    {
        //        return false;
        //    }
        //    sum += buffer[i];
        //}
        //// check #0: sum of all vote must be >= 675*451 (vote of the tick leader is removed)
        //if (sum < (NUMBER_OF_COMPUTORS - 1) * QUORUM)
        //{
        //    return false;
        //}
        // check #1: own number of vote must be zero
        if (buffer[computorIdx] != 0)
        {
            return false;
        }
        return true;
    }

    void addShares(const unsigned char* newSharePacket, unsigned int computorIdx)
    {
        if (validateNewSharesPacket(newSharePacket, computorIdx))
        {
            for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                unsigned int shareCount = extract10Bit(newSharePacket, i);
                accumulateSharesCount(i, shareCount);
            }
        }
    }

    unsigned long long getSharesCount(unsigned int computorIdx)
    {
        return _accumulatedSharesCount[computorIdx];
    }

    void saveAllDataToArray(unsigned char* dst)
    {
        copyMem(dst, &_shareCount[0], sizeof(_shareCount));
        copyMem(dst + sizeof(_shareCount), &_accumulatedSharesCount[0], sizeof(_accumulatedSharesCount));
    }

    void loadAllDataFromArray(const unsigned char* src)
    {
        copyMem(&_shareCount[0], src, sizeof(_shareCount));
        copyMem(&_accumulatedSharesCount[0], src + sizeof(_shareCount), sizeof(_accumulatedSharesCount));
    }
};
