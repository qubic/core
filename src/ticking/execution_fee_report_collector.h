#pragma once
#include "platform/memory.h"
#include "platform/quorum_value.h"
#include "network_messages/execution_fees.h"
#include "contract_core/contract_def.h"
#include "contract_core/qpi_spectrum_impl.h"
#include "logging/logging.h"

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

    bool validateReportEntries(const unsigned int* contractIndices, const long long* executionFees, unsigned int numEntries)
    {
        for (unsigned int i = 0; i < numEntries; i++)
        {
            if (contractIndices[i] >= contractCount || executionFees[i] <= 0)
            {
                return false;
            }
        }
        return true;
    }

    void storeReportEntries(const unsigned int* contractIndices, const long long* executionFees, unsigned int numEntries, unsigned int computorIndex)
    {
        for (unsigned int i = 0; i < numEntries; i++)
        {
            storeReport(contractIndices[i], computorIndex, executionFees[i]);
        }
    }

    void processTransactionData(const Transaction* transaction, const m256i& dataLock)
    {
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
        const unsigned int* contractIndices = ExecutionFeeReportTransactionPrefix::getContractIndices(transaction);
        const long long* executionFees = ExecutionFeeReportTransactionPrefix::getExecutionFees(transaction);

        if (!validateReportEntries(contractIndices, executionFees, numEntries))
        {
            // Report contains invalid entries. E.g., Entry with negative fees or invalid contractIndex.
            return;
        }

        storeReportEntries(contractIndices, executionFees, numEntries, computorIndex);
    }

    void processReports()
    {
        // TODO: Add logging event for quorum value deductions
        for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
        {
            long long quorumValue = calculateAscendingQuorumValue(executionFeeReports[contractIndex], NUMBER_OF_COMPUTORS);

            if (quorumValue > 0)
            {
                addToContractFeeReserve(contractIndex, -quorumValue);
                ContractReserveDeduction message = {quorumValue, getContractFeeReserve(contractIndex), contractIndex};
                logger.logContractReserveDeduction(message);
            }
        }

        reset();
    }
};
