// QAGENT.h - Decentralized AI Agent Infrastructure on Qubic (v2)
// World-class on-chain AI marketplace with tiered verification, game-theoretic
// economics, and computor oracle integration.
// All procedures use _WITH_LOCALS pattern. Non-custodial, integers/hashes only.

using namespace QPI;

// ---
// Constants
// ---

constexpr uint32 QAGENT_MAX_AGENTS = 8192;
constexpr uint32 QAGENT_MAX_TASKS = 32768;
constexpr uint32 QAGENT_MAX_SERVICES = 4096;
constexpr uint32 QAGENT_MAX_STAKES = 8192;
constexpr uint32 QAGENT_MAX_ARBITRATORS = 256;
constexpr uint32 QAGENT_MAX_ACTIVE_TASKS = 8192;
constexpr uint32 QAGENT_MAX_INTERACTIONS = 32768;
constexpr uint32 QAGENT_MAX_PROPOSALS = 64;

constexpr uint32 QAGENT_CHALLENGE_WINDOW_TICKS = 300;
constexpr uint32 QAGENT_DEFAULT_DEADLINE_TICKS = 3600;
constexpr uint32 QAGENT_DISPUTE_RESOLUTION_TICKS = 1200;
constexpr uint32 QAGENT_ORACLE_TIMEOUT_MS = 60000;

constexpr uint32 QAGENT_BPS_SCALE = 10000;
constexpr uint32 QAGENT_PLATFORM_FEE_BPS = 150;
constexpr uint32 QAGENT_KEEPER_FEE_BPS = 10;
constexpr sint64 QAGENT_MIN_KEEPER_FEE = 100LL;
constexpr sint64 QAGENT_MIN_ESCROW = 1000LL;
constexpr sint64 QAGENT_MIN_REGISTRATION_STAKE = 50000LL;

constexpr uint32 QAGENT_DISPUTE_VALIDATORS = 5;
constexpr uint32 QAGENT_DISPUTE_THRESHOLD = 3;
constexpr sint64 QAGENT_MIN_ARBITRATOR_STAKE = 10000LL;

constexpr uint32 QAGENT_STAKE_LOCK_EPOCHS = 4;
constexpr uint32 QAGENT_MAX_TASKS_PER_TICK = 50;

constexpr sint64 QAGENT_ORACLE_AUTO_THRESHOLD = 1000000LL;

constexpr uint32 QAGENT_GOVERNANCE_VOTE_TICKS = 50400;
constexpr uint32 QAGENT_MAX_GOVERNABLE_PARAMS = 10;

// Slash distribution (BPS of slashed amount)
constexpr uint32 QAGENT_SLASH_BURN_BPS = 4000;
constexpr uint32 QAGENT_SLASH_CHALLENGER_BPS = 3000;
constexpr uint32 QAGENT_SLASH_ARBITRATOR_BPS = 2000;
constexpr uint32 QAGENT_SLASH_TREASURY_BPS = 1000;

// Fee distribution (BPS of platform fee)
constexpr uint32 QAGENT_FEE_BURN_BPS = 5000;
constexpr uint32 QAGENT_FEE_TREASURY_BPS = 3000;
constexpr uint32 QAGENT_FEE_ARBITRATOR_BPS = 2000;

// ---
// Enums
// ---

constexpr uint8 QAGENT_STATUS_INACTIVE = 0;
constexpr uint8 QAGENT_STATUS_ACTIVE = 1;
constexpr uint8 QAGENT_STATUS_SUSPENDED = 2;

constexpr uint8 QAGENT_TASK_STATUS_OPEN = 0;
constexpr uint8 QAGENT_TASK_STATUS_ACCEPTED = 1;
constexpr uint8 QAGENT_TASK_STATUS_COMMITTED = 2;
constexpr uint8 QAGENT_TASK_STATUS_REVEALED = 3;
constexpr uint8 QAGENT_TASK_STATUS_COMPLETED = 4;
constexpr uint8 QAGENT_TASK_STATUS_DISPUTED = 5;
constexpr uint8 QAGENT_TASK_STATUS_CANCELLED = 6;
constexpr uint8 QAGENT_TASK_STATUS_TIMED_OUT = 7;
constexpr uint8 QAGENT_TASK_STATUS_ORACLE_PENDING = 8;

constexpr uint8 QAGENT_TASK_TYPE_ANALYSIS = 0;
constexpr uint8 QAGENT_TASK_TYPE_TRADE = 1;
constexpr uint8 QAGENT_TASK_TYPE_PREDICT = 2;
constexpr uint8 QAGENT_TASK_TYPE_SUMMARIZE = 3;
constexpr uint8 QAGENT_TASK_TYPE_CLASSIFY = 4;
constexpr uint8 QAGENT_TASK_TYPE_GENERATE = 5;
constexpr uint8 QAGENT_TASK_TYPE_CUSTOM = 6;
constexpr uint8 QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE = 7;

constexpr uint8 QAGENT_VERDICT_NONE = 0;
constexpr uint8 QAGENT_VERDICT_VALID = 1;
constexpr uint8 QAGENT_VERDICT_INVALID = 2;

constexpr uint8 QAGENT_TIER_UNRANKED = 0;
constexpr uint8 QAGENT_TIER_BRONZE = 1;
constexpr uint8 QAGENT_TIER_SILVER = 2;
constexpr uint8 QAGENT_TIER_GOLD = 3;
constexpr uint8 QAGENT_TIER_DIAMOND = 4;

// Governable parameter IDs
constexpr uint8 QAGENT_PARAM_PLATFORM_FEE_BPS = 0;
constexpr uint8 QAGENT_PARAM_MIN_REG_STAKE = 1;
constexpr uint8 QAGENT_PARAM_CHALLENGE_WINDOW = 2;
constexpr uint8 QAGENT_PARAM_MIN_ESCROW = 3;
constexpr uint8 QAGENT_PARAM_ORACLE_THRESHOLD = 4;
constexpr uint8 QAGENT_PARAM_KEEPER_FEE_BPS = 5;
constexpr uint8 QAGENT_PARAM_DISPUTE_TICKS = 6;
constexpr uint8 QAGENT_PARAM_DEFAULT_DEADLINE = 7;
constexpr uint8 QAGENT_PARAM_PAUSED = 8;
constexpr uint8 QAGENT_PARAM_TREASURY_BURN = 9;

// ---
// Return Codes
// ---

constexpr uint8 QAGENT_RC_SUCCESS = 0;
constexpr uint8 QAGENT_RC_INVALID_INPUT = 1;
constexpr uint8 QAGENT_RC_INSUFFICIENT_FUNDS = 2;
constexpr uint8 QAGENT_RC_AGENT_NOT_FOUND = 3;
constexpr uint8 QAGENT_RC_TASK_NOT_FOUND = 4;
constexpr uint8 QAGENT_RC_WRONG_STATUS = 5;
constexpr uint8 QAGENT_RC_UNAUTHORIZED = 6;
constexpr uint8 QAGENT_RC_CAPACITY_FULL = 7;
constexpr uint8 QAGENT_RC_ALREADY_EXISTS = 8;
constexpr uint8 QAGENT_RC_CHALLENGE_WINDOW_OPEN = 9;
constexpr uint8 QAGENT_RC_CHALLENGE_WINDOW_CLOSED = 10;
constexpr uint8 QAGENT_RC_DEADLINE_NOT_REACHED = 11;
constexpr uint8 QAGENT_RC_SELF_DEALING = 12;
constexpr uint8 QAGENT_RC_COMMIT_MISMATCH = 13;
constexpr uint8 QAGENT_RC_STAKE_LOCKED = 14;
constexpr uint8 QAGENT_RC_SERVICE_NOT_FOUND = 15;
constexpr uint8 QAGENT_RC_ORACLE_PENDING = 16;
constexpr uint8 QAGENT_RC_PROPOSAL_NOT_FOUND = 17;
constexpr uint8 QAGENT_RC_DELEGATION_DEPTH_EXCEEDED = 18;
constexpr uint8 QAGENT_RC_DELEGATION_UNAUTHORIZED = 19;
constexpr uint8 QAGENT_RC_DEADLINE_PASSED = 20;
constexpr uint8 QAGENT_RC_PAUSED = 21;

constexpr uint32 QAGENT_MAX_DELEGATION_DEPTH = 3;

// ---
// Logging
// ---

enum QAGENTLogInfo {
    logRegisterAgent = 1,
    logDeactivateAgent = 2,
    logReactivateAgent = 3,
    logCreateTask = 4,
    logAcceptTask = 5,
    logCommitResult = 6,
    logRevealResult = 7,
    logApproveResult = 8,
    logChallengeResult = 9,
    logFinalizeTask = 10,
    logTimeoutTask = 11,
    logCancelTask = 12,
    logCompleteMilestone = 13,
    logRequestOracle = 14,
    logOracleReply = 15,
    logCastDisputeVote = 16,
    logWithdrawStake = 17,
    logProposeChange = 18,
    logGovernanceExecute = 19,
    logEndTickAutoFinalize = 20,
    logEndTickAutoTimeout = 21,
    logDelegateTask = 22,
    logStakeMore = 23,
    logUpdateAgent = 24,
    logRegisterService = 25,
    logDeprecateService = 26,
    logUpdateServicePrice = 27,
    logRegisterArbitrator = 28,
    logVoteOnProposal = 29,
    logRateAgent = 30,
    logDeactivateArbitrator = 31,
    logWithdrawArbitratorStake = 32,
    logTreasuryBurn = 33,
    logArbPoolDistribute = 34,
};

struct QAGENTLogger
{
    uint32 _contractIndex;
    uint32 _type;
    uint64 taskId;
    id entityId;
    sint64 amount;
    uint8 extra;
    sint8 _terminator;
};

// ---
// Forward declaration
// ---

struct QAGENT2
{
};

// ---
// Main contract
// ---

