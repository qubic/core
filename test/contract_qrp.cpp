#define NO_UEFI

#include "contract_testing.h"

// Procedure/function indices (must match REGISTER_USER_FUNCTIONS_AND_PROCEDURES in `src/contracts/QReservePool.h`).
constexpr uint16 QRP_PROC_GET_RESERVE = 1;
constexpr uint16 QRP_PROC_ADD_AVAILABLE_SC = 2;
constexpr uint16 QRP_PROC_REMOVE_AVAILABLE_SC = 3;

constexpr uint16 QRP_FUNC_GET_AVAILABLE_RESERVE = 1;
constexpr uint16 QRP_FUNC_GET_AVAILABLE_SCS = 2;

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
	bool hasAvailableSC(const id& sc) const { return availableSmartContracts.contains(sc); }
	uint64 availableCount() const { return availableSmartContracts.population(); }
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

	QRP::GetReserve_output getReserve(const id& invocator, uint64 revenue, sint64 attachedAmount = 0)
	{
		QRP::GetReserve_input input{revenue};
		QRP::GetReserve_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, QRP_PROC_GET_RESERVE, input, output, invocator, attachedAmount);
		return output;
	}

	QRP::AddAvailableSC_output addAvailableSC(const id& invocator, uint64 scIndex)
	{
		QRP::AddAvailableSC_input input{scIndex};
		QRP::AddAvailableSC_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, QRP_PROC_ADD_AVAILABLE_SC, input, output, invocator, 0);
		return output;
	}

	QRP::RemoveAvailableSC_output removeAvailableSC(const id& invocator, uint64 scIndex)
	{
		QRP::RemoveAvailableSC_input input{scIndex};
		QRP::RemoveAvailableSC_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, QRP_PROC_REMOVE_AVAILABLE_SC, input, output, invocator, 0);
		return output;
	}

	QRP::GetAvailableReserve_output getAvailableReserve() const
	{
		QRP::GetAvailableReserve_input input{};
		QRP::GetAvailableReserve_output output{};
		callFunction(QRP_CONTRACT_INDEX, QRP_FUNC_GET_AVAILABLE_RESERVE, input, output);
		return output;
	}

	QRP::GetAvailableSC_output getAvailableSCs() const
	{
		QRP::GetAvailableSC_input input{};
		QRP::GetAvailableSC_output output{};
		callFunction(QRP_CONTRACT_INDEX, QRP_FUNC_GET_AVAILABLE_SCS, input, output);
		return output;
	}
};

static bool containsAvailableSC(const QRP::GetAvailableSC_output& available, const id& sc)
{
	for (uint64 i = 0; i < QRP_AVAILABLE_SC_NUM; ++i)
	{
		if (available.availableSCs.get(i) == sc)
		{
			return true;
		}
	}
	return false;
}

