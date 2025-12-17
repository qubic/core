#define NO_UEFI

#include <map>
#include <random>

#include "contract_testing.h"

static std::mt19937_64 rand64;

static unsigned long long random(unsigned long long minValue, unsigned long long maxValue)
{
    if(minValue > maxValue) 
    {
        return 0;
    }
    return minValue + rand64() % (maxValue - minValue);
}

static id getUser(unsigned long long i)
{
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

static std::vector<id> getRandomUsers(unsigned int totalUsers, unsigned int maxNum)
{
    std::map<id, bool> userMap;
    unsigned long long userCount = random(0, maxNum);
    std::vector<id> users;
    users.reserve(userCount);
    for (unsigned int i = 0; i < userCount; ++i)
    {
        unsigned long long userIdx = random(0, totalUsers - 1);
        id user = getUser(userIdx);
        if (userMap.contains(user))
        {
            continue;
        }
        userMap[user] = true;
        users.push_back(user);
    }
    return users;
}

class QRaffleChecker : public QRAFFLE
{
public:
    void registerChecker(const id& user, uint32 expectedRegisters, bool isRegistered)
    {
        if (isRegistered)
        {
            EXPECT_EQ(registers.contains(user), 1);
        }
        else
        {
            EXPECT_EQ(registers.contains(user), 0);
        }
        EXPECT_EQ(numberOfRegisters, expectedRegisters);
    }

    void unregisterChecker(const id& user, uint32 expectedRegisters)
    {
        EXPECT_EQ(registers.contains(user), 0);
        EXPECT_EQ(numberOfRegisters, expectedRegisters);
    }

    void entryAmountChecker(const id& user, uint64 expectedAmount, uint32 expectedSubmitted)
    {
        uint64 amount = 0;
        if (quRaffleEntryAmount.contains(user))
        {
            quRaffleEntryAmount.get(user, amount);
            EXPECT_EQ(amount, expectedAmount);
        }
        EXPECT_EQ(numberOfEntryAmountSubmitted, expectedSubmitted);
    }

    void proposalChecker(uint32 index, const Asset& expectedToken, uint64 expectedEntryAmount)
    {
        EXPECT_EQ(proposals.get(index).token.assetName, expectedToken.assetName);
        EXPECT_EQ(proposals.get(index).token.issuer, expectedToken.issuer);
        EXPECT_EQ(proposals.get(index).entryAmount, expectedEntryAmount);
        EXPECT_EQ(numberOfProposals, index + 1);
    }

    void voteChecker(uint32 proposalIndex, uint32 expectedYes, uint32 expectedNo)
    {
        EXPECT_EQ(proposals.get(proposalIndex).nYes, expectedYes);
        EXPECT_EQ(proposals.get(proposalIndex).nNo, expectedNo);
    }

    void quRaffleMemberChecker(const id& user, uint32 expectedMembers)
    {
        bool found = false;
        for (uint32 i = 0; i < numberOfQuRaffleMembers; i++)
        {
            if (quRaffleMembers.get(i) == user)
            {
                found = true;
                break;
            }
        }
        EXPECT_EQ(found, 1);
        EXPECT_EQ(numberOfQuRaffleMembers, expectedMembers);
    }

    void tokenRaffleMemberChecker(uint32 raffleIndex, const id& user, uint32 expectedMembers)
    {
        tokenRaffleMembers.get(raffleIndex, tmpTokenRaffleMembers);
        bool found = false;
        for (uint32 i = 0; i < numberOfTokenRaffleMembers.get(raffleIndex); i++)
        {
            if (tmpTokenRaffleMembers.get(i) == user)
            {
                found = true;
                break;
            }
        }
        EXPECT_EQ(found, 1);
        EXPECT_EQ(numberOfTokenRaffleMembers.get(raffleIndex), expectedMembers);
    }

    void analyticsChecker(uint64 expectedBurn, uint64 expectedCharity, uint64 expectedShareholder, 
                         uint64 expectedRegister, uint64 expectedFee, uint64 expectedWinner, 
                         uint64 expectedLargestWinner, uint32 expectedRegisters, uint32 expectedProposals,
                         uint32 expectedQuMembers, uint32 expectedActiveTokenRaffle, 
                         uint32 expectedEndedTokenRaffle, uint32 expectedEntrySubmitted)
    {
        EXPECT_EQ(totalBurnAmount, expectedBurn);
        EXPECT_EQ(totalCharityAmount, expectedCharity);
        EXPECT_EQ(totalShareholderAmount, expectedShareholder);
        EXPECT_EQ(totalRegisterAmount, expectedRegister);
        EXPECT_EQ(totalFeeAmount, expectedFee);
        EXPECT_EQ(totalWinnerAmount, expectedWinner);
        EXPECT_EQ(largestWinnerAmount, expectedLargestWinner);
        EXPECT_EQ(numberOfRegisters, expectedRegisters);
        EXPECT_EQ(numberOfProposals, expectedProposals);
        EXPECT_EQ(numberOfQuRaffleMembers, expectedQuMembers);
        EXPECT_EQ(numberOfActiveTokenRaffle, expectedActiveTokenRaffle);
        EXPECT_EQ(numberOfEndedTokenRaffle, expectedEndedTokenRaffle);
        EXPECT_EQ(numberOfEntryAmountSubmitted, expectedEntrySubmitted);
    }

    void activeTokenRaffleChecker(uint32 index, const Asset& expectedToken, uint64 expectedEntryAmount)
    {
        EXPECT_EQ(activeTokenRaffle.get(index).token.assetName, expectedToken.assetName);
        EXPECT_EQ(activeTokenRaffle.get(index).token.issuer, expectedToken.issuer);
        EXPECT_EQ(activeTokenRaffle.get(index).entryAmount, expectedEntryAmount);
    }

    void endedTokenRaffleChecker(uint32 index, const id& expectedWinner, const Asset& expectedToken, 
                                uint64 expectedEntryAmount, uint32 expectedMembers, uint32 expectedWinnerIndex, uint32 expectedEpoch)
    {
        EXPECT_EQ(tokenRaffle.get(index).epochWinner, expectedWinner);
        EXPECT_EQ(tokenRaffle.get(index).token.assetName, expectedToken.assetName);
        EXPECT_EQ(tokenRaffle.get(index).token.issuer, expectedToken.issuer);
        EXPECT_EQ(tokenRaffle.get(index).entryAmount, expectedEntryAmount);
        EXPECT_EQ(tokenRaffle.get(index).numberOfMembers, expectedMembers);
        EXPECT_EQ(tokenRaffle.get(index).winnerIndex, expectedWinnerIndex);
        EXPECT_EQ(tokenRaffle.get(index).epoch, expectedEpoch);
    }

    void quRaffleWinnerChecker(uint16 epoch, const id& expectedWinner, uint64 expectedReceived, 
                              uint64 expectedEntryAmount, uint32 expectedMembers, uint32 expectedWinnerIndex)
    {
        EXPECT_EQ(QuRaffles.get(epoch).epochWinner, expectedWinner);
        EXPECT_EQ(QuRaffles.get(epoch).receivedAmount, expectedReceived);
        EXPECT_EQ(QuRaffles.get(epoch).entryAmount, expectedEntryAmount);
        EXPECT_EQ(QuRaffles.get(epoch).numberOfMembers, expectedMembers);
        EXPECT_EQ(QuRaffles.get(epoch).winnerIndex, expectedWinnerIndex);
    }

    uint64 getQuRaffleEntryAmount()
    {
        return qREAmount;
    }

    uint32 getNumberOfActiveTokenRaffle()
    {
        return numberOfActiveTokenRaffle;
    }

    uint32 getNumberOfEndedTokenRaffle()
    {
        return numberOfEndedTokenRaffle;
    }

    uint64 getEpochQXMRRevenue()
    {
        return epochQXMRRevenue;
    }

    uint32 getNumberOfRegisters()
    {
        return numberOfRegisters;
    }

    id getQXMRIssuer()
    {
        return QXMRIssuer;
    }
};

class ContractTestingQraffle : protected ContractTesting
{
public:
    ContractTestingQraffle()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QRAFFLE);
        callSystemProcedure(QRAFFLE_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QRaffleChecker* getState()
    {
        return (QRaffleChecker*)contractStates[QRAFFLE_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QRAFFLE_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    QRAFFLE::registerInSystem_output registerInSystem(const id& user, uint64 amount, bit useQXMR)
    {
        QRAFFLE::registerInSystem_input input;
        QRAFFLE::registerInSystem_output output;
        
        input.useQXMR = useQXMR;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 1, input, output, user, amount);
        return output;
    }

    QRAFFLE::logoutInSystem_output logoutInSystem(const id& user)
    {
        QRAFFLE::logoutInSystem_input input;
        QRAFFLE::logoutInSystem_output output;
        
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 2, input, output, user, 0);
        return output;
    }

    QRAFFLE::submitEntryAmount_output submitEntryAmount(const id& user, uint64 amount)
    {
        QRAFFLE::submitEntryAmount_input input;
        QRAFFLE::submitEntryAmount_output output;
        
        input.amount = amount;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 3, input, output, user, 0);
        return output;
    }

    QRAFFLE::submitProposal_output submitProposal(const id& user, const Asset& token, uint64 entryAmount)
    {
        QRAFFLE::submitProposal_input input;
        QRAFFLE::submitProposal_output output;
        
        input.tokenIssuer = token.issuer;
        input.tokenName = token.assetName;
        input.entryAmount = entryAmount;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 4, input, output, user, 0);
        return output;
    }

    QRAFFLE::voteInProposal_output voteInProposal(const id& user, uint32 proposalIndex, bit yes)
    {
        QRAFFLE::voteInProposal_input input;
        QRAFFLE::voteInProposal_output output;
        
        input.indexOfProposal = proposalIndex;
        input.yes = yes;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 5, input, output, user, 0);
        return output;
    }

    QRAFFLE::depositInQuRaffle_output depositInQuRaffle(const id& user, uint64 amount)
    {
        QRAFFLE::depositInQuRaffle_input input;
        QRAFFLE::depositInQuRaffle_output output;
        
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 6, input, output, user, amount);
        return output;
    }

    QRAFFLE::depositInTokenRaffle_output depositInTokenRaffle(const id& user, uint32 raffleIndex, uint64 amount)
    {
        QRAFFLE::depositInTokenRaffle_input input;
        QRAFFLE::depositInTokenRaffle_output output;
        
        input.indexOfTokenRaffle = raffleIndex;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 7, input, output, user, amount);
        return output;
    }

    QRAFFLE::getRegisters_output getRegisters(uint32 offset, uint32 limit)
    {
        QRAFFLE::getRegisters_input input;
        QRAFFLE::getRegisters_output output;
        
        input.offset = offset;
        input.limit = limit;
        callFunction(QRAFFLE_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QRAFFLE::getAnalytics_output getAnalytics()
    {
        QRAFFLE::getAnalytics_input input;
        QRAFFLE::getAnalytics_output output;
        
        callFunction(QRAFFLE_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QRAFFLE::getActiveProposal_output getActiveProposal(uint32 proposalIndex)
    {
        QRAFFLE::getActiveProposal_input input;
        QRAFFLE::getActiveProposal_output output;
        
        input.indexOfProposal = proposalIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QRAFFLE::getEndedTokenRaffle_output getEndedTokenRaffle(uint32 raffleIndex)
    {
        QRAFFLE::getEndedTokenRaffle_input input;
        QRAFFLE::getEndedTokenRaffle_output output;
        
        input.indexOfRaffle = raffleIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QRAFFLE::getEndedQuRaffle_output getEndedQuRaffle(uint16 epoch)
    {
        QRAFFLE::getEndedQuRaffle_input input;
        QRAFFLE::getEndedQuRaffle_output output;
        
        input.epoch = epoch;
        callFunction(QRAFFLE_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QRAFFLE::getActiveTokenRaffle_output getActiveTokenRaffle(uint32 raffleIndex)
    {
        QRAFFLE::getActiveTokenRaffle_input input;
        QRAFFLE::getActiveTokenRaffle_output output;
        
        input.indexOfTokenRaffle = raffleIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QRAFFLE::getEpochRaffleIndexes_output getEpochRaffleIndexes(uint16 epoch)
    {
        QRAFFLE::getEpochRaffleIndexes_input input;
        QRAFFLE::getEpochRaffleIndexes_output output;
        
        input.epoch = epoch;
        callFunction(QRAFFLE_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QRAFFLE::getQuRaffleEntryAmountPerUser_output getQuRaffleEntryAmountPerUser(const id& user)
    {
        QRAFFLE::getQuRaffleEntryAmountPerUser_input input;
        QRAFFLE::getQuRaffleEntryAmountPerUser_output output;
        
        input.user = user;
        callFunction(QRAFFLE_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    QRAFFLE::getQuRaffleEntryAverageAmount_output getQuRaffleEntryAverageAmount()
    {
        QRAFFLE::getQuRaffleEntryAverageAmount_input input;
        QRAFFLE::getQuRaffleEntryAverageAmount_output output;
        
        callFunction(QRAFFLE_CONTRACT_INDEX, 9, input, output);
        return output;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, 1000000000ULL);
        return output.issuedNumberOfShares;
    }

    sint64 transferShareOwnershipAndPossession(const id& issuer, uint64 assetName, const id& currentOwnerAndPossesor, sint64 numberOfShares, const id& newOwnerAndPossesor)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;

        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, currentOwnerAndPossesor, 100);
        return output.transferredNumberOfShares;
    }

    sint64 TransferShareManagementRights(const id& issuer, uint64 assetName, uint32 newManagingContractIndex, sint64 numberOfShares, const id& currentOwner)
    {
        QX::TransferShareManagementRights_input input;
        QX::TransferShareManagementRights_output output;

        input.asset.assetName = assetName;
        input.asset.issuer = issuer;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, currentOwner, 0);

        return output.transferredNumberOfShares;
    }

    sint64 TransferShareManagementRightsQraffle(const id& issuer, uint64 assetName, uint32 newManagingContractIndex, sint64 numberOfShares, const id& currentOwner)
    {
        QRAFFLE::TransferShareManagementRights_input input;
        QRAFFLE::TransferShareManagementRights_output output;

        input.tokenName = assetName;
        input.tokenIssuer = issuer;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 8, input, output, currentOwner, QRAFFLE_TRANSFER_SHARE_FEE);
        return output.transferredNumberOfShares;
    }
};

TEST(ContractQraffle, RegisterInSystem)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Test successful registration
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        auto result = qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        qraffle.getState()->registerChecker(user, ++registerCount, true);
    }

    // // Test insufficient funds
    id poorUser = getUser(9999);
    increaseEnergy(poorUser, QRAFFLE_REGISTER_AMOUNT - 1);
    auto result = qraffle.registerInSystem(poorUser, QRAFFLE_REGISTER_AMOUNT - 1, 0);
    EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_FUND);
    qraffle.getState()->registerChecker(poorUser, registerCount, false);

    // Test already registered
    increaseEnergy(users[0], QRAFFLE_REGISTER_AMOUNT);
    result = qraffle.registerInSystem(users[0], QRAFFLE_REGISTER_AMOUNT, 0);
    EXPECT_EQ(result.returnCode, QRAFFLE_ALREADY_REGISTERED);
    qraffle.getState()->registerChecker(users[0], registerCount, true);
}

TEST(ContractQraffle, LogoutInSystem)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Test successful logout
    for (const auto& user : users)
    {
        auto result = qraffle.logoutInSystem(user);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(getBalance(user), QRAFFLE_REGISTER_AMOUNT - QRAFFLE_LOGOUT_FEE);
        qraffle.getState()->unregisterChecker(user, --registerCount);
    }

    // Test unregistered user logout
    qraffle.getState()->unregisterChecker(users[0], registerCount);
    auto result = qraffle.logoutInSystem(users[0]);
    EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);
}

TEST(ContractQraffle, SubmitEntryAmount)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;
    uint32 entrySubmittedCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Test successful entry amount submission
    for (const auto& user : users)
    {
        uint64 amount = random(1000000, 1000000000);
        auto result = qraffle.submitEntryAmount(user, amount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        qraffle.getState()->entryAmountChecker(user, amount, ++entrySubmittedCount);
    }

    // Test unregistered user
    id unregisteredUser = getUser(9999);
    increaseEnergy(unregisteredUser, QRAFFLE_REGISTER_AMOUNT);
    auto result = qraffle.submitEntryAmount(unregisteredUser, 1000000);
    EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);

    // Test update entry amount
    uint64 newAmount = random(1000000, 1000000000);
    result = qraffle.submitEntryAmount(users[0], newAmount);
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->entryAmountChecker(users[0], newAmount, entrySubmittedCount);
}

TEST(ContractQraffle, SubmitProposal)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;
    uint32 proposalCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Issue some test assets
    id issuer = getUser(2000);
    increaseEnergy(issuer, 1000000000ULL);
    uint64 assetName1 = assetNameFromString("TEST1");
    uint64 assetName2 = assetNameFromString("TEST2");
    qraffle.issueAsset(issuer, assetName1, 1000000, 0, 0);
    qraffle.issueAsset(issuer, assetName2, 2000000, 0, 0);

    Asset token1, token2;
    token1.assetName = assetName1;
    token1.issuer = issuer;
    token2.assetName = assetName2;
    token2.issuer = issuer;

    // Test successful proposal submission
    for (const auto& user : users)
    {
        uint64 entryAmount = random(1000000, 1000000000);
        Asset token = (random(0, 2) == 0) ? token1 : token2;
        
        if (proposalCount == QRAFFLE_MAX_PROPOSAL_EPOCH - 1)
        {
           break;
        }
        increaseEnergy(user, 1000);
        auto result = qraffle.submitProposal(user, token, entryAmount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        qraffle.getState()->proposalChecker(proposalCount, token, entryAmount);
        proposalCount++;
    }

    // Test unregistered user
    id unregisteredUser = getUser(1999);
    increaseEnergy(unregisteredUser, QRAFFLE_REGISTER_AMOUNT);
    auto result = qraffle.submitProposal(unregisteredUser, token1, 1000000);
    EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);
}   

