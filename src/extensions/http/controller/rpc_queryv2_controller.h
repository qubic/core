#pragma once
#include "extensions/utils.h"
#include "../utils.h"
#include <drogon/HttpController.h>
#include <fmt/format.h>
using namespace drogon;

namespace RpcQueryV2
{

enum StatusCode
{
    BadRequest = 3,
    NotFound = 5,
};

namespace MiddleWare
{
class TickNumberVerifier : public HttpMiddleware<TickNumberVerifier>
{
public:
    TickNumberVerifier() {}

    void invoke(const HttpRequestPtr &req,
                MiddlewareNextCallback &&nextCb,
                MiddlewareCallback &&mcb) override
    {
        Json::Value result;
        auto json = req->getJsonObject();
        if (!json)
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = "Invalid JSON";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            mcb(res);
            return;
        }

        // check if tickNumber field exists
        if (!(*json).isMember("tickNumber"))
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = "Missing tickNumber field";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            mcb(res);
            return;
        }

        unsigned int tickNumber = (*json)["tickNumber"].asUInt64();
        if (tickNumber > system.tick)
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = fmt::format("invalid tick number: rpc error: code = FailedPrecondition desc = requested tick number {} is greater than last processed tick {}", tickNumber, system.tick);
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            mcb(res);
            return;
        } else if (tickNumber < system.initialTick)
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = fmt::format("invalid tick number: rpc error: code = OutOfRange desc = provided tick number {} was skipped by the system, next available tick is {}", tickNumber, system.initialTick);
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            mcb(res);
            return;
        }

        nextCb([mcb = std::move(mcb)](const HttpResponsePtr &resp) { mcb(resp); });
    }
};
}

