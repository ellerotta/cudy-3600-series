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
#ifdef SUPPORT_GRE_TUNNEL_TR181

#include "cms_util.h"
#include "rcl.h"
#include "rut2_util.h"
#include "rut2_gre.h"
#include "rut_util.h"
#include "qdm_intf.h"
#include "qdm_ipintf.h"

CmsRet rcl_dev2GreObject(_Dev2GreObject *newObj,
                const _Dev2GreObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2GreTunnelObject(_Dev2GreTunnelObject *newObj,
                const _Dev2GreTunnelObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("Enter");

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumGreTunnel_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumGreTunnel_dev2(iidStack, -1);
   }

   return ret;
}

CmsRet rcl_dev2GreTunnelStatsObject(_Dev2GreTunnelStatsObject *newObj,
                const _Dev2GreTunnelStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2GreTunnelInterfaceObject(_Dev2GreTunnelInterfaceObject *newObj,
                const _Dev2GreTunnelInterfaceObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter");
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumGreTunnelIntf_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumGreTunnelIntf_dev2(iidStack, -1);
      rutGre_delete_tunnel_intf_dev2(currObj->name);
      return ret;
   }

   if (newObj != NULL)
   {
      if (newObj->enable)
      {
         UBOOL8 isIpv6;
         UBOOL8 wanServiceUp;
         char statusBuf[BUFLEN_64] = {0};
         UBOOL8 key;
         Dev2GreTunnelObject *tunnelObj = NULL;
         InstanceIdStack tunnelIidStack = *iidStack;
         char wanIfname[CMS_IFNAME_LENGTH] = {0};

         if (IS_EMPTY_STRING(newObj->name) || IS_EMPTY_STRING(newObj->lowerLayers))
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DORMANT,
                                  mdmLibCtx.allocFlags);
            cmsLog_error("Cannot enable GRE interface without name and lowerLayers.");
            return CMSRET_INVALID_ARGUMENTS;
         }

         ret = cmsObj_getAncestorFlags(MDMOID_DEV2_GRE_TUNNEL,
                          MDMOID_DEV2_GRE_TUNNEL_INTERFACE,
                          &tunnelIidStack,
                          OGF_NO_VALUE_UPDATE, (void **)&tunnelObj);

         if (ret != CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ERROR,
                                        mdmLibCtx.allocFlags);
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
               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ERROR,
                                        mdmLibCtx.allocFlags);
               cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. ret=%d lowerLayers=%s",
                                     ret, newObj->lowerLayers);
               cmsObj_free((void **) &tunnelObj);
               return ret;
            }

            key = cmsUtl_strcmp(newObj->keyIdentifierGenerationPolicy, MDMVS_DISABLED);
            ret = rutGre_create_tunnel_intf_dev2(newObj->name,
                                                isIpv6,
                                                newObj->X_BROADCOM_COM_L2_Mode,
                                                newObj->useChecksum,
                                                newObj->useSequenceNumber,
                                                key,
                                                newObj->keyIdentifier,
                                                tunnelObj->remoteEndpoints,
                                                wanIfname);

            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("rutGre_create_tunnel_intf_dev2 failed, ret=%d", ret);
               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ERROR,
                                        mdmLibCtx.allocFlags);
               cmsObj_free((void **) &tunnelObj);
               return ret;
            }

            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
            cmsObj_free((void **) &tunnelObj);
         }
         else
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
            rutGre_delete_tunnel_intf_dev2(newObj->name);
         }
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         rutGre_delete_tunnel_intf_dev2(newObj->name);
      }
   }

   return ret;
}

CmsRet rcl_dev2GreTunnelInterfaceStatsObject(_Dev2GreTunnelInterfaceStatsObject *newObj,
                const _Dev2GreTunnelInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif /* SUPPORT_GRE_TUNNEL_TR181 */

