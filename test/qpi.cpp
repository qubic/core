#define NO_UEFI

#include "contract_testing.h"

#include <type_traits>



// changing offset simulates changed computor set with changed epoch
void initComputors(unsigned short computorIdOffset)
{
    for (unsigned short computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; ++computorIndex)
    {
        broadcastedComputors.computors.publicKeys[computorIndex] = QPI::id(computorIndex + computorIdOffset, 9, 8, 7);
    }
}

TEST(TestCoreQPI, SafeMath)
{
    {
        sint64 a = -1000000000000LL;  // This is valid - negative signed integer
        sint64 b = 2;                 // Positive signed integer
        EXPECT_EQ(smul(a, b), -2000000000000);
    }

    {
        uint64_t a = 1000000;
        uint64_t b = 2000000;
        uint64_t expected_ok = 2000000000000ULL;
        EXPECT_EQ(smul(a, b), expected_ok);
    }
    {
        sint64 a = INT64_MIN;  // -9223372036854775808
        sint64 b = -1;         // -1
        EXPECT_EQ(smul(a, b), INT64_MAX);
    }
    {
        uint64_t a = 123456789ULL;
        uint64_t b = 987654321ULL;

        // Case: Multiplication by zero.
        EXPECT_EQ(smul(a, 0ULL), 0ULL);
        EXPECT_EQ(smul(0ULL, b), 0ULL);

        // Case: Multiplication by one.
        EXPECT_EQ(smul(a, 1ULL), a);
        EXPECT_EQ(smul(1ULL, b), b);
    }
    {
        // Case: A clear overflow case.
        // UINT64_MAX is approximately 1.84e19.
        uint64_t c = 4000000000ULL;
        uint64_t d = 5000000000ULL; // c * d is 2e19, which overflows.
        EXPECT_EQ(smul(c, d), UINT64_MAX);
    }
    {
        // Case: Test the exact boundary of overflow.
        uint64_t max_val = UINT64_MAX;
        uint64_t divisor = 2;
        uint64_t limit = max_val / divisor;

        // This should not overflow.
        EXPECT_EQ(smul(limit, divisor), limit * 2);

        // This should overflow and clamp.
        EXPECT_EQ(smul(limit + 1, divisor), UINT64_MAX);
    }
    {
        // Case: A simple multiplication that does not overflow.
        int64_t e = 1000000;
        int64_t f = -2000000;
        EXPECT_EQ(smul(e, f), -2000000000000LL);
    }
    {
        // Case: Positive * Positive, causing overflow.
        int64_t a = INT64_MAX / 2;
        int64_t b = 3;
        EXPECT_EQ(smul(a, b), INT64_MAX);
    }
    {
        int64_t a = INT64_MAX / 2;
        int64_t b = 3;
        int64_t c = -3;
        int64_t d = INT64_MIN / 2;

        // Case: Positive * Negative, causing underflow.
        EXPECT_EQ(smul(a, c), INT64_MIN);

        // Case: Negative * Positive, causing underflow.
        EXPECT_EQ(smul(d, b), INT64_MIN);
    }
    {
        // Case: Negative * Negative, causing overflow.
        int64_t c = -3;
        int64_t d = INT64_MIN / 2;
        EXPECT_EQ(smul(d, c), INT64_MAX);
    }
    {
        // --- Unsigned 32-bit Tests ---
        // No Overflow
        uint32_t a_u32 = 60000;
        uint32_t b_u32 = 60000;
        EXPECT_EQ(smul(a_u32, b_u32), 3600000000U);

        // Overflow
        uint32_t c_u32 = 70000;
        uint32_t d_u32 = 70000; // 70000*70000 = 4,900,000,000 which is > UINT32_MAX (~4.29e9)
        EXPECT_EQ(smul(c_u32, d_u32), UINT32_MAX);

        // Boundary
        uint32_t limit_u32 = UINT32_MAX / 2;
        uint32_t divisor_u32 = 2;
        EXPECT_EQ(smul(limit_u32, divisor_u32), limit_u32 * 2);
        EXPECT_EQ(smul(limit_u32 + 1, divisor_u32), UINT32_MAX);

        // --- Signed 32-bit Tests ---
        // No Overflow
        int32_t a_s32 = 10000;
        int32_t b_s32 = -10000;
        EXPECT_EQ(smul(a_s32, b_s32), -100000000);

        // Positive Overflow
        int32_t c_s32 = INT32_MAX / 2;
        int32_t d_s32 = 3;
        EXPECT_EQ(smul(c_s32, d_s32), INT32_MAX);

        // Underflow
        int32_t e_s32 = INT32_MIN / 2;
        int32_t f_s32 = 3;
        EXPECT_EQ(smul(e_s32, f_s32), INT32_MIN);

        // Negative * Negative, causing overflow.
        int32_t g_s32 = -3;
        int32_t h_s32 = INT32_MIN / 2;
        EXPECT_EQ(smul(h_s32, g_s32), INT32_MAX);
    }
}

TEST(TestCoreQPI, Array)
{
    //QPI::array<int, 0> mustFail; // should raise compile error

    QPI::Array<QPI::uint8, 4> uint8_4;
    EXPECT_EQ(uint8_4.capacity(), 4);
    //uint8_4.setMem(QPI::id(1, 2, 3, 4)); // should raise compile error
    uint8_4.setAll(2);
    EXPECT_EQ(uint8_4.get(0), 2);
    EXPECT_EQ(uint8_4.get(1), 2);
    EXPECT_EQ(uint8_4.get(2), 2);
    EXPECT_EQ(uint8_4.get(3), 2);
    EXPECT_TRUE(uint8_4.rangeEquals(0, 4, 2));
    EXPECT_TRUE(isArraySorted(uint8_4));
    EXPECT_FALSE(isArraySortedWithoutDuplicates(uint8_4));
    uint8_4.set(3, 1);
    EXPECT_FALSE(isArraySorted(uint8_4));
    EXPECT_FALSE(isArraySortedWithoutDuplicates(uint8_4));
    uint8_4.setRange(1, 3, 0);
    EXPECT_EQ(uint8_4.get(0), 2);
    EXPECT_EQ(uint8_4.get(1), 0);
    EXPECT_EQ(uint8_4.get(2), 0);
    EXPECT_EQ(uint8_4.get(3), 1);
    EXPECT_TRUE(uint8_4.rangeEquals(1, 3, 0));
    EXPECT_FALSE(isArraySorted(uint8_4));
    EXPECT_FALSE(isArraySortedWithoutDuplicates(uint8_4));
    for (int i = 0; i < uint8_4.capacity(); ++i)
        uint8_4.set(i, i+1);
    for (int i = 0; i < uint8_4.capacity(); ++i)
        EXPECT_EQ(uint8_4.get(i), i+1);
    EXPECT_FALSE(uint8_4.rangeEquals(0, 4, 2));
    EXPECT_TRUE(isArraySorted(uint8_4));
    EXPECT_TRUE(isArraySortedWithoutDuplicates(uint8_4));

    QPI::Array<QPI::uint64, 4> uint64_4;
    uint64_4.setMem(QPI::id(101, 102, 103, 104));
    for (int i = 0; i < uint64_4.capacity(); ++i)
        EXPECT_EQ(uint64_4.get(i), i + 101);
    //uint64_4.setMem(uint8_4); // should raise compile error

    QPI::Array<QPI::uint16, 2> uint16_2;
    EXPECT_EQ(uint8_4.capacity(), 4);
    //uint16_2.setMem(QPI::id(1, 2, 3, 4)); // should raise compile error
    uint16_2.setAll(12345);
    EXPECT_EQ((int)uint16_2.get(0), 12345);
    EXPECT_EQ((int)uint16_2.get(1), 12345);
    for (int i = 0; i < uint16_2.capacity(); ++i)
        uint16_2.set(i, i + 987);
    for (int i = 0; i < uint16_2.capacity(); ++i)
        EXPECT_EQ((int)uint16_2.get(i), i + 987);
    uint16_2.setMem(uint8_4);
    for (int i = 0; i < uint16_2.capacity(); ++i)
        EXPECT_EQ((int)uint16_2.get(i), (int)(((2*i+2) << 8) | (2*i + 1)));
}

TEST(TestCoreQPI, BitArray)
{
    //QPI::BitArray<0> mustFail;

    QPI::BitArray<1> b1;
    EXPECT_EQ(b1.capacity(), 1);
    b1.setAll(0);
    EXPECT_EQ(b1.get(0), 0);
    b1.setAll(1);
    EXPECT_EQ(b1.get(0), 1);
    b1.setAll(true);
    EXPECT_EQ(b1.get(0), 1);
    b1.set(0, 1);
    EXPECT_EQ(b1.get(0), 1);
    b1.set(0, 0);
    EXPECT_EQ(b1.get(0), 0);
    b1.set(0, true);
    EXPECT_EQ(b1.get(0), 1);

    b1.setAll(0);
    QPI::BitArray<1> b1_2;
    b1_2.setAll(0);
    QPI::BitArray<1> b1_3;
    b1_3.setAll(1);
    EXPECT_TRUE(b1 == b1_2);
    EXPECT_TRUE(b1 != b1_3);
    EXPECT_FALSE(b1 == b1_3);

    QPI::BitArray<64> b64;
    EXPECT_EQ(b64.capacity(), 64);
    b64.setMem(0x11llu);
    EXPECT_EQ(b64.get(0), 1);
    EXPECT_EQ(b64.get(1), 0);
    EXPECT_EQ(b64.get(2), 0);
    EXPECT_EQ(b64.get(3), 0);
    EXPECT_EQ(b64.get(4), 1);
    EXPECT_EQ(b64.get(5), 0);
    EXPECT_EQ(b64.get(6), 0);
    EXPECT_EQ(b64.get(7), 0);
    QPI::Array<QPI::uint64, 1> llu1;
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0x11llu);
    b64.setAll(0);
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0x0);
    b64.setAll(1);
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0xffffffffffffffffllu);


    b64.setMem(0x11llu);
    QPI::BitArray<64> b64_2;
    EXPECT_EQ(b64.capacity(), 64);
    b64_2.setMem(0x11llu);
    QPI::BitArray<64> b64_3;
    EXPECT_EQ(b64.capacity(), 64);
    b64_3.setMem(0x55llu);
    EXPECT_TRUE(b64 == b64_2);
    EXPECT_TRUE(b64 != b64_3);
    EXPECT_FALSE(b64 == b64_3);

    //QPI::BitArray<96> b96; // must trigger compile error

    QPI::BitArray<128> b128;
    EXPECT_EQ(b128.capacity(), 128);
    QPI::Array<QPI::uint64, 2> llu2;
    llu2.setAll(0x4llu);
    EXPECT_EQ(llu2.get(0), 0x4llu);
    EXPECT_EQ(llu2.get(1), 0x4llu);
    b128.setMem(llu2);
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            EXPECT_EQ(b128.get(i * 64 + j), j == 2);
        }
    }
    b128.set(0, 1);
    b128.set(2, 0);
    llu2.setMem(b128);
    EXPECT_EQ(llu2.get(0), 0x1llu);
    EXPECT_EQ(llu2.get(1), 0x4llu);
    for (int i = 0; i < b128.capacity(); ++i)
    {
        b128.set(i, i % 2 == 0);
        EXPECT_EQ(b128.get(i), i % 2 == 0);
    }
    llu2.setMem(b128);
    EXPECT_EQ(llu2.get(0), 0x5555555555555555llu);
    EXPECT_EQ(llu2.get(1), 0x5555555555555555llu);
    for (int i = 0; i < b128.capacity(); ++i)
    {
        EXPECT_EQ(b128.get(i), i % 2 == 0);
    }

    b128.setAll(1);
    QPI::BitArray<128> b128_2;
    QPI::BitArray<128> b128_3;
    b128_2.setAll(1);
    b128_3.setAll(0);
    EXPECT_TRUE(b128 == b128_2);
    EXPECT_TRUE(b128 != b128_3);
    EXPECT_FALSE(b128 == b128_3);
}

