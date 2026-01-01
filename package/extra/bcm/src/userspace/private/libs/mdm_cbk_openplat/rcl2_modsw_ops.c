/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#ifdef DMP_DEVICE2_SM_BASELINE_1

#include "cms_core.h"
#include "cms_util.h"
#include "cms_msg_modsw.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut2_modsw_ops.h"
#include "cms_qdm.h"
#include "cms_params_modsw.h"


/*!\file rcl_modsw_ops.c
 * \brief This file contains generic modular sw functions (not specific to
 *        any specific execution env.)
 *
 */

CmsRet rcl_swModulesObject( _SwModulesObject *newObj __attribute__((unused)),
                const _SwModulesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* 
    * During system startup, MDM needs to be initialized completely before 
    * operation on this object is being done.  BBCD is launched after SSK completes initialization.
    */ 
   if (mdmShmCtx->inMdmInit)
   {
      return CMSRET_SUCCESS;
   }

   if ((newObj && currObj) && 
       (newObj->X_BROADCOM_COM_PreinstallAtRuntime !=
        currObj->X_BROADCOM_COM_PreinstallAtRuntime))
   {
      if (newObj->X_BROADCOM_COM_PreinstallAtRuntime)
      {
         rutModsw_sendMsgTo(EID_OPENPLAT_MD,
                            CMS_MSG_REQUEST_PREINSTALL_EE_CHANGE,
                            "", 0);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_execEnvObject( _ExecEnvObject *newObj,
                const _ExecEnvObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumExecEnvEntry(iidStack, 1);

	  /* initialize parameters */
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_ContainerName))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ContainerName, "",
                                     mdmLibCtx.allocFlags);
      }
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_MngrAppName))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MngrAppName, "",
                                     mdmLibCtx.allocFlags);
      }

      return CMSRET_SUCCESS;
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumExecEnvEntry(iidStack, -1);

      /* 
       * this object cannot be deleted by mgmt entity, so we do not
       * have to send message to stop the ExecEnv on deleteInstance.
       */

      return CMSRET_SUCCESS;
   }

   if (!(newObj && currObj))
   {
      return CMSRET_SUCCESS;
   }

   if (newObj->requestedRunLevel != currObj->requestedRunLevel)
   {
      if (newObj->currentRunLevel != -1)
      {
         newObj->currentRunLevel = newObj->requestedRunLevel;
      }

      /* 
       * The value of this parameter is not part of the device configuration
       * and is always -1 when read.
       */
      newObj->requestedRunLevel = -1;
   }

   if (newObj->enable != currObj->enable)
   {
      char *fullPath=NULL;
      MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;

      cmsLog_debug("(runtime) change of enable to %d", newObj->enable);

      pathDesc.oid = MDMOID_EXEC_ENV;
      pathDesc.iidStack = *iidStack;
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create fullPath to myself, ret=%d", ret);
         return ret;
      }

      if (newObj->enable)
      {
         if (cmsUtl_strcmp(currObj->status, MDMVS_DISABLED) == 0)
         {
            rutModsw_sendMsgTo(EID_OPENPLAT_MD, CMS_MSG_START_EE,
                               fullPath, sizeof(EErequestStartMsgBody));
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_ERROR) == 0)
         {
            /* Allow enable an EE at ERROR staus. */
            rutModsw_sendMsgTo(EID_OPENPLAT_MD, CMS_MSG_START_EE,
                               fullPath, sizeof(EErequestStartMsgBody));
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_UP))
         {
            cmsLog_debug("start fail!");             
            /* Don't allow enable a running EE again */
            ret = CMSRET_INVALID_PARAM_VALUE;
         }

         /* 
          * EE is enabled.
          * set currentRunLevel to initialRunLevel if it is relevant.
          */
         if (newObj->currentRunLevel != -1)
         {
            newObj->currentRunLevel = newObj->initialRunLevel;
         }
      }
      else
      {
         if (cmsUtl_strcmp(currObj->status, MDMVS_UP) == 0)
         {
            rutModsw_sendMsgTo(EID_OPENPLAT_MD, CMS_MSG_STOP_EE,
                               fullPath, sizeof(EErequestStopMsgBody));
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_ERROR) == 0)
         {
            /* 
             * Disable an EE at ERROR status, simply change the status
             * to DISABLED
             */
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED,
                                        mdmLibCtx.allocFlags);
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_DISABLED)==0)
         {
            /* Don't allow disable a disabled EE again */
            cmsLog_debug("stop fail!");
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   }
   else if (cmsUtl_strcmp(newObj->status, currObj->status))
   {
      if (newObj->enable)
      {
#if 0
         if (!cmsUtl_strcmp(newObj->status, MDMVS_X_BROADCOM_COM_STARTING) ||
             !cmsUtl_strcmp(newObj->status, MDMVS_X_BROADCOM_COM_STOPPING) ||
             !cmsUtl_strcmp(newObj->status, MDMVS_DISABLED))
         {
            UBOOL8 isParentUp;

            isParentUp = isParentEeUp(newObj->parentExecEnv);

            if (!isParentUp && !cmsUtl_strcmp(newObj->status, MDMVS_DISABLED))
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED,
                                                 mdmLibCtx.allocFlags);
            }
            else
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ERROR,
                                                 mdmLibCtx.allocFlags);
            }
         }
         else if (cmsUtl_strcmp(newObj->status, MDMVS_UP) == 0)
         {
            /* status changes to UP. set currentRunLevel to initialRunLevel
             * if it is relevant.
             */
            if (newObj->currentRunLevel != -1)
            {
               newObj->currentRunLevel = newObj->initialRunLevel;
            }
         }
