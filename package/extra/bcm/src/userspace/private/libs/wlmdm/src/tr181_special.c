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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "cms.h"
#include "mdm.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "rut2_unfwifi.h"
#include "sysutil_net.h"
#include "cms_util.h"
#include "cms_lck.h"
#include "cms_phl.h"
#include "cms_obj.h"
#include "wlsysutil.h"
#include "os_defs.h"
#include "special.h"
#include "nvc.h"
#include "nvn.h"
#include "conv.h"
#include "chanspec.h"
#include "cms_helper.h"
#include "wifi_constants.h"

#ifndef BUILD_RDKWIFI
#ifdef BRCM_BDK_BUILD
#include "bcm_zbus_intf.h"
#endif
#endif

#define MAX_PREFIX_LEN        64
#define MAX_PREFIX_ENTR       16

static WlmdmRet foreach_empty(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_lan_wps_oob(const char *nvname, const char *value);
static WlmdmRet get_lan_wps_oob(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_lan_wps_oob(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_chanspec(const char *nvname, const char *value);
static WlmdmRet get_chanspec(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_chanspec(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_lan_ifname(const char *nvname, const char *value);
static WlmdmRet get_lan_ifname(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_lan_ifname(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_lan_ifnames(const char *nvname, const char *value);
static WlmdmRet get_lan_ifnames(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_lan_ifnames(nvc_for_each_func foreach_func, void *data);

static WlmdmRet _set_auto_channel(unsigned int radio_index, UBOOL8 enable);
static int _get_auto_channel(int radio_index);
static int _get_caps(int radio_index, char* capBuf, int size);
static WlmdmRet _retrieve_chanspec_from_mdm(unsigned int radio_index,
                                       struct chanspec_t *chanspec);

static WlmdmRet set_mbss_names(const char *nvname, const char *value);
static WlmdmRet get_mbss_names(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_mbss_names(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_mapbss(const char *nvname, const char *value);
static WlmdmRet get_mapbss(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_mapbss(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_nctrlsb(const char *nvname, const char *value);
static WlmdmRet get_nctrlsb(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_nctrlsb(nvc_for_each_func foreach_func, void *data);

static WlmdmRet get_wl_lan_ipv4addr(const char *nvname, char *value, size_t size);
static WlmdmRet set_wl_lan_ipv4addr(const char *nvname, const char *value);

static WlmdmRet get_ap_friendly_name(const char *nvname, char *value, size_t size);
static WlmdmRet set_ap_friendly_name(const char *nvname, const char *value);

static WlmdmRet get_ap_serial_number(const char *nvname, char *value, size_t size);
static WlmdmRet set_ap_serial_number(const char *nvname, const char *value);

static WlmdmRet get_ap_vendor(const char *nvname, char *value, size_t size);
static WlmdmRet set_ap_vendor(const char *nvname, const char *value);

static WlmdmRet set_mobappd_pairing(const char *nvname, const char *value);
static WlmdmRet get_mobappd_pairing(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_mobappd_pairing(nvc_for_each_func foreach_func, void *data);

extern UBOOL8 match_name(const char *pattern, const char *nvname);

ActionSet handle_lan_wps_oob =
{
    .set = set_lan_wps_oob,
    .get = get_lan_wps_oob,
    .foreach = foreach_lan_wps_oob
};

ActionSet handle_chanspec =
{
    .set = set_chanspec,
    .get = get_chanspec,
    .foreach = foreach_chanspec
};

ActionSet handle_lan_ifname =
{
    .set = set_lan_ifname,
    .get = get_lan_ifname,
    .foreach = foreach_lan_ifname
};

ActionSet handle_lan_ifnames =
{
    .set = set_lan_ifnames,
    .get = get_lan_ifnames,
    .foreach = foreach_lan_ifnames
};

ActionSet handle_mbss_names =
{
    .set = set_mbss_names,
    .get = get_mbss_names,
    .foreach = foreach_mbss_names
};

ActionSet handle_mapbss =
{
    .set = set_mapbss,
    .get = get_mapbss,
    .foreach = foreach_mapbss
};

ActionSet handle_mapbss_noeach =
{
    .set = set_mapbss,
    .get = get_mapbss,
    .foreach = foreach_empty // Handled by handle_mapbss-->foreach_mapbss()
};

ActionSet handle_nctrlsb =
{
    .set = set_nctrlsb,
    .get = get_nctrlsb,
    .foreach = foreach_nctrlsb
};

ActionSet handle_wl_lan_ipv4addr =
{
    .set = set_wl_lan_ipv4addr,
    .get = get_wl_lan_ipv4addr,
    .foreach = foreach_empty
};

ActionSet handle_ap_friendly_name =
{
    .set = set_ap_friendly_name,
    .get = get_ap_friendly_name,
    .foreach = foreach_empty
};

ActionSet handle_ap_serial_number =
{
    .set = set_ap_serial_number,
    .get = get_ap_serial_number,
    .foreach = foreach_empty
};

ActionSet handle_ap_vendor =
{
    .set = set_ap_vendor,
    .get = get_ap_vendor,
    .foreach = foreach_empty
};

ActionSet handle_mobappd_pairing =
{
    .set = set_mobappd_pairing,
    .get = get_mobappd_pairing,
    .foreach = foreach_mobappd_pairing
};

SpecialHandler special_handler_table[] =
{
    { "^wl(\\d+)_chanspec$", &handle_chanspec },
#if !defined(RDK_BUILD) //TBD: don't handle these two items
    { "^lan(\\d+)?_wps_oob$", &handle_lan_wps_oob },
    { "^lan(\\d+)?_ifname$", &handle_lan_ifname },
    { "^lan(\\d+)?_ifnames$", &handle_lan_ifnames },
#endif
    { "^map_bss_names$", &handle_mbss_names },
    { "^wl(\\d+)_nctrlsb$", &handle_nctrlsb },
    { "^[^_]+_(band_flag|ssid|akm|crypto|wpa_psk)$", &handle_mapbss},
    { "^(?!wl(\\d+).*)[^_]+_map$", &handle_mapbss_noeach},
    { "^wl(\\d+(\\.\\d+)?)?_lan_ipv4addr$", &handle_wl_lan_ipv4addr },
    { "^ap_friendly_name$", &handle_ap_friendly_name },
    { "^ap_serial_number$", &handle_ap_serial_number },
    { "^ap_vendor$", &handle_ap_vendor },
    { "^mobappd_pairing_(\\d+)_([a-z]+)(_[a-z]+)?$", &handle_mobappd_pairing },
};

const unsigned int SPECIAL_TABLE_SIZE = sizeof(special_handler_table) / sizeof(SpecialHandler);

/*Parsing prefix string and put in string array res
* Params:
*  @prefixStr: INPUT. prefixes splitted by " ". exp, "fh bh"
*  @res: INPUT. pointer of an string array.
* return: number of entries parsed. -1: failed
*/
static int get_prefixList(const char *prefixStr, char res[][MAX_PREFIX_LEN])
{
   char *dup, *p, *t;
   char *next = NULL;
   unsigned int i = 0;

   if(!prefixStr || !res)
      return -1;

   dup = strdup(prefixStr);
   if(NULL == dup)
      return -1;

   p = strtok_r(dup, " ", &next);
   while(NULL != p)
   {
      t=p;
      while(*t == ' ') t++;
      snprintf(res[i++], MAX_PREFIX_LEN, "%s", t);
      if(i >= MAX_PREFIX_ENTR)
      {
         i = -1;
         break;
      }
      p = strtok_r(NULL, " ", &next);
   }

   free(dup);
   return i;
}

#define MAX_NUM_INTF 64
#define INTFNAME_LEN 16

static int compare(const void *a, const void *b)
{
    char const * str1 = (char const *) a;
    char const * str2 = (char const *) b;
    return strcmp(str1,str2);
}


#ifdef BRCM_BDK_BUILD
#include "cms_mdm.h"
#include "bcm_fsutils.h"
#include <sys/syscall.h>

static void getBridgeChildrenInterfaces(const char* bridge, char* buf, int size)
{
    CmsEntityId eid;
    char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubInterfaceMsgBody)]={0};
    CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
    PubSubInterfaceMsgBody *intfBody = (PubSubInterfaceMsgBody *) (msgHdr+1);
    CmsMsgHeader *msgResp=NULL;
    void *msgHandle = NULL;
    CmsRet ret;

    if (bridge == NULL)
    {
        cmsLog_error("bridge is NULL!");
        return;
    }
    if (bcmUtl_isShutdownInProgress())
    {
        cmsLog_notice("system is shutting down!");
        return;
    }

    // Fill in common parts of the msg
    msgHandle = cmsMdm_getThreadMsgHandle();
    if (msgHandle == NULL)
    {
        cmsLog_error("Failed to get msgHandle from mdmLibCtx!");
        return;
    }

    eid = cmsMsg_getHandleGenericEid(msgHandle);
    msgHdr->src = cmsMsg_getHandleEid(msgHandle);  // this returns the specificEid
    msgHdr->dst = EID_WIFI_MD;
    msgHdr->type = CMS_MSG_GET_EVENT_STATUS;
    msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
    msgHdr->flags_request = 1;
    msgHdr->dataLength = sizeof(PubSubInterfaceMsgBody);

    snprintf(intfBody->intfName, sizeof(intfBody->intfName), "%s", bridge);

    cmsLog_debug("Sending msgType 0x%x dataLength %d intfName %s",
                 msgHdr->type, msgHdr->dataLength, intfBody->intfName);

#ifndef BUILD_RDKWIFI
    if (eid == EID_WIFI_MD)
    {
        //send request to dbus directly
        ret = zbus_out_getEventStatus(msgHdr, &msgResp);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to getEventStatus, ret=%d", ret);
            return; 
        }
    }
    else
#endif
    {
        //Ask wifi_md to act as a proxy to dbus request
        ret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle, msgHdr, &msgResp, 9000);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("get reply message failed. (ret=%d)\n", ret);
            return;
        }
    }

    if (msgResp)
    {
        if (msgResp->wordData == PUBSUB_EVENT_INTERFACE)
        {
            intfBody = (PubSubInterfaceMsgBody *) (msgResp+1);
            cmsLog_debug("GET_EVENT_STATUS response, dataLenght=%d, intfName=%s, child:%s", 
                         msgResp->dataLength, intfBody->intfName, intfBody->childrenIntfNames);
            strncpy(buf, intfBody->childrenIntfNames, size);
        }
        else
        {
            /* we might get error, for example: the record of an interface not found in sysdir */
            cmsLog_debug("get interface(%s) event status failed (ret=%d)\n", bridge, msgResp->wordData);
        }
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);

    {
        char intfArray[32][11] = {0};
        char *str = NULL, *token = NULL;
        char *saveptr, *ptr;
        int i, j;

        for (i = 0, str = buf; i < 32 ; i++, str = NULL)
        {
            token = strtok_r(str, ",", &saveptr);
            if (token == NULL)
                break;
            strncpy(intfArray[i], token, 10);
        }
        qsort((void*)&intfArray, i, sizeof(intfArray[0]), compare);

        for (j = 0, ptr = buf ; j < i ; j++)
        {
            if (j == 0)
                ptr += sprintf(ptr, "%s", intfArray[j]);
            else
                ptr += sprintf(ptr, " %s", intfArray[j]);
        }
    }
    return;
}


static int getBridgeNameFromIndex(int bridgeIndex, char *buf, size_t size)
{
    CmsEntityId eid;
    char msgBuf[sizeof(CmsMsgHeader) + BUFLEN_128] = {0};
    CmsMsgHeader *msgHdr = (CmsMsgHeader *)msgBuf;
    void *msgHandle = NULL;
    char *queryString = (char *)(msgHdr + 1);
    CmsRet cret;
    int ret = 0;

    CmsMsgHeader *msgResp = NULL;

    assert(bridgeIndex >= 0);

    if (bcmUtl_isShutdownInProgress())
    {
        cmsLog_notice("system is shutting down!");
        return -1;
    }

    // Fill in common parts of the msg
    msgHandle = cmsMdm_getThreadMsgHandle();
    if (msgHandle == NULL)
    {
        cmsLog_error("Failed to get msgHandle from mdmLibCtx!");
        return -1;
    }

    eid = cmsMsg_getHandleGenericEid(msgHandle);
    msgHdr->src = cmsMsg_getHandleEid(msgHandle);  // this returns the specificEid
    msgHdr->dst = EID_WIFI_MD;
    msgHdr->type = CMS_MSG_QUERY_EVENT_STATUS;
    msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
    msgHdr->flags_request = 1;

    snprintf(queryString, BUFLEN_128, "bridgeIndex=%d", bridgeIndex);

    msgHdr->dataLength = strlen(queryString) + 1;

    cmsLog_debug("Sending msgType 0x%x dataLength %d queryString=%s",
                 msgHdr->type, msgHdr->dataLength, queryString);

#ifndef BUILD_RDKWIFI
    if (eid == EID_WIFI_MD)
    {
        //send request to dbus directly
        ret = zbus_out_queryEventStatus(0, msgHdr, &msgResp);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to queryEvent, ret=%d", ret);
            return -1; 
        }
    }
    else
#endif
    {
        //Ask wifi_md to act as a proxy to dbus request
        cret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle, msgHdr, &msgResp, 9000);
        if (cret != CMSRET_SUCCESS)
        {
            cmsLog_error("get reply message failed. (ret=%d)\n", cret);
            return -1;
        }
    }

    if (msgResp)
    {
        if (msgResp->wordData == PUBSUB_EVENT_INTERFACE)
        {
            char *respString;
            respString = (char *)(msgResp + 1);
            cmsLog_debug("QUERY_EVENT_STATUS response, dataLenght=%d, respString=%s", 
                         msgResp->dataLength, respString);
            strncpy(buf, respString, size-1);
        }
        else
        {
            /* we might get error, for example: the record of an interface not found in sysdir */
            cmsLog_error("query interface at bridgeIndex %d event status failed (ret=%d)\n", bridgeIndex, msgResp->wordData);
            ret = -1;
        }
    }
    CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);

    return ret;
}

static int getInterfaceIPv4Addr(const char *interface, char *buf, size_t size)
{
    char msgBuf[sizeof(CmsMsgHeader) + sizeof(PubSubInterfaceMsgBody)] = {0};
    CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
    PubSubInterfaceMsgBody *intfBody = (PubSubInterfaceMsgBody *)(msgHdr+1);
    CmsMsgHeader *msgResp = NULL;
    void *msgHandle = NULL;
    CmsRet cret;
    int ret = 0;

    if (interface == NULL)
    {
        cmsLog_error("interface is NULL!");
        return -1;
    }

    // Fill in common parts of the msg
    msgHandle = cmsMdm_getThreadMsgHandle();
    if (msgHandle == NULL)
    {
        cmsLog_error("Failed to get msgHandle from mdmLibCtx!");
        return -1;
    }

    msgHdr->src = cmsMsg_getHandleEid(msgHandle);  // this returns the specificEid
    msgHdr->dst = EID_WIFI_MD;
    msgHdr->type = CMS_MSG_GET_EVENT_STATUS;
    msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
    msgHdr->flags_request = 1;
    msgHdr->dataLength = sizeof(PubSubInterfaceMsgBody);

    snprintf(intfBody->intfName, sizeof(intfBody->intfName), "%s", interface);

    cmsLog_debug("Sending msgType 0x%x dataLength %d intfName %s",
                 msgHdr->type, msgHdr->dataLength, intfBody->intfName);

    memset(buf, 0x0, size);

    //Ask wifi_md to act as a proxy to dbus request
    cret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle, msgHdr, &msgResp, 9000);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("get reply message failed. (ret=%d)\n", cret);
        return -1;
    }

    if (msgResp->wordData == PUBSUB_EVENT_INTERFACE)
    {
        intfBody = (PubSubInterfaceMsgBody *) (msgResp + 1);
        cmsLog_debug("GET_EVENT_STATUS response, dataLength=%d, intfName=%s, ipv4Addr:%s", 
                     msgResp->dataLength, intfBody->intfName, intfBody->ipv4Addr);
        strncpy(buf, intfBody->ipv4Addr, size - 1);
    }
    else
    {
        /* we might get error, for example: the record of an interface not found in sysdir */
        cmsLog_error("get interface(%s) event status failed (ret=%d)\n", interface, msgResp->wordData);
        ret = -1;
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);

    return ret;
}

#else

#include "rut2_bridging.h"

static void getBridgeChildrenInterfaces(const char* bridge, char *buf, int size)
{

   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack sub_iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeObject *brObj = NULL;
   Dev2BridgePortObject *brPortObj = NULL;
   UBOOL8 found = FALSE;
   int i = 0, j = 0;
   char childIntfs[MAX_NUM_INTF][INTFNAME_LEN] = {0};

   if (IS_EMPTY_STRING(bridge))
   {
       return;
   }

   cmsLog_notice("Entered: BridgeName=%s", bridge);

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
       return;

   // See if the wifi interface is already under the requested bridge.
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &brObj) == CMSRET_SUCCESS)
   {
      if (0 == cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, bridge))
      {
         // Found the right bridge, now check the ports
         found = TRUE;
         INIT_INSTANCE_ID_STACK(&sub_iidStack);
         while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                                        &sub_iidStack,
                                        (void **)&brPortObj) == CMSRET_SUCCESS)
         {
            if (!brPortObj->managementPort)
            {
                strncpy(childIntfs[i++], brPortObj->name, INTFNAME_LEN-1);
            }
            cmsObj_free((void **)&brPortObj);
         }
      }
      cmsObj_free((void **)&brObj);
   }

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
   if (rutOpenVS_isEnabled() && !cmsUtl_strcmp(SDN_OVS_BR_NAME, bridge))
   {
      InstanceIdStack OvsStk = EMPTY_INSTANCE_ID_STACK;
      OpenvswitchCfgObject *OvsObj = NULL;
      char ovsIfList[BUFLEN_256] = {0};
      CmsRet ret = CMSRET_SUCCESS;
      char *ptr = NULL;
      char *savePtr = NULL;
   
      i = 0;
      if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &OvsStk, 0, (void **) &OvsObj)) != CMSRET_SUCCESS)
      {
          cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d", ret);
      }
      else
      {
          snprintf(ovsIfList, sizeof(ovsIfList), "%s", OvsObj->ifNameList);
          cmsObj_free((void **) &OvsObj);
   
          ptr = strtok_r(ovsIfList, ",", &savePtr);
          if (!ptr)
              snprintf((char *)childIntfs[i], INTFNAME_LEN, ovsIfList);
   
          while (ptr)
          {
             snprintf((char *)childIntfs[i++], INTFNAME_LEN, ptr);          
             ptr = strtok_r(NULL, ",", &savePtr);
          }   
      }
   }
