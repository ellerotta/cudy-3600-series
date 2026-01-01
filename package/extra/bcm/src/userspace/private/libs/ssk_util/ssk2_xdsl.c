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
#ifdef DMP_DEVICE2_DSL_1


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "ssk_util.h"
#include "AdslMibDef.h"
#include "devctl_adsl.h"
#include "bcmnetlink.h"
#include "devctl_xtm.h"
#include "adslctlapi.h"

/*!\file ssk2_xdsl.c
 *
 * This file contains TR181 related DSL code, including bonding and
 * DSL diagnostics.
 */
#ifdef DMP_DSLDIAGNOSTICS_1
extern dslLoopDiag dslLoopDiagInfo;
#endif
#ifdef SUPPORT_DSL_GFAST
extern void rutfast_cfgProfileInit_dev2(adslCfgProfile *pAdslCfg,  unsigned char lineId);
extern void rutfast_intfCfgInit_dev2(adslCfgProfile *pAdslCfg, Dev2FastLineObject *pFastLineObj);
#endif
extern void xdslUtil_IntfCfgInit_dev2(adslCfgProfile *pAdslCfg,  Dev2DslLineObject *pDslLineObj);




void checkDslLinkStatusLocked_dev2(void *appMsgHandle)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DslLineObject *dslLineObj=NULL;
   CmsMsgType msgType;
   UBOOL8 isLinkStatusChanged = FALSE;
#ifdef DMP_DEVICE2_FAST_1
   Dev2FastLineObject *fastLineObj=NULL;   
#endif

   while (cmsObj_getNext(MDMOID_DEV2_DSL_LINE, &iidStack,
                                    (void **) &dslLineObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("IfName=%s status=%s",
                   dslLineObj->name, dslLineObj->status);

      isLinkStatusChanged = comparePreviousLinkStatus(dslLineObj->name,
                                                      dslLineObj->upstream,
                                                      dslLineObj->status);

      if (isLinkStatusChanged)
      {
         if (!cmsUtl_strcmp(dslLineObj->status, MDMVS_UP))
         {
            printf("(%s) DSL LINE %s link up\n", _myAppName, dslLineObj->name);
            msgType = CMS_MSG_WAN_LINK_UP;
         }
         else
         {
            printf("(%s) DSL LINE %s link down\n", _myAppName, dslLineObj->name);
            msgType = CMS_MSG_WAN_LINK_DOWN;
         }

         intfStack_propagateStatusByIidLocked(appMsgHandle,
                                              MDMOID_DEV2_DSL_LINE,
                                              &iidStack, dslLineObj->status);

         sendStatusMsgToSmd(appMsgHandle, msgType, dslLineObj->name);
      }
      
      cmsObj_free((void **) &dslLineObj);
   }

#ifdef DMP_DEVICE2_FAST_1
   /* if the link had been trained in FAST mode before and now trained in other mode,
    * we still need to update the FAST link status if there is a change.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNext(MDMOID_DEV2_FAST_LINE, &iidStack,
                                  (void **) &fastLineObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("IfName=%s status=%s",
                   fastLineObj->name, fastLineObj->status);

      isLinkStatusChanged = comparePreviousLinkStatus(fastLineObj->name,
                                                      fastLineObj->upstream,
                                                      fastLineObj->status);

      if (isLinkStatusChanged)
      {
         if (!cmsUtl_strcmp(fastLineObj->status, MDMVS_UP))
         {
            printf("(%s) FAST LINE %s link up\n", _myAppName, fastLineObj->name);
            msgType = CMS_MSG_WAN_LINK_UP;
         }
         else
         {
            printf("(%s) FAST LINE %s link down\n", _myAppName, fastLineObj->name);
            msgType = CMS_MSG_WAN_LINK_DOWN;
         }

         intfStack_propagateStatusByIidLocked(appMsgHandle,
                                              MDMOID_DEV2_FAST_LINE,
                                              &iidStack, fastLineObj->status);

         sendStatusMsgToSmd(appMsgHandle, msgType, fastLineObj->name);
      }
      
      cmsObj_free((void **) &fastLineObj);
   }
#endif /* DMP_DEVICE2_FAST_1 */   
   return;
}



