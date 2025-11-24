#include "trdp_engine.hpp"

#include "trdp/trdp_config_loader.hpp"

#include <cstring>
#include <stdexcept>

#include <vos_sock.h>

namespace {

void pdCallback(void *pRefCon, TRDP_APP_SESSION_T appHandle, const TRDP_PD_INFO_T *pMsg, UINT8 *pData, UINT32 dataSize) {
    if (pRefCon != nullptr) {
        static_cast<trdp::TrdpEngine *>(pRefCon)->onPdReceive(pMsg, pData, dataSize);
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
}

void TrdpEngine::stop() {
    running_ = false;
}

std::vector<PdRuntime> TrdpEngine::getPdSnapshot() const { return pd_runtimes_; }

void TrdpEngine::enablePd(uint32_t com_id, bool enable) {
    if (PdRuntime *runtime = findPdRuntime(com_id, {})) {
        runtime->tx_enabled = enable;
    }
}

void TrdpEngine::setPdValues(uint32_t, const std::map<std::string, double> &) {}

void TrdpEngine::pdSchedulerLoop() {}

void TrdpEngine::onPdReceive(const TRDP_PD_INFO_T *, const uint8_t *, uint32_t) {}

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

