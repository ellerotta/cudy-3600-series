/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#ifdef DMP_DEVICE2_BRIDGE_1


/*!\file rut2_bridging.c
 * \brief This file contains helper functions for the Device.Bridging. objects.
 * We may want to move the VLAN related functions out to another file if
 * we want more separation of the code.
 *
 */


#include "odl.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut2_bridging.h"
#ifdef SUPPORT_VLANCTL
#ifdef SUPPORT_VLANSUBIF
#include "vlan_subif.h"
#else
#error This file requires SUPPORT_VLANSUBIF
#endif
#endif
#include "rut_lan.h"

#include "cms_msg_pubsub.h"

static CmsRet rutBridge_startL3Interface(SINT32 vlanMuxId __attribute__((unused)),
                                         SINT32 vlanMuxPr __attribute((unused)),
                                         UINT32 vlanTpid __attribute((unused)),
                                         char * l2IfName,
                                         char * baseL3IfName);

#ifdef DMP_DEVICE2_VLANBRIDGE_1
static CmsRet rutBridge_addVlanPort_dev2(SINT32 vlanId,
                                         const InstanceIdStack *brIidStack,
                                         const MdmPathDescriptor *brPortPathDesc);
#endif

CmsRet rutBridge_getParentBridgeIntfName_dev2(const InstanceIdStack *iidStack,
                                              char *bridgeIntfName)
{
   InstanceIdStack brIidStack;
   Dev2BridgeObject *brObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entered: iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* The parent Bridge object is 1 level above the Port object. */
   brIidStack = *iidStack;
   POP_INSTANCE_ID(&brIidStack);

   ret = cmsObj_get(MDMOID_DEV2_BRIDGE, &brIidStack, 0, (void **)&brObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get parent Bridge obj");
      return ret;
   }

   if (IS_EMPTY_STRING(brObj->X_BROADCOM_COM_IfName))
   {
      cmsLog_error("Parent bridge object is still empty");
   }
   else
   {
      strcpy(bridgeIntfName, brObj->X_BROADCOM_COM_IfName);
      cmsLog_debug("Found parent bridge name=%s", bridgeIntfName);
   }

   cmsObj_free((void **)&brObj);

   return ret;
}

CmsRet rutBridge_getParentBridgeType_dev2(const InstanceIdStack *iidStack,
                                          UBOOL8 *isOVS)
{
   InstanceIdStack brIidStack;
   Dev2BridgeObject *brObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entered: iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* The parent Bridge object is 1 level above the Port object. */
   brIidStack = *iidStack;
   POP_INSTANCE_ID(&brIidStack);

   ret = cmsObj_get(MDMOID_DEV2_BRIDGE, &brIidStack, 0, (void **)&brObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get parent Bridge obj");
      return ret;
   }

   if (cmsUtl_strcmp(brObj->X_BROADCOM_COM_Type, MDMVS_OVS) == 0)
   {
      *isOVS = TRUE;
   }
   else
   {
      *isOVS = FALSE;
   }

   cmsObj_free((void **)&brObj);
   return ret;
}


CmsRet rutBridge_getBridgeTypeByName_dev2(const char *brIntfName, UBOOL8 *isOVS)
{
   CmsRet ret;

   cmsLog_debug("Entered: bridgeName=%s", brIntfName);
   *isOVS = FALSE;

   if (cmsMdm_isCmsClassic() ||
       cmsMdm_isBdkSysmgmt())
   {
      InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2BridgeObject *brObj=NULL;
      UBOOL8 found=FALSE;

      /* First find the bridge */
      if ((ret = cmsLck_acquireAllZoneLocksWithBackoff(0, 200)) != CMSRET_SUCCESS )
      {
         cmsLog_error("could not get lock, ret=%d", ret);
         cmsLck_dumpInfo();
         return ret;
      }

      while (!found &&
            (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brObj)) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
         {
            found = TRUE;
            if (cmsUtl_strcmp(brObj->X_BROADCOM_COM_Type, MDMVS_OVS) == 0)
            {
               *isOVS = TRUE;
            }
            else
            {
               *isOVS = FALSE;
            }
         }
         /* we don't need the bridge object, so always free it */
         cmsObj_free((void **) &brObj);
      }

      cmsLck_releaseAllZoneLocks();
   }
   else
   {
      char msgBuf[sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody) + 128] = {0};  // max possible buf
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
      char *queryString = (char *)(msgHdr + 1);
      CmsMsgHeader *msgResp = NULL;

      msgHdr->src = (UINT32) cmsMsg_getHandleEid(msgHandle);
      msgHdr->dst = EID_SSK;
      msgHdr->type = CMS_MSG_QUERY_EVENT_STATUS;
      msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
      msgHdr->flags_request = 1;

      snprintf(queryString, BUFLEN_128, "bridgeType=%s", MDMVS_OVS);
      msgHdr->dataLength = strlen(queryString) + 1;

      cmsLog_debug("Sending msgType 0x%x dataLength %d queryString=%s",
                   msgHdr->type, msgHdr->dataLength, queryString);
   
      // in BDK mode, it would redirect to component MD
      ret = cmsMsg_sendAndGetReplyBufWithTimeout(msgHandle, msgHdr, &msgResp, 9000);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("get reply message failed. (ret=%d)\n", ret);
         return ret; 
      }

      if (msgResp)
      {
         if (msgResp->wordData == PUBSUB_EVENT_INTERFACE)
         {
            char *respString;
            respString = (char *)(msgResp + 1);
            cmsLog_debug("QUERY_EVENT_STATUS response, dataLenght=%d, respString=%s", 
                         msgResp->dataLength, respString);

            if (strstr(respString, brIntfName) != NULL)
            {
               *isOVS = TRUE;
            }
            else
            {
               *isOVS = FALSE;
            }
         }
         else
         {
            /* we might get error, for example: the record of an interface not found in sysdir */
            cmsLog_error("query interface at %s event status failed (ret=%d)\n", queryString, msgResp->wordData);
            ret = CMSRET_INTERNAL_ERROR;
         }
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);
   }
   return ret;
}

CmsRet rutBridge_getBridgeIndexByName_dev2(const char *brIntfName, SINT32 *index)
{
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeObject *brObj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: bridgeName=%s", brIntfName);

   /* First find the bridge */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
      {
         found = TRUE;
         *index = brObj->X_BROADCOM_COM_Index;
      }
      /* we don't need the bridge object, so always free it */
      cmsObj_free((void **) &brObj);
   }

   return ret;
}

CmsRet rutBridge_getBridgeNameByIndex_dev2(SINT32 index, char *brIntfName, UINT32 size)
{
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeObject *brObj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: bridgeIndex=%d", index);

   /* First find the bridge */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj)) == CMSRET_SUCCESS)
   {
      if (brObj->X_BROADCOM_COM_Index == index)
      {
         found = TRUE;
         strncpy(brIntfName, brObj->X_BROADCOM_COM_IfName, size-1);
      }
      /* we don't need the bridge object, so always free it */
      cmsObj_free((void **) &brObj);
   }

   return ret;
}

