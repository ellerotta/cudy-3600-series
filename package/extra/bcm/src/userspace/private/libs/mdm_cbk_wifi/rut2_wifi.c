/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <mdm.h>
#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_mdm.h"

#include "cms_msg_pubsub.h"

#ifdef DMP_DEVICE2_WIFIRADIO_1

/*!\file rut2_wifi.c
 * \brief This file contains common TR181 Wifi helper functions.
 *
 */

#include "rut2_wifi.h"
#include "qdm_intf.h"
#include "rut_util.h"

/* tmp buf used to form a config line */
char wifi_configBuf[BUFLEN_128];

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1


Dev2WifiSsidObject* rutWifi_get_AP_SSID_dev2(_Dev2WifiAccessPointObject  *obj)
{
    int ssid_idx=0;
    sscanf(obj->SSIDReference,"Device.WiFi.SSID.%d",&ssid_idx);
    if (ssid_idx) {
        InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
        _Dev2WifiSsidObject *ssidObj=NULL;
        iidStack.instance[0]=ssid_idx;
        iidStack.currentDepth=1;
        if ((cmsObj_get(MDMOID_DEV2_WIFI_SSID, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ssidObj)) == CMSRET_SUCCESS) {
            return ssidObj;
        }
    }
    return NULL;
}


CmsRet rutWifi_get_AP_Radio_dev2(const _Dev2WifiAccessPointObject  *apObj,void **radioObj,InstanceIdStack *iidStack) {

    iidStack->instance[0] = apObj->X_BROADCOM_COM_Adapter +1 ;
    iidStack->currentDepth=1;
    return cmsObj_get(MDMOID_DEV2_WIFI_RADIO, iidStack, 0, radioObj);
}


void rutWifi_Clear_AssocicatedDevices(Dev2WifiAccessPointObject *apObj,const InstanceIdStack *iidStack) {
    InstanceIdStack adIidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2WifiAssociatedDeviceObject *associatedDeviceObj=NULL;
    while ((cmsObj_getNextInSubTree(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,
                                    iidStack,
                                    &adIidStack,
                                    (void **) &associatedDeviceObj)) == CMSRET_SUCCESS) {
        cmsObj_deleteInstance(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, &adIidStack);
        INIT_INSTANCE_ID_STACK(&adIidStack);
        cmsObj_free((void **) &associatedDeviceObj);

    }
}

static void rutWifi_updateSTADHCPV4Client(const char* MACAddress, unsigned int op)
{
    InstanceIdStack dhcp_client_iidStack=EMPTY_INSTANCE_ID_STACK;
    Dev2Dhcpv4ServerPoolClientObject *dhcp_clientObj = NULL;
    while (cmsObj_getNextFlags(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                               &dhcp_client_iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&dhcp_clientObj) == CMSRET_SUCCESS)
    {
         if (!cmsUtl_strcasecmp(dhcp_clientObj->chaddr, MACAddress))
         {
             if(op==REMOVE_ENTRY)
             {
                 cmsObj_deleteInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,&dhcp_client_iidStack);
                 cmsLog_notice("REMOVE ENTRY.....");
             }
             else {
                 dhcp_clientObj->active=(op==ACTIVE_ENTRY)?1:0;
                 cmsLog_debug("set active:%d",dhcp_clientObj->active);
                 cmsObj_set(dhcp_clientObj,&dhcp_client_iidStack);
             }
         }
         cmsObj_free((void **) &dhcp_clientObj);
    }
}

