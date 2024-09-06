#define NO_UEFI

#include "gtest/gtest.h"

#define system qubicSystemStruct

#include "../src/spectrum.h"

#include <chrono>
#include <random>

static bool transfer(const m256i& src, const m256i& dst, long long amount)
{
    if (isZero(src) || isZero(dst))
        return false;

    if (amount < 0 || amount > MAX_AMOUNT)
        return false;
    
    const int index = spectrumIndex(src);
    if (index < 0)
        return false;

    if (!decreaseEnergy(index, amount))
        return false;

    increaseEnergy(dst, amount);
    return true;
}

static m256i getRichestEntity()
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

static m256i getAnyEntity()
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

static SpectrumInfo checkAndGetInfo()
{
    // Total amount <= total supply
    SpectrumInfo si{ 0, 0 };
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
        if (!balance && isZero(spectrum[i].publicKey))
            continue;
        EXPECT_GE(balance, 0);
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

static void printEntityCategoryPopulations()
{
    unsigned long long dustThresholdBurnAll, dustThresholdBurnThird;
    updateEntityCategoryPopulations();
    analyzeEntityCategoryPopulations(dustThresholdBurnAll, dustThresholdBurnThird);
    static constexpr int entityCategoryCount = sizeof(entityCategoryPopulations) / sizeof(entityCategoryPopulations[0]);
    for (int i = 0; i < entityCategoryCount; ++i)
    {
        if (entityCategoryPopulations[i])
        {
            unsigned long long lowerBound = (1llu << i), upperBound = (1llu << (i + 1)) - 1;
            const char* burnIndicator = "\t+ bin ";
            if (lowerBound < dustThresholdBurnAll)
                burnIndicator = "\t- bin ";
            else if (lowerBound < dustThresholdBurnThird)
                burnIndicator = "\t* bin ";
            std::cout << burnIndicator << i << ": " << entityCategoryPopulations[i] << " entities with amount ";
            if (i == 0)
                std::cout << lowerBound;
            else
                std::cout << "between " << lowerBound << " and " << upperBound;
            std::cout << std::endl;
        }
    }
}

// Spectrum test class for proper init, cleanup, and other repeated tasks
struct SpectrumTest
{
    SpectrumInfo beforeAntiDustSpectrumInfo;
    std::chrono::steady_clock::time_point beforeAntiDustTimestamp;
    bool antiDustCornerCase;
    std::mt19937_64 rnd64;

    SpectrumTest(unsigned long long seed = 0)
    {
        if (!seed)
            _rdrand64_step(&seed);
        rnd64.seed(seed);
        EXPECT_TRUE(initSpectrum());
        EXPECT_TRUE(initCommonBuffers());
        system.tick = 15600000;
        clearSpectrum();
        antiDustCornerCase = false;
    }

    ~SpectrumTest()
    {
        deinitSpectrum();
        deinitCommonBuffers();
    }

    void clearSpectrum()
    {
        memset(spectrum, 0, spectrumSizeInBytes);
        updateSpectrumInfo();
    }

    void beforeAntiDust()
    {
        // Check and get current spectrum state
        beforeAntiDustSpectrumInfo = checkAndGetInfo();

        // Print distribution of entity balances
        std::cout << "Entity balance distribution before anti-dust:" << std::endl;
        printEntityCategoryPopulations();

        // Start measuring run-time
        beforeAntiDustTimestamp = std::chrono::high_resolution_clock::now();
    }

    void afterAntiDust()
    {
        // Print anti-dust info
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - beforeAntiDustTimestamp);
        std::cout << "Transfer with anti-dust took " << duration_ms << " ms: entities "
            << beforeAntiDustSpectrumInfo.numberOfEntities << " -> " << spectrumInfo.numberOfEntities
            << " (to " << spectrumInfo.numberOfEntities * 100llu / SPECTRUM_CAPACITY
            << "% of capacity);  total amount " << beforeAntiDustSpectrumInfo.totalAmount << " -> " << spectrumInfo.totalAmount
            << " (" << ((long long)spectrumInfo.totalAmount - (long long)beforeAntiDustSpectrumInfo.totalAmount) * 100ll / beforeAntiDustSpectrumInfo.totalAmount << "% reduction)" << std::endl;

        // Print distribution of entity balances
        std::cout << "Entity balance distribution after anti-dust:" << std::endl;
        printEntityCategoryPopulations();

        // Anti-dust always cleans up to at least half of the spectrum
        EXPECT_LE(spectrumInfo.numberOfEntities, (SPECTRUM_CAPACITY / 2));

        // Except for improbably corner cases, never burn more than 10% of the spectrum (quite arbitrary factor, just meaning no huge amount)
        if (!antiDustCornerCase)
            EXPECT_GT(spectrumInfo.totalAmount, beforeAntiDustSpectrumInfo.totalAmount * 9 / 10);
    }

    void dust_attack(unsigned int transferMinAmount, unsigned int transferMaxAmount, unsigned int repetitions);
};

