#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace trdp {

struct DatasetElement {
    std::string name;
    uint32_t type;
    uint32_t array_size;
};

struct Dataset {
    uint32_t id;
    std::string name;
    std::vector<DatasetElement> elements;
};

enum class Direction {
    Source,
    Sink,
    SourceSink
};

struct InterfaceDef {
    std::string name;
    uint32_t network_id;
    std::string host_ip;
};

struct PdTelegramDef {
    std::string name;
    uint32_t com_id;
    uint32_t dataset_id;
    Direction direction;
    uint32_t cycle_us;
    bool marshall;
    std::string interface_name;
};

}  // namespace trdp