CmsRet rutWifi_updateSTAHostEntry(const char* ifname, const char* MACAddress, unsigned int staInstance, unsigned int op)
{
    UBOOL8 found = FALSE;
    Dev2HostObject *hostObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    char *fullPath = NULL;

    if ((ret=qdmIntf_intfnameToFullPathLocked_dev2(ifname, TRUE, &fullPath)) != CMSRET_SUCCESS)
        return ret;

    /* Don't care DHCP lease time here , so avoid trigger STL during traverse */
    while (found == FALSE &&
           (ret = cmsObj_getNextFlags(MDMOID_DEV2_HOST, &iidStack,OGF_NO_VALUE_UPDATE, (void **)&hostObj)) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcasecmp(hostObj->layer1Interface, fullPath) == 0 &&
            cmsUtl_strcasecmp(hostObj->physAddress, MACAddress) == 0)
        {
            found = TRUE;
            switch (op)
            {
            case REMOVE_ENTRY:
                cmsLog_debug("REMOVE ENTRY.....");
                cmsObj_deleteInstance(MDMOID_DEV2_HOST, &iidStack);
                break;

            case INACTIVE_ENTRY:
                cmsLog_debug("INACTIVE ENTRY.....");
                CMSMEM_REPLACE_STRING(hostObj->associatedDevice, "");
                hostObj->active = 0;
                cmsObj_set(hostObj,&iidStack);
                break;

            case ACTIVE_ENTRY:
                {
                    cmsLog_debug("ACTIVE ENTRY.....");
                    MdmPathDescriptor pathDesc={0};
                    char *assocFullPath = NULL;
                    cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);
                    pathDesc.oid = MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE;
                    PUSH_INSTANCE_ID(&pathDesc.iidStack, staInstance);

                    if ((ret=cmsMdm_pathDescriptorToFullPath(&pathDesc, &assocFullPath)) != CMSRET_SUCCESS)
                        break;

                    CMSMEM_REPLACE_STRING(hostObj->associatedDevice, assocFullPath);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(assocFullPath);
                    hostObj->active = 1;
                    cmsObj_set(hostObj,&iidStack);
                }
                break;

            default:
                cmsLog_error("UNKNOWN OPERATION.....");
                break;
            }
            rutWifi_updateSTADHCPV4Client(MACAddress, op);
        }
        cmsObj_free((void **)&hostObj);
    }


    if (fullPath != NULL)
    {
        CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
    }

    return ret;
}

CmsRet rutWifi_sendAPClientAssocChanged_dev2(const char* ifname, const char* MACAddress, unsigned int staInstance, unsigned int op)
{
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    char msgBuf[sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody) + 128] = {0};  // max possible buf
    CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
    CmsRet ret = CMSRET_INTERNAL_ERROR;
    UINT32 myEid = (UINT32) cmsMsg_getHandleEid(msgHandle);

    // Fill in common fields of the CMS msg.
    msgHdr->src = myEid;
    msgHdr->dst = EID_SSK;

    if (cmsMdm_isCmsClassic())
    {
        char *data = (char *)(msgHdr+1);
        // construct value for ifname:MAC Address and event type
        snprintf(data, 128, "%s %u:%u:%s", ifname, staInstance, op, MACAddress);

        msgHdr->type = CMS_MSG_WIFI_UPDATE_ASSOCIATEDDEVICE;
        msgHdr->dataLength = strlen(data)+1;
    }
    else
    {
        PubSubKeyValueMsgBody *kvBody = (PubSubKeyValueMsgBody *)(msgHdr+1);
        char *value = (char *)(kvBody + 1);
        // compose key with prefix and ifname
        snprintf(kvBody->key, sizeof(kvBody->key), "%s:%s", BDK_KEY_AP_CLIENT_ASSOC_CHANGED_PREFIX, ifname);

        // construct value for instance,event type, and MAC Address
        snprintf(value, 128, "%u:%u:%s", staInstance, op, MACAddress);

        kvBody->valueLen = strlen(value)+1;

        msgHdr->flags_event = 1;
        msgHdr->type = CMS_MSG_PUBLISH_EVENT;
        msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
        msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody) + kvBody->valueLen;
    }

    ret = cmsMsg_send(msgHandle, msgHdr);
    if (ret != CMSRET_SUCCESS)
        cmsLog_error("Could not send msg, ret=%d", ret);

    return ret;
}

