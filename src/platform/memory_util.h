#pragma once

#include "console_logging.h"
#include "platform/uefi.h"
#include "memory.h"

#if defined(NO_UEFI)

#include <cstdlib>
#include <cstdbool>
#include <cstdio>

static bool allocPoolWithErrorLog(const wchar_t* name, const unsigned long long size, void** buffer) 
{
    *buffer = malloc(size);
    if (*buffer == nullptr)
    {
        printf("Memory allocation failed for %ls\n", name);
        return false;
    }
    return true;
}

#else

static bool allocPoolWithErrorLog(const CHAR16* name, const unsigned long long size, void** buffer)
    {
    EFI_STATUS status;
    CHAR16 message[512];
    constexpr EFI_MEMORY_TYPE poolType = EfiRuntimeServicesData;
    status = bs->AllocatePool(poolType, size, buffer);
    if (status != EFI_SUCCESS)
    {
        setText(message, L"EFI_BOOT_SERVICES.AllocatePool() fails for ");
        appendText(message, name);
        appendText(message, L" with size ");
        appendNumber(message, size, TRUE);
        logStatusAndMemInfoToConsole(message, status, __LINE__, size);
        return false;
    }
#if !defined(NDEBUG)
    else {
        setText(message, L"EFI_BOOT_SERVICES.AllocatePool() completed for ");
        appendText(message, name);
        appendText(message, L" with size ");
        appendNumber(message, size, TRUE);
        logStatusAndMemInfoToConsole(message, status, __LINE__, size);
            }
#endif
    return true;
}

#endif