CmsRet rutBridge_addIntfNameToBridge_dev2(const char *intfName,
                                          const char *brIntfName)
{
   char *fullPath=NULL;
   CmsRet ret;
   UBOOL8 ethVlanFound=FALSE;

   /*
    * This function might be called from the intf grouping code, so the
    * intfName could be a WAN intfName with a VLANTermination object on top.
    * (Currently, all WAN intf -- including bridge -- has a VLANTermination
    * obj on top).  Check for that first.
    */
   {
      MdmPathDescriptor ethVlanPathDesc = EMPTY_PATH_DESCRIPTOR;
      InstanceIdStack ethVlanIidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2VlanTerminationObject *ethVlanObj = NULL;

      while (!ethVlanFound &&
             cmsObj_getNextFlags(MDMOID_DEV2_VLAN_TERMINATION,
                                 &ethVlanIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &ethVlanObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ethVlanObj->name, intfName))
         {
            ethVlanFound = TRUE;

            /* get the fullpath to this vlantermination obj */
            ethVlanPathDesc.oid = MDMOID_DEV2_VLAN_TERMINATION;
            ethVlanPathDesc.iidStack = ethVlanIidStack;
            ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ethVlanPathDesc, &fullPath);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot fullPath returns error. ret=%d", ret);
            }
         }

         cmsObj_free((void **) &ethVlanObj);
      }
   }

#ifdef SUPPORT_VXLAN_TUNNEL_TR181
   if (!ethVlanFound)
   {
      MdmPathDescriptor vxlanTunnelIfPathDesc = EMPTY_PATH_DESCRIPTOR;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2VxlanTunnelInterfaceObject *tunnelIfObj = NULL;
      UBOOL8 vxlanFound = FALSE;

      while (!vxlanFound &&
             cmsObj_getNextFlags(MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE,
                                 &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &tunnelIfObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(tunnelIfObj->name, intfName))
         {
            vxlanFound = TRUE;

            /* get the fullpath to this vxlan tunnel intf obj */
            vxlanTunnelIfPathDesc.oid = MDMOID_DEV2_VXLAN_TUNNEL_INTERFACE;
            vxlanTunnelIfPathDesc.iidStack = iidStack;
            if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&vxlanTunnelIfPathDesc,
                                                     &fullPath)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
            }
         }

         cmsObj_free((void **) &tunnelIfObj);
      }
   }
#endif

#ifdef SUPPORT_GRE_TUNNEL_TR181
   if (!ethVlanFound && fullPath == NULL)
   {
      MdmPathDescriptor greTunnelIfPathDesc = EMPTY_PATH_DESCRIPTOR;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2GreTunnelInterfaceObject *tunnelIfObj = NULL;
      UBOOL8 greIfFound = FALSE;

      while (!greIfFound && 
             cmsObj_getNextFlags(MDMOID_DEV2_GRE_TUNNEL_INTERFACE,
                                 &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &tunnelIfObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(tunnelIfObj->name, intfName))
         {
            greIfFound = TRUE;

            /* get the fullpath to this GRE tunnel intf obj */
            greTunnelIfPathDesc.oid = MDMOID_DEV2_GRE_TUNNEL_INTERFACE;
            greTunnelIfPathDesc.iidStack = iidStack;
            if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&greTunnelIfPathDesc,
                                                     &fullPath)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
            }
         }

         cmsObj_free((void **) &tunnelIfObj);
      }
   }
#endif

   if (!fullPath)
   {
      /* intfName must be either a L2 intfname, e.g. eth1,
       * or a virtual untagged intfname, e.g. eth1.0
       */
      char l2Ifname[CMS_IFNAME_LENGTH];
      char *p;

      /* If it is a virtual intfname, get the L2 name before the dot. */
      cmsUtl_strncpy(l2Ifname, intfName, sizeof(l2Ifname));

      if (cmsUtl_strncmp(l2Ifname, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
      {
         p = strchr(l2Ifname, '.');
         if (p)
            *p = '\0';
      }

      ret = qdmIntf_intfnameToFullPathLocked_dev2(l2Ifname, TRUE, &fullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_debug("Could not convert %s to L2 Fullpath, ret=%d", l2Ifname, ret);
         return ret;
      }
   }

   cmsLog_debug("intfName %s ==> fullPath %s", intfName, fullPath);

   ret = rutBridge_addFullPathToBridge_dev2(fullPath, brIntfName);
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

#ifdef BRCM_WLAN
   if ((!strncmp(intfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR))))
   {
      if ((ret = rutBridge_updateSSIDBridgeName(intfName, brIntfName)) != CMSRET_SUCCESS)
         cmsLog_error("Setting new bridgeName failed, ret=%d", ret);
   }
#endif
   return ret;
}


CmsRet rutBridge_addFullPathToBridge_dev2(const char *fullPath,
                                          const char *brIntfName)
{
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   Dev2BridgePortObject *brMgmtPortObj=NULL;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brMgmtIidStack = EMPTY_INSTANCE_ID_STACK;
   char *brPortFullPath=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: fullpath=%s brIntfName=%s", fullPath, brIntfName);

   /* First find the bridge */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
      {
         found = TRUE;
      }
      /* we don't need the bridge object, so always free it */
      cmsObj_free((void **) &brObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find bridge %s", brIntfName);
      return ret;
   }

   /* create a new Bridge.{i}.Port.{i} object under the found bridge */
   brPortIidStack = brIidStack;
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add Bridge.Port Instance, ret = %d", ret);
      return ret;
   }

   /* create fullpath to this new Bridge.Port object */
   {
      MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

      pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
      pathDesc.iidStack = brPortIidStack;
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get fullpath for Bridge.Port. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
         return ret;
      }
   }

   /* Find the management port object for this bridge */
   found = FALSE;
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                       &brIidStack,
                                       &brMgmtIidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brMgmtPortObj) == CMSRET_SUCCESS)
   {
      if (brMgmtPortObj->managementPort &&
          !cmsUtl_strcmp(brMgmtPortObj->name, brIntfName))
      {
         /* need this obj, so break now and free it later */
         found = TRUE;
         break;
      }
      cmsObj_free((void **) &brMgmtPortObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find management port for bridge %s", brIntfName);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
      return ret;
   }
   else
   {
      /* add the brPortFullPath to lowerLayers param of the Bridge mgmt port */
      char allLowerLayersStringBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};

      if (!IS_EMPTY_STRING(brMgmtPortObj->lowerLayers))
      {
         sprintf(allLowerLayersStringBuf, "%s", brMgmtPortObj->lowerLayers);
      }
      ret = cmsUtl_addFullPathToCSL(brPortFullPath, allLowerLayersStringBuf, sizeof(allLowerLayersStringBuf));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to add %s to %s", brPortFullPath, allLowerLayersStringBuf);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(brMgmtPortObj->lowerLayers,
                                     allLowerLayersStringBuf,
                                     mdmLibCtx.allocFlags);

         cmsLog_debug("new mgmtPortLL(len=%d)=%s",
                      cmsUtl_strlen(brMgmtPortObj->lowerLayers),
                      brMgmtPortObj->lowerLayers);
         ret = cmsObj_set(brMgmtPortObj, &brMgmtIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set brMgmtPortObj. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
         }
      }

      cmsObj_free((void **) &brMgmtPortObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
   }

   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }


   /*
    * Finally, now that everything is connected up correctly, get and set the
    * brPort object.
    */
   if ((ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack, 0, (void **) &brPortObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Bridge.Port, ret = %d", ret);
      return ret;
   }

   brPortObj->enable = TRUE;
   brPortObj->managementPort = FALSE;
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->lowerLayers, fullPath, mdmLibCtx.allocFlags);

   ret = cmsObj_set(brPortObj, &brPortIidStack);
   cmsObj_free((void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Bridge.Port. ret=%d", ret);
   }

   return ret;
}


