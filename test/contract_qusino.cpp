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

    // Anchors betaStartEpoch on its first call (see Qusino.h's BEGIN_EPOCH) -- the
    // constructor above only runs INITIALIZE, so tests exercising the beta gate or
    // getUserAssetVolume's beta fields must call this explicitly before/after
    // advancing system.epoch, mirroring how a real node calls BEGIN_EPOCH at each
    // epoch transition.
    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUSINO_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
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

    QUSINO::getDailyClaimStatus_output getDailyClaimStatus(const id& user)
    {
        QUSINO::getDailyClaimStatus_input input;
        input.user = user;
        QUSINO::getDailyClaimStatus_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 5, input, output);
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

    QUSINO::getApprovedGameList_output getApprovedGameList(uint32 offset)
    {
        QUSINO::getApprovedGameList_input input;
        input.offset = offset;
        QUSINO::getApprovedGameList_output output;
        callFunction(QUSINO_CONTRACT_INDEX, 7, input, output);
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

// Reported bug: a proposal's own proposer could vote "yes" on it like any
// other voter, padding their own proposal's yesVotes. See
// QUSINO_PROPOSER_CANNOT_VOTE's introduction in voteInGameProposal.
TEST(ContractQUSINO, voteInGameProposal_ProposerCannotVoteOnOwnGame)
{
    ContractTestingQUSINO QUSINO;

    id proposer = QUSINO_testUser1;
    Array<uint8, 64> URI = createURI("https://example.com/game1");

    sint64 requiredReward = QUSINO_GAME_SUBMIT_FEE;
    increaseEnergy(proposer, requiredReward);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, requiredReward);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);

    // Give the proposer plenty of STAR too, so a QUSINO_INSUFFICIENT_VOTE_FEE
    // rejection couldn't be mistaken for the proposer-block actually working.
    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(proposer, starReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(proposer, starAmount, starReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);

    increaseEnergy(proposer, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(proposer, URI, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_PROPOSER_CANNOT_VOTE);
}

// Reported bug: the same URI could be submitted as an unlimited number of
// separate, independently-votable proposals. See QUSINO_DUPLICATE_GAME_URI's
// introduction in submitGame.
TEST(ContractQUSINO, submitGame_RejectsDuplicateURIStillPending)
{
    ContractTestingQUSINO QUSINO;

    id proposer1 = QUSINO_testUser1;
    id proposer2 = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");

    increaseEnergy(proposer1, QUSINO_GAME_SUBMIT_FEE);
    QUSINO::submitGame_output firstSubmit = QUSINO.submitGame(proposer1, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(firstSubmit.returnCode, QUSINO_SUCCESS);

    increaseEnergy(proposer2, QUSINO_GAME_SUBMIT_FEE);
    // Snapshot right before the call, not before increaseEnergy -- the
    // rejection should refund the fee the transaction attached, returning
    // the balance to what it was at the moment of the call, not to some
    // earlier point before this test even funded the account.
    long long proposer2QuBefore = getBalance(proposer2);
    QUSINO::submitGame_output secondSubmit = QUSINO.submitGame(proposer2, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(secondSubmit.returnCode, QUSINO_DUPLICATE_GAME_URI);
    // The rejected submitter's fee should come back in full, same as every
    // other early-rejection branch in submitGame.
    EXPECT_EQ(getBalance(proposer2), proposer2QuBefore);

    // Only the first submission should actually be in gameList.
    QUSINO::getSCInfo_output scInfo = QUSINO.getSCInfo();
    EXPECT_EQ(scInfo.maxGameIndex, 2);
}

// Same as above, but the existing entry has already passed a vote and moved
// into approvedGameList (including its QUSINO_REVOTE_DURATION cooldown
// window) rather than still sitting in gameList.
TEST(ContractQUSINO, submitGame_RejectsDuplicateURIAlreadyApproved)
{
    ContractTestingQUSINO QUSINO;

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/game1");

    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    QUSINO::submitGame_output submitOutput = QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(submitOutput.returnCode, QUSINO_SUCCESS);

    uint64 starAmount = QUSINO_VOTE_FEE;
    sint64 starReward = starAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(voter, starReward);
    QUSINO::earnSTAR_output earnOutput = QUSINO.earnSTAR(voter, starAmount, starReward);
    EXPECT_EQ(earnOutput.returnCode, QUSINO_SUCCESS);

    increaseEnergy(voter, 1);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOutput = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 1);
    EXPECT_EQ(voteOutput.returnCode, QUSINO_SUCCESS);

    QUSINO.endEpoch();
    ++system.epoch;

    // Confirm it actually landed in approvedGameList before relying on that
    // for the real assertion below.
    QUSINO::getApprovedGameList_output approvedList = QUSINO.getApprovedGameList(0);
    EXPECT_EQ(approvedList.gameIndexes.get(0), gameIndex);

    id newProposer = QUSINO_testUser3;
    increaseEnergy(newProposer, QUSINO_GAME_SUBMIT_FEE);
    // Snapshot right before the call -- see the same note in
    // submitGame_RejectsDuplicateURIStillPending above.
    long long newProposerQuBefore = getBalance(newProposer);
    QUSINO::submitGame_output duplicateSubmit = QUSINO.submitGame(newProposer, URI, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(duplicateSubmit.returnCode, QUSINO_DUPLICATE_GAME_URI);
    EXPECT_EQ(getBalance(newProposer), newProposerQuBefore);
}

TEST(ContractQUSINO, getDailyClaimStatus_CanClaimBeforeFirstEverClaim)
{
    ContractTestingQUSINO QUSINO;

    id user = QUSINO_testUser1;
    QUSINO::getDailyClaimStatus_output status = QUSINO.getDailyClaimStatus(user);
    EXPECT_TRUE(status.canClaimNow);
    EXPECT_EQ(status.secondsUntilNextClaim, 0u);
}

TEST(ContractQUSINO, getDailyClaimStatus_CannotClaimRightAfterClaiming)
{
    ContractTestingQUSINO QUSINO;

    id user = QUSINO_testUser1;

    // Same setup as dailyClaimBonus_Success -- a claim needs the bonus pool
    // funded (bonusAmount starts at 0) and a fixed simulated time (qpi.year()
    // etc. are otherwise whatever this process's default/real clock reads,
    // which this test doesn't need to depend on).
    uint64 bonusFund = QUSINO_BONUS_CLAIM_AMOUNT * 10;
    increaseEnergy(user, bonusFund);
    QUSINO::depositBonus_output depOutput = QUSINO.depositBonus(user, bonusFund);
    EXPECT_EQ(depOutput.returnCode, QUSINO_SUCCESS);

    setMemory(utcTime, 0);
    utcTime.Year = 2024;
    utcTime.Month = 1;
    utcTime.Day = 1;
    utcTime.Hour = 0;
    utcTime.Minute = 0;
    utcTime.Second = 0;
    updateQpiTime();

    QUSINO::dailyClaimBonus_output claimOutput = QUSINO.dailyClaimBonus(user, 0);
    EXPECT_EQ(claimOutput.returnCode, QUSINO_SUCCESS);

    QUSINO::getDailyClaimStatus_output status = QUSINO.getDailyClaimStatus(user);
    EXPECT_FALSE(status.canClaimNow);
    // Called immediately after a successful claim -- the full 24h window
    // should still be ahead (allowing a couple of seconds of test-runtime
    // slack rather than asserting the exact boundary value).
    EXPECT_GT(status.secondsUntilNextClaim, (uint32)(QUSINO_DAILY_CLAIM_BONUS_DURATION - 5));
    EXPECT_LE(status.secondsUntilNextClaim, (uint32)QUSINO_DAILY_CLAIM_BONUS_DURATION);
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

    // Game should be in the failed list, gone from both the active list and
    // approvedGameList (a rejected proposal never gets archived as approved).
    QUSINO::getFailedGameList_output failedList = QUSINO.getFailedGameList(0);
    bool foundInFailedList = false;
    for (uint32 i = 0; i < 32; i++)
    {
        if (failedList.games.get(i).proposer == proposer)
        {
            foundInFailedList = true;
            break;
        }
    }
    EXPECT_TRUE(foundInFailedList);
}

// A passed proposal used to trigger an automatic, involuntary QSC-to-Qu conversion
// of the proposer's entire balance (split by a "developer fee") right here in
// END_EPOCH. That's been removed as redundant: this proves a proposer's QSC is left
// completely untouched by their proposal passing -- balance unchanged, circulating
// supply unchanged -- and that they can redeem it themselves afterward via
// redemptionQSCToQubic() at the standard, fee-free 1:1 rate (strictly better than
// the old automatic conversion ever was).
TEST(ContractQUSINO, END_EPOCH_ApprovedProposalDoesNotTouchProposerQSC)
{
    ContractTestingQUSINO QUSINO;

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

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    QUSINO::earnSTAR_output voterEarn = QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(voterEarn.returnCode, QUSINO_SUCCESS);
    QUSINO::getActiveGameList_output gameList = QUSINO.getActiveGameList(0);
    uint64 gameIndex = gameList.gameIndexes.get(0);
    QUSINO::voteInGameProposal_output voteOut = QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 0);
    EXPECT_EQ(voteOut.returnCode, QUSINO_SUCCESS);

    uint64 qscSupplyBefore = QUSINO.getSCInfo().QSCCirclatingSupply;
    long long proposerQuBefore = getBalance(proposer);

    QUSINO.endEpoch();
    ++system.epoch;

    // Proposal resolving passed didn't touch the proposer's QSC or transfer them
    // any Qu -- no more automatic conversion.
    EXPECT_EQ(QUSINO.getUserAssetVolume(proposer).QSCAmount, qscAmount);
    EXPECT_EQ(QUSINO.getSCInfo().QSCCirclatingSupply, qscSupplyBefore);
    EXPECT_EQ(getBalance(proposer), proposerQuBefore);

    // The proposer is no longer in gameList (their proposal is in approvedGameList
    // now), so redemptionQSCToQubic's "you can't redeem while you have a pending
    // proposal" guard no longer blocks them -- they can cash out the full amount
    // themselves, at the standard 1:1 rate, whenever they want.
    QUSINO::redemptionQSCToQubic_output redemption = QUSINO.redemptionQSCToQubic(proposer, qscAmount, 0);
    EXPECT_EQ(redemption.returnCode, QUSINO_SUCCESS);
    EXPECT_EQ((uint64)(getBalance(proposer) - proposerQuBefore), qscAmount * QUSINO_QSC_PRICE);
    EXPECT_EQ(QUSINO.getUserAssetVolume(proposer).QSCAmount, 0u);
}

// A passed proposal used to just vanish after the proposer's payout -- this proves
// it's archived into approvedGameList instead, and that it's no longer sitting in
// the active list once resolved.
TEST(ContractQUSINO, END_EPOCH_PassedGameArchivedToApprovedList)
{
    ContractTestingQUSINO QUSINO;

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/approved-game");

    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);

    QUSINO::getActiveGameList_output activeBefore = QUSINO.getActiveGameList(0);
    uint64 gameIndex = activeBefore.gameIndexes.get(0);

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 0).returnCode, QUSINO_SUCCESS);

    QUSINO.endEpoch();
    ++system.epoch;

    // Gone from the active list...
    QUSINO::getActiveGameList_output activeAfter = QUSINO.getActiveGameList(0);
    bool stillActive = false;
    for (uint32 i = 0; i < 32; i++)
    {
        if (activeAfter.gameIndexes.get(i) == gameIndex && activeAfter.games.get(i).proposer == proposer)
        {
            stillActive = true;
        }
    }
    EXPECT_FALSE(stillActive);

    // ...and archived in approvedGameList under the same gameIndex, not just discarded.
    QUSINO::getApprovedGameList_output approved = QUSINO.getApprovedGameList(0);
    bool foundApproved = false;
    for (uint32 i = 0; i < 32; i++)
    {
        if (approved.gameIndexes.get(i) == gameIndex && approved.games.get(i).proposer == proposer)
        {
            foundApproved = true;
            EXPECT_EQ(approved.games.get(i).yesVotes, 1u);
            EXPECT_EQ(approved.games.get(i).noVotes, 0u);
        }
    }
    EXPECT_TRUE(foundApproved);
}