TEST(ContractQraffle, VoteInProposal)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;
    uint32 proposalCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Create a proposal
    id issuer = getUser(2000);
    increaseEnergy(issuer, 1000000000ULL);
    uint64 assetName = assetNameFromString("VOTETS");
    qraffle.issueAsset(issuer, assetName, 1000000, 0, 0);

    Asset token;
    token.assetName = assetName;
    token.issuer = issuer;

    qraffle.submitProposal(users[0], token, 1000000);
    proposalCount++;

    uint32 yesVotes = 0, noVotes = 0;

    // Test voting
    for (const auto& user : users)
    {
        bit vote = (bit)random(0, 2);
        auto result = qraffle.voteInProposal(user, 0, vote);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        
        if (vote)
            yesVotes++;
        else
            noVotes++;
        
        qraffle.getState()->voteChecker(0, yesVotes, noVotes);
    }

    // Test duplicate vote (should change vote)
    bit newVote = (bit)random(0, 2);
    auto result = qraffle.voteInProposal(users[0], 0, newVote);
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
    
    if (newVote)
    {
        yesVotes++;
        noVotes--;
    }   
    else
    {
        noVotes++;
        yesVotes--;
    }
    
    qraffle.getState()->voteChecker(0, yesVotes, noVotes);

    // Test unregistered user
    id unregisteredUser = getUser(9999);
    increaseEnergy(unregisteredUser, 1000000000ULL);
    result = qraffle.voteInProposal(unregisteredUser, 0, 1);
    EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);

    // Test invalid proposal index
    result = qraffle.voteInProposal(users[0], 9999, 1);
    EXPECT_EQ(result.returnCode, QRAFFLE_INVALID_PROPOSAL);
}

