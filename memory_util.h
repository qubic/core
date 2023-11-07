#pragma once

#ifdef NO_UEFI

#include <cstring>
typedef unsigned short CHAR16;

static inline void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    memset(buffer, value, size);
}

static inline void copyMem(void* destination, void* source, unsigned long long length)
{
    memcpy(destination, source, length);
}

#else
#include "uefi.h"

static inline void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    bs->SetMem(buffer, size, value);
}

static inline void copyMem(void* destination, void* source, unsigned long long length)
{
    bs->CopyMem(destination, source, length);
}

#endif
