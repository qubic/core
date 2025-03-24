using namespace QPI;

struct TESTEXA2
{
};

struct TESTEXA : public ContractBase
{
	struct QpiFunctionsOutput
	{
		id arbitrator;
		id computor0;
		id invocator;
		id originator;
		sint64 invocationReward;
		sint32 numberOfTickTransactions;
		uint32 tick;
		uint16 epoch;
		uint16 millisecond;
		uint8 year;   // [0..99] (0 = 2000, 1 = 2001, ..., 99 = 2099)
		uint8 month;  // [1..12]
		uint8 day;    // [1..31]
		uint8 hour;   // [0..23]
		uint8 minute; // [0..59]
		uint8 second;
		uint8 dayOfWeek;
	};

	struct QueryQpiFunctions_input
	{
		Array<sint8, 128> data;
		Array<sint8, 64> signature;
		id entity;
	};
	struct QueryQpiFunctions_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
		id inputDataK12;
		bit inputSignatureValid;
	};

	struct QueryQpiFunctionsToState_input {};
	struct QueryQpiFunctionsToState_output {};

	struct ReturnQpiFunctionsOutputBeginTick_input 
	{
		uint32 tick;
	};
	struct ReturnQpiFunctionsOutputBeginTick_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
	};

	struct ReturnQpiFunctionsOutputEndTick_input
	{
		uint32 tick;
	};
	struct ReturnQpiFunctionsOutputEndTick_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
	};

	struct ReturnQpiFunctionsOutputUserProc_input
	{
		uint32 tick;
	};
	struct ReturnQpiFunctionsOutputUserProc_output
	{
		QpiFunctionsOutput qpiFunctionsOutput;
	};

	typedef NoData ErrorTriggerFunction_input;
	typedef NoData ErrorTriggerFunction_output;

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

	typedef sint64 RunHeavyComputation_input;
	typedef sint64 RunHeavyComputation_output;

