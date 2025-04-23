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
static constexpr int CUSTOM_MINING_SOLUTION_SHARES_COUNT_MAX_VAL = (1U << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) - 1;
static constexpr unsigned long long MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS = (CUSTOM_MINING_SOLUTION_SHARES_COUNT_MAX_VAL * NUMBER_OF_COMPUTORS);

static_assert((1 << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP, "Invalid data size");
volatile static char accumulatedSharedCountLock = 0;
volatile static char gSystemCustomMiningSolutionLock = 0;
volatile static char gCustomMiningCacheLock = 0;
unsigned long long gSystemCustomMiningSolutionCount = 0;
unsigned long long gSystemCustomMiningDuplicatedSolutionCount = 0;
unsigned long long gSystemCustomMiningSolutionOFCount = 0;

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

/// Cache storing scores for custom mining data (hash map)
static constexpr int CUSTOM_MINING_CACHE_MISS = -1;
static constexpr int CUSTOM_MINING_CACHE_COLLISION = -2;
static constexpr int CUSTOM_MINING_CACHE_HIT = -3;

template <typename T, unsigned int size, unsigned int collisionRetries = 20>
class CustomMininingCache
{
    static_assert(collisionRetries < size, "Number of fetch retries in case of collision is too big!");
public:

    /// Reset all cache entries
    void reset()
    {
        ACQUIRE(lock);
        setMem((unsigned char*)cache, sizeof(cache), 0);
        hits = 0;
        misses = 0;
        collisions = 0;
        RELEASE(lock);
    }

    /// Return maximum number of entries that can be stored in cache
    constexpr unsigned int capacity() const
    {
        return size;
    }

    /// Get cache index based on hash function
    unsigned int getCacheIndex(const T& rData)
    {
        return rData.getHashIndex() % capacity();
    }

    // Try to fetch data from cacheIndex, also checking a few following entries in case of collisions (may update cacheIndex),
    // increments counter of hits, misses, or collisions
    int tryFetching(T& rData, unsigned int& cacheIndex)
    {
        int retVal;
        unsigned int tryFetchIdx = rData.getHashIndex() % capacity();
        ACQUIRE(lock);
        for (unsigned int i = 0; i < collisionRetries; ++i)
        {
            const T& cacheData = cache[tryFetchIdx];
            if (cacheData.isEmpty())
            {
                // miss: data not available in cache yet (entry is empty)
                misses++;
                retVal = CUSTOM_MINING_CACHE_MISS;
                break;
            }

            if (cacheData.isMatched(rData))
            {
                // hit: data available in cache -> return score
                hits++;
                retVal = CUSTOM_MINING_CACHE_HIT;
                break;
            }

            // collision: other data is mapped to same index -> retry at following index
            retVal = CUSTOM_MINING_CACHE_COLLISION;
            tryFetchIdx = (tryFetchIdx + 1) % capacity();
        }
        RELEASE(lock);

        if (retVal ==  CUSTOM_MINING_CACHE_COLLISION)
        {
            ACQUIRE(lock);
            collisions++;
            RELEASE(lock);
        }
        else
        {
            cacheIndex = tryFetchIdx;
        }
        return retVal;
    }

    /// Add entry to cache (may overwrite existing entry)
    void addEntry(const T& rData, unsigned int cacheIndex)
    {
        cacheIndex %= capacity();
        ACQUIRE(lock);
        cache[cacheIndex] = rData;
        RELEASE(lock);
    }

    /// Save custom mining share cache to file
    void save(CHAR16* filename, CHAR16* directory = NULL)
    {
        logToConsole(L"Saving custom mining cache file...");

        const unsigned long long beginningTick = __rdtsc();
        ACQUIRE(lock);
        long long savedSize = ::save(filename, sizeof(cache), (unsigned char*)&cache, directory);
        RELEASE(lock);
        if (savedSize == sizeof(cache))
        {
            setNumber(message, savedSize, TRUE);
            appendText(message, L" bytes of the custom mining cache data are saved (");
            appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
            appendText(message, L" microseconds).");
            logToConsole(message);
        }
    }

    /// Try to load custom mining share cache file
    bool load(CHAR16* filename, CHAR16* directory = NULL)
    {
        bool success = true;
        logToConsole(L"Loading custom mining cache...");
        reset();
        ACQUIRE(lock);
        long long loadedSize = ::load(filename, sizeof(cache), (unsigned char*)cache, directory);
        RELEASE(lock);
        if (loadedSize != sizeof(cache))
        {
            if (loadedSize == -1)
            {
                logToConsole(L"Error while loading custom mining cache: File does not exists (ignore this error if this is the epoch start)");
            }
            else if (loadedSize < sizeof(cache))
            {
                logToConsole(L"Error while loading custom mining cache: Custom mining cache file is smaller than defined. System may not work properly");
            }
            else
            {
                logToConsole(L"Error while loading custom mining cache: Custom mining cache file is larger than defined. System may not work properly");
            }
            success = false;
        }
        else
        {
            logToConsole(L"Loaded score cache data!");
        }
        return success;
    }

    // Return number of hits (data available in cache when fetched)
    unsigned int hitCount() const
    {
        return hits;
    }

    // Return number of misses (data not in cache yet)
    unsigned int missCount() const
    {
        return misses;
    }

    // Return number of collisions (other data is mapped to same index)
    unsigned int collisionCount() const
    {
        return collisions;
    }

private:

    // cache entries (set zero or load from a file on init)
    T cache[size];

    // lock to prevent race conditions on parallel access
    volatile char lock = 0;

    // statistics of hits, misses, and collisions
    unsigned int hits = 0;
    unsigned int misses = 0;
    unsigned int collisions = 0;
};

class CustomMiningSolutionCacheEntry
{
public:
    void reset()
    {
        _taskIndex = 0;
        _isHashed = false;
    }

    void set(const CustomMiningSolution* pCustomMiningSolution)
    {
        reset();
        _taskIndex = pCustomMiningSolution->taskIndex;
        _nonce = pCustomMiningSolution->nonce;
        _padding = pCustomMiningSolution->padding;
    }

    void get(CustomMiningSolution& rCustomMiningSolution)
    {
        rCustomMiningSolution.nonce = _nonce;
        rCustomMiningSolution.taskIndex = _taskIndex;
        rCustomMiningSolution.padding = _padding;
    }

    bool isEmpty() const
    {
        return (_taskIndex == 0);
    }
    bool isMatched(const CustomMiningSolutionCacheEntry& rOther) const
    {
        return (_taskIndex == rOther._taskIndex) && (_nonce == rOther._nonce);
    }
    unsigned long long getHashIndex()
    {
        if (!_isHashed)
        {
            copyMem(_buffer, &_taskIndex, sizeof(_taskIndex));
            copyMem(_buffer + sizeof(_taskIndex), &_nonce, sizeof(_nonce));
            KangarooTwelve(_buffer, sizeof(_taskIndex) + sizeof(_nonce), &_digest, sizeof(_digest));
            _isHashed = true;
        }
        return _digest;
    }
private:
    unsigned long long _taskIndex;
    unsigned int _nonce;
    unsigned int _padding; // currently unused
    unsigned long long _digest;
    unsigned char _buffer[sizeof(_taskIndex) + sizeof(_nonce)];
    bool _isHashed;
};

CustomMininingCache<CustomMiningSolutionCacheEntry, MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS, 20> gSystemCustomMiningSolution;

// Save score cache to SCORE_CACHE_FILE_NAME
void saveCustomMiningCache(int epoch, CHAR16* directory = NULL)
{
    ACQUIRE(gCustomMiningCacheLock);
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
    gSystemCustomMiningSolution.save(CUSTOM_MINING_CACHE_FILE_NAME, directory);
    RELEASE(gCustomMiningCacheLock);
}

// Update score cache filename with epoch and try to load file
bool loadCustomMiningCache(int epoch)
{
    bool success = true;
    ACQUIRE(gCustomMiningCacheLock);
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
    success = gSystemCustomMiningSolution.load(CUSTOM_MINING_CACHE_FILE_NAME);
    RELEASE(gCustomMiningCacheLock);
    return success;
}

