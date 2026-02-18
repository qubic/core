using namespace QPI;

constexpr uint64 QVAULT_MAX_REINVEST_AMOUNT = 100000000000ULL;
constexpr uint64 QVAULT_QCAP_ASSETNAME = 1346454353;
constexpr uint64 QVAULT_QCAP_MAX_SUPPLY = 21000000;
constexpr uint32 QVAULT_MAX_NUMBER_OF_BANNED_ADDRESSES = 16;

struct QVAULT2
{
};

struct QVAULT : public ContractBase
{
    struct StateData
    {
        id QCAP_ISSUER;
        id authAddress1, authAddress2, authAddress3, newAuthAddress1, newAuthAddress2, newAuthAddress3;
        id reinvestingAddress, newReinvestingAddress1, newReinvestingAddress2, newReinvestingAddress3;
        id adminAddress, newAdminAddress1, newAdminAddress2, newAdminAddress3;
        id bannedAddress1, bannedAddress2, bannedAddress3;
        id unbannedAddress1, unbannedAddress2, unbannedAddress3;
        Array<id, QVAULT_MAX_NUMBER_OF_BANNED_ADDRESSES> bannedAddress;
        uint32 numberOfBannedAddress;
        uint32 shareholderDividend, QCAPHolderPermille, reinvestingPermille, devPermille, burnPermille;
        uint32 newQCAPHolderPermille1, newReinvestingPermille1, newDevPermille1;
        uint32 newQCAPHolderPermille2, newReinvestingPermille2, newDevPermille2;
        uint32 newQCAPHolderPermille3, newReinvestingPermille3, newDevPermille3;
    };

public:

    /*
        submitAuthAddress PROCEDURE
        multisig addresses can submit the new multisig address in this procedure.
    */

    struct submitAuthAddress_input
    {
        id newAddress;
    };

    struct submitAuthAddress_output
    {
    };

    /*
        changeAuthAddress PROCEDURE
        the new multisig address can be changed by multisig address in this procedure.
        the new multisig address should be submitted by the 2 multisig addresses in submitAuthAddress. if else, it will not be changed with new address
    */

    struct changeAuthAddress_input
    {
        uint32 numberOfChangedAddress;
    };

    struct changeAuthAddress_output
    {
    };

    /*
        submitDistributionPermille PROCEDURE
        the new distribution Permilles can be submitted by the multisig addresses in this procedure.
    */

    struct submitDistributionPermille_input
    {
        uint32 newQCAPHolderPermille;
        uint32 newReinvestingPermille;
        uint32 newDevPermille;
    };

    struct submitDistributionPermille_output
    {
    };

    /*
        changeDistributionPermille PROCEDURE
        the new distribution Permilles can be changed by multisig address in this procedure.
        the new distribution Permilles should be submitted by multsig addresses in submitDistributionPermille PROCEDURE. if else, it will not be changed with new Permilles.
    */

    struct changeDistributionPermille_input
    {
        uint32 newQCAPHolderPermille;
        uint32 newReinvestingPermille;
        uint32 newDevPermille;
    };

    struct changeDistributionPermille_output
    {
    };

    /*
        submitReinvestingAddress PROCEDURE
        the new reinvestingAddress can be submitted by the multisig addresses in this procedure.
    */

    struct submitReinvestingAddress_input
    {
        id newAddress;
    };

    struct submitReinvestingAddress_output
    {
    };

    /*
        changeReinvestingAddress PROCEDURE
        the new reinvesting address can be changed by multisig address in this procedure.
        the new reinvesting address should be submitted by multsig addresses in submitReinvestingAddress. if else, it will not be changed with new address.
    */

    struct changeReinvestingAddress_input
    {
        id newAddress;
    };

    struct changeReinvestingAddress_output
    {

    };

    struct getData_input
    {
    };

    struct getData_output
    {
        uint64 numberOfBannedAddress;
        uint32 shareholderDividend, QCAPHolderPermille, reinvestingPermille, devPermille;
        id authAddress1, authAddress2, authAddress3, reinvestingAddress, adminAddress;
        id newAuthAddress1, newAuthAddress2, newAuthAddress3;
        id newReinvestingAddress1, newReinvestingAddress2, newReinvestingAddress3;
        id newAdminAddress1, newAdminAddress2, newAdminAddress3;
        id bannedAddress1, bannedAddress2, bannedAddress3;
        id unbannedAddress1, unbannedAddress2, unbannedAddress3;
    };

    /*
        submitAdminAddress PROCEDURE
        the new adminAddress can be submitted by the multisig addresses in this procedure.
    */

    struct submitAdminAddress_input
    {
        id newAddress;
    };

    struct submitAdminAddress_output
    {
    };

    /*
        changeAdminAddress PROCEDURE
        the new admin address can be changed by multisig address in this procedure.
        the new admin address should be submitted by multsig addresses in submitAdminAddress PROCEDURE. if else, it will not be changed with new address.
    */

    struct changeAdminAddress_input
    {
        id newAddress;
    };

    struct changeAdminAddress_output
    {
    };

    /*
        submitBannedAddress PROCEDURE
        the banned addresses can be submitted by multisig address in this procedure.
    */
    struct submitBannedAddress_input
    {
        id bannedAddress;
    };

    struct submitBannedAddress_output
    {
    };

    /*
        saveBannedAddress PROCEDURE
        the banned address can be changed by multisig address in this procedure.
        the banned address should be submitted by multisig addresses in submitBannedAddress PROCEDURE.  if else, it will not be saved.
    */

    struct saveBannedAddress_input
    {
        id bannedAddress;
    };

    struct saveBannedAddress_output
    {
    };

    /*
        submitUnbannedAddress PROCEDURE
        the unbanned addresses can be submitted by multisig address in this procedure.
    */

    struct submitUnbannedAddress_input
    {
        id unbannedAddress;
    };

    struct submitUnbannedAddress_output
    {
    };

    /*
        unblockBannedAddress PROCEDURE
        the banned address can be unblocked by multisig address in this procedure.
        the unbanned address should be submitted by multisig addresses in submitUnbannedAddress PROCEDURE.  if else, it will not be unblocked.
    */

    struct unblockBannedAddress_input
    {
        id unbannedAddress;
    };

    struct unblockBannedAddress_output
    {
    };

protected:

    PUBLIC_PROCEDURE(submitAuthAddress)
    {
        if(qpi.invocator() == state.get().authAddress1)
        {
            state.mut().newAuthAddress1 = input.newAddress;
        }
        if(qpi.invocator() == state.get().authAddress2)
        {
            state.mut().newAuthAddress2 = input.newAddress;
        }
        if(qpi.invocator() == state.get().authAddress3)
        {
            state.mut().newAuthAddress3 = input.newAddress;
        }

    }

    struct changeAuthAddress_locals {
        bit succeed;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(changeAuthAddress)
    {
        if(qpi.invocator() != state.get().authAddress1 && qpi.invocator() != state.get().authAddress2 && qpi.invocator() != state.get().authAddress3)
        {
            return ;
        }

        locals.succeed = 0;

        if(qpi.invocator() != state.get().authAddress1 && input.numberOfChangedAddress == 1 && state.get().newAuthAddress2 != NULL_ID && state.get().newAuthAddress2 == state.get().newAuthAddress3)
        {
            state.mut().authAddress1 = state.get().newAuthAddress2;
            locals.succeed = 1;
        }

        if(qpi.invocator() != state.get().authAddress2 && input.numberOfChangedAddress == 2 && state.get().newAuthAddress1 != NULL_ID && state.get().newAuthAddress1 == state.get().newAuthAddress3)
        {
            state.mut().authAddress2 = state.get().newAuthAddress1;
            locals.succeed = 1;
        }

        if(qpi.invocator() != state.get().authAddress3 && input.numberOfChangedAddress == 3 && state.get().newAuthAddress1 != NULL_ID && state.get().newAuthAddress1 == state.get().newAuthAddress2)
        {
            state.mut().authAddress3 = state.get().newAuthAddress1;
            locals.succeed = 1;
        }

        if(locals.succeed == 1)
        {
            state.mut().newAuthAddress1 = NULL_ID;
            state.mut().newAuthAddress2 = NULL_ID;
            state.mut().newAuthAddress3 = NULL_ID;
        }
    }

