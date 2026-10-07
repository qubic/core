#define NO_UEFI

#include "contract_testing.h"

//#define PRINT_DETAILS 0

static constexpr uint64 QSWAP_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QSWAP_TRANSFER_ASSET_FEE = 10000000ull;
static constexpr uint64 QSWAP_CREATE_POOL_FEE = 1000000000ull;

static const id QSWAP_CONTRACT_ID(QSWAP_CONTRACT_INDEX, 0, 0, 0);

//constexpr uint32 SWAP_FEE_IDX = 1;
constexpr uint32 GET_POOL_BASIC_STATE_IDX = 2;
constexpr uint32 GET_LIQUIDITY_OF_IDX = 3;
constexpr uint32 QUOTE_EXACT_QU_INPUT_IDX = 4;
constexpr uint32 QUOTE_EXACT_QU_OUTPUT_IDX = 5;
constexpr uint32 QUOTE_EXACT_ASSET_INPUT_IDX = 6;
constexpr uint32 QUOTE_EXACT_ASSET_OUTPUT_IDX = 7;
constexpr uint32 INVEST_REWARDS_INFO_IDX = 8;
//
constexpr uint32 ISSUE_ASSET_IDX = 1;
constexpr uint32 TRANSFER_SHARE_OWNERSHIP_AND_POSSESSION_IDX = 2;
constexpr uint32 CREATE_POOL_IDX = 3;
constexpr uint32 ADD_LIQUIDITY_IDX = 4;
constexpr uint32 REMOVE_LIQUIDITY_IDX = 5;
constexpr uint32 SWAP_EXACT_QU_FOR_ASSET_IDX = 6;
constexpr uint32 SWAP_QU_FOR_EXACT_ASSET_IDX = 7;
constexpr uint32 SWAP_EXACT_ASSET_FOR_QU_IDX = 8;
constexpr uint32 SWAP_ASSET_FOR_EXACT_QU_IDX = 9;
constexpr uint32 SET_INVEST_REWARDS_INFO_IDX = 10;
constexpr uint32 TRANSFER_SHARE_MANAGEMENT_RIGHTS_IDX = 11;


class QswapChecker : public QSWAP, public QSWAP::StateData
{
public:
	static const LiquidityKeyInput& packLiquidityKey(const id& poolID, const id& account, LiquidityKeyInput& input)
	{
		return liquidityKeyInput(poolID, account, input);
	}
};


class ContractTestingQswap : protected ContractTesting
{
public:
    ContractTestingQswap()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QSWAP);
        callSystemProcedure(QSWAP_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

	QswapChecker* getState()
	{
		return (QswapChecker*)contractStates[QSWAP_CONTRACT_INDEX];
	}

	QSWAP::StateData* stateData()
	{
		return (QSWAP::StateData*)contractStates[QSWAP_CONTRACT_INDEX];
	}

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QSWAP_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

	bool loadState(const CHAR16* filename)
	{
		return load(filename, sizeof(QSWAP), contractStates[QSWAP_CONTRACT_INDEX]) == sizeof(QSWAP);
	}

	QSWAP::InvestRewardsInfo_output investRewardsInfo()
	{
		QSWAP::InvestRewardsInfo_input input{};
		QSWAP::InvestRewardsInfo_output output;
		callFunction(QSWAP_CONTRACT_INDEX, INVEST_REWARDS_INFO_IDX, input, output);
		return output;
	}

	bool setInvestRewardsInfo(const id& issuer, QSWAP::SetInvestRewardsInfo_input input)
	{
		QSWAP::SetInvestRewardsInfo_output output;
		invokeUserProcedure(QSWAP_CONTRACT_INDEX, SET_INVEST_REWARDS_INFO_IDX, input, output, issuer, 0);
		return output.success;
	}

	sint64 issueAsset(const id& issuer, QSWAP::IssueAsset_input input)
    {
		QSWAP::IssueAsset_output output;
		invokeUserProcedure(QSWAP_CONTRACT_INDEX, ISSUE_ASSET_IDX, input, output, issuer, QSWAP_ISSUE_ASSET_FEE);
		return output.issuedNumberOfShares;
	}

    sint64 transferAsset(const id& issuer, QSWAP::TransferShareOwnershipAndPossession_input input)
    {
        QSWAP::TransferShareOwnershipAndPossession_output output;
		invokeUserProcedure(QSWAP_CONTRACT_INDEX, TRANSFER_SHARE_OWNERSHIP_AND_POSSESSION_IDX, input, output, issuer, QSWAP_TRANSFER_ASSET_FEE);
        return output.transferredAmount;
    }

	bool createPool(const id& issuer, uint64 assetName)
    {
		QSWAP::CreatePool_input input{issuer, assetName};
		QSWAP::CreatePool_output output;
		invokeUserProcedure(QSWAP_CONTRACT_INDEX, CREATE_POOL_IDX, input, output, issuer, QSWAP_CREATE_POOL_FEE);
		return output.success;
	}

	QSWAP::GetPoolBasicState_output getPoolBasicState(const id& issuer, uint64 assetName)
    {
		QSWAP::GetPoolBasicState_input input{issuer, assetName};
		QSWAP::GetPoolBasicState_output output;

		callFunction(QSWAP_CONTRACT_INDEX, GET_POOL_BASIC_STATE_IDX, input, output);
		return output;
	}

	QSWAP::AddLiquidity_output addLiquidity(const id& issuer, QSWAP::AddLiquidity_input input, uint64 inputValue)
    {
		QSWAP::AddLiquidity_output output;
		invokeUserProcedure(
			QSWAP_CONTRACT_INDEX,
			ADD_LIQUIDITY_IDX,
			input,
			output,
			issuer,
			inputValue
		);
		return output;
	}

	QSWAP::RemoveLiquidity_output removeLiquidity(const id& issuer, QSWAP::RemoveLiquidity_input input, uint64 inputValue)
    {
		QSWAP::RemoveLiquidity_output output;
		invokeUserProcedure(
			QSWAP_CONTRACT_INDEX,
			REMOVE_LIQUIDITY_IDX,
			input,
			output,
			issuer,
			inputValue
		);
		return output;
	}

	QSWAP::GetLiquidityOf_output getLiquidityOf(QSWAP::GetLiquidityOf_input input)
    {
		QSWAP::GetLiquidityOf_output output;
		callFunction(QSWAP_CONTRACT_INDEX, GET_LIQUIDITY_OF_IDX, input, output);
		return output;
	}

	QSWAP::SwapExactQuForAsset_output swapExactQuForAsset( const id& issuer, QSWAP::SwapExactQuForAsset_input input, uint64 inputValue)
    {
		QSWAP::SwapExactQuForAsset_output output;
		invokeUserProcedure(
			QSWAP_CONTRACT_INDEX,
			SWAP_EXACT_QU_FOR_ASSET_IDX,
			input,
			output,
			issuer,
			inputValue
		);

		return output;
	}

	QSWAP::SwapQuForExactAsset_output swapQuForExactAsset( const id& issuer, QSWAP::SwapQuForExactAsset_input input, uint64 inputValue)
    {
		QSWAP::SwapQuForExactAsset_output output;
		invokeUserProcedure(
			QSWAP_CONTRACT_INDEX,
			SWAP_QU_FOR_EXACT_ASSET_IDX,
			input,
			output,
			issuer,
			inputValue
		);

		return output;
	}

	QSWAP::SwapExactAssetForQu_output swapExactAssetForQu(const id& issuer, QSWAP::SwapExactAssetForQu_input input, uint64 inputValue) 
    {
		QSWAP::SwapExactAssetForQu_output output;
		invokeUserProcedure(
			QSWAP_CONTRACT_INDEX,
			SWAP_EXACT_ASSET_FOR_QU_IDX,
			input,
			output,
			issuer,
			inputValue
		);

		return output;
	}

	QSWAP::SwapAssetForExactQu_output swapAssetForExactQu(const id& issuer, QSWAP::SwapAssetForExactQu_input input, uint64 inputValue) 
    {
		QSWAP::SwapAssetForExactQu_output output;
		invokeUserProcedure(
			QSWAP_CONTRACT_INDEX,
			SWAP_ASSET_FOR_EXACT_QU_IDX,
			input,
			output,
			issuer,
			inputValue
		);

		return output;
	}

    QSWAP::TransferShareManagementRights_output transferShareManagementRights(const id& invocator, QSWAP::TransferShareManagementRights_input input, uint64 inputValue)
    {
        QSWAP::TransferShareManagementRights_output output;
        invokeUserProcedure(QSWAP_CONTRACT_INDEX, TRANSFER_SHARE_MANAGEMENT_RIGHTS_IDX, input, output, invocator, inputValue);
        return output;
    }

    QSWAP::QuoteExactQuInput_output quoteExactQuInput(QSWAP::QuoteExactQuInput_input input) 
    {
		QSWAP::QuoteExactQuInput_output output;
		callFunction(QSWAP_CONTRACT_INDEX, QUOTE_EXACT_QU_INPUT_IDX, input, output);
		return output;
    }

    QSWAP::QuoteExactQuOutput_output quoteExactQuOutput(QSWAP::QuoteExactQuOutput_input input) 
    {
		QSWAP::QuoteExactQuOutput_output output;
		callFunction(QSWAP_CONTRACT_INDEX, QUOTE_EXACT_QU_OUTPUT_IDX, input, output);
		return output;
    }

    QSWAP::QuoteExactAssetInput_output quoteExactAssetInput(QSWAP::QuoteExactAssetInput_input input)
    {
		QSWAP::QuoteExactAssetInput_output output;
		callFunction(QSWAP_CONTRACT_INDEX, QUOTE_EXACT_ASSET_INPUT_IDX, input, output);
		return output;
    }

    QSWAP::QuoteExactAssetOutput_output quoteExactAssetOutput(QSWAP::QuoteExactAssetOutput_input input)
    {
		QSWAP::QuoteExactAssetOutput_output output;
		callFunction(QSWAP_CONTRACT_INDEX, QUOTE_EXACT_ASSET_OUTPUT_IDX, input, output);
		return output;
    }
};

