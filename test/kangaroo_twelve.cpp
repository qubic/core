#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/kangaroo_twelve.h"
#include "../src/platform/memory.h"
#include <lib/platform_common/qintrin.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>



// This file pins the exact output of every K12 code path used by the node:
//  - one-shot KangarooTwelve() and KangarooTwelve64To32() from kangaroo_twelve.h (all digests)
//  - streaming KangarooTwelveStream from kangaroo_twelve.h (txBodyDigest, logging digest chain)
// Anchors: KT128 vectors from RFC 9861 section 5, plus golden digests recorded from the
// production implementation at boundary lengths. Any change to the hash code must keep these green.

namespace
{
    // ptn(n) as defined in RFC 9861 section 5: bytes 00 01 .. FA repeated, truncated to n bytes.
    std::vector<unsigned char> ptn(size_t n)
    {
        std::vector<unsigned char> v(n);
        for (size_t i = 0; i < n; ++i)
            v[i] = (unsigned char)(i % 251);
        return v;
    }

    // Seeded pseudo-random bytes (LCG), deterministic across platforms.
    std::vector<unsigned char> pseudoRandom(size_t n, unsigned long long seed)
    {
        std::vector<unsigned char> v(n);
        for (size_t i = 0; i < n; ++i)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            v[i] = (unsigned char)(seed >> 56);
        }
        return v;
    }

    std::string toHex(const unsigned char* d, size_t n)
    {
        static const char* digits = "0123456789ABCDEF";
        std::string s;
        s.reserve(2 * n);
        for (size_t i = 0; i < n; ++i)
        {
            s.push_back(digits[d[i] >> 4]);
            s.push_back(digits[d[i] & 15]);
        }
        return s;
    }

    std::string oneShot(const unsigned char* d, size_t len)
    {
        unsigned char out[32];
        KangarooTwelve(d, (unsigned int)len, out, 32);
        return toHex(out, 32);
    }

    // Streaming hash fed in the given update sizes (remaining bytes are fed as a last update).
    // No test checks inside, so it can be timed as well.
    void streamNativeRaw(const unsigned char* d, size_t len, const std::vector<size_t>& updates, unsigned char out[32])
    {
        KangarooTwelveStream s;
        s.init();
        size_t pos = 0;
        for (size_t u : updates)
        {
            s.update(d + pos, u);
            pos += u;
        }
        if (pos < len)
            s.update(d + pos, len - pos);
        s.finalize(out);
    }

    std::string streamNative(const unsigned char* d, size_t len, const std::vector<size_t>& updates)
    {
        size_t total = 0;
        for (size_t u : updates)
            total += u;
        EXPECT_LE(total, len);
        unsigned char out[32];
        streamNativeRaw(d, len, updates, out);
        return toHex(out, 32);
    }

    // The stream must produce `expected` for the given update pattern.
    testing::AssertionResult streamsMatch(const unsigned char* d, size_t len, const std::vector<size_t>& updates, const char* expected)
    {
        const std::string n = streamNative(d, len, updates);
        if (n == expected)
            return testing::AssertionSuccess();
        return testing::AssertionFailure() << "expected " << expected << ", stream " << n;
    }

    // Deterministic split sizes in [1, maxSize], covering the whole input.
    std::vector<size_t> randomSplits(size_t len, size_t maxSize, unsigned long long seed)
    {
        std::vector<size_t> splits;
        size_t pos = 0;
        while (pos < len)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            size_t u = 1 + (size_t)((seed >> 33) % maxSize);
            if (u > len - pos)
                u = len - pos;
            splits.push_back(u);
            pos += u;
        }
        return splits;
    }

    struct Vector
    {
        size_t len;
        const char* digest;
    };

    // RFC 9861 section 5, KT128 with empty customization, 32-byte output.
    // The 10032-byte-output vector is omitted: the node only ever squeezes one block.
    const Vector rfc9861[] = {
        {0, "1AC2D450FC3B4205D19DA7BFCA1B37513C0803577AC7167F06FE2CE1F0EF39E5"},
        {1, "2BDA92450E8B147F8A7CB629E784A058EFCA7CF7D8218E02D345DFAA65244A1F"},
        {17, "6BF75FA2239198DB4772E36478F8E19B0F371205F6A9A93A273F51DF37122888"},
        {289, "0C315EBCDEDBF61426DE7DCF8FB725D1E74675D7F5327A5067F367B108ECB67C"},
        {4913, "CB552E2EC77D9910701D578B457DDF772C12E322E4EE7FE417F92C758F0D59D0"},
        {83521, "8701045E22205345FF4DDA05555CBB5C3AF1A771C2B89BAEF37DB43D9998B9FE"},
        {1419857, "844D610933B1B9963CBDEB5AE3B6B05CC7CBD67CEEDF883EB678A0A8E0371682"},
        {24137569, "3C390782A8A4E89FA6367F72FEAAF13255C8D95878481D3CD8CE85F58E880AF8"},
        {8191, "1B577636F723643E990CC7D6A659837436FD6A103626600EB8301CD1DBE553D6"},
        {8192, "48F256F6772F9EDFB6A8B661EC92DC93B95EBD05A08A17B39AE3490870C926C3"},
    };
    const char* rfc9861Empty64 = "1AC2D450FC3B4205D19DA7BFCA1B37513C0803577AC7167F06FE2CE1F0EF39E54269C056B8C82E48276038B6D292966CC07A3D4645272E31FF38508139EB0A71";

    // Golden digests of ptn(len) recorded from the production one-shot implementation (which matches
    // the RFC vectors above). The hashed sequence is the message plus one byte (length_encode of the
    // empty customization string), so a message of len bytes has ceil((len + 1 - 8192) / 8192) chaining
    // values. Lengths sit on every state-machine boundary:
    //   rate (168) +-1, 2*rate +-1, chunk (8192) +-1, chunk+rate +-1, 2 chunks +-1,
    //   255 -> 256 chaining values (2097151 -> 2097152, length encoding grows from 1 to 2 bytes),
    //   256 -> 257 chaining values (2105343 -> 2105344).
    const Vector golden[] = {
        {0, "1AC2D450FC3B4205D19DA7BFCA1B37513C0803577AC7167F06FE2CE1F0EF39E5"},
        {1, "2BDA92450E8B147F8A7CB629E784A058EFCA7CF7D8218E02D345DFAA65244A1F"},
        {7, "E92480072F237FD4B80A4FA0EE09036D6A7BB0C3435536BB2E484B4626743E2C"},
        {8, "FD792BFE34DCC323453B16CF4378652EDB046E677410A913767256CDA19A83FE"},
        {166, "CBBE9DD1E423F20003FBA7BB219491C8D1F445FA5C4199D6C6C70C9FDC101964"},
        {167, "77DF46FD2D22BCE26E636E02CE10F9A42AE925E071F9056A9236328DB01BA411"},
        {168, "160F86280614CB99A647108165547BDE9073992BAB7D2E6667D27202F5B31B3A"},
        {169, "C3FD1DE0148E91B62EC282518CA3F3B1230000A2F12B13D2481A775C1EA662A0"},
        {335, "5E7CE79C24DCEE7941C94E42A63ADF8F79CDE03F051E9877238E73BB5998258F"},
        {336, "83EE98BE3E66195099A9D5878159E7958B3882F63311E7ED80AE03E11790EA79"},
        {337, "EF5778ABEC128FF8519C1951F3643FAA74095068AF93241D56FD455F9DBCA25C"},
        {8191, "1B577636F723643E990CC7D6A659837436FD6A103626600EB8301CD1DBE553D6"},
        {8192, "48F256F6772F9EDFB6A8B661EC92DC93B95EBD05A08A17B39AE3490870C926C3"},
        {8193, "BB66FE72EAEA5179418D5295EE1344854D8AD7F3FA17EFCB467EC152341284CF"},
        {8359, "4190D8D12B8E57EC584B611CE8B9A10B2CDB05CAA04C325ECE41DE39A6CDC3D6"},
        {8360, "3ABA0F424EB0AF74445F4BC21C0015A9F3591893B3CFCAB5E1F5AA1B89F5E1FA"},
        {8361, "3FAACF2F0458DA48750A3CE20BE783A5BE0F5629F8209D0233E09CED57F24F5C"},
        {16383, "E3DED52118EA64EAF04C7531C6CCB95E32924B7C2B87B2CE68FF2F2EE46E84EF"},
        {16384, "82778F7F7234C83352E76837B721FBDBB5270B88010D84FA5AB0B61EC8CE0956"},
        {16385, "5F8D2B943922B451842B4E82740D02369E2D5F9F33C5123509A53B955FE177B2"},
        {24581, "CCBA2868E8596CDE94FEC66716B9F1884D7205D113B7817DA70A5359EFFDC398"},
        {1048576, "93070BFD10B8028F3C0EBE9304DD7F10F2C8AE403371AE695591F4710928F8DD"},
        {2097151, "4F6AB79C62109A79AF3CCFB1BFC8D82A9ADC397303ABCBD49B22387BE058B032"},
        {2097152, "4DF92021E4E2865374A69E88EE971F1A2F4AF14B8FBC149E84301CE37D4192BB"},
        {2097153, "2F016F3E3B24C15AE36129266D3AB806520B5AE2AC452B62ADBA41AE3E8D308B"},
        {2105343, "82AD68D0EE25FE185D609954E42AD93BC19CDA9347D5294B076F07F15EFD981F"},
        {2105344, "4A3AA504EA411B89A1482062BD6FA67434E46D11291F448A1C208B81ED273A73"},
        {2105345, "96E25D076945EA36BFD4FF7039ED7D21DD6745F961E818857F6AED32E484A3D9"},
    };

    // Split points that land exactly on state-machine boundaries.
    const size_t boundaryCuts[] = {1, 167, 168, 169, 8191, 8192, 8193, 8360, 16384};
}


