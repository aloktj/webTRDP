#pragma once

#include <drogon/HttpController.h>
#include <trdp/core.hpp>

class TrdpController : public drogon::HttpController<TrdpController> {
public:
    static void setEngine(trdp::TrdpEngine *engine);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TrdpController::getPdTelegrams, "/api/pd/telegrams", drogon::Get);
    ADD_METHOD_TO(TrdpController::loadConfig, "/api/configs/load", drogon::Post, drogon::Options);
    METHOD_LIST_END

    void getPdTelegrams(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void loadConfig(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

private:
    static trdp::TrdpEngine *engine_;
};
