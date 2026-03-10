#pragma once

#include "contracts/qpi.h"
#include "oracle_core/oracle_engine.h"
#include "spectrum/spectrum.h"


template <typename OracleInterface, typename ContractStateType, typename LocalsType>
QPI::sint64 QPI::QpiContextProcedureCall::__qpiQueryOracle(
	const OracleInterface::OracleQuery& query,
	void (*notificationProcPtr)(const QPI::QpiContextProcedureCall& qpi, ContractStateType& state, OracleNotificationInput<OracleInterface>& input, NoData& output, LocalsType& locals),
	unsigned int notificationProcId, 
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
	if (!notificationProcPtr || ContractStateType::__contract_index != contractIndex)
		return -1;

	// check vs registry of user procedures for notification
	const UserProcedureRegistry::UserProcedureData* procData;
	if (!userProcedureRegistry || !(procData = userProcedureRegistry->get(notificationProcId)) || procData->procedure != (USER_PROCEDURE)notificationProcPtr)
		return -1;
	ASSERT(procData->inputSize == sizeof(OracleNotificationInput<OracleInterface>));
	ASSERT(procData->localsSize == sizeof(LocalsType));

	// get and destroy fee (not adding to contracts execution fee reserve)
	sint64 fee = OracleInterface::getQueryFee(query);
	int contractSpectrumIdx = ::spectrumIndex(this->_currentContractId);
	if (fee >= 0 && contractSpectrumIdx >= 0 && decreaseEnergy(contractSpectrumIdx, fee))
	{
		// log burning of QU
		const QuTransfer quTransfer = { this->_currentContractId, m256i::zero(), fee };
		logger.logQuTransfer(quTransfer);

		// try to start query
		QPI::sint64 queryId = oracleEngine.startContractQuery(
			contractIndex, OracleInterface::oracleInterfaceIndex,
			&query, sizeof(query), timeoutMillisec, notificationProcId);
		if (queryId >= 0)
		{
			// success
			return queryId;
		}
		else if (fee > 0)
		{
			// failure -> refund fee
			oracleEngine.refundFees(_currentContractId, fee);
		}
	}
#if !defined(NDEBUG) && !defined(NO_UEFI)
	else
	{
		addDebugMessage(L"Cannot start contract oracle query due to fee issue!");
	}
#endif

	// notify about error (status is 0 and queryId is -1, indicating that an error happened before sending query)
	auto* state = (ContractStateType*)contractStates[contractIndex];
	auto* input = (QPI::OracleNotificationInput<OracleInterface>*)__qpiAllocLocals(sizeof(QPI::OracleNotificationInput<OracleInterface>));
	input->status = ORACLE_QUERY_STATUS_UNKNOWN;
	input->queryId = -1;
	input->subscriptionId = -1;
	QPI::NoData output;
	auto* locals = (LocalsType*)__qpiAllocLocals(sizeof(LocalsType));
	notificationProcPtr(*this, *state, *input, output, *locals);
	__qpiFreeLocals();
	__qpiFreeLocals();
	return -1;
}

