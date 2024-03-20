using namespace QPI;

struct QX2
{
};

struct QX
{
public:
	struct Fees_input
	{
	};
	struct Fees_output
	{
		uint32 assetIssuanceFee; // Amount of qus
		uint32 transferFee; // Amount of qus
		uint32 tradeFee; // Number of billionths
	};

	struct AssetAskOrders_input
	{
		id issuer;
		uint64 assetName;
		uint64 offset;
	};
	struct AssetAskOrders_output
	{
		struct Order
		{
			id entity;
			sint64 price;
			sint64 numberOfShares;
		};

		array<Order, 256> orders;
	};

	struct AssetBidOrders_input
	{
		id issuer;
		uint64 assetName;
		uint64 offset;
	};
	struct AssetBidOrders_output
	{
		struct Order
		{
			id entity;
			sint64 price;
			sint64 numberOfShares;
		};

		array<Order, 256> orders;
	};

	struct EntityAskOrders_input
	{
		id entity;
		uint64 offset;
	};
	struct EntityAskOrders_output
	{
		struct Order
		{
			id issuer;
			uint64 assetName;
			sint64 price;
			sint64 numberOfShares;
		};

		array<Order, 256> orders;
	};

	struct EntityBidOrders_input
	{
		id entity;
		uint64 offset;
	};
	struct EntityBidOrders_output
	{
		struct Order
		{
			id issuer;
			uint64 assetName;
			sint64 price;
			sint64 numberOfShares;
		};

		array<Order, 256> orders;
	};

	struct IssueAsset_input
	{
		uint64 assetName;
		sint64 numberOfShares;
		uint64 unitOfMeasurement;
		sint8 numberOfDecimalPlaces;
	};
	struct IssueAsset_output
	{
		sint64 issuedNumberOfShares;
	};

	struct TransferShareOwnershipAndPossession_input
	{
		id issuer;
		id newOwnerAndPossessor;
		uint64 assetName;
		sint64 numberOfShares;
	};
	struct TransferShareOwnershipAndPossession_output
	{
		sint64 transferredNumberOfShares;
	};

	struct AddToAskOrder_input
	{
		id issuer;
		uint64 assetName;
		sint64 price;
		sint64 numberOfShares;
	};
	struct AddToAskOrder_output
	{
		sint64 addedNumberOfShares;
	};

	struct AddToBidOrder_input
	{
		id issuer;
		uint64 assetName;
		sint64 price;
		sint64 numberOfShares;
	};
	struct AddToBidOrder_output
	{
		sint64 addedNumberOfShares;
	};

	struct RemoveFromAskOrder_input
	{
		id issuer;
		uint64 assetName;
		sint64 price;
		sint64 numberOfShares;
	};
	struct RemoveFromAskOrder_output
	{
		sint64 removedNumberOfShares;
	};

	struct RemoveFromBidOrder_input
	{
		id issuer;
		uint64 assetName;
		sint64 price;
		sint64 numberOfShares;
	};
	struct RemoveFromBidOrder_output
	{
		sint64 removedNumberOfShares;
	};

private:
	uint64 _earnedAmount;
	uint64 _distributedAmount;
	uint64 _burnedAmount;

	uint32 _assetIssuanceFee; // Amount of qus
	uint32 _transferFee; // Amount of qus
	uint32 _tradeFee; // Number of billionths

	PUBLIC(Fees)

		output.assetIssuanceFee = state._assetIssuanceFee;
		output.transferFee = state._transferFee;
		output.tradeFee = state._tradeFee;
	_

	PUBLIC(AssetAskOrders)
	_

	PUBLIC(AssetBidOrders)
	_

	PUBLIC(EntityAskOrders)
	_

	PUBLIC(EntityBidOrders)
	_

	PUBLIC(IssueAsset)

		if (invocationReward() < state._assetIssuanceFee)
		{
			if (invocationReward() > 0)
			{
				transfer(invocator(), invocationReward());
			}

			output.issuedNumberOfShares = 0;
		}
		else
		{
			if (invocationReward() > state._assetIssuanceFee)
			{
				transfer(invocator(), invocationReward() - state._assetIssuanceFee);
			}
			state._earnedAmount += state._assetIssuanceFee;

			output.issuedNumberOfShares = issueAsset(input.assetName, invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
		}
	_

	PUBLIC(TransferShareOwnershipAndPossession)

		if (invocationReward() < state._transferFee)
		{
			if (invocationReward() > 0)
			{
				transfer(invocator(), invocationReward());
			}

			output.transferredNumberOfShares = 0;
		}
		else
		{
			if (invocationReward() > state._transferFee)
			{
				transfer(invocator(), invocationReward() - state._transferFee);
			}
			state._earnedAmount += state._transferFee;

			output.transferredNumberOfShares = transferShareOwnershipAndPossession(input.assetName, input.issuer, invocator(), invocator(), input.numberOfShares, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfShares;
		}
	_

	PUBLIC(AddToAskOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		output.addedNumberOfShares = 0;
	_

	PUBLIC(AddToBidOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		output.addedNumberOfShares = 0;
	_

	PUBLIC(RemoveFromAskOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		output.removedNumberOfShares = 0;
	_

	PUBLIC(RemoveFromBidOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		output.removedNumberOfShares = 0;
	_

	REGISTER_USER_FUNCTIONS

		REGISTER_USER_FUNCTION(Fees, 1);
		REGISTER_USER_FUNCTION(AssetAskOrders, 2);
		REGISTER_USER_FUNCTION(AssetBidOrders, 3);
		REGISTER_USER_FUNCTION(EntityAskOrders, 4);
		REGISTER_USER_FUNCTION(EntityBidOrders, 5);
	_

	REGISTER_USER_PROCEDURES

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		//
		//
		REGISTER_USER_PROCEDURE(AddToAskOrder, 5);
		REGISTER_USER_PROCEDURE(AddToBidOrder, 6);
		REGISTER_USER_PROCEDURE(RemoveFromAskOrder, 7);
		REGISTER_USER_PROCEDURE(RemoveFromBidOrder, 8);
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

