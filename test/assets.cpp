#define NO_UEFI

#include "gtest/gtest.h"

#include "logging_test.h"

#include "assets/assets.h"
#include "contract_core/contract_exec.h"
#include "contract_core/qpi_asset_impl.h"

#include "test_util.h"


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
            unsigned int ownershipIdx = indexLists.ownnershipsPossessionsFirstIdx[issuanceIdx];
            while (ownershipIdx != NO_ASSET_INDEX)
            {
                // check ownership
                EXPECT_LT(ownershipIdx, ASSETS_CAPACITY);
                EXPECT_EQ(assets[ownershipIdx].varStruct.issuance.type, OWNERSHIP);

                // check all possessions of ownership
                unsigned int possessionIdx = indexLists.ownnershipsPossessionsFirstIdx[ownershipIdx];
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
            Asset issuanceId(assets[issuanceIdx].varStruct.issuance.publicKey, assetNameFromString(assets[issuanceIdx].varStruct.issuance.name));
            long long numOfSharesOwned = 0, numOfSharesPossessed = 0;
            for (AssetOwnershipIterator iter(issuanceId); !iter.reachedEnd(); iter.next())
            {
                numOfSharesOwned += iter.numberOfOwnedShares();
            }
            for (AssetPossessionIterator iter(issuanceId); !iter.reachedEnd(); iter.next())
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

struct IssuanceTestData
{
    Asset id;
    unsigned short managingContract;
    unsigned int universeIdx;
    long long numOfShares;
    int numOfOwners;
    int transferDivisor;
    std::map<m256i, long long> shares;
    std::map<m256i, long long> ownershipIdx;
    std::map<m256i, long long> possessionIdx;

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
        EXPECT_EQ((int)assets[idxO].varStruct.ownership.managingContractIndex, (int)managingContract);

        const QPI::id& owner = assets[idxO].varStruct.ownership.publicKey;
        const auto sharesIt = shares.find(owner);
        ASSERT_NE(sharesIt, shares.end());
        long long numOfShares = sharesIt->second;
        EXPECT_EQ(assets[idxO].varStruct.ownership.numberOfShares, numOfShares);
        const auto ownershipIdxIt = ownershipIdx.find(owner);
        ASSERT_NE(ownershipIdxIt, ownershipIdx.end());
        EXPECT_EQ(idxO, ownershipIdxIt->second);

        EXPECT_EQ(iter.numberOfOwnedShares(), numOfShares);
        EXPECT_EQ(iter.issuer(), id.issuer);
        EXPECT_EQ(iter.owner(), owner);
    }

    void checkPossessionAndOwnershipAndIssuance(const AssetPossessionIterator& iter) const
    {
        checkOwnershipAndIssuance(iter);

        unsigned int idxP = iter.possessionIndex();
        EXPECT_LT(idxP, ASSETS_CAPACITY);
        EXPECT_EQ(assets[idxP].varStruct.possession.type, POSSESSION);
        EXPECT_EQ(assets[idxP].varStruct.possession.ownershipIndex, iter.ownershipIndex());
        EXPECT_EQ((int)assets[idxP].varStruct.possession.managingContractIndex, (int)managingContract);

        const QPI::id& possessor = assets[idxP].varStruct.possession.publicKey;
        const auto sharesIt = shares.find(possessor);
        ASSERT_NE(sharesIt, shares.end());
        long long numOfShares = sharesIt->second;
        EXPECT_EQ(assets[idxP].varStruct.possession.numberOfShares, numOfShares);
        const auto possessionIdxIt = possessionIdx.find(possessor);
        ASSERT_NE(possessionIdxIt, possessionIdx.end());
        EXPECT_EQ(idxP, possessionIdxIt->second);

        EXPECT_EQ(iter.numberOfPossessedShares(), numOfShares);
        EXPECT_EQ(iter.possessor(), possessor);
    }
};

