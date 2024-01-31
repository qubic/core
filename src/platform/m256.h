#pragma once

#include <intrin.h>

union m256i
{
    __int8              m256i_i8[32];
    __int16             m256i_i16[16];
    __int32             m256i_i32[8];
    __int64             m256i_i64[4];
    unsigned __int8     m256i_u8[32];
    unsigned __int16    m256i_u16[16];
    unsigned __int32    m256i_u32[8];
    unsigned __int64    m256i_u64[4];

    m256i() = default;

    m256i(unsigned long long ull0, unsigned long long ull1, unsigned long long ull2, unsigned long long ull3)
    {
        m256i_u64[0] = ull0;
        m256i_u64[1] = ull1;
        m256i_u64[2] = ull2;
        m256i_u64[3] = ull3;
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
