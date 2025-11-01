#pragma once

#include <drogon/drogon.h>

using namespace drogon;

class HttpServer
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
            "/tick-info",
            [](const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback)
            {
                Json::Value json;
                json["epoch"] = system.epoch;
                json["tick"] = system.tick;
                json["initialTick"] = system.initialTick;
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
            });

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
            });

        app.run();
    }
public:
    static void start(int port = 41841)
    {
        std::thread server_thread(__http_thread, port);
        server_thread.detach();
    }
};