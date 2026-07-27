using namespace QPI;

constexpr uint64 QEARN_MINIMUM_LOCKING_AMOUNT = 10000000;
constexpr uint64 QEARN_MAX_LOCKS = 4194304;
constexpr uint64 QEARN_MAX_EPOCHS = 4096;
constexpr uint64 QEARN_MAX_USERS = 131072;
constexpr uint64 QEARN_MAX_LOCK_AMOUNT = 1000000000000ULL;
constexpr uint64 QEARN_MAX_BONUS_AMOUNT = 1000000000000ULL;
constexpr uint64 QEARN_INITIAL_EPOCH = 138;
// V2 release epoch. This must match the PADDING epoch in contract_def.h,
// and the upgraded binary must be active before BEGIN_EPOCH of this epoch.
constexpr uint64 QEARN_V2_ACTIVATION_EPOCH = 227;

constexpr uint32 QEARN_V2_LOCK_PERIOD_13 = 13;
constexpr uint32 QEARN_V2_LOCK_PERIOD_26 = 26;
constexpr uint32 QEARN_V2_LOCK_PERIOD_52 = 52;
constexpr uint64 QEARN_V2_LAST_LOCK_EPOCH =
    QEARN_MAX_EPOCHS - QEARN_V2_LOCK_PERIOD_52 - 1;
constexpr uint32 QEARN_V2_TERM_INDEX_13 = 0;
constexpr uint32 QEARN_V2_TERM_INDEX_26 = 1;
constexpr uint32 QEARN_V2_TERM_INDEX_52 = 2;
constexpr uint32 QEARN_V2_NUMBER_OF_TERMS = 3;
constexpr uint32 QEARN_V2_TERM_ARRAY_CAPACITY = 4;
constexpr uint64 QEARN_V2_MAX_FULLY_UNLOCK_RECORDS = QEARN_MAX_USERS * 4;

constexpr uint64 QEARN_V2_ALLOCATION_PERCENT_13 = 10;
constexpr uint64 QEARN_V2_ALLOCATION_PERCENT_26 = 20;
constexpr uint64 QEARN_V2_RETURN_PERCENT_13 = 3;
constexpr uint64 QEARN_V2_RETURN_PERCENT_26 = 6;
constexpr uint64 QEARN_V2_RETURN_PERCENT_52 = 18;
constexpr uint64 QEARN_V2_RETURN_SCALE = 10000000ULL;

constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_0_3 = 0;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_4_7 = 5;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_8_11 = 5;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_12_15 = 10;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_16_19 = 15;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_20_23 = 20;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_24_27 = 25;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_28_31 = 30;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_32_35 = 35;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_36_39 = 40;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_40_43 = 45;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_44_47 = 50;
constexpr uint64 QEARN_EARLY_UNLOCKING_PERCENT_48_51 = 55;

constexpr uint64 QEARN_BURN_PERCENT_0_3 = 0;
constexpr uint64 QEARN_BURN_PERCENT_4_7 = 45;
constexpr uint64 QEARN_BURN_PERCENT_8_11 = 45;
constexpr uint64 QEARN_BURN_PERCENT_12_15 = 45;
constexpr uint64 QEARN_BURN_PERCENT_16_19 = 40;
constexpr uint64 QEARN_BURN_PERCENT_20_23 = 40;
constexpr uint64 QEARN_BURN_PERCENT_24_27 = 35;
constexpr uint64 QEARN_BURN_PERCENT_28_31 = 35;
constexpr uint64 QEARN_BURN_PERCENT_32_35 = 35;
constexpr uint64 QEARN_BURN_PERCENT_36_39 = 30;
constexpr uint64 QEARN_BURN_PERCENT_40_43 = 30;
constexpr uint64 QEARN_BURN_PERCENT_44_47 = 30;
constexpr uint64 QEARN_BURN_PERCENT_48_51 = 25;

constexpr sint32 QEARN_INVALID_INPUT_AMOUNT = 0;
constexpr sint32 QEARN_LOCK_SUCCESS = 1;
constexpr sint32 QEARN_INVALID_INPUT_LOCKED_EPOCH = 2;
constexpr sint32 QEARN_INVALID_INPUT_UNLOCK_AMOUNT = 3;
constexpr sint32 QEARN_EMPTY_LOCKED = 4;
constexpr sint32 QEARN_UNLOCK_SUCCESS = 5;
constexpr sint32 QEARN_OVERFLOW_USER = 6;
constexpr sint32 QEARN_LIMIT_LOCK = 7;
constexpr sint32 QEARN_INVALID_LOCK_PERIOD = 8;
constexpr sint32 QEARN_V2_NOT_ACTIVE = 9;
constexpr sint32 QEARN_V2_POSITION_MATURED = 10;
constexpr sint32 QEARN_TRANSFER_FAILED = 11;

enum QEARNLogInfo {
    QearnSuccessLocking = 0,
    QearnFailedTransfer = 1,
    QearnLimitLocking = 2,
    QearnOverflowUser = 3,
    QearnInvalidInput = 4,
    QearnSuccessEarlyUnlocking = 5,
    QearnSuccessFullyUnlocking = 6,
};
struct QEARNLogger
{
    uint32 _contractIndex;
    id sourcePublicKey;
    id destinationPublicKey;
    sint64 amount;
    uint32 _type;
    sint8 _terminator;
};

struct QEARN2
{
};

struct QEARN : public ContractBase
{
    // Types (defined before StateData so they are visible in StateData and procedures)

    struct RoundInfo {

        uint64 _totalLockedAmount;            // The initial total locked amount in any epoch.  Max Epoch is 65535
        uint64 _epochBonusAmount;             // The initial bonus amount per an epoch.         Max Epoch is 65535

    };

    struct EpochIndexInfo {

        uint32 startIndex;
        uint32 endIndex;
    };

    struct LockInfo {

        uint64 _lockedAmount;
        id ID;
        uint32 _lockedEpoch;

    };

    struct HistoryInfo {

        uint64 _unlockedAmount;
        uint64 _rewardedAmount;
        id _unlockedID;

    };

    struct StatsInfo {

        uint64 burnedAmount;
        uint64 boostedAmount;
        uint64 rewardedAmount;

    };

    struct V2TermInfo
    {
        uint64 initialLockedAmount;
        uint64 currentLockedAmount;
        uint64 earlyUnlockedAmount;
        uint64 initialRewardPool;
        uint64 currentRewardPool;
        uint64 rewardedAmount;
        uint64 forfeitedClaimAmount;
    };

    struct V2EpochInfo
    {
        Array<V2TermInfo, QEARN_V2_TERM_ARRAY_CAPACITY> terms;
        uint64 bonusAmount;
        uint64 rewardedAmount;
        uint64 forfeitedClaimAmount;
        uint64 transferredTo52Amount;
        // Unique V2 reward funds transferred out of QEarn to CCF.
        uint64 transferredToCCFAmount;
        uint32 finalized;
    };

    // State data
    struct StateData
    {
        Array<RoundInfo, QEARN_MAX_EPOCHS> _initialRoundInfo;
        Array<RoundInfo, QEARN_MAX_EPOCHS> _currentRoundInfo;
        Array<EpochIndexInfo, QEARN_MAX_EPOCHS> _epochIndex;
        Array<LockInfo, QEARN_MAX_LOCKS> locker;
        Array<HistoryInfo, QEARN_MAX_USERS> earlyUnlocker;
        Array<HistoryInfo, QEARN_MAX_USERS> fullyUnlocker;
        uint32 _earlyUnlockedCnt;
        uint32 _fullyUnlockedCnt;
        Array<StatsInfo, QEARN_MAX_EPOCHS> statsInfo;

        // Appended V2 state. Keeping every V1 field above byte-for-byte unchanged
        // allows the activation upgrade to use PADDING and preserve all V1 positions.
        Array<uint8, QEARN_MAX_LOCKS> lockerLockPeriods;
        Array<V2EpochInfo, QEARN_MAX_EPOCHS> v2EpochInfo;
        // A user may have one maturity in each V2 term during the same epoch.
        // Keep these records separate from the fixed-size V1 history array.
        Array<HistoryInfo, QEARN_V2_MAX_FULLY_UNLOCK_RECORDS> v2FullyUnlocker;
        uint32 v2FullyUnlockedCnt;
    };

public:
    struct getLockInfoPerEpoch_input {
		uint32 Epoch;                             /* epoch number to get information */
    };

    struct getLockInfoPerEpoch_output {
        // For V2 epochs this legacy endpoint reports the 52-epoch term only.
        uint64 lockedAmount;                      /* initial locked amount */
        uint64 bonusAmount;                       /* initial bonus amount */
        uint64 currentLockedAmount;               /* amount excluding early unlocks */
        uint64 currentBonusAmount;                /* bonus excluding early unlock effects */
        uint64 yield;                             /* V1 yield / V2 52-term return, scaled by 10000000 (not APY) */
    };

    struct getUserLockedInfo_input {
        id user;
        uint32 epoch;
    };

    struct getUserLockedInfo_output {
        uint64 lockedAmount;                   /* the amount user locked at input.epoch */
    };

    /*
        getStateOfRound FUNCTION

        getStateOfRound function returns following.

        0 = open epoch,not started yet
        1 = running epoch
        2 = ended epoch(>52weeks)
    */
    struct getStateOfRound_input {
        uint32 epoch;
    };

    struct getStateOfRound_output {
        uint32 state;
    };

    /*
        getUserLockStatus FUNCTION

        the status will return the binary status.
        1101010010110101001011010100101101010010110101001001

        1 means locked in [index of 1] weeks ago. 0 means unlocked in [index of zero] weeks ago.
        The frontend can get the status of locked in 52 epochs. in above binary status,
        the frontend can know that user locked 0 week ago, 1 week ago, 3 weeks ago, 5, 8,10,11,13 weeks ago.
    */
    struct getUserLockStatus_input {
        id user;
    };

    struct getUserLockStatus_output {
        uint64 status;
    };

    /*
        getEndedStatus FUNCTION

        output.earlyRewardedAmount returns the amount rewarded by unlocking early at current epoch
        output.earlyUnlockedAmount returns the amount unlocked by unlocking early at current epoch
        output.fullyRewardedAmount returns the amount rewarded by unlocking fully at the end of previous epoch
        output.fullyUnlockedAmount returns the amount unlocked by unlocking fully at the end of previous epoch

        let's assume that current epoch is 170, user unlocked the 15B qu totally at this epoch and he got the 30B qu of reward.
        in this case, output.earlyUnlockedAmount = 15B qu, output.earlyRewardedAmount = 30B qu
        if this user unlocks 3B qu additionally at this epoch and rewarded 6B qu,
        in this case, output.earlyUnlockedAmount = 18B qu, output.earlyRewardedAmount = 36B qu
        state.earlyUnlocker array would be initialized at the end of every epoch

        let's assume also that current epoch is 170, user got the 15B(locked amount for 52 weeks) + 10B(rewarded amount for 52 weeks) at the end of epoch 169.
        in this case, output.fullyRewardedAmount = 10B, output.fullyUnlockedAmount = 15B
        state.fullyUnlocker array would be decided with distributions at the end of every epoch

        state.earlyUnlocker, state.fullyUnlocker arrays would be initialized and decided by following expression at the END_EPOCH_WITH_LOCALS function.
        state._earlyUnlockedCnt = 0;
        state._fullyUnlockedCnt = 0;
    */

    struct getEndedStatus_input {
        id user;
    };