#endif    
   
   cmsLck_releaseLock();

   qsort((void*)&childIntfs, i, INTFNAME_LEN, compare);

   char *ptr = buf;
   for (j = 0; j < i ; j++)
   {
       if (j ==0)
           ptr += sprintf(ptr, "%s", childIntfs[j]);
       else
           ptr += sprintf(ptr, " %s", childIntfs[j]);
   }
}

static int getBridgeNameFromIndex(int bridgeIndex, char *buf, size_t size)
{
   CmsRet ret;

   cmsLog_notice("Entered: bridgeIndex=%d", bridgeIndex);

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
   {
       return -1;
   }

   ret = rutBridge_getBridgeNameByIndex_dev2(bridgeIndex, buf, size);

   cmsLck_releaseLock();
   return (ret == CMSRET_SUCCESS) ? 0 : -1;
}

static int getInterfaceIPv4Addr(const char *interface, char *buf, size_t size)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2IpInterfaceObject *intfObj = NULL;
    Dev2Ipv4AddressObject *ipv4Obj = NULL;
    UBOOL8 found = FALSE;
    CmsRet cret;

    if (interface == NULL)
    {
        cmsLog_error("Invalid input!");
        return -1;
    }

    cmsLog_notice("Entered: interface=%s", interface);
    memset(buf, 0x0, size);

    // See if the wifi interface is already under the requested bridge.
    while (!found &&
           cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **) &intfObj) == CMSRET_SUCCESS)
    {
       if (0 == strcmp(intfObj->name, interface))
       {
          // Found the right bridge, now check the ports
          found = TRUE;
          PUSH_INSTANCE_ID(&iidStack, 1);
          cret = cmsObj_get(MDMOID_DEV2_IPV4_ADDRESS, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **) &ipv4Obj);
          if (cret == CMSRET_SUCCESS)
          {
              strncpy(buf, ipv4Obj->IPAddress, size - 1);
              cmsObj_free((void **)&ipv4Obj);
          }
       }
       cmsObj_free((void **)&intfObj);
    }
    return found ? 0 : -1;
}

