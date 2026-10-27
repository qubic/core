#define NO_UEFI

// std headers must come before contract_testing.h: core #defines `system`, which breaks process.h
#include <chrono>
#include <cstdlib>
#include <future>
#include <thread>

#include "contract_testing.h"

#define PRINT_DETAILS 0

static constexpr uint64 QX_ISSUE_ASSET_FEE = 1000000000ull;
static constexpr uint64 QX_TRANSFER_FEE = 100ull;

std::string assetNameFromInt64(uint64 assetName);

class QxChecker : public QX, public QX::StateData
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
            QX::EntityOrder order = _entityOrders.element(i);
            if (!order.numberOfShares)
                continue;
            Order o;
            // asset orders only know the low 192 bits of the issuer, so sort both sides by the same key
            o.issuer = id(order.issuer.u64._0, order.issuer.u64._1, order.issuer.u64._2, 0);
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
            QX::AssetOrder order = _assetOrders.element(i);
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

    void fillOrderBook()
    {
        QX::AssetOrder assetOrder{};
        QX::EntityOrder entityOrder{};
        for (uint64 i = _assetOrders.population(); i < _assetOrders.capacity(); ++i)
        {
            _assetOrders.add(id(i, 9, 9, 9), assetOrder, 1);
            _entityOrders.add(id(i, 9, 9, 9), entityOrder, 1);
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

    sint64 addToAskOrder(const id& entity, const id& issuer, uint64 assetName, sint64 price, sint64 numberOfShares)
    {
        QX::AddToAskOrder_input input{ issuer, assetName, price, numberOfShares };
        QX::AddToAskOrder_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 5, input, output, entity, 0);
        return output.addedNumberOfShares;
    }

    sint64 addToBidOrder(const id& entity, const id& issuer, uint64 assetName, sint64 price, sint64 numberOfShares)
    {
        QX::AddToBidOrder_input input{ issuer, assetName, price, numberOfShares };
        QX::AddToBidOrder_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 6, input, output, entity, price * numberOfShares);
        return output.addedNumberOfShares;
    }

    sint64 transferShares(const id& owner, const id& issuer, uint64 assetName, sint64 numberOfShares, const id& newOwner)
    {
        QX::TransferShareOwnershipAndPossession_input input{ issuer, newOwner, assetName, numberOfShares };
        QX::TransferShareOwnershipAndPossession_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, owner, QX_TRANSFER_FEE);
        return output.transferredNumberOfShares;
    }

    QX::AssetAskOrders_output assetAskOrders(const id& issuer, uint64 assetName, uint64 offset)
    {
        QX::AssetAskOrders_input input{ issuer, assetName, offset };
        QX::AssetAskOrders_output output;
        callFunction(QX_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    sint64 removeFromBidOrder(const id& entity, const id& issuer, uint64 assetName, sint64 price, sint64 numberOfShares)
    {
        QX::RemoveFromBidOrder_input input{ issuer, assetName, price, numberOfShares };
        QX::RemoveFromBidOrder_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 8, input, output, entity, 0);
        return output.removedNumberOfShares;
    }

    // TODO: add other procedures

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QX_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void beginTick(bool expectSuccess = true)
    {
        callSystemProcedure(QX_CONTRACT_INDEX, BEGIN_TICK, expectSuccess);
    }

    void endTick(bool expectSuccess = true)
    {
        callSystemProcedure(QX_CONTRACT_INDEX, END_TICK, expectSuccess);
    }
};

TEST(ContractQx, FrozenEntityOrdersRemovedAtCleanupTick)
{
    ContractTestingQx qx;

    id issuer(1, 2, 3, 4);
    uint64 assetName = assetNameFromString("QUTIL");
    m256i frozen;
    getPublicKeyFromIdentity(gFrozenIdentities[0], frozen.m256i_u8);
    ASSERT_TRUE(isFrozenAddress(frozen));

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000000, 0, 0), 1000000);

    // frozen entity: bid 100 shares at 10 (1000 qus escrowed) and, after getting shares, ask 50 at 30
    increaseEnergy(frozen, 1000);
    EXPECT_EQ(qx.addToBidOrder(frozen, issuer, assetName, 10, 100), 100);
    EXPECT_EQ(getBalance(frozen), 0);
    increaseEnergy(issuer, QX_TRANSFER_FEE);
    EXPECT_EQ(qx.transferShares(issuer, issuer, assetName, 50, frozen), 50);
    EXPECT_EQ(qx.addToAskOrder(frozen, issuer, assetName, 30, 50), 50);

    // non-frozen entity: ask 20 shares at 20
    EXPECT_EQ(qx.addToAskOrder(issuer, issuer, assetName, 20, 20), 20);

    // wrong tick: nothing happens
    system.tick = FREEZE_FROM_TICK - 1;
    system.epoch = 237;
    qx.beginTick();
    EXPECT_EQ(getBalance(frozen), 0);

    system.tick = FREEZE_FROM_TICK;
    qx.beginTick();

    EXPECT_EQ(getBalance(frozen), 1000);
    EXPECT_EQ(qx.entityBidOrders(frozen, 0).orders.get(0).price, 0);
    EXPECT_EQ(qx.assetBidOrders(issuer, assetName, 0).orders.get(0).price, 0);
    auto asks = qx.assetAskOrders(issuer, assetName, 0);
    EXPECT_EQ(asks.orders.get(0).entity, issuer);
    EXPECT_EQ(asks.orders.get(0).numberOfShares, 20);
    EXPECT_EQ(asks.orders.get(1).price, 0);
    qx.getState()->checkCollectionConsistency();
}

