#pragma once

// Include this first, to ensure "logging/logging.h" isn't included before the custom LOG_BUFFER_SIZE has been defined
#include "logging_test.h"

#undef MAX_NUMBER_OF_TICKS_PER_EPOCH
#define MAX_NUMBER_OF_TICKS_PER_EPOCH 50
#undef TICKS_TO_KEEP_FROM_PRIOR_EPOCH
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 5
#include "ticking/tick_storage.h"

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
static void checkNetworkMessageOracleMachineQuery(QPI::uint64 expectedOracleQueryId, QPI::uint32 expectedTimeout, const typename OracleInterface::OracleQuery& expectedQuery)
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
    EXPECT_TRUE(compareMem(q, &expectedQuery, sizeof(OracleInterface::OracleQuery)) == 0);
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

static const Transaction* addOracleTransactionToTickStorage(const Transaction* tx, unsigned int txIndex)
{
    ASSERT(txIndex < NUMBER_OF_TRANSACTIONS_PER_TICK);
    Transaction* tsTx = nullptr;
    const unsigned int txSize = tx->totalSize();
    auto* offsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(tx->tick);
    if (ts.nextTickTransactionOffset + txSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
    {
        EXPECT_EQ(offsets[txIndex], 0);
        offsets[txIndex] = ts.nextTickTransactionOffset;
        tsTx = ts.tickTransactions(ts.nextTickTransactionOffset);
        copyMem(tsTx, tx, txSize);
        ts.nextTickTransactionOffset += txSize;
    }
    return tsTx;
}
