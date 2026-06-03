using namespace QPI;

// QASSAND is the Qubic contract implementation name for the Qassandra protocol v0.
constexpr uint16 QASSAND_VERSION = 0;
constexpr uint16 QASSAND_CONSTRUCTION_EPOCH_PLACEHOLDER = 10000;
constexpr sint64 QASSAND_PING_FEE = 100000;
constexpr sint64 QASSAND_PROTOCOL_FEE = 75000;
constexpr sint64 QASSAND_BURN_FEE = 25000;
constexpr uint8 QASSAND_SUCCESS = 0;
constexpr uint8 QASSAND_UNDERPAID = 1;
constexpr uint8 QASSAND_UNKNOWN_LANE = 2;
constexpr uint16 QASSAND_LANE_UNKNOWN = 0;
constexpr uint16 QASSAND_LANE_FORECASTING = 1;
constexpr uint16 QASSAND_LANE_STABLE_OPERATIONS = 2;
constexpr uint16 QASSAND_LANE_DATA_ATTESTATION = 3;

struct QASSAND2
{
};

struct QASSAND : public ContractBase
{
	struct StateData
	{
		uint16 version;
		uint16 constructionEpoch;
		sint64 pingFee;
		sint64 protocolFee;
		sint64 burnFee;
		uint64 totalPingCount;
		sint64 protocolEarnedFee;
		sint64 burnEarnedFee;
		sint64 pendingBurnAmount;
		sint64 totalBurnedAmount;
	};

	struct Ping_input
	{
	};

	struct Ping_output
	{
		uint8 returnCode;
		sint64 acceptedFee;
		sint64 refundedAmount;
		sint64 protocolEarnedFee;
		sint64 burnEarnedFee;
		uint64 totalPingCount;
	};

	struct Ping_locals
	{
		sint64 paidAmount;
		sint64 refundAmount;
	};

	struct GetInfo_input
	{
	};

	struct GetInfo_output
	{
		Array<uint8, 16> protocolName;
		uint16 version;
		uint16 constructionEpoch;
		uint64 totalPingCount;
	};

	struct GetFeeInfo_input
	{
	};

	struct GetFeeInfo_output
	{
		sint64 pingFee;
		sint64 protocolFee;
		sint64 burnFee;
		sint64 protocolEarnedFee;
		sint64 burnEarnedFee;
	};

	struct GetBurnInfo_input
	{
	};

	struct GetBurnInfo_output
	{
		sint64 pendingBurnAmount;
		sint64 totalBurnedAmount;
	};

	struct GetLaneInfo_input
	{
		uint16 laneId;
	};

	struct GetLaneInfo_output
	{
		uint8 returnCode;
		uint16 laneId;
		Array<uint8, 16> laneName;
		sint64 requiredFee;
	};

	struct END_TICK_locals
	{
		sint64 burnAmount;
	};

	INITIALIZE()
	{
		state.mut().version = QASSAND_VERSION;
		state.mut().constructionEpoch = QASSAND_CONSTRUCTION_EPOCH_PLACEHOLDER;
		state.mut().pingFee = QASSAND_PING_FEE;
		state.mut().protocolFee = QASSAND_PROTOCOL_FEE;
		state.mut().burnFee = QASSAND_BURN_FEE;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(Ping, 1);
		REGISTER_USER_FUNCTION(GetInfo, 1);
		REGISTER_USER_FUNCTION(GetFeeInfo, 2);
		REGISTER_USER_FUNCTION(GetBurnInfo, 3);
		REGISTER_USER_FUNCTION(GetLaneInfo, 4);
	}