TEST(ContractSwap, InvestRewardsInfoTest)
{
	ContractTestingQswap qswap;

	{
		QSWAP::InvestRewardsInfo_output info = qswap.investRewardsInfo();

		auto expectIdentity = (const unsigned char*)"VJGRUFWJCUSNHCQJRWRRYXAUEJFCVHYPXWKTDLYKUACPVVYBGOLVCJSF";
		m256i expectPubkey;
		getPublicKeyFromIdentity(expectIdentity, expectPubkey.m256i_u8);
		EXPECT_EQ(info.investRewardsId, expectPubkey);
		EXPECT_EQ(info.investRewardsFee, 3);
	}

	{
		id newInvestRewardsId(6,6,6,6);
		QSWAP::SetInvestRewardsInfo_input input = {newInvestRewardsId};

		id invalidIssuer(1,2,3,4);

		increaseEnergy(invalidIssuer, 100);
		bool res1 = qswap.setInvestRewardsInfo(invalidIssuer, input);
		// printf("res1: %d\n", res1);
		EXPECT_FALSE(res1);

		auto investRewardsIdentity = (const unsigned char*)"VJGRUFWJCUSNHCQJRWRRYXAUEJFCVHYPXWKTDLYKUACPVVYBGOLVCJSF";
		m256i investRewardsPubkey;
		getPublicKeyFromIdentity(investRewardsIdentity, investRewardsPubkey.m256i_u8);

		increaseEnergy(investRewardsPubkey, 100);
		bool res2 = qswap.setInvestRewardsInfo(investRewardsPubkey, input);
		// printf("res2: %d\n", res2);
		EXPECT_TRUE(res2);

		QSWAP::InvestRewardsInfo_output info = qswap.investRewardsInfo();
		EXPECT_EQ(info.investRewardsId, newInvestRewardsId);
		// printf("%d\n", info.investRewardsId == newInvestRewardsId);
	}
}

TEST(ContractSwap, QuoteTest)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    sint64 numberOfShares = 10000 * 1000;

    // issue an asset and create a pool, and init liquidity
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));

        sint64 inputValue = 30*1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 30*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);

        QSWAP::QuoteExactQuInput_input qi_input = {issuer, assetName, 1000};
        QSWAP::QuoteExactQuInput_output qi_output = qswap.quoteExactQuInput(qi_input);
        // printf("quote exact qu input: %lld\n", qi_output.assetAmountOut);
        EXPECT_EQ(qi_output.assetAmountOut, 964);

        QSWAP::QuoteExactQuOutput_input qo_input = {issuer, assetName, 1000};
        QSWAP::QuoteExactQuOutput_output qo_output = qswap.quoteExactQuOutput(qo_input);
        // printf("quote exact qu output: %lld\n", qo_output.assetAmountIn);
        EXPECT_EQ(qo_output.assetAmountIn, 1038);

        QSWAP::QuoteExactAssetInput_input ai_input = {issuer, assetName, 1000};
        QSWAP::QuoteExactAssetInput_output ai_output = qswap.quoteExactAssetInput(ai_input);
        // printf("quote exact asset input: %lld\n", ai_output.quAmountOut);
        EXPECT_EQ(ai_output.quAmountOut, 964);

        QSWAP::QuoteExactAssetOutput_input ao_input = {issuer, assetName, 1000};
        QSWAP::QuoteExactAssetOutput_output ao_output = qswap.quoteExactAssetOutput(ao_input);
        // printf("quote exact asset output: %lld\n", ao_output.quAmountIn);
        EXPECT_EQ(ao_output.quAmountIn, 1038);
    }
}

/*
0. normally issue asset
1. not enough qu for asset issue fee
2. issue duplicate asset
3. issue asset with invalid input params, such as numberOfShares: 0
*/
TEST(ContractSwap, IssueAssetAndTransferShareManagementRights)
{
    ContractTestingQswap qswap;
    qswap.beginEpoch();
    system.epoch = 200;

    id issuer(1, 2, 3, 4);

    // 0. normally issue asset and transfer
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        uint64 assetName = assetNameFromString("QSWAP0");
        sint64 numberOfShares = 1000000;
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), 0);
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);
        EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), QSWAP_ISSUE_ASSET_FEE);

        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        sint64 transferAmount = 1000;
        id newId(2,3,4,5);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, newId, newId, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), 0);
        QSWAP::TransferShareOwnershipAndPossession_input ts_input = {issuer, assetName, newId, transferAmount};
		// printf("ts amount: %lld\n", transferAmount);
        EXPECT_EQ(qswap.transferAsset(issuer, ts_input), transferAmount);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, newId, newId, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), transferAmount);
        // printf("%lld\n", getBalance(QSWAP_CONTRACT_ID));
        increaseEnergy(issuer, 100);
        uint64 qswapIdBalance = getBalance(QSWAP_CONTRACT_ID);
        uint64 issuerBalance = getBalance(issuer);
        QSWAP::TransferShareManagementRights_input tsr_input = {Asset{issuer, assetName}, transferAmount, QX_CONTRACT_INDEX};
        EXPECT_EQ(qswap.transferShareManagementRights(issuer, tsr_input, 100).transferredNumberOfShares, transferAmount);
        EXPECT_EQ(getBalance(id(QX_CONTRACT_INDEX, 0, 0, 0)), 100);
        EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), qswapIdBalance);
        EXPECT_EQ(getBalance(issuer), issuerBalance - 100);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), transferAmount);
    }

    // 1. not enough energy for asset issue fee
    {
        decreaseEnergy(spectrumIndex(issuer), getBalance(issuer));
        uint64 assetName = assetNameFromString("QSWAP1");
        sint64 numberOfShares = 1000000;
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), 0);
    }

    // 2. issue duplicate asset, related to test.0
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        uint64 assetName = assetNameFromString("QSWAP0");
        sint64 numberOfShares = 1000000;
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), 0);
    }

    // 3. issue asset with invalid input params, such as numberOfShares: 0
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        uint64 assetName = assetNameFromString("QSWAP1");
        sint64 numberOfShares = 0;
        QSWAP::IssueAsset_input input = {assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), 0);
    }
}

TEST(ContractSwap, SwapExactQuForAsset)
{
	ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    sint64 numberOfShares = 10000 * 1000;

	// issue an asset and create a pool, and init liquidity
	{
		increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
		QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
		EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
		EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

		increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
		EXPECT_TRUE(qswap.createPool(issuer, assetName));

        sint64 inputValue = 200*1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);
    }

    {
        // swap in 100*1000 qu, get about 1000*50 asset
        id user(2,3,4,5);
        sint64 inputValue = 200*1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(user, inputValue);

        QSWAP::QuoteExactQuInput_input qi_input = {issuer, assetName, inputValue - QSWAP_ADDITIONAL_FEE};
        QSWAP::QuoteExactQuInput_output qi_output = qswap.quoteExactQuInput(qi_input);
        // printf("quote_exact_qu_input, asset out: %lld\n", qi_output.assetAmountOut);

        QSWAP::SwapExactQuForAsset_input input = {issuer, assetName, 0};
        QSWAP::SwapExactQuForAsset_output output = qswap.swapExactQuForAsset(user, input, inputValue);
        // printf("swap_exact_qu_for_asset, asset out: %lld\n", output.assetAmountOut);

        EXPECT_EQ(qi_output.assetAmountOut, output.assetAmountOut); // 49924

        EXPECT_TRUE(output.assetAmountOut <= 50000); // 49924 if swapFee 0.3%

        QSWAP::GetPoolBasicState_output psOutput = qswap.getPoolBasicState(issuer, assetName);
        // printf("%lld, %lld, %lld\n", psOutput.reservedAssetAmount, psOutput.reservedQuAmount, psOutput.totalLiquidity);
		// swapFee is 200_000 * 0.3% = 600, shareholders 27%: 162, QX 5%: 30, invest&rewards 3%: 18, burn 1%: 6 = 216
		EXPECT_TRUE(psOutput.reservedQuAmount >= 399784); // 399784 = (400_000 - 216)
        EXPECT_TRUE(psOutput.reservedAssetAmount >= 50000 ); // 50076
        EXPECT_EQ(psOutput.totalLiquidity, 141421); // liquidity stay the same
    }
}

TEST(ContractSwap, SwapQuForExactAsset)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    sint64 numberOfShares = 10000 * 1000;

    // issue an asset and create a pool, and init liquidity
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));

        sint64 inputValue = 200*1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);
    }

    {
        id user(2,3,4,5);
        sint64 inputValue = 1000 * 200 + QSWAP_ADDITIONAL_FEE;
        sint64 expectQuAmountIn = 22289;
        sint64 assetAmountOut = 10 * 1000;
        increaseEnergy(user, inputValue);

        QSWAP::QuoteExactAssetOutput_input ao_input = {issuer, assetName, assetAmountOut};
        QSWAP::QuoteExactAssetOutput_output ao_output = qswap.quoteExactAssetOutput(ao_input);
        // printf("quote_exact_asset_output, qu in %lld\n", ao_output.quAmountIn);

        QSWAP::SwapQuForExactAsset_input input = {issuer, assetName, assetAmountOut};
        QSWAP::SwapQuForExactAsset_output output = qswap.swapQuForExactAsset(user, input, inputValue);

        EXPECT_EQ(ao_output.quAmountIn, output.quAmountIn); // 22289

        // EXPECT_EQ(output.quAmountIn, 22289);
        // printf("swap_qu_for_exact_asset, asset in: %lld\n", output.quAmountIn);
    }
}

TEST(ContractSwap, SwapExactAssetForQu)
{
    ContractTestingQswap qswap;
    system.epoch = 200;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    sint64 numberOfShares = 10000 * 1000;

    // issue an asset and create a pool, and init liquidity
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));

        sint64 inputValue = 200*1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);
    }

    {
        id user(1, 2,3,4);
        sint64 inputValue = QSWAP_ADDITIONAL_FEE;
        sint64 assetAmountIn = 100*1000;
        sint64 expectQuAmountOut = 99700;
        increaseEnergy(user, inputValue);

        QSWAP::QuoteExactAssetInput_input ai_input = {issuer, assetName, assetAmountIn};
        QSWAP::QuoteExactAssetInput_output ai_output = qswap.quoteExactAssetInput(ai_input);
        // printf("quote exact asset input: %lld\n", ai_output.quAmountOut);

        QSWAP::SwapExactAssetForQu_input input = {issuer, assetName, assetAmountIn, 0};
        QSWAP::SwapExactAssetForQu_output output = qswap.swapExactAssetForQu(user, input, inputValue);
        // printf("swap qu out: %lld\n", output.quAmountOut);
        EXPECT_EQ(ai_output.quAmountOut, output.quAmountOut); // 99700
    }
}

