#define NO_UEFI

#include "contract_testing.h"

//#define PRINT_DETAILS 0

static constexpr uint64 QSWAP_ISSUE_ASSET_FEE = 10000000;
static constexpr uint64 QSWAP_CREATE_POOL_FEE = 10000000;

static const id QSWAP_CONTRACT_ID(QSWAP_CONTRACT_INDEX, 0, 0, 0);

//constexpr uint32 SWAP_FEE_IDX = 1;
constexpr uint32 GET_POOL_BASIC_STATE_IDX = 2;
constexpr uint32 GET_LIQUDITY_OF_IDX = 3;
constexpr uint32 QUOTE_EXACT_QU_INPUT_IDX = 4;
//
constexpr uint32 ISSUE_ASSET_IDX = 1;
// constexpr uint32 TRANSFER_SHARE_OWNERSHIP_AND_POSSESSION_IDX = 2;
constexpr uint32 CREATE_POOL_IDX = 3;
constexpr uint32 ADD_LIQUDITY_IDX = 4;
constexpr uint32 REMOVE_LIQUDITY_IDX = 5;
constexpr uint32 SWAP_EXACT_QU_FOR_ASSET_IDX = 6;
constexpr uint32 SWAP_QU_FOR_EXACT_ASSET_IDX = 7;
constexpr uint32 SWAP_EXACT_ASSET_FOR_QU_IDX = 8;
constexpr uint32 SWAP_ASSET_FOR_EXACT_QU_IDX = 9;

//std::string assetNameFromInt64(uint64 assetName);

