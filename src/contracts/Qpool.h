using namespace QPI;

#define INITIAL_QPT 1000
#define MAX_NUMBER_OF_POOL 128
#define FEE_CREATE_POOL 100000000LL
#define FEE_ISSUE_ASSET 1000000LL

struct QPOOL2
{
};

struct QPOOL
{
private:
	struct PoolInfo {
		uint32 NumberOfToken;     //  Number(maximum 5) of token in a pool
		id_8 Token;               //  Address of Tokens in a Pool
		uint32_8 Weight;          //  Weight of each Token in a Pool
		uint64_8 totalLiquidity;  //  TotalLiquidity of each token in a Pool
		uint64 swapFee;           //  Swap fee in a Pool
	};
	array<PoolInfo, MAX_NUMBER_OF_POOL> pools;
	array<uint64*, MAX_NUMBER_OF_POOL> QPTAmountOfUser;   // Amount of LP token that provider reserved in a pool
	array<id_16384, MAX_NUMBER_OF_POOL> pool_userlist;			  // The Order of user
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
		id_8 Token;                  // Token addresses
		uint64_8 initialAmount;      // Initial amount of Tokens
		uint32_8 Weight;             // Weight of Tokens in Pool
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
		uint32 NumberOfToken;
		id_8 Token;
		uint32_8 Weight;
		uint64_8 totalLiquidity;
		uint64 swapFee;
		uint64 totalAmountOfQPT;
		uint64 totalSupplyByQU;
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

	PUBLIC(CreateLiquidityPool)
		if(qpi.invocationReward() < FEE_CREATE_POOL) {
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}
		if(input.NumberOfToken > 5) {
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}
		uint32 totalWeight = 0;
		for(uint32 i = 0 ; i < input.NumberOfToken; i++) {
			totalWeight += input.Weight.get(i);
		}

		if(totalWeight != 100) 
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		PoolInfo newPool;
		newPool.NumberOfToken = input.NumberOfToken;
		newPool.swapFee = input.swapFee;
		
		for(uint32 i = 0 ; i < input.NumberOfToken; i++) {
			newPool.Token.set(i, input.Token.get(i));
			newPool.Weight.set(i, input.Weight.get(i));
			newPool.totalLiquidity.set(i, input.initialAmount.get(i));
		}
		state.pools.set(state.NumberOfPool, newPool);								//   setting the pool

//		setting the list of users in a new pool
		id_16384 listOfPool;
		listOfPool.set(0, qpi.invocator());
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
		for(uint32 i = 0 ; i < pool.NumberOfToken; i++) {
			output.Token.set(i, pool.Token.get(i));
			output.totalLiquidity.set(i, pool.totalLiquidity.get(i));
			output.Weight.set(i, pool.Weight.get(i));
		}
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