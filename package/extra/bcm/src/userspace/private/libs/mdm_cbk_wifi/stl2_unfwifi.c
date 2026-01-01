/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
************************************************************************/

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_WIFIRADIO_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"
#include "rut_util.h"
#include "rut2_wifi.h"
#include "rut_system.h"
#include "rut_lan.h"
#include "wlsysutil.h"
#include "rut_qos.h"

#ifdef BRCM_BDK_BUILD
#define IS_WL_PRIORITY_GET(eid) (((eid) != EID_WIFI_MD) && \
                                 ((eid) != EID_MDM_CLI))

#else
#define IS_WL_PRIORITY_GET(eid) (((eid) != EID_TR69C) && \
                                 ((eid) != EID_CONSOLED) && \
                                 ((eid) != EID_TELNETD) && \
                                 ((eid) != EID_SSHD))
#endif
                                 

CmsRet stl_dev2WifiObject( _Dev2WifiObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiBsdCfgObject( _Dev2WifiBsdCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiWpsCfgObject( _Dev2WifiWpsCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
CmsRet stl_dev2WifiNasCfgObject( _Dev2WifiNasCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiDebugMonitorCfgObject( _Dev2WifiDebugMonitorCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiCeventdCfgObject( _Dev2WifiCeventdCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiHapdCfgObject( _Dev2WifiHapdCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiSsdCfgObject( _Dev2WifiSsdCfgObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiWbdCfgObject(_Dev2WifiWbdCfgObject *obj __attribute__((unused)), 
         const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiWbdCfgMbssObject(_Dev2WifiWbdCfgMbssObject *obj __attribute__((unused)),
      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiRadioObject( _Dev2WifiRadioObject *obj,
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    unsigned int channel = 0, bandwidth = 0;
    char  buffer[BUFLEN_1024] = {0};
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

    if (obj->enable == FALSE)
    {
        if (cmsUtl_strcmp(obj->status, MDMVS_DOWN) != 0)
        {
            CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
            ret = CMSRET_SUCCESS;
        }
        goto exit;
    }
    
    /** The below parameter value massage is for tr69-tr181 specification only.
     *  if the object query comes from wlmdm/nvram/wlssk, skip these part to get better performance.
     */
    if (IS_WL_PRIORITY_GET(GENERIC_EID(cmsMsg_getHandleEid(msgHandle))))
        goto exit;

    // status is down, Don't get any status through wlsysutil
    if (cmsUtl_strcmp(obj->status, MDMVS_DOWN) == 0)
        goto exit;

    wlgetChannelList (obj->name, buffer, BUFLEN_1024);
    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->possibleChannels, buffer, mdmLibCtx.allocFlags);

    wlgetChanspec(obj->name, &channel, &bandwidth);
    if (obj->autoChannelEnable == TRUE)
        obj->channel = channel; 

    snprintf(buffer, BUFLEN_1024, "%d", channel);
    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->channelsInUse, buffer, mdmLibCtx.allocFlags);

    snprintf(buffer, BUFLEN_1024, "%dMHz", bandwidth);
    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentOperatingChannelBandwidth, buffer, mdmLibCtx.allocFlags);

    obj->maxBitRate =  wlgetRate(obj->name);

exit:
    /* Calculate and return the TR181 LastChange */
    IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

    return ret;
}

CmsRet stl_dev2WifiRadioStatsObject(_Dev2WifiRadioStatsObject *obj,
                                    const InstanceIdStack *iidStack)
{
    _Dev2WifiSsidObject *ssidObj = NULL;
    InstanceIdStack ssidIidStack = EMPTY_INSTANCE_ID_STACK;
    MdmPathDescriptor pathDesc;
    char *fullPathString = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    char radioName[BUFLEN_16] = "\0";
    struct RadioCounters radioCounter = {0};

    UINT64 bytesReceived = 0, packetsReceived = 0, multicastBytesReceived = 0, multicastPacketsReceived = 0;
    UINT64 unicastPacketsReceived = 0, broadcastPacketsReceived = 0, errorsReceived = 0, discardPacketsReceived = 0;
    UINT64 bytesSent = 0, packetsSent = 0, multicastBytesSent = 0, multicastPacketsSent = 0;
    UINT64 unicastPacketsSent = 0, broadcastPacketsSent = 0, errorsSent = 0, discardPacketsSent = 0;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
    memcpy(&pathDesc.iidStack, iidStack, sizeof(InstanceIdStack));
    cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);

    obj->bytesReceived = 0;
    obj->packetsReceived = 0;
    obj->errorsReceived = 0;
    obj->discardPacketsReceived = 0;

    obj->bytesSent = 0;
    obj->packetsSent = 0;
    obj->errorsSent = 0;
    obj->discardPacketsSent = 0;
    obj->PLCPErrorCount = 0;
    obj->FCSErrorCount = 0;
    obj->invalidMACCount = 0;
    obj->packetsOtherReceived = 0;
    obj->noise = 0;

    while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_SSID,
                                      &ssidIidStack,
                                      OGF_NO_VALUE_UPDATE,
                                      (void **)&ssidObj)) == CMSRET_SUCCESS)
    {
        if ( cmsUtl_strcmp(fullPathString, ssidObj->lowerLayers) == 0 )
        {
            // use first matched ssid name as radio device name
            if (strlen(radioName) == 0)
                sprintf(radioName, ssidObj->name);

            rut_getIntfStats_uint64(ssidObj->name, &bytesReceived, &packetsReceived,
                                    &multicastBytesReceived, &multicastPacketsReceived, &unicastPacketsReceived, &broadcastPacketsReceived,
                                    &errorsReceived, &discardPacketsReceived,
                                    &bytesSent, &packetsSent,
                                    &multicastBytesSent, &multicastPacketsSent, &unicastPacketsSent, &broadcastPacketsSent,
                                    &errorsSent, &discardPacketsSent);

            obj->bytesReceived += bytesReceived;
            obj->packetsReceived += packetsReceived;
            obj->errorsReceived += (UINT32)errorsReceived;
            obj->discardPacketsReceived += (UINT32)discardPacketsReceived;

            obj->bytesSent += bytesSent;
            obj->packetsSent += packetsSent;
            obj->errorsSent += (UINT32)errorsSent;
            obj->discardPacketsSent += (UINT32)discardPacketsSent;
        }

        cmsObj_free((void **) &ssidObj);
    }

    // wifi only statistical counters
    if (strlen(radioName)!=0)
    {
        if ((ret = rutWifi_getRadioCounters(radioName, &radioCounter)) == CMSRET_SUCCESS)
        {
            obj->PLCPErrorCount = radioCounter.PLCPErrorCount;
            obj->FCSErrorCount = radioCounter.FCSErrorCount;
            obj->invalidMACCount = radioCounter.invalidMACCount;
            obj->packetsOtherReceived = radioCounter.packetsOtherReceived;
            obj->noise = radioCounter.noise;
        }
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
    return CMSRET_SUCCESS;
}

CmsRet stl_dev2WifiRadioAcsdCfgObject( _Dev2WifiRadioAcsdCfgObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}

CmsRet stl_dev2WifiNeighboringwifidiagnosticObject( _Dev2WifiNeighboringwifidiagnosticObject *obj __attribute__((unused)),
                                                    const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2WifiNeighboringwifidiagnosticResultObject( _Dev2WifiNeighboringwifidiagnosticResultObject *obj __attribute__((unused)),
                                                          const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}



CmsRet stl_dev2WifiSsidObject( _Dev2WifiSsidObject *obj,
                               const InstanceIdStack *iidStack __attribute__((unused)))
{
    UBOOL8 isLinkUp = 0;
    UBOOL8 isBssUp = 0;
    UBOOL8 isStatusChanged = 0;
    char currentStatus[BUFLEN_16]= {0};
    char propageLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]= {0};
    char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]= {0};
    int i = 0;
    MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
    _Dev2WifiRadioObject *radioObj = NULL;
    CmsRet ret;
 
    // get the first lower layer full path
    cmsUtl_strncpy(propageLayerBuf, obj->lowerLayers, sizeof(propageLayerBuf));
    while ((propageLayerBuf[i] != 0) &&
            (propageLayerBuf[i] != ',') &&
            (propageLayerBuf[i] != ' ') && (i < sizeof(lowerLayerBuf)-1))
    {
        lowerLayerBuf[i] = propageLayerBuf[i];
        i++;
    }
    /* Assume SSID lowerLayer will only have one fullpath */
    ret = cmsMdm_fullPathToPathDescriptor(lowerLayerBuf, &pathDesc);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not convert lowerLayer %s, ret=%d", obj->lowerLayers, ret);
        return ret;
    }
    
    if (pathDesc.oid != MDMOID_DEV2_WIFI_RADIO)
    {
        cmsLog_error("Expected LowerLayers to point to Wifi Radio (%d) got %d",
                     MDMOID_DEV2_WIFI_RADIO, pathDesc.oid);
        return ret;
    }
    
    ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE, (void **)&radioObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get Wifi Radio object, ret=%d", ret);
        return ret;
    }
    else
    {
        if (!radioObj->enable && obj->enable)
        {
            CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
            obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
        }
        cmsObj_free((void *) &radioObj);
    }
   
    isLinkUp = cmsNet_isInterfaceLinkUp(obj->name);
    if (isLinkUp)
    {
        wlgetBssStatus(obj->name, currentStatus, BUFLEN_16); 
        if (strncasecmp(currentStatus, MDMVS_UP, strlen(MDMVS_UP)) == 0)
            isBssUp = 1;
    }

    if (!obj->enable)
    {
        if (!isBssUp && !cmsUtl_strcmp(obj->status, MDMVS_UP))
        {
            CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
            isStatusChanged = 1;
        }
    }
    else
    {
        if (isLinkUp && cmsUtl_strcmp(obj->status, MDMVS_UP))
        {
            /* Link has come UP */
            CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
            isStatusChanged = 1;
        }
        else if (!isLinkUp && !cmsUtl_strcmp(obj->status, MDMVS_UP))
        {
            /* Link has gone down */
            CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DORMANT, mdmLibCtx.allocFlags);
            isStatusChanged = 1;
        }
    }

    if (isStatusChanged)
    {
        obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
        rutQos_reconfigAllQueuesOnLayer2Intf_dev2(obj->name);

        /* some classifications may reference this intf on ingress side,
         * so check if they need to be configured.
         */
        rutQos_reconfigAllClassifications_dev2(obj->name);
    }

    /* Calculate and return the TR181 LastChange */
    IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

    return CMSRET_SUCCESS;
}

