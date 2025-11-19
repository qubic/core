#define NO_UEFI

#include "contract_testing.h"
#include "../src/ticking/execution_fee_report_collector.h"

// Helper to create a valid baseline test transaction with given entries
static Transaction* createTestTransaction(unsigned char* buffer, size_t bufferSize,
                                          unsigned int numEntries,
                                          const unsigned int* contractIndices,
                                          const long long* executionFees)
{
    unsigned int alignmentPadding = (numEntries % 2 == 1) ? sizeof(unsigned int) : 0;
    const unsigned int inputSize = sizeof(unsigned int) + sizeof(unsigned int) +
                                    (numEntries * sizeof(unsigned int)) + alignmentPadding +
                                    (numEntries * sizeof(long long)) +
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
    *(unsigned int*)(inputPtr + 4) = numEntries;

    unsigned int* txIndices = (unsigned int*)(inputPtr + 8);
    for (unsigned int i = 0; i < numEntries; i++)
    {
        txIndices[i] = contractIndices[i];
    }

    long long* txFees = (long long*)(inputPtr + 8 + (numEntries * sizeof(unsigned int)) + alignmentPadding);
    for (unsigned int i = 0; i < numEntries; i++)
    {
        txFees[i] = executionFees[i];
    }

    m256i* dataLock = (m256i*)(inputPtr + 8 + (numEntries * sizeof(unsigned int)) + alignmentPadding + (numEntries * sizeof(long long)));
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
    unsigned int contractIndices[2] = {0, 1};
    long long executionFees[2] = {1000, 2000};

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 2, contractIndices, executionFees);
    ASSERT_NE(tx, nullptr);

    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidExecutionFeeReport(tx));
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 2u);

    const unsigned int* parsedIndices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const long long* parsedFees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(parsedIndices[0], 0u);
    EXPECT_EQ(parsedFees[0], 1000);
    EXPECT_EQ(parsedIndices[1], 1u);
    EXPECT_EQ(parsedFees[1], 2000);
}

TEST(ExecutionFeeReportTransaction, RejectNonZeroAmount) {
    unsigned char buffer[512];
    unsigned int contractIndices[1] = {0};
    long long executionFees[1] = {1000};

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 1, contractIndices, executionFees);
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
    unsigned int contractIndices[1] = {0};
    long long executionFees[1] = {1000};

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 1, contractIndices, executionFees);
    ASSERT_NE(tx, nullptr);

    // Valid initially
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));

    // Break alignment by adding 1 byte to inputSize
    // Payload size will no longer match expected size for numEntries
    tx->inputSize += 1;

    // Should now have invalid alignment
    EXPECT_FALSE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
}

TEST(ExecutionFeeReportCollector, ValidateReportEntries) {
    ExecutionFeeReportCollector collector;
    collector.init();

    // Valid entries
    unsigned int validIndices[2] = {0, 1};
    long long validFees[2] = {1000, 2000};
    EXPECT_TRUE(collector.validateReportEntries(validIndices, validFees, 2));

    // Invalid: contractIndex >= contractCount
    unsigned int invalidContractIndices[1] = {contractCount};
    long long invalidContractFees[1] = {1000};
    EXPECT_FALSE(collector.validateReportEntries(invalidContractIndices, invalidContractFees, 1));

    // Invalid: executionFee <= 0
    unsigned int zeroFeeIndices[1] = {0};
    long long zeroFees[1] = {0};
    EXPECT_FALSE(collector.validateReportEntries(zeroFeeIndices, zeroFees, 1));

    unsigned int negativeFeeIndices[1] = {0};
    long long negativeFees[1] = {-100};
    EXPECT_FALSE(collector.validateReportEntries(negativeFeeIndices, negativeFees, 1));

    // Invalid: one good entry, one bad
    unsigned int mixedIndices[2] = {0, contractCount + 5};
    long long mixedFees[2] = {1000, 2000};
    EXPECT_FALSE(collector.validateReportEntries(mixedIndices, mixedFees, 2));
}

TEST(ExecutionFeeReportCollector, StoreReportEntries) {
    ExecutionFeeReportCollector collector;
    collector.init();

    unsigned int contractIndices[3] = {0, 2, 5};
    long long executionFees[3] = {1000, 3000, 7000};

    unsigned int computorIndex = 10;
    collector.storeReportEntries(contractIndices, executionFees, 3, computorIndex);

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
    unsigned int comp0Indices[2] = {0, 1};
    long long comp0Fees[2] = {1000, 2000};
    collector.storeReportEntries(comp0Indices, comp0Fees, 2, 0);

    // Computor 5 reports for contracts 0 and 2 (different fee for contract 0)
    unsigned int comp5Indices[2] = {0, 2};
    long long comp5Fees[2] = {1500, 3000};
    collector.storeReportEntries(comp5Indices, comp5Fees, 2, 5);

    // Computor 10 reports for contract 1 (different fee than computor 0)
    unsigned int comp10Indices[1] = {1};
    long long comp10Fees[1] = {2500};
    collector.storeReportEntries(comp10Indices, comp10Fees, 1, 10);

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

TEST(ExecutionFeeReportBuilder, BuildAndParseEvenEntries) {
    ExecutionFeeReportPayload payload;
    long long contractTimes[contractCount] = {0};
    contractTimes[0] = 100;
    contractTimes[1] = 200;

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 5, 1000);
    EXPECT_EQ(entryCount, 2u);

    // Verify transaction is valid and parseable
    Transaction* tx = (Transaction*)&payload;
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 2u);

    const unsigned int* indices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const long long* fees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(indices[0], 0u);
    EXPECT_EQ(fees[0], 100000);
    EXPECT_EQ(indices[1], 1u);
    EXPECT_EQ(fees[1], 200000);
}

TEST(ExecutionFeeReportBuilder, BuildAndParseOddEntries) {
    ExecutionFeeReportPayload payload;
    long long contractTimes[contractCount] = {0};
    contractTimes[0] = 100;
    contractTimes[2] = 300;
    contractTimes[5] = 600;

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 10, 500);
    EXPECT_EQ(entryCount, 3u);

    // Verify transaction is valid and parseable (with alignment padding)
    Transaction* tx = (Transaction*)&payload;
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 3u);

    const unsigned int* indices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const long long* fees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(indices[0], 0u);
    EXPECT_EQ(fees[0], 50000);
    EXPECT_EQ(indices[1], 2u);
    EXPECT_EQ(fees[1], 150000);
    EXPECT_EQ(indices[2], 5u);
    EXPECT_EQ(fees[2], 300000);
}

TEST(ExecutionFeeReportBuilder, NoEntriesReturnsZero) {
    ExecutionFeeReportPayload payload;
    long long contractTimes[contractCount] = {0};

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 7, 1000);
    EXPECT_EQ(entryCount, 0u);
}