#ifdef TODO_LATER
/* this is needed, special handling in interface stack to call this one here for PTM mode */
/* we need to have the iidstack of the exact ptm link, and not the parentIidStack */
#ifdef DMP_PTMWAN_1

void informErrorSampleStatusChangeLocked_dev2(void *appMsgHandle, const InstanceIdStack *parentIidStack)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPtmLinkCfgObject *ptmLinkCfg = NULL;

   cmsLog_debug("Inform Error sample status");
   while (cmsObj_getNextInSubTree(MDMOID_WAN_PTM_LINK_CFG, parentIidStack, &iidStack, (void **)&ptmLinkCfg) == CMSRET_SUCCESS)
   {
      if (ptmLinkCfg->enable)
      {
        sendStatusMsgToSmd(appMsgHandle, CMS_MSG_WAN_ERRORSAMPLES_AVAILABLE,
                           ptmLinkCfg->X_BROADCOM_COM_IfName);
        cmsLog_debug("ErrorSamplesAvailable event received");
      }
      cmsObj_free((void **) &ptmLinkCfg);
   }
}
#endif /* DMP_PTMWAN_1 */
#endif /* TODO_LATER */


#ifdef DMP_DSLDIAGNOSTICS_1
void processWatchDslLoopDiag_dev2(CmsMsgHeader *msg)
{
   Dev2DslDiagnosticsADSLLineTestObject *dslDiagObj;
   dslDiagMsgBody *info = (dslDiagMsgBody*) (msg+1);
   InstanceIdStack iidStack = info->iidStack;
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
   {
      if (cmsObj_get(MDMOID_ADSL_LINE_TEST, &iidStack, 0, (void **) &dslDiagObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(dslDiagObj->diagnosticsState, MDMVS_REQUESTED) == 0)
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

void getDslLoopDiagResults_dev2(void *appMsgHandle)
{
   Dev2DslDiagnosticsADSLLineTestObject *dslDiagObj;
   InstanceIdStack iidStack = dslLoopDiagInfo.iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter: dslLoopDiagInfo.pollRetries %d, inProgress %d",dslLoopDiagInfo.pollRetries,
                dslLoopDiagInfo.dslLoopDiagInProgress);

   /* this is ugly... */
   if (dslLoopDiagInfo.pollRetries == 0)
   {
      /* We are about to tell dsldriver to start the test-- it will take the dsl link down; 
       * Sleep to give TR69c a chance to end its session.  I have to do it before getting the lock
       * because TR69c it.
       */
      sleep(2);
   }
   
   if ((cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
   {
      if ((ret = cmsObj_get(MDMOID_ADSL_LINE_TEST, &iidStack, 0, (void **) &dslDiagObj)) == CMSRET_SUCCESS)
      {
         if ((cmsUtl_strcmp(dslDiagObj->diagnosticsState, MDMVS_REQUESTED) == 0) &&
             (dslLoopDiagInfo.pollRetries < DIAG_DSL_LOOPBACK_TIMEOUT_PERIOD))
         {
            if (dslLoopDiagInfo.pollRetries == 0)
            {
               /* bring down the link and do the test */
               xdslCtl_SetTestMode(dslDiagObj->X_BROADCOM_COM_LineId,ADSL_TEST_DIAGMODE);
            }
            dslLoopDiagInfo.pollRetries++;
         }
         else
         {
            if (dslLoopDiagInfo.pollRetries >= DIAG_DSL_LOOPBACK_TIMEOUT_PERIOD)
            {
               CMSMEM_REPLACE_STRING(dslDiagObj->diagnosticsState,MDMVS_ERROR_INTERNAL);
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
               msg.wordData = dslDiagObj->X_BROADCOM_COM_LineId;
               if (cmsMsg_send(appMsgHandle, &msg) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not send out CMS_MSG_DSL_LOOP_DIAG_COMPLETE event msg");
               }
               else
               {
                  cmsLog_debug("Send out CMS_MSG_DSL_LOOP_DIAG_COMPLETE event msg.");
               }
            }
            else if ((dslLoopDiagInfo.src == EID_DSL_MD) || (dslLoopDiagInfo.src == EID_DSL_HAL_THREAD))
            {
               publishDiagCompleteMsg(appMsgHandle, CMS_MSG_DSL_LOOP_DIAG_COMPLETE);
            }
            else
            {
               cmsLog_error("dslLoopDiag src %d, diag loop complete msg not sent out",dslLoopDiagInfo.src);
            }
            getAdslLoopDiagResultAndLinkUp();
         }
         cmsObj_free((void **) &dslDiagObj);
      } /* get obj ok */
      else
      {
         cmsLog_error("could not get MDMOID_ADSL_LINE_TEST, ret %d",ret);
      }
      cmsLck_releaseLock();
   } /* lock requested ok */
}

CmsRet getAdslLoopDiagResultAndLinkUp(void)
{
   Dev2DslDiagnosticsADSLLineTestObject *dslDiagObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   SINT32 toneGroupArray[NUM_TONE_GROUP];
   UINT16 bandArray[NUM_BAND];
   UINT16 *ptr;
   adslMibInfo adslMib;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   char   oidStr[] = { kOidAdslPrivate, 0 };
   char   oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
   UINT8 gFactor = 1;
   long len;
   long	size = sizeof(adslMib);
   char *dataStr = NULL;
   UINT32 lineId;

   /* In the case of test error, it depdends on when diagnostics fails, the statistics (CO/CPE exchanges messages) may be useful.
    * So we will get the statistics no matter what the status is.
    */
   
   /* allocate a chunk of buffer -- biggest string formated data length */
   dataStr = cmsMem_alloc(MAX_PS_STRING,ALLOC_ZEROIZE);
   if (dataStr == NULL)
   {
      cmsLog_error("could not get allocate resource to process result");
      return CMSRET_RESOURCE_EXCEEDED;
   }
   
   if (cmsObj_get(MDMOID_ADSL_LINE_TEST, &iidStack, 0, (void **) &dslDiagObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_ADSL_LINE_TEST object");
      cmsMem_free(dataStr);
      return CMSRET_INTERNAL_ERROR;
   }
   lineId = dslDiagObj->X_BROADCOM_COM_LineId;

   ret = xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)&adslMib, &size);  
   if ((ret = xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)&adslMib, &size)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Unable to read data from the driver");
      cmsMem_free(dataStr);
      cmsObj_free((void **) &dslDiagObj);
      return (ret);
   }
   else
   {
      /* get G-factor objects --VDSL only */
      dslDiagObj->HLINGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      dslDiagObj->HLINGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus; 
      dslDiagObj->HLOGGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      dslDiagObj->HLOGGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      dslDiagObj->QLNGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      dslDiagObj->QLNGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      dslDiagObj->SNRGds = adslMib.gFactors.Gfactor_MEDLEYSETds;;
      dslDiagObj->SNRGus = adslMib.gFactors.Gfactor_MEDLEYSETus;
      
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      if (adslMib.adslConnection.modType == kVdslModVdsl2)
      {
         /* per-band objects --VDSL only */ 
         oidStr[1] = kOidAdslPrivLATNdsperband;
         len = sizeof(bandArray);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandArray, &len);
         ptr = &bandArray[0];
         sprintf(dataStr,"%d,%d,%d,%d,%d",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4]);
         CMSMEM_REPLACE_STRING(dslDiagObj->LATNpbds,dataStr);
         
         oidStr[1] = kOidAdslPrivLATNusperband;
         len = sizeof(bandArray);
         ptr = &bandArray[0];
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandArray, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4]);
         CMSMEM_REPLACE_STRING(dslDiagObj->LATNpbus,dataStr);

         oidStr[1] = kOidAdslPrivSATNdsperband;
         len = sizeof(bandArray);
         ptr = &bandArray[0];
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandArray, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4]);
         CMSMEM_REPLACE_STRING(dslDiagObj->SATNds,dataStr);
         
         oidStr[1] = kOidAdslPrivSATNusperband;
         len = sizeof(bandArray);
         ptr = &bandArray[0];
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandArray, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4]);
         CMSMEM_REPLACE_STRING(dslDiagObj->SATNus,dataStr);

