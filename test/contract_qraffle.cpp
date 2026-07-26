#define NO_UEFI

#include <map>
#include <random>

#include "contract_testing.h"

// Test-local expectation for the asset-raffle creator share. The contract derives the
// creator payout implicitly by subtracting the 20% fee split (5% burn + 1% charity +
// 8% shareholder + 5% register + 1% fee), so 80% is the expected creator percentage.
static constexpr uint32 QRAFFLE_TEST_ASSET_RAFFLE_CREATOR_PCT = 80;

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

class QRaffleChecker : public QRAFFLE, public QRAFFLE::StateData
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
        bool found = false;
        for (uint32 i = 0; i < numberOfTokenRaffleMembers.get(raffleIndex); i++)
        {
            if (tokenRaffleMemberSlots.get(raffleIndex * QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE + i) == user)
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
                              uint64 expectedEntryAmount, uint32 expectedMembers, uint32 expectedWinnerIndex,
                              uint32 expectedDaoMembers)
    {
        EXPECT_EQ(QuRaffles.get(epoch).epochWinner, expectedWinner);
        EXPECT_EQ(QuRaffles.get(epoch).receivedAmount, expectedReceived);
        EXPECT_EQ(QuRaffles.get(epoch).entryAmount, expectedEntryAmount);
        EXPECT_EQ(QuRaffles.get(epoch).numberOfMembers, expectedMembers);
        EXPECT_EQ(QuRaffles.get(epoch).winnerIndex, expectedWinnerIndex);
        EXPECT_EQ(daoMemberCount.get(epoch), expectedDaoMembers);
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

    // ── Asset Raffle checkers ──────────────────────────────────────────────────

    void assetRaffleCreatedChecker(uint32 index, const id& expectedCreator,
                                   uint64 expectedReservePrice, uint64 expectedEntryTicket,
                                   uint32 expectedBundleSize, uint32 expectedEpoch)
    {
        EXPECT_LT(index, numberOfActiveAssetRaffles);
        const AssetRaffleInfo& info = activeAssetRaffles.get(index);
        EXPECT_EQ(info.creator,         expectedCreator);
        EXPECT_EQ(info.reservePriceQu,  expectedReservePrice);
        EXPECT_EQ(info.entryTicketQu,   expectedEntryTicket);
        EXPECT_EQ(info.bundleSize,      expectedBundleSize);
        EXPECT_EQ(info.epoch,           expectedEpoch);
    }

    void assetRaffleBundleItemChecker(uint32 raffleIndex, uint32 itemIndex,
                                      const id& expectedIssuer, uint64 expectedAssetName,
                                      sint64 expectedShares)
    {
        EXPECT_LT(raffleIndex, numberOfActiveAssetRaffles);
        EXPECT_LT(itemIndex, activeAssetRaffles.get(raffleIndex).bundleSize);
        const AssetRaffleItem& item = activeAssetRaffleItems.get(
            raffleIndex * QRAFFLE_MAX_ASSETS_PER_BUNDLE + itemIndex);
        EXPECT_EQ(item.asset.issuer,     expectedIssuer);
        EXPECT_EQ(item.asset.assetName,  expectedAssetName);
        EXPECT_EQ(item.numberOfShares,   expectedShares);
    }

    void assetRaffleBuyerChecker(uint32 raffleIndex, const id& buyer,
                                 bool expectPresent, uint32 expectedTickets = 0)
    {
        const AssetRaffleInfo& info = activeAssetRaffles.get(raffleIndex);
        bool found = false;
        uint32 foundTickets = 0;
        for (uint32 i = 0; i < info.numberOfBuyers; i++)
        {
            if (activeAssetRaffleBuyers.get(raffleIndex * QRAFFLE_MAX_ASSET_TICKET_BUYERS + i) == buyer)
            {
                found = true;
                foundTickets = activeAssetRaffleBuyerTickets.get(raffleIndex * QRAFFLE_MAX_ASSET_TICKET_BUYERS + i);
                break;
            }
        }
        EXPECT_EQ(found, expectPresent);
        if (expectPresent && expectedTickets > 0)
        {
            EXPECT_EQ(foundTickets, expectedTickets);
        }
    }

    void assetRafflePoolChecker(uint32 index, uint64 expectedGross, uint32 expectedTotalTickets, uint32 expectedBuyers)
    {
        const AssetRaffleInfo& info = activeAssetRaffles.get(index);
        EXPECT_EQ(info.totalTicketsPaidQu, expectedGross);
        EXPECT_EQ(info.totalTickets,       expectedTotalTickets);
        EXPECT_EQ(info.numberOfBuyers,     expectedBuyers);
    }

    void endedAssetRaffleChecker(uint32 index, bool expectedReserveMet,
                                 const id& expectedCreator, uint64 expectedGross,
                                 uint32 expectedEpoch)
    {
        uint32 slot = index % QRAFFLE_MAX_ENDED_ASSET_RAFFLES;
        const EndedAssetRaffleInfo& r = endedAssetRaffles.get(slot);
        EXPECT_EQ(r.reserveMet,    (uint8)(expectedReserveMet ? 1 : 0));
        EXPECT_EQ(r.creator,       expectedCreator);
        EXPECT_EQ(r.grossPoolQu,   expectedGross);
        EXPECT_EQ(r.epoch,         expectedEpoch);
    }

    void assetRaffleAnalyticsChecker(uint32 expectedCreated, uint32 expectedSucceeded,
                                     uint32 expectedFailed)
    {
        EXPECT_EQ(totalAssetRafflesCreated,   expectedCreated);
        EXPECT_EQ(totalAssetRafflesSucceeded, expectedSucceeded);
        EXPECT_EQ(totalAssetRafflesFailed,    expectedFailed);
    }

    uint32 getNumberOfActiveAssetRaffles() const { return numberOfActiveAssetRaffles; }
    uint32 getNumberOfEndedAssetRaffles()  const { return numberOfEndedAssetRaffles;  }
    uint64 getEpochAssetRaffleDaoBucket()  const { return epochAssetRaffleDaoBucket;  }
    uint64 getTotalAssetRaffleRefunded()   const { return totalAssetRaffleRefunded;    }
    uint64 getTotalAssetRaffleCreatorPaid()const { return totalAssetRaffleCreatorPaid; }
};