CmsRet rutBridge_addLanIntfToBridge_dev2(const char *intfName,
                                         const char *brIntfName)
{
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   Dev2BridgePortObject *brMgmtPortObj=NULL;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brMgmtIidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   char *brPortFullPath=NULL;
   char *lowerLayerFullPath=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: intfName=%s brIntfName=%s", intfName, brIntfName);

   /* First find the bridge */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
      {
         found = TRUE;
      }
      /* we don't need the bridge object, so always free it */
      cmsObj_free((void **) &brObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find bridge %s", brIntfName);
      return ret;
   }

   {
      /* intfName must be either a L2 intfname, e.g. eth1,
       * or a virtual untagged intfname, e.g. eth1.0
       */
      char l2Ifname[CMS_IFNAME_LENGTH];
      char *p;

      /* If it is a virtual intfname, get the L2 name before the dot. */
      cmsUtl_strncpy(l2Ifname, intfName, sizeof(l2Ifname));

      if (cmsUtl_strncmp(l2Ifname, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
      {
         p = strchr(l2Ifname, '.');
         if (p)
            *p = '\0';
      }

      ret = qdmIntf_intfnameToFullPathLocked_dev2(l2Ifname, TRUE, &lowerLayerFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_debug("Could not convert %s to L2 Fullpath, ret=%d", l2Ifname, ret);
         return ret;
      }
   }

   /* create a new Bridge.{i}.Port.{i} object under the found bridge */
   brPortIidStack = brIidStack;
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add Bridge.Port Instance, ret = %d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(lowerLayerFullPath);
      return ret;
   }

   /* create fullpath to this new Bridge.Port object */
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
      pathDesc.iidStack = brPortIidStack;
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get fullpath for Bridge.Port. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
         CMSMEM_FREE_BUF_AND_NULL_PTR(lowerLayerFullPath);
         return ret;
      }
   }

   /* Find the management port object for this bridge */
   found = FALSE;
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                       &brIidStack,
                                       &brMgmtIidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brMgmtPortObj) == CMSRET_SUCCESS)
   {
      if (brMgmtPortObj->managementPort &&
          !cmsUtl_strcmp(brMgmtPortObj->name, brIntfName))
      {
         /* need this obj, so break now and free it later */
         found = TRUE;
         break;
      }
      cmsObj_free((void **) &brMgmtPortObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find management port for bridge %s", brIntfName);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
      CMSMEM_FREE_BUF_AND_NULL_PTR(lowerLayerFullPath);
      return ret;
   }
   else
   {
      /* add the brPortFullPath to lowerLayers param of the Bridge mgmt port */
      char allLowerLayersStringBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};

      if (!IS_EMPTY_STRING(brMgmtPortObj->lowerLayers))
      {
         sprintf(allLowerLayersStringBuf, "%s", brMgmtPortObj->lowerLayers);
      }
      ret = cmsUtl_addFullPathToCSL(brPortFullPath, allLowerLayersStringBuf, sizeof(allLowerLayersStringBuf));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to add %s to %s", brPortFullPath, allLowerLayersStringBuf);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(brMgmtPortObj->lowerLayers,
                                     allLowerLayersStringBuf,
                                     mdmLibCtx.allocFlags);

         cmsLog_debug("new mgmtPortLL(len=%d)=%s",
                      cmsUtl_strlen(brMgmtPortObj->lowerLayers),
                      brMgmtPortObj->lowerLayers);
         ret = cmsObj_set(brMgmtPortObj, &brMgmtIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set brMgmtPortObj. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
         }
      }

      cmsObj_free((void **) &brMgmtPortObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
   }

   if (ret != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(lowerLayerFullPath);
      return ret;
   }

   /*
    * Finally, now that everything is connected up correctly, get and set the
    * brPort object.
    */
   if ((ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack, 0, (void **) &brPortObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Bridge.Port, ret = %d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(lowerLayerFullPath);
      return ret;
   }

   brPortObj->enable = TRUE;
   brPortObj->managementPort = FALSE;
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->X_BROADCOM_COM_Name, intfName, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->lowerLayers, lowerLayerFullPath, mdmLibCtx.allocFlags);
   CMSMEM_FREE_BUF_AND_NULL_PTR(lowerLayerFullPath);

   ret = cmsObj_set(brPortObj, &brPortIidStack);
   cmsObj_free((void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Bridge.Port. ret=%d", ret);
   }

   return ret;
}


void rutBridge_addDynIntfToBridge_dev2(const char *intfName,
                                       const char *brIntfName)
{
   UBOOL8 isOVS = FALSE;

   if (rutBridge_getBridgeTypeByName_dev2(brIntfName, &isOVS) == CMSRET_SUCCESS)
   {
      rutLan_addDynInterfaceToBridge(intfName, brIntfName, isOVS);
   }
}

UBOOL8 rutBridge_isPortInOtherBridge(const char *thisBrName, const char *brPortName)
{
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   UBOOL8 found = FALSE;

   /* Check if the bridge port already exists in the bridge. */
   /* Loop thru all the bridge non-management ports to find all vlan interface
    * index that are in used.
    */
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE,
                              &brIidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&brObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, thisBrName))
      {
         /* skip this bridge */
         cmsObj_free((void **) &brObj);
         continue;
      }

      INIT_INSTANCE_ID_STACK(&brPortIidStack);
      while (!found &&
             cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                          &brIidStack,
                                          &brPortIidStack,
                                          OGF_NO_VALUE_UPDATE,
                                          (void **)&brPortObj) == CMSRET_SUCCESS)
      {
         if (!brPortObj->managementPort &&
             !cmsUtl_strcmp(brPortObj->name, brPortName))
         {
            found = TRUE;
         }
         cmsObj_free((void **) &brPortObj);
      }
      cmsObj_free((void **) &brObj);
   }

   return found;
}


void rutBridge_deleteIntfNameFromBridge_dev2(const char *intfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2BridgePortObject *portObj=NULL;
   UBOOL8 found=FALSE;

   cmsLog_debug("Entered: intfName=%s", intfName);

   /* find non-mgmt port with specified intfName */
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &portObj) == CMSRET_SUCCESS)
   {
      if (!portObj->managementPort &&
          !cmsUtl_strcmp(portObj->name, intfName))
      {
         found = TRUE;

#ifdef DMP_DEVICE2_VLANBRIDGE_1
         /* delete the associated vlan port before deleting the bridge port. */
         rutBridge_deleteVlanPort_dev2(&iidStack);
#endif
         /*
          * delete bridge port instance.  When this object is deleted,
          * an INTFSTACK_OBJECT_DELETED msg will get sent to ssk.
          * Ssk will update the lowerlayers param of the mgmt port, as
          * required by TR181 spec.  So no need to do that explictly here.
          */
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &iidStack);
      }

      cmsObj_free((void **)&portObj);
   }

   if (!found)
   {
      cmsLog_debug("Could not find Bridge.Port obj %s", intfName);
   }

   return;
}


void rutBridge_deleteDynIntfFromBridge_dev2(const char *intfName,
                                            const char *brIntfName)
{
   UBOOL8 isOVS = FALSE;

   if (rutBridge_getBridgeTypeByName_dev2(brIntfName, &isOVS) == CMSRET_SUCCESS)
   {
      rutLan_removeDynInterfaceFromBridge(intfName, brIntfName, isOVS);
   }
}

