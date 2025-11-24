#include "trdp/trdp_config_loader.hpp"

#include "trdp_config.hpp"

#include <fstream>
#include <stdexcept>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <cstdint>

#include <tau_xml.h>
#include <trdp_types.h>
#include <vos_mem.h>
#include <vos_sock.h>

namespace trdp {
namespace {

Direction mapDirection(TRDP_EXCHG_OPTION_T type) {
    switch (type) {
    case TRDP_EXCHG_SOURCE:
        return Direction::Source;
    case TRDP_EXCHG_SINK:
        return Direction::Sink;
    case TRDP_EXCHG_SOURCESINK:
        return Direction::SourceSink;
    case TRDP_EXCHG_UNSET:
    default:
        return Direction::SourceSink;
    }
}

std::unordered_map<uint32_t, std::string> parseTelegramNames(const std::string &xml_path) {
    std::unordered_map<uint32_t, std::string> comIdToName;
    std::ifstream input(xml_path);
    if (!input.is_open()) {
        return comIdToName;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string content = buffer.str();

    const std::regex telegramTag("<\\s*telegram[^>]*>", std::regex::icase);
    const std::regex nameAttr("name\\s*=\\s*\"([^\"]*)\"", std::regex::icase);
    const std::regex comIdAttr("com-id\\s*=\\s*\"([0-9]+)\"", std::regex::icase);

    auto begin = std::sregex_iterator(content.begin(), content.end(), telegramTag);
    const auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        const std::string tag = it->str();
        std::smatch nameMatch;
        std::smatch comIdMatch;

        if (std::regex_search(tag, nameMatch, nameAttr) && std::regex_search(tag, comIdMatch, comIdAttr)) {
            try {
                const uint32_t comId = static_cast<uint32_t>(std::stoul(comIdMatch[1].str()));
                comIdToName.emplace(comId, nameMatch[1].str());
            } catch (...) {
                continue;
            }
        }
    }

    return comIdToName;
}

Direction determineDirection(const TRDP_EXCHG_PAR_T &exchange, const std::string &host_name) {
    bool isSource = false;
    bool isDestination = false;

    for (uint32_t idx = 0u; idx < exchange.srcCnt; ++idx) {
        if (exchange.pSrc[idx].pUriHost1 != nullptr && host_name == exchange.pSrc[idx].pUriHost1) {
            isSource = true;
        }
        if (exchange.pSrc[idx].pUriHost2 != nullptr && host_name == exchange.pSrc[idx].pUriHost2) {
            isSource = true;
        }
    }

    for (uint32_t idx = 0u; idx < exchange.destCnt; ++idx) {
        if (exchange.pDest[idx].pUriHost != nullptr && host_name == exchange.pDest[idx].pUriHost) {
            isDestination = true;
        }
    }

    if (isSource && isDestination) {
        return Direction::SourceSink;
    }
    if (isSource) {
        return Direction::Source;
    }
    if (isDestination) {
        return Direction::Sink;
    }

    return mapDirection(exchange.type);
}

}  // namespace

void TrdpConfigLoader::loadFromXml(const std::string &xml_path, const std::string &host_name) {
    interfaces_.clear();
    pdTelegrams_.clear();
    datasets_.clear();

    TRDP_XML_DOC_HANDLE_T docHandle {};
    TRDP_ERR_T result = tau_prepareXmlDoc(xml_path.c_str(), &docHandle);
    if (result != TRDP_NO_ERR) {
        throw std::runtime_error("Failed to parse TRDP XML document");
    }

    TRDP_MEM_CONFIG_T memConfig {};
    TRDP_DBG_CONFIG_T dbgConfig {};
    UINT32 numComPar = 0u;
    TRDP_COM_PAR_T *pComPar = nullptr;
    UINT32 numIfConfig = 0u;
    TRDP_IF_CONFIG_T *pIfConfig = nullptr;

    UINT32 numComId = 0u;
    TRDP_COMID_DSID_MAP_T *pComIdMap = nullptr;
    UINT32 numDataset = 0u;
    TRDP_DATASET_T **ppDataset = nullptr;

    const auto nameMap = parseTelegramNames(xml_path);

    try {
        result = tau_readXmlDeviceConfig(&docHandle, &memConfig, &dbgConfig, &numComPar, &pComPar, &numIfConfig, &pIfConfig);
        if (result != TRDP_NO_ERR) {
            throw std::runtime_error("Failed to read TRDP device configuration");
        }

        result = tau_readXmlDatasetConfig(&docHandle, &numComId, &pComIdMap, &numDataset, &ppDataset);
        if (result != TRDP_NO_ERR) {
            throw std::runtime_error("Failed to read TRDP dataset configuration");
        }

        datasets_.reserve(numDataset);
        for (UINT32 idx = 0u; idx < numDataset; ++idx) {
            Dataset dataset {};
            dataset.id = ppDataset[idx]->id;
            dataset.name = ppDataset[idx]->name;
            dataset.elements.reserve(ppDataset[idx]->numElement);

            for (UINT16 elemIdx = 0u; elemIdx < ppDataset[idx]->numElement; ++elemIdx) {
                DatasetElement element {};
                element.name = ppDataset[idx]->pElement[elemIdx].name != nullptr ? ppDataset[idx]->pElement[elemIdx].name : "";
                element.type = ppDataset[idx]->pElement[elemIdx].type;
                element.array_size = ppDataset[idx]->pElement[elemIdx].size;
                dataset.elements.push_back(element);
            }
            datasets_.push_back(dataset);
        }

        interfaces_.reserve(numIfConfig);
        for (UINT32 idx = 0u; idx < numIfConfig; ++idx) {
            InterfaceDef iface {};
            iface.name = pIfConfig[idx].ifName;
            iface.network_id = pIfConfig[idx].networkId;
            iface.host_ip = vos_ipDotted(pIfConfig[idx].hostIp);
            interfaces_.push_back(iface);

            TRDP_PROCESS_CONFIG_T processConfig {};
            TRDP_PD_CONFIG_T pdConfig {};
            TRDP_MD_CONFIG_T mdConfig {};
            UINT32 numExchgPar = 0u;
            TRDP_EXCHG_PAR_T *pExchgPar = nullptr;

            result = tau_readXmlInterfaceConfig(&docHandle, pIfConfig[idx].ifName, &processConfig, &pdConfig, &mdConfig, &numExchgPar, &pExchgPar);
            if (result != TRDP_NO_ERR) {
                throw std::runtime_error("Failed to read TRDP interface configuration");
            }

            for (UINT32 telIdx = 0u; telIdx < numExchgPar; ++telIdx) {
                const TRDP_EXCHG_PAR_T &exchange = pExchgPar[telIdx];
                PdTelegramDef telegram {};

                const auto nameIt = nameMap.find(exchange.comId);
                telegram.name = nameIt != nameMap.end() ? nameIt->second : std::string {};
                telegram.com_id = exchange.comId;
                telegram.dataset_id = exchange.datasetId;
                telegram.direction = determineDirection(exchange, host_name);
                telegram.cycle_us = exchange.pPdPar != nullptr ? exchange.pPdPar->cycle : 0u;
                telegram.marshall = exchange.pPdPar != nullptr ? (exchange.pPdPar->flags & TRDP_FLAGS_MARSHALL) != 0u
                                                               : (pdConfig.flags & TRDP_FLAGS_MARSHALL) != 0u;
                telegram.interface_name = iface.name;

                pdTelegrams_.push_back(telegram);
            }

            tau_freeTelegrams(numExchgPar, pExchgPar);
        }
    } catch (...) {
        if (ppDataset != nullptr) {
            tau_freeXmlDatasetConfig(numComId, pComIdMap, numDataset, ppDataset);
        }
        if (pIfConfig != nullptr) {
            vos_memFree(pIfConfig);
        }
        if (pComPar != nullptr) {
            vos_memFree(pComPar);
        }
        tau_freeXmlDoc(&docHandle);
        throw;
    }

    if (ppDataset != nullptr) {
        tau_freeXmlDatasetConfig(numComId, pComIdMap, numDataset, ppDataset);
    }
    if (pIfConfig != nullptr) {
        vos_memFree(pIfConfig);
    }
    if (pComPar != nullptr) {
        vos_memFree(pComPar);
    }
    tau_freeXmlDoc(&docHandle);
}

const std::vector<InterfaceDef> &TrdpConfigLoader::interfaces() const { return interfaces_; }

const std::vector<PdTelegramDef> &TrdpConfigLoader::pdTelegrams() const { return pdTelegrams_; }

const std::vector<Dataset> &TrdpConfigLoader::datasets() const { return datasets_; }

}  // namespace trdp

