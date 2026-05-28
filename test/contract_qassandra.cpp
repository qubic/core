#define NO_UEFI

#include "contract_testing.h"

#include <cstring>

struct QassandraMetadataTestAccess : public QASSANDRA
{
    using QASSANDRA::mapQubicUsdThresholdOutcome;
    using QASSANDRA::isQubicUsdOracleReplyValid;
    using QASSANDRA::isMarketMetadataValid;
    using QASSANDRA::qubicCurrencyId;
    using QASSANDRA::usdCurrencyId;
    using QASSANDRA::usdtCurrencyId;
};

TEST(QassandraScaffold, AssetConstants)
{
    EXPECT_EQ(QASSANDRA_CONTRACT_ASSET_NAME, 1095910481ULL);
    EXPECT_EQ(QASSANDRA_GOV_ASSET_NAME, 24294015454299217ULL);
}

TEST(QassandraScaffold, ContractDescriptionIsIndependentFromQuottery)
{
    ASSERT_NE(QASSANDRA_CONTRACT_INDEX, QUOTTERY_CONTRACT_INDEX);

    EXPECT_STREQ(contractDescriptions[QASSANDRA_CONTRACT_INDEX].assetName, "QDRA");
    EXPECT_STREQ(contractDescriptions[QUOTTERY_CONTRACT_INDEX].assetName, "QTRY");

    EXPECT_EQ(contractDescriptions[QASSANDRA_CONTRACT_INDEX].stateSize, sizeof(QASSANDRA::StateData));
    EXPECT_EQ(contractDescriptions[QUOTTERY_CONTRACT_INDEX].stateSize, sizeof(QUOTTERY::StateData));
}

TEST_F(ContractTesting, QassandraRegistersProceduresIndependently)
{
    EXPECT_NE(contractUserFunctions[QASSANDRA_CONTRACT_INDEX][1], nullptr);
    EXPECT_NE(contractUserProcedures[QASSANDRA_CONTRACT_INDEX][1], nullptr);

    EXPECT_NE(contractUserFunctions[QUOTTERY_CONTRACT_INDEX][1], nullptr);
    EXPECT_NE(contractUserProcedures[QUOTTERY_CONTRACT_INDEX][1], nullptr);
}

TEST(QassandraMarketMetadataScaffold, MarketTypeConstants)
{
    EXPECT_EQ(QASSANDRA_MARKET_TYPE_GENERIC, 0);
    EXPECT_EQ(QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD, 1);
    EXPECT_EQ(QASSANDRA_MARKET_TYPE_ECOSYSTEM_MILESTONE, 2);
}

TEST(QassandraMarketMetadataScaffold, ComparisonConstants)
{
    EXPECT_EQ(QASSANDRA_COMPARISON_UNSPECIFIED, 0);
    EXPECT_EQ(QASSANDRA_COMPARISON_GTE, 1);
    EXPECT_EQ(QASSANDRA_COMPARISON_LTE, 2);
}

TEST(QassandraMarketMetadataScaffold, DefaultMetadataPreservesGenericMarketType)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));

    EXPECT_EQ(metadata.marketType, QASSANDRA_MARKET_TYPE_GENERIC);
    EXPECT_EQ(metadata.comparison, QASSANDRA_COMPARISON_UNSPECIFIED);
    EXPECT_EQ(metadata.thresholdValue, 0);
    EXPECT_EQ(metadata.targetDate, 0);
}

TEST(QassandraMarketMetadataScaffold, MetadataStorageIsQassandraOnly)
{
    using MetadataMap = QPI::HashMap<uint64, QASSANDRA::QdraMarketMetadata, QASSANDRA_MAX_CONCURRENT_EVENT>;
    using SettlementMap = QPI::HashMap<uint64, QASSANDRA::QdraOracleSettlement, QASSANDRA_MAX_CONCURRENT_EVENT>;
    using QueryToEventMap = QPI::HashMap<sint64, uint64, QASSANDRA_MAX_CONCURRENT_EVENT>;

    EXPECT_GT(sizeof(QASSANDRA::QdraMarketMetadata), 0);
    EXPECT_GT(sizeof(QASSANDRA::QdraOracleSettlement), 0);
    EXPECT_EQ(sizeof(QASSANDRA::StateData), sizeof(QUOTTERY::StateData) + sizeof(MetadataMap) + sizeof(SettlementMap) + sizeof(QueryToEventMap));
    EXPECT_EQ(contractDescriptions[QASSANDRA_CONTRACT_INDEX].stateSize, sizeof(QASSANDRA::StateData));
    EXPECT_EQ(contractDescriptions[QUOTTERY_CONTRACT_INDEX].stateSize, sizeof(QUOTTERY::StateData));
}