// The actual bug fix: an approved proposal should not just sit in approvedGameList
// forever -- QUSINO_REVOTE_DURATION epochs after it was (re)confirmed, it should come
// back to gameList for a fresh vote, with the old tally reset (not carried over) so
// it has to be genuinely reconfirmed, not just coast on a vote count from over a
// year and a half ago.
TEST(ContractQUSINO, END_EPOCH_ApprovedGameResurrectsAfterRevoteDuration)
{
    ContractTestingQUSINO QUSINO;

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/revote-game");

    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);

    QUSINO::getActiveGameList_output activeBefore = QUSINO.getActiveGameList(0);
    uint64 gameIndex = activeBefore.gameIndexes.get(0);
    uint32 approvalEpoch = system.epoch;

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(voter, URI, gameIndex, 1, 0).returnCode, QUSINO_SUCCESS);

    QUSINO.endEpoch();
    ++system.epoch; // now approvalEpoch + 1; proposal sits in approvedGameList

    // Fast-forward straight to the epoch right before the revote window opens --
    // no need to actually call endEpoch() for every epoch in between, since nothing
    // else in this test depends on those epochs' side effects (dividends etc.), only
    // on qpi.epoch()'s value at the next endEpoch() call.
    system.epoch = approvalEpoch + QUSINO_REVOTE_DURATION - 1;

    QUSINO.endEpoch();
    ++system.epoch; // now approvalEpoch + QUSINO_REVOTE_DURATION

    // No longer archived -- it's back in play.
    QUSINO::getApprovedGameList_output approvedAfter = QUSINO.getApprovedGameList(0);
    bool stillApproved = false;
    for (uint32 i = 0; i < 32; i++)
    {
        if (approvedAfter.gameIndexes.get(i) == gameIndex)
        {
            stillApproved = true;
        }
    }
    EXPECT_FALSE(stillApproved);

    // Back in gameList, same gameIndex, votes reset to zero -- not carrying over
    // yesVotes=1/noVotes=0 from a year and a half ago.
    QUSINO::getActiveGameList_output resurrected = QUSINO.getActiveGameList(0);
    bool found = false;
    for (uint32 i = 0; i < 32; i++)
    {
        if (resurrected.gameIndexes.get(i) == gameIndex)
        {
            found = true;
            EXPECT_EQ(resurrected.games.get(i).proposer, proposer);
            EXPECT_EQ(resurrected.games.get(i).yesVotes, 0u);
            EXPECT_EQ(resurrected.games.get(i).noVotes, 0u);
            EXPECT_EQ(resurrected.games.get(i).proposedEpoch, approvalEpoch + QUSINO_REVOTE_DURATION);
        }
    }
    ASSERT_TRUE(found);

    // And it's genuinely votable again -- proves voteInGameProposal's simplified
    // proposedEpoch == qpi.epoch() check still recognizes a resurrected entry.
    id secondVoter = QUSINO_testUser3;
    increaseEnergy(secondVoter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(QUSINO.earnSTAR(secondVoter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(secondVoter, URI, gameIndex, 1, 0).returnCode, QUSINO_SUCCESS);

    QUSINO.endEpoch();
    ++system.epoch;

    // Reconfirmed -- back in approvedGameList, re-anchored to this new epoch, ready
    // to repeat the same cycle again in another QUSINO_REVOTE_DURATION epochs.
    QUSINO::getApprovedGameList_output reapproved = QUSINO.getApprovedGameList(0);
    bool foundReapproved = false;
    for (uint32 i = 0; i < 32; i++)
    {
        if (reapproved.gameIndexes.get(i) == gameIndex)
        {
            foundReapproved = true;
            EXPECT_EQ(reapproved.games.get(i).proposedEpoch, approvalEpoch + QUSINO_REVOTE_DURATION);
        }
    }
    EXPECT_TRUE(foundReapproved);
}

// Proves the yesVotes/noVotes underflow exploit is closed: a garbage yesNo value
// (anything but 1 or 2) is rejected outright, before touching STAR balance or
// voteList -- not silently accepted as a no-op "vote" that still burns the fee and
// leaves the voter recorded as "already voted" (which previously let a follow-up
// real vote decrement a counter that was never incremented, underflowing it).
TEST(ContractQUSINO, voteInGameProposal_InvalidYesNoRejectedWithoutCorruptingCounts)
{
    ContractTestingQUSINO QUSINO;

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/bad-yesno");

    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);
    uint64 gameIndex = 1; // first submission in a fresh instance always gets index 1

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100).returnCode, QUSINO_SUCCESS);

    QUSINO::getUserAssetVolume_output starBefore = QUSINO.getUserAssetVolume(voter);

    QUSINO::voteInGameProposal_output badVoteZero = QUSINO.voteInGameProposal(voter, URI, gameIndex, 0, 0);
    EXPECT_EQ(badVoteZero.returnCode, QUSINO_INVALID_INPUT);

    QUSINO::voteInGameProposal_output badVoteHigh = QUSINO.voteInGameProposal(voter, URI, gameIndex, 99, 0);
    EXPECT_EQ(badVoteHigh.returnCode, QUSINO_INVALID_INPUT);

    // Neither rejected call should have burned the fee or touched the tally.
    QUSINO::getUserAssetVolume_output starAfterRejections = QUSINO.getUserAssetVolume(voter);
    EXPECT_EQ(starAfterRejections.STARAmount, starBefore.STARAmount);

    QUSINO::getActiveGameList_output stillZero = QUSINO.getActiveGameList(0);
    EXPECT_EQ(stillZero.games.get(0).yesVotes, 0u);
    EXPECT_EQ(stillZero.games.get(0).noVotes, 0u);

    // The exploit sequence: garbage vote, then a real one. Before the fix, this
    // underflowed yesVotes to ~4.29 billion (the garbage call's rejected-now voteList
    // entry made the real call think it was "switching" an existing yes vote it never
    // actually cast). Since the garbage calls above were rejected before writing
    // anything, this real vote is treated as a genuine first vote.
    QUSINO::voteInGameProposal_output realVote = QUSINO.voteInGameProposal(voter, URI, gameIndex, 2, 0);
    EXPECT_EQ(realVote.returnCode, QUSINO_SUCCESS);

    QUSINO::getActiveGameList_output afterReal = QUSINO.getActiveGameList(0);
    EXPECT_EQ(afterReal.games.get(0).yesVotes, 0u);
    EXPECT_EQ(afterReal.games.get(0).noVotes, 1u);
}

