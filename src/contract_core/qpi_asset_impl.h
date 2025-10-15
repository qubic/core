#pragma once

#include "contracts/qpi.h"

#include "assets/assets.h"
#include "spectrum/spectrum.h"



// Start iteration with issuance filter (selects first record).
void QPI::AssetIssuanceIterator::begin(const QPI::AssetIssuanceSelect& issuance)
{
    _issuance = issuance;
    _issuanceIdx = NO_ASSET_INDEX;

    next();
}

// Return if iteration with next() has reached end.
bool QPI::AssetIssuanceIterator::reachedEnd() const
{
    return _issuanceIdx == NO_ASSET_INDEX;
}

// Step to next issuance record matching filtering criteria.
bool QPI::AssetIssuanceIterator::next()
{
    ASSERT(_issuanceIdx < ASSETS_CAPACITY || _issuanceIdx == NO_ASSET_INDEX);

    if (!_issuance.anyIssuer)
    {
        // searching for specific issuer -> use hash map
        if (_issuanceIdx == NO_ASSET_INDEX)
        {
            // get first candidate for issuance in first call of next()
            _issuanceIdx = _issuance.issuer.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        }
        else
        {
            // get next candidate in following calls of next()
            _issuanceIdx = (_issuanceIdx + 1) & (ASSETS_CAPACITY - 1);
        }

        // search entry in consecutive non-empty fields of hash map
        while (assets[_issuanceIdx].varStruct.issuance.type != EMPTY)
        {
            if (assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE
                && assets[_issuanceIdx].varStruct.issuance.publicKey == _issuance.issuer
                && (_issuance.anyName || ((*((unsigned long long*)assets[_issuanceIdx].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == _issuance.assetName))
            {
                // found matching entry
                return true;
            }

            _issuanceIdx = (_issuanceIdx + 1) & (ASSETS_CAPACITY - 1);
        }

        // no matching entry found
        _issuanceIdx = NO_ASSET_INDEX;
        return false;
    }
    else
    {
        // issuer is unknow -> use index lists instead of hash map to iterate through issuances
        if (_issuanceIdx == NO_ASSET_INDEX)
        {
            // get first issuance
            _issuanceIdx = as.indexLists.issuancesFirstIdx;
        }
        else
        {
            // get next issuance
            _issuanceIdx = as.indexLists.nextIdx[_issuanceIdx];
        }
        ASSERT(_issuanceIdx == NO_ASSET_INDEX
            || (_issuanceIdx < ASSETS_CAPACITY
                && assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE));

        // if specific asset name is requested, make sure the issuance matches
        if (!_issuance.anyName)
        {
            while (_issuanceIdx != NO_ASSET_INDEX
                && ((*((unsigned long long*)assets[_issuanceIdx].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) != _issuance.assetName)
            {
                _issuanceIdx = as.indexLists.nextIdx[_issuanceIdx];
                ASSERT(_issuanceIdx == NO_ASSET_INDEX
                    || (_issuanceIdx < ASSETS_CAPACITY
                        && assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE));
            }
        }

        return _issuanceIdx != NO_ASSET_INDEX;
    }
}

id QPI::AssetIssuanceIterator::issuer() const
{
    ASSERT(_issuanceIdx == NO_ASSET_INDEX || (_issuanceIdx < ASSETS_CAPACITY && assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE));
    return (_issuanceIdx < ASSETS_CAPACITY) ? assets[_issuanceIdx].varStruct.issuance.publicKey : id::zero();
}

uint64 QPI::AssetIssuanceIterator::assetName() const
{
    ASSERT(_issuanceIdx == NO_ASSET_INDEX || (_issuanceIdx < ASSETS_CAPACITY && assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE));
    return (_issuanceIdx < ASSETS_CAPACITY) ? ((*((unsigned long long*)assets[_issuanceIdx].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) : 0;
}


// Start iteration with given issuance and given ownership filter (selects first record).
void QPI::AssetOwnershipIterator::begin(const QPI::Asset& issuance, const QPI::AssetOwnershipSelect& ownership)
{
    _issuance = issuance;
    _issuanceIdx = ::issuanceIndex(issuance.issuer, issuance.assetName);
    _ownership = ownership;
    _ownershipIdx = NO_ASSET_INDEX;

    if (_issuanceIdx == NO_ASSET_INDEX)
        return;

    next();
}

// Return if iteration with next() has reached end.
bool QPI::AssetOwnershipIterator::reachedEnd() const
{
    ASSERT(!(_issuanceIdx == NO_ASSET_INDEX && _ownershipIdx != NO_ASSET_INDEX));
    return _ownershipIdx == NO_ASSET_INDEX;
}

// Step to next ownership record matching filtering criteria.
bool QPI::AssetOwnershipIterator::next()
{
    ASSERT(_issuanceIdx < ASSETS_CAPACITY);

    if (!_ownership.anyOwner)
    {
        // searching for specific owner -> use hash map
        if (_ownershipIdx == NO_ASSET_INDEX)
        {
            // get first candidate for ownership of issuance in first call of next()
            _ownershipIdx = _ownership.owner.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        }
        else
        {
            // get next candidate in following calls of next()
            _ownershipIdx = (_ownershipIdx + 1) & (ASSETS_CAPACITY - 1);
        }

        // search entry in consecutive non-empty fields of hash map
        while (assets[_ownershipIdx].varStruct.ownership.type != EMPTY)
        {
            if (assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP
                && assets[_ownershipIdx].varStruct.ownership.issuanceIndex == _issuanceIdx
                && assets[_ownershipIdx].varStruct.ownership.publicKey == _ownership.owner
                && (_ownership.anyManagingContract || assets[_ownershipIdx].varStruct.ownership.managingContractIndex == _ownership.managingContract))
            {
                // found matching entry
                return true;
            }

            _ownershipIdx = (_ownershipIdx + 1) & (ASSETS_CAPACITY - 1);
        }

        // no matching entry found
        _ownershipIdx = NO_ASSET_INDEX;
        return false;
    }
    else
    {
        // owner is unknow -> use index lists instead of hash map to iterate through ownerships belonging to issuance
        if (_ownershipIdx == NO_ASSET_INDEX)
        {
            // get first ownership of issuance
            _ownershipIdx = as.indexLists.ownershipsPossessionsFirstIdx[_issuanceIdx];
            ASSERT(_ownershipIdx == NO_ASSET_INDEX
                || (_ownershipIdx < ASSETS_CAPACITY
                    && assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP
                    && assets[_ownershipIdx].varStruct.ownership.issuanceIndex == _issuanceIdx));
        }
        else
        {
            // get next ownership
            _ownershipIdx = as.indexLists.nextIdx[_ownershipIdx];
            ASSERT(_ownershipIdx == NO_ASSET_INDEX
                || (_ownershipIdx < ASSETS_CAPACITY
                    && assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP
                    && assets[_ownershipIdx].varStruct.ownership.issuanceIndex == _issuanceIdx));
        }

        // if specific managing contract is requested, make sure the ownership matches
        if (!_ownership.anyManagingContract)
        {
            while (_ownershipIdx != NO_ASSET_INDEX
                && assets[_ownershipIdx].varStruct.ownership.managingContractIndex != _ownership.managingContract)
            {
                _ownershipIdx = as.indexLists.nextIdx[_ownershipIdx];
                ASSERT(_ownershipIdx == NO_ASSET_INDEX
                    || (_ownershipIdx < ASSETS_CAPACITY
                        && assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP
                        && assets[_ownershipIdx].varStruct.ownership.issuanceIndex == _issuanceIdx));
            }
        }

        return _ownershipIdx != NO_ASSET_INDEX;
    }
}

id QPI::AssetOwnershipIterator::issuer() const
{
    ASSERT(_issuanceIdx == NO_ASSET_INDEX || (_issuanceIdx < ASSETS_CAPACITY && assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE));
    return (_issuanceIdx < ASSETS_CAPACITY) ? assets[_issuanceIdx].varStruct.issuance.publicKey : id::zero();
}

uint64 QPI::AssetOwnershipIterator::assetName() const
{
    ASSERT(_issuanceIdx == NO_ASSET_INDEX || (_issuanceIdx < ASSETS_CAPACITY && assets[_issuanceIdx].varStruct.issuance.type == ISSUANCE));
    return (_issuanceIdx < ASSETS_CAPACITY) ? ((*((unsigned long long*)assets[_issuanceIdx].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) : 0;
}

id QPI::AssetOwnershipIterator::owner() const
{
    ASSERT(_ownershipIdx == NO_ASSET_INDEX || (_ownershipIdx < ASSETS_CAPACITY && assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP));
    return (_ownershipIdx < ASSETS_CAPACITY) ? assets[_ownershipIdx].varStruct.ownership.publicKey : id::zero();
}

sint64 QPI::AssetOwnershipIterator::numberOfOwnedShares() const
{
    ASSERT(_ownershipIdx == NO_ASSET_INDEX || (_ownershipIdx < ASSETS_CAPACITY && assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP));
    return (_ownershipIdx < ASSETS_CAPACITY) ? assets[_ownershipIdx].varStruct.ownership.numberOfShares : -1;
}

uint16 QPI::AssetOwnershipIterator::ownershipManagingContract() const
{
    ASSERT(_ownershipIdx == NO_ASSET_INDEX || (_ownershipIdx < ASSETS_CAPACITY && assets[_ownershipIdx].varStruct.ownership.type == OWNERSHIP));
    return (_ownershipIdx < ASSETS_CAPACITY) ? assets[_ownershipIdx].varStruct.ownership.managingContractIndex : 0;
}


// Start iteration with given issuance and given ownership + possession filters (selects first record).
void QPI::AssetPossessionIterator::begin(const Asset& issuance, const AssetOwnershipSelect& ownership, const AssetPossessionSelect& possession)
{
    AssetOwnershipIterator::begin(issuance, ownership);

    _possession = possession;
    _possessionIdx = NO_ASSET_INDEX;

    if (_issuanceIdx == NO_ASSET_INDEX || _ownershipIdx == NO_ASSET_INDEX)
        return;

    next();
}

// Return if iteration with next() has reached end.
bool QPI::AssetPossessionIterator::reachedEnd() const
{
    ASSERT(
        (_possessionIdx != NO_ASSET_INDEX && _ownershipIdx != NO_ASSET_INDEX && _issuanceIdx != NO_ASSET_INDEX) ||
        (_possessionIdx == NO_ASSET_INDEX && _ownershipIdx != NO_ASSET_INDEX && _issuanceIdx != NO_ASSET_INDEX) ||
        (_possessionIdx == NO_ASSET_INDEX && _ownershipIdx == NO_ASSET_INDEX && _issuanceIdx != NO_ASSET_INDEX) ||
        (_possessionIdx == NO_ASSET_INDEX && _ownershipIdx == NO_ASSET_INDEX && _issuanceIdx == NO_ASSET_INDEX)
    );
    return _possessionIdx == NO_ASSET_INDEX;
}

// Step to next possession record matching filtering criteria.
bool QPI::AssetPossessionIterator::next()
{
    ASSERT(_issuanceIdx < ASSETS_CAPACITY && _ownershipIdx < ASSETS_CAPACITY);

    if (!_possession.anyPossessor)
    {
        // searching for specific possessor -> use hash map

        // TODO: the common case of specific possessor and don't care about ownership should be optimized from O(N^2) to O(N)
        do
        {
            if (_possessionIdx == NO_ASSET_INDEX)
            {
                _possessionIdx = _possession.possessor.m256i_u32[0] & (ASSETS_CAPACITY - 1);
            }
            else
            {
                _possessionIdx = (_possessionIdx + 1) & (ASSETS_CAPACITY - 1);
            }
            while (assets[_possessionIdx].varStruct.possession.type != EMPTY)
            {
                if (assets[_possessionIdx].varStruct.possession.type == POSSESSION
                    && assets[_possessionIdx].varStruct.possession.ownershipIndex == _ownershipIdx
                    && assets[_possessionIdx].varStruct.possession.publicKey == _possession.possessor
                    && (_possession.anyManagingContract || assets[_possessionIdx].varStruct.possession.managingContractIndex == _possession.managingContract))
                {
                    // found matching entry
                    return true;
                }

                _possessionIdx = (_possessionIdx + 1) & (ASSETS_CAPACITY - 1);
            }

            // no matching entry found
            _possessionIdx = NO_ASSET_INDEX;

            // retry with next owner
        } while (AssetOwnershipIterator::next());
    }
    else
    {
        // possessor is unknow -> use index lists instead of hash map to iterate through possessions belonging to ownership
        do
        {
            if (_possessionIdx == NO_ASSET_INDEX)
            {
                // get first possession of ownership
                _possessionIdx = as.indexLists.ownershipsPossessionsFirstIdx[_ownershipIdx];
                ASSERT(_possessionIdx == NO_ASSET_INDEX
                    || (_possessionIdx < ASSETS_CAPACITY
                        && assets[_possessionIdx].varStruct.possession.type == POSSESSION
                        && assets[_possessionIdx].varStruct.possession.ownershipIndex == _ownershipIdx));
            }
            else
            {
                // get next ownership
                _possessionIdx = as.indexLists.nextIdx[_possessionIdx];
                ASSERT(_possessionIdx == NO_ASSET_INDEX
                    || (_possessionIdx < ASSETS_CAPACITY
                        && assets[_possessionIdx].varStruct.possession.type == POSSESSION
                        && assets[_possessionIdx].varStruct.possession.ownershipIndex == _ownershipIdx));
            }

            // if specific managing contract is requested, make sure the possession matches
            if (!_possession.anyManagingContract)
            {
                while (_possessionIdx != NO_ASSET_INDEX
                    && assets[_possessionIdx].varStruct.possession.managingContractIndex != _possession.managingContract)
                {
                    _possessionIdx = as.indexLists.nextIdx[_possessionIdx];
                    ASSERT(_possessionIdx == NO_ASSET_INDEX
                        || (_possessionIdx < ASSETS_CAPACITY
                            && assets[_possessionIdx].varStruct.possession.type == POSSESSION
                            && assets[_possessionIdx].varStruct.possession.ownershipIndex == _ownershipIdx));
                }
            }

            if (_possessionIdx != NO_ASSET_INDEX)
                return true;

            // no matching entry found -> retry with next owner
        } while (AssetOwnershipIterator::next());
    }

    ASSERT(_ownershipIdx == NO_ASSET_INDEX && _possessionIdx == NO_ASSET_INDEX);

    return false;
}

inline id QPI::AssetPossessionIterator::possessor() const
{
    ASSERT(_possessionIdx == NO_ASSET_INDEX || (_possessionIdx < ASSETS_CAPACITY && assets[_possessionIdx].varStruct.possession.type == POSSESSION));
    return (_possessionIdx < ASSETS_CAPACITY) ? assets[_possessionIdx].varStruct.possession.publicKey : id::zero();
}

sint64 QPI::AssetPossessionIterator::numberOfPossessedShares() const
{
    ASSERT(_possessionIdx == NO_ASSET_INDEX || (_possessionIdx < ASSETS_CAPACITY && assets[_possessionIdx].varStruct.possession.type == POSSESSION));
    return (_possessionIdx < ASSETS_CAPACITY) ? assets[_possessionIdx].varStruct.possession.numberOfShares : -1;
}

uint16 QPI::AssetPossessionIterator::possessionManagingContract() const
{
    ASSERT(_possessionIdx == NO_ASSET_INDEX || (_possessionIdx < ASSETS_CAPACITY && assets[_possessionIdx].varStruct.possession.type == POSSESSION));
    return (_possessionIdx < ASSETS_CAPACITY) ? assets[_possessionIdx].varStruct.possession.managingContractIndex : 0;
}


////////////////////////////


sint64 QPI::QpiContextProcedureCall::acquireShares(
    const Asset& asset, const id& owner, const id& possessor, sint64 numberOfShares,
    uint16 sourceOwnershipManagingContractIndex, uint16 sourcePossessionManagingContractIndex,
    sint64 offeredTransferFee) const
{
    // prevent nested calling of management rights transfer from callbacks
    if (contractCallbacksRunning & ContractCallbackManagementRightsTransfer)
    {
        return INVALID_AMOUNT;
    }

    // check for unsupported cases
    if (sourceOwnershipManagingContractIndex != sourcePossessionManagingContractIndex
        || owner != possessor)
    {
        return INVALID_AMOUNT;
    }

    // check for invalid inputs
    if (sourcePossessionManagingContractIndex == _currentContractIndex
        || sourcePossessionManagingContractIndex == 0
        || sourcePossessionManagingContractIndex >= contractCount
        || numberOfShares <= 0
        || offeredTransferFee < 0)
    {
        return INVALID_AMOUNT;
    }

    // currently, only contract with index that fits into 16 bits can be managing contract
    if (_currentContractIndex > 0xffffu || _currentContractIndex >= contractCount)
    {
        return INVALID_AMOUNT;
    }
    uint16 currentContractIndex = static_cast<uint16>(this->_currentContractIndex);

    // find records in universe of given asset, owner, and possessor that are under management of contract
    AssetPossessionIterator it(asset,
        AssetOwnershipSelect{ owner, sourceOwnershipManagingContractIndex },
        AssetPossessionSelect{ possessor, sourcePossessionManagingContractIndex });

    // check that number of shares is sufficient
    sint64 possessedSharesUnderManagement = it.numberOfPossessedShares();
    if (possessedSharesUnderManagement < numberOfShares)
    {
        return INVALID_AMOUNT;
    }

    // run PRE_RELEASE_SHARES callback in other contract (without invocation reward)
    QPI::PreManagementRightsTransfer_input pre_input{ asset, owner, possessor, numberOfShares, offeredTransferFee, currentContractIndex };
    QPI::PreManagementRightsTransfer_output pre_output; // output is zeroed in __qpiCallSystemProc
    __qpiCallSystemProc<PRE_RELEASE_SHARES>(sourceOwnershipManagingContractIndex, pre_input, pre_output, 0);
    if (!pre_output.allowTransfer || pre_output.requestedFee < 0 || pre_output.requestedFee > MAX_AMOUNT)
    {
        return INVALID_AMOUNT;
    }
    if (pre_output.requestedFee > offeredTransferFee)
    {
        return -pre_output.requestedFee;
    }

    // transfer requested fee
    if (transfer(id(sourceOwnershipManagingContractIndex, 0, 0, 0), pre_output.requestedFee) < 0)
    {
        return (pre_output.requestedFee) ? -pre_output.requestedFee : INVALID_AMOUNT;
    }

    // transfer management rights
    if (!transferShareManagementRights(it.ownershipIndex(), it.possessionIndex(),
            currentContractIndex, currentContractIndex,
            numberOfShares, nullptr, nullptr, true))
    {
        return INVALID_AMOUNT;
    }

    // run POST_RELEASE_SHARES in other contract (without invocation reward)
    QPI::PostManagementRightsTransfer_input post_input{ asset, owner, possessor, numberOfShares, pre_output.requestedFee, currentContractIndex };
    QPI::NoData post_output;
    __qpiCallSystemProc<POST_RELEASE_SHARES>(sourceOwnershipManagingContractIndex, post_input, post_output, 0);

    // bug check: no other record matches filter criteria
    ASSERT(!it.next());

    return pre_output.requestedFee;
}


bool QPI::QpiContextProcedureCall::distributeDividends(long long amountPerShare) const
{
    if (contractCallbacksRunning & ContractCallbackPostIncomingTransfer)
    {
        return false;
    }

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

    // this part of code doesn't perform completed QuTransfers, instead it `decreaseEnergy` all QUs at once and `increaseEnergy` multiple times.
    // Meanwhile, a QUTransfer requires a pair of both decrease & increase calls.
    // This behavior will produce different numberOfOutgoingTransfers for the SC index.
    // 3rd party software needs to catch the HINT message to know the distribute dividends operation
    DummyCustomMessage dcm{ CUSTOM_MESSAGE_OP_START_DISTRIBUTE_DIVIDENDS };
    logger.logCustomMessage(dcm);

    if (decreaseEnergy(index, amountPerShare * NUMBER_OF_COMPUTORS))
    {
        ACQUIRE(universeLock);

        Asset asset(id::zero(), *((unsigned long long*)contractDescriptions[_currentContractIndex].assetName));
        AssetPossessionIterator iter(asset);
        long long totalShareCounter = 0;

        while (!iter.reachedEnd())
        {
            ASSERT(iter.possessionIndex() < ASSETS_CAPACITY);

            const auto& possession = assets[iter.possessionIndex()].varStruct.possession;

            if (possession.numberOfShares)
            {
                const long long dividend = amountPerShare * possession.numberOfShares;
                increaseEnergy(possession.publicKey, dividend);

                if (!contractActionTracker.addQuTransfer(_currentContractId, possession.publicKey, dividend))
                    __qpiAbort(ContractErrorTooManyActions);

                __qpiNotifyPostIncomingTransfer(_currentContractId, possession.publicKey, dividend, TransferType::qpiDistributeDividends);

                const QuTransfer quTransfer = { _currentContractId, possession.publicKey, dividend };
                logger.logQuTransfer(quTransfer);

                totalShareCounter += possession.numberOfShares;
            }

            iter.next();
        }

        ASSERT(totalShareCounter == NUMBER_OF_COMPUTORS || totalShareCounter == 0);

        RELEASE(universeLock);
    }
    dcm = DummyCustomMessage{ CUSTOM_MESSAGE_OP_END_DISTRIBUTE_DIVIDENDS };
    logger.logCustomMessage(dcm);
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

    // Any time an asset is issued via QPI either invocator or contract can be the issuer. Zero is prohibited in this case.
    if (isZero(issuer) || (issuer != _currentContractId && issuer != _invocator))
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


// TODO: remove after testing period, because numberOfShares() can do this and more
long long QPI::QpiContextFunctionCall::numberOfPossessedShares(unsigned long long assetName, const m256i& issuer, const m256i& owner, const m256i& possessor, unsigned short ownershipManagingContractIndex, unsigned short possessionManagingContractIndex) const
{
    return ::numberOfPossessedShares(assetName, issuer, owner, possessor, ownershipManagingContractIndex, possessionManagingContractIndex);
}

sint64 QPI::QpiContextFunctionCall::numberOfShares(const QPI::Asset& asset, const QPI::AssetOwnershipSelect& ownership, const QPI::AssetPossessionSelect& possession) const
{
    return ::numberOfShares(asset, ownership, possession);
}

sint64 QPI::QpiContextProcedureCall::releaseShares(
    const Asset& asset, const id& owner, const id& possessor, sint64 numberOfShares,
    uint16 destinationOwnershipManagingContractIndex, uint16 destinationPossessionManagingContractIndex,
    sint64 offeredTransferFee) const
{
    // prevent nested calling of management rights transfer from callbacks
    if (contractCallbacksRunning & ContractCallbackManagementRightsTransfer)
    {
        return INVALID_AMOUNT;
    }

    // check for unsupported cases
    if (destinationOwnershipManagingContractIndex != destinationPossessionManagingContractIndex
        || owner != possessor)
    {
        return INVALID_AMOUNT;
    }

    // check for invalid inputs
    if (destinationOwnershipManagingContractIndex == _currentContractIndex
        || destinationOwnershipManagingContractIndex == 0
        || destinationOwnershipManagingContractIndex >= contractCount
        || numberOfShares <= 0
        || offeredTransferFee < 0)
    {
        return INVALID_AMOUNT;
    }

    // currently, only contract with index that fits into 16 bits can be managing contract
    if (_currentContractIndex > 0xffffu || _currentContractIndex >= contractCount)
    {
        return INVALID_AMOUNT;
    }
    uint16 currentContractIndex = static_cast<uint16>(this->_currentContractIndex);

    // find records in universe of given asset, owner, and possessor that are under management of contract
    AssetPossessionIterator it(asset, AssetOwnershipSelect{ owner, currentContractIndex }, AssetPossessionSelect{ possessor, currentContractIndex });

    // check that number of shares is sufficient
    sint64 possessedSharesUnderManagement = it.numberOfPossessedShares();
    if (possessedSharesUnderManagement < numberOfShares)
    {
        return INVALID_AMOUNT;
    }

    // run PRE_ACQUIRE_SHARES callback in other contract (without invocation reward)
    QPI::PreManagementRightsTransfer_input pre_input{ asset, owner, possessor, numberOfShares, offeredTransferFee, currentContractIndex };
    QPI::PreManagementRightsTransfer_output pre_output; // output is zeroed in __qpiCallSystemProc
    __qpiCallSystemProc<PRE_ACQUIRE_SHARES>(destinationOwnershipManagingContractIndex, pre_input, pre_output, 0);
    if (!pre_output.allowTransfer || pre_output.requestedFee < 0 || pre_output.requestedFee > MAX_AMOUNT)
    {
        return INVALID_AMOUNT;
    }
    if (pre_output.requestedFee > offeredTransferFee)
    {
        return -pre_output.requestedFee;
    }

    // transfer requested fee
    if (transfer(id(destinationOwnershipManagingContractIndex, 0, 0, 0), pre_output.requestedFee) < 0)
    {
        return (pre_output.requestedFee) ? -pre_output.requestedFee : INVALID_AMOUNT;
    }

    // transfer management rights
    if (!transferShareManagementRights(it.ownershipIndex(), it.possessionIndex(),
            destinationOwnershipManagingContractIndex, destinationPossessionManagingContractIndex,
            numberOfShares, nullptr, nullptr, true))
    {
        return INVALID_AMOUNT;
    }

    // run POST_ACQUIRE_SHARES in other contract (without invocation reward)
    QPI::PostManagementRightsTransfer_input post_input{ asset, owner, possessor, numberOfShares, pre_output.requestedFee, currentContractIndex };
    QPI::NoData post_output;
    __qpiCallSystemProc<POST_ACQUIRE_SHARES>(destinationOwnershipManagingContractIndex, post_input, post_output, 0);

    // bug check: no other record matches filter criteria
    ASSERT(!it.next());

    return pre_output.requestedFee;
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
                                    if (!::transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, newOwnerAndPossessor, numberOfShares, &destinationOwnershipIndex, &destinationPossessionIndex, false))
                                    {
                                        RELEASE(universeLock);

                                        return INVALID_AMOUNT;
                                    }
                                    else
                                    {
                                        RELEASE(universeLock);

                                        return assets[possessionIndex].varStruct.possession.numberOfShares;
                                    }
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

bool QPI::QpiContextFunctionCall::isAssetIssued(const m256i& issuer, unsigned long long assetName) const
{
    bool res = ::issuanceIndex(issuer, assetName) != NO_ASSET_INDEX;
    return res;
}
