#pragma once

#include "contracts/qpi.h"

#include "assets/assets.h"
#include "../spectrum.h"


bool QPI::QpiContextProcedureCall::distributeDividends(long long amountPerShare) const
{
    if (amountPerShare < 0 || amountPerShare * NUMBER_OF_COMPUTORS > MAX_AMOUNT)
    {
        return false;
    }

    const int index = spectrumIndex(_currentContractId);

    if (index < 0)
    {
        return false;
    }

    const long long remainingAmount = energy(index) - amountPerShare * NUMBER_OF_COMPUTORS;

    if (remainingAmount < 0)
    {
        return false;
    }

    if (decreaseEnergy(index, amountPerShare * NUMBER_OF_COMPUTORS))
    {
        ACQUIRE(universeLock);

        for (int issuanceIndex = 0; issuanceIndex < ASSETS_CAPACITY; issuanceIndex++)
        {
            if (((*((unsigned long long*)assets[issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == *((unsigned long long*)contractDescriptions[_currentContractIndex].assetName)
                && assets[issuanceIndex].varStruct.issuance.type == ISSUANCE
                && isZero(assets[issuanceIndex].varStruct.issuance.publicKey))
            {
                long long shareholderCounter = 0;
                for (int ownershipIndex = 0; shareholderCounter < NUMBER_OF_COMPUTORS && ownershipIndex < ASSETS_CAPACITY; ownershipIndex++)
                {
                    if (assets[ownershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
                        && assets[ownershipIndex].varStruct.ownership.type == OWNERSHIP)
                    {
                        long long possessorCounter = 0;

                        for (int possessionIndex = 0; possessorCounter < assets[ownershipIndex].varStruct.ownership.numberOfShares && possessionIndex < ASSETS_CAPACITY; possessionIndex++)
                        {
                            if (assets[possessionIndex].varStruct.possession.ownershipIndex == ownershipIndex
                                && assets[possessionIndex].varStruct.possession.type == POSSESSION)
                            {
                                possessorCounter += assets[possessionIndex].varStruct.possession.numberOfShares;

                                increaseEnergy(assets[possessionIndex].varStruct.possession.publicKey, amountPerShare * assets[possessionIndex].varStruct.possession.numberOfShares);

                                if (!contractActionTracker.addQuTransfer(_currentContractId, assets[possessionIndex].varStruct.possession.publicKey, amountPerShare * assets[possessionIndex].varStruct.possession.numberOfShares))
                                    __qpiAbort(ContractErrorTooManyActions);

                                const QuTransfer quTransfer = { _currentContractId , assets[possessionIndex].varStruct.possession.publicKey , amountPerShare * assets[possessionIndex].varStruct.possession.numberOfShares };
                                logger.logQuTransfer(quTransfer);
                            }
                        }

                        shareholderCounter += possessorCounter;
                    }
                }

                break;
            }
        }

        RELEASE(universeLock);
    }

    return true;
}


long long QPI::QpiContextProcedureCall::issueAsset(unsigned long long name, const QPI::id& issuer, signed char numberOfDecimalPlaces, long long numberOfShares, unsigned long long unitOfMeasurement) const
{
    if (((unsigned char)name) < 'A' || ((unsigned char)name) > 'Z'
        || name > 0xFFFFFFFFFFFFFF)
    {
        return 0;
    }
    for (unsigned int i = 1; i < 7; i++)
    {
        if (!((unsigned char)(name >> (i * 8))))
        {
            while (++i < 7)
            {
                if ((unsigned char)(name >> (i * 8)))
                {
                    return 0;
                }
            }

            break;
        }
    }
    for (unsigned int i = 1; i < 7; i++)
    {
        if (!((unsigned char)(name >> (i * 8)))
            || (((unsigned char)(name >> (i * 8))) >= '0' && ((unsigned char)(name >> (i * 8))) <= '9')
            || (((unsigned char)(name >> (i * 8))) >= 'A' && ((unsigned char)(name >> (i * 8))) <= 'Z'))
        {
            // Do nothing
        }
        else
        {
            return 0;
        }
    }

    if (issuer != _currentContractId && issuer != _invocator)
    {
        return 0;
    }

    if (numberOfShares <= 0 || numberOfShares > MAX_AMOUNT)
    {
        return 0;
    }

    if (unitOfMeasurement > 0xFFFFFFFFFFFFFF)
    {
        return 0;
    }

    char nameBuffer[7] = { char(name), char(name >> 8), char(name >> 16), char(name >> 24), char(name >> 32), char(name >> 40), char(name >> 48) };
    char unitOfMeasurementBuffer[7] = { char(unitOfMeasurement), char(unitOfMeasurement >> 8), char(unitOfMeasurement >> 16), char(unitOfMeasurement >> 24), char(unitOfMeasurement >> 32), char(unitOfMeasurement >> 40), char(unitOfMeasurement >> 48) };
    int issuanceIndex, ownershipIndex, possessionIndex;
    numberOfShares = ::issueAsset(issuer, nameBuffer, numberOfDecimalPlaces, unitOfMeasurementBuffer, numberOfShares, _currentContractIndex, &issuanceIndex, &ownershipIndex, &possessionIndex);

    return numberOfShares;
}


long long QPI::QpiContextFunctionCall::numberOfPossessedShares(unsigned long long assetName, const m256i& issuer, const m256i& owner, const m256i& possessor, unsigned short ownershipManagingContractIndex, unsigned short possessionManagingContractIndex) const
{
    ACQUIRE(universeLock);

    int issuanceIndex = issuer.m256i_u32[0] & (ASSETS_CAPACITY - 1);
iteration:
    if (assets[issuanceIndex].varStruct.issuance.type == EMPTY)
    {
        RELEASE(universeLock);

        return 0;
    }
    else
    {
        if (assets[issuanceIndex].varStruct.issuance.type == ISSUANCE
            && ((*((unsigned long long*)assets[issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == assetName
            && assets[issuanceIndex].varStruct.issuance.publicKey == issuer)
        {
            int ownershipIndex = owner.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        iteration2:
            if (assets[ownershipIndex].varStruct.ownership.type == EMPTY)
            {
                RELEASE(universeLock);

                return 0;
            }
            else
            {
                if (assets[ownershipIndex].varStruct.ownership.type == OWNERSHIP
                    && assets[ownershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
                    && assets[ownershipIndex].varStruct.ownership.publicKey == owner
                    && assets[ownershipIndex].varStruct.ownership.managingContractIndex == ownershipManagingContractIndex)
                {
                    int possessionIndex = possessor.m256i_u32[0] & (ASSETS_CAPACITY - 1);
                iteration3:
                    if (assets[possessionIndex].varStruct.possession.type == EMPTY)
                    {
                        RELEASE(universeLock);

                        return 0;
                    }
                    else
                    {
                        if (assets[possessionIndex].varStruct.possession.type == POSSESSION
                            && assets[possessionIndex].varStruct.possession.ownershipIndex == ownershipIndex
                            && assets[possessionIndex].varStruct.possession.publicKey == possessor
                            && assets[possessionIndex].varStruct.possession.managingContractIndex == possessionManagingContractIndex)
                        {
                            const long long numberOfPossessedShares = assets[possessionIndex].varStruct.possession.numberOfShares;

                            RELEASE(universeLock);

                            return numberOfPossessedShares;
                        }
                        else
                        {
                            possessionIndex = (possessionIndex + 1) & (ASSETS_CAPACITY - 1);

                            goto iteration3;
                        }
                    }
                }
                else
                {
                    ownershipIndex = (ownershipIndex + 1) & (ASSETS_CAPACITY - 1);

                    goto iteration2;
                }
            }
        }
        else
        {
            issuanceIndex = (issuanceIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration;
        }
    }
}


long long QPI::QpiContextProcedureCall::transferShareOwnershipAndPossession(unsigned long long assetName, const m256i& issuer, const m256i& owner, const m256i& possessor, long long numberOfShares, const m256i& newOwnerAndPossessor) const
{
    if (numberOfShares <= 0 || numberOfShares > MAX_AMOUNT)
    {
        return -((long long)(MAX_AMOUNT + 1));
    }

    ACQUIRE(universeLock);

    int issuanceIndex = issuer.m256i_u32[0] & (ASSETS_CAPACITY - 1);
iteration:
    if (assets[issuanceIndex].varStruct.issuance.type == EMPTY)
    {
        RELEASE(universeLock);

        return -numberOfShares;
    }
    else
    {
        if (assets[issuanceIndex].varStruct.issuance.type == ISSUANCE
            && ((*((unsigned long long*)assets[issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == assetName
            && assets[issuanceIndex].varStruct.issuance.publicKey == issuer)
        {
            int ownershipIndex = owner.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        iteration2:
            if (assets[ownershipIndex].varStruct.ownership.type == EMPTY)
            {
                RELEASE(universeLock);

                return -numberOfShares;
            }
            else
            {
                if (assets[ownershipIndex].varStruct.ownership.type == OWNERSHIP
                    && assets[ownershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
                    && assets[ownershipIndex].varStruct.ownership.publicKey == owner
                    && assets[ownershipIndex].varStruct.ownership.managingContractIndex == _currentContractIndex) // TODO: This condition needs extra attention during refactoring!
                {
                    int possessionIndex = possessor.m256i_u32[0] & (ASSETS_CAPACITY - 1);
                iteration3:
                    if (assets[possessionIndex].varStruct.possession.type == EMPTY)
                    {
                        RELEASE(universeLock);

                        return -numberOfShares;
                    }
                    else
                    {
                        if (assets[possessionIndex].varStruct.possession.type == POSSESSION
                            && assets[possessionIndex].varStruct.possession.ownershipIndex == ownershipIndex
                            && assets[possessionIndex].varStruct.possession.publicKey == possessor)
                        {
                            if (assets[possessionIndex].varStruct.possession.managingContractIndex == _currentContractIndex) // TODO: This condition needs extra attention during refactoring!
                            {
                                if (assets[possessionIndex].varStruct.possession.numberOfShares >= numberOfShares)
                                {
                                    int destinationOwnershipIndex, destinationPossessionIndex;
                                    ::transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, newOwnerAndPossessor, numberOfShares, &destinationOwnershipIndex, &destinationPossessionIndex, false);

                                    RELEASE(universeLock);

                                    return assets[possessionIndex].varStruct.possession.numberOfShares;
                                }
                                else
                                {
                                    RELEASE(universeLock);

                                    return assets[possessionIndex].varStruct.possession.numberOfShares - numberOfShares;
                                }
                            }
                            else
                            {
                                RELEASE(universeLock);

                                return -numberOfShares;
                            }
                        }
                        else
                        {
                            possessionIndex = (possessionIndex + 1) & (ASSETS_CAPACITY - 1);

                            goto iteration3;
                        }
                    }
                }
                else
                {
                    ownershipIndex = (ownershipIndex + 1) & (ASSETS_CAPACITY - 1);

                    goto iteration2;
                }
            }
        }
        else
        {
            issuanceIndex = (issuanceIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration;
        }
    }
}
