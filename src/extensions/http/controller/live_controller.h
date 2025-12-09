#pragma once
#include "extensions/utils.h"

#include <drogon/HttpController.h>

using namespace drogon;

struct AssetIssuanceType
{
    m256i publicKey{};
    unsigned char type{};
    char name[7]{}; // Capital letters + digits
    char numberOfDecimalPlaces{};
    char unitOfMeasurement[7]{}; // Powers of the corresponding SI base units going in alphabetical order
};

struct AssetOwnershipType
{
    m256i publicKey{};
    unsigned char type{};
    char padding[1]{};
    unsigned short managingContractIndex{};
    unsigned int issuanceIndex{};
    long long numberOfShares{};
};

struct AssetPossessionType
{
    m256i publicKey{};
    unsigned char type{};
    char padding[1]{};
    unsigned short managingContractIndex{};
    unsigned int ownershipIndex{};
    long long numberOfShares{};
};

static unsigned long long assetNameFromString(const char* assetName)
{
    size_t n = strlen(assetName);
    unsigned long long integer = 0;
    copyMem(&integer, assetName, n);
    return integer;
}

static std::string assetNameFromInt64(unsigned long long assetName)
{
    char buffer[8];
    copyMem(&buffer, &assetName, sizeof(assetName));
    buffer[7] = 0;
    return buffer;
}

inline Json::Value issuanceToJson(const AssetIssuanceType *asset)
{
    Json::Value assetJson;
    Json::Value unitOfMeasurementArray(Json::arrayValue);
    CHAR16 identity[61] = {};
    getIdentity((unsigned char *)&asset->publicKey, identity, false);
    assetJson["issuerIdentity"] = wchar_to_string(identity);
    assetJson["name"] = std::string(asset->name);
    assetJson["type"] = ASSET_ISSUANCE;
    assetJson["numberOfDecimalPlaces"] = asset->numberOfDecimalPlaces;
    for (int i = 0; i < 8; i++)
    {
        unitOfMeasurementArray.append(asset->unitOfMeasurement[i]);
    }
    assetJson["unitOfMeasurement"] = unitOfMeasurementArray;
    return assetJson;
}

inline Json::Value ownershipToJson(const AssetOwnershipType *asset)
{
    Json::Value assetJson;
    CHAR16 identity[61] = {};
    getIdentity((unsigned char *)&asset->publicKey, identity, false);
    assetJson["ownerIdentity"] = wchar_to_string(identity);
    assetJson["type"] = OWNERSHIP;
    assetJson["managingContractIndex"] = asset->managingContractIndex;
    assetJson["issuanceIndex"] = asset->issuanceIndex;
    assetJson["numberOfUnits"] = std::to_string(asset->numberOfShares);
    return assetJson;
}

inline Json::Value possessionToJson(const AssetPossessionType *asset)
{
    Json::Value assetJson;
    CHAR16 identity[61] = {};
    getIdentity((unsigned char *)&asset->publicKey, identity, false);
    assetJson["possessorIdentity"] = wchar_to_string(identity);
    assetJson["type"] = POSSESSION;
    assetJson["managingContractIndex"] = asset->managingContractIndex;
    assetJson["ownershipIndex"] = asset->ownershipIndex;
    assetJson["numberOfUnits"] = std::to_string(asset->numberOfShares);
    return assetJson;
}

