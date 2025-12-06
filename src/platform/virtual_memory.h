#pragma once

#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/time.h"
#include "platform/memory_util.h"
#include "platform/debugging.h"
#include "platform/file_io.h"

#include "four_q.h"
#include "kangaroo_twelve.h"

template <class T>
inline constexpr const T& max(const T& left, const T& right)
{
    return (left < right) ? right : left;
}

template <class T>
inline constexpr const T& min(const T& left, const T& right)
{
    return (left < right) ? left : right;
}

// an util to use disk as RAM to reduce hardware requirement for qubic core node
// this VirtualMemory doesn't (yet) support amend operation. That means data stay persisted once they are written
// template variables meaning:
// prefixName is used for generating page file names on disk, it must be unique if there are multiple VirtualMemory instances
// pageCapacity is number of items (T) inside a page
// it stores (numCachePage) pages on RAM for faster loading (the strategy mimics CPU cache lines)
// this class can be used to debug illegal memory access issue
template <typename T, unsigned long long prefixName, unsigned long long pageDirectory, unsigned long long pageCapacity = 100000, unsigned long long numCachePage = 128>
class VirtualMemory
{
protected:
    unsigned long long pageSize = sizeof(T) * pageCapacity;

    // on RAM
    T* currentPage = NULL; // current page is cache[0]
    T* cache[numCachePage + 1];
    CHAR16* pageDir = NULL;

    unsigned long long cachePageId[numCachePage + 1];
    unsigned long long lastAccessedTimestamp[numCachePage + 1]; // in millisecond
    unsigned long long currentId; // total items in this array, aka: latest item index + 1
    unsigned long long currentPageId; // current page index that's written on

    volatile char memLock; // every read/write needs a memory lock, can optimize later

    void generatePageName(CHAR16 pageName[64], unsigned long long page_id)
    {
        setMem(pageName, sizeof(pageName), 0);
        char input[32];
        setMem(input, sizeof(input), 0);
        unsigned long long prefix_name = prefixName;
        copyMem(input, &prefix_name, 8);
        copyMem(input + 8, &page_id, 8);
        unsigned char digest[32];
        KangarooTwelve(input, 32, digest, 32);
        getIdentity(digest, pageName, true);
        setMem(pageName + 8, 8, 0); // too long file name will cause crash on some system
        appendText(pageName, L".pg");
    }

    void writeCurrentPageToDisk()
    {
        CHAR16 pageName[64];
        generatePageName(pageName, currentPageId);
#if defined(NO_UEFI) && !defined(REAL_NODE)
        auto sz = save(pageName, pageSize, (unsigned char*)currentPage, pageDir);
#else
        auto sz = asyncSave(pageName, pageSize, (unsigned char*)currentPage, pageDir, true);
#endif

#if !defined(NDEBUG)
        if (sz != pageSize)
        {
            addDebugMessage(L"Failed to store virtualMemory to disk. Old data maybe lost");
        }
        else
        {
            CHAR16 debugMsg[128];
            unsigned long long tmp = prefixName;
            debugMsg[0] = L'[';
            copyMem(debugMsg + 1, &tmp, 8);
            debugMsg[5] = L']';
            debugMsg[6] = L' ';
            debugMsg[7] = 0;
            appendText(debugMsg, L"page ");
            appendNumber(debugMsg, currentPageId, true);
            appendText(debugMsg, L" is written into disk");
            addDebugMessage(debugMsg);
        }
#endif
    }

    // return the most outdated cache page
    int getMostOutdatedCachePage()
    {
        // i = 1 because 0 is used for current page
        int min_index = 1;
        for (int i = 1; i <= numCachePage; i++)
        {
            if (lastAccessedTimestamp[i] == 0)
            {
                return i;
            }
            if ((lastAccessedTimestamp[i] < lastAccessedTimestamp[min_index]) || (lastAccessedTimestamp[i] == lastAccessedTimestamp[min_index] && cachePageId[i] < cachePageId[min_index]))
            {
                min_index = i;
            }
        }
        return min_index;
    }

    void copyCurrentPageToCache()
    {
        int cache_slot_idx = getMostOutdatedCachePage();
        copyMem(cache[cache_slot_idx], currentPage, pageSize);
        lastAccessedTimestamp[cache_slot_idx] = now_ms();
        cachePageId[cache_slot_idx] = currentPageId;
#ifndef NDEBUG
        {
            CHAR16 debugMsg[128];
            unsigned long long tmp = prefixName;
            debugMsg[0] = L'[';
            copyMem(debugMsg + 1, &tmp, 8);
            debugMsg[5] = L']';
            debugMsg[6] = L' ';
            debugMsg[7] = 0;
            appendText(debugMsg, L"page ");
            appendNumber(debugMsg, currentPageId, true);
            appendText(debugMsg, L" is moved to cache slot ");
            appendNumber(debugMsg, cache_slot_idx, true);
            addDebugMessage(debugMsg);
        }
#endif
    }

