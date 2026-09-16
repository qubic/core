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
