#include <stddef.h>

#include "iec61375-2-3.h"
#include "tau_ctrl.h"
#include "tau_ctrl_types.h"
#include "tau_dnr.h"
#include "tau_dnr_types.h"
#include "tau_marshall.h"
#include "tau_so_if.h"
#include "tau_tti.h"
#include "tau_tti_types.h"
#include "tau_xml.h"
#include "trdp_if_light.h"
#include "trdp_serviceRegistry.h"
#include "trdp_tsn_def.h"
#include "trdp_types.h"

int main(void)
{
    /* Instantiate common configuration and type structures to ensure availability. */
    TRDP_MEM_CONFIG_T memConfig = {0};
    TRDP_PROCESS_CONFIG_T processConfig = {0};
    TRDP_APP_SESSION_T session = NULL;

    TRDP_PD_CONFIG_T pdConfig = {0};
    TRDP_MD_CONFIG_T mdConfig = {0};
    TRDP_PD_INFO_T pdInfo = {0};
    TRDP_TIME_T interval = {0};
    TRDP_FDS_T fds = {0};
    TRDP_SOCK_T numDesc = 0;

    /* tau_ctrl_types / tau_ctrl */
    TRDP_ECSP_CTRL_T ctrlInfo = {0};

    /* tau_dnr_types / tau_dnr */
    TRDP_DNS_REQUEST_T dnsRequest = {0};

    /* tau_marshall */
    void *marshallContext = NULL;
    TRDP_DATASET_T *cachedDataset = NULL;
    UINT32 destSize = 0;

    /* tau_so_if */
    SRM_SERVICE_INFO_T serviceInfo = {0};

    /* tau_tti_types / tau_tti */
    TRDP_OP_TRAIN_DIR_STATUS_INFO_T statusInfo = {0};
    UINT16 consistCount = 0;

    /* tau_xml */
    TRDP_XML_DOC_HANDLE_T xmlHandle = {0};

    /* IEC 61375 definitions and other TRDP constants */
    UINT16 pdPort = TRDP_PD_UDP_PORT;
    UINT8 serviceRegistryFlag = TRDP_SR_FLAG_EVENT;
    UINT32 srmComId = SRM_SERVICE_READ_REQ_COMID;
    UINT8 tsnMessage = TRDP_MSG_TSN_PD;

    /* Reference at least one API symbol from each header to validate linkage. */
    static volatile void *symbol_refs[] = {
        /* trdp_if_light.h */
        (void *)&tlc_init,
        (void *)&tlc_terminate,
        (void *)&tlc_openSession,
        (void *)&tlc_closeSession,
        (void *)&tlc_getOwnIpAddress,
        (void *)&tlc_getInterval,
        /* tau_ctrl.h */
        (void *)&tau_initEcspCtrl,
        (void *)&tau_setEcspCtrl,
        /* tau_dnr.h */
        (void *)&tau_initDnr,
        (void *)&tau_DNRstatus,
        /* tau_marshall.h */
        (void *)&tau_initMarshall,
        (void *)&tau_marshall,
        /* tau_so_if.h */
        (void *)&tau_addService,
        /* tau_tti.h */
        (void *)&tau_getOpTrnDirectoryStatusInfo,
        (void *)&tau_getTrnCstCnt,
        /* tau_xml.h */
        (void *)&tau_prepareXmlDoc,
        (void *)&tau_freeXmlDoc
    };

    size_t live_symbols = 0;
    for (size_t i = 0; i < (sizeof(symbol_refs) / sizeof(symbol_refs[0])); ++i)
    {
        live_symbols += (symbol_refs[i] != NULL);
    }

    /* Aggregate values to keep the compiler/linker from discarding references. */
    UINT32 aggregate = (UINT32)live_symbols + pdPort + serviceRegistryFlag + tsnMessage + TRDP_MAX_CST_CNT +
                       ctrlInfo.leadVehOfCst + dnsRequest.tcnUriCnt + statusInfo.state.opTrnDirState +
                       consistCount + srmComId + pdConfig.timeout + mdConfig.replyTimeout + pdInfo.comId +
                       interval.tv_usec + (UINT32)sizeof(fds) + numDesc + (UINT32)(size_t)marshallContext + destSize +
                       (UINT32)(size_t)cachedDataset + (UINT32)(size_t)serviceInfo.serviceId +
                       (UINT32)(size_t)xmlHandle.pXmlDocument + memConfig.size + processConfig.options;

    return (aggregate != 0u || session != NULL) ? 0 : 1;
}
