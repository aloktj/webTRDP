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
    TRDP_MEM_CONFIG_T memConfig = {NULL, 0u, {0}};
    TRDP_PROCESS_CONFIG_T processConfig = {0};
    TRDP_APP_SESSION_T session = NULL;

    TRDP_ERR_T err = tlc_init(NULL, NULL, &memConfig);
    err = tlc_openSession(&session, 0u, 0u, NULL, NULL, NULL, &processConfig);

    (void)tlc_getOwnIpAddress(session);
    TRDP_TIME_T interval = {0};
    TRDP_FDS_T fds = {0};
    TRDP_SOCK_T numDesc = 0;
    (void)tlc_getInterval(session, &interval, &fds, &numDesc);

    /* IEC 61375 definitions and base TRDP types */
    UINT16 pdPort = TRDP_PD_UDP_PORT;
    TRDP_PD_CONFIG_T pdConfig = {0};
    TRDP_MD_CONFIG_T mdConfig = {0};
    TRDP_PD_INFO_T pdInfo = {0};

    /* tau_ctrl_types / tau_ctrl */
    TRDP_ECSP_CTRL_T ctrlInfo = {0};
    (void)tau_initEcspCtrl(session, 0u);
    (void)tau_setEcspCtrl(session, &ctrlInfo);

    /* tau_dnr_types / tau_dnr */
    TRDP_DNS_REQUEST_T dnsRequest = {0};
    (void)tau_initDnr(session, 0u, TRDP_MD_UDP_PORT, NULL, TRDP_DNR_COMMON_THREAD, FALSE);
    (void)tau_DNRstatus(session);

    /* tau_marshall */
    void *marshallContext = NULL;
    (void)tau_initMarshall(&marshallContext, 0u, NULL, 0u, NULL);
    TRDP_DATASET_T *cachedDataset = NULL;
    UINT32 destSize = 0;
    (void)tau_marshall(marshallContext, 0u, NULL, 0u, NULL, &destSize, &cachedDataset);

    /* tau_so_if */
    SRM_SERVICE_INFO_T serviceInfo = {0};
    (void)tau_addService(session, &serviceInfo, FALSE);

    /* tau_tti_types / tau_tti */
    TRDP_OP_TRAIN_DIR_STATUS_INFO_T statusInfo = {0};
    UINT16 consistCount = 0;
    (void)tau_getOpTrnDirectoryStatusInfo(session, &statusInfo);
    (void)tau_getTrnCstCnt(session, &consistCount);

    /* tau_xml */
    TRDP_XML_DOC_HANDLE_T xmlHandle = {0};
    (void)tau_prepareXmlDoc(NULL, &xmlHandle);
    (void)tau_freeXmlDoc(&xmlHandle);

    /* trdp_serviceRegistry */
    UINT8 serviceRegistryFlag = TRDP_SR_FLAG_EVENT;
    UINT32 srmComId = SRM_SERVICE_READ_REQ_COMID;

    /* trdp_tsn_def */
    UINT8 tsnMessage = TRDP_MSG_TSN_PD;

    /* tau_dnr_types use */
    TCN_URI_T uriEntry = {0};

    /* Aggregate values to keep the compiler happy */
    UINT32 aggregate = pdPort + serviceRegistryFlag + tsnMessage + TRDP_MAX_CST_CNT +
                       ctrlInfo.leadVehOfCst + dnsRequest.tcnUriCnt + statusInfo.state.opTrnDirState +
                       consistCount + srmComId + pdConfig.timeout + mdConfig.replyTimeout + pdInfo.comId +
                       uriEntry.tcnUriIpAddr;
    (void)aggregate;

    (void)tlc_terminate();
    return (err == TRDP_NO_ERR) ? 0 : 1;
}
