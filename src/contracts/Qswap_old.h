using namespace QPI;

// Log types enum for QSWAP contract
enum QSWAPLogInfo {
    QSWAPAddLiquidity = 4,
    QSWAPRemoveLiquidity = 5,
    QSWAPSwapExactQuForAsset = 6,
    QSWAPSwapQuForExactAsset = 7,
    QSWAPSwapExactAssetForQu = 8,
    QSWAPSwapAssetForExactQu = 9
};

// FIXED CONSTANTS
constexpr uint64 QSWAP_INITIAL_MAX_POOL = 16384;
constexpr uint64 QSWAP_MAX_POOL = QSWAP_INITIAL_MAX_POOL * X_MULTIPLIER;
constexpr uint64 QSWAP_MAX_USER_PER_POOL = 256;
constexpr sint64 QSWAP_MIN_LIQUIDITY = 1000;
constexpr uint32 QSWAP_SWAP_FEE_BASE = 10000;
constexpr uint32 QSWAP_FEE_BASE_100 = 100;

struct QSWAP2 
{
};

// Logging message structures for QSWAP procedures
struct QSWAPAddLiquidityMessage
{
    uint32 _contractIndex;
    uint32 _type;
    id assetIssuer;
    uint64 assetName;
    sint64 userIncreaseLiquidity;
    sint64 quAmount;
    sint64 assetAmount;
    sint8 _terminator;
};

struct QSWAPRemoveLiquidityMessage
{
    uint32 _contractIndex;
    uint32 _type;
    sint64 quAmount;
    sint64 assetAmount;
    sint8 _terminator;
};

struct QSWAPSwapMessage
{
    uint32 _contractIndex;
    uint32 _type;
    id assetIssuer;
    uint64 assetName;
    sint64 assetAmountIn;
    sint64 assetAmountOut;
    sint8 _terminator;
};

struct QSWAP : public ContractBase
{
public:
	struct Fees_input
	{
	};
	struct Fees_output
	{
		uint32 assetIssuanceFee; 	// Amount of qus
		uint32 poolCreationFee; 	// Amount of qus
		uint32 transferFee; 		// Amount of qus

		uint32 swapFee; 			// 30 -> 0.3%
		uint32 protocolFee;			// 20 -> 20%, for ipo share holders
		uint32 teamFee;				// 20 -> 20%, for dev team 
	};

	struct TeamInfo_input
	{
	};
	struct TeamInfo_output
	{
		uint32 teamFee;				// 20 -> 20%
		id teamId;
	};

	struct SetTeamInfo_input
	{
		id newTeamId;
	};
	struct SetTeamInfo_output
	{
		bool success;
	};

	struct GetPoolBasicState_input
	{
		id assetIssuer;
		uint64 assetName;
	};
	struct GetPoolBasicState_output
	{
		sint64 poolExists;
		sint64 reservedQuAmount;
		sint64 reservedAssetAmount;
		sint64 totalLiquidity;
	};

	struct GetLiquidityOf_input
	{
		id assetIssuer;
		uint64 assetName;
		id account;
	};
	struct GetLiquidityOf_output
	{
		sint64 liquidity;
	};

