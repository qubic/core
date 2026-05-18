using namespace QPI;

// Log types enum for QSWAP contract
enum QSWAPLogInfo {
    QSWAPAddLiquidity = 4,
    QSWAPRemoveLiquidity = 5,
    QSWAPSwapExactQuForAsset = 6,
    QSWAPSwapQuForExactAsset = 7,
    QSWAPSwapExactAssetForQu = 8,
    QSWAPSwapAssetForExactQu = 9,
	QSWAPFailedDistribution = 10,
};

// FIXED CONSTANTS
constexpr uint64 QSWAP_INITIAL_MAX_POOL = 8192;
constexpr uint64 QSWAP_MAX_POOL = QSWAP_INITIAL_MAX_POOL * X_MULTIPLIER;
constexpr uint64 QSWAP_MAX_USER_PER_POOL = 256;
constexpr sint64 QSWAP_MIN_LIQUIDITY = 1000;
constexpr sint64 QSWAP_ADDITIONAL_FEE = 100000;
constexpr uint32 QSWAP_SWAP_FEE_BASE = 10000;
constexpr uint32 QSWAP_FEE_BASE_100 = 100;

struct QSWAP2
{
};

struct QSWAP : public ContractBase
{
public:
	// Logging message structures
	struct AddLiquidityMessage
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

	struct RemoveLiquidityMessage
	{
		uint32 _contractIndex;
		uint32 _type;
		sint64 quAmount;
		sint64 assetAmount;
		sint8 _terminator;
	};

	struct SwapMessage
	{
		uint32 _contractIndex;
		uint32 _type;
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountIn;
		sint64 assetAmountOut;
		sint8 _terminator;
	};

	struct FailedDistributionMessage
	{
		uint32 _contractIndex;
		uint32 _type;
		id dst;
		uint64 amount;
		sint8 _terminator;
	};

	// Types used by state fields
	struct PoolBasicState
	{
		id poolID;
		sint64 reservedQuAmount;
		sint64 reservedAssetAmount;
		sint64 totalLiquidity;
		uint128 accFeePerLPX64;
	};

	struct LiquidityInfo
	{
		sint64 liquidity;
		uint128 feeDebtX64;
		uint64 accumulatedFee;
	};

	struct StateData
	{
		uint32 swapFeeRate; 		// e.g. 30: 0.3% (base: 10_000)
		uint32 investRewardsFeeRate;// 3: 3% of swap fees to Invest & Rewards (base: 100)
		uint32 shareholderFeeRate; 	// 27: 27% of swap fees to SC shareholders (base: 100)
		uint32 poolCreationFeeRate;	// e.g. 10: 10% (base: 100)

		id investRewardsId;
		uint64 investRewardsEarnedFee;
		uint64 investRewardsDistributedAmount;

		uint64 shareholderEarnedFee;
		uint64 shareholderDistributedAmount;

		Array<PoolBasicState, QSWAP_MAX_POOL> mPoolBasicStates;
		Collection<LiquidityInfo, QSWAP_MAX_POOL* QSWAP_MAX_USER_PER_POOL> mLiquidities;

		uint32 qxFeeRate;			// 5: 5% of swap fees to QX (base: 100)
		uint32 burnFeeRate;			// 1: 1% of swap fees burned (base: 100)

		uint64 qxEarnedFee;
		uint64 qxDistributedAmount;

		uint64 burnEarnedFee;		// Total burn fees collected (to be burned in END_TICK)
		uint64 burnedAmount;		// Total amount actually burned

		uint32 cachedIssuanceFee;
		uint32 cachedTransferFee;
	};

	struct FindPoolSlotReadOnly_input
	{
		id assetIssuer;
		uint64 assetName;
	};
	struct FindPoolSlotReadOnly_output
	{
		sint64 poolSlot;
	};

	struct FindPoolSlotReadOnly_locals
	{
		id poolID;
		uint32 i0;
	};

	struct Fees_input
	{
	};
	struct Fees_output
	{
		uint32 assetIssuanceFee; 	// Amount of qus
		uint32 poolCreationFee; 	// Amount of qus
		uint32 transferFee; 		// Amount of qus

		uint32 swapFee; 			// 30 -> 0.3%
		uint32 shareholderFee;		// 27 -> 27% of swap fee, for SC shareholders
		uint32 investRewardsFee;	// 3 -> 3% of swap fee, for Invest & Rewards
		uint32 qxFee;				// 5 -> 5% of swap fee, for QX
		uint32 burnFee;				// 1 -> 1% of swap fee, burned
	};

	struct InvestRewardsInfo_input
	{
	};
	struct InvestRewardsInfo_output
	{
		uint32 investRewardsFee;	// 3 -> 3% of swap fee
		id investRewardsId;
	};

	struct SetInvestRewardsInfo_input
	{
		id newInvestRewardsId;
	};
	struct SetInvestRewardsInfo_output
	{
		sint32 success; // 1 if updated, else 0
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
		uint64 accFeePerLP;
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
		uint64 earnedFees;
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
		sint32 success; // 1 if pool created, else 0
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


	inline static sint64 min(sint64 a, sint64 b)
	{
		return (a < b) ? a : b;
	}

	// Collection PoV must be unique per (pool, LP). Using poolID alone forced O(#LPs) scans; one PoV per pair yields headIndex ~ O(1).
	inline static id liquidityPov(const id& poolID, const id& entity, id& r)
	{
		r = entity;
		r.u64._0 ^= poolID.u64._0;
		r.u64._1 ^= poolID.u64._1;
		r.u64._2 ^= poolID.u64._2;
		r.u64._3 ^= poolID.u64._3;
		return r;
	}

