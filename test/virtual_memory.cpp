#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/public_settings.h"
#include "../src/platform/virtual_memory.h"

#include <random>

TEST(TestVirtualMemory, TestVirtualMemory_NativeChar) {
    initFilesystem();
    registerAsynFileIO(NULL);
    //template <typename T, unsigned long long prefixName, CHAR16 ramDir[16], unsigned long long pageCapacity = 100000, unsigned long long numCachePage = 128>
    //class VirtualMemory
    const unsigned long long name_u64 = 123456789;
    const unsigned long long pageDir = 0;
    VirtualMemory<char, name_u64, pageDir, 1024, 128> test_vm;
    test_vm.init();
    std::vector<char> arr;
    // fill with random value
    const int N = 123456;
    arr.resize(N);
    srand(0);
    for (int i = 0; i < N; i++)
    {
        arr[i] = int(rand() % 256) - 127;
    }
    // append to virtual memory via single append
    for (int i = 0; i < 113; i++)
    {
        test_vm.append(arr[i]);
    }
    int pos = 113;
    int stride = 1234;
    while (pos < N)
    {
        int s = pos;
        int e = std::min(pos + stride, N);
        int n_item = e - s;
        test_vm.appendMany(arr.data() + s, n_item);
        pos += stride;
    }
}