// A proposer with two proposals both resolving as passed in the same epoch used to
// have their QSC balance zeroed by whichever one resolved first, with the second
// paid out (and recorded) from an already-drained balance. With the automatic
// conversion removed entirely, this just proves both proposals resolve
// independently without touching the proposer's QSC at all -- no ordering-dependent
// side effect between them.
TEST(ContractQUSINO, END_EPOCH_MultiplePassedProposalsFromSameProposerDoNotTouchQSC)
{
    ContractTestingQUSINO QUSINO;

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> uriA = createURI("https://example.com/multi-a");
    Array<uint8, 64> uriB = createURI("https://example.com/multi-b");

    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE * 2);
    EXPECT_EQ(QUSINO.submitGame(proposer, uriA, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.submitGame(proposer, uriB, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);
    uint64 gameIndexA = 1;
    uint64 gameIndexB = 2;

    uint64 qscAmount = 500;
    sint64 starReward = qscAmount * QUSINO_STAR_PRICE * 100;
    increaseEnergy(proposer, starReward);
    EXPECT_EQ(QUSINO.earnSTAR(proposer, qscAmount, starReward).returnCode, QUSINO_SUCCESS);

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100 * 2);
    EXPECT_EQ(QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE * 2, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100 * 2).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(voter, uriA, gameIndexA, 1, 0).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(voter, uriB, gameIndexB, 1, 0).returnCode, QUSINO_SUCCESS);

    QUSINO.endEpoch();
    ++system.epoch;

    EXPECT_EQ(QUSINO.getUserAssetVolume(proposer).QSCAmount, qscAmount);

    QUSINO::getApprovedGameList_output approved = QUSINO.getApprovedGameList(0);
    int foundCount = 0;
    for (uint32 i = 0; i < 32; i++)
    {
        if (approved.games.get(i).proposer == proposer)
        {
            foundCount++;
        }
    }
    EXPECT_EQ(foundCount, 2);
}