TEST(TestCoreQPI, Div) {
    EXPECT_EQ(QPI::div(0, 0), 0);
    EXPECT_EQ(QPI::div(10, 0), 0);
    EXPECT_EQ(QPI::div(0, 10), 0);
    EXPECT_EQ(QPI::div(20, 19), 1);
    EXPECT_EQ(QPI::div(20, 20), 1);
    EXPECT_EQ(QPI::div(20, 21), 0);
    EXPECT_EQ(QPI::div(20, 22), 0);
    EXPECT_EQ(QPI::div(50, 24), 2);
    EXPECT_EQ(QPI::div(50, 25), 2);
    EXPECT_EQ(QPI::div(50, 26), 1);
    EXPECT_EQ(QPI::div(50, 27), 1);
    EXPECT_EQ(QPI::div(-2, 0), 0);
    EXPECT_EQ(QPI::div(-2, 1), -2);
    EXPECT_EQ(QPI::div(-2, 2), -1);
    EXPECT_EQ(QPI::div(-2, 3), 0);
    EXPECT_EQ(QPI::div(2, -3), 0);
    EXPECT_EQ(QPI::div(2, -2), -1);
    EXPECT_EQ(QPI::div(2, -1), -2);

    EXPECT_EQ(QPI::div(0.0, 0.0), 0.0);
    EXPECT_EQ(QPI::div(50.0, 0.0), 0.0);
    EXPECT_EQ(QPI::div(0.0, 50.0), 0.0);
    EXPECT_EQ(QPI::div(-25.0, 50.0), -0.5);
    EXPECT_EQ(QPI::div(-25.0, -0.5), 50.0);
}

TEST(TestCoreQPI, Mod) {
    EXPECT_EQ(QPI::mod(0, 0), 0);
    EXPECT_EQ(QPI::mod(10, 0), 0);
    EXPECT_EQ(QPI::mod(0, 10), 0);
    EXPECT_EQ(QPI::mod(20, 19), 1);
    EXPECT_EQ(QPI::mod(20, 20), 0);
    EXPECT_EQ(QPI::mod(20, 21), 20);
    EXPECT_EQ(QPI::mod(20, 22), 20);
    EXPECT_EQ(QPI::mod(50, 23), 4);
    EXPECT_EQ(QPI::mod(50, 24), 2);
    EXPECT_EQ(QPI::mod(50, 25), 0);
    EXPECT_EQ(QPI::mod(50, 26), 24);
    EXPECT_EQ(QPI::mod(50, 27), 23);
    EXPECT_EQ(QPI::mod(-2, 0), 0);
    EXPECT_EQ(QPI::mod(-2, 1), 0);
    EXPECT_EQ(QPI::mod(-2, 2), 0);
    EXPECT_EQ(QPI::mod(-2, 3), -2);
    EXPECT_EQ(QPI::mod(2, -3), 2);
    EXPECT_EQ(QPI::mod(2, -2), 0);
    EXPECT_EQ(QPI::mod(2, -1), 0);
}

struct ContractExecInitDeinitGuard
{
    ContractExecInitDeinitGuard()
    {
        EXPECT_TRUE(initContractExec());
    }
    ~ContractExecInitDeinitGuard()
    {
        deinitContractExec();
    }
};

TEST(TestCoreQPI, ProposalAndVotingByComputors)
{
    ContractExecInitDeinitGuard initDeinitGuard;
    QpiContextUserProcedureCall qpi(0, QPI::id(1, 2, 3, 4), 123);
    QPI::ProposalAndVotingByComputors pv;
    initComputors(0);

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(pv, 0);

    // vote index is computor index
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        EXPECT_EQ(pv.getVoteIndex(qpi, qpi.computor(i)), i);
        EXPECT_EQ(pv.getVoterId(qpi, i), qpi.computor(i));
        EXPECT_EQ(pv.getVoteCount(qpi, i), 1);
    }
    for (int i = NUMBER_OF_COMPUTORS; i < 800; ++i)
    {
        QPI::id testId(i, 9, 8, 7);
        EXPECT_EQ(pv.getVoteIndex(qpi, testId), QPI::INVALID_VOTE_INDEX);
        EXPECT_EQ(pv.getVoterId(qpi, i), QPI::NULL_ID);
        EXPECT_EQ(pv.getVoteCount(qpi, i), 0);
    }
    EXPECT_EQ(pv.getVoteIndex(qpi, qpi.originator()), QPI::INVALID_VOTE_INDEX);

    // valid proposers are computors
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
        EXPECT_TRUE(pv.isValidProposer(qpi, qpi.computor(i)));
    for (int i = NUMBER_OF_COMPUTORS; i < 2 * NUMBER_OF_COMPUTORS; ++i)
        EXPECT_FALSE(pv.isValidProposer(qpi, QPI::id(i, 9, 8, 7)));
    EXPECT_FALSE(pv.isValidProposer(qpi, QPI::NULL_ID));
    EXPECT_FALSE(pv.isValidProposer(qpi, qpi.originator()));

    // no existing proposals
    for (int i = 0; i < 2*NUMBER_OF_COMPUTORS; ++i)
    {
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(i)), (int)QPI::INVALID_PROPOSAL_INDEX);
        EXPECT_EQ(pv.getProposerId(qpi, i), NULL_ID);
    }
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, NULL_ID), (int)QPI::INVALID_PROPOSAL_INDEX);

    // fill all slots
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getNewProposalIndex(qpi, qpi.computor(j)), i);
    }

    // proposals now available
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(j)), i);
        EXPECT_EQ(pv.getProposerId(qpi, i), qpi.computor(j));
    }

    // using other ID fails if full (new computor ID after epoch change)
    QPI::id newId(9, 8, 7, 6);
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, newId), (int)QPI::INVALID_PROPOSAL_INDEX);

    // reusing all slots should work
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getNewProposalIndex(qpi, qpi.computor(j)), i);
    }

    // free one slot
    pv.freeProposalByIndex(qpi, 0);

    // using other ID now works (new computor ID after epoch change)
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, newId), 0);

    // check content
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, newId), 0);
    for (int i = 1; i < NUMBER_OF_COMPUTORS; ++i)
    {
        int j = NUMBER_OF_COMPUTORS - 1 - i;
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(j)), i);
    }
}

static void sortContractShareVector(std::vector<std::pair<m256i, unsigned int>> & shareholders)
{
    std::sort(shareholders.begin(), shareholders.end(),
        [](const std::pair<m256i, unsigned int>& a, const std::pair<m256i, unsigned int>& b)
        {
            return a.first < b.first;
        });
}

template <uint16 proposalSlotCount, uint64 contractAssetName>
static void checkShareholderVotingRights(const QpiContextUserProcedureCall& qpi, const ProposalAndVotingByShareholders<proposalSlotCount, contractAssetName>& pv, uint16 proposalIdx, std::vector<std::pair<m256i, unsigned int>>& shareholderVec)
{
    unsigned int voterIdx = 0;
    for (const auto& ownerSharesPair : shareholderVec)
    {
        const m256i owner = ownerSharesPair.first;
        const auto shareCount = ownerSharesPair.second;
        EXPECT_EQ(pv.getVoteIndex(qpi, owner, proposalIdx), voterIdx);
        for (unsigned int i = 0; i < shareCount; ++i)
        {
            EXPECT_EQ(pv.getVoterId(qpi, voterIdx, proposalIdx), owner);
            EXPECT_EQ(pv.getVoteCount(qpi, voterIdx, proposalIdx), shareCount - i);
            ++voterIdx;
        }
    }
    EXPECT_EQ(voterIdx, NUMBER_OF_COMPUTORS);
    EXPECT_EQ(pv.getVoteIndex(qpi, NULL_ID, proposalIdx), INVALID_VOTE_INDEX);
    EXPECT_EQ(pv.getVoteIndex(qpi, id(12345678, 901234, 5678, 90), proposalIdx), INVALID_VOTE_INDEX);
    EXPECT_EQ(pv.getVoterId(qpi, voterIdx, proposalIdx), NULL_ID);
    EXPECT_EQ(pv.getVoteCount(qpi, voterIdx, proposalIdx), 0);
}

