using namespace QPI;

constexpr uint32 QIP_MAX_NUMBER_OF_ICO = 1024;


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
    };

protected:

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
    };
    Array<ICOInfo, QIP_MAX_NUMBER_OF_ICO> icos;

    uint32 numberOfICO;
	uint32 transferRightsFee;
public:

    struct getICOInfo_locals
    {
        ICOInfo ico;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getICOInfo)
    {
        locals.ico = state.icos.get(input.indexOfICO);
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
    }

    struct createICO_locals
    {
        ICOInfo newICO;
        QIPLogger log;
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
            return;
        }
        if (input.saleAmountForPhase1 + input.saleAmountForPhase2 + input.saleAmountForPhase3 != qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX))
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidSaleAmount;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidSaleAmount;
            return;
        }
        if (input.price1 <= 0 || input.price2 <= 0 || input.price3 <= 0)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidPrice;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidPrice;
            return;
        }
        if (input.percent1 + input.percent2 + input.percent3 + input.percent4 + input.percent5 + input.percent6 + input.percent7 + input.percent8 + input.percent9 + input.percent10 != 95)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidPercent;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidPercent;
            return;
        }
        if (qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.saleAmountForPhase1 + input.saleAmountForPhase2 + input.saleAmountForPhase3, SELF) < 0)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidTransfer;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidTransfer;
            return;
        }
        if (state.numberOfICO >= QIP_MAX_NUMBER_OF_ICO)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_overflowICO;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_overflowICO;
            return;
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
        state.icos.set(state.numberOfICO, locals.newICO);
        state.numberOfICO++;
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
        uint64 distributedAmount, price;
        uint32 idx, percent;
        QIPLogger log;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(buyToken)
    {
        if (input.indexOfICO >= state.numberOfICO)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_ICONotFound;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_ICONotFound;
            return;
        }
        locals.ico = state.icos.get(input.indexOfICO);
        if (qpi.epoch() == locals.ico.startEpoch)
        {
            if (input.amount <= locals.ico.remainingAmountForPhase1)
            {
                locals.price = locals.ico.price1;
                locals.percent = locals.ico.percent1;
            }
            else
            {
                locals.log._contractIndex = SELF_INDEX;
                locals.log._type = QIPLogInfo::QIP_invalidAmount;
                locals.log.dst = qpi.invocator();
                locals.log.amt = 0;
                LOG_INFO(locals.log);
                output.returnCode = QIPLogInfo::QIP_invalidAmount;
                return;
            }
        }
        else if (qpi.epoch() == locals.ico.startEpoch + 1)
        {
            if (input.amount <= locals.ico.remainingAmountForPhase2)
            {
                locals.price = locals.ico.price2;
                locals.percent = locals.ico.percent2;
            }
            else
            {
                locals.log._contractIndex = SELF_INDEX;
                locals.log._type = QIPLogInfo::QIP_invalidAmount;
                locals.log.dst = qpi.invocator();
                locals.log.amt = 0;
                LOG_INFO(locals.log);
                output.returnCode = QIPLogInfo::QIP_invalidAmount;
                return ;
            }
        }
        else if (qpi.epoch() == locals.ico.startEpoch + 2)
        {
            if (input.amount <= locals.ico.remainingAmountForPhase3)
            {
                locals.price = locals.ico.price3;
                locals.percent = locals.ico.percent3;
            }
            else
            {
                locals.log._contractIndex = SELF_INDEX;
                locals.log._type = QIPLogInfo::QIP_invalidAmount;
                locals.log.dst = qpi.invocator();
                locals.log.amt = 0;
                LOG_INFO(locals.log);
                output.returnCode = QIPLogInfo::QIP_invalidAmount;
                return ;
            }
        }
        else
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_invalidEpoch;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_invalidEpoch;
            return;
        }
        if (input.amount * locals.price > (uint64)qpi.invocationReward())
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QIPLogInfo::QIP_insufficientInvocationReward;
            locals.log.dst = qpi.invocator();
            locals.log.amt = 0;
            LOG_INFO(locals.log);
            output.returnCode = QIPLogInfo::QIP_insufficientInvocationReward;
            return;
        }
        if (input.amount * locals.price <= (uint64)qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount * locals.price);
        }
        qpi.transferShareOwnershipAndPossession(locals.ico.assetName, locals.ico.issuer, SELF, SELF, input.amount, qpi.invocator());
        locals.distributedAmount = div(input.amount * locals.price * locals.ico.percent1 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent2 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent3 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent4 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent5 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent6 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent7 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent8 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent9 * 1ULL, 100ULL) + div(input.amount * locals.price * locals.ico.percent10 * 1ULL, 100ULL);
        qpi.transfer(locals.ico.address1, div(input.amount * locals.price * locals.ico.percent1 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address2, div(input.amount * locals.price * locals.ico.percent2 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address3, div(input.amount * locals.price * locals.ico.percent3 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address4, div(input.amount * locals.price * locals.ico.percent4 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address5, div(input.amount * locals.price * locals.ico.percent5 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address6, div(input.amount * locals.price * locals.ico.percent6 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address7, div(input.amount * locals.price * locals.ico.percent7 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address8, div(input.amount * locals.price * locals.ico.percent8 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address9, div(input.amount * locals.price * locals.ico.percent9 * 1ULL, 100ULL));
        qpi.transfer(locals.ico.address10, div(input.amount * locals.price * locals.ico.percent10 * 1ULL, 100ULL));
        qpi.distributeDividends(div((input.amount * locals.price - locals.distributedAmount), 676ULL));

        if (qpi.epoch() == locals.ico.startEpoch)
        {
            locals.ico.remainingAmountForPhase1 -= input.amount;
        }
        else if (qpi.epoch() == locals.ico.startEpoch + 1)
        {
            locals.ico.remainingAmountForPhase2 -= input.amount;
        }
        else if (qpi.epoch() == locals.ico.startEpoch + 2)
        {
            locals.ico.remainingAmountForPhase3 -= input.amount;
        }
        state.icos.set(input.indexOfICO, locals.ico);
        output.returnCode = QIPLogInfo::QIP_success;
        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QIPLogInfo::QIP_success;
        locals.log.dst = qpi.invocator();
        locals.log.amt = 0;
        LOG_INFO(locals.log);
    }

    struct TransferShareManagementRights_locals
    {
        ICOInfo ico;
        uint32 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < state.transferRightsFee)
		{
			return ;
		}

        for (locals.i = 0 ; locals.i < state.numberOfICO; locals.i++)
        {
            locals.ico = state.icos.get(locals.i);
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
				input.newManagingContractIndex, input.newManagingContractIndex, state.transferRightsFee) < 0)
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
				if (qpi.invocationReward() > state.transferRightsFee)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.transferRightsFee);
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
    }

    INITIALIZE()
	{
        state.transferRightsFee = 100;
	}

	struct END_EPOCH_locals
	{
        ICOInfo ico;
        sint32 idx;
	};

	END_EPOCH_WITH_LOCALS()
	{
		for(locals.idx = 0; locals.idx < (sint32)state.numberOfICO; locals.idx++)
		{
            locals.ico = state.icos.get(locals.idx);
            if (locals.ico.startEpoch == qpi.epoch() && locals.ico.remainingAmountForPhase1 > 0)
            {
                locals.ico.remainingAmountForPhase2 += locals.ico.remainingAmountForPhase1; 
                locals.ico.remainingAmountForPhase1 = 0;
                state.icos.set(locals.idx, locals.ico);
            }
            if (locals.ico.startEpoch + 1 == qpi.epoch() && locals.ico.remainingAmountForPhase2 > 0)
            {
                locals.ico.remainingAmountForPhase3 += locals.ico.remainingAmountForPhase2;
                locals.ico.remainingAmountForPhase2 = 0;
                state.icos.set(locals.idx, locals.ico);
            }
            if (locals.ico.startEpoch + 2 == qpi.epoch())
            {
                if (locals.ico.remainingAmountForPhase3 > 0) 
                {
                    qpi.transferShareOwnershipAndPossession(locals.ico.assetName, locals.ico.issuer, SELF, SELF, locals.ico.remainingAmountForPhase3, locals.ico.creatorOfICO);
                }
                state.icos.set(locals.idx, state.icos.get(state.numberOfICO - 1));
                state.numberOfICO--;
                locals.idx--;
            }
		}
	}

    PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }

};