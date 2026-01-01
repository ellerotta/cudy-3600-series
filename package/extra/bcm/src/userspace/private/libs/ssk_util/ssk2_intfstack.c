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
* :>
*/

#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "cms_core.h"
#include "ssk_util.h"
#ifdef DMP_DEVICE2_DSL_1
#include "devctl_xtm.h"
#endif

/*!\file ssk2_intfstack.c
 *
 * This file contains functions which manage the TR181 interface stack.
 * This code is needed in Hybrid TR98+TR181 mode and PURE181 mode.
 */



/* local functions */
static UBOOL8 findIntfStackEntryLocked(const char *higherLayer,
                                       const char *lowerLayer,
                                       Dev2InterfaceStackObject **obj,
                                       InstanceIdStack *iidStack);
static CmsRet addIntfStackEntryLocked(const char *higherLayer, const char *lowerLayer);
static void deleteFullPathFromLowerLayersParam(const char *deletedFullPath,
                                               const char *targetFullPath);
static CmsRet getAliasFromFullPathLocked(const char *fullPath, char *alias);
static CmsRet getAliasFromPathDescLocked(const MdmPathDescriptor *pathDesc, char *alias);

static UBOOL8 isBridgeMgmtPort(const char *higherLayerFullPath);
static UBOOL8 isAnyLowerLayerIntfUpLocked(const char *higherLayerFullPath,
                                       const char *excludeLowerLayerFullPath);
static void writeIntfStackToConfig(void);

static UBOOL8 isOnSameStackLocked(const char *higherLayerFullPath,
                                  const char *targetFullPath);
static void setStatusOnPppFirst(const Dev2InterfaceStackObject *pppIntfStackObj);
static UBOOL8 findIntfStackObjOverFullPathLocked(MdmObjectId topOid,
                                  const char *targetFullPath,
                                  Dev2InterfaceStackObject **retIntfStackObj);


#if defined(DMP_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
static UBOOL8 isTr98WanIpPppObj(const char *fullpath);
static CmsRet getTr98L2StatusFromL3FullPath(const char *fullPath,
                                     char *statusBuf, UINT32 statusBufLen);
#endif


// Tricky way to tell if we are in various BDK special cases without
// using ifdefs
#define IS_BDK_DSL_SSK (_myEid == EID_DSL_SSK)
#define IS_BDK_SYSMGMT (!strcmp(_myCompName, BDK_COMP_SYSMGMT))

#define IS_DEV2_XTM_LINK_OID(oid) ((oid == MDMOID_DEV2_PTM_LINK) || (oid == MDMOID_DEV2_ATM_LINK))
#define IS_DEV2_ETH_LINK_OID(oid) (oid == MDMOID_DEV2_ETHERNET_LINK) 

void processIntfStackLowerLayersChangedMsg(void *appMsgHandle, const CmsMsgHeader *msg)
{
   IntfStackLowerLayersChangedMsgBody *llChangedMsg = (IntfStackLowerLayersChangedMsgBody *) (msg+1);
   char *deltaBuf = llChangedMsg->deltaLowerLayers;
   UINT32 end;
   UINT32 idx=0;
   UBOOL8 addOp=FALSE;
   char lowerLayerFullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN+1];
   UINT32 j;
   MdmPathDescriptor pathDesc;
   char *higherLayerFullPath=NULL;
   CmsRet ret;
   char statusBuf[BUFLEN_64] = {0};

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = llChangedMsg->oid;
   pathDesc.iidStack = llChangedMsg->iidStack;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &higherLayerFullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      return;
   }

   end=strlen(llChangedMsg->deltaLowerLayers);

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      CMSMEM_FREE_BUF_AND_NULL_PTR(higherLayerFullPath);
      return;
   }

   while (idx < end)
   {
      if (deltaBuf[idx] == '+')
      {
         addOp = TRUE;
      }
      else if (deltaBuf[idx] == '-')
      {
         addOp = FALSE;
      }
      else
      {
         cmsLog_error("unrecogized op char %c at idx %d, stop parsing this msg!", deltaBuf[idx], idx);
         break;
      }
      idx++;
      memset(lowerLayerFullPathBuf, 0, sizeof(lowerLayerFullPathBuf));
      j=0;
      while (deltaBuf[idx] != ',' && idx < end)
      {
         lowerLayerFullPathBuf[j++] = deltaBuf[idx++];
      }
      if (addOp)
      {
         if ((ret = addIntfStackEntryLocked(higherLayerFullPath, lowerLayerFullPathBuf)) != CMSRET_SUCCESS)
         {
            cmsLog_error("addIntfStackEntry failed, ret=%d", ret);
            /* just complain, don't exit the function */
         }
         else
         {
            CmsRet r2;
            /* after successful add of new intf stack entry, we need
             * to propagate up the status from the lower layer.
             */
#if defined(DMP_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
            if (isTr98WanIpPppObj(lowerLayerFullPathBuf))
            {
               /* In Hybrid IPv6 case, we will have a lowerLayer pointing to
                * WanIpConn and WanPppConn objects, but what we really
                * want is the status of the underlying Layer 2 link.  So
                * we cannot use qdmIntf_getStatusFromFullPath.  Instead,
                * call special function to get the layer 2 status.
                * XXX fix Hybrid code to correctly point to the layer 2 intf?
                */
               r2 = getTr98L2StatusFromL3FullPath(lowerLayerFullPathBuf,
                                       statusBuf, sizeof(statusBuf));
            }
            else
#endif
            {
               r2 = qdmIntf_getStatusFromFullPathLocked_dev2(lowerLayerFullPathBuf,
                                             statusBuf, sizeof(statusBuf));
            }

            if (r2 != CMSRET_SUCCESS)
            {
               /* may try to get Status on a not-created/deleted object, so ignore CMSRET_INVALID_PARAM_NAME */
               if (r2 != CMSRET_INVALID_PARAM_NAME)
               {
                  cmsLog_error("getStatusFromFullPath failed for %s, ret=%d",
                               lowerLayerFullPathBuf, r2);
                  /* complain but don't exit */
               }
            }
            else
            {
               intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                            lowerLayerFullPathBuf, statusBuf);
            }
         }
      }
      else
      {
         /*
          * addOp == FALSE, so a fullpath has been removed from the LowerLayers
          * param of the higher layer obj.  Note this is NOT the same as a
          * delete of an object.
          * Propagate a "fake" status of LOWERLAYERDOWN to the higher layer
          * obj.  The status is "fake" because it is not the real status of
          * the lower layer obj.  We only do this because the higher layer
          * obj can no longer derive its status from this lower layer obj.
          * Do not propagate this fake status to any other higher layer objects.
          */
         Dev2InterfaceStackObject *intfStackObj=NULL;
         InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

         cmsLog_debug("delete higher=%s lower=%s",
                       higherLayerFullPath, lowerLayerFullPathBuf);

         /*
          * we might get this message after a whole object delete has been
          * processed.  In this case, propagate status is already done and
          * intfStack entry is already deleted.  So if we don't find an
          * intfStack entry, no big deal.
          */
         if (findIntfStackEntryLocked(higherLayerFullPath, lowerLayerFullPathBuf,
                                      &intfStackObj, &iidStack))
         {
#ifdef DMP_DEVICE2_BONDEDDSL_1
            /* for bonding group, its status doesn't just depend on the
             * lowerLayers's (DSL channel) status of UP/DOWN, it also depends on what
             * the CO and CPE xDSL dynamically trained mode is.  So, the removal of
             * channel as lower layer does not necessarily mean the bonding group
             * higher up should always change LOWER_LAYER_DOWN too.
             * So, propagate the real channel's status up instead; if the channel is UP,
             * the bonding group should not be brought down.
             */
            if (strstr(deltaBuf,"BondingGroup") != NULL)
            {
               /* get the real status from the channel */
               ret = qdmIntf_getStatusFromFullPathLocked_dev2(lowerLayerFullPathBuf,
                                                              statusBuf, sizeof(statusBuf));
               if (ret != CMSRET_SUCCESS)
               {
                  intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                        lowerLayerFullPathBuf,
                                                        MDMVS_LOWERLAYERDOWN);
               }
               else
               {
                  intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                        lowerLayerFullPathBuf,
                                                        statusBuf);
               }
            }
            else
#endif  /* DMP_DEVICE2_BONDEDDSL_1 */
            {
               intfStack_propagateStatusOnSingleEntryLocked(appMsgHandle,
                                                         intfStackObj,
                                                         MDMVS_LOWERLAYERDOWN);
            }

            cmsObj_free((void **) &intfStackObj);

            /*
             * Now that status propagation is done, delete the intfStack entry
             */
            ret = cmsObj_deleteInstance(MDMOID_DEV2_INTERFACE_STACK, &iidStack);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("delete instance %s failed, ret=%d",
                            cmsMdm_dumpIidStack(&iidStack), ret);
            }
         }
      }

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("add/del op failed, ret=%d", ret);
      }
      idx++;
   }

   cmsLck_releaseLock();

   writeIntfStackToConfig();

   CMSMEM_FREE_BUF_AND_NULL_PTR(higherLayerFullPath);

   return;
}


void processIntfStackObjectDeletedMsg(void *appMsgHandle, const CmsMsgHeader *msg)
{
   IntfStackObjectDeletedMsgBody *objDeletedMsg = (IntfStackObjectDeletedMsgBody *) (msg+1);
   MdmPathDescriptor pathDesc;
   char *deletedFullPath=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj=NULL;
   UINT32 lowerEntriesDeleted=0;
   UINT32 higherEntriesDeleted=0;
   CmsRet ret;

   cmsLog_debug("Entered:oid=%d iidStack=%s",
          objDeletedMsg->oid, cmsMdm_dumpIidStack(&objDeletedMsg->iidStack));

   /* form the fullpath of the deleted object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = objDeletedMsg->oid;
   pathDesc.iidStack = objDeletedMsg->iidStack;

   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &deletedFullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      return;
   }
   cmsLog_debug("deletedFullpath=%s", deletedFullPath);
   if ((IS_BDK_SYSMGMT) && (pathDesc.oid == MDMOID_DEV2_IP_INTERFACE))
   {
      // UNPUBLISH this interface in sys_directory.
      sendIpInterfaceEventMsgEx(appMsgHandle, deletedFullPath, FALSE, NULL);
   }

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      CMSMEM_FREE_BUF_AND_NULL_PTR(deletedFullPath);
      return;
   }

   {
      // See if we need to do some pppxtm specific actions.
      UINT32 ptmFullpathLen=strlen(DEV2_PTM_LINK_FULLPATH);
      UINT32 atmFullpathLen=strlen(DEV2_ATM_LINK_FULLPATH);
      if (!cmsUtl_strncmp(deletedFullPath, DEV2_PTM_LINK_FULLPATH, ptmFullpathLen) ||
          !cmsUtl_strncmp(deletedFullPath, DEV2_ATM_LINK_FULLPATH, atmFullpathLen))
      {
         pppXtm_delEntry(deletedFullPath);
         if (IS_BDK_DSL_SSK)
         {
            // UNSUBSCRIBE from pppxtm lock for this interface in sys_directory.
            sendSubscribePppXtmLockMsg(appMsgHandle, deletedFullPath, FALSE);
            // UNPUBLISH this interface in sys_directory.
            sendLayer2InterfaceEventMsgEx(appMsgHandle, MDMVS_DOWN, &pathDesc,
                                          FALSE, NULL);
         }
      }
   }

   if (IS_BDK_SYSMGMT)
   {
      // If PPP interface is deleted while the connection is still up, a DOWN
      // status is not propagated on the interface stack, so we can only
      // detect it here.  Unlock the PPPXTM link.
      UINT32 pppIntfFullpathLen=strlen(DEV2_PPP_INTERFACE_FULLPATH);
      if (!cmsUtl_strncmp(deletedFullPath, DEV2_PPP_INTERFACE_FULLPATH,
                          pppIntfFullpathLen))
      {
         char *xtmLinkFullpath=NULL;
         if (isOverXtm(deletedFullPath, &xtmLinkFullpath))
         {
            sendPppXtmLockMsg(appMsgHandle, xtmLinkFullpath, "");
            CMSMEM_FREE_BUF_AND_NULL_PTR(xtmLinkFullpath);
         }
      }
   }

   /*
    * First, find all entries where lowerLayer == deletedFullPath and
    * update the lowerLayers param on the upper object.
    */
   while (findIntfStackEntryLocked(NULL, deletedFullPath, &intfStackObj, &iidStack))
   {
      /* remove the deletedFullPath from the LowerLayers param on the
       * higher object.  This will(may?) generate LowerLayersChanged messages
       * but ssk will have to ignore these.
       */
      deleteFullPathFromLowerLayersParam(deletedFullPath, intfStackObj->higherLayer);
      cmsObj_free((void **) &intfStackObj);

      /*
       * What status should this object have if the LowerLayers param is NULL?
       * Keep things simple and use LowerLayerDown in this case.
       * Propagate LowerLayerDown status to the higher layer object.  Must do
       * this before deleting the intf stack entry because propagateStatus
       * matches against the lowerLayer fullpath.
       */
      cmsLog_debug("propagate LowerLayerDown from %s", deletedFullPath);
      intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                     deletedFullPath, MDMVS_LOWERLAYERDOWN);

      ret = cmsObj_deleteInstance(MDMOID_DEV2_INTERFACE_STACK, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("delete instance %s failed, ret=%d",
                      cmsMdm_dumpIidStack(&iidStack), ret);
      }
      else
      {
         lowerEntriesDeleted++;
      }
   }

   /*
    * Final cleanup: delete all entries where higherLayer == deletedFullPath.
    * Don't have to worry about propagate status in this step.
    */
   while (findIntfStackEntryLocked(deletedFullPath, NULL, NULL, &iidStack))
   {
      ret = cmsObj_deleteInstance(MDMOID_DEV2_INTERFACE_STACK, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("delete instance %s failed, ret=%d",
                      cmsMdm_dumpIidStack(&iidStack), ret);
      }
      else
      {
         higherEntriesDeleted++;
      }
   }

   cmsLog_debug("LowerDeleted=%d higherDeleted=%d",
                lowerEntriesDeleted, higherEntriesDeleted);

   cmsLck_releaseLock();

   if (lowerEntriesDeleted || higherEntriesDeleted)
   {
      writeIntfStackToConfig();
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(deletedFullPath);

   return;
}


/** Find an interface stack entry.
 *
 * @param (IN) Higher Layer fullpath to match, or NULL for wildcard
 * @param (IN) Lower Layer fullpath to match, or NULL for wildcard
 * @param (OUT) if ptr is not NULL, will point to found IntfStackObj.
 *              Caller is responsible for freeing it.
 * @param (OUT) if ptr is not NULL, will contain iidStack of found entry
 *
 * @return TRUE if an entry is found
 */
static UBOOL8 findIntfStackEntryLocked(const char *higherLayer,
                                       const char *lowerLayer,
                                       Dev2InterfaceStackObject **obj,
                                       InstanceIdStack *iidStack)
{
   Dev2InterfaceStackObject *intfStackObj=NULL;
   InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   CmsRet ret;

   while (!found &&
         (ret = cmsObj_getNext(MDMOID_DEV2_INTERFACE_STACK,
                               &localIidStack,
                               (void **)&intfStackObj)) == CMSRET_SUCCESS)
   {

      if (((higherLayer == NULL) ||
           (!cmsUtl_strcmp(intfStackObj->higherLayer, higherLayer))) &&
          ((lowerLayer == NULL) ||
           (!cmsUtl_strcmp(intfStackObj->lowerLayer, lowerLayer))))
      {
         found = TRUE;
         if (iidStack)
         {
            *iidStack = localIidStack;
         }
         if (obj != NULL)
         {
            *obj = intfStackObj;
            /* return immediately so we do not free obj in this function.
             * caller has it now and he is responsible for freeing.
             */
            return found;
         }
      }
      cmsObj_free((void **) &intfStackObj);
   }

   return found;
}


CmsRet addIntfStackEntryLocked(const char *higherLayer, const char *lowerLayer)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj=NULL;
   CmsRet ret;

   cmsLog_debug("higher=%s lower=%s", higherLayer, lowerLayer);

   if (findIntfStackEntryLocked(higherLayer, lowerLayer, NULL, NULL))
   {
      /*
       * The interface stack entries are always saved to the config file.
       * So when MDM initializes during bootup, we will get LowerLayersChanged
       * messages, but the entries are already in the MDM.
       */
      cmsLog_debug("ignore dup intfStack entry, higher=%s lower=%s",
                   higherLayer, lowerLayer);
      return CMSRET_SUCCESS;
   }


   ret = cmsObj_addInstance(MDMOID_DEV2_INTERFACE_STACK, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("addInstance failed, ret=%d", ret);
      return ret;
   }

   ret = cmsObj_get(MDMOID_DEV2_INTERFACE_STACK, &iidStack, 0, (void **)&intfStackObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get new IntfStack instance, ret=%d", ret);
      return ret;
   }

   CMSMEM_REPLACE_STRING(intfStackObj->higherLayer, higherLayer);
   CMSMEM_REPLACE_STRING(intfStackObj->lowerLayer, lowerLayer);

   /* also fill in the aliases when this entry is created */
   {
      char aliasBuf[MDM_ALIAS_BUFLEN]={0};
      memset(aliasBuf, 0, sizeof(aliasBuf));
      if ((CMSRET_SUCCESS == getAliasFromFullPathLocked(higherLayer, aliasBuf)) &&
            (cmsUtl_strlen(aliasBuf) > 0))
      {
         CMSMEM_REPLACE_STRING(intfStackObj->higherAlias, aliasBuf);
      }
      memset(aliasBuf, 0, sizeof(aliasBuf));
      if ((CMSRET_SUCCESS == getAliasFromFullPathLocked(lowerLayer, aliasBuf)) &&
            (cmsUtl_strlen(aliasBuf) > 0))
      {
         CMSMEM_REPLACE_STRING(intfStackObj->lowerAlias, aliasBuf);
      }
   }


   ret = cmsObj_set((void *) intfStackObj, &iidStack);
   cmsObj_free((void **) &intfStackObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of IntfStackObj failed, ret=%d", ret);
   }

   return ret;
}


void deleteFullPathFromLowerLayersParam(const char *deletedFullPath,
                                        const char *targetFullPath)
{
   MdmPathDescriptor pathDesc;
   PhlSetParamValue_t paramValue;
   char lowerLayersBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};
   CmsRet ret;

   cmsLog_debug("delete %s from %s", deletedFullPath, targetFullPath);

   ret = qdmIntf_getLowerLayersFromFullPathLocked_dev2(targetFullPath,
                                 lowerLayersBuf, sizeof(lowerLayersBuf));
   if (ret != CMSRET_SUCCESS)
   {
      /*
       * If delete from WebUI, by the time we get here, the higher layer
       * object may be already deleted, so there is nothing to update.
       */
      cmsLog_debug("could not get LowerLayers param for %s, ret=%d",
                   targetFullPath, ret);
      return;
   }

   /* remove the deletedFullPath from lowerLayers param buf */
   cmsUtl_deleteFullPathFromCSL(deletedFullPath, lowerLayersBuf);

   /* set the updated LowerLayers param value */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(targetFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                   targetFullPath, ret);
      return;
   }

   memset(&paramValue, 0, sizeof(paramValue));
   paramValue.pathDesc.oid = pathDesc.oid;
   paramValue.pathDesc.iidStack = pathDesc.iidStack;
   sprintf(paramValue.pathDesc.paramName, "LowerLayers");
   paramValue.pParamType = "string";
   paramValue.pValue = lowerLayersBuf;
   paramValue.status = CMSRET_SUCCESS;

   ret = cmsPhl_setParameterValues(&paramValue, 1);
   if (ret != CMSRET_SUCCESS || paramValue.status != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsPhl_getParamValue error: %d %d", ret, paramValue.status);
   }
}


void processIntfStackAliasChangedMsg(const CmsMsgHeader *msg)
{
   IntfStackAliasChangedMsgBody *aliasChangedMsg = (IntfStackAliasChangedMsgBody *) (msg+1);
   char aliasBuf[MDM_ALIAS_BUFLEN]={0};
   MdmPathDescriptor pathDesc;
   char *fullPath=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj=NULL;
   UBOOL8 doSet;
   CmsRet ret;

   /* gather info needed for this operation */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = aliasChangedMsg->oid;
   pathDesc.iidStack = aliasChangedMsg->iidStack;

   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      return;
   }


   /*
    * loop through all intfstack table entries, find matching path, and
    * update the alias.
    */
   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return;
   }

   ret = getAliasFromPathDescLocked(&pathDesc, aliasBuf);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get alias, ignore msg");
      cmsLck_releaseLock();
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return;
   }

   cmsLog_debug("update %s alias to %s", fullPath, aliasBuf);

   while ((ret = cmsObj_getNext(MDMOID_DEV2_INTERFACE_STACK, &iidStack, (void **)&intfStackObj)) == CMSRET_SUCCESS)
   {
      doSet = FALSE;
      if (!cmsUtl_strcmp(intfStackObj->higherLayer, fullPath))
      {
         CMSMEM_REPLACE_STRING(intfStackObj->higherAlias, aliasBuf);
         doSet = TRUE;
      }
      if (!cmsUtl_strcmp(intfStackObj->lowerLayer, fullPath))
      {
         CMSMEM_REPLACE_STRING(intfStackObj->lowerAlias, aliasBuf);
         doSet = TRUE;
      }
      if (doSet)
      {
         ret = cmsObj_set((void *) intfStackObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of IntfStackObj failed, ret=%d", ret);
         }
      }
      cmsObj_free((void **) &intfStackObj);
   }

   cmsLck_releaseLock();

   writeIntfStackToConfig();

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   return;
}


