#pragma once

#include "concurrency.h"
#include "file_io.h"


#if defined(EXPECT_TRUE)

// in gtest context, use EXPECT_TRUE as ASSERT
#define ASSERT EXPECT_TRUE

static void addDebugMessage(const CHAR16* msg)
{
    wprintf(L"%ls\n", msg);
}

#elif defined(NDEBUG)
static void addDebugMessage(const CHAR16* msg){} // empty impl
// with NDEBUG, make ASSERT disappear
#define ASSERT(expression) ((void)0)

#else

static CHAR16 debugMessage[128][16384];
static int debugMessageCount = 0;
static char volatile debugLogLock = 0;

#define WRITE_DEBUG_MESSAGES_TO_FILE 1

// Print debug messages added with addDebugMessage().
// CAUTION: Can only be called from main processor thread. Otherwise there is a high risk of crashing.
static void printDebugMessages()
{
    if (!debugMessageCount)
        return;
#if WRITE_DEBUG_MESSAGES_TO_FILE
    // Open debug log file and seek to the end of file for appending
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file = nullptr;
    if (status = root->Open(root, (void**)&file, (CHAR16*)L"debug.log", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
        file = nullptr;
    }
    else
    {
        if (status = root->SetPosition(file, 0xFFFFFFFFFFFFFFFF))
        {
            logStatusToConsole(L"EFI_FILE_PROTOCOL.SetPosition() fails", status, __LINE__);
            file = nullptr;
        }
    }
#endif
    ACQUIRE(debugLogLock);
    for (int i = 0; i < debugMessageCount; i++)
    {
        // Make sure there is a newline at the end
        unsigned int strLen = stringLength(debugMessage[i]);
        if (debugMessage[i][strLen-1] != L'\n')
        {
            appendText(debugMessage[i], L"\r\n");
            strLen += 2;
        }

        // Write to console
        outputStringToConsole(debugMessage[i]);

#if WRITE_DEBUG_MESSAGES_TO_FILE
        // Write to log file
        if (file)
        {
            char* buffer = (char*)debugMessage[i];
            unsigned long long totalSize = strLen * sizeof(CHAR16);
            unsigned long long writtenSize = 0;
            while (writtenSize < totalSize)
            {
                unsigned long long size = (WRITING_CHUNK_SIZE <= (totalSize - writtenSize) ? WRITING_CHUNK_SIZE : (totalSize - writtenSize));
                status = file->Write(file, &size, &buffer[writtenSize]);
                if (status
                    || size != (WRITING_CHUNK_SIZE <= (totalSize - writtenSize) ? WRITING_CHUNK_SIZE : (totalSize - writtenSize)))
                {
                    logStatusToConsole(L"EFI_FILE_PROTOCOL.Write() fails", status, __LINE__);

                    file->Close(file);
                    file = 0;
                    break;
                }
                writtenSize += size;
            }
        }
#endif
    }
    debugMessageCount = 0;
    RELEASE(debugLogLock);
#if WRITE_DEBUG_MESSAGES_TO_FILE
    if (file)
        file->Close(file);
#endif
}

// Add a message for logging from arbitrary thread
static void addDebugMessage(const CHAR16* msg)
{
    ACQUIRE(debugLogLock);
    if (debugMessageCount < 128)
    {
        setText(debugMessage[debugMessageCount], msg);
        ++debugMessageCount;
    }
    RELEASE(debugLogLock);
}

// Add a assert message for logging from arbitrary thread
static void addDebugMessageAssert(const CHAR16* message, const CHAR16* file, const unsigned int lineNumber)
{
    ACQUIRE(debugLogLock);
    if (debugMessageCount < 128)
    {
        setText(debugMessage[debugMessageCount], L"Assertion failed: ");
        appendText(debugMessage[debugMessageCount], message);
        appendText(debugMessage[debugMessageCount], L" at line ");
        appendNumber(debugMessage[debugMessageCount], lineNumber, FALSE);
        appendText(debugMessage[debugMessageCount], L" in ");
        appendText(debugMessage[debugMessageCount], file);
        ++debugMessageCount;
    }
    RELEASE(debugLogLock);
}

#define ASSERT(expression) (void)(                                                       \
            (!!(expression)) ||                                                              \
            (addDebugMessageAssert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned int)(__LINE__)), 0) \
        )

#endif
