#pragma once

#include "contract_core/ipo.h"

bool QPI::QpiContextProcedureCall::bidInIPO(unsigned int IPOContractIndex, long long price, unsigned int quantity) const
{
    if (_currentContractIndex >= contractCount || IPOContractIndex >= contractCount || _currentContractIndex >= IPOContractIndex)
    {
        return false;
    }

    if (system.epoch >= contractDescriptions[IPOContractIndex].constructionEpoch)  // IPO is finished.
    {
        return false;
    }

    const int spectrumIndex = ::spectrumIndex(_currentContractId);

    if (contractCallbacksRunning != NoContractCallback || spectrumIndex < 0)
    {
        return false;
    }

    return bidInContractIPO(price, quantity, _currentContractId, spectrumIndex, IPOContractIndex);
}
