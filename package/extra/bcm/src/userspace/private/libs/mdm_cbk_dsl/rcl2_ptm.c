/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
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

#ifdef DMP_DEVICE2_PTMLINK_1

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
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ptm.h"
#include "rut_atm.h"
#include "rut2_ptm.h"
#include "rut_pmirror.h"
#include "rut_qos.h"
#include "rut_diag.h"

// in rut2_ip.c in cms_core lib
extern void rutIp_sendPropgateStatusExMsg(CmsEntityId dstEid,
                             MdmObjectId oid, const InstanceIdStack *iidStack,
                             const char *lowerLayer, const char *status);


CmsRet rcl_dev2PtmObject( _Dev2PtmObject *newObj __attribute__((unused)),
                      const _Dev2PtmObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2PtmLinkObject( _Dev2PtmLinkObject *newObj,
                          const _Dev2PtmLinkObject *currObj,
                          const InstanceIdStack *iidStack  __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 addIntf = FALSE;
   UBOOL8 enableExisting = FALSE;
   UBOOL8 deleteIntf = FALSE;
   UBOOL8 disableExisting = FALSE;
   UBOOL8 isRealDel = FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   char cmdStr[BUFLEN_128];

   cmsLog_debug("Entered: currObj=%p newObj=%p", currObj, newObj);

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumPtmLink(1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumPtmLink(-1);
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Fill ifName if it is NULL (from TR69 or other non httpd apps) */
      if (newObj->name == NULL)
      {
         /* create the layer 2 ifName */
         if ((ret = rutptm_fillL2IfName_dev2(PTM_EOA, &(newObj->name))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDsl_fillL2IfName failed. error=%d", ret);
            return ret;
         }  
         cmsLog_debug("L2IfName=%s", newObj->name);
      }
      if (newObj->lowerLayers == NULL)
      {
         if ((ret = rutptm_fillLowerLayer(&(newObj->lowerLayers))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDsl_fillLowerLayer failed. error=%d", ret);
            return ret;
         }  
         cmsLog_debug("LowerLayer=%s", newObj->lowerLayers);
      }
      addIntf = TRUE;
      if (ENABLE_EXISTING(newObj, currObj))
      {
         cmsLog_debug("transition from not enabled to enabled (enableExisting)");
         enableExisting = TRUE;
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* delete or disable L2 interface */
      deleteIntf = TRUE;
      if (DELETE_EXISTING(newObj, currObj))
      {
         isRealDel = TRUE;
      }
      else if (DISABLE_EXISTING(newObj, currObj))
      {
         cmsLog_debug("transition from ENABLED to NOT enabled (disableExisting)");
         disableExisting = TRUE;
      }
   }   
   else if (newObj && currObj) 
   {
      cmsLog_debug("currObj-status=%s ==> new status=%s",
                   currObj->status, newObj->status);

      // Curr status is not "Up", and new is "Up", this happens when ssk does
      // status propagation on the interface stack.  It's counter-intuitive:
      // ssk sets status to UP, and then the RCL configures the driver to bring
      // ptm interface up.  Also make sure object is enabled since SQA likes
      // to manually disable this object during testing.
      if (cmsUtl_strcmp(currObj->status, MDMVS_UP) && 
          !cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
          newObj->enable)
      {
         /* for checking dsl link up and status is not "UP" case */
         cmsLog_debug("Status transition to UP on existing and enabled intf %s (addIntf)", newObj->name);
         addIntf = TRUE;
      }
      else if (!cmsUtl_strcmp(currObj->status, MDMVS_UP) && 
               cmsUtl_strcmp(newObj->status, MDMVS_UP))
      {
         /* if old status is "Up", and new is not "Up", need to delete the layer 2 interface */
         cmsLog_debug("Status transition to DOWN on existiing intf %s (deleteIntf)", newObj->name);
         deleteIntf = TRUE;
      }
   }
   
   if (addIntf)
   {
      /* is the channel up ? */
      if (qdmDsl_isAnyLowerLayerChannelUpLocked_dev2(newObj->lowerLayers) == FALSE)
      {
         cmsLog_debug("Lowerlayers (%s) is not up yet, so just return", newObj->lowerLayers);
         return CMSRET_SUCCESS;
      }
      cmsLog_debug("Lowerlayers (%s) is already up! addIntf %s", newObj->lowerLayers, newObj->name);

      if (enableExisting)
      {
         CmsEntityId destEid = (cmsMdm_isCmsClassic()) ? EID_SSK : EID_DSL_SSK;
         rutIp_sendPropgateStatusExMsg(destEid, MDMOID_DEV2_PTM_LINK, iidStack,
                                       newObj->lowerLayers, MDMVS_UP);
         return CMSRET_SUCCESS;
      }

      // This section actually configures the PTM interface.
      if ((ret = rutptm_setConnCfg_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutptm_setConnCfg failed. error=%d", ret);
         return ret;
      }
      if ((ret = rutptm_createInterface_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutptm_createInterface failed. error=%d", ret);
         return ret;      
      }

      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", newObj->name);
      rut_doSystemAction("rcl_wanPtmLinkCfgObject: ifconfig L2IfName up", (cmdStr));
      // RCL should not set status.  It is done by ssk during intfStack propagation.

      {
         char statusStr[BUFLEN_16]={0};
         char hwAddr[BUFLEN_32]={0};

         if (rut_getIfStatusHwaddr(newObj->name, statusStr, hwAddr) == CMSRET_SUCCESS)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
         }
      }

#ifdef BRCM_CMS_BUILD
      /* remove this ifdef when this is resolved in BDK */
      
      /* 1. portmirroring configuration: and the OID should be tied to the 
         dsl block or gpon block or whatever WAN that uses it? */
      
      /* If the interface is used for port mirroring, enable this feature when link is up */
      rutPMirror_enablePortMirrorIfUsed(newObj->name);
#endif      

#ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
      rutEthOam_setService(TRUE, newObj->name);
#endif

      /* Activate queues on this L2 intf */
      rutQos_reconfigAllQueuesOnLayer2Intf_dev2(newObj->name);

   }
   else if (deleteIntf)
   {
      // 3 possible ways we get into this block:
      // (a) SQA manually sets enable to FALSE (disableExisting)
      // (b) the PTM link object is being deleted (isRealDel)
      // (c) lower layer down (DSL line is down), ssk propagates DOWN on interface stack.

      if (disableExisting)
      {
         CmsEntityId destEid = (cmsMdm_isCmsClassic()) ? EID_SSK : EID_DSL_SSK;
         rutIp_sendPropgateStatusExMsg(destEid, MDMOID_DEV2_PTM_LINK, iidStack,
                                       newObj->lowerLayers, MDMVS_DOWN);
         return CMSRET_SUCCESS;
      }

      if (isRealDel)
      {
         /*
          * This PTM intf is being deleted.  Delete all QoS queues
          * defined for this interface.  When the QoS queue is deleted,
          * rcl_dev2QosQueueObject will delete all associated classifiers.
          */
         rutQos_deleteQueues_dev2(currObj->name);
      }
      else
      {
         /* link is down, unconfig queues on this L2 intf */
         rutQos_reconfigAllQueuesOnLayer2Intf_dev2(currObj->name);
      }

#ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
      rutEthOam_setService(FALSE, currObj->name);
#endif

      /* Only delete L2 interface if it was UP  */
      if (!cmsUtl_strcmp(currObj->status, MDMVS_UP))
      {
         if ((ret = rutptm_deleteInterface_dev2(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutptm_deleteInterface failed. error=%d", ret);
         }
         if ((ret = rutptm_deleteConnCfg_dev2(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutptm_deleteConnCfg failed. error=%d", ret);
         }
      }

      // We are not really deleting this PTM link object, just config it DOWN.
      if (!isRealDel)
      {
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down 2>/dev/null", newObj->name);
         rut_doSystemAction("rcl_ptmLinkObject: ifconfig L2IfName down", (cmdStr));
         // RCL should not set status.  It is done by ssk during intfStack propagation.
      }
   }

   return ret;
}

CmsRet rcl_dev2PtmLinkStatsObject( _Dev2PtmLinkStatsObject *newObj __attribute__((unused)),
                               const _Dev2PtmLinkStatsObject *currObj __attribute__((unused)),
                               const InstanceIdStack *iidStack __attribute__((unused)),
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

#endif   /* DMP_DEVICE2_PTMLINK_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
