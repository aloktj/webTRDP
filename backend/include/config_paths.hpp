#pragma once

#include <cstdint>
#include <string>

// Helper utilities for resolving configuration paths and runtime settings.
// Values can be overridden via environment variables and fall back to compile
// time defaults defined in backend/CMakeLists.txt.

std::string getEnvOrEmpty(const char *name);

std::string resolveXmlPath();
std::string resolveHostName();
std::string resolveListenAddress();
uint16_t resolveListenPort();
bool shouldRunAsDaemon();
bool isAddressAvailable(const std::string &address, uint16_t port);

// Returns the directory the backend should scan for TRDP XML configuration
// files. The value is resolved from TRDP_CONFIG_DIR, then the
// TRDP_DEFAULT_CONFIG_DIR compile time definition. If neither is available, it
// falls back to the parent of the default XML path when present.
std::string resolveConfigDirectory();
