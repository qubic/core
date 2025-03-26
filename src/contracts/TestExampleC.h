using namespace QPI;

struct TESTEXC2
{
};

struct TESTEXC : public ContractBase
{
	struct GetTestExampleAShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
	};
	struct GetTestExampleAShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

protected:
	int waitInPreAcqiuireSharesCallback;
	
	struct GetTestExampleAShareManagementRights_locals
	{
		TESTEXA::TransferShareManagementRights_input input;
		TESTEXA::TransferShareManagementRights_output output;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(GetTestExampleAShareManagementRights)
		// This is for the ResolveDeadlockCallbackProcedureAndConcurrentFunction in test/contract_testex.cpp:
		locals.input.asset = input.asset;
		locals.input.numberOfShares = input.numberOfShares;
		locals.input.newManagingContractIndex = SELF_INDEX;
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXA, TransferShareManagementRights, locals.input, locals.output, qpi.invocationReward());
		output.transferredNumberOfShares = locals.output.transferredNumberOfShares;
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_PROCEDURE(GetTestExampleAShareManagementRights, 7);
	_

	struct PRE_ACQUIRE_SHARES_locals
	{
		TESTEXA::RunHeavyComputation_input heavyComputationInput;
		TESTEXA::RunHeavyComputation_output heavyComputationOutput;
		TESTEXB::SetPreAcquireSharesOutput_input textExBInput;
		TESTEXB::SetPreAcquireSharesOutput_output textExBOutput;
	};

	PRE_ACQUIRE_SHARES_WITH_LOCALS
		// This is for the ResolveDeadlockCallbackProcedureAndConcurrentFunction in test/contract_testex.cpp:
		// 1. Check reuse of already owned write lock of TESTEXA and delay execution in order to make sure that the
		//    concurrent contract function TESTEXB::CallTextExAFunc() is running or waiting for read lock of TEXTEXA.
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXA, RunHeavyComputation, locals.heavyComputationInput, locals.heavyComputationOutput, 0);

		// 2. Try to invoke procedure of TESTEXB to trigger deadlock (waiting for release of read lock).
#ifdef NO_UEFI
		printf("Before wait/deadlock in contract %u procedure\n", CONTRACT_INDEX);
#endif
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXB, SetPreAcquireSharesOutput, locals.textExBInput, locals.textExBOutput, 0);
#ifdef NO_UEFI
		printf("After wait/deadlock in contract %u procedure\n", CONTRACT_INDEX);
#endif

		// bug check regarding size of locals
		ASSERT(!locals.textExBInput.allowTransfer);
		ASSERT(!locals.textExBInput.requestedFee);
	_
};