    PUBLIC_PROCEDURE(submitDistributionPermille)
    {
        if(input.newDevPermille + input.newQCAPHolderPermille + input.newReinvestingPermille + state.get().shareholderDividend + state.get().burnPermille != 1000)
        {
            return ;
        }

        if(qpi.invocator() == state.get().authAddress1)
        {
            state.mut().newDevPermille1 = input.newDevPermille;
            state.mut().newQCAPHolderPermille1 = input.newQCAPHolderPermille;
            state.mut().newReinvestingPermille1 = input.newReinvestingPermille;
        }

        if(qpi.invocator() == state.get().authAddress2)
        {
            state.mut().newDevPermille2 = input.newDevPermille;
            state.mut().newQCAPHolderPermille2 = input.newQCAPHolderPermille;
            state.mut().newReinvestingPermille2 = input.newReinvestingPermille;
        }

        if(qpi.invocator() == state.get().authAddress3)
        {
            state.mut().newDevPermille3 = input.newDevPermille;
            state.mut().newQCAPHolderPermille3 = input.newQCAPHolderPermille;
            state.mut().newReinvestingPermille3 = input.newReinvestingPermille;
        }

    }

    PUBLIC_PROCEDURE(changeDistributionPermille)
    {
        if(qpi.invocator() != state.get().authAddress1 && qpi.invocator() != state.get().authAddress2 && qpi.invocator() != state.get().authAddress3)
        {
            return ;
        }

        if(input.newDevPermille + input.newQCAPHolderPermille + input.newReinvestingPermille + state.get().shareholderDividend + state.get().burnPermille != 1000)
        {
            return ;
        }

        if(input.newDevPermille == 0 || input.newDevPermille != state.get().newDevPermille1 || state.get().newDevPermille1 != state.get().newDevPermille2 || state.get().newDevPermille2 != state.get().newDevPermille3)
        {
            return ;
        }

        if(input.newQCAPHolderPermille == 0 || input.newQCAPHolderPermille != state.get().newQCAPHolderPermille1 || state.get().newQCAPHolderPermille1 != state.get().newQCAPHolderPermille2 || state.get().newQCAPHolderPermille2 != state.get().newQCAPHolderPermille3)
        {
            return ;
        }

        if(input.newReinvestingPermille == 0 || input.newReinvestingPermille != state.get().newReinvestingPermille1 || state.get().newReinvestingPermille1 != state.get().newReinvestingPermille2 || state.get().newReinvestingPermille2 != state.get().newReinvestingPermille3)
        {
            return ;
        }

        state.mut().devPermille = state.get().newDevPermille1;
        state.mut().QCAPHolderPermille = state.get().newQCAPHolderPermille1;
        state.mut().reinvestingPermille = state.get().newReinvestingPermille1;

        state.mut().newDevPermille1 = 0;
        state.mut().newDevPermille2 = 0;
        state.mut().newDevPermille3 = 0;

        state.mut().newQCAPHolderPermille1 = 0;
        state.mut().newQCAPHolderPermille2 = 0;
        state.mut().newQCAPHolderPermille3 = 0;

        state.mut().newReinvestingPermille1 = 0;
        state.mut().newReinvestingPermille2 = 0;
        state.mut().newReinvestingPermille3 = 0;
    }

    PUBLIC_PROCEDURE(submitReinvestingAddress)
    {
        if(qpi.invocator() == state.get().authAddress1)
        {
            state.mut().newReinvestingAddress1 = input.newAddress;
        }
        if(qpi.invocator() == state.get().authAddress2)
        {
            state.mut().newReinvestingAddress2 = input.newAddress;
        }
        if(qpi.invocator() == state.get().authAddress3)
        {
            state.mut().newReinvestingAddress3 = input.newAddress;
        }
    }

    PUBLIC_PROCEDURE(changeReinvestingAddress)
    {
        if(qpi.invocator() != state.get().authAddress1 && qpi.invocator() != state.get().authAddress2 && qpi.invocator() != state.get().authAddress3)
        {
            return ;
        }

        if(input.newAddress == NULL_ID || input.newAddress != state.get().newReinvestingAddress1 || state.get().newReinvestingAddress1 != state.get().newReinvestingAddress2  || state.get().newReinvestingAddress2 != state.get().newReinvestingAddress3)
        {
            return ;
        }

        state.mut().reinvestingAddress = state.get().newReinvestingAddress1;

        state.mut().newReinvestingAddress1 = NULL_ID;
        state.mut().newReinvestingAddress2 = NULL_ID;
        state.mut().newReinvestingAddress3 = NULL_ID;
    }

