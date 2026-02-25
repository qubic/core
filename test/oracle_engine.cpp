#define NO_UEFI

#include "oracle_testing.h"

#include "platform/random.h"


struct OracleEngineTest : public LoggingTest
{
	OracleEngineTest()
	{
		EXPECT_TRUE(initSpectrum());
		EXPECT_TRUE(commonBuffers.init(1, 1024 * 1024));
		EXPECT_TRUE(initSpecialEntities());
		EXPECT_TRUE(initContractExec());
		EXPECT_TRUE(ts.init());
		EXPECT_TRUE(OI::initOracleInterfaces());

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
		deinitSpectrum();
		commonBuffers.deinit();
		deinitContractExec();
		ts.deinit();
	}
};

struct OracleEngineWithInitAndDeinit : public OracleEngine
{
	uint16_t ownComputorIdsBegin;
	uint16_t ownComputorIdsEnd;

	OracleEngineWithInitAndDeinit(const m256i* ownComputorPublicKeys, uint16_t ownComputorIdsBegin, uint16_t ownComputorIdsEnd)
	{
		this->init(ownComputorPublicKeys);
		this->ownComputorIdsBegin = ownComputorIdsBegin;
		this->ownComputorIdsEnd = ownComputorIdsEnd;
	}

	~OracleEngineWithInitAndDeinit()
	{
		this->deinit();
	}

	uint32_t getReplyCommitTransaction(
		void* txBuffer, uint16_t computorIdx,
		uint32_t txScheduleTick, uint32_t startIdx = 0)
	{
		ASSERT(computorIdx >= ownComputorIdsBegin);
		ASSERT(computorIdx < ownComputorIdsEnd);
		return OracleEngine::getReplyCommitTransaction(txBuffer, computorIdx, txScheduleTick, startIdx);
	}

	void checkPendingState(int64_t queryId, uint16_t totalCommitTxExecuted, uint16_t ownCommitTxExecuted, uint8_t expectedStatus) const
	{
		uint32_t queryIndex;
		EXPECT_TRUE(this->queryIdToIndex->get(queryId, queryIndex));
		EXPECT_LT(queryIndex, this->oracleQueryCount);
		const OracleQueryMetadata& oqm = this->queries[queryIndex];
		EXPECT_EQ(oqm.status, expectedStatus);
		EXPECT_TRUE(oqm.status == ORACLE_QUERY_STATUS_PENDING || oqm.status == ORACLE_QUERY_STATUS_COMMITTED);
		const OracleReplyState& replyState = this->replyStates[oqm.statusVar.pending.replyStateIndex];
		EXPECT_EQ((int)totalCommitTxExecuted, (int)replyState.totalCommits);
		int executed = 0;
		for (int i = ownComputorIdsBegin; i < ownComputorIdsEnd; ++i)
		{
			if (replyState.replyCommitTicks[i])
				++executed;
		}
		EXPECT_EQ((int)ownCommitTxExecuted, (int)executed);
		EXPECT_EQ(this->getOracleQueryStatus(queryId), expectedStatus);
	}

	void checkStatus(int64_t queryId, uint8_t expectedStatus) const
	{
		uint32_t queryIndex;
		EXPECT_TRUE(this->queryIdToIndex->get(queryId, queryIndex));
		EXPECT_LT(queryIndex, this->oracleQueryCount);
		const OracleQueryMetadata& oqm = this->queries[queryIndex];
		EXPECT_EQ(oqm.status, expectedStatus);
	}

	// Test findFirstQueryIndexOfTick(). Ticks must be sorted!
	void testFindFirstQueryIndexOfTick(const std::vector<uint32_t>& ticks)
	{
		// setup pseudo-queries
		this->oracleQueryCount = 0;
		for (auto tick : ticks)
			this->queries[this->oracleQueryCount++].queryTick = tick;

		// test function with all values in range
		uint32_t minValue = (ticks.empty()) ? 0 : ticks.front() - 1;
		uint32_t maxValue = (ticks.empty()) ? 2 : ticks.back() + 1;
		for (uint32 value = minValue; value <= maxValue; ++value)
		{
			auto it = std::find(ticks.begin(), ticks.end(), value);
			uint32_t expectedIndex = (it == ticks.end()) ? UINT32_MAX : uint32(it - ticks.begin());
			EXPECT_EQ(this->findFirstQueryIndexOfTick(value), expectedIndex);
		}

		this->reset();
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
	OracleEngineWithInitAndDeinit oracleEngine1(allCompPubKeys, 0, 400);
	OracleEngineWithInitAndDeinit oracleEngine2(allCompPubKeys, 400, 600);
	OracleEngineWithInitAndDeinit oracleEngine3(allCompPubKeys, 600, 676);

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
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, timeout, priceQuery);
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
		OracleMachineReply metadata;
		OI::Price::OracleReply data;
	} priceOracleMachineReply;

	priceOracleMachineReply.metadata.oracleMachineErrorFlags = 0;
	priceOracleMachineReply.metadata.oracleQueryId = queryId;
	priceOracleMachineReply.data.numerator = 1234;
	priceOracleMachineReply.data.denominator = 1;

	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	oracleEngine2.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	oracleEngine3.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));

	// duplicate from other node
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));

	// other value from other node
	priceOracleMachineReply.data.numerator = 1233;
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));

	//-------------------------------------------------------------------------
	// create reply commit tx (with computor index 0)
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
	EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, 0, system.tick + 3, 0), UINT32_MAX);
	{
		EXPECT_EQ((int)replyCommitTx->inputType, (int)OracleReplyCommitTransactionPrefix::transactionType());
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[0]);
		EXPECT_TRUE(isZero(replyCommitTx->destinationPublicKey));
		EXPECT_EQ(replyCommitTx->tick, system.tick + 3);
		EXPECT_EQ((int)replyCommitTx->inputSize, (int)sizeof(OracleReplyCommitTransactionItem));
	}

	// second call in the same tick: no commits for tx
	EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	// process commit tx
	system.tick += 3;
	EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
	EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
	EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));

	// no reveal yet
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	// no notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);

	//-------------------------------------------------------------------------
	// create and process enough reply commit tx to trigger reveal tx

	// create tx of node 3 computers and process in all nodes
	for (int i = 600; i < 676; ++i)
	{
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0), UINT32_MAX);
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
		EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0), UINT32_MAX);
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
		EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0), ((expectStatusCommitted) ? 0 : UINT32_MAX));
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

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_SUCCESS);

	OI::Price::OracleReply reply;
	EXPECT_TRUE(oracleEngine1.getOracleReply(queryId, &reply, sizeof(reply)));
	EXPECT_TRUE(compareMem(&reply, &notificationInput->reply, sizeof(reply)) == 0);

	// oracleEngine2 did not process reveal -> no success / reply
	EXPECT_EQ(oracleEngine2.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_COMMITTED);
	EXPECT_FALSE(oracleEngine2.getOracleReply(queryId, &reply, sizeof(reply)));

	//-------------------------------------------------------------------------
	// revenue
	OracleRevenuePoints rev1; oracleEngine1.getRevenuePoints(rev1);
	OracleRevenuePoints rev2; oracleEngine2.getRevenuePoints(rev2);
	OracleRevenuePoints rev3; oracleEngine3.getRevenuePoints(rev3);
	for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
	{
		// first 451 commit messages got through and are all correct:
		// - all of node 3 (computor 600-675)
		// - all of node 2 (computor 400-599)
		// - first 451-276 of node 1 (computor 0-174)
		bool gotCommit = (i >= 400) || (i <= 174);
		EXPECT_EQ(rev1.computorRevPoints[i], (gotCommit) ? 1 : 0);

		// no reveal processed in node 2 and 3
		EXPECT_EQ(rev2.computorRevPoints[i], 0);
		EXPECT_EQ(rev3.computorRevPoints[i], 0);
	}

	// check that oracle engine is in consistent state
	oracleEngine1.checkStateConsistencyWithAssert();
	oracleEngine2.checkStateConsistencyWithAssert();
	oracleEngine3.checkStateConsistencyWithAssert();
}

