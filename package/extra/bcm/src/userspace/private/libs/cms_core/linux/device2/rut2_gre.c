/***********************************************************************
 *
 *  Copyright (c) 2021  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2021:proprietary:standard

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
#ifdef SUPPORT_GRE_TUNNEL_TR181

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_dal.h"
#include "rcl.h"
#include "prctl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut2_ip.h"
#include "qdm_intf.h"
#include "cms_tmr.h"
#include "rut2_route.h"
#include "rut2_ipv6.h"
#include "rut2_gre.h"

CmsRet rutGre_getWanIP_dev2(const char *wanIntf, char *ipaddr, UBOOL8 isIPv4)
{
   const char *searchIfName;
   char *ptr;

   searchIfName = wanIntf;
   if (FALSE == qdmIpIntf_isWanInterfaceUpLocked(searchIfName, isIPv4))
   {
      /* the provided interface is not up */
      return CMSRET_OBJECT_NOT_FOUND;
   }

   if (qdmIpIntf_isWanInterfaceBridgedLocked(searchIfName))
   {
      /* the provided interface is not routed */
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (isIPv4)
   {
      if (CMSRET_SUCCESS != qdmIpIntf_getIpv4AddressByNameLocked(searchIfName, ipaddr))
      {
         return CMSRET_OBJECT_NOT_FOUND;
      }
   }
   else
   {
      if (CMSRET_SUCCESS != qdmIpIntf_getIpv6AddressByNameLocked(searchIfName, ipaddr))
      {
         return CMSRET_OBJECT_NOT_FOUND;
      }
   }

   ptr = cmsUtl_strstr(ipaddr, "/");
   if (ptr)
   {
      *ptr = '\0';
   }

   return CMSRET_SUCCESS;
}

CmsRet rutGre_create_tunnel_intf_dev2(char *name, UBOOL8 isIpv6, UBOOL8 isL2, UBOOL8 cs, UBOOL8 sn,
          UBOOL8 key, UINT32 keyId, char *remoteEP, char *dev)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_256];
   char wanIP[CMS_IPADDR_LENGTH];
   char local_ip[CMS_IPADDR_LENGTH+8];
   char key_str[32] = {0};

   if ((ret = rutGre_getWanIP_dev2(dev, wanIP, !isIpv6)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get Wan: %s IP address", dev);
      local_ip[0] = '\0';
      return ret;
   }
   else
   {
      sprintf(local_ip, "local %s", wanIP);
   }

   if (key)
   {
      sprintf(key_str, "key %d", keyId);
   }
   snprintf(cmd, sizeof(cmd), "ip %s add %s %s %s%s remote %s %s %s %s %s >/dev/null",
                (!isIpv6 && !isL2)? "tunnel":"link",
                name,
                (!isIpv6 && !isL2)? "mode":"type",
                isIpv6? "ip6":"",
                isL2? "gretap":"gre",
                remoteEP,
                local_ip,
                key_str,
                cs? "csum":"",
                sn? "seq":""
          );

   rut_doSystemAction(__FUNCTION__, cmd);

   snprintf(cmd, sizeof(cmd), "ip link set %s up >/dev/null", name);
   rut_doSystemAction(__FUNCTION__, cmd);
   return ret;
}

CmsRet rutGre_delete_tunnel_intf_dev2(char *name)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_256] = {0};
   if ( name != NULL)
   {
      snprintf(cmd, sizeof(cmd), "ip link delete %s >/dev/null", name);
      rut_doSystemAction(__FUNCTION__, cmd);
   }
   return ret;
}

