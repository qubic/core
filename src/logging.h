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
static volatile char logBufferLocks[sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0])] = { 0 };
static char* logBuffers[sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0])] = { NULL };
static unsigned int logBufferTails[sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0])] = { 0 };
static bool logBufferOverflownFlags[sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0])] = { false };


static bool initLogging()
{
#if LOG_QU_TRANSFERS | LOG_ASSET_ISSUANCES | LOG_ASSET_OWNERSHIP_CHANGES | LOG_ASSET_POSSESSION_CHANGES | LOG_CONTRACT_ERROR_MESSAGES | LOG_CONTRACT_WARNING_MESSAGES | LOG_CONTRACT_INFO_MESSAGES | LOG_CONTRACT_DEBUG_MESSAGES | LOG_CUSTOM_MESSAGES
    EFI_STATUS status;
    for (unsigned int logReaderIndex = 0; logReaderIndex < sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0]); logReaderIndex++)
    {
        if (status = bs->AllocatePool(EfiRuntimeServicesData, LOG_BUFFER_SIZE, (void**)&logBuffers[logReaderIndex]))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
    }
#endif
    return true;
}

static void deinitLogging()
{
#if LOG_QU_TRANSFERS | LOG_ASSET_ISSUANCES | LOG_ASSET_OWNERSHIP_CHANGES | LOG_ASSET_POSSESSION_CHANGES | LOG_CONTRACT_ERROR_MESSAGES | LOG_CONTRACT_WARNING_MESSAGES | LOG_CONTRACT_INFO_MESSAGES | LOG_CONTRACT_DEBUG_MESSAGES | LOG_CUSTOM_MESSAGES
    for (unsigned int logReaderIndex = 0; logReaderIndex < sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0]); logReaderIndex++)
    {
        if (logBuffers[logReaderIndex])
        {
            bs->FreePool(logBuffers[logReaderIndex]);
        }
    }
#endif
}

static void logMessage(unsigned int messageSize, unsigned char messageType, void* message)
{
    for (unsigned int logReaderIndex = 0; logReaderIndex < sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0]); logReaderIndex++)
    {
        ACQUIRE(logBufferLocks[logReaderIndex]);

        if (!logBufferOverflownFlags[logReaderIndex] && logBufferTails[logReaderIndex] + 16 + messageSize <= LOG_BUFFER_SIZE)
        {
            *((unsigned char*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 0))) = (unsigned char)(time.Year - 2000);
            *((unsigned char*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 1))) = time.Month;
            *((unsigned char*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 2))) = time.Day;
            *((unsigned char*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 3))) = time.Hour;
            *((unsigned char*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 4))) = time.Minute;
            *((unsigned char*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 5))) = time.Second;

            *((unsigned short*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 6))) = system.epoch;
            *((unsigned int*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 8))) = system.tick;

            *((unsigned int*)(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 12))) = messageSize | (messageType << 24);
            bs->CopyMem(logBuffers[logReaderIndex] + (logBufferTails[logReaderIndex] + 16), message, messageSize);
            logBufferTails[logReaderIndex] += 16 + messageSize;
        }
        else
        {
            logBufferOverflownFlags[logReaderIndex] = true;
        }

        RELEASE(logBufferLocks[logReaderIndex]);
    }
}

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

#if LOG_QU_TRANSFERS && LOG_QU_TRANSFERS_TRACK_TRANSFER_ID
long long CurrentTransferId = 0;
static volatile char CurrentTransferIdLock = 0;
#endif

template <typename T>
static void logQuTransfer(T message)
{
#if LOG_QU_TRANSFERS
#if LOG_QU_TRANSFERS_TRACK_TRANSFER_ID
    ACQUIRE(CurrentTransferIdLock);
    message.transferId = CurrentTransferId++;
    RELEASE(CurrentTransferIdLock);
#endif
    logMessage(offsetof(T, _terminator), QU_TRANSFER, &message);
#endif
}

struct AssetIssuance
{
    m256i issuerPublicKey;
    long long numberOfShares;
    char name[7];
    char numberOfDecimalPlaces;
    char unitOfMeasurement[7];

    char _terminator; // Only data before "_terminator" are logged
};

template <typename T>
static void logAssetIssuance(T message)
{
#if LOG_ASSET_ISSUANCES
    logMessage(offsetof(T, _terminator), ASSET_ISSUANCE, &message);
#endif
}

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