    void cleanCurrentPage()
    {
        setMem(currentPage, pageSize, 0);
    }

    // return cache id given cache_page_id
    int findCachePage(unsigned long long requested_page_id)
    {
        // TODO: consider implementing a tree here for faster search
        for (int i = 0; i <= numCachePage; i++)
        {
            if (cachePageId[i] == requested_page_id)
            {
                lastAccessedTimestamp[i] = now_ms();
                return i;
            }
        }
        return -1;
    }

    // load a page from disk to cache
    // if page is already on cache, return the id
    // return cache index
    int loadPageToCache(unsigned long long pageId)
    {
        int cache_page_id = findCachePage(pageId);

        if (cache_page_id != -1)
        {
            return cache_page_id;
        }
        CHAR16 pageName[64];
        generatePageName(pageName, pageId);
        cache_page_id = getMostOutdatedCachePage();
#if defined(NO_UEFI) && !defined(REAL_NODE)
        auto sz = load(pageName, pageSize, (unsigned char*)cache[cache_page_id], pageDir);
        lastAccessedTimestamp[cache_page_id] = now_ms();
#else
#if !defined(NDEBUG)
        {
            CHAR16 debugMsg[128];
            setText(debugMsg, L"Trying to load OLD page: ");
            appendNumber(debugMsg, pageId, true);
            addDebugMessage(debugMsg);
        }
#endif
        auto sz = asyncLoad(pageName, pageSize, (unsigned char*)cache[cache_page_id], pageDir);
        if (sz != pageSize)
        {
#if !defined(NDEBUG)
            addDebugMessage(L"Failed to load virtualMemory from disk");
#endif
            return -1;
        }
        lastAccessedTimestamp[cache_page_id] = now_ms();
#endif
        cachePageId[cache_page_id] = pageId;
#if !defined(NDEBUG)
        {
            CHAR16 debugMsg[128];
            unsigned long long tmp = prefixName;
            debugMsg[0] = L'[';
            copyMem(debugMsg + 1, &tmp, 8);
            debugMsg[5] = L']';
            debugMsg[6] = L' ';
            debugMsg[7] = 0;
            appendText(debugMsg, L"Load complete. Page ");
            appendNumber(debugMsg, pageId, true);
            appendText(debugMsg, L" is loaded into slot ");
            appendNumber(debugMsg, cache_page_id, true);
            addDebugMessage(debugMsg);
        }
#endif
        return cache_page_id;
    }

    // only call after append
    // check if current page is full
    // if yes, write current page to disk and cache
    // then clean current page
    void tryPersistingPage()
    {
        if (currentId % pageCapacity == 0)
        {
            writeCurrentPageToDisk();
            copyCurrentPageToCache();
            cleanCurrentPage();
            cachePageId[0] = currentId / pageCapacity;
            currentPageId++;
        }
    }

    void reset()
    {
        setMem(currentPage, pageSize * (numCachePage + 1), 0);
        setMem(cachePageId, sizeof(cachePageId), 0xff);
        setMem(lastAccessedTimestamp, sizeof(lastAccessedTimestamp), 0);
        cachePageId[0] = 0;
        currentId = 0;
        currentPageId = 0;
        memLock = 0;
    }

public:
    VirtualMemory()
    {
        memLock = 0;
    }

    bool init()
    {
        ACQUIRE(memLock);
        if (currentPage == NULL)
        {
            if (!allocPoolWithErrorLog(L"VirtualMemory.Page", pageSize * (numCachePage + 1), (void**)&currentPage, __LINE__))
            {
                return false;
            }
            cache[0] = currentPage;
            for (int i = 1; i <= numCachePage; i++)
            {
                char *tmp = (char*)cache[0];
                cache[i] = (T*)(tmp + i * pageSize);
            }
        }

        if (pageDir == NULL)
        {
            if (prefixName != 0 && pageDirectory != 0)
            {
                if (!allocPoolWithErrorLog(L"PageDir", 32, (void**)&pageDir, __LINE__))
                {
                    return false;
                }
                setMem(pageDir, sizeof(pageDir), 0);
                unsigned long long tmp = prefixName;
                copyMem(pageDir, &tmp, 8);
                tmp = pageDirectory;
                copyMem(pageDir + 4, &tmp, 8);
                appendText(pageDir, L".");
            }
        }

        if (pageDir != NULL)
        {
#ifdef REAL_NODE
            addEpochToFileName(pageDir, 12, max(EPOCH, int(system.epoch)));
#else
			addEpochToFileName(pageDir, 12, 0);
#endif
            if (asyncCreateDir(pageDir) != 0)
            {
#if !defined(NDEBUG)
                CHAR16 dbg[128];
                setText(dbg, L"WARNING: Failed to create dir ");
                appendText(dbg, pageDir);
                addDebugMessage(dbg);
#endif
            }
        }

        reset();
        RELEASE(memLock);
        return true;
    }
    void deinit()
    {
        if (currentPage != NULL)
        {
            freePool(currentPage);
            currentPage = NULL;
        }
        if (pageDir != NULL)
        {
            freePool(pageDir);
            pageDir = NULL;
        }
    }

