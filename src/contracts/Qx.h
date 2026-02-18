using namespace QPI;

struct QX2
{
};

struct QX : public ContractBase
{
	// Types used in StateData and procedures
	struct AssetOrder
	{
		id entity;
		sint64 numberOfShares;
	};

	struct EntityOrder
	{
		id issuer;
		uint64 assetName;
		sint64 numberOfShares;
	};

	struct TradeMessage
	{
		unsigned int _contractIndex;
		unsigned int _type;

		id issuer;
		uint64 assetName;
		sint64 price;
		sint64 numberOfShares;

		sint8 _terminator;
	};

	struct _NumberOfReservedShares_input
	{
		id issuer;
		uint64 assetName;
	};
	struct _NumberOfReservedShares_output
	{
		sint64 numberOfShares;
	};

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

	struct StateData
	{
		uint64 _earnedAmount;
		uint64 _distributedAmount;
		uint64 _burnedAmount;

		uint32 _assetIssuanceFee; // Amount of qus
		uint32 _transferFee; // Amount of qus
		uint32 _tradeFee; // Number of billionths

		Collection<AssetOrder, 2097152 * X_MULTIPLIER> _assetOrders;
		Collection<EntityOrder, 2097152 * X_MULTIPLIER> _entityOrders;

		// TODO: change to "locals" variables and remove from state? -> every func/proc can define struct of "locals" that is passed as an argument (stored on stack structure per processor)
		sint64 _elementIndex, _elementIndex2;
		id _issuerAndAssetName;
		AssetOrder _assetOrder;
		EntityOrder _entityOrder;
		sint64 _price;
		sint64 _fee;
		AssetAskOrders_output::Order _assetAskOrder;
		AssetBidOrders_output::Order _assetBidOrder;
		EntityAskOrders_output::Order _entityAskOrder;
		EntityBidOrders_output::Order _entityBidOrder;

		TradeMessage _tradeMessage;

		_NumberOfReservedShares_input _numberOfReservedShares_input;
		_NumberOfReservedShares_output _numberOfReservedShares_output;
	};


protected:

	struct _NumberOfReservedShares_locals
	{
		sint64 _elementIndex;
		EntityOrder _entityOrder;
	};

	PRIVATE_FUNCTION_WITH_LOCALS(_NumberOfReservedShares)
	{
		output.numberOfShares = 0;

		locals._elementIndex = state.get()._entityOrders.headIndex(qpi.invocator(), 0);
		while (locals._elementIndex != NULL_INDEX)
		{
			locals._entityOrder = state.get()._entityOrders.element(locals._elementIndex);
			if (locals._entityOrder.assetName == input.assetName
				&& locals._entityOrder.issuer == input.issuer)
			{
				output.numberOfShares += locals._entityOrder.numberOfShares;
			}

			locals._elementIndex = state.get()._entityOrders.nextElementIndex(locals._elementIndex);
		}
	}


	PUBLIC_FUNCTION(Fees)
	{
		output.assetIssuanceFee = state.get()._assetIssuanceFee;
		output.transferFee = state.get()._transferFee;
		output.tradeFee = state.get()._tradeFee;
	}


