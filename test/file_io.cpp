#define NO_UEFI

#include "gtest/gtest.h"


#include <iostream>
#include <thread>
#include <mutex>
#include <fstream>
#include <chrono>

#include "../src/platform/file_io.h"

static constexpr unsigned long long THREAD_COUNT = 4;
static constexpr unsigned long long MEM_BUFFER_SIZE = 52ULL * 1024ULL * 1024ULL;
std::mutex gMessageLock;

// For testing the scheduler save file
struct FragmentData
{
    // Reserve memories
    unsigned int memBuffer[MEM_BUFFER_SIZE];

    // Randomly parition of data for writing
    unsigned long long dataPos[4][2];
};

static std::vector<FragmentData> threadData;
static FragmentData buffer;
static unsigned char threadFinish[THREAD_COUNT];

inline static unsigned int random(const unsigned int range)
{
    unsigned int value;
    _rdrand32_step(&value);

    return value % range;
}

inline static unsigned long long random64(const unsigned long long range)
{
    unsigned long long value;
    _rdrand64_step(&value);

    return (value % range);
}

class FileSystemWrapper
{
public:
    FileSystemWrapper()
    {
        initFilesystem();
        registerAsynFileIO(NULL);
    }
    ~FileSystemWrapper()
    {
        deInitFileSystem();
    }

    bool initTestData()
    {
        memset(threadFinish, 1, THREAD_COUNT);

        threadData.resize(THREAD_COUNT);
        for (int id = 0; id < THREAD_COUNT; id++)
        {
            // randomly generate a chunk of data
            for (unsigned long long i = 0; i < MEM_BUFFER_SIZE; i++)
            {
                threadData[id].memBuffer[i] = random(4096) * ((int)i + 1);
            }
            // Randomly pick some part of data for writing out
            unsigned long long remainedData = MEM_BUFFER_SIZE;
            for (int i = 0; i < sizeof(threadData[id].dataPos) / sizeof(threadData[id].dataPos[0]); i++)
            {
                // Start of the data
                threadData[id].dataPos[i][0] = random64(MEM_BUFFER_SIZE - 1);

                // Size of the data. Make sure we limit all small files in size of total MEM_BUFFER_SIZE
                unsigned long long dataSize = random64(MEM_BUFFER_SIZE - threadData[id].dataPos[i][0]);
                dataSize = dataSize > remainedData ? remainedData : dataSize;
                remainedData = remainedData - dataSize;

                if (dataSize == 0)
                {
                    dataSize = 1;
                }

                threadData[id].dataPos[i][1] = dataSize;
            }
        }
        return true;
    }
};

static FileSystemWrapper fileSystem;

long long loadFile(CHAR16* fileName, unsigned long long totalSize, char* buffer)
{
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
}

long long saveFile(CHAR16* fileName, unsigned long long totalSize, const char* buffer)
{
    FILE* file = nullptr;
    if (_wfopen_s(&file, fileName, L"wb") != 0 || !file)
    {
        wprintf(L"Error opening file %s!\n", fileName);
        return -1;
    }
    if (fwrite(buffer, 1, totalSize, file) != totalSize)
    {
        wprintf(L"Error saving %llu bytes from %s!\n", totalSize, fileName);
        return -1;
    }
    fclose(file);
    return totalSize;
}