TEST(ContractSwap, SwapAssetForExactQu)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    sint64 numberOfShares = 10000 * 1000;

    // issue an asset and create a pool, and init liquidity
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));

        sint64 inputValue = 200*1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);

        // QSWAP::GetPoolBasicState_output gp_output = qswap.getPoolBasicState(issuer, assetName);
        // printf("%lld, %lld, %lld\n", gp_output.reservedQuAmount, gp_output.reservedAssetAmount, gp_output.totalLiquidity);
    }

    {
       id user(1,2,3,4);
       sint64 inputValue = 0;
       sint64 quAmountOut = 200*1000 - 1;

       QSWAP::QuoteExactQuOutput_input qo_input = {issuer, assetName, quAmountOut};
       QSWAP::QuoteExactQuOutput_output qo_output = qswap.quoteExactQuOutput(qo_input);
       // printf("quote exact qu output: %lld\n", qo_output.assetAmountIn);
       EXPECT_EQ(qo_output.assetAmountIn, -1);
    }

    {
       id user(1,2,3,4);
       sint64 inputValue = QSWAP_ADDITIONAL_FEE;
       sint64 quAmountOut = 100*1000;
       sint64 expectAssetAmountIn = 100604;

       QSWAP::QuoteExactQuOutput_input qo_input = {issuer, assetName, quAmountOut};
       QSWAP::QuoteExactQuOutput_output qo_output = qswap.quoteExactQuOutput(qo_input);
       // printf("quote exact qu output: %lld\n", qo_output.assetAmountIn);
       EXPECT_EQ(qo_output.assetAmountIn, expectAssetAmountIn);

       increaseEnergy(user, inputValue);
       sint64 assetAmountInMax = 200*1000;
       QSWAP::SwapAssetForExactQu_input input = {issuer, assetName, assetAmountInMax, quAmountOut};
       QSWAP::SwapAssetForExactQu_output output = qswap.swapAssetForExactQu(user, input, inputValue);
       // printf("swap asset in: %lld\n", output.assetAmountIn);
       EXPECT_EQ(qo_output.assetAmountIn, output.assetAmountIn);
    }
}

/*
0. check pool state before create
1. normal create pool, check pool existance, pool states
2. create duplicate pool
3. create pool with invalid asset
*/
TEST(ContractSwap, CreatePool)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    sint64 numberOfShares = 1000000;

    // issue asset first
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = {assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);
    }

    // 0. check not exsit pool state before create
    {
        QSWAP::GetPoolBasicState_output output = qswap.getPoolBasicState(issuer, assetName);
        EXPECT_FALSE(output.poolExists);
    }

    // 1. normal create pool, check pool existance, pool states
    {
        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));

        // initial pool state
        QSWAP::GetPoolBasicState_output output = qswap.getPoolBasicState(issuer, assetName);
        EXPECT_EQ(output.poolExists, true);
        EXPECT_EQ(output.reservedQuAmount, 0);
        EXPECT_EQ(output.reservedAssetAmount, 0);
        EXPECT_EQ(output.totalLiquidity, 0);
    }

    // 2. create duplicate pool
    {
        EXPECT_FALSE(qswap.createPool(issuer, assetName));
    }

    // 3. ceate pool with not issued asset
    {
        uint64 assetName2 = assetNameFromString("QswapX");
        EXPECT_FALSE(qswap.createPool(issuer, assetName2));
    }
}

/*
add liquidity 2 times, and then remove 
*/
TEST(ContractSwap, LiqTest1)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    uint64 invalidAssetName = assetNameFromString("QSWAP1");
    sint64 numberOfShares = 1000*1000;

	// 0. issue an asset and create a pool
	{
		increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
		QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
		EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
		EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

		increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
		EXPECT_TRUE(qswap.createPool(issuer, assetName));
	}

    // 1. add liquidity to a initial pool, first time
    {
        sint64 quStakeAmount = 200*1000;
        sint64 inputValue = quStakeAmount + QSWAP_ADDITIONAL_FEE;
        sint64 assetStakeAmount = 100*1000;
        increaseEnergy(issuer, quStakeAmount);
        QSWAP::AddLiquidity_input addLiqInput = {
            issuer,
            assetName,
            assetStakeAmount,
            0,
            0
        };

        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, addLiqInput, inputValue);
        // actually, 141421 liquidity add to the pool, but the first 1000 liquidity is retainedd by the pool rather than the staker
        EXPECT_EQ(output.userIncreaseLiquidity, 140421); 
        EXPECT_EQ(output.quAmount, 200*1000);
        EXPECT_EQ(output.assetAmount, 100*1000);

        QSWAP::GetPoolBasicState_output output2 = qswap.getPoolBasicState(issuer, assetName);
        EXPECT_EQ(output2.poolExists, true);
        EXPECT_EQ(output2.reservedQuAmount, 200*1000);
        EXPECT_EQ(output2.reservedAssetAmount, 100*1000);
        EXPECT_EQ(output2.totalLiquidity, 141421);
        // printf("pool state: %lld, %lld, %lld\n", output2.reservedQuAmount, output2.reservedAssetAmount, output2.totalLiquidity);

        QSWAP::GetLiquidityOf_input getLiqInput = {
           issuer,
           assetName,
           issuer
        };
        QSWAP::GetLiquidityOf_output getLiqOutput = qswap.getLiquidityOf(getLiqInput);
        EXPECT_EQ(getLiqOutput.liquidity, 140421);

        // 2. add liquidity second time
        increaseEnergy(issuer, quStakeAmount);
        addLiqInput = {
            issuer,
            assetName,
            assetStakeAmount,
            0,
            0
        };

        QSWAP::AddLiquidity_output output3 = qswap.addLiquidity(issuer, addLiqInput, inputValue);
        EXPECT_EQ(output3.userIncreaseLiquidity, 141421);
        EXPECT_EQ(output3.quAmount, 200*1000);
        EXPECT_EQ(output3.assetAmount, 100*1000);

        getLiqOutput = qswap.getLiquidityOf(getLiqInput);
        EXPECT_EQ(getLiqOutput.liquidity,  281842); // 140421 + 141421

        QSWAP::RemoveLiquidity_input rmLiqInput = {
            issuer,
            assetName,
            141421,
            200*1000, // should lte 1000*200
            100*1000, // should lte 1000*100
        };

        // 3. remove liquidity
        increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE);
        QSWAP::RemoveLiquidity_output rmLiqOutput = qswap.removeLiquidity(issuer, rmLiqInput, QSWAP_ADDITIONAL_FEE);
        // printf("qu: %lld, asset: %lld\n", rmLiqOutput.quAmount, rmLiqOutput.assetAmount);
        EXPECT_EQ(rmLiqOutput.quAmount, 1000 * 200);
        EXPECT_EQ(rmLiqOutput.assetAmount, 1000 * 100);

        getLiqOutput = qswap.getLiquidityOf(getLiqInput);
        // printf("liq: %lld\n", getLiqOutput.liquidity);
        EXPECT_EQ(getLiqOutput.liquidity,  140421); // 281842 - 141421
    }
}

TEST(ContractSwap, RemoveLiquidityRejectsNegativeBurnLiquidity)
{
	ContractTestingQswap qswap;

	const id issuer(1, 2, 3, 4);
	const id user(2, 3, 4, 5);
	const uint64 assetName = assetNameFromString("QDOGE");
	constexpr sint64 baseQuReserve = 5031523488ll;
	constexpr sint64 baseAssetReserve = 268698693ll;
	constexpr sint64 baseTotalLiquidity = 1148734806ll;
	constexpr sint64 entryQuAmount = 19ll;
	constexpr sint64 entryAssetAmount = 1ll;
	constexpr sint64 expectedEntryLiquidity = 4ll;
	constexpr sint64 signedBurnLiquidity = -5812151178887170433ll;

	// Seed a private QDOGE-shaped pool state; user actions below still go through QSWAP procedures.
	{
		constexpr sint64 issuedShares = baseAssetReserve + entryAssetAmount + 1000ll;
		increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
		QSWAP::IssueAsset_input issueInput = { assetName, issuedShares, 0, 0 };
		EXPECT_EQ(qswap.issueAsset(issuer, issueInput), issuedShares);

		increaseEnergy(issuer, QSWAP_TRANSFER_ASSET_FEE);
		QSWAP::TransferShareOwnershipAndPossession_input reserveTransferInput = {
			issuer,
			assetName,
			QSWAP_CONTRACT_ID,
			baseAssetReserve
		};
		EXPECT_EQ(qswap.transferAsset(issuer, reserveTransferInput), baseAssetReserve);

		increaseEnergy(issuer, QSWAP_TRANSFER_ASSET_FEE);
		QSWAP::TransferShareOwnershipAndPossession_input userTransferInput = {
			issuer,
			assetName,
			user,
			entryAssetAmount
		};
		EXPECT_EQ(qswap.transferAsset(issuer, userTransferInput), entryAssetAmount);

		increaseEnergy(QSWAP_CONTRACT_ID, baseQuReserve);

		QSWAP::PoolBasicState poolBasicState = {
			id::zero(),
			0,
			0,
			0,
			uint128(0)
		};
		poolBasicState.poolID = issuer;
		poolBasicState.poolID.u64._3 = assetName;
		poolBasicState.reservedQuAmount = baseQuReserve;
		poolBasicState.reservedAssetAmount = baseAssetReserve;
		poolBasicState.totalLiquidity = baseTotalLiquidity;
		qswap.stateData()->mPoolBasicStates.set(0, poolBasicState);
	}

	// Confirm the small entry is accepted by the current contract math.
	increaseEnergy(user, entryQuAmount + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input addLiquidityInput = {
		issuer,
		assetName,
		entryAssetAmount,
		0,
		0
	};
	QSWAP::AddLiquidity_output addLiquidityOutput = qswap.addLiquidity(user, addLiquidityInput, entryQuAmount + QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(addLiquidityOutput.quAmount, entryQuAmount);
	EXPECT_EQ(addLiquidityOutput.assetAmount, entryAssetAmount);
	EXPECT_EQ(addLiquidityOutput.userIncreaseLiquidity, expectedEntryLiquidity);

	QSWAP::GetLiquidityOf_input liquidityInput = { issuer, assetName, user };
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).liquidity, expectedEntryLiquidity);
	EXPECT_EQ(getBalance(user), 0);
	EXPECT_EQ(numberOfPossessedShares(assetName, issuer, user, user, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), 0);

	const sint64 activeQuReserve = baseQuReserve + entryQuAmount;
	const sint64 activeAssetReserve = baseAssetReserve + entryAssetAmount;
	const sint64 activeTotalLiquidity = baseTotalLiquidity + expectedEntryLiquidity;
	QSWAP::GetPoolBasicState_output poolAfterEntry = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_TRUE(poolAfterEntry.poolExists);
	EXPECT_EQ(poolAfterEntry.reservedQuAmount, activeQuReserve);
	EXPECT_EQ(poolAfterEntry.reservedAssetAmount, activeAssetReserve);
	EXPECT_EQ(poolAfterEntry.totalLiquidity, activeTotalLiquidity);
	EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), activeQuReserve + QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), activeAssetReserve);

	// The patched contract should reject negative burnLiquidity before withdrawal math or state changes.
	increaseEnergy(user, QSWAP_ADDITIONAL_FEE);
	QSWAP::RemoveLiquidity_input removeLiquidityInput = {
		issuer,
		assetName,
		signedBurnLiquidity,
		0,
		0
	};
	QSWAP::RemoveLiquidity_output removeLiquidityOutput = qswap.removeLiquidity(user, removeLiquidityInput, QSWAP_ADDITIONAL_FEE);

	EXPECT_EQ(removeLiquidityOutput.quAmount, 0);
	EXPECT_EQ(removeLiquidityOutput.assetAmount, 0);
	EXPECT_EQ(getBalance(user), QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), activeQuReserve + QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(numberOfPossessedShares(assetName, issuer, user, user, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), 0);
	EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), activeAssetReserve);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).liquidity, expectedEntryLiquidity);

	QSWAP::GetPoolBasicState_output poolAfterRemove = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_EQ(poolAfterRemove.reservedQuAmount, activeQuReserve);
	EXPECT_EQ(poolAfterRemove.reservedAssetAmount, activeAssetReserve);
	EXPECT_EQ(poolAfterRemove.totalLiquidity, activeTotalLiquidity);
}

