#include "console_logging.h"
#include "platform/uefi.h"

#if defined(NO_UEFI)

#include <cstdlib>
#include <cstdbool>
#include <cstdio>

static bool allocPoolWithErrorLog(const wchar_t* name, const unsigned long long size, void** buffer) 
{
    *buffer = malloc(size);
    if (*buffer == NULL) {
        printf("Memory allocation failed for %ls\n", name);
        return false;
    }
    return true;
}

#else

#include "memory.h"


static bool allocPoolWithErrorLog(const CHAR16* name, const unsigned long long size, void** buffer)
    {
    EFI_STATUS status;
    const EFI_MEMORY_TYPE poolType = EfiRuntimeServicesData;
    if (status = bs->AllocatePool(poolType, size, buffer))
    {
        setText(message, L"EFI_BOOT_SERVICES.AllocatePool() fails for ");
        appendText(message, name);
        appendText(message, L" with size ");
        appendNumber(message, size, TRUE);
        logStatusAndMemInfoToConsole(message, status, __LINE__, size);
        return false;
    }
    return true;
}

#endif
