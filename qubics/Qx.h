using namespace QPI;

struct QX2
{
};

struct QX
{
public:
	struct Transfer_input
	{
	};
	struct Transfer_output
	{
	};

private:
	uint64 _earnedAmount;
	uint64 _distributedAmount;
	uint64 _burnedAmount;

	uint32 _assetIssuanceFee; // Amount of qus
	uint32 _transferFee; // Amount of qus
	uint32 _tradeFee; // Number of billionths

	PUBLIC(Transfer)
	_

	REGISTER_USER_FUNCTIONS
	_

	REGISTER_USER_PROCEDURES
	_

	INITIALIZE

		// No need to initialize _earnedAmount and other variables with 0, whole contract state is zeroed before initialization is invoked

		state._assetIssuanceFee = 1000000000;
		state._transferFee = 1000000;
		state._tradeFee = 5000000; // 0.5%
	_

	BEGIN_EPOCH
	_

	END_EPOCH
	_

	BEGIN_TICK

		id curId = NULL_ID;
		do
		{
			curId = nextId(curId);
			transfer(curId, 0);
		} while (!EQUAL(curId, NULL_ID));

	_

	END_TICK
	_

	EXPAND
	_
};