CmsRet rutBridge_getFilterDhcpVendorIdByBridgeName(const char *bridgeName, InstanceIdStack *iidStack, Dev2BridgeFilterObject **filterObj)
{
   char brIntfName[BUFLEN_16];
   char *ipIntfFullPath=NULL;
   UBOOL8 found = FALSE;
   UBOOL8 isLayer2 = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   /* Get bridge name by group name */
   ret = qdmIpIntf_getBridgeIntfNameByGroupNameLocked_dev2(bridgeName, brIntfName);

   ret = qdmIntf_intfnameToFullPathLocked_dev2(brIntfName, isLayer2, &ipIntfFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("bad ifName %s, ret=%d", brIntfName, ret);
      return CMSRET_INVALID_ARGUMENTS;
   }

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE_FILTER, iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) filterObj)) == CMSRET_SUCCESS)
   {
       if (!cmsUtl_strcmp((*filterObj)->interface, ipIntfFullPath))
       {
          found = TRUE;
       }
       else
           cmsObj_free((void **) &filterObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   if (found)
       return ret;

   return CMSRET_INVALID_ARGUMENTS;
}

void rutBridge_deleteFilterDhcpVendorId(const char *groupName)
{
   Dev2BridgeFilterObject *filterObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = rutBridge_getFilterDhcpVendorIdByBridgeName(groupName, &iidStack, &filterObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("no dhcp vendor id filter to delete for bridge %s", groupName);
      return;
   }

   cmsObj_free((void **) &filterObj);

   /* now our iidStack is pointing to the right instance, delete it */
   if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_FILTER, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not delete bridging filter dhcp vendor id, ret=%d", ret);
   }

   return;
}

SINT32 rutLan_getNextAvailableBridgeNumber_dev2()
{
   UBOOL8 inUseArray[MAX_LAYER2_BRIDGES] = {FALSE};
   UINT32 i;
   Dev2BridgeObject *brObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   /*
    * Enumerate through all of our bridges and for each bridge, mark that
    * bridge's slot in the inUseArray as TRUE;
    */
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj)) == CMSRET_SUCCESS)
   {
      if (brObj->X_BROADCOM_COM_Index >= 0)
      {
          cmsAst_assert(brObj->X_BROADCOM_COM_Index < MAX_LAYER2_BRIDGES);
          inUseArray[brObj->X_BROADCOM_COM_Index] = TRUE;
      }
      cmsObj_free((void **) &brObj);
   }

   /*
    * Now go through the array and return the first available.
    */
   for (i=0; i < MAX_LAYER2_BRIDGES; i++)
   {
      if (inUseArray[i] == FALSE)
      {
         cmsLog_debug("returning %d", (SINT32) i);
         return ((SINT32) i);
      }
   }

   cmsLog_error("All %d bridges in use!", MAX_LAYER2_BRIDGES);
   return (-1);
}


#ifdef DMP_X_BROADCOM_COM_DLNA_1
CmsRet rutLan_updateDlna_dev2(void)
{
   InstanceIdStack dmsIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack ipIidStack = EMPTY_INSTANCE_ID_STACK;
   DmsCfgObject *dmsObj = NULL;
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   char brIntfName[BUFLEN_32];
   UBOOL8 found=FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   if ((ret = cmsObj_get(MDMOID_DMS_CFG, &dmsIidStack, 0, (void **)&dmsObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_DMS_CFG> returns error. ret=%d", ret);
      return ret;
   }

   /* Generate the bridge interface name */
   sprintf(brIntfName, "br%d", dmsObj->brKey);

   /* First find the bridge */
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &ipIidStack,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipIntfObj->name, brIntfName))
      {
         found = TRUE;
      }
      cmsObj_free((void **) &ipIntfObj);
   }

   if (!found)
   {
       cmsLog_error("Could not find bridge %s", brIntfName);
       /* disable the DLNA due to the bridge interface isn't existed. */
       dmsObj->brKey = 0 ;
       dmsObj->enable = 0 ;
       ret = cmsObj_set(dmsObj, &dmsIidStack);
       cmsObj_free((void **) &dmsObj);
       if (ret != CMSRET_SUCCESS)
       {
           cmsLog_error("could not set DLNA cfg, ret=%d", ret);
           return ret;
       }
   }
   else
   {
       cmsObj_free((void **)&dmsObj);
   }

   cmsLog_debug("Exit with ret=%d", ret);

   return ret;
}
#endif

CmsRet rutBridge_addPortToBridge_dev2(const char *llayerFullPath,
                                      const char *brIntfName,
                                      UINT32 tpid,
                                      SINT32 vid,
                                      SINT32 defaultUserPriority,
                                      MdmPathDescriptor *brPortPathDesc)
{
   CmsRet ret = CMSRET_SUCCESS;

   /* To avoid duplicate coding, call rutBridge_movePortToBridge with
    * empty brPortName to indicate that this is adding a new port to the
    * bridge brIntfName, instead of moving an existing port from its current
    * bridge to the new bridge brIntfName.
    */
   ret = rutBridge_movePortToBridge_dev2(NULL,
                                         llayerFullPath,
                                         brIntfName,
                                         tpid,
                                         vid,
                                         defaultUserPriority,
                                         brPortPathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("rutBridge_movePortToBridge_dev2 failed. ret=%d", ret);
   }

   return ret;
}


