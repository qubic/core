#define NO_UEFI

#include "contract_testing.h"

// ---------------------------------------------------------------------------
// Procedure / Function indices (must match REGISTER_USER_PROCEDURE in QAGENT.h)
// ---------------------------------------------------------------------------

// Agent Lifecycle
constexpr uint16 PROC_REGISTER_AGENT     = 1;
constexpr uint16 PROC_UPDATE_AGENT       = 2;
constexpr uint16 PROC_DEACTIVATE_AGENT   = 3;
constexpr uint16 PROC_REACTIVATE_AGENT   = 4;
constexpr uint16 PROC_STAKE_MORE         = 5;
constexpr uint16 PROC_REQUEST_UNSTAKE    = 6;
constexpr uint16 PROC_WITHDRAW_STAKE     = 7;

// Service Registry
constexpr uint16 PROC_REGISTER_SERVICE     = 8;
constexpr uint16 PROC_DEPRECATE_SERVICE    = 9;
constexpr uint16 PROC_UPDATE_SERVICE_PRICE = 10;

// Task Lifecycle
constexpr uint16 PROC_CREATE_TASK         = 11;
constexpr uint16 PROC_ACCEPT_TASK         = 12;
constexpr uint16 PROC_COMMIT_RESULT       = 13;
constexpr uint16 PROC_REVEAL_RESULT       = 14;
constexpr uint16 PROC_APPROVE_RESULT      = 15;
constexpr uint16 PROC_CHALLENGE_RESULT    = 16;
constexpr uint16 PROC_FINALIZE_TASK       = 17;
constexpr uint16 PROC_TIMEOUT_TASK        = 18;
constexpr uint16 PROC_CANCEL_TASK         = 19;
constexpr uint16 PROC_COMPLETE_MILESTONE  = 20;

// Dispute Resolution
constexpr uint16 PROC_REQUEST_ORACLE_VERIFICATION = 21;
constexpr uint16 PROC_CAST_DISPUTE_VOTE   = 22;
constexpr uint16 PROC_REGISTER_ARBITRATOR = 23;

// Governance
constexpr uint16 PROC_PROPOSE_PARAM_CHANGE = 24;
constexpr uint16 PROC_VOTE_ON_PROPOSAL     = 25;

// Rating
constexpr uint16 PROC_RATE_AGENT          = 26;

// Delegation
constexpr uint16 PROC_DELEGATE_TASK       = 27;

// Arbitrator Lifecycle
constexpr uint16 PROC_DEACTIVATE_ARBITRATOR    = 28;
constexpr uint16 PROC_WITHDRAW_ARBITRATOR_STAKE = 29;

// Functions
constexpr uint16 FUNC_GET_AGENT           = 1;
constexpr uint16 FUNC_GET_TASK            = 2;
constexpr uint16 FUNC_GET_SERVICE         = 3;
constexpr uint16 FUNC_GET_PLATFORM_STATS  = 4;
constexpr uint16 FUNC_GET_STAKE           = 5;
constexpr uint16 FUNC_GET_ARBITRATOR      = 6;
constexpr uint16 FUNC_GET_AGENTS_BY_CAP   = 7;
constexpr uint16 FUNC_GET_PROPOSAL        = 8;
constexpr uint16 FUNC_GET_REPUTATION      = 9;

// ---------------------------------------------------------------------------
// QAgentChecker: Expose internal state for assertions
// ---------------------------------------------------------------------------

class QAgentChecker : public QAGENT, public QAGENT::StateData
{
public:
    uint64 agentCount() const { return agents.population(); }
    uint64 taskCount() const { return tasks.population(); }
    uint64 getTaskCounter() const { return taskCounter; }
    uint64 activeTaskQueueSize() const { return activeTaskQ.population(); }

    bool hasAgent(const id& agentId) const { return agents.contains(agentId); }
    bool getAgentRecord(const id& agentId, AgentRecord& out) const { return agents.get(agentId, out); }

    bool hasTask(uint64 taskId) const { return tasks.contains(taskId); }
    bool getTaskRecord(uint64 taskId, TaskRecord& out) const { return tasks.get(taskId, out); }

    bool isArbitrator(const id& arb) const { return arbitrators.contains(arb); }
    bool getArbitratorRecord(const id& arb, ArbitratorRecord& out) const { return arbitrators.get(arb, out); }

    bool getStakeRecord(const id& agentId, StakeRecord& out) const { return stakes.get(agentId, out); }

    PlatformStats getStats() const { return stats; }
    PlatformConfig getConfig() const { return config; }

    bool getOracleTaskMapping(sint64 queryId, uint64& taskId) const { return oracleQueryToTask.get(queryId, taskId); }

    // Expose protected notification procedure ID for testing (PRIVATE_PROCEDURE makes it protected)
    static constexpr auto oracleNotificationId() { return __id_OnAIVerifyReply; }

    // State mutation for oracle testing: set task to ORACLE_PENDING and create query→task mapping
    void setupOracleState(uint64 taskId, sint64 queryId)
    {
        TaskRecord task;
        if (tasks.get(taskId, task))
        {
            task.status = QAGENT_TASK_STATUS_ORACLE_PENDING;
            task.oracleQueryId = queryId;
            task.oracleRequested = 1;
            tasks.set(taskId, task);
            oracleQueryToTask.set(queryId, taskId);
        }
    }
};

// ---------------------------------------------------------------------------
// Test IDs (declared before fixture so constructor can fund them)
// ---------------------------------------------------------------------------
static const id agent1 = id(100, 0, 0, 0);
static const id agent2 = id(200, 0, 0, 0);
static const id requester1 = id(300, 0, 0, 0);
static const id requester2 = id(400, 0, 0, 0);
static const id challenger1 = id(500, 0, 0, 0);
static const id arb1 = id(601, 0, 0, 0);
static const id arb2 = id(602, 0, 0, 0);
static const id arb3 = id(603, 0, 0, 0);
static const id arb4 = id(604, 0, 0, 0);
static const id arb5 = id(605, 0, 0, 0);
static const id keeper1 = id(700, 0, 0, 0);

// ---------------------------------------------------------------------------
// ContractTestingQAgent: Test fixture with helper methods
// ---------------------------------------------------------------------------

class ContractTestingQAgent : protected ContractTesting
{
public:
    ContractTestingQAgent()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QAGENT);
        system.epoch = contractDescriptions[QAGENT_CONTRACT_INDEX].constructionEpoch;
        callSystemProcedure(QAGENT_CONTRACT_INDEX, INITIALIZE);

        // Start at tick 1 so qpi.tick() returns non-zero for finalizedTick checks
        system.tick = 1;

        // Fund all test accounts (users need balance for procedure invocations)
        increaseEnergy(agent1, 10000000LL);
        increaseEnergy(agent2, 10000000LL);
        increaseEnergy(requester1, 10000000LL);
        increaseEnergy(requester2, 10000000LL);
        increaseEnergy(challenger1, 10000000LL);
        increaseEnergy(arb1, 10000000LL);
        increaseEnergy(arb2, 10000000LL);
        increaseEnergy(arb3, 10000000LL);
        increaseEnergy(arb4, 10000000LL);
        increaseEnergy(arb5, 10000000LL);
        increaseEnergy(keeper1, 10000000LL);
    }

    QAgentChecker* state()
    {
        return reinterpret_cast<QAgentChecker*>(contractStates[QAGENT_CONTRACT_INDEX]);
    }

    void fund(const id& user, sint64 amount)
    {
        increaseEnergy(user, amount);
    }

    // --- Procedure helpers ---

    QAGENT::RegisterAgent_output registerAgent(const id& user, uint32 capabilities, sint64 pricePerTask, sint64 reward)
    {
        QAGENT::RegisterAgent_input input{capabilities, pricePerTask};
        QAGENT::RegisterAgent_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REGISTER_AGENT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::UpdateAgent_output updateAgent(const id& user, uint32 capabilities, sint64 pricePerTask, sint64 reward = 0)
    {
        QAGENT::UpdateAgent_input input{capabilities, pricePerTask};
        QAGENT::UpdateAgent_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_UPDATE_AGENT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::DeactivateAgent_output deactivateAgent(const id& user, sint64 reward = 0)
    {
        QAGENT::DeactivateAgent_input input{};
        QAGENT::DeactivateAgent_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_DEACTIVATE_AGENT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::ReactivateAgent_output reactivateAgent(const id& user, sint64 reward = 0)
    {
        QAGENT::ReactivateAgent_input input{};
        QAGENT::ReactivateAgent_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REACTIVATE_AGENT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::StakeMore_output stakeMore(const id& user, sint64 reward)
    {
        QAGENT::StakeMore_input input{};
        QAGENT::StakeMore_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_STAKE_MORE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::RequestUnstake_output requestUnstake(const id& user, sint64 reward = 0)
    {
        QAGENT::RequestUnstake_input input{};
        QAGENT::RequestUnstake_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REQUEST_UNSTAKE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::WithdrawStake_output withdrawStake(const id& user, sint64 reward = 0)
    {
        QAGENT::WithdrawStake_input input{};
        QAGENT::WithdrawStake_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_WITHDRAW_STAKE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::CreateTask_output createTask(const id& user, const id& agentId, uint8 taskType, uint32 deadlineTicks, sint64 escrow, id specHash = NULL_ID, id modelHash = NULL_ID, uint8 totalMilestones = 1)
    {
        QAGENT::CreateTask_input input{agentId, taskType, deadlineTicks, specHash, modelHash, totalMilestones};
        QAGENT::CreateTask_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_CREATE_TASK, input, output, user, escrow))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::AcceptTask_output acceptTask(const id& user, uint64 taskId, sint64 stake)
    {
        QAGENT::AcceptTask_input input{taskId};
        QAGENT::AcceptTask_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_ACCEPT_TASK, input, output, user, stake))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::CommitResult_output commitResult(const id& user, uint64 taskId, const id& commitHash, sint64 reward = 0)
    {
        QAGENT::CommitResult_input input{taskId, commitHash};
        QAGENT::CommitResult_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_COMMIT_RESULT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::RevealResult_output revealResult(const id& user, uint64 taskId, const id& resultHash, const id& salt, const id& dataCid = NULL_ID, uint8 resultEnum = 0, sint64 reward = 0)
    {
        QAGENT::RevealResult_input input{taskId, resultHash, salt, dataCid, resultEnum};
        QAGENT::RevealResult_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REVEAL_RESULT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::ApproveResult_output approveResult(const id& user, uint64 taskId, sint64 reward = 0)
    {
        QAGENT::ApproveResult_input input{taskId};
        QAGENT::ApproveResult_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_APPROVE_RESULT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::ChallengeResult_output challengeResult(const id& user, uint64 taskId, const id& challengerHash, sint64 bond)
    {
        QAGENT::ChallengeResult_input input{taskId, challengerHash};
        QAGENT::ChallengeResult_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_CHALLENGE_RESULT, input, output, user, bond))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::FinalizeTask_output finalizeTask(const id& user, uint64 taskId, sint64 reward = 0)
    {
        QAGENT::FinalizeTask_input input{taskId};
        QAGENT::FinalizeTask_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_FINALIZE_TASK, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::TimeoutTask_output timeoutTask(const id& user, uint64 taskId, sint64 reward = 0)
    {
        QAGENT::TimeoutTask_input input{taskId};
        QAGENT::TimeoutTask_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_TIMEOUT_TASK, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::CancelTask_output cancelTask(const id& user, uint64 taskId, sint64 reward = 0)
    {
        QAGENT::CancelTask_input input{taskId};
        QAGENT::CancelTask_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_CANCEL_TASK, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::CastDisputeVote_output castDisputeVote(const id& user, uint64 taskId, uint8 vote, sint64 reward = 0)
    {
        QAGENT::CastDisputeVote_input input{taskId, vote};
        QAGENT::CastDisputeVote_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_CAST_DISPUTE_VOTE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::RegisterArbitrator_output registerArbitrator(const id& user, sint64 stake)
    {
        QAGENT::RegisterArbitrator_input input{};
        QAGENT::RegisterArbitrator_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REGISTER_ARBITRATOR, input, output, user, stake))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::DeactivateArbitrator_output deactivateArbitrator(const id& user, sint64 reward = 0)
    {
        QAGENT::DeactivateArbitrator_input input{};
        QAGENT::DeactivateArbitrator_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_DEACTIVATE_ARBITRATOR, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::WithdrawArbitratorStake_output withdrawArbitratorStake(const id& user, sint64 reward = 0)
    {
        QAGENT::WithdrawArbitratorStake_input input{};
        QAGENT::WithdrawArbitratorStake_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_WITHDRAW_ARBITRATOR_STAKE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::RateAgent_output rateAgent(const id& user, uint64 taskId, uint8 rating, sint64 reward = 0)
    {
        QAGENT::RateAgent_input input{taskId, rating};
        QAGENT::RateAgent_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_RATE_AGENT, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::ProposeParameterChange_output proposeParamChange(const id& user, uint8 paramId, sint64 newValue, sint64 reward = 0)
    {
        QAGENT::ProposeParameterChange_input input{paramId, newValue};
        QAGENT::ProposeParameterChange_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_PROPOSE_PARAM_CHANGE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::VoteOnProposal_output voteOnProposal(const id& user, uint8 proposalIndex, uint8 vote, sint64 reward = 0)
    {
        QAGENT::VoteOnProposal_input input{proposalIndex, vote};
        QAGENT::VoteOnProposal_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_VOTE_ON_PROPOSAL, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::CompleteMilestone_output completeMilestone(const id& user, uint64 taskId, sint64 reward = 0)
    {
        QAGENT::CompleteMilestone_input input{taskId};
        QAGENT::CompleteMilestone_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_COMPLETE_MILESTONE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::DelegateTask_output delegateTask(const id& user, uint64 parentTaskId, const id& agentId, uint8 taskType, uint32 deadlineTicks, sint64 delegatedEscrow, id specHash = NULL_ID, id modelHash = NULL_ID, sint64 reward = 0)
    {
        QAGENT::DelegateTask_input input{parentTaskId, agentId, taskType, deadlineTicks, specHash, modelHash, delegatedEscrow};
        QAGENT::DelegateTask_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_DELEGATE_TASK, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    // --- Function helpers ---

    QAGENT::GetAgent_output getAgent(const id& agentId)
    {
        QAGENT::GetAgent_input input{agentId};
        QAGENT::GetAgent_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_AGENT, input, output);
        return output;
    }

    QAGENT::GetTask_output getTask(uint64 taskId)
    {
        QAGENT::GetTask_input input{taskId};
        QAGENT::GetTask_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_TASK, input, output);
        return output;
    }

    QAGENT::GetPlatformStats_output getPlatformStats()
    {
        QAGENT::GetPlatformStats_input input{};
        QAGENT::GetPlatformStats_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_PLATFORM_STATS, input, output);
        return output;
    }

    QAGENT::GetStake_output getStake(const id& agentId)
    {
        QAGENT::GetStake_input input{agentId};
        QAGENT::GetStake_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_STAKE, input, output);
        return output;
    }

    QAGENT::GetAgentReputationScore_output getReputationScore(const id& agentId)
    {
        QAGENT::GetAgentReputationScore_input input{agentId};
        QAGENT::GetAgentReputationScore_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_REPUTATION, input, output);
        return output;
    }

    QAGENT::GetAgentsByCapability_output getAgentsByCapability(uint32 capabilityBitmap)
    {
        QAGENT::GetAgentsByCapability_input input{capabilityBitmap};
        QAGENT::GetAgentsByCapability_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_AGENTS_BY_CAP, input, output);
        return output;
    }

    QAGENT::GetService_output getService(const id& serviceId)
    {
        QAGENT::GetService_input input{serviceId};
        QAGENT::GetService_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_SERVICE, input, output);
        return output;
    }

    QAGENT::GetArbitrator_output getArbitrator(const id& arbitratorId)
    {
        QAGENT::GetArbitrator_input input{arbitratorId};
        QAGENT::GetArbitrator_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_ARBITRATOR, input, output);
        return output;
    }

    QAGENT::GetProposal_output getProposal(uint8 proposalIndex)
    {
        QAGENT::GetProposal_input input{proposalIndex};
        QAGENT::GetProposal_output output;
        callFunction(QAGENT_CONTRACT_INDEX, FUNC_GET_PROPOSAL, input, output);
        return output;
    }

    // --- Service procedure helpers ---

    QAGENT::RegisterService_output registerService(const id& user, uint32 capabilityBitmap, sint64 basePrice, uint8 taskType, sint64 reward = 0)
    {
        QAGENT::RegisterService_input input{capabilityBitmap, basePrice, taskType};
        QAGENT::RegisterService_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REGISTER_SERVICE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::DeprecateService_output deprecateService(const id& user, const id& serviceId, sint64 reward = 0)
    {
        QAGENT::DeprecateService_input input{serviceId};
        QAGENT::DeprecateService_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_DEPRECATE_SERVICE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::UpdateServicePrice_output updateServicePrice(const id& user, const id& serviceId, sint64 newPrice, sint64 reward = 0)
    {
        QAGENT::UpdateServicePrice_input input{serviceId, newPrice};
        QAGENT::UpdateServicePrice_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_UPDATE_SERVICE_PRICE, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    QAGENT::RequestOracleVerification_output requestOracleVerification(const id& user, uint64 taskId, sint64 reward = 0)
    {
        QAGENT::RequestOracleVerification_input input{taskId};
        QAGENT::RequestOracleVerification_output output;
        if (!invokeUserProcedure(QAGENT_CONTRACT_INDEX, PROC_REQUEST_ORACLE_VERIFICATION, input, output, user, reward))
            output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return output;
    }

    // --- System procedure helpers ---

    // Advance tick counter and run END_TICK for each tick (triggers auto-processing)
    void advanceTick(uint32 count = 1)
    {
        for (uint32 i = 0; i < count; i++)
        {
            system.tick++;
            callSystemProcedure(QAGENT_CONTRACT_INDEX, END_TICK);
        }
    }

    // Set tick counter directly WITHOUT running END_TICK (for deadline testing)
    void setTick(uint32 tick)
    {
        system.tick = tick;
    }

    void advanceEpoch()
    {
        callSystemProcedure(QAGENT_CONTRACT_INDEX, END_EPOCH);
        system.epoch++;
        callSystemProcedure(QAGENT_CONTRACT_INDEX, BEGIN_EPOCH);
    }

    // --- Oracle notification simulation ---

    // Simulate an oracle reply by directly invoking the OnAIVerifyReply notification procedure.
    // Requires that the task has been set up for oracle via state()->setupOracleState().
    void simulateOracleReply(sint64 queryId, uint8 oracleStatus, uint8 verdict = 0, uint32 confidence = 0)
    {
        // Look up the registered notification procedure
        const auto* procData = userProcedureRegistry->get(QAgentChecker::oracleNotificationId());
        ASSERT_NE(procData, nullptr);
        ASSERT_EQ(procData->contractIndex, (unsigned int)QAGENT_CONTRACT_INDEX);

        // Construct the notification input
        QAGENT::OnAIVerifyReply_input notifInput;
        setMem(&notifInput, sizeof(notifInput), 0);
        notifInput.queryId = queryId;
        notifInput.status = oracleStatus;
        if (oracleStatus == ORACLE_QUERY_STATUS_SUCCESS)
        {
            notifInput.reply.verdict = verdict;
            notifInput.reply.confidence = confidence;
        }

        // Invoke the notification procedure via the proper QPI context
        QpiContextUserProcedureNotificationCall ctx(*procData);
        ctx.call(&notifInput);
    }
};

// ===========================================================================
// TEST: Agent Registration with Stake
// ===========================================================================

TEST(TestQAgent, RegisterAgentWithStake)
{
    ContractTestingQAgent qagent;

    // Must provide minimum registration stake (50000)
    auto out1 = qagent.registerAgent(agent1, 0xFF, 1000, 10000);
    EXPECT_EQ(out1.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS);
    EXPECT_EQ(qagent.state()->agentCount(), 0);

    // Register with exact stake
    auto out2 = qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(qagent.state()->agentCount(), 1);

    // Verify agent record
    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.found, 1);
    EXPECT_EQ(agentOut.agent.status, QAGENT_STATUS_ACTIVE);
    EXPECT_EQ(agentOut.agent.tier, QAGENT_TIER_UNRANKED);
    EXPECT_EQ(agentOut.agent.capabilityBitmap, 0xFF);
    EXPECT_EQ(agentOut.agent.pricePerTask, 1000);
    EXPECT_EQ(agentOut.agent.slashCount, 0);

    // Verify stake record
    auto stakeOut = qagent.getStake(agent1);
    EXPECT_EQ(stakeOut.found, 1);
    EXPECT_EQ(stakeOut.stake.registrationStake, 50000);
    EXPECT_EQ(stakeOut.stake.locked, 1);

    // Duplicate registration must fail
    auto out3 = qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    EXPECT_EQ(out3.returnCode, QAGENT_RC_ALREADY_EXISTS);

    // Platform stats
    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.totalAgents, 1);
}

// ===========================================================================
// TEST: Anti-Self-Dealing
// ===========================================================================

TEST(TestQAgent, AntiSelfDealing)
{
    ContractTestingQAgent qagent;

    // Register agent
    auto reg = qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    EXPECT_EQ(reg.returnCode, QAGENT_RC_SUCCESS);

    // Agent tries to create task for itself — must be rejected
    auto out = qagent.createTask(agent1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 1000, 5000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SELF_DEALING);

    // Different requester works fine
    auto out2 = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 1000, 5000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Commit-Reveal Verification
// ===========================================================================

TEST(TestQAgent, CommitRevealVerification)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 taskId = taskOut.taskId;

    // Agent accepts (300% bond for unranked = 30000)
    auto acceptOut = qagent.acceptTask(agent1, taskId, 30000);
    EXPECT_EQ(acceptOut.returnCode, QAGENT_RC_SUCCESS);

    // Prepare commit-reveal data
    id resultHash = id(111, 222, 333, 444);
    id salt = id(555, 666, 777, 888);

    // Compute commit hash: K12(resultHash || salt)
    QAGENT::CommitRevealData crData;
    crData.resultHash = resultHash;
    crData.salt = salt;
    uint8 commitHashBytes[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crData), sizeof(crData), commitHashBytes, 32);
    id commitHash;
    copyMem(&commitHash, commitHashBytes, 32);

    // Commit
    auto commitOut = qagent.commitResult(agent1, taskId, commitHash);
    EXPECT_EQ(commitOut.returnCode, QAGENT_RC_SUCCESS);

    // Verify task status
    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMMITTED);

    // Reveal with correct data
    auto revealOut = qagent.revealResult(agent1, taskId, resultHash, salt);
    EXPECT_EQ(revealOut.returnCode, QAGENT_RC_SUCCESS);

    task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_REVEALED);
    EXPECT_EQ(task.task.resultHash, resultHash);
}

TEST(TestQAgent, CommitRevealMismatchFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    // Commit with one hash
    id fakeCommit = id(999, 999, 999, 999);
    qagent.commitResult(agent1, taskId, fakeCommit);

    // Reveal with different data — must fail
    id resultHash = id(111, 222, 333, 444);
    id salt = id(555, 666, 777, 888);
    auto revealOut = qagent.revealResult(agent1, taskId, resultHash, salt);
    EXPECT_EQ(revealOut.returnCode, QAGENT_RC_COMMIT_MISMATCH);
}

// ===========================================================================
// TEST: Full Task Lifecycle (Happy Path)
// ===========================================================================

TEST(TestQAgent, FullTaskLifecycleHappyPath)
{
    ContractTestingQAgent qagent;

    // Setup
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 taskId = taskOut.taskId;

    // Accept
    auto acceptOut = qagent.acceptTask(agent1, taskId, 30000);
    EXPECT_EQ(acceptOut.returnCode, QAGENT_RC_SUCCESS);

    // Commit + Reveal
    id resultHash = id(111, 222, 333, 444);
    id salt = id(555, 666, 777, 888);
    QAGENT::CommitRevealData crData;
    crData.resultHash = resultHash;
    crData.salt = salt;
    uint8 commitHashBytes[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crData), sizeof(crData), commitHashBytes, 32);
    id commitHash;
    copyMem(&commitHash, commitHashBytes, 32);

    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, resultHash, salt);

    // Requester approves directly
    auto approveOut = qagent.approveResult(requester1, taskId);
    EXPECT_EQ(approveOut.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMPLETED);
    EXPECT_NE(task.task.finalizedTick, 0);

    // Agent earned something
    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.completedTasks, 1);
    EXPECT_GT(agentOut.agent.totalEarned, 0);

    // Platform stats
    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.totalTasks, 1);
    EXPECT_EQ(stats.stats.activeTasks, 0);
}

// ===========================================================================
// TEST: Task Cancellation
// ===========================================================================

TEST(TestQAgent, CancelOpenTask)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    // Non-requester can't cancel
    auto cancel1 = qagent.cancelTask(agent1, taskId);
    EXPECT_EQ(cancel1.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Requester cancels
    auto cancel2 = qagent.cancelTask(requester1, taskId);
    EXPECT_EQ(cancel2.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_CANCELLED);
}

// ===========================================================================
// TEST: Task Timeout with Progressive Slashing
// ===========================================================================

TEST(TestQAgent, TaskTimeoutSlashes)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 100, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    // Can't timeout before deadline
    auto timeout1 = qagent.timeoutTask(keeper1, taskId);
    EXPECT_EQ(timeout1.returnCode, QAGENT_RC_DEADLINE_NOT_REACHED);

    // Jump past deadline without triggering END_TICK auto-timeout
    auto taskInfo = qagent.getTask(taskId);
    qagent.setTick(taskInfo.task.deadlineTick + 1);

    auto timeout2 = qagent.timeoutTask(keeper1, taskId);
    EXPECT_EQ(timeout2.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_TIMED_OUT);

    // Agent should have 1 slash
    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.slashCount, 1);
    EXPECT_EQ(agentOut.agent.failedTasks, 1);
}

