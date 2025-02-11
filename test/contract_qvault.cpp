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
    void endEpochChecker(uint64 revenue, const std::vector<id>& QCAPHolders) 
    {
        uint64 paymentForShareholders = QPI::div(revenue * shareholderDividend, 1000ULL);
        uint64 paymentForQCAPHolders = QPI::div(revenue * QCAPHolderPermille, 1000ULL);
        uint64 paymentForReinvest = QPI::div(revenue * reinvestingPermille, 1000ULL);
        uint64 amountOfBurn = QPI::div(revenue * burnPermille, 1000ULL);
        uint64 paymentForDevelopment = revenue - paymentForShareholders - paymentForQCAPHolders - paymentForReinvest - amountOfBurn;

        if(paymentForReinvest > QVAULT_MAX_REINVEST_AMOUNT)
        {
            paymentForQCAPHolders += paymentForReinvest - QVAULT_MAX_REINVEST_AMOUNT;
            paymentForReinvest = QVAULT_MAX_REINVEST_AMOUNT;
        }

        uint64 QCAPCirculatedSupply = QVAULT_QCAP_MAX_SUPPLY;

        for(uint64 i = 0 ; i < QCAPHolders.size(); i++)
        {
            for(uint64 j = 0 ; j < numberOfBannedAddress; j++)
            {
                if(QCAPHolders[i] == bannedAddress.get(j))
                {
                    QCAPCirculatedSupply -= numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, QCAP_ISSUER, QCAPHolders[i], QCAPHolders[i], QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
                    break;
                }
            }
        }

        QCAPCirculatedSupply -= numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, QCAP_ISSUER, QVAULT_initialBannedAddress1, QVAULT_initialBannedAddress1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
        QCAPCirculatedSupply -= numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, QCAP_ISSUER, QVAULT_initialBannedAddress2, QVAULT_initialBannedAddress2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
        /*
            This for loop will check the revenue distributed to QCAPHolders.
        */
        for (const auto& user : QCAPHolders)
        {
            uint64 j = 0;
            for(j = 0 ; j < numberOfBannedAddress; j++)
            {
                if(user == bannedAddress.get(j))
                {
                    break;
                }
            }
            if(j != numberOfBannedAddress)
            {
                continue;
            }
            EXPECT_EQ(QPI::div(paymentForQCAPHolders, QCAPCirculatedSupply) * numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, QCAP_ISSUER, user, user, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), getBalance(user) - 1);
        }
        if(paymentForReinvest > QVAULT_MAX_REINVEST_AMOUNT)
        {
            EXPECT_EQ(QVAULT_MAX_REINVEST_AMOUNT, getBalance(reinvestingAddress));
        }
        else 
        {
            EXPECT_EQ(paymentForReinvest, getBalance(reinvestingAddress));
        }
        EXPECT_EQ(paymentForDevelopment, getBalance(adminAddress));
    }

    void balanceChecker(const id& user)
    {
        EXPECT_EQ(getBalance(user), 1);
    }

    void submitAuthAddressChecker()
    {
        EXPECT_EQ(NULL_ID, newAuthAddress1);
        EXPECT_EQ(NULL_ID, newAuthAddress2);
        EXPECT_EQ(NULL_ID, newAuthAddress3);
    }

    void submitAuthAddressWithExactAuthId(const id& newAuthAddress)
    {
        EXPECT_EQ(newAuthAddress1, newAuthAddress);
        EXPECT_EQ(newAuthAddress2, newAuthAddress);
        EXPECT_EQ(newAuthAddress3, newAuthAddress);
    }

    void changeAuthAddressChecker(uint32 numberOfAuth, const id& newAuthAddress)
    {
        if(numberOfAuth == 1)
        {
            EXPECT_EQ(authAddress1, newAuthAddress);
        }
        else if(numberOfAuth == 2)
        {
            EXPECT_EQ(authAddress2, newAuthAddress);
        }
        else 
        {
            EXPECT_EQ(authAddress3, newAuthAddress);
        }
    }

    void submitDistributionPermilleChecker(uint32 newQCAPHolderPt, uint32 newReinvestingPt, uint32 newDevPt)
    {
        EXPECT_EQ(newQCAPHolderPt, newQCAPHolderPermille1);
        EXPECT_EQ(newQCAPHolderPt, newQCAPHolderPermille2);
        EXPECT_EQ(newQCAPHolderPt, newQCAPHolderPermille3);

        EXPECT_EQ(newReinvestingPt, newReinvestingPermille1);
        EXPECT_EQ(newReinvestingPt, newReinvestingPermille2);
        EXPECT_EQ(newReinvestingPt, newReinvestingPermille3);

        EXPECT_EQ(newDevPt, newDevPermille1);
        EXPECT_EQ(newDevPt, newDevPermille2);
        EXPECT_EQ(newDevPt, newDevPermille3);
    }

    void changeDistributionPermilleChecker(uint32 newQCAPHolderPt, uint32 newReinvestingPt, uint32 newDevPt)
    {
        EXPECT_EQ(newQCAPHolderPt, QCAPHolderPermille);
        EXPECT_EQ(newReinvestingPt, reinvestingPermille);
        EXPECT_EQ(newDevPt, devPermille);
    }

    void submitReinvestingAddressChecker(const id& newReinvestingAddress)
    {
        EXPECT_EQ(newReinvestingAddress1, newReinvestingAddress);
        EXPECT_EQ(newReinvestingAddress2, newReinvestingAddress);
        EXPECT_EQ(newReinvestingAddress3, newReinvestingAddress);
    }

    void changeReinvestingAddressChecker(const id& newReinvestingAddress)
    {
        EXPECT_EQ(reinvestingAddress, newReinvestingAddress);
    }

    void submitAdminAddressChecker(const id& newAdminAddress)
    {
        EXPECT_EQ(newAdminAddress1, newAdminAddress);
        EXPECT_EQ(newAdminAddress2, newAdminAddress);
        EXPECT_EQ(newAdminAddress3, newAdminAddress);
    }

    void changeAdminAddressChecker(const id& newAdminAddress)
    {
        EXPECT_EQ(adminAddress, newAdminAddress);
    }

    void submitBannedAddressChecker(const id& newBannedAddress)
    {
        EXPECT_EQ(bannedAddress1, newBannedAddress);
        EXPECT_EQ(bannedAddress2, newBannedAddress);
        EXPECT_EQ(bannedAddress3, newBannedAddress);
    }

    void saveBannedAddressChecker(const id& newBannedAddress)
    {
        EXPECT_EQ(bannedAddress.get(numberOfBannedAddress - 1), newBannedAddress);
    }

    void submitUnbannedAddressChecker(const id& newUnbannedAddress)
    {
        EXPECT_EQ(unbannedAddress1, newUnbannedAddress);
        EXPECT_EQ(unbannedAddress2, newUnbannedAddress);
        EXPECT_EQ(unbannedAddress3, newUnbannedAddress);
    }

    void saveUnbannedAddressChecker(const id& unbannedAddress)
    {
        for(uint32 i = 0 ; i < numberOfBannedAddress; i++)
        {
            EXPECT_NE(unbannedAddress, bannedAddress.get(i));
        }
    }

    void getDataChecker(const getData_output& output)
    {
        EXPECT_EQ(output.authAddress1, authAddress1);
        EXPECT_EQ(output.authAddress2, authAddress2);
        EXPECT_EQ(output.authAddress3, authAddress3);
        EXPECT_EQ(output.reinvestingAddress, reinvestingAddress);
        EXPECT_EQ(output.shareholderDividend, shareholderDividend);
        EXPECT_EQ(output.devPermille, devPermille);
        EXPECT_EQ(output.QCAPHolderPermille, QCAPHolderPermille);
        EXPECT_EQ(output.reinvestingPermille, reinvestingPermille);
        EXPECT_EQ(output.adminAddress, adminAddress);
        EXPECT_EQ(output.newAuthAddress1, newAuthAddress1);
        EXPECT_EQ(output.newAuthAddress2, newAuthAddress2);
        EXPECT_EQ(output.newAuthAddress3, newAuthAddress3);
        EXPECT_EQ(output.newAdminAddress1, newAdminAddress1);
        EXPECT_EQ(output.newAdminAddress2, newAdminAddress2);
        EXPECT_EQ(output.newAdminAddress3, newAdminAddress3);
        EXPECT_EQ(output.newReinvestingAddress1, newReinvestingAddress1);
        EXPECT_EQ(output.newReinvestingAddress2, newReinvestingAddress2);
        EXPECT_EQ(output.newReinvestingAddress3, newReinvestingAddress3);
        EXPECT_EQ(output.numberOfBannedAddress, numberOfBannedAddress);
        EXPECT_EQ(output.bannedAddress1, bannedAddress1);
        EXPECT_EQ(output.bannedAddress2, bannedAddress2);
        EXPECT_EQ(output.bannedAddress3, bannedAddress3);
        EXPECT_EQ(output.unbannedAddress1, unbannedAddress1);
        EXPECT_EQ(output.unbannedAddress2, unbannedAddress2);
        EXPECT_EQ(output.unbannedAddress3, unbannedAddress3);
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

    void submitAuthAddress(const id& authAddress, const id& newAuthAddress)
    {
        QVAULT::submitAuthAddress_input input;
        QVAULT::submitAuthAddress_output output;

        input.newAddress = newAuthAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 1, input, output, authAddress, 0);
    }

    void changeAuthAddress(const id& authAddress, uint32 numberOfChangedAddress)
    {
        QVAULT::changeAuthAddress_input input{numberOfChangedAddress};
        QVAULT::changeAuthAddress_output output;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 2, input, output, authAddress, 0);
    }

    void submitDistributionPermille(const id& authAddress, uint32 newQCAPHolderPermille, uint32 newReinvestingPermille, uint32 newDevPermille)
    {
        QVAULT::submitDistributionPermille_input input;
        QVAULT::submitDistributionPermille_output output;

        input.newDevPermille = newDevPermille;
        input.newQCAPHolderPermille = newQCAPHolderPermille;
        input.newReinvestingPermille = newReinvestingPermille;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 3, input, output, authAddress, 0);
    }

    void changeDistributionPermille(const id& authAddress, uint32 newQCAPHolderPermille, uint32 newReinvestingPermille, uint32 newDevPermille)
    {
        QVAULT::changeDistributionPermille_input input;
        QVAULT::changeDistributionPermille_output output;

        input.newDevPermille = newDevPermille;
        input.newQCAPHolderPermille = newQCAPHolderPermille;
        input.newReinvestingPermille = newReinvestingPermille;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 4, input, output, authAddress, 0);
    }

    void submitReinvestingAddress(const id& authAddress, const id& newReinvestingAddress)
    {
        QVAULT::submitReinvestingAddress_input input;
        QVAULT::submitReinvestingAddress_output output;

        input.newAddress = newReinvestingAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 5, input, output, authAddress, 0);
    }

    void changeReinvestingAddress(const id& authAddress, const id& newReinvestingAddress)
    {
        QVAULT::changeReinvestingAddress_input input;
        QVAULT::changeReinvestingAddress_output output;

        input.newAddress = newReinvestingAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 6, input, output, authAddress, 0);
    }

    void submitAdminAddress(const id& authAddress, const id& newAdminAddress)
    {
        QVAULT::submitAdminAddress_input input;
        QVAULT::submitAdminAddress_output output;

        input.newAddress = newAdminAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 7, input, output, authAddress, 0);
    }

    void changeAdminAddress(const id& authAddress, const id& newAdminAddress)
    {
        QVAULT::changeAdminAddress_input input;
        QVAULT::changeAdminAddress_output output;

        input.newAddress = newAdminAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 8, input, output, authAddress, 0);
    }

    void submitBannedAddress(const id& authAddress, const id& bannedAddress)
    {
        QVAULT::submitBannedAddress_input input;
        QVAULT::submitBannedAddress_output output;

        input.bannedAddress = bannedAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 9, input, output, authAddress, 0);
    }

    void saveBannedAddress(const id& authAddress, const id& bannedAddress)
    {
        QVAULT::saveBannedAddress_input input;
        QVAULT::saveBannedAddress_output output;

        input.bannedAddress = bannedAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 10, input, output, authAddress, 0);
    }

    void submitUnbannedAddress(const id& authAddress, const id& unbannedAddress)
    {
        QVAULT::submitUnbannedAddress_input input;
        QVAULT::submitUnbannedAddress_output output;

        input.unbannedAddress = unbannedAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 11, input, output, authAddress, 0);
    }

    void saveUnbannedAddress(const id& authAddress, const id& unbannedAddress)
    {
        QVAULT::unblockBannedAddress_input input;
        QVAULT::unblockBannedAddress_output output;

        input.unbannedAddress = unbannedAddress;

        invokeUserProcedure(QVAULT_CONTRACT_INDEX, 12, input, output, authAddress, 0);
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

TEST(ContractQvault, END_EPOCH)
{
    ContractTestingQvault qvault;

    id issuer = QVAULT_QCAP_ISSUER;
    uint64 assetName = assetNameFromString("QCAP");
    sint64 numberOfShares = QVAULT_QCAP_MAX_SUPPLY;


    increaseEnergy(issuer, QVAULT_ISSUE_ASSET_FEE);
    EXPECT_EQ(qvault.issueAsset(issuer, assetName, numberOfShares, 0, 0), numberOfShares);

    uint64 currentAmount = QVAULT_QCAP_MAX_SUPPLY;
    uint64 numberOfHolder = 0;
    bool flag = 0;
    auto QCAPHolders = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    std::map<id, bool> transferChecker;

    /*
        sending the QCAP token to bannedAddresses
    */
    increaseEnergy(issuer, QVAULT_TOKEN_TRANSFER_FEE);
    increaseEnergy(QVAULT_initialBannedAddress1, 1);
    currentAmount -= qvault.TransferShareOwnershipAndPossession(issuer, assetName, random(0, QVAULT_BIG_AMOUNT_QCAP_TRANSFER), QVAULT_initialBannedAddress1);

    increaseEnergy(issuer, QVAULT_TOKEN_TRANSFER_FEE);
    increaseEnergy(QVAULT_initialBannedAddress2, 1);
    currentAmount -= qvault.TransferShareOwnershipAndPossession(issuer, assetName, random(0, QVAULT_BIG_AMOUNT_QCAP_TRANSFER), QVAULT_initialBannedAddress2);
    /*
        this while statement will distribute the QCAP token to holders.
    */
    while(1)
    {
        uint64 amountOfQCAPTransfer;
        if(flag) 
        {
            amountOfQCAPTransfer = random(0, QVAULT_BIG_AMOUNT_QCAP_TRANSFER);
        }
        else 
        {
            amountOfQCAPTransfer = random(0, QVAULT_SMALL_AMOUNT_QCAP_TRANSFER);
        }
        if(currentAmount < amountOfQCAPTransfer || numberOfHolder == QCAPHolders.size())
        {
            break;
        }

        if(transferChecker[QCAPHolders[numberOfHolder]])
        {
            QCAPHolders.erase(QCAPHolders.begin() + numberOfHolder);
            continue;
        }
        transferChecker[QCAPHolders[numberOfHolder]] = 1;
        currentAmount -= amountOfQCAPTransfer;
        increaseEnergy(issuer, QVAULT_TOKEN_TRANSFER_FEE);
        increaseEnergy(QCAPHolders[numberOfHolder], 1);
        qvault.getState()->balanceChecker(QCAPHolders[numberOfHolder]);
        EXPECT_EQ(qvault.TransferShareOwnershipAndPossession(issuer, assetName, amountOfQCAPTransfer, QCAPHolders[numberOfHolder]), amountOfQCAPTransfer);
        numberOfHolder++;
    }

    uint64 revenue = random(QVAULT_MIN_REVENUE, QVAULT_MAX_REVENUE);
    increaseEnergy(QVAULT_CONTRACT_ID, revenue);
    qvault.endEpoch();
    qvault.getState()->endEpochChecker(revenue, QCAPHolders);
}

TEST(ContractQvault, submitAuthAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    for (const auto& user : randomAddresses)
    {
        // make sure that user exists in spectrum
        increaseEnergy(user, 1);

        // checking to change the auth address using the non-authAddress
        qvault.submitAuthAddress(user, user);
        qvault.getState()->submitAuthAddressChecker();
    }

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the auth address using the exact authAddresss
    qvault.submitAuthAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.getState()->submitAuthAddressWithExactAuthId(randomAddresses[0]);
}

TEST(ContractQvault, changeAuthAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the authAddress3 with exact process
    qvault.submitAuthAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.changeAuthAddress(QVAULT_authAddress1, 3);
    qvault.getState()->changeAuthAddressChecker(3, randomAddresses[0]);

    qvault.submitAuthAddress(QVAULT_authAddress1, randomAddresses[1]);
    qvault.submitAuthAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.changeAuthAddress(QVAULT_authAddress2, 3);
    qvault.getState()->changeAuthAddressChecker(3, randomAddresses[1]);

    qvault.submitAuthAddress(QVAULT_authAddress1, QVAULT_authAddress3);
    qvault.submitAuthAddress(QVAULT_authAddress2, QVAULT_authAddress3);
    qvault.changeAuthAddress(QVAULT_authAddress2, 3);
    qvault.getState()->changeAuthAddressChecker(3, QVAULT_authAddress3);

    // checking to change the authAddress2 with exact process
    qvault.submitAuthAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.changeAuthAddress(QVAULT_authAddress1, 2);
    qvault.getState()->changeAuthAddressChecker(2, randomAddresses[0]);

    qvault.submitAuthAddress(QVAULT_authAddress1, randomAddresses[1]);
    qvault.submitAuthAddress(QVAULT_authAddress3, randomAddresses[1]);
    qvault.changeAuthAddress(QVAULT_authAddress3, 2);
    qvault.getState()->changeAuthAddressChecker(2, randomAddresses[1]);

    qvault.submitAuthAddress(QVAULT_authAddress1, QVAULT_authAddress2);
    qvault.submitAuthAddress(QVAULT_authAddress3, QVAULT_authAddress2);
    qvault.changeAuthAddress(QVAULT_authAddress3, 2);
    qvault.getState()->changeAuthAddressChecker(2, QVAULT_authAddress2);

    // checking to change the authAddress1 with exact process
    qvault.submitAuthAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.changeAuthAddress(QVAULT_authAddress2, 1);
    qvault.getState()->changeAuthAddressChecker(1, randomAddresses[0]);

    qvault.submitAuthAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.submitAuthAddress(QVAULT_authAddress3, randomAddresses[1]);
    qvault.changeAuthAddress(QVAULT_authAddress3, 1);
    qvault.getState()->changeAuthAddressChecker(1, randomAddresses[1]);

    qvault.submitAuthAddress(QVAULT_authAddress2, QVAULT_authAddress1);
    qvault.submitAuthAddress(QVAULT_authAddress3, QVAULT_authAddress1);
    qvault.changeAuthAddress(QVAULT_authAddress3, 1);
    qvault.getState()->changeAuthAddressChecker(1, QVAULT_authAddress1);
}

TEST(ContractQvault, submitDistributionPermille)
{
    ContractTestingQvault qvault;

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the Permille
    qvault.submitDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress2, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress3, 500, 400, 70);
    qvault.getState()->submitDistributionPermilleChecker(500, 400, 70);
}

