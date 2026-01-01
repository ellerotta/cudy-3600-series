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


/*!\file mdm2_initlan.c
 * \brief MDM initialization for PURE181 Device based tree,
 *        LAN side IPv4 objects.
 *
 */


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "qdm_intf.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_net.h"
#include "bcm_net.h"
#include "bcm_ethswutils.h"

/* local functions */
static CmsRet addBridgeObject(MdmPathDescriptor *pathDesc);

static CmsRet addEthernetInterfaceObject(const MdmPathDescriptor *brPathDesc,
                            const char *ifname,
                            UBOOL8 isUpstream,
                            UBOOL8 isGMAC,
                            UBOOL8 isLanOnly,
                            MdmPathDescriptor *brPortpathDesc);

#ifdef DMP_DEVICE2_USBINTERFACE_1
static CmsRet addUsbInterfaceObject(const MdmPathDescriptor *brPathDesc,
                            const char *ifname,
                            UBOOL8 isUpstream,
                            MdmPathDescriptor *brPortPathDesc);
#endif  /* DMP_DEVICE2_USBINTERFACE_1 */

CmsRet mdm_addDefaultLanBridgeObjects_dev2(MdmPathDescriptor *mgmtBrPortPathDesc)
{
   MdmPathDescriptor brPathDesc;
   Dev2BridgeObject *brObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *ifNames=NULL;
   char ifNameBuf[CMS_IFNAME_LENGTH];
   UINT32 end, c=0;
   UINT32 idx;
   CmsRet ret;
   char *pWANOnlyPortList=NULL;
   char *pLANOnlyPortList=NULL;
   char *pWanPreferredPortList=NULL;

   INIT_PATH_DESCRIPTOR(mgmtBrPortPathDesc);

   /*
    * If we find a Bridge object, then this is not a blank MDM, so we should
    * do nothing.  But ideally, we should
    * verify that all the hardware reported by the hardware is present
    * in the MDM in case some new hardware was added
    */
   ret = mdm_getNextObject(MDMOID_DEV2_BRIDGE, &iidStack, (void **) &brObj);
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("Existing bridge object %s found, just return", brObj->X_BROADCOM_COM_IfName);
      mdm_freeObject((void **) &brObj);
      return CMSRET_SUCCESS;
   }


   /*
    * If we get here, then we need to add the default br0 bridge and all
    * associated LAN side devices.  First create the Bridge and its
    * management port.
    */
   INIT_PATH_DESCRIPTOR(&brPathDesc);
   ret = addBridgeObject(&brPathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create Bridge obj, ret=%d", ret);
      return ret;
   }

   ret = mdmInit_addBridgePortObject_dev2(&brPathDesc,
                                          "br0", TRUE, "",
                                          mgmtBrPortPathDesc);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to add management bridge port, ret=%d", ret);
   }

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
      MdmPathDescriptor brPortPathDesc;
      UBOOL8 isUpstream;

      INIT_PATH_DESCRIPTOR(&brPortPathDesc);
      isUpstream = FALSE;

      idx = 0;
      while (c < end && ifNames[c] != ',')
      {
         ifNameBuf[idx] = ifNames[c];
         c++;
         idx++;
      }
      ifNameBuf[idx] = 0;
      c++;
      cmsLog_debug("ifNameBuf=%s", ifNameBuf);

      if (0 == cmsUtl_strncmp(ifNameBuf, "eth", 3))
      {
         UBOOL8 isGMAC;
         UBOOL8 isLanOnly;

         isGMAC = cmsUtl_isSubOptionPresent(pWanPreferredPortList, ifNameBuf);
         isUpstream = cmsUtl_isSubOptionPresent(pWANOnlyPortList, ifNameBuf);
         isLanOnly= cmsUtl_isSubOptionPresent(pLANOnlyPortList, ifNameBuf);

         cmsLog_debug("Add Ethernet.Interface %s isGMAC=%d isLanOnly=%d  isUpstream=%d",
                       ifNameBuf, isGMAC, isLanOnly, isUpstream);

         ret = addEthernetInterfaceObject(&brPathDesc, ifNameBuf,
                                          isUpstream, isGMAC, isLanOnly, &brPortPathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to add Ethernet.Interface object, ret=%d", ret);
         }
      }
#ifdef DMP_DEVICE2_USBINTERFACE_1
      else if (0 == cmsUtl_strncmp(ifNameBuf, "usb", 3))
      {
         ret = addUsbInterfaceObject(&brPathDesc, ifNameBuf, FALSE, &brPortPathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to add USB.Interface object, ret=%d", ret);
         }
      }