void rutWifi_find_AP_ByIndex_locked(int radioIndex, int ssidIndex, InstanceIdStack *iidStack, void **obj)
{
    CmsRet ret = CMSRET_SUCCESS;
    _Dev2WifiAccessPointObject *apObj = NULL;

    while ((ret = cmsObj_getNext(MDMOID_DEV2_WIFI_ACCESS_POINT, iidStack, (void **)&apObj )) == CMSRET_SUCCESS)
    {
        if (apObj->X_BROADCOM_COM_Index == ssidIndex && apObj->X_BROADCOM_COM_Adapter == radioIndex)
        {
            cmsLog_debug("find access point ID:%d/%d", apObj->X_BROADCOM_COM_Adapter,apObj->X_BROADCOM_COM_Index);
            break;
        }
        cmsObj_free((void **) &apObj);
    }

    if (apObj != NULL)
        *((_Dev2WifiAccessPointObject **)obj) = apObj;
}

#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#ifdef DESKTOP_LINUX
static void writeNvram_desktop(const char *configStr);
#endif

void rutWifi_writeNvram(const char *configStr)
{
#ifdef DESKTOP_LINUX
    writeNvram_desktop(configStr);
    return;
#endif

    // cmsLog_error("not implemented yet, configStr=%s", configStr);
    return;
}


CmsRet rutWifi_getRadioCounters(const char *devName, struct RadioCounters *rCounters)
{
    char cmdBuf[BUFLEN_128] = {0};
    FILE *fp = NULL;

    if (!rCounters || !devName || devName[0]!='w')
        return CMSRET_INVALID_ARGUMENTS;

    if (!rut_isDeviceFound(devName))
        return CMSRET_INVALID_ARGUMENTS;

    snprintf(cmdBuf, sizeof(cmdBuf), "wlctl -i %s counters > /var/%scounters", devName, devName);
    rut_doSystemAction("rutWifi", cmdBuf);
    snprintf(cmdBuf, sizeof(cmdBuf), "wlctl -i %s status >> /var/%scounters", devName, devName);
    rut_doSystemAction("rutWifi", cmdBuf);
    sprintf(cmdBuf, "/var/%scounters", devName);
    fp = fopen(cmdBuf, "r");
    if (fp)
    {
        char buf[BUFLEN_1024];
        while(fgets(buf, 1024, fp))
        {
            char *ptr;
            if ((rCounters->invalidMACCount == 0) && (ptr = strstr(buf, "rxbadproto")) != NULL)
                sscanf(ptr+11, "%u", &(rCounters->invalidMACCount));
            else if ((rCounters->packetsOtherReceived == 0) && (ptr = strstr(buf, "rxbadda")) != NULL)
                sscanf(ptr+8, "%u", &(rCounters->packetsOtherReceived));
            else if ((rCounters->PLCPErrorCount == 0) && (ptr = strstr(buf, "rxbadplcp")) != NULL)
                sscanf(ptr+10, "%u", &(rCounters->PLCPErrorCount));
            else if ((rCounters->FCSErrorCount == 0) && (ptr = strstr(buf, "rxbadfcs")) != NULL)
                sscanf(ptr+9, "%u", &(rCounters->FCSErrorCount));
            else if ((rCounters->noise == 0) && (ptr = strstr(buf, "noise:")) != NULL)
                sscanf(ptr+6, "%d", &(rCounters->noise));
        }
        fclose(fp);
    }
    unlink(cmdBuf);
    return CMSRET_SUCCESS;
}