	// find the sqrt of a*b
	inline static sint64 sqrt(sint64& a, sint64& b, uint128& prod, uint128& y, uint128& z)
	{
		if (a == b)
		{
			return a;
		}

		prod = uint128(a) * uint128(b);

		y = uint128(0);

		z = uint128(1) << uint128(0, 126);
		while (z > prod)
		{
			z >>= uint128(0, 2);
		}

		while (z)
		{
			if (prod >= y + z)
			{
				prod -= y + z;
				y = (y >> uint128(0, 1)) + z;
			}
			else
			{
				y >>= uint128(0, 1);
			}
			z >>= uint128(0, 2);
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
		if (amountIn >= MAX_AMOUNT) return -1;

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
	// x = (reserveIn * amountOut * 10000) / ((reserveOut - amountOut) * (10000 - fee))
	inline static sint64 getAmountInTakeFeeFromInToken(sint64& amountOut, sint64& reserveIn, sint64& reserveOut, uint32 fee, uint128& numerator, uint128& denominator, uint128& tmpRes)
	{
		if (amountOut >= MAX_AMOUNT) return -1;

		// Calculate full numerator first to avoid premature truncation
		numerator = uint128(reserveIn) * uint128(amountOut) * uint128(QSWAP_SWAP_FEE_BASE);
		denominator = uint128(reserveOut - amountOut) * uint128(QSWAP_SWAP_FEE_BASE - fee);

		// Perform single division at the end
		// Use floor + 1 to ensure user pays at least enough (protects LPs)
		tmpRes = div(numerator, denominator) + uint128(1);

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
	// NOTE: Despite the name, this returns the GROSS output (before fee deduction).
	// The fee parameter is unused here because fee is applied separately by the caller.
	// This is intentional: the caller needs the gross value for fee distribution calculation.
	inline static sint64 getAmountOutTakeFeeFromOutToken(sint64& amountIn, sint64& reserveIn, sint64& reserveOut, uint32 fee, uint128& numerator, uint128& denominator, uint128& tmpRes)
	{
		if (amountIn >= MAX_AMOUNT) return -1;

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
	// x = (reserveIn * amountOut * 10000) / (reserveOut * (10000-fee) - amountOut * 10000)
	inline static sint64 getAmountInTakeFeeFromOutToken(sint64& amountOut, sint64& reserveIn, sint64& reserveOut, uint32 fee, uint128& numerator, uint128& denominator, uint128& tmpRes)
	{
		if (amountOut >= MAX_AMOUNT) return -1;

		// Calculate full numerator to avoid premature truncation
		numerator = uint128(reserveIn) * uint128(amountOut) * uint128(QSWAP_SWAP_FEE_BASE);

		// Check: reserveOut * (1-fee) must be greater than amountOut
		// Scale reserveOut by (10000-fee) and amountOut by 10000 for comparison
		// Use tmpRes and denominator temporarily for the comparison
		tmpRes = uint128(reserveOut) * uint128(QSWAP_SWAP_FEE_BASE - fee);
		denominator = uint128(amountOut) * uint128(QSWAP_SWAP_FEE_BASE);

		if (tmpRes <= denominator)
		{
			return -1;
		}

		denominator = tmpRes - denominator;

		// Use floor + 1 to ensure user pays at least enough (protects LPs)
		tmpRes = div(numerator, denominator) + uint128(1);

		if ((tmpRes.high != 0) || (tmpRes.low > 0x7FFFFFFFFFFFFFFF))
		{
			return -1;
		}
		else
		{
			return sint64(tmpRes.low);
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(FindPoolSlotReadOnly)
	{
		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		output.poolSlot = NULL_INDEX;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0++)
		{
			if (state.get().mPoolBasicStates.get(locals.i0).poolID == locals.poolID)
			{
				output.poolSlot = sint64(locals.i0);
				return;
			}
		}
	}

	PUBLIC_FUNCTION(Fees)
	{
		output.assetIssuanceFee = state.get().cachedIssuanceFee;
		output.poolCreationFee = uint32(div(uint64(state.get().cachedIssuanceFee) * uint64(state.get().poolCreationFeeRate), uint64(QSWAP_FEE_BASE_100)));
		output.transferFee = state.get().cachedTransferFee;
		output.swapFee = state.get().swapFeeRate;
		output.shareholderFee = state.get().shareholderFeeRate;
		output.investRewardsFee = state.get().investRewardsFeeRate;
		output.qxFee = state.get().qxFeeRate;
		output.burnFee = state.get().burnFeeRate;
	}

	struct GetPoolBasicState_locals
	{
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		FindPoolSlotReadOnly_input fsRoIn;
		FindPoolSlotReadOnly_output fsRoOut;
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

		locals.fsRoIn.assetIssuer = input.assetIssuer;
		locals.fsRoIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fsRoIn, locals.fsRoOut);
		locals.poolSlot = locals.fsRoOut.poolSlot;

		if (locals.poolSlot == NULL_INDEX)
		{
			return;
		}

		output.poolExists = 1;

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

		output.reservedQuAmount = locals.poolBasicState.reservedQuAmount;
		output.reservedAssetAmount = locals.poolBasicState.reservedAssetAmount;
		output.totalLiquidity = locals.poolBasicState.totalLiquidity;
		output.accFeePerLP = locals.poolBasicState.accFeePerLPX64.high;
	}

	struct GetLiquidityOf_locals
	{
		id poolID;
		id liqPov;
		sint64 liqElementIndex;
		id r;
		GetPoolBasicState_input getPoolBasicState_input;
		GetPoolBasicState_output getPoolBasicState_output;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetLiquidityOf)
	{
		output.liquidity = 0;

		locals.getPoolBasicState_input.assetName = input.assetName;
		locals.getPoolBasicState_input.assetIssuer = input.assetIssuer;
		CALL(GetPoolBasicState, locals.getPoolBasicState_input, locals.getPoolBasicState_output);

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.liqPov = liquidityPov(locals.poolID, input.account, locals.r);
		locals.liqElementIndex = state.get().mLiquidities.headIndex(locals.liqPov, 0);

		if (locals.liqElementIndex != NULL_INDEX)
		{
			output.liquidity = state.get().mLiquidities.element(locals.liqElementIndex).liquidity;
			if (locals.getPoolBasicState_output.poolExists && locals.getPoolBasicState_output.accFeePerLP > 0)
			{
				output.earnedFees = state.get().mLiquidities.element(locals.liqElementIndex).accumulatedFee + state.get().mLiquidities.element(locals.liqElementIndex).liquidity * (locals.getPoolBasicState_output.accFeePerLP - state.get().mLiquidities.element(locals.liqElementIndex).feeDebtX64.high);
			}
		}
	}

	struct QuoteExactQuInput_locals
	{
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint128 i1, i2, i3, i4;
		FindPoolSlotReadOnly_input fsRoIn;
		FindPoolSlotReadOnly_output fsRoOut;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactQuInput)
	{
		output.assetAmountOut = -1;

		if (input.quAmountIn <= 0)
		{
			return;
		}

		locals.fsRoIn.assetIssuer = input.assetIssuer;
		locals.fsRoIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fsRoIn, locals.fsRoOut);
		locals.poolSlot = locals.fsRoOut.poolSlot;

		// no available slot for new pool
		if (locals.poolSlot == NULL_INDEX)
		{
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

		// no liquidity in the pool
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			return;
		}

		output.assetAmountOut = getAmountOutTakeFeeFromInToken(
			input.quAmountIn,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.get().swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3,
			locals.i4
		);
	}

	struct QuoteExactQuOutput_locals
	{
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint128 i1, i2, i3;
		FindPoolSlotReadOnly_input fsRoIn;
		FindPoolSlotReadOnly_output fsRoOut;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactQuOutput)
	{
		output.assetAmountIn = -1;

		if (input.quAmountOut <= 0)
		{
			return;
		}

		locals.fsRoIn.assetIssuer = input.assetIssuer;
		locals.fsRoIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fsRoIn, locals.fsRoOut);
		locals.poolSlot = locals.fsRoOut.poolSlot;

		// no available slot for new pool
		if (locals.poolSlot == NULL_INDEX)
		{
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

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
			state.get().swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);
	}

	struct QuoteExactAssetInput_locals
	{
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountOutWithFee;

		uint128 i1, i2, i3;
		FindPoolSlotReadOnly_input fsRoIn;
		FindPoolSlotReadOnly_output fsRoOut;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactAssetInput)
	{
		output.quAmountOut = -1;

		if (input.assetAmountIn <= 0)
		{
			return;
		}

		locals.fsRoIn.assetIssuer = input.assetIssuer;
		locals.fsRoIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fsRoIn, locals.fsRoOut);
		locals.poolSlot = locals.fsRoOut.poolSlot;

		// no available slot for new pool
		if (locals.poolSlot == NULL_INDEX)
		{
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

		// no liquidity in the pool
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			return;
		}

		locals.quAmountOutWithFee = getAmountOutTakeFeeFromOutToken(
			input.assetAmountIn,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.get().swapFeeRate,
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
			uint128(locals.quAmountOutWithFee) * uint128(QSWAP_SWAP_FEE_BASE - state.get().swapFeeRate),
			uint128(QSWAP_SWAP_FEE_BASE)
		).low);
	}