    struct getEndedStatus_output {
        uint64 fullyUnlockedAmount;
        uint64 fullyRewardedAmount;
        uint64 earlyUnlockedAmount;
        uint64 earlyRewardedAmount;
    };

	struct lock_input {
    };

    struct lock_output {
        sint32 returnCode;
    };

    struct unlock_input {
        uint64 amount;                            /* unlocking amount */
        uint32 lockedEpoch;                      /* locked epoch */
    };

    struct unlock_output {
        sint32 returnCode;
    };

    struct lockV2_input
    {
        // One of 13, 26, or 52. Each user/epoch/term is one position.
        uint32 lockPeriod;
    };

    struct lockV2_output
    {
        sint32 returnCode;
    };

    struct unlockV2_input
    {
        uint32 lockedEpoch;
        uint32 lockPeriod;
    };

    struct unlockV2_output
    {
        sint32 returnCode;
    };

    struct getV2LockInfoPerEpoch_input
    {
        uint32 epoch;
        uint32 lockPeriod;
    };

    struct getV2LockInfoPerEpoch_output
    {
        uint64 initialLockedAmount;
        uint64 currentLockedAmount;
        uint64 termEarlyUnlockedAmount;
        uint64 initialRewardPool;
        uint64 currentRewardPool;
        uint64 termRewardedAmount;
        // Cumulative abandoned reward claims. For the 52-epoch term, retained
        // rewards can be abandoned again and this is not a unique-funds counter.
        uint64 termForfeitedClaimAmount;
        // Total term return scaled by 10,000,000. This is not APY.
        uint64 currentTermReturn;
        uint64 maturityEpoch;
        uint64 epochBonusAmount;
        uint64 epochRewardedAmount;
        // Cumulative abandoned reward claims across the three terms.
        uint64 epochForfeitedClaimAmount;
        uint64 epochTransferredTo52Amount;
        // Initial/dynamic cap surplus and final 52-term settlement remainder.
        uint64 epochTransferredToCCFAmount;
        // 0 while the lock epoch is open; 1 after its pools are allocated.
        uint32 finalized;
        // 0 = future, 1 = open/running, 2 = matured.
        uint32 state;
        sint32 returnCode;
    };

    struct getV2UserLockedInfo_input
    {
        id user;
        uint32 epoch;
        uint32 lockPeriod;
    };

    struct getV2UserLockedInfo_output
    {
        uint64 lockedAmount;
        uint64 maturityEpoch;
        sint32 returnCode;
    };

    struct getStatsPerEpoch_input {
        uint32 epoch;
    };

    struct getStatsPerEpoch_output {

        uint64 earlyUnlockedAmount;
        uint64 earlyUnlockedPercent;
        uint64 totalLockedAmount;
        uint64 averageAPY; // Legacy name: V2 reports average active 52-term return, not APY.

    };

    struct getBurnedAndBoostedStats_input {

    };

    struct getBurnedAndBoostedStats_output {

        uint64 burnedAmount;
        uint64 averageBurnedPercent;
        uint64 boostedAmount;
        uint64 averageBoostedPercent;
        uint64 rewardedAmount;
        uint64 averageRewardedPercent;

    };

    struct getBurnedAndBoostedStatsPerEpoch_input {
        uint32 epoch;
    };

    struct getBurnedAndBoostedStatsPerEpoch_output {

        uint64 burnedAmount;
        uint64 burnedPercent;
        uint64 boostedAmount;
        uint64 boostedPercent;
        uint64 rewardedAmount;
        uint64 rewardedPercent;

    };

protected:

    struct _RemoveGapsInLockerArray_input
    {
    };

    struct _RemoveGapsInLockerArray_output
    {
    };

    struct _RemoveGapsInLockerArray_locals
    {
        EpochIndexInfo tmpEpochIndex;
        LockInfo INITIALIZE_USER;
        uint32 epoch;
        uint32 readIndex;
        uint32 writeIndex;
        uint32 oldStartIndex;
        uint32 oldEndIndex;
        uint32 startEpoch;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_RemoveGapsInLockerArray)
    {
        // Determine the actual start epoch (ensure it's at least QEARN_INITIAL_EPOCH)
        locals.startEpoch = qpi.epoch() - 52;
        if (locals.startEpoch < QEARN_INITIAL_EPOCH)
        {
            locals.startEpoch = QEARN_INITIAL_EPOCH;
        }
        locals.INITIALIZE_USER.ID = NULL_ID;
        locals.INITIALIZE_USER._lockedAmount = 0;
        locals.INITIALIZE_USER._lockedEpoch = 0;

        // Remove all gaps in Locker array and update epochIndex
        locals.tmpEpochIndex.startIndex = 0;
        for (locals.epoch = locals.startEpoch; locals.epoch <= qpi.epoch(); locals.epoch++)
        {
            locals.oldStartIndex = state.get()._epochIndex.get(locals.epoch).startIndex;
            locals.oldEndIndex = state.get()._epochIndex.get(locals.epoch).endIndex;
            locals.writeIndex = locals.tmpEpochIndex.startIndex;
            ASSERT(locals.oldStartIndex <= locals.oldEndIndex);
            ASSERT(locals.writeIndex <= locals.oldStartIndex);

            for (locals.readIndex = locals.oldStartIndex;
                locals.readIndex < locals.oldEndIndex;
                locals.readIndex++)
            {
                if (!state.get().locker.get(locals.readIndex)._lockedAmount)
                {
                    state.mut().lockerLockPeriods.set(locals.readIndex, 0);
                    continue;
                }

                if (locals.writeIndex != locals.readIndex)
                {
                    state.mut().locker.set(locals.writeIndex, state.get().locker.get(locals.readIndex));
                    state.mut().lockerLockPeriods.set(
                        locals.writeIndex,
                        state.get().lockerLockPeriods.get(locals.readIndex));

                    state.mut().locker.set(locals.readIndex, locals.INITIALIZE_USER);
                    state.mut().lockerLockPeriods.set(locals.readIndex, 0);
                }
                locals.writeIndex++;
            }

            locals.tmpEpochIndex.endIndex = locals.writeIndex;
            state.mut()._epochIndex.set(locals.epoch, locals.tmpEpochIndex);

            locals.tmpEpochIndex.startIndex = locals.tmpEpochIndex.endIndex;
        }

        if (qpi.epoch() + 1 < QEARN_MAX_EPOCHS)
        {
            locals.tmpEpochIndex.endIndex = locals.tmpEpochIndex.startIndex;
            state.mut()._epochIndex.set(qpi.epoch() + 1, locals.tmpEpochIndex);
        }
    }

    struct getStateOfRound_locals {
        uint32 firstEpoch;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getStateOfRound)
    {
        if(input.epoch < QEARN_INITIAL_EPOCH)
        {                                                            // non staking
            output.state = 2;
            return ;
        }
        if(input.epoch > qpi.epoch())
        {
            output.state = 0;                                     // opening round, not started yet
        }
        locals.firstEpoch = qpi.epoch() - 52;
        if(input.epoch <= qpi.epoch() && input.epoch >= locals.firstEpoch)
        {
            output.state = 1;       // running round, available unlocking early
        }
        if(input.epoch < locals.firstEpoch)
        {
            output.state = 2;       // ended round
        }
    }

    struct getLockInfoPerEpoch_locals
    {
        V2TermInfo termInfo;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getLockInfoPerEpoch)
    {
        if (input.Epoch >= QEARN_MAX_EPOCHS)
        {
            return;
        }
        if (input.Epoch >= QEARN_V2_ACTIVATION_EPOCH)
        {
            // Procedure 1 becomes a 52-epoch V2 lock after activation. Keep its
            // legacy query paired with that term for QVAULT/QBOND compatibility.
            locals.termInfo = state.get().v2EpochInfo.get(input.Epoch).terms.get(QEARN_V2_TERM_INDEX_52);
            output.bonusAmount = locals.termInfo.initialRewardPool;
            output.lockedAmount = locals.termInfo.initialLockedAmount;
            output.currentBonusAmount = locals.termInfo.currentRewardPool;
            output.currentLockedAmount = locals.termInfo.currentLockedAmount;
            if (!output.currentLockedAmount
                && input.Epoch + QEARN_V2_LOCK_PERIOD_52 <= qpi.epoch())
            {
                ASSERT(locals.termInfo.initialLockedAmount >= locals.termInfo.earlyUnlockedAmount);
                output.currentLockedAmount =
                    locals.termInfo.initialLockedAmount - locals.termInfo.earlyUnlockedAmount;
                output.currentBonusAmount = locals.termInfo.rewardedAmount;
            }
            if (output.currentLockedAmount)
            {
                output.yield = div(output.currentBonusAmount * QEARN_V2_RETURN_SCALE, output.currentLockedAmount);
                if (output.yield > QEARN_V2_RETURN_PERCENT_52 * 100000ULL)
                {
                    output.yield = QEARN_V2_RETURN_PERCENT_52 * 100000ULL;
                }
            }
            else
            {
                output.yield = 0;
            }
            return;
        }

        output.bonusAmount = state.get()._initialRoundInfo.get(input.Epoch)._epochBonusAmount;
        output.lockedAmount = state.get()._initialRoundInfo.get(input.Epoch)._totalLockedAmount;
        output.currentBonusAmount = state.get()._currentRoundInfo.get(input.Epoch)._epochBonusAmount;
        output.currentLockedAmount = state.get()._currentRoundInfo.get(input.Epoch)._totalLockedAmount;
        if(state.get()._currentRoundInfo.get(input.Epoch)._totalLockedAmount)
        {
            output.yield = div(state.get()._currentRoundInfo.get(input.Epoch)._epochBonusAmount * 10000000ULL, state.get()._currentRoundInfo.get(input.Epoch)._totalLockedAmount);
        }
        else
        {
            output.yield = 0ULL;
        }
    }

    struct getStatsPerEpoch_locals
    {
        Entity entity;
        V2EpochInfo epochInfo;
        V2TermInfo termInfo;
        uint64 initialLockedAmount;
        uint32 cnt;
        uint32 termIndex;
        uint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getStatsPerEpoch)
    {
        if (input.epoch >= QEARN_MAX_EPOCHS)
        {
            return;
        }
        if (input.epoch >= QEARN_V2_ACTIVATION_EPOCH)
        {
            locals.epochInfo = state.get().v2EpochInfo.get(input.epoch);
            for (locals.termIndex = 0;
                locals.termIndex < QEARN_V2_NUMBER_OF_TERMS;
                locals.termIndex++)
            {
                locals.termInfo = locals.epochInfo.terms.get(locals.termIndex);
                locals.initialLockedAmount += locals.termInfo.initialLockedAmount;
                output.earlyUnlockedAmount += locals.termInfo.earlyUnlockedAmount;
            }
        }
        else
        {
            locals.initialLockedAmount =
                state.get()._initialRoundInfo.get(input.epoch)._totalLockedAmount;
            output.earlyUnlockedAmount = locals.initialLockedAmount
                - state.get()._currentRoundInfo.get(input.epoch)._totalLockedAmount;
        }
        if (locals.initialLockedAmount)
        {
            output.earlyUnlockedPercent =
                div(output.earlyUnlockedAmount * 10000ULL, locals.initialLockedAmount);
        }

        qpi.getEntity(SELF, locals.entity);
        output.totalLockedAmount = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        // This field is retained for ABI compatibility. For V2 rounds it is the
        // average active 52-epoch term return, scaled by 10,000,000, not APY.
        output.averageAPY = 0;
        locals.cnt = 0;

        for(locals._t = qpi.epoch() - 1U; locals._t >= qpi.epoch() - 52U; locals._t--)
        {
            if(locals._t < QEARN_INITIAL_EPOCH)
            {
                break;
            }
            if (locals._t >= QEARN_V2_ACTIVATION_EPOCH)
            {
                locals.termInfo = state.get().v2EpochInfo.get(locals._t).terms.get(QEARN_V2_TERM_INDEX_52);
                if (!locals.termInfo.currentLockedAmount)
                {
                    continue;
                }
                output.averageAPY += div(
                    locals.termInfo.currentRewardPool * QEARN_V2_RETURN_SCALE,
                    locals.termInfo.currentLockedAmount);
            }
            else
            {
                if (!state.get()._currentRoundInfo.get(locals._t)._totalLockedAmount)
                {
                    continue;
                }
                output.averageAPY += div(
                    state.get()._currentRoundInfo.get(locals._t)._epochBonusAmount * QEARN_V2_RETURN_SCALE,
                    state.get()._currentRoundInfo.get(locals._t)._totalLockedAmount);
            }
            locals.cnt++;
        }

        if (locals.cnt)
        {
            output.averageAPY = div(output.averageAPY, locals.cnt * 1ULL);
        }
    }

