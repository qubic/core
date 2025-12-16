#pragma once

#include "network_messages/transactions.h"
#include "contract_core/contract_def.h"

// Transaction input type for execution fee reporting
constexpr int EXECUTION_FEE_REPORT_INPUT_TYPE = 9;

// Variable-length transaction for reporting execution fees
// Layout: ExecutionFeeReportTransactionPrefix + contractIndices[numEntries] + [alignment padding] + executionFees[numEntries] + ExecutionFeeReportTransactionPostfix
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
        return sizeof(phaseNumber) + sizeof(numEntries);
    }

    static constexpr unsigned short maxInputSize()
    {
        // phaseNumber + numEntries + contractIndices[contractCount] + alignment + executionFees[contractCount] + dataLock
        unsigned int indicesSize = contractCount * sizeof(unsigned int);
        unsigned int alignmentPadding = (contractCount % 2 == 1) ? sizeof(unsigned int) : 0;
        unsigned int feesSize = contractCount * sizeof(unsigned long long);
        return static_cast<unsigned short>(sizeof(phaseNumber) + sizeof(numEntries) + indicesSize + alignmentPadding + feesSize + sizeof(m256i));
    }

    static bool isValidExecutionFeeReport(const Transaction* transaction)
    {
        return transaction->amount == minAmount()
            && transaction->inputSize >= minInputSize()
            && transaction->inputSize <= maxInputSize();
    }

    static unsigned int getNumEntries(const Transaction* transaction)
    {
        const auto* prefix = (const ExecutionFeeReportTransactionPrefix*)transaction;
        return prefix->numEntries;
    }

    static bool isValidEntryAlignment(const Transaction* transaction)
    {
        const auto* prefix = (const ExecutionFeeReportTransactionPrefix*)transaction;
        unsigned int numEntries = prefix->numEntries;

        // Calculate expected payload size
        unsigned int indicesSize = numEntries * sizeof(unsigned int);
        unsigned int alignmentPadding = (numEntries % 2 == 1) ? sizeof(unsigned int) : 0;
        unsigned int feesSize = numEntries * sizeof(unsigned long long);
        unsigned int expectedPayloadSize = indicesSize + alignmentPadding + feesSize;

        // Actual payload size (inputSize - phaseNumber - numEntries - dataLock)
        unsigned int actualPayloadSize = transaction->inputSize - sizeof(prefix->phaseNumber) - sizeof(prefix->numEntries) - sizeof(m256i);

        return actualPayloadSize == expectedPayloadSize;
    }

    static const unsigned int* getContractIndices(const Transaction* transaction)
    {
        const auto* prefix = (const ExecutionFeeReportTransactionPrefix*)transaction;
        return (const unsigned int*)(transaction->inputPtr() + sizeof(prefix->phaseNumber) + sizeof(prefix->numEntries));
    }

    static const unsigned long long* getExecutionFees(const Transaction* transaction)
    {
        const auto* prefix = (const ExecutionFeeReportTransactionPrefix*)transaction;
        unsigned int numEntries = prefix->numEntries;
        unsigned int indicesSize = numEntries * sizeof(unsigned int);
        unsigned int alignmentPadding = (numEntries % 2 == 1) ? sizeof(unsigned int) : 0;

        const unsigned char* afterPrefix = transaction->inputPtr() + sizeof(prefix->phaseNumber) + sizeof(prefix->numEntries);
        return (const unsigned long long*)(afterPrefix + indicesSize + alignmentPadding);
    }

    unsigned int phaseNumber;        // Phase this report is for (tick / NUMBER_OF_COMPUTORS)
    unsigned int numEntries;         // Number of contract entries in this report
    // Followed by:
    // - unsigned int contractIndices[numEntries]
    // - [0 or 4 bytes alignment padding for executionFees array]
    // - long long executionFees[numEntries]
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
    unsigned int contractIndices[contractCount];
    unsigned long long executionFees[contractCount];  // Compiler auto-aligns to 8 bytes
    ExecutionFeeReportTransactionPostfix postfix;
};

// Calculate expected size: Transaction(84) + phaseNumber(4) + numEntries(4) + contractIndices + alignment + executionFees + dataLock(32) + signature(64)
static_assert( sizeof(ExecutionFeeReportPayload) == sizeof(Transaction) + sizeof(unsigned int) + sizeof(unsigned int) + (contractCount * sizeof(unsigned int)) + ((contractCount % 2 == 1) ? sizeof(unsigned int) : 0) + (contractCount * sizeof(unsigned long long)) + sizeof(m256i) + SIGNATURE_SIZE, "ExecutionFeeReportPayload has wrong struct size");
static_assert( sizeof(ExecutionFeeReportPayload) <= sizeof(Transaction) + MAX_INPUT_SIZE + SIGNATURE_SIZE, "ExecutionFeeReportPayload is bigger than max transaction size. Currently max 82 SC are supported by the report");

// Builds the execution fee report payload from contract execution times
// Returns the number of entries added (0 if no contracts were executed)
static inline unsigned int buildExecutionFeeReportPayload(
    ExecutionFeeReportPayload& payload,
    const unsigned long long* contractExecutionTimes,
    const unsigned int phaseNumber,
    const unsigned long long multiplierNumerator,
    const unsigned long long multiplierDenominator
)
{
    if (multiplierDenominator == 0 || multiplierNumerator == 0)
    {
        return 0;
    }

    payload.transaction.phaseNumber = phaseNumber;

    // Build arrays with contract indices and execution fees
    unsigned int entryCount = 0;
    for (unsigned int contractIndex = 1; contractIndex < contractCount; contractIndex++)
    {
        unsigned long long executionTime = contractExecutionTimes[contractIndex];
        if (executionTime > 0)
        {
            unsigned long long executionFee = (executionTime * multiplierNumerator) / multiplierDenominator;
            if (executionFee > 0)
            {
                payload.contractIndices[entryCount] = contractIndex;
                payload.executionFees[entryCount] = executionFee;
                entryCount++;
            }
        }
    }
    payload.transaction.numEntries = entryCount;

    // Return if no contract was executed
    if (entryCount == 0)
    {
        return 0;
    }

    // Compact the executionFees to the correct position (right after contractIndices[entryCount] + alignment)
    unsigned int alignmentPadding = (entryCount % 2 == 1) ? sizeof(unsigned int) : 0;
    unsigned char* afterPrefix = ((unsigned char*)&payload) + sizeof(ExecutionFeeReportTransactionPrefix);
    unsigned char* compactFeesPosition = afterPrefix + (entryCount * sizeof(unsigned int)) + alignmentPadding;
    copyMem(compactFeesPosition, payload.executionFees, entryCount * sizeof(unsigned long long));

    // Calculate and set input size based on actual number of entries
    payload.transaction.inputSize = (unsigned short)(sizeof(payload.transaction.phaseNumber) + sizeof(payload.transaction.numEntries) + (entryCount * sizeof(unsigned int)) + alignmentPadding + (entryCount * sizeof(unsigned long long)) + sizeof(ExecutionFeeReportTransactionPostfix::dataLock));

    return entryCount;
}
