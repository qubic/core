#define NO_UEFI

#include "contract_testing.h"

#define PRINT_DETAILS 0

class GQmPropChecker : public GQMPROP
{
public:
    void checkRevenueDonations(
        std::vector<id>* expectedOrder = nullptr,
        std::vector<GQMPROP::RevenueDonationEntry>* expectedEntries = nullptr,
        bool printTable = PRINT_DETAILS)
    {
        const GQMPROP::RevenueDonationT& revDon = this->revenueDonation;
        const GQMPROP::RevenueDonationEntry* cur = nullptr;
        const GQMPROP::RevenueDonationEntry* prev = nullptr;
        uint64 idxRD = 0;

        if (printTable)
        {
            std::cout << "Revenue donations table (epoch " << system.epoch << "):" << std::endl;
            for (idxRD = 0; idxRD < revDon.capacity(); ++idxRD)
            {
                cur = &revDon.get(idxRD);
                if (isZero(cur->destinationPublicKey))
                    break;
                std::cout << "- " << idxRD << ": ID " << cur->destinationPublicKey << ", epoch " << cur->firstEpoch << ", amount " << float(cur->millionthAmount) / 1000000.f << std::endl;
            }
        }

        // check the used part of the table
        std::set<id> prevIds;
        uint64 idxO = 0;
        for (idxRD = 0; idxRD < revDon.capacity(); ++idxRD)
        {
            cur = &revDon.get(idxRD);
            if (isZero(cur->destinationPublicKey))
            {
                // done with checking used part of the table
                break;
            }
            if (idxRD > 0)
            {
                if (cur->destinationPublicKey == prev->destinationPublicKey)
                {
                    // entries of same ID
                    // -> check order
                    EXPECT_LT((int)prev->firstEpoch, (int)cur->firstEpoch);
                    // -> check that we don't have two non-future entries
                    if (prev->firstEpoch < system.epoch)
                    {
                        EXPECT_GE((int)cur->firstEpoch, (int)system.epoch);
                    }
                }
                else
                {
                    // next ID
                    // -> check that we haven't seen it before
                    EXPECT_EQ(prevIds.find(cur->destinationPublicKey), prevIds.end());
                    ++idxO;
                }
            }

            EXPECT_GE(cur->millionthAmount, 0);
            EXPECT_LE(cur->millionthAmount, 1000000);

            if (expectedOrder)
            {
                EXPECT_LT(idxO, expectedOrder->size());
                EXPECT_EQ(cur->destinationPublicKey, expectedOrder->at(idxO));
            }
            if (expectedEntries)
            {
                EXPECT_LT(idxRD, expectedEntries->size());
                EXPECT_EQ(cur->destinationPublicKey, expectedEntries->at(idxRD).destinationPublicKey);
                EXPECT_EQ((int)cur->firstEpoch, (int)expectedEntries->at(idxRD).firstEpoch);
                EXPECT_EQ(cur->millionthAmount, expectedEntries->at(idxRD).millionthAmount);
            }

            // update prev data for later checks
            prev = cur;
            prevIds.insert(cur->destinationPublicKey);
        }

        // check that all remaining entries are 0
        for (; idxRD < revDon.capacity(); ++idxRD)
        {
            EXPECT_TRUE(isZero(revDon.get(idxRD).destinationPublicKey));
            EXPECT_EQ((int)revDon.get(idxRD).firstEpoch, 0);
            EXPECT_EQ(revDon.get(idxRD).millionthAmount, 0);
        }
    }
};


class ContractTestingGQmProp : protected ContractTesting
{
public:
    ContractTestingGQmProp()
    {
        initEmptySpectrum();
        INIT_CONTRACT(GQMPROP);
        callSystemProcedure(GQMPROP_CONTRACT_INDEX, INITIALIZE);

        // Setup computors
        for (unsigned long long i = 0; i < NUMBER_OF_COMPUTORS; ++i)
        {
            broadcastedComputors.computors.publicKeys[i] = id(i, 1, 2, 3);
            increaseEnergy(id(i, 1, 2, 3), 1000000);
        }
    }

    GQmPropChecker* getState()
    {
        return (GQmPropChecker*)contractStates[GQMPROP_CONTRACT_INDEX];
    }

