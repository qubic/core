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
            std::cout << "Active Subscriptions (total capacity: " << activeSubscriptions.capacity() << "):" << std::endl;
            for (uint64 i = 0; i < activeSubscriptions.capacity(); ++i)
            {
                const SubscriptionData& sub = activeSubscriptions.get(i);
                if (!isZero(sub.destination))
                {
                    std::cout << "- Index " << i << ": destination=" << sub.destination 
                              << ", weeksPerPeriod=" << (int)sub.weeksPerPeriod
                              << ", numberOfPeriods=" << sub.numberOfPeriods
                              << ", amountPerPeriod=" << sub.amountPerPeriod
                              << ", startEpoch=" << sub.startEpoch
                              << ", currentPeriod=" << sub.currentPeriod << std::endl;
                }
            }
            std::cout << "Subscription Proposals (total capacity: " << subscriptionProposals.capacity() << "):" << std::endl;
            for (uint64 i = 0; i < subscriptionProposals.capacity(); ++i)
            {
                const SubscriptionProposalData& prop = subscriptionProposals.get(i);
                if (!isZero(prop.proposerId))
                {
                    std::cout << "- Index " << i << ": proposerId=" << prop.proposerId 
                              << ", destination=" << prop.destination
                              << ", weeksPerPeriod=" << (int)prop.weeksPerPeriod
                              << ", numberOfPeriods=" << prop.numberOfPeriods
                              << ", amountPerPeriod=" << prop.amountPerPeriod
                              << ", startEpoch=" << prop.startEpoch << std::endl;
                }
            }
        }
    }

    const SubscriptionData* getActiveSubscriptionByDestination(const id& destination)
    {
        for (uint64 i = 0; i < activeSubscriptions.capacity(); ++i)
        {
            const SubscriptionData& sub = activeSubscriptions.get(i);
            if (sub.destination == destination && !isZero(sub.destination))
                return &sub;
        }
        return nullptr;
    }

    // Helper to find destination from a proposer's subscription proposal
    id getDestinationByProposer(const id& proposerId)
    {
        // Use constant 128 which matches SubscriptionProposalsT capacity
        for (uint64 i = 0; i < 128; ++i)
        {
            const SubscriptionProposalData& prop = subscriptionProposals.get(i);
            if (prop.proposerId == proposerId && !isZero(prop.proposerId))
                return prop.destination;
        }
        return NULL_ID;
    }

    bool hasActiveSubscription(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr;
    }


    sint32 getSubscriptionCurrentPeriod(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr ? sub->currentPeriod : -1;
    }

    bool getSubscriptionIsActive(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr;
    }

    // Overload for backward compatibility - use proposer ID
    bool getSubscriptionIsActive(const id& proposerId, bool)
    {
        return getSubscriptionIsActiveByProposer(proposerId);
    }

    uint8 getSubscriptionWeeksPerPeriod(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr ? sub->weeksPerPeriod : 0;
    }

    uint32 getSubscriptionNumberOfPeriods(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr ? sub->numberOfPeriods : 0;
    }

    sint64 getSubscriptionAmountPerPeriod(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr ? sub->amountPerPeriod : 0;
    }

    uint32 getSubscriptionStartEpoch(const id& destination)
    {
        const SubscriptionData* sub = getActiveSubscriptionByDestination(destination);
        return sub != nullptr ? sub->startEpoch : 0;
    }

    uint32 countActiveSubscriptions()
    {
        uint32 count = 0;
        for (uint64 i = 0; i < activeSubscriptions.capacity(); ++i)
        {
            if (!isZero(activeSubscriptions.get(i).destination))
                count++;
        }
        return count;
    }

    // Helper function to check if proposer has a subscription proposal
    bool hasSubscriptionProposal(const id& proposerId)
    {
        // Use constant 128 which matches SubscriptionProposalsT capacity
        for (uint64 i = 0; i < 128; ++i)
        {
            const SubscriptionProposalData& prop = subscriptionProposals.get(i);
            if (prop.proposerId == proposerId && !isZero(prop.proposerId))
                return true;
        }
        return false;
    }

    // Helper function for backward compatibility - finds destination from proposer's proposal and checks active subscription
    bool hasActiveSubscriptionByProposer(const id& proposerId)
    {
        id destination = getDestinationByProposer(proposerId);
        if (isZero(destination))
            return false;
        return hasActiveSubscription(destination);
    }

    // Helper function that checks both subscription proposals and active subscriptions by proposer
    bool hasSubscription(const id& proposerId)
    {
        return hasSubscriptionProposal(proposerId) || hasActiveSubscriptionByProposer(proposerId);
    }

    // Helper functions that work with proposer ID (for backward compatibility with tests)
    bool getSubscriptionIsActiveByProposer(const id& proposerId)
    {
        return hasActiveSubscriptionByProposer(proposerId);
    }

    // Helper to get subscription proposal data by proposer ID
    const SubscriptionProposalData* getSubscriptionProposalByProposer(const id& proposerId)
    {
        // Use constant 128 which matches SubscriptionProposalsT capacity
        for (uint64 i = 0; i < 128; ++i)
        {
            const SubscriptionProposalData& prop = subscriptionProposals.get(i);
            if (prop.proposerId == proposerId && !isZero(prop.proposerId))
                return &prop;
        }
        return nullptr;
    }

    uint8 getSubscriptionWeeksPerPeriodByProposer(const id& proposerId)
    {
        // First check subscription proposal
        const SubscriptionProposalData* prop = getSubscriptionProposalByProposer(proposerId);
        if (prop != nullptr)
            return prop->weeksPerPeriod;
        
        // Then check active subscription
        id destination = getDestinationByProposer(proposerId);
        if (!isZero(destination))
            return getSubscriptionWeeksPerPeriod(destination);
        
        return 0;
    }

    uint32 getSubscriptionNumberOfPeriodsByProposer(const id& proposerId)
    {
        // First check subscription proposal
        const SubscriptionProposalData* prop = getSubscriptionProposalByProposer(proposerId);
        if (prop != nullptr)
            return prop->numberOfPeriods;
        
        // Then check active subscription
        id destination = getDestinationByProposer(proposerId);
        if (!isZero(destination))
            return getSubscriptionNumberOfPeriods(destination);
        
        return 0;
    }

    sint64 getSubscriptionAmountPerPeriodByProposer(const id& proposerId)
    {
        // First check subscription proposal
        const SubscriptionProposalData* prop = getSubscriptionProposalByProposer(proposerId);
        if (prop != nullptr)
            return prop->amountPerPeriod;
        
        // Then check active subscription
        id destination = getDestinationByProposer(proposerId);
        if (!isZero(destination))
            return getSubscriptionAmountPerPeriod(destination);
        
        return 0;
    }

    uint32 getSubscriptionStartEpochByProposer(const id& proposerId)
    {
        // First check subscription proposal
        const SubscriptionProposalData* prop = getSubscriptionProposalByProposer(proposerId);
        if (prop != nullptr)
            return prop->startEpoch;
        
        // Then check active subscription
        id destination = getDestinationByProposer(proposerId);
        if (!isZero(destination))
            return getSubscriptionStartEpoch(destination);
        
        return 0;
    }

    sint32 getSubscriptionCurrentPeriodByProposer(const id& proposerId)
    {
        // Only check active subscription (currentPeriod doesn't exist in proposals)
        id destination = getDestinationByProposer(proposerId);
        if (isZero(destination))
            return -1; // No active subscription yet
        return getSubscriptionCurrentPeriod(destination);
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

    CCF::GetProposal_output getProposal(uint32 proposalIndex, const id& subscriptionDestination = NULL_ID)
    {
        CCF::GetProposal_input input;
        input.proposalIndex = (uint16)proposalIndex;
        input.subscriptionDestination = subscriptionDestination;
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
                                      uint32 numberOfPeriods, uint8 weeksPerPeriod, uint32 startEpoch, bool expectSuccess = true)
    {
        CCF::SetProposal_input input;
        setMemory(input, 0);
        input.proposal.epoch = system.epoch;
        input.proposal.type = ProposalTypes::TransferYesNo;
        input.proposal.transfer.destination = destination;
        input.proposal.transfer.amount = amountPerPeriod;
        input.isSubscription = true;
        input.weeksPerPeriod = weeksPerPeriod;
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
        PROPOSER1, ENTITY1, 1000, 12, 4, system.epoch + 1); // 4 weeks per period (monthly)
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Check that subscription proposal was stored
    auto state = test.getState();
    EXPECT_TRUE(state->hasSubscription(PROPOSER1));
    EXPECT_FALSE(state->getSubscriptionIsActiveByProposer(PROPOSER1)); // Not active until accepted
    EXPECT_EQ(state->getSubscriptionWeeksPerPeriodByProposer(PROPOSER1), 4u);
    EXPECT_EQ(state->getSubscriptionNumberOfPeriodsByProposer(PROPOSER1), 12u);
    EXPECT_EQ(state->getSubscriptionAmountPerPeriodByProposer(PROPOSER1), 1000);
    EXPECT_EQ(state->getSubscriptionStartEpochByProposer(PROPOSER1), system.epoch + 1);
    EXPECT_EQ(state->getSubscriptionCurrentPeriodByProposer(PROPOSER1), -1);
    
    // Get proposal with subscription data
    auto proposal = test.getProposal(proposalIndex, NULL_ID);
    EXPECT_TRUE(proposal.okay);
    EXPECT_TRUE(proposal.hasSubscriptionProposal);
    EXPECT_FALSE(isZero(proposal.subscriptionProposal.proposerId));
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
        PROPOSER1, ENTITY1, 1000, 4, 1, system.epoch + 1); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
        
    // Vote to approve
    test.voteMultipleComputors(proposalIndex, 200, 350);
    
    // End epoch
    test.endEpoch();
    
    auto state = test.getState();
    
    // Check subscription is now active (identified by destination)
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    EXPECT_TRUE(state->getSubscriptionIsActive(ENTITY1));
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
        PROPOSER1, ENTITY1, 500, 4, 1, 189); // 1 week per period (weekly)
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
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(ENTITY1), 1);
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
        PROPOSER1, ENTITY1, 1000, 3, 4, 189); // 4 weeks per period (monthly)
    
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
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(ENTITY1), -1);
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
        PROPOSER1, ENTITY1, 1000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);

    increaseEnergy(PROPOSER1, 1000000);
    // Try to create second subscription for same proposer - should overwrite the previous one
    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY2, 2000, 4, 1, 189, true); // 1 week per period (weekly)
    EXPECT_EQ((int)proposalIndex2, (int)proposalIndex1);
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
        PROPOSER1, ENTITY1, 1000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    // Cancel proposal (epoch = 0)
    CCF::SetProposal_input cancelInput;
    setMemory(cancelInput, 0);
    cancelInput.proposal.epoch = 0;
    cancelInput.proposal.type = ProposalTypes::TransferYesNo;
    cancelInput.isSubscription = true;
    cancelInput.weeksPerPeriod = 1; // 1 week per period (weekly)
    cancelInput.numberOfPeriods = 4;
    cancelInput.startEpoch = 189;
    cancelInput.amountPerPeriod = 1000;
    auto cancelOutput = test.setProposal(PROPOSER1, cancelInput);
    EXPECT_NE((int)cancelOutput.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Check subscription was deactivated
    auto state = test.getState();
    EXPECT_FALSE(state->hasSubscriptionProposal(PROPOSER1));    // proposal is canceled, so no subscription proposal
}

