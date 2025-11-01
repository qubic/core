#pragma once

#include <drogon/drogon.h>

using namespace drogon;

class HttpServer
{
private:
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
                json["tick"] = system.tick;
                auto resp = HttpResponse::newHttpJsonResponse(json);
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