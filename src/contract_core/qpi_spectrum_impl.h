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

// Return the amount in the fee reserve of the specified contract (data stored in state of contract 0).
static long long getContractFeeReserve(unsigned int contractIndex)
{
    contractStateLock[0].acquireRead();
    long long reserveAmount = ((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex];
    contractStateLock[0].releaseRead();

    return reserveAmount;
}

// Set the amount in the fee reserve of the specified contract to a new value (data stored in state of contract 0).
// This also sets the contractStateChangeFlag of contract 0.
static void setContractFeeReserve(unsigned int contractIndex, long long newValue)
{
    contractStateLock[0].acquireWrite();
    contractStateChangeFlags[0] |= 1ULL;
    ((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex] = newValue;
    contractStateLock[0].releaseWrite();
}

// Add the given amount to the amount in the fee reserve of the specified contract (data stored in state of contract 0).
// This also sets the contractStateChangeFlag of contract 0.
static void addToContractFeeReserve(unsigned int contractIndex, unsigned long long addAmount)
{
    contractStateLock[0].acquireWrite();
    contractStateChangeFlags[0] |= 1ULL;
    if (addAmount > static_cast<unsigned long long>(INT64_MAX))
        addAmount = INT64_MAX;
    ((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex] =
        math_lib::sadd(((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex], static_cast<long long>(addAmount));
    contractStateLock[0].releaseWrite();
}

// Subtract the given amount from the amount in the fee reserve of the specified contract (data stored in state of contract 0).
// This also sets the contractStateChangeFlag of contract 0.
static void subtractFromContractFeeReserve(unsigned int contractIndex, unsigned long long subtractAmount)
{
    contractStateLock[0].acquireWrite();
    contractStateChangeFlags[0] |= 1ULL;

    long long negativeAddAmount;
    // The smallest representable INT64 number is INT64_MIN = - INT64_MAX - 1
    if (subtractAmount > static_cast<unsigned long long>(INT64_MAX))
        negativeAddAmount = INT64_MIN;
    else
        negativeAddAmount = -1LL * static_cast<long long>(subtractAmount);

    ((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex] =
        math_lib::sadd(((Contract0State*)contractStates[0])->contractFeeReserves[contractIndex], negativeAddAmount);
    contractStateLock[0].releaseWrite();
}

long long QPI::QpiContextFunctionCall::queryFeeReserve(unsigned int contractIndex) const
{
    if (contractIndex < 1 || contractIndex >= contractCount)
        contractIndex = _currentContractIndex;

    return getContractFeeReserve(contractIndex);
}

long long QPI::QpiContextProcedureCall::burn(long long amount, unsigned int contractIndexBurnedFor) const
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

    if (contractIndexBurnedFor < 1 || contractIndexBurnedFor >= contractCount)
        contractIndexBurnedFor = _currentContractIndex;

    if (contractError[contractIndexBurnedFor] == ContractErrorIPOFailed)
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
        addToContractFeeReserve(contractIndexBurnedFor, amount);

        const Burning burning = { _currentContractId , amount, contractIndexBurnedFor };
        logger.logBurning(burning);
    }

    return remainingAmount;
}

long long QPI::QpiContextProcedureCall::__transfer(const m256i& destination, long long amount, unsigned char transferType) const
{
    // Transfer to contract is forbidden inside POST_INCOMING_TRANSFER to prevent nested callbacks
    if (contractCallbacksRunning & ContractCallbackPostIncomingTransfer
        && destination.u64._0 < contractCount && !destination.u64._1 && !destination.u64._2 && !destination.u64._3)
    {
        return INVALID_AMOUNT;
    }

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

        __qpiNotifyPostIncomingTransfer(_currentContractId, destination, amount, transferType);

        const QuTransfer quTransfer = { _currentContractId , destination , amount };
        logger.logQuTransfer(quTransfer);
    }

    return remainingAmount;
}

long long QPI::QpiContextProcedureCall::transfer(const m256i& destination, long long amount) const
{
    return __transfer(destination, amount, TransferType::qpiTransfer);
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