void processIntfStackStaticAddressConfigdMsg(void *appMsgHandle, const CmsMsgHeader *msg)
{
   IntfStackStaticAddressConfig *staticAddrMsg = (IntfStackStaticAddressConfig *) (msg+1);
   char *ipIntfFullPath=NULL;
   MdmPathDescriptor ipIntfPathDesc=EMPTY_PATH_DESCRIPTOR;
   CmsRet ret;

   cmsLog_debug("Enter: ifName=%s isIPv4=%d isAdd=%d isMod=%d isDel=%d",
             staticAddrMsg->ifName, staticAddrMsg->isIPv4,
             staticAddrMsg->isAdd, staticAddrMsg->isMod, staticAddrMsg->isDel);


   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   /*
    * Get all the info we need before really starting to do the work.
    */

   if(qdmIntf_intfnameToFullPathLocked_dev2(staticAddrMsg->ifName, FALSE, &ipIntfFullPath)!=CMSRET_SUCCESS)
   {
      cmsLog_error("cannot get ipIntfFullPath of ifName<%s>", staticAddrMsg->ifName);
      cmsLck_releaseLock();
      return;
   }

   cmsLog_debug("%s ==> %s", staticAddrMsg->ifName, ipIntfFullPath);

   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &ipIntfPathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                   ipIntfFullPath, ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
      cmsLck_releaseLock();
      return;
   }


   /*
    * If delete or modify, and if service state is in SERVICEUP,
    * bring the service status down to SERVICESTARTING.  Also update the
    * IP.Interface.Status (if necessary).
    * If service state is not SERVICEDOWN, that means lower layer link is
    * down.  Don't need to do anything here.  When link comes up, state
    * machine will be updated via propagateStatus.
    */
   if (staticAddrMsg->isDel || staticAddrMsg->isMod)
   {
      if (staticAddrMsg->isIPv4)
      {
         char ipv4ServiceStatusBuf[BUFLEN_64]={0};

         getIpv4ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                         ipv4ServiceStatusBuf,
                                         sizeof(ipv4ServiceStatusBuf));
         cmsLog_debug("current IPv4ServiceStatus=%s", ipv4ServiceStatusBuf);

         if (!strcmp(ipv4ServiceStatusBuf, MDMVS_SERVICEUP))
         {
            /* update IP.Interface.Status first based on new IPv4ServiceStatus */
            intfStack_updateIpIntfStatusLocked(&ipIntfPathDesc.iidStack,
                                               MDMVS_SERVICESTARTING,
                                               NULL);

            setIpv4ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                            MDMVS_SERVICESTARTING);
         }
      }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      else
      {
         char ipv6ServiceStatusBuf[BUFLEN_64]={0};

         getIpv6ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                         ipv6ServiceStatusBuf,
                                         sizeof(ipv6ServiceStatusBuf));
         cmsLog_debug("current IPv6ServiceStatus=%s", ipv6ServiceStatusBuf);

         if (!strcmp(ipv6ServiceStatusBuf, MDMVS_SERVICEUP))
         {
            /* update IP.Interface.Status first based on new IPv6ServiceStatus */
            intfStack_updateIpIntfStatusLocked(&ipIntfPathDesc.iidStack,
                                               NULL,
                                               MDMVS_SERVICESTARTING);

            setIpv6ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                            MDMVS_SERVICESTARTING);
         }
      }
#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
   }


   /*
    * If add or modify, and if service state is in SERVICESTARTING,
    * we can bring the service state up to SERVICEUP.  Also update
    * IP.Interface.Status (if necessary).
    * If service state is not SERVICESTARTING, that means lower layer link is
    * down.  Don't need to do anything here.  When link comes up, state
    * machine will be updated via propagateStatus.
    */
   if (staticAddrMsg->isAdd || staticAddrMsg->isMod)
   {
      if (staticAddrMsg->isIPv4)
      {
         char ipv4ServiceStatusBuf[BUFLEN_64]={0};

         getIpv4ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                         ipv4ServiceStatusBuf,
                                         sizeof(ipv4ServiceStatusBuf));
         cmsLog_debug("current IPv4ServiceStatus=%s", ipv4ServiceStatusBuf);

         if (!strcmp(ipv4ServiceStatusBuf, MDMVS_SERVICESTARTING))
         {
            /* update IP.Interface.Status first based on new IPv4ServiceStatus */
            intfStack_updateIpIntfStatusLocked(&ipIntfPathDesc.iidStack,
                                               MDMVS_SERVICEUP,
                                               NULL);

            setIpv4ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                            MDMVS_SERVICEUP);
         }
      }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      else
      {
         char ipv6ServiceStatusBuf[BUFLEN_64]={0};

         getIpv6ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                         ipv6ServiceStatusBuf,
                                         sizeof(ipv6ServiceStatusBuf));
         cmsLog_debug("current IPv6ServiceStatus=%s", ipv6ServiceStatusBuf);

         if (!strcmp(ipv6ServiceStatusBuf, MDMVS_SERVICESTARTING))
         {
            /* update IP.Interface.Status first based on new IPv6ServiceStatus */
            intfStack_updateIpIntfStatusLocked(&ipIntfPathDesc.iidStack,
                                               NULL,
                                               MDMVS_SERVICEUP);

            setIpv6ServiceStatusByIidLocked(&ipIntfPathDesc.iidStack,
                                            MDMVS_SERVICEUP);
         }
      }
#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
   }

   // Hmm, static IP address config does not go through the regular intf stack
   // processing path, so we have to update sys_directory again here.
   sendIpInterfaceEventMsg(appMsgHandle, ipIntfFullPath);

   CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   cmsLck_releaseLock();

   return;
}


void processIntfStackPropagateMsg(void *appMsgHandle, const CmsMsgHeader *msg)
{
   IntfStackPropagateStaus *propagaeStatusMsg = (IntfStackPropagateStaus *) (msg+1);
   char statusBuf[BUFLEN_64] = {0};
   CmsRet ret;

   cmsLog_notice("Entered: lowerLayerFullPath=%s", propagaeStatusMsg->ipLowerLayerFullPath);

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   if ((ret = qdmIntf_getStatusFromFullPathLocked_dev2(propagaeStatusMsg->ipLowerLayerFullPath,
                                                       statusBuf,
                                                       sizeof(statusBuf))) != CMSRET_SUCCESS)
   {
      cmsLog_error("getStatusFromFullPath failed for %s, ret=%d", propagaeStatusMsg->ipLowerLayerFullPath, ret);
      /* complain but don't exit */
   }
   else
   {
      intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                      propagaeStatusMsg->ipLowerLayerFullPath,
                                      statusBuf);
   }

   cmsLck_releaseLock();
}

void processIntfStackPropagateMsgEx(void *appMsgHandle, const CmsMsgHeader *msg)
{
   IntfStackPropagateStatusExMsgBody *msgBody = (IntfStackPropagateStatusExMsgBody *)(msg+1);
   CmsRet ret;

   cmsLog_notice("Entered: flags=0x%x higherLayerFullPath=%s lowerLayerFullPath=%s data=%s",
                msgBody->flags,
                msgBody->higherLayerFullPath, msgBody->lowerLayerFullPath,
                msgBody->data);

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   if (msgBody->flags & PROPAGATE_FLAG_LOWERLAYER_STATUS)
   {
      // this is the only flag we support right now, but settiing up the
      // framework for future expansion.
      Dev2InterfaceStackObject *intfStackObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      UBOOL8 found = FALSE;

      // Find the exact matching intf stack entry
      while (!found &&
             (cmsObj_getNextFlags(MDMOID_DEV2_INTERFACE_STACK, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&intfStackObj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(intfStackObj->higherLayer, msgBody->higherLayerFullPath) &&
             !cmsUtl_strcmp(intfStackObj->lowerLayer, msgBody->lowerLayerFullPath))
         {
            intfStack_propagateStatusOnSingleEntryLocked(appMsgHandle,
                                                intfStackObj, msgBody->data);
            found = TRUE;
         }

         cmsObj_free((void **) &intfStackObj);
      }
      if (!found)
      {
         // This could happen if we are in the middle of creating the higher
         // layer interface (ptm).  InterfaceStack entry containing the higher
         // layer does not exist yet.  No need to propagate.
         cmsLog_debug("Could not find intfStack entry: higher=%s lower=%s",
                      msgBody->higherLayerFullPath,
                      msgBody->lowerLayerFullPath);
      }
   }
   else
   {
      cmsLog_error("Unsupported flags 0x%x", msgBody->flags);
   }

   cmsLck_releaseLock();

   return;
}


void intfStack_updateIpIntfStatusLocked(const InstanceIdStack *ipIntfIidStack,
                                        const char *newIpv4ServiceStatus,
                                        const char *newIpv6ServiceStatus)
{
   char ipv4ServiceStatusBuf[BUFLEN_64]={0};
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   char ipv6ServiceStatusBuf[BUFLEN_64]={0};
#endif
   char newIpIntfStatusBuf[BUFLEN_64]={0};

   cmsLog_debug("Entered: iidStack=%s newIpv4ServiceStatus=%s newIPv6ServiceStatus=%s",
                cmsMdm_dumpIidStack(ipIntfIidStack),
                newIpv4ServiceStatus,
                newIpv6ServiceStatus);

   if (newIpv4ServiceStatus)
   {
      strncpy(ipv4ServiceStatusBuf, newIpv4ServiceStatus, sizeof(ipv4ServiceStatusBuf)-1);
   }
   else
   {
      getIpv4ServiceStatusByIidLocked(ipIntfIidStack,
                                      ipv4ServiceStatusBuf,
                                      sizeof(ipv4ServiceStatusBuf));
      cmsLog_debug("current IPv4ServiceStatus=%s", ipv4ServiceStatusBuf);
   }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (newIpv6ServiceStatus)
   {
      strncpy(ipv6ServiceStatusBuf, newIpv6ServiceStatus, sizeof(ipv6ServiceStatusBuf)-1);
   }
   else
   {
      getIpv6ServiceStatusByIidLocked(ipIntfIidStack,
                                      ipv6ServiceStatusBuf,
                                      sizeof(ipv6ServiceStatusBuf));
      cmsLog_debug("current IPv6ServiceStatus=%s", ipv6ServiceStatusBuf);
   }
#endif


   /* if either IPv4 or IPv6 state machine is SERVICEUP, then
    * IP.Interface.Status=UP */
   if (!strcmp(ipv4ServiceStatusBuf, MDMVS_SERVICEUP))
   {
      strncpy(newIpIntfStatusBuf, MDMVS_UP, sizeof(newIpIntfStatusBuf)-1);
   }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   else if (!strcmp(ipv6ServiceStatusBuf, MDMVS_SERVICEUP))
   {
      strncpy(newIpIntfStatusBuf, MDMVS_UP, sizeof(newIpIntfStatusBuf)-1);
   }
#endif
   /* else if either IPv4 or IPv6 state machine is SERVICESTARTING, then
    * IP.Interface.Status=DORMANT */
   else if (!strcmp(ipv4ServiceStatusBuf, MDMVS_SERVICESTARTING))
   {
      strncpy(newIpIntfStatusBuf, MDMVS_DORMANT, sizeof(newIpIntfStatusBuf)-1);
   }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   else if (!strcmp(ipv6ServiceStatusBuf, MDMVS_SERVICESTARTING))
   {
      strncpy(newIpIntfStatusBuf, MDMVS_DORMANT, sizeof(newIpIntfStatusBuf)-1);
   }
#endif
   /* else, IP.Interface.Status must be LOWERLAYERDOWN */
   else
   {
      strncpy(newIpIntfStatusBuf, MDMVS_LOWERLAYERDOWN, sizeof(newIpIntfStatusBuf)-1);
   }

   /* now set to new status (its OK if it is the same as current status) */
   {
      MdmPathDescriptor ipIntfPathDesc=EMPTY_PATH_DESCRIPTOR;

      ipIntfPathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
      ipIntfPathDesc.iidStack = *ipIntfIidStack;

      intfStack_setStatusByPathDescLocked(&ipIntfPathDesc, newIpIntfStatusBuf);
   }

   return;
}


CmsRet getAliasFromFullPathLocked(const char *fullPath, char *alias)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", fullPath, ret);
      return ret;
   }

   return (getAliasFromPathDescLocked(&pathDesc, alias));
}


