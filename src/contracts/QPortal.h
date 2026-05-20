using namespace QPI;
#include "qpi.h"

constexpr uint64 QPORTAL_PORTAL_ASSET_NAME = 83843471265616; //Portal toekn asset name
constexpr uint32 QPORTAL_REGISTER_AMOUNT = 5; //Amount of portal token to register
constexpr uint32 QPORTAL_MAX_MEMBER = 4096; //Maximum number of members in the portal DAO

constexpr uint32 QPORTAL_SUCCESS = 0;
constexpr uint32 QPORTAL_INSUFFICIENT_PORTAL = 1;
constexpr uint32 QPORTAL_INVALID_OFFSET_OR_LIMIT = 2;

struct QPortal2
{
};

struct QPortal : public ContractBase
{
public:
    struct Logger
    {
        uint32 _contractIndex;
        uint32 _type;
        sint8 _terminator;
    };

    struct StateData
    {
        id PORTAL_Issuer;
        HashMap<id, bit, QPORTAL_MAX_MEMBER> registers;
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
    }

    struct getRegisters_locals
    {
        id user;
        sint32 index;
        uint32 i;
        Logger log;
    };

    struct registerInPortalDAO_input
    {
    }

    struct registerInPortalDAO_output
    {
        sint32 returnCode;
    }

    struct registerInPortalDAO_locals
    {
        Logger log;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getRegisters)
    {
        if (input.limit > 10)
        {
            output.returnCode = QPORTAL_INVALID_OFFSET_OR_LIMIT;
            return ;
        }
        if (input.offset + input.limit > state.get().registers.size())
        {
            output.returnCode = QPORTAL_INVALID_OFFSET_OR_LIMIT;
            return ;
        }
        locals.index = state.get().registers.nextElementIndex(NULL_INDEX);
        while (locals.index != NULL_INDEX)
        {
            locals.user = state.get().registers.key(locals.index);
            if (locals.i >= input.offset && locals.i < input.offset + input.limit)
            {
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
                if (locals.i >= input.offset + input.limit)
                {
                    break;
                }
                locals.i++;
                locals.index = state.get().registers.nextElementIndex(locals.index);
            }
        }
        output.returnCode = QPORTAL_SUCCESS;

    }

    PUBLIC_PROCEDURE_WITH_LOCALS(registerInPortalDAO)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (qpi.numberOfPossessedShares(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < QPORTAL_REGISTER_AMOUNT + 1)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }

        if(qpi.transferShareOwnershipAndPossession(QPORTAL_PORTAL_ASSET_NAME, state.get().PORTAL_Issuer, qpi.invocator(), qpi.invocator(), QPORTAL_REGISTER_AMOUNT, SELF_INDEX) < 0)
        {
            output.returnCode = QPORTAL_INSUFFICIENT_PORTAL;
            locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_INSUFFICIENT_PORTAL, 0 };
            LOG_INFO(locals.log);
            return ;
        }
        state.mut().registers.set(qpi.invocator(), 1);

        output.returnCode = QPORTAL_SUCCESS;
        locals.log = Logger{ QPORTAL_CONTRACT_INDEX, QPORTAL_SUCCESS, 0 };
        LOG_INFO(locals.log);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(registerInPortalDAO, 1);
    }

    INITIALIZE()
    {
        state.mut().PORTAL_Issuer = ID(_I, _Q, _U, _G, _N, _V, _F, _D, _Q, _S, _L, _T, _X, _F, _J, _S, _I, _O, _P, _P, _N, _P, _Z, _I, _N, _S, _C, _D, _Q, _T, _J, _V, _J, _W, _G, _R, _P, _W, _R, _T, _F, _F, _X, _M, _X, _S, _J, _I, _A, _A, _S, _X, _O, _B, _F, _F);
    }
};