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


/*!\file mdm2_init.c
 * \brief MDM initialization for PURE181 Device based tree, used by multiple
 *        components, so it has to stay in cms_core.
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


static CmsRet addFullPathToMgmtPortLowerLayers(const InstanceIdStack *brIidStack,
                                               const char *fullpath)
{
   Dev2BridgePortObject *mgmtBrPortObj=NULL;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   char allLowerLayersStringBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};
   CmsRet ret;

   /* Get the management port object of this bridge object */
   while (!found &&
          (ret = mdm_getNextObjectInSubTree(MDMOID_DEV2_BRIDGE_PORT,
                               brIidStack, &brPortIidStack,
                              (void **) &mgmtBrPortObj)) == CMSRET_SUCCESS)
   {
      if (mgmtBrPortObj->managementPort)
      {
         found = TRUE;
      }
      else
      {
         mdm_freeObject((void **)&mgmtBrPortObj);
      }
   }

   if (!found)
   {
      cmsLog_error("no management port found in Bridge %s",
                   cmsMdm_dumpIidStack(brIidStack));
      return ret;
   }

   /* Add new fullpath to existing LowerLayers string */
   sprintf(allLowerLayersStringBuf, "%s", mgmtBrPortObj->lowerLayers);
   ret = cmsUtl_addFullPathToCSL(fullpath, allLowerLayersStringBuf, sizeof(allLowerLayersStringBuf));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to add %s to %s", fullpath, allLowerLayersStringBuf);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(mgmtBrPortObj->lowerLayers, allLowerLayersStringBuf, mdmLibCtx.allocFlags);

      cmsLog_debug("new mgmtPortLL(len=%d)=%s",
                   cmsUtl_strlen(mgmtBrPortObj->lowerLayers),
                   mgmtBrPortObj->lowerLayers);
      ret = mdm_setObject((void **) &mgmtBrPortObj, &brPortIidStack, FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set brPortObj. ret=%d", ret);
      }

   }

   mdm_freeObject((void **)&mgmtBrPortObj);

   return ret;
}


CmsRet mdmInit_addBridgePortObject_dev2(const MdmPathDescriptor *brPathDesc,
                                        const char *ifname,
                                        UBOOL8 isManagementPort,
                                        const char *lowerLayers,
                                        MdmPathDescriptor *pathDesc)
{
   Dev2BridgePortObject *brPortObj=NULL;
   char *brPortFullPath=NULL;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("lowerLayers=%s bridge iidStack=%s",
                lowerLayers, cmsMdm_dumpIidStack(&brPathDesc->iidStack));

   INIT_PATH_DESCRIPTOR(pathDesc);
   pathDesc->oid = MDMOID_DEV2_BRIDGE_PORT;
   pathDesc->iidStack = brPathDesc->iidStack;  // must add port as child of parent bridge
   if ((ret = mdm_addObjectInstance(pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_BRIDGE_PORT failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &brPortObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get brPortObj object, ret=%d", ret);
      return ret;
   }

   /* set the params in the newly created object */
   brPortObj->enable = TRUE;
   brPortObj->managementPort = isManagementPort;
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->name, ifname, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->lowerLayers, lowerLayers, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(brPortObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &brPortObj, &pathDesc->iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set brPortObj. ret=%d", ret);
      /* mdm_setObject does not steal obj on error, so we must free it */
      mdm_freeObject((void **) &brPortObj);
      return ret;
   }


   /* also need to have the mgmt port point to this bridge port via LowerLayers */
   if (!isManagementPort)
   {
      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(pathDesc, &brPortFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

      if ((ret = addFullPathToMgmtPortLowerLayers(&brPathDesc->iidStack, brPortFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("addFullPathToMgmtPortLowerLayers returns error. ret=%d", ret);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
   }


   /* need to manually update the count when adding objects during mdm_init */
   if (ret == CMSRET_SUCCESS)
   {
      Dev2BridgeObject *brObj=NULL;

      if ((ret = mdm_getObject(MDMOID_DEV2_BRIDGE, &brPathDesc->iidStack, (void **) &brObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get brObj. ret=%d", ret);
         return ret;
      }

      brObj->portNumberOfEntries++;
      ret = mdm_setObject((void **) &brObj, &brPathDesc->iidStack,  FALSE);
      mdm_freeObject((void **)&brObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set brObj. ret=%d", ret);
      }
   }

   return ret;
}


CmsRet mdmInit_addFullPathToBridge_dev2(const char *bridgeIfName,
                                        const char *lowerLayerIfName,
                                        const char *lowerLayerFullPath)
{
   Dev2BridgeObject *brObj=NULL;
   MdmPathDescriptor brPathDesc=EMPTY_PATH_DESCRIPTOR;
   MdmPathDescriptor brPortPathDesc=EMPTY_PATH_DESCRIPTOR;
   UBOOL8 found=FALSE;
   CmsRet ret;

   /* First find the specified bridgeIntfName */
   brPathDesc.oid = MDMOID_DEV2_BRIDGE;
   while (!found &&
          mdm_getNextObject(brPathDesc.oid, &brPathDesc.iidStack, (void **) &brObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, bridgeIfName))
      {
         found = TRUE;
      }
      mdm_freeObject((void **) &brObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find bridge %s!", bridgeIfName);
      return CMSRET_INTERNAL_ERROR;
   }

   /* add bridge port under the bridge */
   ret = mdmInit_addBridgePortObject_dev2(&brPathDesc,
                                          lowerLayerIfName, FALSE,
                                          lowerLayerFullPath,
                                          &brPortPathDesc);
   return ret;
}


CmsRet mdm_addFullPathToDefaultBridge_dev2(const char *ifname, const char *fullpath)
{
   MdmPathDescriptor brPathDesc;
   MdmPathDescriptor brPortPathDesc;
   char *brFullPath=NULL;
   CmsRet ret;

   cmsLog_debug("adding %s %s", ifname, fullpath);

   INIT_PATH_DESCRIPTOR(&brPathDesc);
   INIT_PATH_DESCRIPTOR(&brPortPathDesc);

   /* XXX assume default LAN port is br0.  Later, use qdmIpIntf_getFirstAvailLanIntfNameLocked_dev2
    *
    */
   ret = qdmIntf_intfnameToFullPathLocked("br0", FALSE, &brFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get fullpath to br0");
      return ret;
   }

   ret = cmsMdm_fullPathToPathDescriptor(brFullPath, &brPathDesc);
   CMSMEM_FREE_BUF_AND_NULL_PTR(brFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not converted fullpath %s", brFullPath);
      return ret;
   }

   ret = mdmInit_addBridgePortObject_dev2(&brPathDesc,
                                          ifname, FALSE, fullpath,
                                          &brPortPathDesc);

   return ret;
}


#endif  /* DMP_DEVICE2_BASELINE_1 */
