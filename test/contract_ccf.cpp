#define NO_UEFI

#include "contract_testing.h"

#define PRINT_DETAILS 0

class CCFChecker : public CCF
{
public:
    void checkSubscriptions(bool printDetails = PRINT_DETAILS)
    {
        if (printDetails)
        {
            std::cout << "Subscriptions (total capacity: " << subscriptions.capacity() << "):" << std::endl;
            for (uint64 i = 0; i < subscriptions.capacity(); ++i)
            {
                const SubscriptionData& sub = subscriptions.get(i);
                if (!isZero(sub.proposerId))
                {
                    std::cout << "- Index " << i << ": proposerId=" << sub.proposerId 
                              << ", isActive=" << (int)sub.isActive
                              << ", periodType=" << (int)sub.periodType
                              << ", numberOfPeriods=" << sub.numberOfPeriods
                              << ", amountPerPeriod=" << sub.amountPerPeriod
                              << ", startEpoch=" << sub.startEpoch
                              << ", currentPeriod=" << sub.currentPeriod << std::endl;
                }
            }
        }
    }

    sint32 findSubscriptionIndex(const id& proposerId)
    {
        for (sint32 i = 0; i < subscriptions.capacity(); ++i)
        {
            const SubscriptionData& sub = subscriptions.get(i);
            if (sub.proposerId == proposerId)
                return i;
        }
        return -1;
    }

    const SubscriptionData* getSubscriptionByProposer(const id& proposerId)
    {
        for (uint64 i = 0; i < subscriptions.capacity(); ++i)
        {
            const SubscriptionData& sub = subscriptions.get(i);
            if (sub.proposerId == proposerId)
                return &sub;
        }
        return nullptr;
    }

    bool hasActiveSubscription(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr && sub->isActive;
    }

    bool hasSubscription(const id& proposerId)
    {
        return getSubscriptionByProposer(proposerId) != nullptr;
    }

    sint32 getSubscriptionCurrentPeriod(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr ? sub->currentPeriod : -1;
    }

    bool getSubscriptionIsActive(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr && sub->isActive;
    }

    uint8 getSubscriptionPeriodType(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr ? sub->periodType : 0;
    }

    uint32 getSubscriptionNumberOfPeriods(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr ? sub->numberOfPeriods : 0;
    }

    sint64 getSubscriptionAmountPerPeriod(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr ? sub->amountPerPeriod : 0;
    }

    uint32 getSubscriptionStartEpoch(const id& proposerId)
    {
        const SubscriptionData* sub = getSubscriptionByProposer(proposerId);
        return sub != nullptr ? sub->startEpoch : 0;
    }

    uint32 countActiveSubscriptions()
    {
        uint32 count = 0;
        for (uint64 i = 0; i < subscriptions.capacity(); ++i)
        {
            if (subscriptions.get(i).isActive)
                count++;
        }
        return count;
    }

    uint32 countSubscriptionsByProposer(const id& proposerId)
    {
        uint32 count = 0;
        for (uint64 i = 0; i < subscriptions.capacity(); ++i)
        {
            const SubscriptionData& sub = subscriptions.get(i);
            if (sub.proposerId == proposerId && sub.isActive)
                count++;
        }
        return count;
    }

    uint32 getMaxSubscriptionEpochs()
    {
        return maxSubscriptionEpochs;
    }
};

class ContractTestingCCF : protected ContractTesting
{
public:
    ContractTestingCCF()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(CCF);
        callSystemProcedure(CCF_CONTRACT_INDEX, INITIALIZE);

