#include "json_utils.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace trdp {
namespace {

std::string directionToString(Direction direction) {
    switch (direction) {
    case Direction::Source:
        return "source";
    case Direction::Sink:
        return "sink";
    case Direction::SourceSink:
        return "source_sink";
    }
    return "unknown";
}

Json::Int64 toMillis(const std::chrono::steady_clock::time_point &tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

std::string payloadToHex(const std::vector<uint8_t> &payload) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const auto byte : payload) {
        stream << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return stream.str();
}

}  // namespace

Json::Value pdRuntimeToJson(const PdRuntime &pd) {
    Json::Value json(Json::objectValue);

    if (pd.def != nullptr) {
        json["interface"] = pd.def->interface_name;
        json["com_id"] = static_cast<Json::UInt64>(pd.def->com_id);
        json["name"] = pd.def->name;
        json["dataset_id"] = static_cast<Json::UInt64>(pd.def->dataset_id);
        json["direction"] = directionToString(pd.def->direction);
        json["cycle_us"] = static_cast<Json::UInt64>(pd.def->cycle_us);
    } else {
        json["interface"] = Json::nullValue;
        json["com_id"] = Json::nullValue;
        json["name"] = Json::nullValue;
        json["dataset_id"] = Json::nullValue;
        json["direction"] = Json::nullValue;
        json["cycle_us"] = Json::nullValue;
    }

    Json::Value stats(Json::objectValue);
    stats["rx_count"] = static_cast<Json::UInt64>(pd.rx_count);
    stats["tx_count"] = static_cast<Json::UInt64>(pd.tx_count);
    stats["avg_period_us"] = pd.avg_period_us;
    stats["last_period_us"] = pd.last_period_us;
    stats["timeout_count"] = static_cast<Json::UInt64>(pd.timeout_count);
    json["stats"] = stats;

    Json::Value last_rx(Json::objectValue);
    last_rx["timestamp"] = pd.last_rx_valid ? toMillis(pd.last_rx_time) : Json::Int64(0);
    last_rx["valid"] = pd.last_rx_valid;
    last_rx["raw_hex"] = payloadToHex(pd.last_rx_payload);
    json["last_rx"] = last_rx;

    return json;
}

}  // namespace trdp

