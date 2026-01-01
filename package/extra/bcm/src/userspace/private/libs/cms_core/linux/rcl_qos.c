/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_msg_pubsub.h"
#include "cms_qos.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_iptables.h"
#include "ethswctl_api.h"
#include "bcm/bcmswapitypes.h"
#include <board.h>
#ifdef SUPPORT_RDP
#include "rdpactl_api.h"
#endif

typedef enum
{
   ORDER_COMPACTION  = 0,
   ORDER_INSERTION   = 1

} ClsReorderType;

#define SET_EXISTING(n, c) \
   (((n) != NULL && (c) != NULL))


#ifdef DMP_QOS_1

/** local functions **/
static CmsRet rclQos_classReorder(const _QMgmtClassificationObject *clsObj, const InstanceIdStack *iidStack, ClsReorderType action);
static CmsRet rclQos_setClassQueueObject(SINT32 queueInstance);
static CmsRet rclQos_setQueueMgmtObject(_QMgmtObject *qMgmtObj);
static CmsRet rclQos_doPolicy(const _QMgmtClassificationObject *newClsObj);
static CmsRet rclQos_removePolicy(void);



CmsRet rcl_qMgmtObject( _QMgmtObject *newObj,
                        const _QMgmtObject *currObj,
                        const InstanceIdStack *iidStack __attribute__((unused)),
                        char **errorParam __attribute__((unused)),
                        CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      /* for single instance object, this is the start-up config */
      if (newObj->enable)
      {
         rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_CONFIG, newObj->defaultDSCPMark);
      }
