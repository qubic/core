#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/score_cache.h"

#include <random>


template <unsigned int cacheCapacity>
void expectEmptyCache(ScoreCache<cacheCapacity>& cache)
{
    EXPECT_EQ(cache.hitCount(), 0);
    EXPECT_EQ(cache.collisionCount(), 0);
    EXPECT_EQ(cache.missCount(), 0);

    // test that all is empty and access out of bounds is no error
    for (unsigned int i = 0; i < cache.capacity() + 100; ++i)
    {
        // test with arbitrary publicKey and nonce (real and pseudo-random is slow, so use a fast quite random pattern)
        unsigned long long a = i * 123456789ull;
        unsigned long long b = 0xbca326450256c63eull - i * 759037ull;
        unsigned long long c = 2345932453043560ull << (i & 63);
        m256i publicKey(a ^ b, a ^ c, b ^ c, a ^ b ^ c);
        m256i miningSeed = m256i(1, 1, 1, 1);
        m256i nonce((a << 2) ^ b, (a << 2) ^ c, (b << 1) ^ c, (b >> 1) ^ c);

        unsigned int ioIdx = i;
        EXPECT_EQ(cache.tryFetching(publicKey, miningSeed, nonce, ioIdx), cache.SCORE_CACHE_MISS);
        EXPECT_TRUE(ioIdx == i || (i >= cache.capacity() && ioIdx == i % cache.capacity()));
    }
}

template <unsigned int cacheCapacity>
unsigned int pseudoRandomCacheTest(ScoreCache<cacheCapacity> & cache, unsigned long long seed, unsigned int entryCount, bool overwrite)
{
    cache.reset();

    // add entries with pseudo-random data
    std::mt19937_64 gen64;
    gen64.seed(seed);
    for (unsigned int i = 0; i < entryCount; ++i)
    {
        m256i publicKey(gen64(), gen64(), gen64(), gen64());
        m256i miningSeed(gen64(), gen64(), gen64(), gen64());
        m256i nonce(gen64(), gen64(), gen64(), gen64());
        int score = gen64() % std::numeric_limits<int>::max();
        assert(score >= 0);
        unsigned int idx = cache.getCacheIndex(publicKey, miningSeed, nonce);
        int fetchedScore = cache.tryFetching(publicKey, miningSeed, nonce, idx);

        // assume that we will not get the same publicKey and nonce twice in random entry generation
        EXPECT_TRUE(fetchedScore == cache.SCORE_CACHE_MISS || fetchedScore == cache.SCORE_CACHE_COLLISION);

        if (fetchedScore != cache.SCORE_CACHE_COLLISION || overwrite)
        {
            cache.addEntry(publicKey, miningSeed, nonce, idx, score);
        }
    }
    EXPECT_EQ(entryCount, cache.missCount() + cache.collisionCount());
    EXPECT_EQ(cache.hitCount(), 0);

    //std::cout << "randomCacheTest: capacity " << cacheCapacity << ", filled " << 100.0 * entryCount / cacheCapacity << "%, overwrite=" << overwrite
    //    << ", collisions " << cache.collisionCount() << ", misses " << cache.missCount() << ", hits " << cache.hitCount() << " (SEED " << seed << ")" << std::endl;

    int collisionCount = cache.collisionCount();

    // test entries with pseudo-random data
    gen64.seed(seed);
    for (unsigned int i = 0; i < entryCount; ++i)
    {
        m256i publicKey(gen64(), gen64(), gen64(), gen64());
        m256i miningSeed(gen64(), gen64(), gen64(), gen64());
        m256i nonce(gen64(), gen64(), gen64(), gen64());
        int expectedScore = gen64() % std::numeric_limits<int>::max();
        
        unsigned int idx = cache.getCacheIndex(publicKey, miningSeed, nonce);
        int fetchedScore = cache.tryFetching(publicKey, miningSeed, nonce, idx);

        EXPECT_TRUE(fetchedScore == cache.SCORE_CACHE_COLLISION || fetchedScore >= cache.MIN_VALID_SCORE);
        if (fetchedScore >= cache.MIN_VALID_SCORE)
        {
            EXPECT_EQ(fetchedScore, expectedScore);
        }
    }

    EXPECT_EQ(entryCount * 2, cache.missCount() + cache.collisionCount() + cache.hitCount());
    

    return collisionCount;
}

template <unsigned int cacheCapacity>
void testCacheSameSeeds(unsigned int fillPercent)
{
    typedef ScoreCache<cacheCapacity>  CacheType;
    CacheType* cache = new CacheType();

    expectEmptyCache(*cache);

    bool overwrite = true;
    const int entryCount = (unsigned long long)cacheCapacity * fillPercent / 100;
    unsigned int collisionCount = 0;
    collisionCount += pseudoRandomCacheTest(*cache, 0, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 1234, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 42, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 987654321, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 1234573574564560925, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 234563875344, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 3245789, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 9357637, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 23648682, entryCount, overwrite);
    collisionCount += pseudoRandomCacheTest(*cache, 347692997236, entryCount, overwrite);
    std::cout << "Total collision count with capacity " << cacheCapacity << " (10 tests): " << collisionCount << std::endl;

    cache->reset();
    expectEmptyCache(*cache);

    delete cache;
}

template <unsigned int cacheCapacity>
void testCacheRandomSeeds(unsigned int fillPercent)
{
    typedef ScoreCache<cacheCapacity>  CacheType;
    CacheType* cache = new CacheType();

    expectEmptyCache(*cache);

    bool overwrite = true;
    const int entryCount = (unsigned long long)cacheCapacity * fillPercent / 100;
    unsigned int collisionCount = 0;
    for (int i = 0; i < 10; ++i)
    {
        unsigned long long seed;
        _rdrand64_step(&seed);
        collisionCount += pseudoRandomCacheTest(*cache, seed, entryCount, overwrite);
    }
    std::cout << "Total collision count with capacity " << cacheCapacity << " (10 tests): " << collisionCount << std::endl;

    cache->reset();
    expectEmptyCache(*cache);

    delete cache;
}


// Some people claimed that the hash table will have less collisions if the capacity
// is a prime number. The experiments with our table do not support this claim.
// A search in the internet showed that you only need prime number capacity
// if your hash function is not good.
// http://srinvis.blogspot.com/2006/07/hash-table-lengths-and-prime-numbers.html
// So it seems like our hash function is good and we do not need prime numbers.

TEST(TestQubicScoreCache, FixedSeeds20pctFilled) {
    testCacheSameSeeds<1000000>(20);    // non-prime number as cache size
    testCacheSameSeeds<1000003>(20);    // prime number as cache size

    testCacheSameSeeds<200000>(20);     // non-prime number as cache size
    testCacheSameSeeds<199999>(20);     // prime number as cache size
}

TEST(TestQubicScoreCache, FixedSeeds50pctFilled) {
    testCacheSameSeeds<1000000>(50);    // non-prime number as cache size
    testCacheSameSeeds<1000003>(50);    // prime number as cache size
    
    testCacheSameSeeds<200000>(50);     // non-prime number as cache size
    testCacheSameSeeds<199999>(50);     // prime number as cache size
}

TEST(TestQubicScoreCache, FixedSeeds80pctFilled) {
    testCacheSameSeeds<1000000>(80);    // non-prime number as cache size
    testCacheSameSeeds<1000003>(80);    // prime number as cache size

    testCacheSameSeeds<200000>(80);     // non-prime number as cache size
    testCacheSameSeeds<199999>(80);     // prime number as cache size
}

TEST(TestQubicScoreCache, RandomSeeds20pctFilled) {
    testCacheRandomSeeds<1000000>(20);    // non-prime number as cache size
    testCacheRandomSeeds<1000003>(20);    // prime number as cache size

    testCacheRandomSeeds<200000>(20);     // non-prime number as cache size
    testCacheRandomSeeds<199999>(20);     // prime number as cache size
}

TEST(TestQubicScoreCache, RandomSeeds50pctFilled) {
    testCacheRandomSeeds<1000000>(50);    // non-prime number as cache size
    testCacheRandomSeeds<1000003>(50);    // prime number as cache size

    testCacheRandomSeeds<200000>(50);     // non-prime number as cache size
    testCacheRandomSeeds<199999>(50);     // prime number as cache size
}

TEST(TestQubicScoreCache, RandomSeeds80pctFilled) {
    testCacheRandomSeeds<1000000>(80);    // non-prime number as cache size
    testCacheRandomSeeds<1000003>(80);    // prime number as cache size

    testCacheRandomSeeds<200000>(80);     // non-prime number as cache size
    testCacheRandomSeeds<199999>(80);     // prime number as cache size
}
