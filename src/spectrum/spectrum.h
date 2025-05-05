#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"
#include "platform/memory.h"
#include "platform/profiling.h"

#include "network_messages/entity.h"

#include "logging/logging.h"

#include "public_settings.h"
#include "system.h"
#include "kangaroo_twelve.h"
#include "common_buffers.h"

GLOBAL_VAR_DECL volatile char spectrumLock GLOBAL_VAR_INIT(0);
GLOBAL_VAR_DECL ::Entity* spectrum GLOBAL_VAR_INIT(nullptr);
GLOBAL_VAR_DECL struct SpectrumInfo {
    unsigned int numberOfEntities = 0;  // Number of entities in the spectrum hash map, may include entries with balance == 0
    unsigned long long totalAmount = 0; // Total amount of qubics in the spectrum
} spectrumInfo;

GLOBAL_VAR_DECL unsigned int entityCategoryPopulations[48]; // Array size depends on max possible balance
static constexpr unsigned char entityCategoryCount = sizeof(entityCategoryPopulations) / sizeof(entityCategoryPopulations[0]);
GLOBAL_VAR_DECL unsigned long long dustThresholdBurnAll GLOBAL_VAR_INIT(0), dustThresholdBurnHalf GLOBAL_VAR_INIT(0);

GLOBAL_VAR_DECL m256i* spectrumDigests GLOBAL_VAR_INIT(nullptr);
static constexpr unsigned long long spectrumDigestsSizeInByte = (SPECTRUM_CAPACITY * 2 - 1) * 32ULL;

GLOBAL_VAR_DECL unsigned long long spectrumReorgTotalExecutionTicks GLOBAL_VAR_INIT(0);


// Update SpectrumInfo data (exensive, because it iterates the whole spectrum), acquire no lock
static void updateSpectrumInfo(SpectrumInfo& si = spectrumInfo)
{
    PROFILE_SCOPE();
    si.numberOfEntities = 0;
    si.totalAmount = 0;
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (balance || !isZero(spectrum[i].publicKey))
        {
            si.numberOfEntities++;
            si.totalAmount += balance;
        }
    }
}

// Compute balances that count as dust and are burned if 75% of spectrum hash map is filled.
// All balances <= dustThresholdBurnAll are burned in this case.
// Every 2nd balance <= dustThresholdBurnHalf is burned in this case.
static void updateAndAnalzeEntityCategoryPopulations()
{
    PROFILE_SCOPE();
    static_assert(MAX_SUPPLY < (1llu << entityCategoryCount));
    setMem(entityCategoryPopulations, sizeof(entityCategoryPopulations), 0);

    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        const unsigned long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (balance)
        {
            entityCategoryPopulations[63 - __lzcnt64(balance)]++;
        }
    }

    dustThresholdBurnAll = 0;
    dustThresholdBurnHalf = 0;
    unsigned int numberOfEntities = 0;
    for (unsigned int categoryIndex = entityCategoryCount; categoryIndex-- > 0; )
    {
        if ((numberOfEntities += entityCategoryPopulations[categoryIndex]) >= SPECTRUM_CAPACITY / 2)
        {
            // Balances in this category + higher balances fill more than half of the spectrum
            // -> Reduce to less than half of the spectrum by detelting this category + lower balances
            if (entityCategoryPopulations[categoryIndex] == numberOfEntities)
            {
                // Corner case handling: if all entities would be deleted, burn only half in the current category
                // and all in the the lower categories
                dustThresholdBurnHalf = (1llu << (categoryIndex + 1)) - 1;
                dustThresholdBurnAll = (1llu << categoryIndex) - 1;
            }
            else
            {
                // Regular case: burn all balances in current category and smaller (reduces
                // spectrum to < 50% of capacity, but keeps as many entities as possible
                // within this contraint and the granularity of categories
                dustThresholdBurnAll = (1llu << (categoryIndex + 1)) - 1;
            }
            break;
        }
    }
}

static void logSpectrumStats()
{
    SpectrumStats spectrumStats;
    spectrumStats.totalAmount = spectrumInfo.totalAmount;
    spectrumStats.dustThresholdBurnAll = dustThresholdBurnAll;
    spectrumStats.dustThresholdBurnHalf = dustThresholdBurnHalf;
    spectrumStats.numberOfEntities = spectrumInfo.numberOfEntities;
    copyMem(spectrumStats.entityCategoryPopulations, entityCategoryPopulations, sizeof(entityCategoryPopulations));

    logger.logSpectrumStats(spectrumStats);
}

