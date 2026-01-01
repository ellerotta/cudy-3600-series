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
#include "linux/device2/rut2_configfile.h"
#include "rut_util.h"

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

/* in mdm2_initdsl.c */
CmsRet mdm_addDefaultDslObjects_dev2(void);

/* in mdm2_initwifi.c */
CmsRet mdm_initWifiAccessPoint_dev2(void);
CmsRet mdm_initWifiAfc_dev2(void);

#ifdef DMP_DEVICE2_SM_BASELINE_1
CmsRet mdm_addDefaultModSwObjects(void);
#else
void mdm_removeBeepDatabase(void);
#endif


/* in mdm2_init_devinfo.c */
extern CmsRet mdm_adjustForHardware_dev2_devinfo(void);


/** This is the "strong" or "main" mdm_adjustForHardware_dev2.
  * It is only used in the CMS Classic build, where it will call all the 
  * adjustForHardware functions for the entire system (monolithic MDM).
  * This function is not called in the Distributed MDM build, so the various
  * weak mdm_adjustForHardware_dev2's (in the various mdm_cbk_xxx libs) will be
  * called.
  */
CmsRet mdm_adjustForHardware_dev2(void)
{
   MdmPathDescriptor mgmtBrPortPathDesc;
   MdmPathDescriptor ipIntfPathDesc;
   CmsRet ret=CMSRET_SUCCESS;
   char br0IpAddrBuf[CMS_IPADDR_LENGTH]={0};
   char br0NetMaskBuf[CMS_IPADDR_LENGTH]={0};

   cmsLog_notice("Entered===> (CMS Classic build)");
   // This main adjustForHardware will call all the
   // adjustForHardware functions in the other shared libs (mdm_cbk_*).

   /* devinfo initialization */
   ret = mdm_adjustForHardware_dev2_devinfo();
   RETURN_IF_NOT_SUCCESS(ret);

#ifdef DMP_DEVICE2_DSL_1
   ret = mdm_addDefaultDslObjects_dev2();
   RETURN_IF_NOT_SUCCESS(ret);
#endif

#ifdef DMP_DEVICE2_ROUTING_1
   ret = mdm_initRouterObject_dev2();
   RETURN_IF_NOT_SUCCESS(ret);
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

#ifdef DMP_DEVICE2_SM_BASELINE_1
   ret = mdm_addDefaultModSwObjects();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add software module management object! ret=%d", ret);
      return ret;
   }
#else
   mdm_removeBeepDatabase();
#endif

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
   /* init Wifi on LAN side, meaning Access Point */
   /* or should it do both LAN side and WAN side at the same time ? */
   if ((ret = mdm_initWifiAccessPoint_dev2()) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_initWifiAccessPoint_dev2 failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_initWifiAfc_dev2()) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_initWifiAfc_dev2 failed, ret=%d", ret);
      return ret;
   }
#endif

   /* XXX later add a DMP_DEVICE2_WIFIENDPOINT for Wifi as WAN */


#ifdef BRCM_VOICE_SUPPORT
   ret = mdm_adjustForVoiceHardware();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_adjustForVoiceHardware failed, ret=%d", ret);
      return ret;
   }
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

   cmsLog_notice("====> returning ret=%d", ret);
   return ret;
}

#endif  /* DMP_DEVICE2_BASELINE_1 */


#ifndef DMP_DEVICE2_SM_BASELINE_1

#include "cms_fil.h"
#include "beep_common.h"

void mdm_removeBeepDatabase(void)
{
   /* it's NON-BEEP image --> remove BEEP database */
   if (cmsFil_isFilePresent(BEEP_DB_FILE))
   {
      unlink(BEEP_DB_FILE);
   }
}

#endif
