#pragma once

#include "trdp_config.hpp"

#include <string>
#include <vector>

namespace trdp {

class TrdpConfigLoader {
public:
    void loadFromXml(const std::string &xml_path, const std::string &host_name);

    const std::vector<InterfaceDef> &interfaces() const;
    const std::vector<PdTelegramDef> &pdTelegrams() const;
    const std::vector<Dataset> &datasets() const;

private:
    std::vector<InterfaceDef> interfaces_;
    std::vector<PdTelegramDef> pdTelegrams_;
    std::vector<Dataset> datasets_;
};

}  // namespace trdp

