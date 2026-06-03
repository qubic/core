using namespace QPI;

// ============================================================================
// WolfPack (GGWP) - Revenue Distribution & Staking Smart Contract
//
// --- Revenue Payout (once per epoch / weekly, via END_EPOCH) ---
//
//   Revenue split:
//     70% -> GGWP token holders (proportional to token holdings)
//     10% -> SC shareholders (676 IPO shares, issuer=NULL_ID)
//     10% -> Active clan members (weighted by rank multiplier)
//     10% -> Reinvestment address (transferred out each payout)
//
//   GGWP token holders are snapshotted at BEGIN_EPOCH via AssetPossessionIterator.
//   SC shareholders (IPO shares) are also snapshotted at BEGIN_EPOCH separately.
//
// --- Staking ---
//
//   GGWP holders can stake their tokens into the contract.
//   Each epoch a fixed reward (WOLFPACK_STAKING_REWARD_PER_EPOCH) is distributed
//   proportionally among stakers from the staking reward pool.
//   Unstaking requires a delay of WOLFPACK_UNSTAKE_DELAY_EPOCHS epochs.
//   Accumulated staking rewards can be claimed at any time via ClaimStakingRewards.
// ============================================================================

// --- Constants ---
constexpr uint64 WOLFPACK_MAX_HOLDERS = 16384;
constexpr uint64 WOLFPACK_MAX_SHAREHOLDERS = 676;
constexpr uint64 WOLFPACK_MAX_CLAN_MEMBERS = 8192;
// Dust filter: token holders below this (accumulated) balance are excluded from the
// 70% distribution snapshot, so dust-spray can't bloat the holder loop. 1B supply / 10k = anti-dust.
constexpr uint64 WOLFPACK_MIN_ELIGIBLE_BALANCE = 10000;
// Minimum staked position: a staker's total staked balance must reach this.
// After a partial unstake the remaining balance must be 0 (full exit) or >= this.
constexpr uint64 WOLFPACK_MIN_STAKE = 500000;
constexpr uint64 WOLFPACK_DISTRIBUTION_PERMILLE_HOLDERS = 700;
constexpr uint64 WOLFPACK_DISTRIBUTION_PERMILLE_SHAREHOLDERS = 100;
constexpr uint64 WOLFPACK_DISTRIBUTION_PERMILLE_CLAN = 100;
constexpr uint64 WOLFPACK_DISTRIBUTION_PERMILLE_REINVEST = 90;       // was 100
constexpr uint64 WOLFPACK_DISTRIBUTION_PERMILLE_EXEC_RESERVE = 10;   // NEW: retained in-contract for execution-fee reserve (never paid out)
static_assert(WOLFPACK_DISTRIBUTION_PERMILLE_HOLDERS + WOLFPACK_DISTRIBUTION_PERMILLE_SHAREHOLDERS
            + WOLFPACK_DISTRIBUTION_PERMILLE_CLAN + WOLFPACK_DISTRIBUTION_PERMILLE_REINVEST
            + WOLFPACK_DISTRIBUTION_PERMILLE_EXEC_RESERVE == 1000,
            "GGWP distribution permille must sum to 1000");
constexpr uint64 WOLFPACK_SC_ASSET_NAME = 1347897159ULL; // "GGWP" as uint64

// --- Shareholder governance: change admin / reinvest address by >51% of SC shares ---
constexpr uint64 WOLFPACK_TOTAL_SC_SHARES = 676;       // total IPO shares = 100%
constexpr uint64 WOLFPACK_GOV_THRESHOLD_PERCENT = 51;  // pass at >= 51% of all SC shares (>= 345 shares)
constexpr uint64 WOLFPACK_GOV_PROPOSAL_MAX_EPOCHS = 4; // a proposal auto-expires after this many epochs
constexpr uint8  WOLFPACK_GOV_TARGET_NONE = 0;
constexpr uint8  WOLFPACK_GOV_TARGET_ADMIN = 1;
constexpr uint8  WOLFPACK_GOV_TARGET_REINVEST = 2;
constexpr uint64 WOLFPACK_MAX_GOV_PROPOSALS = 8;   // up to this many shareholder proposals active at once

// Payout timing
// Revenue is distributed once per epoch (weekly) in END_EPOCH - no time/day gate needed.

// Return codes
constexpr uint32 WOLFPACK_OK = 0;
constexpr uint32 WOLFPACK_ERROR_ACCESS_DENIED = 1;
constexpr uint32 WOLFPACK_ERROR_ZERO_AMOUNT = 2;
constexpr uint32 WOLFPACK_ERROR_NOT_HOLDER = 3;
constexpr uint32 WOLFPACK_ERROR_NOT_CLAN_MEMBER = 4;
constexpr uint32 WOLFPACK_ERROR_ALREADY_CLAN_MEMBER = 5;
constexpr uint32 WOLFPACK_ERROR_INVALID_RANK = 6;
constexpr uint32 WOLFPACK_ERROR_INVALID_SLOT = 15;
constexpr uint32 WOLFPACK_ERROR_NO_REWARD = 7;

// Rank multipliers (in permille: 1000 = 1.0x)
// Rank 0 = Recruit, 1 = Private, 2 = Sergeant, 3 = Lieutenant, 4 = Colonel, 5 = General
constexpr uint64 WOLFPACK_RANK_MULTIPLIER_0 = 1000;  // 1.0x Recruit
constexpr uint64 WOLFPACK_RANK_MULTIPLIER_1 = 1300;  // 1.3x Private
constexpr uint64 WOLFPACK_RANK_MULTIPLIER_2 = 1800;  // 1.8x Sergeant
constexpr uint64 WOLFPACK_RANK_MULTIPLIER_3 = 2500;  // 2.5x Lieutenant
constexpr uint64 WOLFPACK_RANK_MULTIPLIER_4 = 3200;  // 3.2x Colonel
constexpr uint64 WOLFPACK_RANK_MULTIPLIER_5 = 4000;  // 4.0x General
constexpr uint64 WOLFPACK_MAX_RANK = 5;

// Staking constants
constexpr uint64 WOLFPACK_STAKING_REWARD_PER_EPOCH = 1923076ULL; // ~100M / 52 epochs
constexpr uint64 WOLFPACK_UNSTAKE_DELAY_EPOCHS = 2;
constexpr uint16 WOLFPACK_QX_CONTRACT_INDEX = 1;
constexpr sint64 WOLFPACK_QX_TRANSFER_FEE = 100LL; // QX fee for management rights transfer
constexpr sint64 WOLFPACK_STAKE_FEE = 900LL;       // QU charged on Stake(), retained in the execution-fee reserve (causer-pays). User also pays the ~100 QU QX mgmt-rights fee separately → ~1000 total.

