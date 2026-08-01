using namespace QPI;

constexpr uint64 QTREAT_MAX_HOLDERS      = 131072;
constexpr uint64 QTREAT_MAX_STAKERS      = 65536;
constexpr uint64 QTREAT_MAX_ASSETS       = 1024;
constexpr uint64 QTREAT_MIN_ELIGIBLE_BAL = 1;

constexpr uint64 QTREAT_TOKEN_ASSETNAME  = 92639312630865ULL; // "QTREAT"
constexpr uint64 QTREAT_QDOGE_ASSETNAME   = 297549120593ULL; // "QDOGE"

constexpr uint64 QTREAT_MIN_STAKE               = 10000000ULL;
constexpr uint64 QTREAT_STAKER_REWARD_PER_EPOCH = 20000000ULL;
constexpr uint64 QTREAT_PHASE_EPOCHS            = 52;
constexpr uint64 QTREAT_NUM_PHASES              = 1;
constexpr uint64 QTREAT_TOTAL_REWARD_EPOCHS     = QTREAT_PHASE_EPOCHS * QTREAT_NUM_PHASES;
constexpr uint64 QTREAT_UNSTAKE_DELAY_EPOCHS    = 2;

constexpr uint64 QTREAT_PROGRESSIVE_MIN_STEP       = 1000000ULL;
constexpr uint64 QTREAT_PROGRESSIVE_BONUS_PERMILLE = 25;
constexpr uint64 QTREAT_PROGRESSIVE_MAX_STREAK     = 20;
constexpr uint64 QTREAT_PROGRESSIVE_START_DELAY_EPOCHS = 4;

constexpr uint64 QTREAT_BONUS_THRESHOLD       = 50000000ULL;
constexpr uint64 QTREAT_BONUS_INTERVAL_EPOCHS = 12;
constexpr uint64 QTREAT_BONUS_MAX_PER_WALLET  = 4;

constexpr uint64 QTREAT_RAFFLE_EPOCHS         = 52;
constexpr uint16 QTREAT_RAFFLE_ENTROPY_BITS   = 256;
constexpr uint8  QTREAT_RAFFLE_COLLATERAL_TIER = 0;
constexpr uint64 QTREAT_RAFFLE_ENTROPY_FEE    = RANDOM_BITFEE * QTREAT_RAFFLE_ENTROPY_BITS;

constexpr uint64 QTREAT_ASIC_CATALOG_CAPACITY = 512;
constexpr uint64 QTREAT_ASIC_PARTS_TOTAL      = 400;
constexpr uint64 QTREAT_MAX_ASIC_RIGS         = 128;
constexpr uint64 QTREAT_MAX_TOTAL_ASICS       = 100;
// Ownership re-verification / possessor snapshotting is spread across this many epochs
// (round-robin by slot index) instead of touching every entry every epoch, so a large
// registered-rig or dividend-NFT set can't force hundreds of external QBAY calls into a
// single END_EPOCH. Each entry is still re-checked at least once every N epochs.
constexpr uint64 QTREAT_ASIC_VERIFY_SPREAD_EPOCHS = 8;
constexpr uint64 QTREAT_NFT_SNAPSHOT_SPREAD_EPOCHS = 4;
constexpr uint64 QTREAT_MINING_REWARD_DEFAULT = 20000000ULL;
constexpr uint64 QTREAT_MINING_REWARD_MAX     = 100000000ULL;
static_assert(QTREAT_MINING_REWARD_DEFAULT <= QTREAT_MINING_REWARD_MAX);
constexpr uint64 QTREAT_RARITY_POINTS_COMMON    = 1;
constexpr uint64 QTREAT_RARITY_POINTS_UNCOMMON  = 2;
constexpr uint64 QTREAT_RARITY_POINTS_RARE      = 3;
constexpr uint64 QTREAT_RARITY_POINTS_EPIC      = 5;
constexpr uint64 QTREAT_RARITY_POINTS_LEGENDARY = 8;
constexpr uint64 QTREAT_MAX_RARITY = 4;
constexpr uint64 QTREAT_RARITY_SUPPLY_COMMON    = 40;
constexpr uint64 QTREAT_RARITY_SUPPLY_UNCOMMON  = 30;
constexpr uint64 QTREAT_RARITY_SUPPLY_RARE      = 18;
constexpr uint64 QTREAT_RARITY_SUPPLY_EPIC      = 10;
constexpr uint64 QTREAT_RARITY_SUPPLY_LEGENDARY = 2;
static_assert(QTREAT_RARITY_SUPPLY_COMMON + QTREAT_RARITY_SUPPLY_UNCOMMON + QTREAT_RARITY_SUPPLY_RARE
            + QTREAT_RARITY_SUPPLY_EPIC + QTREAT_RARITY_SUPPLY_LEGENDARY == 100);
static_assert(QTREAT_MAX_TOTAL_ASICS == 100);

constexpr uint64 QTREAT_DRIP_EPOCHS = 52;
constexpr uint64 QTREAT_DRIP_QDOGE_COMMON    = 10000;
constexpr uint64 QTREAT_DRIP_QDOGE_UNCOMMON  = 20000;
constexpr uint64 QTREAT_DRIP_QDOGE_RARE      = 30000;
constexpr uint64 QTREAT_DRIP_QDOGE_EPIC      = 50000;
constexpr uint64 QTREAT_DRIP_QDOGE_LEGENDARY = 100000;

constexpr uint64 QTREAT_DIVIDEND_SHAREHOLDER_PERMILLE = 50;
static_assert(QTREAT_DIVIDEND_SHAREHOLDER_PERMILLE < 1000);

constexpr uint64 QTREAT_MAX_NFT_HOLDERS = 1024;
constexpr uint64 QTREAT_MAX_DIVIDEND_NFTS = 256;

constexpr uint64 QTREAT_MAX_EXCLUDE_ADDRESSES = 4;

constexpr uint16 QTREAT_QX_CONTRACT_INDEX = 1;
constexpr sint64 QTREAT_QX_TRANSFER_FEE   = 100LL;

constexpr uint32 QTREAT_OK = 0;
constexpr uint32 QTREAT_ERR_ACCESS_DENIED = 1;
constexpr uint32 QTREAT_ERR_ZERO_AMOUNT = 2;
constexpr uint32 QTREAT_ERR_INSUFFICIENT_STAKE = 3;
constexpr uint32 QTREAT_ERR_UNSTAKE_PENDING = 4;
constexpr uint32 QTREAT_ERR_UNSTAKE_NOT_READY = 5;
constexpr uint32 QTREAT_ERR_NOT_STAKER = 6;
constexpr uint32 QTREAT_ERR_BELOW_MIN_STAKE = 7;
constexpr uint32 QTREAT_ERR_ACQUIRE_FAILED = 8;
constexpr uint32 QTREAT_ERR_TRANSFER_FAILED = 9;
constexpr uint32 QTREAT_ERR_NO_PENDING_BONUS = 10;
constexpr uint32 QTREAT_ERR_INSUFFICIENT_FEE = 11;
constexpr uint32 QTREAT_ERR_INVALID_INPUT = 12;
constexpr uint32 QTREAT_ERR_PHASES_ENDED = 13;
constexpr uint32 QTREAT_ERR_BONUS_POOL_EMPTY = 14;
constexpr uint32 QTREAT_ERR_EXTERNAL_CALL = 15;

struct QTREAT2 {};

struct QTREAT : public ContractBase
{
    struct StakerInfo
    {
        uint64 staked;
        uint64 unstakeAmount;
        uint64 unstakeEpoch;
        uint64 bonusEpochs;
        uint64 hwmHoldings;
        uint64 lastStaked;
        uint64 growthStreak;
        uint64 bonusAwarded;
        uint64 pendingBonus;
    };

    struct RaffleSeed
    {
        bit_4096 entropy;
        id prevSpectrumDigest;
        id prevUniverseDigest;
        id prevComputerDigest;
        uint64 epoch;
        uint64 tick;
        uint64 eligibleCount;
    };

    struct AsicRig
    {
        id owner;
        uint32 partMotherboard;
        uint32 partChip;
        uint32 partPsu;
        uint32 partFan;
        uint64 weight;
        uint64 active;
    };

    struct AssetKey
    {
        id issuer;
        uint64 assetName;
        bool operator==(const AssetKey& o) const { return issuer == o.issuer && assetName == o.assetName; }
        bool operator!=(const AssetKey& o) const { return issuer != o.issuer || assetName != o.assetName; }
    };

    struct StateData
    {
        id adminAddress;

        Asset qtreatToken;
        Asset qdogeToken;

        uint64 dividendFund;
        uint64 totalDividendsDistributed;

        uint64 stakingFund;
        uint64 totalStakingRewardsDistributed;

        HashMap<id, uint64, QTREAT_MAX_HOLDERS> beginBalances;
        HashMap<id, uint64, QTREAT_MAX_HOLDERS> endBalances;
        uint64 totalHoldersSnapshot;

        HashMap<id, StakerInfo, QTREAT_MAX_STAKERS> stakers;
        uint64 totalStaked;
        uint64 stakingStartEpoch;

        uint64 qtreatBonusPool;

        Array<id, QTREAT_MAX_EXCLUDE_ADDRESSES> excludeAddresses;

        Array<uint32, QTREAT_MAX_DIVIDEND_NFTS> dividendNftIds;
        uint64 dividendNftIdCount;
        // Last possessor observed for each dividendNftIds slot (NULL_ID = none/excluded), so the
        // round-robin snapshot below can update nftCounts incrementally instead of resetting and
        // re-querying all QTREAT_MAX_DIVIDEND_NFTS ids every single epoch.
        Array<id, QTREAT_MAX_DIVIDEND_NFTS> dividendNftLastPossessor;
        HashMap<id, uint64, QTREAT_MAX_NFT_HOLDERS> nftCounts;
        uint64 totalNftCount;

        uint64 totalShareholderDividends;

        HashMap<uint64, uint64, QTREAT_ASIC_CATALOG_CAPACITY> asicCatalog;
        Array<uint64, 32> asicCatalogBuckets;
        uint64 asicCatalogSize;
        uint64 asicCatalogLocked;
        HashMap<uint64, uint64, QTREAT_ASIC_CATALOG_CAPACITY> asicUsedParts;
        Array<AsicRig, QTREAT_MAX_ASIC_RIGS> asicRigs;
        uint64 asicRigHighWater;
        uint64 totalAsicCount;
        uint64 totalMiningWeight;
        uint64 miningFund;
        uint64 miningRewardRate;
        uint64 totalMiningRewardsDistributed;

        uint64 dripQdogePool;
        uint64 dripStartEpoch;
        uint64 totalDripQdogeDistributed;
        HashMap<id, uint64, QTREAT_ASIC_CATALOG_CAPACITY> dripTally;

        uint64 totalBonusDelivered;

        uint64 totalRaffleAwarded;
        id lastRaffleWinner;
        uint64 lastRaffleEpoch;

        HashMap<AssetKey, uint64, QTREAT_MAX_ASSETS> generalAssetBalances;
        HashMap<id, uint64, QTREAT_MAX_ASSETS> scDividendTracker;
    };