protected:

	QpiFunctionsOutput qpiFunctionsOutputTemp;
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputBeginTick; // Output of QPI functions queried by the BEGIN_TICK procedure for the last 16 ticks
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputEndTick; // Output of QPI functions queried by the END_TICK procedure for the last 16 ticks
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputUserProc; // Output of QPI functions queried by the USER_PROCEDURE
	
	PreManagementRightsTransfer_output preReleaseSharesOutput;
	PreManagementRightsTransfer_output preAcquireSharesOutput;

	PreManagementRightsTransfer_input prevPreReleaseSharesInput;
	PreManagementRightsTransfer_input prevPreAcquireSharesInput;
	PostManagementRightsTransfer_input prevPostReleaseSharesInput;
	PostManagementRightsTransfer_input prevPostAcquireSharesInput;
	uint32 postReleaseSharesCounter;
	uint32 postAcquireShareCounter;

	sint64 heavyComputationResult;

	PUBLIC_FUNCTION(QueryQpiFunctions)
		output.qpiFunctionsOutput.year = qpi.year();
		output.qpiFunctionsOutput.month = qpi.month();
		output.qpiFunctionsOutput.day = qpi.day();
		output.qpiFunctionsOutput.hour = qpi.hour();
		output.qpiFunctionsOutput.minute = qpi.minute();
		output.qpiFunctionsOutput.second = qpi.second();
		output.qpiFunctionsOutput.millisecond = qpi.millisecond();
		output.qpiFunctionsOutput.dayOfWeek = qpi.dayOfWeek(output.qpiFunctionsOutput.year, output.qpiFunctionsOutput.month, output.qpiFunctionsOutput.day);
		output.qpiFunctionsOutput.arbitrator = qpi.arbitrator();
		output.qpiFunctionsOutput.computor0 = qpi.computor(0);
		output.qpiFunctionsOutput.epoch = qpi.epoch();
		output.qpiFunctionsOutput.invocationReward = qpi.invocationReward();
		output.qpiFunctionsOutput.invocator = qpi.invocator();
		output.qpiFunctionsOutput.numberOfTickTransactions = qpi.numberOfTickTransactions();
		output.qpiFunctionsOutput.originator = qpi.originator();
		output.qpiFunctionsOutput.tick = qpi.tick();
		output.inputDataK12 = qpi.K12(input.data);
		output.inputSignatureValid = qpi.signatureValidity(input.entity, output.inputDataK12, input.signature);
	_

	PUBLIC_PROCEDURE(QueryQpiFunctionsToState)
		state.qpiFunctionsOutputTemp.year = qpi.year();
		state.qpiFunctionsOutputTemp.month = qpi.month();
		state.qpiFunctionsOutputTemp.day = qpi.day();
		state.qpiFunctionsOutputTemp.hour = qpi.hour();
		state.qpiFunctionsOutputTemp.minute = qpi.minute();
		state.qpiFunctionsOutputTemp.second = qpi.second();
		state.qpiFunctionsOutputTemp.millisecond = qpi.millisecond();
		state.qpiFunctionsOutputTemp.dayOfWeek = qpi.dayOfWeek(state.qpiFunctionsOutputTemp.year, state.qpiFunctionsOutputTemp.month, state.qpiFunctionsOutputTemp.day);
		state.qpiFunctionsOutputTemp.arbitrator = qpi.arbitrator();
		state.qpiFunctionsOutputTemp.computor0 = qpi.computor(0);
		state.qpiFunctionsOutputTemp.epoch = qpi.epoch();
		state.qpiFunctionsOutputTemp.invocationReward = qpi.invocationReward();
		state.qpiFunctionsOutputTemp.invocator = qpi.invocator();
		state.qpiFunctionsOutputTemp.numberOfTickTransactions = qpi.numberOfTickTransactions();
		state.qpiFunctionsOutputTemp.originator = qpi.originator();
		state.qpiFunctionsOutputTemp.tick = qpi.tick();

		state.qpiFunctionsOutputUserProc.set(state.qpiFunctionsOutputTemp.tick, state.qpiFunctionsOutputTemp); // 'set' computes index modulo array size
	_

	PUBLIC_FUNCTION(ReturnQpiFunctionsOutputBeginTick)
        if (state.qpiFunctionsOutputBeginTick.get(input.tick).tick == input.tick) // 'get' computes index modulo array size
            output.qpiFunctionsOutput = state.qpiFunctionsOutputBeginTick.get(input.tick);
	_

	PUBLIC_FUNCTION(ReturnQpiFunctionsOutputEndTick)
        if (state.qpiFunctionsOutputEndTick.get(input.tick).tick == input.tick) // 'get' computes index modulo array size
            output.qpiFunctionsOutput = state.qpiFunctionsOutputEndTick.get(input.tick);
	_

	PUBLIC_FUNCTION(ReturnQpiFunctionsOutputUserProc)
        if (state.qpiFunctionsOutputUserProc.get(input.tick).tick == input.tick) // 'get' computes index modulo array size
            output.qpiFunctionsOutput = state.qpiFunctionsOutputUserProc.get(input.tick);
	_

	typedef Array<sint8, 32 * 1024> ErrorTriggerFunction_locals;

#pragma warning(push)
#pragma warning(disable: 4717)
	PUBLIC_FUNCTION_WITH_LOCALS(ErrorTriggerFunction)
		// Recursively call itself to trigger error for testing error handling
		CALL(ErrorTriggerFunction, input, output);
	_