TEST(ContractQx, FullOrderBookDoesNotSwallowOrders)
{
    ContractTestingQx qx;

    id issuer(1, 2, 3, 4);
    id user(5, 6, 7, 8);
    uint64 assetName = assetNameFromString("QUTIL");

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000000, 0, 0), 1000000);

    qx.getState()->fillOrderBook();

    increaseEnergy(user, 1000);
    EXPECT_EQ(qx.addToBidOrder(user, issuer, assetName, 10, 100), 0);
    EXPECT_EQ(getBalance(user), 1000);

    EXPECT_EQ(qx.addToAskOrder(issuer, issuer, assetName, 20, 20), 0);
}


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

// A POST_INCOMING_TRANSFER to a contract that is not currently runnable (its IPO epoch, IPO-failed
// and retained, past destruction, or in an error state) is skipped rather than dispatched. TESTEXA
// has an empty POST_INCOMING_TRANSFER so it can stand in as the recipient.

// Issue QX's 676 contract shares (1 to the given contract) and give QX earnings so END_TICK pays a dividend.
static void setupQxDividendTo(ContractTestingQx& qx, const id& shareHolderContractId, sint64 perShare)
{
    const id otherHolder(7, 7, 7, 7);
    std::vector<std::pair<m256i, unsigned int>> owners = {
        { shareHolderContractId, 1 },
        { otherHolder, (unsigned int)(NUMBER_OF_COMPUTORS - 1) }
    };
    issueContractShares(QX_CONTRACT_INDEX, owners, false);

    increaseEnergy(id(QX_CONTRACT_INDEX, 0, 0, 0), (uint64)perShare * NUMBER_OF_COMPUTORS + 1000000ull);
    qx.getState()->_earnedAmount = (uint64)perShare * NUMBER_OF_COMPUTORS;
    qx.getState()->_distributedAmount = 0;
}

TEST(ContractQx, DividendToErrorStateContractSkipsCallback)
{
    ContractTestingQx qx;
    INIT_CONTRACT(TESTEXA);               // example recipient that declares POST_INCOMING_TRANSFER

    system.epoch = 240;
    constexpr unsigned int errSc = TESTEXA_CONTRACT_INDEX;
    contractError[errSc] = ContractErrorIPOFailed;   // error-state recipient holding a QX share
    const id errScId(errSc, 0, 0, 0);

    constexpr sint64 perShare = 100;
    setupQxDividendTo(qx, errScId, perShare);
    const long long before = getBalance(errScId);

    qx.endTick();                         // notification to the non-runnable recipient is skipped

    EXPECT_EQ(getBalance(errScId) - before, perShare);  // amount still credited
}

TEST(ContractQx, DividendToPreConstructionContractSkipsCallback)
{
    ContractTestingQx qx;
    INIT_CONTRACT(TESTEXA);

    system.epoch = 100;                   // before TESTEXA's construction epoch (138): not yet active
    constexpr unsigned int sc = TESTEXA_CONTRACT_INDEX;
    contractError[sc] = NoContractError;  // exercise the epoch-gate branch, not error-state
    EXPECT_LT((int)system.epoch, (int)contractDescriptions[sc].constructionEpoch);
    const id scId(sc, 0, 0, 0);

    constexpr sint64 perShare = 100;
    setupQxDividendTo(qx, scId, perShare);
    const long long before = getBalance(scId);

    qx.endTick();                         // notification to the not-yet-active recipient is skipped

    EXPECT_EQ(getBalance(scId) - before, perShare);
}

