#pragma once
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/time.h"
#include "platform/memory_util.h"
#include "platform/debugging.h"

#include "network_messages/header.h"
#include "network_messages/logging.h"

#include "private_settings.h"
#include "public_settings.h"
#include "system.h"
#include "kangaroo_twelve.h"

#include "platform/virtual_memory.h"
struct Peer;

#define LOG_CONTRACTS (LOG_CONTRACT_ERROR_MESSAGES | LOG_CONTRACT_WARNING_MESSAGES | LOG_CONTRACT_INFO_MESSAGES | LOG_CONTRACT_DEBUG_MESSAGES)

#if LOG_SPECTRUM | LOG_UNIVERSE | LOG_CONTRACTS | LOG_CUSTOM_MESSAGES
#define ENABLED_LOGGING 1
#else
#define ENABLED_LOGGING 0
#endif


#if LOG_SPECTRUM && LOG_UNIVERSE
#define LOG_STATE_DIGEST 1
#else
#define LOG_STATE_DIGEST 0
#endif

#ifdef NO_UEFI
#undef LOG_STATE_DIGEST
#define LOG_STATE_DIGEST 0
#else
// if we include xkcp "outside" it will break the gtest
#include "K12/kangaroo_twelve_xkcp.h"
#include "common_buffers.h"
#endif

// Logger defines
#define LOG_HEADER_SIZE 26 // 2 bytes epoch + 4 bytes tick + 4 bytes log size/types + 8 bytes log id + 8 bytes log digest

#define QU_TRANSFER 0
#define ASSET_ISSUANCE 1
#define ASSET_OWNERSHIP_CHANGE 2
#define ASSET_POSSESSION_CHANGE 3
#define CONTRACT_ERROR_MESSAGE 4
#define CONTRACT_WARNING_MESSAGE 5
#define CONTRACT_INFORMATION_MESSAGE 6
#define CONTRACT_DEBUG_MESSAGE 7
#define BURNING 8
#define DUST_BURNING 9
#define SPECTRUM_STATS 10
#define ASSET_OWNERSHIP_MANAGING_CONTRACT_CHANGE 11
#define ASSET_POSSESSION_MANAGING_CONTRACT_CHANGE 12
#define CUSTOM_MESSAGE 255

/*
* STRUCTS FOR LOGGING
*/
struct QuTransfer
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    long long amount;
    char _terminator; // Only data before "_terminator" are logged
};

struct AssetIssuance
{
    m256i issuerPublicKey;
    long long numberOfShares;
    char name[7];
    char numberOfDecimalPlaces;
    char unitOfMeasurement[7];

    char _terminator; // Only data before "_terminator" are logged
};

struct AssetOwnershipChange
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    m256i issuerPublicKey;
    long long numberOfShares;
    char name[7];
    char numberOfDecimalPlaces;
    char unitOfMeasurement[7];

    char _terminator; // Only data before "_terminator" are logged
};

struct AssetPossessionChange
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    m256i issuerPublicKey;
    long long numberOfShares;
    char name[7];
    char numberOfDecimalPlaces;
    char unitOfMeasurement[7];

    char _terminator; // Only data before "_terminator" are logged
};

struct AssetOwnershipManagingContractChange
{
    m256i ownershipPublicKey;
    m256i issuerPublicKey;
    unsigned int sourceContractIndex;
    unsigned int destinationContractIndex;
    long long numberOfShares;
    char assetName[7];

    char _terminator; // Only data before "_terminator" are logged
};

struct AssetPossessionManagingContractChange
{
    m256i possessionPublicKey;
    m256i ownershipPublicKey;
    m256i issuerPublicKey;
    unsigned int sourceContractIndex;
    unsigned int destinationContractIndex;
    long long numberOfShares;
    char assetName[7];

    char _terminator; // Only data before "_terminator" are logged
};

struct DummyContractErrorMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

struct DummyContractWarningMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

struct DummyContractInfoMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

struct DummyContractDebugMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

struct DummyCustomMessage
{
    unsigned long long _type; // Assign a random unique number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

struct Burning
{
    m256i sourcePublicKey;
    long long amount;