	struct QuoteExactQuInput_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 quAmountIn;
	};
	struct QuoteExactQuInput_output
	{
		sint64 assetAmountOut;
	};

	struct QuoteExactQuOutput_input{
		id assetIssuer;
		uint64 assetName;
		sint64 quAmountOut;
	};
	struct QuoteExactQuOutput_output
	{
		sint64 assetAmountIn;
	};

	struct QuoteExactAssetInput_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountIn;
	};
	struct QuoteExactAssetInput_output
	{
		sint64 quAmountOut;
	};

	struct QuoteExactAssetOutput_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountOut;
	};
	struct QuoteExactAssetOutput_output
	{
		sint64 quAmountIn;
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

	struct CreatePool_input
	{
		id assetIssuer;
		uint64 assetName;
	};
	struct CreatePool_output 
	{
		bool success;
	};

	struct TransferShareOwnershipAndPossession_input
	{
		id assetIssuer;
		uint64 assetName;
		id newOwnerAndPossessor;
		sint64 amount;
	};
	struct TransferShareOwnershipAndPossession_output
	{
		sint64 transferredAmount;
	};

	/** 
	* @param	quAmountADesired		The amount of tokenA to add as liquidity if the B/A price is <= amountBDesired/amountADesired (A depreciates).
	* @param	assetAmountBDesired		The amount of tokenB to add as liquidity if the A/B price is <= amountADesired/amountBDesired (B depreciates).
	* @param	quAmountMin				Bounds the extent to which the B/A price can go up before the transaction reverts. Must be <= amountADesired.
	* @param	assetAmountMin			Bounds the extent to which the A/B price can go up before the transaction reverts. Must be <= amountBDesired.
	*/
	struct AddLiquidity_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountDesired;
		sint64 quAmountMin;
		sint64 assetAmountMin;
	};
	struct AddLiquidity_output
	{
		sint64 userIncreaseLiquidity;
		sint64 quAmount;
		sint64 assetAmount;
	};

	struct RemoveLiquidity_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 burnLiquidity;
		sint64 quAmountMin;
		sint64 assetAmountMin;
	};

	struct RemoveLiquidity_output 
	{
		sint64 quAmount;
		sint64 assetAmount;
	};

	struct SwapExactQuForAsset_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountOutMin;
	};
	struct SwapExactQuForAsset_output
	{
		sint64 assetAmountOut;
	};

	struct SwapQuForExactAsset_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountOut;
	};
	struct SwapQuForExactAsset_output
	{
		sint64 quAmountIn;
	};

	struct SwapExactAssetForQu_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountIn;
		sint64 quAmountOutMin;
	};
	struct SwapExactAssetForQu_output
	{
		sint64 quAmountOut;
	};

	struct SwapAssetForExactQu_input
	{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountInMax;
		sint64 quAmountOut;
	};
	struct SwapAssetForExactQu_output
	{
		sint64 assetAmountIn;
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
	uint32 swapFeeRate; 		// e.g. 30: 0.3% (base: 10_000)
	uint32 teamFeeRate;			// e.g. 20: 20% (base: 100)
	uint32 protocolFeeRate; 	// e.g. 20: 20% (base: 100) only charge in qu
	uint32 poolCreationFeeRate;	// e.g. 10: 10% (base: 100)

	id teamId;
	uint64 teamEarnedFee;
	uint64 teamDistributedAmount;

	uint64 protocolEarnedFee;
	uint64 protocolDistributedAmount;

	struct PoolBasicState
	{
		id poolID;
		sint64 reservedQuAmount;
		sint64 reservedAssetAmount;
		sint64 totalLiquidity;
	};

	struct LiquidityInfo 
	{
		id entity;
		sint64 liquidity;
	};

	Array<PoolBasicState, QSWAP_MAX_POOL> mPoolBasicStates;
	Collection<LiquidityInfo, QSWAP_MAX_POOL * QSWAP_MAX_USER_PER_POOL> mLiquidities;

	inline static sint64 min(sint64 a, sint64 b) 
	{
		return (a < b) ? a : b;
	}

	// find the sqrt of a*b
	inline static sint64 sqrt(sint64& a, sint64& b, uint128& prod, uint128& y, uint128& z) 
	{
		if (a == b)
		{
			return a;
		}

		prod = uint128(a) * uint128(b);

		// (prod + 1) / 2;
		z = div(prod+uint128(1), uint128(2));
		y = prod;

		while(z < y)
		{
			y = z;
			// (prod / z + z) / 2;
			z = div((div(prod, z) + z), uint128(2));
		}

		return sint64(y.low);
	}

	inline static sint64 quoteEquivalentAmountB(sint64& amountADesired, sint64& reserveA, sint64& reserveB, uint128& tmpRes)
	{
		// amountDesired * reserveB / reserveA
		tmpRes = div(uint128(amountADesired) * uint128(reserveB), uint128(reserveA));

		if ((tmpRes.high != 0)|| (tmpRes.low > 0x7FFFFFFFFFFFFFFF))
		{
			return -1;
		}
		else
		{
			return sint64(tmpRes.low);
		}
	}

	// reserveIn * reserveOut = (reserveIn + amountIn * (1-fee)) * (reserveOut - x)
	// x = reserveOut * amountIn * (1-fee) / (reserveIn + amountIn * (1-fee))
	inline static sint64 getAmountOutTakeFeeFromInToken(
		sint64& amountIn,
		sint64& reserveIn,
		sint64& reserveOut,
		uint32 fee,
		uint128& amountInWithFee,
		uint128& numerator,
		uint128& denominator,
		uint128& tmpRes
	)
	{
		amountInWithFee = uint128(amountIn) * uint128(QSWAP_SWAP_FEE_BASE - fee);
		numerator = uint128(reserveOut) * amountInWithFee;
		denominator = uint128(reserveIn) * uint128(QSWAP_SWAP_FEE_BASE) + amountInWithFee;

		// numerator / denominator
		tmpRes = div(numerator, denominator);
		if ((tmpRes.high != 0) || (tmpRes.low > 0x7FFFFFFFFFFFFFFF))
		{
			return -1;
		}
		else
		{
			return sint64(tmpRes.low);
		}
	}

	// reserveIn * reserveOut = (reserveIn + x * (1-fee)) * (reserveOut - amountOut)
	// x = (reserveIn * amountOut)/((1-fee) * (reserveOut - amountOut)
	inline static sint64 getAmountInTakeFeeFromInToken(sint64& amountOut, sint64& reserveIn, sint64& reserveOut, uint32 fee, uint128& tmpRes)
	{
		// reserveIn*amountOut/(reserveOut - amountOut)*QSWAP_SWAP_FEE_BASE / (QSWAP_SWAP_FEE_BASE - fee)
		tmpRes = div(
			div(
				uint128(reserveIn) * uint128(amountOut),
				uint128(reserveOut - amountOut)
			) * uint128(QSWAP_SWAP_FEE_BASE),
			uint128(QSWAP_SWAP_FEE_BASE - fee)
		);
		if ((tmpRes.high != 0) || (tmpRes.low > 0x7FFFFFFFFFFFFFFF))
		{
			return -1;
		}
		else
		{
			return sint64(tmpRes.low);
		}
	}

	// (reserveIn + amountIn) * (reserveOut - x) = reserveIn * reserveOut
	// x = reserveOut * amountIn / (reserveIn + amountIn)
	inline static sint64 getAmountOutTakeFeeFromOutToken(sint64& amountIn, sint64& reserveIn, sint64& reserveOut, uint32 fee, uint128& numerator, uint128& denominator, uint128& tmpRes)
	{
		numerator = uint128(reserveOut) * uint128(amountIn);
		denominator = uint128(reserveIn + amountIn);

		tmpRes = div(numerator, denominator);
		if ((tmpRes.high != 0)|| (tmpRes.low > 0x7FFFFFFFFFFFFFFF))
		{
			return -1;
		}
		else
		{
			return sint64(tmpRes.low);
		}
	}

	// (reserveIn + x) * (reserveOut - amountOut/(1 - fee)) = reserveIn * reserveOut
	// x = (reserveIn * amountOut ) / (reserveOut * (1-fee) - amountOut)
	inline static sint64 getAmountInTakeFeeFromOutToken(sint64& amountOut, sint64& reserveIn, sint64& reserveOut, uint32 fee, uint128& numerator, uint128& denominator, uint128& tmpRes)
	{
		numerator = uint128(reserveIn) * uint128(amountOut);
		if (div(uint128(reserveOut) * uint128(QSWAP_SWAP_FEE_BASE - fee), uint128(QSWAP_SWAP_FEE_BASE)) < uint128(amountOut))
		{
			return -1;
		}
		denominator = div(uint128(reserveOut) * uint128(QSWAP_SWAP_FEE_BASE - fee), uint128(QSWAP_SWAP_FEE_BASE)) - uint128(amountOut);

		tmpRes = div(numerator, denominator);
		if ((tmpRes.high != 0)|| (tmpRes.low > 0x7FFFFFFFFFFFFFFF))
		{
			return -1;
		}
		else
		{
			return sint64(tmpRes.low);
		}
	}

	struct Fees_locals
	{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(Fees)
	{

		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

		output.assetIssuanceFee = locals.feesOutput.assetIssuanceFee;
		output.poolCreationFee = uint32(div(uint64(locals.feesOutput.assetIssuanceFee) * uint64(state.poolCreationFeeRate), uint64(QSWAP_FEE_BASE_100)));
		output.transferFee = locals.feesOutput.transferFee;
		output.swapFee = state.swapFeeRate;
		output.teamFee = state.teamFeeRate;
		output.protocolFee = state.protocolFeeRate;
	}

	struct GetPoolBasicState_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		uint32 i0;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetPoolBasicState)
	{
		output.poolExists = 0;
		output.totalLiquidity = -1;
		output.reservedAssetAmount = -1;
		output.reservedQuAmount = -1;

		// asset not issued
		if (!qpi.isAssetIssued(input.assetIssuer, input.assetName))
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = NULL_INDEX;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == NULL_INDEX)
		{
			return;
		}

		output.poolExists = 1;

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		output.reservedQuAmount = locals.poolBasicState.reservedQuAmount;
		output.reservedAssetAmount = locals.poolBasicState.reservedAssetAmount;
		output.totalLiquidity = locals.poolBasicState.totalLiquidity;
	}

	struct GetLiquidityOf_locals
	{
		id poolID;
		sint64 liqElementIndex;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetLiquidityOf)
	{
		output.liquidity = 0;

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.liqElementIndex = state.mLiquidities.headIndex(locals.poolID, 0);

		while (locals.liqElementIndex != NULL_INDEX)
		{
			if (state.mLiquidities.element(locals.liqElementIndex).entity == input.account)
			{
				output.liquidity = state.mLiquidities.element(locals.liqElementIndex).liquidity;
				return;
			}
			locals.liqElementIndex = state.mLiquidities.nextElementIndex(locals.liqElementIndex);
		}
	}

	struct QuoteExactQuInput_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint32 i0;
		uint128 i1, i2, i3, i4;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactQuInput)
	{
		output.assetAmountOut = -1;

		if (input.quAmountIn <= 0) 
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) 
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) 
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1)
		{
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liquidity in the pool
		if (locals.poolBasicState.totalLiquidity == 0) 
		{
			return;
		}

		output.assetAmountOut = getAmountOutTakeFeeFromInToken(
			input.quAmountIn,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3,
			locals.i4
		);
	}

	struct QuoteExactQuOutput_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint32 i0;
		uint128 i1, i2, i3;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactQuOutput)
	{
		output.assetAmountIn = -1;

		if (input.quAmountOut <= 0)
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1)
		{
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liquidity in the pool
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			return;
		}

		if (input.quAmountOut >= locals.poolBasicState.reservedQuAmount)
		{
			return;
		}

		output.assetAmountIn = getAmountInTakeFeeFromOutToken(
			input.quAmountOut,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);
	}

	struct QuoteExactAssetInput_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountOutWithFee;

		uint32 i0;
		uint128 i1, i2, i3;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactAssetInput)
	{
		output.quAmountOut = -1;

		if (input.assetAmountIn <= 0)
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1)
		{
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liquidity in the pool
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			return;
		}

		locals.quAmountOutWithFee = getAmountOutTakeFeeFromOutToken(
			input.assetAmountIn,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);

		// above call overflow
		if (locals.quAmountOutWithFee == -1)
		{
			return;
		}

		// amount * (1-fee), no overflow risk
		output.quAmountOut = sint64(div(
			uint128(locals.quAmountOutWithFee) * uint128(QSWAP_SWAP_FEE_BASE - state.swapFeeRate), 
			uint128(QSWAP_SWAP_FEE_BASE)
		).low);
	}

	struct QuoteExactAssetOutput_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint32 i0;
		uint128 i1;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactAssetOutput)
	{
		output.quAmountIn = -1;

		if (input.assetAmountOut <= 0)
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1)
		{
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liquidity in the pool
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			return;
		}

		if (input.assetAmountOut >= locals.poolBasicState.reservedAssetAmount)
		{
			return;
		}

		output.quAmountIn = getAmountInTakeFeeFromInToken(
			input.assetAmountOut,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate,
			locals.i1
		);
	}

	PUBLIC_FUNCTION(TeamInfo)
	{
		output.teamId = state.teamId;
		output.teamFee = state.teamFeeRate;
	}

