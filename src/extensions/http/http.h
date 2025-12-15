#pragma once

static unsigned long long httpPasscodes[4] = {};

#ifdef __linux__

#include <drogon/drogon.h>
#include "controller/rpc_queryv2_controller.h"
#include "controller/rpc_live_controller.h"
#include "controller/rpc_stats_controller.h"
#include "ticking/tick_storage.h"

using namespace drogon;

namespace MiddleWare
{
class PasscodeVerifier : public HttpMiddleware<PasscodeVerifier>
{
public:
    PasscodeVerifier() {}

    void invoke(const HttpRequestPtr &req,
                MiddlewareNextCallback &&nextCb,
                MiddlewareCallback &&mcb) override
    {
        static std::string correctPasscode = std::to_string(httpPasscodes[0]) + "-" +
                                                std::to_string(httpPasscodes[1]) + "-" +
                                                    std::to_string(httpPasscodes[2]) + "-" +
                                                        std::to_string(httpPasscodes[3]);
        bool isPasscodeValid = req->getParameter("passcode") == correctPasscode;
        if (!isPasscodeValid)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid passcode");
            mcb(resp);
            return;
        }

        nextCb([mcb = std::move(mcb)](const HttpResponsePtr &resp) { mcb(resp); });
    }
};
}

class QubicHttpServer
{
private:
    static inline std::string hiddenFolder = ".qubic-tmp";

    static void __http_thread(int port)
    {
        HttpAppFramework &app = drogon::app();
        drogon::app()
            .addListener("0.0.0.0", port)
            .disableSigtermHandling();

        app.registerHandler(
            "/",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setBody("Hello, World!2");
                callback(resp);
            });

        app.registerHandler(
            "/running-ids",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                Json::Value json;
                Json::Value idsJson(Json::arrayValue);
                for (int i = 0; i < computorSeeds.size(); i++)
                {
                    CHAR16 id[61] = {};
                    m256i publicKey = {};
                    m256i privateKey = {};
                    m256i subseed = {};
                    bool isOk = getSubseed(reinterpret_cast<const unsigned char *>(computorSeeds[i].c_str()), subseed.m256i_u8);
                    if (!isOk)
                        continue;
                    getPrivateKey(subseed.m256i_u8, privateKey.m256i_u8);
                    getPublicKey(privateKey.m256i_u8, publicKey.m256i_u8);
                    getIdentity(publicKey.m256i_u8, id, false);
                    if (publicKey != computorPublicKeys[i])
                        continue;

                    idsJson.append(wchar_to_string(id));
                }
                json["runningIds"] = idsJson;
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/latest-created-tick-info",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                Json::Value json;
                CHAR16 id[61] = {};
                getIdentity((const unsigned char*)&latestCreatedTickInfo.id, id, false);
                json["tick"] = latestCreatedTickInfo.tick;
                json["epoch"] = latestCreatedTickInfo.epoch;
                json["numberOfTxs"] = latestCreatedTickInfo.numberOfTxs;
                json["id"] = wchar_to_string(id);
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/solutions",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                Json::Value json(Json::arrayValue);
                for (unsigned int i = 0; i < system.numberOfSolutions; i++)
                {
                    Json::Value solutionJson;
                    solutionJson["computorPublicKey"] = byteToHex((unsigned char *)&system.solutions[i].computorPublicKey, sizeof(m256i));
                    solutionJson["miningSeed"] = byteToHex((unsigned char *)&system.solutions[i].miningSeed, sizeof(m256i));
                    solutionJson["nonce"] = byteToHex((unsigned char *)&system.solutions[i].nonce, sizeof(m256i));
                    json.append(solutionJson);
                }
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/solution-publish-ticks",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                Json::Value json(Json::arrayValue);
                for (unsigned int i = 0; i < system.numberOfSolutions; i++)
                {
                    Json::Value jsonObject;
                    jsonObject["solutionIndex"] = i;
                    jsonObject["publishTick"] = solutionPublicationTicks[i];
                    json.append(jsonObject);
                }
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/tick-data/{1}",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback,
               const std::string &tickStr)
            {
                unsigned int tick = std::stoul(tickStr);
                TickData localTickData;
                TickStorage::tickData.acquireLock();
                TickData* tickData = TickStorage::tickData.getByTickIfNotEmpty(tick);
                if (tickData)
                {
                    copyMem(&localTickData, tickData, sizeof(TickData));
                }
                TickStorage::tickData.releaseLock();
                if (!tickData)
                {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k404NotFound);
                    resp->setBody("Tick data not found");
                    callback(resp);
                    return;
                }

