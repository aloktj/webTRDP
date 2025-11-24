#include "trdp/trdp_config_loader.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

std::string directionToString(trdp::Direction direction) {
    switch (direction) {
    case trdp::Direction::Source:
        return "Source";
    case trdp::Direction::Sink:
        return "Sink";
    case trdp::Direction::SourceSink:
        return "SourceSink";
    }
    return "Unknown";
}

void printUsage(const char *program) {
    std::cerr << "Usage: " << program << " --xml <config.xml> --host <host name>" << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
    std::string xmlPath;
    std::string hostName;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--xml" && i + 1 < argc) {
            xmlPath = argv[++i];
        } else if (arg == "--host" && i + 1 < argc) {
            hostName = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    if (xmlPath.empty() || hostName.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    try {
        trdp::TrdpConfigLoader loader;
        loader.loadFromXml(xmlPath, hostName);

        std::cout << "Interfaces:" << std::endl;
        for (const auto &iface : loader.interfaces()) {
            std::cout << "  - " << iface.name << " (networkId=" << iface.network_id << ", hostIp=" << iface.host_ip
                      << ")" << std::endl;
        }

        std::cout << "\nPD Telegrams:" << std::endl;
        for (const auto &telegram : loader.pdTelegrams()) {
            std::cout << "  - " << telegram.name << " (comId=" << telegram.com_id << ", datasetId=" << telegram.dataset_id
                      << ", direction=" << directionToString(telegram.direction) << ", cycle=" << telegram.cycle_us
                      << " us)" << std::endl;
        }

        std::cout << "\nDatasets:" << std::endl;
        for (const auto &dataset : loader.datasets()) {
            std::cout << "  - id=" << dataset.id << ", name=" << dataset.name
                      << ", elements=" << dataset.elements.size() << std::endl;
        }
    } catch (const std::exception &ex) {
        std::cerr << "Failed to load configuration: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
