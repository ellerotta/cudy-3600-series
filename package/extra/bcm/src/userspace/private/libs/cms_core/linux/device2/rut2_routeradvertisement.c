/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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
/* this file touches TR181 IPv6 objects */
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_ipv6.h"
#include "rut2_ra.h"
#include "rut2_bridging.h"


/*!\file rut2_routeradvertisement.c
 * \brief IPv6 helper functions for rcl2_routeradvertisement.c
 *
 */

const char *radvdConfigFile  = "/var/radvd.conf";

void rutRa_updatePrefixes(const char *ifpath, char **prefixes, UBOOL8 *foundPD, UBOOL8 *isIPv6Rd)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6Prefix = NULL;
   CmsRet ret;

   if (!ifpath)
   {
      cmsLog_error("null ifpath");
      return;
   }

   cmsLog_debug("ifpath=%s", ifpath);

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         
   if ((ret = cmsMdm_fullPathToPathDescriptor(ifpath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", ifpath, ret);
      return;
   }         

   CMSMEM_FREE_BUF_AND_NULL_PTR(*prefixes);
   *prefixes = (char *)cmsMem_alloc(CMS_DEV2_RA_PREFIX_LEN, ALLOC_SHARED_MEM);
   *foundPD = FALSE;

   /*
    * 1. Clear the prefixes
    * 2. Get all prefixes of the IP.Interface.i.IPv6Prefix.
    */
   memset(*prefixes, 0, CMS_DEV2_RA_PREFIX_LEN);

   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX, &(pathDesc.iidStack), 
           &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6Prefix) == CMSRET_SUCCESS)
   {
      UBOOL8 isPD;

      isPD = ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_CHILD) == 0) ||
              ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_STATIC) == 0) &&
               (cmsUtl_strcmp(ipv6Prefix->staticType, MDMVS_CHILD) == 0)));

      /*
       IPv6 Rapid Deployment on IPv4 Infrastructures (6rd)
       Origin = Child, StaticType = Inapplicable 
      */
      *isIPv6Rd = ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_CHILD) == 0) &&
             (cmsUtl_strcmp(ipv6Prefix->staticType, MDMVS_INAPPLICABLE) == 0));

      if (cmsUtl_strcmp(ipv6Prefix->status, MDMVS_DISABLED) &&
          ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_AUTOCONFIGURED) == 0) || isPD)
         )
      {
         MdmPathDescriptor pathDesc_ipv6prefix;
         char *ipv6prefixFullPath=NULL;

         INIT_PATH_DESCRIPTOR(&pathDesc_ipv6prefix);
         pathDesc_ipv6prefix.oid = MDMOID_DEV2_IPV6_PREFIX;
         pathDesc_ipv6prefix.iidStack = iidStack;
         if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc_ipv6prefix, &ipv6prefixFullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
            cmsObj_free((void **)&ipv6Prefix);
            return;
         }

         if (cmsUtl_strlen(*prefixes) > 0)
         {
            strcat(*prefixes, ",");
         }

         strcat(*prefixes, ipv6prefixFullPath);

         CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6prefixFullPath);
         *foundPD |= isPD;
      }

      cmsObj_free((void **)&ipv6Prefix);
   }

#ifdef SUPPORT_MAP
    /*For CD Router MAP-E cases, check if running MAP-T/MAP-E.
      If yes, would not check default gateway before create radvd.conf
    */
    Dev2MapDomainObject *domain;
    InstanceIdStack iidStackMap = EMPTY_INSTANCE_ID_STACK;

    while (cmsObj_getNextFlags(MDMOID_DEV2_MAP_DOMAIN, &iidStackMap,
                     OGF_NO_VALUE_UPDATE, (void **)&domain) == CMSRET_SUCCESS)
    {
       *isIPv6Rd = 1;
       cmsObj_free((void **)&domain);
       break;
    }
#endif

   cmsLog_debug("prefixes<%s> foundPD(%d), isIPv6Rd(%d)", *prefixes, *foundPD, *isIPv6Rd);
   return;
}