    // getMany: 
    // this operation copies numItems * sizeof(T) bytes from [src+offset] to [dst] - note that src and dst are T* not char*
    // offset + numItems must be less than currentId
    // return number of items has been copied
    unsigned long long getMany(T* dst, unsigned long long offset, unsigned long long numItems)
    {
        ACQUIRE(memLock);
        ASSERT(offset + numItems - 1 < currentId);
        if (offset + numItems - 1 >= currentId)
        {
            RELEASE(memLock);
            return 0;
        }
        RELEASE(memLock);

        // processing
        unsigned long long c_bytes = 0;
        unsigned long long p_start = offset;
        unsigned long long p_end = offset + numItems;

        // visualizer:
        // [     PAGE N    ] [ PAGE N + 1] [ PAGE N + 2] [ PAGE N + 3] ... [ PAGE N + K-2 ] [ PAGE N + K-1 ] [ PAGE N + K ]
        //        ^[                        REQUESTED MEMORY REGION                               ]^
        //         [HEAD   ] [                            BODY                            ] [TAIL ]

        // HEAD
        unsigned long long hs = offset; // head start
        unsigned long long rhs = (hs / pageCapacity) * pageCapacity; // rounded hs
        unsigned long long r_page_id = rhs / pageCapacity;
        unsigned long long he = min(rhs + pageCapacity, offset + numItems); // head end
        unsigned long long n_item = he - hs; // copy [hs, he)
        ACQUIRE(memLock);
        int cache_page_idx = loadPageToCache(r_page_id);
        if (cache_page_idx == -1)
        {
#if !defined(NDEBUG)
            addDebugMessage(L"Invalid cache page index, return zeroes array");
#endif
            setMem(dst, numItems * sizeof(T), 0);
            RELEASE(memLock);
            return 0;
        }
        copyMem(dst, cache[cache_page_idx] + (hs % pageCapacity), n_item * sizeof(T));
        dst += n_item;
        c_bytes += n_item * sizeof(T);
        RELEASE(memLock);
        // BODY
        unsigned long long bs = he; //bodystart
        if (he < p_end && (p_end - he) >= pageCapacity) // need to copy fullpage in the "middle"
        {
            for (bs = he; bs + pageCapacity <= p_end; bs += pageCapacity)
            {
                r_page_id = bs / pageCapacity;
                ACQUIRE(memLock);
                cache_page_idx = loadPageToCache(r_page_id);
                if (cache_page_idx == -1)
                {
#if !defined(NDEBUG)
                    addDebugMessage(L"Invalid cache page index, return zeroes array");
#endif
                    setMem(dst, numItems * sizeof(T), 0);
                    RELEASE(memLock);
                    return 0;
                }
                copyMem(dst, cache[cache_page_idx], pageSize);
                dst += pageCapacity;
                c_bytes += pageSize;
                RELEASE(memLock);
            }
        }
        // TAIL
        if (bs < p_end)
        {
            r_page_id = bs / pageCapacity;
            n_item = p_end - bs; // copy [bs, p_end)
            ACQUIRE(memLock);
            int cache_page_idx = loadPageToCache(r_page_id);
            if (cache_page_idx == -1)
            {
#if !defined(NDEBUG)
                addDebugMessage(L"Invalid cache page index, return zeroes array");
#endif
                setMem(dst, numItems * sizeof(T), 0);
                RELEASE(memLock);
                return 0;
            }
            copyMem(dst, cache[cache_page_idx], n_item * sizeof(T));
            dst += n_item;
            c_bytes += n_item * sizeof(T);
            RELEASE(memLock);
        }
        return c_bytes;
    }

    // appendMany: 
    // this operation append/copies multiple (T) to the memory pages
    // return number of items has been copied
    unsigned long long appendMany(T* src, unsigned long long numItems)
    {
        ACQUIRE(memLock);
        unsigned long long c_bytes = 0;
        unsigned long long p_start = currentId;
        unsigned long long p_end = currentId + numItems;

        // HEAD
        unsigned long long hs = currentId; // head start
        unsigned long long rhs = (hs / pageCapacity) * pageCapacity; // rounded hs
        unsigned long long r_page_id = rhs / pageCapacity;
        unsigned long long he = min(rhs + pageCapacity, p_end); // head end
        unsigned long long n_item = he - hs; // copy [hs, he)
        copyMem(currentPage + (currentId % pageCapacity), src, n_item * sizeof(T));
        currentId += n_item;
        src += n_item;
        c_bytes += n_item * sizeof(T);
        tryPersistingPage();
        // BODY
        unsigned long long bs = he; //bodystart
        if (currentId < p_end && (p_end - currentId) >= pageCapacity)
        {
            for (bs = he; bs + pageCapacity <= p_end; bs += pageCapacity)
            {
                copyMem(currentPage, src, pageSize);
                currentId += pageCapacity;
                src += pageCapacity;
                c_bytes += pageSize;
                tryPersistingPage();
            }
        }
        // TAIL
        if (bs < p_end)
        {
            n_item = p_end - bs; // copy [bs, p_end)
            copyMem(currentPage, src, n_item * sizeof(T));
            currentId += n_item;
            src += n_item;
            c_bytes += n_item * sizeof(T);
            tryPersistingPage();
        }
        RELEASE(memLock);
        return c_bytes;
    }