class QswapChecker : public QSWAP
{
//public:
    //void checkCollectionConsistency() {
    //}
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
    }

    QswapChecker* getState()
    {
        return (QswapChecker*)contractStates[QSWAP_CONTRACT_INDEX];
    }

    bool loadState(const CHAR16* filename)
    {
        return load(filename, sizeof(QSWAP), contractStates[QSWAP_CONTRACT_INDEX]) == sizeof(QSWAP);
    }

    sint64 issueAsset(const id& issuer, QSWAP::IssueAsset_input input){
        QSWAP::IssueAsset_output output;
        invokeUserProcedure(QSWAP_CONTRACT_INDEX, ISSUE_ASSET_IDX, input, output, issuer, QSWAP_ISSUE_ASSET_FEE);
        return output.issuedNumberOfShares;
    }

    bool createPool(const id& issuer, uint64 assetName){
        QSWAP::CreatePool_input input{issuer, assetName};
        QSWAP::CreatePool_output output;
        invokeUserProcedure(QSWAP_CONTRACT_INDEX, CREATE_POOL_IDX, input, output, issuer, QSWAP_CREATE_POOL_FEE);
        return output.success;
    }

    QSWAP::GetPoolBasicState_output getPoolBasicState(const id& issuer, uint64 assetName){
        QSWAP::GetPoolBasicState_input input{issuer, assetName};
        QSWAP::GetPoolBasicState_output output;

        callFunction(QSWAP_CONTRACT_INDEX, GET_POOL_BASIC_STATE_IDX, input, output);
        return output;
    }

    QSWAP::AddLiqudity_output addLiqudity(const id& issuer, QSWAP::AddLiqudity_input input, uint64 inputValue){
        QSWAP::AddLiqudity_output output;
        invokeUserProcedure(
            QSWAP_CONTRACT_INDEX,
            ADD_LIQUDITY_IDX,
            input,
            output,
            issuer,
            inputValue
        );
        return output;
    }

    QSWAP::RemoveLiqudity_output removeLiqudity(const id& issuer, QSWAP::RemoveLiqudity_input input, uint64 inputValue){
        QSWAP::RemoveLiqudity_output output;
        invokeUserProcedure(
            QSWAP_CONTRACT_INDEX,
            REMOVE_LIQUDITY_IDX,
            input,
            output,
            issuer,
            inputValue
        );
        return output;
    }

    QSWAP::GetLiqudityOf_output getLiqudityOf(QSWAP::GetLiqudityOf_input input){
        QSWAP::GetLiqudityOf_output output;
        callFunction(QSWAP_CONTRACT_INDEX, GET_LIQUDITY_OF_IDX, input, output);
        return output;
    }

    QSWAP::SwapExactQuForAsset_output swapExactQuForAsset( const id& issuer, QSWAP::SwapExactQuForAsset_input input, uint64 inputValue) {
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

    QSWAP::SwapQuForExactAsset_output swapQuForExactAsset( const id& issuer, QSWAP::SwapQuForExactAsset_input input, uint64 inputValue) {
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

    QSWAP::SwapExactAssetForQu_output swapExactAssetForQu(const id& issuer, QSWAP::SwapExactAssetForQu_input input, uint64 inputValue) {
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

    QSWAP::SwapAssetForExactQu_output swapAssetForExactQu(const id& issuer, QSWAP::SwapAssetForExactQu_input input, uint64 inputValue) {
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
};

TEST(ContractSwap, SwapStuff)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    uint64 invalidAssetName = assetNameFromString("QSWAP1");
    sint64 numberOfShares = 1000 * 10000;


    // issue an asset and create a pool, and init liqudity
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));

        sint64 inputValue = 1000*200;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiqudity_input alInput = { issuer, assetName, 1000*100, 0, 0 };
        QSWAP::AddLiqudity_output output = qswap.addLiqudity(issuer, alInput, inputValue);
        printf("delta liqudity: %lld, %lld, %lld\n", output.userIncreaseLiqudity, output.assetAmount, output.quAmount);
    }

    // swapExactQuForAsset 
    {
        // swap in 1000*100 qu, get about 1000*50 asset
        id user(2,3,4,5);
        sint64 inputValue = 1000*200;
        increaseEnergy(user, inputValue);
        QSWAP::SwapExactQuForAsset_input input = {issuer, assetName, 0};
        QSWAP::SwapExactQuForAsset_output output = qswap.swapExactQuForAsset(user, input, inputValue);
        printf("swap asset out: %lld\n", output.assetAmountOut);
        EXPECT_TRUE(output.assetAmountOut <= 50000); // 49924 if swapFee 0.3%

        QSWAP::GetPoolBasicState_output psOutput = qswap.getPoolBasicState(issuer, assetName);
        printf("%lld, %lld, %lld\n", psOutput.reservedAssetAmount, psOutput.reservedQuAmount, psOutput.totalLiqudity);
        EXPECT_TRUE(psOutput.reservedQuAmount >= (200000*2 - 200)); // 399880 if swapFee 0.3%, protocolFee 20%
        EXPECT_TRUE(psOutput.reservedAssetAmount >= 50000 ); // 50076
        EXPECT_EQ(psOutput.totalLiqudity, 141421); // liqudity stay the same
    }

    // // swapQuForExactAsset 
    // {
    //     id user(2,3,4,5);
    //     sint64 inputValue = 1000 * 200;
    //     increaseEnergy(user, inputValue);
    //     QSWAP::SwapQuForExactAsset_input input = {issuer, assetName, 50*1000};
    //     QSWAP::SwapQuForExactAsset_output output = qswap.swapQuForExactAsset(user, input, inputValue);
    //     printf("swap qu in: %lld\n", output.quAmountIn);
    // }

    // // swapExactAssetForQu
    // {
    //     id user(1, 2,3,4);
    //     sint64 inputValue = 0;
    //     increaseEnergy(user, inputValue);
    //     QSWAP::SwapExactAssetForQu_input input = {issuer, assetName, 100*1000, 0};
    //     QSWAP::SwapExactAssetForQu_output output = qswap.swapExactAssetForQu(user, input, inputValue);
    //     printf("swap qu out: %lld\n", output.quAmountOut);
    // }

    // // swapAssetForExactQu
    // {
    //     id user(1,2,3,4);
    //     sint64 inputValue = 0;
    //     increaseEnergy(user, inputValue);
    //     QSWAP::SwapAssetForExactQu_input input = {issuer, assetName, 200*1000 + 10, 100*1000};
    //     QSWAP::SwapAssetForExactQu_output output = qswap.swapAssetForExactQu(user, input, inputValue);
    //     printf("swap asset in: %lld\n", output.assetAmountIn);
    // }
}

/*
0. normally issue asset
1. not enough qu for asset issue fee
2. issue duplicate asset
3. issue asset with invalid input params, such as numberOfShares: 0
*/
// TEST(ContractSwap, IssueAsset)
// {
//     ContractTestingQswap qswap;

//     id issuer(1, 2, 3, 4);

//     // 0. normally issue asset
//     {
//         increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
//         uint64 assetName = assetNameFromString("QSWAP0");
//         sint64 numberOfShares = 1000000;
//         QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
//         EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), 0);
//         EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
//         EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);
//         EXPECT_EQ(getBalance(QSWAP_CONTRACT_ID), QSWAP_ISSUE_ASSET_FEE);
//     }

