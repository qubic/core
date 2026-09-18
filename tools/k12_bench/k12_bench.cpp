// Standalone K12 benchmark: production-equivalent timing of the one-shot, streaming and 64To32
// paths of src/kangaroo_twelve.h.
// Single translation unit, no gtest, NDEBUG: ASSERT() inside KangarooTwelveStream compiles to
// nothing, exactly as in the Release UEFI build (in test.exe it expands to EXPECT_TRUE).
//
// Build (from the repository root), scalar and AVX-512 flavours:
//   clang++ -std=c++17 -O2 -w -I src -I . -mavx2 -mbmi -DNDEBUG -pthread tools/k12_bench/k12_bench.cpp -o k12_bench_avx2
//   ... same with -mavx512f -mavx512bw -mavx512dq -mavx512vl for the AVX-512 flavour.
// MSVC: cl /O2 /arch:AVX2 /DNDEBUG /DNO_UEFI /I src /I . tools\k12_bench\k12_bench.cpp
// Run pinned to one idle core, e.g.  taskset -c 2 ./k12_bench_avx2
// File mode:  ./k12_bench_avx2 <helpers> contract0001.230 ...   (real state files, one-shot vs parallel)
// Cache modes: ./k12_bench_avx2 selftest            (chaining-value cache vs one-shot, exit code = mismatches)
//              ./k12_bench_avx2 cache <helpers>     (per-tick digest cost with the cache: clean, sparse, all dirty)

#define NO_UEFI
#ifndef NDEBUG
#define NDEBUG
#endif

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "kangaroo_twelve.h"
#include "optimizations/opt_parallel_k12_leaves.h"
#include <thread>

void setMem(void* buffer, unsigned long long size, unsigned char value) { memset(buffer, value, size); }
void copyMem(void* destination, const void* source, unsigned long long length) { memcpy(destination, source, length); }

static void streamNative(const unsigned char* d, size_t len, size_t updateSize, unsigned char out[32])
{
    KangarooTwelveStream s;
    s.init();
    for (size_t p = 0; p < len; p += updateSize)
        s.update(d + p, updateSize);
    s.finalize(out);
}

template <class F>
static long long timeOnce(F fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    return (long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
}

// File mode: k12_bench <helpers> <file>...   hashes each file (e.g. a saved contract state contract0001.230)
// one-shot and through the parallel leaf pool with the given number of helper threads, compares the
// digests and prints both timings. Returns non-zero on any mismatch.
static int fileMode(int argc, char** argv)
{
    const int helperCount = atoi(argv[1]);
    typedef ParallelK12LeafTasksT<ParallelK12LeafTasks::TASK_LEAVES, 64> BenchPool;
    static BenchPool pool;
    int mismatches = 0;
    for (int a = 2; a < argc; a++)
    {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("%s: cannot open\n", argv[a]); mismatches++; continue; }
        fseek(f, 0, SEEK_END);
        const long long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> state((size_t)size);
        if (fread(state.data(), 1, (size_t)size, f) != (size_t)size) { printf("%s: short read\n", argv[a]); fclose(f); mismatches++; continue; }
        fclose(f);
        std::vector<unsigned char> cvBuffer(BenchPool::chainingValueBufferSize((unsigned long long)size));
        pool.init(cvBuffer.data(), (unsigned long long)size);

        unsigned char ref[32], out[32];
        long long oneShotUs = -1, parUs = -1;
        for (int rep = 0; rep < 3; rep++)
        {
            const long long us = timeOnce([&] { KangarooTwelve(state.data(), (unsigned int)size, ref, 32); });
            if (oneShotUs < 0 || us < oneShotUs) oneShotUs = us;
        }
        volatile bool stop = false;
        std::vector<std::thread> helpers;
        for (int i = 0; i < helperCount; i++)
            helpers.emplace_back([&] { while (!stop) { if (!pool.tryProcessOne()) std::this_thread::yield(); } });
        for (int rep = 0; rep < 3; rep++)
        {
            const long long us = timeOnce([&] { pool.digest(state.data(), (unsigned long long)size, out); });
            if (parUs < 0 || us < parUs) parUs = us;
        }
        stop = true;
        for (auto& h : helpers) h.join();
        const bool ok = memcmp(out, ref, 32) == 0;
        if (!ok) mismatches++;
        printf("%-22s %8.1f MB  one-shot %7.1f ms  parallel(%d helpers) %7.1f ms  %5.1fx  digest ", argv[a], size / 1048576.0, oneShotUs / 1000.0, helperCount, parUs / 1000.0, (double)oneShotUs / parUs);
        for (int i = 0; i < 32; i++) printf("%02x", ref[i]);
        printf("  %s\n", ok ? "ok" : "MISMATCH");
    }
    return mismatches ? 1 : 0;
}

static std::vector<unsigned char> pseudoRandom(size_t n, unsigned long long seed)
{
    std::vector<unsigned char> v(n);
    for (size_t i = 0; i < n; i++)
    {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        v[i] = (unsigned char)(seed >> 56);
    }
    return v;
}

typedef ParallelK12LeafTasksT<ParallelK12LeafTasks::TASK_LEAVES, 64> CachePool;
static CachePool cachePool;

struct Helpers
{
    volatile bool stop = false;
    std::vector<std::thread> threads;
    Helpers(int count)
    {
        for (int i = 0; i < count; i++)
            threads.emplace_back([&] { while (!stop) { if (!cachePool.tryProcessOne()) std::this_thread::yield(); } });
    }
    ~Helpers()
    {
        stop = true;
        for (auto& h : threads) h.join();
    }
};

struct CachedState
{
    std::vector<unsigned char> state, cvBuffer, poolScratch;
    std::vector<unsigned long long> bitmap;
    K12ChainingValueCache cache;
    CachedState(size_t size, unsigned long long seed)
        : state(pseudoRandom(size, seed)),
          cvBuffer(K12ChainingValueCache::chainingValuesSize(size) + 1),
          poolScratch(CachePool::chainingValueBufferSize(size)),
          bitmap(K12ChainingValueCache::wordCountOf(size) + 1)
    {
        cachePool.init(poolScratch.data(), size);
        cache.init(cvBuffer.data(), bitmap.data(), size);
    }
    bool digestMatchesOneShot()
    {
        unsigned char ref[32], out[32];
        KangarooTwelve(state.data(), (unsigned int)state.size(), ref, 32);
        cachePool.digest(state.data(), state.size(), cache, out);
        return memcmp(ref, out, 32) == 0;
    }
    // write one byte at offset and report it
    void write(size_t offset)
    {
        state[offset] ^= 0x5A;
        cache.markDirtyBytes(offset, 1);
    }
};

// Correctness of the chaining-value cache against one-shot K12. Returns the number of failures.
static int selfTest()
{
    int failures = 0;
    auto check = [&](bool ok, const char* what, size_t size, size_t detail)
    {
        if (!ok)
        {
            printf("FAIL %s size %zu detail %zu\n", what, size, detail);
            failures++;
        }
    };
    const size_t sizes[] = {0, 1, 4095, 4096, 4097, 8191, 8192, 8193, 12288, 16383, 16384, 16385, 24576, 24577,
                            3 * 1024 * 1024 + 17, 3 * 1024 * 1024 + 8192, 40 * 1024 * 1024 + 4321};
    unsigned long long seed = 7;
    for (size_t size : sizes)
    {
        for (int helperCount : {0, 3})
        {
            Helpers helpers(helperCount);
            CachedState cs(size, 0xC0FFEE + size);
            check(cs.digestMatchesOneShot(), "initial (invalid cache)", size, helperCount);
            check(cs.digestMatchesOneShot(), "unchanged", size, helperCount);
            if (size)
            {
                // first chunk, either half of a leaf, tail, and the very last byte
                const size_t leaves = (size_t)K12ChainingValueCache::leafCountOf(size);
                std::vector<size_t> offsets = {0, size - 1};
                if (size > 8192) offsets.push_back(8191);
                if (leaves) { offsets.push_back(8192); offsets.push_back(8192 + 4096); offsets.push_back(8192 + 8191); }
                if (leaves > 1) { offsets.push_back(8192 + 8192 * (leaves - 1) + 100); }
                if (leaves) { offsets.push_back(8192 + 8192 * leaves); } // first tail byte (or last byte)
                for (size_t off : offsets)
                {
                    if (off >= size) continue;
                    cs.write(off);
                    check(cs.digestMatchesOneShot(), "single write", size, off);
                }
                // two pages of the same leaf in one interval, then random repeated mutations
                if (leaves) { cs.write(8192 + 10); cs.write(8192 + 4096 + 10); check(cs.digestMatchesOneShot(), "two pages one leaf", size, helperCount); }
                for (int round = 0; round < 20; round++)
                {
                    const int writes = 1 + (int)(seed % 5);
                    for (int w = 0; w < writes; w++)
                    {
                        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
                        cs.write((size_t)((seed >> 20) % size));
                    }
                    check(cs.digestMatchesOneShot(), "random mutations", size, round);
                }
                cs.cache.invalidate();
                check(cs.digestMatchesOneShot(), "forced invalidation", size, helperCount);
                check(cs.cache.verifySample(cs.state.data(), 64, 1) == 0, "verifySample clean", size, helperCount);
                // negative: an unreported write to a leaf must break the digest and be caught by a full sample
                if (leaves)
                {
                    cs.state[8192 + 8192 * (leaves / 2) + 5] ^= 1;
                    check(!cs.digestMatchesOneShot(), "unreported write goes unnoticed by digest", size, helperCount);
                    check(cs.cache.verifySample(cs.state.data(), leaves * 8, 3) > 0, "verifySample misses unreported write", size, helperCount);
                    cs.cache.invalidate();
                    check(cs.digestMatchesOneShot(), "recover after invalidate", size, helperCount);
                }
            }
        }
    }
    printf("selftest: %d failures\n", failures);
    return failures;
}

// Per-tick cost of the cached digest on a 600 MiB state: no change, N dirty 4 KiB pages, all dirty.
static int cacheBench(int helperCount)
{
    const size_t size = 600u << 20;
    CachedState cs(size, 42);
    Helpers helpers(helperCount);
    unsigned char ref[32], out[32];
    long long oneShotUs = -1;
    for (int rep = 0; rep < 3; rep++)
    {
        const long long us = timeOnce([&] { KangarooTwelve(cs.state.data(), (unsigned int)size, ref, 32); });
        if (oneShotUs < 0 || us < oneShotUs) oneShotUs = us;
    }
    printf("cache bench: 600 MiB state, %d helpers, one-shot 1 core %lld ms\n", helperCount, oneShotUs / 1000);
    auto scenario = [&](const char* name, auto mutate)
    {
        long long best = -1;
        bool ok = true;
        for (int rep = 0; rep < 5; rep++)
        {
            mutate(rep);
            const long long us = timeOnce([&] { cachePool.digest(cs.state.data(), size, cs.cache, out); });
            KangarooTwelve(cs.state.data(), (unsigned int)size, ref, 32);
            ok = ok && memcmp(ref, out, 32) == 0;
            if (best < 0 || us < best) best = us;
        }
        printf("  %-28s %9.2f ms  %7.1fx  %s\n", name, best / 1000.0, (double)oneShotUs / (best ? best : 1), ok ? "ok" : "MISMATCH");
        return ok ? 0 : 1;
    };
    unsigned long long seed = 99;
    auto dirtyPages = [&](int pages)
    {
        for (int p = 0; p < pages; p++)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            const size_t page = (size_t)((seed >> 20) % (size / 4096));
            cs.state[page * 4096 + 7] ^= 0x33;
            cs.cache.markDirtyBytes(page * 4096, 4096);
        }
    };
    int failures = 0;
    cs.digestMatchesOneShot(); // warm: cache valid
    failures += scenario("no change", [&](int) {});
    failures += scenario("1 dirty page", [&](int) { dirtyPages(1); });
    failures += scenario("64 dirty pages", [&](int) { dirtyPages(64); });
    failures += scenario("1024 dirty pages", [&](int) { dirtyPages(1024); });
    failures += scenario("16384 dirty pages", [&](int) { dirtyPages(16384); });
    failures += scenario("all dirty (invalidate)", [&](int) { cs.cache.invalidate(); });
    return failures;
}

