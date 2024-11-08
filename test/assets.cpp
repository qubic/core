#define NO_UEFI

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

#include "assets/assets.h"
#include "contract_core/contract_exec.h"
#include "contract_core/qpi_asset_impl.h"

#include "test_util.h"


class AssetsTest : public AssetStorage
{
public:
    AssetsTest()
    {
        initAssets();
    }

    ~AssetsTest()
    {
        deinitAssets();
    }

    static void clearUniverse()
    {
        memset(assets, 0, ASSETS_CAPACITY * sizeof(assets[0]));
        as.indexLists.reset();
    }

    static void checkAssetsListsConsistency()
    {
        // check lists
        std::map<unsigned int, unsigned int> listElementCount;
        unsigned int issuanceIdx = indexLists.issuancesFirstIdx;
        while (issuanceIdx != NO_ASSET_INDEX)
        {
            // check issuance
            EXPECT_LT(issuanceIdx, ASSETS_CAPACITY);
            EXPECT_EQ(assets[issuanceIdx].varStruct.issuance.type, ISSUANCE);

            char assetName[8];
            memcpy(assetName, assets[issuanceIdx].varStruct.issuance.name, 7);
            assetName[7] = 0;
            std::cout << "asset " << assetName << ": index " << issuanceIdx << std::endl;

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
        std::map<unsigned int, unsigned int> listElementCountArray;
        for (int index = 0; index < ASSETS_CAPACITY; index++)
        {
            switch (assets[index].varStruct.issuance.type)
            {
            case ISSUANCE:
                ++listElementCountArray[NO_ASSET_INDEX];
                break;
            case OWNERSHIP:
                ++listElementCountArray[assets[index].varStruct.ownership.issuanceIndex];
                break;
            case POSSESSION:
                ++listElementCountArray[assets[index].varStruct.possession.ownershipIndex];
                break;
            }
        }

        // check that counts match
        EXPECT_EQ(listElementCount.size(), listElementCountArray.size());
        for (auto it1 = listElementCount.begin(), it2 = listElementCountArray.begin();
            it1 != listElementCount.end() && it2 != listElementCountArray.end();
            ++it1, ++it2)
        {
            EXPECT_EQ(it1->first, it2->first);
            EXPECT_EQ(it1->second, it2->second);
        }
    }
};


TEST(TestCoreAssets, CheckLoadFile)
{
    AssetsTest test;
    if (loadUniverse(L"universe.132"))
    {
        test.checkAssetsListsConsistency();
    }
    else
    {
        std::cout << "Universe file not found. Skipping file test..." << std::endl;
    }
}

struct IssuanceTestData
{
    AssetIssuanceId id;
    unsigned short managingContract;
    unsigned int universeIdx;
    long long numOfShares;
    int numOfOwners;
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
        { { m256i(1, 2, 3, 4), assetNameFromString("BLUB") }, 1, NO_ASSET_INDEX, 10000, 10 },
        { { m256i::zero(), assetNameFromString("QX") }, 2, NO_ASSET_INDEX, 676, 20 },
        { { m256i(1, 2, 3, 4), assetNameFromString("BLA") }, 3, NO_ASSET_INDEX, 123456789, 200 },
        { { m256i(2, 2, 3, 4), assetNameFromString("BLA") }, 4, NO_ASSET_INDEX, 987654321, 300 },
    };
    constexpr int issuancesCount = sizeof(issuances) / sizeof(issuances[0]);

    // Build universe with multiple owners / possessor per issuance
    for (int i = 0; i < issuancesCount; ++i)
    {
        int firstOwnershipIdx = -1, firstPossessionIdx = -1, issuanceIdx = -1;
        EXPECT_EQ(issueAsset(issuances[i].id.issuer, assetNameFromInt64(issuances[i].id.assetName).c_str(), 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT,
            issuances[i].numOfShares, issuances[i].managingContract, &issuanceIdx, &firstOwnershipIdx, &firstPossessionIdx), issuances[i].numOfShares);
        issuances[i].universeIdx = issuanceIdx;

        long long remainingShares = issuances[i].numOfShares;
        for (int j = 1; j < issuances[i].numOfOwners; ++j)
        {
            long long sharesToTransfer = remainingShares / 20;
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

}

/*
TODO test
- load
- qpi.issueAsset
- qpi.transferOwnershipAndP.
- end epoch

*/





