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

	uint32_t getQueryCount() const
	{
		return oracleQueryCount;
	}

	int64_t expectPriceSubscriptionQuery(unsigned int queryIndex, QPI::DateAndTime queryTime, int64_t subscriptionId,
		std::vector<uint16_t> notifiedContractIndices, const OI::Price::OracleQuery& initialQuery)
	{
		EXPECT_GE(subscriptionId, 0);
		EXPECT_LT(subscriptionId, usedSubscriptionSlots);
		EXPECT_LT(queryIndex, oracleQueryCount);
		const OracleQueryMetadata& oqm = queries[queryIndex];
		EXPECT_EQ(oqm.interfaceIndex, OI::Price::oracleInterfaceIndex);
		EXPECT_EQ(oqm.type, ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION);
		EXPECT_EQ(oqm.typeVar.subscription.subscriptionId, subscriptionId);
		EXPECT_EQ((int)oqm.typeVar.subscription.subscriberCount, (int)notifiedContractIndices.size());
		const auto* query = (OI::Price::OracleQuery*)this->getOracleQueryPointerFromMetadata(oqm, sizeof(OI::Price::OracleQuery));
		EXPECT_NE(query, nullptr);
		EXPECT_EQ(query->oracle, initialQuery.oracle);
		EXPECT_EQ(query->currency1, initialQuery.currency1);
		EXPECT_EQ(query->currency2, initialQuery.currency2);
		EXPECT_EQ(query->timestamp, queryTime);
		const int32_t* subscriberIndices = this->getNotifiedSubscriberIndices(oqm);
		std::set<int32_t> expectedContracts(notifiedContractIndices.begin(), notifiedContractIndices.end());
		for (uint32_t i = 0; i < notifiedContractIndices.size(); ++i)
		{
			const int32_t subscriberIdx = subscriberIndices[i];
			EXPECT_GE(subscriberIdx, 0);
			EXPECT_LT(subscriberIdx, usedSubscriberSlots);
			const OracleSubscriber& subscriber = subscribers[subscriberIdx];
			EXPECT_EQ(subscriber.subscriptionId, subscriptionId);
			auto it = expectedContracts.find(subscriber.contractIndex);
			EXPECT_NE(it, expectedContracts.end());
			expectedContracts.erase(it);
		}
		EXPECT_EQ(expectedContracts.size(), 0);
		EXPECT_EQ(subscriptions[subscriptionId].interfaceIndex, OI::Price::oracleInterfaceIndex);
		EXPECT_LE(notifiedContractIndices.size(), (uint32_t)subscriptions[subscriptionId].subscriberCount);
		return oqm.queryId;
	}

	void printSubscription(int32_t subscriptionId = -1) const
	{
		if (subscriptionId < 0)
			EXPECT_TRUE(nextSubscriptionIdQueue.peek(subscriptionId));

		EXPECT_GE(subscriptionId, 0);
		EXPECT_LT(subscriptionId, usedSubscriptionSlots);
		const OracleSubscription& subscription = subscriptions[subscriptionId];
		std::cout << "Subscription " << subscriptionId << ": interface " << subscription.interfaceIndex
			<< ", " << subscription.subscriberCount << " subscribers" << std::endl;
		int cnt = 0;
		int32_t idx = subscription.firstSubscriberIndex;
		while (idx >= 0)
		{
			const OracleSubscriber& subscriber = subscribers[idx];
			std::cout << "\tcontract " << subscriber.contractIndex << ", next query " << subscriber.nextQueryTimestamp << ", period " << subscriber.notificationPeriodMinutes << std::endl;
			++cnt;
			idx = subscribers[idx].nextSubscriberIdx;
		}
		if (subscription.firstSubscriberIndex >= 0)
			EXPECT_EQ(subscription.nextQueryTimestamp, subscribers[subscription.firstSubscriberIndex].nextQueryTimestamp);
		EXPECT_EQ((int)subscription.subscriberCount, cnt);
	}

	void expectPriceNotification(uint16_t contractIndex, uint32_t notificationProcId, int64_t queryId, uint8_t status,
		int32_t subscriptionId = -1, int64_t numerator = 0, int64_t denominator = 0)
	{
		const OracleNotificationData* notification = getNotification();
		EXPECT_NE(notification, nullptr);
		EXPECT_EQ((int)notification->contractIndex, (int)contractIndex);
		EXPECT_EQ(notification->procedureId, notificationProcId);
		EXPECT_EQ((int)notification->inputSize, sizeof(OracleNotificationInput<OI::Price>));
		const auto* notificationInput = (const OracleNotificationInput<OI::Price>*) & notification->inputBuffer;
		EXPECT_EQ(notificationInput->queryId, queryId);
		EXPECT_EQ(notificationInput->status, status);
		EXPECT_EQ(notificationInput->subscriptionId, subscriptionId);
		EXPECT_EQ(notificationInput->reply.numerator, numerator);
		EXPECT_EQ(notificationInput->reply.denominator, denominator);
	}

	void expectPriceNotifications(const std::vector<uint16_t> contractIndices,
		uint32_t notificationProcId, int64_t queryId, uint8_t status,
		int32_t subscriptionId = -1, int64_t numerator = 0, int64_t denominator = 0)
	{
		if (subscriptionId >= 0)
		{
			uint32_t queryIndex = 0;
			EXPECT_TRUE(queryIdToIndex->get(queryId, queryIndex));
			EXPECT_EQ(queries[queryIndex].type, ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION);
			EXPECT_EQ((size_t)queries[queryIndex].typeVar.subscription.subscriberCount, contractIndices.size());
		}
		std::set<int32_t> expectedContracts(contractIndices.begin(), contractIndices.end());
		for (size_t i = 0; i < contractIndices.size(); ++i)
		{
			const OracleNotificationData* notification = getNotification();
			EXPECT_NE(notification, nullptr);
			EXPECT_EQ(notification->procedureId, notificationProcId);
			EXPECT_EQ((int)notification->inputSize, sizeof(OracleNotificationInput<OI::Price>));
			const auto* notificationInput = (const OracleNotificationInput<OI::Price>*) & notification->inputBuffer;
			EXPECT_EQ(notificationInput->subscriptionId, subscriptionId);
			EXPECT_EQ(notificationInput->queryId, queryId);
			EXPECT_EQ(notificationInput->status, status);
			EXPECT_EQ(notificationInput->reply.numerator, numerator);
			EXPECT_EQ(notificationInput->reply.denominator, denominator);

			auto it = expectedContracts.find(notification->contractIndex);
			EXPECT_NE(it, expectedContracts.end());
			expectedContracts.erase(it);
		}
		EXPECT_EQ(expectedContracts.size(), 0);
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
	oracleEngine1.expectPriceNotification(contractIndex, notificationProcId, replyRevealTx->queryId,
		ORACLE_QUERY_STATUS_SUCCESS, -1, 1234, 1);

	// no additional notifications
	EXPECT_EQ(oracleEngine1.getNotification(), nullptr);

	EXPECT_EQ(oracleEngine1.getOracleQueryStatus(queryId), ORACLE_QUERY_STATUS_SUCCESS);

	OI::Price::OracleReply reply;
	EXPECT_TRUE(oracleEngine1.getOracleReply(queryId, &reply, sizeof(reply)));
	EXPECT_EQ(reply.numerator, 1234);
	EXPECT_EQ(reply.denominator, 1);

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
	// -> no quorum can be reached and status changes from pending to unresolvable directly
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
	oracleEngine1.expectPriceNotification(contractIndex, notificationProcId, queryId, ORACLE_QUERY_STATUS_UNRESOLVABLE);

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
	oracleEngine1.expectPriceNotification(contractIndex, notificationProcId, queryId, ORACLE_QUERY_STATUS_UNRESOLVABLE);

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
	oracleEngine1.expectPriceNotification(contractIndex, notificationProcId, queryId, ORACLE_QUERY_STATUS_TIMEOUT);

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
		EXPECT_EQ(notificationInput->subscriptionId, -1);
		
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

// For simplified checking of timestamps define + for adding minutes
static QPI::DateAndTime operator+(QPI::DateAndTime dt, int minutes)
{
	EXPECT_TRUE(dt.add(0, 0, 0, 0, minutes, 0));
	return dt;
}

TEST(OracleEngine, Subscription)
{
	OracleEngineTest test;
	OracleEngineWithInitAndDeinit oracleEngine(broadcastedComputors.computors.publicKeys, 0, 676);

	QPI::uint16 interfaceIndex = OI::Price::oracleInterfaceIndex;
	uint32_t notificationPeriodMillisec = 60000;
	OI::Price::OracleQuery priceQuery0;
	priceQuery0.currency1 = QPI::id(QPI::Ch::B, QPI::Ch::T, QPI::Ch::C, 0, 0);
	priceQuery0.currency2 = QPI::id(QPI::Ch::U, QPI::Ch::S, QPI::Ch::D, QPI::Ch::T, 0);
	priceQuery0.oracle = OI::Price::getBinanceOracleId();
	const QPI::uint32 notificationProcId = 12345;

	// new subscription: success
	// -> subscription 0 for QX: t0 + N (each minute)
	auto t0 = QPI::DateAndTime::now();
	int32_t subscriptionId0 = oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, 0);

	// new subscription: failure cases (invalid input parameters)
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(2000, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex - 1, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex + 1000, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0) + 1,
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		0, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		10, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec + 1, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		0xffffffff, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, 1024));

	// fail: same subscription from the same contract
	EXPECT_EQ(-1, oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp)));

	// okay: same subscription from a different contract with same notification period
	// -> subscription 0 for QEARN: t0 + 2 * N (each 2 minutes)
	int32_t subscriptionId0b = oracleEngine.startContractSubscription(QEARN_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec * 2, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0b);

	// okay: same subscription from a another contract with different notification period
	// -> subscription 0 for QUTIL: t0 + 5 * N (each 5 minutes)
	int32_t subscriptionId0c = oracleEngine.startContractSubscription(QUTIL_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec * 5, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0c);

	// generate and check queries
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 1);
	const int64_t qid0 = oracleEngine.expectPriceSubscriptionQuery(0, t0, subscriptionId0,
		{ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX, QUTIL_CONTRACT_INDEX }, priceQuery0);

	// advance time by 500 ms -> t1 = t0 + 0.5/60
	advanceTimeAndTick(500);
	auto t1 = QPI::DateAndTime::now();

	// new subscription to a different oracle with two periods
	// -> subscription 1 for QX: t1 + 10 * N (each 10 minutes)
	// -> subscription 1 for RANDOM: t1 + 12 * N (each 12 minutes)
	OI::Price::OracleQuery priceQuery1 = priceQuery0;
	priceQuery1.oracle = OI::Price::getGateOracleId();
	uint32_t notificationPeriod1a = 10 * 60000;
	uint32_t notificationPeriod1b = 12 * 60000;
	int32_t subscriptionId1 = oracleEngine.startContractSubscription(QX_CONTRACT_INDEX, interfaceIndex, &priceQuery1, sizeof(priceQuery1),
		notificationPeriod1a, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId1, 1);
	int32_t subscriptionId1b = oracleEngine.startContractSubscription(RANDOM_CONTRACT_INDEX, interfaceIndex, &priceQuery1, sizeof(priceQuery1),
		notificationPeriod1b, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId1b, subscriptionId1);

	// generate and check queries
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 2);
	const int64_t qid1 = oracleEngine.expectPriceSubscriptionQuery(1, t1, subscriptionId1,
		{ QX_CONTRACT_INDEX, RANDOM_CONTRACT_INDEX }, priceQuery1);

	// advance time by 400 ms -> t2 = t0 + 0.9/60 = t1 + 0.4/60
	advanceTimeAndTick(400);
	auto t2 = QPI::DateAndTime::now();

	// new subscribers to second subscription with two periods
	// -> subscription 1 for QUTIL: t1 + 5 * N (each 5 minutes)
	// -> subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	uint32_t notificationPeriod1c = 5 * 60000;
	uint32_t notificationPeriod1d = 3 * 60000;
	int32_t subscriptionId1c = oracleEngine.startContractSubscription(QUTIL_CONTRACT_INDEX, interfaceIndex, &priceQuery1, sizeof(priceQuery1),
		notificationPeriod1c, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId1c, subscriptionId1);
	int32_t subscriptionId1d = oracleEngine.startContractSubscription(QUOTTERY_CONTRACT_INDEX, interfaceIndex, &priceQuery1, sizeof(priceQuery1),
		notificationPeriod1d, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId1d, subscriptionId1);

	// generate and check queries (no new queries, because QUTIL/QUOTTERY got synced with existing queries)
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 2);

	// no notification yet
	oracleEngine.processTimeouts();
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// advance time by 100 ms + 59 sec -> t3 = t0 + 1
	advanceTimeAndTick(59100);
	auto t3 = QPI::DateAndTime::now();

	// generate and check queries: triggered subscription 0 for QX: t0 + N (each minute)
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 3);
	const int64_t qid2 = oracleEngine.expectPriceSubscriptionQuery(2, t0 + 1, subscriptionId0,
		{ QX_CONTRACT_INDEX }, priceQuery0);

	// notification: timeout of first query (t0)
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX, QUTIL_CONTRACT_INDEX },
		notificationProcId, qid0, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// advance time by 100 ms + 60 sec -> t4 = t0 + 2 + 0.1/60
	advanceTimeAndTick(60100);
	auto t4 = QPI::DateAndTime::now();

	// generate and check queries: triggered subscription 0 for QX: t0 + N (each minute) and QEARN: t0 + 2 * N
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 4);
	int64_t qid3 = oracleEngine.expectPriceSubscriptionQuery(3, t0 + 2, subscriptionId0,
		{ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX }, priceQuery0);

	// notification: timeout of t0 + 1, t1 + 1
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX, RANDOM_CONTRACT_INDEX },
		notificationProcId, qid1, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX },
		notificationProcId, qid2, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// advance time by 61 sec -> t5 = t0 + 3 + 1.1/60
	advanceTimeAndTick(61000);
	auto t5 = QPI::DateAndTime::now();

	// generate and check queries:
	// - triggered subscription 0 for QX: t0 + N (each minute)
	// - triggered subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 6);
	const int64_t qid4 = oracleEngine.expectPriceSubscriptionQuery(4, t0 + 3, subscriptionId0,
		{ QX_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid5 = oracleEngine.expectPriceSubscriptionQuery(5, t1 + 3, subscriptionId1,
		{ QUOTTERY_CONTRACT_INDEX }, priceQuery1);

	// notification: timeout of t0 + 2
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX },
		notificationProcId, qid3, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// simulate stuck network
	// advance time by 120 sec -> t6 = t0 + 5 + 1.1/60
	advanceTimeAndTick(120000);
	auto t6 = QPI::DateAndTime::now();

	// generate and check queries:
	// - triggered subscription 0 for QX: t0 + N (each minute), first query is generated, one is skipped
	// - triggered subscription 0 for QEARN: t0 + 2 * N
	// - triggered subscription 0 for QUTIL: t0 + 5 * N
	// - triggered subscription 1 for QUTIL: t1 + 5 * N (each 5 minutes)
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 9);
	const int64_t qid6 = oracleEngine.expectPriceSubscriptionQuery(6, t0 + 4, subscriptionId0,
		{ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid7 = oracleEngine.expectPriceSubscriptionQuery(7, t0 + 5, subscriptionId0,
		{ QUTIL_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid8 = oracleEngine.expectPriceSubscriptionQuery(8, t1 + 5, subscriptionId1,
		{ QUTIL_CONTRACT_INDEX }, priceQuery1);

	// notification: timeout of t0 + 3, t1 + 3, and t0 + 4
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX },
		notificationProcId, qid4, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QUOTTERY_CONTRACT_INDEX },
		notificationProcId, qid5, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX },
		notificationProcId, qid6, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// advance time by 59.4 sec -> t7 = t0 + 6 + 0.5/60
	advanceTimeAndTick(59400);
	auto t7 = QPI::DateAndTime::now();

	// generate and check queries:
	// - triggered subscription 0 for QX: t0 + N (each minute)
	// - triggered subscription 0 for QEARN: t0 + 2 * N
	// - triggered subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 11);
	const int64_t qid9 = oracleEngine.expectPriceSubscriptionQuery(9, t0 + 6, subscriptionId0,
		{ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid10 = oracleEngine.expectPriceSubscriptionQuery(10, t1 + 6, subscriptionId1,
		{ QUOTTERY_CONTRACT_INDEX }, priceQuery1);

	// notification: timeout of t0 + 5, t1 + 5
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QUTIL_CONTRACT_INDEX },
		notificationProcId, qid7, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QUTIL_CONTRACT_INDEX },
		notificationProcId, qid8, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// simulate stuck network
	// advance time by 240 sec -> t8 = t0 + 10 + 0.5/60
	advanceTimeAndTick(240000);
	auto t8 = QPI::DateAndTime::now();

	// generate and check queries:
	// - triggered subscription 0 for QX: t0 + N (each minute) -> generate one, skip until now
	// - triggered subscription 0 for QEARN: t0 + 2 * N (each 2 minutes) -> generate one, skip until now
	// - triggered subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	// - triggered subscription 0 for QUTIL: t0 + 5 * N (each 5 minutes)
	// - triggered subscription 1 for QUTIL: t1 + 5 * N (each 5 minutes)
	// - triggered subscription 1 for QX: t1 + 10 * N (each 10 minutes)
	oracleEngine.generateSubscriptionQueries();

	EXPECT_EQ(oracleEngine.getQueryCount(), 16);
	const int64_t qid11 = oracleEngine.expectPriceSubscriptionQuery(11, t0 + 7, subscriptionId0,
		{ QX_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid12 = oracleEngine.expectPriceSubscriptionQuery(12, t0 + 8, subscriptionId0,
		{ QEARN_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid13 = oracleEngine.expectPriceSubscriptionQuery(13, t1 + 9, subscriptionId1,
		{ QUOTTERY_CONTRACT_INDEX }, priceQuery1);
	const int64_t qid14 = oracleEngine.expectPriceSubscriptionQuery(14, t0 + 10, subscriptionId0,
		{ QUTIL_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid15 = oracleEngine.expectPriceSubscriptionQuery(15, t1 + 10, subscriptionId1,
		{ QUTIL_CONTRACT_INDEX, QX_CONTRACT_INDEX }, priceQuery1);

	// notification: timeout of t0 + 6, t1 + 6, t0 + 7, t0 + 8, t1 + 9
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX, QEARN_CONTRACT_INDEX },
		notificationProcId, qid9, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QUOTTERY_CONTRACT_INDEX },
		notificationProcId, qid10, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX },
		notificationProcId, qid11, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QEARN_CONTRACT_INDEX },
		notificationProcId, qid12, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QUOTTERY_CONTRACT_INDEX },
		notificationProcId, qid13, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// simulate stuck network
	// advance time by 130 sec -> t9 = t0 + 12 + 10.5/60
	advanceTimeAndTick(130000);
	auto t9 = QPI::DateAndTime::now();

	// generate and check queries:
	// - triggered subscription 0 for QX: t0 + N (each minute) -> generate one, skip until now
	// - triggered subscription 0 for QEARN: t0 + 2 * N (each 2 minutes)
	// - triggered subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	// - triggered subscription 1 for RANDOM: t1 + 12 * N (each 12 minutes)
	oracleEngine.generateSubscriptionQueries();

	//oracleEngine.printSubscription(0);
	//oracleEngine.printSubscription(1);

	EXPECT_EQ(oracleEngine.getQueryCount(), 19);
	const int64_t qid16 = oracleEngine.expectPriceSubscriptionQuery(16, t0 + 11, subscriptionId0,
		{ QX_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid17 = oracleEngine.expectPriceSubscriptionQuery(17, t0 + 12, subscriptionId0,
		{ QEARN_CONTRACT_INDEX }, priceQuery0);
	const int64_t qid18 = oracleEngine.expectPriceSubscriptionQuery(18, t1 + 12, subscriptionId1,
		{ QUOTTERY_CONTRACT_INDEX, RANDOM_CONTRACT_INDEX }, priceQuery1);

	// notification: timeout of t0 + 10, t1 + 10, t0 + 11
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QUTIL_CONTRACT_INDEX },
		notificationProcId, qid14, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QUTIL_CONTRACT_INDEX, QX_CONTRACT_INDEX },
		notificationProcId, qid15, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	oracleEngine.expectPriceNotifications({ QX_CONTRACT_INDEX },
		notificationProcId, qid16, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	oracleEngine.checkStateConsistencyWithAssert();

	// unsubscribe subscription 0 for QX: t0 + N (each minute)
	oracleEngine.stopContractSubscription(subscriptionId0, QX_CONTRACT_INDEX);

	// Remaining subscriptions:
	// -> subscription 0 for QEARN: t0 + 2 * N (each 2 minutes)
	// -> subscription 0 for QUTIL: t0 + 5 * N (each 5 minutes)
	// -> subscription 1 for QX: t1 + 10 * N (each 10 minutes)
	// -> subscription 1 for RANDOM: t1 + 12 * N (each 12 minutes)
	// -> subscription 1 for QUTIL: t1 + 5 * N (each 5 minutes)
	// -> subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	oracleEngine.checkStateConsistencyWithAssert();

	// advance time by 50 sec -> t10 = t0 + 13 + 0.5/60
	advanceTimeAndTick(50000);

	// generate and check queries: none
	oracleEngine.generateSubscriptionQueries();
	EXPECT_EQ(oracleEngine.getQueryCount(), 19);

	// notification: timeout of t0 + 12, t1 + 12
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications({ QEARN_CONTRACT_INDEX },
		notificationProcId, qid17, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications({ QUOTTERY_CONTRACT_INDEX, RANDOM_CONTRACT_INDEX },
		notificationProcId, qid18, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// add new subscriptions
	// -> subscription 0 for RANDOM: t0 + 14 + N (each minute)
	// -> subscription 0 for QBAY: t0 + 14 + N (each minute)
	// -> subscription 0 for MSVAULT: t0 + 14 + 3 * N (each 3 minutes)
	// -> subscription 0 for CCF: t0 + 14 + 8 * N (each 8 minutes)
	// -> subscription 0 for SWATCH: t0 + 15 + 15 * N (each 15 minutes)
	int32_t subscriptionId0d = oracleEngine.startContractSubscription(RANDOM_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0d);
	int32_t subscriptionId0e = oracleEngine.startContractSubscription(QBAY_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0e);
	int32_t subscriptionId0f = oracleEngine.startContractSubscription(MSVAULT_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		3 * notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0f);
	int32_t subscriptionId0g = oracleEngine.startContractSubscription(CCF_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		8 * notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0g);
	int32_t subscriptionId0h = oracleEngine.startContractSubscription(SWATCH_CONTRACT_INDEX, interfaceIndex, &priceQuery0, sizeof(priceQuery0),
		15 * notificationPeriodMillisec, notificationProcId, offsetof(OI::Price::OracleQuery, timestamp));
	EXPECT_EQ(subscriptionId0, subscriptionId0h);

	// advance time by 60 sec -> t11 = t0 + 14 + 0.5/60
	advanceTimeAndTick(60000);

	//oracleEngine.printSubscription(0);
	//oracleEngine.printSubscription(1);

	// generate and check queries:
	// - triggered subscription 0 for QEARN: t0 + 2 * N (each 2 minutes)
	// - triggered subscription 0 for RANDOM: t0 + 14 + N (each minute)
	// - triggered subscription 0 for QBAY: t0 + 14 + N (each minute)
	// - triggered subscription 0 for MSVAULT: t0 + 14 + 3 * N (each 3 minutes)
	// - triggered subscription 0 for CCF: t0 + 14 + 8 * N (each 8 minutes)
	oracleEngine.generateSubscriptionQueries();
	const int64_t qid19 = oracleEngine.expectPriceSubscriptionQuery(19, t0 + 14, subscriptionId0,
		{ QEARN_CONTRACT_INDEX, RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX, CCF_CONTRACT_INDEX },
		priceQuery0);
	EXPECT_EQ(oracleEngine.getQueryCount(), 20);

	// no notifications
	oracleEngine.processTimeouts();
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// advance time by 60 sec -> t12 = t0 + 15 + 0.5/60
	advanceTimeAndTick(60000);

	//oracleEngine.printSubscription(0);
	//oracleEngine.printSubscription(1);

	// generate and check queries:
	// - subscription 0 for RANDOM: t0 + 14 + N (each minute)
	// - subscription 0 for QBAY: t0 + 14 + N (each minute)
	// - subscription 0 for SWATCH: t0 + 15 + 15 * N (each 15 minutes)
	// - subscription 0 for QUTIL: t0 + 5 * N (each 5 minutes)
	// - subscription 1 for QUTIL: t1 + 5 * N (each 5 minutes)
	// - subscription 1 for QUOTTERY: t1 + 3 * N (each 3 minutes)
	oracleEngine.generateSubscriptionQueries();
	const int64_t qid20 = oracleEngine.expectPriceSubscriptionQuery(20, t0 + 15, subscriptionId0,
		{ RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, SWATCH_CONTRACT_INDEX, QUTIL_CONTRACT_INDEX },
		priceQuery0);
	const int64_t qid21 = oracleEngine.expectPriceSubscriptionQuery(21, t1 + 15, subscriptionId1,
		{ QUTIL_CONTRACT_INDEX, QUOTTERY_CONTRACT_INDEX },
		priceQuery1);
	EXPECT_EQ(oracleEngine.getQueryCount(), 22);

	// notification: timeout of t0 + 14
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications(
		{ QEARN_CONTRACT_INDEX, RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX, CCF_CONTRACT_INDEX },
		notificationProcId, qid19, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// unsubscribe some subscriptions
	EXPECT_TRUE(oracleEngine.stopContractSubscription(subscriptionId0, SWATCH_CONTRACT_INDEX));
	EXPECT_FALSE(oracleEngine.stopContractSubscription(subscriptionId0, SWATCH_CONTRACT_INDEX));
	EXPECT_TRUE(oracleEngine.stopContractSubscription(subscriptionId1, RANDOM_CONTRACT_INDEX));
	EXPECT_TRUE(oracleEngine.stopContractSubscription(subscriptionId1, QX_CONTRACT_INDEX));
	EXPECT_TRUE(oracleEngine.stopContractSubscription(subscriptionId1, QUTIL_CONTRACT_INDEX));
	EXPECT_TRUE(oracleEngine.stopContractSubscription(subscriptionId1, QUOTTERY_CONTRACT_INDEX));

	// Remaining subscriptions:
	// -> subscription 0 for QEARN: t0 + 2 * N (each 2 minutes)
	// -> subscription 0 for QUTIL: t0 + 5 * N (each 5 minutes)
    // -> subscription 0 for RANDOM: t0 + 14 + N (each minute)
	// -> subscription 0 for QBAY: t0 + 14 + N (each minute)
	// -> subscription 0 for MSVAULT: t0 + 14 + 3 * N (each 3 minutes)
	// -> subscription 0 for CCF: t0 + 14 + 8 * N (each 8 minutes)
	// -> subscription 1 ALL SUBSCRIBED
	oracleEngine.checkStateConsistencyWithAssert();

	// advance time by 60 sec -> t13 = t0 + 16 + 0.5/60
	advanceTimeAndTick(60000);

	// generate and check queries:
	// - subscription 0 for RANDOM: t0 + 14 + N (each minute)
	// - subscription 0 for QBAY: t0 + 14 + N (each minute)
	// - subscription 0 for QEARN: t0 + 2 * N (each 2 minutes)
	oracleEngine.generateSubscriptionQueries();
	const int64_t qid22 = oracleEngine.expectPriceSubscriptionQuery(22, t0 + 16, subscriptionId0,
		{ RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, QEARN_CONTRACT_INDEX },
		priceQuery0);
	EXPECT_EQ(oracleEngine.getQueryCount(), 23);

	// notification: timeout of t0 + 15, t1 + 15
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications(
		{ RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, SWATCH_CONTRACT_INDEX, QUTIL_CONTRACT_INDEX },
		notificationProcId, qid20, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications(
		{ QUTIL_CONTRACT_INDEX, QUOTTERY_CONTRACT_INDEX },
		notificationProcId, qid21, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId1);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	// advance time by 20 * 60 sec -> t13 = t0 + 36 + 0.5/60
	advanceTimeAndTick(20 * 60000);

	//oracleEngine.printSubscription(0);
	//oracleEngine.printSubscription(1);

	// generate and check queries:
	oracleEngine.generateSubscriptionQueries();
	const int64_t qid23 = oracleEngine.expectPriceSubscriptionQuery(23, t0 + 17, subscriptionId0,
		{ RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX },
		priceQuery0);
	const int64_t qid24 = oracleEngine.expectPriceSubscriptionQuery(24, t0 + 18, subscriptionId0,
		{ QEARN_CONTRACT_INDEX },
		priceQuery0);
	const int64_t qid25 = oracleEngine.expectPriceSubscriptionQuery(25, t0 + 20, subscriptionId0,
		{ QUTIL_CONTRACT_INDEX },
		priceQuery0);
	const int64_t qid26 = oracleEngine.expectPriceSubscriptionQuery(26, t0 + 22, subscriptionId0,
		{ CCF_CONTRACT_INDEX },
		priceQuery0);
	EXPECT_EQ(oracleEngine.getQueryCount(), 27);

	// notification: timeout of all
	oracleEngine.processTimeouts();
	oracleEngine.expectPriceNotifications(
		{ RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, QEARN_CONTRACT_INDEX },
		notificationProcId, qid22, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications(
		{ RANDOM_CONTRACT_INDEX, QBAY_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX },
		notificationProcId, qid23, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications(
		{ QEARN_CONTRACT_INDEX },
		notificationProcId, qid24, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications(
		{ QUTIL_CONTRACT_INDEX },
		notificationProcId, qid25, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	oracleEngine.expectPriceNotifications(
		{ CCF_CONTRACT_INDEX },
		notificationProcId, qid26, ORACLE_QUERY_STATUS_TIMEOUT, subscriptionId0);
	EXPECT_EQ(oracleEngine.getNotification(), nullptr);

	oracleEngine.checkStateConsistencyWithAssert();
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

TEST(PriceOracle, SubscriptionFee)
{
	using OI::Price;
	Price::OracleQuery query;
	EXPECT_EQ(Price::getSubscriptionFee(query, 1 * 60000), 10000);
	EXPECT_EQ(Price::getSubscriptionFee(query, 2 * 60000), 6500);
	EXPECT_EQ(Price::getSubscriptionFee(query, 3 * 60000), 6500);
	EXPECT_EQ(Price::getSubscriptionFee(query, 4 * 60000), 4225);
	EXPECT_EQ(Price::getSubscriptionFee(query, 7 * 60000), 4225);
	EXPECT_EQ(Price::getSubscriptionFee(query, 8 * 60000), 2746);
	EXPECT_EQ(Price::getSubscriptionFee(query, 15 * 60000), 2746);
	EXPECT_EQ(Price::getSubscriptionFee(query, 16 * 60000), 1784);
	EXPECT_EQ(Price::getSubscriptionFee(query, 31 * 60000), 1784);
	EXPECT_EQ(Price::getSubscriptionFee(query, 32 * 60000), 1159);
	EXPECT_EQ(Price::getSubscriptionFee(query, 63 * 60000), 1159);
	EXPECT_EQ(Price::getSubscriptionFee(query, 64 * 60000), 753);
	EXPECT_EQ(Price::getSubscriptionFee(query, 127 * 60000), 753);
	EXPECT_EQ(Price::getSubscriptionFee(query, 128 * 60000), 489);
	EXPECT_EQ(Price::getSubscriptionFee(query, 255 * 60000), 489);
	EXPECT_EQ(Price::getSubscriptionFee(query, 256 * 60000), 317);
	EXPECT_EQ(Price::getSubscriptionFee(query, 511 * 60000), 317);
	EXPECT_EQ(Price::getSubscriptionFee(query, 512 * 60000), 206);
	EXPECT_EQ(Price::getSubscriptionFee(query, 1023 * 60000), 206);
	EXPECT_EQ(Price::getSubscriptionFee(query, 1024 * 60000), 133);
	EXPECT_EQ(Price::getSubscriptionFee(query, 2047 * 60000), 133);
}