    PUBLIC_FUNCTION(getData)
    {
        output.authAddress1 = state.get().authAddress1;
        output.authAddress2 = state.get().authAddress2;
        output.authAddress3 = state.get().authAddress3;
        output.reinvestingAddress = state.get().reinvestingAddress;
        output.shareholderDividend = state.get().shareholderDividend;
        output.devPermille = state.get().devPermille;
        output.QCAPHolderPermille = state.get().QCAPHolderPermille;
        output.reinvestingPermille = state.get().reinvestingPermille;
        output.adminAddress = state.get().adminAddress;
        output.newAuthAddress1 = state.get().newAuthAddress1;
        output.newAuthAddress2 = state.get().newAuthAddress2;
        output.newAuthAddress3 = state.get().newAuthAddress3;
        output.newAdminAddress1 = state.get().newAdminAddress1;
        output.newAdminAddress2 = state.get().newAdminAddress2;
        output.newAdminAddress3 = state.get().newAdminAddress3;
        output.newReinvestingAddress1 = state.get().newReinvestingAddress1;
        output.newReinvestingAddress2 = state.get().newReinvestingAddress2;
        output.newReinvestingAddress3 = state.get().newReinvestingAddress3;
        output.numberOfBannedAddress = state.get().numberOfBannedAddress;
        output.bannedAddress1 = state.get().bannedAddress1;
        output.bannedAddress2 = state.get().bannedAddress2;
        output.bannedAddress3 = state.get().bannedAddress3;
        output.unbannedAddress1 = state.get().unbannedAddress1;
        output.unbannedAddress2 = state.get().unbannedAddress2;
        output.unbannedAddress3 = state.get().unbannedAddress3;

    }

    PUBLIC_PROCEDURE(submitAdminAddress)
    {
        if(qpi.invocator() == state.get().authAddress1)
        {
            state.mut().newAdminAddress1 = input.newAddress;
        }
        if(qpi.invocator() == state.get().authAddress2)
        {
            state.mut().newAdminAddress2 = input.newAddress;
        }
        if(qpi.invocator() == state.get().authAddress3)
        {
            state.mut().newAdminAddress3 = input.newAddress;
        }

    }

    PUBLIC_PROCEDURE(changeAdminAddress)
    {
        if(qpi.invocator() != state.get().authAddress1 && qpi.invocator() != state.get().authAddress2 && qpi.invocator() != state.get().authAddress3)
        {
            return ;
        }

        if(input.newAddress == NULL_ID || input.newAddress != state.get().newAdminAddress1 || state.get().newAdminAddress1 != state.get().newAdminAddress2  || state.get().newAdminAddress2 != state.get().newAdminAddress3)
        {
            return ;
        }

        state.mut().adminAddress = state.get().newAdminAddress1;

        state.mut().newAdminAddress1 = NULL_ID;
        state.mut().newAdminAddress2 = NULL_ID;
        state.mut().newAdminAddress3 = NULL_ID;
    }


    PUBLIC_PROCEDURE(submitBannedAddress)
    {
        if(qpi.invocator() == state.get().authAddress1)
        {
            state.mut().bannedAddress1 = input.bannedAddress;
        }
        if(qpi.invocator() == state.get().authAddress2)
        {
            state.mut().bannedAddress2 = input.bannedAddress;
        }
        if(qpi.invocator() == state.get().authAddress3)
        {
            state.mut().bannedAddress3 = input.bannedAddress;
        }

    }

    PUBLIC_PROCEDURE(saveBannedAddress)
    {
        if(state.get().numberOfBannedAddress >= QVAULT_MAX_NUMBER_OF_BANNED_ADDRESSES)
        {
            return ;
        }

        if(qpi.invocator() != state.get().authAddress1 && qpi.invocator() != state.get().authAddress2 && qpi.invocator() != state.get().authAddress3)
        {
            return ;
        }

        if(input.bannedAddress == NULL_ID || input.bannedAddress != state.get().bannedAddress1 || state.get().bannedAddress1 != state.get().bannedAddress2  || state.get().bannedAddress2 != state.get().bannedAddress3)
        {
            return ;
        }

        state.mut().bannedAddress.set(state.get().numberOfBannedAddress, input.bannedAddress);

        state.mut().numberOfBannedAddress++;
        state.mut().newAdminAddress1 = NULL_ID;
        state.mut().newAdminAddress2 = NULL_ID;
        state.mut().newAdminAddress3 = NULL_ID;

    }

