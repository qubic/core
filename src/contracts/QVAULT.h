using namespace QPI;

constexpr uint64 QVAULT_QCAP_ASSETNAME = 1346454353;
constexpr uint64 QVAULT_QVAULT_ASSETNAME = 92686824592977;
constexpr uint64 QVAULT_PROPOSAL_FEE = 10000000;
constexpr uint64 QVAULT_IPO_PARTICIPATION_MIN_FUND = 1000000000;
constexpr uint32 QVAULT_QCAP_MAX_SUPPLY = 21000000;
constexpr uint32 QVAULT_MAX_NUMBER_OF_BANNED_ADDRESSES = 16;
constexpr uint32 QVAULT_MAX_NUMBER_OF_PROPOSAL = 65536;
constexpr uint32 QVAULT_2025MAX_QCAP_SALE_AMOUNT = 10714286;
constexpr uint32 QVAULT_2026MAX_QCAP_SALE_AMOUNT = 15571429;
constexpr uint32 QVAULT_2027MAX_QCAP_SALE_AMOUNT = 18000000;

constexpr sint32 QVAULT_SUCCESS = 0;
constexpr sint32 QVAULT_INSUFFICIENT_QCAP = 1;
constexpr sint32 QVAULT_NOT_ENOUGH_STAKE = 2;
constexpr sint32 QVAULT_NOT_STAKER = 3;

enum QVAULTLogInfo {
    QvaultSuccess = 0,
    QvaultInsufficientFund = 1,
    QvaultNotTransferredShare = 2,
    QvaultEndedProposal = 3,
    QvaultNotStaked = 4,
    QvaultNoVotingPower = 5,
    QvaultOverflowSaleAmount = 6,
    QvaultNotInTime = 7,
    QvaultNotFair = 8,
    QvaultInsufficientQcap = 9,
    QvaultFailed = 10,
    QvaultInsufficientShare = 11,
    QvaultErrorTransferAsset = 12,
    QvaultInsufficientVotingPower = 13,
    QvaultInputError = 14,
    QvaultOverflowProposal = 15,
    QvaultMaxMuslimId = 16,
    QvaultDuplicatedMuslimId = 17,
    QvaultNotMuslimId = 18
};

struct QVAULT2
{
};

struct QVAULT : public ContractBase
{

public:

    struct getData_input
    {
    };

    struct getData_output
    {
        id adminAddress;
        uint64 totalVotingPower;
        uint64 proposalCreateFund;
        uint64 reinvestingFund;
        uint64 totalNotMSRevenue;
        uint64 totalMuslimRevenue;
        uint64 fundForBurn;
        uint64 totalStakedQcapAmount;
        uint64 qcapMarketCap;
        uint64 raisedFundByQcap;
        uint64 lastRoundPriceOfQcap;
        uint64 revenueByQearn;
        uint32 qcapSoldAmount;
        uint32 shareholderDividend;
        uint32 QCAPHolderPermille;
        uint32 reinvestingPermille;
        uint32 devPermille;
        uint32 burnPermille;
        uint32 qcapBurnPermille;
        uint32 numberOfStaker;
        uint32 numberOfVotingPower;
        uint32 numberOfGP;
        uint32 numberOfQCP;
        uint32 numberOfIPOP;
        uint32 numberOfQEarnP;
        uint32 numberOfFundP;
        uint32 numberOfMKTP;
        uint32 numberOfAlloP;
        uint32 transferRightsFee;
        uint32 numberOfMuslim;
        uint32 numberOfMuslimShare;
        uint32 minQuorumRq;
        uint32 maxQuorumRq;
        uint32 totalQcapBurntAmount;
        uint32 circulatingSupply;
        uint32 quorumPercent;
    };

    struct stake_input
    {
        uint32 amount;
    };

    struct stake_output
    {
        uint32 returnCode;
    };

    struct unStake_input
    {
        uint32 amount;
    };

    struct unStake_output
    {
        uint32 returnCode;
    };

    struct submitGP_input
    {

    };

    struct submitGP_output
    {
        uint32 returnCode;
    };

    struct submitQCP_input
    {
        uint32 newQuorumPercent;
    };

    struct submitQCP_output
    {
        uint32 returnCode;
    };

    struct submitIPOP_input
    {
        uint32 ipoContractIndex;
    };

    struct submitIPOP_output
    {
        uint32 returnCode;
    };

    struct submitQEarnP_input
    {
        uint64 amountPerEpoch;
        uint32 numberOfEpoch;
    };

    struct submitQEarnP_output
    {
        uint32 returnCode;
    };

    struct submitFundP_input
    {
        uint64 priceOfOneQcap;
        uint32 amountOfQcap;
    };

    struct submitFundP_output
    {
        uint32 returnCode;
    };

    struct submitMKTP_input
    {
        uint64 amountOfQubic;
        uint64 shareName;
        uint32 amountOfQcap;
        uint32 indexOfShare;
        uint32 amountOfShare;
    };

    struct submitMKTP_output
    {
        uint32 returnCode;
    };

    struct submitAlloP_input
    {
        uint32 reinvested;
        uint32 team;
        uint32 burn;
        uint32 distribute;
    };

    struct submitAlloP_output
    {
        uint32 returnCode;
    };

    struct submitMSP_input
    {
        uint32 shareIndex;
    };

    struct submitMSP_output
    {
        uint32 returnCode;
    };

    struct voteInProposal_input
    {
        uint64 priceOfIPO;
        uint32 proposalType;
        uint32 proposalId;
        bit yes;
    };

    struct voteInProposal_output
    {
        uint32 returnCode;
    };

    struct buyQcap_input
    {
        uint32 amount;
    };

    struct buyQcap_output
    {
        uint32 returnCode;
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
		uint32 returnCode;
	};

    struct submitMuslimId_input
    {
    };

    struct submitMuslimId_output
    {
        uint32 returnCode;
    };

    struct cancelMuslimId_input
    {

    };

    struct cancelMuslimId_output
    {
        uint32 returnCode;
    };

protected:

    struct stakingInfo
    {
        id stakerAddress;
        uint32 amount;
    };

    Array<stakingInfo, 1048576> staker;
    Array<stakingInfo, 1048576> votingPower;

    struct GPInfo                   // General proposal
    {
        id proposer;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 proposedEpoch;
        uint32 currentQuorumPercent;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum.
    };

    Array<GPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> GP;

    struct QCPInfo                   // Quorum change proposal
    {
        id proposer;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 proposedEpoch;
        uint32 currentQuorumPercent;
        uint32 newQuorumPercent;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum.
    };

    Array<QCPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> QCP;

    struct IPOPInfo         // IPO participation
    {
        id proposer;
        uint64 totalWeight;
        uint64 assignedFund;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 proposedEpoch;
        uint32 ipoContractIndex;
        uint32 currentQuorumPercent;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum. 3 is the insufficient invest funds.
    };

    Array<IPOPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> IPOP;

    struct QEarnPInfo       // Qearn participation proposal
    {
        id proposer;
        uint64 amountOfInvestPerEpoch;
        uint64 assignedFundPerEpoch;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 proposedEpoch;
        uint32 currentQuorumPercent;
        uint8 numberOfEpoch;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum. 3 is the insufficient funds.
    };

    Array<QEarnPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> QEarnP;

    struct FundPInfo            // Fundraising proposal
    {
        id proposer;
        uint64 pricePerOneQcap;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 amountOfQcap;
        uint32 restSaleAmount;
        uint32 proposedEpoch;
        uint32 currentQuorumPercent;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum.
    };

    Array<FundPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> FundP;

    struct MKTPInfo                 //  Marketplace proposal
    {
        id proposer;
        uint64 amountOfQubic;
        uint64 shareName;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 amountOfQcap;
        uint32 currentQuorumPercent;
        uint32 proposedEpoch;
        uint32 shareIndex;
        uint32 amountOfShare;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum. 3 is the insufficient funds. 4 is the insufficient Qcap.
    };

    Array<MKTPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> MKTP;
    
    struct AlloPInfo
    {
        id proposer;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 proposedEpoch;
        uint32 currentQuorumPercent;
        uint32 reinvested;
        uint32 distributed;
        uint32 team;
        uint32 burnQcap;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum.
    };

    Array<AlloPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> AlloP;

    struct MSPInfo
    {
        id proposer;
        uint32 currentTotalVotingPower;
        uint32 numberOfYes;
        uint32 numberOfNo;
        uint32 proposedEpoch;
        uint32 muslimShareIndex;
        uint32 currentQuorumPercent;
        uint8 result;  // 0 is the passed proposal, 1 is the rejected proposal. 2 is the insufficient quorum.
    };