CmsRet stl_dev2WifiSsidStatsObject(_Dev2WifiSsidStatsObject *obj,
                                   const InstanceIdStack *iidStack)
{
    _Dev2WifiSsidObject *ssidObj = NULL;
    InstanceIdStack parentIidStack = *iidStack;
    CmsRet ret = CMSRET_SUCCESS;

    if ((ret = cmsObj_getAncestor(MDMOID_DEV2_WIFI_SSID,
                                  MDMOID_DEV2_WIFI_SSID_STATS,
                                  &parentIidStack,
                                  (void **) &ssidObj)) == CMSRET_SUCCESS)
    {
        if (obj == NULL)
        {
            rut_clearWanIntfStats(ssidObj->name);
        }
        else
        {
            UINT64 bytesReceived = 0, packetsReceived = 0, multicastBytesReceived = 0, multicastPacketsReceived = 0;
            UINT64 unicastPacketsReceived = 0, broadcastPacketsReceived = 0, errorsReceived = 0, discardPacketsReceived = 0;
            UINT64 bytesSent = 0, packetsSent = 0, multicastBytesSent = 0, multicastPacketsSent = 0;
            UINT64 unicastPacketsSent = 0, broadcastPacketsSent = 0, errorsSent = 0, discardPacketsSent = 0;

            struct SSIDCounters ssidCounter = {0};

            rut_getIntfStats_uint64(ssidObj->name, &bytesReceived, &packetsReceived,
                                    &multicastBytesReceived, &multicastPacketsReceived, &unicastPacketsReceived, &broadcastPacketsReceived,
                                    &errorsReceived, &discardPacketsReceived,
                                    &bytesSent, &packetsSent,
                                    &multicastBytesSent, &multicastPacketsSent, &unicastPacketsSent, &broadcastPacketsSent,
                                    &errorsSent, &discardPacketsSent);

            obj->bytesReceived = bytesReceived;
            obj->packetsReceived = packetsReceived;
            obj->multicastPacketsReceived = multicastPacketsReceived;
            obj->unicastPacketsReceived = unicastPacketsReceived;
            obj->broadcastPacketsReceived = broadcastPacketsReceived;
            obj->errorsReceived = (UINT32)errorsReceived;
            obj->discardPacketsReceived = (UINT32)discardPacketsReceived;

            obj->bytesSent = bytesSent;
            obj->packetsSent = packetsSent;
            obj->multicastPacketsSent = multicastPacketsSent;
            obj->unicastPacketsSent = unicastPacketsSent;
            obj->broadcastPacketsSent = broadcastPacketsSent;
            obj->errorsSent = (UINT32)errorsSent;
            obj->discardPacketsSent = (UINT32)discardPacketsSent;

            obj->unknownProtoPacketsReceived = 0;

            // wifi only statistical counters
            rutWifi_getSSIDCounters(ssidObj->name, &ssidCounter);
            obj->retransCount = ssidCounter.retransCount;
            obj->failedRetransCount = ssidCounter.failedRetransCount;
            obj->retryCount = ssidCounter.retryCount;
            obj->multipleRetryCount = ssidCounter.multipleRetryCount;
            obj->ACKFailureCount = ssidCounter.ACKFailureCount;
            obj->aggregatedPacketCount = ssidCounter.aggregatedPacketCount;
        }
        cmsObj_free((void **) &ssidObj);
    }

    return ret;

}