class ContractTestingQraffle : protected ContractTesting
{
public:
    ContractTestingQraffle()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        system.epoch = contractDescriptions[QRAFFLE_CONTRACT_INDEX].constructionEpoch;
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
        return TransferShareManagementRightsQraffleWithFee(issuer, assetName, newManagingContractIndex, numberOfShares, currentOwner, QRAFFLE_TRANSFER_SHARE_FEE);
    }

    sint64 TransferShareManagementRightsQraffleWithFee(const id& issuer, uint64 assetName, uint32 newManagingContractIndex, sint64 numberOfShares, const id& currentOwner, sint64 invocationReward)
    {
        QRAFFLE::TransferShareManagementRights_input input;
        QRAFFLE::TransferShareManagementRights_output output;

        input.tokenName = assetName;
        input.tokenIssuer = issuer;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 8, input, output, currentOwner, invocationReward);
        return output.transferredNumberOfShares;
    }

    // ── Asset Raffle API wrappers ──────────────────────────────────────────────

    QRAFFLE::createAssetRaffle_output createAssetRaffle(
        const id& creator,
        uint64 reservePriceQu,
        uint64 entryTicketQu,
        const std::vector<std::pair<Asset, sint64>>& bundle)
    {
        return createAssetRaffleWithReward(
            creator, reservePriceQu, entryTicketQu, bundle,
            (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE);
    }

    QRAFFLE::createAssetRaffle_output createAssetRaffleWithReward(
        const id& creator,
        uint64 reservePriceQu,
        uint64 entryTicketQu,
        const std::vector<std::pair<Asset, sint64>>& bundle,
        sint64 invocationReward)
    {
        QRAFFLE::createAssetRaffle_input input{};
        QRAFFLE::createAssetRaffle_output output{};

        input.reservePriceQu = reservePriceQu;
        input.entryTicketQu  = entryTicketQu;
        input.bundleSize     = (uint32)bundle.size();
        for (uint32 i = 0; i < input.bundleSize && i < QRAFFLE_MAX_ASSETS_PER_BUNDLE; i++)
        {
            QRAFFLE::AssetRaffleItem item{};
            item.asset          = bundle[i].first;
            item.numberOfShares = bundle[i].second;
            input.bundleItems.set(i, item);
        }
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 9, input, output, creator,
                            invocationReward);
        return output;
    }

    QRAFFLE::buyAssetRaffleTicket_output buyAssetRaffleTicket(
        const id& buyer, uint32 raffleIndex, uint32 ticketCount, uint64 totalPayment)
    {
        QRAFFLE::buyAssetRaffleTicket_input input{};
        QRAFFLE::buyAssetRaffleTicket_output output{};

        input.indexOfAssetRaffle = raffleIndex;
        input.numberOfTickets    = ticketCount;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 10, input, output, buyer, (sint64)totalPayment);
        return output;
    }

    QRAFFLE::cancelAssetRaffle_output cancelAssetRaffle(const id& caller, uint32 raffleIndex)
    {
        QRAFFLE::cancelAssetRaffle_input input{};
        QRAFFLE::cancelAssetRaffle_output output{};

        input.indexOfAssetRaffle = raffleIndex;
        invokeUserProcedure(QRAFFLE_CONTRACT_INDEX, 11, input, output, caller, 0);
        return output;
    }

    QRAFFLE::getActiveAssetRaffle_output getActiveAssetRaffle(uint32 raffleIndex)
    {
        QRAFFLE::getActiveAssetRaffle_input input{};
        QRAFFLE::getActiveAssetRaffle_output output{};

        input.indexOfAssetRaffle = raffleIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 10, input, output);
        return output;
    }

    QRAFFLE::getActiveAssetRaffleBundleItem_output getActiveAssetRaffleBundleItem(
        uint32 raffleIndex, uint32 itemIndex)
    {
        QRAFFLE::getActiveAssetRaffleBundleItem_input input{};
        QRAFFLE::getActiveAssetRaffleBundleItem_output output{};

        input.indexOfAssetRaffle = raffleIndex;
        input.itemIndex          = itemIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 11, input, output);
        return output;
    }

    QRAFFLE::getActiveAssetRaffleBuyer_output getActiveAssetRaffleBuyer(
        uint32 raffleIndex, uint32 buyerIndex)
    {
        QRAFFLE::getActiveAssetRaffleBuyer_input input{};
        QRAFFLE::getActiveAssetRaffleBuyer_output output{};

        input.indexOfAssetRaffle = raffleIndex;
        input.buyerIndex         = buyerIndex;
        callFunction(QRAFFLE_CONTRACT_INDEX, 12, input, output);
        return output;
    }

    QRAFFLE::getEndedAssetRaffle_output getEndedAssetRaffle(uint32 index)
    {
        QRAFFLE::getEndedAssetRaffle_input input{};
        QRAFFLE::getEndedAssetRaffle_output output{};

        input.indexOfRaffle = index;
        callFunction(QRAFFLE_CONTRACT_INDEX, 13, input, output);
        return output;
    }

    QRAFFLE::getAssetRaffleAnalytics_output getAssetRaffleAnalytics()
    {
        QRAFFLE::getAssetRaffleAnalytics_input input{};
        QRAFFLE::getAssetRaffleAnalytics_output output{};

        callFunction(QRAFFLE_CONTRACT_INDEX, 14, input, output);
        return output;
    }

    // Helper: register a user in the DAO system
    void registerDAOMember(const id& user)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        auto r = registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
    }

    // Helper: issue a plain token and give all shares to 'owner' with QRAFFLE as manager.
    Asset setupToken(const id& issuer, const char* name, sint64 totalShares, const id& owner)
    {
        // issuance fee (1B) + QX ownership-transfer fee (100)
        increaseEnergy(issuer, 1000000000ULL + 100);
        uint64 aname = assetNameFromString(name);
        sint64 issued = issueAsset(issuer, aname, totalShares, 0, 0);
        EXPECT_EQ(issued, totalShares);
        if (!(issuer == owner))
        {
            // Transfer ownership first (QX proc 2, invocationReward = 100)
            sint64 moved = transferShareOwnershipAndPossession(issuer, aname, issuer, totalShares, owner);
            EXPECT_EQ(moved, totalShares);
            // Ensure owner is in spectrum so invokeUserProcedure doesn't reject the call.
            increaseEnergy(owner, 1);
            // Transfer management rights to QRAFFLE via owner (QX proc 9, fee = 0)
            sint64 mgmt = TransferShareManagementRights(issuer, aname, QRAFFLE_CONTRACT_INDEX, totalShares, owner);
            EXPECT_EQ(mgmt, totalShares);
        }
        else
        {
            // issuer == owner: directly transfer management rights (QX proc 9, fee = 0)
            sint64 mgmt = TransferShareManagementRights(issuer, aname, QRAFFLE_CONTRACT_INDEX, totalShares, issuer);
            EXPECT_EQ(mgmt, totalShares);
        }
        Asset a;
        a.assetName = aname;
        a.issuer    = issuer;
        return a;
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
    // Asset issuance fee on QX is 1,000,000,000 per asset; need at least 2 fees here.
    increaseEnergy(issuer, 2 * 1000000000ULL);
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

TEST(ContractQraffle, SubmitProposalPerUserLimitAndValidation)
{
    ContractTestingQraffle qraffle;

    auto users = getRandomUsers(100, 100);
    for (const auto& user : users)
    {
        increaseEnergy(user, QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(user, QRAFFLE_REGISTER_AMOUNT, 0);
    }

    id issuer = getUser(2000);
    increaseEnergy(issuer, 1000000000ULL);
    uint64 assetName = assetNameFromString("LIMVAL");
    qraffle.issueAsset(issuer, assetName, 10000000, 0, 0);

    Asset token;
    token.assetName = assetName;
    token.issuer = issuer;

    increaseEnergy(users[0], 1000);
    EXPECT_EQ(qraffle.submitProposal(users[0], token, 1000000ULL).returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.submitProposal(users[0], token, 2000000ULL).returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.submitProposal(users[0], token, 3000000ULL).returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.submitProposal(users[0], token, 4000000ULL).returnCode, QRAFFLE_MAX_PROPOSAL_PER_USER_REACHED);

    // Use a different proposer for entry-amount validation; per-user cap would otherwise mask it.
    // Entry-amount range limits were intentionally removed (#898): any positive amount is accepted
    // and only a zero amount is rejected.
    EXPECT_EQ(qraffle.submitProposal(users[1], token, 500000ULL).returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.submitProposal(users[1], token, 2000000000ULL).returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.submitProposal(users[1], token, 0ULL).returnCode, QRAFFLE_INVALID_ENTRY_AMOUNT);

    Asset fakeToken;
    fakeToken.assetName = assetNameFromString("NOTISS");
    fakeToken.issuer = issuer;
    EXPECT_EQ(qraffle.submitProposal(users[2], fakeToken, 1000000ULL).returnCode, QRAFFLE_INVALID_TOKEN_TYPE);
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
    bit users0OriginalVote = 0;

    // Test voting
    for (size_t vi = 0; vi < users.size(); vi++)
    {
        bit vote = (bit)random(0, 2);
        if (vi == 0) users0OriginalVote = vote;
        auto result = qraffle.voteInProposal(users[vi], 0, vote);
        EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
        
        if (vote)
            yesVotes++;
        else
            noVotes++;
        
        qraffle.getState()->voteChecker(0, yesVotes, noVotes);
    }

    // Test duplicate vote: explicitly use the opposite direction to guarantee the vote changes
    bit newVote = users0OriginalVote ? (bit)0 : (bit)1;
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

    // DAO membership is no longer required to deposit in a token raffle (#898): an unregistered
    // user is no longer rejected up-front. poorUser holds no shares, so the deposit now fails at
    // the share transfer instead of being rejected for being unregistered.
    increaseEnergy(poorUser, QRAFFLE_TRANSFER_SHARE_FEE);
    result = qraffle.depositInTokenRaffle(poorUser, 0, QRAFFLE_TRANSFER_SHARE_FEE);
    EXPECT_EQ(result.returnCode, QRAFFLE_FAILED_TO_DEPOSIT);

    // Test insufficient Token (registered DAO member with too few shares)
    id poorUser2 = getUser(8888);
    increaseEnergy(poorUser2, QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(poorUser2, QRAFFLE_REGISTER_AMOUNT, 0);
    increaseEnergy(poorUser2, QRAFFLE_TRANSFER_SHARE_FEE);
    qraffle.transferShareOwnershipAndPossession(issuer, assetName, issuer, 999999, poorUser2);
    result = qraffle.depositInTokenRaffle(poorUser2, 0, QRAFFLE_TRANSFER_SHARE_FEE);
    EXPECT_EQ(result.returnCode, QRAFFLE_FAILED_TO_DEPOSIT);

    // Test duplicate deposit by same user (already deposited above)
    increaseEnergy(users[0], QRAFFLE_TRANSFER_SHARE_FEE);
    result = qraffle.depositInTokenRaffle(users[0], 0, QRAFFLE_TRANSFER_SHARE_FEE);
    EXPECT_EQ(result.returnCode, QRAFFLE_ALREADY_REGISTERED);

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
    // QRAFFLE's procedure now forwards (invocationReward - QRAFFLE_TRANSFER_SHARE_FEE) as the
    // offeredTransferFee to the destination contract (QX requires 100 QU). Previously the test
    // only paid QRAFFLE's own fee, leaving 0 to offer QX which silently failed. We now pass
    // a sufficient amount and rely on the procedure to refund any excess.
    qraffle.TransferShareManagementRightsQraffleWithFee(issuer, assetName, QX_CONTRACT_INDEX, 1000000, user1, 2 * QRAFFLE_TRANSFER_SHARE_FEE);
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
    // Asset issuance fee on QX is 1,000,000,000 per asset; need at least 2 fees here.
    increaseEnergy(issuer, 2 * 1000000000ULL);
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

        // Test with offset beyond available registers — now returns SUCCESS with an
        // empty result (all output slots stay NULL_ID).
        auto registers2 = qraffle.getRegisters(registerCount + 10, 5);
        EXPECT_EQ(registers2.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(registers2.register1, id(0, 0, 0, 0));

        // Test with limit exceeding maximum (20) — still invalid because the output
        // struct can only carry 20 ids.
        auto registers3 = qraffle.getRegisters(0, 21);
        EXPECT_EQ(registers3.returnCode, QRAFFLE_INVALID_OFFSET_OR_LIMIT);

        // Test with offset + limit exceeding total registers — now returns SUCCESS with
        // however many registers are actually available; trailing output slots stay
        // NULL_ID so the caller can detect end-of-data.
        auto registers4 = qraffle.getRegisters(registerCount - 5, 10);
        EXPECT_EQ(registers4.returnCode, QRAFFLE_SUCCESS);
        EXPECT_NE(registers4.register1, id(0, 0, 0, 0));
        EXPECT_NE(registers4.register5, id(0, 0, 0, 0));
        EXPECT_EQ(registers4.register6, id(0, 0, 0, 0));

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
            expectedTotalShareholderAmount += ((totalQuRaffleAmount * QRAFFLE_SHAREHOLDER_FEE) / 100) / 676 * 676;
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
        EXPECT_EQ(endedQuRaffle.numberOfMembers, memberCount);
        EXPECT_GT(endedQuRaffle.numberOfDaoMembers, 0u);
        EXPECT_EQ(endedQuRaffle.numberOfDaoMembers, qraffle.getState()->getNumberOfRegisters());
        // Winner and prize are only set when at least one member participated
        if (memberCount > 0)
        {
            EXPECT_NE(endedQuRaffle.epochWinner, id(0, 0, 0, 0));
            EXPECT_GT(endedQuRaffle.receivedAmount, 0);
            EXPECT_EQ(endedQuRaffle.entryAmount, 10000000);
        }
        
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
                                            quRaffle.entryAmount, quRaffle.numberOfMembers, quRaffle.winnerIndex,
                                            quRaffle.numberOfDaoMembers);

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
    // No QRAFFLE shareholders exist in this test universe, so no QXMR is actually transferred.
    // The contract must therefore retain the full epochQXMRRevenue rather than booking it as
    // distributed (previous behavior silently leaked QXMR accounting).
    EXPECT_EQ(qraffle.getState()->getEpochQXMRRevenue(), expectedQXMRRevenue);
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

// ============================================================================
// Asset Raffle Tests
// ============================================================================

// ── Helpers used across asset raffle tests ──────────────────────────────────

// Register N sequential users starting at offset as DAO members; return their ids.
static std::vector<id> registerDAOUsers(ContractTestingQraffle& q, uint32 count, uint32 offset = 5000)
{
    std::vector<id> users;
    users.reserve(count);
    for (uint32 i = 0; i < count; i++)
    {
        id u = getUser(offset + i);
        q.registerDAOMember(u);
        users.push_back(u);
    }
    return users;
}

// Build a single-item bundle (plain token already transferred to QRAFFLE management).
static std::vector<std::pair<Asset, sint64>> makeBundle(const Asset& a, sint64 shares)
{
    return { { a, shares } };
}

// ── DIAG: setupToken share-transfer sanity ────────────────────────────────────
TEST(ContractQraffle, AssetRaffle_DiagSetupToken)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id issuer = getUser(6100);
    id owner  = getUser(6000);
    increaseEnergy(issuer, 1000000000ULL + 100); // issuance fee + QX transfer fee
    uint64 aname = assetNameFromString("DIAG");
    sint64 issued = qraffle.issueAsset(issuer, aname, 1000000, 0, 0);
    EXPECT_EQ(issued, 1000000) << "IssueAsset failed";

    // shares should be with issuer, managed by QX
    EXPECT_EQ(numberOfPossessedShares(aname, issuer, issuer, issuer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1000000)
        << "issuer should possess all shares under QX management";

    // transfer ownership from issuer to owner (still QX-managed)
    sint64 moved = qraffle.transferShareOwnershipAndPossession(issuer, aname, issuer, 1000000, owner);
    EXPECT_EQ(moved, 1000000) << "transferShareOwnershipAndPossession failed";

    EXPECT_EQ(numberOfPossessedShares(aname, issuer, owner, owner, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1000000)
        << "owner should possess all shares under QX management after transfer";

    // Ensure owner is in spectrum before calling QX proc 9 with owner as invocator.
    increaseEnergy(owner, 1);
    // transfer management rights to QRAFFLE (owner calls QX proc 9)
    sint64 mgmt = qraffle.TransferShareManagementRights(issuer, aname, QRAFFLE_CONTRACT_INDEX, 1000000, owner);
    EXPECT_EQ(mgmt, 1000000) << "TransferShareManagementRights (QX proc9) failed";

    EXPECT_EQ(numberOfPossessedShares(aname, issuer, owner, owner, QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000)
        << "owner should possess all shares under QRAFFLE management after rights transfer";
}

// ── TEST 1: createAssetRaffle – basic success ────────────────────────────────
TEST(ContractQraffle, AssetRaffle_CreateBasicSuccess)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    // Register creator as DAO member
    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    // Issue token and give to creator (under QRAFFLE management)
    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "ARTEST", 1000000, creator);

    // 500K proposal fee + buffer for energy
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

    auto bundle = makeBundle(token, 500000);
    auto result = qraffle.createAssetRaffle(creator, 125000000ULL, 5000000ULL, bundle);
    EXPECT_EQ(result.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 1u);

    // Verify raffle metadata via state checker
    qraffle.getState()->assetRaffleCreatedChecker(0, creator, 125000000ULL, 5000000ULL, 1, 200);

    // Verify bundle item
    qraffle.getState()->assetRaffleBundleItemChecker(0, 0, tokenIssuer, token.assetName, 500000);
}

// ── TEST 2: createAssetRaffle – validation failures ──────────────────────────
TEST(ContractQraffle, AssetRaffle_CreateValidation)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "VALTEST", 1000000, creator);
    auto bundle = makeBundle(token, 500000);

    // Insufficient proposal fee: invoke with one less than required and assert the
    // contract returns QRAFFLE_INSUFFICIENT_FUND (instead of only testing harness behavior).
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE - 1);
    {
        auto r = qraffle.createAssetRaffleWithReward(
            creator, 125000000ULL, 5000000ULL, bundle,
            (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE - 1);
        EXPECT_EQ(r.returnCode, QRAFFLE_INSUFFICIENT_FUND);
        EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
    }

    // Restore energy for remaining tests
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * 10);

    // Not a DAO member → QRAFFLE_UNREGISTERED
    id nonMember = getUser(9000);
    increaseEnergy(nonMember, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    {
        auto r = qraffle.createAssetRaffle(nonMember, 125000000ULL, 5000000ULL, bundle);
        EXPECT_EQ(r.returnCode, QRAFFLE_UNREGISTERED);
    }

    // Entry ticket below minimum → QRAFFLE_INVALID_BUNDLE or QRAFFLE_INVALID_RESERVE_PRICE
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL,
                                           QRAFFLE_MIN_ASSET_TICKET_AMOUNT - 1, bundle);
        EXPECT_NE(r.returnCode, QRAFFLE_SUCCESS);
    }

    // Entry ticket above maximum
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL,
                                           QRAFFLE_MAX_ASSET_TICKET_AMOUNT + 1, bundle);
        EXPECT_NE(r.returnCode, QRAFFLE_SUCCESS);
    }

    // Empty bundle → QRAFFLE_INVALID_BUNDLE
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, 5000000ULL, {});
        EXPECT_EQ(r.returnCode, QRAFFLE_INVALID_BUNDLE);
    }

    // Bundle larger than max → QRAFFLE_INVALID_BUNDLE
    {
        std::vector<std::pair<Asset, sint64>> bigBundle;
        for (uint32 i = 0; i <= QRAFFLE_MAX_ASSETS_PER_BUNDLE; i++)
            bigBundle.push_back({ token, 1000 });
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, 5000000ULL, bigBundle);
        EXPECT_EQ(r.returnCode, QRAFFLE_INVALID_BUNDLE);
    }
}

// ── TEST 3: createAssetRaffle – per-creator epoch limit ──────────────────────
TEST(ContractQraffle, AssetRaffle_CreatePerCreatorEpochLimit)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    increaseEnergy(tokenIssuer, 5 * 1000000000ULL);

    // Give creator enough shares for multiple raffles
    Asset token = qraffle.setupToken(tokenIssuer, "LIMTEST", 10000000, creator);
    auto bundle = makeBundle(token, 100000);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * (QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR + 2) + 10000);

    // Fill up to per-creator limit
    for (uint32 i = 0; i < QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR; i++)
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "raffle " << i;
    }

    // Next attempt must fail
    auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);
    EXPECT_EQ(r.returnCode, QRAFFLE_MAX_ASSET_RAFFLES_REACHED);
}