    char _terminator; // Only data before "_terminator" are logged
};

// Contains N entites and amounts of dust burns in the memory layout: [N with 2 bytes] | [public key 0] | [amount 0] | [public key 1] | [amount 1] | ... | [public key N-1] | [amount N-1].
// CAUTION: This is a variable-size log type and the full log message content goes boyond the size of this struct!
struct DustBurning
{
    unsigned short numberOfBurns;

    struct Entity
    {
        m256i publicKey;
        unsigned long long amount;
    };
    static_assert(sizeof(Entity) == (sizeof(m256i) + sizeof(unsigned long long)), "Unexpected size");

    unsigned int messageSize() const
    {
        return 2 + numberOfBurns * sizeof(Entity);
    }

    Entity& entity(unsigned short i)
    {
        ASSERT(i < numberOfBurns);
        char* buf = reinterpret_cast<char*>(this);
        return *reinterpret_cast<Entity*>(buf + i * (sizeof(Entity)) + 2);
    }
};

struct SpectrumStats
{
    unsigned long long totalAmount;
    unsigned long long dustThresholdBurnAll;
    unsigned long long dustThresholdBurnHalf;
    unsigned int numberOfEntities;
    unsigned int entityCategoryPopulations[48];
};


/*
 * LOGGING IMPLEMENTATION
 */
#define LOG_BUFFER_PAGE_SIZE 300000000ULL
#define PMAP_LOG_PAGE_SIZE 30000000ULL
#define IMAP_LOG_PAGE_SIZE 10000ULL
#define VM_NUM_CACHE_PAGE 8
 // Virtual memory with 100'000'000 items per page and 4 pages on cache
#ifdef NO_UEFI
#define TEXT_LOGS_AS_NUMBER 0
#define TEXT_PMAP_AS_NUMBER 0
#define TEXT_BUF_AS_NUMBER 0
#define TEXT_IMAP_AS_NUMBER 0 
#else
#define TEXT_LOGS_AS_NUMBER 32370064710631532ULL // L"logs"
#define TEXT_PMAP_AS_NUMBER 31525614010564720ULL // L"pmap"
#define TEXT_IMAP_AS_NUMBER 31525614010564713ULL // L"imap"
#define TEXT_BUF_AS_NUMBER 28710885718818914ULL  // L"buff"
#endif

class qLogger
{
public:
    struct BlobInfo
    {
        long long startIndex;
        long long length;
    };
    struct TickBlobInfo
    {
        long long fromLogId[LOG_TX_PER_TICK];
        long long length[LOG_TX_PER_TICK];
    };

private:
    inline static VirtualMemory<char, TEXT_BUF_AS_NUMBER, TEXT_LOGS_AS_NUMBER, LOG_BUFFER_PAGE_SIZE, VM_NUM_CACHE_PAGE> logBuffer;
    inline static VirtualMemory<BlobInfo, TEXT_PMAP_AS_NUMBER, TEXT_LOGS_AS_NUMBER, PMAP_LOG_PAGE_SIZE, VM_NUM_CACHE_PAGE> mapLogIdToBufferIndex;
    inline static VirtualMemory<TickBlobInfo, TEXT_IMAP_AS_NUMBER, TEXT_LOGS_AS_NUMBER, IMAP_LOG_PAGE_SIZE, VM_NUM_CACHE_PAGE> mapTxToLogId;
    inline static TickBlobInfo currentTickTxToId;
    inline static char responseBuffers[MAX_NUMBER_OF_PROCESSORS][RequestResponseHeader::max_size];

#if LOG_STATE_DIGEST
    // Digests of log data:
    // d(i) = K12(concat(d(i-1), log(spectrum), log(universe))
    // custom log from smart contracts are not included in the digest computation
    inline static m256i digests[MAX_NUMBER_OF_TICKS_PER_EPOCH];
    inline static XKCP::KangarooTwelve_Instance k12;
#endif

    inline static unsigned long long logBufferTail;
    inline static unsigned long long logId;
    inline static unsigned int tickBegin; // initial tick of the epoch
    inline static unsigned int lastUpdatedTick; // tick number that the system has generated all log
    inline static unsigned int currentTxId;
    inline static unsigned int currentTick;

    static unsigned long long getLogId(const char* ptr)
    {
        // first 10 bytes are: epoch(2) + tick(4)+ size/type(4)
        // next 8 bytes are logId
        return *((unsigned long long*)(ptr + 10));
    }