#endif /* BRCM_BDK_BUILD */

int Notify_WL_Intf_EventMsg(const char *status, const Dev2WifiSsidObject *ssidObj)
{
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubInterfaceMsgBody)]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   PubSubInterfaceMsgBody *msgBody = (PubSubInterfaceMsgBody *) (msgHdr+1);
   void *msgHandle = NULL;
   CmsRet cret;

   cmsLog_debug("Enter");

   if (bcmUtl_isShutdownInProgress())
   {
      cmsLog_notice("system down");
      return -1;
   }

    //Fill in data specific to this interface
    if(ssidObj)
    {
      cmsLog_debug("wireless interface name=%s wlBrIndex=%d",
                   ssidObj->name, ssidObj->X_BROADCOM_COM_WlBrIndex);

      snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", ssidObj->name);
      msgBody->wlBrIndex = ssidObj->X_BROADCOM_COM_WlBrIndex;
      msgBody->isUpstream = 0;  // wifi could be used as upstream?
    }
    else
    {
       cmsLog_error("ssidObj is NULL");
       return -1;
    }

    msgHandle = cmsMdm_getThreadMsgHandle();
    if (msgHandle == NULL)
    {
        cmsLog_error("Failed to get msgHandle from mdmLibCtx!");
        return -1;
    }

    // Fill in common parts of the msg
    msgHdr->src = cmsMsg_getHandleEid(msgHandle);  // this returns the specificEid
    // In BDK, if this code is not running in sysmgmt, it will get forwarded to
    // the local bus manager.
    // In CMS Classic, smd will intercept this message.
    msgHdr->dst = EID_SYSMGMT_MD;
    msgHdr->type = CMS_MSG_PUBLISH_EVENT;
    msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
    msgHdr->flags_event = 1;
    msgHdr->dataLength = sizeof(PubSubInterfaceMsgBody);

    msgBody->isLayer2 = 1;
    msgBody->isLinkUp = strcmp(status, MDMVS_UP) ? 0 : 1;
#ifdef BRCM_BDK_BUILD
    snprintf(msgBody->pubCompName, sizeof(msgBody->pubCompName), "%s", BDK_COMP_WIFI);
#else
    snprintf(msgBody->pubCompName, sizeof(msgBody->pubCompName), "%s", BDK_COMP_CMS_CLASSIC);
#endif

    if(ssidObj->lowerLayers)
    {
        snprintf(msgBody->fullpath, sizeof(msgBody->fullpath), "%s", ssidObj->lowerLayers);
    }

    cret = cmsMsg_send(msgHandle, msgHdr);
    if (cret != CMSRET_SUCCESS)
    {
       cmsLog_error("failed to send L2 interface event msg, ret=%d", cret);
    }

    cmsLog_debug("msg sent!");

    return 0;
}

static WlmdmRet set_lan_wps_oob(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    int i, num_ssid, bi;
    char buf[16] = {0};
    long int b;
    MdmPathDescriptor path_ssid;
    MdmPathDescriptor path_wps;

    i = sscanf(nvname, "lan%d_wps_oob", &bi);
    if (i == 0)
    {
        bi = 0;
    }

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);
    for (i = 0; i < num_ssid; i++)
    {
        INIT_PATH_DESCRIPTOR(&path_ssid);
        path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
        PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
        strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrIndex", sizeof(path_ssid.paramName));

        ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
        if (ret != 0)
        {
            cmsLog_notice("Failed to get Device.WiFi.SSID.%d.X_BROADCOM_COM_WlBrIndex!", i+1);
            continue;
        }

        b = strtol((char *)&buf, NULL, 10);
        if (b < 0 || b == LONG_MAX)
        {
            cmsLog_error("Failed to convert %s to proper long int value!", buf);
            continue;
        }

        if ((int)b == bi)
        {
            if (!strcmp(value, "enabled"))
            {
                strcpy((char *)&buf, "0");
            }
            else
            {
                strcpy((char *)&buf, "1");
            }
            INIT_PATH_DESCRIPTOR(&path_wps);
            path_wps.oid = MDMOID_DEV2_WIFI_ACCESS_POINT_WPS;
            PUSH_INSTANCE_ID(&path_wps.iidStack, i + 1);
            strncpy((char *)&path_wps.paramName, "X_BROADCOM_COM_Wsc_config_state", sizeof(path_wps.paramName));

            ret = set_param_from_pathDesc(&path_wps, (char *)&buf);
            if (ret != WLMDM_OK)
            {
                break;
            }
        }
    }
    return ret;
}

static WlmdmRet get_lan_wps_oob(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    int i, num_ssid, bi;
    long int b;
    char buf[16] = {0};
    MdmPathDescriptor path_ssid;
    MdmPathDescriptor path_wps;

    i = sscanf(nvname, "lan%d_wps_oob", &bi);
    if (i == 0)
    {
        bi = 0;
    }

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);
    for (i = 0; i < num_ssid; i++)
    {
        INIT_PATH_DESCRIPTOR(&path_ssid);
        path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
        PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
        strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrIndex", sizeof(path_ssid.paramName));

        ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
        if (ret != 0)
        {
            continue;
        }

        b = strtol((char *)&buf, NULL, 10);
        if ((int)b == bi)
        {
            INIT_PATH_DESCRIPTOR(&path_wps);
            path_wps.oid = MDMOID_DEV2_WIFI_ACCESS_POINT_WPS;
            PUSH_INSTANCE_ID(&path_wps.iidStack, i + 1);
            strncpy((char *)&path_wps.paramName, "X_BROADCOM_COM_Wsc_config_state", sizeof(path_wps.paramName));

            ret = get_param_from_pathDesc(&path_wps, (char *)&buf, sizeof(buf));
            if (ret == WLMDM_OK)
            {
                if (!strcmp((char *)&buf, "0"))
                {
                    strncpy(value, "enabled", size - 1);
                }
                else
                {
                    strncpy(value, "disabled", size - 1);
                }
            }
            return ret;
        }
    }
    return WLMDM_GENERIC_ERROR;
}

static WlmdmRet foreach_lan_wps_oob(nvc_for_each_func foreach_func, void *data)
{
    WlmdmRet ret = 0;
    int bi, i, num_ssid, index;
    long int b;
    char buf[16] = {0};
    char br_name[SYSUTL_IFNAME_LEN] = {0};
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    MdmPathDescriptor path_ssid;
    MdmPathDescriptor path_wps;

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);

    for (bi = 0; bi < MAX_WIFI_BRIDGES; bi++)
    {
        if (0 != getBridgeNameFromIndex(bi, (char *)&br_name, sizeof(br_name)))
        {
            continue;
        }

        index = sysUtl_getIfindexByIfname(br_name);
        if (index < 0)
        {
            cmsLog_notice("interface %s is not present in the system!", br_name);
            continue;
        }

        // This is a valid bridge.
        if (bi == 0)
        {
            snprintf(nvname, sizeof(nvname), "lan_wps_oob");
        }
        else
        {
            snprintf(nvname, sizeof(nvname), "lan%d_wps_oob", bi);
        }

        // Retrieve the value for the current lan%d_wps_oob
        for (i = 0; i < num_ssid; i++)
        {
            INIT_PATH_DESCRIPTOR(&path_ssid);
            path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
            PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
            strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrIndex", sizeof(path_ssid.paramName));

            ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
            if (ret != WLMDM_OK)
            {
                continue;
            }
            
            b = strtol((char *)&buf, NULL, 10);
            if ((int)b == bi)
            {
                break;
            }
        }

        if (i < num_ssid)
        {
            INIT_PATH_DESCRIPTOR(&path_wps);
            path_wps.oid = MDMOID_DEV2_WIFI_ACCESS_POINT_WPS;
            PUSH_INSTANCE_ID(&path_wps.iidStack, i + 1);
            strncpy((char *)&path_wps.paramName, "X_BROADCOM_COM_Wsc_config_state", sizeof(path_wps.paramName));

            ret = get_param_from_pathDesc(&path_wps, (char *)&buf, sizeof(buf));
            if (ret == WLMDM_OK)
            {
                if (!strcmp((char *)&buf, "0"))
                {
                    strncpy(value, "enabled", sizeof(value) - 1);
                }
                else
                {
                    strncpy(value, "disabled", sizeof(value) - 1);
                }
            }
        }
        // Do the callback
        foreach_func((char *)&nvname, (char *)&value, data);
    }

    return ret;
}

