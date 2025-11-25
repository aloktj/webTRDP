#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "trdp_config.hpp"

#include <trdp_if_light.h>

namespace trdp {

struct PdRuntime {
    const PdTelegramDef *def;
    std::vector<uint8_t> tx_payload;
    bool tx_enabled;
    std::chrono::steady_clock::time_point next_tx_due;
    std::vector<uint8_t> last_rx_payload;
    std::chrono::steady_clock::time_point last_rx_time;
    bool last_rx_valid;
    uint64_t rx_count;
    uint64_t tx_count;
    uint64_t timeout_count;
    double last_period_us;
    double avg_period_us;
};

struct InterfaceRuntime {
    InterfaceDef def;
    TRDP_APP_SESSION_T appHandle;
    std::vector<PdRuntime *> pd_list;
};

struct DecodedField {
    std::string name;
    uint32_t type;
    std::vector<int64_t> values;
};

class TrdpEngine {
public:
    void loadConfig(const std::string &xml_path, const std::string &host_name);
    void start();
    void stop();
    std::vector<PdRuntime> getPdSnapshot() const;
    void enablePd(uint32_t com_id, bool enable);
    void setPdValues(uint32_t com_id, const std::map<std::string, double> &values);
    std::vector<DecodedField> decodeLastRx(const PdRuntime &pd) const;
    void onPdReceive(TRDP_APP_SESSION_T, const TRDP_PD_INFO_T *, const uint8_t *, uint32_t);

private:
    std::vector<InterfaceRuntime> interfaces_;
    std::vector<PdTelegramDef> pd_defs_;
    std::vector<PdRuntime> pd_runtimes_;
    std::vector<Dataset> datasets_;
    std::atomic<bool> running_;
    std::thread pd_thread_;
    mutable std::mutex state_mtx_;

    void pdSchedulerLoop();
    InterfaceRuntime *findInterface(const std::string &name);
    PdRuntime *findPdRuntime(uint32_t com_id, const std::string &if_name);
    const Dataset *findDataset(uint32_t id) const;
    void sendPdOnInterface(InterfaceRuntime &iface, PdRuntime &pd_runtime);
};

}  // namespace trdp

