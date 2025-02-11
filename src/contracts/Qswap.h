using namespace QPI;

// FIXED CONSTANTS
constexpr uint64 QSWAP_INITIAL_MAX_POOL = 16;
constexpr uint64 QSWAP_MAX_POOL = QSWAP_INITIAL_MAX_POOL * X_MULTIPLIER;
constexpr uint64 QSWAP_MAX_USER_PER_POOL = 16;
constexpr sint64 QSWAP_MIN_LIQUDITY = 1000;
constexpr uint32 QSWAP_SWAP_FEE_BASE = 10000;
constexpr uint32 QSWAP_PROTOCOL_FEE_BASE = 100;

struct QSWAP2 
{
};

struct QSWAP : public ContractBase
{
protected:
	uint32 swapFeeRate; 		// e.g. 30: 0.3% (base: 10_000)
	uint32 protocolFeeRate; 	// e.g. 20: 20% (base: 100) only charge in qu
	uint32 poolCreateFee;		// qu amount

	uint64 protocolEarnedFee;
	uint64 distributedAmount;

	struct PoolBasicState{
		id poolID;
		sint64 reservedQuAmount;
		sint64 reservedAssetAmount;
		sint64 totalLiqudity;
	};

	struct LiqudityInfo {
		id entity;
		sint64 liqudity;
	};

	Array<PoolBasicState, QSWAP_MAX_POOL> mPoolBasicStates;
	Collection<LiqudityInfo, QSWAP_MAX_POOL * QSWAP_MAX_USER_PER_POOL > mLiquditys;

	inline static sint64 min(sint64 a, sint64 b) {
		return (a < b) ? a : b;
	}

	// find the sqrt of a*b
	inline static sint64 sqrt(sint64 a, sint64 b) {
		if (a == b) { return a; }

		uint128 prod = uint128(a) * uint128(b);

		// (prod + 1) / 2;
		uint128 z = div(prod+uint128(1), uint128(2));
		uint128_t y = prod;

		while(z < y){
			y = z;
			// (prod / z + z) / 2;
			z = div((div(prod, z) + z), uint128(2));
		}

		return sint64(y.low);
	}

	inline static sint64 quoteEquivalentAmountB(sint64 amountADesired, sint64 reserveA, sint64 reserveB) {
		// amountDesired * reserveB / reserveA
		uint128 res = div(uint128(amountADesired) * uint128(reserveB), uint128(reserveA));
		return sint64(res.low);
	}

	// https://github.com/Uniswap/v2-periphery/blob/master/contracts/libraries/UniswapV2Library.sol#L43 
	// x = reserveOut * amountIn * (1-fee) / (reserveIn + amountIn * (1-fee))
	inline static sint64 getAmountOutTakeFeeFromInToken(sint64 amountIn, sint64 reserveIn, sint64 reserveOut, uint32 fee) {
		uint128 amountInWithFee = uint128(amountIn) * uint128(QSWAP_SWAP_FEE_BASE - fee);
		uint128 numerator = uint128(reserveOut) * amountInWithFee;
		uint128 denominator = uint128(reserveIn) * uint128(QSWAP_SWAP_FEE_BASE) + amountInWithFee;

		// numerator / denominator;
		uint128 res = div(numerator, denominator);
		return sint64(res.low);
	}

	// https://github.com/Uniswap/v2-periphery/blob/master/contracts/libraries/UniswapV2Library.sol#L53 
	// x = (reserveIn * amountOut)/((1-fee) * (reserveOut - amountOut)
	inline static sint64 getAmountInTakeFeeFromInToken(sint64 amountOut, sint64 reserveIn, sint64 reserveOut, uint32 fee) {
		// reserveIn*amountOut/(reserveOut - amountOut)*QSWAP_SWAP_FEE_BASE / (QSWAP_SWAP_FEE_BASE - fee)
		uint128 res = div(
			div(
				uint128(reserveIn) * uint128(amountOut),
				uint128(reserveOut - amountOut)
			) * uint128(QSWAP_SWAP_FEE_BASE),
			uint128(QSWAP_SWAP_FEE_BASE - fee)
		);
		return sint64(res.low) + 1;
	}

	// (reserveIn + amountIn) * (reserveOut - x) = reserveIn * reserveOut
	// x = reserveOut * amountIn / (reserveIn + amountIn)
	inline static sint64 getAmountOutTakeFeeFromOutToken(sint64 amountIn, sint64 reserveIn, sint64 reserveOut, uint32 fee) {
		uint128 numerator = uint128(reserveOut) * uint128(amountIn);
		uint128 denominator = uint128(reserveIn + amountIn);

		// (numerator / denominator).low);
		return sint64(div(numerator, denominator).low);
	}

