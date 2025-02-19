using namespace QPI;

struct TESTEXA2
{
};

struct TESTEXA : public ContractBase
{
	struct QpiFunctionsOutput
	{
		uint8 year;   // [0..99] (0 = 2000, 1 = 2001, ..., 99 = 2099)
		uint8 month;  // [1..12]
		uint8 day;    // [1..31]
		uint8 hour;   // [0..23]
		uint8 minute; // [0..59]
		uint8 second;
		uint16 millisecond;
		uint8 dayOfWeek;
		id arbitrator;
		id computor0;
		uint16 epoch;
		sint64 invocationReward;
		id invocator;
		sint32 numberOfTickTransactions;
		id originator;
		uint32 tick;
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

	uint32 iterationIndex;
	QpiFunctionsOutput qpiFunctionsOutputTemp;
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputBeginTick; // Output of QPI functions queried by the BEGIN_TICK procedure for the last 16 ticks
	uint8 qpiFunctionsOutputBeginTickNextIndex; // Next index to write to in qpiFunctionsOutputBeginTick
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputEndTick; // Output of QPI functions queried by the END_TICK procedure for the last 16 ticks
	uint8 qpiFunctionsOutputEndTickNextIndex; // Next index to write to in qpiFunctionsOutputEndTick
	Array<QpiFunctionsOutput, 16> qpiFunctionsOutputUserProc; // Output of QPI functions queried by the END_TICK procedure for the last 16 ticks
	uint8 qpiFunctionsOutputUserProcNextIndex; // Next index to write to in qpiFunctionsOutputUserProc
	
	PreManagementRightsTransfer_output preReleaseSharesOutput;
	PreManagementRightsTransfer_output preAcquireSharesOutput;

	PreManagementRightsTransfer_input prevPreReleaseSharesInput;
	PreManagementRightsTransfer_input prevPreAcquireSharesInput;
	PostManagementRightsTransfer_input prevPostReleaseSharesInput;
	PostManagementRightsTransfer_input prevPostAcquireSharesInput;
	uint32 postReleaseSharesCounter;
	uint32 postAcquireShareCounter;

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

		state.qpiFunctionsOutputUserProc.set(state.qpiFunctionsOutputUserProcNextIndex, state.qpiFunctionsOutputTemp);
		state.qpiFunctionsOutputUserProcNextIndex = mod(state.qpiFunctionsOutputUserProcNextIndex + 1, 16);
	_

	PUBLIC_PROCEDURE(ReturnQpiFunctionsOutputBeginTick)
		for (state.iterationIndex = 0; state.iterationIndex < 16; ++state.iterationIndex)
		{
			if (state.qpiFunctionsOutputBeginTick.get(state.iterationIndex).tick == input.tick)
			{
				output.qpiFunctionsOutput = state.qpiFunctionsOutputBeginTick.get(state.iterationIndex);
				break;
			}
		}
	_

	PUBLIC_PROCEDURE(ReturnQpiFunctionsOutputEndTick)
		for (state.iterationIndex = 0; state.iterationIndex < 16; ++state.iterationIndex)
		{
			if (state.qpiFunctionsOutputEndTick.get(state.iterationIndex).tick == input.tick)
			{
				output.qpiFunctionsOutput = state.qpiFunctionsOutputEndTick.get(state.iterationIndex);
				break;
			}
		}
	_

	PUBLIC_PROCEDURE(ReturnQpiFunctionsOutputUserProc)
		for (state.iterationIndex = 0; state.iterationIndex < 16; ++state.iterationIndex)
		{
			if (state.qpiFunctionsOutputUserProc.get(state.iterationIndex).tick == input.tick)
			{
				output.qpiFunctionsOutput = state.qpiFunctionsOutputUserProc.get(state.iterationIndex);
				break;
			}
		}
	_

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
		REGISTER_USER_FUNCTION(QueryQpiFunctions, 1);

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
		REGISTER_USER_PROCEDURE(QueryQpiFunctionsToState, 7);
		REGISTER_USER_PROCEDURE(ReturnQpiFunctionsOutputBeginTick, 8);
		REGISTER_USER_PROCEDURE(ReturnQpiFunctionsOutputEndTick, 9);
		REGISTER_USER_PROCEDURE(ReturnQpiFunctionsOutputUserProc, 10);
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

		state.qpiFunctionsOutputBeginTick.set(state.qpiFunctionsOutputBeginTickNextIndex, state.qpiFunctionsOutputTemp);
		state.qpiFunctionsOutputBeginTickNextIndex = mod(state.qpiFunctionsOutputBeginTickNextIndex + 1, 16);
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

		state.qpiFunctionsOutputEndTick.set(state.qpiFunctionsOutputEndTickNextIndex, state.qpiFunctionsOutputTemp);
		state.qpiFunctionsOutputEndTickNextIndex = mod(state.qpiFunctionsOutputEndTickNextIndex + 1, 16);
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
