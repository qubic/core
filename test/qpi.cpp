#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/smart_contracts/qpi.h"


TEST(TestCoreQPI, IndexStruct) {
    /*m256i test1(1, 2, 3, 4);
    m256i test2(3, 100, 579, 5431);
    m256i test3(1, 100, 579, 5431);
    m256i test4(123456789, 100, 579, 5431);
    m256i test5(12345689, 100, 579, 5431);

    // ==== TEST index_2x ====
    // for valid init you either need to call reset or load the data from a file
    QPI::index_2x i2x;
    i2x.reset();

    EXPECT_EQ(i2x.capacity(), 2);
    EXPECT_EQ(i2x.population(), 0);
    EXPECT_EQ(i2x.add(test1), 1);
    EXPECT_EQ(i2x.population(), 1);
    EXPECT_EQ(i2x.index(test1), 1);
    EXPECT_EQ(i2x.value(1), test1);
    EXPECT_EQ(i2x.add(test1), 1);
    EXPECT_EQ(i2x.population(), 1);

    // index 3 is mapped to 0 due to capacity limit
    EXPECT_EQ(i2x.capacity(), 2);
    EXPECT_EQ(i2x.population(), 1);
    EXPECT_EQ(i2x.add(test2), 0);
    EXPECT_EQ(i2x.population(), 2);
    EXPECT_EQ(i2x.index(test2), 0);
    EXPECT_EQ(i2x.value(0), test2);

    // adding fails due to capacity limit
    EXPECT_EQ(i2x.capacity(), 2);
    EXPECT_EQ(i2x.population(), 2);
    EXPECT_EQ(i2x.add(test3), QPI::NULL_INDEX);
    EXPECT_EQ(i2x.population(), 2);
    EXPECT_EQ(i2x.index(test3), QPI::NULL_INDEX);


    // ==== TEST index_4x ====
    // for valid init you either need to call reset or load the data from a file
    QPI::index_4x i4x;
    i4x.reset();

    EXPECT_EQ(i4x.capacity(), 4);
    EXPECT_EQ(i4x.population(), 0);
    EXPECT_EQ(i4x.add(test1), 1);
    EXPECT_EQ(i4x.population(), 1);
    EXPECT_EQ(i4x.index(test1), 1);
    EXPECT_EQ(i4x.value(1), test1);
    EXPECT_EQ(i4x.add(test1), 1);
    EXPECT_EQ(i4x.population(), 1);

    EXPECT_EQ(i4x.capacity(), 4);
    EXPECT_EQ(i4x.population(), 1);
    EXPECT_EQ(i4x.add(test2), 3);
    EXPECT_EQ(i4x.population(), 2);
    EXPECT_EQ(i4x.index(test2), 3);
    EXPECT_EQ(i4x.value(3), test2);

    // stored at index 2 because index 1 is already used
    EXPECT_EQ(i4x.capacity(), 4);
    EXPECT_EQ(i4x.population(), 2);
    EXPECT_EQ(i4x.add(test3), 2);
    EXPECT_EQ(i4x.population(), 3);
    EXPECT_EQ(i4x.index(test3), 2);
    EXPECT_EQ(i4x.value(2), test3);

    // stored at index 0 the only free entry
    EXPECT_EQ(i4x.capacity(), 4);
    EXPECT_EQ(i4x.population(), 3);
    EXPECT_EQ(i4x.add(test4), 0);
    EXPECT_EQ(i4x.population(), 4);
    EXPECT_EQ(i4x.index(test4), 0);
    EXPECT_EQ(i4x.value(0), test4);

    // adding fails due to capacity limit
    EXPECT_EQ(i4x.capacity(), 4);
    EXPECT_EQ(i4x.population(), 4);
    EXPECT_EQ(i4x.add(test5), QPI::NULL_INDEX);
    EXPECT_EQ(i4x.population(), 4);
    EXPECT_EQ(i4x.index(test5), QPI::NULL_INDEX);*/
}