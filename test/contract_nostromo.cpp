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
    unsigned long long userCount = random(0, maxNum);
    std::vector<id> users;
    users.reserve(userCount);
    for (unsigned int i = 0; i < userCount; ++i)
    {
        unsigned long long userIdx = random(0, totalUsers - 1);
        users.push_back(getUser(userIdx));
    }
    return users;
}

class NostromoChecker : public NOST
{
public:
    void registerChecker(id registerId, uint32 tierLevel, uint32 indexOfRegister)
    {
        EXPECT_EQ(registerId, Users.get(indexOfRegister).userId);
        EXPECT_EQ(tierLevel, Users.get(indexOfRegister).tierLevel);
    }
    void countOfRegisterChecker(uint32 totalUser)
    {
        EXPECT_EQ(totalUser, numberOfRegister);
    }
    void logoutFromTierChecker(id registerId)
    {
        uint32 i;
        for (i = 0; i < numberOfRegister; i++)
        {
            if (registerId == Users.get(i).userId)
            {
                break;
            }
        }
        EXPECT_EQ(i, numberOfRegister);
    }
    void numberOfCreatedProjectChecker(uint32 numberOfProjects)
    {
        EXPECT_EQ(numberOfProjects, numberOfCreatedProject);
    }
    void createdProjectChecker(uint32 indexOfProject, id creator, uint64 assetName, uint32 supply, uint32 startYear, uint32 startMonth, uint32 startDay, uint32 startHour, uint32 endYear, uint32 endMonth, uint32 endDay, uint32 endHour)
    {
        uint32 startDate, endDate;
        QUOTTERY::packQuotteryDate(startYear, startMonth, startDay, startHour, 0, 0, startDate);
        QUOTTERY::packQuotteryDate(endYear, endMonth, endDay, endHour, 0, 0, endDate);
        
        EXPECT_EQ(tokens.contains(assetName), 1);
        EXPECT_EQ(projects.get(indexOfProject).creator, creator);
        EXPECT_EQ(projects.get(indexOfProject).isCreatedFundarasing, 0);
        EXPECT_EQ(projects.get(indexOfProject).numberOfNo, 0);
        EXPECT_EQ(projects.get(indexOfProject).numberOfYes, 0);
        EXPECT_EQ(projects.get(indexOfProject).supplyOfToken, supply);
        EXPECT_EQ(projects.get(indexOfProject).tokenName, assetName);
        EXPECT_EQ(projects.get(indexOfProject).startDate, startDate);
        EXPECT_EQ(projects.get(indexOfProject).endDate, endDate);
    }
    void epochRevenueChecker(uint64 amountOfRevenue)
    {
        EXPECT_EQ(amountOfRevenue, epochRevenue);
    }
    void totalPoolWeightChecker(uint32 totalWeight)
    {
        EXPECT_EQ(totalWeight, totalPoolWeight);
    }
    void voteInProjectChecker(uint32 indexOfProject, uint32 numberOfYes, uint32 numberOfNo)
    {
        EXPECT_EQ(projects.get(indexOfProject).numberOfYes, numberOfYes);
        EXPECT_EQ(projects.get(indexOfProject).numberOfNo, numberOfNo);
    }
    void numberOfVotedProjectAndVotedListChecker(id registerId, uint32 numberOfProject, Array<uint32, 64> votedList)
    {
        uint32 count;
        numberOfVotedProject.get(registerId, count);
        EXPECT_EQ(count, numberOfProject);

        Array<uint32, 64> vote;
        voteStatus.get(registerId, vote);
        for (uint32 i = 0; i < count; i++)
        {
            EXPECT_EQ(vote.get(i), votedList.get(i));
        }
    }
    void countOfFundaraisingChecker(uint32 count)
    {
        EXPECT_EQ(count, numberOfFundaraising);
    }
    void createFundaraisingChecker(const id& registerId,
        uint64 tokenPrice,
		uint64 soldAmount,
		uint64 requiredFunds,

		uint32 indexOfProject,
		uint32 firstPhaseStartYear,
		uint32 firstPhaseStartMonth,
		uint32 firstPhaseStartDay,
		uint32 firstPhaseStartHour,
		uint32 firstPhaseEndYear,
		uint32 firstPhaseEndMonth,
		uint32 firstPhaseEndDay,
		uint32 firstPhaseEndHour,

		uint32 secondPhaseStartYear,
		uint32 secondPhaseStartMonth,
		uint32 secondPhaseStartDay,
		uint32 secondPhaseStartHour,
		uint32 secondPhaseEndYear,
		uint32 secondPhaseEndMonth,
		uint32 secondPhaseEndDay,
		uint32 secondPhaseEndHour,
    
        uint32 thirdPhaseStartYear,
		uint32 thirdPhaseStartMonth,
		uint32 thirdPhaseStartDay,
		uint32 thirdPhaseStartHour,
		uint32 thirdPhaseEndYear,
		uint32 thirdPhaseEndMonth,
		uint32 thirdPhaseEndDay,
		uint32 thirdPhaseEndHour,
    
        uint32 listingStartYear,
		uint32 listingStartMonth,
		uint32 listingStartDay,
		uint32 listingStartHour,

		uint32 cliffEndYear,
		uint32 cliffEndMonth,
		uint32 cliffEndDay,
		uint32 cliffEndHour,

		uint32 vestingEndYear,
		uint32 vestingEndMonth,
		uint32 vestingEndDay,
		uint32 vestingEndHour,

		uint8 threshold,
		uint8 TGE,
		uint8 stepOfVesting,
    
        uint32 indexOfFundaraising)
    {
        uint32 firstPhaseStartDate_t, secondPhaseStartDate_t, thirdPhaseStartDate_t, firstPhaseEndDate_t, secondPhaseEndDate_t, thirdPhaseEndDate_t, listingStartDate_t, cliffEndDate_t, vestingEndDate_t;
        QUOTTERY::packQuotteryDate(firstPhaseStartYear, firstPhaseStartMonth, firstPhaseStartDay, firstPhaseStartHour, 0, 0, firstPhaseStartDate_t);
		QUOTTERY::packQuotteryDate(secondPhaseStartYear, secondPhaseStartMonth, secondPhaseStartDay, secondPhaseStartHour, 0, 0, secondPhaseStartDate_t);
		QUOTTERY::packQuotteryDate(thirdPhaseStartYear, thirdPhaseStartMonth, thirdPhaseStartDay, thirdPhaseStartHour, 0, 0, thirdPhaseStartDate_t);
		QUOTTERY::packQuotteryDate(firstPhaseEndYear, firstPhaseEndMonth, firstPhaseEndDay, firstPhaseEndHour, 0, 0, firstPhaseEndDate_t);
		QUOTTERY::packQuotteryDate(secondPhaseEndYear, secondPhaseEndMonth, secondPhaseEndDay, secondPhaseEndHour, 0, 0, secondPhaseEndDate_t);
		QUOTTERY::packQuotteryDate(thirdPhaseEndYear, thirdPhaseEndMonth, thirdPhaseEndDay, thirdPhaseEndHour, 0, 0, thirdPhaseEndDate_t);
		QUOTTERY::packQuotteryDate(listingStartYear, listingStartMonth, listingStartDay, listingStartHour, 0, 0, listingStartDate_t);
		QUOTTERY::packQuotteryDate(cliffEndYear, cliffEndMonth, cliffEndDay, cliffEndHour, 0, 0, cliffEndDate_t);
		QUOTTERY::packQuotteryDate(vestingEndYear, vestingEndMonth, vestingEndDay, vestingEndHour, 0, 0, vestingEndDate_t);

        EXPECT_EQ(registerId, projects.get(fundaraisings.get(indexOfFundaraising).indexOfProject).creator);
        EXPECT_EQ(tokenPrice, fundaraisings.get(indexOfFundaraising).tokenPrice);

        EXPECT_EQ(soldAmount, fundaraisings.get(indexOfFundaraising).soldAmount);
        EXPECT_EQ(requiredFunds, fundaraisings.get(indexOfFundaraising).requiredFunds);
        EXPECT_EQ(indexOfProject, fundaraisings.get(indexOfFundaraising).indexOfProject);
        EXPECT_EQ(firstPhaseStartDate_t, fundaraisings.get(indexOfFundaraising).firstPhaseStartDate);
        EXPECT_EQ(secondPhaseStartDate_t, fundaraisings.get(indexOfFundaraising).secondPhaseStartDate);
        EXPECT_EQ(thirdPhaseStartDate_t, fundaraisings.get(indexOfFundaraising).thirdPhaseStartDate);
        EXPECT_EQ(firstPhaseEndDate_t, fundaraisings.get(indexOfFundaraising).firstPhaseEndDate);
        EXPECT_EQ(secondPhaseEndDate_t, fundaraisings.get(indexOfFundaraising).secondPhaseEndDate);
        EXPECT_EQ(thirdPhaseEndDate_t, fundaraisings.get(indexOfFundaraising).thirdPhaseEndDate);
        EXPECT_EQ(listingStartDate_t, fundaraisings.get(indexOfFundaraising).listingStartDate);
        EXPECT_EQ(cliffEndDate_t, fundaraisings.get(indexOfFundaraising).cliffEndDate);
        EXPECT_EQ(vestingEndDate_t, fundaraisings.get(indexOfFundaraising).vestingEndDate);
        EXPECT_EQ(threshold, fundaraisings.get(indexOfFundaraising).threshold);
        EXPECT_EQ(TGE, fundaraisings.get(indexOfFundaraising).TGE);
        EXPECT_EQ(stepOfVesting, fundaraisings.get(indexOfFundaraising).stepOfVesting);
        
    }
    uint32 getTierLevel(id registerId)
    {
        for (uint32 i = 0; i < numberOfRegister; i++)
        {
            if (Users.get(i).userId == registerId)
            {
                return Users.get(i).tierLevel;
            }
        }
        return 0;
    }
    uint64 getInvestedAmount(uint32 indexOfFundaraising, id registerId)
    {
        investors.get(indexOfFundaraising, tmpInvestedList);
        uint32 numberOfInvestors_t = numberOfInvestors.get(indexOfFundaraising);

        for (uint32 i = 0; i < numberOfInvestors_t; i++)
		{
			if (tmpInvestedList.get(i).investorId == registerId)
			{
				return tmpInvestedList.get(i).investedAmount;
			}
		}

        return 0;
    }
    void totalRaisedFundChecker(uint32 indexOfFundaraising, uint64 raisedFund)
    {
        EXPECT_EQ(raisedFund, fundaraisings.get(indexOfFundaraising).raisedFunds);
        
        if (fundaraisings.get(indexOfFundaraising).isCreatedToken)
        {
            Asset assetInfo;
            assetInfo.assetName = assetNameFromString("GGGG");
            assetInfo.issuer = id(NOST_CONTRACT_INDEX, 0, 0, 0);
            EXPECT_EQ(numberOfShares(assetInfo), projects.get(fundaraisings.get(indexOfFundaraising).indexOfProject).supplyOfToken);
            EXPECT_EQ(numberOfPossessedShares(assetNameFromString("GGGG"), id(NOST_CONTRACT_INDEX, 0, 0, 0), projects.get(fundaraisings.get(indexOfFundaraising).indexOfProject).creator, projects.get(fundaraisings.get(indexOfFundaraising).indexOfProject).creator, 13, 13), projects.get(fundaraisings.get(indexOfFundaraising).indexOfProject).supplyOfToken - fundaraisings.get(indexOfFundaraising).soldAmount);
        }
    }
};

