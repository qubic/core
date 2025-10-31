#pragma once

#include <lib/platform_common/qintrin.h>
#include "platform/assert.h"
#ifdef NO_UEFI
#include <cstdio>
#endif
#include <lib/platform_common/processor.h>
#include <lib/platform_common/compiler_optimization.h>
#include <lib/platform_efi/uefi.h>

#include "console_logging.h"
#include "concurrency.h"
#include "memory.h"

// If you get an error reading and writing files, set the chunk sizes below to
// the cluster size set for formatting you disk. If you have no idea about the
// cluster size, try 16384.
#define READING_CHUNK_SIZE 32768
#define WRITING_CHUNK_SIZE 32768
#define FILE_CHUNK_SIZE (209715200ULL) // for large file saving
#define VOLUME_LABEL L"Qubic"

// Size of buffer for non-blocking save, this size will divided into ASYNC_FILE_IO_BLOCKING_MAX_QUEUE_ITEMS.
// Can set by zeros to save memory and don't use non-block save
static constexpr unsigned long long ASYNC_FILE_IO_WRITE_QUEUE_BUFFER_SIZE = 0;//8 * 1024 * 1024 * 1024ULL; // Set 0 if don't need non-blocking save
static constexpr int ASYNC_FILE_IO_BLOCKING_MAX_QUEUE_ITEMS_2FACTOR = 10;
static constexpr int ASYNC_FILE_IO_MAX_QUEUE_ITEMS_2FACTOR = 4;
static constexpr int ASYNC_FILE_IO_MAX_FILE_NAME = 64;
static constexpr int ASYNC_FILE_IO_BLOCKING_MAX_QUEUE_ITEMS = (1ULL << ASYNC_FILE_IO_BLOCKING_MAX_QUEUE_ITEMS_2FACTOR);
static constexpr int ASYNC_FILE_IO_MAX_QUEUE_ITEMS = (1ULL << ASYNC_FILE_IO_MAX_QUEUE_ITEMS_2FACTOR);

static EFI_FILE_PROTOCOL* root = NULL;
class AsyncFileIO;
static AsyncFileIO* gAsyncFileIO = NULL;

static void addDebugMessage(const CHAR16* msg);

static long long getFileSize(CHAR16* fileName, CHAR16* directory = NULL)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of getFileSize() is missing! No valid file size returned!");
    return -1;
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
            return -1;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
        {
            directoryProtocol->Close(directoryProtocol);
            return -1;
        }
        directoryProtocol->Close(directoryProtocol);
    }
    else
    {
        if (status = root->Open(root, (void**)&file, fileName, EFI_FILE_MODE_READ, 0))
        {
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
    copyMem(&fileSize, buffer + 8, 8);
    return fileSize;
#endif
}

static bool checkDir(const CHAR16* dirName)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of checkDir() is missing! No directory checked!");
    return false;
#else
    ASSERT(isMainProcessor());
    EFI_FILE_PROTOCOL* file;

    // Check if the directory exist or not
    if (EFI_SUCCESS == root->Open(root, (void**)&file, (CHAR16*)dirName, EFI_FILE_MODE_READ, 0))
    {
        // Directory already existed. No need to create
        file->Close(file);
        return true;
    }
    return false;
#endif
}

static bool createDir(const CHAR16* dirName)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of createDir() is missing! No directory created!");
    return false;
#else
    ASSERT(isMainProcessor());
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;

    // Check if the directory exist or not
    if (checkDir(dirName))
    {
        // Directory already existed. No need to create
        return true;
    }

    // Create a directory
    if (status = root->Open(root, (void**)&file, (CHAR16*)dirName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
        return false;
    }

    // Close the directory
    file->Close(file);

    return true;
#endif
}

