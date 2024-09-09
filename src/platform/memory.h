#pragma once

#ifdef NO_UEFI

void setMem(void* buffer, unsigned long long size, unsigned char value);

void copyMem(void* destination, const void* source, unsigned long long length);

bool allocatePool(unsigned long long size, void** buffer);

void freePool(void* buffer);

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


// Function to get the current free RAM size
unsigned long long GetFreeRAMSize()
{
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


// Function to get the largest free consecutive memory block size
unsigned long long GetLargestFreeConsecutiveMemory()
{
    unsigned long long MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = nullptr;
    unsigned long long MapKey = 0;
    unsigned long long DescriptorSize = 0;
    unsigned int DescriptorVersion = 0;

    // First call to get the size of the memory map
    EFI_STATUS status = bs->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (status == EFI_BUFFER_TOO_SMALL)
    {
        // Allocate memory for the memory map
        if (!allocatePool(MemoryMapSize, (void**)&MemoryMap))
        {
            return 0;
        }

        // Second call to get the actual memory map
        status = bs->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    }

    if (status != EFI_SUCCESS)
    {
        if (MemoryMap)
        {
            freePool(MemoryMap);
        }
        return 0;
    }

    unsigned long long largestFreeBlock = 0;

    // Calculate the largest free consecutive memory block size
    unsigned long long count = MemoryMapSize / DescriptorSize;
    for (unsigned int i = 0; i < count; ++i)
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((char*)MemoryMap + (i * DescriptorSize));

        if (desc->Type == EfiConventionalMemory)
        {
            unsigned long long blockSize = desc->NumberOfPages * 4096; // Each page is 4KB
            if (blockSize > largestFreeBlock)
            {
                largestFreeBlock = blockSize;
            }
        }
    }

    freePool(MemoryMap);
    return largestFreeBlock;
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