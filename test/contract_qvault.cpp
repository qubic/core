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
static constexpr uint64 QVAULT_MAX_REVENUE = 1000000000000ull;
static constexpr uint64 QVAULT_MIN_REVENUE = 100000000000ull;
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

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QVAULT_CONTRACT_INDEX, END_EPOCH, expectSuccess);
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

    void stake(const id& address, uint32 amount)
    {
        QVAULT::stake_input input;
        QVAULT::stake_output output;

        input.amount = amount;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 1, input, output, address, 0);
    }

    void unStake(const id& address, uint32 amount)
    {
        QVAULT::unStake_input input{amount};
        QVAULT::unStake_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 2, input, output, address, 0);
    }

    void submitGP(const id& address)
    {
        QVAULT::submitGP_input input;
        QVAULT::submitGP_input output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 3, input, output, address, 0);
    }

    void submitQCP(const id& address, uint32 newQuorumPercent)
    {
        QVAULT::submitQCP_input input;
        QVAULT::submitQCP_output output;

        input.newQuorumPercent = newQuorumPercent;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 4, input, output, address, 10000000);
    }

    void submitIPOP(const id& address, uint32 ipoContractIndex)
    {
        QVAULT::submitIPOP_input input;
        QVAULT::submitIPOP_output output;

        input.ipoContractIndex = ipoContractIndex;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 5, input, output, address, 10000000);
    }

    void submitQEarnP(const id& address, uint64 amountPerEpoch, uint32 numberOfEpoch)
    {
        QVAULT::submitQEarnP_input input;
        QVAULT::submitQEarnP_output output;

        input.amountPerEpoch = amountPerEpoch;
        input.numberOfEpoch = numberOfEpoch;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 6, input, output, address, 10000000);
    }

    void submitFundP(const id& address, uint32 amountOfQcap, uint64 priceOfOneQcap)
    {
        QVAULT::submitFundP_input input;
        QVAULT::submitFundP_output output;

        input.amountOfQcap = amountOfQcap;
        input.priceOfOneQcap = priceOfOneQcap;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 7, input, output, address, 10000000);
    }

    void submitMKTP(const id& address, uint64 amountOfQcap, uint64 amountOfQubic, uint64 shareName, uint32 indexOfShare, uint32 amountOfShare)
    {
        QVAULT::submitMKTP_input input;
        QVAULT::submitMKTP_output output;

        input.amountOfQcap = amountOfQcap;
        input.amountOfQubic = amountOfQubic;
        input.amountOfShare = amountOfShare;
        input.indexOfShare = indexOfShare;
        input.shareName = shareName;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 8, input, output, address, 10000000);
    }

    void submitAlloP(const id& address, uint8 reinvested, uint8 team, uint8 burn, uint8 distribute)
    {
        QVAULT::submitAlloP_input input;
        QVAULT::submitAlloP_output output;

        input.reinvested = reinvested;
        input.team = team;
        input.burn = burn ;
        input.distribute = distribute;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 9, input, output, address, 10000000);
    }

    void submitMSP(const id& address)
    {
        QVAULT::submitMSP_input input;
        QVAULT::submitMSP_input output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 10, input, output, address, 10000000);
    }

    void voteInProposal(const id& address, uint64 priceOfIPO, uint32 proposalType, uint32 proposalId, bit yes)
    {
        QVAULT::voteInProposal_input input;
        QVAULT::voteInProposal_output output;

        input.priceOfIPO = priceOfIPO;
        input.proposalType = proposalType;
        input.proposalId = proposalId;
        input.yes = yes;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 11, input, output, address, 10000000);
    }

    void buyQcap(const id& address, uint32 amount, uint64 fund)
    {
        QVAULT::buyQcap_input input;
        QVAULT::buyQcap_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 12, input, output, address, fund);
    }

    void TransferShareManagementRights(const id& address, Asset asset, sint64 numberOfShares, uint32 newManagingContractIndex)
    {
        QVAULT::TransferShareManagementRights_input input;
        QVAULT::TransferShareManagementRights_output output;

        input.asset.assetName = QVAULT_QCAP_ASSETNAME;
        input.asset.issuer = QVAULT_QCAP_ISSUER;
        input.numberOfShares = numberOfShares;
        input.newManagingContractIndex = newManagingContractIndex;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 13, input, output, address, 1000000);
    }

    void submitMuslimId(const id& address)
    {
        QVAULT::submitMuslimId_input input;
        QVAULT::submitMuslimId_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 13, input, output, address, 0);
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QVAULT_ISSUE_ASSET_FEE);
        return output.issuedNumberOfShares;
    }

    sint64 TransferShareOwnershipAndPossession(const id& issuer, uint64 assetName, sint64 numberOfShares, id newOwnerAndPossesor)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;

        input.assetName = assetName;
        input.issuer = issuer;
        input.newOwnerAndPossessor = newOwnerAndPossesor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, issuer, QVAULT_TOKEN_TRANSFER_FEE);

        return output.transferredNumberOfShares;
    }
};
