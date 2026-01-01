/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#include "cms_core.h"
#include "cms_util.h"
#include "qdm_intf.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_ip.h"
#include "rut_ipconcfg.h"
#include "rut2_iptunnel.h"
#include "rut2_dhcpv4.h"
#include "rut_wan.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
static void launchDhcpv4Client(const char *wanIpIntfPath)
{
   cmsLog_debug("enter: %s", wanIpIntfPath);

   if (cmsMdm_isDataModelDevice2())
   {
      /* pure 181 */
      SINT32 pid;
      char intfNameBuf[CMS_IFNAME_LENGTH]={0};
      char vid[BUFLEN_256], uid[BUFLEN_256];
      char iaid[BUFLEN_16], duid[BUFLEN_256];
      char ipAddr[BUFLEN_16], leasedTime[BUFLEN_16];
      char buf[BUFLEN_16], serverIpAddr[BUFLEN_16];
      UBOOL8 op125=FALSE;
      UBOOL8 dyn6rd;

      if (!rutDhcpv4_isClientEnabled_dev2(wanIpIntfPath))
      {
         return;
      }
      rutDhcpv4_stopClientByIpIntfFullPath_dev2(wanIpIntfPath);

      qdmIntf_fullPathToIntfnameLocked(wanIpIntfPath, intfNameBuf);

      memset(vid, 0, BUFLEN_256);
      memset(duid, 0, BUFLEN_256);
      memset(iaid, 0, BUFLEN_16);
      memset(uid, 0, BUFLEN_256);
      memset(buf, 0, BUFLEN_16);
      memset(ipAddr, 0, BUFLEN_16);
      memset(leasedTime, 0, BUFLEN_16);
      memset(serverIpAddr, 0, BUFLEN_16);

      qdmDhcpv4Client_getSentOption_dev2(wanIpIntfPath, 60, vid, BUFLEN_256);
      qdmDhcpv4Client_getSentOption_dev2(wanIpIntfPath, 61, duid, BUFLEN_256);
      qdmDhcpv4Client_getSentOption_dev2(wanIpIntfPath, 61, iaid, BUFLEN_8);
      qdmDhcpv4Client_getSentOption_dev2(wanIpIntfPath, 77, uid, BUFLEN_256);
      if (qdmDhcpv4Client_getSentOption_dev2(wanIpIntfPath, 125, buf, BUFLEN_16) == CMSRET_SUCCESS)
         op125 = atoi(buf);

      qdmDhcpv4Client_getReqOption_dev2(wanIpIntfPath, 50, ipAddr, BUFLEN_16);
      qdmDhcpv4Client_getReqOption_dev2(wanIpIntfPath, 51, leasedTime, BUFLEN_16);
      qdmDhcpv4Client_getReqOption_dev2(wanIpIntfPath, 54, serverIpAddr, BUFLEN_16);

      dyn6rd = rutTunnel_isDynamicTunnel(wanIpIntfPath, TRUE);

      if ((pid = rutWan_startDhcpc(intfNameBuf, 
                                   vid,
                                   duid,
                                   iaid,
                                   uid,
                                   op125,
                                   ipAddr,
                                   serverIpAddr,
                                   leasedTime,
                                   dyn6rd)) == CMS_INVALID_PID)
      {
         rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(wanIpIntfPath,
                                                           pid, MDMVS_ERROR);
      }
      else
      {
         rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(wanIpIntfPath,
                                                         pid, MDMVS_ENABLED);
      }
   }
   else
   {
      /* hybrid 181 */
      MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;

      if (cmsMdm_fullPathToPathDescriptor(wanIpIntfPath, &pathDesc) == CMSRET_SUCCESS)
      {
         WanIpConnObject *wanIpObj = NULL;

         if (cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **) &wanIpObj) == CMSRET_SUCCESS)
         {
            if (rutCfg_launchDhcpv4Client(wanIpObj) == CMSRET_SUCCESS)
            {
               if (cmsObj_set(wanIpObj, &pathDesc.iidStack) != CMSRET_SUCCESS)
               {
                  cmsLog_error("fail to set obj");
               }
            }
            else
            {
               cmsLog_error("launch dhcpc failed");
            }

            cmsObj_free((void **) &wanIpObj);
         }
         else
         {
            cmsLog_error("cannot get dhcpv4Client WAN obj");
         }
      }
      else
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s", wanIpIntfPath);
      }
   }

   return;
}


