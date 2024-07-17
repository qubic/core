using namespace QPI;

#define MINLOCKING_AMOUNT 10000000						// minimum amount for locking

struct QEARN2
{
};

struct QEARN
{
private:
	uint64 _TotalLockedAmount;                          // total locked amount in a week
	uint64 _CountOfLocker;								// number of lockers in a week
	uint64_131072 _AmountOfLocked;						// amount locked of each locker
	uint64 _TotalBonus;									// total bonus < 100B
public:

	struct Lock_input 
	{
	};

	struct Lock_output 
	{
	};

	struct Unlock_input 
	{
		uint64 amount;									// amount for unlocking
		uint32 number;									// identifier of unlocker
		uint8 pct;             							// percent for early unlocking(0 ~ 55%)
	};

	struct Unlock_output 
	{
	};

	struct RewardCalc_input {
		uint32 number;									// identifier of locker
	};

	struct RewardCalc_output {
		uint64 reward;									// reward of locker
	};

	PUBLIC_PROCEDURE(Lock)
		if(qpi.invocationReward() < MINLOCKING_AMOUNT) return ;

		state._TotalLockedAmount += qpi.invocationReward();
		state._AmountOfLocked.set(state._CountOfLocker, qpi.invocationReward());
		state._CountOfLocker++;
	_

	PUBLIC_PROCEDURE(Unlock)
		if(state._AmountOfLocked.get(input.number) < input.amount) return;
		if(input.pct > 100) return;

		uint64 AmountForBurn;
		AmountForBurn =  state._TotalBonus / state._TotalLockedAmount * input.amount * input.pct / 100;
		state._TotalBonus -= AmountForBurn;

		state._AmountOfLocked.set(input.number, state._AmountOfLocked.get(input.number) - input.amount);
		state._TotalLockedAmount -= input.amount;
		qpi.transfer(qpi.invocator(), input.amount);
		qpi.burn(AmountForBurn);
	_

	PUBLIC_FUNCTION(RewardCalc)
		output.reward = state._TotalBonus / state._TotalLockedAmount * state._AmountOfLocked.get(input.number);
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_FUNCTION(RewardCalc, 1);

		REGISTER_USER_PROCEDURE(Lock, 1);
		REGISTER_USER_PROCEDURE(Unlock, 2);
	_

	INITIALIZE
		state._TotalLockedAmount = 0;
		state._CountOfLocker = 0;
		state._TotalBonus = 100000000000;							// 100B
	_

	BEGIN_EPOCH
	_

	END_EPOCH
	_

	BEGIN_TICK
	_

	END_TICK
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