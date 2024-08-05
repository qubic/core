#pragma once

#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/time.h"

#include "private_settings.h"
#include "system.h"
#include "network_core/peers.h"


// Fetches log
struct RequestLog
{
    unsigned long long passcode[4];

    enum {
        type = 44,
    };
};


// Returns buffered log; clears the buffer; make sure you fetch log quickly enough, if the buffer is overflown log stops being written into it till the node restart
struct RespondLog
{
    // Variable-size log;

    enum {
        type = 45,
    };
};



#define QU_TRANSFER 0
#define ASSET_ISSUANCE 1
#define ASSET_OWNERSHIP_CHANGE 2
#define ASSET_POSSESSION_CHANGE 3
#define CONTRACT_ERROR_MESSAGE 4
#define CONTRACT_WARNING_MESSAGE 5
#define CONTRACT_INFORMATION_MESSAGE 6
#define CONTRACT_DEBUG_MESSAGE 7
#define BURNING 8
#define CUSTOM_MESSAGE 255


/*
* STRUCTS FOR LOGGING
*/
struct QuTransfer
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    long long amount;
#if LOG_QU_TRANSFERS && LOG_QU_TRANSFERS_TRACK_TRANSFER_ID
    long long transferId;
#endif
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

/*
 * LOGGING IMPLEMENTATION
 */

class qLogger
{
public:
    unsigned long long logId;
    volatile char logBufferLocks;
    char* logBuffers;
    unsigned long long logBufferTails;
    bool logBufferOverflownFlags;
    bool initLogging()
    {
#if LOG_QU_TRANSFERS | LOG_ASSET_ISSUANCES | LOG_ASSET_OWNERSHIP_CHANGES | LOG_ASSET_POSSESSION_CHANGES | LOG_CONTRACT_ERROR_MESSAGES | LOG_CONTRACT_WARNING_MESSAGES | LOG_CONTRACT_INFO_MESSAGES | LOG_CONTRACT_DEBUG_MESSAGES | LOG_CUSTOM_MESSAGES
        logBufferOverflownFlags = false;
        logBufferTails = 0;
        logBuffers = NULL;
        logBufferLocks = 0;
        EFI_STATUS status;
        if (status = bs->AllocatePool(EfiRuntimeServicesData, LOG_BUFFER_SIZE, (void**)&logBuffers))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
#endif
        return true;
    }
    void deinitLogging()
    {
#if LOG_QU_TRANSFERS | LOG_ASSET_ISSUANCES | LOG_ASSET_OWNERSHIP_CHANGES | LOG_ASSET_POSSESSION_CHANGES | LOG_CONTRACT_ERROR_MESSAGES | LOG_CONTRACT_WARNING_MESSAGES | LOG_CONTRACT_INFO_MESSAGES | LOG_CONTRACT_DEBUG_MESSAGES | LOG_CUSTOM_MESSAGES
        bs->FreePool(logBuffers);
#endif
    }
    void logMessage(unsigned int messageSize, unsigned char messageType, const void* message)
    {
        ACQUIRE(logBufferLocks);

        if (!logBufferOverflownFlags && logBufferTails + 16 + messageSize <= LOG_BUFFER_SIZE)
        {
            *((unsigned char*)(logBuffers + (logBufferTails + 0))) = (unsigned char)(time.Year - 2000);
            *((unsigned char*)(logBuffers + (logBufferTails + 1))) = time.Month;
            *((unsigned char*)(logBuffers + (logBufferTails + 2))) = time.Day;
            *((unsigned char*)(logBuffers + (logBufferTails + 3))) = time.Hour;
            *((unsigned char*)(logBuffers + (logBufferTails + 4))) = time.Minute;
            *((unsigned char*)(logBuffers + (logBufferTails + 5))) = time.Second;

            *((unsigned short*)(logBuffers + (logBufferTails + 6))) = system.epoch;
            *((unsigned int*)(logBuffers + (logBufferTails + 8))) = system.tick;

            *((unsigned int*)(logBuffers + (logBufferTails + 12))) = messageSize | (messageType << 24);
            copyMem(logBuffers + (logBufferTails + 16), message, messageSize);
            logBufferTails += 16 + messageSize;
        }
        else
        {
            logBufferOverflownFlags = true;
        }

        RELEASE(logBufferLocks);
    }

    template <typename T>
    void logQuTransfer(T message)
    {
#if LOG_QU_TRANSFERS
        logMessage(offsetof(T, _terminator), QU_TRANSFER, &message);
#endif
    }

    template <typename T>
    void logAssetIssuance(T message)
    {
#if LOG_ASSET_ISSUANCES
        logMessage(offsetof(T, _terminator), ASSET_ISSUANCE, &message);
#endif
    }

    template <typename T>
    void logAssetOwnershipChange(T message)
    {
#if LOG_ASSET_OWNERSHIP_CHANGES
        logMessage(offsetof(T, _terminator), ASSET_OWNERSHIP_CHANGE, &message);
#endif
    }

    template <typename T>
    void logAssetPossessionChange(T message)
    {
#if LOG_ASSET_POSSESSION_CHANGES
        logMessage(offsetof(T, _terminator), ASSET_POSSESSION_CHANGE, &message);
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
#if LOG_BURNINGS
        logMessage(offsetof(T, _terminator), BURNING, &message);
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

    void processRequestLog(Peer* peer, RequestResponseHeader* header)
    {
        RequestLog* request = header->getPayload<RequestLog>();
        if (request->passcode[0] == logReaderPasscodes[0]
            && request->passcode[1] == logReaderPasscodes[1]
            && request->passcode[2] == logReaderPasscodes[2]
            && request->passcode[3] == logReaderPasscodes[3])
        {
            ACQUIRE(logBufferLocks);

            if (logBufferOverflownFlags)
            {
                RELEASE(logBufferLocks);
                return;
            }
            else
            {
                enqueueResponse(peer, logBufferTails, RespondLog::type, header->dejavu(), logBuffers);
                logBufferTails = 0;
            }
            RELEASE(logBufferLocks);
            return;
        }

        enqueueResponse(peer, 0, RespondLog::type, header->dejavu(), NULL);
    }
};

qLogger logger;

// For smartcontract logging
template <typename T> void __logContractDebugMessage(unsigned int size, T& msg)
{
    logger.__logContractDebugMessage(size, msg);
}
template <typename T> void __logContractErrorMessage(unsigned int size, T& msg)
{
    logger.__logContractErrorMessage(size, msg);
}
template <typename T> void __logContractInfoMessage(unsigned int size, T& msg)
{
    logger.__logContractInfoMessage(size, msg);
}
template <typename T> void __logContractWarningMessage(unsigned int size, T& msg)
{
    logger.__logContractWarningMessage(size, msg);
}