using namespace QPI;

struct TESTEXC2
{
};

struct TESTEXC : public ContractBase
{
	//---------------------------------------------------------------
	// ASSET MANAGEMENT RIGHTS TRANSFER

	struct GetTestExampleAShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
	};
	struct GetTestExampleAShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

protected:
	int waitInPreAcqiuireSharesCallback;
	
	struct GetTestExampleAShareManagementRights_locals
	{
		TESTEXA::TransferShareManagementRights_input input;
		TESTEXA::TransferShareManagementRights_output output;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(GetTestExampleAShareManagementRights)
	{
		// This is for the ResolveDeadlockCallbackProcedureAndConcurrentFunction in test/contract_testex.cpp:
		locals.input.asset = input.asset;
		locals.input.numberOfShares = input.numberOfShares;
		locals.input.newManagingContractIndex = SELF_INDEX;
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXA, TransferShareManagementRights, locals.input, locals.output, qpi.invocationReward());
		output.transferredNumberOfShares = locals.output.transferredNumberOfShares;
	}

	struct PRE_ACQUIRE_SHARES_locals
	{
		TESTEXA::RunHeavyComputation_input heavyComputationInput;
		TESTEXA::RunHeavyComputation_output heavyComputationOutput;
		TESTEXB::SetPreAcquireSharesOutput_input textExBInput;
		TESTEXB::SetPreAcquireSharesOutput_output textExBOutput;
	};

	PRE_ACQUIRE_SHARES_WITH_LOCALS()
	{
		// This is for the ResolveDeadlockCallbackProcedureAndConcurrentFunction in test/contract_testex.cpp:
		// 1. Check reuse of already owned write lock of TESTEXA and delay execution in order to make sure that the
		//    concurrent contract function TESTEXB::CallTextExAFunc() is running or waiting for read lock of TEXTEXA.
		INVOKE_OTHER_CONTRACT_PROCEDURE_E(TESTEXA, RunHeavyComputation, locals.heavyComputationInput, locals.heavyComputationOutput, 0, callError1);

		// 2. Try to invoke procedure of TESTEXB to trigger deadlock (waiting for release of read lock).
#ifdef NO_UEFI
		printf("Before wait/deadlock in contract %u procedure\n", CONTRACT_INDEX);
#endif
		INVOKE_OTHER_CONTRACT_PROCEDURE_E(TESTEXB, SetPreAcquireSharesOutput, locals.textExBInput, locals.textExBOutput, 0, callError2);
#ifdef NO_UEFI
		printf("After wait/deadlock in contract %u procedure\n", CONTRACT_INDEX);
#endif

		// bug check regarding size of locals
		ASSERT(!locals.textExBInput.allowTransfer);
		ASSERT(!locals.textExBInput.requestedFee);
	}

	//---------------------------------------------------------------
	// POST_INCOMING_TRANSFER CALLBACK
public:
	typedef NoData IncomingTransferAmounts_input;
	struct IncomingTransferAmounts_output
	{
		sint64 standardTransactionAmount;
		sint64 procedureTransactionAmount;
		sint64 qpiTransferAmount;
		sint64 qpiDistributeDividendsAmount;
		sint64 revenueDonationAmount;
		sint64 ipoBidRefundAmount;
	};

	struct QpiTransfer_input
	{
		id destinationPublicKey;
		sint64 amount;
	};
	typedef NoData QpiTransfer_output;

	struct QpiDistributeDividends_input
	{
		sint64 amountPerShare;
	};
	typedef NoData QpiDistributeDividends_output;

