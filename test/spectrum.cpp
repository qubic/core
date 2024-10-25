#define NO_UEFI

#define PRINT_TEST_INFO 0

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

// enable some logging for testing
#include "../src/private_settings.h"
#undef LOG_DUST_BURNINGS
#undef LOG_SPECTRUM_STATS
#define LOG_DUST_BURNINGS 1
#define LOG_SPECTRUM_STATS 1

// reduced size of logging buffer (512 MB instead of 8 GB)
#define LOG_BUFFER_SIZE (2*268435456ULL)

// also reduce size of logging tx index by reducing maximum number of ticks per epoch
#include "../src/public_settings.h"
#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 3000

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

static void updateAndPrintEntityCategoryPopulations()
{
    updateAndAnalzeEntityCategoryPopulations();

    // Compute number of entities with 0 balance
    unsigned int sumEntityCategoryPopulations = 0;
    for (int i = 0; i < entityCategoryCount; ++i)
        sumEntityCategoryPopulations += entityCategoryPopulations[i];
    EXPECT_GE(spectrumInfo.numberOfEntities, sumEntityCategoryPopulations);

#if PRINT_TEST_INFO
    unsigned int zeroBalanceEntities = spectrumInfo.numberOfEntities - sumEntityCategoryPopulations;
    if (zeroBalanceEntities > 0)
        std::cout << "  - bin -1: " << zeroBalanceEntities << " entities with zero balance\n";

    static constexpr int entityCategoryCount = sizeof(entityCategoryPopulations) / sizeof(entityCategoryPopulations[0]);
    for (int i = 0; i < entityCategoryCount; ++i)
    {
        if (entityCategoryPopulations[i])
        {
            unsigned long long lowerBound = (1llu << i), upperBound = (1llu << (i + 1)) - 1;
            const char* burnIndicator = "  + bin ";
            if (lowerBound <= dustThresholdBurnAll)
                burnIndicator = "  - bin ";
            else if (lowerBound <= dustThresholdBurnHalf)
                burnIndicator = "  * bin ";
            std::cout << burnIndicator << i << ": " << entityCategoryPopulations[i] << " entities with amount ";
            if (i == 0)
                std::cout << lowerBound;
            else
                std::cout << "between " << lowerBound << " and " << upperBound;
            std::cout << std::endl;
        }
    }
#endif
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
        system.tick = 15700000;
        clearSpectrum();
        antiDustCornerCase = false;
        EXPECT_TRUE(logger.initLogging());
    }

    ~SpectrumTest()
    {
        logger.deinitLogging();
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
#if PRINT_TEST_INFO
        std::cout << "Entity balance distribution before anti-dust:" << std::endl;
#endif
        updateAndPrintEntityCategoryPopulations();

        // Start measuring run-time
        beforeAntiDustTimestamp = std::chrono::high_resolution_clock::now();
    }

    void afterAntiDust()
    {
        checkAndGetInfo();

        // Print anti-dust info
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - beforeAntiDustTimestamp);
        std::cout << "Transfer with anti-dust took " << duration_ms << " ms: entities "
            << beforeAntiDustSpectrumInfo.numberOfEntities << " -> " << spectrumInfo.numberOfEntities
            << " (to " << spectrumInfo.numberOfEntities * 100llu / SPECTRUM_CAPACITY
            << "% of capacity);  total amount " << beforeAntiDustSpectrumInfo.totalAmount << " -> " << spectrumInfo.totalAmount
            << " (" << ((long long)spectrumInfo.totalAmount - (long long)beforeAntiDustSpectrumInfo.totalAmount) * 100ll / beforeAntiDustSpectrumInfo.totalAmount << "% reduction)" << std::endl;

        // Print distribution of entity balances
#if PRINT_TEST_INFO
        std::cout << "Entity balance distribution after anti-dust:" << std::endl;
#endif
        updateAndPrintEntityCategoryPopulations();

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
        std::cout << "Spectrum file state before dust attack:" << std::endl;
        updateAndPrintEntityCategoryPopulations();

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
    }

    test.beforeAntiDust();
    increaseEnergy(m256i::randomValue(), 100llu);
    test.afterAntiDust();
}

SpectrumStats* getSpectrumStatsLog(long long id)
{
    qLogger::BlobInfo bi = logger.logBuf.getBlobInfo(id);
    EXPECT_EQ(bi.length, LOG_HEADER_SIZE + sizeof(SpectrumStats));
    return reinterpret_cast<SpectrumStats*>(logger.logBuffer + bi.startIndex + LOG_HEADER_SIZE);
}

DustBurning* getDustBurningLog(long long id)
{
    qLogger::BlobInfo bi = logger.logBuf.getBlobInfo(id);
    DustBurning* db = reinterpret_cast<DustBurning*>(logger.logBuffer + bi.startIndex + LOG_HEADER_SIZE);
    EXPECT_EQ(bi.length, LOG_HEADER_SIZE + db->messageSize());
    return db;
}