CmsRet rutBridge_movePortToBridge_dev2(const char *brPortName,
                                       const char *llayerFullPath,
                                       const char *brIntfName,
                                       UINT32 tpid,
                                       SINT32 vid,
                                       SINT32 defaultUserPriority,
                                       MdmPathDescriptor *brPortPathDesc)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack oldPortIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brMgmtIidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   Dev2BridgePortObject *brMgmtPortObj=NULL;
   char *brPortFullPath=NULL;
   UBOOL8 found;
   UBOOL8 foundSameVidPortInBridge = FALSE;
   InstanceIdStack foundBrIidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("Entered: brPortName=%s llayerFullpath=%s brIntfName=%s vid=%d",
                brPortName, llayerFullPath, brIntfName, vid);

   if (!IS_EMPTY_STRING(brPortName))
   {
      /* This must be interface grouping. */
      /* Get the bridge port iidStack in current bridge. */
      found = FALSE;
      while (!found &&
             (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE_PORT,
                                        &oldPortIidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&brPortObj) == CMSRET_SUCCESS))
      {
         if (!brPortObj->managementPort &&
             !cmsUtl_strcmp(brPortObj->name, brPortName))
         {
            found = TRUE;

            if (brPortObj->enable)
            {
               /* disable the port in current bridge. */
               brPortObj->enable = FALSE;
               ret = cmsObj_set(brPortObj, &oldPortIidStack);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to set Dev2BridgePortObject, ret=%d", ret);
               }               
            }
         }
         cmsObj_free((void **) &brPortObj);
      }

      if (!found)
      {
         cmsLog_error("Cannot find bridge port %s in any bridge.", brPortName);
         return ret;
      }

      {
         if (strncmp(brPortName, ETH_IFC_STR, strlen(ETH_IFC_STR)) == 0)
         {
            char parentBrIntfNameBuf[CMS_IFNAME_LENGTH]={0};
            UBOOL8 isUpstream;
            char l2IntfName[BUFLEN_32];
            char *ptr;
            UBOOL8 isOVS;

            /*
             * First figure out what my Linux bridge intf name is.
             */
            ret = rutBridge_getParentBridgeIntfName_dev2(&oldPortIidStack, parentBrIntfNameBuf);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get parent bridge intfName, iidStack=%s",
                                            cmsMdm_dumpIidStack(&oldPortIidStack));
               return ret;
            }

            ret = rutBridge_getParentBridgeType_dev2(&oldPortIidStack, &isOVS);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get parent bridge type, iidStack=%s",
                                            cmsMdm_dumpIidStack(&oldPortIidStack));
               return ret;
            }

            strncpy(l2IntfName, brPortName, sizeof(l2IntfName)-1);
            l2IntfName[sizeof(l2IntfName)-1] = '\0';
            ptr = strchr(l2IntfName, '.');
            if (ptr != NULL)
               *ptr = '\0';

            isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

            rutLan_removeInterfaceFromBridgeEx(brPortName, parentBrIntfNameBuf, isOVS);
            rutMulti_updateIgmpMldProxyIntfList();
         }
      }
   }

   /* walk through all bridges */
   found = FALSE;
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE,
                                     &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
      {
         /* found target bridge!! */
         foundBrIidStack = brIidStack;
         found = TRUE;
      }

      /* Only for adding new bridge port case */
      /* Check if any port with the same vid already exists in the bridge. */
      if (IS_EMPTY_STRING(brPortName))
      {
         char llayerIntfname[BUFLEN_64+1];
         SINT32 llayerIntfnameLen;
         UINT32 vlanTpid;
         SINT32 vlanId;
         SINT32 vlanPr;

         qdmIntf_getIntfnameFromFullPathLocked_dev2(llayerFullPath,
                                             llayerIntfname, sizeof(llayerIntfname));
         llayerIntfnameLen = strlen(llayerIntfname);

         INIT_INSTANCE_ID_STACK(&brPortIidStack);
         while (!foundSameVidPortInBridge &&
                cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                             &brIidStack,
                                             &brPortIidStack,
                                             OGF_NO_VALUE_UPDATE,
                                             (void **)&brPortObj) == CMSRET_SUCCESS)
         {
            if (!brPortObj->managementPort &&
                !cmsUtl_strncmp(brPortObj->name, llayerIntfname, llayerIntfnameLen))
            {
               INIT_PATH_DESCRIPTOR(&pathDesc);
               pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
               pathDesc.iidStack = brPortIidStack;

               qdmVlan_getVlanInfoLocked_dev2(&pathDesc, &vlanTpid, &vlanId, &vlanPr);

               if (vlanId == vid)
               {
                  foundSameVidPortInBridge = TRUE;
                  if (brPortPathDesc != NULL)
                  {
                     /* return the existing port path descriptor */
                     *brPortPathDesc = pathDesc;
                  }
               }
            }
            cmsObj_free((void **) &brPortObj);
         }

         if (foundSameVidPortInBridge)
         {
            cmsLog_error("Bridge port with same vid(%d) already exists in %s",
			                            vlanId, brObj->X_BROADCOM_COM_IfName);
            cmsObj_free((void **) &brObj);
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      cmsObj_free((void **) &brObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find bridge %s", brIntfName);
      return ret;
   }

   /* create a new Bridge.{i}.Port.{i} object under the found bridge */
   brPortIidStack = foundBrIidStack;
   ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add Bridge.Port Instance, ret = %d", ret);
      return ret;
   }

   /* create fullpath to this new Bridge.Port object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
   pathDesc.iidStack = brPortIidStack;
   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get fullpath for Bridge.Port. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
      return ret;
   }

   /* Find the management port object for this bridge */
   found = FALSE;
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                       &foundBrIidStack,
                                       &brMgmtIidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brMgmtPortObj) == CMSRET_SUCCESS)
   {
      if (brMgmtPortObj->managementPort &&
          !cmsUtl_strcmp(brMgmtPortObj->name, brIntfName))
      {
         /* need this obj, so break now and free it later */
         found = TRUE;
         break;
      }
      cmsObj_free((void **) &brMgmtPortObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find management port for bridge %s", brIntfName);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
      return ret;
   }
   else
   {
      /* add the brPortFullPath to lowerLayers param of the Bridge mgmt port */
      char allLowerLayersStringBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};

      if (!IS_EMPTY_STRING(brMgmtPortObj->lowerLayers))
      {
         sprintf(allLowerLayersStringBuf, "%s", brMgmtPortObj->lowerLayers);
      }
      ret = cmsUtl_addFullPathToCSL(brPortFullPath, allLowerLayersStringBuf, sizeof(allLowerLayersStringBuf));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to add %s to %s", brPortFullPath, allLowerLayersStringBuf);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(brMgmtPortObj->lowerLayers,
                                     allLowerLayersStringBuf,
                                     mdmLibCtx.allocFlags);

         cmsLog_debug("new mgmtPortLL(len=%d)=%s",
                      cmsUtl_strlen(brMgmtPortObj->lowerLayers),
                      brMgmtPortObj->lowerLayers);
         ret = cmsObj_set(brMgmtPortObj, &brMgmtIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set brMgmtPortObj. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);
         }
      }

      cmsObj_free((void **) &brMgmtPortObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
   }

   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }


   /*
    * Finally, now that everything is connected up correctly, get and set the
    * brPort object.
    */
   ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack, 0, (void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Bridge.Port, ret = %d", ret);
      return ret;
   }

   brPortObj->enable = FALSE;
   brPortObj->managementPort = FALSE;
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->lowerLayers, llayerFullPath, mdmLibCtx.allocFlags);
   if (!IS_EMPTY_STRING(brPortName))
   {
      /* For interface grouping, we want to keep the port name unchange when
       * moving the port to the new bridge. Pass in the port name so that
       * RCL can use it directly.
       */
      CMSMEM_REPLACE_STRING_FLAGS(brPortObj->X_BROADCOM_COM_Name, brPortName, mdmLibCtx.allocFlags);
   }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
   brPortObj->TPID = tpid;

   if (defaultUserPriority >= 0)
   {
      brPortObj->defaultUserPriority = defaultUserPriority;
      brPortObj->priorityTagging = TRUE;
   }
   else
   {
      brPortObj->defaultUserPriority = 0;
      brPortObj->priorityTagging = FALSE;
   }

   if (vid < 0)
   {
      /* neither VLAN nor priority tagging. */
      brPortObj->priorityTagging = FALSE;
   }
#endif

   ret = cmsObj_set(brPortObj, &brPortIidStack);
   cmsObj_free((void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Bridge.Port. ret=%d", ret);
   }

   if (brPortPathDesc != NULL)
   {
      INIT_PATH_DESCRIPTOR(brPortPathDesc);
      brPortPathDesc->oid = MDMOID_DEV2_BRIDGE_PORT;
      brPortPathDesc->iidStack = brPortIidStack;
   }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
   /* If vid <= 0, bridge port is not vlan port. */
   if (vid > 0)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
      pathDesc.iidStack = brPortIidStack;
      ret = rutBridge_addVlanPort_dev2(vid, &foundBrIidStack, &pathDesc);
   }
#endif

   /* Enable bridge port here to avoid trigger creating vlan interface twice. */
   ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack, 0, (void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Bridge.Port, ret = %d", ret);
      return ret;
   }

   brPortObj->enable = TRUE;
   ret = cmsObj_set(brPortObj, &brPortIidStack);
   cmsObj_free((void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Bridge.Port. ret=%d", ret);
   }

   if (!IS_EMPTY_STRING(brPortName))
   {
      /* Now, delete the port from its old bridge. */
#ifdef DMP_DEVICE2_VLANBRIDGE_1
      rutBridge_deleteVlanPort_dev2(&oldPortIidStack);
#endif
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &oldPortIidStack);
   }

   return ret;
}