// The order books are keyed by issuer with u64._3 overwritten by the asset name, so a bid
// with a bogus issuer (same low 192 bits, different u64._3) lands in the real asset's book.
// Matching then searches the asker's entity orders for the bogus issuer, never finds it and
// spins forever in "while (true) // Impossible for the corresponding entity order to not exist".
TEST(ContractQx, BidWithTruncatedIssuerCollisionMustNotHang)
{
    ContractTestingQx qx;

    const id issuer(10, 20, 30, 40);
    const id bogusIssuer(10, 20, 30, 41); // never issued, no private key needed
    const id attacker(7, 7, 7, 7);
    const uint64 assetName = assetNameFromString("TOKEN");
    const sint64 price = 5;

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000, 0, 0), 1000);
    EXPECT_EQ(qx.addToAskOrder(issuer, issuer, assetName, price, 10), 10);

    increaseEnergy(attacker, price);
    auto bid = std::async(std::launch::async, [&] { return qx.addToBidOrder(attacker, bogusIssuer, assetName, price, 1); });
    if (bid.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
    {
        // ponytail: the spinning thread holds QX's state and cannot be stopped, so bail out of the
        // whole binary instead of letting later tests deadlock or the fixture free state under it
        ADD_FAILURE() << "AddToBidOrder with bogus issuer did not return: QX procedure hangs (network halt)";
        std::cerr << "FATAL: QX AddToBidOrder hang reproduced, exiting test binary" << std::endl;
        std::_Exit(1);
    }

    bid.get();
    // The honest ask must be untouched and the books must stay consistent
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1000);
    qx.getState()->checkCollectionConsistency();
}

// Colliding bids rest first, the honest ask crosses later: they must be skipped (full and partial
// fill branch) and an honest bid behind them must still be filled.
TEST(ContractQx, AskSkipsBidsWithTruncatedIssuerCollision)
{
    ContractTestingQx qx;

    const id issuer(10, 20, 30, 40);
    const id bogusIssuer(10, 20, 30, 41);
    const id attacker1(7, 7, 7, 7), attacker2(8, 8, 8, 8), buyer(9, 9, 9, 9);
    const uint64 assetName = assetNameFromString("TOKEN");
    const sint64 price = 5;

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000, 0, 0), 1000);

    increaseEnergy(attacker1, price);
    increaseEnergy(attacker2, price * 100);
    increaseEnergy(buyer, price * 3);
    EXPECT_EQ(qx.addToBidOrder(attacker1, bogusIssuer, assetName, price, 1), 1);
    EXPECT_EQ(qx.addToBidOrder(attacker2, bogusIssuer, assetName, price, 100), 100);
    EXPECT_EQ(qx.addToBidOrder(buyer, issuer, assetName, price, 3), 3);

    EXPECT_EQ(qx.addToAskOrder(issuer, issuer, assetName, price, 5), 5);

    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, buyer, buyer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 3);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, issuer, issuer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 997);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, attacker1, attacker1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, attacker2, attacker2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 0);
    qx.getState()->checkCollectionConsistency();
}

// The core no longer issues an asset colliding with an existing one, so fake a twin issued before that rule
static void renameIssuance(const id& issuer, uint64 fromName, uint64 toName)
{
    for (unsigned int i = 0; i < ASSETS_CAPACITY; ++i)
    {
        auto& issuance = assets[i].varStruct.issuance;
        if (issuance.type == ISSUANCE && issuance.publicKey == issuer && !memcmp(issuance.name, &fromName, 7))
        {
            memcpy(issuance.name, &toName, 7);
            return;
        }
    }
    ADD_FAILURE() << "issuance to rename not found";
}

