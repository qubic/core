using namespace QPI;

struct QVAULT2
{
};

struct QVAULT : public ContractBase
{

public:

    /*
        submitAuthAddress PROCEDURE
        multisig addresses can submit the new multisig address using this procedure.1
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
        the new multisig address can be changed by this procedure.
        there should have already to submit the new multisig address by the 2 multisig addresses. if else, won't be changed with new address
    */

    struct changeAuthAddress_input
    {
        uint32 numberOfChangedAddress;
    };

    struct changeAuthAddress_output
    {
    };

    /*
        submitDistributionPercent PROCEDURE
        the new distribution percents can be submitted by the multisig addresses in this procedure.
    */

    struct submitDistributionPercent_input
    {
        uint32 newQCAPHolderPercent;
        uint32 newReinvestingPercent;
        uint32 newDevPercent;
    };

    struct submitDistributionPercent_output
    {
    };

    /*
        changeDistributionPercent PROCEDURE
        the new distribution percents can be changed by this procedure.
        the new distribution percents should have already to submit by multsig addresses. if else, won't be changed with new percents.
    */

    struct changeDistributionPercent_input
    {
        uint32 newQCAPHolderPercent;
        uint32 newReinvestingPercent;
        uint32 newDevPercent;
    };

    struct changeDistributionPercent_output
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
        the new reinvesting address can be changed by this procedure.
        the new reinvesting address should have already to submit by multsig addresses. if else, won't be changed with new address.
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
        uint32 computorDividend, QCAPHolderPercent, reinvestingPercent, devPercent;
        id authAddress1, authAddress2, authAddress3, reinvestingAddress, adminAddress;
        id newAuthAddress1, newAuthAddress2, newAuthAddress3;
        id newReinvestingAddress1, newReinvestingAddress2, newReinvestingAddress3;
        id newAdminAddress1, newAdminAddress2, newAdminAddress3;
        bit agreedChangeAddress1, agreedChangeAddress2, agreedChangeAddress3;
        bit agreedChangeFee1, agreedChangeFee2, agreedChangeFee3;
        bit agreedChangeReinvestingAddress1, agreedChangeReinvestingAddress2, agreedChangeReinvestingAddress3;
        bit agreedChangeAdminaddress1, agreedChangeAdminaddress2, agreedChangeAdminaddress3;
    };

    struct submitAdminAddress_input
    {
        id newAddress;
    };

    struct submitAdminAddress_output
    {
    };

    struct changeAdminAddress_input
    {
        id newAddress;
    };

    struct changeAdminAddress_output
    {
    };

