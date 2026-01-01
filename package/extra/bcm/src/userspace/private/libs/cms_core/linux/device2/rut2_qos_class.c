/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
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

#ifdef DMP_DEVICE2_QOS_1
/* All of this code is part of the DEVICE2_QOS_1 profile (which is enabled
 * only in Pure TR181 mode.
 */

#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_qos.h"
#include "rcl.h"
#include "prctl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_iptables.h"




/* Local helper functions */
static UBOOL8 isAffectClassifications_dev2(const char *intfName);




void rutQos_reconfigAllClassifications_dev2(const char *intfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   UBOOL8 doReconfig=FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("====  Entered: intfName=%s", intfName);

   /*
    * determine if the intf that came up or down is related to any
    * classifications.  If not, save some CPU cycles and do nothing.
    */
   if ((intfName == NULL) ||
       (!cmsUtl_strncmp(intfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR))) ||
       isAffectClassifications_dev2(intfName))
   {
      doReconfig = TRUE;
   }

   if (!doReconfig)
   {
      cmsLog_debug("No reconfig needed for %s", intfName);
      return;
   }

   /* for more classification rules, the operation may take longer  */
   cmsLck_setHoldTimeWarnThresh(12000);
   /* make sure that all the required modules for qos support are loaded */
   rutIpt_qosLoadModule();


   /* walk over all classifications and unconfig everything that was
    * previously configured */
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("unconfig [%s] status %s", cObj->X_BROADCOM_COM_ClassName, cObj->status);
      if (!cmsUtl_strcmp(cObj->status, MDMVS_ENABLED))
      {
         rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, cObj);

         /* record the fact that we unconfigured.  Just setting status
          * will not trigger any action in rcl_dev2QosClassificationObject
          */
         CMSMEM_REPLACE_STRING_FLAGS(cObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         ret = cmsObj_set(cObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Dev2QosClassificationObject, ret=%d", ret);
         }
      }

      cmsObj_free((void **)&cObj);
   }

   /* now walk over all classifications again and Config everything that
    * needs to be configured.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      UBOOL8 ingressUp=FALSE;
      UBOOL8 egressUp=FALSE;
      char intfNameBuf[CMS_IFNAME_LENGTH]={0};
      CmsRet r2;

      if (IS_EMPTY_STRING(cObj->interface))
      {
         ingressUp = TRUE;
      }
      else if (!cmsUtl_strcmp(cObj->interface, MDMVS_LAN) ||
               !cmsUtl_strcmp(cObj->interface, MDMVS_WAN) ||
               !cmsUtl_strcmp(cObj->interface, MDMVS_LOCAL))
      {
         ingressUp = TRUE;
      }
      else
      {
         r2 = qdmIntf_fullPathToIntfnameLocked(cObj->interface, intfNameBuf);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not convert %s to intfName, r2=%d",
                         cObj->interface, r2);
         }
         else
         {
            ingressUp = cmsNet_isInterfaceUp(intfNameBuf);
         }
      }

      /*
       * If ingress is UP, then we continue to check if egress is UP.
       * If ingress is not UP, then don't do any more work because we
       * are not going to config this classification anyways.
       */
      if (ingressUp)
      {
         UBOOL8 isEnabled=FALSE;

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->X_BROADCOM_COM_ClassQueue,
                                                    &isEnabled, NULL, NULL);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get info on Queue %d", cObj->X_BROADCOM_COM_ClassQueue);
         }
         else
         {
            if (isEnabled && cmsNet_isInterfaceUp(cObj->X_BROADCOM_COM_egressInterface))
            {
               egressUp = TRUE;
            }
         }
      }

      cmsLog_debug("config [%s] ingress=%s ingressUp=%d egr=%s egressUp=%d",
                   cObj->X_BROADCOM_COM_ClassName, cObj->interface, ingressUp,
                   cObj->X_BROADCOM_COM_egressInterface, egressUp);

      if (ingressUp && egressUp)
      {
         rutQos_qMgmtClassConfig(QOS_COMMAND_CONFIG, cObj);

         /* record the fact that we unconfigured.  Just setting status
          * will not trigger any action in rcl_dev2QosClassificationObject
          */
         CMSMEM_REPLACE_STRING_FLAGS(cObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
         cmsObj_set(cObj, &iidStack);
      }

      cmsObj_free((void **)&cObj);
   }


