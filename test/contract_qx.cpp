#define NO_UEFI

#include "contract_testing.h"

#define PRINT_DETAILS 0

static constexpr uint64 QX_ISSUE_ASSET_FEE = 1000000000ull;

std::string assetNameFromInt64(uint64 assetName);

class QxChecker : public QX
{
public:
    struct Order
    {
        id issuer;
        uint64 assetName;
        id entity;
        sint64 price;
        sint64 numberOfShares;

        bool operator<(const Order& other) const
        {
            return memcmp(this, &other, sizeof(other)) < 0;
        }
    };

    void checkCollectionConsistency()
    {
        EXPECT_EQ(_entityOrders.population(), _assetOrders.population());

        std::set<Order> entityOrders;
        std::map<id, unsigned int> entityCounter;
        for (uint64 i = 0; i < _entityOrders.capacity(); ++i)
        {
            QX::_EntityOrder order = _entityOrders.element(i);
            if (!order.numberOfShares)
                continue;
            Order o;
            o.issuer = order.issuer;
            o.assetName = order.assetName;
            o.entity = _entityOrders.pov(i);
            o.price = _entityOrders.priority(i);
            o.numberOfShares = order.numberOfShares;
            entityOrders.insert(o);

            ++entityCounter[o.entity];
        }

        for (const auto& p : entityCounter)
        {
            EXPECT_EQ(_entityOrders.population(p.first), p.second);
        }

        std::set<Order> assetOrders;
        for (uint64 i = 0; i < _assetOrders.capacity(); ++i)
        {
            QX::_AssetOrder order = _assetOrders.element(i);
            if (!order.numberOfShares)
                continue;
            Order o;
            id pov = _assetOrders.pov(i);
            o.issuer = id(pov.u64._0, pov.u64._1, pov.u64._2, 0);
            o.assetName = pov.u64._3;
            o.entity = order.entity;
            o.price = _assetOrders.priority(i);
            o.numberOfShares = order.numberOfShares;
            assetOrders.insert(o);
        }

        EXPECT_EQ(entityOrders.size(), assetOrders.size());
        auto it1 = entityOrders.begin(), it2 = assetOrders.begin();
        while (it1 != entityOrders.end())
        {
            // issuer cannot be fully obtained from assetOrder (4th element missing)
            EXPECT_EQ(it1->issuer.u64._0, it2->issuer.u64._0);
            EXPECT_EQ(it1->issuer.u64._1, it2->issuer.u64._1);
            EXPECT_EQ(it1->issuer.u64._2, it2->issuer.u64._2);
            EXPECT_EQ(it1->assetName, it2->assetName);
            EXPECT_EQ(it1->entity, it2->entity);
            EXPECT_EQ(it1->price, it2->price);
            EXPECT_EQ(it1->numberOfShares, it2->numberOfShares);
            ++it1; ++it2;
        }
    }

    void cleanupCollections()
    {
        constexpr bool forceCleanup = false;
        const auto population = _entityOrders.population();
        checkCollectionConsistency();
        if (forceCleanup)
        {
            _entityOrders.cleanup();
            _assetOrders.cleanup();
        }
        else
        {
            _entityOrders.cleanupIfNeeded(30);
            _assetOrders.cleanupIfNeeded(30);
        }
        checkCollectionConsistency();
        EXPECT_EQ(population, _entityOrders.population());
    }
};

class ContractTestingQx : protected ContractTesting
{
public:
    ContractTestingQx()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    QxChecker* getState()
    {
        return (QxChecker*)contractStates[QX_CONTRACT_INDEX];
    }

    bool loadState(const CHAR16* filename)
    {
        return load(filename, sizeof(QX), contractStates[QX_CONTRACT_INDEX]) == sizeof(QX);
    }

    // TODO: add other functions

    QX::AssetBidOrders_output assetBidOrders(const id& issuer, uint64 assetName, uint64 offset)
    {
        QX::AssetBidOrders_input input{ issuer, assetName, offset };
        QX::AssetBidOrders_output output;
        callFunction(QX_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QX::EntityBidOrders_output entityBidOrders(const id& entity, uint64 offset)
    {
        QX::EntityBidOrders_input input{ entity, offset };
        QX::EntityBidOrders_output output;
        callFunction(QX_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QX_ISSUE_ASSET_FEE);
        return output.issuedNumberOfShares;
    }

    // TODO: add other procedures

    void endTick(bool expectSuccess = true)
    {
        callSystemProcedure(QX_CONTRACT_INDEX, END_TICK, expectSuccess);
    }
};


TEST(ContractQx, IssueAsset)
{
    ContractTestingQx qx;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QUTIL");
    sint64 numberOfShares = 1000000;

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, numberOfShares, 0, 0), numberOfShares);

    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), numberOfShares);
}

TEST(ContractQx, BugEntityBidOrders)
{
    ContractTestingQx qx;
    m256i issuer = m256i::zero();
    uint64 assetName = assetNameFromString("QUTIL");
    auto entityIdentity = (const unsigned char*)"EEWCBEZNLEITWFWVEOFBLKHVXTAARMIGJNXICDIRIFDBUDGFXEYABULCFXAN";
    m256i entityPubkey;
    getPublicKeyFromIdentity(entityIdentity, entityPubkey.m256i_u8);
    
    if (!qx.loadState(L"contract0001.128"))
    {
        std::cout << "Skipping test due to missing file!" << std::endl;
        return;
    }

    qx.getState()->checkCollectionConsistency();

    auto entityBidOrders = qx.entityBidOrders(entityPubkey, 0);
    int entityBidOrdersCount = 0;
    for (auto i = 0ull; i < entityBidOrders.orders.capacity(); ++i)
    {
        const auto& order = entityBidOrders.orders.get(i);
        if (!order.price)
            break;

        if (order.issuer == issuer && order.assetName == assetName)
            ++entityBidOrdersCount;

#if PRINT_DETAILS
        std::cout
            << "entity " << entityPubkey
            << ", issuer " << order.issuer
            << ", assetName " << assetNameFromInt64(order.assetName)
            << ", price " << order.price
            << ", shares " << order.numberOfShares << std::endl;
#endif
    }

    auto assertBidOrders = qx.assetBidOrders(issuer, assetName, 0);
    int assertBidOrdersCount = 0;
    for (auto i = 0ull; i < assertBidOrders.orders.capacity(); ++i)
    {
        const auto& order = assertBidOrders.orders.get(i);
        if (!order.price)
            break;

        if (order.entity == entityPubkey)
            ++assertBidOrdersCount;

#if PRINT_DETAILS
        std::cout
            << "entity " << order.entity
            << ", issuer " << issuer
            << ", assetName " << assetNameFromInt64(assetName)
            << ", price " << order.price
            << ", shares " << order.numberOfShares << std::endl;
#endif
    }

    EXPECT_EQ(assertBidOrdersCount, entityBidOrdersCount);
}

TEST(ContractQx, CleanupCollections)
{
    ContractTestingQx qx;
    if (qx.loadState(L"contract0001.163"))
    {
        std::cout << "QX state file:" << std::endl;
        QxChecker* state = qx.getState();
        qx.endTick();
        state->cleanupCollections();
    }
    else
    {
        std::cout << "QX state file not found. Skipping file test..." << std::endl;
    }
}