#endif
      }
      else
      {
         if (cmsUtl_strcmp(newObj->status, MDMVS_DISABLED) &&
             cmsUtl_strcmp(newObj->status, MDMVS_X_BROADCOM_COM_STOPPING))
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ERROR,
                                              mdmLibCtx.allocFlags);
         }
      }
   }

   return ret;
}

CmsRet rcl_dUObject( _DUObject *newObj,
                const _DUObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDeploymentUnitEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDeploymentUnitEntry(iidStack, -1);
   }
   return CMSRET_SUCCESS;
}

#ifdef SUPPORT_BAS2
static CmsRet sendEuStateChangeReasonMsg(CmsEntityId mngrEid, const char *name,
                                         const char *reason, const char *time)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   EuStateChangedMsgBody *msgPayload = NULL;

   /* code here to initialize and send the rest of the message to pmd */
   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(EuStateChangedMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_EU_STATE_CHANGE_EVENT;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = mngrEid;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(EuStateChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (EuStateChangedMsgBody*)body;

   cmsUtl_strncpy(msgPayload->name, name, sizeof(msgPayload->name));
   cmsUtl_strncpy(msgPayload->reason, reason, sizeof(msgPayload->reason));
   cmsUtl_strncpy(msgPayload->time, time, sizeof(msgPayload->time));
   
   cmsLog_debug("sending to mngrEid=%d euName=%s", mngrEid, msgPayload->name);

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Failed to send message to MngrEid %d (ret=%d)",
                    mngrEid, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}
#else
static CmsRet sendEuStateChangeReasonMsg(CmsEntityId mngrEid __attribute__((unused)), const char *name __attribute__((unused)),
                                         const char *reason __attribute__((unused)), const char *time __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_eUObject( _EUObject *newObj,
                const _EUObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 STARTEU=FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   EUrequestStateChangedMsgBody *payload = NULL;

   /* 
    * During system startup, MDM needs to be initialized completely before 
    * operation on this object is being done.  The EE program manager should handle
    * the EU activation when it's up.
    */ 
   if (mdmShmCtx->inMdmInit)
   {
      return ret;
   }
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumExecutionUnitEntry(iidStack, 1);

      /* initialize parameters */
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_MngrAppName))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MngrAppName, "",
                                     mdmLibCtx.allocFlags);
      }

      /* FIXME: handle bootup case */
      return ret;
   }
   else if(DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumExecutionUnitEntry(iidStack, -1);
      return ret;
   }

   /* modify existing */
   if (newObj->X_BROADCOM_COM_autoRelaunch &&
       !currObj->X_BROADCOM_COM_autoRelaunch)
   {
      cmsLog_debug("autoRelaunch is enabled");

      /* reset the restartCount */
      newObj->X_BROADCOM_COM_restartCount = 0;
   }

   if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_StateChangeReason,
                     currObj->X_BROADCOM_COM_StateChangeReason))
   {
      char dateTimeBuf[BUFLEN_64];

      cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_State_LastChange,
                                  dateTimeBuf, mdmLibCtx.allocFlags);

      sendEuStateChangeReasonMsg(EID_BAS_CLIENT_OPENPLAT, newObj->name,
                                 newObj->X_BROADCOM_COM_StateChangeReason,
                                 newObj->X_BROADCOM_COM_State_LastChange);
   }

   cmsLog_debug("new requestedState=%s curr requestedState=%s currstatus=%s",
              newObj->requestedState, currObj->requestedState, currObj->status);

   if (cmsUtl_strcmp(newObj->requestedState, currObj->requestedState))
   {
      /* we have a change in the requested state */
      if (!cmsUtl_strcmp(newObj->requestedState, MDMVS_ACTIVE))
      {
         /* caller requested start EU, check current status */
         if (!cmsUtl_strcmp(currObj->status, MDMVS_STOPPING))
         {
            cmsLog_error("Requested ACTIVE, but EU is in STOPPING");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_STARTING) ||
                  !cmsUtl_strcmp(currObj->status, MDMVS_ACTIVE))
         {
            cmsLog_notice("Requested ACTIVE, but already %s, no action",
                          currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_SUCCESS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_IDLE))
         {
            cmsLog_debug("Requested ACTIVE, currently idle, start EU");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            STARTEU = TRUE;
         }
         else
         {
            cmsLog_error("Bad status %s", currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INTERNAL_ERROR;
         }
      }
      else if (!cmsUtl_strcmp(newObj->requestedState, MDMVS_IDLE))
      {
         /* caller requested stop EU, check current status */
         if (!cmsUtl_strcmp(currObj->status, MDMVS_STARTING))
         {
            cmsLog_error("Requested IDLE, but EU is in STARTING");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_STOPPING) ||
                  !cmsUtl_strcmp(currObj->status, MDMVS_IDLE))
         {
            cmsLog_notice("Requested IDLE, but already %s, no action",
                          currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_SUCCESS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_ACTIVE))
         {
            cmsLog_debug("Requested IDLE, currently ACTIVE, stop EU");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            STARTEU = FALSE;
         }
         else
         {
            cmsLog_error("Bad status %s", currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState, "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INTERNAL_ERROR;
         }
      }
      else
      {
         cmsLog_error("Bad requested state %s", newObj->requestedState);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                     "",
                                     mdmLibCtx.allocFlags);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      /* don't handle any other param changes (including autostart) */
      return CMSRET_SUCCESS;
   }