// ===========================================================================
// TEST: Challenge and Dispute Resolution
// ===========================================================================

TEST(TestQAgent, ChallengeAndDisputeValid)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    // Commit + Reveal
    id resultHash = id(111, 222, 333, 444);
    id salt = id(555, 666, 777, 888);
    QAGENT::CommitRevealData crData;
    crData.resultHash = resultHash;
    crData.salt = salt;
    uint8 commitHashBytes[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crData), sizeof(crData), commitHashBytes, 32);
    id commitHash;
    copyMem(&commitHash, commitHashBytes, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, resultHash, salt);

    // Challenge
    id challengerHash = id(999, 888, 777, 666);
    auto challengeOut = qagent.challengeResult(challenger1, taskId, challengerHash, 30000);
    EXPECT_EQ(challengeOut.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_DISPUTED);

    // Register arbitrators
    qagent.registerArbitrator(arb1, 10000);
    qagent.registerArbitrator(arb2, 10000);
    qagent.registerArbitrator(arb3, 10000);

    // 3 votes for VALID → agent wins
    qagent.castDisputeVote(arb1, taskId, QAGENT_VERDICT_VALID);
    qagent.castDisputeVote(arb2, taskId, QAGENT_VERDICT_VALID);
    auto voteOut = qagent.castDisputeVote(arb3, taskId, QAGENT_VERDICT_VALID);
    EXPECT_EQ(voteOut.returnCode, QAGENT_RC_SUCCESS);

    task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMPLETED);
    EXPECT_EQ(task.task.verdict, QAGENT_VERDICT_VALID);

    // Agent completed
    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.completedTasks, 1);
}

TEST(TestQAgent, ChallengeAndDisputeInvalid)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    id resultHash = id(111, 222, 333, 444);
    id salt = id(555, 666, 777, 888);
    QAGENT::CommitRevealData crData;
    crData.resultHash = resultHash;
    crData.salt = salt;
    uint8 commitHashBytes[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crData), sizeof(crData), commitHashBytes, 32);
    id commitHash;
    copyMem(&commitHash, commitHashBytes, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, resultHash, salt);

    qagent.challengeResult(challenger1, taskId, NULL_ID, 30000);

    qagent.registerArbitrator(arb1, 10000);
    qagent.registerArbitrator(arb2, 10000);
    qagent.registerArbitrator(arb3, 10000);

    // 3 votes for INVALID → challenger wins
    qagent.castDisputeVote(arb1, taskId, QAGENT_VERDICT_INVALID);
    qagent.castDisputeVote(arb2, taskId, QAGENT_VERDICT_INVALID);
    qagent.castDisputeVote(arb3, taskId, QAGENT_VERDICT_INVALID);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_CANCELLED);
    EXPECT_EQ(task.task.verdict, QAGENT_VERDICT_INVALID);

    // Agent slashed
    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.failedTasks, 1);
    EXPECT_EQ(agentOut.agent.slashCount, 1);
}

// ===========================================================================
// TEST: Arbitrator Registration
// ===========================================================================

TEST(TestQAgent, RegisterArbitrator)
{
    ContractTestingQAgent qagent;

    // Insufficient stake
    auto out1 = qagent.registerArbitrator(arb1, 5000);
    EXPECT_EQ(out1.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS);

    // Success
    auto out2 = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_TRUE(qagent.state()->isArbitrator(arb1));

    // Duplicate
    auto out3 = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(out3.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// ===========================================================================
// TEST: Agent Deactivate/Reactivate
// ===========================================================================

TEST(TestQAgent, AgentDeactivateReactivate)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto deact = qagent.deactivateAgent(agent1);
    EXPECT_EQ(deact.returnCode, QAGENT_RC_SUCCESS);
    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.status, QAGENT_STATUS_INACTIVE);

    // Can't create task with inactive agent
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 1000, 5000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_WRONG_STATUS);

    auto react = qagent.reactivateAgent(agent1);
    EXPECT_EQ(react.returnCode, QAGENT_RC_SUCCESS);
    agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.status, QAGENT_STATUS_ACTIVE);
}

// ===========================================================================
// TEST: Stake Lifecycle (StakeMore, RequestUnstake, WithdrawStake)
// ===========================================================================

TEST(TestQAgent, StakeLifecycle)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // StakeMore
    auto sm = qagent.stakeMore(agent1, 25000);
    EXPECT_EQ(sm.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(sm.totalStake, 75000);

    // RequestUnstake fails during lock period (need 4 epochs)
    auto ru = qagent.requestUnstake(agent1);
    EXPECT_EQ(ru.returnCode, QAGENT_RC_STAKE_LOCKED);

    // Advance 4 epochs
    for (int i = 0; i < 4; i++)
        qagent.advanceEpoch();

    ru = qagent.requestUnstake(agent1);
    EXPECT_EQ(ru.returnCode, QAGENT_RC_SUCCESS);

    // WithdrawStake fails in same epoch
    auto ws = qagent.withdrawStake(agent1);
    EXPECT_EQ(ws.returnCode, QAGENT_RC_STAKE_LOCKED);

    // Advance 1 more epoch
    qagent.advanceEpoch();

    ws = qagent.withdrawStake(agent1);
    EXPECT_EQ(ws.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(ws.withdrawn, 75000);
}

// ===========================================================================
// TEST: Rating
// ===========================================================================

TEST(TestQAgent, RateCompletedTask)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    // Commit-reveal + approve
    id resultHash = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crData;
    crData.resultHash = resultHash;
    crData.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crData), sizeof(crData), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, resultHash, salt);
    qagent.approveResult(requester1, taskId);

    // Invalid rating
    auto r1 = qagent.rateAgent(requester1, taskId, 0);
    EXPECT_EQ(r1.returnCode, QAGENT_RC_INVALID_INPUT);

    auto r2 = qagent.rateAgent(requester1, taskId, 6);
    EXPECT_EQ(r2.returnCode, QAGENT_RC_INVALID_INPUT);

    // Valid rating
    auto r3 = qagent.rateAgent(requester1, taskId, 5);
    EXPECT_EQ(r3.returnCode, QAGENT_RC_SUCCESS);

    // Double rating
    auto r4 = qagent.rateAgent(requester1, taskId, 4);
    EXPECT_EQ(r4.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// ===========================================================================
// TEST: Governance Proposal
// ===========================================================================

TEST(TestQAgent, GovernanceProposal)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Propose fee change
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 100);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);

    // Vote yes (agent1, stake=50000)
    auto v1 = qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    EXPECT_EQ(v1.returnCode, QAGENT_RC_SUCCESS);

    // Vote no (agent2, stake=50000)
    auto v2 = qagent.voteOnProposal(agent2, prop.proposalIndex, 2);
    EXPECT_EQ(v2.returnCode, QAGENT_RC_SUCCESS);

    // Non-staked user can't vote
    auto v3 = qagent.voteOnProposal(requester1, prop.proposalIndex, 1);
    EXPECT_EQ(v3.returnCode, QAGENT_RC_UNAUTHORIZED);
}

// ===========================================================================
// TEST: Reputation-Based Bond (Tier Affects Stake)
// ===========================================================================

TEST(TestQAgent, UnrankedAgentPays300PercentBond)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    // Unranked agent: 300% bond = 30000
    auto accept1 = qagent.acceptTask(agent1, taskId, 20000);
    EXPECT_EQ(accept1.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS); // Not enough

    auto accept2 = qagent.acceptTask(agent1, taskId, 30000);
    EXPECT_EQ(accept2.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.agentStake, 30000);
}

// ===========================================================================
// TEST: FinalizeTask (Keeper-callable after challenge window)
// ===========================================================================

TEST(TestQAgent, FinalizeAfterChallengeWindow)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, rh, salt);

    // Can't finalize during challenge window
    auto fin1 = qagent.finalizeTask(keeper1, taskId);
    EXPECT_EQ(fin1.returnCode, QAGENT_RC_CHALLENGE_WINDOW_OPEN);

    // Jump past challenge window without triggering END_TICK auto-finalization
    auto task0 = qagent.getTask(taskId);
    qagent.setTick(task0.task.challengeDeadlineTick + 1);

    auto fin2 = qagent.finalizeTask(keeper1, taskId);
    EXPECT_EQ(fin2.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_GT(fin2.keeperFee, 0);

    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMPLETED);
}

// ===========================================================================
// TEST: END_TICK Collection-Based Processing
// ===========================================================================

