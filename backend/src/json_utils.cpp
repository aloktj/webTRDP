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

std::string typeToString(uint32_t type) {
    switch (type) {
        case TRDP_BOOL8:
            return "BOOL";
        case TRDP_UINT8:
            return "UINT8";
        case TRDP_INT8:
            return "INT8";
        case TRDP_UINT16:
            return "UINT16";
        case TRDP_INT16:
            return "INT16";
        case TRDP_UINT32:
            return "UINT32";
        case TRDP_INT32:
            return "INT32";
        default:
            return "UNKNOWN";
    }
}

Json::Value valueToJson(uint32_t type, int64_t value) {
    if (type == TRDP_BOOL8) {
        return value != 0;
    }

    return Json::Int64(value);
}

}  // namespace

Json::Value pdRuntimeToJson(const PdRuntime &pd, const TrdpEngine &engine) {
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

    Json::Value decoded_fields(Json::arrayValue);
    for (const auto &field : engine.decodeLastRx(pd)) {
        Json::Value field_json(Json::objectValue);
        field_json["name"] = field.name;
        field_json["type"] = typeToString(field.type);

        if (field.values.empty()) {
            field_json["value"] = Json::nullValue;
        } else if (field.values.size() == 1u) {
            field_json["value"] = valueToJson(field.type, field.values.front());
        } else {
            Json::Value arr(Json::arrayValue);
            for (const auto value : field.values) {
                arr.append(valueToJson(field.type, value));
            }
            field_json["value"] = arr;
        }

        decoded_fields.append(field_json);
    }

    last_rx["decoded_fields"] = decoded_fields;
    json["last_rx"] = last_rx;

    return json;
}

}  // namespace trdp

