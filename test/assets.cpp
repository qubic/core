#define NO_UEFI

#define PRINT_TEST_INFO 0

#include "gtest/gtest.h"
#include "test_util.h"

#include "logging_test.h"

#include "assets/assets.h"
#include "contract_core/contract_exec.h"
#include "contract_core/qpi_asset_impl.h"




class AssetsTest : public AssetStorage, LoggingTest
{
public:
    AssetsTest()
    {
        initAssets();
        initCommonBuffers();
    }

    ~AssetsTest()
    {
        deinitCommonBuffers();
        deinitAssets();
    }

    static void clearUniverse()
    {
        memset(assets, 0, ASSETS_CAPACITY * sizeof(assets[0]));
        as.indexLists.reset();
    }

    static void checkAssetsConsistency(bool printInfo = false)
    {
        // check lists
        std::map<unsigned int, unsigned int> listElementCount;
        unsigned int issuanceIdx = indexLists.issuancesFirstIdx;
        while (issuanceIdx != NO_ASSET_INDEX)
        {
            // check issuance
            EXPECT_LT(issuanceIdx, ASSETS_CAPACITY);
            EXPECT_EQ(assets[issuanceIdx].varStruct.issuance.type, ISSUANCE);

            // check all ownerships of issuance
            unsigned int ownershipIdx = indexLists.ownershipsPossessionsFirstIdx[issuanceIdx];
            while (ownershipIdx != NO_ASSET_INDEX)
            {
                // check ownership
                EXPECT_LT(ownershipIdx, ASSETS_CAPACITY);
                EXPECT_EQ(assets[ownershipIdx].varStruct.issuance.type, OWNERSHIP);

                // check all possessions of ownership
                unsigned int possessionIdx = indexLists.ownershipsPossessionsFirstIdx[ownershipIdx];
                while (possessionIdx != NO_ASSET_INDEX)
                {
                    // check possession
                    EXPECT_LT(possessionIdx, ASSETS_CAPACITY);
                    EXPECT_EQ(assets[possessionIdx].varStruct.issuance.type, POSSESSION);

                    // count possession of this ownership
                    ++listElementCount[ownershipIdx];

                    // get next ownership
                    possessionIdx = indexLists.nextIdx[possessionIdx];
                }

                // count ownerships of this issuance
                ++listElementCount[issuanceIdx];

                // get next ownership
                ownershipIdx = indexLists.nextIdx[ownershipIdx];
            }

            // count issuances (use noAssetIndex to identify list)
            ++listElementCount[NO_ASSET_INDEX];

            // get next issuance
            issuanceIdx = indexLists.nextIdx[issuanceIdx];
        }

        // count based on assets array
        std::map<unsigned int, unsigned int> arrayElementCount;
        for (int index = 0; index < ASSETS_CAPACITY; index++)
        {
            switch (assets[index].varStruct.issuance.type)
            {
            case ISSUANCE:
                if (printInfo)
                {
                    char assetName[8];
                    memcpy(assetName, assets[index].varStruct.issuance.name, 7);
                    assetName[7] = 0;
                    std::cout << "asset " << assetName << " by " << assets[index].varStruct.issuance.publicKey << ": index " << index << std::endl;
                }
                ++arrayElementCount[NO_ASSET_INDEX];
                break;
            case OWNERSHIP:
                ++arrayElementCount[assets[index].varStruct.ownership.issuanceIndex];
                break;
            case POSSESSION:
                ++arrayElementCount[assets[index].varStruct.possession.ownershipIndex];
                break;
            }
        }

        // check that counts match
        EXPECT_EQ(listElementCount.size(), arrayElementCount.size());
        for (auto it1 = listElementCount.begin(), it2 = arrayElementCount.begin();
            it1 != listElementCount.end() && it2 != arrayElementCount.end();
            ++it1, ++it2)
        {
            EXPECT_EQ(it1->first, it2->first);
            EXPECT_EQ(it1->second, it2->second);
        }

        // check that number of owned and possessed shares are equal for each issuance
        issuanceIdx = indexLists.issuancesFirstIdx;
        while (issuanceIdx != NO_ASSET_INDEX)
        {
            Asset asset(assets[issuanceIdx].varStruct.issuance.publicKey, assetNameFromString(assets[issuanceIdx].varStruct.issuance.name));
            long long numOfSharesOwned = 0, numOfSharesPossessed = 0;
            for (AssetOwnershipIterator iter(asset); !iter.reachedEnd(); iter.next())
            {
                numOfSharesOwned += iter.numberOfOwnedShares();
            }
            for (AssetPossessionIterator iter(asset); !iter.reachedEnd(); iter.next())
            {
                numOfSharesPossessed += iter.numberOfPossessedShares();
            }
            EXPECT_EQ(numOfSharesPossessed, numOfSharesOwned);

            issuanceIdx = indexLists.nextIdx[issuanceIdx];
        }
    }
};


TEST(TestCoreAssets, CheckLoadFile)
{
    AssetsTest test;
    if (loadUniverse(L"universe.136"))
    {
        test.checkAssetsConsistency(true);
    }
    else
    {
        std::cout << "Universe file not found. Skipping file test..." << std::endl;
    }
}

struct AssetSharesKey
{
    m256i publicKey;
    unsigned int managingContract;

    bool operator < (const AssetSharesKey& rhs) const
    {
        if (publicKey < rhs.publicKey)
            return true;
        else if (rhs.publicKey < publicKey)
            return false;
        else
            return managingContract < rhs.managingContract;
    }
};

struct AssetKey : public Asset
{
    bool operator < (const Asset& rhs) const
    {
        if (issuer < rhs.issuer)
            return true;
        else if (rhs.issuer < issuer)
            return false;
        else
            return assetName < rhs.assetName;
    }
};

struct IssuanceTestData
{
    Asset id;
    unsigned short managingContract;
    unsigned int universeIdx;
    long long numOfShares;
    int numOfOwners;
    int transferDivisor;

    std::map<AssetSharesKey, long long> shares;
    std::map<AssetSharesKey, long long> ownershipIdx;
    std::map<AssetSharesKey, long long> possessionIdx;

    void checkIssuance(const AssetIssuanceIterator& iter) const
    {
        unsigned int idxI = iter.issuanceIndex();
        EXPECT_LT(idxI, ASSETS_CAPACITY);
        EXPECT_EQ(idxI, universeIdx);
        EXPECT_EQ(assets[idxI].varStruct.issuance.type, ISSUANCE);
        EXPECT_EQ((*((unsigned long long*)assets[idxI].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF, id.assetName);
        EXPECT_EQ(assets[idxI].varStruct.issuance.publicKey, id.issuer);

        EXPECT_EQ(iter.issuer(), id.issuer);
        EXPECT_EQ(iter.assetName(), id.assetName);
    }

    void checkOwnershipAndIssuance(const AssetOwnershipIterator& iter) const
    {
        unsigned int idxI = iter.issuanceIndex();
        EXPECT_LT(idxI, ASSETS_CAPACITY);
        EXPECT_EQ(idxI, universeIdx);
        EXPECT_EQ(assets[idxI].varStruct.issuance.type, ISSUANCE);
        EXPECT_EQ((*((unsigned long long*)assets[idxI].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF, id.assetName);
        EXPECT_EQ(assets[idxI].varStruct.issuance.publicKey, id.issuer);
        
        unsigned int idxO = iter.ownershipIndex();
        EXPECT_LT(idxO, ASSETS_CAPACITY);
        EXPECT_EQ(assets[idxO].varStruct.ownership.type, OWNERSHIP);
        EXPECT_EQ(assets[idxO].varStruct.ownership.issuanceIndex, universeIdx);
        unsigned int managingContract = assets[idxO].varStruct.ownership.managingContractIndex;

        const QPI::id& owner = assets[idxO].varStruct.ownership.publicKey;
        AssetSharesKey ownerKey = { owner, managingContract };
        const auto sharesIt = shares.find(ownerKey);
        ASSERT_NE(sharesIt, shares.end());
        long long numOfShares = sharesIt->second;
        EXPECT_EQ(assets[idxO].varStruct.ownership.numberOfShares, numOfShares);
        const auto ownershipIdxIt = ownershipIdx.find(ownerKey);
        ASSERT_NE(ownershipIdxIt, ownershipIdx.end());
        EXPECT_EQ(idxO, ownershipIdxIt->second);

        EXPECT_EQ(iter.numberOfOwnedShares(), numOfShares);
        EXPECT_EQ(iter.issuer(), id.issuer);
        EXPECT_EQ(iter.assetName(), id.assetName);
        EXPECT_EQ(iter.owner(), owner);
        EXPECT_EQ((uint32)iter.ownershipManagingContract(), managingContract);
    }

    void checkPossessionAndOwnershipAndIssuance(const AssetPossessionIterator& iter) const
    {
        checkOwnershipAndIssuance(iter);

        unsigned int idxP = iter.possessionIndex();
        EXPECT_LT(idxP, ASSETS_CAPACITY);
        EXPECT_EQ(assets[idxP].varStruct.possession.type, POSSESSION);
        EXPECT_EQ(assets[idxP].varStruct.possession.ownershipIndex, iter.ownershipIndex());
        unsigned int managingContract = (int)assets[idxP].varStruct.possession.managingContractIndex;

        const QPI::id& possessor = assets[idxP].varStruct.possession.publicKey;
        AssetSharesKey possessorKey = { possessor, managingContract };
        const auto sharesIt = shares.find(possessorKey);
        ASSERT_NE(sharesIt, shares.end());
        long long numOfShares = sharesIt->second;
        EXPECT_EQ(assets[idxP].varStruct.possession.numberOfShares, numOfShares);
        const auto possessionIdxIt = possessionIdx.find(possessorKey);
        ASSERT_NE(possessionIdxIt, possessionIdx.end());
        EXPECT_EQ(idxP, possessionIdxIt->second);

        EXPECT_EQ(iter.numberOfPossessedShares(), numOfShares);
        EXPECT_EQ(iter.possessor(), possessor);
        EXPECT_EQ((uint32)iter.possessionManagingContract(), managingContract);
    }
};

TEST(TestCoreAssets, AssetIterators)
{
    AssetsTest test;
    test.clearUniverse();

    IssuanceTestData issuances[] = {
        { { m256i(1, 2, 3, 4), assetNameFromString("BLUB") }, 1, NO_ASSET_INDEX, 10000, 10, 30 },
        { { m256i::zero(), assetNameFromString("QX") }, 2, NO_ASSET_INDEX, 676, 20, 30 },
        { { m256i(1, 2, 3, 4), assetNameFromString("BLA") }, 3, NO_ASSET_INDEX, 123456789, 200, 30 },
        { { m256i(2, 2, 3, 4), assetNameFromString("BLA") }, 4, NO_ASSET_INDEX, 987654321, 300, 30 },
        { { m256i(1234, 2, 3, 4), assetNameFromString("BLA") }, 5, NO_ASSET_INDEX, 9876543210123ll, 676, 30 },
        { { m256i(1234, 2, 3, 4), assetNameFromString("FOO") }, 6, NO_ASSET_INDEX, 1000000ll, 2, 1 },
    };
    constexpr int issuancesCount = sizeof(issuances) / sizeof(issuances[0]);
    m256i unusedPublicKey(9876, 4321, 0, 13579);

    // With empty universe, all iterators should stop right after init
    AssetIssuanceIterator iterI;
    EXPECT_TRUE(iterI.reachedEnd());
    EXPECT_EQ(iterI.issuanceIndex(), NO_ASSET_INDEX);
    for (int i = 0; i < issuancesCount; ++i)
    {
        AssetOwnershipIterator iterO(issuances[i].id);
        EXPECT_TRUE(iterO.reachedEnd());
        EXPECT_EQ(iterO.issuanceIndex(), NO_ASSET_INDEX);
        EXPECT_EQ(iterO.ownershipIndex(), NO_ASSET_INDEX);
        AssetPossessionIterator iterP(issuances[i].id);
        EXPECT_TRUE(iterP.reachedEnd());
        EXPECT_EQ(iterP.issuanceIndex(), NO_ASSET_INDEX);
        EXPECT_EQ(iterP.ownershipIndex(), NO_ASSET_INDEX);
        EXPECT_EQ(iterP.possessionIndex(), NO_ASSET_INDEX);
    }

    // Build universe with multiple owners / possessor per issuance
    for (int i = 0; i < issuancesCount; ++i)
    {
        int firstOwnershipIdx = -1, firstPossessionIdx = -1, issuanceIdx = -1;
        EXPECT_EQ(issueAsset(issuances[i].id.issuer, assetNameFromInt64(issuances[i].id.assetName).c_str(), 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT,
            issuances[i].numOfShares, issuances[i].managingContract, &issuanceIdx, &firstOwnershipIdx, &firstPossessionIdx), issuances[i].numOfShares);
        issuances[i].universeIdx = issuanceIdx;

        test.checkAssetsConsistency();

        long long remainingShares = issuances[i].numOfShares;
        for (int j = 1; j < issuances[i].numOfOwners; ++j)
        {
            long long sharesToTransfer = remainingShares / issuances[i].transferDivisor;
            id destId(j*10, 9, 8, 7);
            int destOwnershipIdx = -1, destPossessionIdx = -1;
            EXPECT_TRUE(transferShareOwnershipAndPossession(firstOwnershipIdx, firstPossessionIdx, destId, sharesToTransfer, &destOwnershipIdx, &destPossessionIdx, false));
            AssetSharesKey key{ destId, issuances[i].managingContract };
            issuances[i].shares[key] = sharesToTransfer;
            issuances[i].ownershipIdx[key] = destOwnershipIdx;
            issuances[i].possessionIdx[key] = destPossessionIdx;
            remainingShares -= sharesToTransfer;
        }

        AssetSharesKey key{ issuances[i].id.issuer, issuances[i].managingContract };
        issuances[i].shares[key] = remainingShares;
        issuances[i].ownershipIdx[key] = firstOwnershipIdx;
        issuances[i].possessionIdx[key] = firstPossessionIdx;

        test.checkAssetsConsistency(i == issuancesCount - 1);
    }

    {
        // Test iterating all issuances with AssetIssuanceIterator
        std::map<AssetKey, IssuanceTestData*> testIssuancesSet;
        for (int i = 0; i < issuancesCount; ++i)
        {
            AssetKey key{ issuances[i].id.issuer, issuances[i].id.assetName };
#if PRINT_TEST_INFO > 0
            std::cout << issuances[i].id.issuer << ", name " << issuances[i].id.assetName << ", idx " << issuances[i].universeIdx << std::endl;
#endif
            testIssuancesSet[key] = &issuances[i];
        }
        AssetIssuanceIterator iter;
        while (!iter.reachedEnd())
        {
            AssetKey key{ iter.issuer(), iter.assetName() };
#if PRINT_TEST_INFO > 0
            std::cout << iter.issuer() << ", name " << iter.assetName() << ", idx " << iter.issuanceIndex() << std::endl;
#endif
            auto testIssuancesSetIt = testIssuancesSet.find(key);
            EXPECT_NE(testIssuancesSetIt, testIssuancesSet.end());
            testIssuancesSetIt->second->checkIssuance(iter);
            testIssuancesSet.erase(key);
            bool hasNext = iter.next();
            EXPECT_EQ(hasNext, !iter.reachedEnd());
        }
        EXPECT_EQ(testIssuancesSet.size(), 0);

        // Iterate by issuer (also test reusing the iterator)
        auto assetSelect = AssetIssuanceSelect::byIssuer(issuances[0].id.issuer);
        for (int i = 0; i < issuancesCount; ++i)
        {
            if (issuances[i].id.issuer != assetSelect.issuer)
                continue;
            AssetKey key{ issuances[i].id.issuer, issuances[i].id.assetName };
#if PRINT_TEST_INFO > 0
            std::cout << issuances[i].id.issuer << ", name " << issuances[i].id.assetName << ", idx " << issuances[i].universeIdx << std::endl;
#endif
            testIssuancesSet[key] = &issuances[i];
        }
        iter.begin(assetSelect);
        while (!iter.reachedEnd())
        {
            AssetKey key{ iter.issuer(), iter.assetName() };
#if PRINT_TEST_INFO > 0
            std::cout << iter.issuer() << ", name " << iter.assetName() << ", idx " << iter.issuanceIndex() << std::endl;
#endif
            auto testIssuancesSetIt = testIssuancesSet.find(key);
            EXPECT_NE(testIssuancesSetIt, testIssuancesSet.end());
            testIssuancesSetIt->second->checkIssuance(iter);
            testIssuancesSet.erase(key);
            bool hasNext = iter.next();
            EXPECT_EQ(hasNext, !iter.reachedEnd());
        }
        EXPECT_EQ(testIssuancesSet.size(), 0);

        // Iterate by name
        assetSelect = AssetIssuanceSelect::byName(assetNameFromString("BLA"));
        for (int i = 0; i < issuancesCount; ++i)
        {
            if (issuances[i].id.assetName != assetSelect.assetName)
                continue;
            AssetKey key{ issuances[i].id.issuer, issuances[i].id.assetName };
#if PRINT_TEST_INFO > 0
            std::cout << issuances[i].id.issuer << ", name " << issuances[i].id.assetName << ", idx " << issuances[i].universeIdx << std::endl;
#endif
            testIssuancesSet[key] = &issuances[i];
        }
        iter.begin(assetSelect);
        while (!iter.reachedEnd())
        {
            AssetKey key{ iter.issuer(), iter.assetName() };
#if PRINT_TEST_INFO > 0
            std::cout << iter.issuer() << ", name " << iter.assetName() << ", idx " << iter.issuanceIndex() << std::endl;
#endif
            auto testIssuancesSetIt = testIssuancesSet.find(key);
            EXPECT_NE(testIssuancesSetIt, testIssuancesSet.end());
            testIssuancesSetIt->second->checkIssuance(iter);
            testIssuancesSet.erase(key);
            bool hasNext = iter.next();
            EXPECT_EQ(hasNext, !iter.reachedEnd());
        }
        EXPECT_EQ(testIssuancesSet.size(), 0);

        // Test iterator to return single issuance
        for (int i = 0; i < issuancesCount; ++i)
        {
            iter.begin({ issuances[i].id.issuer, issuances[i].id.assetName });
            issuances[i].checkIssuance(iter);
            EXPECT_FALSE(iter.next());
        }

        // Test issuance iterator with unused key
        iter.begin({ unusedPublicKey, assetNameFromString("UNUSED") });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(AssetIssuanceSelect::byIssuer(unusedPublicKey));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(AssetIssuanceSelect::byName(assetNameFromString("UNUSED")));
        EXPECT_TRUE(iter.reachedEnd());
    }

    {
        // Test iterating all ownerships with AssetOwnershipIterator (also tests reusing iter)
        AssetOwnershipIterator iter(issuances[0].id);
        for (int i = 0; i < issuancesCount; ++i)
        {
            std::map<AssetSharesKey, long long> shares = issuances[i].shares;
            std::map<AssetSharesKey, long long> ownershipIdx = issuances[i].ownershipIdx;

            if (i > 0)
            {
                iter.begin(issuances[i].id);
            }

            while (!iter.reachedEnd())
            {
                issuances[i].checkOwnershipAndIssuance(iter);
                AssetSharesKey key{ iter.owner(), iter.ownershipManagingContract() };
                shares.erase(key);
                ownershipIdx.erase(key);
                bool hasNext = iter.next();
                EXPECT_EQ(hasNext, !iter.reachedEnd());
            }

            EXPECT_EQ(shares.size(), 0);
            EXPECT_EQ(ownershipIdx.size(), 0);
        }

        // Test iterating ownerships with specific owner (only single iteration / record because managing contract hasn't been changed)
        for (int i = 0; i < issuancesCount; ++i)
        {
            for (const auto& ownerOwnershipIdxPair : issuances[i].ownershipIdx)
            {
                iter.begin(issuances[i].id, AssetOwnershipSelect::byOwner(ownerOwnershipIdxPair.first.publicKey));
                EXPECT_FALSE(iter.reachedEnd());
                EXPECT_EQ(iter.ownershipIndex(), ownerOwnershipIdxPair.second);
                issuances[i].checkOwnershipAndIssuance(iter);
                EXPECT_FALSE(iter.next());
                EXPECT_TRUE(iter.reachedEnd());
            }
        }

        // Test iterating all ownerships with specific managing contract (all at the moment)
        for (int i = 0; i < issuancesCount; ++i)
        {
            std::map<AssetSharesKey, long long> shares = issuances[i].shares;
            std::map<AssetSharesKey, long long> ownershipIdx = issuances[i].ownershipIdx;

            iter.begin(issuances[i].id, AssetOwnershipSelect::byManagingContract(issuances[i].managingContract));
            while (!iter.reachedEnd())
            {
                issuances[i].checkOwnershipAndIssuance(iter);
                AssetSharesKey key{ iter.owner(), iter.ownershipManagingContract() };
                shares.erase(key);
                ownershipIdx.erase(key);
                bool hasNext = iter.next();
                EXPECT_EQ(hasNext, !iter.reachedEnd());
            }

            EXPECT_EQ(shares.size(), 0);
            EXPECT_EQ(ownershipIdx.size(), 0);
        }

        // Test ownership iterator with unused key
        iter.begin({ unusedPublicKey, assetNameFromString("UNUSED") });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::byOwner(unusedPublicKey));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::byManagingContract(12345));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect{ id(10, 9, 8, 7), 12345 });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect{ unusedPublicKey, 1 });
        EXPECT_TRUE(iter.reachedEnd());
    }

