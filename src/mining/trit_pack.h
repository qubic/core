#pragma once

// Storage forms for the parts of an ANN. Callers hash and transmit the unpacked bytes; these types
// are only what the store and the replay cache hold.
//
// PackedTrits  - values {0,1,2} at 2 bits each, grouped into one uint64 per group.
// PackedBase3  - values {0,1,2} five to a byte in base 3, the same encoding the task file uses.
//                3^5 = 243 fits a byte, so a trit costs 1.6 bits instead of 2.
// PackedIndices- fixed-width indices at exactly the bits the population needs.

namespace score_engine
{

// Bits needed to address count values, so indices 0 .. count-1 all fit.
static constexpr unsigned int bitsToIndex(unsigned long long count)
{
    unsigned int bits = 0;
    while ((1ULL << bits) < count)
    {
        bits++;
    }
    return bits;
}

template<unsigned long long GROUPS, unsigned long long TRITS_PER_GROUP>
struct PackedTrits
{
    static_assert(GROUPS > 0, "need at least one group");
    static_assert(TRITS_PER_GROUP > 0, "a group needs at least one trit");
    static_assert(TRITS_PER_GROUP * 2 <= 64, "a group must fit at 2 bits per trit in one uint64");

    static constexpr unsigned long long groupCount = GROUPS;
    static constexpr unsigned long long tritsPerGroup = TRITS_PER_GROUP;
    static constexpr unsigned long long tritCount = GROUPS * TRITS_PER_GROUP;

    unsigned long long word[GROUPS];

    void pack(const unsigned char* src)
    {
        for (unsigned long long g = 0; g < GROUPS; g++)
        {
            unsigned long long packed = 0;
            for (unsigned long long i = 0; i < TRITS_PER_GROUP; i++)
            {
                packed |= ((unsigned long long)(src[g * TRITS_PER_GROUP + i] & 3u)) << (i * 2);
            }
            word[g] = packed;
        }
    }

    void unpack(unsigned char* dst) const
    {
        for (unsigned long long g = 0; g < GROUPS; g++)
        {
            const unsigned long long packed = word[g];
            for (unsigned long long i = 0; i < TRITS_PER_GROUP; i++)
            {
                dst[g * TRITS_PER_GROUP + i] = (unsigned char)((packed >> (i * 2)) & 3ull);
            }
        }
    }
};

// The five trits of every byte value. Built with the same repeated mod-3 the packing defines, so the
// 13 byte values above 242 - which only a corrupt file can hold - decode exactly as the arithmetic
// would and still yield trits.
struct Base3ByteTrits
{
    unsigned char trit[256][5];
};

inline constexpr Base3ByteTrits makeBase3ByteTrits()
{
    Base3ByteTrits table{};
    for (unsigned int v = 0; v < 256; v++)
    {
        unsigned int rest = v;
        for (unsigned int k = 0; k < 5; k++)
        {
            table.trit[v][k] = (unsigned char)(rest % 3);
            rest /= 3;
        }
    }
    return table;
}

inline constexpr Base3ByteTrits BASE3_BYTE_TRITS = makeBase3ByteTrits();

// Trits five to a byte in base 3: t0 + 3*t1 + 9*t2 + 27*t3 + 81*t4. Inputs are trits, so pack()
// reduces mod 3 and unpack() always yields 0, 1 or 2. Trailing positions in the last byte are zero.
template<unsigned long long TRITS>
struct PackedBase3
{
    static_assert(TRITS > 0, "need at least one trit");

    static constexpr unsigned long long TRIT_BASE = 3;
    static constexpr unsigned long long TRITS_PER_BYTE = 5;
    static constexpr unsigned long long tritCount = TRITS;
    static constexpr unsigned long long byteCount = (TRITS + TRITS_PER_BYTE - 1) / TRITS_PER_BYTE;

    unsigned char data[byteCount];

