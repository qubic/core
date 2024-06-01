#pragma once

#include "uefi.h"

static bool disableConsoleLogging = false;

// message buffers:
// - message is for public use
// - timestampedMessage is used internally by logToConsole()
// CAUTION: not thread-safe, no buffer overflow protection!!!
static CHAR16 message[16384], timestampedMessage[16384];


#ifdef NO_UEFI

#include <cstdio>

// Output to console on no-UEFI platform
static inline void outputStringToConsole(const CHAR16* str)
{
    wprintf(L"%ls", str);
}

// Log message to console (with line break) on non-UEFI platform
static void logToConsole(const CHAR16* message)
{
    if (disableConsoleLogging)
        return;
    wprintf(L"%ls\n", message);
}

#else

// Output to console on UEFI platform
// CAUTION: Can only be called from main processor thread. Otherwise there is a high risk of crashing.
static inline void outputStringToConsole(const CHAR16* str)
{
    st->ConOut->OutputString(st->ConOut, (CHAR16*)str);
}

// Log message to console (with line break) on UEFI platform (defined in qubic.cpp due to dependencies on time and qubic status)
// CAUTION: Can only be called from main processor thread. Otherwise there is a high risk of crashing.
static void logToConsole(const CHAR16* message);

#endif

static void appendText(CHAR16* dst, const CHAR16* src)
{
    unsigned short dstIndex = 0;
    while (dst[dstIndex] != 0)
    {
        dstIndex++;
    }
    unsigned short srcIndex = 0;
    while ((dst[dstIndex++] = src[srcIndex++]) != 0)
    {
    }
}

static void setText(CHAR16* dst, const CHAR16* src)
{
    dst[0] = 0;
    appendText(dst, src);
}

static void appendNumber(CHAR16* dst, unsigned long long number, BOOLEAN separate)
{
    CHAR16 text[27];
    char textLength = 0;
    do
    {
        text[textLength++] = number % 10 + '0';
    } while ((number /= 10) > 0);
    unsigned short dstIndex = 0;
    while (dst[dstIndex] != 0)
    {
        dstIndex++;
    }
    while (--textLength >= 0)
    {
        dst[dstIndex++] = text[textLength];
        if (separate && textLength % 3 == 0 && textLength != 0)
        {
            dst[dstIndex++] = '\'';
        }
    }
    dst[dstIndex] = 0;
}

static void setNumber(CHAR16* dst, const unsigned long long number, BOOLEAN separate)
{
    dst[0] = 0;
    appendNumber(dst, number, separate);
}
static void appendIPv4Address(CHAR16* dst, EFI_IPv4_ADDRESS address)
{
    appendNumber(dst, address.Addr[0], FALSE);
    appendText(dst, L".");
    appendNumber(dst, address.Addr[1], FALSE);
    appendText(dst, L".");
    appendNumber(dst, address.Addr[2], FALSE);
    appendText(dst, L".");
    appendNumber(dst, address.Addr[3], FALSE);
}