CmsRet rutTunnel_getTunneledWanInfo(const char *lanPath, const char *wanPath,
                                   char *wanIp, UBOOL8 *firewall, 
                                   char *ifname, UBOOL8 isIPv4)
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("lan<%s> wan<%s>", lanPath, wanPath);
   wanIp[0] = '\0';

   if (isIPv4)
   {
      char ifName[CMS_IFNAME_LENGTH];
      UBOOL8 wanUp;

      if ((ret = qdmIntf_fullPathToIntfnameLocked(wanPath, ifName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked failed. ret %d", ret);
         return ret;
      }
      
      cmsLog_debug("ifName<%s>", ifName);

      wanUp = qdmIpIntf_isWanInterfaceUpLocked(ifName, TRUE);
      if (wanUp)
      {
         ret = qdmIpIntf_getIpv4AddressByNameLocked(ifName, wanIp);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Cannot get intf: %s IP address", ifName);
         }
         *firewall = qdmIpIntf_isFirewallEnabledOnIntfnameLocked(ifName);
         cmsUtl_strncpy(ifname, ifName, CMS_IFNAME_LENGTH);
      }
   }
   else
   {
      MdmPathDescriptor pathDesc;
      UBOOL8 found_addr = FALSE;
      InstanceIdStack ipv6AddriidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
      Dev2IpInterfaceObject *ipIntfObj=NULL;

      /* 
       * Get the IP.Interface of WAN.
       * If the ServiceStatus is not up, return empty wanIp
       */
      INIT_PATH_DESCRIPTOR(&pathDesc);
      ret = cmsMdm_fullPathToPathDescriptor(wanPath, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", wanPath, ret);
         return ret;
      }

      if ((ret = cmsObj_get(MDMOID_DEV2_IP_INTERFACE, &pathDesc.iidStack, 0, (void **)&ipIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get fails");
         return ret;
      }

      cmsLog_debug("WAN ipv4Status<%s> ipv6Status<%s> firewall<%d>", 
                ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus,
                ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus,
                ipIntfObj->X_BROADCOM_COM_FirewallEnabled);


      if (cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP))
      {
         cmsLog_debug("IPv6 WAN is not up");
         cmsObj_free((void **) &ipIntfObj);
         goto exit;
      }
      *firewall = ipIntfObj->X_BROADCOM_COM_FirewallEnabled;
      cmsUtl_strncpy(ifname, ipIntfObj->name, CMS_IFNAME_LENGTH);
      cmsLog_debug("IPv6 WAN is up");

      while (!found_addr && 
             (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS, &pathDesc.iidStack, 
               &ipv6AddriidStack, OGF_NO_VALUE_UPDATE, (void **)&ipv6AddrObj) == CMSRET_SUCCESS))
      {
         if (ipv6AddrObj->enable && 
             !cmsUtl_strcmp(ipv6AddrObj->status, MDMVS_ENABLED) &&
             cmsUtl_isValidIpAddress(AF_INET6, ipv6AddrObj->IPAddress))
         {
            cmsUtl_strncpy(wanIp, ipv6AddrObj->IPAddress, CMS_IPADDR_LENGTH);
            found_addr = TRUE;
         }

         cmsObj_free((void **) &ipv6AddrObj);
      }

      /* 
       * RFC 6333 says we must take LAN address for 4in6 tunnel if
       * WAN address is not available
       */
      if (!found_addr)
      {
         MdmPathDescriptor lanpathDesc;

         cmsLog_debug("No WAN IPv6 address found");

         INIT_PATH_DESCRIPTOR(&lanpathDesc);
         ret = cmsMdm_fullPathToPathDescriptor(lanPath, &lanpathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", wanPath, ret);
            cmsObj_free((void **) &ipIntfObj);
            goto exit;
         }

         INIT_INSTANCE_ID_STACK(&ipv6AddriidStack);
         
         while (!found_addr && 
                (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS, &lanpathDesc.iidStack, 
                  &ipv6AddriidStack, OGF_NO_VALUE_UPDATE, (void **)&ipv6AddrObj) == CMSRET_SUCCESS))
         {
            /* Address from delegated prefix is always MDMVS_AUTOCONFIGURED */
            if (ipv6AddrObj->enable && 
                !cmsUtl_strcmp(ipv6AddrObj->status, MDMVS_ENABLED) &&
                !cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_AUTOCONFIGURED) &&
                cmsUtl_isValidIpAddress(AF_INET6, ipv6AddrObj->IPAddress))
            {
               cmsUtl_strncpy(wanIp, ipv6AddrObj->IPAddress, CMS_IPADDR_LENGTH);
               found_addr = TRUE;
            }

            cmsObj_free((void **) &ipv6AddrObj);
         }
      }
      
      cmsObj_free((void **) &ipIntfObj);
   }