bool runAsyncSaveFile(int id, bool blocking = true, bool largeFile = false)
{
    bool sts = true;
    CHAR16 fileName[32];
    setText(fileName, L"tmp_file_");
    appendNumber(fileName, id, false);

    if (largeFile)
    {
        long long sts = asyncSaveLargeFile(fileName, MEM_BUFFER_SIZE * sizeof(unsigned int), (unsigned char*)&(threadData[id].memBuffer[0]), NULL, false, blocking);
        if (sts <= 0)
        {
            std::lock_guard<std::mutex> lock(gMessageLock);
            std::cout << "runAsyncSaveFile::saveFile failed with size " << MEM_BUFFER_SIZE * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
            sts = false;
        }
    }
    else
    {
        // Try to save the files
        for (int i = 0; i < sizeof(threadData[id].dataPos) / sizeof(threadData[id].dataPos[0]); i++)
        {
            unsigned long long dataStart = threadData[id].dataPos[i][0];
            unsigned long long dataCount = threadData[id].dataPos[i][1];

            CHAR16 partionFileName[256];
            setText(partionFileName, fileName);
            appendText(partionFileName, L".");
            appendNumber(partionFileName, i, false);

            long long sts = asyncSave(partionFileName, dataCount * sizeof(unsigned int), (unsigned char*)&(threadData[id].memBuffer[dataStart]), NULL, blocking);
            if (sts <= 0)
            {
                std::lock_guard<std::mutex> lock(gMessageLock);
                std::cout << "runAsyncSaveFile::saveFile failed with size " << dataCount * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
                sts = false;
                break;
            }
        }
    }

    threadFinish[id] = 1;
    return sts;
}

bool prepareAsyncLoadFile(bool largeFile = false)
{
    bool sts = true;
    fileSystem.initTestData();

    for (int id = 0; id < THREAD_COUNT; id++)
    {
        CHAR16 fileName[32];
        setText(fileName, L"tmp_file_");
        appendNumber(fileName, id, false);

        if (largeFile)
        {
            long long sts = saveLargeFile(fileName, MEM_BUFFER_SIZE * sizeof(unsigned int), (unsigned char*)&(threadData[id].memBuffer[0]), NULL, false);
            if (sts <= 0)
            {
                std::lock_guard<std::mutex> lock(gMessageLock);
                std::cout << "prepareAsyncLoadFile::saveFile failed with size " << MEM_BUFFER_SIZE * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
                sts = false;
            }
        }
        else
        {
            // Try to save the files
            for (int i = 0; i < sizeof(threadData[id].dataPos) / sizeof(threadData[id].dataPos[0]); i++)
            {
                unsigned long long dataStart = threadData[id].dataPos[i][0];
                unsigned long long dataCount = threadData[id].dataPos[i][1];

                CHAR16 partionFileName[256];
                setText(partionFileName, fileName);
                appendText(partionFileName, L".");
                appendNumber(partionFileName, i, false);

                long long sts = save(partionFileName, dataCount * sizeof(unsigned int), (unsigned char*)&(threadData[id].memBuffer[dataStart]), NULL);
                if (sts <= 0)
                {
                    std::lock_guard<std::mutex> lock(gMessageLock);
                    std::cout << "prepareAsyncLoadFile::saveFile failed with size " << dataCount * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
                    sts = false;
                    break;
                }
            }
        }
    }
    return sts;
}

bool runAsyncLoadFile(int id, bool largeFile = false)
{
    bool sts = true;
    CHAR16 fileName[32];
    setText(fileName, L"tmp_file_");
    appendNumber(fileName, id, false);

    if (largeFile)
    {
        long long sts = asyncLoadLargeFile(fileName, MEM_BUFFER_SIZE * sizeof(unsigned int), (unsigned char*)&(threadData[id].memBuffer[0]), NULL);
        if (sts <= 0)
        {
            std::lock_guard<std::mutex> lock(gMessageLock);
            std::cout << "runAsyncLoadFile::loadFile failed with size " << MEM_BUFFER_SIZE * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
            sts = false;
        }
    }
    else
    {
        // Try to load the files
        for (int i = 0; i < sizeof(threadData[id].dataPos) / sizeof(threadData[id].dataPos[0]); i++)
        {
            unsigned long long dataStart = threadData[id].dataPos[i][0];
            unsigned long long dataCount = threadData[id].dataPos[i][1];

            CHAR16 partionFileName[256];
            setText(partionFileName, fileName);
            appendText(partionFileName, L".");
            appendNumber(partionFileName, i, false);

            long long sts = asyncLoad(partionFileName, dataCount * sizeof(unsigned int), (unsigned char*)&(threadData[id].memBuffer[dataStart]), NULL);
            if (sts <= 0)
            {
                std::lock_guard<std::mutex> lock(gMessageLock);
                std::cout << "runAsyncLoadFile::loadFile failed with size " << dataCount * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
                sts = false;
                break;
            }
        }
    }

    threadFinish[id] = 1;
    return sts;
}


