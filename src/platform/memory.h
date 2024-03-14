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
