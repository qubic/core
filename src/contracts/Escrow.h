using namespace QPI;

constexpr uint64 ESCROW_INITIAL_MAX_DEALS = 262144ULL;
constexpr uint64 ESCROW_MAX_DEALS = ESCROW_INITIAL_MAX_DEALS * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_DEALS_PER_USER = 8;
constexpr uint64 ESCROW_MAX_ASSETS_IN_DEAL = 4;
constexpr uint64 ESCROW_MAX_RESERVED_ASSETS = ESCROW_MAX_DEALS * ESCROW_MAX_ASSETS_IN_DEAL;
constexpr uint64 ESCROW_DEAL_EXISTENCE_EPOCH_COUNT = 4;

constexpr uint64 ESCROW_BASE_FEE = 250000ULL;
constexpr uint64 ESCROW_ADDITIONAL_FEE_PERCENT = 100; // 1%
constexpr uint64 ESCROW_FEE_PER_SHARE = 1500000ULL;

constexpr uint64 ESCROW_SHAREHOLDERS_TOKEN_DISTRIBUTION_PERCENT = 9500; // 95%
constexpr uint64 ESCROW_SHAREHOLDERS_QU_DISTRIBUTION_PERCENT = 9300; // 93%
constexpr uint64 ESCROW_BURN_QU_PERCENT = 200; // 2%

struct ESCROW2
{
};

struct ESCROW : public ContractBase
{
    struct AssetWithAmount
    {
        id issuer;
        uint64 name;
        uint64 amount;
    };

    struct Deal
    {
        sint64 index;
        id acceptorId;
        uint64 offeredQU;
        uint64 offeredAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        uint64 requestedQU;
        uint64 requestedAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        uint16 creationEpoch;
    };

    struct EscrowAsset
    {
        id issuer;
        uint64 assetName;

        bool operator==(const EscrowAsset other) const
        {
            return issuer == other.issuer && assetName == other.assetName;
        }

        bool operator!=(const EscrowAsset other) const
        {
            return issuer != other.issuer || assetName != other.assetName;
        }

        inline void setFrom(const Asset& asset)
        {
            issuer = asset.issuer;
            assetName = asset.assetName;
        }
    };

    struct CreateDeal_input
    {
        id acceptorId;
        uint64 offeredQU;
        uint64 offeredAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        uint64 requestedQU;
        uint64 requestedAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
    };
    struct CreateDeal_output
    {
    };

    struct AcceptDeal_input
    {
        sint64 index;
    };
    struct AcceptDeal_output
    {
    };

    struct MakeDealPublic_input
    {
        sint64 index;
    };
    struct MakeDealPublic_output
    {
    };

    struct CancelDeal_input
    {
        sint64 index;
    };
    struct CancelDeal_output
    {
    };

    struct TransferShareManagementRights_input
    {
        Asset asset;
        sint64 amount;
        uint32 newContractIndex;
    };
    struct TransferShareManagementRights_output
    {
        sint64 transferredShares;
    };

    struct GetDeals_input
    {
        id owner;
        sint64 proposedDealsOffset;
        sint64 publicDealsOffset;
    };
    struct GetDeals_output
    {
        uint64 ownedDealsAmount;
        uint64 proposedDealsAmount;
        uint64 publicDealsAmount;
        Array<Deal, ESCROW_MAX_DEALS_PER_USER> ownedDeals;
        Array<Deal, 32> proposedDeals;
        Array<Deal, 64> publicDeals;
    };

    struct GetFreeAssetAmount_input
    {
        id owner;
        Asset asset;
    };
    struct GetFreeAssetAmount_output
    {
        sint64 freeAmount;
    };

    struct _NumberOfReservedShares_input
    {
        id owner;
        id issuer;
        uint64 assetName;
    };

    struct _NumberOfReservedShares_output
    {
        sint64 amount;
    };

    struct StateData
    {
        uint64 _earnedAmount;
        uint64 _distributedAmount;
        HashSet<EscrowAsset, ESCROW_MAX_RESERVED_ASSETS> _earnedTokens;

        sint64 _currentDealIndex;
        HashMap<sint64, Deal, ESCROW_MAX_DEALS> _deals;
        Collection<sint64, ESCROW_MAX_DEALS> _acceptorDealIndexes;
        Collection<sint64, ESCROW_MAX_DEALS> _ownerDealIndexes;
        HashMap<sint64, id, ESCROW_MAX_DEALS> _dealIndexOwnerMap;
        HashSet<id, ESCROW_MAX_DEALS> _ownersSet;
        Collection<AssetWithAmount, ESCROW_MAX_RESERVED_ASSETS> _reservedAssets;

        id _devAddress;

        _NumberOfReservedShares_input _numberOfReservedShares_input;

        _NumberOfReservedShares_output _numberOfReservedShares_output;
    };

    // ----------------------------------------------

