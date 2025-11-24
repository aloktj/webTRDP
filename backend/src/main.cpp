#include <drogon/drogon.h>
#include <trdp/core.hpp>

int main() {
    trdp::TrdpEngine engine;
    engine.loadConfig("configs/example.xml", "localhost");

    drogon::app()
        .addListener("0.0.0.0", 8080)
        .setThreadNum(1)
        .setLogPath("./logs")
        .enableRunAsDaemon(false);

    LOG_INFO << "Starting TRDP backend (status: " << engine.status() << ")";
    engine.start();

    drogon::app().run();
    engine.stop();
    return 0;
}

