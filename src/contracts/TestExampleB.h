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

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
	_

	PRE_RELEASE_SHARES
		output = state.preReleaseSharesOutput;
		state.prevPreReleaseSharesInput = input;
	_

	POST_RELEASE_SHARES
		state.postReleaseSharesCounter++;
		state.prevPostReleaseSharesInput = input;
	_

	PRE_ACQUIRE_SHARES
		// check that acquire is triggered by owner, otherwise allowing another contract to acquire
		// management rights is risky
		if (qpi.originator() == input.owner)
		{
            output = state.preAcquireSharesOutput;
		}
        state.prevPreAcquireSharesInput = input;
	_

	POST_ACQUIRE_SHARES
        state.postAcquireShareCounter++;
        state.prevPostAcquireSharesInput = input;
    _
};
