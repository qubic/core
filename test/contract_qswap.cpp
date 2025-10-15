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
constexpr uint32 TEAM_INFO_IDX = 8;
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
constexpr uint32 SET_TEAM_INFO_IDX = 10;
constexpr uint32 TRANSFER_SHARE_MANAGEMENT_RIGHTS_IDX = 11;


class QswapChecker : public QSWAP
{
// public:
// 	void checkCollectionConsistency() {
// 	}
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

	bool loadState(const CHAR16* filename)
	{
		return load(filename, sizeof(QSWAP), contractStates[QSWAP_CONTRACT_INDEX]) == sizeof(QSWAP);
	}

	QSWAP::TeamInfo_output teamInfo()
    {
        QSWAP::TeamInfo_input input{};
		QSWAP::TeamInfo_output output;
		callFunction(QSWAP_CONTRACT_INDEX, TEAM_INFO_IDX, input, output);
		return output;
	}

	bool setTeamId(const id& issuer, QSWAP::SetTeamInfo_input input)
    {
		QSWAP::CreatePool_output output;
		invokeUserProcedure(QSWAP_CONTRACT_INDEX, SET_TEAM_INFO_IDX, input, output, issuer, 0);
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

TEST(ContractSwap, TeamInfoTest)
{
	ContractTestingQswap qswap;

    {
		QSWAP::TeamInfo_output team_info = qswap.teamInfo();

		auto expectIdentity = (const unsigned char*)"IRUNQTXZRMLDEENHPRZQPSGPCFACORRUJYSBVJPQEHFCEKLLURVDDJVEXNBL";
		m256i expectPubkey;
		getPublicKeyFromIdentity(expectIdentity, expectPubkey.m256i_u8);
		EXPECT_EQ(team_info.teamId, expectPubkey);
		EXPECT_EQ(team_info.teamFee, 20);
    }

	{
		id newTeamId(6,6,6,6);
		QSWAP::SetTeamInfo_input input = {newTeamId};

		id invalidIssuer(1,2,3,4);

        increaseEnergy(invalidIssuer, 100);
		bool res1 = qswap.setTeamId(invalidIssuer, input);
		// printf("res1: %d\n", res1);
		EXPECT_FALSE(res1);

		auto teamIdentity = (const unsigned char*)"IRUNQTXZRMLDEENHPRZQPSGPCFACORRUJYSBVJPQEHFCEKLLURVDDJVEXNBL";
		m256i teamPubkey;
		getPublicKeyFromIdentity(teamIdentity, teamPubkey.m256i_u8);

        increaseEnergy(teamPubkey, 100);
		bool res2 = qswap.setTeamId(teamPubkey, input);
		// printf("res2: %d\n", res2);
		EXPECT_TRUE(res2);

		QSWAP::TeamInfo_output team_info = qswap.teamInfo();
		EXPECT_EQ(team_info.teamId, newTeamId);
		// printf("%d\n", team_info.teamId == newTeamId);
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

        sint64 inputValue = 30*1000;
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
        EXPECT_EQ(qo_output.assetAmountIn, 1037);

        QSWAP::QuoteExactAssetInput_input ai_input = {issuer, assetName, 1000};
        QSWAP::QuoteExactAssetInput_output ai_output = qswap.quoteExactAssetInput(ai_input);
        // printf("quote exact asset input: %lld\n", ai_output.quAmountOut);
        EXPECT_EQ(ai_output.quAmountOut, 964);

        QSWAP::QuoteExactAssetOutput_input ao_input = {issuer, assetName, 1000};
        QSWAP::QuoteExactAssetOutput_output ao_output = qswap.quoteExactAssetOutput(ao_input);
        // printf("quote exact asset output: %lld\n", ao_output.quAmountIn);
        EXPECT_EQ(ao_output.quAmountIn, 1037);
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

        sint64 inputValue = 200*1000;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);
    }

    {
        // swap in 100*1000 qu, get about 1000*50 asset
        id user(2,3,4,5);
        sint64 inputValue = 200*1000;
        increaseEnergy(user, inputValue);

        QSWAP::QuoteExactQuInput_input qi_input = {issuer, assetName, inputValue};
        QSWAP::QuoteExactQuInput_output qi_output = qswap.quoteExactQuInput(qi_input);
        // printf("quote_exact_qu_input, asset out: %lld\n", qi_output.assetAmountOut);

        QSWAP::SwapExactQuForAsset_input input = {issuer, assetName, 0};
        QSWAP::SwapExactQuForAsset_output output = qswap.swapExactQuForAsset(user, input, inputValue);
        // printf("swap_exact_qu_for_asset, asset out: %lld\n", output.assetAmountOut);

        EXPECT_EQ(qi_output.assetAmountOut, output.assetAmountOut); // 49924

        EXPECT_TRUE(output.assetAmountOut <= 50000); // 49924 if swapFee 0.3%

        QSWAP::GetPoolBasicState_output psOutput = qswap.getPoolBasicState(issuer, assetName);
        // printf("%lld, %lld, %lld\n", psOutput.reservedAssetAmount, psOutput.reservedQuAmount, psOutput.totalLiquidity);
		// swapFee is 200_000 * 0.3% = 600, teamFee: 120, protocolFee: 96
        EXPECT_TRUE(psOutput.reservedQuAmount >= 399784); // 399784 = (400_000 - 120 - 96)
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

        sint64 inputValue = 200*1000;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);
    }

