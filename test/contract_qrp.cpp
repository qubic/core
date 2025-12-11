#define NO_UEFI

#include "contract_testing.h"

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

	void fundContract(uint64 amount) { increaseEnergy(QRP_CONTRACT_ID, amount); }

	QRP::GetReserve_output getReserve(const id& invocator, uint64 revenue, sint64 attachedAmount = 0)
	{
		QRP::GetReserve_input input{revenue};
		QRP::GetReserve_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, 1, input, output, invocator, attachedAmount);
		return output;
	}

	QRP::AddAvailableSC_output addAvailableSC(const id& invocator, uint64 scIndex)
	{
		QRP::AddAvailableSC_input input{scIndex};
		QRP::AddAvailableSC_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, 2, input, output, invocator, 0);
		return output;
	}

	QRP::RemoveAvailableSC_output removeAvailableSC(const id& invocator, uint64 scIndex)
	{
		QRP::RemoveAvailableSC_input input{scIndex};
		QRP::RemoveAvailableSC_output output{};
		invokeUserProcedure(QRP_CONTRACT_INDEX, 3, input, output, invocator, 0);
		return output;
	}

	QRP::GetAvailableReserve_output getAvailableReserve() const
	{
		QRP::GetAvailableReserve_input input{};
		QRP::GetAvailableReserve_output output{};
		callFunction(QRP_CONTRACT_INDEX, 1, input, output);
		return output;
	}

	QRP::GetAvailableSC_output getAvailableSCs() const
	{
		QRP::GetAvailableSC_input input{};
		QRP::GetAvailableSC_output output{};
		callFunction(QRP_CONTRACT_INDEX, 2, input, output);
		return output;
	}
};

TEST(ContractQReservePool, InitializesOwnerAndDefaultSCList)
{
	ContractTestingQRP qrp;
	QRPChecker* state = qrp.state();

	EXPECT_EQ(state->team(), QRP_OWNER_TEAM_ADDRESS);
	EXPECT_EQ(state->owner(), QRP_OWNER_TEAM_ADDRESS);
	EXPECT_TRUE(state->hasAvailableSC(QRP_DEFAULT_SC_ID));
	EXPECT_EQ(state->availableCount(), 1u);

	const QRP::GetAvailableSC_output available = qrp.getAvailableSCs();
	bool foundDefault = false;
	for (uint64 i = 0; i < QRP_AVAILABLE_SC_NUM; ++i)
	{
		if (available.availableSCs.get(i) == QRP_DEFAULT_SC_ID)
		{
			foundDefault = true;
			break;
		}
	}
	EXPECT_TRUE(foundDefault);
}

TEST(ContractQReservePool, GetReserveEnforcesAuthorizationAndBalance)
{
	ContractTestingQRP qrp;
	const id unauthorized(77, 0, 0, 0);
	increaseEnergy(unauthorized, 0);
	increaseEnergy(QRP_DEFAULT_SC_ID, 0);

	QRP::GetReserve_output denied = qrp.getReserve(unauthorized, 100);
	EXPECT_EQ(denied.returnCode, QRPReturnCode::ACCESS_DENIED);
	EXPECT_EQ(denied.allocatedRevenue, 0ull);

	qrp.fundContract(1000);
	EXPECT_EQ(getBalance(QRP_CONTRACT_ID), 1000);

	QRP::GetReserve_output granted = qrp.getReserve(QRP_DEFAULT_SC_ID, 600);
	EXPECT_EQ(granted.returnCode, QRPReturnCode::SUCCESS);
	EXPECT_EQ(granted.allocatedRevenue, 600ull);
	EXPECT_EQ(getBalance(QRP_CONTRACT_ID), 400);
	EXPECT_EQ(getBalance(QRP_DEFAULT_SC_ID), 600);

	QRP::GetReserve_output insufficient = qrp.getReserve(QRP_DEFAULT_SC_ID, 500);
	EXPECT_EQ(insufficient.returnCode, QRPReturnCode::INSUFFICIENT_RESERVE);
	EXPECT_EQ(insufficient.allocatedRevenue, 0ull);
	EXPECT_EQ(getBalance(QRP_CONTRACT_ID), 400);
	EXPECT_EQ(getBalance(QRP_DEFAULT_SC_ID), 600);
}

TEST(ContractQReservePool, OwnerAddsAndRemovesSmartContracts)
{
	ContractTestingQRP qrp;
	QRPChecker* state = qrp.state();
	const uint64 newScIndex = 77;
	const id newScId(newScIndex, 0, 0, 0);
	const id outsider(200, 0, 0, 0);
	increaseEnergy(newScId, 0);
	increaseEnergy(outsider, 0);
	increaseEnergy(state->owner(), 0);

	QRP::AddAvailableSC_output deniedAdd = qrp.addAvailableSC(outsider, newScIndex);
	EXPECT_EQ(deniedAdd.returnCode, QRPReturnCode::ACCESS_DENIED);
	EXPECT_FALSE(state->hasAvailableSC(newScId));

	QRP::AddAvailableSC_output approvedAdd = qrp.addAvailableSC(state->owner(), newScIndex);
	EXPECT_EQ(approvedAdd.returnCode, QRPReturnCode::SUCCESS);
	EXPECT_TRUE(state->hasAvailableSC(newScId));

	QRP::GetAvailableSC_output available = qrp.getAvailableSCs();
	bool foundNew = false;
	for (uint64 i = 0; i < QRP_AVAILABLE_SC_NUM; ++i)
	{
		if (available.availableSCs.get(i) == newScId)
		{
			foundNew = true;
			break;
		}
	}
	EXPECT_TRUE(foundNew);

	QRP::RemoveAvailableSC_output deniedRemove = qrp.removeAvailableSC(outsider, newScIndex);
	EXPECT_EQ(deniedRemove.returnCode, QRPReturnCode::ACCESS_DENIED);
	EXPECT_TRUE(state->hasAvailableSC(newScId));

	QRP::RemoveAvailableSC_output approvedRemove = qrp.removeAvailableSC(state->owner(), newScIndex);
	EXPECT_EQ(approvedRemove.returnCode, QRPReturnCode::SUCCESS);
	EXPECT_FALSE(state->hasAvailableSC(newScId));
}