	END_TICK_WITH_LOCALS()
	{
		locals.burnAmount = state.get().pendingBurnAmount;
		if (locals.burnAmount <= 0)
		{
			return;
		}

		qpi.burn(locals.burnAmount);
		state.mut().pendingBurnAmount = 0;
		state.mut().totalBurnedAmount += locals.burnAmount;
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(Ping)
	{
		locals.paidAmount = qpi.invocationReward();

		if (locals.paidAmount < state.get().pingFee)
		{
			if (locals.paidAmount > 0)
			{
				qpi.transfer(qpi.invocator(), locals.paidAmount);
			}
			output.returnCode = QASSAND_UNDERPAID;
			output.refundedAmount = locals.paidAmount;
			return;
		}

		locals.refundAmount = locals.paidAmount - state.get().pingFee;
		if (locals.refundAmount > 0)
		{
			qpi.transfer(qpi.invocator(), locals.refundAmount);
		}

		state.mut().totalPingCount += 1;
		state.mut().protocolEarnedFee += state.get().protocolFee;
		state.mut().burnEarnedFee += state.get().burnFee;
		state.mut().pendingBurnAmount += state.get().burnFee;

		output.returnCode = QASSAND_SUCCESS;
		output.acceptedFee = state.get().pingFee;
		output.refundedAmount = locals.refundAmount;
		output.protocolEarnedFee = state.get().protocolFee;
		output.burnEarnedFee = state.get().burnFee;
		output.totalPingCount = state.get().totalPingCount;
	}

	PUBLIC_FUNCTION(GetInfo)
	{
		output.protocolName.set(0, 81);
		output.protocolName.set(1, 97);
		output.protocolName.set(2, 115);
		output.protocolName.set(3, 115);
		output.protocolName.set(4, 97);
		output.protocolName.set(5, 110);
		output.protocolName.set(6, 100);
		output.protocolName.set(7, 114);
		output.protocolName.set(8, 97);
		output.version = state.get().version;
		output.constructionEpoch = state.get().constructionEpoch;
		output.totalPingCount = state.get().totalPingCount;
	}

	PUBLIC_FUNCTION(GetFeeInfo)
	{
		output.pingFee = state.get().pingFee;
		output.protocolFee = state.get().protocolFee;
		output.burnFee = state.get().burnFee;
		output.protocolEarnedFee = state.get().protocolEarnedFee;
		output.burnEarnedFee = state.get().burnEarnedFee;
	}

	PUBLIC_FUNCTION(GetBurnInfo)
	{
		output.pendingBurnAmount = state.get().pendingBurnAmount;
		output.totalBurnedAmount = state.get().totalBurnedAmount;
	}

	PUBLIC_FUNCTION(GetLaneInfo)
	{
		if (input.laneId == QASSAND_LANE_FORECASTING)
		{
			output.returnCode = QASSAND_SUCCESS;
			output.laneId = QASSAND_LANE_FORECASTING;
			output.laneName.set(0, 70);
			output.laneName.set(1, 111);
			output.laneName.set(2, 114);
			output.laneName.set(3, 101);
			output.laneName.set(4, 99);
			output.laneName.set(5, 97);
			output.laneName.set(6, 115);
			output.laneName.set(7, 116);
			output.laneName.set(8, 105);
			output.laneName.set(9, 110);
			output.laneName.set(10, 103);
			output.requiredFee = 0;
			return;
		}

		if (input.laneId == QASSAND_LANE_STABLE_OPERATIONS)
		{
			output.returnCode = QASSAND_SUCCESS;
			output.laneId = QASSAND_LANE_STABLE_OPERATIONS;
			output.laneName.set(0, 83);
			output.laneName.set(1, 116);
			output.laneName.set(2, 97);
			output.laneName.set(3, 98);
			output.laneName.set(4, 108);
			output.laneName.set(5, 101);
			output.laneName.set(6, 79);
			output.laneName.set(7, 112);
			output.laneName.set(8, 115);
			output.requiredFee = 0;
			return;
		}

		if (input.laneId == QASSAND_LANE_DATA_ATTESTATION)
		{
			output.returnCode = QASSAND_SUCCESS;
			output.laneId = QASSAND_LANE_DATA_ATTESTATION;
			output.laneName.set(0, 68);
			output.laneName.set(1, 97);
			output.laneName.set(2, 116);
			output.laneName.set(3, 97);
			output.requiredFee = 0;
			return;
		}

		output.returnCode = QASSAND_UNKNOWN_LANE;
	}
};