#ifdef BRCM_WLAN
   /* see rclQos_classConfig (basically defaultPolicy for wlan) */
   if(intfName && cmsUtl_strstr(intfName, WLAN_IFC_STR))
   {
      char *fullPath=NULL;
      /* convert l2IntfName to fullpath for comparison */
      if ((ret = qdmIntf_intfnameToFullPathLocked(intfName, TRUE, &fullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d",
                    intfName, ret);
      }
      else
      {
         char statusBuf[BUFLEN_16] = {0};
         cmsLog_debug("l2IntfName %s ==> %s", intfName, fullPath);

         ret = qdmIntf_getStatusFromFullPathLocked_dev2(fullPath, statusBuf, sizeof(statusBuf)); 
         if (ret == CMSRET_SUCCESS &&
               !cmsUtl_strcmp(statusBuf, MDMVS_UP))
            rutQos_doDefaultWlPolicy(intfName, TRUE);
         else
            rutQos_doDefaultWlPolicy(intfName, FALSE);

         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }
   }
#endif  /* BRCM_WLAN */


   /* Delete and then re-add the DSCP marks to make sure they are last (well,
    * close to last... I am following the same order as rclQos_classConfig */
   {
      Dev2QosObject *qosObj=NULL;
      CmsRet r2;

      INIT_INSTANCE_ID_STACK(&iidStack);
      r2 = cmsObj_get(MDMOID_DEV2_QOS, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&qosObj);
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get QOS obj, ret=%d", ret);
      }
      else
      {
         rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_UNCONFIG, qosObj->defaultDSCPMark);
         rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_CONFIG, qosObj->defaultDSCPMark);
         cmsObj_free((void **)&qosObj);
      }
   }


   /* these are the hardcoded priority settings for various protocols */
   rutQos_doDefaultPolicy();

#ifdef SUPPORT_FCCTL
   if (!IS_EMPTY_STRING(intfName))
   {
       char cmd[BUFLEN_64];
       snprintf(cmd, sizeof(cmd), "fcctl flush --if %s --silent", intfName);

       rut_doSystemAction("rutQos_reconfigAllClassifications", cmd);
   }
#endif

   cmsLog_debug("Exit");

   return;
}


