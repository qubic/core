using namespace QPI;

struct QX2
{
};

struct QX
{
public:
	struct TransferAssetOwnershipAndPossession_input
	{
		id issuer;
		id possessor;
		id newOwner;
		unsigned long long assetName;
		long long numberOfUnits;
	};
	struct TransferAssetOwnershipAndPossession_output
	{
		long long transferredNumberOfUnits;
	};

private:
	uint64 _earnedAmount;
	uint64 _distributedAmount;
	uint64 _burnedAmount;

	uint32 _assetIssuanceFee; // Amount of qus
	uint32 _transferFee; // Amount of qus
	uint32 _tradeFee; // Number of billionths

	PUBLIC(TransferAssetOwnershipAndPossession)

		output.transferredNumberOfUnits = transferAssetOwnershipAndPossession(input.assetName, input.issuer, originator(), input.possessor, input.numberOfUnits, input.newOwner) < 0 ? 0 : input.numberOfUnits;
	_

	REGISTER_USER_FUNCTIONS
	_

	REGISTER_USER_PROCEDURES

		REGISTER_USER_PROCEDURE(TransferAssetOwnershipAndPossession, 1);
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
	_

	END_TICK
	_

	EXPAND
	_
};

