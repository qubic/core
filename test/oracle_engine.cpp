#define NO_UEFI

#include "oracle_testing.h"


struct OracleEngineTest : public LoggingTest
{
	OracleEngineTest()
	{
		EXPECT_TRUE(initCommonBuffers());
		EXPECT_TRUE(initSpecialEntities());
		EXPECT_TRUE(initContractExec());
		EXPECT_TRUE(ts.init());

		// init computors
		for (int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
		{
			broadcastedComputors.computors.publicKeys[computorIndex] = m256i(computorIndex * 2, 42, 13, 1337);
		}

		// setup tick and time
		system.tick = 1000;
		etalonTick.year = 25;
		etalonTick.month = 12;
		etalonTick.day = 15;
		etalonTick.hour = 16;
		etalonTick.minute = 51;
		etalonTick.second = 12;
		ts.beginEpoch(system.tick);
	}

	~OracleEngineTest()
	{
		deinitCommonBuffers();
		deinitContractExec();
		ts.deinit();
	}
};

template<uint16_t ownComputorSeedsCount>
struct OracleEngineWithInitAndDeinit : public OracleEngine<ownComputorSeedsCount>
{
	OracleEngineWithInitAndDeinit(const m256i* ownComputorPublicKeys)
	{
		this->init(ownComputorPublicKeys);
	}

	~OracleEngineWithInitAndDeinit()
	{
		this->deinit();
	}

	void checkPendingState(int64_t queryId, uint16_t totalCommitTxExecuted, uint16_t ownCommitTxExecuted, uint8_t expectedStatus) const
	{
		uint32_t queryIndex;
		EXPECT_TRUE(this->queryIdToIndex->get(queryId, queryIndex));
		EXPECT_LT(queryIndex, this->oracleQueryCount);
		const OracleQueryMetadata& oqm = this->queries[queryIndex];
		EXPECT_EQ(oqm.status, expectedStatus);
		EXPECT_TRUE(oqm.status == ORACLE_QUERY_STATUS_PENDING || oqm.status == ORACLE_QUERY_STATUS_COMMITTED);
		const OracleReplyState<ownComputorSeedsCount>& replyState = this->replyStates[oqm.statusVar.pending.replyStateIndex];
		EXPECT_EQ((int)totalCommitTxExecuted, (int)replyState.totalCommits);
		EXPECT_EQ((int)ownCommitTxExecuted, (int)replyState.ownReplyCommitExecCount);
	}

