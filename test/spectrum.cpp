#define NO_UEFI

#include "gtest/gtest.h"

#define system qubicSystemStruct

#include "../src/spectrum.h"

#include <chrono>


bool transfer(const m256i& src, const m256i& dst, long long amount)
{
    if (amount < 0 || amount > MAX_AMOUNT)
        return false;
    
    const int index = spectrumIndex(src);
    if (index < 0)
        return false;

    if (decreaseEnergy(index, amount))
        increaseEnergy(dst, amount);

    return true;
}

m256i getRichestEntity()
{
    m256i pubKey(0, 0, 0, 0);
    long long maxBalance = 0;
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (balance > maxBalance)
        {
            maxBalance = balance;
            pubKey = spectrum[i].publicKey;
        }
    }
    return pubKey;
}

m256i getAnyEntity()
{
    m256i pubKey(0, 0, 0, 0);
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (balance > 0)
        {
            pubKey = spectrum[i].publicKey;
            break;
        }
    }
    return pubKey;
}

SpectrumInfo checkAndGetInfo()
{
    // Total amount <= total supply
    SpectrumInfo si{ 0, 0 };
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (balance == 0)
            continue;
        EXPECT_GT(balance, 0);
        EXPECT_LE(spectrum[i].latestIncomingTransferTick, system.tick);
        EXPECT_LE(spectrum[i].latestOutgoingTransferTick, system.tick);
        si.totalAmount += balance;
        si.numberOfEntities++;
    }
    EXPECT_LE((unsigned long long)si.totalAmount, MAX_SUPPLY);
    EXPECT_EQ(si.totalAmount, spectrumInfo.totalAmount);
    EXPECT_EQ(si.numberOfEntities, spectrumInfo.numberOfEntities);
    return si;
}

void printEntityCategoryPopulations()
{
    std::cout << "entityCategoryPopulations:\n";
    static constexpr int entityCategoryCount = sizeof(entityCategoryPopulations) / sizeof(entityCategoryPopulations[0]);
    for (int i = 0; i < entityCategoryCount; ++i)
    {
        if (entityCategoryPopulations[i])
        {
            std::cout << '\t' << i << ": " << entityCategoryPopulations[i] << " entities with amount ";
            if (i == 0)
                std::cout << (1llu << i);
            else
                std::cout << "between " << (1llu << i) << " and " << (1llu << (i + 1)) - 1;
            std::cout << std::endl;
        }
    }
}

void dust_attack(unsigned int transferAmount, unsigned int repetitions)
{
    for (unsigned int rep = 0; rep < repetitions; ++rep)
    {
        m256i richId = getRichestEntity();
        m256i randomId = m256i::randomValue();

        // Check current spectrum state
        checkAndGetInfo();

        // Fill spectrum with dust attack (until next transfer should trigger anti-dust)
        while (spectrumInfo.numberOfEntities < (SPECTRUM_CAPACITY / 2) + (SPECTRUM_CAPACITY / 4))
        {
            transfer(richId, randomId, transferAmount);
            randomId = m256i::randomValue();
        }

        // Check and get current spectrum state
        SpectrumInfo si1 = checkAndGetInfo();

        // Print distribution of entity balances
        updateEntityCategoryPopulations();
        printEntityCategoryPopulations();

        // Should trigger anti-dust
        auto t0 = std::chrono::high_resolution_clock::now();
        transfer(richId, randomId, transferAmount);
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t0);
        std::cout << "Transfer with anti-dust took " << duration_ms << " ms: entities "
            << si1.numberOfEntities << " -> " << spectrumInfo.numberOfEntities << " (delta " << (long long)spectrumInfo.numberOfEntities - (long long)si1.numberOfEntities
            << ");  total amount " << si1.totalAmount << " -> " << spectrumInfo.totalAmount << " (delta " << (long long)spectrumInfo.totalAmount - (long long)si1.totalAmount << ")" << std::endl;
    }
}

TEST(TestCoreSpectrum, AntiDustFile)
{
    EXPECT_TRUE(initSpectrum());
    EXPECT_TRUE(initCommonBuffers());

    if (loadSpectrum(L"spectrum.000"))
    {
        updateSpectrumInfo();
        system.tick = 15600000;

        SpectrumInfo si1 = checkAndGetInfo();
        dust_attack(1, 3);
    }
    else
    {
        std::cout << "Spectrum file not found. Skipping file test..." << std::endl;
    }

    deinitSpectrum();
    deinitCommonBuffers();
}

TEST(TestCoreSpectrum, AntiDustOneRichRandomDust)
{
    EXPECT_TRUE(initSpectrum());
    EXPECT_TRUE(initCommonBuffers());

    // Create spectrum with one rich ID
    memset(spectrum, 0, spectrumSizeInBytes);
    system.tick = 15600000;
    increaseEnergy(m256i::randomValue(), 1000000000000llu);
    updateSpectrumInfo();

    dust_attack(1, 2);
    dust_attack(100, 2);
    dust_attack(10000, 2);

    deinitSpectrum();
    deinitCommonBuffers();
}