TEST(ContractSwap, ReserveZeroPathReleasesWholeAssetReserveLocalValidation)
{
	ContractTestingQswap qswap;

	const id issuer(1, 2, 3, 4);
	const uint64 assetName = assetNameFromString("QZERO");
	constexpr sint64 initialQuReserve = 1000000ll;
	constexpr sint64 initialAssetReserve = 10000ll;
	constexpr sint64 firstAssetIn = 9999990000ll;
	constexpr sint64 secondAssetIn = 19240000000000ll;
	constexpr sint64 thirdAssetIn = 96250000000000ll;
	constexpr sint64 totalIssuedShares = initialAssetReserve + firstAssetIn + secondAssetIn + thirdAssetIn;

	// Build a private pool whose reserves make the reserve-zero edge case easy to observe.
	increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
	QSWAP::IssueAsset_input issueInput = { assetName, totalIssuedShares, 0, 0 };
	EXPECT_EQ(qswap.issueAsset(issuer, issueInput), totalIssuedShares);

	increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
	EXPECT_TRUE(qswap.createPool(issuer, assetName));

	increaseEnergy(issuer, initialQuReserve + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input addLiquidityInput = {
		issuer,
		assetName,
		initialAssetReserve,
		0,
		0
	};
	QSWAP::AddLiquidity_output addLiquidityOutput = qswap.addLiquidity(issuer, addLiquidityInput, initialQuReserve + QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(addLiquidityOutput.quAmount, initialQuReserve);
	EXPECT_EQ(addLiquidityOutput.assetAmount, initialAssetReserve);

	QSWAP::GetPoolBasicState_output pool = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_TRUE(pool.poolExists);
	EXPECT_EQ(pool.reservedQuAmount, initialQuReserve);
	EXPECT_EQ(pool.reservedAssetAmount, initialAssetReserve);
	EXPECT_GT(pool.totalLiquidity, 0);

	const sint64 assetBalanceAfterSetup = numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
	const sint64 quBalanceAfterSetup = getBalance(issuer);
	EXPECT_EQ(assetBalanceAfterSetup, firstAssetIn + secondAssetIn + thirdAssetIn);

	increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE);
	QSWAP::SwapExactAssetForQu_input firstSwapInput = { issuer, assetName, firstAssetIn, 0 };
	QSWAP::SwapExactAssetForQu_output firstSwapOutput = qswap.swapExactAssetForQu(issuer, firstSwapInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(firstSwapOutput.quAmountOut, 996999);
	pool = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_EQ(pool.reservedQuAmount, 1925);
	EXPECT_EQ(pool.reservedAssetAmount, 10000000000ll);

	increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE);
	QSWAP::SwapExactAssetForQu_input secondSwapInput = { issuer, assetName, secondAssetIn, 0 };
	QSWAP::SwapExactAssetForQu_output secondSwapOutput = qswap.swapExactAssetForQu(issuer, secondSwapInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(secondSwapOutput.quAmountOut, 1918);
	pool = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_EQ(pool.reservedQuAmount, 6);
	EXPECT_EQ(pool.reservedAssetAmount, 19250000000000ll);

	increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE);
	QSWAP::SwapExactAssetForQu_input thirdSwapInput = { issuer, assetName, thirdAssetIn, 0 };
	QSWAP::SwapExactAssetForQu_output thirdSwapOutput = qswap.swapExactAssetForQu(issuer, thirdSwapInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(thirdSwapOutput.quAmountOut, 4);
	pool = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_EQ(pool.reservedQuAmount, 0);
	EXPECT_EQ(pool.reservedAssetAmount, totalIssuedShares);
	EXPECT_GT(pool.totalLiquidity, 0);

	increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE + 36);
	QSWAP::SwapExactQuForAsset_input finalSwapInput = { issuer, assetName, 0 };
	QSWAP::SwapExactQuForAsset_output finalSwapOutput = qswap.swapExactQuForAsset(issuer, finalSwapInput, QSWAP_ADDITIONAL_FEE + 36);
	EXPECT_EQ(finalSwapOutput.assetAmountOut, totalIssuedShares);

	pool = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_EQ(pool.reservedQuAmount, 0);
	EXPECT_EQ(pool.reservedAssetAmount, 0);
	EXPECT_GT(pool.totalLiquidity, 0);

	constexpr sint64 sequenceFunding = (QSWAP_ADDITIONAL_FEE * 3ll) + QSWAP_ADDITIONAL_FEE + 36ll;
	EXPECT_EQ(sint64(getBalance(issuer)) - quBalanceAfterSetup - sequenceFunding, 598885);
	EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX) - assetBalanceAfterSetup, initialAssetReserve);
}

// Encode independently of the contract packer so record lookups also verify the stored key format.
static id referenceLiquidityKey(const id& poolID, const id& account)
{
	uint8 bytes[72] = { 'Q', 'S', 'W', 'A', 'P', 'L', 'P', '1' };
	const uint64 words[8] = {
		poolID.u64._0, poolID.u64._1, poolID.u64._2, poolID.u64._3,
		account.u64._0, account.u64._1, account.u64._2, account.u64._3
	};
	for (unsigned int word = 0; word < 8; ++word)
	{
		for (unsigned int byte = 0; byte < 8; ++byte)
		{
			bytes[8 + word * 8 + byte] = uint8(words[word] >> (byte * 8));
		}
	}
	id digest;
	KangarooTwelve(bytes, sizeof(bytes), &digest, sizeof(digest));
	return digest;
}

TEST(ContractSwap, LiquidityKeyEncodingMatchesIndependentK12Vector)
{
	ContractTestingQswap qswap;
	const id poolID(0x0706050403020100ULL, 0x0f0e0d0c0b0a0908ULL,
		0x1716151413121110ULL, 0x1f1e1d1c1b1a1918ULL);
	const id account(0x2726252423222120ULL, 0x2f2e2d2c2b2a2928ULL,
		0x3736353433323130ULL, 0x3f3e3d3c3b3a3938ULL);
	// K12("QSWAPLP1" || bytes 0x00..0x3f), empty customization, 32-byte output.
	const id expected(0xf6de22c1a906dca1ULL, 0x2fea2bee8c77c82fULL,
		0x4e75c2d1e84bce3cULL, 0x6b3e69ab21910aecULL);
	uint8 expectedBytes[72] = { 'Q', 'S', 'W', 'A', 'P', 'L', 'P', '1' };
	for (unsigned int i = 0; i < 64; ++i)
	{
		expectedBytes[8 + i] = uint8(i);
	}
	QSWAP::LiquidityKeyInput input;
	setMem(&input, sizeof(input), 0xff);
	QswapChecker::packLiquidityKey(poolID, account, input);
	ASSERT_EQ(sizeof(input), sizeof(expectedBytes));
	EXPECT_EQ(memcmp(&input, expectedBytes, sizeof(expectedBytes)), 0);
	QpiContextUserFunctionCall qpi(QSWAP_CONTRACT_INDEX);
	EXPECT_EQ(qpi.K12(input), expected);
	EXPECT_EQ(referenceLiquidityKey(poolID, account), expected);
	EXPECT_NE(referenceLiquidityKey(account, poolID), expected);
	EXPECT_NE(referenceLiquidityKey(poolID, id::zero()), expected);
	EXPECT_NE(referenceLiquidityKey(poolID, QSWAP_CONTRACT_ID), expected);

	// Every byte of both IDs participates, including the issuer words and account's last word.
	for (unsigned int i = 0; i < 64; ++i)
	{
		id changedPool = poolID;
		id changedAccount = account;
		if (i < 32)
		{
			reinterpret_cast<uint8*>(&changedPool)[i] ^= 1;
		}
		else
		{
			reinterpret_cast<uint8*>(&changedAccount)[i - 32] ^= 1;
		}
		QswapChecker::packLiquidityKey(changedPool, changedAccount, input);
		EXPECT_EQ(qpi.K12(input), referenceLiquidityKey(changedPool, changedAccount));
		EXPECT_NE(qpi.K12(input), expected);
	}
}

