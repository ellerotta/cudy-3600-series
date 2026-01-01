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

#ifdef DMP_DEVICE2_LOCALAGENT_1

#include "cms.h"
#include "bcm_ulog.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "cms_tms.h"
#include "cms_core.h"
#include "rut_util.h"
#include "mdm.h"
#include "cms_msg_usp.h"


extern CmsRet areReferencesValid
   (const MdmObjectId oid, const char *references, int onlyOneReference);
static UBOOL8 onlyOneWsServerEnabled(MdmPathDescriptor *pathDesc);

CmsRet rcl_dev2LocalagentObject(_Dev2LocalagentObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


static CmsRet sendMtpChanged(const char *fullpath, UBOOL8 enable,
                             UBOOL8 enableMDNS, const char *protocol)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspAgentMtpChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(UspAgentMtpChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_AGENT_MTP_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspAgentMtpChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspAgentMtpChangedMsgBody*)body;

   msgPayload->enable = enable;
   msgPayload->enableMDNS = enableMDNS;
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

static CmsRet sendMtpStompChanged(const char *fullpath, const char *ref,
                                  const char *dest, const char *serverDest,
                                  UBOOL8 isDelete, UBOOL8 activeMtp)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspAgentStompChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspAgentStompChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_AGENT_STOMP_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspAgentStompChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspAgentStompChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->activeMtp = activeMtp;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->reference, ref, sizeof(msgPayload->reference));
   cmsUtl_strncpy(msgPayload->destination, dest,
                  sizeof(msgPayload->destination));
   cmsUtl_strncpy(msgPayload->destinationFromServer, serverDest,
                  sizeof(msgPayload->destinationFromServer));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendPermChanged(const char *fullpath, UBOOL8 enable, UINT32 order,
                              const char *targets, const char *param,
                              const char *obj, const char *instObj,
                              const char *event, UBOOL8 isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspRolePermChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspRolePermChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_ROLE_PERM_CONFIG_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspRolePermChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspRolePermChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->enable = enable;
   msgPayload->order = order;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->targets, targets, sizeof(msgPayload->targets));
   cmsUtl_strncpy(msgPayload->param, param, sizeof(msgPayload->param));
   cmsUtl_strncpy(msgPayload->obj, obj, sizeof(msgPayload->obj));
   cmsUtl_strncpy(msgPayload->instObj, instObj, sizeof(msgPayload->instObj));
   cmsUtl_strncpy(msgPayload->event, event, sizeof(msgPayload->event));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendMtpMqttChanged
   (const char *fullpath, const char *reference,
    const char *responseTopicConfigured, const char *responseTopicDiscovered,
    UINT32 publishQoS, UBOOL8 isDelete, UBOOL8 activeMtp)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspAgentMqttChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspAgentMqttChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_AGENT_MQTT_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspAgentMqttChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspAgentMqttChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->activeMtp = activeMtp;
   msgPayload->publishQoS = publishQoS;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->reference, reference, sizeof(msgPayload->reference));
   cmsUtl_strncpy(msgPayload->responseTopicConfigured, responseTopicConfigured,
                  sizeof(msgPayload->responseTopicConfigured));
   cmsUtl_strncpy(msgPayload->responseTopicDiscovered, responseTopicDiscovered,
                  sizeof(msgPayload->responseTopicDiscovered));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Send CMS_MSG_AGENT_MQTT_CHANGE to obuspa failed, ret=%d", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2LocalagentMtpObject(_Dev2LocalagentMtpObject *newObj,
                                const _Dev2LocalagentMtpObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumAgentMtpEntry_dev2(iidStack, 1);
      return ret;
   }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_MTP;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if ((newObj->enable != currObj->enable) ||
          (newObj->enableMDNS != currObj->enableMDNS) ||
          cmsUtl_strcmp(newObj->protocol, currObj->protocol))
      {
#ifdef DMP_DEVICE2_WEBSOCKETAGENT_1
         /* can only support one active ws server now */
         if (newObj->enable &&
             !cmsUtl_strcmp(newObj->protocol, MDMVS_WEBSOCKET) &&
             !onlyOneWsServerEnabled(&pathDesc))
         {
            cmsLog_error("can only support one active websocket server");
            ret = CMSRET_REQUEST_DENIED;
            goto out;
         }
#endif
         ret = sendMtpChanged(fullpath, currObj->enable, currObj->enableMDNS,
                              currObj->protocol);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumAgentMtpEntry_dev2(iidStack, -1);
   }

out:
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;

}  /* End of rcl_dev2LocalagentMtpObject() */


#ifdef DMP_DEVICE2_STOMPAGENT_1
CmsRet rcl_dev2LocalagentMtpStompObject(_Dev2LocalagentMtpStompObject *newObj,
                                const _Dev2LocalagentMtpStompObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   Dev2LocalagentMtpObject *mtpObj = NULL;

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_MTP_STOMP;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, iidStack,
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
               goto out;
            }

            if (cmsUtl_strcmp(newObj->reference, currObj->reference) ||
                cmsUtl_strcmp(newObj->destination, currObj->destination))
            {
               ret = sendMtpStompChanged(fullpath, currObj->reference,
                                         currObj->destination,
                                         currObj->destinationFromServer, FALSE,
                                         FALSE);
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
         cmsLog_error("cannot get stomp<%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete;
      UBOOL8 enabledStompMtp = FALSE;

      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (mtpObj->enable && !strcmp(mtpObj->protocol, MDMVS_STOMP))
         {
            enabledStompMtp = TRUE;
         }
         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get stomp<%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
         goto out;
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      ret = sendMtpStompChanged(fullpath, currObj->reference,
                                currObj->destination,
                                currObj->destinationFromServer, TRUE,
                                enabledStompMtp);
   }

out:
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}
#endif /* DMP_DEVICE2_STOMPAGENT_1 */


#ifdef DMP_DEVICE2_WEBSOCKETAGENT_1
static UBOOL8 onlyOneWsServerEnabled(MdmPathDescriptor *pathDesc)
{
   Dev2LocalagentMtpObject *mtpObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 ret = TRUE;

   while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_MTP, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&mtpObj) == CMSRET_SUCCESS)
   {
      if (mtpObj->enable && !cmsUtl_strcmp(mtpObj->protocol, MDMVS_WEBSOCKET))
      {
         int mtpInstance;
         int pathInstance;

         mtpInstance = PEEK_INSTANCE_ID(&iidStack);
         pathInstance = PEEK_INSTANCE_ID(&(pathDesc->iidStack));

         if (mtpInstance != pathInstance)
         {
            ret = FALSE;
         }
      }
      cmsObj_free((void **)&mtpObj);
   }

   return ret;
}

static CmsRet sendMtpWebsocketChanged(const char *fullpath,
                                      UBOOL8 isDelete, UBOOL8 activeMtp)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspAgentWebSocketChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspAgentWebSocketChangedMsgBody), ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_AGENT_WEBSOCKET_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspAgentWebSocketChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspAgentWebSocketChangedMsgBody*)body;

   msgPayload->isDelete = isDelete;
   msgPayload->activeMtp = activeMtp;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa (ret=%d)", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2LocalagentMtpWebsocketObject(_Dev2LocalagentMtpWebsocketObject *newObj,
                               const _Dev2LocalagentMtpWebsocketObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   Dev2LocalagentMtpObject *mtpObj = NULL;

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_MTP_WEBSOCKET;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(mtpObj->protocol, MDMVS_WEBSOCKET) == 0)
         {
            if (IS_EMPTY_STRING(newObj->interfaces) == FALSE)
            {
               ret = areReferencesValid(MDMOID_DEV2_IP_INTERFACE,
                                        newObj->interfaces, FALSE);

               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Interfaces <%s> has invalid value.",
                               newObj->interfaces);
                  cmsObj_free((void **)&mtpObj);
                  goto out;
               }
            }

            if (newObj->port < 1 || newObj->port > 65535 ||
                IS_EMPTY_STRING(newObj->path))
            {
               cmsLog_error("invalid websocket configuration");
               ret = CMSRET_INVALID_ARGUMENTS;
               cmsObj_free((void **)&mtpObj);
               goto out;
            }

            if ((newObj->port != currObj->port) ||
                (newObj->enableEncryption != currObj->enableEncryption) ||
                cmsUtl_strcmp(newObj->interfaces, currObj->interfaces) ||
                cmsUtl_strcmp(newObj->path, currObj->path))
            {
               ret = sendMtpWebsocketChanged(fullpath, FALSE, FALSE);
            }
         }
         else
         {
            /* if mtpObj->protocol is not websocket,
             * reset wsObj to default value
             */
            newObj->port = 5683;
            newObj->enableEncryption = TRUE;
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->interfaces);
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->path);
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get websocket<%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete;
      UBOOL8 enabledWebsocketMtp = FALSE;

      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (mtpObj->enable && !strcmp(mtpObj->protocol, MDMVS_WEBSOCKET))
         {
            enabledWebsocketMtp = TRUE;
         }
         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get websocket<%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
         goto out;
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      ret = sendMtpWebsocketChanged(fullpath, TRUE, enabledWebsocketMtp);
   }