// Proves the QST-holder dividend was fully removed (Sept 2026 client feedback:
// "remove the revenue for the QST holders"). The old AssetPossessionIterator loop
// in END_EPOCH used to pay QUSINO_QST_HOLDERS_DIVIDENDS_PERCENT of epochRevenue
// directly to every QST possessor via qpi.transfer -- that whole pass is gone now,
// so a 100%-QST-holding issuer's Qu balance must be completely unaffected by
// endEpoch(). QUSINO_QST_HOLDERS_DIVIDENDS_PERCENT itself was removed from
// Qusino.h too, so there's no old constant left to even reference here.
TEST(ContractQUSINO, END_EPOCH_NoLongerPaysQSTDividend)
{
    ContractTestingQUSINO QUSINO;

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    uint64 totalShares = QUSINO_SUPPLY_OF_QST;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, totalShares), totalShares);
    // qstIssuer now holds 100% of QUSINO_SUPPLY_OF_QST. If any QST dividend logic
    // still existed, the whole pool would land on them alone -- so a zero delta here
    // is a strong signal the removal was complete, not just rate-limited to ~0.

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/qst-dividend-removed-test");
    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);
    uint64 gameIndex = 1;

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(voter, URI, gameIndex, 2, 0).returnCode, QUSINO_SUCCESS);

    uint64 epochRevenueBeforeSplit = QUSINO.getSCInfo().epochRevenue;
    ASSERT_GT(epochRevenueBeforeSplit, 0ULL);

    long long qstIssuerQuBefore = getBalance(qstIssuer);

    QUSINO.endEpoch();
    ++system.epoch;

    long long qstIssuerQuAfter = getBalance(qstIssuer);
    EXPECT_EQ(qstIssuerQuAfter, qstIssuerQuBefore);
}

