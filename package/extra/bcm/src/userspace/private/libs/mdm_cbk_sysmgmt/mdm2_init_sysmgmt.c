/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "sysutil.h"
#include "bcm_retcodes.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_net.h"
#include "bcm_net.h"
#include "linux/rut_system.h"

/* in mdm2_initlan.c */
CmsRet mdm_addDefaultLanBridgeObjects_dev2(MdmPathDescriptor *mgmtBrPortPathDesc);
CmsRet mdm_addDefaultDhcpv4ServerObjects_dev2(const char *ipIntfFullPath,
                            const char *ipv4Addr,
                            const char *netmask);
CmsRet mdm_addIpv4AddressObject_dev2(const MdmPathDescriptor *ipIntfPathDesc,
                            const char *ipv4Addr,
                            const char *netmask,
                            const char *addressingType);

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
void mdm_configDhcpv4RelayObject_dev2(void);
#endif

/* in mdm2_initlan6.c */
CmsRet mdm_addIpv6AddressObject_dev2(const MdmPathDescriptor *ipIntfPathDesc);


/* forward declarations */
/* local functions */

static CmsRet addEthernetLinkObject(const char *ifname,
                            const char *lowerLayer,
                            MdmPathDescriptor *pathDesc);


/** This is the "weak" mdm_adjustForHardware_dev2.
 *  It will only be called in a distributed MDM build of the sysmgmt component.
 */
#pragma weak mdm_adjustForHardware_dev2
CmsRet mdm_adjustForHardware_dev2(void)
{
   MdmPathDescriptor mgmtBrPortPathDesc;
   MdmPathDescriptor ipIntfPathDesc;
   CmsRet ret=CMSRET_SUCCESS;
   char br0IpAddrBuf[CMS_IPADDR_LENGTH]={0};
   char br0NetMaskBuf[CMS_IPADDR_LENGTH]={0};

   cmsLog_notice("Distributed MDM build: adjustForHardware for SysMgmt only");

#ifdef DMP_DEVICE2_ROUTING_1
   ret = mdm_initRouterObject_dev2();
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
#endif

   ret = mdm_addDefaultLanBridgeObjects_dev2(&mgmtBrPortPathDesc);
   if ((ret == CMSRET_SUCCESS) && (mgmtBrPortPathDesc.oid == MDMOID_DEV2_BRIDGE_PORT))
   {
      char *mgmtBrPortFullPath=NULL;
      char *ipIntfFullPath=NULL;
      UBOOL8 supportIpv4=TRUE;
      UBOOL8 supportIpv6=FALSE;

      cmsLog_notice("new LAN Bridge object was created, created IP and other objects too");

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&mgmtBrPortPathDesc, &mgmtBrPortFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

      /* set basic IP params */
      /* TODO: if IPv6 only build, then supportIpv4 = FALSE */

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      supportIpv6 = TRUE;
#endif

      ret = mdm_initIpObject_dev2(supportIpv4, supportIpv6);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("initIpObject failed, ret=%d", ret);
         return ret;
      }

      /* create IP.Interface object here */
      ret = mdm_addDefaultLanIpInterfaceObject_dev2("br0", "Default",
                                              supportIpv4, supportIpv6,
                                              mgmtBrPortFullPath,
                                              &ipIntfPathDesc);
      CMSMEM_FREE_BUF_AND_NULL_PTR(mgmtBrPortFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("addDefaultLanIpIntferfaceObject failed, ret=%d", ret);
         return ret;
      }

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ipIntfPathDesc, &ipIntfFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

/* TODO: should this be surrouned by some IPv4 ifdef ? */
#ifndef DHCP_CLIENT_DEFAULT                                         
      /* Try to use the ipaddress/netmask from uboot environment parameters */
      bcmNet_getDefaultLanIpInfo( br0IpAddrBuf, CMS_IPADDR_LENGTH,
                                  br0NetMaskBuf, CMS_IPADDR_LENGTH);

      ret = mdm_addIpv4AddressObject_dev2(&ipIntfPathDesc,
                                         br0IpAddrBuf, br0NetMaskBuf,
                                         MDMVS_STATIC);
#endif 

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addDefaultLanIpv4AddressObject failed, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
      }

      ret = mdm_addDefaultDhcpv4ServerObjects_dev2(ipIntfFullPath, br0IpAddrBuf, br0NetMaskBuf);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addDefaultDhcpv4ServerObjects failed, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
      }

#if defined(DHCP_CLIENT_DEFAULT) && defined(DMP_DEVICE2_DHCPV4_1)
      ret = mdmInit_addDhcpv4ClientObject_dev2(ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdmInit_addDhcpv4ClientObject_dev2 failed, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
      }