#if 0 /* these are not in diag result */         
         /* get the dsNegBandPlanDiscPresentation */
         oidStr1[2] = kOidAdslPrivBandPlanDSNegDiscoveryPresentation;
         len = sizeof(bandPlanDescriptor32);
         xdslCtl_GetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&dsNegBandPlanDiscPresentation, &len);

         /* get the usNegBandPlanDiscPresentation */
         oidStr1[2] = kOidAdslPrivBandPlanUSNegDiscoveryPresentation;
         xdslCtl_GetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&usNegBandPlanDiscPresentation, &len);

         len = sizeof(bandArray);
         oidStr[1]= kOidAdslPrivSNRMusperband;
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char*)bandArray, &len);
         cmsAdsl_formatSnrmUsString(&dataStr,(void*)bandArray,usNegBandPlanDiscPresentation,MAX_BAND_STRING);
         CMSMEM_REPLACE_STRING(dslDiagObj->SNRMpbus,dataStr);
#endif         
      } /* vdsl only */
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */

      dslDiagObj->ACTPSDds = adslMib.xdslAtucPhys.actualPSD;
      dslDiagObj->ACTPSDus = adslMib.xdslPhys.actualPSD;

      dslDiagObj->SNRMTds = adslMib.adslPhys.SNRMT;
      dslDiagObj->SNRMTus = adslMib.adslAtucPhys.SNRMT;
      dslDiagObj->QLNMTds = adslMib.adslPhys.QLNMT;
      dslDiagObj->QLNMTus = adslMib.adslAtucPhys.QLNMT;
      dslDiagObj->HLOGMTds = adslMib.adslPhys.HLOGMT;
      dslDiagObj->HLOGMTus = adslMib.adslAtucPhys.HLOGMT;
      
      dslDiagObj->HLINSCds = adslMib.adslAtucPhys.adslHlinScaleFactor;
      dslDiagObj->ACTATPus = adslMib.adslPhys.adslCurrOutputPwr;
      dslDiagObj->ACTATPds = adslMib.adslAtucPhys.adslCurrOutputPwr;

      /* first we need to set the Gfactor */
      oidStr1[2] = kOidAdslPrivSetFlagActualGFactor;
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {
         /* get HLINpsds */         
         oidStr[1] = kOidAdslPrivChanCharLinDsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         cmsAdsl_formatHLINString(&dataStr,toneGroupArray,MAX_XDSL_DATA_LENGTH);
         CMSMEM_REPLACE_STRING(dslDiagObj->HLINpsds,dataStr);
      }
      len = 1;

      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {
         /* get HLINpsus */         
         oidStr[1] = kOidAdslPrivChanCharLinUsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatHLINString(&dataStr,toneGroupArray,MAX_XDSL_DATA_LENGTH);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->HLINpsus,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret  == CMSRET_SUCCESS) 
      {
         /* get QLNpsds */
         oidStr[1] = kOidAdslPrivQuietLineNoiseDsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"QLNpsds",MAX_PS_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->QLNpsds,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {  
         /* get QLNpsus */
         oidStr[1] = kOidAdslPrivQuietLineNoiseUsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"QLNpsus",MAX_PS_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->QLNpsus,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {
         /* get SNRpsds */
         oidStr[1] = kOidAdslPrivSNRDsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"SNRpsds",MAX_PS_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->SNRpsds,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {
         /* get SNRpsus */
         oidStr[1] = kOidAdslPrivSNRUsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"SNRpsus",MAX_PS_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->SNRpsus,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {
         /* get HLOGpsds */
         oidStr[1] = kOidAdslPrivChanCharLogDsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"HLOGpsds",MAX_LOGARITHMIC_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->HLOGpsds,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {  
         /* get HLOGusds */
         oidStr[1] = kOidAdslPrivChanCharLogUsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"HLOGpsus",MAX_LOGARITHMIC_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->HLOGpsus,dataStr);
         }
      }
      len = 1;
      ret = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS) 
      {
         /* get BITpsds */
         oidStr[1] = kOidAdslPrivBitAllocDsPerToneGroup;
         len = NUM_TONE_GROUP;
         memset(toneGroupArray,0,len);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&toneGroupArray,&len);
         ret = cmsAdsl_formatSubCarrierDataString(&dataStr,(void*)toneGroupArray,(char*)"BITSpsds",MAX_PS_STRING);
         if (ret == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(dslDiagObj->BITSpsds,dataStr);
         }
      }
      dslDiagObj->QLNMTds = 128;
      dslDiagObj->QLNMTus = 128;
      
      ret = cmsObj_set(dslDiagObj, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("lineId %d, diagnosticStates %s, unable to set stats, ret %d\n",lineId,dslDiagObj->diagnosticsState,ret);
      }
      ret = CMSRET_SUCCESS;
   }    /* finish reading statistics, bring the link back up */
   
   cmsObj_free((void **) &dslDiagObj);
   cmsMem_free(dataStr); 
   devCtl_adslConnectionStart();

   return ret;
}

