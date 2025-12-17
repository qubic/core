using namespace QPI;

constexpr uint64 TESTEXB_ASSET_NAME = 18674403253634388;

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
	{
		output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
	}

	PUBLIC_PROCEDURE(TransferShareOwnershipAndPossession)
	{
		if (qpi.transferShareOwnershipAndPossession(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newOwnerAndPossessor) > 0)
		{
			output.transferredNumberOfShares = input.numberOfShares;
		}
	}

	PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
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
	}

	PUBLIC_PROCEDURE(SetPreReleaseSharesOutput)
	{
		state.preReleaseSharesOutput = input;
	}

	PUBLIC_PROCEDURE(SetPreAcquireSharesOutput)
	{
		state.preAcquireSharesOutput = input;
	}

	PUBLIC_PROCEDURE(AcquireShareManagementRights)
	{
		if (qpi.acquireShares(input.asset, input.ownerAndPossessor, input.ownerAndPossessor, input.numberOfShares,
			input.oldManagingContractIndex, input.oldManagingContractIndex, qpi.invocationReward()) >= 0)
		{
			// success
			output.transferredNumberOfShares = input.numberOfShares;
		}
    }

	struct GetTestExampleAShareManagementRights_locals
	{
		TESTEXA::TransferShareManagementRights_input input;
		TESTEXA::TransferShareManagementRights_output output;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(GetTestExampleAShareManagementRights)
	{
		locals.input.asset = input.asset;
		locals.input.numberOfShares = input.numberOfShares;
		locals.input.newManagingContractIndex = SELF_INDEX;
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXA, TransferShareManagementRights, locals.input, locals.output, qpi.invocationReward());
		output.transferredNumberOfShares = locals.output.transferredNumberOfShares;
	}

	PRE_RELEASE_SHARES()
	{
		// check that qpi.acquireShares() leading to this callback is triggered by owner,
		// otherwise allowing another contract to acquire management rights is risky
		if (qpi.originator() == input.owner)
		{
			output = state.preReleaseSharesOutput;
		}
		state.prevPreReleaseSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	POST_RELEASE_SHARES()
	{
		state.postReleaseSharesCounter++;
		state.prevPostReleaseSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	PRE_ACQUIRE_SHARES()
	{
		output = state.preAcquireSharesOutput;
		state.prevPreAcquireSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	POST_ACQUIRE_SHARES()
	{
		state.postAcquireShareCounter++;
		state.prevPostAcquireSharesInput = input;

		ASSERT(qpi.invocator().u64._0 == input.otherContractIndex);

		// calling qpi.releaseShares() and qpi.acquireShares() is forbidden in *_SHARES callbacks
		// and should return with an error immediately
		ASSERT(qpi.releaseShares(input.asset, input.owner, input.possessor, input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
		ASSERT(qpi.acquireShares(input.asset, input.owner, qpi.invocator(), input.numberOfShares,
			input.otherContractIndex, input.otherContractIndex, qpi.invocationReward()) == INVALID_AMOUNT);
	}

	//---------------------------------------------------------------
	// CONTRACT INTERACTION / RESOLVING DEADLOCKS

public:
	typedef TESTEXA::QueryQpiFunctions_input CallFunctionOfTestExampleA_input;
	typedef TESTEXA::QueryQpiFunctions_output CallFunctionOfTestExampleA_output;

	PUBLIC_FUNCTION(CallFunctionOfTestExampleA)
	{
#ifdef NO_UEFI
		printf("Before wait/deadlock in contract %u function\n", CONTRACT_INDEX);
#endif
		CALL_OTHER_CONTRACT_FUNCTION(TESTEXA, QueryQpiFunctions, input, output);
#ifdef NO_UEFI
		printf("After wait/deadlock in contract %u function\n", CONTRACT_INDEX);
#endif
	}

	//---------------------------------------------------------------
	// POST_INCOMING_TRANSFER CALLBACK
public:
	typedef NoData IncomingTransferAmounts_input;
	struct IncomingTransferAmounts_output
	{
		sint64 standardTransactionAmount;
		sint64 procedureTransactionAmount;
		sint64 qpiTransferAmount;
		sint64 qpiDistributeDividendsAmount;
		sint64 revenueDonationAmount;
		sint64 ipoBidRefundAmount;
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
	{
		output = state.incomingTransfers;
	}

	struct POST_INCOMING_TRANSFER_locals
	{
		Entity before;
		Entity after;
	};

	POST_INCOMING_TRANSFER_WITH_LOCALS()
	{
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
	}

	PUBLIC_PROCEDURE(QpiTransfer)
	{
		qpi.transfer(input.destinationPublicKey, input.amount);
	}

	PUBLIC_PROCEDURE(QpiDistributeDividends)
	{
		qpi.distributeDividends(input.amountPerShare);
	}

	//---------------------------------------------------------------
	// IPO TEST
public:
	struct GetIpoBid_input
	{
		uint32 ipoContractIndex;
		uint32 bidIndex;
	};
	struct GetIpoBid_output
	{
		id publicKey;
		sint64 price;
	};

	struct QpiBidInIpo_input
	{
		uint32 ipoContractIndex;
		sint64 pricePerShare;
		uint16 numberOfShares;
	};
	typedef sint64 QpiBidInIpo_output;

	PUBLIC_FUNCTION(GetIpoBid)
	{
		output.price = qpi.ipoBidPrice(input.ipoContractIndex, input.bidIndex);
		output.publicKey = qpi.ipoBidId(input.ipoContractIndex, input.bidIndex);
	}

	PUBLIC_PROCEDURE(QpiBidInIpo)
	{
		output = qpi.bidInIPO(input.ipoContractIndex, input.pricePerShare, input.numberOfShares);
	}

	//---------------------------------------------------------------
	// SHAREHOLDER PROPOSALS WITH MULTI-OPTION + SCALAR STORAGE

protected:
	// Variables that can be set with proposals
	sint64 fee1;
	sint64 fee2;
	sint64 fee3;

public:
	// Proposal data type. Support up to 8 options and scalar voting.
	typedef ProposalDataV1<true> ProposalDataT;

	// Shareholders of TESTEXA have right to propose and vote. Only 16 slots provided.
	typedef ProposalAndVotingByShareholders<16, TESTEXB_ASSET_NAME> ProposersAndVotersT;

	// Proposal and voting storage type
	typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT;

protected:
	// Proposal storage
	ProposalVotingT proposals;
public:

	struct SetShareholderProposal_input
	{
		ProposalDataT proposalData;
	};
	typedef QPI::SET_SHAREHOLDER_PROPOSAL_output SetShareholderProposal_output;

	struct SetShareholderProposal_locals
	{
		uint16 optionCount;
		uint16 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SetShareholderProposal)
	{
		// - fee can be handled as you like
		// - input.proposalData.epoch == 0 means clearing a proposal

		// default return code: failure
		output = INVALID_PROPOSAL_INDEX;

		// custom checks
		if (input.proposalData.epoch != 0)
		{
			switch (ProposalTypes::cls(input.proposalData.type))
			{
			case ProposalTypes::Class::Variable:
				// check that variable index is in valid range
				if (input.proposalData.variableOptions.variable >= 3)
					return;

				// check that proposed value is in valid range
				// (in this example, it is independent of the variable index; all fees must be positive)
				locals.optionCount = ProposalTypes::optionCount(input.proposalData.type);
				if (locals.optionCount == 0)
				{
					// votes are scalar values
					if (input.proposalData.variableScalar.minValue < 0
						|| input.proposalData.variableScalar.maxValue < 0
						|| input.proposalData.variableScalar.proposedValue < 0)
						return;
				}
				else
				{
					// votes are option indices (option 0 is no change, value i is option i + 1)
					for (locals.i = 0; locals.i < locals.optionCount - 1; ++locals.i)
						if (input.proposalData.variableOptions.values.get(locals.i) < 0)
							return;
				}

				break;

			default:
				// this forbids all other proposals including transfers, multi-variable, general, and all future propsasl classes
				return;
			}
		}

		// Try to set proposal (checks invocator's rights and general validity of input proposal), returns proposal index
		output = qpi(state.proposals).setProposal(qpi.invocator(), input.proposalData);
	}




	struct FinalizeShareholderProposalSetStateVar_input
	{
		sint32 proposalIndex;
		ProposalDataT proposal;
		ProposalSummarizedVotingDataV1 results;
		sint32 acceptedOption;
		sint64 acceptedValue;
	};
	typedef NoData FinalizeShareholderProposalSetStateVar_output;

	PRIVATE_PROCEDURE(FinalizeShareholderProposalSetStateVar)
	{
		if (input.proposal.variableOptions.variable == 0)
			state.fee1 = input.acceptedValue;
		else if (input.proposal.variableOptions.variable == 1)
			state.fee2 = input.acceptedValue;
		else if (input.proposal.variableOptions.variable == 2)
			state.fee3 = input.acceptedValue;
	}

	typedef NoData FinalizeShareholderStateVarProposals_input;
	typedef NoData FinalizeShareholderStateVarProposals_output;
	struct FinalizeShareholderStateVarProposals_locals
	{
		FinalizeShareholderProposalSetStateVar_input p;
		uint16 proposalClass;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeShareholderStateVarProposals)
	{
		// Analyze proposal results and set variables:
		// Iterate all proposals that were open for voting in this epoch ...
		locals.p.proposalIndex = -1;
		while ((locals.p.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.p.proposalIndex, qpi.epoch())) >= 0)
		{
			if (!qpi(state.proposals).getProposal(locals.p.proposalIndex, locals.p.proposal))
				continue;
			
			locals.proposalClass = ProposalTypes::cls(locals.p.proposal.type);

			// Handle proposal type Variable / MultiVariables
			if (locals.proposalClass == ProposalTypes::Class::Variable || locals.proposalClass == ProposalTypes::Class::MultiVariables)
			{
				// Get voting results and check if conditions for proposal acceptance are met
				if (!qpi(state.proposals).getVotingSummary(locals.p.proposalIndex, locals.p.results))
					continue;

				if (locals.p.proposal.type == ProposalTypes::VariableScalarMean)
				{
					if (locals.p.results.totalVotesCasted < QUORUM)
						continue;

					locals.p.acceptedValue = locals.p.results.scalarVotingResult;
				}
				else
				{
					locals.p.acceptedOption = locals.p.results.getAcceptedOption();
					if (locals.p.acceptedOption <= 0)
						continue;

					// option 0 is "no change", option 1 has index 0 in variableOptions
					locals.p.acceptedValue = locals.p.proposal.variableOptions.values.get(locals.p.acceptedOption - 1);
				}

				CALL(FinalizeShareholderProposalSetStateVar, locals.p, output);
			}
		}
	}

	END_EPOCH()
	{
		CALL(FinalizeShareholderStateVarProposals, input, output);
	}


	IMPLEMENT_GetShareholderProposalFees(0)
	IMPLEMENT_GetShareholderProposal()
	IMPLEMENT_GetShareholderProposalIndices()
	IMPLEMENT_SetShareholderVotes()
	IMPLEMENT_GetShareholderVotes()
	IMPLEMENT_GetShareholderVotingResults()
	IMPLEMENT_SET_SHAREHOLDER_PROPOSAL()
	IMPLEMENT_SET_SHAREHOLDER_VOTES()

public:
	struct SetProposalInOtherContractAsShareholder_input
	{
		Array<uint8, 512> proposalDataBuffer;
		uint16 otherContractIndex;
	};
	struct SetProposalInOtherContractAsShareholder_output
	{
		uint16 proposalIndex;
	};
	struct SetProposalInOtherContractAsShareholder_locals
	{
		Array<uint8, 1024> proposalDataBuffer;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(SetProposalInOtherContractAsShareholder)
	{
		// User procedure for letting TESTEXB create a shareholder proposal in TESTEXA as shareholder of TESTEXA.
		// Skipped here: checking that invocator has right to set proposal for this contract (e.g., is contract "admin")
		copyToBuffer(locals.proposalDataBuffer, input.proposalDataBuffer);
		output.proposalIndex = qpi.setShareholderProposal(input.otherContractIndex, locals.proposalDataBuffer, qpi.invocationReward());
	}

	struct SetVotesInOtherContractAsShareholder_input
	{
		ProposalMultiVoteDataV1 voteData;
		uint16 otherContractIndex;
	};
	struct SetVotesInOtherContractAsShareholder_output
	{
		bit success;
	};

	PUBLIC_PROCEDURE(SetVotesInOtherContractAsShareholder)
	{
		// User procedure for letting TESTEXB cast shareholder votes in TESTEXA as shareholder of TESTEXA.
		// Skipped here: checking that invocator has right to cast votes for this contract (e.g., is contract "admin")
		output.success = qpi.setShareholderVotes(input.otherContractIndex, input.voteData, qpi.invocationReward());
	}

	// Test inter-contract call error handling
	struct TestInterContractCallError_input
	{
		uint8 dummy; // Dummy field to avoid zero-size struct
	};

	struct TestInterContractCallError_output
	{
		uint8 errorCode;
		uint8 callSucceeded; // 1 if call happened, 0 if it was skipped
	};

	struct TestInterContractCallError_locals
	{
		TESTEXA::QueryQpiFunctionsToState_input procInput;
		TESTEXA::QueryQpiFunctionsToState_output procOutput;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TestInterContractCallError)
	{
		// Try to invoke a procedure in TestExampleA
		// This will fail if TestExampleA has insufficient fees
		INVOKE_OTHER_CONTRACT_PROCEDURE(TESTEXA, QueryQpiFunctionsToState, locals.procInput, locals.procOutput, 0);

		// interContractCallError is now available from the macro
		output.errorCode = interContractCallError;
		output.callSucceeded = (interContractCallError == NoCallError) ? 1 : 0;
	}

	//---------------------------------------------------------------
	// COMMON PARTS

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(CallFunctionOfTestExampleA, 1);

		REGISTER_USER_FUNCTION(IncomingTransferAmounts, 20);
		REGISTER_USER_FUNCTION(GetIpoBid, 30);

		REGISTER_USER_PROCEDURE(IssueAsset, 1);
		REGISTER_USER_PROCEDURE(TransferShareOwnershipAndPossession, 2);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
		REGISTER_USER_PROCEDURE(SetPreReleaseSharesOutput, 4);
		REGISTER_USER_PROCEDURE(SetPreAcquireSharesOutput, 5);
		REGISTER_USER_PROCEDURE(AcquireShareManagementRights, 6);
		REGISTER_USER_PROCEDURE(GetTestExampleAShareManagementRights, 7);

		REGISTER_USER_PROCEDURE(QpiTransfer, 20);
		REGISTER_USER_PROCEDURE(QpiDistributeDividends, 21);
		REGISTER_USER_PROCEDURE(QpiBidInIpo, 30);
		REGISTER_USER_PROCEDURE(SetProposalInOtherContractAsShareholder, 40);
		REGISTER_USER_PROCEDURE(SetVotesInOtherContractAsShareholder, 41);
		REGISTER_USER_PROCEDURE(TestInterContractCallError, 50);

		REGISTER_SHAREHOLDER_PROPOSAL_VOTING();
	}
};
