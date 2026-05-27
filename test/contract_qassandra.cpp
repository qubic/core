#define NO_UEFI

#include "contract_testing.h"

#include <cstring>

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

    EXPECT_GT(sizeof(QASSANDRA::QdraMarketMetadata), 0);
    EXPECT_EQ(sizeof(QASSANDRA::StateData), sizeof(QUOTTERY::StateData) + sizeof(MetadataMap));
    EXPECT_EQ(contractDescriptions[QASSANDRA_CONTRACT_INDEX].stateSize, sizeof(QASSANDRA::StateData));
    EXPECT_EQ(contractDescriptions[QUOTTERY_CONTRACT_INDEX].stateSize, sizeof(QUOTTERY::StateData));
}