struct QAGENT : public ContractBase
{
public:

// ---
// Data Structures
// ---

struct AgentRecord
{
    id agentId;
    id ownerWallet;
    uint8 status;
    uint8 tier;
    uint32 capabilityBitmap;
    sint64 pricePerTask;
    sint64 totalEarned;
    uint32 completedTasks;
    uint32 failedTasks;
    uint32 totalTasks;
    uint32 oracleVerifiedTasks;
    uint32 oraclePassedTasks;
    uint16 uniqueRequesters;
    uint8 slashCount;
    uint32 registeredTick;
    uint32 registeredEpoch;
};

struct TaskRecord
{
    uint64 taskId;
    id requester;
    id assignedAgent;
    uint8 taskType;
    uint8 status;
    sint64 escrowAmount;
    sint64 agentStake;
    sint64 challengerStake;
    id challengerId;
    id commitHash;
    id resultHash;
    id agentSalt;
    id specHash;
    id modelHash;
    id dataCid;
    uint8 resultEnum;
    uint8 totalMilestones;
    uint8 completedMilestones;
    uint32 createdTick;
    uint32 deadlineTick;
    uint32 submitTick;
    uint32 challengeDeadlineTick;
    uint32 disputeDeadlineTick;
    uint32 finalizedTick;
    uint8 disputeVotesValid;
    uint8 disputeVotesInvalid;
    uint8 verdict;
    id disputeVoters_0;
    id disputeVoters_1;
    id disputeVoters_2;
    id disputeVoters_3;
    id disputeVoters_4;
    uint8 disputeVoterCount;
    uint8 rated;
    sint64 oracleQueryId;
    uint8 oracleRequested;
    uint8 inferenceConfig;
    uint64 parentTaskId;
};

struct StakeRecord
{
    sint64 registrationStake;
    sint64 additionalStake;
    uint32 registeredEpoch;
    uint32 unstakeRequestEpoch;
    uint8 locked;
};

struct ArbitratorRecord
{
    sint64 stakeAmount;
    uint32 casesHandled;
    uint32 majorityVotes;
    uint32 registeredTick;
    uint8 active;
};

struct ServiceDefinition
{
    id ownerId;
    uint32 capabilityBitmap;
    sint64 basePrice;
    uint8 taskType;
    uint8 active;
    uint32 registeredTick;
};

struct GovernanceProposal
{
    id proposer;
    uint8 paramId;
    sint64 newValue;
    uint32 startTick;
    uint32 endTick;
    sint64 yesStakeWeight;
    sint64 noStakeWeight;
    uint8 executed;
    uint8 active;
    uint32 proposalId;
};

struct ProposalVoteKey
{
    id voterId;
    uint32 proposalId;
};

struct PlatformConfig
{
    sint64 platformFeeBPS;
    sint64 minRegistrationStake;
    sint64 challengeWindowTicks;
    sint64 minEscrow;
    sint64 oracleAutoThreshold;
    sint64 keeperFeeBPS;
    sint64 disputeResolutionTicks;
    sint64 defaultDeadlineTicks;
    sint64 paused;
};

struct PlatformStats
{
    uint32 totalAgents;
    uint32 totalTasks;
    sint64 totalVolume;
    sint64 totalBurned;
    sint64 treasuryBalance;
    sint64 arbitratorPool;
    uint32 activeTasks;
    uint32 disputedTasks;
    uint32 oracleVerifications;
    uint32 totalProposals;
};

// Helper struct for K12 commit-reveal hashing
struct CommitRevealData
{
    id resultHash;
    id salt;
};

// Helper struct for interaction tracking key
struct InteractionKey
{
    id agentId;
    id requesterId;
};

// ---
// State Declaration
// ---

struct StateData
{
    HashMap<id, AgentRecord, QAGENT_MAX_AGENTS> agents;
    HashMap<uint64, TaskRecord, QAGENT_MAX_TASKS> tasks;
    HashMap<id, ServiceDefinition, QAGENT_MAX_SERVICES> services;
    HashMap<id, StakeRecord, QAGENT_MAX_STAKES> stakes;
    HashMap<id, ArbitratorRecord, QAGENT_MAX_ARBITRATORS> arbitrators;
    HashMap<id, uint32, QAGENT_MAX_INTERACTIONS> interactions;
    Collection<uint64, QAGENT_MAX_ACTIVE_TASKS> activeTaskQ;
    HashMap<sint64, uint64, 8192> oracleQueryToTask;
    HashMap<id, uint8, 8192> proposalVoters;
    Array<GovernanceProposal, QAGENT_MAX_PROPOSALS> proposals;
    uint32 proposalCounter;
    PlatformConfig config;
    PlatformStats stats;
    uint64 taskCounter;
};

// ---
// Input / Output / Locals for all procedures and functions
// ---

// --- Agent Lifecycle ---

struct RegisterAgent_input
{
    uint32 capabilityBitmap;
    sint64 pricePerTask;
};
struct RegisterAgent_output
{
    uint8 returnCode;
};
struct RegisterAgent_locals
{
    AgentRecord agent;
    StakeRecord stake;
    sint64 excess;
    QAGENTLogger log;
};

struct UpdateAgent_input
{
    uint32 capabilityBitmap;
    sint64 pricePerTask;
};
struct UpdateAgent_output
{
    uint8 returnCode;
};
struct UpdateAgent_locals
{
    AgentRecord agent;
    QAGENTLogger log;
};

struct DeactivateAgent_input {};
struct DeactivateAgent_output
{
    uint8 returnCode;
};
struct DeactivateAgent_locals
{
    AgentRecord agent;
    QAGENTLogger log;
};

struct ReactivateAgent_input {};
struct ReactivateAgent_output
{
    uint8 returnCode;
};
struct ReactivateAgent_locals
{
    AgentRecord agent;
    QAGENTLogger log;
};

struct StakeMore_input {};
struct StakeMore_output
{
    uint8 returnCode;
    sint64 totalStake;
};
struct StakeMore_locals
{
    StakeRecord stake;
    QAGENTLogger log;
};

struct RequestUnstake_input {};
struct RequestUnstake_output
{
    uint8 returnCode;
};
struct RequestUnstake_locals
{
    StakeRecord stake;
};

struct WithdrawStake_input {};
struct WithdrawStake_output
{
    uint8 returnCode;
    sint64 withdrawn;
};
struct WithdrawStake_locals
{
    StakeRecord stake;
    AgentRecord agent;
    sint64 amount;
    QAGENTLogger log;
};

// --- Service Registry ---

struct RegisterService_input
{
    uint32 capabilityBitmap;
    sint64 basePrice;
    uint8 taskType;
};
struct RegisterService_output
{
    uint8 returnCode;
};
struct RegisterService_locals
{
    ServiceDefinition svc;
    QAGENTLogger log;
};

struct DeprecateService_input
{
    id serviceId;
};
struct DeprecateService_output
{
    uint8 returnCode;
};
struct DeprecateService_locals
{
    ServiceDefinition svc;
    QAGENTLogger log;
};

struct UpdateServicePrice_input
{
    id serviceId;
    sint64 newPrice;
};
struct UpdateServicePrice_output
{
    uint8 returnCode;
};
struct UpdateServicePrice_locals
{
    ServiceDefinition svc;
    QAGENTLogger log;
};

// --- Task Lifecycle ---

struct CreateTask_input
{
    id agentId;
    uint8 taskType;
    uint32 deadlineTicks;
    id specHash;
    id modelHash;
    uint8 totalMilestones;
};
struct CreateTask_output
{
    uint8 returnCode;
    uint64 taskId;
};
struct CreateTask_locals
{
    AgentRecord agent;
    TaskRecord task;
    InteractionKey ikey;
    id ikeyHash;
    uint32 icount;
    QAGENTLogger log;
};

struct AcceptTask_input
{
    uint64 taskId;
};
struct AcceptTask_output
{
    uint8 returnCode;
};
struct AcceptTask_locals
{
    TaskRecord task;
    AgentRecord agent;
    StakeRecord stake;
    sint64 requiredStake;
    sint64 excess;
    uint32 bondBPS;
    QAGENTLogger log;
};

struct CommitResult_input
{
    uint64 taskId;
    id commitHash;
};
struct CommitResult_output
{
    uint8 returnCode;
};
struct CommitResult_locals
{
    TaskRecord task;
    QAGENTLogger log;
};

struct RevealResult_input
{
    uint64 taskId;
    id resultHash;
    id salt;
    id dataCid;
    uint8 resultEnum;
};
struct RevealResult_output
{
    uint8 returnCode;
};
struct RevealResult_locals
{
    TaskRecord task;
    CommitRevealData crData;
    id computedHash;
    QAGENTLogger log;
};

struct ApproveResult_input
{
    uint64 taskId;
};
struct ApproveResult_output
{
    uint8 returnCode;
};
struct ApproveResult_locals
{
    TaskRecord task;
    AgentRecord agent;
    sint64 platformFee;
    sint64 burnAmount;
    sint64 treasuryAmount;
    sint64 arbPoolAmount;
    sint64 agentPayout;
    sint64 completionRate;
    QAGENTLogger log;
};

struct ChallengeResult_input
{
    uint64 taskId;
    id challengerResultHash;
};
struct ChallengeResult_output
{
    uint8 returnCode;
};
struct ChallengeResult_locals
{
    TaskRecord task;
    sint64 excess;
    QAGENTLogger log;
};

struct FinalizeTask_input
{
    uint64 taskId;
};
struct FinalizeTask_output
{
    uint8 returnCode;
    sint64 keeperFee;
};
struct FinalizeTask_locals
{
    TaskRecord task;
    AgentRecord agent;
    sint64 platformFee;
    sint64 keeperFee;
    sint64 agentPayout;
    sint64 burnAmount;
    sint64 treasuryAmount;
    sint64 arbPoolAmount;
    sint64 completionRate;
    QAGENTLogger log;
};

struct TimeoutTask_input
{
    uint64 taskId;
};
struct TimeoutTask_output
{
    uint8 returnCode;
    sint64 keeperFee;
};
struct TimeoutTask_locals
{
    TaskRecord task;
    TaskRecord parentTask;
    AgentRecord agent;
    StakeRecord stake;
    sint64 keeperFee;
    sint64 slashAmount;
    sint64 burnAmount;
    sint64 treasuryAmount;
    sint64 completionRate;
    QAGENTLogger log;
};

struct CancelTask_input
{
    uint64 taskId;
};
struct CancelTask_output
{
    uint8 returnCode;
};
struct CancelTask_locals
{
    TaskRecord task;
    QAGENTLogger log;
};

struct CompleteMilestone_input
{
    uint64 taskId;
};
struct CompleteMilestone_output
{
    uint8 returnCode;
    sint64 milestonePayout;
};
struct CompleteMilestone_locals
{
    TaskRecord task;
    AgentRecord agent;
    sint64 payout;
    sint64 platformFee;
    sint64 burnAmount;
    sint64 treasuryAmount;
    sint64 completionRate;
    QAGENTLogger log;
};

// --- Delegation ---

struct DelegateTask_input
{
    uint64 parentTaskId;
    id agentId;
    uint8 taskType;
    uint32 deadlineTicks;
    id specHash;
    id modelHash;
    sint64 delegatedEscrow;
};
struct DelegateTask_output
{
    uint8 returnCode;
    uint64 childTaskId;
};
struct DelegateTask_locals
{
    TaskRecord parentTask;
    TaskRecord childTask;
    AgentRecord parentAgent;
    AgentRecord childAgent;
    uint32 depth;
    uint64 currentTaskId;
    TaskRecord tempTask;
    QAGENTLogger log;
};

// --- Dispute Resolution ---

struct RequestOracleVerification_input
{
    uint64 taskId;
};
struct RequestOracleVerification_output
{
    uint8 returnCode;
    sint64 oracleQueryId;
};
struct RequestOracleVerification_locals
{
    TaskRecord task;
    OI::AIVerify::OracleQuery oracleQuery;
    QAGENTLogger log;
};

typedef OracleNotificationInput<OI::AIVerify> OnAIVerifyReply_input;

typedef NoData OnAIVerifyReply_output;
struct OnAIVerifyReply_locals
{
    TaskRecord task;
    AgentRecord agent;
    sint64 platformFee;
    sint64 agentPayout;
    sint64 burnAmount;
    sint64 slashAmount;
    sint64 completionRate;
    uint64 foundTaskId;
    QAGENTLogger log;
};

struct CastDisputeVote_input
{
    uint64 taskId;
    uint8 vote;
};
struct CastDisputeVote_output
{
    uint8 returnCode;
};
struct CastDisputeVote_locals
{
    TaskRecord task;
    AgentRecord agent;
    ArbitratorRecord arb;
    sint64 winnerPayout;
    sint64 burnAmount;
    sint64 platformFee;
    sint64 challengerReward;
    sint64 arbReward;
    sint64 perVoterReward;
    sint64 treasuryAmount;
    sint64 slashAmount;
    sint64 completionRate;
    uint8 alreadyVoted;
    uint8 resolved;
    QAGENTLogger log;
};

struct RegisterArbitrator_input {};
struct RegisterArbitrator_output
{
    uint8 returnCode;
};
struct RegisterArbitrator_locals
{
    ArbitratorRecord arb;
    sint64 excess;
    QAGENTLogger log;
};

struct DeactivateArbitrator_input {};
struct DeactivateArbitrator_output
{
    uint8 returnCode;
};
struct DeactivateArbitrator_locals
{
    ArbitratorRecord arb;
    QAGENTLogger log;
};

struct WithdrawArbitratorStake_input {};
struct WithdrawArbitratorStake_output
{
    uint8 returnCode;
    sint64 withdrawn;
};
struct WithdrawArbitratorStake_locals
{
    ArbitratorRecord arb;
    sint64 amount;
    QAGENTLogger log;
};

// --- Governance ---

struct ProposeParameterChange_input
{
    uint8 paramId;
    sint64 newValue;
};
struct ProposeParameterChange_output
{
    uint8 returnCode;
    uint8 proposalIndex;
};
struct ProposeParameterChange_locals
{
    StakeRecord stake;
    GovernanceProposal prop;
    uint32 i;
    uint8 freeSlot;
    QAGENTLogger log;
};

struct VoteOnProposal_input
{
    uint8 proposalIndex;
    uint8 vote; // 1=yes, 2=no
};
struct VoteOnProposal_output
{
    uint8 returnCode;
};
struct VoteOnProposal_locals
{
    GovernanceProposal prop;
    StakeRecord stake;
    sint64 weight;
    ProposalVoteKey voteKey;
    id voteKeyHash;
    uint8 alreadyVoted;
    QAGENTLogger log;
};

// --- Rating ---

struct RateAgent_input
{
    uint64 taskId;
    uint8 rating;
};
struct RateAgent_output
{
    uint8 returnCode;
};
struct RateAgent_locals
{
    TaskRecord task;
    QAGENTLogger log;
};

// --- Query Functions ---

struct GetAgent_input
{
    id agentId;
};
struct GetAgent_output
{
    AgentRecord agent;
    uint8 found;
};

struct GetTask_input
{
    uint64 taskId;
};
struct GetTask_output
{
    TaskRecord task;
    uint8 found;
};

struct GetService_input
{
    id serviceId;
};
struct GetService_output
{
    ServiceDefinition service;
    uint8 found;
};

struct GetPlatformStats_input {};
struct GetPlatformStats_output
{
    PlatformStats stats;
    PlatformConfig config;
};

struct GetStake_input
{
    id agentId;
};
struct GetStake_output
{
    StakeRecord stake;
    uint8 found;
};

struct GetArbitrator_input
{
    id arbitratorId;
};
struct GetArbitrator_output
{
    ArbitratorRecord arbitrator;
    uint8 found;
};

struct GetAgentsByCapability_input
{
    uint32 capabilityBitmap;
};
struct GetAgentsByCapability_output
{
    uint32 count;
};
struct GetAgentsByCapability_locals
{
    sint64 elemIdx;
    AgentRecord agent;
};

struct GetProposal_input
{
    uint8 proposalIndex;
};
struct GetProposal_output
{
    GovernanceProposal proposal;
    uint8 found;
};

struct GetAgentReputationScore_input
{
    id agentId;
};
struct GetAgentReputationScore_output
{
    uint32 score; // 0-10000 BPS normalized
    uint8 found;
};
struct GetAgentReputationScore_locals
{
    AgentRecord agent;
    sint64 completionScore;
    sint64 oracleScore;
    sint64 requesterScore;
    sint64 tenureScore;
};

// --- System procedure locals ---

struct END_TICK_locals
{
    sint64 elemIdx;
    uint64 taskId;
    TaskRecord task;
    TaskRecord parentTask;
    AgentRecord agent;
    StakeRecord stake;
    sint64 platformFee;
    sint64 agentPayout;
    sint64 burnAmount;
    sint64 treasuryAmount;
    sint64 arbPoolAmount;
    sint64 slashAmount;
    sint64 completionRate;
    uint32 processed;
    QAGENTLogger log;
};

struct END_EPOCH_locals
{
    uint32 i;
    GovernanceProposal prop;
    uint8 hasActiveProposals;
    sint64 voterIdx;
    sint64 nextVoterIdx;
    uint32 arbCount;
    sint64 arbShare;
    sint64 arbIdx;
    ArbitratorRecord tempArb;
    QAGENTLogger log;
};

// ---
// Registration
// ---

REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
{
    // Agent Lifecycle
    REGISTER_USER_PROCEDURE(RegisterAgent, 1);
    REGISTER_USER_PROCEDURE(UpdateAgent, 2);
    REGISTER_USER_PROCEDURE(DeactivateAgent, 3);
    REGISTER_USER_PROCEDURE(ReactivateAgent, 4);
    REGISTER_USER_PROCEDURE(StakeMore, 5);
    REGISTER_USER_PROCEDURE(RequestUnstake, 6);
    REGISTER_USER_PROCEDURE(WithdrawStake, 7);

    // Service Registry
    REGISTER_USER_PROCEDURE(RegisterService, 8);
    REGISTER_USER_PROCEDURE(DeprecateService, 9);
    REGISTER_USER_PROCEDURE(UpdateServicePrice, 10);

    // Task Lifecycle
    REGISTER_USER_PROCEDURE(CreateTask, 11);
    REGISTER_USER_PROCEDURE(AcceptTask, 12);
    REGISTER_USER_PROCEDURE(CommitResult, 13);
    REGISTER_USER_PROCEDURE(RevealResult, 14);
    REGISTER_USER_PROCEDURE(ApproveResult, 15);
    REGISTER_USER_PROCEDURE(ChallengeResult, 16);
    REGISTER_USER_PROCEDURE(FinalizeTask, 17);
    REGISTER_USER_PROCEDURE(TimeoutTask, 18);
    REGISTER_USER_PROCEDURE(CancelTask, 19);
    REGISTER_USER_PROCEDURE(CompleteMilestone, 20);

    // Dispute Resolution
    REGISTER_USER_PROCEDURE(RequestOracleVerification, 21);
    REGISTER_USER_PROCEDURE(CastDisputeVote, 22);
    REGISTER_USER_PROCEDURE(RegisterArbitrator, 23);
    REGISTER_USER_PROCEDURE(DeactivateArbitrator, 28);
    REGISTER_USER_PROCEDURE(WithdrawArbitratorStake, 29);

    // Governance
    REGISTER_USER_PROCEDURE(ProposeParameterChange, 24);
    REGISTER_USER_PROCEDURE(VoteOnProposal, 25);

    // Rating
    REGISTER_USER_PROCEDURE(RateAgent, 26);

    // Delegation
    REGISTER_USER_PROCEDURE(DelegateTask, 27);

    // Oracle notification
    REGISTER_USER_PROCEDURE_NOTIFICATION(OnAIVerifyReply);

    // Query Functions
    REGISTER_USER_FUNCTION(GetAgent, 1);
    REGISTER_USER_FUNCTION(GetTask, 2);
    REGISTER_USER_FUNCTION(GetService, 3);
    REGISTER_USER_FUNCTION(GetPlatformStats, 4);
    REGISTER_USER_FUNCTION(GetStake, 5);
    REGISTER_USER_FUNCTION(GetArbitrator, 6);
    REGISTER_USER_FUNCTION(GetAgentsByCapability, 7);
    REGISTER_USER_FUNCTION(GetProposal, 8);
    REGISTER_USER_FUNCTION(GetAgentReputationScore, 9);
}

// ---
// System Procedures
// ---

INITIALIZE()
{
    // State is zero-initialized by default; only set non-zero config values
    state.mut().config.platformFeeBPS = QAGENT_PLATFORM_FEE_BPS;
    state.mut().config.minRegistrationStake = QAGENT_MIN_REGISTRATION_STAKE;
    state.mut().config.challengeWindowTicks = QAGENT_CHALLENGE_WINDOW_TICKS;
    state.mut().config.minEscrow = QAGENT_MIN_ESCROW;
    state.mut().config.oracleAutoThreshold = QAGENT_ORACLE_AUTO_THRESHOLD;
    state.mut().config.keeperFeeBPS = QAGENT_KEEPER_FEE_BPS;
    state.mut().config.disputeResolutionTicks = QAGENT_DISPUTE_RESOLUTION_TICKS;
    state.mut().config.defaultDeadlineTicks = QAGENT_DEFAULT_DEADLINE_TICKS;
}

END_EPOCH_WITH_LOCALS()
{
    // Execute passed governance proposals
    for (locals.i = 0; locals.i < QAGENT_MAX_PROPOSALS; locals.i++)
    {
        locals.prop = state.get().proposals.get(locals.i);
        if (locals.prop.active != 0 && locals.prop.executed == 0
            && locals.prop.endTick > 0 && locals.prop.endTick <= qpi.tick())
        {
            // Check if proposal passed (yes > no)
            if (locals.prop.yesStakeWeight > locals.prop.noStakeWeight)
            {
                if (locals.prop.paramId == QAGENT_PARAM_PLATFORM_FEE_BPS && locals.prop.newValue >= 0 && locals.prop.newValue <= 500)
                    state.mut().config.platformFeeBPS = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_MIN_REG_STAKE && locals.prop.newValue >= 10000)
                    state.mut().config.minRegistrationStake = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_CHALLENGE_WINDOW && locals.prop.newValue >= 100 && locals.prop.newValue <= 3000)
                    state.mut().config.challengeWindowTicks = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_MIN_ESCROW && locals.prop.newValue >= 100)
                    state.mut().config.minEscrow = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_ORACLE_THRESHOLD && locals.prop.newValue >= 100000)
                    state.mut().config.oracleAutoThreshold = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_KEEPER_FEE_BPS && locals.prop.newValue >= 0 && locals.prop.newValue <= 100)
                    state.mut().config.keeperFeeBPS = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_DISPUTE_TICKS && locals.prop.newValue >= 300 && locals.prop.newValue <= 6000)
                    state.mut().config.disputeResolutionTicks = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_DEFAULT_DEADLINE && locals.prop.newValue >= 600 && locals.prop.newValue <= 36000)
                    state.mut().config.defaultDeadlineTicks = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_PAUSED && (locals.prop.newValue == 0 || locals.prop.newValue == 1))
                    state.mut().config.paused = locals.prop.newValue;
                else if (locals.prop.paramId == QAGENT_PARAM_TREASURY_BURN
                    && locals.prop.newValue > 0
                    && locals.prop.newValue <= state.get().stats.treasuryBalance)
                {
                    qpi.burn(locals.prop.newValue);
                    state.mut().stats.treasuryBalance -= locals.prop.newValue;
                    state.mut().stats.totalBurned += locals.prop.newValue;
                    locals.log = QAGENTLogger{ CONTRACT_INDEX, logTreasuryBurn, 0, locals.prop.proposer, locals.prop.newValue, QAGENT_PARAM_TREASURY_BURN };
                    LOG_INFO(locals.log);
                }
            }
            locals.prop.executed = 1;
            locals.prop.active = 0;
            state.mut().proposals.set(locals.i, locals.prop);

            locals.log = QAGENTLogger{ CONTRACT_INDEX, logGovernanceExecute, 0, locals.prop.proposer, locals.prop.newValue, locals.prop.paramId };
            LOG_INFO(locals.log);
        }
    }

    // Purge stale proposalVoters entries when all proposals are inactive.
    // ProposalVoters keys are K12 hashes of (voterId, proposalId) — cannot be
    // selectively removed per-proposal without tracking voters. Safe to clear
    // the entire HashMap once no active proposals remain.
    locals.hasActiveProposals = 0;
    for (locals.i = 0; locals.i < QAGENT_MAX_PROPOSALS; locals.i++)
    {
        locals.prop = state.get().proposals.get(locals.i);
        if (locals.prop.active != 0)
        {
            locals.hasActiveProposals = 1;
        }
    }
    if (locals.hasActiveProposals == 0 && state.get().proposalVoters.population() > 0)
    {
        locals.voterIdx = state.get().proposalVoters.nextElementIndex(-1);
        while (locals.voterIdx != NULL_INDEX)
        {
            locals.nextVoterIdx = state.get().proposalVoters.nextElementIndex(locals.voterIdx);
            state.mut().proposalVoters.removeByIndex(locals.voterIdx);
            locals.voterIdx = locals.nextVoterIdx;
        }
        state.mut().proposalVoters.cleanupIfNeeded(0);
    }

    // Distribute accumulated arbitrator pool rewards to active arbitrators (equal split)
    if (state.get().stats.arbitratorPool > 0)
    {
        locals.arbCount = 0;
        locals.arbIdx = state.get().arbitrators.nextElementIndex(NULL_INDEX);
        while (locals.arbIdx != NULL_INDEX)
        {
            locals.tempArb = state.get().arbitrators.value(locals.arbIdx);
            if (locals.tempArb.active != 0)
                locals.arbCount += 1;
            locals.arbIdx = state.get().arbitrators.nextElementIndex(locals.arbIdx);
        }
        if (locals.arbCount > 0)
        {
            locals.arbShare = div<sint64>(state.get().stats.arbitratorPool, static_cast<sint64>(locals.arbCount));
            if (locals.arbShare > 0)
            {
                locals.arbIdx = state.get().arbitrators.nextElementIndex(NULL_INDEX);
                while (locals.arbIdx != NULL_INDEX)
                {
                    locals.tempArb = state.get().arbitrators.value(locals.arbIdx);
                    if (locals.tempArb.active != 0)
                        qpi.transfer(state.get().arbitrators.key(locals.arbIdx), locals.arbShare);
                    locals.arbIdx = state.get().arbitrators.nextElementIndex(locals.arbIdx);
                }
                // Keep remainder in pool (avoid losing rounding dust)
                state.mut().stats.arbitratorPool -= smul(locals.arbShare, static_cast<sint64>(locals.arbCount));
            }
        }
    }
}

