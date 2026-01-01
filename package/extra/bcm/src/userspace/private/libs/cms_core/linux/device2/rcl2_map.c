/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
#ifdef SUPPORT_MAP

#include "rcl.h"
#include "rut_ebtables.h"
#include "rut_iptables.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut2_iptunnel.h"
#include "rut2_map.h"
#include "rut2_ra.h"
#include <sys/utsname.h>

#define MAP_MODULE_NAME             "nat46"
#define MAP_PROC_DEVICE_NAME        "/proc/net/nat46/control"

#define MAP_TUNNEL_NAME             "bmr1"

extern UBOOL8 isModuleInserted(char *moduleName);

static UBOOL8 isMapModuleInserted(void)
{
   UBOOL8 isInserted = FALSE;

   if (access(MAP_PROC_DEVICE_NAME, F_OK) == 0)
   {
      isInserted = TRUE;
   }

   cmsLog_debug("%s is %s", MAP_MODULE_NAME, isInserted ? "in." : "not in.");

   return isInserted;
}

static void insertMapModule()
{
   char cmdStr[CMS_MAX_FULLPATH_LENGTH];
   struct utsname kernel;

   if (uname(&kernel) == -1)
   {
      cmsLog_error("Failed to get kernel version");
      return;
   }

#ifdef DESKTOP_LINUX
   cmsLog_debug("fake insert of Map module");
#else

   if (!isModuleInserted("xt_nat"))
   {
      insertModuleByName("xt_nat");
   }

   if (!isModuleInserted("xt_connlimit"))
   {
      insertModuleByName("xt_connlimit");
   }

   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/extra/%s.ko zero_csum_pass=1", kernel.release, MAP_MODULE_NAME);
   rut_doSystemAction("insertMapModule", cmdStr);

#endif /* DESKTOP_LINUX */

   return;
}

static CmsRet launchMap(const char *brprefix, const char *wanintf,
                         UINT32 psidOffset, UINT32 psidLen, UINT32 psid, UINT32 eaLen,
                         const char *IPv6Prefix, const char *ipv6AddrBuf,
                         const char *subnetCidr4, const char *addr,
                         UBOOL8 mapt, UBOOL8 add)
{
   CmsRet ret = CMSRET_SUCCESS;
   char callerStr[BUFLEN_24];
   char cmdStr[BUFLEN_512];
   char ifNameBuf[CMS_IFNAME_LENGTH]={0};
   PortSet *ps = NULL;

   cmsLog_debug("%s: map<%c> brprefix<%s> wanintf<%s> BMRv6Prefix<%s>", add?"ADD":"DEL", mapt?'T':'E', brprefix, wanintf, IPv6Prefix);

   /* allocate a set of ranges for available port # */
   if ((ret = rutMap_newPortSetRange(psidOffset, psidLen, psid, &ps)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutMap_newPortSetRange fail");
      return ret;
   }

   if ( add )
   {
      snprintf(callerStr, sizeof(callerStr), "Map%cAdd", mapt?'t':'e');
      rutIpt_updateSNATMapRules(add, callerStr, MAP_TUNNEL_NAME, addr, ps);

      if ( mapt )
      {
         if (!isMapModuleInserted())
         {
            insertMapModule();
         }

         snprintf(cmdStr, sizeof(cmdStr), "echo add %s > %s", MAP_TUNNEL_NAME, MAP_PROC_DEVICE_NAME);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "echo config %s local.style MAP local.v4 %s local.v6 %s local.ea-len %d local.psid-offset %d "
                                          "remote.v4 0.0.0.0/0 remote.v6 %s remote.style RFC6052 remote.ea-len 0 remote.psid-offset 0 "
                                          "> %s", MAP_TUNNEL_NAME, subnetCidr4, IPv6Prefix, eaLen, psidOffset, brprefix, MAP_PROC_DEVICE_NAME);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "ip link set dev %s up", MAP_TUNNEL_NAME);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "ip ro add default dev %s", MAP_TUNNEL_NAME);
         rut_doSystemAction(callerStr, cmdStr);
      
         snprintf(cmdStr, sizeof(cmdStr), "ip -6 ro add %s dev %s", ipv6AddrBuf, MAP_TUNNEL_NAME);
         rut_doSystemAction(callerStr, cmdStr);
      }
      else
      {
         char *p;
         SINT32 mtuSize;

         if ((p = strchr(brprefix, '/')) != NULL)
         {
            *p = '\0';
         }

         if ((mtuSize = rutWan_getInterfaceMTU(AF_INET6, wanintf)) <= 0)
         {
            mtuSize = 1500;
         }

         snprintf(cmdStr, sizeof(cmdStr), "ip -6 tunnel add %s mode ip4ip6 remote %s local %s encaplimit none", MAP_TUNNEL_NAME, brprefix, ipv6AddrBuf);
         rut_doSystemAction(callerStr, cmdStr);

         /* tunnel MTU = WAN MTU - ipv6 header */
         snprintf(cmdStr, sizeof(cmdStr), "ip link set dev %s mtu %d up", MAP_TUNNEL_NAME, mtuSize-40);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "ip -6 addr add %s dev %s", ipv6AddrBuf, MAP_TUNNEL_NAME);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "ip ro add default dev %s", MAP_TUNNEL_NAME);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "ip addr add %s dev %s", addr, MAP_TUNNEL_NAME);
         rut_doSystemAction(callerStr, cmdStr);
      }

      qdmIpIntf_getDefaultLanIntfNameLocked_dev2(ifNameBuf);
      if (!IS_EMPTY_STRING(ifNameBuf) && !IS_EMPTY_STRING(IPv6Prefix))
      {
         rutIpt_configRoutingChain6(IPv6Prefix, ifNameBuf, TRUE);
         rutEbt_configIPv6Prefix(IPv6Prefix, TRUE);
      }

      rutRa_updateAllRouterAdvObj();
   }
   else /* case of delete */
   {
      if (rut_isDeviceFound(MAP_TUNNEL_NAME))
      {
         snprintf(callerStr, sizeof(callerStr), "Map%cDel", mapt?'t':'e');
         rutIpt_updateSNATMapRules(add, callerStr, MAP_TUNNEL_NAME, addr, ps);

         /* clear conntracks for IP's of addr and ipv6AddrBuf */
#ifdef SUPPORT_CONNTRACK_TOOLS
         snprintf(cmdStr, sizeof(cmdStr), "conntrack -D -d %s > /dev/null 2>&1", addr);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "conntrack -D -q %s > /dev/null 2>&1", addr);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "conntrack -D -d %s > /dev/null 2>&1", ipv6AddrBuf);
         rut_doSystemAction(callerStr, cmdStr);

         snprintf(cmdStr, sizeof(cmdStr), "conntrack -D -q %s > /dev/null 2>&1", ipv6AddrBuf);
         rut_doSystemAction(callerStr, cmdStr);
