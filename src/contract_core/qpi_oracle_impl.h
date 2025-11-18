#pragma once

#include "contracts/qpi.h"
#include "oracle_core/oracle_engine.h"
#include "spectrum/spectrum.h"

template <typename OracleInterface>
QPI::uint64 QPI::QpiContextProcedureCall::queryOracle(const OracleInterface::OracleQuery& query, uint16 timeoutSeconds) const
{
	// get fee
	sint64 fee = OracleInterface::getRequestFee(query);
	if (fee < 0)
		return 0;

	// destroy fee (not adding to contracts execution fee reserve)
	int contractSpectrumIdx = ::spectrumIndex(this->_currentContractId);
	if (contractSpectrumIdx < 0 || !decreaseEnergy(contractSpectrumIdx, fee))
		return 0;

	ASSERT(this->_currentContractIndex < 0xffff);
	QPI::uint16 contractIndex = static_cast<QPI::uint16>(this->_currentContractIndex);

	return oracleEngine.startContractQuery(contractIndex, OracleInterface::oracleInterfaceIndex, &query, sizeof(query), timeoutSeconds);
}
