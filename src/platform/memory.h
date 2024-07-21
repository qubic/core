#pragma once

#ifdef NO_UEFI

#include <cstring>
#include <cstdlib>

static inline void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    memset(buffer, value, size);
}

static inline void copyMem(void* destination, const void* source, unsigned long long length)
{
    memcpy(destination, source, length);
}

static inline bool allocatePool(unsigned long long size, void** buffer)
{
    void* ptr = malloc(size);
    if (ptr)
    {
        *buffer = ptr;
        return true;
    }
    return false;
}

static inline void freePool(void* buffer)
{
    free(buffer);
}

#else

#include "uefi.h"

static inline void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    bs->SetMem(buffer, size, value);
}

static inline void copyMem(void* destination, const void* source, unsigned long long length)
{
    bs->CopyMem(destination, (void*)source, length);
}

static inline bool allocatePool(unsigned long long size, void** buffer)
{
    return bs->AllocatePool(EfiRuntimeServicesData, size, buffer) == EFI_SUCCESS;
}

static inline void freePool(void* buffer)
{
    bs->FreePool(buffer);
}

#endif

// This should to be optimized if used in non-debugging context (using unsigned long long comparison as much as possible)
static inline bool isZero(const void* ptr, unsigned long long size)
{
    const char* cPtr = (const char*)ptr;
    for (unsigned long long i = 0; i < size; ++i)
    {
        if (cPtr[i] != 0)
            return false;
    }
    return true;
}


// Function to get the current free RAM size
unsigned long long GetFreeRAMSize() {
    unsigned long long MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = nullptr;
    unsigned long long MapKey = 0;
    unsigned long long DescriptorSize = 0;
    unsigned int DescriptorVersion = 0;

    // First call to get the size of the memory map
    EFI_STATUS status = bs->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (status == EFI_BUFFER_TOO_SMALL)
    {
        if (!allocatePool(MemoryMapSize, (void**)&MemoryMap))
        {
            return 0;
        }

        status = bs->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    }

    if (status != EFI_SUCCESS)
    {
        if (NULL != MemoryMap)
        {
            freePool(MemoryMap);
        }
        return 0;
    }

    unsigned long long freeRAM = 0;

    unsigned long long count = MemoryMapSize / DescriptorSize;
    for (unsigned int i = 0; i < count; ++i)
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((char*)MemoryMap + (i * DescriptorSize));

        if (desc->Type == EfiConventionalMemory)
        {
            freeRAM += desc->NumberOfPages * 4096; // Each page is 4KB
        }
    }

    freePool(MemoryMap);
    return freeRAM;
}