void SpectrumTest::dust_attack(unsigned int transferMinAmount, unsigned int transferMaxAmount, unsigned int repetitions)
{
    std::cout << "------------------ Dust attack with transfers between " << transferMinAmount << " and " << transferMaxAmount << " qu ------------------\n";
    for (unsigned int rep = 0; rep < repetitions; ++rep)
    {
        m256i richId = getRichestEntity();
        m256i randomId(rnd64(), rnd64(), rnd64(), rnd64());

        // Check current spectrum state
        checkAndGetInfo();

        // Fill spectrum with dust attack (until next transfer should trigger anti-dust)
        while (spectrumInfo.numberOfEntities < (SPECTRUM_CAPACITY / 2) + (SPECTRUM_CAPACITY / 4))
        {
            unsigned int transferAmount = transferMinAmount;
            if (transferMinAmount < transferMaxAmount)
                transferAmount = (spectrumInfo.numberOfEntities % (transferMaxAmount - transferMinAmount)) + transferMinAmount;

            if (!transfer(richId, randomId, transferAmount))
                richId = getRichestEntity();
            randomId = m256i(rnd64(), rnd64(), rnd64(), rnd64());
        }

        // Should trigger anti-dust
        beforeAntiDust();
        ASSERT_TRUE(transfer(richId, randomId, transferMinAmount));
        afterAntiDust();
    }
}


TEST(TestCoreSpectrum, AntiDustFile)
{
    SpectrumTest test;
    if (loadSpectrum(L"spectrum.000"))
    {
        SpectrumInfo si1 = checkAndGetInfo();
        test.dust_attack(1, 10, 3);
    }
    else
    {
        std::cout << "Spectrum file not found. Skipping file test..." << std::endl;
    }
}

TEST(TestCoreSpectrum, AntiDustOneRichRandomDust)
{
    // Create spectrum with one rich ID
    SpectrumTest test;
    increaseEnergy(m256i::randomValue(), 1000000000000llu);
    spectrumInfo.totalAmount += 1000000000000llu;

    test.dust_attack(1, 1, 1);
    test.dust_attack(100, 100, 1);
    test.dust_attack(1, 10000, 1);
}

TEST(TestCoreSpectrum, AntiDustManyRichRandomDust)
{
    // Create spectrum with many rich IDs
    SpectrumTest test;
    for (int i = 0; i < 10000; i++)
    {
        increaseEnergy(m256i::randomValue(), i * 100000llu);
        spectrumInfo.totalAmount += i * 100000llu;
    }

    test.dust_attack(1, 1000, 1);
    test.dust_attack(1, 50, 1);
    test.dust_attack(1, 1, 1);
}

TEST(TestCoreSpectrum, AntiDustEdgeCaseAllInSameBin)
{
    SpectrumTest test;
    test.antiDustCornerCase = true;
    for (unsigned long long i = 0; i < (SPECTRUM_CAPACITY / 2 + SPECTRUM_CAPACITY / 4); ++i)
    {
        increaseEnergy(m256i(i, 1, 2, 3), 100llu);
        spectrumInfo.totalAmount += 100llu;
    }

    test.beforeAntiDust();
    increaseEnergy(m256i::randomValue(), 100llu);
    test.afterAntiDust();
}


TEST(TestCoreSpectrum, AntiDustEdgeCaseHugeBins)
{
    SpectrumTest test;
    for (unsigned long long i = 0; i < (SPECTRUM_CAPACITY / 2 + SPECTRUM_CAPACITY / 4); ++i)
    {
        unsigned long long amount;
        if (i < SPECTRUM_CAPACITY / 4)
            amount = 100;
        else if (i < SPECTRUM_CAPACITY / 2)
            amount = 1000;
        else if (i < SPECTRUM_CAPACITY / 2 + SPECTRUM_CAPACITY / 4)
            amount = 10000;
        increaseEnergy(m256i(i, 1, 2, 3), amount);
        spectrumInfo.totalAmount += amount;
    }
    test.beforeAntiDust();
    increaseEnergy(m256i::randomValue(), 1000llu);
    test.afterAntiDust();
}