static WlmdmRet set_chanspec(const char *nvname, const char *value)
{
    WlmdmRet ret;
    struct chanspec_t chanspec = {0};
    unsigned int band = 0, nbw = 0, channel = 0;
    int radio_index = -1, bssid_index = -1;
    char buf[32], buf_1[32];
    const char *t;
    int set_sb = 1;
    int seg0 = 0, seg1 = 0;
    int bandscheme = 0;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = chanspec_parse_string(value, &chanspec);
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse chanspec from %s", value);
        return ret;
    }

    channel = chanspec.channel;
    band = chanspec.band;

    if (channel != 0)
    {
        nbw = chanspec.band_width;
        switch(chanspec.band_width)
        {
            case WL_CHANSPEC_BW_320:
                /* Calculate the lowest channel number */
                channel = channel - CH_160MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                bandscheme = chanspec.bandscheme;
                break;

            case WL_CHANSPEC_BW_160:
                /* Calculate the lowest channel number */
                channel = channel - CH_80MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                break;

            case WL_CHANSPEC_BW_8080:
                seg0 = chanspec_get80Mhz_ch(chanspec.chan1_index);
                seg1 = chanspec_get80Mhz_ch(chanspec.chan2_index);
                break;

            case WL_CHANSPEC_BW_80:
                /*  Adjust the lowest channel from center channel */
                channel = channel - CH_40MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                break;

            case WL_CHANSPEC_BW_40:
                channel = channel - CH_20MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                break;

            case WL_CHANSPEC_BW_20:
                set_sb = 0;
                break;
        }

        if (set_sb)
        {
            t = chanspec_get_sb_string(chanspec.ctlsb_index, chanspec.band_width);
            if (t == NULL)
            {
                cmsLog_error("Failed to retrieve proper side band index!");
                return -1;
            }

            ret = nvn_gen("nctrlsb", radio_index, bssid_index, (char *)&buf, sizeof(buf));
            if (ret == WLMDM_OK)
            {
                cmsLog_debug("conv_set_mapped %s=%s", buf, t);
                conv_set((char *)&buf, t);
            }
        }

        ret = nvn_gen("nband", radio_index, bssid_index, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            switch (band)
            {
            case WL_CHANSPEC_BAND_2G:
                conv_set_mapped((char *)&buf, "2");
                break;
            case WL_CHANSPEC_BAND_5G:
                conv_set_mapped((char *)&buf, "1");
                break;
            case WL_CHANSPEC_BAND_6G:
                conv_set_mapped((char *)&buf, "4");
                break;
            default:
                cmsLog_error("unexpected band in chanspec: %d", band);
                break;
            }
        }

        ret = nvn_gen("bw", radio_index, bssid_index, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            sprintf(buf_1, "%d", nbw);
            conv_set_mapped((char *)&buf, (char *)&buf_1);
        }
        _set_auto_channel(radio_index, FALSE);
    }
    else
    {
        _set_auto_channel(radio_index, TRUE);
    }

    ret = nvn_gen("channel", radio_index, bssid_index, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        sprintf(buf_1, "%d", channel);
        cmsLog_debug("conv_set_mapped %s=%s", buf, buf_1);
        conv_set_mapped((char *)&buf, (char *)&buf_1);
    }

    // save seg0 and seg1 for 1st and 2nd 80MHz channels.
    if (nbw == WL_CHANSPEC_BW_8080) 
    {
        ret = nvn_gen("seg0", radio_index, bssid_index, (char*)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            sprintf(buf_1, "%d", seg0);
            conv_set_mapped((char *)&buf, (char *)&buf_1);
        }
        ret = nvn_gen("seg1", radio_index, bssid_index, (char*)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            sprintf(buf_1, "%d", seg1);
            conv_set_mapped((char *)&buf, (char *)&buf_1);
        }
    }

    // save band scheme for 320MHz
    if (nbw == WL_CHANSPEC_BW_320)
    {
       ret = nvn_gen("bandscheme", radio_index, bssid_index, (char*)&buf, sizeof(buf));
       if (ret == WLMDM_OK)
       {
          sprintf(buf_1, "%d", bandscheme);
          conv_set_unmapped((char *)&buf, (char *)&buf_1);
       }
    }

    return WLMDM_OK;
}

static WlmdmRet get_chanspec(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    struct chanspec_t chanspec = {0};
    int radio_index, bssid_index;
    char buf[32];
    int autoChannel = 0;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    autoChannel = _get_auto_channel(radio_index);
    if (autoChannel == -1)
    {
        cmsLog_error("Failed to get \"AutoChannelEnable\" from MDM");
        return ret;
    }
    else if(autoChannel == 1)
    {
        snprintf(value, size, "%d", 0);
        return 0;
    }

    ret = _retrieve_chanspec_from_mdm(radio_index, &chanspec);
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    ret = chanspec_to_string(&chanspec, value, size);

    return ret;
}

static WlmdmRet foreach_chanspec(nvc_for_each_func foreach_func, void *data)
{
    struct chanspec_t chanspec = {0};
    int i, num_radio;
    char nvname[MAX_NVRAM_NAME_SIZE], value[MAX_NVRAM_VALUE_SIZE];
    int autoChannel = 0;
    WlmdmRet ret;

    num_radio = get_num_instances(MDMOID_DEV2_WIFI_RADIO);
    for (i = 0; i < num_radio; i++)
    {
        autoChannel = _get_auto_channel(i);
        if (autoChannel == -1)
        {
            cmsLog_error("Failed to get \"AutoChannelEnable\" from MDM");
            continue;
        }
        else if(autoChannel == 1)
        {
            snprintf(value, sizeof(value), "%d", 0);
        }
        else
        {
            ret = _retrieve_chanspec_from_mdm(i, &chanspec);
            if (ret != WLMDM_OK)
            {
                continue;
            }

            ret = chanspec_to_string(&chanspec, (char *)&value, sizeof(value));
            if (ret != WLMDM_OK)
            {
                continue;
            }
        }
        ret = nvn_gen("chanspec", i, -1, (char *)&nvname, sizeof(nvname));
        if (ret == WLMDM_OK)
        {
            foreach_func(nvname, value, data);
        }
    }
    return WLMDM_OK;
}

static WlmdmRet _retrieve_chanspec_from_mdm(unsigned int radio_index,
                                       struct chanspec_t *chanspec)
{
    WlmdmRet ret;
    char buf[32], buf_1[32];

    assert(chanspec);

    ret = nvn_gen("nband", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            if (0 == strcmp((char *)buf_1, "1"))
            {
                chanspec->band = WL_CHANSPEC_BAND_5G;
            }
            else if (0 == strcmp((char *)buf_1, "2"))
            {
                chanspec->band = WL_CHANSPEC_BAND_2G;
            }
            else
            {
                chanspec->band = WL_CHANSPEC_BAND_6G;
            }
        }
    }

    ret = nvn_gen("bw", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            long int b;
            b = strtol((char *)&buf_1, NULL, 10);
            if ((b > 0) && (b < LONG_MAX))
            {
                chanspec->band_width = b;
            }
            else if (b <= 0)
            {
                // Reference the logic from wlconf.c
                if (chanspec->band == WL_CHANSPEC_BAND_2G)
                {
                    chanspec->band_width = WL_CHANSPEC_BW_40;
                }
                else
                {
                    char capsBuf[1024] = {0};
                    int cap_160 = 0;

                    if (_get_caps(radio_index, capsBuf, sizeof(capsBuf)) == WLMDM_OK)
                    {
                        cap_160 = (strstr(capsBuf, "160") != NULL);
                    }
                    chanspec->band_width = cap_160 ? WL_CHANSPEC_BW_160 : WL_CHANSPEC_BW_80;
                }
            }
            else
            {
                cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
            }
        }
    }

    ret = nvn_gen("nctrlsb", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get((char *)&buf, (char *)buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            if (chanspec->band_width == WL_CHANSPEC_BW_40)
                ret = chanspec_get_40bw_sb_index((char *)buf_1);
            else
                ret = chanspec_get_sb_index((char *)buf_1);
            chanspec->ctlsb_index = ret;
        }
    }

    ret = nvn_gen("channel", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            long int b;
            unsigned int channel;

            b = strtol((char *)&buf_1, NULL, 10);
            if ((b < 0) || (b == LONG_MAX))
            {
                cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
            }
            else
            {
                channel = (unsigned int) b;
                switch (chanspec->band_width)
                {
                    case WL_CHANSPEC_BW_320:
                        /* Calculate the lowest channel number */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_160MHZ_APART - CH_10MHZ_APART;
                        break;


                    case WL_CHANSPEC_BW_160:
                        /* Calculate the lowest channel number */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_80MHZ_APART - CH_10MHZ_APART;
                        break;

                    case WL_CHANSPEC_BW_80:
                        /*  Adjust the lowest channel from center channel */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_40MHZ_APART - CH_10MHZ_APART;
                        break;

                    case WL_CHANSPEC_BW_40:
                        /*  Adjust the lowest channel from center channel */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_20MHZ_APART - CH_10MHZ_APART;
                        break;

                    default:
                        break;
                }
                chanspec->channel = channel;
            }
        }
    }

    // retrieve seg0 and seg1 and conver to index
    if (chanspec->band_width == WL_CHANSPEC_BW_8080 && chanspec->band == WL_CHANSPEC_BAND_5G)
    {
        int ch1=-1, ch2=-1;
        ret = nvn_gen("seg0", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                ch1 = strtol((char *)&buf_1, NULL, 10);
                if ((ch1 < 0) || (ch1 == LONG_MAX))
                {
                   cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
                }
            }
        }
        ret = nvn_gen("seg1", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                ch2 = strtol((char *)&buf_1, NULL, 10);
                if ((ch2 < 0) || (ch2 == LONG_MAX))
                {
                   cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
                }
            }
        }
        chanspec->chan1_index = chanspec_channel_80mhz_to_index(ch1);
        chanspec->chan2_index = chanspec_channel_80mhz_to_index(ch2);
    }

    if (chanspec->band_width == WL_CHANSPEC_BW_320 && chanspec->band == WL_CHANSPEC_BAND_6G)
    {
        int bandscheme = 0;
        ret = nvn_gen("bandscheme", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_unmapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                bandscheme = strtol((char *)&buf_1, NULL, 10);
                if ((bandscheme < 0) || (bandscheme == LONG_MAX))
                {
                   cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
                }
            }
        }
        chanspec->bandscheme = bandscheme;
    }

    return WLMDM_OK;
}

