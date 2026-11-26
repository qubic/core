#define NO_UEFI

#include <map>
#include <random>

#include "contract_testing.h"
#include "network_messages/common_def.h"

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

static uint64 quSalePriceToRequiredCFB(uint64 salePriceQu, uint64 cfbPrice, uint64 qubicPrice)
{
    return div(salePriceQu * cfbPrice + qubicPrice - 1ULL, qubicPrice);
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

class QBAYChecker : public QBAY, public QBAY::StateData
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
        QBAY::packQbayDate(startYear, startMonth, startDay, startHour, 0, 0, startTime);
        QBAY::packQbayDate(endYear, endMonth, endDay, endHour, 0, 0, endTime);
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

// Off-chain reference implementation of the drop commitment (what the QubicBay server / creator tool has to do).
struct QbayDropList
{
    uint32 collectionId = 0;
    uint32 depth = 0;
    std::vector<Array<uint8, 64>> uris;
    std::vector<std::vector<id>> levels; // levels[0] = leaves (padded with NULL_ID to 2^depth), levels.back()[0] = root

    static id hashLeaf(uint32 collectionId, uint32 dropIndex, const Array<uint8, 64>& uri)
    {
        QBAY::DropMerkleLeaf leaf;
        leaf.collectionId = collectionId;
        leaf.dropIndex = dropIndex;
        for (uint32 i = 0; i < 64; i++)
        {
            leaf.URI.set(i, i < QBAY_LENGTH_OF_URI ? uri.get(i) : 0);
        }
        id digest;
        KangarooTwelve(&leaf, sizeof(leaf), &digest, sizeof(digest));
        return digest;
    }

    static id hashNode(const id& left, const id& right)
    {
        QBAY::DropMerkleNode node;
        node.left = left;
        node.right = right;
        id digest;
        KangarooTwelve(&node, sizeof(node), &digest, sizeof(digest));
        return digest;
    }

    QbayDropList(uint32 collectionId_, uint32 numberOfItems) : collectionId(collectionId_)
    {
        uint32 capacity = 1;
        while (capacity < numberOfItems)
        {
            capacity <<= 1;
            depth++;
        }
        std::vector<id> leaves(capacity, NULL_ID);
        for (uint32 i = 0; i < numberOfItems; i++)
        {
            Array<uint8, 64> uri;
            uri.setAll(0);
            // distinct, deterministic 59-char "CID-like" URIs: "bafkrei" + item index digits + filler
            const char* prefix = "bafkrei";
            for (uint32 k = 0; k < 7; k++)
                uri.set(k, (uint8)prefix[k]);
            uint32 v = i;
            for (uint32 k = 58; k >= 7; k--)
            {
                uri.set(k, (uint8)('a' + (v % 26)));
                v /= 26;
            }
            uris.push_back(uri);
            leaves[i] = hashLeaf(collectionId, i, uri);
        }
        levels.push_back(leaves);
        while (levels.back().size() > 1)
        {
            const std::vector<id>& prev = levels.back();
            std::vector<id> next(prev.size() / 2);
            for (size_t j = 0; j < next.size(); j++)
                next[j] = hashNode(prev[2 * j], prev[2 * j + 1]);
            levels.push_back(next);
        }
    }

    id root() const { return levels.back()[0]; }

    Array<id, QBAY_DROP_MAX_MERKLE_DEPTH> proof(uint32 dropIndex) const
    {
        Array<id, QBAY_DROP_MAX_MERKLE_DEPTH> p;
        p.setAll(NULL_ID);
        for (uint32 l = 0; l < depth; l++)
            p.set(l, levels[l][(dropIndex >> l) ^ 1]);
        return p;
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

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QBAY_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
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

    QBAY::mintOfDrop_output mintOfDrop(const id& user, uint32 collectionId, uint64 mintOfDropFee, const id& expectedRoot = NULL_ID)
    {
        QBAY::mintOfDrop_input input;
        QBAY::mintOfDrop_output output;

        setMemory(input, 0);
        input.collectionId = collectionId;
        input.expectedRoot = expectedRoot;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 4, input, output, user, mintOfDropFee);

        return output;
    }

    // 4-byte input (collectionId only), as sent by clients that do not use expectedRoot.
    QBAY::mintOfDrop_output mintOfDropShortInput(const id& user, uint32 collectionId, uint64 mintOfDropFee)
    {
        uint32 input = collectionId;
        QBAY::mintOfDrop_output output;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 4, input, output, user, mintOfDropFee, /*checkInputSize=*/false);

        return output;
    }

    // Layout used by clients before the random-drop upgrade (collectionId + caller-chosen URI).
    struct LegacyMintOfDropInput
    {
        uint32 collectionId;
        Array<uint8, 64> URI;
    };

    QBAY::mintOfDrop_output mintOfDropLegacyInput(const id& user, uint32 collectionId, const Array<uint8, 64>& URI, uint64 mintOfDropFee)
    {
        LegacyMintOfDropInput input;
        QBAY::mintOfDrop_output output;

        input.collectionId = collectionId;
        input.URI = URI;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 4, input, output, user, mintOfDropFee, /*checkInputSize=*/false);

        return output;
    }

    QBAY::configureDrop_output configureDrop(const id& user, uint32 collectionId, const id& merkleRoot, uint32 numberOfItems)
    {
        QBAY::configureDrop_input input;
        QBAY::configureDrop_output output;

        input.collectionId = collectionId;
        input.merkleRoot = merkleRoot;
        input.numberOfItems = numberOfItems;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 18, input, output, user, 0);

        return output;
    }

    QBAY::revealDropNFT_output revealDropNFT(const id& user, uint32 NFTid, const Array<uint8, 64>& URI, const Array<id, QBAY_DROP_MAX_MERKLE_DEPTH>& proof)
    {
        QBAY::revealDropNFT_input input;
        QBAY::revealDropNFT_output output;

        input.NFTid = NFTid;
        input.URI = URI;
        input.proof = proof;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 19, input, output, user, 0);

        return output;
    }

    QBAY::getDropConfig_output getDropConfig(uint32 collectionId) const
    {
        QBAY::getDropConfig_input input;
        QBAY::getDropConfig_output output;

        input.collectionId = collectionId;

        callFunction(QBAY_CONTRACT_INDEX, 10, input, output);
        return output;
    }

    QBAY::getDropNFTInfo_output getDropNFTInfo(uint32 NFTid) const
    {
        QBAY::getDropNFTInfo_input input;
        QBAY::getDropNFTInfo_output output;

        input.NFTid = NFTid;

        callFunction(QBAY_CONTRACT_INDEX, 11, input, output);
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

    QBAY::makeOffer_output makeOfferWithReward(const id& user, uint64 askPrice, uint32 NFTid, bit paymentMethod, sint64 invocationReward)
    {
        QBAY::makeOffer_input input;
        QBAY::makeOffer_output output;

        input.askPrice = askPrice;
        input.NFTid = NFTid;
        input.paymentMethod = paymentMethod;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 11, input, output, user, invocationReward);

        return output;
    }

    QBAY::makeOffer_output makeOffer(const id& user, sint64 askPrice, uint32 NFTid, bit paymentMethod)
    {
        return makeOfferWithReward(user, (uint64)askPrice, NFTid, paymentMethod, paymentMethod == 0 ? askPrice : 0);
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

    QBAY::bidOnTraditionalAuction_output bidOnTraditionalAuctionWithReward(
        const id& user, uint64 price, uint32 NFTId, bit paymentMethod, sint64 invocationReward)
    {
        QBAY::bidOnTraditionalAuction_input input;
        QBAY::bidOnTraditionalAuction_output output;

        input.NFTId = NFTId;
        input.paymentMethod = paymentMethod;
        input.price = price;

        invokeUserProcedure(QBAY_CONTRACT_INDEX, 15, input, output, user, invocationReward);

        return output;
    }

    QBAY::bidOnTraditionalAuction_output bidOnTraditionalAuction(const id& user, uint64 price, uint32 NFTId, bit paymentMethod)
    {
        return bidOnTraditionalAuctionWithReward(user, price, NFTId, paymentMethod, (sint64)price);
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

    pfp.beginEpoch();
    system.epoch = 200;

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
    
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_8001_9000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_8001_9000);
    pfp.createCollection(users[0], 0, 9, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 9000, 10, 100, 1, 2, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_8001_9000;
    totalIncommingNFTNumber += 9000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_7001_8000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_7001_8000);
    pfp.createCollection(users[0], 0, 8, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 8000, 10, 100, 1, 3, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_7001_8000;
    totalIncommingNFTNumber += 8000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_6001_7000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_6001_7000);
    pfp.createCollection(users[0], 0, 7, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 7000, 10, 100, 1, 4, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_6001_7000;
    totalIncommingNFTNumber += 7000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_5001_6000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_5001_6000);
    pfp.createCollection(users[0], 0, 6, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 6000, 10, 100, 1, 5, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_5001_6000;
    totalIncommingNFTNumber += 6000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_4001_5000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_4001_5000);
    pfp.createCollection(users[0], 0, 5, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 5000, 10, 100, 1, 6, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_4001_5000;
    totalIncommingNFTNumber += 5000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_3001_4000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_3001_4000);
    pfp.createCollection(users[0], 0, 4, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 4000, 10, 100, 1, 7, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_3001_4000;
    totalIncommingNFTNumber += 4000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_2001_3000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_2001_3000);
    pfp.createCollection(users[0], 0, 3, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 3000, 10, 100, 1, 8, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_2001_3000;
    totalIncommingNFTNumber += 3000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_1001_2000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_1001_2000);
    pfp.createCollection(users[0], 0, 2, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 2000, 10, 100, 1, 9, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_1001_2000;
    totalIncommingNFTNumber += 2000;
    numberOfCollectionCreated++;

    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_201_1000, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_201_1000);
    pfp.createCollection(users[0], 0, 1, 10, 100, 1, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 1000, 10, 100, 1, 10, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_201_1000;
    totalIncommingNFTNumber += 1000;
    numberOfCollectionCreated++;

    // Collection for Drop. collection id: 10
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, cfbPrice * QBAY_FEE_COLLECTION_CREATE_2_200, users[0]), cfbPrice * QBAY_FEE_COLLECTION_CREATE_2_200);
    pfp.createCollection(users[0], 0, 0, 10, 100, 0, URI);
    pfp.getState()->createCollectionChecker(users[0], 0, 200, 10, 100, 0, 11, URI);
    totalPriceForCollectionCreating += QBAY_FEE_COLLECTION_CREATE_2_200;
    totalIncommingNFTNumber += 200;
    numberOfCollectionCreated++;

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

    EXPECT_EQ(numberOfPossessedShares(assetName, CFB_ISSUER, id(12, 0, 0, 0), id(12, 0, 0, 0), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX), 0);

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

    // dropMint: the item list has to be committed first, then the contract draws the item and the URI is revealed
    QbayDropList dropList(10, 200);
    EXPECT_EQ(pfp.configureDrop(users[0], 10, dropList.root(), 200).returnCode, QBAY::LogInfo::success);
    increaseEnergy(users[0], pfp.getState()->getDropMintPrice(10));
    auto mintOfDrop_output = pfp.mintOfDrop(users[0], 10, pfp.getState()->getDropMintPrice(10));
    EXPECT_EQ(mintOfDrop_output.returnCode, QBAY::LogInfo::success);
    numberOfNFTCreated++;
    EXPECT_EQ(mintOfDrop_output.NFTid, numberOfNFTCreated - 1);
    EXPECT_LT(mintOfDrop_output.dropIndex, 200u);
    EXPECT_EQ(pfp.revealDropNFT(users[0], mintOfDrop_output.NFTid, dropList.uris[mintOfDrop_output.dropIndex], dropList.proof(mintOfDrop_output.dropIndex)).returnCode, QBAY::LogInfo::success);
    pfp.getState()->mintChecker(users[0], 10, 10, dropList.uris[mintOfDrop_output.dropIndex], 0, numberOfNFTCreated - 1);

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
    const uint64 requiredCFBForBuy = quSalePriceToRequiredCFB(10000000ULL, cfbPrice, qubicPrice);
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, requiredCFBForBuy, users[4]), requiredCFBForBuy);

    pfp.buy(users[4], 0, 1, 0);
    earnedCFB += div(requiredCFBForBuy * QBAY_FEE_NFT_SALE_MARKET, 1000ULL);

    pfp.getState()->buyChecker(oldPossesor, users[4], 0, requiredCFBForBuy, 1, initialBalanceOfCreator, initialBalanceOfPossesor, initialBalanceOfMarket, pfp.getState()->getCreatorOfNFT(0) == pfp.getState()->getPossessorOfNFT(0));

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