	void checkStatus(int64_t queryId, uint8_t expectedStatus) const
	{
		uint32_t queryIndex;
		EXPECT_TRUE(this->queryIdToIndex->get(queryId, queryIndex));
		EXPECT_LT(queryIndex, this->oracleQueryCount);
		const OracleQueryMetadata& oqm = this->queries[queryIndex];
		EXPECT_EQ(oqm.status, expectedStatus);
	}
};

static void dummyNotificationProc(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals)
{
}

TEST(OracleEngine, ContractQuerySuccess)
{
	OracleEngineTest test;

	// simulate three nodes: one with 400 computor IDs, one with 200, and one with 76
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit<400> oracleEngine1(allCompPubKeys);
	OracleEngineWithInitAndDeinit<200> oracleEngine2(allCompPubKeys + 400);
	OracleEngineWithInitAndDeinit<76> oracleEngine3(allCompPubKeys + 600);

	OI::Price::OracleQuery priceQuery;
	priceQuery.oracle = m256i(1, 2, 3, 4);
	priceQuery.currency1 = m256i(2, 3, 4, 5);
	priceQuery.currency2 = m256i(3, 4, 5, 6);
	priceQuery.timestamp = QPI::DateAndTime::now();
	QPI::uint32 interfaceIndex = 0;
	QPI::uint16 contractIndex = 1;
	QPI::uint32 timeout = 30000;
	const QPI::uint32 notificationProcId = 12345;
	EXPECT_TRUE(userProcedureRegistry->add(notificationProcId, { dummyNotificationProc, 1, 128, 128, 1 }));

	//-------------------------------------------------------------------------
	// start contract query / check message to OM node
	QPI::sint64 queryId = oracleEngine1.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId);
	EXPECT_EQ(queryId, getContractOracleQueryId(system.tick, 0));
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, priceQuery.oracle, timeout);
	EXPECT_EQ(queryId, oracleEngine2.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId));
	EXPECT_EQ(queryId, oracleEngine3.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId));

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
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

	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));
	oracleEngine2.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));
	oracleEngine3.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));

	// duplicate from other node
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));

	// other value from other node
	priceOracleMachineReply.data.numerator = 1233;
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));

	//-------------------------------------------------------------------------
	// create reply commit tx (with local computor index 0 / global computor index 0)
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
	EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, 0, 0, system.tick + 3, 0), UINT32_MAX);
	{
		EXPECT_EQ((int)replyCommitTx->inputType, (int)OracleReplyCommitTransactionPrefix::transactionType());
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[0]);
		EXPECT_TRUE(isZero(replyCommitTx->destinationPublicKey));
		EXPECT_EQ(replyCommitTx->tick, system.tick + 3);
		EXPECT_EQ((int)replyCommitTx->inputSize, (int)sizeof(OracleReplyCommitTransactionItem));
	}

	// second call in the same tick: no commits for tx
	EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, 0, 0, system.tick + 3, 0), 0);

	// process commit tx
	EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
	EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
	EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));

	// no reveal yet
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	// no notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);

	//-------------------------------------------------------------------------
	// create and process enough reply commit tx to trigger reval tx

	// create tx of node 3 computers and process in all nodes
	for (int i = 600; i < 676; ++i)
	{
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, i, i - 600, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[i]);
		const int txFromNode3 = i - 600;
		oracleEngine1.checkPendingState(queryId, txFromNode3 + 1, 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, txFromNode3 + 2, 1, ORACLE_QUERY_STATUS_PENDING);
		oracleEngine2.checkPendingState(queryId, txFromNode3 + 1, 0, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, txFromNode3 + 2, 0, ORACLE_QUERY_STATUS_PENDING);
		oracleEngine3.checkPendingState(queryId, txFromNode3 + 1, txFromNode3 + 0, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, txFromNode3 + 2, txFromNode3 + 1, ORACLE_QUERY_STATUS_PENDING);
	}

	// create tx of node 2 computers and process in all nodes
	for (int i = 400; i < 600; ++i)
	{
		EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, i, i - 400, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[i]);
		const int txFromNode2 = i - 400;
		oracleEngine1.checkPendingState(queryId, txFromNode2 + 77, 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, txFromNode2 + 78, 1, ORACLE_QUERY_STATUS_PENDING);
		oracleEngine2.checkPendingState(queryId, txFromNode2 + 77, txFromNode2 + 0, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, txFromNode2 + 78, txFromNode2 + 1, ORACLE_QUERY_STATUS_PENDING);
		oracleEngine3.checkPendingState(queryId, txFromNode2 + 77, 76, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, txFromNode2 + 78, 76, ORACLE_QUERY_STATUS_PENDING);
	}

	// create tx of node 1 computers and process in all nodes
	for (int i = 1; i < 400; ++i)
	{
		bool expectStatusCommitted = (i + 276) >= 451;
		EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, i, i, system.tick + 3, 0), ((expectStatusCommitted) ? 0 : UINT32_MAX));
		if (!expectStatusCommitted)
		{
			EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[i]);
			const int txFromNode1 = i;
			uint8_t newStatus = (txFromNode1 + 276 < 450) ? ORACLE_QUERY_STATUS_PENDING : ORACLE_QUERY_STATUS_COMMITTED;
			oracleEngine1.checkPendingState(queryId, txFromNode1 + 276, txFromNode1, ORACLE_QUERY_STATUS_PENDING);
			EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
			oracleEngine1.checkPendingState(queryId, txFromNode1 + 277, txFromNode1 + 1, newStatus);
			oracleEngine2.checkPendingState(queryId, txFromNode1 + 276, 200, ORACLE_QUERY_STATUS_PENDING);
			EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
			oracleEngine2.checkPendingState(queryId, txFromNode1 + 277, 200, newStatus);
			oracleEngine3.checkPendingState(queryId, txFromNode1 + 276, 76, ORACLE_QUERY_STATUS_PENDING);
			EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
			oracleEngine3.checkPendingState(queryId, txFromNode1 + 277, 76, newStatus);
		}
		else
		{
			oracleEngine1.checkPendingState(queryId, 451, 175, ORACLE_QUERY_STATUS_COMMITTED);
			oracleEngine2.checkPendingState(queryId, 451, 200, ORACLE_QUERY_STATUS_COMMITTED);
			oracleEngine3.checkPendingState(queryId, 451, 76, ORACLE_QUERY_STATUS_COMMITTED);
		}
	}

	//-------------------------------------------------------------------------
	// reply reveal tx
	
	// success for one tx
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 1);
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 1), 0);

	// second call does not provide the same tx again
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	system.tick += 3;
	auto* replyRevealTx = (OracleReplyRevealTransactionPrefix*)txBuffer;
	const unsigned int txIndex = 10;
	addOracleTransactionToTickStorage(replyRevealTx, txIndex);
	oracleEngine1.processOracleReplyRevealTransaction(replyRevealTx, txIndex);

	//-------------------------------------------------------------------------
	// notifications
	const OracleNotificationData* notification = oracleEngine1.getNotification();
	EXPECT_NE(notification, nullptr);
	EXPECT_EQ((int)notification->contractIndex, (int)contractIndex);
	EXPECT_EQ(notification->procedureId, notificationProcId);
	EXPECT_EQ((int)notification->inputSize, sizeof(OracleNotificationInput<OI::Price>));
	const auto* notificationInput = (const OracleNotificationInput<OI::Price>*) & notification->inputBuffer;
	EXPECT_EQ(notificationInput->queryId, replyRevealTx->queryId);
	EXPECT_EQ(notificationInput->status, ORACLE_QUERY_STATUS_SUCCESS);
	EXPECT_EQ(notificationInput->subscriptionId, 0);
	EXPECT_EQ(notificationInput->reply.numerator, 1234);
	EXPECT_EQ(notificationInput->reply.denominator, 1);

	// no additional notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);
}