#endif  /* defined(DHCP_CLIENT_DEFAULT) && defined(DMP_DEVICE2_DHCPV4_1) */


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      /* TODO: do we need to add IP.Interface.{i}.IPv6Address object ?*/
//      ret = mdm_addIpv6AddressObject_dev2(&ipIntfPathDesc);
//      if (ret != CMSRET_SUCCESS)
//      {
//         cmsLog_error("mdm_addDefaultLanIpv6AddressObject failed, ret=%d", ret);
//         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
//         return ret;
//      }

      /* IPv6 needs DHCP server and Router Advertisement on LAN side */
      mdm_addDefaultDhcpv6ServerObjects_dev2(ipIntfFullPath);
      mdm_addDefaultRouterAdvertisementObjects_dev2(ipIntfFullPath);

#endif

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   }

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
   mdm_configDhcpv4RelayObject_dev2();
#endif

#ifdef DMP_STORAGESERVICE_1
   ret = mdm_addDefaultStorageServiceObject();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addDefaultStorageService failed, ret=%d", ret);
      return ret;
   }
#endif

#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
   ret = mdm_addDefaultUsbHostObject();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addDefaultUsbHostObject failed, ret=%d", ret);
      return ret;
   }
#endif

#ifdef DMP_X_BROADCOM_COM_BASD_1
#if defined(SUPPORT_BAS2) /* this ifdef can be cleaned up when basv1.4 is removed */
   ret = mdm_addStaticBasClients();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addStaticBasClients failed, ret=%d", ret);
      return ret;
   }
#endif 
#endif
   
   cmsLog_notice("====> returning ret=%d", ret);
   return ret;
}


CmsRet mdm_addDefaultLanIpInterfaceObject_dev2(const char *ifname,
                                         const char *groupName,
                                         UBOOL8 supportIpv4,
                                         UBOOL8 supportIpv6,
                                         const char *lowerLayer,
                                         MdmPathDescriptor *ipIntfPathDesc)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   /*
    * If we find an IP.Interface that starts with "br", then this is
    * not a blank MDM, so we should do nothing.
    */
   INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
   ipIntfPathDesc->oid = MDMOID_DEV2_IP_INTERFACE;
   while (!found && (ret == CMSRET_SUCCESS))
   {
      ret = mdm_getNextObject(ipIntfPathDesc->oid, &ipIntfPathDesc->iidStack, (void **) &ipIntfObj);
      if (ret == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strncmp(ipIntfObj->name, "br", 2))
         {
            found = TRUE;
         }
         mdm_freeObject((void **) &ipIntfObj);
      }
   }

   if (found)
   {
      cmsLog_notice("Found existing IP.Interface with name brx, just return");
      INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
      return CMSRET_SUCCESS;
   }

   return (mdmInit_addIpInterfaceObject_dev2(ifname, groupName,
                                             supportIpv4, supportIpv6,
                                             FALSE, FALSE, FALSE, NULL,
                                             lowerLayer, ipIntfPathDesc));
}


CmsRet mdmInit_addIpInterfaceObject_dev2(const char *ifname,
                            const char *groupName,
                            UBOOL8 supportIpv4 __attribute((unused)),
                            UBOOL8 supportIpv6 __attribute((unused)),
                            UBOOL8 isUpstream,
                            UBOOL8 isBridgeService,
                            UBOOL8 isBridgeIpAddrNeeded,
                            const char *referedBridgeName,
                            const char *lowerLayer,
                            MdmPathDescriptor *ipIntfPathDesc)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   char *ethLinkFullPath=NULL;
   CmsRet ret;

   /*
    * First we need a Ethernet.Link object under the IP.Interface object
    */
   {
      MdmPathDescriptor ethLinkPathDesc=EMPTY_PATH_DESCRIPTOR;

      ret = addEthernetLinkObject(ifname, lowerLayer, &ethLinkPathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not create EthernetLink object, ret=%d", ret);
         return ret;
      }

      /* Create fullPath string needed by IP.Interface lowerlayer */
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ethLinkPathDesc, &ethLinkFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }
   }

   /*
    * Now we can add the IP.Interface object
    */
   INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
   ipIntfPathDesc->oid = MDMOID_DEV2_IP_INTERFACE;
   if ((ret = mdm_addObjectInstance(ipIntfPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add IP.Interface Instance, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ethLinkFullPath);
      return ret;
   }

   if ((ret = mdm_getObject(ipIntfPathDesc->oid, &ipIntfPathDesc->iidStack, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get IP.Interface object, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ethLinkFullPath);
      return ret;
   }

   /* set the params in the newly created IP.Interface object */
   ipIntfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->name, ifname, mdmLibCtx.allocFlags);
   if (groupName)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_GroupName, groupName, mdmLibCtx.allocFlags);
   }

   ipIntfObj->X_BROADCOM_COM_Upstream = isUpstream;
   ipIntfObj->X_BROADCOM_COM_BridgeService = isBridgeService;
   ipIntfObj->X_BROADCOM_COM_BridgeNeedsIpAddr = isBridgeIpAddrNeeded;
   if (referedBridgeName)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_BridgeName, referedBridgeName, mdmLibCtx.allocFlags);
   }

   /* what about IPv6 only, should we only conditionally enable IPv4?  */
   ipIntfObj->IPv4Enable = supportIpv4;

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   ipIntfObj->IPv6Enable = supportIpv6;
#endif

   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->lowerLayers, ethLinkFullPath, mdmLibCtx.allocFlags);
   CMSMEM_FREE_BUF_AND_NULL_PTR(ethLinkFullPath);

   if ((ret = mdm_setObject((void **) &ipIntfObj, &ipIntfPathDesc->iidStack,  FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipIntfObj. ret=%d", ret);
   }
   mdm_freeObject((void **)&ipIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethLinkObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2IpObject *ipObj=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_IP, &iidStack, (void **) &ipObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ipObj. ret=%d", ret);
         return ret;
      }

      ipObj->interfaceNumberOfEntries++;
      ret = mdm_setObject((void **) &ipObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&ipObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ipObj. ret=%d", ret);
      }
   }

   return ret;
}


