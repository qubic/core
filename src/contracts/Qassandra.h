using namespace QPI;

// Inert scaffold for future Qassandra oracle-settled prediction markets.
// This header defines contract-safe identifiers, status/outcome values, and
// deterministic settlement comparison helpers without adding storage or runtime hooks.

constexpr uint64 QASSANDRA_QUBIC_USD_PRICE_SCALE = 100000000ULL;

enum QassandraMarketStatus : uint8
{
	QASSANDRA_MARKET_STATUS_DRAFT = 0,
	QASSANDRA_MARKET_STATUS_OPEN = 1,
	QASSANDRA_MARKET_STATUS_LOCKED = 2,
	QASSANDRA_MARKET_STATUS_SETTLED = 3,
	QASSANDRA_MARKET_STATUS_CANCELLED = 4,
};

enum QassandraOutcome : uint8
{
	QASSANDRA_OUTCOME_UNKNOWN = 0,
	QASSANDRA_OUTCOME_NO = 1,
	QASSANDRA_OUTCOME_YES = 2,
};

enum QassandraComparisonDirection : uint8
{
	QASSANDRA_COMPARE_GT = 0,
	QASSANDRA_COMPARE_GTE = 1,
	QASSANDRA_COMPARE_LT = 2,
	QASSANDRA_COMPARE_LTE = 3,
};

struct QassandraMarketId
{
	id eventId;
	uint64 nonce;
};

struct QassandraOracleSettlement
{
	sint64 thresholdValue;
	uint8 comparisonDirection;
};

// Agent-readable market metadata and deterministic settlement surfaces are planned
// for later Qassandra layers; no wallet, API, or off-chain orchestration is added here.
struct QassandraMarketDescriptor
{
	QassandraMarketId marketId;
	id baseAsset;
	id quoteAsset;
	QassandraOracleSettlement settlement;
};

inline static bool qassandraIsValidPriceReply(const OI::Price::OracleReply& reply)
{
	return reply.numerator > 0 && reply.denominator > 0;
}

inline static bool qassandraCompareScaledPrice(const OI::Price::OracleReply& reply, sint64 thresholdValue, uint8 comparisonDirection)
{
	if (!qassandraIsValidPriceReply(reply) || thresholdValue < 0)
	{
		return false;
	}

	const uint128 left = uint128(reply.numerator) * uint128(QASSANDRA_QUBIC_USD_PRICE_SCALE);
	const uint128 right = uint128(thresholdValue) * uint128(reply.denominator);

	if (comparisonDirection == QASSANDRA_COMPARE_GT)
	{
		return left > right;
	}
	if (comparisonDirection == QASSANDRA_COMPARE_GTE)
	{
		return left >= right;
	}
	if (comparisonDirection == QASSANDRA_COMPARE_LT)
	{
		return left < right;
	}
	if (comparisonDirection == QASSANDRA_COMPARE_LTE)
	{
		return left <= right;
	}
	return false;
}
