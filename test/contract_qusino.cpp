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
        // RANDOM must be constructed before QUSINO so refillRandomBank's cross-contract
        // BuyEntropy call has an active contract to invoke (mirrors contract_qraffle.cpp).
        system.epoch = contractDescriptions[RANDOM_CONTRACT_INDEX].constructionEpoch;
        INIT_CONTRACT(RANDOM);
        callSystemProcedure(RANDOM_CONTRACT_INDEX, INITIALIZE);
        system.epoch = contractDescriptions[QUSINO_CONTRACT_INDEX].constructionEpoch;
        INIT_CONTRACT(QUSINO);
        callSystemProcedure(QUSINO_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QUSINOChecker* getState()
    {
        return (QUSINOChecker*)contractStates[QUSINO_CONTRACT_INDEX];
    }

    RANDOM::StateData* randomState()
    {
        return reinterpret_cast<RANDOM::StateData*>(contractStates[RANDOM_CONTRACT_INDEX]);
    }

    // Directly seeds RANDOM's finalized entropy for the stream/tier that QUSINO's
    // refillRandomBank() will read when called at the current tick (mirrors the +2
    // offset BuyEntropy itself uses to read the last-finalized stream). Lets tests make
    // the entropy purchase deterministically succeed without replaying the full
    // RevealAndCommit/END_TICK provider cycle (see contract_qraffle.cpp for precedent).
    QPI::bit_4096 seedRandomEntropy(uint64 seed)
    {
        QPI::bit_4096 entropy{};
        for (uint64 i = 0; i < QUSINO_RNG_ENTROPY_BITS; ++i)
        {
            entropy.set(i, ((seed + i) & 1ULL) != 0);
        }
        const uint32 stream = (system.tick + 2u) % 3u;
        randomState()->entropy.set(stream * 10u + QUSINO_RNG_COLLATERAL_TIER, entropy);
        return entropy;
    }

    void setTick(uint32 tick) { system.tick = tick; }
    uint32 getTick() const { return system.tick; }

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

    QUSINO::refillRandomBank_output refillRandomBank(const id& user, sint64 invocationReward = 0)
    {
        QUSINO::refillRandomBank_input input;
        QUSINO::refillRandomBank_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 9, input, output, user, invocationReward);
        return output;
    }

    QUSINO::coinFlip_output coinFlip(const id& user, uint8 guess, uint8 assetType, uint64 amount, sint64 invocationReward = 0)
    {
        QUSINO::coinFlip_input input;
        input.guess = guess;
        input.assetType = assetType;
        input.amount = amount;
        QUSINO::coinFlip_output output;
        invokeUserProcedure(QUSINO_CONTRACT_INDEX, 10, input, output, user, invocationReward);
        return output;
    }

    QUSINO::getRandomBankStatus_output getRandomBankStatus()
    {
        QUSINO::getRandomBankStatus_input input;
        QUSINO::getRandomBankStatus_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    // Funds bonusAmount (the Qu game bankroll) by having `owner` deposit `amount` --
    // mirrors how the game owner is expected to seed it in production. Also gives
    // QUSINO's real spectrum balance the matching Qu, which refillRandomBank's actual
    // cross-contract RANDOM fee transfer still separately depends on.
    void fundBonusAmount(uint64 amount)
    {
        const id gameOwner = ID(_G, _O, _W, _N, _A, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y, _Z, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V, _W, _X, _Y);
        increaseEnergy(gameOwner, (sint64)amount);
        QUSINO::depositBonus_output out = depositBonus(gameOwner, amount);
        ASSERT_EQ(out.returnCode, QUSINO_SUCCESS);
    }

    // Credits `user` with `amount` QSC (and amount*100 STAR, incidentally) via earnSTAR
    // -- the only path that mints QSC in this contract.
    void giveUserQSC(const id& user, uint64 amount)
    {
        sint64 requiredReward = (sint64)(amount * QUSINO_STAR_PRICE * 100);
        increaseEnergy(user, requiredReward);
        QUSINO::earnSTAR_output out = earnSTAR(user, amount, requiredReward);
        ASSERT_EQ(out.returnCode, QUSINO_SUCCESS);
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

// ---------------------------------------------------------------------------
// RNG Result Bank (refillRandomBank) + Coin Flip
//
// Coin Flip is funded entirely out of bonusAmount, QUSINO's Qu game bankroll (see
// the comment above QUSINO_GAME_BANKROLL_CAP in Qusino.h): the game owner funds it
// via depositBonus (QUSINO.fundBonusAmount() below), refillRandomBank's RANDOM fee
// is paid from it, and QSC bet payouts/losses flow through it. STAR bets never
// touch it at all.
// ---------------------------------------------------------------------------

TEST(ContractQUSINO, refillRandomBank_FailsWhenNoEntropyAvailable)
{
    ContractTestingQUSINO QUSINO;

    // Fund the bankroll so it *could* pay RANDOM's fee, but never seed any entropy.
    QUSINO.fundBonusAmount(1000000000ULL);
    // The caller identity must have a spectrum entry for invokeUserProcedure to route
    // the call at all, even though refillRandomBank itself takes no payment from it.
    increaseEnergy(QUSINO_testUser1, 1);

    QUSINO::refillRandomBank_output output = QUSINO.refillRandomBank(QUSINO_testUser1);
    EXPECT_EQ(output.returnCode, QUSINO_RNG_REFILL_FAILED);
    EXPECT_EQ(output.valuesAdded, 0u);

    QUSINO::getRandomBankStatus_output status = QUSINO.getRandomBankStatus();
    EXPECT_FALSE(status.poolInitialized);
    EXPECT_EQ(status.reserveFilled, 0u);
}

TEST(ContractQUSINO, refillRandomBank_FailsWhenBonusAmountInsufficient)
{
    ContractTestingQUSINO QUSINO;

    // Entropy is available, but the game bankroll was never funded -- RANDOM must
    // never even be called.
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);

    QUSINO::refillRandomBank_output output = QUSINO.refillRandomBank(QUSINO_testUser1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_BONUS_AMOUNT);
    EXPECT_EQ(output.valuesAdded, 0u);

    QUSINO::getRandomBankStatus_output status = QUSINO.getRandomBankStatus();
    EXPECT_FALSE(status.poolInitialized);
    EXPECT_EQ(status.reserveFilled, 0u);
    EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, 0u);
}

