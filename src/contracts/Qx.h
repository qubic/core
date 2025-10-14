using namespace QPI;

struct QX2
{
};

struct QX : public ContractBase
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

		Array<Order, 256> orders;
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

		Array<Order, 256> orders;
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

		Array<Order, 256> orders;
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

		Array<Order, 256> orders;
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

	// Bid orders may be placed before an asset is issued (as part of IPO conducted on QX)
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

	struct TransferShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};
	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};


protected:
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
	Collection<_AssetOrder, 2097152 * X_MULTIPLIER> _assetOrders;

	struct _EntityOrder
	{
		id issuer;
		uint64 assetName;
		sint64 numberOfShares;
	};
	Collection<_EntityOrder, 2097152 * X_MULTIPLIER> _entityOrders;

	// TODO: change to "locals" variables and remove from state? -> every func/proc can define struct of "locals" that is passed as an argument (stored on stack structure per processor)
	sint64 _elementIndex, _elementIndex2;
	id _issuerAndAssetName;
	_AssetOrder _assetOrder;
	_EntityOrder _entityOrder;
	sint64 _price;
	sint64 _fee;
	AssetAskOrders_output::Order _assetAskOrder;
	AssetBidOrders_output::Order _assetBidOrder;
	EntityAskOrders_output::Order _entityAskOrder;
	EntityBidOrders_output::Order _entityBidOrder;

	struct _TradeMessage
	{
		unsigned int _contractIndex;
		unsigned int _type;

		id issuer;
		uint64 assetName;
		sint64 price;
		sint64 numberOfShares;

		sint8 _terminator;
	} _tradeMessage;

	struct _NumberOfReservedShares_input
	{
		id issuer;
		uint64 assetName;
	} _numberOfReservedShares_input;
	struct _NumberOfReservedShares_output
	{
		sint64 numberOfShares;
	} _numberOfReservedShares_output;

	struct _NumberOfReservedShares_locals
	{
		sint64 _elementIndex;
		_EntityOrder _entityOrder;
	};

	PRIVATE_FUNCTION_WITH_LOCALS(_NumberOfReservedShares)
	{
		output.numberOfShares = 0;

		locals._elementIndex = state._entityOrders.headIndex(qpi.invocator(), 0);
		while (locals._elementIndex != NULL_INDEX)
		{
			locals._entityOrder = state._entityOrders.element(locals._elementIndex);
			if (locals._entityOrder.assetName == input.assetName
				&& locals._entityOrder.issuer == input.issuer)
			{
				output.numberOfShares += locals._entityOrder.numberOfShares;
			}

			locals._elementIndex = state._entityOrders.nextElementIndex(locals._elementIndex);
		}
	}


	PUBLIC_FUNCTION(Fees)
	{
		output.assetIssuanceFee = state._assetIssuanceFee;
		output.transferFee = state._transferFee;
		output.tradeFee = state._tradeFee;
	}


	struct AssetAskOrders_locals
	{
		sint64 _elementIndex, _elementIndex2;
		id _issuerAndAssetName;
		_AssetOrder _assetOrder;
		AssetAskOrders_output::Order _assetAskOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(AssetAskOrders)
	{
		locals._issuerAndAssetName = input.issuer;
		locals._issuerAndAssetName.u64._3 = input.assetName;

		locals._elementIndex = state._assetOrders.headIndex(locals._issuerAndAssetName, 0);
		locals._elementIndex2 = 0;
		while (locals._elementIndex != NULL_INDEX
			&& locals._elementIndex2 < 256)
		{
			if (input.offset > 0)
			{
				input.offset--;
			}
			else
			{
				locals._assetAskOrder.price = -state._assetOrders.priority(locals._elementIndex);
				locals._assetOrder = state._assetOrders.element(locals._elementIndex);
				locals._assetAskOrder.entity = locals._assetOrder.entity;
				locals._assetAskOrder.numberOfShares = locals._assetOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._assetAskOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state._assetOrders.nextElementIndex(locals._elementIndex);
		}

		if (locals._elementIndex2 < 256)
		{
			locals._assetAskOrder.entity = NULL_ID;
			locals._assetAskOrder.price = 0;
			locals._assetAskOrder.numberOfShares = 0;
			while (locals._elementIndex2 < 256)
			{
				output.orders.set(locals._elementIndex2, locals._assetAskOrder);
				locals._elementIndex2++;
			}
		}
	}


	struct AssetBidOrders_locals
	{
		sint64 _elementIndex, _elementIndex2;
		id _issuerAndAssetName;
		_AssetOrder _assetOrder;
		AssetBidOrders_output::Order _assetBidOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(AssetBidOrders)
	{
		locals._issuerAndAssetName = input.issuer;
		locals._issuerAndAssetName.u64._3 = input.assetName;

		locals._elementIndex = state._assetOrders.headIndex(locals._issuerAndAssetName);
		locals._elementIndex2 = 0;
		while (locals._elementIndex != NULL_INDEX
			&& locals._elementIndex2 < 256)
		{
			locals._assetBidOrder.price = state._assetOrders.priority(locals._elementIndex);

			if (locals._assetBidOrder.price <= 0)
			{
				break;
			}

			if (input.offset > 0)
			{
				input.offset--;
			}
			else
			{
				locals._assetOrder = state._assetOrders.element(locals._elementIndex);
				locals._assetBidOrder.entity = locals._assetOrder.entity;
				locals._assetBidOrder.numberOfShares = locals._assetOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._assetBidOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state._assetOrders.nextElementIndex(locals._elementIndex);
		}

		if (locals._elementIndex2 < 256)
		{
			locals._assetBidOrder.entity = NULL_ID;
			locals._assetBidOrder.price = 0;
			locals._assetBidOrder.numberOfShares = 0;
			while (locals._elementIndex2 < 256)
			{
				output.orders.set(locals._elementIndex2, locals._assetBidOrder);
				locals._elementIndex2++;
			}
		}
	}


	struct EntityAskOrders_locals
	{
		sint64 _elementIndex, _elementIndex2;
		_EntityOrder _entityOrder;
		EntityAskOrders_output::Order _entityAskOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(EntityAskOrders)
	{
		locals._elementIndex = state._entityOrders.headIndex(input.entity, 0);
		locals._elementIndex2 = 0;
		while (locals._elementIndex != NULL_INDEX
			&& locals._elementIndex2 < 256)
		{
			if (input.offset > 0)
			{
				input.offset--;
			}
			else
			{
				locals._entityAskOrder.price = -state._entityOrders.priority(locals._elementIndex);
				locals._entityOrder = state._entityOrders.element(locals._elementIndex);
				locals._entityAskOrder.issuer = locals._entityOrder.issuer;
				locals._entityAskOrder.assetName = locals._entityOrder.assetName;
				locals._entityAskOrder.numberOfShares = locals._entityOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._entityAskOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state._entityOrders.nextElementIndex(locals._elementIndex);
		}

		if (locals._elementIndex2 < 256)
		{
			locals._entityAskOrder.issuer = NULL_ID;
			locals._entityAskOrder.assetName = 0;
			locals._entityAskOrder.price = 0;
			locals._entityAskOrder.numberOfShares = 0;
			while (locals._elementIndex2 < 256)
			{
				output.orders.set(locals._elementIndex2, locals._entityAskOrder);
				locals._elementIndex2++;
			}
		}
	}

	
	struct EntityBidOrders_locals
	{
		sint64 _elementIndex, _elementIndex2;
		_EntityOrder _entityOrder;
		EntityBidOrders_output::Order _entityBidOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(EntityBidOrders)
	{
		locals._elementIndex = state._entityOrders.headIndex(input.entity);
		locals._elementIndex2 = 0;
		while (locals._elementIndex != NULL_INDEX
			&& locals._elementIndex2 < 256)
		{
			locals._entityBidOrder.price = state._entityOrders.priority(locals._elementIndex);

			if (locals._entityBidOrder.price <= 0)
			{
				break;
			}

			if (input.offset > 0)
			{
				input.offset--;
			}
			else
			{
				locals._entityOrder = state._entityOrders.element(locals._elementIndex);
				locals._entityBidOrder.issuer = locals._entityOrder.issuer;
				locals._entityBidOrder.assetName = locals._entityOrder.assetName;
				locals._entityBidOrder.numberOfShares = locals._entityOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._entityBidOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state._entityOrders.nextElementIndex(locals._elementIndex);
		}

		if (locals._elementIndex2 < 256)
		{
			locals._entityBidOrder.issuer = NULL_ID;
			locals._entityBidOrder.assetName = 0;
			locals._entityBidOrder.price = 0;
			locals._entityBidOrder.numberOfShares = 0;
			while (locals._elementIndex2 < 256)
			{
				output.orders.set(locals._elementIndex2, locals._entityBidOrder);
				locals._elementIndex2++;
			}
		}
	}


	PUBLIC_PROCEDURE(IssueAsset)
	{
		if (qpi.invocationReward() < state._assetIssuanceFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.issuedNumberOfShares = 0;
		}
		else
		{
			if (qpi.invocationReward() > state._assetIssuanceFee)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state._assetIssuanceFee);
			}
			state._earnedAmount += state._assetIssuanceFee;

			output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
		}
	}

	PUBLIC_PROCEDURE(TransferShareOwnershipAndPossession)
	{
		if (qpi.invocationReward() < state._transferFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.transferredNumberOfShares = 0;
		}
		else
		{
			if (qpi.invocationReward() > state._transferFee)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state._transferFee);
			}
			state._earnedAmount += state._transferFee;

			state._numberOfReservedShares_input.issuer = input.issuer;
			state._numberOfReservedShares_input.assetName = input.assetName;
			CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
			if (qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
			{
				output.transferredNumberOfShares = 0;
			}
			else
			{
				output.transferredNumberOfShares = qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfShares;
			}
		}
	}

	PUBLIC_PROCEDURE(AddToAskOrder)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.price <= 0 || input.price >= MAX_AMOUNT
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT)
		{
			output.addedNumberOfShares = 0;
		}
		else
		{
			state._numberOfReservedShares_input.issuer = input.issuer;
			state._numberOfReservedShares_input.assetName = input.assetName;
			CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
			if (qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
			{
				output.addedNumberOfShares = 0;
			}
			else
			{
				output.addedNumberOfShares = input.numberOfShares;

				state._issuerAndAssetName = input.issuer;
				state._issuerAndAssetName.u64._3 = input.assetName;

				state._elementIndex = state._entityOrders.headIndex(qpi.invocator(), -input.price);
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
						state._entityOrder.numberOfShares += input.numberOfShares;
						state._entityOrders.replace(state._elementIndex, state._entityOrder);

						state._elementIndex = state._assetOrders.headIndex(state._issuerAndAssetName, -input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state._assetOrder = state._assetOrders.element(state._elementIndex);
							if (state._assetOrder.entity == qpi.invocator())
							{
								state._assetOrder.numberOfShares += input.numberOfShares;
								state._assetOrders.replace(state._elementIndex, state._assetOrder);

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
							state._elementIndex = state._assetOrders.remove(state._elementIndex);

							state._elementIndex2 = state._entityOrders.headIndex(state._assetOrder.entity, state._price);
							while (true) // Impossible for the corresponding entity order to not exist
							{
								state._entityOrder = state._entityOrders.element(state._elementIndex2);
								if (state._entityOrder.assetName == input.assetName
									&& state._entityOrder.issuer == input.issuer)
								{
									state._entityOrders.remove(state._elementIndex2);

									break;
								}

								state._elementIndex2 = state._entityOrders.nextElementIndex(state._elementIndex2);
							}

							state._fee = div<sint64>(state._price * state._assetOrder.numberOfShares * state._tradeFee, 1000000000LL) + 1;
							state._earnedAmount += state._fee;
							qpi.transfer(qpi.invocator(), state._price * state._assetOrder.numberOfShares - state._fee);
							qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), state._assetOrder.numberOfShares, state._assetOrder.entity);

							state._tradeMessage.issuer = input.issuer;
							state._tradeMessage.assetName = input.assetName;
							state._tradeMessage.price = state._price;
							state._tradeMessage.numberOfShares = state._assetOrder.numberOfShares;
							LOG_INFO(state._tradeMessage);

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

							state._fee = div<sint64>(state._price * input.numberOfShares * state._tradeFee, 1000000000LL) + 1;
							state._earnedAmount += state._fee;
							qpi.transfer(qpi.invocator(), state._price * input.numberOfShares - state._fee);
							qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, state._assetOrder.entity);

							state._tradeMessage.issuer = input.issuer;
							state._tradeMessage.assetName = input.assetName;
							state._tradeMessage.price = state._price;
							state._tradeMessage.numberOfShares = input.numberOfShares;
							LOG_INFO(state._tradeMessage);

							input.numberOfShares = 0;

							break;
						}
					}

					if (input.numberOfShares > 0)
					{
						state._assetOrder.entity = qpi.invocator();
						state._assetOrder.numberOfShares = input.numberOfShares;
						state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, -input.price);

						state._entityOrder.issuer = input.issuer;
						state._entityOrder.assetName = input.assetName;
						state._entityOrder.numberOfShares = input.numberOfShares;
						state._entityOrders.add(qpi.invocator(), state._entityOrder, -input.price);
					}
				}
			}
		}
	}

	PUBLIC_PROCEDURE(AddToBidOrder)
	{
		if (input.price <= 0  || input.price >= MAX_AMOUNT
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT
			|| qpi.invocationReward() < smul(input.price, input.numberOfShares))
		{
			output.addedNumberOfShares = 0;

			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
		}
		else
		{
			if (qpi.invocationReward() > smul(input.price, input.numberOfShares))
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - smul(input.price, input.numberOfShares));
			}

			output.addedNumberOfShares = input.numberOfShares;

			state._issuerAndAssetName = input.issuer;
			state._issuerAndAssetName.u64._3 = input.assetName;

			state._elementIndex = state._entityOrders.tailIndex(qpi.invocator(), input.price);
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
					state._entityOrder.numberOfShares += input.numberOfShares;
					state._entityOrders.replace(state._elementIndex, state._entityOrder);

					state._elementIndex = state._assetOrders.tailIndex(state._issuerAndAssetName, input.price);
					while (true) // Impossible for the corresponding asset order to not exist
					{
						state._assetOrder = state._assetOrders.element(state._elementIndex);
						if (state._assetOrder.entity == qpi.invocator())
						{
							state._assetOrder.numberOfShares += input.numberOfShares;
							state._assetOrders.replace(state._elementIndex, state._assetOrder);

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
						state._elementIndex = state._assetOrders.remove(state._elementIndex);

						state._elementIndex2 = state._entityOrders.headIndex(state._assetOrder.entity, -state._price);
						while (true) // Impossible for the corresponding entity order to not exist
						{
							state._entityOrder = state._entityOrders.element(state._elementIndex2);
							if (state._entityOrder.assetName == input.assetName
								&& state._entityOrder.issuer == input.issuer)
							{
								state._entityOrders.remove(state._elementIndex2);

								break;
							}

							state._elementIndex2 = state._entityOrders.nextElementIndex(state._elementIndex2);
						}

						state._fee = div<sint64>(state._price * state._assetOrder.numberOfShares * state._tradeFee, 1000000000LL) + 1;
						state._earnedAmount += state._fee;
						qpi.transfer(state._assetOrder.entity, state._price * state._assetOrder.numberOfShares - state._fee);
						qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, state._assetOrder.entity, state._assetOrder.entity, state._assetOrder.numberOfShares, qpi.invocator());
						if (input.price > state._price)
						{
							qpi.transfer(qpi.invocator(), (input.price - state._price) * state._assetOrder.numberOfShares);
						}

						state._tradeMessage.issuer = input.issuer;
						state._tradeMessage.assetName = input.assetName;
						state._tradeMessage.price = state._price;
						state._tradeMessage.numberOfShares = state._assetOrder.numberOfShares;
						LOG_INFO(state._tradeMessage);

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

						state._fee = div<sint64>(state._price * input.numberOfShares * state._tradeFee, 1000000000LL) + 1;
						state._earnedAmount += state._fee;
						qpi.transfer(state._assetOrder.entity, state._price * input.numberOfShares - state._fee);
						qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, state._assetOrder.entity, state._assetOrder.entity, input.numberOfShares, qpi.invocator());
						if (input.price > state._price)
						{
							qpi.transfer(qpi.invocator(), (input.price - state._price) * input.numberOfShares);
						}

						state._tradeMessage.issuer = input.issuer;
						state._tradeMessage.assetName = input.assetName;
						state._tradeMessage.price = state._price;
						state._tradeMessage.numberOfShares = input.numberOfShares;
						LOG_INFO(state._tradeMessage);

						input.numberOfShares = 0;

						break;
					}
				}

				if (input.numberOfShares > 0)
				{
					state._assetOrder.entity = qpi.invocator();
					state._assetOrder.numberOfShares = input.numberOfShares;
					state._assetOrders.add(state._issuerAndAssetName, state._assetOrder, input.price);

					state._entityOrder.issuer = input.issuer;
					state._entityOrder.assetName = input.assetName;
					state._entityOrder.numberOfShares = input.numberOfShares;
					state._entityOrders.add(qpi.invocator(), state._entityOrder, input.price);
				}
			}
		}
	}

	PUBLIC_PROCEDURE(RemoveFromAskOrder)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.price <= 0 || input.price >= MAX_AMOUNT
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT)
		{
			output.removedNumberOfShares = 0;
		}
		else
		{
			state._issuerAndAssetName = input.issuer;
			state._issuerAndAssetName.u64._3 = input.assetName;

			state._elementIndex = state._entityOrders.headIndex(qpi.invocator(), -input.price);
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
						state._entityOrder.numberOfShares -= input.numberOfShares;
						if (state._entityOrder.numberOfShares > 0)
						{
							state._entityOrders.replace(state._elementIndex, state._entityOrder);
						}
						else
						{
							state._entityOrders.remove(state._elementIndex);
						}

						state._elementIndex = state._assetOrders.headIndex(state._issuerAndAssetName, -input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state._assetOrder = state._assetOrders.element(state._elementIndex);
							if (state._assetOrder.entity == qpi.invocator())
							{
								state._assetOrder.numberOfShares -= input.numberOfShares;
								if (state._assetOrder.numberOfShares > 0)
								{
									state._assetOrders.replace(state._elementIndex, state._assetOrder);
								}
								else
								{
									state._assetOrders.remove(state._elementIndex);
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
	}

	PUBLIC_PROCEDURE(RemoveFromBidOrder)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.price <= 0 || input.price >= MAX_AMOUNT
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT)
		{
			output.removedNumberOfShares = 0;
		}
		else
		{
			state._issuerAndAssetName = input.issuer;
			state._issuerAndAssetName.u64._3 = input.assetName;

			state._elementIndex = state._entityOrders.tailIndex(qpi.invocator(), input.price);
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
						state._entityOrder.numberOfShares -= input.numberOfShares;
						if (state._entityOrder.numberOfShares > 0)
						{
							state._entityOrders.replace(state._elementIndex, state._entityOrder);
						}
						else
						{
							state._entityOrders.remove(state._elementIndex);
						}

						state._elementIndex = state._assetOrders.tailIndex(state._issuerAndAssetName, input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state._assetOrder = state._assetOrders.element(state._elementIndex);
							if (state._assetOrder.entity == qpi.invocator())
							{
								state._assetOrder.numberOfShares -= input.numberOfShares;
								if (state._assetOrder.numberOfShares > 0)
								{
									state._assetOrders.replace(state._elementIndex, state._assetOrder);
								}
								else
								{
									state._assetOrders.remove(state._elementIndex);
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

				qpi.transfer(qpi.invocator(), input.price * input.numberOfShares);
			}
		}
	}

	PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		// no fee
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
	
		state._numberOfReservedShares_input.issuer = input.asset.issuer;
		state._numberOfReservedShares_input.assetName = input.asset.assetName;
		CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
		}
		else
		{
			if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, 0) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
			}
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(Fees, 1);
		REGISTER_USER_FUNCTION(AssetAskOrders, 2);
		REGISTER_USER_FUNCTION(AssetBidOrders, 3);
		REGISTER_USER_FUNCTION(EntityAskOrders, 4);
		REGISTER_USER_FUNCTION(EntityBidOrders, 5);

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		//
		//
		REGISTER_USER_PROCEDURE(AddToAskOrder, 5);
		REGISTER_USER_PROCEDURE(AddToBidOrder, 6);
		REGISTER_USER_PROCEDURE(RemoveFromAskOrder, 7);
		REGISTER_USER_PROCEDURE(RemoveFromBidOrder, 8);

		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 9);
	}

	INITIALIZE()
	{
		// No need to initialize _earnedAmount and other variables with 0, whole contract state is zeroed before initialization is invoked

		state._assetIssuanceFee = 1000000000;

		/* Old values before epoch 138 
		state._transferFee = 1000000;
		state._tradeFee = 5000000; // 0.5%
		*/

		// New values since epoch 138
		state._transferFee = 100;
		state._tradeFee = 3000000; // 0.3%
	}

	END_TICK()
	{
		if ((div((state._earnedAmount - state._distributedAmount), 676ULL) > 0) && (state._earnedAmount > state._distributedAmount))
		{
			if (qpi.distributeDividends(div((state._earnedAmount - state._distributedAmount), 676ULL)))
			{
				state._distributedAmount += div((state._earnedAmount - state._distributedAmount), 676ULL) * NUMBER_OF_COMPUTORS;
			}
		}

		// Cleanup collections if more than 30% of hash maps are marked for removal
		state._assetOrders.cleanupIfNeeded(30);
		state._entityOrders.cleanupIfNeeded(30);
	}

	PRE_RELEASE_SHARES()
	{
		// system procedure called before releasing asset management rights
		// when another contract wants to acquire asset management rights from QX
		// -> always reject (default); rights can only be transferred upon user request via TransferShareManagementRights
	}

	POST_RELEASE_SHARES()
	{
	}

	PRE_ACQUIRE_SHARES()
	{
		// system procedure called before acquiring asset management rights
		// when another contract wants to release asset management rights to QX
		// -> always accept given the fee is paid
		output.requestedFee = state._transferFee;
		output.allowTransfer = true;
	}

	POST_ACQUIRE_SHARES()
	{
	}
};

