#pragma once

#include "platform/m256.h"
#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/file_io.h"
#include "platform/console_logging.h"
#include "platform/time_stamp_counter.h"

#include "kangaroo_twelve.h"

/// Cache storing scores for pairs of publicKey and nonce (hash map)
template <unsigned int size, unsigned int collisionRetries = 20>
class ScoreCache
{
    static_assert(collisionRetries < size, "Number of fetch retries in case of collision is too big!");
public:

    /// Init cache
    ScoreCache()
    {
        reset();
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
    unsigned int getCacheIndex(const m256i& publicKey, const m256i& nonce)
    {
        m256i buffer[2] = { publicKey, nonce };
        unsigned char digest[32];
        KangarooTwelve64To32(buffer, digest);
        unsigned int result = *((unsigned long long*)digest) % capacity();

        return result;
    }

    static constexpr int MIN_VALID_SCORE = 0;
    static constexpr int SCORE_CACHE_MISS = -1;
    static constexpr int SCORE_CACHE_COLLISION = -2;

    // Try to fetch data from cacheIndex, also checking a few following entries in case of collisions (may update cacheIndex),
    // increments counter of hits, misses, or collisions
    int tryFetching(const m256i& publicKey, const m256i& nonce, unsigned int & cacheIndex)
    {
        int retVal;
        unsigned int tryFetchIdx = cacheIndex % capacity();
        ACQUIRE(lock);
        for (unsigned int i = 0; i < collisionRetries; ++i)
        {
            const m256i& cachedPublicKey = cache[tryFetchIdx].publicKey;
            if (isZero(cachedPublicKey))
            {
                // miss: data not available in cache yet (entry is empty)
                misses++;
                retVal = SCORE_CACHE_MISS;
                break;
            }

            const m256i& cachedNonce = cache[tryFetchIdx].nonce;
            if (cachedPublicKey == publicKey && cachedNonce == nonce)
            {
                // hit: data available in cache -> return score
                hits++;
                retVal = cache[tryFetchIdx].score;
                break;
            }

            // collision: other data is mapped to same index -> retry at following index
            retVal = SCORE_CACHE_COLLISION;
            tryFetchIdx = (tryFetchIdx + 1) % capacity();
        }
        RELEASE(lock);

        if (retVal == SCORE_CACHE_COLLISION)
        {
            collisions++;
        }
        else
        {
            cacheIndex = tryFetchIdx;
        }
        return retVal;
    }

    /// Add entry to cache (may overwrite existing entry)
    void addEntry(const m256i& publicKey, const m256i& nonce, unsigned int cacheIndex, int score)
    {
        cacheIndex %= capacity();
        ACQUIRE(lock);
        cache[cacheIndex].publicKey = publicKey;
        cache[cacheIndex].nonce = nonce;
        cache[cacheIndex].score = score;
        RELEASE(lock);
    }

    /// Save score cache to file
    void save(CHAR16* filename, CHAR16* directory = NULL)
    {
        logToConsole(L"Saving score cache file...");

        const unsigned long long beginningTick = __rdtsc();
        ACQUIRE(lock);
        long long savedSize = ::save(filename, sizeof(cache), (unsigned char*)&cache, directory);
        RELEASE(lock);
        if (savedSize == sizeof(cache))
        {
            setNumber(message, savedSize, TRUE);
            appendText(message, L" bytes of the score cache data are saved (");
            appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
            appendText(message, L" microseconds).");
            logToConsole(message);
        }
    }

    /// Try to load score cache file
    bool load(CHAR16* filename, CHAR16* directory = NULL)
    {
        bool success = true;
        logToConsole(L"Loading score cache...");
        reset();
        ACQUIRE(lock);
        long long loadedSize = ::load(filename, sizeof(cache), (unsigned char*)cache, directory);
        RELEASE(lock);
        if (loadedSize != sizeof(cache))
        {
            if (loadedSize == -1)
            {
                logToConsole(L"Error while loading score cache: File does not exists (ignore this error if this is the epoch start)");
            }
            else if (loadedSize < sizeof(cache))
            {
                logToConsole(L"Error while loading score cache: Score cache file is smaller than defined. System may not work properly");
            }
            else
            {
                logToConsole(L"Error while loading score cache: Score cache file is larger than defined. System may not work properly");
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
    struct CacheEntry
    {
        m256i publicKey;
        m256i nonce;
        int score;
    };
    
    // cache entries (set zero or load from a file on init)
    CacheEntry cache[size];

    // lock to prevent race conditions on parallel access
    volatile char lock = 0;

    // statistics of hits, misses, and collisions
    unsigned int hits = 0;
    unsigned int misses = 0;
    unsigned int collisions = 0;
};