out:
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}
#endif /* DMP_DEVICE2_WEBSOCKETAGENT_1 */


#ifdef DMP_DEVICE2_MQTTAGENT_1
CmsRet rcl_dev2LocalagentMtpMqttObject(_Dev2LocalagentMtpMqttObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentMtpMqttObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   Dev2LocalagentMtpObject *mtpObj = NULL;

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_MTP_MQTT;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(mtpObj->protocol, MDMVS_MQTT) == 0)
         {
            ret = areReferencesValid(MDMOID_DEV2_MQTT_CLIENT,
                                     newObj->reference, TRUE);

            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("MQTT reference <%s> has invalid value.",
                            newObj->reference);
               goto out;
            }

            if (cmsUtl_strcmp(newObj->reference, currObj->reference) ||
                cmsUtl_strcmp(newObj->responseTopicConfigured, currObj->responseTopicConfigured) ||
                newObj->publishQoS != currObj->publishQoS)
            {
               ret = sendMtpMqttChanged(fullpath, currObj->reference,
                                        currObj->responseTopicConfigured,
                                        currObj->responseTopicDiscovered,
                                        currObj->publishQoS, FALSE,
                                        FALSE);
            }
         }
         else
         {
            /* if mtpObj->protocol is not mqtt,
             * reset mqttObj to default value
             */
            newObj->publishQoS = 0;
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->reference);
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->responseTopicConfigured);
         }

         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get agent mtp <%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete;
      UBOOL8 enabledMqttMtp = FALSE;

      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_MTP, iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mtpObj) == CMSRET_SUCCESS)
      {
         if (mtpObj->enable && !strcmp(mtpObj->protocol, MDMVS_MQTT))
         {
            enabledMqttMtp = TRUE;
         }
         cmsObj_free((void **)&mtpObj);
      }
      else
      {
         cmsLog_error("cannot get agent mtp <%s> mtp", fullpath);
         ret = CMSRET_INTERNAL_ERROR;
         goto out;
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      ret = sendMtpMqttChanged(fullpath, currObj->reference,
                               currObj->responseTopicConfigured,
                               currObj->responseTopicDiscovered,
                               currObj->publishQoS, TRUE,
                               enabledMqttMtp);
   }

out:
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}
#endif /* DMP_DEVICE2_MQTTAGENT_1 */


#ifdef DMP_DEVICE2_SUBSCRIPTIONS_1

#define AUTOGEN_SUBID_PREFIX  "cpe-subid"

static CmsRet sendSubscriptionChanged(UINT32 instance, UBOOL8 isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspSubscriptionChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspSubscriptionChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_SUBSCRIPTION_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspSubscriptionChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspSubscriptionChangedMsgBody*)body;

   msgPayload->instance = instance;
   msgPayload->isDelete = isDelete;

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to obuspa. instance=%d isDelete=%d ret=%d",
                   instance, isDelete, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;

}  /* End of sendSubscriptionChanged() */