class ContractTestingNostromo : protected ContractTesting
{
public:
    ContractTestingNostromo()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(NOST);
        callSystemProcedure(NOST_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    NostromoChecker* getState()
    {
        return (NostromoChecker*)contractStates[NOST_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(NOST_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void registerInTier(const id& registerId, 
        uint32 tierLevel, 
        uint64 depositeAmount)
    {
        NOST::registerInTier_input input;
        NOST::registerInTier_output output;

        input.tierLevel = tierLevel;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 1, input, output, registerId, depositeAmount);
    }

    void logoutFromTier(const id& registerId)
    {
        NOST::logoutFromTier_input input;
        NOST::logoutFromTier_output output;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 2, input, output, registerId, 0);
    }

    void createProject(const id& registerId, 
        uint64 tokenName, 
        uint64 supply, 
        uint32 startYear, 
        uint32 startMonth, 
        uint32 startDay, 
        uint32 startHour,
		uint32 endYear,
		uint32 endMonth,
		uint32 endDay,
		uint32 endHour)
    {
        NOST::createProject_input input;
        NOST::createProject_output output;

        input.tokenName = tokenName;
        input.supply = supply;
        input.startYear = startYear;
        input.startMonth = startMonth;
        input.startDay = startDay;
        input.startHour = startHour;
		input.endYear = endYear;
		input.endMonth = endMonth;
		input.endDay = endDay;
		input.endHour = endHour;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 3, input, output, registerId, NOSTROMO_CREATE_PROJECT_FEE);
    }

    void voteInProject(const id& registerId,
        uint32 indexOfProject,
		bit decision)
    {
        NOST::voteInProject_input input;
        NOST::voteInProject_output output;

        input.decision = decision;
        input.indexOfProject = indexOfProject;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 4, input, output, registerId, 0);
    }