#endif  /* DMP_DEVICE2_USBINTERFACE_1 */


#ifdef DEAL_WITH_LATER
// TODO: is this still needed?  The same code is also present in mdm_initlan.c

#ifdef DMP_X_BROADCOM_COM_DEV2_WIFIWAN_1
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

#endif  /* DEAL_WITH_LATER */

      /*
       * Ignore plc in this loop.  It is detected and added from
       * mdm_adjustForHardwareLocked_dev2.
       */

   }  // end of while loop over all ifnames reported by kernel


   /* Free the LAN interface name list */
   cmsMem_free(ifNames);

   /* No long needed, free the interface name list. */
   free(pWANOnlyPortList);
   free(pWanPreferredPortList);
   free(pLANOnlyPortList);

   return ret;
}


CmsRet mdm_addIpv4AddressObject_dev2(const MdmPathDescriptor *ipIntfPathDesc,
                            const char *ipv4Addr,
                            const char *netmask,
                            const char *addressingType)
{
   Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_IPV4_ADDRESS;
   pathDesc.iidStack = ipIntfPathDesc->iidStack;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for IPV4_ADDRESS failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &ipv4AddrObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get ipv4AddrObj object, ret=%d", ret);
      return ret;
   }
   ipv4AddrObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipv4AddrObj->IPAddress, ipv4Addr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv4AddrObj->subnetMask, netmask, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv4AddrObj->addressingType, addressingType, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &ipv4AddrObj, &pathDesc.iidStack,  FALSE);
   mdm_freeObject((void **)&ipv4AddrObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipv4AddrObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2IpInterfaceObject *ipIntfObj=NULL;

      if ((ret = mdm_getObject(MDMOID_DEV2_IP_INTERFACE, &ipIntfPathDesc->iidStack, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ipIntfObj. ret=%d", ret);
         return ret;
      }

      ipIntfObj->IPv4AddressNumberOfEntries++;
      ret = mdm_setObject((void **) &ipIntfObj, &ipIntfPathDesc->iidStack, FALSE);
      mdm_freeObject((void **)&ipIntfObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ipIntfObj. ret=%d", ret);
      }
   }

   return ret;
}


CmsRet mdm_addDefaultDhcpv4ServerObjects_dev2(const char *ipIntfFullPath,
                            const char *ipv4Addr,
                            const char *netmask)
{
   Dev2Dhcpv4ServerObject *dhcpv4ServerObj = NULL;
   Dev2Dhcpv4ServerPoolObject *dhcpv4ServerPoolObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char minIpAddrBuf[CMS_IPADDR_LENGTH]={0};
   char maxIpAddrBuf[CMS_IPADDR_LENGTH]={0};
   char *ptr = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   char nodhcp[BUFLEN_8];

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = mdm_getObject(MDMOID_DEV2_DHCPV4_SERVER, &iidStack, (void **) &dhcpv4ServerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get dhcpv4ServerObj. ret=%d", ret);
      return ret;
   }
   if (bcmNet_getNodhcpVar(nodhcp,BUFLEN_8))
   {
      if (cmsUtl_strcasecmp(nodhcp,"true") == 0)
      {
         dhcpv4ServerObj->enable = FALSE;
      }
      else
      {
         dhcpv4ServerObj->enable = TRUE;
      }
   }
   else {
#if defined(DHCP_SERVER_DEFAULT)
      dhcpv4ServerObj->enable = TRUE;
#else   
      dhcpv4ServerObj->enable = FALSE;
#endif
   }
   ret = mdm_setObject((void **) &dhcpv4ServerObj, &iidStack,  FALSE);
   mdm_freeObject((void **)&dhcpv4ServerObj);

   INIT_PATH_DESCRIPTOR(&pathDesc);

   pathDesc.oid = MDMOID_DEV2_DHCPV4_SERVER_POOL;
   pathDesc.iidStack = iidStack;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_DHCPV4_SERVER_POOL failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dhcpv4ServerPoolObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get dhcpv4ServerPoolObj object, ret=%d", ret);
      return ret;
   }

   cmsUtl_strncpy(minIpAddrBuf, ipv4Addr, CMS_IPADDR_LENGTH);
   ptr = strrchr(minIpAddrBuf, '.');
   if ( ptr ) // xxx.xxx.xxx.2
   {
      *(ptr+1) = '2';
      *(ptr+2) = '\0';
   }
   cmsUtl_strncpy(maxIpAddrBuf, ipv4Addr, CMS_IPADDR_LENGTH);
   ptr = strrchr(maxIpAddrBuf, '.');
   if ( ptr ) // xxx.xxx.xxx.254
   {
      *(ptr+1) = '2';
      *(ptr+2) = '5';
      *(ptr+3) = '4';
      *(ptr+4) = '\0';
   }

   dhcpv4ServerPoolObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ServerPoolObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ServerPoolObj->minAddress, minIpAddrBuf, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ServerPoolObj->maxAddress, maxIpAddrBuf, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ServerPoolObj->subnetMask, netmask, mdmLibCtx.allocFlags);
   // DNSserver determined when udhcpd.conf is written out.  Don't set at init time.
   // CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ServerPoolObj->DNSServers, "192.168.1.1", mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ServerPoolObj->IPRouters, ipv4Addr, mdmLibCtx.allocFlags);
   dhcpv4ServerPoolObj->leaseTime = 24 * 60 * 60;   // 24 hours

   ret = mdm_setObject((void **) &dhcpv4ServerPoolObj, &pathDesc.iidStack,  FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set dhcpv4ServerPoolObj. ret=%d", ret);
      mdm_freeObject((void **)&dhcpv4ServerPoolObj);
      return ret;
   }

   /* need to manually update the count when adding objects during mdm_init */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = mdm_getObject(MDMOID_DEV2_DHCPV4_SERVER, &iidStack, (void **) &dhcpv4ServerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get dhcpv4ServerObj. ret=%d", ret);
      return ret;
   }

   dhcpv4ServerObj->poolNumberOfEntries++;

   ret = mdm_setObject((void **) &dhcpv4ServerObj, &iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set dhcpv4ServerObj. ret=%d", ret);
      mdm_freeObject((void **)&dhcpv4ServerObj);
   }

   return ret;
}