CmsRet getAliasFromPathDescLocked(const MdmPathDescriptor *pathDescIn, char *alias)
{
   MdmPathDescriptor pathDesc;
   PhlGetParamValue_t   *pParamValue = NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = pathDescIn->oid;
   pathDesc.iidStack = pathDescIn->iidStack;
   sprintf(pathDesc.paramName, "Alias");

   ret = cmsPhl_getParamValue(&pathDesc, &pParamValue);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("cmsPhl_getParamValue error: %d", ret);
   }
   else
   {
      if (cmsUtl_strlen(pParamValue->pValue) > 0)
      {
         sprintf(alias, "%s", pParamValue->pValue);
      }
      cmsPhl_freeGetParamValueBuf(pParamValue, 1);
   }

   return ret;
}


void intfStack_setStatusByFullPathLocked(const char *fullPath, const char *status)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("set %s to status %s", fullPath, status);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", fullPath, ret);
      return;
   }

   intfStack_setStatusByPathDescLocked(&pathDesc, status);
}


void intfStack_setStatusByPathDescLocked(const MdmPathDescriptor *pathDescIn, const char *status)
{
   PhlSetParamValue_t paramValue;
   CmsRet ret;

   cmsLog_debug("Entered: oid=%d iidStack=%s status=%s",
                pathDescIn->oid,
                cmsMdm_dumpIidStack(&pathDescIn->iidStack),
                status);

   memset(&paramValue, 0, sizeof(paramValue));
   paramValue.pathDesc.oid = pathDescIn->oid;
   paramValue.pathDesc.iidStack = pathDescIn->iidStack;
   paramValue.pParamType = "string";
   sprintf(paramValue.pathDesc.paramName, "Status");
   paramValue.pValue = (char *) status;
   paramValue.status = CMSRET_SUCCESS;

   ret = cmsPhl_setParameterValues(&paramValue, 1);
   if (ret != CMSRET_SUCCESS || paramValue.status != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsPhl_getParamValue error: %d %d", ret, paramValue.status);
   }

   return;
}

// Original intf stack code always assumed each obj has Enable=TRUE, however,
// in some SQA scenarios (in ATM and PTM), Enable may be set to FALSE.
// In the future, SQA may expand test cases to other objects, so this
// function can be adjusted to include those objects.
UBOOL8 intfStack_optionalCheckForEnabledByFullpath(const char *fullpath)
{
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   PhlGetParamValue_t   *pParamValue = NULL;
   CmsRet ret;
   UBOOL8 rv = TRUE;  // by default, assume obj is enabled

   ret = cmsMdm_fullPathToPathDescriptor(fullpath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", fullpath, ret);
      return rv;
   }

   // For now, we only do this enabled check on atm and ptm link objects, but
   // we could expand to other objects as needed.  By default, assume object
   // is enabled.
   if ((pathDesc.oid != MDMOID_DEV2_ATM_LINK) &&
       (pathDesc.oid != MDMOID_DEV2_PTM_LINK))
   {
      return rv;
   }

   // oid and iidStack has already been filled in, just add param name.
   sprintf(pathDesc.paramName, "Enable");

   ret = cmsPhl_getParamValue(&pathDesc, &pParamValue);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("cmsPhl_getParamValue error: %d", ret);
   }
   else
   {
      rv = (!cmsUtl_strcasecmp(pParamValue->pValue, "true") ||
            !cmsUtl_strcasecmp(pParamValue->pValue, "1"));
      cmsLog_debug("%s enabled=%d", fullpath, rv);
      cmsPhl_freeGetParamValueBuf(pParamValue, 1);
   }

   return rv;
}


void intfStack_propagateStatusByIidLocked(void *appMsgHandle, MdmObjectId oid, const InstanceIdStack *iidStack, const char *status)
{
   MdmPathDescriptor pathDesc;
   char *fullPath=NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = oid;
   pathDesc.iidStack = *iidStack;

   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      return;
   }

   intfStack_propagateStatusByFullPathLocked(appMsgHandle, fullPath, status);

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

}


void intfStack_propagateStatusByFullPathLocked(void *appMsgHandle,
                                               const char *lowerLayerFullPath,
                                               const char *newStatus)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj=NULL;

   if (IS_EMPTY_STRING(lowerLayerFullPath))
   {
      cmsLog_error("called with NULL or empty lowerLayerFullPath -- just return");
      return;
   }

   cmsLog_debug("Enter: lowerLayerFullPath=%s newStatus=%s",
                lowerLayerFullPath, newStatus);

   /*
    * Walk through the entire interface stack table looking for entries
    * that match our lowerLayerFullPath.
    */
   while (cmsObj_getNextFlags(MDMOID_DEV2_INTERFACE_STACK, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&intfStackObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(intfStackObj->lowerLayer, lowerLayerFullPath))
      {
         intfStack_propagateStatusOnSingleEntryLocked(appMsgHandle,
                                                      intfStackObj, newStatus);
      }

      cmsObj_free((void **) &intfStackObj);
   }

   cmsLog_debug("Exit: lowerLayerFullPath=%s newStatus=%s",
                lowerLayerFullPath, newStatus);

   return;
}

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1

static void intfStack_configureIPv6ChildPrefixDelegation(const InstanceIdStack *iidStackChild)
{
    Dev2IpInterfaceObject *ipIntfObjTmp = NULL;
    InstanceIdStack iidStackIpIntf = *iidStackChild;
    CmsRet ret;

    /* get iidstack of child intf obj for LAN */
    if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IP_INTERFACE,
                                 MDMOID_DEV2_IPV6_PREFIX,
                                 &iidStackIpIntf,
                                 (void **) &ipIntfObjTmp)) != CMSRET_SUCCESS)
    {
       cmsLog_error("could not get parent IP.Interface, ret=%d", ret);
    }
    else
    {
       char ipv6ServiceStatusBufTmp[BUFLEN_64]={0};

       getIpv6ServiceStatusByIidLocked(&iidStackIpIntf,
                                       ipv6ServiceStatusBufTmp,
                                sizeof(ipv6ServiceStatusBufTmp));

       cmsLog_debug("%s current IPv6ServiceStatus=%s",
                    ipIntfObjTmp->name, ipv6ServiceStatusBufTmp);
       cmsObj_free((void **)&ipIntfObjTmp);

       if (!strcmp(ipv6ServiceStatusBufTmp, MDMVS_SERVICEDOWN))
       {
          setIpv6ServiceStatusByIidLocked(&iidStackIpIntf,
                                          MDMVS_SERVICESTARTING);
       }
    }
}

static void intfStack_configureIPv6PrefixDelegation(const char *prefixFullPath)
{
    InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
    Dev2Ipv6PrefixObject *prefixObjChild = NULL;

    while (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX, &iidStackChild,
               OGF_NO_VALUE_UPDATE, (void **)&prefixObjChild) == CMSRET_SUCCESS)
    {
       if (!cmsUtl_strcmp(prefixObjChild->parentPrefix, prefixFullPath) &&
           !cmsUtl_strcmp(prefixObjChild->prefixStatus, MDMVS_INVALID))
       {
           intfStack_configureIPv6ChildPrefixDelegation(&iidStackChild);
       }

       cmsObj_free((void **)&prefixObjChild);
    }
}

static void intfStack_bringDownIPv6ChildPrefixDelegation(void *appMsgHandle, const InstanceIdStack *iidStackChild)
{
    Dev2IpInterfaceObject *ipIntfObjTmp = NULL;
    InstanceIdStack iidStackIpIntf = *iidStackChild;
    CmsRet ret;

    /* get iidstack of child intf obj for LAN */
    if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IP_INTERFACE,
                                 MDMOID_DEV2_IPV6_PREFIX,
                                 &iidStackIpIntf,
                                 (void **) &ipIntfObjTmp)) != CMSRET_SUCCESS)
    {
       cmsLog_error("could not get parent IP.Interface, ret=%d", ret);
    }
    else
    {
       if (!sskConn_hasStaticIpv6AddressLocked(&iidStackIpIntf))
       {
          Dev2InterfaceStackObject *intfStackObjTmp = NULL;

          if (findIntfStackEntryLocked(NULL, ipIntfObjTmp->lowerLayers, &intfStackObjTmp, NULL))
          {
             setIpv6ServiceStatusByIidLocked(&iidStackIpIntf, MDMVS_SERVICEDOWN);

             // Tell sys_directory about IP interface down
             sendIpInterfaceEventMsg(appMsgHandle, intfStackObjTmp->higherLayer);
             cmsObj_free((void **) &intfStackObjTmp);
          }
       }
       cmsObj_free((void **) &ipIntfObjTmp);
    }
}