END_TICK_WITH_LOCALS()
{
    // Collection-based processing: only process tasks with expired deadlines
    // headIndex(pov, maxPriority) returns first element with priority <= tick
    locals.processed = 0;
    locals.elemIdx = state.get().activeTaskQ.headIndex(SELF, static_cast<sint64>(qpi.tick()));

    while (locals.elemIdx != NULL_INDEX && locals.processed < QAGENT_MAX_TASKS_PER_TICK)
    {
        locals.taskId = state.get().activeTaskQ.element(locals.elemIdx);

        if (!state.get().tasks.get(locals.taskId, locals.task))
        {
            // Stale entry — task was removed
            locals.elemIdx = state.mut().activeTaskQ.remove(locals.elemIdx);
            locals.processed += 1;
            continue;
        }

        // Already finalized — clean up queue entry
        if (locals.task.finalizedTick != 0)
        {
            locals.elemIdx = state.mut().activeTaskQ.remove(locals.elemIdx);
            locals.processed += 1;
            continue;
        }

        // ACCEPTED or COMMITTED task past deadline → timeout (agent failed to deliver/reveal)
        if ((locals.task.status == QAGENT_TASK_STATUS_ACCEPTED
            || locals.task.status == QAGENT_TASK_STATUS_COMMITTED)
            && locals.task.deadlineTick > 0
            && qpi.tick() > locals.task.deadlineTick)
        {
            // Return escrow: delegated tasks restore to parent, root tasks refund requester
            if (locals.task.escrowAmount > 0)
            {
                if (locals.task.parentTaskId != 0 && state.get().tasks.get(locals.task.parentTaskId, locals.parentTask))
                {
                    locals.parentTask.escrowAmount += locals.task.escrowAmount;
                    state.mut().tasks.set(locals.task.parentTaskId, locals.parentTask);
                }
                else
                {
                    qpi.transfer(locals.task.requester, locals.task.escrowAmount);
                }
            }
            if (locals.task.agentStake > 0)
            {
                qpi.burn(locals.task.agentStake);
                state.mut().stats.totalBurned += locals.task.agentStake;
            }
            locals.task.status = QAGENT_TASK_STATUS_TIMED_OUT;
            locals.task.finalizedTick = qpi.tick();
            state.mut().tasks.set(locals.task.taskId, locals.task);

            // Update agent slash count and recalculate tier
            if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
            {
                locals.agent.failedTasks += 1;
                locals.agent.slashCount += 1;

                // Progressive: at 4+ offenses also slash 25% registration stake + suspend
                if (locals.agent.slashCount >= 4)
                {
                    locals.agent.status = QAGENT_STATUS_SUSPENDED;
                    if (state.get().stakes.get(locals.task.assignedAgent, locals.stake))
                    {
                        locals.slashAmount = div<sint64>(locals.stake.registrationStake, static_cast<sint64>(4));
                        if (locals.slashAmount > locals.stake.registrationStake)
                            locals.slashAmount = locals.stake.registrationStake;
                        locals.stake.registrationStake -= locals.slashAmount;
                        state.mut().stakes.set(locals.task.assignedAgent, locals.stake);
                        if (locals.slashAmount > 0)
                        {
                            qpi.burn(locals.slashAmount);
                            state.mut().stats.totalBurned += locals.slashAmount;
                        }
                    }
                }

                locals.agent.tier = QAGENT_TIER_UNRANKED;
                if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
                {
                    locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                        static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                    if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                        locals.agent.tier = QAGENT_TIER_DIAMOND;
                    else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                        locals.agent.tier = QAGENT_TIER_GOLD;
                    else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                        locals.agent.tier = QAGENT_TIER_SILVER;
                    else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                        locals.agent.tier = QAGENT_TIER_BRONZE;
                }
                state.mut().agents.set(locals.task.assignedAgent, locals.agent);
            }
            if (state.get().stats.activeTasks > 0)
                state.mut().stats.activeTasks -= 1;

            locals.log = QAGENTLogger{ CONTRACT_INDEX, logEndTickAutoTimeout, locals.task.taskId, locals.task.assignedAgent, locals.task.agentStake, 0 };
            LOG_INFO(locals.log);

            locals.elemIdx = state.mut().activeTaskQ.remove(locals.elemIdx);
            locals.processed += 1;
            continue;
        }

        // REVEALED task past challenge deadline → auto-finalize (no challenge)
        if (locals.task.status == QAGENT_TASK_STATUS_REVEALED
            && locals.task.challengeDeadlineTick > 0
            && qpi.tick() > locals.task.challengeDeadlineTick
            && locals.task.verdict == QAGENT_VERDICT_NONE)
        {
            locals.platformFee = div<sint64>(
                smul(locals.task.escrowAmount, state.get().config.platformFeeBPS),
                static_cast<sint64>(QAGENT_BPS_SCALE));
            locals.agentPayout = locals.task.escrowAmount - locals.platformFee;

            if (locals.agentPayout > 0)
                qpi.transfer(locals.task.assignedAgent, locals.agentPayout);
            if (locals.task.agentStake > 0)
                qpi.transfer(locals.task.assignedAgent, locals.task.agentStake);

            // Distribute platform fee
            if (locals.platformFee > 0)
            {
                locals.burnAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
                locals.treasuryAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
                locals.arbPoolAmount = locals.platformFee - locals.burnAmount - locals.treasuryAmount;
                if (locals.burnAmount > 0)
                {
                    qpi.burn(locals.burnAmount);
                    state.mut().stats.totalBurned += locals.burnAmount;
                }
                state.mut().stats.treasuryBalance += locals.treasuryAmount;
                state.mut().stats.arbitratorPool += locals.arbPoolAmount;
            }

            locals.task.status = QAGENT_TASK_STATUS_COMPLETED;
            locals.task.finalizedTick = qpi.tick();
            state.mut().tasks.set(locals.task.taskId, locals.task);

            if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
            {
                locals.agent.completedTasks += 1;
                locals.agent.totalEarned += locals.agentPayout;

                locals.agent.tier = QAGENT_TIER_UNRANKED;
                if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
                {
                    locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                        static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                    if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                        locals.agent.tier = QAGENT_TIER_DIAMOND;
                    else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                        locals.agent.tier = QAGENT_TIER_GOLD;
                    else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                        locals.agent.tier = QAGENT_TIER_SILVER;
                    else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                        locals.agent.tier = QAGENT_TIER_BRONZE;
                }
                state.mut().agents.set(locals.task.assignedAgent, locals.agent);
            }
            if (state.get().stats.activeTasks > 0)
                state.mut().stats.activeTasks -= 1;

            locals.log = QAGENTLogger{ CONTRACT_INDEX, logEndTickAutoFinalize, locals.task.taskId, locals.task.assignedAgent, locals.agentPayout, 0 };
            LOG_INFO(locals.log);

            locals.elemIdx = state.mut().activeTaskQ.remove(locals.elemIdx);
            locals.processed += 1;
            continue;
        }

        // DISPUTED task past dispute deadline → cancel (no resolution)
        if (locals.task.status == QAGENT_TASK_STATUS_DISPUTED
            && locals.task.disputeDeadlineTick > 0
            && qpi.tick() > locals.task.disputeDeadlineTick
            && locals.task.verdict == QAGENT_VERDICT_NONE)
        {
            if (locals.task.escrowAmount > 0)
                qpi.transfer(locals.task.requester, locals.task.escrowAmount);
            if (locals.task.agentStake > 0)
            {
                qpi.burn(locals.task.agentStake);
                state.mut().stats.totalBurned += locals.task.agentStake;
            }
            if (locals.task.challengerStake > 0)
            {
                qpi.burn(locals.task.challengerStake);
                state.mut().stats.totalBurned += locals.task.challengerStake;
            }
            locals.task.status = QAGENT_TASK_STATUS_CANCELLED;
            locals.task.finalizedTick = qpi.tick();
            state.mut().tasks.set(locals.task.taskId, locals.task);

            if (state.get().stats.activeTasks > 0)
                state.mut().stats.activeTasks -= 1;
            if (state.get().stats.disputedTasks > 0)
                state.mut().stats.disputedTasks -= 1;

            locals.elemIdx = state.mut().activeTaskQ.remove(locals.elemIdx);
            locals.processed += 1;
            continue;
        }

        // Task exists but its relevant deadline hasn't passed yet for current status.
        // This happens when a task changed status (e.g. ACCEPTED→REVEALED) and the
        // queue entry still has the old deadline. Re-add with the correct deadline.
        locals.elemIdx = state.mut().activeTaskQ.remove(locals.elemIdx);

        if (locals.task.status == QAGENT_TASK_STATUS_REVEALED && locals.task.challengeDeadlineTick > 0)
            state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.challengeDeadlineTick));
        else if (locals.task.status == QAGENT_TASK_STATUS_DISPUTED && locals.task.disputeDeadlineTick > 0)
            state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.disputeDeadlineTick));
        else if (locals.task.status == QAGENT_TASK_STATUS_COMMITTED && locals.task.deadlineTick > 0)
            state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.deadlineTick));
        // else: task in unexpected state for queue, drop it

        locals.processed += 1;
    }

    // Periodic HashMap cleanup to prevent internal fragmentation (pattern from Qx.h)
    state.mut().tasks.cleanupIfNeeded(30);
    state.mut().oracleQueryToTask.cleanupIfNeeded(30);
    state.mut().interactions.cleanupIfNeeded(30);
    state.mut().proposalVoters.cleanupIfNeeded(30);
}

