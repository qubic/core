using namespace QPI;

constexpr uint32 QIP_MAX_NUMBER_OF_ICO = 1024;
constexpr sint64 QIP_ICO_SETUP_FEE = 1000000000;
constexpr sint64 QIP_ICO_SETUP_FEE_DISTRIBUTION_PERCENT = 90;
constexpr sint64 QIP_INVESTORS_DISTRIBUTION_PERCENT = 97;
constexpr sint64 QIP_MAX_PENALTY_PERCENT_FOR_VESTING = 40;
constexpr uint64 QIP_DEVELOPMENT_FUND_PERCENT = 1;
constexpr uint64 QIP_SHAREHOLDERS_PERCENT = 2;
constexpr uint32 QIP_MAX_VESTING_PERIOD = 100;
constexpr uint64 QIP_MIN_PARTICIPATION_AMOUNT = 5000000;

enum QIPLogInfo {
    QIP_success = 0,
    QIP_invalidStartEpoch = 1,
    QIP_invalidSaleAmount = 2,
    QIP_invalidPrice = 3,
    QIP_invalidPercent = 4,
    QIP_invalidTransfer = 5,
    QIP_overflowICO = 6,
    QIP_ICONotFound = 7,
    QIP_invalidAmount = 8,
    QIP_invalidEpoch = 9,
    QIP_insufficientInvocationReward = 10,
    QIP_invalidVestingPeriod = 11,
    QIP_maxBuyers = 12,
    QIP_fundsAlreadyReturned = 13,
    QIP_amountBelowMinimum = 14,
};

struct QIPLogger
{
    uint32 _contractIndex;
    uint32 _type;
    id dst;
    sint64 amt;
	sint8 _terminator;
};

struct QIP2
{
};

struct QIP : public ContractBase
{
public:
    struct BuyerInfo
    {
        sint64 toReceive;
        sint64 received;
        sint64 totalQuPayed;
        bit isReturned;
    };

    struct IcoBuyerKey
    {
        id creator;
        id issuer;
        uint64 assetName;
        id buyer;

        bool operator==(const IcoBuyerKey other) const
        {
            return creator == other.creator
                    && issuer == other.issuer
                    && assetName == other.assetName
                    && buyer == other.buyer;
        }
    };

    struct ICOInfo
    {
        id creatorOfICO;
        id issuer;
        id address1, address2, address3, address4, address5, address6, address7, address8, address9, address10;
        uint64 assetName;
        uint64 price1;
        uint64 price2;
        uint64 price3;
        uint64 saleAmountForPhase1;
        uint64 saleAmountForPhase2;
        uint64 saleAmountForPhase3;
        uint64 remainingAmountForPhase1;
        uint64 remainingAmountForPhase2;
        uint64 remainingAmountForPhase3;
        uint32 percent1, percent2, percent3, percent4, percent5, percent6, percent7, percent8, percent9, percent10;
        uint32 startEpoch;
        bit burnRemainingTokens;
        bit isVested;
        uint32 vestingPeriod;
        uint64 distributedQu;
        uint64 quToDistribute;
        uint64 returnedQu;
    };

    struct StateData
    {
        Array<ICOInfo, QIP_MAX_NUMBER_OF_ICO> icos;
        uint32 numberOfICO;
        uint32 transferRightsFee;
        id developmentFundAddress;
        HashMap<IcoBuyerKey, BuyerInfo, 131072> buyersInfo;
    };

    struct createICO_input
    {
        id issuer;
        id address1, address2, address3, address4, address5, address6, address7, address8, address9, address10;
        uint64 assetName;
        uint64 price1;
        uint64 price2;
        uint64 price3;
        uint64 saleAmountForPhase1;
        uint64 saleAmountForPhase2;
        uint64 saleAmountForPhase3;
        uint32 percent1, percent2, percent3, percent4, percent5, percent6, percent7, percent8, percent9, percent10;
        uint32 startEpoch;
        bit burnRemainingTokens;
        bit isVested;
        uint32 vestingPeriod;
    };
    struct createICO_output
    {
        sint32 returnCode;
    };

    struct buyToken_input
    {
        uint32 indexOfICO;
        uint64 amount;
    };
    struct buyToken_output
    {
        sint32 returnCode;
    };

