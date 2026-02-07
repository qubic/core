using namespace QPI;

/**
* Oracle interface "Price" (also providing general documentation about oracle interfaces).
*
* As all oracle interfaces, it provides access to multiple oracles sharing the same interface, that is,
* the same input and output structs (OracleQuery and OracleReply).
*
* It also defines the oracle query and subscription fees through the member functions getQueryFee() and
* getSubscriptionFee(). The subscription fee needs to be paid for each call to qpi.subscribeOracle(),
* which is usually once per epoch. The query fee needs to be paid for each call to qpi.queryOracle()
* and as the amount of each user oracle query transaction.
*
* Each oracle interface is internally identified through the oracleInterfaceIndex.
*
* The oracle interface struct is never instantiated, but its types and static members are available to
* contracts. For the code in the interface, the same language feature restriction apply as for contracts.
*
* The OracleQuery struct must be designed in a way that consecutive queries with the same OracleQuery
* member values always lead the exact same OracleReply. The rationale is that the oracle is queried
* individually be the computors and a quorum needs to agree on the exact same reply in order to confirm
* its correctness.
*
* In order to satisfy this requirement, most oracle query structs will require:
* - an oracle identifier, exactly specifying the source/provider, where to get the information,
* - a timestamp about the exact time in case of information that varies with time, such as prices,
* - additional specification which data is queried, such as a pair of currencies for prices or a
*   location for weather
*
* The size of both oracle query and reply are restricted by the transaction size. The limits are
* available through the constants MAX_ORACLE_QUERY_SIZE and MAX_ORACLE_REPLY_SIZE.
*/
struct Price
{
	//-------------------------------------------------------------------------
	// Mandatory oracle interface definitions

	/// Oracle interface index
	static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

	/// Oracle query data / input to the oracle machine
	struct OracleQuery
	{
		/// Oracle = the source for getting the information, e.g. coingecko
		id oracle;

		/// The timestamp that the reply value should be of. This is required for supporting supporting subscription, because it is set by the scheduler.
		DateAndTime timestamp;

		/// First currency of pair to get the exchange rate for.
		id currency1;

		/// Second currency of pair to get the exchange rate for.
		id currency2;

		// TODO: we may need to add precision requirements regarding response value
	};

	/// Oracle reply data / output of the oracle machine
	struct OracleReply
	{
		// Numerator of exchange rate: at query.timestamp, currency1 = currency2 * numerator / denominator
		sint64 numerator;

		// Denominator of exchange rate: at query.timestamp, currency1 = currency2 * numerator / denominator
		sint64 denominator;
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

	//-------------------------------------------------------------------------
	// Optional: convenience features for contracts using the oracle interface
	//
	// Examples:
	// - functions for validating query and reply
	// - functions returning ID of available oracles and currencies for building query
	// - function for converting currencies
	// - constants supporting with building query

	/// Check if the passed oracle reply is valid
	static bool replyIsValid(const OracleReply& reply)
	{
		return reply.numerator > 0 && reply.denominator > 0;
	}

	// TODO:
	// implement and test currency conversion (including using uint128 on the way in order to support large quantities)


	/// Get oracle ID of mock oracle
	static id getMockOracleId()
	{
		using namespace Ch;
		return id(m, o, c, k, null);
	}

	/// Get oracle ID of binance oracle
	static id getBinanceOracleId()
	{
		using namespace Ch;
		return id(b, i, n, a, n, c, e);
	}

	/// Get oracle ID of mexc oracle
	static id getMexcOracleId()
	{
		using namespace Ch;
		return id(m, e, x, c, 0);
	}

	/// Get oracle ID of gate.io oracle
	static id getGateOracleId()
	{
		using namespace Ch;
		return id(g, a, t, e, 0);
	}

	/// Get oracle ID of coingecko oracle
	static id getCoingeckoOracleId()
	{
		using namespace Ch;
		return id(c, o, i, n, g, e, c, k, o);
	}

	/// Get oracle ID of combined binance + mexc oracle (mean of prices of both sources)
	static id getBinanceMexcOracleId()
	{
		using namespace Ch;
		return id(b, i, n, a, n, c, e, underscore, m, e, x, c);
	}

	/// Get oracle ID of combined binance + gate oracle (mean of prices of both sources)
	static id getBinanceGateOracleId()
	{
		using namespace Ch;
		return id(b, i, n, a, n, c, e, underscore, g, a, t, e);
	}

	/// Get oracle ID of combined gate + mexc oracle (mean of prices of both sources)
	static id getGateMexcOracleId()
	{
		using namespace Ch;
		return id(g, a, t, e, underscore, m, e, x, c);
	}
};