template <typename T>
static void logAssetOwnershipChange(T message)
{
#if LOG_ASSET_OWNERSHIP_CHANGES
    logMessage(offsetof(T, _terminator), ASSET_OWNERSHIP_CHANGE, &message);
#endif
}

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

template <typename T>
static void logAssetPossessionChange(T message)
{
#if LOG_ASSET_POSSESSION_CHANGES
    logMessage(offsetof(T, _terminator), ASSET_POSSESSION_CHANGE, &message);
#endif
}

struct DummyContractErrorMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

template <typename T>
static void __logContractErrorMessage(T message)
{
    static_assert(offsetof(T, _terminator) >= 8, "Invalid contract error message structure");

#if LOG_CONTRACT_ERROR_MESSAGES
    * ((unsigned int*)&message) = executedContractIndex;
    logMessage(offsetof(T, _terminator), CONTRACT_ERROR_MESSAGE, &message);
#endif
}

struct DummyContractWarningMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

template <typename T>
static void __logContractWarningMessage(T message)
{
    static_assert(offsetof(T, _terminator) >= 8, "Invalid contract warning message structure");

#if LOG_CONTRACT_WARNING_MESSAGES
    * ((unsigned int*)&message) = executedContractIndex;
    logMessage(offsetof(T, _terminator), CONTRACT_WARNING_MESSAGE, &message);
#endif
}

struct DummyContractInfoMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

template <typename T>
static void __logContractInfoMessage(T message)
{
    static_assert(offsetof(T, _terminator) >= 8, "Invalid contract info message structure");

#if LOG_CONTRACT_INFO_MESSAGES
    * ((unsigned int*)&message) = executedContractIndex;
    logMessage(offsetof(T, _terminator), CONTRACT_INFORMATION_MESSAGE, &message);
#endif
}

struct DummyContractDebugMessage
{
    unsigned int _contractIndex; // Auto-assigned, any previous value will be overwritten
    unsigned int _type; // Assign a random unique (per contract) number to distinguish messages of different types

    // Other data go here

    char _terminator; // Only data before "_terminator" are logged
};

template <typename T>
static void __logContractDebugMessage(T message)
{
    static_assert(offsetof(T, _terminator) >= 8, "Invalid contract debug message structure");

#if LOG_CONTRACT_DEBUG_MESSAGES
    * ((unsigned int*)&message) = executedContractIndex;
    logMessage(offsetof(T, _terminator), CONTRACT_DEBUG_MESSAGE, &message);
#endif
}

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

template <typename T>
static void logBurning(T message)
{
#if LOG_BURNINGS
    logMessage(offsetof(T, _terminator), BURNING, &message);
#endif
}

template <typename T>
static void logCustomMessage(T message)
{
    static_assert(offsetof(T, _terminator) >= 8, "Invalid custom message structure");

#if LOG_CUSTOM_MESSAGES
    logMessage(offsetof(T, _terminator), CUSTOM_MESSAGE, &message);
#endif
}

static void processRequestLog(Peer* peer, RequestResponseHeader* header)
{
    RequestLog* request = header->getPayload<RequestLog>();
    for (unsigned int logReaderIndex = 0; logReaderIndex < sizeof(logReaderPasscodes) / sizeof(logReaderPasscodes[0]); logReaderIndex++)
    {
        if (request->passcode[0] == logReaderPasscodes[logReaderIndex][0]
            && request->passcode[1] == logReaderPasscodes[logReaderIndex][1]
            && request->passcode[2] == logReaderPasscodes[logReaderIndex][2]
            && request->passcode[3] == logReaderPasscodes[logReaderIndex][3])
        {
            ACQUIRE(logBufferLocks[logReaderIndex]);

            if (logBufferOverflownFlags[logReaderIndex])
            {
                RELEASE(logBufferLocks[logReaderIndex]);

                break;
            }
            else
            {
                enqueueResponse(peer, logBufferTails[logReaderIndex], RespondLog::type, header->dejavu(), logBuffers[logReaderIndex]);
                logBufferTails[logReaderIndex] = 0;
            }

            RELEASE(logBufferLocks[logReaderIndex]);

            return;
        }
    }

    enqueueResponse(peer, 0, RespondLog::type, header->dejavu(), NULL);
}