TEST(ContractSwap, LiquidityKeysSeparateLegacyXorCollision)
{
	ContractTestingQswap qswap;

	const uint64 targetAssetName = assetNameFromString("TARGET1");
	const uint64 aliasAssetName = assetNameFromString("ALIAS1");
	const id targetIssuer(101, 102, 103, 104);
	const id aliasIssuer(201, 202, 203, targetIssuer.u64._3 ^ targetAssetName ^ aliasAssetName);
	const id honestProvider(301, 302, 303, 304);

	constexpr sint64 targetInitialAmount = 1000000;
	constexpr sint64 honestProviderAmount = 1000000;
	constexpr sint64 aliasInitialAmount = 1000000;
	constexpr sint64 aliasSecondAmount = 2000000;

	// These distinct issuer-owned pools intentionally produce the same legacy XOR LP key.
	id targetPoolID = targetIssuer;
	targetPoolID.u64._3 = targetAssetName;
	id aliasPoolID = aliasIssuer;
	aliasPoolID.u64._3 = aliasAssetName;
	id targetLiquidityPov = targetIssuer;
	targetLiquidityPov.u64._0 ^= targetPoolID.u64._0;
	targetLiquidityPov.u64._1 ^= targetPoolID.u64._1;
	targetLiquidityPov.u64._2 ^= targetPoolID.u64._2;
	targetLiquidityPov.u64._3 ^= targetPoolID.u64._3;
	id aliasLiquidityPov = aliasIssuer;
	aliasLiquidityPov.u64._0 ^= aliasPoolID.u64._0;
	aliasLiquidityPov.u64._1 ^= aliasPoolID.u64._1;
	aliasLiquidityPov.u64._2 ^= aliasPoolID.u64._2;
	aliasLiquidityPov.u64._3 ^= aliasPoolID.u64._3;
	EXPECT_NE(targetPoolID, aliasPoolID);
	EXPECT_EQ(targetLiquidityPov, aliasLiquidityPov);

	// Build the target pool with equal contributions from its issuer and an unrelated LP.
	increaseEnergy(targetIssuer, QSWAP_ISSUE_ASSET_FEE);
	QSWAP::IssueAsset_input targetIssueInput = {
		targetAssetName,
		targetInitialAmount + honestProviderAmount,
		0,
		0
	};
	EXPECT_EQ(qswap.issueAsset(targetIssuer, targetIssueInput), targetInitialAmount + honestProviderAmount);

	increaseEnergy(targetIssuer, QSWAP_CREATE_POOL_FEE);
	EXPECT_TRUE(qswap.createPool(targetIssuer, targetAssetName));

	increaseEnergy(targetIssuer, QSWAP_TRANSFER_ASSET_FEE);
	QSWAP::TransferShareOwnershipAndPossession_input targetTransferInput = {
		targetIssuer,
		targetAssetName,
		honestProvider,
		honestProviderAmount
	};
	EXPECT_EQ(qswap.transferAsset(targetIssuer, targetTransferInput), honestProviderAmount);

	increaseEnergy(targetIssuer, targetInitialAmount + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input targetInitialLiquidityInput = {
		targetIssuer,
		targetAssetName,
		targetInitialAmount,
		0,
		0
	};
	QSWAP::AddLiquidity_output targetInitialLiquidity = qswap.addLiquidity(
		targetIssuer,
		targetInitialLiquidityInput,
		targetInitialAmount + QSWAP_ADDITIONAL_FEE
	);
	EXPECT_EQ(targetInitialLiquidity.userIncreaseLiquidity, targetInitialAmount - QSWAP_MIN_LIQUIDITY);

	increaseEnergy(honestProvider, honestProviderAmount + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input honestLiquidityInput = {
		targetIssuer,
		targetAssetName,
		honestProviderAmount,
		0,
		0
	};
	QSWAP::AddLiquidity_output honestLiquidity = qswap.addLiquidity(
		honestProvider,
		honestLiquidityInput,
		honestProviderAmount + QSWAP_ADDITIONAL_FEE
	);
	EXPECT_EQ(honestLiquidity.userIncreaseLiquidity, honestProviderAmount);

	QSWAP::GetLiquidityOf_input targetIssuerLiquidityInput = {
		targetIssuer,
		targetAssetName,
		targetIssuer
	};
	EXPECT_EQ(
		qswap.getLiquidityOf(targetIssuerLiquidityInput).liquidity,
		targetInitialAmount - QSWAP_MIN_LIQUIDITY
	);

	// Initial and repeated alias deposits must leave the target position unchanged.
	increaseEnergy(aliasIssuer, QSWAP_ISSUE_ASSET_FEE);
	QSWAP::IssueAsset_input aliasIssueInput = {
		aliasAssetName,
		aliasInitialAmount + aliasSecondAmount,
		0,
		0
	};
	EXPECT_EQ(qswap.issueAsset(aliasIssuer, aliasIssueInput), aliasInitialAmount + aliasSecondAmount);

	increaseEnergy(aliasIssuer, QSWAP_CREATE_POOL_FEE);
	EXPECT_TRUE(qswap.createPool(aliasIssuer, aliasAssetName));

	increaseEnergy(aliasIssuer, aliasInitialAmount + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input aliasInitialLiquidityInput = {
		aliasIssuer,
		aliasAssetName,
		aliasInitialAmount,
		0,
		0
	};
	EXPECT_EQ(
		qswap.addLiquidity(
			aliasIssuer,
			aliasInitialLiquidityInput,
			aliasInitialAmount + QSWAP_ADDITIONAL_FEE
		).userIncreaseLiquidity,
		aliasInitialAmount - QSWAP_MIN_LIQUIDITY
	);

	EXPECT_EQ(qswap.getLiquidityOf(targetIssuerLiquidityInput).liquidity,
		targetInitialAmount - QSWAP_MIN_LIQUIDITY);
	QSWAP::GetLiquidityOf_input aliasIssuerLiquidityInput = { aliasIssuer, aliasAssetName, aliasIssuer };
	EXPECT_EQ(qswap.getLiquidityOf(aliasIssuerLiquidityInput).liquidity,
		aliasInitialAmount - QSWAP_MIN_LIQUIDITY);

	increaseEnergy(aliasIssuer, aliasSecondAmount + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input aliasSecondLiquidityInput = {
		aliasIssuer,
		aliasAssetName,
		aliasSecondAmount,
		0,
		0
	};
	EXPECT_EQ(
		qswap.addLiquidity(
			aliasIssuer,
			aliasSecondLiquidityInput,
			aliasSecondAmount + QSWAP_ADDITIONAL_FEE
		).userIncreaseLiquidity,
		aliasSecondAmount
	);

	const sint64 targetIssuerLiquidity = targetInitialAmount - QSWAP_MIN_LIQUIDITY;
	const sint64 aliasIssuerLiquidity = aliasInitialAmount - QSWAP_MIN_LIQUIDITY + aliasSecondAmount;
	EXPECT_EQ(qswap.getLiquidityOf(targetIssuerLiquidityInput).liquidity, targetIssuerLiquidity);
	EXPECT_EQ(qswap.getLiquidityOf(aliasIssuerLiquidityInput).liquidity, aliasIssuerLiquidity);
	QSWAP::GetLiquidityOf_input honestLiquidityInputAfterAdd = { targetIssuer, targetAssetName, honestProvider };
	QSWAP::GetLiquidityOf_input targetLockInput = { targetIssuer, targetAssetName, QSWAP_CONTRACT_ID };
	QSWAP::GetLiquidityOf_input aliasLockInput = { aliasIssuer, aliasAssetName, QSWAP_CONTRACT_ID };
	EXPECT_EQ(qswap.getLiquidityOf(honestLiquidityInputAfterAdd).liquidity, honestProviderAmount);
	EXPECT_EQ(qswap.getLiquidityOf(targetLockInput).liquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(aliasLockInput).liquidity, QSWAP_MIN_LIQUIDITY);
	const sint64 targetIndex = qswap.stateData()->mLiquidities.headIndex(referenceLiquidityKey(targetPoolID, targetIssuer), 0);
	const sint64 aliasIndex = qswap.stateData()->mLiquidities.headIndex(referenceLiquidityKey(aliasPoolID, aliasIssuer), 0);
	const sint64 targetLockIndex = qswap.stateData()->mLiquidities.headIndex(referenceLiquidityKey(targetPoolID, QSWAP_CONTRACT_ID), 0);
	const sint64 aliasLockIndex = qswap.stateData()->mLiquidities.headIndex(referenceLiquidityKey(aliasPoolID, QSWAP_CONTRACT_ID), 0);
	ASSERT_NE(targetIndex, NULL_INDEX);
	ASSERT_NE(aliasIndex, NULL_INDEX);
	ASSERT_NE(targetLockIndex, NULL_INDEX);
	ASSERT_NE(aliasLockIndex, NULL_INDEX);
	EXPECT_NE(targetIndex, aliasIndex);
	EXPECT_NE(targetLockIndex, aliasLockIndex);
	EXPECT_EQ(qswap.stateData()->mLiquidities.population(), 5);
	EXPECT_EQ(qswap.stateData()->mLiquidities.element(targetIndex).liquidity, targetIssuerLiquidity);
	EXPECT_EQ(qswap.stateData()->mLiquidities.element(aliasIndex).liquidity, aliasIssuerLiquidity);

	// A withdrawal that previously drained the target pool must refund its fee and change no reserves.
	const auto targetBefore = qswap.getPoolBasicState(targetIssuer, targetAssetName);
	const auto aliasBefore = qswap.getPoolBasicState(aliasIssuer, aliasAssetName);
	const sint64 contractBalanceBefore = getBalance(QSWAP_CONTRACT_ID);
	const sint64 issuerBalanceBefore = getBalance(targetIssuer);
	const sint64 issuerAssetsBefore = numberOfPossessedShares(targetAssetName, targetIssuer,
		targetIssuer, targetIssuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
	increaseEnergy(targetIssuer, QSWAP_ADDITIONAL_FEE);
	QSWAP::RemoveLiquidity_input removeTargetLiquidityInput = {
		targetIssuer, targetAssetName, targetInitialAmount + honestProviderAmount, 0, 0
	};
	const auto rejected = qswap.removeLiquidity(targetIssuer, removeTargetLiquidityInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(rejected.quAmount, 0);
	EXPECT_EQ(rejected.assetAmount, 0);
	EXPECT_EQ(getBalance(targetIssuer), issuerBalanceBefore + QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), contractBalanceBefore);
	EXPECT_EQ(numberOfPossessedShares(targetAssetName, targetIssuer, targetIssuer, targetIssuer,
		QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), issuerAssetsBefore);
	const auto targetAfterRejected = qswap.getPoolBasicState(targetIssuer, targetAssetName);
	EXPECT_EQ(targetAfterRejected.totalLiquidity, targetBefore.totalLiquidity);
	EXPECT_EQ(targetAfterRejected.reservedQuAmount, targetBefore.reservedQuAmount);
	EXPECT_EQ(targetAfterRejected.reservedAssetAmount, targetBefore.reservedAssetAmount);
	EXPECT_EQ(qswap.getLiquidityOf(targetIssuerLiquidityInput).liquidity, targetIssuerLiquidity);

	// Each issuer can withdraw only its own LP; the unrelated provider and both locks survive.
	removeTargetLiquidityInput.burnLiquidity = targetIssuerLiquidity;
	const auto targetExit = qswap.removeLiquidity(targetIssuer, removeTargetLiquidityInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(targetExit.quAmount, targetIssuerLiquidity);
	EXPECT_EQ(targetExit.assetAmount, targetIssuerLiquidity);
	EXPECT_EQ(qswap.getLiquidityOf(targetIssuerLiquidityInput).liquidity, 0);
	EXPECT_EQ(qswap.getLiquidityOf(honestLiquidityInputAfterAdd).liquidity, honestProviderAmount);
	EXPECT_EQ(qswap.getLiquidityOf(aliasIssuerLiquidityInput).liquidity, aliasIssuerLiquidity);
	const auto targetAfterExit = qswap.getPoolBasicState(targetIssuer, targetAssetName);
	EXPECT_EQ(targetAfterExit.totalLiquidity, honestProviderAmount + QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(targetAfterExit.reservedQuAmount, honestProviderAmount + QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(targetAfterExit.reservedAssetAmount, honestProviderAmount + QSWAP_MIN_LIQUIDITY);
	const auto aliasAfterTargetExit = qswap.getPoolBasicState(aliasIssuer, aliasAssetName);
	EXPECT_EQ(aliasAfterTargetExit.totalLiquidity, aliasBefore.totalLiquidity);
	EXPECT_EQ(aliasAfterTargetExit.reservedQuAmount, aliasBefore.reservedQuAmount);
	EXPECT_EQ(aliasAfterTargetExit.reservedAssetAmount, aliasBefore.reservedAssetAmount);

	increaseEnergy(aliasIssuer, QSWAP_ADDITIONAL_FEE);
	QSWAP::RemoveLiquidity_input removeAliasInput = { aliasIssuer, aliasAssetName, aliasIssuerLiquidity, 0, 0 };
	const auto aliasExit = qswap.removeLiquidity(aliasIssuer, removeAliasInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(aliasExit.quAmount, aliasIssuerLiquidity);
	EXPECT_EQ(aliasExit.assetAmount, aliasIssuerLiquidity);
	EXPECT_EQ(qswap.getLiquidityOf(aliasIssuerLiquidityInput).liquidity, 0);
	const auto aliasAfterExit = qswap.getPoolBasicState(aliasIssuer, aliasAssetName);
	EXPECT_EQ(aliasAfterExit.totalLiquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(aliasAfterExit.reservedQuAmount, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(aliasAfterExit.reservedAssetAmount, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(honestLiquidityInputAfterAdd).liquidity, honestProviderAmount);
	EXPECT_EQ(qswap.getLiquidityOf(targetLockInput).liquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(aliasLockInput).liquidity, QSWAP_MIN_LIQUIDITY);

	increaseEnergy(honestProvider, QSWAP_ADDITIONAL_FEE);
	QSWAP::RemoveLiquidity_input removeHonestInput = { targetIssuer, targetAssetName, honestProviderAmount, 0, 0 };
	const auto honestExit = qswap.removeLiquidity(honestProvider, removeHonestInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(honestExit.quAmount, honestProviderAmount);
	EXPECT_EQ(honestExit.assetAmount, honestProviderAmount);
	EXPECT_EQ(qswap.getLiquidityOf(honestLiquidityInputAfterAdd).liquidity, 0);
	const auto targetAfterAllExits = qswap.getPoolBasicState(targetIssuer, targetAssetName);
	EXPECT_EQ(targetAfterAllExits.totalLiquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(targetAfterAllExits.reservedQuAmount, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(targetAfterAllExits.reservedAssetAmount, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(targetLockInput).liquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(aliasLockInput).liquidity, QSWAP_MIN_LIQUIDITY);
}

TEST(ContractSwap, LiquidityFeeBookkeepingSurvivesAddPartialRemoveAndReentry)
{
	ContractTestingQswap qswap;
	const id issuer(401, 402, 403, 404);
	const id trader(501, 502, 503, 504);
	const uint64 assetName = assetNameFromString("FEE1");
	constexpr sint64 initialAmount = 1000000;
	constexpr sint64 swapAmount = 100000;
	QSWAP::GetLiquidityOf_input liquidityInput = { issuer, assetName, issuer };
	QSWAP::GetLiquidityOf_input lockInput = { issuer, assetName, QSWAP_CONTRACT_ID };
	id poolID = issuer;
	poolID.u64._3 = assetName;
	const id issuerKey = referenceLiquidityKey(poolID, issuer);

	increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
	QSWAP::IssueAsset_input issueInput = { assetName, 10000000, 0, 0 };
	ASSERT_EQ(qswap.issueAsset(issuer, issueInput), 10000000);
	increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
	ASSERT_TRUE(qswap.createPool(issuer, assetName));
	increaseEnergy(issuer, initialAmount + QSWAP_ADDITIONAL_FEE);
	QSWAP::AddLiquidity_input addInput = { issuer, assetName, initialAmount, 0, 0 };
	ASSERT_EQ(qswap.addLiquidity(issuer, addInput, initialAmount + QSWAP_ADDITIONAL_FEE).userIncreaseLiquidity,
		initialAmount - QSWAP_MIN_LIQUIDITY);
	ASSERT_EQ(qswap.stateData()->mPoolBasicStates.get(0).poolID, poolID);
	const sint64 issuerIndex = qswap.stateData()->mLiquidities.headIndex(issuerKey, 0);
	ASSERT_NE(issuerIndex, NULL_INDEX);
	EXPECT_EQ(qswap.stateData()->mLiquidities.element(issuerIndex).feeDebtX64, uint128(0));
	EXPECT_EQ(qswap.stateData()->mLiquidities.element(issuerIndex).accumulatedFee, 0);

	// A real swap earns 192 QU for LPs; the issuer owns 999/1000 of the initial supply.
	ASSERT_EQ(qswap.stateData()->swapFeeRate, 30);
	QSWAP::SwapExactQuForAsset_input swapInput = { issuer, assetName, 0 };
	increaseEnergy(trader, swapAmount + QSWAP_ADDITIONAL_FEE);
	ASSERT_GT(qswap.swapExactQuForAsset(trader, swapInput, swapAmount + QSWAP_ADDITIONAL_FEE).assetAmountOut, 0);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 191);

	// Add the pool's current reserve ratio, minting another initialAmount LP without backdating fees.
	const auto beforeAdd = qswap.getPoolBasicState(issuer, assetName);
	addInput.assetAmountDesired = beforeAdd.reservedAssetAmount;
	increaseEnergy(issuer, beforeAdd.reservedQuAmount + QSWAP_ADDITIONAL_FEE);
	const auto added = qswap.addLiquidity(issuer, addInput, beforeAdd.reservedQuAmount + QSWAP_ADDITIONAL_FEE);
	ASSERT_EQ(added.userIncreaseLiquidity, initialAmount);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).liquidity, 2 * initialAmount - QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 191);
	const auto recordAfterAdd = qswap.stateData()->mLiquidities.element(issuerIndex);
	EXPECT_EQ(recordAfterAdd.accumulatedFee, 191);
	EXPECT_EQ(recordAfterAdd.feeDebtX64, qswap.stateData()->mPoolBasicStates.get(0).accFeePerLPX64);

	increaseEnergy(trader, swapAmount + QSWAP_ADDITIONAL_FEE);
	ASSERT_GT(qswap.swapExactQuForAsset(trader, swapInput, swapAmount + QSWAP_ADDITIONAL_FEE).assetAmountOut, 0);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 382);

	constexpr sint64 partialBurn = 500000;
	const auto beforePartial = qswap.getPoolBasicState(issuer, assetName);
	increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE);
	QSWAP::RemoveLiquidity_input removeInput = { issuer, assetName, partialBurn, 0, 0 };
	const auto removed = qswap.removeLiquidity(issuer, removeInput, QSWAP_ADDITIONAL_FEE);
	EXPECT_EQ(removed.quAmount, beforePartial.reservedQuAmount / 4);
	EXPECT_EQ(removed.assetAmount, beforePartial.reservedAssetAmount / 4);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).liquidity,
		2 * initialAmount - QSWAP_MIN_LIQUIDITY - partialBurn);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 382);
	const auto recordAfterPartial = qswap.stateData()->mLiquidities.element(issuerIndex);
	EXPECT_EQ(recordAfterPartial.accumulatedFee, 382);
	EXPECT_EQ(recordAfterPartial.feeDebtX64, qswap.stateData()->mPoolBasicStates.get(0).accFeePerLPX64);

	increaseEnergy(trader, swapAmount + QSWAP_ADDITIONAL_FEE);
	ASSERT_GT(qswap.swapExactQuForAsset(trader, swapInput, swapAmount + QSWAP_ADDITIONAL_FEE).assetAmountOut, 0);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 573);

	// Full exit removes the position, as before; a new position starts at the current fee accumulator.
	removeInput.burnLiquidity = qswap.getLiquidityOf(liquidityInput).liquidity;
	increaseEnergy(issuer, QSWAP_ADDITIONAL_FEE);
	const auto exited = qswap.removeLiquidity(issuer, removeInput, QSWAP_ADDITIONAL_FEE);
	ASSERT_GT(exited.quAmount, 0);
	ASSERT_GT(exited.assetAmount, 0);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).liquidity, 0);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 0);
	EXPECT_EQ(qswap.stateData()->mLiquidities.headIndex(issuerKey, 0), NULL_INDEX);
	const auto afterExit = qswap.getPoolBasicState(issuer, assetName);
	EXPECT_EQ(afterExit.totalLiquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(lockInput).liquidity, QSWAP_MIN_LIQUIDITY);

	addInput.assetAmountDesired = afterExit.reservedAssetAmount;
	increaseEnergy(issuer, afterExit.reservedQuAmount + QSWAP_ADDITIONAL_FEE);
	const auto reentered = qswap.addLiquidity(issuer, addInput, afterExit.reservedQuAmount + QSWAP_ADDITIONAL_FEE);
	ASSERT_EQ(reentered.userIncreaseLiquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).liquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 0);
	EXPECT_EQ(qswap.getLiquidityOf(lockInput).liquidity, QSWAP_MIN_LIQUIDITY);
	EXPECT_EQ(qswap.getPoolBasicState(issuer, assetName).totalLiquidity, 2 * QSWAP_MIN_LIQUIDITY);
	const sint64 reentryIndex = qswap.stateData()->mLiquidities.headIndex(issuerKey, 0);
	ASSERT_NE(reentryIndex, NULL_INDEX);
	const auto recordAfterReentry = qswap.stateData()->mLiquidities.element(reentryIndex);
	EXPECT_EQ(recordAfterReentry.accumulatedFee, 0);
	EXPECT_EQ(recordAfterReentry.feeDebtX64, qswap.stateData()->mPoolBasicStates.get(0).accFeePerLPX64);

	increaseEnergy(trader, swapAmount + QSWAP_ADDITIONAL_FEE);
	ASSERT_GT(qswap.swapExactQuForAsset(trader, swapInput, swapAmount + QSWAP_ADDITIONAL_FEE).assetAmountOut, 0);
	EXPECT_EQ(qswap.getLiquidityOf(liquidityInput).earnedFees, 95);
}

// failed case
TEST(ContractSwap, LiqTest2)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    uint64 invalidAssetName = assetNameFromString("QSWAP1");
    sint64 numberOfShares = 1000*1000;

    // 0. issue an asset and create a pool
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));
    }

    // add liquidity to invalid pool,
    {
        // decreaseEnergy(getBalance(issuer));
        uint64 quAmount = 1000 + QSWAP_ADDITIONAL_FEE;
        increaseEnergy(issuer, quAmount);
        QSWAP::AddLiquidity_input addLiqInput = {
            issuer,
            invalidAssetName,
            1000,
            0,
            0 
        };
        
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, addLiqInput, 1000 + QSWAP_ADDITIONAL_FEE);
        EXPECT_EQ(output.userIncreaseLiquidity, 0);
        EXPECT_EQ(output.quAmount, 0);
        EXPECT_EQ(output.assetAmount, 0);
    }

    // add liquidity with asset more than holdings
    {
        increaseEnergy(issuer, 1000 + QSWAP_ADDITIONAL_FEE);
        QSWAP::AddLiquidity_input addLiqInput = {
            issuer,
            assetName,
            1000*1000 + 100, // excced 1000*1000
            0,
            0 
        };

        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, addLiqInput, 1000 + QSWAP_ADDITIONAL_FEE);
        EXPECT_EQ(output.userIncreaseLiquidity, 0);
        EXPECT_EQ(output.quAmount, 0);
        EXPECT_EQ(output.assetAmount, 0);
    }
}

