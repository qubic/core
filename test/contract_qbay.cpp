#define NO_UEFI

#include <map>
#include <random>

#include "contract_testing.h"

const id MARKETPLACE_OWNER = ID(_R, _K, _D, _H, _C, _M, _R, _J, _Y, _C, _G, _K, _P, _D, _U, _Y, _R, _X, _G, _D, _Y, _Z, _C, _I, _Z, _I, _T, _A, _H, _Y, _O, _V, _G, _I, _U, _T, _K, _N, _D, _T, _E, _H, _P, _C, _C, _L, _W, _L, _Z, _X, _S, _H, _N, _F, _P, _D);
const id CFB_ISSUER = ID(_C, _F, _B, _M, _E, _M, _Z, _O, _I, _D, _E, _X, _Q, _A, _U, _X, _Y, _Y, _S, _Z, _I, _U, _R, _A, _D, _Q, _L, _A, _P, _W, _P, _M, _N, _J, _X, _Q, _S, _N, _V, _Q, _Z, _A, _H, _Y, _V, _O, _P, _Y, _U, _K, _K, _J, _B, _J, _U, _C);
static constexpr uint64 QBAY_ISSUE_ASSET_FEE = 1000000000ULL;
static constexpr uint64 QBAY_TOKEN_TRANSFER_FEE = 1000000ULL;
static constexpr sint64 QBAY_CREATED_CFB_AMOUNT = 1000000000000ULL;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_2_200 = 100;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_201_1000 = 200;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_1001_2000 = 400;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_2001_3000 = 600;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_3001_4000 = 800;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_4001_5000 = 1000;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_5001_6000 = 1200;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_6001_7000 = 1400;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_7001_8000 = 1600;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_8001_9000 = 1800;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_9001_10000 = 2000;

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

class QBAYChecker : public QBAY
{
public:

    void stateVriableChecker(uint64 gt_priceOfCFB, uint64 gt_priceOfQubic, uint64 gt_numberOfNFTIncoming, uint32 gt_numberOfCollection, uint32 gt_numberOfNFT, bit gt_statusOfMarketPlace)
    {
        EXPECT_EQ(gt_priceOfCFB, priceOfCFB);
        EXPECT_EQ(gt_priceOfQubic, priceOfQubic);
        EXPECT_EQ(gt_numberOfNFTIncoming, numberOfNFTIncoming);
        EXPECT_EQ(gt_numberOfCollection, numberOfCollection);
        EXPECT_EQ(gt_numberOfNFT, numberOfNFT);
        EXPECT_EQ(gt_statusOfMarketPlace, statusOfMarketPlace);
    }

    void createCollectionChecker(id user, uint64 priceForDropMint, uint32 countOfNFT, uint32 royalty, uint32 maxSizePerOneId, bit typeOfCollection, uint32 countOfCollection, Array<uint8, 64>& URI)
    {
        EXPECT_EQ(Collections.get(countOfCollection - 1).creator, user);
        EXPECT_EQ(Collections.get(countOfCollection - 1).currentSize, countOfNFT);
        EXPECT_EQ(Collections.get(countOfCollection - 1).maxSizeHoldingPerOneId, maxSizePerOneId);
        EXPECT_EQ(Collections.get(countOfCollection - 1).priceForDropMint, priceForDropMint);
        EXPECT_EQ(Collections.get(countOfCollection - 1).royalty, royalty);
        EXPECT_EQ(Collections.get(countOfCollection - 1).typeOfCollection, typeOfCollection);
        for(uint8 i = 0; i < 64; i++)
        {
            if(i >= QBAY_LENGTH_OF_URI)
            {
                EXPECT_EQ(Collections.get(countOfCollection - 1).URI.get(i), 0);
            }
            else {
                EXPECT_EQ(Collections.get(countOfCollection - 1).URI.get(i), URI.get(i));
            } 
        }
    }

    void mintChecker(id user, uint32 royalty, uint32 collectionId, Array<uint8, 64> URI, bit typeOfMint, uint32 idOfNFT)
    {
        EXPECT_EQ(NFTs.get(idOfNFT).creator, user);
        EXPECT_EQ(NFTs.get(idOfNFT).possessor, user);
        if(typeOfMint == 0)
        {
            EXPECT_EQ(NFTs.get(idOfNFT).royalty, Collections.get(collectionId).royalty);
        }
        else {
            EXPECT_EQ(NFTs.get(idOfNFT).royalty, royalty);
        }
        for(uint8 i = 0; i < 64; i++)
        {
            if(i >= QBAY_LENGTH_OF_URI)
            {
                EXPECT_EQ(NFTs.get(idOfNFT).URI.get(i), 0);
            }
            else {
                EXPECT_EQ(NFTs.get(idOfNFT).URI.get(i), URI.get(i));
            } 
        }
    }

    void transferChecker(id newUser, uint32 NFTId)
    {
        EXPECT_EQ(NFTs.get(NFTId).possessor, newUser);
    }

    void listInMarketChecker(uint32 NFTId, uint64 price)
    {
        EXPECT_EQ(NFTs.get(NFTId).statusOfSale, 1);
        EXPECT_EQ(NFTs.get(NFTId).salePrice, price);
    }

