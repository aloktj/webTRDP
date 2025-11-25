#include "controllers/TrdpController.h"

#include <chrono>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <map>
#include <string>

trdp::TrdpEngine *TrdpController::engine_ = nullptr;

void TrdpController::setEngine(trdp::TrdpEngine *engine) {
    engine_ = engine;
}

namespace {

std::string directionToString(trdp::Direction direction) {
    switch (direction) {
    case trdp::Direction::Source:
        return "source";
    case trdp::Direction::Sink:
        return "sink";
    case trdp::Direction::SourceSink:
        return "source_sink";
    }
    return "unknown";
}

Json::Int64 toMicros(const std::chrono::steady_clock::time_point &tp) {
    return std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count();
}

}  // namespace

void TrdpController::getPdTelegrams(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
    if (engine_ == nullptr) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(R"({"error":"TRDP engine is not initialized"})");
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    Json::Value telegrams(Json::arrayValue);
    const auto snapshot = engine_->getPdSnapshot();
    for (const auto &pd : snapshot) {
        Json::Value entry(Json::objectValue);

        if (pd.def != nullptr) {
            entry["name"] = pd.def->name;
            entry["com_id"] = pd.def->com_id;
            entry["dataset_id"] = pd.def->dataset_id;
            entry["direction"] = directionToString(pd.def->direction);
            entry["cycle_us"] = pd.def->cycle_us;
            entry["interface"] = pd.def->interface_name;
        }

        entry["tx_enabled"] = pd.tx_enabled;
        entry["next_tx_due_us"] = toMicros(pd.next_tx_due);
        entry["tx_payload_size"] = static_cast<Json::UInt64>(pd.tx_payload.size());
        entry["last_rx_payload_size"] = static_cast<Json::UInt64>(pd.last_rx_payload.size());
        entry["last_rx_time_us"] = toMicros(pd.last_rx_time);
        entry["last_rx_valid"] = pd.last_rx_valid;
        entry["rx_count"] = static_cast<Json::UInt64>(pd.rx_count);
        entry["tx_count"] = static_cast<Json::UInt64>(pd.tx_count);
        entry["timeout_count"] = static_cast<Json::UInt64>(pd.timeout_count);
        entry["last_period_us"] = pd.last_period_us;
        entry["avg_period_us"] = pd.avg_period_us;

        telegrams.append(entry);
    }

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

void TrdpController::enablePd(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    uint32_t com_id) const {
    const auto json = req->getJsonObject();
    if (!json || !(*json).isMember("enable")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody(R"({"error":"Missing required field: enable"})");
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

    const bool enable = (*json)["enable"].asBool();
    engine_->enablePd(com_id, enable);

    Json::Value response;
    response["status"] = "pd enable updated";
    response["com_id"] = com_id;
    response["enabled"] = enable;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void TrdpController::setPdValues(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    uint32_t com_id) const {
    const auto json = req->getJsonObject();
    if (!json || !(*json).isMember("fields") || !(*json)["fields"].isArray()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("{\"error\":\"Missing required field: fields (array)\"}");
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    if (engine_ == nullptr) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("{\"error\":\"TRDP engine is not initialized\"}");
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    std::map<std::string, double> values;
    for (const auto &field : (*json)["fields"]) {
        if (!field.isMember("name") || !field.isMember("value") || !field["value"].isNumeric()) {
            continue;
        }

        values[field["name"].asString()] = field["value"].asDouble();
    }

    engine_->setPdValues(com_id, values);

    Json::Value response;
    response["status"] = "pd values updated";
    response["com_id"] = com_id;
    response["updated_fields"] = static_cast<Json::UInt64>(values.size());

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}
