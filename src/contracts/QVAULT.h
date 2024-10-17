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
        submitFees PROCEDURE
        the new fees can be submitted by the multisig addresses in this procedure.
    */

    struct submitFees_input
    {
        uint32 newQCAPHolder_fee;
        uint32 newreinvesting_fee;
        uint32 newdev_fee;
    };

    struct submitFees_output
    {
    };

    /*
        changeFees PROCEDURE
        the new fees can be changed by this procedure.
        the new fees should have already to submit by multsig addresses. if else, won't be changed with new fees.
    */

    struct changeFees_input
    {
        uint32 newQCAPHolder_fee;
        uint32 newreinvesting_fee;
        uint32 newdev_fee;
    };

    struct changeFees_output
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
        uint32 computor_fee, QCAPHolder_fee, reinvesting_fee, dev_fee;
        id AUTH_ADDRESS1, AUTH_ADDRESS2, AUTH_ADDRESS3, Reinvesting_address, admin_address;
        id newAuthAddress1, newAuthAddress2, newAuthAddress3;
        id newReinvesting_address1, newReinvesting_address2, newReinvesting_address3;
        id newAdmin_address1, newAdmin_address2, newAdmin_address3;
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

private:

    id AUTH_ADDRESS1, AUTH_ADDRESS2, AUTH_ADDRESS3, newAuthAddress1, newAuthAddress2, newAuthAddress3;
    id Reinvesting_address, newReinvesting_address1, newReinvesting_address2, newReinvesting_address3;
    id admin_address, newAdmin_address1, newAdmin_address2, newAdmin_address3;
    uint32 computor_fee, QCAPHolder_fee, reinvesting_fee, dev_fee;
    uint32 new_QCAPHolder_fee1, new_reinvesting_fee1, new_dev_fee1;
    uint32 new_QCAPHolder_fee2, new_reinvesting_fee2, new_dev_fee2;
    uint32 new_QCAPHolder_fee3, new_reinvesting_fee3, new_dev_fee3;
    bit agreedChangeAddress1, agreedChangeAddress2, agreedChangeAddress3;
    bit agreedChangeFee1, agreedChangeFee2, agreedChangeFee3;
    bit agreedChangeReinvestingAddress1, agreedChangeReinvestingAddress2, agreedChangeReinvestingAddress3;
    bit agreedChangeAdminaddress1, agreedChangeAdminaddress2, agreedChangeAdminaddress3;

    PUBLIC_PROCEDURE(submitAuthAddress)

        if(qpi.invocator() == state.AUTH_ADDRESS1)
        {
            state.newAuthAddress1 = input.newAddress;
            state.agreedChangeAddress1 = 1;
        }
        if(qpi.invocator() == state.AUTH_ADDRESS2)
        {
            state.newAuthAddress2 = input.newAddress;
            state.agreedChangeAddress2 = 1;
        }
        if(qpi.invocator() == state.AUTH_ADDRESS3)
        {
            state.newAuthAddress3 = input.newAddress;
            state.agreedChangeAddress3 = 1;
        }

    _

    struct changeAuthAddress_locals {
        bit succeed;
    };

    PUBLIC_PROCEDURE(changeAuthAddress)

        if(qpi.invocator() != state.AUTH_ADDRESS1 && qpi.invocator() != state.AUTH_ADDRESS2 && qpi.invocator() != state.AUTH_ADDRESS3)
        {
            return ;
        }

        locals.succeed = 0;

        if(input.numberOfChangedAddress == 1 && state.agreedChangeAddress2 == 1 && state.agreedChangeAddress3 == 1 && state.newAuthAddress2 == state.newAuthAddress3)
        {
            state.AUTH_ADDRESS1 = state.newAuthAddress2;
            locals.succeed = 1;
        }

        if(input.numberOfChangedAddress == 2 && state.agreedChangeAddress1 == 1 && state.agreedChangeAddress3 == 1 && state.newAuthAddress1 == state.newAuthAddress3)
        {
            state.AUTH_ADDRESS2 = state.newAuthAddress1;
            locals.succeed = 1;
        }

        if(input.numberOfChangedAddress == 3 && state.agreedChangeAddress1 == 1 && state.agreedChangeAddress2 == 1 && state.newAuthAddress1 == state.newAuthAddress2)
        {
            state.AUTH_ADDRESS3 = state.newAuthAddress1;
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

    PUBLIC_PROCEDURE(submitFees)

        if(100 - state.computor_fee != input.newdev_fee + input.newQCAPHolder_fee + input.newreinvesting_fee)
        {
            return ;
        }

        if(qpi.invocator() == state.AUTH_ADDRESS1)
        {
            state.agreedChangeFee1 = 1;
            state.new_dev_fee1 = input.newdev_fee;
            state.new_QCAPHolder_fee1 = input.newQCAPHolder_fee;
            state.new_reinvesting_fee1 = input.newreinvesting_fee;
        }

        if(qpi.invocator() == state.AUTH_ADDRESS2)
        {
            state.agreedChangeFee2 = 1;
            state.new_dev_fee2 = input.newdev_fee;
            state.new_QCAPHolder_fee2 = input.newQCAPHolder_fee;
            state.new_reinvesting_fee2 = input.newreinvesting_fee;
        }

        if(qpi.invocator() == state.AUTH_ADDRESS3)
        {
            state.agreedChangeFee3 = 1;
            state.new_dev_fee3 = input.newdev_fee;
            state.new_QCAPHolder_fee3 = input.newQCAPHolder_fee;
            state.new_reinvesting_fee3 = input.newreinvesting_fee;
        }

    _

    PUBLIC_PROCEDURE(changeFees)

        if(qpi.invocator() != state.AUTH_ADDRESS1 && qpi.invocator() != state.AUTH_ADDRESS2 && qpi.invocator() != state.AUTH_ADDRESS3)
        {
            return ;
        }

        if(state.agreedChangeFee1 != 1 || state.agreedChangeFee2 != 1 || state.agreedChangeFee3 != 1)
        {
            return ;
        }

        if(input.newdev_fee != state.new_dev_fee1 || state.new_dev_fee1 != state.new_dev_fee2 || state.new_dev_fee2 != state.new_dev_fee3)
        {
            return ;
        }

        if(input.newQCAPHolder_fee != state.new_QCAPHolder_fee1 || state.new_QCAPHolder_fee1 != state.new_QCAPHolder_fee2 || state.new_QCAPHolder_fee2 != state.new_QCAPHolder_fee3)
        {
            return ;
        }

        if(input.newreinvesting_fee != state.new_reinvesting_fee1 || state.new_reinvesting_fee1 != state.new_reinvesting_fee2 || state.new_reinvesting_fee2 != state.new_reinvesting_fee3)
        {
            return ;
        }

        state.dev_fee = state.new_dev_fee1;
        state.QCAPHolder_fee = state.new_QCAPHolder_fee1;
        state.reinvesting_fee = state.new_reinvesting_fee1;

        state.agreedChangeFee1 = 0;
        state.agreedChangeFee2 = 0;
        state.agreedChangeFee3 = 0;

        state.new_dev_fee1 = 0;
        state.new_dev_fee2 = 0;
        state.new_dev_fee3 = 0;

        state.new_QCAPHolder_fee1 = 0;
        state.new_QCAPHolder_fee2 = 0;
        state.new_QCAPHolder_fee3 = 0;

        state.new_reinvesting_fee1 = 0;
        state.new_reinvesting_fee2 = 0;
        state.new_reinvesting_fee3 = 0;
    _

    PUBLIC_PROCEDURE(submitReinvestingAddress)

        if(qpi.invocator() == state.AUTH_ADDRESS1)
        {
            state.newReinvesting_address1 = input.newAddress;
            state.agreedChangeReinvestingAddress1 = 1;
        }
        if(qpi.invocator() == state.AUTH_ADDRESS2)
        {
            state.newReinvesting_address2 = input.newAddress;
            state.agreedChangeReinvestingAddress2 = 1;
        }
        if(qpi.invocator() == state.AUTH_ADDRESS3)
        {
            state.newReinvesting_address3 = input.newAddress;
            state.agreedChangeReinvestingAddress3 = 1;
        }
    _

    PUBLIC_PROCEDURE(changeReinvestingAddress)

        if(qpi.invocator() != state.AUTH_ADDRESS1 && qpi.invocator() != state.AUTH_ADDRESS2 && qpi.invocator() != state.AUTH_ADDRESS3)
        {
            return ;
        }

        if(state.agreedChangeReinvestingAddress1 != 1 || state.agreedChangeReinvestingAddress2 != 1 || state.agreedChangeReinvestingAddress3 != 1)
        {
            return ;
        }

        if(input.newAddress != state.newReinvesting_address1 || state.newReinvesting_address1 != state.newReinvesting_address2  || state.newReinvesting_address2 != state.newReinvesting_address3)
        {
            return ;
        }

        state.Reinvesting_address = state.newReinvesting_address1;

        state.agreedChangeReinvestingAddress1 = 0;
        state.agreedChangeReinvestingAddress2 = 0;
        state.agreedChangeReinvestingAddress3 = 0;

        state.newReinvesting_address1 = NULL_ID;
        state.newReinvesting_address2 = NULL_ID;
        state.newReinvesting_address3 = NULL_ID;
    _

    PUBLIC_FUNCTION(getData)
    
        output.AUTH_ADDRESS1 = state.AUTH_ADDRESS1;
        output.AUTH_ADDRESS2 = state.AUTH_ADDRESS2;
        output.AUTH_ADDRESS3 = state.AUTH_ADDRESS3;
        output.Reinvesting_address = state.Reinvesting_address;
        output.computor_fee = state.computor_fee;
        output.dev_fee = state.dev_fee;
        output.QCAPHolder_fee = state.QCAPHolder_fee;
        output.reinvesting_fee = state.reinvesting_fee;
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
        output.admin_address = state.admin_address;
        output.newAuthAddress1 = state.newAuthAddress1;
        output.newAuthAddress2 = state.newAuthAddress2;
        output.newAuthAddress3 = state.newAuthAddress3;
        output.newAdmin_address1 = state.newAdmin_address1;
        output.newAdmin_address2 = state.newAdmin_address2;
        output.newAdmin_address3 = state.newAdmin_address3;
        output.newReinvesting_address1 = state.newReinvesting_address1;
        output.newReinvesting_address2 = state.newReinvesting_address2;
        output.newReinvesting_address3 = state.newReinvesting_address3;
        
    _

    PUBLIC_PROCEDURE(submitAdminAddress)

        if(qpi.invocator() == state.AUTH_ADDRESS1)
        {
            state.newAdmin_address1 = input.newAddress;
            state.agreedChangeAdminaddress1 = 1;
        }
        if(qpi.invocator() == state.AUTH_ADDRESS2)
        {
            state.newAdmin_address2 = input.newAddress;
            state.agreedChangeAdminaddress2 = 1;
        }
        if(qpi.invocator() == state.AUTH_ADDRESS3)
        {
            state.newAdmin_address3 = input.newAddress;
            state.agreedChangeAdminaddress3 = 1;
        }

    _

    PUBLIC_PROCEDURE(changeAdminAddress)

        if(qpi.invocator() != state.AUTH_ADDRESS1 && qpi.invocator() != state.AUTH_ADDRESS2 && qpi.invocator() != state.AUTH_ADDRESS3)
        {
            return ;
        }

        if(state.agreedChangeAdminaddress1 != 1 || state.agreedChangeAdminaddress2 != 1 || state.agreedChangeAdminaddress3 != 1)
        {
            return ;
        }

        if(input.newAddress != state.newAdmin_address1 || state.newAdmin_address1 != state.newAdmin_address2  || state.newAdmin_address2 != state.newAdmin_address3)
        {
            return ;
        }

        state.admin_address = state.newAdmin_address1;

        state.agreedChangeAdminaddress1 = 0;
        state.agreedChangeAdminaddress2 = 0;
        state.agreedChangeAdminaddress3 = 0;

        state.newAdmin_address1 = NULL_ID;
        state.newAdmin_address2 = NULL_ID;
        state.newAdmin_address3 = NULL_ID;
    _

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES

        REGISTER_USER_FUNCTION(getData, 1);

        REGISTER_USER_PROCEDURE(submitAuthAddress, 1);
        REGISTER_USER_PROCEDURE(changeAuthAddress, 2);
        REGISTER_USER_PROCEDURE(submitFees, 3);
        REGISTER_USER_PROCEDURE(changeFees, 4);
        REGISTER_USER_PROCEDURE(submitReinvestingAddress, 5);
        REGISTER_USER_PROCEDURE(changeReinvestingAddress, 6);
        REGISTER_USER_PROCEDURE(submitAdminAddress, 7);
        REGISTER_USER_PROCEDURE(changeAdminAddress, 8);

	_

	INITIALIZE

        state.AUTH_ADDRESS1 = ID(_T, _K, _U, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
		state.AUTH_ADDRESS2 = ID(_F, _X, _J, _F, _B, _T, _J, _M, _Y, _F, _J, _H, _P, _B, _X, _C, _D, _Q, _T, _L, _Y, _U, _K, _G, _M, _H, _B, _B, _Z, _A, _A, _F, _T, _I, _C, _W, _U, _K, _R, _B, _M, _E, _K, _Y, _N, _U, _P, _M, _R, _M, _B, _D, _N, _D, _R, _G);
        state.AUTH_ADDRESS3 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
        state.Reinvesting_address = ID(_R, _U, _U, _Y, _R, _V, _N, _K, _J, _X, _M, _L, _R, _B, _B, _I, _R, _I, _P, _D, _I, _B, _M, _H, _D, _H, _U, _A, _Z, _B, _Q, _K, _N, _B, _J, _T, _R, _D, _S, _P, _G, _C, _L, _Z, _C, _Q, _W, _A, _K, _C, _F, _Q, _J, _K, _K, _E);
        state.admin_address = ID(_H, _E, _C, _G, _U, _G, _H, _C, _J, _K, _Q, _O, _S, _D, _T, _M, _E, _H, _Q, _Y, _W, _D, _D, _T, _L, _F, _D, _A, _S, _Z, _K, _M, _G, _J, _L, _S, _R, _C, _S, _T, _H, _H, _A, _P, _P, _E, _D, _L, _G, _B, _L, _X, _J, _M, _N, _D);

        state.computor_fee = 3;
        state.QCAPHolder_fee = 50;
        state.reinvesting_fee = 45;
        state.dev_fee = 2;
	
	_
};
