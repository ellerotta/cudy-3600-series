/***********************************************************************
 *
 *  Copyright (c) 2006-2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#include "stl.h"
#include "cms_util.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "qdm_ipintf.h"

#ifdef SUPPORT_TR69C
/* because this object is a baseline object, it is mostly included.  And if TR69C is disabled, the functions 
 * should be empty.
 */
 
 
// in rut2_tr69c.c
CmsRet rutTr69c_getDefaultLanIntfAddr(char *ipAddress);
CmsRet rutTr69c_getRoutedAndConnectedWanIntfAddr(const char *ifName,
                                                 char *ipAddress);


CmsRet stl_dev2ManagementServerObject(_Dev2ManagementServerObject *obj,
      const InstanceIdStack *iidStack __attribute((unused)))
{
   char url[BUFLEN_1024] = {0};
   char ipAddress[CMS_IPADDR_LENGTH]={0};
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);
   UBOOL8 updateConnReqURL = FALSE;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

#ifdef OMCI_TR69_DUAL_STACK
   // TODO: OMCI TR69 Dual stack is not supported in BDK yet.
   // The second tr69c is for configuring voice and should probably run in the
   // voice component.
   UBOOL8 isIPv4 = TRUE;
   char *pIpAddress = NULL;
   if ((ret = rutOmci_getIpHostAddress(obj->X_BROADCOM_COM_BoundIfName, &pIpAddress, &isIPv4)) == CMSRET_SUCCESS)
   {
         if (isIPv4)
         {
            snprintf(url, sizeof(url), "http://%s:%d%s", pIpAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
         }
         else
         {
            snprintf(url, sizeof(url), "http://[%s]:%d%s", pIpAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
         }
         CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(pIpAddress);
   }
   else
#endif

   cmsLog_notice("Entered: boundIfName=%s myEid=%d",
                 obj->X_BROADCOM_COM_BoundIfName, myEid);
   if (myEid == EID_TR69C)
   {
      // This function will only update connReqURL when tr69c is reading it.
      // All other apps reading this object will just get the current connReqURL.
      updateConnReqURL = TRUE;
   }

   if (cmsUtl_strcmp(obj->X_BROADCOM_COM_BoundIfName, MDMVS_LOOPBACK) == 0)
   {
      // This is probably for DESKTOP_LINUX testing.  Loopback is always up,
      // so we can form the connection request URL now.
      snprintf(url, sizeof(url), "http://127.0.0.1:%d%s", TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
   }
   else if (cmsUtl_strcmp(obj->X_BROADCOM_COM_BoundIfName, MDMVS_LAN) == 0)
   {
      /* LAN side is for testing only, use IPv4 only to make life simple */
      if (updateConnReqURL)
      {
         cmsLog_debug("real call from tr69c, LAN side");
         if (rutTr69c_getDefaultLanIntfAddr(ipAddress) == CMSRET_SUCCESS)
         {
            if (cmsUtl_isValidIpv4Address(ipAddress))
            {
                snprintf(url, sizeof(url), "http://%s:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
            else
            {
                cmsUtl_truncatePrefixFromIpv6AddrStr(ipAddress);
                snprintf(url, sizeof(url), "http://[%s]:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
         }
      }
   }
   else  // ANY_WAN or specific BoundIfName
   {
      if (updateConnReqURL)
      {
         CmsRet r2;
         cmsLog_debug("real call from tr69c, boundIfName=%s", obj->X_BROADCOM_COM_BoundIfName);
         r2 = rutTr69c_getRoutedAndConnectedWanIntfAddr(obj->X_BROADCOM_COM_BoundIfName,
                                                         ipAddress);
         if (r2 == CMSRET_SUCCESS)
         {
            if (cmsUtl_isValidIpv4Address(ipAddress))
            {
               snprintf(url, sizeof(url), "http://%s:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
            else
            {
               cmsUtl_truncatePrefixFromIpv6AddrStr(ipAddress);
               snprintf(url, sizeof(url), "http://[%s]:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
         }
      }
   }

   if (updateConnReqURL)
   {
      if (IS_EMPTY_STRING(url))
      {
         // nothing in url, which means the ACS intf is not up.
         // clear out the current connReqURL if there is one, so
         // clearModemConnectionURL_dev2 can be empty.
         if (obj->connectionRequestURL != NULL)
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(obj->connectionRequestURL);
            ret = CMSRET_SUCCESS;
         }
         cmsLog_debug("ACS intf is not up, connReqURL is null");
      }
      else
      {
         // url is set, which means ACS intf is up.  See if connReqURL has changed.
         if (cmsUtl_strcmp(obj->connectionRequestURL, url))
         {
            CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
            ret = CMSRET_SUCCESS;
            cmsLog_notice("new connreqURL=%s", obj->connectionRequestURL);
         }
         else
         {
            cmsLog_debug("no change to existng connReqURL=%s", obj->connectionRequestURL);
         }
      }
   }

   /*
    * parameterKey should be updated by tr69c during protocol processing.
    * Handler function does not need to generate it or do anything with it.
    */
   return ret;
}


#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
CmsRet stl_dev2ManagementServerManageableDeviceObject(_Dev2ManagementServerManageableDeviceObject *obj __attribute__((unused)),
   const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE 
CmsRet stl_dev2AutonXferCompletePolicyObject(_Dev2AutonXferCompletePolicyObject *obj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */

#ifdef DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1
CmsRet stl_dev2DuStateChangeComplPolicyObject(_Dev2DuStateChangeComplPolicyObject *obj __attribute__((unused)), 
                                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1 */

#else /* SUPPORT_TR69C */

#ifndef SUPPORT_RETAIL_DM
/* retail data model has even the management object stripped from baseline profile */
CmsRet stl_dev2ManagementServerObject(_Dev2ManagementServerObject *obj __attribute((unused)),
      const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
CmsRet stl_dev2ManagementServerManageableDeviceObject(_Dev2ManagementServerManageableDeviceObject *obj __attribute__((unused)),
   const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE 
CmsRet stl_dev2AutonXferCompletePolicyObject(_Dev2AutonXferCompletePolicyObject *obj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */

#ifdef DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1
CmsRet stl_dev2DuStateChangeComplPolicyObject(_Dev2DuStateChangeComplPolicyObject *obj __attribute__((unused)), 
                                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1 */


#endif /* SUPPORT_TR69C */


#endif  /* DMP_DEVICE2_BASELINE_1 */

