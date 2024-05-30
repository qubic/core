using namespace QPI;

#define INITIAL_QPT 1000
#define MAX_NUMBER_OF_POOL 128
#define FEE_CREATE_POOL 100000000LL
#define FEE_ISSUE_ASSET 1000000000LL

struct QPOOL2
{
};

struct QPOOL
{
private:
	struct PoolInfo {
		uint32 NumberOfToken;     //  Number(maximum 5) of token in a pool
		
		uint64 Token1;
		uint64 Token2;
		uint64 Token3;
		uint64 Token4;
		uint64 Token5;              // Token name
		uint64 totalLiquidity1;
		uint64 totalLiquidity2;
		uint64 totalLiquidity3;
		uint64 totalLiquidity4;
		uint64 totalLiquidity5;      // Initial amount of Token1
		uint32 Weight1;
		uint32 Weight2;
		uint32 Weight3;
		uint32 Weight4;
		uint32 Weight5;             // Weight of Token1 in Pool

		uint64 swapFee;           //  Swap fee in a Pool
	};
	array<PoolInfo, MAX_NUMBER_OF_POOL> pools;
	array<uint64*, MAX_NUMBER_OF_POOL> QPTAmountOfUser;   // Amount of LP token that provider reserved in a pool
	array<id*, MAX_NUMBER_OF_POOL> pool_userlist;			  // The Order of user
	uint64_128 totalSupply;   // value of total tokens in a pool
	uint64_128 TotalAmountOfQPT;   //  Amount of total LP in a pool
	uint32 NumberOfPool;						// Number of Pool

	struct StakeInfo {
		id IncentiveToken;               //  Address of Tokens for incentive
		id stakingToken;                 //  Address of bonus token
		uint64 durationOfStaking;        //  duration of staking
	};

	array<PoolInfo, MAX_NUMBER_OF_POOL> stake;                 // information for specific staking pool
	array<id_16384, MAX_NUMBER_OF_POOL> staker_list;			            // The Order of user
	array<id_16384, MAX_NUMBER_OF_POOL> balanceOf;  // Amount of tokens that user stakes 
	uint64_128 AmountOfBonusToken;       // Current amount of incentive tokens
	array<id_16384, MAX_NUMBER_OF_POOL> timeOfStaked;       // Timestamps that uers staked

public:

	struct CreateLiquidityPool_input {
		uint32 NumberOfToken;        // Number(maximum 5) of token in a pool

		uint64 Token1;
		uint64 Token2;
		uint64 Token3;
		uint64 Token4;
		uint64 Token5;              // Token name
		uint64 initialAmount1;
		uint64 initialAmount2;
		uint64 initialAmount3;
		uint64 initialAmount4;
		uint64 initialAmount5;      // Initial amount of Token1
		uint32 Weight1;
		uint32 Weight2;
		uint32 Weight3;
		uint32 Weight4;
		uint32 Weight5;             // Weight of Token1 in Pool

		uint64 swapFee;              // Swap fee in a Pool
	};

	struct CreateLiquidityPool_output {
		uint64 poolAddress;               // created pool address
	};

	struct AddLiquidity_input {
		id poolAddress;           // The address of pool for addition the liquidity 
		uint32 NumberOfToken;      // Number of tokens to be deposited
		id_8 AddressesOfToken;     // Address of tokens
		uint64_8 AmountOfTokens;            // Amount of tokens
	};

	struct AddLiquidity_output {

	};

	struct RemoveLiquidity_input {
		id poolAddress;              // The address of pool to remove the liquidity
		uint64 LPTokenAmount;        // The amount of QPT(LP in a pool)
	};

	struct RemoveLiquidity_output {

	};

	struct RemoveLiquidityForOneToken_input {
		id poolAddress;              // The address of pool to remove the liquidity
		id NeededToken;              // The address of needed token
		uint64 LPTokenAmount;        // The amount of QPT(LP in a pool)
	};

	struct RemoveLiquidityForOneToken_output {

	};

	struct MintLPTokens_input {
		id poolAddress; 
		uint64 ValueOfdeposited;   // Address of tokens to be deposited
	};

	struct MintLPTokens_output {
		uint64 AmountOfMinted;     // Amount of LP tokens which Liquidity provider receive from the Pool when providing the liquidity	
	};

	struct BurnLPTokens_input {
		id poolAddress;
		bit type;                    // Type of burning LP token(one token, all token)
		id tokenAddress;             //  Address of token for one token type
		uint64 LPTokenAmount;        // Amount of QPT tokens which should be burned
	};

	struct BurnLPTokens_output {
		uint64 returnedAmounts;
	};

	struct SwapToken_input {
		id poolAddress;
		id inputToken;       //  The address of token to be input for swapping
		id outputToken;      //  The address of token to be output after swapping
		uint64 inputAmount;  //  Amount of input token
	};

	struct SwapToken_output {
		uint64 outputAmount;
	};

	struct PoolList_input {
		uint32 NumberOfPool;
	};

	struct PoolList_output {
		uint32 NumberOfToken;     //  Number(maximum 5) of token in a pool
		
		uint64 Token1;
		uint64 Token2;
		uint64 Token3;
		uint64 Token4;
		uint64 Token5;              // Token name
		uint64 totalLiquidity1;
		uint64 totalLiquidity2;
		uint64 totalLiquidity3;
		uint64 totalLiquidity4;
		uint64 totalLiquidity5;      // Initial amount of Token1
		uint32 Weight1;
		uint32 Weight2;
		uint32 Weight3;
		uint32 Weight4;
		uint32 Weight5;             // Weight of Token1 in Pool

