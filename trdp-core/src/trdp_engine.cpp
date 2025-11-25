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
    const bool shouldRestart = running_;
    if (shouldRestart || !interfaces_.empty()) {
        stop();
    }

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

    if (shouldRestart) {
        start();
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
    std::lock_guard<std::mutex> lock(state_mtx_);

    if (PdRuntime *runtime = findPdRuntime(com_id, {})) {
        runtime->tx_enabled = enable;
    }
}

void TrdpEngine::setPdValues(uint32_t com_id, const std::map<std::string, double> &values) {
    std::lock_guard<std::mutex> lock(state_mtx_);

    PdRuntime *runtime = findPdRuntime(com_id, {});
    if (runtime == nullptr || runtime->def == nullptr) {
        return;
    }

    const Dataset *dataset = findDataset(runtime->def->dataset_id);
    if (dataset == nullptr) {
        return;
    }

    std::vector<uint8_t> payload;
    for (const auto &element : dataset->elements) {
        const auto valueIt = values.find(element.name);
        const double value = valueIt != values.end() ? valueIt->second : 0.0;
        const auto count = element.array_size == 0u ? 1u : element.array_size;

        for (uint32_t idx = 0u; idx < count; ++idx) {
            switch (element.type) {
                case TRDP_BOOL8: {
                    payload.push_back(value != 0.0 ? 1u : 0u);
                    break;
                }
                case TRDP_UINT8: {
                    payload.push_back(static_cast<uint8_t>(value));
                    break;
                }
                case TRDP_INT8: {
                    payload.push_back(static_cast<uint8_t>(static_cast<int8_t>(value)));
                    break;
                }
                case TRDP_UINT16: {
                    const uint16_t v = static_cast<uint16_t>(value);
                    payload.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>(v & 0xFFu));
                    break;
                }
                case TRDP_INT16: {
                    const uint16_t v = static_cast<uint16_t>(static_cast<int16_t>(value));
                    payload.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>(v & 0xFFu));
                    break;
                }
                case TRDP_UINT32: {
                    const uint32_t v = static_cast<uint32_t>(value);
                    payload.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>(v & 0xFFu));
                    break;
                }
                case TRDP_INT32: {
                    const uint32_t v = static_cast<uint32_t>(static_cast<int32_t>(value));
                    payload.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
                    payload.push_back(static_cast<uint8_t>(v & 0xFFu));
                    break;
                }
                default: {
                    // TODO: Handle additional data types
                    break;
                }
            }
        }
    }

    runtime->tx_payload = std::move(payload);
}

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

std::vector<DecodedField> TrdpEngine::decodeLastRx(const PdRuntime &pd) const {
    std::vector<DecodedField> decoded;

    if (pd.def == nullptr) {
        return decoded;
    }

    const Dataset *dataset = findDataset(pd.def->dataset_id);
    if (dataset == nullptr) {
        return decoded;
    }

    const auto &payload = pd.last_rx_payload;
    size_t offset = 0u;

    auto hasBytes = [&](size_t count) { return offset + count <= payload.size(); };

    for (const auto &element : dataset->elements) {
        const uint32_t count = element.array_size == 0u ? 1u : element.array_size;
        DecodedField field {element.name, element.type, {}};
        field.values.reserve(count);

        for (uint32_t idx = 0u; idx < count; ++idx) {
            switch (element.type) {
                case TRDP_BOOL8:
                case TRDP_UINT8: {
                    if (!hasBytes(1u)) {
                        return decoded;
                    }
                    field.values.push_back(static_cast<int64_t>(payload[offset]));
                    offset += 1u;
                    break;
                }
                case TRDP_INT8: {
                    if (!hasBytes(1u)) {
                        return decoded;
                    }
                    field.values.push_back(static_cast<int64_t>(static_cast<int8_t>(payload[offset])));
                    offset += 1u;
                    break;
                }
                case TRDP_UINT16: {
                    if (!hasBytes(2u)) {
                        return decoded;
                    }
                    const uint16_t value = static_cast<uint16_t>((static_cast<uint16_t>(payload[offset]) << 8u) |
                                                                  static_cast<uint16_t>(payload[offset + 1u]));
                    field.values.push_back(static_cast<int64_t>(value));
                    offset += 2u;
                    break;
                }
                case TRDP_INT16: {
                    if (!hasBytes(2u)) {
                        return decoded;
                    }
                    const int16_t value = static_cast<int16_t>((static_cast<uint16_t>(payload[offset]) << 8u) |
                                                               static_cast<uint16_t>(payload[offset + 1u]));
                    field.values.push_back(static_cast<int64_t>(value));
                    offset += 2u;
                    break;
                }
                case TRDP_UINT32: {
                    if (!hasBytes(4u)) {
                        return decoded;
                    }
                    const uint32_t value = (static_cast<uint32_t>(payload[offset]) << 24u) |
                                           (static_cast<uint32_t>(payload[offset + 1u]) << 16u) |
                                           (static_cast<uint32_t>(payload[offset + 2u]) << 8u) |
                                           static_cast<uint32_t>(payload[offset + 3u]);
                    field.values.push_back(static_cast<int64_t>(value));
                    offset += 4u;
                    break;
                }
                case TRDP_INT32: {
                    if (!hasBytes(4u)) {
                        return decoded;
                    }
                    const int32_t value = (static_cast<uint32_t>(payload[offset]) << 24u) |
                                          (static_cast<uint32_t>(payload[offset + 1u]) << 16u) |
                                          (static_cast<uint32_t>(payload[offset + 2u]) << 8u) |
                                          static_cast<uint32_t>(payload[offset + 3u]);
                    field.values.push_back(static_cast<int64_t>(value));
                    offset += 4u;
                    break;
                }
                default: {
                    return decoded;
                }
            }
        }

        decoded.push_back(std::move(field));
    }

    return decoded;
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

const Dataset *TrdpEngine::findDataset(uint32_t id) const {
    for (const auto &dataset : datasets_) {
        if (dataset.id == id) {
            return &dataset;
        }
    }

    return nullptr;
}

}  // namespace trdp