TEST(ContractQReservePool, GetReserveEnforcesAuthorizationAndBalance)
{
	ContractTestingQRP qrp;
	const id unauthorized = id::randomValue();
	qrp.fund(unauthorized, 0);
	qrp.fund(QRP_DEFAULT_SC_ID, 0);

	QRP::GetReserve_output denied = qrp.getReserve(unauthorized, 100);
	EXPECT_EQ(denied.returnCode, QRP::toReturnCode(QRP::EReturnCode::ACCESS_DENIED));
	EXPECT_EQ(denied.allocatedRevenue, 0ull);

	qrp.fundQrp(1000);
	EXPECT_EQ(qrp.balanceQrp(), 1000);

	QRP::GetReserve_output granted = qrp.getReserve(QRP_DEFAULT_SC_ID, 600);
	EXPECT_EQ(granted.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_EQ(granted.allocatedRevenue, 600ull);
	EXPECT_EQ(qrp.balanceQrp(), 400);
	EXPECT_EQ(qrp.balanceOf(QRP_DEFAULT_SC_ID), 600);

	QRP::GetReserve_output insufficient = qrp.getReserve(QRP_DEFAULT_SC_ID, 500);
	EXPECT_EQ(insufficient.returnCode, QRP::toReturnCode(QRP::EReturnCode::INSUFFICIENT_RESERVE));
	EXPECT_EQ(insufficient.allocatedRevenue, 0ull);
	EXPECT_EQ(qrp.balanceQrp(), 400);
	EXPECT_EQ(qrp.balanceOf(QRP_DEFAULT_SC_ID), 600);
}

TEST(ContractQReservePool, GetReserve_ZeroAndExactRemaining)
{
	ContractTestingQRP qrp;
	qrp.fund(QRP_DEFAULT_SC_ID, 0);

	qrp.fundQrp(1000);
	EXPECT_EQ(qrp.balanceQrp(), 1000);

	// Zero request should not move funds.
	const QRP::GetReserve_output zero = qrp.getReserve(QRP_DEFAULT_SC_ID, 0);
	EXPECT_EQ(zero.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_EQ(zero.allocatedRevenue, 0ull);
	EXPECT_EQ(qrp.balanceQrp(), 1000);

	// Exact remaining should succeed and drain the reserve.
	const QRP::GetReserve_output exact = qrp.getReserve(QRP_DEFAULT_SC_ID, 1000);
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

	QRP::AddAvailableSC_output deniedAdd = qrp.addAvailableSC(outsider, newScIndex);
	EXPECT_EQ(deniedAdd.returnCode, QRP::toReturnCode(QRP::EReturnCode::ACCESS_DENIED));
	EXPECT_FALSE(state->hasAvailableSC(newScId));

	QRP::AddAvailableSC_output approvedAdd = qrp.addAvailableSC(state->owner(), newScIndex);
	EXPECT_EQ(approvedAdd.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_TRUE(state->hasAvailableSC(newScId));

	QRP::GetAvailableSC_output available = qrp.getAvailableSCs();
	EXPECT_TRUE(containsAvailableSC(available, newScId));

	QRP::RemoveAvailableSC_output deniedRemove = qrp.removeAvailableSC(outsider, newScIndex);
	EXPECT_EQ(deniedRemove.returnCode, QRP::toReturnCode(QRP::EReturnCode::ACCESS_DENIED));
	EXPECT_TRUE(state->hasAvailableSC(newScId));

	QRP::RemoveAvailableSC_output approvedRemove = qrp.removeAvailableSC(state->owner(), newScIndex);
	EXPECT_EQ(approvedRemove.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_FALSE(state->hasAvailableSC(newScId));
}

TEST(ContractQReservePool, OwnerAddRemove_IdempotencyAndBounds)
{
	ContractTestingQRP qrp;
	QRPChecker* state = qrp.state();
	qrp.fund(state->owner(), 0);

	constexpr uint64 newScIndex = 88;
	const id newScId(newScIndex, 0, 0, 0);
	qrp.fund(newScId, 0);

	EXPECT_FALSE(state->hasAvailableSC(newScId));

	// This test focuses on idempotency (repeat add/remove) while keeping authorization valid.
	// Add twice: first should succeed, second should not change membership (return code may be SUCCESS or specific).
	const auto add1 = qrp.addAvailableSC(state->owner(), newScIndex);
	EXPECT_EQ(add1.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_TRUE(state->hasAvailableSC(newScId));

	const auto add2 = qrp.addAvailableSC(state->owner(), newScIndex);
	EXPECT_TRUE(state->hasAvailableSC(newScId));

	// Remove twice: first should succeed, second should keep it removed (return code may be SUCCESS or specific).
	const auto rem1 = qrp.removeAvailableSC(state->owner(), newScIndex);
	EXPECT_EQ(rem1.returnCode, QRP::toReturnCode(QRP::EReturnCode::SUCCESS));
	EXPECT_FALSE(state->hasAvailableSC(newScId));

	const auto rem2 = qrp.removeAvailableSC(state->owner(), newScIndex);
	EXPECT_FALSE(state->hasAvailableSC(newScId));
}