CmsRet rutQos_qMgmtClassDelete_dev2(const char *l3IntfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   char *l3FullPath=NULL;
   CmsRet ret;

   cmsLog_debug("Entered: l3IntfName=%s", l3IntfName);

   /* so we can still do path conversion on the intf that is being deleted */
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;
      ret = qdmIntf_intfnameToFullPathLocked(l3IntfName, FALSE, &l3FullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s to fullpath, ret=%d", l3IntfName, ret);
      }
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
   else
   {
      cmsLog_debug("l3IntfName %s ==> %s", l3IntfName, l3FullPath);
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      CmsRet r2;

      /*
       * If there is a match on the L3 ingress interface fullpath or the
       * L3 egress interface name, delete it.
       */
      if (!cmsUtl_strcmp(cObj->interface, l3FullPath) ||
          !cmsUtl_strcmp(cObj->X_BROADCOM_COM_egressInterface, l3IntfName))
      {
         r2 = cmsObj_deleteInstance(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Delete of Class instance at %d failed, ret=%d",
                         PEEK_INSTANCE_ID(&iidStack), r2);
         }
         else
         {
            /* since we deleted this instance, restore iidStack to last
             * non-deleted one.
             */
            iidStack = savedIidStack;
         }
      }

      cmsObj_free((void **) &cObj);

      /* save this iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(l3FullPath);

   return CMSRET_SUCCESS;
}


void rutQos_deleteClassByEgressQueueInstance_dev2(SINT32 queueInstance)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   CmsRet ret;

   cmsLog_debug("Entered: queueInstance=%d", queueInstance);

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      CmsRet r2;

      if (cObj->X_BROADCOM_COM_ClassQueue == queueInstance)
      {
         r2 = cmsObj_deleteInstance(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Delete of Class instance at %d failed, ret=%d",
                         PEEK_INSTANCE_ID(&iidStack), r2);
         }
         else
         {
            /* since we deleted this instance, restore iidStack to last
             * non-deleted one.
             */
            iidStack = savedIidStack;
         }
      }

      cmsObj_free((void **) &cObj);

      /* save this iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   return;
}


UBOOL8 rutQos_isClassificationChanged_dev2(const Dev2QosClassificationObject *newObj,
                                  const Dev2QosClassificationObject *currObj)
{
   /* Note this function does not check all the params in the
    * classification object.  Only those that directly affect how the
    * classification is configured into the kernel.
    */
   if (newObj->enable != currObj->enable ||
       newObj->ethertype != currObj->ethertype ||
       newObj->ethertypeExclude != currObj->ethertypeExclude ||
       cmsUtl_strcmp(newObj->interface, currObj->interface) ||
       cmsUtl_strcmp(newObj->destIP, currObj->destIP) ||
       cmsUtl_strcmp(newObj->destMask, currObj->destMask) ||
       newObj->destIPExclude != currObj->destIPExclude ||
       cmsUtl_strcmp(newObj->sourceIP, currObj->sourceIP) ||
       cmsUtl_strcmp(newObj->sourceMask, currObj->sourceMask) ||
       newObj->sourceIPExclude != currObj->sourceIPExclude ||
       newObj->protocol != currObj->protocol ||
       newObj->protocolExclude != currObj->protocolExclude ||
       newObj->destPort != currObj->destPort ||
       newObj->destPortRangeMax != currObj->destPortRangeMax ||
       newObj->destPortExclude != currObj->destPortExclude ||
       newObj->sourcePort != currObj->sourcePort ||
       newObj->sourcePortRangeMax != currObj->sourcePortRangeMax ||
       newObj->sourcePortExclude != currObj->sourcePortExclude ||
       cmsUtl_strcmp(newObj->destMACAddress, currObj->destMACAddress) ||
       cmsUtl_strcmp(newObj->destMACMask, currObj->destMACMask) ||
       newObj->destMACExclude != currObj->destMACExclude ||
       cmsUtl_strcmp(newObj->sourceMACAddress, currObj->sourceMACAddress) ||
       cmsUtl_strcmp(newObj->sourceMACMask, currObj->sourceMACMask) ||
       newObj->sourceMACExclude != currObj->sourceMACExclude ||
       cmsUtl_strcmp(newObj->sourceVendorClassID, currObj->sourceClientID) ||
       newObj->sourceVendorClassIDExclude != currObj->sourceVendorClassIDExclude ||
       cmsUtl_strcmp(newObj->sourceUserClassID, currObj->sourceUserClassID) ||
       newObj->sourceUserClassIDExclude != currObj->sourceUserClassIDExclude ||
       newObj->DSCPCheck != currObj->DSCPCheck ||
       newObj->DSCPExclude != currObj->DSCPExclude ||
       newObj->DSCPMark != currObj->DSCPMark ||
       newObj->ethernetPriorityCheck != currObj->ethernetPriorityCheck ||
       newObj->ethernetPriorityExclude != currObj->ethernetPriorityExclude ||
       newObj->ethernetPriorityMark != currObj->ethernetPriorityMark ||
       newObj->X_BROADCOM_COM_VLANIDTag != currObj->X_BROADCOM_COM_VLANIDTag ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_egressInterface, currObj->X_BROADCOM_COM_egressInterface) ||
       newObj->X_BROADCOM_COM_ClassQueue != currObj->X_BROADCOM_COM_ClassQueue ||
       newObj->X_BROADCOM_COM_ClassPolicer != currObj->X_BROADCOM_COM_ClassPolicer ||
       newObj->X_BROADCOM_COM_ClassRate != currObj->X_BROADCOM_COM_ClassRate)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


CmsRet rutQos_fillClassKeyArray_dev2(UINT32 *keyArray)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey < 1 || cObj->X_BROADCOM_COM_ClassKey > QOS_CLS_MAX_ENTRY)
      {
         cmsLog_error("Found invalid clsKey %d", cObj->X_BROADCOM_COM_ClassKey);
         ret = CMSRET_INTERNAL_ERROR;
         cmsObj_free((void **)&cObj);
         break;
      }

      keyArray[cObj->X_BROADCOM_COM_ClassKey - 1] = 1;
      cmsObj_free((void **)&cObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;
}


/** When rutQos_reconfigAllClassifications_dev2 is called with some
 *  Layer 2 or Layer 3 intfName, it needs to know if this intfName is
 *  referenced by any classifications, on either ingress or egress.  If so,
 *  we need to do reconfig.  (But if not, we can save ourselves some
 *  work and do nothing).
 *
 *  @intfName  (IN) Linux interface name to check
 *
 *  @return TRUE if given intfName is referenced by any classifications.
 */
UBOOL8 isAffectClassifications_dev2(const char *intfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   char *l2FullPath=NULL;
   char *l3FullPath=NULL;
   UBOOL8 referenced=FALSE;
   
   cmsLog_debug("Entered: intfName=%s", intfName);

   /* intfName may be L2 or L3, so do fullpath conversion on both */
   {
      /* so we can still do path conversion on the intf that is being deleted */
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;
      (void) qdmIntf_intfnameToFullPathLocked(intfName, TRUE, &l2FullPath);
      (void) qdmIntf_intfnameToFullPathLocked(intfName, FALSE, &l3FullPath);
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   cmsLog_debug("intfName %s ==> L2 %s", intfName, l2FullPath);
   cmsLog_debug("intfName %s ==> L3 %s", intfName, l3FullPath);


   while (!referenced &&
          cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &cObj) == CMSRET_SUCCESS)
   {
      if ((l2FullPath && !cmsUtl_strcmp(cObj->interface, l2FullPath)) ||
          (l3FullPath && !cmsUtl_strcmp(cObj->interface, l3FullPath)) ||
          !cmsUtl_strcmp(cObj->X_BROADCOM_COM_egressInterface, intfName))
      {
         cmsLog_debug("Affects [%s] ingress=%s egress=%s",
                      cObj->X_BROADCOM_COM_ClassName,
                      cObj->interface, cObj->X_BROADCOM_COM_egressInterface);
         referenced = TRUE;
      }

      cmsObj_free((void **)&cObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(l2FullPath);
   CMSMEM_FREE_BUF_AND_NULL_PTR(l3FullPath);

   return referenced;
}


UBOOL8 rutQos_isAnotherClassPolicerExist_dev2(UINT32 excludeClsKey,
                                              const char *egressL2IntfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   UBOOL8 exist=FALSE;

   cmsLog_debug("Entered: egressL2IntfName=%s exclude=%d", egressL2IntfName, excludeClsKey);

   /* strangely, the algorithm for determining another policer does not
    * look at policer properties.  It only looks for matching egress
    * L2 IntfName.
    */
   while (!exist &&
          cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &cObj) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey != excludeClsKey &&
          cObj->enable &&
          !cmsUtl_strcmp(cObj->status, MDMVS_ENABLED) &&
          cObj->X_BROADCOM_COM_ClassQueue > 0)
      {
         CmsRet r2;
         char l2IntfName[CMS_IFNAME_LENGTH]={0};

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->X_BROADCOM_COM_ClassQueue,
                                             NULL, NULL, l2IntfName);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QueueInfo on %d, ret=%d",
                          cObj->X_BROADCOM_COM_ClassQueue, r2);
         }
         else
         {
            if (cmsUtl_strcmp(egressL2IntfName, l2IntfName) == 0)
            {
               cmsLog_debug("Found exist l2IntfName %s at %d",
                             l2IntfName, cObj->X_BROADCOM_COM_ClassQueue);
               exist = TRUE;
            }
         }
      }

      cmsObj_free((void **)&cObj);
   }

   return exist;
}

