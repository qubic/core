#pragma once

#include "uefi.h"

extern EFI_HANDLE ih;
extern EFI_SYSTEM_TABLE* st;
extern EFI_RUNTIME_SERVICES* rs;
extern EFI_BOOT_SERVICES* bs;

// TODO: later hide this in processor.cpp
extern EFI_MP_SERVICES_PROTOCOL* mpServicesProtocol;