TEST(TestCoreQPI, ProposalAndVotingByShareholders)
{
    ContractTesting test;
    test.initEmptyUniverse();
    QpiContextUserProcedureCall qpi(QX_CONTRACT_INDEX, QPI::id(1, 2, 3, 4), 123);
    ProposalAndVotingByShareholders<3, MSVAULT_ASSET_NAME> pv;
    initComputors(0);
    
    // Memory must be zeroed to work, which is done in contract states on init
    setMemory(pv, 0);

    // create contract shares
    std::vector<std::pair<id, unsigned int>> initialPossessorShares{
        {id(100, 2, 3, 4), 10},
        {id(1, 2, 3, 4), 200},
        {id(1, 2, 2, 1), 65},
        {id(1, 2, 3, 1), 1},
        {id(0, 0, 0, 1), 400},
    };
    issueContractShares(MSVAULT_CONTRACT_INDEX, initialPossessorShares);
    sortContractShareVector(initialPossessorShares);

    // no existing proposals
    for (int i = 0; i < initialPossessorShares.size(); ++i)
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[i].first), (int)INVALID_PROPOSAL_INDEX);

    // valid proposers are current shareholders
    for (int i = 0; i < initialPossessorShares.size(); ++i)
        EXPECT_TRUE(pv.isValidProposer(qpi, initialPossessorShares[i].first));
    for (int i = 0; i < 2 * NUMBER_OF_COMPUTORS; ++i)
    {
        EXPECT_FALSE(pv.isValidProposer(qpi, QPI::id(i, 9, 8, 7)));
        EXPECT_EQ(pv.getProposerId(qpi, i), NULL_ID);
    }
    EXPECT_FALSE(pv.isValidProposer(qpi, QPI::NULL_ID));

    // add proposal
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[0].first), 0);
    for (int i = 0; i < initialPossessorShares.size(); ++i)
    {
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[i].first), (i == 0) ? 0 : (int)INVALID_PROPOSAL_INDEX);
        EXPECT_EQ(pv.getProposerId(qpi, i), (i == 0) ? initialPossessorShares[i].first : NULL_ID);
    }
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, NULL_ID), (int)QPI::INVALID_PROPOSAL_INDEX);

    // check voting rights (voters are shareholders at time of creating/updating proposal)
    checkShareholderVotingRights(qpi, pv, 0, initialPossessorShares);

    // transfer some shares
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 1, id(20, 2, 3, 5)), 199);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 1, id(30, 2, 3, 5)), 198);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 1, id(40, 2, 3, 5)), 197);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 1, id(50, 2, 3, 5)), 196);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 16, id(1, 2, 3, 5)), 180);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 60, id(1, 2, 3, 1)), 120);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 39, id(1, 2, 3, 0)), 81);
    EXPECT_EQ(qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 1, id(1, 2, 3, 2)), 80);
    std::vector<std::pair<id, unsigned int>> changedPossessorShares{
        {id(0, 0, 0, 1), 400},
        {id(1, 2, 2, 1), 65},
        {id(1, 2, 3, 0), 39},
        {id(1, 2, 3, 1), 61},
        {id(1, 2, 3, 2), 1},
        {id(1, 2, 3, 4), 80},
        {id(1, 2, 3, 5), 16},
        {id(20, 2, 3, 5), 1},
        {id(30, 2, 3, 5), 1},
        {id(40, 2, 3, 5), 1},
        {id(50, 2, 3, 5), 1},
        {id(100, 2, 3, 4), 10},
    };

    // assetsEndEpoch() and as.indexLists.rebuild(), which are called at the end of each epoch, lead to speeding up creating a new proposal (by requiring less sorting operations)
    as.indexLists.rebuild();

    // add another proposal (has changed set of voters)
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[1].first), 1);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[0].first), 0);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[1].first), 1);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[2].first), (int)INVALID_PROPOSAL_INDEX);

    // check voting rights (voters are shareholders at time of creating/updating proposal)
    checkShareholderVotingRights(qpi, pv, 0, initialPossessorShares);
    checkShareholderVotingRights(qpi, pv, 1, changedPossessorShares);

    // add third proposal (has changed set of voters)
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[2].first), 2);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[0].first), 0);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[1].first), 1);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[2].first), 2);

    // check voting rights (voters are shareholders at time of creating/updating proposal)
    checkShareholderVotingRights(qpi, pv, 0, initialPossessorShares);
    checkShareholderVotingRights(qpi, pv, 1, changedPossessorShares);
    checkShareholderVotingRights(qpi, pv, 2, changedPossessorShares);

    // transfer some shares
    qpi.transferShareOwnershipAndPossession(MSVAULT_ASSET_NAME, NULL_ID, qpi.invocator(), qpi.invocator(), 80, id(1, 2, 2, 0));
    std::vector<std::pair<id, unsigned int>> changedPossessorShares2{
        {id(0, 0, 0, 1), 400},
        {id(1, 2, 2, 0), 80},
        {id(1, 2, 2, 1), 65},
        {id(1, 2, 3, 0), 39},
        {id(1, 2, 3, 1), 61},
        {id(1, 2, 3, 2), 1},
        //{id(1, 2, 3, 4), 0},
        {id(1, 2, 3, 5), 16},
        {id(20, 2, 3, 5), 1},
        {id(30, 2, 3, 5), 1},
        {id(40, 2, 3, 5), 1},
        {id(50, 2, 3, 5), 1},
        {id(100, 2, 3, 4), 10},
    };

    // all slots filled -> adding proposal using other ID fails
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[3].first), (int)INVALID_PROPOSAL_INDEX);

    // free one slot
    pv.freeProposalByIndex(qpi, 1);

    // using other ID now works
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[3].first), 1);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[0].first), 0);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[1].first), (int)INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[2].first), 2);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[3].first), 1);

    // check voting rights (voters are shareholders at time of creating/updating proposal)
    checkShareholderVotingRights(qpi, pv, 0, initialPossessorShares);
    checkShareholderVotingRights(qpi, pv, 1, changedPossessorShares2);
    checkShareholderVotingRights(qpi, pv, 2, changedPossessorShares);

    // reusing all slots should work (overwriting proposals sets voters)
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[0].first), 0);
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[1].first), (int)INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[2].first), 2);
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[3].first), 1);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[0].first), 0);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[1].first), (int)INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[2].first), 2);
    EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, initialPossessorShares[3].first), 1);

    // check voting rights (voters are shareholders at time of creating/updating proposal)
    checkShareholderVotingRights(qpi, pv, 0, changedPossessorShares2);
    checkShareholderVotingRights(qpi, pv, 1, changedPossessorShares2);
    checkShareholderVotingRights(qpi, pv, 2, changedPossessorShares2);
}

TEST(TestCoreQPI, ProposalAndVotingByShareholdersTestSorting)
{
    constexpr int shareholderFactor = 512; // set 2 to run more tests
    constexpr int shareholdersWithTwoRecordsStep = 5; // set 1 to run more tests

    static constexpr uint64 MSVAULT_ASSET_NAME = 23727827095802701;
    for (int shareholders = 1; shareholders <= 512; shareholders *= shareholderFactor)
    {
        for (int shareholdersWithTwoRecords = min(shareholders, 4); shareholdersWithTwoRecords > 0; shareholdersWithTwoRecords -= shareholdersWithTwoRecordsStep)
        {
            ContractTesting test;
            test.initEmptyUniverse();
            QpiContextUserProcedureCall qpi(QX_CONTRACT_INDEX, QPI::id(1, 2, 3, 4), 123);
            ProposalAndVotingByShareholders<3, MSVAULT_ASSET_NAME> pv;
            initComputors(0);

            // Memory must be zeroed to work, which is done in contract states on init
            setMemory(pv, 0);

            // create contract shares
            std::vector<std::pair<id, unsigned int>> initialPossessorShares;
            std::vector<unsigned int> twoRecords;
            for (int i = 0; i < shareholders; ++i)
            {
                initialPossessorShares.push_back({ id::randomValue(), 1 });
            }
            for (int i = 0; i < shareholdersWithTwoRecords; ++i)
            {
                // randomly select entries that shall have two records
                int idx = (int)initialPossessorShares[i].first.u64._0 % initialPossessorShares.size();
                initialPossessorShares[idx].second = 2;
                twoRecords.push_back(idx);
            }
            issueContractShares(MSVAULT_CONTRACT_INDEX, initialPossessorShares, false);

            // transfer assset management rights in order to create two records
            Asset asset{ NULL_ID, MSVAULT_ASSET_NAME };
            for (int i = 0; i < shareholdersWithTwoRecords; ++i)
            {
                const id& possessorId = initialPossessorShares[twoRecords[i]].first;
                AssetPossessionIterator it(asset, { possessorId, QX_CONTRACT_INDEX }, { possessorId, QX_CONTRACT_INDEX });
                EXPECT_TRUE(transferShareManagementRights(it.ownershipIndex(), it.possessionIndex(), MSVAULT_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX, 1, nullptr, nullptr, true));
            }

            // add proposal
            EXPECT_EQ((int)pv.getNewProposalIndex(qpi, initialPossessorShares[0].first), 0);

            // check voting rights (voters are shareholders at time of creating/updating proposal)
            sortContractShareVector(initialPossessorShares);
            checkShareholderVotingRights(qpi, pv, 0, initialPossessorShares);
        }
    }
}

// Test internal class ProposalWithAllVoteData that stores valid proposals along with its votes
template <typename ProposalT, QPI::uint32 numVoters>
void testProposalWithAllVoteDataOptionVotes(
    QPI::ProposalWithAllVoteData<ProposalT, numVoters>& pwav,
    const ProposalT& proposal,
    QPI::sint64 numOptions
)
{
    ASSERT_TRUE(numOptions >= 2);
    ASSERT_TRUE(proposal.checkValidity());
    ASSERT_TRUE(QPI::ProposalTypes::isValid(proposal.type));
    if (proposal.type == QPI::ProposalTypes::VariableScalarMean)
        ASSERT_TRUE(QPI::ProposalTypes::optionCount(proposal.type) == 0);
    else
        ASSERT_TRUE(QPI::ProposalTypes::optionCount(proposal.type) == numOptions);

    // set proposal / clear votes
    EXPECT_TRUE(pwav.set(proposal));
    for (QPI::uint32 i = 0; i < numVoters; ++i)
        EXPECT_EQ(pwav.getVoteValue(i), QPI::NO_VOTE_VALUE);

    // test that out-of-range voter indices cannot be set
    EXPECT_FALSE(pwav.setVoteValue(numVoters, 0));
    EXPECT_FALSE(pwav.setVoteValue(numVoters, QPI::NO_VOTE_VALUE));
    EXPECT_FALSE(pwav.setVoteValue(numVoters+1, 123));

    // test that out-of-range vote values cannot be set
    EXPECT_FALSE(pwav.setVoteValue(0, -123456));
    EXPECT_FALSE(pwav.setVoteValue(0, -1));
    EXPECT_FALSE(pwav.setVoteValue(0, numOptions));
    EXPECT_FALSE(pwav.setVoteValue(0, 123456));

    // set and check valid vote range
    for (QPI::uint32 j = 0; j < numOptions; ++j)
    {
        for (QPI::uint32 i = 0; i < numVoters; ++i)
            EXPECT_TRUE(pwav.setVoteValue(i, (j + i) % numOptions));
        for (QPI::uint32 i = 0; i < numVoters; ++i)
            EXPECT_EQ(pwav.getVoteValue(i), (j + i) % numOptions);

        for (QPI::uint32 i = 0; i < numVoters; ++i)
            EXPECT_TRUE(pwav.setVoteValue(i, (j + numVoters - i) % numOptions));
        for (QPI::uint32 i = 0; i < numVoters; ++i)
            EXPECT_EQ(pwav.getVoteValue(i), (j + numVoters - i) % numOptions);
    }

    // clear vote
    for (QPI::uint32 i = 0; i < numVoters; ++i)
        EXPECT_TRUE(pwav.setVoteValue(i, QPI::NO_VOTE_VALUE));
    for (QPI::uint32 i = 0; i < numVoters; ++i)
        EXPECT_EQ(pwav.getVoteValue(i), QPI::NO_VOTE_VALUE);
}

// Test internal class ProposalWithAllVoteData that stores valid proposals along with its votes
template <bool supportScalarVotes>
void testProposalWithAllVoteData()
{
    typedef QPI::ProposalDataV1<supportScalarVotes> ProposalT;
    QPI::ProposalWithAllVoteData<ProposalT, 42> pwav;
    ProposalT proposal;

    // YesNo proposal
    proposal.type = QPI::ProposalTypes::YesNo;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // ThreeOption proposal
    proposal.type = QPI::ProposalTypes::ThreeOptions;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 3);

    // Proposal with 8 options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 8);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 8);

    // TransferYesNo proposal
    proposal.type = QPI::ProposalTypes::TransferYesNo;
    proposal.transfer.destination = QPI::id(1, 2, 3, 4);
    proposal.transfer.amounts.setAll(0);
    proposal.transfer.amounts.set(0, 1234);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // TransferTwoAmounts
    proposal.type = QPI::ProposalTypes::TransferTwoAmounts;
    proposal.transfer.amounts.set(1, 12345);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 3);

    // TransferThreeAmounts
    proposal.type = QPI::ProposalTypes::TransferThreeAmounts;
    proposal.transfer.amounts.set(2, 123456);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 4);

    // TransferFourAmounts
    proposal.type = QPI::ProposalTypes::TransferFourAmounts;
    proposal.transfer.amounts.set(3, 1234567);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 5);

    // TransferInEpochYesNo proposal
    proposal.type = QPI::ProposalTypes::TransferInEpochYesNo;
    proposal.transferInEpoch.destination = QPI::id(1, 2, 3, 4);
    proposal.transferInEpoch.amount = 10;
    proposal.transferInEpoch.targetEpoch = 123;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // fail: test TransferInEpoch proposal with too many or too few options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::TransferInEpoch, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::TransferInEpoch, 3);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());

    // VariableYesNo proposal
    proposal.type = QPI::ProposalTypes::VariableYesNo;
    proposal.variableOptions.variable = 42;
    proposal.variableOptions.values.set(0, 987);
    proposal.variableOptions.values.setRange(1, proposal.variableOptions.values.capacity(), 0);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // VariableTwoValues proposal
    proposal.type = QPI::ProposalTypes::VariableTwoValues;
    proposal.variableOptions.values.set(1, 9876);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 3);

    // VariableThreeValues proposal
    proposal.type = QPI::ProposalTypes::VariableThreeValues;
    proposal.variableOptions.values.set(2, 98765);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 4);

    // VariableFourValues proposal
    proposal.type = QPI::ProposalTypes::VariableFourValues;
    proposal.variableOptions.values.set(3, 987654);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 5);

    // fail: test variable proposal with too many or too few options (0 options means scalar)
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Variable, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Variable, 6);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());

    // fail: wrong sorting with class Variable
    proposal.type = QPI::ProposalTypes::VariableFourValues;
    for (int i = 0; i < 4; ++i)
        proposal.variableOptions.values.set(i, 20 - i);
    EXPECT_FALSE(proposal.checkValidity());

    // VariableScalarMean proposal
    proposal.type = QPI::ProposalTypes::VariableScalarMean;
    proposal.variableScalar.variable = 42;
    proposal.variableScalar.minValue = 0;
    proposal.variableScalar.maxValue = 25;
    proposal.variableScalar.proposedValue = 1;
    if (supportScalarVotes)
        testProposalWithAllVoteDataOptionVotes(pwav, proposal, 26);
    else
        EXPECT_FALSE(pwav.set(proposal));

    // MultiVariablesYesNo proposal
    proposal.type = QPI::ProposalTypes::MultiVariablesYesNo;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // MultiVariablesThreeOptions proposal
    proposal.type = QPI::ProposalTypes::MultiVariablesThreeOptions;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 3);

    // MultiVariablesFourOptions proposal
    proposal.type = QPI::ProposalTypes::MultiVariablesFourOptions;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 4);

    // MultiVariables proposal with 8 options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::MultiVariables, 8);
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 8);

    // fail: test MultiVariables proposal with too many or too few options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::MultiVariables, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::MultiVariables, 9);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());
}

