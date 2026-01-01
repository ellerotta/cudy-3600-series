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

#ifdef DMP_DEVICE2_DSL_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_dsl.h"
#include "rut2_dsl.h"
#include "devctl_adsl.h"
#include "qdm_dsl.h"

CmsRet rcl_dev2DslObject( _Dev2DslObject *newObj __attribute__((unused)),
                          const _Dev2DslObject *currObj __attribute__((unused)),
                          const InstanceIdStack *iidStack __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   cmsLog_debug("Enter");
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslLineObject( _Dev2DslLineObject *newObj,
                              const _Dev2DslLineObject *currObj,
                              const InstanceIdStack *iidStack  __attribute__((unused)),
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("Enter");
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
      
   /*
    * Update DSL Line NumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDslNumOfEntries(MDMOID_DEV2_DSL_LINE, 1);
      /* Regardless of how many DSL lines we have, we just need to init dsl once.
       * It seems xtm driver can only be initialized once.  XXX update/fix this
       * comment or code: rutdsl_configUp has a small section that is not run
       * if dslInitDone is TRUE, but it has other code which seems to be able
       * to run multiple times.  And in the ENABLE_EXISTING+configChanged
       * block below, if user changes the config, we can call rutdsl_configUp
       * multiple times.  Maybe we can move the real one-time-only code to
       * mdm2_initdsl.c/adjustForHardware.
       */
      if (!mdmShmCtx->dslInitDone)
      {
         if (newObj->enable)
         {
            ret = rutdsl_configUp_dev2(newObj);
         }
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /* although we are not supposed to delete a dsl line */
      rutUtil_modifyNumDslNumOfEntries(MDMOID_DEV2_DSL_LINE, -1);
   }
   else if (ENABLE_EXISTING(newObj, currObj) ||
            rutdsl_isDslConfigChanged_dev2(newObj, currObj))
   {
      /* enable or change config on existing enabled intf */
      if (newObj->enable)
      {
         ret = rutdsl_configUp_dev2(newObj);
      }
   }
   else if (DISABLE_EXISTING(newObj, currObj))
   {
      rutdsl_configDown_dev2(newObj);
   }

   return ret;
}

CmsRet rcl_dev2DslLineStatsObject( _Dev2DslLineStatsObject *newObj __attribute__((unused)),
                                   const _Dev2DslLineStatsObject *currObj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                   char **errorParam __attribute__((unused)),
                                   CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslLineStatsTotalObject( _Dev2DslLineStatsTotalObject *newObj __attribute__((unused)),
                                        const _Dev2DslLineStatsTotalObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslLineStatsShowtimeObject( _Dev2DslLineStatsShowtimeObject *newObj __attribute__((unused)),
                                           const _Dev2DslLineStatsShowtimeObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

#ifdef NOT_SUPPORTED
CmsRet rcl_dev2DslLineStatsLastShowtimeObject( _Dev2DslLineStatsLastShowtimeObject *newObj __attribute__((unused)),
                                               const _Dev2DslLineStatsLastShowtimeObject *currObj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)),
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif

CmsRet rcl_dev2DslLineStatsCurrentDayObject( _Dev2DslLineStatsCurrentDayObject *newObj __attribute__((unused)),
                                             const _Dev2DslLineStatsCurrentDayObject *currObj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)),
                                             char **errorParam __attribute__((unused)),
                                             CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslLineStatsQuarterHourObject( _Dev2DslLineStatsQuarterHourObject *newObj __attribute__((unused)),
                                              const _Dev2DslLineStatsQuarterHourObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslLineTestParamsObject( _Dev2DslLineTestParamsObject *newObj __attribute__((unused)),
                                        const _Dev2DslLineTestParamsObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DslChannelObject( _Dev2DslChannelObject *newObj,
                                 const _Dev2DslChannelObject *currObj,
                                 const InstanceIdStack *iidStack  __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("Enter");
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
      
   /*
    * Update DSL Channel NumberOfEntries on successful (these are actually always added after bootup) 
    */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDslNumOfEntries(MDMOID_DEV2_DSL_CHANNEL, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /* although we are not supposed to delete a dsl line */
      rutUtil_modifyNumDslNumOfEntries(MDMOID_DEV2_DSL_CHANNEL, -1);
   }

   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslChannelStatsObject( _Dev2DslChannelStatsObject *newObj __attribute__((unused)),
                                      const _Dev2DslChannelStatsObject *currObj __attribute__((unused)),
                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslChannelStatsTotalObject( _Dev2DslChannelStatsTotalObject *newObj __attribute__((unused)),
                                           const _Dev2DslChannelStatsTotalObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslChannelStatsShowtimeObject( _Dev2DslChannelStatsShowtimeObject *newObj __attribute__((unused)),
                                              const _Dev2DslChannelStatsShowtimeObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslChannelStatsLastShowtimeObject( _Dev2DslChannelStatsLastShowtimeObject *newObj __attribute__((unused)),
                                                  const _Dev2DslChannelStatsLastShowtimeObject *currObj __attribute__((unused)),
                                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                                  char **errorParam __attribute__((unused)),
                                                  CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslChannelStatsCurrentDayObject( _Dev2DslChannelStatsCurrentDayObject *newObj __attribute__((unused)),
                                                const _Dev2DslChannelStatsCurrentDayObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslChannelStatsQuarterHourObject( _Dev2DslChannelStatsQuarterHourObject *newObj __attribute__((unused)),
                                                 const _Dev2DslChannelStatsQuarterHourObject *currObj __attribute__((unused)),
                                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                                 char **errorParam __attribute__((unused)),
                                                 CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

#ifdef DMP_DEVICE2_DSLDIAGNOSTICS_1
CmsRet rcl_dev2DslDiagnosticsObject( _Dev2DslDiagnosticsObject *newObj,
                                     const _Dev2DslDiagnosticsObject *currObj,
                                     const InstanceIdStack *iidStack,
                                     char **errorParam,
                                     CmsRet *errorCode)
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslDiagnosticsADSLLineTestObject( _Dev2DslDiagnosticsADSLLineTestObject *newObj,
                                                 const _Dev2DslDiagnosticsADSLLineTestObject *currObj,
                                                 const InstanceIdStack *iidStack,
                                                 char **errorParam,
                                                 CmsRet *errorCode)
{
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }


   if (cmsUtl_strcmp(newObj->diagnosticsState, currObj->diagnosticsState) != 0)
   {
      /* the only value allow is requested for diagnosticsState */
      if (((myEid != EID_DSL_SSK) && (myEid != EID_SSK)) &&
          (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0))
      {
         cmsLog_error("Mgmt apps may only write requested to this parameter");
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) 
   {
      /* DSL Loop Diagnostics will bring ADSL link down.  ACS (TR69) expects
       * a set request response after initiating this test.  This RCL needs to return
       * set success before initiating this test to allow time for TR69 client to send
       * the response.   SSK will tell the driver to start the test, and poll the results.
       */
       UINT32 msgDataLen = sizeof(dslDiagMsgBody);
       char buf[sizeof(CmsMsgHeader) + sizeof(dslDiagMsgBody)]={0};
       CmsMsgHeader *msg = (CmsMsgHeader *) buf;
       dslDiagMsgBody *info = (dslDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
       
       cmsLog_notice("Sending message to SSK to start doing checking the results, iidStack %s",cmsMdm_dumpIidStack(iidStack));
       msg->type = CMS_MSG_WATCH_DSL_LOOP_DIAG;
       msg->src = cmsMsg_getHandleEid(msgHandle);
       if (!strcmp(mdmShmCtx->compName, BDK_COMP_DSL))
       {
          msg->dst = EID_DSL_SSK;
       }
       else
       {
          msg->dst = EID_SSK;
       }
       msg->flags_request = 1;
       msg->dataLength = msgDataLen;
       info->iidStack = *iidStack;

       if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
       {
          cmsLog_debug("Could not send out CMS_MSG_DSL_LOOP_DIAG_MONITOR ret=%d", ret);
       }
       else
       {
          cmsLog_debug("CMS_MSG_DSL_LOOP_DIAG_MONITOR msg sent");
       }
   } /* requested */
   return (ret);
}
#endif /* DMP_DEVICE2_DSLDIAGNOSTICS_1 */

#ifdef DMP_X_BROADCOM_COM_DSL_1 
CmsRet rcl_dev2DslLineBertTestObject( _Dev2DslLineBertTestObject *newObj __attribute__((unused)),
                                      const _Dev2DslLineBertTestObject *currObj __attribute__((unused)),
                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */

   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return ret;
   }

   ret = rutWan_setAdslBertInfo(newObj,currObj,iidStack);

   return ret;
}
#endif /* DMP_X_BROADCOM_COM_DSL_1  */

#endif /* DMP_DEVICE2_DSL_1 */


#endif /* DMP_DEVICE2_BASELINE_1 */