    // return array[index]
    // if index is not in current page it will try to find it in cache
    // if index is not in cache it will load the page to most outdated cache
    T get(unsigned long long index)
    {
        T result;
        ACQUIRE(memLock);
        if (index >= currentId) // out of bound
        {
            setMem(&result, sizeof(T), 0);
            RELEASE(memLock);
            return result;
        }
        unsigned long long requested_page_id = index / pageCapacity;
        int cache_page_idx = loadPageToCache(requested_page_id);
        if (cache_page_idx == -1)
        {
#if !defined(NDEBUG)
            addDebugMessage(L"Invalid cache page index, return zeroes array");
#endif
            setMem(&result, sizeof(T), 0);
            RELEASE(memLock);
            return result;
        }
        result = cache[cache_page_idx][index % pageCapacity];
        RELEASE(memLock);
        return result;
    }

    // return array[index]
    // if index is not in current page it will try to find it in cache
    // if index is not in cache it will load the page to most outdated cache
    void getOne(unsigned long long index, T* result)
    {
        ACQUIRE(memLock);
        if (index >= currentId) // out of bound
        {
            setMem(result, sizeof(T), 0);
            RELEASE(memLock);
            return;
        }
        unsigned long long requested_page_id = index / pageCapacity;
        int cache_page_idx = loadPageToCache(requested_page_id);
        if (cache_page_idx == -1)
        {
#if !defined(NDEBUG)
            addDebugMessage(L"Invalid cache page index, return zeroes array");
#endif
            setMem(result, sizeof(T), 0);
            RELEASE(memLock);
            return;
        }
        copyMem(result, &(cache[cache_page_idx][index % pageCapacity]), sizeof(T));
        RELEASE(memLock);
        return;
    }

    T operator[](unsigned long long index)
    {
        return get(index);
    }

    // append (single) data to latest
    // if current page is fully written it will:
    // (1) write current page to disk
    // (2) copy current page to cache
    // (3) clean current page for new data
    void append(const T& data)
    {
        ASSERT(currentPage != NULL);
        ACQUIRE(memLock);
        copyMem(&currentPage[currentId % pageCapacity], &data, sizeof(T));
        currentId++;
        tryPersistingPage();
        RELEASE(memLock);
    }

    unsigned long long size()
    {
        return currentId;
    }

    // delete a page on disk given pageId
    bool prune(unsigned long long pageId)
    {
        if (pageId > currentPageId)
        {
            return false;
        }
        CHAR16 pageName[64];
        generatePageName(pageName, pageId);
        ACQUIRE(memLock);
        bool success = (asyncRemoveFile(pageName, pageDir)) == 0;
        RELEASE(memLock);
        return success;
    }

    // delete pages data on disk given (fromId, toId)
    // Since we store the whole page on disk, pageId will be rounded up for fromId and rounded down for toId
    // fromPageId = (fromId + pageCapacity - 1) // pageCapacity
    // toPageId = (toId   - pageCapacity + 1) // pageCapacity
    // eg: pageCapacity is 50'000. To delete the second page, call prune(50000, 99999)
    bool pruneRange(long long fromId, long long toId)
    {
        long long fromPageId = (fromId + pageCapacity - 1) / pageCapacity;
        long long toPageId = (toId - pageCapacity + 1) / pageCapacity;
        if (fromPageId < 0 || toPageId < 0)
        {
            return false;
        }
        if (fromPageId > toPageId)
        {
            return false;
        }
        if (!isPageIdValid(fromPageId))
        {
            return false;
        }
        if (!isPageIdValid(toPageId))
        {
            return false;
        }
        bool success = true;
        for (long long i = fromPageId; i <= toPageId; i++)
        {
            bool ret = prune(i);
            success &= ret;
            if (!ret) return success;
        }
        return success;
    }

    // checking if an index is in valid range of this array
    bool isIndexValid(unsigned long long index)
    {
        return index < currentId;
    }

    // checking if a page id is in valid range of this array
    bool isPageIdValid(unsigned long long index)
    {
        return index <= currentPageId;
    }

    unsigned long long pageCap()
    {
        return pageCapacity;
    }

    unsigned long long getPageSize()
    {
        return pageCapacity * sizeof(T);
    }

    const T* getCurrentPagePtr()
    {
        return currentPage;
    }