TEST(TestCoreQPI, ProposalWithAllVoteDataWithScalarVoteSupport)
{
    testProposalWithAllVoteData<true>();
}

TEST(TestCoreQPI, ProposalWithAllVoteDataWithoutScalarVoteSupport)
{
    testProposalWithAllVoteData<false>();
}

TEST(TestCoreQPI, ProposalWithAllVoteDataYesNoProposals)
{
    // Using ProposalDataYesNo saves storage space by only supporting yes/no choices
    // (or up to 3 options for proposal classes that don't store option values)
    ContractExecInitDeinitGuard initDeinitGuard;
    typedef QPI::ProposalDataYesNo ProposalT;
    QPI::ProposalWithAllVoteData<ProposalT, 42> pwav;
    ProposalT proposal;

    // YesNo proposal
    proposal.type = QPI::ProposalTypes::YesNo;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // ThreeOption proposal (accepted for general proposal, because it does not cost anything)
    proposal.type = QPI::ProposalTypes::ThreeOptions;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 3);

    // Proposal with 4 options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 4);
    EXPECT_FALSE(proposal.checkValidity());

    // Proposal with 8 options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 8);
    EXPECT_FALSE(proposal.checkValidity());

    // TransferYesNo proposal
    proposal.type = QPI::ProposalTypes::TransferYesNo;
    proposal.transfer.destination = QPI::id(1, 2, 3, 4);
    proposal.transfer.amount = 1234;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // TransferTwoAmounts
    proposal.type = QPI::ProposalTypes::TransferTwoAmounts;
    EXPECT_FALSE(proposal.checkValidity());

    // TransferThreeAmounts
    proposal.type = QPI::ProposalTypes::TransferThreeAmounts;
    EXPECT_FALSE(proposal.checkValidity());

    // fail: TransferInEpochYesNo proposal not supported due to lack of storage
    proposal.type = QPI::ProposalTypes::TransferInEpochYesNo;
    EXPECT_FALSE(proposal.checkValidity());

    // VariableYesNo proposal
    proposal.type = QPI::ProposalTypes::VariableYesNo;
    proposal.variableOptions.variable = 42;
    proposal.variableOptions.value = 987;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // VariableTwoValues proposal
    proposal.type = QPI::ProposalTypes::VariableTwoValues;
    EXPECT_FALSE(proposal.checkValidity());

    // VariableThreeValues proposal
    proposal.type = QPI::ProposalTypes::VariableThreeValues;
    EXPECT_FALSE(proposal.checkValidity());

    // fail: test variable proposal with too many or too few options (0 options means scalar)
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Variable, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Variable, 6);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    EXPECT_FALSE(proposal.checkValidity());

    // VariableScalarMean proposal
    proposal.type = QPI::ProposalTypes::VariableScalarMean;
    EXPECT_FALSE(proposal.checkValidity());

    // MultiVariablesYesNo proposal
    proposal.type = QPI::ProposalTypes::MultiVariablesYesNo;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 2);

    // MultiVariablesThreeOptions proposal (accepted for multiple variables proposal, because it does not cost anything)
    proposal.type = QPI::ProposalTypes::MultiVariablesThreeOptions;
    testProposalWithAllVoteDataOptionVotes(pwav, proposal, 3);

    // MultiVariablesFourOptions proposal
    proposal.type = QPI::ProposalTypes::MultiVariablesFourOptions;
    EXPECT_FALSE(proposal.checkValidity());
}

template <typename ProposalVotingType>
void expectNoVotes(
    const QPI::QpiContextFunctionCall& qpi,
    const ProposalVotingType& pv,
    QPI::uint16 proposalIndex
)
{
    QPI::ProposalSingleVoteDataV1 vote;
    for (QPI::uint32 i = 0; i < pv->maxVotes; ++i)
    {
        EXPECT_TRUE(qpi(*pv).getVote(proposalIndex, i, vote));
        EXPECT_EQ(vote.voteValue, QPI::NO_VOTE_VALUE);
    }

    QPI::ProposalSummarizedVotingDataV1 votingSummaryReturned;
    EXPECT_TRUE(qpi(*pv).getVotingSummary(proposalIndex, votingSummaryReturned));
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 0);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), -1);
}

template <bool B>
bool isReturnedProposalAsExpected(
    const QPI::QpiContextFunctionCall& qpi,
    const QPI::ProposalDataV1<B>& proposalReturnedByGet, const QPI::ProposalDataV1<B>& proposalSet)
{
    bool expected =
        proposalReturnedByGet.tick == qpi.tick()
        && proposalReturnedByGet.epoch == qpi.epoch()
        && proposalReturnedByGet.type == proposalSet.type
        && proposalReturnedByGet.supportScalarVotes == proposalSet.supportScalarVotes
        && (memcmp(&proposalReturnedByGet.transfer, &proposalSet.transfer, sizeof(proposalSet.transfer)) == 0)
        && (memcmp(&proposalReturnedByGet.variableOptions, &proposalSet.variableOptions, sizeof(proposalSet.variableOptions)) == 0)
        && (memcmp(&proposalReturnedByGet.variableScalar, &proposalSet.variableScalar, sizeof(proposalSet.variableScalar)) == 0)
        && (memcmp(&proposalReturnedByGet.url, &proposalSet.url, sizeof(proposalSet.url)) == 0);
    return expected;
}

bool operator==(const QPI::ProposalSingleVoteDataV1& p1, const QPI::ProposalSingleVoteDataV1& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}

bool operator==(const QPI::ProposalMultiVoteDataV1& p1, const QPI::ProposalMultiVoteDataV1& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}