// Occupy the real global collection with unrelated test records, without changing its capacity.
static void fillLiquidityCollection(ContractTestingQswap& qswap)
{
	auto& liquidities = qswap.stateData()->mLiquidities;
	const QSWAP::LiquidityInfo filler = { 17, uint128(23), 29 };
	while (liquidities.population() < liquidities.capacity())
	{
		const id key(liquidities.population(), 0xf111111111111111ULL,
			0xf222222222222222ULL, 0xf333333333333333ULL);
		ASSERT_NE(liquidities.add(key, filler, 0), NULL_INDEX);
	}
}

TEST(ContractSwap, LiquidityCollectionFullRefundsNewProviderAndUpdatesExistingRecords)
{
	ContractTestingQswap qswap;
	const id issuer(601, 602, 603, 604);
	const id newProvider(701, 702, 703, 704);
	const uint64 positiveName = assetNameFromString("FULLPOS");
	const uint64 zeroName = assetNameFromString("FULLZER");
	constexpr sint64 deposit = 1000;

	for (const uint64 name : { positiveName, zeroName })
	{
		increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
		ASSERT_EQ(qswap.issueAsset(issuer, { name, 2000000, 0, 0 }), 2000000);
		increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
		ASSERT_TRUE(qswap.createPool(issuer, name));
		const sint64 initial = name == positiveName ? 1000000 : QSWAP_MIN_LIQUIDITY;
		increaseEnergy(issuer, initial + QSWAP_ADDITIONAL_FEE);
		const auto added = qswap.addLiquidity(issuer, { issuer, name, initial, 0, 0 },
			initial + QSWAP_ADDITIONAL_FEE);
		ASSERT_EQ(added.userIncreaseLiquidity, initial - QSWAP_MIN_LIQUIDITY);
	}
	increaseEnergy(issuer, QSWAP_TRANSFER_ASSET_FEE);
	ASSERT_EQ(qswap.transferAsset(issuer, { issuer, positiveName, newProvider, deposit }), deposit);

	id zeroPoolID = issuer;
	zeroPoolID.u64._3 = zeroName;
	const id zeroKey = referenceLiquidityKey(zeroPoolID, issuer);
	auto& liquidities = qswap.stateData()->mLiquidities;
	const sint64 zeroIndex = liquidities.headIndex(zeroKey, 0);
	ASSERT_NE(zeroIndex, NULL_INDEX);
	ASSERT_EQ(liquidities.element(zeroIndex).liquidity, 0);
	fillLiquidityCollection(qswap);
	ASSERT_EQ(liquidities.population(), liquidities.capacity());

	// A new LP must receive its entire reward back without overwriting any native record.
	id collectionBefore, collectionAfter;
	KangarooTwelve(&liquidities, sizeof(liquidities), &collectionBefore, sizeof(collectionBefore));
	const auto poolBefore = qswap.getPoolBasicState(issuer, positiveName);
	const sint64 contractBalanceBefore = getBalance(QSWAP_CONTRACT_ID);
	const uint64 shareholderFeeBefore = qswap.stateData()->shareholderEarnedFee;
	const uint64 burnFeeBefore = qswap.stateData()->burnEarnedFee;
	constexpr sint64 reward = deposit + QSWAP_ADDITIONAL_FEE + 123;
	increaseEnergy(newProvider, reward);
	const sint64 providerBalanceBefore = getBalance(newProvider);
	const auto rejected = qswap.addLiquidity(newProvider, { issuer, positiveName, deposit, 0, 0 }, reward);
	EXPECT_EQ(rejected.userIncreaseLiquidity, 0);
	EXPECT_EQ(rejected.quAmount, 0);
	EXPECT_EQ(rejected.assetAmount, 0);
	EXPECT_EQ(getBalance(newProvider), providerBalanceBefore);
	EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), contractBalanceBefore);
	EXPECT_EQ(numberOfPossessedShares(positiveName, issuer, newProvider, newProvider,
		QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), deposit);
	EXPECT_EQ(qswap.stateData()->shareholderEarnedFee, shareholderFeeBefore);
	EXPECT_EQ(qswap.stateData()->burnEarnedFee, burnFeeBefore);
	const auto poolAfter = qswap.getPoolBasicState(issuer, positiveName);
	EXPECT_EQ(poolAfter.totalLiquidity, poolBefore.totalLiquidity);
	EXPECT_EQ(poolAfter.reservedQuAmount, poolBefore.reservedQuAmount);
	EXPECT_EQ(poolAfter.reservedAssetAmount, poolBefore.reservedAssetAmount);
	EXPECT_EQ(qswap.getLiquidityOf({ issuer, positiveName, newProvider }).liquidity, 0);
	KangarooTwelve(&liquidities, sizeof(liquidities), &collectionAfter, sizeof(collectionAfter));
	EXPECT_EQ(collectionAfter, collectionBefore);

	// Existing positive and zero-sized records need no new slot and remain usable when full.
	for (const uint64 name : { positiveName, zeroName })
	{
		const auto before = qswap.getPoolBasicState(issuer, name);
		const auto positionBefore = qswap.getLiquidityOf({ issuer, name, issuer });
		const sint64 assetsBefore = numberOfPossessedShares(name, issuer, issuer, issuer,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
		increaseEnergy(issuer, deposit + QSWAP_ADDITIONAL_FEE);
		const sint64 balanceBefore = getBalance(issuer);
		const auto added = qswap.addLiquidity(issuer, { issuer, name, deposit, 0, 0 },
			deposit + QSWAP_ADDITIONAL_FEE);
		EXPECT_EQ(added.userIncreaseLiquidity, deposit);
		EXPECT_EQ(added.quAmount, deposit);
		EXPECT_EQ(added.assetAmount, deposit);
		EXPECT_EQ(getBalance(issuer), balanceBefore - deposit - QSWAP_ADDITIONAL_FEE);
		EXPECT_EQ(numberOfPossessedShares(name, issuer, issuer, issuer,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), assetsBefore - deposit);
		const auto after = qswap.getPoolBasicState(issuer, name);
		EXPECT_EQ(after.totalLiquidity, before.totalLiquidity + deposit);
		EXPECT_EQ(after.reservedQuAmount, before.reservedQuAmount + deposit);
		EXPECT_EQ(after.reservedAssetAmount, before.reservedAssetAmount + deposit);
		EXPECT_EQ(qswap.getLiquidityOf({ issuer, name, issuer }).liquidity,
			positionBefore.liquidity + deposit);
		EXPECT_EQ(qswap.getLiquidityOf({ issuer, name, QSWAP_CONTRACT_ID }).liquidity,
			QSWAP_MIN_LIQUIDITY);
		EXPECT_EQ(liquidities.population(), liquidities.capacity());
	}
	EXPECT_EQ(liquidities.headIndex(zeroKey, 0), zeroIndex);
	EXPECT_EQ(liquidities.element(zeroIndex).liquidity, deposit);
}