TEST(ContractCCF, SubscriptionValidation)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Test invalid weeksPerPeriod (must be > 0)
    CCF::SetProposal_input input;
    setMemory(input, 0);
    input.proposal.epoch = system.epoch;
    input.proposal.type = ProposalTypes::TransferYesNo;
    input.proposal.transfer.destination = ENTITY1;
    input.proposal.transfer.amount = 1000;
    input.isSubscription = true;
    input.weeksPerPeriod = 0; // Invalid (must be > 0)
    input.numberOfPeriods = 4;
    input.startEpoch = system.epoch;
    input.amountPerPeriod = 1000;
    
    auto output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);

    // Test start epoch in past
    increaseEnergy(PROPOSER1, 1000000);
    input.weeksPerPeriod = 1; // 1 week per period (weekly)
    input.startEpoch = system.epoch - 1; // Should be >= current epoch
    output = test.setProposal(PROPOSER1, input);
    EXPECT_EQ((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Test that zero numberOfPeriods is allowed (will cancel subscription when accepted)
    increaseEnergy(PROPOSER1, 1000000);
    input.weeksPerPeriod = 1;
    input.startEpoch = system.epoch;
    input.numberOfPeriods = 0; // Allowed - will cancel subscription
    input.amountPerPeriod = 1000;
    output = test.setProposal(PROPOSER1, input);
    EXPECT_NE((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Test that zero amountPerPeriod is allowed (will cancel subscription when accepted)
    increaseEnergy(PROPOSER1, 1000000);
    input.numberOfPeriods = 4;
    input.amountPerPeriod = 0; // Allowed - will cancel subscription
    output = test.setProposal(PROPOSER1, input);
    EXPECT_NE((int)output.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
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
        PROPOSER1, ENTITY1, 1000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);
    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER2, ENTITY2, 2000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex2, (int)INVALID_PROPOSAL_INDEX);

    // Both proposals need to first be voted in before the subscriptions become active.
    auto state = test.getState();
    EXPECT_FALSE(state->hasActiveSubscription(ENTITY1));
    EXPECT_FALSE(state->hasActiveSubscription(ENTITY2));
    EXPECT_EQ(state->countActiveSubscriptions(), 0u);

    // Vote in both subscription proposals to activate them
    test.voteMultipleComputors(proposalIndex1, 200, 400);
    test.voteMultipleComputors(proposalIndex2, 200, 400);

    // Increase energy so contract can execute the subscriptions
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 10000000);
    test.endEpoch();

    // Now both should be active subscriptions (by destination)
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY2));
    EXPECT_EQ(state->countActiveSubscriptions(), 2u);
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
    input.weeksPerPeriod = 4; // 4 weeks per period (monthly)
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
        PROPOSER1, ENTITY1, 500, 3, 1, 189); // 1 week per period (weekly)
    
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
        PROPOSER1, ENTITY1, 1000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);
    
    // Cancel it
    increaseEnergy(PROPOSER1, 1000000);

    CCF::SetProposal_input cancelInput;
    setMemory(cancelInput, 0);
    cancelInput.proposal.epoch = 0;
    cancelInput.proposal.type = ProposalTypes::TransferYesNo;
    cancelInput.proposal.transfer.destination = ENTITY1;
    cancelInput.proposal.transfer.amount = 1000;
    cancelInput.weeksPerPeriod = 1; // 1 week per period (weekly)
    cancelInput.numberOfPeriods = 4;
    cancelInput.startEpoch = 189;
    cancelInput.amountPerPeriod = 1000;
    cancelInput.isSubscription = true;
    auto cancelOutput = test.setProposal(PROPOSER1, cancelInput);
    EXPECT_NE((int)cancelOutput.proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Create a new subscription - should reuse the slot
    increaseEnergy(PROPOSER1, 1000000);

    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY2, 2000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_EQ((int)proposalIndex2, (int)proposalIndex1);
    
    // Vote in the new subscription proposal to activate it
    test.voteMultipleComputors(proposalIndex2, 200, 400);
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Check that subscription was updated (identified by destination)
    auto state = test.getState();
    EXPECT_EQ(state->countActiveSubscriptions(), 1u);
    EXPECT_EQ(state->getSubscriptionAmountPerPeriod(ENTITY2), 2000); // New subscription for ENTITY2
}

