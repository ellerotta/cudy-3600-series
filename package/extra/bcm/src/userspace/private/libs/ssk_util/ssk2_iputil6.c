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

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "prctl.h"
#include "ssk_util.h"
#include "cms_qdm.h"


/*!\file ssk2_iputil6.c.
 * \brief IPv6 helper functions for interface stack.
 */


/* FIXME: how to share rut functions? */
extern UBOOL8 rutIp_findIpv6Addr(const InstanceIdStack *iidStackIpIntf, const char *addr,
                                                  const char *origin, InstanceIdStack *iidStackIpv6Addr);
extern CmsRet rutIp_addIpv6Addr(const InstanceIdStack *iidStackIpIntf, const char *addr,
                                                  const char *origin, const char *prefix, int plt, int vlt);

extern UBOOL8 rutIp_findIpv6Prefix(const InstanceIdStack *iidStackIpIntf, const char *prefix,
                                                  const char *origin, const char *staticType, InstanceIdStack *iidStackIpv6Prefix);
extern CmsRet rutIp_addIpv6Prefix(const InstanceIdStack *iidStackIpIntf, const char *prefix,
                                                  const char *origin, const char *staticType, const char *parent,
                                                  const char *child, UBOOL8 onLink, UBOOL8 Autonomous,
                                                  int plt, int vlt, char *myPathRef, UINT32 pathLen);


void setIpv6ServiceStatusByFullPathLocked(const char *ipIntfFullPath, const char *serviceStatus)
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
      cmsLog_error("fullPath %s must point to IP.Interface", ipIntfFullPath);
      return;
   }

   setIpv6ServiceStatusByIidLocked(&pathDesc.iidStack, serviceStatus);

   return;
}


void setIpv6ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack, const char *serviceStatus)
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

   if (ipIntfObj->IPv6Enable)
   {
      CMSMEM_REPLACE_STRING(ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus, serviceStatus);

      if ((ret = cmsObj_set(ipIntfObj, ipIntfIidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Set of IP.Interface object failed, ret=%d", ret);
      }
   }

   cmsObj_free((void **) &ipIntfObj);

   return;
}


void getIpv6ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack,
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

   if (cmsUtl_strlen(ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus) >= (SINT32) bufLen)
   {
      cmsLog_error("status %s cannot fit into bufLen %d",
            ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus, bufLen);
   }
   else
   {
      strcpy(serviceStatus, ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus);
   }

   cmsObj_free((void **) &ipIntfObj);

   return;
}


UBOOL8 sskConn_hasStaticIpv6AddressLocked(const InstanceIdStack *ipIntfIidStack)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   UBOOL8 hasAddr=FALSE;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   while (!hasAddr &&
          ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              ipIntfIidStack,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj)) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_STATIC) &&
          !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv6AddrObj->IPAddress))
      {
         hasAddr = TRUE;
      }
      cmsObj_free((void **) &ipv6AddrObj);
   }

   return hasAddr;
}


UBOOL8 sskConn_hasAnyIpv6AddressLocked(const InstanceIdStack *ipIntfIidStack)
{
   return (sskConn_getAnyIpv6AddressLocked(ipIntfIidStack, NULL));
}

UBOOL8 sskConn_getAnyIpv6AddressLocked(const InstanceIdStack *ipIntfIidStack,
                                       char *ipv6AddrBuf)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   UBOOL8 hasAddr=FALSE;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   while (!hasAddr &&
          ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              ipIntfIidStack,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj)) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv6AddrObj->IPAddress))
      {
         hasAddr = TRUE;
         if (ipv6AddrBuf != NULL)
         {
            // caller must provide buffer of sufficient length
            sprintf(ipv6AddrBuf, "%s/64", ipv6AddrObj->IPAddress);
         }
      }
      cmsObj_free((void **) &ipv6AddrObj);
   }

   cmsLog_debug("returning %d", hasAddr);
   return hasAddr;
}


CmsRet sskConn_getIpv6NextHopForIpIntfLocked(const char *ipIntfFullpath,
                                             char *ipv6NextHop)
{

   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6ForwardingObject *ipv6ForwardingObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: ipIntfFullPath=%s", ipIntfFullpath);

   // Scan through all forwarding objects looking for match to ipIntfFullPath
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_IPV6_FORWARDING,
                            &iidStack, OGF_NO_VALUE_UPDATE,
                            (void **) &ipv6ForwardingObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(ipv6ForwardingObj->interface, ipIntfFullpath))
      {
          // got match, copy out info
          cmsUtl_strcpy(ipv6NextHop, ipv6ForwardingObj->nextHop);
          found = TRUE;
      }
      cmsObj_free((void **) &ipv6ForwardingObj);
   }

   return (found ? CMSRET_SUCCESS : ret);
}

#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
