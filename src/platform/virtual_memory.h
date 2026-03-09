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
    const unsigned long long pageSize = sizeof(T) * pageCapacity;
private:
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
#ifdef NO_UEFI
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
#ifdef NO_UEFI
#else
                lastAccessedTimestamp[i] = now_ms();
#endif
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
#ifdef NO_UEFI
        auto sz = load(pageName, pageSize, (unsigned char*)cache[cache_page_id], pageDir);
        lastAccessedTimestamp[cache_page_id] = 0;
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
                cache[i] = cache[i - 1] + pageCapacity;
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
#ifdef NO_UEFI
            addEpochToFileName(pageDir, 12, 0);
#else
            addEpochToFileName(pageDir, 12, max(EPOCH, int(system.epoch)));
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