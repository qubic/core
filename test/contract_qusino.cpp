#define NO_UEFI

#include "contract_testing.h"

static constexpr uint64 QUSINO_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QUSINO_TRANSFER_ASSET_FEE = 100ull;
static constexpr uint64 QUSINO_TRANSFER_RIGHTS_FEE = 100ull;

static const id QUSINO_CONTRACT_ID(QUSINO_CONTRACT_INDEX, 0, 0, 0);

const id QUSINO_testUser1 = ID(_U, _S, _E, _R, _A, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_testUser2 = ID(_U, _S, _E, _R, _B, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_testUser3 = ID(_U, _S, _E, _R, _C, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
const id QUSINO_QSTIssuer = ID(_Q, _M, _H, _J, _N, _L, _M, _Q, _R, _I, _B, _I, _R, _E, _F, _I, _W, _V, _K, _Y, _Q, _E, _L, _B, _F, _A, _R, _B, _T, _D, _N, _Y, _K, _I, _O, _B, _O, _F, _F, _Y, _F, _G, _J, _Y, _Z, _S, _X, _J, _B, _V, _G, _B, _S, _U, _Q, _G);

class QUSINOChecker : public QUSINO
{
public:
    void checkSCInfo(const QUSINO::getSCInfo_output& output, uint64 expectedQSC, uint64 expectedSTAR, uint64 expectedBurntSTAR, uint64 expectedEpochRevenue, uint64 expectedMaxGameIndex, uint64 expectedBonusAmount)
    {
        EXPECT_EQ(output.QSCCirclatingSupply, expectedQSC);
        EXPECT_EQ(output.STARCirclatingSupply, expectedSTAR);
        EXPECT_EQ(output.burntSTAR, expectedBurntSTAR);
        EXPECT_EQ(output.epochRevenue, expectedEpochRevenue);
        EXPECT_EQ(output.maxGameIndex, expectedMaxGameIndex);
        EXPECT_EQ(output.bonusAmount, expectedBonusAmount);
    }
};

class ContractTestingQUSINO : protected ContractTesting
{
public:
    ContractTestingQUSINO()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUSINO);
        callSystemProcedure(QUSINO_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QUSINOChecker* getState()
    {
        return (QUSINOChecker*)contractStates[QUSINO_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUSINO_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, uint64 numberOfShares)
    {
        QX::IssueAsset_input input;
        input.assetName = assetName;
        input.numberOfShares = numberOfShares;
        input.unitOfMeasurement = 0;
        input.numberOfDecimalPlaces = 0;
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QUSINO_ISSUE_ASSET_FEE);
        return output.issuedNumberOfShares;
    }

    sint64 transferAsset(const id& from, const id& to, uint64 assetName, const id& issuer, uint64 numberOfShares)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = to;
        input.numberOfShares = numberOfShares;
        QX::TransferShareOwnershipAndPossession_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, QUSINO_TRANSFER_ASSET_FEE);
        return output.transferredNumberOfShares;
    }

    sint64 transferShareManagementRightsQX(const id& invocator, const Asset& asset, sint64 numberOfShares, uint32 newManagingContractIndex, sint64 fee)
    {
        QX::TransferShareManagementRights_input input;
        input.asset.assetName = asset.assetName;
        input.asset.issuer = asset.issuer;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;
        QX::TransferShareManagementRights_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, invocator, fee);
        return output.transferredNumberOfShares;
    }

    QUSINO::depositBonus_output depositBonus(const id& user, uint64 amount)
    {
        QUSINO::depositBonus_input input;
        input.amount = amount;
        QUSINO::depositBonus_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 6, input, output, user, amount);
        return output;
    }

    QUSINO::dailyClaimBonus_output dailyClaimBonus(const id& user, sint64 invocationReward)
    {
        QUSINO::dailyClaimBonus_input input;
        QUSINO::dailyClaimBonus_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 7, input, output, user, invocationReward);
        return output;
    }

    QUSINO::earnSTAR_output earnSTAR(const id& user, uint64 amount, sint64 invocationReward)
    {
        QUSINO::earnSTAR_input input;
        input.amount = amount;
        QUSINO::earnSTAR_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 1, input, output, user, invocationReward);
        return output;
    }

    QUSINO::transferSTAROrQSC_output transferSTAROrQSC(const id& user, const id& dest, uint64 amount, uint8 type, sint64 invocationReward)
    {
        QUSINO::transferSTAROrQSC_input input;
        input.dest = dest;
        input.amount = amount;
        input.type = type;
        QUSINO::transferSTAROrQSC_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 2, input, output, user, invocationReward);
        return output;
    }

    QUSINO::submitGame_output submitGame(const id& user, const Array<uint8, 64>& URI, sint64 invocationReward)
    {
        QUSINO::submitGame_input input;
        copyMemory(input.URI, URI);
        QUSINO::submitGame_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 3, input, output, user, invocationReward);
        return output;
    }

    QUSINO::voteInGameProposal_output voteInGameProposal(const id& user, const Array<uint8, 64>& URI, uint64 gameIndex, uint8 yesNo, sint64 invocationReward)
    {
        QUSINO::voteInGameProposal_input input;
        copyMemory(input.URI, URI);
        input.gameIndex = gameIndex;
        input.yesNo = yesNo;
        QUSINO::voteInGameProposal_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 4, input, output, user, invocationReward);
        return output;
    }

    QUSINO::TransferShareManagementRights_output TransferShareManagementRights(const id& user, const Asset& asset, uint64 numberOfShares, uint32 newManagingContractIndex, sint64 invocationReward)
    {
        QUSINO::TransferShareManagementRights_input input;
        input.asset = asset;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;
        QUSINO::TransferShareManagementRights_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 5, input, output, user, invocationReward);
        return output;
    }

    QUSINO::redemptionQSCToQubic_output redemptionQSCToQubic(const id& user, uint64 amount, sint64 invocationReward)
    {
        QUSINO::redemptionQSCToQubic_input input;
        input.amount = amount;
        QUSINO::redemptionQSCToQubic_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 8, input, output, user, invocationReward);
        return output;
    }

    QUSINO::getUserAssetVolume_output getUserAssetVolume(const id& user)
    {
        QUSINO::getUserAssetVolume_input input;
        input.user = user;
        QUSINO::getUserAssetVolume_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QUSINO::getFailedGameList_output getFailedGameList(uint32 offset)
    {
        QUSINO::getFailedGameList_input input;
        input.offset = offset;
        QUSINO::getFailedGameList_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QUSINO::getSCInfo_output getSCInfo()
    {
        QUSINO::getSCInfo_input input;
        QUSINO::getSCInfo_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QUSINO::getActiveGameList_output getActiveGameList(uint32 offset)
    {
        QUSINO::getActiveGameList_input input;
        input.offset = offset;
        QUSINO::getActiveGameList_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QUSINO::getProposerEarnedQSCInfo_output getProposerEarnedQSCInfo(const id& proposer, uint32 epoch)
    {
        QUSINO::getProposerEarnedQSCInfo_input input;
        input.proposer = proposer;
        input.epoch = epoch;
        QUSINO::getProposerEarnedQSCInfo_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 5, input, output);
        return output;
    }
};

// Helper function to create a URI
Array<uint8, 64> createURI(const char* str)
{
    Array<uint8, 64> URI;
    uint32 len = 0;
    while (str[len] != '\0' && len < 64) len++;
    for (uint32 i = 0; i < 64; i++)
    {
        if (i < len)
            URI.set(i, (uint8)str[i]);
        else
            URI.set(i, 0);
    }
    return URI;
}

TEST(ContractQUSINO, earnSTAR_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE * 100;
    
    increaseEnergy(user, requiredReward);
    
    QUSINO::earnSTAR_output output = QUSINO.earnSTAR(user, amount, requiredReward);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check user's STAR amount
    QUSINO::getUserAssetVolume_output userVolume = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(userVolume.STARAmount, amount * 100);
    
    // earnSTAR also grants amount QSC (1:1 with STAR amount in logical units)
    EXPECT_EQ(userVolume.QSCAmount, amount);

    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.STARCirclatingSupply, amount * 100);
    EXPECT_EQ(scInfo.QSCCirclatingSupply, amount);
}

TEST(ContractQUSINO, earnSTAR_InsufficientFunds)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount = 1000;
    sint64 insufficientReward = amount * QUSINO_STAR_PRICE * 100 - 1;
    
    increaseEnergy(user, insufficientReward);
    
    QUSINO::earnSTAR_output output = QUSINO.earnSTAR(user, amount, insufficientReward);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_FUNDS);
}

