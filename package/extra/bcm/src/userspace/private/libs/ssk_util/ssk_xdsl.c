/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom 
   All Rights Reserved

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

/*!\file ssk_dsl.c
 * \brief This file has the DSL functions for libssk_util.so.
 */

#include <string.h>

#include "cms_log.h"
#include "ssk_util.h"
#include "cms_dal.h"
#ifdef SUPPORT_DSL
#include "AdslMibDef.h"
#include "devctl_adsl.h"
#include "devctl_xtm.h"
#include "bcmxtmcfg.h"
#endif
#include "bcmnetlink.h"


/** In bonding mode, need to keep track of the previous link status for each xDsl link
 *  to compare with the current xDsl links to make a determination on weather the wan
 * link is up or need to be tear down.
 */
typedef struct
{
   UBOOL8 isCurrLine0Up;     /**< current  primary line link state */
   UBOOL8 isNewLine0Up;      /**< new primary line link state  */
   UBOOL8 isCurrLine1Up;     /**< current secondary line link state */
   UBOOL8 isNewLine1Up;      /**< new secondary line link state */
} DslBondingLinkInfo;

DslBondingLinkInfo atmDslBondingLinksStateTable={FALSE, FALSE, FALSE, FALSE};
DslBondingLinkInfo ptmDslBondingLinksStateTable={FALSE, FALSE, FALSE, FALSE};

UBOOL8 dslDiagInProgress = FALSE;
#ifdef DMP_DSLDIAGNOSTICS_1
dslLoopDiag dslLoopDiagInfo = {FALSE, EMPTY_INSTANCE_ID_STACK,0};
#endif

#ifdef DMP_X_BROADCOM_COM_SELT_1
dslDiag dslDiagInfo = {FALSE, 0, 0, 0};
#endif

#ifdef DMP_ADSLWAN_1
extern void xdslUtil_IntfCfgInit(adslCfgProfile *pAdslCfg, WanDslIntfCfgObject *pDlIntfCfg);
#endif

#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
static void fixATMPvcStatusLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack, UBOOL8 wanLinkUp);
#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
static UBOOL8 isBondedAtmLinkUp(void);
void displayBondingStateTable(const char *heading, DslBondingLinkInfo *xDslBondingLinkStatetbl);
#endif
#endif

#ifdef DMP_PTMWAN_1
static void fixPtmChannelStatusLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack, UBOOL8 wanLinkUp);
#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
static UBOOL8 isBondedPtmLinkUp(void);
#endif
static void informErrorSampleStatusChangeLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack);
#endif


#ifdef DMP_BASELINE_1