    void pack(const unsigned char* src)
    {
        for (unsigned long long b = 0; b < byteCount; b++)
        {
            unsigned int packed = 0;
            unsigned int weight = 1;
            for (unsigned long long k = 0; k < TRITS_PER_BYTE; k++)
            {
                const unsigned long long i = b * TRITS_PER_BYTE + k;
                const unsigned int t = (i < TRITS) ? (unsigned int)(src[i] % TRIT_BASE) : 0u;
                packed += t * weight;
                weight *= (unsigned int)TRIT_BASE;
            }
            data[b] = (unsigned char)packed;
        }
    }

    void unpack(unsigned char* dst) const
    {
        static constexpr unsigned long long wholeBytes = TRITS / TRITS_PER_BYTE;
        for (unsigned long long b = 0; b < wholeBytes; b++)
        {
            const unsigned char* const row = BASE3_BYTE_TRITS.trit[data[b]];
            unsigned char* const out = dst + b * TRITS_PER_BYTE;
            out[0] = row[0];
            out[1] = row[1];
            out[2] = row[2];
            out[3] = row[3];
            out[4] = row[4];
        }
        // The last byte when TRITS is not a multiple of five: only the positions that exist.
        const unsigned char* const row = BASE3_BYTE_TRITS.trit[data[byteCount - 1]];
        for (unsigned long long i = wholeBytes * TRITS_PER_BYTE; i < TRITS; i++)
        {
            dst[i] = row[i - wholeBytes * TRITS_PER_BYTE];
        }
    }
};

// COUNT indices of BITS bits each, packed little-endian end to end with no per-index padding.
template<unsigned long long COUNT, unsigned int BITS>
struct PackedIndices
{
    static_assert(COUNT > 0, "need at least one index");
    static_assert(BITS >= 1 && BITS <= 16, "index width must fit the unpacked unsigned short");

    static constexpr unsigned long long indexCount = COUNT;
    static constexpr unsigned int indexBits = BITS;
    static constexpr unsigned long long byteCount = (COUNT * BITS + 7) / 8;

    unsigned char data[byteCount];

    void pack(const unsigned short* src)
    {
        for (unsigned long long b = 0; b < byteCount; b++)
        {
            data[b] = 0;
        }
        for (unsigned long long i = 0; i < COUNT; i++)
        {
            const unsigned long long base = i * BITS;
            const unsigned int value = (unsigned int)src[i];
            for (unsigned int k = 0; k < BITS; k++)
            {
                if ((value >> k) & 1u)
                {
                    const unsigned long long bit = base + k;
                    data[bit >> 3] |= (unsigned char)(1u << (bit & 7));
                }
            }
        }
    }

    void unpack(unsigned short* dst) const
    {
        static constexpr unsigned int MASK = (1u << BITS) - 1u;
        // An index starts at most 7 bits into a byte and is at most 16 wide, so three bytes always
        // hold it. Indices that close to the end have no third byte to read and go one bit at a time.
        static constexpr unsigned long long windowed =
            (byteCount >= 3) ? (((byteCount - 3) * 8 + 7) / BITS + 1) : 0;
        static constexpr unsigned long long fastCount = (windowed < COUNT) ? windowed : COUNT;

        for (unsigned long long i = 0; i < fastCount; i++)
        {
            const unsigned long long base = i * BITS;
            const unsigned long long byteIndex = base >> 3;
            const unsigned int window = (unsigned int)data[byteIndex]
                | ((unsigned int)data[byteIndex + 1] << 8)
                | ((unsigned int)data[byteIndex + 2] << 16);
            dst[i] = (unsigned short)((window >> (base & 7)) & MASK);
        }
        for (unsigned long long i = fastCount; i < COUNT; i++)
        {
            const unsigned long long base = i * BITS;
            unsigned int value = 0;
            for (unsigned int k = 0; k < BITS; k++)
            {
                const unsigned long long bit = base + k;
                if ((data[bit >> 3] >> (bit & 7)) & 1u)
                {
                    value |= (1u << k);
                }
            }
            dst[i] = (unsigned short)value;
        }
    }
};

}
