#include <lib/platform_common/processor.h>
#include <lib/platform_efi/uefi_globals.h>

unsigned long long mainThreadProcessorID = -1;

EFI_MP_SERVICES_PROTOCOL* mpServicesProtocol = nullptr;


// Return processor number of processor running this function
unsigned long long getRunningProcessorID()
{
    unsigned long long processorNumber;
    mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);
    return processorNumber;
}

// Check if running processor is main processor (bootstrap processor in EFI)
bool isMainProcessor()
{
    return getRunningProcessorID() == mainThreadProcessorID;
}