        // Setup computors
        for (unsigned long long i = 0; i < NUMBER_OF_COMPUTORS; ++i)
        {
            broadcastedComputors.computors.publicKeys[i] = id(i, 1, 2, 3);
            increaseEnergy(id(i, 1, 2, 3), 1000000);
        }
    }

    ~ContractTestingCCF()
    {
        checkContractExecCleanup();
    }

    CCFChecker* getState()
    {
        return (CCFChecker*)contractStates[CCF_CONTRACT_INDEX];
    }

    CCF::SetProposal_output setProposal(const id& originator, const CCF::SetProposal_input& input)
    {
        CCF::SetProposal_output output;
        invokeUserProcedure(CCF_CONTRACT_INDEX, 1, input, output, originator, 1000000);
        return output;
    }

    CCF::GetProposal_output getProposal(uint32 proposalIndex, const id& subscriptionProposerId = NULL_ID)
    {
        CCF::GetProposal_input input;
        input.proposalIndex = (uint16)proposalIndex;
        input.subscriptionProposerId = subscriptionProposerId;
        CCF::GetProposal_output output;
        callFunction(CCF_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    CCF::GetVotingResults_output getVotingResults(uint32 proposalIndex)
    {
        CCF::GetVotingResults_input input;
        CCF::GetVotingResults_output output;

        input.proposalIndex = (uint16)proposalIndex;
        callFunction(CCF_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    bool vote(const id& originator, const CCF::Vote_input& input)
    {
        CCF::Vote_output output;
        invokeUserProcedure(CCF_CONTRACT_INDEX, 2, input, output, originator, 0);
        return output.okay;
    }

    CCF::GetLatestTransfers_output getLatestTransfers()
    {
        CCF::GetLatestTransfers_output output;
        callFunction(CCF_CONTRACT_INDEX, 5, CCF::GetLatestTransfers_input(), output);
        return output;
    }

    CCF::GetRegularPayments_output getRegularPayments()
    {
        CCF::GetRegularPayments_output output;
        callFunction(CCF_CONTRACT_INDEX, 7, CCF::GetRegularPayments_input(), output);
        return output;
    }

    CCF::GetProposalFee_output getProposalFee()
    {
        CCF::GetProposalFee_output output;
        callFunction(CCF_CONTRACT_INDEX, 6, CCF::GetProposalFee_input(), output);
        return output;
    }

    CCF::GetProposalIndices_output getProposalIndices(bool activeProposals, sint32 prevProposalIndex = -1)
    {
        CCF::GetProposalIndices_input input;
        input.activeProposals = activeProposals;
        input.prevProposalIndex = prevProposalIndex;
        CCF::GetProposalIndices_output output;
        callFunction(CCF_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(CCF_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(CCF_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    uint32 setupRegularProposal(const id& proposer, const id& destination, sint64 amount, bool expectSuccess = true)
    {
        CCF::SetProposal_input input;
        setMemory(input, 0);
        input.proposal.epoch = system.epoch;
        input.proposal.type = ProposalTypes::TransferYesNo;
        input.proposal.transfer.destination = destination;
        input.proposal.transfer.amount = amount;
        input.isSubscription = false;

        auto output = setProposal(proposer, input);
        if (expectSuccess)
            EXPECT_NE((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
        else
            EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
        return output.proposalIndex;
    }

    uint32 setupSubscriptionProposal(const id& proposer, const id& destination, sint64 amountPerPeriod, 
                                      uint32 numberOfPeriods, uint8 periodType, uint32 startEpoch, bool expectSuccess = true)
    {
        CCF::SetProposal_input input;
        setMemory(input, 0);
        input.proposal.epoch = system.epoch;
        input.proposal.type = ProposalTypes::TransferYesNo;
        input.proposal.transfer.destination = destination;
        input.proposal.transfer.amount = amountPerPeriod;
        input.isSubscription = true;
        input.periodType = periodType;
        input.numberOfPeriods = numberOfPeriods;
        input.startEpoch = startEpoch;
        input.amountPerPeriod = amountPerPeriod;

        auto output = setProposal(proposer, input);
        if (expectSuccess)
            EXPECT_NE((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
        else
            EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
        return output.proposalIndex;
    }

    void voteMultipleComputors(uint32 proposalIndex, uint32 votesNo, uint32 votesYes)
    {
        EXPECT_LE((int)(votesNo + votesYes), (int)NUMBER_OF_COMPUTORS);
        const auto proposal = getProposal(proposalIndex);
        EXPECT_TRUE(proposal.okay);
        
        CCF::Vote_input voteInput;
        voteInput.proposalIndex = (uint16)proposalIndex;
        voteInput.proposalType = proposal.proposal.type;
        voteInput.proposalTick = proposal.proposal.tick;
        
        uint32 compIdx = 0;
        for (uint32 i = 0; i < votesNo; ++i, ++compIdx)
        {
            voteInput.voteValue = 0;  // 0 = no vote
            EXPECT_TRUE(vote(id(compIdx, 1, 2, 3), voteInput));
        }
        for (uint32 i = 0; i < votesYes; ++i, ++compIdx)
        {
            voteInput.voteValue = 1;  // 1 = yes vote
            EXPECT_TRUE(vote(id(compIdx, 1, 2, 3), voteInput));
        }
        
        auto results = getVotingResults(proposalIndex);
        EXPECT_TRUE(results.okay);
        EXPECT_EQ(results.results.optionVoteCount.get(0), uint32(votesNo));
        EXPECT_EQ(results.results.optionVoteCount.get(1), uint32(votesYes));
    }
};

static id ENTITY0(7, 0, 0, 0);
static id ENTITY1(100, 0, 0, 0);
static id ENTITY2(123, 456, 789, 0);
static id ENTITY3(42, 69, 0, 13);
static id ENTITY4(3, 14, 2, 7);

TEST(ContractCCF, BasicInitialization)
{
    ContractTestingCCF test;
    
    // Check initial state
    auto fee = test.getProposalFee();
    EXPECT_EQ(fee.proposalFee, 1000000u);
    EXPECT_EQ(test.getState()->getMaxSubscriptionEpochs(), 52u);
}

TEST(ContractCCF, RegularProposalAndVoting)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();

    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    
    // Set a regular transfer proposal
    increaseEnergy(PROPOSER1, 1000000);
    uint32 proposalIndex = test.setupRegularProposal(PROPOSER1, ENTITY1, 10000);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Get proposal
    auto proposal = test.getProposal(proposalIndex);
    EXPECT_TRUE(proposal.okay);
    EXPECT_EQ(proposal.proposal.transfer.destination, ENTITY1);
    
    // Vote on proposal
    test.voteMultipleComputors(proposalIndex, 200, 350);
    
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);

    // End epoch to process votes
    test.endEpoch();
    
    // Check that transfer was executed
    auto transfers = test.getLatestTransfers();
    bool found = false;
    for (uint64 i = 0; i < transfers.capacity(); ++i)
    {
        if (transfers.get(i).destination == ENTITY1 && transfers.get(i).amount == 10000)
        {
            found = true;
            EXPECT_TRUE(transfers.get(i).success);
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(ContractCCF, SubscriptionProposalCreation)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);

    // Create a subscription proposal
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 12, CCF_SUBSCRIPTION_PERIOD_MONTH, system.epoch + 1);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Check that subscription was stored
    auto state = test.getState();
    EXPECT_TRUE(state->hasSubscription(PROPOSER1));
    EXPECT_TRUE(state->getSubscriptionIsActive(PROPOSER1));
    EXPECT_EQ(state->getSubscriptionPeriodType(PROPOSER1), CCF_SUBSCRIPTION_PERIOD_MONTH);
    EXPECT_EQ(state->getSubscriptionNumberOfPeriods(PROPOSER1), 12u);
    EXPECT_EQ(state->getSubscriptionAmountPerPeriod(PROPOSER1), 1000);
    EXPECT_EQ(state->getSubscriptionStartEpoch(PROPOSER1), system.epoch + 1);
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(PROPOSER1), -1);
    
    // Get proposal with subscription data
    auto proposal = test.getProposal(proposalIndex, PROPOSER1);
    EXPECT_TRUE(proposal.okay);
    EXPECT_FALSE(isZero(proposal.subscription.proposerId));
}

TEST(ContractCCF, SubscriptionProposalVotingAndActivation)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    // Create a subscription proposal starting next epoch
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, system.epoch + 1);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
        
    // Vote to approve
    test.voteMultipleComputors(proposalIndex, 200, 350);
    
    // End epoch
    test.endEpoch();
    
    // Check subscription is deactivated before voting completes
    auto state = test.getState();
    
    // Begin next epoch and end it to activate subscription
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Check subscription is now active
    EXPECT_TRUE(state->hasActiveSubscription(PROPOSER1));
    EXPECT_TRUE(state->getSubscriptionIsActive(PROPOSER1));
}

TEST(ContractCCF, SubscriptionPaymentProcessing)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    // Create subscription starting in epoch 189, weekly payments
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 500, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Approve proposal
    test.voteMultipleComputors(proposalIndex, 200, 350);
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    sint64 initialBalance = getBalance(ENTITY1);

    // Move to start epoch and activate
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Move to next epoch - should trigger first payment
    system.epoch = 190;
    test.beginEpoch();
    test.endEpoch();
    
    // Check payment was made
    sint64 newBalance = getBalance(ENTITY1);
    EXPECT_GE(newBalance, initialBalance + 500 + 500);
    
    // Check regular payments log
    auto payments = test.getRegularPayments();
    bool foundPayment = false;
    for (uint64 i = 0; i < payments.capacity(); ++i)
    {
        const auto& payment = payments.get(i);
        if (payment.destination == ENTITY1 && payment.amount == 500 && payment.periodIndex == 0)
        {
            foundPayment = true;
            EXPECT_TRUE(payment.success);
            break;
        }
    }
    EXPECT_TRUE(foundPayment);
    
    // Check subscription currentPeriod was updated
    auto state = test.getState();
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(PROPOSER1), 1);
}

TEST(ContractCCF, MultipleSubscriptionPayments)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    // Create monthly subscription (4 epochs per period)
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 3, CCF_SUBSCRIPTION_PERIOD_MONTH, 189);
    
    test.voteMultipleComputors(proposalIndex, 200, 350);
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    sint64 initialBalance = getBalance(ENTITY1);

    // Activate subscription
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Move through epochs - should trigger payments at epochs 189, 193, 197
    for (uint32 epoch = 189; epoch <= 197; ++epoch)
    {
        system.epoch = epoch;
        test.beginEpoch();
        test.endEpoch();
    }
    
    // Should have made 3 payments (periods 0, 1, 2)
    sint64 newBalance = getBalance(ENTITY1);
    EXPECT_GE(newBalance, initialBalance + 1000 + 1000 + 1000);
    
    // Check subscription completed all periods
    auto state = test.getState();
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(PROPOSER1), 2);
}

TEST(ContractCCF, PreventMultipleActiveSubscriptions)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    // Create first subscription
    uint32 proposalIndex1 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);

    increaseEnergy(PROPOSER1, 1000000);
    // Try to create second subscription for same proposer - should fail
    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY2, 2000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189, false);
    EXPECT_EQ((int)proposalIndex2, (int)INVALID_PROPOSAL_INDEX);
}

