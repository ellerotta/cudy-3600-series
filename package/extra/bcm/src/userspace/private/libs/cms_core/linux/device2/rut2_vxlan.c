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
#ifdef SUPPORT_VXLAN_TUNNEL_TR181

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
#include "rut2_vxlan.h"

int rutVxlan_numTunnelEntries_dev2(void)
{
   int numTunCfg = 0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanTunnelObject *vxlanTunnelObj = NULL;

   while (cmsObj_getNextFlags(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&vxlanTunnelObj) == CMSRET_SUCCESS)
   {
      if (vxlanTunnelObj->enable)
      {
         numTunCfg++;
      }
      cmsObj_free((void **)&vxlanTunnelObj);
   }

   return numTunCfg;
}


CmsRet rutVxlan_getWanIP_dev2(const char *wanIntf, char *ipaddr, UBOOL8 isIPv4)
{
   char ifNameBuf[CMS_IFNAME_LENGTH]={0};
   const char *searchIfName;
   char *ptr;

   if ((wanIntf == NULL) || (0 == strlen(wanIntf)))
   {
      if (FALSE == rutWan_findFirstIpvxRoutedAndConnected(isIPv4 ? CMS_AF_SELECT_IPV4 : CMS_AF_SELECT_IPV6, ifNameBuf))
      {
         return CMSRET_OBJECT_NOT_FOUND;
      }
      searchIfName = ifNameBuf;
   }
   else
   {
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

CmsRet rutVxlan_create_tunnel_intf_dev2(char *name, UBOOL8 isIpv6, SINT32 vni, UINT32 rport,
          UINT32 sport, char *remoteEP, char *dev, UINT32 timeout, UINT32 thrsd,
          SINT32 zeroCsumTx, SINT32 zeroCsumRx)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_256];
   char wanIP[CMS_IPADDR_LENGTH];
   char local_ip[CMS_IPADDR_LENGTH+8];
   char cs_tx[BUFLEN_32] = {0};
   char cs_rx[BUFLEN_32] = {0};
   

   if ((ret = rutVxlan_getWanIP_dev2(dev, wanIP, !isIpv6)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get Wan: %s IP address", dev);
      local_ip[0] = '\0';
   }
   else
   {
      sprintf(local_ip, "local %s", wanIP);
   }

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get parent tunnel interface, ret=%d", ret);
      return ret;
   }

   if (isIpv6)
   {
        snprintf(cs_tx, sizeof(cs_tx), "%s", zeroCsumTx? "udp6zerocsumtx":"noudp6zerocsumtx");
        snprintf(cs_rx, sizeof(cs_rx), "%s", zeroCsumRx? "udp6zerocsumrx":"noudp6zerocsumrx");
   }

   snprintf(cmd, sizeof(cmd), "ip link add %s type vxlan id %d dstport %d srcport %d %d remote %s %s %s %s dev %s ",
                name, vni, rport, sport, sport, remoteEP, local_ip, cs_tx, cs_rx, dev);
   rut_doSystemAction("rut2", cmd);

   snprintf(cmd, sizeof(cmd), "ifconfig %s up", name);
   rut_doSystemAction("rut2", cmd);

   snprintf(cmd, sizeof(cmd), "%s -w -I INPUT -i %s -p udp --dport %d -j ACCEPT", isIpv6 ? "ip6tables" : "iptables", dev, rport);
   rut_doSystemAction("rut2", cmd);

   return ret;
}

CmsRet rutVxlan_delete_tunnel_intf_dev2(char *name, UBOOL8 isIpv6, UINT32 rport, char *dev)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_256] = {0};

   snprintf(cmd, sizeof(cmd), "%s -w -D INPUT -i %s -p udp --dport %d -j ACCEPT", isIpv6 ? "ip6tables" : "iptables", dev, rport);
   rut_doSystemAction("rut2", cmd);

   snprintf(cmd, sizeof(cmd), "ip link delete %s ", name);
   rut_doSystemAction("rut2", cmd);

   return ret;
}