namespace {

constexpr uint64 kEscrowQu = 1000000ULL;
// Certik PoC value (>= MAX_AMOUNT); rejected by askPrice >= MAX_AMOUNT guard.
constexpr uint64 kWrappedNegativeQu = (uint64)(-(sint64)kEscrowQu);
// >= MAX_AMOUNT; rejected at procedure entry (also covers Certik-style uint64 wrap values).
constexpr uint64 kSignedOverflowAskPrice = (uint64)INT64_MAX + 1ULL;
// Below MAX_AMOUNT; with zero reward must fail unsigned payment check (not succeed).
constexpr uint64 kBelowMaxAskPrice = (uint64)MAX_AMOUNT - 1ULL;

static Array<uint8, 64> makeTestUri()
{
    Array<uint8, 64> uri;
    for (uint32 i = 0; i < QBAY_LENGTH_OF_URI; ++i)
    {
        uri.set(i, (uint8)('a' + (i % 26)));
    }
    return uri;
}

static void initQbayMarketplaceForSignednessTests(ContractTestingQBAY& pfp, id& nftOwner, uint32& nftId)
{
    pfp.beginEpoch();
    system.epoch = 200;

    increaseEnergy(MARKETPLACE_OWNER, 1);
    pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, 1000, 1000);
    pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 1);

    nftOwner = getUser(501);

    const uint64 assetName = assetNameFromString("CFB");
    increaseEnergy(CFB_ISSUER, QBAY_ISSUE_ASSET_FEE);
    EXPECT_EQ(pfp.issueAsset(CFB_ISSUER, assetName, QBAY_CREATED_CFB_AMOUNT, 0, 0), QBAY_CREATED_CFB_AMOUNT);

    increaseEnergy(CFB_ISSUER, QBAY_TOKEN_TRANSFER_FEE);
    increaseEnergy(nftOwner, QBAY_TOKEN_TRANSFER_FEE);
    EXPECT_EQ(pfp.TransferShareOwnershipAndPossession(CFB_ISSUER, assetName, 10000000000, nftOwner), 10000000000ULL);
    // volume 0 collection fee is 100 CFB units; transfer fee * priceOfCFB (1000) shares.
    EXPECT_EQ(
        pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, 1000ULL * 100ULL, nftOwner),
        100000ULL);

    Array<uint8, 64> uri = makeTestUri();
    EXPECT_EQ(pfp.createCollection(nftOwner, 0, 0, 10, 100, 1, uri).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.mint(nftOwner, 0, 0, uri, 0, 0).returnCode, QBAY::LogInfo::success);
    nftId = 0;
}

} // namespace