TEST(ContractQraffle, depositInQuRaffle)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;
    uint32 memberCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Test successful deposit
    for (const auto& user : users)
    {
        increaseEnergy(user, qraffle.getState()->getQuRaffleEntryAmount());
        auto result = qraffle.depositInQuRaffle(user, qraffle.getState()->getQuRaffleEntryAmount());
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        qraffle.getState()->quRaffleMemberChecker(user, ++memberCount);
    }

    // Test insufficient funds
    id poorUser = getUser(9999);
    increaseEnergy(poorUser, qraffle.getState()->getQuRaffleEntryAmount() - 1);
    auto result = qraffle.depositInQuRaffle(poorUser, qraffle.getState()->getQuRaffleEntryAmount() - 1);
    EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_FUND);

    // Test already registered
    increaseEnergy(users[0], qraffle.getState()->getQuRaffleEntryAmount());
    result = qraffle.depositInQuRaffle(users[0], qraffle.getState()->getQuRaffleEntryAmount());
    EXPECT_EQ(result.returnCode, QRAFFLE_ALREADY_REGISTERED);
}

TEST(ContractQraffle, DepositInTokenRaffle)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Create a proposal and vote for it
    id issuer = getUser(2000);    
    increaseEnergy(issuer, 2000000000ULL);
    uint64 assetName = assetNameFromString("TOKENRF");
    qraffle.issueAsset(issuer, assetName, 1000000000000, 0, 0);

    Asset token;
    token.assetName = assetName;
    token.issuer = issuer;

    qraffle.submitProposal(users[0], token, 1000000);
    
    // Vote yes for the proposal
    for (const auto& user : users)
    {
        qraffle.voteInProposal(user, 0, 1);
    }

    // End epoch to activate token raffle
    qraffle.endEpoch();

    // Test active token raffle
    auto activeRaffle = qraffle.getActiveTokenRaffle(0);
    EXPECT_EQ(activeRaffle.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(activeRaffle.tokenName, assetName);
    EXPECT_EQ(activeRaffle.tokenIssuer, issuer);
    EXPECT_EQ(activeRaffle.entryAmount, 1000000);

    // Test successful token raffle deposit
    uint32 memberCount = 0;
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_TRANSFER_SHARE_FEE);
        EXPECT_EQ(qraffle.transferShareOwnershipAndPossession(issuer, assetName, issuer, 1000000, user), 1000000);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, user, user, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1000000);
        
        EXPECT_EQ(qraffle.TransferShareManagementRights(issuer, assetName, QRAFFLE_CONTRACT_INDEX, 1000000, user), 1000000);
        EXPECT_EQ(numberOfPossessedShares(assetName, issuer, user, user, QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000);

        auto result = qraffle.depositInTokenRaffle(user, 0, QRAFFLE_TRANSFER_SHARE_FEE);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        memberCount++;
        qraffle.getState()->tokenRaffleMemberChecker(0, user, memberCount);
    }

    // Test insufficient funds
    id poorUser = getUser(9999);
    increaseEnergy(poorUser, QRAFFLE_TRANSFER_SHARE_FEE - 1);
    auto result = qraffle.depositInTokenRaffle(poorUser, 0, QRAFFLE_TRANSFER_SHARE_FEE - 1);
    EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_FUND);

    // Test insufficient Token
    id poorUser2 = getUser(8888);
    increaseEnergy(poorUser2, QRAFFLE_TRANSFER_SHARE_FEE);
    qraffle.transferShareOwnershipAndPossession(issuer, assetName, issuer, 999999, poorUser2);
    result = qraffle.depositInTokenRaffle(poorUser2, 0, QRAFFLE_TRANSFER_SHARE_FEE);
    EXPECT_EQ(result.returnCode, QRAFFLE_FAILED_TO_DEPOSIT);

    // Test invalid token raffle index
    increaseEnergy(users[0], QRAFFLE_TRANSFER_SHARE_FEE);
    result = qraffle.depositInTokenRaffle(users[0], 999, QRAFFLE_TRANSFER_SHARE_FEE);
    EXPECT_EQ(result.returnCode, QRAFFLE_INVALID_TOKEN_RAFFLE);
}