// Additional error codes
constexpr uint32 WOLFPACK_ERROR_INSUFFICIENT_STAKE = 8;
constexpr uint32 WOLFPACK_ERROR_UNSTAKE_PENDING = 9;
constexpr uint32 WOLFPACK_ERROR_ACQUIRE_FAILED = 10;
constexpr uint32 WOLFPACK_ERROR_TRANSFER_FAILED = 11;
constexpr uint32 WOLFPACK_ERROR_NO_PENDING_REWARDS = 12;
constexpr uint32 WOLFPACK_ERROR_UNSTAKE_NOT_READY = 13;
constexpr uint32 WOLFPACK_ERROR_NOT_STAKER = 14;
constexpr uint32 WOLFPACK_ERROR_NOT_SHAREHOLDER = 16;
constexpr uint32 WOLFPACK_ERROR_NO_ACTIVE_PROPOSAL = 17;
constexpr uint32 WOLFPACK_ERROR_PROPOSAL_ACTIVE = 18;
constexpr uint32 WOLFPACK_ERROR_INVALID_TARGET = 19;
constexpr uint32 WOLFPACK_ERROR_NULL_ADDRESS = 20;
constexpr uint32 WOLFPACK_ERROR_INSUFFICIENT_FEE = 21;
constexpr uint32 WOLFPACK_ERROR_BELOW_MIN_STAKE = 22;
constexpr uint32 WOLFPACK_ERROR_NO_PROPOSAL_SLOT = 23;
constexpr uint32 WOLFPACK_ERROR_INVALID_PROPOSAL = 24;

// Secondary state struct, reserved for future EXPAND events.
struct WOLFPACK2
{
};

struct WOLFPACK : public ContractBase
{
    // A single shareholder governance proposal (one slot in govProposals).
    struct WolfpackGovProposal
    {
        id proposedAddress;     // candidate new admin/reinvest address
        uint64 proposalId;      // unique increasing id (0 = none); votes reference this id
        uint64 proposalEpoch;   // epoch the proposal was opened (for expiry)
        uint8 status;           // 0 = inactive/empty, 1 = active
        uint8 targetType;       // WOLFPACK_GOV_TARGET_ADMIN / _REINVEST
    };

    // ======================== STATE ========================
    struct StateData
    {
        id adminAddress;

        // GGWP token asset reference (external token on QX)
        Asset wpToken;

        // Token holder snapshot (taken at BEGIN_EPOCH) - 70% pool
        HashMap<id, uint64, WOLFPACK_MAX_HOLDERS> holderBalances;
        uint64 totalTokensSnapshot;
        uint64 holderCount;

        // SC shareholder snapshot (taken at BEGIN_EPOCH) - 10% pool
        // These are the 676 IPO shares (issuer=NULL_ID, name="GGWP")
        HashMap<id, uint64, WOLFPACK_MAX_SHAREHOLDERS> shareholderBalances;
        uint64 totalSharesSnapshot;
        uint64 shareholderCount;

        // Clan system
        HashMap<id, uint64, WOLFPACK_MAX_CLAN_MEMBERS> clanRanks;
        uint64 clanMemberCount;
        uint64 clanWeightedTotal;

        // Revenue tracking
        uint64 pendingRevenue;
        uint64 reinvestmentFund;  // cumulative total sent to reinvestAddress
        uint64 execReserveFund;   // cumulative QU retained in-contract for execution-fee reserve
        uint64 totalDistributed;
        uint64 totalDeposited;
        uint64 lastDistributionEpoch;
        uint64 lastPayoutTick;

        // Exclude addresses from distribution
        id excludeAddress1;
        id excludeAddress2;

        // Recipient of the reinvestment share (10% of each payout)
        id reinvestAddress;

        // Shareholder governance (change adminAddress / reinvestAddress by >51% of SC shares).
        // Multiple proposals can be active at once; each holds one slot.
        Array<WolfpackGovProposal, WOLFPACK_MAX_GOV_PROPOSALS> govProposals;
        uint64 govNextProposalId;   // monotonic counter; assigns a unique id to each proposal
        HashMap<id, uint64, WOLFPACK_MAX_SHAREHOLDERS> govVoteMap; // shareholder -> proposalId they support (one vote)

        // Staking system
        HashMap<id, uint64, WOLFPACK_MAX_HOLDERS> stakedBalances;
        uint64 totalStaked;
        uint64 stakerCount;

        // Unstake requests
        HashMap<id, uint64, WOLFPACK_MAX_HOLDERS> unstakeAmounts;
        HashMap<id, uint64, WOLFPACK_MAX_HOLDERS> unstakeEpochs;
        uint64 unstakeCount;

        // Staking reward pool (GGWP tokens held by SC for distribution)
        uint64 stakingRewardPool;
        uint64 totalStakingRewardsDistributed;

        // Pending (unclaimed) staking rewards per user
        HashMap<id, uint64, WOLFPACK_MAX_HOLDERS> pendingStakingRewards;
    };

    // ======================== INPUT / OUTPUT ========================

    struct DepositRevenue_input { };
    struct DepositRevenue_output { uint32 returnCode; };

    struct AddClanMember_input { id memberAddress; uint64 rank; };
    struct AddClanMember_output { uint32 returnCode; };

    struct RemoveClanMember_input { id memberAddress; };
    struct RemoveClanMember_output { uint32 returnCode; };
    struct RemoveClanMember_locals { uint64 rank; };

    struct SetClanRank_input { id memberAddress; uint64 rank; };
    struct SetClanRank_output { uint32 returnCode; };
    struct SetClanRank_locals { uint64 oldRank; };

    struct SetAdmin_input { id newAdmin; };
    struct SetAdmin_output { uint32 returnCode; };

    struct SetExcludeAddress_input { uint64 slot; id address; };
    struct SetExcludeAddress_output { uint32 returnCode; };

    // Shareholder governance I/O
    struct ProposeGovChange_input { uint8 targetType; id newAddress; };
    struct ProposeGovChange_output { uint32 returnCode; uint64 proposalIndex; uint8 passed; };
    struct ProposeGovChange_locals { uint64 power; uint64 yesShares; sint64 vIdx; id voter; uint64 snap; sint64 slot; sint64 freeSlot; uint64 newId; WolfpackGovProposal prop; };

    struct VoteGovChange_input { uint64 proposalIndex; uint8 approve; };
    struct VoteGovChange_output { uint32 returnCode; uint8 passed; };
    struct VoteGovChange_locals { uint64 power; uint64 yesShares; sint64 vIdx; id voter; uint64 snap; uint64 voted; WolfpackGovProposal prop; };

    struct GetGovProposal_input { uint64 proposalIndex; };
    struct GetGovProposal_output { uint8 status; uint8 targetType; id proposedAddress; uint64 proposalId; uint64 proposalEpoch; uint64 yesShares; uint64 totalShares; uint64 requiredShares; };
    struct GetGovProposal_locals { sint64 vIdx; id voter; uint64 snap; WolfpackGovProposal prop; };

    // Staking I/O
    struct Stake_input { uint64 numberOfShares; };
    struct Stake_output { uint32 returnCode; };
    struct Stake_locals { uint64 existingStake; };

    struct RequestUnstake_input { uint64 numberOfShares; };
    struct RequestUnstake_output { uint32 returnCode; };
    struct RequestUnstake_locals { uint64 currentStake; };

    struct FinalizeUnstake_input { };
    struct FinalizeUnstake_output { uint32 returnCode; };
    struct FinalizeUnstake_locals { uint64 unstakeAmount; uint64 unstakeEpoch; sint64 releaseResult; };

    struct DepositStakingRewards_input { uint64 numberOfShares; };
    struct DepositStakingRewards_output { uint32 returnCode; };
    struct DepositStakingRewards_locals { sint64 transferResult; };

    struct ClaimStakingRewards_input { };
    struct ClaimStakingRewards_output { uint32 returnCode; uint64 claimedAmount; };
    struct ClaimStakingRewards_locals { uint64 pending; sint64 releaseResult; };