#endif /* DMP_DSLDIAGNOSTICS_1 */

#ifdef DMP_DEVICE2_BONDEDDSL_1
/* XDSL driver (used to ?) sends a Traffic Mismatch Message to SSK.  (Now this function is called from interface stack).
 * At this point, the driver has dynamically reconfigured everything--bond mode or non-bond mode.
 * To better reflect what mode the system is running in, this is what happens:
 *    1. CPE has bonding enabled, link is trained non-bonded.
 *    2. CPE has bonding disabled, link is trained bonded.  It doesn't matter what users configured,
 *         if the link is trained bonded, the configuration is changed to "bonding enabled".
 *         On the other hand, for case 1, the configuration is left alone to "bonding enabled".
 *     
 * If there is a traffic mismatch, this routine is called. And these will happen:
 *    1. bonding is enabled, and link traffic type is ATM or PTM, then the Bonding Group status is
 *       changed to PeerBondSchemeMismatch.   ATM.Link or PTM.Link's lowerlayer is changed from
 *       BondingGroup.{i} to DSL.Channel.{i} and/or FAST.{i}
 *    2. bonding is enabled, and link traffic type is now ATM_BONDED or PTM_BONDED, Bonding Group Status
 *       is set to None; and the ATM.Link or PTM.Link's lowerLayers is changed back to BondGroup.{i}.
 */
void updateXtmLowerLayerLocked()
{
   XTM_BOND_INFO bondInfo;
   UBOOL8 trainBonded=FALSE;
   UBOOL8 isATM=FALSE;
   UBOOL8 isPTM=FALSE;
   int i;

   for (i=0; i < 2; i++)
   {
      memset(&bondInfo,0,sizeof(XTM_BOND_INFO));
      trainBonded = FALSE;
      isATM = FALSE;
      isPTM = FALSE;

      /* get the the traffic type from driver */
      devCtl_xtmGetBondingInfo(&bondInfo);

      switch (bondInfo.ulTrafficType)
      {
      case TRAFFIC_TYPE_ATM_BONDED:
         trainBonded = TRUE;
         isATM = TRUE;
         break;
      case TRAFFIC_TYPE_ATM:
         trainBonded=FALSE;
         isATM = TRUE;
         break;
      case TRAFFIC_TYPE_PTM_BONDED:
         trainBonded = TRUE;
         isPTM = TRUE;
         break;
      case TRAFFIC_TYPE_PTM:
         trainBonded=FALSE;
         isPTM = TRUE;
         break;
      default:
         // This happens if devCtl_xtmGetBondingInfo() failed, or we get an
         // unknown value for bondInfo.ulTrafficType.
         cmsLog_error("Unknown Bonding trafficType %d (no action taken)", bondInfo.ulTrafficType);
         break;
      }

      if (isATM)
      {
         // We need to give the ATM driver a little time to see if there is a
         // quick change to the bonding or non-bonding mode.  This happens
         // when a non-bonded NITRO DSL line is connected to a CPE running a
         // BONDING image.  sleep 1 and read again.
         sleep(1);
      }
      else if (isPTM)
      {
         // PTM does not need delayed read, so break out of for loop after
         // first read.
         break;
      }
      else
      {
         // Error during read, or unrecognized traffic type, break out of
         // loop after first read.
         break;
      }
   }

   if (isATM)
   {
      updateAtmLowerLayerLocked(trainBonded);
   }
   else if (isPTM)
   {
      updatePtmLowerLayerLocked(trainBonded);
   }
} 

void updateAtmLowerLayerLocked(UBOOL8 trainBonded)
{
   Dev2AtmLinkObject *pAtmLinkObj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor lowerLayerPathDesc;
   MdmPathDescriptor pathDesc;
   Dev2DslBondingGroupObject *bondingGroupObj=NULL;
   InstanceIdStack bondGroupIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS; 
   char *newLowerLayerFullPath = NULL;
   UBOOL8 found = FALSE;
   char *pLowerLayer;
   char *tmp;

   cmsLog_debug("ATM: trained Bonded= %d",(int)trainBonded);

   /* loop through the ATM first */
   while (!found && (ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &iidStack, (void **)&pAtmLinkObj)) == CMSRET_SUCCESS)
   {
      /* we only support one bonding group and a fix numbers of channel, so this task
       * of figuring out new lowerLayer needs to be done once for all the ATM VCC.
       */
      if (pAtmLinkObj->enable == TRUE)
      {
         /* all ATM links are the same, if one doesn't need to change, none needs to be changed */
         found = TRUE;

         cmsLog_debug("ATM: current LowerLayers is %s",pAtmLinkObj->lowerLayers);

         /* find out what the current LowerLayers is.  ATM link's lowerLayers should just be
          * a bonding group or a CSL of DSL.Channel. */
         INIT_PATH_DESCRIPTOR(&lowerLayerPathDesc);
         tmp = cmsMem_strdup(pAtmLinkObj->lowerLayers);
         pLowerLayer = strtok(tmp,",");
         ret = cmsMdm_fullPathToPathDescriptor(pLowerLayer, &lowerLayerPathDesc);
         CMSMEM_FREE_BUF_AND_NULL_PTR(tmp);
         if (ret == CMSRET_SUCCESS)
         {
            if (lowerLayerPathDesc.oid == MDMOID_DEV2_DSL_BONDING_GROUP)
            {
               if (trainBonded == FALSE)
               {
                  /* the lower interface needs to be changed to DSL.Channel that is UP now.
                   * and the bonding group's status needs update.
                   * Find out the lowerLayers of this bonding group now; and figure out the channel.
                   */
                  if (cmsObj_get(lowerLayerPathDesc.oid,&lowerLayerPathDesc.iidStack,
                                 0,(void **)&bondingGroupObj) == CMSRET_SUCCESS)
                  {
                     /* so I do not know which channel will be UP because there could be mediaSearch involved.
                      * I will make the lower layers point to either channels.
                      */
                     newLowerLayerFullPath = cmsMem_strdup(bondingGroupObj->lowerLayers);
                     cmsObj_free((void **) &bondingGroupObj);
                  }
               } /* not trainedBonded */
            } /* current LowerLayer is bondingGroup */
            else
            {
               /* current LowerLayer is non bonding, DSL.Channel */
               if (trainBonded == TRUE)
               {
                  /* the lower interface needs to be changed to bonding group, 
                   * find the bonding group that is up.
                   */
                  while ((ret = cmsObj_getNext(MDMOID_DEV2_DSL_BONDING_GROUP, &bondGroupIidStack, (void **)&bondingGroupObj)) == CMSRET_SUCCESS)
                  {
                     if (cmsUtl_isFullPathInCSL(pAtmLinkObj->lowerLayers,bondingGroupObj->lowerLayers))
                     {
                        INIT_PATH_DESCRIPTOR(&pathDesc);
                        pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
                        pathDesc.iidStack = bondGroupIidStack;
                        if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &newLowerLayerFullPath)) == CMSRET_SUCCESS)
                        {
                           cmsObj_free((void **) &bondingGroupObj);
                           break;
                        }
                        cmsObj_free((void **) &bondingGroupObj);
                     }
                  } /* found the bonding group */
               } /* trained bonded mode */
            } 
         } /* lowerLayer */
         cmsObj_free((void **) &pAtmLinkObj);
      }
   } /* while  ATM LINK */

   /* update all the ATM LINK to new lower layer */
   if (found && (newLowerLayerFullPath != NULL))
   {
      INIT_INSTANCE_ID_STACK(&iidStack);
      while ((ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &iidStack, (void **)&pAtmLinkObj)) == CMSRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING(pAtmLinkObj->lowerLayers,newLowerLayerFullPath);
         if (cmsObj_set(pAtmLinkObj,&iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("Fail to set new LowerLayers (%s) for ATM LINK",newLowerLayerFullPath);
         }
         cmsObj_free((void **) &pAtmLinkObj);
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(newLowerLayerFullPath);
   } /* found */
}

void updatePtmLowerLayerLocked(UBOOL8 trainBonded)
{
   Dev2PtmLinkObject *pPtmLinkObj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor lowerLayerPathDesc;
   MdmPathDescriptor pathDesc;
   Dev2DslBondingGroupObject *bondingGroupObj=NULL;
   InstanceIdStack bondingGroupIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS; 
   char *newLowerLayerFullPath = NULL;
   char *pLast = NULL;
   UBOOL8 found = FALSE;
   char *pLowerLayer;
   char *tmp;
   UINT32 currentLen=0;
   UINT32 totalNeeded=0;

   cmsLog_debug("PTM: trained Bonded= %d",(int)trainBonded);

   /* we support 2 different types of bonding groups: DSL channels (PTM) or FAST lines.
    * So when fast bonding is possible, a PTM link can be stacking on 
    * Device.DSL.BondingGroup.2 (DSL PTM channels),Device.DSL.BondingGroup.3 (FAST) 
    */
   while (!found && (ret = cmsObj_getNext(MDMOID_DEV2_PTM_LINK, &iidStack, (void **)&pPtmLinkObj)) == CMSRET_SUCCESS)
   {
      if (pPtmLinkObj->enable == TRUE)
      {
         /* all PTM links are the same, if one doesn't need to change, none needs to be changed */
         found = TRUE;

         cmsLog_debug("PTM: current LowerLayers is %s",pPtmLinkObj->lowerLayers);

         /* find out what the current LowerLayers is.  PTM link's lowerLayers should just be
          * a bonding group (or 2 bonding groups if FAST and DSL are possible) 
          * a CSL of DSL.Channel. and possibly FAST.Line.{i} too.
          * We do not know if the line is going to be trained FAST mode, DSL mode... so in this case
          * the lower layers of PTM needs to be both FAST and DSL
          */
         INIT_PATH_DESCRIPTOR(&lowerLayerPathDesc);
         tmp = cmsMem_strdup(pPtmLinkObj->lowerLayers);

         /* compose the new lowerlayers string from current lowerLayers which is either one of these:
          * PTM.LowerLayers: DSL.Channel.2, DSL.Channel.4, (if FAST enabled) FAST.Line.1, FAST.Line.2 ==> BondingGroups
          * PTM.LowerLayers: DSL.BondingGroup.2, (if FAST bonding enabled) DSL.BondingGroup.3 ==> channels and FAST lines
          */
         pLowerLayer = strtok_r(tmp,",",&pLast);
         while (pLowerLayer != NULL)
         {
            ret = cmsMdm_fullPathToPathDescriptor(pLowerLayer, &lowerLayerPathDesc);
            if (ret == CMSRET_SUCCESS)
            {
               if (lowerLayerPathDesc.oid == MDMOID_DEV2_DSL_BONDING_GROUP)
               {
                  if (trainBonded == FALSE)
                  {
                     /* the lower interface needs to be changed to DSL.Channel that is UP now.
                      * and the bonding group's status needs update.
                      * Find out the lowerLayers of this bonding group now; and figure out the channel.
                      */
                     if (cmsObj_get(lowerLayerPathDesc.oid,&lowerLayerPathDesc.iidStack,
                                    OGF_NO_VALUE_UPDATE,(void **)&bondingGroupObj) == CMSRET_SUCCESS)
                     {
                        /* so I do not know which channel will be UP because there could be mediaSearch involved.
                         * I will make the lower layers point to either channels.
                         */
                        if (newLowerLayerFullPath == NULL)
                        {
                           currentLen = strlen(bondingGroupObj->lowerLayers);
                           newLowerLayerFullPath = cmsMem_alloc(currentLen+1,ALLOC_ZEROIZE);
                           memcpy(newLowerLayerFullPath,bondingGroupObj->lowerLayers,currentLen);
                        }
                        else
                        {
                           /* +1 for a comma */
                           totalNeeded= currentLen + strlen(bondingGroupObj->lowerLayers) + 1;

                           /* +1 for null terminated */
                           newLowerLayerFullPath = cmsMem_realloc(newLowerLayerFullPath,totalNeeded+1);
                           
                           newLowerLayerFullPath[currentLen]=',';
                           memcpy(newLowerLayerFullPath+currentLen+1,bondingGroupObj->lowerLayers,strlen(bondingGroupObj->lowerLayers));
                           currentLen = totalNeeded;
                           newLowerLayerFullPath[currentLen]='\0';
                        }
                        cmsObj_free((void **) &bondingGroupObj);
                     }
                  } /* not trainedBonded */
               } /* current LowerLayer is bondingGroup */
               else
               {
                  char *bondingGroupFullPath=NULL;
                  
                  /* current LowerLayer is non bonding, DSL.Channel */
                  /* and if FAST is built in, the current LowerLayer can also have FAST.Line */
                  if (trainBonded == TRUE)
                  {
                     /* the lower interface needs to be changed to bonding group, 
                      * find the bonding group that is up.
                      */
                     while ((ret = cmsObj_getNext(MDMOID_DEV2_DSL_BONDING_GROUP, &bondingGroupIidStack, (void **)&bondingGroupObj)) == CMSRET_SUCCESS)
                     {
                        if (cmsUtl_isFullPathInCSL(pLowerLayer,bondingGroupObj->lowerLayers))
                        {
                           INIT_PATH_DESCRIPTOR(&pathDesc);
                           pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
                           pathDesc.iidStack = bondingGroupIidStack;
                           if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &bondingGroupFullPath)) == CMSRET_SUCCESS)
                           {
                              if (newLowerLayerFullPath == NULL)
                              {
                                 currentLen = strlen(bondingGroupFullPath);
                                 newLowerLayerFullPath = cmsMem_alloc(currentLen+1,ALLOC_ZEROIZE);
                                 memcpy(newLowerLayerFullPath,bondingGroupFullPath,currentLen);
                              }
                              else
                              {
                                 if (cmsUtl_isFullPathInCSL(bondingGroupFullPath,newLowerLayerFullPath) == 0)
                                 {
                                    /* add 1 for the comma */
                                    totalNeeded= currentLen + strlen(bondingGroupFullPath)+1;
                                    newLowerLayerFullPath= cmsMem_realloc(newLowerLayerFullPath,totalNeeded+1);
                                    newLowerLayerFullPath[currentLen] = ',';
                                    memcpy(newLowerLayerFullPath+currentLen+1,bondingGroupFullPath,strlen(bondingGroupFullPath));
                                    currentLen=totalNeeded;
                                    newLowerLayerFullPath[currentLen]='\0';
                                 }
                              }
                              CMSMEM_FREE_BUF_AND_NULL_PTR(bondingGroupFullPath);
                              cmsObj_free((void **) &bondingGroupObj);
                              break;
                           }
                        } /* found the bonding group */
                        cmsObj_free((void **) &bondingGroupObj);
                     } /* while */
                  } /* trained bonded mode */
               }
            } /* ret == sucess */
            pLowerLayer = strtok_r(NULL,",",&pLast);
         }  /* while lowerLayer */
         CMSMEM_FREE_BUF_AND_NULL_PTR(tmp);
      } /* ptm link enabled */
      cmsObj_free((void **) &pPtmLinkObj);
   } /* while  PTM LINK */

   
   /* update all the PTM LINK to new lower layer */
   if (found && (newLowerLayerFullPath != NULL))
   {
      INIT_INSTANCE_ID_STACK(&iidStack);
      while ((ret = cmsObj_getNext(MDMOID_DEV2_PTM_LINK, &iidStack, (void **)&pPtmLinkObj)) == CMSRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING(pPtmLinkObj->lowerLayers,newLowerLayerFullPath);
         if (cmsObj_set(pPtmLinkObj,&iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("Fail to set new LowerLayers (%s) for PTM LINK",newLowerLayerFullPath);
         }
         cmsObj_free((void **) &pPtmLinkObj);
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(newLowerLayerFullPath);
   } /* found */
}

#endif /*  DMP_DEVICE2_BONDEDDSL_1 */

#endif /* DMP_DEVICE2_DSL_1  */
#endif /* DMP_DEVICE2_BASELINE_1 */