    static unsigned long long getLogDigest(const char* ptr)
    {
        // first 18 bytes are: epoch(2) + tick(4)+ size/type(4) + logid(8)
        // next 8 bytes are logdigest
        return *((unsigned long long*)(ptr + 18));
    }

    static unsigned int getLogSize(const char* ptr)
    {
        // first 6 bytes are: epoch(2) + tick(4)
        // next 4 bytes are size&type
        unsigned int sizeAndType = *((unsigned int*)(ptr + 6));

        return sizeAndType & 0xFFFFFF; // last 24 bits are message size
    }

    // verify the log event with digests
    static bool verifyLog(const char* ptr, unsigned long long logId)
    {
#if ENABLED_LOGGING
        if (getLogId(ptr) != logId)
        {
            return false;
        }
        unsigned int msgSize = getLogSize(ptr);
        if (msgSize >= RequestResponseHeader::max_size)
        {
            // invalid size
            return false;
        }
#endif
        return true;
    }

    static void logMessage(unsigned int messageSize, unsigned char messageType, const void* message)
    {
#if ENABLED_LOGGING
        char buffer[LOG_HEADER_SIZE];
        tx.addLogId();
        logBuf.set(logId, logBufferTail, LOG_HEADER_SIZE + messageSize);
        *((unsigned short*)(buffer)) = system.epoch;
        *((unsigned int*)(buffer + 2)) = system.tick;
        *((unsigned int*)(buffer + 6)) = messageSize | (messageType << 24);
        *((unsigned long long*)(buffer + 10)) = logId++;
        unsigned long long logDigest = 0;
        KangarooTwelve(message, messageSize, &logDigest, 8);
        *((unsigned long long*)(buffer + 18)) = logDigest;
        logBufferTail += LOG_HEADER_SIZE + messageSize;
        logBuffer.appendMany(buffer, LOG_HEADER_SIZE);
        logBuffer.appendMany((char*)message, messageSize);
#if LOG_STATE_DIGEST
        if (messageType == QU_TRANSFER || messageType == ASSET_ISSUANCE || messageType == ASSET_OWNERSHIP_CHANGE || messageType == ASSET_POSSESSION_CHANGE ||
            messageType == BURNING || messageType == DUST_BURNING || messageType == SPECTRUM_STATS || messageType == ASSET_OWNERSHIP_MANAGING_CONTRACT_CHANGE ||
            messageType == ASSET_POSSESSION_MANAGING_CONTRACT_CHANGE)
        {
            auto ret = XKCP::KangarooTwelve_Update(&k12, reinterpret_cast<const unsigned char*>(message), messageSize);
#ifndef NDEBUG
            if (ret != 0)
            {
                addDebugMessage(L"Failed to update log digests k12");
            }
#endif
        }
#endif
#endif
    }
public:
    // 5 special txs for 5 special events in qubic
    static constexpr unsigned int SC_INITIALIZE_TX = NUMBER_OF_TRANSACTIONS_PER_TICK + 0;
    static constexpr unsigned int SC_BEGIN_EPOCH_TX = NUMBER_OF_TRANSACTIONS_PER_TICK + 1;
    static constexpr unsigned int SC_BEGIN_TICK_TX = NUMBER_OF_TRANSACTIONS_PER_TICK + 2;
    static constexpr unsigned int SC_END_TICK_TX = NUMBER_OF_TRANSACTIONS_PER_TICK + 3;
    static constexpr unsigned int SC_END_EPOCH_TX = NUMBER_OF_TRANSACTIONS_PER_TICK + 4;

#if ENABLED_LOGGING
    // Struct to map log buffer from log id    
    static struct mapLogIdToBuffer
    {
        static bool init()
        {
            return mapLogIdToBufferIndex.init() && logBuffer.init();
        }

        static void deinit()
        {
            mapLogIdToBufferIndex.deinit();
            logBuffer.deinit();
        }

        static long long getIndex(unsigned long long logId)
        {
            char buf[LOG_HEADER_SIZE];
            setMem(buf, sizeof(buf), 0);
            BlobInfo res = mapLogIdToBufferIndex[logId];
            logBuffer.getMany(buf, res.startIndex, LOG_HEADER_SIZE);
            if (verifyLog(buf, logId))
            {
                return res.startIndex;
            }
            return -1;
        }