TEST(TestContractQBAY, SignednessRegression_MakeOfferDoesNotDrainEscrowWithZeroPayment)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    const id victim = getUser(502);
    const id attacker = getUser(503);
    const id qbayContract = id(QBAY_CONTRACT_INDEX, 0, 0, 0);

    increaseEnergy(victim, kEscrowQu);
    increaseEnergy(attacker, 0);
    auto victimOffer = pfp.makeOffer(victim, (sint64)kEscrowQu, nftId, 0);
    EXPECT_EQ(victimOffer.returnCode, QBAY::LogInfo::success);
    pfp.getState()->makeOfferChecker(nftId, victim, 0, kEscrowQu);

    const uint64 contractBalanceBefore = getBalance(qbayContract);
    const uint64 attackerBalanceBefore = getBalance(attacker);
    EXPECT_EQ(contractBalanceBefore, kEscrowQu);

    auto maliciousOffer = pfp.makeOfferWithReward(attacker, kWrappedNegativeQu, nftId, 0, 0);
    EXPECT_EQ(maliciousOffer.returnCode, QBAY::LogInfo::invalidInput);

    auto belowMaxOffer = pfp.makeOfferWithReward(attacker, kBelowMaxAskPrice, nftId, 0, 0);
    EXPECT_EQ(belowMaxOffer.returnCode, QBAY::LogInfo::insufficientQubic);

    EXPECT_EQ(getBalance(qbayContract), contractBalanceBefore);
    EXPECT_EQ(getBalance(attacker), attackerBalanceBefore);
    EXPECT_EQ(pfp.getState()->NFTs.get(nftId).askUser, victim);
    EXPECT_EQ(pfp.getState()->NFTs.get(nftId).askMaxPrice, kEscrowQu);
}

TEST(TestContractQBAY, SignednessRegression_MakeOfferRejectsOutOfRangeAskPrice)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    const id bidder = getUser(504);
    increaseEnergy(bidder, kEscrowQu);

    auto zeroPrice = pfp.makeOfferWithReward(bidder, 0, nftId, 0, kEscrowQu);
    EXPECT_EQ(zeroPrice.returnCode, QBAY::LogInfo::invalidInput);

    auto maxAmountPrice = pfp.makeOfferWithReward(bidder, (uint64)MAX_AMOUNT, nftId, 0, kEscrowQu);
    EXPECT_EQ(maxAmountPrice.returnCode, QBAY::LogInfo::invalidInput);

    auto wrappedMaxAmount = pfp.makeOfferWithReward(bidder, kWrappedNegativeQu, nftId, 0, kEscrowQu);
    EXPECT_EQ(wrappedMaxAmount.returnCode, QBAY::LogInfo::invalidInput);
}

TEST(TestContractQBAY, SignednessRegression_BuyRejectsPoisonedSalePrice)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    const id buyer = getUser(505);
    increaseEnergy(buyer, 0);

    auto listed = pfp.listInMarket(nftOwner, kWrappedNegativeQu, nftId);
    EXPECT_EQ(listed.returnCode, QBAY::LogInfo::wrongNFTId);

    QBAY::InfoOfNFT nft = pfp.getState()->NFTs.get(nftId);
    nft.salePrice = kSignedOverflowAskPrice;
    nft.statusOfSale = 1;
    pfp.getState()->NFTs.set(nftId, nft);

    auto buyOutput = pfp.buy(buyer, nftId, 0, 0);
    EXPECT_EQ(buyOutput.returnCode, QBAY::LogInfo::invalidInput);

    nft.salePrice = kBelowMaxAskPrice;
    pfp.getState()->NFTs.set(nftId, nft);
    auto belowMaxBuy = pfp.buy(buyer, nftId, 0, 0);
    EXPECT_EQ(belowMaxBuy.returnCode, QBAY::LogInfo::insufficientQubic);
    EXPECT_EQ(getBalance(buyer), 0ULL);
}