    {
        // Test iterating all possessions with AssetPossessionIterator (also tests reusing iter)
        AssetPossessionIterator iter(issuances[0].id);
        for (int i = 0; i < issuancesCount; ++i)
        {
            std::map<AssetSharesKey, long long> shares = issuances[i].shares;
            std::map<AssetSharesKey, long long> possessionIdx = issuances[i].possessionIdx;

            if (i > 0)
            {
                iter.begin(issuances[i].id);
            }

            while (!iter.reachedEnd())
            {
                issuances[i].checkPossessionAndOwnershipAndIssuance(iter);
                AssetSharesKey key{ iter.possessor(), iter.possessionManagingContract() };
                shares.erase(key);
                possessionIdx.erase(key);
                bool hasNext = iter.next();
                EXPECT_EQ(hasNext, !iter.reachedEnd());
            }

            EXPECT_EQ(shares.size(), 0);
            EXPECT_EQ(possessionIdx.size(), 0);
        }

        // Test iterating possessions with specific possessor (only single iteration / record because managing contract hasn't been changed)
        for (int i = 0; i < issuancesCount; ++i)
        {
            for (const auto& possessorPossessionIdxPair : issuances[i].possessionIdx)
            {
                iter.begin(issuances[i].id, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(possessorPossessionIdxPair.first.publicKey));
                EXPECT_FALSE(iter.reachedEnd());
                EXPECT_EQ(iter.possessionIndex(), possessorPossessionIdxPair.second);
                issuances[i].checkPossessionAndOwnershipAndIssuance(iter);
                EXPECT_FALSE(iter.next());
                EXPECT_TRUE(iter.reachedEnd());
            }
        }

        // Test possession iterator with unused key
        iter.begin({ unusedPublicKey, assetNameFromString("UNUSED") });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::byOwner(unusedPublicKey));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::byManagingContract(12345));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect{ id(10, 9, 8, 7), 12345 });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect{ unusedPublicKey, 1 });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(unusedPublicKey));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(12345));
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::any(), AssetPossessionSelect{ id(10, 9, 8, 7), 12345 });
        EXPECT_TRUE(iter.reachedEnd());
        iter.begin(issuances[0].id, AssetOwnershipSelect::any(), AssetPossessionSelect{ unusedPublicKey, 1 });
        EXPECT_TRUE(iter.reachedEnd());
    }

    {
        // Test numberOfShares()
        for (int i = 0; i < issuancesCount; ++i)
        {
            // iterate all possession records, compare results of numberOfShares() and numberOfPossessedShares()
            std::map<AssetSharesKey, long long> ownedShares;
            for (AssetPossessionIterator iter(issuances[i].id); !iter.reachedEnd(); iter.next())
            {
                long long numOfShares = numberOfShares(issuances[i].id, { iter.owner(), issuances[i].managingContract }, { iter.possessor(), issuances[i].managingContract });
                EXPECT_EQ(numOfShares, iter.numberOfPossessedShares());

                long long numOfPossessedShares = numberOfPossessedShares(issuances[i].id.assetName, issuances[i].id.issuer, iter.owner(), iter.possessor(), issuances[i].managingContract, issuances[i].managingContract);
                EXPECT_EQ(numOfShares, numOfPossessedShares);

                AssetSharesKey key{ iter.owner(), issuances[i].managingContract };
                ownedShares[key] += numOfShares;
            }

            // iterate all ownership records, compare results of numberOfShares() and numberOfOwnedShares()
            long long totalShares = 0;
            for (AssetOwnershipIterator iter(issuances[i].id); !iter.reachedEnd(); iter.next())
            {
                long long numOfShares = numberOfShares(issuances[i].id, { iter.owner(), issuances[i].managingContract });
                EXPECT_EQ(numOfShares, iter.numberOfOwnedShares());

                AssetSharesKey key{ iter.owner(), iter.ownershipManagingContract() };
                EXPECT_EQ(numOfShares, ownedShares[key]);

                totalShares += numOfShares;
            }

            EXPECT_EQ(totalShares, numberOfShares(issuances[i].id));
            EXPECT_EQ(totalShares, issuances[i].numOfShares);
        }
    }

    // check consistency after rebuild/cleanup of hash map
    assetsEndEpoch();
    test.checkAssetsConsistency();

    {
        // Test burning of shares
        for (int i = 0; i < issuancesCount; ++i)
        {
            for (int j = 3; j >= 1; j--)
            {
                // iterate all possession records and burn 1/j part of the shares
                long long expectedTotalShares = 0;
                for (AssetPossessionIterator iter(issuances[i].id); !iter.reachedEnd(); iter.next())
                {
                    const long long numOfSharesPossessedInitially = iter.numberOfPossessedShares();
                    const long long numOfSharesOwnedInitially = iter.numberOfOwnedShares();
                    const long long numOfSharesToBurn = numOfSharesPossessedInitially / j;
                    const long long numOfSharesPossessedAfterwards = numOfSharesPossessedInitially - numOfSharesToBurn;
                    const long long numOfSharesOwnedAfterwards = numOfSharesOwnedInitially - numOfSharesToBurn;

                    const bool success = transferShareOwnershipAndPossession(iter.ownershipIndex(), iter.possessionIndex(), NULL_ID, numOfSharesToBurn, nullptr, nullptr, false);

                    if (isZero(issuances[i].id.issuer))
                    {
                        // burning fails for contract shares
                        EXPECT_FALSE(success);
                        EXPECT_EQ(numOfSharesPossessedInitially, iter.numberOfPossessedShares());
                        EXPECT_EQ(numOfSharesOwnedInitially, iter.numberOfOwnedShares());
                        expectedTotalShares += numOfSharesPossessedInitially;
                    }
                    else
                    {
                        // burning succeeds for non-contract asset shares
                        EXPECT_TRUE(success);
                        EXPECT_EQ(numOfSharesPossessedAfterwards, iter.numberOfPossessedShares());
                        EXPECT_EQ(numOfSharesOwnedAfterwards, iter.numberOfOwnedShares());
                        expectedTotalShares += numOfSharesPossessedAfterwards;
                    }
                }
                EXPECT_EQ(expectedTotalShares, numberOfShares(issuances[i].id));
            }
        }
    }

    // check consistency after rebuild/cleanup of hash map
    assetsEndEpoch();
    test.checkAssetsConsistency();
}

