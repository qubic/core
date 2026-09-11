// Standalone K12 benchmark: production-equivalent timing of the one-shot, streaming and 64To32
// paths of src/kangaroo_twelve.h.
// Single translation unit, no gtest, NDEBUG: ASSERT() inside KangarooTwelveStream compiles to
// nothing, exactly as in the Release UEFI build (in test.exe it expands to EXPECT_TRUE).
//
// Build (from the repository root), scalar and AVX-512 flavours:
//   clang++ -std=c++17 -O2 -w -I src -I . -mavx2 -mbmi -DNDEBUG tools/k12_bench/k12_bench.cpp -o k12_bench_avx2
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
#if defined(__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
    puts("custom K12: AVX-512 single-state path");
#else
    puts("custom K12: scalar path");
#endif

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