static WlmdmRet set_lan_ifname(const char *nvname __attribute__((unused)),
                               const char *value __attribute__((unused)))
{
    // set lan_ifname is not allowed
    return WLMDM_GENERIC_ERROR;
}


// Given lan_ifname or lanx_ifname, where x is a number, return the bridge
// name in the given value ptr.
static WlmdmRet get_lan_ifname(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    char br_name[SYSUTL_IFNAME_LEN] = {0};
    int bi = 0;
    int index;

    assert(value);
    memset(value, 0x00, size);

    // if nvname is lan_ifname, this sscanf will fail and bi will remain 0.
    sscanf(nvname, "lan%d_ifname", &bi);
    if (0 == getBridgeNameFromIndex(bi, (char *)&br_name, sizeof(br_name)))
    {
        // Do not check the data model because in BDK, the wifi component cannot
        // access the DEV2_BRIDGE object.  (no need to lock MDM anymore)
        // Check with kernel if the interface exists.
        index = sysUtl_getIfindexByIfname(br_name);
        if (index >= 0)
        {
            strncpy(value, br_name, size - 1);
            ret = WLMDM_OK;
        }
    }
    return ret;
}


// For each bridge in the system, call the foreach func with:
// nvname (eg lan_ifname, lan1_ifname), linux intf name (eg br0, br1), and
// context data.
static WlmdmRet foreach_lan_ifname(nvc_for_each_func foreach_func, void *data)
{
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    int bi, index;

    // Do not check the data model because in BDK, the wifi component cannot
    // access the DEV2_BRIDGE object.  (no need to lock MDM anymore)
    // Check with kernel if the interface exists.
    for (bi = 0; bi < MAX_WIFI_BRIDGES; bi++)
    {
        if (0 != getBridgeNameFromIndex(bi, (char *)&value, sizeof(value)))
        {
            continue;
        }

        index = sysUtl_getIfindexByIfname(value);
        if (index < 0)
            continue;

        // This is a valid bridge.
        if (bi == 0)
        {
            snprintf(nvname, sizeof(nvname), "lan_ifname");
        }
        else
        {
            snprintf(nvname, sizeof(nvname), "lan%d_ifname", bi);
        }

        // Do the callback
        foreach_func((char *)&nvname, (char *)&value, data);
    }

    return WLMDM_OK;
}


// Caller must pass in a dup'd string which can be used by strtok_r
// Caller is responsible for freeing the returned array.
static char **valueStringToIntfNameArray(char *dup)
{
    char *saveptr=NULL;
    unsigned int len, max, i=0;
    char **intfNameArray=NULL;

    len = strlen(dup);

    // max number of slots in array, assuming each intf name is only 3 bytes.
    // +1 for round up and +1 for last null slot.
    max = len / 3 + 2;
    intfNameArray = calloc(1, max * sizeof(char *));
    if (intfNameArray == NULL)
    {
        cmsLog_error("Failed to allocate %d slots for nameArray", max);
        return NULL;
    }

    intfNameArray[0] = strtok_r(dup, " ", &saveptr);
    i++;
    while ((intfNameArray[i] = strtok_r(NULL, " ", &saveptr)) != NULL)
    {
        i++;
    }

    return intfNameArray;
}

static int isInIntfNameArray(const char *name, const char **intfNameArray)
{
    unsigned int i=0;
    while (intfNameArray[i] != NULL)
    {
        if (!cmsUtl_strcmp(name, intfNameArray[i]))
            return 1;
        i++;
    }
    return 0;
}

//returns 0 if the wireless interfaces in array_a and array_b are "equal".
//Otherwise returns 1.
static int compareIntfNameArray(const char **array_a, const char **array_b)
{
    unsigned int i;

    for (i = 0; array_a[i] != NULL; i++)
    {
        //Filter out non wireless interfaces
        if (strncmp(array_a[i], "wl", 2) != 0)
            continue;

        if (0 == isInIntfNameArray(array_a[i], array_b))
            return 1;
    }

    for (i = 0; array_b[i] != NULL; i++)
    {
        //Filter out non wireless interfaces
        if (strncmp(array_b[i], "wl", 2) != 0)
            continue;

        if (0 == isInIntfNameArray(array_b[i], array_a))
            return 1;
    }

    return 0;
}

static WlmdmRet set_lan_ifnames(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    int bi = 0;
    int synced, counter;
    char dupValue[MAX_NVRAM_VALUE_SIZE] = {0};
    char childIntfNames[MAX_NVRAM_VALUE_SIZE] = {0};
    char **intfNameArray;
    char **currIntfNameArray;

    assert(value);

    // nvname is lan_ifname or lanx_ifname, where x is the bridge number
    // if nvname is lan_ifname, this sscanf will fail and bi will remain 0.
    sscanf(nvname, "lan%d_ifnames", &bi);

    cmsLog_notice("Entered: nvname:%s (bi=%d) value:%s", nvname, bi, value);

    get_lan_ifnames(nvname, childIntfNames, sizeof(childIntfNames));
    currIntfNameArray = valueStringToIntfNameArray(childIntfNames);
    if (currIntfNameArray == NULL)
    {
        return WLMDM_GENERIC_ERROR;
    }

    // Convert value from a space separated list of wifi intf names to an
    // array of intfName strings.
    {
        strncpy(dupValue, value, sizeof(dupValue)-1);
        intfNameArray = valueStringToIntfNameArray(dupValue);
        if (intfNameArray == NULL)
        {
            free(currIntfNameArray);
            return WLMDM_GENERIC_ERROR;
        }
    }

    if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
    {
        free(currIntfNameArray);
        free(intfNameArray);
        return WLMDM_GENERIC_ERROR;
    }

    // Do a single pass over all the SSID objs to make sure their WlBrIndex
    // is set properly.
    {
       InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
       Dev2WifiSsidObject *ssidObj=NULL;
       CmsRet r2;

       while (cmsObj_getNextFlags(MDMOID_DEV2_WIFI_SSID, &iidStack,
                                  OGF_NO_VALUE_UPDATE,
                                  (void **) &ssidObj) == CMSRET_SUCCESS)
       {
           if (isInIntfNameArray(ssidObj->name, (const char **)intfNameArray))
           {
               if (!isInIntfNameArray(ssidObj->name, (const char **)currIntfNameArray))
               {
                  cmsLog_notice("brchange for %s detected: br index %d=>%d",
                               ssidObj->name,
                               ssidObj->X_BROADCOM_COM_WlBrIndex, bi);
                  ssidObj->X_BROADCOM_COM_WlBrIndex = bi;
                  if ((r2 = cmsObj_set(ssidObj, &iidStack)) != CMSRET_SUCCESS)
                  {
                      cmsLog_error("Set of SSID obj (%s) failed, ret=%d",
                                   ssidObj->name, r2);
                  }

                  // Send Bridge Notify Change to indicate change for interface
                  Notify_WL_Intf_EventMsg(ssidObj->enable ? MDMVS_UP : MDMVS_DOWN, ssidObj);
               }
               else
               {
                   cmsLog_debug("%s no change in bridge %d", ssidObj->name, bi);
               }
           }
           else
           {
               if (isInIntfNameArray(ssidObj->name, (const char **)currIntfNameArray))
               {
                   cmsLog_notice("Moving %s out from bridge %d", ssidObj->name, bi);
                   ssidObj->X_BROADCOM_COM_WlBrIndex = -1;
                   if ((r2 = cmsObj_set(ssidObj, &iidStack)) != CMSRET_SUCCESS)
                   {
                       cmsLog_error("Set of SSID obj (%s) failed, ret=%d",
                                    ssidObj->name, r2);
                   }
                   // Send Bridge Notify Change to indicate change for interface
                   Notify_WL_Intf_EventMsg(ssidObj->enable ? MDMVS_UP : MDMVS_DOWN,ssidObj);
               }
               else
               {
                   cmsLog_debug("no action on %s: enabled=%d WlBrIndex=%d bi=%d",
                                ssidObj->name, ssidObj->enable,
                                ssidObj->X_BROADCOM_COM_WlBrIndex, bi);
               }
           }
           cmsObj_free((void **)&ssidObj);
       }
    }

    cmsLck_releaseLock();
    free(currIntfNameArray);

    synced = 0;
    counter = 0;
    while ((synced == 0) && (counter < 5))
    {
        if (counter > 0)
        {
            cmsLog_notice("Verifying %s, sleep for 1 second...", nvname);
            sleep(1);
        }

        memset(childIntfNames, 0x0, sizeof(childIntfNames));
        get_lan_ifnames(nvname, childIntfNames, sizeof(childIntfNames));
        cmsLog_notice("got %s=%s", nvname, childIntfNames);
        currIntfNameArray = valueStringToIntfNameArray(childIntfNames);
        if (currIntfNameArray == NULL)
        {
            cmsLog_error("Unexpected, buffer exhausted?");
            ret = WLMDM_GENERIC_ERROR;
            break;
        }

        if (0 == compareIntfNameArray((const char **)intfNameArray, (const char **)currIntfNameArray))
        {
            synced = 1;
        }
        free(currIntfNameArray);
        counter++;
    }

    if (synced == 0)
    {
        cmsLog_error("Failed to verify %s value has been synced!", nvname);
    }

    free(intfNameArray);
    return ret;
}