TEST(ContractQUSINO, refillRandomBank_SucceedsAndPrimesCoinFlipPool)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    uint64 bonusBefore = QUSINO.getSCInfo().bonusAmount;

    QUSINO::refillRandomBank_output output = QUSINO.refillRandomBank(QUSINO_testUser1);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(output.valuesAdded, QUSINO_RNG_RESERVE_SIZE);

    // The very first refill also bootstraps game 0's (Coin Flip's) pool straight out of
    // the reserve it just filled, so reserveFilled should be RESERVE_SIZE - POOL_SIZE.
    QUSINO::getRandomBankStatus_output status = QUSINO.getRandomBankStatus();
    EXPECT_TRUE(status.poolInitialized);
    EXPECT_EQ(status.reserveFilled, QUSINO_RNG_RESERVE_SIZE - QUSINO_RNG_POOL_SIZE);
    EXPECT_EQ(status.lastRefillTick, system.tick);

    // The entropy fee actually spent is debited from the game bankroll.
    EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, bonusBefore - QUSINO_RNG_ENTROPY_FEE);
}

TEST(ContractQUSINO, refillRandomBank_TooSoonRejectedOnSameTick)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);

    QUSINO::refillRandomBank_output first = QUSINO.refillRandomBank(QUSINO_testUser1);
    EXPECT_EQ(first.returnCode, QUSINO_SUCCESS);

    // Rate limit applies regardless of whether entropy is available for the retry --
    // the check happens before RANDOM is ever called again.
    QUSINO::refillRandomBank_output second = QUSINO.refillRandomBank(QUSINO_testUser1);
    EXPECT_EQ(second.returnCode, QUSINO_RNG_REFILL_TOO_SOON);
}