TEST(TestCoreK12, Rfc9861Kt128Vectors)
{
    for (const Vector& v : rfc9861)
    {
        const std::vector<unsigned char> m = ptn(v.len);
        EXPECT_EQ(oneShot(m.data(), v.len), v.digest) << "one-shot, len " << v.len;
        EXPECT_TRUE(streamsMatch(m.data(), v.len, {}, v.digest)) << "streams, len " << v.len;
    }

    unsigned char out64[64];
    KangarooTwelve((const unsigned char*)"", 0, out64, 64);
    EXPECT_EQ(toHex(out64, 64), rfc9861Empty64);
}

TEST(TestCoreK12, GoldenOneShot)
{
    for (const Vector& v : golden)
    {
        const std::vector<unsigned char> m = ptn(v.len);
        EXPECT_EQ(oneShot(m.data(), v.len), v.digest) << "one-shot, len " << v.len;
    }
}

TEST(TestCoreK12, GoldenStreamSingleUpdate)
{
    for (const Vector& v : golden)
    {
        const std::vector<unsigned char> m = ptn(v.len);
        EXPECT_TRUE(streamsMatch(m.data(), v.len, {}, v.digest)) << "len " << v.len;
    }
}

TEST(TestCoreK12, StreamEmptyUpdates)
{
    const std::vector<unsigned char> m = ptn(1);
    // length zero, fed as explicit zero-length updates
    EXPECT_TRUE(streamsMatch(m.data(), 0, {0}, golden[0].digest));
    EXPECT_TRUE(streamsMatch(m.data(), 0, {0, 0, 0}, golden[0].digest));
    // length one, surrounded by zero-length updates
    EXPECT_TRUE(streamsMatch(m.data(), 1, {0, 1, 0}, golden[1].digest));
}

TEST(TestCoreK12, GoldenStreamByteAtATime)
{
    for (const Vector& v : golden)
    {
        if (v.len > 20000)
            continue;
        const std::vector<unsigned char> m = ptn(v.len);
        EXPECT_TRUE(streamsMatch(m.data(), v.len, std::vector<size_t>(v.len, 1), v.digest)) << "len " << v.len;
    }
}

TEST(TestCoreK12, GoldenStreamBoundaryCuts)
{
    for (const Vector& v : golden)
    {
        const std::vector<unsigned char> m = ptn(v.len);
        for (size_t cut : boundaryCuts)
        {
            if (cut >= v.len)
                continue;
            // update ending exactly at the boundary, followed by the rest
            EXPECT_TRUE(streamsMatch(m.data(), v.len, {cut}, v.digest)) << "len " << v.len << " cut " << cut;
            // same, with zero-length updates before, between and after
            EXPECT_TRUE(streamsMatch(m.data(), v.len, {0, cut, 0, v.len - cut, 0}, v.digest)) << "len " << v.len << " cut " << cut << " with empty updates";
            // two boundary cuts in a row
            if (2 * cut < v.len)
                EXPECT_TRUE(streamsMatch(m.data(), v.len, {cut, cut}, v.digest)) << "len " << v.len << " cut " << cut << " twice";
        }
        // last byte alone
        if (v.len > 1)
            EXPECT_TRUE(streamsMatch(m.data(), v.len, {v.len - 1}, v.digest)) << "len " << v.len << " last byte alone";
    }
}

TEST(TestCoreK12, GoldenStreamRandomSplits)
{
    for (const Vector& v : golden)
    {
        const std::vector<unsigned char> m = ptn(v.len);
        for (unsigned long long seed = 1; seed <= 5; ++seed)
        {
            EXPECT_TRUE(streamsMatch(m.data(), v.len, randomSplits(v.len, 20000, seed), v.digest)) << "len " << v.len << " seed " << seed;
            EXPECT_TRUE(streamsMatch(m.data(), v.len, randomSplits(v.len, 300, seed), v.digest)) << "len " << v.len << " small splits seed " << seed;
        }
    }
}

