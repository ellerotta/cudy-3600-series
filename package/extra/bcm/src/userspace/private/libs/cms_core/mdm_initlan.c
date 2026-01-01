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


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_net.h"
#include "cms_qdm.h"
#include "bcm_ethswutils.h"

static CmsRet addDefaultLanDeviceObject(void);
static CmsRet addLanEthInterfaceObject(const char *ifName,
                                const char *pGMACPortList,
                                const char *pWANOnlyPortList,
                                const char *pLANOnlyPortList);


#ifdef DMP_USBLAN_1
static CmsRet addLanUsbInterfaceObject(const char *ifName);
#endif

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
static CmsRet addPersistentWanWifiInterfaceObject(const char *ifName);
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
static CmsRet addDefaultL2BridgingEntryObject(void);
static CmsRet addDefaultL2BridgingFilterInterfaceObject(UINT32 intfKey, SINT32 bridgeRef);
CmsRet addDefaultL2BridgingAvailableInterfaceObject(const char *interfaceReference, SINT32 bridgeRef);
#endif



CmsRet mdm_addDefaultLanObjects(void)
{
   void *mdmObj=NULL;
   LanDevObject *lanDevObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *ifNames=NULL;
   char ifNameBuf[CMS_IFNAME_LENGTH];
   UINT32 end, c=0;
   UINT32 idx;
   CmsRet ret;
   char *pWANOnlyPortList=NULL;
   char *pLANOnlyPortList=NULL;
   char *pWanPreferredPortList=NULL;  /* WAN Preferred eth port list */

#ifdef DMP_BRIDGING_1
   L2BridgingObject *l2BridgingObj=NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
#endif

   ret = mdm_getNextObject(MDMOID_LAN_DEV, &iidStack, &mdmObj);
   if (ret == CMSRET_SUCCESS)
   {
      /*
       * if we detect a LANDevice, assume all devices have been added.
       * We do not go further and make sure every device is in the data model.
       * This means once you have booted the modem, you cannot add hardware
       * or enable extra lan drivers because this code will not detect it.
       *
       * That could be changed if needed though....
       */
      mdm_freeObject(&mdmObj);

      return CMSRET_SUCCESS;
   }

   if ((ret = addDefaultLanDeviceObject()) != CMSRET_SUCCESS)
   {
      return ret;
   }


   /*
    * We also need to update the various counters in LANDevice.  Get the object first
    * and then update the fields as we go along.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = mdm_getNextObject(MDMOID_LAN_DEV, &iidStack, (void **) &lanDevObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get LAN DEV object, ret=%d", ret);
      return ret;
   }

#ifdef DMP_BRIDGING_1
      INIT_INSTANCE_ID_STACK(&iidStack2);
      if ((ret = mdm_getObject(MDMOID_L2_BRIDGING, &iidStack2, (void **) &l2BridgingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get LAYER2 BRIDGING object, ret=%d", ret);
         return ret;
      }

   if ((ret = addDefaultL2BridgingEntryObject()) != CMSRET_SUCCESS)
   {
      return ret;
   }

      l2BridgingObj->bridgeNumberOfEntries = 1;
#endif

   /*
    * If there are any LAN/WAN ports (called Preferred WAN ports in driver) in the system, just get the list.
    */
   ethswUtil_getWanPreferredPortIfNameList(&pWanPreferredPortList);
   cmsLog_debug("LANWAN preferred PortList=%s", pWanPreferredPortList);
   /*
    * If there are WAN only Eth port list in the system, get that first.
    */
   ethswUtil_getWANOnlyEthPortIfNameList(&pWANOnlyPortList);
   cmsLog_debug("WANOnlyPortList=%s", pWANOnlyPortList);


   /*
    * If there are any LANOnly ports in the system, just get the list.
    */
   ethswUtil_getLANOnlyEthPortIfNameList(&pLANOnlyPortList);
   cmsLog_debug("LANOnlyPortList=%s", pLANOnlyPortList);

   /*
    * Now add ethernet, usb LAN devices based on what the kernel
    * tells us is there.
    */
   cmsNet_getIfNameList(&ifNames);
   if (ifNames == NULL)
   {
      cmsLog_error("no interfaces found during initialization!");
      return CMSRET_INTERNAL_ERROR;
   }
   end = strlen(ifNames);

   while (c < end)
   {
      idx = 0;
      while (c < end && ifNames[c] != ',')
      {
         ifNameBuf[idx] = ifNames[c];
         c++;
         idx++;
      }
      ifNameBuf[idx] = 0;
      c++;

      if (0 == cmsUtl_strncmp(ifNameBuf, "eth", 3))
      {
#ifndef G9991_COMMON
         addLanEthInterfaceObject(ifNameBuf, pWanPreferredPortList, pWANOnlyPortList, pLANOnlyPortList);
         lanDevObj->LANEthernetInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
         l2BridgingObj->filterNumberOfEntries++;
         l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
#endif  /* G9991_COMMON */
      }

#ifdef G9991_COMMON
      else if (0 == cmsUtl_strncmp(ifNameBuf, "sid", 3))
      {
          addLanEthInterfaceObject(ifNameBuf, pWanPreferredPortList, pWANOnlyPortList, pLANOnlyPortList);

          lanDevObj->LANEthernetInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
          l2BridgingObj->filterNumberOfEntries++;
          l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
      }
#endif  /* G9991_COMMON */


#ifdef DMP_USBLAN_1
      else if (0 == cmsUtl_strncmp(ifNameBuf, "usb", 3))
      {
         addLanUsbInterfaceObject(ifNameBuf);

         lanDevObj->LANUSBInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
         l2BridgingObj->filterNumberOfEntries++;
         l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
      }
#endif  /* DMP_USBLAN */
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1   	
      else if (0 == cmsUtl_strncmp(ifNameBuf, "wl", 2))
      {

         /*
          * If the the Wl is in the persistent wan list,
          * need to add that as the WAN device.
          * NOTE: Since we only support ONE wifi WAN for now,
          *       the first wl interface found in pWanPortList will be used and
          *       the rest of wl interfaces in the list will be ignored.
          */
         if (pWANOnlyPortList &&  cmsUtl_isSubOptionPresent(pWANOnlyPortList, ifNameBuf))
         {
            addPersistentWanWifiInterfaceObject(ifNameBuf);
         }
      }
#endif
  }

   /* Free the LAN interface name list */
   cmsMem_free(ifNames);

   /* No long needed, free the interface name lists. */
   free(pWANOnlyPortList);
   free(pWanPreferredPortList);
   free(pLANOnlyPortList);

   /*
    * Now do the set to save the updated counts of the various interfaces.
    */
   if ((ret = mdm_setObject((void **) &lanDevObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set LAN Dev object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1
   if ((ret = mdm_setObject((void **) &l2BridgingObj, &iidStack2, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set Layer2 Bridging object, ret=%d", ret);
   }
#endif

   return ret;
}


CmsRet addDefaultLanDeviceObject(void)
{
   MdmPathDescriptor pathDesc;
   _IGDObject *igdObj=NULL;
   _LanHostCfgObject *lanHostCfgObj=NULL;
   _LanIpIntfObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("Creating initial LAN Device / br0 LAN interface");


   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_LAN_DEV;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create LANDevice.1, ret=%d", ret);
      return ret;
   }

   /*
    * We also need to update the LANDeviceNumberOfEntries parameter in InternetGatewayDevice.
    */
   if ((ret = mdm_getObject(MDMOID_IGD, &iidStack, (void **) &igdObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get IGD object, ret=%d", ret);
      return ret;
   }

   igdObj->LANDeviceNumberOfEntries = 1;

   if ((ret = mdm_setObject((void **) &igdObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IGD object, ret=%d", ret);
   }


   /*
    * Enable the dhcp server on this initial br0 LAN interface.
    * Also update the IPInterfaceNumberOfEntries since we will add a single IPInterface
    * later in this function.
    */
   if ((ret = mdm_getObject(MDMOID_LAN_HOST_CFG, &(pathDesc.iidStack), (void **) &lanHostCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get LAN_HOST_CFG, ret=%d", ret);
      return ret;
   }

#if defined(DHCP_SERVER_DEFAULT)
   lanHostCfgObj->DHCPServerEnable = TRUE;
#else
   lanHostCfgObj->DHCPServerEnable = FALSE;
#endif
   lanHostCfgObj->IPInterfaceNumberOfEntries = 1;

   if ((ret = mdm_setObject((void **) &lanHostCfgObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set LanHostCfg object, ret=%d", ret);
   }

   /*
    * Now add a single IP interface under the LAN device.
    * IP Interface name will be automatically assigned upon object creation,
    * we should get br0.
    */
   pathDesc.oid = MDMOID_LAN_IP_INTF;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create LANDevice.1.IPInterface.1, ret=%d", ret);
      return ret;
   }

   cmsLog_debug("created default LAN IPInterface at %s", cmsMdm_dumpIidStack(&(pathDesc.iidStack)));

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get IPInterface object, ret=%d", ret);
      return ret;
   }

   /* Enable IP intf */
   ipIntfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_IfName, "br0", mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &ipIntfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IPIntf object, ret=%d", ret);
   }

#ifndef SUPPORT_DM_PURE181
#if defined(DMP_DEVICE2_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1) //for hybrid IPv6
   {
      char ethLinkLowerLayer[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      char *ipIntfFullPath=NULL;
      MdmPathDescriptor ipIntfPathDesc;
      UBOOL8 supportIpv4=FALSE;
      UBOOL8 supportIpv6=TRUE;

      qdmEthLink_getEthLinkLowerLayerFullPathByName("br0", ethLinkLowerLayer, sizeof(ethLinkLowerLayer));

      ret = mdm_initIpObject_dev2(supportIpv4, supportIpv6);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("initIpObject failed, ret=%d", ret);
         return ret;
      }

      /* create IP.Interface object here */
      ret = mdm_addDefaultLanIpInterfaceObject_dev2("br0", "Default",
                                              supportIpv4, supportIpv6,
                                              ethLinkLowerLayer,
                                              &ipIntfPathDesc);

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

      /* IPv6 needs DHCP server and Router Advertisement on LAN side */
      mdm_addDefaultDhcpv6ServerObjects_dev2(ipIntfFullPath);
      mdm_addDefaultRouterAdvertisementObjects_dev2(ipIntfFullPath);

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   }
#endif
#endif /* SUPPORT_DM_PURE181 */

   return ret;
}

CmsRet addLanEthInterfaceObject(const char *ifName,
                                const char *pGMACPortList,
                                const char *pWANOnlyPortList,
                                const char *pLANOnlyPortList)
{
   MdmPathDescriptor pathDesc;
   _LanEthIntfObject *ethObj=NULL;
   char savedEthPortAttribute[BUFLEN_32]={0};
   CmsRet ret;

   cmsLog_debug("adding LAN %s intf object, pGMACPortList=%s, pWANOnlyPortList=%s,  pLANOnlyPortList=%s",
      ifName, pGMACPortList, pWANOnlyPortList, pLANOnlyPortList);

   INIT_PATH_DESCRIPTOR(&pathDesc);

   pathDesc.oid = MDMOID_LAN_ETH_INTF;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default eth interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create default %s interface, ret=%d", ifName, ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &ethObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get eth intf object, ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */
   ethObj->enable = TRUE;

   CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);

   if (cmsUtl_isSubOptionPresent(pGMACPortList, ethObj->X_BROADCOM_COM_IfName))
   {
      /* X_BROADCOM_COM_GMAC_Enabled is used for backward compatibilty and will be depreciated later on */
      ethObj->X_BROADCOM_COM_GMAC_Enabled = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANPREPERRED,  mdmLibCtx.allocFlags);
   }

   if (cmsUtl_isSubOptionPresent(pWANOnlyPortList, ethObj->X_BROADCOM_COM_IfName))
   {
      CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY,  mdmLibCtx.allocFlags);
   }
   else if (cmsUtl_isSubOptionPresent(pLANOnlyPortList, ethObj->X_BROADCOM_COM_IfName))
   {
      CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_LANONLY,  mdmLibCtx.allocFlags);
   }

   /* after mdm_setObject, ethObj->X_BROADCOM_COM_WanLan_Attribute will not be accessable, so save it */
   cmsUtl_strncpy(savedEthPortAttribute, ethObj->X_BROADCOM_COM_WanLan_Attribute, sizeof(savedEthPortAttribute));

   if ((ret = mdm_setObject((void **) &ethObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set eth intf object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1
   /* For not wan only ports */
   if (cmsUtl_strcmp(savedEthPortAttribute, MDMVS_WANONLY))
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("addDefaultL2BridgingAvailableInterfaceObject returns error. ret=%d", ret);
         return ret;
      }
   }
#endif
   cmsLog_debug("ret %d", ret);

   return ret;
}

#ifdef DMP_USBLAN_1
CmsRet addLanUsbInterfaceObject(const char *ifName)
{
   MdmPathDescriptor pathDesc;
   _LanUsbIntfObject *usbObj=NULL;
   CmsRet ret;

   cmsLog_notice("adding LAN %s intf object", ifName);

   INIT_PATH_DESCRIPTOR(&pathDesc);

   pathDesc.oid = MDMOID_LAN_USB_INTF;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default usb interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create default USB interface, ret=%d", ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &usbObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get USB intf object, ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */
   usbObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(usbObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &usbObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set USB intf object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);
   }
#endif

   cmsLog_debug("done, ret=%d", ret);

   return ret;
}
#endif /* DMP_USBLAN_1 */


#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */

CmsRet addDefaultL2BridgingEntryObject(void)
{
   CmsRet                ret        = CMSRET_SUCCESS;
   MdmPathDescriptor     pathDesc;
   _L2BridgingEntryObject *bridgeObj=NULL;


   cmsLog_notice("Adding default L2BridgingEntry");


   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_ENTRY;

   /* add new instance of L2BridgingObject */
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new L2BridgingEntry, ret=%d", ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &bridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingEntryObject, ret=%d", ret);
      return ret;
   }


   /*
    * Set the object values here.
    */
   CMSMEM_REPLACE_STRING_FLAGS(bridgeObj->bridgeName, "Default", mdmLibCtx.allocFlags);
   bridgeObj->bridgeEnable = TRUE;

   /* set the L2BridgingEntryObject */
   ret = mdm_setObject((void **)&bridgeObj, &(pathDesc.iidStack), FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingEntryObject, ret = %d", ret);
   }

   return ret;
}

CmsRet addDefaultL2BridgingFilterInterfaceObject(UINT32 intfKey, SINT32 bridgeRef)
{
   CmsRet                 ret = CMSRET_SUCCESS;
   MdmPathDescriptor      pathDesc;
   L2BridgingFilterObject *filterObj=NULL;
   char filterInterface[BUFLEN_32];

   cmsLog_debug("interface key %d", intfKey);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_FILTER;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create instance of bridge filter object");
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(MDMOID_L2_BRIDGING_FILTER, &(pathDesc.iidStack), (void **) &filterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingEntryObject, ret=%d", ret);
      return ret;
   }

   /* populate the fields of the bridge filter object */

   filterObj->filterEnable = TRUE;
   filterObj->filterBridgeReference = bridgeRef;
   sprintf(filterInterface, "%u", intfKey);
   CMSMEM_REPLACE_STRING_FLAGS(filterObj->filterInterface, filterInterface, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **)&filterObj, &(pathDesc.iidStack), FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingFilterObject, ret = %d", ret);
   }

   cmsLog_debug("done, ret=%d", ret);

   return ret;
}


CmsRet addDefaultL2BridgingAvailableInterfaceObject(const char *interfaceReference, SINT32 bridgeRef)
{
   CmsRet                 ret = CMSRET_SUCCESS;
   MdmPathDescriptor      pathDesc;
   L2BridgingIntfObject   *availIntfObj=NULL;

   cmsLog_debug("interfaceReference=%s", interfaceReference);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_INTF;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create instance of available interfaces object");
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(MDMOID_L2_BRIDGING_INTF, &(pathDesc.iidStack), (void **) &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingAvailableInterfaceObject, ret=%d", ret);
      return ret;
   }

   /* During adjust for hardware, all available interfaces are LAN interfaces */
   CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceType, MDMVS_LANINTERFACE, mdmLibCtx.allocFlags);

   CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceReference, interfaceReference, mdmLibCtx.allocFlags);


   /*
    * add the filter entry associated with this available interface entry.
    * We are supposed to pass in the availIntfObj->availableInterfaceKey, but
    * because we are doing this inside the mdm, the availableInterfaceKey has not
    * been set by the rcl_l2BridgingIntfObject() yet.  Since I know that function
    * uses the instance number as the key, I can use the key here also.
    */
   addDefaultL2BridgingFilterInterfaceObject(PEEK_INSTANCE_ID(&(pathDesc.iidStack)), bridgeRef);

   if ((ret = mdm_setObject((void **)&availIntfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingIntfObject, ret = %d", ret);
   }

   cmsLog_debug("done, ret=%d", ret);

   return ret;
}

#endif  /* DMP_BRIDGING_1 aka SUPPORT_PORT_MAP */


#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
CmsRet addPersistentWanWifiInterfaceObject(const char *ifName)
{
   _WanWifiIntfObject *wanWifiObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("Enabling persistent WAN Wifi %s intf object", ifName);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_WIFI_INTF;
   /* default WAN Wl interface always under WANDevice.7 */
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), CMS_WANDEVICE_WIFI);

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wanWifiObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get wifi intf object, ret=%d", ret);
      return ret;
   }

   /* Only do this once */
   if (!wanWifiObj->enable)
   {
      /* set the intf name and enable */
      wanWifiObj->enable = TRUE;

      wanWifiObj-> persistentDevice= TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(wanWifiObj->connectionMode, MDMVS_VLANMUXMODE, mdmLibCtx.allocFlags);

      CMSMEM_REPLACE_STRING_FLAGS(wanWifiObj->ifName, ifName, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &wanWifiObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set wifi intf object, ret=%d", ret);
      }

      /* Free the wanWifi obect */
      mdm_freeObject((void **) &wanWifiObj);

      /*
       * Also create a single WANConnectionDevice in this WANDevice.
       */
      pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
      }
   }

   return ret;

}

#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */
