#define NO_UEFI

#include "oracle_testing.h"


struct OracleEngineTest : public OracleEngine, LoggingTest
{
	OracleEngineTest()
	{
		EXPECT_TRUE(init());
		EXPECT_TRUE(initCommonBuffers());
	}

	~OracleEngineTest()
	{
		deinitCommonBuffers();
		deinit();
	}
};

static void dummyNotificationProc(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals)
{
}

TEST(OracleEngine, ContractQuery)
{
	OracleEngineTest oracleEngine;

	system.tick = 1000;
	etalonTick.year = 25;
	etalonTick.month = 12;
	etalonTick.day = 15;
	etalonTick.hour = 16;
	etalonTick.minute = 51;
	etalonTick.second = 12;

	OI::Price::OracleQuery priceQuery;
	priceQuery.oracle = m256i(1, 2, 3, 4);
	priceQuery.currency1 = m256i(2, 3, 4, 5);
	priceQuery.currency2 = m256i(3, 4, 5, 6);
	priceQuery.timestamp = QPI::DateAndTime::now();
	QPI::uint32 interfaceIndex = 0;
	QPI::uint16 contractIndex = 1;
	QPI::uint32 timeout = 30000;
	USER_PROCEDURE notificationProc = dummyNotificationProc;
	QPI::uint32 notificationLocalsSize = 128;

	//-------------------------------------------------------------------------
	// start contract query / check message to OM node
	QPI::sint64 queryId = oracleEngine.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProc, notificationLocalsSize);
	EXPECT_EQ(queryId, getContractOracleQueryId(system.tick, 0));
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, priceQuery.oracle, timeout);

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
	EXPECT_EQ(memcmp(&priceQueryReturned, &priceQuery, sizeof(priceQuery)), 0);

	//-------------------------------------------------------------------------
	// process simulated reply from OM node
	struct
	{
		OracleMachineReply metatdata;
		OI::Price::OracleReply data;
	} priceOracleMachineReply;

	priceOracleMachineReply.metatdata.oracleMachineErrorFlags = 0;
	priceOracleMachineReply.metatdata.oracleQueryId = queryId;
	priceOracleMachineReply.data.numerator = 1234;
	priceOracleMachineReply.data.denominator = 1;

	oracleEngine.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));

	// duplicate from other node
	oracleEngine.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));

	// other value from other node
	priceOracleMachineReply.data.numerator = 1233;
	oracleEngine.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));
}