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

#ifdef SUPPORT_IPV6

/*!\file rut_lan6.c
 * \brief All of the functions in this file are for IPv6.
 *
 */

#include <stdio.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_network.h"
#include "rut_wan.h"
#include "rut_wan6.h"
#include "rut_dnsproxy.h"
#include "rut_ebtables.h"

#ifdef DMP_DEVICE2_HOSTS_2
#include "cms_qdm.h"
#endif




const char *radvdConfFile  = "/var/radvd.conf";
const char *dhcp6sConfFileTemplate = "/var/dhcp6s_%s.conf";

CmsRet rutLan_setIPv6Address(char *newAddr, char *newIfName,
                             char *oldAddr, char *oldIfName)
{
   char cmdLine[BUFLEN_128];
   CmsRet ret = CMSRET_SUCCESS;
   char prefixAddr[BUFLEN_128];
   char prefix[BUFLEN_256];
   UINT32 plen;

   cmsLog_debug("Addr: new=%s old=%s IfName: new=%s old=%s",
                newAddr, oldAddr, newIfName, oldIfName);

   if (!IS_EMPTY_STRING(oldAddr) && !IS_EMPTY_STRING(oldIfName))
   {
      if (cmsUtl_parsePrefixAddress(oldAddr, prefixAddr, &plen) != CMSRET_SUCCESS)
      {
         cmsLog_error("Invalid oldAddr =%s", oldAddr);
         return CMSRET_INVALID_PARAM_VALUE;
      } 
            
      cmsNet_subnetIp6SitePrefix(oldAddr, 0, plen, prefixAddr);
      sprintf(prefix, "%s/%d", prefixAddr, plen);
      rutIpt_configRoutingChain6(prefix, oldIfName, FALSE);
      rutEbt_configIPv6Prefix(prefix, FALSE);
      /* delete the current address for this connection */
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr del %s dev %s 2>/dev/null", 
               oldAddr, oldIfName);
      rut_doSystemAction("rut", cmdLine);
   }

   if ( !IS_EMPTY_STRING(newAddr) && !IS_EMPTY_STRING(newIfName) )
   {
      if (cmsUtl_parsePrefixAddress(newAddr, prefixAddr, &plen) != CMSRET_SUCCESS)
      {
         cmsLog_error("Invalid newAddr =%s", newAddr);
         return CMSRET_INVALID_PARAM_VALUE;
      }      
      
      cmsNet_subnetIp6SitePrefix(newAddr, 0, plen, prefixAddr);
      sprintf(prefix, "%s/%d", prefixAddr, plen);
      rutIpt_configRoutingChain6(prefix, newIfName, TRUE);
      rutEbt_configIPv6Prefix(prefix, TRUE);
      /* add the new address for this connection */
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr add %s dev %s 2>/dev/null",
               newAddr, newIfName);
      rut_doSystemAction("rut", cmdLine);
   }

   if(CMSRET_SUCCESS != rutMulti_reloadMcpd())
   {
      cmsLog_error("failed to reload mcpd");
   }

   return ret;
}  /* End of rutLan_setIPv6Address() */

CmsRet rut_getDhcp6sPrefixFromInterface(const char *ifname, char ** sitePrefix)
{
   CmsRet ret = CMSRET_SUCCESS;
   _PrefixInfoObject *prefixObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   *sitePrefix = NULL;

   if (IS_EMPTY_STRING(ifname))
   {
      return ret;
   }

   cmsLog_debug("ifname=%s", ifname);

   /* FIXME: design how to assign multiple addresses with different prefixes */
   while ( (!found) &&
           (cmsObj_getNextFlags(MDMOID_PREFIX_INFO, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&prefixObj) == CMSRET_SUCCESS) )
   {
      if ( cmsUtl_strcmp(prefixObj->delegatedConnection, ifname) == 0 )
      {
         found = TRUE;
         *sitePrefix = cmsMem_strdupFlags(prefixObj->prefix, mdmLibCtx.allocFlags);
      }
      cmsObj_free((void **) &prefixObj);
   }

   if ( *sitePrefix )
   {
      cmsLog_debug("prefix=%s", *sitePrefix);
   }
   else
   {
      cmsLog_notice("Cannot find prefix");
   }

   return ret;
}

