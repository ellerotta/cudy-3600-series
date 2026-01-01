/***********************************************************************
 *
 *  Copyright (c) 2021  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2021:proprietary:standard

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
#ifdef SUPPORT_VXLAN_TUNNEL_TR181

#include "cms_util.h"
#include "rcl.h"
#include "rut2_util.h"
#include "rut2_vxlan.h"
#include "rut_util.h"
#include "qdm_intf.h"
#include "qdm_ipintf.h"

CmsRet rcl_dev2VxlanObject(_Dev2VxlanObject *newObj,
                const _Dev2VxlanObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2VxlanTunnelObject(_Dev2VxlanTunnelObject *newObj,
                const _Dev2VxlanTunnelObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("Enter");

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumVxlanTunnel_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumVxlanTunnel_dev2(iidStack, -1);
   }

   return ret;
}

CmsRet rcl_dev2VxlanTunnelStatsObject(_Dev2VxlanTunnelStatsObject *newObj,
                const _Dev2VxlanTunnelStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2VxlanTunnelInterfaceObject(_Dev2VxlanTunnelInterfaceObject *newObj,
                const _Dev2VxlanTunnelInterfaceObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2VxlanTunnelObject *tunnelObj = NULL;
   InstanceIdStack tunnelIidStack = *iidStack;
   char wanIfname[CMS_IFNAME_LENGTH]={0};
   UINT32 num;
   UBOOL8 isIpv6;

   cmsLog_debug("Enter");
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumVxlanTunnelIntf_dev2(iidStack, 1);
      rutVxlan_updateTunnelIfNum_dev2(1);

      if ((rutVxlan_getTunnelIfNum_dev2(&num) != CMSRET_SUCCESS) || (num > MAX_VXLAN_TUNNELS_INTF))
      {
         cmsLog_error("Could not create vxlan tunnel interface, exceed to the limit:%d", MAX_VXLAN_TUNNELS_INTF);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumVxlanTunnelIntf_dev2(iidStack, -1);
      rutVxlan_updateTunnelIfNum_dev2(-1);
   }

   if (ADD_NEW(newObj, currObj))
   {
      if (!newObj->X_BROADCOM_COM_L2_Mode)
      {
         rutVxlan_active_ipintf_dev2(newObj->name, FALSE);
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (cmsUtl_strcmp(currObj->status, MDMVS_LOWERLAYERDOWN) &&
          cmsUtl_strcmp(currObj->status, MDMVS_DOWN))
      {
         ret = qdmIntf_fullPathToIntfnameLocked_dev2(currObj->lowerLayers, wanIfname);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. ret=%d lowerLayers=%s",
                                     ret, currObj->lowerLayers);
            return ret;
         }

         ret = cmsObj_getAncestorFlags(MDMOID_DEV2_VXLAN_TUNNEL,
                             MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE,
                             &tunnelIidStack,
                             OGF_NO_VALUE_UPDATE, (void **)&tunnelObj);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get parent tunnel interface, ret=%d", ret);
            return ret;
         }

         isIpv6 = !cmsUtl_strcmp(tunnelObj->deliveryHeaderProtocol, MDMVS_IPV6);

         if ((ret = rutVxlan_delete_tunnel_intf_dev2(currObj->name, isIpv6, \
                                     tunnelObj->remotePort, wanIfname)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutVxlan_delete_tunnel_intf_dev2 failed, ret=%d", ret);
         }
         cmsObj_free((void **) &tunnelObj);
      }
   }
   else
   {
      UBOOL8 wanServiceUp;
      char statusBuf[BUFLEN_64]={0};

      if (IS_EMPTY_STRING(newObj->name) || IS_EMPTY_STRING(newObj->lowerLayers))
      {
         cmsLog_error("Cannot enable vxlan interface without name and lowerLayers.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP,
                                  mdmLibCtx.allocFlags);
      ret = cmsObj_getAncestorFlags(MDMOID_DEV2_VXLAN_TUNNEL,
                          MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE,
                          &tunnelIidStack,
                          OGF_NO_VALUE_UPDATE, (void **)&tunnelObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get parent tunnel interface, ret=%d", ret);
         return ret;
      }

      isIpv6 = !cmsUtl_strcmp(tunnelObj->deliveryHeaderProtocol, MDMVS_IPV6);
      qdmIpIntf_getIpvxServiceStatusFromFullPathLocked_dev2(newObj->lowerLayers,
                                           isIpv6? CMS_AF_SELECT_IPV6 : CMS_AF_SELECT_IPV4,
                                           statusBuf, sizeof(statusBuf));

      wanServiceUp = !cmsUtl_strcmp(statusBuf, MDMVS_SERVICEUP);

      if (wanServiceUp)
      {
         ret = qdmIntf_fullPathToIntfnameLocked_dev2(newObj->lowerLayers, wanIfname);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. ret=%d lowerLayers=%s",
                                     ret, newObj->lowerLayers);
            cmsObj_free((void **) &tunnelObj);
            return ret;
         }

         ret = rutVxlan_create_tunnel_intf_dev2(newObj->name,
                                                isIpv6,
                                                newObj->VNI,
                                                tunnelObj->remotePort,
                                                tunnelObj->sourcePort,
                                                tunnelObj->remoteEndpoints,
                                                wanIfname,
                                                tunnelObj->keepAliveTimeout,
                                                tunnelObj->keepAliveThreshold,
                                                newObj->X_BROADCOM_COM_ZeroCsumTx,
                                                newObj->X_BROADCOM_COM_ZeroCsumRx);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("rutVxlan_create_tunnel_intf_dev2 failed, ret=%d", ret);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ERROR,
                                        mdmLibCtx.allocFlags);
            cmsObj_free((void **) &tunnelObj);
            return ret;
         }

         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
      }
      cmsObj_free((void **) &tunnelObj);
   }

   return ret;
}

CmsRet rcl_dev2VxlanTunnelInterfaceStatsObject(_Dev2VxlanTunnelInterfaceStatsObject *newObj,
                const _Dev2VxlanTunnelInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif /* SUPPORT_VXLAN_TUNNEL_TR181 */