    void createFundaraising(const id& registerId,
        uint64 tokenPrice,
		uint64 soldAmount,
		uint64 requiredFunds,

		uint32 indexOfProject,
		uint32 firstPhaseStartYear,
		uint32 firstPhaseStartMonth,
		uint32 firstPhaseStartDay,
		uint32 firstPhaseStartHour,
		uint32 firstPhaseEndYear,
		uint32 firstPhaseEndMonth,
		uint32 firstPhaseEndDay,
		uint32 firstPhaseEndHour,

		uint32 secondPhaseStartYear,
		uint32 secondPhaseStartMonth,
		uint32 secondPhaseStartDay,
		uint32 secondPhaseStartHour,
		uint32 secondPhaseEndYear,
		uint32 secondPhaseEndMonth,
		uint32 secondPhaseEndDay,
		uint32 secondPhaseEndHour,

		uint32 thirdPhaseStartYear,
		uint32 thirdPhaseStartMonth,
		uint32 thirdPhaseStartDay,
		uint32 thirdPhaseStartHour,
		uint32 thirdPhaseEndYear,
		uint32 thirdPhaseEndMonth,
		uint32 thirdPhaseEndDay,
		uint32 thirdPhaseEndHour,
    
        uint32 listingStartYear,
		uint32 listingStartMonth,
		uint32 listingStartDay,
		uint32 listingStartHour,

		uint32 cliffEndYear,
		uint32 cliffEndMonth,
		uint32 cliffEndDay,
		uint32 cliffEndHour,

		uint32 vestingEndYear,
		uint32 vestingEndMonth,
		uint32 vestingEndDay,
		uint32 vestingEndHour,

		uint8 threshold,
		uint8 TGE,
		uint8 stepOfVesting)
    {
        NOST::createFundaraising_input input;
        NOST::createFundaraising_output output;

        input.tokenPrice = tokenPrice;
		input.soldAmount = soldAmount;
		input.requiredFunds = requiredFunds;

		input.indexOfProject = indexOfProject;
		input.firstPhaseStartYear = firstPhaseStartYear;
		input.firstPhaseStartMonth = firstPhaseStartMonth;
		input.firstPhaseStartDay = firstPhaseStartDay;
		input.firstPhaseStartHour = firstPhaseStartHour;
		input.firstPhaseEndYear = firstPhaseEndYear;
		input.firstPhaseEndMonth = firstPhaseEndMonth;
		input.firstPhaseEndDay = firstPhaseEndDay;
		input.firstPhaseEndHour = firstPhaseEndHour;

		input.secondPhaseStartYear = secondPhaseStartYear;
		input.secondPhaseStartMonth = secondPhaseStartMonth;
		input.secondPhaseStartDay = secondPhaseStartDay;
		input.secondPhaseStartHour = secondPhaseStartHour;
		input.secondPhaseEndYear = secondPhaseEndYear;
		input.secondPhaseEndMonth = secondPhaseEndMonth;
		input.secondPhaseEndDay = secondPhaseEndDay;
		input.secondPhaseEndHour = secondPhaseEndHour;

        input.thirdPhaseStartYear = thirdPhaseStartYear;
		input.thirdPhaseStartMonth = thirdPhaseStartMonth;
		input.thirdPhaseStartDay = thirdPhaseStartDay;
		input.thirdPhaseStartHour = thirdPhaseStartHour;
		input.thirdPhaseEndYear = thirdPhaseEndYear;
		input.thirdPhaseEndMonth = thirdPhaseEndMonth;
		input.thirdPhaseEndDay = thirdPhaseEndDay;
		input.thirdPhaseEndHour = thirdPhaseEndHour;

		input.listingStartYear = listingStartYear;
		input.listingStartMonth = listingStartMonth;
		input.listingStartDay = listingStartDay;
		input.listingStartHour = listingStartHour;

		input.cliffEndYear = cliffEndYear;
		input.cliffEndMonth = cliffEndMonth;
		input.cliffEndDay = cliffEndDay;
		input.cliffEndHour = cliffEndHour;

		input.vestingEndYear = vestingEndYear;
		input.vestingEndMonth = vestingEndMonth;
		input.vestingEndDay = vestingEndDay;
		input.vestingEndHour = vestingEndHour;

		input.threshold = threshold;
		input.TGE = TGE;
		input.stepOfVesting = stepOfVesting;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 5, input, output, registerId, NOSTROMO_QX_TOKEN_ISSUANCE_FEE);
    }

    void investInProject(const id& investorId, 
        uint32 indexOfFundaraising,
        uint64 investmentAmount)
    {
        NOST::investInProject_input input;
        NOST::investInProject_output output;

        input.indexOfFundaraising = indexOfFundaraising;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 6, input, output, investorId, investmentAmount);
    }

    void claimToken(const id& claimerId,
        uint64 claimAmount,
        uint32 indexOfFundaraising)
    {
        NOST::claimToken_input input;
        NOST::claimToken_output output;

        input.amount = claimAmount;
        input.indexOfFundaraising = indexOfFundaraising;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 7, input, output, claimerId, 0);
    }

    void upgradeTier(const id& registerId, 
        uint32 newTierLevel,
        uint64 depositAmount)
    {
        NOST::upgradeTier_input input;
        NOST::upgradeTier_output output;

        input.newTierLevel = newTierLevel;
        
        invokeUserProcedure(NOST_CONTRACT_INDEX, 8, input, output, registerId, depositAmount);
    }

    sint64 TransferShareManagementRights(const id& user, Asset asset, sint64 numberOfShares, uint32 newManagingContractIndex)
    {
        NOST::TransferShareManagementRights_input input;
        NOST::TransferShareManagementRights_output output;

        input.asset = asset;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(NOST_CONTRACT_INDEX, 9, input, output, user, 1000000);

        return output.transferredNumberOfShares;
    }

    NOST::getStats_output getStats() const
    {
        NOST::getStats_input input;
        NOST::getStats_output output;

        callFunction(NOST_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    NOST::getTierLevelByUser_output getTierLevelByUser(const id& registerId) const
    {
        NOST::getTierLevelByUser_input input;
        NOST::getTierLevelByUser_output output;

        input.userId = registerId;
        callFunction(NOST_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    NOST::getUserVoteStatus_output getUserVoteStatus(const id& registerId) const
    {
        NOST::getUserVoteStatus_input input;
        NOST::getUserVoteStatus_output output;

        input.userId = registerId;
        callFunction(NOST_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    NOST::checkTokenCreatability_output checkTokenCreatability(uint64 tokenName) const
    {
        NOST::checkTokenCreatability_input input;
        NOST::checkTokenCreatability_output output;

        input.tokenName = tokenName;
        callFunction(NOST_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    NOST::getNumberOfInvestedAndClaimedProjects_output getNumberOfInvestedAndClaimedProjects(const id& invsetorId) const
    {
        NOST::getNumberOfInvestedAndClaimedProjects_input input;
        NOST::getNumberOfInvestedAndClaimedProjects_output output;

        input.userId = invsetorId;
        callFunction(NOST_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    NOST::getProjectByIndex_output getProjectByIndex(uint32 indexOfProject) const
    {
        NOST::getProjectByIndex_input input;
        NOST::getProjectByIndex_output output;

        input.indexOfProject = indexOfProject;
        callFunction(NOST_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    NOST::getFundarasingByIndex_output getFundarasingByIndex(uint32 indexOfFundaraising) const
    {
        NOST::getFundarasingByIndex_input input;
        NOST::getFundarasingByIndex_output output;

        input.indexOfFundarasing = indexOfFundaraising;
        callFunction(NOST_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    NOST::getProjectIndexListByCreator_output getProjectIndexListByCreator(const id& creatorId) const
    {
        NOST::getProjectIndexListByCreator_input input;
        NOST::getProjectIndexListByCreator_output output;

        input.creator = creatorId;
        callFunction(NOST_CONTRACT_INDEX, 8, input, output);
        return output;
    }
};

TEST(TestContractNostromo, registerAndLogoutAndUpgradeFromTierChecker)
{
    ContractTestingNostromo nostromoTestCaseA;

    std::map<id, bool> duplicatedUser;
    auto registers = getRandomUsers(10000, 10000);

    uint32 countOfRegister = 0, totalPoolWeight = 0;
    uint64 totalDepositedQubic = 0, totalLogoutFeeAmount = 0;

    for (const auto& user : registers)
    {
        if (duplicatedUser[user])
        {
            continue;
        }
        uint32 tierLevel = (uint32)random(1, 5);
        uint64 depositeAmount, upgradeDeltaDepositeAmount;
        switch (tierLevel)
        {
        case 1:
            depositeAmount = NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT;
            upgradeDeltaDepositeAmount = NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT - NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT;
            totalLogoutFeeAmount += NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT * NOSTROMO_TIER_CHESTBURST_UNSTAKE_FEE / 100;
            totalPoolWeight += NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT;
            break;
        case 2:
            depositeAmount = NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT;
            upgradeDeltaDepositeAmount = NOSTROMO_TIER_DOG_STAKE_AMOUNT - NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT;
            totalLogoutFeeAmount += NOSTROMO_TIER_DOG_STAKE_AMOUNT * NOSTROMO_TIER_DOG_UNSTAKE_FEE / 100;
            totalPoolWeight += NOSTROMO_TIER_DOG_POOL_WEIGHT;
            break;
        case 3:
            depositeAmount = NOSTROMO_TIER_DOG_STAKE_AMOUNT;
            upgradeDeltaDepositeAmount = NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT - NOSTROMO_TIER_DOG_STAKE_AMOUNT;
            totalLogoutFeeAmount += NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT * NOSTROMO_TIER_XENOMORPH_UNSTAKE_FEE / 100;
            totalPoolWeight += NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT;
            break;
        case 4:
            depositeAmount = NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT;
            upgradeDeltaDepositeAmount = NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT - NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT;
            totalLogoutFeeAmount += NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT * NOSTROMO_TIER_WARRIOR_UNSTAKE_FEE / 100;
            totalPoolWeight += NOSTROMO_TIER_WARRIOR_POOL_WEIGHT;
            break;
        case 5:
            depositeAmount = NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT;
            totalLogoutFeeAmount += NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT * NOSTROMO_TIER_WARRIOR_UNSTAKE_FEE / 100;
            totalPoolWeight += NOSTROMO_TIER_WARRIOR_POOL_WEIGHT;
            break;
        default:
            break;
        }
        /*
            Register Tier
        */
        totalDepositedQubic += depositeAmount;
        increaseEnergy(user, depositeAmount);
        nostromoTestCaseA.registerInTier(user, tierLevel, depositeAmount);
        nostromoTestCaseA.getState()->registerChecker(user, tierLevel, countOfRegister);
        /*
            Upgrade Tier
        */
        totalDepositedQubic += upgradeDeltaDepositeAmount;
        increaseEnergy(user, upgradeDeltaDepositeAmount);
        nostromoTestCaseA.upgradeTier(user, tierLevel + 1, upgradeDeltaDepositeAmount);

        if (tierLevel == 5)
        {
            nostromoTestCaseA.getState()->registerChecker(user, tierLevel, countOfRegister);
        }
        else 
        {
            nostromoTestCaseA.getState()->registerChecker(user, tierLevel + 1, countOfRegister);
        }
        
        duplicatedUser[user] = 1;
        countOfRegister++;
    }
    nostromoTestCaseA.getState()->countOfRegisterChecker(countOfRegister);
    nostromoTestCaseA.getState()->epochRevenueChecker(0);
    nostromoTestCaseA.getState()->totalPoolWeightChecker(totalPoolWeight);
    EXPECT_EQ(totalDepositedQubic, getBalance(id(NOST_CONTRACT_INDEX, 0, 0, 0)));
    
    duplicatedUser.clear();
    for (const auto& user : registers)
    {
        if (duplicatedUser[user])
        {
            continue;
        }
        /*
            Logout From Tier
        */
        nostromoTestCaseA.logoutFromTier(user);
        duplicatedUser[user] = 1;
        nostromoTestCaseA.getState()->logoutFromTierChecker(user);
    }
    EXPECT_EQ(totalLogoutFeeAmount, getBalance(id(NOST_CONTRACT_INDEX, 0, 0, 0)));
    nostromoTestCaseA.getState()->countOfRegisterChecker(0);
    nostromoTestCaseA.getState()->epochRevenueChecker(totalLogoutFeeAmount);
    nostromoTestCaseA.getState()->totalPoolWeightChecker(0);
}

TEST(TestContractNostromo, createProjectAndVoteInProjectChecker)
{
    ContractTestingNostromo nostromoTestCaseB;

    auto registers = getRandomUsers(1000, 1000);

    /*
        Register in each Tiers
    */
    increaseEnergy(registers[0], NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT + NOSTROMO_CREATE_PROJECT_FEE);
    nostromoTestCaseB.registerInTier(registers[0], 1, NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT);

    increaseEnergy(registers[1], NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT + NOSTROMO_CREATE_PROJECT_FEE);
    nostromoTestCaseB.registerInTier(registers[1], 2, NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT);
    
    increaseEnergy(registers[2], NOSTROMO_TIER_DOG_STAKE_AMOUNT + NOSTROMO_CREATE_PROJECT_FEE);
    nostromoTestCaseB.registerInTier(registers[2], 3, NOSTROMO_TIER_DOG_STAKE_AMOUNT);
    
    increaseEnergy(registers[3], NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT + NOSTROMO_CREATE_PROJECT_FEE);
    nostromoTestCaseB.registerInTier(registers[3], 4, NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT);
    
    increaseEnergy(registers[4], NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT + NOSTROMO_CREATE_PROJECT_FEE);
    nostromoTestCaseB.registerInTier(registers[4], 5, NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT);
    
    setMemory(utcTime, 0);
    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 12;
    utcTime.Hour = 0;
    updateQpiTime();

    uint64 assetName = assetNameFromString("AAAA");

    /*
        This creation should be failed because there is no qualified to create the project.
    */
    nostromoTestCaseB.createProject(registers[0], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    nostromoTestCaseB.getState()->numberOfCreatedProjectChecker(0);
    nostromoTestCaseB.getState()->epochRevenueChecker(0);
    EXPECT_EQ(getBalance(registers[0]), NOSTROMO_CREATE_PROJECT_FEE);

    /*
        This creation should be failed because there is no qualified to create the project.
    */
    assetName = assetNameFromString("BBBB");
    nostromoTestCaseB.createProject(registers[1], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    nostromoTestCaseB.getState()->numberOfCreatedProjectChecker(0);
    nostromoTestCaseB.getState()->epochRevenueChecker(0);
    EXPECT_EQ(getBalance(registers[1]), NOSTROMO_CREATE_PROJECT_FEE);

    /*
        This creation should be failed because there is no qualified to create the project.
    */
    assetName = assetNameFromString("CCCC");
    nostromoTestCaseB.createProject(registers[2], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    nostromoTestCaseB.getState()->numberOfCreatedProjectChecker(0);
    nostromoTestCaseB.getState()->epochRevenueChecker(0);
    EXPECT_EQ(getBalance(registers[2]), NOSTROMO_CREATE_PROJECT_FEE);

    /*
        This creation should be succeed because there is a qualified to create the project.
    */
    assetName = assetNameFromString("DDDD");
    nostromoTestCaseB.createProject(registers[3], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    nostromoTestCaseB.getState()->numberOfCreatedProjectChecker(1);
    nostromoTestCaseB.getState()->epochRevenueChecker(NOSTROMO_CREATE_PROJECT_FEE);
    nostromoTestCaseB.getState()->createdProjectChecker(0, registers[3], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    EXPECT_EQ(getBalance(registers[3]), 0);

    /*
        This creation should be succeed because there is a qualified to create the project.
    */
    assetName = assetNameFromString("EEEE");
    nostromoTestCaseB.createProject(registers[4], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    nostromoTestCaseB.getState()->numberOfCreatedProjectChecker(2);
    nostromoTestCaseB.getState()->epochRevenueChecker(NOSTROMO_CREATE_PROJECT_FEE * 2);
    nostromoTestCaseB.getState()->createdProjectChecker(1, registers[4], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);
    EXPECT_EQ(getBalance(registers[4]), 0);

    setMemory(utcTime, 0);
    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 13;
    utcTime.Hour = 0;
    updateQpiTime();

    Array<uint32, 64> votedList;

    nostromoTestCaseB.voteInProject(registers[0], 0, 0);
    votedList.set(0, 0);
    nostromoTestCaseB.voteInProject(registers[1], 0, 1);
    nostromoTestCaseB.voteInProject(registers[2], 0, 1);
    nostromoTestCaseB.voteInProject(registers[3], 0, 1);
    nostromoTestCaseB.voteInProject(registers[4], 0, 0);

    nostromoTestCaseB.getState()->voteInProjectChecker(0, 3, 2);
    nostromoTestCaseB.getState()->numberOfVotedProjectAndVotedListChecker(registers[0], 1, votedList);

    /*
        This vote should be failed.
    */
    nostromoTestCaseB.voteInProject(registers[0], 0, 0);
    nostromoTestCaseB.getState()->voteInProjectChecker(0, 3, 2);
    nostromoTestCaseB.getState()->numberOfVotedProjectAndVotedListChecker(registers[0], 1, votedList);

    /*
        This vote should be succeed.
    */
    nostromoTestCaseB.voteInProject(registers[0], 1, 0);
    votedList.set(1, 1);
    nostromoTestCaseB.getState()->voteInProjectChecker(1, 0, 1);
    nostromoTestCaseB.getState()->numberOfVotedProjectAndVotedListChecker(registers[0], 2, votedList);

    nostromoTestCaseB.voteInProject(registers[1], 1, 1);
    nostromoTestCaseB.voteInProject(registers[2], 1, 1);
    nostromoTestCaseB.voteInProject(registers[3], 1, 1);
    nostromoTestCaseB.voteInProject(registers[4], 1, 1);
    nostromoTestCaseB.getState()->voteInProjectChecker(1, 4, 1);
}

TEST(TestContractNostromo, createFundaraisingAndInvestInProjectAndClaimTokenChecker)
{
    ContractTestingNostromo nostromoTestCaseC;

    auto registers = getRandomUsers(10000, 10000);

    setMemory(utcTime, 0);
    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 11;
    utcTime.Hour = 0;
    updateQpiTime();

    increaseEnergy(registers[0], NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT + NOSTROMO_CREATE_PROJECT_FEE + NOSTROMO_QX_TOKEN_ISSUANCE_FEE);
    nostromoTestCaseC.registerInTier(registers[0], 5, NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT);
    uint64 assetName = assetNameFromString("GGGG");
    nostromoTestCaseC.createProject(registers[0], assetName, 21000000, 25, 6, 13, 0, 25, 6, 15, 0);

    std::map<id, bool> duplicatedUser;
    uint64 totalPoolWeight = NOSTROMO_TIER_WARRIOR_POOL_WEIGHT, totalDepositedQubic = NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT;
    uint32 countOfRegister = 0;

    for (const auto& user : registers)
    {
        if (countOfRegister == 0)
        {
            countOfRegister++;
            continue;
        }
        
        if (duplicatedUser[user])
        {
            continue;
        }
        uint32 tierLevel = (uint32)random(1, 5);
        uint64 depositeAmount;
        switch (tierLevel)
        {
        case 1:
            depositeAmount = NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT;
            totalPoolWeight += NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT;
            break;
        case 2:
            depositeAmount = NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT;
            totalPoolWeight += NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT;
            break;
        case 3:
            depositeAmount = NOSTROMO_TIER_DOG_STAKE_AMOUNT;
            totalPoolWeight += NOSTROMO_TIER_DOG_POOL_WEIGHT;
            break;
        case 4:
            depositeAmount = NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT;
            totalPoolWeight += NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT;
            break;
        case 5:
            depositeAmount = NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT;
            totalPoolWeight += NOSTROMO_TIER_WARRIOR_POOL_WEIGHT;
            break;
        default:
            break;
        }
        /*
            Register Tier
        */
        totalDepositedQubic += depositeAmount;
        increaseEnergy(user, depositeAmount);
        nostromoTestCaseC.registerInTier(user, tierLevel, depositeAmount);

        duplicatedUser[user] = 1;
        countOfRegister++;
    }

    /*
        Vote in Project
    */

    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 14;
    utcTime.Hour = 0;
    updateQpiTime();

    uint32 Ynumber = 0, Nnumber = 0;
    duplicatedUser.clear();

    for (const auto& user : registers)
    {
        if (duplicatedUser[user])
        {
            continue;
        }
        
        bit decision = (bit)random(0, 3);
        if (decision)
        {
            Ynumber++;
        }
        else
        {
            Nnumber++;
        }
        
        nostromoTestCaseC.voteInProject(user, 0, decision);
        duplicatedUser[user] = 1;
    }
    nostromoTestCaseC.getState()->voteInProjectChecker(0, Ynumber, Nnumber);
    
    /*
        Create the Fundaraising
    */

    // This fundaraising should not be created because the voting is not finished yet.
    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 14;
    utcTime.Hour = 0;
    updateQpiTime();

    nostromoTestCaseC.createFundaraising(registers[0], 100, 2000000, 150000000, 0, 
        25, 6, 17, 0,
        25, 6, 25, 0,
        25, 6, 28, 0,
        25, 7, 1, 0,
        25, 7, 10, 0,
        25, 7, 15, 0,
        25, 7, 25, 0,
        25, 7, 27, 0,
        26, 7, 27, 0,
        20, 10, 12);
        
    nostromoTestCaseC.getState()->countOfFundaraisingChecker(0);

    // It should be created.

    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 16;
    utcTime.Hour = 0;
    updateQpiTime();

    nostromoTestCaseC.createFundaraising(registers[0], 100000, 2000000, 150000000000, 0, 
        25, 6, 17, 0,
        25, 6, 25, 0,
        25, 6, 28, 0,
        25, 7, 1, 0,
        25, 7, 10, 0,
        25, 7, 15, 0,
        25, 7, 25, 0,
        25, 7, 27, 0,
        26, 7, 27, 0,
        20, 10, 12);
        
    nostromoTestCaseC.getState()->countOfFundaraisingChecker(1);
    nostromoTestCaseC.getState()->createFundaraisingChecker(registers[0], 100000, 2000000, 150000000000, 0, 
        25, 6, 17, 0,
        25, 6, 25, 0,
        25, 6, 28, 0,
        25, 7, 1, 0,
        25, 7, 10, 0,
        25, 7, 15, 0,
        25, 7, 25, 0,
        25, 7, 27, 0,
        26, 7, 27, 0,
        20, 10, 12, 0);

    /*
        Phase 1 Investment
    */
    utcTime.Year = 2025;
    utcTime.Month = 6;
    utcTime.Day = 17;
    utcTime.Hour = 1;
    updateQpiTime();

    uint64 facehuggerMaxInvestAmount = 180000000000 * NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT / totalPoolWeight;
    uint64 chestburstMaxInvestAmount = 180000000000 * NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT / totalPoolWeight;
    uint64 dogMaxInvestAmount = 180000000000 * NOSTROMO_TIER_DOG_POOL_WEIGHT / totalPoolWeight;
    uint64 xenomorphMaxInvestAmount = 180000000000 * NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT / totalPoolWeight;
    uint64 warriorMaxInvestAmount = 180000000000 * NOSTROMO_TIER_WARRIOR_POOL_WEIGHT / totalPoolWeight;

    uint64 totalInvestedAmount = 0;
    duplicatedUser.clear();
    uint32 ct = 0;
    uint32 overDeposit = 1000;      // it should be ignored
    uint64 originalSCBalance = getBalance(id(NOST_CONTRACT_INDEX, 0, 0, 0));
    for (const auto& user : registers)
    {
        if (duplicatedUser[user])
        {
            ct++;
            continue;
        }
        ct++;
        increaseEnergy(user, 180000000000);
        uint32 tierLevel = nostromoTestCaseC.getState()->getTierLevel(user);

        if (ct = 4000)
        {
            /*
                Phase 2 Investment
            */
            utcTime.Year = 2025;
            utcTime.Month = 6;
            utcTime.Day = 29;
            utcTime.Hour = 0;
            updateQpiTime();
        }

        switch (tierLevel)
        {
        case 1:
            if (ct < 4000)
            {
                totalInvestedAmount += facehuggerMaxInvestAmount;
            }
            nostromoTestCaseC.investInProject(user, 0, facehuggerMaxInvestAmount + overDeposit);
            break;
        case 2:
            if (ct < 4000)
            {
                totalInvestedAmount += chestburstMaxInvestAmount;
            }
            nostromoTestCaseC.investInProject(user, 0, chestburstMaxInvestAmount + overDeposit);
            break;
        case 3:
            if (ct < 4000)
            {
                totalInvestedAmount += dogMaxInvestAmount;
            }
            nostromoTestCaseC.investInProject(user, 0, dogMaxInvestAmount + overDeposit);
            break;
        case 4:
            totalInvestedAmount += xenomorphMaxInvestAmount;
            nostromoTestCaseC.investInProject(user, 0, xenomorphMaxInvestAmount + overDeposit);
            break;
        case 5:
            totalInvestedAmount += warriorMaxInvestAmount;
            nostromoTestCaseC.investInProject(user, 0, warriorMaxInvestAmount + overDeposit);
            break;
        
        default:
            break;
        }
        
        duplicatedUser[user] = 1;
    }

    nostromoTestCaseC.getState()->totalRaisedFundChecker(0, totalInvestedAmount);
    EXPECT_EQ(originalSCBalance + totalInvestedAmount - NOSTROMO_QX_TOKEN_ISSUANCE_FEE, getBalance(id(NOST_CONTRACT_INDEX, 0, 0, 0)));

    /*
        Phase 3 Investment
    */
    utcTime.Year = 2025;
    utcTime.Month = 7;
    utcTime.Day = 11;
    utcTime.Hour = 0;
    updateQpiTime();

    uint64 amount = 10000000;
    duplicatedUser.clear();
    for (const auto& user : registers)
    {
        if (duplicatedUser[user])
        {
            continue;
        }
        increaseEnergy(user, amount);
        if (totalInvestedAmount + amount < 180000000000)
        {
            totalInvestedAmount += amount;
        }
        nostromoTestCaseC.investInProject(user, 0, amount);
        
        duplicatedUser[user] = 1;
    }

    nostromoTestCaseC.getState()->totalRaisedFundChecker(0, totalInvestedAmount);
    EXPECT_EQ(originalSCBalance + totalInvestedAmount - NOSTROMO_QX_TOKEN_ISSUANCE_FEE, getBalance(id(NOST_CONTRACT_INDEX, 0, 0, 0)));

    for (uint32 i = 1; i <= 12; i++)
    {
        if (i >= 6) 
        {
            utcTime.Year = 2026;
        }
        utcTime.Month = (7 + i) % 12;
        if (utcTime.Month == 0) utcTime.Month = 12;
        utcTime.Day = 5;
        utcTime.Hour = 0;
        updateQpiTime();

        duplicatedUser.clear();
        for (const auto& user : registers)
        {
            if (duplicatedUser[user])
            {
                continue;
            }
            
            uint64 investedAmount = nostromoTestCaseC.getState()->getInvestedAmount(0, user);
            uint64 claimAmount = investedAmount / 100000 / 12;

            nostromoTestCaseC.claimToken(user, claimAmount, 0);
            
            duplicatedUser[user] = 1;
        }
    }

    ct = 0;
    duplicatedUser.clear();
    for (const auto& user : registers)
    {
        if (duplicatedUser[user])
        {
            continue;
        }
        if (ct == 0)
        {
            EXPECT_EQ(numberOfPossessedShares(assetName, id(NOST_CONTRACT_INDEX, 0, 0, 0), user, user, NOST_CONTRACT_INDEX, NOST_CONTRACT_INDEX) - 19000000, nostromoTestCaseC.getState()->getInvestedAmount(0, user) / 1200000 * 12);
        }
        else
        {
            EXPECT_EQ(numberOfPossessedShares(assetName, id(NOST_CONTRACT_INDEX, 0, 0, 0), user, user, NOST_CONTRACT_INDEX, NOST_CONTRACT_INDEX), nostromoTestCaseC.getState()->getInvestedAmount(0, user) / 1200000 * 12);
        }
        ct++;
        duplicatedUser[user] = 1;
    }

    /*
        transferShareManagementRights Checker
    */

    increaseEnergy(registers[0], 1000000);

    Asset asset;
    asset.assetName = assetName;
    asset.issuer = id(NOST_CONTRACT_INDEX, 0, 0, 0);
    EXPECT_EQ(nostromoTestCaseC.TransferShareManagementRights(registers[0], asset, 10000, QX_CONTRACT_INDEX), 10000);
    EXPECT_EQ(numberOfPossessedShares(asset.assetName, id(NOST_CONTRACT_INDEX, 0, 0, 0), registers[0], registers[0], QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 10000);
}