static void intfStack_bringDownIPv6PrefixDelegation(void *appMsgHandle, const char *prefixFullPath)
{
    InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
    Dev2Ipv6PrefixObject *prefixObjChild = NULL;

    while (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX, &iidStackChild,
               OGF_NO_VALUE_UPDATE, (void **)&prefixObjChild) == CMSRET_SUCCESS)
    {
       if (!cmsUtl_strcmp(prefixObjChild->parentPrefix, prefixFullPath))
       {
           intfStack_bringDownIPv6ChildPrefixDelegation(appMsgHandle, &iidStackChild);
       }

       cmsObj_free((void **)&prefixObjChild);
    }
}
#endif


void intfStack_propagateStatusOnSingleEntryLocked(void *appMsgHandle,
                                                  const Dev2InterfaceStackObject *intfStackObj,
                                                  const char *newStatus)
{
   char higherLayerStatusBuf[BUFLEN_64]={0};
   MdmPathDescriptor higherLayerPathDesc;
   MdmPathDescriptor lowerLayerPathDesc;
   CmsRet ret;

   cmsLog_debug("Enter: %s->%s (%s)",
                intfStackObj->lowerLayer, intfStackObj->higherLayer,
                newStatus);


   ret = qdmIntf_getStatusFromFullPathLocked_dev2(intfStackObj->higherLayer,
                                      higherLayerStatusBuf,
                                      sizeof(higherLayerStatusBuf));
   if (ret != CMSRET_SUCCESS)
   {
      /*
       * When we delete a whole WAN service from WebUI, by the time
       * we get here, the higher layer object is already deleted.
       * Do not complain loudly about the error, just return.  There is
       * nothing to be done.
       */
      cmsLog_debug("qdmIntf_getStatusFromFullPath for %s failed, ret=%d",
                   intfStackObj->higherLayer, ret);
      return;
   }

   /*
    * Convert higherLayerFullPath to pathDesc to check for special cases
    */
   INIT_PATH_DESCRIPTOR(&higherLayerPathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(intfStackObj->higherLayer, &higherLayerPathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                    intfStackObj->higherLayer, ret);
      return;
   }

   INIT_PATH_DESCRIPTOR(&lowerLayerPathDesc);
   if (!IS_EMPTY_STRING(intfStackObj->lowerLayer))
   {
      cmsMdm_fullPathToPathDescriptor(intfStackObj->lowerLayer,
                                      &lowerLayerPathDesc);
   }


   /*
    * check for special case of non-management bridge port status propagation
    * up to the management bridge port.  It is possible that the list of
    * children (non-management) ports have changed, so trigger update of the
    * IP interface status.to sys_directory.  TODO: By putting the check here,
    * we will trigger an update to the IP interface even when link goes up or
    * down, but I can't find a more precise and reliable place to put this
    * check.
    */
   if (higherLayerPathDesc.oid == MDMOID_DEV2_BRIDGE_PORT)
   {
     if (isBridgeMgmtPort(intfStackObj->higherLayer))
     {
        cmsLog_debug("got update to %s from %s newStatus %s",
                      intfStackObj->higherLayer,
                      intfStackObj->lowerLayer, newStatus);
        sendIpInterfaceEvenMsgForBridge(appMsgHandle, intfStackObj->higherLayer);
     }
   }

   /*
    * check for special case of status propagation up from a management 
    * bridge port when it is added under the bridge. We want to trigger 
    * an update to the sys_directory.
    */
   if (lowerLayerPathDesc.oid == MDMOID_DEV2_BRIDGE_PORT)
   {
     if (isBridgeMgmtPort(intfStackObj->lowerLayer))
     {
        cmsLog_debug("got update to %s from %s newStatus %s",
                      intfStackObj->higherLayer,
                      intfStackObj->lowerLayer, newStatus);
        sendIpInterfaceEvenMsgForBridge(appMsgHandle, intfStackObj->lowerLayer);
     }
   }

   /*
    * If higher layer status is already equal to newStatus, then we
    * don't need to do process further.
    * Special case: for IP.Interface, for the UP status, even when
    * higher layer status is equal to newStatus continue processing
    * because the IP.Interface could be UP and IPv4ServiceStatus is
    * SERVICEUP, but the IPv6ServiceStatus is still in SERVICESTARTING.
    * So continue processing so IPv6ServiceStatus can get moved to
    * SERVICEUP.  (Actually, this is only needed for PPP, not for dhcpc,
    * but I don't bother to check for ppp specifically.)
    */
   if (((higherLayerPathDesc.oid != MDMOID_DEV2_IP_INTERFACE) &&
        !cmsUtl_strcmp(newStatus, higherLayerStatusBuf))            ||
       ((higherLayerPathDesc.oid == MDMOID_DEV2_IP_INTERFACE) &&
        !cmsUtl_strcmp(newStatus, higherLayerStatusBuf) &&
        cmsUtl_strcmp(newStatus, MDMVS_UP)))

   {
      cmsLog_debug("higherLayer %s is already %s -- "
                   "no more processing on this branch of the stack",
                   intfStackObj->higherLayer, newStatus);
      return;
   }

#ifdef DMP_DEVICE2_WIFIRADIO_1
   if (higherLayerPathDesc.oid == MDMOID_DEV2_WIFI_SSID)
   {
      /*
       * we don't want to propagate status from lowerlayer:WIFI.Radio.x
       * to higherLayer:WIFI.SSID.x because SSID object status should be updated 
       * updated by its corresponding net devices(wlx or wlx.y) link state changes,
       * check checkWifiLinkStatusLocked_dev2().
       */
      cmsLog_debug("skip for the higherLayer %s!!", intfStackObj->higherLayer);
      return;
   }
#endif

   cmsLog_debug("higherLayer %s (status=%s) ==> %s",
           intfStackObj->higherLayer, higherLayerStatusBuf, newStatus);

   /*
    * If we get here, we need to set new status on the higherLayer.
    * But first check for special case situations, e.g.
    * ppp state machine, IP layer service state machine,
    * DSL channel types, etc.
    */
   if ((higherLayerPathDesc.oid == MDMOID_DEV2_PPP_INTERFACE) &&
       (!strcmp(newStatus, MDMVS_UP)))
   {
      char *xtmLinkFullpath=NULL;

      cmsLog_notice("in ppp UP special case");
      /* Set PPP.Interface status to DORMANT, meaning it is waiting
       * for the ppp client to connect to the server.
       */
      intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                          MDMVS_DORMANT);

      /* Set ConnectionStatus on the ppp object so it can start the
       * ppp client */
      sskConn_setPppConnStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                          MDMVS_CONNECTING);

      if (IS_BDK_SYSMGMT &&
          isOverXtm(intfStackObj->higherLayer, &xtmLinkFullpath))
      {
         cmsLog_debug("pppd is started, set xtm lock");
         sendPppXtmLockMsg(appMsgHandle, xtmLinkFullpath, "LOCKED");
         CMSMEM_FREE_BUF_AND_NULL_PTR(xtmLinkFullpath);
      }

      /*
       * Do not propagate status up.  PPP will send messages back
       * to ssk.  The ssk2_connstatus.c code will call
       * propagateStatus when it is fully connected.
       */
   }
   else if ((higherLayerPathDesc.oid == MDMOID_DEV2_IP_INTERFACE) &&
           (!strcmp(newStatus, MDMVS_UP)))
   {
      Dev2IpInterfaceObject *ipIntfObj=NULL;
      CmsRet r2;

      cmsLog_debug("in IP.Interface UP special case");

      if ((r2 = cmsObj_get(higherLayerPathDesc.oid,
                           &higherLayerPathDesc.iidStack,
                           OGF_NO_VALUE_UPDATE,
                           (void **)&ipIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IP.Interface object, ret=%d", ret);
      }
      else
      {
         /*
          * DORMANT means IP.Interface is about to come up, but still
          * waiting for some external events, e.g. dhcpc or ppp to
          * get the IP address.
          */
         if (strcmp(higherLayerStatusBuf, MDMVS_DORMANT) &&
             strcmp(higherLayerStatusBuf, MDMVS_UP))
         {
            intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                                MDMVS_DORMANT);
         }

         if (ipIntfObj->IPv4Enable)
         {
            char ipv4ServiceStatusBuf[BUFLEN_64]={0};

            getIpv4ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                            ipv4ServiceStatusBuf,
                                     sizeof(ipv4ServiceStatusBuf));
            cmsLog_debug("%s current IPv4ServiceStatus=%s",
                         ipIntfObj->name, ipv4ServiceStatusBuf);

            /*
             * If we are currently in SERVICEDOWN, advance to
             * SERVICESTARTING.  If we are not in SERVICEUP, check if
             * we can go to SERVICEUP.  We may end up doing nothing on
             * the IPv4 side because this UP status is being propagated
             * up the intfStack for IPv6.
             */
            if (!strcmp(ipv4ServiceStatusBuf, MDMVS_SERVICEDOWN))
            {
               setIpv4ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                               MDMVS_SERVICESTARTING);
               strcpy(ipv4ServiceStatusBuf, MDMVS_SERVICESTARTING);
            }

            if (strcmp(ipv4ServiceStatusBuf, MDMVS_SERVICEUP))
            {
               if (sskConn_hasAnyIpv4AddressLocked(&higherLayerPathDesc.iidStack) ||
                   (ipIntfObj->X_BROADCOM_COM_BridgeService == TRUE &&
                    ipIntfObj->X_BROADCOM_COM_BridgeNeedsIpAddr == FALSE))
               {
                  /*
                   * IP.Interface has an address (it may be static, or
                   * it may have been set already by ppp or dhcpc).
                   * Or it is a bridge service, which normally does not
                   * need IP addr. It can now go into the SERVICEUP state.
                   */
                  intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                                      MDMVS_UP);
                  setIpv4ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                                  MDMVS_SERVICEUP);
#ifdef BRCM_VOICE_SUPPORT
                  if (!ipIntfObj->X_BROADCOM_COM_BridgeService)
                  {
                     if (sskVoiceCallbacks.initVoiceOnIntfUp != NULL)
                     {
                        (*sskVoiceCallbacks.initVoiceOnIntfUp)(
                                          CMS_AF_SELECT_IPV4,
                                          ipIntfObj->name,
                                          ipIntfObj->X_BROADCOM_COM_Upstream);
                     }
                  }
#endif
                  // IPv4 is SERVICEUP, tell sys_directory
                  sendIpInterfaceEventMsg(appMsgHandle, intfStackObj->higherLayer);
               }
               /* else {
                *   this IP.Interface does not have any IP address
                *   configured, so nothing more to do here.  dhcpc will
                *   send messages back to ssk.  The code in
                *   ssk2_connstatus.c will advance the interface stack
                *   status and service state machines.
                * }
                */
            }
         }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
         if (ipIntfObj->IPv6Enable)
         {
            char ipv6ServiceStatusBuf[BUFLEN_64]={0};

            getIpv6ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                            ipv6ServiceStatusBuf,
                                     sizeof(ipv6ServiceStatusBuf));

            cmsLog_debug("%s current IPv6ServiceStatus=%s",
                         ipIntfObj->name, ipv6ServiceStatusBuf);

            if (!strcmp(ipv6ServiceStatusBuf, MDMVS_SERVICEDOWN))
            {
               char *prefixFullPath = NULL;
               InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
               Dev2Ipv6PrefixObject *prefixObj = NULL;


               setIpv6ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                               MDMVS_SERVICESTARTING);

               cmsLog_debug("Search for valid prefix delegation!");

               while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                                                &higherLayerPathDesc.iidStack,
                                                &iidStack,
                                                OGF_NO_VALUE_UPDATE,
                                                (void **)&prefixObj) == CMSRET_SUCCESS)
               {
                  /* Search for invalid prefix delegations and configure each one and its child PD. */
                  if (prefixObj->enable && !cmsUtl_strcmp(prefixObj->origin, MDMVS_STATIC) &&
                       !cmsUtl_strcmp(prefixObj->staticType, MDMVS_PREFIXDELEGATION) &&
                       !cmsUtl_strcmp(prefixObj->prefixStatus, MDMVS_INVALID))
                  {
                     MdmPathDescriptor prefixPathDesc;

                     INIT_PATH_DESCRIPTOR(&prefixPathDesc);
                     prefixPathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
                     prefixPathDesc.iidStack = iidStack;

                     if ((r2 = cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPathDesc, &prefixFullPath)) != CMSRET_SUCCESS)
                     {
                        cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", r2);
                     }
                     else
                     {
                        intfStack_configureIPv6PrefixDelegation(prefixFullPath);
                     }
                     CMSMEM_FREE_BUF_AND_NULL_PTR(prefixFullPath);
                  }
                  cmsObj_free((void **) &prefixObj);
               }
               setIpv6ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                               MDMVS_SERVICESTARTING);
               strcpy(ipv6ServiceStatusBuf, MDMVS_SERVICESTARTING);
            }

            if (strcmp(ipv6ServiceStatusBuf, MDMVS_SERVICEUP))
            {
               /* For IPv6, "any" IPv6 address may be too relaxed.
                * Maybe we can only move to UP and SERVICEUP if we have
                * a globally unique address (not link local).
                */
               if (sskConn_hasAnyIpv6AddressLocked(&higherLayerPathDesc.iidStack))
               {
                  setIpv6ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                                  MDMVS_SERVICEUP);
#ifdef BRCM_VOICE_SUPPORT
                  if (!ipIntfObj->X_BROADCOM_COM_BridgeService)
                  {
                     if (sskVoiceCallbacks.initVoiceOnIntfUp != NULL)
                     {
                        (*sskVoiceCallbacks.initVoiceOnIntfUp)(
                                          CMS_AF_SELECT_IPV6,
                                          ipIntfObj->name,
                                          ipIntfObj->X_BROADCOM_COM_Upstream);
                     }
                  }
#endif
                  // IPv6 is SERVICEUP, tell sys_directory
                  sendIpInterfaceEventMsg(appMsgHandle, intfStackObj->higherLayer);
               }
            }
         }
