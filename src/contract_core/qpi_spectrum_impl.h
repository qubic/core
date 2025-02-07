#pragma once

#include "contract_core/contract_exec.h"
#include "spectrum/spectrum.h"


bool QPI::QpiContextFunctionCall::getEntity(const m256i& id, QPI::Entity& entity) const
{
    int index = spectrumIndex(id);
    if (index < 0)
    {
        entity.publicKey = id;
        entity.incomingAmount = 0;
        entity.outgoingAmount = 0;
        entity.numberOfIncomingTransfers = 0;
        entity.numberOfOutgoingTransfers = 0;
        entity.latestIncomingTransferTick = 0;
        entity.latestOutgoingTransferTick = 0;

        return false;
    }
    else
    {
        entity.publicKey = spectrum[index].publicKey;
        entity.incomingAmount = spectrum[index].incomingAmount;
        entity.outgoingAmount = spectrum[index].outgoingAmount;
        entity.numberOfIncomingTransfers = spectrum[index].numberOfIncomingTransfers;
        entity.numberOfOutgoingTransfers = spectrum[index].numberOfOutgoingTransfers;
        entity.latestIncomingTransferTick = spectrum[index].latestIncomingTransferTick;
        entity.latestOutgoingTransferTick = spectrum[index].latestOutgoingTransferTick;

        return true;
    }
}

// Return reference to fee reserve of contract for changing its value (data stored in state of contract 0)
static long long& contractFeeReserve(unsigned int contractIndex)
{
    contractStateChangeFlags[0] |= 1ULL;
    return ((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex];
}

long long QPI::QpiContextProcedureCall::burn(long long amount) const
{
    if (amount < 0 || amount > MAX_AMOUNT)
    {
        return -((long long)(MAX_AMOUNT + 1));
    }

    const int index = spectrumIndex(_currentContractId);

    if (index < 0)
    {
        return -amount;
    }

    const long long remainingAmount = energy(index) - amount;

    if (remainingAmount < 0)
    {
        return remainingAmount;
    }

    if (decreaseEnergy(index, amount))
    {
        contractStateLock[0].acquireWrite();
        contractFeeReserve(_currentContractIndex) += amount;
        contractStateLock[0].releaseWrite();

        const Burning burning = { _currentContractId , amount };
        logger.logBurning(burning);
    }

    return remainingAmount;
}

long long QPI::QpiContextProcedureCall::transfer(const m256i& destination, long long amount) const
{
    if (amount < 0 || amount > MAX_AMOUNT)
    {
        return -((long long)(MAX_AMOUNT + 1));
    }

    const int index = spectrumIndex(_currentContractId);

    if (index < 0)
    {
        return -amount;
    }

    const long long remainingAmount = energy(index) - amount;

    if (remainingAmount < 0)
    {
        return remainingAmount;
    }

    if (decreaseEnergy(index, amount))
    {
        increaseEnergy(destination, amount);

        if (!contractActionTracker.addQuTransfer(_currentContractId, destination, amount))
            __qpiAbort(ContractErrorTooManyActions);

        const QuTransfer quTransfer = { _currentContractId , destination , amount };
        logger.logQuTransfer(quTransfer);
    }

    return remainingAmount;
}

m256i QPI::QpiContextFunctionCall::nextId(const m256i& currentId) const
{
    int index = spectrumIndex(currentId);
    while (++index < SPECTRUM_CAPACITY)
    {
        const m256i& nextId = spectrum[index].publicKey;
        if (!isZero(nextId))
        {
            return nextId;
        }
    }

    return m256i::zero();
}

m256i QPI::QpiContextFunctionCall::prevId(const m256i& currentId) const
{
    int index = spectrumIndex(currentId);
    while (--index >= 0)
    {
        const m256i& prevId = spectrum[index].publicKey;
        if (!isZero(prevId))
        {
            return prevId;
        }
    }

    return m256i::zero();
}