CmsRet mdm_initIpObject_dev2(UBOOL8 supportIpv4 __attribute((unused)),
                             UBOOL8 supportIpv6 __attribute((unused)))
{
   Dev2IpObject *ipObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = mdm_getObject(MDMOID_DEV2_IP, &iidStack, (void **) &ipObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipObj. ret=%d", ret);
      return ret;
   }

   if (supportIpv4)
   {
      ipObj->IPv4Capable = TRUE;
      ipObj->IPv4Enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ipObj->IPv4Status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (supportIpv6)
   {
      ipObj->IPv6Capable = TRUE;
      ipObj->IPv6Enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ipObj->IPv6Status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }
#endif

   ret = mdm_setObject((void **) &ipObj, &iidStack,  FALSE);
   mdm_freeObject((void **)&ipObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipObj. ret=%d", ret);
   }
   return ret;
}


CmsRet addEthernetLinkObject(const char *ifname,
                             const char *lowerLayer,
                             MdmPathDescriptor *pathDesc)
{
   Dev2EthernetLinkObject *ethLinkObj=NULL;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Adding for ifname=%s lowerLayer=%s", ifname, lowerLayer);

   INIT_PATH_DESCRIPTOR(pathDesc);
   pathDesc->oid = MDMOID_DEV2_ETHERNET_LINK;
   if ((ret = mdm_addObjectInstance(pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_ETHERNET_LINK failed, ret = %d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &ethLinkObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get ethLinkObj object, ret=%d", ret);
      return ret;
   }

   ethLinkObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->name, ifname, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &ethLinkObj, &pathDesc->iidStack,  FALSE);
   mdm_freeObject((void **)&ethLinkObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethLinkObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2EthernetObject *ethObj=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_ETHERNET, &iidStack, (void **) &ethObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ethObj. ret=%d", ret);
         return ret;
      }

      ethObj->linkNumberOfEntries++;
      ret = mdm_setObject((void **) &ethObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&ethObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ethObj. ret=%d", ret);
      }
   }

   return ret;
}


#ifdef DMP_DEVICE2_ROUTING_1
CmsRet mdm_initRouterObject_dev2()
{
   Dev2RoutingObject *routingObj=NULL;
   Dev2RouterObject *routerObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("entered");

   ret = mdm_getObject(MDMOID_DEV2_ROUTING, &iidStack, (void **)&routingObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get DEV2_ROUTING obj");
      return ret;
   }

   if (routingObj->routerNumberOfEntries > 0)
   {
      /*
       * Don't know why TR181 has multiple router entries.  But if we have
       * at least 1 router entry, that is good enough.
       */
      mdm_freeObject((void **)&routingObj);
      return CMSRET_SUCCESS;
   }

   cmsLog_notice("add first instance of router!!");
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_ROUTER;
   pathDesc.iidStack = iidStack;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) == CMSRET_SUCCESS)
   {
      ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **)&routerObj);
      if (ret == CMSRET_SUCCESS)
      {
         routerObj->enable = TRUE;
         ret = mdm_setObject((void **)&routerObj, &pathDesc.iidStack,  FALSE);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to set DEV2_ROUTER obj, ret=%d", ret);
         }
         mdm_freeObject((void **)&routerObj);
      }
      else
      {
         cmsLog_error("Failed to get DEV2_ROUTER obj, ret=%d", ret);
      }
   }
   else
   {
      cmsLog_error("Failed to add DEV2_ROUTER instance, ret=%d", ret);
   }

   /* must manually update the count when adding objects during mdm_init */
   if (ret == CMSRET_SUCCESS)
   {
      routingObj->routerNumberOfEntries++;
      ret = mdm_setObject((void **) &routingObj, &iidStack,  FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set routingObj. ret=%d", ret);
      }
   }

   mdm_freeObject((void **)&routingObj);

   cmsLog_debug("done ");

   return ret;
}

#endif  /* DMP_DEVICE2_ROUTING_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