#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
         cmsObj_free((void **) &ipIntfObj);
      }

      /* Since IP.Interface is the highest interface in the intf stack,
       * no need to propagate up.
       */
   }
#ifdef DMP_DEVICE2_DSL_1
   else if ((higherLayerPathDesc.oid == MDMOID_DEV2_DSL_CHANNEL) &&
            (strcmp(newStatus, MDMVS_DOWN) != 0))
   {
      /* if DSL line is trained to use ATM encapsulation, then propagate UP status to ATM channel,
       * and set the PTM channel to disable.  And the same would apply to PTM encapsulation.
       */
      Dev2DslChannelObject *channelObj=NULL;
      CmsRet rc;
      UBOOL8 encapNotTrained = FALSE;

      if ((rc = cmsObj_get(higherLayerPathDesc.oid,
                           &higherLayerPathDesc.iidStack,
                           OGF_NO_VALUE_UPDATE,
                           (void **)&channelObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get DSL.Channel object, ret=%d", rc);
      }
      else
      {
         if (qdmDsl_isAtmConnectionLocked(channelObj->lowerLayers))
         {
            if (!strcmp(channelObj->linkEncapsulationUsed,MDMVS_G_993_2_ANNEX_K_PTM))
            {
               /* channel is an ATM channel, but link is trained PTM */
               encapNotTrained = TRUE;
            }
         }
         else
         {
            if (!strcmp(channelObj->linkEncapsulationUsed,MDMVS_G_992_3_ANNEX_K_ATM))
            {
               /* channel is PTM, but link is trained ATM */
               encapNotTrained = TRUE;
            }
         }
         if (encapNotTrained == TRUE)
         {
            intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                                MDMVS_NOTPRESENT);
            /* propagate status Link NotPresent to upper layers on the interface stack */
            intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                      intfStackObj->higherLayer,
                                                      MDMVS_NOTPRESENT);
         }
         else
         {
            /* propagate status to upper layers on the interface stack */
            cmsLog_debug("DSL channel: propagate newStatus %s to higher %s",
                         newStatus, intfStackObj->higherLayer);
            intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                                newStatus);
            intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                      intfStackObj->higherLayer,
                                                      newStatus);
         }
         cmsObj_free((void **) &channelObj);
      } /* channelObj */
   } /* DSL channel */
#ifdef DMP_DEVICE2_BONDEDDSL_1
   /* same for bonding, the status of a bonding group is not just dependent on the channels,
      bonding status needs to be retrieved from the driver even when the channels are up */
   else if ((higherLayerPathDesc.oid == MDMOID_DEV2_DSL_BONDING_GROUP) &&
            (strcmp(newStatus,MDMVS_UP) == 0))
   {
      Dev2DslBondingGroupObject *bondingGroupObj=NULL;
      CmsRet rc;

      if ((rc = cmsObj_get(higherLayerPathDesc.oid,
                           &higherLayerPathDesc.iidStack,
                           OGF_NO_VALUE_UPDATE,
                           (void **)&bondingGroupObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get DSL.BondingGroup object, ret=%d", rc);
         intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                             MDMVS_UNKNOWN);
         /* propagate status Link NotPresent to upper layers on the interface stack */
         intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                   intfStackObj->higherLayer,
                                                   MDMVS_UNKNOWN);
      }
      else
      {
         cmsObj_free((void **) &bondingGroupObj);
         intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                             newStatus);
         /* need to check to see if we need to change the lower layer of
          * PTM/ATM links because there may be traffic type mismatch for bonding
         */
         updateXtmLowerLayerLocked();
         intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                   intfStackObj->higherLayer,
                                                   newStatus);
      }
   } /* DSL bonding group OID */
#endif /* DMP_DEVICE2_BONDEDDSL_1 */
#endif /* DMP_DEVICE2_DSL_1 */
   else if (!strcmp(newStatus, MDMVS_DOWN) ||
            !strcmp(newStatus, MDMVS_UNKNOWN) ||
            !strcmp(newStatus, MDMVS_DORMANT) ||
            !strcmp(newStatus, MDMVS_NOTPRESENT) ||
            !strcmp(newStatus, MDMVS_LOWERLAYERDOWN) ||
            !strcmp(newStatus, MDMVS_ERROR))
   {
      // Process the various variants of Down, including special cases.
      cmsLog_debug("in DOWN special case (actual newStatus=%s)", newStatus);

      if (isBridgeMgmtPort(intfStackObj->higherLayer))
      {
         cmsLog_debug("do not propagate non-UP status through bridge mgmt port %s",
                      intfStackObj->higherLayer);
      }
      else if (intfStack_optionalCheckForEnabledByFullpath(intfStackObj->higherLayer) &&
               isAnyLowerLayerIntfUpLocked(intfStackObj->higherLayer,
                                           intfStackObj->lowerLayer))
      {
         cmsLog_debug("one or more lowerLayer interfaces is still UP -- do nothing");
      }
      else
      {
         UBOOL8 doDownAction=TRUE;
         /*
          * xlate lowerLayer status to appropriate higherLayer status.
          * Seems like in all cases, the appropriate higherLayer status
          * is MDMVS_LOWERLAYERDOWN:
          * LowerLayer Status            HigherLayer Status
          * DOWN                   ->    LOWERLAYERDOWN
          * UNKNOWN                ->    LOWERLAYERDOWN
          * DORMANT                ->    LOWERLAYERDOWN
          * NOTPRESENT             ->    LOWERLAYERDOWN
          * LOWERLAYERDOWN         ->    LOWERLAYERDOWN
          * ERROR                  ->    LOWERLAYERDOWN
          */
#ifdef DMP_DEVICE2_DSL_1
         /*
          * Before bringing ptmx.y or atmx.y link down, check if there is a ppp
          * interface above it.  PPP needs to be brought down first so the
          * refcount on the ptmx.y/atmx.y interface goes to 0. This does not
          * seem to be a problem for IPoE interfaces.  See Jira 31621, 38808,
          * and 38643.
          */
         if (IS_BDK_DSL_SSK &&
             IS_DEV2_XTM_LINK_OID(higherLayerPathDesc.oid))
         {
            UBOOL8 isLocked;
            isLocked = pppXtm_linkDownIsLocked(intfStackObj->higherLayer);
            if (isLocked)
               doDownAction = FALSE;
         }
         else if ((IS_BDK_SYSMGMT &&
                   IS_DEV2_XTM_LINK_OID(lowerLayerPathDesc.oid)) ||
                  IS_DEV2_XTM_LINK_OID(higherLayerPathDesc.oid))
         {
            // Due to the way things are connected in BDK, this special case
            // needs to be triggered when the XTM link objs are in the
            // lowerLayerPathDesc.  While in CMS classic, the XTM link objs are
            // in the higherLayerPathDesc.
            Dev2InterfaceStackObject *pppIntfStackObj=NULL;
            if (findIntfStackObjOverFullPathLocked(MDMOID_DEV2_PPP_INTERFACE,
                                                   intfStackObj->higherLayer,
                                                   &pppIntfStackObj))
            {
               setStatusOnPppFirst(pppIntfStackObj);
               /* now propagate to next level above ppp */
               intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                  pppIntfStackObj->higherLayer, MDMVS_LOWERLAYERDOWN);
               cmsObj_free((void **) &pppIntfStackObj);

               if (IS_BDK_SYSMGMT)
               {
                  char *xtmLinkFullpath=NULL;

                  /* 
                   * pppd should be deleted now
                   * Tell dsl_md that it can delete its xtm intf
                   */
                  if (isOverXtm(intfStackObj->higherLayer, &xtmLinkFullpath))
                  {
                     cmsLog_debug("pppd is deleted, delete xtm lock");
                     sendPppXtmLockMsg(appMsgHandle, xtmLinkFullpath, "");
                     CMSMEM_FREE_BUF_AND_NULL_PTR(xtmLinkFullpath);
                  }
               }
            }
         }
#endif  /* DMP_DEVICE2_DSL_1 */

         /*
          * Before bringing ethx.y link down, check if there is a ppp
          * interface above it.  PPP needs to be brought down first so the
          * refcount on the ethx.y interface goes to 0.
          */
         if( IS_DEV2_ETH_LINK_OID(higherLayerPathDesc.oid) )
         {
            Dev2InterfaceStackObject *pppIntfStackObj=NULL;
            if (findIntfStackObjOverFullPathLocked(MDMOID_DEV2_PPP_INTERFACE,
                                                   intfStackObj->higherLayer,
                                                   &pppIntfStackObj))
            {
               setStatusOnPppFirst(pppIntfStackObj);
               /* now propagate to next level above ppp */
               intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                  pppIntfStackObj->higherLayer, MDMVS_LOWERLAYERDOWN);
               cmsObj_free((void **) &pppIntfStackObj);
            }
         }

         if (doDownAction)
         {
            if (intfStack_optionalCheckForEnabledByFullpath(intfStackObj->higherLayer))
            {
               cmsLog_debug("doDownAction, on enabled %s, new status=lowerLayerDown", intfStackObj->higherLayer);
               intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                                   MDMVS_LOWERLAYERDOWN);
            }
            else
            {
               // This object is disabled, so the status should be DOWN
               // LowerLayer might actually be up, so LOWERLAYERDOWN is incorrect.
               cmsLog_debug("doDownAction on disabled %s, new status=DOWN", intfStackObj->higherLayer);
               intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                                   MDMVS_DOWN);
            }
         }

         if (higherLayerPathDesc.oid == MDMOID_DEV2_IP_INTERFACE)
         {
            /*
             * we have just marked the IP.Interface LOWERLAYERDOWN,
             * so change IPv4 and IPv6 service states to SERVICEDOWN.
             */
            Dev2IpInterfaceObject *ipIntfObj=NULL;
            CmsRet r2;

            if ((r2 = cmsObj_get(higherLayerPathDesc.oid,
                                 &higherLayerPathDesc.iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&ipIntfObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get IP.Interface object, ret=%d", r2);
            }
            else
            {
               if (ipIntfObj->IPv4Enable)
               {
                  setIpv4ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                                  MDMVS_SERVICEDOWN);
               }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
               if (ipIntfObj->IPv6Enable)
               {
                  char *prefixFullPath=NULL;
                  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
                  Dev2Ipv6PrefixObject *prefixObj=NULL;

                  cmsLog_debug("Search prefix delegation!");

                  while ((cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                                                   &higherLayerPathDesc.iidStack,
                                                   &iidStack,
                                                   OGF_NO_VALUE_UPDATE,
                                                   (void **)&prefixObj) == CMSRET_SUCCESS))
                  {
                     /* prefix delegation */
                     if (prefixObj->enable && !cmsUtl_strcmp(prefixObj->origin, MDMVS_STATIC) &&
                          !cmsUtl_strcmp(prefixObj->staticType, MDMVS_PREFIXDELEGATION) &&
                          cmsUtl_isValidIpAddress(AF_INET6, prefixObj->prefix))
                     {
                        MdmPathDescriptor prefixPathDesc;

                        INIT_PATH_DESCRIPTOR(&prefixPathDesc);
                        prefixPathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
                        prefixPathDesc.iidStack = iidStack;

                        if ((r2 = cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPathDesc, &prefixFullPath)) != CMSRET_SUCCESS)
                        {
                           cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", r2);
                        }
                        else
                        { 
                           intfStack_bringDownIPv6PrefixDelegation(appMsgHandle, prefixFullPath);
                           CMSMEM_FREE_BUF_AND_NULL_PTR(prefixFullPath);
                        }
                     }
                     cmsObj_free((void **) &prefixObj);
                  }

                  setIpv6ServiceStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                                  MDMVS_SERVICEDOWN);
               }
