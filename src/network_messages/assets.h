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

struct Asset
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

    enum {
        type = 36,
    };
};

static_assert(sizeof(RequestIssuedAssets) == 32, "Something is wrong with the struct size.");


struct RespondIssuedAssets
{
    Asset asset;
    unsigned int tick;
    // TODO: Add siblings

    enum {
        type = 37,
    };
};


struct RequestOwnedAssets
{
    m256i publicKey;

    enum {
        type = 38,
    };
};

static_assert(sizeof(RequestOwnedAssets) == 32, "Something is wrong with the struct size.");


struct RespondOwnedAssets
{
    Asset asset;
    Asset issuanceAsset;
    unsigned int tick;
    // TODO: Add siblings

    enum {
        type = 39,
    };
};


struct RequestPossessedAssets
{
    m256i publicKey;

    enum {
        type = 40,
    };
};

static_assert(sizeof(RequestPossessedAssets) == 32, "Something is wrong with the struct size.");


struct RespondPossessedAssets
{
    Asset asset;
    Asset ownershipAsset;
    Asset issuanceAsset;
    unsigned int tick;
    // TODO: Add siblings

    enum {
        type = 41,
    };
};
