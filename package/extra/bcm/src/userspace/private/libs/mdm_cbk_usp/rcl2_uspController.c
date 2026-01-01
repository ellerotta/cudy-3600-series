/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
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

#if defined(DMP_DEVICE2_LOCALAGENT_1) && defined(DMP_DEVICE2_CONTROLLERS_1)

#include "cms.h"
#include "bcm_ulog.h"
#include "cms_util.h"
#include "cms_core.h"
#include "rcl.h"
#include "rut_util.h"
#include "mdm.h"
#include "cms_msg_usp.h"


static CmsRet sendControllerChanged(const char *fullpath, UBOOL8 enable,
                                    UINT32 interval, const char *notifTime,
                                    UBOOL8 isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspControllerChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspControllerChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_CONTROLLER_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspControllerChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspControllerChangedMsgBody*)body;

   msgPayload->enable = enable;
   msgPayload->isDelete = isDelete;
   msgPayload->interval = interval;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->notifTime, notifTime,
                  sizeof(msgPayload->notifTime));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendControllerMtpChanged(const char *fullpath, UBOOL8 enable,
                                       const char *protocol, UBOOL8 isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspControllerMtpChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(UspControllerMtpChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_CONTROLLER_MTP_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspControllerMtpChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspControllerMtpChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->enable = enable;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->protocol, protocol, sizeof(msgPayload->protocol));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendMtpMqttChanged
   (const char *fullpath, const char *reference, const char *topic,
    int publishRetainResponse, int publishRetainNotify,
    UBOOL8 isDelete, UBOOL8 activeMtp)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspCntrlMqttChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspCntrlMqttChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_CNTRL_MQTT_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspCntrlMqttChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspCntrlMqttChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->activeMtp = activeMtp;
   msgPayload->publishRetainResponse = publishRetainResponse;
   msgPayload->publishRetainNotify = publishRetainNotify;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->reference, reference, sizeof(msgPayload->reference));
   cmsUtl_strncpy(msgPayload->topic, topic, sizeof(msgPayload->topic));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Send CMS_MSG_CNTRL_MQTT_CHANGE to obuspa failed, ret=%d", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendE2eSessionChanged(const char *fullpath)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   E2eSessionChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(E2eSessionChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_E2E_SESSION_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(E2eSessionChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (E2eSessionChangedMsgBody*)body;

   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Send CMS_MSG_E2E_SESSION_CHANGE to obuspa failed, ret=%d", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet areReferencesValid
   (const MdmObjectId oid, const char *references, int onlyOneReference)
{
   int len = 0, count = 0;
   char *info = NULL;
   char *pToken = NULL, *pLast = NULL;
   void *mdmObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret = CMSRET_SUCCESS;

   info = strdup(references);

   for (pToken = strtok_r(info, ",", &pLast), count = 1;
        pToken != NULL;
        pToken = strtok_r(NULL, ",", &pLast), count++)
   {
      /* return error if references contains more than one when
       * it must have only one reference */
      if (onlyOneReference == TRUE && count > 1)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      /* return error if reference is empty
       * AND deletion is NOT in progress */
      if (IS_EMPTY_STRING(pToken) == TRUE &&
          mdmLibCtx.hideObjectsPendingDelete == TRUE)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      len = cmsUtl_strlen(pToken);

      /* return error when reference has '.' at the end */
      if (pToken[len-1] == '.')
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));

      cmsMdm_fullPathToPathDescriptor(pToken, &pathDesc);

      if (pathDesc.oid == oid)
      {
         ret = cmsObj_get(oid, &(pathDesc.iidStack),
                          OGF_NO_VALUE_UPDATE, &mdmObj);

         if (ret == CMSRET_SUCCESS)
         {
            cmsObj_free(&mdmObj);
         }
         else
         {
            /* return error when reference cannot be found */
            ret = CMSRET_INVALID_PARAM_VALUE;
            break;
         }
      }
   }

   free(info);

   return ret;
}