    struct _NumberOfReservedShares_locals
    {
        sint64 elementIndex;
        AssetWithAmount assetWithAmount;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(_NumberOfReservedShares)
    {
        output.amount = 0;

        locals.elementIndex = state.get()._reservedAssets.headIndex(input.owner);
        while (locals.elementIndex != NULL_INDEX)
        {
            locals.assetWithAmount = state.get()._reservedAssets.element(locals.elementIndex);
            if (locals.assetWithAmount.name == input.assetName && locals.assetWithAmount.issuer == input.issuer)
            {
                output.amount = locals.assetWithAmount.amount;
                break;
            }

            locals.elementIndex = state.get()._reservedAssets.nextElementIndex(locals.elementIndex);
        }
    }

    struct CreateDeal_locals
    {
        Deal newDeal;
        uint64 counter;
        uint64 counter2;
        sint64 elementIndex;
        uint64 offeredQuAndFee;
        AssetWithAmount tempAssetWithAmount;
        Asset tempAsset;
        sint64 availableTokenAmount;
        bit error;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        locals.error = false;
        locals.offeredQuAndFee = 0;

        if (state.get()._ownerDealIndexes.population() >= ESCROW_MAX_DEALS
                || state.get()._ownerDealIndexes.population(qpi.invocator()) >= ESCROW_MAX_DEALS_PER_USER
                || (input.offeredAssetsNumber == 0 && input.requestedAssetsNumber == 0)
                || input.offeredQU >= MAX_AMOUNT
                || input.requestedQU >= MAX_AMOUNT
                || input.offeredAssetsNumber > ESCROW_MAX_ASSETS_IN_DEAL
                || input.requestedAssetsNumber > ESCROW_MAX_ASSETS_IN_DEAL)
        {
            locals.error = true;
        }

        for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
        {
            for (locals.counter2 = locals.counter + 1; locals.counter2 < input.offeredAssetsNumber; locals.counter2++)
            {
                if (input.offeredAssets.get(locals.counter).issuer == input.offeredAssets.get(locals.counter2).issuer &&
                    input.offeredAssets.get(locals.counter).name == input.offeredAssets.get(locals.counter2).name)
                    {
                        locals.error = true;
                    }
            }
        }

        for (locals.counter = 0; locals.counter < input.requestedAssetsNumber; locals.counter++)
        {
            for (locals.counter2 = locals.counter + 1; locals.counter2 < input.requestedAssetsNumber; locals.counter2++)
            {
                if (input.requestedAssets.get(locals.counter).issuer == input.requestedAssets.get(locals.counter2).issuer &&
                    input.requestedAssets.get(locals.counter).name == input.requestedAssets.get(locals.counter2).name)
                    {
                        locals.error = true;
                    }
            }
        }
            
        if (!locals.error)
        {
            locals.offeredQuAndFee = sadd(input.offeredQU, ESCROW_BASE_FEE);

            for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
            {
                state.mut()._numberOfReservedShares_input.issuer = input.offeredAssets.get(locals.counter).issuer;
                state.mut()._numberOfReservedShares_input.assetName = input.offeredAssets.get(locals.counter).name;
                state.mut()._numberOfReservedShares_input.owner = qpi.invocator();
                CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
                locals.tempAsset.assetName = input.offeredAssets.get(locals.counter).name;
                locals.tempAsset.issuer = input.offeredAssets.get(locals.counter).issuer;
                locals.availableTokenAmount = qpi.numberOfShares(locals.tempAsset, {qpi.invocator(), SELF_INDEX, false, false}, {qpi.invocator(), SELF_INDEX, false, false}) - state.get()._numberOfReservedShares_output.amount;
                if (input.offeredAssets.get(locals.counter).amount >= MAX_AMOUNT
                        || input.offeredAssets.get(locals.counter).amount < 0
                        || locals.availableTokenAmount < 0
                        || uint64(locals.availableTokenAmount) < input.offeredAssets.get(locals.counter).amount)
                {
                    locals.error = true;
                    break;
                }

                if (input.offeredAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.offeredQuAndFee = sadd(smul(input.offeredAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.offeredQuAndFee);
                }
            }
        }

        if (!locals.error)
        {
            for (locals.counter = 0; locals.counter < input.requestedAssetsNumber; locals.counter++)
            {
                if (input.requestedAssets.get(locals.counter).amount >= MAX_AMOUNT || input.requestedAssets.get(locals.counter).amount < 0)
                {
                    locals.error = true;
                    break;
                }

                if (input.requestedAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.offeredQuAndFee = sadd(smul(input.requestedAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.offeredQuAndFee);
                }
            }
        }

        if (locals.error || uint64(qpi.invocationReward()) < locals.offeredQuAndFee)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (uint64(qpi.invocationReward()) > locals.offeredQuAndFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.offeredQuAndFee);
        }

        locals.newDeal.index = state.get()._currentDealIndex;
        locals.newDeal.acceptorId = (input.acceptorId == NULL_ID || input.acceptorId == qpi.invocator()) ? SELF : input.acceptorId;
        locals.newDeal.offeredQU = input.offeredQU;
        locals.newDeal.offeredAssets = input.offeredAssets;
        locals.newDeal.offeredAssetsNumber = input.offeredAssetsNumber;
        locals.newDeal.requestedQU = input.requestedQU;
        locals.newDeal.requestedAssets = input.requestedAssets;
        locals.newDeal.requestedAssetsNumber = input.requestedAssetsNumber;
        locals.newDeal.creationEpoch = qpi.epoch();

        if (state.get()._ownerDealIndexes.population(qpi.invocator()) == 0)
        {
            state.mut()._ownersSet.add(qpi.invocator());
        }
        state.mut()._deals.set(state.get()._currentDealIndex, locals.newDeal);
        state.mut()._ownerDealIndexes.add(qpi.invocator(), state.get()._currentDealIndex, 0);
        state.mut()._acceptorDealIndexes.add(locals.newDeal.acceptorId, state.get()._currentDealIndex, 0);

        state.mut()._dealIndexOwnerMap.set(locals.newDeal.index, qpi.invocator());
        state.mut()._earnedAmount += ESCROW_BASE_FEE;
        state.mut()._currentDealIndex++;

        for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
        {
            state.mut()._numberOfReservedShares_input.issuer = input.offeredAssets.get(locals.counter).issuer;
            state.mut()._numberOfReservedShares_input.assetName = input.offeredAssets.get(locals.counter).name;
            state.mut()._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
            if (state.get()._numberOfReservedShares_output.amount == 0)
            {
                locals.tempAssetWithAmount = input.offeredAssets.get(locals.counter);
                state.mut()._reservedAssets.add(qpi.invocator(), locals.tempAssetWithAmount, 0);
            }
            else
            {
                locals.elementIndex = state.get()._reservedAssets.headIndex(qpi.invocator());
                while (locals.elementIndex != NULL_INDEX)
                {
                    locals.tempAssetWithAmount = state.get()._reservedAssets.element(locals.elementIndex);
                    if (locals.tempAssetWithAmount.name == input.offeredAssets.get(locals.counter).name && locals.tempAssetWithAmount.issuer == input.offeredAssets.get(locals.counter).issuer)
                    {
                        locals.tempAssetWithAmount.amount += input.offeredAssets.get(locals.counter).amount;
                        state.mut()._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                        break;
                    }

                    locals.elementIndex = state.get()._reservedAssets.nextElementIndex(locals.elementIndex);
                }
            }
        } 
    }

