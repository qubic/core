using namespace QPI;

// Oracle Interface (access to multiple oracles sharing the same interface, not instantiated, but available to contact, the same language feature restriction apply as for contracts)
struct Price
{
	static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;
	struct OracleQuery // size limited by tx size
	{
		id oracle; // a source for getting the information, e.g. coingecko -> string-like similar to asset-name, m256i (string of 32 bytes)
		DateAndTime timestamp;  // timestamp of response value, required for supporting subscription because it is set by the scheduler (if not provided compile error if subscription is tried)
		id currency1;  // Type how to reference currencies is unclear -> enum-like, string-like similar to asset-name, m256i
		id currency2;
	};
	struct OracleReply // size limited by tx size
	{
		sint64 numerator;    // at query.timestamp, currency1 = currency2 * numerator / denominator
		sint64 denominator;
	};
	static sint64 getRequestFee(const OracleQuery& query)
	{
		return 10; // fee may be dependent on the oracle etc.
	}
	static sint64 getSubscriptionFee(const OracleQuery& query, uint32 nofityCyclePeriodInMinutes)
	{
		// paid once per epoch, may depend on oracle / cycle period
		return 1000;
	}

	// additional functions that may be used by the contracts such as for validating query and reply, or for converting currencies
	static bool isResponseValid(const OracleReply& reply)
	{
		return reply.numerator > 0 && reply.denominator > 0;
	}

	// convenience functions, e.g. returning ID of available oracles and currencies
};