// Can only be called from mainthread
static bool removeFile(CHAR16* directory, CHAR16* fileName)
{
#ifdef NO_UEFI
    logToConsole(L"NO_UEFI implementation of removeFile() is missing! No directory checked!");
    return false;
#else
    ASSERT(isMainProcessor());
    long long fileSz = getFileSize(fileName, directory);
    if (fileSz == -1) // file doesn't exist
    {
        return true;
    }
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    EFI_FILE_PROTOCOL* directoryProtocol;
    // Check if there is a directory provided
    if (NULL != directory)
    {
        // Open the directory
        if (status = root->Open(root, (void**)&directoryProtocol, (CHAR16*)directory, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0))
        {
            setText(message, L"FileIOLoad:OpenDir EFI_FILE_PROTOCOL.Open() fails - Cannot open dir: ");
            appendText(message, directory);
            appendText(message, L" near line ");
            appendNumber(message, __LINE__, FALSE);
            logToConsole(message);
#ifndef NDEBUG
            addDebugMessage(message);
#endif
            return false;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, (CHAR16*)fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0))
        {
            setText(message, L"FileIOLoad:OpenDir EFI_FILE_PROTOCOL.Open() fails - Cannot open file: ");
            appendText(message, directory);
            appendText(message, L"/");
            appendText(message, fileName);
            appendText(message, L" near line ");
            appendNumber(message, __LINE__, FALSE);
            logToConsole(message);
            directoryProtocol->Close(directoryProtocol);
#ifndef NDEBUG
            addDebugMessage(message);
#endif
            return false;
        }

        // No need the directory handle. Close it
        directoryProtocol->Close(directoryProtocol);
    }
    else
    {
        if (status = root->Open(root, (void**)&file, (CHAR16*)fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0))
        {
            setText(message, L"FileIOLoad:OpenDir EFI_FILE_PROTOCOL.Open() fails - Cannot open file: ");
            appendText(message, fileName);
            appendText(message, L" near line ");
            appendNumber(message, __LINE__, FALSE);
            logToConsole(message);
#ifndef NDEBUG
            addDebugMessage(message);
#endif
            return false;
        }
    }

    if ((status = file->Delete(file)))
    {
        setText(message, L"FileIORem: Failed - Cannot delete file: ");
        if (directory)
        {
            appendText(message, directory);
            appendText(message, L"/");
        }
        appendText(message, fileName);
        appendText(message, L" near line ");
        appendNumber(message, __LINE__, FALSE);
        logToConsole(message);
#ifndef NDEBUG
        addDebugMessage(message);
#endif
        return false;
    }
    return true;
#endif
}

