using namespace QPI;

constexpr uint64 NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT = 20000000ULL;
constexpr uint64 NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT = 100000000ULL;
constexpr uint64 NOSTROMO_TIER_DOG_STAKE_AMOUNT = 200000000ULL;
constexpr uint64 NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT = 800000000ULL;
constexpr uint64 NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT = 3200000000ULL;
constexpr uint64 NOSTROMO_QX_TOKEN_ISSUANCE_FEE = 1000000000ULL;

constexpr uint32 NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT = 55;
constexpr uint32 NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT = 300;
constexpr uint32 NOSTROMO_TIER_DOG_POOL_WEIGHT = 750;
constexpr uint32 NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT = 3050;
constexpr uint32 NOSTROMO_TIER_WARRIOR_POOL_WEIGHT = 13750;

constexpr uint32 NOSTROMO_TIER_FACEHUGGER_UNSTAKE_FEE = 5;
constexpr uint32 NOSTROMO_TIER_CHESTBURST_UNSTAKE_FEE = 4;
constexpr uint32 NOSTROMO_TIER_DOG_UNSTAKE_FEE = 3;
constexpr uint32 NOSTROMO_TIER_XENOMORPH_UNSTAKE_FEE = 2;
constexpr uint32 NOSTROMO_TIER_WARRIOR_UNSTAKE_FEE = 1;
constexpr uint32 NOSTROMO_CREATE_PROJECT_FEE = 100000000;

constexpr uint32 NOSTROMO_MAX_USER = 524288;
constexpr uint32 NOSTROMO_MAX_NUMBER_PROJECT = 524288;
constexpr uint32 NOSTROMO_MAX_NUMBER_TOKEN = 65536;
constexpr uint32 NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST = 64;

struct NOST2
{
};

struct NOST : public ContractBase
{
public:

	struct registerInTier_input
	{
		uint32 tierLevel;
	};

	struct registerInTier_output
	{
		uint32 tierLevel;
	};

	struct logoutFromTier_input
	{

	};

	struct logoutFromTier_output
	{
		bit result;
	};

	struct createProject_input
	{
		uint64 tokenName;
		uint64 supply;
		uint32 startYear;
		uint32 startMonth;
		uint32 startDay;
		uint32 startHour;
		uint32 endYear;
		uint32 endMonth;
		uint32 endDay;
		uint32 endHour;
	};

	struct createProject_output
	{
		uint32 indexOfProject;
	};

	struct voteInProject_input
	{
		uint32 indexOfProject;
		bit decision;
	};

	struct voteInProject_output
	{

	};

	struct createFundaraising_input
	{
		uint64 tokenPrice;
		uint64 soldAmount;
		uint64 requiredFunds;

		uint32 indexOfProject;
		uint32 firstPhaseStartYear;
		uint32 firstPhaseStartMonth;
		uint32 firstPhaseStartDay;
		uint32 firstPhaseStartHour;
		uint32 firstPhaseEndYear;
		uint32 firstPhaseEndMonth;
		uint32 firstPhaseEndDay;
		uint32 firstPhaseEndHour;

		uint32 secondPhaseStartYear;
		uint32 secondPhaseStartMonth;
		uint32 secondPhaseStartDay;
		uint32 secondPhaseStartHour;
		uint32 secondPhaseEndYear;
		uint32 secondPhaseEndMonth;
		uint32 secondPhaseEndDay;
		uint32 secondPhaseEndHour;

		uint32 thirdPhaseStartYear;
		uint32 thirdPhaseStartMonth;
		uint32 thirdPhaseStartDay;
		uint32 thirdPhaseStartHour;
		uint32 thirdPhaseEndYear;
		uint32 thirdPhaseEndMonth;
		uint32 thirdPhaseEndDay;
		uint32 thirdPhaseEndHour;

		uint32 listingStartYear;
		uint32 listingStartMonth;
		uint32 listingStartDay;
		uint32 listingStartHour;

		uint32 cliffEndYear;
		uint32 cliffEndMonth;
		uint32 cliffEndDay;
		uint32 cliffEndHour;

		uint32 vestingEndYear;
		uint32 vestingEndMonth;
		uint32 vestingEndDay;
		uint32 vestingEndHour;

		uint8 threshold;
		uint8 TGE;
		uint8 stepOfVesting;
	};

	struct createFundaraising_output
	{

	};

	struct investInProject_input
	{
		uint32 indexOfFundaraising;
	};

	struct investInProject_output
	{

	};

	struct claimToken_input
	{
		uint64 amount;
		uint32 indexOfFundaraising;
	};

	struct claimToken_output
	{

	};

	struct upgradeTier_input
	{
		uint32 newTierLevel;
	};

	struct upgradeTier_output
	{

	};

	struct getStats_input
	{

	};

	struct getStats_output
	{
		uint64 epochRevenue, totalPoolWeight;
		uint32 numberOfRegister, numberOfCreatedProject, numberOfFundaraising;
	};

	struct getTierLevelByUser_input
	{
		id userId;
	};

	struct getTierLevelByUser_output
	{
		uint8 tierLevel;
	};

	struct getUserVoteStatus_input
	{
		id userId;
	};

	struct getUserVoteStatus_output
	{
		uint32 numberOfVotedProjects;
		Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST> projectIndexList;
	};

	struct checkTokenCreatability_input
	{
		uint64 tokenName;
	};

	struct checkTokenCreatability_output
	{
		bit result;             // result = 1 is the token already issued by SC
	};

	struct getNumberOfInvestedAndClaimedProjects_input
	{
		id userId;
	};

	struct getNumberOfInvestedAndClaimedProjects_output
	{
		uint32 numberOfInvestedProjects;
		uint32 numberOfClaimedProjects;
	};

protected:

	struct userInfo
	{
		id userId;
		uint8 tierLevel;
	};
	Array<userInfo, NOSTROMO_MAX_USER> Users;
	HashMap<id, Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST>, NOSTROMO_MAX_USER> voteStatus;
	HashMap<id, uint32, NOSTROMO_MAX_USER> numberOfVotedProject;
	HashSet<uint64, NOSTROMO_MAX_NUMBER_TOKEN> tokens;

	struct investInfo
	{
		id investorId;
		uint64 investedAmount;
	}; 

	struct claimInfo
	{
		uint64 claimedAmount;
		uint32 indexOfFundaraising;
	}; 