    Array<MSPInfo, 1024> MSP;

    id QCAP_ISSUER;
    id reinvestingAddress;
    id adminAddress;
    Array<id, 1048576> muslim;
    uint64 proposalCreateFund, reinvestingFund, totalNotMSRevenue, totalMuslimRevenue, fundForBurn, totalHistoryRevenue, rasiedFundByQcap, lastRoundPriceOfQcap, revenueByQearn;
    Array<uint64, 65536> revenueInQcapPerEpoch;
    Array<uint64, 65536> revenueForOneQcapPerEpoch;
    Array<uint64, 65536> revenueForOneMuslimPerEpoch;
    Array<uint64, 65536> revenueForOneQvaultPerEpoch;
    Array<uint64, 65536> revenueForReinvestPerEpoch;
    Array<uint64, 1024> revenuePerShare;
    Array<uint32, 64> muslimShares;
    Array<uint32, 65536> burntQcapAmPerEpoch;
    uint32 totalVotingPower, totalStakedQcapAmount, qcapSoldAmount;
    uint32 shareholderDividend, QCAPHolderPermille, reinvestingPermille, devPermille, burnPermille, qcapBurnPermille, totalQcapBurntAmount;
    uint32 numberOfStaker, numberOfVotingPower;
    uint32 numberOfGP;
    uint32 numberOfQCP;
    uint32 numberOfIPOP;
    uint32 numberOfQEarnP;
    uint32 numberOfFundP;
    uint32 numberOfMKTP;
    uint32 numberOfAlloP;
    uint32 numberOfMSP;
    uint32 transferRightsFee;
    uint32 numberOfMuslimShare;
    sint32 numberOfMuslim;
    uint32 quorumPercent;
    
    /**
	* @return Current date from core node system
	*/

