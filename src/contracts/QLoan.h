using namespace QPI;

constexpr uint64 QLOAN_PLACE_LOAN_REQ_FEE = 100000;

constexpr uint64 QLOAN_ACCEPTANCE_FEE_PERCENT = 15;
constexpr uint64 QLOAN_BURN_PERCENT = 300; // 3%
constexpr uint64 QLOAN_QVAULT_PERCENT = 9700; // 97%

constexpr uint64 QLOAN_MIN_LOAN_PERIOD_IN_EPOCHS = 1;
constexpr uint64 QLOAN_MAX_LOAN_PERIOD_IN_EPOCHS = 52;
constexpr uint64 QLOAN_MAX_INTEREST_RATE = 100;
constexpr uint64 QLOAN_MAX_ASSETS_NUM = 2;
constexpr uint64 QLOAN_MAX_OUTPUT_NUM = 128;

constexpr uint64 QLOAN_MAX_LOAN_REQS_NUM = 1024;

struct QLOAN2
{
};

struct QLOAN : public ContractBase
{
    enum LoanReqState : uint8
    {
        IDLE = 1,
        ACTIVE = 2,
        PAYED = 3,
        EXPIRED = 4,
    };

    struct LoanOutputInfo
    {
        id borrower;
        id creditor;
        id acceptedBy;
        id privateId;

        uint64 reqId;

        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;

        uint64 priceAmount;
        uint64 interestRate;
        uint64 fullInterest;
        uint64 debtAmount;

        uint64 returnPeriodInEpochs;
        uint64 epochsLeft;
        uint64 creationEpoch;

        uint8 state;

        bit assetsToCreditor;
    };

    struct LoanReqInfo
    {
        id borrower;
        id creditor;
        // Using these field to know which request user is accepted and which is created by him
        id acceptedBy;
        id privateId;

        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;

        uint64 priceAmount;
        uint64 interestRate;
        uint64 fullInterest;
        uint64 debtAmount;

        uint64 returnPeriodInEpochs;
        uint64 epochsLeft;
        uint64 creationEpoch;

        uint8 state;

        bit assetsToCreditor;
    };

    struct StateData
    {
        HashMap<uint64, LoanReqInfo, QLOAN_MAX_LOAN_REQS_NUM> _loanReqs;
        uint64 _totalReqs;
        uint64 _currentLoanIndex;

        uint64 _earnedAmount;
        uint64 _distributedAmount;

        // These two mostly for debug purposes
        uint64 _burnedAmount;
        uint64 _toQvaultAmount;
    };

    struct _TransferAssetsFromTo_input
    {
        LoanReqInfo loanReq;
        id from;
        id to;
    };

    struct _TransferAssetsFromTo_output
    {
        bit allGood;
    };

    struct _TransferAssetsFromTo_locals
    {
        Asset userReqAsset;
        uint8 userReqAssetIdx;
        sint64 result;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_TransferAssetsFromTo)
    {
        locals.userReqAssetIdx = 0;
        output.allGood = true;
        
        while (locals.userReqAssetIdx < input.loanReq.assetsNum)
        {
            locals.userReqAsset = input.loanReq.assets.get(locals.userReqAssetIdx);
            locals.result = qpi.transferShareOwnershipAndPossession(locals.userReqAsset.assetName, locals.userReqAsset.issuer,
                                                                        input.from, input.from,
                                                                        input.loanReq.assetAmount.get(locals.userReqAssetIdx),
                                                                        input.to);
            if (locals.result < 0 || locals.result == INVALID_AMOUNT)
            {
                output.allGood = false;
                if (locals.userReqAssetIdx != 0)
                {
                    locals.userReqAssetIdx--;
                    while (true)
                    {
                        locals.userReqAsset = input.loanReq.assets.get(locals.userReqAssetIdx);
                        qpi.transferShareOwnershipAndPossession(locals.userReqAsset.assetName, locals.userReqAsset.issuer,
                                                                        input.to, input.to,
                                                                        input.loanReq.assetAmount.get(locals.userReqAssetIdx),
                                                                        input.from);
                        if (locals.userReqAssetIdx == 0)
                        {
                            break;
                        }
                        locals.userReqAssetIdx--;
                    }
                }
                return;
            }
            locals.userReqAssetIdx++;
        }
    }