TEST(ContractCCF, CancelSubscription)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    // Create subscription
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    // Cancel proposal (epoch = 0)
    CCF::SetProposal_input cancelInput;
    setMemory(cancelInput, 0);
    cancelInput.proposal.epoch = 0;
    cancelInput.proposal.type = ProposalTypes::TransferYesNo;
    cancelInput.isSubscription = true;
    cancelInput.periodType = CCF_SUBSCRIPTION_PERIOD_WEEK;
    cancelInput.numberOfPeriods = 4;
    cancelInput.startEpoch = 189;
    cancelInput.amountPerPeriod = 1000;
    auto cancelOutput = test.setProposal(PROPOSER1, cancelInput);
    EXPECT_NE((int)cancelOutput.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Check subscription was deactivated
    auto state = test.getState();
    EXPECT_TRUE(state->hasSubscription(PROPOSER1));            // because proposal is not cleared, it is still stored in the subscriptions array
    EXPECT_FALSE(state->hasActiveSubscription(PROPOSER1));    // because proposal is canceled, subscription is deactivated
}

TEST(ContractCCF, SubscriptionValidation)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Test invalid period type
    CCF::SetProposal_input input;
    setMemory(input, 0);
    input.proposal.epoch = system.epoch;
    input.proposal.type = ProposalTypes::TransferYesNo;
    input.proposal.transfer.destination = ENTITY1;
    input.proposal.transfer.amount = 1000;
    input.isSubscription = true;
    input.periodType = 99; // Invalid
    input.numberOfPeriods = 4;
    input.startEpoch = system.epoch + 1;
    input.amountPerPeriod = 1000;
    
    auto output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);

    // Test start epoch in past
    increaseEnergy(PROPOSER1, 1000000);
    input.periodType = CCF_SUBSCRIPTION_PERIOD_WEEK;
    input.startEpoch = system.epoch; // Should be > current epoch
    output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);

    // Test zero periods
    increaseEnergy(PROPOSER1, 1000000);
    input.startEpoch = system.epoch + 1;
    input.numberOfPeriods = 0;
    output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Test negative amount
    increaseEnergy(PROPOSER1, 1000000);
    input.numberOfPeriods = 4;
    input.amountPerPeriod = -100;
    output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
}

