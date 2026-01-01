/***********************************************************************
 *
 *  Copyright (c) 2007-2021  Broadcom Corporation
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
#include "cms.h"
#include "cms_util.h"
#include "mdm_validstrings.h"
#include "cms_obj.h"
#include "cms_dal.h"
#include "dal2_wan.h"
#include "qdm_intf.h"
#include "rut2_gre.h"

CmsRet dalGre_addTunnel_dev2(const PWEB_NTWK_VAR pWebVar)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2GreTunnelObject *tunnelObj = NULL;

   cmsLog_debug("Adding new GRE Tunnel entry with %s/%s/%s/%s/%d/%d/%x ",
                pWebVar->greTunnelId, pWebVar->greTunnelRemoteEP,
                pWebVar->greTunnelProto, pWebVar->greTunnelKeepAlivePolicy,
                pWebVar->greTunnelKeepAliveTimeout, pWebVar->greTunnelKeepAliveThrsd,
                pWebVar->greTunnelDftDSCPMark);

   /* Add Tunnel object and fill in data */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_GRE_TUNNEL, &iidStack)) !=
        CMSRET_SUCCESS)
   {
      cmsLog_error("could not create MDMOID_DEV2_GRE_TUNNEL, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_GRE_TUNNEL, &iidStack, 0,
                         (void **) &tunnelObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get tunnelObj, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_GRE_TUNNEL, &iidStack);
      return ret;
   }

   tunnelObj->enable = TRUE;
   CMSMEM_REPLACE_STRING(tunnelObj->remoteEndpoints, pWebVar->greTunnelRemoteEP);
   CMSMEM_REPLACE_STRING(tunnelObj->deliveryHeaderProtocol, pWebVar->greTunnelProto);
   CMSMEM_REPLACE_STRING(tunnelObj->keepAlivePolicy, pWebVar->greTunnelKeepAlivePolicy);
   tunnelObj->keepAliveTimeout = pWebVar->greTunnelKeepAliveTimeout;
   tunnelObj->keepAliveThreshold = pWebVar->greTunnelKeepAliveThrsd;
   tunnelObj->defaultDSCPMark = pWebVar->greTunnelDftDSCPMark;

   if ((ret = cmsObj_set(tunnelObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2GreTunnelObject");
      cmsObj_free((void **) &tunnelObj);
      cmsObj_deleteInstance(MDMOID_DEV2_GRE_TUNNEL, &iidStack);
      return ret;
   }

   cmsObj_free((void **) &tunnelObj);

   dalGre_setDefaultValues(pWebVar);
   return ret;
}


CmsRet dalGre_addTunnelIf_dev2(const PWEB_NTWK_VAR pWebVar)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2GreTunnelObject *tunnelObj = NULL;
   Dev2GreTunnelInterfaceObject *tunnelIfObj = NULL;
   UBOOL8 found = FALSE;

   char *fullPathStringPtr=NULL;
   UBOOL8 isBridge=FALSE;
   UBOOL8 isLayer2=FALSE;
   char ifname[BUFLEN_32] = {0};
   UINT32 i = 1; // GRE intf name start from gre1, gre0 has been resved in kernel

   cmsLog_debug("Adding new GRE Tunnel Interface entry with %s/%s/%s/%d/%d/%s/%d/%d/%d\n",
                pWebVar->greTunnelId, pWebVar->greTunnelProto,
                pWebVar->greTunnelIfLocalGwIf,
                pWebVar->greTunnelIfProtocol,
                pWebVar->greTunnelIfUseCS,
                pWebVar->greTunnelIfKeyPolicy,
                pWebVar->greTunnelIfKeyId,
                pWebVar->greTunnelIfUseSN,
                pWebVar->greTunnelIfL2Mode);

   isBridge = pWebVar->greTunnelIfL2Mode;

   if ((ret = qdmIntf_intfnameToFullPathLocked_dev2(pWebVar->greTunnelIfLocalGwIf,
                                    isLayer2, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_intfnameToFullPathLocked_dev2 failed. ifname=%s",
                       pWebVar->greTunnelIfLocalGwIf);
      return ret;
   }

   /* auto generate a name to tunnel interface */
   do
   {
      snprintf(ifname, sizeof(ifname), "%s%d", GRE_IFC_STR, i++);
      INIT_INSTANCE_ID_STACK(&iidStack);
      found = FALSE;
      while (cmsObj_getNext(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStack,
               (void **) &tunnelIfObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ifname, tunnelIfObj->name))
         {
            cmsObj_free((void **) &tunnelIfObj);
            found = TRUE;
            break;
         }
         cmsObj_free((void **) &tunnelIfObj);
      }
   } while (found);

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNext(MDMOID_DEV2_GRE_TUNNEL, &iidStack,
                     (void **) &tunnelObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(pWebVar->greTunnelId, tunnelObj->alias))
      {
         found = TRUE;
      }
      cmsObj_free((void **) &tunnelObj);
   }

   if (found == FALSE)
   {
      cmsLog_error("GRE tunnel \"%s\" not exists", pWebVar->greTunnelId);
      return CMSRET_INVALID_ARGUMENTS;
   }

   //check entry exist
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStack, &iidStackChild,
           OGF_NO_VALUE_UPDATE, (void **)&tunnelIfObj) == CMSRET_SUCCESS)
   {
      if (((pWebVar->greTunnelIfKeyId == tunnelIfObj->keyIdentifier &&
            (cmsUtl_strcmp(pWebVar->greTunnelIfKeyPolicy, MDMVS_DISABLED) &&
             cmsUtl_strcmp(tunnelIfObj->keyIdentifierGenerationPolicy, MDMVS_DISABLED))) ||
           (!cmsUtl_strcmp(pWebVar->greTunnelIfKeyPolicy, MDMVS_DISABLED) &&
            !cmsUtl_strcmp(tunnelIfObj->keyIdentifierGenerationPolicy, MDMVS_DISABLED))) &&
          !cmsUtl_strcmp(fullPathStringPtr, tunnelIfObj->lowerLayers) &&
          pWebVar->greTunnelIfL2Mode == tunnelIfObj->X_BROADCOM_COM_L2_Mode)
      {
         cmsLog_error("tunnel interface already exists");
         cmsObj_free((void **) &tunnelObj);
         cmsObj_free((void **) &tunnelIfObj);
         return CMSRET_INVALID_ARGUMENTS;
      }
      cmsObj_free((void **) &tunnelIfObj);
   }

   /* Add interface object and fill in data for manual setting */
   memcpy(&iidStackChild, &iidStack, sizeof(InstanceIdStack));
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStackChild)) !=
        CMSRET_SUCCESS)
   {
      cmsLog_error("could not create MDMOID_DEV2_GRE_TUNNEL_INTERFACE, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStackChild, 0,
                         (void **) &tunnelIfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Dev2GreTunnelInterfaceObject, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStackChild);
      return ret;
   }

   tunnelIfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING(tunnelIfObj->name, ifname);
   tunnelIfObj->keyIdentifier = pWebVar->greTunnelIfKeyId;
   tunnelIfObj->X_BROADCOM_COM_L2_Mode = isBridge;
   CMSMEM_REPLACE_STRING(tunnelIfObj->lowerLayers, fullPathStringPtr);
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);

   tunnelIfObj->protocolIdOverride = pWebVar->greTunnelIfProtocol;
   tunnelIfObj->useChecksum = pWebVar->greTunnelIfUseCS;
   tunnelIfObj->useSequenceNumber = pWebVar->greTunnelIfUseSN;
   CMSMEM_REPLACE_STRING(tunnelIfObj->keyIdentifierGenerationPolicy, pWebVar->greTunnelIfKeyPolicy);

   if ((ret = cmsObj_set(tunnelIfObj, &iidStackChild)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2GreTunnelInterfaceObject");
      cmsObj_free((void **) &tunnelIfObj);
      cmsObj_deleteInstance(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStackChild);
      return ret;
   }
   cmsObj_free((void **) &tunnelIfObj);
   dalGre_setDefaultValues(pWebVar);

   return ret;
}

