using namespace QPI;

struct QX2
{
};

struct QX
{
public:
	struct IssueAsset_input
	{
		uint64 name;
		sint64 numberOfUnits;
		uint64 unitOfMeasurement;
		sint8 numberOfDecimalPlaces;
	};
	struct IssueAsset_output
	{
		long long issuedNumberOfUnits;
	};

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

	PUBLIC(IssueAsset)

		if (invocationReward() < state._assetIssuanceFee)
		{
			if (invocationReward() > 0)
			{
				transfer(invocator(), invocationReward());
			}

			output.issuedNumberOfUnits = 0;
		}
		else
		{
			if (invocationReward() > state._assetIssuanceFee)
			{
				transfer(invocator(), invocationReward() - state._assetIssuanceFee);
			}
			state._earnedAmount += state._assetIssuanceFee;

			output.issuedNumberOfUnits = issueAsset(input.name, invocator(), input.numberOfDecimalPlaces, input.numberOfUnits, input.unitOfMeasurement);
		}
	_

	PUBLIC(TransferAssetOwnershipAndPossession)

		if (invocationReward() < state._transferFee)
		{
			if (invocationReward() > 0)
			{
				transfer(invocator(), invocationReward());
			}

			output.transferredNumberOfUnits = 0;
		}
		else
		{
			if (invocationReward() > state._transferFee)
			{
				transfer(invocator(), invocationReward() - state._transferFee);
			}
			state._earnedAmount += state._transferFee;

			output.transferredNumberOfUnits = transferAssetOwnershipAndPossession(input.assetName, input.issuer, invocator(), input.possessor, input.numberOfUnits, input.newOwner) < 0 ? 0 : input.numberOfUnits;
		}
	_

	REGISTER_USER_FUNCTIONS
	_

	REGISTER_USER_PROCEDURES

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferAssetOwnershipAndPossession, 2);
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