// Build and log variable-size DustBurning log message.
// Assumes to be used tick processor or contract processor only, so can use reorgBuffer.
struct DustBurnLogger
{
    DustBurnLogger()
    {
        buf = (DustBurning*)reorgBuffer;
        buf->numberOfBurns = 0;
    }

    // Add burned amount of of entity, may send buffered message to logging.
    void addDustBurn(const m256i& publicKey, unsigned long long amount)
    {
        DustBurning::Entity& e = buf->entity(buf->numberOfBurns++);
        e.publicKey = publicKey;
        e.amount = amount;

        if (buf->numberOfBurns == 1000) // TODO: better use 0xffff (when proved to be stable with logging)
            finished();
    }

    // Send buffered message to logging
    void finished()
    {
        logger.logDustBurning(buf);
        buf->numberOfBurns = 0;
    }

    // send to log when max size is full or finished
private:
    DustBurning* buf;
};

// Clean up spectrum hash map, removing all entities with balance 0. Updates spectrumInfo.
static void reorganizeSpectrum()
{
    PROFILE_SCOPE();

    unsigned long long spectrumReorgStartTick = __rdtsc();

    ::Entity* reorgSpectrum = (::Entity*)reorgBuffer;
    setMem(reorgSpectrum, SPECTRUM_CAPACITY * sizeof(::Entity), 0);
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        if (spectrum[i].incomingAmount - spectrum[i].outgoingAmount)
        {
            unsigned int index = spectrum[i].publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

        iteration:
            if (isZero(reorgSpectrum[index].publicKey))
            {
                copyMem(&reorgSpectrum[index], &spectrum[i], sizeof(::Entity));
            }
            else
            {
                index = (index + 1) & (SPECTRUM_CAPACITY - 1);

                goto iteration;
            }
        }
    }
    copyMem(spectrum, reorgSpectrum, SPECTRUM_CAPACITY * sizeof(::Entity));

    unsigned int digestIndex;
    for (digestIndex = 0; digestIndex < SPECTRUM_CAPACITY; digestIndex++)
    {
        KangarooTwelve64To32(&spectrum[digestIndex], &spectrumDigests[digestIndex]);
    }
    unsigned int previousLevelBeginning = 0;
    unsigned int numberOfLeafs = SPECTRUM_CAPACITY;
    while (numberOfLeafs > 1)
    {
        for (unsigned int i = 0; i < numberOfLeafs; i += 2)
        {
            KangarooTwelve64To32(&spectrumDigests[previousLevelBeginning + i], &spectrumDigests[digestIndex++]);
        }

        previousLevelBeginning += numberOfLeafs;
        numberOfLeafs >>= 1;
    }

    updateSpectrumInfo();

    spectrumReorgTotalExecutionTicks += __rdtsc() - spectrumReorgStartTick;
}

static int spectrumIndex(const m256i& publicKey)
{
    if (isZero(publicKey))
    {
        return -1;
    }

    unsigned int index = publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

    ACQUIRE(spectrumLock);

iteration:
    if (spectrum[index].publicKey == publicKey)
    {
        RELEASE(spectrumLock);

        return index;
    }
    else
    {
        if (isZero(spectrum[index].publicKey))
        {
            RELEASE(spectrumLock);

            return -1;
        }
        else
        {
            index = (index + 1) & (SPECTRUM_CAPACITY - 1);

            goto iteration;
        }
    }
}

static long long energy(const int index)
{
    return spectrum[index].incomingAmount - spectrum[index].outgoingAmount;
}

