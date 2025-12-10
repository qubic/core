#pragma once

#include "common_def.h"

#define EMPTY 0
#define ISSUANCE 1
#define OWNERSHIP 2
#define POSSESSION 3

#define AMPERE 0
#define CANDELA 1
#define KELVIN 2
#define KILOGRAM 3
#define METER 4
#define MOLE 5
#define SECOND 6

struct AssetRecord
{
    union
    {
        struct
        {
            m256i publicKey;
            unsigned char type;
            char name[7]; // Capital letters + digits
            char numberOfDecimalPlaces;
            char unitOfMeasurement[7]; // Powers of the corresponding SI base units going in alphabetical order
        } issuance;

        static_assert(sizeof(issuance) == 32 + 1 + 7 + 1 + 7, "Something is wrong with the struct size.");

        struct
        {
            m256i publicKey;
            unsigned char type;
            char padding[1];
            unsigned short managingContractIndex;
            unsigned int issuanceIndex;
            long long numberOfShares;
        } ownership;

        static_assert(sizeof(ownership) == 32 + 1 + 1 + 2 + 4 + 8, "Something is wrong with the struct size.");

        struct
        {
            m256i publicKey;
            unsigned char type;
            char padding[1];
            unsigned short managingContractIndex;
            unsigned int ownershipIndex;
            long long numberOfShares;
        } possession;

        static_assert(sizeof(possession) == 32 + 1 + 1 + 2 + 4 + 8, "Something is wrong with the struct size.");

    } varStruct;
};


struct RequestIssuedAssets
{
    m256i publicKey;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_ISSUED_ASSETS;
    }
};

static_assert(sizeof(RequestIssuedAssets) == 32, "Something is wrong with the struct size.");


struct RespondIssuedAssets
{
    AssetRecord asset;
    unsigned int tick;
    unsigned int universeIndex;
    m256i siblings[ASSETS_DEPTH];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_ISSUED_ASSETS;
    }
};


struct RequestOwnedAssets
{
    m256i publicKey;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_OWNED_ASSETS;
    }
};

static_assert(sizeof(RequestOwnedAssets) == 32, "Something is wrong with the struct size.");


struct RespondOwnedAssets
{
    AssetRecord asset;
    AssetRecord issuanceAsset;
    unsigned int tick;
    unsigned int universeIndex;
    m256i siblings[ASSETS_DEPTH];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_OWNED_ASSETS;
    }
};


struct RequestPossessedAssets
{
    m256i publicKey;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_POSSESSED_ASSETS;
    }
};

static_assert(sizeof(RequestPossessedAssets) == 32, "Something is wrong with the struct size.");


struct RespondPossessedAssets
{
    AssetRecord asset;
    AssetRecord ownershipAsset;
    AssetRecord issuanceAsset;
    unsigned int tick;
    unsigned int universeIndex;
    m256i siblings[ASSETS_DEPTH];

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_POSSESSED_ASSETS;
    }
};

// Options to request assets:
// - all issued asset records, optionally with filtering by issuer and/or name
// - all ownership records of a specific asset type, optionally with filtering by owner and managing contract
// - all possession records of a specific asset type, optionally with filtering by possessor and managing contract
// - by universeIdx (set issuer and asset name to 0)
union RequestAssets
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_ASSETS;
    }

    // type of asset request
    static constexpr unsigned short requestIssuanceRecords = 0;
    static constexpr unsigned short requestOwnershipRecords = 1;
    static constexpr unsigned short requestPossessionRecords = 2;
    static constexpr unsigned short requestByUniverseIdx = 3;
    unsigned short assetReqType;

    // common flags
    static constexpr unsigned short getSiblings = 0b1;

    // flags of requestIssuanceRecords
    static constexpr unsigned short anyIssuer = 0b10;
    static constexpr unsigned short anyAssetName = 0b100;

    // flags of requestOwnershipRecords
    static constexpr unsigned short anyOwner = 0b1000;
    static constexpr unsigned short anyOwnershipManagingContract = 0b10000;

    // flags of requestOwnershipRecords and requestPossessionRecords
    static constexpr unsigned short anyPossessor = 0b100000;
    static constexpr unsigned short anyPossessionManagingContract = 0b1000000;

    // data of type requestIssuanceRecords, requestOwnershipRecords, and requestPossessionRecords
    struct
    {
        unsigned short assetReqType;
        unsigned short flags;
        unsigned short ownershipManagingContract;
        unsigned short possessionManagingContract;
        m256i issuer;
        unsigned long long assetName;
        m256i owner;
        m256i possessor;
    } byFilter;

    // data of type requestByUniverseIdx
    struct
    {
        unsigned short assetReqType;
        unsigned short flags;
        unsigned int universeIdx;
    } byUniverseIdx;
};

static_assert(sizeof(RequestAssets) == 112, "Something is wrong with the struct size.");


// Response message after RequestAssets without flag getSiblings
struct RespondAssets
{
    AssetRecord asset;
    unsigned int tick;
    unsigned int universeIndex;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_ASSETS;
    }
};

static_assert(sizeof(RespondAssets) == 56, "Something is wrong with the struct size.");

// Response message after RequestAssets with flag getSiblings
struct RespondAssetsWithSiblings : public RespondAssets
{
    m256i siblings[ASSETS_DEPTH];
};

//static_assert(sizeof(RespondAssetsWithSiblings) == 824, "Something is wrong with the struct size.");