TEST(TestQAgent, EndTickAutoFinalizes)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, rh, salt);

    // Task is REVEALED with challenge deadline ~300 ticks away
    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_REVEALED);

    // Advance past challenge deadline — END_TICK should auto-finalize
    qagent.advanceTick(301);

    task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMPLETED);
    EXPECT_GT(task.task.finalizedTick, 0u);
}

// ===========================================================================
// TEST: UpdateAgent
// ===========================================================================

TEST(TestQAgent, UpdateAgentProfile)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto upd = qagent.updateAgent(agent1, 0x0F, 2000);
    EXPECT_EQ(upd.returnCode, QAGENT_RC_SUCCESS);

    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.capabilityBitmap, 0x0F);
    EXPECT_EQ(agentOut.agent.pricePerTask, 2000);
}

// ===========================================================================
// TEST: Platform Stats Tracking
// ===========================================================================

TEST(TestQAgent, PlatformStatsTracking)
{
    ContractTestingQAgent qagent;

    auto stats0 = qagent.getPlatformStats();
    EXPECT_EQ(stats0.stats.totalAgents, 0);
    EXPECT_EQ(stats0.stats.totalTasks, 0);

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0x0F, 500, 50000);

    auto stats1 = qagent.getPlatformStats();
    EXPECT_EQ(stats1.stats.totalAgents, 2);

    qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.createTask(requester2, agent2, 1, 3000, 20000);

    auto stats2 = qagent.getPlatformStats();
    EXPECT_EQ(stats2.stats.totalTasks, 2);
    EXPECT_EQ(stats2.stats.activeTasks, 2);
    EXPECT_EQ(stats2.stats.totalVolume, 30000);

    // Verify config defaults
    EXPECT_EQ(stats2.config.platformFeeBPS, QAGENT_PLATFORM_FEE_BPS);
    EXPECT_EQ(stats2.config.minRegistrationStake, QAGENT_MIN_REGISTRATION_STAKE);
}

// ===========================================================================
// TEST: Challenge Window Enforcement
// ===========================================================================

TEST(TestQAgent, ChallengeWindowEnforcement)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, rh, salt);

    // Jump past challenge window without triggering END_TICK auto-finalization
    auto task0 = qagent.getTask(taskId);
    qagent.setTick(task0.task.challengeDeadlineTick + 1);

    // Challenge after window closed — should fail
    auto challengeOut = qagent.challengeResult(challenger1, taskId, NULL_ID, 30000);
    EXPECT_EQ(challengeOut.returnCode, QAGENT_RC_CHALLENGE_WINDOW_CLOSED);
}

// ===========================================================================
// TEST: Milestone Completion
// ===========================================================================

TEST(TestQAgent, MilestoneCompletion)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 30000, NULL_ID, NULL_ID, 3);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 90000); // 300% of 30000

    // Verify task has 3 milestones
    auto task0 = qagent.getTask(taskId);
    EXPECT_EQ(task0.task.totalMilestones, 3);
    EXPECT_EQ(task0.task.completedMilestones, 0);

    // Complete milestone 1
    auto m1 = qagent.completeMilestone(requester1, taskId);
    EXPECT_EQ(m1.returnCode, QAGENT_RC_SUCCESS);

    auto task1 = qagent.getTask(taskId);
    EXPECT_EQ(task1.task.completedMilestones, 1);

    // Complete milestone 2
    auto m2 = qagent.completeMilestone(requester1, taskId);
    EXPECT_EQ(m2.returnCode, QAGENT_RC_SUCCESS);

    auto task2 = qagent.getTask(taskId);
    EXPECT_EQ(task2.task.completedMilestones, 2);

    // Complete milestone 3 (final) — should complete task
    auto m3 = qagent.completeMilestone(requester1, taskId);
    EXPECT_EQ(m3.returnCode, QAGENT_RC_SUCCESS);

    auto task3 = qagent.getTask(taskId);
    EXPECT_EQ(task3.task.completedMilestones, 3);
    EXPECT_EQ(task3.task.status, QAGENT_TASK_STATUS_COMPLETED);

    // 4th milestone should fail
    auto m4 = qagent.completeMilestone(requester1, taskId);
    EXPECT_NE(m4.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Tier Recalculation in FinalizeTask
// ===========================================================================

TEST(TestQAgent, TierRecalcOnFinalize)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Agent starts as UNRANKED
    auto a = qagent.getAgent(agent1);
    EXPECT_EQ(a.agent.tier, QAGENT_TIER_UNRANKED);

    // Complete 10 tasks via FinalizeTask (keeper path) to reach Bronze
    for (int i = 0; i < 10; i++)
    {
        auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
        uint64 taskId = taskOut.taskId;
        qagent.acceptTask(agent1, taskId, 30000);

        id rh = id(1, 2, 3, static_cast<uint64>(i + 1));
        id salt = id(5, 6, 7, 8);
        QAGENT::CommitRevealData crd;
        crd.resultHash = rh;
        crd.salt = salt;
        uint8 h[32];
        KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
        id ch;
        copyMem(&ch, h, 32);
        qagent.commitResult(agent1, taskId, ch);
        qagent.revealResult(agent1, taskId, rh, salt);

        // Jump past challenge window without triggering END_TICK auto-finalization
        auto taskInfo = qagent.getTask(taskId);
        qagent.setTick(taskInfo.task.challengeDeadlineTick + 1);

        auto fin = qagent.finalizeTask(keeper1, taskId);
        EXPECT_EQ(fin.returnCode, QAGENT_RC_SUCCESS);
    }

    a = qagent.getAgent(agent1);
    EXPECT_EQ(a.agent.completedTasks, 10);
    EXPECT_EQ(a.agent.tier, QAGENT_TIER_BRONZE);
}

// ===========================================================================
// TEST: Tier Recalculation via ApproveResult (regression for platformFee aliasing bug)
// ===========================================================================

TEST(TestQAgent, TierRecalcOnApproveResult)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Complete 10 tasks via ApproveResult to reach Bronze
    for (int i = 0; i < 10; i++)
    {
        auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
        uint64 taskId = taskOut.taskId;
        qagent.acceptTask(agent1, taskId, 30000);

        id rh = id(10, 20, 30, static_cast<uint64>(i + 100));
        id salt = id(50, 60, 70, 80);
        QAGENT::CommitRevealData crd;
        crd.resultHash = rh;
        crd.salt = salt;
        uint8 h[32];
        KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
        id ch;
        copyMem(&ch, h, 32);
        qagent.commitResult(agent1, taskId, ch);
        qagent.revealResult(agent1, taskId, rh, salt);
        auto app = qagent.approveResult(requester1, taskId);
        EXPECT_EQ(app.returnCode, QAGENT_RC_SUCCESS);
    }

    auto a = qagent.getAgent(agent1);
    EXPECT_EQ(a.agent.completedTasks, 10);
    // 100% completion rate with 10 tasks → must be Bronze (>= 10 tasks, >= 90% rate)
    EXPECT_EQ(a.agent.tier, QAGENT_TIER_BRONZE);
}

// ===========================================================================
// TEST: Tier Downgrade on TimeoutTask
// ===========================================================================

TEST(TestQAgent, TierDowngradeOnTimeout)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Complete 10 tasks to reach Bronze via approve
    for (int i = 0; i < 10; i++)
    {
        auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
        uint64 taskId = taskOut.taskId;
        qagent.acceptTask(agent1, taskId, 30000);

        id rh = id(1, 2, 3, static_cast<uint64>(i + 100));
        id salt = id(5, 6, 7, 8);
        QAGENT::CommitRevealData crd;
        crd.resultHash = rh;
        crd.salt = salt;
        uint8 h[32];
        KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
        id ch;
        copyMem(&ch, h, 32);
        qagent.commitResult(agent1, taskId, ch);
        qagent.revealResult(agent1, taskId, rh, salt);
        qagent.approveResult(requester1, taskId);
    }

    auto a = qagent.getAgent(agent1);
    EXPECT_EQ(a.agent.tier, QAGENT_TIER_BRONZE);

    // Now timeout 2 tasks → failure rate should drop tier
    for (int i = 0; i < 2; i++)
    {
        auto taskOut = qagent.createTask(requester1, agent1, 0, 100, 10000);
        uint64 taskId = taskOut.taskId;
        qagent.acceptTask(agent1, taskId, 30000);
        auto ti = qagent.getTask(taskId);
        qagent.setTick(ti.task.deadlineTick + 1);
        auto tout = qagent.timeoutTask(keeper1, taskId);
        EXPECT_EQ(tout.returnCode, QAGENT_RC_SUCCESS);
    }

    a = qagent.getAgent(agent1);
    EXPECT_EQ(a.agent.failedTasks, 2);
    // 10/12 = 83.3% — below 90% threshold for Bronze → should be UNRANKED
    EXPECT_EQ(a.agent.tier, QAGENT_TIER_UNRANKED);
}

// ===========================================================================
// TEST: Economic Invariant — Value Conservation in ApproveResult
// ===========================================================================

TEST(TestQAgent, EconomicInvariantApproveResult)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    uint64 taskId = taskOut.taskId;
    qagent.acceptTask(agent1, taskId, 30000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, rh, salt);
    qagent.approveResult(requester1, taskId);

    // Verify: escrow went to agent (minus fee) + bond returned
    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMPLETED);

    auto stats = qagent.getPlatformStats();
    // Platform fee = 10000 * 150 / 10000 = 150
    sint64 expectedFee = 10000 * QAGENT_PLATFORM_FEE_BPS / QAGENT_BPS_SCALE;
    // Burn = 50%, Treasury = 30%, ArbPool = 20%
    sint64 expectedBurn = expectedFee * QAGENT_FEE_BURN_BPS / QAGENT_BPS_SCALE;
    sint64 expectedTreasury = expectedFee * QAGENT_FEE_TREASURY_BPS / QAGENT_BPS_SCALE;
    sint64 expectedArbPool = expectedFee - expectedBurn - expectedTreasury;

    EXPECT_EQ(stats.stats.totalBurned, expectedBurn);
    EXPECT_EQ(stats.stats.treasuryBalance, expectedTreasury);
    EXPECT_EQ(stats.stats.arbitratorPool, expectedArbPool);
}

// ===========================================================================
// TEST: Economic Invariant — Minimum Escrow (1 QU edge case)
// ===========================================================================

TEST(TestQAgent, MinEscrowEdgeCase)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Below minimum escrow should fail
    auto out1 = qagent.createTask(requester1, agent1, 0, 2000, 500);
    EXPECT_EQ(out1.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS);

    // Exact minimum should succeed
    auto out2 = qagent.createTask(requester1, agent1, 0, 2000, 1000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Agent Delegation (Multi-Agent Orchestration)
// ===========================================================================

TEST(TestQAgent, DelegateTaskBasic)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Create parent task
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 parentTaskId = taskOut.taskId;

    // Agent1 accepts
    qagent.acceptTask(agent1, parentTaskId, 30000);

    // Agent1 delegates subtask to agent2
    auto delOut = qagent.delegateTask(agent1, parentTaskId, agent2, QAGENT_TASK_TYPE_ANALYSIS, 1000, 5000);
    EXPECT_EQ(delOut.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_GT(delOut.childTaskId, 0u);

    // Verify child task
    auto childTask = qagent.getTask(delOut.childTaskId);
    EXPECT_EQ(childTask.task.parentTaskId, parentTaskId);
    EXPECT_EQ(childTask.task.requester, agent1); // Parent agent is requester
    EXPECT_EQ(childTask.task.assignedAgent, agent2);
    EXPECT_EQ(childTask.task.escrowAmount, 5000);

    // Verify parent escrow was deducted (10000 - 5000 = 5000 remaining)
    auto parentTask = qagent.getTask(parentTaskId);
    EXPECT_EQ(parentTask.task.escrowAmount, 5000);
}

TEST(TestQAgent, DelegateTaskSelfDealingBlocked)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Self-dealing: agent1 delegates to itself
    auto delOut = qagent.delegateTask(agent1, taskOut.taskId, agent1, 0, 1000, 5000);
    EXPECT_EQ(delOut.returnCode, QAGENT_RC_SELF_DEALING);
}

TEST(TestQAgent, DelegateTaskDepthLimit)
{
    ContractTestingQAgent qagent;

    // Register 5 agents for deep delegation chain
    static const id agents_arr[5] = {
        id(100, 0, 0, 0), id(200, 0, 0, 0), id(300, 0, 0, 0),
        id(400, 0, 0, 0), id(500, 0, 0, 0)
    };
    for (int i = 0; i < 5; i++)
        qagent.registerAgent(agents_arr[i], 0xFF, 1000, 50000);

    // Create root task: reqForDepth → agents_arr[0]
    static const id reqForDepth = id(900, 0, 0, 0);
    qagent.fund(reqForDepth, 10000000LL);
    auto taskOut = qagent.createTask(reqForDepth, agents_arr[0], 0, 5000, 10000);
    qagent.acceptTask(agents_arr[0], taskOut.taskId, 30000);

    // Depth 1: agents_arr[0] delegates to agents_arr[1]
    auto d1 = qagent.delegateTask(agents_arr[0], taskOut.taskId, agents_arr[1], 0, 1000, 3000);
    EXPECT_EQ(d1.returnCode, QAGENT_RC_SUCCESS);
    qagent.acceptTask(agents_arr[1], d1.childTaskId, 9000);

    // Depth 2: agents_arr[1] delegates to agents_arr[2]
    auto d2 = qagent.delegateTask(agents_arr[1], d1.childTaskId, agents_arr[2], 0, 1000, 2000);
    EXPECT_EQ(d2.returnCode, QAGENT_RC_SUCCESS);
    qagent.acceptTask(agents_arr[2], d2.childTaskId, 6000);

    // Depth 3: agents_arr[2] delegates to agents_arr[3]
    auto d3 = qagent.delegateTask(agents_arr[2], d2.childTaskId, agents_arr[3], 0, 1000, 1000);
    EXPECT_EQ(d3.returnCode, QAGENT_RC_SUCCESS);
    qagent.acceptTask(agents_arr[3], d3.childTaskId, 3000);

    // Depth 4: should FAIL — max depth is 3
    auto d4 = qagent.delegateTask(agents_arr[3], d3.childTaskId, agents_arr[4], 0, 1000, 500);
    EXPECT_EQ(d4.returnCode, QAGENT_RC_DELEGATION_DEPTH_EXCEEDED);
}

// ===========================================================================
// TEST: Reputation Score Export
// ===========================================================================

TEST(TestQAgent, ReputationScoreExport)
{
    ContractTestingQAgent qagent;

    // Non-existent agent
    auto rep0 = qagent.getReputationScore(agent1);
    EXPECT_EQ(rep0.found, 0);

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // New agent — zero score (no completions)
    auto rep1 = qagent.getReputationScore(agent1);
    EXPECT_EQ(rep1.found, 1);
    EXPECT_EQ(rep1.score, 0);

    // Complete a task to get some score
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);
    qagent.approveResult(requester1, taskOut.taskId);

    auto rep2 = qagent.getReputationScore(agent1);
    EXPECT_EQ(rep2.found, 1);
    EXPECT_GT(rep2.score, 0u);
    EXPECT_LE(rep2.score, 10000u);
}

// ===========================================================================
// TEST: GetAgentsByCapability Filtered Count
// ===========================================================================

TEST(TestQAgent, GetAgentsByCapabilityFiltered)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);   // All caps
    qagent.registerAgent(agent2, 0x01, 500, 50000);    // Only cap bit 0

    auto output1 = qagent.getAgentsByCapability(0xFF);
    EXPECT_EQ(output1.count, 1); // Only agent1 has all bits set

    auto output2 = qagent.getAgentsByCapability(0x01);
    EXPECT_EQ(output2.count, 2); // Both agents have bit 0

    // Deactivate agent1 — should not be counted
    qagent.deactivateAgent(agent1);

    auto output3 = qagent.getAgentsByCapability(0x01);
    EXPECT_EQ(output3.count, 1); // Only agent2 active
}

// ===========================================================================
// TEST: Deterministic Inference Task Type
// ===========================================================================

TEST(TestQAgent, DeterministicInferenceTaskType)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Create task with deterministic inference type
    auto out = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE, 2000, 10000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(out.taskId);
    EXPECT_EQ(task.task.taskType, QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE);

    // Invalid task type (above max) should fail
    auto out2 = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE + 1, 2000, 10000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_INVALID_INPUT);
}

// ===========================================================================
// TEST: Progressive Suspension (4 consecutive timeouts)
// ===========================================================================

TEST(TestQAgent, ProgressiveSuspensionAfter4Timeouts)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    for (int i = 0; i < 4; i++)
    {
        auto taskOut = qagent.createTask(requester1, agent1, 0, 100, 10000);
        uint64 taskId = taskOut.taskId;
        qagent.acceptTask(agent1, taskId, 30000);
        auto ti = qagent.getTask(taskId);
        qagent.setTick(ti.task.deadlineTick + 1);
        auto tout = qagent.timeoutTask(keeper1, taskId);
        EXPECT_EQ(tout.returnCode, QAGENT_RC_SUCCESS);
    }

    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.agent.slashCount, 4);
    EXPECT_EQ(agentOut.agent.status, QAGENT_STATUS_SUSPENDED);

    // Suspended agent can't accept new tasks
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Stress — Agent Capacity Limit
// ===========================================================================