	struct AssetAskOrders_locals
	{
		sint64 _elementIndex, _elementIndex2;
		id _issuerAndAssetName;
		AssetOrder _assetOrder;
		AssetAskOrders_output::Order _assetAskOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(AssetAskOrders)
	{
		locals._issuerAndAssetName = input.issuer;
		locals._issuerAndAssetName.u64._3 = input.assetName;

		locals._elementIndex = state.get()._assetOrders.headIndex(locals._issuerAndAssetName, 0);
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
				locals._assetAskOrder.price = -state.get()._assetOrders.priority(locals._elementIndex);
				locals._assetOrder = state.get()._assetOrders.element(locals._elementIndex);
				locals._assetAskOrder.entity = locals._assetOrder.entity;
				locals._assetAskOrder.numberOfShares = locals._assetOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._assetAskOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state.get()._assetOrders.nextElementIndex(locals._elementIndex);
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
		AssetOrder _assetOrder;
		AssetBidOrders_output::Order _assetBidOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(AssetBidOrders)
	{
		locals._issuerAndAssetName = input.issuer;
		locals._issuerAndAssetName.u64._3 = input.assetName;

		locals._elementIndex = state.get()._assetOrders.headIndex(locals._issuerAndAssetName);
		locals._elementIndex2 = 0;
		while (locals._elementIndex != NULL_INDEX
			&& locals._elementIndex2 < 256)
		{
			locals._assetBidOrder.price = state.get()._assetOrders.priority(locals._elementIndex);

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
				locals._assetOrder = state.get()._assetOrders.element(locals._elementIndex);
				locals._assetBidOrder.entity = locals._assetOrder.entity;
				locals._assetBidOrder.numberOfShares = locals._assetOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._assetBidOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state.get()._assetOrders.nextElementIndex(locals._elementIndex);
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
		EntityOrder _entityOrder;
		EntityAskOrders_output::Order _entityAskOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(EntityAskOrders)
	{
		locals._elementIndex = state.get()._entityOrders.headIndex(input.entity, 0);
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
				locals._entityAskOrder.price = -state.get()._entityOrders.priority(locals._elementIndex);
				locals._entityOrder = state.get()._entityOrders.element(locals._elementIndex);
				locals._entityAskOrder.issuer = locals._entityOrder.issuer;
				locals._entityAskOrder.assetName = locals._entityOrder.assetName;
				locals._entityAskOrder.numberOfShares = locals._entityOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._entityAskOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state.get()._entityOrders.nextElementIndex(locals._elementIndex);
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
		EntityOrder _entityOrder;
		EntityBidOrders_output::Order _entityBidOrder;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(EntityBidOrders)
	{
		locals._elementIndex = state.get()._entityOrders.headIndex(input.entity);
		locals._elementIndex2 = 0;
		while (locals._elementIndex != NULL_INDEX
			&& locals._elementIndex2 < 256)
		{
			locals._entityBidOrder.price = state.get()._entityOrders.priority(locals._elementIndex);

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
				locals._entityOrder = state.get()._entityOrders.element(locals._elementIndex);
				locals._entityBidOrder.issuer = locals._entityOrder.issuer;
				locals._entityBidOrder.assetName = locals._entityOrder.assetName;
				locals._entityBidOrder.numberOfShares = locals._entityOrder.numberOfShares;
				output.orders.set(locals._elementIndex2, locals._entityBidOrder);
				locals._elementIndex2++;
			}

			locals._elementIndex = state.get()._entityOrders.nextElementIndex(locals._elementIndex);
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
		if (qpi.invocationReward() < state.get()._assetIssuanceFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.issuedNumberOfShares = 0;
		}
		else
		{
			if (qpi.invocationReward() > state.get()._assetIssuanceFee)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get()._assetIssuanceFee);
			}
			state.mut()._earnedAmount += state.get()._assetIssuanceFee;

			output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
		}
	}

	PUBLIC_PROCEDURE(TransferShareOwnershipAndPossession)
	{
		if (qpi.invocationReward() < state.get()._transferFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.transferredNumberOfShares = 0;
		}
		else
		{
			if (qpi.invocationReward() > state.get()._transferFee)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get()._transferFee);
			}
			state.mut()._earnedAmount += state.get()._transferFee;

			state.mut()._numberOfReservedShares_input.issuer = input.issuer;
			state.mut()._numberOfReservedShares_input.assetName = input.assetName;
			CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
			if (qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state.get()._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
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
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT
			|| smul(input.price, input.numberOfShares) >= MAX_AMOUNT)
		{
			output.addedNumberOfShares = 0;
		}
		else
		{
			state.mut()._numberOfReservedShares_input.issuer = input.issuer;
			state.mut()._numberOfReservedShares_input.assetName = input.assetName;
			CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
			if (qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state.get()._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
			{
				output.addedNumberOfShares = 0;
			}
			else
			{
				output.addedNumberOfShares = input.numberOfShares;

				state.mut()._issuerAndAssetName = input.issuer;
				state.mut()._issuerAndAssetName.u64._3 = input.assetName;

				state.mut()._elementIndex = state.get()._entityOrders.headIndex(qpi.invocator(), -input.price);
				while (state.get()._elementIndex != NULL_INDEX)
				{
					if (state.get()._entityOrders.priority(state.get()._elementIndex) != -input.price)
					{
						state.mut()._elementIndex = NULL_INDEX;

						break;
					}

					state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex);
					if (state.get()._entityOrder.assetName == input.assetName
						&& state.get()._entityOrder.issuer == input.issuer)
					{
						state.mut()._entityOrder.numberOfShares += input.numberOfShares;
						state.mut()._entityOrders.replace(state.get()._elementIndex, state.get()._entityOrder);

						state.mut()._elementIndex = state.get()._assetOrders.headIndex(state.get()._issuerAndAssetName, -input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state.mut()._assetOrder = state.get()._assetOrders.element(state.get()._elementIndex);
							if (state.get()._assetOrder.entity == qpi.invocator())
							{
								state.mut()._assetOrder.numberOfShares += input.numberOfShares;
								state.mut()._assetOrders.replace(state.get()._elementIndex, state.get()._assetOrder);

								break;
							}

							state.mut()._elementIndex = state.get()._assetOrders.nextElementIndex(state.get()._elementIndex);
						}

						break;
					}

					state.mut()._elementIndex = state.get()._entityOrders.nextElementIndex(state.get()._elementIndex);
				}

				if (state.get()._elementIndex == NULL_INDEX) // No other ask orders for the same asset at the same price found
				{
					state.mut()._elementIndex = state.get()._assetOrders.headIndex(state.get()._issuerAndAssetName);
					while (state.get()._elementIndex != NULL_INDEX
						&& input.numberOfShares > 0)
					{
						state.mut()._price = state.get()._assetOrders.priority(state.get()._elementIndex);

						if (state.get()._price < input.price)
						{
							break;
						}

						state.mut()._assetOrder = state.get()._assetOrders.element(state.get()._elementIndex);
						if (state.get()._assetOrder.numberOfShares <= input.numberOfShares)
						{
							state.mut()._elementIndex = state.mut()._assetOrders.remove(state.get()._elementIndex);

							state.mut()._elementIndex2 = state.get()._entityOrders.headIndex(state.get()._assetOrder.entity, state.get()._price);
							while (true) // Impossible for the corresponding entity order to not exist
							{
								state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex2);
								if (state.get()._entityOrder.assetName == input.assetName
									&& state.get()._entityOrder.issuer == input.issuer)
								{
									state.mut()._entityOrders.remove(state.get()._elementIndex2);

									break;
								}

								state.mut()._elementIndex2 = state.get()._entityOrders.nextElementIndex(state.get()._elementIndex2);
							}
							if (smul(state.get()._price, state.get()._assetOrder.numberOfShares) >= div<sint64>(INT64_MAX, state.get()._tradeFee))
							{
								// in this case, traders will pay more fee because it's rounding down, it's better to split the trade into multiple smaller trades
								state.mut()._fee = div<sint64>(state.get()._price * state.get()._assetOrder.numberOfShares, div<sint64>(1000000000LL, state.get()._tradeFee)) + 1;
							}
							else
							{
								state.mut()._fee = div<sint64>(state.get()._price * state.get()._assetOrder.numberOfShares * state.get()._tradeFee, 1000000000LL) + 1;
							}
							state.mut()._earnedAmount += state.get()._fee;
							qpi.transfer(qpi.invocator(), state.get()._price * state.get()._assetOrder.numberOfShares - state.get()._fee);
							qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), state.get()._assetOrder.numberOfShares, state.get()._assetOrder.entity);

							state.mut()._tradeMessage.issuer = input.issuer;
							state.mut()._tradeMessage.assetName = input.assetName;
							state.mut()._tradeMessage.price = state.get()._price;
							state.mut()._tradeMessage.numberOfShares = state.get()._assetOrder.numberOfShares;
							LOG_INFO(state.get()._tradeMessage);

							input.numberOfShares -= state.get()._assetOrder.numberOfShares;
						}
						else
						{
							state.mut()._assetOrder.numberOfShares -= input.numberOfShares;
							state.mut()._assetOrders.replace(state.get()._elementIndex, state.get()._assetOrder);

							state.mut()._elementIndex = state.get()._entityOrders.headIndex(state.get()._assetOrder.entity, state.get()._price);
							while (true) // Impossible for the corresponding entity order to not exist
							{
								state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex);
								if (state.get()._entityOrder.assetName == input.assetName
									&& state.get()._entityOrder.issuer == input.issuer)
								{
									state.mut()._entityOrder.numberOfShares -= input.numberOfShares;
									state.mut()._entityOrders.replace(state.get()._elementIndex, state.get()._entityOrder);

									break;
								}

								state.mut()._elementIndex = state.get()._entityOrders.nextElementIndex(state.get()._elementIndex);
							}
							if (smul(state.get()._price, input.numberOfShares) >= div<sint64>(INT64_MAX, state.get()._tradeFee))
							{
								// in this case, traders will pay more fee because it's rounding down, it's better to split the trade into multiple smaller trades
								state.mut()._fee = div<sint64>(state.get()._price * input.numberOfShares, div<sint64>(1000000000LL, state.get()._tradeFee)) + 1;
							}
							else
							{
								state.mut()._fee = div<sint64>(state.get()._price * input.numberOfShares * state.get()._tradeFee, 1000000000LL) + 1;
							}
							state.mut()._earnedAmount += state.get()._fee;
							qpi.transfer(qpi.invocator(), state.get()._price * input.numberOfShares - state.get()._fee);
							qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, state.get()._assetOrder.entity);

							state.mut()._tradeMessage.issuer = input.issuer;
							state.mut()._tradeMessage.assetName = input.assetName;
							state.mut()._tradeMessage.price = state.get()._price;
							state.mut()._tradeMessage.numberOfShares = input.numberOfShares;
							LOG_INFO(state.get()._tradeMessage);

							input.numberOfShares = 0;

							break;
						}
					}

