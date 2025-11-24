#include "trdp_engine.hpp"

#include "trdp/trdp_config_loader.hpp"

#include <cstring>
#include <stdexcept>

#include <vos_sock.h>

namespace {

void pdCallback(void *pUserRef, TRDP_APP_SESSION_T appHandle, const TRDP_PD_INFO_T *pMsg, UINT8 *pData, UINT32 dataSize) {
    if (pUserRef != nullptr) {
        static_cast<trdp::TrdpEngine *>(pUserRef)->onPdReceive(appHandle, pMsg, pData, dataSize);
    }
}

void mdCallback(void *, TRDP_APP_SESSION_T, const TRDP_MD_INFO_T *, UINT8 *, UINT32) {
    // MD handling not implemented yet
}

}  // namespace

namespace trdp {

void TrdpEngine::loadConfig(const std::string &xml_path, const std::string &host_name) {
    TrdpConfigLoader loader;
    loader.loadFromXml(xml_path, host_name);

    datasets_ = loader.datasets();
    pd_defs_ = loader.pdTelegrams();

    interfaces_.clear();
    pd_runtimes_.clear();

    TRDP_ERR_T err = tlc_init(nullptr, this, nullptr);
    if (err != TRDP_NO_ERR) {
        throw std::runtime_error("tlc_init failed");
    }

    for (const auto &ifaceDef : loader.interfaces()) {
        InterfaceRuntime runtime {};
        runtime.def = ifaceDef;

        TRDP_PD_CONFIG_T pdConfig {};
        pdConfig.pfCbFunction = pdCallback;
        pdConfig.pRefCon = this;

        TRDP_MD_CONFIG_T mdConfig {};
        mdConfig.pfCbFunction = mdCallback;
        mdConfig.pRefCon = this;

        TRDP_PROCESS_CONFIG_T processConfig {};
        std::strncpy(processConfig.hostName, host_name.c_str(), sizeof(processConfig.hostName) - 1u);
        processConfig.cycleTime = 100000u;
        processConfig.options = TRDP_OPTION_BLOCK;

        err = tlc_openSession(&runtime.appHandle,
                              vos_dottedIP(ifaceDef.host_ip.c_str()),
                              0u,
                              nullptr,
                              &pdConfig,
                              &mdConfig,
                              &processConfig);
        if (err != TRDP_NO_ERR) {
            throw std::runtime_error("Failed to initialize TRDP session");
        }

        interfaces_.push_back(runtime);
    }

    pd_runtimes_.reserve(pd_defs_.size());
    for (auto &pdDef : pd_defs_) {
        pd_runtimes_.push_back(PdRuntime {});
        PdRuntime &runtime = pd_runtimes_.back();
        runtime.def = &pdDef;
        runtime.tx_enabled = pdDef.direction != Direction::Sink;
        runtime.next_tx_due = std::chrono::steady_clock::now();
        runtime.last_rx_valid = false;
        runtime.rx_count = 0u;
        runtime.tx_count = 0u;
        runtime.timeout_count = 0u;
        runtime.last_period_us = 0.0;
        runtime.avg_period_us = 0.0;

        if (pdDef.direction != Direction::Source) {
            InterfaceRuntime *iface = findInterface(pdDef.interface_name);
            if (iface == nullptr) {
                throw std::runtime_error("Unknown interface for PD telegram");
            }

            TRDP_SUB_T subHandle {};
            TRDP_COM_PARAM_T comParams {};
            err = tlp_subscribe(iface->appHandle,
                                &subHandle,
                                this,
                                pdCallback,
                                0u,
                                pdDef.com_id,
                                0u,
                                0u,
                                0u,
                                0u,
                                0u,
                                TRDP_FLAGS_CALLBACK,
                                &comParams,
                                pdDef.cycle_us > 0u ? pdDef.cycle_us * 2u : 0u,
                                TRDP_TO_DEFAULT);
            if (err != TRDP_NO_ERR) {
                throw std::runtime_error("Failed to subscribe PD telegram");
            }

            iface->pd_list.push_back(&runtime);
        }
    }
}

void TrdpEngine::start() {
    running_ = true;
    pd_thread_ = std::thread(&TrdpEngine::pdSchedulerLoop, this);
}

void TrdpEngine::stop() {
    running_ = false;

    if (pd_thread_.joinable()) {
        pd_thread_.join();
    }

    for (auto &iface : interfaces_) {
        tlc_closeSession(iface.appHandle);
    }

    tlc_terminate();
}

std::vector<PdRuntime> TrdpEngine::getPdSnapshot() const { return pd_runtimes_; }

void TrdpEngine::enablePd(uint32_t com_id, bool enable) {
    if (PdRuntime *runtime = findPdRuntime(com_id, {})) {
        runtime->tx_enabled = enable;
    }
}

void TrdpEngine::setPdValues(uint32_t, const std::map<std::string, double> &) {}

void TrdpEngine::pdSchedulerLoop() {
    while (running_) {
        const auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(state_mtx_);
        for (auto &runtime : pd_runtimes_) {
            if (!runtime.tx_enabled || runtime.def == nullptr || runtime.def->direction == Direction::Sink) {
                continue;
            }

            if (now >= runtime.next_tx_due) {
                if (InterfaceRuntime *iface = findInterface(runtime.def->interface_name)) {
                    sendPdOnInterface(*iface, runtime);
                    runtime.tx_count++;
                }

                runtime.next_tx_due = now + std::chrono::microseconds(runtime.def->cycle_us);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1u));
    }
}

void TrdpEngine::onPdReceive(TRDP_APP_SESSION_T appHandle, const TRDP_PD_INFO_T *pMsg, const uint8_t *pData, uint32_t dataSize) {
    if (pMsg == nullptr) {
        return;
    }

    InterfaceRuntime *iface = nullptr;
    for (auto &candidate : interfaces_) {
        if (candidate.appHandle == appHandle) {
            iface = &candidate;
            break;
        }
    }

    if (iface == nullptr) {
        return;
    }

    PdRuntime *runtime = findPdRuntime(pMsg->comId, iface->def.name);
    if (runtime == nullptr) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(state_mtx_);

    runtime->last_rx_payload.assign(pData, pData + dataSize);

    if (runtime->last_rx_valid) {
        runtime->last_period_us = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(now - runtime->last_rx_time).count();
        const auto new_count = runtime->rx_count + 1u;
        runtime->avg_period_us += (runtime->last_period_us - runtime->avg_period_us) / static_cast<double>(new_count);
    } else {
        runtime->last_period_us = 0.0;
        runtime->avg_period_us = runtime->last_period_us;
    }

    runtime->last_rx_time = now;
    runtime->last_rx_valid = true;
    runtime->rx_count++;
}

void TrdpEngine::sendPdOnInterface(InterfaceRuntime &iface, PdRuntime &pd_runtime) {
    // TODO: Integrate TRDP PD send API
    (void)iface;
    (void)pd_runtime;
}

InterfaceRuntime *TrdpEngine::findInterface(const std::string &name) {
    for (auto &iface : interfaces_) {
        if (iface.def.name == name) {
            return &iface;
        }
    }
    return nullptr;
}

PdRuntime *TrdpEngine::findPdRuntime(uint32_t com_id, const std::string &if_name) {
    for (auto &pd : pd_runtimes_) {
        if (pd.def != nullptr && pd.def->com_id == com_id && (if_name.empty() || pd.def->interface_name == if_name)) {
            return &pd;
        }
    }
    return nullptr;
}

}  // namespace trdp