#if 0
      if ((ret = rclQos_setQueueMgmtObject(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rclQos_setQueueMgmtObject returns error. ret=%d", ret);
         return ret;
      }
#endif
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance */

      /* if queue management enable is changed, we want to config/unconfig
       * all the classification rules in ebtable or iptable.  We also want to
       * config/unconfig all the queues in ATM.
       */
      if (currObj->enable)
      {
         if (!newObj->enable || (newObj->defaultDSCPMark != currObj->defaultDSCPMark))
         {
            rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_UNCONFIG, currObj->defaultDSCPMark);
         }
      }
      if (newObj->enable)
      {
         if (!currObj->enable || (newObj->defaultDSCPMark != currObj->defaultDSCPMark))
         {
            rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_CONFIG, newObj->defaultDSCPMark);
         }
      }
      if (newObj->enable != currObj->enable)
      {
         /* config or unconfig each queues in the queue table */
         if ((ret = rclQos_setQueueMgmtObject(newObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_setQueueMgmtObject returns error. ret=%d", ret);
            return ret;
         }

         /* if queue management enable is changed, simply config/unconfig 
          * all the classification rules here instead of doing that inside 
          * each of classification object RCL in order to have 
          * better performance
          */
         if (newObj->X_BROADCOM_COM_EnableStateChanged)
         {
            /* First unconfig all the class rules. */
            if ((ret = rclQos_classUnconfig(NULL)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rclQos_classUnconfig returns error. ret=%d", ret);
               return ret;
            }

            /* Then config all the class rules. */
            if ((ret = rclQos_classConfig(NULL, TRUE)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rclQos_classConfig returns error. ret=%d", ret);
               return ret;
            }

            /* set X_BROADCOM_COM_EnableStateChanged to FALSE in the end */
            newObj->X_BROADCOM_COM_EnableStateChanged = FALSE;
         }

         /* Reconfig QoS port shaping for all Ethernet interfaces */
         rutQos_portShapingConfigAll();
      }
   }

   return CMSRET_SUCCESS;

}  /* End of rcl_qMgmtObject() */

CmsRet rcl_qMgmtQueueObject( _QMgmtQueueObject *newObj,
                             const _QMgmtQueueObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      /* either start-up config or adding new object instance */
      /* we don't need to take any action at this point for either cases.
       * for start-up config, we will take action when the interface is up.
       * for adding new object instance, we will take action when the
       * parameters of the new object instance is set.
       */
      rut_modifyNumQMgmtQueue(iidStack, 1);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance or set a new instance */
      InstanceIdStack iidStack2;
      _QMgmtObject *qMgmtObj = NULL;
      UBOOL8 enableStateChanged = FALSE;

      INIT_INSTANCE_ID_STACK(&iidStack2);
      if ((ret = cmsObj_get(MDMOID_Q_MGMT, &iidStack2, 0, (void **)&qMgmtObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get <MDMOID_Q_MGMT> returns error. ret=%d", ret);
         return ret;
      }

      if (newObj->queueEnable && strstr(newObj->queueInterface, "WLANConfiguration") == NULL)
      {
         /* for non-wlan interface queue, if queue management is not enabled,
          * queue can not be enabled.
          */
         newObj->queueEnable = qMgmtObj->enable;
      }

      /* we are in the case of switched on/off QoS from web if X_BROADCOM_COM_EnableStateChanged is set */
      enableStateChanged = qMgmtObj->X_BROADCOM_COM_EnableStateChanged;

      cmsObj_free((void **)&qMgmtObj); /* no longer needed */

      /* unconfig the curr queue object instance */
      if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_UNCONFIG, currObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueConfig returns error. ret=%d", ret);
         return ret;
      }

      /* config the queue again based on newObj */
      if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_CONFIG, newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueConfig returns error. ret=%d", ret);
         return ret;
      }

      /* don't configure any classifiers here when enableStateChanged is TRUE,
       * we will handle it outside this RCL, see rcl_qMgmtObject().
       */
      if (!enableStateChanged)
      {
         /* also configure associated classifiers */
         if ((ret = rclQos_setClassQueueObject(iidStack->instance[0])) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_setClassQueueObject returns error. ret=%d", ret);
            return ret;
         }
      }
   }
   else if (currObj != NULL)
   {
      rut_modifyNumQMgmtQueue(iidStack, -1);

      /* delete curr instance */
      /* before unconfig the queue, we want to delete all the classifications that refer to it */
      if ((ret = rutQos_qMgmtClassOperation(iidStack->instance[iidStack->currentDepth - 1],
                                            TRUE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtClassOperation returns error. ret=%d", ret);
         return ret;
      }

      /* now unconfig the curr queue object instance */
      if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_UNCONFIG, currObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueConfig returns error. ret=%d", ret);
         return ret;
      }
   }

   return CMSRET_SUCCESS;

}  /* End of rcl_qMgmtQueueObject() */

CmsRet rcl_qMgmtQueueStatsObject( _QMgmtQueueStatsObject *newObj __attribute__((unused)),
                const _QMgmtQueueStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

#ifdef HW_POLICING
#define POLICER_ENABLE_NEW_OR_ENABLE_EXISTING(n, c) \
   (((n) != NULL && (n)->policerEnable && (c) == NULL) || \
   ((n) != NULL && (n)->policerEnable && (c) != NULL && !((c)->policerEnable)))
#define POLICER_DELETE_OR_DISABLE_EXISTING(n, c) \
   (((n) == NULL) || \
   ((n) != NULL && !((n)->policerEnable) && (c) != NULL && (c)->policerEnable))
#define POLICER_DISABLE_EXISTING(n, c) \
   ((n) != NULL && !((n)->policerEnable) && (c) != NULL && (c)->policerEnable)
CmsRet rcl_qMgmtPolicerObject(_QMgmtPolicerObject *newObj,
                              const _QMgmtPolicerObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;
   UBOOL8 isRefered = FALSE;
   CmsQosPolicerInfo policer;	
   CmsQosPolicerInfo* pInfo = &policer;	

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (ADD_NEW(newObj, currObj))
   {
      /* either start-up config or adding new object instance */
      /* we don't really need to take any action at this point for either cases.
       * for start-up config, we will take action when the interface is up.
       * for adding new object instance, we will take action when the new object instance is set.
       */
      rut_modifyNumQMgmtPolicer(iidStack, 1);
   }

   if ((newObj) && (newObj->policerEnable))
   {
      /* copy info from pObj to pInfo struct */
      pInfo->enable = newObj->policerEnable;
      pInfo->committedRate = newObj->committedRate;
      pInfo->committedBurstSize = newObj->committedBurstSize;
      pInfo->excessBurstSize = newObj->excessBurstSize;
      pInfo->peakRate = newObj->peakRate;
      pInfo->peakBurstSize = newObj->peakBurstSize;
      cmsUtl_strncpy(pInfo->meterType, newObj->meterType, sizeof(pInfo->meterType));
      cmsUtl_strncpy(pInfo->conformingAction, newObj->conformingAction, sizeof(pInfo->conformingAction));
      cmsUtl_strncpy(pInfo->partialConformingAction, newObj->partialConformingAction, sizeof(pInfo->partialConformingAction));
      cmsUtl_strncpy(pInfo->nonConformingAction, newObj->nonConformingAction, sizeof(pInfo->nonConformingAction));
      cmsUtl_strncpy(pInfo->name, newObj->X_BROADCOM_COM_PolicerName, sizeof(pInfo->name));
      pInfo->policerInfo = newObj->X_BROADCOM_COM_PolicerInfo;

      if (POLICER_ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
      {
         rutQos_createPolicer(pInfo);
         newObj->X_BROADCOM_COM_PolicerInfo = pInfo->policerInfo;
      }
      else if (newObj != NULL && currObj != NULL && newObj->policerEnable)//edit a enabled policer
      {
         rutQos_updatePolicer(pInfo);
      }
   }

   if (POLICER_DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutQos_referenceCheck(MDMOID_Q_MGMT_POLICER,
         iidStack->instance[iidStack->currentDepth - 1],
         &isRefered)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_referenceCheck returns error. ret=%d", ret);
         return ret;
      }
      if (isRefered)
      {
         cmsLog_error("Policer can not be disabled. It is being refered by a Class rule.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      rutQos_deletePolicer(currObj->X_BROADCOM_COM_PolicerInfo);

      if (POLICER_DISABLE_EXISTING(newObj, currObj))
      {
         newObj->X_BROADCOM_COM_PolicerInfo = 0;
      }
      else if (DELETE_EXISTING(newObj, currObj))
      {
         rut_modifyNumQMgmtPolicer(iidStack, -1);
      }
   }

   return CMSRET_SUCCESS;
}  /* End of rcl_qMgmtPolicerObject() */
#else
CmsRet rcl_qMgmtPolicerObject(_QMgmtPolicerObject *newObj,
                              const _QMgmtPolicerObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;
   UBOOL8 isRefered = FALSE;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      /* either start-up config or adding new object instance */
      /* we don't really need to take any action at this point for either cases.
       * for start-up config, we will take action when the interface is up.
       * for adding new object instance, we will take action when the new object instance is set.
       */
      rut_modifyNumQMgmtPolicer(iidStack, 1);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance or set a new instance */
      if (currObj->policerEnable && !newObj->policerEnable)
      {
         if ((ret = rutQos_referenceCheck(MDMOID_Q_MGMT_POLICER,
                                   iidStack->instance[iidStack->currentDepth - 1],
                                   &isRefered)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_referenceCheck returns error. ret=%d", ret);
            return ret;
         }
         if (isRefered)
         {
            cmsLog_error("Policer can not be disabled. It is being refered by a Class rule.");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
   }
   else
   {
      /* delete curr instance */
      if ((ret = rutQos_referenceCheck(MDMOID_Q_MGMT_POLICER,
                                iidStack->instance[iidStack->currentDepth - 1],
                                &isRefered)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_referenceCheck returns error. ret=%d", ret);
         return ret;
      }
      if (isRefered)
      {
         cmsLog_error("Policer can not be deleted. It is being refered by a Class rule.");
         return CMSRET_INVALID_ARGUMENTS;
      }
      rut_modifyNumQMgmtPolicer(iidStack, -1);

   }

   return CMSRET_SUCCESS;

}  /* End of rcl_qMgmtPolicerObject() */
#endif

CmsRet rcl_qMgmtClassificationObject( _QMgmtClassificationObject *newObj,
                                      const _QMgmtClassificationObject *currObj,
                                      const InstanceIdStack *iidStack,
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;
   SINT32 needReorder  = 1;
   UINT32 highestOrder = 0;
   InstanceIdStack iidStack2;
   _QMgmtClassificationObject *cObj = NULL;

   cmsLog_debug("enter");

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      /* either start-up config or adding new object instance */
      /* we don't really need to take any action at this point for either cases.
       * for start-up config, we will take action when the interface is up.
       * for adding new object instance, we will take action when the new object instance is set.
       */
      rut_modifyNumQMgmtClassification(iidStack, 1);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance */
      cmsLog_debug("edit current instance");

      /* get the highest order */
      INIT_INSTANCE_ID_STACK(&iidStack2);
      while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack2, (void **)&cObj)) == CMSRET_SUCCESS)
      {
         /* skip the target instance */
         if (iidStack->instance[iidStack->currentDepth - 1] == iidStack2.instance[iidStack2.currentDepth - 1])
         {
            cmsObj_free((void **)&cObj);
            continue;
         }
         if (cObj->classificationOrder > highestOrder)
         {
            highestOrder = cObj->classificationOrder;
         }
         cmsObj_free((void **)&cObj);
      }
      if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getNext returns error. ret=%d", ret);
         return ret;
      }

      /* highest order dont't need reorder*/
      if (newObj->classificationOrder > highestOrder)
      {
         cmsLog_debug("Don't reorder the table.");
         needReorder = 0;
      }

      if (needReorder)
      {
         /* first we want to unconfig all the classification rules */
         if ((ret = rclQos_classUnconfig(NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_classUnconfig returns error. ret=%d", ret);
            return ret;
         }

         /* if this is an edit operation, we want to do class order compaction before insertion */
         if (currObj->classificationOrder > 0)
         {
            if ((ret = rclQos_classReorder(currObj, iidStack, ORDER_COMPACTION)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rclQos_classReorder <ORDER_COMPACTION> returns error. ret=%d", ret);
               return ret;
            }
         }
         /* after editing a classification object instance, we need to reorder the classification table */
         if ((ret = rclQos_classReorder(newObj, iidStack, ORDER_INSERTION)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_classReorder <ORDER_INSERTION> returns error. ret=%d", ret);
            return ret;
         }

         /* finally, we want to re-config all the classification rules */
         if ((ret = rclQos_classConfig(newObj, TRUE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_classConfig returns error. ret=%d", ret);
            return ret;
         }
      }
      else
      {
         /* just unconfig then config single rules */
         if ((ret = rclQos_classUnconfig(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_classUnconfig returns error. ret=%d", ret);
            return ret;
         }
			
         if ((ret = rclQos_classConfig(newObj, FALSE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rclQos_classConfig returns error. ret=%d", ret);
            return ret;
         }
      }
   }
   else
   {
      /* delete curr instance */
      /* unconfig curr classification object instance */
      cmsLog_debug("delete current instance");
	
      rut_modifyNumQMgmtClassification(iidStack, -1);

      if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, currObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_UNCONFIG> returns error. ret=%d", ret);
         return ret;
      }
	
      /* after deleting a classification object instance, we need to reorder the classification table */
      if ((ret = rclQos_classReorder(currObj, iidStack, ORDER_COMPACTION)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rclQos_classReorder <ORDER_COMPACTION> returns error. ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of rcl_qMgmtClassificationObject() */


#ifdef DMP_QOSDYNAMIC_FLOW_1
CmsRet rcl_qMgmtAppObject( _QMgmtAppObject *newObj __attribute__((unused)),
                const _QMgmtAppObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_qMgmtFlowObject( _QMgmtFlowObject *newObj __attribute__((unused)),
                const _QMgmtFlowObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}
#endif /* DMP_QOSDYNAMIC_FLOW_1 */

CmsRet rclQos_classConfig(const _QMgmtClassificationObject *newObj, UBOOL8 isAll)
{
   InstanceIdStack iidStack;
   _QMgmtObject *qMgmtObj = NULL;
   CmsRet ret;

   cmsLog_debug("enter");

   /* make sure that all the required modules for qos support are loaded */
   rutIpt_qosLoadModule();

   /* set the class queue status of the new class object */
   if (isAll)
   {
	   if ((ret = rclQos_doPolicy(newObj)) != CMSRET_SUCCESS)
	   {
	      cmsLog_error("rclQos_doPolicy returns error. ret=%d", ret);
	      return ret;
	   }
   }
   else
   {
      if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_CONFIG, newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_CONFIG> returns error. ret=%d", ret);
         return ret;
      }
   }

#ifdef BRCM_WLAN
   /* add wireless default policy last */
   {
   QMgmtQueueObject *qObj = NULL;
   char qIntf[MDM_SINGLE_FULLPATH_BUFLEN]={0};

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_Q_MGMT_QUEUE, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&qObj)) == CMSRET_SUCCESS)
   {
      if (strstr(qObj->queueInterface, "WLANConfiguration") != NULL)
      {
         if (strcmp(qObj->queueInterface, qIntf) != 0)
         {
            char wlIfcname[CMS_IFNAME_LENGTH]={0};

            if ((ret = rut_fullPathToIntfname(qObj->queueInterface, wlIfcname)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rut_fullPathToIntfname returns error. ret=%d", ret);
               cmsObj_free((void **)&qObj);
               return ret;
            }
            strcpy(qIntf, qObj->queueInterface);
            rutQos_doDefaultWlPolicy(wlIfcname, qObj->queueEnable);
         }
      }
      cmsObj_free((void **)&qObj);
   }
   }
#endif

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_Q_MGMT, &iidStack, 0, (void **)&qMgmtObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_Q_MGMT> returns error. ret=%d", ret);
      return ret;
   }

   /* to ensure that command for default value will be added in the last line */
   if (qMgmtObj->enable)
   {
      rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_UNCONFIG, qMgmtObj->defaultDSCPMark);
      rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_CONFIG, qMgmtObj->defaultDSCPMark);
   }
   cmsObj_free((void **)&qMgmtObj);

   rutQos_doDefaultPolicy();

   return CMSRET_SUCCESS;

}  /* End of rclQos_classConfig() */

/* NULL point is to unconfig all */
CmsRet rclQos_classUnconfig(const _QMgmtClassificationObject *cObj)
{
   CmsRet ret;

   cmsLog_debug("enter");

   if (cObj)
   {
      if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, cObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_UNCONFIG> returns error. ret=%d", ret);
         return ret;
      }
   }
   else
   {
      if ((ret = rclQos_removePolicy()) != CMSRET_SUCCESS)
      {
         cmsLog_error("rclQos_removePolicy returns error. ret=%d", ret);
         return ret;
      }
   }
		
   return CMSRET_SUCCESS;

}  /* End of rclQos_classUnconfig() */


/*********************************************************************************
 *
 * All the functions below this point are static local functions.
 * They probably should get their prefix removed or use some other prefix.
 * the rclQos_ prefix is confusing.  They are not rcl handler functions.
 *
 *********************************************************************************/

CmsRet rclQos_classReorder(const _QMgmtClassificationObject *clsObj, const InstanceIdStack *iidStack, ClsReorderType action)
{
   InstanceIdStack iidStack2;
   _QMgmtClassificationObject *cObj = NULL;
   UINT32 clsInstance = iidStack->instance[iidStack->currentDepth - 1];
   CmsRet ret;

   cmsLog_debug("enter");

   if (action == ORDER_INSERTION)
   {
      UINT32 highestOrder = 0;

      /* if 'order' is higher than all the classification orders in the classification table,
       * there is no need to reorder the table to make room for the insertion.
       */
      /* find the highest classification order in the classification table */
      INIT_INSTANCE_ID_STACK(&iidStack2);
      while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack2, (void **)&cObj)) == CMSRET_SUCCESS)
      {
         /* skip the target instance */
         if (clsInstance == iidStack2.instance[iidStack2.currentDepth - 1])
         {
            cmsObj_free((void **)&cObj);
            continue;
         }
         if (cObj->classificationOrder > highestOrder)
         {
            highestOrder = cObj->classificationOrder;
         }
         cmsObj_free((void **)&cObj);
      }
      if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getNext returns error. ret=%d", ret);
         return ret;
      }

      if (clsObj->classificationOrder > highestOrder)
      {
         cmsLog_debug("Order is higher than all entries in the table.  There is no need to reorder the table.");
         return CMSRET_SUCCESS;
      }
   }

   /* now, reorder the classification table */
   INIT_INSTANCE_ID_STACK(&iidStack2);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack2, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      /* skip the target instance */
      if (clsInstance == iidStack2.instance[iidStack2.currentDepth - 1])
      {
         cmsObj_free((void **)&cObj);
         continue;
      }
      if (cObj->classificationOrder >= clsObj->classificationOrder)
      {
         if (action == ORDER_COMPACTION)
         {
            cObj->classificationOrder--;
         }
         else  /* order insertion */
         {
            /* shift order up to open an entry for 'order' */
            cObj->classificationOrder++;
         }

         if ((ret = cmsObj_setFlags((void *)cObj, &iidStack2, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_setFlags returns error. ret=%d", ret);
            cmsObj_free((void **)&cObj);
            return ret;
         }
      }
      cmsObj_free((void **)&cObj);
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rclQos_classReorder() */

CmsRet rclQos_setClassQueueObject(SINT32 queueInstance)
{
   InstanceIdStack iidStack;
   _QMgmtClassificationObject *cObj = NULL;
   CmsRet ret;

   cmsLog_debug("enter");

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cObj->classQueue == queueInstance)
      {
         cmsLog_debug("found matching class object");

         /* set the classification object. this will cause a call back to rcl_qMgmtClassificationObject() */
         if ((ret = cmsObj_set(cObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set returns error. ret=%d", ret);
            cmsObj_free((void **)&cObj);
            return ret;
         }
      }
      cmsObj_free((void **)&cObj);
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rclQos_setClassQueueObject() */

CmsRet rclQos_setQueueMgmtObject(_QMgmtObject *qMgmtObj)
{
   InstanceIdStack iidStack;
   _QMgmtQueueObject *qObj = NULL;
   CmsRet ret;

   cmsLog_debug("enter");

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      if (strstr(qObj->queueInterface, "WLANConfiguration") == NULL)
      {
         /* for non-wlan interface queue, if queue management is not enabled,
          * queue can not be enabled.
          */
         qObj->queueEnable = qMgmtObj->enable;
      }

      /* set the queue object. this will cause a call back to rcl_qMgmtQueueObject() */
      if ((ret = cmsObj_set(qObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set returns error. ret=%d", ret);
         cmsObj_free((void **)&qObj);
         return ret;
      }
      cmsObj_free((void **)&qObj);
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_QUEUE> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rclQos_setQueueMgmtObject() */


CmsRet rclQos_doPolicy(const _QMgmtClassificationObject *newClsObj)
{
   InstanceIdStack iidStack;
   QMgmtClassificationObject *cObj = NULL;
   UBOOL8 done = FALSE;
   UINT32 order;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("enter");

   for (order = 1; !done; order++)
   {
      done = TRUE;

      if (newClsObj != NULL && newClsObj->classificationOrder == order)
      {
         done = FALSE;

         if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_CONFIG, newClsObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_CONFIG> returns error. ret=%d", ret);
            return ret;
         }
         continue;
      }

      INIT_INSTANCE_ID_STACK(&iidStack);
      while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
      {
         if (cObj->classificationOrder > order)
         {
            done = FALSE;
         }
         else if (cObj->classificationOrder == order)
         {
            done = FALSE;

            if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_CONFIG, cObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_CONFIG> returns error. ret=%d", ret);
               cmsObj_free((void **)&cObj);
               return ret;
            }
            cmsObj_free((void **)&cObj);
            break;
         }
         cmsObj_free((void **)&cObj);
      }
      if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
         return ret;
      }
   }

   return CMSRET_SUCCESS;

}  /* End of rclQos_doPolicy() */

CmsRet rclQos_removePolicy(void)
{
   InstanceIdStack iidStack;
   QMgmtClassificationObject *cObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("enter");

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, cObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_UNCONFIG> returns error. ret=%d", ret);
         cmsObj_free((void **)&cObj);
         return ret;
      }
      cmsObj_free((void **)&cObj);
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rclQos_removePolicy() */

#endif /* DMP_QOS_1 */

#ifdef DMP_X_BROADCOM_COM_TM_1
#define GET_WAN_TYPE_VALUE_AND_LENGTH(local_buffer, length_in_bytes) do                                      \
    {                                                                                                        \
        length_in_bytes = cmsPsp_get(RDPA_WAN_TYPE_PSP_KEY, local_buffer, sizeof(local_buffer) - 1);         \
        if (0 >= length_in_bytes)                                                                            \
        {                                                                                                    \
            local_buffer[0] = 0;                                                                             \
        }                                                                                                    \
        else                                                                                                 \
        {                                                                                                    \
            local_buffer[length_in_bytes] = 0;                                                               \
        }                                                                                                    \
    } while (0)
#define MATCH_PON(_buf) ((cmsUtl_strcasestr(_buf,"PON") != NULL) ||  (cmsUtl_strcasestr(_buf,"XGS") != NULL))

CmsRet rcl_bCMTrafficManagementObject( _BCMTrafficManagementObject *newObj,
                const _BCMTrafficManagementObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 carModeAction = FALSE;

   // To be technically correct, this RCL handler should only be executed
   // by 1 component in the system.  If this is the full BDK system, then only
   // sysmgmt should execute config actions.  All other components have
   // "inert" mirror copies of the sysmgmt object.  But if we just have a
   // Broadcom DSL component in the system, e.g. a RDK system, then the DSL
   // component should do the config.  But what if we have 2 components,
   // e.g. DSL and Wifi?  Which component should config then?  Fortunately,
   // the lower level driver seems to be ok with getting configured multiple
   // times (with the same data).

   if (ADD_NEW(newObj, currObj))
   {
#if (defined(GPON_HGU) || defined(EPON_HGU))
      carModeAction = TRUE;
#endif

      /* If the value is undefined, set to the default value. */
      if ((newObj->owner != TMCTL_OWNER_BH) &&
        (newObj->owner != TMCTL_OWNER_FE))
      {
          char buf[16] = {};
          int count;

          GET_WAN_TYPE_VALUE_AND_LENGTH(buf, count);
          if (MATCH_PON(buf)) /* PON WAN */
          {
              newObj->owner = TMCTL_OWNER_BH;
          }
          else
          {
              newObj->owner = TMCTL_OWNER_FE; /* GbE, DSL, AE, etc */     
          }
      }

      if ((newObj->qidPrioMap != QID_PRIO_MAP_Q0P7) &&
        (newObj->qidPrioMap != QID_PRIO_MAP_Q7P7))
      {
#ifdef BCM_PON
          newObj->qidPrioMap = QID_PRIO_MAP_Q0P7;
#else
          newObj->qidPrioMap = QID_PRIO_MAP_Q7P7;
#endif
      }
   }
   else if (SET_EXISTING(newObj, currObj))
   {
#if (defined(GPON_HGU) || defined(EPON_HGU))
      carModeAction = TRUE;
#endif
   }

   if (carModeAction == TRUE)
   {
      if (rdpactl_has_rdpa_port_type_gpon(NULL) || rdpactl_has_rdpa_port_type_epon(NULL))
      {
          if (newObj->owner == TMCTL_OWNER_BH)
          {
              rdpactl_set_sys_car_mode(FALSE);
          }
          else
          {
              rdpactl_set_sys_car_mode(TRUE);
          }
      }
   }
   else
   {
       rdpactl_set_sys_car_mode(FALSE);
   }

   // Regardless of ADD_NEW (startup) or set, if this is the sys_mgmt component,
   // publish this obj to sys_directory so other components can see the values.
   if (newObj &&
       (0 == strcmp(mdmShmCtx->compName, BDK_COMP_SYSMGMT)))
   {
      char valueBuf[64]={0};
      snprintf(valueBuf, sizeof(valueBuf), "%d,%d",
               newObj->owner, newObj->qidPrioMap);
      rut_sendKeyValueEventToSysDirectory(BDK_KEY_X_BROADCOM_COM_TM, valueBuf);
   }

   return CMSRET_SUCCESS;
}
#endif /* DMP_X_BROADCOM_COM_TM_1 */