TEST(OracleEngine, ContractQueryUnresolvable)
{
	// 2 nodes send 200 commits each with agreeing digest
	// 1 node sends 276 commits with different digest
	// -> no quroum can be reached and status changes from pending to unresolvable directly
	// -> no reveal / nobody gets revenue

	OracleEngineTest test;

	// simulate three nodes: two with 200 computor IDs each, one with 276 IDs
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit oracleEngine1(allCompPubKeys, 0, 200);
	OracleEngineWithInitAndDeinit oracleEngine2(allCompPubKeys, 200, 400);
	OracleEngineWithInitAndDeinit oracleEngine3(allCompPubKeys, 400, 676);


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
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, timeout, priceQuery);

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
	EXPECT_EQ(memcmp(&priceQueryReturned, &priceQuery, sizeof(priceQuery)), 0);

	//-------------------------------------------------------------------------
	// process simulated reply from OM nodes
	struct
	{
		OracleMachineReply metadata;
		OI::Price::OracleReply data;
	} priceOracleMachineReply;

	// reply received/committed by node 1 and 2
	priceOracleMachineReply.metadata.oracleMachineErrorFlags = 0;
	priceOracleMachineReply.metadata.oracleQueryId = queryId;
	priceOracleMachineReply.data.numerator = 1234;
	priceOracleMachineReply.data.denominator = 1;
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	oracleEngine2.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));

	// reply received/committed by node 3
	priceOracleMachineReply.data.numerator = 1233;
	priceOracleMachineReply.data.denominator = 1;
	oracleEngine3.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));


	//-------------------------------------------------------------------------
	// create and process reply commits of node 3 computers and process in all nodes
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
	for (int ownCompIdx = 0; ownCompIdx < 200; ++ownCompIdx)
	{
		int allCompIdx = ownCompIdx;
		EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);

		allCompIdx = ownCompIdx + 200;
		EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);

		allCompIdx = ownCompIdx + 400;
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
	}

	// create/process transactions that contradict with majority digest and turn status into unresolvable
	for (int allCompIdx = 600; allCompIdx < 676; ++allCompIdx)
	{
		int ownCompIdx = allCompIdx - 400;
		int unknownVotes = 676 - allCompIdx;
		bool moreTxExpected = (unknownVotes > 450 - 400);
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), moreTxExpected ? UINT32_MAX : 0);
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

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_UNRESOLVABLE);

	//-------------------------------------------------------------------------
	// revenue
	OracleRevenuePoints rev1; oracleEngine1.getRevenuePoints(rev1);
	OracleRevenuePoints rev2; oracleEngine2.getRevenuePoints(rev2);
	OracleRevenuePoints rev3; oracleEngine3.getRevenuePoints(rev3);
	for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
	{
		// no reveal
		EXPECT_EQ(rev1.computorRevPoints[i], 0);
		EXPECT_EQ(rev2.computorRevPoints[i], 0);
		EXPECT_EQ(rev3.computorRevPoints[i], 0);
	}

	// check that oracle engine is in consistent state
	oracleEngine1.checkStateConsistencyWithAssert();
	oracleEngine2.checkStateConsistencyWithAssert();
	oracleEngine3.checkStateConsistencyWithAssert();
}

