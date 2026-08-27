#pragma once

#include <lib/platform_efi/uefi.h>
#include <lib/platform_efi/uefi_globals.h>
#include <lib/platform_common/qintrin.h>

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}

typedef enum
{
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct
{
    unsigned int RedMask;
    unsigned int GreenMask;
    unsigned int BlueMask;
    unsigned int ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct
{
    unsigned int Version;
    unsigned int HorizontalResolution;
    unsigned int VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    unsigned int PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct
{
    unsigned int MaxMode;
    unsigned int Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    unsigned long long SizeOfInfo;
    unsigned long long FrameBufferBase;
    unsigned long long FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct
{
    void* QueryMode;
    void* SetMode;
    void* Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

static unsigned int* gFrameBufferBase = NULL;
static unsigned int gFrameBufferPixelsPerScanLine = 0;
static unsigned int gFrameBufferWidth = 0;
static unsigned int gFrameBufferHeight = 0;

static constexpr unsigned int BREADCRUMB_CELL = 16;
static constexpr unsigned int BREADCRUMB_GAP = 4;
static constexpr unsigned int BREADCRUMB_BITS = 16;

static bool initFrameBuffer()
{
    EFI_GUID graphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    if (bs->LocateProtocol(&graphicsOutputProtocolGuid, NULL, (void**)&gop) || !gop || !gop->Mode || !gop->Mode->Info)
    {
        return false;
    }

    const EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = gop->Mode->Info;
    if (info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor
        && info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
    {
        return false;
    }

    gFrameBufferBase = (unsigned int*)gop->Mode->FrameBufferBase;
    gFrameBufferPixelsPerScanLine = info->PixelsPerScanLine;
    gFrameBufferWidth = info->HorizontalResolution;
    gFrameBufferHeight = info->VerticalResolution;
    return gFrameBufferBase != NULL && gFrameBufferPixelsPerScanLine != 0;
}

static void fillFrameBufferRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int colour)
{
    if (!gFrameBufferBase || x + w > gFrameBufferWidth || y + h > gFrameBufferHeight)
    {
        return;
    }
    for (unsigned int row = 0; row < h; row++)
    {
        unsigned int* line = gFrameBufferBase + (unsigned long long)(y + row) * gFrameBufferPixelsPerScanLine + x;
        for (unsigned int col = 0; col < w; col++)
        {
            line[col] = colour;
        }
    }
}

static void drawBreadcrumbRow(unsigned int rowIndex, unsigned int value, unsigned int colour)
{
    if (!gFrameBufferBase)
    {
        return;
    }
    const unsigned int y = rowIndex * (BREADCRUMB_CELL + BREADCRUMB_GAP);
    for (unsigned int bit = 0; bit < BREADCRUMB_BITS; bit++)
    {
        const unsigned int x = bit * (BREADCRUMB_CELL + BREADCRUMB_GAP);
        const bool isSet = (value >> (BREADCRUMB_BITS - 1 - bit)) & 1;
        fillFrameBufferRect(x, y, BREADCRUMB_CELL, BREADCRUMB_CELL, isSet ? colour : 0x00202020);
    }
}
