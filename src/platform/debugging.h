#pragma once

#include "assert.h"
#include "concurrency.h"
#include "console_logging.h"
#include "file_io.h"


#if defined(EXPECT_TRUE)

// In gtest context, print with standard library
static void addDebugMessage(const CHAR16* msg)
{
    wprintf(L"%ls\n", msg);
}

// In gtest context, this is noop
static void printDebugMessages()
{
}

#elif defined(NDEBUG)

// static void addDebugMessage(const CHAR16* msg){} // empty impl
#else

static CHAR16 debugMessage[128][16384];
static int debugMessageCount = 0;
static char volatile debugLogLock = 0;
static bool volatile debugLogOnlyMainProcessorRunning = true;
static bool forceLogToConsoleAsAddDebugMessage = false;

#define WRITE_DEBUG_MESSAGES_TO_FILE 1

// Print debug messages added with addDebugMessage().
// CAUTION: Can only be called from main processor thread. Otherwise there is a high risk of crashing.
static void printDebugMessages()
{
    if (!debugMessageCount)
        return;
    ACQUIRE_WITHOUT_DEBUG_LOGGING(debugLogLock);

    // Write to console first
    for (int i = 0; i < debugMessageCount; i++)
    {
        // Make sure there is a newline at the end
        unsigned int strLen = stringLength(debugMessage[i]);
        if (debugMessage[i][strLen - 1] != L'\n')
        {
            appendText(debugMessage[i], L"\r\n");
            strLen += 2;
        }

        // Write to console
        outputStringToConsole(debugMessage[i]);
    }

#if WRITE_DEBUG_MESSAGES_TO_FILE
    // Open debug log file and seek to the end of file for appending
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file = nullptr;
    if (!root)
    {
    }
    else if (status = root->Open(root, (void**)&file, (CHAR16*)L"debug.log", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
    {
        setText(::message, L"EFI_FILE_PROTOCOL.Open() failed in printDebugMessages() with status ");
        appendErrorStatus(::message, status);
        appendText(::message, L"\r\n");
        outputStringToConsole(::message);
        file = nullptr;
    }
    else
    {
        if (status = root->SetPosition(file, 0xFFFFFFFFFFFFFFFF))
        {
            setText(::message, L"EFI_FILE_PROTOCOL.SetPosition() failed in printDebugMessages() with status ");
            appendErrorStatus(::message, status);
            appendText(::message, L"\r\n");
            outputStringToConsole(::message);
            file->Close(file);
            file = nullptr;
        }
    }

    if (file)
    {
        // Write to log file
        for (int i = 0; i < debugMessageCount; i++)
        {
            unsigned int strLen = stringLength(debugMessage[i]);
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
                    setText(::message, L"EFI_FILE_PROTOCOL.Write() failed in printDebugMessages() with status ");
                    appendErrorStatus(::message, status);
                    appendText(::message, L"\r\n");
                    outputStringToConsole(::message);

                    goto closeFile;
                }
                writtenSize += size;
            }
        }

    closeFile:
        file->Close(file);
    }
#endif

    debugMessageCount = 0;
    RELEASE(debugLogLock);
}

// Add a message for logging from arbitrary thread
static void addDebugMessage(const CHAR16* msg)
{
    ACQUIRE_WITHOUT_DEBUG_LOGGING(debugLogLock);
    if (debugMessageCount < 128)
    {
        setText(debugMessage[debugMessageCount], msg);
        ++debugMessageCount;
    }
    RELEASE(debugLogLock);
    if (debugLogOnlyMainProcessorRunning)
    {
        debugLogOnlyMainProcessorRunning = false;
        printDebugMessages();
        debugLogOnlyMainProcessorRunning = true;
    }
}

// Add a assert message for logging from arbitrary thread
static void addDebugMessageAssert(const char* message, const char* file, const unsigned int lineNumber)
{
    ACQUIRE_WITHOUT_DEBUG_LOGGING(debugLogLock);
    if (debugMessageCount < 128)
    {
        setText(debugMessage[debugMessageCount], L"Assertion failed: ");
        appendText(debugMessage[debugMessageCount], message);
        appendText(debugMessage[debugMessageCount], " at line ");
        appendNumber(debugMessage[debugMessageCount], lineNumber, FALSE);
        appendText(debugMessage[debugMessageCount], " in ");
        appendText(debugMessage[debugMessageCount], file);
        ++debugMessageCount;
    }
    RELEASE(debugLogLock);
    if (debugLogOnlyMainProcessorRunning)
    {
        debugLogOnlyMainProcessorRunning = false;
        printDebugMessages();
        debugLogOnlyMainProcessorRunning = true;
    }
}

#endif