TEST(OracleEngine, ContractQueryWrongKnowledgeProof)
{
	// 3 nodes send 451 commits with agreeing digest, but 150 of them have
	// a wrong knowledge proof -> status gets unresolvable, but the computors
	// with correct knowledge proof get a revenue point

	OracleEngineTest test;

	// simulate three nodes: two with 200 computor IDs each, one with 276 IDs
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit oracleEngine1(allCompPubKeys, 0, 200);
	OracleEngineWithInitAndDeinit oracleEngine2(allCompPubKeys, 200, 400);
	OracleEngineWithInitAndDeinit oracleEngine3(allCompPubKeys, 400, 676);


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
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, timeout, priceQuery);

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
	EXPECT_EQ(memcmp(&priceQueryReturned, &priceQuery, sizeof(priceQuery)), 0);

	//-------------------------------------------------------------------------
	// process simulated reply from OM nodes
	struct
	{
		OracleMachineReply metadata;
		OI::Price::OracleReply data;
	} priceOracleMachineReply;

	// reply received/committed
	priceOracleMachineReply.metadata.oracleMachineErrorFlags = 0;
	priceOracleMachineReply.metadata.oracleQueryId = queryId;
	priceOracleMachineReply.data.numerator = 1234;
	priceOracleMachineReply.data.denominator = 1;
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	oracleEngine2.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	oracleEngine3.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));

	//-------------------------------------------------------------------------
	// create and process reply commits of node 3 computers and process in all nodes
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
	for (int ownCompIdx = 0; ownCompIdx < 200; ++ownCompIdx)
	{
		int allCompIdx = ownCompIdx;
		EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		uint8_t expectedStatus = (3 * ownCompIdx + 1 < 451) ? ORACLE_QUERY_STATUS_PENDING : ORACLE_QUERY_STATUS_COMMITTED;
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx + 1, expectedStatus);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx, expectedStatus);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 1, ownCompIdx, expectedStatus);

		if (3 * ownCompIdx + 1 == 451)
		{
			// After status switched to committed, getReplyCommitTransaction() won't return more tx because these
			// would be to late to get revenue anyway.
			allCompIdx = ownCompIdx + 200;
			EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), 0);
			allCompIdx = ownCompIdx + 400;
			EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), 0);
			break;
		}

		allCompIdx = ownCompIdx + 200;
		EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 2, ownCompIdx, ORACLE_QUERY_STATUS_PENDING);

		allCompIdx = ownCompIdx + 400;
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, allCompIdx, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[allCompIdx]);

		// manipulate knowledge proof of computor 400-600 to simulate that computors just echo the commit of
		// other computors without actually having the oracle reply
		auto* commit = reinterpret_cast<OracleReplyCommitTransactionItem*>(replyCommitTx->inputPtr());
		commit->replyKnowledgeProof.u64._3 += 2;

		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, 3 * ownCompIdx + 3, ownCompIdx + 1, ORACLE_QUERY_STATUS_PENDING);
	}

	//-------------------------------------------------------------------------
	// reply reveal tx

	// success for one tx
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 1);
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 1), 0);

	// second call does not provide the same tx again
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_COMMITTED);

	system.tick += 3;
	auto* replyRevealTx = (OracleReplyRevealTransactionPrefix*)txBuffer;
	const unsigned int txIndex = 10;
	addOracleTransactionToTickStorage(replyRevealTx, txIndex);
	oracleEngine1.processOracleReplyRevealTransaction(replyRevealTx, txIndex);

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_UNRESOLVABLE);

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

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_UNRESOLVABLE);

	//-------------------------------------------------------------------------
	// revenue
	OracleRevenuePoints rev1; oracleEngine1.getRevenuePoints(rev1);
	OracleRevenuePoints rev2; oracleEngine2.getRevenuePoints(rev2);
	OracleRevenuePoints rev3; oracleEngine3.getRevenuePoints(rev3);
	for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
	{
		// In reveal, computors with correct knowledge proof get a point and those
		// with wrong knowledge proof don't get one:
		// - first 151 of node 1 got through and get rev (0-150)
		// - first 150 of node 2 got through and get rev (200-349)
		// - first 150 of node 3 get no rev due to wrong knowledge proof
		// - other don't get rev, because no commit got through
		bool node1rev = i <= 150;
		bool node2rev = i >= 200 && i <= 349;
		uint64_t expectedRevPoints = (node1rev || node2rev) ? 1 : 0;
		EXPECT_EQ(rev1.computorRevPoints[i], expectedRevPoints);

		// no reveal
		EXPECT_EQ(rev2.computorRevPoints[i], 0);
		EXPECT_EQ(rev3.computorRevPoints[i], 0);
	}

	// check that oracle engine is in consistent state
	oracleEngine1.checkStateConsistencyWithAssert();
	oracleEngine2.checkStateConsistencyWithAssert();
	oracleEngine3.checkStateConsistencyWithAssert();
}

