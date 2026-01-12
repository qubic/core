#define NO_UEFI

#include "gtest/gtest.h"
#include "../src/ticking/stable_computor_index.h"

class StableComputorIndexTest : public ::testing::Test
{
protected:
    m256i futureComputors[NUMBER_OF_COMPUTORS];
    m256i currentComputors[NUMBER_OF_COMPUTORS];
    unsigned char tempBuffer[stableComputorIndexBufferSize()];

    void SetUp() override
    {
        memset(futureComputors, 0, sizeof(futureComputors));
        memset(currentComputors, 0, sizeof(currentComputors));
        memset(tempBuffer, 0, sizeof(tempBuffer));
    }

    m256i makeId(int n)
    {
        m256i id = m256i::zero();
        id.m256i_u64[1] = n;
        return id;
    }
};

// Test: All computors requalify - all should keep their indices
TEST_F(StableComputorIndexTest, AllRequalify)
{
    // Set up current computors with IDs 1 to NUMBER_OF_COMPUTORS
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        currentComputors[i] = makeId(i + 1);
    }

    // Future: Same IDs but reversed order (simulating score reordering)
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        futureComputors[i] = makeId(NUMBER_OF_COMPUTORS - i);
    }

    bool result = calculateStableComputorIndex(futureComputors, currentComputors, tempBuffer);
    ASSERT_TRUE(result);

    // All should be back to their original indices
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(futureComputors[i], makeId(i + 1)) << "Index " << i << " mismatch";
    }
}

// Test: Half computors replaced - requalifying keep index, new fill gaps
TEST_F(StableComputorIndexTest, PartialRequalify)
{
    // Current: ID 1 at idx 0, ID 2 at idx 1, ..., ID 676 at idx 675
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        currentComputors[i] = makeId(i + 1);
    }

    // Future input (scrambled): odd IDs requalify, even IDs replaced by new (1001+)
    unsigned int requalifyingId = 1;
    unsigned int newId = 1001;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        if (i % 2 == 0 && requalifyingId <= NUMBER_OF_COMPUTORS)
        {
            futureComputors[i] = makeId(requalifyingId);
            requalifyingId += 2;
        }
        else
        {
            futureComputors[i] = makeId(newId++);
        }
    }

    bool result = calculateStableComputorIndex(futureComputors, currentComputors, tempBuffer);
    ASSERT_TRUE(result);

    // Odd IDs (1,3,5,...) should be at original indices (0,2,4,...)
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i += 2)
    {
        EXPECT_EQ(futureComputors[i], makeId(i + 1));
    }

    // New IDs (1001,1002,...) should fill gaps at indices (1,3,5,...)
    unsigned int expectedNewId = 1001;
    for (unsigned int i = 1; i < NUMBER_OF_COMPUTORS; i += 2)
    {
        EXPECT_EQ(futureComputors[i], makeId(expectedNewId++));
    }
}

// Test: All computors are new - order preserved
TEST_F(StableComputorIndexTest, AllNew)
{
    // Current: IDs 1 to 676
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        currentComputors[i] = makeId(i + 1);
    }

    // Future: Completely new set (IDs 1000 to 1675)
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        futureComputors[i] = makeId(i + 1000);
    }

    bool result = calculateStableComputorIndex(futureComputors, currentComputors, tempBuffer);
    ASSERT_TRUE(result);

    // New computors should fill slots in order
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(futureComputors[i], makeId(i + 1000));
    }
}

// Test: Single computor requalifies
TEST_F(StableComputorIndexTest, SingleRequalify)
{
    // Current: IDs 1 to 676
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        currentComputors[i] = makeId(i + 1);
    }

    // Future: Only ID 100 requalifies (at position 0), rest are new
    futureComputors[0] = makeId(100);  // Requalifying, was at idx 99
    for (unsigned int i = 1; i < NUMBER_OF_COMPUTORS; i++)
    {
        futureComputors[i] = makeId(i + 1000);  // New IDs
    }

    bool result = calculateStableComputorIndex(futureComputors, currentComputors, tempBuffer);
    ASSERT_TRUE(result);

    // ID 100 should be at its original index 99
    EXPECT_EQ(futureComputors[99], makeId(100));

    // New computors fill remaining slots (0-98, 100-675)
    unsigned int newIdx = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        if (i == 99) continue;  // Skip the requalifying slot
        EXPECT_EQ(futureComputors[i], makeId(newIdx + 1001)) << "New computor at index " << i;
        newIdx++;
    }
}


// Test: First and last computor swap positions in input
TEST_F(StableComputorIndexTest, FirstLastSwap)
{
    // Current: IDs 1 to 676
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        currentComputors[i] = makeId(i + 1);
    }

    // Future: All same IDs, but first and last swapped in input order
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        futureComputors[i] = makeId(i + 1);
    }
    futureComputors[0] = makeId(NUMBER_OF_COMPUTORS);  // Last ID at first position
    futureComputors[NUMBER_OF_COMPUTORS - 1] = makeId(1);  // First ID at last position

    bool result = calculateStableComputorIndex(futureComputors, currentComputors, tempBuffer);
    ASSERT_TRUE(result);

    // All should be at their original indices regardless of input order
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(futureComputors[i], makeId(i + 1)) << "Index " << i << " mismatch";
    }
}

// Test: Realistic scenario - 225 computors change (max allowed)
TEST_F(StableComputorIndexTest, MaxChange225)
{
    // Current: IDs 1 to 676
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        currentComputors[i] = makeId(i + 1);
    }

    // Future: First 451 (QUORUM) stay, last 225 are replaced with new IDs
    for (unsigned int i = 0; i < 451; i++)
    {
        futureComputors[i] = makeId(i + 1);  // Same IDs, possibly different order
    }
    for (unsigned int i = 451; i < NUMBER_OF_COMPUTORS; i++)
    {
        futureComputors[i] = makeId(i + 1000);  // New IDs
    }

    bool result = calculateStableComputorIndex(futureComputors, currentComputors, tempBuffer);
    ASSERT_TRUE(result);

    // First 451 should keep their indices
    for (unsigned int i = 0; i < 451; i++)
    {
        EXPECT_EQ(futureComputors[i], makeId(i + 1));
    }

    // Last 225 slots should have the new IDs
    for (unsigned int i = 451; i < NUMBER_OF_COMPUTORS; i++)
    {
        EXPECT_EQ(futureComputors[i], makeId(i + 1000));
    }
}