TEST(ContractQvault, changeDistributionPermille)
{
    ContractTestingQvault qvault;

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the Permille
    qvault.submitDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress2, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress3, 500, 400, 70);
    qvault.changeDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.getState()->changeDistributionPermilleChecker(500, 400, 70);

    qvault.submitDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress2, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress3, 500, 400, 70);
    qvault.changeDistributionPermille(QVAULT_authAddress2, 500, 400, 70);
    qvault.getState()->changeDistributionPermilleChecker(500, 400, 70);

    qvault.submitDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress2, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress3, 500, 400, 70);
    qvault.changeDistributionPermille(QVAULT_authAddress3, 500, 400, 70);
    qvault.getState()->changeDistributionPermilleChecker(500, 400, 70);
}

TEST(ContractQvault, submitReinvestingAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the reinvestingAddress
    qvault.submitReinvestingAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitReinvestingAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitReinvestingAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.getState()->submitReinvestingAddressChecker(randomAddresses[0]);
}

TEST(ContractQvault, changeReinvestingAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the reinvestingAddress
    qvault.submitReinvestingAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitReinvestingAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitReinvestingAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.changeReinvestingAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.getState()->changeReinvestingAddressChecker(randomAddresses[0]);

    qvault.submitReinvestingAddress(QVAULT_authAddress1, randomAddresses[1]);
    qvault.submitReinvestingAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.submitReinvestingAddress(QVAULT_authAddress3, randomAddresses[1]);
    qvault.changeReinvestingAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.getState()->changeReinvestingAddressChecker(randomAddresses[1]);

    qvault.submitReinvestingAddress(QVAULT_authAddress1, randomAddresses[2]);
    qvault.submitReinvestingAddress(QVAULT_authAddress2, randomAddresses[2]);
    qvault.submitReinvestingAddress(QVAULT_authAddress3, randomAddresses[2]);
    qvault.changeReinvestingAddress(QVAULT_authAddress3, randomAddresses[2]);
    qvault.getState()->changeReinvestingAddressChecker(randomAddresses[2]);
}