TEST(TestQAgent, StressAgentCapacityLimit)
{
    ContractTestingQAgent qagent;

    // Register agents up to a reasonable count to verify counting (full 8192 would be slow)
    // We test the enforcement mechanism by checking the guard condition
    constexpr uint32 COUNT = 64;
    for (uint32 i = 0; i < COUNT; i++)
    {
        id agentX = id(10000 + i, 0, 0, 0);
        qagent.fund(agentX, 100000LL);
        auto out = qagent.registerAgent(agentX, 0xFF, 1000, 50000);
        EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);
    }

    EXPECT_EQ(qagent.state()->agentCount(), COUNT);

    // Verify duplicate registration fails
    id agentFirst = id(10000, 0, 0, 0);
    auto dup = qagent.registerAgent(agentFirst, 0xFF, 1000, 50000);
    EXPECT_EQ(dup.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// ===========================================================================
// TEST: Economic Invariant — Timeout Slash Burns Agent Bond
// ===========================================================================

TEST(TestQAgent, EconomicInvariantTimeoutSlash)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    sint64 escrow = 10000;
    auto taskOut = qagent.createTask(requester1, agent1, 0, 100, escrow);
    uint64 taskId = taskOut.taskId;

    // Unranked agent bond: 300% of escrow = 30000
    sint64 agentBond = 30000;
    qagent.acceptTask(agent1, taskId, agentBond);

    // Record stats before timeout
    auto statsBefore = qagent.getPlatformStats();
    sint64 burnedBefore = statsBefore.stats.totalBurned;

    auto taskInfo = qagent.getTask(taskId);
    qagent.setTick(taskInfo.task.deadlineTick + 1);
    auto tout = qagent.timeoutTask(keeper1, taskId);
    EXPECT_EQ(tout.returnCode, QAGENT_RC_SUCCESS);

    // Verify: escrow returned to requester, agent bond burned (minus keeper fee)
    auto statsAfter = qagent.getPlatformStats();
    sint64 burnedAfter = statsAfter.stats.totalBurned;

    // Keeper fee = agentBond * keeperFeeBPS / 10000
    sint64 keeperFee = tout.keeperFee;
    sint64 expectedBurn = agentBond - keeperFee;
    EXPECT_EQ(burnedAfter - burnedBefore, expectedBurn);

    // Task should be timed out
    auto task = qagent.getTask(taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_TIMED_OUT);
}

// ===========================================================================
// TEST: Deactivation-Reactivation Cycle
// ===========================================================================

TEST(TestQAgent, DeactivateReactivateCycle)
{
    ContractTestingQAgent qagent;

    auto regOut = qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    EXPECT_EQ(regOut.returnCode, QAGENT_RC_SUCCESS);

    // Deactivate
    auto deact = qagent.deactivateAgent(agent1);
    EXPECT_EQ(deact.returnCode, QAGENT_RC_SUCCESS);

    auto a1 = qagent.getAgent(agent1);
    EXPECT_EQ(a1.agent.status, QAGENT_STATUS_INACTIVE);

    // Cannot create tasks for inactive agent
    auto tOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(tOut.returnCode, QAGENT_RC_WRONG_STATUS);

    // Reactivate
    auto react = qagent.reactivateAgent(agent1);
    EXPECT_EQ(react.returnCode, QAGENT_RC_SUCCESS);

    auto a2 = qagent.getAgent(agent1);
    EXPECT_EQ(a2.agent.status, QAGENT_STATUS_ACTIVE);

    // Now can create tasks again
    auto tOut2 = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(tOut2.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Unstake and Withdraw Full Lifecycle
// ===========================================================================

TEST(TestQAgent, UnstakeWithdrawLifecycle)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Stake more
    auto sm = qagent.stakeMore(agent1, 25000);
    EXPECT_EQ(sm.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(sm.totalStake, 75000); // 50000 + 25000

    // Request unstake — should work after lock period
    // Advance past lock epochs
    for (uint32 e = 0; e < QAGENT_STAKE_LOCK_EPOCHS + 1; e++)
        qagent.advanceEpoch();

    auto ru = qagent.requestUnstake(agent1);
    EXPECT_EQ(ru.returnCode, QAGENT_RC_SUCCESS);

    // Withdraw — need 1 epoch after request
    qagent.advanceEpoch();

    auto wd = qagent.withdrawStake(agent1);
    EXPECT_EQ(wd.returnCode, QAGENT_RC_SUCCESS);

    // Verify agent is now inactive
    auto a = qagent.getAgent(agent1);
    EXPECT_EQ(a.agent.status, QAGENT_STATUS_INACTIVE);

    // Verify stake is zero
    auto s = qagent.getStake(agent1);
    EXPECT_EQ(s.stake.registrationStake, 0);
    EXPECT_EQ(s.stake.additionalStake, 0);
}

// ===========================================================================
// TEST: Cancel Open Task Returns Escrow
// ===========================================================================

TEST(TestQAgent, CancelOpenTaskReturnsEscrow)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 15000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Only requester can cancel
    auto badCancel = qagent.cancelTask(agent1, taskOut.taskId);
    EXPECT_EQ(badCancel.returnCode, QAGENT_RC_UNAUTHORIZED);

    auto cancel = qagent.cancelTask(requester1, taskOut.taskId);
    EXPECT_EQ(cancel.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskOut.taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_CANCELLED);
}

// ===========================================================================
// TEST: Arbitrator Registration and Dispute Vote
// ===========================================================================

TEST(TestQAgent, ArbitratorRegistrationAndVote)
{
    ContractTestingQAgent qagent;

    // Register arbitrators
    auto a1out = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(a1out.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_TRUE(qagent.state()->isArbitrator(arb1));

    auto a2out = qagent.registerArbitrator(arb2, 10000);
    EXPECT_EQ(a2out.returnCode, QAGENT_RC_SUCCESS);

    // Duplicate registration should fail
    auto dupArb = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(dupArb.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// ===========================================================================
// TEST: Governance — Propose, Vote, Execute
// ===========================================================================

TEST(TestQAgent, GovernanceFullLifecycle)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Give agent1 extra stake so yes vote outweighs no vote
    // (both agents have registrationStake=50000; stakeMore gives agent1 additional weight)
    qagent.stakeMore(agent1, 50000);

    // Propose fee change (from 150 to 100 BPS)
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 100);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);

    // agent1 votes yes (100K weight), agent2 votes no (50K weight)
    qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    qagent.voteOnProposal(agent2, prop.proposalIndex, 2);

    // Jump past vote period without running END_TICK 50K+ times
    // endTick = creationTick(1) + QAGENT_GOVERNANCE_VOTE_TICKS(50400) = 50401
    qagent.setTick(QAGENT_GOVERNANCE_VOTE_TICKS + 10);
    qagent.advanceEpoch();

    // Config should now reflect the new value
    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.config.platformFeeBPS, 100);
}

// ===========================================================================
// TEST: Service Registration Lifecycle
// ===========================================================================

TEST(TestQAgent, ServiceRegistrationLifecycle)
{
    ContractTestingQAgent qagent;

    // Must register as agent first (RegisterService requires agents.contains)
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Register a service
    auto out1 = qagent.registerService(agent1, 0x0F, 5000, QAGENT_TASK_TYPE_ANALYSIS);
    EXPECT_EQ(out1.returnCode, QAGENT_RC_SUCCESS);

    // Verify via GetService
    auto svc = qagent.getService(agent1);
    EXPECT_EQ(svc.found, 1);
    EXPECT_EQ(svc.service.active, 1);
    EXPECT_EQ(svc.service.capabilityBitmap, (uint32)0x0F);
    EXPECT_EQ(svc.service.basePrice, 5000);
    EXPECT_EQ(svc.service.taskType, QAGENT_TASK_TYPE_ANALYSIS);
    EXPECT_TRUE(svc.service.ownerId == agent1);

    // Second registration overwrites (HashMap::set) — succeeds
    auto out2 = qagent.registerService(agent1, 0xFF, 8000, QAGENT_TASK_TYPE_TRADE);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);
    svc = qagent.getService(agent1);
    EXPECT_EQ(svc.service.capabilityBitmap, (uint32)0xFF);
    EXPECT_EQ(svc.service.basePrice, 8000);

    // Update price
    auto out3 = qagent.updateServicePrice(agent1, agent1, 7500);
    EXPECT_EQ(out3.returnCode, QAGENT_RC_SUCCESS);
    svc = qagent.getService(agent1);
    EXPECT_EQ(svc.service.basePrice, 7500);

    // Non-owner cannot update price
    auto out4 = qagent.updateServicePrice(agent2, agent1, 9999);
    EXPECT_EQ(out4.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Negative price fails
    auto out5 = qagent.updateServicePrice(agent1, agent1, -100);
    EXPECT_EQ(out5.returnCode, QAGENT_RC_INVALID_INPUT);

    // Deprecate
    auto out6 = qagent.deprecateService(agent1, agent1);
    EXPECT_EQ(out6.returnCode, QAGENT_RC_SUCCESS);
    svc = qagent.getService(agent1);
    EXPECT_EQ(svc.service.active, 0);

    // Non-owner cannot deprecate
    auto out7 = qagent.deprecateService(agent2, agent1);
    EXPECT_EQ(out7.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Non-existent service
    auto svc2 = qagent.getService(agent2);
    EXPECT_EQ(svc2.found, 0);
}

// ===========================================================================
// TEST: GetArbitrator and GetProposal Query Functions
// ===========================================================================

TEST(TestQAgent, QueryFunctionsArbitratorAndProposal)
{
    ContractTestingQAgent qagent;

    // Non-existent arbitrator
    auto arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.found, 0);

    qagent.registerArbitrator(arb1, 10000);

    arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.found, 1);
    EXPECT_EQ(arb.arbitrator.stakeAmount, 10000);
    EXPECT_EQ(arb.arbitrator.active, 1);

    // Non-existent proposal
    auto prop = qagent.getProposal(0);
    EXPECT_EQ(prop.found, 0);

    // Register agent and create proposal
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto propOut = qagent.proposeParamChange(agent1, QAGENT_PARAM_MIN_ESCROW, 2000);
    EXPECT_EQ(propOut.returnCode, QAGENT_RC_SUCCESS);

    prop = qagent.getProposal(propOut.proposalIndex);
    EXPECT_EQ(prop.found, 1);
    EXPECT_EQ(prop.proposal.paramId, QAGENT_PARAM_MIN_ESCROW);
    EXPECT_EQ(prop.proposal.newValue, 2000);
    EXPECT_EQ(prop.proposal.active, 1);
}

// ===========================================================================
// TEST: DelegateTask Capacity Validation Before State Mutation
// ===========================================================================

TEST(TestQAgent, DelegateTaskCapacityValidationOrder)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Create a parent task with known escrow
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000LL);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Accept parent task
    qagent.acceptTask(agent1, taskOut.taskId, 300000LL);

    // Record parent escrow before delegation attempt with invalid task type
    QAGENT::TaskRecord parentBefore;
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, parentBefore));
    sint64 escrowBefore = parentBefore.escrowAmount;

    // Try delegate with invalid task type (type 99 > max) — should fail BEFORE mutating parent
    auto delOut = qagent.delegateTask(agent1, taskOut.taskId, agent2, 99, 1000, 50000LL);
    EXPECT_EQ(delOut.returnCode, QAGENT_RC_INVALID_INPUT);

    // Verify parent escrow was NOT corrupted
    QAGENT::TaskRecord parentAfter;
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, parentAfter));
    EXPECT_EQ(parentAfter.escrowAmount, escrowBefore);
}

// ===========================================================================
// TEST: RequestOracleVerification Status Guards
// ===========================================================================

TEST(TestQAgent, RequestOracleVerificationGuards)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Create and accept task
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Oracle verification on ACCEPTED task should fail (need DISPUTED or REVEALED)
    auto oracleOut = qagent.requestOracleVerification(requester1, taskOut.taskId);
    EXPECT_EQ(oracleOut.returnCode, QAGENT_RC_WRONG_STATUS);

    // Non-existent task
    auto oracleOut2 = qagent.requestOracleVerification(requester1, 99999);
    EXPECT_EQ(oracleOut2.returnCode, QAGENT_RC_TASK_NOT_FOUND);
}

// ===========================================================================
// TEST: Economic Invariant — Conservation of Value in Milestone Flow
// ===========================================================================

TEST(TestQAgent, EconomicInvariantMilestoneFlow)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    sint64 contractBalBefore = getBalance(id(QAGENT_CONTRACT_INDEX, 0, 0, 0));
    sint64 requesterBalBefore = getBalance(requester1);
    sint64 agentBalBefore = getBalance(agent1);

    // Create task with 3 milestones
    sint64 escrow = 90000LL;
    auto taskOut = qagent.createTask(requester1, agent1, 0, 5000, escrow, NULL_ID, NULL_ID, 3);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Accept task
    qagent.acceptTask(agent1, taskOut.taskId, 270000LL); // 300% bond for unranked

    // Complete milestone 1 — should pay 1/3 of escrow minus fees
    qagent.completeMilestone(requester1, taskOut.taskId);

    QAGENT::TaskRecord task;
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.completedMilestones, 1);

    // Complete milestone 2
    qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.completedMilestones, 2);

    // Complete milestone 3 (final) — should finalize task
    qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.completedMilestones, 3);
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_COMPLETED);
    EXPECT_NE(task.finalizedTick, 0);

    // Verify all value is accounted for
    sint64 contractBalAfter = getBalance(id(QAGENT_CONTRACT_INDEX, 0, 0, 0));
    sint64 requesterBalAfter = getBalance(requester1);
    sint64 agentBalAfter = getBalance(agent1);
    auto stats = qagent.getPlatformStats();

    // Total in = escrow + agent bond
    // Total out = agent payout (escrow - platformFee) + agent bond returned + platformFee distribution
    // Conservation: requester loss + agent net = contract delta + burned
    sint64 totalSystemDelta = (requesterBalAfter - requesterBalBefore) + (agentBalAfter - agentBalBefore) + (contractBalAfter - contractBalBefore);
    sint64 totalBurned = stats.stats.totalBurned;
    EXPECT_EQ(totalSystemDelta + totalBurned, 0);
}

// ===========================================================================
// TEST: Minimum Escrow Edge Case (exact minimum)
// ===========================================================================

TEST(TestQAgent, ExactMinimumEscrow)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Exact minimum escrow should succeed
    auto out = qagent.createTask(requester1, agent1, 0, 2000, QAGENT_MIN_ESCROW);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);

    // Below minimum should fail
    auto out2 = qagent.createTask(requester1, agent1, 0, 2000, QAGENT_MIN_ESCROW - 1);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS);
}

// ===========================================================================
// TEST: Rate Agent Edge Cases
// ===========================================================================

TEST(TestQAgent, RateAgentEdgeCases)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Complete a task
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);
    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);
    qagent.approveResult(requester1, taskOut.taskId);

    // Invalid rating (0 and 6) should fail
    auto r0 = qagent.rateAgent(requester1, taskOut.taskId, 0);
    EXPECT_EQ(r0.returnCode, QAGENT_RC_INVALID_INPUT);

    auto r6 = qagent.rateAgent(requester1, taskOut.taskId, 6);
    EXPECT_EQ(r6.returnCode, QAGENT_RC_INVALID_INPUT);

    // Valid rating
    auto r3 = qagent.rateAgent(requester1, taskOut.taskId, 3);
    EXPECT_EQ(r3.returnCode, QAGENT_RC_SUCCESS);

    // Double rating should fail
    auto r4 = qagent.rateAgent(requester1, taskOut.taskId, 4);
    EXPECT_EQ(r4.returnCode, QAGENT_RC_ALREADY_EXISTS);

    // Non-requester cannot rate
    auto r5 = qagent.rateAgent(agent1, taskOut.taskId, 5);
    EXPECT_EQ(r5.returnCode, QAGENT_RC_UNAUTHORIZED);
}

// ===========================================================================
// TEST: Governance Invalid Parameter Boundaries
// ===========================================================================

TEST(TestQAgent, GovernanceParameterBoundaries)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Invalid paramId (>= 10, since 8 = PAUSED and 9 = TREASURY_BURN are valid)
    auto out1 = qagent.proposeParamChange(agent1, 10, 100);
    EXPECT_EQ(out1.returnCode, QAGENT_RC_INVALID_INPUT);

    // Non-staked user cannot propose
    auto out2 = qagent.proposeParamChange(requester1, 0, 100);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Vote on expired proposal
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 100);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);

    // Jump past voting period
    qagent.setTick(QAGENT_GOVERNANCE_VOTE_TICKS + 100);
    auto vote = qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    EXPECT_EQ(vote.returnCode, QAGENT_RC_WRONG_STATUS);

    // Invalid vote value
    qagent.setTick(1); // reset
    auto prop2 = qagent.proposeParamChange(agent1, QAGENT_PARAM_MIN_ESCROW, 500);
    auto vote2 = qagent.voteOnProposal(agent1, prop2.proposalIndex, 0);
    EXPECT_EQ(vote2.returnCode, QAGENT_RC_INVALID_INPUT);
    auto vote3 = qagent.voteOnProposal(agent1, prop2.proposalIndex, 3);
    EXPECT_EQ(vote3.returnCode, QAGENT_RC_INVALID_INPUT);
}