    unsigned long long dumpVMState(unsigned char* buffer)
    {
        ACQUIRE(memLock);
        unsigned long long ret = 0;
        copyMem(buffer, currentPage, pageSize);
        ret += pageSize;
        buffer += pageSize;

        *((unsigned long long*)buffer) = currentId;
        buffer += 8;
        ret += 8;

        *((unsigned long long*)buffer) = currentPageId;
        buffer += 8;
        ret += 8;
        RELEASE(memLock);
        return ret;
    }

    unsigned long long loadVMState(unsigned char* buffer)
    {
        ACQUIRE(memLock);
        unsigned long long ret = 0;
        copyMem(currentPage, buffer, pageSize);
        ret += pageSize;
        buffer += pageSize;

        currentId = *((unsigned long long*)buffer);
        buffer += 8;
        ret += 8;

        currentPageId = *((unsigned long long*)buffer);
        buffer += 8;
        ret += 8;

        cachePageId[0] = currentPageId;
        lastAccessedTimestamp[0] = now_ms();
        RELEASE(memLock);
        return ret;
    }
};

enum SwapMode
{
    INDEX_MODE = 0, // for stride access pattern (ideally for TickData, Ticks)
    OFFSET_MODE = 1 // for random access pattern using offset (ideally for Transaction)
};

// SwapVirtualMemory don't use append operations, it acts like a continuous chunk of memory that can be read and written randomly
// it will try to persist pages that are not written to disk when loading a page to cache (when there is no empty cache slot)
// NOTE: pages in cache may not be written to disk yet
// NOTE: DO NOT CREATE INSTANCE IN FUNCTION STACK, IT WILL CAUSE STACK OVERFLOW
template <typename T, unsigned long long prefixName, unsigned long long pageDirectory, unsigned long long pageCapacity = 100000, unsigned long long numCachePage = 128, SwapMode mode = INDEX_MODE, long long extraBytesPerElement = 0>
class SwapVirtualMemory : private VirtualMemory<T, prefixName, pageDirectory, pageCapacity, numCachePage>
{
    using VMBase = VirtualMemory<T, prefixName, pageDirectory, pageCapacity, numCachePage>;
    using VMBase::currentPage;
    using VMBase::currentPageId;
    using VMBase::pageSize;
    using VMBase::pageDir;
    using VMBase::cache;
    using VMBase::cachePageId;
    using VMBase::lastAccessedTimestamp;
    using VMBase::memLock;
    using VMBase::generatePageName;
    using VMBase::findCachePage;
    using VMBase::loadPageToCache;
    using VMBase::getMostOutdatedCachePage;

    static constexpr unsigned long long MAX_PAGE = 1024 * 1024; // max 1 million pages, can be adjusted later

private:
    bool* isPageWrittenToDisk; // if current page is written to disk
    static constexpr unsigned long long INVALID_PAGE_ID = -1;
    static constexpr unsigned long long isPageWrittenToDiskSize = sizeof(bool) * MAX_PAGE;

    // used for offset mode
    T* pageExtraBytesBuffer;
    bool* pageHasExtraBytes; // if this page has extra bytes
    unsigned long long* lastestPageExtraBytesOffsetAccessed; // used to track the lastest offset accessed for each page
    static constexpr unsigned long long maxBytesPerElement = sizeof(T) + extraBytesPerElement;
    static constexpr unsigned long long maxBytesPerPage = maxBytesPerElement * pageCapacity;

    static constexpr unsigned long long pageExtraBytesBufferSize = (maxBytesPerElement) * MAX_PAGE;
    static constexpr unsigned long long pageHasExtraBytesBufferSize = sizeof(bool) * MAX_PAGE;
    static constexpr unsigned long long lastestPageExtraBytesOffsetAccessedBufferSize = sizeof(unsigned long long) * MAX_PAGE;

    void writePageToDisk(unsigned long long pageId)
    {
        CHAR16 pageName[64];
        generatePageName(pageName, pageId);

        int cache_idx = findCachePage(pageId);
        if (cache_idx == -1)
        {
            logToConsole(L"Error in writeCachePageToDisk: page not found on cache");
            return;
        }
        unsigned char *pageBuffer = (unsigned char*)cache[cache_idx];

#if defined(NO_UEFI) && !defined(REAL_NODE)
        auto sz = save(pageName, pageSize, (unsigned char*)pageBuffer, pageDir);
#else
        auto sz = save(pageName, pageSize, (unsigned char*)pageBuffer, pageDir);
#endif

#if !defined(NDEBUG)
        if (sz != pageSize)
        {
            addDebugMessage(L"Failed to store virtualMemory to disk. Old data maybe lost");
        }
        else
        {
            CHAR16 debugMsg[128];
            unsigned long long tmp = prefixName;
            debugMsg[0] = L'[';
            copyMem(debugMsg + 1, &tmp, 8);
            debugMsg[5] = L']';
            debugMsg[6] = L' ';
            debugMsg[7] = 0;
            appendText(debugMsg, L"page ");
            appendNumber(debugMsg, currentPageId, true);
            appendText(debugMsg, L" is written into disk");
            addDebugMessage(debugMsg);
        }
#endif

        isPageWrittenToDisk[pageId] = true;
    }