bool verifyResult(int id, bool largeFile = false)
{
    bool testPass = false;
    CHAR16 fileName[32];
    setText(fileName, L"tmp_file_");
    appendNumber(fileName, id, false);

    if (largeFile)
    {
        long long sts = loadLargeFile(fileName, MEM_BUFFER_SIZE * sizeof(unsigned int), (unsigned char*)buffer.memBuffer, NULL);
        unsigned char* originalData = (unsigned char*)&(threadData[id].memBuffer[0]);
        unsigned char* loadedData = (unsigned char*)&(buffer.memBuffer[0]);
        int result = memcmp(originalData, loadedData, MEM_BUFFER_SIZE * sizeof(unsigned int));
        testPass = (result == 0);
    }
    else
    {
        int matchCount = 0;
        int numberOfFiles = sizeof(threadData[id].dataPos) / sizeof(threadData[id].dataPos[0]);
        for (int i = 0; i < numberOfFiles; i++)
        {
            unsigned long long dataStart = threadData[id].dataPos[i][0];
            unsigned long long dataCount = threadData[id].dataPos[i][1];

            CHAR16 partionFileName[256];
            setText(partionFileName, fileName);
            appendText(partionFileName, L".");
            appendNumber(partionFileName, i, false);

            long long sts = loadFile(partionFileName, dataCount * sizeof(unsigned int), (char*)buffer.memBuffer);

            if (sts != dataCount * sizeof(unsigned int))
            {
                std::cout << "verifyResult failed with size " << dataCount * sizeof(unsigned int) / 1024 << " KB. Error " << sts << std::endl;
                return false;
            }

            unsigned char* originalData = (unsigned char*)&(threadData[id].memBuffer[dataStart]);
            unsigned char* loadedData = (unsigned char*)&(buffer.memBuffer[0]);

            int result = memcmp(originalData, loadedData, dataCount * sizeof(unsigned int));
            if (result == 0)
            {
                matchCount++;
            }
        }
        testPass = (matchCount == numberOfFiles);
    }

    return testPass;
}

#if 0

TEST(TestAsyncFileIO, AsyncNonBlockingSaveFile)
{
    fileSystem.initTestData();

    // Run the test
    std::vector<std::unique_ptr<std::thread>> threadVec(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadFinish[i] = 0;
        threadVec[i].reset(new std::thread(runAsyncSaveFile, i, false, false));
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (threadVec[i]->joinable())
        {
            threadVec[i]->join();
        }
    }
    // Actually write happen here
    flushAsyncFileIOBuffer();

    // Verify result
    int testPass = 0;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (verifyResult(i))
        {
            testPass++;
        }
    }
    EXPECT_EQ(testPass, THREAD_COUNT);
}

TEST(TestAsyncFileIO, AsyncNonBlockingSaveLargeFile)
{
    fileSystem.initTestData();

    // Run the test
    std::vector<std::unique_ptr<std::thread>> threadVec(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadFinish[i] = 0;
        threadVec[i].reset(new std::thread(runAsyncSaveFile, i, false, true));
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (threadVec[i]->joinable())
        {
            threadVec[i]->join();
        }
    }
    // Actually write happen here
    flushAsyncFileIOBuffer();

    // Verify result
    int testPass = 0;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (verifyResult(i, true))
        {
            testPass++;
        }
    }
    EXPECT_EQ(testPass, THREAD_COUNT);
}

#endif

