using namespace QPI;
#include "qpi.h";

constexpr uint64 QPORTAL_PORTAL_ASSET_NAME = 83843471265616; //PORTAL asset name
constexpr uint64 QPORTAL_MIN_TOTAL_VOTED_PORTAL = 100000ull; //proposal must have at least 100K PORTAL to approve a proposal(yes + no).
constexpr uint32 QPORTAL_REGISTER_FEE = 5; //Fee for registering in the portal DAO
constexpr uint64 QPORTAL_REFUND_FEE = 5; //PORTAL fee charged on each refund
constexpr uint32 QPORTAL_MAX_MEMBER = 4096; //Maximum number of members in the portal DAO.
constexpr uint32 QPORTAL_MAX_PROPOSAL = 4096; //Maximum number of proposals in the portal DAO.
constexpr uint32 QPORTAL_MAX_PROPOSAL_USER = 2; //The maximum number of proposals a user can submit.
constexpr uint32 QPORTAL_MAX_PROPOSAL_EPOCH = 5; //The maximum number of proposals per epoch.
constexpr uint32 QPORTAL_EPOCH_PROPOSALS_CAPACITY = 8; //Backing capacity for currentEpochProposals (Array requires 2^N, must be >= QPORTAL_MAX_PROPOSAL_EPOCH).
constexpr uint32 QPORTAL_MAX_VOTE = 32768; //The maximum number of votes a user can make (voting in all proposals in all proposal epochs).
constexpr uint32 QPORTAL_EXECUTION_FEE = 20000;
constexpr uint32 QPORTAL_TOTAL_FEE = 4; //100%
constexpr uint32 QPORTAL_BURN_FEE = 1;  // 25%
constexpr uint32 QPORTAL_SHAREHOLDER_FEE = 3; //75%

constexpr uint32 QPORTAL_SUCCESS = 0;
constexpr uint32 QPORTAL_INSUFFICIENT_PORTAL = 1;
constexpr uint32 QPORTAL_INVALID_OFFSET_OR_LIMIT = 2;
constexpr uint32 QPORTAL_ALREADY_REGISTERED = 3;
constexpr uint32 QPORTAL_REACHED_FULL = 4;
constexpr uint32 QPORTAL_NOT_REGISTERED = 5;
constexpr uint32 QPORTAL_REACHED_PROPOSAL = 6;
constexpr uint32 QPORTAL_ALREADY_EXISTED_PROPOSAL = 7;
constexpr uint32 QPORTAL_NOT_EXISTED_PROPOSAL = 8;
constexpr uint32 QPORTAL_ALREADY_VOTED_PROPOSAL = 9;
constexpr uint32 QPORTAL_CLOSED_PROPOSAL = 10;
constexpr uint32 QPORTAL_INVALID_INPUT = 11;
constexpr uint32 QPORTAL_NOT_VOTED_PROPOSAL = 12;
constexpr uint32 QPORTAL_EXISTED_PROPOSAL = 13;
constexpr uint32 QPORTAL_EPOCH_FROZEN = 14; //Submissions and voting are frozen from Monday 00:00 UTC to Wednesday 12:00 UTC.
constexpr uint32 QPORTAL_INSUFFICIENT_EXECUTION_FEE = 15;

struct QPORTAL2
{
};

struct QPORTAL : public ContractBase
{
public:

    struct Logger
    {
        uint32 _contractIndex;
        uint32 _type;
        sint8 _terminator;
    };

    struct submitInfo
    {
        id userId;
        uint8 proposalStatus; // 0 = voting, 1 = accepted, 2 = rejected
    };

    struct voteKey
    {
        id userId;
        id proposalId;
    };

    struct voteRecord
    {
        bit vote;
        uint64 votingPortalAmount;
    };
    
    struct voteResult
    {
        uint32 yesVotes;
        uint32 noVotes;
        uint64 yesPortal;
        uint64 noPortal;
    };