TEST(ContractQraffle, TransferShareManagementRights)
{
    ContractTestingQraffle qraffle;
    
    id issuer = getUser(1000);    
    increaseEnergy(issuer, 2000000000ULL);
    uint64 assetName = assetNameFromString("TOKENRF");
    qraffle.issueAsset(issuer, assetName, 1000000000000, 0, 0);

    id user1 = getUser(1001);
    increaseEnergy(user1, 1000000000ULL);
    qraffle.transferShareOwnershipAndPossession(issuer, assetName, issuer, 1000000, user1);
    EXPECT_EQ(qraffle.TransferShareManagementRights(issuer, assetName, QRAFFLE_CONTRACT_INDEX, 1000000, user1), 1000000);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, user1, user1, QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000);

    increaseEnergy(user1, 1000000000ULL);
    qraffle.TransferShareManagementRightsQraffle(issuer, assetName, QX_CONTRACT_INDEX, 1000000, user1);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, user1, user1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1000000);
}

TEST(ContractQraffle, GetFunctions)
{
    ContractTestingQraffle qraffle;
    system.epoch = 0;

    // Setup: Create test users and register them
    auto users = getRandomUsers(1000, 1000); // Use smaller set for more predictable testing
    uint32 registerCount = 5;
    uint32 proposalCount = 0;
    uint32 entrySubmittedCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        auto result = qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        registerCount++;
    }

    // Submit entry amounts for some users
    for (size_t i = 0; i < users.size() / 2; ++i)
    {
        uint64 amount = random(1000000, 1000000000);
        auto result = qraffle.submitEntryAmount(users[i], amount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        entrySubmittedCount++;
    }

    // Create some proposals
    id issuer = getUser(2000);
    increaseEnergy(issuer, 1000000000ULL);
    uint64 assetName1 = assetNameFromString("TEST1");
    uint64 assetName2 = assetNameFromString("TEST2");
    qraffle.issueAsset(issuer, assetName1, 1000000000, 0, 0);
    qraffle.issueAsset(issuer, assetName2, 2000000000, 0, 0);

    Asset token1, token2;
    token1.assetName = assetName1;
    token1.issuer = issuer;
    token2.assetName = assetName2;
    token2.issuer = issuer;

    // Submit proposals
    for (size_t i = 0; i < std::min(users.size(), (size_t)5); ++i)
    {
        uint64 entryAmount = random(1000000, 1000000000);
        Asset token = (i % 2 == 0) ? token1 : token2;
        
        increaseEnergy(users[i], 1000);
        auto result = qraffle.submitProposal(users[i], token, entryAmount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        proposalCount++;
    }

    // Vote on proposals
    for (const auto& user : users)
    {
        for (uint32 i = 0; i < proposalCount; ++i)
        {
            bit vote = (bit)(i % 2);
            auto result = qraffle.voteInProposal(user, i, vote);
            EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        }
    }

    // Deposit in QuRaffle
    uint32 memberCount = 0;
    for (size_t i = 0; i < users.size() / 3; ++i)
    {
        increaseEnergy(users[i], qraffle.getState()->getQuRaffleEntryAmount());
        auto result = qraffle.depositInQuRaffle(users[i], qraffle.getState()->getQuRaffleEntryAmount());
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        memberCount++;
    }

    // Test 1: getActiveProposal function
    {
        // Test with valid proposal indices
        for (uint32 i = 0; i < proposalCount; ++i)
        {
            auto proposal = qraffle.getActiveProposal(i);
            EXPECT_EQ(proposal.returnCode, QRAFFLE_SUCCESS);
            EXPECT_EQ(proposal.tokenName, (i % 2 == 0) ? assetName1 : assetName2);
            EXPECT_EQ(proposal.tokenIssuer, issuer);
            EXPECT_GT(proposal.entryAmount, 0);
            EXPECT_GE(proposal.nYes, 0u);
            EXPECT_GE(proposal.nNo, 0u);
        }
        
        // Test with invalid proposal index (beyond available proposals)
        auto invalidProposal = qraffle.getActiveProposal(proposalCount + 10);
        EXPECT_EQ(invalidProposal.returnCode, QRAFFLE_INVALID_PROPOSAL);
        
        // Test with very large proposal index
        auto largeIndexProposal = qraffle.getActiveProposal(UINT32_MAX);
        EXPECT_EQ(largeIndexProposal.returnCode, QRAFFLE_INVALID_PROPOSAL);
    }


    // End epoch to create some ended raffles
    qraffle.endEpoch();

    // ===== DETAILED TEST CASES FOR EACH GETTER FUNCTION =====

    // Test 2: getRegisters function
    {
        // Test with valid offset and limit
        auto registers = qraffle.getRegisters(0, 10);
        EXPECT_EQ(registers.returnCode, QRAFFLE_SUCCESS);
        
        // Test with offset beyond available registers
        auto registers2 = qraffle.getRegisters(registerCount + 10, 5);
        EXPECT_EQ(registers2.returnCode, QRAFFLE_INVALID_OFFSET_OR_LIMIT);
        
        // Test with limit exceeding maximum (1024)
        auto registers3 = qraffle.getRegisters(0, 1025);
        EXPECT_EQ(registers3.returnCode, QRAFFLE_INVALID_OFFSET_OR_LIMIT);
        
        // Test with offset + limit exceeding total registers
        auto registers4 = qraffle.getRegisters(registerCount - 5, 10);
        EXPECT_EQ(registers4.returnCode, QRAFFLE_INVALID_OFFSET_OR_LIMIT);
        
        // Test with zero limit
        auto registers5 = qraffle.getRegisters(0, 0);
        EXPECT_EQ(registers5.returnCode, QRAFFLE_SUCCESS);
    }

    // Test 3: getAnalytics function
    {
        auto analytics = qraffle.getAnalytics();
        EXPECT_EQ(analytics.returnCode, QRAFFLE_SUCCESS);
        
        // Validate all analytics fields
        EXPECT_GE(analytics.totalBurnAmount, 0);
        EXPECT_GE(analytics.totalCharityAmount, 0);
        EXPECT_GE(analytics.totalShareholderAmount, 0);
        EXPECT_GE(analytics.totalRegisterAmount, 0);
        EXPECT_GE(analytics.totalFeeAmount, 0);
        EXPECT_GE(analytics.totalWinnerAmount, 0);
        EXPECT_GE(analytics.largestWinnerAmount, 0);
        EXPECT_EQ(analytics.numberOfRegisters, registerCount);
        EXPECT_EQ(analytics.numberOfProposals, 0);
        EXPECT_EQ(analytics.numberOfQuRaffleMembers, 0);
        EXPECT_GE(analytics.numberOfActiveTokenRaffle, 0u);
        EXPECT_GE(analytics.numberOfEndedTokenRaffle, 0u);
        EXPECT_EQ(analytics.numberOfEntryAmountSubmitted, 0u);
        
        // Cross-validate with internal state
        qraffle.getState()->analyticsChecker(analytics.totalBurnAmount, analytics.totalCharityAmount, 
                                            analytics.totalShareholderAmount, analytics.totalRegisterAmount,
                                            analytics.totalFeeAmount, analytics.totalWinnerAmount, 
                                            analytics.largestWinnerAmount, analytics.numberOfRegisters,
                                            analytics.numberOfProposals, analytics.numberOfQuRaffleMembers,
                                            analytics.numberOfActiveTokenRaffle, analytics.numberOfEndedTokenRaffle,
                                            analytics.numberOfEntryAmountSubmitted);

        // Direct-validate with calculated values
        // Calculate expected values based on the test setup
        uint64 expectedTotalBurnAmount = 0;
        uint64 expectedTotalCharityAmount = 0;
        uint64 expectedTotalShareholderAmount = 0;
        uint64 expectedTotalRegisterAmount = 0;
        uint64 expectedTotalFeeAmount = 0;
        uint64 expectedTotalWinnerAmount = 0;
        uint64 expectedLargestWinnerAmount = 0;
        
        // Calculate expected values from QuRaffle (if any members participated)
        if (memberCount > 0) {
            uint64 qREAmount = 10000000; // initial entry amount
            uint64 totalQuRaffleAmount = qREAmount * memberCount;
            
            expectedTotalBurnAmount += (totalQuRaffleAmount * QRAFFLE_BURN_FEE) / 100;
            expectedTotalCharityAmount += (totalQuRaffleAmount * QRAFFLE_CHARITY_FEE) / 100;
            expectedTotalShareholderAmount += ((totalQuRaffleAmount * QRAFFLE_SHRAEHOLDER_FEE) / 100) / 676 * 676;
            expectedTotalRegisterAmount += ((totalQuRaffleAmount * QRAFFLE_REGISTER_FEE) / 100) / registerCount * registerCount;
            expectedTotalFeeAmount += (totalQuRaffleAmount * QRAFFLE_FEE) / 100;
            
            // Winner amount calculation (after all fees)
            uint64 winnerAmount = totalQuRaffleAmount - expectedTotalBurnAmount - expectedTotalCharityAmount 
                                - expectedTotalShareholderAmount - expectedTotalRegisterAmount - expectedTotalFeeAmount;
            expectedTotalWinnerAmount += winnerAmount;
            expectedLargestWinnerAmount = winnerAmount; // First winner sets the largest
        }
        
        // Validate calculated values
        EXPECT_EQ(analytics.totalBurnAmount, expectedTotalBurnAmount);
        EXPECT_EQ(analytics.totalCharityAmount, expectedTotalCharityAmount);
        EXPECT_EQ(analytics.totalShareholderAmount, expectedTotalShareholderAmount);
        EXPECT_EQ(analytics.totalRegisterAmount, expectedTotalRegisterAmount);
        EXPECT_EQ(analytics.totalFeeAmount, expectedTotalFeeAmount);
        EXPECT_EQ(analytics.totalWinnerAmount, expectedTotalWinnerAmount);
        EXPECT_EQ(analytics.largestWinnerAmount, expectedLargestWinnerAmount);
        
        // Validate counters
        EXPECT_EQ(analytics.numberOfRegisters, registerCount);
        EXPECT_EQ(analytics.numberOfProposals, 0); // Proposals are cleared after epoch end
        EXPECT_EQ(analytics.numberOfQuRaffleMembers, 0); // Members are cleared after epoch end
        EXPECT_EQ(analytics.numberOfActiveTokenRaffle, qraffle.getState()->getNumberOfActiveTokenRaffle());
        EXPECT_EQ(analytics.numberOfEndedTokenRaffle, qraffle.getState()->getNumberOfEndedTokenRaffle());
        EXPECT_EQ(analytics.numberOfEntryAmountSubmitted, 0); // Entry amounts are cleared after epoch end
        
    }
    
    // Test 4: getEndedTokenRaffle function
    {
        // Test with valid raffle indices (if any ended raffles exist)
        for (uint32 i = 0; i < qraffle.getState()->getNumberOfEndedTokenRaffle(); ++i)
        {
            auto endedRaffle = qraffle.getEndedTokenRaffle(i);
            EXPECT_EQ(endedRaffle.returnCode, QRAFFLE_SUCCESS);
            EXPECT_NE(endedRaffle.epochWinner, id(0, 0, 0, 0)); // Winner should be set
            EXPECT_GT(endedRaffle.entryAmount, 0);
            EXPECT_GT(endedRaffle.numberOfMembers, 0u);
            EXPECT_GE(endedRaffle.epoch, 0u);
        }
        
        // Test with invalid raffle index (beyond available ended raffles)
        auto invalidEndedRaffle = qraffle.getEndedTokenRaffle(qraffle.getState()->getNumberOfEndedTokenRaffle() + 10);
        EXPECT_EQ(invalidEndedRaffle.returnCode, QRAFFLE_INVALID_TOKEN_RAFFLE);
        
        // Test with very large raffle index
        auto largeIndexEndedRaffle = qraffle.getEndedTokenRaffle(UINT32_MAX);
        EXPECT_EQ(largeIndexEndedRaffle.returnCode, QRAFFLE_INVALID_TOKEN_RAFFLE);
    }

    // Test 5: getEpochRaffleIndexes function
    {
        // Test with current epoch (0)
        auto raffleIndexes = qraffle.getEpochRaffleIndexes(0);
        EXPECT_EQ(raffleIndexes.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(raffleIndexes.StartIndex, 0);
        EXPECT_EQ(raffleIndexes.EndIndex, qraffle.getState()->getNumberOfActiveTokenRaffle());
        
        // Test with future epoch
        auto futureRaffleIndexes = qraffle.getEpochRaffleIndexes(1);
        EXPECT_EQ(futureRaffleIndexes.returnCode, QRAFFLE_INVALID_EPOCH);
        
        // Test with past epoch (if any exist)
        if (qraffle.getState()->getNumberOfEndedTokenRaffle() > 0)
        {
            auto pastRaffleIndexes = qraffle.getEpochRaffleIndexes(0); // Should work for epoch 0
            EXPECT_EQ(pastRaffleIndexes.returnCode, QRAFFLE_SUCCESS);
        }
    }

    // Test 6: getEndedQuRaffle function
    {
        // Test with current epoch (0)
        auto endedQuRaffle = qraffle.getEndedQuRaffle(0);
        EXPECT_EQ(endedQuRaffle.returnCode, QRAFFLE_SUCCESS);
        EXPECT_NE(endedQuRaffle.epochWinner, id(0, 0, 0, 0)); // Winner should be set
        EXPECT_GT(endedQuRaffle.receivedAmount, 0);
        EXPECT_EQ(endedQuRaffle.entryAmount, 10000000);
        EXPECT_EQ(endedQuRaffle.numberOfMembers, memberCount);
        
        // Test with future epoch
        auto futureQuRaffle = qraffle.getEndedQuRaffle(1);
        EXPECT_EQ(futureQuRaffle.returnCode, QRAFFLE_SUCCESS);
        
        // Test with very large epoch number
        auto largeEpochQuRaffle = qraffle.getEndedQuRaffle(UINT16_MAX);
        EXPECT_EQ(largeEpochQuRaffle.returnCode, QRAFFLE_SUCCESS);
    }

    // Test 7: getActiveTokenRaffle function
    {
        // Test with valid raffle indices (if any active raffles exist)
        for (uint32 i = 0; i < qraffle.getState()->getNumberOfActiveTokenRaffle(); ++i)
        {
            auto activeRaffle = qraffle.getActiveTokenRaffle(i);
            EXPECT_EQ(activeRaffle.returnCode, QRAFFLE_SUCCESS);
            EXPECT_GT(activeRaffle.tokenName, 0);
            EXPECT_NE(activeRaffle.tokenIssuer, id(0, 0, 0, 0));
            EXPECT_GT(activeRaffle.entryAmount, 0);
        }
        
        // Test with invalid raffle index (beyond available active raffles)
        auto invalidActiveRaffle = qraffle.getActiveTokenRaffle(qraffle.getState()->getNumberOfActiveTokenRaffle() + 10);
        EXPECT_EQ(invalidActiveRaffle.returnCode, QRAFFLE_INVALID_TOKEN_RAFFLE);
        
        // Test with very large raffle index
        auto largeIndexActiveRaffle = qraffle.getActiveTokenRaffle(UINT32_MAX);
        EXPECT_EQ(largeIndexActiveRaffle.returnCode, QRAFFLE_INVALID_TOKEN_RAFFLE);
    }
}

TEST(ContractQraffle, EndEpoch)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        registerCount++;
    }

    // Submit entry amounts
    for (const auto& user : users)
    {
        uint64 amount = random(1000000, 1000000000);
        qraffle.submitEntryAmount(user, amount);
    }

    // Create proposals and vote for them
    id issuer = getUser(2000);
    increaseEnergy(issuer, 3000000000ULL);
    uint64 assetName1 = assetNameFromString("TOKEN1");
    uint64 assetName2 = assetNameFromString("TOKEN2");
    qraffle.issueAsset(issuer, assetName1, 1000000000, 0, 0);
    qraffle.issueAsset(issuer, assetName2, 2000000000, 0, 0);

    Asset token1, token2;
    token1.assetName = assetName1;
    token1.issuer = issuer;
    token2.assetName = assetName2;
    token2.issuer = issuer;

    qraffle.submitProposal(users[0], token1, 1000000);
    qraffle.submitProposal(users[1], token2, 2000000);

    // Vote yes for both proposals
    for (const auto& user : users)
    {
        qraffle.voteInProposal(user, 0, 1);
        qraffle.voteInProposal(user, 1, 1);
    }

    // Deposit in QuRaffle
    for (const auto& user : users)
    {
        increaseEnergy(user, qraffle.getState()->getQuRaffleEntryAmount());
        qraffle.depositInQuRaffle(user, qraffle.getState()->getQuRaffleEntryAmount());
    }

    // Deposit in token raffles
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_TRANSFER_SHARE_FEE + 1000000);
        EXPECT_EQ(qraffle.transferShareOwnershipAndPossession(issuer, assetName1, issuer, 1000000, user), 1000000);
        EXPECT_EQ(qraffle.transferShareOwnershipAndPossession(issuer, assetName2, issuer, 2000000, user), 2000000);
    }

    // End epoch
    qraffle.endEpoch();

    qraffle.getState()->activeTokenRaffleChecker(0, token1, 1000000);
    qraffle.getState()->activeTokenRaffleChecker(1, token2, 2000000);

    // Deposit in token raffles
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_TRANSFER_SHARE_FEE);
        EXPECT_EQ(qraffle.TransferShareManagementRights(issuer, assetName1, QRAFFLE_CONTRACT_INDEX, 1000000, user), 1000000);
        EXPECT_EQ(qraffle.TransferShareManagementRights(issuer, assetName2, QRAFFLE_CONTRACT_INDEX, 2000000, user), 2000000);

        qraffle.depositInTokenRaffle(user, 0, QRAFFLE_TRANSFER_SHARE_FEE);
        qraffle.depositInTokenRaffle(user, 1, QRAFFLE_TRANSFER_SHARE_FEE);
    }

    // Check that QuRaffle was processed
    auto quRaffle = qraffle.getEndedQuRaffle(0);
    EXPECT_EQ(quRaffle.returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->quRaffleWinnerChecker(0, quRaffle.epochWinner, quRaffle.receivedAmount, 
                                            quRaffle.entryAmount, quRaffle.numberOfMembers, quRaffle.winnerIndex);

    qraffle.endEpoch();
    // Check that token raffles were processed
    auto tokenRaffle1 = qraffle.getEndedTokenRaffle(0);
    EXPECT_EQ(tokenRaffle1.returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->endedTokenRaffleChecker(0, tokenRaffle1.epochWinner, token1, 
                                              tokenRaffle1.entryAmount, tokenRaffle1.numberOfMembers, 
                                              tokenRaffle1.winnerIndex, tokenRaffle1.epoch);

    auto tokenRaffle2 = qraffle.getEndedTokenRaffle(1);
    EXPECT_EQ(tokenRaffle2.returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->endedTokenRaffleChecker(1, tokenRaffle2.epochWinner, token2, 
                                              tokenRaffle2.entryAmount, tokenRaffle2.numberOfMembers, 
                                              tokenRaffle2.winnerIndex, tokenRaffle2.epoch);

    // Check analytics after epoch
    auto analytics = qraffle.getAnalytics();
    EXPECT_EQ(analytics.returnCode, QRAFFLE_SUCCESS);
    EXPECT_GT(analytics.totalBurnAmount, 0);
    EXPECT_GT(analytics.totalCharityAmount, 0);
    EXPECT_GT(analytics.totalShareholderAmount, 0);
    EXPECT_GT(analytics.totalWinnerAmount, 0);
}

TEST(ContractQraffle, RegisterInSystemWithQXMR)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Issue QXMR tokens to users
    id qxmrIssuer = qraffle.getState()->getQXMRIssuer();
    increaseEnergy(qxmrIssuer, 2000000000ULL);
    qraffle.issueAsset(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, 10000000000000, 0, 0);

    // Test successful registration with QXMR tokens
    for (const auto& user : users)
    {
        increaseEnergy(user, 1000);
        // Transfer QXMR tokens to user
        qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT, user);
        qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT, user);
        
        // Register using QXMR tokens
        auto result = qraffle.registerInSystem(user, 0, 1); // useQXMR = 1
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        registerCount++;
        qraffle.getState()->registerChecker(user, registerCount, true);
    }

    // Test insufficient QXMR tokens
    id poorUser = getUser(9999);
    increaseEnergy(poorUser, 1000);
    qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT - 1, poorUser);
    qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT - 1, poorUser);
    auto result = qraffle.registerInSystem(poorUser, 0, 1);
    EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_QXMR);
    qraffle.getState()->registerChecker(poorUser, registerCount, false);

    // Test already registered with QXMR
    qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT, users[0]);
    qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT, users[0]);
    result = qraffle.registerInSystem(users[0], 0, 1);
    EXPECT_EQ(result.returnCode, QRAFFLE_ALREADY_REGISTERED);
    qraffle.getState()->registerChecker(users[0], registerCount, true);
}