// Proves the 30 points that used to go to QST holders were folded into the
// shareholder split: QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT is now 50 (was 20), and
// epochRevenue should be drawn down by exactly lpShare + ccfShare + treasuryShare +
// shareholders676Part with no separate QST term subtracted anymore. Exercised via
// the same lp/ccf/treasury/shareholders arithmetic END_EPOCH itself uses (mirroring
// Qusino.h's formulas) rather than inspecting individual shareholder payouts, since
// qpi.distributeDividends requires a full 676-shareholder cap-table fixture the rest
// of this file doesn't set up.
TEST(ContractQUSINO, END_EPOCH_ShareholdersReceiveFiftyPercentNotTwenty)
{
    ASSERT_EQ(QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT, 50u);

    ContractTestingQUSINO QUSINO;

    id proposer = QUSINO_testUser1;
    id voter = QUSINO_testUser2;
    Array<uint8, 64> URI = createURI("https://example.com/shareholder-split-test");
    increaseEnergy(proposer, QUSINO_GAME_SUBMIT_FEE);
    EXPECT_EQ(QUSINO.submitGame(proposer, URI, QUSINO_GAME_SUBMIT_FEE).returnCode, QUSINO_SUCCESS);
    uint64 gameIndex = 1;

    increaseEnergy(voter, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100);
    EXPECT_EQ(QUSINO.earnSTAR(voter, QUSINO_VOTE_FEE, QUSINO_VOTE_FEE * QUSINO_STAR_PRICE * 100).returnCode, QUSINO_SUCCESS);
    EXPECT_EQ(QUSINO.voteInGameProposal(voter, URI, gameIndex, 2, 0).returnCode, QUSINO_SUCCESS);

    uint64 epochRevenueBeforeSplit = QUSINO.getSCInfo().epochRevenue;
    ASSERT_GT(epochRevenueBeforeSplit, 0ULL);

    uint64 lpShare = epochRevenueBeforeSplit * (uint64)QUSINO_LP_DIVIDENDS_PERCENT / 100ULL;
    uint64 ccfShare = epochRevenueBeforeSplit * (uint64)QUSINO_CCF_DIVIDENDS_PERCENT / 100ULL;
    uint64 treasuryShare = epochRevenueBeforeSplit * (uint64)QUSINO_TREASURY_DIVIDENDS_PERCENT / 100ULL;
    uint64 shareholdersPart = (epochRevenueBeforeSplit * (uint64)QUSINO_SHAREHOLDERS_DIVIDENDS_PERCENT / 67600ULL) * 676ULL;
    uint64 expectedRemaining = epochRevenueBeforeSplit - (lpShare + ccfShare + treasuryShare + shareholdersPart);

    QUSINO.endEpoch();
    ++system.epoch;

    EXPECT_EQ(QUSINO.getSCInfo().epochRevenue, expectedRemaining);
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

TEST(ContractQUSINO, coinFlip_AboveMaxBetRejected)
{
    ContractTestingQUSINO QUSINO;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    increaseEnergy(user, 1);

    // Rejected purely on amount, before ever checking the caller's own QSC
    // balance (this user has none) -- same ordering as the min-bet gate.
    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MAX_BET + 1);
    EXPECT_EQ(output.returnCode, QUSINO_EXCEEDS_MAX_BET);

    // Exactly at the ceiling is still fine (rejected here for a different,
    // expected reason: this user genuinely has no QSC to bet with).
    QUSINO::coinFlip_output atMax = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, QUSINO_COINFLIP_MAX_BET);
    EXPECT_EQ(atMax.returnCode, QUSINO_INSUFFICIENT_QSC);

    // STAR is covered by the same asset-agnostic gate, not just QSC.
    QUSINO::coinFlip_output starOutput = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_STAR, QUSINO_COINFLIP_MAX_BET + 1);
    EXPECT_EQ(starOutput.returnCode, QUSINO_EXCEEDS_MAX_BET);
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

    // Fund the bankroll just enough for the entropy fee -- leaving it at exactly zero,
    // nowhere near enough to cover even the net liability a win on this bet would incur
    // (let alone the payout's gross backing).
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
        // The bankroll moves by exactly the payout's Qu backing net of the stake's own
        // freed backing (the stake's burn above already freed qscRedemptionValueQu of
        // backing, so only the shortfall needs to come out of bonusAmount).
        EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, bonusBefore + qscRedemptionValueQu - expectedQscPayout * QUSINO_QSC_PRICE);
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

TEST(ContractQUSINO, coinFlip_NetGateAllowsBetGrossGateWouldReject)
{
    ContractTestingQUSINO QUSINO;

    id user = QUSINO_testUser2;
    uint64 bet = 1000ULL;
    uint64 qscRedemptionValueQu = bet * QUSINO_QSC_PRICE;
    uint64 winAmountQu = qscRedemptionValueQu * QUSINO_COINFLIP_PAYOUT_PERCENT / 100; // gross backing: 196000
    uint64 qscPayout = winAmountQu / QUSINO_QSC_PRICE;                               // 1960
    uint64 netDebitQu = qscPayout * QUSINO_QSC_PRICE - qscRedemptionValueQu;         // net liability: 96000

    // Fund the bankroll to a level strictly between the net liability this bet would
    // actually incur (netDebitQu) and the payout's full gross backing (winAmountQu) -- a
    // bet the pool can genuinely afford, but only if gated on the net figure. The old
    // gross-based gate would have wrongly rejected this exact bet.
    uint64 targetBonus = (netDebitQu + winAmountQu) / 2;
    ASSERT_GT(targetBonus, netDebitQu);
    ASSERT_LT(targetBonus, winAmountQu);

    QUSINO.fundBonusAmount(QUSINO_RNG_ENTROPY_FEE + targetBonus);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);
    ASSERT_EQ(QUSINO.getSCInfo().bonusAmount, targetBonus);

    QUSINO.giveUserQSC(user, bet);
    uint64 bonusBefore = QUSINO.getSCInfo().bonusAmount;
    uint64 qscBefore = QUSINO.getUserAssetVolume(user).QSCAmount;

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, bet);

    // The point of this test: bonusAmount sits below the gross payout backing but above
    // the net liability, and the bet must still be accepted either way the coin lands.
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);

    if (output.won)
    {
        EXPECT_EQ(output.payout, qscPayout);
        EXPECT_EQ(QUSINO.getUserAssetVolume(user).QSCAmount, qscBefore - bet + qscPayout);
        EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, bonusBefore - netDebitQu);
    }
    else
    {
        EXPECT_EQ(output.payout, 0u);
        EXPECT_EQ(QUSINO.getUserAssetVolume(user).QSCAmount, qscBefore - bet);
        EXPECT_EQ(QUSINO.getSCInfo().bonusAmount, bonusBefore + qscRedemptionValueQu);
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

// coinFlip beta gate (Sept 2026 client feedback): for QUSINO_BETA_DURATION_EPOCHS
// epochs after betaStartEpoch is anchored, only callers holding at least
// QUSINO_BETA_MIN_QST_HOLDING QST may play; everyone else is rejected with
// QUSINO_INSUFFICIENT_QST_FOR_BETA before any RNG-pool or balance checks run (the
// gate sits first in coinFlip), so a blocked call must leave the bank/balances
// completely untouched.
TEST(ContractQUSINO, coinFlip_BetaGateBlocksCallerWithoutSufficientQST)
{
    ContractTestingQUSINO QUSINO;
    QUSINO.beginEpoch(); // anchors betaStartEpoch to the current (construction) epoch

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);
    uint32 reserveBefore = QUSINO.getRandomBankStatus().reserveFilled;

    id user = QUSINO_testUser2;
    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet); // plenty of QSC, but zero QST
    uint64 qscBefore = QUSINO.getUserAssetVolume(user).QSCAmount;

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, bet);
    EXPECT_EQ(output.returnCode, QUSINO_INSUFFICIENT_QST_FOR_BETA);
    EXPECT_EQ(output.result, 0u);
    EXPECT_EQ(output.won, 0u);
    EXPECT_EQ(output.payout, 0u);

    // Gated calls never reach the RNG pool or touch the caller's QSC.
    EXPECT_EQ(QUSINO.getRandomBankStatus().reserveFilled, reserveBefore);
    EXPECT_EQ(QUSINO.getUserAssetVolume(user).QSCAmount, qscBefore);
}