CmsRet rcl_dev2LocalagentControllerObject(_Dev2LocalagentControllerObject *newObj,
                                const _Dev2LocalagentControllerObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumControllerEntry_dev2(iidStack, 1);
      return ret;
   }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLER;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if (IS_EMPTY_STRING(currObj->assignedRole))
      {
         /*
          * If a controller is not configured with assignedRole,
          * assignedRole needs to be set to ControllerTurst.UntrustedRole
          */
         if (IS_EMPTY_STRING(newObj->assignedRole))
         {
            Dev2LocalagentControllertrustObject *trustObj = NULL;
            InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;

            ret = cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST, &iidStack1,
                             OGF_NO_VALUE_UPDATE, (void **)&trustObj);

            if (ret == CMSRET_SUCCESS)
            {
               if (!IS_EMPTY_STRING(trustObj->untrustedRole))
               {
                  REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->assignedRole,
                                 trustObj->untrustedRole, mdmLibCtx.allocFlags);
               }

               cmsObj_free((void **)&trustObj);
            }
         }
      }

      if (cmsUtl_strcmp(newObj->assignedRole, currObj->assignedRole))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                  newObj->assignedRole, FALSE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Assigned role <%s> has invalid value.",
                         newObj->assignedRole);
            goto out;
         }
      }

      if (IS_EMPTY_STRING(newObj->inheritedRole) == FALSE &&
          cmsUtl_strcmp(newObj->inheritedRole, currObj->inheritedRole))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                  newObj->inheritedRole, FALSE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Inherited role <%s> has invalid value.",
                         newObj->inheritedRole);
            goto out;
         }
      }

      if (cmsUtl_strcmp(newObj->credential, currObj->credential))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CERTIFICATE,
                                  newObj->credential, TRUE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Credential <%s> has invalid value.",
                         newObj->credential);
            goto out;
         }
      }

      if ((newObj->enable != currObj->enable) ||
          (newObj->periodicNotifInterval != currObj->periodicNotifInterval) ||
          cmsUtl_strcmp(newObj->periodicNotifTime, currObj->periodicNotifTime))
      {
         ret = sendControllerChanged(fullpath, currObj->enable,
                                     currObj->periodicNotifInterval,
                                     currObj->periodicNotifTime, FALSE);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumControllerEntry_dev2(iidStack, -1);
      ret = sendControllerChanged(fullpath, currObj->enable,
                                  currObj->periodicNotifInterval,
                                  currObj->periodicNotifTime, TRUE);

#ifdef DMP_DEVICE2_SUBSCRIPTIONS_1
      {
         UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
         InstanceIdStack subIidStack=EMPTY_INSTANCE_ID_STACK;
         InstanceIdStack savedIidStack=EMPTY_INSTANCE_ID_STACK;
         Dev2LocalagentSubscriptionObject *subObj=NULL;

         mdmLibCtx.hideObjectsPendingDelete = FALSE;

         /* delete subscription instances that have this controller as recipient. */
         while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_SUBSCRIPTION, &subIidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&subObj) == CMSRET_SUCCESS)
         {
            if (cmsUtl_strcmp(subObj->recipient, fullpath) == 0)
            {
               cmsObj_deleteInstance(MDMOID_DEV2_LOCALAGENT_SUBSCRIPTION, &subIidStack);

               /* since we did a delete, restore iidStack to last good one */
               subIidStack = savedIidStack;
            }
            cmsObj_free((void **)&subObj);

            /* save current iidStack in case we delete the next one */
            savedIidStack = subIidStack;
         }

         mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
      }
#endif
   }

out:

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}  /* End of rcl_dev2LocalagentControllerObject() */


