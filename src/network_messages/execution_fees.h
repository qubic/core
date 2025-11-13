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

static_assert(sizeof(ContractExecutionFeeEntry) == 4 + 4 + 8, "ContractExecutionFeeEntry size must be 12 bytes");

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
        return sizeof(unsigned int);  // At least phase number
    }

    unsigned int phaseNumber;        // Phase this report is for (tick / NUMBER_OF_COMPUTORS)
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
    ExecutionFeeReportTransactionPostfix postfix;  // Reserve space for signature
};