CmsRet rutQos_fillPolicerInfo_dev2(const SINT32 instance, const UINT32 policerInfo)
{
    Dev2QosPolicerObject *pObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;
    
    if (instance <= 0)
    {
        cmsLog_error("invalid instance=%d", instance);
        return CMSRET_INVALID_ARGUMENTS;
    }
    
    cmsLog_debug("policerUpdateIndex insance %d, policerInfo %d.", instance, policerInfo);
    /* the fullpath of the Policer table is Device.QoS.Policer.{i}.
     * so we just need to push the instance number into the first position
     * of the instance id stack.
     */
    PUSH_INSTANCE_ID(&iidStack, instance);
    if ((ret = cmsObj_get(MDMOID_DEV2_QOS_POLICER, &iidStack, 0, (void **) &pObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_get <MDMOID_DEV2_QOS_POLICER> returns error. ret=%d", ret);
        return ret;
    }
    
    pObj->X_BROADCOM_COM_PolicerInfo = policerInfo;
    
    if ((ret = cmsObj_set((void *)pObj, &iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_set <MDMOID_DEV2_QOS_POLICER> returns error. ret=%d", ret);	  
    }
    
    cmsObj_free((void **) &pObj);	
    return ret;
}

UBOOL8 rutQos_isAnotherClassRateLimitExist_dev2(UINT32 excludeClsKey,
                                                const char *egressL2IntfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   UBOOL8 exist=FALSE;

   cmsLog_debug("Entered: egressL2IntfName=%s exclude=%d", egressL2IntfName, excludeClsKey);

   while (!exist &&
          cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &cObj) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey != excludeClsKey &&
          cObj->X_BROADCOM_COM_ClassRate != QOS_RESULT_NO_CHANGE &&
          cObj->enable &&
          !cmsUtl_strcmp(cObj->status, MDMVS_ENABLED) &&
          cObj->X_BROADCOM_COM_ClassQueue > 0)
      {
         CmsRet r2;
         char l2IntfName[CMS_IFNAME_LENGTH]={0};

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->X_BROADCOM_COM_ClassQueue,
                                             NULL, NULL, l2IntfName);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QueueInfo on %d, ret=%d",
                          cObj->X_BROADCOM_COM_ClassQueue, r2);
         }
         else
         {
            if (cmsUtl_strcmp(egressL2IntfName, l2IntfName) == 0)
            {
               cmsLog_debug("Found exist l2IntfName %s at %d",
                             l2IntfName, cObj->X_BROADCOM_COM_ClassQueue);
               exist = TRUE;
            }
         }
      }

      cmsObj_free((void **)&cObj);
   }

   return exist;
}

void rutQos_sendTcpPureAckConfigChanged_dev2(const char *l2FullPath)
{
   char msgBuf[sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody) + 128] = {0};  // max possible buf
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   char intfName[CMS_IFNAME_LENGTH] = {0};
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   void *msgHandle = cmsMdm_getThreadMsgHandle();

   if (IS_EMPTY_STRING(l2FullPath))
   {
      cmsLog_error("NULL or empty l2FullPath");
      return;
   }

   cmsLog_debug("Entered: l2FullPath=%s", l2FullPath);

   // convert l2Fullpath to intfName.
   ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(l2FullPath,
                                                intfName, sizeof(intfName));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s to intfName, ret=%d", l2FullPath, ret);
      return;
   }

   // Fill in common fields of the CMS msg.
   msgHdr->src = cmsMsg_getHandleEid(msgHandle);
   msgHdr->flags_event = 1;

   if (cmsMdm_isCmsClassic() ||
       cmsMdm_isBdkSysmgmt())
   {
      // Send msg directly to ssk.  The intfName follows the header.
      msgHdr->dst = EID_SSK;
      msgHdr->type = CMS_MSG_TCP_PURE_ACK_CONFIG_CHANGED;
      msgHdr->dataLength = strlen(intfName)+1;
      strcpy((char *)(msgHdr+1), intfName);
      cmsLog_debug("direct send to ssk with %s", intfName);
   }
   else
   {
      // publish a key to sys_directory, which will trigger a notification
      // to ssk.
      PubSubKeyValueMsgBody *kvBody = (PubSubKeyValueMsgBody *)(msgHdr+1);
      char *timeStr = (char *)(kvBody+1);

      snprintf(kvBody->key, sizeof(kvBody->key), "%s:%s",
               BDK_KEY_TCP_PURE_ACK_CONFIG_CHANGED_PREFIX, intfName);
      cmsTms_getXSIDateTime(0, timeStr, 128);
      kvBody->valueLen = strlen(timeStr)+1;

      msgHdr->dst = EID_SSK;  // this will get sent to the bus manager, which is dsl_hal_thread in DSL component
      msgHdr->type = CMS_MSG_PUBLISH_EVENT;
      msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
      msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody) + kvBody->valueLen;
      cmsLog_debug("Publishing [%s=%s]", kvBody->key, timeStr);
   }

   ret = cmsMsg_send(msgHandle, msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send msg, ret=%d", ret);
   }

   return;
}