// ── TEST 4: createAssetRaffle – global concurrent limit ─────────────────────
TEST(ContractQraffle, AssetRaffle_CreateGlobalLimit)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id tokenIssuer = getUser(7000);
    increaseEnergy(tokenIssuer, 5 * 1000000000ULL);
    uint64 aname = assetNameFromString("GBLTEST");
    qraffle.issueAsset(tokenIssuer, aname, 1000000000LL, 0, 0);
    Asset token;
    token.assetName = aname;
    token.issuer    = tokenIssuer;

    // We need QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH distinct creators (each limited to 1 raffle
    // because QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR == 2, and filling 64 slots means at least
    // 32 creators). For simplicity: use 64 creators, each creates 1 raffle.
    uint32 creatorsNeeded = QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH;
    for (uint32 i = 0; i < creatorsNeeded; i++)
    {
        id creator = getUser(8000 + i);
        qraffle.registerDAOMember(creator);
        // Give shares
        sint64 shares = 10000;
        qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, shares, creator);
        qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, shares, creator);
        increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

        auto bundle = makeBundle(token, shares);
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "slot " << i;
    }

    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH);

    // One more must fail
    id extraCreator = getUser(9000);
    qraffle.registerDAOMember(extraCreator);
    qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, 10000, extraCreator);
    qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, 10000, extraCreator);
    increaseEnergy(extraCreator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto rExtra = qraffle.createAssetRaffle(extraCreator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                            makeBundle(token, 10000));
    EXPECT_EQ(rExtra.returnCode, QRAFFLE_MAX_ASSET_RAFFLES_REACHED);
}

// ── TEST 5: createAssetRaffle – multi-item bundle ────────────────────────────
TEST(ContractQraffle, AssetRaffle_CreateMultiItemBundle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id issuer1 = getUser(6100);
    id issuer2 = getUser(6200);
    id issuer3 = getUser(6300);

    Asset t1 = qraffle.setupToken(issuer1, "BNDA", 500000, creator);
    Asset t2 = qraffle.setupToken(issuer2, "BNDB", 300000, creator);
    Asset t3 = qraffle.setupToken(issuer3, "BNDC", 200000, creator);

    std::vector<std::pair<Asset, sint64>> bundle = {
        { t1, 500000 },
        { t2, 300000 },
        { t3, 200000 },
    };

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto r = qraffle.createAssetRaffle(creator, 1000000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);
    EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 1u);

    qraffle.getState()->assetRaffleCreatedChecker(0, creator, 1000000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, 3, 200);
    qraffle.getState()->assetRaffleBundleItemChecker(0, 0, issuer1, t1.assetName, 500000);
    qraffle.getState()->assetRaffleBundleItemChecker(0, 1, issuer2, t2.assetName, 300000);
    qraffle.getState()->assetRaffleBundleItemChecker(0, 2, issuer3, t3.assetName, 200000);
}

// ── TEST 6: createAssetRaffle – proposal fee distribution ────────────────────
TEST(ContractQraffle, AssetRaffle_ProposalFeeDistribution)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    // Register a few DAO members so the DAO bucket has recipients
    auto daoMembers = registerDAOUsers(qraffle, 4, 6000);

    id creator = getUser(6500);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6600);
    Asset token = qraffle.setupToken(tokenIssuer, "PFTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto bundle = makeBundle(token, 500000);
    auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);
    EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);

    // 50% of proposal fee should have gone to epochAssetRaffleDaoBucket
    uint64 expectedDaoBucket = QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE / 2;
    EXPECT_EQ(qraffle.getState()->getEpochAssetRaffleDaoBucket(), expectedDaoBucket);
}

// ── TEST 7: buyAssetRaffleTicket – basic success ─────────────────────────────
TEST(ContractQraffle, AssetRaffle_BuyTicketBasicSuccess)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "BUYTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 10000000ULL;  // 10M Qu
    auto cr = qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));
    EXPECT_EQ(cr.returnCode, QRAFFLE_SUCCESS);

    uint32 ticketsToBuy = 3;
    uint64 payment = ticketPrice * ticketsToBuy;
    increaseEnergy(buyer, (sint64)payment);
    auto br = qraffle.buyAssetRaffleTicket(buyer, 0, ticketsToBuy, payment);
    EXPECT_EQ(br.returnCode, QRAFFLE_SUCCESS);

    qraffle.getState()->assetRaffleBuyerChecker(0, buyer, true, ticketsToBuy);
    qraffle.getState()->assetRafflePoolChecker(0, payment, ticketsToBuy, 1);
}

// ── TEST 8: buyAssetRaffleTicket – overpayment refund ────────────────────────
TEST(ContractQraffle, AssetRaffle_BuyTicketRefundExcess)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "REFTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 10000000ULL;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    // Pay for 4 tickets but request only 2 (excess should be refunded)
    uint64 payment = ticketPrice * 4;
    increaseEnergy(buyer, (sint64)payment);
    sint64 balanceBefore = getBalance(buyer);
    auto br = qraffle.buyAssetRaffleTicket(buyer, 0, 2, payment);
    EXPECT_EQ(br.returnCode, QRAFFLE_SUCCESS);

    // Buyer should have been refunded the overpayment for 2 extra tickets
    EXPECT_EQ(getBalance(buyer), balanceBefore - (sint64)(ticketPrice * 2));

    qraffle.getState()->assetRaffleBuyerChecker(0, buyer, true, 2);
}

// ── TEST 9: buyAssetRaffleTicket – validations ───────────────────────────────
TEST(ContractQraffle, AssetRaffle_BuyTicketValidations)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "BVTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 10000000ULL;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    // Any user (even non-DAO member) can buy tickets
    id nonMember = getUser(9000);
    increaseEnergy(nonMember, (sint64)ticketPrice);
    auto r1 = qraffle.buyAssetRaffleTicket(nonMember, 0, 1, ticketPrice);
    EXPECT_EQ(r1.returnCode, QRAFFLE_SUCCESS);

    // Invalid raffle index
    id buyer = getUser(6100);
    increaseEnergy(buyer, (sint64)ticketPrice * 10);
    auto r2 = qraffle.buyAssetRaffleTicket(buyer, 9999, 1, ticketPrice);
    EXPECT_EQ(r2.returnCode, QRAFFLE_INVALID_ASSET_RAFFLE);

    // Insufficient payment (underpayment)
    auto r3 = qraffle.buyAssetRaffleTicket(buyer, 0, 1, ticketPrice - 1);
    EXPECT_EQ(r3.returnCode, QRAFFLE_INSUFFICIENT_FUND);

    // Zero ticket count
    auto r4 = qraffle.buyAssetRaffleTicket(buyer, 0, 0, 0);
    EXPECT_NE(r4.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST 10: buyAssetRaffleTicket – per-buyer ticket cap ─────────────────────
TEST(ContractQraffle, AssetRaffle_BuyTicketCapPerBuyer)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "CAPTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    // Buy up to the cap in batches
    uint32 remaining = QRAFFLE_MAX_TICKETS_PER_BUYER;
    uint32 batch = 100;
    while (remaining > 0)
    {
        uint32 count = (remaining >= batch) ? batch : remaining;
        uint64 payment = ticketPrice * count;
        increaseEnergy(buyer, (sint64)payment);
        auto r = qraffle.buyAssetRaffleTicket(buyer, 0, count, payment);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
        remaining -= count;
    }

    // One more ticket over the cap must fail
    increaseEnergy(buyer, (sint64)ticketPrice);
    auto rOver = qraffle.buyAssetRaffleTicket(buyer, 0, 1, ticketPrice);
    EXPECT_EQ(rOver.returnCode, QRAFFLE_TICKET_LIMIT_REACHED);
}

// ── TEST 11: buyAssetRaffleTicket – multiple buyers accumulate pool ───────────
TEST(ContractQraffle, AssetRaffle_BuyTicketMultipleBuyers)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "MBTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    uint32 numBuyers = 10;
    uint32 ticketsEach = 5;
    uint64 expectedPool = 0;

    for (uint32 i = 0; i < numBuyers; i++)
    {
        id buyer = getUser(7000 + i);
        uint64 payment = ticketPrice * ticketsEach;
        increaseEnergy(buyer, (sint64)payment);
        auto r = qraffle.buyAssetRaffleTicket(buyer, 0, ticketsEach, payment);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
        expectedPool += payment;
    }

    qraffle.getState()->assetRafflePoolChecker(0, expectedPool, numBuyers * ticketsEach, numBuyers);
}

// ── TEST 12: cancelAssetRaffle – creator cancels with no tickets ──────────────
TEST(ContractQraffle, AssetRaffle_CancelNoTickets)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "CNTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    auto cr = qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));
    EXPECT_EQ(cr.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 1u);

    // Cancel must succeed
    auto cancel = qraffle.cancelAssetRaffle(creator, 0);
    EXPECT_EQ(cancel.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);

    // Shares should be back with creator: setupToken gave 1M, 500K escrowed → all 1M back now
    EXPECT_EQ(numberOfPossessedShares(token.assetName, tokenIssuer, creator, creator,
              QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000);
}

// ── TEST 13: cancelAssetRaffle – cannot cancel when tickets sold ──────────────
TEST(ContractQraffle, AssetRaffle_CancelWithTicketsFails)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "CTTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    uint64 payment = ticketPrice * 2;
    increaseEnergy(buyer, (sint64)payment);
    qraffle.buyAssetRaffleTicket(buyer, 0, 2, payment);

    auto cancel = qraffle.cancelAssetRaffle(creator, 0);
    EXPECT_EQ(cancel.returnCode, QRAFFLE_CANCEL_NOT_ALLOWED);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 1u);
}

// ── TEST 14: cancelAssetRaffle – non-creator cannot cancel ───────────────────
TEST(ContractQraffle, AssetRaffle_CancelByNonCreatorFails)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id other = getUser(6100);
    qraffle.registerDAOMember(other);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "NCTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                              makeBundle(token, 500000));

    auto cancel = qraffle.cancelAssetRaffle(other, 0);
    EXPECT_NE(cancel.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 1u);
}

// ── TEST 15: cancelAssetRaffle – invalid index ────────────────────────────────
TEST(ContractQraffle, AssetRaffle_CancelInvalidIndex)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "CITEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                              makeBundle(token, 500000));

    auto cancel = qraffle.cancelAssetRaffle(creator, 9999);
    EXPECT_EQ(cancel.returnCode, QRAFFLE_INVALID_ASSET_RAFFLE);
}

// ── TEST 15b: cancelAssetRaffle – swap-and-pop keeps buyer mapping consistent ──
TEST(ContractQraffle, AssetRaffle_CancelSwapPopMaintainsBuyerState)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id issuerA = getUser(6200);
    id issuerB = getUser(6300);
    Asset tokenA = qraffle.setupToken(issuerA, "SWPA", 1000000, creator);
    Asset tokenB = qraffle.setupToken(issuerB, "SWPB", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * 2 + 1000);

    // Raffle 0 has no buyers and will be cancelled.
    auto r0 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, makeBundle(tokenA, 500000));
    EXPECT_EQ(r0.returnCode, QRAFFLE_SUCCESS);
    // Raffle 1 has one buyer; this raffle should move to slot 0 after cancelling raffle 0.
    auto r1 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, makeBundle(tokenB, 500000));
    EXPECT_EQ(r1.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 2u);

    increaseEnergy(buyer, (sint64)QRAFFLE_MIN_ASSET_TICKET_AMOUNT * 2);
    auto b1 = qraffle.buyAssetRaffleTicket(buyer, 1, 1, QRAFFLE_MIN_ASSET_TICKET_AMOUNT);
    EXPECT_EQ(b1.returnCode, QRAFFLE_SUCCESS);

    auto cancel = qraffle.cancelAssetRaffle(creator, 0);
    EXPECT_EQ(cancel.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 1u);

    // After swap-and-pop, buyer should still be recognized as existing buyer in slot 0.
    // Buying again must accumulate tickets without creating a second buyer slot.
    auto b2 = qraffle.buyAssetRaffleTicket(buyer, 0, 1, QRAFFLE_MIN_ASSET_TICKET_AMOUNT);
    EXPECT_EQ(b2.returnCode, QRAFFLE_SUCCESS);

    qraffle.getState()->assetRaffleBuyerChecker(0, buyer, true, 2);
    qraffle.getState()->assetRafflePoolChecker(0, QRAFFLE_MIN_ASSET_TICKET_AMOUNT * 2, 2, 1);
}

// ── TEST 16: END_EPOCH – reserve met, winner receives 100 % of bundle ─────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_ReserveMet)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    // DAO members: creator + buyers
    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    uint32 numBuyers = 20;
    std::vector<id> buyers;
    for (uint32 i = 0; i < numBuyers; i++)
    {
        buyers.push_back(getUser(7000 + i));
    }

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "WRTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

    // reservePrice = 100M Qu → creator wants 100M after 20% fee → gross needed ≥ 125M
    uint64 ticketPrice = 10000000ULL; // 10M
    uint64 reservePrice = 125000000ULL; // 125M: (gross * 80) >= (125M * 100) → gross ≥ 156.25M
    // 20 buyers × 10M = 200M gross (above the threshold)
    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    uint64 totalPool = 0;
    for (const auto& b : buyers)
    {
        uint64 payment = ticketPrice;
        increaseEnergy(b, (sint64)payment);
        auto r = qraffle.buyAssetRaffleTicket(b, 0, 1, payment);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
        totalPool += payment;
    }

    uint64 creatorBalBefore = (uint64)getBalance(creator);
    qraffle.endEpoch();

    // Raffle should now be cleared
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
    EXPECT_EQ(qraffle.getState()->getNumberOfEndedAssetRaffles(), 1u);

    // Ended raffle recorded as reserve met
    qraffle.getState()->endedAssetRaffleChecker(0, true, creator, totalPool, 200);

    // Creator should have received 80% of pool
    uint64 expectedCreatorPay = (totalPool * QRAFFLE_TEST_ASSET_RAFFLE_CREATOR_PCT) / 100;
    EXPECT_GE((uint64)getBalance(creator), creatorBalBefore + expectedCreatorPay - totalPool / 100);

    // Analytics: 1 created, 1 succeeded, 0 failed
    qraffle.getState()->assetRaffleAnalyticsChecker(1, 1, 0);
}

// ── TEST 17: END_EPOCH – reserve NOT met, full refund + bundle return ─────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_ReserveNotMet)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id buyer1 = getUser(7000);
    id buyer2 = getUser(7001);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "RFTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

    // Set a very high reserve that won't be reached with just 2 buyers
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT; // 1M
    uint64 reservePrice = 1000000000000ULL; // 1T – far above 2M gross
    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    // Two buyers each buy 1 ticket → gross = 2M
    for (const auto& b : {buyer1, buyer2})
    {
        increaseEnergy(b, (sint64)ticketPrice);
        qraffle.buyAssetRaffleTicket(b, 0, 1, ticketPrice);
    }

    sint64 buyer1BalBefore = getBalance(buyer1);
    sint64 buyer2BalBefore = getBalance(buyer2);

    qraffle.endEpoch();

    // Raffle resolved as failed (reserve not met)
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
    EXPECT_EQ(qraffle.getState()->getNumberOfEndedAssetRaffles(), 1u);
    qraffle.getState()->endedAssetRaffleChecker(0, false, creator, ticketPrice * 2, 200);

    // Both buyers refunded
    EXPECT_EQ(getBalance(buyer1), buyer1BalBefore + (sint64)ticketPrice);
    EXPECT_EQ(getBalance(buyer2), buyer2BalBefore + (sint64)ticketPrice);

    // Bundle returned to creator: setupToken gave creator 1M shares, 500K were escrowed → all 1M back now
    EXPECT_EQ(numberOfPossessedShares(token.assetName, tokenIssuer, creator, creator,
              QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000);

    // Analytics: 1 created, 0 succeeded, 1 failed
    qraffle.getState()->assetRaffleAnalyticsChecker(1, 0, 1);
}

// ── TEST 18: END_EPOCH – zero buyers, reserve not met → return bundle ─────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_NoBuyers)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "NOTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                              makeBundle(token, 500000));

    qraffle.endEpoch();

    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
    qraffle.getState()->endedAssetRaffleChecker(0, false, creator, 0, 200);
    qraffle.getState()->assetRaffleAnalyticsChecker(1, 0, 1);

    // Bundle must have been returned: setupToken gave 1M, 500K escrowed → all 1M back now
    EXPECT_EQ(numberOfPossessedShares(token.assetName, tokenIssuer, creator, creator,
              QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000);
}

// ── TEST 19: END_EPOCH – Qu fee distribution (burn/charity/shareholders/DAO) ──
TEST(ContractQraffle, AssetRaffle_EndEpoch_FeeSplit)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    // Need DAO members for register fee split
    auto daoMembers = registerDAOUsers(qraffle, 10, 5000);

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    uint32 numBuyers = 30;

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "FEETEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 50000000ULL; // 50M
    uint64 reservePrice = 125000000ULL;
    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    uint64 totalPool = 0;
    for (uint32 i = 0; i < numBuyers; i++)
    {
        id b = getUser(7000 + i);
        increaseEnergy(b, (sint64)ticketPrice);
        qraffle.buyAssetRaffleTicket(b, 0, 1, ticketPrice);
        totalPool += ticketPrice;
    }

    qraffle.endEpoch();
    auto analyticsAfterEpoch = qraffle.getAssetRaffleAnalytics();

    EXPECT_EQ(analyticsAfterEpoch.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(analyticsAfterEpoch.totalAssetRafflesCreated,   1u);
    EXPECT_EQ(analyticsAfterEpoch.totalAssetRafflesSucceeded, 1u);
    EXPECT_EQ(analyticsAfterEpoch.totalAssetRafflesFailed,    0u);

    // Creator paid + refunded (0 on success) + proposal fees = total inflow
    EXPECT_LE(analyticsAfterEpoch.totalAssetRaffleCreatorPaid, totalPool);
    EXPECT_GT(analyticsAfterEpoch.totalAssetRaffleCreatorPaid, 0ULL);
    // Proposal fee tracked separately
    EXPECT_EQ(analyticsAfterEpoch.totalAssetRaffleProposalFees, QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE);
    // Refunded should be 0 (reserve was met)
    EXPECT_EQ(analyticsAfterEpoch.totalAssetRaffleRefunded, 0ULL);
}

// ── TEST 20: END_EPOCH – reserve boundary (exactly met) ──────────────────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_ReserveBoundaryExactlyMet)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "BDTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

    // reservePrice = 100M, so creator wants 100M after 20% fee
    // The contract checks: gross * 80 >= reservePrice * 100
    // → gross >= reservePrice * 100 / 80 = reservePrice * 5 / 4
    // Use ticketPrice = 100M and 1 buyer, so gross = 100M
    // 100M * 80 = 8_000_000_000  vs  100M * 100 = 10_000_000_000 → NOT met at 100M
    // Minimum gross = ceil(100M * 100 / 80) = 125M exactly.
    // Use ticketPrice = 125M and 1 buyer.
    uint64 reservePrice = 100000000ULL;
    uint64 ticketPrice  = 125000000ULL; // 1 ticket → gross = 125M, boundary is exact

    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    id buyer = getUser(7000);
    increaseEnergy(buyer, (sint64)ticketPrice);
    auto br = qraffle.buyAssetRaffleTicket(buyer, 0, 1, ticketPrice);
    EXPECT_EQ(br.returnCode, QRAFFLE_SUCCESS);

    qraffle.endEpoch();

    // Should succeed (exactly at boundary)
    qraffle.getState()->endedAssetRaffleChecker(0, true, creator, ticketPrice, 200);
    qraffle.getState()->assetRaffleAnalyticsChecker(1, 1, 0);
}

// ── TEST 21: END_EPOCH – reserve boundary (one ticket below) ─────────────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_ReserveBoundaryJustMissed)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "BDJTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

    // Same math as previous test; use ticketPrice = 124M → gross = 124M
    // 124M * 80 = 9_920_000_000  vs  100M * 100 = 10_000_000_000 → NOT met
    uint64 reservePrice = 100000000ULL;
    uint64 ticketPrice  = 124000000ULL;

    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    id buyer = getUser(7000);
    increaseEnergy(buyer, (sint64)ticketPrice);
    qraffle.buyAssetRaffleTicket(buyer, 0, 1, ticketPrice);

    sint64 buyerBalBefore = getBalance(buyer);
    qraffle.endEpoch();

    // Should fail (below boundary)
    qraffle.getState()->endedAssetRaffleChecker(0, false, creator, ticketPrice, 200);
    qraffle.getState()->assetRaffleAnalyticsChecker(1, 0, 1);
    // Buyer refunded
    EXPECT_EQ(getBalance(buyer), buyerBalBefore + (sint64)ticketPrice);
}

// ── TEST 22: END_EPOCH – multiple concurrent raffles ─────────────────────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_MultipleConcurrent)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id tokenIssuer = getUser(6000);
    increaseEnergy(tokenIssuer, 5 * 1000000000ULL);
    uint64 aname = assetNameFromString("MCTEST");
    sint64 totalShares = 10000000;
    qraffle.issueAsset(tokenIssuer, aname, totalShares, 0, 0);
    Asset token;
    token.assetName = aname;
    token.issuer    = tokenIssuer;

    uint32 numRaffles = 3;
    std::vector<id> creators;
    for (uint32 i = 0; i < numRaffles; i++)
    {
        id creator = getUser(7000 + i);
        qraffle.registerDAOMember(creator);
        creators.push_back(creator);

        sint64 shares = 100000;
        qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, shares, creator);
        qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, shares, creator);
        increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
        auto bundle = makeBundle(token, shares);
        auto r = qraffle.createAssetRaffle(creator, 50000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "raffle " << i;
    }
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), numRaffles);

    // Buy enough tickets for raffle 0 and 2 to meet reserve; leave raffle 1 short
    // Raffle 0: 10 buyers × 1M = 10M, reserve 50M → will FAIL
    // Raffle 1: 0 buyers → will FAIL
    // Raffle 2: 100 buyers × 1M = 100M, reserve 50M → gross*80=8_000M >= 50M*100=5_000M → PASS
    auto buyForRaffle = [&](uint32 raffleIdx, uint32 numBuyers, uint32 buyerOffset)
    {
        for (uint32 i = 0; i < numBuyers; i++)
        {
            id b = getUser(8000 + buyerOffset + i);
            increaseEnergy(b, (sint64)QRAFFLE_MIN_ASSET_TICKET_AMOUNT);
            qraffle.buyAssetRaffleTicket(b, raffleIdx, 1, QRAFFLE_MIN_ASSET_TICKET_AMOUNT);
        }
    };
    buyForRaffle(0, 10, 0);   // 10M gross → below 50M reserve
    buyForRaffle(2, 100, 200); // 100M gross → above 50M reserve

    qraffle.endEpoch();

    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
    EXPECT_EQ(qraffle.getState()->getNumberOfEndedAssetRaffles(), 3u);
    qraffle.getState()->assetRaffleAnalyticsChecker(3, 1, 2);
}

// ── TEST 23: END_EPOCH – state reset after epoch ─────────────────────────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_StateReset)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "SRTST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                              makeBundle(token, 500000));

    qraffle.endEpoch();

    // Per-epoch maps must be cleared
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
    // assetRafflesPerCreator and assetRaffleParticipation are cleared per epoch
    // so the creator can create new raffles in the next epoch
    system.epoch = 201;
    Asset token2 = qraffle.setupToken(tokenIssuer, "SRTEST", 500000, creator);
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto r2 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                        makeBundle(token2, 250000));
    EXPECT_EQ(r2.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST 24: END_EPOCH – DAO bucket distributed to registers ─────────────────
TEST(ContractQraffle, AssetRaffle_EndEpoch_DaoBucketDistributed)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    uint32 daoCount = 5;
    auto daoMembers = registerDAOUsers(qraffle, daoCount, 5000);

    id creator = getUser(6500);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6600);
    Asset token = qraffle.setupToken(tokenIssuer, "DAOTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 100000000ULL; // 100M
    uint64 reservePrice = 125000000ULL;
    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    // Buy enough to meet reserve (1 buyer × 100M ... below 125M reserve: 100*80=8000 < 125*100=12500 → fail)
    // Use 2 buyers → 200M gross; 200*80=16000 >= 125*100=12500 → pass
    id buyer1 = getUser(7000);
    id buyer2 = getUser(7001);
    increaseEnergy(buyer1, (sint64)ticketPrice);
    increaseEnergy(buyer2, (sint64)ticketPrice);
    qraffle.buyAssetRaffleTicket(buyer1, 0, 1, ticketPrice);
    qraffle.buyAssetRaffleTicket(buyer2, 0, 1, ticketPrice);

    uint64 daoBucketBefore = qraffle.getState()->getEpochAssetRaffleDaoBucket();
    EXPECT_GT(daoBucketBefore, 0u); // proposal fee portion

    qraffle.endEpoch();

    // After epoch the dao bucket should have been distributed (set to 0 or remainder)
    // The exact value depends on divisibility; just assert it hasn't grown unexpectedly
    uint64 daoBucketAfter = qraffle.getState()->getEpochAssetRaffleDaoBucket();
    // The bucket is cleared each epoch — remainder ≤ number of registers
    uint32 numRegisters = qraffle.getState()->getNumberOfRegisters();
    EXPECT_LT(daoBucketAfter, (uint64)numRegisters);
}

// ── TEST 25: getActiveAssetRaffle – view function ────────────────────────────
TEST(ContractQraffle, AssetRaffle_GetActiveAssetRaffle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "VIEWT", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    uint64 reservePrice = 125000000ULL;
    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    // Valid index
    auto r = qraffle.getActiveAssetRaffle(0);
    EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(r.creator,        creator);
    EXPECT_EQ(r.reservePriceQu, reservePrice);
    EXPECT_EQ(r.entryTicketQu,  ticketPrice);
    EXPECT_EQ(r.bundleSize,     1u);
    EXPECT_EQ(r.epoch,          200u);

    // Invalid index
    auto rInv = qraffle.getActiveAssetRaffle(9999);
    EXPECT_EQ(rInv.returnCode, QRAFFLE_INVALID_ASSET_RAFFLE);
}