TEST(ContractQvault, submitAdminAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the adminAddress
    qvault.submitAdminAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAdminAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitAdminAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.getState()->submitAdminAddressChecker(randomAddresses[0]);
}

TEST(ContractQvault, changeAdminAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to change the adminAddress
    qvault.submitAdminAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAdminAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitAdminAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.changeAdminAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.getState()->changeAdminAddressChecker(randomAddresses[0]);

    qvault.submitAdminAddress(QVAULT_authAddress1, randomAddresses[1]);
    qvault.submitAdminAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.submitAdminAddress(QVAULT_authAddress3, randomAddresses[1]);
    qvault.changeAdminAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.getState()->changeAdminAddressChecker(randomAddresses[1]);

    qvault.submitAdminAddress(QVAULT_authAddress1, randomAddresses[2]);
    qvault.submitAdminAddress(QVAULT_authAddress2, randomAddresses[2]);
    qvault.submitAdminAddress(QVAULT_authAddress3, randomAddresses[2]);
    qvault.changeAdminAddress(QVAULT_authAddress3, randomAddresses[2]);
    qvault.getState()->changeAdminAddressChecker(randomAddresses[2]);
}

TEST(ContractQvault, submitBannedAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to submit the bannedAddress
    qvault.submitBannedAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitBannedAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitBannedAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.getState()->submitBannedAddressChecker(randomAddresses[0]);
}