TEST(ContractCCF, MultipleProposers)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    id PROPOSER2 = broadcastedComputors.computors.publicKeys[1];
    increaseEnergy(PROPOSER2, 1000000);
    
    // Create subscriptions for different proposers
    uint32 proposalIndex1 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);
    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER2, ENTITY2, 2000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex2, (int)INVALID_PROPOSAL_INDEX);
    
    // Both should be stored
    auto state = test.getState();
    EXPECT_TRUE(state->hasActiveSubscription(PROPOSER1));
    EXPECT_TRUE(state->hasActiveSubscription(PROPOSER2));
    EXPECT_EQ(state->countSubscriptionsByProposer(PROPOSER1), 1u);
    EXPECT_EQ(state->countSubscriptionsByProposer(PROPOSER2), 1u);
}

TEST(ContractCCF, ProposalRejectedNoQuorum)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    uint32 proposalIndex = test.setupRegularProposal(PROPOSER1, ENTITY1, 10000);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote but not enough for quorum
    test.voteMultipleComputors(proposalIndex, 100, 200);
    
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Transfer should not have been executed
    auto transfers = test.getLatestTransfers();
    bool found = false;
    for (uint64 i = 0; i < transfers.capacity(); ++i)
    {
        if (transfers.get(i).destination == ENTITY1 && transfers.get(i).amount == 10000)
        {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found);
}

