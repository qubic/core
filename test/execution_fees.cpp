#define NO_UEFI

#include "contract_testing.h"
#include "../src/ticking/execution_fee_report_collector.h"

// Helper to create a valid baseline test transaction with given entries
static Transaction* createTestTransaction(unsigned char* buffer, size_t bufferSize,
                                          unsigned int numEntries,
                                          const ContractExecutionFeeEntry* entries)
{
    const unsigned int inputSize = sizeof(unsigned int) + sizeof(unsigned int) +
                                    (numEntries * sizeof(ContractExecutionFeeEntry)) +
                                    sizeof(m256i);

    if (sizeof(Transaction) + inputSize > bufferSize)
    {
        return nullptr;
    }

    Transaction* tx = (Transaction*)buffer;
    tx->sourcePublicKey = m256i::zero();
    tx->destinationPublicKey = m256i::zero();
    tx->amount = 0;
    tx->tick = 1000;
    tx->inputType = EXECUTION_FEE_REPORT_INPUT_TYPE;
    tx->inputSize = inputSize;

    unsigned char* inputPtr = tx->inputPtr();
    *(unsigned int*)inputPtr = 5; // phaseNumber
    *(unsigned int*)(inputPtr + 4) = 0; // padding

    ContractExecutionFeeEntry* txEntries = (ContractExecutionFeeEntry*)(inputPtr + 8);
    for (unsigned int i = 0; i < numEntries; i++)
    {
        txEntries[i] = entries[i];
    }

    m256i* dataLock = (m256i*)(inputPtr + 8 + (numEntries * sizeof(ContractExecutionFeeEntry)));
    *dataLock = m256i::zero();

    return tx;
}

TEST(ExecutionFeeReportCollector, InitAndStore) {
    ExecutionFeeReportCollector collector;
    collector.init();

    collector.storeReport(0, 0, 1000);
    collector.storeReport(0, 1, 2000);
    collector.storeReport(1, 0, 5000);
    collector.storeReport(1, 500, 6000);

    long long* reports = collector.getReportsForContract(0);
    ASSERT_NE(reports, nullptr);
    EXPECT_EQ(reports[0], 1000);
    EXPECT_EQ(reports[1], 2000);

    reports = collector.getReportsForContract(1);
    ASSERT_NE(reports, nullptr);
    EXPECT_EQ(reports[0], 5000);
    EXPECT_EQ(reports[500], 6000);
}

TEST(ExecutionFeeReportCollector, Reset) {
    ExecutionFeeReportCollector collector;
    collector.init();

    for (unsigned int i = 0; i < 10; i++) {
        collector.storeReport(0, i, i * 1000);
    }

    long long* reports = collector.getReportsForContract(0);
    EXPECT_EQ(reports[5], 5000);

    collector.reset();

    reports = collector.getReportsForContract(0);
    EXPECT_EQ(reports[5], 0);
}

TEST(ExecutionFeeReportCollector, BoundaryValidation) {
    ExecutionFeeReportCollector collector;
    collector.init();

    collector.storeReport(0, 0, 100);
    collector.storeReport(contractCount - 1, NUMBER_OF_COMPUTORS - 1, 200);

    collector.storeReport(contractCount, 0, 100);
    collector.storeReport(0, NUMBER_OF_COMPUTORS, 100);

    long long* reports = collector.getReportsForContract(0);
    EXPECT_EQ(reports[0], 100);

    reports = collector.getReportsForContract(contractCount - 1);
    EXPECT_EQ(reports[NUMBER_OF_COMPUTORS - 1], 200);
}

TEST(ExecutionFeeReportTransaction, ParseValidTransaction) {
    unsigned char buffer[512];
    ContractExecutionFeeEntry entries[2] = {
        {0, 0, 1000},
        {1, 0, 2000}
    };

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 2, entries);
    ASSERT_NE(tx, nullptr);

    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidExecutionFeeReport(tx));
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 2u);

    const ContractExecutionFeeEntry* parsedEntries = ExecutionFeeReportTransactionPrefix::getEntries(tx);
    EXPECT_EQ(parsedEntries[0].contractIndex, 0u);
    EXPECT_EQ(parsedEntries[0].executionFee, 1000);
    EXPECT_EQ(parsedEntries[1].contractIndex, 1u);
    EXPECT_EQ(parsedEntries[1].executionFee, 2000);
}

TEST(ExecutionFeeReportTransaction, RejectNonZeroAmount) {
    unsigned char buffer[512];
    ContractExecutionFeeEntry entries[1] = {
        {0, 0, 1000}
    };

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 1, entries);
    ASSERT_NE(tx, nullptr);

    // Valid initially
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidExecutionFeeReport(tx));

    // Make amount non-zero (execution fee reports must have amount = 0)
    tx->amount = 100;

    // Should now be invalid
    EXPECT_FALSE(ExecutionFeeReportTransactionPrefix::isValidExecutionFeeReport(tx));
}