TEST(TestCoreAssets, AssetTransferShareManagementRights)
{
    AssetsTest test;
    test.clearUniverse();

    IssuanceTestData issuances[] = {
        { { m256i(1, 2, 3, 4), assetNameFromString("BLUB") }, 1, NO_ASSET_INDEX, 10000, 10, 30 },
        { { m256i::zero(), assetNameFromString("QX") }, 1, NO_ASSET_INDEX, 676, 20, 30 },
        { { m256i(1, 2, 3, 4), assetNameFromString("BLA") }, 1, NO_ASSET_INDEX, 123456789, 200, 30 },
        { { m256i(2, 2, 3, 4), assetNameFromString("BLA") }, 1, NO_ASSET_INDEX, 987654321, 300, 30 },
        { { m256i(1234, 2, 3, 4), assetNameFromString("BLA") }, 1, NO_ASSET_INDEX, 9876543210123ll, 676, 30 },
        { { m256i(1234, 2, 3, 4), assetNameFromString("FOO") }, 1, NO_ASSET_INDEX, 1000000ll, 2, 1 },
    };
    constexpr int issuancesCount = sizeof(issuances) / sizeof(issuances[0]);

    // Build universe with multiple managing contracts per issuance
    for (int i = 0; i < issuancesCount; ++i)
    {
        int firstOwnershipIdx = -1, firstPossessionIdx = -1, issuanceIdx = -1;
        EXPECT_EQ(issueAsset(issuances[i].id.issuer, assetNameFromInt64(issuances[i].id.assetName).c_str(), 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT,
            issuances[i].numOfShares, issuances[i].managingContract, &issuanceIdx, &firstOwnershipIdx, &firstPossessionIdx), issuances[i].numOfShares);
        issuances[i].universeIdx = issuanceIdx;

        test.checkAssetsConsistency();

        long long remainingShares = issuances[i].numOfShares;
        for (int j = 1; j < issuances[i].numOfOwners; ++j)
        {
            long long sharesToTransfer = remainingShares / issuances[i].transferDivisor;
            unsigned int destContractIdx = j % 15;
            int destOwnershipIdx = -1, destPossessionIdx = -1;
            EXPECT_TRUE(transferShareManagementRights(firstOwnershipIdx, firstPossessionIdx,
                destContractIdx, destContractIdx, sharesToTransfer, &destOwnershipIdx, &destPossessionIdx, false));
            AssetSharesKey key{ issuances[i].id.issuer, destContractIdx };
            issuances[i].shares[key] += sharesToTransfer;
            issuances[i].ownershipIdx[key] = destOwnershipIdx;
            issuances[i].possessionIdx[key] = destPossessionIdx;
            remainingShares -= sharesToTransfer;
        }

        AssetSharesKey key{ issuances[i].id.issuer, issuances[i].managingContract };
        issuances[i].shares[key] += remainingShares;
        issuances[i].ownershipIdx[key] = firstOwnershipIdx;
        issuances[i].possessionIdx[key] = firstPossessionIdx;

        test.checkAssetsConsistency(i == issuancesCount - 1);
    }

    {
        // Test iterating all possessions with AssetPossessionIterator (also tests reusing iter)
        AssetPossessionIterator iter(issuances[0].id);
        for (int i = 0; i < issuancesCount; ++i)
        {
            std::map<AssetSharesKey, long long> shares = issuances[i].shares;
            std::map<AssetSharesKey, long long> possessionIdx = issuances[i].possessionIdx;

            if (i > 0)
            {
                iter.begin(issuances[i].id);
            }

            while (!iter.reachedEnd())
            {
                issuances[i].checkPossessionAndOwnershipAndIssuance(iter);
                AssetSharesKey key{ iter.possessor(), iter.possessionManagingContract() };
                shares.erase(key);
                possessionIdx.erase(key);
                bool hasNext = iter.next();
                EXPECT_EQ(hasNext, !iter.reachedEnd());
            }

            EXPECT_EQ(shares.size(), 0);
            EXPECT_EQ(possessionIdx.size(), 0);
        }

        // Test iterating possessions with specific possessor (different managing contracts)
        for (int i = 0; i < issuancesCount; ++i)
        {
            iter.begin(issuances[i].id, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(issuances[i].id.issuer));
            EXPECT_FALSE(iter.reachedEnd());
            while (!iter.reachedEnd())
            {
                issuances[i].checkPossessionAndOwnershipAndIssuance(iter);
                AssetSharesKey key{ iter.possessor(), iter.possessionManagingContract() };
                EXPECT_NE(issuances[i].ownershipIdx.find(key), issuances[i].ownershipIdx.end());
                EXPECT_EQ(iter.ownershipIndex(), issuances[i].ownershipIdx[key]);
                EXPECT_EQ(iter.possessionIndex(), issuances[i].possessionIdx[key]);
                EXPECT_EQ((int)iter.ownershipManagingContract(), (int)iter.possessionManagingContract());
                EXPECT_EQ(iter.numberOfOwnedShares(), iter.numberOfPossessedShares());
                EXPECT_EQ(iter.numberOfPossessedShares(), issuances[i].shares[key]);
                bool hasNext = iter.next();
                EXPECT_EQ(hasNext, !iter.reachedEnd());
            }
        }
    }
}