#if 0 //FIXME: handle non-standalone app case
   if (qdmModsw_getMngrEidByExecEnvFullPathLocked(newObj->executionEnvRef, &mngrEid) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get mngrEid for ExecEnvFullPath %s",
            newObj->executionEnvRef);
      return CMSRET_INVALID_ARGUMENTS;
   }
#endif

   payload = cmsMem_alloc(sizeof(EUrequestStateChangedMsgBody), ALLOC_ZEROIZE);

   cmsUtl_strncpy(payload->euid, currObj->EUID, sizeof(payload->euid));
   cmsUtl_strncpy(payload->name, currObj->name, sizeof(payload->name));
   cmsUtl_strncpy(payload->version, currObj->version,
                  sizeof(payload->version));

   if (STARTEU)
   {
      cmsUtl_strncpy(payload->operation, SW_MODULES_OPERATION_START,
                     sizeof(payload->operation));
   }
   else
   {
      cmsUtl_strncpy(payload->operation, SW_MODULES_OPERATION_STOP,
                     sizeof(payload->operation));
   }

   rutModsw_sendMsgTo(EID_OPENPLAT_MD, CMS_MSG_REQUEST_EU_STATE_CHANGE,
                      (char *)payload, sizeof(EUrequestStateChangedMsgBody));

   CMSMEM_FREE_BUF_AND_NULL_PTR(payload);

   return ret;
}