// ---
// Agent Lifecycle Procedures
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(RegisterAgent)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.pricePerTask < 0)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (state.get().agents.contains(qpi.invocator()))
    {
        output.returnCode = QAGENT_RC_ALREADY_EXISTS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (state.get().agents.population() >= QAGENT_MAX_AGENTS)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Require minimum registration stake
    if (qpi.invocationReward() < state.get().config.minRegistrationStake)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.agent.agentId = qpi.invocator();
    locals.agent.ownerWallet = qpi.originator();
    locals.agent.status = QAGENT_STATUS_ACTIVE;
    locals.agent.tier = QAGENT_TIER_UNRANKED;
    locals.agent.capabilityBitmap = input.capabilityBitmap;
    locals.agent.pricePerTask = input.pricePerTask;
    locals.agent.totalEarned = 0;
    locals.agent.completedTasks = 0;
    locals.agent.failedTasks = 0;
    locals.agent.totalTasks = 0;
    locals.agent.oracleVerifiedTasks = 0;
    locals.agent.oraclePassedTasks = 0;
    locals.agent.uniqueRequesters = 0;
    locals.agent.slashCount = 0;
    locals.agent.registeredTick = qpi.tick();
    locals.agent.registeredEpoch = qpi.epoch();
    state.mut().agents.set(qpi.invocator(), locals.agent);

    // Create stake record
    locals.stake.registrationStake = state.get().config.minRegistrationStake;
    locals.stake.additionalStake = 0;
    locals.stake.registeredEpoch = qpi.epoch();
    locals.stake.unstakeRequestEpoch = 0;
    locals.stake.locked = 1;
    state.mut().stakes.set(qpi.invocator(), locals.stake);

    state.mut().stats.totalAgents += 1;

    // Refund excess beyond registration stake
    locals.excess = qpi.invocationReward() - state.get().config.minRegistrationStake;
    if (locals.excess > 0)
        qpi.transfer(qpi.invocator(), locals.excess);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logRegisterAgent, 0, qpi.invocator(), state.get().config.minRegistrationStake, 0 };
    LOG_INFO(locals.log);

    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(UpdateAgent)
{
    if (!state.get().agents.get(qpi.invocator(), locals.agent))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.pricePerTask < 0)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.agent.capabilityBitmap = input.capabilityBitmap;
    locals.agent.pricePerTask = input.pricePerTask;
    state.mut().agents.set(qpi.invocator(), locals.agent);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logUpdateAgent, 0, qpi.invocator(), input.pricePerTask, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(DeactivateAgent)
{
    if (!state.get().agents.get(qpi.invocator(), locals.agent))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.agent.status = QAGENT_STATUS_INACTIVE;
    state.mut().agents.set(qpi.invocator(), locals.agent);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logDeactivateAgent, 0, qpi.invocator(), 0, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(ReactivateAgent)
{
    if (!state.get().agents.get(qpi.invocator(), locals.agent))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.agent.status != QAGENT_STATUS_INACTIVE)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.agent.status = QAGENT_STATUS_ACTIVE;
    state.mut().agents.set(qpi.invocator(), locals.agent);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logReactivateAgent, 0, qpi.invocator(), 0, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(StakeMore)
{
    // Reject SC-behind-SC calls for financial safety
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().stakes.get(qpi.invocator(), locals.stake))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocationReward() <= 0)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        return;
    }
    locals.stake.additionalStake += qpi.invocationReward();
    state.mut().stakes.set(qpi.invocator(), locals.stake);

    output.totalStake = locals.stake.registrationStake + locals.stake.additionalStake;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logStakeMore, 0, qpi.invocator(), qpi.invocationReward(), 0 };
    LOG_INFO(locals.log);

    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(RequestUnstake)
{
    // Reject SC-behind-SC calls for financial safety
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().stakes.get(qpi.invocator(), locals.stake))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.stake.locked != 0
        && qpi.epoch() < locals.stake.registeredEpoch + QAGENT_STAKE_LOCK_EPOCHS)
    {
        output.returnCode = QAGENT_RC_STAKE_LOCKED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.stake.unstakeRequestEpoch = qpi.epoch();
    locals.stake.locked = 0;
    state.mut().stakes.set(qpi.invocator(), locals.stake);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawStake)
{
    // Reject SC-behind-SC calls for financial safety
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().stakes.get(qpi.invocator(), locals.stake))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.stake.unstakeRequestEpoch == 0)
    {
        output.returnCode = QAGENT_RC_STAKE_LOCKED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Must wait at least 1 epoch after unstake request
    if (qpi.epoch() <= locals.stake.unstakeRequestEpoch)
    {
        output.returnCode = QAGENT_RC_STAKE_LOCKED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.amount = locals.stake.registrationStake + locals.stake.additionalStake;
    if (locals.amount > 0)
        qpi.transfer(qpi.invocator(), locals.amount);

    locals.stake.registrationStake = 0;
    locals.stake.additionalStake = 0;
    state.mut().stakes.set(qpi.invocator(), locals.stake);

    // Deactivate agent
    if (state.get().agents.get(qpi.invocator(), locals.agent))
    {
        locals.agent.status = QAGENT_STATUS_INACTIVE;
        state.mut().agents.set(qpi.invocator(), locals.agent);
    }

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logWithdrawStake, 0, qpi.invocator(), locals.amount, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.withdrawn = locals.amount;
    output.returnCode = QAGENT_RC_SUCCESS;
}

// ---
// Service Registry Procedures
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(RegisterService)
{
    if (!state.get().agents.contains(qpi.invocator()))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (state.get().services.population() >= QAGENT_MAX_SERVICES)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.taskType > QAGENT_TASK_TYPE_CUSTOM || input.basePrice < 0)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.svc.ownerId = qpi.invocator();
    locals.svc.capabilityBitmap = input.capabilityBitmap;
    locals.svc.basePrice = input.basePrice;
    locals.svc.taskType = input.taskType;
    locals.svc.active = 1;
    locals.svc.registeredTick = qpi.tick();
    state.mut().services.set(qpi.invocator(), locals.svc);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logRegisterService, 0, qpi.invocator(), input.basePrice, input.taskType };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(DeprecateService)
{
    if (!state.get().services.get(input.serviceId, locals.svc))
    {
        output.returnCode = QAGENT_RC_SERVICE_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.svc.ownerId != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.svc.active = 0;
    state.mut().services.set(input.serviceId, locals.svc);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logDeprecateService, 0, qpi.invocator(), 0, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(UpdateServicePrice)
{
    if (!state.get().services.get(input.serviceId, locals.svc))
    {
        output.returnCode = QAGENT_RC_SERVICE_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.svc.ownerId != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.newPrice < 0)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.svc.basePrice = input.newPrice;
    state.mut().services.set(input.serviceId, locals.svc);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logUpdateServicePrice, 0, qpi.invocator(), input.newPrice, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

// ---
// Task Lifecycle Procedures
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(CreateTask)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Minimum escrow
    if (qpi.invocationReward() < state.get().config.minEscrow)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Agent must exist and be active
    if (!state.get().agents.get(input.agentId, locals.agent))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.agent.status != QAGENT_STATUS_ACTIVE)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Anti-self-dealing: requester must not be the agent itself
    if (qpi.invocator() == input.agentId)
    {
        output.returnCode = QAGENT_RC_SELF_DEALING;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Anti-self-dealing: originator must not be agent's owner
    if (qpi.originator() == locals.agent.ownerWallet)
    {
        output.returnCode = QAGENT_RC_SELF_DEALING;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (state.get().tasks.population() >= QAGENT_MAX_TASKS)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.taskType > QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    state.mut().taskCounter += 1;

    locals.task.taskId = state.get().taskCounter;
    locals.task.requester = qpi.invocator();
    locals.task.assignedAgent = input.agentId;
    locals.task.taskType = input.taskType;
    locals.task.status = QAGENT_TASK_STATUS_OPEN;
    locals.task.escrowAmount = qpi.invocationReward();
    locals.task.agentStake = 0;
    locals.task.challengerStake = 0;
    locals.task.challengerId = NULL_ID;
    locals.task.commitHash = NULL_ID;
    locals.task.resultHash = NULL_ID;
    locals.task.agentSalt = NULL_ID;
    locals.task.specHash = input.specHash;
    locals.task.modelHash = input.modelHash;
    locals.task.dataCid = NULL_ID;
    locals.task.resultEnum = 0;
    locals.task.totalMilestones = (input.totalMilestones > 0) ? input.totalMilestones : 1;
    locals.task.completedMilestones = 0;
    locals.task.createdTick = qpi.tick();
    locals.task.deadlineTick = (input.deadlineTicks > 0)
        ? qpi.tick() + input.deadlineTicks
        : qpi.tick() + static_cast<uint32>(state.get().config.defaultDeadlineTicks);
    locals.task.submitTick = 0;
    locals.task.challengeDeadlineTick = 0;
    locals.task.disputeDeadlineTick = 0;
    locals.task.finalizedTick = 0;
    locals.task.disputeVotesValid = 0;
    locals.task.disputeVotesInvalid = 0;
    locals.task.verdict = QAGENT_VERDICT_NONE;
    locals.task.disputeVoters_0 = NULL_ID;
    locals.task.disputeVoters_1 = NULL_ID;
    locals.task.disputeVoters_2 = NULL_ID;
    locals.task.disputeVoters_3 = NULL_ID;
    locals.task.disputeVoters_4 = NULL_ID;
    locals.task.disputeVoterCount = 0;
    locals.task.rated = 0;
    locals.task.oracleQueryId = 0;
    locals.task.oracleRequested = 0;
    locals.task.inferenceConfig = 0;
    locals.task.parentTaskId = 0;

    state.mut().tasks.set(locals.task.taskId, locals.task);

    state.mut().stats.totalTasks += 1;
    state.mut().stats.activeTasks += 1;
    state.mut().stats.totalVolume += qpi.invocationReward();

    // Track interaction for anti-Sybil (unique requester count)
    locals.ikey.agentId = input.agentId;
    locals.ikey.requesterId = qpi.invocator();
    locals.ikeyHash = qpi.K12(locals.ikey);
    if (!state.get().interactions.get(locals.ikeyHash, locals.icount))
    {
        locals.icount = 0;
        // New unique requester
        locals.agent.uniqueRequesters += 1;
        state.mut().agents.set(input.agentId, locals.agent);
    }
    locals.icount += 1;
    state.mut().interactions.set(locals.ikeyHash, locals.icount);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logCreateTask, locals.task.taskId, qpi.invocator(), qpi.invocationReward(), input.taskType };
    LOG_INFO(locals.log);

    output.returnCode = QAGENT_RC_SUCCESS;
    output.taskId = locals.task.taskId;
}

PUBLIC_PROCEDURE_WITH_LOCALS(AcceptTask)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.assignedAgent != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_OPEN)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Reputation-based bond: calculate required stake based on agent tier
    if (!state.get().agents.get(qpi.invocator(), locals.agent))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Suspended or inactive agents must not accept new tasks
    if (locals.agent.status != QAGENT_STATUS_ACTIVE)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Tier-based bond multiplier (BPS of escrow)
    if (locals.agent.tier == QAGENT_TIER_DIAMOND)
        locals.bondBPS = 7500;
    else if (locals.agent.tier == QAGENT_TIER_GOLD)
        locals.bondBPS = 10000;
    else if (locals.agent.tier == QAGENT_TIER_SILVER)
        locals.bondBPS = 15000;
    else if (locals.agent.tier == QAGENT_TIER_BRONZE)
        locals.bondBPS = 20000;
    else
        locals.bondBPS = 30000;

    locals.requiredStake = div<sint64>(
        smul(locals.task.escrowAmount, static_cast<sint64>(locals.bondBPS)),
        static_cast<sint64>(QAGENT_BPS_SCALE));

    if (qpi.invocationReward() < locals.requiredStake)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.task.agentStake = locals.requiredStake;
    locals.task.status = QAGENT_TASK_STATUS_ACCEPTED;

    locals.excess = qpi.invocationReward() - locals.requiredStake;
    if (locals.excess > 0)
        qpi.transfer(qpi.invocator(), locals.excess);

    state.mut().tasks.set(input.taskId, locals.task);

    // Update agent task counter
    locals.agent.totalTasks += 1;
    state.mut().agents.set(qpi.invocator(), locals.agent);

    // Add to priority queue with deadline
    state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.deadlineTick));

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logAcceptTask, input.taskId, qpi.invocator(), locals.requiredStake, locals.agent.tier };
    LOG_INFO(locals.log);

    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(CommitResult)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.assignedAgent != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_ACCEPTED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.tick() > locals.task.deadlineTick)
    {
        output.returnCode = QAGENT_RC_DEADLINE_PASSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.task.commitHash = input.commitHash;
    locals.task.submitTick = qpi.tick();
    locals.task.status = QAGENT_TASK_STATUS_COMMITTED;
    state.mut().tasks.set(input.taskId, locals.task);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logCommitResult, input.taskId, qpi.invocator(), 0, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(RevealResult)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.assignedAgent != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_COMMITTED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.tick() > locals.task.deadlineTick)
    {
        output.returnCode = QAGENT_RC_DEADLINE_PASSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Verify commit-reveal: K12(resultHash || salt) must equal commitHash
    locals.crData.resultHash = input.resultHash;
    locals.crData.salt = input.salt;
    locals.computedHash = qpi.K12(locals.crData);

    if (locals.computedHash != locals.task.commitHash)
    {
        output.returnCode = QAGENT_RC_COMMIT_MISMATCH;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.task.resultHash = input.resultHash;
    locals.task.agentSalt = input.salt;
    locals.task.dataCid = input.dataCid;
    locals.task.resultEnum = input.resultEnum;
    locals.task.challengeDeadlineTick = qpi.tick() + static_cast<uint32>(state.get().config.challengeWindowTicks);
    locals.task.status = QAGENT_TASK_STATUS_REVEALED;
    state.mut().tasks.set(input.taskId, locals.task);

    // Add to active queue with challenge deadline for auto-finalization
    state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.challengeDeadlineTick));

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logRevealResult, input.taskId, qpi.invocator(), 0, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(ApproveResult)
{
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.requester != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_REVEALED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Compute and distribute fees
    locals.platformFee = div<sint64>(
        smul(locals.task.escrowAmount, state.get().config.platformFeeBPS),
        static_cast<sint64>(QAGENT_BPS_SCALE));
    locals.agentPayout = locals.task.escrowAmount - locals.platformFee;

    if (locals.agentPayout > 0)
        qpi.transfer(locals.task.assignedAgent, locals.agentPayout);
    if (locals.task.agentStake > 0)
        qpi.transfer(locals.task.assignedAgent, locals.task.agentStake);

    if (locals.platformFee > 0)
    {
        locals.burnAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.treasuryAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.arbPoolAmount = locals.platformFee - locals.burnAmount - locals.treasuryAmount;
        if (locals.burnAmount > 0)
        {
            qpi.burn(locals.burnAmount);
            state.mut().stats.totalBurned += locals.burnAmount;
        }
        state.mut().stats.treasuryBalance += locals.treasuryAmount;
        state.mut().stats.arbitratorPool += locals.arbPoolAmount;
    }

    locals.task.status = QAGENT_TASK_STATUS_COMPLETED;
    locals.task.finalizedTick = qpi.tick();
    state.mut().tasks.set(input.taskId, locals.task);

    if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
    {
        locals.agent.completedTasks += 1;
        locals.agent.totalEarned += locals.agentPayout;

        locals.agent.tier = QAGENT_TIER_UNRANKED;
        if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
        {
            locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
            if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                locals.agent.tier = QAGENT_TIER_DIAMOND;
            else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                locals.agent.tier = QAGENT_TIER_GOLD;
            else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                locals.agent.tier = QAGENT_TIER_SILVER;
            else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                locals.agent.tier = QAGENT_TIER_BRONZE;
        }
        state.mut().agents.set(locals.task.assignedAgent, locals.agent);
    }

    if (state.get().stats.activeTasks > 0)
        state.mut().stats.activeTasks -= 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logApproveResult, input.taskId, locals.task.assignedAgent, locals.agentPayout, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(ChallengeResult)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Can't challenge own result (assigned agent)
    if (qpi.invocator() == locals.task.assignedAgent)
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Can't challenge own task as requester (prevents self-dealing cycle:
    // requester challenges → votes as arbitrator → wins escrow + agent stake)
    if (qpi.invocator() == locals.task.requester)
    {
        output.returnCode = QAGENT_RC_SELF_DEALING;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_REVEALED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.tick() > locals.task.challengeDeadlineTick)
    {
        output.returnCode = QAGENT_RC_CHALLENGE_WINDOW_CLOSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Challenger must match agent's bond
    if (qpi.invocationReward() < locals.task.agentStake)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.excess = qpi.invocationReward() - locals.task.agentStake;
    locals.task.challengerStake = locals.task.agentStake;
    locals.task.challengerId = qpi.invocator();
    locals.task.status = QAGENT_TASK_STATUS_DISPUTED;
    locals.task.disputeDeadlineTick = qpi.tick() + static_cast<uint32>(state.get().config.disputeResolutionTicks);

    if (locals.excess > 0)
        qpi.transfer(qpi.invocator(), locals.excess);

    state.mut().tasks.set(input.taskId, locals.task);
    state.mut().stats.disputedTasks += 1;

    // Add to queue with dispute deadline
    state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.disputeDeadlineTick));

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logChallengeResult, input.taskId, qpi.invocator(), locals.task.challengerStake, 0 };
    LOG_INFO(locals.log);

    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(FinalizeTask)
{
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_REVEALED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.tick() <= locals.task.challengeDeadlineTick)
    {
        output.returnCode = QAGENT_RC_CHALLENGE_WINDOW_OPEN;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.platformFee = div<sint64>(
        smul(locals.task.escrowAmount, state.get().config.platformFeeBPS),
        static_cast<sint64>(QAGENT_BPS_SCALE));
    locals.keeperFee = div<sint64>(
        smul(locals.task.escrowAmount, state.get().config.keeperFeeBPS),
        static_cast<sint64>(QAGENT_BPS_SCALE));
    if (locals.keeperFee < QAGENT_MIN_KEEPER_FEE)
        locals.keeperFee = QAGENT_MIN_KEEPER_FEE;
    if (locals.platformFee + locals.keeperFee > locals.task.escrowAmount)
        locals.keeperFee = locals.task.escrowAmount - locals.platformFee;
    if (locals.keeperFee < 0)
        locals.keeperFee = 0;

    locals.agentPayout = locals.task.escrowAmount - locals.platformFee - locals.keeperFee;

    if (locals.agentPayout > 0)
        qpi.transfer(locals.task.assignedAgent, locals.agentPayout);
    if (locals.task.agentStake > 0)
        qpi.transfer(locals.task.assignedAgent, locals.task.agentStake);
    if (locals.keeperFee > 0)
        qpi.transfer(qpi.invocator(), locals.keeperFee);

    if (locals.platformFee > 0)
    {
        locals.burnAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.treasuryAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.arbPoolAmount = locals.platformFee - locals.burnAmount - locals.treasuryAmount;
        if (locals.burnAmount > 0)
        {
            qpi.burn(locals.burnAmount);
            state.mut().stats.totalBurned += locals.burnAmount;
        }
        state.mut().stats.treasuryBalance += locals.treasuryAmount;
        state.mut().stats.arbitratorPool += locals.arbPoolAmount;
    }

    locals.task.status = QAGENT_TASK_STATUS_COMPLETED;
    locals.task.finalizedTick = qpi.tick();
    state.mut().tasks.set(input.taskId, locals.task);

    if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
    {
        locals.agent.completedTasks += 1;
        locals.agent.totalEarned += locals.agentPayout;

        locals.agent.tier = QAGENT_TIER_UNRANKED;
        if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
        {
            locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
            if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                locals.agent.tier = QAGENT_TIER_DIAMOND;
            else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                locals.agent.tier = QAGENT_TIER_GOLD;
            else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                locals.agent.tier = QAGENT_TIER_SILVER;
            else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                locals.agent.tier = QAGENT_TIER_BRONZE;
        }
        state.mut().agents.set(locals.task.assignedAgent, locals.agent);
    }
    if (state.get().stats.activeTasks > 0)
        state.mut().stats.activeTasks -= 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logFinalizeTask, locals.task.taskId, qpi.invocator(), locals.keeperFee, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.returnCode = QAGENT_RC_SUCCESS;
    output.keeperFee = locals.keeperFee;
}