TEST(OracleEngine, ContractQueryTimeout)
{
	// no response from OM node
	// -> pending to timeout directly
	// -> no reveal / nobody gets revenue

	OracleEngineTest test;

	// simulate one node
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit oracleEngine1(allCompPubKeys, 0, 676);

	OI::Price::OracleQuery priceQuery;
	priceQuery.oracle = m256i(10, 20, 30, 40);
	priceQuery.currency1 = m256i(20, 30, 40, 50);
	priceQuery.currency2 = m256i(30, 40, 50, 60);
	priceQuery.timestamp = QPI::DateAndTime::now();
	QPI::uint32 interfaceIndex = 0;
	QPI::uint16 contractIndex = 2;
	QPI::uint32 timeout = 10000;
	const QPI::uint32 notificationProcId = 12345;
	EXPECT_TRUE(userProcedureRegistry->add(notificationProcId, { dummyNotificationProc, 1, 1024, 128, 1 }));

	//-------------------------------------------------------------------------
	// start contract query / check message to OM node
	QPI::sint64 queryId = oracleEngine1.startContractQuery(contractIndex, interfaceIndex, &priceQuery, sizeof(priceQuery), timeout, notificationProcId);
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, timeout, priceQuery);

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
	EXPECT_EQ(memcmp(&priceQueryReturned, &priceQuery, sizeof(priceQuery)), 0);

	//-------------------------------------------------------------------------
	// timeout: no response from OM node
	++system.tick;
	++etalonTick.hour;
	oracleEngine1.processTimeouts();

	//-------------------------------------------------------------------------
	// notifications
	const OracleNotificationData* notification = oracleEngine1.getNotification();
	EXPECT_NE(notification, nullptr);
	EXPECT_EQ((int)notification->contractIndex, (int)contractIndex);
	EXPECT_EQ(notification->procedureId, notificationProcId);
	EXPECT_EQ((int)notification->inputSize, sizeof(OracleNotificationInput<OI::Price>));
	const auto* notificationInput = (const OracleNotificationInput<OI::Price>*) & notification->inputBuffer;
	EXPECT_EQ(notificationInput->queryId, queryId);
	EXPECT_EQ(notificationInput->status, ORACLE_QUERY_STATUS_TIMEOUT);
	EXPECT_EQ(notificationInput->subscriptionId, 0);
	EXPECT_EQ(notificationInput->reply.numerator, 0);
	EXPECT_EQ(notificationInput->reply.denominator, 0);

	// no additional notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_TIMEOUT);

	//-------------------------------------------------------------------------
	// revenue
	OracleRevenuePoints rev1; oracleEngine1.getRevenuePoints(rev1);
	for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
	{
		// no reveal
		EXPECT_EQ(rev1.computorRevPoints[i], 0);
	}

	// check that oracle engine is in consistent state
	oracleEngine1.checkStateConsistencyWithAssert();
}

template <typename OracleEngine>
static void checkReplyCommitTransactions(
	OracleEngine& oracleEngine, int globalCompIdxBegin, int globalCompIdxEnd,
	const std::vector<QPI::sint64>& queryIds, QPI::sint64 queryIdWithoutReply)
{
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;

	// create tx of node 3 computers and process in all nodes
	for (int globalCompIdx = globalCompIdxBegin; globalCompIdx < globalCompIdxEnd; ++globalCompIdx)
	{
		std::set<QPI::sint64> pendingCommitQueryIds;
		pendingCommitQueryIds.insert(queryIds.begin(), queryIds.end());

		unsigned int retCode = 0;
		do
		{
			retCode = oracleEngine.getReplyCommitTransaction(txBuffer, globalCompIdx, system.tick + 3, retCode);
			if (!retCode)
				break;
			unsigned short commitCount = replyCommitTx->inputSize / sizeof(OracleReplyCommitTransactionItem);
			auto* commits = reinterpret_cast<OracleReplyCommitTransactionItem*>(replyCommitTx->inputPtr());
			for (unsigned short i = 0; i < commitCount; ++i)
			{
				pendingCommitQueryIds.erase(commits[i].queryId);
			}
		} while (retCode != UINT32_MAX);

		// only the query without OM reply is not returned
		EXPECT_EQ(pendingCommitQueryIds.size(), 1);
		EXPECT_TRUE(pendingCommitQueryIds.contains(queryIdWithoutReply));
	}
}

TEST(OracleEngine, MultiContractQuerySuccess)
{
	OracleEngineTest test;

	// simulate three nodes: one with 350 computor IDs, one with 250, and one with 76
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit oracleEngine1(allCompPubKeys, 0, 350);
	OracleEngineWithInitAndDeinit oracleEngine2(allCompPubKeys, 350, 600);
	OracleEngineWithInitAndDeinit oracleEngine3(allCompPubKeys, 600, 676);

	QPI::uint16 contractIndex = 4;
	QPI::uint32 timeout = 50000;
	const QPI::uint32 notificationProcId = 12345;
	EXPECT_TRUE(userProcedureRegistry->add(notificationProcId, { dummyNotificationProc, 1, 128, 128, 1 }));

	constexpr int queryCount = 25;
	QPI::uint16 interfaceIndex = OI::Mock::oracleInterfaceIndex;
	OI::Mock::OracleQuery mockQuery;

	//-------------------------------------------------------------------------
	// start contract query / check message to OM node / check query
	std::vector<QPI::sint64> queryIds;
	for (int i = 0; i < queryCount; ++i)
	{
		mockQuery.value = i + 1000;
		QPI::sint64 queryId = oracleEngine1.startContractQuery(contractIndex, interfaceIndex, &mockQuery, sizeof(mockQuery), timeout, notificationProcId);
		EXPECT_EQ(queryId, getContractOracleQueryId(system.tick, i));
		EXPECT_EQ(queryId, oracleEngine2.startContractQuery(contractIndex, interfaceIndex, &mockQuery, sizeof(mockQuery), timeout, notificationProcId));
		EXPECT_EQ(queryId, oracleEngine3.startContractQuery(contractIndex, interfaceIndex, &mockQuery, sizeof(mockQuery), timeout, notificationProcId));
		
		checkNetworkMessageOracleMachineQuery<OI::Mock>(queryId, timeout, mockQuery);
		
		OI::Mock::OracleQuery mockQueryReturned;
		EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &mockQueryReturned, sizeof(mockQueryReturned)));
		EXPECT_EQ(memcmp(&mockQueryReturned, &mockQuery, sizeof(mockQuery)), 0);

		queryIds.push_back(queryId);
	}

	//-------------------------------------------------------------------------
	// process simulated reply from OM node
	struct
	{
		OracleMachineReply metadata;
		OI::Mock::OracleReply data;
	} oracleMachineReply;

	for (int i = 0; i < queryCount; ++i)
	{
		oracleMachineReply.metadata.oracleMachineErrorFlags = 0;
		oracleMachineReply.metadata.oracleQueryId = queryIds[i];
		oracleMachineReply.data.echoedValue = 1000 + i;
		oracleMachineReply.data.doubledValue = (1000 + i) * 2;

		// the first 3 queries don't get OM reply all oracleEngines
		if (i != 0)
			oracleEngine1.processOracleMachineReply(&oracleMachineReply.metadata, sizeof(oracleMachineReply));
		if (i != 1)
			oracleEngine2.processOracleMachineReply(&oracleMachineReply.metadata, sizeof(oracleMachineReply));
		if (i != 2)
			oracleEngine3.processOracleMachineReply(&oracleMachineReply.metadata, sizeof(oracleMachineReply));
	}

	//-------------------------------------------------------------------------
	// create and process reply commit transactions

	// get all commit tx for checking that correct set of commits is returned

	checkReplyCommitTransactions(oracleEngine1, 0, 350, queryIds, queryIds[0]);
	checkReplyCommitTransactions(oracleEngine2, 350, 600, queryIds, queryIds[1]);
	checkReplyCommitTransactions(oracleEngine3, 600, 676, queryIds, queryIds[2]);

	// advance few ticks in order to get commit tx returned again for processing the commits
	system.tick += 5;

	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;

	unsigned int txIndexInTickData = 0;

	int globalCompIdxBeginEnd[] = { 0, 350, 600, 676 };
	for (int oracleEngineIdx = 0; oracleEngineIdx < 3; ++oracleEngineIdx)
	{
		for (int globalCompIdx = globalCompIdxBeginEnd[oracleEngineIdx];
			globalCompIdx < globalCompIdxBeginEnd[oracleEngineIdx + 1];
			++globalCompIdx)
		{
			unsigned int retCode = 0;
			do
			{
				if (oracleEngineIdx == 0)
					retCode = oracleEngine1.getReplyCommitTransaction(
						txBuffer, globalCompIdx,
						system.tick + 3, retCode);
				else if (oracleEngineIdx == 1)
					retCode = oracleEngine2.getReplyCommitTransaction(
						txBuffer, globalCompIdx,
						system.tick + 3, retCode);
				else
					retCode = oracleEngine3.getReplyCommitTransaction(
						txBuffer, globalCompIdx,
						system.tick + 3, retCode);
				if (!retCode)
					break;

				// store commit tx in tick storage for processing tx later
				addOracleTransactionToTickStorage(replyCommitTx, txIndexInTickData);
				txIndexInTickData++;
			} while (retCode != UINT32_MAX);
		}

		// each engine send its txs in a different tick here
		++system.tick;
		txIndexInTickData = 0;
	}

	// process commit txs
	ts.checkStateConsistencyWithAssert();
	for (int tick = 0; tick < 3; ++tick)
	{
		const unsigned long long* tsTickTransactionOffsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(system.tick);
		const unsigned long long* tsTickTransactionOffsets2 = ts.tickTransactionOffsets.getByTickInCurrentEpoch(system.tick + 1);
		const unsigned long long* tsTickTransactionOffsets3 = ts.tickTransactionOffsets.getByTickInCurrentEpoch(system.tick + 2);
		for (txIndexInTickData = 0; txIndexInTickData < NUMBER_OF_TRANSACTIONS_PER_TICK; ++txIndexInTickData)
		{
			const unsigned long long offset = tsTickTransactionOffsets[txIndexInTickData];
			if (offset)
			{
				const auto* commitTx = (OracleReplyCommitTransactionPrefix*)ts.tickTransactions.ptr(offset);
				EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(commitTx));
				EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(commitTx));
				EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(commitTx));
			}
		}
		++system.tick;
	}

	// check status
	oracleEngine1.checkPendingState(queryIds[0], 326, 0, ORACLE_QUERY_STATUS_PENDING);
	oracleEngine2.checkPendingState(queryIds[0], 326, 250, ORACLE_QUERY_STATUS_PENDING);
	oracleEngine3.checkPendingState(queryIds[0], 326, 76, ORACLE_QUERY_STATUS_PENDING);

	oracleEngine1.checkPendingState(queryIds[1], 426, 350, ORACLE_QUERY_STATUS_PENDING);
	oracleEngine2.checkPendingState(queryIds[1], 426, 0, ORACLE_QUERY_STATUS_PENDING);
	oracleEngine3.checkPendingState(queryIds[1], 426, 76, ORACLE_QUERY_STATUS_PENDING);

	oracleEngine1.checkPendingState(queryIds[2], 600, 350, ORACLE_QUERY_STATUS_COMMITTED);
	oracleEngine2.checkPendingState(queryIds[2], 600, 250, ORACLE_QUERY_STATUS_COMMITTED);
	oracleEngine3.checkPendingState(queryIds[2], 600, 0, ORACLE_QUERY_STATUS_COMMITTED);

	for (int i = 3; i < queryCount; ++i)
	{
		oracleEngine1.checkPendingState(queryIds[i], 676, 350, ORACLE_QUERY_STATUS_COMMITTED);
		oracleEngine2.checkPendingState(queryIds[i], 676, 250, ORACLE_QUERY_STATUS_COMMITTED);
		oracleEngine3.checkPendingState(queryIds[i], 676, 76, ORACLE_QUERY_STATUS_COMMITTED);
	}

	//-------------------------------------------------------------------------
	// reply reveal tx

	std::set<QPI::sint64> pendingRevealQueryIds;
	pendingRevealQueryIds.insert(queryIds.begin() + 2, queryIds.end());

	// generate all reveal of oracleEngine3
	auto* replyRevealTx = (OracleReplyRevealTransactionPrefix*)txBuffer;
	unsigned int retCode = 0;
	std::vector<const OracleReplyRevealTransactionPrefix*> revealTxs;
	txIndexInTickData = 0;
	while ((retCode = oracleEngine3.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, retCode)) != 0)
	{
		oracleEngine1.announceExpectedRevealTransaction(replyRevealTx);
		oracleEngine2.announceExpectedRevealTransaction(replyRevealTx);
		oracleEngine3.announceExpectedRevealTransaction(replyRevealTx);

		// save pointer to reveal tx in tick storage for processing tx later
		revealTxs.push_back((OracleReplyRevealTransactionPrefix*)addOracleTransactionToTickStorage(replyRevealTx, txIndexInTickData));
		txIndexInTickData++;
	}

	// process reveal tx
	system.tick += 3;
	for (txIndexInTickData = 0; txIndexInTickData < revealTxs.size(); ++txIndexInTickData)
	{
		const auto* revealTx = revealTxs[txIndexInTickData];
		pendingRevealQueryIds.erase(revealTx->queryId);

		oracleEngine1.processOracleReplyRevealTransaction(revealTx, txIndexInTickData);
		oracleEngine2.processOracleReplyRevealTransaction(revealTx, txIndexInTickData);
		oracleEngine3.processOracleReplyRevealTransaction(revealTx, txIndexInTickData);
	}

	// no reveal possible for oracleEngine3 in query 2, because it didn't get OM reply
	EXPECT_EQ(pendingRevealQueryIds.size(), 1);
	EXPECT_TRUE(pendingRevealQueryIds.contains(queryIds[2]));

	// reveal query 2 using oracleEngine2
	EXPECT_EQ(oracleEngine2.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 1);
	EXPECT_EQ(replyRevealTx->queryId, queryIds[2]);
	EXPECT_EQ(oracleEngine2.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 1), 0);
	addOracleTransactionToTickStorage(replyRevealTx, txIndexInTickData);
	system.tick += 3;
	oracleEngine1.processOracleReplyRevealTransaction(replyRevealTx, txIndexInTickData);
	oracleEngine2.processOracleReplyRevealTransaction(replyRevealTx, txIndexInTickData);
	oracleEngine3.processOracleReplyRevealTransaction(replyRevealTx, txIndexInTickData);

	// nothing to reveal for oracleEngine1
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	//-------------------------------------------------------------------------
	// notifications

	std::set<QPI::sint64> pendingNotificationQueryIds;
	pendingNotificationQueryIds.insert(queryIds.begin() + 2, queryIds.end());

	for (int i = 2; i < queryCount; ++i)
	{
		const OracleNotificationData* notification = oracleEngine1.getNotification();
		EXPECT_NE(notification, nullptr);
		EXPECT_EQ((int)notification->contractIndex, (int)contractIndex);
		EXPECT_EQ(notification->procedureId, notificationProcId);
		EXPECT_EQ((int)notification->inputSize, sizeof(OracleNotificationInput<OI::Mock>));
		const auto* notificationInput = (const OracleNotificationInput<OI::Mock>*) & notification->inputBuffer;
		EXPECT_NE(notificationInput->queryId, 0);
		EXPECT_EQ(notificationInput->status, ORACLE_QUERY_STATUS_SUCCESS);
		EXPECT_EQ(notificationInput->subscriptionId, 0);
		
		EXPECT_EQ(oracleEngine1.getOracleQueryStatus(notificationInput->queryId), ORACLE_QUERY_STATUS_SUCCESS);
		OI::Mock::OracleQuery query;
		EXPECT_TRUE(oracleEngine1.getOracleQuery(notificationInput->queryId, &query, sizeof(query)));
		
		EXPECT_EQ(notificationInput->reply.echoedValue, query.value);
		EXPECT_EQ(notificationInput->reply.doubledValue, query.value * 2);

		OI::Mock::OracleReply reply;
		EXPECT_TRUE(oracleEngine1.getOracleReply(notificationInput->queryId, &reply, sizeof(reply)));
		EXPECT_TRUE(compareMem(&reply, &notificationInput->reply, sizeof(reply)) == 0);

		pendingNotificationQueryIds.erase(notificationInput->queryId);
	}

	// no additional notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);
	EXPECT_EQ(pendingNotificationQueryIds.size(), 0);

	//-------------------------------------------------------------------------
	// revenue
	OracleRevenuePoints rev1; oracleEngine1.getRevenuePoints(rev1);
	OracleRevenuePoints rev2; oracleEngine2.getRevenuePoints(rev2);
	OracleRevenuePoints rev3; oracleEngine3.getRevenuePoints(rev3);
	for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
	{
		bool hadFastestCommits = (i < 600);
		uint64_t expectedRevPoints = (hadFastestCommits) ? queryCount - 2 : 0;
		EXPECT_EQ(rev1.computorRevPoints[i], expectedRevPoints);
		EXPECT_EQ(rev2.computorRevPoints[i], expectedRevPoints);
		EXPECT_EQ(rev3.computorRevPoints[i], expectedRevPoints);
	}

	// check that oracle engine is in consistent state
	oracleEngine1.checkStateConsistencyWithAssert();
	oracleEngine2.checkStateConsistencyWithAssert();
	oracleEngine3.checkStateConsistencyWithAssert();
}