protected:
	IncomingTransferAmounts_output incomingTransfers;

	PUBLIC_FUNCTION(IncomingTransferAmounts)
	{
		output = state.incomingTransfers;
	}

	struct POST_INCOMING_TRANSFER_locals
	{
		Entity before;
		Entity after;
		TESTEXB::QpiTransfer_input input;
		TESTEXB::QpiTransfer_output output;
	};

	POST_INCOMING_TRANSFER_WITH_LOCALS()
	{
		ASSERT(input.amount > 0);
		switch (input.type)
		{
		case TransferType::standardTransaction:
			state.incomingTransfers.standardTransactionAmount += input.amount;
			break;
		case TransferType::procedureTransaction:
			state.incomingTransfers.procedureTransactionAmount += input.amount;
			break;
		case TransferType::qpiTransfer:
			state.incomingTransfers.qpiTransferAmount += input.amount;
			break;
		case TransferType::qpiDistributeDividends:
			state.incomingTransfers.qpiDistributeDividendsAmount += input.amount;
			break;
		case TransferType::revenueDonation:
			state.incomingTransfers.revenueDonationAmount += input.amount;
			break;
		case TransferType::ipoBidRefund:
			state.incomingTransfers.ipoBidRefundAmount += input.amount;
			break;
		default:
			ASSERT(false);
		}

		// check that everything that transfers QUs is disabled
		ASSERT(qpi.transfer(SELF, 1000) < 0);
		ASSERT(!qpi.distributeDividends(10));

		// procedures are invoked, but without transfer of invocation reward
		ASSERT(qpi.getEntity(SELF, locals.before));
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXB, QpiTransfer, locals.input, locals.output, 1000);
		ASSERT(qpi.getEntity(SELF, locals.after));
		ASSERT(locals.before.outgoingAmount == locals.after.outgoingAmount);
	}

	PUBLIC_PROCEDURE(QpiTransfer)
	{
		qpi.transfer(input.destinationPublicKey, input.amount);
	}

	PUBLIC_PROCEDURE(QpiDistributeDividends)
	{
		qpi.distributeDividends(input.amountPerShare);
	}

	//---------------------------------------------------------------
	// IPO TEST
public:
	struct GetIpoBid_input
	{
		uint32 ipoContractIndex;
		uint32 bidIndex;
	};
	struct GetIpoBid_output
	{
		id publicKey;
		sint64 price;
	};

	struct QpiBidInIpo_input
	{
		uint32 ipoContractIndex;
		sint64 pricePerShare;
		uint16 numberOfShares;
	};
	typedef sint64 QpiBidInIpo_output;

	PUBLIC_FUNCTION(GetIpoBid)
	{
		output.price = qpi.ipoBidPrice(input.ipoContractIndex, input.bidIndex);
		output.publicKey = qpi.ipoBidId(input.ipoContractIndex, input.bidIndex);
	}

	PUBLIC_PROCEDURE(QpiBidInIpo)
	{
		output = qpi.bidInIPO(input.ipoContractIndex, input.pricePerShare, input.numberOfShares);
	}

	//---------------------------------------------------------------
	// ORACLE TESTING

	struct NotificationLog
	{
		uint32 contractIndex;
		uint8 interface;
		uint8 status;
		uint16 dataCheck;
		sint64 data;
		sint64 queryId;
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	// optional: additional of contract data associated with oracle query
	HashMap<uint64, uint32, 64> oracleQueryExtraData;

	uint32 oracleSubscriptionId;

	struct QueryPriceOracle_input
	{
		OI::Price::OracleQuery priceOracleQuery;
		uint32 timeoutMilliseconds;
	};
	struct QueryPriceOracle_output
	{
		sint64 oracleQueryId;
	};

	PUBLIC_PROCEDURE(QueryPriceOracle)
	{
		output.oracleQueryId = QUERY_ORACLE(OI::Price, input.priceOracleQuery, NotifyPriceOracleReply, input.timeoutMilliseconds);
		if (output.oracleQueryId < 0)
		{
			// error
			return;
		}

		// example: store additional data realted to oracle query
		state.oracleQueryExtraData.set(output.oracleQueryId, 0);
	}

	struct SubscribePriceOracle_input
	{
		OI::Price::OracleQuery priceOracleQuery;
		uint16 subscriptionIntervalMinutes;
	};
	struct SubscribePriceOracle_output
	{
		uint32 oracleSubscriptionId;
	};

	PUBLIC_PROCEDURE(SubscribePriceOracle)
	{
		output.oracleSubscriptionId = SUBSCRIBE_ORACLE(OI::Price, input.priceOracleQuery, NotifyPriceOracleReply, input.subscriptionIntervalMinutes, true);
		if (output.oracleSubscriptionId < 0)
		{
			// error
		}
	}

	typedef OracleNotificationInput<OI::Price> NotifyPriceOracleReply_input;
	typedef NoData NotifyPriceOracleReply_output;
	struct NotifyPriceOracleReply_locals
	{
		OI::Price::OracleQuery query;
		uint32 queryExtraData;
		NotificationLog notificationLog;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(NotifyPriceOracleReply)
	{
		locals.notificationLog = NotificationLog{CONTRACT_INDEX, OI::Price::oracleInterfaceIndex, input.status, OI::Price::replyIsValid(input.reply), input.reply.numerator, input.queryId };
		LOG_INFO(locals.notificationLog);

		if (input.status == ORACLE_QUERY_STATUS_SUCCESS)
		{
			// get and use query info if needed
			if (!qpi.getOracleQuery<OI::Price>(input.queryId, locals.query))
				return;
			
			// get and use additional query info stored by contract if needed
			if (!state.oracleQueryExtraData.get(input.queryId, locals.queryExtraData))
				return;

			// use example convenience function provided by oracle interface
			if (!OI::Price::replyIsValid(input.reply))
				return;
		}
		else
		{
			// handle failure ...
		}
	}

	// MOCK ORACLE TESTING
	typedef OracleNotificationInput<OI::Mock> NotifyMockOracleReply_input;
	typedef NoData NotifyMockOracleReply_output;
	struct NotifyMockOracleReply_locals
	{
		OI::Mock::OracleQuery query;
		OI::Mock::OracleReply reply;
		uint32 queryExtraData;
		NotificationLog notificationLog;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(NotifyMockOracleReply)
	{
		locals.notificationLog = NotificationLog{ CONTRACT_INDEX, OI::Mock::oracleInterfaceIndex, input.status, 0, (sint64)input.reply.echoedValue, input.queryId };

		ASSERT(qpi.getOracleQueryStatus(input.queryId) == input.status);
		if (input.status == ORACLE_QUERY_STATUS_SUCCESS)
		{
			// success
			if (qpi.getOracleQuery<OI::Mock>(input.queryId, locals.query))
			{
				ASSERT(locals.query.value == input.reply.echoedValue);
				ASSERT(locals.query.value == input.reply.doubledValue / 2);

				locals.notificationLog.dataCheck = OI::Mock::replyIsValid(locals.query, input.reply);
			}
			ASSERT(qpi.getOracleQueryStatus(input.queryId) == ORACLE_QUERY_STATUS_SUCCESS);
			ASSERT(qpi.getOracleReply<OI::Mock>(input.queryId, locals.reply));
			ASSERT(locals.reply.echoedValue == input.reply.echoedValue);
			ASSERT(locals.reply.doubledValue == input.reply.doubledValue);
		}
		else
		{
			// handle failure ...
			ASSERT(qpi.getOracleQueryStatus(input.queryId) == ORACLE_QUERY_STATUS_TIMEOUT || qpi.getOracleQueryStatus(input.queryId) == ORACLE_QUERY_STATUS_UNRESOLVABLE);
			ASSERT(!qpi.getOracleReply<OI::Mock>(input.queryId, locals.reply));
		}

		LOG_INFO(locals.notificationLog);
	}

	struct END_TICK_locals
	{
		OI::Price::OracleQuery priceOracleQuery;
		OI::Mock::OracleQuery mockOracleQuery;
		sint64 oracleQueryId;
	};

	END_TICK_WITH_LOCALS()
	{
		// Query oracles
		if (qpi.tick() % 10 == 0)
		{
			// Setup query (in extra scope limit scope of using namespace Ch
			{
				using namespace Ch;
				locals.priceOracleQuery.oracle = OI::Price::getMockOracleId();
				locals.priceOracleQuery.currency1 = id(B, T, C, null, null);
				locals.priceOracleQuery.currency2 = id(U, S, D, null, null);
				locals.priceOracleQuery.timestamp = qpi.now();
			}

			locals.oracleQueryId = QUERY_ORACLE(OI::Price, locals.priceOracleQuery, NotifyPriceOracleReply, 20000);
			ASSERT(qpi.getOracleQueryStatus(locals.oracleQueryId) == ORACLE_QUERY_STATUS_PENDING);
		}
		if (qpi.tick() % 2 == 1)
		{
			locals.mockOracleQuery.value = qpi.tick();
			QUERY_ORACLE(OI::Mock, locals.mockOracleQuery, NotifyMockOracleReply, 8000);
		}
	}

	//---------------------------------------------------------------
	// COMMON PARTS

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(IncomingTransferAmounts, 20);
		REGISTER_USER_FUNCTION(GetIpoBid, 30);

		REGISTER_USER_PROCEDURE(GetTestExampleAShareManagementRights, 7);
		REGISTER_USER_PROCEDURE(QpiTransfer, 20);
		REGISTER_USER_PROCEDURE(QpiDistributeDividends, 21);
		REGISTER_USER_PROCEDURE(QpiBidInIpo, 30);

		REGISTER_USER_PROCEDURE(QueryPriceOracle, 100);

		REGISTER_USER_PROCEDURE_NOTIFICATION(NotifyPriceOracleReply);
		REGISTER_USER_PROCEDURE_NOTIFICATION(NotifyMockOracleReply);
	}
};
