#include "console_logging.h"

#if defined(NO_UEFI)

static bool allocPoolWithErrorLog(const EFI_MEMORY_TYPE poolType, const CHAR16* name, const unsigned long long size, void** buffer)

#else

#include "memory.h"


static bool allocPoolWithErrorLog(const EFI_MEMORY_TYPE poolType, const CHAR16* name, const unsigned long long size, void** buffer)
    {
    EFI_STATUS status;
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