TEST(ContractQUSINO, transferSTAROrQSC_STAR_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id sender = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    uint64 amount = 1000;
    // amount is in logical STAR units; earnSTAR uses amount*100 internally
    sint64 requiredReward = amount * QUSINO_STAR_PRICE * 100;
    
    // First earn STAR
    increaseEnergy(sender, requiredReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(sender, amount, requiredReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Transfer all earned STAR (amount * 100 units)
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount * 100, QUSINO_ASSET_TYPE_STAR, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check balances
    QUSINO::getUserAssetVolume_output senderVolume = QUSINO.getUserAssetVolume(sender);
    QUSINO::getUserAssetVolume_output receiverVolume = QUSINO.getUserAssetVolume(receiver);
    EXPECT_EQ(senderVolume.STARAmount, 0);
    EXPECT_EQ(receiverVolume.STARAmount, amount * 100);
}

TEST(ContractQUSINO, transferSTAROrQSC_QSC_Success)
{
    ContractTestingQUSINO QUSINO;

    id sender = QUSINO_testUser2;
    id receiver = QUSINO_testUser3;
    uint64 amount = 5000;

    // Earn STAR (and get equal amount of QSC) for sender
    sint64 requiredReward = amount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(sender, requiredReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(sender, amount, requiredReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);

    // Transfer QSC from sender to receiver
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, QUSINO_ASSET_TYPE_QSC, 1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);

    // Check balances
    QUSINO::getUserAssetVolume_output senderVolume = QUSINO.getUserAssetVolume(sender);
    QUSINO::getUserAssetVolume_output receiverVolume = QUSINO.getUserAssetVolume(receiver);
    EXPECT_EQ(senderVolume.QSCAmount, 0);
    EXPECT_EQ(receiverVolume.QSCAmount, amount);
}

TEST(ContractQUSINO, transferSTAROrQSC_InvalidGameProposer)
{
    ContractTestingQUSINO QUSINO;

    id proposer = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");

    // Proposer submits a game (has active game)
    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    QUSINO::submitGame_output subOut = QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(subOut.returnCode, QUSINO_SUCCESS);

    // Proposer earns STAR and QSC
    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(proposer, requiredReward);
    QUSINO::earnSTAR_output earnOut = QUSINO.earnSTAR(proposer, amount, requiredReward);
    EXPECT_EQ(earnOut.returnCode, QUSINO_SUCCESS);

    // Proposer cannot transfer while they have an active game proposal
    increaseEnergy(proposer, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(proposer, receiver, amount, QUSINO_ASSET_TYPE_QSC, 1);
    EXPECT_EQ(output.returnCode, QUSINO_INVALID_GAME_PROPOSER);
}

TEST(ContractQUSINO, transferSTAROrQSC_InsufficientSTAR)
{
    ContractTestingQUSINO QUSINO;
    
    id sender = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    uint64 amount = 1000;
    
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, QUSINO_ASSET_TYPE_STAR, 1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_STAR);
}

TEST(ContractQUSINO, transferSTAROrQSC_InsufficientQSC)
{
    ContractTestingQUSINO QUSINO;
    
    id sender = QUSINO_testUser1;
    id receiver = QUSINO_testUser2;
    uint64 amount = 1000;
    
    increaseEnergy(sender, 1);
    QUSINO::transferSTAROrQSC_output output = QUSINO.transferSTAROrQSC(sender, receiver, amount, QUSINO_ASSET_TYPE_QSC, 1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QSC);
}

TEST(ContractQUSINO, submitGame_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    
    increaseEnergy(user, requiredReward);
    QUSINO::submitGame_output output = QUSINO.submitGame(user, URI, requiredReward);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check game was added
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    EXPECT_EQ(gameList.gameIndexes.get(0), 1);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.maxGameIndex, 2); // Starts at 1, so first game is index 1
    uint64 expectedEpochRevenue = QUSINO_GAME_SUBMIT_FEE - div<uint64>(QUSINO_GAME_SUBMIT_FEE, 676ULL * 10) * 676ULL;
    EXPECT_EQ(scInfo.epochRevenue, expectedEpochRevenue);
}

TEST(ContractQUSINO, submitGame_InsufficientFunds)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    sint64 insufficientReward = QUSINO_GAME_SUBMIT_FEE - 1;
    
    increaseEnergy(user, insufficientReward);
    QUSINO::submitGame_output output = QUSINO.submitGame(user, URI, insufficientReward);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_FUNDS);
}

