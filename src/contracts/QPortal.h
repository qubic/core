using namespace QPI;
#include "qpi.h"

constexpr uint64 QPORTAL_PORTAL_ASSET_NAME = 83843471265616; //Portal toekn asset name
constexpr uint32 QPORTAL_REGISTER_AMOUNT = 5; //Amount of portal token to register
constexpr uint32 QPORTAL_MIN_RETAINED = 1; //Members must keep >=1 portal token
constexpr uint32 QPORTAL_MAX_MEMBER = 4096; //Maximum number of members in the portal DAO.
constexpr uint32 QPORTAL_MAX_PROPOSAL = 4096; //Maximum number of proposals in the portal DAO.
constexpr uint32 QPORTAL_MAX_PROPOSAL_USER = 2; //The maximum number of proposals a user can submit.
constexpr uint32 QPORTAL_MAX_PROPOSAL_EPOCH = 5; //The maximum number

constexpr uint32 QPORTAL_SUCCESS = 0;
constexpr uint32 QPORTAL_INSUFFICIENT_PORTAL = 1;
constexpr uint32 QPORTAL_INVALID_OFFSET_OR_LIMIT = 2;
constexpr uint32 QPORTAL_ALREADY_REGISTERED = 3;
constexpr uint32 QPORTAL_REACHED_FULL = 4;
constexpr uint32 QPORTAL_NOT_REGISTERED = 5;
constexpr uint32 QPORTAL_REACHED_PROPOSAL = 6;
constexpr uint32 QPORTAL_ALREADY_EXISTED_PROPOSAL = 7;

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

    // struct 

    struct StateData
    {
        id PORTAL_Issuer;
        HashMap<id, bit, QPORTAL_MAX_MEMBER> registers; //registered members in the portal DAO
        uint32 numberOfRegisters, numberOfProposalEpochs, numberOfProposals;
        HashMap<id, uint32, QPORTAL_MAX_MEMBER> userProposalStatus; // 0 = no proposal, 1 = submitted 1 proposal, 2 = submitted 2 proposals (max)
        HashMap<id, uint32, QPORTAL_MAX_PROPOSAL> submittedProposals; // 0 = voting, 1 = accepted, 2 = rejected
        
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

    struct registerInPortalDAO_input
    {
    };

    struct registerInPortalDAO_output
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

protected:

    struct getRegisters_locals
    {
        id user;
        sint32 index;
        uint32 i;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getRegisters)
    {
        if (input.limit > 10)
        {
            output.returnCode = QPORTAL_INVALID_OFFSET_OR_LIMIT;
            return ;
        }
        if (input.offset + input.limit > state.get().numberOfRegisters)
        {
            output.returnCode = QPORTAL_INVALID_OFFSET_OR_LIMIT;
            return ;
        }
        locals.index = state.get().registers.nextElementIndex(NULL_INDEX);
        while (locals.index != NULL_INDEX)
        {
            if (locals.i >= input.offset + input.limit) break;
            if (locals.i >= input.offset && locals.i < input.offset + input.limit)
            {
                locals.user = state.get().registers.key(locals.index);
                if (locals.i - input.offset == 0)
                {
                    output.register1 = locals.user;
                }
                else if (locals.i - input.offset == 1)
                {
                    output.register2 = locals.user;
                }
                else if (locals.i - input.offset == 2)
                {
                    output.register3 = locals.user;
                }
                else if (locals.i - input.offset == 3)
                {
                    output.register4 = locals.user;
                }
                else if (locals.i - input.offset == 4)
                {
                    output.register5 = locals.user;
                }
                else if (locals.i - input.offset == 5)
                {
                    output.register6 = locals.user;
                }
                else if (locals.i - input.offset == 6)
                {
                    output.register7 = locals.user;
                }
                else if (locals.i - input.offset == 7)
                {
                    output.register8 = locals.user;
                }
                else if (locals.i - input.offset == 8)
                {
                    output.register9 = locals.user;
                }
                else if (locals.i - input.offset == 9)
                {
                    output.register10 = locals.user;
                }
            }
            locals.i++;
            locals.index = state.get().registers.nextElementIndex(locals.index);
        }
        output.returnCode = QPORTAL_SUCCESS;

    }

    struct registerInPortalDAO_locals
    {
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(registerInPortalDAO)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

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

        if (qpi.numberOfPossessedShares(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < QPORTAL_REGISTER_AMOUNT + QPORTAL_MIN_RETAINED) // +1 to check if the user has enough shares to pay the fee
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if(qpi.transferShareOwnershipAndPossession(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), QPORTAL_REGISTER_AMOUNT, SELF) < 0)
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

    struct submitProposal_locals
    {
        sint32 user_index, proposal_index;
        uint32 user_status, proposal_status;
        Logger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitProposal)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (!state.get().registers.contains(qpi.invocator()))
        {
            output.returnCode = QPORTAL_NOT_REGISTERED;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_NOT_REGISTERED, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if ( state.get().numberOfProposals >= QPORTAL_MAX_PROPOSAL)
        {
            output.returnCode = QPORTAL_REACHED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_REACHED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if ( state.get().numberOfProposalEpochs >= QPORTAL_MAX_PROPOSAL_EPOCH)
        {
            output.returnCode = QPORTAL_REACHED_PROPOSAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_REACHED_PROPOSAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        locals.user_index = state.get().userProposalStatus.getElementIndex(qpi.invocator());

        if (locals.user_index != NULL_INDEX && (state.get().userProposalStatus.value(locals.user_index) >= QPORTAL_MAX_PROPOSAL_USER))
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

        state.mut().submittedProposals.set(input.proposalId, 0);
        
        locals.user_status = locals.user_index == NULL_INDEX ? 0 : state.get().userProposalStatus.value(locals.user_index);

        state.mut().userProposalStatus.set(qpi.invocator(), ++ locals.user_status);
        state.mut().numberOfProposals ++;
        state.mut().numberOfProposalEpochs ++;

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getRegisters, 1);

        REGISTER_USER_PROCEDURE(registerInPortalDAO, 1);
        REGISTER_USER_PROCEDURE(submitProposal, 2);
    }

    END_EPOCH()
    {
        state.mut().numberOfProposalEpochs = 0;
        state.mut().userProposalStatus.reset();
    }

    INITIALIZE()
    {
        state.mut().PORTAL_Issuer = ID(_I, _Q, _U, _G, _N, _V, _F, _D, _Q, _S, _L, _T, _X, _F, _J, _S, _I, _O, _P, _P, _N, _P, _Z, _I, _N, _S, _C, _D, _Q, _T, _J, _V, _J, _W, _G, _R, _P, _W, _R, _T, _F, _F, _X, _M, _X, _S, _J, _I, _A, _A, _S, _X, _O, _B, _F, _F);
        state.mut().numberOfRegisters = 0;
        state.mut().numberOfProposalEpochs = 0;
        state.mut().numberOfProposals = 0;
        state.mut().registers.reset();
        state.mut().userProposalStatus.reset();
        state.mut().submittedProposals.reset();
    }
};