TEST(QassandraOracleSettlementScaffold, CurrencyIds)
{
    EXPECT_EQ(QassandraMetadataTestAccess::qubicCurrencyId(), id(Ch::Q, Ch::U, Ch::B, Ch::I, Ch::C));
    EXPECT_EQ(QassandraMetadataTestAccess::usdCurrencyId(), id(Ch::U, Ch::S, Ch::D, Ch::null, Ch::null));
    EXPECT_EQ(QassandraMetadataTestAccess::usdtCurrencyId(), id(Ch::U, Ch::S, Ch::D, Ch::T, Ch::null));
}

TEST(QassandraOracleSettlementScaffold, PriceScaleAndStatusConstants)
{
    EXPECT_EQ(QASSANDRA_QUBIC_USD_PRICE_SCALE, 1000000000ULL);
    EXPECT_EQ(QASSANDRA_ORACLE_SETTLEMENT_NONE, 0);
    EXPECT_EQ(QASSANDRA_ORACLE_SETTLEMENT_PENDING, 1);
    EXPECT_EQ(QASSANDRA_ORACLE_SETTLEMENT_SUCCESS, 2);
    EXPECT_EQ(QASSANDRA_ORACLE_SETTLEMENT_TIMEOUT, 3);
    EXPECT_EQ(QASSANDRA_ORACLE_SETTLEMENT_UNRESOLVABLE, 4);
    EXPECT_EQ(QASSANDRA_ORACLE_SETTLEMENT_INVALID_REPLY, 5);
}

TEST(QassandraOracleSettlementScaffold, PriceReplyValidation)
{
    EXPECT_TRUE(QassandraMetadataTestAccess::isQubicUsdOracleReplyValid({ 1, 1 }));
    EXPECT_FALSE(QassandraMetadataTestAccess::isQubicUsdOracleReplyValid({ 0, 1 }));
    EXPECT_FALSE(QassandraMetadataTestAccess::isQubicUsdOracleReplyValid({ -1, 1 }));
    EXPECT_FALSE(QassandraMetadataTestAccess::isQubicUsdOracleReplyValid({ 1, 0 }));
    EXPECT_FALSE(QassandraMetadataTestAccess::isQubicUsdOracleReplyValid({ 1, -1 }));
}

TEST_F(ContractTesting, CreateTypedForecastMarketRegistersAtNextFreeSlot)
{
    EXPECT_NE(contractUserProcedures[QASSANDRA_CONTRACT_INDEX][16], nullptr);
    EXPECT_NE(contractUserProcedures[QASSANDRA_CONTRACT_INDEX][15], nullptr);
    EXPECT_NE(contractUserProcedures[QASSANDRA_CONTRACT_INDEX][20], nullptr);
    EXPECT_NE(contractUserProcedures[QASSANDRA_CONTRACT_INDEX][1], nullptr);
}

TEST(QassandraTypedMarketScaffold, CreateEventAbiStaysGeneric)
{
    EXPECT_EQ(sizeof(QASSANDRA::CreateEvent_input), sizeof(QASSANDRA::QdraEventInfo));
    EXPECT_GT(sizeof(QASSANDRA::CreateTypedForecastMarket_input), sizeof(QASSANDRA::CreateEvent_input));
    EXPECT_EQ(sizeof(QASSANDRA::CreateTypedForecastMarket_input),
        sizeof(QASSANDRA::QdraEventInfo) + sizeof(QASSANDRA::QdraMarketMetadata));
}

TEST(QassandraTypedMarketScaffold, ValidatesQubicUsdThresholdMetadata)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));

    metadata.marketType = QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD;
    metadata.comparison = QASSANDRA_COMPARISON_GTE;
    metadata.thresholdValue = 1000000;
    metadata.targetDate = 20270101;

    EXPECT_TRUE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.comparison = QASSANDRA_COMPARISON_UNSPECIFIED;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.comparison = QASSANDRA_COMPARISON_LTE;
    metadata.thresholdValue = 0;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.thresholdValue = 1000000;
    metadata.targetDate = 0;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));
}

TEST(QassandraTypedMarketScaffold, ValidatesEcosystemMilestoneMetadata)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));

    metadata.marketType = QASSANDRA_MARKET_TYPE_ECOSYSTEM_MILESTONE;
    metadata.comparison = QASSANDRA_COMPARISON_UNSPECIFIED;
    metadata.thresholdValue = 0;
    metadata.targetDate = 20270101;

    EXPECT_TRUE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.comparison = QASSANDRA_COMPARISON_GTE;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.comparison = QASSANDRA_COMPARISON_UNSPECIFIED;
    metadata.thresholdValue = 1;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));
}

TEST(QassandraTypedMarketScaffold, RejectsGenericUnknownAndReservedMetadata)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));

    metadata.marketType = QASSANDRA_MARKET_TYPE_GENERIC;
    metadata.targetDate = 20270101;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.marketType = 255;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));

    metadata.marketType = QASSANDRA_MARKET_TYPE_ECOSYSTEM_MILESTONE;
    metadata.reserved0 = 1;
    EXPECT_FALSE(QassandraMetadataTestAccess::isMarketMetadataValid(metadata));
}

TEST(QassandraOracleSettlementScaffold, MapsGteThresholdOutcomes)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));
    metadata.marketType = QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD;
    metadata.comparison = QASSANDRA_COMPARISON_GTE;
    metadata.thresholdValue = 5 * QASSANDRA_QUBIC_USD_PRICE_SCALE;

    sint8 outcome = QASSANDRA_RESULT_NOT_SET;
    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 6, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_YES);

    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 5, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_YES);

    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 4, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NO);
}

TEST(QassandraOracleSettlementScaffold, MapsLteThresholdOutcomes)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));
    metadata.marketType = QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD;
    metadata.comparison = QASSANDRA_COMPARISON_LTE;
    metadata.thresholdValue = 5 * QASSANDRA_QUBIC_USD_PRICE_SCALE;

    sint8 outcome = QASSANDRA_RESULT_NOT_SET;
    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 4, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_YES);

    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 5, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_YES);

    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 6, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NO);
}

TEST(QassandraOracleSettlementScaffold, RejectsInvalidThresholdMappingInputs)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));
    metadata.marketType = QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD;
    metadata.comparison = QASSANDRA_COMPARISON_GTE;
    metadata.thresholdValue = QASSANDRA_QUBIC_USD_PRICE_SCALE;

    sint8 outcome = QASSANDRA_RESULT_YES;
    EXPECT_FALSE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 0, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NOT_SET);

    EXPECT_FALSE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 1, 0 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NOT_SET);

    metadata.thresholdValue = 0;
    EXPECT_FALSE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 1, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NOT_SET);

    metadata.thresholdValue = QASSANDRA_QUBIC_USD_PRICE_SCALE;
    metadata.comparison = QASSANDRA_COMPARISON_UNSPECIFIED;
    EXPECT_FALSE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 1, 1 }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NOT_SET);
}

TEST(QassandraOracleSettlementScaffold, UsesUint128ForLargeThresholdComparison)
{
    QASSANDRA::QdraMarketMetadata metadata;
    std::memset(&metadata, 0, sizeof(metadata));
    metadata.marketType = QASSANDRA_MARKET_TYPE_QUBIC_USD_THRESHOLD;
    metadata.comparison = QASSANDRA_COMPARISON_GTE;
    metadata.thresholdValue = 89999999999LL;

    sint8 outcome = QASSANDRA_RESULT_NOT_SET;
    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 90000000000LL, 1000000000LL }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_YES);

    metadata.thresholdValue = 90000000001LL;
    EXPECT_TRUE(QassandraMetadataTestAccess::mapQubicUsdThresholdOutcome({ 90000000000LL, 1000000000LL }, metadata, outcome));
    EXPECT_EQ(outcome, QASSANDRA_RESULT_NO);
}