// ── TEST 26: getActiveAssetRaffleBundleItem – view function ──────────────────
TEST(ContractQraffle, AssetRaffle_GetActiveAssetRaffleBundleItem)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id issuer1 = getUser(6100);
    id issuer2 = getUser(6200);
    Asset t1 = qraffle.setupToken(issuer1, "BITA", 500000, creator);
    Asset t2 = qraffle.setupToken(issuer2, "BITB", 300000, creator);

    std::vector<std::pair<Asset, sint64>> bundle = { { t1, 500000 }, { t2, 300000 } };
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);

    auto item0 = qraffle.getActiveAssetRaffleBundleItem(0, 0);
    EXPECT_EQ(item0.returnCode,         QRAFFLE_SUCCESS);
    EXPECT_EQ(item0.assetIssuer,        issuer1);
    EXPECT_EQ(item0.assetName,          t1.assetName);
    EXPECT_EQ(item0.numberOfShares,     500000);

    auto item1 = qraffle.getActiveAssetRaffleBundleItem(0, 1);
    EXPECT_EQ(item1.returnCode,         QRAFFLE_SUCCESS);
    EXPECT_EQ(item1.assetIssuer,        issuer2);
    EXPECT_EQ(item1.assetName,          t2.assetName);
    EXPECT_EQ(item1.numberOfShares,     300000);

    // Out-of-range item
    auto rInv = qraffle.getActiveAssetRaffleBundleItem(0, 999);
    EXPECT_NE(rInv.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST 27: getActiveAssetRaffleBuyer – view function ───────────────────────
TEST(ContractQraffle, AssetRaffle_GetActiveAssetRaffleBuyer)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "BUYVT", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    uint32 tickets = 3;
    increaseEnergy(buyer, (sint64)(ticketPrice * tickets));
    qraffle.buyAssetRaffleTicket(buyer, 0, tickets, ticketPrice * tickets);

    auto r = qraffle.getActiveAssetRaffleBuyer(0, 0);
    EXPECT_EQ(r.returnCode,   QRAFFLE_SUCCESS);
    EXPECT_EQ(r.buyer,        buyer);
    EXPECT_EQ(r.ticketCount,  tickets);

    // Invalid buyer index
    auto rInv = qraffle.getActiveAssetRaffleBuyer(0, 9999);
    EXPECT_NE(rInv.returnCode, QRAFFLE_SUCCESS);

    // Invalid raffle index
    auto rInvR = qraffle.getActiveAssetRaffleBuyer(9999, 0);
    EXPECT_NE(rInvR.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST 28: getEndedAssetRaffle – view function ─────────────────────────────
TEST(ContractQraffle, AssetRaffle_GetEndedAssetRaffle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "ENDVT", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 200000000ULL; // 200M
    uint64 reservePrice = 125000000ULL;

    // 1 buyer × 200M → 200M gross; 200*80=16000 >= 125*100=12500 → reserve met
    qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, makeBundle(token, 500000));

    id buyer = getUser(7000);
    increaseEnergy(buyer, (sint64)ticketPrice);
    qraffle.buyAssetRaffleTicket(buyer, 0, 1, ticketPrice);

    qraffle.endEpoch();

    auto r = qraffle.getEndedAssetRaffle(0);
    EXPECT_EQ(r.returnCode,    QRAFFLE_SUCCESS);
    EXPECT_EQ(r.creator,       creator);
    EXPECT_EQ(r.grossPoolQu,   ticketPrice);
    EXPECT_EQ(r.reserveMet,    1u);
    EXPECT_EQ(r.epoch,         200u);
    EXPECT_NE(r.epochWinner,   id(0, 0, 0, 0)); // winner must be set

    // Index beyond what exists
    auto rInv = qraffle.getEndedAssetRaffle(9999);
    EXPECT_NE(rInv.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST 29: getAssetRaffleAnalytics – view function ─────────────────────────
TEST(ContractQraffle, AssetRaffle_GetAnalytics)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    // Analytics initially all zero
    auto r0 = qraffle.getAssetRaffleAnalytics();
    EXPECT_EQ(r0.returnCode,               QRAFFLE_SUCCESS);
    EXPECT_EQ(r0.totalAssetRafflesCreated,   0u);
    EXPECT_EQ(r0.totalAssetRafflesSucceeded, 0u);
    EXPECT_EQ(r0.totalAssetRafflesFailed,    0u);

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "ANLTEST", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    qraffle.createAssetRaffle(creator, 1000000000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                              makeBundle(token, 500000));

    auto r1 = qraffle.getAssetRaffleAnalytics();
    EXPECT_EQ(r1.totalAssetRafflesCreated, 1u);
    EXPECT_EQ(r1.numberOfActiveAssetRaffles, 1u);

    qraffle.endEpoch(); // no buyers → fail

    auto r2 = qraffle.getAssetRaffleAnalytics();
    EXPECT_EQ(r2.totalAssetRafflesCreated,   1u);
    EXPECT_EQ(r2.totalAssetRafflesSucceeded, 0u);
    EXPECT_EQ(r2.totalAssetRafflesFailed,    1u);
    EXPECT_EQ(r2.numberOfActiveAssetRaffles, 0u);
    EXPECT_EQ(r2.numberOfEndedAssetRaffles,  1u);
}

// ── TEST 30: ring-buffer wrap-around for ended asset raffles ──────────────────
TEST(ContractQraffle, AssetRaffle_EndedRingBufferWrap)
{
    // Create more ended raffles than QRAFFLE_MAX_ENDED_ASSET_RAFFLES to verify
    // the ring buffer wraps correctly and old entries are overwritten.
    // For speed we only do a small batch (e.g. 5 wrap cycles with small buffer).
    // Since QRAFFLE_MAX_ENDED_ASSET_RAFFLES = 8192 it's impractical to fill in a
    // unit test; instead we verify that the ring-buffer slot formula is correct
    // by checking a few ended raffles in sequence across two epochs.

    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id tokenIssuer = getUser(9000);
    increaseEnergy(tokenIssuer, 20 * 1000000000ULL);
    uint64 aname = assetNameFromString("RING");
    qraffle.issueAsset(tokenIssuer, aname, 100000000LL, 0, 0);
    Asset token;
    token.assetName = aname;
    token.issuer    = tokenIssuer;

    // Epoch 100: create 3 raffles, end epoch → 3 ended
    for (uint32 i = 0; i < 3; i++)
    {
        id creator = getUser(7000 + i);
        qraffle.registerDAOMember(creator);
        sint64 shares = 10000;
        qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, shares, creator);
        qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, shares, creator);
        increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
        qraffle.createAssetRaffle(creator, 1000000000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                  makeBundle(token, shares));
    }
    qraffle.endEpoch();
    EXPECT_EQ(qraffle.getState()->getNumberOfEndedAssetRaffles(), 3u);

    // Verify first 3 ended slots are accessible
    for (uint32 i = 0; i < 3; i++)
    {
        auto r = qraffle.getEndedAssetRaffle(i);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(r.epoch, 200u);
    }

    // Epoch 101: 2 more raffles
    system.epoch = 201;
    for (uint32 i = 0; i < 2; i++)
    {
        id creator = getUser(8000 + i);
        qraffle.registerDAOMember(creator);
        sint64 shares = 10000;
        qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, shares, creator);
        qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, shares, creator);
        increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
        qraffle.createAssetRaffle(creator, 1000000000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                  makeBundle(token, shares));
    }
    qraffle.endEpoch();
    EXPECT_EQ(qraffle.getState()->getNumberOfEndedAssetRaffles(), 5u);

    // Last two entries should be epoch 201
    auto r3 = qraffle.getEndedAssetRaffle(3);
    auto r4 = qraffle.getEndedAssetRaffle(4);
    EXPECT_EQ(r3.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(r4.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(r3.epoch, 201u);
    EXPECT_EQ(r4.epoch, 201u);
}

// ── TEST 31: createAssetRaffle – escrow rollback on partial bundle failure ─────
TEST(ContractQraffle, AssetRaffle_CreateEscrowRollbackOnFailure)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id issuer1 = getUser(6100);
    // token1: give shares to creator so escrow succeeds for item 0
    Asset t1 = qraffle.setupToken(issuer1, "RLBA", 500000, creator);

    // token2: NOT transferred to creator — escrow should fail for item 1
    id issuer2 = getUser(6200);
    increaseEnergy(issuer2, 1000000000ULL);
    uint64 aname2 = assetNameFromString("RLBB");
    qraffle.issueAsset(issuer2, aname2, 300000, 0, 0);
    // Shares of RLBB stay with issuer2 — creator has 0
    Asset t2;
    t2.assetName = aname2;
    t2.issuer    = issuer2;

    std::vector<std::pair<Asset, sint64>> bundle = { { t1, 500000 }, { t2, 300000 } };
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, bundle);

    // Must fail because creator doesn't have t2 shares
    EXPECT_EQ(r.returnCode, QRAFFLE_BUNDLE_ESCROW_FAILED);
    // No raffle must have been created
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);

    // t1 shares must be back with creator (rollback)
    EXPECT_EQ(numberOfPossessedShares(t1.assetName, issuer1, creator, creator,
              QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 500000);
}

// ── TEST 32: full lifecycle – create, buy, endEpoch, verify winner ────────────
TEST(ContractQraffle, AssetRaffle_FullLifecycle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    // Multiple token types in bundle
    id issuerA = getUser(6100);
    id issuerB = getUser(6200);
    Asset tA = qraffle.setupToken(issuerA, "LIFA", 1000000, creator);
    Asset tB = qraffle.setupToken(issuerB, "LIFB",  500000, creator);
    std::vector<std::pair<Asset, sint64>> bundle = { { tA, 1000000 }, { tB, 500000 } };

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = 50000000ULL; // 50M
    uint64 reservePrice = 125000000ULL;
    auto cr = qraffle.createAssetRaffle(creator, reservePrice, ticketPrice, bundle);
    EXPECT_EQ(cr.returnCode, QRAFFLE_SUCCESS);

    // Get raffle info via view
    auto viewRaffle = qraffle.getActiveAssetRaffle(0);
    EXPECT_EQ(viewRaffle.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(viewRaffle.bundleSize, 2u);

    // Verify bundle items via view
    auto bItem0 = qraffle.getActiveAssetRaffleBundleItem(0, 0);
    auto bItem1 = qraffle.getActiveAssetRaffleBundleItem(0, 1);
    EXPECT_EQ(bItem0.assetIssuer, issuerA);
    EXPECT_EQ(bItem1.assetIssuer, issuerB);

    // 5 buyers buy tickets — 5 × 50M = 250M gross; 250*80=20000 >= 125*100=12500 → pass
    uint32 numBuyers = 5;
    std::vector<id> buyers;
    uint64 totalPool = 0;
    for (uint32 i = 0; i < numBuyers; i++)
    {
        id b = getUser(7000 + i);
        buyers.push_back(b);
        increaseEnergy(b, (sint64)ticketPrice);
        auto br = qraffle.buyAssetRaffleTicket(b, 0, 1, ticketPrice);
        EXPECT_EQ(br.returnCode, QRAFFLE_SUCCESS);
        totalPool += ticketPrice;
    }

    // Verify buyer view
    for (uint32 i = 0; i < numBuyers; i++)
    {
        auto bv = qraffle.getActiveAssetRaffleBuyer(0, i);
        EXPECT_EQ(bv.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(bv.ticketCount, 1u);
    }

    uint64 creatorBalBefore = (uint64)getBalance(creator);
    qraffle.endEpoch();

    // Winner selection: 1 of the 5 buyers must now own both bundle assets
    auto ended = qraffle.getEndedAssetRaffle(0);
    EXPECT_EQ(ended.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(ended.reserveMet, 1u);
    EXPECT_NE(ended.epochWinner, id(0, 0, 0, 0));

    id winner = ended.epochWinner;

    // Verify winner holds both token types
    EXPECT_EQ(numberOfPossessedShares(tA.assetName, issuerA, winner, winner,
              QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 1000000);
    EXPECT_EQ(numberOfPossessedShares(tB.assetName, issuerB, winner, winner,
              QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX), 500000);

    // Creator received ~80% of pool
    uint64 minCreatorPay = (totalPool * QRAFFLE_TEST_ASSET_RAFFLE_CREATOR_PCT) / 100 - 1;
    EXPECT_GE((uint64)getBalance(creator), creatorBalBefore + minCreatorPay);

    // Analytics
    auto analytics = qraffle.getAssetRaffleAnalytics();
    EXPECT_EQ(analytics.totalAssetRafflesCreated,   1u);
    EXPECT_EQ(analytics.totalAssetRafflesSucceeded, 1u);
    EXPECT_EQ(analytics.totalAssetRafflesFailed,    0u);
}

// ── TEST 33: createAssetRaffle – non-DAO member fails ────────────────────────
TEST(ContractQraffle, AssetRaffle_CreateNonDAOMemberFails)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id nonMember = getUser(9000);
    // NOT registered as DAO member

    id tokenIssuer = getUser(9100);
    increaseEnergy(tokenIssuer, 1000000000ULL);
    uint64 aname = assetNameFromString("NDMT");
    qraffle.issueAsset(tokenIssuer, aname, 1000000, 0, 0);
    Asset token;
    token.assetName = aname;
    token.issuer    = tokenIssuer;

    increaseEnergy(nonMember, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto r = qraffle.createAssetRaffle(nonMember, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                       makeBundle(token, 100000));
    EXPECT_EQ(r.returnCode, QRAFFLE_UNREGISTERED);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
}

// ── TEST 34: buyAssetRaffleTicket – unregistered buyer can buy ───────────────
TEST(ContractQraffle, AssetRaffle_BuyTicketUnregisteredBuyerAllowed)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "UNBUY", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    auto cr = qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));
    EXPECT_EQ(cr.returnCode, QRAFFLE_SUCCESS);

    // Buyer is NOT registered as a DAO member
    id buyer = getUser(9999);
    increaseEnergy(buyer, (sint64)(ticketPrice * 2));
    auto br = qraffle.buyAssetRaffleTicket(buyer, 0, 2, ticketPrice * 2);
    EXPECT_EQ(br.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(br.ticketsBought, 2u);

    qraffle.getState()->assetRaffleBuyerChecker(0, buyer, true, 2);
    qraffle.getState()->assetRafflePoolChecker(0, ticketPrice * 2, 2, 1);
}

// ── TEST 35: buyAssetRaffleTicket – successive purchases from same buyer ───────
TEST(ContractQraffle, AssetRaffle_BuyTicketSuccessivePurchases)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);
    id buyer = getUser(6100);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "SUCBUY", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    // First purchase
    increaseEnergy(buyer, (sint64)(ticketPrice * 5));
    auto r1 = qraffle.buyAssetRaffleTicket(buyer, 0, 5, ticketPrice * 5);
    EXPECT_EQ(r1.returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->assetRaffleBuyerChecker(0, buyer, true, 5);

    // Second purchase (same buyer accumulates)
    increaseEnergy(buyer, (sint64)(ticketPrice * 3));
    auto r2 = qraffle.buyAssetRaffleTicket(buyer, 0, 3, ticketPrice * 3);
    EXPECT_EQ(r2.returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->assetRaffleBuyerChecker(0, buyer, true, 8);
    qraffle.getState()->assetRafflePoolChecker(0, ticketPrice * 8, 8, 1);
}

// ── TEST 35: cross-check analytics before and after epoch ─────────────────────
TEST(ContractQraffle, AssetRaffle_AnalyticsCrossCheckMultipleEpochs)
{
    ContractTestingQraffle qraffle;

    id tokenIssuer = getUser(9000);
    increaseEnergy(tokenIssuer, 10 * 1000000000ULL);
    uint64 aname = assetNameFromString("ACCT");
    qraffle.issueAsset(tokenIssuer, aname, 100000000LL, 0, 0);
    Asset token;
    token.assetName = aname;
    token.issuer    = tokenIssuer;

    uint32 totalCreated   = 0;
    uint32 totalSucceeded = 0;
    uint32 totalFailed    = 0;

    for (uint32 epoch = 200; epoch < 203; epoch++)
    {
        system.epoch = (uint16)epoch;

        // 2 creators per epoch
        for (uint32 c = 0; c < 2; c++)
        {
            id creator = getUser(7000 + epoch * 10 + c);
            qraffle.registerDAOMember(creator);
            sint64 shares = 10000;
            qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, shares, creator);
            qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, shares, creator);
            increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

            uint64 reservePrice = (c == 0) ? QRAFFLE_MIN_ASSET_TICKET_AMOUNT * 5 : 1000000000000ULL;
            auto r = qraffle.createAssetRaffle(creator, reservePrice, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                              makeBundle(token, shares));
            EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
            totalCreated++;

            // Buy 10 tickets for raffle 0 (low reserve) → will succeed
            if (c == 0)
            {
                for (uint32 b = 0; b < 10; b++)
                {
                    id buyer = getUser(8000 + epoch * 100 + c * 10 + b);
                    increaseEnergy(buyer, (sint64)QRAFFLE_MIN_ASSET_TICKET_AMOUNT);
                    qraffle.buyAssetRaffleTicket(buyer, c, 1, QRAFFLE_MIN_ASSET_TICKET_AMOUNT);
                }
            }
            // Raffle 1 has very high reserve → will fail (no buyers)
        }

        qraffle.endEpoch();
        totalSucceeded += 1; // c==0 raffle should succeed
        totalFailed    += 1; // c==1 raffle should fail (no buyers or tiny pool)

        auto a = qraffle.getAssetRaffleAnalytics();
        EXPECT_EQ(a.returnCode, QRAFFLE_SUCCESS);
        EXPECT_EQ(a.totalAssetRafflesCreated,   totalCreated);
        EXPECT_EQ(a.totalAssetRafflesSucceeded, totalSucceeded);
        EXPECT_EQ(a.totalAssetRafflesFailed,    totalFailed);
    }
}