        static BlobInfo getBlobInfo(unsigned long long logId)
        {
            char buf[LOG_HEADER_SIZE];
            setMem(buf, sizeof(buf), 0);
            BlobInfo res = mapLogIdToBufferIndex[logId];
            logBuffer.getMany(buf, res.startIndex, LOG_HEADER_SIZE);
            if (verifyLog(buf, logId))
            {
                return res;
            }
            return BlobInfo{ -1,-1 };
        }

        static void set(unsigned long long logId, long long index, long long length)
        {
            ASSERT(logId == mapLogIdToBufferIndex.size());
            BlobInfo res;
            res.startIndex = index;
            res.length = length;
            mapLogIdToBufferIndex.append(res);
        }

        static void get(char* dst, unsigned long long logId)
        {
            BlobInfo bi = logBuf.getBlobInfo(logId);
            logBuffer.getMany((char*)dst, bi.startIndex, bi.length);
        }

        static void getMany(char* dst, unsigned long long offset, unsigned long long numItems)
        {
            logBuffer.getMany((char*)dst, offset, numItems);
        }
    } logBuf;
    // Struct to map log id ranges from tx hash
    static struct mapTxToLogIdAccess
    {
        static bool init()
        {
            return mapTxToLogId.init();
        }

        static void deinit()
        {
            mapTxToLogId.deinit();
        }

        // return the logID ranges of a tx hash
        static BlobInfo getLogIdInfo(unsigned long long processorNumber, unsigned int tick, unsigned int txId)
        {
            unsigned long long tickOffset = tick - tickBegin;
            if (tickOffset < MAX_NUMBER_OF_TICKS_PER_EPOCH && txId < LOG_TX_PER_TICK && tick <= lastUpdatedTick)
            {
                TickBlobInfo* res = (TickBlobInfo * )responseBuffers[processorNumber];
                {
                    for (int i = 0; i < LOG_TX_PER_TICK; i++)
                    {
                        res->fromLogId[i] = -1;
                        res->length[i] = -1;
                    }
                }
                unsigned long long sz = mapTxToLogId.getMany(res, tickOffset, 1); // bypass MSVC compiler (auto optimize to memcpy and memset)
                ASSERT(sz != 0);
                BlobInfo result = { res->fromLogId[txId], res->length[txId] };
                return result;
            }
            return BlobInfo{ -1,-1 };
        }

        static void getTickLogIdInfo(TickBlobInfo* output, const unsigned int tick)
        {
            unsigned long long tickOffset = tick - tickBegin;
            for (int i = 0; i < LOG_TX_PER_TICK; i++)
            {
                output->fromLogId[i] = -1;
                output->length[i] = -1;
            }
            if (tickOffset < MAX_NUMBER_OF_TICKS_PER_EPOCH && tick <= lastUpdatedTick)
            {
                mapTxToLogId.getOne(tickOffset, output);
            }
            return;
        }

        static void _registerNewTx(const unsigned int tick, const unsigned int txId)
        {
            if (currentTick != tick || currentTxId != txId)
            {
                currentTick = tick;
                currentTxId = txId;
            }
        }

        static void addLogId()
        {
            unsigned long long offsetTick = currentTick - tickBegin;
            ASSERT(offsetTick < MAX_NUMBER_OF_TICKS_PER_EPOCH);
            ASSERT(currentTxId < LOG_TX_PER_TICK);
            if (offsetTick < MAX_NUMBER_OF_TICKS_PER_EPOCH && currentTxId < LOG_TX_PER_TICK)
            {
                auto& startIndex = currentTickTxToId.fromLogId[currentTxId];
                auto& length = currentTickTxToId.length[currentTxId];
                if (startIndex == -1)
                {
                    startIndex = logId;
                    length = 1;
                }
                else
                {
                    ASSERT(startIndex != -1);
                    length++;
                }
            }
        }

        static void cleanCurrentTickTxToId()
        {
            for (int i = 0; i < LOG_TX_PER_TICK; i++)
            {
                currentTickTxToId.fromLogId[i] = -1;
                currentTickTxToId.length[i] = -1;
            }
        }

        static void _commit()
        {
            mapTxToLogId.append(currentTickTxToId);
        }

        static void commitAndCleanCurrentTxToLogId()
        {
            _commit();
            cleanCurrentTickTxToId();
        }
    } tx;
#endif