#endif
               cmsObj_free((void **) &ipIntfObj);
            }

            // Tell sys_directory about IP interface down
            sendIpInterfaceEventMsg(appMsgHandle, intfStackObj->higherLayer);

            /* IP.Interface is highest layer in the stack, so no need
             * to propagate further.
             */
         }
         else
         {
            if (higherLayerPathDesc.oid == MDMOID_DEV2_PPP_INTERFACE)
            {
               /*
                * When link goes down, we also need to set ppp
                * connStatus to DISCONNECTED in addition to all the
                * normal interface stack propagation.
                */
               sskConn_setPppConnStatusByIidLocked(&higherLayerPathDesc.iidStack,
                                                   MDMVS_DISCONNECTED);
            }

            /* propagate up the interface stack */
            cmsLog_debug("down case: set %s ==> %s and propagate",
                         intfStackObj->higherLayer, newStatus);
            intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                               intfStackObj->higherLayer,
                                               MDMVS_LOWERLAYERDOWN);

            // Notify sys_directory of L2 interface down
            if ((higherLayerPathDesc.oid == MDMOID_DEV2_BRIDGE_PORT) ||
                 IS_DEV2_XTM_LINK_OID(higherLayerPathDesc.oid))
            {
               sendLayer2InterfaceEventMsg(appMsgHandle, newStatus,
                                           &higherLayerPathDesc);
               if (doDownAction &&
                   IS_BDK_DSL_SSK &&
                   IS_DEV2_XTM_LINK_OID(higherLayerPathDesc.oid))
               {
                  sendSubscribePppXtmLockMsg(appMsgHandle,
                                             intfStackObj->higherLayer, FALSE);
               }
            }
         }
      }
   }
   else
   {
      /*
       * None of the special cases above were applicable, so just set the
       * current higher layer object status and propagate up the interface stack.
       * In practice, by the time we get to this block, the new status is
       * always UP.
       */
      cmsLog_debug("normal: set %s (%d) ==> %s and propagate",
                   intfStackObj->higherLayer, higherLayerPathDesc.oid,
                   newStatus);

      if (!cmsUtl_strcmp(newStatus, MDMVS_UP) &&
          !intfStack_optionalCheckForEnabledByFullpath(intfStackObj->higherLayer))
      {
         // Status to propagate is UP, but this object is not enabled,
         // do nothing on this object and stop propagation.
         cmsLog_debug("%s is not enabled, stop propagation",
                      intfStackObj->higherLayer);
         return;
      }

      intfStack_setStatusByFullPathLocked(intfStackObj->higherLayer,
                                          newStatus);

      intfStack_propagateStatusByFullPathLocked(appMsgHandle,
                                                intfStackObj->higherLayer,
                                                newStatus);

      // Special processing for bridge and DSL ports (update sys_directory).
      if ((higherLayerPathDesc.oid == MDMOID_DEV2_BRIDGE_PORT) ||
          IS_DEV2_XTM_LINK_OID(higherLayerPathDesc.oid))
      {
         // Special processing for UP case (create xtm/ppp lock).
         if (!cmsUtl_strcmp(newStatus, MDMVS_UP) &&
             IS_BDK_DSL_SSK &&
             IS_DEV2_XTM_LINK_OID(higherLayerPathDesc.oid))
         {
            pppXtm_linkUpAddEntry(intfStackObj->higherLayer);
            sendSubscribePppXtmLockMsg(appMsgHandle,
                                       intfStackObj->higherLayer, TRUE);
         }
         sendLayer2InterfaceEventMsg(appMsgHandle, newStatus, &higherLayerPathDesc);
      }
   }

   cmsLog_debug("Exit: %s->%s (%s)",
                intfStackObj->lowerLayer, intfStackObj->higherLayer,
                newStatus);

   return;
}


UBOOL8 isBridgeMgmtPort(const char *higherLayerFullPath)
{
   Dev2BridgePortObject *brPortObj=NULL;
   MdmPathDescriptor pathDesc;
   UBOOL8 rv=FALSE;

   if (cmsMdm_fullPathToPathDescriptor(higherLayerFullPath, &pathDesc) == CMSRET_SUCCESS)
   {
      if (pathDesc.oid == MDMOID_DEV2_BRIDGE_PORT)
      {
         if (cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **) &brPortObj) == CMSRET_SUCCESS)
         {
            rv = brPortObj->managementPort;
            cmsObj_free((void **) &brPortObj);
         }
      }
   }

   return rv;
}


/** check if the given higherLayerFullPath has any lowerLayers which is
 *  still up, but do not consider the excludeLowerlayerFullPath.  The
 *  excludeLowerLayerFullPath logic is needed in the delete a fullpath from
 *  the LowerLayers param case because we propagate status before deleting
 *  the intf stack entry.
 *
 *  @param higherLayerFullPath (IN) higher layer to check
 *  @param excludeLowerLayerFullPath (IN) exclude this lower layer
 *
 *  @return TRUE if there is a lower layer with status UP
 */
UBOOL8 isAnyLowerLayerIntfUpLocked(const char *higherLayerFullPath,
                                   const char *excludeLowerLayerFullPath)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj=NULL;
   UBOOL8 isUp=FALSE;
   CmsRet ret;

   while (!isUp &&
          (ret = cmsObj_getNext(MDMOID_DEV2_INTERFACE_STACK, &iidStack, (void **)&intfStackObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(intfStackObj->higherLayer, higherLayerFullPath) &&
           cmsUtl_strcmp(intfStackObj->lowerLayer, excludeLowerLayerFullPath))
      {
         isUp = qdmIntf_isStatusUpOnFullPathLocked_dev2(intfStackObj->lowerLayer);
      }

      cmsObj_free((void **) &intfStackObj);
   }

   cmsLog_debug("higherLayer=%s isUp=%d", higherLayerFullPath, isUp);
   return isUp;
}


UBOOL8 getUpperLayerPathDescFromLowerLayerLocked(const char *lowerLayerFullPath,
                                       MdmObjectId oid,
                                       MdmPathDescriptor *upperLayerPathDesc)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj=NULL;
   MdmPathDescriptor higherLayerPathDesc;
   CmsRet ret;
   UBOOL8 found=FALSE;

   INIT_PATH_DESCRIPTOR(upperLayerPathDesc);

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_INTERFACE_STACK, &iidStack,
                                 (void **)&intfStackObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(intfStackObj->lowerLayer, lowerLayerFullPath))
      {
         INIT_PATH_DESCRIPTOR(&higherLayerPathDesc);
         ret = cmsMdm_fullPathToPathDescriptor(intfStackObj->higherLayer, &higherLayerPathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                          intfStackObj->higherLayer, ret);
            cmsObj_free((void **) &intfStackObj);
            return FALSE;
         }

         if (higherLayerPathDesc.oid == oid)
         {
            *upperLayerPathDesc = higherLayerPathDesc;
            found = TRUE;
         }
         else
         {
            /* recurse */
            return (getUpperLayerPathDescFromLowerLayerLocked(
                                               intfStackObj->higherLayer,
                                               oid,
                                               upperLayerPathDesc));
         }
      }

      cmsObj_free((void **) &intfStackObj);
   }

   return found;
}