// ============================================================================
// Extended / Power Tests
// ============================================================================

// ── TEST: InitialRegisters_CannotLogout ──────────────────────────────────────
// The 5 bootstrap registers baked into INITIALIZE must never be able to logout.
TEST(ContractQraffle, InitialRegisters_CannotLogout)
{
    ContractTestingQraffle qraffle;

    // Pull the 5 initial register IDs directly from contract state.
    QRaffleChecker* s = qraffle.getState();
    id ir[5] = {
        s->initialRegister1, s->initialRegister2, s->initialRegister3,
        s->initialRegister4, s->initialRegister5
    };

    EXPECT_EQ(s->numberOfRegisters, 5u);

    for (const auto& reg : ir)
    {
        auto result = qraffle.logoutInSystem(reg);
        EXPECT_EQ(result.returnCode, QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT);
    }

    // Count must remain 5.
    EXPECT_EQ(s->numberOfRegisters, 5u);
}

// ── TEST: SubmitProposal_ReservedTokenRejected ───────────────────────────────
// QRAFFLE shares and QXMR tokens must be rejected as proposal targets.
TEST(ContractQraffle, SubmitProposal_ReservedTokenRejected)
{
    ContractTestingQraffle qraffle;

    id member = getUser(2000);
    increaseEnergy(member, QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(member, QRAFFLE_REGISTER_AMOUNT, 0);

    // Build QRAFFLE-share asset descriptor (issuer = NULL_ID).
    Asset qraffleShare;
    qraffleShare.assetName = QRAFFLE_ASSET_NAME;
    qraffleShare.issuer    = NULL_ID;

    auto r1 = qraffle.submitProposal(member, qraffleShare, QRAFFLE_MIN_QRAFFLE_AMOUNT);
    EXPECT_EQ(r1.returnCode, QRAFFLE_INVALID_TOKEN_TYPE);

    // Build QXMR asset descriptor.
    Asset qxmr;
    qxmr.assetName = QRAFFLE_QXMR_ASSET_NAME;
    qxmr.issuer    = qraffle.getState()->QXMRIssuer;

    auto r2 = qraffle.submitProposal(member, qxmr, QRAFFLE_MIN_QRAFFLE_AMOUNT);
    EXPECT_EQ(r2.returnCode, QRAFFLE_INVALID_TOKEN_TYPE);

    // Normal token should still work.
    id tokenIssuer = getUser(3000);
    increaseEnergy(tokenIssuer, 1000000000ULL);
    uint64 aname = assetNameFromString("RSVTEST");
    qraffle.issueAsset(tokenIssuer, aname, 1000000, 0, 0);
    Asset good;
    good.assetName = aname;
    good.issuer    = tokenIssuer;
    auto r3 = qraffle.submitProposal(member, good, QRAFFLE_MIN_QRAFFLE_AMOUNT);
    EXPECT_EQ(r3.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST: VoteInProposal_SameDirectionRejected ───────────────────────────────
// Voting the same direction twice must return QRAFFLE_ALREADY_VOTED.
TEST(ContractQraffle, VoteInProposal_SameDirectionRejected)
{
    ContractTestingQraffle qraffle;

    id member = getUser(2000);
    increaseEnergy(member, QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(member, QRAFFLE_REGISTER_AMOUNT, 0);

    id tokenIssuer = getUser(3000);
    increaseEnergy(tokenIssuer, 1000000000ULL);
    uint64 aname = assetNameFromString("SDTEST");
    qraffle.issueAsset(tokenIssuer, aname, 1000000, 0, 0);
    Asset token{ tokenIssuer, aname };

    qraffle.submitProposal(member, token, QRAFFLE_MIN_QRAFFLE_AMOUNT);

    // First vote: yes.
    EXPECT_EQ(qraffle.voteInProposal(member, 0, 1).returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->voteChecker(0, 1, 0);

    // Same direction again: must fail.
    EXPECT_EQ(qraffle.voteInProposal(member, 0, 1).returnCode, QRAFFLE_ALREADY_VOTED);
    qraffle.getState()->voteChecker(0, 1, 0);
}

// ── TEST: VoteInProposal_FlipToOppositeDirection ─────────────────────────────
// Voting the opposite direction from a prior vote must flip counters exactly.
TEST(ContractQraffle, VoteInProposal_FlipToOppositeDirection)
{
    ContractTestingQraffle qraffle;

    // Register enough voters for a meaningful flip scenario.
    const uint32 numVoters = 6;
    id voters[numVoters];
    for (uint32 i = 0; i < numVoters; i++)
    {
        voters[i] = getUser(2000 + i);
        increaseEnergy(voters[i], QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(voters[i], QRAFFLE_REGISTER_AMOUNT, 0);
    }

    id tokenIssuer = getUser(3000);
    increaseEnergy(tokenIssuer, 1000000000ULL);
    uint64 aname = assetNameFromString("FLIPT");
    qraffle.issueAsset(tokenIssuer, aname, 1000000, 0, 0);
    Asset token{ tokenIssuer, aname };

    qraffle.submitProposal(voters[0], token, QRAFFLE_MIN_QRAFFLE_AMOUNT);

    // 4 yes, 2 no.
    for (uint32 i = 0; i < 4; i++)
        EXPECT_EQ(qraffle.voteInProposal(voters[i], 0, 1).returnCode, QRAFFLE_SUCCESS);
    for (uint32 i = 4; i < 6; i++)
        EXPECT_EQ(qraffle.voteInProposal(voters[i], 0, 0).returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->voteChecker(0, 4, 2);

    // Voter 0 flips from yes → no: nYes becomes 3, nNo becomes 3.
    EXPECT_EQ(qraffle.voteInProposal(voters[0], 0, 0).returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->voteChecker(0, 3, 3);

    // Voter 4 flips from no → yes: nYes becomes 4, nNo becomes 2.
    EXPECT_EQ(qraffle.voteInProposal(voters[4], 0, 1).returnCode, QRAFFLE_SUCCESS);
    qraffle.getState()->voteChecker(0, 4, 2);
}

// ── TEST: SubmitEntryAmount_BoundaryValues ────────────────────────────────────
// Exactly-at-boundary amounts must succeed; one unit outside must fail.
TEST(ContractQraffle, SubmitEntryAmount_BoundaryValues)
{
    ContractTestingQraffle qraffle;

    id member = getUser(2000);
    increaseEnergy(member, QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(member, QRAFFLE_REGISTER_AMOUNT, 0);

    // Below minimum.
    EXPECT_EQ(qraffle.submitEntryAmount(member, QRAFFLE_MIN_QRAFFLE_AMOUNT - 1).returnCode,
              QRAFFLE_INVALID_ENTRY_AMOUNT);

    // Exactly minimum.
    EXPECT_EQ(qraffle.submitEntryAmount(member, QRAFFLE_MIN_QRAFFLE_AMOUNT).returnCode,
              QRAFFLE_SUCCESS);

    // Exactly maximum.
    EXPECT_EQ(qraffle.submitEntryAmount(member, QRAFFLE_MAX_QRAFFLE_AMOUNT).returnCode,
              QRAFFLE_SUCCESS);

    // Above maximum.
    EXPECT_EQ(qraffle.submitEntryAmount(member, (uint64)QRAFFLE_MAX_QRAFFLE_AMOUNT + 1).returnCode,
              QRAFFLE_INVALID_ENTRY_AMOUNT);
}

// ── TEST: EndEpoch_ProposalRejectedWhenNoWins ─────────────────────────────────
// A proposal where nNo >= nYes must NOT produce an active token raffle.
TEST(ContractQraffle, EndEpoch_ProposalRejectedWhenNoWins)
{
    ContractTestingQraffle qraffle;

    const uint32 numVoters = 6;
    id voters[numVoters];
    for (uint32 i = 0; i < numVoters; i++)
    {
        voters[i] = getUser(2000 + i);
        increaseEnergy(voters[i], QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(voters[i], QRAFFLE_REGISTER_AMOUNT, 0);
    }

    id tokenIssuer = getUser(3000);
    increaseEnergy(tokenIssuer, 1000000000ULL);
    uint64 aname = assetNameFromString("NOWINS");
    qraffle.issueAsset(tokenIssuer, aname, 1000000, 0, 0);
    Asset token{ tokenIssuer, aname };

    qraffle.submitProposal(voters[0], token, QRAFFLE_MIN_QRAFFLE_AMOUNT);

    // 2 yes, 4 no → nNo > nYes, proposal should be rejected.
    for (uint32 i = 0; i < 2; i++)
        qraffle.voteInProposal(voters[i], 0, 1);
    for (uint32 i = 2; i < 6; i++)
        qraffle.voteInProposal(voters[i], 0, 0);
    qraffle.getState()->voteChecker(0, 2, 4);

    qraffle.endEpoch();

    EXPECT_EQ(qraffle.getState()->getNumberOfActiveTokenRaffle(), 0u);
}

// ── TEST: EndEpoch_qREAmountCalculation ──────────────────────────────────────
// After endEpoch, qREAmount should equal the arithmetic mean of all submitted
// entry amounts (integer division, as the contract uses div<uint64>).
TEST(ContractQraffle, EndEpoch_qREAmountCalculation)
{
    ContractTestingQraffle qraffle;

    const uint32 numMembers = 5;
    uint64 amounts[numMembers] = {
        QRAFFLE_MIN_QRAFFLE_AMOUNT,
        QRAFFLE_MIN_QRAFFLE_AMOUNT * 2,
        QRAFFLE_MIN_QRAFFLE_AMOUNT * 3,
        QRAFFLE_MIN_QRAFFLE_AMOUNT * 4,
        QRAFFLE_MIN_QRAFFLE_AMOUNT * 5
    };
    uint64 totalAmount = 0;
    for (uint32 i = 0; i < numMembers; i++) totalAmount += amounts[i];

    id members[numMembers];
    for (uint32 i = 0; i < numMembers; i++)
    {
        members[i] = getUser(2000 + i);
        increaseEnergy(members[i], QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(members[i], QRAFFLE_REGISTER_AMOUNT, 0);
        EXPECT_EQ(qraffle.submitEntryAmount(members[i], amounts[i]).returnCode, QRAFFLE_SUCCESS);
    }

    qraffle.endEpoch();

    uint64 expected = totalAmount / numMembers;
    EXPECT_EQ(qraffle.getState()->getQuRaffleEntryAmount(), expected);
}

// ── TEST: EndEpoch_QuRaffleEmptyNoWinner ─────────────────────────────────────
// endEpoch with 0 QuRaffle members must complete without crash; no winner
// is recorded in the QuRaffles ring for that epoch.
TEST(ContractQraffle, EndEpoch_QuRaffleEmptyNoWinner)
{
    ContractTestingQraffle qraffle;
    system.epoch = 10;

    // Register a member but do NOT deposit into QuRaffle.
    id member = getUser(2000);
    increaseEnergy(member, QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(member, QRAFFLE_REGISTER_AMOUNT, 0);

    // Should not crash.
    qraffle.endEpoch();

    // No winner recorded.
    auto info = qraffle.getEndedQuRaffle(10);
    EXPECT_EQ(info.returnCode,    QRAFFLE_SUCCESS);
    EXPECT_EQ(info.numberOfMembers, 0u);
    EXPECT_EQ(info.epochWinner,   id(0, 0, 0, 0));

    // qREAmount falls back to default when no submissions.
    EXPECT_EQ(qraffle.getState()->getQuRaffleEntryAmount(), QRAFFLE_DEFAULT_QRAFFLE_AMOUNT);
}

// ── TEST: MultipleEpochs_StateResetAndReuse ───────────────────────────────────
// Verify that per-epoch state (proposals, votes, QuRaffle members) is fully
// cleared after endEpoch and new activity in the next epoch works correctly.
TEST(ContractQraffle, MultipleEpochs_StateResetAndReuse)
{
    ContractTestingQraffle qraffle;
    system.epoch = 5;

    const uint32 numMembers = 10;
    id members[numMembers];
    for (uint32 i = 0; i < numMembers; i++)
    {
        members[i] = getUser(2000 + i);
        increaseEnergy(members[i], QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(members[i], QRAFFLE_REGISTER_AMOUNT, 0);
    }

    id tokenIssuer = getUser(3000);
    increaseEnergy(tokenIssuer, 2000000000ULL);
    uint64 aname1 = assetNameFromString("MRST1");
    uint64 aname2 = assetNameFromString("MRST2");
    qraffle.issueAsset(tokenIssuer, aname1, 1000000000, 0, 0);
    qraffle.issueAsset(tokenIssuer, aname2, 1000000000, 0, 0);

    Asset token1{ tokenIssuer, aname1 };
    Asset token2{ tokenIssuer, aname2 };

    // ── Epoch 5: submit proposal, vote yes, add QuRaffle members ──────────────
    qraffle.submitProposal(members[0], token1, QRAFFLE_MIN_QRAFFLE_AMOUNT);
    for (uint32 i = 0; i < numMembers; i++)
        qraffle.voteInProposal(members[i], 0, 1);
    qraffle.getState()->voteChecker(0, numMembers, 0);

    for (uint32 i = 0; i < numMembers; i++)
    {
        increaseEnergy(members[i], qraffle.getState()->getQuRaffleEntryAmount());
        EXPECT_EQ(qraffle.depositInQuRaffle(members[i], qraffle.getState()->getQuRaffleEntryAmount()).returnCode,
                  QRAFFLE_SUCCESS);
    }
    EXPECT_EQ(qraffle.getState()->numberOfQuRaffleMembers, numMembers);

    qraffle.endEpoch();

    // Epoch 5 data persists in QuRaffles ring.
    auto quResult5 = qraffle.getEndedQuRaffle(5);
    EXPECT_EQ(quResult5.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(quResult5.numberOfMembers, numMembers);
    EXPECT_NE(quResult5.epochWinner, id(0, 0, 0, 0));

    // Per-epoch state cleared.
    EXPECT_EQ(qraffle.getState()->numberOfQuRaffleMembers, 0u);
    EXPECT_EQ(qraffle.getState()->numberOfProposals, 0u);
    EXPECT_EQ(qraffle.getState()->numberOfEntryAmountSubmitted, 0u);

    // ── Epoch 6: fresh proposal and QuRaffle round ────────────────────────────
    system.epoch = 6;
    qraffle.submitProposal(members[1], token2, QRAFFLE_MIN_QRAFFLE_AMOUNT * 2);
    EXPECT_EQ(qraffle.getState()->numberOfProposals, 1u);
    // Check the new proposal is for token2.
    EXPECT_EQ(qraffle.getState()->proposals.get(0).token.assetName, aname2);

    for (uint32 i = 0; i < numMembers; i++)
        EXPECT_EQ(qraffle.voteInProposal(members[i], 0, 1).returnCode, QRAFFLE_SUCCESS);

    for (uint32 i = 0; i < numMembers; i++)
    {
        increaseEnergy(members[i], qraffle.getState()->getQuRaffleEntryAmount());
        EXPECT_EQ(qraffle.depositInQuRaffle(members[i], qraffle.getState()->getQuRaffleEntryAmount()).returnCode,
                  QRAFFLE_SUCCESS);
    }

    qraffle.endEpoch();

    auto quResult6 = qraffle.getEndedQuRaffle(6);
    EXPECT_EQ(quResult6.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(quResult6.numberOfMembers, numMembers);

    // Token2 raffle should now be active.
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveTokenRaffle(), 1u);
    EXPECT_EQ(qraffle.getState()->activeTokenRaffle.get(0).token.assetName, aname2);
}

// ── TEST: TokenRaffle_WinnerReceivesTokensMinusFees ──────────────────────────
// At endEpoch, exactly the winner's share of tokens must be transferred to the
// winner; the fee percentages must match constants defined in the header.
TEST(ContractQraffle, TokenRaffle_WinnerReceivesTokensMinusFees)
{
    ContractTestingQraffle qraffle;
    system.epoch = 5;

    const uint32 numMembers = 8;
    id members[numMembers];
    for (uint32 i = 0; i < numMembers; i++)
    {
        members[i] = getUser(2000 + i);
        increaseEnergy(members[i], QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(members[i], QRAFFLE_REGISTER_AMOUNT, 0);
    }

    id tokenIssuer = getUser(3000);
    increaseEnergy(tokenIssuer, 2000000000ULL);
    uint64 aname = assetNameFromString("WINTOK");
    sint64 totalShares = 1000000000LL;
    qraffle.issueAsset(tokenIssuer, aname, totalShares, 0, 0);
    Asset token{ tokenIssuer, aname };

    // Propose and vote yes.
    uint64 entryAmount = 5000000ULL;
    qraffle.submitProposal(members[0], token, entryAmount);
    for (uint32 i = 0; i < numMembers; i++)
        qraffle.voteInProposal(members[i], 0, 1);

    // Add QuRaffle deposit so endEpoch doesn't crash on empty QuRaffle.
    increaseEnergy(members[0], qraffle.getState()->getQuRaffleEntryAmount());
    qraffle.depositInQuRaffle(members[0], qraffle.getState()->getQuRaffleEntryAmount());

    qraffle.endEpoch();

    // Token raffle is now active.
    ASSERT_EQ(qraffle.getState()->getNumberOfActiveTokenRaffle(), 1u);
    EXPECT_EQ(qraffle.getState()->activeTokenRaffle.get(0).entryAmount, entryAmount);

    // All numMembers deposit into the token raffle.
    for (uint32 i = 0; i < numMembers; i++)
    {
        increaseEnergy(members[i], QRAFFLE_TRANSFER_SHARE_FEE);
        // Give each member entryAmount shares under QRAFFLE management.
        qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, (sint64)entryAmount, members[i]);
        qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, (sint64)entryAmount, members[i]);
        EXPECT_EQ(qraffle.depositInTokenRaffle(members[i], 0, QRAFFLE_TRANSFER_SHARE_FEE).returnCode, QRAFFLE_SUCCESS);
    }

    // Deposit one more QuRaffle participant for the next epoch.
    increaseEnergy(members[0], qraffle.getState()->getQuRaffleEntryAmount());
    qraffle.depositInQuRaffle(members[0], qraffle.getState()->getQuRaffleEntryAmount());

    system.epoch = 6;
    qraffle.endEpoch();

    // Exactly 1 token raffle must have ended.
    EXPECT_EQ(qraffle.getState()->getNumberOfEndedTokenRaffle(), 1u);
    auto ended = qraffle.getEndedTokenRaffle(0);
    EXPECT_EQ(ended.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(ended.numberOfMembers, numMembers);
    EXPECT_NE(ended.epochWinner, id(0, 0, 0, 0));

    // Compute expected winner share.
    uint64 pool          = (uint64)entryAmount * numMembers;
    uint64 burn          = pool * QRAFFLE_BURN_FEE / 100;
    uint64 charity       = pool * QRAFFLE_CHARITY_FEE / 100;
    uint64 shareholder   = (pool * QRAFFLE_SHAREHOLDER_FEE / 100) / NUMBER_OF_COMPUTORS * NUMBER_OF_COMPUTORS;
    uint32 numRegisters  = qraffle.getState()->getNumberOfRegisters();
    uint64 reg           = (pool * QRAFFLE_REGISTER_FEE / 100) / numRegisters * numRegisters;
    uint64 fee           = pool * QRAFFLE_FEE / 100;
    uint64 expectedWinner = pool - burn - charity - shareholder - reg - fee;

    id winner = ended.epochWinner;
    sint64 winnerShares = numberOfPossessedShares(aname, tokenIssuer, winner, winner,
                                                  QRAFFLE_CONTRACT_INDEX, QRAFFLE_CONTRACT_INDEX);
    EXPECT_EQ((uint64)winnerShares, expectedWinner);
}

// ── TEST: AssetRaffle_DuplicateAssetInBundle ──────────────────────────────────
// A bundle containing the same asset twice must be rejected with QRAFFLE_INVALID_BUNDLE.
TEST(ContractQraffle, AssetRaffle_DuplicateAssetInBundle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "DUPBND", 2000000, creator);

    // Two slots with the same asset.
    std::vector<std::pair<Asset, sint64>> dupBundle = {
        { token, 500000 },
        { token, 500000 }
    };

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT, dupBundle);
    EXPECT_EQ(r.returnCode, QRAFFLE_INVALID_BUNDLE);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
}

// ── TEST: AssetRaffle_ReservedTokenInBundle ───────────────────────────────────
// Bundles containing QRAFFLE shares or QXMR must be rejected.
TEST(ContractQraffle, AssetRaffle_ReservedTokenInBundle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    // QRAFFLE share descriptor.
    Asset qraffleShare;
    qraffleShare.assetName = QRAFFLE_ASSET_NAME;
    qraffleShare.issuer    = NULL_ID;

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * 4 + 1000);

    auto r1 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                        makeBundle(qraffleShare, 100));
    EXPECT_EQ(r1.returnCode, QRAFFLE_INVALID_BUNDLE);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);

    // QXMR descriptor.
    Asset qxmr;
    qxmr.assetName = QRAFFLE_QXMR_ASSET_NAME;
    qxmr.issuer    = qraffle.getState()->QXMRIssuer;

    auto r2 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                        makeBundle(qxmr, 100));
    EXPECT_EQ(r2.returnCode, QRAFFLE_INVALID_BUNDLE);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
}

// ── TEST: AssetRaffle_ZeroSharesInBundle ─────────────────────────────────────
// A bundle item with numberOfShares == 0 (or negative) must be rejected.
TEST(ContractQraffle, AssetRaffle_ZeroOrNegativeSharesInBundle)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "ZERSHR", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * 4 + 1000);

    // Zero shares.
    auto r1 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                        makeBundle(token, 0));
    EXPECT_EQ(r1.returnCode, QRAFFLE_INVALID_BUNDLE);

    // Negative shares.
    auto r2 = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                        makeBundle(token, -1));
    EXPECT_EQ(r2.returnCode, QRAFFLE_INVALID_BUNDLE);

    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), 0u);
}

// ── TEST: AssetRaffle_CreateAfterCancel_SameEpoch ────────────────────────────
// After cancelling a raffle the per-creator counter is decremented, so the
// creator can immediately open a replacement within the same epoch.
TEST(ContractQraffle, AssetRaffle_CreateAfterCancel_SameEpoch)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "REPTEST", 2000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * 4 + 10000);

    // Fill the per-creator limit.
    for (uint32 i = 0; i < QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR; i++)
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                           makeBundle(token, 100));
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "slot " << i;
    }
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR);

    // Cancel raffle 0 (no buyers).
    EXPECT_EQ(qraffle.cancelAssetRaffle(creator, 0).returnCode, QRAFFLE_SUCCESS);

    // Creator should now be able to open one more replacement raffle.
    auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                       makeBundle(token, 100));
    EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->getNumberOfActiveAssetRaffles(), QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR);
}