TEST(ContractQraffle, LogoutInSystemWithQXMR)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Issue QXMR tokens
    id qxmrIssuer = qraffle.getState()->getQXMRIssuer();
    increaseEnergy(qxmrIssuer, 2000000000ULL);
    qraffle.issueAsset(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, 10000000000000, 0, 0);

    // Register users with QXMR tokens first
    for (const auto& user : users)
    {
        increaseEnergy(user, 1000);
        qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT, user);
        qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT, user);
        qraffle.registerInSystem(user, 0, 1);
        registerCount++;
    }

    // Test successful logout with QXMR tokens
    for (const auto& user : users)
    {
        increaseEnergy(user, 1000);
        auto result = qraffle.logoutInSystem(user); 
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        
        // Check that user received QXMR refund
        uint64 expectedRefund = QRAFFLE_QXMR_REGISTER_AMOUNT - QRAFFLE_QXMR_LOGOUT_FEE;
        EXPECT_EQ(numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, user, user, QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), expectedRefund);
        
        registerCount--;
        qraffle.getState()->unregisterChecker(user, registerCount);
    }

    // Test unregistered user logout with QXMR
    increaseEnergy(users[0], 1000);
    auto result = qraffle.logoutInSystem(users[0]);
    EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);
}

TEST(ContractQraffle, MixedRegistrationAndLogout)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Issue QXMR tokens
    id qxmrIssuer = qraffle.getState()->getQXMRIssuer();
    increaseEnergy(qxmrIssuer, 2000000000ULL);
    qraffle.issueAsset(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, 10000000000000, 0, 0);

    // Register some users with qubic, some with QXMR
    for (size_t i = 0; i < users.size(); ++i)
    {
        if (i % 2 == 0)
        {
            // Register with qubic
            increaseEnergy(users[i], QRAFFLE_REGISTER_AMOUNT);
            auto result = qraffle.registerInSystem(users[i], QRAFFLE_REGISTER_AMOUNT, 0); // useQXMR = 0
            EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        }
        else
        {
            // Register with QXMR
            increaseEnergy(users[i], 1000);
            qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT, users[i]);
            qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT, users[i]);
            auto result = qraffle.registerInSystem(users[i], 0, 1); // useQXMR = 1
            EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        }
        registerCount++;
    }

    // Logout some users with qubic, some with QXMR
    for (size_t i = 0; i < users.size(); ++i)
    {
        if (i % 2 == 0)
        {
            // Logout with qubic
            auto result = qraffle.logoutInSystem(users[i]); 
            EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
            EXPECT_EQ(getBalance(users[i]), QRAFFLE_REGISTER_AMOUNT - QRAFFLE_LOGOUT_FEE);
        }
        else
        {
            // Logout with QXMR
            auto result = qraffle.logoutInSystem(users[i]);
            EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
            
            uint64 expectedRefund = QRAFFLE_QXMR_REGISTER_AMOUNT - QRAFFLE_QXMR_LOGOUT_FEE;
            EXPECT_EQ(numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, users[i], users[i], QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), expectedRefund);
        }
        registerCount--;
    }

    // Verify final state
    EXPECT_EQ(qraffle.getState()->getNumberOfRegisters(), registerCount);
}