TEST(ContractSwap, InitialDepositRefundsWhenFewerThanTwoLiquiditySlotsRemain)
{
	ContractTestingQswap qswap;
	const id issuer(801, 802, 803, 804);
	const uint64 fullName = assetNameFromString("FULLINI");
	const uint64 oneSlotName = assetNameFromString("ONEINI");
	constexpr sint64 deposit = 2000;
	constexpr sint64 reward = deposit + QSWAP_ADDITIONAL_FEE + 123;
	for (const uint64 name : { fullName, oneSlotName })
	{
		increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
		ASSERT_EQ(qswap.issueAsset(issuer, { name, 10000, 0, 0 }), 10000);
		increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
		ASSERT_TRUE(qswap.createPool(issuer, name));
	}
	auto& liquidities = qswap.stateData()->mLiquidities;
	auto& pools = qswap.stateData()->mPoolBasicStates;
	fillLiquidityCollection(qswap);
	ASSERT_EQ(liquidities.population(), liquidities.capacity());

	// Neither first deposit may move funds or store only one of its two positions.
	for (const uint64 name : { fullName, oneSlotName })
	{
		const uint64 freeSlots = name == oneSlotName ? 1 : 0;
		if (freeSlots)
		{
			liquidities.remove(liquidities.population() - 1);
		}
		ASSERT_EQ(liquidities.population(), liquidities.capacity() - freeSlots);
		id collectionBefore, collectionAfter, poolsBefore, poolsAfter;
		KangarooTwelve(&liquidities, sizeof(liquidities), &collectionBefore, sizeof(collectionBefore));
		KangarooTwelve(&pools, sizeof(pools), &poolsBefore, sizeof(poolsBefore));
		const auto poolBefore = qswap.getPoolBasicState(issuer, name);
		ASSERT_TRUE(poolBefore.poolExists);
		ASSERT_EQ(poolBefore.totalLiquidity, 0);
		const sint64 assetsBefore = numberOfPossessedShares(name, issuer, issuer, issuer,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
		const sint64 contractAssetsBefore = numberOfPossessedShares(name, issuer,
			QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
		const sint64 contractBalanceBefore = getBalance(QSWAP_CONTRACT_ID);
		const uint64 shareholderFeeBefore = qswap.stateData()->shareholderEarnedFee;
		const uint64 burnFeeBefore = qswap.stateData()->burnEarnedFee;
		const uint64 investRewardsFeeBefore = qswap.stateData()->investRewardsEarnedFee;
		const uint64 qxFeeBefore = qswap.stateData()->qxEarnedFee;
		increaseEnergy(issuer, reward);
		const sint64 balanceBefore = getBalance(issuer);
		const auto rejected = qswap.addLiquidity(issuer, { issuer, name, deposit, 0, 0 }, reward);
		EXPECT_EQ(rejected.userIncreaseLiquidity, 0);
		EXPECT_EQ(rejected.quAmount, 0);
		EXPECT_EQ(rejected.assetAmount, 0);
		EXPECT_EQ(getBalance(issuer), balanceBefore);
		EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), contractBalanceBefore);
		EXPECT_EQ(numberOfPossessedShares(name, issuer, issuer, issuer,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), assetsBefore);
		EXPECT_EQ(numberOfPossessedShares(name, issuer, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), contractAssetsBefore);
		EXPECT_EQ(qswap.stateData()->shareholderEarnedFee, shareholderFeeBefore);
		EXPECT_EQ(qswap.stateData()->burnEarnedFee, burnFeeBefore);
		EXPECT_EQ(qswap.stateData()->investRewardsEarnedFee, investRewardsFeeBefore);
		EXPECT_EQ(qswap.stateData()->qxEarnedFee, qxFeeBefore);
		const auto poolAfter = qswap.getPoolBasicState(issuer, name);
		EXPECT_EQ(poolAfter.poolExists, poolBefore.poolExists);
		EXPECT_EQ(poolAfter.totalLiquidity, poolBefore.totalLiquidity);
		EXPECT_EQ(poolAfter.reservedQuAmount, poolBefore.reservedQuAmount);
		EXPECT_EQ(poolAfter.reservedAssetAmount, poolBefore.reservedAssetAmount);
		EXPECT_EQ(poolAfter.accFeePerLP, poolBefore.accFeePerLP);
		id poolID = issuer;
		poolID.u64._3 = name;
		for (const id account : { issuer, QSWAP_CONTRACT_ID })
		{
			EXPECT_EQ(liquidities.headIndex(referenceLiquidityKey(poolID, account), 0), NULL_INDEX);
			const auto position = qswap.getLiquidityOf({ issuer, name, account });
			EXPECT_EQ(position.liquidity, 0);
			EXPECT_EQ(position.earnedFees, 0);
		}
		EXPECT_EQ(liquidities.population(), liquidities.capacity() - freeSlots);
		KangarooTwelve(&liquidities, sizeof(liquidities), &collectionAfter, sizeof(collectionAfter));
		KangarooTwelve(&pools, sizeof(pools), &poolsAfter, sizeof(poolsAfter));
		EXPECT_EQ(collectionAfter, collectionBefore);
		EXPECT_EQ(poolsAfter, poolsBefore);
	}
}

