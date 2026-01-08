#define NO_UEFI

#include "contract_testing.h"

// Procedure/function indices (must match REGISTER_USER_FUNCTIONS_AND_PROCEDURES in `src/contracts/QReservePool.h`).
constexpr uint16 QRP_PROC_GET_RESERVE = 1;
constexpr uint16 QRP_PROC_ADD_ALLOWED_SC = 2;
constexpr uint16 QRP_PROC_REMOVE_ALLOWED_SC = 3;

constexpr uint16 QRP_FUNC_GET_AVAILABLE_RESERVE = 1;
constexpr uint16 QRP_FUNC_GET_ALLOWED_SC = 2;

static const id QRP_CONTRACT_ID(QRP_CONTRACT_INDEX, 0, 0, 0);
static const id QRP_DEFAULT_SC_ID(QRP_QTF_INDEX, 0, 0, 0);
static const id QRP_OWNER_TEAM_ADDRESS =
    ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L, _W, _E, _T, _H, _N, _G,
       _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

class QRPChecker : public QRP
{
public:
	const id& team() const { return teamAddress; }
	const id& owner() const { return ownerAddress; }
	bool hasAllowedSC(const id& sc) const { return allowedSmartContracts.contains(sc); }
	uint64 allowedCount() const { return allowedSmartContracts.population(); }
};

class ContractTestingQRP : protected ContractTesting
{
public:
	ContractTestingQRP()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QRP);
		callSystemProcedure(QRP_CONTRACT_INDEX, INITIALIZE);
	}

	QRPChecker* state() { return reinterpret_cast<QRPChecker*>(contractStates[QRP_CONTRACT_INDEX]); }

	uint64 balanceOf(const id& account) const { return static_cast<uint64>(getBalance(account)); }
	uint64 balanceQrp() const { return balanceOf(QRP_CONTRACT_ID); }
	void fund(const id& account, uint64 amount) { increaseEnergy(account, amount); }
	void fundQrp(uint64 amount) { fund(QRP_CONTRACT_ID, amount); }

	QRP::WithdrawReserve_output withdrawReserveReserve(const id& invocator, uint64 revenue, sint64 attachedAmount = 0)
	{
		QRP::WithdrawReserve_input input{revenue};
		QRP::WithdrawReserve_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, QRP_PROC_GET_RESERVE, input, output, invocator, attachedAmount);
		return output;
	}

	QRP::AddAllowedSC_output addAllowedSC(const id& invocator, uint64 scIndex)
	{
		QRP::AddAllowedSC_input input{scIndex};
		QRP::AddAllowedSC_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, QRP_PROC_ADD_ALLOWED_SC, input, output, invocator, 0);
		return output;
	}

	QRP::RemoveAllowedSC_output removeAllowedSC(const id& invocator, uint64 scIndex)
	{
		QRP::RemoveAllowedSC_input input{scIndex};
		QRP::RemoveAllowedSC_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, QRP_PROC_REMOVE_ALLOWED_SC, input, output, invocator, 0);
		return output;
	}

	QRP::GetAvailableReserve_output getAvailableReserve() const
	{
		QRP::GetAvailableReserve_input input{};
		QRP::GetAvailableReserve_output output{};
		callFunction(QRP_CONTRACT_INDEX, QRP_FUNC_GET_AVAILABLE_RESERVE, input, output);
		return output;
	}

	QRP::GetAllowedSC_output getAllowedSC() const
	{
		QRP::GetAllowedSC_input input{};
		QRP::GetAllowedSC_output output{};
		callFunction(QRP_CONTRACT_INDEX, QRP_FUNC_GET_ALLOWED_SC, input, output);
		return output;
	}
};

static bool containsAllowedSC(const QRP::GetAllowedSC_output& allowed, const id& sc)
{
	for (uint64 i = 0; i < QRP_ALLOWED_SC_NUM; ++i)
	{
		if (allowed.allowedSC.get(i) == sc)
		{
			return true;
		}
	}
	return false;
}