	HashMap<uint32, Array<investInfo, NOSTROMO_MAX_USER>, 16> investors;
	HashMap<id, Array<claimInfo, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST>, NOSTROMO_MAX_USER> claimers;
	Array<uint32, NOSTROMO_MAX_USER> numberOfInvestors;
	HashMap<id, uint32, NOSTROMO_MAX_USER> numberOfInvestedProjects;
	HashMap<id, uint32, NOSTROMO_MAX_USER> numberOfClaimedProjects;
	Array<investInfo, NOSTROMO_MAX_USER> tmpInvestedList;
	Array<claimInfo, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST> tmpClaimedList;

	struct projectInfo
	{
		id creator;
		uint64 tokenName;
		uint64 supplyOfToken;
		uint32 startDate;
		uint32 endDate;
		uint32 numberOfYes;
		uint32 numberOfNo;
		bit isCreatedFundarasing;
	};

	Array<projectInfo, NOSTROMO_MAX_NUMBER_PROJECT> projects;

	struct fundaraisingInfo
	{
		uint64 tokenPrice;
		uint64 soldAmount;
		uint64 requiredFunds;
		uint64 raisedFunds;
		uint32 indexOfProject;
		uint32 firstPhaseStartDate;
		uint32 firstPhaseEndDate;
		uint32 secondPhaseStartDate;
		uint32 secondPhaseEndDate;
		uint32 thirdPhaseStartDate;
		uint32 thirdPhaseEndDate;
		uint32 listingStartDate;
		uint32 cliffEndDate;
		uint32 vestingEndDate;
		uint8 threshold;
		uint8 TGE;
		uint8 stepOfVesting;
		bit isCreatedToken;
	};

	Array<fundaraisingInfo, NOSTROMO_MAX_NUMBER_PROJECT> fundarasings;

	uint64 epochRevenue, totalPoolWeight;
	uint32 numberOfRegister, numberOfCreatedProject, numberOfFundaraising;

	/**
	* @return Current date from core node system
	*/

