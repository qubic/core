#define NO_UEFI
#define DEFINE_VARIABLES_SHARED_BETWEEN_COMPILE_UNITS

#include "contract_testing.h"
#include "logging_test.h"
#include "platform/concurrency_impl.h"
#include "platform/profiling.h"

// Implement non-QPI version of notification trigger function that is defined in qubic.cpp, where it hands over the
// notification to the contract processor for running the incomming transfer callback.
// We don't have separate tick and contract processors in testing, so we run the callback directly.
void notifyContractOfIncomingTransfer(const m256i& source, const m256i& dest, long long amount, unsigned char type)
{
    // Only notify if amount > 0 and dest is contract
    if (amount <= 0 || dest.u64._0 >= contractCount || dest.u64._1 || dest.u64._2 || dest.u64._3)
        return;

    unsigned int contractIndex = (unsigned int)dest.m256i_u64[0];
    ASSERT(system.epoch >= contractDescriptions[contractIndex].constructionEpoch);
    ASSERT(system.epoch < contractDescriptions[contractIndex].destructionEpoch);

    // Run callback system procedure POST_INCOMING_TRANSFER
    QpiContextSystemProcedureCall qpiContext(contractIndex, POST_INCOMING_TRANSFER);
    QPI::PostIncomingTransfer_input input{ source, amount, type };
    qpiContext.call(input);
}