static void appendErrorStatus(CHAR16* dst, const EFI_STATUS status)
{
    switch (status)
    {
    case EFI_LOAD_ERROR:			appendText(dst, L"EFI_LOAD_ERROR");				break;
    case EFI_INVALID_PARAMETER:		appendText(dst, L"EFI_INVALID_PARAMETER");		break;
    case EFI_UNSUPPORTED:			appendText(dst, L"EFI_UNSUPPORTED");			break;
    case EFI_BAD_BUFFER_SIZE:		appendText(dst, L"EFI_BAD_BUFFER_SIZE");		break;
    case EFI_BUFFER_TOO_SMALL:		appendText(dst, L"EFI_BUFFER_TOO_SMALL");		break;
    case EFI_NOT_READY:				appendText(dst, L"EFI_NOT_READY");				break;
    case EFI_DEVICE_ERROR:			appendText(dst, L"EFI_DEVICE_ERROR");			break;
    case EFI_WRITE_PROTECTED:		appendText(dst, L"EFI_WRITE_PROTECTED");		break;
    case EFI_OUT_OF_RESOURCES:		appendText(dst, L"EFI_OUT_OF_RESOURCES");		break;
    case EFI_VOLUME_CORRUPTED:		appendText(dst, L"EFI_VOLUME_CORRUPTED");		break;
    case EFI_VOLUME_FULL:			appendText(dst, L"EFI_VOLUME_FULL");			break;
    case EFI_NO_MEDIA:				appendText(dst, L"EFI_NO_MEDIA");				break;
    case EFI_MEDIA_CHANGED:			appendText(dst, L"EFI_MEDIA_CHANGED");			break;
    case EFI_NOT_FOUND:				appendText(dst, L"EFI_NOT_FOUND");				break;
    case EFI_ACCESS_DENIED:			appendText(dst, L"EFI_ACCESS_DENIED");			break;
    case EFI_NO_RESPONSE:			appendText(dst, L"EFI_NO_RESPONSE");			break;
    case EFI_NO_MAPPING:			appendText(dst, L"EFI_NO_MAPPING");				break;
    case EFI_TIMEOUT:				appendText(dst, L"EFI_TIMEOUT");				break;
    case EFI_NOT_STARTED:			appendText(dst, L"EFI_NOT_STARTED");			break;
    case EFI_ALREADY_STARTED:		appendText(dst, L"EFI_ALREADY_STARTED");		break;
    case EFI_ABORTED:				appendText(dst, L"EFI_ABORTED");				break;
    case EFI_ICMP_ERROR:			appendText(dst, L"EFI_ICMP_ERROR");				break;
    case EFI_TFTP_ERROR:			appendText(dst, L"EFI_TFTP_ERROR");				break;
    case EFI_PROTOCOL_ERROR:		appendText(dst, L"EFI_PROTOCOL_ERROR");			break;
    case EFI_INCOMPATIBLE_VERSION:	appendText(dst, L"EFI_INCOMPATIBLE_VERSION");	break;
    case EFI_SECURITY_VIOLATION:	appendText(dst, L"EFI_SECURITY_VIOLATION");		break;
    case EFI_CRC_ERROR:				appendText(dst, L"EFI_CRC_ERROR");				break;
    case EFI_END_OF_MEDIA:			appendText(dst, L"EFI_END_OF_MEDIA");			break;
    case EFI_END_OF_FILE:			appendText(dst, L"EFI_END_OF_FILE");			break;
    case EFI_INVALID_LANGUAGE:		appendText(dst, L"EFI_INVALID_LANGUAGE");		break;
    case EFI_COMPROMISED_DATA:		appendText(dst, L"EFI_COMPROMISED_DATA");		break;
    case EFI_IP_ADDRESS_CONFLICT:	appendText(dst, L"EFI_IP_ADDRESS_CONFLICT");	break;
    case EFI_HTTP_ERROR:			appendText(dst, L"EFI_HTTP_ERROR");				break;
    case EFI_NETWORK_UNREACHABLE:	appendText(dst, L"EFI_NETWORK_UNREACHABLE");	break;
    case EFI_HOST_UNREACHABLE:		appendText(dst, L"EFI_HOST_UNREACHABLE");		break;
    case EFI_PROTOCOL_UNREACHABLE:	appendText(dst, L"EFI_PROTOCOL_UNREACHABLE");	break;
    case EFI_PORT_UNREACHABLE:		appendText(dst, L"EFI_PORT_UNREACHABLE");		break;
    case EFI_CONNECTION_FIN:		appendText(dst, L"EFI_CONNECTION_FIN");			break;
    case EFI_CONNECTION_RESET:		appendText(dst, L"EFI_CONNECTION_RESET");		break;
    case EFI_CONNECTION_REFUSED:	appendText(dst, L"EFI_CONNECTION_REFUSED");		break;
    default: appendNumber(dst, status, FALSE);
    }
}

static void logStatusToConsole(const CHAR16* message, const EFI_STATUS status, const unsigned int lineNumber)
{
    setText(::message, message);
    appendText(::message, L" (");
    appendErrorStatus(::message, status);
    appendText(::message, L") near line ");
    appendNumber(::message, lineNumber, FALSE);
    appendText(::message, L"!");
    logToConsole(::message);
}

// Count characters before terminating NULL
static unsigned int stringLength(const CHAR16* str)
{
    unsigned int l = 0;
    while (str[l] != 0)
        l++;
    return l;
}
