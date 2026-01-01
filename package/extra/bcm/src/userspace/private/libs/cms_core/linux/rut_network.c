/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_iptables.h"
#include "rut_ethswitch.h"
#include "rut_network.h"
#include "rut_dnsproxy.h"
#include "rut_pmap.h"

CmsRet rutNtwk_configActiveDnsIp(void)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered");

   rutLan_updateDhcpd();


   /* Will always creates Resolv configuration even if dns1 is '0.0.0.0'.
    * advanced dmz IS NOT IMPLEMENTED!!!  Sean TODO!!
    */     
   rutLan_createResolvCfg();
   
   rutDpx_updateDnsproxy();

   return ret;

}




#ifdef  DMP_BASELINE_1

CmsRet rutNtwk_doSystemDns(UBOOL8 isIPv4)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("Enter: isIPv4=%d", isIPv4);

   if (isIPv4)
   {
      _NetworkConfigObject *networkCfg=NULL;
      char activeDNSServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
      char activeDnsIfName[CMS_IFNAME_LENGTH]={0};

      if ((ret = cmsObj_get(MDMOID_NETWORK_CONFIG, &iidStack, OGF_NO_VALUE_UPDATE, (void *) &networkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get NETWORK_CONFIG. ret=%d", ret);
         return ret;
      }   

      /* Select Active IPv4 DNS Servers */
      rutNtwk_selectActiveIpvxDnsServers(CMS_AF_SELECT_IPV4,
                                         networkCfg->DNSIfName,
                                         networkCfg->DNSServers,
                                         activeDnsIfName, activeDNSServers);
      
      if (cmsUtl_strcmp(networkCfg->activeDNSServers, activeDNSServers))
      {
         /* set active dns ip if differs with the previous one */
         CMSMEM_REPLACE_STRING_FLAGS(networkCfg->activeDNSServers, activeDNSServers, mdmLibCtx.allocFlags);
      }

      if ((ret = cmsObj_set(networkCfg, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set MDMOID_NETWORK_CONFIG returns error. ret=%d", ret);
      }      
      cmsObj_free((void **) &networkCfg);
   }

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   else
   {
      _IPv6LanHostCfgObject *IPv6LanCfgObj=NULL;

      if ((ret = cmsObj_getNext(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack,  (void **)&IPv6LanCfgObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
         return ret;
      }

      /* TODO: We should follow the same logic as IPv4 dns */
      if ((ret = cmsObj_set(IPv6LanCfgObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
      }

      cmsObj_free((void **) &IPv6LanCfgObj);

   }
#endif

   cmsLog_debug("Exit. ret=%d", ret);
   
   return ret;
   
}


CmsRet rutNtwk_getIpv4DnsServersFromIfName_igd(const char *ifName, char *DNSServers)
{
   return (rutNtwk_getIpvxDnsServersFromIfName_igd(CMS_AF_SELECT_IPV4,
                                                   ifName, DNSServers));
}


CmsRet rutNtwk_getIpvxDnsServersFromIfName_igd(UINT32 ipvx, const char *ifName, char *DNSServers)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *pppConn = NULL;
   _WanIpConnObject  *ipConn = NULL;
   LanIpIntfObject *lanIpObj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret;
   
   cmsLog_debug("Entered: ipvx=%d ifName=%s", ipvx, ifName);

   if (IS_EMPTY_STRING(ifName) || !DNSServers)
   {
      cmsLog_error("NULL string.");
      return CMSRET_INTERNAL_ERROR;
   }
   DNSServers[0] = '\0';

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   /* In Hybrid mode, look for IPv6 on _dev2 side */
   if (ipvx & CMS_AF_SELECT_IPV6)
   {
      ret = rutNtwk_getIpvxDnsServersFromIfName_dev2(CMS_AF_SELECT_IPV6, ifName, DNSServers);
      if (ret == CMSRET_SUCCESS)
      {
         return ret;
      }
   }
#endif

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName))
      {
         found = TRUE;
#ifdef DMP_X_BROADCOM_COM_IPV6_1
         if ((ipvx & CMS_AF_SELECT_IPV6) &&
             !IS_EMPTY_STRING(ipConn->X_BROADCOM_COM_IPv6DNSServers))
         {
            cmsUtl_strncpy(DNSServers, ipConn->X_BROADCOM_COM_IPv6DNSServers,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
#endif
         if (DNSServers[0] == '\0' &&
             (ipvx & CMS_AF_SELECT_IPV4) &&
             (!IS_EMPTY_STRING(ipConn->DNSServers)))
         {
            cmsUtl_strncpy(DNSServers, ipConn->DNSServers,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
      }
      
      cmsObj_free((void **) &ipConn);
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&pppConn) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, pppConn->X_BROADCOM_COM_IfName))
      {
         found = TRUE;
#ifdef DMP_X_BROADCOM_COM_IPV6_1
         if ((ipvx & CMS_AF_SELECT_IPV6) &&
             !IS_EMPTY_STRING(pppConn->X_BROADCOM_COM_IPv6DNSServers))
         {
            cmsUtl_strncpy(DNSServers, pppConn->X_BROADCOM_COM_IPv6DNSServers,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
#endif
         if (DNSServers[0] == '\0' &&
             (ipvx & CMS_AF_SELECT_IPV4) &&
             (!IS_EMPTY_STRING(pppConn->DNSServers)))
         {
            cmsUtl_strncpy(DNSServers, pppConn->DNSServers,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
      }
      cmsObj_free((void **) &pppConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&lanIpObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, lanIpObj->X_BROADCOM_COM_IfName))
      {
         found = TRUE;
#ifdef DMP_X_BROADCOM_COM_IPV6_1
         if (ipvx & CMS_AF_SELECT_IPV6)
         {
            cmsLog_error("No support for dynamic DNS in old IPv6 LAN side");
         }
#endif
         if (DNSServers[0] == '\0' &&
             (ipvx & CMS_AF_SELECT_IPV4) &&
             (!IS_EMPTY_STRING(lanIpObj->X_BROADCOM_COM_DNSServers)))
         {
            cmsUtl_strncpy(DNSServers, lanIpObj->X_BROADCOM_COM_DNSServers,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
      }

      cmsObj_free((void **)&lanIpObj);
   }

   if (found && DNSServers[0] != '\0')
   {
      cmsLog_debug("Got DNSServers %s.", DNSServers);
      ret = CMSRET_SUCCESS;
   }
   else
   {
      cmsLog_debug("No DNSServers got from %s.", ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
   
}

#endif  /* DMP_BASELINE_1 */


CmsRet rutNtwk_selectActiveIpvxDnsServers(UINT32 ipvx,
                                          const char *dnsIfNameList,
                                          const char *staticDnsServers,
                                          char *activeDnsIfName,
                                          char *activeDnsServers)
{
   UBOOL8 found=FALSE;

   cmsLog_debug("Enter: ipvx=%d",ipvx);

   if ((ipvx & CMS_AF_SELECT_IPVX) == CMS_AF_SELECT_IPVX)
   {
      cmsLog_error("must specify either IPv4 or IPv6 (not both)");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (!activeDnsIfName)
   {
      cmsLog_error("activeDnsIfName is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }
   activeDnsIfName[0] = '\0';

   if (!activeDnsServers)
   {
      cmsLog_error("activeDnsServers is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }
   activeDnsServers[0] = '\0';


   if (!IS_EMPTY_STRING(staticDnsServers))
   {
      /*
       * statically configured DNS has the highest precedence.  If static
       * DNS servers are given, use it.
       */
      cmsUtl_strncpy(activeDnsServers, staticDnsServers, (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
      cmsUtl_strncpy(activeDnsIfName, CMS_FAKE_STATIC_DNS_IFNAME, CMS_IFNAME_LENGTH);
      cmsLog_notice("Use default static DNS %s", activeDnsServers);
      return CMSRET_SUCCESS;
   }


   if (dnsIfNameList)
   {
      /* Look for first UP/connected interface in the dns name list */
      
      char *dnsList, *newDnsIfName, *ptr, *savePtr=NULL;
      UINT32 count=0;
      UBOOL8 isIpv4 = (ipvx & CMS_AF_SELECT_IPV4);
      
      dnsList = cmsMem_strdup(dnsIfNameList);
      ptr = strtok_r(dnsList, ",", &savePtr);
      
      while (!found && (ptr != NULL) && (count < CMS_MAX_DNSIFNAME))
      {
         newDnsIfName=ptr;
         while ((isspace(*newDnsIfName)) && (*newDnsIfName != 0))
         {
            /* skip white space after comma */
            newDnsIfName++;
         }         
         cmsLog_debug("checking ifname %s (isIPv4=%d)", newDnsIfName, isIpv4);

         if (qdmIpIntf_isWanInterfaceUpLocked(newDnsIfName, isIpv4))
         {
            found = TRUE;
            strcpy(activeDnsIfName, newDnsIfName);
            cmsLog_debug("found UP wan ifname %s", activeDnsIfName);
         }
         
         count++;
         ptr = strtok_r(NULL, ",", &savePtr);
      }

      cmsMem_free(dnsList);
   }

   if (!found)
   {
      /*
       * In the case we don't find any UP/connected WAN interface from the
       * dnsIfNameList, try to use any WAN interface that is UP/connected.
       */
      found = rutWan_findFirstIpvxRoutedAndConnected(ipvx, activeDnsIfName);
   }


   if (found)
   {
      /* Get DNS ip from activeDnsIfName */
      if (rutNtwk_getIpvxDnsServersFromIfName(ipvx, activeDnsIfName, activeDnsServers) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Found active dns ip %s.", activeDnsServers);
      }
      else
      {
         cmsLog_debug("Fail to get dns ip from %s.", activeDnsIfName);
         activeDnsIfName[0] = '\0';
         found = FALSE;
      }
   }
   else
   {
      /*
       * No DNS servers found (no WAN services up), so return empty set of
       * DNS servers, i.e. 0.0.0.0, 0.0.0.0
       */
       strcpy(activeDnsServers, "0.0.0.0,0.0.0.0");
       cmsLog_debug("No DNS servers yet, activeDnsServers=%s", activeDnsServers);
   }

   cmsLog_debug("Exit: found=%d activeDnsIfName=%s activeDnsServers=%s",
                found, activeDnsIfName, activeDnsServers);

   return CMSRET_SUCCESS;
}


void rutNtwk_removeIfNameFromList(const char *ifName, char *ifNameList)
{
   char *tmpList, *newList;
   char *currIfName, *ptr, *savePtr=NULL;

   cmsLog_debug("Enter: ifName=%s ifNameList=%s", ifName, ifNameList);

   if (IS_EMPTY_STRING(ifName) || IS_EMPTY_STRING(ifNameList))
   {
      /* nothing to do */
      return;
   }

   tmpList = cmsMem_strdup(ifNameList);
   newList = cmsMem_strdup(ifNameList);
   if (!tmpList || !newList)
   {
      cmsLog_error("Memory allocation failure");
      cmsMem_free(tmpList);
      cmsMem_free(newList);
      return;
   }

   memset(newList, 0, strlen(newList));

   ptr = strtok_r(tmpList, ",", &savePtr);

   while (ptr != NULL)
   {
      currIfName=ptr;
      while ((isspace(*currIfName)) && (*currIfName != 0))
      {
         /* skip white space after comma */
         currIfName++;
      }

      /* Only copy it back if not matched with the ifName */
      if (cmsUtl_strcmp(ifName, currIfName))
      {
         if (newList[0] != '\0')
         {
            strcat(newList, ",");
         }
         strcat(newList, currIfName);
      }
      else
      {
         cmsLog_debug("%s is removed from ifNameList", ifName);
      }

      ptr = strtok_r(NULL, ",", &savePtr);
   }

   /* copy the newList to the caller's list (modify caller's list).
    * newList is guaranteed to be shorter than the original ifNameList
    */
   sprintf(ifNameList, "%s", newList);

   cmsMem_free(tmpList);
   cmsMem_free(newList);

   return;
}


#ifdef DMP_BASELINE_1

void rutNtwk_removeIpvxDnsIfNameFromList_igd(UINT32 ipvx, const char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter: ipvx=%d ifName=%s", ipvx, ifName);

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (ipvx & CMS_AF_SELECT_IPV6)
   {
      /* in Hybrid mode, we have to do the delete on the _dev side */
      rutNtwk_removeIpvxDnsIfNameFromList_dev2(CMS_AF_SELECT_IPV6, ifName);
   }
#endif

   if (ipvx & CMS_AF_SELECT_IPV4)
   {
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
      _NetworkConfigObject *networkCfg=NULL;

      if ((ret = cmsObj_get(MDMOID_NETWORK_CONFIG, &iidStack, OGF_NO_VALUE_UPDATE, (void *) &networkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get NETWORK_CONFIG. ret=%d", ret);
         return;
      }

      if (IS_EMPTY_STRING(networkCfg->DNSIfName) || IS_EMPTY_STRING(ifName))
      {
         cmsLog_debug("networkCfg->DNSIfName or ifName is NULL, do nothing");
      }
      else if (!strstr(networkCfg->DNSIfName, ifName))
      {
         cmsLog_debug("ifName %s is not in list %s, do nothing", ifName, networkCfg->DNSIfName);
      }
      else
      {
         rutNtwk_removeIfNameFromList(ifName, networkCfg->DNSIfName);

         if ((ret = cmsObj_set(networkCfg, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <NETWORK_CONFIG> returns error. ret=%d", ret);
         }
      }

      cmsObj_free((void **) &networkCfg);
   }

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   if (ipvx & CMS_AF_SELECT_IPV6)
   {
      InstanceIdStack iidStack1=EMPTY_INSTANCE_ID_STACK;
      _IPv6LanHostCfgObject *ipv6LanCfgObj=NULL;

      if ((ret = cmsObj_getNext(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack1, (void **)&ipv6LanCfgObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
         return;
      }

      if (IS_EMPTY_STRING(ipv6LanCfgObj->IPv6DNSWANConnection) &&
          !cmsUtl_strcmp(ipv6LanCfgObj->IPv6DNSConfigType, MDMVS_DHCP))
      {
         cmsLog_debug("Currently is static DNS - No action.");
      }
      else
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6LanCfgObj->IPv6DNSWANConnection);

         if ((ret = cmsObj_set(ipv6LanCfgObj, &iidStack1)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
         }
      }

      cmsObj_free((void **) &ipv6LanCfgObj);
   }
#endif

   cmsLog_debug("Exit");
   
   return;
}
#endif  /* DMP_BASELINE_1 */



CmsRet rutNtwk_RestoreDefaultDhcpConfig(void)
{
  LanHostCfgObject *lanHostCfg = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("enter");

   /* the system should always have a LAN_HOST_CFG instance to get from */
   if ((ret = cmsObj_getNext(MDMOID_LAN_HOST_CFG,
                             &iidStack,
                             (void *) &lanHostCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get current lanHostCfg, ret=%d", ret);
      return ret;
   }
   
   if ((ret = cmsObj_set(lanHostCfg, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set lanHostCfgt, ret=%d", ret);
   }

   cmsObj_free((void **) &lanHostCfg);

   return ret;

}


#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1

void rutNtwk_getDomainName(char **domainName)
{
   DnsProxyCfgObject *dnsProxyObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &iidStack, 0, (void **) &dnsProxyObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get dnsproxy cfg object, ret=%d", ret);
      return;
   }

   *domainName = cmsMem_strdupFlags(dnsProxyObj->deviceDomainName, mdmLibCtx.allocFlags);

   cmsObj_free((void **) &dnsProxyObj);

   return;
}

#endif  /* DMP_X_BROADCOM_COM_DNSPROXY_1 */
