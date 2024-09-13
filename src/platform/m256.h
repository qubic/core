#pragma once

#include <intrin.h>

// Used for all kinds of IDs, including in QPI and contracts.
// Existing interface and behavior should never be changed! (However, it may be extended.)
union m256i
{
    // access for loops and compatibility with __m256i
    __int8              m256i_i8[32];
    __int16             m256i_i16[16];
    __int32             m256i_i32[8];
    __int64             m256i_i64[4];
    unsigned __int8     m256i_u8[32];
    unsigned __int16    m256i_u16[16];
    unsigned __int32    m256i_u32[8];
    unsigned __int64    m256i_u64[4];

    // interface for QPI (no [] allowed)
    struct
    {
        unsigned __int64 _0, _1, _2, _3;
    } u64;
    struct
    {
        __int64 _0, _1, _2, _3;
    } i64;
    struct
    {
        unsigned __int32 _0, _1, _2, _3, _4, _5, _6, _7;
    } u32;
    struct
    {
        __int32 _0, _1, _2, _3, _4, _5, _6, _7;
    } i32;
    struct
    {
        unsigned __int16 _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15;
    } u16;
    struct
    {
        __int16 _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15;
    } i16;
    struct
    {
        unsigned __int8 _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15;
        unsigned __int8 _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31;
    } u8;
    struct
    {
        __int8 _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15;
        __int8 _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31;
    } i8;

    m256i() = default;

    m256i(unsigned long long ull0, unsigned long long ull1, unsigned long long ull2, unsigned long long ull3)
    {
        m256i_intr() = _mm256_set_epi64x(ull3, ull2, ull1, ull0);
    }

    m256i(const unsigned char value[32])
    {
        assign(*(m256i*)value);
    }

    m256i(const __m256i& value)
    {
        assign(*(m256i*)&value);
    }

    m256i(const m256i& value)
    {
        assign(value);
    }

    m256i(const volatile m256i& value)
    {
        assign(value);
    }

    m256i(m256i&& other) noexcept
    {
        assign(other);
    }

    m256i& operator=(const m256i& other)
    {
        assign(other);
        return *this;
    }

    volatile m256i& operator=(const m256i& other) volatile
    {
        assign(other);
        return *this;
    }

    m256i& operator=(const volatile m256i& other)
    {
        assign(other);
        return *this;
    }

    volatile m256i& operator=(const volatile m256i& other) volatile
    {
        assign(other);
        return *this;
    }

    m256i& operator=(m256i&& other) noexcept
    {
        assign(other);
        return *this;
    }

    void assign(const m256i& value) noexcept
    {
        // supports self-assignment
        _mm256_storeu_si256((__m256i*)this, _mm256_lddqu_si256((const __m256i*) & value));
    }

    volatile void assign(const m256i& value) volatile noexcept
    {
        // supports self-assignment
        _mm256_storeu_si256((__m256i*)this, _mm256_lddqu_si256((const __m256i*) & value));
    }

    void assign(const volatile m256i& value) noexcept
    {
        // supports self-assignment
        _mm256_storeu_si256((__m256i*)this, _mm256_lddqu_si256((const __m256i*) & value));
    }

    volatile void assign(volatile const m256i& value) volatile noexcept
    {
        // supports self-assignment
        _mm256_storeu_si256((__m256i*)this, _mm256_lddqu_si256((const __m256i*) & value));
    }

    __m256i& m256i_intr()
    {
        return *(__m256i*) this;
    }

    const __m256i& m256i_intr() const
    {
        return *(const __m256i*) this;
    }

    void setRandomValue()
    {
        _rdrand64_step(&m256i_u64[0]);
        _rdrand64_step(&m256i_u64[1]);
        _rdrand64_step(&m256i_u64[2]);
        _rdrand64_step(&m256i_u64[3]);
    }

    static m256i randomValue()
    {
        m256i ret;
        ret.setRandomValue();
        return ret;
    }

    inline static m256i zero()
    {
        return _mm256_setzero_si256();
    }
};

static_assert(sizeof(m256i) == 32, "m256 has unexpected size!");


static inline const __m256i& __m256i_convert(const __m256i& a)
{
    return a;
}

static inline const __m256i& __m256i_convert(const m256i& a)
{
    return *((__m256i*) & a);
}

static inline const __m256i& __m256i_convert(volatile const m256i& a)
{
    return *((__m256i*) & a);
}

static inline const __m256i& __m256i_convert(const unsigned char a[32])
{
    return *((__m256i*)a);
}

/*
static inline bool EQUAL(const __m256i& a, const __m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(a, b)) == 0xFFFFFFFF;
}
*/

#if 0
// Enable this for more flexibility regarding comparisons of m256 variants, but these general == and != operators sometimes lead to very misleading error messages.
template <typename TA, typename TB>
static inline bool operator==(const TA& a, const TB& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) == 0xFFFFFFFF;
}

template <typename TA, typename TB>
static inline bool operator!=(const TA& a, const TB& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) != 0xFFFFFFFF;
}

#else

static inline bool operator==(const m256i& a, const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) == 0xFFFFFFFF;
}

static inline bool operator!=(const m256i& a, const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) != 0xFFFFFFFF;
}

static inline bool operator==(const m256i& a, volatile const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) == 0xFFFFFFFF;
}

static inline bool operator!=(const m256i& a, volatile const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) != 0xFFFFFFFF;
}

static inline bool operator==(volatile const m256i& a, const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) == 0xFFFFFFFF;
}

static inline bool operator!=(volatile const m256i& a, const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) != 0xFFFFFFFF;
}

static inline bool operator==(volatile const m256i& a, volatile const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) == 0xFFFFFFFF;
}

static inline bool operator!=(volatile const m256i& a, volatile const m256i& b)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi64(__m256i_convert(a), __m256i_convert(b))) != 0xFFFFFFFF;
}
#endif

static inline bool operator<(const m256i& a, const m256i& b)
{
    // probably this can be done more efficiently, but it is only used in the testing code for now
    for (int i = 0; i < 4; ++i)
    {
        if (a.m256i_u64[i] < b.m256i_u64[i])
            return true;
        if (a.m256i_u64[i] > b.m256i_u64[i])
            return false;
    }
    return false;
}

static inline bool isZero(const __m256i& a)
{
    return _mm256_testz_si256(a, a) == 1;
}

template <typename T>
static inline bool isZero(const T& a)
{
    const __m256i& ac = __m256i_convert(a);
    return _mm256_testz_si256(ac, ac) == 1;
}