class RpcQueryV2Controller : public HttpController<RpcQueryV2Controller>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RpcQueryV2Controller::getComputorListsForEpoch, "/getComputorListsForEpoch", Post);
    ADD_METHOD_TO(RpcQueryV2Controller::getLastProcessedTick, "/getLastProcessedTick", Get);
    ADD_METHOD_TO(RpcQueryV2Controller::getProcessedTickIntervals, "/getProcessedTickIntervals", Get);
    ADD_METHOD_TO(RpcQueryV2Controller::getTickData, "/getTickData", Post, "RpcQueryV2::MiddleWare::TickNumberVerifier");
    ADD_METHOD_TO(RpcQueryV2Controller::getTransactionByHash, "/getTransactionByHash", Post);
    ADD_METHOD_TO(RpcQueryV2Controller::getTransactionsForIdentity, "/getTransactionsForIdentity", Post);
    ADD_METHOD_TO(RpcQueryV2Controller::getTransactionsForTick, "/getTransactionsForTick", Post, "RpcQueryV2::MiddleWare::TickNumberVerifier");
    METHOD_LIST_END

    inline void getComputorListsForEpoch(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        auto json = req->getJsonObject();
        if (!json)
        {
            result["code"] = -1;
            result["message"] = "Invalid JSON";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }

        // check if epoch field exists
        if (!(*json).isMember("epoch"))
        {
            result["code"] = -1;
            result["message"] = "Missing epoch field";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
            return;
        }

        unsigned int epoch = (*json)["epoch"].asUInt64();
        Json::Value computorLists(Json::arrayValue);
        Json::Value computorObject;
        Json::Value idLists(Json::arrayValue);
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            m256i &pubKey = broadcastedComputors.computors.publicKeys[i];
            CHAR16 id[61] = {};
            getIdentity((const unsigned char *)&pubKey, id, false);
            idLists.append(wchar_to_string(id));
        }
        computorObject["epoch"] = epoch;
        computorObject["tickNumber"] = 0;
        computorObject["identities"] = idLists;
        computorObject["signature"] = base64_encode(broadcastedComputors.computors.signature, SIGNATURE_SIZE);
        computorLists.append(computorObject);
        result["computorsLists"] = computorLists;

        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void getLastProcessedTick(const HttpRequestPtr &req,
                                     std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        result["tickNumber"] = system.tick;
        result["epoch"] = system.epoch;
        result["intervalInitialTick"] = system.initialTick;
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void getProcessedTickIntervals(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result(Json::arrayValue);
        Json::Value tickInterval;
        tickInterval["epoch"] = system.epoch;
        tickInterval["firstTick"] = system.initialTick;
        tickInterval["lastTick"] = system.tick;
        result.append(tickInterval);
        cb(HttpResponse::newHttpJsonResponse(result));
    }

    inline void getTickData(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        auto json = req->getJsonObject();
        unsigned int tickNumber = (*json)["tickNumber"].asUInt64();
        TickData localTickData;
        TickStorage::tickData.acquireLock();
        TickData *tickData = TickStorage::tickData.getByTickIfNotEmpty(tickNumber);
        if (tickData)
        {
            copyMem(&localTickData, tickData, sizeof(TickData));
        }
        TickStorage::tickData.releaseLock();
        if (!tickData)
        {
            result["code"] = StatusCode::NotFound;
            result["message"] = "Tick data not found";
            auto resp = HttpResponse::newHttpJsonResponse(result);
            resp->setStatusCode(k404NotFound);
            cb(resp);
            return;
        }

        Json::Value jsonObject;
        jsonObject["tickNumber"] = localTickData.tick;
        jsonObject["epoch"] = localTickData.epoch;
        jsonObject["computorIndex"] = localTickData.computorIndex;
        jsonObject["timelock"] = base64_encode(localTickData.timelock.m256i_u8, 32);
        jsonObject["timestamp"] = HttpUtils::formatTimestamp(
            localTickData.millisecond,
            localTickData.second,
            localTickData.minute,
            localTickData.hour,
            localTickData.day,
            localTickData.month,
            localTickData.year);
        jsonObject["varStruct"] = "";
        Json::Value txDigestsJson(Json::arrayValue);
        for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
        {
            if (localTickData.transactionDigests[i] != m256i::zero())
            {
                CHAR16 id[61] = {};
                getIdentity((unsigned char *)&localTickData.transactionDigests[i], id, true);
                txDigestsJson.append(wchar_to_string(id));
            }
        }
        jsonObject["transactionDigests"] = txDigestsJson;
        Json::Value contractFeesJson(Json::arrayValue);
        for (unsigned int i = 0; i < MAX_NUMBER_OF_CONTRACTS; i++)
        {
            contractFeesJson.append(Json::UInt64(localTickData.contractFees[i]));
        }
        jsonObject["contractFees"] = contractFeesJson;
        jsonObject["signature"] = base64_encode(localTickData.signature, SIGNATURE_SIZE);

        auto resp = HttpResponse::newHttpJsonResponse(jsonObject);
        cb(resp);
    }

    inline void getTransactionByHash(const HttpRequestPtr &req,
                                     std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        auto json = req->getJsonObject();
        if (!json)
        {
            result["code"] = -1;
            result["message"] = "Invalid JSON";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }

        // check if txHash field exists
        if (!(*json).isMember("hash"))
        {
            result["code"] = -1;
            result["message"] = "Missing hash field";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }

        std::string txHash = (*json)["hash"].asString();
        if (txHash.length() != 60)
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = fmt::format("invalid id format: converting id to pubkey: invalid ID length, expected 60, found {}", txHash.length());
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }
        // To upperse txhash
        std::transform(txHash.begin(), txHash.end(), txHash.begin(), ::toupper);
        m256i txDigest = {};
        if (!getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(txHash.c_str()), txDigest.m256i_u8))
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = fmt::format("invalid id format: invalid hash [{}]", txHash);
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }
        TickStorage::transactionsDigestAccess.acquireLock();
        const Transaction *transaction = TickStorage::transactionsDigestAccess.findTransaction(txDigest);
        if (!transaction)
        {
            result["code"] = StatusCode::NotFound;
            result["message"] = "Transaction not found";
            auto resp = HttpResponse::newHttpJsonResponse(result);
            resp->setStatusCode(k404NotFound);
            cb(resp);
            TickStorage::transactionsDigestAccess.releaseLock();
            return;
        }
        Json::Value jsonObject = HttpUtils::transactionToJson(const_cast<Transaction *>(transaction));
        auto resp = HttpResponse::newHttpJsonResponse(jsonObject);
        cb(resp);
        TickStorage::transactionsDigestAccess.releaseLock();
    }

    inline void getTransactionsForIdentity(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&cb)
    {
        try
        {
            Json::Value result;
        auto json = req->getJsonObject();
        if (!json)
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = "Invalid JSON";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }

        // check if identity field exists
        if (!(*json).isMember("identity"))
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = "Missing identity field";
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }

        // type: map<string,string>
        auto filters = (*json)["filters"];
        // type: map<string, Range>
        auto ranges = (*json)["ranges"];
        // type
        // {
        //     offset: number,
        //     size: number
        // }
        auto pagination = (*json)["pagination"];

        std::string identityStr = (*json)["identity"].asString();
        m256i publicKey{};
        if (!getPublicKeyFromIdentity(reinterpret_cast<const unsigned char *>(identityStr.c_str()), publicKey.m256i_u8))
        {
            result["code"] = StatusCode::BadRequest;
            result["message"] = fmt::format("invalid id format: invalid identity [{}]", identityStr);
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k400BadRequest);
            cb(res);
            return;
        }

        Json::Value transactions(Json::arrayValue);
        TickStorage::transactionsDigestAccess.acquireLock();
        for (unsigned int tick = system.initialTick; tick <= system.tick; tick++)
        {
            TickData *tickData = TickStorage::tickData.getByTickIfNotEmpty(tick);
            if (!tickData)
            {
                continue;
            }

            for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
            {
                m256i &txDigest = tickData->transactionDigests[i];
                if (isZero(txDigest))
                {
                    continue;
                }

                const Transaction *transaction = TickStorage::transactionsDigestAccess.findTransaction(txDigest);
                if (!transaction)
                {
                    continue;
                }

                if (transaction->sourcePublicKey == publicKey)
                {
                    Json::Value txJson = HttpUtils::transactionToJson((Transaction *)transaction);
                    transactions.append(txJson);
                }
            }
        }
        TickStorage::transactionsDigestAccess.releaseLock();

        // filter transactions based on filters
        if (filters.isObject())
        {
            Json::Value filteredTransactions(Json::arrayValue);
            for (unsigned int i = 0; i < transactions.size(); i++)
            {
                Json::Value tx = transactions[i];
                bool match = true;
                for (const auto &key : filters.getMemberNames())
                {
                    if (tx.isMember(key))
                    {
                        if (tx[key].asString() != filters[key].asString())
                        {
                            match = false;
                            break;
                        }
                    }
                }
                if (match)
                {
                    filteredTransactions.append(tx);
                }
            }
            transactions = filteredTransactions;
        }
        // filter transactions based on ranges
        if (ranges.isObject())
        {
            Json::Value rangedTransactions(Json::arrayValue);
            for (unsigned int i = 0; i < transactions.size(); i++)
            {
                Json::Value tx = transactions[i];
                bool match = true;
                for (const auto &key : ranges.getMemberNames())
                {
                    if (tx.isMember(key))
                    {
                        Json::Value range = ranges[key];
                        if (range.isObject())
                        {
                            if (range.isMember("lt"))
                            {
                                if (!(stoull(tx[key].asString()) < stoull(range["lt"].asString())))
                                {
                                    match = false;
                                    break;
                                }
                            }
                            if (range.isMember("gt"))
                            {
                                if (!(stoull(tx[key].asString()) > stoull(range["gt"].asString())))
                                {
                                    match = false;
                                    break;
                                }
                            }
                            if (range.isMember("lte"))
                            {
                                if (!(stoull(tx[key].asString()) <= stoull(range["lte"].asString())))
                                {
                                    match = false;
                                    break;
                                }
                            }
                            if (range.isMember("gte"))
                            {
                                if (!(stoull(tx[key].asString()) >= stoull(range["gte"].asString())))
                                {
                                    match = false;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (match)
                {
                    rangedTransactions.append(tx);
                }
            }
            transactions = rangedTransactions;
        }
        // apply pagination
        if (pagination.isObject())
        {
            unsigned int offset = 0;
            unsigned int size = 0;
            if (pagination.isMember("offset"))
            {
                offset = pagination["offset"].asUInt64();
            }
            offset = std::min(offset, (unsigned int)10000);
            if (pagination.isMember("size"))
            {
                size = pagination["size"].asUInt64();
            } else
            {
                size = 10;
            }
            size = std::min(size, (unsigned int)1000);
            Json::Value paginatedTransactions(Json::arrayValue);
            for (unsigned int i = offset; i < transactions.size() && i < offset + size; i++)
            {
                paginatedTransactions.append(transactions[i]);
            }
            transactions = paginatedTransactions;
        }

        result["transactions"] = transactions;
        result["validForTick"] = 0;
        result["hits"]["total"] = transactions.size();
        result["hits"]["from"] = 0;
        result["hits"]["size"] = transactions.size();
        auto resp = HttpResponse::newHttpJsonResponse(result);
        cb(resp);
        } catch (const std::exception &e)
        {
            Json::Value result;
            result["code"] = -1;
            result["message"] = std::string("Internal server error: ") + e.what();
            auto res = HttpResponse::newHttpJsonResponse(result);
            res->setStatusCode(k500InternalServerError);
            cb(res);
        }
    }

    inline void getTransactionsForTick(const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&cb)
    {
        Json::Value result;
        auto json = req->getJsonObject();
        unsigned int tickNumber = (*json)["tickNumber"].asUInt64();
        TickData localTickData;
        TickStorage::tickData.acquireLock();
        TickData *tickData = TickStorage::tickData.getByTickIfNotEmpty(tickNumber);
        if (tickData)
        {
            copyMem(&localTickData, tickData, sizeof(TickData));
        }
        TickStorage::tickData.releaseLock();
        if (!tickData)
        {
            result["code"] = StatusCode::NotFound;
            result["message"] = "Tick data not found";
            auto resp = HttpResponse::newHttpJsonResponse(result);
            resp->setStatusCode(k404NotFound);
            cb(resp);
            return;
        }

        Json::Value transactions(Json::arrayValue);
        TickStorage::transactionsDigestAccess.acquireLock();
        for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
        {
            m256i &txDigest = localTickData.transactionDigests[i];
            if (isZero(txDigest))
            {
                continue;
            }

            const Transaction *transaction = TickStorage::transactionsDigestAccess.findTransaction(txDigest);
            if (!transaction)
            {
                continue;
            }

            Json::Value txJson = HttpUtils::transactionToJson((Transaction *)transaction);
            transactions.append(txJson);
        }
        TickStorage::transactionsDigestAccess.releaseLock();
        result["transactions"] = transactions;
        auto resp = HttpResponse::newHttpJsonResponse(result);
        cb(resp);
    }
};
} // namespace RpcQueryV2