//
// procedure
// 
	struct IssueAsset_locals
	{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(IssueAsset)
	{
		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

		output.issuedNumberOfShares = 0;
		if ((qpi.invocationReward() < locals.feesOutput.assetIssuanceFee)) 
		{
			if (qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// check the validity of input
		if ((input.numberOfShares <= 0) || (input.numberOfDecimalPlaces < 0))
		{
			if (qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// asset already issued 
		if (qpi.isAssetIssued(qpi.invocator(), input.assetName))
		{
			if (qpi.invocationReward() > 0 )
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		output.issuedNumberOfShares = qpi.issueAsset(
			input.assetName,
			qpi.invocator(),
			input.numberOfDecimalPlaces,
			input.numberOfShares,
			input.unitOfMeasurement
		);

		if (output.issuedNumberOfShares == 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		else if (qpi.invocationReward() > locals.feesOutput.assetIssuanceFee )
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.feesOutput.assetIssuanceFee);
			state.protocolEarnedFee += locals.feesOutput.assetIssuanceFee;
		}
	}

	struct CreatePool_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
		uint32 poolCreationFee;

		uint32 i0, i1;
	};

	// create uniswap like pool
	// TODO: reject if there is no shares avaliabe shares in current contract, e.g. asset is issue in contract qx
	PUBLIC_PROCEDURE_WITH_LOCALS(CreatePool)
	{
		output.success = false;

		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);
		locals.poolCreationFee = uint32(div(uint64(locals.feesOutput.assetIssuanceFee) * uint64(state.poolCreationFeeRate), uint64(QSWAP_FEE_BASE_100)));

		// fee check
		if(qpi.invocationReward() < locals.poolCreationFee )
		{
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// asset no exist
		if (!qpi.isAssetIssued(qpi.invocator(), input.assetName))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		// check if pool already exist
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL ; locals.i0 ++ )
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}
		}

		// find an vacant pool slot
		locals.poolSlot = -1;
		for (locals.i1 = 0; locals.i1 < QSWAP_MAX_POOL; locals.i1 ++)
		{
			if (state.mPoolBasicStates.get(locals.i1).poolID == id(0,0,0,0))
			{
				locals.poolSlot = locals.i1;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState.poolID = locals.poolID;
		locals.poolBasicState.reservedAssetAmount = 0;
		locals.poolBasicState.reservedQuAmount = 0;
		locals.poolBasicState.totalLiquidity = 0;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		if(qpi.invocationReward() > locals.poolCreationFee)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.poolCreationFee );
		}
		state.protocolEarnedFee += locals.poolCreationFee;

		output.success = true;
	}


	struct AddLiquidity_locals
	{
		QSWAPAddLiquidityMessage addLiquidityMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		LiquidityInfo tmpLiquidity;

		sint64 userLiquidityElementIndex;
		sint64 quAmountDesired;

		sint64 quTransferAmount;
		sint64 assetTransferAmount;
		sint64 quOptimalAmount;
		sint64 assetOptimalAmount;
		sint64 increaseLiquidity;
		sint64 reservedAssetAmountBefore;
		sint64 reservedAssetAmountAfter;

		uint128 tmpIncLiq0;
		uint128 tmpIncLiq1;

		uint32 i0;
		uint128 i1, i2, i3;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(AddLiquidity)
	{
		output.userIncreaseLiquidity = 0;
		output.assetAmount = 0;
		output.quAmount = 0;

		// add liquidity must stake both qu and asset
		if (qpi.invocationReward() <= 0)
		{
			return;
		}

		locals.quAmountDesired = qpi.invocationReward();

		// check the vadility of input params
		if ((input.assetAmountDesired <= 0) ||
			(input.quAmountMin < 0) ||
			(input.assetAmountMin < 0))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		// check the pool existance
		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check if pool state meet the input condition before desposit 
		// and confirm the final qu and asset amount to stake
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			locals.quTransferAmount = locals.quAmountDesired;
			locals.assetTransferAmount = input.assetAmountDesired;
		}
		else
		{
			locals.assetOptimalAmount = quoteEquivalentAmountB(
				locals.quAmountDesired,
				locals.poolBasicState.reservedQuAmount,
				locals.poolBasicState.reservedAssetAmount,
				locals.i1
			);
			// overflow
			if (locals.assetOptimalAmount == -1)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return ;
			}

			if (locals.assetOptimalAmount <= input.assetAmountDesired )
			{
				if (locals.assetOptimalAmount < input.assetAmountMin)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				locals.quTransferAmount = locals.quAmountDesired;
				locals.assetTransferAmount = locals.assetOptimalAmount;
			}
			else
			{
				locals.quOptimalAmount = quoteEquivalentAmountB(
					input.assetAmountDesired,
					locals.poolBasicState.reservedAssetAmount,
					locals.poolBasicState.reservedQuAmount,
					locals.i1
				);
				// overflow
				if (locals.quOptimalAmount == -1)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				if (locals.quOptimalAmount > locals.quAmountDesired)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				if (locals.quOptimalAmount < input.quAmountMin)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				locals.quTransferAmount = locals.quOptimalAmount;
				locals.assetTransferAmount = input.assetAmountDesired;
			}
		}

		// check if the qu is enough
		if (qpi.invocationReward() < locals.quTransferAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// check if the asset is enough
		if (qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				SELF_INDEX,
				SELF_INDEX
			) < locals.assetTransferAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// for pool's initial mint 
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			locals.increaseLiquidity = sqrt(locals.quTransferAmount, locals.assetTransferAmount, locals.i1, locals.i2, locals.i3);

			if (locals.increaseLiquidity < QSWAP_MIN_LIQUIDITY )
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			locals.reservedAssetAmountBefore = qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				SELF,
				SELF,
				SELF_INDEX,
				SELF_INDEX
			);
			qpi.transferShareOwnershipAndPossession(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				locals.assetTransferAmount,
				SELF
			);
			locals.reservedAssetAmountAfter = qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				SELF,
				SELF,
				SELF_INDEX,
				SELF_INDEX
			);

			if (locals.reservedAssetAmountAfter - locals.reservedAssetAmountBefore < locals.assetTransferAmount)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			// permanently lock the first MINIMUM_LIQUIDITY tokens
			locals.tmpLiquidity.entity = SELF;
			locals.tmpLiquidity.liquidity = QSWAP_MIN_LIQUIDITY;
			state.mLiquidities.add(locals.poolID, locals.tmpLiquidity, 0);

			locals.tmpLiquidity.entity = qpi.invocator();
			locals.tmpLiquidity.liquidity = locals.increaseLiquidity - QSWAP_MIN_LIQUIDITY;
			state.mLiquidities.add(locals.poolID, locals.tmpLiquidity, 0);

			output.quAmount = locals.quTransferAmount;
			output.assetAmount = locals.assetTransferAmount;
			output.userIncreaseLiquidity = locals.increaseLiquidity - QSWAP_MIN_LIQUIDITY;
		}
		else
		{
			locals.tmpIncLiq0 = div(
				uint128(locals.quTransferAmount) * uint128(locals.poolBasicState.totalLiquidity),
				uint128(locals.poolBasicState.reservedQuAmount)
			);
			if (locals.tmpIncLiq0.high != 0 || locals.tmpIncLiq0.low > 0x7FFFFFFFFFFFFFFF)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}
			locals.tmpIncLiq1 = div(
				uint128(locals.assetTransferAmount) * uint128(locals.poolBasicState.totalLiquidity),
				uint128(locals.poolBasicState.reservedAssetAmount)
			);
			if (locals.tmpIncLiq1.high != 0 || locals.tmpIncLiq1.low > 0x7FFFFFFFFFFFFFFF)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			// increaseLiquity = min(
			// 	quTransferAmount * totalLiquity / reserveQuAmount,
			// 	assetTransferAmount * totalLiquity / reserveAssetAmount
			// );
			locals.increaseLiquidity = min(sint64(locals.tmpIncLiq0.low), sint64(locals.tmpIncLiq1.low));

			// maybe too little input 
			if (locals.increaseLiquidity == 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			// find user liquidity index
			locals.userLiquidityElementIndex = state.mLiquidities.headIndex(locals.poolID, 0);
			while (locals.userLiquidityElementIndex != NULL_INDEX)
			{
				if(state.mLiquidities.element(locals.userLiquidityElementIndex).entity == qpi.invocator())
				{
					break;
				}

				locals.userLiquidityElementIndex = state.mLiquidities.nextElementIndex(locals.userLiquidityElementIndex);
			}

			// no more space for new liquidity item
			if ((locals.userLiquidityElementIndex == NULL_INDEX) && ( state.mLiquidities.population() == state.mLiquidities.capacity()))
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			// transfer the asset from invocator to contract
			locals.reservedAssetAmountBefore = qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				SELF,
				SELF,
				SELF_INDEX,
				SELF_INDEX
			);
			qpi.transferShareOwnershipAndPossession(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				locals.assetTransferAmount,
				SELF
			);
			locals.reservedAssetAmountAfter = qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				SELF,
				SELF,
				SELF_INDEX,
				SELF_INDEX
			);

			// only trust the amount in the contract
			if (locals.reservedAssetAmountAfter - locals.reservedAssetAmountBefore < locals.assetTransferAmount)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			if (locals.userLiquidityElementIndex == NULL_INDEX)
			{
				locals.tmpLiquidity.entity = qpi.invocator();
				locals.tmpLiquidity.liquidity = locals.increaseLiquidity;
				state.mLiquidities.add(locals.poolID, locals.tmpLiquidity, 0);
			}
			else
			{
				locals.tmpLiquidity = state.mLiquidities.element(locals.userLiquidityElementIndex);
				locals.tmpLiquidity.liquidity += locals.increaseLiquidity;
				state.mLiquidities.replace(locals.userLiquidityElementIndex, locals.tmpLiquidity);
			}

			output.quAmount = locals.quTransferAmount;
			output.assetAmount = locals.assetTransferAmount;
			output.userIncreaseLiquidity = locals.increaseLiquidity;
		}

		locals.poolBasicState.reservedQuAmount += locals.quTransferAmount;
		locals.poolBasicState.reservedAssetAmount += locals.assetTransferAmount;
		locals.poolBasicState.totalLiquidity += locals.increaseLiquidity;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log AddLiquidity procedure
		locals.addLiquidityMessage._contractIndex = SELF_INDEX;
		locals.addLiquidityMessage._type = QSWAPAddLiquidity;
		locals.addLiquidityMessage.assetIssuer = input.assetIssuer;
		locals.addLiquidityMessage.assetName = input.assetName;
		locals.addLiquidityMessage.userIncreaseLiquidity = output.userIncreaseLiquidity;
		locals.addLiquidityMessage.quAmount = output.quAmount;
		locals.addLiquidityMessage.assetAmount = output.assetAmount;
		LOG_INFO(locals.addLiquidityMessage);

		if (qpi.invocationReward() > locals.quTransferAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.quTransferAmount);
		}
	}

	struct RemoveLiquidity_locals
	{
		QSWAPRemoveLiquidityMessage removeLiquidityMessage;
		id poolID;
		PoolBasicState poolBasicState;
		sint64 userLiquidityElementIndex;
		sint64 poolSlot;
		LiquidityInfo userLiquidity;
		sint64 burnQuAmount;
		sint64 burnAssetAmount;

		uint32 i0;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(RemoveLiquidity)
	{
		output.quAmount = 0;
		output.assetAmount = 0;

		if (qpi.invocationReward() > 0 )
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		// check the vadility of input params
		if (input.quAmountMin < 0 || input.assetAmountMin < 0)
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		// get the pool's basic state
		locals.poolSlot = -1;

		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// the pool does not exsit
		if (locals.poolSlot == -1)
		{
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		locals.userLiquidityElementIndex = state.mLiquidities.headIndex(locals.poolID, 0);
		while (locals.userLiquidityElementIndex != NULL_INDEX)
		{
			if(state.mLiquidities.element(locals.userLiquidityElementIndex).entity == qpi.invocator())
			{
				break;
			}

			locals.userLiquidityElementIndex = state.mLiquidities.nextElementIndex(locals.userLiquidityElementIndex);
		}

		if (locals.userLiquidityElementIndex == NULL_INDEX)
		{
			return;
		}

		locals.userLiquidity = state.mLiquidities.element(locals.userLiquidityElementIndex);

		// not enough liquidity for burning
		if (locals.userLiquidity.liquidity < input.burnLiquidity )
		{
			return;
		}

		if (locals.poolBasicState.totalLiquidity < input.burnLiquidity )
		{
			return;
		}

		// since burnLiquidity < totalLiquidity, so there will be no overflow risk
		locals.burnQuAmount = sint64(div(
				uint128(input.burnLiquidity) * uint128(locals.poolBasicState.reservedQuAmount),
				uint128(locals.poolBasicState.totalLiquidity)
			).low);

		// since burnLiquidity < totalLiquidity, so there will be no overflow risk
		locals.burnAssetAmount = sint64(div(
				uint128(input.burnLiquidity) * uint128(locals.poolBasicState.reservedAssetAmount),
				uint128(locals.poolBasicState.totalLiquidity)
			).low);


		if ((locals.burnQuAmount < input.quAmountMin) || (locals.burnAssetAmount < input.assetAmountMin))
		{
			return;
		}

		// return qu and asset to invocator
		qpi.transfer(qpi.invocator(), locals.burnQuAmount);
		qpi.transferShareOwnershipAndPossession(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			locals.burnAssetAmount,
			qpi.invocator()
		);

		output.quAmount = locals.burnQuAmount;
		output.assetAmount = locals.burnAssetAmount;

		// modify invocator's liquidity info
		locals.userLiquidity.liquidity -= input.burnLiquidity;
		if (locals.userLiquidity.liquidity == 0)
		{
			state.mLiquidities.remove(locals.userLiquidityElementIndex);
		}
		else
		{
			state.mLiquidities.replace(locals.userLiquidityElementIndex, locals.userLiquidity);
		}

		// modify the pool's liquidity info
		locals.poolBasicState.totalLiquidity -= input.burnLiquidity;
		locals.poolBasicState.reservedQuAmount -= locals.burnQuAmount;
		locals.poolBasicState.reservedAssetAmount -= locals.burnAssetAmount;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log RemoveLiquidity procedure
		locals.removeLiquidityMessage._contractIndex = SELF_INDEX;
		locals.removeLiquidityMessage._type = QSWAPRemoveLiquidity;
		locals.removeLiquidityMessage.quAmount = output.quAmount;
		locals.removeLiquidityMessage.assetAmount = output.assetAmount;
		LOG_INFO(locals.removeLiquidityMessage);
	}

	struct SwapExactQuForAsset_locals
	{
		QSWAPSwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		sint64 quAmountIn;
		PoolBasicState poolBasicState;
		sint64 assetAmountOut;

		uint32 i0;
		uint128 i1, i2, i3, i4;
		uint128 swapFee;
		uint128 feeToTeam;
		uint128 feeToProtocol;
	};

	// given an input qu amountIn, only execute swap in case (amountOut >= amountOutMin)
	// https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#swapexacttokensfortokens
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapExactQuForAsset)
	{
		output.assetAmountOut = 0;

		// require input qu > 0
		if (qpi.invocationReward() <= 0)
		{
			return;
		}

		if (input.assetAmountOutMin < 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.quAmountIn = qpi.invocationReward();

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check the liquidity validity 
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.assetAmountOut = getAmountOutTakeFeeFromInToken(
			locals.quAmountIn,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3,
			locals.i4
		);

		// overflow
		if (locals.assetAmountOut == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// not meet user's amountOut requirement
		if (locals.assetAmountOut < input.assetAmountOutMin)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// transfer the asset from pool to qpi.invocator()
		output.assetAmountOut = qpi.transferShareOwnershipAndPossession(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			locals.assetAmountOut,
			qpi.invocator()
		) < 0 ? 0: locals.assetAmountOut;

		// in case asset transfer failed
		if (output.assetAmountOut == 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// swapFee = quAmountIn * 0.3% (swapFeeRate/10000)
		// feeToTeam = swapFee * 20% (teamFeeRate/100)
		// feeToProtocol = (swapFee - feeToTeam) * 20% (protocolFeeRate/100)
		locals.swapFee = div(uint128(locals.quAmountIn)*uint128(state.swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE));
		locals.feeToTeam = div(locals.swapFee * uint128(state.teamFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToProtocol = div((locals.swapFee - locals.feeToTeam) * uint128(state.protocolFeeRate), uint128(QSWAP_FEE_BASE_100));

		state.teamEarnedFee += locals.feeToTeam.low;
		state.protocolEarnedFee += locals.feeToProtocol.low;

		locals.poolBasicState.reservedQuAmount += locals.quAmountIn - sint64(locals.feeToTeam.low) - sint64(locals.feeToProtocol.low);
		locals.poolBasicState.reservedAssetAmount -= locals.assetAmountOut;
		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log SwapExactQuForAsset procedure
		locals.swapMessage._contractIndex = SELF_INDEX;
		locals.swapMessage._type = QSWAPSwapExactQuForAsset;
		locals.swapMessage.assetIssuer = input.assetIssuer;
		locals.swapMessage.assetName = input.assetName;
		locals.swapMessage.assetAmountIn = locals.quAmountIn;
		locals.swapMessage.assetAmountOut = output.assetAmountOut;
		LOG_INFO(locals.swapMessage);
	}

	struct SwapQuForExactAsset_locals
	{
		QSWAPSwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountIn;
		sint64 transferredAssetAmount;

		uint32 i0;
		uint128 i1;
		uint128 swapFee;
		uint128 feeToTeam;
		uint128 feeToProtocol;
	};

	// https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#swaptokensforexacttokens
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapQuForExactAsset)
	{
		output.quAmountIn = 0;

		// require input qu amount > 0
		if (qpi.invocationReward() <= 0)
		{
			return;
		}

		// check input param validity
		if (input.assetAmountOut <= 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check if there is liquidity in the poool 
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// check if reserved asset is enough
		if (input.assetAmountOut >= locals.poolBasicState.reservedAssetAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.quAmountIn = getAmountInTakeFeeFromInToken(
			input.assetAmountOut,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate,
			locals.i1
		);

		// above call overflow
		if (locals.quAmountIn == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// not enough qu amountIn
		if (locals.quAmountIn > qpi.invocationReward()) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// not meet user's amountIn limit
		if (locals.quAmountIn > qpi.invocationReward())
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// transfer the asset from pool to qpi.invocator()
		locals.transferredAssetAmount = qpi.transferShareOwnershipAndPossession(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			input.assetAmountOut,
			qpi.invocator()
		) < 0 ? 0: input.assetAmountOut;

		// asset transfer failed
		if (locals.transferredAssetAmount == 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		output.quAmountIn = locals.quAmountIn;
		if (qpi.invocationReward() > locals.quAmountIn)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.quAmountIn);
		}

		// swapFee = quAmountIn * 0.3%
		// feeToTeam = swapFee * 20%
		// feeToProtocol = (swapFee - feeToTeam) * 20%
		locals.swapFee = div(uint128(locals.quAmountIn)*uint128(state.swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE));
		locals.feeToTeam = div(locals.swapFee * uint128(state.teamFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToProtocol = div((locals.swapFee - locals.feeToTeam) * uint128(state.protocolFeeRate), uint128(QSWAP_FEE_BASE_100));

		state.teamEarnedFee += locals.feeToTeam.low;
		state.protocolEarnedFee += locals.feeToProtocol.low;

		locals.poolBasicState.reservedQuAmount += locals.quAmountIn - sint64(locals.feeToTeam.low) - sint64(locals.feeToProtocol.low);
		locals.poolBasicState.reservedAssetAmount -= input.assetAmountOut;
		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log SwapQuForExactAsset procedure
		locals.swapMessage._contractIndex = SELF_INDEX;
		locals.swapMessage._type = QSWAPSwapQuForExactAsset;
		locals.swapMessage.assetIssuer = input.assetIssuer;
		locals.swapMessage.assetName = input.assetName;
		locals.swapMessage.assetAmountIn = output.quAmountIn;
		locals.swapMessage.assetAmountOut = input.assetAmountOut;
		LOG_INFO(locals.swapMessage);
	}

	struct SwapExactAssetForQu_locals
	{
		QSWAPSwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountOut;
		sint64 quAmountOutWithFee;
		sint64 transferredAssetAmountBefore;
		sint64 transferredAssetAmountAfter;

		uint32 i0;
		uint128 i1, i2, i3;
		uint128 swapFee;
		uint128 feeToTeam;
		uint128 feeToProtocol;
	};

	// given an amount of asset swap in, only execute swaping if quAmountOut >= input.amountOutMin
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapExactAssetForQu)
	{
		output.quAmountOut = 0;
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		// check input param validity
		if ((input.assetAmountIn <= 0 )||(input.quAmountOutMin < 0))
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0++)
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check the liquidity validity 
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			return;
		}

		// invocator's asset not enough 
		if (qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				SELF_INDEX,
				SELF_INDEX
			) < input.assetAmountIn )
		{
			return;
		}

		locals.quAmountOutWithFee = getAmountOutTakeFeeFromOutToken(
			input.assetAmountIn,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);

		// above call overflow
		if (locals.quAmountOutWithFee == -1)
		{
			return;
		}

		// no overflow risk
		// locals.quAmountOutWithFee * (QSWAP_SWAP_FEE_BASE - state.swapFeeRate) / QSWAP_SWAP_FEE_BASE
		locals.quAmountOut = sint64(div(
				uint128(locals.quAmountOutWithFee) * uint128(QSWAP_SWAP_FEE_BASE - state.swapFeeRate), 
				uint128(QSWAP_SWAP_FEE_BASE)
			).low);

		// not meet user min amountOut requirement
		if (locals.quAmountOut < input.quAmountOutMin)
		{
			return;
		}

		locals.transferredAssetAmountBefore = qpi.numberOfPossessedShares(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			SELF_INDEX,
			SELF_INDEX
		);
		qpi.transferShareOwnershipAndPossession(
			input.assetName,
			input.assetIssuer,
			qpi.invocator(),
			qpi.invocator(),
			input.assetAmountIn,
			SELF
		);
		locals.transferredAssetAmountAfter = qpi.numberOfPossessedShares(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			SELF_INDEX,
			SELF_INDEX
		);

		// pool does not receive enough asset
		if (locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore < input.assetAmountIn)
		{
			return;
		}

		qpi.transfer(qpi.invocator(), locals.quAmountOut);
		output.quAmountOut = locals.quAmountOut;

		// swapFee = quAmountOutWithFee * 0.3%
		// feeToTeam = swapFee * 20%
		// feeToProtocol = (swapFee - feeToTeam) * 20%
		locals.swapFee = div(uint128(locals.quAmountOutWithFee) * uint128(state.swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE));
		locals.feeToTeam = div(locals.swapFee * uint128(state.teamFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToProtocol = div((locals.swapFee - locals.feeToTeam) * uint128(state.protocolFeeRate), uint128(QSWAP_FEE_BASE_100));

		// update pool states
		locals.poolBasicState.reservedAssetAmount += input.assetAmountIn;
		locals.poolBasicState.reservedQuAmount -= locals.quAmountOut;
		locals.poolBasicState.reservedQuAmount -= sint64(locals.feeToTeam.low);
		locals.poolBasicState.reservedQuAmount -= sint64(locals.feeToProtocol.low);

		state.teamEarnedFee += locals.feeToTeam.low;
		state.protocolEarnedFee += locals.feeToProtocol.low;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log SwapExactAssetForQu procedure
		locals.swapMessage._contractIndex = SELF_INDEX;
		locals.swapMessage._type = QSWAPSwapExactAssetForQu;
		locals.swapMessage.assetIssuer = input.assetIssuer;
		locals.swapMessage.assetName = input.assetName;
		locals.swapMessage.assetAmountIn = input.assetAmountIn;
		locals.swapMessage.assetAmountOut = output.quAmountOut;
		LOG_INFO(locals.swapMessage);
	}

	struct SwapAssetForExactQu_locals
	{
		QSWAPSwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 assetAmountIn;
		sint64 transferredAssetAmountBefore;
		sint64 transferredAssetAmountAfter;

		uint32 i0;
		uint128 i1, i2, i3;
		uint128 swapFee;
		uint128 feeToTeam;
		uint128 feeToProtocol;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SwapAssetForExactQu)
	{
		output.assetAmountIn = 0;
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if ((input.assetAmountInMax <= 0 )||(input.quAmountOut <= 0))
		{
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) 
		{
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) 
			{
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check the liquidity validity 
		if (locals.poolBasicState.totalLiquidity == 0) 
		{
			return;
		}

		// pool does not hold enough asset
		if (input.quAmountOut >= locals.poolBasicState.reservedQuAmount)
		{
			return;
		}

		locals.assetAmountIn = getAmountInTakeFeeFromOutToken(
			input.quAmountOut,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);

		// invalid input, assetAmountIn overflow
		if (locals.assetAmountIn == -1)
		{
			return;
		}

		// user does not hold enough asset
		if (qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				SELF_INDEX,
				SELF_INDEX
			) < locals.assetAmountIn )
		{
			return;
		}

		// not meet user amountIn reqiurement 
		if (locals.assetAmountIn > input.assetAmountInMax)
		{
			return;
		}

		locals.transferredAssetAmountBefore = qpi.numberOfPossessedShares(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			SELF_INDEX,
			SELF_INDEX
		);
		qpi.transferShareOwnershipAndPossession(
			input.assetName,
			input.assetIssuer,
			qpi.invocator(),
			qpi.invocator(),
			locals.assetAmountIn,
			SELF
		);
		locals.transferredAssetAmountAfter = qpi.numberOfPossessedShares(
			input.assetName,
			input.assetIssuer,
			SELF,
			SELF,
			SELF_INDEX,
			SELF_INDEX
		);

		if (locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore < locals.assetAmountIn)
		{
			return;
		}

		qpi.transfer(qpi.invocator(), input.quAmountOut);
		output.assetAmountIn = locals.assetAmountIn;

		// swapFee = quAmountOut * 30/(10_000 - 30)
		// feeToTeam = swapFee * 20% (teamFeeRate/100)
		// feeToProtocol = (swapFee - feeToTeam) * 20% (protocolFeeRate/100)
		locals.swapFee = div(uint128(input.quAmountOut) * uint128(state.swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE - state.swapFeeRate));
		locals.feeToTeam = div(locals.swapFee * uint128(state.teamFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToProtocol = div((locals.swapFee - locals.feeToTeam) * uint128(state.protocolFeeRate), uint128(QSWAP_FEE_BASE_100));

		state.teamEarnedFee += locals.feeToTeam.low;
		state.protocolEarnedFee += locals.feeToProtocol.low;

		// update pool states
		locals.poolBasicState.reservedAssetAmount += locals.assetAmountIn;
		locals.poolBasicState.reservedQuAmount -= input.quAmountOut;
		locals.poolBasicState.reservedQuAmount -= sint64(locals.feeToTeam.low);
		locals.poolBasicState.reservedQuAmount -= sint64(locals.feeToProtocol.low);
		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log SwapAssetForExactQu procedure
		locals.swapMessage._contractIndex = SELF_INDEX;
		locals.swapMessage._type = QSWAPSwapAssetForExactQu;
		locals.swapMessage.assetIssuer = input.assetIssuer;
		locals.swapMessage.assetName = input.assetName;
		locals.swapMessage.assetAmountIn = output.assetAmountIn;
		locals.swapMessage.assetAmountOut = input.quAmountOut;
		LOG_INFO(locals.swapMessage);
	}

	struct TransferShareOwnershipAndPossession_locals
	{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareOwnershipAndPossession)
	{
		output.transferredAmount = 0;

		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

		if (qpi.invocationReward() < locals.feesOutput.transferFee)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (input.amount <= 0)
		{
			if (qpi.invocationReward() > 0 )
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		if (qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				SELF_INDEX,
				SELF_INDEX
			) < input.amount)
		{

			if (qpi.invocationReward() > 0 )
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		output.transferredAmount = qpi.transferShareOwnershipAndPossession(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				input.amount,
				input.newOwnerAndPossessor
			) < 0 ? 0 : input.amount;

		if (output.transferredAmount == 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		else if (qpi.invocationReward() > locals.feesOutput.transferFee)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.feesOutput.transferFee);
		}

		state.protocolEarnedFee += locals.feesOutput.transferFee;
	}

	PUBLIC_PROCEDURE(SetTeamInfo)
	{
		output.success = false;
		if (qpi.invocator() != state.teamId)
		{
			return;
		}

		state.teamId = input.newTeamId;
		output.success = true;
	}

	PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < QSWAP_FEE_BASE_100)
		{
			return ;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
		}
		else
		{
			if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, QSWAP_FEE_BASE_100) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
				if (qpi.invocationReward() > QSWAP_FEE_BASE_100)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  QSWAP_FEE_BASE_100);
				}
			}
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// functions
		REGISTER_USER_FUNCTION(Fees, 1);
		REGISTER_USER_FUNCTION(GetPoolBasicState, 2);
		REGISTER_USER_FUNCTION(GetLiquidityOf, 3);
		REGISTER_USER_FUNCTION(QuoteExactQuInput, 4);
		REGISTER_USER_FUNCTION(QuoteExactQuOutput, 5);
		REGISTER_USER_FUNCTION(QuoteExactAssetInput, 6);
		REGISTER_USER_FUNCTION(QuoteExactAssetOutput, 7);
		REGISTER_USER_FUNCTION(TeamInfo, 8);

		// procedure
		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(CreatePool, 3);
		REGISTER_USER_PROCEDURE(AddLiquidity, 4);
		REGISTER_USER_PROCEDURE(RemoveLiquidity, 5);
		REGISTER_USER_PROCEDURE(SwapExactQuForAsset, 6);
		REGISTER_USER_PROCEDURE(SwapQuForExactAsset, 7);
		REGISTER_USER_PROCEDURE(SwapExactAssetForQu, 8);
		REGISTER_USER_PROCEDURE(SwapAssetForExactQu, 9);
		REGISTER_USER_PROCEDURE(SetTeamInfo, 10);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 11);
	}

	INITIALIZE()
	{
		state.swapFeeRate = 30; 		// 0.3%, must less than 10000
		state.poolCreationFeeRate = 20; 	// 20%, must less than 100
		// earned fee: 20% to team, 80% to (shareholders and stakers), share holders take 16% (20% * 80%), stakers take  64% (80% * 80%)
		state.teamFeeRate = 20;			// 20%
		state.protocolFeeRate = 20; 	// 20%, must less than 100
		// IRUNQTXZRMLDEENHPRZQPSGPCFACORRUJYSBVJPQEHFCEKLLURVDDJVEXNBL
		state.teamId = ID(_I, _R, _U, _N, _Q, _T, _X, _Z, _R, _M, _L, _D, _E, _E, _N, _H, _P, _R, _Z, _Q, _P, _S, _G, _P, _C, _F, _A, _C, _O, _R, _R, _U, _J, _Y, _S, _B, _V, _J, _P, _Q, _E, _H, _F, _C, _E, _K, _L, _L, _U, _R, _V, _D, _D, _J, _V, _E);
	}

	END_TICK()
	{
		// distribute team fee
		if (state.teamEarnedFee > state.teamDistributedAmount)
		{
			qpi.transfer(state.teamId, state.teamEarnedFee - state.teamDistributedAmount);
			state.teamDistributedAmount += state.teamEarnedFee - state.teamDistributedAmount;
		}

		// distribute ipo fee
		if ((div((state.protocolEarnedFee - state.protocolDistributedAmount), 676ULL) > 0) && (state.protocolEarnedFee > state.protocolDistributedAmount))
		{
			if (qpi.distributeDividends(div((state.protocolEarnedFee - state.protocolDistributedAmount), 676ULL)))
			{
				state.protocolDistributedAmount += div((state.protocolEarnedFee- state.protocolDistributedAmount), 676ULL) * NUMBER_OF_COMPUTORS;
			}
		}
	}
	PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