    struct _CheckAssetsPresence_input
    {
        id owner;
        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;
        uint64 excludeReqId;
    };

    struct _CheckAssetsPresence_output
    {
        bit allGood;
    };

    struct _CheckAssetsPresence_locals
    {
        uint64 loanReqsIdx;
        LoanReqInfo tmpLoanReq;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> userAssetsAmountInvolved;

        uint64 inputReqAssetIdx;
        uint64 tmpReqAssetIdx;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(_CheckAssetsPresence)
    {
        output.allGood = true;

        // Collect all user's tokens involved in others deals
        locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);
        while (locals.loanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReq = state.get()._loanReqs.value(locals.loanReqsIdx);
            // If user is a borrower - we need to check if assets transfered to the creditor in case of Active request,
            // otherwise request might be in the IDLE state and we still should count these tokens
            // If user is a creditor and assets should be transfered to creditor in active loan request then we should count these tokens too
            if (state.get()._loanReqs.key(locals.loanReqsIdx) != input.excludeReqId
                && ((locals.tmpLoanReq.borrower == input.owner && ((locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == false) || locals.tmpLoanReq.state == LoanReqState::IDLE))
                || (locals.tmpLoanReq.creditor == input.owner && (locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == true))))
            {
                // Iterate over assets in the user loan request
                locals.inputReqAssetIdx = 0;
                while (locals.inputReqAssetIdx < input.assetsNum)
                {
                    locals.tmpReqAssetIdx = 0;
                    while (locals.tmpReqAssetIdx < locals.tmpLoanReq.assetsNum)
                    {
                        if (input.assets.get(locals.inputReqAssetIdx).assetName == locals.tmpLoanReq.assets.get(locals.tmpReqAssetIdx).assetName
                            && input.assets.get(locals.inputReqAssetIdx).issuer == locals.tmpLoanReq.assets.get(locals.tmpReqAssetIdx).issuer)
                        {
                            locals.userAssetsAmountInvolved.set(locals.inputReqAssetIdx,
                                locals.userAssetsAmountInvolved.get(locals.inputReqAssetIdx) + locals.tmpLoanReq.assetAmount.get(locals.tmpReqAssetIdx));
                        }
                        locals.tmpReqAssetIdx++;
                    }
                    locals.inputReqAssetIdx++;
                }
            }
            locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.loanReqsIdx);
        }
        // Check that user have enough of tokens considering his tokens from others deals
        locals.inputReqAssetIdx = 0;
        while (locals.inputReqAssetIdx < input.assetsNum)
        {
            if (qpi.numberOfPossessedShares(input.assets.get(locals.inputReqAssetIdx).assetName,
                input.assets.get(locals.inputReqAssetIdx).issuer,
                input.owner, input.owner,
                SELF_INDEX, SELF_INDEX) - locals.userAssetsAmountInvolved.get(locals.inputReqAssetIdx) < input.assetAmount.get(locals.inputReqAssetIdx))
            {
                output.allGood = false;
                return;
            }
            locals.inputReqAssetIdx++;
        }
    }

    struct _FillLoanReqForOutput_input
    {
        uint64 loanReqId;
        LoanReqInfo loanReqInfo;
    };

    struct _FillLoanReqForOutput_output
    {
        LoanOutputInfo loanOutputInfo;
    };

    PRIVATE_FUNCTION(_FillLoanReqForOutput)
    {
        output.loanOutputInfo.borrower = input.loanReqInfo.borrower;
        output.loanOutputInfo.creditor = input.loanReqInfo.creditor;
        output.loanOutputInfo.acceptedBy = input.loanReqInfo.acceptedBy;
        output.loanOutputInfo.privateId = input.loanReqInfo.privateId;
        output.loanOutputInfo.reqId = input.loanReqId;
        output.loanOutputInfo.assets = input.loanReqInfo.assets;
        output.loanOutputInfo.assetAmount = input.loanReqInfo.assetAmount;
        output.loanOutputInfo.assetsNum = input.loanReqInfo.assetsNum;
        output.loanOutputInfo.priceAmount = input.loanReqInfo.priceAmount;
        output.loanOutputInfo.interestRate = input.loanReqInfo.interestRate;
        output.loanOutputInfo.fullInterest = input.loanReqInfo.fullInterest;
        output.loanOutputInfo.debtAmount = input.loanReqInfo.debtAmount;
        output.loanOutputInfo.returnPeriodInEpochs = input.loanReqInfo.returnPeriodInEpochs;
        output.loanOutputInfo.epochsLeft = input.loanReqInfo.epochsLeft;
        output.loanOutputInfo.creationEpoch= input.loanReqInfo.creationEpoch;
        output.loanOutputInfo.assetsToCreditor = input.loanReqInfo.assetsToCreditor;
        output.loanOutputInfo.state = input.loanReqInfo.state;
    }