static UBOOL8 rutRa_isDfltGtwyExist()
{
    InstanceIdStack iidStackIpv6Forward = EMPTY_INSTANCE_ID_STACK;
    Dev2Ipv6ForwardingObject *ForwardingObj=NULL;
    UBOOL8 found = FALSE;

    while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_FORWARDING, &iidStackIpv6Forward, OGF_NO_VALUE_UPDATE, (void **)&ForwardingObj) == CMSRET_SUCCESS))
    {
        if (!IS_EMPTY_STRING(ForwardingObj->nextHop))
        {
           found = TRUE;
        }

        cmsObj_free((void *) &ForwardingObj);
    }

    cmsLog_debug("found=%d", found);
    return found;
}

static CmsRet getInterfaceName(const char *ipIntfFullPath, char **ifName)
{
    MdmPathDescriptor pathDesc;
    CmsRet ret;
    Dev2IpInterfaceObject *ipIntfObj;

    ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to convert fullpath %s to pathDesc", ipIntfFullPath);
        return ret;
    }

    ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **) &ipIntfObj);
    if (ret == CMSRET_SUCCESS)
    {
        *ifName = cmsMem_strdup(ipIntfObj->name);
        cmsObj_free((void **) &ipIntfObj);
    }
    return CMSRET_SUCCESS;
}

void rutRa_createRadvdConf(UBOOL8 foundPD, UBOOL8 isIPv6Rd)
{
#ifdef DESKTOP_LINUX
   cmsLog_debug("skip create RadvdConf");
   return;
#else
const char *radvdConf0 = "\
interface %s\n\
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
    FlushRDNSS on;\n\
  };\n\
";