    struct StateData
    {
        id PORTAL_Issuer;
        HashMap<id, bit, QPORTAL_MAX_MEMBER> registers; //registered members in the portal DAO
        uint32 numberOfRegisters, numberOfCurrentEpochProposals, numberOfProposals;
        HashMap<id, uint32, QPORTAL_MAX_MEMBER> userProposalStatus; // 0 = no proposal, 1 = submitted 1 proposal, 2 = submitted 2 proposals (max)
        Array<id, QPORTAL_EPOCH_PROPOSALS_CAPACITY> currentEpochProposals; // record of proposal IDs in the current proposal epoch
        HashMap<id, submitInfo, QPORTAL_MAX_PROPOSAL> submittedProposals; // 0 = voting, 1 = accepted, 2 = rejected
        HashMap<id, uint64, QPORTAL_MAX_MEMBER> lockedAmount;
        HashMap<id, voteRecord, QPORTAL_MAX_VOTE> proposalVotes; // Voting records for each user per proposal epoch, used to check whether the user voted in the current proposal epoch.
        HashMap<id, voteResult, QPORTAL_MAX_PROPOSAL> proposalResults; // record of vote results for all of proposals
        uint64 epochRevenue;
        uint64 portalEpochRevenue;
    };

    struct getRegisters_input
    {
        uint32 offset;
        uint32 limit;
    };

    struct getRegisters_output
    {
        id register1, register2, register3, register4, register5, register6, register7, register8, register9, register10;
        sint32 returnCode;
    };

    struct getCurrentNumberOfRegisters_input
    {
    };

    struct getCurrentNumberOfRegisters_output
    {
        uint32 numberOfRegisters;
        sint32 returnCode;
    };

    struct getCurrentEpochProposals_input
    {
    };

    struct getCurrentEpochProposals_output
    {
        id proposal1, proposal2, proposal3, proposal4, proposal5;
        sint32 returnCode;
    };

    struct getProposalStatus_input
    {
        id proposalId;
    };

    struct getProposalStatus_output
    {
        uint32 status;
        sint32 returnCode;
    };

    struct getUserLockedAmount_input
    {
        id userId;
    };

    struct getUserLockedAmount_output
    {
        uint64 lockedAmount;
        sint32 returnCode;
    };

    struct getProposalInfo_input
    {
        id proposalId;
    };

    struct getProposalInfo_output
    {
        id proposer;
        uint8 status;
        uint32 yesVotes;
        uint32 noVotes;
        uint64 yesPortal;
        uint64 noPortal;
        sint32 returnCode;
    };

    struct registerInPortalDAO_input
    {
    };

    struct registerInPortalDAO_output
    {
        sint32 returnCode;
    };

    struct logoutInPortalDAO_input
    {
    };

    struct logoutInPortalDAO_output
    {
        sint32 returnCode;
    };

    struct submitProposal_input
    {
        id proposalId;
    };

    struct submitProposal_output
    {
        sint32 returnCode;
    };

    struct submitInVote_input
    {
        id proposalId;
        bit vote; // 0 = no, 1 = yes
        uint64 votingPortalAmount;
    };

    struct submitInVote_output
    {
        sint32 returnCode;
    };

    struct submitOutVote_input
    {
        id proposalId;
    };

    struct submitOutVote_output
    {
        sint32 returnCode;
    };

    struct requestRefund_input
    {
        uint64 amount;
    };

    struct requestRefund_output
    {
        sint32 returnCode;
    };

    struct transferShareManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
        uint16 newManagementContractIndex;
    };

    struct transferShareManagementRights_output
    {
        sint64 transferredNumberOfShares;
        sint32 returnCode;
    };