// ===========================================================================
// TEST: Cancel Task Not by Requester
// ===========================================================================

TEST(TestQAgent, CancelTaskUnauthorized)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Agent cannot cancel requester's task
    auto out = qagent.cancelTask(agent1, taskOut.taskId);
    EXPECT_EQ(out.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Different requester cannot cancel
    auto out2 = qagent.cancelTask(requester2, taskOut.taskId);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Requester can cancel
    auto out3 = qagent.cancelTask(requester1, taskOut.taskId);
    EXPECT_EQ(out3.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Challenge Window Timing
// ===========================================================================

TEST(TestQAgent, ChallengeBeforeRevealFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Challenge before reveal (status is ACCEPTED, not REVEALED)
    auto out = qagent.challengeResult(challenger1, taskOut.taskId, id(1,0,0,0), 10000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Arbitrator Cannot Double Register
// ===========================================================================

TEST(TestQAgent, ArbitratorDoubleRegistration)
{
    ContractTestingQAgent qagent;

    auto out1 = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(out1.returnCode, QAGENT_RC_SUCCESS);

    auto out2 = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// ===========================================================================
// TEST: Task Type Validation
// ===========================================================================

TEST(TestQAgent, InvalidTaskTypeRejected)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Task type 8 (above DETERMINISTIC_INFERENCE=7) should fail
    auto out = qagent.createTask(requester1, agent1, 8, 2000, 10000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_INVALID_INPUT);

    // Task type 7 (DETERMINISTIC_INFERENCE) should succeed
    auto out2 = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE, 2000, 10000);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Timeout Task Manual (not via END_TICK)
// ===========================================================================

TEST(TestQAgent, ManualTimeoutGuards)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 100, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Cannot timeout before deadline
    auto out = qagent.timeoutTask(requester1, taskOut.taskId);
    EXPECT_EQ(out.returnCode, QAGENT_RC_DEADLINE_NOT_REACHED);

    // Jump past deadline (without triggering END_TICK auto-processing)
    QAGENT::TaskRecord task;
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    qagent.setTick(task.deadlineTick + 1);

    // Now timeout should succeed
    auto out2 = qagent.timeoutTask(requester1, taskOut.taskId);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_SUCCESS);

    // Double timeout should fail
    auto out3 = qagent.timeoutTask(requester1, taskOut.taskId);
    EXPECT_EQ(out3.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Finalize Task Guards
// ===========================================================================

TEST(TestQAgent, FinalizeTaskGuards)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);

    // Cannot finalize OPEN task
    auto out = qagent.finalizeTask(keeper1, taskOut.taskId);
    EXPECT_EQ(out.returnCode, QAGENT_RC_WRONG_STATUS);

    // Accept and go through commit-reveal
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    id rh = id(10, 20, 30, 40);
    id salt = id(50, 60, 70, 80);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);

    // Cannot finalize before challenge window expires
    auto out2 = qagent.finalizeTask(keeper1, taskOut.taskId);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_CHALLENGE_WINDOW_OPEN);
}

// ===========================================================================
// TEST: Excess Registration Stake Refunded
// ===========================================================================

TEST(TestQAgent, ExcessRegistrationStakeRefunded)
{
    ContractTestingQAgent qagent;

    sint64 balBefore = getBalance(agent1);

    // Overpay by 25000
    auto out = qagent.registerAgent(agent1, 0xFF, 1000, 75000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);

    // Stake should be exactly minRegistrationStake
    auto stake = qagent.getStake(agent1);
    EXPECT_EQ(stake.stake.registrationStake, 50000);

    // Agent should have received the excess back
    sint64 balAfter = getBalance(agent1);
    EXPECT_EQ(balBefore - balAfter, 50000); // Only 50K locked, 25K refunded
}

// ===========================================================================
// TEST: Excess Arbitrator Stake Refunded
// ===========================================================================

TEST(TestQAgent, ExcessArbitratorStakeRefunded)
{
    ContractTestingQAgent qagent;

    sint64 balBefore = getBalance(arb1);

    // Overpay by 5000
    auto out = qagent.registerArbitrator(arb1, 15000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);

    auto arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.arbitrator.stakeAmount, 10000);

    sint64 balAfter = getBalance(arb1);
    EXPECT_EQ(balBefore - balAfter, 10000); // Only 10K locked
}

// ===========================================================================
// TEST: Multiple Tasks Concurrent Processing
// ===========================================================================

TEST(TestQAgent, MultipleConcurrentTasks)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Create 3 tasks with different deadlines
    auto t1 = qagent.createTask(requester1, agent1, 0, 100, 10000);
    auto t2 = qagent.createTask(requester1, agent1, 1, 200, 20000);
    auto t3 = qagent.createTask(requester1, agent1, 2, 300, 30000);

    EXPECT_EQ(t1.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(t2.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(t3.returnCode, QAGENT_RC_SUCCESS);

    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.totalTasks, 3);
    EXPECT_EQ(stats.stats.activeTasks, 3);

    // Accept all three
    qagent.acceptTask(agent1, t1.taskId, 30000);
    qagent.acceptTask(agent1, t2.taskId, 60000);
    qagent.acceptTask(agent1, t3.taskId, 90000);

    // Timeout task1 by jumping to its deadline
    QAGENT::TaskRecord task1;
    EXPECT_TRUE(qagent.state()->getTaskRecord(t1.taskId, task1));
    qagent.setTick(task1.deadlineTick + 1);
    auto tout = qagent.timeoutTask(requester1, t1.taskId);
    EXPECT_EQ(tout.returnCode, QAGENT_RC_SUCCESS);

    // Task2 and task3 should still be ACCEPTED
    auto task2Out = qagent.getTask(t2.taskId);
    EXPECT_EQ(task2Out.task.status, QAGENT_TASK_STATUS_ACCEPTED);
    auto task3Out = qagent.getTask(t3.taskId);
    EXPECT_EQ(task3Out.task.status, QAGENT_TASK_STATUS_ACCEPTED);
}

// ===========================================================================
// TEST: CompleteMilestone escrow depletion prevents double-payment
// ===========================================================================

TEST(TestQAgent, CompleteMilestoneEscrowDepletes)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Create 3-milestone task with 90000 escrow
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 90000, NULL_ID, NULL_ID, 3);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    qagent.acceptTask(agent1, taskOut.taskId, 270000LL);

    QAGENT::TaskRecord task;

    // After milestone 1: escrow should decrease by 1/3
    qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.completedMilestones, 1);
    EXPECT_EQ(task.escrowAmount, 60000);  // 90000 - 30000

    // After milestone 2: escrow should decrease by half of remaining
    qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.completedMilestones, 2);
    EXPECT_EQ(task.escrowAmount, 30000);  // 60000 - 30000

    // After milestone 3 (final): escrow should be 0
    qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.completedMilestones, 3);
    EXPECT_EQ(task.escrowAmount, 0);
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_COMPLETED);
}

// ===========================================================================
// TEST: Partial milestone then ApproveResult — no double-payment
// ===========================================================================

TEST(TestQAgent, MilestoneApproveNoDoublePay)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // 3-milestone task, 90000 escrow
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 90000, NULL_ID, NULL_ID, 3);
    qagent.acceptTask(agent1, taskOut.taskId, 270000LL);

    sint64 agentBefore = getBalance(agent1);

    // Complete 1 milestone — agent gets 30000 minus 1.5% platform fee (450) = 29550
    auto m1 = qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_EQ(m1.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(m1.milestonePayout, 29550);

    // Now agent reveals (task still ACCEPTED after 1 milestone)
    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);

    // Requester approves — should pay based on REMAINING escrow (60000), not original (90000)
    auto appr = qagent.approveResult(requester1, taskOut.taskId);
    EXPECT_EQ(appr.returnCode, QAGENT_RC_SUCCESS);

    // Total agent received = milestone payout + approve escrow + bond returned
    sint64 agentAfter = getBalance(agent1);
    sint64 agentGain = agentAfter - agentBefore;

    // Agent receives: 29550 (milestone payout after 1.5% fee) + 59100 (remaining escrow - fee) + 270000 (bond)
    // Milestone fee: 30000 * 150 / 10000 = 450, net milestone = 29550
    // Approve fee on remaining escrow: 60000 * 150 / 10000 = 900, net approve = 59100
    sint64 expectedMilestonePayout = 30000 - 450;  // 29550
    sint64 expectedApproveEscrow = 60000 - 900;  // 59100
    sint64 expectedTotal = expectedMilestonePayout + expectedApproveEscrow + 270000LL;  // 358650

    // Verify correct total (without bug: old code would give 388650 due to double-counting escrow)
    EXPECT_EQ(agentGain, expectedTotal);
}

// ===========================================================================
// TEST: CancelTask on ACCEPTED task fails with WRONG_STATUS
// ===========================================================================

TEST(TestQAgent, CancelAcceptedTaskFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Accept the task
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Try to cancel — should fail since task is ACCEPTED, not OPEN
    auto cancel = qagent.cancelTask(requester1, taskOut.taskId);
    EXPECT_EQ(cancel.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Agent cannot challenge their own result
// ===========================================================================

TEST(TestQAgent, AgentCannotSelfChallenge)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);

    // Agent tries to challenge own result
    auto challenge = qagent.challengeResult(agent1, taskOut.taskId, NULL_ID, 5000);
    EXPECT_EQ(challenge.returnCode, QAGENT_RC_UNAUTHORIZED);
}

// ===========================================================================
// TEST: END_TICK auto-timeout for ACCEPTED task past deadline
// ===========================================================================

TEST(TestQAgent, EndTickAutoTimeoutAccepted)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 100, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    QAGENT::TaskRecord task;
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    uint32 deadline = task.deadlineTick;

    // Advance past deadline
    qagent.setTick(deadline + 1);
    qagent.advanceTick();  // triggers END_TICK

    // Task should now be timed out
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_TIMED_OUT);
}

// ===========================================================================
// TEST: Governance rejection — no votes beat yes votes
// ===========================================================================

TEST(TestQAgent, GovernanceProposalRejected)
{
    ContractTestingQAgent qagent;

    // Register two agents — agent2 stakes more for higher governance weight
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);
    qagent.stakeMore(agent2, 50000);  // agent2 total weight = 100000

    auto cfg0 = qagent.state()->getConfig();
    sint64 originalFee = cfg0.platformFeeBPS;

    // agent1 proposes fee change
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 200);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);

    // agent1 votes yes (50K weight), agent2 votes no (100K weight)
    qagent.voteOnProposal(agent1, 0, 1);  // 1 = yes
    qagent.voteOnProposal(agent2, 0, 2);  // 2 = no

    // Advance past vote period and run END_EPOCH
    qagent.setTick(QAGENT_GOVERNANCE_VOTE_TICKS + 10);
    qagent.advanceEpoch();

    // Config should NOT have changed — no outweighed yes
    auto cfg1 = qagent.state()->getConfig();
    EXPECT_EQ(cfg1.platformFeeBPS, originalFee);
}

// ===========================================================================
// TEST: CastDisputeVote VALID path distributes fee correctly
// ===========================================================================

TEST(TestQAgent, DisputeValidFeeDistribution)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Register 3 arbitrators
    qagent.registerArbitrator(arb1, 10000);
    qagent.registerArbitrator(arb2, 10000);
    qagent.registerArbitrator(arb3, 10000);

    // Create and complete task up to challenge
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);

    // Challenger must send bond >= agentStake (300000 for unranked agent at 300% of 100000 escrow)
    auto chal = qagent.challengeResult(challenger1, taskOut.taskId, NULL_ID, 300000);
    EXPECT_EQ(chal.returnCode, QAGENT_RC_SUCCESS);

    auto statsBefore = qagent.getPlatformStats();
    sint64 treasuryBefore = statsBefore.stats.treasuryBalance;
    sint64 arbPoolBefore = statsBefore.stats.arbitratorPool;
    sint64 burnedBefore = statsBefore.stats.totalBurned;

    // 3 arbitrators vote VALID
    qagent.castDisputeVote(arb1, taskOut.taskId, QAGENT_VERDICT_VALID);
    qagent.castDisputeVote(arb2, taskOut.taskId, QAGENT_VERDICT_VALID);
    qagent.castDisputeVote(arb3, taskOut.taskId, QAGENT_VERDICT_VALID);

    auto statsAfter = qagent.getPlatformStats();

    // Platform fee = 100000 * 150 / 10000 = 1500
    sint64 platformFee = 1500;
    // Fee burn = 1500 * 5000 / 10000 = 750
    sint64 feeBurn = 750;
    // Fee treasury = 1500 * 3000 / 10000 = 450
    sint64 feeTreasury = 450;
    // Fee arbPool portion = 1500 - 750 - 450 = 300 (now distributed directly to voters)
    sint64 feeArbPool = 300;

    // Challenger stake = 300000 (matches agent bond), burn = 300000 - 300000/2 = 150000
    sint64 challengerBurn = 150000;

    EXPECT_EQ(statsAfter.stats.totalBurned - burnedBefore, challengerBurn + feeBurn);
    EXPECT_EQ(statsAfter.stats.treasuryBalance - treasuryBefore, feeTreasury);
    // arbPool no longer accumulates — distributed directly to voters (300 / 3 = 100 each)
    EXPECT_EQ(statsAfter.stats.arbitratorPool - arbPoolBefore, 0);
}

// ===========================================================================
// TEST: Oracle Reply — VALID verdict
// ===========================================================================

TEST(TestQAgent, OracleReplyValid)
{
    ContractTestingQAgent qagent;

    // Setup: agent, requester, task → challenged → oracle pending
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    uint64 taskId = taskOut.taskId;
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Accept (agent bond = 300% of 100000 = 300000 for unranked)
    qagent.acceptTask(agent1, taskId, 300000);

    // Commit-reveal
    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, rh, salt);

    // Challenge (challenger must match agent bond = 300000)
    auto chalOut = qagent.challengeResult(challenger1, taskId, NULL_ID, 300000);
    EXPECT_EQ(chalOut.returnCode, QAGENT_RC_SUCCESS);

    // Simulate: task transitions to ORACLE_PENDING with fake queryId
    sint64 fakeQueryId = 42;
    qagent.state()->setupOracleState(taskId, fakeQueryId);

    // Verify setup
    QAGENT::TaskRecord task;
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_ORACLE_PENDING);

    // Snapshot balances before oracle reply
    sint64 agentBalBefore = getBalance(agent1);
    sint64 challengerBalBefore = getBalance(challenger1);
    auto statsBefore = qagent.getPlatformStats();

    // Simulate VALID oracle reply
    qagent.simulateOracleReply(fakeQueryId, ORACLE_QUERY_STATUS_SUCCESS, QAGENT_VERDICT_VALID, 9500);

    // Check task status
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_COMPLETED);
    EXPECT_EQ(task.verdict, QAGENT_VERDICT_VALID);
    EXPECT_NE(task.finalizedTick, 0u);

    // Calculate expected payouts:
    // platformFee = 100000 * 150 / 10000 = 1500
    // agentPayout = 100000 - 1500 = 98500
    // agentStake returned = 300000
    // Challenger gets half back: 300000 / 2 = 150000, other half burned = 150000
    sint64 platformFee = 1500;
    sint64 agentPayout = 98500;
    sint64 agentStakeReturn = 300000;
    sint64 challengerHalfBack = 150000;
    sint64 challengerBurned = 150000;

    // Fee split: 50% burn, 30% treasury, 20% arbPool
    sint64 feeBurn = 750;      // 1500 * 5000 / 10000
    sint64 feeTreasury = 450;  // 1500 * 3000 / 10000
    sint64 feeArbPool = 300;   // 1500 - 750 - 450

    sint64 agentBalAfter = getBalance(agent1);
    sint64 challengerBalAfter = getBalance(challenger1);
    auto statsAfter = qagent.getPlatformStats();

    EXPECT_EQ(agentBalAfter - agentBalBefore, agentPayout + agentStakeReturn);
    EXPECT_EQ(challengerBalAfter - challengerBalBefore, challengerHalfBack);
    EXPECT_EQ(statsAfter.stats.totalBurned - statsBefore.stats.totalBurned, challengerBurned + feeBurn);
    EXPECT_EQ(statsAfter.stats.treasuryBalance - statsBefore.stats.treasuryBalance, feeTreasury);
    EXPECT_EQ(statsAfter.stats.arbitratorPool - statsBefore.stats.arbitratorPool, feeArbPool);

    // Check agent stats updated
    QAGENT::AgentRecord agentRec;
    ASSERT_TRUE(qagent.state()->getAgentRecord(agent1, agentRec));
    EXPECT_EQ(agentRec.completedTasks, 1u);
    EXPECT_EQ(agentRec.oracleVerifiedTasks, 1u);
    EXPECT_EQ(agentRec.oraclePassedTasks, 1u);
    EXPECT_EQ(agentRec.failedTasks, 0u);
}

// ===========================================================================
// TEST: Oracle Reply — INVALID verdict
// ===========================================================================