CmsRet rutWifi_getSSIDCounters(const char *devName, struct SSIDCounters *sCounters)
{
    char cmdBuf[BUFLEN_128] = {0};
    FILE *fp = NULL;

    if (!sCounters || !devName || devName[0]!='w')
        return CMSRET_INVALID_ARGUMENTS;

    if (!rut_isDeviceFound(devName))
        return CMSRET_INVALID_ARGUMENTS;

    sprintf(cmdBuf, "wlctl -i %s counters > /var/%scounters", devName, devName);
    rut_doSystemAction("rutWifi", cmdBuf);
    sprintf(cmdBuf, "/var/%scounters", devName);
    fp = fopen(cmdBuf, "r");
    if (fp)
    {
        char buf[BUFLEN_1024];
        while(fgets(buf, 1024, fp))
        {
            char *ptr;
            if ((ptr = strstr(buf, "txretrans"))!= NULL)
                sscanf(ptr+10, "%d", &(sCounters->retransCount));
            if ((ptr = strstr(buf, "txfail")) != NULL)
                sscanf(ptr+7, "%d", &(sCounters->failedRetransCount));
            if ((ptr = strstr(buf, "d11_txretry")) != NULL)
                sscanf(ptr+12, "%d", &(sCounters->retryCount));
            if ((ptr = strstr(buf, "d11_txretrie")) != NULL)
                sscanf(ptr+13, "%d", &(sCounters->multipleRetryCount));
            if ((ptr = strstr(buf, "d11_txnoack")) != NULL)
                sscanf(ptr+12, "%d", &(sCounters->ACKFailureCount));
            if ((ptr = strstr(buf, "txampdu")) != NULL)
                sscanf(ptr+8, "%d", &(sCounters->aggregatedPacketCount));
        }
        fclose(fp);
    }
    unlink(cmdBuf);
    return CMSRET_SUCCESS;
}

#define STA_INFO_KEY_LASTRXRATE      "rate of last rx pkt:"
#define STA_INFO_KEY_LASTTXRATE      "rate of last tx pkt:"
#define STA_INFO_KEY_RSSI            "rssi of rx data frames:"
#define STA_INFO_KEY_NOISE           "noise floor:"

static char* getNumer(char* source, SINT32* value)
{
    char *ptr = source;
    int negtive = 0;
    int ans = 0;

    while ((*ptr != '\0') && (*ptr != ' '))
    {
        if (isdigit(*ptr))
            ans = 10*ans + (*ptr-'0');
        else if (!negtive && *ptr=='-')
            negtive = 1;

        ptr++;
    }
    *value = negtive ? ans*-1 : ans;

    if (*ptr == '\0')
        return NULL;
    else 
        return ptr+1;
}

CmsRet rutWifi_getAssocDevCounters(const char *devName, const char *staMACAddr, struct AssocDevCounters *aCounters)
{
    char cmdBuf[BUFLEN_256] = {0};
    FILE *fp = NULL;

    if (!aCounters || !devName || devName[0]!='w')
        return CMSRET_INVALID_ARGUMENTS;

    if (!rut_isDeviceFound(devName))
        return CMSRET_INVALID_ARGUMENTS;

    snprintf(cmdBuf, sizeof(cmdBuf), "wlctl -i %s sta_info %s > /var/%s_%scounters",
                                     devName, staMACAddr, devName, staMACAddr);
    rut_doSystemAction("rutWifi", cmdBuf);
    sprintf(cmdBuf, "/var/%s_%scounters", devName, staMACAddr);
    fp = fopen(cmdBuf, "r");
    if (fp)
    {
        char buf[BUFLEN_1024];
        while(fgets(buf, 1024, fp))
        {
            char *ptr;
            if ((ptr = strstr(buf, STA_INFO_KEY_LASTRXRATE))!= NULL)
                sscanf(ptr+strlen(STA_INFO_KEY_LASTRXRATE), "%u", &(aCounters->lastDataDownlinkRate));
            else if ((ptr = strstr(buf, STA_INFO_KEY_LASTTXRATE))!= NULL)
                sscanf(ptr+strlen(STA_INFO_KEY_LASTTXRATE), "%u", &(aCounters->lastDataUplinkRate));
            else if ((ptr = strstr(buf, STA_INFO_KEY_RSSI))!= NULL)
            {
                /* This is per antenna average data, take the first non-zero as result. */
                aCounters->signalStrength = 0; 
                ptr += strlen(STA_INFO_KEY_RSSI);
                while (!aCounters->signalStrength && ptr != NULL)
                    ptr = getNumer(ptr, &(aCounters->signalStrength));
            }
            else if ((ptr = strstr(buf, STA_INFO_KEY_NOISE)) != NULL)
            {
                /* This is per antenna noise floor, take the first non-zero as result. */
                aCounters->noise = 0;
                ptr += strlen(STA_INFO_KEY_NOISE);
                while (!aCounters->noise && ptr != NULL)
                    ptr = getNumer(ptr, &(aCounters->noise));
            }

        }
        fclose(fp);
    }
    unlink(cmdBuf);
    return CMSRET_SUCCESS;
}

