#define NO_UEFI

#include "../src/kangaroo_twelve.h"

#include "gtest/gtest.h"

#include <chrono>


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
        KangarooTwelve(inputPtr, inputN, outputArray, outputN);
    auto durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);

    double bytePerMilliSec = double(repN * inputN) / double(durationMilliSec.count());
    double gigaBytePerSec = bytePerMilliSec * (1000.0 / bytesPerGigaByte);
    std::cout << "K12 of 1 GB to 32 Byte digest: " << gigaBytePerSec << " GB/sec = " << 1.0 / gigaBytePerSec << " sec/GB" << std::endl;

    delete [] inputPtr;
}
