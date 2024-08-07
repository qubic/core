#define NO_UEFI

#include "gtest/gtest.h"

namespace QPI
{
    struct QpiContextProcedureCall;
    struct QpiContextFunctionCall;
}
typedef void (*USER_FUNCTION)(const QPI::QpiContextFunctionCall&, void* state, void* input, void* output, void* locals);
typedef void (*USER_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);

#include "../src/contracts/qpi.h"
#include "../src/contract_core/qpi_collection_impl.h"
#include "../src/contract_core/qpi_proposal_voting.h"

struct QpiContextUserProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextUserProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QPI::QpiContextProcedureCall(contractIndex, originator, invocationReward)
    {
    }
};

// changing offset simulates changed computor set with changed epoch
static int computorIdOffset = 0;

QPI::id QPI::QpiContextFunctionCall::computor(unsigned short computorIndex) const
{
    return QPI::id(computorIndex + computorIdOffset, 9, 8, 7);
}

QPI::uint16 currentEpoch = 12345;

QPI::uint16 QPI::QpiContextFunctionCall::epoch() const
{
    return currentEpoch;
}




TEST(TestCoreQPI, Array)
{
    //QPI::array<int, 0> mustFail; // should raise compile error

    QPI::array<QPI::uint8, 4> uint8_4;
    EXPECT_EQ(uint8_4.capacity(), 4);
    //uint8_4.setMem(QPI::id(1, 2, 3, 4)); // should raise compile error
    uint8_4.setAll(2);
    EXPECT_EQ(uint8_4.get(0), 2);
    EXPECT_EQ(uint8_4.get(1), 2);
    EXPECT_EQ(uint8_4.get(2), 2);
    EXPECT_EQ(uint8_4.get(3), 2);
    for (int i = 0; i < uint8_4.capacity(); ++i)
        uint8_4.set(i, i+1);
    for (int i = 0; i < uint8_4.capacity(); ++i)
        EXPECT_EQ(uint8_4.get(i), i+1);

    QPI::array<QPI::uint64, 4> uint64_4;
    uint64_4.setMem(QPI::id(101, 102, 103, 104));
    for (int i = 0; i < uint64_4.capacity(); ++i)
        EXPECT_EQ(uint64_4.get(i), i + 101);
    //uint64_4.setMem(uint8_4); // should raise compile error

    QPI::array<QPI::uint16, 2> uint16_2;
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
    //QPI::bit_array<0> mustFail;

    QPI::bit_array<1> b1;
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

    QPI::bit_array<64> b64;
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
    QPI::array<QPI::uint64, 1> llu1;
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0x11llu);
    b64.setAll(0);
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0x0);
    b64.setAll(1);
    llu1.setMem(b64);
    EXPECT_EQ(llu1.get(0), 0xffffffffffffffffllu);

    //QPI::bit_array<96> b96; // must trigger compile error

    QPI::bit_array<128> b128;
    EXPECT_EQ(b128.capacity(), 128);
    QPI::array<QPI::uint64, 2> llu2;
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

TEST(TestCoreQPI, ProposalAndVotingByComputors)
{
    QpiContextUserProcedureCall qpi(0, QPI::id(1, 2, 3, 4), 123);
    QPI::ProposalAndVotingByComputors pv;

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(pv, 0);

    // voter index is computor index
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        EXPECT_EQ(pv.getVoterIndex(qpi, qpi.computor(i)), i);
        EXPECT_EQ(pv.getVoterId(qpi, i), qpi.computor(i));
    }
    for (int i = NUMBER_OF_COMPUTORS; i < 800; ++i)
    {
        EXPECT_EQ(pv.getVoterIndex(qpi, qpi.computor(i)), QPI::INVALID_VOTER_INDEX);
        EXPECT_EQ(pv.getVoterId(qpi, i), QPI::NULL_ID);
    }
    EXPECT_EQ(pv.getVoterIndex(qpi, qpi.originator()), QPI::INVALID_VOTER_INDEX);

    // valid proposers are computors
    for (int i = 0; i < 2 * NUMBER_OF_COMPUTORS; ++i)
        EXPECT_EQ(pv.isValidProposer(qpi, qpi.computor(i)), (i < NUMBER_OF_COMPUTORS));
    EXPECT_FALSE(pv.isValidProposer(qpi, QPI::NULL_ID));
    EXPECT_FALSE(pv.isValidProposer(qpi, qpi.originator()));

    // no existing proposals
    for (int i = 0; i < 2*NUMBER_OF_COMPUTORS; ++i)
        EXPECT_EQ((int)pv.getExistingProposalIndex(qpi, qpi.computor(i)), (int)QPI::INVALID_PROPOSAL_INDEX);

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
    }

    // using other ID fails if full (new computor ID after epoch change)
    QPI::id newId(9, 8, 7, 6);
    EXPECT_EQ((int)pv.getNewProposalIndex(qpi, newId), (int)QPI::INVALID_PROPOSAL_INDEX);

    // reuseing all slots should work
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

bool operator==(const QPI::ProposalData& p1, const QPI::ProposalData& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}

bool operator==(const QPI::SingleProposalVoteData& p1, const QPI::SingleProposalVoteData& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}

template <typename ProposalVotingType>
void setProposalWithSuccessCheck(const QpiContextUserProcedureCall& qpi, const ProposalVotingType& pv, const QPI::id& proposerId, const QPI::ProposalData& proposal)
{
    QPI::ProposalData proposalReturned;
    EXPECT_TRUE(qpi(*pv).setProposal(proposerId, proposal));
    QPI::uint16 proposalIdx = qpi(*pv).proposalIndex(proposerId);
    EXPECT_NE((int)proposalIdx, (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(proposalIdx), proposerId);
    EXPECT_TRUE(qpi(*pv).getProposal(proposalIdx, proposalReturned));
    EXPECT_TRUE(proposal == proposalReturned);
    expectNoVotes(qpi, pv, proposalIdx);
}

template <bool successExpected, typename ProposalVotingType>
void voteWithValidVoter(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 voteProposalIndex,
    QPI::uint16 voteProposalType,
    QPI::sint64 voteValue
)
{
    QPI::uint32 voterIdx = qpi(*pv).voterIndex(voterId);
    EXPECT_NE(voterIdx, QPI::INVALID_VOTER_INDEX);
    QPI::id voterIdReturned = qpi(*pv).voterId(voterIdx);
    EXPECT_EQ(voterIdReturned, voterId);

    QPI::SingleProposalVoteData voteReturnedBefore;
    bool oldVoteAvailable = qpi(*pv).getVote(voteProposalIndex, voterIdx, voteReturnedBefore);

    QPI::SingleProposalVoteData vote;
    vote.proposalIndex = voteProposalIndex;
    vote.proposalType = voteProposalType;
    vote.voteValue = voteValue;
    EXPECT_EQ(qpi(*pv).vote(voterId, vote), successExpected);

    QPI::SingleProposalVoteData voteReturned;
    if (successExpected)
    {
        if (voteProposalType == 0)
        {
            if (vote.voteValue != 0 && vote.voteValue != QPI::NO_VOTE_VALUE)
                vote.voteValue = 1;
        }

        EXPECT_TRUE(qpi(*pv).getVote(vote.proposalIndex, voterIdx, voteReturned));
        EXPECT_TRUE(vote == voteReturned);

        QPI::ProposalData proposalReturned;
        EXPECT_TRUE(qpi(*pv).getProposal(vote.proposalIndex, proposalReturned));
        EXPECT_TRUE(proposalReturned.type == voteReturned.proposalType);
    }
    else if (oldVoteAvailable)
    {
        EXPECT_TRUE(qpi(*pv).getVote(vote.proposalIndex, voterIdx, voteReturned));
        EXPECT_TRUE(voteReturnedBefore == voteReturned);
    }
}

template <typename ProposalVotingType>
void voteWithInvalidVoter(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv,
    const QPI::id& voterId,
    QPI::uint16 voteProposalIndex,
    QPI::uint16 voteProposalType,
    QPI::sint64 voteValue
)
{
    QPI::SingleProposalVoteData vote;
    vote.proposalIndex = voteProposalIndex;
    vote.proposalType = voteProposalType;
    vote.voteValue = voteValue;
    EXPECT_FALSE(qpi(*pv).vote(voterId, vote));
}

template <typename ProposalVotingType>
void expectNoVotes(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv,
    QPI::uint16 proposalIndex
)
{
    QPI::SingleProposalVoteData vote;
    for (QPI::uint32 i = 0; i < pv->maxVoters; ++i)
    {
        EXPECT_TRUE(qpi(*pv).getVote(proposalIndex, i, vote));
        EXPECT_EQ(vote.voteValue, QPI::NO_VOTE_VALUE);
    }
}

template <typename ProposalVotingType>
int countActiveProposals(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv
)
{
    int activeProposals = 0;
    QPI::sint32 idx = -1;
    while ((idx = qpi(*pv).nextActiveProposalIndex(idx)) >= 0)
        ++activeProposals;
    return activeProposals;
}

template <typename ProposalVotingType>
int countFinishedProposals(
    const QpiContextUserProcedureCall& qpi,
    const ProposalVotingType& pv
)
{
    int finishedProposals = 0;
    QPI::sint32 idx = -1;
    while ((idx = qpi(*pv).nextFinishedProposalIndex(idx)) >= 0)
        ++finishedProposals;
    return finishedProposals;
}

TEST(TestCoreQPI, ProposalVoting)
{
    QpiContextUserProcedureCall qpi(0, QPI::id(1,2,3,4), 123);
    auto * pv = new QPI::ProposalVoting<
        QPI::ProposalAndVotingByComputors<200>,   // Allow less proposals than NUMBER_OF_COMPUTORS to check handling of full arrays
        QPI::ProposalData>;

    // Memory must be zeroed to work, which is done in contract states on init
    QPI::setMemory(*pv, 0);

    // fail: get before propsals have been set
    QPI::ProposalData proposalReturned;
    QPI::SingleProposalVoteData voteDataReturned;
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        EXPECT_FALSE(qpi(*pv).getProposal(i, proposalReturned));
        EXPECT_FALSE(qpi(*pv).getVote(i, 0, voteDataReturned));
    }
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(123456), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(123456), -1);

    // fail: get with invalid proposal index or vote index
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getProposal(pv->maxProposals + 1, proposalReturned));
    EXPECT_FALSE(qpi(*pv).getVote(pv->maxProposals, 0, voteDataReturned));
    EXPECT_FALSE(qpi(*pv).getVote(pv->maxProposals + 1, 0, voteDataReturned));

    // fail: no proposals for given IDs and invalid input
    EXPECT_EQ((int)qpi(*pv).proposalIndex(QPI::NULL_ID), (int)QPI::INVALID_PROPOSAL_INDEX); // always equal
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.originator()), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(0)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(qpi(*pv).proposerId(QPI::INVALID_PROPOSAL_INDEX), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(pv->maxProposals), QPI::NULL_ID); // always equal
    EXPECT_EQ(qpi(*pv).proposerId(0), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).proposerId(1), QPI::NULL_ID);

    // okay: voters are available independently of propsals (= computors)
    for (int i = 0; i < pv->maxVoters; ++i)
    {
        EXPECT_EQ(qpi(*pv).voterIndex(qpi.computor(i)), i);
        EXPECT_EQ(qpi(*pv).voterId(i), qpi.computor(i));
    }

    // fail: IDs / indcies of non-voters
    EXPECT_EQ(qpi(*pv).voterIndex(qpi.originator()), QPI::INVALID_VOTER_INDEX);
    EXPECT_EQ(qpi(*pv).voterIndex(QPI::NULL_ID), QPI::INVALID_VOTER_INDEX);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVoters), QPI::NULL_ID);
    EXPECT_EQ(qpi(*pv).voterId(pv->maxVoters + 1), QPI::NULL_ID);

    // okay: set proposal for computor 0
    QPI::ProposalData proposal;
    proposal.url.set(0, 0);
    proposal.epoch = qpi.epoch();
    proposal.type = 0;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(0), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(0)), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote although no proposal is available
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 1, 0, 0);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 12345, 0, 0);

    // fail: vote with wrong type
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 0, 1, 0);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), 0, 2, 0);

    // fail: vote with non-computor
    voteWithInvalidVoter(qpi, pv, qpi.originator(), 0, 0, 0);
    voteWithInvalidVoter(qpi, pv, QPI::NULL_ID, 0, 0, 0);

    // okay: correct votes in proposalIndex 0
    expectNoVotes(qpi, pv, 0);
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), 0, 0, (i - 4) % 5);
    voteWithValidVoter<true>(qpi, pv, qpi.computor(0), 0, 0, QPI::NO_VOTE_VALUE); // remove vote

    // fail: originator id(1,2,3,4) is no computor (see custom qpi.computor() above)
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.originator(), proposal));

    // fail: invalid type
    proposal.type = 123;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));

    // fail: wrong epoch (only current epoch possible)
    proposal.type = 0;
    proposal.epoch = 1;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));
    proposal.epoch = qpi.epoch() + 1;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));

    // okay: set proposal for computor 2 (proposal index 1, first use)
    proposal.epoch = qpi.epoch();
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(2), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(2)), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // okay: correct votes in proposalIndex 1 (first use)
    expectNoVotes(qpi, pv, 1);
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), 1, proposal.type, (i - 4) % 5);

    // fail: proposal of revenue distribution with wrong address
    proposal.type = 1;
    proposal.revenueDistribution.targetAddress = QPI::NULL_ID;
    proposal.revenueDistribution.proposedAmount = 100000;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));
    // check that overwrite did not work
    EXPECT_TRUE(qpi(*pv).getProposal(qpi(*pv).proposalIndex(qpi.computor(2)), proposalReturned));
    EXPECT_FALSE(proposalReturned == proposal);

    // fail: proposal of revenue distribution with invalid amount
    proposal.revenueDistribution.targetAddress = qpi.originator();
    proposal.revenueDistribution.proposedAmount = -123456;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(2), proposal));

    // okay: revenue distribution, overwrite existing proposal of comp 2 (proposal index 1, reused)
    proposal.revenueDistribution.targetAddress = qpi.originator();
    proposal.revenueDistribution.proposedAmount = 1005;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(2), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(2)), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, -1);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(1), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, -123456);

    // okay: correct votes in proposalIndex 1 (reused)
    expectNoVotes(qpi, pv, 1); // checks that setProposal clears previous votes
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, 1000 * i);
    voteWithValidVoter<true>(qpi, pv, qpi.computor(2), qpi(*pv).proposalIndex(qpi.computor(2)), proposal.type, QPI::NO_VOTE_VALUE); // remove vote

    // fail: type 2 proposal with wrong min/max
    proposal.type = 2;
    proposal.variableValue.proposedValue = 10;
    proposal.variableValue.minValue = 11;
    proposal.variableValue.maxValue = 20;
    proposal.variableValue.variable = 123; // not checked, full range usable
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));
    proposal.variableValue.minValue = 0;
    proposal.variableValue.maxValue = 9;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));

    // fail: type 2 proposal with full range is invalid, because NO_VOTE_VALUE is reserved for no vote
    proposal.variableValue.minValue = 0x8000000000000000;
    proposal.variableValue.maxValue = 0x7fffffffffffffff;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(1), proposal));

    // okay: type 2 proposal with nearly full range
    proposal.variableValue.minValue = 0x8000000000000001;
    proposal.variableValue.maxValue = 0x7fffffffffffffff;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(1), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(1)), 2);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(2)), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), 2);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(2), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // okay: type 2 proposal with limited range
    proposal.variableValue.minValue = -100;
    proposal.variableValue.maxValue = 100;
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(10), proposal);
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(10)), 3);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(-1), 0);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(0), 1);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(1), 2);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(2), 3);
    EXPECT_EQ(qpi(*pv).nextActiveProposalIndex(3), -1);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: vote with invalid values
    voteWithValidVoter<false>(qpi, pv, qpi.computor(0), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, -101);
    voteWithValidVoter<false>(qpi, pv, qpi.computor(1), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, 101);

    // okay: correct votes in proposalIndex of computor 10
    expectNoVotes(qpi, pv, qpi(*pv).proposalIndex(qpi.computor(10)));
    for (int i = 0; i < pv->maxVoters; ++i)
        voteWithValidVoter<true>(qpi, pv, qpi.computor(i), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, (i % 201) - 100);
    voteWithValidVoter<true>(qpi, pv, qpi.computor(2), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, QPI::NO_VOTE_VALUE); // remove vote

    // fill proposal storage
    for (int i = 0; i < pv->maxProposals; ++i)
    {
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    }
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(qpi(*pv).nextFinishedProposalIndex(-1), -1);

    // fail: no space left
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.computor(pv->maxProposals), proposal));

    // simulate epoch change
    ++currentEpoch;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals);

    // okay: same setProposal after epoch change, because the oldest proposal will be deleted
    proposal.epoch = qpi.epoch();
    setProposalWithSuccessCheck(qpi, pv, qpi.computor(pv->maxProposals), proposal);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 1);

    // fail: vote in wrong epoch
    voteWithValidVoter<false>(qpi, pv, qpi.computor(123), qpi(*pv).proposalIndex(qpi.computor(10)), proposal.type, 0);

    // manually clear some proposals
    EXPECT_FALSE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.originator())));
    EXPECT_TRUE(qpi(*pv).clearProposal(qpi(*pv).proposalIndex(qpi.computor(5))));
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(5)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);
    proposal.epoch = 0;
    EXPECT_FALSE(qpi(*pv).setProposal(qpi.originator(), proposal));
    EXPECT_TRUE(qpi(*pv).setProposal(qpi.computor(7), proposal));
    EXPECT_EQ((int)qpi(*pv).proposalIndex(qpi.computor(7)), (int)QPI::INVALID_PROPOSAL_INDEX);
    EXPECT_EQ(countActiveProposals(qpi, pv), 1);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 3);

    // simulate epoch change with changes in computors
    ++currentEpoch;
    computorIdOffset += 100;
    EXPECT_EQ(countActiveProposals(qpi, pv), 0);
    EXPECT_EQ(countFinishedProposals(qpi, pv), (int)pv->maxProposals - 2);

    // set new proposals, adding new computor IDs (simulated next epoch)
    proposal.epoch = qpi.epoch();
    proposal.type = 0;
    for (int i = 0; i < pv->maxProposals; ++i)
        setProposalWithSuccessCheck(qpi, pv, qpi.computor(i), proposal);
    for (int i = 0; i < pv->maxProposals; ++i)
        expectNoVotes(qpi, pv, i);
    EXPECT_EQ(countActiveProposals(qpi, pv), (int)pv->maxProposals);
    EXPECT_EQ(countFinishedProposals(qpi, pv), 0);

    // get results
}