TEST(ContractQraffle, QXMRInvalidTokenType)
{
    ContractTestingQraffle qraffle;

    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Issue QXMR tokens
    id qxmrIssuer = qraffle.getState()->getQXMRIssuer();
    increaseEnergy(qxmrIssuer, 2000000000ULL);
    qraffle.issueAsset(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, 10000000000000, 0, 0);

    // Register user with qubic (token type 1)
    increaseEnergy(users[0], QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(users[0], QRAFFLE_REGISTER_AMOUNT, 0);
    registerCount++;

    // Try to logout with QXMR when registered with qubic
    auto result = qraffle.logoutInSystem(users[0]); 
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);

    // Register user with QXMR (token type 2)
    increaseEnergy(users[1], 1000);
    qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT, users[1]);
    qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT, users[1]);
    qraffle.registerInSystem(users[1], 0, 1);
    registerCount++;

    // Try to logout
    result = qraffle.logoutInSystem(users[1]); 
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);

    registerCount--;
}

TEST(ContractQraffle, QXMRRevenueDistribution)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;

    // Issue QXMR tokens
    id qxmrIssuer = qraffle.getState()->getQXMRIssuer();
    increaseEnergy(qxmrIssuer, 2000000000ULL);
    qraffle.issueAsset(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, 10000000000000, 0, 0);

    // Register some users with QXMR to generate QXMR revenue
    for (const auto& user : users)
    {
        increaseEnergy(user, 1000);
        qraffle.transferShareOwnershipAndPossession(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, qxmrIssuer, QRAFFLE_QXMR_REGISTER_AMOUNT, user);
        qraffle.TransferShareManagementRights(qxmrIssuer, QRAFFLE_QXMR_ASSET_NAME, QRAFFLE_CONTRACT_INDEX, QRAFFLE_QXMR_REGISTER_AMOUNT, user);
        qraffle.registerInSystem(user, 0, 1);
        registerCount++;
    }

    uint64 expectedQXMRRevenue = 0;
    // Logout some users to generate QXMR revenue
    for (size_t i = 0; i < users.size(); ++i)
    {
        auto result = qraffle.logoutInSystem(users[i]);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        expectedQXMRRevenue += QRAFFLE_QXMR_LOGOUT_FEE;
        registerCount--;
    }

    // Check that QXMR revenue was recorded
    EXPECT_EQ(qraffle.getState()->getEpochQXMRRevenue(), expectedQXMRRevenue);

    // Test QXMR revenue distribution during epoch end
    increaseEnergy(users[0], QRAFFLE_DEFAULT_QRAFFLE_AMOUNT);
    qraffle.depositInQuRaffle(users[0], QRAFFLE_DEFAULT_QRAFFLE_AMOUNT);

    qraffle.endEpoch();
    EXPECT_EQ(qraffle.getState()->getEpochQXMRRevenue(), expectedQXMRRevenue - div(expectedQXMRRevenue, 676ull) * 676);
}