TEST(ContractCCF, ProposalRejectedMoreNoVotes)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    uint32 proposalIndex = test.setupRegularProposal(PROPOSER1, ENTITY1, 10000);
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // More "no" votes than "yes" votes
    test.voteMultipleComputors(proposalIndex, 350, 200);
    
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Transfer should not have been executed
    auto transfers = test.getLatestTransfers();
    bool found = false;
    for (uint64 i = 0; i < transfers.capacity(); ++i)
    {
        if (transfers.get(i).destination == ENTITY1 && transfers.get(i).amount == 10000)
        {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found);
}

TEST(ContractCCF, SubscriptionMaxEpochsValidation)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Try to create subscription that exceeds max epochs (52)
    // Monthly subscription with 14 periods = 14 * 4 = 56 epochs > 52
    CCF::SetProposal_input input;
    setMemory(input, 0);
    input.proposal.epoch = system.epoch;
    input.proposal.type = ProposalTypes::TransferYesNo;
    input.proposal.transfer.destination = ENTITY1;
    input.proposal.transfer.amount = 1000;
    input.isSubscription = true;
    input.periodType = CCF_SUBSCRIPTION_PERIOD_MONTH;
    input.numberOfPeriods = 14; // 14 * 4 = 56 epochs > 52 max
    input.startEpoch = system.epoch + 1;
    input.amountPerPeriod = 1000;
    
    auto output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Try with valid number (12 months = 48 epochs < 52)
    input.numberOfPeriods = 12;
    output = test.setProposal(PROPOSER1, input);
    EXPECT_NE((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
}

TEST(ContractCCF, SubscriptionExpiration)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Create weekly subscription with 3 periods
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 500, 3, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    
    test.voteMultipleComputors(proposalIndex, 200, 350);
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    sint64 initialBalance = getBalance(ENTITY1);

    // Activate and process payments
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Process first payment (epoch 190)
    system.epoch = 190;
    test.beginEpoch();
    test.endEpoch();
    
    // Process second payment (epoch 191)
    system.epoch = 191;
    test.beginEpoch();
    test.endEpoch();
    
    sint64 balanceAfter3Payments = getBalance(ENTITY1);
    EXPECT_GE(balanceAfter3Payments, initialBalance + 500 + 500 + 500);
    
    // Move to epoch 192 - subscription should be expired, no more payments
    system.epoch = 192;
    test.beginEpoch();
    test.endEpoch();
    
    sint64 balanceAfterExpiration = getBalance(ENTITY1);
    EXPECT_EQ(balanceAfterExpiration, balanceAfter3Payments); // No new payment
}

TEST(ContractCCF, GetProposalIndices)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    // Create multiple proposals
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    id PROPOSER2 = broadcastedComputors.computors.publicKeys[1];
    increaseEnergy(PROPOSER2, 1000000);
    uint32 proposalIndex1 = test.setupRegularProposal(PROPOSER1, ENTITY1, 1000);
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);
    uint32 proposalIndex2 = test.setupRegularProposal(PROPOSER2, ENTITY2, 2000);
    EXPECT_NE((int)proposalIndex2, (int)INVALID_PROPOSAL_INDEX);
    
    auto output = test.getProposalIndices(true, -1);
    
    EXPECT_GE((int)output.numOfIndices, 2);
    bool found1 = false, found2 = false;
    for (uint32 i = 0; i < output.numOfIndices; ++i)
    {
        if (output.indices.get(i) == proposalIndex1)
            found1 = true;
        if (output.indices.get(i) == proposalIndex2)
            found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST(ContractCCF, SubscriptionSlotReuse)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Create and cancel a subscription
    uint32 proposalIndex1 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);
    
    // Cancel it
    increaseEnergy(PROPOSER1, 1000000);

    CCF::SetProposal_input cancelInput;
    setMemory(cancelInput, 0);
    cancelInput.proposal.epoch = 0;
    cancelInput.proposal.type = ProposalTypes::TransferYesNo;
    cancelInput.proposal.transfer.destination = ENTITY1;
    cancelInput.proposal.transfer.amount = 1000;
    cancelInput.periodType = CCF_SUBSCRIPTION_PERIOD_WEEK;
    cancelInput.numberOfPeriods = 4;
    cancelInput.startEpoch = 189;
    cancelInput.amountPerPeriod = 1000;
    cancelInput.isSubscription = true;
    auto cancelOutput = test.setProposal(PROPOSER1, cancelInput);
    EXPECT_NE((int)cancelOutput.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Create a new subscription - should reuse the slot
    increaseEnergy(PROPOSER1, 1000000);

    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY2, 2000, 4, CCF_SUBSCRIPTION_PERIOD_WEEK, 189);
    EXPECT_NE((int)proposalIndex2, (int)INVALID_PROPOSAL_INDEX);
    
    // Check that only one subscription exists for this proposer
    auto state = test.getState();
    EXPECT_EQ(state->countSubscriptionsByProposer(PROPOSER1), 1u);
    EXPECT_EQ(state->getSubscriptionAmountPerPeriod(PROPOSER1), 2000);
}
