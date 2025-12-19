using namespace QPI;

// Number of available smart contracts in the QRP contract.
static constexpr uint16 QRP_AVAILABLE_SC_NUM = 128;
static constexpr uint64 QRP_QTF_INDEX = 21;

struct QRP2
{
};

struct QRP : public ContractBase
{
public:
	enum class EReturnCode : uint8
	{
		SUCCESS = 0,
		ACCESS_DENIED = 1,
		INSUFFICIENT_RESERVE = 2,

		MAX_VALUE = UINT8_MAX
	};

	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); };

public:
	// Get Reserve
	struct GetReserve_input
	{
		uint64 revenue;
	};

	struct GetReserve_output
	{
		// How much revenue is allocated to SC
		uint64 allocatedRevenue;
		uint8 returnCode;
	};

	struct GetReserve_locals
	{
		Entity entity;
		uint64 checkAmount;
	};

	// Add Available Smart Contract
	struct AddAvailableSC_input
	{
		uint64 scIndex;
	};

	struct AddAvailableSC_output
	{
		uint8 returnCode;
	};

	// Remove Available Smart Contract
	struct RemoveAvailableSC_input
	{
		uint64 scIndex;
	};

	struct RemoveAvailableSC_output
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

	// Get Available Smart Contract
	struct GetAvailableSC_input
	{
	};

	struct GetAvailableSC_output
	{
		Array<id, QRP_AVAILABLE_SC_NUM> availableSCs;
	};

	struct GetAvailableSC_locals
	{
		sint64 nextIndex;
		uint64 arrayIndex;
	};

public:
	INITIALIZE()
	{
		// Set team/developer address (owner and team are the same for now)
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);
		state.ownerAddress = state.teamAddress;

		// Adds QTF to the list of available smart contracts.
		state.availableSmartContracts.add(id(QRP_QTF_INDEX, 0, 0, 0));
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// Procedures
		REGISTER_USER_PROCEDURE(GetReserve, 1);
		REGISTER_USER_PROCEDURE(AddAvailableSC, 2);
		REGISTER_USER_PROCEDURE(RemoveAvailableSC, 3);
		// Functions
		REGISTER_USER_FUNCTION(GetAvailableReserve, 1);
		REGISTER_USER_FUNCTION(GetAvailableSC, 2);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(GetReserve)
	{
		if (!state.availableSmartContracts.contains(qpi.invocator()))
		{
			output.allocatedRevenue = 0;
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		qpi.getEntity(SELF, locals.entity);
		locals.checkAmount = max(locals.entity.incomingAmount - locals.entity.outgoingAmount, 0i64);
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

	PUBLIC_PROCEDURE(AddAvailableSC)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		state.availableSmartContracts.add(id(input.scIndex, 0, 0, 0));
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(RemoveAvailableSC)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		state.availableSmartContracts.remove(id(input.scIndex, 0, 0, 0));
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetAvailableReserve)
	{
		qpi.getEntity(SELF, locals.entity);
		output.availableReserve = max(locals.entity.incomingAmount - locals.entity.outgoingAmount, 0i64);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetAvailableSC)
	{
		locals.arrayIndex = 0;
		locals.nextIndex = -1;

		locals.nextIndex = state.availableSmartContracts.nextElementIndex(locals.nextIndex);
		while (locals.nextIndex != NULL_INDEX)
		{
			output.availableSCs.set(locals.arrayIndex++, state.availableSmartContracts.key(locals.nextIndex));
			locals.nextIndex = state.availableSmartContracts.nextElementIndex(locals.nextIndex);
		}
	}

protected:
	template<typename T> static constexpr const T& max(const T& a, const T& b) { return (a > b) ? a : b; }

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

	HashSet<id, QRP_AVAILABLE_SC_NUM> availableSmartContracts;
};