	struct QuoteExactAssetOutput_locals
	{
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint128 i1, i2, i3;
		FindPoolSlotReadOnly_input fsRoIn;
		FindPoolSlotReadOnly_output fsRoOut;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactAssetOutput)
	{
		output.quAmountIn = -1;

		if (input.assetAmountOut <= 0)
		{
			return;
		}

		locals.fsRoIn.assetIssuer = input.assetIssuer;
		locals.fsRoIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fsRoIn, locals.fsRoOut);
		locals.poolSlot = locals.fsRoOut.poolSlot;

		// no available slot for new pool
		if (locals.poolSlot == NULL_INDEX)
		{
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

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
			state.get().swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);
	}

	PUBLIC_FUNCTION(InvestRewardsInfo)
	{
		output.investRewardsFee = state.get().investRewardsFeeRate;
		output.investRewardsId = state.get().investRewardsId;
	}

//
// procedure
//
	PUBLIC_PROCEDURE(IssueAsset)
	{
		output.issuedNumberOfShares = 0;
		if ((qpi.invocationReward() < state.get().cachedIssuanceFee))
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
		else
		{
			if (qpi.invocationReward() > state.get().cachedIssuanceFee)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().cachedIssuanceFee);
			}
			state.mut().shareholderEarnedFee += state.get().cachedIssuanceFee;
		}
	}

	struct CreatePool_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		uint32 poolCreationFee;

		uint32 i1;
		FindPoolSlotReadOnly_input fsRoIn;
		FindPoolSlotReadOnly_output fsRoOut;
	};

	// create uniswap like pool
	// TODO: reject if there is no shares avaliabe shares in current contract, e.g. asset is issue in contract qx
	PUBLIC_PROCEDURE_WITH_LOCALS(CreatePool)
	{
		output.success = 0;

		locals.poolCreationFee = uint32(div(uint64(state.get().cachedIssuanceFee) * uint64(state.get().poolCreationFeeRate), uint64(QSWAP_FEE_BASE_100)));

		// fee check
		if (qpi.invocationReward() < locals.poolCreationFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// asset no exist
		if (!qpi.isAssetIssued(input.assetIssuer, input.assetName))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.fsRoIn.assetIssuer = input.assetIssuer;
		locals.fsRoIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fsRoIn, locals.fsRoOut);
		if (locals.fsRoOut.poolSlot != NULL_INDEX)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
		}

		// find an vacant pool slot
		locals.poolSlot = -1;
		for (locals.i1 = 0; locals.i1 < QSWAP_MAX_POOL; locals.i1 ++)
		{
			if (state.get().mPoolBasicStates.get(locals.i1).poolID == id(0,0,0,0))
			{
				locals.poolSlot = locals.i1;
				break;
			}
		}

		// no available slot for new pool
		if (locals.poolSlot == -1)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState.poolID = locals.poolID;
		locals.poolBasicState.reservedAssetAmount = 0;
		locals.poolBasicState.reservedQuAmount = 0;
		locals.poolBasicState.totalLiquidity = 0;

		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		if(qpi.invocationReward() > locals.poolCreationFee)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.poolCreationFee );
		}
		state.mut().shareholderEarnedFee += locals.poolCreationFee;

		output.success = 1;
	}


	struct AddLiquidity_locals
	{
		AddLiquidityMessage addLiquidityMessage;
		id poolID;
		sint64 poolSlot;
		id r;
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

		uint128 i1, i2, i3;

		uint128 pendingFeeX64;
		FindPoolSlotReadOnly_input fspIn;
		FindPoolSlotReadOnly_output fspOut;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(AddLiquidity)
	{
		output.userIncreaseLiquidity = 0;
		output.assetAmount = 0;
		output.quAmount = 0;

		// add liquidity must stake both qu and asset
		if (qpi.invocationReward() <= QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.quAmountDesired = qpi.invocationReward() - QSWAP_ADDITIONAL_FEE;

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

		locals.fspIn.assetIssuer = input.assetIssuer;
		locals.fspIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fspIn, locals.fspOut);
		locals.poolSlot = locals.fspOut.poolSlot;

		if (locals.poolSlot == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

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

			state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
			state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

			// permanently lock the first MINIMUM_LIQUIDITY tokens
			locals.tmpLiquidity.liquidity = QSWAP_MIN_LIQUIDITY;
			state.mut().mLiquidities.add(liquidityPov(locals.poolID, SELF, locals.r), locals.tmpLiquidity, 0);

			locals.tmpLiquidity.liquidity = locals.increaseLiquidity - QSWAP_MIN_LIQUIDITY;
			state.mut().mLiquidities.add(liquidityPov(locals.poolID, qpi.invocator(), locals.r), locals.tmpLiquidity, 0);

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

			locals.userLiquidityElementIndex = state.get().mLiquidities.headIndex(liquidityPov(locals.poolID, qpi.invocator(), locals.r), 0);

			// no more space for new liquidity item
			if ((locals.userLiquidityElementIndex == NULL_INDEX) && ( state.get().mLiquidities.population() == state.get().mLiquidities.capacity()))
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

			state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
			state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

			if (locals.userLiquidityElementIndex == NULL_INDEX)
			{
				locals.tmpLiquidity.liquidity = locals.increaseLiquidity;
				locals.tmpLiquidity.feeDebtX64 = locals.poolBasicState.accFeePerLPX64;
				state.mut().mLiquidities.add(liquidityPov(locals.poolID, qpi.invocator(), locals.r), locals.tmpLiquidity, 0);
			}
			else
			{
				locals.tmpLiquidity = state.get().mLiquidities.element(locals.userLiquidityElementIndex);
				locals.pendingFeeX64 = uint128(locals.tmpLiquidity.liquidity) * (locals.poolBasicState.accFeePerLPX64 - locals.tmpLiquidity.feeDebtX64);
				locals.tmpLiquidity.accumulatedFee += locals.pendingFeeX64.high;
				locals.tmpLiquidity.liquidity += locals.increaseLiquidity;
				locals.tmpLiquidity.feeDebtX64 = locals.poolBasicState.accFeePerLPX64;
				state.mut().mLiquidities.replace(locals.userLiquidityElementIndex, locals.tmpLiquidity);
			}

			output.quAmount = locals.quTransferAmount;
			output.assetAmount = locals.assetTransferAmount;
			output.userIncreaseLiquidity = locals.increaseLiquidity;
		}

		locals.poolBasicState.reservedQuAmount += locals.quTransferAmount;
		locals.poolBasicState.reservedAssetAmount += locals.assetTransferAmount;
		locals.poolBasicState.totalLiquidity += locals.increaseLiquidity;

		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log AddLiquidity procedure
		locals.addLiquidityMessage._contractIndex = SELF_INDEX;
		locals.addLiquidityMessage._type = QSWAPAddLiquidity;
		locals.addLiquidityMessage.assetIssuer = input.assetIssuer;
		locals.addLiquidityMessage.assetName = input.assetName;
		locals.addLiquidityMessage.userIncreaseLiquidity = output.userIncreaseLiquidity;
		locals.addLiquidityMessage.quAmount = output.quAmount;
		locals.addLiquidityMessage.assetAmount = output.assetAmount;
		LOG_INFO(locals.addLiquidityMessage);

		if (qpi.invocationReward() - QSWAP_ADDITIONAL_FEE > locals.quTransferAmount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.quTransferAmount - QSWAP_ADDITIONAL_FEE);
		}
	}

	struct RemoveLiquidity_locals
	{
		RemoveLiquidityMessage removeLiquidityMessage;
		id poolID;
		PoolBasicState poolBasicState;
		id r;
		sint64 userLiquidityElementIndex;
		sint64 poolSlot;
		LiquidityInfo userLiquidity;
		sint64 burnQuAmount;
		sint64 burnAssetAmount;

		uint32 i0;
		uint128 pendingFeeX64;
		FindPoolSlotReadOnly_input fspIn;
		FindPoolSlotReadOnly_output fspOut;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(RemoveLiquidity)
	{
		output.quAmount = 0;
		output.assetAmount = 0;

		if (qpi.invocationReward() < QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		else if (qpi.invocationReward() > QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QSWAP_ADDITIONAL_FEE);
		}

		// check the vadility of input params
		if (input.quAmountMin < 0 || input.assetAmountMin < 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.fspIn.assetIssuer = input.assetIssuer;
		locals.fspIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fspIn, locals.fspOut);
		locals.poolSlot = locals.fspOut.poolSlot;

		// the pool does not exsit
		if (locals.poolSlot == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

		locals.userLiquidityElementIndex = state.get().mLiquidities.headIndex(liquidityPov(locals.poolID, qpi.invocator(), locals.r), 0);

		if (locals.userLiquidityElementIndex == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.userLiquidity = state.get().mLiquidities.element(locals.userLiquidityElementIndex);

		// not enough liquidity for burning
		if (locals.userLiquidity.liquidity < input.burnLiquidity )
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		if (locals.poolBasicState.totalLiquidity < input.burnLiquidity )
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
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
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
		state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

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
		locals.pendingFeeX64 = uint128(locals.userLiquidity.liquidity) * (locals.poolBasicState.accFeePerLPX64 - locals.userLiquidity.feeDebtX64);
		locals.userLiquidity.liquidity -= input.burnLiquidity;
		if (locals.userLiquidity.liquidity == 0)
		{
			state.mut().mLiquidities.remove(locals.userLiquidityElementIndex);
		}
		else
		{
			locals.userLiquidity.accumulatedFee += locals.pendingFeeX64.high;
			locals.userLiquidity.feeDebtX64 = locals.poolBasicState.accFeePerLPX64;
			state.mut().mLiquidities.replace(locals.userLiquidityElementIndex, locals.userLiquidity);
		}

		// modify the pool's liquidity info
		locals.poolBasicState.totalLiquidity -= input.burnLiquidity;
		locals.poolBasicState.reservedQuAmount -= locals.burnQuAmount;
		locals.poolBasicState.reservedAssetAmount -= locals.burnAssetAmount;

		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log RemoveLiquidity procedure
		locals.removeLiquidityMessage._contractIndex = SELF_INDEX;
		locals.removeLiquidityMessage._type = QSWAPRemoveLiquidity;
		locals.removeLiquidityMessage.quAmount = output.quAmount;
		locals.removeLiquidityMessage.assetAmount = output.assetAmount;
		LOG_INFO(locals.removeLiquidityMessage);
	}

	struct SwapExactQuForAsset_locals
	{
		SwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		sint64 quAmountIn;
		PoolBasicState poolBasicState;
		sint64 assetAmountOut;

		uint128 i1, i2, i3, i4;
		uint128 swapFee;
		uint128 feeToInvestRewards;
		uint128 feeToShareholders;

		uint128 feeToQx;
		uint128 feeToBurn;

		sint64 totalFee;
		uint128 scaledLpFee;
		FindPoolSlotReadOnly_input fspIn;
		FindPoolSlotReadOnly_output fspOut;
	};

	// given an input qu amountIn, only execute swap in case (amountOut >= amountOutMin)
	// https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#swapexacttokensfortokens
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapExactQuForAsset)
	{
		output.assetAmountOut = 0;

		if (qpi.invocationReward() <= QSWAP_ADDITIONAL_FEE || input.assetAmountOutMin < 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.quAmountIn = qpi.invocationReward() - QSWAP_ADDITIONAL_FEE;

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.fspIn.assetIssuer = input.assetIssuer;
		locals.fspIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fspIn, locals.fspOut);
		locals.poolSlot = locals.fspOut.poolSlot;

		if (locals.poolSlot == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

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
			state.get().swapFeeRate,
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

		// swapFee = quAmountIn * 0.3% (swapFeeRate/10000)
		// swapFee distribution: 27% shareholders, 5% QX, 3% invest&rewards, 1% burn, 64% LP
		locals.swapFee = div(uint128(locals.quAmountIn) * uint128(state.get().swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE));
		if (locals.swapFee == uint128_t(0))
		{
			locals.swapFee = QSWAP_FEE_BASE_100;
		}
		locals.feeToShareholders = div(locals.swapFee * uint128(state.get().shareholderFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToQx = div(locals.swapFee * uint128(state.get().qxFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToInvestRewards = div(locals.swapFee * uint128(state.get().investRewardsFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToBurn = div(locals.swapFee * uint128(state.get().burnFeeRate), uint128(QSWAP_FEE_BASE_100));

		locals.totalFee = sint64(locals.feeToShareholders.low) + sint64(locals.feeToQx.low) + sint64(locals.feeToInvestRewards.low) + sint64(locals.feeToBurn.low);

		// Overflow protection: ensure all fees fit in uint64
		if (locals.feeToShareholders.high != 0 || locals.feeToQx.high != 0
			|| locals.feeToInvestRewards.high != 0 || locals.feeToBurn.high != 0
			|| locals.quAmountIn < locals.totalFee)
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

		state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
		state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

		// update fee state after successful transfer
		state.mut().shareholderEarnedFee += locals.feeToShareholders.low;
		state.mut().qxEarnedFee += locals.feeToQx.low;
		state.mut().investRewardsEarnedFee += locals.feeToInvestRewards.low;
		state.mut().burnEarnedFee += locals.feeToBurn.low;

		locals.poolBasicState.reservedQuAmount += locals.quAmountIn - locals.totalFee;
		locals.poolBasicState.reservedAssetAmount -= locals.assetAmountOut;
		locals.scaledLpFee.high = locals.swapFee.low - locals.totalFee;
		locals.scaledLpFee.low = 0;
		locals.poolBasicState.accFeePerLPX64 += div(locals.scaledLpFee, uint128(locals.poolBasicState.totalLiquidity));
		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

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
		SwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountIn;
		sint64 transferredAssetAmount;

		uint128 i1, i2, i3;
		uint128 swapFee;
		uint128 feeToInvestRewards;
		uint128 feeToShareholders;
		uint128 feeToQx;
		uint128 feeToBurn;

		sint64 totalFee;
		uint128 scaledLpFee;
		FindPoolSlotReadOnly_input fspIn;
		FindPoolSlotReadOnly_output fspOut;
	};

	// https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#swaptokensforexacttokens
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapQuForExactAsset)
	{
		output.quAmountIn = 0;

		// check input param validity
		if (qpi.invocationReward() <= QSWAP_ADDITIONAL_FEE ||input.assetAmountOut <= 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.fspIn.assetIssuer = input.assetIssuer;
		locals.fspIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fspIn, locals.fspOut);
		locals.poolSlot = locals.fspOut.poolSlot;

		if (locals.poolSlot == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

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
			state.get().swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);

		// above call overflow
		if (locals.quAmountIn == -1 || locals.quAmountIn > qpi.invocationReward() - QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// swapFee = quAmountIn * 0.3% (swapFeeRate/10000)
		// swapFee distribution: 27% shareholders, 5% QX, 3% invest&rewards, 1% burn, 64% LP
		locals.swapFee = div(uint128(locals.quAmountIn) * uint128(state.get().swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE));
		if (locals.swapFee == uint128_t(0))
		{
			locals.swapFee = QSWAP_FEE_BASE_100;
		}
		locals.feeToShareholders = div(locals.swapFee * uint128(state.get().shareholderFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToQx = div(locals.swapFee * uint128(state.get().qxFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToInvestRewards = div(locals.swapFee * uint128(state.get().investRewardsFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToBurn = div(locals.swapFee * uint128(state.get().burnFeeRate), uint128(QSWAP_FEE_BASE_100));

		locals.totalFee = sint64(locals.feeToShareholders.low) + sint64(locals.feeToQx.low) + sint64(locals.feeToInvestRewards.low) + sint64(locals.feeToBurn.low);
		if (locals.quAmountIn < locals.totalFee)
		{
			qpi.transfer(qpi.invocator(), locals.quAmountIn);
			return;
		}

		// Overflow protection: ensure all fees fit in uint64
		if (locals.feeToShareholders.high != 0 || locals.feeToQx.high != 0 ||
		    locals.feeToInvestRewards.high != 0 || locals.feeToBurn.high != 0)
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
		if (qpi.invocationReward() - QSWAP_ADDITIONAL_FEE > locals.quAmountIn)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.quAmountIn - QSWAP_ADDITIONAL_FEE);
		}

		state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
		state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

		// update fee state after successful transfer
		state.mut().shareholderEarnedFee += locals.feeToShareholders.low;
		state.mut().qxEarnedFee += locals.feeToQx.low;
		state.mut().investRewardsEarnedFee += locals.feeToInvestRewards.low;
		state.mut().burnEarnedFee += locals.feeToBurn.low;

		locals.poolBasicState.reservedQuAmount += locals.quAmountIn - locals.totalFee;
		locals.poolBasicState.reservedAssetAmount -= input.assetAmountOut;
		locals.scaledLpFee.high = locals.swapFee.low - locals.totalFee;
		locals.scaledLpFee.low = 0;
		locals.poolBasicState.accFeePerLPX64 += div(locals.scaledLpFee, uint128(locals.poolBasicState.totalLiquidity));
		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

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
		SwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountOut;
		sint64 quAmountOutWithFee;
		sint64 transferredAssetAmountBefore;
		sint64 transferredAssetAmountAfter;

		uint128 i1, i2, i3;
		uint128 swapFee;
		uint128 feeToInvestRewards;
		uint128 feeToShareholders;
		uint128 feeToQx;
		uint128 feeToBurn;

		sint64 totalFee;
		uint128 scaledLpFee;
		FindPoolSlotReadOnly_input fspIn;
		FindPoolSlotReadOnly_output fspOut;
	};

	// given an amount of asset swap in, only execute swaping if quAmountOut >= input.amountOutMin
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapExactAssetForQu)
	{
		output.quAmountOut = 0;
		if (qpi.invocationReward() < QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		else if (qpi.invocationReward() > QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QSWAP_ADDITIONAL_FEE);
		}

		// check input param validity
		if (input.assetAmountIn <= 0 || input.quAmountOutMin < 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.fspIn.assetIssuer = input.assetIssuer;
		locals.fspIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fspIn, locals.fspOut);
		locals.poolSlot = locals.fspOut.poolSlot;

		if (locals.poolSlot == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

		// check the liquidity validity
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
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
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.quAmountOutWithFee = getAmountOutTakeFeeFromOutToken(
			input.assetAmountIn,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.get().swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);

		// above call overflow
		if (locals.quAmountOutWithFee == -1)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		// no overflow risk
		// locals.quAmountOutWithFee * (QSWAP_SWAP_FEE_BASE - state.swapFeeRate) / QSWAP_SWAP_FEE_BASE
		locals.quAmountOut = sint64(div(
				uint128(locals.quAmountOutWithFee) * uint128(QSWAP_SWAP_FEE_BASE - state.get().swapFeeRate),
				uint128(QSWAP_SWAP_FEE_BASE)
			).low);

		// not meet user min amountOut requirement
		if (locals.quAmountOut < input.quAmountOutMin)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		// swapFee = quAmountOutWithFee * 0.3% (swapFeeRate/10000)
		// swapFee distribution: 27% shareholders, 5% QX, 3% invest&rewards, 1% burn, 64% LP
		locals.swapFee = div(uint128(locals.quAmountOutWithFee) * uint128(state.get().swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE));
		if (locals.swapFee == uint128_t(0))
		{
			locals.swapFee = QSWAP_FEE_BASE_100;
		}
		locals.feeToShareholders = div(locals.swapFee * uint128(state.get().shareholderFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToQx = div(locals.swapFee * uint128(state.get().qxFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToInvestRewards = div(locals.swapFee * uint128(state.get().investRewardsFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToBurn = div(locals.swapFee * uint128(state.get().burnFeeRate), uint128(QSWAP_FEE_BASE_100));

		// Overflow protection: ensure all fees fit in uint64
		if (locals.feeToShareholders.high != 0 || locals.feeToQx.high != 0 ||
		    locals.feeToInvestRewards.high != 0 || locals.feeToBurn.high != 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		// transfer assets from user to pool
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

		// pool does not receive enough asset, rollback any received shares
		if (locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore < input.assetAmountIn)
		{
			// return any shares that were transferred
			if (locals.transferredAssetAmountAfter > locals.transferredAssetAmountBefore)
			{
				qpi.transferShareOwnershipAndPossession(
					input.assetName,
					input.assetIssuer,
					SELF,
					SELF,
					locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore,
					qpi.invocator()
				);
			}
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
		state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

		qpi.transfer(qpi.invocator(), locals.quAmountOut);
		output.quAmountOut = locals.quAmountOut;

		// update fee state after successful transfers
		state.mut().shareholderEarnedFee += locals.feeToShareholders.low;
		state.mut().qxEarnedFee += locals.feeToQx.low;
		state.mut().investRewardsEarnedFee += locals.feeToInvestRewards.low;
		state.mut().burnEarnedFee += locals.feeToBurn.low;

		// update pool states
		locals.poolBasicState.reservedAssetAmount += input.assetAmountIn;
		locals.totalFee = locals.quAmountOut + sint64(locals.feeToShareholders.low) + sint64(locals.feeToQx.low) + sint64(locals.feeToInvestRewards.low) + sint64(locals.feeToBurn.low);
		if (locals.poolBasicState.reservedQuAmount < locals.totalFee)
		{
			locals.poolBasicState.reservedQuAmount = 0;
		}
		else
		{
			locals.poolBasicState.reservedQuAmount -= locals.totalFee;
			locals.scaledLpFee.high = locals.swapFee.low - (locals.feeToShareholders.low + locals.feeToQx.low + locals.feeToInvestRewards.low + locals.feeToBurn.low);
			locals.scaledLpFee.low = 0;
			locals.poolBasicState.accFeePerLPX64 += div(locals.scaledLpFee, uint128(locals.poolBasicState.totalLiquidity));
		}
		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

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
		SwapMessage swapMessage;
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 assetAmountIn;
		sint64 transferredAssetAmountBefore;
		sint64 transferredAssetAmountAfter;

		uint128 i1, i2, i3;
		uint128 swapFee;
		uint128 feeToInvestRewards;
		uint128 feeToShareholders;
		uint128 feeToQx;
		uint128 feeToBurn;

		sint64 totalFee;
		uint128 scaledLpFee;
		FindPoolSlotReadOnly_input fspIn;
		FindPoolSlotReadOnly_output fspOut;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SwapAssetForExactQu)
	{
		output.assetAmountIn = 0;

		if (qpi.invocationReward() < QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		else if (qpi.invocationReward() > QSWAP_ADDITIONAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QSWAP_ADDITIONAL_FEE);
		}

		if (input.assetAmountInMax <= 0 || input.quAmountOut <= 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.fspIn.assetIssuer = input.assetIssuer;
		locals.fspIn.assetName = input.assetName;
		CALL(FindPoolSlotReadOnly, locals.fspIn, locals.fspOut);
		locals.poolSlot = locals.fspOut.poolSlot;

		if (locals.poolSlot == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.poolBasicState = state.get().mPoolBasicStates.get(locals.poolSlot);

		// check the liquidity validity
		if (locals.poolBasicState.totalLiquidity == 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		// pool does not hold enough asset
		if (input.quAmountOut >= locals.poolBasicState.reservedQuAmount)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		locals.assetAmountIn = getAmountInTakeFeeFromOutToken(
			input.quAmountOut,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.get().swapFeeRate,
			locals.i1,
			locals.i2,
			locals.i3
		);

		// invalid input, assetAmountIn overflow
		if (locals.assetAmountIn == -1)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
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
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		// not meet user amountIn reqiurement
		if (locals.assetAmountIn > input.assetAmountInMax)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		// swapFee = quAmountOut * 30/(10_000 - 30)
		// swapFee distribution: 27% shareholders, 5% QX, 3% invest&rewards, 1% burn, 64% LP
		locals.swapFee = div(uint128(input.quAmountOut) * uint128(state.get().swapFeeRate), uint128(QSWAP_SWAP_FEE_BASE - state.get().swapFeeRate));
		if (locals.swapFee == uint128_t(0))
		{
			locals.swapFee = QSWAP_FEE_BASE_100;
		}
		locals.feeToShareholders = div(locals.swapFee * uint128(state.get().shareholderFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToQx = div(locals.swapFee * uint128(state.get().qxFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToInvestRewards = div(locals.swapFee * uint128(state.get().investRewardsFeeRate), uint128(QSWAP_FEE_BASE_100));
		locals.feeToBurn = div(locals.swapFee * uint128(state.get().burnFeeRate), uint128(QSWAP_FEE_BASE_100));

		// Overflow protection: ensure all fees fit in uint64
		if (locals.feeToShareholders.high != 0 || locals.feeToQx.high != 0 ||
		    locals.feeToInvestRewards.high != 0 || locals.feeToBurn.high != 0)
		{
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
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

		// pool does not receive enough asset, rollback any received shares
		if (locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore < locals.assetAmountIn)
		{
			// return any shares that were transferred
			if (locals.transferredAssetAmountAfter > locals.transferredAssetAmountBefore)
			{
				qpi.transferShareOwnershipAndPossession(
					input.assetName,
					input.assetIssuer,
					SELF,
					SELF,
					locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore,
					qpi.invocator()
				);
			}
			qpi.transfer(qpi.invocator(), QSWAP_ADDITIONAL_FEE);
			return;
		}

		state.mut().shareholderEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE * 3LL, 4LL);  // 75% to shareholders
		state.mut().burnEarnedFee += QPI::div(QSWAP_ADDITIONAL_FEE, 4LL);               // 25% to burn

		qpi.transfer(qpi.invocator(), input.quAmountOut);
		output.assetAmountIn = locals.assetAmountIn;

		// update fee state after successful transfers
		state.mut().shareholderEarnedFee += locals.feeToShareholders.low;
		state.mut().qxEarnedFee += locals.feeToQx.low;
		state.mut().investRewardsEarnedFee += locals.feeToInvestRewards.low;
		state.mut().burnEarnedFee += locals.feeToBurn.low;

		// update pool states
		locals.poolBasicState.reservedAssetAmount += locals.assetAmountIn;
		locals.totalFee = input.quAmountOut + sint64(locals.feeToShareholders.low) + sint64(locals.feeToQx.low) + sint64(locals.feeToInvestRewards.low) + sint64(locals.feeToBurn.low);
		if (locals.poolBasicState.reservedQuAmount < locals.totalFee)
		{
			locals.poolBasicState.reservedQuAmount = 0;
		}
		else
		{
			locals.poolBasicState.reservedQuAmount -= locals.totalFee;
			locals.scaledLpFee.high = locals.swapFee.low - (locals.feeToShareholders.low + locals.feeToQx.low + locals.feeToInvestRewards.low + locals.feeToBurn.low);
			locals.scaledLpFee.low = 0;
			locals.poolBasicState.accFeePerLPX64 += div(locals.scaledLpFee, uint128(locals.poolBasicState.totalLiquidity));
		}
		state.mut().mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		// Log SwapAssetForExactQu procedure
		locals.swapMessage._contractIndex = SELF_INDEX;
		locals.swapMessage._type = QSWAPSwapAssetForExactQu;
		locals.swapMessage.assetIssuer = input.assetIssuer;
		locals.swapMessage.assetName = input.assetName;
		locals.swapMessage.assetAmountIn = output.assetAmountIn;
		locals.swapMessage.assetAmountOut = input.quAmountOut;
		LOG_INFO(locals.swapMessage);
	}

	PUBLIC_PROCEDURE(TransferShareOwnershipAndPossession)
	{
		output.transferredAmount = 0;

		if (qpi.invocationReward() < state.get().cachedTransferFee)
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
		else
		{
			if (qpi.invocationReward() > state.get().cachedTransferFee)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().cachedTransferFee);
			}
			state.mut().shareholderEarnedFee += state.get().cachedTransferFee;
		}
	}

	PUBLIC_PROCEDURE(SetInvestRewardsInfo)
	{
		output.success = 0;
		if (qpi.invocator() != state.get().investRewardsId)
		{
			return;
		}

		state.mut().investRewardsId = input.newInvestRewardsId;
		output.success = 1;
	}

	struct TransferShareManagementRights_locals
	{
		sint64 result;
		sint64 reward;
		sint64 refundAmount;
		sint64 requiredFee;
		bit success;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		locals.reward = qpi.invocationReward();
		locals.refundAmount = locals.reward;

		output.transferredNumberOfShares = 0;

		locals.success = false;

		if (qpi.numberOfPossessedShares(
				input.asset.assetName,
				input.asset.issuer,
				qpi.invocator(),
				qpi.invocator(),
				SELF_INDEX,
				SELF_INDEX) >= input.numberOfShares)
		{
			locals.result = qpi.releaseShares(
				input.asset,
				qpi.invocator(),
				qpi.invocator(),
				input.numberOfShares,
				input.newManagingContractIndex,
				input.newManagingContractIndex,
				locals.reward
			);

			if (locals.result != INVALID_AMOUNT && locals.result >= 0)
			{
				locals.success = true;
				locals.refundAmount = locals.reward - locals.result;
			}
		}

		if (locals.success)
		{
			output.transferredNumberOfShares = input.numberOfShares;
		}

		if (locals.refundAmount > 0)
		{
			qpi.transfer(qpi.invocator(), locals.refundAmount);
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
		REGISTER_USER_FUNCTION(InvestRewardsInfo, 8);

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
		REGISTER_USER_PROCEDURE(SetInvestRewardsInfo, 10);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 11);
	}

	INITIALIZE()
	{
		state.mut().swapFeeRate = 30; 			// 0.3%, must be less than 10000
		state.mut().poolCreationFeeRate = 20; 	// 20%, must be less than 100

		// swapFee distribution: 27% shareholders, 5% QX, 3% invest&rewards, 1% burn, 64% LP providers
		state.mut().shareholderFeeRate = 27; 		// 27% of swap fees to SC shareholders
		state.mut().investRewardsFeeRate = 3;		// 3% of swap fees to Invest & Rewards
		state.mut().qxFeeRate = 5;				// 5% of swap fees to QX
		state.mut().burnFeeRate = 1;				// 1% of swap fees burned

		ASSERT(state.get().swapFeeRate < QSWAP_SWAP_FEE_BASE);
		ASSERT(state.get().shareholderFeeRate + state.get().investRewardsFeeRate + state.get().qxFeeRate + state.get().burnFeeRate <= 100);
		//
        state.mut().investRewardsId = ID(_V, _J, _G, _R, _U, _F, _W, _J, _C, _U, _S, _N, _H, _C, _Q, _J, _R, _W, _R, _R, _Y, _X, _A, _U, _E, _J, _F, _C, _V, _H, _Y, _P, _X, _W, _K, _T, _D, _L, _Y, _K, _U, _A, _C, _P, _V, _V, _Y, _B, _G, _O, _L, _V, _C, _J, _S, _F);
	}

	struct BEGIN_EPOCH_locals
	{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	BEGIN_EPOCH_WITH_LOCALS()
	{
		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

		if (interContractCallError == NoCallError)
		{
			state.mut().cachedIssuanceFee = locals.feesOutput.assetIssuanceFee;
			state.mut().cachedTransferFee = locals.feesOutput.transferFee;
		}
	}

	struct END_TICK_locals
	{
		uint64 toDistribute;
		uint64 toBurn;
		uint64 dividendPerComputor;
		sint64 transferredAmount;
		FailedDistributionMessage logMsg;
	};

	END_TICK_WITH_LOCALS()
	{
		// Distribute Invest & Rewards fees
		if (state.get().investRewardsEarnedFee > state.get().investRewardsDistributedAmount)
		{
			locals.toDistribute = state.get().investRewardsEarnedFee - state.get().investRewardsDistributedAmount;
			locals.transferredAmount = qpi.transfer(state.get().investRewardsId, locals.toDistribute);
			if (locals.transferredAmount < 0)
			{
				locals.logMsg._contractIndex = SELF_INDEX;
				locals.logMsg._type = QSWAPFailedDistribution;
				locals.logMsg.dst = state.get().investRewardsId;
				locals.logMsg.amount = locals.toDistribute;
				LOG_INFO(locals.logMsg);
			}
			else
				state.mut().investRewardsDistributedAmount += locals.toDistribute;
		}

		// Distribute QX fees as donation
		if (state.get().qxEarnedFee > state.get().qxDistributedAmount)
		{
			locals.toDistribute = state.get().qxEarnedFee - state.get().qxDistributedAmount;
			locals.transferredAmount = qpi.transfer(id(QX_CONTRACT_INDEX, 0, 0, 0), locals.toDistribute);
			if (locals.transferredAmount < 0)
			{
				locals.logMsg._contractIndex = SELF_INDEX;
				locals.logMsg._type = QSWAPFailedDistribution;
				locals.logMsg.dst = id(QX_CONTRACT_INDEX, 0, 0, 0);
				locals.logMsg.amount = locals.toDistribute;
				LOG_INFO(locals.logMsg);
			}
			else
				state.mut().qxDistributedAmount += locals.toDistribute;
		}

		// Distribute shareholder fees (to IPO shareholders via dividends)
		if (state.get().shareholderEarnedFee > state.get().shareholderDistributedAmount)
		{
			locals.dividendPerComputor = div((state.get().shareholderEarnedFee - state.get().shareholderDistributedAmount), 676ULL);
			if (locals.dividendPerComputor > 0 && qpi.distributeDividends(locals.dividendPerComputor))
			{
				state.mut().shareholderDistributedAmount += locals.dividendPerComputor * NUMBER_OF_COMPUTORS;
			}
		}

		// Burn fees (adds to contract execution fee reserve)
		if (state.get().burnEarnedFee > state.get().burnedAmount)
		{
			locals.toBurn = state.get().burnEarnedFee - state.get().burnedAmount;
			qpi.burn(locals.toBurn);
			state.mut().burnedAmount += locals.toBurn;
		}
	}

	PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};
