#include <lib/platform_common/processor.h>
#include <lib/platform_efi/uefi_globals.h>

unsigned long long mainThreadProcessorID = -1;

EFI_MP_SERVICES_PROTOCOL* mpServicesProtocol = nullptr;


// Return processor number of processor running this function
unsigned long long getRunningProcessorID()
{
    // if mpServicesProtocol isn't set, we can be sure that this is still the main processor thread, because
    // mpServicesProtocol is needed to startup other processors
    unsigned long long processorNumber = mainThreadProcessorID;
    if (mpServicesProtocol)
        mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);
    return processorNumber;
}

// Check if running processor is main processor (bootstrap processor in EFI)
bool isMainProcessor()
{
    return getRunningProcessorID() == mainThreadProcessorID;
}