void rutQos_reconfigTcpPureAckRulesLayer2_dev2(const char *l2FullPath)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;
   Dev2IpInterfaceObject *ipIntf = NULL;
   char *ipIntfFullPath = NULL;
   char higherLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN] = {0};
   char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN] = {0};
   CmsRet ret;

   /* Loop all IP interfaces to find the matched layer 2 interface. */
   while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack, (void **)&ipIntf) == CMSRET_SUCCESS)
   {
      /* Currently, TCP Pure-ACK is supported on WAN interfaces. */
      if (!ipIntf->X_BROADCOM_COM_Upstream)
      {
         cmsObj_free((void **)&ipIntf);  
         continue;
      }

      /* Get the full path of this IP interface. */
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
      pathDesc.iidStack = iidStack;
      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &ipIntfFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. ret=%d", ret);
         cmsObj_free((void **)&ipIntf);
         return;
      }

      /* Find the layer 2 interface of this IP interface. */
      cmsUtl_strncpy(lowerLayerBuf, ipIntfFullPath, sizeof(lowerLayerBuf));
      while(!qdmIntf_isFullPathLayer2Locked_dev2(lowerLayerBuf) && ret == CMSRET_SUCCESS)
      {
         cmsUtl_strncpy(higherLayerBuf, lowerLayerBuf, sizeof(higherLayerBuf));
         memset(lowerLayerBuf, 0, sizeof(lowerLayerBuf));
         ret = qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(higherLayerBuf, lowerLayerBuf, sizeof(lowerLayerBuf));
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not convert %s, ret=%d", higherLayerBuf, ret);
            CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
            cmsObj_free((void **)&ipIntf);
            return;
         }
         if (IS_EMPTY_STRING(lowerLayerBuf))
         {
            cmsLog_error("Did not find layer2 intf before hitting empty lowerLayer (higher=%s)", higherLayerBuf);
            CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
            cmsObj_free((void **)&ipIntf);
            return;
         }
      }

      if(cmsUtl_strcmp(l2FullPath, lowerLayerBuf) == 0)
      {
         /* The layer 2 interface is matched with input.
          * Configure TCP Pure-ACK rules for this IP interface.
          */
         rutQos_reconfigTcpPureAckRulesLayer3_dev2(ipIntfFullPath, QOS_COMMAND_CONFIG);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
      cmsObj_free((void **)&ipIntf);
   }

   return;
}  /* End of rutQos_reconfigTcpPureAckRulesLayer2_dev2() */	