    int loadPageToCacheAndTryToPersist(unsigned long long pageId)
    {
        int cache_page_id = findCachePage(pageId);

        if (cache_page_id != -1)
        {
            return cache_page_id;
        }
        CHAR16 pageName[64];
        generatePageName(pageName, pageId);
        cache_page_id = getMostOutdatedCachePageExceptCurrentPage();
        if (cachePageId[cache_page_id] != INVALID_PAGE_ID)
        {
            writePageToDisk(cachePageId[cache_page_id]);
        }
#if false
        auto sz = load(pageName, pageSize, (unsigned char*)cache[cache_page_id], pageDir);
        lastAccessedTimestamp[cache_page_id] = now_ms();
#else
#if !defined(NDEBUG)
        {
            CHAR16 debugMsg[128];
            setText(debugMsg, L"Trying to load OLD page: ");
            appendNumber(debugMsg, pageId, true);
            addDebugMessage(debugMsg);
        }
#endif
        unsigned long long sz = 0;
        if (isPageWrittenToDisk[pageId])
        {
            sz = load(pageName, pageSize, (unsigned char*)cache[cache_page_id], pageDir);
        } else
        {
            sz = pageSize;
            setMem(cache[cache_page_id], pageSize, 0);
        }
        if (sz != pageSize)
        {
#if !defined(NDEBUG)
            addDebugMessage(L"Failed to load virtualMemory from disk");
#endif
            return -1;
        }
        lastAccessedTimestamp[cache_page_id] = now_ms();
#endif
        cachePageId[cache_page_id] = pageId;
#if !defined(NDEBUG)
        {
            CHAR16 debugMsg[128];
            unsigned long long tmp = prefixName;
            debugMsg[0] = L'[';
            copyMem(debugMsg + 1, &tmp, 8);
            debugMsg[5] = L']';
            debugMsg[6] = L' ';
            debugMsg[7] = 0;
            appendText(debugMsg, L"Load complete. Page ");
            appendNumber(debugMsg, pageId, true);
            appendText(debugMsg, L" is loaded into slot ");
            appendNumber(debugMsg, cache_page_id, true);
            addDebugMessage(debugMsg);
        }
#endif
        return cache_page_id;
    }

    // return the most outdated cache page
    int getMostOutdatedCachePageExceptCurrentPage()
    {
        int min_index = 0;
        for (int i = 0; i <= numCachePage; i++)
        {
            if (lastAccessedTimestamp[i] == 0 && cachePageId[i] != currentPageId)
            {
                return i;
            }
            if ((lastAccessedTimestamp[i] < lastAccessedTimestamp[min_index]) || (lastAccessedTimestamp[i] == lastAccessedTimestamp[min_index] && cachePageId[i] < cachePageId[min_index]))
            {
                if (cachePageId[i] != currentPageId) // skip current page
                {
                    min_index = i;
                }
            }
        }
        return min_index;
    }
public:
    SwapVirtualMemory()
    {
        VMBase();
    }

    void reset() {
        VMBase::reset();
        setMem(isPageWrittenToDisk, isPageWrittenToDiskSize, 0);
        if (mode == SwapMode::OFFSET_MODE) {
            setMem(pageExtraBytesBuffer, pageExtraBytesBufferSize, 0);
            setMem(pageHasExtraBytes, pageHasExtraBytesBufferSize, 0);
            setMem(lastestPageExtraBytesOffsetAccessed, lastestPageExtraBytesOffsetAccessedBufferSize, 0xff);
        }
    }

    bool init()
    requires (mode == SwapMode::OFFSET_MODE)
    {
        ASSERT(extraBytesPerElement >= 0);
        pageSize = maxBytesPerPage;
        if (!allocPoolWithErrorLog(L"SwapVM.IsPageWrittenToDisk", isPageWrittenToDiskSize, (void**)&isPageWrittenToDisk, __LINE__))
        {
            return false;
        }

        if (
            !allocPoolWithErrorLog(L"SwapVM.ExtraBytes", pageExtraBytesBufferSize, (void**)&pageExtraBytesBuffer, __LINE__) ||
            !allocPoolWithErrorLog(L"SwapVM.PageHasExtraBytes", pageHasExtraBytesBufferSize, (void**)&pageHasExtraBytes, __LINE__) ||
            !allocPoolWithErrorLog(L"SwapVM.LastestPageExtraBytesOffsetAccessed", lastestPageExtraBytesOffsetAccessedBufferSize, (void**)&lastestPageExtraBytesOffsetAccessed, __LINE__))
        {
            return false;
        }
        bool ok = VMBase::init();
        return ok;
    }