// Given a nvname (eg lan_ifname, lan1_ifname) return a space separated list
// of wifi intf names in value.
static WlmdmRet get_lan_ifnames(const char *nvname, char *value, size_t size)
{
    int bi = 0, index;
    char br_name[SYSUTL_IFNAME_LEN] = {0};
    char childIntfNames[MAX_NVRAM_VALUE_SIZE] = {0}, *p;

    assert(value);
    memset(value, 0x00, size);

    // if nvname is lan_ifnames, this sscanf will fail and bi will remain 0.
    sscanf(nvname, "lan%d_ifnames", &bi);

    if (0 != getBridgeNameFromIndex(bi, (char *)&br_name, sizeof(br_name)))
    {
        return WLMDM_INVALID_PARAM;
    }

    // If bridge does not even exist on the system, skip it.
    index = sysUtl_getIfindexByIfname(br_name);
    if (index < 0)
        return WLMDM_OK;
 
    getBridgeChildrenInterfaces(br_name, childIntfNames, MAX_NVRAM_VALUE_SIZE);
    p = childIntfNames;
    while (*p != '\0')
    {
        if (*p == ',')
            *p = ' ';
        p++;
    }
    strncpy(value, childIntfNames, size);

    return WLMDM_OK;
}


// For each bridge in the system, call the foreach func with:
// nvname (eg lan_ifname, lan1_ifname), a space separated list of wifi intf
// names under that bridge, and the given context data.
static WlmdmRet foreach_lan_ifnames(nvc_for_each_func foreach_func, void *data)
{
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    char br_name[SYSUTL_IFNAME_LEN] = {0};
    char childIntfNames[MAX_NVRAM_VALUE_SIZE] = {0}, *p;
    int bi, index;

    for (bi = 0; bi < MAX_WIFI_BRIDGES; bi++)
    {
        if (0 != getBridgeNameFromIndex(bi, (char *)&br_name, sizeof(br_name)))
        {
            continue;
        }

        // If bridge does not even exist on the system, skip it.
        index = sysUtl_getIfindexByIfname(br_name);
        if (index < 0)
            continue;

        memset((void *)&nvname, 0x00, sizeof(nvname));
        memset((void *)&value, 0x00, sizeof(value));
        memset((void *)&childIntfNames, 0x00, sizeof(childIntfNames));

        if (bi == 0)
        {
            snprintf(nvname, sizeof(nvname), "lan_ifnames");
        }
        else
        {
            snprintf(nvname, sizeof(nvname), "lan%d_ifnames", bi);
        }

        getBridgeChildrenInterfaces(br_name, childIntfNames, MAX_NVRAM_VALUE_SIZE);

        p = childIntfNames;
        while (*p != '\0')
        {
            if (*p == ',')
                *p = ' ';
            p++;
        }
        strncpy(value, childIntfNames, MAX_NVRAM_VALUE_SIZE);

        // Call the provided callback func.
        foreach_func((char *)&nvname, (char *)&value, data);
    }

    return WLMDM_OK;
}


static WlmdmRet _set_auto_channel(unsigned int radio_index, UBOOL8 enable)
{
    MdmPathDescriptor pathDesc;
    WlmdmRet ret = 0;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
    PUSH_INSTANCE_ID(&pathDesc.iidStack, radio_index + 1);
    strncpy((char *)&pathDesc.paramName, "AutoChannelEnable", sizeof(pathDesc.paramName));

    if (enable == TRUE)
    {
        ret = set_param_from_pathDesc(&pathDesc, "1");
    }
    else
    {
        ret = set_param_from_pathDesc(&pathDesc, "0");
    }
    return ret;

}

static int _get_auto_channel(int radio_index)
{
    MdmPathDescriptor pathDesc;
    char buf[MAX_NVRAM_VALUE_SIZE];
    WlmdmRet ret;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
    PUSH_INSTANCE_ID(&pathDesc.iidStack, radio_index + 1);
    strncpy((char *)&pathDesc.paramName, "AutoChannelEnable", sizeof(pathDesc.paramName));

    ret = get_param_from_pathDesc(&pathDesc, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return -1;
    }
    return (int) strtol((char *)&buf, NULL, 10);
}

static int _get_caps(int radio_index, char* cap, int size)
{
    char cmd[32];
    FILE *fp;

    if (cap == NULL || wlgetintfNo() == 0)
        return -1;

    snprintf(cmd, sizeof(cmd)-1, "wl -i wl%d cap", radio_index);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return -1;
    }

    fgets(cap, size-1, fp);
    pclose(fp);
    return WLMDM_OK;
}

static WlmdmRet set_mbss_names(const char *nvname, const char *value)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_OK;
   Dev2WifiWbdCfgMbssObject *mbssObjArray[MAX_PREFIX_ENTR] = {NULL};
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   int mbssIdx = 0;

   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack nextIidStack = EMPTY_INSTANCE_ID_STACK;
   char prefixArr[MAX_PREFIX_ENTR][MAX_PREFIX_LEN] = {0};
   int entries, i, j;

   if(!nvname || !value)
      return WLMDM_INVALID_PARAM;

   entries = get_prefixList(value, prefixArr);
   if(entries < 0)
      return WLMDM_GENERIC_ERROR;

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
      return WLMDM_GENERIC_ERROR;

   /* first pass. retrieve all existing instances */
   while (cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&mbssObj) == CMSRET_SUCCESS)
   {
      mbssObjArray[mbssIdx++] = mbssObj;
      mbssObj = NULL;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);

   /* second pass. re-build up all instances with new prefix */
   for(i=0; i < entries; i++)
   {
      if (strlen(prefixArr[i]) == 0)
         continue;

      if (cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack, OGF_NO_VALUE_UPDATE,
               (void **)&mbssObj) != CMSRET_SUCCESS)
      {
         // no more instance to fill new prefix, add new instance
         if ((ret = cmsObj_addInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack)) != CMSRET_SUCCESS)
         {
            mdmRet = WLMDM_GENERIC_ERROR;
            goto grace_exit;
         }

         if ((ret = cmsObj_get(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &mbssObj)) != CMSRET_SUCCESS)
         { 
            cmsObj_deleteInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack);
            mdmRet = WLMDM_GENERIC_ERROR;
            goto grace_exit;
         }
      }

      // Reset instance to default
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(mbssObj->prefix, prefixArr[i], mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(mbssObj->ssid);
         CMSMEM_FREE_BUF_AND_NULL_PTR(mbssObj->akm);
         CMSMEM_FREE_BUF_AND_NULL_PTR(mbssObj->crypto);
         CMSMEM_FREE_BUF_AND_NULL_PTR(mbssObj->wpsPsk);
         mbssObj->map = 0;
         mbssObj->bandFlag = 0;
      }

      // Copy over the values from the existing instance with matched prefix.
      for (j = 0 ; j < mbssIdx ; j++)
      {
         if (strcmp(prefixArr[i], mbssObjArray[j]->prefix) == 0)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(mbssObj->ssid, mbssObjArray[j]->ssid, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(mbssObj->akm, mbssObjArray[j]->akm, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(mbssObj->crypto, mbssObjArray[j]->crypto, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(mbssObj->wpsPsk, mbssObjArray[j]->wpsPsk, mdmLibCtx.allocFlags);
            mbssObj->map = mbssObjArray[j]->map;
            mbssObj->bandFlag = mbssObjArray[j]->bandFlag;
         }
      }

      if ((ret = cmsObj_set((void *)mbssObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack);
         mdmRet = WLMDM_GENERIC_ERROR;
      }
      cmsObj_free((void **)&mbssObj);
   }

   // delete the rest redundant instances
   memcpy((void *)&nextIidStack, &iidStack, sizeof(InstanceIdStack));
   while (cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &nextIidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&mbssObj) == CMSRET_SUCCESS)
   {
      cmsObj_deleteInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &nextIidStack);
      cmsObj_free((void **)&mbssObj);
      memcpy((void *)&nextIidStack, &iidStack, sizeof(InstanceIdStack));
   }

grace_exit:
   /* clear mbssObjArray*/
   for (i = 0 ; i < mbssIdx ; i++)
   {
      if(mbssObjArray[i] != NULL)
      {
         cmsObj_free((void **)&mbssObjArray[i]);
      }
   }

   cmsLck_releaseLock();
   return mdmRet;
}

static WlmdmRet get_mbss_names(const char *nvname, char *value, size_t size)
{
   CmsRet cret = CMSRET_SUCCESS;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *sep;

   if(!nvname || !value || (0 != strcmp(nvname, "map_bss_names")))
      return WLMDM_INVALID_PARAM;

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
      return WLMDM_GENERIC_ERROR;

   value[0] = '\0';
   sep = "";

   while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
      if ((mbssObj->prefix == NULL) || (strlen(mbssObj->prefix) == 0))
      {
          cmsObj_free((void **)&mbssObj);
          continue;
      }

      if (strlen(value) + strlen(mbssObj->prefix) + strlen(sep) < size)
      {
          strcat(value, sep);
          strcat(value, mbssObj->prefix);
          sep = " ";
      }
      else
      {
          cmsLog_error("Buffer size not enough!");
          cmsObj_free((void **)&mbssObj);
          break;
      }
      cmsObj_free((void **)&mbssObj);
   }

   cmsLck_releaseLock();

   return strlen(value) ? WLMDM_OK : WLMDM_NOT_FOUND;
}

static WlmdmRet foreach_mbss_names(nvc_for_each_func foreach_func, void *data)
{
   char val[1024]={0};

   if (foreach_func)
   {
      get_mbss_names("map_bss_names", val, 1024);
      foreach_func("map_bss_names", val, data);
   }

   return WLMDM_OK;
}

