using namespace QPI;

struct TESTEXD2
{
};

struct TESTEXD : public ContractBase
{
	struct END_TICK_locals
	{
		Entity entity;
		sint64 balance;
	};

	END_TICK_WITH_LOCALS()
	{
		// Distribute balance to sharesholders at the end of each tick
		qpi.getEntity(SELF, locals.entity);
		locals.balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
		if (locals.balance > NUMBER_OF_COMPUTORS)
		{
			qpi.distributeDividends(div<sint64>(locals.balance, NUMBER_OF_COMPUTORS));
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
	}
};