    static void registerNewTx(const unsigned int tick, const unsigned int txId)
    {
#if ENABLED_LOGGING
        tx._registerNewTx(tick, txId);
#endif
    }

    static bool initLogging()
    {
#if ENABLED_LOGGING
        if (!logBuf.init())
        {
            return false;
        }

        if (!tx.init())
        {
            return false;
        }

        reset(0);
#endif
        return true;
    }

    static void deinitLogging()
    {
#if ENABLED_LOGGING
        logBuf.deinit();
        tx.deinit();
#endif
    }

    static void reset(unsigned int _tickBegin)
    {
#if ENABLED_LOGGING
        logBuf.init();
        tx.init();
        logBufferTail = 0;
        logId = 0;
        lastUpdatedTick = 0;
        tickBegin = _tickBegin;
        tx.cleanCurrentTickTxToId();
#if LOG_STATE_DIGEST
        XKCP::KangarooTwelve_Initialize(&k12, 128, 32);
        m256i zeroHash = m256i::zero();
        XKCP::KangarooTwelve_Update(&k12, zeroHash.m256i_u8, 32); // init tick, feed zero hash
#endif
#endif
    }

    // updateTick is called right after _tick is processed
    static void updateTick(unsigned int _tick)
    {
#if ENABLED_LOGGING
        ASSERT((_tick == lastUpdatedTick + 1) || (_tick == tickBegin));
        ASSERT(_tick >= tickBegin);
#if LOG_STATE_DIGEST
        unsigned long long index = _tick - tickBegin;
        XKCP::KangarooTwelve_Final(&k12, digests[index].m256i_u8, (const unsigned char*)"", 0);
        XKCP::KangarooTwelve_Initialize(&k12, 128, 32); // init new k12
        XKCP::KangarooTwelve_Update(&k12, digests[index].m256i_u8, 32); // feed the prev hash back to this
#endif
        tx.commitAndCleanCurrentTxToLogId();
        ASSERT(mapTxToLogId.size() == (_tick - tickBegin + 1));
        lastUpdatedTick = _tick;
#endif
    }
    
#ifdef NO_UEFI
#else

    // This function is part of save/load feature and can only be called from main thread
    bool saveCurrentLoggingStates(CHAR16* dir)
    {
#if ENABLED_LOGGING
        unsigned char* buffer = (unsigned char*)__scratchpad();        
        static_assert(reorgBufferSize >= LOG_BUFFER_PAGE_SIZE + PMAP_LOG_PAGE_SIZE * sizeof(BlobInfo) + IMAP_LOG_PAGE_SIZE * sizeof(TickBlobInfo)
            + sizeof(digests) + 600, "scratchpad is too small");
        unsigned long long writeSz = 0;
        // copy currentPage of log buffer ~ 100MiB
        unsigned long long sz = logBuffer.dumpVMState(buffer);
        buffer += sz;
        writeSz += sz;

        // copy current page of mapLogIdToBufferIndex ~ 80MiB
        sz = mapLogIdToBufferIndex.dumpVMState(buffer);
        buffer += sz;
        writeSz += sz;

        // mapTxToLogId ~ 150MiB
        sz = mapTxToLogId.dumpVMState(buffer);
        buffer += sz;
        writeSz += sz;

        // copy digests ~ 13MiB
        copyMem(buffer, digests, sizeof(digests));
        buffer += sizeof(digests);
        writeSz += sizeof(digests);

        // copy k12 instance
        copyMem(buffer, &k12, sizeof(k12));
        buffer += sizeof(k12);
        writeSz += sizeof(k12);

        // copy variables
        *((unsigned long long*)buffer) = logBufferTail; buffer += 8;
        *((unsigned long long*)buffer) = logId; buffer += 8;
        *((unsigned int*)buffer) = tickBegin; buffer += 4;
        *((unsigned int*)buffer) = lastUpdatedTick; buffer += 4;
        *((unsigned int*)buffer) = currentTxId; buffer += 4;
        *((unsigned int*)buffer) = currentTick; buffer += 4;
        writeSz += 8 + 8 + 4 + 4 + 4 + 4;
        buffer = (unsigned char*)__scratchpad(); // reset back to original pos
        sz = save(L"logEventState.db", writeSz, buffer, dir);
        if (sz != writeSz)
        {
            logToConsole(L"Failed to save logging event data!");
            return false;
        }
#endif
        return true;
    }

    // This function is part of save/load feature and can only be called from main thread
    void loadLastLoggingStates(CHAR16* dir)
    {
#if ENABLED_LOGGING
        unsigned char* buffer = (unsigned char*)__scratchpad();
        static_assert(reorgBufferSize >= LOG_BUFFER_PAGE_SIZE + PMAP_LOG_PAGE_SIZE * sizeof(BlobInfo) + IMAP_LOG_PAGE_SIZE * sizeof(TickBlobInfo)
            + sizeof(digests) + 600, "scratchpad is too small");
        CHAR16 fileName[] = L"logEventState.db";
        const long long fileSz = getFileSize(fileName, dir);
        if (fileSz == -1)
        {
            logToConsole(L"[1] Failed to load logging events");
            return;
        }
        unsigned long long sz = load(fileName, fileSz, buffer, dir);
        if (fileSz != sz)
        {
            // failed to load
            logToConsole(L"[2] Failed to load logging events");
            return;
        }

        unsigned long long readSz = 0;
        // copy currentPage of log buffer ~ 100MiB
        sz = logBuffer.loadVMState(buffer);
        buffer += sz;
        readSz += sz;

        // copy current page of mapLogIdToBufferIndex ~ 80MiB
        sz = mapLogIdToBufferIndex.loadVMState(buffer);
        buffer += sz;
        readSz += sz;

        // mapTxToLogId ~ 150MiB
        sz = mapTxToLogId.loadVMState(buffer);
        buffer += sz;
        readSz += sz;

        // copy digests ~ 13MiB
        copyMem(digests, buffer, sizeof(digests));
        buffer += sizeof(digests);
        readSz += sizeof(digests);

        // copy k12 instance
        copyMem(&k12, buffer, sizeof(k12));
        buffer += sizeof(k12);
        readSz += sizeof(k12);

        // copy variables
        logBufferTail = *((unsigned long long*)buffer); buffer += 8;
        logId = *((unsigned long long*)buffer); buffer += 8;
        tickBegin = *((unsigned int*)buffer); buffer += 4;
        lastUpdatedTick = *((unsigned int*)buffer); buffer += 4;
        currentTxId = *((unsigned int*)buffer); buffer += 4;
        currentTick = *((unsigned int*)buffer);
#endif
    }

#endif

    template <typename T>
    void logQuTransfer(T message)
    {
#if LOG_SPECTRUM
        logMessage(offsetof(T, _terminator), QU_TRANSFER, &message);
#endif
    }

    template <typename T>
    void logAssetIssuance(T message)
    {
#if LOG_UNIVERSE
        logMessage(offsetof(T, _terminator), ASSET_ISSUANCE, &message);
#endif
    }

    template <typename T>
    void logAssetOwnershipChange(T message)
    {
#if LOG_UNIVERSE
        logMessage(offsetof(T, _terminator), ASSET_OWNERSHIP_CHANGE, &message);
#endif
    }

    template <typename T>
    void logAssetPossessionChange(T message)
    {
#if LOG_UNIVERSE
        logMessage(offsetof(T, _terminator), ASSET_POSSESSION_CHANGE, &message);
#endif
    }

    template <typename T>
    void logAssetOwnershipManagingContractChange(const T& message)
    {
#if LOG_UNIVERSE
        logMessage(offsetof(T, _terminator), ASSET_OWNERSHIP_MANAGING_CONTRACT_CHANGE, &message);
#endif
    }

    template <typename T>
    void logAssetPossessionManagingContractChange(const T& message)
    {
#if LOG_UNIVERSE
        logMessage(offsetof(T, _terminator), ASSET_POSSESSION_MANAGING_CONTRACT_CHANGE, &message);
#endif
    }

    template <typename T>
    void __logContractErrorMessage(unsigned int contractIndex, T& message)
    {
        static_assert(offsetof(T, _terminator) >= 8, "Invalid contract error message structure");

#if LOG_CONTRACT_ERROR_MESSAGES
        * ((unsigned int*)&message) = contractIndex;
        logMessage(offsetof(T, _terminator), CONTRACT_ERROR_MESSAGE, &message);
#endif

        // In order to keep state changes consistent independently of (a) whether logging is enabled and
        // (b) potential compiler optimizes, set contractIndex to 0 after logging
        * ((unsigned int*)&message) = 0;
    }