PUBLIC_PROCEDURE_WITH_LOCALS(TimeoutTask)
{
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_ACCEPTED
        && locals.task.status != QAGENT_TASK_STATUS_COMMITTED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.tick() <= locals.task.deadlineTick)
    {
        output.returnCode = QAGENT_RC_DEADLINE_NOT_REACHED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Keeper fee from agent's stake
    locals.keeperFee = div<sint64>(
        smul(locals.task.agentStake, state.get().config.keeperFeeBPS),
        static_cast<sint64>(QAGENT_BPS_SCALE));
    if (locals.keeperFee < QAGENT_MIN_KEEPER_FEE && locals.task.agentStake >= QAGENT_MIN_KEEPER_FEE)
        locals.keeperFee = QAGENT_MIN_KEEPER_FEE;
    // Guard: keeper fee must never exceed agent stake (prevents underflow if governance sets keeperFeeBPS > 10000)
    if (locals.keeperFee > locals.task.agentStake)
        locals.keeperFee = locals.task.agentStake;

    // Return escrow: delegated tasks restore to parent task, root tasks refund requester
    if (locals.task.escrowAmount > 0)
    {
        if (locals.task.parentTaskId != 0 && state.get().tasks.get(locals.task.parentTaskId, locals.parentTask))
        {
            // Delegated task: return escrow to parent task's escrow pool (prevents agent from pocketing via failed delegation)
            locals.parentTask.escrowAmount += locals.task.escrowAmount;
            state.mut().tasks.set(locals.task.parentTaskId, locals.parentTask);
        }
        else
        {
            qpi.transfer(locals.task.requester, locals.task.escrowAmount);
        }
    }
    // Keeper gets fee
    if (locals.keeperFee > 0)
        qpi.transfer(qpi.invocator(), locals.keeperFee);

    // Progressive slashing of agent stake
    if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
    {
        locals.agent.failedTasks += 1;
        locals.agent.slashCount += 1;

        // Progressive: 100% of task bond always burned; at 4+ offenses also 25% of registration stake + suspension
        if (locals.agent.slashCount >= 4)
        {
            locals.agent.status = QAGENT_STATUS_SUSPENDED;
            // Slash 100% of task bond + 25% of registration stake
            if (state.get().stakes.get(locals.task.assignedAgent, locals.stake))
            {
                locals.slashAmount = div<sint64>(locals.stake.registrationStake, static_cast<sint64>(4));
                // Guard: slash cannot exceed remaining registration stake
                if (locals.slashAmount > locals.stake.registrationStake)
                    locals.slashAmount = locals.stake.registrationStake;
                locals.stake.registrationStake -= locals.slashAmount;
                state.mut().stakes.set(locals.task.assignedAgent, locals.stake);
                // Burn the registration slash
                if (locals.slashAmount > 0)
                {
                    qpi.burn(locals.slashAmount);
                    state.mut().stats.totalBurned += locals.slashAmount;
                }
            }
        }

        locals.agent.tier = QAGENT_TIER_UNRANKED;
        if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
        {
            locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
            if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                locals.agent.tier = QAGENT_TIER_DIAMOND;
            else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                locals.agent.tier = QAGENT_TIER_GOLD;
            else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                locals.agent.tier = QAGENT_TIER_SILVER;
            else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                locals.agent.tier = QAGENT_TIER_BRONZE;
        }
        state.mut().agents.set(locals.task.assignedAgent, locals.agent);
    }

    // Burn remaining agent stake (minus keeper fee)
    locals.burnAmount = locals.task.agentStake - locals.keeperFee;
    if (locals.burnAmount > 0)
    {
        qpi.burn(locals.burnAmount);
        state.mut().stats.totalBurned += locals.burnAmount;
    }

    locals.task.status = QAGENT_TASK_STATUS_TIMED_OUT;
    locals.task.finalizedTick = qpi.tick();
    state.mut().tasks.set(input.taskId, locals.task);

    if (state.get().stats.activeTasks > 0)
        state.mut().stats.activeTasks -= 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logTimeoutTask, locals.task.taskId, locals.task.assignedAgent, locals.burnAmount, locals.agent.slashCount };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.returnCode = QAGENT_RC_SUCCESS;
    output.keeperFee = locals.keeperFee;
}

