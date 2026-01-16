#define NO_UEFI

#include "contract_testing.h"
#include "../src/ticking/execution_fee_report_collector.h"
#include "../src/contract_core/execution_time_accumulator.h"

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

TEST(ExecutionFeeReportCollector, InitAndStore)
{
    ExecutionFeeReportCollector collector;
    collector.init();

    collector.storeReport(2, 0, 1000);
    collector.storeReport(2, 1, 2000);
    collector.storeReport(1, 0, 5000);
    collector.storeReport(1, 500, 6000);

    const unsigned long long* reports = collector.getReportsForContract(2);
    ASSERT_NE(reports, nullptr);
    EXPECT_EQ(reports[0], 1000);
    EXPECT_EQ(reports[1], 2000);

    reports = collector.getReportsForContract(1);
    ASSERT_NE(reports, nullptr);
    EXPECT_EQ(reports[0], 5000);
    EXPECT_EQ(reports[500], 6000);
}

TEST(ExecutionFeeReportCollector, Reset)
{
    ExecutionFeeReportCollector collector;
    collector.init();

    for (unsigned int i = 0; i < 10; i++) {
        collector.storeReport(1, i, i * 1000);
    }

    const unsigned long long* reports = collector.getReportsForContract(1);
    EXPECT_EQ(reports[5], 5000);

    collector.reset();

    reports = collector.getReportsForContract(1);
    EXPECT_EQ(reports[5], 0);
}

TEST(ExecutionFeeReportCollector, BoundaryValidation)
{
    ExecutionFeeReportCollector collector;
    collector.init();

    collector.storeReport(1, 0, 100);
    collector.storeReport(contractCount - 1, NUMBER_OF_COMPUTORS - 1, 200);

    collector.storeReport(contractCount, 0, 100);
    collector.storeReport(1, NUMBER_OF_COMPUTORS, 100);

    const unsigned long long* reports = collector.getReportsForContract(1);
    EXPECT_EQ(reports[0], 100);

    reports = collector.getReportsForContract(contractCount - 1);
    EXPECT_EQ(reports[NUMBER_OF_COMPUTORS - 1], 200);
}

TEST(ExecutionFeeReportTransaction, ParseValidTransaction)
{
    unsigned char buffer[512];
    unsigned int contractIndices[2] = {2, 1};
    long long executionFees[2] = {1000, 2000};

    Transaction* tx = createTestTransaction(buffer, sizeof(buffer), 2, contractIndices, executionFees);
    ASSERT_NE(tx, nullptr);

    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidExecutionFeeReport(tx));
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 2u);

    const unsigned int* parsedIndices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const unsigned long long* parsedFees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(parsedIndices[0], 2u);
    EXPECT_EQ(parsedFees[0], 1000);
    EXPECT_EQ(parsedIndices[1], 1u);
    EXPECT_EQ(parsedFees[1], 2000);
}

TEST(ExecutionFeeReportTransaction, RejectNonZeroAmount) {
    unsigned char buffer[512];
    unsigned int contractIndices[1] = {1};
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
    unsigned int contractIndices[1] = {1};
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
    unsigned int validIndices[2] = {1, 2};
    unsigned long long validFees[2] = {1000, 2000};
    EXPECT_TRUE(collector.validateReportEntries(validIndices, validFees, 2));

    // Invalid: contractIndex >= contractCount
    unsigned int invalidContractIndices[1] = {contractCount};
    unsigned long long invalidContractFees[1] = {1000};
    EXPECT_FALSE(collector.validateReportEntries(invalidContractIndices, invalidContractFees, 1));

    // Invalid: executionFee <= 0
    unsigned int zeroFeeIndices[1] = {1};
    unsigned long long zeroFees[1] = {0};
    EXPECT_FALSE(collector.validateReportEntries(zeroFeeIndices, zeroFees, 1));

    // Invalid: one good entry, one bad
    unsigned int mixedIndices[2] = {1, contractCount + 5};
    unsigned long long mixedFees[2] = {1000, 2000};
    EXPECT_FALSE(collector.validateReportEntries(mixedIndices, mixedFees, 2));
}