    struct getBurnedAndBoostedStats_locals
    {
        uint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getBurnedAndBoostedStats)
    {
        output.boostedAmount = 0;
        output.burnedAmount = 0;
        output.rewardedAmount = 0;
        output.averageBurnedPercent = 0;
        output.averageBoostedPercent = 0;
        output.averageRewardedPercent = 0;

        for(locals._t = 138; locals._t < qpi.epoch(); locals._t++)
        {
            output.boostedAmount += state.get().statsInfo.get(locals._t).boostedAmount;
            output.burnedAmount += state.get().statsInfo.get(locals._t).burnedAmount;
            output.rewardedAmount += state.get().statsInfo.get(locals._t).rewardedAmount;

            output.averageBurnedPercent += div(state.get().statsInfo.get(locals._t).burnedAmount * 10000000, state.get()._initialRoundInfo.get(locals._t)._epochBonusAmount);
            output.averageBoostedPercent += div(state.get().statsInfo.get(locals._t).boostedAmount * 10000000, state.get()._initialRoundInfo.get(locals._t)._epochBonusAmount);
            output.averageRewardedPercent += div(state.get().statsInfo.get(locals._t).rewardedAmount * 10000000, state.get()._initialRoundInfo.get(locals._t)._epochBonusAmount);
        }

        output.averageBurnedPercent = div(output.averageBurnedPercent, qpi.epoch() - 138ULL);
        output.averageBoostedPercent = div(output.averageBoostedPercent, qpi.epoch() - 138ULL);
        output.averageRewardedPercent = div(output.averageRewardedPercent, qpi.epoch() - 138ULL);

    }

    PUBLIC_FUNCTION(getBurnedAndBoostedStatsPerEpoch)
    {
        output.boostedAmount = state.get().statsInfo.get(input.epoch).boostedAmount;
        output.burnedAmount = state.get().statsInfo.get(input.epoch).burnedAmount;
        output.rewardedAmount = state.get().statsInfo.get(input.epoch).rewardedAmount;

        output.burnedPercent = div(state.get().statsInfo.get(input.epoch).burnedAmount * 10000000, state.get()._initialRoundInfo.get(input.epoch)._epochBonusAmount);
        output.boostedPercent = div(state.get().statsInfo.get(input.epoch).boostedAmount * 10000000, state.get()._initialRoundInfo.get(input.epoch)._epochBonusAmount);
        output.rewardedPercent = div(state.get().statsInfo.get(input.epoch).rewardedAmount * 10000000, state.get()._initialRoundInfo.get(input.epoch)._epochBonusAmount);

    }

    struct getUserLockedInfo_locals {
        uint32 _t;
        uint32 startIndex;
        uint32 endIndex;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getUserLockedInfo)
    {
        if (input.epoch >= QEARN_MAX_EPOCHS)
        {
            return;
        }
        locals.startIndex = state.get()._epochIndex.get(input.epoch).startIndex;
        locals.endIndex = state.get()._epochIndex.get(input.epoch).endIndex;

        for(locals._t = locals.startIndex; locals._t < locals.endIndex; locals._t++)
        {
            if(state.get().locker.get(locals._t).ID == input.user
                && (input.epoch < QEARN_V2_ACTIVATION_EPOCH
                    || state.get().lockerLockPeriods.get(locals._t) == QEARN_V2_LOCK_PERIOD_52))
            {
                output.lockedAmount = state.get().locker.get(locals._t)._lockedAmount;
                return;
            }
        }
    }