CmsRet dalGre_deleteTunnelIf_dev2(const char* name)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2GreTunnelObject *tunnelObj = NULL;
   Dev2GreTunnelInterfaceObject *tunnelIfObj = NULL;

   /* deleting a tunnel intf entry */
   cmsLog_debug("Deleting GRE Tunnel interface entry with name: %s", name);

   while (cmsObj_getNextFlags(MDMOID_DEV2_GRE_TUNNEL, &iidStack,
          OGF_NO_VALUE_UPDATE, (void **)&tunnelObj) == CMSRET_SUCCESS)
   {
      while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStack, &iidStackChild,
             OGF_NO_VALUE_UPDATE, (void **) &tunnelIfObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(name, tunnelIfObj->name))
         {
            ret = cmsObj_deleteInstance(MDMOID_DEV2_GRE_TUNNEL_INTERFACE, &iidStackChild);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to delete Dev2GreTunnelInterfaceObject, ret = %d", ret);
            }

            cmsObj_free((void **) &tunnelIfObj);
            cmsObj_free((void **) &tunnelObj);

            return ret;
         }
         cmsObj_free((void **) &tunnelIfObj);
      }

      // if tunnel interface not found, check tunnel name
      if (!cmsUtl_strcmp(name, tunnelObj->alias))
      {
         ret = cmsObj_deleteInstance(MDMOID_DEV2_GRE_TUNNEL, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete Dev2GreTunnelObject, ret = %d", ret);
         }

         cmsObj_free((void **) &tunnelObj);
         return ret;
      }
      cmsObj_free((void **) &tunnelObj);
   }

   cmsLog_error("bad tunnel/intf name %s, no entry found", name);
   return CMSRET_INVALID_ARGUMENTS;
}


CmsRet dalGre_setDefaultValues(PWEB_NTWK_VAR pWebVar)
{
   // GRE Configuration
    strcpy(pWebVar->greTunnelRemoteEP, "0.0.0.0");

    strcpy(pWebVar->greTunnelProto, "IPv4");
    strcpy(pWebVar->greTunnelKeepAlivePolicy, "None");
    pWebVar->greTunnelKeepAliveTimeout = 10;
    pWebVar->greTunnelKeepAliveThrsd = 3;
    pWebVar->greTunnelDftDSCPMark = 0;

    strcpy(pWebVar->greTunnelIfLocalGwIf, "");
    pWebVar->greTunnelIfProtocol = 0;
    pWebVar->greTunnelIfUseCS = 0;
    strcpy(pWebVar->greTunnelIfKeyPolicy, MDMVS_DISABLED);
    pWebVar->greTunnelIfKeyId = 0;
    pWebVar->greTunnelIfUseSN = 0;

    return CMSRET_SUCCESS;
}
#endif /* SUPPORT_GRE_TUNNEL_TR181 */
