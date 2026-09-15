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

#define NO_UEFI
#ifndef NDEBUG
#define NDEBUG
#endif

#include <chrono>
#include <cstdio>
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

int main()
{
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
