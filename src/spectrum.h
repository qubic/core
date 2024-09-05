#pragma once

#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"

#include "network_messages/entity.h"

#include "public_settings.h"
#include "system.h"
#include "kangaroo_twelve.h"
#include "common_buffers.h"


static volatile char spectrumLock = 0;
static ::Entity* spectrum = nullptr;
static struct SpectrumInfo {
    unsigned int numberOfEntities = 0;
    long long totalAmount = 0;
} spectrumInfo;
static unsigned int entityCategoryPopulations[48]; // Array size depends on max possible balance
static m256i* spectrumDigests = nullptr;
constexpr unsigned long long spectrumDigestsSizeInByte = (SPECTRUM_CAPACITY * 2 - 1) * 32ULL;


// Update SpectrumInfo data (exensive, because it iterates the whole spectrum), acquire no lock
void updateSpectrumInfo(SpectrumInfo& si = spectrumInfo)
{
    si.numberOfEntities = 0;
    si.totalAmount = 0;
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (balance)
        {
            si.numberOfEntities++;
            si.totalAmount += balance;
        }
    }
}

static void reorganizeSpectrum()
{
    ::Entity* reorgSpectrum = (::Entity*)reorgBuffer;
    bs->SetMem(reorgSpectrum, SPECTRUM_CAPACITY * sizeof(::Entity), 0);
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        if (spectrum[i].incomingAmount - spectrum[i].outgoingAmount)
        {
            unsigned int index = spectrum[i].publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

        iteration:
            if (isZero(reorgSpectrum[index].publicKey))
            {
                bs->CopyMem(&reorgSpectrum[index], &spectrum[i], sizeof(::Entity));
            }
            else
            {
                index = (index + 1) & (SPECTRUM_CAPACITY - 1);

                goto iteration;
            }
        }
    }
    bs->CopyMem(spectrum, reorgSpectrum, SPECTRUM_CAPACITY * sizeof(::Entity));

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

static void increaseEnergy(const m256i& publicKey, long long amount)
{
    if (!isZero(publicKey) && amount >= 0)
    {
        unsigned int index = publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

        ACQUIRE(spectrumLock);

        if (spectrumInfo.numberOfEntities >= (SPECTRUM_CAPACITY / 2) + (SPECTRUM_CAPACITY / 4))
        {
            setMem(entityCategoryPopulations, sizeof(entityCategoryPopulations), 0);

            for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
            {
                const unsigned long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
                if (balance)
                {
                    entityCategoryPopulations[63 - __lzcnt64(balance)]++;
                }
            }

            unsigned int newNumberOfEntities = 0;
            for (unsigned int categoryIndex = sizeof(entityCategoryPopulations) / sizeof(entityCategoryPopulations[0]); categoryIndex-- > 0; )
            {
                if ((newNumberOfEntities += entityCategoryPopulations[categoryIndex]) >= SPECTRUM_CAPACITY / 2)
                {
                    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
                    {
                        if (__lzcnt64((unsigned long long)(spectrum[i].incomingAmount - spectrum[i].outgoingAmount)) > 63 - categoryIndex)
                        {
                            spectrum[i].outgoingAmount = spectrum[i].incomingAmount;
                        }
                    }

                    reorganizeSpectrum();

                    // Correct total amount (spectrum info has been recomputed after decreasing
                    // but before increasing energy again)
                    spectrumInfo.totalAmount += amount;

                    break;
                }
            }
        }

    iteration:
        if (spectrum[index].publicKey == publicKey)
        {
            spectrum[index].incomingAmount += amount;
            spectrum[index].numberOfIncomingTransfers++;
            spectrum[index].latestIncomingTransferTick = system.tick;
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

            RELEASE(spectrumLock);

            return true;
        }

        RELEASE(spectrumLock);
    }

    return false;
}


static bool loadSpectrum(CHAR16* directory)
{
    logToConsole(L"Loading spectrum file ...");
    long long loadedSize = load(SPECTRUM_FILE_NAME, SPECTRUM_CAPACITY * sizeof(::Entity), (unsigned char*)spectrum, directory);
    if (loadedSize != SPECTRUM_CAPACITY * sizeof(::Entity))
    {
        logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);

        return false;
    }
    return true;
}

static bool saveSpectrum(CHAR16* directory)
{
    logToConsole(L"Saving spectrum file...");

    const unsigned long long beginningTick = __rdtsc();

    ACQUIRE(spectrumLock);
    long long savedSize = save(SPECTRUM_FILE_NAME, SPECTRUM_CAPACITY * sizeof(::Entity), (unsigned char*)spectrum, directory);
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
    if (!allocatePool(spectrumSizeInBytes, (void**)&spectrum)
        || !allocatePool(spectrumDigestsSizeInByte, (void**)&spectrumDigests))
    {
        logToConsole(L"Failed to allocate spectrum memory!");
        return false;
    }

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
