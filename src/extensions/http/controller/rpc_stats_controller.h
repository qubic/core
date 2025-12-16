#pragma once
#include "../utils.h"
#include "extensions/utils.h"

#include <cmath>
#include <drogon/HttpController.h>

using namespace drogon;

namespace RpcStats
{
class RpcStatsController : public HttpController<RpcStatsController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RpcStatsController::queryAssetOwners, "/v1/issuers/{issuerIdentity}/assets/{assetName}/owners", Get);
    ADD_METHOD_TO(RpcStatsController::latestStats, "/v1/latest-stats", Get);
    ADD_METHOD_TO(RpcStatsController::richList, "/v1/rich-list", Get);
    METHOD_LIST_END

    inline void queryAssetOwners(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&cb,
                                 const std::string &issuerIdentity,
                                 const std::string &assetName)
    {
        Json::Value result;
        Json::Value ownersArray(Json::arrayValue);
        m256i issuerPublicKey;
        getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(issuerIdentity.c_str()), issuerPublicKey.m256i_u8);
        auto targetIssuanceIndex = issuanceIndex(issuerPublicKey, HttpUtils::assetNameFromString(assetName.c_str()));

        // extract page,pageSize from query parameters
        long long page = 0;
        long long pageSize = 10;
        long long currentIndex = 0;
        if (req->getParameter("page") != "")
        {
            page = std::stoll(req->getParameter("page"));
        }
        if (req->getParameter("pageSize") != "")
        {
            pageSize = std::stoll(req->getParameter("pageSize"));
        }

        for (unsigned long long i = 0; i < ASSETS_CAPACITY; i++)
        {
            if (assets[i].varStruct.ownership.type == OWNERSHIP)
            {
                auto &asset = assets[i].varStruct.ownership;
                unsigned short currentManagingContractIndex = asset.managingContractIndex;
                unsigned int currentIssuanceIndex = asset.issuanceIndex;
                if (targetIssuanceIndex >= 0 && currentIssuanceIndex != targetIssuanceIndex)
                {
                    continue;
                }
                // apply pagination
                if (currentIndex < (page + 1) * pageSize && currentIndex >= page * pageSize)
                {
                    CHAR16 identity[61] = {};
                    getIdentity((unsigned char *)&asset.publicKey, identity, false);
                    std::string identityStr = wchar_to_string(identity);
                    Json::Value ownerJson;
                    ownerJson["identity"] = identityStr;
                    ownerJson["numberOfShares"] = std::to_string(asset.numberOfShares);
                    ownersArray.append(ownerJson);
                }
                currentIndex++;
            }
        }

        Json::Value pagination;
        pagination["totalRecords"] = Json::UInt64(currentIndex);
        pagination["currentPage"] = Json::UInt64(page);
        pagination["totalPages"] = Json::UInt64(std::ceil((float)currentIndex / pageSize));
        pagination["pageSize"] = Json::UInt64(pageSize);

        result["pagination"] = pagination;
        result["owners"] = ownersArray;
        result["tick"] = system.tick;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void latestStats(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        Json::Value data;
        TickStorage::tickData.acquireLock();
        TickData *tickData = TickStorage::tickData.getByTickIfNotEmpty(system.tick - 1);
        if (tickData)
        {
            data["timestamp"] = HttpUtils::formatTimestamp(
                tickData->millisecond,
                tickData->second,
                tickData->minute,
                tickData->hour,
                tickData->day,
                tickData->month,
                tickData->year);
        } else
        {
            data["timestamp"] = "0";
        }
        TickStorage::tickData.releaseLock();

        data["circulatingSupply"] = Json::UInt64(spectrumInfo.totalAmount);
        data["activeAddresses"] = spectrumInfo.numberOfEntities;
        data["price"] = 0;
        data["marketCap"] = "0";
        data["epoch"] = system.epoch;
        data["currentTick"] = system.tick;
        data["ticksInCurrentEpoch"] = system.tick - system.initialTick;
        unsigned int emptyTicks = 0;
        {
            TickStorage::tickData.acquireLock();
           for (unsigned int tick = system.initialTick; tick < system.tick; tick++)
           {
               TickData *tickData = TickStorage::tickData.getByTickIfNotEmpty(tick);
               if (!tickData)
               {
                   emptyTicks++;
               }
           }
            TickStorage::tickData.releaseLock();
        }
        data["emptyTicksInCurrentEpoch"] = emptyTicks;
        data["epochTickQuality"] = system.tick - system.initialTick == 0 ? 0 : std::roundf((float)(system.tick - system.initialTick - emptyTicks) / (float)(system.tick - system.initialTick) * 100000.0f) / 100000.0f;
        data["burnedQus"] = 0;
        result["data"] = data;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void richList(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        Json::Value richListArray(Json::arrayValue);

        // extract page,pageSize from query parameters
        long long page = 0;
        long long pageSize = 10;
        if (req->getParameter("page") != "")
        {
            page = std::stoll(req->getParameter("page"));
        }
        if (req->getParameter("pageSize") != "")
        {
            pageSize = std::stoll(req->getParameter("pageSize"));
        }

        // create a vector of pairs to sort
        std::vector<std::pair<m256i, long long>> balances;
        for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
        {
            const long long balance = spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
            if (balance > 0)
            {
                balances.emplace_back(spectrum[i].publicKey, balance);
            }
        }

        // sort the vector by balance in descending order
        std::sort(balances.begin(), balances.end(),
                  [](const std::pair<m256i, long long> &a, const std::pair<m256i, long long> &b)
                  {
                      return a.second > b.second;
                  });

        // apply pagination
        long long start = page * pageSize;
        long long end = std::min(start + pageSize, (long long)balances.size());
        for (long long i = start; i < end; i++)
        {
            Json::Value entry;
            CHAR16 identity[61] = {};
            getIdentity((unsigned char *)&balances[i].first, identity, false);
            std::string identityStr = wchar_to_string(identity);
            entry["identity"] = identityStr;
            entry["balance"] = std::to_string(balances[i].second);
            richListArray.append(entry);
        }

        Json::Value pagination;
        pagination["totalRecords"] = Json::UInt64(balances.size());
        pagination["currentPage"] = Json::UInt64(page);
        pagination["totalPages"] = Json::UInt64(std::ceil((float)balances.size() / pageSize));
        pagination["pageSize"] = Json::UInt64(pageSize);
        result["pagination"] = pagination;
        result["richList"]["entities"] = richListArray;
        result["epoch"] = system.epoch;
        cb(HttpResponse::newHttpJsonResponse(result));
    }
};
}