    struct GetStakingInfo_input { id stakerAddress; };
    struct GetStakingInfo_output { uint64 stakedAmount; uint64 pendingRewards; uint64 unstakeAmount; uint64 unstakeEpoch; uint64 totalStaked; uint64 stakingRewardPool; uint32 isStaker; };
    struct GetStakingInfo_locals { uint64 val; };

    struct GetStatus_input { };
    struct GetStatus_output
    {
        uint64 holderCount;
        uint64 totalTokensSnapshot;
        uint64 shareholderCount;
        uint64 totalSharesSnapshot;
        uint64 clanMemberCount;
        uint64 clanWeightedTotal;
        uint64 pendingRevenue;
        uint64 reinvestmentFund;
        uint64 execReserveFund;
        uint64 totalDistributed;
        uint64 totalDeposited;
        uint64 lastPayoutTick;
        uint64 lastDistributionEpoch;
        id adminAddress;
    };

    struct GetHolderInfo_input { id holderAddress; };
    struct GetHolderInfo_output { uint64 tokenBalance; uint32 isHolder; };
    struct GetHolderInfo_locals { uint64 val; };

    struct GetShareholderInfo_input { id shareholderAddress; };
    struct GetShareholderInfo_output { uint64 shares; uint32 isShareholder; };
    struct GetShareholderInfo_locals { uint64 val; };

    struct GetClanMemberInfo_input { id memberAddress; };
    struct GetClanMemberInfo_output { uint64 rank; uint32 isMember; };
    struct GetClanMemberInfo_locals { uint64 val; };

    struct GetExcludeAddresses_input { };
    struct GetExcludeAddresses_output { id address1; id address2; };

    // BUG-SONDE: returns the split that END_EPOCH would compute for a given
    // amount, without executing the transfers. Lets tests verify the
    // permille arithmetic in isolation from the qpi.transfer step.
    struct GetDistributionPreview_input { uint64 amount; };
    struct GetDistributionPreview_output { uint64 holderShare; uint64 shareholderShare; uint64 clanShare; uint64 reinvestShare; uint64 execReserveShare; };

    // ======================== FUNCTIONS (read-only) ========================