exit:
   cmsLog_debug("wanIp<%s> firewall<%d>", wanIp, *firewall);

   return CMSRET_SUCCESS;
}

CmsRet rutTunnel_getAftrAddress(const char *ipaddr, const char *aftr, char *ipstr)
{
   char addrStr[CMS_IPADDR_LENGTH];

   if (IS_EMPTY_STRING(ipaddr) && IS_EMPTY_STRING(aftr))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (!IS_EMPTY_STRING(ipaddr)) /* static configuration */
   {
      cmsUtl_strncpy(addrStr, ipaddr, CMS_AFTR_NAME_LENGTH);
   }
   else /* aftr from dhcpv6 */
   {
      /* If the AFTR info is domain name, we need to get address resolved */
      if (cmsUtl_isValidIpAddress(AF_INET6, aftr) == FALSE)
      {
         struct addrinfo hints, *res;
         int status;

         memset(&hints, 0, sizeof hints);
         hints.ai_family = AF_INET6;
         hints.ai_socktype = SOCK_STREAM;
 
         if ((status = getaddrinfo(aftr, NULL, &hints, &res)) != 0) 
         {
            cmsLog_error("getaddrinfo: %s", gai_strerror(status));
            return CMSRET_INVALID_PARAM_VALUE;
         }
         else
         {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
 
            inet_ntop(res->ai_family, &(ipv6->sin6_addr), addrStr, INET6_ADDRSTRLEN);
            freeaddrinfo(res);
         }
      }
      else
      {
         cmsUtl_strncpy(addrStr, aftr, CMS_IPADDR_LENGTH);
      }
   }

   if (cmsUtl_isValidIpAddress(AF_INET6, addrStr) == FALSE)
   {
      cmsLog_error("invald address: %s", addrStr);
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsUtl_strncpy(ipstr, addrStr, CMS_IPADDR_LENGTH);

   return CMSRET_SUCCESS;
}


void rutTunnel_dsliteControl(const char *ipIntfFullPath)
{
   UBOOL8 found = FALSE;
   Dev2DsliteInterfaceSettingObject *obj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("ipIntfPath: %s", ipIntfFullPath);

   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_DSLITE_INTERFACE_SETTING, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&obj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_TunneledInterface, ipIntfFullPath))
      {
         found = TRUE;
      }
      else
      {
         cmsObj_free((void **)&obj);
      }
   }

   cmsLog_debug("found<%d>", found);

   if (found)
   {
      if (cmsObj_set(obj, &iidStack) != CMSRET_SUCCESS)
      {
         cmsLog_error("fail set dsliteObj");
      }

      cmsObj_free((void **)&obj);
   }

   return;
}


UBOOL8 rutTunnel_isDynamicTunnel(const char *wanIpIntfPath, UBOOL8 is6rd)
{
   UBOOL8 ret = FALSE;
   UBOOL8 found = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("wanIpIntfPath<%s> is6rd<%d>", wanIpIntfPath, is6rd);

   if (is6rd)
   {
      Dev2Ipv6rdInterfaceSettingObject *obj;

      while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6RD_INTERFACE_SETTING, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&obj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_TunneledInterface, wanIpIntfPath))
         {
            found = TRUE;
            ret = obj->X_BROADCOM_COM_Dynamic;
         }

         cmsObj_free((void **)&obj);
      }
   }
   else
   {
      Dev2DsliteInterfaceSettingObject *obj;

      while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_DSLITE_INTERFACE_SETTING, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&obj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_TunneledInterface, wanIpIntfPath))
         {
            found = TRUE;

            if (!cmsUtl_strcmp(obj->origin, MDMVS_DHCPV6))
            {
               ret = TRUE;
            }
         }

         cmsObj_free((void **)&obj);
      }
   }

   cmsLog_debug("found<%d> isdynamic<%d>", found, ret);

   return ret;
}


void rutTunnel_restartDhcpClientForTunnel(const char *wanIpIntfPath, UBOOL8 is6rd)
{
   if (wanIpIntfPath == NULL)
   {
      cmsLog_error("null path");
      return;
   }

   cmsLog_debug("wanIpIntfPath<%s> is6rd<%d>", wanIpIntfPath, is6rd);

   if (is6rd)
   {
      launchDhcpv4Client(wanIpIntfPath);
   }
   else
   {
      char ifname[CMS_IFNAME_LENGTH];
      CmsRet ret=CMSRET_SUCCESS;
      if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(wanIpIntfPath, ifname)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. ret %d", ret);
         return;
      }
      cmsLog_debug("ifname<%s>", ifname);

      rutIpv6Service_launchDhcp6c(wanIpIntfPath, ifname);
   }

   return;
}