TEST(TestContractQBAY, SignednessRegression_ListInMarketRejectsOutOfRangePrice)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    EXPECT_EQ(pfp.listInMarket(nftOwner, 0, nftId).returnCode, QBAY::LogInfo::wrongNFTId);
    EXPECT_EQ(pfp.listInMarket(nftOwner, (uint64)MAX_AMOUNT, nftId).returnCode, QBAY::LogInfo::wrongNFTId);
}

TEST(TestContractQBAY, SignednessRegression_BidRejectsOutOfRangePrice)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    setMemory(utcTime, 0);
    utcTime.Year = 2025;
    utcTime.Month = 12;
    utcTime.Day = 31;
    utcTime.Hour = 0;
    updateQpiTime();

    auto auction = pfp.createTraditionalAuction(nftOwner, kEscrowQu, nftId, 0, 26, 1, 1, 0, 26, 1, 5, 0);
    EXPECT_EQ(auction.returnCode, QBAY::LogInfo::success);

    setMemory(utcTime, 0);
    utcTime.Year = 2026;
    utcTime.Month = 1;
    utcTime.Day = 3;
    utcTime.Hour = 0;
    updateQpiTime();

    const id bidder = getUser(506);
    increaseEnergy(bidder, kEscrowQu);

    EXPECT_EQ(pfp.bidOnTraditionalAuctionWithReward(bidder, 0, nftId, 0, 0).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.bidOnTraditionalAuctionWithReward(bidder, (uint64)MAX_AMOUNT, nftId, 0, 0).returnCode, QBAY::LogInfo::invalidInput);

    auto wrappedBid = pfp.bidOnTraditionalAuctionWithReward(bidder, kWrappedNegativeQu, nftId, 0, 0);
    EXPECT_EQ(wrappedBid.returnCode, QBAY::LogInfo::invalidInput);

    auto signedOverflowBid = pfp.bidOnTraditionalAuctionWithReward(bidder, kSignedOverflowAskPrice, nftId, 0, 0);
    EXPECT_EQ(signedOverflowBid.returnCode, QBAY::LogInfo::invalidInput);

    auto belowMaxBid = pfp.bidOnTraditionalAuctionWithReward(bidder, kBelowMaxAskPrice, nftId, 0, 0);
    EXPECT_EQ(belowMaxBid.returnCode, QBAY::LogInfo::insufficientQubic);
}

TEST(TestContractQBAY, SignednessRegression_CreateCollectionRejectsOutOfRangeDropMintPrice)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    Array<uint8, 64> uri = makeTestUri();
    EXPECT_EQ(
        pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, 1000ULL * 100ULL, nftOwner),
        100000ULL);
    EXPECT_EQ(pfp.createCollection(nftOwner, 0, 0, 10, 100, 0, uri).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.createCollection(nftOwner, (uint64)MAX_AMOUNT, 0, 10, 100, 0, uri).returnCode, QBAY::LogInfo::invalidInput);
}

TEST(TestContractQBAY, SignednessRegression_MakeOfferReplaceOfferBlocksPoisonedEscrow)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    const id victim = getUser(507);
    const id attacker = getUser(508);
    const id qbayContract = id(QBAY_CONTRACT_INDEX, 0, 0, 0);

    increaseEnergy(victim, kEscrowQu);
    increaseEnergy(attacker, kEscrowQu * 2);
    EXPECT_EQ(pfp.makeOffer(victim, (sint64)kEscrowQu, nftId, 0).returnCode, QBAY::LogInfo::success);

    QBAY::InfoOfNFT nft = pfp.getState()->NFTs.get(nftId);
    nft.askMaxPrice = kWrappedNegativeQu;
    pfp.getState()->NFTs.set(nftId, nft);

    const uint64 contractBalanceBefore = getBalance(qbayContract);
    auto replaceOffer = pfp.makeOfferWithReward(attacker, kEscrowQu * 2, nftId, 0, kEscrowQu * 2);
    EXPECT_EQ(replaceOffer.returnCode, QBAY::LogInfo::invalidInput);

    EXPECT_EQ(pfp.getState()->NFTs.get(nftId).askUser, victim);
    EXPECT_EQ(getBalance(qbayContract), contractBalanceBefore);
}

TEST(TestContractQBAY, SettingOraclePriceRejectsZero)
{
    ContractTestingQBAY pfp;
    pfp.beginEpoch();
    system.epoch = 200;
    increaseEnergy(MARKETPLACE_OWNER, 1);

    EXPECT_EQ(pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, 0, 1000).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, 1000, 0).returnCode, QBAY::LogInfo::invalidInput);
}

TEST(TestContractQBAY, EconomicExploit_BuyWithCFBModeCanAcquireForFreeWhenSalePriceBelowQubicPrice)
{
    ContractTestingQBAY pfp;
    id nftOwner;
    uint32 nftId = 0;
    initQbayMarketplaceForSignednessTests(pfp, nftOwner, nftId);

    const id buyer = getUser(550);
    const uint64 livePriceOfCFB = 454000ULL;
    const uint64 livePriceOfQubic = 1052631ULL;
    const uint64 salePrice = livePriceOfQubic - 1ULL;

    pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, livePriceOfCFB, livePriceOfQubic);
    EXPECT_EQ(pfp.getState()->priceOfCFB, livePriceOfCFB);
    EXPECT_EQ(pfp.getState()->priceOfQubic, livePriceOfQubic);

    EXPECT_EQ(pfp.listInMarket(nftOwner, salePrice, nftId).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getState()->NFTs.get(nftId).possessor, nftOwner);
    EXPECT_EQ(pfp.getState()->NFTs.get(nftId).statusOfSale, 1);

    increaseEnergy(buyer, 0);
    const sint64 buyerCfbBefore =
        numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, buyer, buyer, QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX)
        + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, buyer, buyer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);

    auto out = pfp.buy(buyer, nftId, 1, 0);
    EXPECT_EQ(out.returnCode, QBAY::LogInfo::insufficientCFB);
    EXPECT_EQ(pfp.getState()->NFTs.get(nftId).possessor, nftOwner);

    const sint64 buyerCfbAfter =
        numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, buyer, buyer, QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX)
        + numberOfPossessedShares(QBAY_CFB_NAME, CFB_ISSUER, buyer, buyer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    EXPECT_EQ(buyerCfbBefore, buyerCfbAfter);
    EXPECT_EQ(getBalance(buyer), 0ULL);
}
// ---------------------------------------------------------------------------------------------------------------
// On-chain random drop (fix for "reverse lookup + re-roll" exploit of mintOfDrop)
// ---------------------------------------------------------------------------------------------------------------
namespace {

constexpr uint32 kDropCollectionId = 0;
constexpr uint32 kDropCollectionSize = 200; // volume 0

static void initQbayWithDropCollection(ContractTestingQBAY& pfp, const id& creator, uint64 dropPrice, uint32 maxPerId)
{
    pfp.beginEpoch();
    system.epoch = 200;

    increaseEnergy(MARKETPLACE_OWNER, 1);
    pfp.settingCFBAndQubicPrice(MARKETPLACE_OWNER, 1000, 1000);
    pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 1);

    const uint64 assetName = assetNameFromString("CFB");
    increaseEnergy(CFB_ISSUER, QBAY_ISSUE_ASSET_FEE);
    EXPECT_EQ(pfp.issueAsset(CFB_ISSUER, assetName, QBAY_CREATED_CFB_AMOUNT, 0, 0), QBAY_CREATED_CFB_AMOUNT);

    increaseEnergy(CFB_ISSUER, QBAY_TOKEN_TRANSFER_FEE);
    increaseEnergy(creator, QBAY_TOKEN_TRANSFER_FEE);
    EXPECT_EQ(pfp.TransferShareOwnershipAndPossession(CFB_ISSUER, assetName, 10000000000, creator), 10000000000ULL);
    // two collections of volume 0 can be paid (100 CFB units * priceOfCFB 1000 each)
    EXPECT_EQ(pfp.TransferShareManagementRights(CFB_ISSUER, QBAY_CFB_NAME, QBAY_CONTRACT_INDEX, 2ULL * 1000ULL * 100ULL, creator), 200000ULL);

    Array<uint8, 64> uri = makeTestUri();
    EXPECT_EQ(pfp.createCollection(creator, dropPrice, 0, 10, maxPerId, 0, uri).returnCode, QBAY::LogInfo::success);
}

static bool isUriEmpty(const Array<uint8, 64>& uri)
{
    for (uint32 i = 0; i < 64; i++)
    {
        if (uri.get(i) != 0)
            return false;
    }
    return true;
}

static bool isUriEqual(const Array<uint8, 64>& a, const Array<uint8, 64>& b)
{
    for (uint32 i = 0; i < 64; i++)
    {
        if (a.get(i) != b.get(i))
            return false;
    }
    return true;
}

} // namespace

TEST(TestContractQBAY, RandomDrop_MintIsDisabledUntilItemListIsCommitted)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(601);
    const id minter = getUser(602);
    initQbayWithDropCollection(pfp, creator, 1000, kDropCollectionSize);

    increaseEnergy(minter, 1000);
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 1000).returnCode, QBAY::LogInfo::dropNotConfigured);
    // old client layout with a caller-chosen URI is rejected the same way
    EXPECT_EQ(pfp.mintOfDropLegacyInput(minter, kDropCollectionId, makeTestUri(), 1000).returnCode, QBAY::LogInfo::dropNotConfigured);
    EXPECT_EQ(getBalance(minter), 1000ULL);
    EXPECT_EQ(pfp.getState()->numberOfNFT, 0u);
    EXPECT_EQ(pfp.getState()->Collections.get(kDropCollectionId).currentSize, (sint16)kDropCollectionSize);
}

TEST(TestContractQBAY, RandomDrop_ConfigureDropValidation)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(611);
    const id stranger = getUser(612);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);
    QbayDropList dropList(kDropCollectionId, kDropCollectionSize);

    increaseEnergy(stranger, 1);
    EXPECT_EQ(pfp.configureDrop(stranger, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::notCollectionCreator);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), 0).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize + 1).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, NULL_ID, kDropCollectionSize).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.configureDrop(creator, 7, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).status, QBAY_DROP_STATUS_NOT_CONFIGURED);

    // normal (non-drop) collection cannot be configured
    Array<uint8, 64> uri = makeTestUri();
    EXPECT_EQ(pfp.createCollection(creator, 0, 0, 10, 100, 1, uri).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.configureDrop(creator, 1, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::notCollectionForDrop);

    // marketplace owner may configure (needed for drops that were running before the upgrade); the creator may
    // replace the list as long as nothing has been minted
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);

    auto cfg = pfp.getDropConfig(kDropCollectionId);
    EXPECT_EQ(cfg.status, QBAY_DROP_STATUS_CONFIGURED);
    EXPECT_EQ(cfg.merkleRoot, dropList.root());
    EXPECT_EQ(cfg.numberOfItems, kDropCollectionSize);
    EXPECT_EQ(cfg.remainingItems, kDropCollectionSize);
    EXPECT_EQ(cfg.reservedItems, kDropCollectionSize);
    EXPECT_EQ(cfg.merkleDepth, 8u);
    // pool slots are only assigned when the first mint locks the list
    EXPECT_EQ(pfp.getState()->dropPoolUsed, 0u);

    increaseEnergy(stranger, 1);
    ASSERT_EQ(pfp.mintOfDrop(stranger, kDropCollectionId, 0).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).status, QBAY_DROP_STATUS_LOCKED);
    EXPECT_EQ(pfp.getState()->dropPoolUsed, kDropCollectionSize);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize - 1).returnCode, QBAY::LogInfo::dropAlreadyConfigured);
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, dropList.root(), kDropCollectionSize - 1).returnCode, QBAY::LogInfo::dropAlreadyConfigured);
}