// Pseudo-random input, all three implementations, random split sizes: guards against any
// pattern-specific coincidence in the ptn-based goldens.
TEST(TestCoreK12, RandomInputAllImplementationsAgree)
{
    const size_t lens[] = {0, 1, 100, 167, 168, 169, 8191, 8192, 8193, 30000, 100000, 2097151, 2097152, 2097153};
    for (size_t len : lens)
    {
        const std::vector<unsigned char> m = pseudoRandom(len, 0xC0FFEE + len);
        const std::string expected = oneShot(m.data(), len);
        EXPECT_TRUE(streamsMatch(m.data(), len, {}, expected.c_str())) << "len " << len;
        EXPECT_TRUE(streamsMatch(m.data(), len, randomSplits(len, 5000, len + 1), expected.c_str())) << "len " << len << " split";
    }
}

TEST(TestCoreK12, Digest64To32MatchesOneShot)
{
    const std::vector<unsigned char> m = pseudoRandom(64 * 1000 + 63, 42);
    for (size_t i = 0; i < 1000; ++i)
    {
        unsigned char a[32];
        KangarooTwelve64To32(m.data() + i * 64, a);
        EXPECT_EQ(toHex(a, 32), oneShot(m.data() + i * 64, 64)) << "block " << i;
    }
    // unaligned input
    unsigned char a[32];
    KangarooTwelve64To32(m.data() + 3, a);
    EXPECT_EQ(toHex(a, 32), oneShot(m.data() + 3, 64));
    // all-zero and all-0xFF inputs
    unsigned char edge[64];
    setMem(edge, 64, 0);
    KangarooTwelve64To32(edge, a);
    EXPECT_EQ(toHex(a, 32), oneShot(edge, 64));
    setMem(edge, 64, 0xFF);
    KangarooTwelve64To32(edge, a);
    EXPECT_EQ(toHex(a, 32), oneShot(edge, 64));
}

// Pins the contract of the logging digest chain (logging.h reset()/updateTick()/log()):
//   digest[0] = K12(zeroHash || messages[0])
//   digest[t] = K12(digest[t-1] || messages[t])
// where messages[t] are the selected log messages of tick t fed by separate updates, and the
// stream is finalized once per tick and re-initialized with the previous digest.
// The chain itself is not compiled in NO_UEFI builds today (LOG_STATE_DIGEST forced to 0), so this
// test drives the same call sequence directly on the stream; logging_digest.cpp checks the wiring
// through qLogger itself.
TEST(TestCoreK12, LoggingDigestChainContract)
{
    const std::vector<unsigned char> pool = pseudoRandom(5000, 7);
    // per tick: list of (offset, size) messages; tick 2 has none
    const std::vector<std::vector<std::pair<size_t, size_t>>> ticks = {
        {{0, 72}, {72, 120}, {192, 41}},
        {{233, 1}, {234, 168}},
        {},
        {{402, 4000}},
    };

    KangarooTwelveStream stream;
    unsigned char prev[32];
    setMem(prev, 32, 0);
    stream.init();
    stream.update(prev, 32);

    for (size_t t = 0; t < ticks.size(); ++t)
    {
        std::vector<unsigned char> expectedInput(prev, prev + 32);
        for (const auto& msg : ticks[t])
        {
            stream.update(pool.data() + msg.first, msg.second);
            expectedInput.insert(expectedInput.end(), pool.data() + msg.first, pool.data() + msg.first + msg.second);
        }
        const std::string expected = oneShot(expectedInput.data(), expectedInput.size());

        unsigned char digest[32];
        stream.finalize(digest);
        EXPECT_EQ(toHex(digest, 32), expected) << "tick " << t;

        stream.init();
        stream.update(digest, 32);
        copyMem(prev, digest, 32);
    }
}