	// (reserveIn + x) * (reserveOut - amountOut/(1 - fee)) = reserveIn * reserveOut
	// x = (reserveIn * amountOut ) / (reserveOut * (1-fee) - amountOut)
	inline static sint64 getAmountInTakeFeeFromOutToken(sint64 amountOut, sint64 reserveIn, sint64 reserveOut, uint32 fee) {
		uint128 numerator = uint128(reserveIn) * uint128(amountOut);
		uint128 denominator = uint128(reserveOut) * uint128(QSWAP_SWAP_FEE_BASE - fee) / uint128(QSWAP_SWAP_FEE_BASE) - uint128(amountOut);

		// (numerator / denominator).low);
		return sint64(div(numerator, denominator).low) + 1;
	}

//
// functions
//
public:
	struct SwapFee_input{};
	struct SwapFee_output{
		uint32 fee;
	};

	PUBLIC_FUNCTION(SwapFee)
		output.fee = state.swapFeeRate;
	_

public:
	struct GetPoolBasicState_input{
		id assetIssuer;
		uint64 assetName;
	};
	struct GetPoolBasicState_output{
		bool poolExists;
		sint64 reservedQuAmount;
		sint64 reservedAssetAmount;
		sint64 totalLiqudity;
	};

protected:
	struct GetPoolBasicState_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		uint32 i0;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetPoolBasicState)
		output.poolExists = false;
		output.totalLiqudity = -1;
		output.reservedAssetAmount = -1;
		output.reservedQuAmount = -1;

		// asset not issued
		if (!qpi.isAssetIssued(input.assetIssuer, input.assetName)){
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = NULL_INDEX;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0++){
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == NULL_INDEX){
			return;
		}

		output.poolExists = true;

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		output.reservedQuAmount = locals.poolBasicState.reservedQuAmount;
		output.reservedAssetAmount = locals.poolBasicState.reservedAssetAmount;
		output.totalLiqudity = locals.poolBasicState.totalLiqudity;
	_

public:
	struct GetLiqudityOf_input{
		id assetIssuer;
		uint64 assetName;
		id account;
	};
	struct GetLiqudityOf_output{
		sint64 liqudity;
	};

protected:
	struct GetLiqudityOf_locals{
		id poolID;
		sint64 liqElementIndex;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(GetLiqudityOf)
		output.liqudity = 0;

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.liqElementIndex = state.mLiquditys.headIndex(locals.poolID, 0);

		while (locals.liqElementIndex != NULL_INDEX) {
			if (state.mLiquditys.element(locals.liqElementIndex).entity == input.account) {
				output.liqudity = state.mLiquditys.element(locals.liqElementIndex).liqudity;
				return;
			}
			locals.liqElementIndex = state.mLiquditys.nextElementIndex(locals.liqElementIndex);
		}
	_

public:
	struct QuoteExactQuInput_input{
		id assetIssuer;
		uint64 assetName;
		sint64 quAmountIn;
	};
	struct QuoteExactQuInput_output{
		sint64 assetAmountOut;
	};

	struct QuoteExactQuInput_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint32 i0;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactQuInput)
		output.assetAmountOut = -1;

		if (input.quAmountIn <= 0) {
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1) {
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liqudity in the pool
		if (locals.poolBasicState.totalLiqudity == 0) {
			return;
		}

		output.assetAmountOut = getAmountOutTakeFeeFromInToken(
			input.quAmountIn,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate
		);
	_

public:
	struct QuoteExactQuOutput_input{
		id assetIssuer;
		uint64 assetName;
		sint64 quAmountOut;
	};
	struct QuoteExactQuOutput_output{
		sint64 assetAmountIn;
	};

protected:
	struct QuoteExactQuOutput_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint32 i0;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactQuOutput)
		output.assetAmountIn = -1;

		if (input.quAmountOut <= 0) {
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1) {
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liqudity in the pool
		if (locals.poolBasicState.totalLiqudity == 0) {
			return;
		}

		if (input.quAmountOut >= locals.poolBasicState.reservedQuAmount){
			return;
		}

		output.assetAmountIn = getAmountInTakeFeeFromOutToken(
			input.quAmountOut,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate
		);
	_

public:
	struct QuoteExactAssetInput_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountIn;
	};
	struct QuoteExactAssetInput_output{
		sint64 quAmountOut;
	};