TEST(ContractQUSINO, voteInGameProposal_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    
    // First submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    // Earn STAR for voting
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(voter, starReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(voter, starAmount, starReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Vote on the game
    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_SUCCESS);
    
    // Check vote was recorded
    QUSINO::getActiveGameList_output updatedGameList = QUSINO.getActiveGameList(0);
    // Note: We can't directly check votes, but we can verify the game still exists
    EXPECT_GT(updatedGameList.gameIndexes.get(0), 0);
}

TEST(ContractQUSINO, voteInGameProposal_InsufficientVoteFee)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    
    // Submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    // Try to vote without enough STAR
    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_INSUFFICIENT_VOTE_FEE);
}

TEST(ContractQUSINO, voteInGameProposal_WrongGameURI)
{
    ContractTestingQUSINO QUSINO;
    
    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI1 = createURI("https://example.com/game1");
    Array<uint8, 64> URI2 = createURI("https://example.com/game2");
    
    // Submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI1, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    // Earn STAR for voting
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(voter, starReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(voter, starAmount, starReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);
    
    // Try to vote with wrong URI
    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI2, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_WRONG_GAME_URI_FOR_VOTE);
}

TEST(ContractQUSINO, getUserAssetVolume_Empty)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    QUSINO::getUserAssetVolume_output output = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(output.STARAmount, 0);
    EXPECT_EQ(output.QSCAmount, 0);
}