#ifdef DMP_DEVICE2_VLANBRIDGE_1
CmsRet rutBridge_addVlanPort_dev2(SINT32 vlanId,
                                  const InstanceIdStack *brIidStack,
                                  const MdmPathDescriptor *brPortPathDesc)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 foundVlan = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor brVlanPathDesc = EMPTY_PATH_DESCRIPTOR;
   Dev2BridgeVlanObject *brVlanObj=NULL;
   Dev2BridgeVlanPortObject *brVlanPortObj=NULL;
   char *brVlanFullPath=NULL;
   char *brPortFullPath=NULL;
   char name[BUFLEN_64+1];

   cmsLog_debug("Enter: vlanId=%d brIidStack=%s", vlanId, cmsMdm_dumpIidStack(brIidStack));

   while (!foundVlan &&
          (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN,
                                       brIidStack,
                                       &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **)&brVlanObj)) == CMSRET_SUCCESS)
   {
      if (brVlanObj->VLANID == vlanId)
      {
         foundVlan = TRUE;
      }

      cmsObj_free((void **)&brVlanObj);
   }

   if (!foundVlan)
   {
      /* Add new Bridge VLAN object instance */
      iidStack = *brIidStack;
      ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE_VLAN, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to add BRIDGE_VLAN Instance, ret = %d", ret);
         return ret;
      }

      ret = cmsObj_get(MDMOID_DEV2_BRIDGE_VLAN, &iidStack, 0, (void **)&brVlanObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get BRIDGE_VLAN object, ret = %d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &iidStack);
         return ret;
      }

      brVlanObj->enable = TRUE;
      sprintf(name, "VLAN%d", vlanId);
      CMSMEM_REPLACE_STRING(brVlanObj->name, name);
      brVlanObj->VLANID = vlanId;

      ret = cmsObj_set(brVlanObj, &iidStack);
      cmsObj_free((void **) &brVlanObj);

      if (ret  != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set BRIDGE VLAN object. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &iidStack);
         return ret;
      }
   }

   brVlanPathDesc.oid = MDMOID_DEV2_BRIDGE_VLAN;
   brVlanPathDesc.iidStack = iidStack;
   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&brVlanPathDesc, &brVlanFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      /* delete the newly added VLAN object */
      if (!foundVlan)
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &brVlanPathDesc.iidStack);
      return ret;
   }

   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(brPortPathDesc, &brPortFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      /* delete the newly added VLAN object */
      if (!foundVlan)
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &brVlanPathDesc.iidStack);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brVlanFullPath);
      return ret;
   }

   /* Add new Bridge VLAN object instance as child of parent bridge. */
   iidStack = *brIidStack;
   ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE_VLAN_PORT, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add BRIDGE_VLAN_PORT Instance, ret = %d", ret);
      /* delete the newly added VLAN object */
      if (!foundVlan)
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &brVlanPathDesc.iidStack);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brVlanFullPath);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
      return ret;
   }

   ret = cmsObj_get(MDMOID_DEV2_BRIDGE_VLAN_PORT, &iidStack, 0, (void **)&brVlanPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get BRIDGE_VLAN_PORT object, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN_PORT, &iidStack);
      /* delete the newly added VLAN object */
      if (!foundVlan)
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &brVlanPathDesc.iidStack);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brVlanFullPath);
      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
      return ret;
   }

   brVlanPortObj->enable = TRUE;
   brVlanPortObj->untagged = FALSE;  //untagged;
   CMSMEM_REPLACE_STRING(brVlanPortObj->VLAN, brVlanFullPath);
   CMSMEM_REPLACE_STRING(brVlanPortObj->port, brPortFullPath);

   CMSMEM_FREE_BUF_AND_NULL_PTR(brVlanFullPath);
   CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

   ret = cmsObj_set(brVlanPortObj, &iidStack);
   cmsObj_free((void **) &brVlanPortObj);

   if (ret  != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set BRIDGE VLAN_PORT object. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN_PORT, &iidStack);
      /* delete the newly added VLAN object */
      if (!foundVlan)
         cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN, &brVlanPathDesc.iidStack);
      return ret;
   }

   cmsLog_debug("Exit, ret=%d", ret);

   return ret;
}


CmsRet rutBridge_deRefVlanFromVlanPort_dev2(const InstanceIdStack *brVlanIidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
   char *brVlanFullPath = NULL;

   cmsLog_debug("brVlanIidStack=%s", cmsMdm_dumpIidStack(brVlanIidStack));

   pathDesc.oid = MDMOID_DEV2_BRIDGE_VLAN;
   pathDesc.iidStack = *brVlanIidStack;

   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brVlanFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get fullpath for Bridge.Vlan. ret=%d", ret);
      return ret;
   }

   /* The parent Bridge object is 1 level above the vlan object. */
   brIidStack = *brVlanIidStack;
   POP_INSTANCE_ID(&brIidStack);

   while (ret == CMSRET_SUCCESS &&
          (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                 &brIidStack,
                                 &brVlanPortIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &brVlanPortObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brVlanPortObj->VLAN, brVlanFullPath))
      {
         CMSMEM_REPLACE_STRING_FLAGS(brVlanPortObj->VLAN, "", mdmLibCtx.allocFlags);
         ret = cmsObj_set(brVlanPortObj, &brVlanPortIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Bridge VLAN port object set failed. ret=%d", ret);
         }
      }
      cmsObj_free((void **) &brVlanPortObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(brVlanFullPath);

   if (ret == CMSRET_NO_MORE_INSTANCES) ret = CMSRET_SUCCESS;

   return ret;
}


CmsRet rutBridge_deRefPortFromVlanPort_dev2(const InstanceIdStack *brPortIidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
   char *brPortFullPath = NULL;

   cmsLog_debug("brPortIidStack=%s", cmsMdm_dumpIidStack(brPortIidStack));

   pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
   pathDesc.iidStack = *brPortIidStack;

   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot failed. ret=%d DEV2_BRIDGE_PORT iidStack=%s",
                   ret, cmsMdm_dumpIidStack(brPortIidStack));
      return ret;
   }

   /* The parent Bridge object is 1 level above the port object. */
   brIidStack = *brPortIidStack;
   POP_INSTANCE_ID(&brIidStack);

   while (ret == CMSRET_SUCCESS &&
          (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                 &brIidStack,
                                 &brVlanPortIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &brVlanPortObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brVlanPortObj->port, brPortFullPath))
      {
         CMSMEM_REPLACE_STRING_FLAGS(brVlanPortObj->port, "", mdmLibCtx.allocFlags);
         ret = cmsObj_set(brVlanPortObj, &brVlanPortIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Bridge VLAN port object set failed. ret=%d", ret);
         }
      }
      cmsObj_free((void **) &brVlanPortObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

   if (ret == CMSRET_NO_MORE_INSTANCES) ret = CMSRET_SUCCESS;

   return ret;
}