protected:

    struct getRegisters_locals
    {
        id user;
        sint64 index;
        uint32 i, cnt;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getRegisters)
    {
        if (input.limit == 0 || input.limit > 10)
        {
            output.returnCode = QPORTAL_INVALID_OFFSET_OR_LIMIT;
            return ;
        }

        if (input.offset >= state.get().numberOfRegisters)
        {
            output.returnCode = QPORTAL_INVALID_OFFSET_OR_LIMIT;
            return ;
        }

        locals.index = state.get().registers.nextElementIndex(NULL_INDEX);
        locals.cnt = input.offset + input.limit > state.get().numberOfRegisters ? state.get().numberOfRegisters - input.offset : input.limit;
        for (locals.i = 0; locals.i < input.offset; locals.i ++)
        {
            locals.index = state.get().registers.nextElementIndex(locals.index);
        }
        for (locals.i = 0; locals.i < locals.cnt && locals.index != NULL_INDEX; locals.i++)
        {
            locals.user = state.get().registers.key(locals.index);
            switch (locals.i)
            {
                case 0: output.register1 = locals.user; break;
                case 1: output.register2 = locals.user; break;
                case 2: output.register3 = locals.user; break;
                case 3: output.register4 = locals.user; break;
                case 4: output.register5 = locals.user; break;
                case 5: output.register6 = locals.user; break;
                case 6: output.register7 = locals.user; break;
                case 7: output.register8 = locals.user; break;
                case 8: output.register9 = locals.user; break;
                case 9: output.register10 = locals.user; break;
            }
            locals.index = state.get().registers.nextElementIndex(locals.index);
        }

        output.returnCode = QPORTAL_SUCCESS;
    }

    PUBLIC_FUNCTION(getCurrentNumberOfRegisters)
    {
        output.numberOfRegisters = state.get().numberOfRegisters;
        output.returnCode = QPORTAL_SUCCESS;
    }

    PUBLIC_FUNCTION(getCurrentEpochProposals)
    {
        output.proposal1 = state.get().currentEpochProposals.get(0);
        output.proposal2 = state.get().currentEpochProposals.get(1);
        output.proposal3 = state.get().currentEpochProposals.get(2);
        output.proposal4 = state.get().currentEpochProposals.get(3);
        output.proposal5 = state.get().currentEpochProposals.get(4);

        output.returnCode = QPORTAL_SUCCESS;
    }

    PUBLIC_FUNCTION(getProposalStatus)
    {
        if (input.proposalId == NULL_ID)
        {
            output.returnCode = QPORTAL_INVALID_INPUT;
            return ;
        }

        if (!state.get().submittedProposals.contains(input.proposalId))
        {
            output.returnCode = QPORTAL_NOT_EXISTED_PROPOSAL;
            return ;
        }

        output.status = state.get().submittedProposals.value(state.get().submittedProposals.getElementIndex(input.proposalId)).proposalStatus;
        output.returnCode = QPORTAL_SUCCESS;
    }

    PUBLIC_FUNCTION(getUserLockedAmount)
    {
        if (!state.get().registers.contains(input.userId))
        {
            output.returnCode = QPORTAL_NOT_REGISTERED;
            return ;
        }

        output.lockedAmount = state.get().lockedAmount.contains(input.userId) ? state.get().lockedAmount.value(state.get().lockedAmount.getElementIndex(input.userId)) : 0;
        output.returnCode = QPORTAL_SUCCESS;
    }

    struct getProposalInfo_locals
    {
        submitInfo si;
        voteResult vr;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getProposalInfo)
    {
        if (!state.get().submittedProposals.contains(input.proposalId))
        {
            output.returnCode = QPORTAL_NOT_EXISTED_PROPOSAL;
            return ;
        }

        locals.si = state.get().submittedProposals.value(state.get().submittedProposals.getElementIndex(input.proposalId));
        output.proposer = locals.si.userId;
        output.status = locals.si.proposalStatus;

        if (state.get().proposalResults.contains(input.proposalId))
        {
            locals.vr = state.get().proposalResults.value(state.get().proposalResults.getElementIndex(input.proposalId));
            output.yesVotes = locals.vr.yesVotes;
            output.noVotes = locals.vr.noVotes;
            output.yesPortal = locals.vr.yesPortal;
            output.noPortal = locals.vr.noPortal;
        }

        output.returnCode = QPORTAL_SUCCESS;
    }

    struct registerInPortalDAO_locals
    { 
        sint64 ownedShares;
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(registerInPortalDAO)
    {
        if (qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_EXECUTION_FEE, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return;
        }

        if (qpi.invocationReward() > QPORTAL_EXECUTION_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPORTAL_EXECUTION_FEE);
        }

        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        if (state.get().registers.contains(qpi.invocator()))
        {
            output.returnCode = QPORTAL_ALREADY_REGISTERED;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_ALREADY_REGISTERED, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (state.get().numberOfRegisters >= QPORTAL_MAX_MEMBER)
        {
            output.returnCode = QPORTAL_REACHED_FULL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_REACHED_FULL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        locals.ownedShares = qpi.numberOfPossessedShares(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
        if (locals.ownedShares < 0 || (uint64)locals.ownedShares < QPORTAL_REGISTER_FEE)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if(qpi.transferShareOwnershipAndPossession(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), QPORTAL_REGISTER_FEE, SELF) < 0)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        state.mut().registers.set(qpi.invocator(), 1);
        state.mut().numberOfRegisters ++;

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct logoutInPortalDAO_locals
    {
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(logoutInPortalDAO)
    {
        if(qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return;
        }

        if (qpi.invocationReward() > QPORTAL_EXECUTION_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPORTAL_EXECUTION_FEE);
        }

        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        if (!state.get().registers.contains(qpi.invocator()))
        {
            output.returnCode = QPORTAL_NOT_REGISTERED;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_REGISTERED, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        state.mut().registers.removeByKey(qpi.invocator());
        state.mut().registers.cleanupIfNeeded();
        state.mut().numberOfRegisters --;
        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct submitProposal_locals
    {
        sint64 index;
        uint32 status;
        uint8 dow;
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitProposal)
    {
        if (qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_EXECUTION_FEE, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return;
        }

        if (qpi.invocationReward() > QPORTAL_EXECUTION_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPORTAL_EXECUTION_FEE);
        }

        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        {
            locals.dow = qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day());
            if (locals.dow == 5 || locals.dow == 6 || (locals.dow == 0 && qpi.hour() < 12))
            {
                output.returnCode = QPORTAL_EPOCH_FROZEN;
                locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_EPOCH_FROZEN, 0 };
                LOG_INFO(locals.log);
                return ;
            }
        }

        if (!state.get().registers.contains(qpi.invocator()))
        {
            output.returnCode = QPORTAL_NOT_REGISTERED;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_REGISTERED, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (input.proposalId == NULL_ID)
        {
            output.returnCode = QPORTAL_INVALID_INPUT;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INVALID_INPUT, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (state.get().numberOfProposals >= QPORTAL_MAX_PROPOSAL)
        {
            output.returnCode = QPORTAL_REACHED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_REACHED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (state.get().numberOfCurrentEpochProposals >= QPORTAL_MAX_PROPOSAL_EPOCH)
        {
            output.returnCode = QPORTAL_REACHED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_REACHED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        locals.index = state.get().userProposalStatus.getElementIndex(qpi.invocator());

        if (locals.index != NULL_INDEX && (state.get().userProposalStatus.value(locals.index) >= QPORTAL_MAX_PROPOSAL_USER))
        {
            output.returnCode = QPORTAL_REACHED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_REACHED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (state.get().submittedProposals.contains(input.proposalId))
        {
            output.returnCode = QPORTAL_ALREADY_EXISTED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_ALREADY_EXISTED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        state.mut().submittedProposals.set(input.proposalId, {qpi.invocator(), 0}); // record proposal info with voting status = 0 (voting)
        state.mut().currentEpochProposals.set(state.get().numberOfCurrentEpochProposals, input.proposalId); // add proposal to the current proposal epoch
        
        locals.status = locals.index == NULL_INDEX ? 0 : state.get().userProposalStatus.value(locals.index);

        state.mut().userProposalStatus.set(qpi.invocator(), ++ locals.status);
        state.mut().numberOfProposals ++;
        state.mut().numberOfCurrentEpochProposals ++;

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

     struct submitInVote_locals
     {
        id key;
        uint8 dow;
        sint64 index;
        uint64 amount;
        sint64 ownedShares;
        voteKey vk;
        voteResult voteRes;
        Logger log;
     };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitInVote)
    {

        if (qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_EXECUTION_FEE, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return;
        }

        if (qpi.invocationReward() > QPORTAL_EXECUTION_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPORTAL_EXECUTION_FEE);
        }

        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        {
            locals.dow = qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day());
            if (locals.dow == 5 || locals.dow == 6 || (locals.dow == 0 && qpi.hour() < 12))
            {
                output.returnCode = QPORTAL_EPOCH_FROZEN;
                locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_EPOCH_FROZEN, 0 };
                LOG_INFO(locals.log);
                return ;
            }
        }

        if (input.proposalId == NULL_ID || input.votingPortalAmount <= 0)
        {
            output.returnCode = QPORTAL_INVALID_INPUT;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INVALID_INPUT, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (!state.get().registers.contains(qpi.invocator()))
        {
            output.returnCode = QPORTAL_NOT_REGISTERED;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_REGISTERED, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        locals.ownedShares = qpi.numberOfPossessedShares(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
        if (locals.ownedShares < 0 || (uint64)locals.ownedShares < input.votingPortalAmount)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (!state.get().submittedProposals.contains(input.proposalId))
        {
            output.returnCode = QPORTAL_NOT_EXISTED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_EXISTED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return;
        }

        locals.index = state.get().submittedProposals.getElementIndex(input.proposalId);
        if (state.get().submittedProposals.value(locals.index).proposalStatus != 0)
        {
            output.returnCode = QPORTAL_CLOSED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_CLOSED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return;
        }

        locals.vk.userId = qpi.invocator();
        locals.vk.proposalId = input.proposalId;
        locals.key = qpi.K12(locals.vk);

        if (state.get().proposalVotes.getElementIndex(locals.key) != NULL_INDEX)
        {
            output.returnCode = QPORTAL_ALREADY_VOTED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_ALREADY_VOTED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (qpi.transferShareOwnershipAndPossession(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), input.votingPortalAmount, SELF) < 0)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (state.get().lockedAmount.contains(qpi.invocator()))
        {
            locals.amount = state.get().lockedAmount.value(state.get().lockedAmount.getElementIndex(qpi.invocator()));
            state.mut().lockedAmount.set(qpi.invocator(), locals.amount + input.votingPortalAmount);
        }
        else
        {
            state.mut().lockedAmount.set(qpi.invocator(), input.votingPortalAmount);
        }

        state.mut().proposalVotes.set(locals.key, {input.vote, input.votingPortalAmount}); // record user's vote and voting portal amount for the proposal

        locals.index = state.get().proposalResults.getElementIndex(input.proposalId);
        if (locals.index == NULL_INDEX)
        {
            if (input.vote == 1)
            {
                state.mut().proposalResults.set(input.proposalId, { 1, 0, input.votingPortalAmount, 0 });
            }
            else
            {
                state.mut().proposalResults.set(input.proposalId, { 0, 1, 0, input.votingPortalAmount });
            }
        }
        else
        {
            locals.voteRes = state.get().proposalResults.value(locals.index);
            state.mut().proposalResults.set(input.proposalId, { locals.voteRes.yesVotes + (input.vote == 1 ? 1 : 0), locals.voteRes.noVotes + (input.vote == 0 ? 1 : 0), locals.voteRes.yesPortal + (input.vote == 1 ? input.votingPortalAmount : 0), locals.voteRes.noPortal + (input.vote == 0 ? input.votingPortalAmount : 0) });
        }

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct submitOutVote_locals
    {
        uint8 dow;
        sint64 index;
        voteRecord vr;
        voteResult tmp;
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitOutVote)
    {
        if (qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_EXECUTION_FEE, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return;
        }

        if (qpi.invocationReward() > QPORTAL_EXECUTION_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPORTAL_EXECUTION_FEE);
        }

        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        {
            locals.dow = qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day());
            if (locals.dow == 5 || locals.dow == 6 || (locals.dow == 0 && qpi.hour() < 12))
            {
                output.returnCode = QPORTAL_EPOCH_FROZEN;
                locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_EPOCH_FROZEN, 0 };
                LOG_INFO(locals.log);
                return ;
            }
        }

        if (input.proposalId == NULL_ID)
        {
            output.returnCode = QPORTAL_INVALID_INPUT;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INVALID_INPUT, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (!state.get().registers.contains(qpi.invocator()))
        {
            output.returnCode = QPORTAL_NOT_REGISTERED;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_REGISTERED, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (!state.get().submittedProposals.contains(input.proposalId))
        {
            output.returnCode = QPORTAL_NOT_EXISTED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_EXISTED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return;
        }

        if (state.get().submittedProposals.value(state.get().submittedProposals.getElementIndex(input.proposalId)).proposalStatus != 0)
        {
            output.returnCode = QPORTAL_CLOSED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_CLOSED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        locals.index = state.get().proposalVotes.getElementIndex(qpi.K12(voteKey{ qpi.invocator(), input.proposalId }));
        if (locals.index == NULL_INDEX)
        {
            output.returnCode = QPORTAL_NOT_VOTED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_VOTED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        
        locals.vr = state.get().proposalVotes.value(locals.index);
        locals.tmp = state.get().proposalResults.value(state.get().proposalResults.getElementIndex(input.proposalId));
        state.mut().proposalResults.set(input.proposalId, { 
            locals.tmp.yesVotes - (locals.vr.vote == 1 ? 1 : 0), 
            locals.tmp.noVotes - (locals.vr.vote == 0 ? 1 : 0), 
            locals.tmp.yesPortal - (locals.vr.vote == 1 ? locals.vr.votingPortalAmount : 0), 
            locals.tmp.noPortal - (locals.vr.vote == 0 ? locals.vr.votingPortalAmount : 0) 
        });

        state.mut().proposalVotes.removeByIndex(locals.index);
        state.mut().proposalVotes.cleanupIfNeeded();

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct requestRefund_locals
    {
        uint32 i;
        Logger log;
        sint64 remaining;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(requestRefund)
    {
        if (qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_EXECUTION_FEE, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return;
        }

        if (qpi.invocationReward() > QPORTAL_EXECUTION_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPORTAL_EXECUTION_FEE); 
        }

        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        if (input.amount <= 0)
        {
            output.returnCode = QPORTAL_INVALID_INPUT;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INVALID_INPUT, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        for (; locals.i < state.get().numberOfCurrentEpochProposals; locals.i ++)
        {
            if (state.get().proposalVotes.getElementIndex(qpi.K12(voteKey{ qpi.invocator(), state.get().currentEpochProposals.get(locals.i) })) != NULL_INDEX)
            {
                output.returnCode = QPORTAL_EXISTED_PROPOSAL;
                locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_EXISTED_PROPOSAL, 0 };
                LOG_INFO(locals.log);
                return ;
            }
        }

        if (!state.get().lockedAmount.contains(qpi.invocator()) || state.get().lockedAmount.value(state.get().lockedAmount.getElementIndex(qpi.invocator())) < (input.amount + QPORTAL_REFUND_FEE))
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if(qpi.transferShareOwnershipAndPossession(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, SELF, SELF, input.amount, qpi.invocator()) < 0)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        state.mut().portalEpochRevenue += QPORTAL_REFUND_FEE;

        locals.remaining = state.get().lockedAmount.value(state.get().lockedAmount.getElementIndex(qpi.invocator())) - input.amount;
        if (locals.remaining > 0)
        {
            state.mut().lockedAmount.set(qpi.invocator(), locals.remaining);
        }
        else
        {
            state.mut().lockedAmount.removeByIndex(state.get().lockedAmount.getElementIndex(qpi.invocator()));
            state.mut().lockedAmount.cleanupIfNeeded();
        }

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    struct transferShareManagementRights_locals
    {
        sint64 result;
        sint64 transferredShares;
        sint64 offeredFee;
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(transferShareManagementRights)
    {
        if (qpi.invocationReward() < QPORTAL_EXECUTION_FEE)
        {
            state.mut().epochRevenue += qpi.invocationReward();
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_EXECUTION_FEE, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_EXECUTION_FEE;
            return ;
        }

        locals.offeredFee = qpi.invocationReward() - QPORTAL_EXECUTION_FEE;
        state.mut().epochRevenue += QPORTAL_EXECUTION_FEE;

        if (input.numberOfShares <= 0 || input.newManagementContractIndex == 0 || input.newManagementContractIndex == SELF_INDEX)
        {
            qpi.transfer(qpi.invocator(), locals.offeredFee);
            output.returnCode = QPORTAL_INVALID_INPUT;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INVALID_INPUT, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
        {
            qpi.transfer(qpi.invocator(), locals.offeredFee);
            output.transferredNumberOfShares = 0;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            return ;
        }

        locals.result = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagementContractIndex, input.newManagementContractIndex, locals.offeredFee);
        if (locals.result == INVALID_AMOUNT || locals.result < 0)
        {
            qpi.transfer(qpi.invocator(), locals.offeredFee);
            output.transferredNumberOfShares = 0;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            return ;
        }

        qpi.transfer(qpi.invocator(), locals.offeredFee - locals.result);

        output.transferredNumberOfShares = input.numberOfShares;
        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getRegisters, 1);
        REGISTER_USER_FUNCTION(getCurrentNumberOfRegisters, 2);
        REGISTER_USER_FUNCTION(getCurrentEpochProposals, 3);
        REGISTER_USER_FUNCTION(getProposalStatus, 4);
        REGISTER_USER_FUNCTION(getUserLockedAmount, 5);
        REGISTER_USER_FUNCTION(getProposalInfo, 6);

        REGISTER_USER_PROCEDURE(registerInPortalDAO, 1);
        REGISTER_USER_PROCEDURE(logoutInPortalDAO, 2);
        REGISTER_USER_PROCEDURE(submitProposal, 3);
        REGISTER_USER_PROCEDURE(submitInVote, 4);
        REGISTER_USER_PROCEDURE(submitOutVote, 5);
        REGISTER_USER_PROCEDURE(requestRefund, 6);
        REGISTER_USER_PROCEDURE(transferShareManagementRights, 7);
    }

    struct END_EPOCH_locals
    {
        sint64 i, index, sharesHeld;
        id proposalId;
        uint32 yesVotes, noVotes;
        uint64 yesPortal, noPortal;
        uint64 burnFee, shareholderFeePerShare, shareholderPortalFeePerShare;
        AssetPossessionIterator iter;
		Asset QPortalAsset;

    };

    END_EPOCH_WITH_LOCALS()
    {
        if (state.get().numberOfCurrentEpochProposals > 0)
        {
             
            for (; locals.i < state.get().numberOfCurrentEpochProposals; locals.i ++)
            {
                locals.proposalId = state.get().currentEpochProposals.get(locals.i);
    
                locals.index = state.get().proposalResults.getElementIndex(locals.proposalId);
                if (locals.index == NULL_INDEX)
                {
                    state.mut().submittedProposals.set(locals.proposalId, {state.get().submittedProposals.value(state.get().submittedProposals.getElementIndex(locals.proposalId)).userId, 2}); // if there is no vote for the proposal, set the proposal status to rejected (2)
                    continue;
                }
                locals.yesVotes = state.get().proposalResults.value(locals.index).yesVotes;
                locals.noVotes = state.get().proposalResults.value(locals.index).noVotes;
                locals.yesPortal = state.get().proposalResults.value(locals.index).yesPortal;
                locals.noPortal = state.get().proposalResults.value(locals.index).noPortal;
                
                if (locals.yesPortal + locals.noPortal >= QPORTAL_MIN_TOTAL_VOTED_PORTAL && locals.yesPortal > locals.noPortal && locals.yesVotes > locals.noVotes)
                {
                    state.mut().submittedProposals.set(locals.proposalId, {state.get().submittedProposals.value(state.get().submittedProposals.getElementIndex(locals.proposalId)).userId, 1});
                    continue;
                }
                else
                {
                    state.mut().submittedProposals.set(locals.proposalId, {state.get().submittedProposals.value(state.get().submittedProposals.getElementIndex(locals.proposalId)).userId, 2});
                }
            }

        }

        locals.shareholderFeePerShare = div<uint64>(div<uint64>(state.get().epochRevenue, QPORTAL_TOTAL_FEE) * QPORTAL_SHAREHOLDER_FEE, NUMBER_OF_COMPUTORS);
        if (locals.shareholderFeePerShare > 0)
        {
            qpi.distributeDividends(locals.shareholderFeePerShare);
        }

        locals.burnFee = state.get().epochRevenue - locals.shareholderFeePerShare * NUMBER_OF_COMPUTORS;
        if (locals.burnFee > 0)
        {
            qpi.burn(locals.burnFee);
        }

        locals.shareholderPortalFeePerShare = div<uint64>(state.get().portalEpochRevenue, NUMBER_OF_COMPUTORS);
        if (locals.shareholderPortalFeePerShare > 0)
        {
            locals.QPortalAsset.assetName = QPORTAL_PORTAL_ASSET_NAME;
            locals.QPortalAsset.issuer = state.get().PORTAL_Issuer;
            locals.iter.begin(locals.QPortalAsset);
            while (!locals.iter.reachedEnd())
            {
                locals.sharesHeld = locals.iter.numberOfPossessedShares();
                if (locals.sharesHeld > 0)
                {
                    qpi.transferShareOwnershipAndPossession(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, SELF, SELF, (sint64)(locals.shareholderPortalFeePerShare * (uint64)locals.sharesHeld), locals.iter.possessor());
                }
                locals.iter.next();
            }
            state.mut().portalEpochRevenue = state.get().portalEpochRevenue - locals.shareholderPortalFeePerShare * NUMBER_OF_COMPUTORS;
        }

        state.mut().epochRevenue = 0;
        state.mut().numberOfCurrentEpochProposals = 0;
        state.mut().userProposalStatus.reset();
        state.mut().proposalVotes.reset();
        state.mut().currentEpochProposals.setAll(NULL_ID);
        state.mut().lockedAmount.cleanupIfNeeded();
        state.mut().proposalResults.cleanupIfNeeded();
        state.mut().submittedProposals.cleanupIfNeeded();
        state.mut().registers.cleanupIfNeeded();
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer =
            (input.asset.assetName == QPORTAL_PORTAL_ASSET_NAME) &&
            (input.asset.issuer == state.get().PORTAL_Issuer);
    }

    INITIALIZE()
    {
        state.mut().PORTAL_Issuer = ID(_I, _Q, _U, _G, _N, _V, _F, _D, _Q, _S, _L, _T, _X, _F, _J, _S, _I, _O, _P, _P, _N, _P, _Z, _I, _N, _S, _C, _D, _Q, _T, _J, _V, _J, _W, _G, _R, _P, _W, _R, _T, _F, _F, _X, _M, _X, _S, _J, _I, _A, _A, _S, _X, _O, _B, _F, _F);
        state.mut().numberOfRegisters = 0;
        state.mut().numberOfCurrentEpochProposals = 0;
        state.mut().numberOfProposals = 0;
        state.mut().epochRevenue = 0;
        state.mut().lockedAmount.reset();
        state.mut().currentEpochProposals.setAll(NULL_ID);
        state.mut().proposalVotes.reset();
        state.mut().registers.reset();
        state.mut().userProposalStatus.reset();
        state.mut().submittedProposals.reset();
        state.mut().proposalResults.reset();
    }
};