const char *radvdConf3Ext = "\
  DNSSL %s\n\
  {\n\
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
   char prefixStr[CMS_DEV2_RA_PREFIX_LEN];
   char *nextToken;
   char advManageStr[BUFLEN_8];
   char routerLifetime[BUFLEN_32]="";
   char *interface;
   UBOOL8 statefulDhcp;
   InstanceIdStack iidStack_raIntf = EMPTY_INSTANCE_ID_STACK;
   Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj = NULL;

   cmsLog_debug("enter foundPD<%d>", foundPD);

   /* create a temporary radvd.conf file for write */
   if ((fp = fopen(tmpConfFile, "w")) == NULL)
   {
      cmsLog_error("failed to open %s\n", tmpConfFile);
      return;
   }

   /* 
    * RFC 6204: If there is no delegated prefix, set router lifetime to 0 
    * (item ULA-5 and L-4)
    *
    * If there is no default gateway at WAN, set router lifetime to 0 (except for 6rd)
    */
   if (!foundPD || (foundPD && !isIPv6Rd && !rutRa_isDfltGtwyExist()))
   {
      cmsLog_notice("NULL prefix in prefixObj or zero lifetime RA received,foundPD=%d,isIPv6Rd=%d",foundPD,isIPv6Rd);
      strcpy(routerLifetime, "AdvDefaultLifetime 0;");
   }

   while (cmsObj_getNext(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, 
                         &iidStack_raIntf, (void **) &raIntfObj) == CMSRET_SUCCESS)
   {
      interface = NULL;
      statefulDhcp = raIntfObj->advManagedFlag;
      SINT32 subnetId;

      if (statefulDhcp)
      {
         strcpy(advManageStr, "on");
      }
      else
      {
         strcpy(advManageStr, "off");
      }

      if (CMSRET_SUCCESS != getInterfaceName(raIntfObj->interface, &interface))
      {
          cmsLog_error("Failed to get interface name!");
          continue;
      }

      /* write the first part of the conf file */
      fprintf(fp, radvdConf0, interface, advManageStr, routerLifetime);

      fprintf(fp, radvdConf0Ext);

      /* 
       * Advertise prefixes based on RAIntf.i.prefixes which is already updated
       */

#ifdef DMP_DEVICE2_BRIDGE_1
      if (CMSRET_SUCCESS != rutBridge_getBridgeIndexByName_dev2(interface, &subnetId))
      {
          cmsLog_error("Failed to get the bridge index for %s", interface);
      }
#else
      subnetId = 0;
#endif

      cmsMem_free(interface);

      /* get a local copy of prefixes because we are going to use strtok_r() */
      *prefixStr = '\0';
      if (!IS_EMPTY_STRING(raIntfObj->prefixes))
      {
         strncpy(prefixStr, raIntfObj->prefixes, sizeof(prefixStr)-1);
         prefixStr[sizeof(prefixStr)-1] = '\0';
      }

      /* retrieve each prefix from prefixStr */
      nextToken = NULL;
      sp = strtok_r(prefixStr, ",", &nextToken);

      while (!IS_EMPTY_STRING(sp))
      {
         MdmPathDescriptor pathDesc;
         InstanceIdStack dhcp6cRcvIidStack = EMPTY_INSTANCE_ID_STACK;
         Dev2Ipv6PrefixObject *ipv6Prefix = NULL; 
         Dev2Dhcp6cRcvOptionObject *dhcp6cRcvObj=NULL;
         char pltimeStr[BUFLEN_16];
         char vltimeStr[BUFLEN_16];
         char vltimeOldStr[BUFLEN_16];
         char slaacStr[BUFLEN_8];
         char snPrefix[CMS_IPADDR_LENGTH];
         char snPrefixOld[CMS_IPADDR_LENGTH];
         char recursiveDns[BUFLEN_128];
         char dnssSearchList[BUFLEN_128];
         char *separator;
         CmsRet ret;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         ret = cmsMdm_fullPathToPathDescriptor(sp, &pathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", sp, ret);
            cmsObj_free((void **) &raIntfObj);
            fclose(fp);
            (void) remove(tmpConfFile);
            return;
         }

         if ((ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **)&ipv6Prefix)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get failed for %s, ret=%d", sp, ret);
            cmsObj_free((void **) &raIntfObj);
            fclose(fp);
            (void) remove(tmpConfFile);
            return;
         }


         if ((ret = cmsNet_subnetIp6SitePrefix(ipv6Prefix->prefix, subnetId, 64, snPrefix)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsNet_subnetIp6SitePrefix failed for %s, ret=%d", sp, ret);
            cmsObj_free((void **) &raIntfObj);
            cmsObj_free((void **) &ipv6Prefix);
            fclose(fp);
            (void) remove(tmpConfFile);
            return;
         }

         if (ipv6Prefix->X_BROADCOM_COM_Vlt_Old > 0)
         {
            if ((ret = cmsNet_subnetIp6SitePrefix(ipv6Prefix->X_BROADCOM_COM_Prefix_Old, subnetId, 64, snPrefixOld)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsNet_subnetIp6SitePrefix failed for %s, ret=%d", sp, ret);
               cmsObj_free((void **) &raIntfObj);
               cmsObj_free((void **) &ipv6Prefix);
               fclose(fp);
               (void) remove(tmpConfFile);
               return;
            }
         }

         if (cmsUtl_strcmp(ipv6Prefix->status, MDMVS_DISABLED))
         {
            if (ipv6Prefix->X_BROADCOM_COM_Plt < 0)
            {
               strcpy(pltimeStr, "infinity");
            }
            else
            {
               sprintf(pltimeStr, "%d", ipv6Prefix->X_BROADCOM_COM_Plt);
            }
            if (ipv6Prefix->X_BROADCOM_COM_Vlt < 0)
            {
               strcpy(vltimeStr, "infinity");
            }
            else
            {
               sprintf(vltimeStr, "%d", ipv6Prefix->X_BROADCOM_COM_Vlt);
            }

            if (statefulDhcp)
            {
               strcpy(slaacStr, "off");
            }
            else
            {
               strcpy(slaacStr, "on");
            }

            cmsUtl_strcat(snPrefix, "/64");
            fprintf(fp, radvdConf1, snPrefix, pltimeStr, vltimeStr, slaacStr);

            if (ipv6Prefix->X_BROADCOM_COM_Vlt_Old > 0)
            {
               if (!IS_EMPTY_STRING(snPrefixOld))
               {
                  cmsUtl_strcat(snPrefixOld, "/64");
                  sprintf(vltimeOldStr, "%d", ipv6Prefix->X_BROADCOM_COM_Vlt_Old);
                  fprintf(fp, radvdConf1Ext, snPrefixOld, vltimeOldStr, slaacStr);
               }
            }

            /* RFC 4191: Route Information Option FIXME: for multiple route!!! Sync with Prefix */
            fprintf(fp, radvdConf3, ipv6Prefix->prefix, vltimeStr);

            /* check for the cdrouter dhcpv6_pd_130 */
            cmsLog_debug("<%s>assigned for prefix definition", snPrefix);
            cmsLog_debug("<%s>assigned for route definition", ipv6Prefix->prefix);
         }


         /* RFC 7084:
             The IPv6 CE router MUST support providing DNS information in
             the Router Advertisement Recursive DNS Server (RDNSS) and DNS
             Search List options.  Both options are specified in [RFC6106].
         */
         while (!isIPv6Rd && cmsObj_getNext(MDMOID_DEV2_DHCP6C_RCV_OPTION, 
                &dhcp6cRcvIidStack, (void **) &dhcp6cRcvObj) == CMSRET_SUCCESS)
         {
             if (!cmsUtl_strcmp(ipv6Prefix->prefix, dhcp6cRcvObj->prefix))
             {
                 if (!IS_EMPTY_STRING(dhcp6cRcvObj->DNSServers))
                 {
                     /* get a local copy since we are going to modify the string. */
                     strncpy(recursiveDns, dhcp6cRcvObj->DNSServers, sizeof(recursiveDns)-1);
                     recursiveDns[sizeof(recursiveDns)-1] = '\0';
    
                     /* replace commas in the servers string with spaces */
                      while ((separator = strchr(recursiveDns, ',')))
                      {
                          *separator = ' ';
                      }
    
                      fprintf(fp, radvdConf2Ext, recursiveDns);
                 }
     
                 if (!IS_EMPTY_STRING(dhcp6cRcvObj->domainName))
                 {
                      /* get a local copy since we are going to modify the string. */
                      strncpy(dnssSearchList, dhcp6cRcvObj->domainName, sizeof(dnssSearchList)-1);
                      dnssSearchList[sizeof(dnssSearchList)-1] = '\0';
 
                      /* replace commas in the servers string with spaces */
                      while ((separator = strchr(dnssSearchList, ',')))
                      {
                          *separator = ' ';
                      }
      
                      fprintf(fp, radvdConf3Ext, dnssSearchList);
                 }
             }

             cmsObj_free((void **) &dhcp6cRcvObj);
         }

         cmsObj_free((void **) &ipv6Prefix);

         sp = strtok_r(NULL, ",", &nextToken);
      }

      fprintf(fp, radvdConf2);
      cmsObj_free((void **) &raIntfObj);   
   }

   fclose(fp);

   (void) remove(radvdConfigFile);
   (void) rename(tmpConfFile, radvdConfigFile);

   return;

#endif /* DESKTOP_LINUX */  
}