// Increase balance of entity.
static void increaseEnergy(const m256i& publicKey, long long amount)
{
    if (!isZero(publicKey) && amount >= 0)
    {
        unsigned int index = publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

        ACQUIRE(spectrumLock);

        // Anti-dust feature: prevent that spectrum fills to more than 75% of capacity to keep hash map lookup fast
        if (spectrumInfo.numberOfEntities >= (SPECTRUM_CAPACITY / 2) + (SPECTRUM_CAPACITY / 4))
        {
            // Update anti-dust burn thresholds (and log spectrum stats before burning)
            updateAndAnalzeEntityCategoryPopulations();
#if LOG_SPECTRUM
            logSpectrumStats();
#endif
#if LOG_SPECTRUM
            DustBurnLogger dbl;
#endif

            if (dustThresholdBurnAll > 0)
            {
                // Burn every balance with balance < dustThresholdBurnAll
                for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
                {
                    const unsigned long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
                    if (balance <= dustThresholdBurnAll && balance)
                    {
                        spectrum[i].outgoingAmount = spectrum[i].incomingAmount;
#if LOG_SPECTRUM
                        dbl.addDustBurn(spectrum[i].publicKey, balance);
#endif
                    }
                }
            }

            if (dustThresholdBurnHalf > 0)
            {
                // Burn every second balance with balance < dustThresholdBurnHalf
                unsigned int countBurnCanadiates = 0;
                for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
                {
                    const unsigned long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
                    if (balance <= dustThresholdBurnHalf && balance)
                    {
                        if (++countBurnCanadiates & 1)
                        {
                            spectrum[i].outgoingAmount = spectrum[i].incomingAmount;
#if LOG_SPECTRUM
                            dbl.addDustBurn(spectrum[i].publicKey, balance);
#endif
                        }
                    }
                }
            }

#if LOG_SPECTRUM
            // Finished dust burning (pass message to log)
            dbl.finished();
#endif

            // Remove entries with balance zero from hash map
            reorganizeSpectrum();

#if LOG_SPECTRUM
            // Log spectrum stats after burning (before increasing energy / potenitally creating entity)
            updateAndAnalzeEntityCategoryPopulations();
            logSpectrumStats();
#endif
        }

    iteration:
        if (spectrum[index].publicKey == publicKey)
        {
            spectrum[index].incomingAmount += amount;
            spectrum[index].numberOfIncomingTransfers++;
            spectrum[index].latestIncomingTransferTick = system.tick;

            spectrumInfo.totalAmount += amount;
        }
        else
        {
            if (isZero(spectrum[index].publicKey))
            {
                spectrum[index].publicKey = publicKey;
                spectrum[index].incomingAmount = amount;
                spectrum[index].numberOfIncomingTransfers = 1;
                spectrum[index].latestIncomingTransferTick = system.tick;

                spectrumInfo.numberOfEntities++;
                spectrumInfo.totalAmount += amount;

#if LOG_SPECTRUM
                if ((spectrumInfo.numberOfEntities & 0x7ffff) == 1)
                {
                    // Log spectrum stats when the number of entities hits the next half million
                    // (== 1 is to avoid duplicate when anti-dust is triggered)
                    updateAndAnalzeEntityCategoryPopulations();
                    logSpectrumStats();
                }
#endif
            }
            else
            {
                index = (index + 1) & (SPECTRUM_CAPACITY - 1);

                goto iteration;
            }
        }

        RELEASE(spectrumLock);
    }
}

// Decrease balance of entity if it is high enough. Does NOT check if index is valid.
static bool decreaseEnergy(const int index, long long amount)
{
    if (amount >= 0)
    {
        ACQUIRE(spectrumLock);

        if (energy(index) >= amount)
        {
            spectrum[index].outgoingAmount += amount;
            spectrum[index].numberOfOutgoingTransfers++;
            spectrum[index].latestOutgoingTransferTick = system.tick;

            spectrumInfo.totalAmount -= amount;

            RELEASE(spectrumLock);

            return true;
        }

        RELEASE(spectrumLock);
    }

    return false;
}


static bool loadSpectrum(const CHAR16* fileName = SPECTRUM_FILE_NAME, const CHAR16* directory = nullptr)
{
    logToConsole(L"Loading spectrum file ...");
    long long loadedSize = load(fileName, SPECTRUM_CAPACITY * sizeof(::Entity), (unsigned char*)spectrum, directory);
    if (loadedSize != SPECTRUM_CAPACITY * sizeof(::Entity))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);

        return false;
    }
    updateSpectrumInfo();
    return true;
}

static bool saveSpectrum(const CHAR16* fileName = SPECTRUM_FILE_NAME, const CHAR16* directory = nullptr)
{
    logToConsole(L"Saving spectrum file...");

    const unsigned long long beginningTick = __rdtsc();

    ACQUIRE(spectrumLock);
    long long savedSize = save(fileName, SPECTRUM_CAPACITY * sizeof(::Entity), (unsigned char*)spectrum, directory);
    RELEASE(spectrumLock);

    if (savedSize == SPECTRUM_CAPACITY * sizeof(::Entity))
    {
        setNumber(message, savedSize, TRUE);
        appendText(message, L" bytes of the spectrum data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
        return true;
    }
    return false;
}

static bool initSpectrum()
{
    if (!allocPoolWithErrorLog(L"spectrum", spectrumSizeInBytes, (void**)&spectrum, __LINE__)
        || !allocPoolWithErrorLog(L"spectrumDigests", spectrumDigestsSizeInByte, (void**)&spectrumDigests, __LINE__))
    {
        return false;
    }
    spectrumLock = 0;

    return true;
}

static void deinitSpectrum()
{
    if (spectrumDigests)
    {
        freePool(spectrumDigests);
    }
    if (spectrum)
    {
        freePool(spectrum);
    }
}