TEST(TestContractQBAY, RandomDrop_EachCommittedItemIsDrawnExactlyOnceAndCallerCannotChooseIt)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(621);
    const id minter = getUser(622);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);
    QbayDropList dropList(kDropCollectionId, kDropCollectionSize);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);

    increaseEnergy(minter, 1);
    std::vector<uint32> drawn;
    std::vector<bool> seen(kDropCollectionSize, false);

    // The old input layout passing the URI of item 0 does not choose anything: it is refunded (URI bytes land in
    // expectedRoot, which does not match the committed list).
    increaseEnergy(minter, 1000);
    auto legacy = pfp.mintOfDropLegacyInput(minter, kDropCollectionId, dropList.uris[0], 1000);
    EXPECT_EQ(legacy.returnCode, QBAY::LogInfo::dropRootMismatch);
    EXPECT_EQ(getBalance(minter), 1001ULL);
    EXPECT_EQ(pfp.getState()->numberOfNFT, 0u);

    // 4-byte input (no expectedRoot) works
    auto first = pfp.mintOfDropShortInput(minter, kDropCollectionId, 0);
    EXPECT_EQ(first.returnCode, QBAY::LogInfo::success);
    EXPECT_TRUE(isUriEmpty(pfp.getState()->NFTs.get(first.NFTid).URI));
    drawn.push_back(first.dropIndex);

    for (uint32 i = 1; i < kDropCollectionSize; i++)
    {
        auto out = pfp.mintOfDrop(minter, kDropCollectionId, 0);
        EXPECT_EQ(out.returnCode, QBAY::LogInfo::success);
        EXPECT_EQ(out.NFTid, i);
        drawn.push_back(out.dropIndex);
    }
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 0).returnCode, QBAY::LogInfo::limitCollectionVolume);

    bool isIdentityOrder = true;
    for (uint32 i = 0; i < kDropCollectionSize; i++)
    {
        ASSERT_LT(drawn[i], kDropCollectionSize);
        EXPECT_FALSE(seen[drawn[i]]) << "item drawn twice: " << drawn[i];
        seen[drawn[i]] = true;
        if (drawn[i] != i)
            isIdentityOrder = false;

        auto info = pfp.getDropNFTInfo(i);
        EXPECT_TRUE(info.isRandomDrop);
        EXPECT_FALSE(info.isRevealed);
        EXPECT_EQ(info.collectionId, kDropCollectionId);
        EXPECT_EQ(info.dropIndex, drawn[i]);
        EXPECT_EQ(pfp.getState()->NFTs.get(i).creator, creator);
        EXPECT_EQ(pfp.getState()->NFTs.get(i).possessor, minter);
    }
    EXPECT_FALSE(isIdentityOrder);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).remainingItems, 0u);
}

TEST(TestContractQBAY, RandomDrop_RevealOnlyAcceptsCommittedUriOfDrawnIndex)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(631);
    const id minter = getUser(632);
    const id anybody = getUser(633);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);
    QbayDropList dropList(kDropCollectionId, kDropCollectionSize);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);

    increaseEnergy(minter, 1);
    increaseEnergy(anybody, 1);
    auto out = pfp.mintOfDrop(minter, kDropCollectionId, 0);
    ASSERT_EQ(out.returnCode, QBAY::LogInfo::success);
    const uint32 idx = out.dropIndex;
    const uint32 otherIdx = (idx + 1) % kDropCollectionSize;

    // another item's URI (e.g. a "legendary" one) cannot be attached, neither with its own nor with the right proof
    EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, dropList.uris[otherIdx], dropList.proof(otherIdx)).returnCode, QBAY::LogInfo::invalidMerkleProof);
    EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, dropList.uris[otherIdx], dropList.proof(idx)).returnCode, QBAY::LogInfo::invalidMerkleProof);
    EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, dropList.uris[idx], dropList.proof(otherIdx)).returnCode, QBAY::LogInfo::invalidMerkleProof);
    EXPECT_TRUE(isUriEmpty(pfp.getState()->NFTs.get(out.NFTid).URI));

    // reveal is permissionless
    EXPECT_EQ(pfp.revealDropNFT(anybody, out.NFTid, dropList.uris[idx], dropList.proof(idx)).returnCode, QBAY::LogInfo::success);
    EXPECT_TRUE(isUriEqual(pfp.getState()->NFTs.get(out.NFTid).URI, dropList.uris[idx]));
    EXPECT_TRUE(pfp.getDropNFTInfo(out.NFTid).isRevealed);
    EXPECT_TRUE(isUriEqual(pfp.getInfoOfNFTById(out.NFTid).URI, dropList.uris[idx]));
    EXPECT_EQ(pfp.revealDropNFT(anybody, out.NFTid, dropList.uris[idx], dropList.proof(idx)).returnCode, QBAY::LogInfo::alreadyRevealed);

    EXPECT_EQ(pfp.revealDropNFT(anybody, out.NFTid + 1, dropList.uris[idx], dropList.proof(idx)).returnCode, QBAY::LogInfo::wrongNFTId);

    // NFTs not minted through the random drop cannot be "revealed"
    increaseEnergy(creator, QBAY_SINGLE_NFT_CREATE_FEE);
    EXPECT_EQ(pfp.mint(creator, 0, 0, makeTestUri(), 1, QBAY_SINGLE_NFT_CREATE_FEE).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.revealDropNFT(anybody, out.NFTid + 1, dropList.uris[idx], dropList.proof(idx)).returnCode, QBAY::LogInfo::notDropNFT);
    EXPECT_FALSE(pfp.getDropNFTInfo(out.NFTid + 1).isRandomDrop);
}

TEST(TestContractQBAY, RandomDrop_LegacyPartiallyMintedCollectionCommitsRemainingItems)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(641);
    const id minter = getUser(642);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);

    // emulate a drop that was partially minted (with off-chain chosen URIs) before the upgrade
    constexpr uint32 remaining = 150;
    QBAY::InfoOfCollection c = pfp.getState()->Collections.get(kDropCollectionId);
    c.currentSize = (sint16)remaining;
    pfp.getState()->Collections.set(kDropCollectionId, c);

    QbayDropList fullList(kDropCollectionId, kDropCollectionSize);
    QbayDropList remainingList(kDropCollectionId, remaining);
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, fullList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, remainingList.root(), remaining).returnCode, QBAY::LogInfo::success);

    increaseEnergy(minter, 1);
    std::vector<bool> seen(remaining, false);
    for (uint32 i = 0; i < remaining; i++)
    {
        auto out = pfp.mintOfDrop(minter, kDropCollectionId, 0);
        ASSERT_EQ(out.returnCode, QBAY::LogInfo::success);
        ASSERT_LT(out.dropIndex, remaining);
        EXPECT_FALSE(seen[out.dropIndex]);
        seen[out.dropIndex] = true;
        EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, remainingList.uris[out.dropIndex], remainingList.proof(out.dropIndex)).returnCode, QBAY::LogInfo::success);
    }
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 0).returnCode, QBAY::LogInfo::limitCollectionVolume);
}

