#pragma once

#include "contracts/qpi.h"
#include "oracle_core/oracle_engine.h"
#include "spectrum/spectrum.h"


template <typename OracleInterface, typename ContractStateType, typename LocalsType>
QPI::uint64 QPI::QpiContextProcedureCall::queryOracle(
	const OracleInterface::OracleQuery& query,
	void (*notificationCallback)(const QPI::QpiContextProcedureCall& qpi, ContractStateType& state, QPI::OracleNotificationInput<OracleInterface>& input, QPI::NoData& output, LocalsType& locals),
	uint32 timeoutMillisec
) const
{
	// check that size of oracle query, oracle reply, notification procedure locals are valid
	static_assert(sizeof(OracleInterface::OracleQuery) <= MAX_ORACLE_QUERY_SIZE);
	static_assert(sizeof(OracleInterface::OracleReply) <= MAX_ORACLE_REPLY_SIZE);
	static_assert(sizeof(LocalsType) <= MAX_SIZE_OF_CONTRACT_LOCALS);

	// check that oracle interface and notification input type are as expected
	static_assert(sizeof(QPI::OracleNotificationInput<typename OracleInterface::OracleReply>) == 16 + sizeof(OracleInterface::OracleReply));
	static_assert(OracleInterface::oracleInterfaceIndex < OI::oracleInterfacesCount);
	static_assert(OI::oracleInterfaces[OracleInterface::oracleInterfaceIndex].replySize == sizeof(OracleInterface::OracleReply));
	static_assert(OI::oracleInterfaces[OracleInterface::oracleInterfaceIndex].querySize == sizeof(OracleInterface::OracleQuery));
	
	// check contract index
	ASSERT(this->_currentContractIndex < 0xffff);
	const QPI::uint16 contractIndex = static_cast<QPI::uint16>(this->_currentContractIndex);

	// check callback
	if (!notificationCallback || ContractStateType::__contract_index != contractIndex)
		return 0;

	// get and destroy fee (not adding to contracts execution fee reserve)
	sint64 fee = OracleInterface::getQueryFee(query);
	int contractSpectrumIdx = ::spectrumIndex(this->_currentContractId);
	if (fee >= 0 && contractSpectrumIdx >= 0 && decreaseEnergy(contractSpectrumIdx, fee))
	{
		// try to start query
		QPI::uint64 queryId = oracleEngine.startContractQuery(
			contractIndex, OracleInterface::oracleInterfaceIndex,
			&query, sizeof(query), timeoutMillisec,
			(USER_PROCEDURE)notificationCallback, sizeof(LocalsType));
		if (queryId != 0)
		{
			// success
			return queryId;
		}
	}

	// notify about error (status and queryId are 0, indicating that an error happened before sending query)
	auto* state = (ContractStateType*)contractStates[contractIndex];
	auto* input = (QPI::OracleNotificationInput<OracleInterface>*)__qpiAllocLocals(sizeof(QPI::OracleNotificationInput<OracleInterface>));
	input->status = ORACLE_QUERY_STATUS_UNKNOWN;
	QPI::NoData output;
	auto* locals = (LocalsType*)__qpiAllocLocals(sizeof(LocalsType));
	notificationCallback(*this, *state, *input, output, *locals);
	__qpiFreeLocals();
	__qpiFreeLocals();
	return 0;
}

template <typename OracleInterface, typename ContractStateType, typename LocalsType>
inline QPI::uint32 QPI::QpiContextProcedureCall::subscribeOracle(
	const OracleInterface::OracleQuery& query,
	void (*notificationCallback)(const QPI::QpiContextProcedureCall& qpi, ContractStateType& state, OracleNotificationInput<OracleInterface>& input, NoData& output, LocalsType& locals),
	QPI::uint32 notificationIntervalInMinutes,
	bool notifyWithPreviousReply
) const
{
	static_assert(sizeof(query.timestamp) == sizeof(DateAndTime));
	// TODO
	return 0;
}

inline bool QPI::QpiContextProcedureCall::unsubscribeOracle(
	QPI::uint32 oracleSubscriptionId
) const
{
	// TODO
	return false;
}

template <typename OracleInterface>
bool QPI::QpiContextFunctionCall::getOracleQuery(QPI::uint64 queryId, OracleInterface::OracleQuery& query) const
{
	// TODO
	return false;
}

template <typename OracleInterface>
bool QPI::QpiContextFunctionCall::getOracleReply(QPI::uint64 queryId, OracleInterface::OracleReply& reply) const
{
	// TODO
	return false;
}

template <typename OracleInterface>
inline QPI::uint8 QPI::QpiContextFunctionCall::getOracleQueryStatus(uint64 queryId) const
{
	// TODO
	return ORACLE_QUERY_STATUS_UNKNOWN;
}