TEST(OracleEngine, ContractQueryUnresolvable)
{
	OracleEngineTest test;

	// simulate three nodes: two with 200 computor IDs each, one with 276 IDs
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit<200> oracleEngine1(allCompPubKeys);
	OracleEngineWithInitAndDeinit<200> oracleEngine2(allCompPubKeys + 200);
	OracleEngineWithInitAndDeinit<276> oracleEngine3(allCompPubKeys + 400);


	OI::Price::OracleQuery priceQuery;
	priceQuery.oracle = m256i(10, 20, 30, 40);
	priceQuery.currency1 = m256i(20, 30, 40, 50);
	priceQuery.currency2 = m256i(30, 40, 50, 60);
	priceQuery.timestamp = QPI::DateAndTime::now();
	QPI::uint32 interfaceIndex = 0;
	QPI::uint16 contractIndex = 2;
	QPI::uint32 timeout = 120000;
	const QPI::uint32 notificationProcId = 12345;
	EXPECT_TRUE(userProcedureRegistry->add(notificationProcId, { dummyNotificationProc, 1, 1024, 128, 1 }));

	//-------------------------------------------------------------------------
	// start contract query / check message to OM node
	QPI::sint64 queryId = oracleEngine1.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId);
	EXPECT_EQ(queryId, getContractOracleQueryId(system.tick, 0));
	EXPECT_EQ(queryId, oracleEngine2.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId));
	EXPECT_EQ(queryId, oracleEngine3.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId));
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, priceQuery.oracle, timeout);

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
	EXPECT_EQ(memcmp(&priceQueryReturned, &priceQuery, sizeof(priceQuery)), 0);

	//-------------------------------------------------------------------------
	// process simulated reply from OM nodes
	struct
	{
		OracleMachineReply metatdata;
		OI::Price::OracleReply data;
	} priceOracleMachineReply;

	// reply received/committed by node 1 and 2
	priceOracleMachineReply.metatdata.oracleMachineErrorFlags = 0;
	priceOracleMachineReply.metatdata.oracleQueryId = queryId;
	priceOracleMachineReply.data.numerator = 1234;
	priceOracleMachineReply.data.denominator = 1;
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));
	oracleEngine2.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));

	// reply received/committed by node 1 and 2
	priceOracleMachineReply.data.numerator = 1233;
	priceOracleMachineReply.data.denominator = 1;
	oracleEngine3.processOracleMachineReply(&priceOracleMachineReply.metatdata, sizeof(priceOracleMachineReply));


	//-------------------------------------------------------------------------
	// create and process reply commits of node 3 computers and process in all nodes
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
	for (int ownCompIdx = 0; ownCompIdx < 200; ++ownCompIdx)
	{
		int allCompIdx = ownCompIdx;
		EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, allCompIdx, ownCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);

		allCompIdx = ownCompIdx + 200;
		EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, allCompIdx, ownCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);

		allCompIdx = ownCompIdx + 400;
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, allCompIdx, ownCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
	}

	// create/process transcations that contradict with majority digest and turn status into unresolvable
	for (int allCompIdx = 600; allCompIdx < 676; ++allCompIdx)
	{
		int ownCompIdx = allCompIdx - 400;
		int unknownVotes = 676 - allCompIdx;
		bool moreTxExpected = (unknownVotes > 450 - 400);
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, allCompIdx, ownCompIdx, system.tick + 3, 0), moreTxExpected ? UINT32_MAX : 0);
		if (moreTxExpected)
		{
			EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);

			EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
			EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
			EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		}

		if (unknownVotes > 451 - 400)
		{
			oracleEngine1.checkPendingState(queryId, allCompIdx + 1, 200, ORACLE_QUERY_STATUS_PENDING);
			oracleEngine2.checkPendingState(queryId, allCompIdx + 1, 200, ORACLE_QUERY_STATUS_PENDING);
			oracleEngine3.checkPendingState(queryId, allCompIdx + 1, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		}
		else
		{
			oracleEngine1.checkStatus(queryId, ORACLE_QUERY_STATUS_UNRESOLVABLE);
			oracleEngine2.checkStatus(queryId, ORACLE_QUERY_STATUS_UNRESOLVABLE);
			oracleEngine3.checkStatus(queryId, ORACLE_QUERY_STATUS_UNRESOLVABLE);
		}
	}

	//-------------------------------------------------------------------------
	// notifications
	const OracleNotificationData* notification = oracleEngine1.getNotification();
	EXPECT_NE(notification, nullptr);
	EXPECT_EQ((int)notification->contractIndex, (int)contractIndex);
	EXPECT_EQ(notification->procedureId, notificationProcId);
	EXPECT_EQ((int)notification->inputSize, sizeof(OracleNotificationInput<OI::Price>));
	const auto* notificationInput = (const OracleNotificationInput<OI::Price>*) & notification->inputBuffer;
	EXPECT_EQ(notificationInput->queryId, queryId);
	EXPECT_EQ(notificationInput->status, ORACLE_QUERY_STATUS_UNRESOLVABLE);
	EXPECT_EQ(notificationInput->subscriptionId, 0);
	EXPECT_EQ(notificationInput->reply.numerator, 0);
	EXPECT_EQ(notificationInput->reply.denominator, 0);

	// no additional notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);
}

/*
Tests:
- oracleEngine.getReplyCommitTransaction() with more than 1 commit / tx
- processOracleReplyCommitTransaction wihtout get getReplyCommitTransaction
- trigger failure
*/