    GQMPROP::GetProposal_output getProposal(uint16 proposalIndex)
    {
        GQMPROP::GetProposal_input input{ proposalIndex };
        GQMPROP::GetProposal_output output;
        callFunction(GQMPROP_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    GQMPROP::GetVotingResults_output getResults(uint16 proposalIndex)
    {
        GQMPROP::GetVotingResults_input input{ proposalIndex };
        GQMPROP::GetVotingResults_output output;
        callFunction(GQMPROP_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    uint16 setProposal(const id& originator, const GQMPROP::ProposalDataT& proposalData)
    {
        GQMPROP::SetProposal_input input = proposalData;
        GQMPROP::SetProposal_output output;
        invokeUserProcedure(GQMPROP_CONTRACT_INDEX, 1, input, output, originator, 0);
        return output.proposalIndex;
    }

    bool vote(const id& originator, uint16 proposalIndex, const GQMPROP::ProposalDataT& proposalData, sint64 voteValue)
    {
        GQMPROP::Vote_input input{proposalIndex, proposalData.type, proposalData.tick, voteValue};
        GQMPROP::Vote_output output;
        invokeUserProcedure(GQMPROP_CONTRACT_INDEX, 2, input, output, originator, 0);
        return output.okay;
    }

    // TODO: add other procedures

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(GQMPROP_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void voteMultipleComputors(uint16 proposalIndex, uint16 votesNo, uint16 votesYes1, uint16 votesYes2 = 0)
    {
        EXPECT_LE(int(votesNo + votesYes1 + votesYes2), NUMBER_OF_COMPUTORS);
        const auto proposal = getProposal(proposalIndex);
        EXPECT_TRUE(proposal.okay);
        uint16 compIdx = 0;
        for (uint16 i = 0; i < votesNo; ++i, ++compIdx)
            EXPECT_TRUE(vote(id(compIdx, 1, 2, 3), proposalIndex, proposal.proposal, 0));
        for (uint16 i = 0; i < votesYes1; ++i, ++compIdx)
            EXPECT_TRUE(vote(id(compIdx, 1, 2, 3), proposalIndex, proposal.proposal, 1));
        for (uint16 i = 0; i < votesYes2; ++i, ++compIdx)
            EXPECT_TRUE(vote(id(compIdx, 1, 2, 3), proposalIndex, proposal.proposal, 2));
        auto results = getResults(proposalIndex);
        EXPECT_TRUE(results.okay);
        EXPECT_EQ(results.results.optionVoteCount.get(0), uint32(votesNo));
        EXPECT_EQ(results.results.optionVoteCount.get(1), uint32(votesYes1));
        EXPECT_EQ(results.results.optionVoteCount.get(2), uint32(votesYes2));
    }

    uint16 setupProposal(uint16 proposer, uint16 type, const id& dest, sint64 amount = 0, uint16 targetEpoch = 0, bool expectSuccess = true)
    {
        GQMPROP::ProposalDataT proposal;
        setMemory(proposal, 0);
        proposal.epoch = system.epoch;
        proposal.type = type;
        switch (ProposalTypes::cls(type))
        {
        case ProposalTypes::Class::Transfer:
        {
            const auto amountCount = ProposalTypes::optionCount(type) - 1;
            for (int i = 0; i < amountCount; ++i)
                proposal.transfer.amounts.set(i, amount + i);
            proposal.transfer.destination = dest;
            break;
        }
        case ProposalTypes::Class::TransferInEpoch:
            proposal.transferInEpoch.amount = amount;
            proposal.transferInEpoch.destination = dest;
            proposal.transferInEpoch.targetEpoch = targetEpoch;
            break;
        }
        uint16 proposalIdx = setProposal(id(proposer, 1, 2, 3), proposal);
        if (expectSuccess)
            EXPECT_NE((int)proposalIdx, (int)INVALID_PROPOSAL_INDEX);
        else
            EXPECT_EQ((int)proposalIdx, (int)INVALID_PROPOSAL_INDEX);
        return proposalIdx;
    }
};

static id ENTITY0(7, 0, 0, 0);
static id ENTITY1(100, 0, 0, 0);
static id ENTITY2(123, 456, 789, 0);
static id ENTITY3(42, 69, 0, 13);
static id ENTITY4(3, 14, 2, 7);

TEST(ContractGQmProp, RevDonation)
{
    ContractTestingGQmProp test;
    uint16 proposalIndex;
    std::vector<id> revDonationOrder;
    std::vector<GQMPROP::RevenueDonationEntry> revDonationEntries;

    // rev donation from INITALIZE
    revDonationOrder.push_back(ENTITY0);
    revDonationEntries = { {ENTITY0, 150000, 123} };

    //-------------------------------------------------------------
    // epoch 200
    system.epoch = 200;
    test.beginEpoch();
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);
    
    // one successful and several unsuccessful proposals (testing insert at end)
    proposalIndex = test.setupProposal(0, ProposalTypes::TransferYesNo, ENTITY1, 1000);
    test.voteMultipleComputors(proposalIndex, 200, 350);
    revDonationOrder.push_back(ENTITY1);

    proposalIndex = test.setupProposal(1, ProposalTypes::TransferYesNo, ENTITY2, 10000);
    test.voteMultipleComputors(proposalIndex, 100, 200); // total votes < QUORUM

    proposalIndex = test.setupProposal(2, ProposalTypes::TransferYesNo, ENTITY2, 20000);
    test.voteMultipleComputors(proposalIndex, 379, 256); // most noted is "no"

    proposalIndex = test.setupProposal(3, ProposalTypes::TransferThreeAmounts, ENTITY2, 30000);
    test.voteMultipleComputors(proposalIndex, 10, 20, 400); // total votes < QUORUM

    proposalIndex = test.setupProposal(4, ProposalTypes::TransferThreeAmounts, ENTITY2, 40000);
    test.voteMultipleComputors(proposalIndex, 201, 202, 203); // max voted < QUORUM/2

    //-------------------------------------------------------------
    // epoch 201
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = { {ENTITY0, 150000, 123}, {ENTITY1, 1000, 201} };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    // add future revenue donations (testing insert at end and in the middle)
    proposalIndex = test.setupProposal(0, ProposalTypes::TransferInEpochYesNo, ENTITY2, 2000, 205);
    test.voteMultipleComputors(proposalIndex, 0, 600);
    revDonationOrder.push_back(ENTITY2);

    proposalIndex = test.setupProposal(1, ProposalTypes::TransferInEpochYesNo, ENTITY1, 3000, 204);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(2, ProposalTypes::TransferInEpochYesNo, ENTITY1, 4000, 203);
    test.voteMultipleComputors(proposalIndex, 300, 350);

    proposalIndex = test.setupProposal(3, ProposalTypes::TransferInEpochYesNo, ENTITY1, 5000, 205);
    test.voteMultipleComputors(proposalIndex, 300, 350);

    //-------------------------------------------------------------
    // epoch 202
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = { {ENTITY0, 150000, 123}, {ENTITY1, 1000, 201}, {ENTITY1, 4000, 203}, {ENTITY1, 3000, 204}, {ENTITY1, 5000, 205}, {ENTITY2, 2000, 205} };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    //-------------------------------------------------------------
    // epoch 203 -> {ENTITY1, 1000, 201} is cleaned up, because {ENTITY1, 4000, 203} is in action now
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 150000, 123},
        {ENTITY1, 4000, 203}, {ENTITY1, 3000, 204}, {ENTITY1, 5000, 205},
        {ENTITY2, 2000, 205}
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    // add more dontaions
    proposalIndex = test.setupProposal(1, ProposalTypes::TransferThreeAmounts, ENTITY4, 6000);
    test.voteMultipleComputors(proposalIndex, 100, 100, 400); // vote for second amount (6001)
    revDonationOrder.push_back(ENTITY3);

    proposalIndex = test.setupProposal(0, ProposalTypes::TransferInEpochYesNo, ENTITY3, 7000, 205);
    test.voteMultipleComputors(proposalIndex, 100, 500);
    revDonationOrder.push_back(ENTITY4);

    // add at front of ENTITY3
    proposalIndex = test.setupProposal(2, ProposalTypes::TransferInEpochYesNo, ENTITY3, 8000, 204);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(3, ProposalTypes::TransferInEpochYesNo, ENTITY3, 9000, 210);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(4, ProposalTypes::TransferInEpochYesNo, ENTITY3, 10000, 207);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(5, ProposalTypes::TransferInEpochYesNo, ENTITY4, 11000, 220);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(6, ProposalTypes::TransferInEpochYesNo, ENTITY4, 12000, 210);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(7, ProposalTypes::TransferInEpochYesNo, ENTITY4, 13000, 208);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    // rejected due to non-future epoch
    test.setupProposal(1, ProposalTypes::TransferInEpochYesNo, ENTITY4, 7500, 203, false);

    //-------------------------------------------------------------
    // epoch 204 -> {ENTITY1, 4000, 203} is cleaned up, because {ENTITY1, 3000, 204} is in action now
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 150000, 123},
        {ENTITY1, 3000, 204}, {ENTITY1, 5000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 8000, 204}, {ENTITY3, 7000, 205}, {ENTITY3, 10000, 207}, {ENTITY3, 9000, 210},
        {ENTITY4, 6001, 204}, {ENTITY4, 13000, 208}, {ENTITY4, 12000, 210}, {ENTITY4, 11000, 220},
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    // update amounts (feature to overwrite/cancel scheduled changes)
    proposalIndex = test.setupProposal(0, ProposalTypes::TransferYesNo, ENTITY3, 14000); // epoch 205
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(1, ProposalTypes::TransferInEpochYesNo, ENTITY1, 15000, 205);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(2, ProposalTypes::TransferInEpochYesNo, ENTITY4, 16000, 220);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(3, ProposalTypes::TransferInEpochYesNo, ENTITY3, 17000, 207);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    // update with higher proposal index overwrites the one with lower index (both ENTITY3 epoch 205)
    proposalIndex = test.setupProposal(4, ProposalTypes::TransferInEpochYesNo, ENTITY3, 18000, 205);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    //-------------------------------------------------------------
    // epoch 205 -> {ENTITY1, 3000, 204} and {ENTITY3, 8000, 204} are cleaned up
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 150000, 123},
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 18000, 205}, {ENTITY3, 17000, 207}, {ENTITY3, 9000, 210},
        {ENTITY4, 6001, 204}, {ENTITY4, 13000, 208}, {ENTITY4, 12000, 210}, {ENTITY4, 16000, 220},
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    // update amount (feature to overwrite/cancel scheduled changes)
    proposalIndex = test.setupProposal(0, ProposalTypes::TransferInEpochYesNo, ENTITY3, 19000, 210);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    // schedule new rev. donation for ENTITY0
    proposalIndex = test.setupProposal(1, ProposalTypes::TransferInEpochYesNo, ENTITY0, 20000, 210);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(2, ProposalTypes::TransferInEpochYesNo, ENTITY0, 21000, 207);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(3, ProposalTypes::TransferInEpochYesNo, ENTITY0, 22000, 215);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(4, ProposalTypes::TransferInEpochYesNo, ENTITY0, 23000, 208);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    //-------------------------------------------------------------
    // epoch 206 -> nothing cleaned up (not epoch 206 donation entry gets in action)
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 150000, 123}, {ENTITY0, 21000, 207}, {ENTITY0, 23000, 208}, {ENTITY0, 20000, 210}, {ENTITY0, 22000, 215},
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 18000, 205}, {ENTITY3, 17000, 207}, {ENTITY3, 19000, 210},
        {ENTITY4, 6001, 204}, {ENTITY4, 13000, 208}, {ENTITY4, 12000, 210}, {ENTITY4, 16000, 220},
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    //-------------------------------------------------------------
    // epoch 207 -> entries from ENTITY0 and ENTITY3 cleaned up
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 21000, 207}, {ENTITY0, 23000, 208}, {ENTITY0, 20000, 210}, {ENTITY0, 22000, 215},
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 17000, 207}, {ENTITY3, 19000, 210},
        {ENTITY4, 6001, 204}, {ENTITY4, 13000, 208}, {ENTITY4, 12000, 210}, {ENTITY4, 16000, 220},
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    //-------------------------------------------------------------
    // epoch 208 -> entries from ENTITY0 and ENTITY4 cleaned up
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 23000, 208}, {ENTITY0, 20000, 210}, {ENTITY0, 22000, 215},
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 17000, 207}, {ENTITY3, 19000, 210},
        {ENTITY4, 13000, 208}, {ENTITY4, 12000, 210}, {ENTITY4, 16000, 220},
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    // fill up the revenue donation table storage completely (2 of 10 are updated, 118 are added, 10 cannot be added
    // due to lack of storage)
    revDonationEntries = { {ENTITY0, 23000, 208} };
    for (unsigned int i = 0; i < 130; ++i)
    {
        proposalIndex = test.setupProposal(i, ProposalTypes::TransferInEpochYesNo, ENTITY0, 30000 + i, 210 + i);
        test.voteMultipleComputors(proposalIndex, 100, 500);
        if (i < 120)
            revDonationEntries.push_back({ ENTITY0, 30000 + i, uint16(210 + i) });
    }
    revDonationEntries.insert(revDonationEntries.end(), {
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 17000, 207}, {ENTITY3, 19000, 210},
        {ENTITY4, 13000, 208}, {ENTITY4, 12000, 210}, {ENTITY4, 16000, 220} });
    EXPECT_EQ(revDonationEntries.size(), 128);

    //-------------------------------------------------------------
    // epoch 209 -> no entries cleaned up
    ++system.epoch;
    test.beginEpoch();
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    //-------------------------------------------------------------
    // epoch 210 -> 3 entries cleaned up (ENTITY0, ENTITY3, ENTITY4) 
    ++system.epoch;
    test.beginEpoch();

    revDonationEntries.clear();
    for (unsigned int i = 0; i < 120; ++i)
    {
        revDonationEntries.push_back({ ENTITY0, 30000 + i, uint16(210 + i) });
    }
    revDonationEntries.insert(revDonationEntries.end(), {
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 19000, 210},
        {ENTITY4, 12000, 210}, {ENTITY4, 16000, 220} });

    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);


    //-------------------------------------------------------------
    // epoch 211 -> entry removed from ENTITY0 (first entry)
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries.erase(revDonationEntries.begin());
    EXPECT_EQ(revDonationEntries.size(), 124);
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    // fill up storage at the end
    proposalIndex = test.setupProposal(0, ProposalTypes::TransferInEpochYesNo, ENTITY4, 40000, 213);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(1, ProposalTypes::TransferInEpochYesNo, ENTITY4, 41000, 225);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(2, ProposalTypes::TransferInEpochYesNo, ENTITY4, 42000, 215);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(3, ProposalTypes::TransferInEpochYesNo, ENTITY4, 43000, 214);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    // will fail due to storage limitation
    proposalIndex = test.setupProposal(4, ProposalTypes::TransferInEpochYesNo, ENTITY4, 44000, 230);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(5, ProposalTypes::TransferInEpochYesNo, ENTITY4, 44000, 212);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    revDonationEntries.erase(revDonationEntries.end() - 1);
    revDonationEntries.insert(revDonationEntries.end(), {
        {ENTITY4, 40000, 213}, {ENTITY4, 43000, 214}, {ENTITY4, 42000, 215}, {ENTITY4, 16000, 220}, {ENTITY4, 41000, 225},
        });

    //-------------------------------------------------------------
    // epoch 212 -> entry removed from ENTITY0 (first entry)
    ++system.epoch;
    test.beginEpoch();
    revDonationEntries.erase(revDonationEntries.begin());
    EXPECT_EQ(revDonationEntries.size(), 127);
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);

    //-------------------------------------------------------------
    // run 120 epochs reducing table back to one entry per entity
    revDonationEntries = {
        {ENTITY0, 30119, 329},
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 19000, 210},
        {ENTITY4, 41000, 225},
    };
    for (unsigned int i = 0; i < 120; ++i)
    {
        ++system.epoch;
        test.beginEpoch();
        if (i == 119)
            test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries, true);
        else
            test.getState()->checkRevenueDonations(&revDonationOrder, nullptr, false);
    }

    //-------------------------------------------------------------
    // test that IDs are not removed even if donation percentage is 0
    proposalIndex = test.setupProposal(0, ProposalTypes::TransferYesNo, ENTITY0, 0);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    proposalIndex = test.setupProposal(1, ProposalTypes::TransferYesNo, ENTITY3, 0);
    test.voteMultipleComputors(proposalIndex, 100, 500);

    ++system.epoch;
    test.beginEpoch();
    revDonationEntries = {
        {ENTITY0, 0, 333},
        {ENTITY1, 15000, 205},
        {ENTITY2, 2000, 205},
        {ENTITY3, 0, 333},
        {ENTITY4, 41000, 225},
    };
    test.getState()->checkRevenueDonations(&revDonationOrder, &revDonationEntries);
}