CmsRet rutBridge_getVlanPortInfo_dev2(const InstanceIdStack *brPortIidStack,
                                      UBOOL8 *untagged,
                                      SINT32 *vlanId)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
   char *brPortFullPath = NULL;
   UBOOL8 foundVlanPort = FALSE;

   cmsLog_debug("Enter: brPortIidStack=%s", cmsMdm_dumpIidStack(brPortIidStack));

   /* Initialize output parameters. */
   *untagged = TRUE;
   *vlanId   = -1;

   pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
   pathDesc.iidStack = *brPortIidStack;
   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("pathDescriptorToFullPathNoEndDot failed. ret=%d DEV2_BRIDGE_PORT iidStack=%s",
                   ret, cmsMdm_dumpIidStack(brPortIidStack));
      return ret;
   }

   /* The parent Bridge object is 1 level above the port object. */
   brIidStack = *brPortIidStack;
   POP_INSTANCE_ID(&brIidStack);

   /* find the associated vlan port */
   while (!foundVlanPort &&
          (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                               &brIidStack,
                               &brVlanPortIidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&brVlanPortObj)) == CMSRET_SUCCESS)
   {
      if (brVlanPortObj->enable &&
          !cmsUtl_strcmp(brVlanPortObj->port, brPortFullPath))
      {
         foundVlanPort = TRUE;

         if (!IS_EMPTY_STRING(brVlanPortObj->VLAN))
         {
            Dev2BridgeVlanObject *brVlanObj = NULL;

            INIT_PATH_DESCRIPTOR(&pathDesc);
            ret = cmsMdm_fullPathToPathDescriptor(brVlanPortObj->VLAN, &pathDesc);
            if (ret == CMSRET_SUCCESS)
            {
               ret = cmsObj_get(MDMOID_DEV2_BRIDGE_VLAN,
                                &pathDesc.iidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **)&brVlanObj);
               if (ret == CMSRET_SUCCESS)
               {
                  *untagged = brVlanPortObj->untagged;
                  *vlanId   = brVlanObj->VLANID;
                  cmsObj_free((void **)&brVlanObj);
               }
               else
               {
                  cmsLog_error("cmsObj_get failed. ret=%d DEV2_BRIDGE_VLAN, iidStack=%s",
                               ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
               }
            }
            else
            {
               cmsLog_error("fullPathToPathDescriptor failed. ret=%d path=%s",
                            ret, brVlanPortObj->VLAN);
            }
         }
      }
      cmsObj_free((void **)&brVlanPortObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

   if (ret == CMSRET_NO_MORE_INSTANCES) ret = CMSRET_SUCCESS;
   return ret;
}


CmsRet rutBridge_configVlanRules_dev2(char *portName,
                                      char *l2Ifname,
                                      UINT32 tpid,
                                      UBOOL8 priorityTagging,
                                      SINT32 pbits,
                                      UBOOL8 untagged,
                                      SINT32 vlanId)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter: portName=%s l2Ifname=%s tpid=0x%x priorityTagging=%d pbits=%d untagged=%d vlanId=%d",
                portName, l2Ifname, tpid, priorityTagging, pbits, untagged, vlanId);

   if (strchr(portName, '.') == NULL ||
       !cmsUtl_strncmp(portName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
   {
      /* Bridge port is not a vlan device. Do nothing. */
      return CMSRET_SUCCESS;
   }

   if (untagged)
   {
      if (priorityTagging)
      {
         vlanId = 0;    /* priority tagging */
      }
      else
      {
         vlanId = -1;   /* no tagging */
      }
   }

   cmsLog_debug("Calling rutBridge_StartL3Interface. vlanId=%d pbits=%d tpid=0x%X l2Ifname=%s port=%s",
                vlanId, pbits, tpid, l2Ifname, portName);
   ret = rutBridge_startL3Interface(vlanId, pbits, tpid, l2Ifname, portName);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("rutBridge_startL3Interface failed. ret=%d", ret);
   }

   return ret;
}


CmsRet rutBridge_configVlan_dev2(const InstanceIdStack *brVlanIidStack,
                                 SINT32 vlanId)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
   char *brVlanFullPath = NULL;
   UBOOL8 untagged = TRUE;

   cmsLog_debug("Enter: brVlanIidStack=%s", cmsMdm_dumpIidStack(brVlanIidStack));

   /* Config vlan rules for all bridge ports associated with this vlan */

   pathDesc.oid = MDMOID_DEV2_BRIDGE_VLAN;
   pathDesc.iidStack = *brVlanIidStack;
   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brVlanFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get fullpath for Bridge.Vlan. ret=%d", ret);
      return ret;
   }

   /* The parent Bridge object is 1 level above the VLAN object. */
   brIidStack = *brVlanIidStack;
   POP_INSTANCE_ID(&brIidStack);

   /* get the bridge VLAN port object */
   while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                    &brIidStack,
                                    &brVlanPortIidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **) &brVlanPortObj)) == CMSRET_SUCCESS)
   {
      if (brVlanPortObj->enable &&
          !cmsUtl_strcmp(brVlanPortObj->VLAN, brVlanFullPath))
      {
         if (vlanId >= 0)
         {
            untagged = brVlanPortObj->untagged;
         }
         else
         {
            /* vlanId < 0 means this VLAN object is going to be deleted.
             * So we want to config port vlan rules as non-vlan tagging,
             * regardless of the value of brVlanPortObj->untagged.
             */
            untagged = TRUE;
         }

         cmsLog_debug("Calling rutBridge_configPort_dev2: port=%s untagged=%d vlanId=%d",
                      brVlanPortObj->port, untagged, vlanId);
         ret = rutBridge_configPort_dev2(brVlanPortObj->port, untagged, vlanId);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
            cmsObj_free((void **)&brVlanPortObj);
            break;
         }
      }

      cmsObj_free((void **)&brVlanPortObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(brVlanFullPath);

   if (ret == CMSRET_NO_MORE_INSTANCES) ret = CMSRET_SUCCESS;

   return ret;
}


CmsRet rutBridge_configPort_dev2(const char *brPortFullPath,
                                 UBOOL8 untagged,
                                 SINT32 vlanId)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   Dev2BridgePortObject *brPortObj = NULL;
   char brIfname[BUFLEN_64+1]={0};
   char l2Ifname[CMS_IFNAME_LENGTH+1]={0};

   cmsLog_debug("Enter: brPortFullPath=%s untagged=%d vlanId=%d",
                brPortFullPath, untagged, vlanId);

   /* If this vlan port is not associated with a bridge port, do nothing. */
   if (IS_EMPTY_STRING(brPortFullPath))
   {
      /* do nothing */
      return CMSRET_SUCCESS;
   }

   /* get the bridge port object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(brPortFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed. ret=%d", ret);
      return ret;
   }

   ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE,
                    (void **)&brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get failed. MDMOID_DEV2_BRIDGE_PORT, ret=%d", ret);
      return ret;
   }

   /* We don't allow a bridge port be configured with both VLANTermination and
    * VLANBridge objects.
    */
   if (cmsUtl_strcasestr(brPortObj->lowerLayers, "VLANTermination"))
   {
      cmsLog_error("bridge port %s had been configured with VLANTermination object.",
                   brPortObj->name);
      cmsObj_free((void **) &brPortObj);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (cmsUtl_strcmp(brPortObj->status, MDMVS_UP))
   {
      /* Bridge port is not UP. do nothing. */
      cmsLog_debug("bridge port %s is not UP.", brPortObj->name);
      cmsObj_free((void **) &brPortObj);
      return CMSRET_SUCCESS;
   }

   if (strchr(brPortObj->name, '.') == NULL)
   {
      /* Bridge port is not a vlan device. Do nothing. */
      cmsObj_free((void **) &brPortObj);
      return CMSRET_SUCCESS;
   }

   ret = rutBridge_getParentBridgeIntfName_dev2(&pathDesc.iidStack, brIfname);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get parent bridge intfName, iidStack=%s",
                   cmsMdm_dumpIidStack(&pathDesc.iidStack));
      return ret;
   }

   if (!rutBridge_isPortInOtherBridge(brIfname, brPortObj->name))
   {
      ret = qdmIntf_fullPathToIntfnameLocked_dev2(brPortObj->lowerLayers, l2Ifname);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. lowerLayers=%s ret=%d",
                      brPortObj->lowerLayers, ret);
      }
      else
      {
         ret = rutBridge_configVlanRules_dev2(brPortObj->name,
                                              l2Ifname,
                                              brPortObj->TPID,
                                              brPortObj->priorityTagging,
                                              brPortObj->defaultUserPriority,
                                              untagged,
                                              vlanId);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("rutBridge_configVlanRules_dev2 failed. ret=%d", ret);
         }
      }
   }
   cmsObj_free((void **) &brPortObj);

   return ret;
}


