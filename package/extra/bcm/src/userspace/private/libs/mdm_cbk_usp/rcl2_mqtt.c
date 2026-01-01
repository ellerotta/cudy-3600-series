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


#ifdef DMP_DEVICE2_MQTTCLIENTBASE_1

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "rut_util.h"
#include "mdm.h"
#include "cms_msg_usp.h"


static CmsRet sendClientMsgChanged(const char *fullpath, int enable, int isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   MqttClientChangedMsgBody *msgPayload = NULL;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) +
                         sizeof(MqttClientChangedMsgBody), ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_MQTT_CLIENT_CONFIG_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(MqttClientChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (MqttClientChangedMsgBody*)body;

   msgPayload->enable = enable;
   msgPayload->isDelete = isDelete;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Send CMS_MSG_MQTT_CLIENT_CONFIG_CHANGE to obuspa failed, ret=%d.", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendSubsMsgChanged(const char *fullpath, int enable, int isDelete)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   MqttSubsChangedMsgBody *msgPayload = NULL;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) +
                         sizeof(MqttSubsChangedMsgBody), ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = (CmsMsgType) CMS_MSG_MQTT_SUBS_CONFIG_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = EID_OBUSPA;
   reqMsg->flags_event = 1;
   reqMsg->dataLength = sizeof(MqttSubsChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (MqttSubsChangedMsgBody*)body;

   msgPayload->enable = enable;
   msgPayload->isDelete = isDelete;
   cmsUtl_strncpy(msgPayload->fullPath, fullpath, sizeof(msgPayload->fullPath));

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Send CMS_MSG_MQTT_SUBS_CONFIG_CHANGE to obuspa failed, ret=%d.", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_dev2MqttObject( _Dev2MqttObject *newObj __attribute__((unused)),
                const _Dev2MqttObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2MqttCapabilitiesObject( _Dev2MqttCapabilitiesObject *newObj __attribute__((unused)),
                const _Dev2MqttCapabilitiesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2MqttClientObject( _Dev2MqttClientObject *newObj,
                const _Dev2MqttClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char *fullpath = NULL;
   MdmPathDescriptor pathDesc;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumClientEntry_dev2(iidStack, 1);
      return ret;
   }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_MQTT_CLIENT;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      /* notify obuspa only if any config changes */
      if (newObj->enable != currObj->enable ||
          newObj->enableEncryption != currObj->enableEncryption ||
          newObj->cleanSession != currObj->cleanSession ||
          newObj->cleanStart != currObj->cleanStart ||
          cmsUtl_strcmp(newObj->protocolVersion, currObj->protocolVersion) ||
          cmsUtl_strcmp(newObj->brokerAddress, currObj->brokerAddress) ||
          newObj->brokerPort != currObj->brokerPort ||
#ifdef DMP_DEVICE2_MQTTCLIENTEXTENDED_1
          cmsUtl_strcmp(newObj->name, currObj->name) ||
          cmsUtl_strcmp(newObj->username, currObj->username) ||
          cmsUtl_strcmp(newObj->password, currObj->password) ||
          cmsUtl_strcmp(newObj->transportProtocol, currObj->transportProtocol) ||
          newObj->connectRetryTime != currObj->connectRetryTime ||
#endif
#ifdef DMP_DEVICE2_MQTTCLIENTCON_1
          newObj->requestResponseInfo != currObj->requestResponseInfo ||
          newObj->requestProblemInfo != currObj->requestProblemInfo ||
          newObj->connectRetryIntervalMultiplier != currObj->connectRetryIntervalMultiplier ||
          newObj->connectRetryMaxInterval != currObj->connectRetryMaxInterval ||
          cmsUtl_strcmp(newObj->responseInformation, currObj->responseInformation) ||
#endif
          newObj->keepAliveTime != currObj->keepAliveTime)
      {
         ret = sendClientMsgChanged(fullpath, currObj->enable, FALSE);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      Dev2LocalagentMtpMqttObject *agentMqttObj = NULL;
      Dev2LocalagentControllerMtpMqttObject *cntrlMqttObj = NULL;
      InstanceIdStack iidMtpStack;
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;

      rutUtil_modifyMqttNumClientEntry_dev2(iidStack, -1);

      ret = sendClientMsgChanged(fullpath, currObj->enable, TRUE);

      INIT_INSTANCE_ID_STACK(&iidMtpStack);

      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_MTP_MQTT,
                                 &iidMtpStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&agentMqttObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(agentMqttObj->reference, fullpath) == 0)
         {
            CMSMEM_REPLACE_STRING_FLAGS(agentMqttObj->reference, "",
                                        mdmLibCtx.allocFlags);

            if ((ret = cmsObj_setFlags(agentMqttObj, &iidMtpStack,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set agent mtp mqtt obj failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &agentMqttObj);
      }

      INIT_INSTANCE_ID_STACK(&iidMtpStack);

      while (cmsObj_getNextFlags(MDMOID_DEV2_LOCALAGENT_CONTROLLER_MTP_MQTT,
                                 &iidMtpStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&cntrlMqttObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(cntrlMqttObj->reference, fullpath) == 0)
         {
            CMSMEM_REPLACE_STRING_FLAGS(cntrlMqttObj->reference, "",
                                        mdmLibCtx.allocFlags);

            if ((ret = cmsObj_setFlags(cntrlMqttObj, &iidMtpStack,
                                       OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Set controller mtp mqtt obj failed, ret=%d.", ret);
            }
         }

         cmsObj_free((void **) &cntrlMqttObj);
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}

#if defined(DMP_DEVICE2_MQTTCLIENTSUBSCRIBE_1)
static CmsRet checkSubscriptionEntry(const InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack1 = *iidStack;
   Dev2MqttClientObject *clientObj=NULL;
   Dev2MqttCapabilitiesObject *capObj=NULL;

   if (mdmShmCtx->inMdmInit)
   {
      cmsLog_debug("don't check count at bootup");
      return ret;
   }
   
   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_MQTT_CLIENT,
                             MDMOID_DEV2_MQTT_CLIENT_SUBSCRIPTION, &iidStack1,
                             OGF_NO_VALUE_UPDATE, (void **)&clientObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find MQTT client obj with iidStack<%s>",
                   cmsMdm_dumpIidStack(iidStack));
      goto out;
   }

   INIT_INSTANCE_ID_STACK(&iidStack1);

   ret = cmsObj_get(MDMOID_DEV2_MQTT_CAPABILITIES, &iidStack1,
                    OGF_NO_VALUE_UPDATE, (void **)&capObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find MQTT Capabilities obj");
      goto free_out;
   }

   if (clientObj->subscriptionNumberOfEntries > capObj->maxNumberOfClientSubscriptions)
   {
      cmsLog_error("too many sub entries, num<%d> max<%d>",
                   clientObj->subscriptionNumberOfEntries,
                   capObj->maxNumberOfClientSubscriptions);
      ret = CMSRET_INTERNAL_ERROR;
   }

   cmsObj_free((void **) &capObj);

free_out:
   cmsObj_free((void **) &clientObj);
out:
   return ret;
}

CmsRet rcl_dev2MqttClientSubscriptionObject( _Dev2MqttClientSubscriptionObject *newObj,
                const _Dev2MqttClientSubscriptionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char *fullpath = NULL;
   MdmPathDescriptor pathDesc;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumClientSubscriptionEntry_dev2(iidStack, 1);

      ret = checkSubscriptionEntry(iidStack);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("too many subscription entries");
         return ret;
      }

      if (IS_EMPTY_STRING(newObj->alias))
      {
         char alias[BUFLEN_256]={0};

         snprintf(alias, sizeof(alias), "cpe-mqtt-client-subscription-%d-%d",
                  INSTANCE_ID_AT_DEPTH(iidStack, DEPTH_OF_IIDSTACK(iidStack)-2),
                  PEEK_INSTANCE_ID(iidStack));

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->alias, alias, mdmLibCtx.allocFlags);
      }

      return ret;
   }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_MQTT_CLIENT_SUBSCRIPTION;
   pathDesc.iidStack = *iidStack;

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath);

   if (newObj && currObj)
   {
      /* notify obuspa only if any config changes */
      if (newObj->enable != currObj->enable ||
          newObj->qoS != currObj->qoS ||
          cmsUtl_strcmp(newObj->topic, currObj->topic))
      {
         ret = sendSubsMsgChanged(fullpath, currObj->enable, FALSE);
      }
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyMqttNumClientSubscriptionEntry_dev2(iidStack, -1);

      ret = sendSubsMsgChanged(fullpath, currObj->enable, TRUE);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   return ret;
}
#endif

#if defined(DMP_DEVICE2_MQTTCLIENTUSERPROPERTY_1)
CmsRet rcl_dev2MqttClientUserPropertyObject( _Dev2MqttClientUserPropertyObject *newObj,
                const _Dev2MqttClientUserPropertyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumClientUserPropertyEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyMqttNumClientUserPropertyEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif

#if 0    // MqttClientStats table is not supported yet.
CmsRet rcl_dev2MqttClientStatsObject( _Dev2MqttClientStatsObject *newObj __attribute__((unused)),
                const _Dev2MqttClientStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_DEVICE2_MQTTBROKERBASE_1)
CmsRet rcl_dev2MqttBrokerObject( _Dev2MqttBrokerObject *newObj,
                const _Dev2MqttBrokerObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_DEVICE2_MQTTBROKERBASE_1) && defined(DMP_DEVICE2_MQTTBROKERBRIDGEBASE_1)
CmsRet rcl_dev2MqttBrokerBridgeObject( _Dev2MqttBrokerBridgeObject *newObj,
                const _Dev2MqttBrokerBridgeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerBridgeEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerBridgeEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_DEVICE2_MQTTBROKERBASE_1) && defined(DMP_DEVICE2_MQTTBROKERBRIDGEBASE_1)
CmsRet rcl_dev2MqttBrokerBridgeServerObject( _Dev2MqttBrokerBridgeServerObject *newObj,
                const _Dev2MqttBrokerBridgeServerObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerBridgeServerEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerBridgeServerEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_DEVICE2_MQTTBROKERBASE_1) && defined(DMP_DEVICE2_MQTTBROKERBRIDGEBASE_1)
CmsRet rcl_dev2MqttBrokerBridgeSubscriptionObject( _Dev2MqttBrokerBridgeSubscriptionObject *newObj,
                const _Dev2MqttBrokerBridgeSubscriptionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerBridgeSubscriptionEntry_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyMqttNumBrokerBridgeSubscriptionEntry_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_DEVICE2_MQTTBROKERBASE_1)
CmsRet rcl_dev2MqttBrokerStatsObject( _Dev2MqttBrokerStatsObject *newObj __attribute__((unused)),
                const _Dev2MqttBrokerStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif


#endif /* DMP_DEVICE2_MQTTCLIENTBASE_1 */

