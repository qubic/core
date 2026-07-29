#pragma once

#include "qpi/qpi.h"
#include "oc_core/oc_engine.h"
#include "spectrum/spectrum.h"


template <typename OcInterface>
QPI::sint64 QPI::QpiContextProcedureCall::__qpiInvokeOC(
    const typename OcInterface::OcRequest& request
) const
{
    // compile-time checks
    static_assert(sizeof(typename OcInterface::OcRequest) <= MAX_OC_REQUEST_SIZE);
    static_assert(OcInterface::ocInterfaceIndex < OCI::ocInterfacesCount);
    static_assert(OCI::ocInterfaces[OcInterface::ocInterfaceIndex].requestSize == sizeof(typename OcInterface::OcRequest));

    // validate contract index
    ASSERT(this->_currentContractIndex < 0xffff);
    if (this->_currentContractIndex >= MAX_NUMBER_OF_CONTRACTS)
        return -1;
    const QPI::uint16 contractIndex = static_cast<QPI::uint16>(this->_currentContractIndex);

    // compute fee
    const sint64 fee = OCI::getOcInvocationFeeFunc[OcInterface::ocInterfaceIndex](&request);
    if (fee < MIN_OC_INVOCATION_FEE)
        return -1;

    // decrease contract spectrum balance
    const int contractSpectrumIdx = ::spectrumIndex(this->_currentContractId);
    if (contractSpectrumIdx < 0 || !decreaseEnergy(contractSpectrumIdx, fee))
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        addDebugMessage(L"INVOKE_OC failed: insufficient contract balance for invocation fee");
#endif
        return -1;
    }

    // log fee burn
    const QuTransfer feeBurn = { this->_currentContractId, m256i::zero(), fee };
    logger.logQuTransfer(feeBurn);

    // record invocation
    const sint64 invocationId = ocEngine.startContractInvocation(
        contractIndex, OcInterface::ocInterfaceIndex,
        &request, sizeof(typename OcInterface::OcRequest));

    if (invocationId < 0)
    {
        // engine refused (storage exhaustion etc.) — refund fee
        OcEngine::refundFees(this->_currentContractId, fee);
        return -1;
    }

    return invocationId;
}


inline QPI::uint8 QPI::QpiContextFunctionCall::getOcInvocationStatus(sint64 invocationId) const
{
    return ocEngine.getOcInvocationStatus(invocationId);
}