TEST(ContractQUSINO, refillRandomBank_BlockedWhileReserveNotEmptyEvenPastTickGap)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);

    QUSINO::refillRandomBank_output first = QUSINO.refillRandomBank(QUSINO_testUser1);
    ASSERT_EQ(first.returnCode, QUSINO_SUCCESS);
    uint32 reserveAfterFirst = QUSINO.getRandomBankStatus().reserveFilled;
    uint32 tickAfterFirst = QUSINO.getRandomBankStatus().lastRefillTick;
    ASSERT_GT(reserveAfterFirst, 0u);

    // Advance well past QUSINO_RNG_MIN_REFILL_TICK_GAP and make fresh entropy available
    // again -- under the old (buggy) rate-limit-only guard this would succeed and
    // silently overwrite hundreds of still-unspent reserve entries, wasting the RANDOM
    // fee already paid for them. It must now be rejected purely because the reserve
    // isn't empty yet, tick gap notwithstanding.
    QUSINO.setTick(QUSINO.getTick() + QUSINO_RNG_MIN_REFILL_TICK_GAP + 10);
    QUSINO.seedRandomEntropy(0xF00D);

    QUSINO::refillRandomBank_output second = QUSINO.refillRandomBank(QUSINO_testUser1);
    EXPECT_EQ(second.returnCode, QUSINO_RNG_REFILL_TOO_SOON);
    EXPECT_EQ(second.valuesAdded, 0u);

    // No wasted purchase: reserve and last-refill-tick bookkeeping must be untouched.
    QUSINO::getRandomBankStatus_output status = QUSINO.getRandomBankStatus();
    EXPECT_EQ(status.reserveFilled, reserveAfterFirst);
    EXPECT_EQ(status.lastRefillTick, tickAfterFirst);
}

TEST(ContractQUSINO, refillRandomBank_RefundsAnyAttachedInvocationReward)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    QUSINO.seedRandomEntropy(0xA11CE);

    id caller = QUSINO_testUser1;
    sint64 attachedReward = 12345;
    increaseEnergy(caller, attachedReward);
    long long balanceBefore = getBalance(caller);

    QUSINO::refillRandomBank_output output = QUSINO.refillRandomBank(caller, attachedReward);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    // refillRandomBank takes no payment from the caller -- QUSINO funds RANDOM's fee
    // out of the game bankroll, so any attached reward must come straight back.
    EXPECT_EQ(getBalance(caller), balanceBefore);
}

TEST(ContractQUSINO, coinFlip_NotReadyBeforeBankPrimed)
{
    ContractTestingQUSINO QUSINO;

    id user = QUSINO_testUser1;
    increaseEnergy(user, 1);

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MIN_BET);
    EXPECT_EQ(output.returnCode, QUSINO_RNG_NOT_READY);
    EXPECT_EQ(output.payout, 0u);
}

TEST(ContractQUSINO, coinFlip_InvalidGuessRejected)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    increaseEnergy(user, 1);

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 2, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MIN_BET); // only 0/1 valid
    EXPECT_EQ(output.returnCode, QUSINO_INVALID_INPUT);
}

TEST(ContractQUSINO, coinFlip_InvalidAssetTypeRejected)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    increaseEnergy(user, 1);

    // Only QSC and STAR are valid Coin Flip bet assets -- raw Qu and QST are not.
    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QUBIC, QUSINO_COINFLIP_MIN_BET);
    EXPECT_EQ(output.returnCode, QUSINO_WRONG_ASSET_TYPE);
}

TEST(ContractQUSINO, coinFlip_BelowMinBetRejected)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    increaseEnergy(user, 1);

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MIN_BET - 1);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_FUNDS);
}

TEST(ContractQUSINO, coinFlip_InsufficientQscRejected)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    increaseEnergy(user, 1); // spectrum entry only, no QSC minted

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MIN_BET);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QSC);
}

TEST(ContractQUSINO, coinFlip_InsufficientStarRejected)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    increaseEnergy(user, 1); // spectrum entry only, no STAR minted

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_STAR, QUSINO_COINFLIP_MIN_BET);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_STAR);
}