#ifdef DMP_DEVICE2_DHCPV4RELAY_1
void mdm_configDhcpv4RelayObject_dev2(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Dhcpv4RelayObject *relayObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entered:");

   if ((ret = mdm_getObject(MDMOID_DEV2_DHCPV4_RELAY, &iidStack, (void **) &relayObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get DHCPV4_RELAY object, ret=%d", ret);
      return;
   }

   /* If the feature is enabled, then just set Enabled=TRUE and status=Enabled */
   relayObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(relayObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);


   ret = mdm_setObject((void **) &relayObj, &iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set DHCPV4_RELAY obj. ret=%d", ret);
      mdm_freeObject((void **)&relayObj);
   }
}
#endif  /* DMP_DEVICE2_DHCPV4RELAY_1 */


#ifdef DMP_DEVICE2_DHCPV4_1

CmsRet mdmInit_addDhcpv4ClientObject_dev2(const char *ipIntfFullPath)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;

   Dev2Dhcpv4ClientObject *dhcpv4ClientObj=NULL;

   CmsRet ret;

   pathDesc.oid = MDMOID_DEV2_DHCPV4_CLIENT;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_DHCPV4_CLIENT failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dhcpv4ClientObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get dhcpv4ClientObj object, ret=%d", ret);
      return ret;
   }

   dhcpv4ClientObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(dhcpv4ClientObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &dhcpv4ClientObj, &pathDesc.iidStack,  FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set dhcpv4ClientObj. ret=%d", ret);
      mdm_freeObject((void **)&dhcpv4ClientObj);
      return ret;
   }

   /* need to manually update the count when adding objects during mdm_init */
   {
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
      Dev2Dhcpv4Object *dhcpv4Obj=NULL;

      if ((ret = mdm_getObject(MDMOID_DEV2_DHCPV4, &iidStack, (void **) &dhcpv4Obj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get dhcpv4Obj. ret=%d", ret);
         return ret;
      }

      dhcpv4Obj->clientNumberOfEntries++;

      ret = mdm_setObject((void **) &dhcpv4Obj, &iidStack, FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set dhcpv4Obj. ret=%d", ret);
         mdm_freeObject((void **)&dhcpv4Obj);
      }
   }

   return ret;
}

#endif  /* DMP_DEVICE2_DHCPV4_1 */


CmsRet addBridgeObject(MdmPathDescriptor *pathDesc)
{
   Dev2BridgeObject *brObj=NULL;
   CmsRet ret;

   cmsLog_notice("Adding Bridge object");

   INIT_PATH_DESCRIPTOR(pathDesc);
   pathDesc->oid = MDMOID_DEV2_BRIDGE;
   if ((ret = mdm_addObjectInstance(pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_BRIDGE failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &brObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get brObj object, ret=%d", ret);
      return ret;
   }

   brObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(brObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   brObj->X_BROADCOM_COM_Index = 0;

   ret = mdm_setObject((void **) &brObj, &pathDesc->iidStack, FALSE);
   mdm_freeObject((void **)&brObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set brObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2BridgingObject *bridgingObj=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_BRIDGING, &iidStack, (void **) &bridgingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get bridgingObj. ret=%d", ret);
         return ret;
      }

      bridgingObj->bridgeNumberOfEntries++;
      ret = mdm_setObject((void **) &bridgingObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&bridgingObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set bridgingObj. ret=%d", ret);
      }
   }

   return ret;
}


CmsRet addEthernetInterfaceObject(const MdmPathDescriptor *brPathDesc,
                                  const char *ifname,
                                  UBOOL8 isUpstream,
                                  UBOOL8 isGMAC,
                                  UBOOL8 isLanOnly,
                                  MdmPathDescriptor *brPortPathDesc)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   Dev2EthernetInterfaceObject *ethIntfObj=NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(brPortPathDesc);  // only filled in for LAN Eth Intf

   /* create the Ethernet.Interface object */
   pathDesc.oid = MDMOID_DEV2_ETHERNET_INTERFACE;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_ETHERNET_INTERFACE failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &ethIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get ethIntfObj object, ret=%d", ret);
      return ret;
   }

   /* set the params in the newly created object */
   ethIntfObj->enable = TRUE;
   ethIntfObj->upstream = isUpstream;

   CMSMEM_REPLACE_STRING_FLAGS(ethIntfObj->name, ifname, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ethIntfObj->status, MDMVS_DORMANT, mdmLibCtx.allocFlags);

   if (isGMAC)
   {
      /* X_BROADCOM_COM_GMAC_Enabled is used for backward compatibilty and will be depreciated later on */
      ethIntfObj->X_BROADCOM_COM_GMAC_Enabled = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ethIntfObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANPREPERRED,  mdmLibCtx.allocFlags);
   }

   if (isLanOnly)
   {
      /* If this is LAN only port, need to overwrite the default LanAndWan attribute to LanOnly */
      CMSMEM_REPLACE_STRING_FLAGS(ethIntfObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_LANONLY, mdmLibCtx.allocFlags);
   }
   else if (isUpstream)
   {
      /* Need to overwrite the default LanAndWan attribute to WanOnly
      *
      *  NOTE: Need to set enable to FALSE since WANONLY port uses enable to show if it it configured or not. ie.
      *  1). enable == FALSE.  The enet port is floating, (not in br0) and is not configured for WAN enet port.
      *  2). enable == TRUE if only if this enet port is configured as layer 2 WAN port.
      */
      CMSMEM_REPLACE_STRING_FLAGS(ethIntfObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY, mdmLibCtx.allocFlags);
      ethIntfObj->enable = FALSE;
   }

   ret = mdm_setObject((void **) &ethIntfObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **)&ethIntfObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethIntfObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2EthernetObject *ethObj=NULL;
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_ETHERNET, &iidStack, (void **) &ethObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ethObj. ret=%d", ret);
         return ret;
      }

      ethObj->interfaceNumberOfEntries++;
      ret = mdm_setObject((void **) &ethObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&ethObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ethObj. ret=%d", ret);
      }
   }


   /*
    * If this is a LAN side Ethernet.Interface, create a Bridge.Port object
    * on top of the Ethernet.Interface object
    */
   if (!isUpstream)
   {
      char *fullPathString=NULL;

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

#ifdef SUPPORT_LANVLAN
      {
         char vlanIface[CMS_IFNAME_LENGTH] = {0};

         /* Add bridge port with default LANVLAN interface name */
         cmsUtl_strcpy(vlanIface, ifname);
         cmsUtl_strncat(vlanIface, CMS_IFNAME_LENGTH, ".0");
         ret = mdmInit_addBridgePortObject_dev2(brPathDesc,
                                                vlanIface, FALSE, fullPathString,
                                                brPortPathDesc);
      }
#else
      ret = mdmInit_addBridgePortObject_dev2(brPathDesc,
                                             ifname, FALSE, fullPathString,
                                             brPortPathDesc);
#endif
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to create Bridge.Port object, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
         return ret;
      }

      // Adding Qos Queues for this ethernet interface is now done in
      // rcl_dev2EthernetInterfaceObject

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
   }

   return ret;
}


