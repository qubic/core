using namespace QPI;

constexpr uint64 QEARN_MINIMUM_LOCKING_AMOUNT = 10000000;
constexpr uint64 QEARN_MAX_LOCKS = 4194304;
constexpr uint64 QEARN_MAX_EPOCHS = 4096;
constexpr uint64 QEARN_MAX_USERS = 131072;
constexpr uint64 QEARN_MAX_LOCK_AMOUNT = 1000000000000ULL;
constexpr uint64 QEARN_MAX_BONUS_AMOUNT = 1000000000000ULL;
constexpr uint64 QEARN_INITIAL_EPOCH = 138;

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
public:
    struct getLockInfoPerEpoch_input {
		uint32 Epoch;                             /* epoch number to get information */
    };

    struct getLockInfoPerEpoch_output {
        uint64 lockedAmount;                      /* initial total locked amount in epoch */
        uint64 bonusAmount;                       /* initial bonus amount in epoch*/
        uint64 currentLockedAmount;               /* total locked amount in epoch. exactly the amount excluding the amount unlocked early*/
        uint64 currentBonusAmount;                /* bonus amount in epoch excluding the early unlocking */
        uint64 yield;                             /* Yield calculated by 10000000 multiple*/
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

    struct getStatsPerEpoch_input {
        uint32 epoch;
    };

    struct getStatsPerEpoch_output {

        uint64 earlyUnlockedAmount;
        uint64 earlyUnlockedPercent;
        uint64 totalLockedAmount;
        uint64 averageAPY;

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

    struct RoundInfo {

        uint64 _totalLockedAmount;            // The initial total locked amount in any epoch.  Max Epoch is 65535
        uint64 _epochBonusAmount;             // The initial bonus amount per an epoch.         Max Epoch is 65535 

    };

    Array<RoundInfo, QEARN_MAX_EPOCHS> _initialRoundInfo;
    Array<RoundInfo, QEARN_MAX_EPOCHS> _currentRoundInfo;

    struct EpochIndexInfo {

        uint32 startIndex;
        uint32 endIndex;
    };

    Array<EpochIndexInfo, QEARN_MAX_EPOCHS> _epochIndex;

    struct LockInfo {

        uint64 _lockedAmount;
        id ID;
        uint32 _lockedEpoch;
        
    };

    Array<LockInfo, QEARN_MAX_LOCKS> locker;

    struct HistoryInfo {

        uint64 _unlockedAmount;
        uint64 _rewardedAmount;
        id _unlockedID;

    };

    Array<HistoryInfo, QEARN_MAX_USERS> earlyUnlocker;
    Array<HistoryInfo, QEARN_MAX_USERS> fullyUnlocker;
    
    uint32 _earlyUnlockedCnt;
    uint32 _fullyUnlockedCnt;

    struct StatsInfo {

        uint64 burnedAmount;
        uint64 boostedAmount;
        uint64 rewardedAmount;

    };

    Array<StatsInfo, QEARN_MAX_EPOCHS> statsInfo;

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

    PUBLIC_FUNCTION(getLockInfoPerEpoch)
    {
        output.bonusAmount = state._initialRoundInfo.get(input.Epoch)._epochBonusAmount;
        output.lockedAmount = state._initialRoundInfo.get(input.Epoch)._totalLockedAmount;
        output.currentBonusAmount = state._currentRoundInfo.get(input.Epoch)._epochBonusAmount;
        output.currentLockedAmount = state._currentRoundInfo.get(input.Epoch)._totalLockedAmount;
        if(state._currentRoundInfo.get(input.Epoch)._totalLockedAmount) 
        {
            output.yield = div(state._currentRoundInfo.get(input.Epoch)._epochBonusAmount * 10000000ULL, state._currentRoundInfo.get(input.Epoch)._totalLockedAmount);
        }
        else 
        {
            output.yield = 0ULL;
        }
    }

    struct getStatsPerEpoch_locals 
    {
        Entity entity;
        uint32 cnt, _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getStatsPerEpoch)
    {
        output.earlyUnlockedAmount = state._initialRoundInfo.get(input.epoch)._totalLockedAmount - state._currentRoundInfo.get(input.epoch)._totalLockedAmount;
        output.earlyUnlockedPercent = div(output.earlyUnlockedAmount * 10000ULL, state._initialRoundInfo.get(input.epoch)._totalLockedAmount);

        qpi.getEntity(SELF, locals.entity);
        output.totalLockedAmount = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        output.averageAPY = 0;
        locals.cnt = 0;

        for(locals._t = qpi.epoch() - 1U; locals._t >= qpi.epoch() - 52U; locals._t--)
        {
            if(locals._t < QEARN_INITIAL_EPOCH)
            {
                break;
            }
            if(state._currentRoundInfo.get(locals._t)._totalLockedAmount == 0)
            {
                continue;
            }

            locals.cnt++;
            output.averageAPY += div(state._currentRoundInfo.get(locals._t)._epochBonusAmount * 10000000ULL, state._currentRoundInfo.get(locals._t)._totalLockedAmount);
        }

        output.averageAPY = div(output.averageAPY, locals.cnt * 1ULL);
        
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
            output.boostedAmount += state.statsInfo.get(locals._t).boostedAmount;
            output.burnedAmount += state.statsInfo.get(locals._t).burnedAmount;
            output.rewardedAmount += state.statsInfo.get(locals._t).rewardedAmount;

            output.averageBurnedPercent += div(state.statsInfo.get(locals._t).burnedAmount * 10000000, state._initialRoundInfo.get(locals._t)._epochBonusAmount);
            output.averageBoostedPercent += div(state.statsInfo.get(locals._t).boostedAmount * 10000000, state._initialRoundInfo.get(locals._t)._epochBonusAmount);
            output.averageRewardedPercent += div(state.statsInfo.get(locals._t).rewardedAmount * 10000000, state._initialRoundInfo.get(locals._t)._epochBonusAmount);
        }

        output.averageBurnedPercent = div(output.averageBurnedPercent, qpi.epoch() - 138ULL);
        output.averageBoostedPercent = div(output.averageBoostedPercent, qpi.epoch() - 138ULL);
        output.averageRewardedPercent = div(output.averageRewardedPercent, qpi.epoch() - 138ULL);

    }

    PUBLIC_FUNCTION(getBurnedAndBoostedStatsPerEpoch)
    {
        output.boostedAmount = state.statsInfo.get(input.epoch).boostedAmount;
        output.burnedAmount = state.statsInfo.get(input.epoch).burnedAmount;
        output.rewardedAmount = state.statsInfo.get(input.epoch).rewardedAmount;

        output.burnedPercent = div(state.statsInfo.get(input.epoch).burnedAmount * 10000000, state._initialRoundInfo.get(input.epoch)._epochBonusAmount);
        output.boostedPercent = div(state.statsInfo.get(input.epoch).boostedAmount * 10000000, state._initialRoundInfo.get(input.epoch)._epochBonusAmount);
        output.rewardedPercent = div(state.statsInfo.get(input.epoch).rewardedAmount * 10000000, state._initialRoundInfo.get(input.epoch)._epochBonusAmount);
    
    }

    struct getUserLockedInfo_locals {
        uint32 _t;
        uint32 startIndex;
        uint32 endIndex;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getUserLockedInfo)
    {
        locals.startIndex = state._epochIndex.get(input.epoch).startIndex;
        locals.endIndex = state._epochIndex.get(input.epoch).endIndex;
        
        for(locals._t = locals.startIndex; locals._t < locals.endIndex; locals._t++) 
        {
            if(state.locker.get(locals._t).ID == input.user) 
            {
                output.lockedAmount = state.locker.get(locals._t)._lockedAmount; 
                return;
            }
        }
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
        locals.endIndex = state._epochIndex.get(qpi.epoch()).endIndex;
        
        for(locals._t = 0; locals._t < locals.endIndex; locals._t++) 
        {
            if(state.locker.get(locals._t)._lockedAmount > 0 && state.locker.get(locals._t).ID == input.user) 
            {
            
                locals.lockedWeeks = qpi.epoch() - state.locker.get(locals._t)._lockedEpoch;
                locals.bn = 1ULL<<locals.lockedWeeks;

                output.status += locals.bn;
            }
        }

    }
    
    struct getEndedStatus_locals {
        uint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getEndedStatus)
    {
        output.earlyRewardedAmount = 0;
        output.earlyUnlockedAmount = 0;
        output.fullyRewardedAmount = 0;
        output.fullyUnlockedAmount = 0;

        for(locals._t = 0; locals._t < state._earlyUnlockedCnt; locals._t++) 
        {
            if(state.earlyUnlocker.get(locals._t)._unlockedID == input.user) 
            {
                output.earlyRewardedAmount = state.earlyUnlocker.get(locals._t)._rewardedAmount;
                output.earlyUnlockedAmount = state.earlyUnlocker.get(locals._t)._unlockedAmount;

                break ;
            }
        }

        for(locals._t = 0; locals._t < state._fullyUnlockedCnt; locals._t++) 
        {
            if(state.fullyUnlocker.get(locals._t)._unlockedID == input.user) 
            {
                output.fullyRewardedAmount = state.fullyUnlocker.get(locals._t)._rewardedAmount;
                output.fullyUnlockedAmount = state.fullyUnlocker.get(locals._t)._unlockedAmount;
            
                return ;
            }
        }
    }

    struct lock_locals {

        LockInfo newLocker;
        RoundInfo updatedRoundInfo;
        EpochIndexInfo tmpIndex;
        QEARNLogger log;
        uint32 t;
        uint32 endIndex;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(lock)
    {
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

        locals.endIndex = state._epochIndex.get(qpi.epoch()).endIndex;

        for(locals.t = state._epochIndex.get(qpi.epoch()).startIndex ; locals.t < locals.endIndex; locals.t++) 
        {

            if(state.locker.get(locals.t).ID == qpi.invocator()) 
            {      // the case to be locked several times at one epoch, at that time, this address already located in state.Locker array, the amount will be increased as current locking amount.
                if(state.locker.get(locals.t)._lockedAmount + qpi.invocationReward() > QEARN_MAX_LOCK_AMOUNT)
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

                locals.newLocker._lockedAmount = state.locker.get(locals.t)._lockedAmount + qpi.invocationReward();
                locals.newLocker._lockedEpoch = qpi.epoch();
                locals.newLocker.ID = qpi.invocator();

                state.locker.set(locals.t, locals.newLocker);

                locals.updatedRoundInfo._totalLockedAmount = state._initialRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
                locals.updatedRoundInfo._epochBonusAmount = state._initialRoundInfo.get(qpi.epoch())._epochBonusAmount;
                state._initialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

                locals.updatedRoundInfo._totalLockedAmount = state._currentRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
                locals.updatedRoundInfo._epochBonusAmount = state._currentRoundInfo.get(qpi.epoch())._epochBonusAmount;
                state._currentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);
                
                output.returnCode = QEARN_LOCK_SUCCESS;          //  additional locking of this epoch is succeed

                locals.log = {QEARN_CONTRACT_INDEX, qpi.invocator(), SELF, qpi.invocationReward(), QearnSuccessLocking, 0};
                LOG_INFO(locals.log);
                return ;
            }

        }

        if(locals.endIndex == QEARN_MAX_LOCKS - 1) 
        {
            output.returnCode = QEARN_OVERFLOW_USER;

            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), qpi.invocationReward(), QearnOverflowUser, 0};
            LOG_INFO(locals.log);

            if(qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;                        // overflow users in Qearn
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

        state.locker.set(locals.endIndex, locals.newLocker);

        locals.tmpIndex.startIndex = state._epochIndex.get(qpi.epoch()).startIndex;
        locals.tmpIndex.endIndex = locals.endIndex + 1;
        state._epochIndex.set(qpi.epoch(), locals.tmpIndex);

        locals.updatedRoundInfo._totalLockedAmount = state._initialRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
        locals.updatedRoundInfo._epochBonusAmount = state._initialRoundInfo.get(qpi.epoch())._epochBonusAmount;
        state._initialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        locals.updatedRoundInfo._totalLockedAmount = state._currentRoundInfo.get(qpi.epoch())._totalLockedAmount + qpi.invocationReward();
        locals.updatedRoundInfo._epochBonusAmount = state._currentRoundInfo.get(qpi.epoch())._epochBonusAmount;
        state._currentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        output.returnCode = QEARN_LOCK_SUCCESS;            //  new locking of this epoch is succeed

        locals.log = {QEARN_CONTRACT_INDEX, qpi.invocator(), SELF, qpi.invocationReward(), QearnSuccessLocking, 0};
        LOG_INFO(locals.log);
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
        sint64 divCalcu;
        uint32 earlyUnlockingPercent;
        uint32 burnPercent;
        uint32 indexOfinvocator;
        uint32 _t;
        uint32 countOfLastVacancy;
        uint32 countOfLockedEpochs;
        uint32 startIndex;
        uint32 endIndex;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(unlock)
    {
        if (input.lockedEpoch > QEARN_MAX_EPOCHS || input.lockedEpoch < QEARN_INITIAL_EPOCH)
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

        locals.indexOfinvocator = QEARN_MAX_LOCKS;
        locals.startIndex = state._epochIndex.get(input.lockedEpoch).startIndex;
        locals.endIndex = state._epochIndex.get(input.lockedEpoch).endIndex;

        for(locals._t = locals.startIndex ; locals._t < locals.endIndex; locals._t++) 
        {

            if(state.locker.get(locals._t).ID == qpi.invocator()) 
            { 
                if(state.locker.get(locals._t)._lockedAmount < input.amount) 
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
        if(state.locker.get(locals.indexOfinvocator)._lockedAmount - input.amount < QEARN_MINIMUM_LOCKING_AMOUNT) 
        {
            locals.amountOfUnlocking = state.locker.get(locals.indexOfinvocator)._lockedAmount;
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

        locals.rewardPercent = div(state._currentRoundInfo.get(input.lockedEpoch)._epochBonusAmount * 10000000ULL, state._currentRoundInfo.get(input.lockedEpoch)._totalLockedAmount);
        locals.divCalcu = div(locals.rewardPercent * locals.amountOfUnlocking , 100ULL);
        locals.amountOfReward = div(locals.divCalcu * locals.earlyUnlockingPercent * 1ULL , 10000000ULL);
        locals.amountOfburn = div(locals.divCalcu * locals.burnPercent * 1ULL, 10000000ULL);

        qpi.transfer(qpi.invocator(), locals.amountOfUnlocking + locals.amountOfReward);
        locals.transferAmount = locals.amountOfUnlocking + locals.amountOfReward;
        
        locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), locals.transferAmount, QearnSuccessEarlyUnlocking, 0};
        LOG_INFO(locals.log);

        qpi.burn(locals.amountOfburn);

        if(input.lockedEpoch != qpi.epoch())
        {
            locals.tmpStats.burnedAmount = state.statsInfo.get(input.lockedEpoch).burnedAmount + locals.amountOfburn;
            locals.tmpStats.rewardedAmount = state.statsInfo.get(input.lockedEpoch).rewardedAmount + locals.amountOfReward;
            locals.tmpStats.boostedAmount = state.statsInfo.get(input.lockedEpoch).boostedAmount + div(locals.divCalcu * (100 - locals.burnPercent - locals.earlyUnlockingPercent) * 1ULL, 10000000ULL);

            state.statsInfo.set(input.lockedEpoch, locals.tmpStats);
        }

        locals.updatedRoundInfo._totalLockedAmount = state._currentRoundInfo.get(input.lockedEpoch)._totalLockedAmount - locals.amountOfUnlocking;
        locals.updatedRoundInfo._epochBonusAmount = state._currentRoundInfo.get(input.lockedEpoch)._epochBonusAmount - locals.amountOfReward - locals.amountOfburn;

        state._currentRoundInfo.set(input.lockedEpoch, locals.updatedRoundInfo);

        if(qpi.epoch() == input.lockedEpoch)
        {
            locals.updatedRoundInfo._totalLockedAmount = state._initialRoundInfo.get(input.lockedEpoch)._totalLockedAmount - locals.amountOfUnlocking;
            locals.updatedRoundInfo._epochBonusAmount = state._initialRoundInfo.get(input.lockedEpoch)._epochBonusAmount;
            
            state._initialRoundInfo.set(input.lockedEpoch, locals.updatedRoundInfo);
        }

        if(state.locker.get(locals.indexOfinvocator)._lockedAmount == locals.amountOfUnlocking)
        {
            locals.updatedUserInfo.ID = NULL_ID;
            locals.updatedUserInfo._lockedAmount = 0;
            locals.updatedUserInfo._lockedEpoch = 0;
        }
        else 
        {
            locals.updatedUserInfo.ID = qpi.invocator();
            locals.updatedUserInfo._lockedAmount = state.locker.get(locals.indexOfinvocator)._lockedAmount - locals.amountOfUnlocking;
            locals.updatedUserInfo._lockedEpoch = state.locker.get(locals.indexOfinvocator)._lockedEpoch;
        }

        state.locker.set(locals.indexOfinvocator, locals.updatedUserInfo);

        if(state._currentRoundInfo.get(input.lockedEpoch)._totalLockedAmount == 0 && input.lockedEpoch != qpi.epoch()) 
        {
            
            // If all users have unlocked early, burn bonus
            qpi.burn(state._currentRoundInfo.get(input.lockedEpoch)._epochBonusAmount);

            locals.updatedRoundInfo._totalLockedAmount = 0;
            locals.updatedRoundInfo._epochBonusAmount = 0;

            state._currentRoundInfo.set(input.lockedEpoch, locals.updatedRoundInfo);

        }

        if(input.lockedEpoch != qpi.epoch()) 
        {

            locals.unlockerInfo._unlockedID = qpi.invocator();
            
            for(locals._t = 0; locals._t < state._earlyUnlockedCnt; locals._t++) 
            {
                if(state.earlyUnlocker.get(locals._t)._unlockedID == qpi.invocator()) 
                {

                    locals.unlockerInfo._rewardedAmount = state.earlyUnlocker.get(locals._t)._rewardedAmount + locals.amountOfReward;
                    locals.unlockerInfo._unlockedAmount = state.earlyUnlocker.get(locals._t)._unlockedAmount + locals.amountOfUnlocking;

                    state.earlyUnlocker.set(locals._t, locals.unlockerInfo);

                    break;
                }
            }

            if(locals._t == state._earlyUnlockedCnt && state._earlyUnlockedCnt < QEARN_MAX_USERS) 
            {
                locals.unlockerInfo._rewardedAmount = locals.amountOfReward;
                locals.unlockerInfo._unlockedAmount = locals.amountOfUnlocking;

                state.earlyUnlocker.set(locals._t, locals.unlockerInfo);
                state._earlyUnlockedCnt++;
            }

        }

        output.returnCode = QEARN_UNLOCK_SUCCESS; //  unlock is succeed
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

        REGISTER_USER_PROCEDURE(lock, 1);
		REGISTER_USER_PROCEDURE(unlock, 2);

	}

    struct BEGIN_EPOCH_locals
    {
        HistoryInfo INITIALIZE_HISTORY;
        LockInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;
        StatsInfo INITIALIZE_STATS;

        uint32 t;
        bit status;
        uint64 pre_epoch_balance;
        uint64 current_balance;
        Entity entity;
        uint32 locked_epoch;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        qpi.getEntity(SELF, locals.entity);
        locals.current_balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        locals.pre_epoch_balance = 0ULL;
        locals.locked_epoch = qpi.epoch() - 52;
        for(locals.t = qpi.epoch() - 1; locals.t >= locals.locked_epoch; locals.t--) 
        {
            locals.pre_epoch_balance += state._currentRoundInfo.get(locals.t)._epochBonusAmount + state._currentRoundInfo.get(locals.t)._totalLockedAmount;
        }

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

        state._initialRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
        state._currentRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
	}

    struct END_EPOCH_locals 
    {
        HistoryInfo INITIALIZE_HISTORY;
        LockInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;
        EpochIndexInfo tmpEpochIndex;
        StatsInfo tmpStats; 
        QEARNLogger log;

        uint64 _rewardPercent;
        uint64 _rewardAmount;
        uint64 _burnAmount;
        sint64 transferAmount;
        uint32 lockedEpoch;
        uint32 startEpoch;
        uint32 _t;
        sint32 st;
        sint32 en;
        uint32 endIndex;

    };

	END_EPOCH_WITH_LOCALS()
    {
        state._earlyUnlockedCnt = 0;
        state._fullyUnlockedCnt = 0;
        locals.lockedEpoch = qpi.epoch() - 52;
        locals.endIndex = state._epochIndex.get(locals.lockedEpoch).endIndex;
        
        locals._burnAmount = state._currentRoundInfo.get(locals.lockedEpoch)._epochBonusAmount;
        
        locals._rewardPercent = div(state._currentRoundInfo.get(locals.lockedEpoch)._epochBonusAmount * 10000000ULL, state._currentRoundInfo.get(locals.lockedEpoch)._totalLockedAmount);
        locals.tmpStats.rewardedAmount = state.statsInfo.get(locals.lockedEpoch).rewardedAmount;

        for(locals._t = state._epochIndex.get(locals.lockedEpoch).startIndex; locals._t < locals.endIndex; locals._t++) 
        {
            if(state.locker.get(locals._t)._lockedAmount == 0) 
            {
                continue;
            }

            ASSERT(state.locker.get(locals._t)._lockedEpoch == locals.lockedEpoch);

            locals._rewardAmount = div(state.locker.get(locals._t)._lockedAmount * locals._rewardPercent, 10000000ULL);
            qpi.transfer(state.locker.get(locals._t).ID, locals._rewardAmount + state.locker.get(locals._t)._lockedAmount);

            locals.transferAmount = locals._rewardAmount + state.locker.get(locals._t)._lockedAmount;
            locals.log = {QEARN_CONTRACT_INDEX, SELF, qpi.invocator(), locals.transferAmount, QearnSuccessFullyUnlocking, 0};
            LOG_INFO(locals.log);

            if(state._fullyUnlockedCnt < QEARN_MAX_USERS) 
            {

                locals.INITIALIZE_HISTORY._unlockedID = state.locker.get(locals._t).ID;
                locals.INITIALIZE_HISTORY._unlockedAmount = state.locker.get(locals._t)._lockedAmount;
                locals.INITIALIZE_HISTORY._rewardedAmount = locals._rewardAmount;

                state.fullyUnlocker.set(state._fullyUnlockedCnt, locals.INITIALIZE_HISTORY);

                state._fullyUnlockedCnt++;
            }

            locals.INITIALIZE_USER.ID = NULL_ID;
            locals.INITIALIZE_USER._lockedAmount = 0;
            locals.INITIALIZE_USER._lockedEpoch = 0;

            state.locker.set(locals._t, locals.INITIALIZE_USER);

            locals._burnAmount -= locals._rewardAmount;
            locals.tmpStats.rewardedAmount += locals._rewardAmount;
        }

        locals.tmpEpochIndex.startIndex = 0;
        locals.tmpEpochIndex.endIndex = 0;
        state._epochIndex.set(locals.lockedEpoch, locals.tmpEpochIndex);

        locals.startEpoch = locals.lockedEpoch + 1;
        if (locals.startEpoch < QEARN_INITIAL_EPOCH)
            locals.startEpoch = QEARN_INITIAL_EPOCH;

        // remove all gaps in Locker array (from beginning) and update epochIndex
        locals.tmpEpochIndex.startIndex = 0;
        for(locals._t = locals.startEpoch; locals._t <= qpi.epoch(); locals._t++)
        {
            // This for loop iteration moves all elements of one epoch the to start of its range of the Locker array.
            // The startIndex is given by the end of the range of the previous epoch, the new endIndex is found in the
            // gap removal process.
            locals.st = locals.tmpEpochIndex.startIndex;
            locals.en = state._epochIndex.get(locals._t).endIndex;
            ASSERT(locals.st <= locals.en);

            while(locals.st < locals.en)
            {
                // try to set locals.st to first empty slot
                while (state.locker.get(locals.st)._lockedAmount && locals.st < locals.en)
                {
                    locals.st++;
                }

                // try set locals.en to last non-empty slot in epoch
                --locals.en;
                while (!state.locker.get(locals.en)._lockedAmount && locals.st < locals.en)
                {
                    locals.en--;
                }

                // if st and en meet, there are no gaps to be closed by moving in this epoch range
                if (locals.st >= locals.en)
                {
                    // make locals.en point behind last element again
                    ++locals.en;
                    break;
                }

                // move entry from locals.en to locals.st
                state.locker.set(locals.st, state.locker.get(locals.en));

                // make locals.en slot empty -> locals.en points behind last element again
                locals.INITIALIZE_USER.ID = NULL_ID;
                locals.INITIALIZE_USER._lockedAmount = 0;
                locals.INITIALIZE_USER._lockedEpoch = 0;
                state.locker.set(locals.en, locals.INITIALIZE_USER);
            }

            // update epoch index
            locals.tmpEpochIndex.endIndex = locals.en;
            state._epochIndex.set(locals._t, locals.tmpEpochIndex);

            // set start index of next epoch to end index of current epoch
            locals.tmpEpochIndex.startIndex = locals.tmpEpochIndex.endIndex;
        }

        locals.tmpEpochIndex.endIndex = locals.tmpEpochIndex.startIndex;
        state._epochIndex.set(qpi.epoch() + 1, locals.tmpEpochIndex);

        qpi.burn(locals._burnAmount);

        locals.tmpStats.boostedAmount = state.statsInfo.get(locals.lockedEpoch).boostedAmount;
        locals.tmpStats.burnedAmount = state.statsInfo.get(locals.lockedEpoch).burnedAmount + locals._burnAmount;

        state.statsInfo.set(locals.lockedEpoch, locals.tmpStats);
	}
};