void rutQos_reconfigTcpPureAckRulesLayer3_dev2(const char *l3FullPath, QosCommandType cmdType)
{
#ifdef SUPPORT_NF_NAT
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosQueueObject *qObj = NULL;
   CmsQosQueueInfo qInfo;
   FILE *fpTcpPureAck;
   UBOOL8 prevHideObjectsPendingDelete;
   char higherLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   int qPrio, tcpPureAckPrio = MAX_QOS_LEVELS;
   char ifName[CMS_IFNAME_LENGTH];
   char tcpPureAckScript[BUFLEN_64];
   char cmd[BUFLEN_1024];
   CmsRet ret = CMSRET_SUCCESS;

   /* Find the layer 2 interface of this IP interface. */
   cmsUtl_strncpy(lowerLayerBuf, l3FullPath, sizeof(lowerLayerBuf));
   while(!qdmIntf_isFullPathLayer2Locked_dev2(lowerLayerBuf) && ret == CMSRET_SUCCESS)
   {
      cmsUtl_strncpy(higherLayerBuf, lowerLayerBuf, sizeof(higherLayerBuf));
      memset(lowerLayerBuf, 0, sizeof(lowerLayerBuf));
      ret = qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(higherLayerBuf, lowerLayerBuf, sizeof(lowerLayerBuf));
      if(ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s, ret=%d", higherLayerBuf, ret);
         return;
      }
      if(IS_EMPTY_STRING(lowerLayerBuf))
      {
         /* We cannot find the layer 2 interface of this IP interface. Return silently. */
         cmsLog_debug("Did not find layer2 intf before hitting empty lowerLayer (higher=%s)", higherLayerBuf);
         return;
      }
   }
   
   if(!qdmIntf_isLayer2FullPathUpstreamLocked_dev2(lowerLayerBuf))
   {
      cmsLog_debug("Currently, TCP Pure-ACK rules are only for upstream. %s is not.", lowerLayerBuf);
      return;
   }

   /* Hide the objects pending for delete, so that we can remove TCP Pure-ACK
    * rules correctly when configuration is changed.
    */
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = TRUE;

   /* Get the lowest priority level of all TCP Pure-ACK queues within this interface. */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while(cmsObj_getNext(MDMOID_DEV2_QOS_QUEUE, &iidStack, (void **)&qObj) == CMSRET_SUCCESS)
   {
      if(qObj->enable && (cmsUtl_strcmp(qObj->interface, lowerLayerBuf) == 0))
      {
         memset(&qInfo, 0, sizeof(qInfo));
         qdmQos_convertDmQueueObjToCmsQueueInfoLocked(qObj, &qInfo);
         if(qInfo.tcpPureAckQueue)
         {
            if(!cmsUtl_strncmp(qInfo.intfName, ATM_IFC_STR, strlen(ATM_IFC_STR)) ||
                !cmsUtl_strncmp(qInfo.intfName, PTM_IFC_STR, strlen(PTM_IFC_STR)))
            {
               qPrio = XTM_QOS_LEVELS - qInfo.queuePrecedence;
            }
            else if(!cmsUtl_strncmp(qInfo.intfName, ETH_IFC_STR, strlen(ETH_IFC_STR)))
            {
               qPrio = ETHWAN_QOS_LEVELS - qInfo.queuePrecedence;
            }
            else
            {
               cmsLog_debug("TCP Pure-ACK rules for interface %s is not supported yet.", qInfo.intfName);               
               cmsObj_free((void **)&qObj);
               return;
            }
            if(qPrio < tcpPureAckPrio)
            {
               tcpPureAckPrio = qPrio;
            }
         }
      }
      cmsObj_free((void **)&qObj);
   }

   /* Restore the configuration of hiding objects pending for delete. */
   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   /* We need the Linux interface name for iptables and ebtables use. */
   qdmIntf_fullPathToIntfnameLocked_dev2(l3FullPath, ifName);

   /* Prepare the netfilter script for TCP Pure-ACK rules.*/
   snprintf(tcpPureAckScript, sizeof(tcpPureAckScript), "/var/tcp_pure_ack_%s.sh", ifName);

   fpTcpPureAck = fopen(tcpPureAckScript, "w");
   if (fpTcpPureAck == NULL) {
      cmsLog_error("%s: unable to open file\n", tcpPureAckScript);
      return;
   }

#ifndef SUPPORT_NF_TABLES
   /* Delete old TCP Pure-ACK rules first.*/
   /* Bridging */
   fprintf(fpTcpPureAck, "ebtables --concurrent -t nat -D POSTROUTING -o %s --tcp-pureack -j tcp-pureack-%s\n", ifName, ifName);
   fprintf(fpTcpPureAck, "ebtables --concurrent -t nat -F tcp-pureack-%s\n", ifName);
   fprintf(fpTcpPureAck, "ebtables --concurrent -t nat -X tcp-pureack-%s\n", ifName);

   /* Routing and Local-out */
   fprintf(fpTcpPureAck, "iptables -w -t mangle -D POSTROUTING -o %s -p tcp --tcp-pureack -j tcp-pureack-%s\n", ifName, ifName);
   fprintf(fpTcpPureAck, "iptables -w -t mangle -F tcp-pureack-%s\n", ifName);
   fprintf(fpTcpPureAck, "iptables -w -t mangle -X tcp-pureack-%s\n", ifName);

#ifdef SUPPORT_IPV6
   fprintf(fpTcpPureAck, "ip6tables -w -t mangle -D POSTROUTING -o %s -p tcp --tcp-pureack -j tcp-pureack-%s\n", ifName, ifName);
   fprintf(fpTcpPureAck, "ip6tables -w -t mangle -F tcp-pureack-%s\n", ifName);
   fprintf(fpTcpPureAck, "ip6tables -w -t mangle -X tcp-pureack-%s\n", ifName);
#endif

   /* Add new TCP Pure-ACK rules. */
   if((cmdType == QOS_COMMAND_CONFIG) && (tcpPureAckPrio > 0) && (tcpPureAckPrio < MAX_QOS_LEVELS))
   {
      /* Bridging */
      fprintf(fpTcpPureAck, "ebtables --concurrent -t nat -N tcp-pureack-%s\n", ifName);
      fprintf(fpTcpPureAck, "ebtables --concurrent -t nat -A POSTROUTING -o %s --tcp-pureack -j tcp-pureack-%s\n", ifName, ifName);
      fprintf(fpTcpPureAck, "ebtables --concurrent -t nat -A tcp-pureack-%s -j mark --mark-or 0x%x\n", ifName, tcpPureAckPrio);

      /* Routing and Local-out */
      fprintf(fpTcpPureAck, "iptables -w -t mangle -N tcp-pureack-%s\n", ifName);
      fprintf(fpTcpPureAck, "iptables -w -t mangle -A POSTROUTING -o %s -p tcp --tcp-pureack -j tcp-pureack-%s\n", ifName, ifName);
      fprintf(fpTcpPureAck, "iptables -w -t mangle -A tcp-pureack-%s -j MARK --or-mark 0x%x\n", ifName, tcpPureAckPrio);

#ifdef SUPPORT_IPV6
      fprintf(fpTcpPureAck, "ip6tables -w -t mangle -N tcp-pureack-%s\n", ifName);
      fprintf(fpTcpPureAck, "ip6tables -w -t mangle -A POSTROUTING -o %s -p tcp --tcp-pureack -j tcp-pureack-%s\n", ifName, ifName);
      fprintf(fpTcpPureAck, "ip6tables -w -t mangle -A tcp-pureack-%s -j MARK --or-mark 0x%x\n", ifName, tcpPureAckPrio);
#endif
   }
#else
   /* Delete old TCP Pure-ACK rules first.*/
   /* Bridging */
   fprintf(fpTcpPureAck, "nft 'delete chain inet filter tcp-pureack-%s'\n", ifName);

   /* Routing and Local-out */
   fprintf(fpTcpPureAck, "nft 'delete chain bridge br_filter tcp-pureack-%s'\n", ifName);

   /* Add new TCP Pure-ACK rules. */
   if((cmdType == QOS_COMMAND_CONFIG) && (tcpPureAckPrio > 0) && (tcpPureAckPrio < MAX_QOS_LEVELS))
   {
      /* Bridging */
      fprintf(fpTcpPureAck, "nft 'add chain bridge br_filter tcp-pureack-%s { type filter hook postrouting priority filter; policy accept; }'\n", ifName);
      fprintf(fpTcpPureAck, "nft 'add rule bridge br_filter tcp-pureack-%s oifname %s bcm_ext tcp-pure-ack counter meta mark set mark or 0x1'\n", ifName, ifName);

      /* Routing and Local-out */
      fprintf(fpTcpPureAck, "nft 'add chain inet filter tcp-pureack-%s { type filter hook postrouting priority mangle; policy accept; }'\n", ifName);
      fprintf(fpTcpPureAck, "nft 'add rule inet filter tcp-pureack-%s oifname %s bcm_ext tcp-pure-ack counter meta mark set mark or 0x1'\n", ifName, ifName);      
   }
#endif /* SUPPORT_NF_TABLES */

   fclose(fpTcpPureAck);

   /* Execute the netfilter script for TCP Pure-ACK rules. */
   snprintf(cmd, sizeof(cmd), "sh %s 2>/dev/null", tcpPureAckScript);
   rut_doSystemAction("rutQos_reconfigTcpPureAckRulesLayer3_dev2", cmd);
   snprintf(cmd, sizeof(cmd), "rm -f %s 2>/dev/null", tcpPureAckScript);
   rut_doSystemAction("rutQos_reconfigTcpPureAckRulesLayer3_dev2", cmd);
#endif /* SUPPORT_NF_NAT */

   return;
}  /* End of rutQos_reconfigTcpPureAckRulesLayer3_dev2() */

#endif  /* DMP_DEVICE2_QOS_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