// Smoke-level timing of the two stream shapes the node produces per tick, for whichever K12
// configuration this binary was built with. One warmup, then equal repetitions per backend with the
// backend order rotated every repetition, minimum reported; results are validated outside the
// timed region.
// Caveat: in this binary every KangarooTwelveStream::update() carries the gtest expansion of
// ASSERT(!finalized), which the Release UEFI build does not have. The production-equivalent
// measurement is tools/k12_bench/k12_bench.cpp (single TU, no gtest, NDEBUG); use that for
// performance decisions.
//  - txBodyDigest: up to NUMBER_OF_TRANSACTIONS_PER_TICK updates of transaction size
//  - logging chain: many small updates
TEST(TestCoreK12, PerformanceStreamShapes)
{
    const std::vector<unsigned char> m = ptn(4096 * 1200);
    constexpr int repetitions = 7;

    auto measure = [&](size_t updates, size_t updateSize, const char* name)
    {
        const std::vector<size_t> splits(updates, updateSize);
        const size_t len = updates * updateSize;
        unsigned char outNative[32], outOneShot[32];

        auto runNative = [&] { streamNativeRaw(m.data(), len, splits, outNative); };
        auto runOneShot = [&] { KangarooTwelve(m.data(), (unsigned int)len, outOneShot, 32); };
        auto timeOnce = [](auto&& fn)
        {
            auto start = std::chrono::high_resolution_clock::now();
            fn();
            return (long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
        };

        runNative();
        runOneShot();
        long long best[2] = {-1, -1};
        for (int rep = 0; rep < repetitions; ++rep)
        {
            for (int k = 0; k < 2; ++k)
            {
                const int backend = (rep + k) % 2;
                const long long us = backend == 0 ? timeOnce(runNative) : timeOnce(runOneShot);
                if (best[backend] < 0 || us < best[backend])
                    best[backend] = us;
            }
        }

        std::cout << name << ": " << updates << " x " << updateSize << " B, min of " << repetitions << " runs [us]: "
                  << "stream " << best[0] << ", one-shot " << best[1] << std::endl;

        EXPECT_EQ(memcmp(outNative, outOneShot, 32), 0);
    };

    measure(4096, 1200, "txBodyDigest worst case");
    measure(4096, 100, "txBodyDigest small txs");
    measure(20000, 80, "logging chain");
}

TEST(TestCoreK12, PerformanceDigest32Of1GB)
{
    constexpr size_t bytesPerGigaByte = 1024 * 1024 * 1024;
    constexpr size_t repN = 1;
    constexpr size_t inputN = bytesPerGigaByte;
    constexpr size_t outputN = 32;

    char* inputPtr = new char[inputN];
    for (size_t i = 0; i < 100; ++i)
    {
        unsigned int pos, val;
        _rdrand32_step(&pos);
        _rdrand32_step(&val);
        inputPtr[pos % inputN] = val & 0xff;
    }
    char outputArray[outputN];

    auto startTime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < repN; ++i)
        KangarooTwelve((unsigned char *) inputPtr, inputN, (unsigned char*) outputArray, outputN);
    auto durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);

    double bytePerMilliSec = double(repN * inputN) / double(durationMilliSec.count());
    double gigaBytePerSec = bytePerMilliSec * (1000.0 / bytesPerGigaByte);
    std::cout << "K12 of 1 GB to 32 Byte digest: " << gigaBytePerSec << " GB/sec = " << 1.0 / gigaBytePerSec << " sec/GB" << std::endl;

    delete [] inputPtr;
}

TEST(TestCoreK12, CompareOneShotAndStream1GB)
{
    constexpr size_t bytesPerGigaByte = 1024 * 1024 * 1024;
    constexpr size_t inputN = bytesPerGigaByte;
    constexpr size_t outputN = 32;

    char* inputPtr = new char[inputN];
    for (size_t i = 0; i < 100; ++i)
    {
        unsigned int pos, val;
        _rdrand32_step(&pos);
        _rdrand32_step(&val);
        inputPtr[pos % inputN] = val & 0xff;
    }

    char outputArray[outputN];
    auto startTime = std::chrono::high_resolution_clock::now();
    KangarooTwelve((unsigned char *) inputPtr, inputN, (unsigned char*) outputArray, outputN);
    auto durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);
    double gigaBytePerSec = double(inputN) / double(durationMilliSec.count()) * (1000.0 / bytesPerGigaByte);
    std::cout << "K12 of 1 GB to 32 Byte digest via one-shot: " << gigaBytePerSec << " GB/sec, digest " << toHex((const unsigned char*)outputArray, outputN) << std::endl;

    char outputArrayStream[outputN];
    startTime = std::chrono::high_resolution_clock::now();
    streamNativeRaw((unsigned char *) inputPtr, inputN, {}, (unsigned char*) outputArrayStream);
    durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);
    gigaBytePerSec = double(inputN) / double(durationMilliSec.count()) * (1000.0 / bytesPerGigaByte);
    std::cout << "K12 of 1 GB to 32 Byte digest via stream: " << gigaBytePerSec << " GB/sec" << std::endl;
    ASSERT_EQ(memcmp(outputArrayStream, outputArray, outputN), 0);

    delete [] inputPtr;
}