/*
Tests:
- error conditions
*/

struct PriceQueryTransaction : public OracleUserQueryTransactionPrefix
{
	OI::Price::OracleQuery query;
	uint8_t signature[SIGNATURE_SIZE];
};

static PriceQueryTransaction getPriceQueryTransaction(const OI::Price::OracleQuery& query, const m256i& sourcePublicKey, int64_t fee, uint32_t timeout)
{
	PriceQueryTransaction tx;
	tx.amount = fee;
	tx.destinationPublicKey = m256i::zero();
	tx.inputSize = OracleUserQueryTransactionPrefix::minInputSize() + sizeof(query);
	tx.inputType = OracleUserQueryTransactionPrefix::transactionType();
	tx.oracleInterfaceIndex = OI::Price::oracleInterfaceIndex;
	tx.sourcePublicKey = sourcePublicKey;
	tx.tick = system.tick;
	tx.timeoutMilliseconds = timeout;
	tx.query = query;
	setMem(tx.signature, SIGNATURE_SIZE, 0); // keep signature uninitialized
	return tx;
}

TEST(OracleEngine, UserQuerySuccess)
{
	OracleEngineTest test;

	const id USER1(123, 456, 789, 876);
	increaseEnergy(USER1, 10000000000);

	// simulate three nodes: one with 400 computor IDs, one with 200, and one with 76
	const m256i* allCompPubKeys = broadcastedComputors.computors.publicKeys;
	OracleEngineWithInitAndDeinit oracleEngine1(allCompPubKeys, 0, 400);
	OracleEngineWithInitAndDeinit oracleEngine2(allCompPubKeys, 400, 600);
	OracleEngineWithInitAndDeinit oracleEngine3(allCompPubKeys, 600, 676);

	OI::Price::OracleQuery priceQuery;
	priceQuery.oracle = m256i(42, 13, 100, 1000);
	priceQuery.currency1 = m256i(20, 30, 40, 50);
	priceQuery.currency2 = m256i(300, 400, 500, 600);
	priceQuery.timestamp = QPI::DateAndTime::now();
	QPI::uint32 interfaceIndex = 0;
	QPI::uint32 timeout = 40000;
	QPI::sint64 fee = OI::Price::getQueryFee(priceQuery);
	PriceQueryTransaction priceQueryTx = getPriceQueryTransaction(priceQuery, USER1, fee, timeout);
	unsigned int priceQueryTxIndex = 15;
	addOracleTransactionToTickStorage(&priceQueryTx, priceQueryTxIndex);

	const QPI::uint32 notificationProcId = 12345;
	EXPECT_TRUE(userProcedureRegistry->add(notificationProcId, { dummyNotificationProc, 1, 128, 128, 1 }));

	//-------------------------------------------------------------------------
	// start user query / check message to OM node
	QPI::sint64 queryId = oracleEngine1.startUserQuery(&priceQueryTx, priceQueryTxIndex);
	EXPECT_EQ(queryId, getUserOracleQueryId(system.tick, priceQueryTxIndex));
	checkNetworkMessageOracleMachineQuery<OI::Price>(queryId, timeout, priceQuery);
	EXPECT_EQ(queryId, oracleEngine2.startUserQuery(&priceQueryTx, priceQueryTxIndex));
	EXPECT_EQ(queryId, oracleEngine3.startUserQuery(&priceQueryTx, priceQueryTxIndex));

	//-------------------------------------------------------------------------
	// get query contract data
	OI::Price::OracleQuery priceQueryReturned;
	EXPECT_TRUE(oracleEngine1.getOracleQuery(queryId, &priceQueryReturned, sizeof(priceQueryReturned)));
	EXPECT_EQ(compareMem(&priceQueryReturned, &priceQuery, sizeof(priceQuery)), 0);

	//-------------------------------------------------------------------------
	// process simulated reply from OM node
	struct
	{
		OracleMachineReply metadata;
		OI::Price::OracleReply data;
	} priceOracleMachineReply;

	priceOracleMachineReply.metadata.oracleMachineErrorFlags = 0;
	priceOracleMachineReply.metadata.oracleQueryId = queryId;
	priceOracleMachineReply.data.numerator = 1234;
	priceOracleMachineReply.data.denominator = 1;

	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	oracleEngine2.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	// test: no reply to oracle engine 3!

	// duplicate from other node
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));

	// other value from other node
	priceOracleMachineReply.data.numerator = 1233;
	oracleEngine1.processOracleMachineReply(&priceOracleMachineReply.metadata, sizeof(priceOracleMachineReply));
	priceOracleMachineReply.data.numerator = 1234;

	//-------------------------------------------------------------------------
	// create reply commit tx (with computor index 0)
	uint8_t txBuffer[MAX_TRANSACTION_SIZE];
	auto* replyCommitTx = (OracleReplyCommitTransactionPrefix*)txBuffer;
	EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, 0, system.tick + 3, 0), UINT32_MAX);
	{
		EXPECT_EQ((int)replyCommitTx->inputType, (int)OracleReplyCommitTransactionPrefix::transactionType());
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[0]);
		EXPECT_TRUE(isZero(replyCommitTx->destinationPublicKey));
		EXPECT_EQ(replyCommitTx->tick, system.tick + 3);
		EXPECT_EQ((int)replyCommitTx->inputSize, (int)sizeof(OracleReplyCommitTransactionItem));
	}

	// second call in the same tick: no commits for tx
	EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	// process commit tx
	EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
	EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
	EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));

	// no reveal yet
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	// no notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);

	//-------------------------------------------------------------------------
	// create and process enough reply commit tx to trigger reveal tx

	// create tx of node 3 computers? -> no commit tx because reply data is not available in node 3 (no OM reply)
	for (int i = 600; i < 676; ++i)
	{
		EXPECT_EQ(oracleEngine3.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0), 0);
	}

	// create tx of node 2 computers and process in all nodes
	for (int i = 400; i < 600; ++i)
	{
		EXPECT_EQ(oracleEngine2.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0), UINT32_MAX);
		EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[i]);
		const int txFromNode2 = i - 400;
		oracleEngine1.checkPendingState(queryId, txFromNode2 + 1, 1, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine1.checkPendingState(queryId, txFromNode2 + 2, 1, ORACLE_QUERY_STATUS_PENDING);
		oracleEngine2.checkPendingState(queryId, txFromNode2 + 1, txFromNode2 + 0, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine2.checkPendingState(queryId, txFromNode2 + 2, txFromNode2 + 1, ORACLE_QUERY_STATUS_PENDING);
		oracleEngine3.checkPendingState(queryId, txFromNode2 + 1, 0, ORACLE_QUERY_STATUS_PENDING);
		EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
		oracleEngine3.checkPendingState(queryId, txFromNode2 + 2, 0, ORACLE_QUERY_STATUS_PENDING);
	}

	// create tx of node 1 computers and process in all nodes
	for (int i = 1; i < 400; ++i)
	{
		bool expectStatusCommitted = (i + 200) >= 451;
		EXPECT_EQ(oracleEngine1.getReplyCommitTransaction(txBuffer, i, system.tick + 3, 0), ((expectStatusCommitted) ? 0 : UINT32_MAX));
		if (!expectStatusCommitted)
		{
			EXPECT_EQ(replyCommitTx->sourcePublicKey, allCompPubKeys[i]);
			const int txFromNode1 = i;
			uint8_t newStatus = (txFromNode1 + 200 < 450) ? ORACLE_QUERY_STATUS_PENDING : ORACLE_QUERY_STATUS_COMMITTED;
			oracleEngine1.checkPendingState(queryId, txFromNode1 + 200, txFromNode1, ORACLE_QUERY_STATUS_PENDING);
			EXPECT_TRUE(oracleEngine1.processOracleReplyCommitTransaction(replyCommitTx));
			oracleEngine1.checkPendingState(queryId, txFromNode1 + 201, txFromNode1 + 1, newStatus);
			oracleEngine2.checkPendingState(queryId, txFromNode1 + 200, 200, ORACLE_QUERY_STATUS_PENDING);
			EXPECT_TRUE(oracleEngine2.processOracleReplyCommitTransaction(replyCommitTx));
			oracleEngine2.checkPendingState(queryId, txFromNode1 + 201, 200, newStatus);
			oracleEngine3.checkPendingState(queryId, txFromNode1 + 200, 0, ORACLE_QUERY_STATUS_PENDING);
			EXPECT_TRUE(oracleEngine3.processOracleReplyCommitTransaction(replyCommitTx));
			oracleEngine3.checkPendingState(queryId, txFromNode1 + 201, 0, newStatus);
		}
		else
		{
			oracleEngine1.checkPendingState(queryId, 451, 251, ORACLE_QUERY_STATUS_COMMITTED);
			oracleEngine2.checkPendingState(queryId, 451, 200, ORACLE_QUERY_STATUS_COMMITTED);
			oracleEngine3.checkPendingState(queryId, 451, 0, ORACLE_QUERY_STATUS_COMMITTED);
		}
	}
	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_COMMITTED);

	//-------------------------------------------------------------------------
	// reply reveal tx

	// success for one tx
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 1);
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 1), 0);

	// second call does not provide the same tx again
	EXPECT_EQ(oracleEngine1.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	// node 3 is in committed state but cannot generate reveal tx, because it did not receive OM reply
	EXPECT_EQ(oracleEngine3.getReplyRevealTransaction(txBuffer, 0, system.tick + 3, 0), 0);

	system.tick += 3;
	auto* replyRevealTx = (OracleReplyRevealTransactionPrefix*)txBuffer;
	const unsigned int txIndex = 10;
	addOracleTransactionToTickStorage(replyRevealTx, txIndex);
	oracleEngine1.processOracleReplyRevealTransaction(replyRevealTx, txIndex);
	oracleEngine2.processOracleReplyRevealTransaction(replyRevealTx, txIndex);
	oracleEngine3.processOracleReplyRevealTransaction(replyRevealTx, txIndex);

	//-------------------------------------------------------------------------
	// status
	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_SUCCESS);
	EXPECT_EQ(oracleEngine2.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_SUCCESS);
	EXPECT_EQ(oracleEngine3.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_SUCCESS);

	OI::Price::OracleReply reply;
	EXPECT_TRUE(oracleEngine1.getOracleReply(queryId, &reply, sizeof(reply)));
	EXPECT_TRUE(compareMem(&reply, &priceOracleMachineReply.data, sizeof(reply)) == 0);
	EXPECT_TRUE(oracleEngine2.getOracleReply(queryId, &reply, sizeof(reply)));
	EXPECT_TRUE(compareMem(&reply, &priceOracleMachineReply.data, sizeof(reply)) == 0);
	EXPECT_TRUE(oracleEngine3.getOracleReply(queryId, &reply, sizeof(reply)));
	EXPECT_TRUE(compareMem(&reply, &priceOracleMachineReply.data, sizeof(reply)) == 0);

	// check that oracle engine is in consistent state
	oracleEngine1.checkStateConsistencyWithAssert();
	oracleEngine2.checkStateConsistencyWithAssert();
	oracleEngine3.checkStateConsistencyWithAssert();
}