TEST(TestCoreAssets, AssetIteratorOwnershipAndPossession)
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

    // With empty universe, all iterators should stop right after init
    for (int i = 0; i < issuancesCount; ++i)
    {
        AssetOwnershipIterator iterO(issuances[i].id);
        EXPECT_TRUE(iterO.reachedEnd());
        AssetOwnershipIterator iterP(issuances[i].id);
        EXPECT_TRUE(iterP.reachedEnd());
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
            issuances[i].shares[destId] = sharesToTransfer;
            issuances[i].ownershipIdx[destId] = destOwnershipIdx;
            issuances[i].possessionIdx[destId] = destPossessionIdx;
            remainingShares -= sharesToTransfer;
        }

        issuances[i].shares[issuances[i].id.issuer] = remainingShares;
        issuances[i].ownershipIdx[issuances[i].id.issuer] = firstOwnershipIdx;
        issuances[i].possessionIdx[issuances[i].id.issuer] = firstPossessionIdx;

        test.checkAssetsConsistency(i == issuancesCount - 1);
    }

    {
        // Test iterating all ownerships with AssetOwnershipIterator (also tests reusing iter)
        AssetOwnershipIterator iter(issuances[0].id);
        for (int i = 0; i < issuancesCount; ++i)
        {
            std::map<m256i, long long> shares = issuances[i].shares;
            std::map<m256i, long long> ownershipIdx = issuances[i].ownershipIdx;

            if (i > 0)
            {
                iter.begin(issuances[i].id);
            }

            while (!iter.reachedEnd())
            {
                issuances[i].checkOwnershipAndIssuance(iter);
                shares.erase(iter.owner());
                ownershipIdx.erase(iter.owner());
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
                iter.begin(issuances[i].id, AssetOwnershipSelect::byOwner(ownerOwnershipIdxPair.first));
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
            std::map<m256i, long long> shares = issuances[i].shares;
            std::map<m256i, long long> ownershipIdx = issuances[i].ownershipIdx;

            iter.begin(issuances[i].id, AssetOwnershipSelect::byManagingContract(issuances[i].managingContract));
            while (!iter.reachedEnd())
            {
                issuances[i].checkOwnershipAndIssuance(iter);
                shares.erase(iter.owner());
                ownershipIdx.erase(iter.owner());
                bool hasNext = iter.next();
                EXPECT_EQ(hasNext, !iter.reachedEnd());
            }

            EXPECT_EQ(shares.size(), 0);
            EXPECT_EQ(ownershipIdx.size(), 0);
        }
    }

    {
        // Test iterating all possessions with AssetPossessionIterator (also tests reusing iter)
        AssetPossessionIterator iter(issuances[0].id);
        for (int i = 0; i < issuancesCount; ++i)
        {
            std::map<m256i, long long> shares = issuances[i].shares;
            std::map<m256i, long long> possessionIdx = issuances[i].possessionIdx;

            if (i > 0)
            {
                iter.begin(issuances[i].id);
            }

            while (!iter.reachedEnd())
            {
                issuances[i].checkPossessionAndOwnershipAndIssuance(iter);
                shares.erase(iter.possessor());
                possessionIdx.erase(iter.possessor());
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
                iter.begin(issuances[i].id, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(possessorPossessionIdxPair.first));
                EXPECT_FALSE(iter.reachedEnd());
                EXPECT_EQ(iter.possessionIndex(), possessorPossessionIdxPair.second);
                issuances[i].checkPossessionAndOwnershipAndIssuance(iter);
                EXPECT_FALSE(iter.next());
                EXPECT_TRUE(iter.reachedEnd());
            }
        }
    }

    {
        // Test numberOfShares()
        for (int i = 0; i < issuancesCount; ++i)
        {
            // iterate all possession records, compare results of numberOfShares() and numberOfPossessedShares()
            std::map<m256i, long long> ownedShares;
            for (AssetPossessionIterator iter(issuances[i].id); !iter.reachedEnd(); iter.next())
            {
                long long numOfShares = numberOfShares(issuances[i].id, { iter.owner(), issuances[i].managingContract }, { iter.possessor(), issuances[i].managingContract });
                EXPECT_EQ(numOfShares, iter.numberOfPossessedShares());

                long long numOfPossessedShares = numberOfPossessedShares(issuances[i].id.assetName, issuances[i].id.issuer, iter.owner(), iter.possessor(), issuances[i].managingContract, issuances[i].managingContract);
                EXPECT_EQ(numOfShares, numOfPossessedShares);

                ownedShares[iter.owner()] += numOfShares;
            }

            // iterate all ownership records, compare results of numberOfShares() and numberOfOwnedShares()
            long long totalShares = 0;
            for (AssetOwnershipIterator iter(issuances[i].id); !iter.reachedEnd(); iter.next())
            {
                long long numOfShares = numberOfShares(issuances[i].id, { iter.owner(), issuances[i].managingContract });
                EXPECT_EQ(numOfShares, iter.numberOfOwnedShares());

                EXPECT_EQ(numOfShares, ownedShares[iter.owner()]);

                totalShares += numOfShares;
            }

            EXPECT_EQ(totalShares, numberOfShares(issuances[i].id));
            EXPECT_EQ(totalShares, issuances[i].numOfShares);
        }
    }

    // check consistency after rebuild/cleanup of hash map
    assetsEndEpoch();
    test.checkAssetsConsistency();
}

/*
TODO test
- end epoch

*/