CmsRet rcl_dev2LocalagentControllerMtpObject(_Dev2LocalagentControllerMtpObject *newObj,
                                const _Dev2LocalagentControllerMtpObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      if (IS_EMPTY_STRING(newObj->alias))
      {
         char alias[BUFLEN_256]={0};

         snprintf(alias, sizeof(alias), "cpe-controller-mtp-%d-%d",
                  INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-2),
                  PEEK_INSTANCE_ID(iidStack));

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->alias, alias, mdmLibCtx.allocFlags);
      }

      rutUtil_modifyNumControllerMtpEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      MdmPathDescriptor pathDesc;
      char *fullpath = NULL;

      rutUtil_modifyNumControllerMtpEntry_dev2(iidStack, -1);

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP;
      pathDesc.iidStack = *iidStack;

      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

      ret = sendControllerMtpChanged(fullpath, currObj->enable, currObj->protocol, TRUE);

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   }
   else if (newObj && currObj)
   {
      MdmPathDescriptor pathDesc;
      char *fullpath = NULL;

      if (newObj->enable == TRUE &&
          cmsUtl_strcmp(newObj->protocol, MDMVS_WEBSOCKET) == 0)
      {
         Dev2LocalagentControllerMtpWebsocketObject *wsObj = NULL;

         if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_WEBSOCKET,
                        iidStack,
                        0,
                        (void **) &wsObj) == CMSRET_SUCCESS)
         {
            if (wsObj->host == NULL || wsObj->host[0] == '\0' ||  
                wsObj->path == NULL || wsObj->path[0] == '\0' ||  
                wsObj->port == 0) 
            {
               cmsObj_free((void **) &wsObj);
               return CMSRET_INVALID_PARAM_VALUE;
            }

            cmsObj_free((void **) &wsObj);
         }
      }

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP;
      pathDesc.iidStack = *iidStack;

      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

      if ((newObj->enable != currObj->enable) ||
          cmsUtl_strcmp(newObj->protocol, currObj->protocol))
      {
         ret = sendControllerMtpChanged(fullpath, currObj->enable, currObj->protocol, FALSE);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
   }

   return ret;

}  /* End of rcl_dev2LocalagentControllerMtpObject() */ 


#ifdef DMP_DEVICE2_STOMPCONTROLLER_1
CmsRet rcl_dev2LocalagentControllerMtpStompObject(_Dev2LocalagentControllerMtpStompObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentControllerMtpStompObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      Dev2LocalagentControllerMtpObject *mtpObj=NULL;

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(mtpObj->protocol, MDMVS_STOMP) == 0)
         {
            ret = areReferencesValid(MDMOID_DEV2_STOMP_CONNECTION,
                                     newObj->reference, TRUE);

            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("STOMP reference <%s> has invalid value.",
                            newObj->reference);
            }
         }
         else
         {
            /* if mtpObj->protocol is not stomp,
             * reset stompObj to default value
             */
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->reference);
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->destination);
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get controller mtp");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   return ret;
}
#endif /* DMP_DEVICE2_STOMPCONTROLLER_1 */