TEST(ContractQUSINO, redemptionQSCToQubic_Success)
{
    ContractTestingQUSINO QUSINO;

    id user = QUSINO_testUser1;
    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(user, requiredReward);
    QUSINO::earnSTAR_output earnOut = QUSINO.earnSTAR(user, amount, requiredReward);
    EXPECT_EQ(earnOut.returnCode, QUSINO_SUCCESS);

    uint64 redeemAmount = 500;
    QUSINO::redemptionQSCToQubic_output output = QUSINO.redemptionQSCToQubic(user, redeemAmount, 0);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);

    QUSINO::getUserAssetVolume_output vol = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(vol.QSCAmount, amount - redeemAmount);
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.QSCCirclatingSupply, amount - redeemAmount);
}

TEST(ContractQUSINO, redemptionQSCToQubic_InsufficientQSC)
{
    ContractTestingQUSINO QUSINO;

    id user = QUSINO_testUser1;
    increaseEnergy(user, 1);
    QUSINO::redemptionQSCToQubic_output output = QUSINO.redemptionQSCToQubic(user, 100, 0);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QSC);
}

TEST(ContractQUSINO, redemptionQSCToQubic_InvalidGameProposer)
{
    ContractTestingQUSINO QUSINO;

    id proposer = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    QUSINO::submitGame_output subOut = QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(subOut.returnCode, QUSINO_SUCCESS);

    uint64 amount = 1000;
    sint64 requiredReward = amount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(proposer, requiredReward);
    QUSINO::earnSTAR_output earnOut = QUSINO.earnSTAR(proposer, amount, requiredReward);
    EXPECT_EQ(earnOut.returnCode, QUSINO_SUCCESS);

    QUSINO::redemptionQSCToQubic_output output = QUSINO.redemptionQSCToQubic(proposer, 100, 0);
    EXPECT_EQ(output.returnCode, QUSINO_INVALID_GAME_PROPOSER);
}