CmsRet rcl_dev2LocalagentSubscriptionObject(_Dev2LocalagentSubscriptionObject *newObj,
                                const _Dev2LocalagentSubscriptionObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 instance;

   instance = INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-1);

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      char timeStr[BUFLEN_128];

      rutUtil_modifyNumSubscriptionEntry_dev2(iidStack, 1);

      cmsTms_getXSIDateTime(0, timeStr, sizeof(timeStr));
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->creationDate, timeStr, mdmLibCtx.allocFlags);
      return ret;
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_EXISTING");

      rutUtil_modifyNumSubscriptionEntry_dev2(iidStack, -1);
      ret = sendSubscriptionChanged(instance, TRUE);
      return ret;
   }

   if (newObj && currObj)
   {
      cmsLog_debug("UPDATE");

      InstanceIdStack iidStack2=EMPTY_INSTANCE_ID_STACK;
      Dev2LocalagentSubscriptionObject *obj=NULL;
      UBOOL8 uniqueID=TRUE;

      if (IS_EMPTY_STRING(newObj->recipient))
      {
         cmsLog_error("Recipient cannot be empty");
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (IS_EMPTY_STRING(currObj->recipient))
      {
         /* This must be the set for auto-fill recipient at the create of
          * the subscription -- add, and then set of recipient only.
          * Other parameters are with default values. No need to validate
          * them at this time.
          */
         goto out;
      }
      
      if (cmsUtl_strcmp(newObj->recipient, currObj->recipient))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLER,
                                  newObj->recipient, TRUE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Recipient <%s> has invalid value.",
                         newObj->recipient);
            return ret;
         }
      }

      /* Per data-model: Once the value of the ReferenceList is written, the value
       * cannot be changed as the Subscription instance is considered to be immutable.
       * If the value of a non-empty ReferenceList parameter needs to change, the
       * Subscription instance MUST be deleted and a new Subscription instance created.
       */
      if (!IS_EMPTY_STRING(currObj->referenceList) &&
          cmsUtl_strcmp(newObj->referenceList, currObj->referenceList))
      {
         cmsLog_error("referenceList cannot change");
         return CMSRET_INVALID_ARGUMENTS;
      }

      /* Per data-model: ID is immutable and therefore MUST NOT change once
       * it has been assigned.
       *
       * When subscription object is created, CMS autogenerates ID.
       * To support ID can be changed once. We will only allow ID to be changed
       * if current ID is in autogenerated format.
       * In addition, the length of ID string is between 1 and 64
       */
      if (cmsUtl_strcmp(newObj->ID, currObj->ID) != 0)
      {
         if ((cmsUtl_strncmp(currObj->ID, AUTOGEN_SUBID_PREFIX,
                             strlen(AUTOGEN_SUBID_PREFIX)) != 0) ||
             (newObj->ID &&
              (strlen(newObj->ID) < 1 || strlen(newObj->ID) > 64)))
         {
            cmsLog_error("invalid ID update");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }

      /* validate ID is a unique value within the namespace of each controller.
       * Note: Since the agent does not auto generate value for empty ID,
       * we want to skip checking against existing subscriptions with empty ID.
       */
      while (uniqueID &&
             cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_SUBSCRIPTION, &iidStack2,
                                 OGF_NO_VALUE_UPDATE, (void **)&obj) == CMSRET_SUCCESS)
      {
         if ((instance != INSTANCE_ID_AT_DEPTH(&iidStack2, DEPTH_OF_IIDSTACK(&iidStack2)-1)) &&
             (cmsUtl_strcmp(obj->recipient, newObj->recipient) == 0) &&
             (cmsUtl_strcmp(obj->ID, newObj->ID) == 0))
         {
            uniqueID = FALSE;
         }
         cmsObj_free((void **)&obj);
      }
      if (uniqueID == FALSE)
      {
         cmsLog_error("Subscription ID %s is not unique", newObj->ID);
         return CMSRET_INVALID_ARGUMENTS;
      }

out:
      ret = sendSubscriptionChanged(instance, FALSE);
   }

   return ret;

}  /* End of rcl_dev2LocalagentSubscriptionObject() */
#endif /* DMP_DEVICE2_SUBSCRIPTIONS_1 */