CmsRet rutVxlan_active_ipintf_dev2(char *ifname, UBOOL8 active)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStackIpIf = EMPTY_INSTANCE_ID_STACK;
   char dev_name[CMS_IFNAME_LENGTH]={0};

   cmsLog_notice("Entered: %s VxLan IP service on %s",
                 active ? "Active" : "Deactive", ifname);

   while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStackIpIf,
                          (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if ((ret =qdmIntf_fullPathToIntfnameLocked_dev2(ipIntfObj->lowerLayers, dev_name)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. lowerLayers=%s",
                       ipIntfObj->lowerLayers);
         cmsObj_free((void **) &ipIntfObj);
         return ret;
      }

      if (cmsUtl_strcmp(dev_name, ifname))
      {
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      if (active)
      {
         ipIntfObj->enable = 0;
         if ((ret = cmsObj_set(ipIntfObj, &iidStackIpIf)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2IpInterfaceObject");
         }
      }

      ipIntfObj->enable = active;
      CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->status, active? MDMVS_UP : MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
      if ((ret = cmsObj_set(ipIntfObj, &iidStackIpIf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set Dev2IpInterfaceObject");
      }

      cmsObj_free((void **) &ipIntfObj);
   }
   return ret;
}

CmsRet rutVxlan_active_brport_dev2(char *ifname, UBOOL8 active)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2BridgePortObject *brPortObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_notice("Entered: %s VxLan br port %s",
                 active ? "Active" : "Deactive", ifname);

   while (cmsObj_getNext(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                          (void **) &brPortObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(brPortObj->name, ifname))
      {
         cmsObj_free((void **) &brPortObj);
         continue;
      }

      // Internal code, such as this RUT function, should not set the "enable" param.
      // "enable" is something the management system (WebUI, TR69 ACS) uses to "enable" or "disable"
      // an object.  Leave it alone for now.
      brPortObj->enable = active;

      // The actual intention of the code is to set the status to "UP" or
      // "DOWN".  When rcl_dev2BridgePortObject() detects the status has
      // changed, it will do actions.
      CMSMEM_REPLACE_STRING_FLAGS(brPortObj->status,
                                  (active ? MDMVS_UP : MDMVS_DOWN),
                                  mdmLibCtx.allocFlags);
      if ((ret = cmsObj_set(brPortObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set Dev2BridgePortObject");
      }

      cmsObj_free((void **) &brPortObj);
   }
   return ret;
}


CmsRet rutVxlan_activateTunnelIf_dev2(const char *ifname, UBOOL8 active)
{
   CmsRet ret = CMSRET_SUCCESS;

   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanTunnelObject *tunnelObj = NULL;
   Dev2VxlanTunnelInterfaceObject *intfObj = NULL;
   char dev_name[CMS_IFNAME_LENGTH]={0};

   cmsLog_notice("Entered: %s vxlan tunnel intf %s",
                 active ? "Active" : "Deactive", ifname);

   while (cmsObj_getNextFlags(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack,
                 OGF_NO_VALUE_UPDATE, (void **)&tunnelObj) == CMSRET_SUCCESS)
   {
       while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStack, &iidStackChild,
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
            cmsLog_error("Failed to set Dev2VxlanTunnelInterfaceObject");
            cmsObj_free((void **) &intfObj);
            cmsObj_free((void **)&tunnelObj);
            return ret;
         }

         if (intfObj->X_BROADCOM_COM_L2_Mode)
         {
            printf("VXLAN: %s Layer 2 tunnel in %s/%s",
                   active ? "Activating" : "Deactivating",
                   intfObj->name, ifname);
            rutVxlan_active_brport_dev2(intfObj->name, active);
         }
         else
         {
            printf("VXLAN: %s Layer 3 tunnel in %s/%s",
                   active ? "Activating" : "Deactivating",
                   intfObj->name, ifname);
            rutVxlan_active_ipintf_dev2(intfObj->name, active);
         }

         cmsObj_free((void **)&intfObj);
      }
      cmsObj_free((void **)&tunnelObj);
   }
   return ret;
}

void rutVxlan_updateTunnelIfNum_dev2(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanObject *vxlanObj = NULL;
   CmsRet ret;

   INIT_INSTANCE_ID_STACK(&iidStack);

   if ((delta != 1) && (delta != -1))
   {
      cmsLog_error("delta support 1 or -1 only, delta:%d", delta);
   }

   if (cmsObj_get(MDMOID_DEV2_VXLAN, &iidStack,
               OGF_NO_VALUE_UPDATE, (void **) &vxlanObj) == CMSRET_SUCCESS)
   {
      if ((delta < 0) && (vxlanObj->X_BROADCOM_COM_TunnelIfNumOfEntries == 0))
      {
         cmsLog_error("underflow detected");
      }
      else
      {
         vxlanObj->X_BROADCOM_COM_TunnelIfNumOfEntries += delta;

         if ((ret = cmsObj_set(vxlanObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set Dev2VxlanObject error, ret=%d", ret);
         }
      }
      cmsObj_free((void **) &vxlanObj);
   }
   else
   {
      cmsLog_error("get Dev2VxlanObject failed.");
   }
}

CmsRet rutVxlan_getTunnelIfNum_dev2(UINT32 *num)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanObject *vxlanObj = NULL;
   CmsRet ret;

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_DEV2_VXLAN, &iidStack,
               OGF_NO_VALUE_UPDATE, (void **) &vxlanObj)) == CMSRET_SUCCESS)
   {
   	  *num = vxlanObj->X_BROADCOM_COM_TunnelIfNumOfEntries;
      cmsObj_free((void **) &vxlanObj);
   }
   else
   {
      cmsLog_error("get Dev2VxlanObject failed, ret=%d.", ret);
   }
   return ret;
}

#endif /* SUPPORT_VXLAN_TUNNEL_TR181 */
