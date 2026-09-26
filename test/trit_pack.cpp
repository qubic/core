#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/mining/trit_pack.h"

// Trit packing is a STORAGE format: the ant colony's ANN pool is packed in memory and written to
// disk that way, so a layout change silently breaks snapshot loads rather than failing to compile.
// The layout is therefore pinned by golden values here, not just round-tripped.
//
// trit_pack.h includes nothing, so this file does too - if that ever stops being true the test
// stops building and the header has quietly grown a dependency.

using score_engine::PackedTrits;

// Two groups of five trits is small enough that every value the structure can hold fits in a loop,
// so this is exhaustive rather than a sample: 3^10 assignments, each packed and unpacked.
TEST(TestTritPack, RoundTripsEveryPossibleValue)
{
    using P = PackedTrits<2, 5>;
    static constexpr unsigned int COMBINATIONS = 59049;   // 3^10

    for (unsigned int v = 0; v < COMBINATIONS; v++)
    {
        unsigned char src[P::tritCount];
        unsigned int rest = v;
        for (unsigned long long i = 0; i < P::tritCount; i++)
        {
            src[i] = (unsigned char)(rest % 3);
            rest /= 3;
        }

        P packed;
        packed.pack(src);

        unsigned char back[P::tritCount];
        for (unsigned long long i = 0; i < P::tritCount; i++)
        {
            back[i] = 0xFF;   // so a trit the unpack never writes fails loudly
        }
        packed.unpack(back);

        for (unsigned long long i = 0; i < P::tritCount; i++)
        {
            ASSERT_EQ(back[i], src[i]) << "value " << v << ", trit " << i;
        }
    }
}

// The documented layout - trit i of group g at bits [2i, 2i+2), lowest index in the lowest bits.
// Round-trip tests pass under any self-consistent layout, so only fixed words catch a reordering
// that would leave existing snapshot files unreadable.
TEST(TestTritPack, LayoutIsTwoBitsPerTritLowestIndexFirst)
{
    PackedTrits<2, 4> packed;
    const unsigned char src[8] = { 1, 2, 0, 1,   2, 2, 1, 0 };
    packed.pack(src);

    EXPECT_EQ(packed.word[0], 1ull + (2ull << 2) + (0ull << 4) + (1ull << 6));   // 73
    EXPECT_EQ(packed.word[1], 2ull + (2ull << 2) + (1ull << 4) + (0ull << 6));   // 26
}

// A group is one scorer row and gets its own word, so editing a row must not touch any other. This
// is what lets the colony reason about a neuron's LUT independently.
TEST(TestTritPack, GroupsAreIndependent)
{
    using P = PackedTrits<4, 6>;
    unsigned char src[P::tritCount];
    for (unsigned long long i = 0; i < P::tritCount; i++)
    {
        src[i] = 0;
    }

    P base;
    base.pack(src);

    for (unsigned long long g = 0; g < P::groupCount; g++)
    {
        src[g * P::tritsPerGroup] = 2;
        P edited;
        edited.pack(src);
        src[g * P::tritsPerGroup] = 0;

        for (unsigned long long m = 0; m < P::groupCount; m++)
        {
            if (m == g)
            {
                ASSERT_NE(edited.word[m], base.word[m]) << "group " << m << " should have changed";
            }
            else
            {
                ASSERT_EQ(edited.word[m], base.word[m]) << "group " << m << " must not change";
            }
        }
    }
}

// 32 trits is the widest group a uint64 holds, so the top trit sits at bits 62-63. A shift written
// on a 32-bit type would be undefined there and would typically lose the high half silently.
TEST(TestTritPack, WidestLegalGroupRoundTrips)
{
    using P = PackedTrits<1, 32>;
    unsigned char src[P::tritCount];
    for (unsigned long long i = 0; i < P::tritCount; i++)
    {
        src[i] = 2;
    }

    P packed;
    packed.pack(src);
    EXPECT_EQ(packed.word[0], 0xAAAAAAAAAAAAAAAAull);   // every trit 0b10

    unsigned char back[P::tritCount];
    packed.unpack(back);
    for (unsigned long long i = 0; i < P::tritCount; i++)
    {
        ASSERT_EQ(back[i], 2) << "trit " << i;
    }
}

// pack() masks instead of validating. A byte the scorer should never have written is truncated to
// its low two bits and stays inside its own trit - it does not shift the ones after it, which is
// the property that keeps one bad byte from corrupting a whole row.
TEST(TestTritPack, OutOfRangeByteCannotDisturbItsNeighbours)
{
    PackedTrits<1, 4> packed;
    const unsigned char src[4] = { 4, 1, 7, 2 };   // 4 -> 0, 7 -> 3
    packed.pack(src);

    unsigned char back[4];
    packed.unpack(back);

    EXPECT_EQ(back[0], 0);
    EXPECT_EQ(back[1], 1);
    EXPECT_EQ(back[2], 3);
    EXPECT_EQ(back[3], 2);
}

// ---------------------------------------------------------------------------------------------
// PackedBase3: the start state and the LUTs, five trits to a byte

using score_engine::PackedBase3;

