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
};

struct CustomMiningTask
{
    unsigned long long taskIndex; // ever increasing number (unix timestamp in ms)

    unsigned char blob[408]; // Job data from pool
    unsigned long long size;  // length of the blob
    unsigned long long target; // Pool difficulty
    unsigned long long height; // Block height
    unsigned char seed[32]; // Seed hash for XMR
    unsigned int extraNonce;
};

struct CustomMiningSolution
{
    unsigned long long taskIndex; // should match the index from task
    unsigned int nonce;         // xmrig::JobResult.nonce
    unsigned int padding;
    m256i result;               // xmrig::JobResult.result, 32 bytes
};


#define CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES 848
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
static_assert((1 << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP, "Invalid data size");
static char accumulatedSharedCountLock = 0;

struct CustomMiningSharePayload
{
    Transaction transaction;
    unsigned char packedScore[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES];
    unsigned char signature[SIGNATURE_SIZE];
};

struct BroadcastCustomMiningTransaction
{
    CustomMiningSharePayload payload;
    bool isBroadcasted;
};

BroadcastCustomMiningTransaction gCustomMiningBroadcastTxBuffer[NUMBER_OF_COMPUTORS];

class CustomMiningSharesCounter
{
private:
    unsigned int _shareCount[NUMBER_OF_COMPUTORS];
    unsigned long long _accumulatedSharesCount[NUMBER_OF_COMPUTORS];
    unsigned int _buffer[NUMBER_OF_COMPUTORS];
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
    static constexpr unsigned int _customMiningSolutionCounterDataSize = sizeof(_shareCount) + sizeof(_accumulatedSharesCount);
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
        setMem(_buffer, sizeof(_buffer), 0);
        for (int j = 0; j < NUMBER_OF_COMPUTORS; j++)
        {
            _buffer[j] = _shareCount[j];
        }

        _buffer[ownComputorIdx] = 0; // remove self-report
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            update10Bit(customMiningShareCountPacket, i, _buffer[i]);
        }
    }

    bool validateNewSharesPacket(const unsigned char* customMiningShareCountPacket, unsigned int computorIdx)
    {
        // check #1: own number of share must be zero
        if (extract10Bit(customMiningShareCountPacket, computorIdx) != 0)
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

void initCustomMining()
{
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        // Initialize the broadcast transaction buffer. Assume the all previous is broadcasted.
        gCustomMiningBroadcastTxBuffer[i].isBroadcasted = true;
    }
}

// Compute revenue of computors without donation
void computeRev(
    const unsigned long long* revenueScore,
    unsigned long long* rev)
{
    // Sort revenue scores to get lowest score of quorum
    unsigned long long sortedRevenueScore[QUORUM + 1];
    bs->SetMem(sortedRevenueScore, sizeof(sortedRevenueScore), 0);
    for (unsigned short computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        sortedRevenueScore[QUORUM] = revenueScore[computorIndex];
        unsigned int i = QUORUM;
        while (i
            && sortedRevenueScore[i - 1] < sortedRevenueScore[i])
        {
            const unsigned long long tmp = sortedRevenueScore[i - 1];
            sortedRevenueScore[i - 1] = sortedRevenueScore[i];
            sortedRevenueScore[i--] = tmp;
        }
    }
    if (!sortedRevenueScore[QUORUM - 1])
    {
        sortedRevenueScore[QUORUM - 1] = 1;
    }

    // Compute revenue of computors and arbitrator
    long long arbitratorRevenue = ISSUANCE_RATE;
    constexpr long long issuancePerComputor = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;
    constexpr long long scalingThreshold = 0xFFFFFFFFFFFFFFFFULL / issuancePerComputor;
    static_assert(MAX_NUMBER_OF_TICKS_PER_EPOCH <= 605020, "Redefine scalingFactor");
    // maxRevenueScore for 605020 ticks = ((7099 * 605020) / 676) * 605020 * 675
    constexpr unsigned scalingFactor = 208100; // >= (maxRevenueScore600kTicks / 0xFFFFFFFFFFFFFFFFULL) * issuancePerComputor =(approx)= 208078.5
    for (unsigned int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        // Compute initial computor revenue, reducing arbitrator revenue
        long long revenue;
        if (revenueScore[computorIndex] >= sortedRevenueScore[QUORUM - 1])
            revenue = issuancePerComputor;
        else
        {
            if (revenueScore[computorIndex] > scalingThreshold)
            {
                // scale down to prevent overflow, then scale back up after division
                unsigned long long scaledRev = revenueScore[computorIndex] / scalingFactor;
                revenue = ((issuancePerComputor * scaledRev) / sortedRevenueScore[QUORUM - 1]);
                revenue *= scalingFactor;
            }
            else
            {
                revenue = ((issuancePerComputor * ((unsigned long long)revenueScore[computorIndex])) / sortedRevenueScore[QUORUM - 1]);
            }
        }
        rev[computorIndex] = revenue;
    }
}

static unsigned long long customMiningScoreBuffer[NUMBER_OF_COMPUTORS];
void computeRevWithCustomMining(
    const unsigned long long* oldScore,
    const unsigned long long* customMiningSharesCount,
    unsigned long long* oldRev,
    unsigned long long* customMiningRev)
{
    // Old score
    computeRev(oldScore, oldRev);

    // Revenue of custom mining shares combination
    // Formula: newScore =  vote_count * tx * customMiningShare = revenueOldScore * customMiningShare
    for (unsigned short computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        customMiningScoreBuffer[computorIndex] = oldScore[computorIndex] * customMiningSharesCount[computorIndex];
    }
    computeRev(customMiningScoreBuffer, customMiningRev);
}
