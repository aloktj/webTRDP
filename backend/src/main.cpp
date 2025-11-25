#include <drogon/drogon.h>
#include <filesystem>
#include <memory>

#include "trdp_engine.hpp"

#include "controllers/TrdpController.h"
#include "config_paths.hpp"

std::unique_ptr<trdp::TrdpEngine> g_trdpEngine;

namespace {

bool ensureConfigAvailable(const std::string &xmlPath) {
    if (xmlPath.empty()) {
        LOG_FATAL << "No TRDP XML configuration path provided. Set TRDP_XML_PATH to a valid file.";
        return false;
    }

    if (!std::filesystem::exists(xmlPath)) {
        LOG_FATAL << "TRDP XML configuration not found: " << xmlPath
                  << ". Set TRDP_XML_PATH to a valid file (e.g. third_party/TCNopen/trdp/test/xml/example.xml).";
        return false;
    }

    return true;
}

}  // namespace

int main() {
    const std::string xmlPath = resolveXmlPath();
    const std::string hostName = resolveHostName();
    const std::string listenAddress = resolveListenAddress();
    const uint16_t listenPort = resolveListenPort();

    if (!ensureConfigAvailable(xmlPath)) {
        return 1;
    }

    if (!isAddressAvailable(listenAddress, listenPort)) {
        LOG_FATAL << "Bind address unavailable: " << listenAddress << ':' << listenPort
                  << ". Adjust TRDP_LISTEN_ADDRESS or TRDP_LISTEN_PORT.";
        return 1;
    }

    try {
        g_trdpEngine = std::make_unique<trdp::TrdpEngine>();
        g_trdpEngine->loadConfig(xmlPath, hostName);
        g_trdpEngine->start();
    } catch (const std::exception &ex) {
        LOG_FATAL << "Failed to initialize TRDP engine: " << ex.what();
        return 1;
    }

    const std::filesystem::path logDir = std::filesystem::current_path() / "logs";
    std::error_code logErr;
    std::filesystem::create_directories(logDir, logErr);
    if (logErr) {
        LOG_FATAL << "Failed to create log directory at " << logDir << ": " << logErr.message();
        return 1;
    }

    const bool runAsDaemon = shouldRunAsDaemon();

    auto &app = drogon::app();
    app.addListener(listenAddress, listenPort)
        .setThreadNum(1)
        .setLogPath(logDir.string());

    if (runAsDaemon) {
        app.enableRunAsDaemon();
    }

    TrdpController::setEngine(g_trdpEngine.get());

    LOG_INFO << "Starting TRDP backend" << (runAsDaemon ? " as daemon" : " in foreground");

    app.run();
    g_trdpEngine->stop();
    return 0;
}