TEST(ContractCCF, CancelSubscriptionByZeroAmount)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Create and activate a subscription
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote to approve
    test.voteMultipleComputors(proposalIndex, 200, 350);
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Move to start epoch to activate
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Verify subscription is active
    auto state = test.getState();
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    EXPECT_EQ(state->getSubscriptionAmountPerPeriod(ENTITY1), 1000);
    
    // Propose cancellation by setting amountPerPeriod to 0
    increaseEnergy(PROPOSER1, 1000000);
    CCF::SetProposal_input cancelInput;
    setMemory(cancelInput, 0);
    cancelInput.proposal.epoch = system.epoch;
    cancelInput.proposal.type = ProposalTypes::TransferYesNo;
    cancelInput.proposal.transfer.destination = ENTITY1;
    cancelInput.proposal.transfer.amount = 0;
    cancelInput.isSubscription = true;
    cancelInput.weeksPerPeriod = 1;
    cancelInput.numberOfPeriods = 4;
    cancelInput.startEpoch = system.epoch + 1;
    cancelInput.amountPerPeriod = 0; // Zero amount will cancel subscription
    
    uint32 cancelProposalIndex = test.setProposal(PROPOSER1, cancelInput).proposalIndex;
    EXPECT_NE((int)cancelProposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote to approve cancellation
    test.voteMultipleComputors(cancelProposalIndex, 200, 350);
    // Increase energy for contract
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Verify subscription was deleted
    state = test.getState();
    EXPECT_FALSE(state->hasActiveSubscription(ENTITY1));
    EXPECT_EQ(state->countActiveSubscriptions(), 0u);
}

TEST(ContractCCF, CancelSubscriptionByZeroPeriods)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Create and activate a subscription
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, 1, 189); // 1 week per period (weekly)
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote to approve
    test.voteMultipleComputors(proposalIndex, 200, 350);
    // Increase energy for contract to pay for the proposal
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Move to start epoch to activate
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Verify subscription is active
    auto state = test.getState();
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    
    // Propose cancellation by setting numberOfPeriods to 0
    increaseEnergy(PROPOSER1, 1000000);
    CCF::SetProposal_input cancelInput;
    setMemory(cancelInput, 0);
    cancelInput.proposal.epoch = system.epoch;
    cancelInput.proposal.type = ProposalTypes::TransferYesNo;
    cancelInput.proposal.transfer.destination = ENTITY1;
    cancelInput.proposal.transfer.amount = 0;
    cancelInput.isSubscription = true;
    cancelInput.weeksPerPeriod = 1;
    cancelInput.numberOfPeriods = 0; // Zero periods will cancel subscription
    cancelInput.startEpoch = system.epoch + 1;
    cancelInput.amountPerPeriod = 1000;
    
    uint32 cancelProposalIndex = test.setProposal(PROPOSER1, cancelInput).proposalIndex;
    EXPECT_NE((int)cancelProposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote to approve cancellation
    test.voteMultipleComputors(cancelProposalIndex, 200, 350);
    // Increase energy for contract
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    // Verify subscription was deleted
    state = test.getState();
    EXPECT_FALSE(state->hasActiveSubscription(ENTITY1));
    EXPECT_EQ(state->countActiveSubscriptions(), 0u);
}

TEST(ContractCCF, SubscriptionWithDifferentWeeksPerPeriod)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    
    // Create subscription with 2 weeks per period
    uint32 proposalIndex = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 6, 2, 189); // 2 weeks per period, 6 periods
    EXPECT_NE((int)proposalIndex, (int)INVALID_PROPOSAL_INDEX);
    
    // Verify proposal data
    auto state = test.getState();
    EXPECT_EQ(state->getSubscriptionWeeksPerPeriodByProposer(PROPOSER1), 2u);
    EXPECT_EQ(state->getSubscriptionNumberOfPeriodsByProposer(PROPOSER1), 6u);
    
    // Vote to approve
    test.voteMultipleComputors(proposalIndex, 200, 350);
    // Increase energy for contract
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();

    // Still period -1, no payment yet
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(ENTITY1), -1);
    
    // Move to start epoch
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();

    // period 0, it is the first payment period.
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(ENTITY1), 0);
    
    // Verify subscription is active with correct weeksPerPeriod
    state = test.getState();
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    EXPECT_EQ(state->getSubscriptionWeeksPerPeriod(ENTITY1), 2u);
    EXPECT_EQ(state->getSubscriptionNumberOfPeriods(ENTITY1), 6u);
    
    system.epoch = 190;
    test.beginEpoch();
    test.endEpoch();
    
    system.epoch = 191;
    test.beginEpoch();
    test.endEpoch();
    
    // period 1, it is the second payment period.
    EXPECT_EQ(state->getSubscriptionCurrentPeriod(ENTITY1), 1);
    
    sint64 balance = getBalance(ENTITY1);
    EXPECT_GE(balance, 1000); // Payment was made
}