CmsRet stl_dev2WifiSsidBsdCfgObject( _Dev2WifiSsidBsdCfgObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}
CmsRet stl_dev2WifiSsidHspotCfgObject( _Dev2WifiSsidHspotCfgObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}
CmsRet stl_dev2WifiSsidSsdCfgObject( _Dev2WifiSsidSsdCfgObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}

#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
CmsRet stl_dev2WifiAccessPointObject( _Dev2WifiAccessPointObject *obj __attribute__((unused)),
                                      const InstanceIdStack *iidStack __attribute__((unused)))
{
    Dev2WifiSsidObject *ssidObj = rutWifi_get_AP_SSID_dev2(obj);
    if(ssidObj) {
        if(!cmsUtl_strcmp(ssidObj->status,MDMVS_UP)) {
            CMSMEM_REPLACE_STRING_FLAGS(obj->status,MDMVS_ENABLED,mdmLibCtx.allocFlags);
        } else
            CMSMEM_REPLACE_STRING_FLAGS(obj->status,MDMVS_DISABLED,mdmLibCtx.allocFlags);
        cmsObj_free((void **)&ssidObj);
    }
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2WifiAccessPointSecurityObject( _Dev2WifiAccessPointSecurityObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiAccessPointAccountingObject( _Dev2WifiAccessPointAccountingObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiAccessPointWpsObject( _Dev2WifiAccessPointWpsObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#define DATA_MINRATE 1000

CmsRet stl_dev2WifiAssociatedDeviceObject( _Dev2WifiAssociatedDeviceObject *obj,
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    struct AssocDevCounters assocDevCounters = {0};

    // wifi only statistical counters
    rutWifi_getAssocDevCounters(obj->X_BROADCOM_COM_BssIfname, obj->MACAddress, &assocDevCounters);

    obj->lastDataDownlinkRate = assocDevCounters.lastDataDownlinkRate >= DATA_MINRATE ? assocDevCounters.lastDataDownlinkRate : DATA_MINRATE;
    obj->lastDataUplinkRate   = assocDevCounters.lastDataUplinkRate >= DATA_MINRATE ? assocDevCounters.lastDataUplinkRate : DATA_MINRATE;
    obj->signalStrength       = assocDevCounters.signalStrength ;
    obj->noise                = assocDevCounters.noise;

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiAssociateddeviceStatsObject( _Dev2WifiAssociateddeviceStatsObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    Station_Stats stats;
    _Dev2WifiAssociatedDeviceObject *assocDevObj = NULL;
    InstanceIdStack parentIidStack = *iidStack;

    if (cmsObj_getAncestorFlags(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,
                                  MDMOID_DEV2_WIFI_ASSOCIATEDDEVICE_STATS,
                                  &parentIidStack, OGF_NO_VALUE_UPDATE,
                                  (void **) &assocDevObj) == CMSRET_SUCCESS)
    {
        wlgetStationStats(assocDevObj->X_BROADCOM_COM_BssIfname, assocDevObj->MACAddress, &stats);
        cmsLog_debug("bytesSent:%llu bytesReceived:%llu packetsSent:%llu packetsReceived:%llu\n",
                     stats.bytesSent, stats.bytesReceived, stats.packetsSent, stats.packetsReceived);

        obj->bytesSent = stats.bytesSent;
        obj->bytesReceived = stats.bytesReceived;
        obj->packetsSent = stats.packetsSent;
        obj->packetsReceived = stats.packetsReceived;
        obj->errorsSent = stats.errorsSent;
        obj->retransCount = stats.retransCount;
        obj->failedRetransCount = stats.failedRetransCount;
        obj->retryCount = stats.retryCount;
#ifdef NOT_SUPPORT
        obj->multipleRetryCount = stats.multipleRetryCount;
#endif
        cmsObj_free((void **) &assocDevObj);
        return CMSRET_SUCCESS;
    }
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2WifiAccessPointAcObject ( _Dev2WifiAccessPointAcObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiAccessPointAcStatsObject ( _Dev2WifiAccessPointAcStatsObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#ifdef DMP_DEVICE2_WIFIENDPOINT_1


CmsRet stl_dev2WifiEndPointObject( _Dev2WifiEndPointObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointStatsObject( _Dev2WifiEndPointStatsObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointSecurityObject( _Dev2WifiEndPointSecurityObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointProfileObject( _Dev2WifiEndPointProfileObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointProfileSecurityObject( _Dev2WifiEndPointProfileSecurityObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointWpsObject( _Dev2WifiEndPointWpsObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointAcObject(_Dev2WifiEndPointAcObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiEndPointAcStatsObject(_Dev2WifiEndPointAcStatsObject *obj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_DEVICE2_WIFIENDPOINT_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