TEST(OracleEngine, FindFirstQueryIndexOfTick)
{
	OracleEngineTest test;
	OracleEngineWithInitAndDeinit oracleEngine(broadcastedComputors.computors.publicKeys, 0, 676);
	oracleEngine.testFindFirstQueryIndexOfTick({ 1 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 2 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 3 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 1 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 2 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 3 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 2, 2 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 2, 3 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 3, 3 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 1, 1 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 1, 2 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 1, 3 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 1, 4 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 1, 2, 4 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 2, 3, 4 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 2, 4, 8 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 1, 8, 8, 8 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 100, 105, 108, 109 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 100, 100, 105, 108, 109 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 100, 105, 105, 108, 109 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 100, 105, 108, 108, 109 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 100, 105, 108, 109, 109 });
	oracleEngine.testFindFirstQueryIndexOfTick({ 100, 100, 100, 100, 100, 105, 105, 105, 108, 109, 109 });

	// random test
	std::vector<uint32_t> ticks;
	uint32_t ticksWithQueries = random(100) + 1;
	uint32_t prevTick = 100;
	for (uint32_t i = 0; i < ticksWithQueries; ++i)
	{
		const uint32_t tick = prevTick + random(10);
		const uint32_t queriesInTick = random(100) + 1;
		for (uint32_t j = 0; j < queriesInTick; ++j)
		{
			ticks.push_back(tick);
		}
		prevTick = tick;
	}
	oracleEngine.testFindFirstQueryIndexOfTick(ticks);
}
