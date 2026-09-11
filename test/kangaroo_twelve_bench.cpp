// Timed K12 entry points for the performance tests in kangaroo_twelve.cpp.
// Deliberately a separate translation unit without gtest and with NDEBUG defined, so that the
// ASSERT() calls inside KangarooTwelveStream compile to nothing, exactly as in the Release UEFI build.
// Do not include gtest here.
#define NO_UEFI
#ifndef NDEBUG
#define NDEBUG
#endif

#include "../src/kangaroo_twelve.h"

void k12BenchOneShot(const unsigned char* data, size_t len, unsigned char out[32])
{
    KangarooTwelve(data, (unsigned int)len, out, 32);
}

void k12BenchNativeStream(const unsigned char* data, size_t len, const size_t* updates, size_t updateCount, unsigned char out[32])
{
    KangarooTwelveStream s;
    s.init();
    size_t pos = 0;
    for (size_t i = 0; i < updateCount; ++i)
    {
        s.update(data + pos, updates[i]);
        pos += updates[i];
    }
    if (pos < len)
        s.update(data + pos, len - pos);
    s.finalize(out);
}