TEST(TestCoreSpectrum, AntiDustEdgeCaseHugeBinsAndLogging)
{
    SpectrumTest test;
    test.antiDustCornerCase = true;

    // build-up spectrum
    for (unsigned long long i = 0; i < (SPECTRUM_CAPACITY / 2 + SPECTRUM_CAPACITY / 4); ++i)
    {
        unsigned long long amount;
        if (i < SPECTRUM_CAPACITY / 4)
            amount = 100;
        else if (i < SPECTRUM_CAPACITY / 2 + SPECTRUM_CAPACITY / 4)
            amount = 10000;
        increaseEnergy(m256i(i, 1, 2, 3), amount);
    }

    // test anti-dust
    test.beforeAntiDust();
    increaseEnergy(m256i(SPECTRUM_CAPACITY - 1, 1, 2, 3), 1000llu);
    test.afterAntiDust();

    // check logs:
    // first 24 are from building up spectrum
    SpectrumStats* stats;
    for (int i = 0; i < 24; ++i)
    {
        stats = getSpectrumStatsLog(i);
        EXPECT_EQ(stats->numberOfEntities, i * 524288 + 1);
        EXPECT_EQ(stats->entityCategoryPopulations[6], std::min(i * 524288 + 1, int(SPECTRUM_CAPACITY / 4)));
        EXPECT_EQ(stats->entityCategoryPopulations[13], (i < 8) ? 0 : (i - 8) * 524288 + 1);
        EXPECT_EQ(stats->totalAmount, stats->entityCategoryPopulations[6] * 100llu + stats->entityCategoryPopulations[13] * 10000llu);

        if (i < 16)
        {
            EXPECT_EQ(stats->dustThresholdBurnAll, 0);
            EXPECT_EQ(stats->dustThresholdBurnHalf, 0);
        }
        else
        {
            EXPECT_EQ(stats->dustThresholdBurnAll, (2 << 6) - 1);
            EXPECT_EQ(stats->dustThresholdBurnHalf, 0);
        }
    }

    // Check state before anti-dust
    SpectrumStats* beforeAntidustStats = getSpectrumStatsLog(24);
    EXPECT_EQ(beforeAntidustStats->numberOfEntities, 24 * 524288);
    EXPECT_EQ(beforeAntidustStats->entityCategoryPopulations[6], SPECTRUM_CAPACITY / 4);
    EXPECT_EQ(beforeAntidustStats->entityCategoryPopulations[13], SPECTRUM_CAPACITY / 2);
    EXPECT_EQ(beforeAntidustStats->totalAmount, beforeAntidustStats->entityCategoryPopulations[6] * 100llu + beforeAntidustStats->entityCategoryPopulations[13] * 10000llu);
    EXPECT_EQ(beforeAntidustStats->dustThresholdBurnAll, (2 << 12) - 1);
    EXPECT_EQ(beforeAntidustStats->dustThresholdBurnHalf, (2 << 13) - 1);

    // Check dust burning log messages
    int balancesBurned = 0;
    int logId = 25;
    while (balancesBurned < 8 * 1048576)
    {
        DustBurning* db = getDustBurningLog(logId);
        for (int i = 0; i < db->numberOfBurns; ++i)
        {
            // Of the first 4M entities, all are burned (amount 100), of the following every second is burned.
            unsigned long long expectedSpectrumIndex = balancesBurned;
            if (balancesBurned >= 4194304)
                expectedSpectrumIndex = (balancesBurned - 4194304) * 2 + 4194304;

            DustBurning::Entity& e = db->entity(i);
            EXPECT_EQ(e.publicKey, m256i(expectedSpectrumIndex, 1, 2, 3));
            EXPECT_EQ(e.amount, (balancesBurned < 4194304) ? 100 : 10000);
            ++balancesBurned;
        }
        ++logId;
    }

    // Finally, check state logged after dust burning (logged before increaing energy / adding new entity)
    SpectrumStats* afterAntidustStats = getSpectrumStatsLog(logId);
    EXPECT_EQ(afterAntidustStats->numberOfEntities, 4194304);
    EXPECT_EQ(afterAntidustStats->entityCategoryPopulations[9], 0);
    EXPECT_EQ(afterAntidustStats->entityCategoryPopulations[13], 4 * 1048576);
    EXPECT_EQ(afterAntidustStats->totalAmount, afterAntidustStats->entityCategoryPopulations[13] * 10000llu);
    EXPECT_EQ(afterAntidustStats->dustThresholdBurnAll, 0);
    EXPECT_EQ(afterAntidustStats->dustThresholdBurnHalf, 0);
}

TEST(TestCoreSpectrum, AntiDustEdgeCaseHugeBinZeroBalance)
{
    SpectrumTest test;
    m256i richId(123, 4, 5, 6);
    unsigned long long amount = 1000;
    increaseEnergy(richId, 100 * amount);
    unsigned int spectrum75pct = (SPECTRUM_CAPACITY / 2 + SPECTRUM_CAPACITY / 4);
    for (unsigned long long i = 0; i < spectrum75pct - 1; ++i)
    {
        m256i id(i, 1, 2, 3);
        increaseEnergy(id, amount);
        decreaseEnergy(spectrumIndex(id), amount);
    }
    test.beforeAntiDust();
    transfer(richId, m256i(1234, 4, 5, 6), 100 * amount);
    test.afterAntiDust();
}