// The documented layout - t0 + 3*t1 + 9*t2 + 27*t3 + 81*t4, the same encoding the task file uses.
// Fixed bytes, so a reordering that still round-trips is still caught.
TEST(TestPackedBase3, LayoutIsFiveTritsPerByteLowestIndexFirst)
{
    PackedBase3<10> packed;
    const unsigned char src[10] = { 1, 2, 0, 1, 2,   2, 0, 0, 1, 1 };
    packed.pack(src);

    EXPECT_EQ(packed.byteCount, 2u);
    EXPECT_EQ(packed.data[0], 1 + 3 * 2 + 9 * 0 + 27 * 1 + 81 * 2);   // 196
    EXPECT_EQ(packed.data[1], 2 + 3 * 0 + 9 * 0 + 27 * 1 + 81 * 1);   // 110
}

// Five trits is 3^5 = 243 values, so a packed byte is never 243 or above. That is what leaves room
// for the task file's own validity check on the same encoding.
TEST(TestPackedBase3, EveryByteStaysBelowTheBaseLimit)
{
    using P = PackedBase3<5>;
    unsigned char src[5];
    for (unsigned int v = 0; v < 243; v++)
    {
        unsigned int rest = v;
        for (unsigned long long i = 0; i < 5; i++)
        {
            src[i] = (unsigned char)(rest % 3);
            rest /= 3;
        }
        P packed;
        packed.pack(src);
        ASSERT_LT(packed.data[0], 243) << "value " << v;

        unsigned char back[5];
        packed.unpack(back);
        for (unsigned long long i = 0; i < 5; i++)
        {
            ASSERT_EQ(back[i], src[i]) << "value " << v << ", trit " << i;
        }
    }
}

// A count that is not a multiple of five leaves unused positions in the last byte. They are packed
// as zero and skipped on the way out, so the tail of the caller's buffer is never written.
TEST(TestPackedBase3, RaggedTailIsZeroPaddedAndNotWrittenBack)
{
    using P = PackedBase3<7>;
    EXPECT_EQ(P::byteCount, 2u);

    const unsigned char src[7] = { 2, 1, 0, 2, 2,   1, 0 };
    P packed;
    packed.pack(src);
    EXPECT_EQ(packed.data[1], 1 + 3 * 0);   // the three unused positions contribute nothing

    unsigned char back[8];
    for (unsigned long long i = 0; i < 8; i++)
    {
        back[i] = 0xFF;
    }
    packed.unpack(back);
    for (unsigned long long i = 0; i < 7; i++)
    {
        ASSERT_EQ(back[i], src[i]) << "trit " << i;
    }
    EXPECT_EQ(back[7], 0xFF) << "unpack wrote past the trit count";
}

// ---------------------------------------------------------------------------------------------
// PackedIndices: the wiring, at the width the population needs

using score_engine::PackedIndices;
using score_engine::bitsToIndex;

// Indices run 0 .. count-1, so a power of two needs exactly its own exponent and one more value
// needs a further bit.
TEST(TestPackedIndices, IndexWidthCoversTheWholeRange)
{
    EXPECT_EQ(bitsToIndex(16), 4u);
    EXPECT_EQ(bitsToIndex(1024), 10u);
    EXPECT_EQ(bitsToIndex(1025), 11u);
    EXPECT_EQ(bitsToIndex(2048), 11u);
    EXPECT_EQ(bitsToIndex(1000), 10u);
}

// The documented layout - index i occupies bits [i*BITS, (i+1)*BITS), least significant bit first,
// running across byte boundaries with no padding between indices.
TEST(TestPackedIndices, LayoutIsLittleEndianAndUnpadded)
{
    PackedIndices<3, 11> packed;
    const unsigned short src[3] = { 0x7FF, 0, 1 };
    packed.pack(src);

    EXPECT_EQ(packed.byteCount, 5u);   // 33 bits
    EXPECT_EQ(packed.data[0], 0xFF);   // first index fills bits 0-10
    EXPECT_EQ(packed.data[1], 0x07);
    EXPECT_EQ(packed.data[2], 0x40);   // second index is zero; third starts at bit 22
    EXPECT_EQ(packed.data[3], 0x00);
    EXPECT_EQ(packed.data[4], 0x00);
}

// Every index at the production width, including both ends of the range, must survive the trip.
TEST(TestPackedIndices, RoundTripsBothEndsOfTheRange)
{
    using P = PackedIndices<64, 11>;
    unsigned short src[P::indexCount];
    for (unsigned long long i = 0; i < P::indexCount; i++)
    {
        src[i] = (unsigned short)((i % 2) ? 2047 : (unsigned short)(i * 31 % 2048));
    }

    P packed;
    packed.pack(src);

    unsigned short back[P::indexCount];
    for (unsigned long long i = 0; i < P::indexCount; i++)
    {
        back[i] = 0xFFFF;
    }
    packed.unpack(back);
    for (unsigned long long i = 0; i < P::indexCount; i++)
    {
        ASSERT_EQ(back[i], src[i]) << "index " << i;
    }
}

// pack() starts by clearing, so packing into a reused buffer cannot leave bits of the old contents
// behind in positions the new indices do not set.
TEST(TestPackedIndices, RepackingClearsTheOldContents)
{
    PackedIndices<8, 11> packed;
    unsigned short all[8];
    for (unsigned long long i = 0; i < 8; i++)
    {
        all[i] = 2047;
    }
    packed.pack(all);

    unsigned short none[8];
    for (unsigned long long i = 0; i < 8; i++)
    {
        none[i] = 0;
    }
    packed.pack(none);

    for (unsigned long long b = 0; b < PackedIndices<8, 11>::byteCount; b++)
    {
        ASSERT_EQ(packed.data[b], 0) << "byte " << b;
    }
}