CmsRet rcl_dev2LocalagentRequestObject(_Dev2LocalagentRequestObject *newObj,
                                const _Dev2LocalagentRequestObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumLocalAgentRequestEntry_dev2(iidStack, 1);
   }

   if (newObj && currObj)
   {
      cmsLog_debug("update[%d] command=%s commandKey=%s status=%s",
                   PEEK_INSTANCE_ID(iidStack),
                   newObj->command, newObj->commandKey, newObj->status);
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("delete[%d] (command=%s commandKey=%s)",
                   PEEK_INSTANCE_ID(iidStack), currObj->command, currObj->commandKey);
      rutUtil_modifyNumLocalAgentRequestEntry_dev2(iidStack, -1);
   }
   
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2LocalagentRequestArgsObject(_Dev2LocalagentRequestArgsObject *newObj __attribute__((unused)),
                                           const _Dev2LocalagentRequestArgsObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2LocalagentCertificateObject(_Dev2LocalagentCertificateObject *newObj,
                                const _Dev2LocalagentCertificateObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      rutUtil_modifyNumAgentCertificateEntry_dev2(iidStack, 1);

      return ret;
   }

   if (newObj && currObj)
   {
      char alias[BUFLEN_256]={0};
      UBOOL8 found = FALSE;
      UINT32 id = 0;
      UINT32 instance = (UINT32) PEEK_INSTANCE_ID(iidStack);
      Dev2LocalagentCertificateObject *certObj = NULL;
      InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;

      cmsLog_debug("UPDATE");

      if (IS_EMPTY_STRING(newObj->alias) && IS_EMPTY_STRING(currObj->alias))
      {
         /* Per data-model: If the value is not assigned by the 
          * Controller at creation time, the Agent MUST assign
          * a value with an "cpe-" prefix.
          */
         snprintf(alias, sizeof(alias), "cpe-certificate-%d",
                  PEEK_INSTANCE_ID(iidStack));

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->alias, alias,
                                           mdmLibCtx.allocFlags);
      }
      else if (!IS_EMPTY_STRING(newObj->alias) && !IS_EMPTY_STRING(currObj->alias))
      {
         /* Per data-model: Alias is immutable and therefore
          * MUST NOT change once it has been assigned.
          */
         if (cmsUtl_strcmp(newObj->alias, currObj->alias) != 0)
         {
            cmsLog_error("Alias cannot be changed.");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      else if (IS_EMPTY_STRING(newObj->alias) && !IS_EMPTY_STRING(currObj->alias))
      {
         /* Per data-model: Alias is immutable and therefore
          * MUST NOT change once it has been assigned.
          */
         cmsLog_error("Alias cannot be changed.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      /* Per data-model: At most one entry in this table can exist 
       * with the same values for SerialNumber and Issuer
       */

      /* look for certObj matching with issuer and serial number*/
      while (found == FALSE &&
             cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CERTIFICATE,
                                 &iidStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&certObj) == CMSRET_SUCCESS)
      {
         id = (UINT32) PEEK_INSTANCE_ID(&iidStack2);

         if (instance != id &&
             cmsUtl_strcmp(newObj->issuer, certObj->issuer) == 0 &&
             cmsUtl_strcmp(newObj->serialNumber, certObj->serialNumber) == 0)
         {
            found = TRUE;
         }

         /* free local agent certificate */
         cmsObj_free((void **) &certObj);
      }

      /* return error if issuer and serial number are already present */
      if (found == TRUE)
      {
         cmsLog_error("Serial number <%s> and issuer <%s> are already present.",
                      newObj->serialNumber, newObj->issuer);
         ret = CMSRET_INVALID_ARGUMENTS;
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      char *fullpath = NULL;
      MdmPathDescriptor pathDesc;
      Dev2LocalagentControllerObject *cntrlObj = NULL;
      Dev2LocalagentControllertrustCredentialObject *credentialObj = NULL;
      InstanceIdStack iiStack2;

      cmsLog_debug("DELETE_EXISTING");

      rutUtil_modifyNumAgentCertificateEntry_dev2(iidStack, -1);

      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CERTIFICATE;
      pathDesc.iidStack = *iidStack;

      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

      INIT_INSTANCE_ID_STACK(&iiStack2);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER,
                                 &iiStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&cntrlObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_isFullPathInCSL(fullpath, cntrlObj->credential) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, cntrlObj->credential);

            if ((ret = cmsObj_setFlags(cntrlObj, &iiStack2,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set Dev2LocalagentControllerObject failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &cntrlObj);
      }

      INIT_INSTANCE_ID_STACK(&iiStack2);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL,
                                 &iiStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&credentialObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(credentialObj->credential, fullpath) == 0)
         {
            CMSMEM_REPLACE_STRING_FLAGS(credentialObj->credential, "",
                                        mdmLibCtx.allocFlags);

            if ((ret = cmsObj_setFlags(credentialObj, &iiStack2,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set Dev2LocalagentControllertrustCredentialObject failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &credentialObj);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   return ret;
}


#ifdef DMP_DEVICE2_CONTROLLERTRUST_1
CmsRet rcl_dev2LocalagentControllertrustObject(_Dev2LocalagentControllertrustObject *newObj __attribute__((unused)),
                                const _Dev2LocalagentControllertrustObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      if (cmsUtl_strcmp(newObj->untrustedRole, currObj->untrustedRole))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                  newObj->untrustedRole, FALSE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Untrusted role <%s> has invalid value.",
                         newObj->untrustedRole);
            goto out;
         }
      }

      if (cmsUtl_strcmp(newObj->bannedRole, currObj->bannedRole))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                  newObj->bannedRole, FALSE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Banned role <%s> has invalid value.",
                         newObj->bannedRole);
            goto out;
         }
      }
   }

