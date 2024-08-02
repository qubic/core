using namespace QPI;

#define MINIMUM_LOCKING_AMOUNT 10000000
#define MAXIMUM_OF_USER 16777216
#define MAXIMUM_EPOCH 65536
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

struct QEARN
{
public:

	struct Lock_input {	
    };

    struct Lock_output {	
    };

    struct Unlock_input {
        uint64 Amount;                            /* unlocking amount */	
        uint16 Locked_Epoch;                      /* locked epoch */
    };

    struct  Unlock_output {
    };

    struct GetLockInforPerEpoch_input {
		uint16 Epoch;                             /* epoch number to get information */
    };

    struct GetLockInforPerEpoch_output {
        uint64 LockedAmount;                      /* total locked amount in epoch */
        uint64 BonusAmount;                       /* bonus amount in epoch excluding the early unlocking */
        uint64 RewardPercent;                      /* percentage that user get the reward after 52 epoch */
    };

    struct GetUserLockedInfor_input {
    };

    struct GetUserLockedInfor_output {
        uint64_64 LockedAmount;                   /* the amount user locked at every epoch */
        uint16_64 LockedEpoch;                    /* the epoch user locked */
    };


private:
	uint64_65536 _Total_Locked_Amount;            // The total locked amount in any epoch.         Max Epoch is 65536
    uint64_65536 _Epoch_Bonus_Amount;             // The bonus amount per an epoch.                Max Epoch is 65536  

    uint64_16777216 _User_Locked_Amount;

    uint64 bg_locals_pre_epoch_balance;
    uint64 bg_locals_current_balance;

    uint64 end_locals_total_reward_amount;
    uint64 end_locals_reward_percent;
    uint64 end_locals_reward_amount;

    ::Entity bg_locals_entity;

    uint32 _Locked_Last_Index;                    // The index of the last locker in User_Locked_Amount, User_Locked_Epoch, User_ID arrays.
    uint32 end_locals_count_Of_last_vacancy;

    uint16_16777216 _User_Locked_Epoch;
    uint16 end_locals_t;

    id_16777216 _User_ID;

    uint8 bg_locals_t;
    bit bg_locals_status;

    struct Lock_locals {

        uint32 first_vacancy;
        uint32 t;
        bit flag;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Lock)
    
        if(qpi.invocationReward() < MINIMUM_LOCKING_AMOUNT) return ;

        locals.flag = 0;

        for(locals.t = 0 ; locals.t < state._Locked_Last_Index; locals.t++) {

            if(state._User_ID.get(locals.t) == qpi.invocator() && state._User_Locked_Epoch.get(locals.t) == qpi.epoch()) {      // the case to be locked several times at one epoch
                
                state._User_Locked_Amount.set(locals.t, state._User_Locked_Amount.get(locals.t) + qpi.invocationReward());
                state._Total_Locked_Amount.set(qpi.epoch(), state._Total_Locked_Amount.get(qpi.epoch()) + qpi.invocationReward());
                
                return ;
            }

            if(locals.flag == 0 && state._User_Locked_Amount.get(locals.t) == 0) {
                locals.first_vacancy = locals.t;                             // the index of first vacancy(Unlocked Position of array) in array for dataset of user
                locals.flag = 1;
            }

        }

        //
        // the following "if" and "else" is the case to be locked newly at one epoch
        //

        if(locals.flag) {                 // if there is a vacancy in array
            state._User_ID.set(locals.first_vacancy, qpi.invocator());
            state._User_Locked_Epoch.set(locals.first_vacancy, qpi.epoch());
            state._User_Locked_Amount.set(locals.first_vacancy, qpi.invocationReward());
        }
        
        else {
            if(state._Locked_Last_Index == MAXIMUM_OF_USER - 1) return ;                        // overflow users in Qearn

            state._User_ID.set(state._Locked_Last_Index, qpi.invocator());
            state._User_Locked_Epoch.set(state._Locked_Last_Index, qpi.epoch());
            state._User_Locked_Amount.set(state._Locked_Last_Index, qpi.invocationReward());

            state._Locked_Last_Index++;
        }

        state._Total_Locked_Amount.set(qpi.epoch(), state._Total_Locked_Amount.get(qpi.epoch()) + qpi.invocationReward());
    _

    struct Unlock_locals {
        
        uint64 AmountOfUnlocking;
        uint64 AmountOfReward;
        uint64 AmountOfburn;
        uint32 indexOfinvocator;
        uint32 t;
        uint32 count_Of_last_vacancy;
        
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Unlock)