template <typename ProposalVotingType, typename ProposalDataType>
void setProposalWithSuccessCheck(const QPI::QpiContextProcedureCall& qpi, const ProposalVotingType& pv, const QPI::id& proposerId, const ProposalDataType& proposal)
{
    ProposalDataType proposalReturned;
    EXPECT_NE((int)qpi(*pv).setProposal(proposerId, proposal), (int)QPI::INVALID_PROPOSAL_INDEX);
    QPI::uint16 proposalIdx = qpi(*pv).proposalIndex(proposerId);
    EXPECT_NE((int)proposalIdx, (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(proposalIdx), proposerId);
    EXPECT_TRUE(qpi(*pv).getProposal(proposalIdx, proposalReturned));
    EXPECT_TRUE(isReturnedProposalAsExpected(qpi, proposalReturned, proposal));
    expectNoVotes(qpi, pv, proposalIdx);
}

template <typename ProposalVotingType, typename ProposalDataType>
void setProposalExpectFailure(const QPI::QpiContextProcedureCall& qpi, const ProposalVotingType& pv, const QPI::id& proposerId, const ProposalDataType& proposal)
{
    EXPECT_EQ((int)qpi(*pv).setProposal(proposerId, proposal), (int)QPI::INVALID_PROPOSAL_INDEX);
}

template <bool successExpected, typename ProposalVotingType>
void voteWithValidVoter(
    const QPI::QpiContextProcedureCall& qpi,
    ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 proposalIndex,
    QPI::uint16 proposalType,
    QPI::uint32 proposalTick,
    QPI::sint64 voteValue
)
{
    QPI::uint32 voterIdx = qpi(pv).voteIndex(voterId);
    EXPECT_NE(voterIdx, QPI::INVALID_VOTE_INDEX);
    QPI::id voterIdReturned = qpi(pv).voterId(voterIdx);
    EXPECT_EQ(voterIdReturned, voterId);

    QPI::ProposalSingleVoteDataV1 voteReturnedBefore;
    bool oldVoteAvailable = qpi(pv).getVote(proposalIndex, voterIdx, voteReturnedBefore);

    QPI::ProposalSingleVoteDataV1 vote;
    vote.proposalIndex = proposalIndex;
    vote.proposalType = proposalType;
    vote.proposalTick = proposalTick;
    vote.voteValue = voteValue;
    EXPECT_EQ(qpi(pv).vote(voterId, vote), successExpected);

    QPI::ProposalSingleVoteDataV1 voteReturned;
    if (successExpected)
    {
        EXPECT_TRUE(qpi(pv).getVote(vote.proposalIndex, voterIdx, voteReturned));
        EXPECT_TRUE(vote == voteReturned);

        typename ProposalVotingType::ProposalDataType proposalReturned;
        EXPECT_TRUE(qpi(pv).getProposal(vote.proposalIndex, proposalReturned));
        EXPECT_TRUE(proposalReturned.type == voteReturned.proposalType);
        EXPECT_TRUE(proposalReturned.tick == voteReturned.proposalTick);
    }
    else if (oldVoteAvailable)
    {
        EXPECT_TRUE(qpi(pv).getVote(vote.proposalIndex, voterIdx, voteReturned));
        EXPECT_TRUE(voteReturnedBefore == voteReturned);
    }
}

template <bool successExpected, typename ProposalVotingType>
void voteWithValidVoterMultiVote(
    const QPI::QpiContextProcedureCall& qpi,
    ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 proposalIndex,
    QPI::uint16 proposalType,
    QPI::uint32 proposalTick,
    QPI::sint64 voteValue,
    QPI::uint32 voteCount = 0, // for first voteValue, 0 means all
    QPI::sint64 voteValue2 = 0,
    QPI::uint32 voteCount2 = 0,
    QPI::sint64 voteValue3 = 0,
    QPI::uint32 voteCount3 = 0
)
{
    QPI::uint32 voterIdx = qpi(pv).voteIndex(voterId);
    EXPECT_NE(voterIdx, QPI::INVALID_VOTE_INDEX);
    QPI::id voterIdReturned = qpi(pv).voterId(voterIdx);
    EXPECT_EQ(voterIdReturned, voterId);

    QPI::ProposalMultiVoteDataV1 voteReturnedBefore;
    bool oldVoteAvailable = qpi(pv).getVotes(proposalIndex, voterId, voteReturnedBefore);

    // set all votes of voter (1 in computor voting) with multi-data voting function
    QPI::ProposalMultiVoteDataV1 vote;
    vote.proposalIndex = proposalIndex;
    vote.proposalType = proposalType;
    vote.proposalTick = proposalTick;
    vote.voteValues.setAll(0);
    vote.voteCounts.setAll(0);
    vote.voteValues.set(0, voteValue);
    if (voteCount != 0)
    {
        vote.voteCounts.set(0, voteCount);
        vote.voteValues.set(1, voteValue2);
        vote.voteCounts.set(1, voteCount2);
        vote.voteValues.set(2, voteValue3);
        vote.voteCounts.set(2, voteCount3);
    }
    EXPECT_EQ(qpi(pv).vote(voterId, vote), successExpected);

    QPI::ProposalMultiVoteDataV1 voteReturned;
    if (successExpected)
    {
        EXPECT_TRUE(qpi(pv).getVotes(vote.proposalIndex, voterId, voteReturned));
        EXPECT_EQ((int)vote.proposalIndex, (int)voteReturned.proposalIndex);
        EXPECT_EQ((int)vote.proposalType, (int)voteReturned.proposalType);
        EXPECT_EQ(vote.proposalTick, voteReturned.proposalTick);
        int totalVoteCount = qpi(pv).voteCount(voterIdx, proposalIndex);
        if (voteCount == 0)
        {
            for (int i = 0; i < voteReturned.voteCounts.capacity(); ++i)
            {
                if (voteReturned.voteValues.get(i) == voteValue)
                {
                    EXPECT_EQ(voteReturned.voteCounts.get(i), totalVoteCount);
                    totalVoteCount = 0; // for duplicates (e.g. value 0), expect count 0
                }
                else
                {
                    EXPECT_EQ(voteReturned.voteCounts.get(i), 0);
                }
            }
        }
        else
        {
            std::set<sint64> voteInputIdx = { 0, 1, 2 };
            for (int i = 0; i < voteReturned.voteCounts.capacity(); ++i)
            {
                if (voteReturned.voteCounts.get(i) != 0)
                {
                    bool found = false;
                    for (int j = 0; j < 4; ++j)
                    {
                        if (vote.voteValues.get(j) == voteReturned.voteValues.get(i))
                        {
                            EXPECT_EQ(vote.voteCounts.get(j), voteReturned.voteCounts.get(i));
                            EXPECT_TRUE(voteInputIdx.contains(j));
                            voteInputIdx.erase(j);
                            found = true;
                            break;
                        }
                    }
                    EXPECT_TRUE(found);
                }
            }
            EXPECT_TRUE(voteInputIdx.empty() || voteCount3 == 0);
        }

        typename ProposalVotingType::ProposalDataType proposalReturned;
        EXPECT_TRUE(qpi(pv).getProposal(vote.proposalIndex, proposalReturned));
        EXPECT_TRUE(proposalReturned.type == voteReturned.proposalType);
        EXPECT_TRUE(proposalReturned.tick == voteReturned.proposalTick);
    }
    else if (oldVoteAvailable)
    {
        EXPECT_TRUE(qpi(pv).getVotes(vote.proposalIndex, voterId, voteReturned));
        EXPECT_TRUE(voteReturnedBefore == voteReturned);
    }
}


template <typename ProposalVotingType>
void voteWithInvalidVoter(
    const QPI::QpiContextProcedureCall& qpi,
    ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 proposalIndex,
    QPI::uint16 proposalType,
    QPI::uint32 proposalTick,
    QPI::sint64 voteValue
)
{
    QPI::ProposalSingleVoteDataV1 vote;
    vote.proposalIndex = proposalIndex;
    vote.proposalType = proposalType;
    vote.proposalTick = proposalTick;
    vote.voteValue = voteValue;
    EXPECT_FALSE(qpi(pv).vote(voterId, vote));

    QPI::ProposalMultiVoteDataV1 vote2;
    vote2.proposalIndex = proposalIndex;
    vote2.proposalType = proposalType;
    vote2.proposalTick = proposalTick;
    vote2.voteValues.setAll(0);
    vote2.voteValues.set(0, voteValue);
    vote2.voteCounts.setAll(0);
    EXPECT_FALSE(qpi(pv).vote(voterId, vote2));
}


template <typename ProposalVotingType>
int countActiveProposals(
    const QPI::QpiContextFunctionCall& qpi,
    const ProposalVotingType& pv
)
{
    int activeProposals = 0;
    QPI::sint32 idx = -1;
    while ((idx = qpi(*pv).nextProposalIndex(idx)) >= 0)
        ++activeProposals;
    return activeProposals;
}

template <typename ProposalVotingType>
int countFinishedProposals(
    const QPI::QpiContextFunctionCall& qpi,
    const ProposalVotingType& pv
)
{
    int finishedProposals = 0;
    QPI::sint32 idx = -1;
    while ((idx = qpi(*pv).nextFinishedProposalIndex(idx)) >= 0)
        ++finishedProposals;
    return finishedProposals;
}

template <bool supportScalarVotes, bool proposalByComputorsOnly>
void testProposalVotingComputorsV1()
{
    ContractExecInitDeinitGuard initDeinitGuard;

    system.tick = 123456789;
    system.epoch = 12345;
    initComputors(0);

    typedef std::conditional<
        proposalByComputorsOnly,
        QPI::ProposalAndVotingByComputors<200>,   // Allow less proposals than NUMBER_OF_COMPUTORS to check handling of full arrays
        QPI::ProposalByAnyoneVotingByComputors<200>
    >::type ProposerAndVoterHandling;

    QpiContextUserProcedureCall qpi(0, QPI::id(1,2,3,4), 123);
    auto * pv = new QPI::ProposalVoting<
        ProposerAndVoterHandling,
        QPI::ProposalDataV1<supportScalarVotes>>;

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(*pv, 0);

    // fail: get before proposals have been set
    QPI::ProposalDataV1<supportScalarVotes> proposalReturned;
    QPI::ProposalSingleVoteDataV1 voteDataReturned;
    QPI::ProposalSummarizedVotingDataV1 votingSummaryReturned;
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        proposalReturned.type = 42; // test that additional error indicator is set 0
        EXPECT_FALSE(qpi(*pv).getProposal(i, proposalReturned));
        EXPECT_EQ((int)proposalReturned.type, 0);

        voteDataReturned.proposalType = 42; // test that additional error indicator is set 0
        EXPECT_FALSE(qpi(*pv).getVote(i, 0, voteDataReturned));
        EXPECT_EQ((int)voteDataReturned.proposalType, 0);

        votingSummaryReturned.totalVotesAuthorized = 42; // test that additional error indicator is set 0
        EXPECT_FALSE(qpi(*pv).getVotingSummary(i, votingSummaryReturned));
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, 0);
    }
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(123456), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(123456), -1);

    // fail: get with invalid proposal index
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals + 1, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getVote(pv->maxProposals, 0, voteDataReturned));
    EXPECT_FALSE(qpi(*pv).getVote(pv->maxProposals + 1, 0, voteDataReturned));
    EXPECT_FALSE(qpi(*pv).getVotingSummary(pv->maxProposals, votingSummaryReturned));
    EXPECT_FALSE(qpi(*pv).getVotingSummary(pv->maxProposals + 1, votingSummaryReturned));

    // fail: no proposals for given IDs and invalid input
    EXPECT_EQ((int)qpi(*pv).proposalIndex(QPI::NULL_ID), (int)QPI::INVALID_PROPOSAL_INDEX); // always equal
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.originator()), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(0)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(QPI::INVALID_PROPOSAL_INDEX), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(pv->maxProposals), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(0), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).proposerId(1), QPI::NULL_ID);

    // okay: voters are available independently of proposals (= computors)
    for (int i = 0; i < pv->maxVotes; ++i)
    {
        EXPECT_EQ(qpi(*pv).voteIndex(qpi.computor(i)), i);
        EXPECT_EQ(qpi(*pv).voteCount(i), 1);
        EXPECT_EQ(qpi(*pv).voterId(i), qpi.computor(i));
    }

    // fail: IDs / indices of non-voters
    EXPECT_EQ(qpi(*pv).voteIndex(qpi.originator()), QPI::INVALID_VOTE_INDEX);
    EXPECT_EQ(qpi(*pv).voteIndex(QPI::NULL_ID), QPI::INVALID_VOTE_INDEX);
    EXPECT_EQ(qpi(*pv).voteCount(QPI::INVALID_VOTE_INDEX), 0);
    EXPECT_EQ(qpi(*pv).voteCount(1000), 0);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVotes), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVotes + 1), QPI::NULL_ID);

    // okay: set proposal for computor 0
    QPI::ProposalDataV1<supportScalarVotes> proposal;
    proposal.url.set(0, 0);
    proposal.epoch = qpi.epoch();
    proposal.type = QPI::ProposalTypes::YesNo;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(0), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(0)), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote although no proposal is available at proposal index
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 1, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 1, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 12345, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 12345, QPI::ProposalTypes::YesNo, qpi.tick(), 0);

    // fail: vote with wrong type
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::TransferYesNo, qpi.tick(), 0);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::TransferYesNo, qpi.tick(), 0);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::VariableScalarMean, qpi.tick(), 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::VariableScalarMean, qpi.tick(), 0);

    // fail: vote with non-computor
    voteWithInvalidVoter(qpi, *pv, qpi.originator(), 0, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithInvalidVoter(qpi, *pv, QPI::NULL_ID, 0, QPI::ProposalTypes::YesNo, qpi.tick(), 0);

    // fail: vote with invalid value
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick(), 2);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick(), 2);

    // fail: vote with wrong tick
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick()-1, 0);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick()-1, 0);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick()+1, 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick()+1, 0);

    // okay: correct votes in proposalIndex 0
    expectNoVotes(qpi, pv, 0);
    for (int i = 0; i < pv->maxVotes; ++i)
    {
        voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), 0, QPI::ProposalTypes::YesNo, qpi.tick(), i % 2);
        voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), 0, QPI::ProposalTypes::YesNo, qpi.tick(), i % 2);
    }
    voteWithValidVoter<true>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(0), 0, QPI::ProposalTypes::YesNo, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    EXPECT_TRUE(qpi(*pv).getVotingSummary(0, votingSummaryReturned));
    EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 0);
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, pv->maxVotes - 1);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 2);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(0), pv->maxVotes / 2 - 1);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(1), pv->maxVotes / 2);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 1);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), 1);

    if (proposalByComputorsOnly)
    {
        // fail: originator id(1,2,3,4) is no computor (see custom qpi.computor() above)
        setProposalExpectFailure(qpi, pv, qpi.originator(), proposal);
    }
    else
    {
        // okay if anyone is allowed to set proposal
        setProposalWithSuccessCheck(qpi, pv, qpi.originator(), proposal);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.originator()), 1);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(1), -1);
        EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

        // clear proposal again
        EXPECT_TRUE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.originator())));
        EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.originator()), (int)QPI::INVALID_PROPOSAL_INDEX);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(0), -1);
    }

    // fail: invalid type (more options than supported)
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 9);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

    // fail: invalid type (less options than supported)
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 0);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

    // okay: set proposal for computor 2 / other ID (proposal index 1, first use)
    QPI::id secondNonComputorId(12345, 6789, 987, 654);
    QPI::id secondProposer = (proposalByComputorsOnly) ? qpi.computor(2) : secondNonComputorId;
    proposal.type = QPI::ProposalTypes::FourOptions;
    proposal.epoch = 1; // non-zero means current epoch
    setProposalWithSuccessCheck(qpi, pv, secondProposer, proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(secondProposer), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values (for yes/no only the values 0 and 1 are valid)
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), 1, proposal.type, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), 1, proposal.type, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(1), 1, proposal.type, qpi.tick(), 4);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(1), 1, proposal.type, qpi.tick(), 4);

    // fail: vote with non-computor
    voteWithInvalidVoter(qpi, *pv, qpi.originator(), 1, proposal.type, qpi.tick(), 0);
    voteWithInvalidVoter(qpi, *pv, secondNonComputorId, 1, proposal.type, qpi.tick(), 0);
    voteWithInvalidVoter(qpi, *pv, QPI::NULL_ID, 1, proposal.type, qpi.tick(), 0);

    // okay: correct votes in proposalIndex 1 (first use)
    expectNoVotes(qpi, pv, 1);
    for (int i = 0; i < pv->maxVotes; ++i)
    {
        voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), 1, proposal.type, qpi.tick(), (i < 100) ? i % 4 : 3);
        voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), 1, proposal.type, qpi.tick(), (i < 100) ? i % 4 : 3);
    }
    EXPECT_TRUE(qpi(*pv).getVotingSummary(1, votingSummaryReturned));
    EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 1);
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, pv->maxVotes);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 4);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(0), 25);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(1), 25);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(2), 25);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(3), pv->maxVotes - 75);
    for (int i = 4; i < votingSummaryReturned.optionVoteCount.capacity(); ++i)
        EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(i), 0);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 3);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), 3);

    // fail: proposal of transfer with wrong address
    proposal.type = QPI::ProposalTypes::TransferYesNo;
    proposal.transfer.destination = QPI::NULL_ID;
    proposal.transfer.amounts.setAll(0);
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);
    // check that overwrite did not work
    EXPECT_TRUE(qpi(*pv).getProposal(qpi(*pv).proposalIndex(secondProposer), proposalReturned));
    EXPECT_FALSE(isReturnedProposalAsExpected(qpi, proposalReturned, proposal));

    // fail: proposal of transfer with too many or too few options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Transfer, 0);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Transfer, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Transfer, 6);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);

    // fail: proposal of revenue distribution with invalid amount
    proposal.type = QPI::ProposalTypes::TransferYesNo;
    proposal.transfer.destination = qpi.originator();
    proposal.transfer.amounts.set(0, -123456);
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);

    // okay: revenue distribution, overwrite existing proposal of comp 2 (proposal index 1, reused)
    proposal.transfer.destination = qpi.originator();
    proposal.transfer.amounts.set(0, 1005);
    setProposalWithSuccessCheck(qpi, pv, secondProposer, proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(secondProposer), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values (for yes/no only the values 0 and 1 are valid)
    QPI::uint16 secondProposalIdx = qpi(*pv).proposalIndex(secondProposer);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), secondProposalIdx, proposal.type, qpi.tick(), -1);
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(1), secondProposalIdx, proposal.type, qpi.tick(), 2);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), secondProposalIdx, proposal.type, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(1), secondProposalIdx, proposal.type, qpi.tick(), 2);

    // okay: correct votes in proposalIndex 1 (reused)
    expectNoVotes(qpi, pv, 1); // checks that setProposal clears previous votes
    for (int i = 0; i < pv->maxVotes; ++i)
    {
        voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), secondProposalIdx, proposal.type, qpi.tick(), i % 2);
        voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), secondProposalIdx, proposal.type, qpi.tick(), i % 2);
    }
    voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(3), secondProposalIdx, proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(5), secondProposalIdx, proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    voteWithValidVoter<true>(qpi, *pv, qpi.computor(3), secondProposalIdx, proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    voteWithValidVoter<true>(qpi, *pv, qpi.computor(5), secondProposalIdx, proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    EXPECT_TRUE(qpi(*pv).getVotingSummary(1, votingSummaryReturned));
    EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 1);
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, pv->maxVotes - 2);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 2);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(0), pv->maxVotes / 2);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(1), pv->maxVotes / 2 - 2);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 0);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), 0);

    if (!supportScalarVotes)
    {
        // fail: scalar proposal not supported
        proposal.type = QPI::ProposalTypes::VariableScalarMean;
        setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);
    }
    else
    {
        // fail: scalar proposal with wrong min/max
        proposal.type = QPI::ProposalTypes::VariableScalarMean;
        proposal.variableScalar.proposedValue = 10;
        proposal.variableScalar.minValue = 11;
        proposal.variableScalar.maxValue = 20;
        proposal.variableScalar.variable = 123; // not checked, full range usable
        setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);
        proposal.variableScalar.minValue = 0;
        proposal.variableScalar.maxValue = 9;
        setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

        // fail: scalar proposal with full range is invalid, because NO_VOTE_VALUE is reserved for no vote
        proposal.variableScalar.minValue = proposal.variableScalar.minSupportedValue - 1;
        proposal.variableScalar.maxValue = proposal.variableScalar.maxSupportedValue;
        setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

        // okay: scalar proposal with nearly full range
        proposal.variableScalar.minValue = proposal.variableScalar.minSupportedValue;
        proposal.variableScalar.maxValue = proposal.variableScalar.maxSupportedValue;
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(1), proposal);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(1)), 2);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(secondProposer), (int)secondProposalIdx);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(1), 2);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(2), -1);
        EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

        // okay: votes in proposalIndex of computor 1 for testing overflow-avoiding summary algorithm for average
        expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(qpi.computor(1)));
        for (int i = 0; i < 99; ++i)
        {
            voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(1)), proposal.type, qpi.tick(), proposal.variableScalar.maxSupportedValue - 2 + i % 3);
            voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(1)), proposal.type, qpi.tick(), proposal.variableScalar.maxSupportedValue - 2 + i % 3);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(qpi(*pv).proposalIndex(qpi.computor(1)), votingSummaryReturned));
        EXPECT_EQ((int)votingSummaryReturned.proposalIndex, (int)qpi(*pv).proposalIndex(qpi.computor(1)));
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 99);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), -1);
        EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), -1);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, proposal.variableScalar.maxSupportedValue - 1);
        for (int i = 0; i < 555; ++i)
        {
            voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(1)), proposal.type, qpi.tick(), proposal.variableScalar.minSupportedValue + 10 - i % 5);
            voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(1)), proposal.type, qpi.tick(), proposal.variableScalar.minSupportedValue + 10 - i % 5);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(qpi(*pv).proposalIndex(qpi.computor(1)), votingSummaryReturned));
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 555);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, proposal.variableScalar.minSupportedValue + 8);

        // okay: scalar proposal with limited range
        proposal.variableScalar.minValue = -1000;
        proposal.variableScalar.maxValue = 1000;
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(10), proposal);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(10)), 3);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(1), 2);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(2), 3);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(3), -1);
        EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

        // fail: vote with invalid values
        voteWithValidVoter<false>(qpi, *pv, qpi.computor(0), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), -1001);
        voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(0), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), -1001);
        voteWithValidVoterMultiVote<false>(qpi, *pv, qpi.computor(1), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 1001);
        voteWithValidVoter<false>(qpi, *pv, qpi.computor(1), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 1001);

        // okay: correct votes in proposalIndex of computor 10
        expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(qpi.computor(10)));
        for (int i = 0; i < 603; ++i)
        {
            voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), (i % 201) - 100);
            voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), (i % 201) - 100);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(3, votingSummaryReturned));
        EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 3);
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 603);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, 0);
        EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), -1);
        EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), -1);

        // another case for scalar voting summary
        for (int i = 0; i < 603; ++i)
        {
            voteWithValidVoterMultiVote <true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
            voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
        }
        for (int i = 0; i < 200; ++i)
        {
            voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), i + 1);
            voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), i + 1);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(3, votingSummaryReturned));
        EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 3);
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 200);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, (200 * 201 / 2) / 200);
    }

    // fail: test multi-option transfer proposal with invalid amounts
    proposal.type = QPI::ProposalTypes::TransferThreeAmounts;
    proposal.transfer.destination = qpi.originator();
    for (int i = 0; i < 4; ++i)
    {
        proposal.transfer.amounts.setAll(0);
        proposal.transfer.amounts.set(i, -100 * i - 1);
        setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);
    }
    proposal.transfer.amounts.set(0, 0);
    proposal.transfer.amounts.set(1, 10);
    proposal.transfer.amounts.set(2, 20);
    proposal.transfer.amounts.set(3, 100); // for ProposalTypes::TransferThreeAmounts, fourth must be 0
    setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

    // fail: duplicate options
    proposal.transfer.amounts.setAll(0);
    setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

    // fail: options not sorted
    for (int i = 0; i < 3; ++i)
        proposal.transfer.amounts.set(i, 100 - i);
    setProposalExpectFailure(qpi, pv, qpi.computor(1), proposal);

    // okay: fill proposal storage
    proposal.transfer.amounts.setAll(0);
    constexpr QPI::uint16 computorProposalToFillAll = (proposalByComputorsOnly) ? pv->maxProposals : pv->maxProposals - 1;
    for (int i = 0; i < computorProposalToFillAll; ++i)
    {
        proposal.transfer.amounts.set(0, i);
        proposal.transfer.amounts.set(1, i * 2 + 1);
        proposal.transfer.amounts.set(2, i * 3 + 2);
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    }
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: no space left
    setProposalExpectFailure(qpi, pv, qpi.computor(pv->maxProposals), proposal);

    // cast some votes before epoch change to test querying voting summary afterwards
    for (int i = 0; i < 20; ++i)
        voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 0);
    for (int i = 20; i < 60; ++i)
        voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 1);
    for (int i = 60; i < 160; ++i)
        voteWithValidVoter<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 2);
    for (int i = 160; i < 360; ++i)
        voteWithValidVoterMultiVote<true>(qpi, *pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 3);

    // simulate epoch change
    ++system.epoch;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals);

    // okay: same setProposal after epoch change, because the oldest proposal will be deleted
    proposal.epoch = qpi.epoch();
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(pv->maxProposals), proposal);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 1);

    // fail: vote in wrong epoch
    voteWithValidVoter<false>(qpi, *pv, qpi.computor(123), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, qpi.tick(), 0);

    // okay: query voting summary of other epoch
    EXPECT_TRUE(qpi(*pv).getVotingSummary(qpi(*pv).proposalIndex(qpi.computor(10)), votingSummaryReturned));
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 20+40+100+200);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 4);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(0), 20);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(1), 40);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(2), 100);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(3), 200);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(4), 0);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 3);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), -1);

    // manually clear some proposals
    EXPECT_FALSE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.originator())));
    EXPECT_TRUE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.computor(5))));
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(5)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);
    proposal.epoch = 0;
    setProposalExpectFailure(qpi, pv, qpi.originator(), proposal);
    EXPECT_NE((int)qpi(*pv).setProposal(qpi.computor(7), proposal), (int)QPI::INVALID_PROPOSAL_INDEX); // success
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(7)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 3);

    // simulate epoch change with changes in computors
    ++system.epoch;
    initComputors(100);
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);

    // set new proposals, adding new computor IDs (simulated next epoch)
    proposal.epoch = qpi.epoch();
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 6);
    for (int i = 0; i < pv->maxProposals; ++i)
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    for (int i = 0; i < pv->maxProposals; ++i)
        expectNoVotes(qpi, pv, i);
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(countFinishedProposals(qpi, pv), 0);

    delete pv;
}

TEST(TestCoreQPI, ProposalVotingV1proposalOnlyByComputorWithScalarVoteSupport)
{
    testProposalVotingComputorsV1<true, false>();
}

TEST(TestCoreQPI, ProposalVotingV1proposalOnlyByComputorWithoutScalarVoteSupport)
{
    testProposalVotingComputorsV1<false, false>();
}

TEST(TestCoreQPI, ProposalVotingV1proposalByAnyoneWithScalarVoteSupport)
{
    testProposalVotingComputorsV1<true, true>();
}

TEST(TestCoreQPI, ProposalVotingV1proposalByAnyoneWithoutScalarVoteSupport)
{
    testProposalVotingComputorsV1<false, true>();
}

// TODO: ProposalVoting YesNo

template <bool supportScalarVotes>
void testProposalVotingShareholdersV1()
{
    ContractTesting test;
    test.initEmptyUniverse();

    system.tick = 123456789;
    system.epoch = 12345;
    initComputors(0);

    typedef QPI::ProposalAndVotingByShareholders<6, MSVAULT_ASSET_NAME> ProposerAndVoterHandling;

    QpiContextUserProcedureCall qpi(0, QPI::id(1, 2, 3, 4), 123);
    auto* pv = new QPI::ProposalVoting<
        ProposerAndVoterHandling,
        QPI::ProposalDataV1<supportScalarVotes>>;

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(*pv, 0);

    std::vector<std::pair<id, unsigned int>> shareholderShares{
        {id(100, 20, 3, 4), 200},
        {id(0, 0, 0, 3), 100},
        {id(0, 0, 0, 2), 250},
        {id(0, 0, 0, 1), 50},
        {id(10, 20, 3, 4), 10},
        {id(10, 20, 2, 1), 54},
        {id(10, 20, 3, 1), 12},
    };
    issueContractShares(MSVAULT_CONTRACT_INDEX, shareholderShares);
    sortContractShareVector(shareholderShares);

    // fail: get before proposals have been set
    QPI::ProposalDataV1<supportScalarVotes> proposalReturned;
    QPI::ProposalMultiVoteDataV1 voteDataReturned;
    QPI::ProposalSummarizedVotingDataV1 votingSummaryReturned;
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        proposalReturned.type = 42; // test that additional error indicator is set 0
        EXPECT_FALSE(qpi(*pv).getProposal(i, proposalReturned));
        EXPECT_EQ((int)proposalReturned.type, 0);

        voteDataReturned.proposalType = 42; // test that additional error indicator is set 0
        EXPECT_FALSE(qpi(*pv).getVotes(i, shareholderShares[0].first, voteDataReturned));
        EXPECT_EQ((int)voteDataReturned.proposalType, 0);

        votingSummaryReturned.totalVotesAuthorized = 42; // test that additional error indicator is set 0
        EXPECT_FALSE(qpi(*pv).getVotingSummary(i, votingSummaryReturned));
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, 0);
    }
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(123456), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(123456), -1);

    // fail: get with invalid proposal index
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals + 1, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getVotes(pv->maxProposals, shareholderShares[0].first, voteDataReturned));
    EXPECT_FALSE(qpi(*pv).getVotes(pv->maxProposals + 1, shareholderShares[0].first, voteDataReturned));
    EXPECT_FALSE(qpi(*pv).getVotingSummary(pv->maxProposals, votingSummaryReturned));
    EXPECT_FALSE(qpi(*pv).getVotingSummary(pv->maxProposals + 1, votingSummaryReturned));

    // fail: no proposals for given IDs and invalid input
    EXPECT_EQ((int)qpi(*pv).proposalIndex(QPI::NULL_ID), (int)QPI::INVALID_PROPOSAL_INDEX); // always equal
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.originator()), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(shareholderShares[0].first), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(QPI::INVALID_PROPOSAL_INDEX), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(pv->maxProposals), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(0), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).proposerId(1), QPI::NULL_ID);

    // fail: IDs / indices of non-voters
    EXPECT_EQ(qpi(*pv).voteIndex(qpi.originator()), QPI::INVALID_VOTE_INDEX);
    EXPECT_EQ(qpi(*pv).voteIndex(QPI::NULL_ID), QPI::INVALID_VOTE_INDEX);
    EXPECT_EQ(qpi(*pv).voteCount(QPI::INVALID_VOTE_INDEX), 0);
    EXPECT_EQ(qpi(*pv).voteCount(1000), 0);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVotes), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVotes + 1), QPI::NULL_ID);

    // okay: set proposal for shareholder 0
    QPI::ProposalDataV1<supportScalarVotes> proposal;
    proposal.url.set(0, 0);
    proposal.epoch = qpi.epoch();
    proposal.type = QPI::ProposalTypes::YesNo;
    setProposalWithSuccessCheck(qpi, pv, shareholderShares[0].first, proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(shareholderShares[0].first), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // check that voters match shareholders
    for (size_t i = 0; i < shareholderShares.size(); ++i)
    {
        uint32 voterIdx = qpi(*pv).voteIndex(shareholderShares[i].first);
        EXPECT_NE(voterIdx, NO_VOTE_VALUE);
        EXPECT_EQ(qpi(*pv).voteCount(voterIdx), shareholderShares[i].second);
        EXPECT_EQ(qpi(*pv).voterId(voterIdx), shareholderShares[i].first);
    }

    // fail: vote although no proposal is available at proposal index
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 1, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 1, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 12345, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 12345, QPI::ProposalTypes::YesNo, qpi.tick(), 0);

    // fail: vote with wrong type
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::TransferYesNo, qpi.tick(), 0);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::TransferYesNo, qpi.tick(), 0);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::VariableScalarMean, qpi.tick(), 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::VariableScalarMean, qpi.tick(), 0);

    // fail: vote with non-computor
    voteWithInvalidVoter(qpi, *pv, qpi.originator(), 0, QPI::ProposalTypes::YesNo, qpi.tick(), 0);
    voteWithInvalidVoter(qpi, *pv, QPI::NULL_ID, 0, QPI::ProposalTypes::YesNo, qpi.tick(), 0);

    // fail: vote with invalid value
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), 2);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), 2);

    // fail: vote with wrong tick
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick() - 1, 0);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick() - 1, 0);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick() + 1, 0);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick() + 1, 0);

    // okay: correct votes in proposalIndex 0
    expectNoVotes(qpi, pv, 0);
    int optionProposalVoteCounts[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    for (size_t i = 0; i < shareholderShares.size(); ++i)
    {
        if (i % 3 != 0)
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), i % 2);
        else
            voteWithValidVoter<true>(qpi, *pv, shareholderShares[i].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), i % 2);
        optionProposalVoteCounts[i % 2] += shareholderShares[i].second;
    }
    voteWithValidVoter<true>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[0].first, 0, QPI::ProposalTypes::YesNo, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    EXPECT_TRUE(qpi(*pv).getVotingSummary(0, votingSummaryReturned));
    EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 0);
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, pv->maxVotes - shareholderShares[0].second);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 2);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(0), optionProposalVoteCounts[0] - shareholderShares[0].second);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(1), optionProposalVoteCounts[1]);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 1);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), 1);

    // fail: originator id(1,2,3,4) is no shareholder (see custom qpi.computor() above)
    setProposalExpectFailure(qpi, pv, qpi.originator(), proposal);

    // fail: invalid type (more options than supported)
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 9);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

    // fail: invalid type (less options than supported)
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 0);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

    // okay: set proposal for computor 2 (proposal index 1, first use)
    QPI::id secondProposer = shareholderShares[2].first;
    proposal.type = QPI::ProposalTypes::FourOptions;
    proposal.epoch = 1; // non-zero means current epoch
    setProposalWithSuccessCheck(qpi, pv, secondProposer, proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(secondProposer), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values (for yes/no only the values 0 and 1 are valid)
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, 1, proposal.type, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, 1, proposal.type, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[1].first, 1, proposal.type, qpi.tick(), 4);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[1].first, 1, proposal.type, qpi.tick(), 4);

    // fail: vote with non-shareholder
    voteWithInvalidVoter(qpi, *pv, qpi.originator(), 1, proposal.type, qpi.tick(), 0);
    voteWithInvalidVoter(qpi, *pv, QPI::NULL_ID, 1, proposal.type, qpi.tick(), 0);

    // okay: cast votes in proposalIndex 1 (first use)
    expectNoVotes(qpi, pv, 1);
    for (int i = 0; i < 8; ++i)
        optionProposalVoteCounts[i] = 0;
    for (size_t i = 0; i < shareholderShares.size(); ++i)
    {
        int voteValue = i % 4;
        optionProposalVoteCounts[voteValue] += shareholderShares[i].second;
        if (i & 1)
            voteWithValidVoter<true>(qpi, *pv, shareholderShares[i].first, 1, proposal.type, qpi.tick(), voteValue);
        else
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, 1, proposal.type, qpi.tick(), voteValue);
    }
    EXPECT_TRUE(qpi(*pv).getVotingSummary(1, votingSummaryReturned));
    EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 1);
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, pv->maxVotes);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 4);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(i), optionProposalVoteCounts[i]);
    for (int i = 4; i < votingSummaryReturned.optionVoteCount.capacity(); ++i)
        EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(i), 0);

    // fail: proposal of transfer with wrong address
    proposal.type = QPI::ProposalTypes::TransferYesNo;
    proposal.transfer.destination = QPI::NULL_ID;
    proposal.transfer.amounts.setAll(0);
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);
    // check that overwrite did not work
    EXPECT_TRUE(qpi(*pv).getProposal(qpi(*pv).proposalIndex(secondProposer), proposalReturned));
    EXPECT_FALSE(isReturnedProposalAsExpected(qpi, proposalReturned, proposal));

    // fail: proposal of transfer with too many or too few options
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Transfer, 0);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Transfer, 1);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::Transfer, 6);
    EXPECT_FALSE(QPI::ProposalTypes::isValid(proposal.type));
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);

    // fail: proposal of revenue distribution with invalid amount
    proposal.type = QPI::ProposalTypes::TransferYesNo;
    proposal.transfer.destination = qpi.originator();
    proposal.transfer.amounts.set(0, -123456);
    setProposalExpectFailure(qpi, pv, secondProposer, proposal);

    // okay: revenue distribution, overwrite existing proposal of comp 2 (proposal index 1, reused)
    proposal.transfer.destination = qpi.originator();
    proposal.transfer.amounts.set(0, 1005);
    setProposalWithSuccessCheck(qpi, pv, secondProposer, proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(secondProposer), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values (for yes/no only the values 0 and 1 are valid)
    QPI::uint16 secondProposalIdx = qpi(*pv).proposalIndex(secondProposer);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, secondProposalIdx, proposal.type, qpi.tick(), -1);
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[1].first, secondProposalIdx, proposal.type, qpi.tick(), 2);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, secondProposalIdx, proposal.type, qpi.tick(), -1);
    voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[1].first, secondProposalIdx, proposal.type, qpi.tick(), 2);

    // okay: cast votes in proposalIndex 1 (reused)
    expectNoVotes(qpi, pv, 1); // checks that setProposal clears previous votes
    optionProposalVoteCounts[0] = 0;
    optionProposalVoteCounts[1] = 0;
    for (size_t i = 0; i < shareholderShares.size(); ++i)
    {
        int voteValue = i % 2;
        optionProposalVoteCounts[voteValue] += shareholderShares[i].second;
        if (i & 1)
            voteWithValidVoter<true>(qpi, *pv, shareholderShares[i].first, secondProposalIdx, proposal.type, qpi.tick(), voteValue);
        else
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, secondProposalIdx, proposal.type, qpi.tick(), voteValue);
    }
    voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[3].first, secondProposalIdx, proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    optionProposalVoteCounts[1] -= shareholderShares[3].second;
    voteWithValidVoter<true>(qpi, *pv, shareholderShares[5].first, secondProposalIdx, proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
    optionProposalVoteCounts[1] -= shareholderShares[5].second;
    EXPECT_TRUE(qpi(*pv).getVotingSummary(1, votingSummaryReturned));
    EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 1);
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, optionProposalVoteCounts[0] + optionProposalVoteCounts[1]);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 2);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(0), optionProposalVoteCounts[0]);
    EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(1), optionProposalVoteCounts[1]);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 0);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), 0);

    if (!supportScalarVotes)
    {
        // fail: scalar proposal not supported
        proposal.type = QPI::ProposalTypes::VariableScalarMean;
        setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);
    }
    else
    {
        // fail: scalar proposal with wrong min/max
        proposal.type = QPI::ProposalTypes::VariableScalarMean;
        proposal.variableScalar.proposedValue = 10;
        proposal.variableScalar.minValue = 11;
        proposal.variableScalar.maxValue = 20;
        proposal.variableScalar.variable = 123; // not checked, full range usable
        setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);
        proposal.variableScalar.minValue = 0;
        proposal.variableScalar.maxValue = 9;
        setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

        // fail: scalar proposal with full range is invalid, because NO_VOTE_VALUE is reserved for no vote
        proposal.variableScalar.minValue = proposal.variableScalar.minSupportedValue - 1;
        proposal.variableScalar.maxValue = proposal.variableScalar.maxSupportedValue;
        setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

        // okay: scalar proposal with nearly full range
        proposal.variableScalar.minValue = proposal.variableScalar.minSupportedValue;
        proposal.variableScalar.maxValue = proposal.variableScalar.maxSupportedValue;
        setProposalWithSuccessCheck(qpi, pv, shareholderShares[1].first, proposal);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(shareholderShares[1].first), 2);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(secondProposer), (int)secondProposalIdx);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(1), 2);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(2), -1);
        EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

        // okay: votes in proposalIndex for testing overflow-avoiding summary algorithm for average
        expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(shareholderShares[1].first));
        for (int i = 0; i < 3; ++i)
        {
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, qpi(*pv).proposalIndex(shareholderShares[1].first),
                proposal.type, qpi.tick(),
                proposal.variableScalar.maxSupportedValue - 2 + i % 3, 5,
                proposal.variableScalar.maxSupportedValue - 2 + (i + 1) % 3, 3,
                proposal.variableScalar.maxSupportedValue - 2 + (i + 2) % 3, 2);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(qpi(*pv).proposalIndex(shareholderShares[1].first), votingSummaryReturned));
        EXPECT_EQ((int)votingSummaryReturned.proposalIndex, (int)qpi(*pv).proposalIndex(shareholderShares[1].first));
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 30);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, proposal.variableScalar.maxSupportedValue - 1);
        EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), -1);
        EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), -1);

        for (int i = 0; i < 5; ++i)
        {
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, qpi(*pv).proposalIndex(shareholderShares[1].first),
                proposal.type, qpi.tick(),
                proposal.variableScalar.minSupportedValue + 4 - i % 5, 4,
                proposal.variableScalar.minSupportedValue + 12 - i % 5, 2);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(qpi(*pv).proposalIndex(shareholderShares[1].first), votingSummaryReturned));
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 30);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, proposal.variableScalar.minSupportedValue + 2 + 3);

        // okay: scalar proposal with limited range
        proposal.variableScalar.minValue = -1000;
        proposal.variableScalar.maxValue = 1000;
        setProposalWithSuccessCheck(qpi, pv, shareholderShares[5].first, proposal);
        EXPECT_EQ((int)qpi(*pv).proposalIndex(shareholderShares[5].first), 3);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(-1), 0);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(0), 1);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(1), 2);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(2), 3);
        EXPECT_EQ(qpi(*pv).nextProposalIndex(3), -1);
        EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

        // fail: vote with invalid values
        voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(), -1001);
        voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[0].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(), -1001);
        voteWithValidVoterMultiVote<false>(qpi, *pv, shareholderShares[1].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(), 1001);
        voteWithValidVoter<false>(qpi, *pv, shareholderShares[1].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(), 1001);

        // okay: cast votes in proposalIndex of shareholder 5
        expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(shareholderShares[5].first));
        for (int i = 0; i < (int)shareholderShares.size(); ++i)
        {
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, qpi(*pv).proposalIndex(shareholderShares[5].first),
                proposal.type, qpi.tick(),
                (i + 1) * 5, 6,
                (i + 1) * -10, 3,
                0, shareholderShares[i].second - 9);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(3, votingSummaryReturned));
        EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 3);
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, 676);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, 0);

        // another case for scalar voting summary
        for (size_t i = 0; i < shareholderShares.size(); ++i)
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(), QPI::NO_VOTE_VALUE); // remove vote
        expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(shareholderShares[5].first));
        for (size_t i = 0; i < shareholderShares.size(); ++i)
        {
            voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, qpi(*pv).proposalIndex(shareholderShares[5].first),
                proposal.type, qpi.tick(),
                i * 3, 2,
                i * 3 + 1, 3,
                i * 3 + 2, 2);
        }
        EXPECT_TRUE(qpi(*pv).getVotingSummary(3, votingSummaryReturned));
        EXPECT_EQ((int)votingSummaryReturned.proposalIndex, 3);
        EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
        EXPECT_EQ(votingSummaryReturned.totalVotesCasted, shareholderShares.size() * 7);
        EXPECT_EQ((int)votingSummaryReturned.optionCount, 0);
        EXPECT_EQ(votingSummaryReturned.scalarVotingResult, (shareholderShares.size() / 2) * 3 + 1);
    }

    // fail: test multi-option transfer proposal with invalid amounts
    proposal.type = QPI::ProposalTypes::TransferThreeAmounts;
    proposal.transfer.destination = qpi.originator();
    for (int i = 0; i < 4; ++i)
    {
        proposal.transfer.amounts.setAll(0);
        proposal.transfer.amounts.set(i, -100 * i - 1);
        setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);
    }
    proposal.transfer.amounts.set(0, 0);
    proposal.transfer.amounts.set(1, 10);
    proposal.transfer.amounts.set(2, 20);
    proposal.transfer.amounts.set(3, 100); // for ProposalTypes::TransferThreeAmounts, fourth must be 0
    setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

    // fail: duplicate options
    proposal.transfer.amounts.setAll(0);
    setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

    // fail: options not sorted
    for (int i = 0; i < 3; ++i)
        proposal.transfer.amounts.set(i, 100 - i);
    setProposalExpectFailure(qpi, pv, shareholderShares[1].first, proposal);

    // okay: fill proposal storage
    proposal.transfer.amounts.setAll(0);
    ASSERT_EQ((int)pv->maxProposals, (int)shareholderShares.size() - 1);
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        proposal.transfer.amounts.set(0, i);
        proposal.transfer.amounts.set(1, i * 2 + 1);
        proposal.transfer.amounts.set(2, i * 3 + 2);
        setProposalWithSuccessCheck(qpi, pv, shareholderShares[i].first, proposal);
    }
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: no space left
    setProposalExpectFailure(qpi, pv, shareholderShares[pv->maxProposals].first, proposal);

    // cast some votes before epoch change to test querying voting summary afterwards
    for (int i = 0; i < 8; ++i)
        optionProposalVoteCounts[i] = 0;
    for (size_t i = 0; i < shareholderShares.size(); ++i)
    {
        voteWithValidVoterMultiVote<true>(qpi, *pv, shareholderShares[i].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(),
            i & 1, shareholderShares[i].second / 2,
            2, shareholderShares[i].second / 8,
            3, shareholderShares[i].second / 4);
        optionProposalVoteCounts[i & 1] += shareholderShares[i].second / 2;
        optionProposalVoteCounts[2] += shareholderShares[i].second / 8;
        optionProposalVoteCounts[3] += shareholderShares[i].second / 4;
    }
    
    // simulate epoch change
    ++system.epoch;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals);

    // okay: same setProposal after epoch change, because the oldest proposal will be deleted
    proposal.epoch = qpi.epoch();
    setProposalWithSuccessCheck(qpi, pv, shareholderShares[pv->maxProposals].first, proposal);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 1);

    // fail: vote in wrong epoch
    voteWithValidVoter<false>(qpi, *pv, shareholderShares[0].first, qpi(*pv).proposalIndex(shareholderShares[5].first), proposal.type, qpi.tick(), 0);

    // okay: query voting summary of other epoch
    EXPECT_TRUE(qpi(*pv).getVotingSummary(qpi(*pv).proposalIndex(shareholderShares[5].first), votingSummaryReturned));
    EXPECT_EQ(votingSummaryReturned.totalVotesAuthorized, pv->maxVotes);
    uint32 voteCountSum = 0;
    for (int i = 0; i < 4; ++i)
        voteCountSum += optionProposalVoteCounts[i];
    EXPECT_EQ(votingSummaryReturned.totalVotesCasted, voteCountSum);
    EXPECT_EQ((int)votingSummaryReturned.optionCount, 4);
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(votingSummaryReturned.optionVoteCount.get(i), (i < 4) ? optionProposalVoteCounts[i] : 0);
    EXPECT_EQ(votingSummaryReturned.getMostVotedOption(), 0);
    EXPECT_EQ(votingSummaryReturned.getAcceptedOption(), -1);

    // manually clear some proposals
    EXPECT_FALSE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.originator())));
    EXPECT_TRUE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(shareholderShares[5].first)));
    EXPECT_EQ((int)qpi(*pv).proposalIndex(shareholderShares[5].first), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);
    proposal.epoch = 0;
    setProposalExpectFailure(qpi, pv, qpi.originator(), proposal);
    EXPECT_NE((int)qpi(*pv).setProposal(shareholderShares[3].first, proposal), (int)QPI::INVALID_PROPOSAL_INDEX); // success (clear by epoch 0)
    EXPECT_EQ((int)qpi(*pv).proposalIndex(shareholderShares[3].first), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 3);

    // simulate epoch change
    ++system.epoch;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);

    // change of shareholders
    AssetPossessionIterator iter({ NULL_ID, MSVAULT_ASSET_NAME });
    int newPossessorCount = 0;
    while (!iter.reachedEnd())
    {
        if (iter.numberOfPossessedShares())
        {
            int destinationOwnershipIndex, destinationPossessionIndex;
            EXPECT_TRUE(transferShareOwnershipAndPossession(iter.ownershipIndex(), iter.possessionIndex(), qpi.computor(newPossessorCount),
                iter.numberOfPossessedShares(), &destinationOwnershipIndex, &destinationPossessionIndex, true));
            ++newPossessorCount;
        }
        iter.next();
    }
    EXPECT_GE(newPossessorCount, (int)pv->maxProposals);

    // set new proposals by new shareholders
    proposal.epoch = qpi.epoch();
    proposal.type = QPI::ProposalTypes::type(QPI::ProposalTypes::Class::GeneralOptions, 6);
    for (int i = 0; i < pv->maxProposals; ++i)
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    for (int i = 0; i < pv->maxProposals; ++i)
        expectNoVotes(qpi, pv, i);
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(countFinishedProposals(qpi, pv), 0);

    delete pv;
}

TEST(TestCoreQPI, ProposalVotingV1shareholderWithScalarVoteSupport)
{
    testProposalVotingShareholdersV1<true>();
}

TEST(TestCoreQPI, ProposalVotingV1shareholderWithoutScalarVoteSupport)
{
    testProposalVotingShareholdersV1<false>();
}
