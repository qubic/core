using namespace QPI;
#include <map>

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
	std::map<uint32, PoolInfo> pools;
	std::map<uint32, std::map<id, uint64>> QPTAmountOfUser;   // Amount of LP token that provider reserved in a pool
	std::map<uint32, uint64> totalSupply;   // value of total tokens in a pool
	std::map<uint32, uint64> TotalAmountOfQPT;   //  Amount of total LP in a pool
	uint32 NumberOfPool;						// Number of Pool

	struct StakeInfo {
		id IncentiveToken;               //  Address of Tokens for incentive
		id stakingToken;                 //  Address of bonus token
		uint64 durationOfStaking;        //  duration of staking
	};

	std::map<uint32, StakeInfo> stake;                 // information for specific staking pool
	std::map<uint32, std::map<id, uint64>> balanceOf;  // Amount of tokens that user stakes 
	std::map<uint32, uint64> AmountOfBonusToken;       // Current amount of incentive tokens
	std::map<uint32, std::map<id, uint64>> timeOfStaked;       // Timestamps that uers staked

public:

	struct CreateLiquidityPool_input {
		uint32 NumberOfToken;        // Number(maximum 5) of token in a pool
		id_8 Token;                  // Token addresses
		sint64_8 initialAmount;      // Initial amount of Tokens
		uint32_8 Weight;             // Weight of Tokens in Pool
		uint64 swapFee;              // Swap fee in a Pool
	};

	struct CreateLiquidityPool_output {
		id poolAddress;               // created pool address
	};

	struct AddLiquidity_input {
		id poolAddress;           // The address of pool for addition the liquidity 
		uint32 NumberOfToken;      // Number of tokens to be deposited
		id_8 AddressesOfToken;     // Address of tokens
		std::map<id, uint64> AmountOfTokens;            // Amount of tokens
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
	};

	PUBLIC(CreateLiquidityPool)
		if(input.NumberOfToken > 5) return;

		PoolInfo newPool;
		newPool.NumberOfToken = input.NumberOfToken;
		newPool.swapFee = input.swapFee;
		
		for(int i = 0 ; i < input.NumberOfToken; i++) {
			newPool.Token.set(i, input.Token.get(i));
			newPool.Weight.set(i, input.Weight.get(i));
			newPool.totalLiquidity.set(i, input.initialAmount.get(i));
		}
		state.pools[state.NumberOfPool] = newPool;
		state.NumberOfPool++;
	_

	PUBLIC(PoolList)
		if(input.NumberOfPool >= state.NumberOfPool) return ;
		output.NumberOfToken = state.pools[input.NumberOfPool].NumberOfToken;
		output.swapFee = state.pools[input.NumberOfPool].swapFee;
		for(int i = 0 ; i < state.pools[input.NumberOfPool].NumberOfToken; i++) {
			output.Token.set(i, state.pools[input.NumberOfPool].Token.get(i));
			output.totalLiquidity.set(i, state.pools[input.NumberOfPool].totalLiquidity.get(i));
			output.Weight.set(i, state.pools[input.NumberOfPool].Weight.get(i));
		}
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_FUNCTION(PoolList, 1);

		REGISTER_USER_PROCEDURE(CreateLiquidityPool, 1);
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