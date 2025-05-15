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
constexpr uint32 NOSTROMO_MAX_NUMBER_PROJECT = 2097152;
constexpr uint32 NOSTROMO_MAX_NUMBER_TOKEN = 65536;

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

protected:

	HashMap<id, uint8, NOSTROMO_MAX_USER> Users;

	struct voteInfo
	{
		id user;
		uint32 indexOfProject;
	};

	HashMap<voteInfo, bit, NOSTROMO_MAX_USER * 64> voteStatus;
	HashMap<uint64, bit, NOSTROMO_MAX_NUMBER_TOKEN> tokens;

	struct investInfo
	{
		id user;
		uint32 indexOfFundaraising;
	};

	HashMap<investInfo, uint64, NOSTROMO_MAX_USER * 64> investors;
	HashMap<investInfo, uint64, NOSTROMO_MAX_USER * 64> claimers;

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

	uint64 epochRevenue;
	uint64 totalPoolWeight;
	uint32 numberOfRegister;
	uint32 numberOfCreatedProject;
	uint32 numberOfFundaraising;

	/**
	* @return Current date from core node system
	*/

	inline static void getCurrentDate(const QPI::QpiContextFunctionCall& qpi, uint32& res) 
	{
        QUOTTERY::packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), res);
    }

	PUBLIC_PROCEDURE(registerInTier)
	{
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
				state.Users.set(qpi.invocator(), input.tierLevel);
				state.numberOfRegister++;
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
				state.Users.set(qpi.invocator(), input.tierLevel);
				state.numberOfRegister++;
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
				state.Users.set(qpi.invocator(), input.tierLevel);
				state.numberOfRegister++;
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
				state.Users.set(qpi.invocator(), input.tierLevel);
				state.numberOfRegister++;
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
				state.Users.set(qpi.invocator(), input.tierLevel);
				state.numberOfRegister++;
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
		uint32 elementIndex;
		uint8 tierLevel;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(logoutFromTier)
	{
		locals.elementIndex = state.Users.getElementIndex(qpi.invocator());
		if (state.Users.key(locals.elementIndex) == qpi.invocator())
		{
			locals.tierLevel = state.Users.value(locals.elementIndex);
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

			state.numberOfRegister--;
			state.Users.removeByKey(qpi.invocator());
			output.result = 1;
		}
	}

	struct createProject_locals
	{
		projectInfo newProject;
		uint32 elementIndex, startDate, endDate, curDate;
		uint8 tierLevel;
		bit flag;
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

		state.tokens.get(input.tokenName, locals.flag);
		if (locals.flag)
		{
			output.indexOfProject = NOSTROMO_MAX_NUMBER_PROJECT;
			return ;
		}

		locals.elementIndex = state.Users.getElementIndex(qpi.invocator());
		locals.tierLevel = state.Users.value(locals.elementIndex);

		if (state.Users.key(locals.elementIndex) == qpi.invocator() && (locals.tierLevel == 4 || locals.tierLevel == 5))
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
			state.tokens.set(input.tokenName, 1);
		}
		else 
		{
			output.indexOfProject = NOSTROMO_MAX_NUMBER_PROJECT;
		}
	}

	struct voteInProject_locals
	{
		voteInfo newVote;
		projectInfo votedProject;
		uint32 elementIndex, curDate;
		bit flag;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(voteInProject)
	{
		if (input.indexOfProject >= state.numberOfCreatedProject)
		{
			return ;
		}
		locals.elementIndex = state.Users.getElementIndex(qpi.invocator());
		if (state.Users.key(locals.elementIndex) != qpi.invocator())
		{
			return ;
		}
		locals.newVote.indexOfProject = input.indexOfProject;
		locals.newVote.user = qpi.invocator();

		state.voteStatus.get(locals.newVote, locals.flag);
		if (locals.flag)
		{
			return ;
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
			state.voteStatus.set(locals.newVote, 1); //// it should be initialized at the end of every epoch!!!!!!!!!!!!!!!!!!!
			state.projects.set(input.indexOfProject, locals.votedProject);
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
		investInfo newInvestor;
		fundaraisingInfo tmpFundaraising;
		uint64 maxCap, minCap, maxInvestmentPerUser, userInvestedAmount;
		uint32 curDate, elementIndex;
		uint8 tierLevel;
		bit flag;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(investInProject)
	{
		if (input.indexOfFundaraising >= state.numberOfFundaraising || qpi.invocationReward() == 0)
		{
			return ;
		}
		if (state.fundarasings.get(input.indexOfFundaraising).raisedFunds >= locals.maxCap)
		{
			return ;
		}
		
		getCurrentDate(qpi, locals.curDate);

		locals.maxCap = state.fundarasings.get(input.indexOfFundaraising).requiredFunds + div(state.fundarasings.get(input.indexOfFundaraising).requiredFunds * state.fundarasings.get(input.indexOfFundaraising).threshold, 100ULL);
		locals.newInvestor.indexOfFundaraising = input.indexOfFundaraising;
		locals.newInvestor.user = qpi.invocator();

		locals.tmpFundaraising = state.fundarasings.get(input.indexOfFundaraising);

		if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).firstPhaseStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).firstPhaseEndDate)
		{
			locals.elementIndex = state.Users.getElementIndex(qpi.invocator());
			if (state.Users.key(locals.elementIndex) != qpi.invocator())
			{
				return ;
			}			
			state.Users.get(qpi.invocator(), locals.tierLevel);
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

			if (state.investors.get(locals.newInvestor, locals.userInvestedAmount))
			{
				if (qpi.invocationReward() + locals.userInvestedAmount > locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.userInvestedAmount - locals.maxInvestmentPerUser);
					state.investors.replace(locals.newInvestor, locals.maxInvestmentPerUser);
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser - locals.userInvestedAmount;
				}
				else 
				{
					state.investors.replace(locals.newInvestor, qpi.invocationReward() + locals.userInvestedAmount);
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}
			}
			else 
			{
				if (qpi.invocationReward() > locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.maxInvestmentPerUser);
					state.investors.replace(locals.newInvestor, locals.maxInvestmentPerUser);
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser;
				}
				else 
				{
					state.investors.replace(locals.newInvestor, qpi.invocationReward());
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}
			}
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).secondPhaseStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).secondPhaseEndDate)
		{
			locals.elementIndex = state.Users.getElementIndex(qpi.invocator());
			if (state.Users.key(locals.elementIndex) != qpi.invocator())
			{
				return ;
			}
			state.Users.get(qpi.invocator(), locals.tierLevel);
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

			if (state.investors.get(locals.newInvestor, locals.userInvestedAmount))
			{
				if (qpi.invocationReward() + locals.userInvestedAmount > locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.userInvestedAmount - locals.maxInvestmentPerUser);
					state.investors.replace(locals.newInvestor, locals.maxInvestmentPerUser);
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser - locals.userInvestedAmount;
				}
				else 
				{
					state.investors.replace(locals.newInvestor, qpi.invocationReward() + locals.userInvestedAmount);
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}
			}
			else 
			{
				if (qpi.invocationReward() > locals.maxInvestmentPerUser)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.maxInvestmentPerUser);
					state.investors.replace(locals.newInvestor, locals.maxInvestmentPerUser);
					locals.tmpFundaraising.raisedFunds += locals.maxInvestmentPerUser;
				}
				else 
				{
					state.investors.replace(locals.newInvestor, qpi.invocationReward());
					locals.tmpFundaraising.raisedFunds += qpi.invocationReward();
				}
			}
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).thirdPhaseStartDate && locals.curDate < state.fundarasings.get(input.indexOfFundaraising).thirdPhaseEndDate)
		{
			if (state.investors.get(locals.newInvestor, locals.userInvestedAmount))
			{
				state.investors.replace(locals.newInvestor, qpi.invocationReward() + locals.userInvestedAmount);
			}
			else 
			{
				state.investors.replace(locals.newInvestor, qpi.invocationReward());
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

			}
		}

		state.fundarasings.set(input.indexOfFundaraising, locals.tmpFundaraising);
		
	}

	struct claimToken_locals
	{
		investInfo user;
		uint64 maxClaimAmount, investedAmount, dayA, dayB, start_cur_diffSecond, cur_end_diffSecond, claimedAmount;
		uint32 curDate, tmpDate;
		sint32 i;
		uint8 curVestingStep, vestingPercent;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(claimToken)
	{
		getCurrentDate(qpi, locals.curDate);

		locals.user.indexOfFundaraising = input.indexOfFundaraising;
		locals.user.user = qpi.invocator();

		if (state.investors.get(locals.user, locals.investedAmount) == 0)
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
			locals.vestingPercent = div(100ULL - state.fundarasings.get(input.indexOfFundaraising).TGE, state.fundarasings.get(input.indexOfFundaraising).stepOfVesting * 1ULL) * locals.curVestingStep;
			locals.maxClaimAmount = div(div(locals.investedAmount, state.fundarasings.get(input.indexOfFundaraising).tokenPrice) * (state.fundarasings.get(input.indexOfFundaraising).TGE + locals.vestingPercent), 100ULL);
		}
		else if (locals.curDate >= state.fundarasings.get(input.indexOfFundaraising).vestingEndDate)
		{
			locals.maxClaimAmount = div(locals.investedAmount, state.fundarasings.get(input.indexOfFundaraising).tokenPrice);
		}		
		if (state.claimers.get(locals.user, locals.claimedAmount))
		{
			if (input.amount + locals.claimedAmount > locals.maxClaimAmount)
			{
				return ;
			}
			else 
			{
				qpi.transferShareOwnershipAndPossession(state.projects.get(state.fundarasings.get(input.indexOfFundaraising).indexOfProject).tokenName, SELF, SELF, SELF, input.amount, qpi.invocator());
				state.claimers.set(locals.user, input.amount + locals.claimedAmount);
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
				state.claimers.set(locals.user, input.amount);
			}
		}
		
	}

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(registerInTier, 1);
		REGISTER_USER_PROCEDURE(logoutFromTier, 2);
		REGISTER_USER_PROCEDURE(createProject, 3);
		REGISTER_USER_PROCEDURE(voteInProject, 4);
		REGISTER_USER_PROCEDURE(createFundaraising, 5);
		REGISTER_USER_PROCEDURE(investInProject, 6);
		REGISTER_USER_PROCEDURE(claimToken, 7);
	}
};