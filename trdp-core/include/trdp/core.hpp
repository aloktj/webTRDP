#pragma once

#include <string>

namespace trdp {

class TrdpEngine {
public:
    TrdpEngine();
    ~TrdpEngine();

    void loadConfig(const std::string &xml_path, const std::string &host_name);
    void start();
    void stop();

    [[nodiscard]] std::string status() const;

private:
    bool running_{false};
    std::string config_path_;
    std::string host_name_;
};

}  // namespace trdp