PUBLIC_PROCEDURE_WITH_LOCALS(CancelTask)
{
    // Reject SC-behind-SC calls for financial safety
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.requester != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_OPEN)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    if (locals.task.escrowAmount > 0)
        qpi.transfer(qpi.invocator(), locals.task.escrowAmount);

    locals.task.status = QAGENT_TASK_STATUS_CANCELLED;
    locals.task.finalizedTick = qpi.tick();
    state.mut().tasks.set(input.taskId, locals.task);

    if (state.get().stats.activeTasks > 0)
        state.mut().stats.activeTasks -= 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logCancelTask, input.taskId, qpi.invocator(), locals.task.escrowAmount, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(CompleteMilestone)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Only requester can approve milestones
    if (locals.task.requester != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_REVEALED
        && locals.task.status != QAGENT_TASK_STATUS_ACCEPTED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.completedMilestones >= locals.task.totalMilestones)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.task.completedMilestones += 1;

    // Release proportional escrow (based on remaining escrow, not original)
    locals.payout = div<sint64>(locals.task.escrowAmount, static_cast<sint64>(locals.task.totalMilestones - locals.task.completedMilestones + 1));
    locals.task.escrowAmount -= locals.payout;

    // Deduct platform fee from milestone payout (same rate as ApproveResult/FinalizeTask)
    locals.platformFee = div<sint64>(smul(locals.payout, state.get().config.platformFeeBPS), static_cast<sint64>(QAGENT_BPS_SCALE));
    locals.payout -= locals.platformFee;

    if (locals.payout > 0)
        qpi.transfer(locals.task.assignedAgent, locals.payout);

    // Distribute platform fee 3-way: burn/treasury/arbPool
    if (locals.platformFee > 0)
    {
        locals.burnAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.treasuryAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        if (locals.burnAmount > 0)
        {
            qpi.burn(locals.burnAmount);
            state.mut().stats.totalBurned += locals.burnAmount;
        }
        state.mut().stats.treasuryBalance += locals.treasuryAmount;
        state.mut().stats.arbitratorPool += locals.platformFee - locals.burnAmount - locals.treasuryAmount;
    }

    // If all milestones done, complete the task
    if (locals.task.completedMilestones >= locals.task.totalMilestones)
    {
        if (locals.task.agentStake > 0)
            qpi.transfer(locals.task.assignedAgent, locals.task.agentStake);
        locals.task.status = QAGENT_TASK_STATUS_COMPLETED;
        locals.task.finalizedTick = qpi.tick();

        if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
        {
            locals.agent.completedTasks += 1;
            locals.agent.totalEarned += locals.payout;

            locals.agent.tier = QAGENT_TIER_UNRANKED;
            if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
            {
                locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                    static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                    locals.agent.tier = QAGENT_TIER_DIAMOND;
                else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                    locals.agent.tier = QAGENT_TIER_GOLD;
                else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                    locals.agent.tier = QAGENT_TIER_SILVER;
                else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                    locals.agent.tier = QAGENT_TIER_BRONZE;
            }
            state.mut().agents.set(locals.task.assignedAgent, locals.agent);
        }
        if (state.get().stats.activeTasks > 0)
            state.mut().stats.activeTasks -= 1;
    }

    state.mut().tasks.set(input.taskId, locals.task);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logCompleteMilestone, locals.task.taskId, qpi.invocator(), locals.payout, locals.task.completedMilestones };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.milestonePayout = locals.payout;
    output.returnCode = QAGENT_RC_SUCCESS;
}

// ---
// Dispute Resolution Procedures
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(RequestOracleVerification)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Only disputed or revealed tasks can be oracle-verified
    if (locals.task.status != QAGENT_TASK_STATUS_DISPUTED
        && locals.task.status != QAGENT_TASK_STATUS_REVEALED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.oracleRequested != 0)
    {
        output.returnCode = QAGENT_RC_ORACLE_PENDING;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Build oracle query — use setMemory to zero struct (no [] brackets)
    setMemory(locals.oracleQuery, 0);
    locals.oracleQuery.taskId = locals.task.taskId;
    locals.oracleQuery.specHash = locals.task.specHash;
    locals.oracleQuery.resultHash = locals.task.resultHash;
    locals.oracleQuery.modelHash = locals.task.modelHash;
    locals.oracleQuery.taskType = locals.task.taskType;
    locals.oracleQuery.outputFormat = 0;
    locals.oracleQuery.inferenceConfig = locals.task.inferenceConfig;

    output.oracleQueryId = QUERY_ORACLE(OI::AIVerify, locals.oracleQuery, OnAIVerifyReply, QAGENT_ORACLE_TIMEOUT_MS);

    if (output.oracleQueryId < 0)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // O(1) oracle query → task mapping
    state.mut().oracleQueryToTask.set(output.oracleQueryId, locals.task.taskId);

    locals.task.oracleQueryId = output.oracleQueryId;
    locals.task.oracleRequested = 1;
    locals.task.status = QAGENT_TASK_STATUS_ORACLE_PENDING;
    state.mut().tasks.set(input.taskId, locals.task);

    state.mut().stats.oracleVerifications += 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logRequestOracle, locals.task.taskId, qpi.invocator(), output.oracleQueryId, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PRIVATE_PROCEDURE_WITH_LOCALS(OnAIVerifyReply)
{
    // O(1) oracle query → task lookup via HashMap
    locals.foundTaskId = 0;
    if (!state.get().oracleQueryToTask.get(input.queryId, locals.foundTaskId))
        return; // No mapping — nothing to do

    if (!state.get().tasks.get(locals.foundTaskId, locals.task))
        return; // Task not found — stale mapping

    if (locals.task.oracleQueryId != input.queryId || locals.task.taskId == 0)
        return; // Sanity check

    if (input.status != ORACLE_QUERY_STATUS_SUCCESS)
    {
        // Oracle failed — revert to disputed status for arbitrator resolution
        locals.task.status = QAGENT_TASK_STATUS_DISPUTED;
        locals.task.oracleRequested = 0;
        locals.task.disputeDeadlineTick = qpi.tick() + static_cast<uint32>(state.get().config.disputeResolutionTicks);
        state.mut().tasks.set(locals.task.taskId, locals.task);
        state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.disputeDeadlineTick));
        state.mut().stats.disputedTasks += 1;
        return;
    }

    // Validate oracle reply has a decisive verdict with confidence > 0
    // (matches AIVerify::replyIsValid pattern — inlined to avoid QPI scope resolution)
    if (input.reply.verdict == 0 || input.reply.confidence == 0)
    {
        // Non-decisive reply (abstain or zero confidence) — fall back to dispute
        locals.task.status = QAGENT_TASK_STATUS_DISPUTED;
        locals.task.oracleRequested = 0;
        locals.task.disputeDeadlineTick = qpi.tick() + static_cast<uint32>(state.get().config.disputeResolutionTicks);
        state.mut().tasks.set(locals.task.taskId, locals.task);
        state.mut().activeTaskQ.add(SELF, locals.task.taskId, static_cast<sint64>(locals.task.disputeDeadlineTick));
        return;
    }

    // Oracle verdict received (decisive with confidence > 0)
    if (input.reply.verdict == QAGENT_VERDICT_VALID)
    {
        // Agent's result confirmed valid — pay agent, return challenger stake
        locals.platformFee = div<sint64>(
            smul(locals.task.escrowAmount, state.get().config.platformFeeBPS),
            static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.agentPayout = locals.task.escrowAmount - locals.platformFee;

        if (locals.agentPayout > 0)
            qpi.transfer(locals.task.assignedAgent, locals.agentPayout);
        if (locals.task.agentStake > 0)
            qpi.transfer(locals.task.assignedAgent, locals.task.agentStake);

        // Slash challenger (failed challenge): 50% of challenger stake burned
        if (locals.task.challengerStake > 0)
        {
            locals.slashAmount = div<sint64>(locals.task.challengerStake, static_cast<sint64>(2));
            locals.burnAmount = locals.task.challengerStake - locals.slashAmount;
            if (locals.slashAmount > 0)
                qpi.transfer(locals.task.challengerId, locals.slashAmount);
            if (locals.burnAmount > 0)
            {
                qpi.burn(locals.burnAmount);
                state.mut().stats.totalBurned += locals.burnAmount;
            }
        }

        // Three-way fee split: 50% burn, 30% treasury, 20% arbitrator pool
        if (locals.platformFee > 0)
        {
            locals.burnAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
            locals.slashAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
            if (locals.burnAmount > 0)
            {
                qpi.burn(locals.burnAmount);
                state.mut().stats.totalBurned += locals.burnAmount;
            }
            state.mut().stats.treasuryBalance += locals.slashAmount;
            state.mut().stats.arbitratorPool += locals.platformFee - locals.burnAmount - locals.slashAmount;
        }

        locals.task.verdict = QAGENT_VERDICT_VALID;
        locals.task.status = QAGENT_TASK_STATUS_COMPLETED;
        locals.task.finalizedTick = qpi.tick();
        state.mut().tasks.set(locals.task.taskId, locals.task);

        if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
        {
            locals.agent.completedTasks += 1;
            locals.agent.oracleVerifiedTasks += 1;
            locals.agent.oraclePassedTasks += 1;
            locals.agent.totalEarned += locals.agentPayout;

            locals.agent.tier = QAGENT_TIER_UNRANKED;
            if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
            {
                locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                    static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                    locals.agent.tier = QAGENT_TIER_DIAMOND;
                else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                    locals.agent.tier = QAGENT_TIER_GOLD;
                else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                    locals.agent.tier = QAGENT_TIER_SILVER;
                else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                    locals.agent.tier = QAGENT_TIER_BRONZE;
            }
            state.mut().agents.set(locals.task.assignedAgent, locals.agent);
        }

        locals.log = QAGENTLogger{ CONTRACT_INDEX, logOracleReply, locals.task.taskId, locals.task.assignedAgent, locals.agentPayout, QAGENT_VERDICT_VALID };
        LOG_INFO(locals.log);
    }
    else if (input.reply.verdict == QAGENT_VERDICT_INVALID)
    {
        // Agent's result invalid — refund requester, slash agent
        if (locals.task.escrowAmount > 0)
            qpi.transfer(locals.task.requester, locals.task.escrowAmount);

        // Slash agent bond progressively
        if (locals.task.agentStake > 0)
        {
            // Distribution: 40% burn, 30% challenger, 20% arbitrators, 10% treasury
            locals.burnAmount = div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
            locals.slashAmount = div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_CHALLENGER_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));

            if (locals.task.challengerId != NULL_ID && locals.slashAmount > 0)
                qpi.transfer(locals.task.challengerId, locals.slashAmount);
            else if (locals.slashAmount > 0)
            {
                // No challenger — redirect challenger share to burn
                qpi.burn(locals.slashAmount);
                state.mut().stats.totalBurned += locals.slashAmount;
            }
            if (locals.burnAmount > 0)
            {
                qpi.burn(locals.burnAmount);
                state.mut().stats.totalBurned += locals.burnAmount;
            }
            state.mut().stats.arbitratorPool += div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_ARBITRATOR_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
            state.mut().stats.treasuryBalance += div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        }

        // Return challenger's stake
        if (locals.task.challengerStake > 0)
            qpi.transfer(locals.task.challengerId, locals.task.challengerStake);

        locals.task.verdict = QAGENT_VERDICT_INVALID;
        locals.task.status = QAGENT_TASK_STATUS_CANCELLED;
        locals.task.finalizedTick = qpi.tick();
        state.mut().tasks.set(locals.task.taskId, locals.task);

        if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
        {
            locals.agent.failedTasks += 1;
            locals.agent.oracleVerifiedTasks += 1;
            locals.agent.slashCount += 1;

            locals.agent.tier = QAGENT_TIER_UNRANKED;
            if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
            {
                locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                    static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                    locals.agent.tier = QAGENT_TIER_DIAMOND;
                else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                    locals.agent.tier = QAGENT_TIER_GOLD;
                else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                    locals.agent.tier = QAGENT_TIER_SILVER;
                else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                    locals.agent.tier = QAGENT_TIER_BRONZE;
            }
            state.mut().agents.set(locals.task.assignedAgent, locals.agent);
        }

        locals.log = QAGENTLogger{ CONTRACT_INDEX, logOracleReply, locals.task.taskId, locals.task.assignedAgent, 0, QAGENT_VERDICT_INVALID };
        LOG_INFO(locals.log);
    }

    if (state.get().stats.activeTasks > 0)
        state.mut().stats.activeTasks -= 1;
    // Only decrement disputedTasks if the task was actually disputed before oracle
    // (tasks in REVEALED status that went directly to oracle were never counted as disputed)
    if (locals.task.challengerId != NULL_ID && state.get().stats.disputedTasks > 0)
        state.mut().stats.disputedTasks -= 1;

    // Remove resolved oracle mapping to prevent HashMap exhaustion
    state.mut().oracleQueryToTask.removeByKey(input.queryId);
}