    struct returnFunds_input
    {
        uint32 indexOfICO;
    };
    struct returnFunds_output
    {
        sint32 returnCode;
    };

    struct TransferShareManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
        uint32 newManagingContractIndex;
    };
    struct TransferShareManagementRights_output
    {
        sint64 transferredNumberOfShares;
    };

    struct getICOInfo_input
    {
        uint32 indexOfICO;
    };
    struct getICOInfo_output
    {
        id creatorOfICO;
        id issuer;
        id address1, address2, address3, address4, address5, address6, address7, address8, address9, address10;
        uint64 assetName;
        uint64 price1;
        uint64 price2;
        uint64 price3;
        uint64 saleAmountForPhase1;
        uint64 saleAmountForPhase2;
        uint64 saleAmountForPhase3;
        uint64 remainingAmountForPhase1;
        uint64 remainingAmountForPhase2;
        uint64 remainingAmountForPhase3;
        uint32 percent1, percent2, percent3, percent4, percent5, percent6, percent7, percent8, percent9, percent10;
        uint32 startEpoch;
        bit burnRemainingTokens;
        bit isVested;
        uint32 vestingPeriod;
    };

    struct getICOInfo_locals
    {
        ICOInfo ico;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getICOInfo)
    {
        locals.ico = state.get().icos.get(input.indexOfICO);
        output.creatorOfICO = locals.ico.creatorOfICO;
        output.issuer = locals.ico.issuer;
        output.address1 = locals.ico.address1;
        output.address2 = locals.ico.address2;
        output.address3 = locals.ico.address3;
        output.address4 = locals.ico.address4;
        output.address5 = locals.ico.address5;
        output.address6 = locals.ico.address6;
        output.address7 = locals.ico.address7;
        output.address8 = locals.ico.address8;
        output.address9 = locals.ico.address9;
        output.address10 = locals.ico.address10;
        output.assetName = locals.ico.assetName;
        output.price1 = locals.ico.price1;
        output.price2 = locals.ico.price2;
        output.price3 = locals.ico.price3;
        output.saleAmountForPhase1 = locals.ico.saleAmountForPhase1;
        output.saleAmountForPhase2 = locals.ico.saleAmountForPhase2;
        output.saleAmountForPhase3 = locals.ico.saleAmountForPhase3;
        output.remainingAmountForPhase1 = locals.ico.remainingAmountForPhase1;
        output.remainingAmountForPhase2 = locals.ico.remainingAmountForPhase2;
        output.remainingAmountForPhase3 = locals.ico.remainingAmountForPhase3;
        output.percent1 = locals.ico.percent1;
        output.percent2 = locals.ico.percent2;
        output.percent3 = locals.ico.percent3;
        output.percent4 = locals.ico.percent4;
        output.percent5 = locals.ico.percent5;
        output.percent6 = locals.ico.percent6;
        output.percent7 = locals.ico.percent7;
        output.percent8 = locals.ico.percent8;
        output.percent9 = locals.ico.percent9;
        output.percent10 = locals.ico.percent10;
        output.startEpoch = locals.ico.startEpoch;
        output.burnRemainingTokens = locals.ico.burnRemainingTokens;
        output.isVested = locals.ico.isVested;
        output.vestingPeriod = locals.ico.vestingPeriod;
    }

    struct createICO_locals
    {
        ICOInfo newICO;
        QIPLogger log;
        sint64 amountPerShare;
        BuyerInfo emptyInfo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(createICO)
    {
        if (input.startEpoch <= (uint32)qpi.epoch() + 1)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidStartEpoch;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidStartEpoch;
        }
        else if (input.saleAmountForPhase1 + input.saleAmountForPhase2 + input.saleAmountForPhase3 != qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX))
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidSaleAmount;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidSaleAmount;
        }
        else if (input.price1 <= 0 || input.price2 <= 0 || input.price3 <= 0)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidPrice;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidPrice;
        }
        else if (input.percent1 + input.percent2 + input.percent3 + input.percent4 + input.percent5 + input.percent6 + input.percent7 + input.percent8 + input.percent9 + input.percent10 != QIP_INVESTORS_DISTRIBUTION_PERCENT
                    || (input.isVested && input.percent1 > QIP_MAX_PENALTY_PERCENT_FOR_VESTING))
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidPercent;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidPercent;
        }
        else if (state.get().numberOfICO >= QIP_MAX_NUMBER_OF_ICO)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_overflowICO;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_overflowICO;
        }
        else if (qpi.invocationReward() < QIP_ICO_SETUP_FEE)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_insufficientInvocationReward;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_insufficientInvocationReward;
        }
        else if (input.isVested && (input.vestingPeriod == 0 || input.vestingPeriod > QIP_MAX_VESTING_PERIOD))
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidVestingPeriod;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidVestingPeriod;
        }
        else if (qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.saleAmountForPhase1 + input.saleAmountForPhase2 + input.saleAmountForPhase3, SELF) < 0)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidTransfer;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidTransfer;
        }

        if (output.returnCode != 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }
        else if (qpi.invocationReward() > QIP_ICO_SETUP_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QIP_ICO_SETUP_FEE);
        }
        locals.newICO.creatorOfICO = qpi.invocator();
        locals.newICO.issuer = input.issuer;
        locals.newICO.address1 = input.address1;
        locals.newICO.address2 = input.address2;
        locals.newICO.address3 = input.address3;
        locals.newICO.address4 = input.address4;
        locals.newICO.address5 = input.address5;
        locals.newICO.address6 = input.address6;
        locals.newICO.address7 = input.address7;
        locals.newICO.address8 = input.address8;
        locals.newICO.address9 = input.address9;
        locals.newICO.address10 = input.address10;
        locals.newICO.assetName = input.assetName;
        locals.newICO.price1 = input.price1;
        locals.newICO.price2 = input.price2;
        locals.newICO.price3 = input.price3;
        locals.newICO.saleAmountForPhase1 = input.saleAmountForPhase1;
        locals.newICO.saleAmountForPhase2 = input.saleAmountForPhase2;
        locals.newICO.saleAmountForPhase3 = input.saleAmountForPhase3;
        locals.newICO.remainingAmountForPhase1 = input.saleAmountForPhase1;
        locals.newICO.remainingAmountForPhase2 = input.saleAmountForPhase2;
        locals.newICO.remainingAmountForPhase3 = input.saleAmountForPhase3;
        locals.newICO.percent1 = input.percent1;
        locals.newICO.percent2 = input.percent2;
        locals.newICO.percent3 = input.percent3;
        locals.newICO.percent4 = input.percent4;
        locals.newICO.percent5 = input.percent5;
        locals.newICO.percent6 = input.percent6;
        locals.newICO.percent7 = input.percent7;
        locals.newICO.percent8 = input.percent8;
        locals.newICO.percent9 = input.percent9;
        locals.newICO.percent10 = input.percent10;
        locals.newICO.startEpoch = input.startEpoch;
        locals.newICO.burnRemainingTokens = input.burnRemainingTokens;
        locals.newICO.isVested = input.isVested;
        locals.newICO.vestingPeriod = input.vestingPeriod;
        state.mut().icos.set(state.get().numberOfICO, locals.newICO);
        state.mut().numberOfICO++;

        locals.amountPerShare = QPI::div(smul(QPI::div(QIP_ICO_SETUP_FEE, 10LL), 6LL), 676LL);
        qpi.distributeDividends(locals.amountPerShare);
        qpi.transfer(state.get().developmentFundAddress, smul(QPI::div(QIP_ICO_SETUP_FEE, 10LL), 3LL));
        qpi.burn(QIP_ICO_SETUP_FEE - smul(locals.amountPerShare, 676LL) - smul(QPI::div(QIP_ICO_SETUP_FEE, 10LL), 3LL));

        output.returnCode = QIPLogInfo::QIP_success;
        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QIPLogInfo::QIP_success;
        locals.log.dst = qpi.invocator();
        locals.log.amt = 0;
        LOG_INFO(locals.log);
    }

    struct buyToken_locals
    {
        ICOInfo ico;
        uint64 distributedAmount, price, requiredPayment;
        uint32 idx, percent;
        QIPLogger log;
        uint16 salePhase;
        uint64 currentPrice;
        sint64 elementIndex;
        IcoBuyerKey icoBuyerKey;
        BuyerInfo buyerInfo;
        uint32 errorCode;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(buyToken)
    {
        if (input.indexOfICO >= state.get().numberOfICO)
        {
            locals.errorCode = QIPLogInfo::QIP_ICONotFound;
        }
        else
        {
            locals.ico = state.get().icos.get(input.indexOfICO);
            locals.salePhase = qpi.epoch() - locals.ico.startEpoch;

            if (locals.salePhase == 0)
            {
                locals.price = locals.ico.price1;

                if (input.amount > locals.ico.remainingAmountForPhase1)
                {
                    locals.errorCode = QIPLogInfo::QIP_invalidAmount;
                } 
            }
            else if (locals.salePhase == 1)
            {
                locals.price = locals.ico.price2;

                if (input.amount > locals.ico.remainingAmountForPhase2)
                {
                    locals.errorCode = QIPLogInfo::QIP_invalidAmount;
                }
            }
            else if (locals.salePhase == 2)
            {
                locals.price = locals.ico.price3;

                if (input.amount > locals.ico.remainingAmountForPhase3)
                {
                    locals.errorCode = QIPLogInfo::QIP_invalidAmount;
                }
            }
            else
            {
                locals.errorCode = QIPLogInfo::QIP_invalidEpoch;
            }

            if (locals.errorCode == 0)
            {
                if (input.amount == 0 || input.amount > MAX_AMOUNT)
                {
                    locals.errorCode = QIPLogInfo::QIP_invalidAmount;
                }
                else if (locals.price == 0 || input.amount * locals.price > MAX_AMOUNT)
                {
                    locals.errorCode = QIPLogInfo::QIP_insufficientInvocationReward;
                }
                else
                {
                    locals.requiredPayment = input.amount * locals.price;

                    if (locals.requiredPayment < QIP_MIN_PARTICIPATION_AMOUNT)
                    {
                        locals.errorCode = QIPLogInfo::QIP_amountBelowMinimum;
                    }
                    else if (locals.requiredPayment > (uint64)qpi.invocationReward())
                    {
                        locals.errorCode = QIPLogInfo::QIP_insufficientInvocationReward;
                    }
                }
            }
        }

        if (locals.errorCode != QIPLogInfo::QIP_success)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = locals.errorCode;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = locals.errorCode;
            return;
        }

        if (state.get().buyersInfo.population() == state.get().buyersInfo.capacity())
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_maxBuyers;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_maxBuyers;
            return;
        }

        if (locals.ico.isVested)
        {
            locals.icoBuyerKey.assetName = locals.ico.assetName;
            locals.icoBuyerKey.issuer = locals.ico.issuer;
            locals.icoBuyerKey.creator = locals.ico.creatorOfICO;
            locals.icoBuyerKey.buyer = qpi.invocator();

            if (state.get().buyersInfo.get(locals.icoBuyerKey, locals.buyerInfo))
            {
                locals.buyerInfo.toReceive += sint64(input.amount);
                locals.buyerInfo.totalQuPayed += sint64(locals.requiredPayment);
                state.mut().buyersInfo.replace(locals.icoBuyerKey, locals.buyerInfo);
            }
            else
            {
                locals.buyerInfo.toReceive = sint64(input.amount);
                locals.buyerInfo.totalQuPayed = sint64(locals.requiredPayment);
                state.mut().buyersInfo.set(locals.icoBuyerKey, locals.buyerInfo);
            }
            qpi.transfer(locals.ico.address1, QPI::div(locals.requiredPayment * locals.ico.percent1, 100ULL));
            qpi.transfer(state.get().developmentFundAddress, QPI::div(locals.requiredPayment * QIP_DEVELOPMENT_FUND_PERCENT, 100ULL));
            qpi.distributeDividends(QPI::div(QPI::div(locals.requiredPayment * QIP_SHAREHOLDERS_PERCENT, 100ULL), 676ULL));
        }
        else
        {
            if (qpi.transferShareOwnershipAndPossession(locals.ico.assetName, locals.ico.issuer, SELF, SELF, input.amount, qpi.invocator()) < 0)
            {
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                locals.log._contractIndex = SELF_INDEX;
                locals.log._type = QIPLogInfo::QIP_invalidTransfer;
                locals.log.dst = qpi.invocator();
                locals.log.amt = 0;
                LOG_INFO(locals.log);
                output.returnCode = QIPLogInfo::QIP_invalidTransfer;
                return;
            }
            locals.distributedAmount = div(locals.requiredPayment * locals.ico.percent1 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent2 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent3 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent4 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent5 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent6 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent7 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent8 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent9 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * locals.ico.percent10 * 1ULL, 100ULL)
                                        + div(locals.requiredPayment * QIP_DEVELOPMENT_FUND_PERCENT * 1ULL, 100ULL);
            qpi.transfer(locals.ico.address1, div(locals.requiredPayment * locals.ico.percent1 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address2, div(locals.requiredPayment * locals.ico.percent2 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address3, div(locals.requiredPayment * locals.ico.percent3 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address4, div(locals.requiredPayment * locals.ico.percent4 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address5, div(locals.requiredPayment * locals.ico.percent5 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address6, div(locals.requiredPayment * locals.ico.percent6 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address7, div(locals.requiredPayment * locals.ico.percent7 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address8, div(locals.requiredPayment * locals.ico.percent8 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address9, div(locals.requiredPayment * locals.ico.percent9 * 1ULL, 100ULL));
            qpi.transfer(locals.ico.address10, div(locals.requiredPayment * locals.ico.percent10 * 1ULL, 100ULL));
            qpi.transfer(state.get().developmentFundAddress, div(locals.requiredPayment * QIP_DEVELOPMENT_FUND_PERCENT * 1ULL, 100ULL));
            qpi.distributeDividends(div(locals.requiredPayment - locals.distributedAmount, 676ULL));
        }

        if ((uint64) qpi.invocationReward() > locals.requiredPayment)
        {
           qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.requiredPayment);
        }

        if (locals.salePhase == 0)
        {
            locals.ico.remainingAmountForPhase1 -= input.amount;
        }
        else if (locals.salePhase == 1)
        {
            locals.ico.remainingAmountForPhase2 -= input.amount;
        }
        else if (locals.salePhase == 2)
        {
            locals.ico.remainingAmountForPhase3 -= input.amount;
        }

        if (locals.ico.isVested)
        {
            locals.currentPrice = (locals.salePhase == 0) ? locals.ico.price1 :
                                    (locals.salePhase == 1) ? locals.ico.price2 :
                                                            locals.ico.price3;
            locals.ico.quToDistribute += input.amount * locals.currentPrice;
        }

        state.mut().icos.set(input.indexOfICO, locals.ico);
        output.returnCode = QIPLogInfo::QIP_success;
        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QIPLogInfo::QIP_success;
        locals.log.dst = qpi.invocator();
        locals.log.amt = 0;
        LOG_INFO(locals.log);
    }

    struct returnFunds_locals
    {
        ICOInfo ico;
        QIPLogger log;
        IcoBuyerKey icoBuyerKey;
        BuyerInfo buyerInfo;
        sint64 returnAmount;
        sint64 fullReturnAmount;
        sint64 elementIndex;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(returnFunds)
    {
        if (input.indexOfICO >= state.get().numberOfICO)
        {
            locals.log._type = QIPLogInfo::QIP_ICONotFound;
        }

        if (locals.log._type == QIP_success)
        {
            locals.log._type = QIPLogInfo::QIP_ICONotFound;
            locals.ico = state.get().icos.get(input.indexOfICO);

            locals.icoBuyerKey.assetName = locals.ico.assetName;
            locals.icoBuyerKey.issuer = locals.ico.issuer;
            locals.icoBuyerKey.creator = locals.ico.creatorOfICO;
            locals.icoBuyerKey.buyer = qpi.invocator();

            if (state.get().buyersInfo.get(locals.icoBuyerKey, locals.buyerInfo))
            {
                locals.log._type = QIP_success;
            }
        }
        
        if (locals.log._type == QIP_success && locals.buyerInfo.isReturned)
        {
            locals.log._type = QIPLogInfo::QIP_fundsAlreadyReturned;
        }

        if (locals.log._type != QIP_success)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            locals.log._contractIndex = SELF_INDEX;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = locals.log._type;
            return;
        }

        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        locals.fullReturnAmount = QPI::div(locals.buyerInfo.totalQuPayed * (QIP_INVESTORS_DISTRIBUTION_PERCENT - locals.ico.percent1), 100ll);
        locals.returnAmount = locals.fullReturnAmount - QPI::div(locals.fullReturnAmount, sint64(locals.ico.vestingPeriod)) * (qpi.epoch() - locals.ico.startEpoch - 3);
        qpi.transfer(qpi.invocator(), locals.returnAmount);
        locals.buyerInfo.isReturned = true;
        locals.ico.quToDistribute -= locals.buyerInfo.totalQuPayed;
        state.mut().buyersInfo.replace(locals.icoBuyerKey, locals.buyerInfo);
        state.mut().icos.set(input.indexOfICO, locals.ico);
    }

    struct TransferShareManagementRights_locals
    {
        ICOInfo ico;
        uint32 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
    {
        if (qpi.invocationReward() < state.get().transferRightsFee)
        {
            return ;
        }

        for (locals.i = 0 ; locals.i < state.get().numberOfICO; locals.i++)
        {
            locals.ico = state.get().icos.get(locals.i);
            if (locals.ico.issuer == input.asset.issuer && locals.ico.assetName == input.asset.assetName)
            {
                return ;
            }
        }

        if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
        {
            // not enough shares available
            output.transferredNumberOfShares = 0;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
        }
        else
        {
            if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
                input.newManagingContractIndex, input.newManagingContractIndex, state.get().transferRightsFee) < 0)
            {
                // error
                output.transferredNumberOfShares = 0;
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
            }
            else
            {
                // success
                output.transferredNumberOfShares = input.numberOfShares;
                if (qpi.invocationReward() > state.get().transferRightsFee)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.get().transferRightsFee);
                }
            }
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getICOInfo, 1);

        REGISTER_USER_PROCEDURE(createICO, 1);
        REGISTER_USER_PROCEDURE(buyToken, 2);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 3);
        REGISTER_USER_PROCEDURE(returnFunds, 4);
    }

    INITIALIZE()
    {
        state.mut().transferRightsFee = 100;
    }

    struct BEGIN_EPOCH_locals
    {
        id address1;
        id address2;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        state.mut().developmentFundAddress = ID(_Q, _O, _W, _F, _W, _Q, _C, _P, _C, _M, _V, _S, _W, _G, _Z, _K, _H, _V, _D, _D, _A, _B, _L, _L, _T, _E, _N, _D, _T, _H, _H, _V, _T, _P, _L, _X, _B, _N, _V, _W, _E, _D, _P, _G, _Q, _A, _E, _L, _Y, _L, _I, _S, _P, _G, _T, _A);
        locals.address1 = ID(_K, _D, _F, _D, _H, _J, _F, _E, _B, _J, _W, _E, _Z, _D, _F, _I, _C, _T, _P, _Z, _N, _N, _D, _T, _A, _Z, _S, _A, _H, _F, _G, _Q, _H, _D, _O, _C, _X, _R, _P, _G, _D, _D, _I, _B, _F, _P, _D, _D, _Q, _Z, _H, _O, _V, _H, _V, _D);
        locals.address2 = ID(_Z, _V, _V, _C, _R, _T, _G, _Y, _W, _B, _K, _H, _X, _D, _F, _M, _Q, _D, _X, _F, _V, _B, _D, _N, _N, _L, _I, _D, _K, _Q, _F, _N, _B, _Q, _K, _V, _U, _L, _R, _N, _H, _F, _E, _Z, _K, _C, _K, _B, _H, _X, _I, _B, _E, _N, _S, _E);

        if (qpi.epoch() == 199)
        {
            qpi.transfer(locals.address1, 30000000);
            qpi.transfer(locals.address2, 100000000);
        }
    }

    struct END_EPOCH_locals
    {
        ICOInfo ico;
        sint32 idx;
        id buyer;
        sint64 elementIndex;
        sint64 buyerIndex;
        sint64 tempAmount;
        IcoBuyerKey icoBuyerKey;
        BuyerInfo buyerInfo;
    };

    END_EPOCH_WITH_LOCALS()
    {
        for(locals.idx = 0; locals.idx < (sint32)state.get().numberOfICO; locals.idx++)
        {
            locals.ico = state.get().icos.get(locals.idx);
            if (locals.ico.startEpoch == qpi.epoch() && locals.ico.remainingAmountForPhase1 > 0)
            {
                locals.ico.remainingAmountForPhase2 += locals.ico.remainingAmountForPhase1;
                locals.ico.remainingAmountForPhase1 = 0;
                state.mut().icos.set(locals.idx, locals.ico);
            }
            else if (locals.ico.startEpoch + 1 == qpi.epoch() && locals.ico.remainingAmountForPhase2 > 0)
            {
                locals.ico.remainingAmountForPhase3 += locals.ico.remainingAmountForPhase2;
                locals.ico.remainingAmountForPhase2 = 0;
                state.mut().icos.set(locals.idx, locals.ico);
            }
            else if (locals.ico.startEpoch + 2 == qpi.epoch())
            {
                if (locals.ico.remainingAmountForPhase3 > 0)
                {
                    if (locals.ico.burnRemainingTokens)
                    {
                        qpi.transferShareOwnershipAndPossession(locals.ico.assetName, locals.ico.issuer, SELF, SELF, locals.ico.remainingAmountForPhase3, NULL_ID);
                    }
                    else 
                    {
                        qpi.transferShareOwnershipAndPossession(locals.ico.assetName, locals.ico.issuer, SELF, SELF, locals.ico.remainingAmountForPhase3, locals.ico.creatorOfICO);
                    }
                }

                if (!locals.ico.isVested)
                {
                    state.mut().icos.set(locals.idx, state.get().icos.get(state.get().numberOfICO - 1));
                    state.mut().numberOfICO--;
                    locals.idx--;
                }
            }
            else if (locals.ico.isVested)
            {
                locals.elementIndex = state.get().buyersInfo.nextElementIndex(NULL_INDEX);
                while (locals.elementIndex != NULL_INDEX)
                {
                    if (state.get().buyersInfo.key(locals.elementIndex).assetName == locals.ico.assetName
                        && state.get().buyersInfo.key(locals.elementIndex).issuer == locals.ico.issuer
                        && state.get().buyersInfo.key(locals.elementIndex).creator == locals.ico.creatorOfICO
                        && !state.get().buyersInfo.value(locals.elementIndex).isReturned)
                    {
                        locals.buyerInfo = state.get().buyersInfo.value(locals.elementIndex);
                        locals.icoBuyerKey = state.get().buyersInfo.key(locals.elementIndex);
                        if (qpi.epoch() == locals.ico.startEpoch + locals.ico.vestingPeriod + 2) // last vesting epoch
                        {
                            locals.tempAmount = locals.buyerInfo.toReceive - locals.buyerInfo.received;
                            state.mut().buyersInfo.removeByIndex(locals.elementIndex);
                        }
                        else 
                        {
                            locals.tempAmount = QPI::div(locals.buyerInfo.toReceive, sint64(locals.ico.vestingPeriod));
                            locals.buyerInfo.received += locals.tempAmount;
                            state.mut().buyersInfo.replace(locals.icoBuyerKey, locals.buyerInfo);
                        }
                        qpi.transferShareOwnershipAndPossession(locals.ico.assetName, locals.ico.issuer, SELF, SELF, locals.tempAmount, locals.icoBuyerKey.buyer);
                    }
                    locals.elementIndex = state.get().buyersInfo.nextElementIndex(locals.elementIndex);
                }

                qpi.transfer(locals.ico.address2, div(div(locals.ico.quToDistribute * locals.ico.percent2, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address3, div(div(locals.ico.quToDistribute * locals.ico.percent3, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address4, div(div(locals.ico.quToDistribute * locals.ico.percent4, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address5, div(div(locals.ico.quToDistribute * locals.ico.percent5, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address6, div(div(locals.ico.quToDistribute * locals.ico.percent6, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address7, div(div(locals.ico.quToDistribute * locals.ico.percent7, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address8, div(div(locals.ico.quToDistribute * locals.ico.percent8, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address9, div(div(locals.ico.quToDistribute * locals.ico.percent9, 100ULL), uint64(locals.ico.vestingPeriod)));
                qpi.transfer(locals.ico.address10, div(div(locals.ico.quToDistribute * locals.ico.percent10, 100ULL), uint64(locals.ico.vestingPeriod)));

                if (qpi.epoch() == locals.ico.startEpoch + locals.ico.vestingPeriod + 2) // last vesting epoch
                {
                    state.mut().icos.set(locals.idx, state.get().icos.get(state.get().numberOfICO - 1));
                    state.mut().numberOfICO--;
                    locals.idx--;
                }
            }
        }

        state.mut().buyersInfo.cleanupIfNeeded();
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }
};
