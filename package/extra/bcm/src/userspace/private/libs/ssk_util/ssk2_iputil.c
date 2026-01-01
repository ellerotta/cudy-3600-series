/*
* <:copyright-BRCM:2013:proprietary:standard
*
*    Copyright (c) 2013 Broadcom
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
* :>
*/

#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "prctl.h"
#include "cms_core.h"
#include "ssk_util.h"


/*!\file ssk2_iputil.c
 * \brief Helper functions for the interface stack dealing with IP object.
 */


void setIpv4ServiceStatusByFullPathLocked(const char *ipIntfFullPath, const char *serviceStatus)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                   ipIntfFullPath, ret);
      return;
   }

   if (pathDesc.oid != MDMOID_DEV2_IP_INTERFACE)
   {
      cmsLog_error("fullPath (%s) must point to IP.Interface", ipIntfFullPath);
      return;
   }

   setIpv4ServiceStatusByIidLocked(&pathDesc.iidStack, serviceStatus);

   return;
}


void setIpv4ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack, const char *serviceStatus)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_DEV2_IP_INTERFACE, ipIntfIidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **)&ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get IP.Interface object, ret=%d", ret);
      return;
   }

   if (ipIntfObj->IPv4Enable)
   {
      CMSMEM_REPLACE_STRING(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, serviceStatus);

      if ((ret = cmsObj_set(ipIntfObj, ipIntfIidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Set of IP.Interface object failed, ret=%d", ret);
      }
   }

   cmsObj_free((void **) &ipIntfObj);

   return;
}


void getIpv4ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack,
                                     char *serviceStatus, UINT32 bufLen)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_DEV2_IP_INTERFACE, ipIntfIidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **)&ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Could not get IP.Interface object at %s, ret=%d",
                   cmsMdm_dumpIidStack(ipIntfIidStack), ret);
      /* For intf grouping, the entire IP.Interface subtree could get
       * deleted, so we cannot see the IP.Interface status.  Just report a
       * DOWN status.
       */
      cmsUtl_strncpy(serviceStatus, MDMVS_DOWN, bufLen);
      return;
   }

   if (cmsUtl_strlen(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus) >= (SINT32) bufLen)
   {
      cmsLog_error("status %s cannot fit into bufLen %d",
            ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, bufLen);
   }
   else
   {
      strcpy(serviceStatus, ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus);
   }

   cmsObj_free((void **) &ipIntfObj);

   return;
}


CmsRet getIpIntfByFullPath(const char *ipIntfFullPath,
                           InstanceIdStack *ipIntfIidStack,
                           Dev2IpInterfaceObject **ipIntfObj)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                   ipIntfFullPath, ret);
      return ret;
   }

   if (pathDesc.oid != MDMOID_DEV2_IP_INTERFACE)
   {
      cmsLog_error("fullPath (%s) must point to IP.Interface", ipIntfFullPath);
      return CMSRET_INVALID_PARAM_NAME;
   }

   *ipIntfIidStack = pathDesc.iidStack;

   if ((ret = cmsObj_get(MDMOID_DEV2_IP_INTERFACE, ipIntfIidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get IP.Interface object, ret=%d", ret);
      return ret;
   }

   return ret;
}


void sskConn_setPppConnStatusByFullPathLocked(const char *pppIntfFullPath, const char *connStatus)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(pppIntfFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                   pppIntfFullPath, ret);
      return;
   }

   if (pathDesc.oid != MDMOID_DEV2_PPP_INTERFACE)
   {
      cmsLog_error("fullPath %s must point to PPP.Interface", pppIntfFullPath);
      return;
   }

   sskConn_setPppConnStatusByIidLocked(&pathDesc.iidStack, connStatus);

   return;
}


void sskConn_setPppConnStatusByIidLocked(const InstanceIdStack *pppIntfIidStack, const char *connStatus)
{
   Dev2PppInterfaceObject *pppIntfObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entered: iidStack=%s connStatus=%s",
                cmsMdm_dumpIidStack(pppIntfIidStack), connStatus);

   if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE, pppIntfIidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **)&pppIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get PPP.Interface object, ret=%d", ret);
      return;
   }

   sskConn_setPppConnStatusByObjLocked(pppIntfIidStack, pppIntfObj, connStatus);

   cmsObj_free((void **) &pppIntfObj);

   return;
}


