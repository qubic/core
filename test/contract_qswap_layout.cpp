#define NO_UEFI

#include "contract_testing.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#endif

// Frozen state layout from Core develop 33561bd7, before the LP-key conversion.
// Keep these definitions independent of the patched QSWAP types and constants.
namespace LegacyQswap
{
    using namespace QPI;

    constexpr uint64 maxPools = 8192 * X_MULTIPLIER;
    constexpr uint64 maxUsersPerPool = 256;

    struct PoolBasicState
    {
        id poolID;
        sint64 reservedQuAmount;
        sint64 reservedAssetAmount;
        sint64 totalLiquidity;
        uint128 accFeePerLPX64;
    };

    struct LiquidityInfo
    {
        sint64 liquidity;
        uint128 feeDebtX64;
        uint64 accumulatedFee;
    };

    struct StateData
    {
        uint32 swapFeeRate;
        uint32 investRewardsFeeRate;
        uint32 shareholderFeeRate;
        uint32 poolCreationFeeRate;

        id investRewardsId;
        uint64 investRewardsEarnedFee;
        uint64 investRewardsDistributedAmount;
        uint64 shareholderEarnedFee;
        uint64 shareholderDistributedAmount;

        Array<PoolBasicState, maxPools> mPoolBasicStates;
        Collection<LiquidityInfo, maxPools * maxUsersPerPool> mLiquidities;

        uint32 qxFeeRate;
        uint32 burnFeeRate;
        uint64 qxEarnedFee;
        uint64 qxDistributedAmount;
        uint64 burnEarnedFee;
        uint64 burnedAmount;
        uint32 cachedIssuanceFee;
        uint32 cachedTransferFee;
    };
}

static_assert(sizeof(QSWAP::StateData) == sizeof(LegacyQswap::StateData) + sizeof(id));
static_assert(offsetof(QSWAP::StateData, liquidityKeyFormat) == sizeof(LegacyQswap::StateData));
static_assert(sizeof(QSWAP::LiquidityInfo) == sizeof(LegacyQswap::LiquidityInfo));

#define CHECK_LEGACY_OFFSET(field) \
    static_assert(offsetof(QSWAP::StateData, field) == offsetof(LegacyQswap::StateData, field))
CHECK_LEGACY_OFFSET(swapFeeRate);
CHECK_LEGACY_OFFSET(investRewardsFeeRate);
CHECK_LEGACY_OFFSET(shareholderFeeRate);
CHECK_LEGACY_OFFSET(poolCreationFeeRate);
CHECK_LEGACY_OFFSET(investRewardsId);
CHECK_LEGACY_OFFSET(investRewardsEarnedFee);
CHECK_LEGACY_OFFSET(investRewardsDistributedAmount);
CHECK_LEGACY_OFFSET(shareholderEarnedFee);
CHECK_LEGACY_OFFSET(shareholderDistributedAmount);
CHECK_LEGACY_OFFSET(mPoolBasicStates);
CHECK_LEGACY_OFFSET(mLiquidities);
CHECK_LEGACY_OFFSET(qxFeeRate);
CHECK_LEGACY_OFFSET(burnFeeRate);
CHECK_LEGACY_OFFSET(qxEarnedFee);
CHECK_LEGACY_OFFSET(qxDistributedAmount);
CHECK_LEGACY_OFFSET(burnEarnedFee);
CHECK_LEGACY_OFFSET(burnedAmount);
CHECK_LEGACY_OFFSET(cachedIssuanceFee);
CHECK_LEGACY_OFFSET(cachedTransferFee);
#undef CHECK_LEGACY_OFFSET

TEST(ContractSwap, LiquidityKeyFormatIncreasesStateSize)
{
    EXPECT_EQ(sizeof(QSWAP::StateData), sizeof(LegacyQswap::StateData) + 32);
    EXPECT_EQ(contractDescriptions[QSWAP_CONTRACT_INDEX].stateSize, sizeof(QSWAP::StateData));
    printf("QSWAP state bytes: legacy=%zu patched=%zu marker_offset=%zu\n",
        sizeof(LegacyQswap::StateData), sizeof(QSWAP::StateData),
        offsetof(QSWAP::StateData, liquidityKeyFormat));
}

TEST(ContractSwap, QswapUpgradeRequiresOfflineConversion)
{
    // Check the release's configured epoch. Historical entries for other epochs remain valid.
    for (unsigned int i = 0; i < contractStateChangeCount; ++i)
    {
        const auto& change = contractStateChangeInfos[i];
        EXPECT_FALSE(change.contractIndex == QSWAP_CONTRACT_INDEX && change.changeEpoch == EPOCH)
            << "QSWAP requires offline conversion at epoch " << EPOCH
            << "; do not configure PADDING, RESET or built-in MIGRATE (entry " << i << ").";
    }
}

// Core's existing Windows short-read path retains the file handle, preventing fixture cleanup.
// Run this proof in the Linux harness; Windows still runs the layout and release checks.
#ifndef _WIN32
TEST(ContractSwap, UnconvertedLegacySizeFileFailsPatchedLoad)
{
    const auto path = std::filesystem::temp_directory_path()
        / ("qswap-legacy-state-" + std::to_string(getpid()) + ".bin");
    // A sparse file exercises the real loader's size handling without needing live state.
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(file.is_open());
    file.seekp(static_cast<std::streamoff>(sizeof(LegacyQswap::StateData) - 1));
    file.put(0);
    file.close();
    ASSERT_TRUE(file);
    ASSERT_EQ(std::filesystem::file_size(path), sizeof(LegacyQswap::StateData));

    std::vector<unsigned char> buffer(sizeof(QSWAP::StateData));
    // This Linux fixture's temporary filename is ASCII. Encode it for the node's CHAR16 ABI.
    std::vector<CHAR16> filename;
    for (const char character : path.string())
        filename.push_back(static_cast<CHAR16>(character));
    filename.push_back(0);
    EXPECT_EQ(load(filename.data(), sizeof(LegacyQswap::StateData), buffer.data()),
        sizeof(LegacyQswap::StateData));
    EXPECT_EQ(load(filename.data(), sizeof(QSWAP::StateData), buffer.data()), -1);
    EXPECT_TRUE(std::filesystem::remove(path));
}
#endif