TEST(ContractQUSINO, END_EPOCH_FailedGameRemoval)
{
    ContractTestingQUSINO QUSINO;

    // issue QST
    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    
    id proposer = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");
    
    // Submit a game
    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);
    
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    
    // Vote no to make it fail
    id voter1 = QUSINO_testUser2;
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(voter1, starReward);
    QUSINO::earnSTAR_output earnOutput1 = QUSINO.earnSTAR(voter1, starAmount, starReward);
    EXPECT_EQ(earnOutput1.returnCode, QUSINO_SUCCESS);
    
    increaseEnergy(voter1, 1);
    QUSINO::voteInGameProposal_output voteOutput1 = QUSINO.voteInGameProposal(voter1, URI, gameIndex, 2, 0);
    EXPECT_EQ(voteOutput1.returnCode, QUSINO_SUCCESS);
    
    // End epoch - game should be moved to failed list if no votes >= yes votes
    QUSINO.endEpoch();
    ++system.epoch;
    
    // Check failed game list
    QUSINO::getFailedGameList_output failedList = QUSINO.getFailedGameList(0);
    // Game should be in failed list
}

TEST(ContractQUSINO, END_EPOCH_ProposerEarnedQSCInfo)
{
    ContractTestingQUSINO QUSINO;

    // issue QST
    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game2");

    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    QUSINO::submitGame_output subOut = QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(subOut.returnCode, QUSINO_SUCCESS);

    uint64 qscAmount = 500;
    sint64 starReward = qscAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(proposer, starReward);
    QUSINO::earnSTAR_output earnOut = QUSINO.earnSTAR(proposer, qscAmount, starReward);
    EXPECT_EQ(earnOut.returnCode, QUSINO_SUCCESS);

    uint32 epochBeforeEnd = system.epoch;
    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    QUSINO::earnSTAR_output voterEarn = QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(voterEarn.returnCode, QUSINO_SUCCESS);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOut = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 0);
    EXPECT_EQ(voteOut.returnCode, QUSINO_SUCCESS);

    QUSINO.endEpoch();
    ++system.epoch;

    QUSINO::getProposerEarnedQSCInfo_output info = QUSINO.getProposerEarnedQSCInfo(proposer, epochBeforeEnd);
    EXPECT_EQ(info.earnedQSC, qscAmount);
}

TEST(ContractQUSINO, depositBonus_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    uint64 amount1 = 1000;
    uint64 amount2 = 500;
    
    // Initial bonusAmount
    QUSINO::getSCInfo_output scInfo0 = QUSINO.getSCInfo();
    uint64 initialBonus = scInfo0.bonusAmount;
    
    // First deposit
    increaseEnergy(user, amount1);
    QUSINO::depositBonus_output output1 = QUSINO.depositBonus(user, amount1);
    EXPECT_EQ(output1.returnCode, QUSINO_SUCCESS);
    
    QUSINO::getSCInfo_output scInfo1 = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo1.bonusAmount, initialBonus + amount1);
    
    // Second deposit
    increaseEnergy(user, amount2);
    QUSINO::depositBonus_output output2 = QUSINO.depositBonus(user, amount2);
    EXPECT_EQ(output2.returnCode, QUSINO_SUCCESS);
    
    QUSINO::getSCInfo_output scInfo2 = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo2.bonusAmount, initialBonus + amount1 + amount2);
}

TEST(ContractQUSINO, dailyClaimBonus_Success)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    
    // Fund bonus pool
    uint64 bonusFund = QUSINO_BONUS_CLAIM_AMOUNT * 10;
    increaseEnergy(user, bonusFund);
    QUSINO::depositBonus_output depOutput = QUSINO.depositBonus(user, bonusFund);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Set current time
    setMemory(utcTime, 0);
    utcTime.Year = 2024;
    utcTime.Month = 1;
    utcTime.Day = 1;
    utcTime.Hour = 0;
    utcTime.Minute = 0;
    utcTime.Second = 0;
    updateQpiTime();
    
    // Snapshot before claim
    QUSINO::getSCInfo_output scInfoBefore = QUSINO.getSCInfo();
    QUSINO::getUserAssetVolume_output volBefore = QUSINO.getUserAssetVolume(user);
    
    // First claim
    QUSINO::dailyClaimBonus_output output = QUSINO.dailyClaimBonus(user, 0);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    
    // Check user balances
    QUSINO::getUserAssetVolume_output volAfter = QUSINO.getUserAssetVolume(user);
    EXPECT_EQ(volAfter.STARAmount, volBefore.STARAmount + QUSINO_BONUS_CLAIM_AMOUNT_STAR);
    EXPECT_EQ(volAfter.QSCAmount, volBefore.QSCAmount + QUSINO_BONUS_CLAIM_AMOUNT_QSC);
    
    // Check SC info
    QUSINO::getSCInfo_output scInfoAfter = QUSINO.getSCInfo();
    EXPECT_EQ(scInfoAfter.bonusAmount, scInfoBefore.bonusAmount - QUSINO_BONUS_CLAIM_AMOUNT);
    EXPECT_EQ(scInfoAfter.STARCirclatingSupply, scInfoBefore.STARCirclatingSupply + QUSINO_BONUS_CLAIM_AMOUNT_STAR);
    EXPECT_EQ(scInfoAfter.QSCCirclatingSupply, scInfoBefore.QSCCirclatingSupply + QUSINO_BONUS_CLAIM_AMOUNT_QSC);
    EXPECT_EQ(scInfoAfter.epochRevenue, scInfoBefore.epochRevenue + QUSINO_BONUS_CLAIM_AMOUNT_STAR);
}

