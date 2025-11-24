#include <drogon/drogon.h>
#include <memory>

#include "trdp_engine.hpp"

#include "controllers/TrdpController.h"

std::unique_ptr<trdp::TrdpEngine> g_trdpEngine;

int main() {
    g_trdpEngine = std::make_unique<trdp::TrdpEngine>();
    g_trdpEngine->loadConfig("configs/example.xml", "localhost");
    g_trdpEngine->start();

    drogon::app()
        .addListener("0.0.0.0", 8080)
        .setThreadNum(1)
        .setLogPath("./logs")
        .enableRunAsDaemon(false);

    TrdpController::setEngine(g_trdpEngine.get());

    LOG_INFO << "Starting TRDP backend";

    drogon::app().run();
    g_trdpEngine->stop();
    return 0;
}