void rutRa_updateAllRouterAdvObj()
{
    InstanceIdStack iidStack_raIntf = EMPTY_INSTANCE_ID_STACK;
    Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj = NULL;
    CmsRet ret = CMSRET_SUCCESS;
    
    if (cmsObj_getNext(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, 
                 &iidStack_raIntf, (void **) &raIntfObj) == CMSRET_SUCCESS)
    {
       ret = cmsObj_set(raIntfObj, &iidStack_raIntf);
       if (ret != CMSRET_SUCCESS)
       {
          cmsLog_error("Failed to set Dev2RouterAdvertisementInterfaceSettingObject, ret=%d", ret);
       }     
       cmsObj_free((void **) &raIntfObj);
    }
}

void rutRa_updateRouterAdvObj(const char *ipIntfFullPath)
{
   InstanceIdStack iidStackRaIntf = EMPTY_INSTANCE_ID_STACK;
   Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj = NULL;
   UBOOL8 foundRa = FALSE;

   cmsLog_debug("radvd for PD:ip.intf<%s>", ipIntfFullPath);

   while (!foundRa && cmsObj_getNext(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING,
                      &iidStackRaIntf, (void **) &raIntfObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(raIntfObj->interface, ipIntfFullPath))
      {
         foundRa = TRUE;
      }
      else
      {
         cmsObj_free((void **) &raIntfObj);
      }
   }

   if (foundRa)
   {
      CmsRet ret;

      ret = cmsObj_set(raIntfObj, &iidStackRaIntf);
      cmsObj_free((void **) &raIntfObj);

      if (ret != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed setting RAInterfaceSetting. ret %d", ret);
      }
   }
   else
   {
      cmsLog_error("Failed getting RAInterfaceSetting.");
   }

   return;
}

