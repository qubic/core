#pragma once
#include "platform/memory.h"
#include "network_messages/execution_fees.h"
#include "contract_core/contract_def.h"

class ExecutionFeeReportCollector
{
private:
    long long executionFeeReports[contractCount][NUMBER_OF_COMPUTORS];

public:
    void init()
    {
        setMem(executionFeeReports, sizeof(executionFeeReports), 0);
    }

    void reset()
    {
        setMem(executionFeeReports, sizeof(executionFeeReports), 0);
    }

    // Store execution fee report from a computor for a contract
    void storeReport(unsigned int contractIndex, unsigned int computorIndex, long long executionFee)
    {
        if (contractIndex < contractCount && computorIndex < NUMBER_OF_COMPUTORS)
        {
            executionFeeReports[contractIndex][computorIndex] = executionFee;
        }
    }

    long long* getReportsForContract(unsigned int contractIndex)
    {
        if (contractIndex < contractCount)
        {
            return executionFeeReports[contractIndex];
        }
        return nullptr;
    }

    bool validateReportEntries(const ContractExecutionFeeEntry* entries, unsigned int numEntries)
    {
        for (unsigned int i = 0; i < numEntries; i++)
        {
            if (entries[i].contractIndex >= contractCount || entries[i].executionFee <= 0)
            {
                return false;
            }
        }
        return true;
    }

    void storeReportEntries(const ContractExecutionFeeEntry* entries, unsigned int numEntries, unsigned int computorIndex)
    {
        for (unsigned int i = 0; i < numEntries; i++)
        {
            storeReport(entries[i].contractIndex, computorIndex, entries[i].executionFee);
        }
    }

    void processTransactionData(const Transaction* transaction, const m256i& dataLock)
    {
#ifndef NO_UEFI
        int computorIndex = transaction->tick % NUMBER_OF_COMPUTORS;
        if (transaction->sourcePublicKey != broadcastedComputors.computors.publicKeys[computorIndex])
        {
            // Report was sent by wrong src
            return;
        }

        if (!ExecutionFeeReportTransactionPrefix::isValidExecutionFeeReport(transaction))
        {
            // Report amount or size
            return;
        }

        const unsigned char* dataLockPtr = transaction->inputPtr() + transaction->inputSize - sizeof(m256i);
        m256i txDataLock = *((m256i*)dataLockPtr);

        if (txDataLock != dataLock)
        {
#ifndef NDEBUG
            CHAR16 dbg[256];
            setText(dbg, L"TRACE: [Execution fee report tx] Wrong datalock from comp ");
            appendNumber(dbg, computorIndex, false);
            addDebugMessage(dbg);
#endif
            return;
        }

        if (!ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(transaction))
        {
            // Entries array has incomplete entries.
            return;
        }

        const unsigned int numEntries = ExecutionFeeReportTransactionPrefix::getNumEntries(transaction);
        const auto* entries = ExecutionFeeReportTransactionPrefix::getEntries(transaction);

        if (!validateReportEntries(entries, numEntries))
        {
            // Report contains invalid entries. E.g., Entry with negative fees or invalid contractIndex.
            return;
        }

        storeReportEntries(entries, numEntries, computorIndex);
#endif
    }

    void processReports()
    {
        // TODO: Implement processing logic
        // 1. For each contract, sort reports from all computors
        // 2. Calculate quorum value
        // 3. Deduct quorum value from executionFeeReserve
        // 4. Reset executionFeeReportCollector for next phase
    }
};