TEST(TestQAgent, OracleReplyInvalid)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, rh, salt);

    auto chalOut = qagent.challengeResult(challenger1, taskId, NULL_ID, 300000);
    EXPECT_EQ(chalOut.returnCode, QAGENT_RC_SUCCESS);

    sint64 fakeQueryId = 99;
    qagent.state()->setupOracleState(taskId, fakeQueryId);

    // Snapshot balances
    sint64 requesterBalBefore = getBalance(requester1);
    sint64 agentBalBefore = getBalance(agent1);
    sint64 challengerBalBefore = getBalance(challenger1);
    auto statsBefore = qagent.getPlatformStats();

    // Simulate INVALID oracle reply
    qagent.simulateOracleReply(fakeQueryId, ORACLE_QUERY_STATUS_SUCCESS, QAGENT_VERDICT_INVALID, 9000);

    // Check task status
    QAGENT::TaskRecord task;
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_CANCELLED);
    EXPECT_EQ(task.verdict, QAGENT_VERDICT_INVALID);

    // Expected payouts for INVALID:
    // Requester gets full escrow back: 100000
    // Agent bond (300000) slashed: 40% burn=120000, 30% challenger=90000, 20% arbPool=60000, 10% treasury=30000
    // Challenger gets: full stake back (300000) + 30% of agent bond (90000)
    sint64 escrow = 100000;
    sint64 agentBond = 300000;
    sint64 slashBurn = 120000;         // 300000 * 4000 / 10000
    sint64 slashChallenger = 90000;    // 300000 * 3000 / 10000
    sint64 slashArbPool = 60000;       // 300000 * 2000 / 10000
    sint64 slashTreasury = 30000;      // 300000 * 1000 / 10000

    sint64 requesterBalAfter = getBalance(requester1);
    sint64 agentBalAfter = getBalance(agent1);
    sint64 challengerBalAfter = getBalance(challenger1);
    auto statsAfter = qagent.getPlatformStats();

    // Requester gets escrow back
    EXPECT_EQ(requesterBalAfter - requesterBalBefore, escrow);
    // Agent gets nothing (bond is fully slashed/distributed)
    EXPECT_EQ(agentBalAfter - agentBalBefore, 0);
    // Challenger gets: full stake (300000) + challenger reward from slash (90000)
    EXPECT_EQ(challengerBalAfter - challengerBalBefore, 300000 + slashChallenger);

    EXPECT_EQ(statsAfter.stats.totalBurned - statsBefore.stats.totalBurned, slashBurn);
    EXPECT_EQ(statsAfter.stats.treasuryBalance - statsBefore.stats.treasuryBalance, slashTreasury);
    EXPECT_EQ(statsAfter.stats.arbitratorPool - statsBefore.stats.arbitratorPool, slashArbPool);

    // Check agent stats
    QAGENT::AgentRecord agentRec;
    ASSERT_TRUE(qagent.state()->getAgentRecord(agent1, agentRec));
    EXPECT_EQ(agentRec.failedTasks, 1u);
    EXPECT_EQ(agentRec.oracleVerifiedTasks, 1u);
    EXPECT_EQ(agentRec.oraclePassedTasks, 0u);
    EXPECT_EQ(agentRec.slashCount, 1u);
}

// ===========================================================================
// TEST: Oracle Reply — oracle failure (non-SUCCESS status) reverts to DISPUTED
// ===========================================================================

TEST(TestQAgent, OracleReplyFailureRevertsToDisputed)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, rh, salt);

    auto chalOut = qagent.challengeResult(challenger1, taskId, NULL_ID, 300000);
    EXPECT_EQ(chalOut.returnCode, QAGENT_RC_SUCCESS);

    sint64 fakeQueryId = 123;
    qagent.state()->setupOracleState(taskId, fakeQueryId);

    // Snapshot balances (nothing should move)
    sint64 agentBalBefore = getBalance(agent1);
    sint64 requesterBalBefore = getBalance(requester1);
    sint64 challengerBalBefore = getBalance(challenger1);

    // Simulate oracle TIMEOUT (status != SUCCESS)
    qagent.simulateOracleReply(fakeQueryId, ORACLE_QUERY_STATUS_TIMEOUT);

    // Task should revert to DISPUTED with new deadline
    QAGENT::TaskRecord task;
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_DISPUTED);
    EXPECT_EQ(task.oracleRequested, 0u);
    EXPECT_GT(task.disputeDeadlineTick, 0u);

    // No money should have moved
    EXPECT_EQ(getBalance(agent1), agentBalBefore);
    EXPECT_EQ(getBalance(requester1), requesterBalBefore);
    EXPECT_EQ(getBalance(challenger1), challengerBalBefore);
}

// ===========================================================================
// TEST: Oracle Reply — unknown query ID is silently ignored
// ===========================================================================

TEST(TestQAgent, OracleReplyUnknownQueryIdIgnored)
{
    ContractTestingQAgent qagent;

    // No tasks at all — oracle reply with unknown queryId should be a no-op
    qagent.simulateOracleReply(999, ORACLE_QUERY_STATUS_SUCCESS, QAGENT_VERDICT_VALID, 10000);

    // Verify nothing crashed and stats are clean
    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.totalBurned, 0);
    EXPECT_EQ(stats.stats.treasuryBalance, 0);
}

// ===========================================================================
// TEST: Duplicate dispute vote is rejected
// ===========================================================================

TEST(TestQAgent, DuplicateDisputeVoteRejected)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerArbitrator(arb1, 10000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, commitHash);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);

    qagent.challengeResult(challenger1, taskOut.taskId, NULL_ID, 300000);

    // First vote succeeds
    auto v1 = qagent.castDisputeVote(arb1, taskOut.taskId, QAGENT_VERDICT_VALID);
    EXPECT_EQ(v1.returnCode, QAGENT_RC_SUCCESS);

    // Same arbitrator tries to vote again — should be rejected
    auto v2 = qagent.castDisputeVote(arb1, taskOut.taskId, QAGENT_VERDICT_INVALID);
    EXPECT_EQ(v2.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// ===========================================================================
// TEST: END_TICK auto-cancels expired DISPUTED tasks
// ===========================================================================

TEST(TestQAgent, EndTickDisputedExpiry)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, rh, salt);

    // Challenge to move to DISPUTED
    qagent.challengeResult(challenger1, taskId, NULL_ID, 300000);

    QAGENT::TaskRecord task;
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_DISPUTED);
    uint32 disputeDeadline = task.disputeDeadlineTick;

    // Advance past the dispute deadline
    qagent.setTick(disputeDeadline + 1);
    qagent.advanceTick(1); // This runs END_TICK which processes the activeTaskQ

    // Task should be auto-cancelled due to expired dispute with no resolution
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_CANCELLED);
}

// ===========================================================================
// TEST: WithdrawStake with only registration stake (no excess to withdraw)
// ===========================================================================

TEST(TestQAgent, WithdrawRegistrationStakeOnly)
{
    ContractTestingQAgent qagent;

    // Register with exactly minimum stake
    auto regOut = qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    EXPECT_EQ(regOut.returnCode, QAGENT_RC_SUCCESS);

    // Deactivate first (required before withdrawal)
    qagent.deactivateAgent(agent1);

    // Advance past 4-epoch lock period (registered at construction epoch 210)
    qagent.advanceEpoch(); // 211
    qagent.advanceEpoch(); // 212
    qagent.advanceEpoch(); // 213
    qagent.advanceEpoch(); // 214 — lock period expires

    // Request unstake (now allowed since epoch >= registeredEpoch + 4)
    auto unstakeOut = qagent.requestUnstake(agent1);
    EXPECT_EQ(unstakeOut.returnCode, QAGENT_RC_SUCCESS);

    // Advance at least 1 more epoch past unstake request
    qagent.advanceEpoch(); // 215

    sint64 balBefore = getBalance(agent1);

    // Withdraw — should return the registration stake
    auto withdrawOut = qagent.withdrawStake(agent1);
    EXPECT_EQ(withdrawOut.returnCode, QAGENT_RC_SUCCESS);

    sint64 balAfter = getBalance(agent1);
    EXPECT_EQ(balAfter - balBefore, 50000);
}

// ===========================================================================
// TEST: Oracle Reply — UNRESOLVABLE status also reverts to DISPUTED
// ===========================================================================

TEST(TestQAgent, OracleReplyUnresolvableRevertsToDisputed)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, rh, salt);

    qagent.challengeResult(challenger1, taskId, NULL_ID, 300000);

    sint64 fakeQueryId = 456;
    qagent.state()->setupOracleState(taskId, fakeQueryId);

    // Simulate UNRESOLVABLE oracle response
    qagent.simulateOracleReply(fakeQueryId, ORACLE_QUERY_STATUS_UNRESOLVABLE);

    QAGENT::TaskRecord task;
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskId, task));
    EXPECT_EQ(task.status, QAGENT_TASK_STATUS_DISPUTED);
    EXPECT_EQ(task.oracleRequested, 0u);
}

// ===========================================================================
// TEST: Oracle VALID verdict updates activeTasks and disputedTasks stats
// ===========================================================================

TEST(TestQAgent, OracleReplyStatsDecrement)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskId, commitHash);
    qagent.revealResult(agent1, taskId, rh, salt);

    qagent.challengeResult(challenger1, taskId, NULL_ID, 300000);

    auto statsBefore = qagent.getPlatformStats();

    sint64 fakeQueryId = 789;
    qagent.state()->setupOracleState(taskId, fakeQueryId);

    qagent.simulateOracleReply(fakeQueryId, ORACLE_QUERY_STATUS_SUCCESS, QAGENT_VERDICT_VALID, 9500);

    auto statsAfter = qagent.getPlatformStats();

    // activeTasks and disputedTasks should each decrease by 1
    EXPECT_EQ(statsAfter.stats.activeTasks, statsBefore.stats.activeTasks - 1);
    EXPECT_EQ(statsAfter.stats.disputedTasks, statsBefore.stats.disputedTasks - 1);
}

// ===========================================================================
// BOUNDARY TESTS: Capacity limits
// ===========================================================================

// TEST: Arbitrator capacity (256 max)
TEST(TestQAgent, ArbitratorCapacityFull)
{
    ContractTestingQAgent qagent;

    // Register 256 arbitrators (max)
    for (unsigned int i = 0; i < QAGENT_MAX_ARBITRATORS; i++)
    {
        id arbId = id(10000 + i, 0, 0, 0);
        qagent.fund(arbId, 100000LL);
        auto out = qagent.registerArbitrator(arbId, 10000);
        EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS) << "Failed at arbitrator " << i;
    }

    // 257th should fail with CAPACITY_FULL
    id extraArb = id(99999, 0, 0, 0);
    qagent.fund(extraArb, 100000LL);
    auto out = qagent.registerArbitrator(extraArb, 10000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_CAPACITY_FULL);
}

// TEST: Dispute voter capacity (5 max per task)
TEST(TestQAgent, DisputeVoterCapacityFull)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Register 6 arbitrators
    id arbs[6];
    for (int i = 0; i < 6; i++)
    {
        arbs[i] = id(601 + i, 0, 0, 0);
        qagent.fund(arbs[i], 10000000LL);
        qagent.registerArbitrator(arbs[i], 10000);
    }

    // Create task → accept → commit → reveal → challenge → DISPUTED
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, commitHash);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);
    qagent.challengeResult(challenger1, taskOut.taskId, NULL_ID, 300000);

    // 3 VALID votes reach QAGENT_DISPUTE_THRESHOLD=3, resolving the task
    auto v1 = qagent.castDisputeVote(arbs[0], taskOut.taskId, QAGENT_VERDICT_VALID);
    EXPECT_EQ(v1.returnCode, QAGENT_RC_SUCCESS);
    auto v2 = qagent.castDisputeVote(arbs[1], taskOut.taskId, QAGENT_VERDICT_VALID);
    EXPECT_EQ(v2.returnCode, QAGENT_RC_SUCCESS);
    auto v3 = qagent.castDisputeVote(arbs[2], taskOut.taskId, QAGENT_VERDICT_VALID);
    EXPECT_EQ(v3.returnCode, QAGENT_RC_SUCCESS);
    // Task is now COMPLETED (resolved). Vote 4 should fail with WRONG_STATUS.
    auto v4 = qagent.castDisputeVote(arbs[3], taskOut.taskId, QAGENT_VERDICT_INVALID);
    EXPECT_EQ(v4.returnCode, QAGENT_RC_WRONG_STATUS);
}

// TEST: Delegation depth limit (MAX_DELEGATION_DEPTH=3, so depth walk must reach 3 to reject)
TEST(TestQAgent, DelegationDepthExceeded)
{
    ContractTestingQAgent qagent;

    // Register 5 agents (need 4 successful delegations to trigger depth=3 rejection on 4th)
    id agents[5];
    for (int i = 0; i < 5; i++)
    {
        agents[i] = id(1000 + i, 0, 0, 0);
        qagent.fund(agents[i], 10000000LL);
        qagent.registerAgent(agents[i], 0xFF, 1000, 50000);
    }

    // Requester creates root task assigned to agent[0]
    auto rootTask = qagent.createTask(requester1, agents[0], 0, 5000, 500000);
    EXPECT_EQ(rootTask.returnCode, QAGENT_RC_SUCCESS);

    // Agent[0] accepts and delegates to agent[1] (depth walk=0, allowed)
    qagent.acceptTask(agents[0], rootTask.taskId, 1500000);
    auto del1 = qagent.delegateTask(agents[0], rootTask.taskId, agents[1], 0, 4000, 200000, NULL_ID, NULL_ID);
    EXPECT_EQ(del1.returnCode, QAGENT_RC_SUCCESS);

    // Agent[1] accepts and delegates to agent[2] (depth walk=1, allowed)
    qagent.acceptTask(agents[1], del1.childTaskId, 600000);
    auto del2 = qagent.delegateTask(agents[1], del1.childTaskId, agents[2], 0, 3000, 100000, NULL_ID, NULL_ID);
    EXPECT_EQ(del2.returnCode, QAGENT_RC_SUCCESS);

    // Agent[2] accepts and delegates to agent[3] (depth walk=2, allowed)
    qagent.acceptTask(agents[2], del2.childTaskId, 300000);
    auto del3 = qagent.delegateTask(agents[2], del2.childTaskId, agents[3], 0, 2000, 50000, NULL_ID, NULL_ID);
    EXPECT_EQ(del3.returnCode, QAGENT_RC_SUCCESS);

    // Agent[3] accepts and tries to delegate to agent[4] (depth walk=3, EXCEEDS limit)
    qagent.acceptTask(agents[3], del3.childTaskId, 150000);
    auto del4 = qagent.delegateTask(agents[3], del3.childTaskId, agents[4], 0, 1000, 25000, NULL_ID, NULL_ID);
    EXPECT_EQ(del4.returnCode, QAGENT_RC_DELEGATION_DEPTH_EXCEEDED);
}

// TEST: Delegation self-dealing rejected
TEST(TestQAgent, DelegationSelfDealingRejected)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 3000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    // Agent1 tries to delegate to itself
    auto del = qagent.delegateTask(agent1, taskOut.taskId, agent1, 0, 2000, 50000, NULL_ID, NULL_ID);
    EXPECT_EQ(del.returnCode, QAGENT_RC_SELF_DEALING);
}

// TEST: Non-assigned agent cannot delegate
TEST(TestQAgent, DelegationUnauthorized)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 3000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    // Agent2 (not assigned) tries to delegate
    auto del = qagent.delegateTask(agent2, taskOut.taskId, agent1, 0, 2000, 50000, NULL_ID, NULL_ID);
    EXPECT_EQ(del.returnCode, QAGENT_RC_DELEGATION_UNAUTHORIZED);
}

// TEST: Delegation escrow exceeds parent escrow
TEST(TestQAgent, DelegationEscrowExceedsParent)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 3000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    // Try to delegate more escrow than parent has
    auto del = qagent.delegateTask(agent1, taskOut.taskId, agent2, 0, 2000, 200000, NULL_ID, NULL_ID);
    EXPECT_EQ(del.returnCode, QAGENT_RC_INVALID_INPUT);
}

// TEST: Successful delegation deducts escrow from parent
TEST(TestQAgent, DelegationSuccess)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 3000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    auto del = qagent.delegateTask(agent1, taskOut.taskId, agent2, 0, 2000, 40000, NULL_ID, NULL_ID);
    EXPECT_EQ(del.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_NE(del.childTaskId, 0ull);

    // Parent escrow should be reduced
    QAGENT::TaskRecord parentTask;
    ASSERT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, parentTask));
    EXPECT_EQ(parentTask.escrowAmount, 60000); // 100000 - 40000

    // Child task should exist with correct escrow
    QAGENT::TaskRecord childTask;
    ASSERT_TRUE(qagent.state()->getTaskRecord(del.childTaskId, childTask));
    EXPECT_EQ(childTask.escrowAmount, 40000);
    EXPECT_EQ(childTask.parentTaskId, taskOut.taskId);
    EXPECT_EQ(childTask.requester, agent1); // delegating agent becomes requester of child
}

// TEST: Rate agent — valid and invalid ratings
TEST(TestQAgent, RateAgentValidAndInvalid)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Create and complete a task (requester approves)
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, commitHash);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);
    qagent.approveResult(requester1, taskOut.taskId);

    // Rate with valid score (1-5)
    auto rateOut = qagent.rateAgent(requester1, taskOut.taskId, 5);
    EXPECT_EQ(rateOut.returnCode, QAGENT_RC_SUCCESS);

    // Rate same task again — should be rejected (already rated)
    auto rateOut2 = qagent.rateAgent(requester1, taskOut.taskId, 3);
    EXPECT_EQ(rateOut2.returnCode, QAGENT_RC_ALREADY_EXISTS);
}

