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

	struct _AssetOrder
	{
		id entity;
		sint64 numberOfShares;
	};
	collection<_AssetOrder, 2097152 * X_MULTIPLIER> _assetOrders;

	struct _EntityOrder
	{
		id issuer;
		uint64 assetName;
		sint64 numberOfShares;
	};
	collection<_EntityOrder, 2097152 * X_MULTIPLIER> _entityOrders;

	sint64 _elementIndex, _elementIndex2;
	id _issuerAndAssetName;
	_AssetOrder _assetOrder;
	_EntityOrder _entityOrder;
	sint64 _price;
	sint64 _fee;

	struct _NumberOfReservedShares_input
	{
		id issuer;
		uint64 assetName;
	} _numberOfReservedShares_input;
	struct _NumberOfReservedShares_output
	{
		sint64 numberOfShares;
	} _numberOfReservedShares_output;

	PRIVATE(_NumberOfReservedShares)

		output.numberOfShares = 0;

		state._elementIndex = state._entityOrders.headIndex(invocator(), 0);
		while (state._elementIndex != NULL_INDEX)
		{
			state._entityOrder = state._entityOrders.element(state._elementIndex);
			if (state._entityOrder.assetName == input.assetName
				&& state._entityOrder.issuer == input.issuer)
			{
				output.numberOfShares += state._entityOrder.numberOfShares;
			}

			state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
		}
	_

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

			state._numberOfReservedShares_input.issuer = input.issuer;
			state._numberOfReservedShares_input.assetName = input.assetName;
			_NumberOfReservedShares(state, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
			if (numberOfPossessedShares(input.assetName, input.issuer, invocator(), invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
			{
				output.transferredNumberOfShares = 0;
			}
			else
			{
				output.transferredNumberOfShares = transferShareOwnershipAndPossession(input.assetName, input.issuer, invocator(), invocator(), input.numberOfShares, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfShares;
			}
		}
	_

	PUBLIC(AddToAskOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		if (input.price <= 0
			|| input.numberOfShares <= 0)
		{
			output.addedNumberOfShares = 0;
		}
		else
		{
			state._numberOfReservedShares_input.issuer = input.issuer;
			state._numberOfReservedShares_input.assetName = input.assetName;
			_NumberOfReservedShares(state, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
			if (numberOfPossessedShares(input.assetName, input.issuer, invocator(), invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
			{
				output.addedNumberOfShares = 0;
			}
			else
			{
				output.addedNumberOfShares = input.numberOfShares;

				state._issuerAndAssetName = input.issuer;
				state._issuerAndAssetName._0 = input.assetName;

				state._elementIndex = state._entityOrders.headIndex(invocator(), -input.price);
				while (state._elementIndex != NULL_INDEX)
				{
					if (state._entityOrders.priority(state._elementIndex) != -input.price)
					{
						state._elementIndex = NULL_INDEX;

						break;
					}

					state._entityOrder = state._entityOrders.element(state._elementIndex);
					if (state._entityOrder.assetName == input.assetName
						&& state._entityOrder.issuer == input.issuer)
					{
						state._entityOrders.remove(state._elementIndex);
						state._entityOrder.numberOfShares += input.numberOfShares;
						state._entityOrders.add(invocator(), state._entityOrder, -input.price);

						state._elementIndex = state._assetOrders.headIndex(state._issuerAndAssetName, -input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state._assetOrder = state._assetOrders.element(state._elementIndex);
							if (state._assetOrder.entity == invocator())
							{
								state._assetOrders.remove(state._elementIndex);
								state._assetOrder.numberOfShares += input.numberOfShares;
								state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, -input.price);

								break;
							}

							state._elementIndex = state._assetOrders.nextElementIndex(state._elementIndex);
						}

						break;
					}

					state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
				}

				if (state._elementIndex == NULL_INDEX) // No other ask orders for the same asset at the same price found
				{
					state._elementIndex = state._assetOrders.headIndex(state._issuerAndAssetName);
					while (state._elementIndex != NULL_INDEX
						&& input.numberOfShares > 0)
					{
						state._price = state._assetOrders.priority(state._elementIndex);

						if (state._price < input.price)
						{
							break;
						}

						state._assetOrder = state._assetOrders.element(state._elementIndex);
						if (state._assetOrder.numberOfShares <= input.numberOfShares)
						{
							state._elementIndex2 = state._assetOrders.nextElementIndex(state._elementIndex);

							state._assetOrders.remove(state._elementIndex);

							state._elementIndex = state._entityOrders.headIndex(state._assetOrder.entity, state._price);
							while (true) // Impossible for the corresponding entity order to not exist
							{
								state._entityOrder = state._entityOrders.element(state._elementIndex);
								if (state._entityOrder.assetName == input.assetName
									&& state._entityOrder.issuer == input.issuer)
								{
									state._entityOrders.remove(state._elementIndex);

									break;
								}

								state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
							}

							state._fee = (state._price * state._assetOrder.numberOfShares * state._tradeFee / 1000000000UL) + 1;
							state._earnedAmount += state._fee;
							transfer(invocator(), state._price * state._assetOrder.numberOfShares - state._fee);
							transferShareOwnershipAndPossession(input.assetName, input.issuer, invocator(), invocator(), state._assetOrder.numberOfShares, state._assetOrder.entity);

							state._elementIndex = state._elementIndex2;
							input.numberOfShares -= state._assetOrder.numberOfShares;
						}
						else
						{
							state._assetOrder.numberOfShares -= input.numberOfShares;
							state._assetOrders.replace(state._elementIndex, state._assetOrder);

							state._elementIndex = state._entityOrders.headIndex(state._assetOrder.entity, state._price);
							while (true) // Impossible for the corresponding entity order to not exist
							{
								state._entityOrder = state._entityOrders.element(state._elementIndex);
								if (state._entityOrder.assetName == input.assetName
									&& state._entityOrder.issuer == input.issuer)
								{
									state._entityOrder.numberOfShares -= input.numberOfShares;
									state._entityOrders.replace(state._elementIndex, state._entityOrder);

									break;
								}

								state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
							}

							state._fee = (state._price * input.numberOfShares * state._tradeFee / 1000000000UL) + 1;
							state._earnedAmount += state._fee;
							transfer(invocator(), state._price * input.numberOfShares - state._fee);
							transferShareOwnershipAndPossession(input.assetName, input.issuer, invocator(), invocator(), input.numberOfShares, state._assetOrder.entity);

							input.numberOfShares = 0;

							break;
						}
					}

					if (input.numberOfShares > 0)
					{
						state._assetOrder.entity = invocator();
						state._assetOrder.numberOfShares = input.numberOfShares;
						state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, -input.price);

						state._entityOrder.issuer = input.issuer;
						state._entityOrder.assetName = input.assetName;
						state._entityOrder.numberOfShares = input.numberOfShares;
						state._entityOrders.add(invocator(), state._entityOrder, -input.price);
					}
				}
			}
		}
	_

	PUBLIC(AddToBidOrder)

		if (input.price <= 0
			|| input.numberOfShares <= 0
			|| invocationReward() < input.price * input.numberOfShares)
		{
			output.addedNumberOfShares = 0;

			if (invocationReward() > 0)
			{
				transfer(invocator(), invocationReward());
			}
		}
		else
		{
			output.addedNumberOfShares = input.numberOfShares;

			state._issuerAndAssetName = input.issuer;
			state._issuerAndAssetName._0 = input.assetName;

			state._elementIndex = state._entityOrders.tailIndex(invocator(), input.price);
			while (state._elementIndex != NULL_INDEX)
			{
				if (state._entityOrders.priority(state._elementIndex) != input.price)
				{
					state._elementIndex = NULL_INDEX;

					break;
				}

				state._entityOrder = state._entityOrders.element(state._elementIndex);
				if (state._entityOrder.assetName == input.assetName
					&& state._entityOrder.issuer == input.issuer)
				{
					state._entityOrders.remove(state._elementIndex);
					state._entityOrder.numberOfShares += input.numberOfShares;
					state._entityOrders.add(invocator(), state._entityOrder, input.price);

					state._elementIndex = state._assetOrders.tailIndex(state._issuerAndAssetName, input.price);
					while (true) // Impossible for the corresponding asset order to not exist
					{
						state._assetOrder = state._assetOrders.element(state._elementIndex);
						if (state._assetOrder.entity == invocator())
						{
							state._assetOrders.remove(state._elementIndex);
							state._assetOrder.numberOfShares += input.numberOfShares;
							state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, input.price);

							break;
						}

						state._elementIndex = state._assetOrders.prevElementIndex(state._elementIndex);
					}

					break;
				}

				state._elementIndex = state._entityOrders.prevElementIndex(state._elementIndex);
			}

			if (state._elementIndex == NULL_INDEX) // No other bid orders for the same asset at the same price found
			{
				state._elementIndex = state._assetOrders.headIndex(state._issuerAndAssetName, 0);
				while (state._elementIndex != NULL_INDEX
					&& input.numberOfShares > 0)
				{
					state._price = -state._assetOrders.priority(state._elementIndex);

					if (state._price > input.price)
					{
						break;
					}

					state._assetOrder = state._assetOrders.element(state._elementIndex);
					if (state._assetOrder.numberOfShares <= input.numberOfShares)
					{
						state._elementIndex2 = state._assetOrders.nextElementIndex(state._elementIndex);

						state._assetOrders.remove(state._elementIndex);

						state._elementIndex = state._entityOrders.headIndex(state._assetOrder.entity, -state._price);
						while (true) // Impossible for the corresponding entity order to not exist
						{
							state._entityOrder = state._entityOrders.element(state._elementIndex);
							if (state._entityOrder.assetName == input.assetName
								&& state._entityOrder.issuer == input.issuer)
							{
								state._entityOrders.remove(state._elementIndex);

								break;
							}

							state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
						}

						state._fee = (state._price * state._assetOrder.numberOfShares * state._tradeFee / 1000000000UL) + 1;
						state._earnedAmount += state._fee;
						transfer(state._assetOrder.entity, state._price * state._assetOrder.numberOfShares - state._fee);
						transferShareOwnershipAndPossession(input.assetName, input.issuer, state._assetOrder.entity, state._assetOrder.entity, state._assetOrder.numberOfShares, invocator());

						state._elementIndex = state._elementIndex2;
						input.numberOfShares -= state._assetOrder.numberOfShares;
					}
					else
					{
						state._assetOrder.numberOfShares -= input.numberOfShares;
						state._assetOrders.replace(state._elementIndex, state._assetOrder);

						state._elementIndex = state._entityOrders.headIndex(state._assetOrder.entity, -state._price);
						while (true) // Impossible for the corresponding entity order to not exist
						{
							state._entityOrder = state._entityOrders.element(state._elementIndex);
							if (state._entityOrder.assetName == input.assetName
								&& state._entityOrder.issuer == input.issuer)
							{
								state._entityOrder.numberOfShares -= input.numberOfShares;
								state._entityOrders.replace(state._elementIndex, state._entityOrder);

								break;
							}

							state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
						}

						state._fee = (state._price * input.numberOfShares * state._tradeFee / 1000000000UL) + 1;
						state._earnedAmount += state._fee;
						transfer(state._assetOrder.entity, state._price * input.numberOfShares - state._fee);
						transferShareOwnershipAndPossession(input.assetName, input.issuer, state._assetOrder.entity, state._assetOrder.entity, input.numberOfShares, invocator());

						input.numberOfShares = 0;

						break;
					}
				}

				if (input.numberOfShares > 0)
				{
					state._assetOrder.entity = invocator();
					state._assetOrder.numberOfShares = input.numberOfShares;
					state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, input.price);

					state._entityOrder.issuer = input.issuer;
					state._entityOrder.assetName = input.assetName;
					state._entityOrder.numberOfShares = input.numberOfShares;
					state._entityOrders.add(invocator(), state._entityOrder, input.price);
				}
			}

			if (invocationReward() > input.price * input.numberOfShares)
			{
				transfer(invocator(), invocationReward() - input.price * input.numberOfShares);
			}
		}
	_

	PUBLIC(RemoveFromAskOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		if (input.price <= 0
			|| input.numberOfShares <= 0)
		{
			output.removedNumberOfShares = 0;
		}
		else
		{
			state._issuerAndAssetName = input.issuer;
			state._issuerAndAssetName._0 = input.assetName;

			state._elementIndex = state._entityOrders.headIndex(invocator(), -input.price);
			while (state._elementIndex != NULL_INDEX)
			{
				if (state._entityOrders.priority(state._elementIndex) != -input.price)
				{
					state._elementIndex = NULL_INDEX;

					break;
				}

				state._entityOrder = state._entityOrders.element(state._elementIndex);
				if (state._entityOrder.assetName == input.assetName
					&& state._entityOrder.issuer == input.issuer)
				{
					if (state._entityOrder.numberOfShares < input.numberOfShares)
					{
						state._elementIndex = NULL_INDEX;
					}
					else
					{
						state._entityOrders.remove(state._elementIndex);
						state._entityOrder.numberOfShares -= input.numberOfShares;
						if (state._entityOrder.numberOfShares > 0)
						{
							state._entityOrders.add(invocator(), state._entityOrder, -input.price);
						}

						state._elementIndex = state._assetOrders.headIndex(state._issuerAndAssetName, -input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state._assetOrder = state._assetOrders.element(state._elementIndex);
							if (state._assetOrder.entity == invocator())
							{
								state._assetOrders.remove(state._elementIndex);
								state._assetOrder.numberOfShares -= input.numberOfShares;
								if (state._assetOrder.numberOfShares > 0)
								{
									state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, -input.price);
								}

								break;
							}

							state._elementIndex = state._assetOrders.nextElementIndex(state._elementIndex);
						}
					}

					break;
				}

				state._elementIndex = state._entityOrders.nextElementIndex(state._elementIndex);
			}

			if (state._elementIndex == NULL_INDEX) // No other ask orders for the same asset at the same price found
			{
				output.removedNumberOfShares = 0;
			}
			else
			{
				output.removedNumberOfShares = input.numberOfShares;
			}
		}
	_

	PUBLIC(RemoveFromBidOrder)

		if (invocationReward() > 0)
		{
			transfer(invocator(), invocationReward());
		}

		if (input.price <= 0
			|| input.numberOfShares <= 0)
		{
			output.removedNumberOfShares = 0;
		}
		else
		{
			state._issuerAndAssetName = input.issuer;
			state._issuerAndAssetName._0 = input.assetName;

			state._elementIndex = state._entityOrders.tailIndex(invocator(), input.price);
			while (state._elementIndex != NULL_INDEX)
			{
				if (state._entityOrders.priority(state._elementIndex) != input.price)
				{
					state._elementIndex = NULL_INDEX;

					break;
				}

				state._entityOrder = state._entityOrders.element(state._elementIndex);
				if (state._entityOrder.assetName == input.assetName
					&& state._entityOrder.issuer == input.issuer)
				{
					if (state._entityOrder.numberOfShares < input.numberOfShares)
					{
						state._elementIndex = NULL_INDEX;
					}
					else
					{
						state._entityOrders.remove(state._elementIndex);
						state._entityOrder.numberOfShares -= input.numberOfShares;
						if (state._entityOrder.numberOfShares > 0)
						{
							state._entityOrders.add(invocator(), state._entityOrder, input.price);
						}

						state._elementIndex = state._assetOrders.tailIndex(state._issuerAndAssetName, input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state._assetOrder = state._assetOrders.element(state._elementIndex);
							if (state._assetOrder.entity == invocator())
							{
								state._assetOrders.remove(state._elementIndex);
								state._assetOrder.numberOfShares -= input.numberOfShares;
								if (state._assetOrder.numberOfShares > 0)
								{
									state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, input.price);
								}

								break;
							}

							state._elementIndex = state._assetOrders.prevElementIndex(state._elementIndex);
						}
					}

					break;
				}

				state._elementIndex = state._entityOrders.prevElementIndex(state._elementIndex);
			}

			if (state._elementIndex == NULL_INDEX) // No other bid orders for the same asset at the same price found
			{
				output.removedNumberOfShares = 0;
			}
			else
			{
				output.removedNumberOfShares = input.numberOfShares;

				transfer(invocator(), input.price * input.numberOfShares);
			}
		}
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