CmsRet rut_restartDhcp6s(const char *ifName, unsigned int *pid,
                         const char *dns6Servers, const char *domainName,
                         UBOOL8 isStateful, const char *minIntfID, const char *maxIntfID,
                         SINT32 leaseTime, const char *sitePrefix)
{
const char *dhcp6sConf = "\
%s\n\
%s\n\
interface %s\n\
{\n\
  allow rapid-commit;\n\
  address-pool IPv6AddrPool %d;\n\
};\n\
pool IPv6AddrPool\n\
{\n\
  range %s to %s;\n\
};\n\
";

   char cmdLine[BUFLEN_128];
   char dns6[BUFLEN_128];
   char dhcp6sConfFile[BUFLEN_64];
   char targetIfName[BUFLEN_16];
   char dnsString[BUFLEN_128+BUFLEN_32];
   char domainNameStr[BUFLEN_64];
   char addrBgn[CMS_IPADDR_LENGTH];
   char addrEnd[CMS_IPADDR_LENGTH];
   char prefixStr[CMS_IPADDR_LENGTH];
   char *tmpConfFile = "/var/dhcp6s_tmp.conf";
   char *separator;
   FILE *fp;
   UINT32 npid;
   UINT32 specificEid;
   UBOOL8 stopFlag = FALSE;

   if (isStateful && IS_EMPTY_STRING(sitePrefix))
   {
      cmsLog_notice("stateful dhcp6s with empty prefix info.");
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("ifName=%s dnsServer=%s stateful=%d prefix=%s minID=%s maxID=%s domainName=%s", 
                ifName, dns6Servers, isStateful, sitePrefix, minIntfID, maxIntfID, domainName);

   *prefixStr = '\0';
   if (!IS_EMPTY_STRING(sitePrefix))
   {
      strncpy(prefixStr, sitePrefix, sizeof(prefixStr)-1);
      prefixStr[sizeof(prefixStr)-1] = '\0';
   }

   *domainNameStr = '\0';
   if (!IS_EMPTY_STRING(domainName))
   {
      snprintf(domainNameStr, sizeof(domainNameStr), "option domain-name \"%s\";", domainName);
   }

   if (ifName == NULL)
   {
       strcpy(dhcp6sConfFile, "/var/dhcp6s.conf");
       strcpy(targetIfName, "br0");
   }
   else
   {
       sprintf(dhcp6sConfFile, dhcp6sConfFileTemplate, ifName);
       strncpy(targetIfName, ifName, sizeof(targetIfName)-1);
       targetIfName[sizeof(targetIfName)-1] = '\0';
   }

   /* create a temporary dhcp6s.conf file for write */
   if ((fp = fopen(tmpConfFile, "w")) == NULL)
   {
      /* error */
      cmsLog_error("failed to open %s", tmpConfFile);
      return CMSRET_INTERNAL_ERROR;
   }

   *dnsString = '\0';
   if (!IS_EMPTY_STRING(dns6Servers))
   {
      /* get a local copy since we are going to modify the string. */
      strncpy(dns6, dns6Servers, sizeof(dns6)-1);
      dns6[sizeof(dns6)-1] = '\0';
      
      /* replace commas in the servers string with spaces */
      while ((separator = strchr(dns6, ',')))
      {
         *separator = ' ';
      }

      snprintf(dnsString, sizeof(dnsString), "option domain-name-servers %s;", dns6);
   }

   if (isStateful)
   {
      UINT32 i;
      char *ptr;

      separator = strchr(prefixStr, '/');
      if( separator != NULL )
      {
         *separator = '\0';

         /*
          * Setup prefixStr: PD may be 11:22:33:44::/64 or 11:22::/64
          * Because interface ID is always 64 bits, we need to make sure prefixStr
          * is at the format of 11:22:33:44: or 11:22:0:0:
          */
         *(separator-1) = '\0';
      }
      ptr = prefixStr;

      for ( i=0;i<4;i++ )
      {
         ptr = strchr(ptr, ':');
         if (ptr == NULL)
         {
            break;
         }
         ptr++;
      }

      i = 4 - i;
      for (;i>0;i--)
      {
         strcat(prefixStr, "0:");
      }

      cmsUtl_strncpy(addrBgn, prefixStr, sizeof(prefixStr));
      cmsUtl_strncpy(addrEnd, prefixStr, sizeof(prefixStr));

      strncat(addrBgn, minIntfID, sizeof(addrBgn)-1);
      addrBgn[sizeof(addrBgn)-1] = '\0';   
         
      strncat(addrEnd, maxIntfID, sizeof(addrEnd)-1);
      addrEnd[sizeof(addrEnd)-1] = '\0';  
      
      fprintf(fp, dhcp6sConf, dnsString, domainNameStr, targetIfName, leaseTime, addrBgn, addrEnd);
   }
   else if (!IS_EMPTY_STRING(dnsString) || !IS_EMPTY_STRING(domainNameStr))
   {
      /* write domain-name-servers and domain-name statement to the conf file */
      fprintf(fp, "%s\n%s\n", dnsString, domainNameStr);
   }
   else
   {
      stopFlag = TRUE;
   }

   fclose(fp);

   if (cmsFil_compareFile(dhcp6sConfFile, tmpConfFile))
   {
      cmsLog_notice("no action due to conf file unchanged.");
      (void) remove(tmpConfFile);
      return CMSRET_SUCCESS;
   }

   if (stopFlag)
   {
      cmsLog_notice("stop dhcp6s due to empty conf file.");
      rut_stopDhcp6s(targetIfName, *pid);
      (void) remove(tmpConfFile);
      *pid = CMS_INVALID_PID;
      return CMSRET_SUCCESS;
   }

   (void) remove(dhcp6sConfFile);
   (void) rename(tmpConfFile, dhcp6sConfFile);

   snprintf(cmdLine, sizeof(cmdLine), "-c %s %s", dhcp6sConfFile, targetIfName);

   specificEid = MAKE_SPECIFIC_EID(*pid, EID_DHCP6S);
   if ((npid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, specificEid,
                               cmdLine, strlen(cmdLine)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart dhcp6s on %s", targetIfName);
   }
   else
   {
      cmsLog_debug("restarting dhcp6s, pid=%d on %s", npid, targetIfName);
      if (dns6Servers)
      {
         rutDpx_updateDnsproxy();
      }
   }

   *pid = npid;
   return CMSRET_SUCCESS;

}  /* End of rut_restartDhcp6s() */

CmsRet rut_stopDhcp6s(const char *ifName, UINT32 pid)
{
   char dhcp6sConfFile[BUFLEN_64];
   UINT32 specificEid;

   if (pid == CMS_INVALID_PID)
   {
      return CMSRET_SUCCESS;
   }

   if (ifName == NULL)
   {
      strcpy(dhcp6sConfFile, "/var/dhcp6s.conf");
   }
   else
   {
      sprintf(dhcp6sConfFile, dhcp6sConfFileTemplate, ifName);
   }

   specificEid = MAKE_SPECIFIC_EID(pid, EID_DHCP6S);

   rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0);
   (void) remove(dhcp6sConfFile);
   rutDpx_updateDnsproxy();
   return CMSRET_SUCCESS;
}  /* End of rut_stopDhcp6s() */

#ifdef DMP_DEVICE2_HOSTS_2
static void rut_sendRenewMsgToRastatus6(void)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   CmsRet ret;

   if (qdmIpIntf_isAllBridgeWanServiceLocked())
   {
      cmsLog_debug("All bridge configuration and no need to send msg to rastatus6");
      return;
   }

   if (rut_isApplicationRunning(EID_RASTATUS6) == FALSE)
   {
      cmsLog_debug("rastatus6 is not launched");
      return;
   }

   msg.type = CMS_MSG_RASTATUS6_HOST6_RENEW;
   msg.src = mdmLibCtx.eid;
   msg.dst = EID_RASTATUS6;
   msg.flags_event = 1;

   if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send HOST6_RENEW msg to rastatus6, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("CMS_MSG_RASTATUS6_HOST6_RENEW sent successfully");
   }

   return;
}
#endif

