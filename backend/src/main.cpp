#include <drogon/drogon.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "trdp_engine.hpp"

#include "controllers/TrdpController.h"

std::unique_ptr<trdp::TrdpEngine> g_trdpEngine;

namespace {

std::string getEnvOrEmpty(const char *name) {
    const char *value = std::getenv(name);
    return value != nullptr ? value : std::string {};
}

std::string defaultXmlPath() {
#ifdef TRDP_DEFAULT_XML_PATH
    return TRDP_DEFAULT_XML_PATH;
#else
    return {};
#endif
}

std::string defaultHostName() {
#ifdef TRDP_DEFAULT_HOST_NAME
    return TRDP_DEFAULT_HOST_NAME;
#else
    return "localhost";
#endif
}

std::string resolveXmlPath() {
    const std::string envOverride = getEnvOrEmpty("TRDP_XML_PATH");
    if (!envOverride.empty()) {
        return envOverride;
    }

    return defaultXmlPath();
}

std::string resolveHostName() {
    const std::string envOverride = getEnvOrEmpty("TRDP_HOST_NAME");
    if (!envOverride.empty()) {
        return envOverride;
    }

    return defaultHostName();
}

std::string resolveListenAddress() {
    const std::string envOverride = getEnvOrEmpty("TRDP_LISTEN_ADDRESS");
    return envOverride.empty() ? std::string {"0.0.0.0"} : envOverride;
}

uint16_t resolveListenPort() {
    const std::string envOverride = getEnvOrEmpty("TRDP_LISTEN_PORT");
    if (!envOverride.empty()) {
        try {
            const int value = std::stoi(envOverride);
            if (value > 0 && value <= 65535) {
                return static_cast<uint16_t>(value);
            }
        } catch (...) {
        }
    }

    return 8080u;
}

bool isAddressAvailable(const std::string &address, uint16_t port) {
    const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
        ::close(sock);
        return false;
    }

    const bool available = ::bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0;
    ::close(sock);
    return available;
}

bool shouldRunAsDaemon() {
    const std::string envOverride = getEnvOrEmpty("TRDP_RUN_AS_DAEMON");
    if (envOverride.empty()) {
        return false;
    }

    std::string lowered = envOverride;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lowered == "1" || lowered == "true" || lowered == "yes";
}

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

