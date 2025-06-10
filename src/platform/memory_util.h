#pragma once

#include "console_logging.h"
#include <lib/platform_efi/uefi.h>
#include "memory.h"

#ifdef NO_UEFI

#include <cstdlib>
#include <cstdbool>
#include <cstdio>

static bool allocPoolWithErrorLog(const wchar_t* name, const unsigned long long size, void** buffer, const int LINE) 
{
    *buffer = malloc(size);
    if (*buffer == nullptr)
    {
        printf("Memory allocation failed for %ls on line %u\n", name, LINE);
        return false;
    }

    // Zero out allocated memory
    setMem(*buffer, size, 0);

    return true;
}

#else

static bool allocPoolWithErrorLog(const CHAR16* name, const unsigned long long size, void** buffer, const int LINE)
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
    if (*buffer != nullptr) {
        setMem(*buffer, size, 0);
    }
    return true;
}

#endif
