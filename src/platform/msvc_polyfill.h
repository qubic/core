//
// Created by kali on 9/17/25.
//

#ifndef MSVC_POLYFILL_H
#define MSVC_POLYFILL_H
#include <stdint.h>

#ifdef _MSC_VER
  /* MSVC already provides _umul128 / _mul128 */
#else

/* Unsigned: returns low 64 bits; writes high 64 bits to *hi if hi != NULL */
template <typename T>
static inline T _umul128(T a, T b, T *hi) {
    __uint128_t r = (__uint128_t)a * (__uint128_t)b;
    if (hi) *hi = (T)(r >> (unsigned int)64);
    return (T)r;
}

/* Signed: returns low 64 bits; writes high 64 bits to *hi (signed high) if hi != NULL */
template <typename T>
static inline T _mul128(T a, T b, T *hi) {
    __int128 r = ( __int128)a * ( __int128)b;
    if (hi) *hi = (T)(r >> 64);
    return (T)r;
}
#endif /* _MSC_VER */

#endif //MSVC_POLYFILL_H