#endif

         if ( !mapt )
         {
            snprintf(cmdStr, sizeof(cmdStr), "ip ro del default dev %s", MAP_TUNNEL_NAME);
            rut_doSystemAction(callerStr, cmdStr);

            snprintf(cmdStr, sizeof(cmdStr), "ip link set dev %s down", MAP_TUNNEL_NAME);
            rut_doSystemAction(callerStr, cmdStr);

            snprintf(cmdStr, sizeof(cmdStr), "ip -6 tunnel del %s", MAP_TUNNEL_NAME);
            rut_doSystemAction(callerStr, cmdStr);
         }
         else
         {
            if (isMapModuleInserted())
            {
               snprintf(cmdStr, sizeof(cmdStr), "echo del %s > %s", MAP_TUNNEL_NAME, MAP_PROC_DEVICE_NAME);
               rut_doSystemAction(callerStr, cmdStr);

               snprintf(cmdStr, sizeof(cmdStr), "rmmod %s", MAP_MODULE_NAME);
               rut_doSystemAction(callerStr, cmdStr);
            }
         }

         qdmIpIntf_getDefaultLanIntfNameLocked_dev2(ifNameBuf);
         if (!IS_EMPTY_STRING(ifNameBuf) && !IS_EMPTY_STRING(IPv6Prefix))
         {
            rutIpt_configRoutingChain6(IPv6Prefix, ifNameBuf, FALSE);
            rutEbt_configIPv6Prefix(IPv6Prefix, FALSE);
         }

         rutRa_updateAllRouterAdvObj();
      }
   }

   rutMap_deletePortSetRange(ps);

   return ret;
}