        if(input.Locked_Epoch > MAXIMUM_EPOCH) return ;

        locals.indexOfinvocator = MAXIMUM_OF_USER;

        for(locals.t = 0 ; locals.t < state._Locked_Last_Index; locals.t++) {

            if(state._User_ID.get(locals.t) == qpi.invocator() && state._User_Locked_Epoch.get(locals.t) == input.Locked_Epoch) { 
                if(state._User_Locked_Amount.get(locals.t) < input.Amount) return ;   // the amount to be unlocked has been exceeded
                else {
                    locals.indexOfinvocator = locals.t;
                    break;
                }
            }

        }

        if(locals.indexOfinvocator == state._Locked_Last_Index) return ;   // the case that invocator didn't lock at input.epoch

        /* the rest amount after unlocking should be more than MINIMUM_LOCKING_AMOUNT */
        if(state._User_Locked_Amount.get(locals.indexOfinvocator) - input.Amount < MINIMUM_LOCKING_AMOUNT) locals.AmountOfUnlocking = state._User_Locked_Amount.get(locals.indexOfinvocator);
        else locals.AmountOfUnlocking = input.Amount;

        if(qpi.epoch() - input.Locked_Epoch >= 0 && qpi.epoch() - input.Locked_Epoch <= 3) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_0_3 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_0_3 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 4 && qpi.epoch() - input.Locked_Epoch <= 7) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_4_7 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_4_7 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 8 && qpi.epoch() - input.Locked_Epoch <= 11) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_8_11 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_8_11 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 12 && qpi.epoch() - input.Locked_Epoch <= 15) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_12_15 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_12_15 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >=16 && qpi.epoch() - input.Locked_Epoch <= 19) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_16_19 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_16_19 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 20 && qpi.epoch() - input.Locked_Epoch <= 23) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_20_23 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_20_23 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 24 && qpi.epoch() - input.Locked_Epoch <= 27) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_24_27 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_24_27 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 28 && qpi.epoch() - input.Locked_Epoch <= 31) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_28_31 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_28_31 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 32 && qpi.epoch() - input.Locked_Epoch <= 35) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_32_35 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_32_35 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 36 && qpi.epoch() - input.Locked_Epoch <= 39) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_36_39 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_36_39 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 40 && qpi.epoch() - input.Locked_Epoch <= 43) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_40_43 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_40_43 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 44 && qpi.epoch() - input.Locked_Epoch <= 47) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_44_47 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_44_47 / 100 / 100;
        }

        if(qpi.epoch() - input.Locked_Epoch >= 48 && qpi.epoch() - input.Locked_Epoch <= 51) {
            locals.AmountOfReward = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * EARLY_UNLOCKING_PERCENT_48_51 / 100 / 100;
            locals.AmountOfburn = state._Epoch_Bonus_Amount.get(input.Locked_Epoch) * 100 / state._Total_Locked_Amount.get(input.Locked_Epoch) * locals.AmountOfUnlocking * BURN_PERCENT_48_51 / 100 / 100;
        }

        qpi.transfer(qpi.invocator(), locals.AmountOfUnlocking + locals.AmountOfReward);
        qpi.burn(locals.AmountOfburn);

        state._Total_Locked_Amount.set(input.Locked_Epoch, state._Total_Locked_Amount.get(input.Locked_Epoch) - locals.AmountOfUnlocking);
        state._Epoch_Bonus_Amount.set(input.Locked_Epoch, state._Epoch_Bonus_Amount.get(input.Locked_Epoch) - locals.AmountOfReward - locals.AmountOfburn);
        state._User_Locked_Amount.set(locals.indexOfinvocator, state._User_Locked_Amount.get(locals.indexOfinvocator) - locals.AmountOfUnlocking);

        if(locals.indexOfinvocator == state._Locked_Last_Index - 1 && state._User_Locked_Amount.get(locals.indexOfinvocator) == 0) { // the case to be unlocked last index in array for dataset of user
            locals.count_Of_last_vacancy = 0;          
            // if array [10, 10, 0, 0, 10, 0, 0, 0, 0, 0] and state._Locked_Last_Index = 8 and indexOfinvocator = 7, count_Of_last_vacancy should be 3.
            for(locals.t = state._Locked_Last_Index - 1; locals.t >= 0; locals.t--) {
                if(state._User_Locked_Amount.get(locals.t) == 0) locals.count_Of_last_vacancy++;
                else break;
            }

            state._Locked_Last_Index -= locals.count_Of_last_vacancy;
        }

    _

    PUBLIC_FUNCTION(GetLockInforPerEpoch)

        output.BonusAmount = state._Epoch_Bonus_Amount.get(input.Epoch);
        output.LockedAmount = state._Total_Locked_Amount.get(input.Epoch);
        output.RewardPercent = state._Epoch_Bonus_Amount.get(input.Epoch) * 100 / state._Total_Locked_Amount.get(input.Epoch);

    _

    struct GetUserLockedInfor_locals {
        uint32 _t, _k;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserLockedInfor)

        for(locals._t = 0, locals._k = 0 ; locals._t < state._Locked_Last_Index; locals._t++) if(state._User_ID.get(locals._t) == qpi.invocator()) {
            output.LockedEpoch.set(locals._k, state._User_Locked_Epoch.get(locals._t));
            output.LockedAmount.set(locals._k++, state._User_Locked_Amount.get(locals._t));
        }

    _

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
    
        REGISTER_USER_FUNCTION(GetLockInforPerEpoch, 1);
        REGISTER_USER_FUNCTION(GetUserLockedInfor, 2);

        REGISTER_USER_PROCEDURE(Lock, 1);
		REGISTER_USER_PROCEDURE(Unlock, 2);

	_

	INITIALIZE

        state._Locked_Last_Index = 0;

        for(uint16 t = 0 ; t < MAXIMUM_EPOCH; t++) {
            state._Total_Locked_Amount.set(t, 0);
            state._Epoch_Bonus_Amount.set(t, 0);
        }
        
	_

    BEGIN_EPOCH

        state.bg_locals_status = qpi.getEntity(SELF, state.bg_locals_entity);
        state.bg_locals_current_balance = state.bg_locals_entity.incomingAmount - state.bg_locals_entity.outgoingAmount;

        state.bg_locals_pre_epoch_balance = 0;
        for(state.bg_locals_t = qpi.epoch() - 1; state.bg_locals_t >= qpi.epoch() - 52; state.bg_locals_t--) state.bg_locals_pre_epoch_balance += state._Epoch_Bonus_Amount.get(state.bg_locals_t) + state._Total_Locked_Amount.get(state.bg_locals_t);

        state._Epoch_Bonus_Amount.set(qpi.epoch(), state.bg_locals_current_balance - state.bg_locals_pre_epoch_balance);

	_

	END_EPOCH

        state.end_locals_total_reward_amount = 0;
        state.end_locals_reward_percent = state._Epoch_Bonus_Amount.get(qpi.epoch() - 52) * 100 / state._Total_Locked_Amount.get(qpi.epoch() - 52);

        for(state.end_locals_t = 0 ; state.end_locals_t < state._Locked_Last_Index; state.end_locals_t++) if(state._User_Locked_Epoch.get(state.end_locals_t) == qpi.epoch() - 52) {

            state.end_locals_reward_amount = state._User_Locked_Amount.get(qpi.epoch() - 52) * state.end_locals_reward_percent / 100;
            qpi.transfer(state._User_ID.get(state.end_locals_t), state.end_locals_reward_amount + state._User_Locked_Amount.get(state.end_locals_t));
            state.end_locals_total_reward_amount += state.end_locals_reward_amount;

            state._User_Locked_Amount.set(state.end_locals_t, 0);
        }
        
        // if reward_percent = 12.3%, 0.3% would be burned. so 0 ~ 0.999999.. % would be burned.

        qpi.burn(state._Epoch_Bonus_Amount.get(qpi.epoch() - 52) - state.end_locals_total_reward_amount);

        if(state._User_Locked_Amount.get(state._Locked_Last_Index - 1) == 0) { // the case to be unlocked last index in array for dataset of user
            state.end_locals_count_Of_last_vacancy = 0;          
            // if array [10, 10, 0, 0, 10, 0, 0, 0, 0, 0] and state._Locked_Last_Index = 8, count_Of_last_vacancy should be 3.
            for(state.end_locals_t = state._Locked_Last_Index - 1; state.end_locals_t >= 0; state.end_locals_t--) {
                if(state._User_Locked_Amount.get(state.end_locals_t) == 0) state.end_locals_count_Of_last_vacancy++;
                else break;
            }

            state._Locked_Last_Index -= state.end_locals_count_Of_last_vacancy;
        }
	_

	EMPTY_BEGIN_TICK
	_

	EMPTY_END_TICK
	_

	PRE_ACQUIRE_SHARES
	_

	POST_ACQUIRE_SHARES
	_

	PRE_RELEASE_SHARES
	_

	POST_RELEASE_SHARES
	_

	EXPAND
	_
};