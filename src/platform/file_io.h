#pragma once

#include <intrin.h>

#include "uefi.h"
#include "logging.h"

#define READING_CHUNK_SIZE 1048576
#define WRITING_CHUNK_SIZE 1048576

static EFI_FILE_PROTOCOL* root = NULL;


static long long load(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer)
{
#ifdef NO_UEFI
    log(L"NO_UEFI implementation of load() is missing! No file loaded!");
    return 0;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    if (status = root->Open(root, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
    {
        logStatus(L"EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);

        return -1;
    }
    else
    {
        unsigned long long readSize = 0;
        while (readSize < totalSize)
        {
            unsigned long long size = (READING_CHUNK_SIZE <= (totalSize - readSize) ? READING_CHUNK_SIZE : (totalSize - readSize));
            status = file->Read(file, &size, &buffer[readSize]);
            if (status
                || size != (READING_CHUNK_SIZE <= (totalSize - readSize) ? READING_CHUNK_SIZE : (totalSize - readSize)))
            {
                logStatus(L"EFI_FILE_PROTOCOL.Read() fails", status, __LINE__);

                file->Close(file);

                return -1;
            }
            readSize += size;
        }
        file->Close(file);

        return readSize;
    }
#endif
}

static long long save(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer)
{
#ifdef NO_UEFI
    log(L"NO_UEFI implementation of save() is missing! No file saved!");
    return 0;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    if (status = root->Open(root, (void**)&file, fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
    {
        logStatus(L"EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);

        return -1;
    }
    else
    {
        unsigned long long writtenSize = 0;
        while (writtenSize < totalSize)
        {
            unsigned long long size = (WRITING_CHUNK_SIZE <= (totalSize - writtenSize) ? WRITING_CHUNK_SIZE : (totalSize - writtenSize));
            status = file->Write(file, &size, &buffer[writtenSize]);
            if (status
                || size != (WRITING_CHUNK_SIZE <= (totalSize - writtenSize) ? WRITING_CHUNK_SIZE : (totalSize - writtenSize)))
            {
                logStatus(L"EFI_FILE_PROTOCOL.Write() fails", status, __LINE__);

                file->Close(file);

                return -1;
            }
            writtenSize += size;
        }
        file->Close(file);

        return writtenSize;
    }
#endif
}