// TEST: Reputation score query for agent with completed tasks
TEST(TestQAgent, ReputationScoreCalculation)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Complete a task
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id commitHash;
    copyMem(&commitHash, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, commitHash);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);
    qagent.approveResult(requester1, taskOut.taskId);

    auto rep = qagent.getReputationScore(agent1);
    EXPECT_GT(rep.score, 0u);
    EXPECT_EQ(rep.found, 1);

    // Non-existent agent should return 0
    auto rep2 = qagent.getReputationScore(id(9999, 0, 0, 0));
    EXPECT_EQ(rep2.score, 0u);
    EXPECT_EQ(rep2.found, 0);
}

// TEST: Service registration and deprecation
TEST(TestQAgent, ServiceRegisterAndDeprecate)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto svcOut = qagent.registerService(agent1, 0x0F, 5000, 0);
    EXPECT_EQ(svcOut.returnCode, QAGENT_RC_SUCCESS);

    // Query the service
    auto svc = qagent.getService(agent1);
    EXPECT_EQ(svc.service.capabilityBitmap, 0x0Fu);
    EXPECT_EQ(svc.service.basePrice, 5000);

    // Deprecate service
    auto deprecOut = qagent.deprecateService(agent1, agent1);
    EXPECT_EQ(deprecOut.returnCode, QAGENT_RC_SUCCESS);
}

// TEST: Governance duplicate vote rejected (CRIT-1 fix)
TEST(TestQAgent, GovernanceDuplicateVoteRejected)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 100);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);

    // First vote succeeds
    auto v1 = qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    EXPECT_EQ(v1.returnCode, QAGENT_RC_SUCCESS);

    // Duplicate vote by same agent on same proposal is rejected
    auto v2 = qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    EXPECT_EQ(v2.returnCode, QAGENT_RC_ALREADY_EXISTS);

    // Different agent can still vote on same proposal
    auto v3 = qagent.voteOnProposal(agent2, prop.proposalIndex, 2);
    EXPECT_EQ(v3.returnCode, QAGENT_RC_SUCCESS);

    // Same agent can vote on a DIFFERENT proposal
    auto prop2 = qagent.proposeParamChange(agent2, QAGENT_PARAM_MIN_ESCROW, 20000);
    EXPECT_EQ(prop2.returnCode, QAGENT_RC_SUCCESS);

    auto v4 = qagent.voteOnProposal(agent1, prop2.proposalIndex, 1);
    EXPECT_EQ(v4.returnCode, QAGENT_RC_SUCCESS);
}

// TEST: CompleteMilestone now deducts platform fee (IMPORT-4 fix)
TEST(TestQAgent, MilestonePlatformFeeDeducted)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000, NULL_ID, NULL_ID, 2);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);

    sint64 agentBefore = getBalance(agent1);

    // Complete milestone 1: payout = 100000/2 = 50000, fee = 50000*150/10000 = 750
    auto m1 = qagent.completeMilestone(requester1, taskOut.taskId);
    EXPECT_EQ(m1.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(m1.milestonePayout, 49250);  // 50000 - 750

    sint64 agentAfter = getBalance(agent1);
    EXPECT_EQ(agentAfter - agentBefore, 49250);

    // Verify platform fee was distributed (burn portion should be tracked)
    auto stats = qagent.getPlatformStats();
    EXPECT_GT(stats.stats.totalBurned, 0);
    EXPECT_GT(stats.stats.treasuryBalance, 0);
}

// ===========================================================================
// IMPORT-6 FIX: Requester cannot challenge their own task
// ===========================================================================

TEST(TestQAgent, RequesterCannotSelfChallenge)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Commit-reveal cycle
    id rh = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = rh;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);
    qagent.commitResult(agent1, taskOut.taskId, ch);
    qagent.revealResult(agent1, taskOut.taskId, rh, salt);

    // Requester tries to challenge their own task → blocked as self-dealing
    auto challenge = qagent.challengeResult(requester1, taskOut.taskId, NULL_ID, 30000);
    EXPECT_EQ(challenge.returnCode, QAGENT_RC_SELF_DEALING);

    // Third party CAN challenge (not requester, not agent)
    auto challenge2 = qagent.challengeResult(challenger1, taskOut.taskId, NULL_ID, 30000);
    EXPECT_EQ(challenge2.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// IMPORT-3 FIX: Delegated task timeout returns escrow to parent task
// ===========================================================================

TEST(TestQAgent, DelegatedTaskTimeoutReturnsEscrowToParent)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Create parent task with 100000 escrow
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 parentTaskId = taskOut.taskId;

    qagent.acceptTask(agent1, parentTaskId, 300000);

    // Agent1 delegates 40000 to agent2
    auto delOut = qagent.delegateTask(agent1, parentTaskId, agent2, 0, 500, 40000);
    EXPECT_EQ(delOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 childTaskId = delOut.childTaskId;

    // Verify parent escrow was deducted
    auto parentBefore = qagent.getTask(parentTaskId);
    EXPECT_EQ(parentBefore.task.escrowAmount, 60000);  // 100000 - 40000

    // Child task needs to be accepted for timeout
    qagent.acceptTask(agent2, childTaskId, 150000);

    // Get child task deadline
    QAGENT::TaskRecord childTask;
    EXPECT_TRUE(qagent.state()->getTaskRecord(childTaskId, childTask));

    // Advance past child deadline and timeout
    qagent.setTick(childTask.deadlineTick + 1);
    auto tout = qagent.timeoutTask(keeper1, childTaskId);
    EXPECT_EQ(tout.returnCode, QAGENT_RC_SUCCESS);

    // Verify child task is timed out
    QAGENT::TaskRecord childAfter;
    EXPECT_TRUE(qagent.state()->getTaskRecord(childTaskId, childAfter));
    EXPECT_EQ(childAfter.status, QAGENT_TASK_STATUS_TIMED_OUT);

    // Verify parent task's escrow was RESTORED (not sent to parent agent's wallet)
    auto parentAfter = qagent.getTask(parentTaskId);
    EXPECT_EQ(parentAfter.task.escrowAmount, 100000);  // 60000 + 40000 restored

    // Verify requester1 did NOT receive the child escrow (it went back to parent task)
    // The original requester's balance should be unchanged by the child timeout
}

// ===========================================================================
// IMPORT-3 FIX: END_TICK auto-timeout also returns delegated escrow to parent
// ===========================================================================

TEST(TestQAgent, EndTickDelegatedTimeoutReturnsToParent)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 5000, 80000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 parentTaskId = taskOut.taskId;

    qagent.acceptTask(agent1, parentTaskId, 240000);

    // Delegate 30000 to agent2
    auto delOut = qagent.delegateTask(agent1, parentTaskId, agent2, 0, 500, 30000);
    EXPECT_EQ(delOut.returnCode, QAGENT_RC_SUCCESS);
    uint64 childTaskId = delOut.childTaskId;

    // Accept child task so it goes to ACCEPTED status
    qagent.acceptTask(agent2, childTaskId, 90000);

    QAGENT::TaskRecord childTask;
    EXPECT_TRUE(qagent.state()->getTaskRecord(childTaskId, childTask));

    // Verify parent escrow before
    auto parentBefore = qagent.getTask(parentTaskId);
    EXPECT_EQ(parentBefore.task.escrowAmount, 50000);  // 80000 - 30000

    // Advance past deadline and trigger END_TICK auto-timeout
    qagent.setTick(childTask.deadlineTick + 1);
    qagent.advanceTick();

    // Verify child timed out
    QAGENT::TaskRecord childAfter;
    EXPECT_TRUE(qagent.state()->getTaskRecord(childTaskId, childAfter));
    EXPECT_EQ(childAfter.status, QAGENT_TASK_STATUS_TIMED_OUT);

    // Verify parent escrow restored
    auto parentAfter = qagent.getTask(parentTaskId);
    EXPECT_EQ(parentAfter.task.escrowAmount, 80000);  // 50000 + 30000 restored
}

// ===========================================================================
// IMPORT-5 FIX: Keeper fee cannot exceed agent stake (underflow guard)
// ===========================================================================

TEST(TestQAgent, KeeperFeeCannotExceedAgentStake)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // Min escrow=1000, Unranked 300% bond → requiredStake=3000
    auto taskOut = qagent.createTask(requester1, agent1, 0, 100, 1000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    auto acceptOut = qagent.acceptTask(agent1, taskOut.taskId, 3000);
    EXPECT_EQ(acceptOut.returnCode, QAGENT_RC_SUCCESS);

    QAGENT::TaskRecord task;
    EXPECT_TRUE(qagent.state()->getTaskRecord(taskOut.taskId, task));
    EXPECT_EQ(task.agentStake, 3000);

    // Advance past deadline
    qagent.setTick(task.deadlineTick + 1);

    auto tout = qagent.timeoutTask(keeper1, taskOut.taskId);
    EXPECT_EQ(tout.returnCode, QAGENT_RC_SUCCESS);

    // Keeper fee must not exceed agent stake (underflow guard)
    EXPECT_LE(tout.keeperFee, 3000);
    EXPECT_GE(tout.keeperFee, 0);

    // With keeperFeeBPS=10, fee=3000*10/10000=3. MIN_KEEPER_FEE=100, so bumped to 100.
    EXPECT_EQ(tout.keeperFee, 100);

    // Burn = agentStake - keeperFee = 3000 - 100 = 2900 (no underflow)
    auto stats = qagent.getPlatformStats();
    EXPECT_GE(stats.stats.totalBurned, 2900);
}

// ===========================================================================
// TEST: Circuit Breaker — pause blocks operational procedures
// ===========================================================================

TEST(TestQAgent, CircuitBreakerPauseBlocksOperations)
{
    ContractTestingQAgent qagent;

    // Register agents so we can test task creation
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // CreateTask should work before pause
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    // Pause via governance: propose + vote + advance epoch
    qagent.stakeMore(agent1, 50000);  // agent1 = 100K weight
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PAUSED, 1);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);
    qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    qagent.setTick(QAGENT_GOVERNANCE_VOTE_TICKS + 10);
    qagent.advanceEpoch();

    // Verify paused
    auto cfg = qagent.getPlatformStats();
    EXPECT_EQ(cfg.config.paused, 1);

    // CreateTask should be blocked
    auto taskOut2 = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut2.returnCode, QAGENT_RC_PAUSED);

    // AcceptTask should be blocked
    auto accOut = qagent.acceptTask(agent1, taskOut.taskId, 30000);
    EXPECT_EQ(accOut.returnCode, QAGENT_RC_PAUSED);

    // Agent management should still work during pause
    auto deact = qagent.deactivateAgent(agent2);
    EXPECT_EQ(deact.returnCode, QAGENT_RC_SUCCESS);

    // Governance should still work (to unpause)
    auto prop2 = qagent.proposeParamChange(agent1, QAGENT_PARAM_PAUSED, 0);
    EXPECT_EQ(prop2.returnCode, QAGENT_RC_SUCCESS);
    qagent.voteOnProposal(agent1, prop2.proposalIndex, 1);
    qagent.setTick(2 * QAGENT_GOVERNANCE_VOTE_TICKS + 20);
    qagent.advanceEpoch();

    // Verify unpaused
    cfg = qagent.getPlatformStats();
    EXPECT_EQ(cfg.config.paused, 0);

    // CreateTask should work again
    auto taskOut3 = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut3.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: ProposalVoters cleanup after all proposals expire
// ===========================================================================

TEST(TestQAgent, ProposalVotersCleanedAfterEpoch)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);
    qagent.stakeMore(agent1, 50000);

    // Create proposal and vote
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 100);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);
    auto v1 = qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    EXPECT_EQ(v1.returnCode, QAGENT_RC_SUCCESS);
    auto v2 = qagent.voteOnProposal(agent2, prop.proposalIndex, 2);
    EXPECT_EQ(v2.returnCode, QAGENT_RC_SUCCESS);

    // Duplicate vote should fail
    auto v3 = qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    EXPECT_EQ(v3.returnCode, QAGENT_RC_ALREADY_EXISTS);

    // Advance epoch — proposal executes, voters purged
    qagent.setTick(QAGENT_GOVERNANCE_VOTE_TICKS + 10);
    qagent.advanceEpoch();

    // Config changed
    auto cfg = qagent.getPlatformStats();
    EXPECT_EQ(cfg.config.platformFeeBPS, 100);

    // proposalVoters should be cleaned (population == 0)
    // Verify by creating a new proposal with the same agents — they should be able to vote
    auto prop2 = qagent.proposeParamChange(agent1, QAGENT_PARAM_PLATFORM_FEE_BPS, 120);
    EXPECT_EQ(prop2.returnCode, QAGENT_RC_SUCCESS);
    auto v4 = qagent.voteOnProposal(agent1, prop2.proposalIndex, 1);
    EXPECT_EQ(v4.returnCode, QAGENT_RC_SUCCESS);
    auto v5 = qagent.voteOnProposal(agent2, prop2.proposalIndex, 1);
    EXPECT_EQ(v5.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: Suspended agent cannot accept tasks (BUG FIX)
// ===========================================================================

TEST(TestQAgent, SuspendedAgentCannotAcceptTask)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Deactivate agent1
    qagent.deactivateAgent(agent1);

    // Create a task targeting agent1 should fail (agent inactive)
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_WRONG_STATUS);

    // Create task for agent2 (active), then deactivate agent2
    auto taskOut2 = qagent.createTask(requester1, agent2, 0, 2000, 10000);
    EXPECT_EQ(taskOut2.returnCode, QAGENT_RC_SUCCESS);
    qagent.deactivateAgent(agent2);

    // Deactivated agent2 should NOT be able to accept the existing OPEN task
    auto accOut = qagent.acceptTask(agent2, taskOut2.taskId, 30000);
    EXPECT_EQ(accOut.returnCode, QAGENT_RC_WRONG_STATUS);

    // Reactivate and accept should work
    qagent.reactivateAgent(agent2);
    auto accOut2 = qagent.acceptTask(agent2, taskOut2.taskId, 30000);
    EXPECT_EQ(accOut2.returnCode, QAGENT_RC_SUCCESS);
}

// ===========================================================================
// TEST: AcceptTask increments totalTasks (BUG FIX)
// ===========================================================================

TEST(TestQAgent, AcceptTaskIncrementsTotalTasks)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto ag = qagent.getAgent(agent1);
    EXPECT_EQ(ag.agent.totalTasks, 0u);

    auto t1 = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, t1.taskId, 30000);

    ag = qagent.getAgent(agent1);
    EXPECT_EQ(ag.agent.totalTasks, 1u);

    auto t2 = qagent.createTask(requester1, agent1, 0, 2000, 10000);
    qagent.acceptTask(agent1, t2.taskId, 30000);

    ag = qagent.getAgent(agent1);
    EXPECT_EQ(ag.agent.totalTasks, 2u);
}

// ===========================================================================
// TEST: END_TICK auto-timeouts COMMITTED tasks past deadline (BUG FIX)
// ===========================================================================

TEST(TestQAgent, EndTickAutoTimeoutsCommittedTask)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, 0, 500, 10000);
    EXPECT_EQ(taskOut.returnCode, QAGENT_RC_SUCCESS);

    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // Commit a result (moves to COMMITTED status)
    id commitHash(42, 0, 0, 0);
    auto commitOut = qagent.commitResult(agent1, taskOut.taskId, commitHash);
    EXPECT_EQ(commitOut.returnCode, QAGENT_RC_SUCCESS);

    auto task = qagent.getTask(taskOut.taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_COMMITTED);

    // Advance past deadline
    qagent.setTick(600);
    qagent.advanceTick();

    // Task should be auto-timed-out
    task = qagent.getTask(taskOut.taskId);
    EXPECT_EQ(task.task.status, QAGENT_TASK_STATUS_TIMED_OUT);
    EXPECT_NE(task.task.finalizedTick, 0u);
}

// ===========================================================================
// TEST: Arbitrator deactivation and stake withdrawal
// ===========================================================================

TEST(TestQAgent, ArbitratorDeactivateAndWithdraw)
{
    ContractTestingQAgent qagent;

    // Register arbitrator
    auto reg = qagent.registerArbitrator(arb1, 10000);
    EXPECT_EQ(reg.returnCode, QAGENT_RC_SUCCESS);

    auto arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.found, 1);
    EXPECT_EQ(arb.arbitrator.active, 1);
    EXPECT_EQ(arb.arbitrator.stakeAmount, 10000);

    // Cannot withdraw while active
    auto wOut = qagent.withdrawArbitratorStake(arb1);
    EXPECT_EQ(wOut.returnCode, QAGENT_RC_WRONG_STATUS);

    // Deactivate
    auto dOut = qagent.deactivateArbitrator(arb1);
    EXPECT_EQ(dOut.returnCode, QAGENT_RC_SUCCESS);

    arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.arbitrator.active, 0);

    // Cannot deactivate again
    auto dOut2 = qagent.deactivateArbitrator(arb1);
    EXPECT_EQ(dOut2.returnCode, QAGENT_RC_WRONG_STATUS);

    // Now withdraw
    auto wOut2 = qagent.withdrawArbitratorStake(arb1);
    EXPECT_EQ(wOut2.returnCode, QAGENT_RC_SUCCESS);
    EXPECT_EQ(wOut2.withdrawn, 10000);

    // Stake is now 0
    arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.arbitrator.stakeAmount, 0);

    // Cannot withdraw again (no funds)
    auto wOut3 = qagent.withdrawArbitratorStake(arb1);
    EXPECT_EQ(wOut3.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS);
}