                Json::Value json;
                json["tick"] = localTickData.tick;
                json["epoch"] = localTickData.epoch;
                json["computorIndex"] = localTickData.computorIndex;
                json["millisecond"] = localTickData.millisecond;
                json["second"] = localTickData.second;
                json["minute"] = localTickData.minute;
                json["hour"] = localTickData.hour;
                json["day"] = localTickData.day;
                json["month"] = localTickData.month;
                json["year"] = localTickData.year;
                json["timelock"] = byteToHex((unsigned char*)&localTickData.timelock, sizeof(m256i));
                Json::Value txDigestsJson(Json::arrayValue);
                for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
                {
                    if (localTickData.transactionDigests[i] != m256i::zero())
                        txDigestsJson.append(byteToHex((unsigned char*)&localTickData.transactionDigests[i], sizeof(m256i)));
                }
                json["transactionDigests"] = txDigestsJson;
                Json::Value contractFeesJson(Json::arrayValue);
                for (unsigned int i = 0; i < MAX_NUMBER_OF_CONTRACTS; i++)
                {
                    contractFeesJson.append(Json::UInt64(localTickData.contractFees[i]));
                }
                json["contractFees"] = contractFeesJson;
                json["signature"] = byteToHex((unsigned char*)localTickData.signature, SIGNATURE_SIZE);

                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/tick/{1}",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback,
               const std::string &tickStr)
            {
                unsigned int tick = std::stoul(tickStr);
                Tick localTicks[NUMBER_OF_COMPUTORS];
                setMem(localTicks, sizeof(localTicks), 0);
                for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                {
                    TickStorage::ticks.acquireLock(i);
                    Tick* tickVote = TickStorage::ticks.getByTickInCurrentEpoch(tick) + i;
                    if (tickVote)
                        copyMem(&localTicks[i], tickVote, sizeof(Tick));
                    TickStorage::ticks.releaseLock(i);
                }
                Json::Value json(Json::arrayValue);
                for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                {
                    if (isAllBytesZero(localTicks + i, sizeof(Tick)))
                        continue;

                    Json::Value tickJson;
                    tickJson["tick"] = localTicks[i].tick;
                    tickJson["epoch"] = localTicks[i].epoch;
                    tickJson["computorIndex"] = localTicks[i].computorIndex;
                    tickJson["millisecond"] = localTicks[i].millisecond;
                    tickJson["second"] = localTicks[i].second;
                    tickJson["minute"] = localTicks[i].minute;
                    tickJson["hour"] = localTicks[i].hour;
                    tickJson["day"] = localTicks[i].day;
                    tickJson["month"] = localTicks[i].month;
                    tickJson["year"] = localTicks[i].year;
                    tickJson["prevResourceTestingDigest"] = Json::UInt64(localTicks[i].prevResourceTestingDigest);
                    tickJson["saltedResourceTestingDigest"] = Json::UInt64(localTicks[i].saltedResourceTestingDigest);
                    tickJson["prevTransactionBodyDigest"] = Json::UInt64(localTicks[i].prevTransactionBodyDigest);
                    tickJson["saltedTransactionBodyDigest"] = Json::UInt64(localTicks[i].saltedTransactionBodyDigest);
                    tickJson["prevSpectrumDigest"] = byteToHex((unsigned char*)&localTicks[i].prevSpectrumDigest, sizeof(m256i));
                    tickJson["prevUniverseDigest"] = byteToHex((unsigned char*)&localTicks[i].prevUniverseDigest, sizeof(m256i));
                    tickJson["prevComputerDigest"] = byteToHex((unsigned char*)&localTicks[i].prevComputerDigest, sizeof(m256i));
                    tickJson["saltedSpectrumDigest"] = byteToHex((unsigned char*)&localTicks[i].saltedSpectrumDigest, sizeof(m256i));
                    tickJson["saltedUniverseDigest"] = byteToHex((unsigned char*)&localTicks[i].saltedUniverseDigest, sizeof(m256i));
                    tickJson["saltedComputerDigest"] = byteToHex((unsigned char*)&localTicks[i].saltedComputerDigest, sizeof(m256i));
                    tickJson["transactionDigest"] = byteToHex((unsigned char*)&localTicks[i].transactionDigest, sizeof(m256i));
                    tickJson["expectedNextTickTransactionDigest"] = byteToHex((unsigned char*)&localTicks[i].expectedNextTickTransactionDigest, sizeof(m256i));
                    tickJson["signature"] = byteToHex((unsigned char*)localTicks[i].signature, SIGNATURE_SIZE);
                    json.append(tickJson);
                }
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/transaction/{1}",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback,
               const std::string &txId)
            {
                m256i txDigest;
                hexToByte(txId, sizeof(m256i), (unsigned char*)&txDigest);
                Transaction localTransaction;
                TickStorage::transactionsDigestAccess.acquireLock();
                const Transaction* transaction = TickStorage::transactionsDigestAccess.findTransaction(txDigest);
                if (transaction)
                {
                    copyMem(&localTransaction, transaction, sizeof(Transaction));
                }
                TickStorage::transactionsDigestAccess.releaseLock();
                if (!transaction)
                {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k404NotFound);
                    resp->setBody("Transaction not found");
                    callback(resp);
                    return;
                }

                Json::Value json;
                CHAR16 humanId[61] = {0};
                getIdentity((const unsigned char*)&localTransaction.sourcePublicKey, humanId, false);
                json["sourcePublicKey"] = wchar_to_string(humanId);
                getIdentity((const unsigned char*)&localTransaction.destinationPublicKey, humanId, false);
                json["destinationPublicKey"] = wchar_to_string(humanId);
                json["amount"] = Json::UInt64(localTransaction.amount);
                json["tick"] = localTransaction.tick;
                json["inputType"] = localTransaction.inputType;
                json["inputSize"] = localTransaction.inputSize;
                json["inputData"] = byteToHex(localTransaction.inputPtr(), localTransaction.inputSize);
                json["signature"] = byteToHex(localTransaction.signaturePtr(), SIGNATURE_SIZE);

                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/request-save-snapshot",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                requestPersistingNodeState = 1;
                Json::Value json;
                json["status"] = "ok";
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            });

        app.registerHandler(
            "/spectrum",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                bool isZip = req->getParameter("zip") == "true";
                std::string path;
                if (isZip)
                {
                    path = hiddenFolder + "/" + "spectrum.zip";
                }
                else
                {
                    path = "spectrum." + std::to_string(system.epoch);
                }

                // create zip if not exists in .qubic-tmp/
                if (isZip && !std::filesystem::exists(path))
                {
                    // check if hidden folder exists
                    if (!std::filesystem::exists(hiddenFolder + "/"))
                    {
                        std::filesystem::create_directory(hiddenFolder);
                    }

                    std::string inputFile = "spectrum." + std::to_string(system.epoch);
                    std::string command = "zip -j " + path + " " + inputFile;
                    if (exec(command.c_str()) != 0)
                    {
                        auto resp = HttpResponse::newHttpResponse();
                        resp->setStatusCode(k500InternalServerError);
                        resp->setBody("Failed to create zip file");
                        callback(resp);
                        return;
                    }
                }

                auto resp = HttpResponse::newFileResponse(path);
                std::string fileName = isZip ? "spectrum.zip" : ("spectrum." + std::to_string(system.epoch));
                resp->addHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
                callback(resp);
            }, {drogon::Get, "MiddleWare::PasscodeVerifier"});

        app.registerHandler(
            "/universe",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                bool isZip = req->getParameter("zip") == "true";
                std::string path;
                if (isZip)
                {
                    path = hiddenFolder + "/" + "universe.zip";
                }
                else
                {
                    path = "universe." + std::to_string(system.epoch);
                }

                // create zip if not exists in .qubic-tmp/
                if (isZip && !std::filesystem::exists(path))
                {
                    // check if hidden folder exists
                    if (!std::filesystem::exists(hiddenFolder + "/"))
                    {
                        std::filesystem::create_directory(hiddenFolder);
                    }
                    std::string inputFile = "universe." + std::to_string(system.epoch);
                    std::string command = "zip -j " + path + " " + inputFile;
                    if (exec(command.c_str()) != 0)
                    {
                        Json::Value json;
                        json["error"] = "Failed to create zip file";
                        auto resp = HttpResponse::newHttpJsonResponse(json);
                        callback(resp);
                        return;
                    }
                }

                auto resp = HttpResponse::newFileResponse(path);
                std::string fileName = isZip ? "universe.zip" : ("universe." + std::to_string(system.epoch));
                resp->addHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
                callback(resp);
            }, {drogon::Get, "MiddleWare::PasscodeVerifier"});

        app.registerHandler(
            "/shutdown",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                shutDownNode = 1;
                Json::Value json;
                json["status"] = "ok";
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            }, {drogon::Get, "MiddleWare::PasscodeVerifier"});

        app.run();
    }
public:
    static void start(int port = 41841)
    {
        std::thread server_thread(__http_thread, port);
        server_thread.detach();
    }

    static void stop()
    {
        drogon::app().quit();
    }
};
#else
class QubicHttpServer
{
public:
	static void start(int port = 41841)
	{
		// No-op on non-Linux platforms
	}
	static void stop()
	{
		// No-op on non-Linux platforms
	}
};
#endif