TEST(ExecutionFeeReportTransaction, RejectMisalignedEntries) {
    unsigned char buffer[512];
    ContractExecutionFeeEntry entries[1] = {
        {0, 0, 1000}
    };

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 1, entries);
    ASSERT_NE(tx, nullptr);

    // Valid initially
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));

    // Break alignment by adding 1 byte to inputSize
    // Payload size will no longer be divisible by sizeof(ContractExecutionFeeEntry)
    tx->inputSize += 1;

    // Should now have invalid alignment
    EXPECT_FALSE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
}

TEST(ExecutionFeeReportCollector, ValidateReportEntries) {
    ExecutionFeeReportCollector collector;
    collector.init();

    // Valid entries
    ContractExecutionFeeEntry validEntries[2] = {
        {0, 0, 1000},
        {1, 0, 2000}
    };
    EXPECT_TRUE(collector.validateReportEntries(validEntries, 2));

    // Invalid: contractIndex >= contractCount
    ContractExecutionFeeEntry invalidContract[1] = {
        {contractCount, 0, 1000}
    };
    EXPECT_FALSE(collector.validateReportEntries(invalidContract, 1));

    // Invalid: executionFee <= 0
    ContractExecutionFeeEntry zeroFee[1] = {
        {0, 0, 0}
    };
    EXPECT_FALSE(collector.validateReportEntries(zeroFee, 1));

    ContractExecutionFeeEntry negativeFee[1] = {
        {0, 0, -100}
    };
    EXPECT_FALSE(collector.validateReportEntries(negativeFee, 1));

    // Invalid: one good entry, one bad
    ContractExecutionFeeEntry mixedEntries[2] = {
        {0, 0, 1000},
        {contractCount + 5, 0, 2000}
    };
    EXPECT_FALSE(collector.validateReportEntries(mixedEntries, 2));
}

TEST(ExecutionFeeReportCollector, StoreReportEntries) {
    ExecutionFeeReportCollector collector;
    collector.init();

    ContractExecutionFeeEntry entries[3] = {
        {0, 0, 1000},
        {2, 0, 3000},
        {5, 0, 7000}
    };

    unsigned int computorIndex = 10;
    collector.storeReportEntries(entries, 3, computorIndex);

    // Verify entries were stored at correct positions
    long long* reports0 = collector.getReportsForContract(0);
    EXPECT_EQ(reports0[computorIndex], 1000);

    long long* reports2 = collector.getReportsForContract(2);
    EXPECT_EQ(reports2[computorIndex], 3000);

    long long* reports5 = collector.getReportsForContract(5);
    EXPECT_EQ(reports5[computorIndex], 7000);

    // Verify other positions remain zero
    EXPECT_EQ(reports0[0], 0);
    EXPECT_EQ(reports0[computorIndex + 1], 0);
}

TEST(ExecutionFeeReportCollector, MultipleComputorsReporting) {
    ExecutionFeeReportCollector collector;
    collector.init();

    // Computor 0 reports for contracts 0 and 1
    ContractExecutionFeeEntry comp0Entries[2] = {
        {0, 0, 1000},
        {1, 0, 2000}
    };
    collector.storeReportEntries(comp0Entries, 2, 0);

    // Computor 5 reports for contracts 0 and 2 (different fee for contract 0)
    ContractExecutionFeeEntry comp5Entries[2] = {
        {0, 0, 1500},
        {2, 0, 3000}
    };
    collector.storeReportEntries(comp5Entries, 2, 5);

    // Computor 10 reports for contract 1 (different fee than computor 0)
    ContractExecutionFeeEntry comp10Entries[1] = {
        {1, 0, 2500}
    };
    collector.storeReportEntries(comp10Entries, 1, 10);

    // Verify contract 0 has reports from computors 0 and 5
    long long* reports0 = collector.getReportsForContract(0);
    EXPECT_EQ(reports0[0], 1000);
    EXPECT_EQ(reports0[5], 1500);
    EXPECT_EQ(reports0[10], 0);  // Computor 10 didn't report for contract 0

    // Verify contract 1 has reports from computors 0 and 10
    long long* reports1 = collector.getReportsForContract(1);
    EXPECT_EQ(reports1[0], 2000);
    EXPECT_EQ(reports1[5], 0);   // Computor 5 didn't report for contract 1
    EXPECT_EQ(reports1[10], 2500);

    // Verify contract 2 has report only from computor 5
    long long* reports2 = collector.getReportsForContract(2);
    EXPECT_EQ(reports2[0], 0);
    EXPECT_EQ(reports2[5], 3000);
    EXPECT_EQ(reports2[10], 0);
}