CmsRet rutGre_active_brport_dev2(char *ifname, UBOOL8 active)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2BridgePortObject *brPortObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   cmsLog_notice("Enter ifname=%s, active=%d", ifname, active);

   while ((!found) &&
          (cmsObj_getNext(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                          (void **) &brPortObj) == CMSRET_SUCCESS))
   {
      if (cmsUtl_strcmp(brPortObj->name, ifname) == 0)
      {
         found = TRUE;

         // Internal code, i.e. this RUT function, should not set the "enable" param.
         // "enable" is something the management system (WebUI, TR69 ACS) uses to "enable" or "disable"
         // an object.  To minimize changes for now, leave it here, but
         // could/should be removed later if it causes problems.
         brPortObj->enable = active;

         // The actual intention of the code is to set the status to "UP" or
         // "DOWN".  When rcl_dev2BridgePortObject() detects the status has
         // changed, it will do actions.
         CMSMEM_REPLACE_STRING_FLAGS(brPortObj->status,
                                     (active ? MDMVS_UP : MDMVS_DOWN),
                                     mdmLibCtx.allocFlags);
         if ((ret = cmsObj_set(brPortObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2BridgePortObject, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &brPortObj);
   }
   return ret;
}

#ifdef DMP_DEVICE2_ROUTING_1
CmsRet rutGre_activateRouting_dev2(const char *ifname, UBOOL8 active)
{
   CmsRet ret = CMSRET_SUCCESS;

   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv4ForwardingObject *ipv4ForwardingObj = NULL;
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   Dev2Ipv6ForwardingObject *ipv6ForwardingObj = NULL;
#endif
   char dev_name[CMS_IFNAME_LENGTH] = {0};

   cmsLog_debug("Enter %s GRE tunnel intf:%s", active? "Active" : "Deactive", ifname);
   while (!ret && cmsObj_getNextFlags(MDMOID_DEV2_IPV4_FORWARDING, &iidStack,
          OGF_NO_VALUE_UPDATE, (void **)&ipv4ForwardingObj) == CMSRET_SUCCESS)
   {
      if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(ipv4ForwardingObj->interface, dev_name)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to qdmIntf_fullPathToIntfnameLocked_dev2");
      }
      else if (!cmsUtl_strcmp(dev_name, ifname))
      {
         ipv4ForwardingObj->enable = active;
         if ((ret = cmsObj_set(ipv4ForwardingObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2Ipv4ForwardingObject");
         }
      }
      cmsObj_free((void **)&ipv4ForwardingObj);
   }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!ret && cmsObj_getNextFlags(MDMOID_DEV2_IPV6_FORWARDING, &iidStack,
          OGF_NO_VALUE_UPDATE, (void **)&ipv6ForwardingObj) == CMSRET_SUCCESS)
   {
      if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(ipv6ForwardingObj->interface, dev_name)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to qdmIntf_fullPathToIntfnameLocked_dev2");
      }
      else if (!cmsUtl_strcmp(dev_name, ifname))
      {
         ipv6ForwardingObj->enable = active;
         if ((ret = cmsObj_set(ipv6ForwardingObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2Ipv6ForwardingObject");
         }
      }
      cmsObj_free((void **)&ipv6ForwardingObj);
   }
#endif
   return ret;
}
#endif

CmsRet rutGre_activateTunnelIf_dev2(const char *ifname, UBOOL8 active)
{
   CmsRet ret = CMSRET_SUCCESS;

   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2GreTunnelObject *tunnelObj = NULL;
   Dev2GreTunnelInterfaceObject *intfObj = NULL;
   char dev_name[CMS_IFNAME_LENGTH] = {0};

   cmsLog_notice("Entered: %s GRE tunnel intf %s",
                 active ? "Active" : "Deactive", ifname);

   while (cmsObj_getNextFlags(MDMOID_DEV2_GRE_TUNNEL, &iidStack,
                 OGF_NO_VALUE_UPDATE, (void **)&tunnelObj) == CMSRET_SUCCESS)
   {
       while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStack, &iidStackChild,
                 OGF_NO_VALUE_UPDATE, (void **)&intfObj) == CMSRET_SUCCESS)
      {
         if ((ret =qdmIntf_fullPathToIntfnameLocked_dev2(intfObj->lowerLayers, dev_name)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. lowerLayers=%s",
                          intfObj->lowerLayers);
            cmsObj_free((void **) &tunnelObj);
            cmsObj_free((void **) &intfObj);
            return ret;
         }
         if (cmsUtl_strcmp(dev_name, ifname))
         {
            cmsObj_free((void **) &intfObj);
            continue;
         }

         if ((ret = cmsObj_set(intfObj, &iidStackChild)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2GreTunnelInterfaceObject");
            cmsObj_free((void **) &intfObj);
            cmsObj_free((void **)&tunnelObj);
            return ret;
         }

         if (intfObj->X_BROADCOM_COM_L2_Mode)
         {
            printf("GRE: %s Layer 2 tunnel in %s/%s",
                   active ? "Activating" : "Deactivating",
                   intfObj->name, ifname);
            rutGre_active_brport_dev2(intfObj->name, active);
         }
#ifdef DMP_DEVICE2_ROUTING_1
         else
         {
            printf("GRE: %s Layer 3 tunnel in %s/%s",
                   active ? "Activating" : "Deactivating",
                   intfObj->name, ifname);
            rutGre_activateRouting_dev2(intfObj->name, active);
         }
#endif

         cmsObj_free((void **)&intfObj);
      }
      cmsObj_free((void **)&tunnelObj);
   }
   return ret;
}

UBOOL8 rut_isGreTunnelInterfaceUpLocked(const char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2GreTunnelInterfaceObject *intfObj = NULL;
   UBOOL8 isServiceUp = FALSE;
   UBOOL8 found = FALSE;

   while (!found && cmsObj_getNextFlags(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&intfObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(intfObj->name, ifName))
      {
         isServiceUp = !cmsUtl_strcmp(intfObj->status,
                                      MDMVS_UP);
         found = TRUE;
      }
      cmsObj_free((void **) &intfObj);
   }

   cmsLog_debug("ifName=%s isServiceUp=%d", ifName, isServiceUp);

   return isServiceUp;
}


#endif /* SUPPORT_GRE_TUNNEL_TR181 */

