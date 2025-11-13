#define NO_UEFI

#include "contract_testing.h"
#include "../src/ticking/execution_fee_report_collector.h"

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
