using namespace QPI;

constexpr uint64 QEARN_MINIMUM_LOCKING_AMOUNT = 10000000;
constexpr uint64 QEARN_MAX_LOCKS = 4194304;
constexpr uint64 QEARN_MAX_EPOCHS = 4096;
constexpr uint64 QEARN_MAX_USERS = 131072;
constexpr uint64 QEARN_MAX_LOCK_AMOUNT = 1000000000000ULL;
constexpr uint64 QEARN_INITIAL_EPOCH = 128;                             //  we need to change this epoch when merging

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

constexpr sint32 QEARN_INVALID_INPUT_AMOUNT = -1;
constexpr sint32 QEARN_LOCK_SUCCESS = 0;
constexpr sint32 QEARN_INVALID_INPUT_LOCKED_EPOCH = 1;
constexpr sint32 QEARN_INVALID_INPUT_UNLOCK_AMOUNT = 2;
constexpr sint32 QEARN_EMPTY_LOCKED = 3;
constexpr sint32 QEARN_UNLOCK_SUCCESS = 4;
constexpr sint32 QEARN_OVERFLOW_USER = 5;
constexpr sint32 QEARN_LIMIT_LOCK = 6;

struct QEARN2
{
};

struct QEARN : public ContractBase
{
public:
    struct getLockInforPerEpoch_input {
		uint32 Epoch;                             /* epoch number to get information */
    };

    struct getLockInforPerEpoch_output {
        uint64 LockedAmount;                      /* initial total locked amount in epoch */
        uint64 BonusAmount;                       /* initial bonus amount in epoch*/
        uint64 CurrentLockedAmount;               /* total locked amount in epoch. exactly the amount excluding the amount unlocked early*/
        uint64 CurrentBonusAmount;                /* bonus amount in epoch excluding the early unlocking */
        uint64 Yield;                             /* Yield calculated by 10000000 multiple*/
    };

    struct getUserLockedInfor_input {
        id user;
        uint32 epoch;
    };

    struct getUserLockedInfor_output {
        uint64 LockedAmount;                   /* the amount user locked at input.epoch */
    };

    /*
        getStateOfRound FUNCTION

        getStateOfRound function returns following.

        0 = open epoch,not started yet
        1 = running epoch
        2 = ended epoch(>52weeks)
    */
    struct getStateOfRound_input {
        uint32 Epoch;
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

        output.Early_Rewarded_Amount returns the amount rewarded by unlocking early at current epoch
        output.Early_Unlocked_Amount returns the amount unlocked by unlocking early at current epoch
        output.Fully_Rewarded_Amount returns the amount rewarded by unlocking fully at the end of previous epoch
        output.Fully_Unlocked_Amount returns the amount unlocked by unlocking fully at the end of previous epoch

        let's assume that current epoch is 170, user unlocked the 15B qu totally at this epoch and he got the 30B qu of reward.
        in this case, output.Early_Unlocked_Amount = 15B qu, output.Early_Rewarded_Amount = 30B qu
        if this user unlocks 3B qu additionally at this epoch and rewarded 6B qu, 
        in this case, output.Early_Unlocked_Amount = 18B qu, output.Early_Rewarded_Amount = 36B qu
        state.EarlyUnlocker array would be initialized at the end of every epoch

        let's assume also that current epoch is 170, user got the 15B(locked amount for 52 weeks) + 10B(rewarded amount for 52 weeks) at the end of epoch 169.
        in this case, output.Fully_Rewarded_Amount = 10B, output.Fully_Unlocked_Amount = 15B
        state.FullyUnlocker array would be decided with distributions at the end of every epoch

        state.EarlyUnlocker, state.FullyUnlocker arrays would be initialized and decided by following expression at the END_EPOCH_WITH_LOCALS function.
        state._EarlyUnlocked_cnt = 0;
        state._FullyUnlocked_cnt = 0;
    */

    struct getEndedStatus_input {
        id user;
    };

    struct getEndedStatus_output {
        uint64 Fully_Unlocked_Amount;
        uint64 Fully_Rewarded_Amount;
        uint64 Early_Unlocked_Amount;
        uint64 Early_Rewarded_Amount;
    };

	struct lock_input {	
    };

    struct lock_output {	
        sint32 returnCode;
    };

    struct unlock_input {
        uint64 Amount;                            /* unlocking amount */	
        uint32 Locked_Epoch;                      /* locked epoch */
    };

    struct unlock_output {
        sint32 returnCode;
    };

private:

    struct RoundInfo {

        uint64 _Total_Locked_Amount;            // The initial total locked amount in any epoch.  Max Epoch is 65535
        uint64 _Epoch_Bonus_Amount;             // The initial bonus amount per an epoch.         Max Epoch is 65535 

    };

    array<RoundInfo, QEARN_MAX_EPOCHS> _InitialRoundInfo;
    array<RoundInfo, QEARN_MAX_EPOCHS> _CurrentRoundInfo;

    struct EpochIndexInfo {

        uint32 startIndex;
        uint32 endIndex;
    };

    array<EpochIndexInfo, QEARN_MAX_EPOCHS> EpochIndex;

    struct LockInfo {

        uint64 _Locked_Amount;
        id ID;
        uint32 _Locked_Epoch;
        
    };

    array<LockInfo, QEARN_MAX_LOCKS> Locker;

    struct HistoryInfo {

        uint64 _Unlocked_Amount;
        uint64 _Rewarded_Amount;
        id _Unlocked_ID;

    };

    array<HistoryInfo, QEARN_MAX_USERS> EarlyUnlocker;
    array<HistoryInfo, QEARN_MAX_USERS> FullyUnlocker;
    
    uint64 remain_amount;          //  The remain amount boosted by last early unlocker of one round, this amount will put in next round

    uint32 _EarlyUnlocked_cnt;
    uint32 _FullyUnlocked_cnt;

    struct getStateOfRound_locals {
        uint32 first_epoch;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getStateOfRound)
        if(input.Epoch < QEARN_INITIAL_EPOCH) 
        {                                                            // non staking
            output.state = 2; 
            return ;
        }
        if(input.Epoch > qpi.epoch()) 
        {
            output.state = 0;                                     // opening round, not started yet
        }
        locals.first_epoch = qpi.epoch() - 52;
        if(input.Epoch <= qpi.epoch() && input.Epoch >= locals.first_epoch) 
        {
            output.state = 1;       // running round, available unlocking early
        }
        if(input.Epoch < locals.first_epoch) 
        {
            output.state = 2;       // ended round
        }
    _

    PUBLIC_FUNCTION(getLockInforPerEpoch)

        output.BonusAmount = state._InitialRoundInfo.get(input.Epoch)._Epoch_Bonus_Amount;
        output.LockedAmount = state._InitialRoundInfo.get(input.Epoch)._Total_Locked_Amount;
        output.CurrentBonusAmount = state._CurrentRoundInfo.get(input.Epoch)._Epoch_Bonus_Amount;
        output.CurrentLockedAmount = state._CurrentRoundInfo.get(input.Epoch)._Total_Locked_Amount;
        if(state._CurrentRoundInfo.get(input.Epoch)._Total_Locked_Amount) 
        {
            output.Yield = state._CurrentRoundInfo.get(input.Epoch)._Epoch_Bonus_Amount * 10000000ULL / state._CurrentRoundInfo.get(input.Epoch)._Total_Locked_Amount;
        }
        else 
        {
            output.Yield = 0ULL;
        }
    _

    struct getUserLockedInfor_locals {
        uint32 _t;
        uint32 startIndex;
        uint32 endIndex;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getUserLockedInfor)

        locals.startIndex = state.EpochIndex.get(input.epoch).startIndex;
        locals.endIndex = state.EpochIndex.get(input.epoch).endIndex;
        
        for(locals._t = locals.startIndex; locals._t < locals.endIndex; locals._t++) 
        {
            if(state.Locker.get(locals._t).ID == input.user) 
            {
                output.LockedAmount = state.Locker.get(locals._t)._Locked_Amount; 
                return;
            }
        }
    _

    struct getUserLockStatus_locals {
        uint64 bn;
        uint32 _t;
        uint32 _r;
        uint32 endIndex;
        uint8 lockedWeeks;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getUserLockStatus)

        output.status = 0ULL;
        locals.endIndex = state.EpochIndex.get(qpi.epoch()).endIndex;
        
        for(locals._t = 0; locals._t < locals.endIndex; locals._t++) 
        {
            if(state.Locker.get(locals._t)._Locked_Amount > 0 && state.Locker.get(locals._t).ID == input.user) 
            {
            
                locals.lockedWeeks = qpi.epoch() - state.Locker.get(locals._t)._Locked_Epoch;
                locals.bn = 1ULL<<locals.lockedWeeks;

                output.status += locals.bn;
            }
        }

    _
    
    struct getEndedStatus_locals {
        uint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getEndedStatus)

        output.Early_Rewarded_Amount = 0;
        output.Early_Unlocked_Amount = 0;
        output.Fully_Rewarded_Amount = 0;
        output.Fully_Unlocked_Amount = 0;

        for(locals._t = 0; locals._t < state._EarlyUnlocked_cnt; locals._t++) 
        {
            if(state.EarlyUnlocker.get(locals._t)._Unlocked_ID == input.user) 
            {
                output.Early_Rewarded_Amount = state.EarlyUnlocker.get(locals._t)._Rewarded_Amount;
                output.Early_Unlocked_Amount = state.EarlyUnlocker.get(locals._t)._Unlocked_Amount;

                break ;
            }
        }

        for(locals._t = 0; locals._t < state._FullyUnlocked_cnt; locals._t++) 
        {
            if(state.FullyUnlocker.get(locals._t)._Unlocked_ID == input.user) 
            {
                output.Fully_Rewarded_Amount = state.FullyUnlocker.get(locals._t)._Rewarded_Amount;
                output.Fully_Unlocked_Amount = state.FullyUnlocker.get(locals._t)._Unlocked_Amount;
            
                return ;
            }
        }
    _

    struct lock_locals {

        LockInfo newLocker;
        RoundInfo updatedRoundInfo;
        EpochIndexInfo tmpIndex;
        uint32 t;
        uint32 endIndex;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(lock)
    
        if(qpi.invocationReward() < QEARN_MINIMUM_LOCKING_AMOUNT)
        {
            output.returnCode = QEARN_INVALID_INPUT_AMOUNT;         // if the amount of locking is less than 10M, it should be failed to lock.
            
            if(qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.endIndex = state.EpochIndex.get(qpi.epoch()).endIndex;

        for(locals.t = state.EpochIndex.get(qpi.epoch()).startIndex ; locals.t < locals.endIndex; locals.t++) 
        {

            if(state.Locker.get(locals.t).ID == qpi.invocator()) 
            {      // the case to be locked several times at one epoch, at that time, this address already located in state.Locker array, the amount will be increased as current locking amount.
                if(state.Locker.get(locals.t)._Locked_Amount + qpi.invocationReward() > QEARN_MAX_LOCK_AMOUNT)
                {
                    output.returnCode = QEARN_LIMIT_LOCK;
                    if(qpi.invocationReward() > 0) 
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return;
                }

                locals.newLocker._Locked_Amount = state.Locker.get(locals.t)._Locked_Amount + qpi.invocationReward();
                locals.newLocker._Locked_Epoch = qpi.epoch();
                locals.newLocker.ID = qpi.invocator();

                state.Locker.set(locals.t, locals.newLocker);

                locals.updatedRoundInfo._Total_Locked_Amount = state._InitialRoundInfo.get(qpi.epoch())._Total_Locked_Amount + qpi.invocationReward();
                locals.updatedRoundInfo._Epoch_Bonus_Amount = state._InitialRoundInfo.get(qpi.epoch())._Epoch_Bonus_Amount;
                state._InitialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

                locals.updatedRoundInfo._Total_Locked_Amount = state._CurrentRoundInfo.get(qpi.epoch())._Total_Locked_Amount + qpi.invocationReward();
                locals.updatedRoundInfo._Epoch_Bonus_Amount = state._CurrentRoundInfo.get(qpi.epoch())._Epoch_Bonus_Amount;
                state._CurrentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);
                
                output.returnCode = QEARN_LOCK_SUCCESS;          //  additional locking of this epoch is succeed
                return ;
            }

        }

        if(locals.endIndex == QEARN_MAX_LOCKS - 1) 
        {
            output.returnCode = QEARN_OVERFLOW_USER;
            if(qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;                        // overflow users in Qearn
        }
        
        if(qpi.invocationReward() > QEARN_MAX_LOCK_AMOUNT)
        {
            output.returnCode = QEARN_LIMIT_LOCK;
            if(qpi.invocationReward() > 0) 
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.newLocker.ID = qpi.invocator();
        locals.newLocker._Locked_Amount = qpi.invocationReward();
        locals.newLocker._Locked_Epoch = qpi.epoch();

        state.Locker.set(locals.endIndex, locals.newLocker);

        locals.tmpIndex.startIndex = state.EpochIndex.get(qpi.epoch()).startIndex;
        locals.tmpIndex.endIndex = locals.endIndex + 1;
        state.EpochIndex.set(qpi.epoch(), locals.tmpIndex);

        locals.updatedRoundInfo._Total_Locked_Amount = state._InitialRoundInfo.get(qpi.epoch())._Total_Locked_Amount + qpi.invocationReward();
        locals.updatedRoundInfo._Epoch_Bonus_Amount = state._InitialRoundInfo.get(qpi.epoch())._Epoch_Bonus_Amount;
        state._InitialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        locals.updatedRoundInfo._Total_Locked_Amount = state._CurrentRoundInfo.get(qpi.epoch())._Total_Locked_Amount + qpi.invocationReward();
        locals.updatedRoundInfo._Epoch_Bonus_Amount = state._CurrentRoundInfo.get(qpi.epoch())._Epoch_Bonus_Amount;
        state._CurrentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        output.returnCode = QEARN_LOCK_SUCCESS;            //  new locking of this epoch is succeed
    _

    struct unlock_locals {

        RoundInfo updatedRoundInfo;
        LockInfo updatedUserInfo;
        HistoryInfo unlockerInfo;
        
        uint64 AmountOfUnlocking;
        uint64 AmountOfReward;
        uint64 AmountOfburn;
        uint64 RewardPercent;
        sint64 divCalcu;
        uint32 indexOfinvocator;
        uint32 t;
        uint32 count_Of_last_vacancy;
        uint32 locked_weeks;
        uint32 startIndex;
        uint32 endIndex;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(unlock)

        if(input.Locked_Epoch > QEARN_MAX_EPOCHS || input.Amount < QEARN_MINIMUM_LOCKING_AMOUNT) 
        {

            output.returnCode = QEARN_INVALID_INPUT_LOCKED_EPOCH;               //   if user try to unlock with wrong locked epoch, it should be failed to unlock.
            return ;

        }

        locals.indexOfinvocator = QEARN_MAX_LOCKS;
        locals.startIndex = state.EpochIndex.get(input.Locked_Epoch).startIndex;
        locals.endIndex = state.EpochIndex.get(input.Locked_Epoch).endIndex;

        for(locals.t = locals.startIndex ; locals.t < locals.endIndex; locals.t++) 
        {

            if(state.Locker.get(locals.t).ID == qpi.invocator()) 
            { 
                if(state.Locker.get(locals.t)._Locked_Amount < input.Amount) 
                {

                    output.returnCode = QEARN_INVALID_INPUT_UNLOCK_AMOUNT;  //  if the amount to be wanted to unlock is more than locked amount, it should be failed to unlock
                    return ;  

                }
                else 
                {
                    locals.indexOfinvocator = locals.t;
                    break;
                }
            }

        }

        if(locals.indexOfinvocator == QEARN_MAX_LOCKS) 
        {
            
            output.returnCode = QEARN_EMPTY_LOCKED;     //   if there is no any locked info in state.Locker array, it shows this address didn't lock at the epoch (input.Locked_Epoch)
            return ;  
        }

        /* the rest amount after unlocking should be more than MINIMUM_LOCKING_AMOUNT */
        if(state.Locker.get(locals.indexOfinvocator)._Locked_Amount - input.Amount < QEARN_MINIMUM_LOCKING_AMOUNT) 
        {
            locals.AmountOfUnlocking = state.Locker.get(locals.indexOfinvocator)._Locked_Amount;
        }
        else 
        {
            locals.AmountOfUnlocking = input.Amount;
        }

        locals.RewardPercent = QPI::div(state._CurrentRoundInfo.get(input.Locked_Epoch)._Epoch_Bonus_Amount * 10000000ULL, state._CurrentRoundInfo.get(input.Locked_Epoch)._Total_Locked_Amount);
        locals.locked_weeks = qpi.epoch() - input.Locked_Epoch - 1;
        locals.divCalcu = QPI::div(locals.RewardPercent * locals.AmountOfUnlocking , 100ULL);

        if(qpi.epoch() - input.Locked_Epoch >= 0 && locals.locked_weeks <= 3) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_0_3, 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_0_3 , 10000000ULL);
        }

        if(locals.locked_weeks >= 4 && locals.locked_weeks <= 7) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_4_7 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_4_7 , 10000000ULL);
        }

        if(locals.locked_weeks >= 8 && locals.locked_weeks <= 11) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_8_11 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_8_11 , 10000000ULL);
        }

        if(locals.locked_weeks >= 12 && locals.locked_weeks <= 15) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_12_15 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_12_15 , 10000000ULL);
        }

        if(locals.locked_weeks >= 16 && locals.locked_weeks <= 19) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_16_19 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_16_19 , 10000000ULL);
        }

        if(locals.locked_weeks >= 20 && locals.locked_weeks <= 23) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_20_23 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_20_23 , 10000000ULL);
        }

        if(locals.locked_weeks >= 24 && locals.locked_weeks <= 27) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_24_27 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_24_27 , 10000000ULL);
        }

        if(locals.locked_weeks >= 28 && locals.locked_weeks <= 31) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_28_31 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_28_31 , 10000000ULL);
        }

        if(locals.locked_weeks >= 32 && locals.locked_weeks <= 35) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_32_35 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_32_35 , 10000000ULL);
        }

        if(locals.locked_weeks >= 36 && locals.locked_weeks <= 39) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_36_39 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_36_39 , 10000000ULL);
        }

        if(locals.locked_weeks >= 40 && locals.locked_weeks <= 43) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_40_43 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_40_43 , 10000000ULL);
        }

        if(locals.locked_weeks >= 44 && locals.locked_weeks <= 47) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_44_47 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_44_47 , 10000000ULL);
        }

        if(locals.locked_weeks >= 48 && locals.locked_weeks <= 51) 
        {
            locals.AmountOfReward = QPI::div(locals.divCalcu * QEARN_EARLY_UNLOCKING_PERCENT_48_51 , 10000000ULL);
            locals.AmountOfburn = QPI::div(locals.divCalcu * QEARN_BURN_PERCENT_48_51 , 10000000ULL);
        }

        qpi.transfer(qpi.invocator(), locals.AmountOfUnlocking + locals.AmountOfReward);
        qpi.burn(locals.AmountOfburn);

        locals.updatedRoundInfo._Total_Locked_Amount = state._CurrentRoundInfo.get(input.Locked_Epoch)._Total_Locked_Amount - locals.AmountOfUnlocking;
        locals.updatedRoundInfo._Epoch_Bonus_Amount = state._CurrentRoundInfo.get(input.Locked_Epoch)._Epoch_Bonus_Amount - locals.AmountOfReward - locals.AmountOfburn;

        state._CurrentRoundInfo.set(input.Locked_Epoch, locals.updatedRoundInfo);

        if(state.Locker.get(locals.indexOfinvocator)._Locked_Amount == locals.AmountOfUnlocking)
        {
            locals.updatedUserInfo.ID = NULL_ID;
            locals.updatedUserInfo._Locked_Amount = 0;
            locals.updatedUserInfo._Locked_Epoch = 0;
        }
        else 
        {
            locals.updatedUserInfo.ID = qpi.invocator();
            locals.updatedUserInfo._Locked_Amount = state.Locker.get(locals.indexOfinvocator)._Locked_Amount - locals.AmountOfUnlocking;
            locals.updatedUserInfo._Locked_Epoch = state.Locker.get(locals.indexOfinvocator)._Locked_Epoch;
        }

        state.Locker.set(locals.indexOfinvocator, locals.updatedUserInfo);

        if(state._CurrentRoundInfo.get(input.Locked_Epoch)._Total_Locked_Amount == 0 && input.Locked_Epoch != qpi.epoch()) 
        {   // The case to be unlocked early all users of one epoch, at this time, boost amount will put in next round.
            
            state.remain_amount = state._CurrentRoundInfo.get(input.Locked_Epoch)._Epoch_Bonus_Amount;

            locals.updatedRoundInfo._Total_Locked_Amount = 0;
            locals.updatedRoundInfo._Epoch_Bonus_Amount = 0;

            state._CurrentRoundInfo.set(input.Locked_Epoch, locals.updatedRoundInfo);

        }

        if(input.Locked_Epoch != qpi.epoch()) 
        {

            locals.unlockerInfo._Unlocked_ID = qpi.invocator();
            
            for(locals.t = 0; locals.t < state._EarlyUnlocked_cnt; locals.t++) 
            {
                if(state.EarlyUnlocker.get(locals.t)._Unlocked_ID == qpi.invocator()) 
                {

                    locals.unlockerInfo._Rewarded_Amount = state.EarlyUnlocker.get(locals.t)._Rewarded_Amount + locals.AmountOfReward;
                    locals.unlockerInfo._Unlocked_Amount = state.EarlyUnlocker.get(locals.t)._Unlocked_Amount + locals.AmountOfUnlocking;

                    state.EarlyUnlocker.set(locals.t, locals.unlockerInfo);

                    break;
                }
            }

            if(locals.t == state._EarlyUnlocked_cnt && state._EarlyUnlocked_cnt < QEARN_MAX_USERS) 
            {
                locals.unlockerInfo._Rewarded_Amount = locals.AmountOfReward;
                locals.unlockerInfo._Unlocked_Amount = locals.AmountOfUnlocking;

                state.EarlyUnlocker.set(locals.t, locals.unlockerInfo);
                state._EarlyUnlocked_cnt++;
            }

        }

        output.returnCode = QEARN_UNLOCK_SUCCESS; //  unlock is succeed
    _

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
    
        REGISTER_USER_FUNCTION(getLockInforPerEpoch, 1);
        REGISTER_USER_FUNCTION(getUserLockedInfor, 2);
        REGISTER_USER_FUNCTION(getStateOfRound, 3);
        REGISTER_USER_FUNCTION(getUserLockStatus, 4);
        REGISTER_USER_FUNCTION(getEndedStatus, 5);

        REGISTER_USER_PROCEDURE(lock, 1);
		REGISTER_USER_PROCEDURE(unlock, 2);

	_

    struct BEGIN_EPOCH_locals
    {
        HistoryInfo INITIALIZE_HISTORY;
        LockInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;

        uint32 t;
        bit status;
        uint64 pre_epoch_balance;
        uint64 current_balance;
        ::Entity entity;
        uint32 locked_epoch;
    };

    BEGIN_EPOCH_WITH_LOCALS

        qpi.getEntity(SELF, locals.entity);
        locals.current_balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        locals.pre_epoch_balance = 0ULL;
        locals.locked_epoch = qpi.epoch() - 52;
        for(locals.t = qpi.epoch() - 1; locals.t >= locals.locked_epoch; locals.t--) 
        {
            locals.pre_epoch_balance += state._CurrentRoundInfo.get(locals.t)._Epoch_Bonus_Amount + state._CurrentRoundInfo.get(locals.t)._Total_Locked_Amount;
        }

        locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount = locals.current_balance - locals.pre_epoch_balance;
        locals.INITIALIZE_ROUNDINFO._Total_Locked_Amount = 0;

        state._InitialRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
        state._CurrentRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);

        if(locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount > 0) 
        {
            
            locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount += state.remain_amount;

            state._InitialRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
            state._CurrentRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);

            state.remain_amount = 0;

        }
	_

    struct END_EPOCH_locals 
    {
        HistoryInfo INITIALIZE_HISTORY;
        LockInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;
        EpochIndexInfo tmpEpochIndex;

        uint64 _reward_percent;
        uint64 _reward_amount;
        uint64 _burn_amount;
        uint32 locked_epoch;
        uint32 startEpoch;
        uint32 _t;
        sint32 st;
        sint32 en;
        uint32 empty_cnt;
        uint32 endIndex;

    };

	END_EPOCH_WITH_LOCALS

        state._EarlyUnlocked_cnt = 0;
        state._FullyUnlocked_cnt = 0;
        locals.locked_epoch = qpi.epoch() - 52;
        locals.endIndex = state.EpochIndex.get(locals.locked_epoch).endIndex;
        
        locals._burn_amount = state._CurrentRoundInfo.get(locals.locked_epoch)._Epoch_Bonus_Amount;
        
        locals._reward_percent = QPI::div(state._CurrentRoundInfo.get(locals.locked_epoch)._Epoch_Bonus_Amount * 10000000ULL, state._CurrentRoundInfo.get(locals.locked_epoch)._Total_Locked_Amount);

        for(locals._t = state.EpochIndex.get(locals.locked_epoch).startIndex; locals._t < locals.endIndex; locals._t++) 
        {
            if(state.Locker.get(locals._t)._Locked_Amount == 0) 
            {
                continue;
            }

            locals._reward_amount = QPI::div(state.Locker.get(locals._t)._Locked_Amount * locals._reward_percent, 10000000ULL);
            qpi.transfer(state.Locker.get(locals._t).ID, locals._reward_amount + state.Locker.get(locals._t)._Locked_Amount);

            if(state._FullyUnlocked_cnt < QEARN_MAX_USERS) 
            {

                locals.INITIALIZE_HISTORY._Unlocked_ID = state.Locker.get(locals._t).ID;
                locals.INITIALIZE_HISTORY._Unlocked_Amount = state.Locker.get(locals._t)._Locked_Amount;
                locals.INITIALIZE_HISTORY._Rewarded_Amount = locals._reward_amount;

                state.FullyUnlocker.set(state._FullyUnlocked_cnt, locals.INITIALIZE_HISTORY);

                state._FullyUnlocked_cnt++;
            }

            locals.INITIALIZE_USER.ID = NULL_ID;
            locals.INITIALIZE_USER._Locked_Amount = 0;
            locals.INITIALIZE_USER._Locked_Epoch = 0;

            state.Locker.set(locals._t, locals.INITIALIZE_USER);

            locals._burn_amount -= locals._reward_amount;
        }

        locals.st = 0;

        for(locals._t = 0; locals._t < QEARN_MAX_LOCKS; locals._t++)
        {
            if(state.Locker.get(locals._t)._Locked_Epoch) 
            {
                locals.startEpoch = state.Locker.get(locals._t)._Locked_Epoch;
                break;
            }
        }

        for(locals._t = locals.startEpoch; locals._t <= qpi.epoch(); locals._t++)
        {
            locals.empty_cnt = 0;
            if(locals._t == locals.startEpoch) 
            {
                locals.st = 0;
            }
            else 
            {
                locals.st = state.EpochIndex.get(locals._t).startIndex;
            }
            locals.en = state.EpochIndex.get(locals._t).endIndex - 1;

            while(locals.st < locals.en)
            {
                while(state.Locker.get(locals.st)._Locked_Amount)
                {
                    locals.st++;
                }

                while(!state.Locker.get(locals.en)._Locked_Amount)
                {
                    locals.en--;
                    locals.empty_cnt++;
                }

                if(locals.st > locals.en) break;

                locals.INITIALIZE_USER.ID = state.Locker.get(locals.en).ID;
                locals.INITIALIZE_USER._Locked_Amount = state.Locker.get(locals.en)._Locked_Amount;
                locals.INITIALIZE_USER._Locked_Epoch = state.Locker.get(locals.en)._Locked_Epoch;

                state.Locker.set(locals.st, locals.INITIALIZE_USER);

                locals.INITIALIZE_USER.ID = NULL_ID;
                locals.INITIALIZE_USER._Locked_Amount = 0;
                locals.INITIALIZE_USER._Locked_Epoch = 0;

                state.Locker.set(locals.en, locals.INITIALIZE_USER);

                locals.st++;
                locals.en--;
                locals.empty_cnt++;
            }

            if(locals.st == locals.en && !state.Locker.get(locals.st)._Locked_Amount)
            {
                locals.empty_cnt++;
            }

            if(locals._t == locals.startEpoch) 
            {
                locals.tmpEpochIndex.startIndex = 0;
            }
            else 
            {
                locals.tmpEpochIndex.startIndex = state.EpochIndex.get(locals._t).startIndex;
            }
            locals.tmpEpochIndex.endIndex = state.EpochIndex.get(locals._t).endIndex - locals.empty_cnt;

            state.EpochIndex.set(locals._t, locals.tmpEpochIndex);

            locals.tmpEpochIndex.startIndex = locals.tmpEpochIndex.endIndex;
            locals.tmpEpochIndex.endIndex = state.EpochIndex.get(locals._t + 1).endIndex;

            state.EpochIndex.set(locals._t + 1, locals.tmpEpochIndex);
            
        }

        locals.tmpEpochIndex.startIndex = state.EpochIndex.get(qpi.epoch() + 1).startIndex;
        locals.tmpEpochIndex.endIndex = locals.tmpEpochIndex.startIndex;

        state.EpochIndex.set(qpi.epoch() + 1, locals.tmpEpochIndex);

        qpi.burn(locals._burn_amount);
	_
};