PUBLIC_PROCEDURE_WITH_LOCALS(CastDisputeVote)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_DISPUTED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().arbitrators.get(qpi.invocator(), locals.arb))
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.vote != QAGENT_VERDICT_VALID && input.vote != QAGENT_VERDICT_INVALID)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.disputeVoterCount >= QAGENT_DISPUTE_VALIDATORS)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Exclude parties involved in the dispute from voting (conflict of interest)
    if (qpi.invocator() == locals.task.assignedAgent
        || qpi.invocator() == locals.task.requester
        || qpi.invocator() == locals.task.challengerId)
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Check for duplicate votes
    locals.alreadyVoted = 0;
    if (locals.task.disputeVoters_0 == qpi.invocator()) locals.alreadyVoted = 1;
    if (locals.task.disputeVoters_1 == qpi.invocator()) locals.alreadyVoted = 1;
    if (locals.task.disputeVoters_2 == qpi.invocator()) locals.alreadyVoted = 1;
    if (locals.task.disputeVoters_3 == qpi.invocator()) locals.alreadyVoted = 1;
    if (locals.task.disputeVoters_4 == qpi.invocator()) locals.alreadyVoted = 1;
    if (locals.alreadyVoted != 0)
    {
        output.returnCode = QAGENT_RC_ALREADY_EXISTS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Record vote
    if (locals.task.disputeVoterCount == 0) locals.task.disputeVoters_0 = qpi.invocator();
    else if (locals.task.disputeVoterCount == 1) locals.task.disputeVoters_1 = qpi.invocator();
    else if (locals.task.disputeVoterCount == 2) locals.task.disputeVoters_2 = qpi.invocator();
    else if (locals.task.disputeVoterCount == 3) locals.task.disputeVoters_3 = qpi.invocator();
    else if (locals.task.disputeVoterCount == 4) locals.task.disputeVoters_4 = qpi.invocator();
    locals.task.disputeVoterCount += 1;

    if (input.vote == QAGENT_VERDICT_VALID)
        locals.task.disputeVotesValid += 1;
    else
        locals.task.disputeVotesInvalid += 1;

    // Update arbitrator stats
    locals.arb.casesHandled += 1;

    // Check for resolution (3-of-5 majority)
    locals.resolved = 0;
    if (locals.task.disputeVotesValid >= QAGENT_DISPUTE_THRESHOLD)
    {
        locals.resolved = 1;
        locals.task.verdict = QAGENT_VERDICT_VALID;
        locals.task.status = QAGENT_TASK_STATUS_COMPLETED;
        locals.task.finalizedTick = qpi.tick();

        // Commit resolved state BEFORE transfers (prevents same-tick re-entry)
        state.mut().tasks.set(input.taskId, locals.task);

        locals.platformFee = div<sint64>(
            smul(locals.task.escrowAmount, state.get().config.platformFeeBPS),
            static_cast<sint64>(QAGENT_BPS_SCALE));

        locals.winnerPayout = locals.task.escrowAmount
            + div<sint64>(locals.task.challengerStake, static_cast<sint64>(2))
            - locals.platformFee;

        if (locals.winnerPayout > 0)
            qpi.transfer(locals.task.assignedAgent, locals.winnerPayout);
        if (locals.task.agentStake > 0)
            qpi.transfer(locals.task.assignedAgent, locals.task.agentStake);

        // Burn half of challenger's losing stake
        locals.burnAmount = locals.task.challengerStake
            - div<sint64>(locals.task.challengerStake, static_cast<sint64>(2));
        if (locals.burnAmount > 0)
        {
            qpi.burn(locals.burnAmount);
            state.mut().stats.totalBurned += locals.burnAmount;
        }

        // Distribute platform fee 3-way (burn/treasury/arbPool) — same as ApproveResult/FinalizeTask
        if (locals.platformFee > 0)
        {
            locals.slashAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
            locals.treasuryAmount = div<sint64>(smul(locals.platformFee, static_cast<sint64>(QAGENT_FEE_TREASURY_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
            if (locals.slashAmount > 0)
            {
                qpi.burn(locals.slashAmount);
                state.mut().stats.totalBurned += locals.slashAmount;
            }
            state.mut().stats.treasuryBalance += locals.treasuryAmount;
            // Distribute arbPool portion directly to dispute voters
            locals.arbReward = locals.platformFee - locals.slashAmount - locals.treasuryAmount;
            if (locals.arbReward > 0 && locals.task.disputeVoterCount > 0)
            {
                locals.perVoterReward = div<sint64>(locals.arbReward, static_cast<sint64>(locals.task.disputeVoterCount));
                if (locals.perVoterReward > 0)
                {
                    if (locals.task.disputeVoters_0 != NULL_ID)
                        qpi.transfer(locals.task.disputeVoters_0, locals.perVoterReward);
                    if (locals.task.disputeVoters_1 != NULL_ID)
                        qpi.transfer(locals.task.disputeVoters_1, locals.perVoterReward);
                    if (locals.task.disputeVoters_2 != NULL_ID)
                        qpi.transfer(locals.task.disputeVoters_2, locals.perVoterReward);
                    if (locals.task.disputeVoters_3 != NULL_ID)
                        qpi.transfer(locals.task.disputeVoters_3, locals.perVoterReward);
                    if (locals.task.disputeVoters_4 != NULL_ID)
                        qpi.transfer(locals.task.disputeVoters_4, locals.perVoterReward);
                }
            }
        }

        if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
        {
            locals.agent.completedTasks += 1;
            locals.agent.totalEarned += locals.winnerPayout;

            locals.agent.tier = QAGENT_TIER_UNRANKED;
            if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
            {
                locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                    static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                    locals.agent.tier = QAGENT_TIER_DIAMOND;
                else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                    locals.agent.tier = QAGENT_TIER_GOLD;
                else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                    locals.agent.tier = QAGENT_TIER_SILVER;
                else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                    locals.agent.tier = QAGENT_TIER_BRONZE;
            }
            state.mut().agents.set(locals.task.assignedAgent, locals.agent);
        }

        locals.arb.majorityVotes += 1;

        if (state.get().stats.activeTasks > 0)
            state.mut().stats.activeTasks -= 1;
        if (state.get().stats.disputedTasks > 0)
            state.mut().stats.disputedTasks -= 1;
    }
    else if (locals.task.disputeVotesInvalid >= QAGENT_DISPUTE_THRESHOLD)
    {
        locals.resolved = 1;
        locals.task.verdict = QAGENT_VERDICT_INVALID;
        locals.task.status = QAGENT_TASK_STATUS_CANCELLED;
        locals.task.finalizedTick = qpi.tick();

        // Commit resolved state BEFORE transfers (prevents same-tick re-entry)
        state.mut().tasks.set(input.taskId, locals.task);

        // Refund requester
        if (locals.task.escrowAmount > 0)
            qpi.transfer(locals.task.requester, locals.task.escrowAmount);

        // Slash agent bond with distribution
        locals.challengerReward = div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_CHALLENGER_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.burnAmount = div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_BURN_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.arbReward = div<sint64>(smul(locals.task.agentStake, static_cast<sint64>(QAGENT_SLASH_ARBITRATOR_BPS)), static_cast<sint64>(QAGENT_BPS_SCALE));
        locals.treasuryAmount = locals.task.agentStake - locals.challengerReward - locals.burnAmount - locals.arbReward;

        if (locals.challengerReward > 0)
            qpi.transfer(locals.task.challengerId, locals.challengerReward);
        if (locals.task.challengerStake > 0)
            qpi.transfer(locals.task.challengerId, locals.task.challengerStake);
        if (locals.burnAmount > 0)
        {
            qpi.burn(locals.burnAmount);
            state.mut().stats.totalBurned += locals.burnAmount;
        }
        // Distribute arbitrator reward directly to dispute voters (equal split)
        if (locals.arbReward > 0 && locals.task.disputeVoterCount > 0)
        {
            locals.perVoterReward = div<sint64>(locals.arbReward, static_cast<sint64>(locals.task.disputeVoterCount));
            if (locals.perVoterReward > 0)
            {
                if (locals.task.disputeVoters_0 != NULL_ID)
                    qpi.transfer(locals.task.disputeVoters_0, locals.perVoterReward);
                if (locals.task.disputeVoters_1 != NULL_ID)
                    qpi.transfer(locals.task.disputeVoters_1, locals.perVoterReward);
                if (locals.task.disputeVoters_2 != NULL_ID)
                    qpi.transfer(locals.task.disputeVoters_2, locals.perVoterReward);
                if (locals.task.disputeVoters_3 != NULL_ID)
                    qpi.transfer(locals.task.disputeVoters_3, locals.perVoterReward);
                if (locals.task.disputeVoters_4 != NULL_ID)
                    qpi.transfer(locals.task.disputeVoters_4, locals.perVoterReward);
            }
        }
        state.mut().stats.treasuryBalance += locals.treasuryAmount;

        if (state.get().agents.get(locals.task.assignedAgent, locals.agent))
        {
            locals.agent.failedTasks += 1;
            locals.agent.slashCount += 1;

            locals.agent.tier = QAGENT_TIER_UNRANKED;
            if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
            {
                locals.completionRate = div<sint64>(smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(QAGENT_BPS_SCALE)),
                    static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
                if (locals.agent.completedTasks >= 100 && locals.completionRate >= 9800 && locals.agent.uniqueRequesters >= 25)
                    locals.agent.tier = QAGENT_TIER_DIAMOND;
                else if (locals.agent.completedTasks >= 50 && locals.completionRate >= 9600 && locals.agent.uniqueRequesters >= 10)
                    locals.agent.tier = QAGENT_TIER_GOLD;
                else if (locals.agent.completedTasks >= 25 && locals.completionRate >= 9300 && locals.agent.uniqueRequesters >= 5)
                    locals.agent.tier = QAGENT_TIER_SILVER;
                else if (locals.agent.completedTasks >= 10 && locals.completionRate >= 9000)
                    locals.agent.tier = QAGENT_TIER_BRONZE;
            }
            state.mut().agents.set(locals.task.assignedAgent, locals.agent);
        }

        locals.arb.majorityVotes += 1;

        if (state.get().stats.activeTasks > 0)
            state.mut().stats.activeTasks -= 1;
        if (state.get().stats.disputedTasks > 0)
            state.mut().stats.disputedTasks -= 1;
    }

    state.mut().arbitrators.set(qpi.invocator(), locals.arb);
    // Only write task if not already committed by a resolution branch
    if (locals.resolved == 0)
        state.mut().tasks.set(input.taskId, locals.task);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logCastDisputeVote, input.taskId, qpi.invocator(), 0, input.vote };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(RegisterArbitrator)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (state.get().arbitrators.contains(qpi.invocator()))
    {
        output.returnCode = QAGENT_RC_ALREADY_EXISTS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (state.get().arbitrators.population() >= QAGENT_MAX_ARBITRATORS)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocationReward() < QAGENT_MIN_ARBITRATOR_STAKE)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.arb.stakeAmount = QAGENT_MIN_ARBITRATOR_STAKE;
    locals.arb.casesHandled = 0;
    locals.arb.majorityVotes = 0;
    locals.arb.registeredTick = qpi.tick();
    locals.arb.active = 1;
    state.mut().arbitrators.set(qpi.invocator(), locals.arb);

    locals.excess = qpi.invocationReward() - QAGENT_MIN_ARBITRATOR_STAKE;
    if (locals.excess > 0)
        qpi.transfer(qpi.invocator(), locals.excess);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logRegisterArbitrator, 0, qpi.invocator(), QAGENT_MIN_ARBITRATOR_STAKE, 0 };
    LOG_INFO(locals.log);

    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(DeactivateArbitrator)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().arbitrators.get(qpi.invocator(), locals.arb))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.arb.active == 0)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.arb.active = 0;
    state.mut().arbitrators.set(qpi.invocator(), locals.arb);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logDeactivateArbitrator, 0, qpi.invocator(), 0, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawArbitratorStake)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().arbitrators.get(qpi.invocator(), locals.arb))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Must be deactivated first (prevents withdrawal while actively judging disputes)
    if (locals.arb.active != 0)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.amount = locals.arb.stakeAmount;
    if (locals.amount <= 0)
    {
        output.returnCode = QAGENT_RC_INSUFFICIENT_FUNDS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.arb.stakeAmount = 0;
    state.mut().arbitrators.set(qpi.invocator(), locals.arb);

    qpi.transfer(qpi.invocator(), locals.amount);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logWithdrawArbitratorStake, 0, qpi.invocator(), locals.amount, 0 };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.withdrawn = locals.amount;
    output.returnCode = QAGENT_RC_SUCCESS;
}

