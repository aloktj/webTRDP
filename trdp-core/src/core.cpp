#include "trdp/core.hpp"

#include <iostream>
#include <utility>

namespace trdp {

TrdpEngine::TrdpEngine() = default;
TrdpEngine::~TrdpEngine() = default;

void TrdpEngine::loadConfig(const std::string &xml_path, const std::string &host_name) {
    config_path_ = xml_path;
    host_name_ = host_name;
}

void TrdpEngine::start() {
    running_ = true;
    std::cout << "Starting TRDP engine for host " << host_name_ << " using config " << config_path_ << "..." << std::endl;
}

void TrdpEngine::stop() {
    if (running_) {
        std::cout << "Stopping TRDP engine..." << std::endl;
        running_ = false;
    }
}

std::string TrdpEngine::status() const {
    return running_ ? "running" : "stopped";
}

}  // namespace trdp