    struct AcceptDeal_locals
    {
        Deal tempDeal;
        sint64 dealIndexInCollection;
        uint64 counter;
        sint64 transferredShares;
        sint64 transferredFeeShares;
        sint64 elementIndex;
        sint64 elementIndex2;
        uint64 requestedQuAndFee;
        AssetWithAmount tempAssetWithAmount;
        Asset tempAsset;
        EscrowAsset wrapper;
        uint64 tempAmount;
        id dealOwner;
        sint64 availableTokenAmount;
        sint64 transferResult;
        bit error;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AcceptDeal)
    {
        locals.error = false;
        locals.requestedQuAndFee = 0;

        if (!state.get()._deals.get(input.index, locals.tempDeal)
                || !state.get()._dealIndexOwnerMap.get(input.index, locals.dealOwner)
                || locals.tempDeal.index != input.index
                || (locals.tempDeal.acceptorId != SELF && locals.tempDeal.acceptorId != qpi.invocator()))
        {
            locals.error = true;
        }

        if (!locals.error)
        {
            locals.requestedQuAndFee = locals.tempDeal.requestedQU;

            for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
            {
                state.mut()._numberOfReservedShares_input.issuer = locals.tempDeal.requestedAssets.get(locals.counter).issuer;
                state.mut()._numberOfReservedShares_input.assetName = locals.tempDeal.requestedAssets.get(locals.counter).name;
                state.mut()._numberOfReservedShares_input.owner = qpi.invocator();
                CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
                locals.tempAsset.assetName = locals.tempDeal.requestedAssets.get(locals.counter).name;
                locals.tempAsset.issuer = locals.tempDeal.requestedAssets.get(locals.counter).issuer;
                locals.availableTokenAmount = qpi.numberOfShares(locals.tempAsset, {qpi.invocator(), SELF_INDEX, false, false}, {qpi.invocator(), SELF_INDEX, false, false}) - state.get()._numberOfReservedShares_output.amount;
                if (locals.availableTokenAmount < 0 || uint64(locals.availableTokenAmount) < locals.tempDeal.requestedAssets.get(locals.counter).amount)
                {
                    locals.error = true;
                    break;
                }

                if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.requestedQuAndFee = sadd(smul(locals.tempDeal.requestedAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.requestedQuAndFee);
                }
            }

            for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
            {
                if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.requestedQuAndFee = sadd(smul(locals.tempDeal.offeredAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.requestedQuAndFee);
                }
            }
        }

        if (locals.error || uint64(qpi.invocationReward()) < locals.requestedQuAndFee)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (uint64(qpi.invocationReward()) > locals.requestedQuAndFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.requestedQuAndFee);
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
        {
            if (locals.transferResult >= 0)
            {
                if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.transferredFeeShares = 0;
                    locals.transferredShares = locals.tempDeal.offeredAssets.get(locals.counter).amount;
                    locals.transferResult = qpi.transferShareOwnershipAndPossession(
                                                    locals.tempDeal.offeredAssets.get(locals.counter).name,
                                                    locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                                                    locals.dealOwner,
                                                    locals.dealOwner,
                                                    locals.tempDeal.offeredAssets.get(locals.counter).amount,
                                                    qpi.invocator());
                }
                else
                {
                    locals.transferredShares = locals.tempDeal.offeredAssets.get(locals.counter).amount - div(smul(locals.tempDeal.offeredAssets.get(locals.counter).amount, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL);
                    locals.transferResult = qpi.transferShareOwnershipAndPossession(
                                                    locals.tempDeal.offeredAssets.get(locals.counter).name,
                                                    locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                                                    locals.dealOwner,
                                                    locals.dealOwner,
                                                    locals.transferredShares,
                                                    qpi.invocator());

                    locals.transferredFeeShares = div(smul(locals.tempDeal.offeredAssets.get(locals.counter).amount, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL);
                    qpi.transferShareOwnershipAndPossession(
                            locals.tempDeal.offeredAssets.get(locals.counter).name,
                            locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                            locals.dealOwner,
                            locals.dealOwner,
                            locals.transferredFeeShares,
                            SELF);
                }
            }

            locals.tempAsset.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            locals.tempAsset.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            locals.wrapper.setFrom(locals.tempAsset);

            if (locals.tempAsset.issuer != NULL_ID && !state.get()._earnedTokens.contains(locals.wrapper))
            {
                state.mut()._earnedTokens.add(locals.wrapper);
            }

            state.mut()._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            state.mut()._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            state.mut()._numberOfReservedShares_input.owner = locals.dealOwner;
            CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
            locals.elementIndex = state.get()._reservedAssets.headIndex(locals.dealOwner);
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempAssetWithAmount = state.get()._reservedAssets.element(locals.elementIndex);
                if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                    && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                {
                    if (state.get()._numberOfReservedShares_output.amount - locals.transferredShares - locals.transferredFeeShares <= 0)
                    {
                        state.mut()._reservedAssets.remove(locals.elementIndex);
                    }
                    else
                    {
                        locals.tempAssetWithAmount.amount -= locals.transferredShares;
                        locals.tempAssetWithAmount.amount -= locals.transferredFeeShares;
                        state.mut()._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                    }
                    break;
                }
                locals.elementIndex = state.get()._reservedAssets.nextElementIndex(locals.elementIndex);
            }
        }

        if (locals.transferResult >= 0)
        {
            for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
            {
                if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.transferResult = qpi.transferShareOwnershipAndPossession(
                                                    locals.tempDeal.requestedAssets.get(locals.counter).name,
                                                    locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                                                    qpi.invocator(),
                                                    qpi.invocator(),
                                                    locals.tempDeal.requestedAssets.get(locals.counter).amount,
                                                    locals.dealOwner);
                }
                else
                {
                    locals.transferResult = qpi.transferShareOwnershipAndPossession(
                                                    locals.tempDeal.requestedAssets.get(locals.counter).name,
                                                    locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                                                    qpi.invocator(),
                                                    qpi.invocator(),
                                                    locals.tempDeal.requestedAssets.get(locals.counter).amount - div(smul(locals.tempDeal.requestedAssets.get(locals.counter).amount, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL),
                                                    locals.dealOwner);

                    qpi.transferShareOwnershipAndPossession(
                        locals.tempDeal.requestedAssets.get(locals.counter).name,
                        locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                        qpi.invocator(),
                        qpi.invocator(),
                        div(smul(locals.tempDeal.requestedAssets.get(locals.counter).amount, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL),
                        SELF);
                }

                if (locals.transferResult < 0 || locals.transferResult == INVALID_AMOUNT)
                {
                    break;
                }
                
                locals.tempAsset.issuer = locals.tempDeal.requestedAssets.get(locals.counter).issuer;
                locals.tempAsset.assetName = locals.tempDeal.requestedAssets.get(locals.counter).name;
                locals.wrapper.setFrom(locals.tempAsset);

                if (locals.tempAsset.issuer != NULL_ID && !state.get()._earnedTokens.contains(locals.wrapper))
                {
                    state.mut()._earnedTokens.add(locals.wrapper);
                }
            }
        }

        if (locals.tempDeal.offeredQU > 0)
        {
            if (locals.transferResult >= 0)
            {
                qpi.transfer(qpi.invocator(), locals.tempDeal.offeredQU - div(smul(locals.tempDeal.offeredQU, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL));
            }
            else
            {
                qpi.transfer(locals.dealOwner, locals.tempDeal.offeredQU - div(smul(locals.tempDeal.offeredQU, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL));
            }
        }
        
        if (locals.tempDeal.requestedQU > 0)
        {
            if (locals.transferResult >= 0)
            {
                qpi.transfer(locals.dealOwner, locals.tempDeal.requestedQU - div(smul(locals.tempDeal.requestedQU, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL));
            }
            else
            {
                qpi.transfer(qpi.invocator(), locals.tempDeal.requestedQU - div(smul(locals.tempDeal.requestedQU, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL));
            }
        }

        if (state.get()._ownerDealIndexes.population(locals.dealOwner) == 1)
        {
            state.mut()._ownersSet.remove(locals.dealOwner);
        }
        state.mut()._deals.removeByKey(input.index);

        state.mut()._dealIndexOwnerMap.removeByKey(input.index);

        locals.dealIndexInCollection = state.get()._acceptorDealIndexes.headIndex(qpi.invocator());
        while (locals.dealIndexInCollection != NULL_INDEX)
        {
            if (state.get()._acceptorDealIndexes.element(locals.dealIndexInCollection) == input.index)
            {
                state.mut()._acceptorDealIndexes.remove(locals.dealIndexInCollection);
                break;
            }
            locals.dealIndexInCollection = state.get()._acceptorDealIndexes.nextElementIndex(locals.dealIndexInCollection);
        }

        locals.dealIndexInCollection = state.get()._ownerDealIndexes.headIndex(locals.dealOwner);
        while (locals.dealIndexInCollection != NULL_INDEX)
        {
            if (state.get()._ownerDealIndexes.element(locals.dealIndexInCollection) == input.index)
            {
                state.mut()._ownerDealIndexes.remove(locals.dealIndexInCollection);
                break;
            }
            locals.dealIndexInCollection = state.get()._ownerDealIndexes.nextElementIndex(locals.dealIndexInCollection);
        }

        state.mut()._earnedAmount = sadd(div(smul(locals.tempDeal.offeredQU, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL), state.get()._earnedAmount);
        state.mut()._earnedAmount = sadd(div(smul(locals.tempDeal.requestedQU, ESCROW_ADDITIONAL_FEE_PERCENT), 10000ULL), state.get()._earnedAmount);
        state.mut()._earnedAmount = sadd((locals.requestedQuAndFee - locals.tempDeal.requestedQU) * 2, state.get()._earnedAmount);
    }

    struct MakeDealPublic_locals
    {
        Deal tempDeal;
        sint64 dealIndexInCollection;
        id dealOwner;
        id oldAcceptor;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(MakeDealPublic)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (!state.get()._deals.get(input.index, locals.tempDeal)
                || !state.get()._dealIndexOwnerMap.get(input.index, locals.dealOwner)
                || locals.dealOwner != qpi.invocator()
                || locals.tempDeal.acceptorId == SELF)
        {
            return;
        }

        locals.oldAcceptor = locals.tempDeal.acceptorId;
        locals.tempDeal.acceptorId = SELF;
        state.mut()._deals.replace(input.index, locals.tempDeal);

        locals.dealIndexInCollection = state.get()._acceptorDealIndexes.headIndex(locals.oldAcceptor);
        while (locals.dealIndexInCollection != NULL_INDEX)
        {
            if (state.get()._acceptorDealIndexes.element(locals.dealIndexInCollection) == input.index)
            {
                state.mut()._acceptorDealIndexes.remove(locals.dealIndexInCollection);
                state.mut()._acceptorDealIndexes.add(SELF, input.index, 0);
                break;
            }
            locals.dealIndexInCollection = state.get()._acceptorDealIndexes.nextElementIndex(locals.dealIndexInCollection);
        }
    }

    struct CancelDeal_locals
    {
        Deal tempDeal;
        uint64 counter;
        sint64 dealIndexInCollection;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
        uint64 quForReturn;
        id dealOwner;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CancelDeal)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (!state.get()._deals.get(input.index, locals.tempDeal)
                || !state.get()._dealIndexOwnerMap.get(input.index, locals.dealOwner)
                || locals.dealOwner != qpi.invocator())
        {
            return;
        }

        locals.quForReturn = locals.tempDeal.offeredQU;

        for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
        {
            state.mut()._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            state.mut()._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            state.mut()._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
            locals.elementIndex = state.get()._reservedAssets.headIndex(qpi.invocator());
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempAssetWithAmount = state.get()._reservedAssets.element(locals.elementIndex);
                if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                    && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                {
                    if (state.get()._numberOfReservedShares_output.amount - locals.tempDeal.offeredAssets.get(locals.counter).amount <= 0)
                    {
                        state.mut()._reservedAssets.remove(locals.elementIndex);
                        break;
                    }
                    else
                    {
                        locals.tempAssetWithAmount.amount -= locals.tempDeal.offeredAssets.get(locals.counter).amount;
                        state.mut()._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                    }
                }
                locals.elementIndex = state.get()._reservedAssets.nextElementIndex(locals.elementIndex);
            }

            if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.quForReturn = sadd(smul(locals.tempDeal.offeredAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.quForReturn);
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
        {
            if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.quForReturn = sadd(smul(locals.tempDeal.requestedAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.quForReturn);
            }
        }

        qpi.transfer(qpi.invocator(), locals.quForReturn);

        state.mut()._deals.removeByKey(input.index);
        state.mut()._dealIndexOwnerMap.removeByKey(input.index);
        if (state.get()._ownerDealIndexes.population(locals.dealOwner) == 1)
        {
            state.mut()._ownersSet.remove(locals.dealOwner);
        }

        locals.dealIndexInCollection = state.get()._ownerDealIndexes.headIndex(locals.dealOwner);
        while (locals.dealIndexInCollection != NULL_INDEX)
        {
            if (state.get()._ownerDealIndexes.element(locals.dealIndexInCollection) == input.index)
            {
                state.mut()._ownerDealIndexes.remove(locals.dealIndexInCollection);
                break;
            }
            locals.dealIndexInCollection = state.get()._ownerDealIndexes.nextElementIndex(locals.dealIndexInCollection);
        }

        locals.dealIndexInCollection = state.get()._acceptorDealIndexes.headIndex(locals.tempDeal.acceptorId);
        while (locals.dealIndexInCollection != NULL_INDEX)
        {
            if (state.get()._acceptorDealIndexes.element(locals.dealIndexInCollection) == input.index)
            {
                state.mut()._acceptorDealIndexes.remove(locals.dealIndexInCollection);
                break;
            }
            locals.dealIndexInCollection = state.get()._acceptorDealIndexes.nextElementIndex(locals.dealIndexInCollection);
        }
    }

    struct TransferShareManagementRights_locals
    {
        sint64 result;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
    {
        if (input.amount >= MAX_AMOUNT || input.amount <= 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        state.mut()._numberOfReservedShares_input.issuer = input.asset.issuer;
        state.mut()._numberOfReservedShares_input.assetName = input.asset.assetName;
        state.mut()._numberOfReservedShares_input.owner = qpi.invocator();
        CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
        if (qpi.numberOfShares(input.asset, {qpi.invocator(), SELF_INDEX, false, false}, {qpi.invocator(), SELF_INDEX, false, false}) - state.get()._numberOfReservedShares_output.amount < input.amount)
        {
            output.transferredShares = 0;
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        else
        {
            locals.result = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.amount, input.newContractIndex, input.newContractIndex, qpi.invocationReward());
            if (locals.result < 0 || locals.result == INVALID_AMOUNT)
            {
                output.transferredShares = 0;
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            else
            {
                output.transferredShares = input.amount;
                if (qpi.invocationReward() > locals.result)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.result);
				}
            }
        }
    }

    struct GetDeals_locals
    {
        sint64 elementIndex;
        sint64 elementIndex2;
        sint64 elementIndex3;
        sint64 elementIndex4;
        Deal tempDeal;
        id dealOwner;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetDeals)
    {
        output.ownedDealsAmount = state.get()._ownerDealIndexes.population(input.owner);

        locals.elementIndex = state.get()._ownerDealIndexes.headIndex(input.owner);
        locals.elementIndex2 = 0;
        while (locals.elementIndex != NULL_INDEX
            && locals.elementIndex2 < ESCROW_MAX_DEALS_PER_USER)
        {
            if (state.get()._deals.get(state.get()._ownerDealIndexes.element(locals.elementIndex), locals.tempDeal))
            {
                output.ownedDeals.set(locals.elementIndex2, locals.tempDeal);
                locals.elementIndex = state.get()._ownerDealIndexes.nextElementIndex(locals.elementIndex);
                locals.elementIndex2++;
            }
        }

        locals.elementIndex3 = 0;
        locals.elementIndex4 = 0;

        locals.elementIndex = state.get()._acceptorDealIndexes.headIndex(input.owner);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.proposedDealsOffset > 0)
            {
                input.proposedDealsOffset--;
            }
            else if (state.get()._deals.get(state.get()._acceptorDealIndexes.element(locals.elementIndex), locals.tempDeal)
                        && state.get()._dealIndexOwnerMap.get(state.get()._acceptorDealIndexes.element(locals.elementIndex), locals.dealOwner)
                        && locals.elementIndex3 < 32)
            {
                locals.tempDeal.acceptorId = locals.dealOwner;
                output.proposedDeals.set(locals.elementIndex3, locals.tempDeal);
                locals.elementIndex3++;
            }

            locals.elementIndex = state.get()._acceptorDealIndexes.nextElementIndex(locals.elementIndex);
        }

        locals.elementIndex = state.get()._acceptorDealIndexes.headIndex(SELF);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.publicDealsOffset > 0)
            {
                input.publicDealsOffset--;
            }
            else if (state.get()._deals.get(state.get()._acceptorDealIndexes.element(locals.elementIndex), locals.tempDeal)
                        && state.get()._dealIndexOwnerMap.get(state.get()._acceptorDealIndexes.element(locals.elementIndex), locals.dealOwner)
                        && locals.elementIndex4 < 64
                        && locals.dealOwner != input.owner)
            {
                locals.tempDeal.acceptorId = locals.dealOwner;
                output.publicDeals.set(locals.elementIndex4, locals.tempDeal);
                locals.elementIndex4++;
            }
            
            locals.elementIndex = state.get()._acceptorDealIndexes.nextElementIndex(locals.elementIndex);
        }

        output.proposedDealsAmount = locals.elementIndex3;
        output.publicDealsAmount = locals.elementIndex4;
    }

    struct GetFreeAssetAmount_locals
    {
        _NumberOfReservedShares_input reservedInput;
        _NumberOfReservedShares_output reservedOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetFreeAssetAmount)
    {
        locals.reservedInput.issuer = input.asset.issuer;
        locals.reservedInput.assetName = input.asset.assetName;
        locals.reservedInput.owner = input.owner;
        CALL(_NumberOfReservedShares, locals.reservedInput, locals.reservedOutput);
        output.freeAmount = qpi.numberOfShares(input.asset, {input.owner, SELF_INDEX, false, false}, {input.owner, SELF_INDEX, false, false}) - locals.reservedOutput.amount;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(CreateDeal, 1);
        REGISTER_USER_PROCEDURE(AcceptDeal, 2);
        REGISTER_USER_PROCEDURE(MakeDealPublic, 3);
        REGISTER_USER_PROCEDURE(CancelDeal, 4);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 5);

        REGISTER_USER_FUNCTION(GetDeals, 1);
        REGISTER_USER_FUNCTION(GetFreeAssetAmount, 2);
    }

    INITIALIZE()
    {
        state.mut()._devAddress = ID(_E, _S, _C, _R, _O, _W, _F, _P, _Z, _M, _F, _P, _D, _F, _T, _M, _G, _K, _N, _N, _Z, _L, _N, _B, _U, _J, _L, _C, _W, _G, _B, _U, _L, _K, _S, _N, _W, _L, _S, _D, _R, _G, _T, _Y, _T, _B, _E, _M, _F, _O, _X, _B, _C, _A, _E, _H);
        state.mut()._currentDealIndex = 1;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        Deal tempDeal;
        id dealOwner;
        uint64 counter;
        sint64 ownerIndex;
        sint64 dealIndex;
        sint64 reservedAssetIndex;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
        uint64 quForReturn;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        locals.ownerIndex = state.get()._ownersSet.nextElementIndex(NULL_INDEX);
        while (locals.ownerIndex != NULL_INDEX)
        {
            locals.dealOwner = state.get()._ownersSet.key(locals.ownerIndex);
            locals.dealIndex = state.get()._ownerDealIndexes.headIndex(locals.dealOwner);
            while (locals.dealIndex != NULL_INDEX)
            {
                if (state.get()._deals.get(state.get()._ownerDealIndexes.element(locals.dealIndex), locals.tempDeal)
                        && locals.tempDeal.creationEpoch + ESCROW_DEAL_EXISTENCE_EPOCH_COUNT > qpi.epoch())
                {
                    locals.dealIndex = state.get()._ownerDealIndexes.nextElementIndex(locals.dealIndex);
                    continue;
                }

                locals.quForReturn = locals.tempDeal.offeredQU;
                for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
                {
                    state.mut()._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
                    state.mut()._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
                    state.mut()._numberOfReservedShares_input.owner = locals.dealOwner;
                    CALL(_NumberOfReservedShares, state.mut()._numberOfReservedShares_input, state.mut()._numberOfReservedShares_output);
                    locals.reservedAssetIndex = state.get()._reservedAssets.headIndex(locals.dealOwner);
                    while (locals.reservedAssetIndex != NULL_INDEX)
                    {
                        locals.tempAssetWithAmount = state.get()._reservedAssets.element(locals.reservedAssetIndex);
                        if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                            && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                        {
                            if (state.get()._numberOfReservedShares_output.amount - locals.tempDeal.offeredAssets.get(locals.counter).amount <= 0)
                            {
                                state.mut()._reservedAssets.remove(locals.reservedAssetIndex);
                                break;
                            }
                            else
                            {
                                locals.tempAssetWithAmount.amount -= locals.tempDeal.offeredAssets.get(locals.counter).amount;
                                state.mut()._reservedAssets.replace(locals.reservedAssetIndex, locals.tempAssetWithAmount);
                            }
                        }
                        locals.reservedAssetIndex = state.get()._reservedAssets.nextElementIndex(locals.reservedAssetIndex);
                    }

                    if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
                    {
                        locals.quForReturn = sadd(smul(locals.tempDeal.offeredAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.quForReturn);
                    }
                }

                for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
                {
                    if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
                    {
                        locals.quForReturn = sadd(smul(locals.tempDeal.requestedAssets.get(locals.counter).amount, ESCROW_FEE_PER_SHARE), locals.quForReturn);
                    }
                }

                if (locals.quForReturn > 0)
                {
                    qpi.transfer(locals.dealOwner, locals.quForReturn);
                }

                state.mut()._dealIndexOwnerMap.removeByKey(locals.tempDeal.index);

                if (state.get()._ownerDealIndexes.population(locals.dealOwner) == 1)
                {
                    state.mut()._ownersSet.remove(locals.dealOwner);
                }

                locals.elementIndex = state.get()._acceptorDealIndexes.headIndex(locals.tempDeal.acceptorId);
                while (locals.elementIndex != NULL_INDEX)
                {
                    if (state.get()._acceptorDealIndexes.element(locals.elementIndex) == locals.tempDeal.index)
                    {
                        state.mut()._acceptorDealIndexes.remove(locals.elementIndex);
                        break;
                    }
                    locals.elementIndex = state.get()._acceptorDealIndexes.nextElementIndex(locals.elementIndex);
                }

                state.mut()._deals.removeByKey(locals.tempDeal.index);

                locals.dealIndex = state.mut()._ownerDealIndexes.remove(locals.dealIndex);
            }
            locals.ownerIndex = state.get()._ownersSet.nextElementIndex(locals.ownerIndex);
        }
    }

    struct END_EPOCH_locals
    {
        uint64 amountToDistribute;
        uint64 amountToBurn;
        uint64 amountToDevs;
        AssetOwnershipIterator assetIt;
        Asset selfShare;
        sint64 elementIndex;
        EscrowAsset tempAsset;
        uint64 tempEarnedAmount;
        uint64 tokenAmountToDistribute;
    };

    END_EPOCH_WITH_LOCALS()
    {
        locals.amountToDistribute = div((state.get()._earnedAmount - state.get()._distributedAmount) * ESCROW_SHAREHOLDERS_QU_DISTRIBUTION_PERCENT, 10000ULL);
        locals.amountToBurn = div((state.get()._earnedAmount - state.get()._distributedAmount) * ESCROW_BURN_QU_PERCENT, 10000ULL);
        locals.amountToDevs = state.get()._earnedAmount - state.get()._distributedAmount - locals.amountToDistribute - locals.amountToBurn;

        if ((div(locals.amountToDistribute, 676ULL) > 0) && (state.get()._earnedAmount > state.get()._distributedAmount))
        {
            if (qpi.distributeDividends(div(locals.amountToDistribute, 676ULL)))
            {
                qpi.burn(locals.amountToBurn);
                qpi.transfer(state.get()._devAddress, locals.amountToDevs);
                state.mut()._distributedAmount += div(locals.amountToDistribute, 676ULL) * NUMBER_OF_COMPUTORS;
                state.mut()._distributedAmount += locals.amountToBurn;
                state.mut()._distributedAmount += locals.amountToDevs;
            }
        }

        locals.selfShare.issuer = NULL_ID;
        locals.selfShare.assetName = 95998194176837ULL;

        locals.elementIndex = state.get()._earnedTokens.nextElementIndex(NULL_INDEX);
        while (locals.elementIndex != NULL_INDEX)
        {
            locals.tempAsset = state.get()._earnedTokens.key(locals.elementIndex);

            locals.tempEarnedAmount = qpi.numberOfShares({ locals.tempAsset.issuer, locals.tempAsset.assetName }, {SELF, SELF_INDEX, false, false}, {SELF, SELF_INDEX, false, false});
            locals.tokenAmountToDistribute = div(smul(locals.tempEarnedAmount, ESCROW_SHAREHOLDERS_TOKEN_DISTRIBUTION_PERCENT), 10000ULL);

            if (locals.tokenAmountToDistribute < 676ULL)
            {
                locals.elementIndex = state.get()._earnedTokens.nextElementIndex(locals.elementIndex);
                continue;
            }

            locals.assetIt.begin(locals.selfShare);
            while (!locals.assetIt.reachedEnd())
            {
                qpi.transferShareOwnershipAndPossession(
                        locals.tempAsset.assetName,
                        locals.tempAsset.issuer,
                        SELF,
                        SELF,
                        div(locals.tokenAmountToDistribute, 676ULL) * locals.assetIt.numberOfOwnedShares(),
                        locals.assetIt.owner());
                locals.assetIt.next();
            }

            qpi.transferShareOwnershipAndPossession(
                        locals.tempAsset.assetName,
                        locals.tempAsset.issuer,
                        SELF,
                        SELF,
                        qpi.numberOfShares({ locals.tempAsset.issuer, locals.tempAsset.assetName }, {SELF, SELF_INDEX, false, false}, {SELF, SELF_INDEX, false, false}),
                        state.get()._devAddress);

            locals.elementIndex = state.get()._earnedTokens.nextElementIndex(locals.elementIndex);
            state.mut()._earnedTokens.remove(locals.tempAsset);
        }

        state.mut()._deals.cleanupIfNeeded();
        state.mut()._ownerDealIndexes.cleanupIfNeeded();
        state.mut()._acceptorDealIndexes.cleanupIfNeeded();
        state.mut()._dealIndexOwnerMap.cleanupIfNeeded();
        state.mut()._ownersSet.cleanupIfNeeded();
        state.mut()._earnedTokens.cleanupIfNeeded();
        state.mut()._reservedAssets.cleanupIfNeeded();
    }
};