TEST(ContractSwap, InitialDepositStoresBothPositionsWithExactlyTwoLiquiditySlots)
{
	// The minimum deposit still stores a zero-sized caller position in the second slot.
	for (const sint64 deposit : { sint64(2000), QSWAP_MIN_LIQUIDITY })
	{
		ContractTestingQswap qswap;
		const id issuer(901, 902, 903, 904);
		const uint64 assetName = assetNameFromString("TWOINI");
		increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
		ASSERT_EQ(qswap.issueAsset(issuer, { assetName, 10000, 0, 0 }), 10000);
		increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
		ASSERT_TRUE(qswap.createPool(issuer, assetName));
		auto& liquidities = qswap.stateData()->mLiquidities;
		fillLiquidityCollection(qswap);
		liquidities.remove(liquidities.population() - 1);
		liquidities.remove(liquidities.population() - 1);
		ASSERT_EQ(liquidities.population(), liquidities.capacity() - 2);
		const id sentinelKey = liquidities.pov(0);
		const auto sentinel = liquidities.element(0);
		const sint64 assetsBefore = numberOfPossessedShares(assetName, issuer, issuer, issuer,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
		const sint64 contractAssetsBefore = numberOfPossessedShares(assetName, issuer,
			QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX);
		const sint64 contractBalanceBefore = getBalance(QSWAP_CONTRACT_ID);
		const uint64 shareholderFeeBefore = qswap.stateData()->shareholderEarnedFee;
		const uint64 burnFeeBefore = qswap.stateData()->burnEarnedFee;
		const uint64 investRewardsFeeBefore = qswap.stateData()->investRewardsEarnedFee;
		const uint64 qxFeeBefore = qswap.stateData()->qxEarnedFee;
		increaseEnergy(issuer, deposit + QSWAP_ADDITIONAL_FEE);
		const sint64 balanceBefore = getBalance(issuer);
		const auto added = qswap.addLiquidity(issuer, { issuer, assetName, deposit, 0, 0 },
			deposit + QSWAP_ADDITIONAL_FEE);
		EXPECT_EQ(added.userIncreaseLiquidity, deposit - QSWAP_MIN_LIQUIDITY);
		EXPECT_EQ(added.quAmount, deposit);
		EXPECT_EQ(added.assetAmount, deposit);
		EXPECT_EQ(getBalance(issuer), balanceBefore - deposit - QSWAP_ADDITIONAL_FEE);
		EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), contractBalanceBefore + deposit + QSWAP_ADDITIONAL_FEE);
		EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), assetsBefore - deposit);
		EXPECT_EQ(numberOfPossessedShares(assetName, issuer, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID,
			QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), contractAssetsBefore + deposit);
		EXPECT_EQ(qswap.stateData()->shareholderEarnedFee,
			shareholderFeeBefore + QSWAP_ADDITIONAL_FEE * 3 / 4);
		EXPECT_EQ(qswap.stateData()->burnEarnedFee, burnFeeBefore + QSWAP_ADDITIONAL_FEE / 4);
		EXPECT_EQ(qswap.stateData()->investRewardsEarnedFee, investRewardsFeeBefore);
		EXPECT_EQ(qswap.stateData()->qxEarnedFee, qxFeeBefore);
		const auto pool = qswap.getPoolBasicState(issuer, assetName);
		EXPECT_TRUE(pool.poolExists);
		EXPECT_EQ(pool.totalLiquidity, deposit);
		EXPECT_EQ(pool.reservedQuAmount, deposit);
		EXPECT_EQ(pool.reservedAssetAmount, deposit);
		EXPECT_EQ(pool.accFeePerLP, 0);
		EXPECT_EQ(qswap.stateData()->mPoolBasicStates.get(0).accFeePerLPX64, uint128(0));
		id poolID = issuer;
		poolID.u64._3 = assetName;
		const id userKey = referenceLiquidityKey(poolID, issuer);
		const id lockKey = referenceLiquidityKey(poolID, QSWAP_CONTRACT_ID);
		const sint64 userIndex = liquidities.headIndex(userKey, 0);
		const sint64 lockIndex = liquidities.headIndex(lockKey, 0);
		ASSERT_NE(userIndex, NULL_INDEX);
		ASSERT_NE(lockIndex, NULL_INDEX);
		EXPECT_NE(userIndex, lockIndex);
		EXPECT_EQ(liquidities.pov(userIndex), userKey);
		EXPECT_EQ(liquidities.pov(lockIndex), lockKey);
		EXPECT_EQ(liquidities.element(userIndex).liquidity, deposit - QSWAP_MIN_LIQUIDITY);
		EXPECT_EQ(liquidities.element(lockIndex).liquidity, QSWAP_MIN_LIQUIDITY);
		for (const sint64 index : { userIndex, lockIndex })
		{
			EXPECT_EQ(liquidities.element(index).feeDebtX64, uint128(0));
			EXPECT_EQ(liquidities.element(index).accumulatedFee, 0);
		}
		const auto user = qswap.getLiquidityOf({ issuer, assetName, issuer });
		const auto lock = qswap.getLiquidityOf({ issuer, assetName, QSWAP_CONTRACT_ID });
		EXPECT_EQ(user.liquidity, deposit - QSWAP_MIN_LIQUIDITY);
		EXPECT_EQ(lock.liquidity, QSWAP_MIN_LIQUIDITY);
		EXPECT_EQ(user.earnedFees, 0);
		EXPECT_EQ(lock.earnedFees, 0);
		EXPECT_EQ(user.liquidity + lock.liquidity, pool.totalLiquidity);
		EXPECT_EQ(liquidities.population(), liquidities.capacity());
		EXPECT_EQ(liquidities.pov(0), sentinelKey);
		EXPECT_EQ(liquidities.element(0).liquidity, sentinel.liquidity);
		EXPECT_EQ(liquidities.element(0).feeDebtX64, sentinel.feeDebtX64);
		EXPECT_EQ(liquidities.element(0).accumulatedFee, sentinel.accumulatedFee);
	}
}