out:

   return ret;
}


CmsRet rcl_dev2LocalagentControllertrustRoleObject(_Dev2LocalagentControllertrustRoleObject *newObj,
                                const _Dev2LocalagentControllertrustRoleObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumRoleEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 changed = FALSE;
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      char *fullpath = NULL;
      MdmPathDescriptor pathDesc;
      Dev2LocalagentControllerObject *cntrlObj = NULL;
      Dev2LocalagentControllertrustObject *ctrlTrustObj = NULL;
      Dev2LocalagentControllertrustCredentialObject *credentialObj = NULL;
#ifdef DMP_DEVICE2_CHALLENGE_1
      Dev2LocalagentControllertrustChallengeObject *challengeObj = NULL;
#endif /* DMP_DEVICE2_CHALLENGE_1 */
      InstanceIdStack iiStack2;
      CmsRet ret = CMSRET_SUCCESS;

      /* FIXME: due to obuspa's limited role implementation, always keep
       * first two roles as TrustedRole and UntrustedRole and don't allow
       * admin to delete them
       */
      if (cmsUtl_strcmp(currObj->name, "TrustedRole") == 0 ||
          cmsUtl_strcmp(currObj->name, "UntrustedRole") == 0)
      {
         cmsLog_error("cannot delete TrustedRole or UntrustedRole");
         return CMSRET_REQUEST_DENIED;
      }

      rutUtil_modifyNumRoleEntry_dev2(iidStack, -1);

      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE;
      pathDesc.iidStack = *iidStack;

      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

      INIT_INSTANCE_ID_STACK(&iiStack2);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER,
                                 &iiStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&cntrlObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_isFullPathInCSL(fullpath, cntrlObj->assignedRole) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, cntrlObj->assignedRole);
            changed = TRUE;
         }
         if (cmsUtl_isFullPathInCSL(fullpath, cntrlObj->inheritedRole) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, cntrlObj->inheritedRole);
            changed = TRUE;
         }
         if (changed == TRUE)
         {
            if ((ret = cmsObj_setFlags(cntrlObj, &iiStack2,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set Dev2LocalagentControllerObject failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &cntrlObj);
      }

      changed = FALSE;
      INIT_INSTANCE_ID_STACK(&iiStack2);

      if (cmsObj_get(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST, &iiStack2,
                     OGF_NO_VALUE_UPDATE, (void **)&ctrlTrustObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_isFullPathInCSL(fullpath, ctrlTrustObj->untrustedRole) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, ctrlTrustObj->untrustedRole);
            changed = TRUE;
         }
         if (cmsUtl_isFullPathInCSL(fullpath, ctrlTrustObj->bannedRole) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, ctrlTrustObj->bannedRole);
            changed = TRUE;
         }
         if (changed == TRUE)
         {
            if ((ret = cmsObj_setFlags(ctrlTrustObj, &iiStack2,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set Dev2LocalagentControllertrustObject failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &ctrlTrustObj);
      }

      INIT_INSTANCE_ID_STACK(&iiStack2);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL,
                                 &iiStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&credentialObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_isFullPathInCSL(fullpath, credentialObj->role) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, credentialObj->role);
            if ((ret = cmsObj_setFlags(credentialObj, &iiStack2,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set Dev2LocalagentControllertrustCredentialObject failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &credentialObj);
      }

#ifdef DMP_DEVICE2_CHALLENGE_1
      INIT_INSTANCE_ID_STACK(&iiStack2);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CHALLENGE,
                                 &iiStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&challengeObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_isFullPathInCSL(fullpath, challengeObj->role) == TRUE)
         {
            cmsUtl_deleteFullPathFromCSL(fullpath, challengeObj->role);
            if ((ret = cmsObj_setFlags(challengeObj, &iiStack2,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set Dev2LocalagentControllertrustChallengeObject failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &credentialObj);
      }
#endif /* DMP_DEVICE2_CHALLENGE_1 */

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
   }
   else if (newObj && currObj)
   {
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2LocalagentControllertrustRolePermissionObject(_Dev2LocalagentControllertrustRolePermissionObject *newObj,
                                const _Dev2LocalagentControllertrustRolePermissionObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      if (IS_EMPTY_STRING(newObj->alias))
      {
         char alias[BUFLEN_256]={0};

         snprintf(alias, sizeof(alias), "cpe-role-permission-%d-%d",
                  INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-2),
                  PEEK_INSTANCE_ID(iidStack));

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->alias, alias,
                                           mdmLibCtx.allocFlags);
      }

      rutUtil_modifyNumRolePermissionEntry_dev2(iidStack, 1);
      return ret;
   }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE_PERMISSION;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      if ((newObj->enable != currObj->enable) ||
          (newObj->order != currObj->order) ||
          cmsUtl_strcmp(newObj->targets, currObj->targets) ||
          cmsUtl_strcmp(newObj->param, currObj->param) ||
          cmsUtl_strcmp(newObj->obj, currObj->obj) ||
          cmsUtl_strcmp(newObj->instantiatedObj, currObj->instantiatedObj) ||
          cmsUtl_strcmp(newObj->commandEvent, currObj->commandEvent))
      {
         ret = sendPermChanged(fullpath, currObj->enable, currObj->order,
                               currObj->targets, currObj->param, currObj->obj,
                               currObj->instantiatedObj, currObj->commandEvent,
                               FALSE);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumRolePermissionEntry_dev2(iidStack, -1);
      ret = sendPermChanged(fullpath, currObj->enable, currObj->order,
                            currObj->targets, currObj->param, currObj->obj,
                            currObj->instantiatedObj, currObj->commandEvent,
                            TRUE);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}


CmsRet rcl_dev2LocalagentControllertrustCredentialObject(_Dev2LocalagentControllertrustCredentialObject *newObj,
                                const _Dev2LocalagentControllertrustCredentialObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      rutUtil_modifyNumControllerTrustCredentialEntry_dev2(iidStack, 1);

      return ret;
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_EXISTING");

      rutUtil_modifyNumControllerTrustCredentialEntry_dev2(iidStack, -1);

      return ret;
   }

   if (newObj && currObj)
   {
      char alias[BUFLEN_256]={0};
      UBOOL8 found = FALSE;
      UINT32 id = 0;
      UINT32 instance = (UINT32) PEEK_INSTANCE_ID(iidStack);
      Dev2LocalagentControllertrustCredentialObject *credentialObj = NULL;
      InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;

      cmsLog_debug("UPDATE");

      if (IS_EMPTY_STRING(newObj->alias) && IS_EMPTY_STRING(currObj->alias))
      {
         /* Per data-model: If the value is not assigned by the 
          * Controller at creation time, the Agent MUST assign
          * a value with an "cpe-" prefix.
          */
         snprintf(alias, sizeof(alias), "cpe-credential-%d",
                  PEEK_INSTANCE_ID(iidStack));

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->alias, alias,
                                           mdmLibCtx.allocFlags);
      }
      else if (!IS_EMPTY_STRING(newObj->alias) && !IS_EMPTY_STRING(currObj->alias))
      {
         /* Per data-model: Alias is immutable and therefore
          * MUST NOT change once it has been assigned.
          */
         if (cmsUtl_strcmp(newObj->alias, currObj->alias) != 0)
         {
            cmsLog_error("Alias cannot be changed.");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      else if (IS_EMPTY_STRING(newObj->alias) && !IS_EMPTY_STRING(currObj->alias))
      {
         /* Per data-model: Alias is immutable and therefore
          * MUST NOT change once it has been assigned.
          */
         cmsLog_error("Alias cannot be changed.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (cmsUtl_strcmp(newObj->role, currObj->role))
      {
         ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                  newObj->role, FALSE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Role <%s> has invalid value.",
                         newObj->role);
            return ret;
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
            return ret;
         }
      }

      /* Per data-model: At most one entry in this table can exist 
       * with the same values for Credential
       */

      /* look for credentialObj matching with credential */
      while (found == FALSE &&
             cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_CREDENTIAL,
                                 &iidStack2,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&credentialObj) == CMSRET_SUCCESS)
      {
         id = (UINT32) PEEK_INSTANCE_ID(&iidStack2);

         if (instance != id &&
             !IS_EMPTY_STRING(credentialObj->credential) &&
             cmsUtl_strcmp(newObj->credential, credentialObj->credential) == 0)
         {
            found = TRUE;
         }

         /* free local agent controller trust credential */
         cmsObj_free((void **) &credentialObj);
      }

      /* return error if credential is already present */
      if (found == TRUE)
      {
         cmsLog_error("Credential <%s> is already present.",
                      newObj->credential);
         ret = CMSRET_INVALID_ARGUMENTS;
      }
   }

   return ret;
}


#ifdef DMP_DEVICE2_CHALLENGE_1
static CmsRet sendChallengeChanged(CmsUspMsgType type, UINT32 instance,
                                   UBOOL8 isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   UspChallengeChangedMsgBody *msgPayload = NULL;

   reqMsg=cmsMem_alloc(sizeof(CmsMsgHeader)+sizeof(UspChallengeChangedMsgBody),
                       ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   reqMsg->type = (CmsMsgType) type;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(UspChallengeChangedMsgBody);

   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (UspChallengeChangedMsgBody*)body;

   msgPayload->instanceId = instance;
   msgPayload->isDelete = isDelete;

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("sendMsg fail: type=%d instance=%d isDelete=%d ret=%d",
                   type, instance, isDelete, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2LocalagentControllertrustChallengeObject(_Dev2LocalagentControllertrustChallengeObject *newObj,
                                const _Dev2LocalagentControllertrustChallengeObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   UINT32 instance;

   instance = INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-1);

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumControllerTrustChallengeEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumControllerTrustChallengeEntry_dev2(iidStack, -1);
      sendChallengeChanged(CMS_MSG_CHALLENGE_CONFIG_CHANGE, instance, TRUE);
   }
   else if (newObj && currObj)
   {
      if (cmsUtl_strcmp(newObj->role, currObj->role))
      {
         CmsRet ret = areReferencesValid(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE,
                                         newObj->role, FALSE);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Role <%s> has invalid value.",
                         newObj->role);
            return ret;
         }
      }
   }

   return CMSRET_SUCCESS;
}
#endif /* DMP_DEVICE2_CHALLENGE_1 */

#endif /* DMP_DEVICE2_CONTROLLERTRUST_1 */


#endif /* DMP_DEVICE2_LOCALAGENT_1 */