#ifdef DMP_DEVICE2_USBINTERFACE_1

static CmsRet addUsbInterfaceObject(const MdmPathDescriptor *brPathDesc,
                                    const char *ifname,
                                    UBOOL8 isUpstream,
                                    MdmPathDescriptor *brPortPathDesc)
{
   MdmPathDescriptor pathDesc;
   Dev2UsbInterfaceObject *usbIntfObj = NULL;
   char *fullPathString = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   /* create the USB.Interface object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_USB_INTERFACE;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_USB_INTERFACE failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &usbIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get usbIntfObj object, ret=%d", ret);
      return ret;
   }

   /* set the params in the newly created object */
   usbIntfObj->enable = TRUE;
   usbIntfObj->upstream = isUpstream;
   CMSMEM_REPLACE_STRING_FLAGS(usbIntfObj->name, ifname, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(usbIntfObj->status, MDMVS_DORMANT, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &usbIntfObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **)&usbIntfObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set usbIntfObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2UsbObject *usbObj=NULL;
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_USB, &iidStack, (void **) &usbObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get usbObj. ret=%d", ret);
         return ret;
      }

      usbObj->interfaceNumberOfEntries++;
      ret = mdm_setObject((void **) &usbObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&usbObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set usbObj. ret=%d", ret);
      }
   }


   /* now create a Bridge.Port object on top of the USB.Link object */
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      return ret;
   }
   INIT_PATH_DESCRIPTOR(brPortPathDesc);
   ret = mdmInit_addBridgePortObject_dev2(brPathDesc,
                                          ifname, FALSE, fullPathString,
                                          brPortPathDesc);
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create Bridge.Port object, ret=%d", ret);
   }

   return ret;
}