#ifdef DESKTOP_LINUX
#define WIFI_DESKTOP_NVRAM  "wifi-nvram.txt"

static void writeNvram_desktop(const char *configStr)
{
    FILE *fp;
    size_t count;
    UINT32 len;

    len = cmsUtl_strlen(configStr);
    if (len == 0)
    {
        cmsLog_error("configStr is NULL or 0 len");
        return;
    }

    fp = fopen(WIFI_DESKTOP_NVRAM, "a+");
    if (fp == NULL)
    {
        cmsLog_error("open of %s failed", WIFI_DESKTOP_NVRAM);
        return;
    }

    count = fwrite(configStr, len, 1, fp);
    if (count != (size_t) 1)
    {
        cmsLog_error("fwrite error, got %d expected %d", (int)count, 1);
    }

    /* for desktop only: write a newline (real nvram does not need it?) */
    count = fwrite("\n", 1, 1, fp);
    if (count != 1)
    {
        cmsLog_error("fwrite of newline failed!");
    }

    fclose(fp);
}
#endif

static const char* SB_40M_TABLE[] = 
{
MDMVS_BELOWCONTROLCHANNEL,
MDMVS_ABOVECONTROLCHANNEL,
};

static const char* SB_80M_TABLE[] = 
{
MDMVS_X_BROADCOM_COM_ABOVECTRLCHLL,
MDMVS_X_BROADCOM_COM_ABOVECTRLCHLU,
MDMVS_X_BROADCOM_COM_BELOWCTRLCHUL,
MDMVS_X_BROADCOM_COM_BELOWCTRLCHUU,
};

static const char* SB_160M_TABLE[] = 
{
MDMVS_X_BROADCOM_COM_ABOVECTRLCHLLL,
MDMVS_X_BROADCOM_COM_ABOVECTRLCHLLU,
MDMVS_X_BROADCOM_COM_ABOVECTRLCHLUL,
MDMVS_X_BROADCOM_COM_ABOVECTRLCHLUU,
MDMVS_X_BROADCOM_COM_BELOWCTRLCHULL,
MDMVS_X_BROADCOM_COM_BELOWCTRLCHULU,
MDMVS_X_BROADCOM_COM_BELOWCTRLCHUUL,
MDMVS_X_BROADCOM_COM_BELOWCTRLCHUUU,
};

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

