#pragma once

#include <drogon/HttpController.h>

#include "trdp_engine.hpp"

class TrdpController : public drogon::HttpController<TrdpController> {
public:
    static void setEngine(trdp::TrdpEngine *engine);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TrdpController::getPdTelegrams, "/api/pd/telegrams", drogon::Get, drogon::Options);
    ADD_METHOD_TO(TrdpController::listConfigs, "/api/configs", drogon::Get, drogon::Options);
    ADD_METHOD_TO(TrdpController::loadConfig, "/api/configs/load", drogon::Post, drogon::Options);
    ADD_METHOD_TO(TrdpController::enablePd, "/api/pd/{com_id}/enable", drogon::Post, drogon::Options);
    ADD_METHOD_TO(TrdpController::setPdValues, "/api/pd/{com_id}/values", drogon::Patch, drogon::Options);
    METHOD_LIST_END

    void getPdTelegrams(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void listConfigs(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void loadConfig(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const;

    void enablePd(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  uint32_t com_id) const;

    void setPdValues(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     uint32_t com_id) const;

private:
    static trdp::TrdpEngine *engine_;
};
