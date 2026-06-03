#define NO_UEFI

#include "contract_testing.h"

constexpr uint16 QASSAND_PROC_PING = 1;
constexpr uint16 QASSAND_FUNC_GET_INFO = 1;
constexpr uint16 QASSAND_FUNC_GET_FEE_INFO = 2;
constexpr uint16 QASSAND_FUNC_GET_BURN_INFO = 3;
constexpr uint16 QASSAND_FUNC_GET_LANE_INFO = 4;

static const id QASSAND_CONTRACT_ID(QASSAND_CONTRACT_INDEX, 0, 0, 0);

class QassandChecker : public QASSAND, public QASSAND::StateData
{
public:
	uint16 versionValue() const { return version; }
	uint16 constructionEpochValue() const { return constructionEpoch; }
	uint64 totalPingCountValue() const { return totalPingCount; }
	sint64 protocolEarnedFeeValue() const { return protocolEarnedFee; }
	sint64 burnEarnedFeeValue() const { return burnEarnedFee; }
	sint64 pendingBurnAmountValue() const { return pendingBurnAmount; }
	sint64 totalBurnedAmountValue() const { return totalBurnedAmount; }
};

class ContractTestingQassand : protected ContractTesting
{
public:
	ContractTestingQassand()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QASSAND);
		callSystemProcedure(QASSAND_CONTRACT_INDEX, INITIALIZE);
	}

	QassandChecker* state() { return reinterpret_cast<QassandChecker*>(contractStates[QASSAND_CONTRACT_INDEX]); }

	uint64 balanceOf(const id& account) const { return static_cast<uint64>(getBalance(account)); }
	uint64 balanceQassand() const { return balanceOf(QASSAND_CONTRACT_ID); }
	void fund(const id& account, uint64 amount) { increaseEnergy(account, amount); }

	QASSAND::Ping_output ping(const id& invocator, sint64 amount)
	{
		QASSAND::Ping_input input{};
		QASSAND::Ping_output output{};
		invokeUserProcedure(QASSAND_CONTRACT_INDEX, QASSAND_PROC_PING, input, output, invocator, amount);
		return output;
	}

	QASSAND::GetInfo_output getInfo() const
	{
		QASSAND::GetInfo_input input{};
		QASSAND::GetInfo_output output{};
		callFunction(QASSAND_CONTRACT_INDEX, QASSAND_FUNC_GET_INFO, input, output);
		return output;
	}

	QASSAND::GetFeeInfo_output getFeeInfo() const
	{
		QASSAND::GetFeeInfo_input input{};
		QASSAND::GetFeeInfo_output output{};
		callFunction(QASSAND_CONTRACT_INDEX, QASSAND_FUNC_GET_FEE_INFO, input, output);
		return output;
	}

	QASSAND::GetBurnInfo_output getBurnInfo() const
	{
		QASSAND::GetBurnInfo_input input{};
		QASSAND::GetBurnInfo_output output{};
		callFunction(QASSAND_CONTRACT_INDEX, QASSAND_FUNC_GET_BURN_INFO, input, output);
		return output;
	}

	QASSAND::GetLaneInfo_output getLaneInfo(uint16 laneId) const
	{
		QASSAND::GetLaneInfo_input input{laneId};
		QASSAND::GetLaneInfo_output output{};
		callFunction(QASSAND_CONTRACT_INDEX, QASSAND_FUNC_GET_LANE_INFO, input, output);
		return output;
	}

	void endTick() { callSystemProcedure(QASSAND_CONTRACT_INDEX, END_TICK); }
};

static bool arrayStartsWith(const Array<uint8, 16>& value, const char* expected)
{
	for (uint16 i = 0; expected[i] != 0; ++i)
	{
		if (value.get(i) != static_cast<uint8>(expected[i]))
		{
			return false;
		}
	}
	return true;
}

