#include "config_paths.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

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

std::string defaultConfigDirectory() {
#ifdef TRDP_DEFAULT_CONFIG_DIR
    return TRDP_DEFAULT_CONFIG_DIR;
#else
    return {};
#endif
}

}  // namespace

std::string getEnvOrEmpty(const char *name) {
    const char *value = std::getenv(name);
    return value != nullptr ? value : std::string {};
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
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lowered == "1" || lowered == "true" || lowered == "yes";
}

std::string resolveConfigDirectory() {
    const std::string envOverride = getEnvOrEmpty("TRDP_CONFIG_DIR");
    if (!envOverride.empty()) {
        return envOverride;
    }

    const std::string compiledDefault = defaultConfigDirectory();
    if (!compiledDefault.empty()) {
        return compiledDefault;
    }

    const std::string xmlPath = defaultXmlPath();
    if (!xmlPath.empty()) {
        const std::filesystem::path parent = std::filesystem::path(xmlPath).parent_path();
        if (!parent.empty()) {
            return parent.string();
        }
    }

    return {};
}
