using namespace QPI;

#define MINIMUM_LOCKING_AMOUNT 10000000
#define MAXIMUM_OF_USER 16777216
#define MAXIMUM_EPOCH 65535
#define MAXIMUM_UNLOCK_HISTORY 1048576

#define EARLY_UNLOCKING_PERCENT_0_3 0
#define EARLY_UNLOCKING_PERCENT_4_7 5
#define EARLY_UNLOCKING_PERCENT_8_11 5
#define EARLY_UNLOCKING_PERCENT_12_15 10
#define EARLY_UNLOCKING_PERCENT_16_19 15
#define EARLY_UNLOCKING_PERCENT_20_23 20
#define EARLY_UNLOCKING_PERCENT_24_27 25
#define EARLY_UNLOCKING_PERCENT_28_31 30
#define EARLY_UNLOCKING_PERCENT_32_35 35
#define EARLY_UNLOCKING_PERCENT_36_39 40
#define EARLY_UNLOCKING_PERCENT_40_43 45
#define EARLY_UNLOCKING_PERCENT_44_47 50
#define EARLY_UNLOCKING_PERCENT_48_51 55

#define BURN_PERCENT_0_3 0
#define BURN_PERCENT_4_7 45
#define BURN_PERCENT_8_11 45
#define BURN_PERCENT_12_15 45
#define BURN_PERCENT_16_19 40
#define BURN_PERCENT_20_23 40
#define BURN_PERCENT_24_27 35
#define BURN_PERCENT_28_31 35
#define BURN_PERCENT_32_35 35
#define BURN_PERCENT_36_39 30
#define BURN_PERCENT_40_43 30
#define BURN_PERCENT_44_47 30
#define BURN_PERCENT_48_51 25

struct QEARN2
{
};

struct QEARN : public ContractBase
{
public:
    struct GetLockInforPerEpoch_input {
		uint32 Epoch;                             /* epoch number to get information */
    };

    struct GetLockInforPerEpoch_output {
        uint64 LockedAmount;                      /* initial total locked amount in epoch */
        uint64 BonusAmount;                       /* initial bonus amount in epoch*/
        uint64 CurrentLockedAmount;               /* total locked amount in epoch. exactly the amount excluding the amount unlocked early*/
        uint64 CurrentBonusAmount;                /* bonus amount in epoch excluding the early unlocking */
        uint64 Yield;                             /* Yield calculated by 10000000 multiple*/
    };

    struct GetUserLockedInfor_input {
        id user;
        uint32 epoch;
    };

    struct GetUserLockedInfor_output {
        uint64 LockedAmount;                   /* the amount user locked at input.epoch */
    };

    struct GetStateOfRound_input {
        uint32 Epoch;
    };

    struct GetStateOfRound_output {
        uint32 state;
    };
    
    struct GetUserLockStatus_input {
        id user;
    };

    struct GetUserLockStatus_output {
        uint64 status;
    };

    struct GetEndedStatus_input {
        id user;
    };

    struct GetEndedStatus_output {
        uint64 Fully_Unlocked_Amount;
        uint64 Fully_Rewarded_Amount;
        uint64 Early_Unlocked_Amount;
        uint64 Early_Rewarded_Amount;
    };

	struct Lock_input {	
    };

    struct Lock_output {	
    };

    struct Unlock_input {
        uint64 Amount;                            /* unlocking amount */	
        uint32 Locked_Epoch;                      /* locked epoch */
    };

    struct Unlock_output {
    };

private:

    struct RoundInfo {

        uint64 _Total_Locked_Amount;            // The initial total locked amount in any epoch.  Max Epoch is 65535
        uint64 _Epoch_Bonus_Amount;             // The initial bonus amount per an epoch.         Max Epoch is 65535 

    };

    array<RoundInfo, 65536> _InitialRoundInfo;
    array<RoundInfo, 65536> _CurrentRoundInfo;

    struct UserInfo {

        uint64 _Locked_Amount;
        uint16 _Locked_Epoch;
        id ID;

    };

    array<UserInfo, 16777216> Locker;

    struct HistoryInfo {

        uint64 _Unlocked_Amount;
        uint64 _Rewarded_Amount;
        id _Unlocked_ID;
        bit _Unlocked_Status;

    };

    array<HistoryInfo, 1048576> Unlocker;
    
    uint64 remain_amount;                                  //  The remain amount boosted by last early unlocker of one round, this amount will put in next round

    uint32 _Locked_Last_Index;                    // The index of the last locker in User_Locked_Amount, User_Locked_Epoch, User_ID arrays.
    uint32 _Unlocked_cnt;

    PUBLIC_FUNCTION(GetStateOfRound)
        if(input.Epoch <= 122) {                                                            // non staking
            output.state = 2; return ;
        }
        if(input.Epoch > qpi.epoch()) output.state = 0;                                     // opening round, not started yet
        if(input.Epoch <= qpi.epoch() && input.Epoch >= qpi.epoch() - 52) output.state = 1; // running round, available unlocking early
        if(input.Epoch < qpi.epoch() - 52) output.state = 2;                                // ended round
    _

    PUBLIC_FUNCTION(GetLockInforPerEpoch)

        output.BonusAmount = state._InitialRoundInfo.get(input.Epoch)._Epoch_Bonus_Amount;
        output.LockedAmount = state._InitialRoundInfo.get(input.Epoch)._Total_Locked_Amount;
        output.CurrentBonusAmount = state._CurrentRoundInfo.get(input.Epoch)._Epoch_Bonus_Amount;
        output.CurrentLockedAmount = state._CurrentRoundInfo.get(input.Epoch)._Total_Locked_Amount;
        if(state._CurrentRoundInfo.get(input.Epoch)._Total_Locked_Amount) output.Yield = state._CurrentRoundInfo.get(input.Epoch)._Epoch_Bonus_Amount * 10000000UL / state._CurrentRoundInfo.get(input.Epoch)._Total_Locked_Amount;
        else output.Yield = 0UL;
    _

    struct GetUserLockedInfor_locals {
        uint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserLockedInfor)
        
        for(locals._t = 0; locals._t < state._Locked_Last_Index; locals._t++) if(state.Locker.get(locals._t).ID == input.user && state.Locker.get(locals._t)._Locked_Epoch == input.epoch) {
            output.LockedAmount = state.Locker.get(locals._t)._Locked_Amount; break;
        }
    _

    struct GetUserLockStatus_locals {
        uint64 bn;
        uint32 _t;
        uint32 _r;
        uint8 lockedWeeks;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserLockStatus)

        output.status = 0UL;
        
        for(locals._t = 0; locals._t < state._Locked_Last_Index; locals._t++) if(state.Locker.get(locals._t).ID == input.user && state.Locker.get(locals._t)._Locked_Amount > 0) {
            
            locals.lockedWeeks = qpi.epoch() - state.Locker.get(locals._t)._Locked_Epoch;
            locals.bn = 1;

            for(locals._r = 0; locals._r < locals.lockedWeeks; locals._r++) locals.bn *= 2;

            output.status += locals.bn;
        }

    _
    
    struct GetEndedStatus_locals {
        uint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetEndedStatus)

        output.Early_Rewarded_Amount = 0;
        output.Early_Unlocked_Amount = 0;
        output.Fully_Rewarded_Amount = 0;
        output.Fully_Unlocked_Amount = 0;

        for(locals._t = 0; locals._t < state._Unlocked_cnt; locals._t++) if(state.Unlocker.get(locals._t)._Unlocked_ID == input.user) {

            if(state.Unlocker.get(locals._t)._Unlocked_Status == 1) {
                output.Fully_Rewarded_Amount = state.Unlocker.get(locals._t)._Rewarded_Amount;
                output.Fully_Unlocked_Amount = state.Unlocker.get(locals._t)._Unlocked_Amount;
            }

            if(state.Unlocker.get(locals._t)._Unlocked_Status == 0) {
                output.Early_Rewarded_Amount = state.Unlocker.get(locals._t)._Rewarded_Amount;
                output.Early_Unlocked_Amount = state.Unlocker.get(locals._t)._Unlocked_Amount;

                return ;
            }
        }

    _

    struct Lock_locals {

        UserInfo newLocker;
        RoundInfo updatedRoundInfo;

        uint32 first_vacancy;
        sint32 t;
        bit flag;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Lock)
    
        if(qpi.invocationReward() < MINIMUM_LOCKING_AMOUNT) return ;

        locals.flag = 0;

        for(locals.t = 0 ; locals.t < state._Locked_Last_Index; locals.t++) {

            if(state.Locker.get(locals.t).ID == qpi.invocator() && state.Locker.get(locals.t)._Locked_Epoch == qpi.epoch()) {      // the case to be locked several times at one epoch
                
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
                
                return ;
            }

            if(locals.flag == 0 && state.Locker.get(locals.t)._Locked_Amount == 0) {
                locals.first_vacancy = locals.t;                             // the index of first vacancy(Unlocked Position of array) in array for dataset of user
                locals.flag = 1;
            }

        }

        //
        // the following "if" and "else" is the case to be locked newly at one epoch
        //

        locals.newLocker.ID = qpi.invocator();
        locals.newLocker._Locked_Amount = qpi.invocationReward();
        locals.newLocker._Locked_Epoch = qpi.epoch();

        if(locals.flag) {                 // if there is a vacancy in array

            state.Locker.set(locals.first_vacancy, locals.newLocker);

        }
        
        else {
            if(state._Locked_Last_Index == MAXIMUM_OF_USER - 1) return ;                        // overflow users in Qearn

            state.Locker.set(state._Locked_Last_Index, locals.newLocker);

            state._Locked_Last_Index++;
        }

        locals.updatedRoundInfo._Total_Locked_Amount = state._InitialRoundInfo.get(qpi.epoch())._Total_Locked_Amount + qpi.invocationReward();
        locals.updatedRoundInfo._Epoch_Bonus_Amount = state._InitialRoundInfo.get(qpi.epoch())._Epoch_Bonus_Amount;
        state._InitialRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);

        locals.updatedRoundInfo._Total_Locked_Amount = state._CurrentRoundInfo.get(qpi.epoch())._Total_Locked_Amount + qpi.invocationReward();
        locals.updatedRoundInfo._Epoch_Bonus_Amount = state._CurrentRoundInfo.get(qpi.epoch())._Epoch_Bonus_Amount;
        state._CurrentRoundInfo.set(qpi.epoch(), locals.updatedRoundInfo);
    _

    struct Unlock_locals {

        RoundInfo updatedRoundInfo;
        UserInfo updatedUserInfo;
        HistoryInfo unlockerInfo;
        
        uint64 AmountOfUnlocking;
        uint64 AmountOfReward;
        uint64 AmountOfburn;
        uint64 RewardPercent;
        uint32 indexOfinvocator;
        sint32 t;
        uint32 count_Of_last_vacancy;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Unlock)

        if(input.Locked_Epoch > MAXIMUM_EPOCH) return ;

        locals.indexOfinvocator = MAXIMUM_OF_USER;

        for(locals.t = 0 ; locals.t < state._Locked_Last_Index; locals.t++) {

            if(state.Locker.get(locals.t).ID == qpi.invocator() && state.Locker.get(locals.t)._Locked_Epoch == input.Locked_Epoch) { 
                if(state.Locker.get(locals.t)._Locked_Amount < input.Amount) return ;   // the amount to be unlocked has been exceeded
                else {
                    locals.indexOfinvocator = locals.t;
                    break;
                }
            }

        }

        if(locals.indexOfinvocator == MAXIMUM_OF_USER) return ;   // the case that invocator didn't lock at input.epoch

        /* the rest amount after unlocking should be more than MINIMUM_LOCKING_AMOUNT */
        if(state.Locker.get(locals.indexOfinvocator)._Locked_Amount - input.Amount < MINIMUM_LOCKING_AMOUNT) locals.AmountOfUnlocking = state.Locker.get(locals.indexOfinvocator)._Locked_Amount;
        else locals.AmountOfUnlocking = input.Amount;

        locals.RewardPercent = state._CurrentRoundInfo.get(input.Locked_Epoch)._Epoch_Bonus_Amount * 10000000UL / state._CurrentRoundInfo.get(input.Locked_Epoch)._Total_Locked_Amount;

        if(qpi.epoch() - input.Locked_Epoch >= 0 && qpi.epoch() - input.Locked_Epoch <= 4) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_0_3 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_0_3 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 5 && qpi.epoch() - input.Locked_Epoch <= 8) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_4_7 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_4_7 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 9 && qpi.epoch() - input.Locked_Epoch <= 12) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_8_11 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_8_11 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 13 && qpi.epoch() - input.Locked_Epoch <= 16) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_12_15 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_12_15 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 17 && qpi.epoch() - input.Locked_Epoch <= 20) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_16_19 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_16_19 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 21 && qpi.epoch() - input.Locked_Epoch <= 24) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_20_23 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_20_23 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 25 && qpi.epoch() - input.Locked_Epoch <= 28) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_24_27 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_24_27 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 29 && qpi.epoch() - input.Locked_Epoch <= 32) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_28_31 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_28_31 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 33 && qpi.epoch() - input.Locked_Epoch <= 36) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_32_35 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_32_35 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 37 && qpi.epoch() - input.Locked_Epoch <= 40) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_36_39 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_36_39 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 41 && qpi.epoch() - input.Locked_Epoch <= 44) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_40_43 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_40_43 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 45 && qpi.epoch() - input.Locked_Epoch <= 48) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_44_47 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_44_47 / 10000000UL;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 49 && qpi.epoch() - input.Locked_Epoch <= 52) {
            locals.AmountOfReward = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * EARLY_UNLOCKING_PERCENT_48_51 / 10000000UL;
            locals.AmountOfburn = locals.RewardPercent * locals.AmountOfUnlocking / 100UL * BURN_PERCENT_48_51 / 10000000UL;
        }

        qpi.transfer(qpi.invocator(), locals.AmountOfUnlocking + locals.AmountOfReward);
        qpi.burn(locals.AmountOfburn);

        locals.updatedRoundInfo._Total_Locked_Amount = state._CurrentRoundInfo.get(input.Locked_Epoch)._Total_Locked_Amount - locals.AmountOfUnlocking;
        locals.updatedRoundInfo._Epoch_Bonus_Amount = state._CurrentRoundInfo.get(input.Locked_Epoch)._Epoch_Bonus_Amount - locals.AmountOfReward - locals.AmountOfburn;

        state._CurrentRoundInfo.set(input.Locked_Epoch, locals.updatedRoundInfo);

        locals.updatedUserInfo.ID = qpi.invocator();
        locals.updatedUserInfo._Locked_Amount = state.Locker.get(locals.indexOfinvocator)._Locked_Amount - locals.AmountOfUnlocking;
        locals.updatedUserInfo._Locked_Epoch = state.Locker.get(locals.indexOfinvocator)._Locked_Epoch;

        state.Locker.set(locals.indexOfinvocator, locals.updatedUserInfo);

        if(state._CurrentRoundInfo.get(input.Locked_Epoch)._Total_Locked_Amount == 0 && input.Locked_Epoch != qpi.epoch()) {            // The case to be unlocked early all users of one epoch, at this time, boost amount will put in next round.
            
            state.remain_amount = state._CurrentRoundInfo.get(input.Locked_Epoch)._Epoch_Bonus_Amount;

            locals.updatedRoundInfo._Total_Locked_Amount = 0;
            locals.updatedRoundInfo._Epoch_Bonus_Amount = 0;

            state._CurrentRoundInfo.set(input.Locked_Epoch, locals.updatedRoundInfo);

        }

        if(input.Locked_Epoch != qpi.epoch()) {

            locals.unlockerInfo._Unlocked_ID = qpi.invocator();
            locals.unlockerInfo._Unlocked_Status = 0;
            
            for(locals.t = 0; locals.t < state._Unlocked_cnt; locals.t++) if(state.Unlocker.get(locals.t)._Unlocked_ID == qpi.invocator() && state.Unlocker.get(locals.t)._Unlocked_Status == 0) {

                locals.unlockerInfo._Rewarded_Amount = state.Unlocker.get(locals.t)._Rewarded_Amount + locals.AmountOfReward;
                locals.unlockerInfo._Unlocked_Amount = state.Unlocker.get(locals.t)._Unlocked_Amount + locals.AmountOfUnlocking;

                state.Unlocker.set(locals.t, locals.unlockerInfo);

                break;
            }

            if(locals.t == state._Unlocked_cnt && state._Unlocked_cnt < MAXIMUM_UNLOCK_HISTORY) {

                locals.unlockerInfo._Rewarded_Amount = locals.AmountOfReward;
                locals.unlockerInfo._Unlocked_Amount = locals.AmountOfUnlocking;

                state.Unlocker.set(locals.t, locals.unlockerInfo);
                state._Unlocked_cnt++;

            }

        }

        if(locals.indexOfinvocator == state._Locked_Last_Index - 1 && state.Locker.get(locals.indexOfinvocator)._Locked_Amount == 0) { // the case to be unlocked last index in array for dataset of user
            locals.count_Of_last_vacancy = 0;          
            // if array [10, 10, 0, 0, 10, 0, 0, 0, 0, 0] and state._Locked_Last_Index = 8 and indexOfinvocator = 7, count_Of_last_vacancy should be 3.
            for(locals.t = state._Locked_Last_Index - 1; locals.t >= 0; locals.t--) {
                if(state.Locker.get(locals.t)._Locked_Amount == 0) locals.count_Of_last_vacancy++;
                else break;
            }

            state._Locked_Last_Index -= locals.count_Of_last_vacancy;
        }

    _

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
    
        REGISTER_USER_FUNCTION(GetLockInforPerEpoch, 1);
        REGISTER_USER_FUNCTION(GetUserLockedInfor, 2);
        REGISTER_USER_FUNCTION(GetStateOfRound, 3);
        REGISTER_USER_FUNCTION(GetUserLockStatus, 4);
        REGISTER_USER_FUNCTION(GetEndedStatus, 5);

        REGISTER_USER_PROCEDURE(Lock, 1);
		REGISTER_USER_PROCEDURE(Unlock, 2);

	_

    struct INITIALIZE_locals {

        sint64 _t;
        HistoryInfo INITIALIZE_HISTORY;
        UserInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;

    };

	INITIALIZE_WITH_LOCALS

        state._Locked_Last_Index = 0;
        state._Unlocked_cnt = 0;
        state.remain_amount = 0;

        for(locals._t = 0 ; locals._t < MAXIMUM_EPOCH; locals._t++) {

            locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount = 0;
            locals.INITIALIZE_ROUNDINFO._Total_Locked_Amount = 0;

            state._InitialRoundInfo.set(locals._t, locals.INITIALIZE_ROUNDINFO);
            state._CurrentRoundInfo.set(locals._t, locals.INITIALIZE_ROUNDINFO);

        }
	_

    struct BEGIN_EPOCH_locals
    {
        HistoryInfo INITIALIZE_HISTORY;
        UserInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;

        uint32 t;
        bit status;
        uint64 pre_epoch_balance;
        uint64 current_balance;
        ::Entity entity;
    };

    BEGIN_EPOCH_WITH_LOCALS

        qpi.getEntity(SELF, locals.entity);
        locals.current_balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        locals.pre_epoch_balance = 0UL;
        for(locals.t = qpi.epoch() - 1; locals.t >= qpi.epoch() - 52; locals.t--) locals.pre_epoch_balance += state._CurrentRoundInfo.get(locals.t)._Epoch_Bonus_Amount + state._CurrentRoundInfo.get(locals.t)._Total_Locked_Amount;

        locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount = locals.current_balance - locals.pre_epoch_balance;
        locals.INITIALIZE_ROUNDINFO._Total_Locked_Amount = 0;

        state._InitialRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
        state._CurrentRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);

        if(locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount > 0) {
            
            locals.INITIALIZE_ROUNDINFO._Epoch_Bonus_Amount += state.remain_amount;

            state._InitialRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);
            state._CurrentRoundInfo.set(qpi.epoch(), locals.INITIALIZE_ROUNDINFO);

            state.remain_amount = 0;

        }
	_

    struct END_EPOCH_locals 
    {
        HistoryInfo INITIALIZE_HISTORY;
        UserInfo INITIALIZE_USER;
        RoundInfo INITIALIZE_ROUNDINFO;

        uint64 _reward_percent;
        uint64 _reward_amount;
        uint64 _burn_amount;
        uint32 _count_Of_last_vacancy;
        uint32 _t;

    };

	END_EPOCH_WITH_LOCALS

        locals._count_Of_last_vacancy = 0;
        state._Unlocked_cnt = 0;
        
        locals._burn_amount = state._CurrentRoundInfo.get(qpi.epoch() - 52)._Epoch_Bonus_Amount;
        if(state._CurrentRoundInfo.get(qpi.epoch() - 52)._Total_Locked_Amount > 0) locals._reward_percent = state._CurrentRoundInfo.get(qpi.epoch() - 52)._Epoch_Bonus_Amount * 10000000UL / state._CurrentRoundInfo.get(qpi.epoch() - 52)._Total_Locked_Amount;

        for(locals._t = 0 ; locals._t < state._Locked_Last_Index; locals._t++) {
            if(state.Locker.get(locals._t)._Locked_Epoch == qpi.epoch() - 52) {

                if(state.Locker.get(locals._t)._Locked_Amount == 0) {                            //  the case to be unlocked early

                    locals._count_Of_last_vacancy++; continue;
                    
                }

                locals._reward_amount = state.Locker.get(locals._t)._Locked_Amount * locals._reward_percent / 10000000UL;
                qpi.transfer(state.Locker.get(locals._t).ID, locals._reward_amount + state.Locker.get(locals._t)._Locked_Amount);

                if(state._Unlocked_cnt < MAXIMUM_UNLOCK_HISTORY) {

                    locals.INITIALIZE_HISTORY._Unlocked_ID = state.Locker.get(locals._t).ID;
                    locals.INITIALIZE_HISTORY._Unlocked_Status = 1;
                    locals.INITIALIZE_HISTORY._Unlocked_Amount = state.Locker.get(locals._t)._Locked_Amount;
                    locals.INITIALIZE_HISTORY._Rewarded_Amount = locals._reward_amount;

                    state.Unlocker.set(state._Unlocked_cnt, locals.INITIALIZE_HISTORY);

                    state._Unlocked_cnt++;
                }

                locals.INITIALIZE_USER.ID = _mm256_setzero_si256();
                locals.INITIALIZE_USER._Locked_Amount = 0;
                locals.INITIALIZE_USER._Locked_Epoch = 0;

                state.Locker.set(locals._t, locals.INITIALIZE_USER);

                locals._burn_amount -= locals._reward_amount;

                locals._count_Of_last_vacancy++;
            }
            else locals._count_Of_last_vacancy = 0;
        }

        state._Locked_Last_Index -= locals._count_Of_last_vacancy;
        qpi.burn(locals._burn_amount);
	_
};