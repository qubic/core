using namespace QPI;

struct TESTEXD2
{
};

struct TESTEXD : public ContractBase
{
	struct END_TICK_locals
	{
		OI::Price::OracleQuery priceOracleQuery;
		uint64 oracleQueryId;
		Entity entity;
		sint64 balance;
	};

	END_TICK_WITH_LOCALS()
	{
		// Query oracle
		if (qpi.tick() % 5 == 0)
		{
			locals.oracleQueryId = qpi.queryOracle<OI::Price>(locals.priceOracleQuery, 20);
		}

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