protected:

    id authAddress1, authAddress2, authAddress3, newAuthAddress1, newAuthAddress2, newAuthAddress3;
    id reinvestingAddress, newReinvestingAddress1, newReinvestingAddress2, newReinvestingAddress3;
    id adminAddress, newAdminAddress1, newAdminAddress2, newAdminAddress3;
    uint32 computorDividend, QCAPHolderPercent, reinvestingPercent, devPercent;
    uint32 newQCAPHolderPercent1, newReinvestingPercent1, newDevPercent1;
    uint32 newQCAPHolderPercent2, newReinvestingPercent2, newDevPercent2;
    uint32 newQCAPHolderPercent3, newReinvestingPercent3, newDevPercent3;
    bit agreedChangeAddress1, agreedChangeAddress2, agreedChangeAddress3;
    bit agreedChangeFee1, agreedChangeFee2, agreedChangeFee3;
    bit agreedChangeReinvestingAddress1, agreedChangeReinvestingAddress2, agreedChangeReinvestingAddress3;
    bit agreedChangeAdminaddress1, agreedChangeAdminaddress2, agreedChangeAdminaddress3;

    PUBLIC_PROCEDURE(submitAuthAddress)

        if(qpi.invocator() == state.authAddress1)
        {
            state.newAuthAddress1 = input.newAddress;
            state.agreedChangeAddress1 = 1;
        }
        if(qpi.invocator() == state.authAddress2)
        {
            state.newAuthAddress2 = input.newAddress;
            state.agreedChangeAddress2 = 1;
        }
        if(qpi.invocator() == state.authAddress3)
        {
            state.newAuthAddress3 = input.newAddress;
            state.agreedChangeAddress3 = 1;
        }

    _

    struct changeAuthAddress_locals {
        bit succeed;
    };

    PUBLIC_PROCEDURE(changeAuthAddress)

        if(qpi.invocator() != state.authAddress1 && qpi.invocator() != state.authAddress2 && qpi.invocator() != state.authAddress3)
        {
            return ;
        }

        locals.succeed = 0;

        if(input.numberOfChangedAddress == 1 && state.agreedChangeAddress2 == 1 && state.agreedChangeAddress3 == 1 && state.newAuthAddress2 == state.newAuthAddress3)
        {
            state.authAddress1 = state.newAuthAddress2;
            locals.succeed = 1;
        }

        if(input.numberOfChangedAddress == 2 && state.agreedChangeAddress1 == 1 && state.agreedChangeAddress3 == 1 && state.newAuthAddress1 == state.newAuthAddress3)
        {
            state.authAddress2 = state.newAuthAddress1;
            locals.succeed = 1;
        }

        if(input.numberOfChangedAddress == 3 && state.agreedChangeAddress1 == 1 && state.agreedChangeAddress2 == 1 && state.newAuthAddress1 == state.newAuthAddress2)
        {
            state.authAddress3 = state.newAuthAddress1;
            locals.succeed = 1;
        }

        if(locals.succeed == 1)
        {
            state.agreedChangeAddress1 = 0;
            state.agreedChangeAddress2 = 0;
            state.agreedChangeAddress3 = 0;
            state.newAuthAddress1 = 0;
            state.newAuthAddress2 = 0;
            state.newAuthAddress3 = 0;
        }
    _

    PUBLIC_PROCEDURE(submitDistributionPercent)

        if(100 - state.computorDividend != input.newDevPercent + input.newQCAPHolderPercent + input.newReinvestingPercent)
        {
            return ;
        }

        if(qpi.invocator() == state.authAddress1)
        {
            state.agreedChangeFee1 = 1;
            state.newDevPercent1 = input.newDevPercent;
            state.newQCAPHolderPercent1 = input.newQCAPHolderPercent;
            state.newReinvestingPercent1 = input.newReinvestingPercent;
        }

        if(qpi.invocator() == state.authAddress2)
        {
            state.agreedChangeFee2 = 1;
            state.newDevPercent2 = input.newDevPercent;
            state.newQCAPHolderPercent2 = input.newQCAPHolderPercent;
            state.newReinvestingPercent2 = input.newReinvestingPercent;
        }

        if(qpi.invocator() == state.authAddress3)
        {
            state.agreedChangeFee3 = 1;
            state.newDevPercent3 = input.newDevPercent;
            state.newQCAPHolderPercent3 = input.newQCAPHolderPercent;
            state.newReinvestingPercent3 = input.newReinvestingPercent;
        }

    _

    PUBLIC_PROCEDURE(changeDistributionPercent)

        if(qpi.invocator() != state.authAddress1 && qpi.invocator() != state.authAddress2 && qpi.invocator() != state.authAddress3)
        {
            return ;
        }

        if(state.agreedChangeFee1 != 1 || state.agreedChangeFee2 != 1 || state.agreedChangeFee3 != 1)
        {
            return ;
        }

        if(input.newDevPercent != state.newDevPercent1 || state.newDevPercent1 != state.newDevPercent2 || state.newDevPercent2 != state.newDevPercent3)
        {
            return ;
        }

        if(input.newQCAPHolderPercent != state.newQCAPHolderPercent1 || state.newQCAPHolderPercent1 != state.newQCAPHolderPercent2 || state.newQCAPHolderPercent2 != state.newQCAPHolderPercent3)
        {
            return ;
        }

        if(input.newReinvestingPercent != state.newReinvestingPercent1 || state.newReinvestingPercent1 != state.newReinvestingPercent2 || state.newReinvestingPercent2 != state.newReinvestingPercent3)
        {
            return ;
        }

        state.devPercent = state.newDevPercent1;
        state.QCAPHolderPercent = state.newQCAPHolderPercent1;
        state.reinvestingPercent = state.newReinvestingPercent1;

        state.agreedChangeFee1 = 0;
        state.agreedChangeFee2 = 0;
        state.agreedChangeFee3 = 0;

        state.newDevPercent1 = 0;
        state.newDevPercent2 = 0;
        state.newDevPercent3 = 0;

        state.newQCAPHolderPercent1 = 0;
        state.newQCAPHolderPercent2 = 0;
        state.newQCAPHolderPercent3 = 0;

        state.newReinvestingPercent1 = 0;
        state.newReinvestingPercent2 = 0;
        state.newReinvestingPercent3 = 0;
    _

    PUBLIC_PROCEDURE(submitReinvestingAddress)

        if(qpi.invocator() == state.authAddress1)
        {
            state.newReinvestingAddress1 = input.newAddress;
            state.agreedChangeReinvestingAddress1 = 1;
        }
        if(qpi.invocator() == state.authAddress2)
        {
            state.newReinvestingAddress2 = input.newAddress;
            state.agreedChangeReinvestingAddress2 = 1;
        }
        if(qpi.invocator() == state.authAddress3)
        {
            state.newReinvestingAddress3 = input.newAddress;
            state.agreedChangeReinvestingAddress3 = 1;
        }
    _

    PUBLIC_PROCEDURE(changeReinvestingAddress)

        if(qpi.invocator() != state.authAddress1 && qpi.invocator() != state.authAddress2 && qpi.invocator() != state.authAddress3)
        {
            return ;
        }

        if(state.agreedChangeReinvestingAddress1 != 1 || state.agreedChangeReinvestingAddress2 != 1 || state.agreedChangeReinvestingAddress3 != 1)
        {
            return ;
        }

        if(input.newAddress != state.newReinvestingAddress1 || state.newReinvestingAddress1 != state.newReinvestingAddress2  || state.newReinvestingAddress2 != state.newReinvestingAddress3)
        {
            return ;
        }

        state.reinvestingAddress = state.newReinvestingAddress1;

        state.agreedChangeReinvestingAddress1 = 0;
        state.agreedChangeReinvestingAddress2 = 0;
        state.agreedChangeReinvestingAddress3 = 0;

        state.newReinvestingAddress1 = NULL_ID;
        state.newReinvestingAddress2 = NULL_ID;
        state.newReinvestingAddress3 = NULL_ID;
    _

    PUBLIC_FUNCTION(getData)
    
        output.authAddress1 = state.authAddress1;
        output.authAddress2 = state.authAddress2;
        output.authAddress3 = state.authAddress3;
        output.reinvestingAddress = state.reinvestingAddress;
        output.computorDividend = state.computorDividend;
        output.devPercent = state.devPercent;
        output.QCAPHolderPercent = state.QCAPHolderPercent;
        output.reinvestingPercent = state.reinvestingPercent;
        output.agreedChangeAddress1 = state.agreedChangeAddress1;
        output.agreedChangeAddress2 = state.agreedChangeAddress2;
        output.agreedChangeAddress3 = state.agreedChangeAddress3;
        output.agreedChangeAdminaddress1 = state.agreedChangeAdminaddress1;
        output.agreedChangeAdminaddress2 = state.agreedChangeAdminaddress2;
        output.agreedChangeAdminaddress3 = state.agreedChangeAdminaddress3;
        output.agreedChangeFee1 = state.agreedChangeFee1;
        output.agreedChangeFee2 = state.agreedChangeFee2;
        output.agreedChangeFee3 = state.agreedChangeFee3;
        output.agreedChangeReinvestingAddress1 = state.agreedChangeReinvestingAddress1;
        output.agreedChangeReinvestingAddress2 = state.agreedChangeReinvestingAddress2;
        output.agreedChangeReinvestingAddress3 = state.agreedChangeReinvestingAddress3;
        output.adminAddress = state.adminAddress;
        output.newAuthAddress1 = state.newAuthAddress1;
        output.newAuthAddress2 = state.newAuthAddress2;
        output.newAuthAddress3 = state.newAuthAddress3;
        output.newAdminAddress1 = state.newAdminAddress1;
        output.newAdminAddress2 = state.newAdminAddress2;
        output.newAdminAddress3 = state.newAdminAddress3;
        output.newReinvestingAddress1 = state.newReinvestingAddress1;
        output.newReinvestingAddress2 = state.newReinvestingAddress2;
        output.newReinvestingAddress3 = state.newReinvestingAddress3;
        
    _

    PUBLIC_PROCEDURE(submitAdminAddress)

        if(qpi.invocator() == state.authAddress1)
        {
            state.newAdminAddress1 = input.newAddress;
            state.agreedChangeAdminaddress1 = 1;
        }
        if(qpi.invocator() == state.authAddress2)
        {
            state.newAdminAddress2 = input.newAddress;
            state.agreedChangeAdminaddress2 = 1;
        }
        if(qpi.invocator() == state.authAddress3)
        {
            state.newAdminAddress3 = input.newAddress;
            state.agreedChangeAdminaddress3 = 1;
        }

    _

    PUBLIC_PROCEDURE(changeAdminAddress)

        if(qpi.invocator() != state.authAddress1 && qpi.invocator() != state.authAddress2 && qpi.invocator() != state.authAddress3)
        {
            return ;
        }

        if(state.agreedChangeAdminaddress1 != 1 || state.agreedChangeAdminaddress2 != 1 || state.agreedChangeAdminaddress3 != 1)
        {
            return ;
        }

        if(input.newAddress != state.newAdminAddress1 || state.newAdminAddress1 != state.newAdminAddress2  || state.newAdminAddress2 != state.newAdminAddress3)
        {
            return ;
        }

        state.adminAddress = state.newAdminAddress1;

        state.agreedChangeAdminaddress1 = 0;
        state.agreedChangeAdminaddress2 = 0;
        state.agreedChangeAdminaddress3 = 0;

        state.newAdminAddress1 = NULL_ID;
        state.newAdminAddress2 = NULL_ID;
        state.newAdminAddress3 = NULL_ID;
    _

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES

        REGISTER_USER_FUNCTION(getData, 1);

        REGISTER_USER_PROCEDURE(submitAuthAddress, 1);
        REGISTER_USER_PROCEDURE(changeAuthAddress, 2);
        REGISTER_USER_PROCEDURE(submitDistributionPercent, 3);
        REGISTER_USER_PROCEDURE(changeDistributionPercent, 4);
        REGISTER_USER_PROCEDURE(submitReinvestingAddress, 5);
        REGISTER_USER_PROCEDURE(changeReinvestingAddress, 6);
        REGISTER_USER_PROCEDURE(submitAdminAddress, 7);
        REGISTER_USER_PROCEDURE(changeAdminAddress, 8);

	_

	INITIALIZE

        state.authAddress1 = ID(_T, _K, _U, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
		state.authAddress2 = ID(_F, _X, _J, _F, _B, _T, _J, _M, _Y, _F, _J, _H, _P, _B, _X, _C, _D, _Q, _T, _L, _Y, _U, _K, _G, _M, _H, _B, _B, _Z, _A, _A, _F, _T, _I, _C, _W, _U, _K, _R, _B, _M, _E, _K, _Y, _N, _U, _P, _M, _R, _M, _B, _D, _N, _D, _R, _G);
        state.authAddress3 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
        state.reinvestingAddress = ID(_R, _U, _U, _Y, _R, _V, _N, _K, _J, _X, _M, _L, _R, _B, _B, _I, _R, _I, _P, _D, _I, _B, _M, _H, _D, _H, _U, _A, _Z, _B, _Q, _K, _N, _B, _J, _T, _R, _D, _S, _P, _G, _C, _L, _Z, _C, _Q, _W, _A, _K, _C, _F, _Q, _J, _K, _K, _E);
        state.adminAddress = ID(_H, _E, _C, _G, _U, _G, _H, _C, _J, _K, _Q, _O, _S, _D, _T, _M, _E, _H, _Q, _Y, _W, _D, _D, _T, _L, _F, _D, _A, _S, _Z, _K, _M, _G, _J, _L, _S, _R, _C, _S, _T, _H, _H, _A, _P, _P, _E, _D, _L, _G, _B, _L, _X, _J, _M, _N, _D);

        state.computorDividend = 3;
        state.QCAPHolderPercent = 50;
        state.reinvestingPercent = 45;
        state.devPercent = 2;
	
	_
};