	inline static void getCurrentDate(const QPI::QpiContextFunctionCall& qpi, uint32& res) 
	{
        QUOTTERY::packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), res);
    }

	struct registerInTier_locals
	{
		userInfo newUser;
		uint32 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(registerInTier)
	{
		for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			if (state.Users.get(locals.i).userId == qpi.invocator())
			{
				return ;
			}
		}
		
		switch (input.tierLevel)
		{
		case 1:
			if (qpi.invocationReward() < NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			else 
			{
				locals.newUser.userId = qpi.invocator();
				locals.newUser.tierLevel = input.tierLevel;
				state.Users.set(state.numberOfRegister++, locals.newUser);
				if (qpi.invocationReward() > NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT);
				}
				state.totalPoolWeight += NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT;
				output.tierLevel = input.tierLevel;
			}
			break;
		case 2:
			if (qpi.invocationReward() < NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			else 
			{
				locals.newUser.userId = qpi.invocator();
				locals.newUser.tierLevel = input.tierLevel;
				state.Users.set(state.numberOfRegister++, locals.newUser);
				if (qpi.invocationReward() > NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT);
				}
				state.totalPoolWeight += NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT;
				output.tierLevel = input.tierLevel;
			}
			break;
		case 3:
			if (qpi.invocationReward() < NOSTROMO_TIER_DOG_STAKE_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			else 
			{
				locals.newUser.userId = qpi.invocator();
				locals.newUser.tierLevel = input.tierLevel;
				state.Users.set(state.numberOfRegister++, locals.newUser);
				if (qpi.invocationReward() > NOSTROMO_TIER_DOG_STAKE_AMOUNT)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - NOSTROMO_TIER_DOG_STAKE_AMOUNT);
				}
				state.totalPoolWeight += NOSTROMO_TIER_DOG_POOL_WEIGHT;
				output.tierLevel = input.tierLevel;
			}
			break;
		case 4:
			if (qpi.invocationReward() < NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			else 
			{
				locals.newUser.userId = qpi.invocator();
				locals.newUser.tierLevel = input.tierLevel;
				state.Users.set(state.numberOfRegister++, locals.newUser);
				if (qpi.invocationReward() > NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT);
				}
				state.totalPoolWeight += NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT;
				output.tierLevel = input.tierLevel;
			}
			break;
		case 5:
			if (qpi.invocationReward() < NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			else 
			{
				locals.newUser.userId = qpi.invocator();
				locals.newUser.tierLevel = input.tierLevel;
				state.Users.set(state.numberOfRegister++, locals.newUser);
				if (qpi.invocationReward() > NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT);
				}
				state.totalPoolWeight += NOSTROMO_TIER_WARRIOR_POOL_WEIGHT;
				output.tierLevel = input.tierLevel;
			}
			break;
		default:
			break;
		}
	}

	struct logoutFromTier_locals
	{
		uint64 earnedAmount;
		uint32 elementIndex, i;
		uint8 tierLevel;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(logoutFromTier)
	{
		for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			if (state.Users.get(locals.i).userId == qpi.invocator())
			{
				break;
			}
		}
		if (locals.i == state.numberOfRegister)
		{
			return ;
		}
		
		locals.tierLevel = state.Users.get(locals.i).tierLevel;
		switch (locals.tierLevel)
		{
		case 1:
			locals.earnedAmount = div(NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT * NOSTROMO_TIER_FACEHUGGER_UNSTAKE_FEE, 100ULL);
			qpi.transfer(qpi.invocator(), qpi.invocationReward() + NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT - locals.earnedAmount);
			state.epochRevenue += locals.earnedAmount;
			state.totalPoolWeight -= NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT;
			break;
		case 2:
			locals.earnedAmount = div(NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT * NOSTROMO_TIER_CHESTBURST_UNSTAKE_FEE, 100ULL);
			qpi.transfer(qpi.invocator(), qpi.invocationReward() + NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT - locals.earnedAmount);
			state.epochRevenue += locals.earnedAmount;
			state.totalPoolWeight -= NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT;
			break;
		case 3:
			locals.earnedAmount = div(NOSTROMO_TIER_DOG_STAKE_AMOUNT * NOSTROMO_TIER_DOG_UNSTAKE_FEE, 100ULL);
			qpi.transfer(qpi.invocator(), qpi.invocationReward() + NOSTROMO_TIER_DOG_STAKE_AMOUNT - locals.earnedAmount);
			state.epochRevenue += locals.earnedAmount;
			state.totalPoolWeight -= NOSTROMO_TIER_DOG_POOL_WEIGHT;
			break;
		case 4:
			locals.earnedAmount = div(NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT * NOSTROMO_TIER_XENOMORPH_UNSTAKE_FEE, 100ULL);
			qpi.transfer(qpi.invocator(), qpi.invocationReward() + NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT - locals.earnedAmount);
			state.epochRevenue += locals.earnedAmount;
			state.totalPoolWeight -= NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT;
			break;
		case 5:
			locals.earnedAmount = div(NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT * NOSTROMO_TIER_WARRIOR_UNSTAKE_FEE, 100ULL);
			qpi.transfer(qpi.invocator(), qpi.invocationReward() + NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT - locals.earnedAmount);
			state.epochRevenue += locals.earnedAmount;
			state.totalPoolWeight -= NOSTROMO_TIER_WARRIOR_POOL_WEIGHT;
			break;
		}

		state.Users.set(locals.i, state.Users.get(--state.numberOfRegister));
		output.result = 1;
	}

	struct createProject_locals
	{
		projectInfo newProject;
		uint32 elementIndex, startDate, endDate, curDate, i;
		uint8 tierLevel;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createProject)
	{
		QUOTTERY::packQuotteryDate(input.startYear, input.startMonth, input.startDay, input.startHour, 0, 0, locals.startDate);
		QUOTTERY::packQuotteryDate(input.endYear, input.endMonth, input.endDay, input.endHour, 0, 0, locals.endDate);
		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate > locals.startDate || locals.startDate >= locals.endDate || QUOTTERY::checkValidQtryDateTime(locals.startDate) == 0 || QUOTTERY::checkValidQtryDateTime(locals.endDate) == 0)
		{
			output.indexOfProject = NOSTROMO_MAX_NUMBER_PROJECT;
			return;
		}

		if (state.tokens.contains(input.tokenName))
		{
			output.indexOfProject = NOSTROMO_MAX_NUMBER_PROJECT;
			return ;
		}

		for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			if (state.Users.get(locals.i).userId == qpi.invocator())
			{
				break;
			}
		}

		if (locals.i != state.numberOfRegister && (locals.tierLevel == 4 || locals.tierLevel == 5))
		{
			if (qpi.invocationReward() < NOSTROMO_CREATE_PROJECT_FEE)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.indexOfProject = NOSTROMO_MAX_NUMBER_PROJECT;
				return ;
			}
			if (qpi.invocationReward() > NOSTROMO_CREATE_PROJECT_FEE)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - NOSTROMO_CREATE_PROJECT_FEE);
			}
			state.epochRevenue += NOSTROMO_CREATE_PROJECT_FEE;
			
			locals.newProject.creator = qpi.invocator();
			locals.newProject.tokenName = input.tokenName;
			locals.newProject.supplyOfToken = input.supply;
			locals.newProject.startDate = locals.startDate;
			locals.newProject.endDate = locals.endDate;
			locals.newProject.numberOfYes = 0;
			locals.newProject.numberOfNo = 0;

			output.indexOfProject = state.numberOfCreatedProject;
			state.projects.set(state.numberOfCreatedProject++, locals.newProject);
			state.tokens.add(input.tokenName);
		}
		else 
		{
			output.indexOfProject = NOSTROMO_MAX_NUMBER_PROJECT;
		}
	}

	struct voteInProject_locals
	{
		projectInfo votedProject;
		Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST> votedList;
		uint32 elementIndex, curDate, numberOfVotedProject, i;
		bit flag;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(voteInProject)
	{
		if (input.indexOfProject >= state.numberOfCreatedProject)
		{
			return ;
		}
		for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			if (state.Users.get(locals.i).userId == qpi.invocator())
			{
				break;
			}
		}
		if (locals.i == state.numberOfRegister)
		{
			return ;
		}
		state.numberOfVotedProject.get(qpi.invocator(), locals.numberOfVotedProject);
		if (locals.numberOfVotedProject == NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST)
		{
			return ;
		}
		state.voteStatus.get(qpi.invocator(), locals.votedList);
		for (locals.i = 0; locals.i < locals.numberOfVotedProject; locals.i++)
		{
			if (locals.votedList.get(locals.i) == input.indexOfProject)
			{
				return ;
			}
			
		}
		getCurrentDate(qpi, locals.curDate);
		if (locals.curDate >= state.projects.get(input.indexOfProject).startDate && locals.curDate < state.projects.get(input.indexOfProject).endDate)
		{
			locals.votedProject = state.projects.get(input.indexOfProject);
			if (input.decision)
			{
				locals.votedProject.numberOfYes++;
			}
			else 
			{
				locals.votedProject.numberOfNo++;
			}
			state.projects.set(input.indexOfProject, locals.votedProject);
			locals.votedList.set(locals.numberOfVotedProject++, input.indexOfProject);
			state.voteStatus.replace(qpi.invocator(), locals.votedList);
			state.numberOfVotedProject.replace(qpi.invocator(), locals.numberOfVotedProject);
		}
	}

	struct createFundaraising_locals
	{
		projectInfo tmpProject;
		fundaraisingInfo newFundaraising;
		uint32 curDate, firstPhaseStartDate, firstPhaseEndDate, secondPhaseStartDate, secondPhaseEndDate, thirdPhaseStartDate, thirdPhaseEndDate, listingStartDate, cliffEndDate, vestingEndDate;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createFundaraising)
	{
		getCurrentDate(qpi, locals.curDate);
		QUOTTERY::packQuotteryDate(input.firstPhaseStartYear, input.firstPhaseStartMonth, input.firstPhaseStartDay, input.firstPhaseStartHour, 0, 0, locals.firstPhaseStartDate);
		QUOTTERY::packQuotteryDate(input.secondPhaseStartYear, input.secondPhaseStartMonth, input.secondPhaseStartDay, input.secondPhaseStartHour, 0, 0, locals.secondPhaseStartDate);
		QUOTTERY::packQuotteryDate(input.thirdPhaseStartYear, input.thirdPhaseStartMonth, input.thirdPhaseStartDay, input.thirdPhaseStartHour, 0, 0, locals.thirdPhaseStartDate);
		QUOTTERY::packQuotteryDate(input.firstPhaseEndYear, input.firstPhaseEndMonth, input.firstPhaseEndDay, input.firstPhaseEndHour, 0, 0, locals.firstPhaseEndDate);
		QUOTTERY::packQuotteryDate(input.secondPhaseEndYear, input.secondPhaseEndMonth, input.secondPhaseEndDay, input.secondPhaseEndHour, 0, 0, locals.secondPhaseEndDate);
		QUOTTERY::packQuotteryDate(input.thirdPhaseEndYear, input.thirdPhaseEndMonth, input.thirdPhaseEndDay, input.thirdPhaseEndHour, 0, 0, locals.thirdPhaseEndDate);
		QUOTTERY::packQuotteryDate(input.listingStartYear, input.listingStartMonth, input.listingStartDay, input.listingStartHour, 0, 0, locals.listingStartDate);
		QUOTTERY::packQuotteryDate(input.cliffEndYear, input.cliffEndMonth, input.cliffEndDay, input.cliffEndHour, 0, 0, locals.cliffEndDate);
		QUOTTERY::packQuotteryDate(input.vestingEndYear, input.vestingEndMonth, input.vestingEndDay, input.vestingEndHour, 0, 0, locals.vestingEndDate);

		if (locals.curDate > locals.firstPhaseStartDate || locals.firstPhaseStartDate >= locals.firstPhaseEndDate || locals.firstPhaseEndDate > locals.secondPhaseStartDate || locals.secondPhaseStartDate >= locals.secondPhaseEndDate || locals.secondPhaseEndDate > locals.thirdPhaseStartDate || locals.thirdPhaseStartDate >= locals.thirdPhaseEndDate || locals.thirdPhaseEndDate > locals.listingStartDate || locals.listingStartDate > locals.cliffEndDate || locals.cliffEndDate > locals.vestingEndDate)
		{
			return ;
		}

		if (input.stepOfVesting == 0 || input.stepOfVesting > 12 || input.TGE > 50 || input.threshold > 50 || input.indexOfProject >= state.numberOfCreatedProject)
		{
			return ;
		}
		

		if (state.projects.get(input.indexOfProject).creator != qpi.invocator())
		{
			return ;
		}

		if (input.soldAmount > state.projects.get(input.indexOfProject).supplyOfToken)
		{
			return ;
		}
		
		  
		getCurrentDate(qpi, locals.curDate);

		if (locals.curDate <= state.projects.get(input.indexOfProject).endDate || state.projects.get(input.indexOfProject).numberOfYes <= state.projects.get(input.indexOfProject).numberOfNo || state.projects.get(input.indexOfProject).isCreatedFundarasing == 1)
		{
			return ;
		}

		if (input.tokenPrice * input.soldAmount < input.requiredFunds + div(input.requiredFunds * input.threshold, 100ULL))
		{
			return ;
		}
		
		if (qpi.invocationReward() < NOSTROMO_QX_TOKEN_ISSUANCE_FEE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if (qpi.invocationReward() > NOSTROMO_QX_TOKEN_ISSUANCE_FEE)
		{
			qpi.transfer(qpi.invocator(), NOSTROMO_QX_TOKEN_ISSUANCE_FEE - qpi.invocationReward());
		}
		
		locals.tmpProject = state.projects.get(input.indexOfProject);
		locals.tmpProject.isCreatedFundarasing = 1;
		state.projects.set(input.indexOfProject, locals.tmpProject);
		
		locals.newFundaraising.tokenPrice = input.tokenPrice;
		locals.newFundaraising.soldAmount = input.soldAmount;
		locals.newFundaraising.requiredFunds = input.requiredFunds;
		locals.newFundaraising.raisedFunds = 0;
		locals.newFundaraising.indexOfProject = input.indexOfProject;
		locals.newFundaraising.firstPhaseStartDate = locals.firstPhaseStartDate;
		locals.newFundaraising.firstPhaseEndDate = locals.firstPhaseEndDate;
		locals.newFundaraising.secondPhaseStartDate = locals.secondPhaseStartDate;
		locals.newFundaraising.secondPhaseEndDate = locals.secondPhaseEndDate;
		locals.newFundaraising.thirdPhaseStartDate = locals.thirdPhaseStartDate;
		locals.newFundaraising.thirdPhaseEndDate = locals.thirdPhaseEndDate;
		locals.newFundaraising.listingStartDate = locals.listingStartDate;
		locals.newFundaraising.cliffEndDate = locals.cliffEndDate;
		locals.newFundaraising.vestingEndDate = locals.vestingEndDate;
		locals.newFundaraising.threshold = input.threshold;
		locals.newFundaraising.TGE = input.TGE;
		locals.newFundaraising.stepOfVesting = input.stepOfVesting;

		state.fundarasings.set(state.numberOfFundaraising++, locals.newFundaraising);
	}

	struct investInProject_locals
	{
		QX::IssueAsset_input input;
		QX::IssueAsset_output output;
		QX::TransferShareManagementRights_input TransferShareManagementRightsInput;
		QX::TransferShareManagementRights_output TransferShareManagementRightsOutput;
		investInfo tmpInvestData;
		fundaraisingInfo tmpFundaraising;
		uint64 maxCap, minCap, maxInvestmentPerUser, userInvestedAmount;
		uint32 curDate, elementIndex, i, numberOfInvestors, numberOfClaimers, numberOfInvestedProjects;
		uint8 tierLevel;
		bit flag;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(investInProject)
	{
		if (input.indexOfFundaraising >= state.numberOfFundaraising || qpi.invocationReward() == 0)
		{
			return ;
		}

		locals.maxCap = state.fundarasings.get(input.indexOfFundaraising).requiredFunds + div(state.fundarasings.get(input.indexOfFundaraising).requiredFunds * state.fundarasings.get(input.indexOfFundaraising).threshold, 100ULL);
		if (state.fundarasings.get(input.indexOfFundaraising).raisedFunds >= locals.maxCap)
		{
			return ;
		}
		locals.flag = state.numberOfInvestedProjects.get(qpi.invocator(), locals.numberOfInvestedProjects);
		if (locals.flag && locals.numberOfInvestedProjects >= NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST)
		{
			return ;
		}
		
		getCurrentDate(qpi, locals.curDate);

		locals.tmpFundaraising = state.fundarasings.get(input.indexOfFundaraising);

		if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).firstPhaseStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).firstPhaseEndDate)
		{
			for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
			{
				if (state.Users.get(locals.i).userId == qpi.invocator())
				{
					break;
				}
			}
			if (locals.i == state.numberOfRegister)
			{
				return ;
			}
			
			locals.tierLevel = state.Users.get(locals.i).tierLevel;
			switch (locals.tierLevel)
			{
			case 1:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT, state.totalPoolWeight);
				break;
			case 2:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT, state.totalPoolWeight);
				break;
			case 3:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_DOG_POOL_WEIGHT, state.totalPoolWeight);
				break;
			case 4:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT, state.totalPoolWeight);
				break;
			case 5:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_WARRIOR_POOL_WEIGHT, state.totalPoolWeight);
				break;
			}

			state.investors.get(input.indexOfFundaraising, state.tmpInvestedList);
			locals.numberOfInvestors = state.numberOfInvestors.get(input.indexOfFundaraising);

			for (locals.i = 0; locals.i < locals.numberOfInvestors; locals.i++)
			{
				if (state.tmpInvestedList.get(locals.i).investorId == qpi.invocator())
				{
					locals.userInvestedAmount = state.tmpInvestedList.get(locals.i).investedAmount;
					break;
				}
			}
			
			locals.tmpInvestData.investorId = qpi.invocator();

			if (locals.i < locals.numberOfInvestors)
			{
				if (qpi.invocationReward() + locals.userInvestedAmount > locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.userInvestedAmount - locals.maxInvestmentPerUser);
					
					locals.tmpInvestData.investedAmount = locals.maxInvestmentPerUser;
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser - locals.userInvestedAmount;
				}
				else 
				{
					locals.tmpInvestData.investedAmount = qpi.invocationReward() + locals.userInvestedAmount;
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}
				state.tmpInvestedList.set(locals.i, locals.tmpInvestData);
				state.investors.replace(input.indexOfFundaraising, state.tmpInvestedList);
			}
			else 
			{
				if (qpi.invocationReward() > (sint64)locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.maxInvestmentPerUser);
					locals.tmpInvestData.investedAmount = locals.maxInvestmentPerUser;				
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser;
				}
				else 
				{
					locals.tmpInvestData.investedAmount = qpi.invocationReward();		
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}

				state.tmpInvestedList.set(locals.numberOfInvestors, locals.tmpInvestData);
				state.investors.replace(input.indexOfFundaraising, state.tmpInvestedList);
				state.numberOfInvestors.set(input.indexOfFundaraising, locals.numberOfInvestors + 1);
				locals.flag = state.numberOfInvestedProjects.get(qpi.invocator(), locals.numberOfInvestedProjects);
				if (locals.flag)
				{
					state.numberOfInvestedProjects.set(qpi.invocator(), locals.numberOfInvestedProjects + 1);
				}
				else 
				{
					state.numberOfInvestedProjects.set(qpi.invocator(), 1);
				}
			}
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).secondPhaseStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).secondPhaseEndDate)
		{
			for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
			{
				if (state.Users.get(locals.i).userId == qpi.invocator())
				{
					break;
				}
			}
			if (locals.i == state.numberOfRegister)
			{
				return ;
			}
			
			locals.tierLevel = state.Users.get(locals.i).tierLevel;
			if (locals.tierLevel < 4)
			{
				return ;
			}
			switch (locals.tierLevel)
			{
			case 4:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT, state.totalPoolWeight);
				break;
			case 5:
				locals.maxInvestmentPerUser = div(locals.maxCap * NOSTROMO_TIER_WARRIOR_POOL_WEIGHT, state.totalPoolWeight);
				break;
			}

			state.investors.get(input.indexOfFundaraising, state.tmpInvestedList);
			locals.numberOfInvestors = state.numberOfInvestors.get(input.indexOfFundaraising);

			for (locals.i = 0; locals.i < locals.numberOfInvestors; locals.i++)
			{
				if (state.tmpInvestedList.get(locals.i).investorId == qpi.invocator())
				{
					locals.userInvestedAmount = state.tmpInvestedList.get(locals.i).investedAmount;
					break;
				}
			}
			
			locals.tmpInvestData.investorId = qpi.invocator();

			if (locals.i < locals.numberOfInvestors)
			{
				if (qpi.invocationReward() + locals.userInvestedAmount > locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.userInvestedAmount - locals.maxInvestmentPerUser);
					
					locals.tmpInvestData.investedAmount = locals.maxInvestmentPerUser;
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser - locals.userInvestedAmount;
				}
				else 
				{
					locals.tmpInvestData.investedAmount = qpi.invocationReward() + locals.userInvestedAmount;
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}
				state.tmpInvestedList.set(locals.i, locals.tmpInvestData);
				state.investors.replace(input.indexOfFundaraising, state.tmpInvestedList);
			}
			else 
			{
				if (qpi.invocationReward() > (sint64)locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.maxInvestmentPerUser);
					locals.tmpInvestData.investedAmount = locals.maxInvestmentPerUser;				
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser;
				}
				else 
				{
					locals.tmpInvestData.investedAmount = qpi.invocationReward();		
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}

				state.tmpInvestedList.set(locals.numberOfInvestors, locals.tmpInvestData);
				state.investors.replace(input.indexOfFundaraising, state.tmpInvestedList);
				state.numberOfInvestors.set(input.indexOfFundaraising, locals.numberOfInvestors + 1);
				locals.flag = state.numberOfInvestedProjects.get(qpi.invocator(), locals.numberOfInvestedProjects);
				if (locals.flag)
				{
					state.numberOfInvestedProjects.set(qpi.invocator(), locals.numberOfInvestedProjects + 1);
				}
				else 
				{
					state.numberOfInvestedProjects.set(qpi.invocator(), 1);
				}
			}
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).thirdPhaseStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).thirdPhaseEndDate)
		{
			state.investors.get(input.indexOfFundaraising, state.tmpInvestedList);
			locals.numberOfInvestors = state.numberOfInvestors.get(input.indexOfFundaraising);

			for (locals.i = 0; locals.i < locals.numberOfInvestors; locals.i++)
			{
				if (state.tmpInvestedList.get(locals.i).investorId == qpi.invocator())
				{
					locals.userInvestedAmount = state.tmpInvestedList.get(locals.i).investedAmount;
					break;
				}
			}

			locals.tmpInvestData.investorId = qpi.invocator();

			if (locals.i < locals.numberOfInvestors)
			{
				locals.tmpInvestData.investedAmount = qpi.invocationReward() + locals.userInvestedAmount;
				state.tmpInvestedList.set(locals.i, locals.tmpInvestData);
				state.investors.replace(input.indexOfFundaraising, state.tmpInvestedList);
			}
			else 
			{
				locals.tmpInvestData.investedAmount = qpi.invocationReward();

				state.tmpInvestedList.set(locals.numberOfInvestors, locals.tmpInvestData);
				state.investors.replace(input.indexOfFundaraising, state.tmpInvestedList);
				state.numberOfInvestors.set(input.indexOfFundaraising, locals.numberOfInvestors + 1);

				locals.flag = state.numberOfInvestedProjects.get(qpi.invocator(), locals.numberOfInvestedProjects);
				if (locals.flag)
				{
					state.numberOfInvestedProjects.set(qpi.invocator(), locals.numberOfInvestedProjects + 1);
				}
				else 
				{
					state.numberOfInvestedProjects.set(qpi.invocator(), 1);
				}
			}
			locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
		}

		if (locals.minCap <= locals.tmpFundaraising.raisedFunds && locals.tmpFundaraising.isCreatedToken == 0)
		{
			locals.input.assetName = state.projects.get(locals.tmpFundaraising.indexOfProject).tokenName;
			locals.input.numberOfDecimalPlaces = 0;
			locals.input.numberOfShares = state.projects.get(locals.tmpFundaraising.indexOfProject).supplyOfToken;
			locals.input.unitOfMeasurement = 0;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, IssueAsset, locals.input, locals.output, NOSTROMO_QX_TOKEN_ISSUANCE_FEE);

			if (locals.output.issuedNumberOfShares == state.projects.get(locals.tmpFundaraising.indexOfProject).supplyOfToken)
			{
				locals.tmpFundaraising.isCreatedToken = 1;

				locals.TransferShareManagementRightsInput.asset.assetName = state.projects.get(state.fundarasings.get(input.indexOfFundaraising).indexOfProject).tokenName;
				locals.TransferShareManagementRightsInput.asset.issuer = SELF;
				locals.TransferShareManagementRightsInput.newManagingContractIndex = SELF_INDEX;
				locals.TransferShareManagementRightsInput.numberOfShares = state.projects.get(state.fundarasings.get(input.indexOfFundaraising).indexOfProject).supplyOfToken;

				INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.TransferShareManagementRightsInput, locals.TransferShareManagementRightsOutput, 0);

				qpi.transferShareOwnershipAndPossession(state.projects.get(locals.tmpFundaraising.indexOfProject).tokenName, SELF, SELF, SELF, state.projects.get(locals.tmpFundaraising.indexOfProject).supplyOfToken - state.fundarasings.get(input.indexOfFundaraising).soldAmount, state.projects.get(locals.tmpFundaraising.indexOfProject).creator);
			}
		}

		state.fundarasings.set(input.indexOfFundaraising, locals.tmpFundaraising);
		
	}

	struct claimToken_locals
	{
		investInfo tmpInvestData;
		claimInfo tmpClaimData;
		uint64 maxClaimAmount, investedAmount, dayA, dayB, start_cur_diffSecond, cur_end_diffSecond, claimedAmount;
		uint32 curDate, tmpDate, numberOfInvestors, numberOfClaimedProjects, numberOfInvestedProjects;
		sint32 i, j;
		uint8 curVestingStep, vestingPercent;
		bit flag;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(claimToken)
	{
		getCurrentDate(qpi, locals.curDate);

		state.investors.get(input.indexOfFundaraising, state.tmpInvestedList);
		locals.numberOfInvestors = state.numberOfInvestors.get(input.indexOfFundaraising);

		for (locals.i = 0; locals.i < (sint32)locals.numberOfInvestors; locals.i++)
		{
			if (state.tmpInvestedList.get(locals.i).investorId == qpi.invocator())
			{
				locals.investedAmount = state.tmpInvestedList.get(locals.i).investedAmount;
				break;
			}
		}

		if (locals.i == locals.numberOfInvestors)
		{
			return ;
		}

		if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).listingStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).cliffEndDate)
		{
			locals.maxClaimAmount = div(div(locals.investedAmount, state.fundarasings.get(input.indexOfFundaraising).tokenPrice) * state.fundarasings.get(input.indexOfFundaraising).TGE, 100ULL);
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).cliffEndDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).vestingEndDate)
		{
			locals.tmpDate = state.fundarasings.get(input.indexOfFundaraising).cliffEndDate;
			QUOTTERY::diffDateInSecond(locals.tmpDate, locals.curDate, locals.i, locals.dayA, locals.dayB, locals.start_cur_diffSecond);
			locals.tmpDate = state.fundarasings.get(input.indexOfFundaraising).vestingEndDate;
			QUOTTERY::diffDateInSecond(locals.curDate, locals.tmpDate, locals.i, locals.dayA, locals.dayB, locals.cur_end_diffSecond);

			locals.curVestingStep = (uint8)div(locals.start_cur_diffSecond, div(locals.start_cur_diffSecond + locals.cur_end_diffSecond, state.fundarasings.get(input.indexOfFundaraising).stepOfVesting * 1ULL));
			locals.vestingPercent = (uint8)div(100ULL - state.fundarasings.get(input.indexOfFundaraising).TGE, state.fundarasings.get(input.indexOfFundaraising).stepOfVesting * 1ULL) * locals.curVestingStep;
			locals.maxClaimAmount = div(div(locals.investedAmount, state.fundarasings.get(input.indexOfFundaraising).tokenPrice) * (state.fundarasings.get(input.indexOfFundaraising).TGE + locals.vestingPercent), 100ULL);
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).vestingEndDate)
		{
			locals.maxClaimAmount = div(locals.investedAmount, state.fundarasings.get(input.indexOfFundaraising).tokenPrice);
		}

		state.claimers.get(qpi.invocator(), state.tmpClaimedList);
		locals.flag = state.numberOfClaimedProjects.get(qpi.invocator(), locals.numberOfClaimedProjects);

		for (locals.j = 0; locals.j < (sint32)locals.numberOfClaimedProjects; locals.j++)
		{
			if (state.tmpClaimedList.get(locals.j).indexOfFundaraising == input.indexOfFundaraising)
			{
				locals.claimedAmount = state.tmpClaimedList.get(locals.j).claimedAmount;
				break;
			}
		}

		locals.tmpClaimData.indexOfFundaraising = input.indexOfFundaraising;
		if (locals.flag && locals.j < (sint32)locals.numberOfClaimedProjects)
		{
			if (input.amount + locals.claimedAmount > locals.maxClaimAmount)
			{
				return ;
			}
			else 
			{
				qpi.transferShareOwnershipAndPossession(state.projects.get(state.fundarasings.get(input.indexOfFundaraising).indexOfProject).tokenName, SELF, SELF, SELF, input.amount, qpi.invocator());
				if (input.amount + locals.claimedAmount == locals.maxClaimAmount && locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).vestingEndDate)
				{
					state.tmpClaimedList.set(locals.j, state.tmpClaimedList.get(locals.numberOfClaimedProjects - 1));
					state.numberOfClaimedProjects.set(qpi.invocator(), locals.numberOfClaimedProjects - 1);
					state.numberOfInvestedProjects.get(qpi.invocator(), locals.numberOfInvestedProjects);
					state.numberOfInvestedProjects.set(qpi.invocator(), locals.numberOfInvestedProjects - 1);

					state.investors.get(input.indexOfFundaraising, state.tmpInvestedList);
					state.tmpInvestedList.set(locals.i, state.tmpInvestedList.get(locals.numberOfInvestors - 1));
					state.investors.set(input.indexOfFundaraising, state.tmpInvestedList);
					state.numberOfInvestors.set(input.indexOfFundaraising, locals.numberOfInvestors - 1);
				}
				else 
				{
					locals.tmpClaimData.claimedAmount = input.amount + locals.claimedAmount;
					state.tmpClaimedList.set(locals.j, locals.tmpClaimData);
				}
				state.claimers.replace(qpi.invocator(), state.tmpClaimedList);
			}
		}
		else 
		{
			if (input.amount > locals.maxClaimAmount)
			{
				return ;
			}
			else 
			{
				qpi.transferShareOwnershipAndPossession(state.projects.get(state.fundarasings.get(input.indexOfFundaraising).indexOfProject).tokenName, SELF, SELF, SELF, input.amount, qpi.invocator());
				if (input.amount == locals.maxClaimAmount && locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).vestingEndDate)
				{
					state.tmpClaimedList.set(locals.j, state.tmpClaimedList.get(locals.numberOfClaimedProjects - 1));
					state.numberOfClaimedProjects.set(qpi.invocator(), locals.numberOfClaimedProjects - 1);
					state.numberOfInvestedProjects.get(qpi.invocator(), locals.numberOfInvestedProjects);
					state.numberOfInvestedProjects.set(qpi.invocator(), locals.numberOfInvestedProjects - 1);

					state.investors.get(input.indexOfFundaraising, state.tmpInvestedList);
					state.tmpInvestedList.set(locals.i, state.tmpInvestedList.get(locals.numberOfInvestors - 1));
					state.investors.set(input.indexOfFundaraising, state.tmpInvestedList);
					state.numberOfInvestors.set(input.indexOfFundaraising, locals.numberOfInvestors - 1);
				}
				else 
				{
					locals.tmpClaimData.claimedAmount = input.amount;
					state.tmpClaimedList.set(locals.j, locals.tmpClaimData);
				}
				state.claimers.replace(qpi.invocator(), state.tmpClaimedList);
			}
		}
	}

	struct upgradeTier_locals
	{
		userInfo user;
		uint32 i;
		uint8 currentTierLevel;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(upgradeTier)
	{
		for ( locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			if (state.Users.get(locals.i).userId == qpi.invocator())
			{
				locals.currentTierLevel = state.Users.get(locals.i).tierLevel;
				break;
			}
		}
		
		switch (locals.currentTierLevel)
		{
			case 1:
				if (input.newTierLevel != 2 || qpi.invocationReward() < NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT - NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}
				else 
				{
					locals.user.userId = qpi.invocator();
					locals.user.tierLevel = input.newTierLevel;
					state.Users.set(locals.i, locals.user);
					if (qpi.invocationReward() > NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT - NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward() - (NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT - NOSTROMO_TIER_FACEHUGGER_STAKE_AMOUNT));
					}
					state.totalPoolWeight += NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT - NOSTROMO_TIER_FACEHUGGER_POOL_WEIGHT;
				}
				break;
			case 2:
				if (input.newTierLevel != 3 || qpi.invocationReward() < NOSTROMO_TIER_DOG_STAKE_AMOUNT - NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}
				else 
				{
					locals.user.userId = qpi.invocator();
					locals.user.tierLevel = input.newTierLevel;
					state.Users.set(locals.i, locals.user);
					if (qpi.invocationReward() > NOSTROMO_TIER_DOG_STAKE_AMOUNT - NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward() - (NOSTROMO_TIER_DOG_STAKE_AMOUNT - NOSTROMO_TIER_CHESTBURST_STAKE_AMOUNT));
					}
					state.totalPoolWeight += NOSTROMO_TIER_DOG_POOL_WEIGHT - NOSTROMO_TIER_CHESTBURST_POOL_WEIGHT;
				}
				break;
			case 3:
				if (input.newTierLevel != 4 || qpi.invocationReward() < NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT - NOSTROMO_TIER_DOG_STAKE_AMOUNT)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}
				else 
				{
					locals.user.userId = qpi.invocator();
					locals.user.tierLevel = input.newTierLevel;
					state.Users.set(locals.i, locals.user);
					if (qpi.invocationReward() > NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT - NOSTROMO_TIER_DOG_STAKE_AMOUNT)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward() - (NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT - NOSTROMO_TIER_DOG_STAKE_AMOUNT));
					}
					state.totalPoolWeight += NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT - NOSTROMO_TIER_DOG_POOL_WEIGHT;
				}
				break;
			case 4:
				if (input.newTierLevel != 5 || qpi.invocationReward() < NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT - NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}
				else 
				{
					locals.user.userId = qpi.invocator();
					locals.user.tierLevel = input.newTierLevel;
					state.Users.set(locals.i, locals.user);
					if (qpi.invocationReward() > NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT - NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward() - (NOSTROMO_TIER_WARRIOR_STAKE_AMOUNT - NOSTROMO_TIER_XENOMORPH_STAKE_AMOUNT));
					}
					state.totalPoolWeight += NOSTROMO_TIER_WARRIOR_POOL_WEIGHT - NOSTROMO_TIER_XENOMORPH_POOL_WEIGHT;
				}
				break;
		}
	}

	PUBLIC_FUNCTION(getStats)
	{
		output.epochRevenue = state.epochRevenue;
		output.numberOfCreatedProject = state.numberOfCreatedProject;
		output.numberOfFundaraising = state.numberOfFundaraising;
		output.numberOfRegister = state.numberOfRegister;
		output.totalPoolWeight = state.totalPoolWeight;
	}

	struct getTierLevelByUser_locals
	{
		uint32 i;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getTierLevelByUser)
	{
		for (locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			if (input.userId == state.Users.get(locals.i).userId)
			{
				output.tierLevel = state.Users.get(locals.i).tierLevel;
				return ;
			}
		}
	}

	PUBLIC_FUNCTION(getUserVoteStatus)
	{
		state.numberOfVotedProject.get(input.userId, output.numberOfVotedProjects);
		state.voteStatus.get(input.userId, output.projectIndexList);
	}

	PUBLIC_FUNCTION(checkTokenCreatability)
	{
		output.result = state.tokens.contains(input.tokenName);
	}

	PUBLIC_FUNCTION(getNumberOfInvestedAndClaimedProjects)
	{
		state.numberOfInvestedProjects.get(input.userId, output.numberOfInvestedProjects);
		state.numberOfClaimedProjects.get(input.userId, output.numberOfClaimedProjects);
	}