TEST(TestContractQBAY, RandomDrop_PaymentGoesToCreatorAndOverpaymentIsRefunded)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(651);
    const id minter = getUser(652);
    constexpr uint64 price = 1000;
    initQbayWithDropCollection(pfp, creator, price, kDropCollectionSize);
    QbayDropList dropList(kDropCollectionId, kDropCollectionSize);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);

    const uint64 creatorBefore = getBalance(creator);
    increaseEnergy(minter, 1500);
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 999).returnCode, QBAY::LogInfo::insufficientQubic);
    EXPECT_EQ(getBalance(minter), 1500ULL);
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 1500).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(getBalance(minter), 500ULL);
    EXPECT_EQ(getBalance(creator), creatorBefore + price);
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), 0ULL);
}

TEST(TestContractQBAY, RandomDrop_ShorterListReducesSupplyAndReleasesUnusedSlots)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(661);
    const id minter = getUser(662);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);
    constexpr uint32 realItems = 4;
    QbayDropList dropList(kDropCollectionId, realItems);
    EXPECT_EQ(pfp.getState()->numberOfNFTIncoming, (uint64)kDropCollectionSize);

    // 200 free slots, only 4 real items
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), realItems).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getState()->Collections.get(kDropCollectionId).currentSize, (sint16)realItems);
    EXPECT_EQ(pfp.getInfoOfCollectionById(kDropCollectionId).currentSize, (sint32)realItems);
    auto cfg = pfp.getDropConfig(kDropCollectionId);
    EXPECT_EQ(cfg.numberOfItems, realItems);
    EXPECT_EQ(cfg.remainingItems, realItems);
    EXPECT_EQ(cfg.reservedItems, kDropCollectionSize);
    EXPECT_EQ(cfg.merkleDepth, 2u);
    // still reserved while the list can be replaced
    EXPECT_EQ(pfp.getState()->numberOfNFTIncoming, (uint64)kDropCollectionSize);

    increaseEnergy(minter, 1);
    std::vector<bool> seen(realItems, false);
    for (uint32 i = 0; i < realItems; i++)
    {
        auto out = pfp.mintOfDrop(minter, kDropCollectionId, 0, dropList.root());
        ASSERT_EQ(out.returnCode, QBAY::LogInfo::success);
        ASSERT_LT(out.dropIndex, realItems);
        EXPECT_FALSE(seen[out.dropIndex]);
        seen[out.dropIndex] = true;
        EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, dropList.uris[out.dropIndex], dropList.proof(out.dropIndex)).returnCode, QBAY::LogInfo::success);
        if (i == 0)
        {
            // first mint locks the list and releases the 196 unused slots for other collections
            EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).status, QBAY_DROP_STATUS_LOCKED);
            EXPECT_EQ(pfp.getState()->numberOfNFTIncoming, (uint64)realItems);
            EXPECT_EQ(pfp.getState()->dropPoolUsed, realItems);
        }
    }
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 0).returnCode, QBAY::LogInfo::limitCollectionVolume);
    EXPECT_EQ(pfp.getState()->numberOfNFT, realItems);
    EXPECT_EQ(pfp.getState()->numberOfNFTIncoming, (uint64)realItems);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).remainingItems, 0u);
}

TEST(TestContractQBAY, RandomDrop_LegacyPartiallyMintedCollectionWithShorterList)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(665);
    const id minter = getUser(666);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);

    // like mainnet collection 1: a partially minted legacy drop with more free slots than real items left
    constexpr uint32 freeSlots = 150;
    constexpr uint32 realItems = 4;
    QBAY::InfoOfCollection c = pfp.getState()->Collections.get(kDropCollectionId);
    c.currentSize = (sint16)freeSlots;
    pfp.getState()->Collections.set(kDropCollectionId, c);
    const uint64 incomingBefore = pfp.getState()->numberOfNFTIncoming;

    QbayDropList dropList(kDropCollectionId, realItems);
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, dropList.root(), freeSlots + 1).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, dropList.root(), realItems).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).reservedItems, freeSlots);

    increaseEnergy(minter, 1);
    for (uint32 i = 0; i < realItems; i++)
    {
        ASSERT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 0).returnCode, QBAY::LogInfo::success);
    }
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, 0).returnCode, QBAY::LogInfo::limitCollectionVolume);
    EXPECT_EQ(pfp.getState()->numberOfNFTIncoming, incomingBefore - (freeSlots - realItems));
}