    PUBLIC_PROCEDURE(submitUnbannedAddress)
    {
        if(qpi.invocator() == state.get().authAddress1)
        {
            state.mut().unbannedAddress1 = input.unbannedAddress;
        }
        if(qpi.invocator() == state.get().authAddress2)
        {
            state.mut().unbannedAddress2 = input.unbannedAddress;
        }
        if(qpi.invocator() == state.get().authAddress3)
        {
            state.mut().unbannedAddress3 = input.unbannedAddress;
        }

    }

    struct unblockBannedAddress_locals
    {
        uint32 _t, flag;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(unblockBannedAddress)
    {
        if(qpi.invocator() != state.get().authAddress1 && qpi.invocator() != state.get().authAddress2 && qpi.invocator() != state.get().authAddress3)
        {
            return ;
        }

        if(input.unbannedAddress == NULL_ID || input.unbannedAddress != state.get().unbannedAddress1 || state.get().unbannedAddress1 != state.get().unbannedAddress2  || state.get().unbannedAddress2 != state.get().unbannedAddress3)
        {
            return ;
        }

        locals.flag = 0;

        for(locals._t = 0; locals._t < state.get().numberOfBannedAddress; locals._t++)
        {
            if(locals.flag == 1 || input.unbannedAddress == state.get().bannedAddress.get(locals._t))
            {
                if(locals._t == state.get().numberOfBannedAddress - 1)
                {
                    state.mut().bannedAddress.set(locals._t, NULL_ID);
                    locals.flag = 1;
                    break;
                }
                state.mut().bannedAddress.set(locals._t, state.get().bannedAddress.get(locals._t + 1));
                locals.flag = 1;
            }
        }

        if(locals.flag == 1)
        {
            state.mut().numberOfBannedAddress--;
        }
        state.mut().unbannedAddress1 = NULL_ID;
        state.mut().unbannedAddress2 = NULL_ID;
        state.mut().unbannedAddress3 = NULL_ID;

    }

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getData, 1);

        REGISTER_USER_PROCEDURE(submitAuthAddress, 1);
        REGISTER_USER_PROCEDURE(changeAuthAddress, 2);
        REGISTER_USER_PROCEDURE(submitDistributionPermille, 3);
        REGISTER_USER_PROCEDURE(changeDistributionPermille, 4);
        REGISTER_USER_PROCEDURE(submitReinvestingAddress, 5);
        REGISTER_USER_PROCEDURE(changeReinvestingAddress, 6);
        REGISTER_USER_PROCEDURE(submitAdminAddress, 7);
        REGISTER_USER_PROCEDURE(changeAdminAddress, 8);
        REGISTER_USER_PROCEDURE(submitBannedAddress, 9);
        REGISTER_USER_PROCEDURE(saveBannedAddress, 10);
        REGISTER_USER_PROCEDURE(submitUnbannedAddress, 11);
        REGISTER_USER_PROCEDURE(unblockBannedAddress, 12);

	}

	INITIALIZE()
    {
        state.mut().QCAP_ISSUER = ID(_Q, _C, _A, _P, _W, _M, _Y, _R, _S, _H, _L, _B, _J, _H, _S, _T, _T, _Z, _Q, _V, _C, _I, _B, _A, _R, _V, _O, _A, _S, _K, _D, _E, _N, _A, _S, _A, _K, _N, _O, _B, _R, _G, _P, _F, _W, _W, _K, _R, _C, _U, _V, _U, _A, _X, _Y, _E);
        state.mut().authAddress1 = ID(_T, _K, _U, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
		state.mut().authAddress2 = ID(_F, _X, _J, _F, _B, _T, _J, _M, _Y, _F, _J, _H, _P, _B, _X, _C, _D, _Q, _T, _L, _Y, _U, _K, _G, _M, _H, _B, _B, _Z, _A, _A, _F, _T, _I, _C, _W, _U, _K, _R, _B, _M, _E, _K, _Y, _N, _U, _P, _M, _R, _M, _B, _D, _N, _D, _R, _G);
        state.mut().authAddress3 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
        state.mut().reinvestingAddress = ID(_R, _U, _U, _Y, _R, _V, _N, _K, _J, _X, _M, _L, _R, _B, _B, _I, _R, _I, _P, _D, _I, _B, _M, _H, _D, _H, _U, _A, _Z, _B, _Q, _K, _N, _B, _J, _T, _R, _D, _S, _P, _G, _C, _L, _Z, _C, _Q, _W, _A, _K, _C, _F, _Q, _J, _K, _K, _E);
        state.mut().adminAddress = ID(_H, _E, _C, _G, _U, _G, _H, _C, _J, _K, _Q, _O, _S, _D, _T, _M, _E, _H, _Q, _Y, _W, _D, _D, _T, _L, _F, _D, _A, _S, _Z, _K, _M, _G, _J, _L, _S, _R, _C, _S, _T, _H, _H, _A, _P, _P, _E, _D, _L, _G, _B, _L, _X, _J, _M, _N, _D);

        state.mut().shareholderDividend = 30;
        state.mut().QCAPHolderPermille = 500;
        state.mut().reinvestingPermille = 450;
        state.mut().devPermille = 20;
        state.mut().burnPermille = 0;

        /*
            initial banned addresses
        */
        state.mut().bannedAddress.set(0, ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D));
        state.mut().bannedAddress.set(1, ID(_E, _S, _C, _R, _O, _W, _B, _O, _T, _F, _T, _F, _I, _C, _I, _F, _P, _U, _X, _O, _J, _K, _G, _Q, _P, _Y, _X, _C, _A, _B, _L, _Z, _V, _M, _M, _U, _C, _M, _J, _F, _S, _G, _S, _A, _I, _A, _T, _Y, _I, _N, _V, _T, _Y, _G, _O, _A));
        state.mut().numberOfBannedAddress = 2;

	}