TEST(ExecutionFeeReportCollector, StoreReportEntries) {
    ExecutionFeeReportCollector collector;
    collector.init();

    unsigned int contractIndices[3] = {1, 2, 5};
    unsigned long long executionFees[3] = {1000, 3000, 7000};

    unsigned int computorIndex = 10;
    collector.storeReportEntries(contractIndices, executionFees, 3, computorIndex);

    // Verify entries were stored at correct positions
    const unsigned long long* reports1 = collector.getReportsForContract(1);
    EXPECT_EQ(reports1[computorIndex], 1000);

    const unsigned long long* reports2 = collector.getReportsForContract(2);
    EXPECT_EQ(reports2[computorIndex], 3000);

    const unsigned long long* reports5 = collector.getReportsForContract(5);
    EXPECT_EQ(reports5[computorIndex], 7000);

    // Verify other positions remain zero
    EXPECT_EQ(reports1[0], 0);
    EXPECT_EQ(reports1[computorIndex + 1], 0);
}

TEST(ExecutionFeeReportCollector, MultipleComputorsReporting) {
    ExecutionFeeReportCollector collector;
    collector.init();

    // Computor 0 reports for contracts 3 and 1
    unsigned int comp0Indices[2] = {3, 1};
    unsigned long long comp0Fees[2] = {1000, 2000};
    collector.storeReportEntries(comp0Indices, comp0Fees, /*numEntries=*/2, /*computorIndex=*/0);

    // Computor 5 reports for contracts 3 and 2 (different fee for contract 3)
    unsigned int comp5Indices[2] = {3, 2};
    unsigned long long comp5Fees[2] = {1500, 3000};
    collector.storeReportEntries(comp5Indices, comp5Fees, /*numEntries=*/2, /*computorIndex=*/5);

    // Computor 10 reports for contract 1 (different fee than computor 0)
    unsigned int comp10Indices[1] = {1};
    unsigned long long comp10Fees[1] = {2500};
    collector.storeReportEntries(comp10Indices, comp10Fees, /*numEntries=*/1, /*computorIndex=*/10);

    // Verify contract 3 has reports from computors 0 and 5
    const unsigned long long* reports3 = collector.getReportsForContract(3);
    EXPECT_EQ(reports3[0], 1000);
    EXPECT_EQ(reports3[5], 1500);
    EXPECT_EQ(reports3[10], 0);  // Computor 10 didn't report for contract 3

    // Verify contract 1 has reports from computors 0 and 10
    const unsigned long long* reports1 = collector.getReportsForContract(1);
    EXPECT_EQ(reports1[0], 2000);
    EXPECT_EQ(reports1[5], 0);   // Computor 5 didn't report for contract 1
    EXPECT_EQ(reports1[10], 2500);

    // Verify contract 2 has report only from computor 5
    const unsigned long long* reports2 = collector.getReportsForContract(2);
    EXPECT_EQ(reports2[0], 0);
    EXPECT_EQ(reports2[5], 3000);
    EXPECT_EQ(reports2[10], 0);
}

TEST(ExecutionFeeReportBuilder, BuildAndParseEvenEntries) {
    ExecutionFeeReportPayload payload;
    unsigned long long contractTimes[contractCount] = {0};
    contractTimes[1] = 200;
    contractTimes[3] = 100;

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 5, 1, 1);
    EXPECT_EQ(entryCount, 2u);

    // Verify transaction is valid and parseable
    Transaction* tx = (Transaction*)&payload;
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 2u);

    const unsigned int* indices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const unsigned long long* fees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(indices[0], 1u);
    EXPECT_EQ(fees[0], 200);  // (200 * 1) / 1
    EXPECT_EQ(indices[1], 3u);
    EXPECT_EQ(fees[1], 100);  // (100 * 1) / 1
}

