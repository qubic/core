#include <lib/platform_common/processor.h>
#include <lib/platform_efi/uefi_globals.h>

unsigned long long mainProcessorID = -1;


// Return processor number of processor running this function
unsigned long long getRunningProcessorID()
{
    // TODO: implement this
    return mainProcessorID;
}

// Check if running processor is main processor (bootstrap processor in EFI)
bool isMainProcessor()
{
    return getRunningProcessorID() == mainProcessorID;
}