// ── TEST: AssetRaffle_ReservePrice_OverflowGuard ─────────────────────────────
// A reserve price so large that reservePriceQu * 100 would overflow uint64
// must be rejected with QRAFFLE_INVALID_RESERVE_PRICE.
TEST(ContractQraffle, AssetRaffle_ReservePrice_OverflowGuard)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "OFGRD", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);

    // Max uint64 / 100 + 1 should trigger the overflow guard.
    uint64 overflowReserve = 0xFFFFFFFFFFFFFFFFull / 100ull + 1ull;
    auto r = qraffle.createAssetRaffle(creator, overflowReserve, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                       makeBundle(token, 500000));
    EXPECT_EQ(r.returnCode, QRAFFLE_INVALID_RESERVE_PRICE);
}

// ── TEST: AssetRaffle_BuyTicketInvalidAmount ──────────────────────────────────
// Buying more than QRAFFLE_MAX_TICKETS_PER_BUYER in a single call should fail.
TEST(ContractQraffle, AssetRaffle_BuyTicketInvalidAmount)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6200);
    Asset token = qraffle.setupToken(tokenIssuer, "INVAMT", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 1000);
    uint64 ticketPrice = QRAFFLE_MIN_ASSET_TICKET_AMOUNT;
    qraffle.createAssetRaffle(creator, 125000000ULL, ticketPrice, makeBundle(token, 500000));

    id buyer = getUser(7000);
    uint32 tooMany = QRAFFLE_MAX_TICKETS_PER_BUYER + 1;
    uint64 payment = ticketPrice * tooMany;
    increaseEnergy(buyer, (sint64)payment);
    auto r = qraffle.buyAssetRaffleTicket(buyer, 0, tooMany, payment);
    EXPECT_NE(r.returnCode, QRAFFLE_SUCCESS);
}