#ifdef DMP_DEVICE2_WEBSOCKETCONTROLLER_1
static CmsRet sendMtpWebsocketChanged(const char *fullpath, const char *host,
                                      const char *path, int port, int encrypt,
                                      int interval, int retryInterval,
                                      int retryMulti,
                                      UBOOL8 isDelete, UBOOL8 activeMtp)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspCntrlWebsocketChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspCntrlWebsocketChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_CNTRL_WEBSOCKET_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspCntrlWebsocketChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspCntrlWebsocketChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->activeMtp = activeMtp;
   msgPayload->port = port;
   msgPayload->encrypt = encrypt;
   msgPayload->interval = interval;
   msgPayload->retryInterval = retryInterval;
   msgPayload->retryMulti = retryMulti;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->host, host, sizeof(msgPayload->host));
   cmsUtl_strncpy(msgPayload->path, path, sizeof(msgPayload->path));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("send CMS_MSG_CNTRL_WEBSOCKET_CHANGE failed, ret<%d>", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2LocalagentControllerMtpWebsocketObject(_Dev2LocalagentControllerMtpWebsocketObject *newObj,
                     const _Dev2LocalagentControllerMtpWebsocketObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   Dev2LocalagentControllerMtpObject *mtpObj = NULL;

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_WEBSOCKET;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(mtpObj->protocol, MDMVS_WEBSOCKET) == 0)
         {
            if (IS_EMPTY_STRING(newObj->host) || newObj->port < 1 ||
                newObj->port > 65535 || newObj->keepAliveInterval < 1)
            {
               cmsLog_error("Empty wsHost or invalid wsPort<%d> or invalid interval<%d>",
                            newObj->port, newObj->keepAliveInterval);
               ret = CMSRET_INVALID_ARGUMENTS;
               cmsObj_free((void **)&mtpObj);
               goto out;
            }

            if (cmsUtl_strcmp(newObj->host, currObj->host) ||
                cmsUtl_strcmp(newObj->path, currObj->path) ||
                newObj->port != currObj->port ||
                newObj->keepAliveInterval != currObj->keepAliveInterval ||
                newObj->sessionRetryMinimumWaitInterval != currObj->sessionRetryMinimumWaitInterval ||
                newObj->sessionRetryIntervalMultiplier != currObj->sessionRetryIntervalMultiplier ||
                newObj->enableEncryption != currObj->enableEncryption)
            {
               ret = sendMtpWebsocketChanged(fullpath, currObj->host,
                                             currObj->path, currObj->port,
                                             currObj->enableEncryption,
                                             currObj->keepAliveInterval, 
                                             currObj->sessionRetryMinimumWaitInterval,
                                             currObj->sessionRetryIntervalMultiplier,
                                             FALSE, FALSE);
            }
         }
         else
         {
            /* if mtpObj->protocol is not websocket,
             * reset wsObj to default value
             */
            newObj->port = 5683;
            newObj->keepAliveInterval = 0;
            newObj->sessionRetryMinimumWaitInterval = 5;
            newObj->sessionRetryIntervalMultiplier = 2000;
            newObj->enableEncryption = TRUE;
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->host);
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->path);
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get controller mtp");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete;
      UBOOL8 enabledWebsocketMtp = TRUE;
      UBOOL8 sendMTPChangeMsg = TRUE;

      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (strcmp(mtpObj->protocol, MDMVS_WEBSOCKET) != 0 || mtpObj->enable == FALSE)
         {
            cmsLog_notice("Do not sendMtpWebsocketChanged for MTP Websockets if protocol is not WebSocket or MTP protocol is not enable");
            sendMTPChangeMsg = FALSE;
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get controller mtp<%s>", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
         sendMTPChangeMsg = FALSE;
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      if (sendMTPChangeMsg == TRUE)
      {
         ret = sendMtpWebsocketChanged(fullpath, currObj->host,
                                       currObj->path, currObj->port,
                                       currObj->enableEncryption,
                                       currObj->keepAliveInterval, 
                                       currObj->sessionRetryMinimumWaitInterval,
                                       currObj->sessionRetryIntervalMultiplier,
                                       TRUE, enabledWebsocketMtp);
      }
   }

out:
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}


#endif /* DMP_DEVICE2_WEBSOCKETCONTROLLER_1 */


