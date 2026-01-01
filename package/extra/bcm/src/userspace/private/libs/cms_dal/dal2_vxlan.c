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
#ifdef SUPPORT_VXLAN_TUNNEL_TR181
#include "cms.h"
#include "cms_util.h"
#include "mdm_validstrings.h"
#include "cms_obj.h"
#include "cms_dal.h"
#include "dal2_wan.h"
#include "qdm_intf.h"
#include "rut2_vxlan.h"

CmsRet dalVxlan_addTunnel_dev2(const PWEB_NTWK_VAR pWebVar)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanTunnelObject *tunnelObj = NULL;

   cmsLog_debug("Adding new Vxlan Tunnel entry with %s/%s/%d/%d/%s/%s/%d/%d/%x ",
                pWebVar->vxlanTunnelId, pWebVar->vxlanTunnelRemoteEP,
                pWebVar->vxlanTunnelSourcePort, pWebVar->vxlanTunnelRemotePort,
                pWebVar->vxlanTunnelProto, pWebVar->vxlanTunnelKeepAlivePolicy,
                pWebVar->vxlanTunnelKeepAliveTimeout, pWebVar->vxlanTunnelKeepAliveThrsd,
                pWebVar->vxlanTunnelDftDSCPMark);

   /* Add Tunnel object and fill in data */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack)) !=
        CMSRET_SUCCESS)
   {
      cmsLog_error("could not create MDMOID_DEV2_VXLAN_TUNNEL, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack, 0,
                         (void **) &tunnelObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get tunnelObj, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack);
      return ret;
   }

   tunnelObj->enable = TRUE;
   CMSMEM_REPLACE_STRING(tunnelObj->remoteEndpoints, pWebVar->vxlanTunnelRemoteEP);
   tunnelObj->sourcePort = pWebVar->vxlanTunnelSourcePort;
   tunnelObj->remotePort = pWebVar->vxlanTunnelRemotePort;
   CMSMEM_REPLACE_STRING(tunnelObj->deliveryHeaderProtocol, pWebVar->vxlanTunnelProto);
   CMSMEM_REPLACE_STRING(tunnelObj->keepAlivePolicy, pWebVar->vxlanTunnelKeepAlivePolicy);
   tunnelObj->keepAliveTimeout = pWebVar->vxlanTunnelKeepAliveTimeout;
   tunnelObj->keepAliveThreshold = pWebVar->vxlanTunnelKeepAliveThrsd;
   tunnelObj->defaultDSCPMark = pWebVar->vxlanTunnelDftDSCPMark;

   if ((ret = cmsObj_set(tunnelObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2VxlanTunnelObject");
      cmsObj_free((void **) &tunnelObj);
      cmsObj_deleteInstance(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack);
      return ret;
   }

   cmsObj_free((void **) &tunnelObj);

   dalVxlan_setDefaultValues(pWebVar);
   return ret;
}


CmsRet dalVxlan_addTunnelIf_dev2(const PWEB_NTWK_VAR pWebVar)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanTunnelObject *tunnelObj = NULL;
   Dev2VxlanTunnelInterfaceObject *tunnelIfObj = NULL;
   UBOOL8 found = FALSE;

   char *fullPathStringPtr=NULL;
   UBOOL8 supportIpv4 = TRUE;
   UBOOL8 supportIpv6 = FALSE;
   UBOOL8 enableIgmp = FALSE;
   UBOOL8 enableIgmpSource = FALSE;
   UBOOL8 enableMld = FALSE;
   UBOOL8 enableMldSource = FALSE;
   UBOOL8 isBridge=FALSE;
   UBOOL8 enblFirewall=FALSE;
   UBOOL8 isLayer2=FALSE;
   MdmPathDescriptor vxlanTunnelIfPathDesc;
   char *vxlanTunnelIfFullPath = NULL;
   char ipIntfFullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN] = {0};
   MdmPathDescriptor ipIntfPathDesc;
   UBOOL8 isIpv6;
   char ifname[BUFLEN_8] = {0};
   UINT32 i;

   cmsLog_debug("Adding new Vxlan Tunnel Interface entry with %s/%s/%s/%d/%s/%s/%d/%d/%d",
                pWebVar->vxlanTunnelId, pWebVar->vxlanTunnelProto,
                pWebVar->vxlanTunnelIfLocalGwIf, pWebVar->vxlanTunnelIfVNI,
                pWebVar->vxlanTunnelIfIpAddress,
                pWebVar->vxlanTunnelIfSubnet,
                pWebVar->vxlanTunnelIsBridge,
                pWebVar->vxlanTunnelIfZeroCsumTx,
                pWebVar->vxlanTunnelIfZeroCsumRx);

   isIpv6 = !cmsUtl_strcmp(pWebVar->vxlanTunnelProto, "IPv6");
   isBridge = pWebVar->vxlanTunnelIsBridge;