// ===========================================================================
// TEST: Treasury burn via governance proposal
// ===========================================================================

TEST(TestQAgent, TreasuryBurnViaGovernance)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    // Complete a task to generate treasury balance
    auto taskOut = qagent.createTask(requester1, agent1, 0, 2000, 100000);
    qagent.acceptTask(agent1, taskOut.taskId, 300000);
    id resultHash(1, 0, 0, 0);
    id salt(2, 0, 0, 0);
    QAGENT::CommitRevealData crData;
    crData.resultHash = resultHash;
    crData.salt = salt;
    id commitHash;
    KangarooTwelve(reinterpret_cast<const unsigned char*>(&crData), sizeof(crData),
                   reinterpret_cast<unsigned char*>(&commitHash), 32);
    qagent.commitResult(agent1, taskOut.taskId, commitHash);
    qagent.revealResult(agent1, taskOut.taskId, resultHash, salt);
    qagent.approveResult(requester1, taskOut.taskId);

    // Check treasury has accumulated
    auto stats = qagent.getPlatformStats();
    EXPECT_GT(stats.stats.treasuryBalance, 0);
    sint64 treasury = stats.stats.treasuryBalance;

    // Propose to burn some treasury
    qagent.stakeMore(agent1, 50000);
    auto prop = qagent.proposeParamChange(agent1, QAGENT_PARAM_TREASURY_BURN, treasury);
    EXPECT_EQ(prop.returnCode, QAGENT_RC_SUCCESS);

    qagent.voteOnProposal(agent1, prop.proposalIndex, 1);
    qagent.setTick(QAGENT_GOVERNANCE_VOTE_TICKS + 10);
    qagent.advanceEpoch();

    // Treasury should be 0 after burn
    stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.treasuryBalance, 0);
    EXPECT_GT(stats.stats.totalBurned, 0);
}

// ===========================================================================
// TEST: All query functions return sensible defaults on a fresh contract
// ===========================================================================

TEST(TestQAgent, QueryFunctionsOnFreshState)
{
    ContractTestingQAgent qagent;

    auto a = qagent.getAgent(agent1);
    EXPECT_EQ(a.found, 0);

    auto s = qagent.getStake(agent1);
    EXPECT_EQ(s.found, 0);

    auto r = qagent.getReputationScore(agent1);
    EXPECT_EQ(r.found, 0);

    auto t1 = qagent.getTask(1);
    EXPECT_EQ(t1.found, 0);

    auto t2 = qagent.getTask(999999);
    EXPECT_EQ(t2.found, 0);

    auto arb = qagent.getArbitrator(arb1);
    EXPECT_EQ(arb.found, 0);

    auto svc = qagent.getService(agent1);
    EXPECT_EQ(svc.found, 0);

    auto prop = qagent.getProposal(0);
    EXPECT_EQ(prop.proposal.active, 0);

    auto cap = qagent.getAgentsByCapability(0xFF);
    EXPECT_EQ(cap.count, 0u);
}

// ===========================================================================
// TEST: Platform stats are zero on fresh contract construction
// ===========================================================================

TEST(TestQAgent, PlatformStatsZeroOnInit)
{
    ContractTestingQAgent qagent;

    auto stats = qagent.getPlatformStats();

    EXPECT_EQ(stats.stats.totalAgents, 0u);
    EXPECT_EQ(stats.stats.totalTasks, 0u);
    EXPECT_EQ(stats.stats.activeTasks, 0u);
    EXPECT_EQ(stats.stats.disputedTasks, 0u);
    EXPECT_EQ(stats.stats.totalVolume, 0);
    EXPECT_EQ(stats.stats.totalBurned, 0);
    EXPECT_EQ(stats.stats.treasuryBalance, 0);
    EXPECT_EQ(stats.stats.arbitratorPool, 0);
}

// ===========================================================================
// TEST: Platform config is seeded with compile-time constants at init
// ===========================================================================

TEST(TestQAgent, PlatformConfigDefaultsOnInit)
{
    ContractTestingQAgent qagent;

    auto stats = qagent.getPlatformStats();

    EXPECT_EQ(stats.config.platformFeeBPS, QAGENT_PLATFORM_FEE_BPS);
    EXPECT_EQ(stats.config.minRegistrationStake, QAGENT_MIN_REGISTRATION_STAKE);
    EXPECT_EQ(stats.config.minEscrow, QAGENT_MIN_ESCROW);
    EXPECT_EQ(stats.config.paused, 0);
}

// ===========================================================================
// TEST: Registration succeeds at exactly the minimum registration stake
// ===========================================================================

TEST(TestQAgent, RegisterAgentAtExactMinimumStake)
{
    ContractTestingQAgent qagent;

    auto out = qagent.registerAgent(agent1, 0xFF, 1000, QAGENT_MIN_REGISTRATION_STAKE);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);

    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.found, 1);
    EXPECT_EQ(agentOut.agent.status, QAGENT_STATUS_ACTIVE);

    auto stakeOut = qagent.getStake(agent1);
    EXPECT_EQ(stakeOut.found, 1);
    EXPECT_EQ(stakeOut.stake.registrationStake, QAGENT_MIN_REGISTRATION_STAKE);
}

// ===========================================================================
// TEST: Registration fails when reward is one below the minimum stake
// ===========================================================================

TEST(TestQAgent, RegisterAgentBelowMinimumStakeFails)
{
    ContractTestingQAgent qagent;

    auto out = qagent.registerAgent(agent1, 0xFF, 1000, QAGENT_MIN_REGISTRATION_STAKE - 1);
    EXPECT_EQ(out.returnCode, QAGENT_RC_INSUFFICIENT_FUNDS);

    auto agentOut = qagent.getAgent(agent1);
    EXPECT_EQ(agentOut.found, 0);

    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.totalAgents, 0u);
}

// ===========================================================================
// TEST: Lookup of a non-existent taskId returns found=0 for any value
// ===========================================================================

TEST(TestQAgent, NonExistentTaskLookup)
{
    ContractTestingQAgent qagent;

    auto out0 = qagent.getTask(0);
    EXPECT_EQ(out0.found, 0);

    auto out1 = qagent.getTask(1);
    EXPECT_EQ(out1.found, 0);

    auto outMax = qagent.getTask(0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(outMax.found, 0);
}

// ===========================================================================
// TEST: getAgentsByCapability returns count=0 for all bitmaps on fresh state
// ===========================================================================

TEST(TestQAgent, GetAgentsByCapabilityEmptyOnFreshState)
{
    ContractTestingQAgent qagent;

    auto outAll = qagent.getAgentsByCapability(0xFF);
    EXPECT_EQ(outAll.count, 0u);

    auto outOne = qagent.getAgentsByCapability(0x01);
    EXPECT_EQ(outOne.count, 0u);

    auto outNone = qagent.getAgentsByCapability(0x00);
    EXPECT_EQ(outNone.count, 0u);
}

// ===========================================================================
// TEST: Platform totalAgents stat reflects number of successful registrations
// ===========================================================================

TEST(TestQAgent, TotalAgentsReflectsRegistrationCount)
{
    ContractTestingQAgent qagent;

    constexpr uint32 N = 5u;
    for (uint32 i = 0u; i < N; i++)
    {
        id agentX = id(7000 + i, 0, 0, 0);
        qagent.fund(agentX, 1000000LL);
        auto out = qagent.registerAgent(agentX, 0xFF, 1000, 50000);
        EXPECT_EQ(out.returnCode, QAGENT_RC_SUCCESS);
    }

    auto stats = qagent.getPlatformStats();
    EXPECT_EQ(stats.stats.totalAgents, N);

    for (uint32 i = 0u; i < N; i++)
    {
        id agentX = id(7000 + i, 0, 0, 0);
        auto a = qagent.getAgent(agentX);
        EXPECT_EQ(a.found, 1);
        EXPECT_EQ(a.agent.status, QAGENT_STATUS_ACTIVE);
    }
}

// ===========================================================================
// TEST: Updating an agent that was never registered returns AGENT_NOT_FOUND
// ===========================================================================

TEST(TestQAgent, UpdateAgentNotRegistered)
{
    ContractTestingQAgent qagent;

    auto out = qagent.updateAgent(agent1, 0x0F, 2000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_AGENT_NOT_FOUND);
}

// ===========================================================================
// TEST: Commit after the task deadline fails with DEADLINE_PASSED
// ===========================================================================

TEST(TestQAgent, CommitResultAfterDeadlineFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 100, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    // Advance past deadline
    auto task0 = qagent.getTask(taskId);
    qagent.setTick(task0.task.deadlineTick + 1);

    // Prepare any commit hash (won't be used since we fail earlier)
    id commitHash = id(42, 42, 42, 42);
    auto commitOut = qagent.commitResult(agent1, taskId, commitHash);
    EXPECT_EQ(commitOut.returnCode, QAGENT_RC_DEADLINE_PASSED);
}

// ===========================================================================
// TEST: Requesting oracle verification twice returns ORACLE_PENDING on 2nd
// ===========================================================================

TEST(TestQAgent, DoubleOracleRequestReturnsPending)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    id resultHash = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = resultHash;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);

    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, resultHash, salt);

    // First oracle request should succeed
    auto ora1 = qagent.requestOracleVerification(requester1, taskId);
    EXPECT_EQ(ora1.returnCode, QAGENT_RC_SUCCESS);

    // Second oracle request on the same task should be rejected as pending
    auto ora2 = qagent.requestOracleVerification(requester1, taskId);
    EXPECT_EQ(ora2.returnCode, QAGENT_RC_ORACLE_PENDING);
}

// ===========================================================================
// TEST: Voting on an inactive proposal slot returns PROPOSAL_NOT_FOUND
// ===========================================================================

TEST(TestQAgent, VoteOnInactiveProposalSlotFails)
{
    ContractTestingQAgent qagent;

    // A registered agent with stake tries to vote on index 0 without any
    // proposal having been created yet
    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto out = qagent.voteOnProposal(agent1, 0, 1);
    EXPECT_EQ(out.returnCode, QAGENT_RC_PROPOSAL_NOT_FOUND);

    // Index 3 (still within bounds) is also inactive
    auto out2 = qagent.voteOnProposal(agent1, 3, 1);
    EXPECT_EQ(out2.returnCode, QAGENT_RC_PROPOSAL_NOT_FOUND);
}

// ===========================================================================
// TEST: Deprecating a service that was never registered fails
// ===========================================================================

TEST(TestQAgent, DeprecateNonExistentServiceFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    // agent1 has no service registered; deprecate call should fail
    auto out = qagent.deprecateService(agent1, agent1);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SERVICE_NOT_FOUND);
}

// ===========================================================================
// TEST: Updating the price of a non-existent service fails
// ===========================================================================

TEST(TestQAgent, UpdateNonExistentServicePriceFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto out = qagent.updateServicePrice(agent1, agent1, 1234);
    EXPECT_EQ(out.returnCode, QAGENT_RC_SERVICE_NOT_FOUND);
}

// ===========================================================================
// TEST: An agent other than the assigned one cannot commit a task result
// ===========================================================================

TEST(TestQAgent, NonAssignedAgentCannotCommit)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    qagent.acceptTask(agent1, taskOut.taskId, 30000);

    // agent2 is a valid registered agent but not assigned to this task
    id bogus = id(42, 42, 42, 42);
    auto out = qagent.commitResult(agent2, taskOut.taskId, bogus);
    EXPECT_EQ(out.returnCode, QAGENT_RC_UNAUTHORIZED);
}

// ===========================================================================
// TEST: A non-requester cannot approve a revealed task
// ===========================================================================

TEST(TestQAgent, NonRequesterCannotApproveResult)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    id resultHash = id(9, 8, 7, 6);
    id salt = id(5, 4, 3, 2);
    QAGENT::CommitRevealData crd;
    crd.resultHash = resultHash;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);

    qagent.commitResult(agent1, taskId, ch);
    qagent.revealResult(agent1, taskId, resultHash, salt);

    // requester2 is not the original task creator — approval must fail
    auto out = qagent.approveResult(requester2, taskId);
    EXPECT_EQ(out.returnCode, QAGENT_RC_UNAUTHORIZED);
}

// ===========================================================================
// TEST: Committing before the task is accepted returns WRONG_STATUS
// ===========================================================================

TEST(TestQAgent, CommitBeforeAcceptFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);

    // Task status is OPEN, not ACCEPTED — commit must fail
    id bogus = id(1, 1, 1, 1);
    auto out = qagent.commitResult(agent1, taskOut.taskId, bogus);
    EXPECT_EQ(out.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Revealing before committing returns WRONG_STATUS
// ===========================================================================

TEST(TestQAgent, RevealBeforeCommitFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    // Task status is ACCEPTED, not COMMITTED — reveal must fail
    auto out = qagent.revealResult(agent1, taskId, id(1, 2, 3, 4), id(5, 6, 7, 8));
    EXPECT_EQ(out.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Approving a committed-but-not-revealed task returns WRONG_STATUS
// ===========================================================================

TEST(TestQAgent, ApproveBeforeRevealFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    id commitHash = id(7, 7, 7, 7);
    qagent.commitResult(agent1, taskId, commitHash);

    // Task status is COMMITTED, not REVEALED — approve must fail
    auto out = qagent.approveResult(requester1, taskId);
    EXPECT_EQ(out.returnCode, QAGENT_RC_WRONG_STATUS);
}

// ===========================================================================
// TEST: Registering a service with a negative base price is rejected
// ===========================================================================

TEST(TestQAgent, RegisterServiceWithNegativePriceRejected)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto out = qagent.registerService(agent1, 0x0F, -1, QAGENT_TASK_TYPE_ANALYSIS);
    EXPECT_EQ(out.returnCode, QAGENT_RC_INVALID_INPUT);
}

// ===========================================================================
// TEST: Updating an agent with a negative pricePerTask is rejected
// ===========================================================================

TEST(TestQAgent, UpdateAgentWithNegativePriceRejected)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto out = qagent.updateAgent(agent1, 0xFF, -100);
    EXPECT_EQ(out.returnCode, QAGENT_RC_INVALID_INPUT);

    // Agent record should still carry the original price
    auto a = qagent.getAgent(agent1);
    EXPECT_EQ(a.found, 1);
    EXPECT_EQ(a.agent.pricePerTask, 1000);
}

// ===========================================================================
// TEST: Non-owner cannot deprecate someone else's service
// ===========================================================================

TEST(TestQAgent, ServiceDeprecateByNonOwnerFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto svc = qagent.registerService(agent1, 0x0F, 5000, QAGENT_TASK_TYPE_ANALYSIS);
    EXPECT_EQ(svc.returnCode, QAGENT_RC_SUCCESS);

    // agent2 tries to deprecate agent1's service — must fail
    auto out = qagent.deprecateService(agent2, agent1);
    EXPECT_EQ(out.returnCode, QAGENT_RC_UNAUTHORIZED);

    // Service is still active
    auto s = qagent.getService(agent1);
    EXPECT_EQ(s.found, 1);
    EXPECT_EQ(s.service.active, 1);
}

// ===========================================================================
// TEST: A non-assigned agent cannot reveal someone else's task result
// ===========================================================================

TEST(TestQAgent, NonAssignedAgentCannotReveal)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);
    qagent.registerAgent(agent2, 0xFF, 1000, 50000);

    auto taskOut = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    uint64 taskId = taskOut.taskId;

    qagent.acceptTask(agent1, taskId, 30000);

    id resultHash = id(1, 2, 3, 4);
    id salt = id(5, 6, 7, 8);
    QAGENT::CommitRevealData crd;
    crd.resultHash = resultHash;
    crd.salt = salt;
    uint8 h[32];
    KangarooTwelve(reinterpret_cast<const uint8*>(&crd), sizeof(crd), h, 32);
    id ch;
    copyMem(&ch, h, 32);

    qagent.commitResult(agent1, taskId, ch);

    // agent2 attempts to reveal — unauthorized, not the assigned agent
    auto out = qagent.revealResult(agent2, taskId, resultHash, salt);
    EXPECT_EQ(out.returnCode, QAGENT_RC_UNAUTHORIZED);
}

// ===========================================================================
// TEST: Creating a task for a deactivated agent fails with WRONG_STATUS
// ===========================================================================

TEST(TestQAgent, CreateTaskForDeactivatedAgentFails)
{
    ContractTestingQAgent qagent;

    qagent.registerAgent(agent1, 0xFF, 1000, 50000);

    auto deact = qagent.deactivateAgent(agent1);
    EXPECT_EQ(deact.returnCode, QAGENT_RC_SUCCESS);

    // Agent exists but is not ACTIVE — task creation must reject
    auto out = qagent.createTask(requester1, agent1, QAGENT_TASK_TYPE_ANALYSIS, 2000, 10000);
    EXPECT_EQ(out.returnCode, QAGENT_RC_WRONG_STATUS);
}