TEST(ContractQUSINO, coinFlip_BetaGateAllowsCallerWithSufficientQST)
{
    ContractTestingQUSINO QUSINO;
    QUSINO.beginEpoch();

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, QUSINO_SUPPLY_OF_QST), QUSINO_SUPPLY_OF_QST);

    id user = QUSINO_testUser2;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, user, qstAssetName, qstIssuer, QUSINO_BETA_MIN_QST_HOLDING), (sint64)QUSINO_BETA_MIN_QST_HOLDING);

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet);

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, bet);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
}

TEST(ContractQUSINO, coinFlip_BetaGateInactiveBeforeAnchor)
{
    ContractTestingQUSINO QUSINO;
    // Deliberately no beginEpoch() call -- betaStartEpoch is still the 0 sentinel,
    // exactly like a freshly-constructed pre-upgrade contract before its first
    // post-deploy epoch boundary. The gate must be skipped entirely in this state.

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet); // zero QST

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, bet);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
}

TEST(ContractQUSINO, coinFlip_BetaGateInactiveAfterDurationElapses)
{
    ContractTestingQUSINO QUSINO;
    QUSINO.beginEpoch();
    uint32 anchorEpoch = system.epoch;

    // Jump straight to the epoch the beta window ends at -- no QST required from here on.
    system.epoch = anchorEpoch + QUSINO_BETA_DURATION_EPOCHS;

    QUSINO.fundBonusAmount(1000000000ULL);
    increaseEnergy(QUSINO_testUser1, 1);
    QUSINO.seedRandomEntropy(0xA11CE);
    ASSERT_EQ(QUSINO.refillRandomBank(QUSINO_testUser1).returnCode, QUSINO_SUCCESS);

    id user = QUSINO_testUser2;
    uint64 bet = QUSINO_COINFLIP_MIN_BET;
    QUSINO.giveUserQSC(user, bet); // zero QST

    QUSINO::coinFlip_output output = QUSINO.coinFlip(user, 0, QUSINO_ASSET_TYPE_QSC, bet);
    EXPECT_EQ(output.returnCode, QUSINO_SUCCESS);
}