TEST(ExecutionFeeReportBuilder, BuildAndParseOddEntries) {
    ExecutionFeeReportPayload payload;
    unsigned long long contractTimes[contractCount] = {0};
    contractTimes[1] = 100;
    contractTimes[2] = 300;
    contractTimes[5] = 600;

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 10, 1, 1);
    EXPECT_EQ(entryCount, 3u);

    // Verify transaction is valid and parseable (with alignment padding)
    Transaction* tx = (Transaction*)&payload;
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));
    EXPECT_EQ(ExecutionFeeReportTransactionPrefix::getNumEntries(tx), 3u);

    const unsigned int* indices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const unsigned long long* fees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(indices[0], 1u);
    EXPECT_EQ(fees[0], 100);  // (100 * 1) / 1
    EXPECT_EQ(indices[1], 2u);
    EXPECT_EQ(fees[1], 300);  // (300 * 1) / 1
    EXPECT_EQ(indices[2], 5u);
    EXPECT_EQ(fees[2], 600);  // (600 * 1) / 1
}

TEST(ExecutionFeeReportBuilder, NoEntriesReturnsZero) {
    ExecutionFeeReportPayload payload;
    unsigned long long contractTimes[contractCount] = {0};

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 7, 1, 1);
    EXPECT_EQ(entryCount, 0u);
}

TEST(ExecutionFeeReportBuilder, BuildWithDivisionMultiplier) {
    ExecutionFeeReportPayload payload;
    unsigned long long contractTimes[contractCount] = {0};
    contractTimes[1] = 5;    // Will become 0 after (5 * 1) / 10 - should be excluded
    contractTimes[2] = 25;   // Will become 2 after (25 * 1) / 10
    contractTimes[3] = 100;  // Will become 10 after (100 * 1) / 10

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 5, 1, 10);

    // Only contracts with non-zero fees after division should be included
    EXPECT_EQ(entryCount, 2u);  // contracts 0 and 2 (contract 1 becomes 0)

    Transaction* tx = (Transaction*)&payload;
    EXPECT_TRUE(ExecutionFeeReportTransactionPrefix::isValidEntryAlignment(tx));

    const unsigned int* indices = ExecutionFeeReportTransactionPrefix::getContractIndices(tx);
    const unsigned long long* fees = ExecutionFeeReportTransactionPrefix::getExecutionFees(tx);
    EXPECT_EQ(indices[0], 2u);
    EXPECT_EQ(fees[0], 2);  // (100 * 1) / 10
    EXPECT_EQ(indices[1], 3u);
    EXPECT_EQ(fees[1], 10);   // (25 * 1) / 10
}

TEST(ExecutionFeeReportBuilder, BuildWithMultiplicationMultiplier) {
    ExecutionFeeReportPayload payload;
    unsigned long long contractTimes[contractCount] = {0};
    contractTimes[1] = 10;
    contractTimes[3] = 25;

    unsigned int entryCount = buildExecutionFeeReportPayload(payload, contractTimes, 8, 100, 1);
    EXPECT_EQ(entryCount, 2u);

    const unsigned int* indices = ExecutionFeeReportTransactionPrefix::getContractIndices((Transaction*)&payload);
    const unsigned long long* fees = ExecutionFeeReportTransactionPrefix::getExecutionFees((Transaction*)&payload);
    EXPECT_EQ(indices[0], 1u);
    EXPECT_EQ(fees[0], 1000);  // (10 * 100) / 1
    EXPECT_EQ(indices[1], 3u);
    EXPECT_EQ(fees[1], 2500);  // (25 * 100) / 1
}

TEST(ExecutionTimeAccumulatorTest, AddingAndPhaseSwitching)
{
    ExecutionTimeAccumulator accum;

    accum.init();
    frequency = 0; // Bypass microseconds conversion

    accum.addTime(/*contractIndex=*/0, /*time=*/52784);
    accum.addTime(/*contractIndex=*/contractCount/2, /*time=*/8795);

    accum.startNewAccumulation();

    const unsigned long long* prevPhaseTimes = accum.getPrevPhaseAccumulatedTimes();

    for (unsigned int c = 0; c < contractCount; ++c)
    {
        if (c == 0)
            EXPECT_EQ(prevPhaseTimes[c], 52784);
        else if (c == contractCount / 2)
            EXPECT_EQ(prevPhaseTimes[c], 8795);
        else
            EXPECT_EQ(prevPhaseTimes[c], 0);
    }
}
