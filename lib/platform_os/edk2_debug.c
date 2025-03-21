// implements DebugLib of edk2 in platform_common

#include <lib/platform_common/edk2_mdepkg/Include/Base.h>
#include <lib/platform_common/edk2_mdepkg/Include/Library/DebugLib.h>

#include <stdio.h>

VOID
EFIAPI
DebugAssert(
    IN CONST CHAR8* FileName,
    IN UINTN        LineNumber,
    IN CONST CHAR8* Description
)
{
    printf("ASSERT failed in file %s line %llu: %s\n", FileName, LineNumber, Description);
}

BOOLEAN
EFIAPI
DebugAssertEnabled(
    VOID
)
{
    return TRUE;
}
