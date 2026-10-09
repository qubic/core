#define NO_UEFI

#include "contract_testing.h"
#include <cstddef>

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

static_assert(sizeof(QSWAP::StateData) == sizeof(LegacyQswap::StateData));
static_assert(alignof(QSWAP::StateData) == alignof(LegacyQswap::StateData));
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

TEST(ContractSwap, StateLayoutMatchesOriginal)
{
    EXPECT_EQ(sizeof(QSWAP::StateData), sizeof(LegacyQswap::StateData));
    EXPECT_EQ(alignof(QSWAP::StateData), alignof(LegacyQswap::StateData));
    EXPECT_EQ(contractDescriptions[QSWAP_CONTRACT_INDEX].stateSize, sizeof(LegacyQswap::StateData));
}
