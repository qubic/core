#define NO_UEFI

#include <map>
#include <random>

#include "contract_testing.h"

static std::mt19937_64 rand64;
static constexpr uint64 QVAULT_QCAP_MAX_HOLDERS = 131072;
static constexpr uint64 QVAULT_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QVAULT_TOKEN_TRANSFER_FEE = 1000000ull;
static constexpr uint32 QVAULT_SMALL_AMOUNT_QCAP_TRANSFER = 1000;
static constexpr uint32 QVAULT_BIG_AMOUNT_QCAP_TRANSFER = 1000000;
static constexpr uint32 QVAULT_QCAP_SOLD_AMOUNT = 1652235;
static constexpr uint64 QVAULT_MAX_REVENUE = 1000000000000ull;
static constexpr uint64 QVAULT_MIN_REVENUE = 100000000000ull;;
static const id QVAULT_CONTRACT_ID(QVAULT_CONTRACT_INDEX, 0, 0, 0);
const id QVAULT_QCAP_ISSUER = ID(_Q, _C, _A, _P, _W, _M, _Y, _R, _S, _H, _L, _B, _J, _H, _S, _T, _T, _Z, _Q, _V, _C, _I, _B, _A, _R, _V, _O, _A, _S, _K, _D, _E, _N, _A, _S, _A, _K, _N, _O, _B, _R, _G, _P, _F, _W, _W, _K, _R, _C, _U, _V, _U, _A, _X, _Y, _E);
const id QVAULT_adminAddress = ID(_H, _E, _C, _G, _U, _G, _H, _C, _J, _K, _Q, _O, _S, _D, _T, _M, _E, _H, _Q, _Y, _W, _D, _D, _T, _L, _F, _D, _A, _S, _Z, _K, _M, _G, _J, _L, _S, _R, _C, _S, _T, _H, _H, _A, _P, _P, _E, _D, _L, _G, _B, _L, _X, _J, _M, _N, _D);

static unsigned long long random(unsigned long long minValue, unsigned long long maxValue)
{
    if(minValue > maxValue) 
    {
        return 0;
    }
    return minValue + rand64() % (maxValue - minValue);
}