CmsRet rcl_extensionsObject( _ExtensionsObject *newObj __attribute__((unused)),
                         const _ExtensionsObject *currObj __attribute__((unused)),
                         const InstanceIdStack *iidStack __attribute__((unused)),
                         char **errorParam __attribute__((unused)),
                         CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_busObject( _BusObject *newObj __attribute__((unused)),
                      const _BusObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_busObjectPathObject( _BusObjectPathObject *newObj,
                                const _BusObjectPathObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusObjectPathEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusObjectPathEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busInterfaceObject( _BusInterfaceObject *newObj,
                               const _BusInterfaceObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusInterfaceEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusInterfaceEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busMethodObject( _BusMethodObject *newObj,
                            const _BusMethodObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusMethodEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusMethodEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busSignalObject( _BusSignalObject *newObj,
                            const _BusSignalObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusSignalEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusSignalEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busPropertyObject( _BusPropertyObject *newObj,
                              const _BusPropertyObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusPropertyEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusPropertyEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busClientObject( _BusClientObject *newObj __attribute__((unused)),
                          const _BusClientObject *currObj __attribute__((unused)),
                          const InstanceIdStack *iidStack __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


static CmsRet sendExtEuClientPrivilegeChanged(CmsEntityId mngrEid,
                                              const char *username,
                                              const char *fullPath,
                                              const char *operation)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   ExtEuClientPrivilegeChangedMsgBody *msgPayload = NULL;

   /* code here to initialize and send the rest of the message to pmd */
   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(ExtEuClientPrivilegeChangedMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_EXT_EU_CLIENT_PRIVILEGE_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = mngrEid;
   reqMsg->flags_request = 1;
   reqMsg->dataLength = sizeof(ExtEuClientPrivilegeChangedMsgBody);
   reqMsg->flags_bounceIfNotRunning = 1;

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (ExtEuClientPrivilegeChangedMsgBody*)body;

   cmsUtl_strncpy(msgPayload->username, username, sizeof(msgPayload->username));
   cmsUtl_strncpy(msgPayload->fullPath, fullPath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->operation, operation, sizeof(msgPayload->operation));
   
   cmsLog_debug("sending to mngrEid=%d operation=%s", mngrEid, msgPayload->operation);

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to MngrEid %d (ret=%d)",
                    mngrEid, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;

}  /* End of sendExtEuClientPrivilegeChanged() */


static UBOOL8 isMessageAllowedToSend(const char *euFullPath)
{
   UBOOL8 ret = FALSE;
   UBOOL8 found = FALSE;
   DUObject *duObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &duObj) == CMSRET_SUCCESS)
   {
      char *string = NULL, *token = NULL;

      string = duObj->executionUnitList;
      while (string != NULL)
      {
         token = strsep(&string, ",");

         if (!cmsUtl_strcmp(token, euFullPath))
         {
            found = TRUE;

            /* Only allow when DU status is NOT uninstalling
               to avoid error caused by EUObject is already deleted */
            if (duObj->status != NULL &&
                strcmp(duObj->status, MDMVS_UPDATING) != 0 &&
                strcmp(duObj->status, MDMVS_UNINSTALLING) != 0)
            {
               ret = TRUE;
            }

            break;
         }
      }

      cmsObj_free((void **)&duObj);
   }

   return ret;

}  /* End of isMessageAllowedToSend() */


CmsRet rcl_busClientPrivilegeObject( _BusClientPrivilegeObject *newObj,
                                     const _BusClientPrivilegeObject *currObj,
                                     const InstanceIdStack *iidStack,
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char euFullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   EUObject *euObj = NULL;
   InstanceIdStack iidStackEu = EMPTY_INSTANCE_ID_STACK;

   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD: name=%s, path=%s, interface=%s, member=%s, memberType=%s\n",
                   newObj->wellknownName, newObj->objectPath, newObj->interface,
                   newObj->member, newObj->memberType);

      /* increase number of bus client privileges */
      rutUtil_modifyNumBusClientPrivilegeEntry(iidStack, 1);

      return ret;
   }

   /* do not hide object when it's deleted */
   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE: name=%s, path=%s, interface=%s, member=%s, memberType=%s\n",
                   currObj->wellknownName, currObj->objectPath, currObj->interface,
                   currObj->member, currObj->memberType);

      /* decrease number of bus client privileges */
      rutUtil_modifyNumBusClientPrivilegeEntry(iidStack, -1);

      mdmLibCtx.hideObjectsPendingDelete = FALSE;
   }

   /* iidStackEu is started with BusClientPrivilegeObject iidStack */
   memcpy(&iidStackEu, iidStack, sizeof(InstanceIdStack));

   /* get EUObject */
   ret = cmsObj_getAncestorFlags(MDMOID_EU,
                                 MDMOID_BUS_CLIENT_PRIVILEGE,
                                 &iidStackEu,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&euObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get EUObject from BusClientPrivilegeObject iidStack, ret=%d", ret);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* get execution unit full path */
   ret = qdmModsw_getExecUnitFullPathByEuidLocked(euObj->EUID,
                                                  euFullPath,
                                                  sizeof(euFullPath));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get Execution Unit full path of EUID %s",
                   euObj->EUID);
      ret = CMSRET_INVALID_ARGUMENTS;
      goto out;
   }

   /* check message is allowed to send */
   if (!isMessageAllowedToSend(euFullPath) ||
       !cmsUtl_strcmp(euObj->status, MDMVS_STARTING))
   {
      cmsLog_notice("CMS_MSG_EXT_EU_CLIENT_PRIVILEGE_CHANGE is NOT allowed to send.");
      ret = CMSRET_SUCCESS;
      goto out;
   }

    /* delete existing */
   if (DELETE_EXISTING(newObj, currObj))
   {
      /* reset to hide object when it's deleted */
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      /* send EU client privilege changed message to spd
         for updating the busgate policy of the client. */
      ret = sendExtEuClientPrivilegeChanged(EID_OPENPLAT_MD,
                                            euObj->X_BROADCOM_COM_Username,
                                            euFullPath,
                                            SW_MODULES_OPERATION_UPDATE_PRIVILEGE);
   }
   /* modify existing */
   else
   {
      cmsLog_debug("EDIT: name=%s, path=%s, interface=%s, member=%s, memberType=%s\n",
                   newObj->wellknownName, newObj->objectPath, newObj->interface,
                   newObj->member, newObj->memberType);

      /* send EU client privilege changed message to spd
         for updating the busgate policy of the client. */
      ret = sendExtEuClientPrivilegeChanged(EID_OPENPLAT_MD,
                                            euObj->X_BROADCOM_COM_Username,
                                            euFullPath,
                                            SW_MODULES_OPERATION_UPDATE_PRIVILEGE);
   }

out:
   cmsObj_free((void **)&euObj);
   return ret;

}  /* End of rcl_busClientPrivilegeObject() */


CmsRet rcl_manifestObject( _ManifestObject *newObj,
                          const _ManifestObject *currObj,
                          const InstanceIdStack *iidStack __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      if (IS_EMPTY_STRING(newObj->exposedPorts))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->exposedPorts, "",
                                     mdmLibCtx.allocFlags);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dmAccessObject(_DmAccessObject *newObj __attribute__((unused)),
                          const _DmAccessObject *currObj __attribute__((unused)),
                          const InstanceIdStack *iidStack __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeManifestObject( _EeManifestObject *newObj __attribute__((unused)),
                const _EeManifestObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeBusObject( _EeBusObject *newObj __attribute__((unused)),
                const _EeBusObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeProcessObject( _EeProcessObject *newObj __attribute__((unused)),
                const _EeProcessObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeProcessArgsObject( _EeProcessArgsObject *newObj __attribute__((unused)),
                const _EeProcessArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeProcessEnvObject( _EeProcessEnvObject *newObj __attribute__((unused)),
                const _EeProcessEnvObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeMountsObject( _EeMountsObject *newObj __attribute__((unused)),
                const _EeMountsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeMountsOptionObject( _EeMountsOptionObject *newObj __attribute__((unused)),
                const _EeMountsOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeLinuxObject( _EeLinuxObject *newObj __attribute__((unused)),
                const _EeLinuxObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeResourceObject( _EeResourceObject *newObj __attribute__((unused)),
                const _EeResourceObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeBlkioObject( _EeBlkioObject *newObj __attribute__((unused)),
                const _EeBlkioObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeCpuObject( _EeCpuObject *newObj __attribute__((unused)),
                const _EeCpuObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeResDevicesObject( _EeResDevicesObject *newObj __attribute__((unused)),
                const _EeResDevicesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeNetworkSetupObject( _EeNetworkSetupObject *newObj __attribute__((unused)),
                const _EeNetworkSetupObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeDevicesObject( _EeDevicesObject *newObj __attribute__((unused)),
                const _EeDevicesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeHooksObject( _EeHooksObject *newObj __attribute__((unused)),
                const _EeHooksObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePresetupObject( _EePresetupObject *newObj __attribute__((unused)),
                const _EePresetupObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePresetupArgsObject( _EePresetupArgsObject *newObj __attribute__((unused)),
                const _EePresetupArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePrestartObject( _EePrestartObject *newObj __attribute__((unused)),
                const _EePrestartObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePrestartArgsObject( _EePrestartArgsObject *newObj __attribute__((unused)),
                const _EePrestartArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePoststartObject( _EePoststartObject *newObj __attribute__((unused)),
                const _EePoststartObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePoststartArgsObject( _EePoststartArgsObject *newObj __attribute__((unused)),
                const _EePoststartArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePoststopObject( _EePoststopObject *newObj __attribute__((unused)),
                const _EePoststopObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eePoststopArgsObject( _EePoststopArgsObject *newObj __attribute__((unused)),
                const _EePoststopArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eeNetworkObject( _EeNetworkObject *newObj __attribute__((unused)),
                const _EeNetworkObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euBusObject( _EuBusObject *newObj __attribute__((unused)),
                const _EuBusObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euProcessObject( _EuProcessObject *newObj __attribute__((unused)),
                const _EuProcessObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euProcessArgsObject( _EuProcessArgsObject *newObj __attribute__((unused)),
                const _EuProcessArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euProcessEnvObject( _EuProcessEnvObject *newObj __attribute__((unused)),
                const _EuProcessEnvObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euMountsObject( _EuMountsObject *newObj __attribute__((unused)),
                const _EuMountsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euMountsOptionObject( _EuMountsOptionObject *newObj __attribute__((unused)),
                const _EuMountsOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euLinuxObject( _EuLinuxObject *newObj __attribute__((unused)),
                const _EuLinuxObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euResourceObject( _EuResourceObject *newObj __attribute__((unused)),
                const _EuResourceObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euBlkioObject( _EuBlkioObject *newObj __attribute__((unused)),
                const _EuBlkioObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euCpuObject( _EuCpuObject *newObj __attribute__((unused)),
                const _EuCpuObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euResDevicesObject( _EuResDevicesObject *newObj __attribute__((unused)),
                const _EuResDevicesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euDevicesObject( _EuDevicesObject *newObj __attribute__((unused)),
                const _EuDevicesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euHooksObject( _EuHooksObject *newObj __attribute__((unused)),
                const _EuHooksObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPresetupObject( _EuPresetupObject *newObj __attribute__((unused)),
                const _EuPresetupObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPresetupArgsObject( _EuPresetupArgsObject *newObj __attribute__((unused)),
                const _EuPresetupArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPrestartObject( _EuPrestartObject *newObj __attribute__((unused)),
                const _EuPrestartObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPrestartArgsObject( _EuPrestartArgsObject *newObj __attribute__((unused)),
                const _EuPrestartArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPoststartObject( _EuPoststartObject *newObj __attribute__((unused)),
                const _EuPoststartObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPoststartArgsObject( _EuPoststartArgsObject *newObj __attribute__((unused)),
                const _EuPoststartArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPoststopObject( _EuPoststopObject *newObj __attribute__((unused)),
                const _EuPoststopObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euPoststopArgsObject( _EuPoststopArgsObject *newObj __attribute__((unused)),
                const _EuPoststopArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euDependencyObject( _EuDependencyObject *newObj __attribute__((unused)),
                const _EuDependencyObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euSeccompObject( _EuSeccompObject *newObj __attribute__((unused)),
                const _EuSeccompObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euSyscallsObject( _EuSyscallsObject *newObj __attribute__((unused)),
                const _EuSyscallsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_euSyscallArgsObject( _EuSyscallArgsObject *newObj __attribute__((unused)),
                const _EuSyscallArgsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_SM_BASELINE_1 */