class LiveController : public HttpController<LiveController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(LiveController::assetsIssuances, "/assets/issuances", Get);
    ADD_METHOD_TO(LiveController::assetsIssuancesIndex, "/assets/issuances/{index}", Get);
    ADD_METHOD_TO(LiveController::assetsOwnerships, "/assets/ownerships", Get);
    ADD_METHOD_TO(LiveController::assetsOwnershipsIndex, "/assets/ownerships/{index}", Get);
    ADD_METHOD_TO(LiveController::assetsPossessions, "/assets/possessions", Get);
    ADD_METHOD_TO(LiveController::assetsPossessionsIndex, "/assets/possessions/{index}", Get);
    ADD_METHOD_TO(LiveController::assetsIdentityIssued, "/assets/{identity}/issued", Get);
    ADD_METHOD_TO(LiveController::assetsIdentityOwned, "/assets/{identity}/owned", Get);
    ADD_METHOD_TO(LiveController::assetsIdentityPossessed, "/assets/{identity}/possessed", Get);
    ADD_METHOD_TO(LiveController::balancesId, "/balances/{id}", Get);
    ADD_METHOD_TO(LiveController::tickInfo, "/block-height", Get);
    ADD_METHOD_TO(LiveController::tickInfo, "/tick-info", Get);
    ADD_METHOD_TO(LiveController::broadcastTransaction, "/broadcast-transaction", Post);
    METHOD_LIST_END

    inline void assetsIssuances(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb)
    {
        auto issuerIdentity = req->getParameter("issuerIdentity");
        auto assetName = req->getParameter("assetName");
        Json::Value result;
        Json::Value assetsArray(Json::arrayValue);

        for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.issuance.type == ISSUANCE)
            {
                auto &asset = assets[i].varStruct.issuance;
                CHAR16 identity[61] = {};
                getIdentity((unsigned char *)&asset.publicKey, identity, false);
                std::string identityStr = wchar_to_string(identity);
                std::string assetNameStr = std::string(asset.name);

                if ((!issuerIdentity.empty() && identityStr != issuerIdentity) ||
                    (!assetName.empty() && assetNameStr != assetName))
                {
                    continue;
                }

                Json::Value root;
                Json::Value assetJson = issuanceToJson((AssetIssuanceType*)&asset);
                root["data"] = assetJson;
                assetsArray.append(root);
            }
        }
        result["assets"] = assetsArray;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsIssuancesIndex(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &indexStr)
    {
        Json::Value result;
        unsigned int index = std::stoul(indexStr);
        if (index >= ASSETS_CAPACITY)
        {
            result["error"] = "Index out of range";
            cb(HttpResponse::newHttpJsonResponse(result));
            return;
        }

        if (assets[index].varStruct.issuance.type != ISSUANCE)
        {
            result["error"] = "No asset issuance at the given index";
            cb(HttpResponse::newHttpJsonResponse(result));
            return;
        }

        auto &asset = assets[index].varStruct.issuance;
        Json::Value assetJson = issuanceToJson((AssetIssuanceType*)&asset);
        result["data"] = assetJson;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsOwnerships(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb)
    {
        auto issuerIdentity = req->getParameter("issuerIdentity");
        auto assetName = req->getParameter("assetName");
        auto ownerIdentity = req->getParameter("ownerIdentity");
        int64_t ownershipManagingContract = stoll(req->getParameter("ownershipManagingContract"));
        Json::Value result;
        Json::Value assetsArray(Json::arrayValue);

        m256i issuerPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(issuerIdentity.c_str()), issuerPublicKey.m256i_u8);
        auto targetIssuanceIndex = issuanceIndex(issuerPublicKey, assetNameFromString(assetName.c_str()));
        for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.ownership.type == OWNERSHIP)
            {
                auto &asset = assets[i].varStruct.ownership;
                CHAR16 identity[61] = {};
                getIdentity((unsigned char *)&asset.publicKey, identity, false);
                std::string identityStr = wchar_to_string(identity);

                if ((!ownerIdentity.empty() && identityStr != ownerIdentity) ||
                    (ownershipManagingContract >= 0 && asset.managingContractIndex != ownershipManagingContract) ||
                    (targetIssuanceIndex >= 0 && asset.issuanceIndex != (unsigned int)targetIssuanceIndex))
                {
                    continue;
                }

                Json::Value root;
                Json::Value assetJson = ownershipToJson((AssetOwnershipType*)&asset);
                // TODO: fill these info
                root["tick"] = 0;
                root["universeIndex"] = 0;
                root["data"] = assetJson;
                assetsArray.append(root);
            }
        }
        result["assets"] = assetsArray;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsOwnershipsIndex(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &indexStr)
    {
        Json::Value result;
        unsigned int index = std::stoul(indexStr);
        if (index >= ASSETS_CAPACITY)
        {
            result["error"] = "Index out of range";
            cb(HttpResponse::newHttpJsonResponse(result));
            return;
        }

        if (assets[index].varStruct.ownership.type != OWNERSHIP)
        {
            result["error"] = "No asset ownership at the given index";
            cb(HttpResponse::newHttpJsonResponse(result));
            return;
        }

        auto &asset = assets[index].varStruct.ownership;
        Json::Value assetJson = ownershipToJson((AssetOwnershipType*)&asset);
        result["data"] = assetJson;
        // TODO: fill these info
        result["tick"] = 0;
        result["universeIndex"] = 0;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsPossessions(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb)
    {
        auto issuerIdentity = req->getParameter("issuerIdentity");
        auto assetName = req->getParameter("assetName");
        auto ownerIdentity = req->getParameter("ownerIdentity");
        auto possessorIdentity = req->getParameter("possessorIdentity");
        int64_t ownershipManagingContract = stoll(req->getParameter("ownershipManagingContract"));
        int64_t possessionManagingContract = stoll(req->getParameter("possessionManagingContract"));
        Json::Value result;
        Json::Value assetsArray(Json::arrayValue);

        m256i issuerPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(issuerIdentity.c_str()), issuerPublicKey.m256i_u8);
        auto targetIssuanceIndex = issuanceIndex(issuerPublicKey, assetNameFromString(assetName.c_str()));

        for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.possession.type == POSSESSION)
            {
                auto &asset = assets[i].varStruct.possession;
                CHAR16 identity[61] = {};
                getIdentity((unsigned char *)&asset.publicKey, identity, false);
                std::string identityStr = wchar_to_string(identity);
                unsigned short currentOwnershipManagingContractIndex = assets[asset.ownershipIndex].varStruct.ownership.managingContractIndex;
                unsigned int currentIssuanceIndex = assets[asset.ownershipIndex].varStruct.ownership.issuanceIndex;
                if ((!possessorIdentity.empty() && identityStr != possessorIdentity) ||
                    (!ownerIdentity.empty() && identityStr != ownerIdentity) ||
                    (ownershipManagingContract >= 0 && currentOwnershipManagingContractIndex != ownershipManagingContract) ||
                    (possessionManagingContract >= 0 && asset.managingContractIndex != possessionManagingContract) ||
                    (targetIssuanceIndex >= 0 && currentIssuanceIndex != (unsigned int)targetIssuanceIndex))
                {
                    continue;
                }

                Json::Value root;
                Json::Value assetJson = possessionToJson((AssetPossessionType*)&asset);
                root["data"] = assetJson;
                // TODO: fill these info
                root["tick"] = 0;
                root["universeIndex"] = 0;
                assetsArray.append(root);
            }
            result["assets"] = assetsArray;
        }

        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsPossessionsIndex(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &indexStr)
    {
        Json::Value result;
        unsigned int index = std::stoul(indexStr);
        if (index >= ASSETS_CAPACITY)
        {
            result["error"] = "Index out of range";
            cb(HttpResponse::newHttpJsonResponse(result));
            return;
        }

        if (assets[index].varStruct.possession.type != POSSESSION)
        {
            result["error"] = "No asset possession at the given index";
            cb(HttpResponse::newHttpJsonResponse(result));
            return;
        }

        auto &asset = assets[index].varStruct.possession;
        Json::Value assetJson = possessionToJson((AssetPossessionType*)&asset);
        result["data"] = assetJson;
        // TODO: fill these info
        result["tick"] = 0;
        result["universeIndex"] = 0;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsIdentityIssued(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &identityStr)
    {
        Json::Value result;
        Json::Value assetsArray(Json::arrayValue);

        m256i identityPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(identityStr.c_str()), identityPublicKey.m256i_u8);

        for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.issuance.type == ISSUANCE)
            {
                auto &asset = assets[i].varStruct.issuance;

                if (asset.publicKey != identityPublicKey)
                {
                    continue;
                }

                Json::Value root;
                Json::Value assetJson = issuanceToJson((AssetIssuanceType*)&asset);
                root["data"] = assetJson;
                Json::Value info(Json::objectValue);
                info["tick"] = 0; // TODO: fill tick
                info["universeIndex"] = 0; // TODO: fill universe index
                root["info"] = info;
                assetsArray.append(root);
            }
        }
        result["issuedAssets"] = assetsArray;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsIdentityOwned(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &identityStr)
    {
        Json::Value result;
        Json::Value assetsArray(Json::arrayValue);

        m256i identityPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(identityStr.c_str()), identityPublicKey.m256i_u8);

        for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.ownership.type == OWNERSHIP)
            {
                auto &asset = assets[i].varStruct.ownership;
                auto issuanceAsset = &assets[asset.issuanceIndex].varStruct.issuance;
                if (asset.publicKey != identityPublicKey)
                {
                    continue;
                }

                Json::Value root;
                Json::Value assetJson = ownershipToJson((AssetOwnershipType*)&asset);
                assetJson["issuedAsset"] = issuanceToJson((AssetIssuanceType*)issuanceAsset);
                root["data"] = assetJson;
                Json::Value info(Json::objectValue);
                info["tick"] = 0; // TODO: fill tick
                info["universeIndex"] = 0; // TODO: fill universe index
                root["info"] = info;
                assetsArray.append(root);
            }
        }
        result["ownedAssets"] = assetsArray;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void assetsIdentityPossessed(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &identityStr)
    {
        Json::Value result;
        Json::Value assetsArray(Json::arrayValue);

        m256i identityPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(identityStr.c_str()), identityPublicKey.m256i_u8);

        for (unsigned int i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.possession.type == POSSESSION)
            {
                auto &asset = assets[i].varStruct.possession;
                auto ownershipAsset = &assets[asset.ownershipIndex].varStruct.ownership;
                auto issuanceAsset = &assets[ownershipAsset->issuanceIndex].varStruct.issuance;
                if (asset.publicKey != identityPublicKey)
                {
                    continue;
                }

                Json::Value root;
                Json::Value assetJson = possessionToJson((AssetPossessionType*)&asset);
                assetJson["ownedAsset"] = ownershipToJson((AssetOwnershipType*)ownershipAsset);
                assetJson["ownedAsset"]["issuedAsset"] = issuanceToJson((AssetIssuanceType*)issuanceAsset);
                root["data"] = assetJson;
                Json::Value info(Json::objectValue);
                info["tick"] = 0; // TODO: fill tick
                info["universeIndex"] = 0; // TODO: fill universe index
                root["info"] = info;
                assetsArray.append(root);
            }
        }
        result["possessedAssets"] = assetsArray;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void balancesId(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb,
                                const std::string &idStr)
    {
        Json::Value result;
        Json::Value balance;
        m256i identityPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(idStr.c_str()), identityPublicKey.m256i_u8);
        auto spectrumInfo = spectrum[spectrumIndex(identityPublicKey)];
        balance["id"] = idStr;
        balance["balance"] = std::to_string(spectrumInfo.outgoingAmount - spectrumInfo.incomingAmount);
        balance["validForTick"] = 0;
        balance["latestIncomingTransferTick"] = spectrumInfo.latestIncomingTransferTick;
        balance["latestOutgoingTransferTick"] = spectrumInfo.latestOutgoingTransferTick;
        balance["incomingAmount"] = std::to_string(spectrumInfo.incomingAmount);
        balance["outgoingAmount"] = std::to_string(spectrumInfo.outgoingAmount);
        balance["numberOfIncomingTransfers"] = spectrumInfo.numberOfIncomingTransfers;
        balance["numberOfOutgoingTransfers"] = spectrumInfo.numberOfOutgoingTransfers;
        result["balance"] = balance;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void tickInfo(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value json;
        json["epoch"] = system.epoch;
        json["tick"] = system.tick;
        json["initialTick"] = system.initialTick;
        json["alignedVotes"] = gTickNumberOfComputors;
        json["misalignedVotes"] = gTickTotalNumberOfComputors - gTickNumberOfComputors;
        json["mainAuxStatus"] = mainAuxStatus;
        auto resp = HttpResponse::newHttpJsonResponse(json);
        cb(resp);
    }

    inline void broadcastTransaction(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        try
        {
            auto json = req->getJsonObject();
            if (!json)
            {
                result["error"] = "Invalid JSON";
                cb(HttpResponse::newHttpJsonResponse(result));
                return;
            }

            std::string txBase64 = (*json)["encodedTransaction"].asString();
            // decode base64
            auto txData = base64_decode(txBase64);
            Transaction tx;
            if (txData.size() != sizeof(Transaction))
            {
                result["error"] = "Invalid transaction size";
                cb(HttpResponse::newHttpJsonResponse(result));
                return;
            }
            copyMem(&tx, txData.data(), sizeof(Transaction));
            if (!tx.checkValidity())
            {
                result["error"] = "Invalid validity";
                cb(HttpResponse::newHttpJsonResponse(result));
                return;
            }

            //BROADCAST
        }
        catch (const std::exception &e)
        {
            result["error"] = "Exception: " + std::string(e.what());
            cb(HttpResponse::newHttpJsonResponse(result));
        }
    }
};