TEST(ContractQvault, saveBannedAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to save the bannedAddress
    qvault.submitBannedAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitBannedAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitBannedAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.saveBannedAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.getState()->saveBannedAddressChecker(randomAddresses[0]);

    qvault.submitBannedAddress(QVAULT_authAddress1, randomAddresses[1]);
    qvault.submitBannedAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.submitBannedAddress(QVAULT_authAddress3, randomAddresses[1]);
    qvault.saveBannedAddress(QVAULT_authAddress2, randomAddresses[1]);
    qvault.getState()->saveBannedAddressChecker(randomAddresses[1]);

    qvault.submitBannedAddress(QVAULT_authAddress1, randomAddresses[2]);
    qvault.submitBannedAddress(QVAULT_authAddress2, randomAddresses[2]);
    qvault.submitBannedAddress(QVAULT_authAddress3, randomAddresses[2]);
    qvault.saveBannedAddress(QVAULT_authAddress3, randomAddresses[2]);
    qvault.getState()->saveBannedAddressChecker(randomAddresses[2]);
}

TEST(ContractQvault, submitUnbannedAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    // checking to submit unbannedAddress
    qvault.submitUnbannedAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitUnbannedAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitUnbannedAddress(QVAULT_authAddress3, randomAddresses[0]);
    qvault.getState()->submitUnbannedAddressChecker(randomAddresses[0]);
}