static UINT32 intfStackMsgCount=0;
static CmsTimestamp lastMsgTimestamp;

void writeIntfStackToConfig()
{
   CmsTimestamp nowTms;
   UINT32 deltaSinceBoot;
   UINT32 deltaSinceLastMsg;
   UBOOL8 saveConfig=FALSE;

   intfStackMsgCount++;
   if (intfStackMsgCount == 1)
   {
      // init lastMsgTimestamp on first msg, otherwise, deltaSinceLastMsg
      // will be incorrect (too large) on the first msg.
      cmsTms_get(&lastMsgTimestamp);
   }

   cmsTms_get(&nowTms);
   deltaSinceBoot = cmsTms_deltaInMilliSeconds(&nowTms, &bootTimestamp);
   deltaSinceLastMsg = cmsTms_deltaInMilliSeconds(&nowTms, &lastMsgTimestamp);
   lastMsgTimestamp = nowTms;

   /*
    * When the system boots, we will get 20 to 100+ LowerLayersChanged messages
    * back-to-back as the MDM is initializing itself.
    * Do not write out the config file during this time because either 
    * they are duplicate entries, or they will be regenerated on the
    * next boot.  However, after this initial period (let's say 45 seconds
    * in case the system is slow), or if this message is not back-to-back
    * with the previous message (separated by more than 3 seconds), then
    * this must be the ACS or a human configuring something, so write out
    * the config file.
    */
   saveConfig = ((deltaSinceBoot > 45000) || (deltaSinceLastMsg > 3000));

   cmsLog_debug("[%d] deltaSinceBoot=%d deltaSinceLastMsg=%d saveConfig=%d",
                intfStackMsgCount,
                deltaSinceBoot, deltaSinceLastMsg, saveConfig);

   if (saveConfig)
   {
      CmsRet ret;

      // With MDM Zone locking and full auto-lock, it is better to call
      // cmsMgm_saveConfigToFlash without lock (enables backoff and retry).
      if ((ret = cmsMgm_saveConfigToFlash()) != CMSRET_SUCCESS)
      {
         cmsLog_error("write to config file failed, ret=%d", ret);
      }
   }

   return;
}

UBOOL8 isOnSameStackLocked(const char *higherLayerFullPath, const char *targetFullPath)
{
   Dev2InterfaceStackObject *intfStackObj=NULL;
   UBOOL8 found=FALSE;

   /* check corner conditions first */
   if (higherLayerFullPath == NULL)
   {
      return FALSE;
   }
   if (!strcmp(higherLayerFullPath, targetFullPath))
   {
      return TRUE;
   }

   /* convert fullpath to intfStackObject to get the lowerlayer path */
   if (!findIntfStackEntryLocked(higherLayerFullPath, NULL, &intfStackObj, NULL))
   {
      /* For PPPoA case, lowerlay may be in different md */
      cmsLog_notice("Intf Stack inconsistency! could not find obj for %s",
          higherLayerFullPath);
      return FALSE;
   }

   if (intfStackObj->lowerLayer == NULL)
   {
      /* hit bottom of interface stack */
      found = FALSE;
   }
   if (!strcmp(intfStackObj->lowerLayer, targetFullPath))
   {
      /* found what we were looking for */
      found = TRUE;
   }
   else
   {
      /* recurse down one layer */
      found = isOnSameStackLocked(intfStackObj->lowerLayer, targetFullPath);
   }
   cmsObj_free((void **) &intfStackObj);
   return found;
}

UBOOL8 findIntfStackObjOverFullPathLocked(MdmObjectId topOid,
                                    const char *targetFullPath,
                                    Dev2InterfaceStackObject **retIntfStackObj)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj = NULL;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

   cmsLog_debug("finding oid %d over %s", topOid, targetFullPath);

   while (cmsObj_getNextFlags(MDMOID_DEV2_INTERFACE_STACK, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&intfStackObj) == CMSRET_SUCCESS)
   {
      cmsMdm_fullPathToPathDescriptor(intfStackObj->higherLayer, &pathDesc);
      if (pathDesc.oid == topOid)
      {
         if (isOnSameStackLocked(intfStackObj->lowerLayer, targetFullPath))
         {
            *retIntfStackObj = intfStackObj;
            return TRUE;
         }
      }

      cmsObj_free((void **) &intfStackObj);
   }

   return FALSE;
}

void setStatusOnPppFirst(const Dev2InterfaceStackObject *pppIntfStackObj)
{
   Dev2InterfaceStackObject *vlanIntfStackObj=NULL;
   PhlSetParamValue_t paramValues[3];
   CmsRet ret;
   
   cmsLog_debug("Entered with ppp at %s", pppIntfStackObj->higherLayer);
   /*
    * Find the vlan or normal layer 2 interface object below ppp.
    * The PPP rcl handler will not kill ppp unless it sees the lower layer
    * is down.
    */
   if (!findIntfStackEntryLocked(pppIntfStackObj->lowerLayer, NULL, &vlanIntfStackObj, NULL))
   {
      cmsLog_error("Intf Stack inconsistency! could not find obj for %s", pppIntfStackObj->lowerLayer);
      return;
   }
   cmsLog_debug("Found vlan at %s", vlanIntfStackObj->higherLayer);
   
   /*
    * Construct a special PHL set sequence.  The first 2 params are on the
    * PPP interface object to set status=>LOWERLAYERDOWN and
    * connectionStatus=>DISCONNECTED.  But we also have to set the vlan
    * object below the PPP as LOWERLAYERDOWN because the PPP rcl handler
    * code will not kill pppd unless it sees the lower layer is down.
    * By setting the vlan object in the same cmsPhl_set command, the updated
    * vlan object will be in the ODL setqueue, and its RCL handler function
    * will be called after the RCL handler function for ppp.
    */
   memset(paramValues, 0, sizeof(paramValues));
   cmsMdm_fullPathToPathDescriptor(pppIntfStackObj->higherLayer,
                                   &(paramValues[0].pathDesc));
   sprintf(paramValues[0].pathDesc.paramName, "Status");
   paramValues[0].pParamType = "string";
   paramValues[0].pValue = MDMVS_LOWERLAYERDOWN;
   paramValues[0].status = CMSRET_SUCCESS;
   
   paramValues[1] = paramValues[0];
   sprintf(paramValues[1].pathDesc.paramName, "ConnectionStatus");
   paramValues[1].pValue = MDMVS_DISCONNECTED;
   
   paramValues[2] = paramValues[0];
   cmsMdm_fullPathToPathDescriptor(vlanIntfStackObj->higherLayer,
                                   &(paramValues[2].pathDesc));
   sprintf(paramValues[2].pathDesc.paramName, "Status");

   cmsObj_free((void **) &vlanIntfStackObj);
   
   ret = cmsPhl_setParameterValues(paramValues,
      sizeof(paramValues)/sizeof(PhlSetParamValue_t));
   
   cmsLog_debug("Exit (ret=%d)", ret);
}

// Return TRUE if given higherLayer sits on top of a PTM_LINK or ATM_LINK.
// Also returns the fullpath to the XTM_LINK.
// Caller is responsible for freeing xtmLinkFullpath.
// Needed in BDK XTM link down processing, but outside of ifdefs to avoid
// yet another ifdef.
UBOOL8 isOverXtm(const char *higherLayerFullpath, char **xtmLinkFullpath)
{
   UBOOL8 found=FALSE;
   Dev2InterfaceStackObject *intfStackObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // Find the intfstack obj again, match with higher layer
   while ((ret = cmsObj_getNext(MDMOID_DEV2_INTERFACE_STACK, &iidStack,
                               (void **)&intfStackObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(intfStackObj->higherLayer, higherLayerFullpath))
      {
         break;
      }
      cmsObj_free((void **) &intfStackObj);
   }
   if (ret != CMSRET_SUCCESS)
   {
      return FALSE;
   }

   cmsLog_debug("Entered: higher %s lower %s",
                intfStackObj->higherLayer,
                intfStackObj->lowerLayer);

   {
      // now look for ATM Link or PTM LINK
      UINT32 ptmFullpathLen=strlen(DEV2_PTM_LINK_FULLPATH);
      UINT32 atmFullpathLen=strlen(DEV2_ATM_LINK_FULLPATH);
      if (!cmsUtl_strncmp(intfStackObj->lowerLayer, DEV2_PTM_LINK_FULLPATH, ptmFullpathLen) ||
          !cmsUtl_strncmp(intfStackObj->lowerLayer, DEV2_ATM_LINK_FULLPATH, atmFullpathLen))
      {
         *xtmLinkFullpath = cmsMem_strdup(intfStackObj->lowerLayer);
         cmsLog_debug("found xtm! %s", *xtmLinkFullpath);
         found = TRUE;
      }
   }

   if (!found && !IS_EMPTY_STRING(intfStackObj->lowerLayer))
   {
      found = isOverXtm(intfStackObj->lowerLayer, xtmLinkFullpath);
   }

   cmsObj_free((void **) &intfStackObj);
   return found;
}


#if defined(DMP_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)

UBOOL8 isTr98WanIpPppObj(const char *fullPath)
{
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", fullPath, ret);
      return FALSE;
   }

   if ((pathDesc.oid == MDMOID_WAN_IP_CONN) ||
       (pathDesc.oid == MDMOID_WAN_PPP_CONN))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}



/* in rut_wanlayer2.c */
extern UBOOL8 rutWl2_isWanLayer2LinkUp(MdmObjectId wanConnOid,
                                       const InstanceIdStack *iidStack);

CmsRet getTr98L2StatusFromL3FullPath(const char *fullPath,
                                     char *statusBuf, UINT32 statusBufLen)
{
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   CmsRet ret;

   memset(statusBuf, 0, statusBufLen);

   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", fullPath, ret);
      return ret;
   }

   if ((pathDesc.oid != MDMOID_WAN_IP_CONN) &&
       (pathDesc.oid != MDMOID_WAN_PPP_CONN))
   {
      cmsLog_error("OID %d not supported", pathDesc.oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (rutWl2_isWanLayer2LinkUp(pathDesc.oid, &pathDesc.iidStack))
   {
      snprintf(statusBuf, statusBufLen, "%s", MDMVS_UP);
   }
   else
   {
      snprintf(statusBuf, statusBufLen, "%s", MDMVS_DOWN);
   }

   cmsLog_debug("%s status %s", fullPath, statusBuf);
   return CMSRET_SUCCESS;
}
#endif /* defined(DMP_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1) */

#endif  /* DMP_DEVICE2_BASELINE_1 */