void rutTunnel_ipv6rdControl(const char *ipIntfFullPath)
{
   UBOOL8 found = FALSE;
   Dev2Ipv6rdInterfaceSettingObject *obj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("ipIntfPath: %s", ipIntfFullPath);

   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6RD_INTERFACE_SETTING, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&obj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_TunneledInterface, ipIntfFullPath))
      {
         found = TRUE;
      }
      else
      {
         cmsObj_free((void **)&obj);
      }
   }

   cmsLog_debug("found<%d>", found);

   if (found)
   {
      if (cmsObj_set(obj, &iidStack) != CMSRET_SUCCESS)
      {
         cmsLog_error("fail set ipv6rd object");
      }

      cmsObj_free((void **)&obj);
   }

   return;
}


void rutTunnel_getLanAddrForDsLite_dev2( const char *wanintf, char *ipaddr )
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack iidStack_ipv6Prefix;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *prefix = NULL;
   char *prefixfullpath = NULL;
   UBOOL8 found = FALSE;

   cmsLog_debug("wanintf<%s>", wanintf);

   if (!ipaddr)
   {
      cmsLog_error("NULL ipaddr");
      return;
   }
   else
   {
      ipaddr[0] = '\0';
   }

   /* 
    * wanintf -> WAN IP.Interface.Prefix (static/PD) ->
    * LAN IP.Interface.Prefix (parent match) ->
    * LAN IP.Interface.Address (prefix match)
    */
   if (qdmIntf_getPathDescFromIntfnameLocked_dev2(wanintf, FALSE, &pathDesc)
           != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot get wanintf<%s> pathdesc", wanintf);
      return;
   }

   if (!qdmIpIntf_findIpv6Prefix(&pathDesc.iidStack, NULL, MDMVS_STATIC,
                           MDMVS_PREFIXDELEGATION, &iidStack_ipv6Prefix))
   {
      cmsLog_notice("no PD prefix found");
      return;
   }
   else
   {
      MdmPathDescriptor prefixPathDesc=EMPTY_PATH_DESCRIPTOR;

      prefixPathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
      prefixPathDesc.iidStack = iidStack_ipv6Prefix;

      if (cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPathDesc, &prefixfullpath) != CMSRET_SUCCESS)
      {
         cmsLog_error("pathDescToFullPath failed!");
         return;
      }
   }

   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&prefix) == CMSRET_SUCCESS))
   {
      if (prefix->enable && 
          !cmsUtl_strcmp(prefix->status, MDMVS_ENABLED) &&
          !cmsUtl_strcmp(prefix->origin, MDMVS_STATIC) &&
          !cmsUtl_strcmp(prefix->staticType, MDMVS_CHILD) &&
          !cmsUtl_strcmp(prefix->parentPrefix, prefixfullpath))
      {
         found = TRUE;
      }

      cmsObj_free((void **) &prefix);
   }

   if (found)
   {
      Dev2Ipv6AddressObject *addrObj = NULL;
      InstanceIdStack addrIidStack = EMPTY_INSTANCE_ID_STACK;
      MdmPathDescriptor prefixPathDesc=EMPTY_PATH_DESCRIPTOR;

      CMSMEM_FREE_BUF_AND_NULL_PTR(prefixfullpath);
      prefixPathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
      prefixPathDesc.iidStack = iidStack;

      if (cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPathDesc, &prefixfullpath) != CMSRET_SUCCESS)
      {
         cmsLog_error("pathDescToFullPath failed!");
         return;
      }

      found = FALSE;

      while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_ADDRESS, &addrIidStack,
                       OGF_NO_VALUE_UPDATE, (void **)&addrObj) == CMSRET_SUCCESS))
      {
         if (addrObj->enable && 
             !cmsUtl_strcmp(addrObj->status, MDMVS_ENABLED) &&
             !cmsUtl_strcmp(addrObj->prefix, prefixfullpath))
         {
            found = TRUE;
            cmsUtl_strncpy(ipaddr, addrObj->IPAddress, CMS_IPADDR_LENGTH);
         }

         cmsObj_free((void **) &addrObj);
      }
   }
   else
   {
      cmsLog_notice("cannot find associated child prefix");
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(prefixfullpath);

   cmsLog_debug("ipaddr<%s>", ipaddr);
}

#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