/* called from updateLinkStatus_dev2 */
void checkDslLinkStatusLocked_igd(void *appMsgHandle __attribute__((unused)),
                             WanLinkInfo *wanLinkInfo  __attribute__((unused)))
{

#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
      if (wanLinkInfo->isATM)
      {
         WanDslIntfCfgObject *dslIntfCfg=NULL;
         UBOOL8 changed=FALSE;
         UBOOL8 atmWanLinkUp=FALSE;
         CmsRet ret;

         if ((ret = cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, &(wanLinkInfo->iidStack), 0, (void **) &dslIntfCfg))
                              == CMSRET_SUCCESS)
         {
#ifndef DMP_X_BROADCOM_COM_DSLBONDING_1
            /* This is for no bonding atm wan link status */
            atmWanLinkUp =  (cmsUtl_strcmp(dslIntfCfg->status, MDMVS_UP)==0) ? TRUE : FALSE;  
            
#else  /* BONDING section */

            UBOOL8 bothLinkStateChange=FALSE;

            displayBondingStateTable("Before read the xDsl driver", &atmDslBondingLinksStateTable);

            /* First get the new line 0 link state */
            atmDslBondingLinksStateTable.isNewLine0Up = (cmsUtl_strcmp(dslIntfCfg->status, MDMVS_UP)==0) ? TRUE : FALSE;  

            /* get new secondary line (1) */
            atmDslBondingLinksStateTable.isNewLine1Up = isBondedAtmLinkUp();

            /* Now, compare with the previous (curr) link stats to decide
            * if the both links is changed at the same time or not.  If both changed, and only one of the 2 lines is up, 
            * atmWanLinkUp should not be up since there could be  a time that both are down and wan link need 
            * to be tear down.  If both lines change and both lines are up, the wan link should be up.
            */
            if ((atmDslBondingLinksStateTable.isNewLine0Up != atmDslBondingLinksStateTable.isCurrLine0Up) &&
                (atmDslBondingLinksStateTable.isNewLine1Up != atmDslBondingLinksStateTable.isCurrLine1Up))
            {
               bothLinkStateChange = TRUE;
            }

            displayBondingStateTable("after read  the xDsl driver", &atmDslBondingLinksStateTable);
            cmsLog_notice("bothLinkStateChange %d", bothLinkStateChange);

            if (!bothLinkStateChange &&
                (atmDslBondingLinksStateTable.isNewLine0Up || atmDslBondingLinksStateTable.isNewLine1Up))
            {               
               atmWanLinkUp = TRUE;
            }

            if (bothLinkStateChange &&
                (atmDslBondingLinksStateTable.isNewLine0Up && atmDslBondingLinksStateTable.isNewLine1Up))
            {               
               atmWanLinkUp = TRUE;
            }
            
            /* save the new in curr link stats */
            atmDslBondingLinksStateTable.isCurrLine0Up = atmDslBondingLinksStateTable.isNewLine0Up;
            atmDslBondingLinksStateTable.isCurrLine1Up = atmDslBondingLinksStateTable.isNewLine1Up;
            
#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */

            if (!wanLinkInfo->isUp && atmWanLinkUp)
            {
               /* ATM wan link went from down to up */

               wanLinkInfo->isUp = TRUE;
               changed = TRUE;
               printf("(%s) xDSL link up, Connection Type: ATM\n", _myAppName);
               matchRdpaWanType("DSL");
            }
            else if (wanLinkInfo->isUp && !atmWanLinkUp)
            {
               /* ATM wan link went from up to down */

               wanLinkInfo->isUp = FALSE;
               changed = TRUE;
               printf("(%s) xDSL ATM link down.\n", _myAppName);
            }
            else
            {
               cmsLog_debug("no change in ATM link status, %d", wanLinkInfo->isUp);
            }

            cmsObj_free((void **) &dslIntfCfg);

            if (changed)
            {
               /* For layer 2 link state: down -> up, need to work on layer 2 object first in fixATMPvcStatusLocked
               * then layer 3 object in updateWanConnStatusInSubtreeLocked
               * For layer 2 link state: up -> down, work on layer 3 first then layer 2 
               */
               if (wanLinkInfo->isUp)
               {
                  fixATMPvcStatusLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
                  updateWanConnStatusInSubtreeLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
               }
               else
               {
                  updateWanConnStatusInSubtreeLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
                  fixATMPvcStatusLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
               }            
            }
         }
         else
         {
            cmsLog_error("cmsObj_get MDMOID_WAN_DSL_INTF_CFG returns error %d", ret);
         }
      }
#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */


#ifdef DMP_PTMWAN_1
      if (wanLinkInfo->isPTM)
      {
         WanDslIntfCfgObject *dslIntfCfg=NULL;
         UBOOL8 changed=FALSE;
         UBOOL8 ptmWanLinkUp=FALSE;
         CmsRet ret;

         if ((ret = cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, &(wanLinkInfo->iidStack), 0, (void **) &dslIntfCfg))
                              == CMSRET_SUCCESS)
         {

#ifndef DMP_X_BROADCOM_COM_DSLBONDING_1
            /* This is for no bonding ptm wan link status */
            ptmWanLinkUp =  (cmsUtl_strcmp(dslIntfCfg->status, MDMVS_UP)==0) ? TRUE : FALSE;  
            
#else   /* BONDING section */

            UBOOL8 bothLinkStateChange=FALSE;

            displayBondingStateTable("Before read the xDsl driver", &ptmDslBondingLinksStateTable);

            /* First get the new line 0 link state */
            ptmDslBondingLinksStateTable.isNewLine0Up = (cmsUtl_strcmp(dslIntfCfg->status, MDMVS_UP)==0) ? TRUE : FALSE;

            /* get new secondary line (1) */
            ptmDslBondingLinksStateTable.isNewLine1Up = isBondedPtmLinkUp();

            /* Now, compare with the previous (curr) link stats to decide
            * if the both links is changed at the same time or not.  If both changed, and only one of the 2 lines is up, 
            * ptmWanLinkUp should not be up since there could be  a time that both are down and wan link need 
            * to be tear down.  If both lines change and both lines are up, the wan link should be up.
            */
            if ((ptmDslBondingLinksStateTable.isNewLine0Up != ptmDslBondingLinksStateTable.isCurrLine0Up) &&
                (ptmDslBondingLinksStateTable.isNewLine1Up != ptmDslBondingLinksStateTable.isCurrLine1Up))
            {
               bothLinkStateChange = TRUE;
            }

            displayBondingStateTable("after read  the xDsl driver", &ptmDslBondingLinksStateTable);
            cmsLog_notice("bothLinkStateChange %d", bothLinkStateChange);

            if (!bothLinkStateChange &&
                (ptmDslBondingLinksStateTable.isNewLine0Up || ptmDslBondingLinksStateTable.isNewLine1Up))
            {               
               ptmWanLinkUp = TRUE;
            }

            if (bothLinkStateChange &&
                (ptmDslBondingLinksStateTable.isNewLine0Up && ptmDslBondingLinksStateTable.isNewLine1Up))
            {               
               ptmWanLinkUp = TRUE;
            }
            
            /* save the new in curr link stats */
            ptmDslBondingLinksStateTable.isCurrLine0Up = ptmDslBondingLinksStateTable.isNewLine0Up;
            ptmDslBondingLinksStateTable.isCurrLine1Up = ptmDslBondingLinksStateTable.isNewLine1Up;
            
#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */

            if (!wanLinkInfo->isUp && ptmWanLinkUp)
            {
               /* PTM wan link went from down to up */

               wanLinkInfo->isUp = TRUE;
               changed = TRUE;
               printf("(%s) xDSL link up, Connection Type: PTM\n", _myAppName);
               matchRdpaWanType("DSL");
            }
            else if (wanLinkInfo->isUp && !ptmWanLinkUp)
            {
               /* PTM wan link went from up to down */

               wanLinkInfo->isUp = FALSE;
               changed = TRUE;
               printf("(%s) xDSL PTM link down.\n", _myAppName);
            }
            else
            {
               cmsLog_debug("no change in PTM link status, %d", wanLinkInfo->isUp);
            }

            if (dslIntfCfg->errorSamplesAvailable)
            {
              informErrorSampleStatusChangeLocked(appMsgHandle, &(wanLinkInfo->iidStack));
            }
            
            cmsObj_free((void **) &dslIntfCfg);

            if (changed)
            {
               /* For layer 2 link state: down -> up, need to work on layer 2 object first in fixPtmChannelStatusLocked
               * then layer 3 object in updateWanConnStatusInSubtreeLocked
               * For layer 2 link state: up -> down, work on layer 3 first then layer 2 
               */
               if (wanLinkInfo->isUp)
               {
                  fixPtmChannelStatusLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
                  updateWanConnStatusInSubtreeLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
               }
               else
               {
                  updateWanConnStatusInSubtreeLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
                  fixPtmChannelStatusLocked(appMsgHandle, &(wanLinkInfo->iidStack), wanLinkInfo->isUp);
               }
            }
         }
         else
         {
            cmsLog_error("cmsObj_get MDMOID_WAN_DSL_INTF_CFG returns error %d", ret);
         }
      }
#endif /* DMP_PTMWAN_1 */

}

#endif  /* DMP_BASELINE_1 */


/* For the 63138 and 63148, implement a workaround to strip bytes and
 * allow OAM traffic due to JIRA HW63138-12
 */
#if defined (CONFIG_BCM963138) || defined (CONFIG_BCM963148)

/** Stub routine to run when the OAM strip byte workaround is
 *  activated and deactivated.
 */
void processOamStripByte(UINT32 msgId)
{
    /* If parameter is zero, workaround is activated.  If not, it is deactivated. */
    if(msgId == 0)
    {
        cmsLog_debug("OAM Strip Byte Workaround Activated");
    }
    else
    {
        cmsLog_debug("OAM Strip Byte Workaround Deactivated");
    }

   return;
}

#endif




#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
void fixATMPvcStatusLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack, UBOOL8 wanLinkUp)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanDslLinkCfgObject *dslLinkCfg = NULL;

   /*
    * mwang: hmm, we have to start the dslLinkCfg, which is perVCC, which goes into the
    * rcl handler function.  THis could be done better.
    */
   while (cmsObj_getNextInSubTree(MDMOID_WAN_DSL_LINK_CFG, parentIidStack, &iidStack, (void **)&dslLinkCfg) == CMSRET_SUCCESS)
   {
      cmsLog_debug("wanLinkUp=%d, dslLinkCfg->enable=%d, dslLinkCfg->linkStatus=%s", 
                   wanLinkUp, dslLinkCfg->enable, dslLinkCfg->linkStatus);

      if (dslLinkCfg->enable)
      {
         /* if dslLinkCfg is enabled and dsl link is up and dslLinkCfg->linkStatus is not "UP", start lay2 interface */
         if (wanLinkUp && cmsUtl_strcmp(dslLinkCfg->linkStatus, MDMVS_UP))
         {
            CMSMEM_REPLACE_STRING(dslLinkCfg->linkStatus, MDMVS_UP);
            cmsLog_debug("Activate PVC on %s", dslLinkCfg->X_BROADCOM_COM_IfName);
            ret = cmsObj_set(dslLinkCfg, &iidStack);

            cmsLog_debug("send link up message2");
            sendStatusMsgToSmd(appMsgHandle, CMS_MSG_WAN_LINK_UP, dslLinkCfg->X_BROADCOM_COM_IfName);
         }
         else if (!wanLinkUp && !cmsUtl_strcmp(dslLinkCfg->linkStatus, MDMVS_UP))
         {
            CMSMEM_REPLACE_STRING(dslLinkCfg->linkStatus, MDMVS_DOWN);
            cmsLog_debug("Deactivate PVC on %s", dslLinkCfg->X_BROADCOM_COM_IfName);
            ret = cmsObj_set(dslLinkCfg, &iidStack);

            cmsLog_debug("send link up message3");
            sendStatusMsgToSmd(appMsgHandle, CMS_MSG_WAN_LINK_DOWN, dslLinkCfg->X_BROADCOM_COM_IfName);
         }
         if (ret != CMSRET_SUCCESS)
         {
            cmsObj_free((void **) &dslLinkCfg);
            cmsLog_error("Failed to set PVC status, error=%d", ret);
            return;
         }            
      }
      else
      {
         cmsLog_debug("dslLinkCfg is not enabled");
      }
         
      cmsObj_free((void **) &dslLinkCfg);
   }
}

#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */

#ifdef DMP_PTMWAN_1

void informErrorSampleStatusChangeLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPtmLinkCfgObject *ptmLinkCfg = NULL;
 
   cmsLog_debug("Inform Error sample status");
   while (cmsObj_getNextInSubTree(MDMOID_WAN_PTM_LINK_CFG, parentIidStack, &iidStack, (void **)&ptmLinkCfg) == CMSRET_SUCCESS)
   {
      if (ptmLinkCfg->enable)
      {
        sendStatusMsgToSmd(appMsgHandle, CMS_MSG_WAN_ERRORSAMPLES_AVAILABLE, ptmLinkCfg->X_BROADCOM_COM_IfName);
        cmsLog_debug("ErrorSamplesAvailable event received");
      }
      cmsObj_free((void **) &ptmLinkCfg);
      if (ret != CMSRET_SUCCESS)
      {
        cmsLog_error("Failed to set ptmLinkCfg. ret=%d", ret);
        return;
      }
   }
}

void fixPtmChannelStatusLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack, UBOOL8 wanLinkUp)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPtmLinkCfgObject *ptmLinkCfg = NULL;
 
   while (cmsObj_getNextInSubTree(MDMOID_WAN_PTM_LINK_CFG, parentIidStack, &iidStack, (void **)&ptmLinkCfg) == CMSRET_SUCCESS)
   {
      cmsLog_debug("wanLinkUp=%d, ptmLinkCfg->enable=%d, ptmLinkCfg->linkStatus=%s",
                   wanLinkUp, ptmLinkCfg->enable, ptmLinkCfg->linkStatus);

      if (ptmLinkCfg->enable)
      {
         /* if ptmLinkCfg is enabled and dsl link is up and ptmLinkCfg->linkStatus is not "UP", start xtm operation */
         if (ptmLinkCfg->enable && wanLinkUp && cmsUtl_strcmp(ptmLinkCfg->linkStatus, MDMVS_UP))
         {
            CMSMEM_REPLACE_STRING(ptmLinkCfg->linkStatus, MDMVS_UP);
            cmsLog_debug("Activate PTM channel %s", ptmLinkCfg->X_BROADCOM_COM_IfName);
            ret = cmsObj_set(ptmLinkCfg, &iidStack);

            cmsLog_debug("send link up message4");
            sendStatusMsgToSmd(appMsgHandle, CMS_MSG_WAN_LINK_UP,
                               ptmLinkCfg->X_BROADCOM_COM_IfName);
         }
         else if (!wanLinkUp && !cmsUtl_strcmp(ptmLinkCfg->linkStatus, MDMVS_UP))
         {
            CMSMEM_REPLACE_STRING(ptmLinkCfg->linkStatus, MDMVS_DOWN);
            cmsLog_debug("Deactivate PTM channel %s", ptmLinkCfg->X_BROADCOM_COM_IfName);
            ret = cmsObj_set(ptmLinkCfg, &iidStack);

            cmsLog_debug("send link up message5");
            sendStatusMsgToSmd(appMsgHandle, CMS_MSG_WAN_LINK_DOWN,
                               ptmLinkCfg->X_BROADCOM_COM_IfName);
         }

         if (ret != CMSRET_SUCCESS)
         {
            cmsObj_free((void **) &ptmLinkCfg);
            cmsLog_error("Failed to set ptmLinkCfg. ret=%d", ret);
            return;
         }
      }

      cmsObj_free((void **) &ptmLinkCfg);
   }
}
#endif /* DMP_PTMWAN_1 */


void getDslDiagResults(void *appMsgHandle __attribute__((unused)))
{
#ifdef DMP_DSLDIAGNOSTICS_1
   if (dslLoopDiagInfo.dslLoopDiagInProgress == TRUE)
   {
      getDslLoopDiagResults(appMsgHandle);
   }
#endif   
#ifdef DMP_X_BROADCOM_COM_SELT_1
   if (dslDiagInfo.dslDiagInProgress == TRUE)
   {
      getDslSeltDiagResults(appMsgHandle);
   }
#endif
}