#pragma warning(pop)

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

	struct RunHeavyComputation_locals
	{
		id entityId;
		Entity entity;
		AssetIssuanceIterator issuanceIter;
		AssetPossessionIterator possessionIter;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(RunHeavyComputation)
		state.heavyComputationResult = input;
		
		// Iterate through spectrum
		while (!isZero(locals.entityId = qpi.nextId(locals.entityId)))
		{
			qpi.getEntity(locals.entityId, locals.entity);
			state.heavyComputationResult += locals.entity.incomingAmount;

			// Iterate through universe
			locals.issuanceIter.begin(AssetIssuanceSelect::any());
			while (!locals.issuanceIter.reachedEnd())
			{
				locals.possessionIter.begin(locals.issuanceIter.asset(), AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(locals.entityId));
				while (!locals.possessionIter.reachedEnd())
				{
					state.heavyComputationResult += locals.possessionIter.numberOfPossessedShares();
					locals.possessionIter.next();
				}

				locals.issuanceIter.next();
			}
		}
		
		output = state.heavyComputationResult;
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
		REGISTER_USER_FUNCTION(QueryQpiFunctions, 1);
		REGISTER_USER_FUNCTION(ReturnQpiFunctionsOutputBeginTick, 2);
		REGISTER_USER_FUNCTION(ReturnQpiFunctionsOutputEndTick, 3);
		REGISTER_USER_FUNCTION(ReturnQpiFunctionsOutputUserProc, 4);
		REGISTER_USER_FUNCTION(ErrorTriggerFunction, 5);

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
		REGISTER_USER_PROCEDURE(QueryQpiFunctionsToState, 7);
		REGISTER_USER_PROCEDURE(RunHeavyComputation, 8);
	_

	BEGIN_TICK
		state.qpiFunctionsOutputTemp.year = qpi.year();
		state.qpiFunctionsOutputTemp.month = qpi.month();
		state.qpiFunctionsOutputTemp.day = qpi.day();
		state.qpiFunctionsOutputTemp.hour = qpi.hour();
		state.qpiFunctionsOutputTemp.minute = qpi.minute();
		state.qpiFunctionsOutputTemp.second = qpi.second();
		state.qpiFunctionsOutputTemp.millisecond = qpi.millisecond();
		state.qpiFunctionsOutputTemp.dayOfWeek = qpi.dayOfWeek(state.qpiFunctionsOutputTemp.year, state.qpiFunctionsOutputTemp.month, state.qpiFunctionsOutputTemp.day);
		state.qpiFunctionsOutputTemp.arbitrator = qpi.arbitrator();
		state.qpiFunctionsOutputTemp.computor0 = qpi.computor(0);
		state.qpiFunctionsOutputTemp.epoch = qpi.epoch();
		state.qpiFunctionsOutputTemp.invocationReward = qpi.invocationReward();
		state.qpiFunctionsOutputTemp.invocator = qpi.invocator();
		state.qpiFunctionsOutputTemp.numberOfTickTransactions = qpi.numberOfTickTransactions();
		state.qpiFunctionsOutputTemp.originator = qpi.originator();
		state.qpiFunctionsOutputTemp.tick = qpi.tick();

		state.qpiFunctionsOutputBeginTick.set(state.qpiFunctionsOutputTemp.tick, state.qpiFunctionsOutputTemp); // 'set' computes index modulo array size
	_

	END_TICK
		state.qpiFunctionsOutputTemp.year = qpi.year();
		state.qpiFunctionsOutputTemp.month = qpi.month();
		state.qpiFunctionsOutputTemp.day = qpi.day();
		state.qpiFunctionsOutputTemp.hour = qpi.hour();
		state.qpiFunctionsOutputTemp.minute = qpi.minute();
		state.qpiFunctionsOutputTemp.second = qpi.second();
		state.qpiFunctionsOutputTemp.millisecond = qpi.millisecond();
		state.qpiFunctionsOutputTemp.dayOfWeek = qpi.dayOfWeek(state.qpiFunctionsOutputTemp.year, state.qpiFunctionsOutputTemp.month, state.qpiFunctionsOutputTemp.day);
		state.qpiFunctionsOutputTemp.arbitrator = qpi.arbitrator();
		state.qpiFunctionsOutputTemp.computor0 = qpi.computor(0);
		state.qpiFunctionsOutputTemp.epoch = qpi.epoch();
		state.qpiFunctionsOutputTemp.invocationReward = qpi.invocationReward();
		state.qpiFunctionsOutputTemp.invocator = qpi.invocator();
		state.qpiFunctionsOutputTemp.numberOfTickTransactions = qpi.numberOfTickTransactions();
		state.qpiFunctionsOutputTemp.originator = qpi.originator();
		state.qpiFunctionsOutputTemp.tick = qpi.tick();

		state.qpiFunctionsOutputEndTick.set(state.qpiFunctionsOutputTemp.tick, state.qpiFunctionsOutputTemp); // 'set' computes index modulo array size
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