static long long load(const CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, const CHAR16* directory = NULL)
{
#ifdef NO_UEFI
    if (directory)
    {
        logToConsole(L"Argument directory not implemented for NO_UEFI load()! Pass full path as fileName!");
        return -1;
    }
    FILE* file = nullptr;
    if (_wfopen_s(&file, fileName, L"rb") != 0 || !file)
    {
        wprintf(L"Error opening file %s!\n", fileName);
        return -1;
    }
    if (fread(buffer, 1, totalSize, file) != totalSize)
    {
        wprintf(L"Error reading %llu bytes from %s!\n", totalSize, fileName);
        return -1;
    }
    fclose(file);
    return totalSize;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    EFI_FILE_PROTOCOL* directoryProtocol;

    // Check if there is a directory provided
    if (NULL != directory)
    {
        // Open the directory
        if (status = root->Open(root, (void**)&directoryProtocol, (CHAR16*)directory, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOLoad:OpenDir EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, (CHAR16*)fileName, EFI_FILE_MODE_READ, 0))
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
        if (status = root->Open(root, (void**)&file, (CHAR16*)fileName, EFI_FILE_MODE_READ, 0))
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

static long long save(const CHAR16* fileName, unsigned long long totalSize, const unsigned char* buffer, const CHAR16* directory = NULL)
{
#ifdef NO_UEFI
    if (directory)
    {
        logToConsole(L"Argument directory not implemented for NO_UEFI save()! Pass full path as fileName!");
        return -1;
    }
    FILE* file = nullptr;
    if (_wfopen_s(&file, fileName, L"wb") != 0 || !file)
    {
        wprintf(L"Error opening file %s!\n", fileName);
        return -1;
    }
    if (fwrite(buffer, 1, totalSize, file) != totalSize)
    {
        wprintf(L"Error writting %llu bytes from %s!\n", totalSize, fileName);
        return -1;
    }
    fclose(file);
    return totalSize;
#else
    EFI_STATUS status;
    EFI_FILE_PROTOCOL* file = NULL;
    EFI_FILE_PROTOCOL* directoryProtocol = NULL;

    // Check if there is a directory provided
    if (NULL != directory)
    {
        // Create the directory
        createDir(directory);

        // Open the directory
        if (status = root->Open(root, (void**)&directoryProtocol, (CHAR16*)directory, EFI_FILE_MODE_READ, 0))
        {
            logStatusToConsole(L"FileIOSave:OpenDir EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            return -1;
        }

        if (NULL == directoryProtocol)
        {
            logStatusToConsole(L"FileIOSave:OpenDir directory protocols is NULL", status, __LINE__);
            return -1;
        }

        // Open the file from the directory.
        if (status = directoryProtocol->Open(directoryProtocol, (void**)&file, (CHAR16*)fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
        {
            logStatusToConsole(L"FileIOSave:OpenDir::OpenFile EFI_FILE_PROTOCOL.Open() fails", status, __LINE__);
            directoryProtocol->Close(directoryProtocol);
            return -1;
        }
        directoryProtocol->Close(directoryProtocol);
    }
    else
    {
        if (status = root->Open(root, (void**)&file, (CHAR16*)fileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0))
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
            status = file->Write(file, &size, (void*)&buffer[writtenSize]);
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

OPTIMIZE_OFF()

struct FileItem
{
    enum ItemState
    {
        kFree = 0,
        kProcessed,
        kBlockingWait,
        kWait,
        kFillingData,
    };

    CHAR16 mFileName[ASYNC_FILE_IO_MAX_FILE_NAME];
    CHAR16 mDirectory[ASYNC_FILE_IO_MAX_FILE_NAME];
    bool mHaveDirectory;
    unsigned long long mSize;
    unsigned char* mpBuffer;
    const unsigned char* mpConstBuffer;
    char mState;
    unsigned long long mReservedSize;
    long long mAge;

    void set(const CHAR16* fileName, unsigned long long fileSize, const CHAR16* directory)
    {
        setText(mFileName, fileName);
        mHaveDirectory = false;
        if (NULL != directory)
        {
            setText(mDirectory, directory);
            mHaveDirectory = true;
        }
        mSize = fileSize;
    }

    void setState(char val)
    {
        ATOMIC_STORE8(mState, val);
    }
    // Check if a slot is occupied
    bool waitForProcess()
    {
        char state;
        ATOMIC_STORE8(state, mState);
        return (state == FileItem::kBlockingWait || state == FileItem::kWait);
    }

    bool isProcessed()
    {
        char state;
        ATOMIC_STORE8(state, mState);
        return (state == FileItem::kProcessed);
    }

    // Check if a slot is free
    bool slotIsFree()
    {
        char state;
        ATOMIC_STORE8(state, mState);
        return (state == FileItem::kFree);
    }

    void markAsDone()
    {
        char stateChange = FileItem::kProcessed;
        if (mState == FileItem::kBlockingWait)
        {
            stateChange = FileItem::kProcessed;
        }
        else if (mState == FileItem::kWait)
        {
            stateChange = FileItem::kFree;
        }
        setState(stateChange);
        ATOMIC_STORE64(mAge, 0);
    }

};

template<typename KeyType, typename ValueType>
struct PairStruct
{
    KeyType _key;
    ValueType _value;
};

// Partition, move all element greater than pivot to the left
template<typename KeyType, typename ValueType>
int partition(PairStruct<KeyType, ValueType>* arr, int left, int right)
{
    ValueType pivot = arr[right]._value;  // Select pivot
    int i = left - 1;

    for (int j = left; j < right; j++)
    {
        if (arr[j]._value >= pivot)
        {  // Sorting in descending order
            i++;
            PairStruct temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    // Swap pivot
    PairStruct temp = arr[i + 1];
    arr[i + 1] = arr[right];
    arr[right] = temp;

    return i + 1;
}

// Find k largest elements in the array and put them in the left
template<typename KeyType, typename ValueType>
void findKLargest(PairStruct<KeyType, ValueType>* arr, int k, int dataSize)
{
    if (k <= 0 || k > dataSize)
        return;

    int left = 0;
    int right = dataSize - 1;

    while (left <= right)
    {
        int pivotIndex = partition(arr, left, right);

        // If pivot is same as k, we found k largest elements in the left
        if (pivotIndex == k)
        {
            break;
        }
        else if (pivotIndex > k) // If pivot is greater than k, update the right to find less elements
        {
            right = pivotIndex - 1;
        }
        else // If pivot is lower than k, update the right to find less elements
        {
            left = pivotIndex + 1;
        }
    }
}

template<int maxItems>
class FileItemStorage
{
public:
    void initializeQueue(unsigned char* pWriteBuffer)
    {
        for (int i = 0; i < maxItems; i++)
        {
            mFileItems[i].mpBuffer = NULL;
            mFileItems[i].mpConstBuffer = NULL;
        }
        mCurrentIdx = 0;

        for (int i = 0; i < maxItems; i++)
        {
            mFileItems[i].mSize = 0;
            mFileItems[i].mReservedSize = (1ULL << 63);
            mFileItems[i].mState = FileItem::kFree;
            mFileItems[i].mHaveDirectory = false;
            mFileItems[i].mAge = 0;
        }

        // If external buffer is provided, allow schedule write file later
        if (pWriteBuffer != NULL)
        {
            unsigned char* pBuffer = pWriteBuffer;
            constexpr unsigned long long itemSize = ASYNC_FILE_IO_WRITE_QUEUE_BUFFER_SIZE / maxItems;
            for (int i = 0; i < maxItems; i++, pBuffer += itemSize)
            {
                mFileItems[i].mReservedSize = itemSize;
                mFileItems[i].mpBuffer = pBuffer;
            }
        }
    }

    FileItem* requestFreeSlot(unsigned long long requestedSize)
    {
        for (int i = 0; i < maxItems; i++)
        {
            long long index = ATOMIC_INC64(mCurrentIdx) & (maxItems - 1);
            if (FileItem::kFree == mFileItems[index].mState)
            {
                mFileItems[index].mState = FileItem::kFillingData;
                mFileItems[index].mSize = requestedSize;
                return &mFileItems[index];
            }
        }
        return NULL;
    }
protected:
    // Real write happen here. This function expected call in main thread only. Need to flush all data in queue
    int flushIO(bool isSave, int numberOfProcessedItems = 0)
    {
        ASSERT(isMainProcessor());
        if (numberOfProcessedItems == 1)
        {
            return flushIOMaxPriorityItem(isSave);
        }

        unsigned int processedItemsCount = numberOfProcessedItems > 0 ? numberOfProcessedItems : maxItems;
        // Increase the age of items that waiting for process at current time
        unsigned int numberOfWaitingItems = 0;
        for (unsigned int i = 0; i < maxItems; i++)
        {
            FileItem& item = mFileItems[i];
            if (item.waitForProcess())
            {
                mPriorityArray[numberOfWaitingItems]._key = i;
                // Sort value will use age as indicator.
                // TODO: it should be a combination between file size and age
                mPriorityArray[numberOfWaitingItems]._value = item.mAge;

                // Increase the age of the item
                ATOMIC_INC64(item.mAge);
                numberOfWaitingItems++;
            }
        }

        // Pick k oldest items
        if (numberOfWaitingItems > 0)
        {
            processedItemsCount = numberOfWaitingItems < processedItemsCount ? numberOfWaitingItems : processedItemsCount;
            findKLargest(mPriorityArray, processedItemsCount, numberOfWaitingItems);
            for (unsigned int i = 0; i < processedItemsCount; i++)
            {
                FileItem& item = mFileItems[mPriorityArray[i]._key];
                long long sts = 0;
                if (isSave)
                {
                    sts = save(item.mFileName, item.mSize, item.mpConstBuffer, item.mHaveDirectory ? item.mDirectory : NULL);
                }
                else
                {
                    sts = load(item.mFileName, item.mSize, item.mpBuffer, item.mHaveDirectory ? item.mDirectory : NULL);
                }
                item.markAsDone();
            }
        }

        // Clean up the index
        ATOMIC_AND64(mCurrentIdx, maxItems - 1);

        return (numberOfWaitingItems - processedItemsCount);
    }

    // Flush largest priority item
    int flushIOMaxPriorityItem(bool isSave)
    {
        int numberOfWaitingItems = 0;
        // Increase the age of items that waiting for process at current time
        long long maxAge = -1;
        int maxIdx = -1;
        for (unsigned int i = 0; i < maxItems; i++)
        {
            FileItem& item = mFileItems[i];
            if (item.waitForProcess())
            {
                if (maxAge <= item.mAge)
                {
                    maxAge = item.mAge;
                    maxIdx = i;
                }
                // Increase the age of the item
                ATOMIC_INC64(item.mAge);
                numberOfWaitingItems++;
            }
        }

        // Pick k oldest items
        if (maxIdx >= 0)
        {
            FileItem& item = mFileItems[maxIdx];
            long long sts = 0;
            if (isSave)
            {
                sts = save(item.mFileName, item.mSize, item.mpConstBuffer, item.mHaveDirectory ? item.mDirectory : NULL);
            }
            else
            {
                sts = load(item.mFileName, item.mSize, item.mpBuffer, item.mHaveDirectory ? item.mDirectory : NULL);
            }
            item.markAsDone();
        }
        else
        {
            numberOfWaitingItems = 0;
        }

        // Clean up the index
        ATOMIC_AND64(mCurrentIdx, maxItems - 1);

        return (numberOfWaitingItems - 1);
    }

    // List of files need to be written
    FileItem mFileItems[maxItems];
    long long mCurrentIdx;
    PairStruct<unsigned int, long long> mPriorityArray[maxItems];
};

template<int maxItems>
class LoadFileItemStorage : public FileItemStorage<maxItems>
{
public:
    // Real read happen here. This function expected call in main thread only
    int flushRead(int numberOfProcessedItems = 0)
    {
        ASSERT(isMainProcessor());
        return this->flushIO(false, numberOfProcessedItems);
    }
};

template<int maxItems>
class SaveFileItemStorage : public FileItemStorage<maxItems>
{
public:
    // Real write happen here. This function expected call in main thread only. Need to flush all data in queue
    int flushWrite(int numberOfProcessedItems = 0)
    {
        ASSERT(isMainProcessor());
        return this->flushIO(true, numberOfProcessedItems);
    }
};

class AsyncFileIO
{
public:
    enum Status
    {
        kNoError = 0,
        kUnknown = -1,
        kQueueFull = -2,
        kBufferFull = -3,
        kUnsupported = -4,
        kTimeOut = -5,
        kStop = -6
    };

    bool init(EFI_MP_SERVICES_PROTOCOL* pMPServices, unsigned long long totalWriteSize)
    {
#ifdef NO_UEFI
#else
        mpFileSystemMPServices = pMPServices;

        // Get number of processers and enabled processors
        unsigned long long  enableProcsCount;
        unsigned long long  allProcsCount;
        EFI_STATUS status = mpFileSystemMPServices->GetNumberOfProcessors(mpFileSystemMPServices, &allProcsCount, &enableProcsCount);
        if (EFI_SUCCESS != status)
        {
            logStatusToConsole(L"AsyncFileIO::Can not get number of processors.", status, __LINE__);
            return false;
        }
        int bsProcID = 0;
        for (int i = 0; i < allProcsCount; i++)
        {
            EFI_PROCESSOR_INFORMATION procInfo;
            status = mpFileSystemMPServices->GetProcessorInfo(mpFileSystemMPServices, i, &procInfo);
            if (procInfo.StatusFlag & 0x1)
            {
                mBSProcID = i;
            }
        }
#endif
        mFileBlockingReadQueue.initializeQueue(NULL);
        mFileBlockingWriteQueue.initializeQueue(NULL);
        mEnableNonBlockSave = false;
        mIsStop = false;

        // Init byte buffer and allocate memory
        mpSaveBuffer = NULL;
        if (totalWriteSize > 0)
        {
            bool sts = allocatePool(totalWriteSize, (void**)&mpSaveBuffer);
            if (!sts)
            {
                return false;
            }

            mFileWriteQueue.initializeQueue(mpSaveBuffer);
            setMem(mpSaveBuffer, totalWriteSize, 0);
            mEnableNonBlockSave = true;
        }

        setMem(mRemoveFileNameQueue, sizeof(mRemoveFileNameQueue), 0);
        setMem(mRemoveFileDirQueue, sizeof(mRemoveFileDirQueue), 0);
        mRemoveFilePathQueueCount = 0;
        mRemoveFilePathQueueLock = 0;


        setMem(mCreateDirQueue, sizeof(mCreateDirQueue), 0);
        mCreateDirQueueCount = 0;
        mCreateDirQueueLock = 0;

        return true;
    }

    void deInit()
    {
        // Deinit happen. Does not accept any new jobs
        mIsStop = true;

        // Flush all remained tasks
        flush();
        if (mpSaveBuffer == NULL)
        {
            freePool(mpSaveBuffer);
        }
    }

    bool isMainThread()
    {
#ifdef NO_UEFI
        return false;
#else
        unsigned long long processorNumber;
        mpFileSystemMPServices->WhoAmI(mpFileSystemMPServices, &processorNumber);
        return (mBSProcID == processorNumber);
#endif
    }

    void sleep(unsigned long long ms)
    {
#ifdef NO_UEFI
        _mm_pause();
#else
        bs->Stall(ms);
#endif
    }

    // Function to schedule write
    long long asyncSave(const CHAR16* fileName, unsigned long long totalSize, const unsigned char* buffer, const CHAR16* directory = NULL)
    {
        if (mIsStop)
        {
            return kStop;
        }
        if (!mEnableNonBlockSave)
        {
            return kUnsupported;
        }

        FileItem* pFileItem = mFileWriteQueue.requestFreeSlot(totalSize);
        if (pFileItem == NULL)
        {
            return kQueueFull;
        }

        if (pFileItem->mReservedSize < totalSize)
        {
            return kBufferFull;
        }

        // Non-blocking. Copy data and the write operation happend later
        pFileItem->set(fileName, totalSize, directory);
        copyMem(pFileItem->mpBuffer, buffer, totalSize);
        pFileItem->mpConstBuffer = pFileItem->mpBuffer;
        pFileItem->mState = FileItem::kWait;
        return (long long)totalSize;
    }

    long long asyncBlockingSave(const CHAR16* fileName, unsigned long long totalSize, const unsigned char* buffer, const CHAR16* directory = NULL)
    {
        if (mIsStop)
        {
            return kStop;
        }

        FileItem* pFileItem = mFileBlockingWriteQueue.requestFreeSlot(totalSize);

        if (pFileItem == NULL)
        {
            return kBufferFull;
        }

        // Blocking just steal the buffer
        pFileItem->set(fileName, totalSize, directory);
        pFileItem->mpConstBuffer = buffer;
        pFileItem->mState = FileItem::kBlockingWait;

        // Mainthread. Flush the save queue immediately
        if (isMainThread())
        {
            mFileBlockingWriteQueue.flushWrite();
            return (long long)totalSize;
        }

        // Blocking wait for the save operator finish
        while (!pFileItem->isProcessed() && !mIsStop)
        {
            sleep(1000);
        }
        pFileItem->mState = FileItem::kFree;
        return (long long)totalSize;
    }

    // Function to schedule load. Buffer will be filled data, make sure the buffer is untouched until this function done
    long long asyncLoad(const CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, const CHAR16* directory = NULL)
    {
        // Stop already. Don't process further
        if (mIsStop)
        {
            return kStop;
        }

        int sts = kUnknown;
        bool mainThread = isMainThread();
        FileItem* pFileItem = mFileBlockingReadQueue.requestFreeSlot(totalSize);

        if (pFileItem == NULL)
        {
            return kBufferFull;
        }

        // Get the buffer. Load operation will be execute later in main thread
        pFileItem->set(fileName, totalSize, directory);
        pFileItem->mpBuffer = buffer;
        pFileItem->mState = FileItem::kBlockingWait;

        // In case of main thread. Read immediately.
        if (mainThread)
        {
            mFileBlockingReadQueue.flushRead();
            return (long long)totalSize;
        }

        // Wait for data is processed
        while (!pFileItem->isProcessed() && !mIsStop)
        {
            sleep(1000);
        }
        pFileItem->mState = FileItem::kFree;
        return (long long)totalSize;
    }

    void flushRem()
    {
        ACQUIRE(mRemoveFilePathQueueLock);
        if (mRemoveFilePathQueueCount)
        {
            for (int i = 0; i < mRemoveFilePathQueueCount; i++)
            {
                removeFile(mRemoveFileDirQueue[i], mRemoveFileNameQueue[i]);
            }
            setMem(mRemoveFileNameQueue, sizeof(mRemoveFileNameQueue), 0);
            setMem(mRemoveFileDirQueue, sizeof(mRemoveFileDirQueue), 0);
            mRemoveFilePathQueueCount = 0;
        }
        RELEASE(mRemoveFilePathQueueLock);
    }

    void flushCreateDir()
    {
        ACQUIRE(mCreateDirQueueLock);
        if (mCreateDirQueueCount)
        {
            for (int i = 0; i < mCreateDirQueueCount; i++)
            {
                createDir(mCreateDirQueue[i]);
            }
            setMem(mCreateDirQueue, sizeof(mCreateDirQueue), 0);
            mCreateDirQueueCount = 0;
        }
        RELEASE(mCreateDirQueueLock);
    }

    void asyncRem(const CHAR16* directory, const CHAR16* fileName)
    {
        if (mIsStop)
        {
            return;
        }
        ACQUIRE(mRemoveFilePathQueueLock);
        int index = mRemoveFilePathQueueCount;
        setText(mRemoveFileNameQueue[index], fileName);
        setText(mRemoveFileDirQueue[index], directory);
        mRemoveFilePathQueueCount++;
        RELEASE(mRemoveFilePathQueueLock);

        bool mainThread = isMainThread();
        if (mainThread)
        {
            flushRem();
            return;
        }
        // Note: if this remove operation is used by more modules,
        // consider upgrade this design because mRemoveFilePathQueueCount may never hit 0
        while (mRemoveFilePathQueueCount != 0)
        {
            sleep(10);
        }
    }

    void asyncCreateDir(const CHAR16* directory)
    {
        if (mIsStop)
        {
            return;
        }

        ACQUIRE(mCreateDirQueueLock);
        int index = mCreateDirQueueCount;
        setText(mCreateDirQueue[index], directory);
        mCreateDirQueueCount++;
        RELEASE(mCreateDirQueueLock);

        bool mainThread = isMainThread();
        if (mainThread)
        {
            flushCreateDir();
            return;
        }
        // Note: if this remove operation is used by more modules,
        // consider upgrade this design because mCreateDirQueueCount may never hit 0
        while (mCreateDirQueueCount != 0)
        {
            sleep(10);
        }
    }

    int flush(int numberOfItemsPerQueue = 0)
    {
        int remainedItems = 0;
        remainedItems = remainedItems + mFileBlockingReadQueue.flushRead(numberOfItemsPerQueue);
        remainedItems = remainedItems + mFileBlockingWriteQueue.flushWrite(numberOfItemsPerQueue);
        if (mEnableNonBlockSave)
        {
            remainedItems = remainedItems + mFileWriteQueue.flushWrite(numberOfItemsPerQueue);
        }
        flushRem();
        flushCreateDir();
        return remainedItems;
    }

private:
    EFI_MP_SERVICES_PROTOCOL* mpFileSystemMPServices;
    unsigned int mBSProcID;

    // Buffer for scheduler write
    unsigned char* mpSaveBuffer;

    bool mEnableNonBlockSave;
    bool mIsStop;

    // File queue
    SaveFileItemStorage<ASYNC_FILE_IO_MAX_QUEUE_ITEMS> mFileWriteQueue;
    SaveFileItemStorage<ASYNC_FILE_IO_BLOCKING_MAX_QUEUE_ITEMS> mFileBlockingWriteQueue;
    LoadFileItemStorage<ASYNC_FILE_IO_BLOCKING_MAX_QUEUE_ITEMS> mFileBlockingReadQueue;

    // remove queue: rare operation, only need a simple queue
    CHAR16 mRemoveFileNameQueue[1024][1024];
    CHAR16 mRemoveFileDirQueue[1024][1024];
    int mRemoveFilePathQueueCount;
    volatile char mRemoveFilePathQueueLock;

    // Create directory queue: rare operation, only need a simple queue
    CHAR16 mCreateDirQueue[1024][1024];
    int mCreateDirQueueCount;
    volatile char mCreateDirQueueLock;
};

OPTIMIZE_ON()

// Asynchorous save file
// This function can be called from any thread and have blocking and non blocking mode
// - Blocking mode: to avoid lock and the actual save happen, flushAsyncFileIOBuffer must be called in main thread
// - non-blocking mode: return immediately, the save operation happen in flushAsyncFileIOBuffer
static long long asyncSave(const CHAR16* fileName, unsigned long long totalSize, const unsigned char* buffer, const CHAR16* directory = NULL, bool blocking = true)
{
    if (gAsyncFileIO)
    {
        if (blocking)
        {
            return gAsyncFileIO->asyncBlockingSave(fileName, totalSize, buffer, directory);
        }
        else
        {
            return gAsyncFileIO->asyncSave(fileName, totalSize, buffer, directory);
        }
    }

    return (long long)AsyncFileIO::kUnknown;
}

// Asynchorous load a file
// This function can be called from any thread and is a blocking function
// To avoid lock and the actual load happen, flushAsyncFileIOBuffer must be called in main thread
static long long asyncLoad(const CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, const CHAR16* directory = NULL)
{
    if (gAsyncFileIO)
    {
        return gAsyncFileIO->asyncLoad(fileName, totalSize, buffer, directory);
    }
    return 0;
}

// Asynchorous remove a file
// This function can be called from any thread and is a blocking function
// To avoid lock and the actual remove happen, flushAsyncFileIOBuffer must be called in main thread
static long long asyncRemoveFile(CHAR16* fileName, CHAR16* directory = NULL)
{
    if (!fileName)
    {
        return -1;
    }
    if (gAsyncFileIO)
    {
        gAsyncFileIO->asyncRem(directory, fileName);
        return 0;
    }
    // the only case that gAsyncFileIO == NULL is when main thread initializing => can run rem file directly
    else if (removeFile(directory, fileName))
    {
        return 0;
    }
    return -1;
}

// Asynchorous create a dir if it doesn't exist
// This function can be called from any thread and is a blocking function
static long long asyncCreateDir(const CHAR16* directory)
{
    if (!directory)
    {
        return -1;
    }
    if (gAsyncFileIO)
    {
        gAsyncFileIO->asyncCreateDir(directory);
        return 0;
    }
    // the only case that gAsyncFileIO == NULL is when main thread initializing => can run create dir directly
    else if (createDir(directory))
    {
        return 0;
    }
    return -1;
}

static bool initFilesystem()
{
#ifdef NO_UEFI
    return true;
#else
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

                freePool(handles);

                return false;
            }
            else
            {
                if (status = simpleFileSystemProtocol->OpenVolume(simpleFileSystemProtocol, (void**)&root))
                {
                    logStatusToConsole(L"EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume() fails", status, __LINE__);

                    bs->CloseProtocol(handles[i], &simpleFileSystemProtocolGuid, ih, NULL);
                    freePool(handles);

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
                        freePool(handles);

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

        freePool(handles);
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
#endif
}

static void registerAsynFileIO(EFI_MP_SERVICES_PROTOCOL* pServiceProtocol)
{
    allocatePool(sizeof(AsyncFileIO), (void**)(&gAsyncFileIO));
    setMem(gAsyncFileIO, sizeof(AsyncFileIO), 0);
    gAsyncFileIO->init(pServiceProtocol, ASYNC_FILE_IO_WRITE_QUEUE_BUFFER_SIZE);
}

OPTIMIZE_OFF()

static void deInitFileSystem()
{
    if (gAsyncFileIO)
    {
        gAsyncFileIO->deInit();
        freePool(gAsyncFileIO);
        gAsyncFileIO = NULL;
    }
}

static int flushAsyncFileIOBuffer(int numberOfItemsPerQueue = 0)
{
    if (gAsyncFileIO)
    {
        return gAsyncFileIO->flush(numberOfItemsPerQueue);
    }
    return 0;
}
OPTIMIZE_ON()

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

// Break the large file to many chunks to write if the size is greater or equal FILE_CHUNK_SIZE
// - skipWriteEqualChunkSize: skip write the chunk file if the size of existed file match with buffer data. Set false if need the write always happens
static long long saveLargeFile(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, CHAR16* directory = NULL, bool skipWriteEqualChunkSize = true)
{
    const unsigned long long maxWriteSizePerChunk = FILE_CHUNK_SIZE;
    if (totalSize < maxWriteSizePerChunk)
    {
        return save(fileName, totalSize, buffer, directory);
    }
    int chunkId = 0;
    unsigned long long totalWriteSize = 0;
    while (totalSize)
    {
        CHAR16 fileNameWithChunkId[64];
        setText(fileNameWithChunkId, fileName);
        appendText(fileNameWithChunkId, L".XXX");
        addEpochToFileName(fileNameWithChunkId, getTextSize(fileNameWithChunkId, 64) + 1, chunkId);
        const unsigned long long writeSize = maxWriteSizePerChunk < totalSize ? maxWriteSizePerChunk : totalSize;
        if (!skipWriteEqualChunkSize || (getFileSize(fileNameWithChunkId, directory) != writeSize))
        {
            unsigned long long res = save(fileNameWithChunkId, writeSize, buffer, directory);
            if (res != writeSize)
            {
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
    if (totalSize < maxReadSizePerChunk)
    {
        return load(fileName, totalSize, buffer, directory);
    }
    int chunkId = 0;
    unsigned long long totalReadSize = 0;
    while (totalSize)
    {
        CHAR16 fileNameWithChunkId[64];
        setText(fileNameWithChunkId, fileName);
        appendText(fileNameWithChunkId, L".XXX");
        addEpochToFileName(fileNameWithChunkId, getTextSize(fileNameWithChunkId, 64) + 1, chunkId);
        const unsigned long long readSize = maxReadSizePerChunk < totalSize ? maxReadSizePerChunk : totalSize;
        unsigned long long res = load(fileNameWithChunkId, readSize, buffer, directory);
        if (res != readSize)
        {
            return totalReadSize;
        }
        buffer += readSize;
        totalReadSize += readSize;
        totalSize -= readSize;
        chunkId++;
    }
    return totalReadSize;
}

// Asynchorous load a large file
// File with size greater than FILE_CHUNK_SIZE will be break into smaller file to be written. So this function will load the smaller chunks
// into a large bugger
// This function can be called from any thread and have blocking and non blocking mode
// - Blocking mode: to avoid lock and the actual save happen, flushAsyncFileIOBuffer must be called in main thread
// - non-blocking mode: return immediately, the save operation happen in flushAsyncFileIOBuffer
static long long asyncSaveLargeFile(
    CHAR16* fileName,
    unsigned long long totalSize,
    unsigned char* buffer,
    CHAR16* directory = NULL,
    bool skipWriteEqualChunkSize = true,
    bool blocking = true)
{
    const unsigned long long maxWriteSizePerChunk = FILE_CHUNK_SIZE;
    if (totalSize < maxWriteSizePerChunk)
    {
        return asyncSave(fileName, totalSize, buffer, directory, blocking);
    }
    int chunkId = 0;
    unsigned long long totalWriteSize = 0;
    while (totalSize)
    {
        CHAR16 fileNameWithChunkId[64];
        setText(fileNameWithChunkId, fileName);
        appendText(fileNameWithChunkId, L".XXX");
        addEpochToFileName(fileNameWithChunkId, getTextSize(fileNameWithChunkId, 64) + 1, chunkId);
        const unsigned long long writeSize = maxWriteSizePerChunk < totalSize ? maxWriteSizePerChunk : totalSize;
        long long res = asyncSave(fileNameWithChunkId, writeSize, buffer, directory, blocking);
        if (res != writeSize)
        {
            return totalWriteSize;
        }
        buffer += writeSize;
        totalWriteSize += writeSize;
        totalSize -= writeSize;
        chunkId++;
    }
    return totalWriteSize;
}

// Asynchorous load a large file
// File with size greater than FILE_CHUNK_SIZE will be break into smaller file to be written. So this function will load the smaller chunks
// into a large bugger
// This function is blocking call and can be called from any thread
// To avoid lock and the actual load happen, flushAsyncFileIOBuffer must be called in main thread
static long long asyncLoadLargeFile(CHAR16* fileName, unsigned long long totalSize, unsigned char* buffer, CHAR16* directory = NULL)
{
    const unsigned long long maxReadSizePerChunk = FILE_CHUNK_SIZE;
    if (totalSize < maxReadSizePerChunk)
    {
        return asyncLoad(fileName, totalSize, buffer, directory);
    }
    int chunkId = 0;
    unsigned long long totalReadSize = 0;
    while (totalSize)
    {
        CHAR16 fileNameWithChunkId[64];
        setText(fileNameWithChunkId, fileName);
        appendText(fileNameWithChunkId, L".XXX");
        addEpochToFileName(fileNameWithChunkId, getTextSize(fileNameWithChunkId, 64) + 1, chunkId);
        const unsigned long long readSize = maxReadSizePerChunk < totalSize ? maxReadSizePerChunk : totalSize;
        unsigned long long res = asyncLoad(fileNameWithChunkId, readSize, buffer, directory);
        if (res != readSize)
        {
            return totalReadSize;
        }
        buffer += readSize;
        totalReadSize += readSize;
        totalSize -= readSize;
        chunkId++;
    }
    return totalReadSize;
}