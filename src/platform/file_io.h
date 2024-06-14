#pragma once

#include <intrin.h>

#include "uefi.h"
#include "console_logging.h"

// If you get an error reading and writing files, set the chunk sizes below to
// the cluster size set for formatting you disk. If you have no idea about the
// cluster size, try 32768.
#define READING_CHUNK_SIZE 1048576
#define WRITING_CHUNK_SIZE 1048576
#define FILE_CHUNK_SIZE (209715200ULL) // for large file saving
#define VOLUME_LABEL L"Qubic"

static EFI_FILE_PROTOCOL* root = NULL;

static long long getFileSize(CHAR16* fileName, CHAR16* directory = NULL)
{
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    EFI_FILE_PROTOCOL* directoryProtocol;
    // Check if there is a directory provided
    if (NULL != directory)
    {
        // Open the directory
        if (status = root->Open(root, (void**)&directoryProtocol, directory, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOgetFileSize:OpenDir EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOgetFileSize:OpenDir:OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            directoryProtocol->Close(directoryProtocol);
            return -1;
        }
        directoryProtocol->Close(directoryProtocol);
    }
    else
    {
        if (status = root->Open(root, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOgetFileSize:OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }
    }

    if (EFI_SUCCESS != status)
    {
        // file doesnt exist
        return -1;
    }
    EFI_GUID fileSystemInfoId = EFI_FILE_INFO_ID;
    unsigned long long bufferSize = 1024;
    unsigned char buffer[1024];
    status = file->GetInfo(file, &fileSystemInfoId, &bufferSize, buffer);
    unsigned long long fileSize = 0;
#ifndef NO_UEFI
    copyMem(&fileSize, buffer+8, 8);
#endif
    return fileSize;
}

static bool checkDir(CHAR16* dirName)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of checkDir() is missing! No directory checked!");
    return false;
#else
    EFI_FILE_PROTOCOL* file;

    // Check if the directory exist or not
    if (EFI_SUCCESS == root->Open(root, (void**)&file, dirName, EFI_FILE_MODE_READ, 0))
    {
        // Directory already existed. No need to create
        file->Close(file);
        return true;
    }
    return false;
#endif
}

static bool createDir(CHAR16* dirName)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of createDir() is missing! No directory created!");
    return false;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;

    // Check if the directory exist or not
    if (checkDir(dirName))
    {
        // Directory already existed. No need to create
        return true;
    }

    // Create a directory
    if (status = root->Open(root, (void**)&file, dirName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
        return false;
    }

    // Close the directory
    file->Close(file);

    return true;
#endif
}

static long long load(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, CHAR16* directory = NULL)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of load() is missing! No file loaded!");
    return 0;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    EFI_FILE_PROTOCOL* directoryProtocol;

    // Check if there is a directory provided
    if (NULL != directory)
    {
        // Open the directory
        if (status = root->Open(root, (void**)&directoryProtocol, directory, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOLoad:OpenDir EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
         {
            logStatusToConsole(L"FileIOLoad:OpenDir:OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            directoryProtocol->Close(directoryProtocol);
            return -1;
        }

        // No need the directory handle. Close it
        directoryProtocol->Close(directoryProtocol);
    }
    else
    {
        if (status = root->Open(root, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOLoad:OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }
    }

    
    if (EFI_SUCCESS == status)
    {
        unsigned long long readSize = 0;
        while (readSize < totalSize)
        {
            unsigned long long size = (READING_CHUNK_SIZE <= (totalSize - readSize) ? READING_CHUNK_SIZE : (totalSize - readSize));
            status = file->Read(file, &size, &buffer[readSize]);
            if (status
                || size != (READING_CHUNK_SIZE <= (totalSize - readSize) ? READING_CHUNK_SIZE : (totalSize - readSize)))
            {
                // If this error occurs, see the definition of READING_CHUNK_SIZE above.
                logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() fails", status, __LINE__);

                file->Close(file);

                return -1;
            }
            readSize += size;
        }
        file->Close(file);

        return readSize;
    }
    return -1;
#endif
}

static long long save(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, CHAR16* directory = NULL)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of save() is missing! No file saved!");
    return 0;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    EFI_FILE_PROTOCOL* directoryProtocol;

    // Check if there is a directory provided
    if (NULL != directory)
    {
        // Create the directory
        createDir(directory);

        // Open the directory
        if (status = root->Open(root, (void**)&directoryProtocol, directory, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOSave:OpenDir EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
        {
            logStatusToConsole(L"FileIOSave:OpenDir::OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            directoryProtocol->Close(directoryProtocol);
            return -1;
        }
        directoryProtocol->Close(directoryProtocol);
    }
    else
    {
        if (status = root->Open(root, (void**)&file, fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
        {
            logStatusToConsole(L"FileIOSave:OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }
    }
    
    if (EFI_SUCCESS == status)
    {
        unsigned long long writtenSize = 0;
        while (writtenSize < totalSize)
        {
            unsigned long long size = (WRITING_CHUNK_SIZE <= (totalSize - writtenSize) ? WRITING_CHUNK_SIZE : (totalSize - writtenSize));
            status = file->Write(file, &size, &buffer[writtenSize]);
            if (status
                || size != (WRITING_CHUNK_SIZE <= (totalSize - writtenSize) ? WRITING_CHUNK_SIZE : (totalSize - writtenSize)))
            {
                // If this error occurs, see the definition of WRITING_CHUNK_SIZE above.
                logStatusToConsole(L"EFI_FILE_PROTOCOL.Write() fails", status, __LINE__);

                file->Close(file);

                return -1;
            }
            writtenSize += size;
        }
        file->Close(file);

        return writtenSize;
    }
    return -1;
#endif
}


static bool initFilesystem()
{
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* simpleFileSystemProtocol = NULL;

    EFI_STATUS status;

    EFI_GUID simpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    unsigned long long numberOfHandles;
    EFI_HANDLE* handles;
    if (status = bs->LocateHandleBuffer(ByProtocol, &simpleFileSystemProtocolGuid, NULL, &numberOfHandles, &handles))
    {
        logStatusToConsole(L"EFI_BOOT_SERVICES.LocateHandleBuffer() fails", status, __LINE__);

        return false;
    }
    else
    {
        for (unsigned int i = 0; i < numberOfHandles; i++)
        {
            if (status = bs->OpenProtocol(handles[i], &simpleFileSystemProtocolGuid, (void**)&simpleFileSystemProtocol, ih, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL))
            {
                logStatusToConsole(L"EFI_BOOT_SERVICES.OpenProtocol() fails", status, __LINE__);

                bs->FreePool(handles);

                return false;
            }
            else
            {
                if (status = simpleFileSystemProtocol->OpenVolume(simpleFileSystemProtocol, (void**)&root))
                {
                    logStatusToConsole(L"EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume() fails", status, __LINE__);

                    bs->CloseProtocol(handles[i], &simpleFileSystemProtocolGuid, ih, NULL);
                    bs->FreePool(handles);

                    return false;
                }
                else
                {
                    EFI_GUID fileSystemInfoId = EFI_FILE_SYSTEM_INFO_ID;
                    EFI_FILE_SYSTEM_INFO info;
                    unsigned long long size = sizeof(info);
                    if (status = root->GetInfo(root, &fileSystemInfoId, &size, &info))
                    {
                        logStatusToConsole(L"EFI_FILE_PROTOCOL.GetInfo() fails", status, __LINE__);

                        bs->CloseProtocol(handles[i], &simpleFileSystemProtocolGuid, ih, NULL);
                        bs->FreePool(handles);

                        return false;
                    }
                    else
                    {
                        setText(message, L"Volume #");
                        appendNumber(message, i, FALSE);
                        appendText(message, L" (");
                        appendText(message, info.VolumeLabel);
                        appendText(message, L"): ");
                        appendNumber(message, info.FreeSpace, TRUE);
                        appendText(message, L" / ");
                        appendNumber(message, info.VolumeSize, TRUE);
                        appendText(message, L" free bytes | Read-");
                        appendText(message, info.ReadOnly ? L"only." : L"Write.");
                        logToConsole(message);

                        bool matches = true;
                        for (unsigned int j = 0; j < sizeof(info.VolumeLabel) / sizeof(info.VolumeLabel[0]); j++)
                        {
                            if (info.VolumeLabel[j] != VOLUME_LABEL[j] && info.VolumeLabel[j] != (VOLUME_LABEL[j] ^ 0x20))
                            {
                                matches = false;

                                break;
                            }
                            if (!VOLUME_LABEL[j])
                            {
                                break;
                            }
                        }
                        if (matches)
                        {
                            break;
                        }
                        else
                        {
                            bs->CloseProtocol(handles[i], &simpleFileSystemProtocolGuid, ih, NULL);
                            simpleFileSystemProtocol = NULL;
                        }
                    }
                }
            }
        }

        bs->FreePool(handles);
    }

    if (!simpleFileSystemProtocol)
    {
        bs->LocateProtocol(&simpleFileSystemProtocolGuid, NULL, (void**)&simpleFileSystemProtocol);
    }
    if (status = simpleFileSystemProtocol->OpenVolume(simpleFileSystemProtocol, (void**)&root))
    {
        logStatusToConsole(L"EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume() fails", status, __LINE__);

        return false;
    }

    return true;
}

// add epoch number as an extension to a filename
static void addEpochToFileName(unsigned short* filename, int nameSize, short epoch)
{
    filename[nameSize - 4] = epoch / 100 + L'0';
    filename[nameSize - 3] = (epoch % 100) / 10 + L'0';
    filename[nameSize - 2] = epoch % 10 + L'0';
}

// note: remember to plus 1 for end of string symbol if you going to use this returned value for addEpochToFileName
static unsigned int getTextSize(CHAR16* text, int maximumSize)
{
    int result = 0;
    while (result < maximumSize && text[result]) result++;
    return result;
}

static long long saveLargeFile(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, CHAR16* directory = NULL)
{
    
    const unsigned long long maxWriteSizePerChunk = FILE_CHUNK_SIZE;
    if (totalSize < maxWriteSizePerChunk) {
        return save(fileName, totalSize, buffer, directory);
    }
    int chunkId = 0;
    unsigned long long totalWriteSize = 0;
    while (totalSize) {
        CHAR16 fileNameWithChunkId[64];
        setText(fileNameWithChunkId, fileName);
        appendText(fileNameWithChunkId, L".XXX");
        addEpochToFileName(fileNameWithChunkId, getTextSize(fileNameWithChunkId, 64) + 1, chunkId);
        const unsigned long long writeSize = maxWriteSizePerChunk < totalSize ? maxWriteSizePerChunk : totalSize;
        long long existFileSize = getFileSize(fileNameWithChunkId);
        if (existFileSize != writeSize) {
            unsigned long long res = save(fileNameWithChunkId, writeSize, buffer, directory);
            if (res != writeSize) {
                return totalWriteSize;
            }
        }
        buffer += writeSize;
        totalWriteSize += writeSize;
        totalSize -= writeSize;
        chunkId++;
    }
    return totalWriteSize;
}

static long long loadLargeFile(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, CHAR16* directory = NULL)
{
    const unsigned long long maxReadSizePerChunk = FILE_CHUNK_SIZE;
    if (totalSize < maxReadSizePerChunk) {
        return load(fileName, totalSize, buffer, directory);
    }
    int chunkId = 0;
    unsigned long long totalReadSize = 0;
    while (totalSize) {
        CHAR16 fileNameWithChunkId[64];
        setText(fileNameWithChunkId, fileName);
        appendText(fileNameWithChunkId, L".XXX");
        addEpochToFileName(fileNameWithChunkId, getTextSize(fileNameWithChunkId, 64) + 1, chunkId);
        const unsigned long long readSize = maxReadSizePerChunk < totalSize ? maxReadSizePerChunk : totalSize;
        unsigned long long res = load(fileNameWithChunkId, readSize, buffer, directory);
        if (res != readSize) {
            return totalReadSize;
        }
        buffer += readSize;
        totalReadSize += readSize;
        totalSize -= readSize;
        chunkId++;
    }
    return totalReadSize;
}