public:
	struct getProjectByIndex_input
	{
		uint32 indexOfProject;
	};

	struct getProjectByIndex_output
	{
		projectInfo project;
	};

	PUBLIC_FUNCTION(getProjectByIndex)
	{
		output.project = state.projects.get(input.indexOfProject);
	}

	struct getFundarasingByIndex_input
	{
		uint32 indexOfFundarasing;
	};

	struct getFundarasingByIndex_output
	{
		fundaraisingInfo fundarasing;
	};

	PUBLIC_FUNCTION(getFundarasingByIndex)
	{
		output.fundarasing = state.fundarasings.get(input.indexOfFundarasing);
	}

	struct getProjectIndexListByCreator_input
	{
		id creator;
	};

	struct getProjectIndexListByCreator_output
	{
		Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST> indexListForProjects;
	};

	struct getProjectIndexListByCreator_locals
	{
		uint32 i, countOfProject;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getProjectIndexListByCreator)
	{
		for (locals.i = 0; locals.i < state.numberOfCreatedProject; locals.i++)
		{
			if (state.projects.get(locals.i).creator == input.creator)
			{
				output.indexListForProjects.set(locals.countOfProject++, locals.i);
			}
		}
	}

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(getStats, 1);
		REGISTER_USER_FUNCTION(getTierLevelByUser, 2);
		REGISTER_USER_FUNCTION(getUserVoteStatus, 3);
		REGISTER_USER_FUNCTION(checkTokenCreatability, 4);
		REGISTER_USER_FUNCTION(getNumberOfInvestedAndClaimedProjects, 5);
		REGISTER_USER_FUNCTION(getProjectByIndex, 6);
		REGISTER_USER_FUNCTION(getFundarasingByIndex, 7);
		REGISTER_USER_FUNCTION(getProjectIndexListByCreator, 8);
		
		REGISTER_USER_PROCEDURE(registerInTier, 1);
		REGISTER_USER_PROCEDURE(logoutFromTier, 2);
		REGISTER_USER_PROCEDURE(createProject, 3);
		REGISTER_USER_PROCEDURE(voteInProject, 4);
		REGISTER_USER_PROCEDURE(createFundaraising, 5);
		REGISTER_USER_PROCEDURE(investInProject, 6);
		REGISTER_USER_PROCEDURE(claimToken, 7);
		REGISTER_USER_PROCEDURE(upgradeTier, 8);
	}

	struct END_EPOCH_locals
	{
		fundaraisingInfo tmpFundaraising;
		userInfo user;
		investInfo tmpInvest;
		Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST> votedList;
		Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST> clearedVotedList;
		uint32 numberOfVotedProject, clearedNumberOfVotedProject, i, j, curDate, indexOfProject, numberOfInvestors, numberOfInvestedProjects;
	};

	END_EPOCH_WITH_LOCALS()
	{
		getCurrentDate(qpi, locals.curDate);

		for (locals.i = 0; locals.i < state.numberOfFundaraising; locals.i++)
		{
			if (state.fundarasings.get(locals.i).thirdPhaseEndDate < locals.curDate && state.fundarasings.get(locals.i).isCreatedToken == 0 && state.fundarasings.get(locals.i).raisedFunds != 0)
			{
				locals.tmpFundaraising = state.fundarasings.get(locals.i);
				locals.tmpFundaraising.raisedFunds = 0;
				state.fundarasings.set(locals.i, locals.tmpFundaraising);
				
				state.investors.get(locals.i, state.tmpInvestedList);
				locals.numberOfInvestors = state.numberOfInvestors.get(locals.i);

				for (locals.j = 0; locals.j < locals.numberOfInvestors; locals.j++)
				{
					qpi.transfer(state.tmpInvestedList.get(locals.j).investorId, state.tmpInvestedList.get(locals.j).investedAmount);
					state.tmpInvestedList.set(locals.j, locals.tmpInvest);
					state.numberOfInvestedProjects.get(state.tmpInvestedList.get(locals.j).investorId, locals.numberOfInvestedProjects);
					state.numberOfInvestedProjects.set(state.tmpInvestedList.get(locals.j).investorId, locals.numberOfInvestedProjects - 1);
				}

				state.investors.set(locals.i, state.tmpInvestedList);
				state.numberOfInvestors.set(locals.i, 0);
			}
			else if (state.fundarasings.get(locals.i).thirdPhaseEndDate < locals.curDate && state.fundarasings.get(locals.i).isCreatedToken == 1 && state.fundarasings.get(locals.i).raisedFunds != 0)
			{
				locals.tmpFundaraising = state.fundarasings.get(locals.i);
				locals.tmpFundaraising.raisedFunds = 0;
				state.fundarasings.set(locals.i, locals.tmpFundaraising);

				state.epochRevenue += div(state.fundarasings.get(locals.i).raisedFunds * 5, 100ULL);
				qpi.transfer(state.projects.get(state.fundarasings.get(locals.i).indexOfProject).creator, state.fundarasings.get(locals.i).raisedFunds - div(state.fundarasings.get(locals.i).raisedFunds * 5, 100ULL));
			}
		}

		qpi.distributeDividends(div(state.epochRevenue, 676ULL));
		state.epochRevenue = 0;
		
		for (locals.i = 0; locals.i < state.numberOfRegister; locals.i++)
		{
			locals.user = state.Users.get(locals.i);
			state.numberOfVotedProject.get(locals.user.userId, locals.numberOfVotedProject);
			state.voteStatus.get(locals.user.userId, locals.votedList);

			for (locals.j = 0; locals.j < locals.numberOfVotedProject; locals.j++)
			{
				locals.indexOfProject = locals.votedList.get(locals.j);

				if (state.projects.get(locals.indexOfProject).endDate > locals.curDate)
				{
					locals.clearedVotedList.set(locals.clearedNumberOfVotedProject++, locals.indexOfProject);
				}
			}

			state.numberOfVotedProject.set(locals.user.userId, locals.clearedNumberOfVotedProject);
			state.voteStatus.set(locals.user.userId, locals.clearedVotedList);
		}
	}
};