TEST(TestAsyncFileIO, AsyncSaveFile)
{
    fileSystem.initTestData();

    // Run the test
    std::vector<std::unique_ptr<std::thread>> threadVec(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadFinish[i] = 0;
        threadVec[i].reset(new std::thread(runAsyncSaveFile, i, true, false));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    int readyCount = 0;
    while (readyCount < THREAD_COUNT)
    {
        // Don't flush right away. Wait sometimes for simulate
        unsigned long long waitingTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
        if (waitingTimeInMs > 10000)
        {
            startTime = std::chrono::high_resolution_clock::now();
            flushAsyncFileIOBuffer();
        }

        readyCount = 0;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            char readyFlag = 0;
            readyFlag = threadFinish[i];
            if (readyFlag)
            {
                readyCount++;
            }
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (threadVec[i]->joinable())
        {
            threadVec[i]->join();
        }
    }

    // Verify result
    int testPass = 0;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (verifyResult(i))
        {
            testPass++;
        }
    }
    EXPECT_EQ(testPass, THREAD_COUNT);
}

TEST(TestAsyncFileIO, AsyncSaveLargeFile)
{
    fileSystem.initTestData();

    // Run the test
    std::vector<std::unique_ptr<std::thread>> threadVec(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadFinish[i] = 0;
        threadVec[i].reset(new std::thread(runAsyncSaveFile, i, true, true));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    int readyCount = 0;
    while (readyCount < THREAD_COUNT)
    {
        // Don't flush right away. Wait sometimes for simulate
        unsigned long long waitingTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
        if (waitingTimeInMs > 10000)
        {
            startTime = std::chrono::high_resolution_clock::now();
            flushAsyncFileIOBuffer();
        }

        readyCount = 0;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            char readyFlag = 0;
            readyFlag = threadFinish[i];
            if (readyFlag)
            {
                readyCount++;
            }
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (threadVec[i]->joinable())
        {
            threadVec[i]->join();
        }
    }

    // Verify result
    int testPass = 0;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (verifyResult(i, true))
        {
            testPass++;
        }
    }
    EXPECT_EQ(testPass, THREAD_COUNT);
}

TEST(TestAsyncFileIO, AsyncLoadFile)
{
    prepareAsyncLoadFile(false);

    // Run the test
    std::vector<std::unique_ptr<std::thread>> threadVec(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadFinish[i] = 0;
        threadVec[i].reset(new std::thread(runAsyncLoadFile, i, false));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    int readyCount = 0;
    while (readyCount < THREAD_COUNT)
    {
        // Don't flush right away. Wait sometimes for simulate
        unsigned long long waitingTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
        if (waitingTimeInMs > 10000)
        {
            startTime = std::chrono::high_resolution_clock::now();
            flushAsyncFileIOBuffer();
        }

        readyCount = 0;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            char readyFlag = 0;
            readyFlag = threadFinish[i];
            if (readyFlag)
            {
                readyCount++;
            }
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (threadVec[i]->joinable())
        {
            threadVec[i]->join();
        }
    }

    // Verify result
    int testPass = 0;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (verifyResult(i))
        {
            testPass++;
        }
    }
    EXPECT_EQ(testPass, THREAD_COUNT);
}

TEST(TestAsyncFileIO, AsyncLoadLargeFile)
{
    prepareAsyncLoadFile(true);

    // Run the test
    std::vector<std::unique_ptr<std::thread>> threadVec(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadFinish[i] = 0;
        threadVec[i].reset(new std::thread(runAsyncLoadFile, i, true));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    int readyCount = 0;
    while (readyCount < THREAD_COUNT)
    {
        // Don't flush right away. Wait sometimes for simulate
        unsigned long long waitingTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
        if (waitingTimeInMs > 10000)
        {
            startTime = std::chrono::high_resolution_clock::now();
            flushAsyncFileIOBuffer();
        }

        readyCount = 0;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            char readyFlag = 0;
            readyFlag = threadFinish[i];
            if (readyFlag)
            {
                readyCount++;
            }
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (threadVec[i]->joinable())
        {
            threadVec[i]->join();
        }
    }

    // Verify result
    int testPass = 0;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (verifyResult(i, true))
        {
            testPass++;
        }
    }
    EXPECT_EQ(testPass, THREAD_COUNT);
}