// getUserAssetVolume's new QST balance + beta-status fields, mirroring coinFlip's
// gate exactly (see its comment in Qusino.h) so the frontend can show an accurate
// notification without duplicating the gate logic itself.
TEST(ContractQUSINO, getUserAssetVolume_ReportsQSTBalanceAndBetaStatus)
{
    ContractTestingQUSINO QUSINO;

    id qstIssuer = QUSINO_QSTIssuer;
    uint64 qstAssetName = 5526353;
    increaseEnergy(qstIssuer, QUSINO_ISSUE_ASSET_FEE);
    EXPECT_EQ(QUSINO.issueAsset(qstIssuer, qstAssetName, QUSINO_SUPPLY_OF_QST), QUSINO_SUPPLY_OF_QST);

    id belowThresholdUser = QUSINO_testUser1;
    id eligibleUser = QUSINO_testUser2;
    increaseEnergy(qstIssuer, QUSINO_TRANSFER_ASSET_FEE * 2);
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, belowThresholdUser, qstAssetName, qstIssuer, QUSINO_BETA_MIN_QST_HOLDING - 1), (sint64)(QUSINO_BETA_MIN_QST_HOLDING - 1));
    EXPECT_EQ(QUSINO.transferAsset(qstIssuer, eligibleUser, qstAssetName, qstIssuer, QUSINO_BETA_MIN_QST_HOLDING), (sint64)QUSINO_BETA_MIN_QST_HOLDING);

    // Before the beta window is anchored: reported inactive, everyone eligible.
    QUSINO::getUserAssetVolume_output preAnchor = QUSINO.getUserAssetVolume(belowThresholdUser);
    EXPECT_EQ(preAnchor.QSTAmount, QUSINO_BETA_MIN_QST_HOLDING - 1);
    EXPECT_EQ(preAnchor.betaActive, 0);
    EXPECT_EQ(preAnchor.betaEligible, 1);
    EXPECT_EQ(preAnchor.betaEpochsRemaining, 0u);

    QUSINO.beginEpoch();
    uint32 anchorEpoch = system.epoch;

    QUSINO::getUserAssetVolume_output belowDuringBeta = QUSINO.getUserAssetVolume(belowThresholdUser);
    EXPECT_EQ(belowDuringBeta.betaActive, 1);
    EXPECT_EQ(belowDuringBeta.betaEligible, 0);
    EXPECT_EQ(belowDuringBeta.betaEpochsRemaining, QUSINO_BETA_DURATION_EPOCHS);

    QUSINO::getUserAssetVolume_output eligibleDuringBeta = QUSINO.getUserAssetVolume(eligibleUser);
    EXPECT_EQ(eligibleDuringBeta.QSTAmount, QUSINO_BETA_MIN_QST_HOLDING);
    EXPECT_EQ(eligibleDuringBeta.betaActive, 1);
    EXPECT_EQ(eligibleDuringBeta.betaEligible, 1);

    // Past the beta window: inactive again, everyone eligible regardless of QST.
    system.epoch = anchorEpoch + QUSINO_BETA_DURATION_EPOCHS;
    QUSINO::getUserAssetVolume_output afterBeta = QUSINO.getUserAssetVolume(belowThresholdUser);
    EXPECT_EQ(afterBeta.betaActive, 0);
    EXPECT_EQ(afterBeta.betaEligible, 1);
    EXPECT_EQ(afterBeta.betaEpochsRemaining, 0u);
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
