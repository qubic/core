#pragma once

// Ternary storage: values {0,1,2} at 2 bits each.
//
// Qubic's mining networks store their genome as trits 
// One byte per trit wastes six bits of eight; two bits per trit cuts the stored genome to a quarter.
// Callers hash and transmit the unpacked bytes

namespace score_engine
{

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

}