    PUBLIC_FUNCTION(GetStatus)
    {
        output.holderCount = state.get().holderCount;
        output.totalTokensSnapshot = state.get().totalTokensSnapshot;
        output.shareholderCount = state.get().shareholderCount;
        output.totalSharesSnapshot = state.get().totalSharesSnapshot;
        output.clanMemberCount = state.get().clanMemberCount;
        output.clanWeightedTotal = state.get().clanWeightedTotal;
        output.pendingRevenue = state.get().pendingRevenue;
        output.reinvestmentFund = state.get().reinvestmentFund;
        output.execReserveFund = state.get().execReserveFund;
        output.totalDistributed = state.get().totalDistributed;
        output.totalDeposited = state.get().totalDeposited;
        output.lastPayoutTick = state.get().lastPayoutTick;
        output.lastDistributionEpoch = state.get().lastDistributionEpoch;
        output.adminAddress = state.get().adminAddress;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetHolderInfo)
    {
        output.isHolder = state.get().holderBalances.get(input.holderAddress, locals.val) ? 1 : 0;
        if (output.isHolder)
        {
            output.tokenBalance = locals.val;
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetShareholderInfo)
    {
        output.isShareholder = state.get().shareholderBalances.get(input.shareholderAddress, locals.val) ? 1 : 0;
        if (output.isShareholder)
        {
            output.shares = locals.val;
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetClanMemberInfo)
    {
        output.isMember = state.get().clanRanks.get(input.memberAddress, locals.val) ? 1 : 0;
        if (output.isMember)
        {
            output.rank = locals.val;
        }
    }

    PUBLIC_FUNCTION(GetExcludeAddresses)
    {
        output.address1 = state.get().excludeAddress1;
        output.address2 = state.get().excludeAddress2;
    }

    PUBLIC_FUNCTION(GetDistributionPreview)
    {
        // Use `.low` instead of `(uint64)` cast: uint128_t only has
        // `operator bool()`, so `(uint64)<uint128_t>` collapses to 1.
        output.holderShare = div((uint128)input.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_HOLDERS, (uint128)1000ULL).low;
        output.shareholderShare = div((uint128)input.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_SHAREHOLDERS, (uint128)1000ULL).low;
        output.clanShare = div((uint128)input.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_CLAN, (uint128)1000ULL).low;
        output.execReserveShare = div((uint128)input.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_EXEC_RESERVE, (uint128)1000ULL).low;
        output.reinvestShare = input.amount - output.holderShare - output.shareholderShare - output.clanShare - output.execReserveShare;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetGovProposal)
    {
        output.totalShares = WOLFPACK_TOTAL_SC_SHARES;
        // required = ceil(totalShares * threshold% / 100)
        output.requiredShares = (WOLFPACK_TOTAL_SC_SHARES * WOLFPACK_GOV_THRESHOLD_PERCENT + 99) / 100;
        if (input.proposalIndex >= WOLFPACK_MAX_GOV_PROPOSALS)
        {
            return; // out of range -> zeroed output
        }
        locals.prop = state.get().govProposals.get((sint64)input.proposalIndex);
        output.targetType = locals.prop.targetType;
        output.proposedAddress = locals.prop.proposedAddress;
        output.proposalId = locals.prop.proposalId;
        output.proposalEpoch = locals.prop.proposalEpoch;
        // active only if marked active AND not yet expired
        output.status = (locals.prop.status == 1 &&
            qpi.epoch() < locals.prop.proposalEpoch + WOLFPACK_GOV_PROPOSAL_MAX_EPOCHS) ? 1 : 0;

        // Sum current-snapshot voting power of voters supporting this proposalId.
        output.yesShares = 0;
        for (locals.vIdx = state.get().govVoteMap.nextElementIndex(NULL_INDEX);
             locals.vIdx != NULL_INDEX;
             locals.vIdx = state.get().govVoteMap.nextElementIndex(locals.vIdx))
        {
            if (state.get().govVoteMap.value(locals.vIdx) != locals.prop.proposalId) continue;
            locals.voter = state.get().govVoteMap.key(locals.vIdx);
            locals.snap = 0;
            state.get().shareholderBalances.get(locals.voter, locals.snap);
            output.yesShares = sadd(output.yesShares, locals.snap);
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetStakingInfo)
    {
        output.isStaker = state.get().stakedBalances.get(input.stakerAddress, locals.val) ? 1 : 0;
        if (output.isStaker)
        {
            output.stakedAmount = locals.val;
        }
        locals.val = 0;
        state.get().pendingStakingRewards.get(input.stakerAddress, locals.val);
        output.pendingRewards = locals.val;
        locals.val = 0;
        if (state.get().unstakeAmounts.get(input.stakerAddress, locals.val))
        {
            output.unstakeAmount = locals.val;
            state.get().unstakeEpochs.get(input.stakerAddress, locals.val);
            output.unstakeEpoch = locals.val;
        }
        output.totalStaked = state.get().totalStaked;
        output.stakingRewardPool = state.get().stakingRewardPool;
    }

    // ======================== PROCEDURES (state-modifying) ========================

    PUBLIC_PROCEDURE(DepositRevenue)
    {
        if (qpi.invocationReward() == 0)
        {
            output.returnCode = WOLFPACK_ERROR_ZERO_AMOUNT;
            return;
        }
        state.mut().pendingRevenue = state.get().pendingRevenue + qpi.invocationReward();
        state.mut().totalDeposited = state.get().totalDeposited + qpi.invocationReward();
        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE(AddClanMember)
    {
        if (qpi.invocator() != state.get().adminAddress)
        {
            output.returnCode = WOLFPACK_ERROR_ACCESS_DENIED;
            return;
        }
        if (state.get().clanRanks.contains(input.memberAddress))
        {
            output.returnCode = WOLFPACK_ERROR_ALREADY_CLAN_MEMBER;
            return;
        }
        if (input.rank > WOLFPACK_MAX_RANK)
        {
            output.returnCode = WOLFPACK_ERROR_INVALID_RANK;
            return;
        }

        state.mut().clanRanks.set(input.memberAddress, input.rank);
        state.mut().clanMemberCount = state.get().clanMemberCount + 1;

        if (input.rank == 0) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_0;
        if (input.rank == 1) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_1;
        if (input.rank == 2) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_2;
        if (input.rank == 3) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_3;
        if (input.rank == 4) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_4;
        if (input.rank == 5) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_5;

        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveClanMember)
    {
        if (qpi.invocator() != state.get().adminAddress)
        {
            output.returnCode = WOLFPACK_ERROR_ACCESS_DENIED;
            return;
        }
        if (!state.get().clanRanks.get(input.memberAddress, locals.rank))
        {
            output.returnCode = WOLFPACK_ERROR_NOT_CLAN_MEMBER;
            return;
        }

        if (locals.rank == 0) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_0;
        if (locals.rank == 1) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_1;
        if (locals.rank == 2) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_2;
        if (locals.rank == 3) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_3;
        if (locals.rank == 4) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_4;
        if (locals.rank == 5) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_5;

        state.mut().clanRanks.removeByKey(input.memberAddress);
        state.mut().clanMemberCount = state.get().clanMemberCount - 1;

        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(SetClanRank)
    {
        if (qpi.invocator() != state.get().adminAddress)
        {
            output.returnCode = WOLFPACK_ERROR_ACCESS_DENIED;
            return;
        }
        if (!state.get().clanRanks.get(input.memberAddress, locals.oldRank))
        {
            output.returnCode = WOLFPACK_ERROR_NOT_CLAN_MEMBER;
            return;
        }
        if (input.rank > WOLFPACK_MAX_RANK)
        {
            output.returnCode = WOLFPACK_ERROR_INVALID_RANK;
            return;
        }

        if (locals.oldRank == 0) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_0;
        if (locals.oldRank == 1) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_1;
        if (locals.oldRank == 2) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_2;
        if (locals.oldRank == 3) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_3;
        if (locals.oldRank == 4) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_4;
        if (locals.oldRank == 5) state.mut().clanWeightedTotal = state.get().clanWeightedTotal - WOLFPACK_RANK_MULTIPLIER_5;

        state.mut().clanRanks.replace(input.memberAddress, input.rank);

        if (input.rank == 0) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_0;
        if (input.rank == 1) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_1;
        if (input.rank == 2) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_2;
        if (input.rank == 3) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_3;
        if (input.rank == 4) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_4;
        if (input.rank == 5) state.mut().clanWeightedTotal = state.get().clanWeightedTotal + WOLFPACK_RANK_MULTIPLIER_5;

        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE(SetAdmin)
    {
        // Admin address is immutable — hardcoded to token issuer at INITIALIZE.
        // SetAdmin is deprecated and no longer supported to prevent NULL_ID attacks.
        output.returnCode = WOLFPACK_ERROR_ACCESS_DENIED;
    }

    // Open a shareholder proposal to change the admin or reinvest address.
    // Caller must be an SC shareholder (present in the current BEGIN_EPOCH snapshot).
    // Up to WOLFPACK_MAX_GOV_PROPOSALS proposals can be active at the same time.
    PUBLIC_PROCEDURE_WITH_LOCALS(ProposeGovChange)
    {
        // Governance calls are not payable - refund any attached amount.
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        locals.power = 0;
        state.get().shareholderBalances.get(qpi.invocator(), locals.power);
        if (locals.power == 0)
        {
            output.returnCode = WOLFPACK_ERROR_NOT_SHAREHOLDER;
            return;
        }
        if (input.targetType != WOLFPACK_GOV_TARGET_ADMIN && input.targetType != WOLFPACK_GOV_TARGET_REINVEST)
        {
            output.returnCode = WOLFPACK_ERROR_INVALID_TARGET;
            return;
        }
        if (input.newAddress == NULL_ID)
        {
            output.returnCode = WOLFPACK_ERROR_NULL_ADDRESS;
            return;
        }

        // Find a free slot: empty (status 0) or holding an expired proposal.
        locals.freeSlot = -1;
        for (locals.slot = 0; locals.slot < (sint64)WOLFPACK_MAX_GOV_PROPOSALS; locals.slot++)
        {
            locals.prop = state.get().govProposals.get(locals.slot);
            if (locals.prop.status == 0 ||
                qpi.epoch() >= locals.prop.proposalEpoch + WOLFPACK_GOV_PROPOSAL_MAX_EPOCHS)
            {
                locals.freeSlot = locals.slot;
                break;
            }
        }
        if (locals.freeSlot < 0)
        {
            output.returnCode = WOLFPACK_ERROR_NO_PROPOSAL_SLOT;
            return;
        }

        // Open the proposal with a fresh unique id; proposer auto-votes "yes".
        // The unique id means stale votes from a recycled slot can never count.
        locals.newId = state.get().govNextProposalId + 1;
        state.mut().govNextProposalId = locals.newId;

        locals.prop.proposedAddress = input.newAddress;
        locals.prop.proposalId = locals.newId;
        locals.prop.proposalEpoch = qpi.epoch();
        locals.prop.status = 1;
        locals.prop.targetType = input.targetType;
        state.mut().govProposals.set(locals.freeSlot, locals.prop);

        state.mut().govVoteMap.set(qpi.invocator(), locals.newId);

        // Tally shares of voters supporting this proposalId.
        locals.yesShares = 0;
        for (locals.vIdx = state.get().govVoteMap.nextElementIndex(NULL_INDEX);
             locals.vIdx != NULL_INDEX;
             locals.vIdx = state.get().govVoteMap.nextElementIndex(locals.vIdx))
        {
            if (state.get().govVoteMap.value(locals.vIdx) != locals.newId) continue;
            locals.voter = state.get().govVoteMap.key(locals.vIdx);
            locals.snap = 0;
            state.get().shareholderBalances.get(locals.voter, locals.snap);
            locals.yesShares = sadd(locals.yesShares, locals.snap);
        }

        output.proposalIndex = (uint64)locals.freeSlot;
        output.passed = 0;
        if (locals.yesShares * 100 >= WOLFPACK_TOTAL_SC_SHARES * WOLFPACK_GOV_THRESHOLD_PERCENT)
        {
            if (locals.prop.targetType == WOLFPACK_GOV_TARGET_ADMIN)
            {
                state.mut().adminAddress = locals.prop.proposedAddress;
            }
            else
            {
                state.mut().reinvestAddress = locals.prop.proposedAddress;
            }
            locals.prop.status = 0;
            state.mut().govProposals.set(locals.freeSlot, locals.prop);
            output.passed = 1;
        }
        output.returnCode = WOLFPACK_OK;
    }

    // Vote on a specific proposal by index. approve=1 casts/keeps a "yes"; approve=0 withdraws.
    // A shareholder supports at most one proposal at a time. Executes once >= 51% is reached.
    PUBLIC_PROCEDURE_WITH_LOCALS(VoteGovChange)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (input.proposalIndex >= WOLFPACK_MAX_GOV_PROPOSALS)
        {
            output.returnCode = WOLFPACK_ERROR_INVALID_PROPOSAL;
            return;
        }
        locals.prop = state.get().govProposals.get((sint64)input.proposalIndex);
        if (locals.prop.status == 0 ||
            qpi.epoch() >= locals.prop.proposalEpoch + WOLFPACK_GOV_PROPOSAL_MAX_EPOCHS)
        {
            output.returnCode = WOLFPACK_ERROR_NO_ACTIVE_PROPOSAL;
            return;
        }

        locals.power = 0;
        state.get().shareholderBalances.get(qpi.invocator(), locals.power);
        if (locals.power == 0)
        {
            output.returnCode = WOLFPACK_ERROR_NOT_SHAREHOLDER;
            return;
        }

        if (input.approve == 0)
        {
            // Withdraw only if currently supporting THIS proposal.
            locals.voted = 0;
            if (state.get().govVoteMap.get(qpi.invocator(), locals.voted) && locals.voted == locals.prop.proposalId)
            {
                state.mut().govVoteMap.removeByKey(qpi.invocator());
            }
        }
        else
        {
            // One vote per voter: switches support to this proposal.
            state.mut().govVoteMap.set(qpi.invocator(), locals.prop.proposalId);
        }

        // Tally shares of voters supporting this proposalId (transfers can't double-count -
        // a sold-out voter contributes 0 from the snapshot).
        locals.yesShares = 0;
        for (locals.vIdx = state.get().govVoteMap.nextElementIndex(NULL_INDEX);
             locals.vIdx != NULL_INDEX;
             locals.vIdx = state.get().govVoteMap.nextElementIndex(locals.vIdx))
        {
            if (state.get().govVoteMap.value(locals.vIdx) != locals.prop.proposalId) continue;
            locals.voter = state.get().govVoteMap.key(locals.vIdx);
            locals.snap = 0;
            state.get().shareholderBalances.get(locals.voter, locals.snap);
            locals.yesShares = sadd(locals.yesShares, locals.snap);
        }

        output.passed = 0;
        if (locals.yesShares * 100 >= WOLFPACK_TOTAL_SC_SHARES * WOLFPACK_GOV_THRESHOLD_PERCENT)
        {
            if (locals.prop.targetType == WOLFPACK_GOV_TARGET_ADMIN)
            {
                state.mut().adminAddress = locals.prop.proposedAddress;
            }
            else
            {
                state.mut().reinvestAddress = locals.prop.proposedAddress;
            }
            locals.prop.status = 0;
            state.mut().govProposals.set((sint64)input.proposalIndex, locals.prop);
            output.passed = 1;
        }
        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE(SetExcludeAddress)
    {
        if (qpi.invocator() != state.get().adminAddress)
        {
            output.returnCode = WOLFPACK_ERROR_ACCESS_DENIED;
            return;
        }
        if (input.slot == 1)
        {
            state.mut().excludeAddress1 = input.address;
        }
        else if (input.slot == 2)
        {
            state.mut().excludeAddress2 = input.address;
        }
        else
        {
            output.returnCode = WOLFPACK_ERROR_INVALID_SLOT;
            return;
        }
        output.returnCode = WOLFPACK_OK;
    }

    // ======================== STAKING PROCEDURES ========================

    PUBLIC_PROCEDURE_WITH_LOCALS(Stake)
    {
        if (input.numberOfShares == 0)
        {
            output.returnCode = WOLFPACK_ERROR_ZERO_AMOUNT;
            return;
        }
        if (state.get().unstakeAmounts.contains(qpi.invocator()))
        {
            output.returnCode = WOLFPACK_ERROR_UNSTAKE_PENDING;
            return;
        }
        // Minimum stake: the resulting staked position must reach WOLFPACK_MIN_STAKE.
        locals.existingStake = 0;
        state.get().stakedBalances.get(qpi.invocator(), locals.existingStake);
        if (locals.existingStake + input.numberOfShares < WOLFPACK_MIN_STAKE)
        {
            output.returnCode = WOLFPACK_ERROR_BELOW_MIN_STAKE;
            return;
        }
        // Verify invocator has enough GGWP shares already under WP's management.
        // User must call QX.TransferShareManagementRights(asset=wpToken, shares=N, newMgmtIdx=GGWP) first.
        if (qpi.numberOfPossessedShares(state.get().wpToken.assetName, state.get().wpToken.issuer,
            qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < (sint64)input.numberOfShares)
        {
            output.returnCode = WOLFPACK_ERROR_ACQUIRE_FAILED;
            return;
        }

        // Causer-pays (self-sustain): the stake fee stays in the contract's QU balance
        // (= execution-fee reserve). It is NOT transferred out and NOT burned.
        if (qpi.invocationReward() < WOLFPACK_STAKE_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = WOLFPACK_ERROR_INSUFFICIENT_FEE;
            return;
        }
        if (qpi.invocationReward() > WOLFPACK_STAKE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - WOLFPACK_STAKE_FEE);
        }
        state.mut().execReserveFund = state.get().execReserveFund + (uint64)WOLFPACK_STAKE_FEE;

        locals.existingStake = 0;
        state.get().stakedBalances.get(qpi.invocator(), locals.existingStake);
        state.mut().stakedBalances.set(qpi.invocator(), locals.existingStake + input.numberOfShares);
        if (locals.existingStake == 0)
        {
            state.mut().stakerCount = state.get().stakerCount + 1;
        }
        state.mut().totalStaked = state.get().totalStaked + input.numberOfShares;
        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(RequestUnstake)
    {
        if (input.numberOfShares == 0)
        {
            output.returnCode = WOLFPACK_ERROR_ZERO_AMOUNT;
            return;
        }
        if (!state.get().stakedBalances.get(qpi.invocator(), locals.currentStake))
        {
            output.returnCode = WOLFPACK_ERROR_NOT_STAKER;
            return;
        }
        if (input.numberOfShares > locals.currentStake)
        {
            output.returnCode = WOLFPACK_ERROR_INSUFFICIENT_STAKE;
            return;
        }
        if (state.get().unstakeAmounts.contains(qpi.invocator()))
        {
            output.returnCode = WOLFPACK_ERROR_UNSTAKE_PENDING;
            return;
        }
        // Either fully exit (remaining 0) or keep at least the minimum stake.
        // Partial unstakes that would leave a sub-minimum dust position are rejected.
        if (input.numberOfShares < locals.currentStake &&
            locals.currentStake - input.numberOfShares < WOLFPACK_MIN_STAKE)
        {
            output.returnCode = WOLFPACK_ERROR_BELOW_MIN_STAKE;
            return;
        }

        if (input.numberOfShares == locals.currentStake)
        {
            state.mut().stakedBalances.removeByKey(qpi.invocator());
            state.mut().stakerCount = state.get().stakerCount - 1;
        }
        else
        {
            state.mut().stakedBalances.replace(qpi.invocator(), locals.currentStake - input.numberOfShares);
        }
        state.mut().totalStaked = state.get().totalStaked - input.numberOfShares;

        state.mut().unstakeAmounts.set(qpi.invocator(), input.numberOfShares);
        state.mut().unstakeEpochs.set(qpi.invocator(), qpi.epoch());
        state.mut().unstakeCount = state.get().unstakeCount + 1;
        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(FinalizeUnstake)
    {
        if (!state.get().unstakeAmounts.get(qpi.invocator(), locals.unstakeAmount))
        {
            output.returnCode = WOLFPACK_ERROR_NOT_STAKER;
            return;
        }
        state.get().unstakeEpochs.get(qpi.invocator(), locals.unstakeEpoch);
        if (qpi.epoch() < locals.unstakeEpoch + WOLFPACK_UNSTAKE_DELAY_EPOCHS)
        {
            output.returnCode = WOLFPACK_ERROR_UNSTAKE_NOT_READY;
            return;
        }

        // Unstake fee (mirrors Stake): 100 QU covers the QX release fee, 900 QU is
        // retained in the execution-fee reserve. Total = WOLFPACK_QX_TRANSFER_FEE + WOLFPACK_STAKE_FEE.
        // (The 100 QU offsets the fee releaseShares deducts from SELF; the 900 QU stay in the balance.)
        if (qpi.invocationReward() < WOLFPACK_QX_TRANSFER_FEE + WOLFPACK_STAKE_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = WOLFPACK_ERROR_INSUFFICIENT_FEE;
            return;
        }
        if (qpi.invocationReward() > WOLFPACK_QX_TRANSFER_FEE + WOLFPACK_STAKE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (WOLFPACK_QX_TRANSFER_FEE + WOLFPACK_STAKE_FEE));
        }
        state.mut().execReserveFund = state.get().execReserveFund + (uint64)WOLFPACK_STAKE_FEE;

        locals.releaseResult = qpi.releaseShares(state.get().wpToken, qpi.invocator(), qpi.invocator(),
            (sint64)locals.unstakeAmount, WOLFPACK_QX_CONTRACT_INDEX, WOLFPACK_QX_CONTRACT_INDEX, WOLFPACK_QX_TRANSFER_FEE);
        if (locals.releaseResult < 0)
        {
            output.returnCode = WOLFPACK_ERROR_TRANSFER_FAILED;
            return;
        }

        state.mut().unstakeAmounts.removeByKey(qpi.invocator());
        state.mut().unstakeEpochs.removeByKey(qpi.invocator());
        state.mut().unstakeCount = state.get().unstakeCount - 1;
        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(DepositStakingRewards)
    {
        if (input.numberOfShares == 0)
        {
            output.returnCode = WOLFPACK_ERROR_ZERO_AMOUNT;
            return;
        }
        // Verify invocator has enough GGWP shares under WP's management.
        // User must call QX.TransferShareManagementRights(asset=wpToken, newMgmtIdx=GGWP) first.
        if (qpi.numberOfPossessedShares(state.get().wpToken.assetName, state.get().wpToken.issuer,
            qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < (sint64)input.numberOfShares)
        {
            output.returnCode = WOLFPACK_ERROR_ACQUIRE_FAILED;
            return;
        }

        state.mut().stakingRewardPool = state.get().stakingRewardPool + input.numberOfShares;
        output.returnCode = WOLFPACK_OK;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(ClaimStakingRewards)
    {
        if (!state.get().pendingStakingRewards.get(qpi.invocator(), locals.pending) || locals.pending == 0)
        {
            output.returnCode = WOLFPACK_ERROR_NO_PENDING_REWARDS;
            return;
        }

        // Fix 1: the user covers the QX release fee (the contract no longer pays it).
        if (qpi.invocationReward() < WOLFPACK_QX_TRANSFER_FEE)
        {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = WOLFPACK_ERROR_INSUFFICIENT_FEE;
            return;
        }
        if (qpi.invocationReward() > WOLFPACK_QX_TRANSFER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - WOLFPACK_QX_TRANSFER_FEE);
        }

        locals.releaseResult = qpi.releaseShares(state.get().wpToken, qpi.invocator(), qpi.invocator(),
            (sint64)locals.pending, WOLFPACK_QX_CONTRACT_INDEX, WOLFPACK_QX_CONTRACT_INDEX, WOLFPACK_QX_TRANSFER_FEE);
        if (locals.releaseResult < 0)
        {
            output.returnCode = WOLFPACK_ERROR_TRANSFER_FAILED;
            return;
        }

        state.mut().pendingStakingRewards.removeByKey(qpi.invocator());
        output.claimedAmount = locals.pending;
        output.returnCode = WOLFPACK_OK;
    }

    // ======================== REGISTRATION ========================

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetStatus, 1);
        REGISTER_USER_FUNCTION(GetHolderInfo, 2);
        REGISTER_USER_FUNCTION(GetClanMemberInfo, 3);
        REGISTER_USER_FUNCTION(GetShareholderInfo, 4);

        REGISTER_USER_PROCEDURE(DepositRevenue, 1);
        REGISTER_USER_PROCEDURE(AddClanMember, 2);
        REGISTER_USER_PROCEDURE(RemoveClanMember, 3);
        REGISTER_USER_PROCEDURE(SetClanRank, 4);
        REGISTER_USER_PROCEDURE(SetAdmin, 5);
        REGISTER_USER_PROCEDURE(SetExcludeAddress, 6);
        REGISTER_USER_PROCEDURE(Stake, 7);
        REGISTER_USER_PROCEDURE(RequestUnstake, 8);
        REGISTER_USER_PROCEDURE(FinalizeUnstake, 9);
        REGISTER_USER_PROCEDURE(DepositStakingRewards, 10);
        REGISTER_USER_PROCEDURE(ClaimStakingRewards, 11);
        REGISTER_USER_PROCEDURE(ProposeGovChange, 12);
        REGISTER_USER_PROCEDURE(VoteGovChange, 13);

        REGISTER_USER_FUNCTION(GetStakingInfo, 5);
        REGISTER_USER_FUNCTION(GetExcludeAddresses, 6);
        REGISTER_USER_FUNCTION(GetDistributionPreview, 7);
        REGISTER_USER_FUNCTION(GetGovProposal, 8);
    }

    // ======================== SYSTEM PROCEDURES ========================

    INITIALIZE()
    {
        // GGWP token (external, issued on QX by MLMWPS...)
        state.mut().wpToken.issuer = ID(
            _M, _L, _M, _W, _P, _S, _Q, _N,
            _V, _A, _I, _B, _R, _F, _D, _H,
            _W, _C, _K, _S, _F, _O, _V, _U,
            _A, _Z, _D, _D, _W, _K, _J, _G,
            _C, _L, _R, _S, _Y, _Z, _I, _U,
            _E, _F, _D, _U, _R, _P, _W, _I,
            _P, _Q, _X, _A, _C, _Y, _O, _E
        );
        state.mut().wpToken.assetName = WOLFPACK_SC_ASSET_NAME; // "GGWP"

        // Admin and reinvestment recipient are hardcoded to the GGWP token
        // issuer identity. Hardcoding the admin (instead of a NULL_ID bootstrap)
        // closes the race window where any first caller of SetAdmin could
        // seize control. The admin can still rotate itself later via SetAdmin.
        state.mut().adminAddress = state.get().wpToken.issuer;
        state.mut().reinvestAddress = state.get().wpToken.issuer;

        // Shareholder governance starts with no active proposals.
        // (govProposals slots are zero-initialised => status 0 / inactive.)
        state.mut().govNextProposalId = 0;
        state.mut().govVoteMap.reset();

        state.mut().totalTokensSnapshot = 0;
        state.mut().holderCount = 0;
        state.mut().totalSharesSnapshot = 0;
        state.mut().shareholderCount = 0;
        state.mut().clanMemberCount = 0;
        state.mut().clanWeightedTotal = 0;
        state.mut().pendingRevenue = 0;
        state.mut().reinvestmentFund = 0;
        state.mut().execReserveFund = 0;
        state.mut().totalDistributed = 0;
        state.mut().totalDeposited = 0;
        state.mut().lastDistributionEpoch = 0;
        state.mut().lastPayoutTick = 0;
        state.mut().excludeAddress1 = NULL_ID;
        state.mut().excludeAddress2 = NULL_ID;

        // Staking
        state.mut().totalStaked = 0;
        state.mut().stakerCount = 0;
        state.mut().unstakeCount = 0;
        state.mut().stakingRewardPool = 0;
        state.mut().totalStakingRewardsDistributed = 0;
    }

    // Snapshot GGWP token holders AND SC shareholders at the start of each epoch
    struct BEGIN_EPOCH_locals
    {
        AssetPossessionIterator tokenIter;
        AssetPossessionIterator scIter;
        Asset scAsset;
        uint64 balance;
        id holder;
        uint64 existingBalance;
        // Staking reward distribution
        sint64 stakingIdx;
        uint64 rewardThisEpoch;
        uint64 stakerTokens;
        uint64 stakerReward;
        uint64 existingReward;
        uint64 quotient;
        uint64 remainder;
    };
    BEGIN_EPOCH_WITH_LOCALS()
    {
        // ---- Pass 1: GGWP Token holders (external token, issuer=MLMWPS...) ----
        state.mut().holderBalances.reset();
        state.mut().totalTokensSnapshot = 0;
        state.mut().holderCount = 0;

        if (state.get().wpToken.issuer != NULL_ID)
        {
            for (locals.tokenIter.begin(state.get().wpToken); !locals.tokenIter.reachedEnd(); locals.tokenIter.next())
            {
                if (locals.tokenIter.possessor() == SELF) continue;
                if (state.get().excludeAddress1 != NULL_ID && locals.tokenIter.possessor() == state.get().excludeAddress1) continue;
                if (state.get().excludeAddress2 != NULL_ID && locals.tokenIter.possessor() == state.get().excludeAddress2) continue;

                locals.balance = locals.tokenIter.numberOfPossessedShares();
                locals.holder = locals.tokenIter.possessor();

                if (locals.balance > 0)
                {
                    locals.existingBalance = 0;
                    state.get().holderBalances.get(locals.holder, locals.existingBalance);
                    locals.balance = sadd(locals.existingBalance, locals.balance);

                    // Dust filter: only include holders whose accumulated balance reaches
                    // the minimum. Keeps dust-spray addresses out of the distribution loop.
                    if (locals.balance < WOLFPACK_MIN_ELIGIBLE_BALANCE) continue;

                    if (state.mut().holderBalances.set(locals.holder, locals.balance) != NULL_INDEX)
                    {
                        state.mut().totalTokensSnapshot = sadd(state.get().totalTokensSnapshot, (uint64)locals.tokenIter.numberOfPossessedShares());
                        if (locals.existingBalance == 0)
                        {
                            state.mut().holderCount = state.get().holderCount + 1;
                        }
                    }
                }
            }
        }

        // ---- Pass 2: SC shareholders (IPO shares, issuer=NULL_ID, name="GGWP") ----
        state.mut().shareholderBalances.reset();
        state.mut().totalSharesSnapshot = 0;
        state.mut().shareholderCount = 0;

        locals.scAsset.issuer = NULL_ID;
        locals.scAsset.assetName = WOLFPACK_SC_ASSET_NAME;

        for (locals.scIter.begin(locals.scAsset); !locals.scIter.reachedEnd(); locals.scIter.next())
        {
            if (locals.scIter.possessor() == SELF) continue;
            if (state.get().excludeAddress1 != NULL_ID && locals.scIter.possessor() == state.get().excludeAddress1) continue;
            if (state.get().excludeAddress2 != NULL_ID && locals.scIter.possessor() == state.get().excludeAddress2) continue;

            locals.balance = locals.scIter.numberOfPossessedShares();
            locals.holder = locals.scIter.possessor();

            if (locals.balance > 0)
            {
                locals.existingBalance = 0;
                state.get().shareholderBalances.get(locals.holder, locals.existingBalance);
                locals.balance = sadd(locals.existingBalance, locals.balance);

                if (state.mut().shareholderBalances.set(locals.holder, locals.balance) != NULL_INDEX)
                {
                    state.mut().totalSharesSnapshot = sadd(state.get().totalSharesSnapshot, (uint64)locals.scIter.numberOfPossessedShares());
                    if (locals.existingBalance == 0)
                    {
                        state.mut().shareholderCount = state.get().shareholderCount + 1;
                    }
                }
            }
        }

        // ---- Staking reward distribution ----
        locals.rewardThisEpoch = WOLFPACK_STAKING_REWARD_PER_EPOCH;
        if (locals.rewardThisEpoch > state.get().stakingRewardPool)
        {
            locals.rewardThisEpoch = state.get().stakingRewardPool;
        }
        if (locals.rewardThisEpoch > 0 && state.get().totalStaked > 0)
        {
            for (locals.stakingIdx = state.get().stakedBalances.nextElementIndex(NULL_INDEX);
                 locals.stakingIdx != NULL_INDEX;
                 locals.stakingIdx = state.get().stakedBalances.nextElementIndex(locals.stakingIdx))
            {
                locals.holder = state.get().stakedBalances.key(locals.stakingIdx);
                locals.stakerTokens = state.get().stakedBalances.value(locals.stakingIdx);
                if (locals.stakerTokens == 0) continue;

                locals.quotient = div(locals.rewardThisEpoch, state.get().totalStaked);
                locals.remainder = mod(locals.rewardThisEpoch, state.get().totalStaked);
                locals.stakerReward = ((uint128)locals.quotient * (uint128)locals.stakerTokens).low + div((uint128)locals.remainder * (uint128)locals.stakerTokens, (uint128)state.get().totalStaked).low;
                if (locals.stakerReward == 0) continue;

                locals.existingReward = 0;
                state.get().pendingStakingRewards.get(locals.holder, locals.existingReward);
                state.mut().pendingStakingRewards.set(locals.holder, locals.existingReward + locals.stakerReward);
            }
            state.mut().stakingRewardPool = state.get().stakingRewardPool - locals.rewardThisEpoch;
            state.mut().totalStakingRewardsDistributed = state.get().totalStakingRewardsDistributed + locals.rewardThisEpoch;
        }
    }

    END_TICK()
    {
    }

    BEGIN_TICK()
    {
    }

    // Weekly revenue payout runs once per epoch in END_EPOCH (below).
    struct END_EPOCH_locals
    {
        uint64 amount;
        uint64 holderShare;
        uint64 shareholderShare;
        uint64 clanShare;
        uint64 reinvestShare;
        uint64 execReserveShare;
        sint64 idx;
        id holder;
        uint64 tokens;
        uint64 reward;
        uint64 rank;
        uint64 multiplier;
        Entity entity;
        uint64 contractBalance;
        uint64 quotient;
        uint64 remainder;
    };
    END_EPOCH_WITH_LOCALS()
    {
        // Compact hash maps once per epoch (previously done in END_EPOCH).
        state.mut().holderBalances.cleanupIfNeeded();
        state.mut().shareholderBalances.cleanupIfNeeded();
        state.mut().clanRanks.cleanupIfNeeded();
        state.mut().stakedBalances.cleanupIfNeeded();
        state.mut().unstakeAmounts.cleanupIfNeeded();
        state.mut().unstakeEpochs.cleanupIfNeeded();
        state.mut().pendingStakingRewards.cleanupIfNeeded();

        // Weekly revenue distribution: fires exactly once per epoch, using this
        // epoch's BEGIN_EPOCH holder/shareholder snapshot. No time/day gate needed.
        if (state.get().pendingRevenue == 0)
        {
            return;
        }

        // --- Step 1: Split revenue ---
        locals.amount = state.get().pendingRevenue;
        locals.holderShare = div((uint128)locals.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_HOLDERS, (uint128)1000ULL).low;
        locals.shareholderShare = div((uint128)locals.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_SHAREHOLDERS, (uint128)1000ULL).low;
        locals.clanShare = div((uint128)locals.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_CLAN, (uint128)1000ULL).low;
        locals.execReserveShare = div((uint128)locals.amount * (uint128)WOLFPACK_DISTRIBUTION_PERMILLE_EXEC_RESERVE, (uint128)1000ULL).low;
        // Execution-fee reserve: retained in the contract's own QU balance (never transferred out).
        // reinvestShare takes the remainder so all five portions sum to exactly `amount`.
        locals.reinvestShare = locals.amount - locals.holderShare - locals.shareholderShare - locals.clanShare - locals.execReserveShare;

        state.mut().pendingRevenue = 0;
        state.mut().totalDistributed = state.get().totalDistributed + locals.amount;
        state.mut().lastDistributionEpoch = qpi.epoch();
        state.mut().lastPayoutTick = qpi.tick();
        state.mut().reinvestmentFund = state.get().reinvestmentFund + locals.reinvestShare;
        state.mut().execReserveFund = state.get().execReserveFund + locals.execReserveShare;

        qpi.getEntity(SELF, locals.entity);
        locals.contractBalance = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        // --- Step 2: Push 70% to GGWP token holders ---
        if (locals.holderShare > 0 && state.get().totalTokensSnapshot > 0)
        {
            for (locals.idx = state.get().holderBalances.nextElementIndex(NULL_INDEX);
                 locals.idx != NULL_INDEX;
                 locals.idx = state.get().holderBalances.nextElementIndex(locals.idx))
            {
                locals.holder = state.get().holderBalances.key(locals.idx);
                locals.tokens = state.get().holderBalances.value(locals.idx);
                if (locals.tokens == 0) continue;

                locals.quotient = div(locals.holderShare, state.get().totalTokensSnapshot);
                locals.remainder = mod(locals.holderShare, state.get().totalTokensSnapshot);
                locals.reward = locals.quotient * locals.tokens + div((uint128)locals.remainder * (uint128)locals.tokens, (uint128)state.get().totalTokensSnapshot).low;
                if (locals.reward == 0) continue;
                if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;

                qpi.transfer(locals.holder, locals.reward);
                locals.contractBalance = locals.contractBalance - locals.reward;
                if (locals.contractBalance == 0) break;
            }
        }

        // --- Step 3: Push 10% to SC shareholders ---
        if (locals.shareholderShare > 0 && state.get().totalSharesSnapshot > 0)
        {
            if (locals.contractBalance == 0)
            {
                qpi.getEntity(SELF, locals.entity);
                locals.contractBalance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
            }

            for (locals.idx = state.get().shareholderBalances.nextElementIndex(NULL_INDEX);
                 locals.idx != NULL_INDEX;
                 locals.idx = state.get().shareholderBalances.nextElementIndex(locals.idx))
            {
                locals.holder = state.get().shareholderBalances.key(locals.idx);
                locals.tokens = state.get().shareholderBalances.value(locals.idx);
                if (locals.tokens == 0) continue;

                locals.quotient = div(locals.shareholderShare, state.get().totalSharesSnapshot);
                locals.remainder = mod(locals.shareholderShare, state.get().totalSharesSnapshot);
                locals.reward = locals.quotient * locals.tokens + div((uint128)locals.remainder * (uint128)locals.tokens, (uint128)state.get().totalSharesSnapshot).low;
                if (locals.reward == 0) continue;
                if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;

                qpi.transfer(locals.holder, locals.reward);
                locals.contractBalance = locals.contractBalance - locals.reward;
                if (locals.contractBalance == 0) break;
            }
        }

        // --- Step 4: Push 10% to clan members ---
        if (locals.clanShare > 0 && state.get().clanWeightedTotal > 0)
        {
            if (locals.contractBalance == 0)
            {
                qpi.getEntity(SELF, locals.entity);
                locals.contractBalance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
            }

            for (locals.idx = state.get().clanRanks.nextElementIndex(NULL_INDEX);
                 locals.idx != NULL_INDEX;
                 locals.idx = state.get().clanRanks.nextElementIndex(locals.idx))
            {
                locals.holder = state.get().clanRanks.key(locals.idx);
                locals.rank = state.get().clanRanks.value(locals.idx);

                locals.multiplier = WOLFPACK_RANK_MULTIPLIER_0;
                if (locals.rank == 1) locals.multiplier = WOLFPACK_RANK_MULTIPLIER_1;
                if (locals.rank == 2) locals.multiplier = WOLFPACK_RANK_MULTIPLIER_2;
                if (locals.rank == 3) locals.multiplier = WOLFPACK_RANK_MULTIPLIER_3;
                if (locals.rank == 4) locals.multiplier = WOLFPACK_RANK_MULTIPLIER_4;
                if (locals.rank == 5) locals.multiplier = WOLFPACK_RANK_MULTIPLIER_5;

                locals.quotient = div(locals.clanShare, state.get().clanWeightedTotal);
                locals.remainder = mod(locals.clanShare, state.get().clanWeightedTotal);
                locals.reward = locals.quotient * locals.multiplier + div((uint128)locals.remainder * (uint128)locals.multiplier, (uint128)state.get().clanWeightedTotal).low;
                if (locals.reward == 0) continue;
                if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;

                qpi.transfer(locals.holder, locals.reward);
                locals.contractBalance = locals.contractBalance - locals.reward;
                if (locals.contractBalance == 0) break;
            }
        }

        // --- Step 5: Push 10% reinvestment share to the reinvest address ---
        if (locals.reinvestShare > 0 && state.get().reinvestAddress != NULL_ID)
        {
            if (locals.contractBalance == 0)
            {
                qpi.getEntity(SELF, locals.entity);
                locals.contractBalance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
            }
            locals.reward = locals.reinvestShare;
            if (locals.reward > locals.contractBalance) locals.reward = locals.contractBalance;
            if (locals.reward > 0)
            {
                qpi.transfer(state.get().reinvestAddress, locals.reward);
            }
        }
    }

    PRE_ACQUIRE_SHARES()
    {
        // Accept management rights transfer from QX for GGWP tokens only.
        // This enables users to stake by first calling QX.TransferShareManagementRights.
        if (input.asset.assetName == state.get().wpToken.assetName
            && input.asset.issuer == state.get().wpToken.issuer
            && input.otherContractIndex == WOLFPACK_QX_CONTRACT_INDEX)
        {
            output.allowTransfer = true;
            output.requestedFee = 0;
        }
    }
};