    void buyChecker(id oldPossesor, id newPossessor, uint32 NFTId, uint64 price, bit typfOfPayment, uint64 initialBalanceOfCreator, uint64 initialBalanceOfPossesor, uint64 initialBalanceOfMarket, bit isSameCreatorAndPossesor)
    {
        EXPECT_EQ(NFTs.get(NFTId).possessor, newPossessor);
        if(typfOfPayment == 0)
        {
            if(isSameCreatorAndPossesor == 1)
            {
                EXPECT_EQ(initialBalanceOfPossesor + price - div(price * 25ULL, 1000ULL), getBalance(oldPossesor));
                EXPECT_EQ(initialBalanceOfMarket + div(price * 25ULL, 1000ULL), getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)));
            }
            else {
                EXPECT_EQ(initialBalanceOfCreator + div(price * NFTs.get(NFTId).royalty * 1ULL, 100ULL), getBalance(NFTs.get(NFTId).creator));
                EXPECT_EQ(initialBalanceOfPossesor + price - div(price * (NFTs.get(NFTId).royalty * 10 + 30) * 1ULL, 1000ULL), getBalance(oldPossesor));
            }
        }
        else 
        {
            if(isSameCreatorAndPossesor == 1)
            {
                EXPECT_EQ(initialBalanceOfPossesor + price - div(price * 25ULL, 1000ULL), numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, oldPossesor, oldPossesor, QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, oldPossesor, oldPossesor, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
                EXPECT_EQ(initialBalanceOfMarket + div(price * 25ULL, 1000ULL), numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
            }
            else 
            {
                EXPECT_EQ(initialBalanceOfCreator + div(price * NFTs.get(NFTId).royalty * 1ULL, 100ULL), numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, NFTs.get(NFTId).creator, NFTs.get(NFTId).creator, QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, NFTs.get(NFTId).creator, NFTs.get(NFTId).creator, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
                EXPECT_EQ(initialBalanceOfPossesor + price - div(price * NFTs.get(NFTId).royalty * 1ULL, 100ULL) - div(price * 20ULL, 1000ULL), numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, oldPossesor, oldPossesor, QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, oldPossesor, oldPossesor, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
                EXPECT_EQ(initialBalanceOfMarket + div(price * 20ULL, 1000ULL), numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
            }
        }
    }
    
    void cancelSaleChecker(uint32 NFTId)
    {
        EXPECT_EQ(NFTs.get(NFTId).salePrice, QBAY_SALE_PRICE);
        EXPECT_EQ(NFTs.get(NFTId).statusOfSale, 0);
    }

    void listInExchangeChecker(id user, uint32 possessedNFT, uint32 anotherNFT)
    {
        EXPECT_EQ(NFTs.get(anotherNFT).NFTidForExchange, possessedNFT);
        EXPECT_EQ(NFTs.get(anotherNFT).statusOfExchange, 1);
    }

    void possesorChecker(id user, uint32 possessedNFT)
    {
        EXPECT_EQ(NFTs.get(possessedNFT).possessor, user);
    }

    void cancelExchangeChecker(uint32 NFTId)
    {
        EXPECT_EQ(NFTs.get(NFTId).NFTidForExchange, QBAY_MAX_NUMBER_NFT);
        EXPECT_EQ(NFTs.get(NFTId).statusOfExchange, 0);
    }

    void makeOfferChecker(uint32 NFTId, id user, bit paymentMethod, uint64 askPrice)
    {
        EXPECT_EQ(NFTs.get(NFTId).askUser, user);
        EXPECT_EQ(NFTs.get(NFTId).paymentMethodOfAsk, paymentMethod);
        EXPECT_EQ(NFTs.get(NFTId).askMaxPrice, askPrice);
    }

    void acceptOfferChecker(id user, uint32 NFTId)
    {
        EXPECT_LT(NFTs.get(NFTId).askUser, user);
        EXPECT_EQ(NFTs.get(NFTId).askMaxPrice, 0);
        EXPECT_EQ(NFTs.get(NFTId).statusOfAsk, 0);
    }

    void cancelOfferChecker(uint32 NFTId)
    {
        EXPECT_EQ(NFTs.get(NFTId).askUser, NULL_ID);
        EXPECT_EQ(NFTs.get(NFTId).askMaxPrice, 0);
    }

    void createAuctionChecker(uint32 NFTId, uint64 price, bit gt_statusOfAuction, bit gt_paymentMethodOfAuction, id gt_creatorOfAuction, uint32 startYear, uint32 startMonth, uint32 startDay, uint32 startHour, uint32 endYear, uint32 endMonth, uint32 endDay, uint32 endHour)
    {
        uint32 startTime, endTime;
        QUOTTERY::packQuotteryDate(startYear, startMonth, startDay, startHour, 0, 0, startTime);
        QUOTTERY::packQuotteryDate(endYear, endMonth, endDay, endHour, 0, 0, endTime);
        EXPECT_EQ(NFTs.get(NFTId).startTimeOfAuction, startTime);
        EXPECT_EQ(NFTs.get(NFTId).endTimeOfAuction, endTime);
        EXPECT_EQ(NFTs.get(NFTId).currentPriceOfAuction, price);
        EXPECT_EQ(NFTs.get(NFTId).statusOfAuction, gt_statusOfAuction);
        EXPECT_EQ(NFTs.get(NFTId).paymentMethodOfAuction, gt_paymentMethodOfAuction);
        EXPECT_EQ(NFTs.get(NFTId).creatorOfAuction, gt_creatorOfAuction);
    }

    void bidOnAuctionChecker(id user, uint32 NFTId, uint64 price)
    {
        EXPECT_EQ(NFTs.get(NFTId).possessor, user);
        EXPECT_EQ(NFTs.get(NFTId).currentPriceOfAuction, price);
        EXPECT_EQ(NFTs.get(NFTId).statusOfAuction, 2);
    }

    void profitChecker(uint64 amountOfQubic, uint64 amountOfCFB)
    {
        EXPECT_EQ(amountOfQubic, earnedQubic);
        EXPECT_EQ(amountOfCFB, earnedCFB);
    }

    void getNumberOfNFTForUserChecker(getNumberOfNFTForUser_output output, uint32 numberOfNFT)
    {
        EXPECT_EQ(output.numberOfNFT, numberOfNFT);
    }

    void getInfoOfNFTUserPossessedChecker(getInfoOfNFTUserPossessed_output output, uint32 idOfNFT)
    {
        EXPECT_EQ(output.creator, NFTs.get(idOfNFT).creator);
        EXPECT_EQ(output.possessor, NFTs.get(idOfNFT).possessor);
        EXPECT_EQ(output.askMaxPrice, NFTs.get(idOfNFT).askMaxPrice);
        EXPECT_EQ(output.askUser, NFTs.get(idOfNFT).askUser);
        EXPECT_EQ(output.creatorOfAuction, NFTs.get(idOfNFT).creatorOfAuction);
        EXPECT_EQ(output.currentPriceOfAuction, NFTs.get(idOfNFT).currentPriceOfAuction);
        EXPECT_EQ(output.statusOfSale, NFTs.get(idOfNFT).statusOfSale);
        EXPECT_EQ(output.statusOfAsk, NFTs.get(idOfNFT).statusOfAsk);
        EXPECT_EQ(output.statusOfAuction, NFTs.get(idOfNFT).statusOfAuction);
        EXPECT_EQ(output.statusOfExchange, NFTs.get(idOfNFT).statusOfExchange);
        EXPECT_EQ(output.royalty, NFTs.get(idOfNFT).royalty);
        EXPECT_EQ(output.salePrice, NFTs.get(idOfNFT).salePrice);
        EXPECT_EQ(output.NFTidForExchange, NFTs.get(idOfNFT).NFTidForExchange);
        EXPECT_EQ(output.paymentMethodOfAsk, NFTs.get(idOfNFT).paymentMethodOfAsk);
        for(uint32 i = 0 ; i < 64; i++)
        {
            EXPECT_EQ(output.URI.get(i), NFTs.get(idOfNFT).URI.get(i));
        }
    }

    void getInfoOfMarketplaceChecker(getInfoOfMarketplace_output output)
    {
        EXPECT_EQ(output.earnedCFB, earnedCFB);
        EXPECT_EQ(output.earnedQubic, earnedQubic);
        EXPECT_EQ(output.numberOfCollection, numberOfCollection);
        EXPECT_EQ(output.numberOfNFT, numberOfNFT);
        EXPECT_EQ(output.numberOfNFTIncoming, numberOfNFTIncoming);
        EXPECT_EQ(output.priceOfCFB, priceOfCFB);
        EXPECT_EQ(output.priceOfQubic, priceOfQubic);
        EXPECT_EQ(output.statusOfMarketPlace, statusOfMarketPlace);
    }

    void getInfoOfCollectionByCreatorChecker(getInfoOfCollectionByCreator_output output, uint32 idOfCollection)
    {
        EXPECT_EQ(output.currentSize, Collections.get(idOfCollection).currentSize);
        EXPECT_EQ(output.idOfCollection, idOfCollection);
        EXPECT_EQ(output.maxSizeHoldingPerOneId, Collections.get(idOfCollection).maxSizeHoldingPerOneId);
        EXPECT_EQ(output.priceForDropMint, Collections.get(idOfCollection).priceForDropMint);
        EXPECT_EQ(output.royalty, Collections.get(idOfCollection).royalty);
        EXPECT_EQ(output.typeOfCollection, Collections.get(idOfCollection).typeOfCollection);
        for(uint32 i = 0 ; i < 64; i++)
        {
            EXPECT_EQ(output.URI.get(i), Collections.get(idOfCollection).URI.get(i));
        }
    }

    void getInfoOfCollectionByIdChecker(getInfoOfCollectionById_output output, uint32 idOfCollection)
    {
        EXPECT_EQ(output.creator, Collections.get(idOfCollection).creator);
        EXPECT_EQ(output.currentSize, Collections.get(idOfCollection).currentSize);
        EXPECT_EQ(output.maxSizeHoldingPerOneId, Collections.get(idOfCollection).maxSizeHoldingPerOneId);
        EXPECT_EQ(output.priceForDropMint, Collections.get(idOfCollection).priceForDropMint);
        EXPECT_EQ(output.royalty, Collections.get(idOfCollection).royalty);
        EXPECT_EQ(output.typeOfCollection, Collections.get(idOfCollection).typeOfCollection);
        for(uint32 i = 0 ; i < 64; i++)
        {
            EXPECT_EQ(output.URI.get(i), Collections.get(idOfCollection).URI.get(i));
        }
    }

    void getIncomingAuctionsChecker(getIncomingAuctions_output output, uint32 offset, uint32 count)
    {
        for(uint32 i = offset, k = 0; i < count; i++, k++)
        {
            EXPECT_EQ(NFTs.get(output.NFTId.get(k)).statusOfAuction, 1);
        }
    }

    void getInfoOfNFTByIdChecker(getInfoOfNFTById_output output, uint32 idOfNFT)
    {
        EXPECT_EQ(output.creator, NFTs.get(idOfNFT).creator);
        EXPECT_EQ(output.possessor, NFTs.get(idOfNFT).possessor);
        EXPECT_EQ(output.askMaxPrice, NFTs.get(idOfNFT).askMaxPrice);
        EXPECT_EQ(output.askUser, NFTs.get(idOfNFT).askUser);
        EXPECT_EQ(output.creatorOfAuction, NFTs.get(idOfNFT).creatorOfAuction);
        EXPECT_EQ(output.currentPriceOfAuction, NFTs.get(idOfNFT).currentPriceOfAuction);
        EXPECT_EQ(output.statusOfSale, NFTs.get(idOfNFT).statusOfSale);
        EXPECT_EQ(output.statusOfAsk, NFTs.get(idOfNFT).statusOfAsk);
        EXPECT_EQ(output.statusOfAuction, NFTs.get(idOfNFT).statusOfAuction);
        EXPECT_EQ(output.statusOfExchange, NFTs.get(idOfNFT).statusOfExchange);
        EXPECT_EQ(output.royalty, NFTs.get(idOfNFT).royalty);
        EXPECT_EQ(output.salePrice, NFTs.get(idOfNFT).salePrice);
        EXPECT_EQ(output.NFTidForExchange, NFTs.get(idOfNFT).NFTidForExchange);
        EXPECT_EQ(output.paymentMethodOfAsk, NFTs.get(idOfNFT).paymentMethodOfAsk);
        for(uint32 i = 0 ; i < 64; i++)
        {
            EXPECT_EQ(output.URI.get(i), NFTs.get(idOfNFT).URI.get(i));
        }
    }

    uint64 getDropMintPrice(uint32 collectionId)
    {
        return Collections.get(collectionId).priceForDropMint;
    }

    id getCreatorOfNFT(uint32 NFTId)
    {
        return NFTs.get(NFTId).creator;
    }

    id getPossessorOfNFT(uint32 NFTId)
    {
        return NFTs.get(NFTId).possessor;
    }
    
};

class ContractTestingQBAY : protected ContractTesting
{
public:
    ContractTestingQBAY()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QBAY);
        callSystemProcedure(QBAY_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    ~ContractTestingQBAY()
    {
    }

    QBAYChecker* getState()
    {
        return (QBAYChecker*)contractStates[QBAY_CONTRACT_INDEX];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QBAY_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    QBAY::getNumberOfNFTForUser_output getNumberOfNFTForUser(id user) const
    {
        QBAY::getNumberOfNFTForUser_input input;
        QBAY::getNumberOfNFTForUser_output output;

        input.user = user;

        callFunction(QBAY_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    QBAY::getInfoOfNFTUserPossessed_output getInfoOfNFTUserPossessed(id user, uint32 NFTNumber) const
    {
        QBAY::getInfoOfNFTUserPossessed_input input;
        QBAY::getInfoOfNFTUserPossessed_output output;

        input.user = user;
        input.NFTNumber = NFTNumber;

        callFunction(QBAY_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QBAY::getInfoOfMarketplace_output getInfoOfMarketplace() const
    {
        QBAY::getInfoOfMarketplace_input input;
        QBAY::getInfoOfMarketplace_output output;

        callFunction(QBAY_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QBAY::getInfoOfCollectionByCreator_output getInfoOfCollectionByCreator(id creator, uint32 orderOfCollection) const
    {
        QBAY::getInfoOfCollectionByCreator_input input;
        QBAY::getInfoOfCollectionByCreator_output output;

        input.creator = creator;
        input.orderOfCollection = orderOfCollection;

        callFunction(QBAY_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QBAY::getInfoOfCollectionById_output getInfoOfCollectionById(uint32 idOfCollection) const
    {
        QBAY::getInfoOfCollectionById_input input;
        QBAY::getInfoOfCollectionById_output output;

        input.idOfCollection = idOfCollection;

        callFunction(QBAY_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QBAY::getIncomingAuctions_output getIncomingAuctions(uint32 offset, uint32 count) const
    {
        QBAY::getIncomingAuctions_input input;
        QBAY::getIncomingAuctions_output output;

        input.count = count;
        input.offset = offset;

        callFunction(QBAY_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QBAY::getInfoOfNFTById_output getInfoOfNFTById(uint32 NFTId) const
    {
        QBAY::getInfoOfNFTById_input input;
        QBAY::getInfoOfNFTById_output output;

        input.NFTId = NFTId;

        callFunction(QBAY_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QBAY::getUserCreatedCollection_output getUserCreatedCollection(id user, uint32 offset, uint32 count) const
    {
        QBAY::getUserCreatedCollection_input input;
        QBAY::getUserCreatedCollection_output output;

        input.user = user;
        input.offset = offset;
        input.count = count;

        callFunction(QBAY_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    QBAY::getUserCreatedNFT_output getUserCreatedNFT(id user, uint32 offset, uint32 count) const
    {
        QBAY::getUserCreatedNFT_input input;
        QBAY::getUserCreatedNFT_output output;

        input.user = user;
        input.offset = offset;
        input.count = count;

        callFunction(QBAY_CONTRACT_INDEX, 9, input, output);
        return output;
    }

    QBAY::settingCFBAndQubicPrice_output settingCFBAndQubicPrice(const id& marketOwnerAdress, uint64 CFBPrice, uint64 QubicPrice)
    {
        QBAY::settingCFBAndQubicPrice_input input;
        QBAY::settingCFBAndQubicPrice_output output;

        input.CFBPrice = CFBPrice;
        input.QubicPrice = QubicPrice;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 1, input, output, marketOwnerAdress, 0);

        return output;
    }

    QBAY::createCollection_output createCollection(const id& user, uint64 priceForDropMint, uint32 volume, uint32 royalty, uint32 maxSizePerOneId, bit typeOfCollection, Array<uint8, 64>& URI)
    {
        QBAY::createCollection_input input;
        QBAY::createCollection_output output;

        input.maxSizePerOneId = maxSizePerOneId;
        input.priceForDropMint = priceForDropMint;
        input.volume = volume;
        input.royalty = royalty;
        input.typeOfCollection = typeOfCollection;

        for(uint32 i = 0 ; i < 64; i++) 
		{
			if(i >= QBAY_LENGTH_OF_URI) 
			{
				input.URI.set(i, 0);
			}
			else 
			{
				input.URI.set(i, URI.get(i));
			}
		}

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 2, input, output, user, 0);

        return output;
    }

    QBAY::mint_output mint(const id& user, uint32 royalty, uint32 collectionId, Array<uint8, 64> URI, bit typeOfMint, uint64 mintFee)
    {
        QBAY::mint_input input;
        QBAY::mint_output output;

        input.collectionId = collectionId;
        input.royalty = royalty;
        input.typeOfMint = typeOfMint;

        for(uint32 i = 0 ; i < 64; i++)
        {
            input.URI.set(i, URI.get(i));
        }

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 3, input, output, user, mintFee);

        return output;
    }

    QBAY::mintOfDrop_output mintOfDrop(const id& user, uint32 collectionId, Array<uint8, 64> URI, uint64 mintOfDropFee)
    {
        QBAY::mintOfDrop_input input;
        QBAY::mintOfDrop_output output;

        input.collectionId = collectionId;
        
        for(uint32 i = 0; i < 64; i++)
        {
            input.URI.set(i, URI.get(i));
        }

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 4, input, output, user, mintOfDropFee);

        return output;
    }

    QBAY::transfer_output transfer(const id& user, uint32 NFTid, id receiver, uint64 transferFee)
    {
        QBAY::transfer_input input;
        QBAY::transfer_output output;

        input.NFTid = NFTid;
        input.receiver = receiver;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, input, output, user, transferFee);

        return output;
    }

    QBAY::listInMarket_output listInMarket(const id& user, uint64 price, uint32 NFTid)
    {
        QBAY::listInMarket_input input;
        QBAY::listInMarket_output output;

        input.NFTid = NFTid;
        input.price = price;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 6, input, output, user, 0);

        return output;
    }

    QBAY::buy_output buy(const id& user, uint32 NFTid, bit methodOfPayment, uint64 salePrice)
    {
        QBAY::buy_input input;
        QBAY::buy_output output;

        input.methodOfPayment = methodOfPayment;
        input.NFTid = NFTid;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 7, input, output, user, salePrice);

        return output;
    }

    QBAY::cancelSale_output cancelSale(const id& user, uint32 NFTid)
    {
        QBAY::cancelSale_input input;
        QBAY::cancelSale_output output;

        input.NFTid = NFTid;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 8, input, output, user, 0);

        return output;
    }

    QBAY::listInExchange_output listInExchange(const id& user, uint32 possessedNFT, uint32 anotherNFT)
    {
        QBAY::listInExchange_input input;
        QBAY::listInExchange_output output;

        input.anotherNFT = anotherNFT;
        input.possessedNFT = possessedNFT;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 9, input, output, user, 0);

        return output;
    }

    QBAY::cancelExchange_output cancelExchange(const id& user, uint32 possessedNFT, uint32 anotherNFT)
    {
        QBAY::cancelExchange_input input;
        QBAY::cancelExchange_output output;

        input.possessedNFT = possessedNFT;
        input.anotherNFT = anotherNFT;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 10, input, output, user, 0);

        return output;
    }

    QBAY::makeOffer_output makeOffer(const id& user, sint64 askPrice, uint32 NFTid, bit paymentMethod)
    {
        QBAY::makeOffer_input input;
        QBAY::makeOffer_output output;

        input.askPrice = askPrice;
        input.NFTid = NFTid;
        input.paymentMethod = paymentMethod;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 11, input, output, user, paymentMethod == 0?askPrice:0);

        return output;
    }

    QBAY::acceptOffer_output acceptOffer(const id& user, uint32 NFTid)
    {
        QBAY::acceptOffer_input input;
        QBAY::acceptOffer_output output;

        input.NFTid = NFTid;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 12, input, output, user, 0);

        return output;
    }

    QBAY::cancelOffer_output cancelOffer(const id& user, uint32 NFTid)
    {
        QBAY::cancelOffer_input input;
        QBAY::cancelOffer_output output;

        input.NFTid = NFTid;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 13, input, output, user, 0);

        return output;
    }

    QBAY::createTraditionalAuction_output createTraditionalAuction(const id& user, uint64 minPrice, uint32 NFTId, bit paymentMethodOfAuction, uint32 startYear, uint32 startMonth, uint32 startDay, uint32 startHour, uint32 endYear, uint32 endMonth, uint32 endDay, uint32 endHour)
    {
        QBAY::createTraditionalAuction_input input;
        QBAY::createTraditionalAuction_output output;

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

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 14, input, output, user, 0);

        return output;
    }

    QBAY::bidOnTraditionalAuction_output bidOnTraditionalAuction(const id& user, uint64 price, uint32 NFTId, bit paymentMethod)
    {
        QBAY::bidOnTraditionalAuction_input input;
        QBAY::bidOnTraditionalAuction_output output;

        input.NFTId = NFTId;
        input.paymentMethod = paymentMethod;
        input.price = price;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 15, input, output, user, price);

        return output;
    }

    QBAY::changeStatusOfMarketPlace_output changeStatusOfMarketPlace(const id& user, bit status)
    {
        QBAY::changeStatusOfMarketPlace_input input;
        QBAY::changeStatusOfMarketPlace_output output;

        input.status = status;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 17, input, output, user, 0);

        return output;
    }

    QBAY::TransferShareManagementRights_output qbayTransferShareManagementRights(const id& user, sint64 numberOfShares, uint32 newManagingContractIndex, uint64 fee)
    {
        QBAY::TransferShareManagementRights_input input;
        QBAY::TransferShareManagementRights_output output;

        input.asset.assetName = QBAY_CFB_NAME;
        input.asset.issuer = CFB_ISSUER;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 16, input, output, user, fee);

        return output;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QBAY_ISSUE_ASSET_FEE);
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

        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, issuer, QBAY_TOKEN_TRANSFER_FEE);

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

TEST(TestContractQBAY, testingAllProceduresAndFunctions)
{
    ContractTestingQBAY pfp;

    uint64 cfbPrice = random(1, 1000000);
    uint64 qubicPrice = random(1, 1000000);
    uint64 earnedQubic = 0;
    uint64 earnedCFB = 0;
    uint64 collectedShareHolderFee = 0;
    uint32 totalIncommingNFTNumber = 0;
    uint32 totalPriceForCollectionCreating = 0;
    uint32 numberOfCollectionCreated = 0;
    uint32 numberOfNFTCreated = 0;

    increaseEnergy(MARKETPLACE_OWNER, 1);

    pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, cfbPrice, qubicPrice);
    pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 1);
    pfp.getState()->stateVriableChecker(cfbPrice, qubicPrice, totalIncommingNFTNumber, numberOfCollectionCreated, numberOfNFTCreated, 1);

    uint64 assetName = assetNameFromString("CFB");

    increaseEnergy(CFB_ISSUER, QBAY_ISSUE_ASSET_FEE);
    EXPECT_EQ(pfp.issueAsset(CFB_ISSUER, assetName, QBAY_CREATED_CFB_AMOUNT, 0, 0), QBAY_CREATED_CFB_AMOUNT);

    auto users = getRandomUsers(10000, 10000);

    increaseEnergy(CFB_ISSUER, QBAY_TOKEN_TRANSFER_FEE);
    // EXPECT_EQ(users[0], users[1]);
    increaseEnergy(users[0], QBAY_TOKEN_TRANSFER_FEE);
    pfp.TransferShareOwnershipAndPossession(CFB_ISSUER, assetName, 10000000000, users[0]);

    Array<uint8, 64> URI;
    for(uint32 i = 0 ; i < 64; i++)
    {
        URI.set(i, getRandomURI().get(i));
    }
    
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_9001_10000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_9001_10000);
    pfp.createCollection(users[0], 0, 10, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 10000, 10, 100, 1, 1, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_9001_10000;
    totalIncommingNFTNumber += 10000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_9001_10000 * cfbPrice;
    
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_8001_9000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_8001_9000);
    pfp.createCollection(users[0], 0, 9, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 9000, 10, 100, 1, 2, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_8001_9000;
    totalIncommingNFTNumber += 9000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_8001_9000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_7001_8000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_7001_8000);
    pfp.createCollection(users[0], 0, 8, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 8000, 10, 100, 1, 3, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_7001_8000;
    totalIncommingNFTNumber += 8000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_7001_8000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_6001_7000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_6001_7000);
    pfp.createCollection(users[0], 0, 7, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 7000, 10, 100, 1, 4, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_6001_7000;
    totalIncommingNFTNumber += 7000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_6001_7000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_5001_6000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_5001_6000);
    pfp.createCollection(users[0], 0, 6, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 6000, 10, 100, 1, 5, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_5001_6000;
    totalIncommingNFTNumber += 6000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_5001_6000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_4001_5000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_4001_5000);
    pfp.createCollection(users[0], 0, 5, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 5000, 10, 100, 1, 6, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_4001_5000;
    totalIncommingNFTNumber += 5000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_4001_5000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_3001_4000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_3001_4000);
    pfp.createCollection(users[0], 0, 4, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 4000, 10, 100, 1, 7, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_3001_4000;
    totalIncommingNFTNumber += 4000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_3001_4000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_2001_3000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_2001_3000);
    pfp.createCollection(users[0], 0, 3, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 3000, 10, 100, 1, 8, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_2001_3000;
    totalIncommingNFTNumber += 3000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_2001_3000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_1001_2000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_1001_2000);
    pfp.createCollection(users[0], 0, 2, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 2000, 10, 100, 1, 9, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_1001_2000;
    totalIncommingNFTNumber += 2000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_1001_2000 * cfbPrice;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_201_1000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_201_1000);
    pfp.createCollection(users[0], 0, 1, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 1000, 10, 100, 1, 10, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_201_1000;
    totalIncommingNFTNumber += 1000;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_201_1000 * cfbPrice;

    // Collection for Drop. collection id: 10
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_2_200, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_2_200);
    pfp.createCollection(users[0], 0, 0, 10, 100, 0, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 200, 10, 100, 0, 11, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_2_200;
    totalIncommingNFTNumber += 200;
    numberOfCollectionCreated++;
    earnedCFB += QBAY_FEE_COLLECTION_CREATE_2_200 * cfbPrice;

    // getting the id of collection user created

    auto getUserCreatedCollection_output = pfp.getUserCreatedCollection(users[0], 0, 10);
    for(uint32 i = 0 ; i < 11; i++)
    {
        EXPECT_EQ(getUserCreatedCollection_output.collectionId.get(i), 10 - i);
    }

    // checking the infos of collection by creator
    auto getInfoOfCollectionByCreator_output = pfp.getInfoOfCollectionByCreator(users[0], 1);
    pfp.getState()->getInfoOfCollectionByCreatorChecker(getInfoOfCollectionByCreator_output, 0);

    // checking the infos of collection by id
    auto getInfoOfCollectionById_output = pfp.getInfoOfCollectionById(0);
    pfp.getState()->getInfoOfCollectionByIdChecker(getInfoOfCollectionById_output, 0);

    EXPECT_EQ(numberOfPossessedShares(assetName, CFB_ISSUER, id(12, 0, 0, 0), id(12, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), cfbPrice * totalPriceForCollectionCreating);

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic);
    // mint the NFT using collection

    pfp.mint(users[0], 0, 0, URI, 0, 0);
    numberOfNFTCreated++;
    pfp.getState()->mintChecker(users[0], 0, 0, URI, 0, numberOfNFTCreated - 1);
    pfp.getState()->stateVriableChecker(cfbPrice, qubicPrice, totalIncommingNFTNumber, numberOfCollectionCreated, numberOfNFTCreated, 1);

    // mint the single NFT
    increaseEnergy(users[0], QBAY_SINGLE_NFT_CREATE_FEE);
    pfp.mint(users[0], 10, 0, URI, 1, QBAY_SINGLE_NFT_CREATE_FEE);
    numberOfNFTCreated++;
    totalIncommingNFTNumber++;
    earnedQubic += QBAY_SINGLE_NFT_CREATE_FEE;
    pfp.getState()->mintChecker(users[0], 10, 0, URI, 1, numberOfNFTCreated - 1);
    pfp.getState()->stateVriableChecker(cfbPrice, qubicPrice, totalIncommingNFTNumber, numberOfCollectionCreated, numberOfNFTCreated, 1);

    auto getUserCreatedNFT_output = pfp.getUserCreatedNFT(users[0], 0, 2);
    for(uint32 i = 0 ; i < 2; i++)
    {
        EXPECT_EQ(getUserCreatedNFT_output.NFTId.get(i), 1 - i);
    }

    // checking if 2 NFTs are minted by users[0]
    auto getNumberOfNFTForUser_output = pfp.getNumberOfNFTForUser(users[0]);
    pfp.getState()->getNumberOfNFTForUserChecker(getNumberOfNFTForUser_output, 2);

    // checking the info of users[0]'s first NFT
    auto getInfoOfNFTUserPossessed_output = pfp.getInfoOfNFTUserPossessed(users[0], 1);
    pfp.getState()->getInfoOfNFTUserPossessedChecker(getInfoOfNFTUserPossessed_output, 0);

    // checking the info of NFT by id

    auto getInfoOfNFTById_output = pfp.getInfoOfNFTById(0);
    pfp.getState()->getInfoOfNFTByIdChecker(getInfoOfNFTById_output, 0);


    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic);

    // dropMint
    increaseEnergy(users[0], pfp.getState()->getDropMintPrice(10));
    pfp.mintOfDrop(users[0], 10, URI, pfp.getState()->getDropMintPrice(10));
    numberOfNFTCreated++;
    pfp.getState()->mintChecker(users[0], 10, 10, URI, 0, numberOfNFTCreated - 1);

    // transfer NFT

    increaseEnergy(users[1], 1);
    pfp.transfer(users[0], 0, users[1], 0);
    pfp.getState()->transferChecker(users[1], 0);

    // listInMarket

    pfp.listInMarket(users[1], 100000, 0);
    pfp.getState()->listInMarketChecker(0, 100000);

    // buy with $Qubic
    // small price

    increaseEnergy(users[2], 100000);
    uint64 initialBalanceOfCreator = getBalance(pfp.getState()->getCreatorOfNFT(0));
    uint64 initialBalanceOfPossesor = getBalance(pfp.getState()->getPossessorOfNFT(0));
    uint64 initialBalanceOfMarket = getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0));
    id oldPossesor = pfp.getState()->getPossessorOfNFT(0);

    uint64 gt_marketFee = div(100000 * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
    uint64 gt_shareHolderFee = div(100000 * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

    pfp.buy(users[2], 0, 0, 100000);
    pfp.getState()->buyChecker(oldPossesor, users[2], 0, 100000, 0, initialBalanceOfCreator, initialBalanceOfPossesor, initialBalanceOfMarket, pfp.getState()->getCreatorOfNFT(0) == pfp.getState()->getPossessorOfNFT(0));
    earnedQubic += gt_marketFee;
    collectedShareHolderFee += gt_shareHolderFee;

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);
    
    // big price

    pfp.listInMarket(users[2], 10000000, 0);
    pfp.getState()->listInMarketChecker(0, 10000000);

    increaseEnergy(users[3], 10000000);
    initialBalanceOfCreator = getBalance(pfp.getState()->getCreatorOfNFT(0));
    initialBalanceOfPossesor = getBalance(pfp.getState()->getPossessorOfNFT(0));
    initialBalanceOfMarket = getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0));
    oldPossesor = pfp.getState()->getPossessorOfNFT(0);

    pfp.buy(users[3], 0, 0, 10000000);
    gt_marketFee = div(10000000 * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
    gt_shareHolderFee = div(10000000 * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);
    pfp.getState()->buyChecker(oldPossesor, users[3], 0, 10000000, 0, initialBalanceOfCreator, initialBalanceOfPossesor, initialBalanceOfMarket, pfp.getState()->getCreatorOfNFT(0) == pfp.getState()->getPossessorOfNFT(0));
    earnedQubic += gt_marketFee;
    collectedShareHolderFee += gt_shareHolderFee;

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);
    
    // buy with $CFB

    pfp.listInMarket(users[3], 10000000, 0);
    pfp.getState()->listInMarketChecker(0, 10000000);

    initialBalanceOfCreator = numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, pfp.getState()->getCreatorOfNFT(0), pfp.getState()->getCreatorOfNFT(0), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, pfp.getState()->getCreatorOfNFT(0), pfp.getState()->getCreatorOfNFT(0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
    initialBalanceOfPossesor = numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, pfp.getState()->getPossessorOfNFT(0), pfp.getState()->getPossessorOfNFT(0), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, pfp.getState()->getPossessorOfNFT(0), pfp.getState()->getPossessorOfNFT(0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
    initialBalanceOfMarket = numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
    oldPossesor = pfp.getState()->getPossessorOfNFT(0);

    increaseEnergy(users[4], 10000000);
    increaseEnergy(CFB_ISSUER, QBAY_TOKEN_TRANSFER_FEE);
    EXPECT_EQ(pfp.TransferShareOwnershipAndPossession(CFB_ISSUER, assetName, 10000000000, users[4]), 10000000000);
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, div(10000000ULL, qubicPrice) * cfbPrice, users[4]), div(10000000ULL, qubicPrice) * cfbPrice);

    pfp.buy(users[4], 0, 1, 0);
    earnedCFB += div(div(10000000ULL, qubicPrice) * cfbPrice * QBAY_FEE_NFT_SALE_MARKET, 1000ULL);

    pfp.getState()->buyChecker(oldPossesor, users[4], 0, div(10000000ULL, qubicPrice) * cfbPrice, 1, initialBalanceOfCreator, initialBalanceOfPossesor, initialBalanceOfMarket, pfp.getState()->getCreatorOfNFT(0) == pfp.getState()->getPossessorOfNFT(0));

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);
    // cancelSale 

    pfp.listInMarket(users[4], 10000000, 0);
    pfp.getState()->listInMarketChecker(0, 10000000);

    pfp.cancelSale(users[4], 0);
    pfp.getState()->cancelSaleChecker(0);

    // listInExchange
    
    increaseEnergy(users[4], 10000000);
    increaseEnergy(users[0], 10000000);

    pfp.listInExchange(users[0], 1, 0);
    pfp.getState()->listInExchangeChecker(users[0], 1, 0);

    pfp.listInExchange(users[4], 0, 1);
    pfp.getState()->possesorChecker(users[0], 0);
    pfp.getState()->possesorChecker(users[4], 1);

    // cancelExchange

    pfp.listInExchange(users[4], 1, 0);
    pfp.cancelExchange(users[4], 1, 0);
    pfp.getState()->cancelExchangeChecker(0);

    // makeOffer with $Qubic

    pfp.makeOffer(users[4], 10000000, 0, 0);
    pfp.getState()->makeOfferChecker(0, users[4], 0, 10000000);
    increaseEnergy(users[5], 100000000);
    pfp.makeOffer(users[5], 100000000, 0, 0);
    pfp.getState()->makeOfferChecker(0, users[5], 0, 100000000);

    // makeOffer with $CFB in high price

    sint64 askPrice = (div(1000000000ULL, qubicPrice) + 1) * cfbPrice;
    increaseEnergy(CFB_ISSUER, 100000000);
    EXPECT_EQ(pfp.TransferShareOwnershipAndPossession(CFB_ISSUER, assetName, askPrice, users[4]), askPrice);
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, askPrice, users[4]), askPrice);
    
    pfp.makeOffer(users[4], askPrice, 0, 1);
    pfp.getState()->makeOfferChecker(0, users[4], 1, askPrice);

    // acceptOffer 

    pfp.acceptOffer(users[0], 0);
    pfp.getState()->acceptOfferChecker(users[0], 0);
    earnedCFB += div(askPrice * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);

    // cancelOffer

    pfp.makeOffer(users[5], 100000000, 0, 0);
    pfp.getState()->makeOfferChecker(0, users[5], 0, 100000000);
    pfp.cancelOffer(users[5], 0);
    pfp.getState()->cancelOfferChecker(0);

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);

    // createTraditionalAuction
    setMemory(utcTime, 0);
    utcTime.Year = 2025;
    utcTime.Month = 12;
    utcTime.Day = 31;
    utcTime.Hour = 0;
    updateQpiTime();
    pfp.createTraditionalAuction(users[4], 10000000, 0, 0, 26, 1, 1, 0, 26, 1, 5, 0);
    pfp.getState()->createAuctionChecker(0, 10000000, 1, 0, users[4], 26, 1, 1, 0, 26, 1, 5, 0);

    // getting the info of Auction
    auto getIncomingAuctions_output = pfp.getIncomingAuctions(0, 1);
    pfp.getState()->getIncomingAuctionsChecker(getIncomingAuctions_output, 0, 1);
    
    setMemory(utcTime, 0);
    utcTime.Year = 2026;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);

    // bidOnAuction
    pfp.bidOnTraditionalAuction(users[5], 12000000, 0, 0);
    pfp.getState()->bidOnAuctionChecker(users[5], 0, 12000000);
    gt_marketFee = div(12000000 * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
    gt_shareHolderFee = div(12000000 * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

    earnedQubic += gt_marketFee;
    collectedShareHolderFee += gt_shareHolderFee;

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);

    increaseEnergy(users[6], 110000000);
    pfp.bidOnTraditionalAuction(users[6], 110000000, 0, 0);
    pfp.getState()->bidOnAuctionChecker(users[6], 0, 110000000);
    gt_marketFee = div((110000000 - 12000000) * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
    gt_shareHolderFee = div((110000000 - 12000000) * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

    earnedQubic += gt_marketFee;
    collectedShareHolderFee += gt_shareHolderFee;

    EXPECT_EQ(numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, id(QBAY_CONTRACT_INDEX, 0, 0, 0), id(QBAY_CONTRACT_INDEX, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), earnedCFB);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), earnedQubic + collectedShareHolderFee);

    auto getInfoOfMarketplace_output = pfp.getInfoOfMarketplace();
    pfp.getState()->getInfoOfMarketplaceChecker(getInfoOfMarketplace_output);

    pfp.getState()->profitChecker(earnedQubic, earnedCFB);

    pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 0);
    pfp.getState()->stateVriableChecker(cfbPrice, qubicPrice, totalIncommingNFTNumber, numberOfCollectionCreated, numberOfNFTCreated, 0);

    pfp.endEpoch();

    // increased the amount of MARKETPLACE_OWNER in line 771, so the balance of marketPlaceOwner should be earnedQubic + 1.
    EXPECT_EQ(getBalance(MARKETPLACE_OWNER), earnedQubic + 1);

    uint64 numberOfQXCFB = numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, CFB_ISSUER, CFB_ISSUER, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, 10000, CFB_ISSUER), 10000);
    EXPECT_EQ(numberOfQXCFB - 10000, numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, CFB_ISSUER, CFB_ISSUER, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
    increaseEnergy(CFB_ISSUER, 1000000);
    EXPECT_EQ(pfp.qbayTransferShareManagementRights(CFB_ISSUER, 10000, QX_CONTRACT_INDEX, 1000000).transferredNumberOfShares, 10000);
    EXPECT_EQ(numberOfQXCFB, numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, CFB_ISSUER, CFB_ISSUER, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX));
}