TEST(ContractQUSINO, coinFlip_InsufficientBonusAmountRejectsQscBet)
{
    ContractTestingQUSINO QUSINO;

    // Fund the bankroll just enough for the entropy fee -- nowhere near enough to cover
    // a win payout on this bet (bet * 196 Qu).
    QUSINO.fundBonusAmount(QUSINO_RNG_ENTROPY_FEE);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);
    ASSERT_EQ(QUSINO.getSCInfo().bonusAmount, 0u);

    id user = QUSINO_testUser2;
    QUSINO.giveUserQSC(user, QUSINO_COINFLIP_MIN_BET);
    uint64 qscBefore = QUSINO.getUserAssetVolume(user).QSCAmount;

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MIN_BET);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_BONUS_AMOUNT);
    // Rejected bet must not touch the user's QSC at all.
    EXPECT_EQ(QUSINO.getUserAssetVolume(user).QSCAmount, qscBefore);
}

TEST(ContractQUSINO, coinFlip_QscSettlesConsistentlyAndUpdatesBank)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    uint32 reserveBefore = QUSINO.getRandomBankStatus().reserveFilled;
    uint64 epochRevenueBefore = QUSINO.getSCInfo().epochRevenue;
    uint64 bonusBefore = QUSINO.getSCInfo().bonusAmount;

    id user = QUSINO_testUser2;
    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet);
    // Captured *after* minting the bet's QSC via giveUserQSC, so this reflects supply
    // right before the wager itself -- not before the mint that funded it.
    uint64 qscSupplyBefore = QUSINO.getSCInfo().QSCCirclatingSupply;
    uint64 qscBefore = QUSINO.getUserAssetVolume(user).QSCAmount;
    long long qubicBalanceBefore = getBalance(user);

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, bet);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    EXPECT_LE(output.result, 1);

    // The consumed pool slot is immediately replenished from the reserve, so exactly
    // one reserve entry is spent per flip regardless of win/lose.
    EXPECT_EQ(QUSINO.getRandomBankStatus().reserveFilled, reserveBefore - 1);

    // coinFlip takes no invocationReward and never sends Qu directly (win or lose), so
    // the caller's real Qu balance is never touched by playing.
    EXPECT_EQ(getBalance(user), qubicBalanceBefore);

    uint64 qscRedemptionValueQu = bet * QUSINO_QSC_PRICE;
    uint64 winAmountQu = qscRedemptionValueQu * QUSINO_COINFLIP_PAYOUT_PERCENT / 100;
    if (output.won)
    {
        // A win credits new QSC back to the caller (redeemable for Qu later via
        // redemptionQSCToQubic) instead of paying Qu directly -- the wager itself
        // still left circulation up front, so the net QSC change is payout - bet.
        uint64 expectedQscPayout = winAmountQu / QUSINO_QSC_PRICE;
        EXPECT_EQ(output.payout, expectedQscPayout);
        EXPECT_EQ(QUSINO.getUserAssetVolume(user).QSCAmount, qscBefore - bet + expectedQscPayout);
        EXPECT_EQ(QUSINO.getSCInfo().QSCCirclatingSupply, qscSupplyBefore - bet + expectedQscPayout);
        // The bankroll is debited by exactly the Qu backing the credited QSC.
        EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, bonusBefore - expectedQscPayout * QUSINO_QSC_PRICE);
        EXPECT_EQ(QUSINO.getSCInfo().epochRevenue, epochRevenueBefore);
    }
    else
    {
        EXPECT_EQ(output.payout, 0u);
        EXPECT_EQ(QUSINO.getUserAssetVolume(user).QSCAmount, qscBefore - bet);
        EXPECT_EQ(QUSINO.getSCInfo().QSCCirclatingSupply, qscSupplyBefore - bet);
        // The redeemed QSC's Qu value tops up the game bankroll instead of paying out.
        EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, bonusBefore + qscRedemptionValueQu);
        EXPECT_EQ(QUSINO.getSCInfo().epochRevenue, epochRevenueBefore);
    }
}