#endif  /* DMP_DEVICE2_USBINTERFACE_1 */


#ifdef LATER_PORT_TO_TR181_OBJS

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

#if defined(DHCP_CLIENT_DEFAULT)
   /* Enable DHCP client by default */
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceAddressingType, MDMVS_DHCP, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceIPAddress, "0.0.0.0", mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceSubnetMask, "0.0.0.0", mdmLibCtx.allocFlags);
#endif

   if ((ret = mdm_setObject((void **) &ipIntfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IPIntf object, ret=%d", ret);
   }

   return ret;
}



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




#ifdef DMP_ETHERNETWAN_1_NOT_USED // Need deletion later on

UBOOL8 isEthInterfaceGMACEnabled(const char *ifName, const char *pGMACPortList)
{
   UBOOL8 isGMACEnabled = FALSE;

   if (pGMACPortList)
   {
      isGMACEnabled = cmsUtl_isSubOptionPresent(pGMACPortList, ifName);
   }

   return isGMACEnabled;
}

CmsRet addPersistentWanEthInterfaceObject(const char *ifName, const char *pGMACPortList)
{
   _WanEthIntfObject *wanEthObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("Enabling persistent WAN Eth %s intf object", ifName);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_ETH_INTF;
   /* default WAN eth interface always under WANDevice.3 */
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), CMS_WANDEVICE_ETHERNET);

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wanEthObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get eth intf object, ret=%d", ret);
      return ret;
   }

   /* Only do this once */
   if (!wanEthObj->enable)
   {
      /* set the intf name and enable */
      wanEthObj->enable = TRUE;

      wanEthObj->X_BROADCOM_COM_PersistentDevice = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->duplexMode, MDMVS_AUTO, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->maxBitRate, MDMVS_AUTO, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE, mdmLibCtx.allocFlags);

      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);

      wanEthObj->X_BROADCOM_COM_GMAC_Enabled = isEthInterfaceGMACEnabled(ifName, pGMACPortList);
      if ((ret = mdm_setObject((void **) &wanEthObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set eth intf object, ret=%d", ret);
      }

      /* Free the wanEnet obect */
      mdm_freeObject((void **) &wanEthObj);

      // Adding Qos Queues for this ethernet interface is now done in
      // rcl_dev2EthernetInterfaceObject

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

#endif /* DMP_ETHERNETWAN_1_NOT_USED */

#ifdef DMP_X_BROADCOM_COM_DEV2_WIFIWAN_1
CmsRet addPersistentWanWifiInterfaceObject(const char *ifName)
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_error("XXX not implemented yet! Enabling persistent WAN Wifi %s intf object", ifName);

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_DEV2_WIFIWAN_1 */

#endif  /* LATER_PORT_TO_TR181_OBJS */


#endif  /* DMP_DEVICE2_BASELINE_1 */