static WlmdmRet set_mapbss(const char *nvname, const char *value)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_GENERIC_ERROR;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;
   char *dup, *prefix, *suffix;

   if(!nvname || !value)
      return WLMDM_INVALID_PARAM;

   dup = strdup(nvname);
   if(NULL == dup)
      return mdmRet;

   prefix = strtok_r(dup, "_", &suffix);

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
   {
      free(dup);
      return mdmRet;
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
        if (strcmp(mbssObj->prefix, prefix) == 0)
        {
            found = TRUE;
            break;
        }
        cmsObj_free((void **)&mbssObj);
   }

   if (found)
   {
      if (cmsUtl_strcmp("ssid", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->ssid, value);
      else if (cmsUtl_strcmp("akm", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->akm, value);
      else if (cmsUtl_strcmp("crypto", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->crypto, value);
      else if (cmsUtl_strcmp("wpa_psk", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->wpsPsk, value);
      else if (cmsUtl_strcmp("map", suffix) == 0)
         mbssObj->map = strtol(value, NULL, 0);
      else if (cmsUtl_strcmp("band_flag", suffix) == 0)
         mbssObj->bandFlag = strtol(value, NULL, 0);

      ret = cmsObj_set(mbssObj, &iidStack);
      cmsObj_free((void **)&mbssObj);
      mdmRet = WLMDM_OK;
   }
   else
   {
      mdmRet = WLMDM_NOT_FOUND;
   }

   cmsLck_releaseLock();
   free(dup);

   return mdmRet;
}

static WlmdmRet get_mapbss(const char *nvname, char *value, size_t size)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_GENERIC_ERROR;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;
   char *dup, *prefix, *suffix;

   if(!nvname || !value)
      return WLMDM_INVALID_PARAM;

   dup = strdup(nvname);
   if (NULL == dup)
      return mdmRet;

   prefix = strtok_r(dup, "_", &suffix);

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
   {
      free(dup);
      return mdmRet;
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
        if (strcmp(mbssObj->prefix, prefix) == 0)
        {
            found = TRUE;
            break;
        }
        cmsObj_free((void **)&mbssObj);
   }

   if (found)
   {
      if(cmsUtl_strcmp("ssid", suffix) == 0)
      {
         if (mbssObj->ssid)
            snprintf(value, size, "%s", mbssObj->ssid);
         else
            mdmRet = WLMDM_NOT_FOUND;

      }
      else if(cmsUtl_strcmp("akm", suffix) == 0)
      {
         if (mbssObj->akm)
            snprintf(value, size, "%s", mbssObj->akm);
         else
            mdmRet = WLMDM_NOT_FOUND;
      }
      else if(cmsUtl_strcmp("crypto", suffix) == 0)
      {
         if (mbssObj->crypto)
            snprintf(value, size, "%s", mbssObj->crypto);
         else
            mdmRet = WLMDM_NOT_FOUND;
      }
      else if(cmsUtl_strcmp("wpa_psk", suffix) == 0)
      {
         if (mbssObj->wpsPsk)
            snprintf(value, size, "%s", mbssObj->wpsPsk);
         else
            mdmRet = WLMDM_NOT_FOUND;
      }
      else if(cmsUtl_strcmp("map", suffix) == 0)
      {
         snprintf(value, size, "0x%x", mbssObj->map);
      }
      else if (cmsUtl_strcmp("band_flag", suffix) == 0)
      {
         snprintf(value, size, "0x%x", mbssObj->bandFlag);
      }

      cmsObj_free((void **)&mbssObj);
      if (mdmRet != WLMDM_NOT_FOUND)
         mdmRet = WLMDM_OK;
   }
   else
   {
      mdmRet = WLMDM_NOT_FOUND;
   }

   cmsLck_releaseLock();
   free(dup);

   return mdmRet;
}

static WlmdmRet foreach_mapbss(nvc_for_each_func foreach_func, void *data)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_GENERIC_ERROR;  /*actually the caller does not care*/
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char name[MAX_NVRAM_NAME_SIZE] = {0};
   char value[MAX_NVRAM_VALUE_SIZE] = {0};

   if(!foreach_func)
      return WLMDM_INVALID_PARAM;

   if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
      return mdmRet;

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_ssid", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->ssid);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_akm", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->akm);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_crypto", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->crypto);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_wpa_psk", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->wpsPsk);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_map", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%d", mbssObj->map);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_band_flag", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%d", mbssObj->bandFlag);
      foreach_func(name, value, data);

      cmsObj_free((void **)&mbssObj);
   }

   cmsLck_releaseLock();
   return WLMDM_OK;
}

static WlmdmRet set_nctrlsb(const char *nvname, const char *value)
{
    WlmdmRet ret;
    int radio_index, bssid_index;
    char buf[32]= {0};
    
    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = nvn_gen("mdmsideband", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    return conv_set_mapped((char *)&buf, (char *)value);        
}


static WlmdmRet get_nctrlsb(const char *nvname, char *value, size_t size)
{

    WlmdmRet ret;
    struct chanspec_t chanspecbuf = {0};
    struct chanspec_t *chanspec=&chanspecbuf;
    int radio_index, bssid_index;
    char buf[32], buf_1[32];
    const char *str = NULL ;
    int autoChannel = 0;
    long int b;
    unsigned int channel = 0;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = nvn_gen("mdmsideband", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    ret = conv_get_mapped((char *)&buf , (char *)value, size);
    if (ret != WLMDM_OK)
    {
        return WLMDM_INVALID_PARAM;    
    }

    cmsLog_debug("%s=%s", nvname,value);

    if(strcmp(value, "Auto"))
    { /* Control Sideband != "Auto" */
        return ret;
    }

    //Handle Control Sideband == "Auto"

    /* by default use lowest channel as control channel */
    chanspec->band_width = WL_CHANSPEC_BW_20;
    chanspec->ctlsb_index = 0;

    cmsLog_debug("Auto Control Sideband : %s=%s", nvname,value);

    do{
        ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
        if (ret != WLMDM_OK)
        {
            cmsLog_error("Failed to parse nvname: %s", nvname);
            return ret;
        }

        ret = nvn_gen("bw", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                long int b;
                b = strtol((char *)&buf_1, NULL, 10);
                if ((b >= 0) && (b < LONG_MAX))
                {
                    /*
                     * translate bw_cap to chanspec.band_width.
                     * for example: bw_cap = 0x7 -> 20/40/80MHz
                     * chanspec.band_width will be WL_CHANSPEC_BW_80 as max bandwidth of the specified capability
                     */
                    chanspec->band_width = WL_BW_CAP_320MHZ(b) ? WL_CHANSPEC_BW_320 :
                                           (WL_BW_CAP_160MHZ(b) ? WL_CHANSPEC_BW_160 :
                                           (WL_BW_CAP_80MHZ(b) ? WL_CHANSPEC_BW_80 :
                                           (WL_BW_CAP_40MHZ(b) ? WL_CHANSPEC_BW_40 :
                                           (WL_BW_CAP_20MHZ(b) ? WL_CHANSPEC_BW_20 : 0))));
                }
                else
                {
                    cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
                }
            }
        }

        autoChannel = _get_auto_channel(radio_index);
        if ((autoChannel != 0))
        { /* Nothing to do with AutoChannel + AutoSideBand */
            cmsLog_debug("No Channel from MDM");
            break;
        }

        ret = nvn_gen("channel", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                b = strtol((char *)&buf_1, NULL, 10);
                if ((b < 0) || (b == LONG_MAX))
                {
                    cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
                    break;
                }
                else
                {
                    channel = (unsigned int) b;
                }
            }
        }

        ret = nvn_gen("nband", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                if (0 == strcmp((char *)buf_1, "1"))
                {
                    chanspec->band = WL_CHANSPEC_BAND_5G;
                }
                else if (0 == strcmp((char *)buf_1, "2"))
                {
                    chanspec->band = WL_CHANSPEC_BAND_2G;
                }
                else
                {
                    chanspec->band = WL_CHANSPEC_BAND_6G;
                }
            }
        }

        /* Handle Auto Side Band Select */
        switch (chanspec->band_width)
        {
            case WL_CHANSPEC_BW_40:
                /* Auto side band base on channel on 40Mhz
                   CH 1 -->  7 => "lower"
                   CH 8 --> 14 => "upper"
                   For 5G : use lowest channel as control channel
                */
                if(chanspec->band == WL_CHANSPEC_BAND_2G)
                {
                    chanspec->ctlsb_index = chanspec_get_40bw_sb_index((channel < 8) ? "lower" : "upper");
                    cmsLog_debug("Auto sideband CH:%d  SB:%d",channel,chanspec->ctlsb_index);
                }else
                {
                    /* use lowest channel as control channel */
                    chanspec->ctlsb_index = 0; 
                }
                break;
    
            case WL_CHANSPEC_BW_80:
            case WL_CHANSPEC_BW_160:
            case WL_CHANSPEC_BW_320:
            default:
                /* use lowest channel as control channel */
                chanspec->ctlsb_index = 0; 
                break;
        }

    }while(0);

    if((str = chanspec_get_sb_string(chanspec->ctlsb_index, chanspec->band_width)) != NULL)
    {
        strncpy(value, str, size - 1);
        ret = WLMDM_OK;
    }else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}

static WlmdmRet foreach_nctrlsb(nvc_for_each_func foreach_func, void *data)
{
    int i, num_radio;
    char nvname[MAX_NVRAM_NAME_SIZE] = {0}, value[MAX_NVRAM_VALUE_SIZE] = {0};
    WlmdmRet ret;

    num_radio = get_num_instances(MDMOID_DEV2_WIFI_RADIO);
    for (i = 0; i < num_radio; i++)
    {
        ret = nvn_gen("mdmsideband", i, -1, (char *)&nvname, sizeof(nvname));
        if (ret == WLMDM_OK)
        {
            ret = get_nctrlsb(nvname, value, MAX_NVRAM_VALUE_SIZE);
            if (ret == WLMDM_OK)
            {
                ret = nvn_gen("nctrlsb", i, -1, (char *)&nvname, sizeof(nvname));
                foreach_func(nvname, value, data);
            }
        }
    }
    return WLMDM_OK;
}