TEST(ContractQassand, InitializeSetsPingV0Metadata)
{
	ContractTestingQassand qassand;
	QassandChecker* state = qassand.state();

	EXPECT_EQ(state->versionValue(), QASSAND_VERSION);
	EXPECT_EQ(state->constructionEpochValue(), QASSAND_CONSTRUCTION_EPOCH_PLACEHOLDER);
	EXPECT_EQ(state->totalPingCountValue(), 0ull);
	EXPECT_EQ(state->protocolEarnedFeeValue(), 0ll);
	EXPECT_EQ(state->burnEarnedFeeValue(), 0ll);
	EXPECT_EQ(state->pendingBurnAmountValue(), 0ll);
	EXPECT_EQ(state->totalBurnedAmountValue(), 0ll);
}

TEST(ContractQassand, ReadsMetadataAndFeeState)
{
	ContractTestingQassand qassand;

	const QASSAND::GetInfo_output info = qassand.getInfo();
	EXPECT_TRUE(arrayStartsWith(info.protocolName, "Qassandra"));
	EXPECT_EQ(info.version, QASSAND_VERSION);
	EXPECT_EQ(info.constructionEpoch, QASSAND_CONSTRUCTION_EPOCH_PLACEHOLDER);
	EXPECT_EQ(info.totalPingCount, 0ull);

	const QASSAND::GetFeeInfo_output fees = qassand.getFeeInfo();
	EXPECT_EQ(fees.pingFee, QASSAND_PING_FEE);
	EXPECT_EQ(fees.protocolFee, QASSAND_PROTOCOL_FEE);
	EXPECT_EQ(fees.burnFee, QASSAND_BURN_FEE);
	EXPECT_EQ(fees.protocolEarnedFee, 0ll);
	EXPECT_EQ(fees.burnEarnedFee, 0ll);

	const QASSAND::GetBurnInfo_output burn = qassand.getBurnInfo();
	EXPECT_EQ(burn.pendingBurnAmount, 0ll);
	EXPECT_EQ(burn.totalBurnedAmount, 0ll);
}

TEST(ContractQassand, ReadsLaneTaxonomy)
{
	ContractTestingQassand qassand;

	const QASSAND::GetLaneInfo_output forecastingLane = qassand.getLaneInfo(QASSAND_LANE_FORECASTING);
	EXPECT_EQ(forecastingLane.returnCode, QASSAND_SUCCESS);
	EXPECT_EQ(forecastingLane.laneId, QASSAND_LANE_FORECASTING);
	EXPECT_TRUE(arrayStartsWith(forecastingLane.laneName, "Forecasting"));
	EXPECT_EQ(forecastingLane.requiredFee, 0ll);

	const QASSAND::GetLaneInfo_output stableLane = qassand.getLaneInfo(QASSAND_LANE_STABLE_OPERATIONS);
	EXPECT_EQ(stableLane.returnCode, QASSAND_SUCCESS);
	EXPECT_EQ(stableLane.laneId, QASSAND_LANE_STABLE_OPERATIONS);
	EXPECT_TRUE(arrayStartsWith(stableLane.laneName, "StableOps"));
	EXPECT_EQ(stableLane.requiredFee, 0ll);

	const QASSAND::GetLaneInfo_output attestationLane = qassand.getLaneInfo(QASSAND_LANE_DATA_ATTESTATION);
	EXPECT_EQ(attestationLane.returnCode, QASSAND_SUCCESS);
	EXPECT_EQ(attestationLane.laneId, QASSAND_LANE_DATA_ATTESTATION);
	EXPECT_TRUE(arrayStartsWith(attestationLane.laneName, "Data"));
	EXPECT_EQ(attestationLane.requiredFee, 0ll);

	const QASSAND::GetLaneInfo_output unknownLane = qassand.getLaneInfo(QASSAND_LANE_UNKNOWN);
	EXPECT_EQ(unknownLane.returnCode, QASSAND_UNKNOWN_LANE);
}