// Asks of a twin issuer (same low 192 bits) must be skipped by an honest bid.
TEST(ContractQx, BidSkipsAsksWithTruncatedIssuerCollision)
{
    ContractTestingQx qx;

    const id issuer(10, 20, 30, 40);
    const id twin(10, 20, 30, 40 ^ 0x8000000000000000ull); // public key of N - k
    const id buyer(9, 9, 9, 9);
    const uint64 assetName = assetNameFromString("TOKEN");
    const sint64 price = 5;

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    increaseEnergy(twin, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000, 0, 0), 1000);
    EXPECT_EQ(qx.issueAsset(twin, assetNameFromString("TOKEX"), 1000, 0, 0), 1000);
    renameIssuance(twin, assetNameFromString("TOKEX"), assetName);

    EXPECT_EQ(qx.addToAskOrder(twin, twin, assetName, price, 1), 1);
    EXPECT_EQ(qx.addToAskOrder(issuer, issuer, assetName, price, 3), 3);

    increaseEnergy(buyer, price * 2);
    EXPECT_EQ(qx.addToBidOrder(buyer, issuer, assetName, price, 2), 2);

    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, buyer, buyer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 2);
    EXPECT_EQ(numberOfPossessedShares(assetName, twin, buyer, buyer, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfPossessedShares(assetName, twin, twin, twin, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1000);
    qx.getState()->checkCollectionConsistency();
}

// One entity holding bids for the real and a colliding issuer at the same price makes its asset
// orders indistinguishable: an honest ask filled the bogus asset order against the real entity
// order, and removing the bogus bid then refunded escrow that was already paid out.
TEST(ContractQx, RejectOwnOrdersWithTruncatedIssuerCollision)
{
    ContractTestingQx qx;

    const id issuer(10, 20, 30, 40);
    const id bogusIssuer(10, 20, 30, 41);
    const id attacker(7, 7, 7, 7);
    const uint64 assetName = assetNameFromString("TOKEN");
    const sint64 price = 5;

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000, 0, 0), 1000);

    increaseEnergy(attacker, price * 11);
    EXPECT_EQ(qx.addToBidOrder(attacker, bogusIssuer, assetName, price, 10), 10);
    EXPECT_EQ(qx.addToBidOrder(attacker, issuer, assetName, price, 1), 0);
    EXPECT_EQ(getBalance(attacker), price);

    EXPECT_EQ(qx.addToAskOrder(issuer, issuer, assetName, price, 10), 10);
    EXPECT_EQ(numberOfPossessedShares(assetName, issuer, attacker, attacker, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 0);

    EXPECT_EQ(qx.removeFromBidOrder(attacker, bogusIssuer, assetName, price, 10), 10);
    EXPECT_EQ(getBalance(attacker), price * 11);
    qx.getState()->checkCollectionConsistency();

    // Same in the other order
    EXPECT_EQ(qx.addToBidOrder(attacker, issuer, assetName, price - 1, 1), 1);
    EXPECT_EQ(qx.addToBidOrder(attacker, bogusIssuer, assetName, price - 1, 1), 0);
    qx.getState()->checkCollectionConsistency();
}

// Before the twin issuer fix, colliding orders could leave asset and entity orders inconsistent. The sweep in
// BEGIN_EPOCH of epoch 237 must cancel all orders of that entity under the key, refund exactly the escrow left in
// its asset orders and leave other entities untouched.
TEST(ContractQx, TwinFixSweepRepairsInconsistentOrders)
{
    ContractTestingQx qx;
    const auto savedEpoch = system.epoch;

    const id issuer(10, 20, 30, 40);
    const id attacker(7, 7, 7, 7), buyer(9, 9, 9, 9);
    const id qxId(QX_CONTRACT_INDEX, 0, 0, 0);
    const uint64 assetName = assetNameFromString("TOKEN");

    increaseEnergy(issuer, QX_ISSUE_ASSET_FEE);
    EXPECT_EQ(qx.issueAsset(issuer, assetName, 1000, 0, 0), 1000);

    increaseEnergy(attacker, 5 * 2);
    EXPECT_EQ(qx.addToBidOrder(attacker, issuer, assetName, 5, 2), 2);
    increaseEnergy(buyer, 4);
    EXPECT_EQ(qx.addToBidOrder(buyer, issuer, assetName, 4, 1), 1);

    // leftover of the old bug: escrowed asset order of attacker at 10 without entity order
    id key = issuer;
    key.u64._3 = assetName;
    QX::AssetOrder leftover{ attacker, 2 };
    qx.getState()->_assetOrders.add(key, leftover, 10);
    increaseEnergy(qxId, 2 * 10);

    system.epoch = 236;
    qx.beginEpoch();
    EXPECT_EQ(qx.assetBidOrders(issuer, assetName, 0).orders.get(0).price, 10);

    const long long qxBalanceBefore = getBalance(qxId);
    system.epoch = 237;
    qx.beginEpoch();

    EXPECT_EQ(getBalance(attacker), 2 * 10 + 2 * 5);
    EXPECT_EQ(qxBalanceBefore - getBalance(qxId), 2 * 10 + 2 * 5);
    EXPECT_EQ(qx.entityBidOrders(attacker, 0).orders.get(0).price, 0);
    auto bids = qx.assetBidOrders(issuer, assetName, 0);
    EXPECT_EQ(bids.orders.get(0).entity, buyer);
    EXPECT_EQ(bids.orders.get(0).price, 4);
    EXPECT_EQ(bids.orders.get(1).price, 0);
    qx.getState()->checkCollectionConsistency();

    system.epoch = savedEpoch;
}
