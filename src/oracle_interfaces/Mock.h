using namespace QPI;

/**
* Oracle interface "Mock" (see Price.h for general documentation about oracle interfaces).
*
* This is for useful testing the oracle machine and the core logic without involving external services.
*/
struct Mock
{
	//-------------------------------------------------------------------------
	// Mandatory oracle interface definitions

	/// Oracle interface index
	static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

	/// Oracle query data / input to the oracle machine
	struct OracleQuery
	{
		/// Value that processed
		uint64 value;
	};

	/// Oracle reply data / output of the oracle machine
	struct OracleReply
	{
		/// Value given in query
		uint64 echoedValue;

		// 2 * value given in query
		uint64 doubledValue;
	};

	/// Return query fee, which may depend on the specific query (for example on the oracle).
	static sint64 getQueryFee(const OracleQuery& query)
	{
		return 10;
	}

	/// Return subscription fee, which may depend on query and interval.
	static sint64 getSubscriptionFee(const OracleQuery& query, uint16 notifyIntervalInMinutes)
	{
		return 1000;
	}

	/// Check if the passed oracle reply is valid
	static bool replyIsValid(const OracleQuery& query, const OracleReply& reply)
	{
		return (reply.echoedValue == query.value) && (reply.doubledValue == 2 * query.value);
	}
};