TEST(ContractQUSINO, coinFlip_QscConsecutiveFlipsAdvancePoolNonce)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet * 5);

    // Enough reserve and QSC for several flips; just confirm every one of them settles
    // cleanly and the bank keeps accounting correctly call over call (no crash/
    // duplicate-spend of the same reserve slot).
    for (int i = 0; i < 5; i++)
    {
        QUSINO::coinFlip_output output = QUSINO.coinFlip(user, (uint8)(i % 2), QUSINO_ASSET_TYPE_QSC, bet);
        EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
    }

    QUSINO::getRandomBankStatus_output status = QUSINO.getRandomBankStatus();
    EXPECT_EQ(status.reserveFilled, (QUSINO_RNG_RESERVE_SIZE - QUSINO_RNG_POOL_SIZE) - 5);
}

TEST(ContractQUSINO, coinFlip_StarBetMintsOrBurnsDirectlyNoBonusAmount)
{
    ContractTestingQUSINO QUSINO;

    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    // refillRandomBank still needs the bankroll to buy entropy in the first place, so
    // fund it for only that one-time bootstrap, then confirm it's fully drained back to
    // 0 -- proving the STAR bet below doesn't need or touch it at all.
    QUSINO.fundBonusAmount(QUSINO_RNG_ENTROPY_FEE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);
    ASSERT_EQ(QUSINO.getSCInfo().bonusAmount, 0u);

    id user = QUSINO_testUser2;
    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet); // earnSTAR mints STAR too (bet*100 units)
    uint64 starBefore = QUSINO.getUserAssetVolume(user).STARAmount;
    uint64 starSupplyBefore = QUSINO.getSCInfo().STARCirclatingSupply;
    uint64 burntBefore = QUSINO.getSCInfo().burntSTAR;
    long long qubicBefore = getBalance(user);

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_STAR, bet);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);

    // STAR bets never touch Qu or the game bankroll.
    EXPECT_EQ(getBalance(user), qubicBefore);
    EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, 0u);

    if (output.won)
    {
        uint64 expectedPayout = bet * QUSINO_COINFLIP_PAYOUT_PERCENT / 100;
        EXPECT_EQ(output.payout, expectedPayout);
        // Net STAR change is the payout minted back on top of the wagered amount.
        EXPECT_EQ(QUSINO.getUserAssetVolume(user).STARAmount, starBefore - bet + expectedPayout);
        EXPECT_EQ(QUSINO.getSCInfo().STARCirclatingSupply, starSupplyBefore - bet + expectedPayout);
        EXPECT_EQ(QUSINO.getSCInfo().burntSTAR, burntBefore);
    }
    else
    {
        EXPECT_EQ(output.payout, 0u);
        EXPECT_EQ(QUSINO.getUserAssetVolume(user).STARAmount, starBefore - bet);
        EXPECT_EQ(QUSINO.getSCInfo().STARCirclatingSupply, starSupplyBefore - bet);
        EXPECT_EQ(QUSINO.getSCInfo().burntSTAR, burntBefore + bet);
    }
}

TEST(ContractQUSINO, depositBonus_CapsAtGameBankrollAndRoutesOverflowToEpochRevenue)
{
    ContractTestingQUSINO QUSINO;

    id owner = QUSINO_testUser1;
    uint64 firstDeposit = QUSINO_GAME_BANKROLL_CAP - 100;
    increaseEnergy(owner, (sint64)firstDeposit);
    ASSERT_EQ(QUSINO.depositBonus(owner, firstDeposit).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, firstDeposit);

    uint64 epochRevenueBefore = QUSINO.getSCInfo().epochRevenue;
    uint64 secondDeposit = 1000; // pushes bonusAmount 900 past the cap
    increaseEnergy(owner, (sint64)secondDeposit);
    ASSERT_EQ(QUSINO.depositBonus(owner, secondDeposit).returnCode, QUSINO_SUCCESS);

    EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, QUSINO_GAME_BANKROLL_CAP);
    EXPECT_EQ(QUSINO.getSCInfo().epochRevenue, epochRevenueBefore + 900);
}
