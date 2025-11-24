#include "controllers/TrdpController.h"

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <ctime>

trdp::TrdpEngine *TrdpController::engine_ = nullptr;

void TrdpController::setEngine(trdp::TrdpEngine *engine) {
    engine_ = engine;
}

void TrdpController::getPdTelegrams(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
    Json::Value telegrams(Json::arrayValue);
    Json::Value sample;
    sample["id"] = "dummy";
    sample["timestamp"] = static_cast<Json::UInt64>(std::time(nullptr));
    sample["status"] = "snapshot";
    telegrams.append(sample);

    auto resp = drogon::HttpResponse::newHttpJsonResponse(telegrams);
    callback(resp);
}

void TrdpController::loadConfig(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
    const auto json = req->getJsonObject();
    if (!json || !(*json).isMember("path") || !(*json).isMember("host_name")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody(R"({"error":"Missing required fields: path, host_name"})");
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    if (engine_ == nullptr) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(R"({"error":"TRDP engine is not initialized"})");
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    const auto &path = (*json)["path"].asString();
    const auto &hostName = (*json)["host_name"].asString();
    engine_->loadConfig(path, hostName);

    Json::Value response;
    response["status"] = "config loaded";
    response["path"] = path;
    response["host_name"] = hostName;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}
