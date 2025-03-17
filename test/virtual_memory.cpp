#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/network_messages/tick.h"
#include "../src/public_settings.h"
#include "../src/platform/virtual_memory.h"

#include <random>

TEST(TestVirtualMemory, TestVirtualMemory_NativeChar) {
    initFilesystem();
    registerAsynFileIO(NULL);
    const unsigned long long name_u64 = 123456789;
    const unsigned long long pageDir = 0;
    VirtualMemory<char, name_u64, pageDir, 1997, 128> test_vm;
    test_vm.init();
    std::vector<char> arr;
    // fill with random value
    const int N = 1000000;
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
    int stride = 1337;
    while (pos < N)
    {
        int s = pos;
        int e = std::min(pos + stride, N);
        int n_item = e - s;
        test_vm.appendMany(arr.data() + s, n_item);
        pos += stride;
    }
    std::vector<char> fetcher;
    
    for (int i = 0; i < 1024; i++)
    {
        int offset = rand() % (N/2);
        int test_len = rand() % (N - offset);
        fetcher.resize(test_len);
        test_vm.getMany(fetcher.data(), offset, test_len);
        EXPECT_TRUE(memcmp(fetcher.data(), arr.data() + offset, test_len) == 0);
    }
    
    for (int i = 0; i < 1024; i++)
    {
        int index = rand() % N;
        EXPECT_TRUE(test_vm[index] == arr[index]);
    }
    test_vm.deinit();
}

#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)
static_assert((RAND_MAX& (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");
uint64_t rand64(void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned)rand();
    }
    return r;
}

TEST(TestVirtualMemory, TestVirtualMemory_NativeU64) {
    initFilesystem();
    registerAsynFileIO(NULL);
    const unsigned long long name_u64 = 123456789;
    const unsigned long long pageDir = 0;
    VirtualMemory<unsigned long long, name_u64, pageDir, 2005, 128> test_vm;
    test_vm.init();
    std::vector<unsigned long long> arr;
    // fill with random value
    const int N = 1000000;
    arr.resize(N);
    srand(0);
    for (int i = 0; i < N; i++)
    {
        arr[i] = rand64();
    }
    // append to virtual memory via single append
    for (int i = 0; i < 113; i++)
    {
        test_vm.append(arr[i]);
    }
    int pos = 113;
    int stride = 1337;
    while (pos < N)
    {
        int s = pos;
        int e = std::min(pos + stride, N);
        int n_item = e - s;
        test_vm.appendMany(arr.data() + s, n_item);
        pos += stride;
    }
    std::vector<unsigned long long> fetcher;

    for (int i = 0; i < 1024; i++)
    {
        int offset = rand() % (N/2);
        int test_len = rand() % (N - offset);
        fetcher.resize(test_len);
        test_vm.getMany(fetcher.data(), offset, test_len);
        EXPECT_TRUE(memcmp(fetcher.data(), arr.data() + offset, test_len * sizeof(unsigned long long)) == 0);
    }
    for (int i = 0; i < 1024; i++)
    {
        int index = rand() % N;
        EXPECT_TRUE(test_vm[index] == arr[index]);
    }
    test_vm.deinit();
}

TickData randTick()
{
    TickData res;
    int* ptr = (int*)&res;
    int sz = sizeof(res);
    for (int i = 0; i < sz/4; i++) {
        ptr[i] = rand();
    }
    return res;
}

bool tickEqual(const TickData a, const TickData b)
{
    return memcmp(&a, &b, sizeof(TickData)) == 0;
}

TEST(TestVirtualMemory, TestVirtualMemory_TickStruct) {
    initFilesystem();
    registerAsynFileIO(NULL);
    const unsigned long long name_u64 = 123456789;
    const unsigned long long pageDir = 0;
    VirtualMemory<TickData, name_u64, pageDir, 2005, 128> test_vm;
    test_vm.init();
    std::vector<TickData> arr;
    // fill with random value
    const int N = 1000;
    arr.resize(N);
    srand(0);
    for (int i = 0; i < N; i++)
    {
        arr[i] = randTick();
    }
    // append to virtual memory via single append
    for (int i = 0; i < 113; i++)
    {
        test_vm.append(arr[i]);
    }
    int pos = 113;
    int stride = 1337;
    while (pos < N)
    {
        int s = pos;
        int e = std::min(pos + stride, N);
        int n_item = e - s;
        test_vm.appendMany(arr.data() + s, n_item);
        pos += stride;
    }
    std::vector<TickData> fetcher;

    for (int i = 0; i < 1024; i++)
    {
        int offset = rand() % (N/2);
        int test_len = rand() % (N - offset);
        fetcher.resize(test_len);
        test_vm.getMany(fetcher.data(), offset, test_len);
        EXPECT_TRUE(memcmp(fetcher.data(), arr.data() + offset, test_len * sizeof(TickData)) == 0);
    }
    for (int i = 0; i < 1024; i++)
    {
        int index = rand() % N;
        EXPECT_TRUE(tickEqual(test_vm[index], arr[index]));
    }
    test_vm.deinit();
}