    struct END_EPOCH_locals
    {
        Entity entity;
        AssetPossessionIterator iter;
        Asset QCAPId;
        uint64 revenue;
        uint64 paymentForShareholders;
        uint64 paymentForQCAPHolders;
        uint64 paymentForReinvest;
        uint64 paymentForDevelopment;
        uint64 amountOfBurn;
        uint64 circulatedSupply;
        uint32 _t;
        id possessorPubkey;
    };

    END_EPOCH_WITH_LOCALS()
    {
        qpi.getEntity(SELF, locals.entity);
        locals.revenue = locals.entity.incomingAmount - locals.entity.outgoingAmount;

        locals.paymentForShareholders = div(locals.revenue * state.get().shareholderDividend, 1000ULL);
        locals.paymentForQCAPHolders = div(locals.revenue * state.get().QCAPHolderPermille, 1000ULL);
        locals.paymentForReinvest = div(locals.revenue * state.get().reinvestingPermille, 1000ULL);
        locals.amountOfBurn = div(locals.revenue * state.get().burnPermille, 1000ULL);
        locals.paymentForDevelopment = locals.revenue - locals.paymentForShareholders - locals.paymentForQCAPHolders - locals.paymentForReinvest - locals.amountOfBurn;

        if(locals.paymentForReinvest > QVAULT_MAX_REINVEST_AMOUNT)
        {
            locals.paymentForQCAPHolders += locals.paymentForReinvest - QVAULT_MAX_REINVEST_AMOUNT;
            locals.paymentForReinvest = QVAULT_MAX_REINVEST_AMOUNT;
        }

        qpi.distributeDividends(div(locals.paymentForShareholders, 676ULL));
        qpi.transfer(state.get().adminAddress, locals.paymentForDevelopment);
        qpi.transfer(state.get().reinvestingAddress, locals.paymentForReinvest);
        qpi.burn(locals.amountOfBurn);

        locals.circulatedSupply = QVAULT_QCAP_MAX_SUPPLY;

        for(locals._t = 0 ; locals._t < state.get().numberOfBannedAddress; locals._t++)
        {
            locals.circulatedSupply -= qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.get().QCAP_ISSUER, state.get().bannedAddress.get(locals._t), state.get().bannedAddress.get(locals._t), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
        }

        locals.QCAPId.assetName = QVAULT_QCAP_ASSETNAME;
        locals.QCAPId.issuer = state.get().QCAP_ISSUER;

        locals.iter.begin(locals.QCAPId);
        while (!locals.iter.reachedEnd())
        {
            locals.possessorPubkey = locals.iter.possessor();

            for(locals._t = 0 ; locals._t < state.get().numberOfBannedAddress; locals._t++)
            {
                if(locals.possessorPubkey == state.get().bannedAddress.get(locals._t))
                {
                    break;
                }
            }

            if(locals._t == state.get().numberOfBannedAddress)
            {
                qpi.transfer(locals.possessorPubkey, div(locals.paymentForQCAPHolders, locals.circulatedSupply) * qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.get().QCAP_ISSUER, locals.possessorPubkey, locals.possessorPubkey, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
            }

            locals.iter.next();
        }

    }
};