TEST(ContractQReservePool, WithdrawReserveEnforcesAuthorizationAndBalance)
{
	ContractTestingQRP qrp;
	const id unauthorized = id::randomValue();
	qrp.fund(unauthorized, 0);
	qrp.fund(QRP_DEFAULT_SC_ID, 0);

	QRP::WithdrawReserve_output denied = qrp.withdrawReserveReserve(unauthorized, 100);
	EXPECT_EQ(denied.returnCode, QRP::toReturnCode(QRP::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(denied.allocatedRevenue, 0ull);

	qrp.fundQrp(1000);
	EXPECT_EQ(qrp.balanceQrp(), 1000);

	QRP::WithdrawReserve_output granted = qrp.withdrawReserveReserve(QRP_DEFAULT_SC_ID, 600);
	EXPECT_EQ(granted.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_EQ(granted.allocatedRevenue, 600ull);
	EXPECT_EQ(qrp.balanceQrp(), 400);
	EXPECT_EQ(qrp.balanceOf(QRP_DEFAULT_SC_ID), 600);

	QRP::WithdrawReserve_output insufficient = qrp.withdrawReserveReserve(QRP_DEFAULT_SC_ID, 500);
	EXPECT_EQ(insufficient.returnCode, QRP::toReturnCode(QRP::EReturnCode::INSUFFICIENT_RESERVE));
	EXPECT_EQ(insufficient.allocatedRevenue, 0ull);
	EXPECT_EQ(qrp.balanceQrp(), 400);
	EXPECT_EQ(qrp.balanceOf(QRP_DEFAULT_SC_ID), 600);
}

TEST(ContractQReservePool, WithdrawReserve_ZeroAndExactRemaining)
{
	ContractTestingQRP qrp;
	qrp.fund(QRP_DEFAULT_SC_ID, 0);

	qrp.fundQrp(1000);
	EXPECT_EQ(qrp.balanceQrp(), 1000);

	// Zero request should not move funds.
	const QRP::WithdrawReserve_output zero = qrp.withdrawReserveReserve(QRP_DEFAULT_SC_ID, 0);
	EXPECT_EQ(zero.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_EQ(zero.allocatedRevenue, 0ull);
	EXPECT_EQ(qrp.balanceQrp(), 1000);

	// Exact remaining should succeed and drain the reserve.
	const QRP::WithdrawReserve_output exact = qrp.withdrawReserveReserve(QRP_DEFAULT_SC_ID, 1000);
	EXPECT_EQ(exact.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_EQ(exact.allocatedRevenue, 1000ull);
	EXPECT_EQ(qrp.balanceQrp(), 0);
	EXPECT_EQ(qrp.balanceOf(QRP_DEFAULT_SC_ID), 1000);
}

TEST(ContractQReservePool, OwnerAddsAndRemovesSmartContracts)
{
	ContractTestingQRP qrp;
	QRPChecker* state = qrp.state();
	constexpr uint64 newScIndex = 77;
	const id newScId(newScIndex, 0, 0, 0);
	const id outsider(200, 0, 0, 0);
	qrp.fund(newScId, 0);
	qrp.fund(outsider, 0);
	qrp.fund(state->owner(), 0);

	QRP::AddAllowedSC_output deniedAdd = qrp.addAllowedSC(outsider, newScIndex);
	EXPECT_EQ(deniedAdd.returnCode, QRP::toReturnCode(QRP::EReturnCode::ACCESS_DENIED));
	EXPECT_FALSE(state->hasAllowedSC(newScId));

	QRP::AddAllowedSC_output approvedAdd = qrp.addAllowedSC(state->owner(), newScIndex);
	EXPECT_EQ(approvedAdd.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_TRUE(state->hasAllowedSC(newScId));

	QRP::GetAllowedSC_output allowed = qrp.getAllowedSC();
	EXPECT_TRUE(containsAllowedSC(allowed, newScId));

	QRP::RemoveAllowedSC_output deniedRemove = qrp.removeAllowedSC(outsider, newScIndex);
	EXPECT_EQ(deniedRemove.returnCode, QRP::toReturnCode(QRP::EReturnCode::ACCESS_DENIED));
	EXPECT_TRUE(state->hasAllowedSC(newScId));

	QRP::RemoveAllowedSC_output approvedRemove = qrp.removeAllowedSC(state->owner(), newScIndex);
	EXPECT_EQ(approvedRemove.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_FALSE(state->hasAllowedSC(newScId));
}

TEST(ContractQReservePool, OwnerAddRemove_IdempotencyAndBounds)
{
	ContractTestingQRP qrp;
	QRPChecker* state = qrp.state();
	qrp.fund(state->owner(), 0);

	constexpr uint64 newScIndex = 88;
	const id newScId(newScIndex, 0, 0, 0);
	qrp.fund(newScId, 0);

	EXPECT_FALSE(state->hasAllowedSC(newScId));

	// This test focuses on idempotency (repeat add/remove) while keeping authorization valid.
	// Add twice: first should succeed, second should not change membership (return code may be SUCCESS or specific).
	const auto add1 = qrp.addAllowedSC(state->owner(), newScIndex);
	EXPECT_EQ(add1.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_TRUE(state->hasAllowedSC(newScId));

	const auto add2 = qrp.addAllowedSC(state->owner(), newScIndex);
	EXPECT_TRUE(state->hasAllowedSC(newScId));

	// Remove twice: first should succeed, second should keep it removed (return code may be SUCCESS or specific).
	const auto rem1 = qrp.removeAllowedSC(state->owner(), newScIndex);
	EXPECT_EQ(rem1.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_FALSE(state->hasAllowedSC(newScId));

	const auto rem2 = qrp.removeAllowedSC(state->owner(), newScIndex);
	EXPECT_FALSE(state->hasAllowedSC(newScId));
}