    struct getV2LockInfoPerEpoch_locals
    {
        V2EpochInfo epochInfo;
        V2TermInfo termInfo;
        uint32 termIndex;
        uint64 returnCap;
        uint64 maturedLockedAmount;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getV2LockInfoPerEpoch)
    {
        output.returnCode = QEARN_INVALID_LOCK_PERIOD;
        if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_13)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_13;
            locals.returnCap = QEARN_V2_RETURN_PERCENT_13 * 100000ULL;
        }
        else if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_26)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_26;
            locals.returnCap = QEARN_V2_RETURN_PERCENT_26 * 100000ULL;
        }
        else if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_52)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_52;
            locals.returnCap = QEARN_V2_RETURN_PERCENT_52 * 100000ULL;
        }
        else
        {
            return;
        }

        if (input.epoch < QEARN_V2_ACTIVATION_EPOCH || input.epoch >= QEARN_MAX_EPOCHS)
        {
            output.returnCode = QEARN_INVALID_INPUT_LOCKED_EPOCH;
            return;
        }

        locals.epochInfo = state.get().v2EpochInfo.get(input.epoch);
        locals.termInfo = locals.epochInfo.terms.get(locals.termIndex);
        output.initialLockedAmount = locals.termInfo.initialLockedAmount;
        output.currentLockedAmount = locals.termInfo.currentLockedAmount;
        output.termEarlyUnlockedAmount = locals.termInfo.earlyUnlockedAmount;
        output.initialRewardPool = locals.termInfo.initialRewardPool;
        output.currentRewardPool = locals.termInfo.currentRewardPool;
        output.termRewardedAmount = locals.termInfo.rewardedAmount;
        output.termForfeitedClaimAmount = locals.termInfo.forfeitedClaimAmount;
        output.maturityEpoch = input.epoch + input.lockPeriod;
        output.epochBonusAmount = locals.epochInfo.bonusAmount;
        output.epochRewardedAmount = locals.epochInfo.rewardedAmount;
        output.epochForfeitedClaimAmount = locals.epochInfo.forfeitedClaimAmount;
        output.epochTransferredTo52Amount = locals.epochInfo.transferredTo52Amount;
        output.epochTransferredToCCFAmount = locals.epochInfo.transferredToCCFAmount;
        output.finalized = locals.epochInfo.finalized;

        if (output.currentLockedAmount)
        {
            if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_52)
            {
                output.currentTermReturn = div(output.currentRewardPool * QEARN_V2_RETURN_SCALE, output.currentLockedAmount);
            }
            else if (output.initialLockedAmount)
            {
                output.currentTermReturn = div(output.initialRewardPool * QEARN_V2_RETURN_SCALE, output.initialLockedAmount);
            }
        }
        else if (qpi.epoch() >= output.maturityEpoch)
        {
            ASSERT(output.initialLockedAmount >= output.termEarlyUnlockedAmount);
            locals.maturedLockedAmount =
                output.initialLockedAmount - output.termEarlyUnlockedAmount;
            if (locals.maturedLockedAmount)
            {
                output.currentTermReturn = div(
                    output.termRewardedAmount * QEARN_V2_RETURN_SCALE,
                    locals.maturedLockedAmount);
            }
        }

        if (output.currentTermReturn > locals.returnCap)
        {
            output.currentTermReturn = locals.returnCap;
        }

        if (input.epoch > qpi.epoch())
        {
            output.state = 0;
        }
        else if (qpi.epoch() < output.maturityEpoch
            || (qpi.epoch() == output.maturityEpoch && output.currentLockedAmount))
        {
            output.state = 1;
        }
        else
        {
            output.state = 2;
        }
        output.returnCode = QEARN_LOCK_SUCCESS;
    }

    struct getV2UserLockedInfo_locals
    {
        uint32 t;
        uint32 startIndex;
        uint32 endIndex;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getV2UserLockedInfo)
    {
        output.returnCode = QEARN_INVALID_LOCK_PERIOD;
        if (input.lockPeriod != QEARN_V2_LOCK_PERIOD_13
            && input.lockPeriod != QEARN_V2_LOCK_PERIOD_26
            && input.lockPeriod != QEARN_V2_LOCK_PERIOD_52)
        {
            return;
        }
        if (input.epoch < QEARN_V2_ACTIVATION_EPOCH || input.epoch >= QEARN_MAX_EPOCHS)
        {
            output.returnCode = QEARN_INVALID_INPUT_LOCKED_EPOCH;
            return;
        }

        output.maturityEpoch = input.epoch + input.lockPeriod;
        locals.startIndex = state.get()._epochIndex.get(input.epoch).startIndex;
        locals.endIndex = state.get()._epochIndex.get(input.epoch).endIndex;
        for (locals.t = locals.startIndex; locals.t < locals.endIndex; locals.t++)
        {
            if (state.get().locker.get(locals.t).ID == input.user
                && state.get().lockerLockPeriods.get(locals.t) == input.lockPeriod)
            {
                output.lockedAmount = state.get().locker.get(locals.t)._lockedAmount;
                break;
            }
        }
        output.returnCode = QEARN_LOCK_SUCCESS;
    }

    struct getUserLockStatus_locals {
        uint64 bn;
        uint32 _t;
        uint32 _r;
        uint32 endIndex;
        uint8 lockedWeeks;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getUserLockStatus)
    {
        output.status = 0ULL;
        locals.endIndex = state.get()._epochIndex.get(qpi.epoch()).endIndex;

        for(locals._t = 0; locals._t < locals.endIndex; locals._t++)
        {
            if(state.get().locker.get(locals._t)._lockedAmount > 0 && state.get().locker.get(locals._t).ID == input.user)
            {

                locals.lockedWeeks = qpi.epoch() - state.get().locker.get(locals._t)._lockedEpoch;
                locals.bn = 1ULL<<locals.lockedWeeks;

                output.status |= locals.bn;
            }
        }

    }

    struct getEndedStatus_locals {
        uint32 _t;
        uint32 _v2t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getEndedStatus)
    {
        output.earlyRewardedAmount = 0;
        output.earlyUnlockedAmount = 0;
        output.fullyRewardedAmount = 0;
        output.fullyUnlockedAmount = 0;

        for(locals._t = 0; locals._t < state.get()._earlyUnlockedCnt; locals._t++)
        {
            if(state.get().earlyUnlocker.get(locals._t)._unlockedID == input.user)
            {
                output.earlyRewardedAmount += state.get().earlyUnlocker.get(locals._t)._rewardedAmount;
                output.earlyUnlockedAmount += state.get().earlyUnlocker.get(locals._t)._unlockedAmount;
                break;
            }
        }

        for(locals._t = 0; locals._t < state.get()._fullyUnlockedCnt; locals._t++)
        {
            if(state.get().fullyUnlocker.get(locals._t)._unlockedID == input.user)
            {
                output.fullyRewardedAmount += state.get().fullyUnlocker.get(locals._t)._rewardedAmount;
                output.fullyUnlockedAmount += state.get().fullyUnlocker.get(locals._t)._unlockedAmount;
            }
        }
        for (locals._v2t = 0;
            locals._v2t < state.get().v2FullyUnlockedCnt;
            locals._v2t++)
        {
            if (state.get().v2FullyUnlocker.get(locals._v2t)._unlockedID == input.user)
            {
                output.fullyRewardedAmount +=
                    state.get().v2FullyUnlocker.get(locals._v2t)._rewardedAmount;
                output.fullyUnlockedAmount +=
                    state.get().v2FullyUnlocker.get(locals._v2t)._unlockedAmount;
            }
        }
    }

    struct _LockV2_input
    {
        uint32 lockPeriod;
    };

    struct _LockV2_output
    {
        sint32 returnCode;
    };

    struct _LockV2_locals
    {
        LockInfo newLocker;
        RoundInfo updatedRoundInfo;
        EpochIndexInfo tmpIndex;
        V2EpochInfo epochInfo;
        V2TermInfo termInfo;
        QEARNLogger log;
        uint32 termIndex;
        uint32 t;
        uint32 endIndex;
        uint64 amount;
        _RemoveGapsInLockerArray_input gapRemovalInput;
        _RemoveGapsInLockerArray_output gapRemovalOutput;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_LockV2)
    {
        output.returnCode = QEARN_INVALID_LOCK_PERIOD;
        if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_13)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_13;
        }
        else if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_26)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_26;
        }
        else if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_52)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_52;
        }
        else
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (qpi.epoch() < QEARN_V2_ACTIVATION_EPOCH
            || qpi.epoch() > QEARN_V2_LAST_LOCK_EPOCH)
        {
            output.returnCode = QEARN_V2_NOT_ACTIVE;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (qpi.invocationReward() < sint64(QEARN_MINIMUM_LOCKING_AMOUNT))
        {
            output.returnCode = QEARN_INVALID_INPUT_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }
        locals.amount = uint64(qpi.invocationReward());

        locals.endIndex = state.get()._epochIndex.get(qpi.epoch()).endIndex;
        for (locals.t = state.get()._epochIndex.get(qpi.epoch()).startIndex; locals.t < locals.endIndex; locals.t++)
        {
            if (state.get().locker.get(locals.t).ID == qpi.invocator()
                && state.get().lockerLockPeriods.get(locals.t) == input.lockPeriod)
            {
                if (state.get().locker.get(locals.t)._lockedAmount + locals.amount > QEARN_MAX_LOCK_AMOUNT)
                {
                    output.returnCode = QEARN_LIMIT_LOCK;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return;
                }

                locals.newLocker = state.get().locker.get(locals.t);
                locals.newLocker._lockedAmount += locals.amount;
                state.mut().locker.set(locals.t, locals.newLocker);

                locals.epochInfo = state.get().v2EpochInfo.get(qpi.epoch());
                locals.termInfo = locals.epochInfo.terms.get(locals.termIndex);
                locals.termInfo.initialLockedAmount += locals.amount;
                locals.termInfo.currentLockedAmount += locals.amount;
                locals.epochInfo.terms.set(locals.termIndex, locals.termInfo);
                state.mut().v2EpochInfo.set(qpi.epoch(), locals.epochInfo);

                locals.updatedRoundInfo = state.get()._initialRoundInfo.get(qpi.epoch());
                locals.updatedRoundInfo._totalLockedAmount += locals.amount;
                state.mut()._initialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);
                locals.updatedRoundInfo = state.get()._currentRoundInfo.get(qpi.epoch());
                locals.updatedRoundInfo._totalLockedAmount += locals.amount;
                state.mut()._currentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

                output.returnCode = QEARN_LOCK_SUCCESS;
                locals.log = {QEARN_CONTRACT_INDEX, qpi.invocator(), SELF, qpi.invocationReward(), QearnSuccessLocking, 0};
                LOG_INFO(locals.log);
                return;
            }
        }

        if (locals.endIndex >= QEARN_MAX_LOCKS - 1)
        {
            CALL(_RemoveGapsInLockerArray, locals.gapRemovalInput, locals.gapRemovalOutput);
            locals.endIndex = state.get()._epochIndex.get(qpi.epoch()).endIndex;
            if (locals.endIndex >= QEARN_MAX_LOCKS - 1)
            {
                output.returnCode = QEARN_OVERFLOW_USER;
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                return;
            }
        }

        if (locals.amount > QEARN_MAX_LOCK_AMOUNT)
        {
            output.returnCode = QEARN_LIMIT_LOCK;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.newLocker.ID = qpi.invocator();
        locals.newLocker._lockedAmount = locals.amount;
        locals.newLocker._lockedEpoch = qpi.epoch();
        state.mut().locker.set(locals.endIndex, locals.newLocker);
        state.mut().lockerLockPeriods.set(locals.endIndex, uint8(input.lockPeriod));

        locals.tmpIndex = state.get()._epochIndex.get(qpi.epoch());
        locals.tmpIndex.endIndex = locals.endIndex + 1;
        state.mut()._epochIndex.set(qpi.epoch(), locals.tmpIndex);

        locals.epochInfo = state.get().v2EpochInfo.get(qpi.epoch());
        locals.termInfo = locals.epochInfo.terms.get(locals.termIndex);
        locals.termInfo.initialLockedAmount += locals.amount;
        locals.termInfo.currentLockedAmount += locals.amount;
        locals.epochInfo.terms.set(locals.termIndex, locals.termInfo);
        state.mut().v2EpochInfo.set(qpi.epoch(), locals.epochInfo);

        locals.updatedRoundInfo = state.get()._initialRoundInfo.get(qpi.epoch());
        locals.updatedRoundInfo._totalLockedAmount += locals.amount;
        state.mut()._initialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);
        locals.updatedRoundInfo = state.get()._currentRoundInfo.get(qpi.epoch());
        locals.updatedRoundInfo._totalLockedAmount += locals.amount;
        state.mut()._currentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        output.returnCode = QEARN_LOCK_SUCCESS;
        locals.log = {QEARN_CONTRACT_INDEX, qpi.invocator(), SELF, qpi.invocationReward(), QearnSuccessLocking, 0};
        LOG_INFO(locals.log);
    }

    struct lockV2_locals
    {
        _LockV2_input lockInput;
        _LockV2_output lockOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(lockV2)
    {
        locals.lockInput.lockPeriod = input.lockPeriod;
        CALL(_LockV2, locals.lockInput, locals.lockOutput);
        output.returnCode = locals.lockOutput.returnCode;
    }

    struct lock_locals {

        LockInfo newLocker;
        RoundInfo updatedRoundInfo;
        EpochIndexInfo tmpIndex;
        QEARNLogger log;
        uint32 t;
        uint32 endIndex;
        _RemoveGapsInLockerArray_input gapRemovalInput;
        _RemoveGapsInLockerArray_output gapRemovalOutput;
        _LockV2_input lockV2Input;
        _LockV2_output lockV2Output;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(lock)
    {
        if (qpi.epoch() >= QEARN_V2_ACTIVATION_EPOCH)
        {
            locals.lockV2Input.lockPeriod = QEARN_V2_LOCK_PERIOD_52;
            CALL(_LockV2, locals.lockV2Input, locals.lockV2Output);
            output.returnCode = locals.lockV2Output.returnCode;
            return;
        }

        if (qpi.invocationReward() < QEARN_MINIMUM_LOCKING_AMOUNT || qpi.epoch() < QEARN_INITIAL_EPOCH)
        {
            output.returnCode = QEARN_INVALID_INPUT_AMOUNT;         // if the amount of locking is less than 10M, it should be failed to lock.

            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), qpi.invocationReward(), QearnInvalidInput, 0};
            LOG_INFO(locals.log);
            if(qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.endIndex = state.get()._epochIndex.get(qpi.epoch()).endIndex;

        for(locals.t = state.get()._epochIndex.get(qpi.epoch()).startIndex ; locals.t < locals.endIndex; locals.t++)
        {

            if(state.get().locker.get(locals.t).ID == qpi.invocator())
            {      // the case to be locked several times at one epoch, at that time, this address already located in state.Locker array, the amount will be increased as current locking amount.
                if(state.get().locker.get(locals.t)._lockedAmount + qpi.invocationReward() > QEARN_MAX_LOCK_AMOUNT)
                {
                    output.returnCode = QEARN_LIMIT_LOCK;

                    locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), qpi.invocationReward(), QearnLimitLocking, 0};
                    LOG_INFO(locals.log);

                    if(qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return;
                }

                locals.newLocker._lockedAmount = state.get().locker.get(locals.t)._lockedAmount + qpi.invocationReward();
                locals.newLocker._lockedEpoch = qpi.epoch();
                locals.newLocker.ID = qpi.invocator();

                state.mut().locker.set(locals.t, locals.newLocker);
                state.mut().lockerLockPeriods.set(locals.t, 0);

                locals.updatedRoundInfo._totalLockedAmount = state.get()._initialRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
                locals.updatedRoundInfo._epochBonusAmount = state.get()._initialRoundInfo.get(qpi.epoch())._epochBonusAmount;
                state.mut()._initialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

                locals.updatedRoundInfo._totalLockedAmount = state.get()._currentRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
                locals.updatedRoundInfo._epochBonusAmount = state.get()._currentRoundInfo.get(qpi.epoch())._epochBonusAmount;
                state.mut()._currentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

                output.returnCode = QEARN_LOCK_SUCCESS;          //  additional locking of this epoch is succeed

                locals.log = {QEARN_CONTRACT_INDEX, qpi.invocator(), SELF, qpi.invocationReward(), QearnSuccessLocking, 0};
                LOG_INFO(locals.log);
                return ;
            }

        }

        if(locals.endIndex >= QEARN_MAX_LOCKS - 1)
        {
            // Remove gaps in locker array to free up memory slots
            CALL(_RemoveGapsInLockerArray, locals.gapRemovalInput, locals.gapRemovalOutput);

            // Re-check if there's space after gap removal
            locals.endIndex = state.get()._epochIndex.get(qpi.epoch()).endIndex;

            if(locals.endIndex >= QEARN_MAX_LOCKS - 1)
            {
                output.returnCode = QEARN_OVERFLOW_USER;

                locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), qpi.invocationReward(), QearnOverflowUser, 0};
                LOG_INFO(locals.log);

                if(qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                return ;                        // overflow users in Qearn after gap removal
            }
        }

        if(qpi.invocationReward() > QEARN_MAX_LOCK_AMOUNT)
        {
            output.returnCode = QEARN_LIMIT_LOCK;

            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), qpi.invocationReward(), QearnLimitLocking, 0};
            LOG_INFO(locals.log);

            if(qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.newLocker.ID = qpi.invocator();
        locals.newLocker._lockedAmount = qpi.invocationReward();
        locals.newLocker._lockedEpoch = qpi.epoch();

        state.mut().locker.set(locals.endIndex, locals.newLocker);
        state.mut().lockerLockPeriods.set(locals.endIndex, 0);

        locals.tmpIndex.startIndex = state.get()._epochIndex.get(qpi.epoch()).startIndex;
        locals.tmpIndex.endIndex = locals.endIndex + 1;
        state.mut()._epochIndex.set(qpi.epoch(), locals.tmpIndex);

        locals.updatedRoundInfo._totalLockedAmount = state.get()._initialRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
        locals.updatedRoundInfo._epochBonusAmount = state.get()._initialRoundInfo.get(qpi.epoch())._epochBonusAmount;
        state.mut()._initialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        locals.updatedRoundInfo._totalLockedAmount = state.get()._currentRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
        locals.updatedRoundInfo._epochBonusAmount = state.get()._currentRoundInfo.get(qpi.epoch())._epochBonusAmount;
        state.mut()._currentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        output.returnCode = QEARN_LOCK_SUCCESS;            //  new locking of this epoch is succeed

        locals.log = {QEARN_CONTRACT_INDEX, qpi.invocator(), SELF, qpi.invocationReward(), QearnSuccessLocking, 0};
        LOG_INFO(locals.log);
    }

    struct _TransferV2SurplusToCCF_input
    {
        uint32 lockedEpoch;
        uint64 amount;
    };

    struct _TransferV2SurplusToCCF_output
    {
    };

    struct _TransferV2SurplusToCCF_locals
    {
        V2EpochInfo epochInfo;
        sint64 transferResult;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_TransferV2SurplusToCCF)
    {
        if (!input.amount)
        {
            return;
        }

        locals.transferResult = qpi.transfer(
            id(CCF_CONTRACT_INDEX, 0, 0, 0),
            sint64(input.amount));
        if (locals.transferResult < 0)
        {
            // Callers only pass surplus already backed by QEarn's balance.
            // Do not report a CCF transfer if that invariant is ever violated.
            ASSERT(locals.transferResult >= 0);
            return;
        }

        locals.epochInfo = state.get().v2EpochInfo.get(input.lockedEpoch);
        locals.epochInfo.transferredToCCFAmount += input.amount;
        state.mut().v2EpochInfo.set(input.lockedEpoch, locals.epochInfo);
    }

    struct _FundV2LongPool_input
    {
        uint32 lockedEpoch;
        uint64 amount;
    };

    struct _FundV2LongPool_output
    {
        uint64 fundedAmount;
        uint64 surplusAmount;
    };

    struct _FundV2LongPool_locals
    {
        V2EpochInfo epochInfo;
        V2TermInfo longTermInfo;
        RoundInfo roundInfo;
        StatsInfo stats;
        _TransferV2SurplusToCCF_input ccfInput;
        _TransferV2SurplusToCCF_output ccfOutput;
        uint128 calculation;
        uint64 cap;
        uint64 availableAmount;
        uint64 previousPool;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_FundV2LongPool)
    {
        locals.epochInfo = state.get().v2EpochInfo.get(input.lockedEpoch);
        locals.longTermInfo = locals.epochInfo.terms.get(QEARN_V2_TERM_INDEX_52);
        locals.previousPool = locals.longTermInfo.currentRewardPool;
        locals.calculation = div(
            uint128(locals.longTermInfo.currentLockedAmount) * uint128(QEARN_V2_RETURN_PERCENT_52),
            uint128(100));
        ASSERT(locals.calculation.high == 0);
        locals.cap = locals.calculation.low;
        locals.availableAmount = locals.previousPool + input.amount;

        if (locals.availableAmount > locals.cap)
        {
            locals.longTermInfo.currentRewardPool = locals.cap;
            output.surplusAmount = locals.availableAmount - locals.cap;
        }
        else
        {
            locals.longTermInfo.currentRewardPool = locals.availableAmount;
        }

        if (locals.longTermInfo.currentRewardPool > locals.previousPool)
        {
            output.fundedAmount = locals.longTermInfo.currentRewardPool - locals.previousPool;
        }
        locals.epochInfo.transferredTo52Amount += output.fundedAmount;
        locals.epochInfo.terms.set(QEARN_V2_TERM_INDEX_52, locals.longTermInfo);
        state.mut().v2EpochInfo.set(input.lockedEpoch, locals.epochInfo);

        if (output.surplusAmount)
        {
            locals.roundInfo = state.get()._currentRoundInfo.get(input.lockedEpoch);
            ASSERT(locals.roundInfo._epochBonusAmount >= output.surplusAmount);
            locals.roundInfo._epochBonusAmount -= output.surplusAmount;
            state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.roundInfo);
        }

        if (output.fundedAmount)
        {
            locals.stats = state.get().statsInfo.get(input.lockedEpoch);
            locals.stats.boostedAmount += output.fundedAmount;
            state.mut().statsInfo.set(input.lockedEpoch, locals.stats);
        }

        if (output.surplusAmount)
        {
            locals.ccfInput.lockedEpoch = input.lockedEpoch;
            locals.ccfInput.amount = output.surplusAmount;
            CALL(_TransferV2SurplusToCCF, locals.ccfInput, locals.ccfOutput);
        }
    }

    struct _UnlockV2_input
    {
        uint32 lockedEpoch;
        uint32 lockPeriod;
        uint64 requestedAmount;
        uint32 validateRequestedAmount;
    };

    struct _UnlockV2_output
    {
        sint32 returnCode;
    };

    struct _UnlockV2_locals
    {
        V2EpochInfo epochInfo;
        V2TermInfo termInfo;
        RoundInfo roundInfo;
        LockInfo emptyLocker;
        HistoryInfo historyInfo;
        QEARNLogger log;
        _FundV2LongPool_input fundInput;
        _FundV2LongPool_output fundOutput;
        uint128 calculation;
        uint32 termIndex;
        uint32 t;
        uint32 index;
        uint32 startIndex;
        uint32 endIndex;
        uint64 amount;
        uint64 forfeitedAmount;
        uint64 amountToLongPool;
        sint64 transferAmount;
        sint64 transferResult;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_UnlockV2)
    {
        output.returnCode = QEARN_INVALID_LOCK_PERIOD;
        if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_13)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_13;
        }
        else if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_26)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_26;
        }
        else if (input.lockPeriod == QEARN_V2_LOCK_PERIOD_52)
        {
            locals.termIndex = QEARN_V2_TERM_INDEX_52;
        }
        else
        {
            return;
        }

        if (input.lockedEpoch < QEARN_V2_ACTIVATION_EPOCH
            || input.lockedEpoch >= QEARN_MAX_EPOCHS
            || input.lockedEpoch > qpi.epoch())
        {
            output.returnCode = QEARN_INVALID_INPUT_LOCKED_EPOCH;
            return;
        }
        if (input.lockedEpoch == qpi.epoch())
        {
            output.returnCode = QEARN_INVALID_INPUT_LOCKED_EPOCH;
            return;
        }
        if (qpi.epoch() >= input.lockedEpoch + input.lockPeriod)
        {
            output.returnCode = QEARN_V2_POSITION_MATURED;
            return;
        }

        locals.index = QEARN_MAX_LOCKS;
        locals.startIndex = state.get()._epochIndex.get(input.lockedEpoch).startIndex;
        locals.endIndex = state.get()._epochIndex.get(input.lockedEpoch).endIndex;
        for (locals.t = locals.startIndex; locals.t < locals.endIndex; locals.t++)
        {
            if (state.get().locker.get(locals.t).ID == qpi.invocator()
                && state.get().lockerLockPeriods.get(locals.t) == input.lockPeriod)
            {
                locals.index = locals.t;
                break;
            }
        }
        if (locals.index == QEARN_MAX_LOCKS)
        {
            output.returnCode = QEARN_EMPTY_LOCKED;
            return;
        }

        locals.amount = state.get().locker.get(locals.index)._lockedAmount;
        if (input.validateRequestedAmount && input.requestedAmount != locals.amount)
        {
            output.returnCode = QEARN_INVALID_INPUT_UNLOCK_AMOUNT;
            return;
        }

        locals.epochInfo = state.get().v2EpochInfo.get(input.lockedEpoch);
        locals.termInfo = locals.epochInfo.terms.get(locals.termIndex);
        ASSERT(locals.termInfo.currentLockedAmount >= locals.amount);
        if (locals.termIndex == QEARN_V2_TERM_INDEX_52)
        {
            if (locals.termInfo.currentLockedAmount)
            {
                locals.calculation = div(
                    uint128(locals.termInfo.currentRewardPool) * uint128(locals.amount),
                    uint128(locals.termInfo.currentLockedAmount));
                ASSERT(locals.calculation.high == 0);
                locals.forfeitedAmount = locals.calculation.low;
            }
        }
        else if (locals.termInfo.initialLockedAmount)
        {
            locals.calculation = div(
                uint128(locals.termInfo.initialRewardPool) * uint128(locals.amount),
                uint128(locals.termInfo.initialLockedAmount));
            ASSERT(locals.calculation.high == 0);
            locals.forfeitedAmount = locals.calculation.low;
            if (locals.forfeitedAmount > locals.termInfo.currentRewardPool)
            {
                locals.forfeitedAmount = locals.termInfo.currentRewardPool;
            }
            locals.termInfo.currentRewardPool -= locals.forfeitedAmount;
            locals.amountToLongPool = locals.forfeitedAmount;
        }

        // A transfer to a contract is rejected while an incoming-transfer
        // callback is running. Do not close the position unless its principal
        // has actually been returned.
        locals.transferAmount = sint64(locals.amount);
        locals.transferResult = qpi.transfer(qpi.invocator(), locals.transferAmount);
        if (locals.transferResult < 0)
        {
            output.returnCode = QEARN_TRANSFER_FAILED;
            locals.log = {
                QEARN_CONTRACT_INDEX,
                SELF,
                qpi.invocator(),
                locals.transferAmount,
                QearnFailedTransfer,
                0
            };
            LOG_INFO(locals.log);
            return;
        }

        locals.termInfo.currentLockedAmount -= locals.amount;
        locals.termInfo.earlyUnlockedAmount += locals.amount;
        locals.termInfo.forfeitedClaimAmount += locals.forfeitedAmount;
        locals.epochInfo.forfeitedClaimAmount += locals.forfeitedAmount;
        if (locals.termIndex != QEARN_V2_TERM_INDEX_52 && !locals.termInfo.currentLockedAmount)
        {
            locals.amountToLongPool += locals.termInfo.currentRewardPool;
            locals.termInfo.currentRewardPool = 0;
        }
        locals.epochInfo.terms.set(locals.termIndex, locals.termInfo);
        state.mut().v2EpochInfo.set(input.lockedEpoch, locals.epochInfo);

        locals.roundInfo = state.get()._currentRoundInfo.get(input.lockedEpoch);
        ASSERT(locals.roundInfo._totalLockedAmount >= locals.amount);
        locals.roundInfo._totalLockedAmount -= locals.amount;
        state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.roundInfo);

        state.mut().locker.set(locals.index, locals.emptyLocker);
        state.mut().lockerLockPeriods.set(locals.index, 0);
        locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), locals.transferAmount, QearnSuccessEarlyUnlocking, 0};
        LOG_INFO(locals.log);

        locals.historyInfo._unlockedID = qpi.invocator();
        locals.historyInfo._unlockedAmount = locals.amount;
        for (locals.t = 0; locals.t < state.get()._earlyUnlockedCnt; locals.t++)
        {
            if (state.get().earlyUnlocker.get(locals.t)._unlockedID == qpi.invocator())
            {
                locals.historyInfo._unlockedAmount += state.get().earlyUnlocker.get(locals.t)._unlockedAmount;
                locals.historyInfo._rewardedAmount += state.get().earlyUnlocker.get(locals.t)._rewardedAmount;
                state.mut().earlyUnlocker.set(locals.t, locals.historyInfo);
                break;
            }
        }
        if (locals.t == state.get()._earlyUnlockedCnt && state.get()._earlyUnlockedCnt < QEARN_MAX_USERS)
        {
            state.mut().earlyUnlocker.set(locals.t, locals.historyInfo);
            state.mut()._earlyUnlockedCnt++;
        }

        locals.fundInput.lockedEpoch = input.lockedEpoch;
        locals.fundInput.amount = locals.amountToLongPool;
        CALL(_FundV2LongPool, locals.fundInput, locals.fundOutput);
        output.returnCode = QEARN_UNLOCK_SUCCESS;
    }

    struct unlockV2_locals
    {
        _UnlockV2_input unlockInput;
        _UnlockV2_output unlockOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(unlockV2)
    {
        locals.unlockInput.lockedEpoch = input.lockedEpoch;
        locals.unlockInput.lockPeriod = input.lockPeriod;
        CALL(_UnlockV2, locals.unlockInput, locals.unlockOutput);
        output.returnCode = locals.unlockOutput.returnCode;
    }

    struct unlock_locals {

        RoundInfo updatedRoundInfo;
        LockInfo updatedUserInfo;
        HistoryInfo unlockerInfo;
        StatsInfo tmpStats;
        QEARNLogger log;

        uint64 amountOfUnlocking;
        uint64 amountOfReward;
        uint64 amountOfburn;
        uint64 rewardPercent;
        sint64 transferAmount;
        sint64 transferResult;
        uint64 divCalcu;
        uint32 earlyUnlockingPercent;
        uint32 burnPercent;
        uint32 indexOfinvocator;
        uint32 _t;
        uint32 countOfLastVacancy;
        uint32 countOfLockedEpochs;
        uint32 startIndex;
        uint32 endIndex;
        _UnlockV2_input unlockV2Input;
        _UnlockV2_output unlockV2Output;

    };

    PUBLIC_PROCEDURE_WITH_LOCALS(unlock)
    {
        if (input.lockedEpoch >= QEARN_MAX_EPOCHS || input.lockedEpoch < QEARN_INITIAL_EPOCH)
        {

            output.returnCode = QEARN_INVALID_INPUT_LOCKED_EPOCH;               //   if user try to unlock with wrong locked epoch, it should be failed to unlock.

            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), 0, QearnInvalidInput, 0};
            LOG_INFO(locals.log);

            return ;

        }

        if(input.amount < QEARN_MINIMUM_LOCKING_AMOUNT)
        {

            output.returnCode = QEARN_INVALID_INPUT_AMOUNT;

            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), 0, QearnInvalidInput, 0};
            LOG_INFO(locals.log);

            return ;

        }

        if (input.lockedEpoch >= QEARN_V2_ACTIVATION_EPOCH)
        {
            locals.unlockV2Input.lockedEpoch = input.lockedEpoch;
            locals.unlockV2Input.lockPeriod = QEARN_V2_LOCK_PERIOD_52;
            locals.unlockV2Input.requestedAmount = input.amount;
            locals.unlockV2Input.validateRequestedAmount = 1;
            CALL(_UnlockV2, locals.unlockV2Input, locals.unlockV2Output);
            output.returnCode = locals.unlockV2Output.returnCode;
            return;
        }

        locals.indexOfinvocator = QEARN_MAX_LOCKS;
        locals.startIndex = state.get()._epochIndex.get(input.lockedEpoch).startIndex;
        locals.endIndex = state.get()._epochIndex.get(input.lockedEpoch).endIndex;

        for(locals._t = locals.startIndex ; locals._t < locals.endIndex; locals._t++)
        {

            if(state.get().locker.get(locals._t).ID == qpi.invocator())
            {
                if(state.get().locker.get(locals._t)._lockedAmount < input.amount)
                {

                    output.returnCode = QEARN_INVALID_INPUT_UNLOCK_AMOUNT;  //  if the amount to be wanted to unlock is more than locked amount, it should be failed to unlock

                    locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), 0, QearnInvalidInput, 0};
                    LOG_INFO(locals.log);

                    return ;

                }
                else
                {
                    locals.indexOfinvocator = locals._t;
                    break;
                }
            }

        }

        if(locals.indexOfinvocator == QEARN_MAX_LOCKS)
        {

            output.returnCode = QEARN_EMPTY_LOCKED;     //   if there is no any locked info in state.Locker array, it shows this address didn't lock at the epoch (input.Locked_Epoch)

            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), 0, QearnInvalidInput, 0};
            LOG_INFO(locals.log);

            return ;
        }

        /* the rest amount after unlocking should be more than MINIMUM_LOCKING_AMOUNT */
        if(state.get().locker.get(locals.indexOfinvocator)._lockedAmount - input.amount < QEARN_MINIMUM_LOCKING_AMOUNT)
        {
            locals.amountOfUnlocking = state.get().locker.get(locals.indexOfinvocator)._lockedAmount;
        }
        else
        {
            locals.amountOfUnlocking = input.amount;
        }

        locals.countOfLockedEpochs = qpi.epoch() - input.lockedEpoch - 1;

        locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_0_3;
        locals.burnPercent = QEARN_BURN_PERCENT_0_3;

        if(locals.countOfLockedEpochs >= 4 && locals.countOfLockedEpochs <= 7)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_4_7;
            locals.burnPercent = QEARN_BURN_PERCENT_4_7;
        }

        else if(locals.countOfLockedEpochs >= 8 && locals.countOfLockedEpochs <= 11)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_8_11;
            locals.burnPercent = QEARN_BURN_PERCENT_8_11;
        }

        else if(locals.countOfLockedEpochs >= 12 && locals.countOfLockedEpochs <= 15)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_12_15;
            locals.burnPercent = QEARN_BURN_PERCENT_12_15;
        }

        else if(locals.countOfLockedEpochs >= 16 && locals.countOfLockedEpochs <= 19)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_16_19;
            locals.burnPercent = QEARN_BURN_PERCENT_16_19;
        }

        else if(locals.countOfLockedEpochs >= 20 && locals.countOfLockedEpochs <= 23)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_20_23;
            locals.burnPercent = QEARN_BURN_PERCENT_20_23;
        }

        else if(locals.countOfLockedEpochs >= 24 && locals.countOfLockedEpochs <= 27)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_24_27;
            locals.burnPercent = QEARN_BURN_PERCENT_24_27;
        }

        else if(locals.countOfLockedEpochs >= 28 && locals.countOfLockedEpochs <= 31)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_28_31;
            locals.burnPercent = QEARN_BURN_PERCENT_28_31;
        }

        else if(locals.countOfLockedEpochs >= 32 && locals.countOfLockedEpochs <= 35)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_32_35;
            locals.burnPercent = QEARN_BURN_PERCENT_32_35;
        }

        else if(locals.countOfLockedEpochs >= 36 && locals.countOfLockedEpochs <= 39)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_36_39;
            locals.burnPercent = QEARN_BURN_PERCENT_36_39;
        }

        else if(locals.countOfLockedEpochs >= 40 && locals.countOfLockedEpochs <= 43)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_40_43;
            locals.burnPercent = QEARN_BURN_PERCENT_40_43;
        }

        else if(locals.countOfLockedEpochs >= 44 && locals.countOfLockedEpochs <= 47)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_44_47;
            locals.burnPercent = QEARN_BURN_PERCENT_44_47;
        }

        else if(locals.countOfLockedEpochs >= 48 && locals.countOfLockedEpochs <= 51)
        {
            locals.earlyUnlockingPercent = QEARN_EARLY_UNLOCKING_PERCENT_48_51;
            locals.burnPercent = QEARN_BURN_PERCENT_48_51;
        }

        locals.rewardPercent = div(state.get()._currentRoundInfo.get(input.lockedEpoch)._epochBonusAmount * 10000000ULL, state.get()._currentRoundInfo.get(input.lockedEpoch)._totalLockedAmount);
        locals.divCalcu = div(locals.rewardPercent * locals.amountOfUnlocking , 100ULL);
        locals.amountOfReward = div(locals.divCalcu * locals.earlyUnlockingPercent * 1ULL , 10000000ULL);
        locals.amountOfburn = div(locals.divCalcu * locals.burnPercent * 1ULL, 10000000ULL);

        locals.transferAmount = locals.amountOfUnlocking + locals.amountOfReward;
        locals.transferResult = qpi.transfer(qpi.invocator(), locals.transferAmount);
        if (locals.transferResult < 0)
        {
            output.returnCode = QEARN_TRANSFER_FAILED;
            locals.log = {
                QEARN_CONTRACT_INDEX,
                SELF,
                qpi.invocator(),
                locals.transferAmount,
                QearnFailedTransfer,
                0
            };
            LOG_INFO(locals.log);
            return;
        }

        locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), locals.transferAmount, QearnSuccessEarlyUnlocking, 0};
        LOG_INFO(locals.log);

        qpi.burn(locals.amountOfburn);

        if(input.lockedEpoch != qpi.epoch())
        {
            locals.tmpStats.burnedAmount = state.get().statsInfo.get(input.lockedEpoch).burnedAmount + locals.amountOfburn;
            locals.tmpStats.rewardedAmount = state.get().statsInfo.get(input.lockedEpoch).rewardedAmount + locals.amountOfReward;
            locals.tmpStats.boostedAmount = state.get().statsInfo.get(input.lockedEpoch).boostedAmount + div(locals.divCalcu * (100 - locals.burnPercent - locals.earlyUnlockingPercent) * 1ULL, 10000000ULL);

            state.mut().statsInfo.set(input.lockedEpoch, locals.tmpStats);
        }

        locals.updatedRoundInfo._totalLockedAmount = state.get()._currentRoundInfo.get(input.lockedEpoch)._totalLockedAmount - locals.amountOfUnlocking;
        locals.updatedRoundInfo._epochBonusAmount = state.get()._currentRoundInfo.get(input.lockedEpoch)._epochBonusAmount - locals.amountOfReward - locals.amountOfburn;

        state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.updatedRoundInfo);

        if(qpi.epoch() == input.lockedEpoch)
        {
            locals.updatedRoundInfo._totalLockedAmount = state.get()._initialRoundInfo.get(input.lockedEpoch)._totalLockedAmount - locals.amountOfUnlocking;
            locals.updatedRoundInfo._epochBonusAmount = state.get()._initialRoundInfo.get(input.lockedEpoch)._epochBonusAmount;

            state.mut()._initialRoundInfo.set(input.lockedEpoch, locals.updatedRoundInfo);
        }

        if(state.get().locker.get(locals.indexOfinvocator)._lockedAmount == locals.amountOfUnlocking)
        {
            locals.updatedUserInfo.ID = NULL_ID;
            locals.updatedUserInfo._lockedAmount = 0;
            locals.updatedUserInfo._lockedEpoch = 0;
        }
        else
        {
            locals.updatedUserInfo.ID = qpi.invocator();
            locals.updatedUserInfo._lockedAmount = state.get().locker.get(locals.indexOfinvocator)._lockedAmount - locals.amountOfUnlocking;
            locals.updatedUserInfo._lockedEpoch = state.get().locker.get(locals.indexOfinvocator)._lockedEpoch;
        }

        state.mut().locker.set(locals.indexOfinvocator, locals.updatedUserInfo);

        if(state.get()._currentRoundInfo.get(input.lockedEpoch)._totalLockedAmount == 0 && input.lockedEpoch != qpi.epoch())
        {

            // If all users have unlocked early, burn bonus
            qpi.burn(state.get()._currentRoundInfo.get(input.lockedEpoch)._epochBonusAmount);

            locals.updatedRoundInfo._totalLockedAmount = 0;
            locals.updatedRoundInfo._epochBonusAmount = 0;

            state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.updatedRoundInfo);

        }

        if(input.lockedEpoch != qpi.epoch())
        {

            locals.unlockerInfo._unlockedID = qpi.invocator();

            for(locals._t = 0; locals._t < state.get()._earlyUnlockedCnt; locals._t++)
            {
                if(state.get().earlyUnlocker.get(locals._t)._unlockedID == qpi.invocator())
                {

                    locals.unlockerInfo._rewardedAmount = state.get().earlyUnlocker.get(locals._t)._rewardedAmount + locals.amountOfReward;
                    locals.unlockerInfo._unlockedAmount = state.get().earlyUnlocker.get(locals._t)._unlockedAmount + locals.amountOfUnlocking;

                    state.mut().earlyUnlocker.set(locals._t, locals.unlockerInfo);

                    break;
                }
            }

            if(locals._t == state.get()._earlyUnlockedCnt && state.get()._earlyUnlockedCnt < QEARN_MAX_USERS)
            {
                locals.unlockerInfo._rewardedAmount = locals.amountOfReward;
                locals.unlockerInfo._unlockedAmount = locals.amountOfUnlocking;

                state.mut().earlyUnlocker.set(locals._t, locals.unlockerInfo);
                state.mut()._earlyUnlockedCnt++;
            }

        }

        output.returnCode = QEARN_UNLOCK_SUCCESS; //  unlock is succeed
    }

    struct _FinalizeV2Round_input
    {
        uint32 lockedEpoch;
    };

    struct _FinalizeV2Round_output
    {
    };

    struct _FinalizeV2Round_locals
    {
        V2EpochInfo epochInfo;
        V2TermInfo term13;
        V2TermInfo term26;
        V2TermInfo term52;
        RoundInfo roundInfo;
        _TransferV2SurplusToCCF_input ccfInput;
        _TransferV2SurplusToCCF_output ccfOutput;
        uint128 calculation;
        uint64 bonusAmount;
        uint64 allocation13;
        uint64 allocation26;
        uint64 cap13;
        uint64 cap26;
        uint64 cap52;
        uint64 available52;
        uint64 surplusAmount;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_FinalizeV2Round)
    {
        if (input.lockedEpoch < QEARN_V2_ACTIVATION_EPOCH || input.lockedEpoch >= QEARN_MAX_EPOCHS)
        {
            return;
        }

        locals.epochInfo = state.get().v2EpochInfo.get(input.lockedEpoch);
        if (locals.epochInfo.finalized)
        {
            return;
        }

        locals.term13 = locals.epochInfo.terms.get(QEARN_V2_TERM_INDEX_13);
        locals.term26 = locals.epochInfo.terms.get(QEARN_V2_TERM_INDEX_26);
        locals.term52 = locals.epochInfo.terms.get(QEARN_V2_TERM_INDEX_52);
        locals.bonusAmount = state.get()._initialRoundInfo.get(input.lockedEpoch)._epochBonusAmount;

        locals.calculation = div(
            uint128(locals.bonusAmount) * uint128(QEARN_V2_ALLOCATION_PERCENT_13),
            uint128(100));
        ASSERT(locals.calculation.high == 0);
        locals.allocation13 = locals.calculation.low;
        locals.calculation = div(
            uint128(locals.term13.initialLockedAmount) * uint128(QEARN_V2_RETURN_PERCENT_13),
            uint128(100));
        ASSERT(locals.calculation.high == 0);
        locals.cap13 = locals.calculation.low;
        if (locals.allocation13 > locals.cap13)
        {
            locals.allocation13 = locals.cap13;
        }

        locals.calculation = div(
            uint128(locals.bonusAmount) * uint128(QEARN_V2_ALLOCATION_PERCENT_26),
            uint128(100));
        ASSERT(locals.calculation.high == 0);
        locals.allocation26 = locals.calculation.low;
        locals.calculation = div(
            uint128(locals.term26.initialLockedAmount) * uint128(QEARN_V2_RETURN_PERCENT_26),
            uint128(100));
        ASSERT(locals.calculation.high == 0);
        locals.cap26 = locals.calculation.low;
        if (locals.allocation26 > locals.cap26)
        {
            locals.allocation26 = locals.cap26;
        }

        ASSERT(locals.bonusAmount >= locals.allocation13 + locals.allocation26);
        locals.available52 = locals.bonusAmount - locals.allocation13 - locals.allocation26;
        locals.calculation = div(
            uint128(locals.term52.initialLockedAmount) * uint128(QEARN_V2_RETURN_PERCENT_52),
            uint128(100));
        ASSERT(locals.calculation.high == 0);
        locals.cap52 = locals.calculation.low;

        locals.term13.initialRewardPool = locals.allocation13;
        locals.term13.currentRewardPool = locals.allocation13;
        locals.term26.initialRewardPool = locals.allocation26;
        locals.term26.currentRewardPool = locals.allocation26;
        if (locals.available52 > locals.cap52)
        {
            locals.term52.initialRewardPool = locals.cap52;
            locals.term52.currentRewardPool = locals.cap52;
            locals.surplusAmount = locals.available52 - locals.cap52;
        }
        else
        {
            locals.term52.initialRewardPool = locals.available52;
            locals.term52.currentRewardPool = locals.available52;
        }

        locals.epochInfo.terms.set(QEARN_V2_TERM_INDEX_13, locals.term13);
        locals.epochInfo.terms.set(QEARN_V2_TERM_INDEX_26, locals.term26);
        locals.epochInfo.terms.set(QEARN_V2_TERM_INDEX_52, locals.term52);
        locals.epochInfo.bonusAmount = locals.bonusAmount;
        locals.epochInfo.finalized = 1;
        state.mut().v2EpochInfo.set(input.lockedEpoch, locals.epochInfo);

        locals.roundInfo = state.get()._currentRoundInfo.get(input.lockedEpoch);
        locals.roundInfo._epochBonusAmount = locals.allocation13 + locals.allocation26 + locals.term52.currentRewardPool;
        state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.roundInfo);

        if (locals.surplusAmount)
        {
            locals.ccfInput.lockedEpoch = input.lockedEpoch;
            locals.ccfInput.amount = locals.surplusAmount;
            CALL(_TransferV2SurplusToCCF, locals.ccfInput, locals.ccfOutput);
        }
    }

    struct _PayoutV2Term_input
    {
        uint32 lockedEpoch;
        uint32 lockPeriod;
        uint32 termIndex;
    };

    struct _PayoutV2Term_output
    {
    };

    struct _PayoutV2Term_locals
    {
        V2EpochInfo epochInfo;
        V2TermInfo termInfo;
        RoundInfo roundInfo;
        StatsInfo stats;
        LockInfo emptyLocker;
        HistoryInfo historyInfo;
        QEARNLogger log;
        _FundV2LongPool_input fundInput;
        _FundV2LongPool_output fundOutput;
        _TransferV2SurplusToCCF_input ccfInput;
        _TransferV2SurplusToCCF_output ccfOutput;
        uint128 calculation;
        uint32 t;
        uint32 startIndex;
        uint32 endIndex;
        uint64 denominator;
        uint64 numerator;
        uint64 rewardAmount;
        uint64 totalRewardedAmount;
        uint64 totalUnlockedAmount;
        uint64 remainingRewardPool;
        sint64 transferAmount;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_PayoutV2Term)
    {
        if (input.lockedEpoch < QEARN_V2_ACTIVATION_EPOCH || input.lockedEpoch >= QEARN_MAX_EPOCHS)
        {
            return;
        }
        locals.epochInfo = state.get().v2EpochInfo.get(input.lockedEpoch);
        if (!locals.epochInfo.finalized)
        {
            return;
        }
        locals.termInfo = locals.epochInfo.terms.get(input.termIndex);
        locals.remainingRewardPool = locals.termInfo.currentRewardPool;
        if (input.termIndex == QEARN_V2_TERM_INDEX_52)
        {
            locals.numerator = locals.termInfo.currentRewardPool;
            locals.denominator = locals.termInfo.currentLockedAmount;
        }
        else
        {
            locals.numerator = locals.termInfo.initialRewardPool;
            locals.denominator = locals.termInfo.initialLockedAmount;
        }

        locals.startIndex = state.get()._epochIndex.get(input.lockedEpoch).startIndex;
        locals.endIndex = state.get()._epochIndex.get(input.lockedEpoch).endIndex;
        for (locals.t = locals.startIndex; locals.t < locals.endIndex; locals.t++)
        {
            if (!state.get().locker.get(locals.t)._lockedAmount
                || state.get().lockerLockPeriods.get(locals.t) != input.lockPeriod)
            {
                continue;
            }

            if (locals.denominator)
            {
                locals.calculation = div(
                    uint128(locals.numerator) * uint128(state.get().locker.get(locals.t)._lockedAmount),
                    uint128(locals.denominator));
                ASSERT(locals.calculation.high == 0);
                locals.rewardAmount = locals.calculation.low;
                if (locals.rewardAmount > locals.remainingRewardPool)
                {
                    locals.rewardAmount = locals.remainingRewardPool;
                }
            }
            else
            {
                locals.rewardAmount = 0;
            }

            locals.transferAmount = sint64(state.get().locker.get(locals.t)._lockedAmount + locals.rewardAmount);
            qpi.transfer(state.get().locker.get(locals.t).ID, locals.transferAmount);
            locals.log = {QEARN_CONTRACT_INDEX, SELF, state.get().locker.get(locals.t).ID, locals.transferAmount, QearnSuccessFullyUnlocking, 0};
            LOG_INFO(locals.log);

            locals.historyInfo._unlockedID = state.get().locker.get(locals.t).ID;
            locals.historyInfo._unlockedAmount = state.get().locker.get(locals.t)._lockedAmount;
            locals.historyInfo._rewardedAmount = locals.rewardAmount;
            if (state.get().v2FullyUnlockedCnt < QEARN_V2_MAX_FULLY_UNLOCK_RECORDS)
            {
                state.mut().v2FullyUnlocker.set(
                    state.get().v2FullyUnlockedCnt,
                    locals.historyInfo);
                state.mut().v2FullyUnlockedCnt++;
            }

            locals.totalUnlockedAmount += state.get().locker.get(locals.t)._lockedAmount;
            locals.totalRewardedAmount += locals.rewardAmount;
            locals.remainingRewardPool -= locals.rewardAmount;
            state.mut().locker.set(locals.t, locals.emptyLocker);
            state.mut().lockerLockPeriods.set(locals.t, 0);
        }

        ASSERT(locals.termInfo.currentLockedAmount >= locals.totalUnlockedAmount);
        locals.termInfo.currentLockedAmount -= locals.totalUnlockedAmount;
        ASSERT(locals.termInfo.currentRewardPool >= locals.totalRewardedAmount);
        locals.termInfo.currentRewardPool = 0;
        locals.termInfo.rewardedAmount += locals.totalRewardedAmount;
        locals.epochInfo.terms.set(input.termIndex, locals.termInfo);
        locals.epochInfo.rewardedAmount += locals.totalRewardedAmount;
        state.mut().v2EpochInfo.set(input.lockedEpoch, locals.epochInfo);

        locals.roundInfo = state.get()._currentRoundInfo.get(input.lockedEpoch);
        ASSERT(locals.roundInfo._totalLockedAmount >= locals.totalUnlockedAmount);
        ASSERT(locals.roundInfo._epochBonusAmount >= locals.totalRewardedAmount);
        locals.roundInfo._totalLockedAmount -= locals.totalUnlockedAmount;
        locals.roundInfo._epochBonusAmount -= locals.totalRewardedAmount;
        state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.roundInfo);

        locals.stats = state.get().statsInfo.get(input.lockedEpoch);
        locals.stats.rewardedAmount += locals.totalRewardedAmount;
        state.mut().statsInfo.set(input.lockedEpoch, locals.stats);

        if (input.termIndex == QEARN_V2_TERM_INDEX_52)
        {
            if (locals.remainingRewardPool)
            {
                locals.roundInfo = state.get()._currentRoundInfo.get(input.lockedEpoch);
                ASSERT(locals.roundInfo._epochBonusAmount >= locals.remainingRewardPool);
                locals.roundInfo._epochBonusAmount -= locals.remainingRewardPool;
                state.mut()._currentRoundInfo.set(input.lockedEpoch, locals.roundInfo);
                locals.ccfInput.lockedEpoch = input.lockedEpoch;
                locals.ccfInput.amount = locals.remainingRewardPool;
                CALL(_TransferV2SurplusToCCF, locals.ccfInput, locals.ccfOutput);
            }
        }
        else
        {
            locals.fundInput.lockedEpoch = input.lockedEpoch;
            locals.fundInput.amount = locals.remainingRewardPool;
            CALL(_FundV2LongPool, locals.fundInput, locals.fundOutput);
        }
    }

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getLockInfoPerEpoch, 1);
        REGISTER_USER_FUNCTION(getUserLockedInfo, 2);
        REGISTER_USER_FUNCTION(getStateOfRound, 3);
        REGISTER_USER_FUNCTION(getUserLockStatus, 4);
        REGISTER_USER_FUNCTION(getEndedStatus, 5);
        REGISTER_USER_FUNCTION(getStatsPerEpoch, 6);
        REGISTER_USER_FUNCTION(getBurnedAndBoostedStats, 7);
        REGISTER_USER_FUNCTION(getBurnedAndBoostedStatsPerEpoch, 8);
        REGISTER_USER_FUNCTION(getV2LockInfoPerEpoch, 9);
        REGISTER_USER_FUNCTION(getV2UserLockedInfo, 10);

        REGISTER_USER_PROCEDURE(lock, 1);
		REGISTER_USER_PROCEDURE(unlock, 2);
        REGISTER_USER_PROCEDURE(lockV2, 3);
        REGISTER_USER_PROCEDURE(unlockV2, 4);

	}

    struct BEGIN_EPOCH_locals
    {
        HistoryInfo INITIALIZE_HISTORY;
        LockInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;
        StatsInfo INITIALIZE_STATS;

        uint32 t;
        uint32 start_index;
        uint32 end_index;
        bit status;
        uint64 pre_epoch_balance;
        uint64 current_balance;
        Entity entity;
        uint32 locked_epoch;
        uint64 totalLockedAmountInEpoch217;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        qpi.getEntity(SELF, locals.entity);
        locals.current_balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
        locals.pre_epoch_balance = 0ULL;
        locals.totalLockedAmountInEpoch217 = 0ULL;

        if (qpi.epoch() == 218)
        {
            locals.start_index = state.get()._epochIndex.get(217).startIndex;
            locals.end_index = state.get()._epochIndex.get(217).endIndex;

            for (locals.t = locals.start_index; locals.t < locals.end_index; locals.t++)
            {
                locals.totalLockedAmountInEpoch217 += state.get().locker.get(locals.t)._lockedAmount;
            }
            locals.INITIALIZE_ROUNDINFO._totalLockedAmount = locals.totalLockedAmountInEpoch217;
            locals.INITIALIZE_ROUNDINFO._epochBonusAmount = 50227542196;
            state.mut()._initialRoundInfo.set(217, locals.INITIALIZE_ROUNDINFO);
            state.mut()._currentRoundInfo.set(217, locals.INITIALIZE_ROUNDINFO);
        }

        locals.pre_epoch_balance = 0ULL;
        locals.locked_epoch = qpi.epoch() - 52;
        for(locals.t = qpi.epoch() - 1; locals.t >= locals.locked_epoch; locals.t--)
        {
            locals.pre_epoch_balance += state.get()._currentRoundInfo.get(locals.t)._epochBonusAmount + state.get()._currentRoundInfo.get(locals.t)._totalLockedAmount;
        }

        if (state.get()._initialRoundInfo.get(qpi.epoch())._totalLockedAmount == 0) 
        {
            if(locals.current_balance - locals.pre_epoch_balance > QEARN_MAX_BONUS_AMOUNT)
            {
                qpi.burn(locals.current_balance - locals.pre_epoch_balance - QEARN_MAX_BONUS_AMOUNT);
                locals.INITIALIZE_ROUNDINFO._epochBonusAmount = QEARN_MAX_BONUS_AMOUNT;
            }
            else
            {
                locals.INITIALIZE_ROUNDINFO._epochBonusAmount = locals.current_balance - locals.pre_epoch_balance;
            }
            locals.INITIALIZE_ROUNDINFO._totalLockedAmount = 0;
    
            state.mut()._initialRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
            state.mut()._currentRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);   
        }
	}

    struct END_EPOCH_locals
    {
        HistoryInfo INITIALIZE_HISTORY;
        LockInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;
        EpochIndexInfo tmpEpochIndex;
        StatsInfo tmpStats;
        QEARNLogger log;
        _RemoveGapsInLockerArray_input gapRemovalInput;
        _RemoveGapsInLockerArray_output gapRemovalOutput;
        _PayoutV2Term_input payout13Input;
        _PayoutV2Term_output payout13Output;
        _PayoutV2Term_input payout26Input;
        _PayoutV2Term_output payout26Output;
        _PayoutV2Term_input payout52Input;
        _PayoutV2Term_output payout52Output;
        _FinalizeV2Round_input finalizeInput;
        _FinalizeV2Round_output finalizeOutput;

        uint64 _rewardPercent;
        uint64 _rewardAmount;
        uint64 _burnAmount;
        sint64 transferAmount;
        uint32 lockedEpoch;
        uint32 _t;
        uint32 endIndex;

    };

	END_EPOCH_WITH_LOCALS()
    {
        state.mut()._earlyUnlockedCnt = 0;
        state.mut()._fullyUnlockedCnt = 0;
        state.mut().v2FullyUnlockedCnt = 0;

        if (qpi.epoch() >= QEARN_V2_ACTIVATION_EPOCH + QEARN_V2_LOCK_PERIOD_13)
        {
            locals.payout13Input.lockedEpoch = qpi.epoch() - QEARN_V2_LOCK_PERIOD_13;
            locals.payout13Input.lockPeriod = QEARN_V2_LOCK_PERIOD_13;
            locals.payout13Input.termIndex = QEARN_V2_TERM_INDEX_13;
            CALL(_PayoutV2Term, locals.payout13Input, locals.payout13Output);
        }
        if (qpi.epoch() >= QEARN_V2_ACTIVATION_EPOCH + QEARN_V2_LOCK_PERIOD_26)
        {
            locals.payout26Input.lockedEpoch = qpi.epoch() - QEARN_V2_LOCK_PERIOD_26;
            locals.payout26Input.lockPeriod = QEARN_V2_LOCK_PERIOD_26;
            locals.payout26Input.termIndex = QEARN_V2_TERM_INDEX_26;
            CALL(_PayoutV2Term, locals.payout26Input, locals.payout26Output);
        }

        locals.lockedEpoch = qpi.epoch() - 52;
        locals.endIndex = state.get()._epochIndex.get(locals.lockedEpoch).endIndex;

        if (locals.lockedEpoch < QEARN_V2_ACTIVATION_EPOCH)
        {
            locals._burnAmount = state.get()._currentRoundInfo.get(locals.lockedEpoch)._epochBonusAmount;

            locals._rewardPercent = div(state.get()._currentRoundInfo.get(locals.lockedEpoch)._epochBonusAmount * 10000000ULL, state.get()._currentRoundInfo.get(locals.lockedEpoch)._totalLockedAmount);
            locals.tmpStats.rewardedAmount = state.get().statsInfo.get(locals.lockedEpoch).rewardedAmount;

            for(locals._t = state.get()._epochIndex.get(locals.lockedEpoch).startIndex; locals._t < locals.endIndex; locals._t++)
            {
                if(state.get().locker.get(locals._t)._lockedAmount == 0)
                {
                    continue;
                }

                ASSERT(state.get().locker.get(locals._t)._lockedEpoch == locals.lockedEpoch);

                locals._rewardAmount = div(state.get().locker.get(locals._t)._lockedAmount * locals._rewardPercent, 10000000ULL);
                qpi.transfer(state.get().locker.get(locals._t).ID, locals._rewardAmount + state.get().locker.get(locals._t)._lockedAmount);

                locals.transferAmount = locals._rewardAmount + state.get().locker.get(locals._t)._lockedAmount;
                locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), locals.transferAmount, QearnSuccessFullyUnlocking, 0};
                LOG_INFO(locals.log);

                locals.INITIALIZE_HISTORY._unlockedID = state.get().locker.get(locals._t).ID;
                locals.INITIALIZE_HISTORY._unlockedAmount = state.get().locker.get(locals._t)._lockedAmount;
                locals.INITIALIZE_HISTORY._rewardedAmount = locals._rewardAmount;
                if (state.get()._fullyUnlockedCnt < QEARN_MAX_USERS)
                {
                    state.mut().fullyUnlocker.set(state.get()._fullyUnlockedCnt, locals.INITIALIZE_HISTORY);
                    state.mut()._fullyUnlockedCnt++;
                }

                locals.INITIALIZE_USER.ID = NULL_ID;
                locals.INITIALIZE_USER._lockedAmount = 0;
                locals.INITIALIZE_USER._lockedEpoch = 0;

                state.mut().locker.set(locals._t, locals.INITIALIZE_USER);
                state.mut().lockerLockPeriods.set(locals._t, 0);

                locals._burnAmount -= locals._rewardAmount;
                locals.tmpStats.rewardedAmount += locals._rewardAmount;
            }

            qpi.burn(locals._burnAmount);

            locals.tmpStats.boostedAmount = state.get().statsInfo.get(locals.lockedEpoch).boostedAmount;
            locals.tmpStats.burnedAmount = state.get().statsInfo.get(locals.lockedEpoch).burnedAmount + locals._burnAmount;

            state.mut().statsInfo.set(locals.lockedEpoch, locals.tmpStats);
        }
        else
        {
            locals.payout52Input.lockedEpoch = locals.lockedEpoch;
            locals.payout52Input.lockPeriod = QEARN_V2_LOCK_PERIOD_52;
            locals.payout52Input.termIndex = QEARN_V2_TERM_INDEX_52;
            CALL(_PayoutV2Term, locals.payout52Input, locals.payout52Output);
        }

        if (qpi.epoch() >= QEARN_V2_ACTIVATION_EPOCH)
        {
            locals.finalizeInput.lockedEpoch = qpi.epoch();
            CALL(_FinalizeV2Round, locals.finalizeInput, locals.finalizeOutput);
        }

        locals.tmpEpochIndex.startIndex = 0;
        locals.tmpEpochIndex.endIndex = 0;
        state.mut()._epochIndex.set(locals.lockedEpoch, locals.tmpEpochIndex);
        CALL(_RemoveGapsInLockerArray, locals.gapRemovalInput, locals.gapRemovalOutput);
		}
};