//     // 1. not enough energy for asset issue fee
//     {
//         decreaseEnergy(spectrumIndex(issuer), getBalance(issuer));
//         uint64 assetName = assetNameFromString("QSWAP1");
//         sint64 numberOfShares = 1000000;
//         QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
//         EXPECT_EQ(qswap.issueAsset(issuer, input), 0);
//     }

//     // 2. issue duplicate asset, related to test.0
//     {
//         increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
//         uint64 assetName = assetNameFromString("QSWAP0");
//         sint64 numberOfShares = 1000000;
//         QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
//         EXPECT_EQ(qswap.issueAsset(issuer, input), 0);
//     }

//     // 3. issue asset with invalid input params, such as numberOfShares: 0
//     {
//         increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
//         uint64 assetName = assetNameFromString("QSWAP1");
//         sint64 numberOfShares = 0;
//         QSWAP::IssueAsset_input input = {assetName, numberOfShares, 0, 0 };
//         EXPECT_EQ(qswap.issueAsset(issuer, input), 0);
//     }
// }

/*
0. check pool state before create
1. normal create pool, check pool existance, pool states
2. create duplicate pool
3. create pool with invalid asset
*/
// TEST(ContractSwap, CreatePool)
// {
//     ContractTestingQswap qswap;

//     id issuer(1, 2, 3, 4);
//     uint64 assetName = assetNameFromString("QSWAP0");
//     sint64 numberOfShares = 1000000;

//     // issue asset first
//     {
//         increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
//         QSWAP::IssueAsset_input input = {assetName, numberOfShares, 0, 0 };
//         EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
//         EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);
//     }

//     // 0. check not exsit pool state before create
//     {
//         QSWAP::GetPoolBasicState_output output = qswap.getPoolBasicState(issuer, assetName);
//         EXPECT_FALSE(output.poolExists);
//     }

//     // 1. normal create pool, check pool existance, pool states
//     {
//         increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
//         EXPECT_TRUE(qswap.createPool(issuer, assetName));

//         // initial pool state
//         QSWAP::GetPoolBasicState_output output = qswap.getPoolBasicState(issuer, assetName);
//         EXPECT_EQ(output.poolExists, true);
//         EXPECT_EQ(output.reservedQuAmount, 0);
//         EXPECT_EQ(output.reservedAssetAmount, 0);
//         EXPECT_EQ(output.totalLiqudity, 0);
//     }

//     // 2. create duplicate pool
//     EXPECT_FALSE(qswap.createPool(issuer, assetName));

//     // 3. ceate pool with not issued asset
//     {
//         uint64 assetName2 = assetNameFromString("QswapX");
//         EXPECT_FALSE(qswap.createPool(issuer, assetName2));
//     }
// }