static id getUser(unsigned long long i)
{
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

static std::vector<id> getRandomUsers(unsigned int totalUsers, unsigned int maxNum)
{
    unsigned long long userCount = random(0, maxNum);
    std::vector<id> users;
    users.reserve(userCount);
    for (unsigned int i = 0; i < userCount; ++i)
    {
        unsigned long long userIdx = random(0, totalUsers - 1);
        users.push_back(getUser(userIdx));
    }
    return users;
}

class QVAULTChecker : public QVAULT
{
public:
    void submitGPChecker(uint32 index, id proposer)
    {
        EXPECT_EQ(GP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(GP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(GP.get(index).numberOfNo, 0);
        EXPECT_EQ(GP.get(index).numberOfYes, 0);
        EXPECT_EQ(GP.get(index).proposedEpoch, 139);
        EXPECT_EQ(GP.get(index).proposer, proposer);
        EXPECT_EQ(GP.get(index).result, 0);
    }

    void submitQCPChecker(uint32 index, id proposer, uint32 newQuorumPercent)
    {
        EXPECT_EQ(QCP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(QCP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(QCP.get(index).numberOfNo, 0);
        EXPECT_EQ(QCP.get(index).numberOfYes, 0);
        EXPECT_EQ(QCP.get(index).proposedEpoch, 139);
        EXPECT_EQ(QCP.get(index).proposer, proposer);
        EXPECT_EQ(QCP.get(index).result, 0);
        EXPECT_EQ(QCP.get(index).newQuorumPercent, newQuorumPercent);
    }

    void submitIPOPChecker(uint32 index, id proposer, uint32 ipoContractIndex)
    {
        EXPECT_EQ(IPOP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(IPOP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(IPOP.get(index).numberOfNo, 0);
        EXPECT_EQ(IPOP.get(index).numberOfYes, 0);
        EXPECT_EQ(IPOP.get(index).proposedEpoch, 139);
        EXPECT_EQ(IPOP.get(index).proposer, proposer);
        EXPECT_EQ(IPOP.get(index).result, 0);
        EXPECT_EQ(IPOP.get(index).ipoContractIndex, ipoContractIndex);
        EXPECT_EQ(IPOP.get(index).totalWeight, 0);
        EXPECT_EQ(IPOP.get(index).assignedFund, 0);
    }

    void submitQEarnPChecker(uint32 index, id proposer, uint64 amountPerEpoch, uint32 numberOfEpoch)
    {
        EXPECT_EQ(QEarnP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(QEarnP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(QEarnP.get(index).numberOfNo, 0);
        EXPECT_EQ(QEarnP.get(index).numberOfYes, 0);
        EXPECT_EQ(QEarnP.get(index).proposedEpoch, 139);
        EXPECT_EQ(QEarnP.get(index).proposer, proposer);
        EXPECT_EQ(QEarnP.get(index).result, 0);
        EXPECT_EQ(QEarnP.get(index).amountOfInvestPerEpoch, amountPerEpoch);
        EXPECT_EQ(QEarnP.get(index).assignedFundPerEpoch, amountPerEpoch);
        EXPECT_EQ(QEarnP.get(index).numberOfEpoch, numberOfEpoch);
    }

    void submitFundPChecker(uint32 index, id proposer, uint32 amountOfQcap, uint64 pricePerOneQcap)
    {
        EXPECT_EQ(FundP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(FundP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(FundP.get(index).numberOfNo, 0);
        EXPECT_EQ(FundP.get(index).numberOfYes, 0);
        EXPECT_EQ(FundP.get(index).proposedEpoch, 139);
        EXPECT_EQ(FundP.get(index).proposer, proposer);
        EXPECT_EQ(FundP.get(index).result, 0);
        EXPECT_EQ(FundP.get(index).amountOfQcap, amountOfQcap);
        EXPECT_EQ(FundP.get(index).pricePerOneQcap, pricePerOneQcap);
        EXPECT_EQ(FundP.get(index).restSaleAmount, amountOfQcap);   
    }

    void submitMKTPChecker(uint32 index, id proposer, uint64 amountOfQcap, uint64 amountOfQubic, uint64 shareName, uint32 indexOfShare, uint32 amountOfShare)
    {
        EXPECT_EQ(MKTP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(MKTP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(MKTP.get(index).numberOfNo, 0);
        EXPECT_EQ(MKTP.get(index).numberOfYes, 0);
        EXPECT_EQ(MKTP.get(index).proposedEpoch, 139);
        EXPECT_EQ(MKTP.get(index).proposer, proposer);
        EXPECT_EQ(MKTP.get(index).result, 0);
        EXPECT_EQ(MKTP.get(index).amountOfQcap, amountOfQcap);
        EXPECT_EQ(MKTP.get(index).amountOfQubic, amountOfQubic);
        EXPECT_EQ(MKTP.get(index).shareName, shareName);
        EXPECT_EQ(MKTP.get(index).shareIndex, indexOfShare);
        EXPECT_EQ(MKTP.get(index).amountOfShare, amountOfShare);
    }

    void submitAlloPChecker(uint32 index, id proposer, uint32 reinvested, uint32 team, uint32 burn, uint32 distribute)
    {
        EXPECT_EQ(AlloP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(AlloP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(AlloP.get(index).numberOfNo, 0);
        EXPECT_EQ(AlloP.get(index).numberOfYes, 0);
        EXPECT_EQ(AlloP.get(index).proposedEpoch, 139);
        EXPECT_EQ(AlloP.get(index).proposer, proposer);
        EXPECT_EQ(AlloP.get(index).result, 0);
        EXPECT_EQ(AlloP.get(index).reinvested, reinvested);
        EXPECT_EQ(AlloP.get(index).team, team);
        EXPECT_EQ(AlloP.get(index).burnQcap, burn);
        EXPECT_EQ(AlloP.get(index).distributed, distribute);
    }

    void MSPChecker(uint32 index, id proposer, uint32 shareIndex)
    {
        EXPECT_EQ(MSP.get(index).currentQuorumPercent, 670);
        EXPECT_EQ(MSP.get(index).currentTotalVotingPower, 10000);
        EXPECT_EQ(MSP.get(index).numberOfNo, 0);
        EXPECT_EQ(MSP.get(index).numberOfYes, 0);
        EXPECT_EQ(MSP.get(index).proposedEpoch, 139);
        EXPECT_EQ(MSP.get(index).proposer, proposer);
        EXPECT_EQ(MSP.get(index).result, 0);
        EXPECT_EQ(MSP.get(index).muslimShareIndex, shareIndex);
    }

    void voteInProposalChecker(uint32 proposalId, uint32 proposalType, uint32 numberOfYes, uint32 numberOfNo)
    {
        switch (proposalType)
        {
        case 1:
            EXPECT_EQ(GP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(GP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 2:
            EXPECT_EQ(QCP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(QCP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 3:
            EXPECT_EQ(IPOP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(IPOP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 4:
            EXPECT_EQ(QEarnP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(QEarnP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 5:
            EXPECT_EQ(FundP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(FundP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 6:
            EXPECT_EQ(MKTP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(MKTP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 7:
            EXPECT_EQ(AlloP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(AlloP.get(proposalId).numberOfNo, numberOfNo);
            break;
        case 8:
            EXPECT_EQ(MSP.get(proposalId).numberOfYes, numberOfYes);
            EXPECT_EQ(MSP.get(proposalId).numberOfNo, numberOfNo);
            break;
        
        default:
            break;
        }
    }

    void submitMuslimIdChecker(id muslimId)
    {
        EXPECT_EQ(muslimId, muslim.get(0));
    }

    void POST_INCOMING_TRANSFER_checker(uint64 distributedAmount, uint32 epoch, uint32 shareIndex)
    {
        EXPECT_EQ(distributedAmount, totalNotMSRevenue);
        EXPECT_EQ(distributedAmount, totalHistoryRevenue);
        EXPECT_EQ(distributedAmount, revenueInQcapPerEpoch.get(epoch));
        EXPECT_EQ(distributedAmount, revenuePerShare.get(shareIndex));
    }
};

class QXChecker : public QX
{
public:
    uint64 getDistributedAmountInEndTick()
    {
        return _distributedAmount;
    }
};

class ContractTestingQvault : protected ContractTesting
{
public:
    ContractTestingQvault()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QVAULT);
        callSystemProcedure(QVAULT_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QVAULTChecker* getState()
    {
        return (QVAULTChecker*)contractStates[QVAULT_CONTRACT_INDEX];
    }

    QXChecker* qxGetState()
    {
        return (QXChecker*)contractStates[QX_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QVAULT_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    void qxEndTick(bool expectSuccess = true)
    {
        callSystemProcedure(QX_CONTRACT_INDEX, END_TICK, expectSuccess);
    }

    QVAULT::getData_output getData() const
    {
        QVAULT::getData_input input;
        QVAULT::getData_output output;

        callFunction(QVAULT_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QVAULT::getStakedAmountAndVotingPower_output getStakedAmountAndVotingPower(id address) const
    {
        QVAULT::getStakedAmountAndVotingPower_input input;
        QVAULT::getStakedAmountAndVotingPower_output output;

        input.address = address;

        callFunction(QVAULT_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QVAULT::getGP_output getGP(uint32 proposalId) const
    {
        QVAULT::getGP_input input;
        QVAULT::getGP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QVAULT::getQCP_output getQCP(uint32 proposalId) const
    {
        QVAULT::getQCP_input input;
        QVAULT::getQCP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QVAULT::getIPOP_output getIPOP(uint32 proposalId) const
    {
        QVAULT::getIPOP_input input;
        QVAULT::getIPOP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QVAULT::getQEarnP_output getQEarnP(uint32 proposalId) const
    {
        QVAULT::getQEarnP_input input;
        QVAULT::getQEarnP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QVAULT::getFundP_output getFundP(uint32 proposalId) const
    {
        QVAULT::getFundP_input input;
        QVAULT::getFundP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QVAULT::getMKTP_output getMKTP(uint32 proposalId) const
    {
        QVAULT::getMKTP_input input;
        QVAULT::getMKTP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    QVAULT::getAlloP_output getAlloP(uint32 proposalId) const
    {
        QVAULT::getAlloP_input input;
        QVAULT::getAlloP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 9, input, output);
        return output;
    }

    QVAULT::getMSP_output getMSP(uint32 proposalId) const
    {
        QVAULT::getMSP_input input;
        QVAULT::getMSP_output output;

        input.proposalId = proposalId;

        callFunction(QVAULT_CONTRACT_INDEX, 10, input, output);
        return output;
    }

    QVAULT::getIdentitiesHvVtPw_output getIdentitiesHvVtPw(uint32 offset, uint32 count) const
    {
        QVAULT::getIdentitiesHvVtPw_input input;
        QVAULT::getIdentitiesHvVtPw_output output;

        input.count = count;
        input.offset = offset;

        callFunction(QVAULT_CONTRACT_INDEX, 11, input, output);
        return output;
    }

    QVAULT::ppCreationPower_output ppCreationPower(id address) const
    {
        QVAULT::ppCreationPower_input input;
        QVAULT::ppCreationPower_output output;

        input.address = address;

        callFunction(QVAULT_CONTRACT_INDEX, 12, input, output);
        return output;
    }

    QVAULT::getQcapBurntAmountInLastEpoches_output getQcapBurntAmountInLastEpoches(uint32 numberOfLastEpoches) const
    {
        QVAULT::getQcapBurntAmountInLastEpoches_input input;
        QVAULT::getQcapBurntAmountInLastEpoches_output output;

        input.numberOfLastEpoches = numberOfLastEpoches;

        callFunction(QVAULT_CONTRACT_INDEX, 13, input, output);
        return output;
    }

    QVAULT::getAmountToBeSoldPerYear_output getAmountToBeSoldPerYear(uint32 year) const
    {
        QVAULT::getAmountToBeSoldPerYear_input input;
        QVAULT::getAmountToBeSoldPerYear_output output;

        input.year = year;

        callFunction(QVAULT_CONTRACT_INDEX, 14, input, output);
        return output;
    }

    QVAULT::getTotalRevenueInQcap_output getTotalRevenueInQcap() const
    {
        QVAULT::getTotalRevenueInQcap_input input;
        QVAULT::getTotalRevenueInQcap_output output;

        callFunction(QVAULT_CONTRACT_INDEX, 15, input, output);
        return output;
    }

    QVAULT::getRevenueInQcapPerEpoch_output getRevenueInQcapPerEpoch(uint32 epoch) const
    {
        QVAULT::getRevenueInQcapPerEpoch_input input;
        QVAULT::getRevenueInQcapPerEpoch_output output;

        input.epoch = epoch;

        callFunction(QVAULT_CONTRACT_INDEX, 16, input, output);
        return output;
    }

    QVAULT::getRevenuePerShare_output getRevenuePerShare(uint32 contractIndex) const
    {
        QVAULT::getRevenuePerShare_input input;
        QVAULT::getRevenuePerShare_output output;

        input.contractIndex = contractIndex;

        callFunction(QVAULT_CONTRACT_INDEX, 17, input, output);
        return output;
    }

    QVAULT::getAmountOfShareQvaultHold_output getAmountOfShareQvaultHold(Asset assetInfo) const
    {
        QVAULT::getAmountOfShareQvaultHold_input input;
        QVAULT::getAmountOfShareQvaultHold_output output;

        input.assetInfo = assetInfo;

        callFunction(QVAULT_CONTRACT_INDEX, 18, input, output);
        return output;
    }

    QVAULT::getNumberOfHolderAndAvgAm_output getNumberOfHolderAndAvgAm() const
    {
        QVAULT::getNumberOfHolderAndAvgAm_input input;
        QVAULT::getNumberOfHolderAndAvgAm_output output;

        callFunction(QVAULT_CONTRACT_INDEX, 19, input, output);
        return output;
    }

    QVAULT::getAmountForQearnInUpcomingEpoch_output getAmountForQearnInUpcomingEpoch(uint32 epoch) const
    {
        QVAULT::getAmountForQearnInUpcomingEpoch_input input;
        QVAULT::getAmountForQearnInUpcomingEpoch_output output;

        input.epoch = epoch;

        callFunction(QVAULT_CONTRACT_INDEX, 20, input, output);
        return output;
    }

    uint32 stake(const id& address, uint32 amount)
    {
        QVAULT::stake_input input;
        QVAULT::stake_output output;

        input.amount = amount;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 1, input, output, address, 0);

        return output.returnCode;
    }

    uint32 unStake(const id& address, uint32 amount)
    {
        QVAULT::unStake_input input{amount};
        QVAULT::unStake_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 2, input, output, address, 0);

        return output.returnCode;
    }

    uint32 submitGP(const id& address)
    {
        QVAULT::submitGP_input input;
        QVAULT::submitGP_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 3, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitQCP(const id& address, uint32 newQuorumPercent)
    {
        QVAULT::submitQCP_input input;
        QVAULT::submitQCP_output output;

        input.newQuorumPercent = newQuorumPercent;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 4, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitIPOP(const id& address, uint32 ipoContractIndex)
    {
        QVAULT::submitIPOP_input input;
        QVAULT::submitIPOP_output output;

        input.ipoContractIndex = ipoContractIndex;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 5, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitQEarnP(const id& address, uint64 amountPerEpoch, uint32 numberOfEpoch)
    {
        QVAULT::submitQEarnP_input input;
        QVAULT::submitQEarnP_output output;

        input.amountPerEpoch = amountPerEpoch;
        input.numberOfEpoch = numberOfEpoch;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 6, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitFundP(const id& address, uint32 amountOfQcap, uint64 priceOfOneQcap)
    {
        QVAULT::submitFundP_input input;
        QVAULT::submitFundP_output output;

        input.amountOfQcap = amountOfQcap;
        input.priceOfOneQcap = priceOfOneQcap;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 7, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitMKTP(const id& address, uint64 amountOfQcap, uint64 amountOfQubic, uint64 shareName, uint32 indexOfShare, uint32 amountOfShare)
    {
        QVAULT::submitMKTP_input input;
        QVAULT::submitMKTP_output output;

        input.amountOfQcap = amountOfQcap;
        input.amountOfQubic = amountOfQubic;
        input.amountOfShare = amountOfShare;
        input.indexOfShare = indexOfShare;
        input.shareName = shareName;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 8, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitAlloP(const id& address, uint32 reinvested, uint32 team, uint32 burn, uint32 distribute)
    {
        QVAULT::submitAlloP_input input;
        QVAULT::submitAlloP_output output;

        input.reinvested = reinvested;
        input.team = team;
        input.burn = burn ;
        input.distribute = distribute;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 9, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 submitMSP(const id& address, uint32 shareIndex)
    {
        QVAULT::submitMSP_input input;
        QVAULT::submitMSP_output output;

        input.shareIndex = shareIndex;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 10, input, output, address, 10000000);

        return output.returnCode;
    }

    uint32 voteInProposal(const id& address, uint64 priceOfIPO, uint32 proposalType, uint32 proposalId, bit yes)
    {
        QVAULT::voteInProposal_input input;
        QVAULT::voteInProposal_output output;

        input.priceOfIPO = priceOfIPO;
        input.proposalType = proposalType;
        input.proposalId = proposalId;
        input.yes = yes;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 11, input, output, address, 0);

        return output.returnCode;
    }

    uint32 buyQcap(const id& address, uint32 amount, uint64 fund)
    {
        QVAULT::buyQcap_input input;
        QVAULT::buyQcap_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 12, input, output, address, fund);

        return output.returnCode;
    }

    uint64 TransferShareManagementRights(const id& address, Asset asset, sint64 numberOfShares, uint32 newManagingContractIndex)
    {
        QVAULT::TransferShareManagementRights_input input;
        QVAULT::TransferShareManagementRights_output output;

        input.asset.assetName = QVAULT_QCAP_ASSETNAME;
        input.asset.issuer = QVAULT_QCAP_ISSUER;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 13, input, output, address, 1000000);

        return output.transferredNumberOfShares;
    }

    sint64 QXTransferShareManagementRights(const id& issuer, uint64 assetName, uint32 newManagingContractIndex, sint64 numberOfShares, id currentOwner)
    {
        QX::TransferShareManagementRights_input input;
        QX::TransferShareManagementRights_output output;

        input.asset.assetName = assetName;
        input.asset.issuer = issuer;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, currentOwner, 0);

        return output.transferredNumberOfShares;
    }


    uint32 submitMuslimId(const id& address)
    {
        QVAULT::submitMuslimId_input input;
        QVAULT::submitMuslimId_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 14, input, output, address, 0);

        return output.returnCode;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QVAULT_ISSUE_ASSET_FEE);
        return output.issuedNumberOfShares;
    }

    sint64 TransferShareOwnershipAndPossession(const id& issuer, uint64 assetName, sint64 numberOfShares, id oldOwnerAndPossessor, id newOwnerAndPossesor)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;

        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, oldOwnerAndPossessor, QVAULT_TOKEN_TRANSFER_FEE);

        return output.transferredNumberOfShares;
    }
};


TEST(TestContractQvault, testingAllProceduresAndFunctions)
{
    system.epoch = 138;
    ContractTestingQvault QvaultV2;

    uint64 qcapAssetName = assetNameFromString("QCAP");

    increaseEnergy(QVAULT_QCAP_ISSUER, QVAULT_ISSUE_ASSET_FEE);
    EXPECT_EQ(QvaultV2.issueAsset(QVAULT_QCAP_ISSUER, qcapAssetName, QVAULT_QCAP_MAX_SUPPLY, 0, 0), QVAULT_QCAP_MAX_SUPPLY);

    increaseEnergy(QVAULT_QCAP_ISSUER, QVAULT_TOKEN_TRANSFER_FEE);
    EXPECT_EQ(QvaultV2.TransferShareOwnershipAndPossession(QVAULT_QCAP_ISSUER, qcapAssetName, QVAULT_QCAP_MAX_SUPPLY, QVAULT_QCAP_ISSUER, QVAULT_CONTRACT_ID), QVAULT_QCAP_MAX_SUPPLY);

    auto stakers = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    uint32 maxTransferAmount = QVAULT_QCAP_SOLD_AMOUNT;
    for (const auto& user : stakers)
    {
        uint32 amountTransfer = (uint32)random(1000ULL, 100000ULL);
        if(maxTransferAmount < amountTransfer)
        {
            break;
        }
        maxTransferAmount -= amountTransfer;
        increaseEnergy(QVAULT_CONTRACT_ID, QVAULT_TOKEN_TRANSFER_FEE);

        EXPECT_EQ(QvaultV2.TransferShareOwnershipAndPossession(QVAULT_QCAP_ISSUER, qcapAssetName, amountTransfer, QVAULT_CONTRACT_ID, user), amountTransfer);

        increaseEnergy(user, 1);
        EXPECT_EQ(QvaultV2.QXTransferShareManagementRights(QVAULT_QCAP_ISSUER, qcapAssetName, QVAULT_CONTRACT_INDEX, amountTransfer, user), amountTransfer);
        EXPECT_EQ(numberOfPossessedShares(qcapAssetName, QVAULT_QCAP_ISSUER, user, user, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX), amountTransfer);
        EXPECT_EQ(QvaultV2.stake(user, amountTransfer), 0);
        EXPECT_EQ(numberOfPossessedShares(qcapAssetName, QVAULT_QCAP_ISSUER, QVAULT_CONTRACT_ID, QVAULT_CONTRACT_ID, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX), amountTransfer);
        EXPECT_EQ(QvaultV2.unStake(user, amountTransfer), 0);
        EXPECT_EQ(numberOfPossessedShares(qcapAssetName, QVAULT_QCAP_ISSUER, user, user, QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX), amountTransfer);
    }

    QvaultV2.TransferShareOwnershipAndPossession(QVAULT_QCAP_ISSUER, qcapAssetName, 10000, QVAULT_CONTRACT_ID, stakers[0]);
    EXPECT_EQ(QvaultV2.stake(stakers[0], 10000), 0);

    QvaultV2.endEpoch();
    system.epoch++;

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitGP(stakers[0]), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 10000000);
    QvaultV2.getState()->submitGPChecker(0, stakers[0]);

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitQCP(stakers[0], 500), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 20000000);
    QvaultV2.getState()->submitQCPChecker(0, stakers[0], 500);

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitIPOP(stakers[0], 15), QVAULTLogInfo::QvaultInsufficientFund);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 20000000);
    // QvaultV2.getState()->submitIPOPChecker(0, stakers[0], 15);

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitQEarnP(stakers[0], 1000000000, 10), QVAULTLogInfo::QvaultInsufficientFund);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 20000000);
    // QvaultV2.getState()->submitQEarnPChecker(0, stakers[0], 1000000000, 10);

    updateTime();
    updateQpiTime();

    setMemory(utcTime, 0);
    utcTime.Year = 2025;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_2025MAX_QCAP_SALE_AMOUNT, 100000), QVAULTLogInfo::QvaultOverflowSaleAmount);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 20000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_2025MAX_QCAP_SALE_AMOUNT - 1652235, 100000), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 30000000);
    QvaultV2.getState()->submitFundPChecker(0, stakers[0], QVAULT_2025MAX_QCAP_SALE_AMOUNT - 1652235, 100000);

    setMemory(utcTime, 0);
    utcTime.Year = 2026;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_2026MAX_QCAP_SALE_AMOUNT, 100000), QVAULTLogInfo::QvaultOverflowSaleAmount);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 30000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_2026MAX_QCAP_SALE_AMOUNT - 1652235, 100000), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 40000000);
    QvaultV2.getState()->submitFundPChecker(1, stakers[0], QVAULT_2026MAX_QCAP_SALE_AMOUNT - 1652235, 100000);

    setMemory(utcTime, 0);
    utcTime.Year = 2027;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_2027MAX_QCAP_SALE_AMOUNT, 100000), QVAULTLogInfo::QvaultOverflowSaleAmount);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 40000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_2027MAX_QCAP_SALE_AMOUNT - 1652235, 100000), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 50000000);
    QvaultV2.getState()->submitFundPChecker(2, stakers[0], QVAULT_2027MAX_QCAP_SALE_AMOUNT - 1652235, 100000);

    setMemory(utcTime, 0);
    utcTime.Year = 2028;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_QCAP_MAX_SUPPLY, 100000), QVAULTLogInfo::QvaultOverflowSaleAmount);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 50000000);
    EXPECT_EQ(QvaultV2.submitFundP(stakers[0], QVAULT_QCAP_MAX_SUPPLY - 1652235, 100000), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 60000000);
    QvaultV2.getState()->submitFundPChecker(3, stakers[0], QVAULT_QCAP_MAX_SUPPLY - 1652235, 100000);

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitMKTP(stakers[0], 10000, 1000000000, assetNameFromString("QX"), 1, 5), QVAULTLogInfo::QvaultNotTransferredShare);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 60000000);
    // QvaultV2.getState()->submitMKTPChecker(0, stakers[0], 10000, 1000000000, assetNameFromString("QX"), 1, 5);

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitAlloP(stakers[0], 450, 20, 100, 400), QVAULTLogInfo::QvaultNotInTime);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 60000000);

    setMemory(utcTime, 0);
    utcTime.Year = 2029;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    EXPECT_EQ(QvaultV2.submitAlloP(stakers[0], 450, 20, 100, 400), QVAULTLogInfo::QvaultNotInTime);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 60000000);

    setMemory(utcTime, 0);
    utcTime.Year = 2029;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    EXPECT_EQ(QvaultV2.submitAlloP(stakers[0], 450, 0, 120, 400), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 70000000);
    QvaultV2.getState()->submitAlloPChecker(0, stakers[0], 450, 0, 120, 400);

    increaseEnergy(stakers[0], 10000000);
    EXPECT_EQ(QvaultV2.submitMSP(stakers[0], 2), QVAULTLogInfo::QvaultSuccess);
    EXPECT_EQ(getBalance(QVAULT_CONTRACT_ID), 80000000);
    QvaultV2.getState()->MSPChecker(0, stakers[0], 2);

    EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 1, 0, 1), QVAULTLogInfo::QvaultSuccess);
    QvaultV2.getState()->voteInProposalChecker(0, 1, 10000, 0);

    EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 2, 0, 1), QVAULTLogInfo::QvaultSuccess);
    QvaultV2.getState()->voteInProposalChecker(0, 2, 10000, 0);

    // EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 3, 0, 1), QVAULTLogInfo::QvaultSuccess);
    // QvaultV2.getState()->voteInProposalChecker(0, 3, 10000, 0);

    // EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 4, 0, 1), QVAULTLogInfo::QvaultSuccess);
    // QvaultV2.getState()->voteInProposalChecker(0, 4, 10000, 0);

    EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 5, 0, 1), QVAULTLogInfo::QvaultSuccess);
    QvaultV2.getState()->voteInProposalChecker(0, 5, 10000, 0);

    // EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 6, 0, 1), QVAULTLogInfo::QvaultSuccess);
    // QvaultV2.getState()->voteInProposalChecker(0, 6, 10000, 0);

    EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 7, 0, 1), QVAULTLogInfo::QvaultSuccess);
    QvaultV2.getState()->voteInProposalChecker(0, 7, 10000, 0);

    EXPECT_EQ(QvaultV2.voteInProposal(stakers[0], 100000000, 8, 0, 1), QVAULTLogInfo::QvaultSuccess);
    QvaultV2.getState()->voteInProposalChecker(0, 8, 10000, 0);

    Asset qcapShare;
    qcapShare.assetName = qcapAssetName;
    qcapShare.issuer = QVAULT_QCAP_ISSUER;

    EXPECT_EQ(QvaultV2.QXTransferShareManagementRights(QVAULT_QCAP_ISSUER, qcapAssetName, QVAULT_CONTRACT_INDEX, 10000, QVAULT_CONTRACT_ID), 10000);
    increaseEnergy(QVAULT_QCAP_ISSUER, 1000000);
    EXPECT_EQ(QvaultV2.TransferShareManagementRights(QVAULT_CONTRACT_ID, qcapShare, 10000, QX_CONTRACT_INDEX), 10000);

    QvaultV2.submitMuslimId(stakers[0]);
    QvaultV2.getState()->submitMuslimIdChecker(stakers[0]);

    std::vector<std::pair<m256i, unsigned int>> qxSharesHolers{{QVAULT_CONTRACT_ID, 676}};
    issueContractShares(QX_CONTRACT_INDEX, qxSharesHolers);

    std::vector<std::pair<m256i, unsigned int>> qvaultSharesHolers{ {stakers[1], 1}, {stakers[2], 675}};
    issueContractShares(QVAULT_CONTRACT_INDEX, qvaultSharesHolers);

    increaseEnergy(stakers[1], 10000000);
    EXPECT_EQ(QvaultV2.submitGP(stakers[1]), QVAULTLogInfo::QvaultSuccess);
    QvaultV2.getState()->submitGPChecker(1, stakers[1]);
    
    for (uint32 t = 0 ; t < 100; t++)
    {
        uint64 newAssetName;
        char strAssetName[6];
        for (uint32 r = 0 ; r < 5; r++)
        {
            strAssetName[r] = 'A' + (uint32)random(0, 25);
        }
        strAssetName[5] = 0;
        newAssetName = assetNameFromString(strAssetName);
        increaseEnergy(stakers[2], 1000000000);
        QvaultV2.issueAsset(stakers[2], newAssetName, 1000000000, 0, 0);
    }

    QvaultV2.qxEndTick();
    
    QvaultV2.getState()->POST_INCOMING_TRANSFER_checker(QvaultV2.qxGetState()->getDistributedAmountInEndTick(), system.epoch, QX_CONTRACT_INDEX);

    
}