	inline static void getCurrentDate(const QPI::QpiContextFunctionCall& qpi, uint32& res) 
	{
        QUOTTERY::packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), res);
    }

    struct getData_locals
    {
        sint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getData)
    {
        output.adminAddress = state.adminAddress;
        output.quorumPercent = state.quorumPercent;
        output.totalVotingPower = state.totalVotingPower;
        output.proposalCreateFund = state.proposalCreateFund;
        output.reinvestingFund = state.reinvestingFund;
        output.totalNotMSRevenue = state.totalNotMSRevenue;
        output.totalMuslimRevenue = state.totalMuslimRevenue;
        output.shareholderDividend = state.shareholderDividend;
        output.QCAPHolderPermille = state.QCAPHolderPermille;
        output.reinvestingPermille = state.reinvestingPermille;
        output.devPermille = state.devPermille;
        output.burnPermille = state.burnPermille;
        output.qcapBurnPermille = state.qcapBurnPermille;
        output.numberOfStaker = state.numberOfStaker;
        output.numberOfVotingPower = state.numberOfVotingPower;
        output.qcapSoldAmount = state.qcapSoldAmount;
        output.numberOfGP = state.numberOfGP;
        output.numberOfQCP = state.numberOfQCP;
        output.numberOfIPOP = state.numberOfIPOP;
        output.numberOfQEarnP = state.numberOfQEarnP;
        output.numberOfFundP = state.numberOfFundP;
        output.numberOfMKTP = state.numberOfMKTP;
        output.numberOfAlloP = state.numberOfAlloP;
        output.transferRightsFee = state.transferRightsFee;
        output.numberOfMuslim = state.numberOfMuslim;
        output.fundForBurn = state.fundForBurn;
        output.numberOfMuslimShare = state.numberOfMuslimShare;
        output.totalStakedQcapAmount = state.totalStakedQcapAmount;
        output.minQuorumRq = 330;
        output.maxQuorumRq = 670;
        output.totalQcapBurntAmount = state.totalQcapBurntAmount;
        output.raisedFundByQcap = state.rasiedFundByQcap;
        output.circulatingSupply = (uint32)(QVAULT_QCAP_MAX_SUPPLY - state.totalQcapBurntAmount - qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) - qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX) + state.totalStakedQcapAmount);
        for (locals._t = state.numberOfFundP - 1; locals._t >= 0; locals._t--)
        {
            if (state.FundP.get(locals._t).result == 0 && state.FundP.get(locals._t).proposedEpoch + 1 < qpi.epoch())
            {
                output.qcapMarketCap = output.circulatingSupply * state.FundP.get(locals._t).pricePerOneQcap;
                break;
            }
        }
        output.lastRoundPriceOfQcap = state.lastRoundPriceOfQcap;
        output.revenueByQearn = state.revenueByQearn;
    }

    struct stake_locals
    {
        stakingInfo user;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(stake)
    {
        if (input.amount > (uint32)qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX))
        {
            output.returnCode = QVAULT_INSUFFICIENT_QCAP;
            return ;
        }

        qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, qpi.invocator(), qpi.invocator(), input.amount, SELF);

        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            if (state.staker.get(locals._t).stakerAddress == qpi.invocator())
            {
                break;
            }
        }

        if (locals._t == state.numberOfStaker)
        {
            locals.user.amount = input.amount;
            locals.user.stakerAddress = qpi.invocator();
            state.numberOfStaker++;
        }
        else 
        {
            locals.user.amount = state.staker.get(locals._t).amount + input.amount;
            locals.user.stakerAddress = state.staker.get(locals._t).stakerAddress;
        }

        state.totalStakedQcapAmount += input.amount;
        state.staker.set(locals._t, locals.user);
        output.returnCode = QVAULT_SUCCESS;
    }

    struct unStake_locals
    {
        stakingInfo user;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(unStake)
    {
        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            if (state.staker.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.staker.get(locals._t).amount < input.amount)
                {
                    output.returnCode = QVAULT_NOT_ENOUGH_STAKE;
                }
                else if (state.staker.get(locals._t).amount >= input.amount)
                {
                    locals.user.stakerAddress = state.staker.get(locals._t).stakerAddress;
                    locals.user.amount = state.staker.get(locals._t).amount - input.amount;
                    state.staker.set(locals._t, locals.user);
                    qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, input.amount, qpi.invocator());

                    state.totalStakedQcapAmount -= input.amount;
                    output.returnCode = QVAULT_SUCCESS;
                    if (locals.user.amount == 0)
                    {
                        state.numberOfStaker--;
                        for (; locals._t < (sint32)state.numberOfStaker; locals._t++)
                        {
                            if (locals._t == QVAULT_QCAP_MAX_SUPPLY)
                            {
                                return ;
                            }
                            state.staker.set(locals._t, state.staker.get(locals._t + 1));
                        }
                    }
                }
                return ;
            }
        }

        output.returnCode = QVAULT_NOT_STAKER;
    }

    struct submitGP_locals
    {
        GPInfo newProposal;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitGP)
    {
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }
        
        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;

        state.GP.set(state.numberOfGP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitQCP_locals
    {
        QCPInfo newProposal;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitQCP)
    {
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;
        
        locals.newProposal.newQuorumPercent = input.newQuorumPercent;

        state.QCP.set(state.numberOfQCP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitIPOP_locals
    {
        IPOPInfo newProposal;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitIPOP)
    {
        if (state.reinvestingFund < QVAULT_IPO_PARTICIPATION_MIN_FUND)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;
        locals.newProposal.assignedFund = 0;
        
        locals.newProposal.ipoContractIndex = input.ipoContractIndex;
        locals.newProposal.totalWeight = 0;

        state.IPOP.set(state.numberOfIPOP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitQEarnP_locals
    {
        QEarnPInfo newProposal;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitQEarnP)
    {
        if (input.numberOfEpoch > 52)
        {
            output.returnCode = QVAULTLogInfo::QvaultInputError;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        if (input.amountPerEpoch * input.numberOfEpoch > state.reinvestingFund)
        {
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;
        
        locals.newProposal.assignedFundPerEpoch = input.amountPerEpoch;
        locals.newProposal.amountOfInvestPerEpoch = input.amountPerEpoch;
        locals.newProposal.numberOfEpoch = input.numberOfEpoch;

        state.QEarnP.set(state.numberOfQEarnP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitFundP_locals
    {
        FundPInfo newProposal;
        sint32 _t;
        uint32 curDate;
        uint8 year;
        uint8 month;
        uint8 day;
        uint8 hour;
        uint8 minute;
        uint8 second;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitFundP)
    {
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }

        getCurrentDate(qpi, locals.curDate);
        QUOTTERY::unpackQuotteryDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
        if (locals.year == 25 && state.qcapSoldAmount + input.amountOfQcap > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 26 && state.qcapSoldAmount + input.amountOfQcap > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 27 && state.qcapSoldAmount + input.amountOfQcap > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (state.qcapSoldAmount + input.amountOfQcap > QVAULT_QCAP_MAX_SUPPLY)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;
        
        locals.newProposal.restSaleAmount = input.amountOfQcap;
        locals.newProposal.amountOfQcap = input.amountOfQcap;
        locals.newProposal.pricePerOneQcap = input.priceOfOneQcap;

        state.FundP.set(state.numberOfFundP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitMKTP_locals
    {
        MKTPInfo newProposal;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitMKTP)
    {
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.transferShareOwnershipAndPossession(input.shareName, NULL_ID, qpi.invocator(), qpi.invocator(), input.amountOfShare, SELF) < 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNotTransferredShare;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;
        
        locals.newProposal.shareIndex = input.indexOfShare;
        locals.newProposal.amountOfShare = input.amountOfShare;
        locals.newProposal.amountOfQubic = input.amountOfQubic;
        locals.newProposal.amountOfQcap = input.amountOfQcap;
        locals.newProposal.shareName = input.shareName;

        state.MKTP.set(state.numberOfMKTP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitAlloP_locals
    {
        AlloPInfo newProposal;
        sint32 _t;
        uint32 curDate;
        uint8 year;
        uint8 month;
        uint8 day;
        uint8 hour;
        uint8 minute;
        uint8 second;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitAlloP)
    {
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }

        getCurrentDate(qpi, locals.curDate);
        QUOTTERY::unpackQuotteryDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
        if (locals.year < 29 && input.burn != 0)
        {
            output.returnCode = QVAULTLogInfo::QvaultNotInTime;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        if (locals.year >= 27 && input.team != 0)
        {
            output.returnCode = QVAULTLogInfo::QvaultNotInTime;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        if (input.burn + input.distribute + input.reinvested + input.team != 970)
        {
            output.returnCode = QVAULTLogInfo::QvaultNotFair;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;
        
        locals.newProposal.burnQcap = input.burn;
        locals.newProposal.distributed = input.distribute;
        locals.newProposal.reinvested = input.reinvested;
        locals.newProposal.team = input.team;

        state.AlloP.set(state.numberOfAlloP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }

    struct submitMSP_locals
    {
        MSPInfo newProposal;
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(submitMSP)
    {
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    break;
                }
                else 
                {
                    output.returnCode = QVAULTLogInfo::QvaultInsufficientVotingPower;
                    if (qpi.invocationReward() > 0)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    }
                    return ;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
            return ;
        }
        
        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = 0;

        locals.newProposal.muslimShareIndex = input.shareIndex;

        state.MSP.set(state.numberOfMSP++, locals.newProposal);
        output.returnCode = QVAULTLogInfo::QvaultSuccess;
    }
    
    struct voteInProposal_locals
    {
        GPInfo updatedGProposal;
        QCPInfo updatedQCProposal;
        IPOPInfo updatedIPOProposal;
        QEarnPInfo updatedQEarnProposal;
        FundPInfo updatedFundProposal;
        MKTPInfo updatedMKTProposal;
        AlloPInfo updatedAlloProposal;
        MSPInfo updatedMSProposal;
        uint32 numberOfYes;
        uint32 numberOfNo;
        sint32 _t;
        bit statusOfProposal;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(voteInProposal)
    {
        if (input.proposalId > QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowProposal;
            return ;
        }
        switch (input.proposalType)
        {
            case 1:
                locals.updatedGProposal = state.GP.get(input.proposalId);
                locals.statusOfProposal = state.GP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 2:
                locals.updatedQCProposal = state.QCP.get(input.proposalId);
                locals.statusOfProposal = state.QCP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 3:
                locals.updatedIPOProposal = state.IPOP.get(input.proposalId);
                locals.statusOfProposal = state.IPOP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 4:
                locals.updatedQEarnProposal = state.QEarnP.get(input.proposalId);
                locals.statusOfProposal = state.QEarnP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 5:
                locals.updatedFundProposal = state.FundP.get(input.proposalId);
                locals.statusOfProposal = state.FundP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 6:
                locals.updatedMKTProposal = state.MKTP.get(input.proposalId);
                locals.statusOfProposal = state.MKTP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 7:
                locals.updatedAlloProposal = state.AlloP.get(input.proposalId);
                locals.statusOfProposal = state.AlloP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 8:
                locals.updatedMSProposal = state.MSP.get(input.proposalId);
                locals.statusOfProposal = state.MSP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
        }

        if (locals.statusOfProposal == 0)
        {
            output.returnCode = QVAULTLogInfo::QvaultEndedProposal;
            return ;
        }

        if (locals.statusOfProposal == 1)
        {
            locals.numberOfYes = 0;
            locals.numberOfNo = 0;
            for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t)
            {
                if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
                {
                    if (input.yes == 1)
                    {
                        locals.numberOfYes = state.votingPower.get(locals._t).amount;
                    }
                    else 
                    {
                        locals.numberOfNo = state.votingPower.get(locals._t).amount;
                    }
                    switch (input.proposalType)
                    {
                        case 1:
                            locals.updatedGProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedGProposal.numberOfNo += locals.numberOfNo;
                            state.GP.set(input.proposalId, locals.updatedGProposal);
                            break;
                        case 2:
                            locals.updatedQCProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedQCProposal.numberOfNo += locals.numberOfNo;
                            state.QCP.set(input.proposalId, locals.updatedQCProposal);
                            break;
                        case 3:
                            locals.updatedIPOProposal.totalWeight = locals.numberOfYes * input.priceOfIPO;
                            locals.updatedIPOProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedIPOProposal.numberOfNo += locals.numberOfNo;
                            state.IPOP.set(input.proposalId, locals.updatedIPOProposal);
                            break;
                        case 4:
                            locals.updatedQEarnProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedQEarnProposal.numberOfNo += locals.numberOfNo;
                            state.QEarnP.set(input.proposalId, locals.updatedQEarnProposal);
                            break;
                        case 5:
                            locals.updatedFundProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedFundProposal.numberOfNo += locals.numberOfNo;
                            state.FundP.set(input.proposalId, locals.updatedFundProposal);
                            break;
                        case 6:
                            locals.updatedMKTProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedMKTProposal.numberOfNo += locals.numberOfNo;
                            state.MKTP.set(input.proposalId, locals.updatedMKTProposal);
                            break;
                        case 7:
                            locals.updatedAlloProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedAlloProposal.numberOfNo += locals.numberOfNo;
                            state.AlloP.set(input.proposalId, locals.updatedAlloProposal);
                            break;
                        case 8:
                            locals.updatedMSProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedMSProposal.numberOfNo += locals.numberOfNo;
                            state.MSP.set(input.proposalId, locals.updatedMSProposal);
                            break;
                    }
                    output.returnCode = QVAULTLogInfo::QvaultSuccess;
                    return ;
                }
            }
        }

        output.returnCode = QVAULTLogInfo::QvaultNoVotingPower;
    }

    struct buyQcap_locals
    {
        QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
        FundPInfo updatedFundProposal;
        sint32 _t;
        uint32 curDate;
        uint8 year;
        uint8 month;
        uint8 day;
        uint8 hour;
        uint8 minute;
        uint8 second;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(buyQcap)
    {
        getCurrentDate(qpi, locals.curDate);
        QUOTTERY::unpackQuotteryDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
        if (locals.year == 25 && state.qcapSoldAmount + input.amount > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 26 && state.qcapSoldAmount + input.amount > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 27 && state.qcapSoldAmount + input.amount > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (state.qcapSoldAmount + input.amount > QVAULT_QCAP_MAX_SUPPLY)
        {
            output.returnCode = QVAULTLogInfo::QvaultOverflowSaleAmount;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        for (locals._t = state.numberOfFundP - 1; locals._t >= 0; locals._t--)
        {
            if (state.FundP.get(locals._t).result == 0 && state.FundP.get(locals._t).proposedEpoch + 1 < qpi.epoch() && state.FundP.get(locals._t).restSaleAmount > input.amount)
            {
                if (qpi.invocationReward() >= (sint64)state.FundP.get(locals._t).pricePerOneQcap * input.amount)
                {
                    if (qpi.invocationReward() > (sint64)state.FundP.get(locals._t).pricePerOneQcap * input.amount)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward() - (state.FundP.get(locals._t).pricePerOneQcap * input.amount));
                    }

                    state.rasiedFundByQcap += state.FundP.get(locals._t).pricePerOneQcap * input.amount;
                    locals.transferShareManagementRights_input.asset.assetName = QVAULT_QCAP_ASSETNAME;
                    locals.transferShareManagementRights_input.asset.issuer = state.QCAP_ISSUER;
                    locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
                    locals.transferShareManagementRights_input.numberOfShares = input.amount;

                    INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

                    qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, input.amount, qpi.invocator());

                    state.qcapSoldAmount += input.amount;
                    locals.updatedFundProposal = state.FundP.get(locals._t);
                    locals.updatedFundProposal.restSaleAmount -= input.amount;
                    state.FundP.set(locals._t, locals.updatedFundProposal);

                    state.reinvestingFund += state.FundP.get(locals._t).pricePerOneQcap * input.amount;
                    output.returnCode = QVAULTLogInfo::QvaultSuccess;
                    return ;
                }
            }
        }

        qpi.transfer(qpi.invocator(), qpi.invocationReward());

        output.returnCode = QVAULTLogInfo::QvaultFailed;
    }

	PUBLIC_PROCEDURE(TransferShareManagementRights)
    {
		if (qpi.invocationReward() < state.transferRightsFee)
		{
			output.returnCode = QVAULTLogInfo::QvaultInsufficientFund;
			return ;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = QVAULTLogInfo::QvaultInsufficientShare;
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

				output.returnCode = QVAULTLogInfo::QvaultErrorTransferAsset;
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
				qpi.transfer(id(QX_CONTRACT_INDEX, 0, 0, 0), state.transferRightsFee);
				if (qpi.invocationReward() > state.transferRightsFee)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.transferRightsFee);
				}

				output.returnCode = QVAULTLogInfo::QvaultSuccess;
			}
		}
    }

    struct submitMuslimId_locals
    {
        sint32 _t;
    };

	PUBLIC_PROCEDURE_WITH_LOCALS(submitMuslimId)
    {
        if (state.numberOfMuslim == 1048576)
        {
            output.returnCode = QVAULTLogInfo::QvaultMaxMuslimId;
            return ;
        }
        for (locals._t = 0 ; locals._t < state.numberOfMuslim; locals._t++)
        {
            if (state.muslim.get(locals._t) == qpi.invocator())
            {
                output.returnCode = QVAULTLogInfo::QvaultDuplicatedMuslimId;
                return ;
            }
        }
        state.muslim.set(state.numberOfMuslim++, qpi.invocator());
    }

    struct cancelMuslimId_locals
    {
        sint32 _t;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(cancelMuslimId)
    {
        for (locals._t = 0 ; locals._t < state.numberOfMuslim; locals._t++)
        {
            if (state.muslim.get(locals._t) == qpi.invocator())
            {
                state.numberOfMuslim--;
                for (; locals._t < state.numberOfMuslim; locals._t++)
                {
                    state.muslim.set(locals._t, state.muslim.get(locals._t + 1));
                }
                output.returnCode = QVAULTLogInfo::QvaultSuccess;
                return ;
            }
        }

        output.returnCode = QVAULTLogInfo::QvaultNotMuslimId;
    }

    struct getStakedAmountAndVotingPower_input
    {
        id address;
    };

    struct getStakedAmountAndVotingPower_output
    {
        uint32 stakedAmount;
        uint32 votingPower;
    };

    struct getStakedAmountAndVotingPower_locals
    {
        sint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getStakedAmountAndVotingPower)
    {
        for (locals._t = 0; locals._t < (sint64)state.numberOfStaker; locals._t++)
        {
            if (state.staker.get(locals._t).stakerAddress == input.address)
            {
                output.stakedAmount = state.staker.get(locals._t).amount;
                break;
            }
        }
        for (locals._t = 0; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == input.address)
            {
                output.votingPower = state.votingPower.get(locals._t).amount;
                break;
            }
        }
    }

    struct getGP_input
    {
        uint32 proposalId;
    };

    struct getGP_output
    {
        GPInfo proposal;
    };

    PUBLIC_FUNCTION(getGP)
    {
        output.proposal = state.GP.get(input.proposalId);
    }

    struct getQCP_input
    {
        uint32 proposalId;
    };

    struct getQCP_output
    {
        QCPInfo proposal;
    };

    PUBLIC_FUNCTION(getQCP)
    {
        output.proposal = state.QCP.get(input.proposalId);
    }

    struct getIPOP_input
    {
        uint32 proposalId;
    };

    struct getIPOP_output
    {
        IPOPInfo proposal;
    };

    PUBLIC_FUNCTION(getIPOP)
    {
        output.proposal = state.IPOP.get(input.proposalId);
    }

    struct getQEarnP_input
    {
        uint32 proposalId;
    };

    struct getQEarnP_output
    {
        QEarnPInfo proposal;
    };

    PUBLIC_FUNCTION(getQEarnP)
    {
        output.proposal = state.QEarnP.get(input.proposalId);
    }

    struct getFundP_input
    {
        uint32 proposalId;
    };

    struct getFundP_output
    {
        FundPInfo proposal;
    };

    PUBLIC_FUNCTION(getFundP)
    {
        output.proposal = state.FundP.get(input.proposalId);
    }

    struct getMKTP_input
    {
        uint32 proposalId;
    };

    struct getMKTP_output
    {
        MKTPInfo proposal;
    };

    PUBLIC_FUNCTION(getMKTP)
    {
        output.proposal = state.MKTP.get(input.proposalId);
    }

    struct getAlloP_input
    {
        uint32 proposalId;
    };

    struct getAlloP_output
    {
        AlloPInfo proposal;
    };

    PUBLIC_FUNCTION(getAlloP)
    {
        output.proposal = state.AlloP.get(input.proposalId);
    }

    struct getMSP_input
    {
        uint32 proposalId;
    };

    struct getMSP_output
    {
        MSPInfo proposal;
    };

    PUBLIC_FUNCTION(getMSP)
    {
        output.proposal = state.MSP.get(input.proposalId);
    }

    struct getIdentitiesHvVtPw_input
    {
        uint32 offset;
        uint32 count;
    };

    struct HvVtPwListInfo
    {
        id address;
        uint32 power;
    };

    struct getIdentitiesHvVtPw_output
    {
        Array<HvVtPwListInfo, 256> list;
    };

    struct getIdentitiesHvVtPw_locals
    {
        HvVtPwListInfo tmpList;
        sint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getIdentitiesHvVtPw)
    {
        for (locals._t = (sint32)input.offset; locals._t < (sint32)(input.offset + input.count); locals._t++)
        {
            if (locals._t - input.offset > 256)
            {
                return ;
            }
            locals.tmpList.address = state.votingPower.get(locals._t).stakerAddress;
            locals.tmpList.power = state.votingPower.get(locals._t).amount;
        }
    }

    struct ppCreationPower_input
    {
        id address;
    };

    struct ppCreationPower_output
    {
        bit status;         // 0 means that there is no the proposal creation power, 1 means that there is.
    };

    struct ppCreationPower_locals
    {
        sint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(ppCreationPower)
    {
        for (locals._t = 0; locals._t < (sint32)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == input.address)
            {
                if (state.votingPower.get(locals._t).amount >= 10000)
                {
                    output.status = 1;
                }
                else 
                {
                    output.status = 0;
                }
                return ;
            }
        }
        if (qpi.numberOfPossessedShares(QVAULT_QVAULT_ASSETNAME, NULL_ID, input.address, input.address, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) > 0)
        {
            output.status = 1;
        }
        else 
        {
            output.status = 0;
        }
    }

    struct getQcapBurntAmountInLastEpoches_input
    {
        uint32 numberOfLastEpoches;
    };

    struct getQcapBurntAmountInLastEpoches_output
    {
        uint32 burntAmount;
    };


    struct getQcapBurntAmountInLastEpoches_locals
    {
        sint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getQcapBurntAmountInLastEpoches)
    {
        if (input.numberOfLastEpoches > 100)
        {
            return ;
        }
        for (locals._t = (sint32)qpi.epoch(); locals._t >= (sint32)(qpi.epoch() - input.numberOfLastEpoches); locals._t--)
        {
            output.burntAmount += state.burntQcapAmPerEpoch.get(locals._t);
        }
    }

    struct getAmountToBeSoldPerYear_input
    {
        uint32 year;
    };

    struct getAmountToBeSoldPerYear_output
    {
        uint32 amount;
    };

    PUBLIC_FUNCTION(getAmountToBeSoldPerYear)
    {
        if (input.year < 2025)
        {
            output.amount = 0;
        }
        else if (input.year == 2025)
        {
            output.amount = QVAULT_2025MAX_QCAP_SALE_AMOUNT - state.qcapSoldAmount;
        }
        else if (input.year == 2026)
        {
            output.amount = QVAULT_2026MAX_QCAP_SALE_AMOUNT - state.qcapSoldAmount;
        }
        else if (input.year == 2027)
        {
            output.amount = QVAULT_2027MAX_QCAP_SALE_AMOUNT - state.qcapSoldAmount;
        }
        else if (input.year > 2027)
        {
            output.amount = QVAULT_QCAP_MAX_SUPPLY - state.qcapSoldAmount;
        }
    }

    struct getTotalRevenueInQcap_input
    {

    };

    struct getTotalRevenueInQcap_output
    {
        uint64 revenue;
    };

    PUBLIC_FUNCTION(getTotalRevenueInQcap)
    {
        output.revenue = state.totalHistoryRevenue;
    }

    struct getRevenueInQcapPerEpoch_input
    {
        uint32 epoch;
    };

    struct getRevenueInQcapPerEpoch_output
    {
        uint64 epochTotalRevenue;
        uint64 epochOneQcapRevenue;
        uint64 epochOneMuslimRevenue;
        uint64 epochOneQvaultRevenue;
        uint64 epochReinvestAmount;
    };

    PUBLIC_FUNCTION(getRevenueInQcapPerEpoch)
    {
        output.epochTotalRevenue = state.revenueInQcapPerEpoch.get(input.epoch);
        output.epochOneQcapRevenue = state.revenueForOneQcapPerEpoch.get(input.epoch);
        output.epochOneMuslimRevenue = state.revenueForOneMuslimPerEpoch.get(input.epoch);
        output.epochOneQvaultRevenue = state.revenueForOneQvaultPerEpoch.get(input.epoch);
        output.epochReinvestAmount = state.revenueForReinvestPerEpoch.get(input.epoch);
    }

    struct getRevenuePerShare_input
    {
        uint32 contractIndex;
    };

    struct getRevenuePerShare_output
    {
        uint64 revenue;
    };

    PUBLIC_FUNCTION(getRevenuePerShare)
    {
        output.revenue = state.revenuePerShare.get(input.contractIndex);
    }

    struct getAmountOfShareQvaultHold_input
    {
        Asset assetInfo;
    };

    struct getAmountOfShareQvaultHold_output
    {
        uint32 amount;
    };

    PUBLIC_FUNCTION(getAmountOfShareQvaultHold)
    {
        output.amount = (uint32)qpi.numberOfPossessedShares(input.assetInfo.assetName, input.assetInfo.issuer, SELF, SELF, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    }

    struct getNumberOfHolderAndAvgAm_input
    {

    };

    struct getNumberOfHolderAndAvgAm_output
    {
        uint32 numberOfQcapHolder;
        uint32 avgAmount;
    };

    struct getNumberOfHolderAndAvgAm_locals
    {
        AssetPossessionIterator iter;
        Asset QCAPId;
        uint32 count;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getNumberOfHolderAndAvgAm)
    {
        locals.QCAPId.assetName = QVAULT_QCAP_ASSETNAME;
        locals.QCAPId.issuer = state.QCAP_ISSUER;

        locals.iter.begin(locals.QCAPId);
        while (!locals.iter.reachedEnd())
        {
            locals.count++;

            locals.iter.next();
        }

        output.numberOfQcapHolder = locals.count;
        output.avgAmount = (uint32)div(QVAULT_QCAP_MAX_SUPPLY - state.totalQcapBurntAmount - qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) - qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX) * 1ULL, output.numberOfQcapHolder * 1ULL);
    }

    struct getAmountForQearnInUpcomingEpoch_input
    {
        uint32 epoch;
    };

    struct getAmountForQearnInUpcomingEpoch_output
    {
        uint64 amount;
    };

    struct getAmountForQearnInUpcomingEpoch_locals
    {
        sint32 _t;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getAmountForQearnInUpcomingEpoch)
    {
        if (input.epoch <= qpi.epoch())
        {
            return ;
        }
        for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
        {
            if (state.QEarnP.get(locals._t).proposedEpoch + 1 < qpi.epoch() && state.QEarnP.get(locals._t).result == 0 && state.QEarnP.get(locals._t).numberOfEpoch + state.QEarnP.get(locals._t).proposedEpoch + 1 >= qpi.epoch())
            {
                output.amount += state.QEarnP.get(locals._t).assignedFundPerEpoch;
            }
        }
    }

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getData, 1);
        REGISTER_USER_FUNCTION(getStakedAmountAndVotingPower, 2);
        REGISTER_USER_FUNCTION(getGP, 3);
        REGISTER_USER_FUNCTION(getQCP, 4);
        REGISTER_USER_FUNCTION(getIPOP, 5);
        REGISTER_USER_FUNCTION(getQEarnP, 6);
        REGISTER_USER_FUNCTION(getFundP, 7);
        REGISTER_USER_FUNCTION(getMKTP, 8);
        REGISTER_USER_FUNCTION(getAlloP, 9);
        REGISTER_USER_FUNCTION(getMSP, 10);
        REGISTER_USER_FUNCTION(getIdentitiesHvVtPw, 11);
        REGISTER_USER_FUNCTION(ppCreationPower, 12);
        REGISTER_USER_FUNCTION(getQcapBurntAmountInLastEpoches, 13);
        REGISTER_USER_FUNCTION(getAmountToBeSoldPerYear, 14);
        REGISTER_USER_FUNCTION(getTotalRevenueInQcap, 15);
        REGISTER_USER_FUNCTION(getRevenueInQcapPerEpoch, 16);
        REGISTER_USER_FUNCTION(getRevenuePerShare, 17);
        REGISTER_USER_FUNCTION(getAmountOfShareQvaultHold, 18);
        REGISTER_USER_FUNCTION(getNumberOfHolderAndAvgAm, 19);
        REGISTER_USER_FUNCTION(getAmountForQearnInUpcomingEpoch, 20);

        REGISTER_USER_PROCEDURE(stake, 1);
        REGISTER_USER_PROCEDURE(unStake, 2);
        REGISTER_USER_PROCEDURE(submitGP, 3);
        REGISTER_USER_PROCEDURE(submitQCP, 4);
        REGISTER_USER_PROCEDURE(submitIPOP, 5);
        REGISTER_USER_PROCEDURE(submitQEarnP, 6);
        REGISTER_USER_PROCEDURE(submitFundP, 7);
        REGISTER_USER_PROCEDURE(submitMKTP, 8);
        REGISTER_USER_PROCEDURE(submitAlloP, 9);
        REGISTER_USER_PROCEDURE(submitMSP, 10);
        REGISTER_USER_PROCEDURE(voteInProposal, 11);
        REGISTER_USER_PROCEDURE(buyQcap, 12);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 13);
        REGISTER_USER_PROCEDURE(submitMuslimId, 14);
        REGISTER_USER_PROCEDURE(cancelMuslimId, 15);

    }

	INITIALIZE()
    {
        state.QCAP_ISSUER = ID(_Q, _C, _A, _P, _W, _M, _Y, _R, _S, _H, _L, _B, _J, _H, _S, _T, _T, _Z, _Q, _V, _C, _I, _B, _A, _R, _V, _O, _A, _S, _K, _D, _E, _N, _A, _S, _A, _K, _N, _O, _B, _R, _G, _P, _F, _W, _W, _K, _R, _C, _U, _V, _U, _A, _X, _Y, _E);
        state.adminAddress = ID(_H, _E, _C, _G, _U, _G, _H, _C, _J, _K, _Q, _O, _S, _D, _T, _M, _E, _H, _Q, _Y, _W, _D, _D, _T, _L, _F, _D, _A, _S, _Z, _K, _M, _G, _J, _L, _S, _R, _C, _S, _T, _H, _H, _A, _P, _P, _E, _D, _L, _G, _B, _L, _X, _J, _M, _N, _D);
        state.qcapSoldAmount = 1652235;
        state.transferRightsFee = 1000000;
        state.quorumPercent = 670;
        state.devPermille = 20;
        state.qcapBurnPermille = 0;
        state.burnPermille = 0;
        state.QCAPHolderPermille = 500;
        state.reinvestingPermille = 450;
        state.shareholderDividend = 30;

    }

    struct BEGIN_EPOCH_locals
    {
        QEARN::lock_input lock_input;
        QEARN::lock_output lock_output;
        QX::AssetAskOrders_input assetAskOrders_input;
        QX::AssetAskOrders_output assetAskOrders_output;
        QX::AddToBidOrder_input addToBidOrder_input;
        QX::AddToBidOrder_output addToBidOrder_output;
        QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
        uint32 purchasedQcap;
        sint32 _t;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        for (locals._t = 0 ; locals._t < (sint32)state.numberOfQEarnP; locals._t++)
        {
            if (state.QEarnP.get(locals._t).result == 0 && state.QEarnP.get(locals._t).proposedEpoch + state.QEarnP.get(locals._t).numberOfEpoch + 1 >= qpi.epoch())
            {
                if (state.QEarnP.get(locals._t).proposedEpoch + 1 == qpi.epoch())
                {
                    continue;
                }
                INVOKE_OTHER_CONTRACT_PROCEDURE(QEARN, lock, locals.lock_input, locals.lock_output, state.QEarnP.get(locals._t).assignedFundPerEpoch);
            }
        }

        for (locals._t = state.numberOfQCP - 1 ; locals._t >= 0; locals._t--)
        {
            if (state.QCP.get(locals._t).result == 0 && state.QCP.get(locals._t).proposedEpoch + 2 == qpi.epoch())
            {
                state.quorumPercent = state.QCP.get(locals._t).newQuorumPercent;
                break;
            }
            if (state.QCP.get(locals._t).proposedEpoch + 2 < qpi.epoch())
            {
                break;
            }
        }

        for (locals._t = state.numberOfAlloP - 1; locals._t >= 0; locals._t--)
        {
            if (state.AlloP.get(locals._t).result == 0 && state.AlloP.get(locals._t).proposedEpoch + 2 == qpi.epoch())
            {
                state.QCAPHolderPermille = state.AlloP.get(locals._t).distributed;
                state.reinvestingPermille = state.AlloP.get(locals._t).reinvested;
                state.qcapBurnPermille = state.AlloP.get(locals._t).burnQcap;
                state.devPermille = state.AlloP.get(locals._t).team;
                break;
            }
            if (state.AlloP.get(locals._t).proposedEpoch + 2 < qpi.epoch())
            {
                break;
            }
        }

        for (locals._t = state.numberOfMSP - 1; locals._t >= 0; locals._t--)
        {
            if (state.MSP.get(locals._t).result == 0 && state.MSP.get(locals._t).proposedEpoch + 2 == qpi.epoch())
            {
                state.muslimShares.set(state.numberOfMuslimShare++, state.MSP.get(locals._t).muslimShareIndex);
            }
            if (state.MSP.get(locals._t).proposedEpoch + 2 < qpi.epoch())
            {
                break;
            }
        }

        locals.purchasedQcap = 0;
        while(state.fundForBurn > 0)
        {
            locals.assetAskOrders_input.assetName = QVAULT_QCAP_ASSETNAME;
            locals.assetAskOrders_input.issuer = state.QCAP_ISSUER;
            locals.assetAskOrders_input.offset = 0;
            CALL_OTHER_CONTRACT_FUNCTION(QX, AssetAskOrders, locals.assetAskOrders_input, locals.assetAskOrders_output);
            if (locals.assetAskOrders_output.orders.get(0).price <= (sint64)state.fundForBurn && locals.assetAskOrders_output.orders.get(0).price != 0)
            {
                locals.addToBidOrder_input.assetName = QVAULT_QCAP_ASSETNAME;
                locals.addToBidOrder_input.issuer = state.QCAP_ISSUER;
                locals.addToBidOrder_input.numberOfShares = (sint64)div(state.fundForBurn, locals.assetAskOrders_output.orders.get(0).price * 1ULL) > locals.assetAskOrders_output.orders.get(0).numberOfShares ? locals.assetAskOrders_output.orders.get(0).numberOfShares : div(state.fundForBurn, locals.assetAskOrders_output.orders.get(0).price * 1ULL);
                locals.addToBidOrder_input.price = locals.assetAskOrders_output.orders.get(0).price;

                INVOKE_OTHER_CONTRACT_PROCEDURE(QX, AddToBidOrder, locals.addToBidOrder_input, locals.addToBidOrder_output, locals.addToBidOrder_input.price * locals.addToBidOrder_input.numberOfShares);

                state.fundForBurn -= locals.addToBidOrder_input.price * locals.addToBidOrder_input.numberOfShares;
                state.burntQcapAmPerEpoch.set(qpi.epoch(), state.burntQcapAmPerEpoch.get(qpi.epoch()) + (uint32)locals.addToBidOrder_input.numberOfShares);
                state.totalQcapBurntAmount += (uint32)locals.addToBidOrder_input.numberOfShares;

                locals.purchasedQcap += (uint32)locals.addToBidOrder_output.addedNumberOfShares;
            }
            else 
            {
                break;
            }
        }

        locals.transferShareManagementRights_input.asset.assetName = QVAULT_QCAP_ASSETNAME;
        locals.transferShareManagementRights_input.asset.issuer = state.QCAP_ISSUER;
        locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
        locals.transferShareManagementRights_input.numberOfShares = locals.purchasedQcap;

        INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

        qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, locals.purchasedQcap, NULL_ID);
    }

    struct END_EPOCH_locals 
    {
        QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
        GPInfo updatedGProposal;
        QCPInfo updatedQCProposal;
        IPOPInfo updatedIPOProposal;
        QEarnPInfo updatedQEarnProposal;
        FundPInfo updatedFundProposal;
        MKTPInfo updatedMKTProposal;
        AlloPInfo updatedAlloProposal;
        MSPInfo updatedMSProposal;
        QPI::Entity entity;
        AssetPossessionIterator iter;
        Asset QCAPId;
        id possessorPubkey;
        uint64 paymentForShareholders;
        uint64 paymentForQCAPHolders;
        uint64 paymentForReinvest;
        uint64 paymentForDevelopment;
        uint64 muslimRevenue;
        uint64 amountOfBurn;
        uint64 circulatedSupply;
        uint64 requiredFund;
        uint64 tmpAmount;
        uint32 amountOfQcapMuslimHold;
        sint32 _t, _r;
        uint32 curDate;
        uint8 year;
        uint8 month;
        uint8 day;
        uint8 hour;
        uint8 minute;
        uint8 second;
    };

    END_EPOCH_WITH_LOCALS()
    {
        locals.paymentForShareholders = div((state.totalNotMSRevenue + state.totalMuslimRevenue) * state.shareholderDividend, 1000ULL);
        locals.paymentForQCAPHolders = div(state.totalNotMSRevenue * state.QCAPHolderPermille, 1000ULL);
        locals.muslimRevenue = div(state.totalMuslimRevenue * state.QCAPHolderPermille, 1000ULL);
        state.reinvestingFund += div((state.totalNotMSRevenue + state.totalMuslimRevenue) * state.reinvestingPermille, 1000ULL);
        state.fundForBurn += div((state.totalNotMSRevenue + state.totalMuslimRevenue) * state.qcapBurnPermille, 1000ULL);
        locals.amountOfBurn = div((state.totalNotMSRevenue + state.totalMuslimRevenue) * state.burnPermille, 1000ULL);
        locals.paymentForDevelopment = state.totalNotMSRevenue + state.totalMuslimRevenue - locals.paymentForShareholders - locals.paymentForQCAPHolders - locals.muslimRevenue - locals.paymentForReinvest - locals.amountOfBurn;

        state.totalNotMSRevenue = 0;
        state.totalMuslimRevenue = 0;
        qpi.distributeDividends(div(locals.paymentForShareholders + state.proposalCreateFund, 676ULL));
        state.proposalCreateFund = 0;
        qpi.transfer(state.adminAddress, locals.paymentForDevelopment);
        qpi.burn(locals.amountOfBurn);

        locals.circulatedSupply = QVAULT_QCAP_MAX_SUPPLY - state.totalQcapBurntAmount - qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) - qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX) + state.totalStakedQcapAmount;

        for (locals._t = 0 ; locals._t < state.numberOfMuslim; locals._t++)
        {
            locals.amountOfQcapMuslimHold += (uint32)qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, state.muslim.get(locals._t), state.muslim.get(locals._t), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) + (uint32)qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, state.muslim.get(locals._t), state.muslim.get(locals._t), QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX);
            for (locals._r = 0 ; locals._r < (sint32)state.numberOfStaker; locals._r++)
            {
                if (state.muslim.get(locals._t) == state.staker.get(locals._r).stakerAddress)
                {
                    locals.amountOfQcapMuslimHold += state.staker.get(locals._r).amount;
                    break;
                }
            }
        }

        state.revenueForOneQcapPerEpoch.set(qpi.epoch(), div(locals.paymentForQCAPHolders, locals.circulatedSupply * 1ULL) + div(locals.muslimRevenue, locals.circulatedSupply - locals.amountOfQcapMuslimHold));
        state.revenueForOneMuslimPerEpoch.set(qpi.epoch(), div(locals.paymentForQCAPHolders, locals.circulatedSupply * 1ULL));
        state.revenueForOneQvaultPerEpoch.set(qpi.epoch(), div(locals.paymentForShareholders + state.proposalCreateFund, 676ULL));
        state.revenueForReinvestPerEpoch.set(qpi.epoch(), div((state.totalNotMSRevenue + state.totalMuslimRevenue) * state.reinvestingPermille, 1000ULL));

        locals.QCAPId.assetName = QVAULT_QCAP_ASSETNAME;
        locals.QCAPId.issuer = state.QCAP_ISSUER;

        locals.iter.begin(locals.QCAPId);
        while (!locals.iter.reachedEnd())
        {
            locals.possessorPubkey = locals.iter.possessor();

            if (locals.possessorPubkey != SELF)
            {
                qpi.transfer(locals.possessorPubkey, div(locals.paymentForQCAPHolders, locals.circulatedSupply) * (qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, locals.possessorPubkey, locals.possessorPubkey, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) + qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, locals.possessorPubkey, locals.possessorPubkey, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX)));
                for (locals._t = 0 ; locals._t < state.numberOfMuslim; locals._t++)
                {
                    if (state.muslim.get(locals._t) == locals.possessorPubkey)
                    {
                        break;
                    }
                }
                if (locals._t == state.numberOfMuslim)
                {
                    qpi.transfer(locals.possessorPubkey, div(locals.muslimRevenue, locals.circulatedSupply - locals.amountOfQcapMuslimHold) * (qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, locals.possessorPubkey, locals.possessorPubkey, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) + qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, locals.possessorPubkey, locals.possessorPubkey, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX)));
                }
            }

            locals.iter.next();
        }

        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            qpi.transfer(state.staker.get(locals._t).stakerAddress, div(locals.paymentForQCAPHolders, locals.circulatedSupply) * state.staker.get(locals._t).amount);
            for (locals._r = 0 ; locals._r < state.numberOfMuslim; locals._r++)
            {

                if (state.muslim.get(locals._r) == state.staker.get(locals._t).stakerAddress)
                {
                    break;
                }
            }
            if (locals._r == state.numberOfMuslim)
            {
                qpi.transfer(state.staker.get(locals._t).stakerAddress, div(locals.muslimRevenue, locals.circulatedSupply - locals.amountOfQcapMuslimHold) * state.staker.get(locals._t).amount);
            }
        }

        /*
            General Proposal Result
        */

        for (locals._t = state.numberOfGP - 1; locals._t >= 0; locals._t--)
        {
            if (state.GP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedGProposal = state.GP.get(locals._t);
                if (state.GP.get(locals._t).numberOfYes + state.GP.get(locals._t).numberOfNo < div(state.GP.get(locals._t).currentQuorumPercent * state.GP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedGProposal.result = 2;
                }
                else if (state.GP.get(locals._t).numberOfYes <= state.GP.get(locals._t).numberOfNo)
                {
                    locals.updatedGProposal.result = 1;
                }
                else 
                {
                    locals.updatedGProposal.result = 0;
                }
                state.GP.set(locals._t, locals.updatedGProposal);
            }
            else 
            {
                break;
            }
        }

        /*
            Quorum Proposal Result
        */

        for (locals._t = state.numberOfQCP - 1; locals._t >= 0; locals._t--)
        {
            if (state.QCP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedQCProposal = state.QCP.get(locals._t);
                if (state.QCP.get(locals._t).numberOfYes + state.QCP.get(locals._t).numberOfNo < div(state.QCP.get(locals._t).currentQuorumPercent * state.QCP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedQCProposal.result = 2;
                }
                else if (state.QCP.get(locals._t).numberOfYes <= state.QCP.get(locals._t).numberOfNo)
                {
                    locals.updatedQCProposal.result = 1;
                }   
                else 
                {
                    locals.updatedQCProposal.result = 0;
                }
                state.QCP.set(locals._t, locals.updatedQCProposal);
            }
            else 
            {
                break;
            }
        }

        /*
            IPO Proposal Result
        */

        locals.requiredFund = 0;

        for (locals._t = state.numberOfIPOP - 1; locals._t >= 0; locals._t--)
        {
            if (state.IPOP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedIPOProposal = state.IPOP.get(locals._t);
                if (state.IPOP.get(locals._t).numberOfYes + state.IPOP.get(locals._t).numberOfNo < div(state.IPOP.get(locals._t).currentQuorumPercent * state.IPOP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedIPOProposal.result = 2;
                }
                else if (state.IPOP.get(locals._t).numberOfYes <= state.IPOP.get(locals._t).numberOfNo)
                {
                    locals.updatedIPOProposal.result = 1;
                }
                else 
                {
                    locals.updatedIPOProposal.result = 0;
                    locals.requiredFund += div(locals.updatedIPOProposal.totalWeight * 1ULL, locals.updatedIPOProposal.numberOfYes * 1ULL);
                    locals.updatedIPOProposal.assignedFund = div(locals.updatedIPOProposal.totalWeight * 1ULL, locals.updatedIPOProposal.numberOfYes * 1ULL);
                }
                state.IPOP.set(locals._t, locals.updatedIPOProposal);
            }
            else 
            {
                break;
            }
        }

        locals.tmpAmount = 0;

        for (locals._t = state.numberOfIPOP - 1; locals._t >= 0; locals._t--)
        {
            if (state.IPOP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                if (state.IPOP.get(locals._t).result != 0)
                {
                    continue;
                }
                locals.updatedIPOProposal = state.IPOP.get(locals._t);
                if (state.reinvestingFund < locals.requiredFund)
                {
                    locals.updatedIPOProposal.assignedFund = div(div(div(locals.updatedIPOProposal.totalWeight * 1ULL, locals.updatedIPOProposal.numberOfYes * 1ULL) * 1000, locals.requiredFund) * state.reinvestingFund, 1000ULL);
                }
                state.IPOP.set(locals._t, locals.updatedIPOProposal);

                for (locals._r = 675 ; locals._r >= 0; locals._r--)
                {
                    if ((676 - locals._r) * (qpi.ipoBidPrice(locals.updatedIPOProposal.ipoContractIndex, locals._r) + 1) > (sint64)locals.updatedIPOProposal.assignedFund)
                    {
                        if (locals._r == 675)
                        {
                            break;
                        }
                        qpi.bidInIPO(locals.updatedIPOProposal.ipoContractIndex, qpi.ipoBidPrice(locals.updatedIPOProposal.ipoContractIndex, locals._r + 1) + 1, 675 - locals._r);
                        locals.tmpAmount += (qpi.ipoBidPrice(locals.updatedIPOProposal.ipoContractIndex, locals._r + 1) + 1) * (675 - locals._r);
                        break;
                    }
                }
            }
            else 
            {
                break;
            }
        }

        state.reinvestingFund -= locals.tmpAmount;

        /*
            QEarn Proposal Result  
        */

        locals.requiredFund = 0;

        for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
        {
            if (state.IPOP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedQEarnProposal = state.QEarnP.get(locals._t);
                if (state.QEarnP.get(locals._t).numberOfYes + state.QEarnP.get(locals._t).numberOfNo < div(state.QEarnP.get(locals._t).currentQuorumPercent * state.QEarnP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedQEarnProposal.result = 2;
                }
                else if (state.QEarnP.get(locals._t).numberOfYes <= state.QEarnP.get(locals._t).numberOfNo)
                {
                    locals.updatedQEarnProposal.result = 1;
                }
                else 
                {
                    locals.updatedQEarnProposal.result = 0;
                    locals.requiredFund += locals.updatedQEarnProposal.amountOfInvestPerEpoch * locals.updatedQEarnProposal.numberOfEpoch;
                }
                state.QEarnP.set(locals._t, locals.updatedQEarnProposal);
            }
            else 
            {
                break;
            }
        }

        locals.tmpAmount = 0;

        if (state.reinvestingFund < locals.requiredFund)
        {
            for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
            {
                if (state.QEarnP.get(locals._t).proposedEpoch == qpi.epoch())
                {
                    if (state.QEarnP.get(locals._t).result != 0)
                    {
                        continue;
                    }
                    locals.updatedQEarnProposal = state.QEarnP.get(locals._t);
                    locals.updatedQEarnProposal.assignedFundPerEpoch = div(div(div(locals.updatedQEarnProposal.numberOfEpoch * locals.updatedQEarnProposal.amountOfInvestPerEpoch * 1000, locals.requiredFund) * state.reinvestingFund, 1000ULL), locals.updatedQEarnProposal.numberOfEpoch * 1ULL);
                    locals.tmpAmount += locals.updatedQEarnProposal.assignedFundPerEpoch * locals.updatedQEarnProposal.numberOfEpoch;
                    state.QEarnP.set(locals._t, locals.updatedQEarnProposal);
                }
                else 
                {
                    break;
                }
            }
            state.reinvestingFund -= locals.tmpAmount;
        }

        else 
        {
            state.reinvestingFund -= locals.requiredFund;
        }

        /*
            Fundrasing Proposal Result
        */

        for (locals._t = state.numberOfFundP - 1; locals._t >= 0; locals._t--)
        {
            if (state.FundP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedFundProposal = state.FundP.get(locals._t);
                if (state.FundP.get(locals._t).numberOfYes + state.FundP.get(locals._t).numberOfNo < div(state.FundP.get(locals._t).currentQuorumPercent * state.FundP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedFundProposal.result = 2;
                }
                else if (state.FundP.get(locals._t).numberOfYes <= state.FundP.get(locals._t).numberOfNo)
                {
                    locals.updatedFundProposal.result = 1;
                }
                else 
                {
                    locals.updatedFundProposal.result = 0;

                    getCurrentDate(qpi, locals.curDate);
                    QUOTTERY::unpackQuotteryDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
                    if (locals.year == 25 && state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedFundProposal.result = 3;
                    }
                    else if (locals.year == 26 && state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedFundProposal.result = 3;
                    }
                    else if (locals.year == 27 && state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedFundProposal.result = 3;
                    }
                    else if (state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_QCAP_MAX_SUPPLY)
                    {
                        locals.updatedFundProposal.result = 3;
                    }
                }
                state.FundP.set(locals._t, locals.updatedFundProposal);
            }
            else 
            {
                break;
            }
        }

        /*
            Marketplace Proposal Result
        */

        for (locals._t = state.numberOfMKTP - 1; locals._t >= 0; locals._t--)
        {
            if (state.MKTP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedMKTProposal = state.MKTP.get(locals._t);
                if (state.MKTP.get(locals._t).numberOfYes + state.MKTP.get(locals._t).numberOfNo < div(state.MKTP.get(locals._t).currentQuorumPercent * state.MKTP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedMKTProposal.result = 2;
                    qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                }
                else if (state.MKTP.get(locals._t).numberOfYes <= state.MKTP.get(locals._t).numberOfNo)
                {
                    locals.updatedMKTProposal.result = 1;
                    qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                }
                else 
                {
                    locals.updatedMKTProposal.result = 0;

                    getCurrentDate(qpi, locals.curDate);
                    QUOTTERY::unpackQuotteryDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
                    if (locals.year == 25 && state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedMKTProposal.result = 3;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (locals.year == 26 && state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedMKTProposal.result = 3;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (locals.year == 27 && state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedMKTProposal.result = 3;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_QCAP_MAX_SUPPLY)
                    {
                        locals.updatedMKTProposal.result = 3;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (state.reinvestingFund < locals.updatedMKTProposal.amountOfQubic)
                    {
                        locals.updatedMKTProposal.result = 3;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else 
                    {
                        state.qcapSoldAmount += locals.updatedMKTProposal.amountOfQcap;

                        qpi.transfer(locals.updatedMKTProposal.proposer, locals.updatedMKTProposal.amountOfQubic);

                        state.reinvestingFund -= locals.updatedMKTProposal.amountOfQubic;

                        locals.transferShareManagementRights_input.asset.assetName = QVAULT_QCAP_ASSETNAME;
                        locals.transferShareManagementRights_input.asset.issuer = state.QCAP_ISSUER;
                        locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
                        locals.transferShareManagementRights_input.numberOfShares = locals.updatedMKTProposal.amountOfQcap;

                        INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

                        qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, locals.updatedMKTProposal.amountOfQcap, locals.updatedMKTProposal.proposer);
                    }
                }
                state.MKTP.set(locals._t, locals.updatedMKTProposal);
            }
            else 
            {
                break;
            }
        }
        

        /*
            Allocation Proposal Result
        */

        for (locals._t = state.numberOfAlloP - 1; locals._t >= 0; locals._t--)
        {
            if (state.AlloP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedAlloProposal = state.AlloP.get(locals._t);
                if (state.AlloP.get(locals._t).numberOfYes + state.AlloP.get(locals._t).numberOfNo < div(state.AlloP.get(locals._t).currentQuorumPercent * state.AlloP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedAlloProposal.result = 2;
                }
                else if (state.AlloP.get(locals._t).numberOfYes <= state.AlloP.get(locals._t).numberOfNo)
                {
                    locals.updatedAlloProposal.result = 1;
                }
                else 
                {
                    locals.updatedAlloProposal.result = 0;
                }
                state.AlloP.set(locals._t, locals.updatedAlloProposal);
            }
            else 
            {
                break;
            }
        }

        /*
            Muslim Proposal Result
        */

        for (locals._t = state.numberOfMSP - 1; locals._t >= 0; locals._t--)
        {
            if (state.MSP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedMSProposal = state.MSP.get(locals._t);
                if (state.MSP.get(locals._t).numberOfYes + state.MSP.get(locals._t).numberOfNo < div(state.MSP.get(locals._t).currentQuorumPercent * state.MSP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedMSProposal.result = 2;
                }
                else if (state.MSP.get(locals._t).numberOfYes <= state.MSP.get(locals._t).numberOfNo)
                {
                    locals.updatedMSProposal.result = 1;
                }
                else 
                {
                    locals.updatedMSProposal.result = 0;
                }
                state.MSP.set(locals._t, locals.updatedMSProposal);
            }
            else 
            {
                break;
            }
        }

        state.totalVotingPower = 0;
        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            state.votingPower.set(locals._t, state.staker.get(locals._t));
            state.totalVotingPower += state.staker.get(locals._t).amount;
        }
        state.numberOfVotingPower = state.numberOfStaker;

    }

    struct POST_INCOMING_TRANSFER_locals
    {
        AssetPossessionIterator iter;
        Asset QCAPId;
        id possessorPubkey;
        uint64 circulatedSupply;
        uint64 lockedFund;
        sint32 _t;
    };

    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        switch (input.type)
        {
            case TransferType::qpiTransfer:
                if (input.sourceId.u64._0 == QEARN_CONTRACT_INDEX)
                {
                    locals.lockedFund = 0;
                    for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
                    {
                        if (state.QEarnP.get(locals._t).result == 0 && state.QEarnP.get(locals._t).proposedEpoch + 54 == qpi.epoch())
                        {
                            locals.lockedFund += state.QEarnP.get(locals._t).assignedFundPerEpoch;
                        }
                    }
                    state.reinvestingFund += locals.lockedFund;
                    state.totalNotMSRevenue += input.amount - locals.lockedFund;
                    state.totalHistoryRevenue += input.amount - locals.lockedFund;
                    state.revenueInQcapPerEpoch.set(qpi.epoch(), state.revenueInQcapPerEpoch.get(qpi.epoch()) + input.amount - locals.lockedFund);
                    state.revenueByQearn += input.amount - locals.lockedFund;

                }
                break;
            case TransferType::ipoBidRefund:
                state.reinvestingFund += input.amount;
                break;
            case TransferType::qpiDistributeDividends:
                state.totalHistoryRevenue += input.amount;
                state.revenueInQcapPerEpoch.set(qpi.epoch(), state.revenueInQcapPerEpoch.get(qpi.epoch()) + input.amount);
                state.revenuePerShare.set(input.sourceId.u64._0, input.amount);
                
                for (locals._t = 0; locals._t < (sint32)state.numberOfMuslimShare; locals._t++)
                {
                    if (state.muslimShares.get(locals._t) == input.sourceId.u64._0)
                    {
                        state.totalMuslimRevenue += input.amount;
                        break;
                    }
                }
                if (locals._t == state.numberOfMuslimShare)
                {
                    state.totalNotMSRevenue += input.amount;
                }
                break;
        }
    }

    PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};