#ifdef DMP_DEVICE2_MQTTCONTROLLER_1
CmsRet rcl_dev2LocalagentControllerMtpMqttObject(_Dev2LocalagentControllerMtpMqttObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentControllerMtpMqttObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   Dev2LocalagentControllerMtpObject *mtpObj = NULL;

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_MQTT;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(mtpObj->protocol, MDMVS_MQTT) == 0)
         {
            ret = areReferencesValid(MDMOID_DEV2_MQTT_CLIENT,
                                     newObj->reference, TRUE);

            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("MQTT reference has <%s> invalid value.",
                            newObj->reference);
               cmsObj_free((void **)&mtpObj);
               goto out;
            }

            if (cmsUtl_strcmp(newObj->reference, currObj->reference) ||
                cmsUtl_strcmp(newObj->topic, currObj->topic) ||
                newObj->publishRetainResponse != currObj->publishRetainResponse ||
                newObj->publishRetainNotify != currObj->publishRetainNotify)
            {
               ret = sendMtpMqttChanged(fullpath, currObj->reference,
                                        currObj->topic,
                                        currObj->publishRetainResponse,
                                        currObj->publishRetainNotify, FALSE,
                                        FALSE);
            }
         }
         else
         {
            /* if mtpObj->protocol is not mqtt,
             * reset mqttObj to default value
             */
            newObj->publishRetainResponse = FALSE;
            newObj->publishRetainNotify = FALSE;
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->reference);
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->topic);
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get controller mtp");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete;
      UBOOL8 enabledMqttMtp = TRUE;
      UBOOL8 sendMTPChangeMsg = TRUE;

      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (strcmp(mtpObj->protocol, MDMVS_MQTT) != 0 || mtpObj->enable == FALSE)
         {
            cmsLog_notice("Do not sendMtpMqttChanged for MTP MQTT if protocol is not MQTT or MTP protocol is not enable");
            sendMTPChangeMsg = FALSE;
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get controller mtp <%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
         sendMTPChangeMsg = FALSE;
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      if (sendMTPChangeMsg == TRUE)
      {
         ret = sendMtpMqttChanged(fullpath, currObj->reference,
                                  currObj->topic,
                                  currObj->publishRetainResponse,
                                  currObj->publishRetainNotify, TRUE,
                                  enabledMqttMtp);
      }
   }

out:
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}
#endif /* DMP_DEVICE2_MQTTCONTROLLER_1 */


CmsRet rcl_dev2LocalagentControllerTransfercompletepolicyObject(_Dev2LocalagentControllerTransfercompletepolicyObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentControllerTransfercompletepolicyObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#ifdef DMP_DEVICE2_REBOOT_1
CmsRet rcl_dev2LocalagentControllerBootparameterObject(_Dev2LocalagentControllerBootparameterObject *newObj,
                                const _Dev2LocalagentControllerBootparameterObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBootParameterEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBootParameterEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif /* DMP_DEVICE2_REBOOT_1 */


CmsRet rcl_dev2LocalagentControllerE2esessionObject(_Dev2LocalagentControllerE2esessionObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentControllerE2esessionObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      return CMSRET_SUCCESS;
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      return CMSRET_SUCCESS;
   }

   if ((newObj->sessionMode != NULL && currObj->sessionMode != NULL &&
        cmsUtl_strcmp(newObj->sessionMode, currObj->sessionMode) != 0) ||
       (newObj->payloadSecurity != NULL && currObj->payloadSecurity != NULL &&
        cmsUtl_strcmp(newObj->payloadSecurity, currObj->payloadSecurity) != 0) ||
       newObj->sessionExpiration != currObj->sessionExpiration ||
       newObj->sessionRetryMinimumWaitInterval != currObj->sessionRetryMinimumWaitInterval ||
       newObj->sessionRetryIntervalMultiplier != currObj->sessionRetryIntervalMultiplier ||
       newObj->segmentedPayloadChunkSize != currObj->segmentedPayloadChunkSize ||
       newObj->maxRetransmitTries != currObj->maxRetransmitTries)
   {
      char *fullpath = NULL;
      MdmPathDescriptor pathDesc;

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLER_E2ESESSION;
      pathDesc.iidStack = *iidStack;

      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

      sendE2eSessionChanged(fullpath);

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
   }

   return CMSRET_SUCCESS;
}


#endif /* DMP_DEVICE2_LOCALAGENT_1 && DMP_DEVICE2_CONTROLLERS_1 */