static CmsRet rutRa_setUlaForBridge(const char *bridgeName)
{  
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6Prefix = NULL;
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   UBOOL8 found = FALSE;
   char   ULAddress[BUFLEN_48];
   char   cmdLine[BUFLEN_128];
   UINT32 prefixLen;
   CmsRet ret;

   while (found == FALSE &&
           (ret = cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                                      OGF_NO_VALUE_UPDATE, (void **) &ipIntfObj)) == CMSRET_SUCCESS)
   {
       if (ipIntfObj->name != NULL &&
           (cmsUtl_strcmp(ipIntfObj->name, bridgeName) == 0))
       {
           found = TRUE;
       }
       cmsObj_free((void **) &ipIntfObj);
   }

   if (found == FALSE)
   {
       cmsLog_error("Failed to find IPInterface object for %s", bridgeName);
       return CMSRET_OBJECT_NOT_FOUND;
   }

   found = FALSE;
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                                       &iidStack, &iidStackChild,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **)&ipv6Prefix) == CMSRET_SUCCESS)
   {
      if (ipv6Prefix->X_BROADCOM_COM_UniqueLocalFlag)
      {
         /* ULA Prefix info */
         if (!IS_EMPTY_STRING(ipv6Prefix->prefix))
         {
            /* add UL Address to the specified bridge */
            *ULAddress = '\0';
            cmsUtl_getULAddressByPrefix(ipv6Prefix->prefix, bridgeName, ULAddress, &prefixLen);
            snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr add %s/%u dev %s 2>/dev/null", ULAddress, prefixLen, bridgeName);
            rut_doSystemAction("rut", cmdLine);
            cmsObj_free((void **) &ipv6Prefix);
            break;
         }
      }
      cmsObj_free((void **) &ipv6Prefix);
   }
  
   return CMSRET_SUCCESS;
}

/* Check if the interface has IPv6 address associated */ 
CmsRet isIntfHasAddr6(const char *ifname)
{
   FILE     *fp;
   char     line[BUFLEN_64];
   CmsRet   ret = CMSRET_INTERNAL_ERROR;

   if ((fp = fopen("/proc/net/if_inet6", "r")) == NULL)
   {
      cmsLog_error("failed to open /proc/net/if_inet6");
      return CMSRET_INTERNAL_ERROR;
   }

   while (fgets(line, sizeof(line), fp) != NULL)
   {
      /* remove the carriage return char */
      line[strlen(line)-1] = '\0';

      if (strstr(line, ifname) != NULL)
      {
         ret = CMSRET_SUCCESS;
         break;
      }
   }
   fclose(fp);
   return ret;
}

CmsRet rutRa_restartRadvdForBridge(const char *bridgeName)
{
   _Dev2RouterAdvertisementInterfaceSettingObject *adInfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = isIntfHasAddr6(bridgeName)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("No IPv6 address associated with %s", bridgeName);
      return ret;
   }

   if ((ret = cmsObj_getNextFlags(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &adInfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Could not get MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, ret=%d", ret);
      return ret;
   }

   if (adInfObj->enable)
   {
      rutRa_setUlaForBridge(bridgeName);
#ifndef DESKTOP_LINUX
      if (access(radvdConfigFile, F_OK) == 0)
      {
         rut_restartRadvd();
      }
#endif      
   }

   cmsObj_free((void **) &adInfObj);

   return CMSRET_SUCCESS;
}

#endif  /* SUPPORT_IPV6 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

