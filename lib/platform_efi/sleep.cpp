#include <lib/platform_common/sleep.h>

#include "uefi_globals.h"

void sleepMicroseconds(unsigned int microseconds)
{
    bs->Stall(microseconds);
}

void sleepMilliseconds(unsigned int milliseconds)
{
    bs->Stall(milliseconds * 1000);
}
