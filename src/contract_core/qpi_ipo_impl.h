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

    return bidInContractIPO(price, quantity, _currentContractId, spectrumIndex, IPOContractIndex, this);
}

// Returns the ID of the entity who has made this IPO bid or NULL_ID if the ipoContractIndex or ipoBidIndex are invalid.
QPI::id QPI::QpiContextFunctionCall::ipoBidId(QPI::uint32 ipoContractIndex, QPI::uint32 ipoBidIndex) const
{
    if (ipoContractIndex >= contractCount || system.epoch >= contractDescriptions[ipoContractIndex].constructionEpoch || ipoBidIndex >= NUMBER_OF_COMPUTORS)
    {
        return NULL_ID;
    }

    contractStateLock[ipoContractIndex].acquireRead();
    IPO* ipo = (IPO*)contractStates[ipoContractIndex];
    QPI::id publicKey = ipo->publicKeys[ipoBidIndex];
    contractStateLock[ipoContractIndex].releaseRead();

    return publicKey;
}

// Returns the price of an IPO bid, -1 if contract index is invalid, -2 if contract is not in IPO, -3 if bid index is invalid.
QPI::sint64 QPI::QpiContextFunctionCall::ipoBidPrice(QPI::uint32 ipoContractIndex, QPI::uint32 ipoBidIndex) const
{
    if (ipoContractIndex >= contractCount)
    {
        return -1;
    }

    if (system.epoch >= contractDescriptions[ipoContractIndex].constructionEpoch)
    {
        return -2;
    }

    if (ipoBidIndex >= NUMBER_OF_COMPUTORS)
    {
        return -3;
    }

    contractStateLock[ipoContractIndex].acquireRead();
    IPO* ipo = (IPO*)contractStates[ipoContractIndex];
    QPI::sint64 price = ipo->prices[ipoBidIndex];
    contractStateLock[ipoContractIndex].releaseRead();

    return price;
}