CmsRet rut_restartRadvd(void)
{
   char cmdLine[BUFLEN_128];
   UINT32 pid;

   snprintf(cmdLine, sizeof(cmdLine), "-C %s", radvdConfFile);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_RADVD,
                               cmdLine, strlen(cmdLine)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart radvd on br0");
      return CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("restarting radvd, pid=%d on br0", pid);
#ifdef DMP_DEVICE2_HOSTS_2
      rut_sendRenewMsgToRastatus6();
#endif
   }

   return CMSRET_SUCCESS;
}

CmsRet rut_stopRadvd(void)
{
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_RADVD, NULL, 0);
   (void) remove(radvdConfFile);
   return CMSRET_SUCCESS;

}  /* End of rut_stopRadvd() */

#endif  /* SUPPORT_IPV6 */

#ifdef DMP_X_BROADCOM_COM_IPV6_1
CmsRet rut_createRadvdConf(void)
{
#ifdef DESKTOP_LINUX
   cmsLog_debug("skip create RadvdConf");
   return CMSRET_SUCCESS;
#else

const char *radvdConf0 = "\
interface br0\n\
{\n\
  AdvSendAdvert on;\n\
  AdvManagedFlag %s;\n\
  AdvOtherConfigFlag on;\n\
  %s\n\
  AdvDefaultPreference low;\n\
";

const char *radvdConf1 = "\
  prefix %s\n\
  {\n\
    AdvPreferredLifetime %s;\n\
    AdvValidLifetime %s;\n\
    AdvOnLink on;\n\
    AdvAutonomous %s;\n\
    AdvRouterAddr off;\n\
  };\n\
";

const char *radvdConf0Ext = "\
  MaxRtrAdvInterval 180;\n\
  MinRtrAdvInterval 60;\n\
";

const char *radvdConf1Ext = "\
  prefix %s\n\
  {\n\
    AdvPreferredLifetime 0;\n\
    AdvValidLifetime %s;\n\
    AdvOnLink on;\n\
    AdvAutonomous %s;\n\
    AdvRouterAddr off;\n\
    DecrementLifetimes on;\n\
  };\n\
";

const char *radvdConf2Ext = "\
  RDNSS %s\n\
  {\n\
    AdvRDNSSLifetime 360;\n\
    FlushRDNSS on;\n\
  };\n\
";

const char *radvdConf3Ext = "\
  DNSSL %s\n\
  {\n\
    AdvDNSSLLifetime 360;\n\
    FlushDNSSL on;\n\
  };\n\
";

const char *radvdConf3 = "\
  route %s\n\
  {\n\
    AdvRoutePreference high;\n\
    AdvRouteLifetime %s;\n\
  };\n\
";

const char *radvdConf2 = "\
};\n\
";

   const char *tmpConfFile = "/var/radvd_tmp.conf";
   FILE *fp;
   char *sp;
   char snPrefix[CMS_IPADDR_LENGTH];
   char snPrefixOld[CMS_IPADDR_LENGTH];
   char address[CMS_IPADDR_LENGTH];
   char prefixStr[BUFLEN_256];
   char *nextToken;
   char advManageStr[BUFLEN_8];
   char routerLifetime[BUFLEN_32]="";
   UBOOL8 statefulDhcp;
   _IPv6LanHostCfgObject *ipv6LanCfgObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _PrefixInfoObject *prefixObj = NULL;
   InstanceIdStack iidStackPrefix0 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackPrefix = EMPTY_INSTANCE_ID_STACK;
   _ULAPrefixInfoObject *ulaObj = NULL;
   _RadvdOtherInfoObject *radvdOtherObj = NULL;
   InstanceIdStack iidStackOther = EMPTY_INSTANCE_ID_STACK;
   char recursiveDns[BUFLEN_128];
   char dnssSearchList[BUFLEN_128];
   char *separator;
   CmsRet ret;

   cmsLog_debug("Enter rut_createRadvdConf()");

   /* check if autoconfiguration should be on or off */
   if ((ret = cmsObj_getNextFlags(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&ipv6LanCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
      return ret;
   }

   statefulDhcp = ipv6LanCfgObj->statefulDHCPv6Server;
   cmsObj_free((void **) &ipv6LanCfgObj);

   /* create a temporary radvd.conf file for write */
   if ((fp = fopen(tmpConfFile, "w")) == NULL)
   {
      cmsLog_error("failed to open %s\n", tmpConfFile);
      return CMSRET_INTERNAL_ERROR;
   }

   if (statefulDhcp)
   {
      strcpy(advManageStr, "on");
   }
   else
   {
      strcpy(advManageStr, "off");
   }

   /* 
    * RFC 6204: If there is no delegated prefix, set router lifetime to 0 
    * (item ULA-5 and L-4)
    *
    * If there is no default gateway at WAN, set router lifetime to 0 (except for 6rd)
    */
   if ( cmsObj_getNextFlags(MDMOID_PREFIX_INFO, &iidStackPrefix0, OGF_NO_VALUE_UPDATE, (void **)&prefixObj) == CMSRET_SUCCESS )
   {
      if ( (prefixObj->prefix == NULL) || 
           ((prefixObj->prefix != NULL) && (cmsUtl_strcmp(prefixObj->mode, MDMVS_TUNNELDELEGATED) != 0) && 
            !rutWan_isDfltGtwyExist()) 
         )
      {
         cmsLog_notice("NULL prefix in prefixObj or zero lifetime RA received!!");
         strcpy(routerLifetime, "AdvDefaultLifetime 0;");
      }
      else
      {
         cmsLog_debug("Valid delegated prefix, don't set routerLifetime to 0");
      }
      cmsObj_free((void **) &prefixObj);
   }
   else
   {
      cmsLog_debug("No valid delegated prefix, set routerLifetime to 0");
      strcpy(routerLifetime, "AdvDefaultLifetime 0;");
   }

   /* write the first part of the conf file */
   fprintf(fp, radvdConf0, advManageStr, routerLifetime);

   fprintf(fp, radvdConf0Ext);

   /* ULA Prefix info */
   if ((ret = cmsObj_get(MDMOID_ULA_PREFIX_INFO, &iidStack, 0, (void **) &ulaObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_ULA_PREFIX_INFO!, ret=%d", ret);
   }
   else
   {
      if ( (ulaObj->enable) && !IS_EMPTY_STRING(ulaObj->prefix))
      {
         char pltimeStr[BUFLEN_16];
         char vltimeStr[BUFLEN_16];
         char slaacStr[BUFLEN_8];

         if (ulaObj->preferredLifeTime < 0)
         {
            strcpy(pltimeStr, "infinity");
         }
         else
         {
            sprintf(pltimeStr, "%d", ulaObj->preferredLifeTime);
         }
         if (ulaObj->validLifeTime < 0)
         {
            strcpy(vltimeStr, "infinity");
         }
         else
         {
            sprintf(vltimeStr, "%d", ulaObj->validLifeTime);
         }

         if (statefulDhcp)
         {
            strcpy(slaacStr, "off");
         }
         else
         {
            strcpy(slaacStr, "on");
         }

         fprintf(fp, radvdConf1, ulaObj->prefix, pltimeStr, vltimeStr, slaacStr);

         fprintf(fp, radvdConf3, ulaObj->prefix, vltimeStr);
      }
      cmsObj_free((void **) &ulaObj);
   }

    /* RFC 7084:
       The IPv6 CE router MUST support providing DNS information in
       the Router Advertisement Recursive DNS Server (RDNSS) and DNS
       Search List options.  Both options are specified in [RFC6106].
   */
   if (cmsObj_getNextFlags(MDMOID_RADVD_OTHER_INFO, &iidStackOther, OGF_NO_VALUE_UPDATE, (void **)&radvdOtherObj) == CMSRET_SUCCESS)
   {
	   if (!IS_EMPTY_STRING(radvdOtherObj->recursiveDns))
	   {
			/* get a local copy since we are going to modify the string. */
			strncpy(recursiveDns, radvdOtherObj->recursiveDns, sizeof(recursiveDns));
			
			/* replace commas in the servers string with spaces */
			while ((separator = strchr(recursiveDns, ',')))
			{
			   *separator = ' ';
			}

			fprintf(fp, radvdConf2Ext, recursiveDns);
	   }
	   
	   if (!IS_EMPTY_STRING(radvdOtherObj->dnssSearchList))
	   {
			strncpy(dnssSearchList, radvdOtherObj->dnssSearchList, sizeof(dnssSearchList));
			
			/* replace commas in the servers string with spaces */
			while ((separator = strchr(dnssSearchList, ',')))
			{
			   *separator = ' ';
			}
			
			fprintf(fp, radvdConf3Ext, dnssSearchList);
	   }

	   cmsObj_free((void **) &radvdOtherObj);
   }

   /* Write the configuration file with all possible prefixes on br0 */
   while ( cmsObj_getNextFlags(MDMOID_PREFIX_INFO, &iidStackPrefix, OGF_NO_VALUE_UPDATE, (void **)&prefixObj) == CMSRET_SUCCESS )
   {
      char vltimeStr[BUFLEN_16];
      char vltimeOldStr[BUFLEN_16];

      if ( prefixObj->prefix == NULL )
      {
         cmsLog_error("NULL prefix in prefixObj!!");
         cmsObj_free((void **) &prefixObj);
         continue;
      }

      /* check if the link is up or tunnel is ready */
      if ( cmsUtl_strcmp(prefixObj->mode, MDMVS_WANDELEGATED) == 0 )
      {
         if ( !rut_isWanInterfaceUp(prefixObj->delegatedConnection, FALSE) )
         {
            cmsObj_free((void **) &prefixObj);
            continue;
         }
      }

      /* get a local copy of sitePrefix because we are going to use strtok_r() */
      *prefixStr = '\0';
      if (!IS_EMPTY_STRING(prefixObj->prefix))
      {
         strncpy(prefixStr, prefixObj->prefix, sizeof(prefixStr));
      }

      /* retrieve each site prefix from prefixStr */
      nextToken = NULL;
      sp = strtok_r(prefixStr, ",", &nextToken);
      while (!IS_EMPTY_STRING(sp))
      {
         if ((ret = cmsNet_subnetIp6SitePrefix(sp, 0, 64, snPrefix)) != CMSRET_SUCCESS)
         {
            cmsObj_free((void **) &prefixObj);
            cmsLog_error("cmsNet_subnetIp6SitePrefix returns error. ret=%d", ret);
            fclose(fp);
            (void) remove(tmpConfFile);
            return ret;
         }
         if (prefixObj->validLifeTimeOld > 0)
         {
            if ((ret = cmsNet_subnetIp6SitePrefix(prefixObj->prefixOld, 0, 64, snPrefixOld)) != CMSRET_SUCCESS)
            {
               cmsObj_free((void **) &prefixObj);
               cmsLog_error("cmsNet_subnetIp6SitePrefix returns error. ret=%d", ret);
               fclose(fp);
               (void) remove(tmpConfFile);
               return ret;
            }
         }
         if (!IS_EMPTY_STRING(snPrefix))
         {
            char pltimeStr[BUFLEN_16];
            char slaacStr[BUFLEN_8];

            if (prefixObj->preferredLifeTime < 0)
            {
               strcpy(pltimeStr, "infinity");
            }
            else
            {
               sprintf(pltimeStr, "%d", prefixObj->preferredLifeTime);
            }
            if (prefixObj->validLifeTime < 0)
            {
               strcpy(vltimeStr, "infinity");
            }
            else
            {
               sprintf(vltimeStr, "%d", prefixObj->validLifeTime);
            }

            if (statefulDhcp)
            {
               strcpy(slaacStr, "off");
            }
            else
            {
               strcpy(slaacStr, "on");
            }

            if (0 > snprintf(address, CMS_IPADDR_LENGTH, "%s/64", snPrefix))
            {
               cmsLog_debug("snprintf failed to write '%s/64' into address", snPrefix);
               address[0] = 0;
            }
            fprintf(fp, radvdConf1, address, pltimeStr, vltimeStr, slaacStr);

            if (prefixObj->validLifeTimeOld > 0)
            {
               if (!IS_EMPTY_STRING(snPrefixOld))
               {
                  if (0 > snprintf(address, CMS_IPADDR_LENGTH, "%s/64", snPrefixOld))
                  {
                     cmsLog_debug("snprintf failed to write '%s/64' into address", snPrefixOld);
                     address[0] = 0;
                  }
                  sprintf(vltimeOldStr, "%d", prefixObj->validLifeTimeOld);
                  fprintf(fp, radvdConf1Ext, address, vltimeOldStr, slaacStr);
               }
            }

            /* check for the cdrouter dhcpv6_pd_130 */
            cmsLog_debug("<%s>assigned for prefix definition", address);
         }

         strcpy(address, sp);

         sp = strtok_r(NULL, ",", &nextToken);
      }

      /* RFC 4191: Route Information Option FIXME: for multiple route!!! Sync with Prefix */
      fprintf(fp, radvdConf3, address, vltimeStr);

      /* check for the cdrouter dhcpv6_pd_130 */
      cmsLog_debug("<%s>assigned for route definition", address);

      cmsObj_free((void **) &prefixObj);
   }

   /* write the last part of the conf file */
   fprintf(fp, radvdConf2);
   fclose(fp);

   (void) remove(radvdConfFile);
   (void) rename(tmpConfFile, radvdConfFile);

   return CMSRET_SUCCESS;

#endif /* DESKTOP_LINUX */  

}  /* End of rut_createRadvdConf() */

static CmsRet rut_setUlaForBridge(const char *bridgeName)
{  
    _ULAPrefixInfoObject *ulaObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char   ULAddress[BUFLEN_48];
   char   cmdLine[BUFLEN_128];
   UINT32 prefixLen;
   CmsRet ret;

   /* ULA Prefix info */
   if ((ret = cmsObj_getNextFlags(MDMOID_ULA_PREFIX_INFO, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ulaObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_ULA_PREFIX_INFO!, ret=%d", ret);
   }
   else
   {
      if ((ulaObj->enable) && !IS_EMPTY_STRING(ulaObj->prefix))
      {
        /* add UL Address to br0 at boot time*/
         *ULAddress = '\0';
         cmsUtl_getULAddressByPrefix(ulaObj->prefix, bridgeName, ULAddress, &prefixLen);
         snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr add %s/%u dev %s 2>/dev/null", ULAddress, prefixLen, bridgeName);
         rut_doSystemAction("rut", cmdLine);
      }
       
      cmsObj_free((void **) &ulaObj);
   }

   return CMSRET_SUCCESS;
}

CmsRet rut_restartRadvdForBridge(const char *bridgeName __attribute__((unused)))
{
   _RadvdConfigMgtObject *radvdObj = NULL;
   InstanceIdStack iidStackRadvdCfg = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("bridgeName<%s>", bridgeName);

   if ((ret = cmsObj_getNextFlags(MDMOID_RADVD_CONFIG_MGT, &iidStackRadvdCfg, OGF_NO_VALUE_UPDATE, (void **)&radvdObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_RADVD_CONFIG_MGT, ret=%d", ret);
      return ret;
   }

   if (radvdObj->enable)
   {
      if (!cmsUtl_strcmp(bridgeName, "br0"))
      {
         rut_setUlaForBridge(bridgeName); /*radvd only works on br0*/
      }
       
      rut_createRadvdConf();
      rut_restartRadvd();
   }

   cmsObj_free((void **) &radvdObj);

   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_IPV6_1 */
