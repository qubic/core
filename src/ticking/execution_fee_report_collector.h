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
    unsigned long long executionFeeReports[contractCount][NUMBER_OF_COMPUTORS];

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
    void storeReport(unsigned int contractIndex, unsigned int computorIndex, unsigned long long executionFee)
    {
        if (contractIndex > 0 && contractIndex < contractCount && computorIndex < NUMBER_OF_COMPUTORS)
        {
            executionFeeReports[contractIndex][computorIndex] = executionFee;
        }
    }

    const unsigned long long* getReportsForContract(unsigned int contractIndex)
    {
        if (contractIndex < contractCount)
        {
            return executionFeeReports[contractIndex];
        }
        return nullptr;
    }

    bool validateReportEntries(const unsigned int* contractIndices, const unsigned long long* executionFees, unsigned int numEntries)
    {
        for (unsigned int i = 0; i < numEntries; i++)
        {
            if (contractIndices[i] == 0 || contractIndices[i] >= contractCount || executionFees[i] == 0)
            {
                return false;
            }
        }
        return true;
    }

    void storeReportEntries(const unsigned int* contractIndices, const unsigned long long* executionFees, unsigned int numEntries, unsigned int computorIndex)
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
        const unsigned long long* executionFees = ExecutionFeeReportTransactionPrefix::getExecutionFees(transaction);

        if (!validateReportEntries(contractIndices, executionFees, numEntries))
        {
            // Report contains invalid entries. E.g., Entry with negative fees or invalid contractIndex.
            return;
        }

        storeReportEntries(contractIndices, executionFees, numEntries, computorIndex);
    }

    void processReports()
    {
        for (unsigned int contractIndex = 1; contractIndex < contractCount; contractIndex++)
        {
#if !defined(NDEBUG) && !defined(NO_UEFI)
            unsigned int numNonZero = 0;
            int firstNonZero = -1;
            int lastNonZero = -1;
            for (unsigned int compIndex = 0; compIndex < NUMBER_OF_COMPUTORS; ++compIndex)
            {
                if (executionFeeReports[contractIndex][compIndex] > 0)
                {
                    if (firstNonZero == -1)
                        firstNonZero = compIndex;
                    lastNonZero = compIndex;
                    numNonZero++;
                }
            }

            CHAR16 dbgMsgBuf[128];
            setText(dbgMsgBuf, L"Contract ");
            appendNumber(dbgMsgBuf, contractIndex, FALSE);
            appendText(dbgMsgBuf, L": ");
            appendNumber(dbgMsgBuf, numNonZero, FALSE);
            appendText(dbgMsgBuf, L" non-zero fee reports, first non-zero comp ");
            appendNumber(dbgMsgBuf, firstNonZero, FALSE);
            appendText(dbgMsgBuf, L", last non-zero comp ");
            appendNumber(dbgMsgBuf, lastNonZero, FALSE);
            appendText(dbgMsgBuf, L" (0-indexed)");
            addDebugMessage(dbgMsgBuf);
#endif

            unsigned long long quorumValue = calculateAscendingQuorumValue(executionFeeReports[contractIndex], NUMBER_OF_COMPUTORS);

            if (quorumValue > 0)
            {
                // TODO: enable subtraction after mainnet testing phase
                // subtractFromContractFeeReserve(contractIndex, quorumValue);
                ContractReserveDeduction message = { quorumValue, getContractFeeReserve(contractIndex), contractIndex };
                logger.logContractReserveDeduction(message);
            }
        }

        reset();
    }

    bool saveToFile(const CHAR16* fileName, const CHAR16* directory = NULL)
    {
        long long savedSize = save(fileName, sizeof(ExecutionFeeReportCollector), (unsigned char*)this, directory);
        if (savedSize == sizeof(ExecutionFeeReportCollector))
            return true;
        else
            return false;
    }

    bool loadFromFile(const CHAR16* fileName, const CHAR16* directory = NULL)
    {
        long long loadedSize = load(fileName, sizeof(ExecutionFeeReportCollector), (unsigned char*)this, directory);
        if (loadedSize == sizeof(ExecutionFeeReportCollector))
            return true;
        else
            return false;
    }
};
