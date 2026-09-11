#define NO_UEFI

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

#include "logging_test.h"

#include <cstddef>
#include <string>
#include <vector>


// Wiring test of the per-tick log state digest chain in qLogger:
//   digest[t] = K12(digest[t-1] || selected log messages of tick t), digest[-1] = zero hash,
// with the stream finalized once per tick and re-initialized with the previous digest.

namespace
{
    std::string toHex(const unsigned char* d, size_t n)
    {
        static const char* digits = "0123456789ABCDEF";
        std::string s;
        for (size_t i = 0; i < n; ++i)
        {
            s.push_back(digits[d[i] >> 4]);
            s.push_back(digits[d[i] & 15]);
        }
        return s;
    }

    QuTransfer transfer(unsigned char tag, long long amount)
    {
        QuTransfer t;
        setMem(&t, sizeof(t), 0);
        t.sourcePublicKey.m256i_u8[0] = tag;
        t.destinationPublicKey.m256i_u8[1] = tag;
        t.amount = amount;
        return t;
    }

    // bytes that logMessage() feeds into the digest stream for one QuTransfer
    void append(std::vector<unsigned char>& v, const QuTransfer& t)
    {
        const unsigned char* p = (const unsigned char*)&t;
        v.insert(v.end(), p, p + offsetof(QuTransfer, _terminator));
    }
}

TEST(TestCoreLogging, StateDigestChain)
{
    LoggingTest loggingInit;
    const unsigned int tickBegin = 15700000;
    system.epoch = 200;
    qLogger::reset(tickBegin);

    // messages per tick; tick 2 has none
    const std::vector<std::vector<QuTransfer>> ticks = {
        {transfer(1, 100), transfer(2, 200), transfer(3, 300)},
        {transfer(4, 400)},
        {},
        {transfer(5, 500), transfer(6, 600)},
    };

    m256i prev = m256i::zero();
    for (size_t i = 0; i < ticks.size(); ++i)
    {
        const unsigned int tick = tickBegin + (unsigned int)i;
        system.tick = tick;
        logger.registerNewTx(tick, logger.SC_BEGIN_TICK_TX); // logs need a registered tx context
        std::vector<unsigned char> expectedInput(prev.m256i_u8, prev.m256i_u8 + 32);
        for (const QuTransfer& t : ticks[i])
        {
            logger.logQuTransfer(t);
            append(expectedInput, t);
        }
        qLogger::updateTick(tick);

        m256i expected;
        KangarooTwelve(expectedInput.data(), (unsigned int)expectedInput.size(), &expected, 32);
        EXPECT_EQ(toHex(qLogger::getStateDigest(tick).m256i_u8, 32), toHex(expected.m256i_u8, 32)) << "tick " << tick;
        prev = qLogger::getStateDigest(tick);
    }

    // a new epoch restarts the chain from the zero hash
    qLogger::reset(tickBegin + 100);
    system.tick = tickBegin + 100;
    logger.registerNewTx(tickBegin + 100, logger.SC_BEGIN_TICK_TX);
    logger.logQuTransfer(transfer(7, 700));
    qLogger::updateTick(tickBegin + 100);
    std::vector<unsigned char> input(32, 0);
    append(input, transfer(7, 700));
    m256i expected;
    KangarooTwelve(input.data(), (unsigned int)input.size(), &expected, 32);
    EXPECT_EQ(toHex(qLogger::getStateDigest(tickBegin + 100).m256i_u8, 32), toHex(expected.m256i_u8, 32));
}

// The stream is persisted in logEventState.db in the layout of the XKCP instance used before
// KangarooTwelveStream. A stream must survive the round trip through that layout at any point of
// its state machine and continue to the same digest.
TEST(TestCoreLogging, StateDigestStreamFileLayoutRoundTrip)
{
    std::vector<unsigned char> m(3 * 8192 + 500);
    for (size_t i = 0; i < m.size(); ++i)
        m[i] = (unsigned char)(i % 251);
    unsigned char reference[32];
    KangarooTwelve(m.data(), (unsigned int)m.size(), reference, 32);

    const size_t cuts[] = {0, 1, 167, 168, 169, 8191, 8192, 8193, 8360, 16384, 16385, 3 * 8192, m.size()};
    for (size_t cut : cuts)
    {
        KangarooTwelveStream s;
        s.init();
        s.update(m.data(), cut);

        K12StreamFileLayout file;
        k12StreamToFileLayout(s, file);
        EXPECT_EQ(file.finalNode.rateInBits, 1344u);
        EXPECT_EQ(file.securityLevel, 128);
        EXPECT_EQ(file.phase, 0);

        KangarooTwelveStream restored;
        k12StreamFromFileLayout(file, restored);
        restored.update(m.data() + cut, m.size() - cut);
        unsigned char out[32];
        restored.finalize(out);
        EXPECT_EQ(toHex(out, 32), toHex(reference, 32)) << "cut " << cut;
    }
}
