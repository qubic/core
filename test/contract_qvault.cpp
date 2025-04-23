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
const id QVAULT_authAddress1 = ID(_T, _K, _U, _W, _W, _S, _N, _B, _A, _E, _G, _W, _J, _H, _Q, _J, _D, _F, _L, _G, _Q, _H, _J, _J, _C, _J, _B, _A, _X, _B, _S, _Q, _M, _Q, _A, _Z, _J, _J, _D, _Y, _X, _E, _P, _B, _V, _B, _B, _L, _I, _Q, _A, _N, _J, _T, _I, _D);
const id QVAULT_authAddress2 = ID(_F, _X, _J, _F, _B, _T, _J, _M, _Y, _F, _J, _H, _P, _B, _X, _C, _D, _Q, _T, _L, _Y, _U, _K, _G, _M, _H, _B, _B, _Z, _A, _A, _F, _T, _I, _C, _W, _U, _K, _R, _B, _M, _E, _K, _Y, _N, _U, _P, _M, _R, _M, _B, _D, _N, _D, _R, _G);
const id QVAULT_authAddress3 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
const id QVAULT_reinvestingAddress = ID(_R, _U, _U, _Y, _R, _V, _N, _K, _J, _X, _M, _L, _R, _B, _B, _I, _R, _I, _P, _D, _I, _B, _M, _H, _D, _H, _U, _A, _Z, _B, _Q, _K, _N, _B, _J, _T, _R, _D, _S, _P, _G, _C, _L, _Z, _C, _Q, _W, _A, _K, _C, _F, _Q, _J, _K, _K, _E);
const id QVAULT_adminAddress = ID(_H, _E, _C, _G, _U, _G, _H, _C, _J, _K, _Q, _O, _S, _D, _T, _M, _E, _H, _Q, _Y, _W, _D, _D, _T, _L, _F, _D, _A, _S, _Z, _K, _M, _G, _J, _L, _S, _R, _C, _S, _T, _H, _H, _A, _P, _P, _E, _D, _L, _G, _B, _L, _X, _J, _M, _N, _D);
const id QVAULT_initialBannedAddress1 = ID(_K, _E, _F, _D, _Z, _T, _Y, _L, _F, _E, _R, _A, _H, _D, _V, _L, _N, _Q, _O, _R, _D, _H, _F, _Q, _I, _B, _S, _B, _Z, _C, _W, _S, _Z, _X, _Z, _F, _F, _A, _N, _O, _T, _F, _A, _H, _W, _M, _O, _V, _G, _T, _R, _Q, _J, _P, _X, _D);
const id QVAULT_initialBannedAddress2 = ID(_E, _S, _C, _R, _O, _W, _B, _O, _T, _F, _T, _F, _I, _C, _I, _F, _P, _U, _X, _O, _J, _K, _G, _Q, _P, _Y, _X, _C, _A, _B, _L, _Z, _V, _M, _M, _U, _C, _M, _J, _F, _S, _G, _S, _A, _I, _A, _T, _Y, _I, _N, _V, _T, _Y, _G, _O, _A);

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

    void stake(const id& address, uint64 amount)
    {
        QVAULT::stake_input input;
        QVAULT::stake_output output;

        input.amount = amount;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 1, input, output, address, 0);
    }

    void unStake(const id& address, uint64 amount)
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

    void submitFundP(const id& address, uint64 amountOfQcap, uint64 priceOfOneQcap)
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
