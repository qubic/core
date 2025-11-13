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

    void processReports()
    {
        // TODO: Implement processing logic
        // 1. For each contract, sort reports from all computors
        // 2. Calculate quorum value
        // 3. Deduct quorum value from executionFeeReserve
        // 4. Reset executionFeeReportCollector for next phase
    }
};