public:
    struct PlaceLoanReq_input
    {
        // It's for whom these deal
        id privateId;

        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;

        uint64 price;
        uint64 interestRate;
        uint64 returnPeriodInEpochs;

        bit isLoanReq;
        bit assetsToCreditor;
    };

    struct PlaceLoanReq_output
    {
    };

    struct PlaceLoanReq_locals
    {
        LoanReqInfo loanReqInfo;
        uint8 userReqAssetIdx;
        uint8 userReqAssetIdx2;

        _CheckAssetsPresence_input checkAssetsPresenceInput;
        _CheckAssetsPresence_output checkAssetsPresenceOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(PlaceLoanReq)
    {
        // Check if we have space for new request
        if (state.get()._totalReqs >= QLOAN_MAX_LOAN_REQS_NUM)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Check all inputs are valid
        if (input.returnPeriodInEpochs > QLOAN_MAX_LOAN_PERIOD_IN_EPOCHS
            || input.returnPeriodInEpochs < QLOAN_MIN_LOAN_PERIOD_IN_EPOCHS
            || input.price == 0
            || input.price >= MAX_AMOUNT - QLOAN_PLACE_LOAN_REQ_FEE
            || input.interestRate == 0
            || input.interestRate > QLOAN_MAX_INTEREST_RATE
            || input.assetsNum == 0
            || input.assetsNum > QLOAN_MAX_ASSETS_NUM)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        for (locals.userReqAssetIdx = 0; locals.userReqAssetIdx < input.assetsNum; locals.userReqAssetIdx++)
        {
            if (input.assetAmount.get(locals.userReqAssetIdx) <= 0 || input.assetAmount.get(locals.userReqAssetIdx) >= MAX_AMOUNT)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }

            for (locals.userReqAssetIdx2 = locals.userReqAssetIdx + 1; locals.userReqAssetIdx2 < input.assetsNum; locals.userReqAssetIdx2++)
            {
                if (input.assets.get(locals.userReqAssetIdx).assetName == input.assets.get(locals.userReqAssetIdx2).assetName
                    && input.assets.get(locals.userReqAssetIdx).issuer == input.assets.get(locals.userReqAssetIdx2).issuer)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    return;
                }
            }
        }

        if (input.isLoanReq)
        {
            // Check he have enough qus for FEE
            if (qpi.invocationReward() < QLOAN_PLACE_LOAN_REQ_FEE)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
            else if (qpi.invocationReward() > QLOAN_PLACE_LOAN_REQ_FEE)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - QLOAN_PLACE_LOAN_REQ_FEE);
            }

            locals.checkAssetsPresenceInput.owner = qpi.invocator();
            locals.checkAssetsPresenceInput.assets = input.assets;
            locals.checkAssetsPresenceInput.assetAmount = input.assetAmount;
            locals.checkAssetsPresenceInput.assetsNum = input.assetsNum;
            locals.checkAssetsPresenceInput.excludeReqId = state.get()._currentLoanIndex;
            CALL(_CheckAssetsPresence, locals.checkAssetsPresenceInput, locals.checkAssetsPresenceOutput);

            if (locals.checkAssetsPresenceOutput.allGood == false)
            {
                qpi.transfer(qpi.invocator(), QLOAN_PLACE_LOAN_REQ_FEE);
                return;
            }
        }
        else    // User want to create creditor request, he wants assets, we need to check he have enough QUs
        {
            if (qpi.invocationReward() < sint64(input.price + QLOAN_PLACE_LOAN_REQ_FEE))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
            else if (qpi.invocationReward() > sint64(input.price + QLOAN_PLACE_LOAN_REQ_FEE))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.price - QLOAN_PLACE_LOAN_REQ_FEE);
            }
        }
        state.mut()._earnedAmount += QLOAN_PLACE_LOAN_REQ_FEE;

        locals.loanReqInfo.borrower = input.isLoanReq ? qpi.invocator() : NULL_ID;
        locals.loanReqInfo.creditor = input.isLoanReq ? NULL_ID : qpi.invocator();
        locals.loanReqInfo.acceptedBy = NULL_ID;
        locals.loanReqInfo.assets = input.assets;
        locals.loanReqInfo.assetAmount = input.assetAmount;
        locals.loanReqInfo.assetsNum = input.assetsNum;
        locals.loanReqInfo.priceAmount = input.price;
        locals.loanReqInfo.interestRate = input.interestRate;
        locals.loanReqInfo.fullInterest = div(smul(input.price, input.interestRate), 100ULL);
        // until 1/3 of the loan period has passed, the debt will be input.price + 1/3 of the total interest
        locals.loanReqInfo.debtAmount = input.price + div(smul(input.price, input.interestRate), 300ULL);
        locals.loanReqInfo.returnPeriodInEpochs = input.returnPeriodInEpochs;
        locals.loanReqInfo.epochsLeft = input.returnPeriodInEpochs;
        locals.loanReqInfo.creationEpoch = qpi.epoch();
        locals.loanReqInfo.privateId = input.privateId;
        locals.loanReqInfo.assetsToCreditor = input.assetsToCreditor;
        locals.loanReqInfo.state = LoanReqState::IDLE;

        state.mut()._loanReqs.set(state.get()._currentLoanIndex, locals.loanReqInfo);
        state.mut()._totalReqs++;
        state.mut()._currentLoanIndex++;
    }

    struct AcceptLoanReq_input
    {
        uint64 reqId;
    };

    struct AcceptLoanReq_output
    {
    };

    struct AcceptLoanReq_locals
    {
        LoanReqInfo tmpLoanReq;
        sint64 requestedFee;

        _CheckAssetsPresence_input checkAssetsPresenceInput;
        _CheckAssetsPresence_output checkAssetsPresenceOutput;

        _TransferAssetsFromTo_input transferAssetsFromToInput;
        _TransferAssetsFromTo_output transferAssetsFromToOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AcceptLoanReq)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.tmpLoanReq) == false)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // We need to figure out which request we want to accept(loan or credit)
        // We could do that by checking ID's of the borrower and creditor
        // If borrower is not empty -> loan(need to check that acceptor have enought QU's money)
        // Else creditor is not empty -> credit(need to check that acceptor have enough assets listed in request)

        // Check that request was not created by the qpi.invocator()
        if (locals.tmpLoanReq.borrower == qpi.invocator() || locals.tmpLoanReq.creditor == qpi.invocator())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        // Check that request in IDLE state
        if (locals.tmpLoanReq.state != LoanReqState::IDLE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Check that request is not private or IF private then check that user can accept it
        if (locals.tmpLoanReq.privateId != NULL_ID && locals.tmpLoanReq.privateId != qpi.invocator())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.requestedFee = div(smul(div(smul(locals.tmpLoanReq.priceAmount, locals.tmpLoanReq.interestRate), 100ULL), QLOAN_ACCEPTANCE_FEE_PERCENT), 100ULL);

        // If request was created by the borrower(he wants money for the assets)
        if (locals.tmpLoanReq.borrower != NULL_ID)
        {
            locals.checkAssetsPresenceInput.owner = locals.tmpLoanReq.borrower;
            locals.checkAssetsPresenceInput.assets = locals.tmpLoanReq.assets;
            locals.checkAssetsPresenceInput.assetAmount = locals.tmpLoanReq.assetAmount;
            locals.checkAssetsPresenceInput.assetsNum = locals.tmpLoanReq.assetsNum;
            locals.checkAssetsPresenceInput.excludeReqId = input.reqId;
            CALL(_CheckAssetsPresence, locals.checkAssetsPresenceInput, locals.checkAssetsPresenceOutput);

            // Check that borrower has enough assets and creditor send us right amount of money for contract
            if (locals.checkAssetsPresenceOutput.allGood == false || qpi.invocationReward() < sint64(locals.tmpLoanReq.priceAmount))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }

            // Transfer all assets from request to creditor if request was created like that
            if (locals.tmpLoanReq.assetsToCreditor)
            {
                locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReq;
                locals.transferAssetsFromToInput.from = locals.tmpLoanReq.borrower;
                locals.transferAssetsFromToInput.to = qpi.invocator();
                CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);

                if (!locals.transferAssetsFromToOutput.allGood)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    return;
                }
            }
            
            locals.tmpLoanReq.creditor = qpi.invocator();
            locals.tmpLoanReq.acceptedBy = qpi.invocator();
            locals.tmpLoanReq.state = LoanReqState::ACTIVE;

            state.mut()._loanReqs.replace(input.reqId, locals.tmpLoanReq);

            state.mut()._earnedAmount += locals.requestedFee;
            // Send coins to the borrower
            qpi.transfer(locals.tmpLoanReq.borrower,
                locals.tmpLoanReq.priceAmount - locals.requestedFee);
            // Send left coins to the creditor
            if (qpi.invocationReward() > sint64(locals.tmpLoanReq.priceAmount))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.tmpLoanReq.priceAmount);
            }
        }
        else    // If request was created by the creditor(he wants asssets for the money)
        {
            locals.checkAssetsPresenceInput.owner = qpi.invocator();
            locals.checkAssetsPresenceInput.assets = locals.tmpLoanReq.assets;
            locals.checkAssetsPresenceInput.assetAmount = locals.tmpLoanReq.assetAmount;
            locals.checkAssetsPresenceInput.assetsNum = locals.tmpLoanReq.assetsNum;
            locals.checkAssetsPresenceInput.excludeReqId = input.reqId;
            CALL(_CheckAssetsPresence, locals.checkAssetsPresenceInput, locals.checkAssetsPresenceOutput);
            if (locals.checkAssetsPresenceOutput.allGood == false)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }

            // Transfer all assets from request to creditor if request was created like that
            if (locals.tmpLoanReq.assetsToCreditor)
            {
                locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReq;
                locals.transferAssetsFromToInput.from = qpi.invocator();
                locals.transferAssetsFromToInput.to = locals.tmpLoanReq.creditor;
                CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);

                if (!locals.transferAssetsFromToOutput.allGood)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    return;
                }
            }

            locals.tmpLoanReq.borrower = qpi.invocator();
            locals.tmpLoanReq.acceptedBy = qpi.invocator();
            locals.tmpLoanReq.state = LoanReqState::ACTIVE;
            state.mut()._loanReqs.replace(input.reqId, locals.tmpLoanReq);

            state.mut()._earnedAmount += locals.requestedFee;
            qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.tmpLoanReq.priceAmount - locals.requestedFee);
        }
    }

    struct RemoveLoanReq_input
    {
        uint64 reqId;
    };

    struct RemoveLoanReq_output
    {
    };

    struct RemoveLoanReq_locals
    {
        LoanReqInfo tmpLoanReq;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveLoanReq)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.tmpLoanReq) == false)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if ((locals.tmpLoanReq.borrower != qpi.invocator() && locals.tmpLoanReq.creditor != qpi.invocator())
            || locals.tmpLoanReq.state != LoanReqState::IDLE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Need to figure out who is user in this request.
        // He might be a borrower and creditor. If he is a borrower, we need to send all his tokens back.
        // If he is a creditor, we need to send him back his QUs.
        if (locals.tmpLoanReq.creditor == qpi.invocator())
        {
            // Send him back all his QUs
            qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.tmpLoanReq.priceAmount);
        }

        // Remove loan req from the state
        state.mut()._loanReqs.removeByKey(input.reqId);
        state.mut()._totalReqs--;
    }

    struct TransferShareManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
        uint32 newManagingContractIndex;
    };

    struct TransferShareManagementRights_output
    {
    };

    struct TransferShareManagementRights_locals
    {
        sint64 loanReqsIdx;
        LoanReqInfo tmpLoanReq;

        sint64 releaseResult;
        uint64 userAssetsAmountInHold;

        Asset userReqAsset;
        uint8 userReqAssetIdx;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
    {
        if (qpi.invocationReward() == 0)
        {
            return;
        }

        locals.userAssetsAmountInHold = 0;
        locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);
        while (locals.loanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReq = state.get()._loanReqs.value(locals.loanReqsIdx);

            if ((locals.tmpLoanReq.borrower == qpi.invocator() && ((locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == false) || locals.tmpLoanReq.state == LoanReqState::IDLE))
                || (locals.tmpLoanReq.creditor == qpi.invocator() && (locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == true)))
            {
                // Need to scan all assets in loan request to make sure it's have an assets user want to release
                locals.userReqAssetIdx = 0;
                while (locals.userReqAssetIdx < locals.tmpLoanReq.assetsNum)
                {
                    locals.userReqAsset = locals.tmpLoanReq.assets.get(locals.userReqAssetIdx);
                    if (locals.userReqAsset.assetName == input.asset.assetName
                        && locals.userReqAsset.issuer == input.asset.issuer)
                    {
                        locals.userAssetsAmountInHold += locals.tmpLoanReq.assetAmount.get(locals.userReqAssetIdx);
                    }
                    locals.userReqAssetIdx++;
                }
            }
            locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.loanReqsIdx);
        }

        if (input.numberOfShares > 0 && qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,
                                                                    qpi.invocator(), qpi.invocator(),
                                                                    SELF_INDEX, SELF_INDEX) - sint64(locals.userAssetsAmountInHold) >= input.numberOfShares)
        {
            locals.releaseResult = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(),
                                                        input.numberOfShares, input.newManagingContractIndex,
                                                        input.newManagingContractIndex, qpi.invocationReward());
            if (locals.releaseResult < 0 || locals.releaseResult == INVALID_AMOUNT)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }

            if (qpi.invocationReward() > locals.releaseResult)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.releaseResult);
            }
        }
        else
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    }

    struct PayLoanDebt_input
    {
        uint64 reqId;
    };

    struct PayLoanDebt_output
    {
    };

    struct PayLoanDebt_locals
    {
        LoanReqInfo tmpLoanReq;

        _TransferAssetsFromTo_input transferAssetsFromToInput;
        _TransferAssetsFromTo_output transferAssetsFromToOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(PayLoanDebt)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.tmpLoanReq) == false)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Make sure we check all conditions before moving forward
        if (locals.tmpLoanReq.borrower != qpi.invocator()
            || locals.tmpLoanReq.state != LoanReqState::ACTIVE
            || sint64(locals.tmpLoanReq.debtAmount) > qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Transfer all assets back to the borrower if it was transfered to the creditor
        if (locals.tmpLoanReq.assetsToCreditor)
        {
            locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReq;
            locals.transferAssetsFromToInput.from = locals.tmpLoanReq.creditor;
            locals.transferAssetsFromToInput.to = locals.tmpLoanReq.borrower;
            CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);

            if (!locals.transferAssetsFromToOutput.allGood)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
        }

        // Send all money with percentage to the creditor from borrower
        qpi.transfer(locals.tmpLoanReq.creditor, locals.tmpLoanReq.debtAmount);

        if (qpi.invocationReward() > sint64(locals.tmpLoanReq.debtAmount))
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.tmpLoanReq.debtAmount);
        }

        state.mut()._loanReqs.removeByKey(input.reqId);
        state.mut()._totalReqs--;
    }

    struct GetAllLoanReqs_input
    {
    };

    struct GetAllLoanReqs_output
    {
        Array<LoanOutputInfo, QLOAN_MAX_OUTPUT_NUM> reqs;
        uint64 reqsAmount;
    };

    struct GetAllLoanReqs_locals
    {
        LoanReqInfo tmpLoanReqInfo;

        sint64 outputLoanReqsIdx;
        sint64 activeLoanReqsIdx;

        _FillLoanReqForOutput_input fillLoanReqForOutputInput;
        _FillLoanReqForOutput_output fillLoanReqForOutputOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetAllLoanReqs)
    {
        output.reqsAmount = 0;
        locals.outputLoanReqsIdx = 0;

        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);

        while (locals.activeLoanReqsIdx != NULL_INDEX && locals.outputLoanReqsIdx < QLOAN_MAX_OUTPUT_NUM)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);

            locals.fillLoanReqForOutputInput.loanReqId = state.get()._loanReqs.key(locals.activeLoanReqsIdx);
            locals.fillLoanReqForOutputInput.loanReqInfo = locals.tmpLoanReqInfo;
            CALL(_FillLoanReqForOutput, locals.fillLoanReqForOutputInput, locals.fillLoanReqForOutputOutput);

            output.reqs.set(locals.outputLoanReqsIdx, locals.fillLoanReqForOutputOutput.loanOutputInfo);
            locals.outputLoanReqsIdx++;
            output.reqsAmount++;

            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct GetUserActiveLoanReqs_input
    {
        id userId;
    };

    struct GetUserActiveLoanReqs_output
    {
        Array<LoanOutputInfo, QLOAN_MAX_OUTPUT_NUM> reqs;
        uint64 reqsAmount;
    };

    struct GetUserActiveLoanReqs_locals
    {
        LoanReqInfo tmpLoanReqInfo;

        sint64 outputLoanReqsIdx;
        sint64 activeLoanReqsIdx;

        _FillLoanReqForOutput_input fillLoanReqForOutputInput;
        _FillLoanReqForOutput_output fillLoanReqForOutputOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserActiveLoanReqs)
    {
        output.reqsAmount = 0;
        locals.outputLoanReqsIdx = 0;

        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);

        while (locals.activeLoanReqsIdx != NULL_INDEX && locals.outputLoanReqsIdx < QLOAN_MAX_OUTPUT_NUM)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);
            if ((locals.tmpLoanReqInfo.borrower == input.userId || locals.tmpLoanReqInfo.creditor == input.userId)
                && locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                locals.fillLoanReqForOutputInput.loanReqId = state.get()._loanReqs.key(locals.activeLoanReqsIdx);
                locals.fillLoanReqForOutputInput.loanReqInfo = locals.tmpLoanReqInfo;
                CALL(_FillLoanReqForOutput, locals.fillLoanReqForOutputInput, locals.fillLoanReqForOutputOutput);

                output.reqs.set(locals.outputLoanReqsIdx, locals.fillLoanReqForOutputOutput.loanOutputInfo);
                locals.outputLoanReqsIdx++;
                output.reqsAmount++;
            }

            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct GetFeesInfo_input
    {
    };

    struct GetFeesInfo_output
    {
        uint64 acceptanceFeePercent;
        uint64 burnFeePercent;

        uint64 earnedAmount;
        uint64 distributedAmount;
        uint64 burnedAmount;
        uint64 toQvaultAmount;
    };


    PUBLIC_FUNCTION(GetFeesInfo)
    {
        output.acceptanceFeePercent = QLOAN_ACCEPTANCE_FEE_PERCENT;
        output.burnFeePercent = QLOAN_BURN_PERCENT;

        output.earnedAmount = state.get()._earnedAmount;
        output.distributedAmount = state.get()._distributedAmount;
        output.burnedAmount = state.get()._burnedAmount;
        output.toQvaultAmount = state.get()._toQvaultAmount;
    }

    struct GetUserDebt_input
    {
        id userId;
    };

    struct GetUserDebt_output
    {
        uint64 totalUserDebt;
    };

    struct GetUserDebt_locals
    {
        LoanReqInfo tmpLoanReqInfo;
        sint64 activeLoanReqsIdx;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserDebt)
    {
        locals.activeLoanReqsIdx = 0;
        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);

        while (locals.activeLoanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);
            if (locals.tmpLoanReqInfo.borrower == input.userId && locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                output.totalUserDebt += locals.tmpLoanReqInfo.debtAmount;
            }
            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct GetLoanReqById_input
    {
        uint64 reqId;
    };

    struct GetLoanReqById_output
    {
        LoanOutputInfo loanOutputInfo;
    };

    struct GetLoanReqById_locals
    {
        LoanReqInfo loanReqInfo;

        _FillLoanReqForOutput_input fillLoanReqForOutputInput;
        _FillLoanReqForOutput_output fillLoanReqForOutputOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetLoanReqById)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.loanReqInfo))
        {
            locals.fillLoanReqForOutputInput.loanReqId = input.reqId;
            locals.fillLoanReqForOutputInput.loanReqInfo = locals.loanReqInfo;
            CALL(_FillLoanReqForOutput, locals.fillLoanReqForOutputInput, locals.fillLoanReqForOutputOutput);
            output.loanOutputInfo = locals.fillLoanReqForOutputOutput.loanOutputInfo;
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(PlaceLoanReq, 1);
        REGISTER_USER_PROCEDURE(RemoveLoanReq, 2);
        REGISTER_USER_PROCEDURE(AcceptLoanReq, 3);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 4);
        REGISTER_USER_PROCEDURE(PayLoanDebt, 5);

        REGISTER_USER_FUNCTION(GetAllLoanReqs, 1);
        REGISTER_USER_FUNCTION(GetUserActiveLoanReqs, 2);
        REGISTER_USER_FUNCTION(GetUserDebt, 3);
        REGISTER_USER_FUNCTION(GetFeesInfo, 4);
        REGISTER_USER_FUNCTION(GetLoanReqById, 5);
    }

    struct BEGIN_EPOCH_locals
    {
        sint64 activeLoanReqsIdx;
        LoanReqInfo tmpLoanReqInfo;
        uint64 accruedInterest;

        _TransferAssetsFromTo_input transferAssetsFromToInput;
        _TransferAssetsFromTo_output transferAssetsFromToOutput;
    };


    BEGIN_EPOCH_WITH_LOCALS()
    {
        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);
        while (locals.activeLoanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);

            // Check if contract already ends
            if (locals.tmpLoanReqInfo.epochsLeft == 0 && locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReqInfo;

                // Check if assets were NOT transfered to the creditor, if not - transfer assets from borrower to the creditor
                if (locals.tmpLoanReqInfo.assetsToCreditor == false)
                {
                    locals.transferAssetsFromToInput.from = locals.tmpLoanReqInfo.borrower;
                    locals.transferAssetsFromToInput.to = locals.tmpLoanReqInfo.creditor;
                    CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);
                    if (!locals.transferAssetsFromToOutput.allGood)
                    {
                        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
                        continue;
                    }
                }

                state.mut()._loanReqs.removeByKey(state.get()._loanReqs.key(locals.activeLoanReqsIdx));
                state.mut()._totalReqs--;
            }
            else if (locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                locals.tmpLoanReqInfo.epochsLeft--;
                
                if (div((locals.tmpLoanReqInfo.returnPeriodInEpochs - locals.tmpLoanReqInfo.epochsLeft) * 100, locals.tmpLoanReqInfo.returnPeriodInEpochs) >= 34)
                {
                    locals.tmpLoanReqInfo.debtAmount = sadd(locals.tmpLoanReqInfo.priceAmount, div(smul(locals.tmpLoanReqInfo.fullInterest, locals.tmpLoanReqInfo.returnPeriodInEpochs - locals.tmpLoanReqInfo.epochsLeft), locals.tmpLoanReqInfo.returnPeriodInEpochs));
                }

                state.mut()._loanReqs.replace(state.get()._loanReqs.key(locals.activeLoanReqsIdx), locals.tmpLoanReqInfo);
            }

            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct END_EPOCH_locals
    {
        uint64 amountToBurn;
        uint64 amountToQvault;
    };

    END_EPOCH_WITH_LOCALS()
    {
        locals.amountToBurn = div(smul((state.get()._earnedAmount - state.get()._distributedAmount), QLOAN_BURN_PERCENT), 10000ULL);
        locals.amountToQvault = div(smul((state.get()._earnedAmount - state.get()._distributedAmount), QLOAN_QVAULT_PERCENT), 10000ULL);

        if (state.get()._earnedAmount > state.get()._distributedAmount)
        {
            qpi.burn(locals.amountToBurn);
            qpi.transfer(id(QVAULT_CONTRACT_INDEX, 0, 0, 0), locals.amountToQvault);
            state.mut()._distributedAmount += locals.amountToBurn;
            state.mut()._distributedAmount += locals.amountToQvault;

            state.mut()._burnedAmount += locals.amountToBurn;
            state.mut()._toQvaultAmount += locals.amountToQvault;
        }
    }

    INITIALIZE()
    {
        state.mut()._currentLoanIndex = 1;
        state.mut()._totalReqs = 0;

        state.mut()._earnedAmount = 0;
        state.mut()._distributedAmount = 0;
        state.mut()._burnedAmount = 0;
        state.mut()._toQvaultAmount = 0;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    POST_INCOMING_TRANSFER()
    {
        if (input.type == TransferType::standardTransaction)
        {
            qpi.transfer(input.sourceId, input.amount);
        }
    }
};