CmsRet rutBridge_deleteVlanPort_dev2(const InstanceIdStack *brPortIidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2BridgePortObject *brPortObj=NULL;

   cmsLog_debug("Enter: brPortIidStack=%s", cmsMdm_dumpIidStack(brPortIidStack));

   ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, brPortIidStack, OGF_NO_VALUE_UPDATE,
                    (void **)&brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get failed. ret=%d DEV2_BRIDGE_PORT iidStack=%s",
                   ret, cmsMdm_dumpIidStack(brPortIidStack));
   }
   else
   {
      if (!brPortObj->managementPort)
      {
         MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
         char *brPortFullPath = NULL;

         pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
         pathDesc.iidStack = *brPortIidStack;

         ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("pathDescriptorToFullPath failed. ret=%d DEV2_BRIDGE_PORT iidStack=%s",
                         ret, cmsMdm_dumpIidStack(brPortIidStack));
         }
         else
         {
            InstanceIdStack brIidStack;
            InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
            Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
            char vlanFullPath[MDM_SINGLE_FULLPATH_BUFLEN] = {0};
            UBOOL8 found = FALSE;

            /* The parent Bridge object is 1 level above the Port object. */
            brIidStack = *brPortIidStack;
            POP_INSTANCE_ID(&brIidStack);

            /* find the associated vlan port */
            while (!found &&
                   cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                        &brIidStack,
                                        &brVlanPortIidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&brVlanPortObj) == CMSRET_SUCCESS)
            {
               if (!cmsUtl_strcmp(brVlanPortObj->port, brPortFullPath))
               {
                  found = TRUE;

                  /* delete the VLAN port object */
                  ret = cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                              &brVlanPortIidStack);
                  if (ret != CMSRET_SUCCESS)
                  {
                     cmsLog_error("cmsObj_deleteInstance failed. ret=%d DEV2_BRIDGE_VLAN_PORT iidStack=%s",
                                  ret, cmsMdm_dumpIidStack(&brVlanPortIidStack));
                  }
                  else
                  {
                     /* save the vlan fullpath */
                     cmsUtl_strncpy(vlanFullPath, brVlanPortObj->VLAN, sizeof(vlanFullPath));
                  }
               }
               cmsObj_free((void **)&brVlanPortObj);
            }
            CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

            if (!IS_EMPTY_STRING(vlanFullPath))
            {
               /* delete the VLAN object if it is not referred by other VLAN ports. */
               found = FALSE;
               INIT_INSTANCE_ID_STACK(&brVlanPortIidStack);
               while (!found &&
                      cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                           &brIidStack,
                                           &brVlanPortIidStack,
                                           OGF_NO_VALUE_UPDATE,
                                           (void **)&brVlanPortObj) == CMSRET_SUCCESS)
               {
                  if (!cmsUtl_strcmp(brVlanPortObj->VLAN, vlanFullPath))
                  {
                     found = TRUE;
                  }
                  cmsObj_free((void **)&brVlanPortObj);
               }

               if (!found)
               {
                  /* delete the VLAN object */
                  ret = cmsMdm_fullPathToPathDescriptor(vlanFullPath, &pathDesc);
                  if (ret != CMSRET_SUCCESS)
                  {
                     cmsLog_error("fullPathToPathDescriptor failed. ret=%d fullpath=%s",
                                  ret, vlanFullPath);
                  }
                  else
                  {
                     ret = cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_VLAN,
                                                 &pathDesc.iidStack);
                     if (ret != CMSRET_SUCCESS)
                     {
                        cmsLog_error("cmsObj_deleteInstance failed. ret=%d DEV2_BRIDGE_VLAN iidStack=%s",
                                     ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
                     }
                  }
               }
            }
         }
      }
      cmsObj_free((void **)&brPortObj);
   }

   return ret;
}


CmsRet rutBridge_startL3Interface(SINT32 vlanMuxId __attribute__((unused)),
                                  SINT32 vlanMuxPr __attribute((unused)),
                                  UINT32 vlanTpid __attribute((unused)),
                                  char * l2IfName,
                                  char * baseL3IfName)
{
   char cmdStr[BUFLEN_128];
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter: vlanId=%d vlan801p=%d vlanTpid=0x%X L2=%s BaseL3=%s",
                vlanMuxId, vlanMuxPr, vlanTpid, l2IfName, baseL3IfName);

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
   {
      vlansubif_options_t vlan_subif_options = {
         .name = baseL3IfName,
         .pbit = vlanMuxPr,
         .tpid = vlanTpid,
         .is_ok_if_exists = 1,
         .is_phyif_other_owned = 0
      };

      if (rut_wanGetIntfIndex(baseL3IfName) > 0)
      {
         cmsLog_debug("Intf already exist <%s>. Re-initialized\n",baseL3IfName);
      }

#ifdef DMP_X_ITU_ORG_GPON_1
      if (rutOmci_isEthPortControlledByOmci(l2IfName) == TRUE)
      {
         vlan_subif_options.is_phyif_other_owned = 1;
      }
#endif

      /* Create untagged virtual interface */
      vlanSubif_createVlanInterface(l2IfName, vlanMuxId, &vlan_subif_options);

      if (!rut_waitIntfExists(baseL3IfName))
      {
         cmsLog_error("Failed to create %s", baseL3IfName);
         return CMSRET_INTERNAL_ERROR;
      }
   }
#endif

   /* if layer 3 IfName differs from layer 2 ifName, do a "ifconfig L3IfName up" */
   if (cmsUtl_strcmp(l2IfName, baseL3IfName))
   {
      if (rut_waitIntfExists(baseL3IfName))
      {
         /* bring interface up */
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", baseL3IfName);
         rut_doSystemAction("rutWan_startL3Interface: ifconfig L3IfName up", (cmdStr));
      }
      else
      {
         cmsLog_error("L2IfName %s is not up", baseL3IfName);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   return ret;
}
#endif   /* DMP_DEVICE2_VLANBRIDGE_1 */

#ifdef BRCM_WLAN
CmsRet rutBridge_updateSSIDBridgeName(const char *intfName, const char *brIntfName)
{
    Dev2WifiSsidObject *ssidObj = NULL;
    InstanceIdStack iidStack= EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;
    SINT32 brIndex;

    ret = rutBridge_getBridgeIndexByName_dev2(brIntfName, &brIndex);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get bridge index for %s", brIntfName);
        return ret;
    }

    while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_SSID, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ssidObj)) == CMSRET_SUCCESS)
    {
        if (!cmsUtl_strcmp(ssidObj->name, intfName))
        {
            ssidObj->X_BROADCOM_COM_WlBrIndex = brIndex;
            /*  When configuring interface grouping from WEB UI or CLI(intfgroup) with multiple wlan interfaces,
             *  this function will be invoked multiple times. If calling normal cmsObj_set() here, then it will
             *  trigger RCL function to request wlssk restart, and eventually result in a consecutive wlssk restart,
             *  but it is what we don't want to see. Hence, we invoke cmsObj_setFlags() with OSF_NO_RCL_CALLBACK instead
             *  to avoid triggering restart, and the restart thing should be done somewhere outside this function.
             *  For the interface grouping case, we only do restart once after all the SSID objects have been set finished,
             *  please see cgiPortMapAdd() or processIntfGroupCmd() for the details.
             */
            ret = cmsObj_setFlags((void *)ssidObj, &iidStack, OSF_NO_RCL_CALLBACK);
            cmsObj_free((void **)&ssidObj);
            break;
        }
        cmsObj_free((void **) &ssidObj);
    }

    if (ret != CMSRET_SUCCESS)
       cmsLog_error("Setting new bridgeName failed, ret=%d", ret);

    return ret;
}
#endif

#endif  /* DMP_DEVICE2_BRIDGE_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */



