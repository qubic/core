#pragma once

#include "platform/assert.h"
#include "platform/concurrency.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
#include "network_messages/custom_mining.h"
#include "network_messages/transactions.h"
#include "kangaroo_twelve.h"

#include <lib/platform_efi/uefi.h>

static unsigned int getTickInMiningPhaseCycle()
{
#ifdef NO_UEFI
    return 0;
#else
    return (system.tick) % (INTERNAL_COMPUTATIONS_INTERVAL + EXTERNAL_COMPUTATIONS_INTERVAL);
#endif
}

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
    unsigned long long taskIndex;       // ever increasing number (unix timestamp in ms)
    unsigned short firstComputorIndex;  // the first computor index assigned by this task
    unsigned short lastComputorIndex;   // the last computor index assigned by this task
    unsigned int padding;

    unsigned char blob[408]; // Job data from pool
    unsigned long long size;  // length of the blob
    unsigned long long target; // Pool difficulty
    unsigned long long height; // Block height
    unsigned char seed[32]; // Seed hash for XMR
};

struct CustomMiningTaskV2 {
    unsigned long long taskIndex;
    unsigned char m_template[896];
    unsigned long long m_extraNonceOffset;
    unsigned long long m_size;
    unsigned long long m_target;
    unsigned long long m_height;
    unsigned char m_seed[32];
};

struct CustomMiningSolution
{
    unsigned long long taskIndex;       // should match the index from task
    unsigned short firstComputorIndex;  // should match the index from task
    unsigned short lastComputorIndex;   // should match the index from task
    unsigned int nonce;                 // xmrig::JobResult.nonce
    m256i result;                       // xmrig::JobResult.result, 32 bytes
};

struct CustomMiningSolutionV2 {
    unsigned long long taskIndex;
    unsigned long long nonce; // (extraNonce<<32) | nonce
    unsigned long long reserve0;
    unsigned long long reserve1;
    unsigned long long reserve2;
    m256i result;
};

#define CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES 848
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
#define TICK_VOTE_COUNTER_PUBLICATION_OFFSET 2 // Must be 2
static constexpr int CUSTOM_MINING_SOLUTION_SHARES_COUNT_MAX_VAL = (1U << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) - 1;
static_assert((1 << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP, "Invalid data size");

struct CustomMiningSharePayload
{
    Transaction transaction;
    unsigned char packedScore[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES];
    m256i dataLock;
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

    bool isEmptyPacket(const unsigned char* data) const
    {
        for (int i = 0; i < CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES; i++)
        {
            if (data[i] != 0)
            {
                return false;
            }
        }
        return true;
    }

    // get and compress number of shares of 676 computors to 676x10 bit numbers
    void compressNewSharesPacket(unsigned int ownComputorIdx, unsigned char customMiningShareCountPacket[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES])
    {
        setMem(customMiningShareCountPacket, CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES, 0);
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

    void processTransactionData(const Transaction* transaction, const m256i& dataLock)
    {
#ifndef NO_UEFI
        int computorIndex = transaction->tick % NUMBER_OF_COMPUTORS;
        int tickPhase = getTickInMiningPhaseCycle();
        if (transaction->sourcePublicKey == broadcastedComputors.computors.publicKeys[computorIndex] // this tx was sent by the tick leader of this tick
            && transaction->inputSize == CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES + sizeof(m256i)
            && tickPhase <= NUMBER_OF_COMPUTORS + TICK_VOTE_COUNTER_PUBLICATION_OFFSET) // only accept tick within internal mining phase (+ 2 from broadcast time)
        {
            if (!transaction->amount)
            {
                m256i txDataLock = m256i(transaction->inputPtr() + CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES);
                if (txDataLock == dataLock)
                {
                    addShares(transaction->inputPtr(), computorIndex);
                }
#ifndef NDEBUG
                else
                {
                    CHAR16 dbg[256];
                    setText(dbg, L"TRACE: [Custom mining point tx] Wrong datalock from comp ");
                    appendNumber(dbg, computorIndex, false);
                    addDebugMessage(dbg);
                }
#endif
            }
        }
#endif
    }
};

/// Cache storing scores for custom mining data (hash map)
static constexpr int CUSTOM_MINING_CACHE_MISS = -1;
static constexpr int CUSTOM_MINING_CACHE_COLLISION = -2;
static constexpr int CUSTOM_MINING_CACHE_HIT = -3;

template <typename T, unsigned int size, unsigned int collisionRetries = 20>
class CustomMininingCache
{
    static_assert(collisionRetries < size, "Number of fetch retries in case of collision is too big!");
public:

    void init()
    {
        setMem((unsigned char*)cache, sizeof(cache), 0);
        lock = 0;
        hits = 0;
        misses = 0;
        collisions = 0;
        verification = 0;
        invalid = 0;
    }

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

    /// Get entry from cache
    void getEntry(T& rData, unsigned int cacheIndex)
    {
        cacheIndex %= capacity();
        ACQUIRE(lock);
        rData = cache[cacheIndex];
        RELEASE(lock);
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

    // Try to fetch data from cacheIndex, also checking a few following entries in case of collisions (may update cacheIndex),
    // increments counter of hits, misses, or collisions
    int tryFetchingAndUpdate(T& rData, int updateCondition)
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
        if (retVal == updateCondition)
        {
            cache[tryFetchIdx] = rData;
        }
        RELEASE(lock);

        if (retVal == CUSTOM_MINING_CACHE_COLLISION)
        {
            ACQUIRE(lock);
            collisions++;
            RELEASE(lock);
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

#ifdef NO_UEFI
#else
    /// Save custom mining share cache to file
    void save(CHAR16* filename, CHAR16* directory = NULL)
    {
        const unsigned long long beginningTick = __rdtsc();
        ACQUIRE(lock);
        long long savedSize = ::save(filename, sizeof(cache), (unsigned char*)cache, directory);
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
#endif

    // Return number of hits (data available in cache when fetched)
    unsigned int hitCount()
    {
        unsigned int result = 0;
        ACQUIRE(lock);
        result = hits;
        RELEASE(lock);
        return result;
    }

    // Return number of misses (data not in cache yet)
    unsigned int missCount()
    {
        unsigned int result = 0;
        ACQUIRE(lock);
        result = misses;
        RELEASE(lock);
        return result;
    }

    // Return number of collisions (other data is mapped to same index)
    unsigned int collisionCount()
    {
        unsigned int result = 0;
        ACQUIRE(lock);
        result = collisions;
        RELEASE(lock);
        return result;
    }

private:

    // cache entries (set zero or load from a file on init)
    T cache[size];

    // lock to prevent race conditions on parallel access
    volatile char lock;

    // statistics of hits, misses, and collisions
    unsigned int hits;
    unsigned int misses;
    unsigned int collisions;

    // statistics of verification and invalid count
    unsigned int verification;
    unsigned int invalid;
};

class CustomMiningSolutionCacheEntry
{
public:
    void reset()
    {
        _solution.taskIndex = 0;
        _isHashed = false;
        _isVerification = false;
        _isValid = true;
    }

    void set(const CustomMiningSolution* pCustomMiningSolution)
    {
        reset();
        _solution = *pCustomMiningSolution;
    }

    void set(const unsigned long long taskIndex, unsigned int nonce, unsigned short firstComputorIndex, unsigned short lastComputorIndex)
    {
        reset();
        _solution.taskIndex = taskIndex;
        _solution.nonce = nonce;
        _solution.firstComputorIndex = firstComputorIndex;
        _solution.lastComputorIndex = lastComputorIndex;
    }

    void get(CustomMiningSolution& rCustomMiningSolution)
    {
        rCustomMiningSolution = _solution;
    }

    bool isEmpty() const
    {
        return (_solution.taskIndex == 0);
    }
    bool isMatched(const CustomMiningSolutionCacheEntry& rOther) const
    {
        return (_solution.taskIndex == rOther.getTaskIndex()) && (_solution.nonce == rOther.getNonce());
    }
    unsigned long long getHashIndex()
    {
        // TODO: reserve each computor ID a limited slot.
        // This will avoid them spawning invalid solutions without verification
        if (!_isHashed)
        {
            copyMem(_buffer, &_solution.taskIndex, sizeof(_solution.taskIndex));
            copyMem(_buffer + sizeof(_solution.taskIndex), &_solution.nonce, sizeof(_solution.nonce));
            KangarooTwelve(_buffer, sizeof(_solution.taskIndex) + sizeof(_solution.nonce), &_digest, sizeof(_digest));
            _isHashed = true;
        }
        return _digest;
    }

    unsigned long long getTaskIndex() const
    {
        return _solution.taskIndex;
    }

    unsigned long long getNonce() const
    {
        return _solution.nonce;
    }

    bool isValid()
    {
        return _isValid;
    }

    bool isVerified()
    {
        return _isVerification;
    }

    void setValid(bool val)
    {
        _isValid = val;
    }

    void setVerified(bool val)
    {
        _isVerification = true;
    }

    void setEmpty()
    {
        _solution.taskIndex = 0;
    }

private:
    CustomMiningSolution _solution;
    unsigned long long _digest;
    unsigned char _buffer[sizeof(_solution.taskIndex) + sizeof(_solution.nonce)];
    bool _isHashed;
    bool _isVerification;
    bool _isValid;
};

class CustomMiningSolutionV2CacheEntry
{
public:
    void reset()
    {
        _solution.taskIndex = 0;
        _isHashed = false;
        _isVerification = false;
        _isValid = true;
    }

    void set(const CustomMiningSolutionV2* pCustomMiningSolution)
    {
        reset();
        _solution = *pCustomMiningSolution;
    }

    void get(CustomMiningSolutionV2& rCustomMiningSolution)
    {
        rCustomMiningSolution = _solution;
    }

    bool isEmpty() const
    {
        return (_solution.taskIndex == 0);
    }
    bool isMatched(const CustomMiningSolutionV2CacheEntry& rOther) const
    {
        return (_solution.taskIndex == rOther.getTaskIndex()) && (_solution.nonce == rOther.getNonce());
    }
    unsigned long long getHashIndex()
    {
        // TODO: reserve each computor ID a limited slot.
        // This will avoid them spawning invalid solutions without verification
        if (!_isHashed)
        {
            copyMem(_buffer, &_solution.taskIndex, sizeof(_solution.taskIndex));
            copyMem(_buffer + sizeof(_solution.taskIndex), &_solution.nonce, sizeof(_solution.nonce));
            KangarooTwelve(_buffer, sizeof(_solution.taskIndex) + sizeof(_solution.nonce), &_digest, sizeof(_digest));
            _isHashed = true;
        }
        return _digest;
    }

    unsigned long long getTaskIndex() const
    {
        return _solution.taskIndex;
    }

    unsigned long long getNonce() const
    {
        return _solution.nonce;
    }

    bool isValid()
    {
        return _isValid;
    }

    bool isVerified()
    {
        return _isVerification;
    }

    void setValid(bool val)
    {
        _isValid = val;
    }

    void setVerified(bool val)
    {
        _isVerification = true;
    }

    void setEmpty()
    {
        _solution.taskIndex = 0;
    }

private:
    CustomMiningSolutionV2 _solution;
    unsigned long long _digest;
    unsigned char _buffer[sizeof(_solution.taskIndex) + sizeof(_solution.nonce)];
    bool _isHashed;
    bool _isVerification;
    bool _isValid;
};


// In charge of storing custom mining
constexpr unsigned int NUMBER_OF_TASK_PARTITIONS = 4;
constexpr unsigned long long MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS = (200ULL << 20) / NUMBER_OF_TASK_PARTITIONS / sizeof(CustomMiningSolutionCacheEntry);
constexpr unsigned long long CUSTOM_MINING_INVALID_INDEX = 0xFFFFFFFFFFFFFFFFULL;
constexpr unsigned long long CUSTOM_MINING_TASK_STORAGE_COUNT = 60 * 60 * 24 * 8 / 2 / 10; // All epoch tasks in 7 (+1) days, 10s per task, idle phases only
constexpr unsigned long long CUSTOM_MINING_TASK_STORAGE_SIZE = CUSTOM_MINING_TASK_STORAGE_COUNT * sizeof(CustomMiningTask); // ~16.6MB
constexpr unsigned long long CUSTOM_MINING_SOLUTION_STORAGE_COUNT = MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS;
constexpr unsigned long long CUSTOM_MINING_STORAGE_PROCESSOR_MAX_STORAGE = 10 * 1024 * 1024; // 10MB
constexpr unsigned long long CUSTOM_MINING_RESPOND_MESSAGE_MAX_SIZE = 1 * 1024 * 1024; // 1MB

static volatile char gCustomMiningSharesCountLock = 0;
static char gIsInCustomMiningState = 0;
static volatile char gIsInCustomMiningStateLock = 0;
static volatile char gCustomMiningTaskStorageLock = 0;
static volatile char gCustomMiningSolutionStorageLock = 0;

// Stats of custom mining.
// Reset after epoch change.
// Some variable is reset after end of each custom mining phase.
struct CustomMiningStats
{
    struct Counter
    {
        long long tasks;        // Task that replaced by node

        // Shares related
        long long shares;       // Total shares/solutions = unverifed/no-task + valid + invalid
        long long valid;        // Solutions are marked as valid by verififer
        long long invalid;      // Solutions are marked as invalid by verififer
        long long duplicated;   // Duplicated solutions, ussually causes by solution message broadcasted or

        void reset()
        {
            ATOMIC_STORE64(tasks, 0);

            ATOMIC_STORE64(shares, 0);
            ATOMIC_STORE64(valid, 0);
            ATOMIC_STORE64(invalid, 0);
            ATOMIC_STORE64(duplicated, 0);
        }
    };

    // Stats of current epoch until last custom mining phase end
    Counter lastPhases;              // Stats of all last/previous phases
    long long maxOverflowShareCount; // Max number of shares that exceed the data packed in transaction
    long long maxCollisionShareCount; // Max number of shares that are not save in cached because of collision

    // Stats of current custom mining phase
    Counter phase[NUMBER_OF_TASK_PARTITIONS];
    Counter phaseV2;

    // Asume at begining of epoch.
    void epochReset()
    {
        lastPhases.reset();
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
        {
            phase[i].reset();
        }

        phaseV2.reset();
        ATOMIC_STORE64(maxOverflowShareCount, 0);
        ATOMIC_STORE64(maxCollisionShareCount, 0);
    }

    // At the end of phase. Ussually the task/sols message still arrive
    void phaseResetAndEpochAccumulate()
    {
        // Load the phase stats
        long long tasks = 0;
        long long shares = 0;
        long long valid = 0;
        long long invalid = 0;
        long long duplicated = 0;
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
        {
            tasks += ATOMIC_LOAD64(phase[i].tasks);
            shares += ATOMIC_LOAD64(phase[i].shares);
            valid += ATOMIC_LOAD64(phase[i].valid);
            invalid += ATOMIC_LOAD64(phase[i].invalid);
            duplicated += ATOMIC_LOAD64(phase[i].duplicated);
        }

        // Accumulate the phase into last phases
        ATOMIC_ADD64(lastPhases.tasks, tasks);
        ATOMIC_ADD64(lastPhases.shares, shares);
        ATOMIC_ADD64(lastPhases.valid, valid);
        ATOMIC_ADD64(lastPhases.invalid, invalid);
        ATOMIC_ADD64(lastPhases.duplicated, duplicated);

        // Reset phase number
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
        {
            phase[i].reset();
        }
        phaseV2.reset();
    }

    void appendLog(CHAR16* message)
    {
        long long customMiningTasks = 0;
        long long customMiningShares = 0;
        long long customMiningValidShares = 0;
        long long customMiningInvalidShares = 0;
        long long customMiningDuplicated = 0;
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
        {
            customMiningTasks += ATOMIC_LOAD64(phase[i].tasks);
            customMiningShares += ATOMIC_LOAD64(phase[i].shares);
            customMiningValidShares += ATOMIC_LOAD64(phase[i].valid);
            customMiningInvalidShares += ATOMIC_LOAD64(phase[i].invalid);
            customMiningDuplicated += ATOMIC_LOAD64(phase[i].duplicated);
        }

        appendText(message, L"Phase:");
        appendText(message, L" Tasks: ");
        appendNumber(message, customMiningTasks, true);
        appendText(message, L" | Shares: ");
        appendNumber(message, customMiningShares, true);
        appendText(message, L" | Valid: ");
        appendNumber(message, customMiningValidShares, true);
        appendText(message, L" | InValid: ");
        appendNumber(message, customMiningInvalidShares, true);
        appendText(message, L" | Duplicated: ");
        appendNumber(message, customMiningDuplicated, true);

        appendText(message, L"Phase V2:");
        appendText(message, L" Tasks: ");
        appendNumber(message, phaseV2.tasks, true);
        appendText(message, L" | Shares: ");
        appendNumber(message, phaseV2.shares, true);
        appendText(message, L" | Valid: ");
        appendNumber(message, phaseV2.valid, true);
        appendText(message, L" | InValid: ");
        appendNumber(message, phaseV2.invalid, true);
        appendText(message, L" | Duplicated: ");
        appendNumber(message, phaseV2.duplicated, true);

        long long customMiningEpochTasks = customMiningTasks + ATOMIC_LOAD64(lastPhases.tasks);
        long long customMiningEpochShares = customMiningShares + ATOMIC_LOAD64(lastPhases.shares);
        long long customMiningEpochInvalidShares = customMiningInvalidShares + ATOMIC_LOAD64(lastPhases.invalid);
        long long customMiningEpochValidShares = customMiningValidShares + ATOMIC_LOAD64(lastPhases.valid);
        long long customMiningEpochDuplicated = customMiningDuplicated + ATOMIC_LOAD64(lastPhases.duplicated);

        appendText(message, L". Epoch (not count v2):");
        appendText(message, L" Tasks: ");
        appendNumber(message, customMiningEpochTasks, false);
        appendText(message, L" | Shares: ");
        appendNumber(message, customMiningEpochShares, false);
        appendText(message, L" | Valid: ");
        appendNumber(message, customMiningEpochValidShares, false);
        appendText(message, L" | Invalid: ");
        appendNumber(message, customMiningEpochInvalidShares, false);
        appendText(message, L" | Duplicated: ");
        appendNumber(message, customMiningEpochDuplicated, false);
    }
};


struct CustomMiningRespondDataHeader
{
    unsigned long long itemCount;       // size of the data
    unsigned long long itemSize;        // size of the data
    unsigned long long fromTimeStamp;   // start of the ts
    unsigned long long toTimeStamp;     // end of the ts
    unsigned long long respondType;     // message type
};

template <typename DataType, unsigned long long maxItems, bool allowDuplicated = false>
class CustomMiningSortedStorage
{
public:
    enum Status
    {
        OK = 0,
        DATA_EXISTED = 1,
        BUFFER_FULL = 2,
        UNKNOWN_ERROR = 3,
    };
    void init()
    {
        allocPoolWithErrorLog(L"CustomMiningSortedStorageData", maxItems * sizeof(DataType), (void**)&_data, __LINE__);
        allocPoolWithErrorLog(L"CustomMiningSortedStorageIndices", maxItems * sizeof(unsigned long long), (void**)&_indices, __LINE__);
        _storageIndex = 0;

        // Buffer allocation for each processors. It is limited to 10MB each
        for (unsigned int i = 0; i < MAX_NUMBER_OF_PROCESSORS; i++)
        {
            allocPoolWithErrorLog(L"CustomMiningSortedStorageProcBuffer", CUSTOM_MINING_STORAGE_PROCESSOR_MAX_STORAGE, (void**)&_dataBuffer[i], __LINE__);
        }
    }

    void deinit()
    {
        if (NULL != _data)
        {
            freePool(_data);
            _data = NULL;
        }
        if (NULL != _indices)
        {
            freePool(_indices);
            _indices = NULL;
        }

        for (unsigned int i = 0; i < MAX_NUMBER_OF_PROCESSORS; i++)
        {
            if (NULL != _dataBuffer[i])
            {
                freePool(_dataBuffer[i]);
            }
        }
    }

    void reset()
    {
        _storageIndex = 0;
    }

    // Binary search for taskIndex or closest greater-than task index
    unsigned long long searchTaskIndex(unsigned long long taskIndex, bool& exactMatch) const
    {
        unsigned long long left = 0, right = (_storageIndex > 0) ? _storageIndex - 1 : 0;
        unsigned long long result = CUSTOM_MINING_INVALID_INDEX;
        exactMatch = false;

        while (left <= right && left < _storageIndex)
        {
            unsigned long long mid = (left + right) / 2;
            unsigned long long midTaskIndex = _data[_indices[mid]].taskIndex;

            if (midTaskIndex == taskIndex)
            {
                exactMatch = true;
                result = mid;
                break;
            }
            else if (midTaskIndex < taskIndex)
            {
                left = mid + 1;
            }
            else
            {
                result = mid;
                if (mid == 0)
                {
                    break; // prevent underflow
                }
                right = mid - 1;
            }
        }

        // In case of exact match. Make sure get the most left data
        if (exactMatch)
        {
            while (result > 0)
            {
                if (taskIndex == _data[_indices[result - 1]].taskIndex)
                {
                    result--;
                }
                else
                {
                    break;
                }
            }
        }

        return result;
    }


    // Return the task whose task index >= taskIndex
    unsigned long long lookForTaskGE(unsigned long long taskIndex)
    {
        bool exactMatch = false;
        unsigned long long idx = searchTaskIndex(taskIndex, exactMatch);
        return idx;
    }

    // Return the task whose task index == taskIndex
    unsigned long long lookForTask(unsigned long long taskIndex)
    {
        bool exactMatch = false;
        unsigned long long idx = searchTaskIndex(taskIndex, exactMatch);
        if (exactMatch)
        {
            return idx;
        }
        return CUSTOM_MINING_INVALID_INDEX;
    }

    bool dataExisted(const DataType* pData)
    {
        bool exactMatch = false;
        searchTaskIndex(pData->taskIndex, exactMatch);
        return exactMatch;
    }

    // Add the task to the storage
    int addData(const DataType* pData)
    {
        // Don't added if the task already existed
        if (!allowDuplicated && dataExisted(pData))
        {
            return DATA_EXISTED;
        }

        // Reset the storage if the number of tasks exceeds the limit
        if (_storageIndex >= maxItems)
        {
            return BUFFER_FULL;
        }

        unsigned long long newIndex = _storageIndex;
        _data[newIndex] = *pData;

        unsigned long long insertPos = 0;
        unsigned long long left = 0, right = (_storageIndex > 0) ? _storageIndex - 1 : 0;

        while (left <= right && left < _storageIndex)
        {
            unsigned long long mid = (left + right) / 2;
            if (_data[_indices[mid]].taskIndex < pData->taskIndex)
            {
                left = mid + 1;
            }
            else
            {
                if (mid == 0) break;
                right = mid - 1;
            }
        }
        insertPos = left;

        // Shift indices right
        for (unsigned long long i = _storageIndex; i > insertPos; --i)
        {
            _indices[i] = _indices[i - 1];
        }

        _indices[insertPos] = newIndex;
        _storageIndex++;

        return OK;
    }

    // Get the data from index
    DataType* getDataByIndex(unsigned long long index)
    {
        if (index >= _storageIndex)
        {
            return NULL;
        }
        return &_data[_indices[index]];
    }

    // Get the total number of tasks
    unsigned long long getCount() const
    {
        return _storageIndex;
    }

    // Packed array of data to a serialized data
    unsigned char* getSerializedData(
        unsigned long long fromTimeStamp, 
        unsigned long long toTimeStamp,
        unsigned long long processorNumber)
    {
        unsigned char* pData = _dataBuffer[processorNumber];

        // 8 bytes for the item size
        CustomMiningRespondDataHeader* pHeader = (CustomMiningRespondDataHeader*)pData;

        // Init the header as an empty data
        pHeader->itemCount = 0;
        pHeader->itemSize = 0;
        pHeader->fromTimeStamp = CUSTOM_MINING_INVALID_INDEX;
        pHeader->toTimeStamp = CUSTOM_MINING_INVALID_INDEX;

        // Remainder of the data
        pData += sizeof(CustomMiningRespondDataHeader);

        // Fill the header/
        constexpr long long remainedSize = CUSTOM_MINING_STORAGE_PROCESSOR_MAX_STORAGE - sizeof(CustomMiningRespondDataHeader);
        constexpr long long maxReturnItems = remainedSize / sizeof(DataType);

        // Look for the first task index
        unsigned long long startIndex = 0;
        if (fromTimeStamp > 0)
        {
            startIndex = lookForTaskGE(fromTimeStamp);
        }

        if (startIndex == CUSTOM_MINING_INVALID_INDEX)
        {
            return NULL;
        }

        // Check for the range of task
        unsigned long long endIndex = 0;
        if (toTimeStamp < fromTimeStamp || toTimeStamp == 0) // Return all tasks
        {
            endIndex = _storageIndex;
        }
        else
        {
           endIndex = lookForTaskGE(toTimeStamp);
            if (endIndex == CUSTOM_MINING_INVALID_INDEX)
            {
                endIndex = _storageIndex;
            }
        }

        unsigned long long respondTaskCount = endIndex - startIndex;
        if (respondTaskCount == 0)
        {
            return NULL;
        }

        // Pack data into respond
        respondTaskCount = (maxReturnItems < respondTaskCount) ? maxReturnItems : respondTaskCount;
        for (unsigned long long i = 0; i < respondTaskCount; i++, pData += sizeof(DataType))
        {
            unsigned long long index = startIndex + i;
            DataType* pItemData = getDataByIndex(index);
            copyMem(pData, pItemData, sizeof(DataType));
        }

        pHeader->itemCount = respondTaskCount;
        pHeader->itemSize = sizeof(DataType);

        pHeader->fromTimeStamp = fromTimeStamp;
        pHeader->toTimeStamp = getDataByIndex(startIndex + respondTaskCount - 1)->taskIndex;

        // Return the pointer to the data
        return _dataBuffer[processorNumber];
    }

    // Packed array of data to a serialized data
    unsigned char* getSerializedData(
        unsigned long long timeStamp,
        unsigned long long processorNumber)
    {
        unsigned char* pData = _dataBuffer[processorNumber];

        // 8 bytes for the item size
        CustomMiningRespondDataHeader* pHeader = (CustomMiningRespondDataHeader*)pData;

        // Init the header as an empty data
        pHeader->itemCount = 0;
        pHeader->itemSize = 0;
        pHeader->fromTimeStamp = CUSTOM_MINING_INVALID_INDEX;
        pHeader->toTimeStamp = CUSTOM_MINING_INVALID_INDEX;

        // Remainder of the data
        pData += sizeof(CustomMiningRespondDataHeader);

        // Fill the header/
        constexpr long long remainedSize = CUSTOM_MINING_STORAGE_PROCESSOR_MAX_STORAGE - sizeof(CustomMiningRespondDataHeader);
        constexpr long long maxReturnItems = remainedSize / sizeof(DataType);

        // Look for the first task index
        unsigned long long startIndex = 0;
        if (timeStamp > 0)
        {
            startIndex = lookForTaskGE(timeStamp);
        }

        if (startIndex == CUSTOM_MINING_INVALID_INDEX)
        {
            return NULL;
        }

        // Check for the range of task
        unsigned long long respondTaskCount = 0;
        for (unsigned long long i = startIndex; i < _storageIndex; i++)
        {
            DataType* pItemData = getDataByIndex(i);
            if (pItemData->taskIndex == timeStamp)
            {
                copyMem(pData, pItemData, sizeof(DataType));
                pData += sizeof(DataType);
                respondTaskCount++;
            }
            else
            {
                break;
            }
        }

        if (respondTaskCount == 0)
        {
            return NULL;
        }

        // Pack data into respond
        pHeader->itemCount = respondTaskCount;
        pHeader->itemSize = sizeof(DataType);

        pHeader->fromTimeStamp = timeStamp;
        pHeader->toTimeStamp = timeStamp;

        // Return the pointer to the data
        return _dataBuffer[processorNumber];
    }


private:
    DataType* _data;
    unsigned long long* _indices;
    unsigned long long _storageIndex;

    unsigned char* _dataBuffer[MAX_NUMBER_OF_PROCESSORS];
};

struct CustomMiningSolutionStorageEntry
{
    unsigned long long taskIndex;
    unsigned long long nonce;
    unsigned long long cacheEntryIndex;
};

typedef CustomMiningSortedStorage<CustomMiningTask, CUSTOM_MINING_TASK_STORAGE_COUNT, false> CustomMiningTaskStorage;
typedef CustomMiningSortedStorage<CustomMiningSolutionStorageEntry, CUSTOM_MINING_SOLUTION_STORAGE_COUNT, true> CustomMiningSolutionStorage;

typedef CustomMiningSortedStorage<CustomMiningTaskV2, CUSTOM_MINING_TASK_STORAGE_COUNT, false> CustomMiningTaskV2Storage;

class CustomMiningStorage
{
public:
    void init()
    {
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; ++i)
        {
            _taskStorage[i].init();
            _solutionStorage[i].init();
        }
        _taskV2Storage.init();
        _solutionV2Storage.init();
        // Buffer allocation for each processors. It is limited to 10MB each
        for (unsigned int i = 0; i < MAX_NUMBER_OF_PROCESSORS; i++)
        {
            allocPoolWithErrorLog(L"CustomMiningStorageProcBuffer", CUSTOM_MINING_STORAGE_PROCESSOR_MAX_STORAGE, (void**)&_dataBuffer[i], __LINE__);
        }

        _last3TaskV2Indexes[0] = 0;
        _last3TaskV2Indexes[1] = 0;
        _last3TaskV2Indexes[2] = 0;
    }
    void deinit()
    {
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; ++i)
        {
            _taskStorage[i].deinit();
            _solutionStorage[i].deinit();
        }
        _taskV2Storage.deinit();
        _solutionV2Storage.deinit();
        for (unsigned int i = 0; i < MAX_NUMBER_OF_PROCESSORS; i++)
        {
            freePool(_dataBuffer[i]);
        }
    }

    void reset()
    {
        ACQUIRE(gCustomMiningTaskStorageLock);
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; ++i)
        {
            _taskStorage[i].reset();
        }
        _taskV2Storage.reset();
        RELEASE(gCustomMiningTaskStorageLock);

        ACQUIRE(gCustomMiningSolutionStorageLock);
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; ++i)
        {
            _solutionStorage[i].reset();
        }
        _solutionV2Storage.reset();
        RELEASE(gCustomMiningSolutionStorageLock);

    }

    unsigned char* getSerializedTaskData(
        unsigned long long fromTimeStamp,
        unsigned long long toTimeStamp,
        unsigned long long processorNumber)
    {
        unsigned long long remainedSize = CUSTOM_MINING_STORAGE_PROCESSOR_MAX_STORAGE;
        unsigned char* packedData = _dataBuffer[processorNumber];
        CustomMiningRespondDataHeader* packedHeader = (CustomMiningRespondDataHeader*)packedData;
        packedHeader->respondType = RespondCustomMiningData::taskType;
        packedHeader->itemSize = sizeof(CustomMiningTask);
        packedHeader->fromTimeStamp = fromTimeStamp;
        packedHeader->toTimeStamp = toTimeStamp;
        packedHeader->itemCount = 0;

        unsigned char* traverseData = packedData + sizeof(CustomMiningRespondDataHeader);
        for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
        {
            unsigned char* data = _taskStorage[i].getSerializedData(fromTimeStamp, toTimeStamp, processorNumber);
            if (data != NULL)
            {
                CustomMiningRespondDataHeader* customMiningInternalHeader = (CustomMiningRespondDataHeader*)data;
                ASSERT(packedHeader->itemSize == customMiningInternalHeader->itemSize);
                unsigned long long dataSize = customMiningInternalHeader->itemCount * sizeof(CustomMiningTask);
                if (customMiningInternalHeader->itemCount > 0 && remainedSize >= dataSize)
                {
                    packedHeader->itemCount += customMiningInternalHeader->itemCount;
                    // Copy data
                    copyMem(traverseData, data + sizeof(CustomMiningRespondDataHeader), dataSize);

                    // Update pointer and size
                    traverseData += dataSize;
                    remainedSize -= dataSize;
                }
            }
        }

        return packedData;
    }

    // update the last 3 indexes, caller is supposed to lock all task arrays before calling
    // only effective from v2
    void updateTaskIndex(unsigned long long ti)
    {
        if (ti < _last3TaskV2Indexes[0])
        {
            // invalid: ti is supposed to be newer than the last index
            return;
        }
        _last3TaskV2Indexes[2] = _last3TaskV2Indexes[1];
        _last3TaskV2Indexes[1] = _last3TaskV2Indexes[0];
        _last3TaskV2Indexes[0] = ti;
    }

    // check if the solution is stale: task index is less than the last 3
    // only effective from v2
    bool isSolutionStale(unsigned long long ti)
    {
        return ti < _last3TaskV2Indexes[2];
    }

    unsigned long long _last3TaskV2Indexes[3]; // [0] is max, [2] is min
    CustomMiningTaskStorage _taskStorage[NUMBER_OF_TASK_PARTITIONS];
    CustomMiningTaskV2Storage _taskV2Storage;
    CustomMiningSolutionStorage _solutionStorage[NUMBER_OF_TASK_PARTITIONS];
    CustomMiningSolutionStorage _solutionV2Storage;



    // Buffer can accessed from multiple threads
    unsigned char* _dataBuffer[MAX_NUMBER_OF_PROCESSORS];

};

// Describe how a task is divided into groups 
// The task is for computorID in range [firstComputorIdx, lastComputorIdx]
// domainSize determine the range of nonce that computor should work on
// Currenly, it is designed as
//  domainSize = (2 ^ 32)/ (lastComputorIndex - firstComputorIndex + 1);
//  myNonceOffset = (myComputorIndex - firstComputorIndex) * domainSize;
//  myNonce = myNonceOffset + x; x = [0, domainSize - 1]
// For computorID calculation,
// computorID = myNonce / domainSize + firstComputorIndex
struct CustomMiningTaskPartition
{
    unsigned short firstComputorIdx;
    unsigned short lastComputorIdx;
    unsigned int domainSize;
};

#define SOLUTION_CACHE_DYNAMIC_MEM 0

static CustomMiningTaskPartition gTaskPartition[NUMBER_OF_TASK_PARTITIONS];

#if SOLUTION_CACHE_DYNAMIC_MEM 
static CustomMininingCache<CustomMiningSolutionCacheEntry, MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS, 20>* gSystemCustomMiningSolutionCache = NULL;
#else
static CustomMininingCache<CustomMiningSolutionCacheEntry, MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS, 20> gSystemCustomMiningSolutionCache[NUMBER_OF_TASK_PARTITIONS];
#endif

static CustomMininingCache<CustomMiningSolutionV2CacheEntry, MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS, 20> gSystemCustomMiningSolutionV2Cache;

static CustomMiningStorage gCustomMiningStorage;
static CustomMiningStats gCustomMiningStats;

// Get the part ID
int customMiningGetPartitionID(unsigned short firstComputorIndex, unsigned short lastComputorIndex)
{
    int partitionID = -1;
    for (int k = 0; k < NUMBER_OF_TASK_PARTITIONS; k++)
    {
        if (firstComputorIndex == gTaskPartition[k].firstComputorIdx
            && lastComputorIndex == gTaskPartition[k].lastComputorIdx)
        {
            partitionID = k;
            break;
        }
    }
    return partitionID;
}


// Generate computor task partition
int customMiningInitTaskPartitions()
{
    for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
    {
        // Currently the task is partitioned evenly
        gTaskPartition[i].firstComputorIdx = i * NUMBER_OF_COMPUTORS / NUMBER_OF_TASK_PARTITIONS;
        gTaskPartition[i].lastComputorIdx = gTaskPartition[i].firstComputorIdx + NUMBER_OF_COMPUTORS / NUMBER_OF_TASK_PARTITIONS - 1;
        ASSERT(gTaskPartition[i].lastComputorIdx > gTaskPartition[i].firstComputorIdx + 1);
        gTaskPartition[i].domainSize = (unsigned int)((1ULL << 32) / ((unsigned long long)gTaskPartition[i].lastComputorIdx - gTaskPartition[i].firstComputorIdx + 1));
    }
    return 0;
}

// Get computor ids
int customMiningGetComputorID(unsigned int nonce, int partId)
{
    return nonce / gTaskPartition[partId].domainSize + gTaskPartition[partId].firstComputorIdx;
}

int customMiningInitialize()
{
    gCustomMiningStorage.init();
#if SOLUTION_CACHE_DYNAMIC_MEM 
    allocPoolWithErrorLog(L"gSystemCustomMiningSolutionCache",
        NUMBER_OF_TASK_PARTITIONS * sizeof(CustomMininingCache<CustomMiningSolutionCacheEntry, MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS, 20>),
        (void**)&gSystemCustomMiningSolutionCache,
        __LINE__);
#endif
    setMem((unsigned char*)gSystemCustomMiningSolutionCache, NUMBER_OF_TASK_PARTITIONS * sizeof(CustomMininingCache<CustomMiningSolutionCacheEntry, MAX_NUMBER_OF_CUSTOM_MINING_SOLUTIONS, 20>), 0);
    for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
    {
        gSystemCustomMiningSolutionCache[i].init();
    }
    gSystemCustomMiningSolutionV2Cache.init();
    customMiningInitTaskPartitions();

    return 0;
}

int customMiningDeinitialize()
{
#if SOLUTION_CACHE_DYNAMIC_MEM 
    if (gSystemCustomMiningSolutionCache)
    {
        freePool(gSystemCustomMiningSolutionCache);
        gSystemCustomMiningSolutionCache = NULL;
    }
#endif
    gCustomMiningStorage.deinit();
    return 0;
}

#ifdef NO_UEFI
#else
// Save score cache to SCORE_CACHE_FILE_NAME
void saveCustomMiningCache(int epoch, CHAR16* directory = NULL)
{
    logToConsole(L"Saving custom mining cache file...");
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
    for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
    {
        CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 8] = i / 100 + L'0';
        CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 7] = (i % 100) / 10 + L'0';
        CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 6] = i % 10 + L'0';
        gSystemCustomMiningSolutionCache[i].save(CUSTOM_MINING_CACHE_FILE_NAME, directory);
    }

    CUSTOM_MINING_V2_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
    CUSTOM_MINING_V2_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
    CUSTOM_MINING_V2_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
    gSystemCustomMiningSolutionV2Cache.save(CUSTOM_MINING_V2_CACHE_FILE_NAME, directory);
}

// Update score cache filename with epoch and try to load file
bool loadCustomMiningCache(int epoch)
{
    logToConsole(L"Loading custom mining cache...");
    bool success = true;
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
    CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
    // TODO: Support later
    for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
    {
        CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 8] = i / 100 + L'0';
        CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 7] = (i % 100) / 10 + L'0';
        CUSTOM_MINING_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_CACHE_FILE_NAME[0]) - 6] = i % 10 + L'0';
        success &= gSystemCustomMiningSolutionCache[i].load(CUSTOM_MINING_CACHE_FILE_NAME);
    }

    CUSTOM_MINING_V2_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
    CUSTOM_MINING_V2_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
    CUSTOM_MINING_V2_CACHE_FILE_NAME[sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME) / sizeof(CUSTOM_MINING_V2_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
    success &= gSystemCustomMiningSolutionV2Cache.load(CUSTOM_MINING_V2_CACHE_FILE_NAME);

    return success;
}
#endif