CmsRet rutWifi_channelToSideband(unsigned int channel, unsigned int bw, unsigned int band, char* sb_buf)
{
    unsigned int begin_ch, max_ch, ch_interval;
    unsigned int sb_idx = 0, found = 0;

    if (bw == 20 || bw == 0)  // auto bandwidth or 20MHz, ignore it
       return CMSRET_INVALID_PARAM_VALUE;

    if (band == 2)
    {
        if (bw == 40)
        {
            if ( channel < 3 || channel > 13) /* channel < 3 or > 13*/
                return CMSRET_INVALID_PARAM_VALUE;

            if (channel < 10 && strcmp(sb_buf, MDMVS_BELOWCONTROLCHANNEL) == 0) /* channel:1~9, lower */
               return CMSRET_SUCCESS;
            else if (channel > 4 && strcmp(sb_buf, MDMVS_ABOVECONTROLCHANNEL) == 0) /* channel:5~13, upper */
               return CMSRET_SUCCESS;
            else if (channel < 10)
               sprintf(sb_buf, "%s", MDMVS_BELOWCONTROLCHANNEL);
            else
               sprintf(sb_buf, "%s", MDMVS_ABOVECONTROLCHANNEL);

            return CMSRET_SUCCESS;
        }

        if (channel < CH_MIN_2G_CHANNEL || channel > CH_MAX_2G_CHANNEL)
           return CMSRET_INVALID_PARAM_VALUE;

        begin_ch = CH_MIN_2G_CHANNEL; // 1
        max_ch = CH_MAX_2G_CHANNEL;   // 14
    }
    else if (band == 5)
    {
        if (channel < CH_MIN_5G_CHANNEL || channel > CH_MAX_5G_CHANNEL)
           return CMSRET_INVALID_PARAM_VALUE;
        begin_ch = CH_MIN_5G_CHANNEL; // 36
        max_ch = CH_MAX_5G_CHANNEL;   // 181
    }
    else if (band == 6)
    {
        if (channel < CH_MIN_6G_CHANNEL || channel > CH_MAX_6G_CHANNEL)
            return CMSRET_INTERNAL_ERROR;

        begin_ch = CH_MIN_6G_CHANNEL; // 1
        max_ch = CH_MAX_6G_CHANNEL;   // 233
    }
    else 
       return CMSRET_INVALID_PARAM_VALUE;

    ch_interval = (bw == 160 ? CH_160MHZ_APART :
                  (bw == 80 ? CH_80MHZ_APART : CH_40MHZ_APART));

    while (begin_ch < max_ch)
    {
       if ((channel - begin_ch) < ch_interval) // found 
       {
           sb_idx = (channel - begin_ch) / CH_20MHZ_APART;
           found = 1;
           break;
       }
       else
           begin_ch += ch_interval;
    }

    if (!found)
       return CMSRET_INVALID_PARAM_VALUE;

    if (bw == 40 && sb_idx < ARRAYSIZE(SB_40M_TABLE))
       sprintf(sb_buf, "%s", SB_40M_TABLE[sb_idx]);
    else if (bw == 80 && sb_idx <  ARRAYSIZE(SB_80M_TABLE))
       sprintf(sb_buf, "%s", SB_80M_TABLE[sb_idx]);
    else if (bw == 160 && sb_idx <  ARRAYSIZE(SB_160M_TABLE))
       sprintf(sb_buf, "%s", SB_160M_TABLE[sb_idx]); 

    return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

void rutUtil_modifyNumWifiSsid(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2WifiObject *wifiObj = NULL;

   if (mdmShmCtx->inMdmInit)
   {
      /*
       * During system startup, we might have loaded from a config file.
       * The config file already contains the correct count of these objects.
       * So don't update the count in the parent object.
       */
      cmsLog_debug("don't update count in MDMOID_DEV2_WIFI for new SSID object (delta=%d)", delta);
      return;
   }

   if ((cmsObj_get(MDMOID_DEV2_WIFI, &iidStack, 0, (void *) &wifiObj)) == CMSRET_SUCCESS)
   {
#ifdef DMP_DEVICE2_WIFISSID_1
      wifiObj->SSIDNumberOfEntries += delta;
#endif
      cmsObj_setFlags(wifiObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &wifiObj);
   }
}

void rutUtil_modifyNumWifiAccessPoint(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2WifiObject *wifiObj = NULL;

   if (mdmShmCtx->inMdmInit)
   {
      /*
       * During system startup, we might have loaded from a config file.
       * The config file already contains the correct count of these objects.
       * So don't update the count in the parent object.
       */
      cmsLog_debug("don't update count in MDMOID_DEV2_WIFI for new AccessPoint object (delta=%d)", delta);
      return;
   }

   if ((cmsObj_get(MDMOID_DEV2_WIFI, &iidStack, 0, (void *) &wifiObj)) == CMSRET_SUCCESS)
   {
#ifdef DMP_DEVICE2_WIFISSID_1
      wifiObj->accessPointNumberOfEntries += delta;
#endif
      cmsObj_setFlags(wifiObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &wifiObj);
   }
}

void rutUtil_modifyNumWifiAssociatedDevice(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2WifiAccessPointObject *apObj = NULL;
   CmsRet ret;

   if (mdmShmCtx->inMdmInit)
      return;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_WIFI_ACCESS_POINT, MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &apObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_WIFI_ACCESS_POINT, MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   apObj->associatedDeviceNumberOfEntries += delta;
   cmsObj_setFlags(apObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&apObj);
}

#endif  /* DMP_DEVICE2_BASELINE_1 */