    bool init()
    requires (mode == SwapMode::INDEX_MODE)
    {
        if (!allocPoolWithErrorLog(L"SwapVM.IsPageWrittenToDisk", isPageWrittenToDiskSize, (void**)&isPageWrittenToDisk, __LINE__))
        {
            return false;
        }

        bool ok = VMBase::init();
        return ok;
    }

    T& getRef(unsigned long long index)
    requires (mode == SwapMode::INDEX_MODE)
    {
        static T empty;
        ACQUIRE(memLock);

        unsigned long long requested_page_id = index / pageCapacity;
        currentPageId = requested_page_id > currentPageId ? requested_page_id : currentPageId;
        int cache_page_idx = loadPageToCacheAndTryToPersist(requested_page_id);
        if (cache_page_idx == -1)
        {
            setText(message, L"Fatal Error: Invalid cache page index | Line ");
            appendNumber(message, __LINE__, true);
            logToConsole(message);
            // Exit program
            exit(1);
        }
        T& resultRef = cache[cache_page_idx][index % pageCapacity];
        RELEASE(memLock);
        return resultRef;
    }

    // NOTE: if getPtr is used, all other operations need to be SEQUENCE even they are reading operations (get, getMany)
    // because reading operations may flush the page T* live into disk and change cache page state
    T* getPtr(unsigned long long index)
    requires (mode == SwapMode::INDEX_MODE)
    {
        static T* result = nullptr;
        ACQUIRE(memLock);
		result = nullptr;
        unsigned long long requested_page_id = index / pageCapacity;
        currentPageId = requested_page_id > currentPageId ? requested_page_id : currentPageId;
        int cache_page_idx = loadPageToCacheAndTryToPersist(requested_page_id);
        if (cache_page_idx == -1)
        {
            setText(message, L"Fatal Error: Invalid cache page index | Line ");
            appendNumber(message, __LINE__, true);
            logToConsole(message);

            setText(message, L"Requested Page ID: ");
            appendNumber(message, requested_page_id, true);
            appendText(message, L"| Max Page ID: ");
            appendNumber(message, currentPageId, true);
            logToConsole(message);

            setText(message, L"SwapVm Prefix: ");
            appendText(message, pageDir);
            logToConsole(message);
            // Exit program
            exit(1);
        }
        result = &cache[cache_page_idx][index % pageCapacity];
        RELEASE(memLock);
        return result;
    }

    T* operator[](unsigned long long offset)
    requires (mode == SwapMode::OFFSET_MODE)
    {
        static T* result = nullptr;
        ACQUIRE(memLock);
		result = nullptr;
        unsigned long long pageId = offset / maxBytesPerPage;
        unsigned long long offsetInPage = offset % maxBytesPerPage;
        long long lastElementLength = 0;

        int cache_page_idx = loadPageToCacheAndTryToPersist(pageId);
        if (cache_page_idx == -1)
        {
            setText(message, L"Fatal Error: Invalid cache page index | Line ");
            appendNumber(message, __LINE__, true);
            logToConsole(message);
            // Exit program
            exit(1);
        }

        // To avoid cross page access, if the remaining bytes in this page is less than maxBytesPerElement,
        // we will tmp use extra buffer to store the element
        if (maxBytesPerPage - offsetInPage < maxBytesPerElement) {
            T* thisPageExtraBuffer = (T*)((unsigned char *)pageExtraBytesBuffer + pageId * maxBytesPerElement);
            if (pageHasExtraBytes[pageId]) {
                lastElementLength = offset - lastestPageExtraBytesOffsetAccessed[pageId];
                // if lastElementLength < 0, means the element already in cache, just return the element in cache
                if (lastElementLength < 0) {
                    goto normal_access;
                } else if (lastElementLength == 0) { // if lastElementLength == 0, means the element is already in extra buffer, just return the extra buffer
                    RELEASE(memLock);
                    return thisPageExtraBuffer;
                }
                // if lastElementLength > 0 (new element need to be inserted), copy the last element in extra buffer to cache
                copyMem((unsigned char *)cache[cache_page_idx] + (lastestPageExtraBytesOffsetAccessed[pageId] % maxBytesPerPage), thisPageExtraBuffer, lastElementLength);
                lastestPageExtraBytesOffsetAccessed[pageId] = offset;
                // reset the extra buffer
                setMem(thisPageExtraBuffer, maxBytesPerElement, 0);
                RELEASE(memLock);
                return thisPageExtraBuffer;
            } else {
                lastestPageExtraBytesOffsetAccessed[pageId] = offset;
                pageHasExtraBytes[pageId] = true;
                RELEASE(memLock);
                return thisPageExtraBuffer;
            }
        }

        normal_access:
        unsigned char *pageBuffer = (unsigned char*)cache[cache_page_idx];
        result = (T*)(pageBuffer + offsetInPage);
        RELEASE(memLock);
        return result;
    }

    T* operator[](unsigned long long index)
    requires (mode == SwapMode::INDEX_MODE)
    {
        return getPtr(index);
    }