		uint64 swapFee;           //  Swap fee in a Pool
		uint64 totalAmountOfQPT;   // Amount of all QPT
		uint64 totalSupplyByQU;		// total supply by qu
	};

	struct IssueAsset_input
	{
		uint64 assetName;
		sint64 numberOfShares;
		sint8 numberOfDecimalPlaces;
		uint64 unitOfMeasurement;
	};
	
	struct IssueAsset_output
	{
		sint64 issuedNumberOfShares;
	};

	PUBLIC(CreateLiquidityPool)
		// if(qpi.invocationReward() < FEE_CREATE_POOL) {
		// 	if (qpi.invocationReward() > 0)
		// 	{
		// 		qpi.transfer(qpi.invocator(), qpi.invocationReward());
		// 	}
		// 	return ;
		// }
		// if(input.NumberOfToken > 5) {
		// 	if (qpi.invocationReward() > 0)
		// 	{
		// 		qpi.transfer(qpi.invocator(), qpi.invocationReward());
		// 	}
		// 	return;
		// }
		// uint32 totalWeight = input.Weight1 +  input.Weight2 +  input.Weight3 +  input.Weight4 +  input.Weight5;

		// if(totalWeight != 100 || input.Weight1 < 10 || input.Token1 != 1279350609) 
		// {
		// 	if (qpi.invocationReward() > 0)
		// 	{
		// 		qpi.transfer(qpi.invocator(), qpi.invocationReward());
		// 	}
		// 	return;
		// }

		PoolInfo newPool;
		newPool.NumberOfToken = input.NumberOfToken;
		newPool.swapFee = input.swapFee;

		newPool.Token1 = input.Token1;
		newPool.Weight1 = input.Weight1;
		newPool.totalLiquidity1 = input.initialAmount1;

		newPool.Token2 = input.Token2;
		newPool.Weight2 = input.Weight2;
		newPool.totalLiquidity2 = input.initialAmount2;

		newPool.Token3 = input.Token3;
		newPool.Weight3 = input.Weight3;
		newPool.totalLiquidity3 = input.initialAmount3;

		newPool.Token4 = input.Token4;
		newPool.Weight4 = input.Weight4;
		newPool.totalLiquidity4 = input.initialAmount4;

		newPool.Token5 = input.Token5;
		newPool.Weight5 = input.Weight5;
		newPool.totalLiquidity5 = input.initialAmount5;

		state.pools.set(state.NumberOfPool, newPool);								//   setting the pool

//		setting the list of users in a new pool
		id* listOfPool;
		listOfPool[0] = qpi.invocator();
		state.pool_userlist.set(state.NumberOfPool, listOfPool);
//
//		setting the QPT amount of first user(pool creator) in a new pool
		uint64* QPTAmountOfFirstUser;
		QPTAmountOfFirstUser[0] = INITIAL_QPT;
		state.QPTAmountOfUser.set(state.NumberOfPool, QPTAmountOfFirstUser);

//		setting the total QPT amount in a new pool
		state.TotalAmountOfQPT.set(state.NumberOfPool, INITIAL_QPT);
//		setting the total value of a new pool by QU
		state.totalSupply.set(state.NumberOfPool, qpi.invocationReward() - FEE_CREATE_POOL);
//		add the number of pool
		state.NumberOfPool++;
	_
	
	PUBLIC(IssueAsset)
		if(qpi.invocationReward() < FEE_ISSUE_ASSET) {
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.issuedNumberOfShares = 0;
			return ;
		}
		if (qpi.invocationReward() > FEE_ISSUE_ASSET)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - FEE_ISSUE_ASSET);
		}

		output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
	_

	PUBLIC(PoolList)
		if(input.NumberOfPool >= state.NumberOfPool) return ;
		PoolInfo pool;
		pool = state.pools.get(input.NumberOfPool);
		output.NumberOfToken = pool.NumberOfToken;
		output.swapFee = pool.swapFee;

		output.Token1 = pool.Token1;
		output.totalLiquidity1 = pool.totalLiquidity1;
		output.Weight1 = pool.Weight1;

		output.Token2 = pool.Token2;
		output.totalLiquidity2 = pool.totalLiquidity2;
		output.Weight2 = pool.Weight2;

		output.Token3 = pool.Token3;
		output.totalLiquidity3 = pool.totalLiquidity3;
		output.Weight3 = pool.Weight3;

		output.Token4 = pool.Token4;
		output.totalLiquidity4 = pool.totalLiquidity4;
		output.Weight4 = pool.Weight4;

		output.Token5 = pool.Token5;
		output.totalLiquidity5 = pool.totalLiquidity5;
		output.Weight5 = pool.Weight5;
		
		output.totalAmountOfQPT = state.TotalAmountOfQPT.get(input.NumberOfPool);
		output.totalSupplyByQU = state.totalSupply.get(input.NumberOfPool);
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_FUNCTION(PoolList, 1);

		REGISTER_USER_PROCEDURE(CreateLiquidityPool, 1);
		REGISTER_USER_PROCEDURE(IssueAsset, 2);
	_

	INITIALIZE
		state.NumberOfPool = 0;
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