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

    void proposalChecker(uint32 index, const Asset& expectedToken, uint64 expectedEntryAmount, uint32 expectedProposals)
    {
        EXPECT_EQ(proposals.get(index).token.assetName, expectedToken.assetName);
        EXPECT_EQ(proposals.get(index).token.issuer, expectedToken.issuer);
        EXPECT_EQ(proposals.get(index).entryAmount, expectedEntryAmount);
        EXPECT_EQ(numberOfProposals, expectedProposals);
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
        Array<id, QRAFFLE_MAX_MEMBER> members;
        tokenRaffleMembers.get(raffleIndex, members);
        bool found = false;
        for (uint32 i = 0; i < numberOfTokenRaffleMembers.get(raffleIndex); i++)
        {
            if (members.get(i) == user)
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
        EXPECT_EQ(lagestWinnerAmount, expectedLargestWinner);
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
                                uint64 expectedEntryAmount, uint32 expectedMembers, uint32 expectedWinnerIndex, uint16 expectedEpoch)
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

    QRAFFLE::registerInSystem_output registerInSystem(const id& user, uint64 amount)
    {
        QRAFFLE::registerInSystem_input input;
        QRAFFLE::registerInSystem_output output;
        
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
        
        input.token = token;
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

    QRAFFLE::depositeInQuRaffle_output depositeInQuRaffle(const id& user, uint64 amount)
    {
        QRAFFLE::depositeInQuRaffle_input input;
        QRAFFLE::depositeInQuRaffle_output output;
        
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 6, input, output, user, amount);
        return output;
    }

    QRAFFLE::depositeInTokenRaffle_output depositeInTokenRaffle(const id& user, uint32 raffleIndex, uint64 amount)
    {
        QRAFFLE::depositeInTokenRaffle_input input;
        QRAFFLE::depositeInTokenRaffle_output output;
        
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

    QRAFFLE::getAnalaytics_output getAnalytics()
    {
        QRAFFLE::getAnalaytics_input input;
        QRAFFLE::getAnalaytics_output output;
        
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

    QRAFFLE::getEpochWinner_output getEpochWinner(uint32 epoch)
    {
        QRAFFLE::getEpochWinner_input input;
        QRAFFLE::getEpochWinner_output output;
        
        input.epoch = epoch;
        callFunction(QRAFFLE_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QRAFFLE::getEpochRaffleIndexes_output getEpochRaffleIndexes(uint16 epoch)
    {
        QRAFFLE::getEpochRaffleIndexes_input input;
        QRAFFLE::getEpochRaffleIndexes_output output;
        
        input.epoch = epoch;
        callFunction(QRAFFLE_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QRAFFLE::getEndedQuRaffle_output getEndedQuRaffle(uint16 epoch)
    {
        QRAFFLE::getEndedQuRaffle_input input;
        QRAFFLE::getEndedQuRaffle_output output;
        
        input.epoch = epoch;
        callFunction(QRAFFLE_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QRAFFLE::getActiveTokenRaffle_output getActiveTokenRaffle(uint32 raffleIndex)
    {
        QRAFFLE::getActiveTokenRaffle_input input;
        QRAFFLE::getActiveTokenRaffle_output output;
        
        input.indexOfTokenRaffle = raffleIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, 1000000000ULL);
        return output.issuedNumberOfShares;
    }

    sint64 transferShareOwnershipAndPossession(const id& issuer, uint64 assetName, sint64 numberOfShares, const id& newOwnerAndPossesor)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;

        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, issuer, 1000000ULL);
        return output.transferredNumberOfShares;
    }
};

// TEST(ContractQraffle, RegisterInSystem)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(1000, 1000);
//     uint32 registerCount = 0;

//     // Test successful registration
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         auto result = qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
//         qraffle.getState()->registerChecker(user, ++registerCount, true);
//     }

//     // // Test insufficient funds
//     id poorUser = getUser(9999);
//     increaseEnergy(poorUser, QRAFFLE_REGISTER_AMOUNT - 1);
//     auto result = qraffle.registerInSystem(poorUser, QRAFFLE_REGISTER_AMOUNT - 1);
//     EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_FUND);
//     qraffle.getState()->registerChecker(poorUser, registerCount, false);

//     // Test already registered
//     increaseEnergy(users[0], QRAFFLE_REGISTER_AMOUNT);
//     result = qraffle.registerInSystem(users[0], QRAFFLE_REGISTER_AMOUNT);
//     EXPECT_EQ(result.returnCode, QRAFFLE_ALREADY_REGISTERED);
//     qraffle.getState()->registerChecker(users[0], registerCount, true);
// }

TEST(ContractQraffle, LogoutInSystem)
{
    ContractTestingQraffle qraffle;
    
    auto users = getRandomUsers(1000, 1000);
    uint32 registerCount = 0;

    // Register users first
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
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
    id unregisteredUser = getUser(9999);
    qraffle.getState()->unregisterChecker(unregisteredUser, registerCount);
    auto result = qraffle.logoutInSystem(unregisteredUser);
    EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);
}

// TEST(ContractQraffle, SubmitEntryAmount)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(1000, 1000);
//     uint32 registerCount = 0;
//     uint32 entrySubmittedCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Test successful entry amount submission
//     for (const auto& user : users)
//     {
//         uint64 amount = random(1000000, 1000000000);
//         auto result = qraffle.submitEntryAmount(user, amount);
//         EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
//         qraffle.getState()->entryAmountChecker(user, amount, ++entrySubmittedCount);
//     }

//     // Test unregistered user
//     id unregisteredUser = getUser(9999);
//     increaseEnergy(unregisteredUser, QRAFFLE_REGISTER_AMOUNT);
//     auto result = qraffle.submitEntryAmount(unregisteredUser, 1000000);
//     EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);

//     // Test update entry amount
//     uint64 newAmount = random(1000000, 1000000000);
//     result = qraffle.submitEntryAmount(users[0], newAmount);
//     EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
//     qraffle.getState()->entryAmountChecker(users[0], newAmount, entrySubmittedCount);
// }

// TEST(ContractQraffle, SubmitProposal)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(100, 5);
//     uint32 registerCount = 0;
//     uint32 proposalCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Issue some test assets
//     id issuer = getUser(1000);
//     increaseEnergy(issuer, 1000000000ULL);
//     uint64 assetName1 = assetNameFromString("TEST1");
//     uint64 assetName2 = assetNameFromString("TEST2");
//     qraffle.issueAsset(issuer, assetName1, 1000000, 0, 0);
//     qraffle.issueAsset(issuer, assetName2, 2000000, 0, 0);

//     Asset token1, token2;
//     token1.assetName = assetName1;
//     token1.issuer = issuer;
//     token2.assetName = assetName2;
//     token2.issuer = issuer;

//     // Test successful proposal submission
//     for (const auto& user : users)
//     {
//         uint64 entryAmount = random(1000000, 1000000000);
//         Asset token = (random(0, 2) == 0) ? token1 : token2;
        
//         auto result = qraffle.submitProposal(user, token, entryAmount);
//         EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
//         qraffle.getState()->proposalChecker(proposalCount, token, entryAmount, ++proposalCount);
//     }

//     // Test unregistered user
//     id unregisteredUser = getUser(999);
//     auto result = qraffle.submitProposal(unregisteredUser, token1, 1000000);
//     EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);
// }

// TEST(ContractQraffle, VoteInProposal)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(100, 10);
//     uint32 registerCount = 0;
//     uint32 proposalCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Create a proposal
//     id issuer = getUser(1000);
//     increaseEnergy(issuer, 1000000000ULL);
//     uint64 assetName = assetNameFromString("VOTETEST");
//     qraffle.issueAsset(issuer, assetName, 1000000, 0, 0);

//     Asset token;
//     token.assetName = assetName;
//     token.issuer = issuer;

//     qraffle.submitProposal(users[0], token, 1000000);
//     proposalCount++;

//     uint32 yesVotes = 0, noVotes = 0;

//     // Test voting
//     for (const auto& user : users)
//     {
//         bit vote = (bit)random(0, 2);
//         auto result = qraffle.voteInProposal(user, 0, vote);
//         EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        
//         if (vote)
//             yesVotes++;
//         else
//             noVotes++;
        
//         qraffle.getState()->voteChecker(0, yesVotes, noVotes);
//     }

//     // Test duplicate vote (should change vote)
//     bit newVote = (bit)random(0, 2);
//     auto result = qraffle.voteInProposal(users[0], 0, newVote);
//     EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
    
//     if (newVote)
//         yesVotes++;
//     else
//         noVotes++;
    
//     qraffle.getState()->voteChecker(0, yesVotes, noVotes);

//     // Test unregistered user
//     id unregisteredUser = getUser(999);
//     result = qraffle.voteInProposal(unregisteredUser, 0, 1);
//     EXPECT_EQ(result.returnCode, QRAFFLE_UNREGISTERED);

//     // Test invalid proposal index
//     result = qraffle.voteInProposal(users[0], 999, 1);
//     EXPECT_EQ(result.returnCode, QRAFFLE_INVALID_PROPOSAL);
// }

// TEST(ContractQraffle, DepositeInQuRaffle)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(100, 5);
//     uint32 registerCount = 0;
//     uint32 memberCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Test successful deposit
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, qraffle.getState()->getQuRaffleEntryAmount());
//         auto result = qraffle.depositeInQuRaffle(user, qraffle.getState()->getQuRaffleEntryAmount());
//         EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
//         qraffle.getState()->quRaffleMemberChecker(user, ++memberCount);
//     }

//     // Test insufficient funds
//     id poorUser = getUser(999);
//     increaseEnergy(poorUser, qraffle.getState()->getQuRaffleEntryAmount() - 1);
//     auto result = qraffle.depositeInQuRaffle(poorUser, qraffle.getState()->getQuRaffleEntryAmount() - 1);
//     EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_FUND);

//     // Test already registered
//     increaseEnergy(users[0], qraffle.getState()->getQuRaffleEntryAmount());
//     result = qraffle.depositeInQuRaffle(users[0], qraffle.getState()->getQuRaffleEntryAmount());
//     EXPECT_EQ(result.returnCode, QRAFFLE_ALREADY_REGISTERED);
// }

// TEST(ContractQraffle, DepositeInTokenRaffle)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(100, 5);
//     uint32 registerCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Create a proposal and vote for it
//     id issuer = getUser(1000);
//     increaseEnergy(issuer, 1000000000ULL);
//     uint64 assetName = assetNameFromString("TOKENRAFFLE");
//     qraffle.issueAsset(issuer, assetName, 1000000, 0, 0);

//     Asset token;
//     token.assetName = assetName;
//     token.issuer = issuer;

//     qraffle.submitProposal(users[0], token, 1000000);
    
//     // Vote yes for the proposal
//     for (const auto& user : users)
//     {
//         qraffle.voteInProposal(user, 0, 1);
//     }

//     // End epoch to activate token raffle
//     qraffle.endEpoch();

//     // Test successful token raffle deposit
//     uint32 memberCount = 0;
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_TRANSFER_SHARE_FEE + 1000000);
//         qraffle.transferShareOwnershipAndPossession(issuer, assetName, 1000000, user);
        
//         auto result = qraffle.depositeInTokenRaffle(user, 0, QRAFFLE_TRANSFER_SHARE_FEE);
//         EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
//         qraffle.getState()->tokenRaffleMemberChecker(0, user, ++memberCount);
//     }

//     // Test insufficient funds
//     id poorUser = getUser(999);
//     increaseEnergy(poorUser, QRAFFLE_TRANSFER_SHARE_FEE - 1);
//     auto result = qraffle.depositeInTokenRaffle(poorUser, 0, QRAFFLE_TRANSFER_SHARE_FEE - 1);
//     EXPECT_EQ(result.returnCode, QRAFFLE_INSUFFICIENT_FUND);

//     // Test invalid token raffle index
//     result = qraffle.depositeInTokenRaffle(users[0], 999, QRAFFLE_TRANSFER_SHARE_FEE);
//     EXPECT_EQ(result.returnCode, QRAFFLE_INVALID_TOKEN_RAFFLE);
// }

// TEST(ContractQraffle, GetFunctions)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(100, 5);
//     uint32 registerCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Test getRegisters
//     auto registers = qraffle.getRegisters(0, 10);
//     EXPECT_EQ(registers.returnCode, QRAFFLE_SUCCESS);
//     EXPECT_LE(registers.registers.capacity(), 10);

//     // Test getAnalytics
//     auto analytics = qraffle.getAnalytics();
//     EXPECT_EQ(analytics.returnCode, QRAFFLE_SUCCESS);
//     qraffle.getState()->analyticsChecker(analytics.totalBurnAmount, analytics.totalCharityAmount, 
//                                         analytics.totalShareholderAmount, analytics.totalRegisterAmount,
//                                         analytics.totalFeeAmount, analytics.totalWinnerAmount, 
//                                         analytics.lagestWinnerAmount, analytics.numberOfRegisters,
//                                         analytics.numberOfProposals, analytics.numberOfQuRaffleMembers,
//                                         analytics.numberOfActiveTokenRaffle, analytics.numberOfEndedTokenRaffle,
//                                         analytics.numberOfEntryAmountSubmitted);

//     // Test getActiveProposal
//     auto proposal = qraffle.getActiveProposal(0);
//     EXPECT_EQ(proposal.returnCode, QRAFFLE_SUCCESS);

//     // Test getEndedTokenRaffle
//     auto endedRaffle = qraffle.getEndedTokenRaffle(0);
//     EXPECT_EQ(endedRaffle.returnCode, QRAFFLE_SUCCESS);

//     // Test getEpochWinner
//     auto epochWinner = qraffle.getEpochWinner(0);
//     EXPECT_EQ(epochWinner.returnCode, QRAFFLE_SUCCESS);

//     // Test getEpochRaffleIndexes
//     auto raffleIndexes = qraffle.getEpochRaffleIndexes(0);
//     EXPECT_EQ(raffleIndexes.returnCode, QRAFFLE_SUCCESS);

//     // Test getEndedQuRaffle
//     auto endedQuRaffle = qraffle.getEndedQuRaffle(0);
//     EXPECT_EQ(endedQuRaffle.returnCode, QRAFFLE_SUCCESS);

//     // Test getActiveTokenRaffle
//     auto activeRaffle = qraffle.getActiveTokenRaffle(0);
//     EXPECT_EQ(activeRaffle.returnCode, QRAFFLE_SUCCESS);
// }

// TEST(ContractQraffle, EndEpoch)
// {
//     ContractTestingQraffle qraffle;
    
//     auto users = getRandomUsers(100, 10);
//     uint32 registerCount = 0;

//     // Register users first
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
//         qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT);
//         registerCount++;
//     }

//     // Submit entry amounts
//     for (const auto& user : users)
//     {
//         uint64 amount = random(1000000, 1000000000);
//         qraffle.submitEntryAmount(user, amount);
//     }

//     // Create proposals and vote for them
//     id issuer = getUser(1000);
//     increaseEnergy(issuer, 1000000000ULL);
//     uint64 assetName1 = assetNameFromString("EPOCHTEST1");
//     uint64 assetName2 = assetNameFromString("EPOCHTEST2");
//     qraffle.issueAsset(issuer, assetName1, 1000000, 0, 0);
//     qraffle.issueAsset(issuer, assetName2, 2000000, 0, 0);

//     Asset token1, token2;
//     token1.assetName = assetName1;
//     token1.issuer = issuer;
//     token2.assetName = assetName2;
//     token2.issuer = issuer;

//     qraffle.submitProposal(users[0], token1, 1000000);
//     qraffle.submitProposal(users[1], token2, 2000000);

//     // Vote yes for both proposals
//     for (const auto& user : users)
//     {
//         qraffle.voteInProposal(user, 0, 1);
//         qraffle.voteInProposal(user, 1, 1);
//     }

//     // Deposit in QuRaffle
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, qraffle.getState()->getQuRaffleEntryAmount());
//         qraffle.depositeInQuRaffle(user, qraffle.getState()->getQuRaffleEntryAmount());
//     }

//     // Deposit in token raffles
//     for (const auto& user : users)
//     {
//         increaseEnergy(user, QRAFFLE_TRANSFER_SHARE_FEE + 1000000);
//         qraffle.transferShareOwnershipAndPossession(issuer, assetName1, 1000000, user);
//         qraffle.transferShareOwnershipAndPossession(issuer, assetName2, 2000000, user);
//     }

//     // End epoch
//     qraffle.endEpoch();

//     // Check that QuRaffle was processed
//     auto quRaffle = qraffle.getEndedQuRaffle(0);
//     EXPECT_EQ(quRaffle.returnCode, QRAFFLE_SUCCESS);
//     qraffle.getState()->quRaffleWinnerChecker(0, quRaffle.epochWinner, quRaffle.receivedAmount, 
//                                             quRaffle.entryAmount, quRaffle.numberOfMembers, quRaffle.winnerIndex);

//     // Check that token raffles were processed
//     auto tokenRaffle1 = qraffle.getEndedTokenRaffle(0);
//     EXPECT_EQ(tokenRaffle1.returnCode, QRAFFLE_SUCCESS);
//     qraffle.getState()->endedTokenRaffleChecker(0, tokenRaffle1.epochWinner, token1, 
//                                               tokenRaffle1.entryAmount, tokenRaffle1.numberOfMembers, 
//                                               tokenRaffle1.winnerIndex, tokenRaffle1.epoch);

//     auto tokenRaffle2 = qraffle.getEndedTokenRaffle(1);
//     EXPECT_EQ(tokenRaffle2.returnCode, QRAFFLE_SUCCESS);
//     qraffle.getState()->endedTokenRaffleChecker(1, tokenRaffle2.epochWinner, token2, 
//                                               tokenRaffle2.entryAmount, tokenRaffle2.numberOfMembers, 
//                                               tokenRaffle2.winnerIndex, tokenRaffle2.epoch);

//     // Check analytics after epoch
//     auto analytics = qraffle.getAnalytics();
//     EXPECT_EQ(analytics.returnCode, QRAFFLE_SUCCESS);
//     EXPECT_GT(analytics.totalBurnAmount, 0);
//     EXPECT_GT(analytics.totalCharityAmount, 0);
//     EXPECT_GT(analytics.totalShareholderAmount, 0);
//     EXPECT_GT(analytics.totalWinnerAmount, 0);
// }
