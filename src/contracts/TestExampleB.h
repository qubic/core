using namespace QPI;

struct TESTEXB2
{
};

struct TESTEXB : public ContractBase
{
	struct IssueAsset_input
	{
		uint64 assetName;
		sint64 numberOfShares;
		uint64 unitOfMeasurement;
		sint8 numberOfDecimalPlaces;
	};
	struct IssueAsset_output
	{
		sint64 issuedNumberOfShares;
	};

	struct TransferShareOwnershipAndPossession_input
	{
		Asset asset;
		sint64 numberOfShares;
		id newOwnerAndPossessor;
	};
	struct TransferShareOwnershipAndPossession_output
	{
		sint64 transferredNumberOfShares;
	};

	struct TransferShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};
	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

	typedef PreManagementRightsTransfer_output SetPreReleaseSharesOutput_input;
	typedef NoData SetPreReleaseSharesOutput_output;

	typedef PreManagementRightsTransfer_output SetPreAcquireSharesOutput_input;
	typedef NoData SetPreAcquireSharesOutput_output;

	struct AcquireShareManagementRights_input
	{
		Asset asset;
		id ownerAndPossessor;
		sint64 numberOfShares;
		uint32 oldManagingContractIndex;
	};
	struct AcquireShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

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
	
	PreManagementRightsTransfer_output preReleaseSharesOutput;
	PreManagementRightsTransfer_output preAcquireSharesOutput;

	PreManagementRightsTransfer_input prevPreReleaseSharesInput;
	PreManagementRightsTransfer_input prevPreAcquireSharesInput;
	PostManagementRightsTransfer_input prevPostReleaseSharesInput;
	PostManagementRightsTransfer_input prevPostAcquireSharesInput;
	uint32 postReleaseSharesCounter;
	uint32 postAcquireShareCounter;

	PUBLIC_PROCEDURE(IssueAsset)
		output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
	_

	PUBLIC_PROCEDURE(TransferShareOwnershipAndPossession)
		if (qpi.transferShareOwnershipAndPossession(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newOwnerAndPossessor) > 0)
		{
			output.transferredNumberOfShares = input.numberOfShares;
		}
	_

	PUBLIC_PROCEDURE(TransferShareManagementRights)
		if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
			input.newManagingContractIndex, input.newManagingContractIndex, qpi.invocationReward()) < 0)
		{
			// error
			output.transferredNumberOfShares = 0;
		}
		else
		{
			// success
			output.transferredNumberOfShares = input.numberOfShares;
		}
	_

	PUBLIC_PROCEDURE(SetPreReleaseSharesOutput)
		state.preReleaseSharesOutput = input;
	_

	PUBLIC_PROCEDURE(SetPreAcquireSharesOutput)
        state.preAcquireSharesOutput = input;
_

	PUBLIC_PROCEDURE(AcquireShareManagementRights)
		if (qpi.acquireShares(input.asset, input.ownerAndPossessor, input.ownerAndPossessor, input.numberOfShares,
			input.oldManagingContractIndex, input.oldManagingContractIndex, qpi.invocationReward()) >= 0)
		{
			// success
			output.transferredNumberOfShares = input.numberOfShares;
		}
    _

	struct GetTestExampleAShareManagementRights_locals
	{
		TESTEXA::TransferShareManagementRights_input input;
		TESTEXA::TransferShareManagementRights_output output;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(GetTestExampleAShareManagementRights)
		locals.input.asset = input.asset;
		locals.input.numberOfShares = input.numberOfShares;
		locals.input.newManagingContractIndex = SELF_INDEX;
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXA, TransferShareManagementRights, locals.input, locals.output, qpi.invocationReward());
		output.transferredNumberOfShares = locals.output.transferredNumberOfShares;
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
		REGISTER_USER_PROCEDURE(GetTestExampleAShareManagementRights, 7);
	_

	PRE_RELEASE_SHARES
		// check that qpi.acquireShares() leading to this callback is triggered by owner,
		// otherwise allowing another contract to acquire management rights is risky
		if (qpi.originator() == input.owner)
		{
			output = state.preReleaseSharesOutput;
		}
		state.prevPreReleaseSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immeditately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	_

	POST_RELEASE_SHARES
		state.postReleaseSharesCounter++;
		state.prevPostReleaseSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immeditately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	_

	PRE_ACQUIRE_SHARES
		output = state.preAcquireSharesOutput;
		state.prevPreAcquireSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immeditately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	_

	POST_ACQUIRE_SHARES
		state.postAcquireShareCounter++;
		state.prevPostAcquireSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immeditately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	_
};
