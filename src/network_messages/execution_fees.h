#pragma once

#include "network_messages/transactions.h"
#include "contract_core/contract_def.h"

// Transaction input type for execution fee reporting
constexpr int EXECUTION_FEE_REPORT_INPUT_TYPE = 9;

// Entry for a single contract's execution fee
struct ContractExecutionFeeEntry
{
    unsigned int contractIndex;      // Contract index
    unsigned int _padding;           // 4 bytes
    long long executionFee;          // executionTime * multiplier
};

static_assert(sizeof(ContractExecutionFeeEntry) == 4 + 4 + 8, "ContractExecutionFeeEntry size must be 16 bytes");

// Variable-length transaction for reporting execution fees
// Layout: ExecutionFeeReportTransactionPrefix + array of ContractExecutionFeeEntry + ExecutionFeeReportTransactionPostfix
struct ExecutionFeeReportTransactionPrefix : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return EXECUTION_FEE_REPORT_INPUT_TYPE;
    }

    static constexpr long long minAmount()
    {
        return 0;  // System transaction
    }

    static constexpr unsigned short minInputSize()
    {
        return sizeof(phaseNumber);
    }

    static constexpr unsigned short maxInputSize()
    {
        return sizeof(phaseNumber) + sizeof(_padding) + contractCount * sizeof(ContractExecutionFeeEntry) + sizeof(m256i);
    }

    static bool isValidExecutionFeeReport(const Transaction* transaction)
    {
        return transaction->amount == minAmount()
            && transaction->inputSize >= minInputSize()
            && transaction->inputSize <= maxInputSize();
    }

    static unsigned int getPayloadSize(const Transaction* transaction)
    {
        const auto* prefix = (const ExecutionFeeReportTransactionPrefix*)transaction;
        return transaction->inputSize - sizeof(prefix->phaseNumber) - sizeof(prefix->_padding) - sizeof(m256i);
    }

    static unsigned int getNumEntries(const Transaction* transaction)
    {
        return getPayloadSize(transaction) / sizeof(ContractExecutionFeeEntry);
    }

    static bool isValidEntryAlignment(const Transaction* transaction)
    {
        return (getPayloadSize(transaction) % sizeof(ContractExecutionFeeEntry)) == 0;
    }

    static const ContractExecutionFeeEntry* getEntries(const Transaction* transaction)
    {
        const auto* prefix = (const ExecutionFeeReportTransactionPrefix*)transaction;
        return (const ContractExecutionFeeEntry*)(transaction->inputPtr() + sizeof(prefix->phaseNumber) + sizeof(prefix->_padding));
    }

    unsigned int phaseNumber;        // Phase this report is for (tick / NUMBER_OF_COMPUTORS)
    unsigned int _padding;
    // Followed by variable number of ContractExecutionFeeEntry structures
};

struct ExecutionFeeReportTransactionPostfix
{
    m256i dataLock;
    unsigned char signature[SIGNATURE_SIZE];
};

// Payload structure for execution fee transaction
// Note: postfix is written at variable position based on actual entry count, not at fixed position
struct ExecutionFeeReportPayload
{
    ExecutionFeeReportTransactionPrefix transaction;
    ContractExecutionFeeEntry entries[contractCount];
    ExecutionFeeReportTransactionPostfix postfix;
};

static_assert( sizeof(ExecutionFeeReportPayload) == 84 + 4 /* padding */ + (contractCount * 16) + 32 +64, "ExecutionFeeReportPayload has wrong struct size");