CmsRet rcl_dev2MapObject( _Dev2MapObject *newObj,
                const _Dev2MapObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2MapDomainObject( _Dev2MapDomainObject *newObj,
                const _Dev2MapDomainObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (newObj && newObj->enable)
   {
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }
   else if (newObj && !newObj->enable)
   {
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2MapDomainRuleObject( _Dev2MapDomainRuleObject *newObj,
                const _Dev2MapDomainRuleObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char mechanism[BUFLEN_24];
   char brprefix[CMS_IPADDR_LENGTH];
   char wanintf[CMS_IFNAME_LENGTH];
   char ipv6AddrBuf[CMS_IPADDR_LENGTH];
   char subnetCidr4[CMS_IPADDR_LENGTH];
   char addr[CMS_IPADDR_LENGTH];
   UINT32 psidOffset, psidLen, psid, ipv4MaskLen;
   UBOOL8 firewall, isMapt;

   if ( ADD_NEW(newObj, currObj) )
   {
      return ret;
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /*
       * By default, orig is set to static. When the first time this object 
       * is configured, orig will be set to DHCP if IPv6Prefix is null.
       * Once orig is set to DHCP, the orig will not change any more.
       */
      if (cmsUtl_strcmp(currObj->origin, MDMVS_DHCPV6) != 0)
      {
         if (newObj->IPv6Prefix)
         {
            if (!cmsUtl_isValidIpAddress(AF_INET6, newObj->IPv6Prefix))
            {
               cmsLog_error("invalid static BMR IPv6 prefix <%s>", newObj->IPv6Prefix);
               return CMSRET_INVALID_ARGUMENTS;
            }

            CMSMEM_REPLACE_STRING_FLAGS(newObj->origin, MDMVS_STATIC, mdmLibCtx.allocFlags);
         }
         else
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->origin, MDMVS_DHCPV6, mdmLibCtx.allocFlags);

            /*
             * if configure dynamic, need to re-launch dhcp6c to request mapt or mape
             */
            if (cmsUtl_strcmp(currObj->origin, MDMVS_DHCPV6) != 0)
            {
               InstanceIdStack iidStack_domain;
               Dev2MapDomainObject *domain = NULL;

               iidStack_domain = *iidStack;
               if (cmsObj_getAncestorFlags(MDMOID_DEV2_MAP_DOMAIN, MDMOID_DEV2_MAP_DOMAIN_RULE,
                                           &iidStack_domain, OGF_NO_VALUE_UPDATE,
                                           (void **) &domain) != CMSRET_SUCCESS )
               {
                  cmsLog_error("Cannot get domain obj");
                  return CMSRET_MDM_TREE_ERROR;
               }

               rutTunnel_restartDhcpClientForTunnel(domain->WANInterface, FALSE);

               cmsLog_debug("dynamic MAP-%c restarted dhcpv6", *domain->transportMode);

               cmsObj_free((void **) &domain);

               return ret;
            }
         }
      }
   }

   /* Fetch the associated domain information */
   if ((ret = rutMap_getDomainInfo(iidStack, mechanism, brprefix, wanintf, &firewall, &psidOffset,
                            &psidLen, &psid)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutMap_getDomainInfo fail");
      return ret;
   }

   cmsLog_debug( "Configuring MAP-%c: OldStatus<%s> "
                 "BRPrefix<%s> wanintf<%s> firewall<%d>, psid<0x%x>, psidOff<%d> psidLen<%d>", 
                 *mechanism, currObj->status,
                 brprefix, wanintf, firewall, psid, psidOffset, psidLen);

   isMapt = cmsUtl_strcmp(mechanism, MDMVS_ENCAPSULATION) ? TRUE : FALSE;

   if (newObj != NULL && newObj->enable)
   {
      /*
       * ipv6AddrBuf: MAP IPv6 address (RFC 7597 Figure 3)
       * subnetCidr4: BMR IPv4 prefix (including prefix length) in CIDR format
       * addr:        NAT public address for v4 network
       * ipv4MaskLen: BMR IPv4 prefix length
       */
      if ((ret = rutMap_parseDomainRuleInfo(newObj->IPv6Prefix, newObj->IPv4Prefix, newObj->EABitsLength, psidLen, psid,
                                            ipv6AddrBuf, subnetCidr4, addr, &ipv4MaskLen)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutMap_parseDomainRuleInfo fail");
         return ret;
      }

      if (qdmIpIntf_isWanInterfaceUpLocked_dev2(wanintf, FALSE))
      {
         ret = launchMap(brprefix, wanintf, psidOffset, psidLen, psid, newObj->EABitsLength, newObj->IPv6Prefix, ipv6AddrBuf, subnetCidr4, addr, isMapt, TRUE);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);

         /* 
          * If the respective IPv6 WAN interface is firewall enabled,
          * this MAP tunnel needs to enable IPv4 firewall too.
          */
         if ( firewall )
         {
            rutIpt_insertIpModules();
            rutIpt_initFirewall(PF_INET, MAP_TUNNEL_NAME);
//            rutIpt_TCPMSSforIPTunnel(ifname, FALSE); //FIXME AAA!!!
         }
      }
      else
      {
         cmsLog_debug("WAN<%s> is not up", wanintf);
         if (cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) == 0)
         {
            ret = launchMap(brprefix, wanintf, psidOffset, psidLen, psid, newObj->EABitsLength, newObj->IPv6Prefix, ipv6AddrBuf, subnetCidr4, addr, isMapt, FALSE);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         }
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting MAP-%c", isMapt ? 'T' : 'E');
      if (cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) == 0)
      {
         if ((ret = rutMap_parseDomainRuleInfo(currObj->IPv6Prefix, currObj->IPv4Prefix, currObj->EABitsLength, psidLen, psid,
                                               ipv6AddrBuf, subnetCidr4, addr, &ipv4MaskLen)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutMap_parseDomainRuleInfo fail");
            return ret;
         }

         ret = launchMap(brprefix, wanintf, psidOffset, psidLen, psid, currObj->EABitsLength, currObj->IPv6Prefix, ipv6AddrBuf, subnetCidr4, addr, isMapt, FALSE);
      }

      if (newObj) /* case of disable */
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      }
   }   

   return ret;
}

#endif /* SUPPORT_MAP */
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */


