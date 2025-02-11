#define NO_UEFI

#include <map>
#include <random>

#include "contract_testing.h"

const id MARKETPLACE_OWNER = ID(_Y, _R, _P, _H, _H, _S, _U, _E, _E, _B, _S, _A, _X, _B, _Y, _F, _B, _A, _X, _P, _U, _R, _E, _X, _F, _E, _S, _A, _Q, _F, _N, _C, _J, _O, _M, _R, _I, _G, _B, _C, _W, _D, _I, _M, _K, _R, _R, _I, _Z, _T, _K, _P, _O, _J, _F, _H);
const id CFB_ISSUER = ID(_C, _F, _B, _M, _E, _M, _Z, _O, _I, _D, _E, _X, _Q, _A, _U, _X, _Y, _Y, _S, _Z, _I, _U, _R, _A, _D, _Q, _L, _A, _P, _W, _P, _M, _N, _J, _X, _Q, _S, _N, _V, _Q, _Z, _A, _H, _Y, _V, _O, _P, _Y, _U, _K, _K, _J, _B, _J, _U, _C);
static constexpr uint64 PFP_ISSUE_ASSET_FEE = 1000000000ULL;
static constexpr uint64 PFP_TOKEN_TRANSFER_FEE = 1000000ULL;
static constexpr sint64 PFP_CREATED_CFB_AMOUNT = 1000000000000ULL;

static std::mt19937_64 rand64;

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

static Array<uint8, 64> getRandomURI()
{
    Array<uint8, 64> URI;

    for(sint32 i = 0 ; i < 64; i++)
    {
        uint8 t = (uint8)random(0, 127);
        if((t >= 48 && t <= 57) || (t >= 65 && t <= 90) || (t >= 97 && t <= 122))
        {
            URI.set(i, t);
            continue;
        }
        i--;
    }

    return URI;
}

class PFPChecker : public PFP
{
public:

    void cfbAndQubicPriceChecker(uint64 cfbPrice, uint64 qubicPrice)
    {
        EXPECT_EQ(cfbPrice, priceOfCFB);
        EXPECT_EQ(qubicPrice, priceOfQubic);
    }

    void createCollectionChecker(id user, uint64 priceForDropMint, uint32 countOfNFT, uint32 royalty, uint32 maxSizePerOneId, bit typeOfCollection, uint32 countOfCollection, Array<uint8, 64>& URI)
    {
        EXPECT_EQ(countOfNFT, numberOfNFTIncoming);
        EXPECT_EQ(numberOfCollection, countOfCollection);
        EXPECT_EQ(Collections.get(countOfCollection - 1).creator, user);
        EXPECT_EQ(Collections.get(countOfCollection - 1).currentSize, countOfNFT);
        EXPECT_EQ(Collections.get(countOfCollection - 1).maxSizeHoldingPerOneId, maxSizePerOneId);
        EXPECT_EQ(Collections.get(countOfCollection - 1).priceForDropMint, priceForDropMint);
        EXPECT_EQ(Collections.get(countOfCollection - 1).royalty, royalty);
        EXPECT_EQ(Collections.get(countOfCollection - 1).typeOfCollection, typeOfCollection);
        for(uint8 i = 0; i < 64; i++)
        {
            if(i >= PFP_LENGTH_OF_URI)
            {
                EXPECT_EQ(Collections.get(0).URI.get(i), 0);
            }
            else {
                EXPECT_EQ(Collections.get(0).URI.get(i), URI.get(i));
            } 
        }
    }
    
};

class ContractTestingPFP : protected ContractTesting
{
public:
    ContractTestingPFP()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(PFP);
        callSystemProcedure(PFP_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
        qLogger::initLogging();
    }

    ~ContractTestingPFP()
    {
        qLogger::deinitLogging();
    }

    PFPChecker* getState()
    {
        return (PFPChecker*)contractStates[PFP_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QVAULT_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    PFP::getNumberOfNFTForUser_output getNumberOfNFTForUser(const id& user) const
    {
        PFP::getNumberOfNFTForUser_input input;
        PFP::getNumberOfNFTForUser_output output;

        input.user = user;

        callFunction(PFP_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    PFP::getInfoOfNFTUserPossessed_output getInfoOfNFTUserPossessed(const id& user, uint32 NFTNumber) const
    {
        PFP::getInfoOfNFTUserPossessed_input input;
        PFP::getInfoOfNFTUserPossessed_output output;

        input.user = user;
        input.NFTNumber = NFTNumber;

        callFunction(PFP_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    PFP::getInfoOfMarketplace_output getInfoOfMarketplace() const
    {
        PFP::getInfoOfMarketplace_input input;
        PFP::getInfoOfMarketplace_output output;

        callFunction(PFP_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    PFP::settingCFBAndQubicPrice_output settingCFBAndQubicPrice(const id& marketOwnerAdress, uint64 CFBPrice, uint64 QubicPrice)
    {
        PFP::settingCFBAndQubicPrice_input input;
        PFP::settingCFBAndQubicPrice_output output;

        input.CFBPrice = CFBPrice;
        input.QubicPrice = QubicPrice;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 1, input, output, marketOwnerAdress, 0);

        return output;
    }

    PFP::createCollection_output createCollection(const id& user, uint64 priceForDropMint, uint32 volumn, uint32 royalty, uint32 maxSizePerOneId, bit typeOfCollection, Array<uint8, 64>& URI)
    {
        PFP::createCollection_input input;
        PFP::createCollection_output output;

        input.maxSizePerOneId = maxSizePerOneId;
        input.priceForDropMint = priceForDropMint;
        input.volumn = volumn;
        input.royalty = royalty;
        input.typeOfCollection = typeOfCollection;

        for(uint32 i = 0 ; i < 64; i++) 
		{
			if(i >= PFP_LENGTH_OF_URI) 
			{
				input.URI.set(i, 0);
			}
			else 
			{
				input.URI.set(i, URI.get(i));
			}
		}

        invokeUserProcedure(PFP_CONTRACT_INDEX, 2, input, output, user, 0);

        return output;
    }

    PFP::mint_output mint(const id& user, uint32 royalty, uint32 collectionId, Array<uint8, 64> URI, bit typeOfMint, uint64 mintFee)
    {
        PFP::mint_input input;
        PFP::mint_output output;

        input.collectionId = collectionId;
        input.royalty = royalty;
        input.typeOfMint = typeOfMint;

        for(uint32 i = 0 ; i < 64; i++)
        {
            input.URI.set(i, URI.get(i));
        }

        invokeUserProcedure(PFP_CONTRACT_INDEX, 3, input, output, user, mintFee);

        return output;
    }

    PFP::mintOfDrop_output mintOfDrop(const id& user, uint32 collectionId, Array<uint8, 64> URI, uint64 mintOfDropFee)
    {
        PFP::mintOfDrop_input input;
        PFP::mintOfDrop_output output;

        input.collectionId = collectionId;
        
        for(uint32 i = 0; i < 64; i++)
        {
            input.URI.set(i, URI.get(i));
        }

        invokeUserProcedure(PFP_CONTRACT_INDEX, 4, input, output, user, mintOfDropFee);

        return output;
    }

    PFP::transfer_output transfer(const id& user, uint32 NFTid, id receiver, uint64 transferFee)
    {
        PFP::transfer_input input;
        PFP::transfer_output output;

        input.NFTid = NFTid;
        input.receiver = receiver;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 5, input, output, user, transferFee);

        return output;
    }

    PFP::listInMarket_output listInMarket(const id& user, uint64 price, uint32 NFTid)
    {
        PFP::listInMarket_input input;
        PFP::listInMarket_output output;

        input.NFTid = NFTid;
        input.price = price;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 6, input, output, user, 0);

        return output;
    }

    PFP::sell_output sell(const id& user, uint32 NFTid, bit methodOfPayment, uint64 salePrice)
    {
        PFP::sell_input input;
        PFP::sell_output output;

        input.methodOfPayment = methodOfPayment;
        input.NFTid = NFTid;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 7, input, output, user, salePrice);

        return output;
    }

    PFP::cancelSale_output cancelSale(const id& user, uint32 NFTid)
    {
        PFP::cancelSale_input input;
        PFP::cancelSale_output output;

        input.NFTid = NFTid;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 8, input, output, user, 0);

        return output;
    }

    PFP::listInExchange_output listInExchange(const id& user, uint32 possessedNFT, uint32 anotherNFT)
    {
        PFP::listInExchange_input input;
        PFP::listInExchange_output output;

        input.anotherNFT = anotherNFT;
        input.possessedNFT = possessedNFT;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 9, input, output, user, 0);

        return output;
    }

    PFP::cancelExchange_output cancelExchange(const id& user, uint32 possessedNFT)
    {
        PFP::cancelExchange_input input;
        PFP::cancelExchange_output output;

        input.possessedNFT = possessedNFT;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 10, input, output, user, 0);

        return output;
    }

    PFP::makeOffer_output makeOffer(const id& user, sint64 askPrice, uint32 NFTid, bit paymentMethod)
    {
        PFP::makeOffer_input input;
        PFP::makeOffer_output output;

        input.askPrice = askPrice;
        input.NFTid = NFTid;
        input.paymentMethod = paymentMethod;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 11, input, output, user, askPrice);

        return output;
    }

    PFP::acceptOffer_output acceptOffer(const id& user, uint32 NFTid)
    {
        PFP::acceptOffer_input input;
        PFP::acceptOffer_output output;

        input.NFTid = NFTid;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 12, input, output, user, 0);

        return output;
    }

    PFP::cancelOffer_output cancelOffer(const id& user, uint32 NFTid)
    {
        PFP::cancelOffer_input input;
        PFP::cancelOffer_output output;

        input.NFTid = NFTid;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 13, input, output, user, 0);

        return output;
    }

    PFP::createTraditionalAuction_output createTraditionalAuction(const id& user, uint64 minPrice, uint32 NFTId, bit paymentMethodOfAuction, uint32 startYear, uint32 startMonth, uint32 startDay, uint32 startHour, uint32 endYear, uint32 endMonth, uint32 endDay, uint32 endHour)
    {
        PFP::createTraditionalAuction_input input;
        PFP::createTraditionalAuction_output output;

        input.startYear = startYear;
        input.startMonth = startMonth;
        input.startDay = startDay;
        input.startHour = startHour;
        input.endYear = startYear;
        input.endMonth = endMonth;
        input.endDay = endDay;
        input.endHour = endHour;
        input.minPrice = minPrice;
        input.NFTId = NFTId;
        input.paymentMethodOfAuction = paymentMethodOfAuction;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 14, input, output, user, 0);

        return output;
    }

    PFP::bidOnTraditionalAuction_output bidOnTraditionalAuction(const id& user, uint64 price, uint32 NFTId, bit paymentMethod)
    {
        PFP::bidOnTraditionalAuction_input input;
        PFP::bidOnTraditionalAuction_output output;

        input.NFTId = NFTId;
        input.paymentMethod = paymentMethod;
        input.price = price;

        invokeUserProcedure(PFP_CONTRACT_INDEX, 15, input, output, user, price);

        return output;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, PFP_ISSUE_ASSET_FEE);
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

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, issuer, PFP_TOKEN_TRANSFER_FEE);

        return output.transferredNumberOfShares;
    }

    sint64 TransferShareManagementRights(const id& issuer, uint64 assetName, uint32 newManagingContractIndex, sint64 numberOfShares, id currentOwner)
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
};

TEST(TestContractPFP, testingAllProceduresAndFunctions)
{
    ContractTestingPFP pfp;

    uint64 cfbPrice = random(1, 1000000);
    uint64 qubicPrice = random(1, 1000000);

    increaseEnergy(MARKETPLACE_OWNER, 1);

    pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, cfbPrice, qubicPrice);
    pfp.getState()->cfbAndQubicPriceChecker(cfbPrice, qubicPrice);

    uint64 assetName = assetNameFromString("CFB");

    increaseEnergy(CFB_ISSUER, PFP_ISSUE_ASSET_FEE);
    EXPECT_EQ(pfp.issueAsset(CFB_ISSUER, assetName, PFP_CREATED_CFB_AMOUNT, 0, 0), PFP_CREATED_CFB_AMOUNT);

    auto users = getRandomUsers(10000, 10000);

    increaseEnergy(CFB_ISSUER, PFP_TOKEN_TRANSFER_FEE);
    // EXPECT_EQ(users[0], users[1]);
    increaseEnergy(users[0], PFP_TOKEN_TRANSFER_FEE);
    pfp.TransferShareOwnershipAndPossession(CFB_ISSUER, assetName, 10000000000, users[0]);

    Array<uint8, 64> URI;
    for(uint32 i = 0 ; i < 64; i++)
    {
        URI.set(i, getRandomURI().get(i));
    }

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, PFP_CFB_NAME, PFP_CONTRACT_INDEX, cfbPrice * PFP_FEE_COLLECTION_CREATE_9001_10000, users[0]), cfbPrice * PFP_FEE_COLLECTION_CREATE_9001_10000);
    
    pfp.createCollection(users[0], 0, 10, 10, 100, 1, URI);

    EXPECT_EQ(numberOfPossessedShares(assetName, CFB_ISSUER, id(11, 0, 0, 0), id(11, 0, 0, 0), PFP_CONTRACT_INDEX, PFP_CONTRACT_INDEX), cfbPrice * PFP_FEE_COLLECTION_CREATE_9001_10000);

    pfp.getState()->createCollectionChecker(users[0], 0, 10000, 10, 100, 1, 1, URI);
}