TEST(ContractQassand, UnderpaymentRefundsAndDoesNotAccountFee)
{
	ContractTestingQassand qassand;
	const id user = id::randomValue();
	qassand.fund(user, QASSAND_PING_FEE);

	const sint64 underpaidAmount = QASSAND_PING_FEE - 1;
	const QASSAND::Ping_output underpaid = qassand.ping(user, underpaidAmount);
	EXPECT_EQ(underpaid.returnCode, QASSAND_UNDERPAID);
	EXPECT_EQ(underpaid.refundedAmount, underpaidAmount);
	EXPECT_EQ(qassand.balanceOf(user), QASSAND_PING_FEE);
	EXPECT_EQ(qassand.balanceQassand(), 0ull);
	EXPECT_EQ(qassand.state()->totalPingCountValue(), 0ull);
	EXPECT_EQ(qassand.state()->protocolEarnedFeeValue(), 0ll);
	EXPECT_EQ(qassand.state()->burnEarnedFeeValue(), 0ll);
	EXPECT_EQ(qassand.state()->pendingBurnAmountValue(), 0ll);
}

TEST(ContractQassand, ExactFeeAccountsProtocolAndDeferredBurn)
{
	ContractTestingQassand qassand;
	const id user = id::randomValue();
	qassand.fund(user, QASSAND_PING_FEE);

	const QASSAND::Ping_output ping = qassand.ping(user, QASSAND_PING_FEE);
	EXPECT_EQ(ping.returnCode, QASSAND_SUCCESS);
	EXPECT_EQ(ping.acceptedFee, QASSAND_PING_FEE);
	EXPECT_EQ(ping.refundedAmount, 0ll);
	EXPECT_EQ(ping.protocolEarnedFee, QASSAND_PROTOCOL_FEE);
	EXPECT_EQ(ping.burnEarnedFee, QASSAND_BURN_FEE);
	EXPECT_EQ(ping.totalPingCount, 1ull);

	EXPECT_EQ(qassand.balanceOf(user), 0ull);
	EXPECT_EQ(qassand.balanceQassand(), QASSAND_PING_FEE);
	EXPECT_EQ(qassand.state()->totalPingCountValue(), 1ull);
	EXPECT_EQ(qassand.state()->protocolEarnedFeeValue(), QASSAND_PROTOCOL_FEE);
	EXPECT_EQ(qassand.state()->burnEarnedFeeValue(), QASSAND_BURN_FEE);
	EXPECT_EQ(qassand.state()->pendingBurnAmountValue(), QASSAND_BURN_FEE);
}

TEST(ContractQassand, ExcessFeeRefundsOnlyOverage)
{
	ContractTestingQassand qassand;
	const id user = id::randomValue();
	const sint64 paidAmount = QASSAND_PING_FEE + 12345;
	qassand.fund(user, paidAmount);

	const QASSAND::Ping_output ping = qassand.ping(user, paidAmount);
	EXPECT_EQ(ping.returnCode, QASSAND_SUCCESS);
	EXPECT_EQ(ping.acceptedFee, QASSAND_PING_FEE);
	EXPECT_EQ(ping.refundedAmount, 12345ll);
	EXPECT_EQ(qassand.balanceOf(user), 12345ull);
	EXPECT_EQ(qassand.balanceQassand(), QASSAND_PING_FEE);
	EXPECT_EQ(qassand.state()->totalPingCountValue(), 1ull);
	EXPECT_EQ(qassand.state()->pendingBurnAmountValue(), QASSAND_BURN_FEE);
}

TEST(ContractQassand, EndTickBurnsDeferredAmount)
{
	ContractTestingQassand qassand;
	const id user = id::randomValue();
	qassand.fund(user, QASSAND_PING_FEE);

	qassand.ping(user, QASSAND_PING_FEE);
	EXPECT_EQ(qassand.state()->pendingBurnAmountValue(), QASSAND_BURN_FEE);
	EXPECT_EQ(qassand.state()->totalBurnedAmountValue(), 0ll);

	qassand.endTick();
	EXPECT_EQ(qassand.state()->pendingBurnAmountValue(), 0ll);
	EXPECT_EQ(qassand.state()->totalBurnedAmountValue(), QASSAND_BURN_FEE);
	EXPECT_EQ(qassand.balanceQassand(), QASSAND_PROTOCOL_FEE);
}