    T& operator()(unsigned long long index)
    requires (mode == SwapMode::INDEX_MODE)
    {
        return getRef(index);
    }

    unsigned long long getVmStateSize()
    {
        unsigned long long totalOffsetModeExtraSize = 0;
        if (mode == SwapMode::OFFSET_MODE)
        {
            totalOffsetModeExtraSize = pageExtraBytesBufferSize + pageHasExtraBytesBufferSize + lastestPageExtraBytesOffsetAccessedBufferSize;
        }
        return pageSize * (numCachePage + 1) + isPageWrittenToDiskSize  + sizeof(cachePageId) + sizeof(lastAccessedTimestamp) + 8 + totalOffsetModeExtraSize;
    }

    unsigned long long dumpVMState(unsigned char* buffer)
    {
        ACQUIRE(memLock);
        unsigned long long ret = 0;
        for (int i = 0; i <= numCachePage; i++)
        {
            copyMem(buffer, cache[i], pageSize);
            buffer += pageSize;
        }
        ret += (numCachePage+1) * pageSize;

        copyMem(buffer, isPageWrittenToDisk, isPageWrittenToDiskSize);
        ret += isPageWrittenToDiskSize;
        buffer += isPageWrittenToDiskSize;

        if (mode == SwapMode::OFFSET_MODE)
        {
            copyMem(buffer, pageExtraBytesBuffer, pageExtraBytesBufferSize);
            buffer += pageExtraBytesBufferSize;
            ret += pageExtraBytesBufferSize;

            copyMem(buffer, pageHasExtraBytes, pageHasExtraBytesBufferSize);
            buffer += pageHasExtraBytesBufferSize;
            ret += pageHasExtraBytesBufferSize;

            copyMem(buffer, lastestPageExtraBytesOffsetAccessed, lastestPageExtraBytesOffsetAccessedBufferSize);
            buffer += lastestPageExtraBytesOffsetAccessedBufferSize;
            ret += lastestPageExtraBytesOffsetAccessedBufferSize;
        }

        copyMem(buffer, lastAccessedTimestamp, sizeof(lastAccessedTimestamp));
        buffer += sizeof(lastAccessedTimestamp);
        ret += sizeof(lastAccessedTimestamp);

        copyMem(buffer, cachePageId, sizeof(cachePageId));
        ret += sizeof(cachePageId);
        buffer += sizeof(cachePageId);

        *((unsigned long long*)buffer) = currentPageId;
        buffer += 8;
        ret += 8;
        RELEASE(memLock);
        return ret;
    }

    unsigned long long loadVMState(unsigned char* buffer)
    {
        ACQUIRE(memLock);
        unsigned long long ret = 0;
        for (int i = 0; i <= numCachePage; i++)
        {
            copyMem(cache[i], buffer, pageSize);
            buffer += pageSize;
        }
        ret += (numCachePage+1) * pageSize;

        copyMem(isPageWrittenToDisk, buffer, isPageWrittenToDiskSize);
        buffer += isPageWrittenToDiskSize;
        ret += isPageWrittenToDiskSize;

        if (mode == SwapMode::OFFSET_MODE)
        {
            copyMem(pageExtraBytesBuffer, buffer, pageExtraBytesBufferSize);
            buffer += pageExtraBytesBufferSize;
            ret += pageExtraBytesBufferSize;

            copyMem(pageHasExtraBytes, buffer, pageHasExtraBytesBufferSize);
            buffer += pageHasExtraBytesBufferSize;
            ret += pageHasExtraBytesBufferSize;

            copyMem(lastestPageExtraBytesOffsetAccessed, buffer, lastestPageExtraBytesOffsetAccessedBufferSize);
            buffer += lastestPageExtraBytesOffsetAccessedBufferSize;
            ret += lastestPageExtraBytesOffsetAccessedBufferSize;
        }

        copyMem(lastAccessedTimestamp, buffer, sizeof(lastAccessedTimestamp));
        buffer += sizeof(lastAccessedTimestamp);
        ret += sizeof(lastAccessedTimestamp);

        copyMem(cachePageId, buffer, sizeof(cachePageId));
        buffer += sizeof(cachePageId);
        ret += sizeof(cachePageId);

        currentPageId = *((unsigned long long*)buffer);
        buffer += 8;
        ret += 8;

        RELEASE(memLock);
        return ret;
    }

    T* getPageBuffer(unsigned long long pageId) {
        auto cacheIndex = findCachePage(pageId);
        if (cacheIndex == -1) {
            return nullptr;
        }
        return cache[cacheIndex];
    }

    T* getCacheBuffer(unsigned long long cacheId) {
        if (cacheId > numCachePage) {
            return nullptr;
        }
        return cache[cacheId];
    }

    T* getExtraBuffer(unsigned long long pageId) {
        return &pageExtraBytesBuffer[pageId];
    }

    unsigned long long getPageSize()
    {
        return pageSize;
    }

    const T* getCurrentPagePtr()
    {
        return currentPage;
    }
};