    struct DepositDividends_input {}; struct DepositDividends_output { uint32 returnCode; };
    struct DepositStakingFund_input {}; struct DepositStakingFund_output { uint32 returnCode; };

    struct RequestUnstake_input { uint64 amount; }; struct RequestUnstake_output { uint32 returnCode; };
    struct RequestUnstake_locals { StakerInfo info; };
    struct FinalizeUnstake_input {}; struct FinalizeUnstake_output { uint32 returnCode; };
    struct FinalizeUnstake_locals { StakerInfo info; sint64 rel; };

    struct ClaimQtreatBonus_input {}; struct ClaimQtreatBonus_output { uint32 returnCode; uint64 claimed; };
    struct ClaimQtreatBonus_locals { StakerInfo info; sint64 xfer; sint64 rel; };

    struct DepositQtreatTokens_input { uint64 amount; }; struct DepositQtreatTokens_output { uint32 returnCode; };
    struct DepositQtreatTokens_locals { sint64 xfer; };

    struct DepositGeneralAsset_input { Asset asset; uint64 amount; };
    struct DepositGeneralAsset_output { uint32 returnCode; };
    struct DepositGeneralAsset_locals { sint64 managed; sint64 xfer; AssetKey key; uint64 bal; };

    struct RevokeGeneralAsset_input { Asset asset; uint64 amount; };
    struct RevokeGeneralAsset_output { uint32 returnCode; };
    struct RevokeGeneralAsset_locals { AssetKey key; uint64 bal; sint64 xfer; sint64 rel; };

    struct ReleaseManagedShares_input { Asset asset; uint64 amount; };
    struct ReleaseManagedShares_output { uint32 returnCode; };
    struct ReleaseManagedShares_locals { sint64 managed; sint64 rel; StakerInfo info; uint64 accounted; };

    struct SetExcludeAddress_input { uint64 slot; id address; };
    struct SetExcludeAddress_output { uint32 returnCode; };

    struct DepositMiningFund_input {}; struct DepositMiningFund_output { uint32 returnCode; };

    struct SetMiningRate_input { uint64 ratePerEpoch; };
    struct SetMiningRate_output { uint32 returnCode; };

    struct DepositDripQdoge_input { uint64 amount; };
    struct DepositDripQdoge_output { uint32 returnCode; };
    struct DepositDripQdoge_locals { sint64 xfer; };

    struct LoadAsicPart_input { uint32 nftId; uint8 category; uint8 rarity; };
    struct LoadAsicPart_output { uint32 returnCode; uint64 catalogSize; uint64 locked; };
    struct LoadAsicPart_locals { uint64 budget; uint64 dummy; uint64 partPoints; };

    struct RegisterAsic_input { uint32 partMotherboard; uint32 partChip; uint32 partPsu; uint32 partFan; };
    struct RegisterAsic_output { uint32 returnCode; uint64 rigIndex; uint64 asicWeight; };
    struct RegisterAsic_locals
    {
        AsicRig rig; uint64 points; uint64 partPoints; uint64 packed; uint64 r; sint64 i; uint32 partId; uint64 dummy;
        QBAY::getInfoOfNFTById_input qbayIn;
        QBAY::getInfoOfNFTById_output qbayOut;
        sint64 freeSlot;
    };

    struct UnregisterAsic_input { uint64 rigIndex; };
    struct UnregisterAsic_output { uint32 returnCode; };
    struct UnregisterAsic_locals { AsicRig rig; };

    struct GetMinerInfo_input { id wallet; };
    struct GetMinerInfo_output { uint64 asicCount; uint64 weight; uint64 totalAsicCount; uint64 totalMiningWeight; uint64 miningFund; };
    struct GetMinerInfo_locals { AsicRig rig; sint64 i; };

    struct GetAsicCatalogInfo_input { uint32 nftId; };
    struct GetAsicCatalogInfo_output { uint64 catalogSize; uint64 locked; uint64 isCataloged; uint64 category; uint64 rarity; uint64 usedByRigPlusOne; };
    struct GetAsicCatalogInfo_locals { uint64 packed; uint64 used; };

    struct GetNftInfo_input { id wallet; };
    struct GetNftInfo_output { uint64 nftCount; uint64 totalNftCount; };
    struct GetNftInfo_locals { uint64 val; };

    struct GetExcludeAddresses_input {};
    struct GetExcludeAddresses_output { Array<id, QTREAT_MAX_EXCLUDE_ADDRESSES> addresses; };
    struct GetExcludeAddresses_locals { sint64 i; };

    struct GetStakingInfo_input { id staker; };
    struct GetStakingInfo_output
    {
        uint64 staked; uint64 unstakeAmount; uint64 unstakeEpoch;
        uint64 bonusEpochs; uint64 bonusAwarded; uint64 pendingBonus;
        uint64 growthStreak; uint64 hwmHoldings;
        uint64 totalStaked; uint64 stakingFund; uint32 isStaker;
    };
    struct GetStakingInfo_locals { StakerInfo info; };

    struct GetPhaseInfo_input {};
    struct GetPhaseInfo_output
    {
        uint64 stakingStartEpoch;
        uint64 currentPhase;
        uint64 epochsRemaining;
    };
    struct GetPhaseInfo_locals { uint64 elapsed; };

    struct GetRaffleInfo_input {};
    struct GetRaffleInfo_output
    {
        uint64 totalRaffleAwarded;
        id lastRaffleWinner;
        uint64 lastRaffleEpoch;
        uint64 raffleEpochsRemaining;
    };
    struct GetRaffleInfo_locals { uint64 elapsed; };