template <typename OracleInterface, typename ContractStateType, typename LocalsType>
inline QPI::sint32 QPI::QpiContextProcedureCall::__qpiSubscribeOracle(
	const OracleInterface::OracleQuery& query,
	void (*notificationProcPtr)(const QPI::QpiContextProcedureCall& qpi, ContractStateType& state, OracleNotificationInput<OracleInterface>& input, NoData& output, LocalsType& locals),
	unsigned int notificationProcId,
	QPI::uint32 notificationPeriodInMilliseconds,
	bool notifyWithPreviousReply
) const
{
	// check that query has timestamp member
	static_assert(sizeof(query.timestamp) == sizeof(DateAndTime));

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
	if (!notificationProcPtr || ContractStateType::__contract_index != contractIndex)
		return -1;

	// check vs registry of user procedures for notification
	const UserProcedureRegistry::UserProcedureData* procData;
	if (!userProcedureRegistry || !(procData = userProcedureRegistry->get(notificationProcId)) || procData->procedure != (USER_PROCEDURE)notificationProcPtr)
		return -1;
	ASSERT(procData->inputSize == sizeof(OracleNotificationInput<OracleInterface>));
	ASSERT(procData->localsSize == sizeof(LocalsType));

	// get and destroy fee (not adding to contracts execution fee reserve)
	const sint64 fee = OracleInterface::getSubscriptionFee(query, notificationPeriodInMilliseconds);
	const int contractSpectrumIdx = ::spectrumIndex(this->_currentContractId);
	if (fee >= 0 && contractSpectrumIdx >= 0 && decreaseEnergy(contractSpectrumIdx, fee))
	{
		// log burning of QU
		const QuTransfer quTransfer = { this->_currentContractId, m256i::zero(), fee };
		logger.logQuTransfer(quTransfer);

		// try to subscribe
		QPI::sint32 subscriptionId = oracleEngine.startContractSubscription(
			contractIndex, OracleInterface::oracleInterfaceIndex,
			&query, sizeof(query), notificationPeriodInMilliseconds,
			notificationProcId, offsetof(OracleInterface::OracleQuery, timestamp));
		if (subscriptionId >= 0)
		{
			// success
			if (notifyWithPreviousReply)
			{
				// notify contract with previous reply if any is available
				const OracleSubscription* subscription = oracleEngine.getOracleSubscription(subscriptionId);
				if (subscription && subscription->lastRevealedQueryId >= 0)
				{
					const int64_t queryId = subscription->lastRevealedQueryId;
					auto* state = (ContractStateType*)contractStates[contractIndex];
					auto* input = (QPI::OracleNotificationInput<OracleInterface>*)__qpiAllocLocals(sizeof(QPI::OracleNotificationInput<OracleInterface>));
					input->status = ORACLE_QUERY_STATUS_SUCCESS;
					input->queryId = queryId;
					input->subscriptionId = subscriptionId;
					oracleEngine.getOracleReply(queryId, &input->reply, sizeof(input->reply));
					QPI::NoData output;
					auto* locals = (LocalsType*)__qpiAllocLocals(sizeof(LocalsType));
					notificationProcPtr(*this, *state, *input, output, *locals);
					__qpiFreeLocals();
					__qpiFreeLocals();
				}
			}

			return subscriptionId;
		}
		else if (fee > 0)
		{
			// failure -> refund fee
			oracleEngine.refundFees(_currentContractId, fee);
		}
	}
#if !defined(NDEBUG) && !defined(NO_UEFI)
	else
	{
		addDebugMessage(L"Cannot start contract oracle subscription due to fee issue!");
	}
#endif

	// notify about error (status is 0 and subscriptionId is -1, indicating that subscribing failed)
	auto* state = (ContractStateType*)contractStates[contractIndex];
	auto* input = (QPI::OracleNotificationInput<OracleInterface>*)__qpiAllocLocals(sizeof(QPI::OracleNotificationInput<OracleInterface>));
	input->status = ORACLE_QUERY_STATUS_UNKNOWN;
	input->queryId = -1;
	input->subscriptionId = -1;
	QPI::NoData output;
	auto* locals = (LocalsType*)__qpiAllocLocals(sizeof(LocalsType));
	notificationProcPtr(*this, *state, *input, output, *locals);
	__qpiFreeLocals();
	__qpiFreeLocals();
	return -1;
}

inline bool QPI::QpiContextProcedureCall::unsubscribeOracle(
	QPI::sint32 oracleSubscriptionId
) const
{
	ASSERT(this->_currentContractIndex < 0xffff);
	const QPI::uint16 contractIndex = static_cast<QPI::uint16>(this->_currentContractIndex);
	return oracleEngine.stopContractSubscription(oracleSubscriptionId, contractIndex);
}

template <typename OracleInterface>
bool QPI::QpiContextFunctionCall::getOracleQuery(QPI::sint64 queryId, OracleInterface::OracleQuery& query) const
{
	return oracleEngine.getOracleQuery(queryId, &query, sizeof(query));
}

template <typename OracleInterface>
bool QPI::QpiContextFunctionCall::getOracleReply(QPI::sint64 queryId, OracleInterface::OracleReply& reply) const
{
	return oracleEngine.getOracleReply(queryId, &reply, sizeof(reply));
}

inline QPI::uint8 QPI::QpiContextFunctionCall::getOracleQueryStatus(sint64 queryId) const
{
	return oracleEngine.getOracleQueryStatus(queryId);
}