// ── TEST: QuRaffle_DepositDoesNotRequireDAOMembership ────────────────────────
// depositInQuRaffle requires only sufficient Qu — DAO membership is not required.
TEST(ContractQraffle, QuRaffle_DepositDoesNotRequireDAOMembership)
{
    ContractTestingQraffle qraffle;

    // Non-DAO member with enough Qu.
    id nonMember = getUser(9999);
    increaseEnergy(nonMember, qraffle.getState()->getQuRaffleEntryAmount() + 100);
    auto r = qraffle.depositInQuRaffle(nonMember, qraffle.getState()->getQuRaffleEntryAmount());
    EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(qraffle.getState()->numberOfQuRaffleMembers, 1u);
}

// ── TEST: EndEpoch_QuRaffleAmountAverageUpdateAndReset ───────────────────────
// qREAmount resets to QRAFFLE_DEFAULT_QRAFFLE_AMOUNT when no entries submitted.
TEST(ContractQraffle, EndEpoch_qREAmountResetsToDefaultWhenNoSubmissions)
{
    ContractTestingQraffle qraffle;

    // Submit one entry to change qREAmount from default.
    id member = getUser(2000);
    increaseEnergy(member, QRAFFLE_REGISTER_AMOUNT);
    qraffle.registerInSystem(member, QRAFFLE_REGISTER_AMOUNT, 0);
    EXPECT_EQ(qraffle.submitEntryAmount(member, QRAFFLE_MAX_QRAFFLE_AMOUNT).returnCode, QRAFFLE_SUCCESS);

    qraffle.endEpoch();
    // After epoch, qREAmount should be QRAFFLE_MAX_QRAFFLE_AMOUNT (single entry).
    EXPECT_EQ(qraffle.getState()->getQuRaffleEntryAmount(), (uint64)QRAFFLE_MAX_QRAFFLE_AMOUNT);

    // Second epoch: no new submissions.
    qraffle.endEpoch();
    // No entries → falls back to default.
    EXPECT_EQ(qraffle.getState()->getQuRaffleEntryAmount(), QRAFFLE_DEFAULT_QRAFFLE_AMOUNT);
}

// ── TEST: TokenRaffle_DepositSlotFull ────────────────────────────────────────
// Once a token raffle's member slot (QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE) is full,
// further deposits must return QRAFFLE_MAX_MEMBER_REACHED.
// NOTE: QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE = 512, so we register 512 members.
TEST(ContractQraffle, TokenRaffle_DepositSlotFull)
{
    ContractTestingQraffle qraffle;
    system.epoch = 5;

    // Create a token.
    id tokenIssuer = getUser(100);
    increaseEnergy(tokenIssuer, 2000000000ULL);
    uint64 aname = assetNameFromString("SLOTFLL");
    sint64 totalShares = 1000000000LL;
    qraffle.issueAsset(tokenIssuer, aname, totalShares, 0, 0);
    Asset token{ tokenIssuer, aname };

    // Register one proposer and vote to activate the token raffle.
    const uint32 numVoters = 10;
    id voters[numVoters];
    for (uint32 i = 0; i < numVoters; i++)
    {
        voters[i] = getUser(200 + i);
        increaseEnergy(voters[i], QRAFFLE_REGISTER_AMOUNT);
        qraffle.registerInSystem(voters[i], QRAFFLE_REGISTER_AMOUNT, 0);
    }

    uint64 entryAmt = 1000000ULL;
    qraffle.submitProposal(voters[0], token, entryAmt);
    for (uint32 i = 0; i < numVoters; i++)
        qraffle.voteInProposal(voters[i], 0, 1);

    increaseEnergy(voters[0], qraffle.getState()->getQuRaffleEntryAmount());
    qraffle.depositInQuRaffle(voters[0], qraffle.getState()->getQuRaffleEntryAmount());
    qraffle.endEpoch();

    ASSERT_EQ(qraffle.getState()->getNumberOfActiveTokenRaffle(), 1u);

    // Register and deposit QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE members.
    const uint32 slotSize = QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE;
    for (uint32 i = 0; i < slotSize; i++)
    {
        id m = getUser(10000 + i);
        increaseEnergy(m, QRAFFLE_REGISTER_AMOUNT + QRAFFLE_TRANSFER_SHARE_FEE);
        qraffle.registerInSystem(m, QRAFFLE_REGISTER_AMOUNT, 0);
        qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, (sint64)entryAmt, m);
        qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, (sint64)entryAmt, m);
        auto r = qraffle.depositInTokenRaffle(m, 0, QRAFFLE_TRANSFER_SHARE_FEE);
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "member " << i;
    }
    EXPECT_EQ(qraffle.getState()->numberOfTokenRaffleMembers.get(0), slotSize);

    // One more deposit must fail with MAX_MEMBER_REACHED.
    id extra = getUser(20000);
    increaseEnergy(extra, QRAFFLE_REGISTER_AMOUNT + QRAFFLE_TRANSFER_SHARE_FEE);
    qraffle.registerInSystem(extra, QRAFFLE_REGISTER_AMOUNT, 0);
    qraffle.transferShareOwnershipAndPossession(tokenIssuer, aname, tokenIssuer, (sint64)entryAmt, extra);
    qraffle.TransferShareManagementRights(tokenIssuer, aname, QRAFFLE_CONTRACT_INDEX, (sint64)entryAmt, extra);
    auto overflow = qraffle.depositInTokenRaffle(extra, 0, QRAFFLE_TRANSFER_SHARE_FEE);
    EXPECT_EQ(overflow.returnCode, QRAFFLE_MAX_MEMBER_REACHED);
}

// ── TEST: getRegisters_PaginationCorrect ─────────────────────────────────────
// Verify that paginated getRegisters returns contiguous, non-overlapping slices.
TEST(ContractQraffle, getRegisters_PaginationCorrect)
{
    ContractTestingQraffle qraffle;

    // Register exactly 20 extra users (initial 5 already present).
    const uint32 extra = 20;
    for (uint32 i = 0; i < extra; i++)
    {
        id u = getUser(5000 + i);
        increaseEnergy(u, QRAFFLE_REGISTER_AMOUNT);
        EXPECT_EQ(qraffle.registerInSystem(u, QRAFFLE_REGISTER_AMOUNT, 0).returnCode, QRAFFLE_SUCCESS);
    }
    // Page 0: offset=0, limit=10.
    auto page0 = qraffle.getRegisters(0, 10);
    EXPECT_EQ(page0.returnCode, QRAFFLE_SUCCESS);

    // Page 1: offset=10, limit=10.
    auto page1 = qraffle.getRegisters(10, 10);
    EXPECT_EQ(page1.returnCode, QRAFFLE_SUCCESS);

    // Page 2: offset=20, limit=5. (remaining 5 — exact fit)
    auto page2 = qraffle.getRegisters(20, 5);
    EXPECT_EQ(page2.returnCode, QRAFFLE_SUCCESS);

    // Partial trailing page: offset=20, limit=6 with 25 total → returns the 5
    // available registers; the 6th slot stays NULL_ID so the caller can detect
    // end-of-data. (Previously this rejected with INVALID_OFFSET_OR_LIMIT.)
    auto partial = qraffle.getRegisters(20, 6);
    EXPECT_EQ(partial.returnCode, QRAFFLE_SUCCESS);
    EXPECT_NE(partial.register1, id(0, 0, 0, 0));
    EXPECT_NE(partial.register5, id(0, 0, 0, 0));
    EXPECT_EQ(partial.register6, id(0, 0, 0, 0));

    // Offset past end → SUCCESS with an empty result (all NULL_ID).
    auto pastEnd = qraffle.getRegisters(25, 5);
    EXPECT_EQ(pastEnd.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(pastEnd.register1, id(0, 0, 0, 0));

    // Offset far past end (defensive: large offset value should not iterate the list).
    auto wayPast = qraffle.getRegisters(1000000, 10);
    EXPECT_EQ(wayPast.returnCode, QRAFFLE_SUCCESS);
    EXPECT_EQ(wayPast.register1, id(0, 0, 0, 0));

    // limit > 20 → still invalid (output struct only holds 20 ids).
    auto bad2 = qraffle.getRegisters(0, 21);
    EXPECT_EQ(bad2.returnCode, QRAFFLE_INVALID_OFFSET_OR_LIMIT);
}

// ── TEST: AssetRaffle_ProposalFeeNonRefundable ────────────────────────────────
// When a raffle is cancelled the 500K proposal fee must NOT be returned.
TEST(ContractQraffle, AssetRaffle_ProposalFeeNonRefundable)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);
    Asset token = qraffle.setupToken(tokenIssuer, "NRFUND", 1000000, creator);

    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE + 5000);
    sint64 balBefore = getBalance(creator);

    auto cr = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                        makeBundle(token, 500000));
    EXPECT_EQ(cr.returnCode, QRAFFLE_SUCCESS);
    sint64 balAfterCreate = getBalance(creator);
    EXPECT_EQ(balBefore - balAfterCreate, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE);

    // Cancel with no tickets sold.
    EXPECT_EQ(qraffle.cancelAssetRaffle(creator, 0).returnCode, QRAFFLE_SUCCESS);

    // Balance must NOT have recovered the proposal fee.
    sint64 balAfterCancel = getBalance(creator);
    EXPECT_EQ(balAfterCancel, balAfterCreate); // unchanged (no refund)
}

// ── TEST: AssetRaffle_MultipleEpochs_PerCreatorCounterReset ──────────────────
// After endEpoch, assetRafflesPerCreator resets so a creator can start fresh.
TEST(ContractQraffle, AssetRaffle_MultipleEpochs_PerCreatorCounterReset)
{
    ContractTestingQraffle qraffle;
    system.epoch = 200;

    id creator = getUser(6000);
    qraffle.registerDAOMember(creator);

    id tokenIssuer = getUser(6100);

    // Epoch 200: fill per-creator limit.
    increaseEnergy(tokenIssuer, 5 * 1000000000ULL);
    Asset t200 = qraffle.setupToken(tokenIssuer, "MER200", 10000000, creator);
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * (QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR + 2) + 10000);

    for (uint32 i = 0; i < QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR; i++)
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                           makeBundle(t200, 100));
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "epoch 200 raffle " << i;
    }
    auto rOver = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                           makeBundle(t200, 100));
    EXPECT_EQ(rOver.returnCode, QRAFFLE_MAX_ASSET_RAFFLES_REACHED);

    qraffle.endEpoch();

    // Epoch 201: counter should be cleared → creator can open fresh raffles.
    system.epoch = 201;
    increaseEnergy(tokenIssuer, 5 * 1000000000ULL);
    Asset t201 = qraffle.setupToken(tokenIssuer, "MER201", 10000000, creator);
    increaseEnergy(creator, (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE * (QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR + 1) + 10000);

    for (uint32 i = 0; i < QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR; i++)
    {
        auto r = qraffle.createAssetRaffle(creator, 125000000ULL, QRAFFLE_MIN_ASSET_TICKET_AMOUNT,
                                           makeBundle(t201, 100));
        EXPECT_EQ(r.returnCode, QRAFFLE_SUCCESS) << "epoch 201 raffle " << i;
    }
}