// ---
// Governance Procedures
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(ProposeParameterChange)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Must be a staked agent
    if (!state.get().stakes.get(qpi.invocator(), locals.stake))
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.paramId >= QAGENT_MAX_GOVERNABLE_PARAMS)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Find a free proposal slot
    locals.freeSlot = 255;
    for (locals.i = 0; locals.i < QAGENT_MAX_PROPOSALS; locals.i++)
    {
        locals.prop = state.get().proposals.get(locals.i);
        if (locals.prop.active == 0)
        {
            locals.freeSlot = static_cast<uint8>(locals.i);
            break;
        }
    }
    if (locals.freeSlot == 255)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    state.mut().proposalCounter += 1;
    locals.prop.proposer = qpi.invocator();
    locals.prop.paramId = input.paramId;
    locals.prop.newValue = input.newValue;
    locals.prop.startTick = qpi.tick();
    locals.prop.endTick = qpi.tick() + QAGENT_GOVERNANCE_VOTE_TICKS;
    locals.prop.yesStakeWeight = 0;
    locals.prop.noStakeWeight = 0;
    locals.prop.executed = 0;
    locals.prop.active = 1;
    locals.prop.proposalId = state.get().proposalCounter;
    state.mut().proposals.set(locals.freeSlot, locals.prop);

    state.mut().stats.totalProposals += 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logProposeChange, 0, qpi.invocator(), input.newValue, input.paramId };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.proposalIndex = locals.freeSlot;
    output.returnCode = QAGENT_RC_SUCCESS;
}

PUBLIC_PROCEDURE_WITH_LOCALS(VoteOnProposal)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.proposalIndex >= QAGENT_MAX_PROPOSALS)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    locals.prop = state.get().proposals.get(input.proposalIndex);
    if (locals.prop.active == 0)
    {
        output.returnCode = QAGENT_RC_PROPOSAL_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.tick() > locals.prop.endTick)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.vote != 1 && input.vote != 2)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // Must be a staked agent — stake weight = total stake
    if (!state.get().stakes.get(qpi.invocator(), locals.stake))
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.weight = locals.stake.registrationStake + locals.stake.additionalStake;

    // Check for duplicate vote using K12-hashed composite key (safe across slot recycling)
    locals.voteKey.voterId = qpi.invocator();
    locals.voteKey.proposalId = locals.prop.proposalId;
    locals.voteKeyHash = qpi.K12(locals.voteKey);
    locals.alreadyVoted = 0;
    if (state.get().proposalVoters.get(locals.voteKeyHash, locals.alreadyVoted))
    {
        output.returnCode = QAGENT_RC_ALREADY_EXISTS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    if (input.vote == 1)
        locals.prop.yesStakeWeight += locals.weight;
    else
        locals.prop.noStakeWeight += locals.weight;

    state.mut().proposals.set(input.proposalIndex, locals.prop);

    // Mark proposal as voted by this agent
    state.mut().proposalVoters.set(locals.voteKeyHash, static_cast<uint8>(1));

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logVoteOnProposal, 0, qpi.invocator(), locals.weight, input.vote };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

// ---
// Rating Procedure
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(RateAgent)
{
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (!state.get().tasks.get(input.taskId, locals.task))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.requester != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.status != QAGENT_TASK_STATUS_COMPLETED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.rating < 1 || input.rating > 5)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.task.rated != 0)
    {
        output.returnCode = QAGENT_RC_ALREADY_EXISTS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    locals.task.rated = 1;
    state.mut().tasks.set(input.taskId, locals.task);

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logRateAgent, input.taskId, qpi.invocator(), 0, input.rating };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    output.returnCode = QAGENT_RC_SUCCESS;
}

// ---
// Delegation Procedure (Phase 4b: Multi-Agent Orchestration)
// ---

PUBLIC_PROCEDURE_WITH_LOCALS(DelegateTask)
{
    if (state.get().config.paused != 0)
    {
        output.returnCode = QAGENT_RC_PAUSED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (qpi.invocator() != qpi.originator())
    {
        output.returnCode = QAGENT_RC_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Only the assigned agent of an active task can delegate
    if (!state.get().tasks.get(input.parentTaskId, locals.parentTask))
    {
        output.returnCode = QAGENT_RC_TASK_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.parentTask.assignedAgent != qpi.invocator())
    {
        output.returnCode = QAGENT_RC_DELEGATION_UNAUTHORIZED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.parentTask.status != QAGENT_TASK_STATUS_ACCEPTED
        && locals.parentTask.status != QAGENT_TASK_STATUS_COMMITTED)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Check delegation depth (walk parent chain)
    locals.depth = 0;
    locals.currentTaskId = locals.parentTask.parentTaskId;
    while (locals.currentTaskId != 0 && locals.depth < QAGENT_MAX_DELEGATION_DEPTH + 1)
    {
        if (!state.get().tasks.get(locals.currentTaskId, locals.tempTask))
            break;
        locals.depth += 1;
        locals.currentTaskId = locals.tempTask.parentTaskId;
    }
    if (locals.depth >= QAGENT_MAX_DELEGATION_DEPTH)
    {
        output.returnCode = QAGENT_RC_DELEGATION_DEPTH_EXCEEDED;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Anti-self-dealing
    if (qpi.invocator() == input.agentId)
    {
        output.returnCode = QAGENT_RC_SELF_DEALING;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Target agent must exist and be active
    if (!state.get().agents.get(input.agentId, locals.childAgent))
    {
        output.returnCode = QAGENT_RC_AGENT_NOT_FOUND;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (locals.childAgent.status != QAGENT_STATUS_ACTIVE)
    {
        output.returnCode = QAGENT_RC_WRONG_STATUS;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Delegated escrow must be within parent's remaining escrow
    if (input.delegatedEscrow <= 0 || input.delegatedEscrow > locals.parentTask.escrowAmount)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Validate capacity and input BEFORE mutating parent task state
    if (state.get().tasks.population() >= QAGENT_MAX_TASKS)
    {
        output.returnCode = QAGENT_RC_CAPACITY_FULL;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    if (input.taskType > QAGENT_TASK_TYPE_DETERMINISTIC_INFERENCE)
    {
        output.returnCode = QAGENT_RC_INVALID_INPUT;
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }
    // Deduct delegated escrow from parent task (all validation passed)
    locals.parentTask.escrowAmount -= input.delegatedEscrow;
    state.mut().tasks.set(input.parentTaskId, locals.parentTask);

    // Create child task
    state.mut().taskCounter += 1;

    locals.childTask.taskId = state.get().taskCounter;
    // Parent agent is the requester of the child task. Note: delegated tasks intentionally
    // do NOT count toward the child agent's uniqueRequesters (anti-Sybil). Only direct
    // CreateTask calls track unique requesters to prevent reputation inflation via delegation.
    locals.childTask.requester = qpi.invocator();
    locals.childTask.assignedAgent = input.agentId;
    locals.childTask.taskType = input.taskType;
    locals.childTask.status = QAGENT_TASK_STATUS_OPEN;
    locals.childTask.escrowAmount = input.delegatedEscrow;
    locals.childTask.agentStake = 0;
    locals.childTask.challengerStake = 0;
    locals.childTask.challengerId = NULL_ID;
    locals.childTask.commitHash = NULL_ID;
    locals.childTask.resultHash = NULL_ID;
    locals.childTask.agentSalt = NULL_ID;
    locals.childTask.specHash = input.specHash;
    locals.childTask.modelHash = input.modelHash;
    locals.childTask.dataCid = NULL_ID;
    locals.childTask.resultEnum = 0;
    locals.childTask.totalMilestones = 1;
    locals.childTask.completedMilestones = 0;
    locals.childTask.createdTick = qpi.tick();
    locals.childTask.deadlineTick = (input.deadlineTicks > 0)
        ? qpi.tick() + input.deadlineTicks
        : locals.parentTask.deadlineTick; // Inherit parent deadline if not specified
    locals.childTask.submitTick = 0;
    locals.childTask.challengeDeadlineTick = 0;
    locals.childTask.disputeDeadlineTick = 0;
    locals.childTask.finalizedTick = 0;
    locals.childTask.disputeVotesValid = 0;
    locals.childTask.disputeVotesInvalid = 0;
    locals.childTask.verdict = QAGENT_VERDICT_NONE;
    locals.childTask.disputeVoters_0 = NULL_ID;
    locals.childTask.disputeVoters_1 = NULL_ID;
    locals.childTask.disputeVoters_2 = NULL_ID;
    locals.childTask.disputeVoters_3 = NULL_ID;
    locals.childTask.disputeVoters_4 = NULL_ID;
    locals.childTask.disputeVoterCount = 0;
    locals.childTask.rated = 0;
    locals.childTask.oracleQueryId = 0;
    locals.childTask.oracleRequested = 0;
    locals.childTask.inferenceConfig = 0;
    locals.childTask.parentTaskId = input.parentTaskId;

    state.mut().tasks.set(locals.childTask.taskId, locals.childTask);

    state.mut().stats.totalTasks += 1;
    state.mut().stats.activeTasks += 1;

    locals.log = QAGENTLogger{ CONTRACT_INDEX, logDelegateTask, locals.childTask.taskId, qpi.invocator(), input.delegatedEscrow, static_cast<uint8>(locals.depth + 1) };
    LOG_INFO(locals.log);

    qpi.transfer(qpi.invocator(), qpi.invocationReward());

    output.returnCode = QAGENT_RC_SUCCESS;
    output.childTaskId = locals.childTask.taskId;
}

// ---
// Query Functions
// ---

PUBLIC_FUNCTION(GetAgent)
{
    output.found = 0;
    if (state.get().agents.get(input.agentId, output.agent))
        output.found = 1;
}

PUBLIC_FUNCTION(GetTask)
{
    output.found = 0;
    if (state.get().tasks.get(input.taskId, output.task))
        output.found = 1;
}

PUBLIC_FUNCTION(GetService)
{
    output.found = 0;
    if (state.get().services.get(input.serviceId, output.service))
        output.found = 1;
}

PUBLIC_FUNCTION(GetPlatformStats)
{
    output.stats = state.get().stats;
    output.config = state.get().config;
}

PUBLIC_FUNCTION(GetStake)
{
    output.found = 0;
    if (state.get().stakes.get(input.agentId, output.stake))
        output.found = 1;
}

PUBLIC_FUNCTION(GetArbitrator)
{
    output.found = 0;
    if (state.get().arbitrators.get(input.arbitratorId, output.arbitrator))
        output.found = 1;
}

PUBLIC_FUNCTION_WITH_LOCALS(GetAgentsByCapability)
{
    output.count = 0;
    locals.elemIdx = state.get().agents.nextElementIndex(NULL_INDEX);
    while (locals.elemIdx != NULL_INDEX)
    {
        locals.agent = state.get().agents.value(locals.elemIdx);
        if ((locals.agent.capabilityBitmap & input.capabilityBitmap) == input.capabilityBitmap
            && locals.agent.status == QAGENT_STATUS_ACTIVE)
        {
            output.count += 1;
        }
        locals.elemIdx = state.get().agents.nextElementIndex(locals.elemIdx);
    }
}

PUBLIC_FUNCTION(GetProposal)
{
    output.found = 0;
    if (input.proposalIndex < QAGENT_MAX_PROPOSALS)
    {
        output.proposal = state.get().proposals.get(input.proposalIndex);
        if (output.proposal.active != 0 || output.proposal.executed != 0)
            output.found = 1;
    }
}

PUBLIC_FUNCTION_WITH_LOCALS(GetAgentReputationScore)
{
    output.found = 0;
    output.score = 0;
    if (!state.get().agents.get(input.agentId, locals.agent))
        return;

    output.found = 1;

    // Completion score: 40% weight (completedTasks / (completed + failed))
    locals.completionScore = 0;
    if (locals.agent.completedTasks + locals.agent.failedTasks > 0)
    {
        locals.completionScore = div<sint64>(
            smul(static_cast<sint64>(locals.agent.completedTasks), static_cast<sint64>(4000)),
            static_cast<sint64>(locals.agent.completedTasks + locals.agent.failedTasks));
    }

    // Oracle score: 30% weight (oraclePassedTasks / oracleVerifiedTasks)
    locals.oracleScore = 0;
    if (locals.agent.oracleVerifiedTasks > 0)
    {
        locals.oracleScore = div<sint64>(
            smul(static_cast<sint64>(locals.agent.oraclePassedTasks), static_cast<sint64>(3000)),
            static_cast<sint64>(locals.agent.oracleVerifiedTasks));
    }
    else if (locals.agent.completedTasks > 0)
    {
        // No oracle verifications yet — assume neutral (half credit)
        locals.oracleScore = 1500;
    }

    // Unique requesters score: 20% weight (capped at 50 requesters = max)
    locals.requesterScore = static_cast<sint64>(locals.agent.uniqueRequesters);
    if (locals.requesterScore > 50) locals.requesterScore = 50;
    locals.requesterScore = div<sint64>(smul(locals.requesterScore, static_cast<sint64>(2000)), static_cast<sint64>(50));

    // Tenure score: 10% weight (capped at 20 epochs = max)
    locals.tenureScore = 0;
    if (qpi.epoch() > locals.agent.registeredEpoch)
    {
        locals.tenureScore = static_cast<sint64>(qpi.epoch() - locals.agent.registeredEpoch);
        if (locals.tenureScore > 20) locals.tenureScore = 20;
        locals.tenureScore = div<sint64>(smul(locals.tenureScore, static_cast<sint64>(1000)), static_cast<sint64>(20));
    }

    output.score = static_cast<uint32>(locals.completionScore + locals.oracleScore + locals.requesterScore + locals.tenureScore);
    if (output.score > 10000) output.score = 10000;
}

};