void sskConn_setPppConnStatusByObjLocked(const InstanceIdStack *iidStack,
                      Dev2PppInterfaceObject *pppIntfObj,
                      const char *connStatus)
{
   CmsRet ret;

   CMSMEM_REPLACE_STRING(pppIntfObj->connectionStatus, connStatus);

   if ((ret = cmsObj_set(pppIntfObj, iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of PPP.Interface object failed, ret=%d", ret);
   }

   return;
}


UBOOL8 sskConn_hasStaticIpv4AddressLocked(const InstanceIdStack *ipIntfIidStack)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
   UBOOL8 hasAddr=FALSE;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   while (!hasAddr &&
          ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_ADDRESS,
                              ipIntfIidStack,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv4AddrObj)) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(ipv4AddrObj->addressingType, MDMVS_STATIC) &&
          !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv4AddrObj->IPAddress))
      {
         hasAddr = TRUE;
      }
      cmsObj_free((void **) &ipv4AddrObj);
   }

   return hasAddr;
}


UBOOL8 sskConn_hasAnyIpv4AddressLocked(const InstanceIdStack *ipIntfIidStack)
{
   return (sskConn_getAnyIpv4AddressLocked(ipIntfIidStack, NULL, NULL));
}

UBOOL8 sskConn_getAnyIpv4AddressLocked(const InstanceIdStack *ipIntfIidStack,
                                       char *ipv4AddrBuf, char *ipv4SubnetBuf)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
   UBOOL8 hasAddr=FALSE;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   while (!hasAddr &&
          ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_ADDRESS,
                              ipIntfIidStack,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv4AddrObj)) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv4AddrObj->IPAddress))
      {
         hasAddr = TRUE;
         if (ipv4AddrBuf != NULL)
         {
            // caller must provide buffer of sufficient length
            strcpy(ipv4AddrBuf, ipv4AddrObj->IPAddress);
         }
         if (ipv4SubnetBuf != NULL)
         {
            // caller must provide buffer of sufficient length
            strcpy(ipv4SubnetBuf, ipv4AddrObj->subnetMask);
         }
      }
      cmsObj_free((void **) &ipv4AddrObj);
   }

   cmsLog_debug("returning %d", hasAddr);
   return hasAddr;
}


CmsRet sskConn_getIpv4GatewayForIpIntfLocked(const char *ipIntfFullpath,
                                             char *ipv4Gateway)
{

   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv4ForwardingObject *ipv4ForwardingObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: ipIntfFullPath=%s", ipIntfFullpath);

   // Scan through all forwarding objects looking for match to ipIntfFullPath
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_IPV4_FORWARDING,
                            &iidStack, OGF_NO_VALUE_UPDATE,
                            (void **) &ipv4ForwardingObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(ipv4ForwardingObj->interface, ipIntfFullpath))
      {
          // got match, copy out info
          cmsUtl_strcpy(ipv4Gateway, ipv4ForwardingObj->gatewayIPAddress);
          found = TRUE;
      }
      cmsObj_free((void **) &ipv4ForwardingObj);
   }

   return (found ? CMSRET_SUCCESS : ret);
}


// from rut2_dns.c
extern CmsRet rutNtwk_getIpvxDnsServersFromIfName_dev2(UINT32 ipvx,
                                                       const char *ifName,
                                                       char *DNSServers);

CmsRet sskConn_getDnsServersForIpIntfLocked(const char *ipIntfName,
                                            char *dnsServers)
{
   CmsRet ret;

   cmsLog_debug("Entered: ipIntfName=%s", ipIntfName);

   // This function only returns the first dynamic IPv4 or IPv6 DNS server
   // found.  Good enough for now.
   ret = rutNtwk_getIpvxDnsServersFromIfName_dev2(CMS_AF_SELECT_IPVX,
                                                  ipIntfName, dnsServers);
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("[%s] %s", ipIntfName, dnsServers);
   }
   else
   {
      cmsLog_debug("Could not find any DNS servers for %s", ipIntfName);
   }
   return ret;
}


#endif  /* DMP_DEVICE2_BASELINE_1 */