TEST(ContractCCF, SubscriptionOverwriteByDestination)
{
    ContractTestingCCF test;
    system.epoch = 188;
    test.beginEpoch();
    
    id PROPOSER1 = broadcastedComputors.computors.publicKeys[0];
    increaseEnergy(PROPOSER1, 1000000);
    id PROPOSER2 = broadcastedComputors.computors.publicKeys[1];
    increaseEnergy(PROPOSER2, 1000000);
    
    // PROPOSER1 creates subscription for ENTITY1
    uint32 proposalIndex1 = test.setupSubscriptionProposal(
        PROPOSER1, ENTITY1, 1000, 4, 1, 189);
    EXPECT_NE((int)proposalIndex1, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote and activate
    test.voteMultipleComputors(proposalIndex1, 200, 350);
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    system.epoch = 189;
    test.beginEpoch();
    test.endEpoch();
    
    // Verify first subscription is active
    auto state = test.getState();
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    EXPECT_EQ(state->getSubscriptionAmountPerPeriod(ENTITY1), 1000);
    
    // PROPOSER2 creates a new subscription proposal for the same destination
    // This should overwrite the existing subscription when accepted
    uint32 proposalIndex2 = test.setupSubscriptionProposal(
        PROPOSER2, ENTITY1, 2000, 6, 2, system.epoch + 1); // Different amount and schedule
    EXPECT_NE((int)proposalIndex2, (int)INVALID_PROPOSAL_INDEX);
    
    // Vote and activate the new subscription
    test.voteMultipleComputors(proposalIndex2, 200, 350);
    increaseEnergy(id(CCF_CONTRACT_INDEX, 0, 0, 0), 1000000);
    test.endEpoch();
    
    system.epoch = 190;
    test.beginEpoch();
    test.endEpoch();
    
    // Verify the subscription was overwritten
    state = test.getState();
    EXPECT_TRUE(state->hasActiveSubscription(ENTITY1));
    EXPECT_EQ(state->getSubscriptionAmountPerPeriod(ENTITY1), 2000); // New amount
    EXPECT_EQ(state->getSubscriptionWeeksPerPeriod(ENTITY1), 2u); // New schedule
    EXPECT_EQ(state->getSubscriptionNumberOfPeriods(ENTITY1), 6u); // New number of periods
    EXPECT_EQ(state->countActiveSubscriptions(), 1u); // Still only one subscription per destination
}
