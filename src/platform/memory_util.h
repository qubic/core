#pragma once

#include "console_logging.h"
#include <lib/platform_efi/uefi.h>
#include "memory.h"

#ifdef NO_UEFI

#include <cstdlib>
#include <cstdbool>
#include <cstdio>

inline void* qVirtualAlloc(const unsigned long long size, bool commitMem);
inline void* qVirtualCommit(void* address, const unsigned long long size);
inline bool qVirtualDecommit(void* address, const unsigned long long size);

// useVirtualMem indicates whether to use VirtualAlloc or malloc
// commitMem indicates whether to commit memory when using VirtualAlloc
// NOTE: commitMem only used if host machine have enough RAM+Pagefile, otherwise VirtualAlloc will fail
static bool allocPoolWithErrorLog(const wchar_t* name, const unsigned long long size, void** buffer, const int LINE, bool useVirtualMem = false, bool commitMem = false)
{
    static unsigned long long totalMemoryUsed = 0;
    if (useVirtualMem) {
		*buffer = qVirtualAlloc(size, commitMem);
    }
    else {
        *buffer = malloc(size);
    }

    if (*buffer == nullptr)
    {
        printf("Memory allocation failed for %ls on line %u\n", name, LINE);
        return false;
    }

    // Zero out allocated memory
    if(!useVirtualMem)
     setMem(*buffer, size, 0);

    //totalMemoryUsed += size;
    //setText(message, L"Memory allocated ");
    //appendNumber(message, size / 1048576, TRUE);
    //appendText(message, L" MiB for ");
    //appendText(message, name);
    //appendText(message, L"| Total memory used: ");
    //appendNumber(message, totalMemoryUsed / 1048576, TRUE);
    //appendText(message, L" MiB.");
    //logToConsole(message);
    return true;
}

#else

static bool allocPoolWithErrorLog(const CHAR16* name, const unsigned long long size, void** buffer, const int LINE, bool needZerOut = true)
    {
    EFI_STATUS status;
    CHAR16 message[512];
    constexpr EFI_MEMORY_TYPE poolType = EfiRuntimeServicesData;

    // Check for invalid input
    if (buffer == nullptr || size == 0) {
        logStatusAndMemInfoToConsole(L"Invalid buffer pointer or size", EFI_INVALID_PARAMETER, __LINE__, size);
        return false;
    }

    status = bs->AllocatePool(poolType, size, buffer);
    if (status != EFI_SUCCESS)
    {
        setText(message, L"EFI_BOOT_SERVICES.AllocatePool() fails for ");
        appendText(message, name);
        appendText(message, L" with size ");
        appendNumber(message, size, TRUE);
        logStatusAndMemInfoToConsole(message, status, LINE, size);
        return false;
    }
#ifndef NDEBUG
    else {
        setText(message, L"EFI_BOOT_SERVICES.AllocatePool() completed for ");
        appendText(message, name);
        appendText(message, L" with size ");
        appendNumber(message, size, TRUE);
        logStatusAndMemInfoToConsole(message, status, LINE, size);
            }
#endif
    // Zero out allocated memory
    if (*buffer != nullptr && needZerOut) {
        setMem(*buffer, size, 0);
    }
    return true;
}

#endif