static WlmdmRet get_wl_lan_ipv4addr(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    int radio_index = -1, bssid_index = -1, br_index;
    int ssid_instance;
    char fullpath[BUFLEN_512] = {0};
    char buf[BUFLEN_32];
    char *br_index_str;
    InstanceIdStack iidStack;

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = get_iidStack(MDMOID_DEV2_WIFI_SSID, radio_index, bssid_index, &iidStack);
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to get iidStack, radio_index[%d], bssid_index[%d]",
                       radio_index, bssid_index);
        return ret;
    }

    ssid_instance = iidStack.instance[0];
    snprintf(fullpath, sizeof(fullpath) - 1,
            "Device.WiFi.SSID.%d.X_BROADCOM_COM_WlBrIndex",
            ssid_instance);

    br_index_str = get_parameter_local(fullpath, OGF_NO_VALUE_UPDATE);
    cmsLog_notice("bridge index is %s", br_index_str);
    br_index = atoi(br_index_str);
    free(br_index_str);
    
    ret = getBridgeNameFromIndex(br_index, buf, sizeof(buf));
    if (ret == -1)
    {
        cmsLog_error("Failed to get bridge name from bridge index %d", br_index);
        return WLMDM_GENERIC_ERROR;
    }
    cmsLog_notice("brname for index %d is %s", br_index, buf);

    ret = (0 == getInterfaceIPv4Addr(buf, value, size)) ?
                WLMDM_OK : WLMDM_GENERIC_ERROR;

    return ret;
}

static WlmdmRet set_wl_lan_ipv4addr(const char *nvname __attribute__((unused)),
                                    const char *value __attribute__((unused)))
{
    return WLMDM_GENERIC_ERROR;
}

static WlmdmRet get_ap_friendly_name(const char *nvname, char *value, size_t size)
{
    char *result;
    WlmdmRet ret = WLMDM_OK;

#ifdef BRCM_BDK_BUILD
    result = get_parameter_remote(BDK_COMP_DEVINFO, "Device.DeviceInfo.FriendlyName",
                                  OGF_NO_VALUE_UPDATE | OGF_LOCAL_MDM_ONLY);
#else
    result = get_parameter_local("Device.DeviceInfo.FriendlyName", OGF_NO_VALUE_UPDATE);
#endif
    if (result == NULL)
    {
        cmsLog_error("Failed to get ap_friendly_name from MDM!");
        ret = WLMDM_GENERIC_ERROR;
    }
    else
    {
        strncpy(value, result, size - 1);
        free(result);
    }
    return ret;
}

static WlmdmRet set_ap_friendly_name(const char *nvname __attribute__((unused)),
                                    const char *value __attribute__((unused)))
{
    return WLMDM_GENERIC_ERROR;
}

static WlmdmRet get_ap_serial_number(const char *nvname, char *value, size_t size)
{
    char *result;
    WlmdmRet ret = WLMDM_OK;
#ifdef BRCM_BDK_BUILD
    result = get_parameter_remote(BDK_COMP_DEVINFO, "Device.DeviceInfo.SerialNumber",
                                  OGF_NO_VALUE_UPDATE | OGF_LOCAL_MDM_ONLY);
#else
    result = get_parameter_local("Device.DeviceInfo.SerialNumber", OGF_NO_VALUE_UPDATE);
#endif
    if (result == NULL)
    {
        cmsLog_error("Failed to get ap_serial_number from MDM!");
        ret = WLMDM_GENERIC_ERROR;
    }
    else
    {
        strncpy(value, result, size - 1);
        free(result);
    }
    return ret;
}

static WlmdmRet set_ap_serial_number(const char *nvname __attribute__((unused)),
                                    const char *value __attribute__((unused)))
{
    return WLMDM_GENERIC_ERROR;
}

static WlmdmRet get_ap_vendor(const char *nvname, char *value, size_t size)
{
    char *result;
    WlmdmRet ret = WLMDM_OK;

#ifdef BRCM_BDK_BUILD
    result = get_parameter_remote(BDK_COMP_DEVINFO, "Device.DeviceInfo.Manufacturer",
                                  OGF_NO_VALUE_UPDATE | OGF_LOCAL_MDM_ONLY);
#else
    result = get_parameter_local("Device.DeviceInfo.Manufacturer", OGF_NO_VALUE_UPDATE);
#endif
    if (result == NULL)
    {
        cmsLog_error("Failed to get ap_vendor from MDM!");
        ret = WLMDM_GENERIC_ERROR;
    }
    else
    {
        strncpy(value, result, size - 1);
        free(result);
    }
    return ret;
}

static WlmdmRet set_ap_vendor(const char *nvname __attribute__((unused)),
                              const char *value __attribute__((unused)))
{
    return WLMDM_GENERIC_ERROR;
}
static WlmdmRet foreach_empty(nvc_for_each_func foreach_func, void *data)
{
    return WLMDM_OK;
}


static WlmdmRet parse_mobappd_pairing_name(const char *nvname, int *index, char *param_name)
{
    int ret;
    const char *mobappd_pairing_pattern = "mobappd_pairing_%d_%40s";

    ret = sscanf(nvname, mobappd_pairing_pattern, index, param_name);
    if (ret != 2)
    {
        cmsLog_error("expecting format %s", mobappd_pairing_pattern);
        return WLMDM_INVALID_PARAM;
    }
    return WLMDM_OK;
}

static const char* mobappd_lookup_param_name(const char *nv_subname)
{
    const char *param_name = NULL;

    if (0 == strcmp(nv_subname, "status"))
    {
        param_name = "Status";
    }
    else if (0 == strcmp(nv_subname, "dev_id"))
    {
        param_name = "DeviceID";
    }
    else if (0 == strcmp(nv_subname, "dev_type"))
    {
        param_name = "DeviceType";
    }
    else if (0 == strcmp(nv_subname, "name"))
    {
        param_name = "FriendlyName";
    }
    else if (0 == strcmp(nv_subname, "push_token"))
    {
        param_name = "PushToken";
    }
    else if (0 == strcmp(nv_subname, "attestation_key"))
    {
        param_name = "AttestationKey";
    }
    else if (0 == strcmp(nv_subname, "dev_model"))
    {
        param_name = "DeviceModel";
    }
    else if (0 == strcmp(nv_subname, "os_version"))
    {
        param_name = "OsVersion";
    }
    else if (0 == strcmp(nv_subname, "date_added"))
    {
        param_name = "DateAdded";
    }
    return param_name;
}

static WlmdmRet get_mobappd_pairing(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret = WLMDM_OK;
    int index = 0; 
    char nv_subname[MAX_NVRAM_NAME_SIZE] = {0};
    const char *param_name;
    MdmPathDescriptor path_pairing = {0};

    ret = parse_mobappd_pairing_name(nvname, &index, (char *)&nv_subname);
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    cmsLog_debug("index=%d, nv_subname=%s", index, nv_subname);

    param_name = mobappd_lookup_param_name(nv_subname);
    if (param_name == NULL)
    {
        return WLMDM_NOT_FOUND;
    }

    path_pairing.oid = MDMOID_DEV2_WIFI_MOBAPPD_PAIRING;
    PUSH_INSTANCE_ID(&path_pairing.iidStack, index + 1);
    strncpy((char *)&path_pairing.paramName, param_name, sizeof(path_pairing.paramName)-1);

    ret = get_param_from_pathDesc(&path_pairing, value, size);

    return ret; 
}

static WlmdmRet set_mobappd_pairing(const char *nvname __attribute__((unused)),
                                    const char *value __attribute__((unused)))
{
    WlmdmRet ret = WLMDM_OK;
    int index; 
    char nv_subname[MAX_NVRAM_NAME_SIZE] = {0};
    const char *param_name;
    MdmPathDescriptor path_pairing = {0};

    ret = parse_mobappd_pairing_name(nvname, &index, (char *)&nv_subname);
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    cmsLog_debug("index=%d, nv_subname=%s", index, nv_subname);

    param_name = mobappd_lookup_param_name(nv_subname);
    if (param_name == NULL)
    {
        return WLMDM_NOT_FOUND;
    }

    path_pairing.oid = MDMOID_DEV2_WIFI_MOBAPPD_PAIRING;
    PUSH_INSTANCE_ID(&path_pairing.iidStack, index + 1);
    strncpy((char *)&path_pairing.paramName, param_name, sizeof(path_pairing.paramName)-1);

    ret = set_param_from_pathDesc(&path_pairing, value);

    return ret; 
}

static WlmdmRet foreach_mobappd_pairing(nvc_for_each_func foreach_func, void *data)
{
    Dev2WifiMobappdPairingObject *pairingObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    char name[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    const char *mobappd_pairing_pattern = "mobappd_pairing_%d_%s";

    if (!foreach_func)
       return WLMDM_INVALID_PARAM;

    if (cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT) != CMSRET_SUCCESS)
       return WLMDM_GENERIC_ERROR;

    while (CMSRET_SUCCESS == cmsObj_getNextFlags(MDMOID_DEV2_WIFI_MOBAPPD_PAIRING, &iidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **) &pairingObj))
    {
        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "status");
        snprintf(value, sizeof(value), "%s", pairingObj->status);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "dev_id");
        snprintf(value, sizeof(value), "%s", pairingObj->deviceID);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "dev_type");
        snprintf(value, sizeof(value), "%s", pairingObj->deviceType);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "name");
        snprintf(value, sizeof(value), "%s", pairingObj->friendlyName);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "push_token");
        snprintf(value, sizeof(value), "%s", pairingObj->pushToken);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "attestation_key");
        snprintf(value, sizeof(value), "%s", pairingObj->attestationKey);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "dev_model");
        snprintf(value, sizeof(value), "%s", pairingObj->deviceModel);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "os_version");
        snprintf(value, sizeof(value), "%s", pairingObj->osVersion);
        foreach_func(name, value, data);

        snprintf(name, sizeof(name), mobappd_pairing_pattern,
                PEEK_INSTANCE_ID(&iidStack) - 1, "date_added");
        snprintf(value, sizeof(value), "%s", pairingObj->dateAdded);
        foreach_func(name, value, data);

        cmsObj_free((void **)&pairingObj);
    }

    cmsLck_releaseLock();
    return WLMDM_OK;
}