TEST(ContractQraffle, GetQuRaffleEntryAmountPerUser)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;
    uint32 entrySubmittedCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        auto result = qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        registerCount++;
    }

    // Test 1: Query entry amount for users who haven't submitted any
    for (const auto& user : users)
    {
        auto result = qraffle.getQuRaffleEntryAmountPerUser(user);
        EXPECT_EQ(result.returnCode, QRAFFLE_USER_NOT_FOUND);
        EXPECT_EQ(result.entryAmount, 0);
    }

    // Submit entry amounts for some users
    std::vector<uint64> submittedAmounts;
    for (size_t i = 0; i < users.size() / 2; ++i)
    {
        uint64 amount = random(1000000, 1000000000);
        auto result = qraffle.submitEntryAmount(users[i], amount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        submittedAmounts.push_back(amount);
        entrySubmittedCount++;
    }

    // Test 2: Query entry amount for users who have submitted amounts
    for (size_t i = 0; i < submittedAmounts.size(); ++i)
    {
        auto result = qraffle.getQuRaffleEntryAmountPerUser(users[i]);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(result.entryAmount, submittedAmounts[i]);
    }

    // Test 3: Query entry amount for users who haven't submitted amounts
    for (size_t i = submittedAmounts.size(); i < users.size(); ++i)
    {
        auto result = qraffle.getQuRaffleEntryAmountPerUser(users[i]);
        EXPECT_EQ(result.returnCode, QRAFFLE_USER_NOT_FOUND);
        EXPECT_EQ(result.entryAmount, 0);
    }

    // Test 4: Update entry amount and verify
    uint64 newAmount = random(1000000, 1000000000);
    auto result = qraffle.submitEntryAmount(users[0], newAmount);
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
    
    auto updatedResult = qraffle.getQuRaffleEntryAmountPerUser(users[0]);
    EXPECT_EQ(updatedResult.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(updatedResult.entryAmount, newAmount);

    // Test 5: Query for non-existent user
    id nonExistentUser = getUser(99999);
    auto nonExistentResult = qraffle.getQuRaffleEntryAmountPerUser(nonExistentUser);
    EXPECT_EQ(nonExistentResult.returnCode, QRAFFLE_USER_NOT_FOUND);
    EXPECT_EQ(nonExistentResult.entryAmount, 0);

    // Test 6: Query for unregistered user
    id unregisteredUser = getUser(88888);
    increaseEnergy(unregisteredUser, QRAFFLE_REGISTER_AMOUNT);
    auto unregisteredResult = qraffle.getQuRaffleEntryAmountPerUser(unregisteredUser);
    EXPECT_EQ(unregisteredResult.returnCode, QRAFFLE_USER_NOT_FOUND);
    EXPECT_EQ(unregisteredResult.entryAmount, 0);
}

TEST(ContractQraffle, GetQuRaffleEntryAverageAmount)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 5;
    uint32 entrySubmittedCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        auto result = qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        registerCount++;
    }

    // Test 1: Query average when no users have submitted entry amounts
    auto result = qraffle.getQuRaffleEntryAverageAmount();
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(result.entryAverageAmount, 0);

    // Submit entry amounts for some users
    std::vector<uint64> submittedAmounts;
    uint64 totalAmount = 0;
    for (size_t i = 0; i < users.size() / 2; ++i)
    {
        uint64 amount = random(1000000, 1000000000);
        increaseEnergy(users[i], amount);
        auto result = qraffle.submitEntryAmount(users[i], amount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        submittedAmounts.push_back(amount);
        totalAmount += amount;
        entrySubmittedCount++;
    }

    // Test 2: Query average with submitted amounts
    auto averageResult = qraffle.getQuRaffleEntryAverageAmount();
    EXPECT_EQ(averageResult.returnCode, QRAFFLE_SUCCESS);
    
    // Calculate expected average
    uint64 expectedAverage = 0;
    if (submittedAmounts.size() > 0)
    {
        expectedAverage = totalAmount / submittedAmounts.size();
    }
    EXPECT_EQ(averageResult.entryAverageAmount, expectedAverage);

    // Test 3: Add more users and verify average updates
    std::vector<uint64> additionalAmounts;
    uint64 additionalTotal = 0;
    for (size_t i = users.size() / 2; i < users.size(); ++i)
    {
        uint64 amount = random(1000000, 1000000000);
        auto result = qraffle.submitEntryAmount(users[i], amount);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        additionalAmounts.push_back(amount);
        additionalTotal += amount;
        entrySubmittedCount++;
    }

    // Calculate new expected average
    uint64 newTotalAmount = totalAmount + additionalTotal;
    uint64 newExpectedAverage = newTotalAmount / (submittedAmounts.size() + additionalAmounts.size());

    auto updatedAverageResult = qraffle.getQuRaffleEntryAverageAmount();
    EXPECT_EQ(updatedAverageResult.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(updatedAverageResult.entryAverageAmount, newExpectedAverage);

    // Test 4: Update existing user's entry amount and verify average
    uint64 updatedAmount = random(1000000, 1000000000);
    auto updateResult = qraffle.submitEntryAmount(users[0], updatedAmount);
    EXPECT_EQ(updateResult.returnCode, QRAFFLE_SUCCESS);
    
    // Recalculate expected average with updated amount
    uint64 recalculatedTotal = newTotalAmount - submittedAmounts[0] + updatedAmount;
    uint64 recalculatedAverage = recalculatedTotal / (submittedAmounts.size() + additionalAmounts.size());

    auto recalculatedAverageResult = qraffle.getQuRaffleEntryAverageAmount();
    EXPECT_EQ(recalculatedAverageResult.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(recalculatedAverageResult.entryAverageAmount, recalculatedAverage);
}