    struct GetFunds_input {};
    struct GetFunds_output
    {
        uint64 dividendFund; uint64 stakingFund; uint64 qtreatBonusPool;
        uint64 totalDividendsDistributed; uint64 totalStakingRewardsDistributed;
        uint64 totalBonusDelivered;
        uint64 totalShareholderDividends;
        uint64 miningFund;
        uint64 miningRewardRate;
        uint64 totalMiningRewardsDistributed;
        uint64 dripQdogePool;
        uint64 dripStartEpoch;
        uint64 totalDripQdogeDistributed;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetStakingInfo)
    {
        output.isStaker = state.get().stakers.get(input.staker, locals.info) ? 1 : 0;
        if (output.isStaker)
        {
            output.staked = locals.info.staked;
            output.unstakeAmount = locals.info.unstakeAmount;
            output.unstakeEpoch = locals.info.unstakeEpoch;
            output.bonusEpochs = locals.info.bonusEpochs;
            output.bonusAwarded = locals.info.bonusAwarded;
            output.pendingBonus = locals.info.pendingBonus;
            output.growthStreak = locals.info.growthStreak;
            output.hwmHoldings = locals.info.hwmHoldings;
        }
        output.totalStaked = state.get().totalStaked;
        output.stakingFund = state.get().stakingFund;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetPhaseInfo)
    {
        output.stakingStartEpoch = state.get().stakingStartEpoch;
        output.currentPhase = 0;
        output.epochsRemaining = 0;
        if (state.get().stakingStartEpoch == 0) return;
        locals.elapsed = qpi.epoch() - state.get().stakingStartEpoch;
        if (locals.elapsed < QTREAT_TOTAL_REWARD_EPOCHS)
        {
            output.currentPhase = div(locals.elapsed, QTREAT_PHASE_EPOCHS) + 1;
            output.epochsRemaining = QTREAT_TOTAL_REWARD_EPOCHS - locals.elapsed;
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetRaffleInfo)
    {
        output.totalRaffleAwarded = state.get().totalRaffleAwarded;
        output.lastRaffleWinner = state.get().lastRaffleWinner;
        output.lastRaffleEpoch = state.get().lastRaffleEpoch;
        output.raffleEpochsRemaining = 0;
        if (state.get().stakingStartEpoch != 0)
        {
            locals.elapsed = qpi.epoch() - state.get().stakingStartEpoch;
            if (locals.elapsed < QTREAT_RAFFLE_EPOCHS)
            {
                output.raffleEpochsRemaining = QTREAT_RAFFLE_EPOCHS - locals.elapsed;
            }
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetMinerInfo)
    {
        for (locals.i = 0; locals.i < (sint64)state.get().asicRigHighWater; locals.i++)
        {
            locals.rig = state.get().asicRigs.get(locals.i);
            if (locals.rig.active == 0 || locals.rig.owner != input.wallet) continue;
            output.asicCount = output.asicCount + 1;
            output.weight = output.weight + locals.rig.weight;
        }
        output.totalAsicCount = state.get().totalAsicCount;
        output.totalMiningWeight = state.get().totalMiningWeight;
        output.miningFund = state.get().miningFund;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetAsicCatalogInfo)
    {
        output.catalogSize = state.get().asicCatalogSize;
        output.locked = state.get().asicCatalogLocked;
        locals.packed = 0;
        output.isCataloged = state.get().asicCatalog.get((uint64)input.nftId, locals.packed) ? 1 : 0;
        if (output.isCataloged)
        {
            output.category = div(locals.packed, 8ULL);
            output.rarity = mod(locals.packed, 8ULL);
        }
        locals.used = 0;
        state.get().asicUsedParts.get((uint64)input.nftId, locals.used);
        output.usedByRigPlusOne = locals.used;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetNftInfo)
    {
        locals.val = 0;
        state.get().nftCounts.get(input.wallet, locals.val);
        output.nftCount = locals.val;
        output.totalNftCount = state.get().totalNftCount;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetExcludeAddresses)
    {
        for (locals.i = 0; locals.i < (sint64)QTREAT_MAX_EXCLUDE_ADDRESSES; locals.i++)
        {
            output.addresses.set(locals.i, state.get().excludeAddresses.get(locals.i));
        }
    }

    PUBLIC_FUNCTION(GetFunds)
    {
        output.dividendFund = state.get().dividendFund;
        output.stakingFund = state.get().stakingFund;
        output.qtreatBonusPool = state.get().qtreatBonusPool;
        output.totalDividendsDistributed = state.get().totalDividendsDistributed;
        output.totalStakingRewardsDistributed = state.get().totalStakingRewardsDistributed;
        output.totalBonusDelivered = state.get().totalBonusDelivered;
        output.totalShareholderDividends = state.get().totalShareholderDividends;
        output.miningFund = state.get().miningFund;
        output.miningRewardRate = state.get().miningRewardRate;
        output.totalMiningRewardsDistributed = state.get().totalMiningRewardsDistributed;
        output.dripQdogePool = state.get().dripQdogePool;
        output.dripStartEpoch = state.get().dripStartEpoch;
        output.totalDripQdogeDistributed = state.get().totalDripQdogeDistributed;
    }

    PUBLIC_PROCEDURE(DepositDividends)
    {
        if (qpi.invocationReward() == 0) { output.returnCode = QTREAT_ERR_ZERO_AMOUNT; return; }
        state.mut().dividendFund = sadd(state.get().dividendFund, (uint64)qpi.invocationReward());
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE(DepositStakingFund)
    {
        if (qpi.invocationReward() == 0) { output.returnCode = QTREAT_ERR_ZERO_AMOUNT; return; }
        state.mut().stakingFund = sadd(state.get().stakingFund, (uint64)qpi.invocationReward());
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(RequestUnstake)
    {
        if (!state.get().stakers.get(qpi.invocator(), locals.info) || locals.info.staked == 0)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_NOT_STAKER; return;
        }
        if (input.amount == 0 || input.amount > locals.info.staked)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INSUFFICIENT_STAKE; return;
        }
        if (locals.info.unstakeAmount > 0)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_UNSTAKE_PENDING; return;
        }
        if (input.amount < locals.info.staked && locals.info.staked - input.amount < QTREAT_MIN_STAKE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_BELOW_MIN_STAKE; return;
        }
        if (qpi.invocationReward() < QTREAT_QX_TRANSFER_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INSUFFICIENT_FEE; return;
        }
        if (qpi.invocationReward() > QTREAT_QX_TRANSFER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QTREAT_QX_TRANSFER_FEE);
        }

        locals.info.staked -= input.amount;
        locals.info.unstakeAmount = input.amount;
        locals.info.unstakeEpoch = qpi.epoch();
        if (locals.info.staked == 0)
        {
            locals.info.bonusEpochs = 0;
            locals.info.growthStreak = 0;
            locals.info.lastStaked = 0;
        }
        state.mut().stakers.set(qpi.invocator(), locals.info);
        state.mut().totalStaked = state.get().totalStaked - input.amount;
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(FinalizeUnstake)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (!state.get().stakers.get(qpi.invocator(), locals.info) || locals.info.unstakeAmount == 0)
        {
            output.returnCode = QTREAT_ERR_NOT_STAKER; return;
        }
        if (qpi.epoch() < locals.info.unstakeEpoch + QTREAT_UNSTAKE_DELAY_EPOCHS)
        {
            output.returnCode = QTREAT_ERR_UNSTAKE_NOT_READY; return;
        }

        locals.rel = qpi.releaseShares(state.get().qdogeToken, qpi.invocator(), qpi.invocator(),
            (sint64)locals.info.unstakeAmount, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
            QTREAT_QX_TRANSFER_FEE);
        if (locals.rel < 0) { output.returnCode = QTREAT_ERR_TRANSFER_FAILED; return; }

        locals.info.unstakeAmount = 0;
        locals.info.unstakeEpoch = 0;
        if (locals.info.staked == 0 && locals.info.pendingBonus == 0
            && locals.info.bonusAwarded == 0)
        {
            state.mut().stakers.removeByKey(qpi.invocator());
        }
        else
        {
            state.mut().stakers.set(qpi.invocator(), locals.info);
        }
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(DepositQtreatTokens)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().adminAddress) { output.returnCode = QTREAT_ERR_ACCESS_DENIED; return; }
        if (input.amount == 0) { output.returnCode = QTREAT_ERR_ZERO_AMOUNT; return; }
        locals.xfer = qpi.transferShareOwnershipAndPossession(
            state.get().qtreatToken.assetName, state.get().qtreatToken.issuer,
            qpi.invocator(), qpi.invocator(), (sint64)input.amount, SELF);
        if (locals.xfer < 0) { output.returnCode = QTREAT_ERR_ACQUIRE_FAILED; return; }
        state.mut().qtreatBonusPool = sadd(state.get().qtreatBonusPool, input.amount);
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(ClaimQtreatBonus)
    {
        if (!state.get().stakers.get(qpi.invocator(), locals.info) || locals.info.pendingBonus == 0)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_NO_PENDING_BONUS; return;
        }
        if (state.get().qtreatBonusPool < locals.info.pendingBonus)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_BONUS_POOL_EMPTY; return;
        }

        if (qpi.invocationReward() < QTREAT_QX_TRANSFER_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INSUFFICIENT_FEE;
            return;
        }
        if (qpi.invocationReward() > QTREAT_QX_TRANSFER_FEE)
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QTREAT_QX_TRANSFER_FEE);

        locals.xfer = qpi.transferShareOwnershipAndPossession(
            state.get().qtreatToken.assetName, state.get().qtreatToken.issuer,
            SELF, SELF, (sint64)locals.info.pendingBonus, qpi.invocator());
        if (locals.xfer < 0) { output.returnCode = QTREAT_ERR_TRANSFER_FAILED; return; }

        state.mut().qtreatBonusPool = state.get().qtreatBonusPool - locals.info.pendingBonus;
        output.claimed = locals.info.pendingBonus;
        locals.info.pendingBonus = 0;
        state.mut().stakers.set(qpi.invocator(), locals.info);

        locals.rel = qpi.releaseShares(state.get().qtreatToken, qpi.invocator(), qpi.invocator(),
            (sint64)output.claimed, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
            QTREAT_QX_TRANSFER_FEE);
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(DepositGeneralAsset)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().adminAddress) { output.returnCode = QTREAT_ERR_ACCESS_DENIED; return; }
        if (input.amount == 0 || input.asset.issuer == NULL_ID) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }

        locals.managed = qpi.numberOfShares(input.asset,
            { qpi.invocator(), SELF_INDEX }, { qpi.invocator(), SELF_INDEX });
        if (locals.managed < (sint64)input.amount) { output.returnCode = QTREAT_ERR_ACQUIRE_FAILED; return; }

        locals.key.issuer = input.asset.issuer;
        locals.key.assetName = input.asset.assetName;
        if (!state.get().generalAssetBalances.contains(locals.key)
            && state.get().generalAssetBalances.population() >= QTREAT_MAX_ASSETS)
        {
            output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
        }

        locals.xfer = qpi.transferShareOwnershipAndPossession(
            input.asset.assetName, input.asset.issuer,
            qpi.invocator(), qpi.invocator(), (sint64)input.amount, SELF);
        if (locals.xfer < 0) { output.returnCode = QTREAT_ERR_TRANSFER_FAILED; return; }

        locals.bal = 0;
        state.get().generalAssetBalances.get(locals.key, locals.bal);
        state.mut().generalAssetBalances.set(locals.key, sadd(locals.bal, input.amount));
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(ReleaseManagedShares)
    {
        locals.managed = qpi.numberOfShares(input.asset,
            { qpi.invocator(), SELF_INDEX }, { qpi.invocator(), SELF_INDEX });
        if (input.amount == 0 || locals.managed < (sint64)input.amount)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
        }
        if (input.asset.assetName == state.get().qdogeToken.assetName
            && input.asset.issuer == state.get().qdogeToken.issuer)
        {
            locals.info.staked = 0; locals.info.unstakeAmount = 0;
            state.get().stakers.get(qpi.invocator(), locals.info);
            locals.accounted = sadd(locals.info.staked, locals.info.unstakeAmount);
            if ((sint64)locals.accounted + (sint64)input.amount > locals.managed)
            {
                if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
            }
        }
        if (qpi.invocationReward() < QTREAT_QX_TRANSFER_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INSUFFICIENT_FEE; return;
        }
        if (qpi.invocationReward() > QTREAT_QX_TRANSFER_FEE)
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QTREAT_QX_TRANSFER_FEE);

        locals.rel = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(),
            (sint64)input.amount, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
            QTREAT_QX_TRANSFER_FEE);
        if (locals.rel < 0) { output.returnCode = QTREAT_ERR_TRANSFER_FAILED; return; }
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE(DepositMiningFund)
    {
        if (qpi.invocationReward() == 0) { output.returnCode = QTREAT_ERR_ZERO_AMOUNT; return; }
        state.mut().miningFund = sadd(state.get().miningFund, (uint64)qpi.invocationReward());
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(LoadAsicPart)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().adminAddress) { output.returnCode = QTREAT_ERR_ACCESS_DENIED; return; }
        if (state.get().asicCatalogLocked != 0) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }
        if (input.category >= 4 || input.rarity > QTREAT_MAX_RARITY) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }
        if (state.get().asicCatalog.contains((uint64)input.nftId)) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }

        locals.budget = QTREAT_RARITY_SUPPLY_COMMON;
        if (input.rarity == 1) locals.budget = QTREAT_RARITY_SUPPLY_UNCOMMON;
        if (input.rarity == 2) locals.budget = QTREAT_RARITY_SUPPLY_RARE;
        if (input.rarity == 3) locals.budget = QTREAT_RARITY_SUPPLY_EPIC;
        if (input.rarity == 4) locals.budget = QTREAT_RARITY_SUPPLY_LEGENDARY;
        if (state.get().asicCatalogBuckets.get((sint64)input.category * 8 + (sint64)input.rarity) >= locals.budget)
        {
            output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
        }

        if (state.mut().asicCatalog.set((uint64)input.nftId, (uint64)input.category * 8 + (uint64)input.rarity) == NULL_INDEX)
        {
            output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
        }
        state.mut().asicCatalogBuckets.set((sint64)input.category * 8 + (sint64)input.rarity,
            state.get().asicCatalogBuckets.get((sint64)input.category * 8 + (sint64)input.rarity) + 1);
        state.mut().asicCatalogSize = state.get().asicCatalogSize + 1;
        if (state.get().asicCatalogSize == QTREAT_ASIC_PARTS_TOTAL)
        {
            state.mut().asicCatalogLocked = 1;
            state.mut().dripStartEpoch = qpi.epoch();
        }
        output.catalogSize = state.get().asicCatalogSize;
        output.locked = state.get().asicCatalogLocked;
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(RegisterAsic)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (state.get().asicCatalogLocked == 0) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }
        if (state.get().totalAsicCount >= QTREAT_MAX_TOTAL_ASICS) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }

        locals.points = 0;
        for (locals.i = 0; locals.i < 4; locals.i++)
        {
            if (locals.i == 0) locals.partId = input.partMotherboard;
            if (locals.i == 1) locals.partId = input.partChip;
            if (locals.i == 2) locals.partId = input.partPsu;
            if (locals.i == 3) locals.partId = input.partFan;

            locals.packed = 0;
            if (!state.get().asicCatalog.get((uint64)locals.partId, locals.packed)
                || div(locals.packed, 8ULL) != (uint64)locals.i)
            {
                output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
            }
            locals.dummy = 0;
            if (state.get().asicUsedParts.get((uint64)locals.partId, locals.dummy))
            {
                output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
            }
            locals.qbayIn.NFTId = locals.partId;
            CALL_OTHER_CONTRACT_FUNCTION(QBAY, getInfoOfNFTById, locals.qbayIn, locals.qbayOut);
            if (interContractCallError != NoCallError)
            {
                output.returnCode = QTREAT_ERR_EXTERNAL_CALL; return;
            }
            if (locals.qbayOut.possessor != qpi.invocator())
            {
                output.returnCode = QTREAT_ERR_ACCESS_DENIED; return;
            }

            locals.r = mod(locals.packed, 8ULL);
            locals.partPoints = QTREAT_RARITY_POINTS_COMMON;
            if (locals.r == 1) locals.partPoints = QTREAT_RARITY_POINTS_UNCOMMON;
            if (locals.r == 2) locals.partPoints = QTREAT_RARITY_POINTS_RARE;
            if (locals.r == 3) locals.partPoints = QTREAT_RARITY_POINTS_EPIC;
            if (locals.r == 4) locals.partPoints = QTREAT_RARITY_POINTS_LEGENDARY;
            locals.points += locals.partPoints;
        }

        locals.freeSlot = -1;
        for (locals.i = 0; locals.i < (sint64)state.get().asicRigHighWater; locals.i++)
        {
            if (state.get().asicRigs.get(locals.i).active == 0) { locals.freeSlot = locals.i; break; }
        }
        if (locals.freeSlot < 0)
        {
            if (state.get().asicRigHighWater >= QTREAT_MAX_ASIC_RIGS)
            {
                output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
            }
            locals.freeSlot = (sint64)state.get().asicRigHighWater;
            state.mut().asicRigHighWater = state.get().asicRigHighWater + 1;
        }

        locals.rig.owner = qpi.invocator();
        locals.rig.partMotherboard = input.partMotherboard;
        locals.rig.partChip = input.partChip;
        locals.rig.partPsu = input.partPsu;
        locals.rig.partFan = input.partFan;
        locals.rig.weight = locals.points;
        locals.rig.active = 1;
        state.mut().asicRigs.set(locals.freeSlot, locals.rig);
        state.mut().asicUsedParts.set((uint64)input.partMotherboard, (uint64)locals.freeSlot + 1);
        state.mut().asicUsedParts.set((uint64)input.partChip, (uint64)locals.freeSlot + 1);
        state.mut().asicUsedParts.set((uint64)input.partPsu, (uint64)locals.freeSlot + 1);
        state.mut().asicUsedParts.set((uint64)input.partFan, (uint64)locals.freeSlot + 1);
        state.mut().totalAsicCount = state.get().totalAsicCount + 1;
        state.mut().totalMiningWeight = sadd(state.get().totalMiningWeight, locals.points);

        output.rigIndex = (uint64)locals.freeSlot;
        output.asicWeight = locals.points;
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(UnregisterAsic)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (input.rigIndex >= state.get().asicRigHighWater) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }
        locals.rig = state.get().asicRigs.get((sint64)input.rigIndex);
        if (locals.rig.active == 0 || locals.rig.owner != qpi.invocator())
        {
            output.returnCode = QTREAT_ERR_ACCESS_DENIED; return;
        }
        state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partMotherboard);
        state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partChip);
        state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partPsu);
        state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partFan);
        state.mut().totalAsicCount = state.get().totalAsicCount - 1;
        state.mut().totalMiningWeight = state.get().totalMiningWeight - locals.rig.weight;
        locals.rig.active = 0;
        state.mut().asicRigs.set((sint64)input.rigIndex, locals.rig);
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE(SetMiningRate)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().adminAddress) { output.returnCode = QTREAT_ERR_ACCESS_DENIED; return; }
        if (input.ratePerEpoch > QTREAT_MINING_REWARD_MAX) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }
        state.mut().miningRewardRate = input.ratePerEpoch;
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(DepositDripQdoge)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().adminAddress) { output.returnCode = QTREAT_ERR_ACCESS_DENIED; return; }
        if (input.amount == 0) { output.returnCode = QTREAT_ERR_ZERO_AMOUNT; return; }
        locals.xfer = qpi.transferShareOwnershipAndPossession(
            state.get().qdogeToken.assetName, state.get().qdogeToken.issuer,
            qpi.invocator(), qpi.invocator(), (sint64)input.amount, SELF);
        if (locals.xfer < 0) { output.returnCode = QTREAT_ERR_ACQUIRE_FAILED; return; }
        state.mut().dripQdogePool = sadd(state.get().dripQdogePool, input.amount);
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE(SetExcludeAddress)
    {
        if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
        if (qpi.invocator() != state.get().adminAddress) { output.returnCode = QTREAT_ERR_ACCESS_DENIED; return; }
        if (input.slot >= QTREAT_MAX_EXCLUDE_ADDRESSES) { output.returnCode = QTREAT_ERR_INVALID_INPUT; return; }
        state.mut().excludeAddresses.set((sint64)input.slot, input.address);
        output.returnCode = QTREAT_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(RevokeGeneralAsset)
    {
        if (qpi.invocator() != state.get().adminAddress)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_ACCESS_DENIED; return;
        }
        locals.key.issuer = input.asset.issuer;
        locals.key.assetName = input.asset.assetName;
        locals.bal = 0;
        state.get().generalAssetBalances.get(locals.key, locals.bal);
        if (input.amount == 0 || input.amount > locals.bal)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INVALID_INPUT; return;
        }
        if (qpi.invocationReward() < QTREAT_QX_TRANSFER_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QTREAT_ERR_INSUFFICIENT_FEE; return;
        }
        if (qpi.invocationReward() > QTREAT_QX_TRANSFER_FEE)
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QTREAT_QX_TRANSFER_FEE);

        locals.xfer = qpi.transferShareOwnershipAndPossession(
            input.asset.assetName, input.asset.issuer,
            SELF, SELF, (sint64)input.amount, qpi.invocator());
        if (locals.xfer < 0) { output.returnCode = QTREAT_ERR_TRANSFER_FAILED; return; }
        locals.rel = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(),
            (sint64)input.amount, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
            QTREAT_QX_TRANSFER_FEE);
        if (locals.rel < 0) { output.returnCode = QTREAT_ERR_TRANSFER_FAILED; return; }

        if (locals.bal - input.amount == 0)
        {
            state.mut().generalAssetBalances.removeByKey(locals.key);
        }
        else
        {
            state.mut().generalAssetBalances.set(locals.key, locals.bal - input.amount);
        }
        output.returnCode = QTREAT_OK;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetStakingInfo, 1);
        REGISTER_USER_FUNCTION(GetPhaseInfo, 2);
        REGISTER_USER_FUNCTION(GetFunds, 3);
        REGISTER_USER_FUNCTION(GetRaffleInfo, 4);
        REGISTER_USER_FUNCTION(GetExcludeAddresses, 5);
        REGISTER_USER_FUNCTION(GetNftInfo, 6);
        REGISTER_USER_FUNCTION(GetMinerInfo, 7);
        REGISTER_USER_FUNCTION(GetAsicCatalogInfo, 8);

        REGISTER_USER_PROCEDURE(DepositDividends, 1);
        REGISTER_USER_PROCEDURE(DepositStakingFund, 2);
        REGISTER_USER_PROCEDURE(RequestUnstake, 3);
        REGISTER_USER_PROCEDURE(FinalizeUnstake, 4);
        REGISTER_USER_PROCEDURE(ClaimQtreatBonus, 5);
        REGISTER_USER_PROCEDURE(DepositQtreatTokens, 6);
        REGISTER_USER_PROCEDURE(DepositGeneralAsset, 7);
        REGISTER_USER_PROCEDURE(SetExcludeAddress, 8);
        REGISTER_USER_PROCEDURE(RevokeGeneralAsset, 9);
        REGISTER_USER_PROCEDURE(ReleaseManagedShares, 10);
        REGISTER_USER_PROCEDURE(DepositMiningFund, 11);
        REGISTER_USER_PROCEDURE(LoadAsicPart, 12);
        REGISTER_USER_PROCEDURE(RegisterAsic, 13);
        REGISTER_USER_PROCEDURE(UnregisterAsic, 14);
        REGISTER_USER_PROCEDURE(DepositDripQdoge, 15);
        REGISTER_USER_PROCEDURE(SetMiningRate, 16);
    }

    INITIALIZE()
    {
        state.mut().qtreatToken.issuer = ID(
            _Q, _D, _O, _G, _E, _E, _E, _S,
            _K, _Y, _P, _A, _I, _C, _E, _C,
            _H, _E, _A, _H, _O, _X, _P, _U,
            _L, _E, _O, _A, _D, _T, _K, _G,
            _E, _J, _H, _A, _V, _Y, _P, _F,
            _K, _H, _L, _E, _W, _G, _X, _X,
            _Z, _Q, _U, _G, _I, _G, _M, _B
        );
        state.mut().qtreatToken.assetName = QTREAT_TOKEN_ASSETNAME;

        state.mut().qdogeToken.issuer = state.get().qtreatToken.issuer;
        state.mut().qdogeToken.assetName = QTREAT_QDOGE_ASSETNAME;

        // Admin address: QTREATZZIVFYQAIBKCZPSHGLIRMALZKHEWAPFLFXJAMDAXMGTBKQVXHHDHUD
        state.mut().adminAddress = ID(
            _Q, _T, _R, _E, _A, _T, _Z, _Z,
            _I, _V, _F, _Y, _Q, _A, _I, _B,
            _K, _C, _Z, _P, _S, _H, _G, _L,
            _I, _R, _M, _A, _L, _Z, _K, _H,
            _E, _W, _A, _P, _F, _L, _F, _X,
            _J, _A, _M, _D, _A, _X, _M, _G,
            _T, _B, _K, _Q, _V, _X, _H, _H
        );

        state.mut().dividendNftIds.set(0, 4968);
        state.mut().dividendNftIds.set(1, 4969);
        state.mut().dividendNftIds.set(2, 4970);
        state.mut().dividendNftIds.set(3, 4971);
        state.mut().dividendNftIds.set(4, 4972);
        state.mut().dividendNftIds.set(5, 4973);
        state.mut().dividendNftIds.set(6, 4974);
        state.mut().dividendNftIds.set(7, 4975);
        state.mut().dividendNftIds.set(8, 4976);
        state.mut().dividendNftIds.set(9, 4977);
        state.mut().dividendNftIds.set(10, 4978);
        state.mut().dividendNftIds.set(11, 4979);
        state.mut().dividendNftIds.set(12, 4980);
        state.mut().dividendNftIds.set(13, 4981);
        state.mut().dividendNftIds.set(14, 4982);
        state.mut().dividendNftIds.set(15, 4983);
        state.mut().dividendNftIds.set(16, 4984);
        state.mut().dividendNftIds.set(17, 4985);
        state.mut().dividendNftIds.set(18, 4986);
        state.mut().dividendNftIds.set(19, 4987);
        state.mut().dividendNftIds.set(20, 4988);
        state.mut().dividendNftIds.set(21, 4989);
        state.mut().dividendNftIds.set(22, 4990);
        state.mut().dividendNftIds.set(23, 4991);
        state.mut().dividendNftIds.set(24, 4992);
        state.mut().dividendNftIds.set(25, 4993);
        state.mut().dividendNftIds.set(26, 4994);
        state.mut().dividendNftIds.set(27, 4995);
        state.mut().dividendNftIds.set(28, 4996);
        state.mut().dividendNftIds.set(29, 4997);
        state.mut().dividendNftIds.set(30, 4998);
        state.mut().dividendNftIds.set(31, 4999);
        state.mut().dividendNftIds.set(32, 5000);
        state.mut().dividendNftIds.set(33, 5001);
        state.mut().dividendNftIds.set(34, 5002);
        state.mut().dividendNftIds.set(35, 5003);
        state.mut().dividendNftIds.set(36, 5004);
        state.mut().dividendNftIds.set(37, 5005);
        state.mut().dividendNftIds.set(38, 5006);
        state.mut().dividendNftIds.set(39, 5007);
        state.mut().dividendNftIds.set(40, 5008);
        state.mut().dividendNftIds.set(41, 5009);
        state.mut().dividendNftIds.set(42, 5010);
        state.mut().dividendNftIds.set(43, 5011);
        state.mut().dividendNftIds.set(44, 5012);
        state.mut().dividendNftIds.set(45, 5013);
        state.mut().dividendNftIds.set(46, 5014);
        state.mut().dividendNftIds.set(47, 5015);
        state.mut().dividendNftIds.set(48, 5016);
        state.mut().dividendNftIds.set(49, 5017);
        state.mut().dividendNftIds.set(50, 5018);
        state.mut().dividendNftIds.set(51, 5019);
        state.mut().dividendNftIds.set(52, 5020);
        state.mut().dividendNftIds.set(53, 5021);
        state.mut().dividendNftIds.set(54, 5022);
        state.mut().dividendNftIds.set(55, 5023);
        state.mut().dividendNftIds.set(56, 5024);
        state.mut().dividendNftIds.set(57, 5025);
        state.mut().dividendNftIds.set(58, 5026);
        state.mut().dividendNftIds.set(59, 5027);
        state.mut().dividendNftIds.set(60, 5028);
        state.mut().dividendNftIds.set(61, 5029);
        state.mut().dividendNftIds.set(62, 5030);
        state.mut().dividendNftIds.set(63, 5031);
        state.mut().dividendNftIds.set(64, 5032);
        state.mut().dividendNftIds.set(65, 5033);
        state.mut().dividendNftIds.set(66, 5034);
        state.mut().dividendNftIds.set(67, 5035);
        state.mut().dividendNftIds.set(68, 5036);
        state.mut().dividendNftIds.set(69, 5037);
        state.mut().dividendNftIds.set(70, 5038);
        state.mut().dividendNftIds.set(71, 5039);
        state.mut().dividendNftIds.set(72, 5040);
        state.mut().dividendNftIds.set(73, 5041);
        state.mut().dividendNftIds.set(74, 5042);
        state.mut().dividendNftIds.set(75, 5043);
        state.mut().dividendNftIds.set(76, 5044);
        state.mut().dividendNftIds.set(77, 5045);
        state.mut().dividendNftIds.set(78, 5046);
        state.mut().dividendNftIds.set(79, 5047);
        state.mut().dividendNftIds.set(80, 5048);
        state.mut().dividendNftIds.set(81, 5049);
        state.mut().dividendNftIds.set(82, 5050);
        state.mut().dividendNftIds.set(83, 5051);
        state.mut().dividendNftIds.set(84, 5052);
        state.mut().dividendNftIds.set(85, 5053);
        state.mut().dividendNftIds.set(86, 5054);
        state.mut().dividendNftIds.set(87, 5055);
        state.mut().dividendNftIds.set(88, 5056);
        state.mut().dividendNftIds.set(89, 5057);
        state.mut().dividendNftIds.set(90, 5058);
        state.mut().dividendNftIds.set(91, 5059);
        state.mut().dividendNftIds.set(92, 5060);
        state.mut().dividendNftIds.set(93, 5061);
        state.mut().dividendNftIds.set(94, 5062);
        state.mut().dividendNftIds.set(95, 5063);
        state.mut().dividendNftIds.set(96, 5064);
        state.mut().dividendNftIds.set(97, 5065);
        state.mut().dividendNftIds.set(98, 5066);
        state.mut().dividendNftIds.set(99, 5067);
        state.mut().dividendNftIds.set(100, 5068);
        state.mut().dividendNftIds.set(101, 5069);
        state.mut().dividendNftIds.set(102, 5070);
        state.mut().dividendNftIds.set(103, 5071);
        state.mut().dividendNftIds.set(104, 5072);
        state.mut().dividendNftIds.set(105, 5073);
        state.mut().dividendNftIds.set(106, 5074);
        state.mut().dividendNftIds.set(107, 5075);
        state.mut().dividendNftIds.set(108, 5076);
        state.mut().dividendNftIds.set(109, 5077);
        state.mut().dividendNftIds.set(110, 5078);
        state.mut().dividendNftIds.set(111, 5079);
        state.mut().dividendNftIds.set(112, 5080);
        state.mut().dividendNftIds.set(113, 5081);
        state.mut().dividendNftIds.set(114, 5082);
        state.mut().dividendNftIds.set(115, 5083);
        state.mut().dividendNftIds.set(116, 5084);
        state.mut().dividendNftIds.set(117, 5085);
        state.mut().dividendNftIds.set(118, 5086);
        state.mut().dividendNftIds.set(119, 5087);
        state.mut().dividendNftIds.set(120, 5088);
        state.mut().dividendNftIds.set(121, 5089);
        state.mut().dividendNftIds.set(122, 5090);
        state.mut().dividendNftIds.set(123, 5091);
        state.mut().dividendNftIds.set(124, 5092);
        state.mut().dividendNftIds.set(125, 5093);
        state.mut().dividendNftIds.set(126, 5094);
        state.mut().dividendNftIds.set(127, 5095);
        state.mut().dividendNftIds.set(128, 5096);
        state.mut().dividendNftIds.set(129, 5097);
        state.mut().dividendNftIds.set(130, 5098);
        state.mut().dividendNftIds.set(131, 5099);
        state.mut().dividendNftIds.set(132, 5100);
        state.mut().dividendNftIds.set(133, 5101);
        state.mut().dividendNftIds.set(134, 5102);
        state.mut().dividendNftIds.set(135, 5103);
        state.mut().dividendNftIds.set(136, 5104);
        state.mut().dividendNftIds.set(137, 5105);
        state.mut().dividendNftIds.set(138, 5106);
        state.mut().dividendNftIds.set(139, 5107);
        state.mut().dividendNftIds.set(140, 5108);
        state.mut().dividendNftIds.set(141, 5109);
        state.mut().dividendNftIds.set(142, 5110);
        state.mut().dividendNftIds.set(143, 5111);
        state.mut().dividendNftIds.set(144, 5112);
        state.mut().dividendNftIds.set(145, 5113);
        state.mut().dividendNftIds.set(146, 5114);
        state.mut().dividendNftIds.set(147, 5117);
        state.mut().dividendNftIds.set(148, 5118);
        state.mut().dividendNftIds.set(149, 5119);
        state.mut().dividendNftIds.set(150, 5120);
        state.mut().dividendNftIds.set(151, 5121);
        state.mut().dividendNftIds.set(152, 5122);
        state.mut().dividendNftIds.set(153, 5123);
        state.mut().dividendNftIds.set(154, 5124);
        state.mut().dividendNftIds.set(155, 5125);
        state.mut().dividendNftIds.set(156, 5126);
        state.mut().dividendNftIds.set(157, 5127);
        state.mut().dividendNftIds.set(158, 5128);
        state.mut().dividendNftIds.set(159, 5129);
        state.mut().dividendNftIds.set(160, 5130);
        state.mut().dividendNftIds.set(161, 5131);
        state.mut().dividendNftIds.set(162, 5132);
        state.mut().dividendNftIds.set(163, 5133);
        state.mut().dividendNftIds.set(164, 5134);
        state.mut().dividendNftIds.set(165, 5135);
        state.mut().dividendNftIds.set(166, 5136);
        state.mut().dividendNftIds.set(167, 5137);
        state.mut().dividendNftIds.set(168, 5138);
        state.mut().dividendNftIds.set(169, 5139);
        state.mut().dividendNftIds.set(170, 5140);
        state.mut().dividendNftIds.set(171, 5141);
        state.mut().dividendNftIds.set(172, 5142);
        state.mut().dividendNftIds.set(173, 5143);
        state.mut().dividendNftIds.set(174, 5144);
        state.mut().dividendNftIds.set(175, 5145);
        state.mut().dividendNftIds.set(176, 5146);
        state.mut().dividendNftIds.set(177, 5147);
        state.mut().dividendNftIds.set(178, 5148);
        state.mut().dividendNftIds.set(179, 5149);
        state.mut().dividendNftIds.set(180, 5150);
        state.mut().dividendNftIds.set(181, 5151);
        state.mut().dividendNftIds.set(182, 5152);
        state.mut().dividendNftIds.set(183, 5153);
        state.mut().dividendNftIds.set(184, 5154);
        state.mut().dividendNftIds.set(185, 5155);
        state.mut().dividendNftIds.set(186, 5156);
        state.mut().dividendNftIds.set(187, 5157);
        state.mut().dividendNftIds.set(188, 5158);
        state.mut().dividendNftIds.set(189, 5159);
        state.mut().dividendNftIds.set(190, 5160);
        state.mut().dividendNftIds.set(191, 5161);
        state.mut().dividendNftIds.set(192, 5162);
        state.mut().dividendNftIds.set(193, 5163);
        state.mut().dividendNftIds.set(194, 5164);
        state.mut().dividendNftIds.set(195, 5170);
        state.mut().dividendNftIds.set(196, 5172);
        state.mut().dividendNftIds.set(197, 5173);
        state.mut().dividendNftIds.set(198, 5174);
        state.mut().dividendNftIds.set(199, 5175);
        state.mut().dividendNftIdCount = 200;

        state.mut().miningRewardRate = QTREAT_MINING_REWARD_DEFAULT;

        state.mut().stakingStartEpoch = 0;
    }

    struct POST_INCOMING_TRANSFER_locals { uint64 prev; };
    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        if (input.type == TransferType::qpiDistributeDividends)
        {
            state.mut().dividendFund = sadd(state.get().dividendFund, (uint64)input.amount);
            locals.prev = 0;
            state.get().scDividendTracker.get(input.sourceId, locals.prev);
            state.mut().scDividendTracker.set(input.sourceId, sadd(locals.prev, (uint64)input.amount));
            return;
        }
        if (input.type == TransferType::qpiTransfer
            && input.sourceId == id(RANDOM_CONTRACT_INDEX, 0, 0, 0))
        {
            state.mut().stakingFund = sadd(state.get().stakingFund, (uint64)input.amount);
            return;
        }
        if (input.type == TransferType::standardTransaction
            || input.type == TransferType::qpiTransfer
            || input.type == TransferType::revenueDonation)
        {
            state.mut().dividendFund = sadd(state.get().dividendFund, (uint64)input.amount);
        }
    }

    struct BEGIN_EPOCH_locals { AssetPossessionIterator it; uint64 bal; id h; uint64 existing; sint64 exIdx; uint64 excluded; };
    BEGIN_EPOCH_WITH_LOCALS()
    {
        if (state.get().stakingStartEpoch == 0)
        {
            state.mut().stakingStartEpoch = qpi.epoch();
        }

        state.mut().beginBalances.reset();
        state.mut().endBalances.reset();
        state.mut().totalHoldersSnapshot = 0;
        if (state.get().qtreatToken.issuer != NULL_ID)
        {
            for (locals.it.begin(state.get().qtreatToken); !locals.it.reachedEnd(); locals.it.next())
            {
                if (locals.it.possessor() == SELF) continue;
                locals.bal = locals.it.numberOfPossessedShares();
                if (locals.bal < QTREAT_MIN_ELIGIBLE_BAL) continue;
                locals.h = locals.it.possessor();
                locals.excluded = 0;
                for (locals.exIdx = 0; locals.exIdx < (sint64)QTREAT_MAX_EXCLUDE_ADDRESSES; locals.exIdx++)
                {
                    if (state.get().excludeAddresses.get(locals.exIdx) != NULL_ID
                        && locals.h == state.get().excludeAddresses.get(locals.exIdx))
                    {
                        locals.excluded = 1;
                    }
                }
                if (locals.excluded != 0) continue;
                locals.existing = 0;
                state.get().beginBalances.get(locals.h, locals.existing);
                if (state.mut().beginBalances.set(locals.h, sadd(locals.existing, locals.bal)) != NULL_INDEX)
                {
                    state.mut().totalHoldersSnapshot = sadd(state.get().totalHoldersSnapshot, locals.bal);
                }
            }
        }
    }

    struct END_EPOCH_locals
    {
        AssetPossessionIterator it; uint64 bal; id h; uint64 existing;
        sint64 sidx; StakerInfo info; uint64 rewardBudget; uint64 reward; uint64 q; uint64 rem;
        uint64 phaseActive; uint64 elapsed;
        RANDOM::BuyEntropy_input entropyIn;
        RANDOM::BuyEntropy_output entropyOut;
        bit_4096 zeroEntropy;
        RaffleSeed seed;
        m256i raffleDigest;
        uint64 eligibleCount; uint64 winnerIdx; uint64 n;
        sint64 relResult;
        sint64 walletBal; uint64 holdings; uint64 totalWeight; uint64 weight;
        uint64 amount; sint64 idx; uint64 beginBal; uint64 endBal; uint64 eligible;
        uint64 nftCnt; uint64 totalEligible; sint64 exIdx; uint64 isExcluded;
        uint64 shareholderShare; uint64 perShare; uint64 paidShareholders;
        uint64 reservedDividend; uint64 distributedDividend;
        uint64 dripRate; uint64 dripDue; sint64 dripIdx; uint64 dripPacked; uint64 dripRarity;
        sint64 dripXfer; sint64 dripManaged; uint64 dripAccounted; StakerInfo dripStakerInfo;
        AsicRig rig; uint64 miningBudget; uint64 minerBudget; uint64 miningDividendCut;
        uint64 minerReward; uint64 stillOwned; uint32 rigPartId; sint64 pIdx; uint64 verifyOk;
        QBAY::getInfoOfNFTById_input qbayIn; QBAY::getInfoOfNFTById_output qbayOut;
        Entity ent; uint64 contractBalance;
        id oldPossessor;
    };
    END_EPOCH_WITH_LOCALS()
    {
        state.mut().beginBalances.cleanupIfNeeded();
        state.mut().endBalances.cleanupIfNeeded();
        state.mut().stakers.cleanupIfNeeded();
        state.mut().generalAssetBalances.cleanupIfNeeded();
        state.mut().scDividendTracker.cleanupIfNeeded();
        state.mut().nftCounts.cleanupIfNeeded();
        state.mut().asicUsedParts.cleanupIfNeeded();

        for (locals.sidx = state.get().stakers.nextElementIndex(NULL_INDEX);
             locals.sidx != NULL_INDEX;
             locals.sidx = state.get().stakers.nextElementIndex(locals.sidx))
        {
            locals.info = state.get().stakers.value(locals.sidx);
            if (locals.info.unstakeAmount == 0) continue;
            if (qpi.epoch() < locals.info.unstakeEpoch + QTREAT_UNSTAKE_DELAY_EPOCHS) continue;

            locals.h = state.get().stakers.key(locals.sidx);
            locals.relResult = qpi.releaseShares(state.get().qdogeToken, locals.h, locals.h,
                (sint64)locals.info.unstakeAmount, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
                QTREAT_QX_TRANSFER_FEE);
            if (locals.relResult < 0) continue;

            locals.info.unstakeAmount = 0;
            locals.info.unstakeEpoch = 0;
            if (locals.info.staked == 0 && locals.info.pendingBonus == 0
                && locals.info.bonusAwarded == 0)
            {
                state.mut().stakers.removeByKey(locals.h);
            }
            else
            {
                state.mut().stakers.set(locals.h, locals.info);
            }
        }

        qpi.getEntity(SELF, locals.ent);
        locals.contractBalance = locals.ent.incomingAmount - locals.ent.outgoingAmount;

        locals.phaseActive = 0;
        if (state.get().stakingStartEpoch != 0)
        {
            locals.elapsed = qpi.epoch() - state.get().stakingStartEpoch;
            if (locals.elapsed < QTREAT_TOTAL_REWARD_EPOCHS) locals.phaseActive = 1;
        }

        if (locals.phaseActive == 1 && state.get().totalStaked > 0)
        {
            locals.rewardBudget = QTREAT_STAKER_REWARD_PER_EPOCH;
            if (locals.rewardBudget > state.get().stakingFund) locals.rewardBudget = state.get().stakingFund;

            locals.totalWeight = 0;
            for (locals.sidx = state.get().stakers.nextElementIndex(NULL_INDEX);
                 locals.sidx != NULL_INDEX;
                 locals.sidx = state.get().stakers.nextElementIndex(locals.sidx))
            {
                locals.h = state.get().stakers.key(locals.sidx);
                locals.info = state.get().stakers.value(locals.sidx);
                if (locals.info.staked == 0)
                {
                    if (locals.info.bonusEpochs != 0 || locals.info.growthStreak != 0
                        || locals.info.lastStaked != 0)
                    {
                        locals.info.bonusEpochs = 0;
                        locals.info.growthStreak = 0;
                        locals.info.lastStaked = 0;
                        state.mut().stakers.set(locals.h, locals.info);
                    }
                    continue;
                }

                locals.walletBal = qpi.numberOfPossessedShares(
                    state.get().qdogeToken.assetName, state.get().qdogeToken.issuer,
                    locals.h, locals.h, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX);
                if (locals.walletBal < 0) locals.walletBal = 0;
                locals.holdings = sadd(sadd(locals.info.staked, locals.info.unstakeAmount), (uint64)locals.walletBal);
                if (locals.elapsed < QTREAT_PROGRESSIVE_START_DELAY_EPOCHS)
                {
                    locals.info.growthStreak = 0;
                    if (locals.holdings > locals.info.hwmHoldings)
                    {
                        locals.info.hwmHoldings = locals.holdings;
                    }
                }
                else if (locals.info.staked > locals.info.lastStaked
                    && locals.info.staked - locals.info.lastStaked >= QTREAT_PROGRESSIVE_MIN_STEP
                    && locals.holdings > locals.info.hwmHoldings)
                {
                    if (locals.info.growthStreak < QTREAT_PROGRESSIVE_MAX_STREAK)
                    {
                        locals.info.growthStreak += 1;
                    }
                    locals.info.hwmHoldings = locals.holdings;
                }
                else
                {
                    locals.info.growthStreak = 0;
                }
                locals.info.lastStaked = locals.info.staked;

                if (locals.info.staked >= QTREAT_BONUS_THRESHOLD)
                {
                    if (locals.info.bonusAwarded < QTREAT_BONUS_MAX_PER_WALLET)
                    {
                        locals.info.bonusEpochs += 1;
                        while (locals.info.bonusEpochs >= QTREAT_BONUS_INTERVAL_EPOCHS
                            && locals.info.bonusAwarded < QTREAT_BONUS_MAX_PER_WALLET)
                        {
                            locals.info.bonusEpochs -= QTREAT_BONUS_INTERVAL_EPOCHS;
                            locals.info.bonusAwarded += 1;
                            locals.info.pendingBonus += 1;
                        }
                    }
                }
                else
                {
                    locals.info.bonusEpochs = 0;
                }

                state.mut().stakers.set(locals.h, locals.info);
                locals.weight = ((uint128)locals.info.staked
                    * (uint128)(1000 + locals.info.growthStreak * QTREAT_PROGRESSIVE_BONUS_PERMILLE)).low;
                locals.totalWeight = sadd(locals.totalWeight, locals.weight);
            }

            if (locals.rewardBudget > 0 && locals.totalWeight > 0)
            {
                for (locals.sidx = state.get().stakers.nextElementIndex(NULL_INDEX);
                     locals.sidx != NULL_INDEX;
                     locals.sidx = state.get().stakers.nextElementIndex(locals.sidx))
                {
                    locals.h = state.get().stakers.key(locals.sidx);
                    locals.info = state.get().stakers.value(locals.sidx);
                    if (locals.info.staked == 0) continue;

                    locals.weight = ((uint128)locals.info.staked
                        * (uint128)(1000 + locals.info.growthStreak * QTREAT_PROGRESSIVE_BONUS_PERMILLE)).low;
                    locals.reward = div((uint128)locals.rewardBudget * (uint128)locals.weight,
                        (uint128)locals.totalWeight).low;
                    if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;
                    if (locals.reward == 0) continue;

                    qpi.transfer(locals.h, locals.reward);
                    locals.contractBalance -= locals.reward;
                    state.mut().stakingFund = state.get().stakingFund - locals.reward;
                    state.mut().totalStakingRewardsDistributed =
                        sadd(state.get().totalStakingRewardsDistributed, locals.reward);
                }
            }
        }

        if (state.get().stakingStartEpoch != 0
            && qpi.epoch() - state.get().stakingStartEpoch < QTREAT_RAFFLE_EPOCHS
            && state.get().qtreatBonusPool > 0
            && state.get().stakingFund >= QTREAT_RAFFLE_ENTROPY_FEE)
        {
            locals.eligibleCount = 0;
            for (locals.sidx = state.get().stakers.nextElementIndex(NULL_INDEX);
                 locals.sidx != NULL_INDEX;
                 locals.sidx = state.get().stakers.nextElementIndex(locals.sidx))
            {
                locals.info = state.get().stakers.value(locals.sidx);
                if (locals.info.staked > 0 && locals.info.unstakeAmount == 0)
                {
                    locals.eligibleCount++;
                }
            }

            if (locals.eligibleCount > 0)
            {
                state.mut().stakingFund = state.get().stakingFund - QTREAT_RAFFLE_ENTROPY_FEE;
                locals.entropyIn.collateralTier = QTREAT_RAFFLE_COLLATERAL_TIER;
                locals.entropyIn.numberOfBits = QTREAT_RAFFLE_ENTROPY_BITS;
                locals.entropyIn.trustee = id::zero();
                INVOKE_OTHER_CONTRACT_PROCEDURE(RANDOM, BuyEntropy, locals.entropyIn, locals.entropyOut, (sint64)QTREAT_RAFFLE_ENTROPY_FEE);

                if (interContractCallError != NoCallError)
                {
                    state.mut().stakingFund = sadd(state.get().stakingFund, QTREAT_RAFFLE_ENTROPY_FEE);
                    // Failed invoke leaves output stale; zero it.
                    locals.entropyOut.entropy = locals.zeroEntropy;
                }

                // Fallback seed is public and grindable without RANDOM.
                locals.seed.entropy = locals.entropyOut.entropy;
                locals.seed.prevSpectrumDigest = qpi.getPrevSpectrumDigest();
                locals.seed.prevUniverseDigest = qpi.getPrevUniverseDigest();
                locals.seed.prevComputerDigest = qpi.getPrevComputerDigest();
                locals.seed.epoch = qpi.epoch();
                locals.seed.tick = qpi.tick();
                locals.seed.eligibleCount = locals.eligibleCount;
                locals.raffleDigest = qpi.K12(locals.seed);
                locals.winnerIdx = mod((uint64)locals.raffleDigest.u64._0, locals.eligibleCount);

                locals.n = 0;
                for (locals.sidx = state.get().stakers.nextElementIndex(NULL_INDEX);
                     locals.sidx != NULL_INDEX;
                     locals.sidx = state.get().stakers.nextElementIndex(locals.sidx))
                {
                    locals.info = state.get().stakers.value(locals.sidx);
                    if (locals.info.staked == 0 || locals.info.unstakeAmount > 0) continue;
                    if (locals.n == locals.winnerIdx)
                    {
                        locals.h = state.get().stakers.key(locals.sidx);
                        locals.info.pendingBonus += 1;
                        state.mut().stakers.set(locals.h, locals.info);
                        state.mut().totalRaffleAwarded = state.get().totalRaffleAwarded + 1;
                        state.mut().lastRaffleWinner = locals.h;
                        state.mut().lastRaffleEpoch = qpi.epoch();
                        break;
                    }
                    locals.n++;
                }
            }
        }

        for (locals.sidx = state.get().stakers.nextElementIndex(NULL_INDEX);
             locals.sidx != NULL_INDEX;
             locals.sidx = state.get().stakers.nextElementIndex(locals.sidx))
        {
            locals.info = state.get().stakers.value(locals.sidx);
            if (locals.info.pendingBonus == 0) continue;
            if (state.get().qtreatBonusPool < locals.info.pendingBonus) continue;
            if (state.get().stakingFund < (uint64)QTREAT_QX_TRANSFER_FEE) continue;

            locals.h = state.get().stakers.key(locals.sidx);
            if (qpi.transferShareOwnershipAndPossession(
                    state.get().qtreatToken.assetName, state.get().qtreatToken.issuer,
                    SELF, SELF, (sint64)locals.info.pendingBonus, locals.h) < 0)
            {
                continue;
            }
            state.mut().qtreatBonusPool = state.get().qtreatBonusPool - locals.info.pendingBonus;
            state.mut().totalBonusDelivered = sadd(state.get().totalBonusDelivered, locals.info.pendingBonus);
            state.mut().stakingFund = state.get().stakingFund - (uint64)QTREAT_QX_TRANSFER_FEE;
            qpi.releaseShares(state.get().qtreatToken, locals.h, locals.h,
                (sint64)locals.info.pendingBonus, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
                QTREAT_QX_TRANSFER_FEE);

            locals.info.pendingBonus = 0;
            state.mut().stakers.set(locals.h, locals.info);
        }

        for (locals.sidx = 0; locals.sidx < (sint64)state.get().asicRigHighWater; locals.sidx++)
        {
            locals.rig = state.get().asicRigs.get(locals.sidx);
            if (locals.rig.active == 0) continue;
            // Only re-verify a rotating slice of rigs this epoch (see QTREAT_ASIC_VERIFY_SPREAD_EPOCHS);
            // un-checked rigs simply keep their current active state until their turn comes up.
            if (mod((uint64)locals.sidx + qpi.epoch(), QTREAT_ASIC_VERIFY_SPREAD_EPOCHS) != 0) continue;
            locals.stillOwned = 1;
            locals.verifyOk = 1;
            for (locals.pIdx = 0; locals.pIdx < 4; locals.pIdx++)
            {
                if (locals.pIdx == 0) locals.rigPartId = locals.rig.partMotherboard;
                if (locals.pIdx == 1) locals.rigPartId = locals.rig.partChip;
                if (locals.pIdx == 2) locals.rigPartId = locals.rig.partPsu;
                if (locals.pIdx == 3) locals.rigPartId = locals.rig.partFan;
                locals.qbayIn.NFTId = locals.rigPartId;
                CALL_OTHER_CONTRACT_FUNCTION(QBAY, getInfoOfNFTById, locals.qbayIn, locals.qbayOut);
                if (interContractCallError != NoCallError) { locals.verifyOk = 0; break; }
                if (locals.qbayOut.possessor != locals.rig.owner)
                {
                    locals.stillOwned = 0;
                }
            }
            if (locals.verifyOk == 1 && locals.stillOwned == 0)
            {
                state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partMotherboard);
                state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partChip);
                state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partPsu);
                state.mut().asicUsedParts.removeByKey((uint64)locals.rig.partFan);
                state.mut().totalAsicCount = state.get().totalAsicCount - 1;
                state.mut().totalMiningWeight = state.get().totalMiningWeight - locals.rig.weight;
                locals.rig.active = 0;
                state.mut().asicRigs.set(locals.sidx, locals.rig);
            }
        }

        if (state.get().totalMiningWeight > 0 && state.get().miningFund > 0)
        {
            qpi.getEntity(SELF, locals.ent);
            locals.contractBalance = locals.ent.incomingAmount - locals.ent.outgoingAmount;
            locals.miningBudget = state.get().miningRewardRate;
            if (locals.miningBudget > state.get().miningFund) locals.miningBudget = state.get().miningFund;

            // Split 50/50; odd QU goes to dividends.
            locals.minerBudget = div(locals.miningBudget, 2ULL);
            locals.miningDividendCut = locals.miningBudget - locals.minerBudget;

            if (locals.miningDividendCut > 0)
            {
                state.mut().miningFund = state.get().miningFund - locals.miningDividendCut;
                state.mut().dividendFund = sadd(state.get().dividendFund, locals.miningDividendCut);
            }

            for (locals.sidx = 0; locals.sidx < (sint64)state.get().asicRigHighWater; locals.sidx++)
            {
                locals.rig = state.get().asicRigs.get(locals.sidx);
                if (locals.rig.active == 0 || locals.rig.weight == 0) continue;

                locals.minerReward = div((uint128)locals.minerBudget * (uint128)locals.rig.weight,
                    (uint128)state.get().totalMiningWeight).low;
                if (locals.minerReward == 0) continue;
                if (locals.minerReward > locals.contractBalance) locals.minerReward = locals.contractBalance;

                qpi.transfer(locals.rig.owner, locals.minerReward);
                locals.contractBalance -= locals.minerReward;
                state.mut().miningFund = state.get().miningFund - locals.minerReward;
                state.mut().totalMiningRewardsDistributed =
                    sadd(state.get().totalMiningRewardsDistributed, locals.minerReward);
                if (locals.contractBalance == 0) break;
            }
        }

        if (state.get().dividendNftIdCount > 0)
        {
            // Re-check only a rotating slice of the dividend-NFT ids this epoch (see
            // QTREAT_NFT_SNAPSHOT_SPREAD_EPOCHS) instead of resetting nftCounts and re-querying
            // QBAY for all of them every epoch. nftCounts/totalNftCount are updated incrementally
            // against dividendNftLastPossessor, so ids not re-checked this epoch keep contributing
            // their last-known possessor unchanged.
            for (locals.sidx = 0; locals.sidx < (sint64)state.get().dividendNftIdCount; locals.sidx++)
            {
                if (mod((uint64)locals.sidx + qpi.epoch(), QTREAT_NFT_SNAPSHOT_SPREAD_EPOCHS) != 0) continue;

                locals.qbayIn.NFTId = state.get().dividendNftIds.get(locals.sidx);
                CALL_OTHER_CONTRACT_FUNCTION(QBAY, getInfoOfNFTById, locals.qbayIn, locals.qbayOut);
                locals.h = (interContractCallError != NoCallError) ? NULL_ID : locals.qbayOut.possessor;
                if (locals.h == SELF) locals.h = NULL_ID;
                if (locals.h != NULL_ID)
                {
                    locals.isExcluded = 0;
                    for (locals.exIdx = 0; locals.exIdx < (sint64)QTREAT_MAX_EXCLUDE_ADDRESSES; locals.exIdx++)
                    {
                        if (state.get().excludeAddresses.get(locals.exIdx) != NULL_ID
                            && locals.h == state.get().excludeAddresses.get(locals.exIdx))
                        {
                            locals.isExcluded = 1;
                        }
                    }
                    if (locals.isExcluded != 0) locals.h = NULL_ID;
                }

                locals.oldPossessor = state.get().dividendNftLastPossessor.get(locals.sidx);
                if (locals.h != locals.oldPossessor)
                {
                    if (locals.oldPossessor != NULL_ID)
                    {
                        locals.nftCnt = 0;
                        state.get().nftCounts.get(locals.oldPossessor, locals.nftCnt);
                        if (locals.nftCnt <= 1)
                            state.mut().nftCounts.removeByKey(locals.oldPossessor);
                        else
                            state.mut().nftCounts.set(locals.oldPossessor, locals.nftCnt - 1);
                        state.mut().totalNftCount = state.get().totalNftCount - 1;
                    }
                    if (locals.h != NULL_ID)
                    {
                        locals.nftCnt = 0;
                        state.get().nftCounts.get(locals.h, locals.nftCnt);
                        state.mut().nftCounts.set(locals.h, locals.nftCnt + 1);
                        state.mut().totalNftCount = state.get().totalNftCount + 1;
                    }
                    state.mut().dividendNftLastPossessor.set(locals.sidx, locals.h);
                }
            }
        }

        if (state.get().asicCatalogLocked == 1
            && state.get().dripStartEpoch != 0
            && qpi.epoch() - state.get().dripStartEpoch < QTREAT_DRIP_EPOCHS
            && state.get().dripQdogePool > 0)
        {
            state.mut().dripTally.reset();
            for (locals.dripIdx = state.get().asicCatalog.nextElementIndex(NULL_INDEX);
                 locals.dripIdx != NULL_INDEX;
                 locals.dripIdx = state.get().asicCatalog.nextElementIndex(locals.dripIdx))
            {
                locals.qbayIn.NFTId = (uint32)state.get().asicCatalog.key(locals.dripIdx);
                locals.dripPacked = state.get().asicCatalog.value(locals.dripIdx);
                locals.dripRarity = mod(locals.dripPacked, 8ULL);
                CALL_OTHER_CONTRACT_FUNCTION(QBAY, getInfoOfNFTById, locals.qbayIn, locals.qbayOut);
                if (interContractCallError != NoCallError) continue;
                if (locals.qbayOut.possessor == NULL_ID || locals.qbayOut.possessor == SELF) continue;
                if (locals.qbayOut.possessor == state.get().adminAddress) continue;

                locals.dripRate = QTREAT_DRIP_QDOGE_COMMON;
                if (locals.dripRarity == 1) locals.dripRate = QTREAT_DRIP_QDOGE_UNCOMMON;
                if (locals.dripRarity == 2) locals.dripRate = QTREAT_DRIP_QDOGE_RARE;
                if (locals.dripRarity == 3) locals.dripRate = QTREAT_DRIP_QDOGE_EPIC;
                if (locals.dripRarity == 4) locals.dripRate = QTREAT_DRIP_QDOGE_LEGENDARY;

                locals.existing = 0;
                state.get().dripTally.get(locals.qbayOut.possessor, locals.existing);
                state.mut().dripTally.set(locals.qbayOut.possessor, locals.existing + locals.dripRate);
            }

            for (locals.dripIdx = state.get().dripTally.nextElementIndex(NULL_INDEX);
                 locals.dripIdx != NULL_INDEX;
                 locals.dripIdx = state.get().dripTally.nextElementIndex(locals.dripIdx))
            {
                locals.h = state.get().dripTally.key(locals.dripIdx);
                locals.dripDue = state.get().dripTally.value(locals.dripIdx);
                if (locals.dripDue == 0) continue;
                if (state.get().dripQdogePool < locals.dripDue) continue;
                if (state.get().miningFund < (uint64)QTREAT_QX_TRANSFER_FEE) break;

                locals.dripXfer = qpi.transferShareOwnershipAndPossession(
                    state.get().qdogeToken.assetName, state.get().qdogeToken.issuer,
                    SELF, SELF, (sint64)locals.dripDue, locals.h);
                if (locals.dripXfer < 0) continue;
                state.mut().dripQdogePool = state.get().dripQdogePool - locals.dripDue;
                state.mut().totalDripQdogeDistributed = sadd(state.get().totalDripQdogeDistributed, locals.dripDue);
                state.mut().miningFund = state.get().miningFund - (uint64)QTREAT_QX_TRANSFER_FEE;
                locals.dripManaged = qpi.numberOfPossessedShares(
                    state.get().qdogeToken.assetName, state.get().qdogeToken.issuer,
                    locals.h, locals.h, SELF_INDEX, SELF_INDEX);
                locals.dripStakerInfo.staked = 0; locals.dripStakerInfo.unstakeAmount = 0;
                state.get().stakers.get(locals.h, locals.dripStakerInfo);
                locals.dripAccounted = locals.dripStakerInfo.staked + locals.dripStakerInfo.unstakeAmount;
                if (locals.dripManaged > (sint64)locals.dripAccounted)
                {
                    qpi.releaseShares(state.get().qdogeToken, locals.h, locals.h,
                        locals.dripManaged - (sint64)locals.dripAccounted,
                        QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX,
                        QTREAT_QX_TRANSFER_FEE);
                }
            }
        }

        if (state.get().qtreatToken.issuer != NULL_ID)
        {
            for (locals.it.begin(state.get().qtreatToken); !locals.it.reachedEnd(); locals.it.next())
            {
                if (locals.it.possessor() == SELF) continue;
                locals.bal = locals.it.numberOfPossessedShares();
                if (locals.bal == 0) continue;
                locals.h = locals.it.possessor();
                locals.existing = 0;
                state.get().endBalances.get(locals.h, locals.existing);
                state.mut().endBalances.set(locals.h, sadd(locals.existing, locals.bal));
            }
        }

        if (state.get().dividendFund == 0) return;
        qpi.getEntity(SELF, locals.ent);
        locals.contractBalance = locals.ent.incomingAmount - locals.ent.outgoingAmount;
        locals.amount = state.get().dividendFund;
        if (locals.amount > locals.contractBalance) locals.amount = locals.contractBalance;
        locals.totalEligible = sadd(state.get().totalHoldersSnapshot, state.get().totalNftCount);
        if (locals.amount == 0 || locals.totalEligible == 0) return;

        locals.reservedDividend = locals.amount;
        locals.distributedDividend = 0;
        state.mut().dividendFund = state.get().dividendFund - locals.reservedDividend;

        locals.shareholderShare = div((uint128)locals.amount * (uint128)QTREAT_DIVIDEND_SHAREHOLDER_PERMILLE, (uint128)1000ULL).low;
        locals.perShare = div(locals.shareholderShare, (uint64)NUMBER_OF_COMPUTORS);
        if (locals.perShare > 0 && qpi.distributeDividends((sint64)locals.perShare))
        {
            locals.paidShareholders = locals.perShare * (uint64)NUMBER_OF_COMPUTORS;
            locals.amount -= locals.paidShareholders;
            locals.distributedDividend = locals.paidShareholders;
            locals.contractBalance = (locals.contractBalance > locals.paidShareholders)
                ? locals.contractBalance - locals.paidShareholders : 0;
            state.mut().totalShareholderDividends = sadd(state.get().totalShareholderDividends, locals.paidShareholders);
        }

        for (locals.idx = state.get().beginBalances.nextElementIndex(NULL_INDEX);
             locals.idx != NULL_INDEX;
             locals.idx = state.get().beginBalances.nextElementIndex(locals.idx))
        {
            locals.h = state.get().beginBalances.key(locals.idx);
            locals.beginBal = state.get().beginBalances.value(locals.idx);
            locals.endBal = 0;
            state.get().endBalances.get(locals.h, locals.endBal);
            locals.eligible = (locals.endBal < locals.beginBal) ? locals.endBal : locals.beginBal;
            locals.nftCnt = 0;
            state.get().nftCounts.get(locals.h, locals.nftCnt);
            locals.eligible = sadd(locals.eligible, locals.nftCnt);
            if (locals.eligible == 0) continue;

            locals.q = div(locals.amount, locals.totalEligible);
            locals.rem = mod(locals.amount, locals.totalEligible);
            locals.reward = locals.q * locals.eligible
                + div((uint128)locals.rem * (uint128)locals.eligible, (uint128)locals.totalEligible).low;
            if (locals.reward == 0) continue;
            if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;

            qpi.transfer(locals.h, locals.reward);
            locals.contractBalance -= locals.reward;
            locals.distributedDividend = sadd(locals.distributedDividend, locals.reward);
            if (locals.contractBalance == 0) break;
        }

        for (locals.idx = state.get().nftCounts.nextElementIndex(NULL_INDEX);
             locals.idx != NULL_INDEX;
             locals.idx = state.get().nftCounts.nextElementIndex(locals.idx))
        {
            locals.h = state.get().nftCounts.key(locals.idx);
            if (state.get().beginBalances.contains(locals.h)) continue;
            locals.isExcluded = 0;
            for (locals.exIdx = 0; locals.exIdx < (sint64)QTREAT_MAX_EXCLUDE_ADDRESSES; locals.exIdx++)
            {
                if (state.get().excludeAddresses.get(locals.exIdx) != NULL_ID
                    && locals.h == state.get().excludeAddresses.get(locals.exIdx))
                {
                    locals.isExcluded = 1;
                }
            }
            if (locals.isExcluded != 0) continue;
            locals.eligible = state.get().nftCounts.value(locals.idx);
            if (locals.eligible == 0) continue;

            locals.q = div(locals.amount, locals.totalEligible);
            locals.rem = mod(locals.amount, locals.totalEligible);
            locals.reward = locals.q * locals.eligible
                + div((uint128)locals.rem * (uint128)locals.eligible, (uint128)locals.totalEligible).low;
            if (locals.reward == 0) continue;
            if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;

            qpi.transfer(locals.h, locals.reward);
            locals.contractBalance -= locals.reward;
            locals.distributedDividend = sadd(locals.distributedDividend, locals.reward);
            if (locals.contractBalance == 0) break;
        }

        if (locals.reservedDividend > locals.distributedDividend)
        {
            state.mut().dividendFund = sadd(state.get().dividendFund,
                locals.reservedDividend - locals.distributedDividend);
        }
        state.mut().totalDividendsDistributed =
            sadd(state.get().totalDividendsDistributed, locals.distributedDividend);
    }

    struct PRE_ACQUIRE_SHARES_locals { StakerInfo info; };
    PRE_ACQUIRE_SHARES_WITH_LOCALS()
    {
        if (input.otherContractIndex != QTREAT_QX_CONTRACT_INDEX) return;

        if (input.asset.assetName == state.get().qdogeToken.assetName
            && input.asset.issuer == state.get().qdogeToken.issuer)
        {
            if (input.owner == state.get().adminAddress)
            {
                output.allowTransfer = true;
                output.requestedFee = 0;
                return;
            }
            if (input.numberOfShares <= 0) return;
            locals.info.staked = 0; locals.info.unstakeAmount = 0; locals.info.unstakeEpoch = 0;
            locals.info.bonusEpochs = 0; locals.info.bonusAwarded = 0; locals.info.pendingBonus = 0;
            state.get().stakers.get(input.owner, locals.info);
            if (state.get().stakingStartEpoch != 0
                && qpi.epoch() - state.get().stakingStartEpoch >= QTREAT_TOTAL_REWARD_EPOCHS) return;
            if (locals.info.unstakeAmount > 0) return;
            if (locals.info.staked + (uint64)input.numberOfShares < QTREAT_MIN_STAKE) return;
            if (!state.get().stakers.contains(input.owner)
                && state.get().stakers.population() >= QTREAT_MAX_STAKERS) return;
            output.allowTransfer = true;
            output.requestedFee = 0;
            return;
        }

        output.allowTransfer = true;
        output.requestedFee = 0;
    }

    struct POST_ACQUIRE_SHARES_locals { StakerInfo info; uint64 existed; sint64 wallet; };
    POST_ACQUIRE_SHARES_WITH_LOCALS()
    {
        if (input.otherContractIndex != QTREAT_QX_CONTRACT_INDEX) return;
        if (input.asset.assetName != state.get().qdogeToken.assetName
            || input.asset.issuer != state.get().qdogeToken.issuer) return;
        if (input.owner == state.get().adminAddress) return;
        if (input.numberOfShares <= 0) return;

        locals.info.staked = 0; locals.info.unstakeAmount = 0; locals.info.unstakeEpoch = 0;
        locals.info.bonusEpochs = 0; locals.info.bonusAwarded = 0; locals.info.pendingBonus = 0;
        locals.info.hwmHoldings = 0; locals.info.lastStaked = 0; locals.info.growthStreak = 0;
        locals.existed = state.get().stakers.get(input.owner, locals.info) ? 1 : 0;
        locals.info.staked = sadd(locals.info.staked, (uint64)input.numberOfShares);
        if (locals.existed == 0)
        {
            locals.wallet = qpi.numberOfPossessedShares(
                state.get().qdogeToken.assetName, state.get().qdogeToken.issuer,
                input.owner, input.owner, QTREAT_QX_CONTRACT_INDEX, QTREAT_QX_CONTRACT_INDEX);
            if (locals.wallet < 0) locals.wallet = 0;
            locals.info.hwmHoldings = sadd(locals.info.staked, (uint64)locals.wallet);
        }
        if (state.mut().stakers.set(input.owner, locals.info) == NULL_INDEX)
        {
            return;
        }
        state.mut().totalStaked = sadd(state.get().totalStaked, (uint64)input.numberOfShares);
    }
};
