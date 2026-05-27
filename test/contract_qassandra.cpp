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