int main(int argc, char** argv)
{
    if (argc >= 2 && strcmp(argv[1], "selftest") == 0)
        return selfTest();
    if (argc >= 3 && strcmp(argv[1], "cache") == 0)
        return cacheBench(atoi(argv[2]));
    if (argc >= 3)
        return fileMode(argc, argv);
    constexpr int repetitions = 7;

    std::vector<unsigned char> m(256u << 20);
    for (size_t i = 0; i < m.size(); i++)
        m[i] = (unsigned char)(i % 251);

    // Equal repetitions per backend, order rotated every repetition, minimum reported.
    auto shape = [&](size_t updates, size_t updateSize, const char* name)
    {
        const size_t len = updates * updateSize;
        unsigned char b[32], c[32];
        auto runNative = [&] { streamNative(m.data(), len, updateSize, b); };
        auto runOneShot = [&] { KangarooTwelve(m.data(), (unsigned int)len, c, 32); };
        runNative(); runOneShot();
        long long best[2] = {-1, -1};
        for (int rep = 0; rep < repetitions; ++rep)
            for (int k = 0; k < 2; ++k)
            {
                const int backend = (rep + k) % 2;
                const long long us = backend == 0 ? timeOnce(runNative) : timeOnce(runOneShot);
                if (best[backend] < 0 || us < best[backend])
                    best[backend] = us;
            }
        printf("%-22s %5zu x %4zu B  [us] stream %6lld | one-shot %6lld | %s\n", name, updates, updateSize,
               best[0], best[1], memcmp(b, c, 32) ? "MISMATCH" : "ok");
    };
    shape(4096, 1200, "txBodyDigest worst");
    shape(4096, 100, "txBodyDigest small");
    shape(20000, 80, "logging chain");

    {
        const size_t big = m.size();
        unsigned char b[32], c[32];
        long long best[2] = {-1, -1};
        auto runNative = [&] { streamNative(m.data(), big, 8192 * 64, b); };
        auto runOneShot = [&] { KangarooTwelve(m.data(), (unsigned int)big, c, 32); };
        runNative(); runOneShot();
        for (int rep = 0; rep < repetitions; ++rep)
            for (int k = 0; k < 2; ++k)
            {
                const int backend = (rep + k) % 2;
                const long long us = backend == 0 ? timeOnce(runNative) : timeOnce(runOneShot);
                if (best[backend] < 0 || us < best[backend])
                    best[backend] = us;
            }
        printf("256 MiB [MB/s]: stream %.0f | one-shot %.0f | %s\n", big / 1.048576 / best[0], big / 1.048576 / best[1], memcmp(b, c, 32) ? "MISMATCH" : "ok");
    }

    // Parallel leaf hashing of a QX-sized state (600 MB): tick processor plus N helper threads that act
    // as request processors, versus the one-shot on one core.
    {
        const size_t stateSize = 600u << 20;
        std::vector<unsigned char> state(stateSize);
        for (size_t i = 0; i < stateSize; i++)
            state[i] = (unsigned char)(i % 251);
        // production task size, helper cap lifted so the scaling beyond PARALLEL_K12_LEAVES_MAX_HELPERS is visible
        typedef ParallelK12LeafTasksT<ParallelK12LeafTasks::TASK_LEAVES, 64> BenchPool;
        static BenchPool benchPool;
        std::vector<unsigned char> cvBuffer(BenchPool::chainingValueBufferSize(stateSize));
        benchPool.init(cvBuffer.data(), stateSize);
        unsigned char ref[32], out[32];
        long long oneShotUs = -1;
        for (int rep = 0; rep < 3; rep++)
        {
            const long long us = timeOnce([&] { KangarooTwelve(state.data(), (unsigned int)stateSize, ref, 32); });
            if (oneShotUs < 0 || us < oneShotUs) oneShotUs = us;
        }
        printf("600 MiB state one-shot, 1 core: %lld ms\n", oneShotUs / 1000);
        const int helperCounts[] = {0, 3, 7, 15, 23, 31};
        for (int helperCount : helperCounts)
        {
            if (helperCount + 1 > (int)std::thread::hardware_concurrency())
                break;
            volatile bool stop = false;
            std::vector<std::thread> helpers;
            for (int i = 0; i < helperCount; i++)
                helpers.emplace_back([&] { while (!stop) { if (!benchPool.tryProcessOne()) std::this_thread::yield(); } });
            long long best = -1;
            for (int rep = 0; rep < 3; rep++)
            {
                const long long us = timeOnce([&] { benchPool.digest(state.data(), stateSize, out); });
                if (best < 0 || us < best) best = us;
            }
            stop = true;
            for (auto& h : helpers) h.join();
            printf("600 MiB state parallel leaves, ticker + %2d helpers: %5lld ms  (%.1fx)  %s\n", helperCount, best / 1000,
                   (double)oneShotUs / best, memcmp(out, ref, 32) ? "MISMATCH" : "ok");
        }
    }

    {
        std::vector<unsigned char> r(64 * 100000);
        unsigned long long seed = 1;
        for (auto& x : r)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            x = (unsigned char)(seed >> 56);
        }
        unsigned long long checksum = 0; // consumed so the loop cannot be optimized away
        long long best = -1;
        for (int rep = 0; rep <= repetitions; ++rep)
        {
            const long long us = timeOnce([&] {
                for (size_t i = 0; i < 100000; i++)
                {
                    unsigned char d[32];
                    KangarooTwelve64To32(r.data() + 64 * i, d);
                    checksum += *(unsigned long long*)d;
                }
            });
            if (rep && (best < 0 || us < best))
                best = us;
        }
        printf("64To32: %.1f ns/hash (checksum %016llx)\n", best * 1000.0 / 100000, checksum);
    }
    return 0;
}
