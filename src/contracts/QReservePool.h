using namespace QPI;

// Number of available smart contracts in the QRP contract.
constexpr uint16 QRP_ALLOWED_SC_NUM = 128;
constexpr uint64 QRP_QTF_INDEX = QRP_CONTRACT_INDEX + 1;
constexpr uint64 QRP_REMOVAL_THRESHOLD_PERCENT = 75;

struct QRP2
{
};

struct QRP : ContractBase
{
	enum class EReturnCode : uint8
	{
		SUCCESS = 0,
		ACCESS_DENIED = 1,
		INSUFFICIENT_RESERVE = 2,

		MAX_VALUE = UINT8_MAX
	};

	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); };

	// Withdraw Reserve
	struct WithdrawReserve_input
	{
		uint64 revenue;
	};

	struct WithdrawReserve_output
	{
		// How much revenue is allocated to SC
		uint64 allocatedRevenue;
		uint8 returnCode;
	};

	struct WithdrawReserve_locals
	{
		Entity entity;
		uint64 checkAmount;
	};

	// Add Allowed Smart Contract
	struct AddAllowedSC_input
	{
		uint64 scIndex;
	};

	struct AddAllowedSC_output
	{
		uint8 returnCode;
	};

	// Remove Allowed Smart Contract
	struct RemoveAllowedSC_input
	{
		uint64 scIndex;
	};

	struct RemoveAllowedSC_output
	{
		uint8 returnCode;
	};

	// Get Available Reserve
	struct GetAvailableReserve_input
	{
	};

	struct GetAvailableReserve_output
	{
		uint64 availableReserve;
	};

	struct GetAvailableReserve_locals
	{
		Entity entity;
	};

	// Get Allowed Smart Contract
	struct GetAllowedSC_input
	{
	};

	struct GetAllowedSC_output
	{
		Array<id, QRP_ALLOWED_SC_NUM> allowedSC;
	};

	struct GetAllowedSC_locals
	{
		sint64 nextIndex;
		uint64 arrayIndex;
	};

	INITIALIZE()
	{
		// Set team/developer address (owner and team are the same for now)
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);
		state.ownerAddress = state.teamAddress;

		// Adds QTF to the list of allowed smart contracts.
		state.allowedSmartContracts.add(id(QRP_QTF_INDEX, 0, 0, 0));
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// Procedures
		REGISTER_USER_PROCEDURE(WithdrawReserve, 1);
		REGISTER_USER_PROCEDURE(AddAllowedSC, 2);
		REGISTER_USER_PROCEDURE(RemoveAllowedSC, 3);
		// Functions
		REGISTER_USER_FUNCTION(GetAvailableReserve, 1);
		REGISTER_USER_FUNCTION(GetAllowedSC, 2);
	}

	END_EPOCH() { state.allowedSmartContracts.cleanup(); }

	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawReserve)
	{
		if (!state.allowedSmartContracts.contains(qpi.invocator()))
		{
			output.allocatedRevenue = 0;
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		qpi.getEntity(SELF, locals.entity);
		locals.checkAmount = RL::max(locals.entity.incomingAmount - locals.entity.outgoingAmount, 0i64);
		if (locals.checkAmount == 0 || input.revenue > locals.checkAmount)
		{
			output.allocatedRevenue = 0;
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_RESERVE);
			return;
		}

		output.allocatedRevenue = input.revenue;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);

		qpi.transfer(qpi.invocator(), output.allocatedRevenue);
	}

	PUBLIC_PROCEDURE(AddAllowedSC)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		state.allowedSmartContracts.add(id(input.scIndex, 0, 0, 0));
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(RemoveAllowedSC)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		state.allowedSmartContracts.remove(id(input.scIndex, 0, 0, 0));
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);

		state.allowedSmartContracts.cleanupIfNeeded(QRP_REMOVAL_THRESHOLD_PERCENT);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetAvailableReserve)
	{
		qpi.getEntity(SELF, locals.entity);
		output.availableReserve = RL::max(locals.entity.incomingAmount - locals.entity.outgoingAmount, 0i64);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetAllowedSC)
	{
		locals.arrayIndex = 0;
		locals.nextIndex = -1;

		locals.nextIndex = state.allowedSmartContracts.nextElementIndex(locals.nextIndex);
		while (locals.nextIndex != NULL_INDEX)
		{
			output.allowedSC.set(locals.arrayIndex++, state.allowedSmartContracts.key(locals.nextIndex));
			locals.nextIndex = state.allowedSmartContracts.nextElementIndex(locals.nextIndex);
		}
	}

protected:
	/**
	 * @brief Address of the team managing the lottery contract.
	 * Initialized to a zero address.
	 */
	id teamAddress;

	/**
	 * @brief Address of the owner of the lottery contract.
	 * Initialized to a zero address.
	 */
	id ownerAddress;

	HashSet<id, QRP_ALLOWED_SC_NUM> allowedSmartContracts;
};