					if (input.numberOfShares > 0)
					{
						state.mut()._assetOrder.entity = qpi.invocator();
						state.mut()._assetOrder.numberOfShares = input.numberOfShares;
						state.mut()._assetOrders.add(state.get()._issuerAndAssetName, state.get()._assetOrder, -input.price);

						state.mut()._entityOrder.issuer = input.issuer;
						state.mut()._entityOrder.assetName = input.assetName;
						state.mut()._entityOrder.numberOfShares = input.numberOfShares;
						state.mut()._entityOrders.add(qpi.invocator(), state.get()._entityOrder, -input.price);
					}
				}
			}
		}
	}

	PUBLIC_PROCEDURE(AddToBidOrder)
	{
		if (input.price <= 0  || input.price >= MAX_AMOUNT
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT
			|| smul(input.price, input.numberOfShares) >= MAX_AMOUNT
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

			state.mut()._issuerAndAssetName = input.issuer;
			state.mut()._issuerAndAssetName.u64._3 = input.assetName;

			state.mut()._elementIndex = state.get()._entityOrders.tailIndex(qpi.invocator(), input.price);
			while (state.get()._elementIndex != NULL_INDEX)
			{
				if (state.get()._entityOrders.priority(state.get()._elementIndex) != input.price)
				{
					state.mut()._elementIndex = NULL_INDEX;

					break;
				}

				state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex);
				if (state.get()._entityOrder.assetName == input.assetName
					&& state.get()._entityOrder.issuer == input.issuer)
				{
					state.mut()._entityOrder.numberOfShares += input.numberOfShares;
					state.mut()._entityOrders.replace(state.get()._elementIndex, state.get()._entityOrder);

					state.mut()._elementIndex = state.get()._assetOrders.tailIndex(state.get()._issuerAndAssetName, input.price);
					while (true) // Impossible for the corresponding asset order to not exist
					{
						state.mut()._assetOrder = state.get()._assetOrders.element(state.get()._elementIndex);
						if (state.get()._assetOrder.entity == qpi.invocator())
						{
							state.mut()._assetOrder.numberOfShares += input.numberOfShares;
							state.mut()._assetOrders.replace(state.get()._elementIndex, state.get()._assetOrder);

							break;
						}

						state.mut()._elementIndex = state.get()._assetOrders.prevElementIndex(state.get()._elementIndex);
					}

					break;
				}

				state.mut()._elementIndex = state.get()._entityOrders.prevElementIndex(state.get()._elementIndex);
			}

			if (state.get()._elementIndex == NULL_INDEX) // No other bid orders for the same asset at the same price found
			{
				state.mut()._elementIndex = state.get()._assetOrders.headIndex(state.get()._issuerAndAssetName, 0);
				while (state.get()._elementIndex != NULL_INDEX
					&& input.numberOfShares > 0)
				{
					state.mut()._price = -state.get()._assetOrders.priority(state.get()._elementIndex);

					if (state.get()._price > input.price)
					{
						break;
					}

					state.mut()._assetOrder = state.get()._assetOrders.element(state.get()._elementIndex);
					if (state.get()._assetOrder.numberOfShares <= input.numberOfShares)
					{
						state.mut()._elementIndex = state.mut()._assetOrders.remove(state.get()._elementIndex);

						state.mut()._elementIndex2 = state.get()._entityOrders.headIndex(state.get()._assetOrder.entity, -state.get()._price);
						while (true) // Impossible for the corresponding entity order to not exist
						{
							state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex2);
							if (state.get()._entityOrder.assetName == input.assetName
								&& state.get()._entityOrder.issuer == input.issuer)
							{
								state.mut()._entityOrders.remove(state.get()._elementIndex2);

								break;
							}

							state.mut()._elementIndex2 = state.get()._entityOrders.nextElementIndex(state.get()._elementIndex2);
						}

						if (smul(state.get()._price, state.get()._assetOrder.numberOfShares) >= div<sint64>(INT64_MAX, state.get()._tradeFee))
						{
							state.mut()._fee = div<sint64>(state.get()._price * state.get()._assetOrder.numberOfShares, div<sint64>(1000000000LL, state.get()._tradeFee)) + 1;
						}
						else
						{
							state.mut()._fee = div<sint64>(state.get()._price * state.get()._assetOrder.numberOfShares * state.get()._tradeFee, 1000000000LL) + 1;
						}
						state.mut()._earnedAmount += state.get()._fee;
						qpi.transfer(state.get()._assetOrder.entity, state.get()._price * state.get()._assetOrder.numberOfShares - state.get()._fee);
						qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, state.get()._assetOrder.entity, state.get()._assetOrder.entity, state.get()._assetOrder.numberOfShares, qpi.invocator());
						if (input.price > state.get()._price)
						{
							qpi.transfer(qpi.invocator(), (input.price - state.get()._price) * state.get()._assetOrder.numberOfShares);
						}

						state.mut()._tradeMessage.issuer = input.issuer;
						state.mut()._tradeMessage.assetName = input.assetName;
						state.mut()._tradeMessage.price = state.get()._price;
						state.mut()._tradeMessage.numberOfShares = state.get()._assetOrder.numberOfShares;
						LOG_INFO(state.get()._tradeMessage);

						input.numberOfShares -= state.get()._assetOrder.numberOfShares;
					}
					else
					{
						state.mut()._assetOrder.numberOfShares -= input.numberOfShares;
						state.mut()._assetOrders.replace(state.get()._elementIndex, state.get()._assetOrder);

						state.mut()._elementIndex = state.get()._entityOrders.headIndex(state.get()._assetOrder.entity, -state.get()._price);
						while (true) // Impossible for the corresponding entity order to not exist
						{
							state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex);
							if (state.get()._entityOrder.assetName == input.assetName
								&& state.get()._entityOrder.issuer == input.issuer)
							{
								state.mut()._entityOrder.numberOfShares -= input.numberOfShares;
								state.mut()._entityOrders.replace(state.get()._elementIndex, state.get()._entityOrder);

								break;
							}

							state.mut()._elementIndex = state.get()._entityOrders.nextElementIndex(state.get()._elementIndex);
						}

						if (smul(state.get()._price, input.numberOfShares) >= div<sint64>(INT64_MAX, state.get()._tradeFee))
						{
							state.mut()._fee = div<sint64>(state.get()._price * input.numberOfShares, div<sint64>(1000000000LL, state.get()._tradeFee)) + 1;
						}
						else
						{
							state.mut()._fee = div<sint64>(state.get()._price * input.numberOfShares * state.get()._tradeFee, 1000000000LL) + 1;
						}
						state.mut()._earnedAmount += state.get()._fee;
						qpi.transfer(state.get()._assetOrder.entity, state.get()._price * input.numberOfShares - state.get()._fee);
						qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, state.get()._assetOrder.entity, state.get()._assetOrder.entity, input.numberOfShares, qpi.invocator());
						if (input.price > state.get()._price)
						{
							qpi.transfer(qpi.invocator(), (input.price - state.get()._price) * input.numberOfShares);
						}

						state.mut()._tradeMessage.issuer = input.issuer;
						state.mut()._tradeMessage.assetName = input.assetName;
						state.mut()._tradeMessage.price = state.get()._price;
						state.mut()._tradeMessage.numberOfShares = input.numberOfShares;
						LOG_INFO(state.get()._tradeMessage);

						input.numberOfShares = 0;

						break;
					}
				}

				if (input.numberOfShares > 0)
				{
					state.mut()._assetOrder.entity = qpi.invocator();
					state.mut()._assetOrder.numberOfShares = input.numberOfShares;
					state.mut()._assetOrders.add(state.get()._issuerAndAssetName, state.get()._assetOrder, input.price);

					state.mut()._entityOrder.issuer = input.issuer;
					state.mut()._entityOrder.assetName = input.assetName;
					state.mut()._entityOrder.numberOfShares = input.numberOfShares;
					state.mut()._entityOrders.add(qpi.invocator(), state.get()._entityOrder, input.price);
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
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT
			|| smul(input.price, input.numberOfShares) >= MAX_AMOUNT)
		{
			output.removedNumberOfShares = 0;
		}
		else
		{
			state.mut()._issuerAndAssetName = input.issuer;
			state.mut()._issuerAndAssetName.u64._3 = input.assetName;

			state.mut()._elementIndex = state.get()._entityOrders.headIndex(qpi.invocator(), -input.price);
			while (state.get()._elementIndex != NULL_INDEX)
			{
				if (state.get()._entityOrders.priority(state.get()._elementIndex) != -input.price)
				{
					state.mut()._elementIndex = NULL_INDEX;

					break;
				}

				state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex);
				if (state.get()._entityOrder.assetName == input.assetName
					&& state.get()._entityOrder.issuer == input.issuer)
				{
					if (state.get()._entityOrder.numberOfShares < input.numberOfShares)
					{
						state.mut()._elementIndex = NULL_INDEX;
					}
					else
					{
						state.mut()._entityOrder.numberOfShares -= input.numberOfShares;
						if (state.get()._entityOrder.numberOfShares > 0)
						{
							state.mut()._entityOrders.replace(state.get()._elementIndex, state.get()._entityOrder);
						}
						else
						{
							state.mut()._entityOrders.remove(state.get()._elementIndex);
						}

						state.mut()._elementIndex = state.get()._assetOrders.headIndex(state.get()._issuerAndAssetName, -input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state.mut()._assetOrder = state.get()._assetOrders.element(state.get()._elementIndex);
							if (state.get()._assetOrder.entity == qpi.invocator())
							{
								state.mut()._assetOrder.numberOfShares -= input.numberOfShares;
								if (state.get()._assetOrder.numberOfShares > 0)
								{
									state.mut()._assetOrders.replace(state.get()._elementIndex, state.get()._assetOrder);
								}
								else
								{
									state.mut()._assetOrders.remove(state.get()._elementIndex);
								}

								break;
							}

							state.mut()._elementIndex = state.get()._assetOrders.nextElementIndex(state.get()._elementIndex);
						}
					}

					break;
				}

				state.mut()._elementIndex = state.get()._entityOrders.nextElementIndex(state.get()._elementIndex);
			}

			if (state.get()._elementIndex == NULL_INDEX) // No other ask orders for the same asset at the same price found
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
			|| input.numberOfShares <= 0 || input.numberOfShares >= MAX_AMOUNT
			|| smul(input.price, input.numberOfShares) >= MAX_AMOUNT)
		{
			output.removedNumberOfShares = 0;
		}
		else
		{
			state.mut()._issuerAndAssetName = input.issuer;
			state.mut()._issuerAndAssetName.u64._3 = input.assetName;

			state.mut()._elementIndex = state.get()._entityOrders.tailIndex(qpi.invocator(), input.price);
			while (state.get()._elementIndex != NULL_INDEX)
			{
				if (state.get()._entityOrders.priority(state.get()._elementIndex) != input.price)
				{
					state.mut()._elementIndex = NULL_INDEX;

					break;
				}

				state.mut()._entityOrder = state.get()._entityOrders.element(state.get()._elementIndex);
				if (state.get()._entityOrder.assetName == input.assetName
					&& state.get()._entityOrder.issuer == input.issuer)
				{
					if (state.get()._entityOrder.numberOfShares < input.numberOfShares)
					{
						state.mut()._elementIndex = NULL_INDEX;
					}
					else
					{
						state.mut()._entityOrder.numberOfShares -= input.numberOfShares;
						if (state.get()._entityOrder.numberOfShares > 0)
						{
							state.mut()._entityOrders.replace(state.get()._elementIndex, state.get()._entityOrder);
						}
						else
						{
							state.mut()._entityOrders.remove(state.get()._elementIndex);
						}

						state.mut()._elementIndex = state.get()._assetOrders.tailIndex(state.get()._issuerAndAssetName, input.price);
						while (true) // Impossible for the corresponding asset order to not exist
						{
							state.mut()._assetOrder = state.get()._assetOrders.element(state.get()._elementIndex);
							if (state.get()._assetOrder.entity == qpi.invocator())
							{
								state.mut()._assetOrder.numberOfShares -= input.numberOfShares;
								if (state.get()._assetOrder.numberOfShares > 0)
								{
									state.mut()._assetOrders.replace(state.get()._elementIndex, state.get()._assetOrder);
								}
								else
								{
									state.mut()._assetOrders.remove(state.get()._elementIndex);
								}

								break;
							}

							state.mut()._elementIndex = state.get()._assetOrders.prevElementIndex(state.get()._elementIndex);
						}
					}

					break;
				}

				state.mut()._elementIndex = state.get()._entityOrders.prevElementIndex(state.get()._elementIndex);
			}

			if (state.get()._elementIndex == NULL_INDEX) // No other bid orders for the same asset at the same price found
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

		state.mut()._numberOfReservedShares_input.issuer = input.asset.issuer;
		state.mut()._numberOfReservedShares_input.assetName = input.asset.assetName;
		CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state.get()._numberOfReservedShares_output.numberOfShares < input.numberOfShares)
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

		state.mut()._assetIssuanceFee = 1000000000;

		/* Old values before epoch 138
		state.mut()._transferFee = 1000000;
		state.mut()._tradeFee = 5000000; // 0.5%
		*/

		// New values since epoch 138
		state.mut()._transferFee = 100;
		state.mut()._tradeFee = 3000000; // 0.3%
	}

	END_TICK()
	{
		if ((div((state.get()._earnedAmount - state.get()._distributedAmount), 676ULL) > 0) && (state.get()._earnedAmount > state.get()._distributedAmount))
		{
			if (qpi.distributeDividends(div((state.get()._earnedAmount - state.get()._distributedAmount), 676ULL)))
			{
				state.mut()._distributedAmount += div((state.get()._earnedAmount - state.get()._distributedAmount), 676ULL) * NUMBER_OF_COMPUTORS;
			}
		}

		// Cleanup collections if more than 30% of hash maps are marked for removal
		if (state.get()._assetOrders.needsCleanup(30)) { state.mut()._assetOrders.cleanup(); }
		if (state.get()._entityOrders.needsCleanup(30)) { state.mut()._entityOrders.cleanup(); }
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
		output.requestedFee = state.get()._transferFee;
		output.allowTransfer = true;
	}

	POST_ACQUIRE_SHARES()
	{
	}

	POST_INCOMING_TRANSFER()
	{
		switch (input.type)
		{
		case TransferType::standardTransaction:
			qpi.transfer(input.sourceId, input.amount);
			break;
		case TransferType::qpiTransfer:
		case TransferType::revenueDonation:
			// add amount to _earnedAmount which will be distributed to shareholders in END_TICK
			state.mut()._earnedAmount += input.amount;
			break;
		default:
			break;
		}
	}
};
