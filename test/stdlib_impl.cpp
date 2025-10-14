// Implements NO_UEFI versions of several functions that require cstdlib functions.
// Having them in a separate cpp file avoids the name clash between cstdlib's system and Qubic's system.

#include <cstring>
#include <cstdlib>
#include <ctime>

#define NO_UEFI

#include "platform/time.h"

#ifdef __linux__
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/mman.h>
#include <map>
#else
#include <Windows.h>
#include <conio.h>
#include <map>
#endif

void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    memset(buffer, value, size);
}

void copyMem(void* destination, const void* source, unsigned long long length)
{
    memcpy(destination, source, length);
}

bool allocatePool(unsigned long long size, void** buffer)
{
    void* ptr = malloc(size);
    if (ptr)
    {
        *buffer = ptr;
        return true;
    }
    return false;
}

void freePool(void* buffer)
{
    free(buffer);
}

void updateTime()
{
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::gmtime(&t);
    utcTime.Year = tm->tm_year + 1900;
    utcTime.Month = tm->tm_mon + 1;
    utcTime.Day = tm->tm_mday;
    utcTime.Hour = tm->tm_hour;
    utcTime.Minute = tm->tm_min;
    utcTime.Second = tm->tm_sec;
    utcTime.Nanosecond = 0;
    utcTime.TimeZone = 0;
    utcTime.Daylight = 0;
}

unsigned long long now_ms()
{
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::gmtime(&t);
    return ms((unsigned char)(tm->tm_year % 100), tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, 0);
}

bool isMainProcessor() {
    return true;
}

inline std::map<unsigned long long, bool> commitMemMap;

#ifdef _MSC_VER
void* qVirtualAlloc(const unsigned long long size, bool commitMem = false) {
    void *addr = VirtualAlloc(NULL, (SIZE_T)size, MEM_RESERVE | (commitMem ? MEM_COMMIT : 0), PAGE_READWRITE);
    if (addr != nullptr)
    {
        commitMemMap[(unsigned long long)addr] = commitMem;
        return addr;
    }
    printf("CRITIAL: VirtualAlloc failed in qVirtualAlloc");
    return nullptr;
}

void* qVirtualCommit(void* address, const unsigned long long size) {
	return VirtualAlloc(address, (SIZE_T)size, MEM_COMMIT, PAGE_READWRITE);
}

bool qVirtualFreeAndRecommit(void* address, const unsigned long long size) {
    VirtualFree(address, (SIZE_T)size, MEM_DECOMMIT);
    bool commitMem = commitMemMap[(unsigned long long)address];
	if (!commitMem) {
		return true;
	}
    return VirtualAlloc(address, (SIZE_T)size, MEM_COMMIT, PAGE_READWRITE) != address;
}
#else
void* qVirtualAlloc(const unsigned long long size, bool commitMem = false) {
    int prot = commitMem ? (PROT_READ | PROT_WRITE) : PROT_NONE;
    void* addr = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr != MAP_FAILED)
    {
        commitMemMap[(unsigned long long)addr] = commitMem;
        return addr;
    }

    printf("CRITIAL: mmap failed in qVirtualAlloc");
    return nullptr;
}

void* qVirtualCommit(void* address, const unsigned long long size) {
    static long ps = sysconf(_SC_PAGESIZE);
    uintptr_t start = (uintptr_t)address & ~(ps - 1);
    uintptr_t end   = (uintptr_t)address + size;
    size_t aligned_len = end - start;
    aligned_len = (aligned_len + ps - 1) & ~(ps - 1);
    if (mprotect((void*)start, aligned_len, PROT_READ | PROT_WRITE) == 0)
    {
        return address;
    }

    printf("CRITIAL: mprotect failed in qVirtualCommit");
    return nullptr;
}

bool qVirtualFreeAndRecommit(void* address, const unsigned long long size) {
    bool commitMem = commitMemMap[(unsigned long long)address];
    int prot = commitMem ? (PROT_READ | PROT_WRITE) : PROT_NONE;
    return mmap(address, size, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == address;
}

#endif

unsigned long long mainThreadProcessorID = 1;