#ifdef DMP_DEVICE2_IPV6INTERFACE_1
   if (isIpv6)
   {
      supportIpv6 = TRUE;
   }
   else
#endif
   {
      supportIpv6 = FALSE;
   }

   if ((ret = qdmIntf_intfnameToFullPathLocked_dev2(pWebVar->vxlanTunnelIfLocalGwIf,
                                    isLayer2, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_intfnameToFullPathLocked_dev2 failed. ifname=%s",
                       pWebVar->vxlanTunnelIfLocalGwIf);
      return ret;
   }

   //check VNI
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNext(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStack,
               (void **) &tunnelIfObj) == CMSRET_SUCCESS)
   {
      if (pWebVar->vxlanTunnelIfVNI == tunnelIfObj->VNI)
      {
         cmsLog_error("tunnel interface VNI \"%d\" already exists", pWebVar->vxlanTunnelIfVNI);
         cmsObj_free((void **) &tunnelIfObj);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);
         return CMSRET_INVALID_ARGUMENTS;
      }
      cmsObj_free((void **) &tunnelIfObj);
   }

   /* auto generate a name to tunnel interface */
   for (i = 0; i < MAX_VXLAN_TUNNELS_INTF; i++)
   {
      snprintf(ifname, sizeof(ifname), "%s%d", VXLAN_IFC_STR, i);
      INIT_INSTANCE_ID_STACK(&iidStack);
      found = FALSE;
      while (cmsObj_getNext(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStack,
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
      if (!found)
           break;
   }

   if (i == MAX_VXLAN_TUNNELS_INTF)
   {
      cmsLog_error("could not create vxlan tunnel interface, exceed to the limit:%d", MAX_VXLAN_TUNNELS_INTF);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);
      return ret;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNext(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack,
                     (void **) &tunnelObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(pWebVar->vxlanTunnelId, tunnelObj->alias))
      {
         found = TRUE;
      }
      cmsObj_free((void **) &tunnelObj);
   }

   if (found == FALSE)
   {
      cmsLog_error("Vxlan tunnel \"%s\" not exists", pWebVar->vxlanTunnelId);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* Add interface object and fill in data for manual setting */
   memcpy(&iidStackChild, &iidStack, sizeof(InstanceIdStack));
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStackChild)) !=
        CMSRET_SUCCESS)
   {
      cmsLog_error("could not create MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStackChild, 0,
                         (void **) &tunnelIfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Dev2VxlanTunnelInterfaceObject, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStackChild);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);
      return ret;
   }

   tunnelIfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING(tunnelIfObj->name, ifname);
   tunnelIfObj->VNI = pWebVar->vxlanTunnelIfVNI;
   tunnelIfObj->X_BROADCOM_COM_L2_Mode = isBridge;
   tunnelIfObj->X_BROADCOM_COM_ZeroCsumTx = pWebVar->vxlanTunnelIfZeroCsumTx;
   tunnelIfObj->X_BROADCOM_COM_ZeroCsumRx = pWebVar->vxlanTunnelIfZeroCsumRx;
   CMSMEM_REPLACE_STRING(tunnelIfObj->lowerLayers, fullPathStringPtr);
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);

   if ((ret = cmsObj_set(tunnelIfObj, &iidStackChild)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2VxlanTunnelInterfaceObject");
      cmsObj_free((void **) &tunnelIfObj);
      cmsObj_deleteInstance(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStackChild);
      return ret;
   }
   cmsObj_free((void **) &tunnelIfObj);

   if (!isBridge)
   {
      INIT_PATH_DESCRIPTOR(&vxlanTunnelIfPathDesc);
      vxlanTunnelIfPathDesc.iidStack = iidStackChild;
      vxlanTunnelIfPathDesc.oid = MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE;
      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&vxlanTunnelIfPathDesc,
                             &vxlanTunnelIfFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      }

      if ((ret = dalIp_addIntfObject_dev2(supportIpv4,
                                       supportIpv6,
                                       NULL,  /* intfGroupName */
                                       isBridge,
                                       (isBridge ? "br0" : NULL),
                                       enblFirewall,
                                       enableIgmp, enableIgmpSource,
                                       enableMld, enableMldSource,
                                       vxlanTunnelIfFullPath,
                                       ipIntfFullPathBuf,
                                       sizeof(ipIntfFullPathBuf),
                                       &ipIntfPathDesc)) != CMSRET_SUCCESS)
      {
         cmsLog_error("dalIp_addIntfObject_dev2 failed. ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(vxlanTunnelIfFullPath);
         return ret;
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(vxlanTunnelIfFullPath);

      if (!isIpv6)
      {
         if ((ret = dalIp_addIpIntfIpv4Address_dev2(&(ipIntfPathDesc.iidStack),
                          pWebVar->vxlanTunnelIfIpAddress,
                          pWebVar->vxlanTunnelIfSubnet)) != CMSRET_SUCCESS)
         {
            cmsLog_error("dalIp_addIpIntfIpv4Address_dev2 failed. ret=%d", ret);
         }
      }
      else
      {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
         char prefixPathRef[MDM_SINGLE_FULLPATH_BUFLEN]={0};
         strcpy(pWebVar->wanAddr6, pWebVar->vxlanTunnelIfIpAddress);
         strcpy(pWebVar->wanAddr6Type, MDMVS_STATIC);

         /*  Add and set the ipv6 prefix object */
         if ((ret = dalIp_addIpIntfIPv6Prefix_dev2(&(ipIntfPathDesc.iidStack),
                                                pWebVar,
                                                prefixPathRef,
                                                sizeof(prefixPathRef))) != CMSRET_SUCCESS)
         {
            cmsLog_error("dalIp_addIpIntfIPv6Prefix_dev2 failed. ret=%d", ret);
         }
         /* Add and set the ipv6 address object */
         else if ((ret = dalIp_addIpIntfIpv6Address_dev2(&(ipIntfPathDesc.iidStack),
                                                pWebVar,
                                                prefixPathRef)) != CMSRET_SUCCESS)
         {
            cmsLog_error("dalIp_addIpIntfIpv6Address_dev2 failed. ret=%d", ret);
         }
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
      }
   }
   dalVxlan_setDefaultValues(pWebVar);

   return ret;
}

CmsRet dalVxlan_deleteTunnelIf_dev2(const char* name)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2VxlanTunnelObject *tunnelObj = NULL;
   Dev2VxlanTunnelInterfaceObject *tunnelIfObj = NULL;
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   UBOOL8 found = FALSE;

   /* deleting a tunnel intf entry */
   cmsLog_debug("Deleting Vxlan Tunnel interface entry with name: %s", name);

   while (cmsObj_getNextFlags(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack,
          OGF_NO_VALUE_UPDATE, (void **)&tunnelObj) == CMSRET_SUCCESS)
   {
      while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStack, &iidStackChild,
             OGF_NO_VALUE_UPDATE, (void **) &tunnelIfObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(name, tunnelIfObj->name))
         {
            if (!tunnelIfObj->X_BROADCOM_COM_L2_Mode)
            {
               while (!found && cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &ipIntfIidStack,
                   OGF_NO_VALUE_UPDATE, (void **)&ipIntfObj) == CMSRET_SUCCESS)
               {
                  if (!cmsUtl_strcmp(name, ipIntfObj->name))
                  {
                     ret = cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &ipIntfIidStack);
                     if (ret != CMSRET_SUCCESS)
                     {
                        cmsLog_error("Failed to delete Dev2IpInterfaceObject, ret = %d", ret);
                     }
                     found = TRUE;
                  }
               }
               cmsObj_free((void **) &ipIntfObj);

               if (!found)
			   {
                  cmsLog_debug("not found ip obj for VxLan tunnel interface:%s", name);
               }
            }

            ret = cmsObj_deleteInstance(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE, &iidStackChild);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to delete Dev2VxlanTunnelInterfaceObject, ret = %d", ret);
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
         ret = cmsObj_deleteInstance(MDMOID_DEV2_VXLAN_TUNNEL, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete Dev2VxlanTunnelObject, ret = %d", ret);
         }

         cmsObj_free((void **) &tunnelObj);
         return ret;
      }
      cmsObj_free((void **) &tunnelObj);
   }

   cmsLog_error("bad tunnel/intf name %s, no entry found", name);
   return CMSRET_INVALID_ARGUMENTS;
}


CmsRet dalVxlan_setDefaultValues(PWEB_NTWK_VAR pWebVar)
{
   // Vxlan Configuration
    strcpy(pWebVar->vxlanTunnelRemoteEP, "0.0.0.0");
    pWebVar->vxlanTunnelSourcePort = 0;
    pWebVar->vxlanTunnelRemotePort = 4789;

    strcpy(pWebVar->vxlanTunnelProto, "IPv4");
    strcpy(pWebVar->vxlanTunnelKeepAlivePolicy, "None");
    pWebVar->vxlanTunnelKeepAliveTimeout = 10;
    pWebVar->vxlanTunnelKeepAliveThrsd = 3;
    pWebVar->vxlanTunnelDftDSCPMark = 0;

    strcpy(pWebVar->vxlanTunnelIfLocalGwIf, "");
    pWebVar->vxlanTunnelIfVNI = 1;
    pWebVar->vxlanTunnelIfZeroCsumTx = 1;
    pWebVar->vxlanTunnelIfZeroCsumRx = 1;

    return CMSRET_SUCCESS;
}
#endif /* SUPPORT_VXLAN_TUNNEL_TR181 */
