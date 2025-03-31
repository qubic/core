using namespace QPI;

struct TESTEXB2
{
};

struct TESTEXB : public ContractBase
{
	//---------------------------------------------------------------
	// ASSET MANAGEMENT RIGHTS TRANSFER

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

	//---------------------------------------------------------------
	// CONTRACT INTERACTION / RESOLVING DEADLOCKS

public:
	typedef TESTEXA::QueryQpiFunctions_input CallFunctionOfTestExampleA_input;
	typedef TESTEXA::QueryQpiFunctions_output CallFunctionOfTestExampleA_output;

	PUBLIC_FUNCTION(CallFunctionOfTestExampleA)
#ifdef NO_UEFI
		printf("Before wait/deadlock in contract %u function\n", CONTRACT_INDEX);
#endif
		CALL_OTHER_CONTRACT_FUNCTION(TESTEXA, QueryQpiFunctions, input, output);
#ifdef NO_UEFI
		printf("After wait/deadlock in contract %u function\n", CONTRACT_INDEX);
#endif
	_

	//---------------------------------------------------------------
	// POST_INCOMING_TRANSFER CALLBACK
public:
	typedef NoData IncomingTransferAmounts_input;
	struct IncomingTransferAmounts_output
	{
		sint64 standardTransactionAmount;
		uint64 procedureTransactionAmount;
		uint64 qpiTransferAmount;
		uint64 qpiDistributeDividendsAmount;
		uint64 revenueDonationAmount;
		uint64 ipoBidRefundAmount;
	};

	struct QpiTransfer_input
	{
		id destinationPublicKey;
		sint64 amount;
	};
	typedef NoData QpiTransfer_output;

	struct QpiDistributeDividends_input
	{
		sint64 amountPerShare;
	};
	typedef NoData QpiDistributeDividends_output;

protected:
	IncomingTransferAmounts_output incomingTransfers;

	PUBLIC_FUNCTION(IncomingTransferAmounts)
		output = state.incomingTransfers;
	_

	struct POST_INCOMING_TRANSFER_locals
	{
		Entity before;
		Entity after;
	};

	POST_INCOMING_TRANSFER_WITH_LOCALS
		ASSERT(input.amount > 0);
		switch (input.type)
		{
		case TransferType::standardTransaction:
			state.incomingTransfers.standardTransactionAmount += input.amount;
			break;
		case TransferType::procedureTransaction:
			state.incomingTransfers.procedureTransactionAmount += input.amount;
			break;
		case TransferType::qpiTransfer:
			state.incomingTransfers.qpiTransferAmount += input.amount;
			break;
		case TransferType::qpiDistributeDividends:
			state.incomingTransfers.qpiDistributeDividendsAmount += input.amount;
			break;
		case TransferType::revenueDonation:
			state.incomingTransfers.revenueDonationAmount += input.amount;
			break;
		case TransferType::ipoBidRefund:
			state.incomingTransfers.ipoBidRefundAmount += input.amount;
			break;
		default:
			ASSERT(false);
		}

		// check that everything that transfers QUs is disabled
		ASSERT(qpi.transfer(SELF, 1000) < 0);
		ASSERT(!qpi.distributeDividends(10));
	_

	PUBLIC_PROCEDURE(QpiTransfer)
		qpi.transfer(input.destinationPublicKey, input.amount);
	_

	PUBLIC_PROCEDURE(QpiDistributeDividends)
		qpi.distributeDividends(input.amountPerShare);
	_

	//---------------------------------------------------------------
	// COMMON PARTS

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_FUNCTION(CallFunctionOfTestExampleA, 1);

		REGISTER_USER_FUNCTION(IncomingTransferAmounts, 20);

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
		REGISTER_USER_PROCEDURE(GetTestExampleAShareManagementRights, 7);

		REGISTER_USER_PROCEDURE(QpiTransfer, 20);
		REGISTER_USER_PROCEDURE(QpiDistributeDividends, 21);
	_
};