TEST(ContractQUSINO, dailyClaimBonus_AlreadyClaimedToday)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    
    // Fund bonus pool
    uint64 bonusFund = QUSINO_BONUS_CLAIM_AMOUNT * 10;
    increaseEnergy(user, bonusFund);
    QUSINO::depositBonus_output depOutput = QUSINO.depositBonus(user, bonusFund);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Set current time
    setMemory(utcTime, 0);
    utcTime.Year = 2024;
    utcTime.Month = 1;
    utcTime.Day = 1;
    utcTime.Hour = 0;
    utcTime.Minute = 0;
    utcTime.Second = 0;
    updateQpiTime();
    
    // First claim
    QUSINO::dailyClaimBonus_output output1 = QUSINO.dailyClaimBonus(user, 0);
    EXPECT_EQ(output1.returnCode, QUSINO_SUCCESS);
    
    // Second claim on same day should fail
    QUSINO::dailyClaimBonus_output output2 = QUSINO.dailyClaimBonus(user, 0);
    EXPECT_EQ(output2.returnCode, QUSINO_ALREADY_CLAIMED_TODAY);
}

TEST(ContractQUSINO, dailyClaimBonus_BonusClaimTimeNotCome)
{
    ContractTestingQUSINO QUSINO;
    
    id user1 = QUSINO_testUser1;
    id user2 = QUSINO_testUser2;
    
    // Fund bonus pool
    uint64 bonusFund = QUSINO_BONUS_CLAIM_AMOUNT * 10;
    increaseEnergy(user1, bonusFund);
    QUSINO::depositBonus_output depOutput = QUSINO.depositBonus(user1, bonusFund);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);
    
    // Set current time
    setMemory(utcTime, 0);
    utcTime.Year = 2026;
    utcTime.Month = 1;
    utcTime.Day = 1;
    utcTime.Hour = 0;
    utcTime.Minute = 0;
    utcTime.Second = 0;
    updateQpiTime();
    
    // First claim by user1
    QUSINO::dailyClaimBonus_output output1 = QUSINO.dailyClaimBonus(user1, 0);
    EXPECT_EQ(output1.returnCode, QUSINO_SUCCESS);
    
    // Immediate claim by user2 should fail due to global cooldown
    increaseEnergy(user2, 1);
    QUSINO::dailyClaimBonus_output output2 = QUSINO.dailyClaimBonus(user2, 0);
    EXPECT_EQ(output2.returnCode, QUSINO_BONUS_CLAIM_TIME_NOT_COME);
}

TEST(ContractQUSINO, dailyClaimBonus_InsufficientBonusAmount)
{
    ContractTestingQUSINO QUSINO;
    
    id user = QUSINO_testUser1;
    
    // Set current time
    setMemory(utcTime, 0);
    utcTime.Year = 2026;
    utcTime.Month = 1;
    utcTime.Day = 1;
    utcTime.Hour = 0;
    utcTime.Minute = 0;
    utcTime.Second = 0;
    updateQpiTime();
    
    // No bonus deposited -> insufficient bonus amount
    increaseEnergy(user, 1);
    QUSINO::dailyClaimBonus_output output = QUSINO.dailyClaimBonus(user, 0);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_BONUS_AMOUNT);
}