    template <typename T>
    void __logContractWarningMessage(unsigned int contractIndex, T& message)
    {
        static_assert(offsetof(T, _terminator) >= 8, "Invalid contract warning message structure");

#if LOG_CONTRACT_WARNING_MESSAGES
        * ((unsigned int*)&message) = contractIndex;
        logMessage(offsetof(T, _terminator), CONTRACT_WARNING_MESSAGE, &message);
#endif

        // In order to keep state changes consistent independently of (a) whether logging is enabled and
        // (b) potential compiler optimizes, set contractIndex to 0 after logging
        * ((unsigned int*)&message) = 0;
    }

    template <typename T>
    void __logContractInfoMessage(unsigned int contractIndex, T& message)
    {
        static_assert(offsetof(T, _terminator) >= 8, "Invalid contract info message structure");

#if LOG_CONTRACT_INFO_MESSAGES
        * ((unsigned int*)&message) = contractIndex;
        logMessage(offsetof(T, _terminator), CONTRACT_INFORMATION_MESSAGE, &message);
#endif

        // In order to keep state changes consistent independently of (a) whether logging is enabled and
        // (b) potential compiler optimizes, set contractIndex to 0 after logging
        * ((unsigned int*)&message) = 0;
    }

    template <typename T>
    void __logContractDebugMessage(unsigned int contractIndex, T& message)
    {
        static_assert(offsetof(T, _terminator) >= 8, "Invalid contract debug message structure");

#if LOG_CONTRACT_DEBUG_MESSAGES
        * ((unsigned int*)&message) = contractIndex;
        logMessage(offsetof(T, _terminator), CONTRACT_DEBUG_MESSAGE, &message);
#endif

        // In order to keep state changes consistent independently of (a) whether logging is enabled and
        // (b) potential compiler optimizes, set contractIndex to 0 after logging
        * ((unsigned int*)&message) = 0;
    }

    template <typename T>
    void logBurning(T message)
    {
#if LOG_SPECTRUM
        logMessage(offsetof(T, _terminator), BURNING, &message);
#endif
    }

    void logDustBurning(const DustBurning* message)
    {
#if LOG_SPECTRUM
        logMessage(message->messageSize(), DUST_BURNING, message);
#endif
    }

    void logSpectrumStats(const SpectrumStats& message)
    {
#if LOG_SPECTRUM
        logMessage(sizeof(SpectrumStats), SPECTRUM_STATS, &message);
#endif
    }

    template <typename T>
    void logCustomMessage(T message)
    {
        static_assert(offsetof(T, _terminator) >= 8, "Invalid custom message structure");

#if LOG_CUSTOM_MESSAGES
        logMessage(offsetof(T, _terminator), CUSTOM_MESSAGE, &message);
#endif
    }

    // get logging content from log ID
    static void processRequestLog(unsigned long long processorNumber, Peer* peer, RequestResponseHeader* header);

    // convert from tx id to log ID
    static void processRequestTxLogInfo(unsigned long long processorNumber, Peer* peer, RequestResponseHeader* header);

    // get all log ID (mapping to tx id) from a tick
    static void processRequestTickTxLogInfo(unsigned long long processorNumber, Peer* peer, RequestResponseHeader* header);

    // prune unused page files to save disk storage
    static void processRequestPrunePageFile(Peer* peer, RequestResponseHeader* header);

    // get log state digest
    static void processRequestGetLogDigest(Peer* peer, RequestResponseHeader* header);
};

GLOBAL_VAR_DECL qLogger logger;

// For smartcontract logging
template <typename T>
static void __logContractDebugMessage(unsigned int size, T& msg)
{
    logger.__logContractDebugMessage(size, msg);
}
template <typename T>
static void __logContractErrorMessage(unsigned int size, T& msg)
{
    logger.__logContractErrorMessage(size, msg);
}
template <typename T>
static void __logContractInfoMessage(unsigned int size, T& msg)
{
    logger.__logContractInfoMessage(size, msg);
}
template <typename T>
static void __logContractWarningMessage(unsigned int size, T& msg)
{
    logger.__logContractWarningMessage(size, msg);
}