TEST(TestContractQBAY, RandomDrop_ListCanBeReplacedUntilFirstMint)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(671);
    const id minter = getUser(672);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);

    // a wrong list is committed first ...
    QbayDropList wrongList(kDropCollectionId, 4);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, wrongList.root(), 4).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getState()->Collections.get(kDropCollectionId).currentSize, (sint16)4);

    // ... and replaced by a longer one (bounded by the free slots at the first configuration, not by currentSize)
    QbayDropList rightList(kDropCollectionId, 10);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, rightList.root(), kDropCollectionSize + 1).returnCode, QBAY::LogInfo::invalidInput);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, rightList.root(), 10).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getState()->Collections.get(kDropCollectionId).currentSize, (sint16)10);
    auto cfg = pfp.getDropConfig(kDropCollectionId);
    EXPECT_EQ(cfg.merkleRoot, rightList.root());
    EXPECT_EQ(cfg.numberOfItems, 10u);
    EXPECT_EQ(cfg.reservedItems, kDropCollectionSize);
    EXPECT_EQ(cfg.status, QBAY_DROP_STATUS_CONFIGURED);
    EXPECT_EQ(pfp.getState()->dropPoolUsed, 0u);

    // the full list could still be restored before the first mint
    QbayDropList fullList(kDropCollectionId, kDropCollectionSize);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, fullList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, rightList.root(), 10).returnCode, QBAY::LogInfo::success);

    increaseEnergy(minter, 1);
    auto out = pfp.mintOfDrop(minter, kDropCollectionId, 0);
    ASSERT_EQ(out.returnCode, QBAY::LogInfo::success);
    ASSERT_LT(out.dropIndex, 10u);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).status, QBAY_DROP_STATUS_LOCKED);
    EXPECT_EQ(pfp.getState()->numberOfNFTIncoming, 10ULL);

    // locked: no replacement any more, by nobody
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, wrongList.root(), 4).returnCode, QBAY::LogInfo::dropAlreadyConfigured);
    EXPECT_EQ(pfp.configureDrop(MARKETPLACE_OWNER, kDropCollectionId, wrongList.root(), 4).returnCode, QBAY::LogInfo::dropAlreadyConfigured);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).merkleRoot, rightList.root());
    EXPECT_EQ(pfp.getState()->Collections.get(kDropCollectionId).currentSize, (sint16)9);

    // only the final list reveals
    if (out.dropIndex < 4)
    {
        EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, wrongList.uris[out.dropIndex], wrongList.proof(out.dropIndex)).returnCode, QBAY::LogInfo::invalidMerkleProof);
    }
    EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, rightList.uris[out.dropIndex], rightList.proof(out.dropIndex)).returnCode, QBAY::LogInfo::success);
}

TEST(TestContractQBAY, RandomDrop_ExpectedRootRefundsMintIfListWasReplaced)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(681);
    const id minter = getUser(682);
    constexpr uint64 price = 1000;
    initQbayWithDropCollection(pfp, creator, price, kDropCollectionSize);

    QbayDropList shownList(kDropCollectionId, 10);
    QbayDropList swappedList(kDropCollectionId, 5);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, shownList.root(), 10).returnCode, QBAY::LogInfo::success);
    // creator swaps the list right before the minter's transaction is executed
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, swappedList.root(), 5).returnCode, QBAY::LogInfo::success);

    const uint64 creatorBefore = getBalance(creator);
    increaseEnergy(minter, price);
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, price, shownList.root()).returnCode, QBAY::LogInfo::dropRootMismatch);
    EXPECT_EQ(getBalance(minter), price);
    EXPECT_EQ(getBalance(creator), creatorBefore);
    EXPECT_EQ(pfp.getState()->numberOfNFT, 0u);
    EXPECT_EQ(pfp.getDropConfig(kDropCollectionId).status, QBAY_DROP_STATUS_CONFIGURED);

    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, price, swappedList.root()).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(getBalance(minter), 0ULL);
    EXPECT_EQ(getBalance(creator), creatorBefore + price);
}

TEST(TestContractQBAY, RandomDrop_MintOfDropIsPausedWithMarketplaceStatus)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(691);
    const id minter = getUser(692);
    constexpr uint64 price = 1000;
    initQbayWithDropCollection(pfp, creator, price, kDropCollectionSize);
    QbayDropList dropList(kDropCollectionId, kDropCollectionSize);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, dropList.root(), kDropCollectionSize).returnCode, QBAY::LogInfo::success);

    increaseEnergy(minter, 2 * price);
    auto first = pfp.mintOfDrop(minter, kDropCollectionId, price);
    ASSERT_EQ(first.returnCode, QBAY::LogInfo::success);

    // only the marketplace owner can pause
    EXPECT_EQ(pfp.changeStatusOfMarketPlace(creator, 0).returnCode, QBAY::LogInfo::notOwner);
    EXPECT_EQ(pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 0).returnCode, QBAY::LogInfo::success);

    // paused: refunded, for everybody (including creator and owner), nothing changes
    const uint64 creatorBefore = getBalance(creator);
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, price).returnCode, QBAY::LogInfo::notAvailableCreateAndMint);
    EXPECT_EQ(getBalance(minter), price);
    EXPECT_EQ(getBalance(creator), creatorBefore);
    increaseEnergy(creator, price);
    EXPECT_EQ(pfp.mintOfDrop(creator, kDropCollectionId, price).returnCode, QBAY::LogInfo::notAvailableCreateAndMint);
    EXPECT_EQ(pfp.mintOfDrop(MARKETPLACE_OWNER, kDropCollectionId, 0).returnCode, QBAY::LogInfo::notAvailableCreateAndMint);
    EXPECT_EQ(pfp.getState()->numberOfNFT, 1u);
    EXPECT_EQ(pfp.getState()->Collections.get(kDropCollectionId).currentSize, (sint16)(kDropCollectionSize - 1));
    EXPECT_EQ(getBalance(id(QBAY_CONTRACT_INDEX, 0, 0, 0)), 0ULL);

    // reveal still works while paused
    EXPECT_EQ(pfp.revealDropNFT(minter, first.NFTid, dropList.uris[first.dropIndex], dropList.proof(first.dropIndex)).returnCode, QBAY::LogInfo::success);

    // resume
    EXPECT_EQ(pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 1).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.mintOfDrop(minter, kDropCollectionId, price).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.getState()->numberOfNFT, 2u);
}

TEST(TestContractQBAY, RandomDrop_ListCanBeFixedWhilePaused)
{
    ContractTestingQBAY pfp;
    const id creator = getUser(695);
    const id minter = getUser(696);
    initQbayWithDropCollection(pfp, creator, 0, kDropCollectionSize);

    QbayDropList wrongList(kDropCollectionId, 7);
    QbayDropList rightList(kDropCollectionId, 4);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, wrongList.root(), 7).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 0).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.configureDrop(creator, kDropCollectionId, rightList.root(), 4).returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.changeStatusOfMarketPlace(MARKETPLACE_OWNER, 1).returnCode, QBAY::LogInfo::success);

    increaseEnergy(minter, 1);
    auto out = pfp.mintOfDrop(minter, kDropCollectionId, 0, rightList.root());
    ASSERT_EQ(out.returnCode, QBAY::LogInfo::success);
    EXPECT_EQ(pfp.revealDropNFT(minter, out.NFTid, rightList.uris[out.dropIndex], rightList.proof(out.dropIndex)).returnCode, QBAY::LogInfo::success);
}
