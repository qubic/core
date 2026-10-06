#define NO_UEFI

#include "contract_testing.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#endif

// Compile the untouched baseline with the same QPI types and compiler as the patch.
namespace LegacyQswap
{
#define CONTRACT_INDEX QSWAP_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QSWAP
#define CONTRACT_STATE2_TYPE QSWAP2
// contract_def.h undefines the non-locals system macros after compiling contracts.
#define INITIALIZE() NO_IO_SYSTEM_PROC(INITIALIZE, __initialize, NoData, NoData)
#define PRE_ACQUIRE_SHARES() \
    NO_IO_SYSTEM_PROC(PRE_ACQUIRE_SHARES, __preAcquireShares, PreManagementRightsTransfer_input, PreManagementRightsTransfer_output)
#include "data/qswap_legacy.h"
#undef INITIALIZE
#undef PRE_ACQUIRE_SHARES
#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE
}

static_assert(sizeof(QSWAP::StateData) == sizeof(LegacyQswap::QSWAP::StateData) + sizeof(id));
static_assert(offsetof(QSWAP::StateData, liquidityKeyFormat) == sizeof(LegacyQswap::QSWAP::StateData));
static_assert(sizeof(QSWAP::LiquidityInfo) == sizeof(LegacyQswap::QSWAP::LiquidityInfo));

#define CHECK_LEGACY_OFFSET(field) \
    static_assert(offsetof(QSWAP::StateData, field) == offsetof(LegacyQswap::QSWAP::StateData, field))
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
    EXPECT_EQ(sizeof(QSWAP::StateData), sizeof(LegacyQswap::QSWAP::StateData) + 32);
    EXPECT_EQ(contractDescriptions[QSWAP_CONTRACT_INDEX].stateSize, sizeof(QSWAP::StateData));
    printf("QSWAP state bytes: legacy=%zu patched=%zu marker_offset=%zu\n",
        sizeof(LegacyQswap::QSWAP::StateData), sizeof(QSWAP::StateData),
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
    file.seekp(static_cast<std::streamoff>(sizeof(LegacyQswap::QSWAP::StateData) - 1));
    file.put(0);
    file.close();
    ASSERT_TRUE(file);
    ASSERT_EQ(std::filesystem::file_size(path), sizeof(LegacyQswap::QSWAP::StateData));

    std::vector<unsigned char> buffer(sizeof(QSWAP::StateData));
    // This Linux fixture's temporary filename is ASCII. Encode it for the node's CHAR16 ABI.
    std::vector<CHAR16> filename;
    for (const char character : path.string())
        filename.push_back(static_cast<CHAR16>(character));
    filename.push_back(0);
    EXPECT_EQ(load(filename.data(), sizeof(LegacyQswap::QSWAP::StateData), buffer.data()),
        sizeof(LegacyQswap::QSWAP::StateData));
    EXPECT_EQ(load(filename.data(), sizeof(QSWAP::StateData), buffer.data()), -1);
    EXPECT_TRUE(std::filesystem::remove(path));
}
#endif
