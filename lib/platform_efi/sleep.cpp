#include <lib/platform_common/sleep.h>

#include "uefi_globals.h"

void sleepMilliseconds(unsigned int milliseconds)
{
    bs->Stall(milliseconds * 1000);
}