TEST(ContractQvault, unblockBannedAddress)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    for(uint32 i = 0 ; i < 10; i++)
    {
        qvault.submitBannedAddress(QVAULT_authAddress1, randomAddresses[i]);
        qvault.submitBannedAddress(QVAULT_authAddress2, randomAddresses[i]);
        qvault.submitBannedAddress(QVAULT_authAddress3, randomAddresses[i]);
        qvault.saveBannedAddress(QVAULT_authAddress1, randomAddresses[i]);
    }

    // checking to unblock the bannedAddress
    for(uint32 i = 0 ; i < 10; i++)
    {
        qvault.submitUnbannedAddress(QVAULT_authAddress1, randomAddresses[i]);
        qvault.submitUnbannedAddress(QVAULT_authAddress2, randomAddresses[i]);
        qvault.submitUnbannedAddress(QVAULT_authAddress3, randomAddresses[i]);
        qvault.saveUnbannedAddress(QVAULT_authAddress1, randomAddresses[i]);
        qvault.getState()->saveUnbannedAddressChecker(randomAddresses[i]);
    }
}

TEST(ContractQvault, getData)
{
    ContractTestingQvault qvault;

    auto randomAddresses = getRandomUsers(QVAULT_QCAP_MAX_HOLDERS, QVAULT_QCAP_MAX_HOLDERS);

    // make sure that user exists in spectrum
    increaseEnergy(QVAULT_authAddress1, 1);
    increaseEnergy(QVAULT_authAddress2, 1);
    increaseEnergy(QVAULT_authAddress3, 1);

    qvault.submitAuthAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitAuthAddress(QVAULT_authAddress3, randomAddresses[0]);

    qvault.submitDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress2, 500, 400, 70);
    qvault.submitDistributionPermille(QVAULT_authAddress3, 500, 400, 70);

    qvault.submitReinvestingAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitReinvestingAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitReinvestingAddress(QVAULT_authAddress3, randomAddresses[0]);

    qvault.submitAdminAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitAdminAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitAdminAddress(QVAULT_authAddress3, randomAddresses[0]);

    qvault.submitBannedAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitBannedAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitBannedAddress(QVAULT_authAddress3, randomAddresses[0]);

    qvault.submitUnbannedAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.submitUnbannedAddress(QVAULT_authAddress2, randomAddresses[0]);
    qvault.submitUnbannedAddress(QVAULT_authAddress3, randomAddresses[0]);

    auto output = qvault.getData();
    qvault.getState()->getDataChecker(output);

    qvault.changeAuthAddress(QVAULT_authAddress1, 3);
    qvault.changeDistributionPermille(QVAULT_authAddress1, 500, 400, 70);
    qvault.changeReinvestingAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.changeAdminAddress(QVAULT_authAddress1, randomAddresses[0]);
    qvault.saveBannedAddress(QVAULT_authAddress1, randomAddresses[0]);

    output = qvault.getData();
    qvault.getState()->getDataChecker(output);
}