/*
1. add liqudity first time 
2. add liqudity second time
3. add liqudity from user1, user2

4. addLiqudity to invalid pool
5. addLiqudity with not enough qu
5. addLiqudity with not enough asset
6. addLiqudity failed if not satisfy quAmountMin/assetAmountMin
7. addLiqudity with quAmountDesired/assetAmountDesired = 0
8. addLiqudity which satisfy quAmountDesired >= quAmountMin
*/
TEST(ContractSwap, LiqudityStuff)
{
    ContractTestingQswap qswap;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QSWAP0");
    uint64 invalidAssetName = assetNameFromString("QSWAP1");
    sint64 numberOfShares = 1000 *1000;

    // 0. issue an asset and create a pool
    {
        increaseEnergy(issuer, QSWAP_ISSUE_ASSET_FEE);
        QSWAP::IssueAsset_input input = { assetName, numberOfShares, 0, 0 };
        EXPECT_EQ(qswap.issueAsset(issuer, input), numberOfShares);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), numberOfShares);

        increaseEnergy(issuer, QSWAP_CREATE_POOL_FEE);
        EXPECT_TRUE(qswap.createPool(issuer, assetName));
    }

    // 1. add liqudity to a initial pool, first add, afterward add
    // {
    //     sint64 quStakeAmount = 1000*200;
    //     sint64 inputValue = quStakeAmount;
    //     sint64 assetStakeAmount = 1000*100;
    //     increaseEnergy(issuer, quStakeAmount);
    //     QSWAP::AddLiqudity_input input = {
    //         issuer,
    //         assetName,
    //         assetStakeAmount,
    //         0,
    //         0
    //     };

    //     QSWAP::AddLiqudity_output output = qswap.addLiqudity(issuer, input, inputValue);
    //     // actually, 141421 liqudity add to the pool, but the first 1000 liqudity is retainedd by the pool rather than staker
    //     EXPECT_EQ(output.userIncreaseLiqudity, 140421); 
    //     EXPECT_EQ(output.quAmount, 1000*200);
    //     EXPECT_EQ(output.assetAmount, 1000*100);

    //     QSWAP::GetPoolBasicState_output output2 = qswap.getPoolBasicState(issuer, assetName);
    //     EXPECT_EQ(output2.poolExists, true);
    //     EXPECT_EQ(output2.reservedQuAmount, 1000*200);
    //     EXPECT_EQ(output2.reservedAssetAmount, 1000*100);
    //     EXPECT_EQ(output2.totalLiqudity, 141421);
    //     // printf("pool state: %lld, %lld, %lld\n", output2.reservedQuAmount, output2.reservedAssetAmount, output2.totalLiqudity);

    //     /* temporary disable
    //     QSWAP::GetLiqudityOf_input liqInput = {
    //        issuer,
    //        assetName,
    //        issuer
    //     };
    //     QSWAP::GetLiqudityOf_output liqOutput = qswap.getLiqudityOf(liqInput);
    //     EXPECT_EQ(liqOutput.liqudity, 0);
    //     */

    //     increaseEnergy(issuer, quStakeAmount);
    //     QSWAP::AddLiqudity_input input2 = {
    //         issuer,
    //         assetName,
    //         assetStakeAmount,
    //         0,
    //         0
    //     };

    //     QSWAP::AddLiqudity_output output3 = qswap.addLiqudity(issuer, input, inputValue);
    //     EXPECT_EQ(output3.userIncreaseLiqudity, 141421);
    //     EXPECT_EQ(output3.quAmount, 1000*200);
    //     EXPECT_EQ(output3.assetAmount, 1000*100);
    // }

    // 2. add liqudity to non 0 liqudity pool, and remove liqudity from the pool, combine with section 0
    {
        sint64 quStakeAmount = 1000*200;
        sint64 inputValue = quStakeAmount;
        sint64 assetStakeAmount = 1000*100;
        increaseEnergy(issuer, inputValue);
        QSWAP::AddLiqudity_input alInput = {
            issuer,
            assetName,
            assetStakeAmount,
            0,
            0 
        };
        
        // after the first 1000 liqudity, the liqudity belongs to the invocator()
        QSWAP::AddLiqudity_output alOutput = qswap.addLiqudity(issuer, alInput, inputValue);
        EXPECT_EQ(alOutput.userIncreaseLiqudity, 140421);
        EXPECT_EQ(alOutput.quAmount, 1000*200);
        EXPECT_EQ(alOutput.assetAmount, 1000*100);

        QSWAP::GetPoolBasicState_output psOutput = qswap.getPoolBasicState(issuer, assetName);
        EXPECT_EQ(psOutput.poolExists, true);
        EXPECT_EQ(psOutput.reservedQuAmount, 1000*200);
        EXPECT_EQ(psOutput.reservedAssetAmount, 1000*100);
        EXPECT_EQ(psOutput.totalLiqudity, 141421);

        /* temporary disabled
        QSWAP::GetLiqudityOf_input liqInput = {
           assetIssuer: issuer,
           assetName: assetName,
           acount: issuer
        };
        QSWAP::GetLiqudityOf_output liqOutput = qswap.getLiqudityOf(liqInput);
        EXPECT_EQ(liqOutput.liqudity, 1000);
        */

        sint64 burnLiqudity = 10000;
        QSWAP::RemoveLiqudity_input rlInput = {
            issuer,
            assetName,
            burnLiqudity,
            0,
            0,
        };
        QSWAP::RemoveLiqudity_output rlOutput = qswap.removeLiqudity(issuer, rlInput, 0);
        // printf("%lld, %lld\n", rlOutput.assetAmount, rlOutput.quAmount);
        EXPECT_EQ(rlOutput.assetAmount, 7071); // (10000 / 141421) * 1000_00
        EXPECT_EQ(rlOutput.quAmount, 14142);   // (10000 / 141421) * 2000_00

        psOutput = qswap.getPoolBasicState(issuer, assetName);
        // printf("%lld, %lld, %lld\n", psOutput.reservedAssetAmount, psOutput.reservedQuAmount, psOutput.totalLiqudity);
        EXPECT_EQ(psOutput.reservedQuAmount, 1000*200 - 14142);
        EXPECT_EQ(psOutput.reservedAssetAmount, 1000*100 - 7071);
        EXPECT_EQ(psOutput.totalLiqudity, 131421);
    }

    /* temporary disabled
    // 3. add liqudity to invalid pool
    {
       decreaseEnergy(getBalance(issuer));
       increaseEnergy(issuer, 1000);
       QSWAP::AddLiqudity_input input = {
           assetIssuer: issuer,
           assetName: invalidAssetName,
           assetAmountDesired: 1000,
           quAmountMin: 0,
           assetAmountMin: 0 
       };
       
       QSWAP::AddLiqudity_output output = qswap.addLiqudity(input);
       EXPECT_EQ(output.deltaLiqudity, 0);
       EXPECT_EQ(output.quAmount, 0);
       EXPECT_EQ(output.assetAmount, 0);
    }

    // 4. add liqudity with not enough qu
    {
       decreaseEnergy(getBalance(issuer));
       increaseEnergy(issuer, 2000);
       QSWAP::AddLiqudity_input input = {
           assetIssuer: issuer,
           assetName: assetName,
           assetAmountDesired: 3000,
           quAmountMin: 0,
           assetAmountMin: 0 
       };
       
       QSWAP::AddLiqudity_output output = qswap.addLiqudity(input);
       EXPECT_EQ(output.deltaLiqudity, 0);
       EXPECT_EQ(output.quAmount, 0);
       EXPECT_EQ(output.assetAmount, 0);
    }

    // 5. add liqudity with not enough asset
    {
       decreaseEnergy(getBalance(issuer));
       increaseEnergy(issuer, 5 *1000);
       QSWAP::AddLiqudity_input input = {
           assetIssuer: issuer,
           assetName: assetName,
           assetAmountDesired: 5*1000,
           quAmountMin: 0,
           assetAmountMin: 0 
       };
       
       QSWAP::AddLiqudity_output output = qswap.addLiqudity(input);
       EXPECT_EQ(output.deltaLiqudity, 0);
       EXPECT_EQ(output.quAmount, 0);
       EXPECT_EQ(output.assetAmount, 0);
    }

    // 6. add liqudity failed if not satisfy quAmountMin/assetAmountMin
    {
       decreaseEnergy(getBalance(issuer));
       increaseEnergy(issuer, 1300);
       QSWAP::AddLiqudity_input input = {
           assetIssuer: issuer,
           assetName: assetName,
           assetAmountDesired: 1000,
           quAmountMin: 1200,
           assetAmountMin: 1000 
       };
       
       QSWAP::AddLiqudity_output output = qswap.addLiqudity(input);
       EXPECT_EQ(output.deltaLiqudity, 0);
       EXPECT_EQ(output.quAmount, 0);
       EXPECT_EQ(output.assetAmount, 0);
    }

    // 6. add liqudity satisfy quAmountMin/assetAmountMin
    {
       decreaseEnergy(getBalance(issuer));
       increaseEnergy(issuer, 1300);
       QSWAP::AddLiqudity_input input = {
           assetIssuer: issuer,
           assetName: assetName,
           assetAmountDesired: 1000,
           quAmountMin: 1000,
           assetAmountMin: 1000 
       };
       
       QSWAP::AddLiqudity_output output = qswap.addLiqudity(input);
       EXPECT_EQ(output.deltaLiqudity, 1000);
       EXPECT_EQ(output.quAmount, 1000);
       EXPECT_EQ(output.assetAmount, 1000);
    }

    // 7. add liqudity with quAmountDesired/assetAmountDesired = 0
    {
       decreaseEnergy(getBalance(issuer));
       increaseEnergy(issuer, 1300);
       QSWAP::AddLiqudity_input input = {
           assetIssuer: issuer,
           assetName: assetName,
           assetAmountDesired: 0,
           quAmountMin: 0,
           assetAmountMin: 0 
       };
       
       QSWAP::AddLiqudity_output output = qswap.addLiqudity(input);
       EXPECT_EQ(output.deltaLiqudity, 0);
       EXPECT_EQ(output.quAmount, 0);
       EXPECT_EQ(output.assetAmount, 0);
    }
    */
}