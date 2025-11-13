#define NO_UEFI

#include "contract_testing.h"
#include "../src/ticking/execution_fee_counter.h"

TEST(ExecutionFeeCounter, InitAndStore) {
    ExecutionFeeCounter counter;
    counter.init();

    counter.storeReport(0, 0, 1000);
    counter.storeReport(0, 1, 2000);
    counter.storeReport(1, 0, 5000);
    counter.storeReport(1, 500, 6000);

    long long* reports = counter.getReportsForContract(0);
    ASSERT_NE(reports, nullptr);
    EXPECT_EQ(reports[0], 1000);
    EXPECT_EQ(reports[1], 2000);

    reports = counter.getReportsForContract(1);
    ASSERT_NE(reports, nullptr);
    EXPECT_EQ(reports[0], 5000);
    EXPECT_EQ(reports[500], 6000);
}

TEST(ExecutionFeeCounter, Reset) {
    ExecutionFeeCounter counter;
    counter.init();

    for (unsigned int i = 0; i < 10; i++) {
        counter.storeReport(0, i, i * 1000);
    }

    long long* reports = counter.getReportsForContract(0);
    EXPECT_EQ(reports[5], 5000);

    counter.reset();

    reports = counter.getReportsForContract(0);
    EXPECT_EQ(reports[5], 0);
}

TEST(ExecutionFeeCounter, BoundaryValidation) {
    ExecutionFeeCounter counter;
    counter.init();

    counter.storeReport(0, 0, 100);
    counter.storeReport(contractCount - 1, NUMBER_OF_COMPUTORS - 1, 200);

    counter.storeReport(contractCount, 0, 100);
    counter.storeReport(0, NUMBER_OF_COMPUTORS, 100);

    long long* reports = counter.getReportsForContract(0);
    EXPECT_EQ(reports[0], 100);

    reports = counter.getReportsForContract(contractCount - 1);
    EXPECT_EQ(reports[NUMBER_OF_COMPUTORS - 1], 200);
}