protected:
	struct QuoteExactAssetInput_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountOutWithFee;

		uint32 i0;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactAssetInput)
		output.quAmountOut = -1;

		if (input.assetAmountIn <= 0) {
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1) {
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liqudity in the pool
		if (locals.poolBasicState.totalLiqudity == 0) {
			return;
		}

		locals.quAmountOutWithFee = getAmountOutTakeFeeFromOutToken(
			input.assetAmountIn,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate
		);

		output.quAmountOut = sint64((
			uint128(locals.quAmountOutWithFee) * 
			uint128(QSWAP_SWAP_FEE_BASE - state.swapFeeRate) / 
			uint128(QSWAP_SWAP_FEE_BASE)).low
		);
	_

public:
	struct QuoteExactAssetOutput_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountOut;
	};
	struct QuoteExactAssetOutput_output{
		sint64 quAmountIn;
	};

protected:
	struct QuoteExactAssetOutput_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;

		uint32 i0;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(QuoteExactAssetOutput)
		output.quAmountIn = -1;

		if (input.assetAmountOut <= 0) {
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1) {
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// no liqudity in the pool
		if (locals.poolBasicState.totalLiqudity == 0) {
			return;
		}

		if (input.assetAmountOut >= locals.poolBasicState.reservedAssetAmount) {
			return;
		}

		output.quAmountIn = getAmountInTakeFeeFromInToken(
			input.assetAmountOut,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate
		);
	_

//
// procudue
// 
public:
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
	struct IssueAsset_locals{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(IssueAsset)
		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

		output.issuedNumberOfShares = 0;
		if ((qpi.invocationReward() < locals.feesOutput.assetIssuanceFee)) {
			if (qpi.invocationReward() > 0) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// check the validity of input
		if ((input.numberOfShares <= 0) || (input.numberOfDecimalPlaces < 0)){
			if (qpi.invocationReward() > 0) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// asset already issued 
		if (qpi.isAssetIssued(qpi.invocator(), input.assetName)) {
			if (qpi.invocationReward() > 0 ) {
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

		if (output.issuedNumberOfShares == 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		} else if (qpi.invocationReward() > locals.feesOutput.assetIssuanceFee ){
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.feesOutput.assetIssuanceFee);
			state.protocolEarnedFee += locals.feesOutput.assetIssuanceFee;
		}
	_

public:
	struct CreatePool_input {
		id assetIssuer;
		uint64 assetName;
	};
	struct CreatePool_output {
		bool success;
	};

protected:
	struct CreatePool_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		uint32 i0, i1;
	};

	// create uniswap like pool
	// TODO: reject if there is no shares avaliabe shares in current contract, e.g. asset is issue in contract qx
	PUBLIC_PROCEDURE_WITH_LOCALS(CreatePool)
		output.success = false;

		// fee check
		if(qpi.invocationReward() < state.poolCreateFee ) {
			if(qpi.invocationReward() > 0) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// asset no exist
		if (!qpi.isAssetIssued(qpi.invocator(), input.assetName)) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		// check if pool already exist
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL ; locals.i0 ++ ){
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}
		}

		// find an vacant pool slot
		locals.poolSlot = -1;
		for (locals.i1 = 0; locals.i1 < QSWAP_MAX_POOL; locals.i1 ++) {
			if (state.mPoolBasicStates.get(locals.i1).poolID == id(0,0,0,0)) {
				locals.poolSlot = locals.i1;
				break;
			}
		}

		// no available solt for new pool 
		if (locals.poolSlot == -1) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState.poolID = locals.poolID;
		locals.poolBasicState.reservedAssetAmount = 0;
		locals.poolBasicState.reservedQuAmount = 0;
		locals.poolBasicState.totalLiqudity = 0;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		if(qpi.invocationReward() > state.poolCreateFee) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.poolCreateFee );
		}
		state.protocolEarnedFee += state.poolCreateFee;

		output.success = true;
	_


	/** 
	* @param	quAmountADesired		The amount of tokenA to add as liquidity if the B/A price is <= amountBDesired/amountADesired (A depreciates).
	* @param	assetAmountBDesired		The amount of tokenB to add as liquidity if the A/B price is <= amountADesired/amountBDesired (B depreciates).
	* @param	quAmountMin				Bounds the extent to which the B/A price can go up before the transaction reverts. Must be <= amountADesired.
	* @param	assetAmountMin			Bounds the extent to which the A/B price can go up before the transaction reverts. Must be <= amountBDesired.
	* https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#addliquidity
	*/
	struct AddLiqudity_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountDesired;
		sint64 quAmountMin;
		sint64 assetAmountMin;
	};
	struct AddLiqudity_output{
		sint64 userIncreaseLiqudity;
		sint64 quAmount;
		sint64 assetAmount;
	};

	struct AddLiqudity_locals
	{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		LiqudityInfo tmpLiqudity;

		sint64 userLiqudityElementIndex;
		sint64 quAmountDesired;

		sint64 quTransferAmount;
		sint64 assetTransferAmount;
		sint64 quOptimalAmount;
		sint64 assetOptimalAmount;
		sint64 increaseLiqudity;
		sint64 reservedAssetAmountBefore;
		sint64 reservedAssetAmountAfter;

		uint32 i0;
	};

	// https://github.com/Uniswap/v2-periphery/blob/0335e8f7e1bd1e8d8329fd300aea2ef2f36dd19f/contracts/UniswapV2Router02.sol#L61
	PUBLIC_PROCEDURE_WITH_LOCALS(AddLiqudity)
		output.userIncreaseLiqudity = 0;
		output.assetAmount = 0;
		output.quAmount = 0;

		// add liqudity must stake both qu and asset
		if (qpi.invocationReward() <= 0) {
			return;
		}

		locals.quAmountDesired = qpi.invocationReward();

		// check the vadility of input params
		if ((input.assetAmountDesired <= 0) ||
			(input.quAmountMin < 0) ||
			(input.assetAmountMin < 0)
		){
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		// check the pool existance
		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check if pool state meet the input condition before desposit 
		// and confirm the final qu and asset amount to stake
		if (locals.poolBasicState.totalLiqudity == 0) {
			locals.quTransferAmount = locals.quAmountDesired;
			locals.assetTransferAmount = input.assetAmountDesired;
		} else {
			locals.assetOptimalAmount = quoteEquivalentAmountB(
				locals.quAmountDesired,
				locals.poolBasicState.reservedQuAmount,
				locals.poolBasicState.reservedAssetAmount
			);
			if (locals.assetOptimalAmount <= input.assetAmountDesired ){
				if (locals.assetOptimalAmount < input.assetAmountMin) {
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				locals.quTransferAmount = locals.quAmountDesired;
				locals.assetTransferAmount = locals.assetOptimalAmount;
			} else {
				locals.quOptimalAmount = quoteEquivalentAmountB(
					input.assetAmountDesired,
					locals.poolBasicState.reservedAssetAmount,
					locals.poolBasicState.reservedQuAmount
				);
				if (locals.quOptimalAmount > locals.quAmountDesired) {
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				if (locals.quOptimalAmount < input.quAmountMin){
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
					return ;
				}
				locals.quTransferAmount = locals.quOptimalAmount;
				locals.assetTransferAmount = input.assetAmountDesired;
			}
		}

		// check if the qu is enough
		if (qpi.invocationReward() < locals.quTransferAmount){
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
		if (locals.poolBasicState.totalLiqudity == 0) {
			locals.increaseLiqudity = sqrt(locals.quTransferAmount, locals.assetTransferAmount);

			if (locals.increaseLiqudity < QSWAP_MIN_LIQUDITY ){
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

			if (locals.reservedAssetAmountAfter - locals.reservedAssetAmountBefore < locals.assetTransferAmount) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			// permanently lock the first MINIMUM_LIQUIDITY tokens
			locals.tmpLiqudity.entity = SELF;
			locals.tmpLiqudity.liqudity = QSWAP_MIN_LIQUDITY;
			state.mLiquditys.add(locals.poolID, locals.tmpLiqudity, 0);

			locals.tmpLiqudity.entity = qpi.invocator();
			locals.tmpLiqudity.liqudity = locals.increaseLiqudity - QSWAP_MIN_LIQUDITY;
			state.mLiquditys.add(locals.poolID, locals.tmpLiqudity, 0);

			output.quAmount = locals.quTransferAmount;
			output.assetAmount = locals.assetTransferAmount;
			output.userIncreaseLiqudity = locals.increaseLiqudity - QSWAP_MIN_LIQUDITY;

		} else {
			locals.increaseLiqudity = min(
				sint64(div(
					uint128(locals.quTransferAmount) * uint128(locals.poolBasicState.totalLiqudity),
					uint128(locals.poolBasicState.reservedQuAmount)
				).low),

				sint64(div(
					uint128(locals.assetTransferAmount) * uint128(locals.poolBasicState.totalLiqudity),
					uint128(locals.poolBasicState.reservedAssetAmount)
				).low)
			);

			// maybe too little input 
			if (locals.increaseLiqudity == 0) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			// find user liqudity index
			locals.userLiqudityElementIndex = state.mLiquditys.headIndex(locals.poolID, 0);
			while (locals.userLiqudityElementIndex != NULL_INDEX) {
				if(state.mLiquditys.element(locals.userLiqudityElementIndex).entity == qpi.invocator()) {
					break;
				}

				locals.userLiqudityElementIndex = state.mLiquditys.nextElementIndex(locals.userLiqudityElementIndex);
			}

			// no more space for new liqudity item
			if ((locals.userLiqudityElementIndex == NULL) && ( state.mLiquditys.population() == state.mLiquditys.capacity())) {
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
			if (locals.reservedAssetAmountAfter - locals.reservedAssetAmountBefore < locals.assetTransferAmount) {
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			if (locals.userLiqudityElementIndex == NULL_INDEX) {
				locals.tmpLiqudity.entity = qpi.invocator();
				locals.tmpLiqudity.liqudity = locals.increaseLiqudity;
				state.mLiquditys.add(locals.poolID, locals.tmpLiqudity, 0);
			} else {
				locals.tmpLiqudity = state.mLiquditys.element(locals.userLiqudityElementIndex);
				locals.tmpLiqudity.liqudity += locals.increaseLiqudity;
				state.mLiquditys.replace(locals.userLiqudityElementIndex, locals.tmpLiqudity);
			}

			output.quAmount = locals.quTransferAmount;
			output.assetAmount = locals.assetTransferAmount;
			output.userIncreaseLiqudity = locals.increaseLiqudity;
		}

		locals.poolBasicState.reservedQuAmount += locals.quTransferAmount;
		locals.poolBasicState.reservedAssetAmount += locals.assetTransferAmount;
		locals.poolBasicState.totalLiqudity += locals.increaseLiqudity;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);

		if (qpi.invocationReward() > locals.quTransferAmount) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.quTransferAmount);
		}
	_

	struct RemoveLiqudity_input{
		id assetIssuer;
		uint64 assetName;
		sint64 burnLiqudity;
		sint64 quAmountMin;
		sint64 assetAmountMin;
	};

	struct RemoveLiqudity_output {
		sint64 quAmount;
		sint64 assetAmount;
	};

	struct RemoveLiqudity_locals {
		id poolID;
		PoolBasicState poolBasicState;
		sint64 userLiqudityElementIndex;
		sint64 poolSlot;
		uint32 i0;
		LiqudityInfo userLiqudity;

		sint64 burnQuAmount;
		sint64 burnAssetAmount;
	};

	// https://github.com/Uniswap/v2-periphery/blob/0335e8f7e1bd1e8d8329fd300aea2ef2f36dd19f/contracts/UniswapV2Router02.sol#L102
	PUBLIC_PROCEDURE_WITH_LOCALS(RemoveLiqudity)
		output.quAmount = 0;
		output.assetAmount = 0;

		if (qpi.invocationReward() > 0 ) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		// check the vadility of input params
		if (input.quAmountMin < 0 || input.assetAmountMin < 0) {
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		// get the pool's basic state
		locals.poolSlot = -1;

		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		// the pool does not exsit
		if (locals.poolSlot == -1) {
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		locals.userLiqudityElementIndex = state.mLiquditys.headIndex(locals.poolID, 0);
		while (locals.userLiqudityElementIndex != NULL_INDEX) {
			if(state.mLiquditys.element(locals.userLiqudityElementIndex).entity == qpi.invocator()) {
				break;
			}

			locals.userLiqudityElementIndex = state.mLiquditys.nextElementIndex(locals.userLiqudityElementIndex);
		}

		if (locals.userLiqudityElementIndex == NULL){
			return;
		}

		locals.userLiqudity = state.mLiquditys.element(locals.userLiqudityElementIndex);

		// not enough liqudity for burning
		if (locals.userLiqudity.liqudity < input.burnLiqudity ){
			return;
		}

		if (locals.poolBasicState.totalLiqudity < input.burnLiqudity ){
			return;
		}

		locals.burnQuAmount = sint64(div(
				uint128(input.burnLiqudity) * uint128(locals.poolBasicState.reservedQuAmount),
				uint128(locals.poolBasicState.totalLiqudity)
			).low);

		locals.burnAssetAmount = sint64(div(
				uint128(input.burnLiqudity) * uint128(locals.poolBasicState.reservedAssetAmount),
				uint128(locals.poolBasicState.totalLiqudity)
			).low);


		if ((locals.burnQuAmount < input.quAmountMin) || (locals.burnAssetAmount < input.assetAmountMin)){
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

		// modify invocator's liqudity info
		locals.userLiqudity.liqudity -= input.burnLiqudity;
		if (locals.userLiqudity.liqudity == 0) {
			state.mLiquditys.remove(locals.userLiqudityElementIndex);
		} else {
			state.mLiquditys.replace(locals.userLiqudityElementIndex, locals.userLiqudity);
		}

		// modify the pool's liqudity info
		locals.poolBasicState.totalLiqudity -= input.burnLiqudity;
		locals.poolBasicState.reservedQuAmount -= locals.burnQuAmount;
		locals.poolBasicState.reservedAssetAmount -= locals.burnAssetAmount;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);
	_

	// swap_exact_qu_for_asset
	struct SwapExactQuForAsset_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountOutMin;
	};
	struct SwapExactQuForAsset_output{
		sint64 assetAmountOut;
	};
	struct SwapExactQuForAsset_locals{
		id poolID;
		sint64 poolSlot;
		sint64 quAmountIn;
		PoolBasicState poolBasicState;
		sint64 feeAmount;
		sint64 assetAmountOut;
		sint64 feeToProtocol;

		uint32 i0;
	};

	// given an input qu amountIn, only execute swap in case (amountOut >= amountOutMin)
	// https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#swapexacttokensfortokens
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapExactQuForAsset)
		output.assetAmountOut = 0;

		// require input qu > 0
		if (qpi.invocationReward() <= 0) {
			return;
		}

		if (input.assetAmountOutMin < 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.quAmountIn = qpi.invocationReward();

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check the liqudity validity 
		if (locals.poolBasicState.totalLiqudity == 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.assetAmountOut = getAmountOutTakeFeeFromInToken(
			locals.quAmountIn,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate
		);

		// not meet user's amountOut requirement
		if (locals.assetAmountOut < input.assetAmountOutMin) {
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
		if (output.assetAmountOut == 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// take fee from qu
		// quAmountIn * swapFeeRate / QSWAP_SWAP_FEE_BASE * state.protocolFeeRate / QSWAP_PROTOCOL_FEE_BASE
		locals.feeToProtocol = sint64(div(
				div(
					uint128(locals.quAmountIn) * uint128(state.swapFeeRate),
					uint128(QSWAP_SWAP_FEE_BASE)
				) * uint128(state.protocolFeeRate),
				uint128(QSWAP_PROTOCOL_FEE_BASE)
			).low);
		state.protocolEarnedFee += locals.feeToProtocol;

		locals.poolBasicState.reservedQuAmount += locals.quAmountIn - locals.feeToProtocol;
		locals.poolBasicState.reservedAssetAmount -= locals.assetAmountOut;
		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);
	_


	struct SwapQuForExactAsset_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountOut;
	};
	struct SwapQuForExactAsset_output{
		sint64 quAmountIn;
	};
	struct SwapQuForExactAsset_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountIn;
		sint64 feeToProtocol;
		sint64 transferredAssetAmount;
		uint32 i0;
	};

	// https://docs.uniswap.org/contracts/v2/reference/smart-contracts/router-02#swaptokensforexacttokens
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapQuForExactAsset)
		output.quAmountIn = 0;

		// require input qu amount > 0
		if (qpi.invocationReward() <= 0) {
			return;
		}

		// check input param validity
		if (input.assetAmountOut <= 0){
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check if there is liqudity in the poool 
		if (locals.poolBasicState.totalLiqudity == 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// check if reserved asset is enough
		if (input.assetAmountOut >= locals.poolBasicState.reservedAssetAmount) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.quAmountIn = getAmountInTakeFeeFromInToken(
			input.assetAmountOut,
			locals.poolBasicState.reservedQuAmount,
			locals.poolBasicState.reservedAssetAmount,
			state.swapFeeRate
		);

		// not enough qu amountIn
		if (locals.quAmountIn > qpi.invocationReward()) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// not meet user's amountIn limit
		if (locals.quAmountIn > qpi.invocationReward()){
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
		if (locals.transferredAssetAmount == 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		output.quAmountIn = locals.quAmountIn;
		if (qpi.invocationReward() > locals.quAmountIn) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.quAmountIn);
		}

		// update pool states
		// locals.quAmountIn * state.swapFeeRate / QSWAP_SWAP_FEE_BASE * state.protocolFeeRate / QSWAP_PROTOCOL_FEE_BASE
		locals.feeToProtocol = sint64(div(
				div(
					uint128(locals.quAmountIn) * uint128(state.swapFeeRate),
					uint128(QSWAP_SWAP_FEE_BASE)
				) * uint128(state.protocolFeeRate),
				uint128(QSWAP_PROTOCOL_FEE_BASE)
			).low);

		state.protocolEarnedFee += locals.feeToProtocol;
		locals.poolBasicState.reservedQuAmount += locals.quAmountIn - locals.feeToProtocol;
		locals.poolBasicState.reservedAssetAmount -= input.assetAmountOut;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);
	_

	struct SwapExactAssetForQu_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountIn;
		sint64 quAmountOutMin;
	};
	struct SwapExactAssetForQu_output{
		sint64 quAmountOut;
	};
	struct SwapExactAssetForQu_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 quAmountOut;
		sint64 quAmountOutWithFee;
		sint64 protocolFee;
		sint64 transferredAssetAmountBefore;
		sint64 transferredAssetAmountAfter;
		uint32 i0;
	};

	// given an amount of asset swap in, only execute swaping if quAmountOut >= input.amountOutMin
	PUBLIC_PROCEDURE_WITH_LOCALS(SwapExactAssetForQu)
		output.quAmountOut = 0;
		if (qpi.invocationReward() > 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		// check input param validity
		if ((input.assetAmountIn <= 0 )||(input.quAmountOutMin < 0)){
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check the liqudity validity 
		if (locals.poolBasicState.totalLiqudity == 0) {
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
			) < input.assetAmountIn ) {
			return;
		}

		locals.quAmountOutWithFee = getAmountOutTakeFeeFromOutToken(
			input.assetAmountIn,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate
		);

		// locals.quAmountOutWithFee * (QSWAP_SWAP_FEE_BASE - state.swapFeeRate) / QSWAP_SWAP_FEE_BASE
		locals.quAmountOut = sint64(div(
				uint128(locals.quAmountOutWithFee) * uint128(QSWAP_SWAP_FEE_BASE - state.swapFeeRate), 
				uint128(QSWAP_SWAP_FEE_BASE)
			).low);

		// locals.quAmountOutWithFee * state.swapFeeRate / QSWAP_SWAP_FEE_BASE * state.protocolFeeRate / QSWAP_PROTOCOL_FEE_BASE
		locals.protocolFee = sint64(div(
				div(
					uint128(locals.quAmountOutWithFee) * uint128(state.swapFeeRate), 
					uint128(QSWAP_SWAP_FEE_BASE)
				) * uint128(state.protocolFeeRate),
				uint128(QSWAP_PROTOCOL_FEE_BASE)
			).low
		);

		// not meet user min amountOut requirement
		if (locals.quAmountOut < input.quAmountOutMin) {
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
		if (locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore < input.assetAmountIn) {
			return;
		}

		qpi.transfer(qpi.invocator(), locals.quAmountOut);
		output.quAmountOut = locals.quAmountOut;

		// update pool states
		locals.poolBasicState.reservedAssetAmount += input.assetAmountIn;
		locals.poolBasicState.reservedQuAmount -= locals.quAmountOut;
		locals.poolBasicState.reservedQuAmount -= locals.protocolFee;

		state.protocolEarnedFee += locals.protocolFee;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);
	_

public:
	struct SwapAssetForExactQu_input{
		id assetIssuer;
		uint64 assetName;
		sint64 assetAmountInMax;
		sint64 quAmountOut;
	};
	struct SwapAssetForExactQu_output{
		sint64 assetAmountIn;
	};

protected:
	struct SwapAssetForExactQu_locals{
		id poolID;
		sint64 poolSlot;
		PoolBasicState poolBasicState;
		sint64 assetAmountIn;
		sint64 protocolFee;
		sint64 transferredAssetAmountBefore;
		sint64 transferredAssetAmountAfter;
		uint32 i0;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SwapAssetForExactQu)
		output.assetAmountIn = 0;
		if (qpi.invocationReward() > 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if ((input.assetAmountInMax <= 0 )||(input.quAmountOut <= 0)){
			return;
		}

		locals.poolID = input.assetIssuer;
		locals.poolID.u64._3 = input.assetName;

		locals.poolSlot = -1;
		for (locals.i0 = 0; locals.i0 < QSWAP_MAX_POOL; locals.i0 ++) {
			if (state.mPoolBasicStates.get(locals.i0).poolID == locals.poolID) {
				locals.poolSlot = locals.i0;
				break;
			}
		}

		if (locals.poolSlot == -1) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.poolBasicState = state.mPoolBasicStates.get(locals.poolSlot);

		// check the liqudity validity 
		if (locals.poolBasicState.totalLiqudity == 0) {
			return;
		}

		// pool does not hold enough asset
		if (input.quAmountOut >= locals.poolBasicState.reservedQuAmount) {
			return;
		}

		locals.assetAmountIn = getAmountInTakeFeeFromOutToken(
			input.quAmountOut,
			locals.poolBasicState.reservedAssetAmount,
			locals.poolBasicState.reservedQuAmount,
			state.swapFeeRate
		);
		// input.quAmountOut * state.swapFeeRate / (QSWAP_SWAP_FEE_BASE - state.swapFeeRate) * state.protocolFeeRate / QSWAP_PROTOCOL_FEE_BASE
		locals.protocolFee = sint64(div(
				div(
					uint128(input.quAmountOut) * uint128(state.swapFeeRate),
					uint128(QSWAP_SWAP_FEE_BASE - state.swapFeeRate)
				) * uint128(state.protocolFeeRate),
				uint128(QSWAP_PROTOCOL_FEE_BASE)
			).low
		);

		// user does not hold enough asset
		if (qpi.numberOfPossessedShares(
				input.assetName,
				input.assetIssuer,
				qpi.invocator(),
				qpi.invocator(),
				SELF_INDEX,
				SELF_INDEX
			) < locals.assetAmountIn ) {
			return;
		}

		// not meet user amountIn reqiurement 
		if (locals.assetAmountIn > input.assetAmountInMax) {
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

		if (locals.transferredAssetAmountAfter - locals.transferredAssetAmountBefore < locals.assetAmountIn) {
			return;
		}

		qpi.transfer(qpi.invocator(), input.quAmountOut);
		output.assetAmountIn = locals.assetAmountIn;

		// update pool states
		locals.poolBasicState.reservedAssetAmount += locals.assetAmountIn;
		locals.poolBasicState.reservedQuAmount -= input.quAmountOut;
		locals.poolBasicState.reservedQuAmount -= locals.protocolFee;

		state.protocolEarnedFee += locals.protocolFee;

		state.mPoolBasicStates.set(locals.poolSlot, locals.poolBasicState);
	_

public:
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
	struct TransferShareOwnershipAndPossession_locals {
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareOwnershipAndPossession)
		output.transferredAmount = 0;

		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

		if (qpi.invocationReward() < locals.feesOutput.transferFee){
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (input.amount <= 0){
			if (qpi.invocationReward() > 0 ) {
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
			) < input.amount) {

			if (qpi.invocationReward() > 0 ) {
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

		if (output.transferredAmount == 0) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		} else if (qpi.invocationReward() > locals.feesOutput.transferFee) {
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.feesOutput.transferFee);
		}

		state.protocolEarnedFee += locals.feesOutput.transferFee;
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		// functions
		REGISTER_USER_FUNCTION(SwapFee, 1);
		REGISTER_USER_FUNCTION(GetPoolBasicState, 2);
		REGISTER_USER_FUNCTION(GetLiqudityOf, 3);
		REGISTER_USER_FUNCTION(QuoteExactQuInput, 4);
		REGISTER_USER_FUNCTION(QuoteExactQuOutput, 5);
		REGISTER_USER_FUNCTION(QuoteExactAssetInput, 6);
		REGISTER_USER_FUNCTION(QuoteExactAssetOutput, 7);

		// procedure
		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(CreatePool, 3);
		REGISTER_USER_PROCEDURE(AddLiqudity, 4);
		REGISTER_USER_PROCEDURE(RemoveLiqudity, 5);
		REGISTER_USER_PROCEDURE(SwapExactQuForAsset, 6);
		REGISTER_USER_PROCEDURE(SwapQuForExactAsset, 7);
		REGISTER_USER_PROCEDURE(SwapExactAssetForQu, 8);
		REGISTER_USER_PROCEDURE(SwapAssetForExactQu, 9);
	_

	INITIALIZE
		state.swapFeeRate = 30; 	// 0.3%, must less than 10000
		state.protocolFeeRate = 20; // 20%, must less than 100
		state.poolCreateFee = 10000000;
	_

	END_TICK
		if ((div((state.protocolEarnedFee - state.distributedAmount), 676ULL) > 0) && (state.protocolEarnedFee > state.distributedAmount))
		{
			if (qpi.distributeDividends(div((state.protocolEarnedFee - state.distributedAmount), 676ULL)))
			{
				state.distributedAmount += div((state.protocolEarnedFee- state.distributedAmount), 676ULL) * NUMBER_OF_COMPUTORS;
			}
		}
	_
};
