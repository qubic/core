#pragma once

// Include this first, to ensure "logging/logging.h" isn't included before the custom LOG_BUFFER_SIZE has been defined
#include "logging_test.h"

#include "gtest/gtest.h"

#include "oracle_core/oracle_engine.h"
#include "contract_core/qpi_ticking_impl.h"
#include "contract_core/qpi_spectrum_impl.h"


union EnqueuedNetworkMessage
{
    RequestResponseHeader header;

    struct
    {
        RequestResponseHeader header;
        OracleMachineQuery queryMetadata;
        unsigned char queryData[MAX_ORACLE_QUERY_SIZE];
    } omQuery;
};

GLOBAL_VAR_DECL EnqueuedNetworkMessage enqueuedNetworkMessage;

template <typename OracleInterface>
static void checkNetworkMessageOracleMachineQuery(QPI::uint64 expectedOracleQueryId, QPI::id expectedOracle, QPI::uint32 expectedTimeout)
{
    EXPECT_EQ(enqueuedNetworkMessage.header.type(), OracleMachineQuery::type());
    EXPECT_GT(enqueuedNetworkMessage.header.size(), sizeof(RequestResponseHeader) + sizeof(OracleMachineQuery));
    QPI::uint32 queryDataSize = enqueuedNetworkMessage.header.size() - sizeof(RequestResponseHeader) - sizeof(OracleMachineQuery);
    EXPECT_LE(queryDataSize, (QPI::uint32)MAX_ORACLE_QUERY_SIZE);
    EXPECT_EQ(queryDataSize, sizeof(typename OracleInterface::OracleQuery));
    EXPECT_EQ(enqueuedNetworkMessage.omQuery.queryMetadata.oracleInterfaceIndex, OracleInterface::oracleInterfaceIndex);
    EXPECT_EQ(enqueuedNetworkMessage.omQuery.queryMetadata.oracleQueryId, expectedOracleQueryId);
    EXPECT_EQ(enqueuedNetworkMessage.omQuery.queryMetadata.timeoutInMilliseconds, expectedTimeout);
    const auto* q = (const OracleInterface::OracleQuery*)enqueuedNetworkMessage.omQuery.queryData;
    EXPECT_EQ(q->oracle, expectedOracle);
}

static void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data)
{
    EXPECT_EQ(peer, (Peer*)0x1);
    EXPECT_LE(dataSize, sizeof(OracleMachineQuery) + MAX_ORACLE_QUERY_SIZE);
    EXPECT_TRUE(enqueuedNetworkMessage.header.checkAndSetSize(sizeof(RequestResponseHeader) + dataSize));
    enqueuedNetworkMessage.header.setType(type);
    enqueuedNetworkMessage.header.setDejavu(dejavu);
    copyMem(&enqueuedNetworkMessage.omQuery.queryMetadata, data, dataSize);
}

static inline QPI::uint64 getContractOracleQueryId(QPI::uint32 tick, QPI::uint32 indexInTick)
{
    return ((QPI::uint64)tick << 31) | (indexInTick + NUMBER_OF_TRANSACTIONS_PER_TICK);
}