    {
        id user(2,3,4,5);
        sint64 inputValue = 1000 * 200;
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

        sint64 inputValue = 200*1000;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiquidity_input alInput = { issuer, assetName, 100*1000, 0, 0 };
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, alInput, inputValue);
        // printf("increase liquidity: %lld, %lld, %lld\n", output.userIncreaseLiquidity, output.assetAmount, output.quAmount);
    }

    {
        id user(1, 2,3,4);
        sint64 inputValue = 0;
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

        sint64 inputValue = 200*1000;
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
       sint64 inputValue = 0;
       sint64 quAmountOut = 100*1000;
       sint64 expectAssetAmountIn = 100603;

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
        sint64 inputValue = quStakeAmount;
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
        QSWAP::RemoveLiquidity_output rmLiqOutput = qswap.removeLiquidity(issuer, rmLiqInput, 0);
        // printf("qu: %lld, asset: %lld\n", rmLiqOutput.quAmount, rmLiqOutput.assetAmount);
        EXPECT_EQ(rmLiqOutput.quAmount, 1000 * 200);
        EXPECT_EQ(rmLiqOutput.assetAmount, 1000 * 100);

        getLiqOutput = qswap.getLiquidityOf(getLiqInput);
        // printf("liq: %lld\n", getLiqOutput.liquidity);
        EXPECT_EQ(getLiqOutput.liquidity,  140421); // 281842 - 141421
    }
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
        uint64 quAmount = 1000;
        increaseEnergy(issuer, quAmount);
        QSWAP::AddLiquidity_input addLiqInput = {
            issuer,
            invalidAssetName,
            1000,
            0,
            0 
        };
        
        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, addLiqInput, 1000);
        EXPECT_EQ(output.userIncreaseLiquidity, 0);
        EXPECT_EQ(output.quAmount, 0);
        EXPECT_EQ(output.assetAmount, 0);
    }

    // add liquidity with asset more than holdings
    {
        increaseEnergy(issuer, 1000);
        QSWAP::AddLiquidity_input addLiqInput = {
            issuer,
            assetName,
            1000*1000 + 100, // excced 1000*1000
            0,
            0 
        };

        QSWAP::AddLiquidity_output output = qswap.addLiquidity(issuer, addLiqInput, 1000);
        EXPECT_EQ(output.userIncreaseLiquidity, 0);
        EXPECT_EQ(output.quAmount, 0);
        EXPECT_EQ(output.assetAmount, 0);
    }
}