#ifdef DMP_DSLDIAGNOSTICS_1
void processWatchDslLoopDiag_igd(CmsMsgHeader *msg)
{
   WanDslDiagObject *dslDiagObj;
   dslDiagMsgBody *info = (dslDiagMsgBody*) (msg+1);
   InstanceIdStack iidStack = info->iidStack;
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
   {
      if (cmsObj_get(MDMOID_WAN_DSL_DIAG, &iidStack, 0, (void **) &dslDiagObj) == CMSRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING(dslDiagObj->loopDiagnosticsState,MDMVS_REQUESTED);
         if ((ret = cmsObj_set(dslDiagObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set returns error. ret=%d", ret);
         }
         else
         {
            dslDiagInProgress = TRUE;
            dslLoopDiagInfo.dslLoopDiagInProgress = TRUE;
            dslLoopDiagInfo.iidStack = info->iidStack;
            dslLoopDiagInfo.pollRetries = 0;
            dslLoopDiagInfo.src = msg->src;
         }
         cmsObj_free((void **) &dslDiagObj);
      }
      cmsLck_releaseLock();
   } /* aquire lock ok */
}


void getDslLoopDiagResults_igd(void *appMsgHandle)
{
   WanDslDiagObject *dslDiagObj;
   InstanceIdStack iidStack = dslLoopDiagInfo.iidStack;
   WanDslIntfCfgObject *dslCfgObj=NULL;
   UINT32 lineId=0;
   
   cmsLog_debug("Enter: dslLoopDiagInfo.pollRetries %d, inProgress %d",dslLoopDiagInfo.pollRetries,
                dslLoopDiagInfo.dslLoopDiagInProgress);

   if ((cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
   {   
      if (cmsObj_get(MDMOID_WAN_DSL_DIAG, &iidStack, 0, (void **) &dslDiagObj) == CMSRET_SUCCESS)
      {
         if ((cmsUtl_strcmp(dslDiagObj->loopDiagnosticsState, MDMVS_REQUESTED) == 0) &&
             (dslLoopDiagInfo.pollRetries < DIAG_DSL_LOOPBACK_TIMEOUT_PERIOD))
         {
            dslLoopDiagInfo.pollRetries++;
         }
         else
         {
            if (dslLoopDiagInfo.pollRetries >= DIAG_DSL_LOOPBACK_TIMEOUT_PERIOD)
            {
               CMSMEM_REPLACE_STRING(dslDiagObj->loopDiagnosticsState,MDMVS_ERROR_INTERNAL);
               cmsObj_set(dslDiagObj, &iidStack);
            }
            dslDiagInProgress=FALSE;
            CmsMsgHeader msg = EMPTY_MSG_HEADER;
            dslLoopDiagInfo.dslLoopDiagInProgress = FALSE;
            dslLoopDiagInfo.pollRetries = 0;

            if (dslLoopDiagInfo.src == EID_TR69C)
            {
               msg.type = CMS_MSG_DSL_LOOP_DIAG_COMPLETE;
               msg.src =  EID_SSK;
               msg.dst = EID_TR69C;
               msg.flags_event = 1;
               if (cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, &iidStack, 0,(void **)&dslCfgObj) == CMSRET_SUCCESS)
               {
                  lineId = dslCfgObj->X_BROADCOM_COM_BondingLineNumber;
                  cmsObj_free((void **) &dslCfgObj);
               }
               msg.wordData = lineId;               
               if (cmsMsg_send(appMsgHandle, &msg) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not send out CMS_MSG_DSL_LOOP_DIAG_COMPLETE event msg");
               }
               else
               {
                  cmsLog_debug("Send out CMS_MSG_DSL_LOOP_DIAG_COMPLETE event msg.");
               }
            }
         }
         cmsObj_free((void **) &dslDiagObj);

      } /* get obj ok */
      cmsLck_releaseLock();
   } /* lock requested ok */
}

#endif /* DMP_DSLDIAGNOSTICS_1 */



#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
void displayBondingStateTable(const char *heading, DslBondingLinkInfo *xDslBondingLinkStatetbl )
{
   cmsLog_notice("%s  xDsl link state table:", heading);
   cmsLog_notice("line0_curr %d, line0_new %d", xDslBondingLinkStatetbl->isCurrLine0Up, xDslBondingLinkStatetbl->isNewLine0Up);
   cmsLog_notice("line1_curr %d, line1_new %d", xDslBondingLinkStatetbl->isCurrLine1Up, xDslBondingLinkStatetbl->isNewLine1Up);
}   
#endif


#if defined(DMP_X_BROADCOM_COM_ATMWAN_1) && defined(DMP_X_BROADCOM_COM_DSLBONDING_1)
UBOOL8 isBondedAtmLinkUp(void)
{
   UBOOL8 isUp=FALSE;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   WanDslIntfCfgObject *bondedDslIntfObj=NULL;
   CmsRet ret;

   ret = dalDsl_getBondingAtmDslIntfObject(&iidStack, &bondedDslIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("could not get Bonded WanDslIntfObject, ret=%d", ret);
      return isUp;
   }

   isUp = !cmsUtl_strcmp(bondedDslIntfObj->status, MDMVS_UP);

   cmsObj_free((void **) &bondedDslIntfObj);

   return isUp;
}
#endif  /* DMP_X_BROADCOM_COM_ATMWAN_1 && DMP_X_BROADCOM_COM_DSLBONDING_1 */


#if defined(DMP_PTMWAN_1) && defined(DMP_X_BROADCOM_COM_DSLBONDING_1)

UBOOL8 isBondedPtmLinkUp(void)
{
   UBOOL8 isUp=FALSE;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   WanDslIntfCfgObject *bondedDslIntfObj=NULL;
   CmsRet ret;

   ret = dalDsl_getBondingPtmDslIntfObject(&iidStack, &bondedDslIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("could not get Bonded WanDslIntfObject, ret=%d", ret);
      return isUp;
   }

   isUp = !cmsUtl_strcmp(bondedDslIntfObj->status, MDMVS_UP);

   cmsObj_free((void **) &bondedDslIntfObj);

   return isUp;
}
#endif  /* DMP_PTMWAN_1 && DMP_X_BROADCOM_COM_DSLBONDING_1 */


#ifdef SUPPORT_DSL_BONDING
/** Kernel traffic type mismatch , call handler function to determine if we
 *  need to reboot the system taking account of the traffic type mismatch.
 */
void processTrafficMismatchMessage(unsigned int msgData __attribute__((unused)))
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      /* just a kernel event, I guess we can try later. */
      return;
   }

   setDslBondingTrafficType() ;

   cmsLck_releaseLock();

   return;
}


void setDslBondingTrafficType()
{
   int status ;
   CmsRet ret ;
   XTM_BOND_INFO bondInfo ;

   ret = dalDsl_getDslBonding (&status) ;
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("could not get Bonded WanDslIntfObject, ret=%d", ret) ;
      return ;
   }

	ret = devCtl_xtmGetBondingInfo ( &bondInfo );

   status =  (ret == CMSRET_SUCCESS) ? 0x1 : 0x0 ;

   sleep (1) ;

	if (status == 0x1) {

		ret = dalDsl_setDslBonding (status) ;
		if (ret != CMSRET_SUCCESS)
		{
			cmsLog_debug("could not set Bonded WanDslIntfObject, ret=%d", ret) ;
			return ;
		}

		cmsMgm_saveConfigToFlash() ;
	}

	/* else for non-bonded traffic, we will still keep the system mode as dual
	 * interface mode, as it is a super set configuration, which  will work for
	 * single line mode & will facilitate future dual interface configuration
	 * switching */
}
#endif /* SUPPORT_DSL_BONDING */

